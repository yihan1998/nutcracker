#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <linux/if_ether.h>

#include <doca_flow.h>
#include <doca_log.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_flow.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_version.h>

#include "utils/printk.h"
#include "backend/doca.h"
#include "drivers/doca/common.h"
#include "drivers/doca/context.h"

#define PULL_TIME_OUT 10000						/* Maximum timeout for pulling */

struct entries_status {
	bool failure;		/* will be set to true if some entry status will not be success */
	int nb_processed;	/* will hold the number of entries that was already processed */
	void *ft_entry;		/* pointer to struct vxlan_fwd_ft_entry */
};

/* Convert IPv4 address to big endian */
#define BE_IPV4_ADDR(a, b, c, d) \
	(RTE_BE32(((uint32_t)a<<24) + (b<<16) + (c<<8) + d))

/* Set the MAC address with respect to the given 6 bytes */
#define SET_MAC_ADDR(addr, a, b, c, d, e, f)\
do {\
	addr[0] = a & 0xff;\
	addr[1] = b & 0xff;\
	addr[2] = c & 0xff;\
	addr[3] = d & 0xff;\
	addr[4] = e & 0xff;\
	addr[5] = f & 0xff;\
} while (0)

#define NB_ACTIONS_ARR	(1)

int doca_create_hw_pipe_for_port(struct doca_flow_pipe **pipe, struct flow_pipe_cfg* pipe_cfg, int port_id, struct flow_fwd* fwd, struct flow_fwd* fwd_miss) {
	struct doca_flow_match doca_match;
	struct doca_flow_fwd doca_fwd, doca_fwd_miss, * doca_fwd_ptr = NULL, *doca_fwd_miss_ptr = NULL;
	struct doca_flow_actions doca_actions, *doca_actions_arr[NB_ACTIONS_ARR];
	struct doca_flow_pipe_cfg doca_cfg;
	struct doca_flow_pipe *doca_pipe;
	struct doca_flow_pipe_entry *entry;
	struct entries_status status = {0};
	int num_of_entries = 1;
	doca_error_t result;

	memset(&doca_match, 0, sizeof(doca_match));
	memset(&doca_actions, 0, sizeof(doca_actions));
	memset(&doca_fwd, 0, sizeof(doca_fwd));
	memset(&doca_fwd_miss, 0, sizeof(doca_fwd_miss));
	memset(&doca_cfg, 0, sizeof(doca_cfg));

	doca_cfg.attr.name = pipe_cfg->attr.name;
	doca_cfg.attr.type = DOCA_FLOW_PIPE_BASIC;
	doca_cfg.attr.is_root = pipe_cfg->attr.is_root;
	doca_cfg.attr.domain = (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_EGRESS)? DOCA_FLOW_PIPE_DOMAIN_EGRESS : DOCA_FLOW_PIPE_DOMAIN_DEFAULT;
	doca_cfg.port = ports[port_id];

	if (pipe_cfg->match) {
		/* Set match.meta */
		// doca_match.meta.pkt_meta = pipe_cfg->match->meta.pkt_meta;
		// memcpy(doca_match.meta.u32, pipe_cfg->match->meta.u32, 4 * sizeof(uint32_t));

		// memcpy(doca_match.outer.eth.src_mac, pipe_cfg->match->outer.eth.h_source, 6);
		// memcpy(doca_match.outer.eth.dst_mac, pipe_cfg->match->outer.eth.h_dest, 6);
		// doca_match.outer.eth.type = pipe_cfg->match->outer.eth.h_proto;
		/* Set outer */
		if (pipe_cfg->match->outer.l3_type == FLOW_L3_TYPE_IP4) {
			doca_match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
			switch (pipe_cfg->match->outer.l4_type_ext)
			{
				case FLOW_L4_TYPE_EXT_UDP:
					/* Only set l3_type, setting dst_ip and src_ip will cause matching failure */
					// doca_match.outer.udp.l4_port.dst_port = pipe_cfg->match->outer.udp.dest;
					// doca_match.outer.udp.l4_port.src_port = pipe_cfg->match->outer.udp.source;
					doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
					break;

				case FLOW_L4_TYPE_EXT_TCP:
					doca_match.outer.ip4.dst_ip = pipe_cfg->match->outer.ip4.daddr;
					doca_match.outer.ip4.src_ip = pipe_cfg->match->outer.ip4.saddr;
					doca_match.outer.tcp.l4_port.dst_port = pipe_cfg->match->outer.tcp.dest;
					doca_match.outer.tcp.l4_port.src_port = pipe_cfg->match->outer.tcp.source;
					doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
					break;

				default:
					break;
			}
		}

		doca_cfg.match = &doca_match;
	}

	// doca_match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
	// doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
	// doca_match.outer.udp.l4_port.dst_port = 0xffff;

	doca_actions.meta.pkt_meta = UINT32_MAX;
	doca_cfg.attr.nb_actions = NB_ACTIONS_ARR;
	doca_actions_arr[0] = &doca_actions;
	doca_cfg.actions = doca_actions_arr;
	// doca_cfg.match = &doca_match;

	if (pipe_cfg->attr.nb_actions > 0) {
		// doca_cfg.attr.nb_actions=pipe_cfg->attr.nb_actions;

		/* Only have 1 action */
		for (int i = 0; i < pipe_cfg->attr.nb_actions; i++) {
			struct flow_actions* action = pipe_cfg->actions[i];
			if (action->meta.pkt_meta) {
				doca_actions.meta.pkt_meta = action->meta.pkt_meta;
			}
			if (action->has_encap) {
				doca_actions.has_encap = true;
				memcpy(doca_actions.encap.outer.eth.src_mac, action->outer.eth.h_source, ETH_ALEN);
				memcpy(doca_actions.encap.outer.eth.dst_mac, action->outer.eth.h_dest, ETH_ALEN);
				doca_actions.encap.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
				doca_actions.encap.outer.ip4.src_ip = htonl(action->outer.ip4.saddr);
				doca_actions.encap.outer.ip4.dst_ip = htonl(action->outer.ip4.daddr);
				doca_actions.encap.outer.ip4.ttl = 0xff;
				doca_actions.encap.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
				doca_actions.encap.outer.udp.l4_port.src_port = htons(action->outer.udp.source);
				doca_actions.encap.outer.udp.l4_port.dst_port = htons(action->outer.udp.dest);
				doca_actions.encap.tun.type = DOCA_FLOW_TUN_VXLAN;
				doca_actions.encap.tun.vxlan_tun_id = 0xffffffff;

				// SET_MAC_ADDR(doca_actions.encap.outer.eth.src_mac, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
				// SET_MAC_ADDR(doca_actions.encap.outer.eth.dst_mac, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
				// doca_actions.encap.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
				// doca_actions.encap.outer.ip4.src_ip = 0xffffffff;
				// doca_actions.encap.outer.ip4.dst_ip = 0xffffffff;
				// doca_actions.encap.outer.ip4.ttl = 0xff;
				// doca_actions.encap.tun.type = DOCA_FLOW_TUN_VXLAN;
				// doca_actions.encap.tun.vxlan_tun_id = 0xffffffff;

			} else {
				memcpy(doca_actions.outer.eth.src_mac, action->outer.eth.h_source, ETH_ALEN);
				memcpy(doca_actions.outer.eth.dst_mac, action->outer.eth.h_dest, ETH_ALEN);
			}
		}
	}

	/* Set fwd */
	if (fwd) {
		if (fwd->type == FLOW_FWD_RSS) {
			doca_fwd.next_pipe = rss_pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_HAIRPIN) {
			doca_fwd.next_pipe = hairpin_pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_PORT) {
			doca_fwd.port_id = port_id;
			doca_fwd.type = DOCA_FLOW_FWD_PORT;
		} else if (fwd->type == FLOW_FWD_PIPE) {
			doca_fwd.next_pipe = fwd->next_pipe->hwPipe.pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_DROP) {
			doca_fwd.type = DOCA_FLOW_FWD_DROP;
		} else {
			printf("Unknown fwd type! (%d)\n", fwd->type);
		}

		doca_fwd_ptr = &doca_fwd;
	}

	/* Set fwd_miss */
	if (fwd_miss) {
		if (fwd_miss->type == FLOW_FWD_RSS) {
			doca_fwd_miss.next_pipe = rss_pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd_miss->type == FLOW_FWD_HAIRPIN) {
			doca_fwd_miss.next_pipe = hairpin_pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd_miss->type == FLOW_FWD_PORT) {
			doca_fwd_miss.port_id = port_id;
			doca_fwd_miss.type = DOCA_FLOW_FWD_PORT;
		} else if (fwd_miss->type == FLOW_FWD_PIPE) {
			doca_fwd_miss.next_pipe = fwd_miss->next_pipe->hwPipe.pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd_miss->type == FLOW_FWD_DROP) {
			doca_fwd_miss.type = DOCA_FLOW_FWD_DROP;
		} else {
			printf("Unknown fwd miss type! (%d)\n", fwd_miss->type);
		}

		doca_fwd_miss_ptr = &doca_fwd_miss;
	}

	result = doca_flow_pipe_create(&doca_cfg, doca_fwd_ptr, doca_fwd_miss_ptr, &doca_pipe);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		printf(LIGHT_RED "[ERR]" RESET " Failed to create pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf(LIGHT_RED "[ERR]" RESET " Failed to create pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
		return result;
	}

	if (doca_fwd_ptr) {
		result = doca_flow_pipe_add_entry(0, doca_pipe, &doca_match, &doca_actions, NULL, doca_fwd_ptr, 0, &status, &entry);
		if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
			printf(LIGHT_RED "[ERR]" RESET " Failed to add entry to pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf(LIGHT_RED "[ERR]" RESET " Failed to add entry to pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
			return -1;
		}

		result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
		if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
			printf(LIGHT_RED "[ERR]" RESET " Failed to process entry to pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
			printf(LIGHT_RED "[ERR]" RESET " Failed to process entry to pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
			return -1;
		}
	}

	*pipe = doca_pipe;

	// printf(GREEN "[INFO]" RESET " New DOCA pipe %s created on port %d: %p\n", doca_cfg.attr.name, port_id, doca_pipe);

	return DOCA_SUCCESS;
}

int doca_create_hw_pipe(struct flow_pipe* pipe, struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss) {
    int portid;
	RTE_ETH_FOREACH_DEV(portid) {
	    doca_create_hw_pipe_for_port(&pipe->hwPipe.pipe[portid], pipe_cfg, portid, fwd, fwd_miss);
	}
	return DOCA_SUCCESS;
}

int doca_create_hw_control_pipe_for_port(int port_id, struct doca_flow_pipe **pipe, struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss) {
	struct doca_flow_pipe_cfg doca_cfg;
	// struct doca_flow_fwd doca_fwd;
	struct doca_flow_fwd doca_fwd_miss;
	struct doca_flow_fwd *doca_fwd_miss_ptr = NULL;
	doca_error_t result;

	memset(&doca_cfg, 0, sizeof(doca_cfg));
	// memset(&doca_fwd, 0, sizeof(doca_fwd));
	memset(&doca_fwd_miss, 0, sizeof(doca_fwd_miss));

	doca_cfg.attr.name = pipe_cfg->attr.name;
	doca_cfg.attr.type = DOCA_FLOW_PIPE_CONTROL;
	doca_cfg.attr.is_root = pipe_cfg->attr.is_root;
	doca_cfg.attr.domain = (pipe_cfg->attr.domain == FLOW_PIPE_DOMAIN_EGRESS)? DOCA_FLOW_PIPE_DOMAIN_EGRESS : DOCA_FLOW_PIPE_DOMAIN_DEFAULT;
	doca_cfg.port = ports[port_id];

	// /* Set fwd_miss */
	if (fwd_miss) {
		if (fwd_miss->type == FLOW_FWD_RSS) {
			doca_fwd_miss.next_pipe = rss_pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd_miss->type == FLOW_FWD_HAIRPIN) {
			doca_fwd_miss.next_pipe = hairpin_pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd_miss->type == FLOW_FWD_PORT) {
			doca_fwd_miss.port_id = port_id;
			doca_fwd_miss.type = DOCA_FLOW_FWD_PORT;
		} else if (fwd_miss->type == FLOW_FWD_PIPE) {
			doca_fwd_miss.next_pipe = fwd_miss->next_pipe->hwPipe.pipe[port_id];
			doca_fwd_miss.type = DOCA_FLOW_FWD_PIPE;
		} else {
			printf("Unknown fwd type! (%d)\n", fwd_miss->type);
		}

		doca_fwd_miss_ptr = &doca_fwd_miss;
	}

	result = doca_flow_pipe_create(&doca_cfg, NULL, doca_fwd_miss_ptr, pipe);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		printf(LIGHT_RED "[ERR]" RESET " Failed to create pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf(LIGHT_RED "[ERR]" RESET " Failed to create pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
		return -1;
	}
	return 0;
}

int doca_create_hw_control_pipe(struct flow_pipe* pipe, struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss) {
    int portid;
	RTE_ETH_FOREACH_DEV(portid) {
	    doca_create_hw_control_pipe_for_port(portid, &pipe->hwPipe.pipe[portid], pipe_cfg, fwd, fwd_miss);
	}
	return DOCA_SUCCESS;
}

int doca_hw_control_pipe_add_entry_for_port(int port_id, struct doca_flow_pipe *pipe, int priority, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd) {
	struct doca_flow_match doca_match;
	struct doca_flow_actions doca_actions;
	struct doca_flow_fwd doca_fwd;
	struct doca_flow_pipe_entry *entry;
	doca_error_t result;

	memset(&doca_match, 0, sizeof(doca_match));
	memset(&doca_actions, 0, sizeof(doca_actions));
	memset(&doca_fwd, 0, sizeof(doca_fwd));	

	switch (match->outer.l3_type)
	{
	case FLOW_L3_TYPE_IP4:
		doca_match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
		break;
	
	default:
		break;
	}

	switch (match->outer.ip4.protocol) {
	case IPPROTO_UDP:
		doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
		break;

	default:
		break;
	}

	switch (fwd->type)
	{
	case FLOW_FWD_PIPE:
		doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		doca_fwd.next_pipe = fwd->next_pipe->hwPipe.pipe[port_id];
		// doca_fwd.next_pipe = rss_pipe[port_id];
		break;

	case FLOW_FWD_DROP:
		doca_fwd.type = DOCA_FLOW_FWD_DROP;
		break;

	default:
		break;
	}

	doca_actions.meta.pkt_meta = actions->meta.pkt_meta;

	result = doca_flow_pipe_control_add_entry(0, priority, pipe, &doca_match,
						 NULL, &doca_actions, NULL, NULL, NULL, &doca_fwd, NULL, &entry);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		printf(LIGHT_RED "[ERR]" RESET " Failed to add entry to pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf(LIGHT_RED "[ERR]" RESET " Failed to add entry to pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
		return -1;
	}

	return 0;
}

int doca_hw_control_pipe_add_entry(struct flow_pipe* pipe, int priority, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd) {
    int portid;
	RTE_ETH_FOREACH_DEV(portid) {
	    doca_hw_control_pipe_add_entry_for_port(portid, pipe->hwPipe.pipe[portid], priority, match, actions, fwd);
	}
	return DOCA_SUCCESS;
}

int doca_hw_pipe_add_entry_for_port(int port_id, struct doca_flow_pipe *pipe, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd) {
	struct doca_flow_match doca_match;
	struct doca_flow_actions doca_actions;
	struct doca_flow_fwd doca_fwd;
	struct doca_flow_pipe_entry *entry;
	struct entries_status status = {0};
	int num_of_entries = 1;
	doca_error_t result;

	printf(GREEN "[INFO]" RESET " Adding entry to pipe on port %d...\n", port_id);

	memset(&doca_match, 0, sizeof(doca_match));
	memset(&doca_actions, 0, sizeof(doca_actions));
	memset(&doca_fwd, 0, sizeof(doca_fwd));

	if (match) {
		/* Set match.meta */
		doca_match.meta.pkt_meta = match->meta.pkt_meta;
		memcpy(doca_match.meta.u32, match->meta.u32, 4 * sizeof(uint32_t));

		if (match->outer.l3_type == FLOW_L3_TYPE_IP4) {
			doca_match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
			switch (match->outer.l4_type_ext)
			{
				case FLOW_L4_TYPE_EXT_UDP:
					doca_match.outer.udp.l4_port.dst_port = htons(match->outer.udp.dest);
					doca_match.outer.udp.l4_port.src_port = htons(match->outer.udp.source);
					doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
					break;

				case FLOW_L4_TYPE_EXT_TCP:
					doca_match.outer.ip4.dst_ip = match->outer.ip4.daddr;
					doca_match.outer.ip4.src_ip = match->outer.ip4.saddr;
					doca_match.outer.tcp.l4_port.dst_port = match->outer.tcp.dest;
					doca_match.outer.tcp.l4_port.src_port = match->outer.tcp.source;
					doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
					break;

				default:
					break;
			}
		}
	}
	// doca_match.outer.udp.l4_port.dst_port = htons(1234);
	// doca_match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
	// doca_match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;

	if (actions) {
		if (actions->meta.pkt_meta) {
			doca_actions.meta.pkt_meta = actions->meta.pkt_meta;
		}
		if (actions->has_encap) {
			doca_actions.has_encap = true;
			memcpy(doca_actions.encap.outer.eth.src_mac, actions->outer.eth.h_source, ETH_ALEN);
			memcpy(doca_actions.encap.outer.eth.dst_mac, actions->outer.eth.h_dest, ETH_ALEN);
			doca_actions.encap.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
			doca_actions.encap.outer.ip4.src_ip = htonl(actions->outer.ip4.saddr);
			doca_actions.encap.outer.ip4.dst_ip = htonl(actions->outer.ip4.daddr);
			doca_actions.encap.outer.ip4.ttl = 17;
			doca_actions.encap.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
			doca_actions.encap.outer.udp.l4_port.src_port = htons(actions->outer.udp.source);
			doca_actions.encap.outer.udp.l4_port.dst_port = htons(actions->outer.udp.dest);
			doca_actions.encap.tun.type = DOCA_FLOW_TUN_VXLAN;
			doca_actions.encap.tun.vxlan_tun_id = actions->encap.tun.vxlan_tun_id;
		
			// doca_be32_t encap_dst_ip_addr = BE_IPV4_ADDR(81, 81, 81, 81);
			// doca_be32_t encap_src_ip_addr = BE_IPV4_ADDR(11, 21, 31, 41);
			// uint8_t encap_ttl = 17;
			// doca_be32_t encap_vxlan_tun_id = BUILD_VNI(0xadadad);
			// uint8_t src_mac[] = {0xde, 0xaf, 0x98, 0xef, 0x05, 0x6d};
			// uint8_t dst_mac[] = {0x10, 0x70, 0xfd, 0xc8, 0x94, 0x75};
			// doca_actions.has_encap = true;
			// SET_MAC_ADDR(doca_actions.encap.outer.eth.src_mac, src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
			// SET_MAC_ADDR(doca_actions.encap.outer.eth.dst_mac, dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
			// doca_actions.encap.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
			// doca_actions.encap.outer.ip4.src_ip = encap_src_ip_addr;
			// doca_actions.encap.outer.ip4.dst_ip = encap_dst_ip_addr;
			// doca_actions.encap.outer.ip4.ttl = encap_ttl;
			// doca_actions.encap.tun.type = DOCA_FLOW_TUN_VXLAN;
			// doca_actions.encap.tun.vxlan_tun_id = encap_vxlan_tun_id;			
		} else {
			uint8_t src_mac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
			uint8_t dst_mac[] = {0x10, 0x70, 0xFD, 0xC8, 0x94, 0x75};
			SET_MAC_ADDR(doca_actions.outer.eth.src_mac, src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
			SET_MAC_ADDR(doca_actions.outer.eth.dst_mac, dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
			doca_actions.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
		}
	}

	if (fwd) {
		if (fwd->type == FLOW_FWD_RSS) {
			doca_fwd.next_pipe = rss_pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_HAIRPIN) {
			doca_fwd.next_pipe = hairpin_pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_PORT) {
			doca_fwd.port_id = port_id;
			doca_fwd.type = DOCA_FLOW_FWD_PORT;
		} else if (fwd->type == FLOW_FWD_PIPE) {
			doca_fwd.next_pipe = fwd->next_pipe->hwPipe.pipe[port_id];
			doca_fwd.type = DOCA_FLOW_FWD_PIPE;
		} else if (fwd->type == FLOW_FWD_DROP) {
			doca_fwd.type = DOCA_FLOW_FWD_DROP;
		} else {
			printf("Unknown fwd type! (%d)\n", fwd->type);
		}
	}

	result = doca_flow_pipe_add_entry(0, pipe, &doca_match, &doca_actions, NULL, &doca_fwd, 0, &status, &entry);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		pr_err("Failed to add entry to pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		pr_err("Failed to add entry to pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		pr_err("Failed to process entry to pipe on port %d (%s)\n", port_id, doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		pr_err("Failed to process entry to pipe on port %d (%s)\n", port_id, doca_error_get_descr(result));
#endif
		return -1;
	}

	return DOCA_SUCCESS;
}

int doca_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd) {
    int portid;
	RTE_ETH_FOREACH_DEV(portid) {
	    doca_hw_pipe_add_entry_for_port(portid, pipe->hwPipe.pipe[portid], match, actions, fwd);
	}
	return DOCA_SUCCESS;
}

int vxlan_encap_offloading() {
	int port_id = 1234;
	/* Add ingress */
	if (1) {
		struct flow_pipe * pipe = flow_get_pipe("ingress_udp_tbl_0");
		// pipe->hwPipe.ops.add_pipe_entry(pipe, "ingress_hairpin_2", 1234);
		for (int i = 0; i < 10; i++) {
			pipe->hwPipe.ops.add_pipe_entry(pipe, "ingress_hairpin_2", port_id + i);
		}
	}

	/* Add egress */
	if (1) {
		{
			struct flow_pipe * pipe = flow_get_pipe("egress_vxlan_encap_tbl_2");
    		// pipe->hwPipe.ops.add_pipe_entry(pipe, "egress_encap_3", 1234);
			for (int i = 0; i < 10; i++) {
				pipe->hwPipe.ops.add_pipe_entry(pipe, "egress_encap_3", port_id + i);
			}
		}
		{
			struct flow_pipe * pipe = flow_get_pipe("egress_encap_3");
    		uint8_t srcMac[6] = {0xde,0xed,0xbe,0xef,0xab,0xcd};
    		uint8_t dstMac[6] = {0x10,0x70,0xfd,0xc8,0x94,0x75};
    		// pipe->hwPipe.ops.add_pipe_entry(pipe, "egress_fwd_port_4", srcMac, dstMac, 5678, 1234, 90);
			for (int i = 0; i < 10; i++) {
				pipe->hwPipe.ops.add_pipe_entry(pipe, "egress_fwd_port_4", dstMac, srcMac, BE_IPV4_ADDR(8,8,8,8), BE_IPV4_ADDR(1,2,3,4), 5678, port_id + 1, 90);
			}
		}
	}
	return DOCA_SUCCESS;
}
