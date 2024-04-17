#include <time.h>
#include "test_doca_regex.h"

typedef doca_error_t (*jobs_check)(struct doca_devinfo *);

doca_error_t
open_doca_device_with_pci(const char *pci_addr, jobs_check func, struct doca_dev **retval)
{
	struct doca_devinfo **dev_list;
	uint32_t nb_devs;
	uint8_t is_addr_equal = 0;
	int res;
	size_t i;

	/* Set default return value */
	*retval = NULL;

	res = doca_devinfo_list_create(&dev_list, &nb_devs);
	if (res != DOCA_SUCCESS) {
		printf("Failed to load doca devices list. Doca_error value: %d", res);
		return res;
	}

	/* Search */
	for (i = 0; i < nb_devs; i++) {
		res = doca_devinfo_get_is_pci_addr_equal(dev_list[i], pci_addr, &is_addr_equal);
		if (res == DOCA_SUCCESS && is_addr_equal) {
			/* If any special capabilities are needed */
			if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS)
				continue;

			/* if device can be opened */
			res = doca_dev_open(dev_list[i], retval);
			if (res == DOCA_SUCCESS) {
				doca_devinfo_list_destroy(dev_list);
				return res;
			}
		}
	}

	printf("Matching device not found");
	res = DOCA_ERROR_NOT_FOUND;

	doca_devinfo_list_destroy(dev_list);
	return res;
}

doca_error_t
read_file(char const *path, char **out_bytes, size_t *out_bytes_len)
{
	FILE *file;
	char *bytes;

	file = fopen(path, "rb");
	if (file == NULL)
		return DOCA_ERROR_NOT_FOUND;

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return DOCA_ERROR_IO_FAILED;
	}

	long const nb_file_bytes = ftell(file);

	if (nb_file_bytes == -1) {
		fclose(file);
		return DOCA_ERROR_IO_FAILED;
	}

	if (nb_file_bytes == 0) {
		fclose(file);
		return DOCA_ERROR_INVALID_VALUE;
	}

	bytes = malloc(nb_file_bytes);
	if (bytes == NULL) {
		fclose(file);
		return DOCA_ERROR_NO_MEMORY;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		free(bytes);
		fclose(file);
		return DOCA_ERROR_IO_FAILED;
	}

	size_t const read_byte_count = fread(bytes, 1, nb_file_bytes, file);

	fclose(file);

	if (read_byte_count != (size_t)nb_file_bytes) {
		free(bytes);
		return DOCA_ERROR_IO_FAILED;
	}

	*out_bytes = bytes;
	*out_bytes_len = read_byte_count;

	return DOCA_SUCCESS;
}

doca_error_t
test_dns_init(struct test_dns_config *app_cfg)
{
	doca_error_t result;

	char *rules_file_data;
	size_t rules_file_size;

    app_cfg->data_buffer = (char *)calloc(1024, sizeof(char));
    app_cfg->data_buffer_len = 1024;

	/* find doca_dev */
	result = open_doca_device_with_pci(app_cfg->pci_address, NULL, &app_cfg->dev);
	if (result != DOCA_SUCCESS) {
		printf("No device matching PCI address found. Reason: %s\n", doca_get_error_string(result));
		return result;
	}

	/* Create a DOCA RegEx instance */
	result = doca_regex_create(&(app_cfg->doca_regex));
	if (result != DOCA_SUCCESS) {
		printf("Unable to create RegEx device. Reason: %s\n", doca_get_error_string(result));
		doca_dev_close(app_cfg->dev);
		return DOCA_ERROR_NO_MEMORY;
	}

	/* Set the RegEx device as the main HW accelerator */
	result = doca_ctx_dev_add(doca_regex_as_ctx(app_cfg->doca_regex), app_cfg->dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to set RegEx device. Reason: %s\n", doca_get_error_string(result));
		result = DOCA_ERROR_INVALID_VALUE;
		goto regex_destroy;
	}

	/* Set work queue memory pool size */
	result = doca_regex_set_workq_matches_memory_pool_size(app_cfg->doca_regex, 0);
	if (result != DOCA_SUCCESS) {
		printf("Unable to set matches mempool size. Reason: %s\n", doca_get_error_string(result));
		goto regex_destroy;
	}

	/* Attach rules file to DOCA RegEx */
	result = read_file(app_cfg->rules_file_path, &rules_file_data, &rules_file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to load rules file content. Reason: %s", doca_get_error_string(result));
		goto regex_cleanup;
	}

	result = doca_regex_set_hardware_compiled_rules(app_cfg->doca_regex, rules_file_data, rules_file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to program rules. Reason: %s", doca_get_error_string(result));
		free(rules_file_data);
		goto regex_cleanup;
	}
	free(rules_file_data);

	/* Start doca RegEx */
	result = doca_ctx_start(doca_regex_as_ctx(app_cfg->doca_regex));
	if (result != DOCA_SUCCESS) {
		printf("Unable to start DOCA RegEx. Reason: %s\n", doca_get_error_string(result));
		result = DOCA_ERROR_INITIALIZATION;
		goto regex_destroy;
	}

	result = doca_buf_inventory_create(NULL, BUF_INVENTORY_POOL_SIZE, DOCA_BUF_EXTENSION_NONE, &app_cfg->buf_inventory);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_buf_inventory. Reason: %s\n", doca_get_error_string(result));
		goto regex_cleanup;
	}

	result = doca_buf_inventory_start(app_cfg->buf_inventory);
	if (result != DOCA_SUCCESS) {
		printf("Unable to start doca_buf_inventory. Reason: %s\n", doca_get_error_string(result));
		goto buf_inventory_cleanup;
	}

	result = doca_mmap_create(NULL, &app_cfg->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_mmap. Reason: %s\n", doca_get_error_string(result));
		goto buf_inventory_cleanup;
	}

	result = doca_mmap_dev_add(app_cfg->mmap, app_cfg->dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_get_error_string(result));
		goto mmap_cleanup;
	}

	result = doca_mmap_set_memrange(app_cfg->mmap, app_cfg->data_buffer, app_cfg->data_buffer_len);
	if (result != DOCA_SUCCESS) {
		printf("Unable to register memory with doca_mmap. Reason: %s\n", doca_get_error_string(result));
		goto mmap_cleanup;
	}

	result = doca_mmap_start(app_cfg->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to start doca_mmap. Reason: %s\n", doca_get_error_string(result));
		goto mmap_cleanup;
	}

	result = doca_workq_create(BUF_INVENTORY_POOL_SIZE, &app_cfg->workq);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create work queue. Reason: %s\n", doca_get_error_string(result));
		goto mmap_cleanup;
	}

	result = doca_ctx_workq_add(doca_regex_as_ctx(app_cfg->doca_regex), app_cfg->workq);
	if (result != DOCA_SUCCESS) {
		printf("Unable to attach work queue to RegEx. Reason: %s\n", doca_get_error_string(result));
		goto workq_destroy;
	}


	return DOCA_SUCCESS;

	doca_ctx_workq_rm(doca_regex_as_ctx(app_cfg->doca_regex), app_cfg->workq);
workq_destroy:
	doca_workq_destroy(app_cfg->workq);
mmap_cleanup:
	doca_mmap_destroy(app_cfg->mmap);
buf_inventory_cleanup:
	doca_buf_inventory_destroy(app_cfg->buf_inventory);
regex_cleanup:
	doca_ctx_stop(doca_regex_as_ctx(app_cfg->doca_regex));
regex_destroy:
	doca_regex_destroy(app_cfg->doca_regex);
	doca_dev_close(app_cfg->dev);
	return result;
}

/*
 * Prints RegEx match result
 *
 * @app_cfg [in]: Application configuration
 * @event [in]: DOCA event struct
 */
static void
report_results(struct test_dns_config *app_cfg, struct doca_event *event)
{
	struct test_dns_job_metadata * const meta = (struct test_dns_job_metadata *)event->user_data.ptr;
	struct doca_regex_search_result * const result = &(meta->result);
	struct doca_regex_match *match;
    printf("Detected matches: %d, num matches: %d\n", result->detected_matches, result->num_matches);

	if (result->detected_matches > 0)
		printf("Job complete. Detected %d match(es)\n", result->detected_matches);
	if (result->num_matches == 0)
		return;

	for (match = result->matches; match != NULL;) {
		printf("Matched rule Id: %12d\n", match->rule_id);
		match = match->next;
	}

	result->matches = NULL;
}

#define SLEEP_IN_NANOS (10 * 1000)		/* Sample the job every 10 microseconds */

int main() {
    struct test_dns_config cfg = {
        .rules_file_path = "/tmp/dns_filter_rules.rof2.binary",
        .pci_address = "03:00.0",
    };
    struct doca_buf *buf;
	struct test_dns_job_metadata meta = {0}, *ret;
    int res;
    char string[] = "www.youtube.com";
	struct doca_event event = {0};
    void *mbuf_data;
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = SLEEP_IN_NANOS,
	};

    test_dns_init(&cfg);

    printf("inv: %p, mmap: %p, data buffer: %p, len: %ld, buf: %p\n", cfg.buf_inventory, cfg.mmap, cfg.data_buffer, cfg.data_buffer_len, &buf);
    if (doca_buf_inventory_buf_by_addr(cfg.buf_inventory, cfg.mmap, cfg.data_buffer, cfg.data_buffer_len, &buf) != DOCA_SUCCESS) {
        printf("Failed to create inventory buf!\n");
        return 0;
    }

    memcpy(cfg.data_buffer, string, strlen(string));

    doca_buf_get_data(buf, &mbuf_data);
    doca_buf_set_data(buf, mbuf_data, strlen(string));

    struct doca_regex_job_search const job = {
        .base = {
            .ctx = doca_regex_as_ctx(cfg.doca_regex),
            .type = DOCA_REGEX_JOB_SEARCH,
            .user_data = { .ptr = &meta },
        },
        .rule_group_ids = { 1, 0, 0, 0 },
        .buffer = buf,
        .result = &(meta.result),
        .allow_batching = 0
    };

    printf("To match: %s\n", cfg.data_buffer);

    res = doca_workq_submit(cfg.workq, (struct doca_job *)&job);
    if (res == DOCA_SUCCESS) {
        /* store ref to job data so it can be released once a result is obtained */
        printf("Submit to workq...\n");
        meta.job_data = buf;
    } else if (res == DOCA_ERROR_NO_MEMORY) {
        printf("Unable to enqueue job. [%s]", doca_get_error_string(res));
        return -1;
    } else {
        printf("Unable to enqueue job. [%s]", doca_get_error_string(res));
        return -1;
    }

    do {
		res = doca_workq_progress_retrieve(cfg.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
		if (res == DOCA_SUCCESS) {
			/* Handle the completed jobs */
			ret = (struct test_dns_job_metadata *)event.user_data.ptr;
			if (ret->result.status_flags & DOCA_REGEX_STATUS_SEARCH_FAILED) {
				printf("RegEx search failed\n");
				if (ret->result.status_flags & DOCA_REGEX_STATUS_MAX_MATCH)
					printf("DOCA RegEx engine reached maximum number of matches, should reduce job size by using \"chunk-size\" flag\n");
				/* In case there are other jobs in workq, need to dequeue them and then to exit */
				return -1;
			} else {
				report_results(&cfg, &event);
            }
			doca_buf_refcount_rm(ret->job_data, NULL);
		} else if (res == DOCA_ERROR_AGAIN) {
            printf("Waiting for result...\n");
			nanosleep(&ts, &ts);
		} else {
			printf("Unable to dequeue results. [%s]\n", doca_get_error_string(res));
			return -1;
		}
	} while (res != DOCA_SUCCESS);

    return 0;
}