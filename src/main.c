#include <stdlib.h>
#include <signal.h>
#include <Python.h>

#include "printk.h"
#include "syscall.h"
#include "lib.h"
#include "fs/fs.h"
#include "ipc/ipc.h"
#include "kernel/sched.h"
#include "net/dpdk_module.h"
#include "net/net.h"
#ifdef CONFIG_DOCA
#include <doca_flow.h>
#include "doca/context.h"
#endif  /* CONFIG_DOCA */

bool kernel_early_boot = true;
bool kernel_shutdown = false;

struct doca_flow_port *ports[1];

struct lcore_arg {
	enum Role role;
};

DEFINE_PER_CPU(enum Role, role);

void signal_handler(int sig) { 
    switch (sig) {
        case SIGINT:
            pr_info("Caught SIGINT! Terminating...\n");
            pr_info("Goodbye!\n");
            if (Py_IsInitialized()) {
                Py_Finalize();
            }
            exit(1);
            break;
        default:
            break;
    }
}

struct entries_status {
	bool failure;		/* will be set to true if some entry status will not be success */
	int nb_processed;	/* will hold the number of entries that was already processed */
};

static void
check_for_valid_entry(struct doca_flow_pipe_entry *entry, uint16_t pipe_queue,
		      enum doca_flow_entry_status status, enum doca_flow_entry_op op, void *user_ctx)
{
	(void)entry;
	(void)op;
	(void)pipe_queue;
	struct entries_status *entry_status = (struct entries_status *)user_ctx;

	if (entry_status == NULL)
		return;
	if (status != DOCA_FLOW_ENTRY_STATUS_SUCCESS)
		entry_status->failure = true; /* set failure to true if processing failed */
	entry_status->nb_processed++;
}

doca_error_t
init_doca_flow_cb(int nb_queues, const char *mode, struct doca_flow_resources resource, uint32_t nr_shared_resources[], doca_flow_entry_process_cb cb)
{
	struct doca_flow_cfg flow_cfg;
	int shared_resource_idx;

	memset(&flow_cfg, 0, sizeof(flow_cfg));

	flow_cfg.queues = nb_queues;
	flow_cfg.mode_args = mode;
	flow_cfg.resource = resource;
	flow_cfg.cb = cb;
	for (shared_resource_idx = 0; shared_resource_idx < DOCA_FLOW_SHARED_RESOURCE_MAX; shared_resource_idx++)
		flow_cfg.nr_shared_resources[shared_resource_idx] = nr_shared_resources[shared_resource_idx];
	return doca_flow_init(&flow_cfg);
}

doca_error_t
init_doca_flow(int nb_queues, const char *mode, struct doca_flow_resources resource, uint32_t nr_shared_resources[])
{
	return init_doca_flow_cb(nb_queues, mode, resource, nr_shared_resources, check_for_valid_entry);
}

static doca_error_t
create_doca_flow_port(int port_id, struct doca_flow_port **port)
{
	int max_port_str_len = 128;
	struct doca_flow_port_cfg port_cfg;
	char port_id_str[max_port_str_len];

	memset(&port_cfg, 0, sizeof(port_cfg));

	port_cfg.port_id = port_id;
	port_cfg.type = DOCA_FLOW_PORT_DPDK_BY_ID;
	snprintf(port_id_str, max_port_str_len, "%d", port_cfg.port_id);
	port_cfg.devargs = port_id_str;
	return doca_flow_port_start(&port_cfg, port);
}

void
stop_doca_flow_ports(int nb_ports, struct doca_flow_port *ports[])
{
	int portid;

	for (portid = 0; portid < nb_ports; portid++) {
		if (ports[portid] != NULL)
			doca_flow_port_stop(ports[portid]);
	}
}

doca_error_t
init_doca_flow_ports(int nb_ports, struct doca_flow_port *ports[], bool is_hairpin)
{
	int portid;
	doca_error_t result;

	for (portid = 0; portid < nb_ports; portid++) {
		/* Create doca flow port */
		printf("Starting port %d...\n", portid);
		result = create_doca_flow_port(portid, &ports[portid]);
		if (result != DOCA_SUCCESS) {
			printf("Failed to start port: %s\n", doca_get_error_string(result));
			stop_doca_flow_ports(portid + 1, ports);
			return result;
		}
		/* Pair ports should be done in the following order: port0 with port1, port2 with port3 etc */
		if (!is_hairpin || !portid || !(portid % 2))
			continue;
		/* pair odd port with previous port */
		result = doca_flow_port_pair(ports[portid], ports[portid ^ 1]);
		if (result != DOCA_SUCCESS) {
			printf("Failed to pair ports %u - %u\n", portid, portid ^ 1);
			stop_doca_flow_ports(portid + 1, ports);
			return result;
		}
	}
	return DOCA_SUCCESS;
}

#define NB_ACTION_ARRAY	(1)

doca_error_t create_drop_pipe(struct doca_flow_port *port, struct doca_flow_pipe **pipe) {
	struct doca_flow_match match;
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg pipe_cfg;
	struct entries_status status;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&fwd, 0, sizeof(fwd));
	memset(&status, 0, sizeof(status));
	memset(&pipe_cfg, 0, sizeof(pipe_cfg));

	pipe_cfg.attr.name = "DROP_PIPE";
	pipe_cfg.attr.type = DOCA_FLOW_PIPE_BASIC;
	pipe_cfg.match = &match;
	pipe_cfg.port = port;
	pipe_cfg.attr.is_root = false;

    fwd.type = DOCA_FLOW_FWD_DROP;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, NULL, pipe);
	if (result != DOCA_SUCCESS) {
		printf("Failed to create DROP pipe: %s\n", doca_get_error_string(result));
		return result;
	} else {
		printf("Create DROP pipe!\n");
	}

	return DOCA_SUCCESS;
}

doca_error_t create_rss_pipe(struct doca_flow_port *port, struct doca_flow_pipe **pipe) {
	struct doca_flow_match match;
    struct doca_flow_actions actions;
	struct doca_flow_actions *actions_arr[NB_ACTION_ARRAY];
	struct doca_flow_fwd fwd;
	struct doca_flow_pipe_cfg pipe_cfg;
	struct entries_status status;
    int nb_queues = 1;
	uint16_t rss_queues[nb_queues];
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));
	memset(&status, 0, sizeof(status));
	memset(&pipe_cfg, 0, sizeof(pipe_cfg));

	pipe_cfg.attr.name = "RSS_PIPE";
	pipe_cfg.attr.type = DOCA_FLOW_PIPE_BASIC;
	pipe_cfg.match = &match;
    actions_arr[0] = &actions;
	pipe_cfg.actions = actions_arr;
	pipe_cfg.attr.nb_actions = NB_ACTION_ARRAY;
	pipe_cfg.port = port;
	pipe_cfg.attr.is_root = false;

    for (int i = 0; i < nb_queues; i++)
        rss_queues[i] = i;

    fwd.type = DOCA_FLOW_FWD_RSS;
    fwd.rss_outer_flags = DOCA_FLOW_RSS_IPV4;
    fwd.num_of_queues = nb_queues;
    fwd.rss_queues = rss_queues;

	result = doca_flow_pipe_create(&pipe_cfg, &fwd, NULL, pipe);
	if (result != DOCA_SUCCESS) {
		printf("Failed to create RSS pipe: %s\n", doca_get_error_string(result));
		return result;
	} else {
		printf("Create RSS pipe!\n");
	}

	return DOCA_SUCCESS;
}

int lcore_main(void * arg) {
    struct lcore_arg * args = (struct lcore_arg *)arg;
    int lid = rte_lcore_id();
    bool is_main = false;
    pistachio_lcore_init(lid);

    role = args[lid].role;

#if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 0, 0)
    if (rte_lcore_id() == rte_get_main_lcore()) is_main = true;
#else
    if (rte_lcore_id() == rte_get_master_lcore()) is_main = true;
#endif

    pr_info("Start %s >>>\n", (role == WORKER)? "WORKER" : (role == RXTX)? "NETWORK" : "IDLE");

    flax_worker_ctx_fetch(lid);

    /* Init network info */
    memset(&net_info, 0, sizeof(struct network_info));
    clock_gettime(CLOCK_REALTIME, &net_info.last_log);

	struct doca_flow_pipe *rss_pipe, *drop_pipe;
	create_rss_pipe(ports[0], &rss_pipe);
	create_drop_pipe(ports[0], &drop_pipe);

    while (1) {
        if (is_main) {
            ipc_poll();
        }
        if (role == RXTX) {
            network_main();
        } else {
            worker_main();
        }
    }

    return 0;
}

#define MAX_FILE_SIZE   1024

/* DOCA core objects used by the samples / applications */
struct program_core_objects {
	struct doca_dev *dev;			/* doca device */
	struct doca_mmap *src_mmap;		/* doca mmap for source buffer */
	struct doca_mmap *dst_mmap;		/* doca mmap for destination buffer */
	struct doca_buf_inventory *buf_inv;	/* doca buffer inventory */
	struct doca_ctx *ctx;			/* doca context */
	struct doca_workq *workq;		/* doca work queue */
};

doca_error_t
create_core_objects(struct program_core_objects *state, uint32_t max_bufs)
{
	doca_error_t res;

	res = doca_mmap_create(NULL, &state->src_mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create source mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(state->src_mmap, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to source mmap: %s", doca_get_error_string(res));
		doca_mmap_destroy(state->src_mmap);
		state->src_mmap = NULL;
		return res;
	}

	res = doca_mmap_create(NULL, &state->dst_mmap);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create destination mmap: %s", doca_get_error_string(res));
		return res;
	}
	res = doca_mmap_dev_add(state->dst_mmap, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to add device to destination mmap: %s", doca_get_error_string(res));
		doca_mmap_destroy(state->dst_mmap);
		state->dst_mmap = NULL;
		return res;
	}

	res = doca_buf_inventory_create(NULL, max_bufs, DOCA_BUF_EXTENSION_NONE, &state->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_buf_inventory_start(state->buf_inv);
	if (res != DOCA_SUCCESS) {
		printf("Unable to start buffer inventory: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_ctx_dev_add(state->ctx, state->dev);
	if (res != DOCA_SUCCESS) {
		printf("Unable to register device with lib context: %s", doca_get_error_string(res));
		state->ctx = NULL;
		return res;
	}

	return res;
}

doca_error_t
start_context(struct program_core_objects *state, uint32_t workq_depth)
{
	doca_error_t res;
	struct doca_workq *workq;

	res = doca_ctx_start(state->ctx);
	if (res != DOCA_SUCCESS) {
		printf("Unable to start lib context: %s", doca_get_error_string(res));
		doca_ctx_dev_rm(state->ctx, state->dev);
		state->ctx = NULL;
		return res;
	}

	res = doca_workq_create(workq_depth, &workq);
	if (res != DOCA_SUCCESS) {
		printf("Unable to create work queue: %s", doca_get_error_string(res));
		return res;
	}

	res = doca_ctx_workq_add(state->ctx, workq);
	if (res != DOCA_SUCCESS) {
		printf("Unable to register work queue with context: %s", doca_get_error_string(res));
		state->workq = NULL;
	} else
		state->workq = workq;

	return res;
}

doca_error_t
init_core_objects(struct program_core_objects *state, uint32_t workq_depth, uint32_t max_bufs)
{
	doca_error_t res;

	res = create_core_objects(state, max_bufs);
	if (res != DOCA_SUCCESS)
		return res;
	res = start_context(state, workq_depth);
	return res;
}

static doca_error_t
compress_jobs_decompress_is_supported(struct doca_devinfo *devinfo)
{
	return doca_compress_job_get_supported(devinfo, DOCA_COMPRESS_DEFLATE_JOB);
}

doca_error_t compress_deflate(const char *pci_addr, char *file_data, size_t file_size, enum doca_compress_job_types job_type) {
	struct program_core_objects state = {0};
	struct doca_event event = {0};
	struct doca_compress *compress;
	struct doca_buf *src_doca_buf;
	struct doca_buf *dst_doca_buf;
	uint32_t workq_depth = 1;		/* The sample will run 1 compress job */
	uint32_t max_bufs = 2;			/* The sample will use 2 doca buffers */
	char *dst_buffer;
	doca_error_t result;

	result = doca_compress_create(&compress);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create compress engine: %s", doca_get_error_string(result));
		return result;
	}

	state.ctx = doca_compress_as_ctx(compress);

    result = open_doca_device_with_pci(pci_addr, compress_jobs_decompress_is_supported, &state.dev);

	if (result != DOCA_SUCCESS) {
		doca_compress_destroy(compress);
		return result;
	}

	result = init_core_objects(&state, workq_depth, max_bufs);
	if (result != DOCA_SUCCESS) {
		return result;
	}

	dst_buffer = calloc(1, MAX_FILE_SIZE);
	if (dst_buffer == NULL) {
		printf("Failed to allocate memory");
		return -1;
	}

	result = doca_mmap_set_memrange(state.dst_mmap, dst_buffer, MAX_FILE_SIZE);
	if (result != DOCA_SUCCESS) {
		return result;
	}
	result = doca_mmap_start(state.dst_mmap);
	if (result != DOCA_SUCCESS) {
		free(dst_buffer);
		return result;
	}

	result = doca_mmap_set_memrange(state.src_mmap, file_data, file_size);
	if (result != DOCA_SUCCESS) {
		return result;
	}
	result = doca_mmap_start(state.src_mmap);
	if (result != DOCA_SUCCESS) {
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, state.src_mmap, file_data, file_size, &src_doca_buf);
	if (result != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing source buffer: %s", doca_get_error_string(result));
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, state.dst_mmap, dst_buffer, MAX_FILE_SIZE, &dst_doca_buf);
	if (result != DOCA_SUCCESS) {
		printf("Unable to acquire DOCA buffer representing destination buffer: %s", doca_get_error_string(result));
		return result;
	}

	/* setting data length in doca buffer */
	result = doca_buf_set_data(src_doca_buf, file_data, file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to set DOCA buffer data: %s\n", doca_get_error_string(result));
		doca_buf_refcount_rm(src_doca_buf, NULL);
		doca_buf_refcount_rm(dst_doca_buf, NULL);
		return result;
	}

	/* Construct compress job */
	const struct doca_compress_deflate_job compress_job = {
		.base = (struct doca_job) {
			.type = job_type,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = state.ctx,
			.user_data.u64 = job_type,
			},
		.dst_buff = dst_doca_buf,
		.src_buff = src_doca_buf,
	};

	/* Enqueue compress job */
	result = doca_workq_submit(state.workq, &compress_job.base);
	if (result != DOCA_SUCCESS) {
		printf("Failed to submit compress job: %s\n", doca_get_error_string(result));
		doca_buf_refcount_rm(dst_doca_buf, NULL);
		doca_buf_refcount_rm(src_doca_buf, NULL);
		return result;
	}

	/* Wait for job completion */
	while ((result = doca_workq_progress_retrieve(state.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) == DOCA_ERROR_AGAIN);

	if (result != DOCA_SUCCESS)
		printf("Failed to retrieve compress job: %s\n", doca_get_error_string(result));

	else if (event.result.u64 != DOCA_SUCCESS)
		printf("Compress job finished unsuccessfully\n");

	else if (((int)(event.type) != (int)job_type) || (event.user_data.u64 != job_type))
		printf("Received wrong event\n");

	else {
		printf("Compress done!\n");
	}

	if (doca_buf_refcount_rm(src_doca_buf, NULL) != DOCA_SUCCESS ||
	    doca_buf_refcount_rm(dst_doca_buf, NULL) != DOCA_SUCCESS)
		printf("Failed to decrease DOCA buffer reference count\n");

	return result;
}

int main(int argc, char ** argv) {
	struct lcore_arg * args;
	int n, i = 0, lcore_id = 0;

    pr_info("init: starting DPDK...\n");
    dpdk_init(argc, argv);

#ifdef CONFIG_DOCA
    docadv_init();
#endif  /* CONFIG_DOCA */

    kernel_early_boot = false;

    pr_info("init: initializing fs...\n");
	fs_init();

    /* Reserve 0, 1, and 2 for stdin, stdout, and stderr */
    set_open_fd(0);
    set_open_fd(1);
    set_open_fd(2);

	pr_info("init: initializing worker threads...\n");
    worker_init();

    pr_info("init: initializing ipc...\n");
	ipc_init();

    /* Init scheduler */
    pr_info("Init: Flax module...\n");
    flax_init();

    /* Init network IO */
    pr_info("Init: pistachIO module...\n");
    pistachio_init();

    /* Init runtime */
    pr_info("Init: Cacao module...\n");
    cacao_init();

    pr_info("init: initializing SYSCALL...\n");
    syscall_hook_init();

    pr_info("init: initializing zlib...\n");
    zlib_hook_init();

    /* Register termination handling callback */
    signal(SIGINT, signal_handler); 

    n = rte_lcore_count();

    args = calloc(n, sizeof(struct lcore_arg));
    RTE_LCORE_FOREACH(lcore_id) {
        struct worker_context * ctx = &contexts[lcore_id];
#ifdef CONFIG_DOCA
        docadv_worker_ctx_init(&ctx->docadv_ctx);
#endif  /* CONFIG_DOCA */
        if (i == 0) {
    		args[i].role = RXTX;
        } else {
    		args[i].role = WORKER;
        }
		i++;
	}

    /* Launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(lcore_main, args, CALL_MAIN);
	rte_eal_mp_wait_lcore();

	/* clean up the EAL */
	rte_eal_cleanup();

    return 0;
}