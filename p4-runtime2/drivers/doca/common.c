#include <stdlib.h>

#include "net/dpdk.h"

#ifdef CONFIG_DOCA
#include "utils/printk.h"
#include "backend/flowPipeInternal.h"
#include "drivers/doca/common.h"
#include "drivers/doca/compress.h"

#include <doca_flow.h>
#include <doca_mmap.h>
#include <doca_buf.h>
#include <doca_buf_inventory.h>

/* Convert IPv4 address to big endian */
#define BE_IPV4_ADDR(a, b, c, d) \
	(RTE_BE32(((uint32_t)a<<24) + (b<<16) + (c<<8) + d))

int nb_ports = 2;
struct doca_flow_port *ports[MAX_PORTS];
struct doca_flow_pipe *rss_pipe[2], *hairpin_pipe[2], *port_pipe[2];

struct entries_status {
	bool failure;		/* will be set to true if some entry status will not be success */
	int nb_processed;	/* will hold the number of entries that was already processed */
	void *ft_entry;		/* pointer to struct vxlan_fwd_ft_entry */
};

struct flow_resources {
	uint32_t nr_counters; /* number of counters to configure */
	uint32_t nr_meters;   /* number of traffic meters to configure */
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

#if CONFIG_BLUEFIELD2
	res = doca_devinfo_list_create(&dev_list, &nb_devs);
#elif CONFIG_BLUEFIELD3
	res = doca_devinfo_create_list(&dev_list, &nb_devs);
#endif
	if (res != DOCA_SUCCESS) {
		pr_err("Failed to load doca devices list. Doca_error value: %d\n", res);
		return res;
	}

	/* Search */
	for (i = 0; i < nb_devs; i++) {
#if CONFIG_BLUEFIELD2
		res = doca_devinfo_get_is_pci_addr_equal(dev_list[i], pci_addr, &is_addr_equal);
#elif CONFIG_BLUEFIELD3
		res = doca_devinfo_is_equal_pci_addr(dev_list[i], pci_addr, &is_addr_equal);
#endif
		if (res == DOCA_SUCCESS && is_addr_equal) {
			/* If any special capabilities are needed */
			if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS) {
				pr_warn("Function not support!\n");
				continue;
			}

			/* if device can be opened */
			res = doca_dev_open(dev_list[i], retval);
			if (res == DOCA_SUCCESS) {
#if CONFIG_BLUEFIELD2
				doca_devinfo_list_destroy(dev_list);
#elif CONFIG_BLUEFIELD3
				doca_devinfo_destroy_list(dev_list);
#endif
				return res;
			}
		}
	}

	pr_err("Matching device not found\n");
	res = DOCA_ERROR_NOT_FOUND;

#if CONFIG_BLUEFIELD2
	doca_devinfo_list_destroy(dev_list);
#elif CONFIG_BLUEFIELD3
	doca_devinfo_destroy_list(dev_list);
#endif

	return res;
}

doca_error_t init_buf(struct doca_dev * dev, struct doca_buf_inventory * buf_inv, struct buf_info * info, int buf_size) {
	doca_error_t result;

	info->data = (char *)calloc(buf_size, sizeof(char));
	info->size = buf_size;

#ifdef CONFIG_BLUEFIELD2
	result = doca_mmap_create(NULL, &info->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_dev_add(info->mmap, dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to add device to doca_mmap. (dev: %p, mmap: %p) Reason: %s\n", 
				dev, info->mmap, doca_get_error_string(result));
		return 0;
	}
#elif CONFIG_BLUEFIELD3
	result = doca_mmap_create(&info->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_mmap. Reason: %s\n", doca_error_get_descr(result));
		return 0;
	}

	result = doca_mmap_add_dev(info->mmap, dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_error_get_descr(result));
		return 0;
	}
#endif

	result = doca_mmap_set_memrange(info->mmap, info->data, info->size);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		printf("Unable to set memory range of source memory map: %s", doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf("Unable to set memory range of source memory map: %s", doca_error_get_descr(result));
#endif
		return result;
	}

	result = doca_mmap_start(info->mmap);
	if (result != DOCA_SUCCESS) {
#ifdef CONFIG_BLUEFIELD2
		printf("Unable to start source memory map: %s", doca_get_error_string(result));
#elif CONFIG_BLUEFIELD3
		printf("Unable to start source memory map: %s", doca_error_get_descr(result));
#endif
		return result;
	}

#ifdef CONFIG_BLUEFIELD2
	if (doca_buf_inventory_buf_by_addr(buf_inv, info->mmap, info->data, info->size, &info->buf) != DOCA_SUCCESS) {
#elif CONFIG_BLUEFIELD3
	if (doca_buf_inventory_buf_get_by_addr(buf_inv, info->mmap, info->data, info->size, &info->buf) != DOCA_SUCCESS) {
#endif
        printf("Failed to create inventory buf!\n");
        return 0;
    }

	return DOCA_SUCCESS;
}

#ifdef CONFIG_BLUEFIELD2
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
#elif CONFIG_BLUEFIELD3
static doca_error_t create_doca_flow_port(int port_id,
					  struct doca_dev *dev,
					  enum doca_flow_port_operation_state state,
					  struct doca_flow_port **port)
{
	int max_port_str_len = 128;
	struct doca_flow_port_cfg *port_cfg;
	char port_id_str[max_port_str_len];
	doca_error_t result, tmp_result;

	result = doca_flow_port_cfg_create(&port_cfg);
	if (result != DOCA_SUCCESS) {
		printf("Failed to create doca_flow_port_cfg: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_port_cfg_set_dev(port_cfg, dev);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_port_cfg dev: %s\n", doca_error_get_descr(result));
		goto destroy_port_cfg;
	}

	snprintf(port_id_str, max_port_str_len, "%d", port_id);
	result = doca_flow_port_cfg_set_devargs(port_cfg, port_id_str);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_port_cfg devargs: %s\n", doca_error_get_descr(result));
		goto destroy_port_cfg;
	}

	result = doca_flow_port_cfg_set_operation_state(port_cfg, state);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_port_cfg operation state: %s\n", doca_error_get_descr(result));
		goto destroy_port_cfg;
	}

	result = doca_flow_port_start(port_cfg, port);
	if (result != DOCA_SUCCESS) {
		printf("Failed to start doca_flow port: %s\n", doca_error_get_descr(result));
		goto destroy_port_cfg;
	}

destroy_port_cfg:
	tmp_result = doca_flow_port_cfg_destroy(port_cfg);
	if (tmp_result != DOCA_SUCCESS) {
		printf("Failed to destroy doca_flow port: %s\n", doca_error_get_descr(tmp_result));
	}

	return result;
}
#endif

#ifdef CONFIG_BLUEFIELD2
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
		printf("Failed to init DOCA Flow: %s\n", doca_get_error_string(result));
		return -1;
	}

	return 0;
}
#elif CONFIG_BLUEFIELD3
#define SHARED_RESOURCE_NUM_VALUES (8) /* Number of doca_flow_shared_resource_type values */
int init_doca_flow(int nb_queues, const char *mode, struct flow_resources *resource, uint32_t nr_shared_resources[])
{
	struct doca_flow_cfg *flow_cfg;
	doca_error_t result;
	uint16_t qidx, rss_queues[nb_queues];
	struct doca_flow_resource_rss_cfg rss = {0};

	result = doca_flow_cfg_create(&flow_cfg);
	if (result != DOCA_SUCCESS) {
		printf("Failed to create doca_flow_cfg: %s\n", doca_error_get_descr(result));
		return result;
	}

	rss.nr_queues = nb_queues;
	for (qidx = 0; qidx < nb_queues; qidx++)
		rss_queues[qidx] = qidx;
	rss.queues_array = rss_queues;
	result = doca_flow_cfg_set_default_rss(flow_cfg, &rss);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_cfg rss: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_cfg_set_pipe_queues(flow_cfg, nb_queues);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_cfg pipe_queues: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_cfg_set_mode_args(flow_cfg, mode);
	if (result != DOCA_SUCCESS) {
		printf("Failed to set doca_flow_cfg mode_args: %s\n", doca_error_get_descr(result));
		return -1;
	}

	for (int i = 0; i < SHARED_RESOURCE_NUM_VALUES; i++) {
		result = doca_flow_cfg_set_nr_shared_resource(flow_cfg, nr_shared_resources[i], i);
		if (result != DOCA_SUCCESS) {
			printf("Failed to set doca_flow_cfg nr_shared_resources: %s\n", doca_error_get_descr(result));
			return -1;
		}
	}

	result = doca_flow_init(flow_cfg);
	if (result != DOCA_SUCCESS) {
		printf("Failed to initialize DOCA Flow: %s\n", doca_error_get_descr(result));
		return -1;
	}

	return 0;
}
#endif

#ifdef CONFIG_BLUEFIELD2
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
#elif CONFIG_BLUEFIELD3
int init_doca_flow_ports(int nb_ports, struct doca_flow_port *ports[], bool is_hairpin, struct doca_dev *dev_arr[]) {
	int portid;
	doca_error_t result;
	enum doca_flow_port_operation_state state;

	for (portid = 0; portid < nb_ports; portid++) {
		state = DOCA_FLOW_PORT_OPERATION_STATE_ACTIVE;
		/* Create doca flow port */
		result = create_doca_flow_port(portid, dev_arr[portid], state, &ports[portid]);
		if (result != DOCA_SUCCESS) {
			printf("Failed to start port: %s\n", doca_error_get_descr(result));
			return result;
		}
		/* Pair ports should be done in the following order: port0 with port1, port2 with port3 etc */
		if (!is_hairpin || !portid || !(portid % 2))
			continue;
		/* pair odd port with previous port */
		result = doca_flow_port_pair(ports[portid], ports[portid ^ 1]);
		if (result != DOCA_SUCCESS) {
			printf("Failed to pair ports %u - %u\n", portid, portid ^ 1);
			return -1;
		}
	}
	return 0;
}
#endif

int build_port_pipe(uint16_t port_id) {
#ifdef CONFIG_BLUEFIELD2
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg pipe_cfg;
	struct entries_status *status;
	int num_of_entries = 1;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));
	memset(&pipe_cfg, 0, sizeof(pipe_cfg));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	pipe_cfg.attr.name = "PORT_PIPE";
	pipe_cfg.match = &match;
	actions_arr[0] = &actions;
	pipe_cfg.actions = actions_arr;
	pipe_cfg.attr.is_root = false;
	pipe_cfg.attr.nb_actions = NB_ACTION_ARRAY;
	pipe_cfg.port = ports[port_id];

	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, NULL, &port_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, port_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, NULL);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
		return -1;
	}
#elif CONFIG_BLUEFIELD3
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct entries_status *status;
	int num_of_entries = 1;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	actions_arr[0] = &actions;

	result = doca_flow_pipe_cfg_create(&pipe_cfg,  ports[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to create doca_flow_pipe_cfg: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_name(pipe_cfg, "PORT_PIPE");
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg name: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_BASIC);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg type: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_is_root(pipe_cfg, false);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg is_root: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_domain(pipe_cfg, DOCA_FLOW_PIPE_DOMAIN_EGRESS);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg to EGRESS: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_match(pipe_cfg, &match, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg match: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_actions(pipe_cfg, actions_arr, NULL, NULL, NB_ACTION_ARRAY);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg actions: %s\n", doca_error_get_descr(result));
		return result;
	}

	/* forwarding traffic to other port */
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, &port_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set create hairpin pipe: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, port_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to add entry to hairpin pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to process entry in hairpin pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}
#endif
	return 0;
}

int build_hairpin_pipe(uint16_t port_id) {
#ifdef CONFIG_BLUEFIELD2
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg pipe_cfg;
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

	result = doca_flow_pipe_add_entry(0, hairpin_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, NULL);
	if (result != DOCA_SUCCESS) {
		free(status);
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
		return -1;
	}
#elif CONFIG_BLUEFIELD3
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct entries_status *status;
	int num_of_entries = 1;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	actions_arr[0] = &actions;

	result = doca_flow_pipe_cfg_create(&pipe_cfg,  ports[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to create doca_flow_pipe_cfg: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_name(pipe_cfg, "HAIRPIN_PIPE");
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg name: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_BASIC);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg type: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_is_root(pipe_cfg, false);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg is_root: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_match(pipe_cfg, &match, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg match: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_actions(pipe_cfg, actions_arr, NULL, NULL, NB_ACTION_ARRAY);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg actions: %s\n", doca_error_get_descr(result));
		return result;
	}

	/* forwarding traffic to other port */
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id ^ 1;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, &hairpin_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set create hairpin pipe: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, hairpin_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to add entry to hairpin pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to process entry in hairpin pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}
#endif
	return 0;
}

int build_rss_pipe(uint16_t port_id) {
#ifdef CONFIG_BLUEFIELD2
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
#elif CONFIG_BLUEFIELD3
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg *pipe_cfg;
	struct entries_status *status;
		uint16_t rss_queues[1];
	int num_of_entries = 1;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));

	status = (struct entries_status *)calloc(1, sizeof(struct entries_status));

	actions_arr[0] = &actions;

	result = doca_flow_pipe_cfg_create(&pipe_cfg, ports[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to create doca_flow_pipe_cfg: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_name(pipe_cfg, "RSS_PIPE");
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg name: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_type(pipe_cfg, DOCA_FLOW_PIPE_BASIC);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg type: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_is_root(pipe_cfg, false);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg is_root: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_match(pipe_cfg, &match, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg match: %s\n", doca_error_get_descr(result));
		return result;
	}

	result = doca_flow_pipe_cfg_set_actions(pipe_cfg, actions_arr, NULL, NULL, NB_ACTION_ARRAY);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set doca_flow_pipe_cfg actions: %s\n", doca_error_get_descr(result));
		return result;
	}

	/* RSS queue - send matched traffic to queue 0  */
	rss_queues[0] = 0;
	fwd.type = DOCA_FLOW_FWD_RSS;
	fwd.rss_queues = rss_queues;
	fwd.rss_outer_flags = DOCA_FLOW_RSS_IPV4 | DOCA_FLOW_RSS_UDP;
	fwd.num_of_queues = 1;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, &rss_pipe[port_id]);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to set create hairpin pipe: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_pipe_add_entry(0, rss_pipe[port_id], &match, &actions, NULL, &fwd, 0, status, NULL);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to add entry to rss pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}

	result = doca_flow_entries_process(ports[port_id], 0, PULL_TIME_OUT, num_of_entries);
	if (result != DOCA_SUCCESS) {
		printf(ESC LIGHT_RED "[ERR]" RESET " Failed to process entry in rss pipes: %s\n", doca_error_get_descr(result));
		return -1;
	}
#endif
	return 0;
}

int doca_init(void) {
#ifdef CONFIG_BLUEFIELD2
	struct doca_flow_resources resource = {0};
	uint32_t nr_shared_resources[DOCA_FLOW_SHARED_RESOURCE_MAX] = {0};
	doca_error_t result;

    result = init_doca_flow(dpdk_config.port_config.nb_queues, "vnf,hws", resource, nr_shared_resources);
	if (result != DOCA_SUCCESS) {
		pr_err("Failed to init DOCA Flow: %s\n", doca_get_error_string(result));
		return result;
	}

	pr_info("DOCA flow init!\n");

	result = init_doca_flow_ports(dpdk_config.port_config.nb_ports, ports, true);
	if (result != DOCA_SUCCESS) {
		pr_err("Failed to init DOCA ports: %s\n", doca_get_error_string(result));
		doca_flow_destroy();
		return result;
	}

	pr_info("DOCA flow ports init!\n");
#elif CONFIG_BLUEFIELD3
	int nb_ports = 2;
	struct flow_resources resource = {0};
	uint32_t nr_shared_resources[SHARED_RESOURCE_NUM_VALUES] = {0};
	struct doca_dev *dev_arr[nb_ports];
	doca_error_t result;

	result = init_doca_flow(dpdk_config.port_config.nb_queues, "vnf,hws", &resource, nr_shared_resources);
	if (result != DOCA_SUCCESS) {
		printf("Failed to init DOCA Flow: %s\n", doca_error_get_descr(result));
		return result;
	}

	memset(dev_arr, 0, sizeof(struct doca_dev *) * nb_ports);
	result = init_doca_flow_ports(nb_ports, ports, true, dev_arr);
	if (result != DOCA_SUCCESS) {
		printf("Failed to init DOCA ports: %s\n", doca_error_get_descr(result));
		doca_flow_destroy();
		return result;
	}
#endif

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

		printf(ESC GREEN "[INFO]" RESET " Build PORT pipe on port %d => ", port_id);
		result = build_port_pipe(port_id);
		if (result < 0) {
			printf("Failed building PORT pipe\n");
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