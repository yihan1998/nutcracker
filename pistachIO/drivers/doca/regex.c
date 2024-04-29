#include <regex.h>

#include "kernel/threads.h"
#include "doca/utils.h"
#include "doca/common.h"
#include "doca/regex.h"

#include "opt.h"
#include "printk.h"
#include "libc.h"

#define MAX_RULES	100
#define MAX_REGEX_LENGTH	256

#ifdef CONFIG_DOCA_REGEX
struct regex {
	regex_t * rules;
	int nb_rule;
	uint8_t * rule_buf;
	int buf_len;
};
struct doca_regex_config doca_regex_cfg = {
	.pci_address = "03:00.0",   
};
#endif /* CONFIG_DOCA_REGEX */

/*
 * Printing the RegEx results
 *
 * @regex_cfg [in]: sample RegEx configuration struct
 * @event [in]: DOCA event structure
 * @chunk_len [in]: chunk size, used for calculate job data offset
 */
// static void regex_scan_report_results(struct doca_event * event) {
// 	struct doca_regex_match *ptr;
// 	struct doca_regex_search_result * const result = (struct doca_regex_search_result *)event->result.ptr;

// 	if (result->num_matches == 0) {
// 		pr_debug(REGEX_DEBUG, "No matching rule!\n");
// 		return;
// 	}
// 	ptr = result->matches;
// 	pr_debug(REGEX_DEBUG, "Match rule id: %d\n", ptr->rule_id);
// }

struct doca_regex_match_metadata {
	struct doca_buf *job_data;		/* Pointer to the data to be scanned with this job */
	struct doca_regex_search_result result;	/* Storage for results */
};

/*
 * Prints RegEx match result
 *
 * @app_cfg [in]: Application configuration
 * @event [in]: DOCA event struct
 */
int report_results(struct doca_event *event) {
	struct doca_regex_match_metadata * const meta = (struct doca_regex_match_metadata *)event->user_data.ptr;
	struct doca_regex_search_result * const result = &(meta->result);
	struct doca_regex_match *match;
    printf("Detected matches: %d\n", result->detected_matches);

	if (result->detected_matches > 0)
		printf("Job complete. Detected %d match(es), num matched: %d\n", result->detected_matches, result->num_matches);
	if (result->num_matches == 0)
		return 0;

	for (match = result->matches; match != NULL;) {
		printf("Matched rule Id: %12d\n", match->rule_id);
		match = match->next;
	}

	result->matches = NULL;
	return 0;
}

doca_error_t doca_regex_percore_init(struct doca_regex_ctx * regex_ctx) {
	doca_error_t result;

	char *rules_file_data;
	size_t rules_file_size;

    regex_ctx->data_buffer = (char *)calloc(1024, sizeof(char));
    regex_ctx->data_buffer_len = 1024;

	/* Attach rules file to DOCA RegEx */
	result = read_file("/tmp/dns_filter_rules.rof2.binary", &rules_file_data, &rules_file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to load rules file content. Reason: %s", doca_get_error_string(result));
		return 0;
	}

	/* Attach rules file to DOCA RegEx */
	result = doca_regex_set_hardware_compiled_rules(regex_ctx->doca_reg, rules_file_data, rules_file_size);
	if (result != DOCA_SUCCESS) {
		printf("Unable to program rules file. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_buf_inventory_create(NULL, 1, DOCA_BUF_EXTENSION_NONE, &regex_ctx->buf_inv);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_buf_inventory. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_buf_inventory_start(regex_ctx->buf_inv);
	if (result != DOCA_SUCCESS) {
		printf("Unable to start doca_buf_inventory. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_create(NULL, &regex_ctx->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to create doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_dev_add(regex_ctx->mmap, regex_ctx->dev);
	if (result != DOCA_SUCCESS) {
		printf("Unable to add device to doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_set_memrange(regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len);
	if (result != DOCA_SUCCESS) {
		printf("Unable to register memory with doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	result = doca_mmap_start(regex_ctx->mmap);
	if (result != DOCA_SUCCESS) {
		printf("Unable to start doca_mmap. Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	if (doca_buf_inventory_buf_by_addr(regex_ctx->buf_inv, regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len, &regex_ctx->buf) != DOCA_SUCCESS) {
        printf("Failed to create inventory buf!\n");
        return 0;
    }
#if 0
	struct doca_buf *buf;
	struct doca_regex_match_metadata meta, *ret;
    int res;
    char string[] = "www.youtube.com";
	struct doca_event event = {0};
    void *mbuf_data;

	if (doca_buf_inventory_buf_by_addr(regex_ctx->buf_inv, regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len, &buf) != DOCA_SUCCESS) {
        printf("Failed to create inventory buf!\n");
        return 0;
    }

    memcpy(regex_ctx->data_buffer, string, strlen(string));

    doca_buf_get_data(buf, &mbuf_data);
    doca_buf_set_data(buf, mbuf_data, strlen(string));

    struct doca_regex_job_search const job = {
        .base = {
            .ctx = doca_regex_as_ctx(regex_ctx->doca_reg),
            .type = DOCA_REGEX_JOB_SEARCH,
            .user_data = { .ptr = &meta },
        },
        .rule_group_ids = { 1, 0, 0, 0 },
        .buffer = buf,
        .result = &(meta.result),
        .allow_batching = 0
    };

    res = doca_workq_submit(worker_ctx->workq, (struct doca_job *)&job);
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
		res = doca_workq_progress_retrieve(worker_ctx->workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
		if (res == DOCA_SUCCESS) {
			/* Handle the completed jobs */
			ret = (struct doca_regex_match_metadata *)event.user_data.ptr;
			if (ret->result.status_flags & DOCA_REGEX_STATUS_SEARCH_FAILED) {
				printf("RegEx search failed\n");
				if (ret->result.status_flags & DOCA_REGEX_STATUS_MAX_MATCH)
					printf("DOCA RegEx engine reached maximum number of matches, should reduce job size by using \"chunk-size\" flag\n");
				/* In case there are other jobs in workq, need to dequeue them and then to exit */
				return -1;
			} else {
				report_results(&event);
            }
			doca_buf_refcount_rm(ret->job_data, NULL);
		} else if (res == DOCA_ERROR_AGAIN) {
            printf("Waiting for result...\n");
		} else {
			printf("Unable to dequeue results. [%s]\n", doca_get_error_string(res));
			return -1;
		}
	} while (res != DOCA_SUCCESS);
#endif
	return 0;

#if 0
	doca_error_t result;

	regex_ctx->data_buffer = (char *)calloc(1024, sizeof(char));
	regex_ctx->data_buffer_len = 1024;

	/* Create and start buffer inventory */
	result = doca_buf_inventory_create(NULL, 1, DOCA_BUF_EXTENSION_NONE, &regex_ctx->buf_inv);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to create buffer inventory. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_buf_inventory_start(regex_ctx->buf_inv);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to start buffer inventory. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_mmap_create(NULL, &regex_ctx->mmap);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to create doca_mmap. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_mmap_dev_add(regex_ctx->mmap, regex_ctx->dev);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to add device to doca_mmap. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_mmap_set_memrange(regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to register memory with doca_mmap. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_mmap_start(regex_ctx->mmap);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to start doca_mmap. Reason: %s", doca_get_error_string(result));
		return result;
	}

	result = doca_buf_inventory_buf_by_addr(regex_ctx->buf_inv, regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len, &regex_ctx->buf);
	if (result != DOCA_SUCCESS) {
		pr_err("Failed to create buf inventory! Reason: %s\n", doca_get_error_string(result));
		return 0;
	}

	/* Attach rules file to DOCA RegEx */
	char *rules_file_data;
	size_t rules_file_size;
	result = read_file("/tmp/dns_filter_rules.rof2.binary", &rules_file_data, &rules_file_size);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to load rules file content. Reason: %s", doca_get_error_string(result));
		return -1;
	}

	result = doca_regex_set_hardware_compiled_rules(regex_ctx->doca_reg, rules_file_data, rules_file_size);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to program rules. Reason: %s", doca_get_error_string(result));
		free(rules_file_data);
		return -1;
	}
	free(rules_file_data);

	struct doca_event event = {0};
	struct doca_regex_match_metadata meta, *ret;
	// const char * data = __String;
	// int data_len = strlen(__String);
	void * mbuf_data;

	// pr_debug(REGEX_DEBUG, "Matching %s...\n", __String);
	// memcpy(worker_ctx->regex_ctx.data_buffer, data, data_len);

	// doca_buf_get_data(worker_ctx->regex_ctx.buf, &mbuf_data);
	// doca_buf_set_data(worker_ctx->regex_ctx.buf, mbuf_data, data_len);

	char string[] = "credit card: 2142 0123 0123 0123\n";
	memcpy(worker_ctx->regex_ctx.data_buffer, string, strlen(string));

	doca_buf_get_data(worker_ctx->regex_ctx.buf, &mbuf_data);
    doca_buf_set_data(worker_ctx->regex_ctx.buf, mbuf_data, strlen(string));

	struct doca_regex_job_search const job_request = {
		.base = {
			.ctx = doca_regex_as_ctx(worker_ctx->regex_ctx.doca_reg),
			.type = DOCA_REGEX_JOB_SEARCH,
            .user_data = { .ptr = &meta },
		},
		.rule_group_ids = {1, 0, 0, 0},
		.buffer = worker_ctx->regex_ctx.buf,
        .result = &(meta.result),
		.allow_batching = 0,
	};

	result = doca_workq_submit(worker_ctx->workq, (struct doca_job *)&job_request);
	if (result == DOCA_SUCCESS) {
        meta.job_data = worker_ctx->regex_ctx.buf;
	} else if (result == DOCA_ERROR_NO_MEMORY) {
		// doca_buf_refcount_rm(buf, NULL);
		return -1;
	}

	/* Wait for job completion */
	while ((result = doca_workq_progress_retrieve(worker_ctx->workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) == DOCA_ERROR_AGAIN);

	if (result == DOCA_SUCCESS) {
		/* Handle the completed jobs */
		ret = (struct doca_regex_match_metadata *)event.user_data.ptr;
		if (ret->result.status_flags & DOCA_REGEX_STATUS_SEARCH_FAILED) {
			printf("RegEx search failed\n");
			if (ret->result.status_flags & DOCA_REGEX_STATUS_MAX_MATCH)
				printf("DOCA RegEx engine reached maximum number of matches, should reduce job size by using \"chunk-size\" flag\n");
			/* In case there are other jobs in workq, need to dequeue them and then to exit */
			return -1;
		} else {
			printf("RegEx search succeeded!\n");
			regex_scan_report_results(&event);
		}
	}
	
	return DOCA_SUCCESS;
#endif
}

/*
 * RegEx context initialization
 *
 * @doca_regex_cfg [in/out]: application configuration structure
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t doca_regex_init(void) {
#ifdef CONFIG_DOCA_REGEX
	doca_error_t result;

	/* Open DOCA device */
	result = open_doca_device_with_pci(doca_regex_cfg.pci_address, NULL, &doca_regex_cfg.dev);
	if (result != DOCA_SUCCESS) {
		pr_err("No device matching PCI address found. Reason: %s", doca_get_error_string(result));
		return result;
	}

	/* Create a DOCA RegEx instance */
	result = doca_regex_create(&(doca_regex_cfg.doca_reg));
	if (result != DOCA_SUCCESS) {
		pr_err("DOCA RegEx creation Failed. Reason: %s", doca_get_error_string(result));
		doca_dev_close(doca_regex_cfg.dev);
		return DOCA_ERROR_INITIALIZATION;
	}

	/* Set hw RegEx device to DOCA RegEx */
	result = doca_ctx_dev_add(doca_regex_as_ctx(doca_regex_cfg.doca_reg), doca_regex_cfg.dev);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to install RegEx device. Reason: %s", doca_get_error_string(result));
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}
	/* Set matches memory pool to 0 because the app needs to check if there are matches and don't need the matches details  */
	result = doca_regex_set_workq_matches_memory_pool_size(doca_regex_cfg.doca_reg, 0);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to create match memory pools. Reason: %s", doca_get_error_string(result));
		return 0;
	}

	/* Start DOCA RegEx */
	result = doca_ctx_start(doca_regex_as_ctx(doca_regex_cfg.doca_reg));
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to start DOCA RegEx. Reason: %s", doca_get_error_string(result));
		result = DOCA_ERROR_INITIALIZATION;
		return 0;
	}
#endif
	return DOCA_SUCCESS;
}

int regcomp(regex_t *_Restrict_ __preg, const char *_Restrict_ __pattern, int __cflags) {
    // pr_debug(REGEX_DEBUG, "Compiling regex rule: %s...\n", __pattern);
#ifndef CONFIG_DOCA_REGEX
    return libc_regcomp(__preg, __pattern, __cflags);
#else
#if 0
	doca_error_t result;
    int ret;
	struct regex * regex = (struct regex *)__preg;

	if (!regex->rule_buf) {
		regex->rules = (regex_t *)malloc(MAX_RULES * sizeof(regex_t));
		regex->nb_rule = 0;
		regex->rule_buf = (uint8_t *)malloc(MAX_RULES * MAX_REGEX_LENGTH);
		regex->buf_len = 0;
	}

	ret = libc_regcomp(&regex->rules[regex->nb_rule], __pattern, REG_EXTENDED);
	if (ret) {
		pr_err("Could not compile regex: %s\n\n", __pattern);
		return -1;
	}

	// regex->nb_rule++;

	// memcpy(&regex->rule_buf[regex->buf_len], __pattern, strlen(__pattern));
	// regex->buf_len += strlen(__pattern);

	// printf("Last character: %c\n",__pattern[strlen(__pattern) - 1]);

	/* Attach rules file to DOCA RegEx */
	char *rules_file_data;
	size_t rules_file_size;
	result = read_file("/tmp/dns_filter_rules.rof2.binary", &rules_file_data, &rules_file_size);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to load rules file content. Reason: %s", doca_get_error_string(result));
		return -1;
	}

	result = doca_regex_set_hardware_compiled_rules(doca_regex_cfg.doca_reg, rules_file_data, rules_file_size);
	if (result != DOCA_SUCCESS) {
		pr_err("Unable to program rules. Reason: %s", doca_get_error_string(result));
		free(rules_file_data);
		return -1;
	}
	free(rules_file_data);
#endif

	return 0;
#endif
}

int regexec(const regex_t *_Restrict_ __preg, const char *_Restrict_ __String, size_t __nmatch,
			regmatch_t __pmatch[_Restrict_arr_ _REGEX_NELTS (__nmatch)], int __eflags) {
#ifndef CONFIG_DOCA_REGEX
    return libc_regexec(__preg, __String, __nmatch, __pmatch, __eflags);
#else
	struct doca_regex_match_metadata meta, *ret;
    int res;
	struct doca_event event = {0};
    void *mbuf_data;
	struct doca_regex_ctx * regex_ctx = &worker_ctx->regex_ctx;

	// if (doca_buf_inventory_buf_by_addr(regex_ctx->buf_inv, regex_ctx->mmap, regex_ctx->data_buffer, regex_ctx->data_buffer_len, &buf) != DOCA_SUCCESS) {
    //     printf("Failed to create inventory buf!\n");
    //     return 0;
    // }

    memcpy(regex_ctx->data_buffer, __String, strlen(__String));

    doca_buf_get_data(regex_ctx->buf, &mbuf_data);
    doca_buf_set_data(regex_ctx->buf, mbuf_data, strlen(__String));

    struct doca_regex_job_search const job = {
        .base = {
            .ctx = doca_regex_as_ctx(regex_ctx->doca_reg),
            .type = DOCA_REGEX_JOB_SEARCH,
            .user_data = { .ptr = &meta },
        },
        .rule_group_ids = { 1, 0, 0, 0 },
        .buffer = regex_ctx->buf,
        .result = &(meta.result),
        .allow_batching = 0
    };

    res = doca_workq_submit(worker_ctx->workq, (struct doca_job *)&job);
    if (res == DOCA_SUCCESS) {
        /* store ref to job data so it can be released once a result is obtained */
        meta.job_data = regex_ctx->buf;
    } else if (res == DOCA_ERROR_NO_MEMORY) {
        printf("Unable to enqueue job. [%s]", doca_get_error_string(res));
        return -1;
    } else {
        printf("Unable to enqueue job. [%s]", doca_get_error_string(res));
        return -1;
    }

    do {
		res = doca_workq_progress_retrieve(worker_ctx->workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
		if (res == DOCA_SUCCESS) {
			/* Handle the completed jobs */
			ret = (struct doca_regex_match_metadata *)event.user_data.ptr;
			if (ret->result.status_flags & DOCA_REGEX_STATUS_SEARCH_FAILED) {
				printf("RegEx search failed\n");
				if (ret->result.status_flags & DOCA_REGEX_STATUS_MAX_MATCH)
					printf("DOCA RegEx engine reached maximum number of matches, should reduce job size by using \"chunk-size\" flag\n");
				/* In case there are other jobs in workq, need to dequeue them and then to exit */
				return -1;
			} else {
				// report_results(&event);
				struct doca_regex_match_metadata * search_meta = (struct doca_regex_match_metadata *)event.user_data.ptr;
				struct doca_regex_search_result * search_result = &(search_meta->result);
				if (search_result->detected_matches > 0) return 0;
				else return -1; 
            }
			doca_buf_refcount_rm(ret->job_data, NULL);
		} else if (res == DOCA_ERROR_AGAIN) {
			continue;
		} else {
			printf("Unable to dequeue results. [%s]\n", doca_get_error_string(res));
			return -1;
		}
	} while (res != DOCA_SUCCESS);

	return 0;
#endif
}