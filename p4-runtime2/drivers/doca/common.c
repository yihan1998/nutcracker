#include <stdlib.h>

#include "net/dpdk.h"

#ifdef CONFIG_DOCA
#include "utils/printk.h"
#include "backend/flowPipeInternal.h"
#include "drivers/doca/common.h"
#include "drivers/doca/compress.h"

#include <doca_flow.h>

/* Convert IPv4 address to big endian */
#define BE_IPV4_ADDR(a, b, c, d) \
	(RTE_BE32(((uint32_t)a<<24) + (b<<16) + (c<<8) + d))

int nb_ports = 2;
struct doca_flow_port *ports[MAX_PORTS];
struct doca_flow_pipe *rss_pipe[2], *hairpin_pipe[2];

struct entries_status {
	bool failure;		/* will be set to true if some entry status will not be success */
	int nb_processed;	/* will hold the number of entries that was already processed */
	void *ft_entry;		/* pointer to struct vxlan_fwd_ft_entry */
};

#define NB_ACTION_ARRAY	(1)

#define PULL_TIME_OUT 10000

doca_error_t open_doca_device_with_pci(const char *pci_addr, jobs_check func, struct doca_dev **retval) {
	struct doca_devinfo **dev_list;
	uint32_t nb_devs;
	uint8_t is_addr_equal = 0;
	int res;
	size_t i;

	/* Set default return value */
	*retval = NULL;

	res = doca_devinfo_list_create(&dev_list, &nb_devs);
	if (res != DOCA_SUCCESS) {
		pr_err("Failed to load doca devices list. Doca_error value: %d\n", res);
		return res;
	}

	/* Search */
	for (i = 0; i < nb_devs; i++) {
		res = doca_devinfo_get_is_pci_addr_equal(dev_list[i], pci_addr, &is_addr_equal);
		if (res == DOCA_SUCCESS && is_addr_equal) {
			/* If any special capabilities are needed */
			if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS) {
				pr_warn("Function not support!\n");
				continue;
			}

			/* if device can be opened */
			res = doca_dev_open(dev_list[i], retval);
			if (res == DOCA_SUCCESS) {
				doca_devinfo_list_destroy(dev_list);
				return res;
			}
		}
	}

	pr_err("Matching device not found\n");
	res = DOCA_ERROR_NOT_FOUND;

	doca_devinfo_list_destroy(dev_list);
	return res;
}

doca_error_t init_buf(struct doca_dev * dev, struct doca_buf_inventory * buf_inv, struct buf_info * info, int buf_size) {
	doca_error_t result;

	info->data = (char *)calloc(buf_size, sizeof(char));
	info->size = buf_size;

	result = doca_mmap_create(NULL, &info->mmap);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		printf("Unable to create doca_mmap. Reason: %s\n", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		printf("Unable to create doca_mmap. Reason: %s\n", doca_error_get_descr(result));
#endif
		return 0;
	}

	result = doca_mmap_dev_add(info->mmap, dev);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_error_get_descr(result));
#endif
		return 0;
	}

	result = doca_mmap_set_memrange(info->mmap, info->data, info->size);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		printf("Unable to set memory range of source memory map: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		printf("Unable to set memory range of source memory map: %s", doca_error_get_descr(result));
#endif
		return result;
	}

	result = doca_mmap_start(info->mmap);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		printf("Unable to start source memory map: %s", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		printf("Unable to start source memory map: %s", doca_error_get_descr(result));
#endif
		return result;
	}

	if (doca_buf_inventory_buf_by_addr(buf_inv, info->mmap, info->data, info->size, &info->buf) != DOCA_SUCCESS) {
        printf("Failed to create inventory buf!\n");
        return 0;
    }

	return DOCA_SUCCESS;
}

/**
 * @brief Creates a DOCA flow port with the specified port ID.
 * 
 * This function initializes a DOCA flow port configuration structure, 
 * sets the port ID and its type, and starts the port. It constructs the 
 * device arguments from the port ID, ensuring it is correctly formatted.
 * 
 * @param port_id The ID of the port to be created. This should be a valid 
 *                port identifier as required by the DOCA framework.
 * 
 * @return A pointer to the created `doca_flow_port` structure if successful; 
 *         otherwise, NULL if the port could not be started.
 */
static struct doca_flow_port *_create_doca_flow_port(int port_id) {
	int max_port_str_len = 128;
	struct doca_flow_port_cfg port_cfg;
	char port_id_str[max_port_str_len];
	struct doca_flow_port *port;

	memset(&port_cfg, 0, sizeof(port_cfg));

	port_cfg.port_id = port_id;
	port_cfg.type = DOCA_FLOW_PORT_DPDK_BY_ID;
	snprintf(port_id_str, max_port_str_len, "%d", port_cfg.port_id);
	port_cfg.devargs = port_id_str;

	if (doca_flow_port_start(&port_cfg, &port) != DOCA_SUCCESS)
		return NULL;

	return port;
}

int init_doca_flow(int nb_queues, const char *mode, struct doca_flow_resources resource, uint32_t nr_shared_resources[]) {
	struct doca_flow_cfg flow_cfg;
	int shared_resource_idx;
	doca_error_t result;

	memset(&flow_cfg, 0, sizeof(flow_cfg));

	flow_cfg.queues = nb_queues;
	flow_cfg.mode_args = mode;
	flow_cfg.resource = resource;
	for (shared_resource_idx = 0; shared_resource_idx < DOCA_FLOW_SHARED_RESOURCE_MAX; shared_resource_idx++)
		flow_cfg.nr_shared_resources[shared_resource_idx] = nr_shared_resources[shared_resource_idx];
	result = doca_flow_init(&flow_cfg);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		printf("Failed to init DOCA Flow: %s\n", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		printf("Failed to init DOCA Flow: %s\n", doca_error_get_descr(result));
#endif
		return -1;
	}

	return 0;
}

int init_doca_flow_ports(int nb_ports, struct doca_flow_port *ports[], bool is_hairpin) {
	int portid;

	for (portid = 0; portid < nb_ports; portid++) {
		/* Create doca flow port */
		ports[portid] = _create_doca_flow_port(portid);
		if (ports[portid] == NULL) {
			return -1;
		}

		/* Pair ports should be done in the following order: port0 with port1, port2 with port3 etc */
		if (!is_hairpin || !portid || !(portid % 2))
			continue;

		/* pair odd port with previous port */
		if (doca_flow_port_pair(ports[portid], ports[portid ^ 1]) != DOCA_SUCCESS) {
			return -1;
		}
	}
	return 0;
}

int build_hairpin_pipe(uint16_t port_id) {
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg pipe_cfg;
	struct doca_flow_pipe_entry *entry;
	struct entries_status *status;
	int num_of_entries = 1;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));
	memset(&pipe_cfg, 0, sizeof(pipe_cfg));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	pipe_cfg.attr.name = "HAIRPIN_PIPE";
	pipe_cfg.match = &match;
	actions_arr[0] = &actions;
	pipe_cfg.actions = actions_arr;
	pipe_cfg.attr.is_root = false;
	pipe_cfg.attr.nb_actions = NB_ACTION_ARRAY;
	pipe_cfg.port = ports[port_id];

	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id ^ 1;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, NULL, &hairpin_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, hairpin_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, &entry);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS)
		return -1;

	return 0;
}

int build_rss_pipe(uint16_t port_id) {
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd, fwd_miss;
	struct doca_flow_pipe_cfg pipe_cfg;
	struct doca_flow_pipe_entry *entry;
	struct entries_status *status;
	uint16_t rss_queues[dpdk_config.port_config.nb_queues];
	int num_of_entries = 1;
	int i;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));
	memset(&fwd_miss, 0, sizeof(fwd_miss));
	memset(&pipe_cfg, 0, sizeof(pipe_cfg));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	pipe_cfg.attr.name = "RSS_PIPE";
	pipe_cfg.match = &match;
	actions_arr[0] = &actions;
	pipe_cfg.actions = actions_arr;
	pipe_cfg.attr.is_root = false;
	pipe_cfg.attr.nb_actions = NB_ACTION_ARRAY;
	pipe_cfg.port = ports[port_id];

	for (i = 0; i < dpdk_config.port_config.nb_queues; i++)
		rss_queues[i] = i;

	fwd.type = DOCA_FLOW_FWD_RSS;
	fwd.rss_outer_flags = DOCA_FLOW_RSS_IPV4 | DOCA_FLOW_RSS_TCP | DOCA_FLOW_RSS_UDP;
	fwd.num_of_queues = dpdk_config.port_config.nb_queues;
	fwd.rss_queues = rss_queues;

	fwd_miss.type = DOCA_FLOW_FWD_DROP;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, &fwd_miss, &rss_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, rss_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, &entry);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS)
		return -1;

	return 0;
}

int doca_init(void) {
	struct doca_flow_resources resource = {0};
	uint32_t nr_shared_resources[DOCA_FLOW_SHARED_RESOURCE_MAX] = {0};
	doca_error_t result;

    result = init_doca_flow(dpdk_config.port_config.nb_queues, "vnf,hws", resource, nr_shared_resources);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("Failed to init DOCA Flow: %s\n", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("Failed to init DOCA Flow: %s\n", doca_error_get_descr(result));
#endif
		return result;
	}

	pr_info("DOCA flow init!\n");

	result = init_doca_flow_ports(dpdk_config.port_config.nb_ports, ports, true);
	if (result != DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
		pr_err("Failed to init DOCA ports: %s\n", doca_get_error_string(result));
#else if CONFIG_BLUEFIELD3
		pr_err("Failed to init DOCA ports: %s\n", doca_error_get_descr(result));
#endif
		doca_flow_destroy();
		return result;
	}

	pr_info("DOCA flow ports init!\n");

	for (int port_id = 0; port_id < dpdk_config.port_config.nb_ports; port_id++) {
		printf(ESC GREEN "[INFO]" RESET " Build hairpin pipe on port %d => ", port_id);
		result = build_hairpin_pipe(port_id);
		if (result < 0) {
			printf("Failed building hairpin pipe\n");
			return -1;
		}
		printf(ESC GREEN "[DONE]" RESET "\n");

		printf(ESC GREEN "[INFO]" RESET " Build RSS pipe on port %d => ", port_id);
		result = build_rss_pipe(port_id);
		if (result < 0) {
			printf("Failed building RSS pipe\n");
			return -1;
		}
		printf(ESC GREEN "[DONE]" RESET "\n");
	}
    return 0;
}

int __init docadv_init(void) {
#ifdef CONFIG_DOCA_COMPRESS
	pr_info("Init: DOCA device...\n");
	docadv_compress_init();
#endif /* CONFIG_DOCA_COMPRESS */
    return 0;
}

#endif  /* CONFIG_DOCA */