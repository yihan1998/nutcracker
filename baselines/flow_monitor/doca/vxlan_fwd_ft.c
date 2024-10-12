/*
 * Copyright (c) 2021-2022 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <doca_flow.h>
#include <doca_log.h>

#include "vxlan_fwd.h"
#include "vxlan_fwd_ft.h"

DOCA_LOG_REGISTER(VXLAN_FWD_FT);

/* Bucket is a struct encomassing the list and the synchronization mechanism used for accessing the flows list */
struct vxlan_fwd_ft_bucket {
	struct vxlan_fwd_ft_entry_head head;	/* The head of the list of the flows */
	rte_spinlock_t lock;			/* Lock, a synchronization mechanism */
};

/* Stats for the flow table */
struct vxlan_fwd_ft_stats {
	uint64_t add;		/* Number of insertions to the flow table */
	uint64_t rm;		/* Number of removals from the flow table */
	uint64_t memuse;	/* Memory ysage of the flow table */
};

/* Flow table configuration */
struct vxlan_fwd_ft_cfg {
	uint32_t size;			/* Nuumber of maximum flows in a given time while the application is running */
	uint32_t mask;			/* Masking; */
	uint32_t user_data_size;	/* User data size needed for allocation */
	uint32_t entry_size;		/* Size needed for storing a single entry flow */
};

/* Flow table as represented in the application */
struct vxlan_fwd_ft {
	struct vxlan_fwd_ft_cfg cfg;						/* Flow table configurations */
	struct vxlan_fwd_ft_stats stats;					/* Stats for the flow table */
	uint32_t fid_ctr;							/* Flow table ID , used for controlling the flow table */
	void (*vxlan_fwd_aging_cb)(struct vxlan_fwd_ft_user_ctx *ctx);	/* Callback holder; callback for handling aged flows */
	void (*vxlan_fwd_aging_hw_cb)(void);					/* HW callback holder; callback for handling aged flows*/
	struct vxlan_fwd_ft_bucket buckets[0];					/* Pointer for the Bucket in the flow table; list of entries */
};

void
vxlan_fwd_ft_update_age_sec(struct vxlan_fwd_ft_entry *e, uint32_t age_sec)
{
	e->age_sec = age_sec;
}

void
vxlan_fwd_ft_update_expiration(struct vxlan_fwd_ft_entry *e)
{
	if (e->age_sec)
		e->expiration = rte_rdtsc() + rte_get_timer_hz() * e->age_sec;
}

/*
 * Destroy flow entry in the flow table
 *
 * @ft [in]: the flow table to remove the entry from
 * @ft_entry [in]: entry flow to remove, as represented in the application
 */
static void
_ft_destroy_entry(struct vxlan_fwd_ft *ft,
		  struct vxlan_fwd_ft_entry *ft_entry)
{
	LIST_REMOVE(ft_entry, next);
	ft->vxlan_fwd_aging_cb(&ft_entry->user_ctx);
	free(ft_entry);
	ft->stats.rm++;
}

void
vxlan_fwd_ft_destroy_entry(struct vxlan_fwd_ft *ft,
			    struct vxlan_fwd_ft_entry *ft_entry)
{
	int idx = ft_entry->buckets_index;

	rte_spinlock_lock(&ft->buckets[idx].lock);
	_ft_destroy_entry(ft, ft_entry);
	rte_spinlock_unlock(&ft->buckets[idx].lock);
}

/*
 * Build table key according to parsed packet.
 *
 * @pinfo [in]: the packet's info
 * @key [out]: the generated key
 * @return: 0 on success and negative value otherwise
 */
static int
vxlan_fwd_ft_key_fill(struct vxlan_fwd_pkt_info *pinfo,
		       struct vxlan_fwd_ft_key *key)
{
	bool inner = false;

	if (pinfo->tun_type != DOCA_FLOW_TUN_NONE)
		inner = true;

	/* support ipv6 */
	if (pinfo->outer.l3_type != IPV4)
		return -1;

	key->rss_hash = pinfo->rss_hash;
	/* 5-tuple of inner if there is tunnel or outer if none */
	key->protocol = inner ? pinfo->inner.l4_type : pinfo->outer.l4_type;
	key->ipv4_1 = vxlan_fwd_ft_key_get_ipv4_src(inner, pinfo);
	key->ipv4_2 = vxlan_fwd_ft_key_get_ipv4_dst(inner, pinfo);
	key->port_1 = vxlan_fwd_ft_key_get_src_port(inner, pinfo);
	key->port_2 = vxlan_fwd_ft_key_get_dst_port(inner, pinfo);
	key->port_id = pinfo->orig_port_id;

	/* in case of tunnel , use tun type and vni */
	if (pinfo->tun_type != DOCA_FLOW_TUN_NONE) {
		key->tun_type = pinfo->tun_type;
		key->vni = pinfo->tun.vni;
	}
	return 0;
}

/*
 * Compare keys
 *
 * @key1 [in]: first key for comparison
 * @key2 [in]: first key for comparison
 * @return: true if keys are equal, false otherwise
 */
static bool
vxlan_fwd_ft_key_equal(struct vxlan_fwd_ft_key *key1,
			struct vxlan_fwd_ft_key *key2)
{
	uint64_t *keyp1 = (uint64_t *)key1;
	uint64_t *keyp2 = (uint64_t *)key2;
	uint64_t res = keyp1[0] ^ keyp2[0];

	res |= keyp1[1] ^ keyp2[1];
	res |= keyp1[2] ^ keyp2[2];
	return (res == 0);
}

struct vxlan_fwd_ft *
vxlan_fwd_ft_create(int nb_flows, uint32_t user_data_size,
	       void (*vxlan_fwd_aging_cb)(struct vxlan_fwd_ft_user_ctx *ctx),
	       void (*vxlan_fwd_aging_hw_cb)(void))
{
	struct vxlan_fwd_ft *ft;
	uint32_t nb_flows_aligned;
	uint32_t alloc_size;
	uint32_t i;

	if (nb_flows <= 0)
		return NULL;
	/* Align to the next power of 2, 32bits integer is enough now */
	if (!rte_is_power_of_2(nb_flows))
		nb_flows_aligned = rte_align32pow2(nb_flows);
	else
		nb_flows_aligned = nb_flows;
	/* double the flows to avoid collisions */
	nb_flows_aligned <<= 1;
	alloc_size = sizeof(struct vxlan_fwd_ft)
		+ sizeof(struct vxlan_fwd_ft_bucket) * nb_flows_aligned;
	DOCA_DLOG_DBG("Malloc size =%d", alloc_size);

	ft = calloc(1, alloc_size);
	if (ft == NULL) {
		DOCA_LOG_ERR("No memory");
		return NULL;
	}
	ft->cfg.entry_size = sizeof(struct vxlan_fwd_ft_entry)
		+ user_data_size;
	ft->cfg.user_data_size = user_data_size;
	ft->cfg.size = nb_flows_aligned;
	ft->cfg.mask = nb_flows_aligned - 1;
	ft->vxlan_fwd_aging_cb = vxlan_fwd_aging_cb;
	ft->vxlan_fwd_aging_hw_cb = vxlan_fwd_aging_hw_cb;

	DOCA_DLOG_DBG("FT created: flows=%d, user_data_size=%d", nb_flows_aligned,
		     user_data_size);
	for (i = 0; i < ft->cfg.size; i++)
		rte_spinlock_init(&ft->buckets[i].lock);
	return ft;
}

/*
 * find if there is an existing entry matching the given packet generated key
 *
 * @ft [in]: flow table to search in
 * @key [in]: the packet generated key used for search in the flow table
 * @return: pointer to the flow entry if found, NULL otherwise
 */
static struct vxlan_fwd_ft_entry*
_vxlan_fwd_ft_find(struct vxlan_fwd_ft *ft,
		    struct vxlan_fwd_ft_key *key)
{
	uint32_t idx;
	struct vxlan_fwd_ft_entry_head *first;
	struct vxlan_fwd_ft_entry *node;

	idx = key->rss_hash & ft->cfg.mask;
	DOCA_DLOG_DBG("Looking for index %d", idx);
	first = &ft->buckets[idx].head;
	LIST_FOREACH(node, first, next) {
		if (vxlan_fwd_ft_key_equal(&node->key, key)) {
			vxlan_fwd_ft_update_expiration(node);
			return node;
		}
	}
	return NULL;
}

doca_error_t
vxlan_fwd_ft_find(struct vxlan_fwd_ft *ft,
		   struct vxlan_fwd_pkt_info *pinfo,
		   struct vxlan_fwd_ft_user_ctx **ctx)
{
	doca_error_t result = DOCA_SUCCESS;
	struct vxlan_fwd_ft_entry *fe;
	struct vxlan_fwd_ft_key key = {0};

	if (vxlan_fwd_ft_key_fill(pinfo, &key)) {
		result = DOCA_ERROR_UNEXPECTED;
		DOCA_LOG_DBG("Failed to build key for entry in the flow table %s", doca_get_error_string(result));
		return result;
	}

	fe = _vxlan_fwd_ft_find(ft, &key);
	if (fe == NULL) {
		result = DOCA_ERROR_NOT_FOUND;
		DOCA_LOG_DBG("Entry not found in flow table %s", doca_get_error_string(result));
		return result;
	}

	*ctx = &fe->user_ctx;
	return DOCA_SUCCESS;
}

doca_error_t
vxlan_fwd_ft_add_new(struct vxlan_fwd_ft *ft,
		      struct vxlan_fwd_pkt_info *pinfo,
		      struct vxlan_fwd_ft_user_ctx **ctx)
{
	doca_error_t result = DOCA_SUCCESS;
	int idx;
	struct vxlan_fwd_ft_key key = {0};
	struct vxlan_fwd_ft_entry *new_e;
	struct vxlan_fwd_ft_entry_head *first;

	if (!ft)
		return false;

	if (vxlan_fwd_ft_key_fill(pinfo, &key)) {
		result = DOCA_ERROR_UNEXPECTED;
		DOCA_LOG_DBG("Failed to build key: %s", doca_get_error_string(result));
		return result;
	}

	new_e = calloc(1, ft->cfg.entry_size);
	if (new_e == NULL) {
		result = DOCA_ERROR_NO_MEMORY;
		DOCA_LOG_WARN("OOM: %s", doca_get_error_string(result));
		return result;
	}

	vxlan_fwd_ft_update_expiration(new_e);
	new_e->user_ctx.fid = ft->fid_ctr++;
	*ctx = &new_e->user_ctx;

	DOCA_DLOG_DBG("Defined new flow %llu",
		     (unsigned int long long)new_e->user_ctx.fid);
	memcpy(&new_e->key, &key, sizeof(struct vxlan_fwd_ft_key));
	idx = pinfo->rss_hash & ft->cfg.mask;
	new_e->buckets_index = idx;
	first = &ft->buckets[idx].head;

	rte_spinlock_lock(&ft->buckets[idx].lock);
	LIST_INSERT_HEAD(first, new_e, next);
	rte_spinlock_unlock(&ft->buckets[idx].lock);
	ft->stats.add++;
	return result;
}

doca_error_t
vxlan_fwd_ft_destroy(struct vxlan_fwd_ft *ft)
{
	uint32_t i;
	struct vxlan_fwd_ft_entry *node, *ptr;

	if (ft == NULL)
		return DOCA_ERROR_INVALID_VALUE;
	for (i = 0; i < ft->cfg.size; i++) {
		node = LIST_FIRST(&ft->buckets[i].head);
		while (node != NULL) {
			ptr = LIST_NEXT(node, next);
			_ft_destroy_entry(ft, node);
			node = ptr;
		}
	}
	free(ft);
	return DOCA_SUCCESS;
}