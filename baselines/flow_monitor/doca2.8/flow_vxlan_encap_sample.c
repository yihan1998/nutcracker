/*
 * Copyright (c) 2022 NVIDIA CORPORATION AND AFFILIATES.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include <unistd.h>

#include <doca_log.h>
#include <doca_flow.h>

#include "flow_common.h"

DOCA_LOG_REGISTER(FLOW_VXLAN_ENCAP);

/*
 * Create DOCA Flow pipe with 5 tuple match and set pkt meta value
 *
 * @port [in]: port of the pipe
 * @port_id [in]: port ID of the pipe
 * @pipe [out]: created pipe pointer
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise.
 */
static doca_error_t create_match_pipe(struct doca_flow_port *port, int port_id, struct doca_flow_pipe **pipe)
{
	struct doca_flow_match match;
	struct doca_flow_actions actions, *actions_arr[NB_ACTIONS_ARR];
	struct doca_flow_fwd fwd;
	struct doca_flow_monitor counter;
	struct doca_flow_pipe_cfg *pipe_cfg;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));
	memset(&fwd, 0, sizeof(fwd));
	memset(&counter, 0, sizeof(counter));

	/* 5 tuple match */
	SET_MAC_ADDR(match.outer.eth.dst_mac, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
	counter.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;

	result = doca_flow_pipe_cfg_create(&pipe_cfg, port);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create doca_flow_pipe_cfg: %s", doca_error_get_descr(result));
		return result;
	}

	result = set_flow_pipe_cfg(pipe_cfg, "MATCH_PIPE", DOCA_FLOW_PIPE_BASIC, true);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set doca_flow_pipe_cfg: %s", doca_error_get_descr(result));
		goto destroy_pipe_cfg;
	}
	result = doca_flow_pipe_cfg_set_match(pipe_cfg, &match, NULL);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set doca_flow_pipe_cfg match: %s", doca_error_get_descr(result));
		goto destroy_pipe_cfg;
	}
	result = doca_flow_pipe_cfg_set_actions(pipe_cfg, actions_arr, NULL, NULL, NB_ACTIONS_ARR);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set doca_flow_pipe_cfg monitor: %s", doca_error_get_descr(result));
		goto destroy_pipe_cfg;
	}
	result = doca_flow_pipe_cfg_set_monitor(pipe_cfg, &counter);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set doca_flow_pipe_cfg counter: %s", doca_error_get_descr(result));
		goto destroy_pipe_cfg;
	}

	/* forwarding traffic to other port */
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = port_id ^ 1;

	result = doca_flow_pipe_create(pipe_cfg, &fwd, NULL, pipe);
destroy_pipe_cfg:
	doca_flow_pipe_cfg_destroy(pipe_cfg);
	return result;
}

/*
 * Add DOCA Flow pipe entry with example 5 tuple match
 *
 * @pipe [in]: pipe of the entry
 * @status [in]: user context for adding entry
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise.
 */
static doca_error_t add_match_pipe_entry(struct doca_flow_pipe *pipe, int id,
							struct entries_status *status, 
						   	struct doca_flow_pipe_entry **entry)
{
	struct doca_flow_match match;
	struct doca_flow_actions actions;
	doca_error_t result;

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));

	SET_MAC_ADDR(match.outer.eth.src_mac, 0xa0, 0x99, 0xc2, 0x31, 0xf7, i);

	result = doca_flow_pipe_add_entry(0, pipe, &match, &actions, NULL, NULL, 0, status, entry);
	if (result != DOCA_SUCCESS)
		return result;

	return DOCA_SUCCESS;
}

/*
 * Add DOCA Flow pipe entry with example encap values
 *
 * @pipe [in]: pipe of the entry
 * @vxlan_type [in]: vxlan type
 * @status [in]: user context for adding entry
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise.
 */
static doca_error_t add_vxlan_encap_pipe_entry(struct doca_flow_pipe *pipe,
					       enum doca_flow_tun_ext_vxlan_type vxlan_type,
					       struct entries_status *status, 
						   struct doca_flow_pipe_entry **entry)
{
	struct doca_flow_match match;
	struct doca_flow_actions actions;
	doca_error_t result;

	doca_be32_t encap_dst_ip_addr = BE_IPV4_ADDR(81, 81, 81, 81);
	doca_be32_t encap_src_ip_addr = BE_IPV4_ADDR(11, 21, 31, 41);
	doca_be16_t encap_flags_fragment_offset = RTE_BE16(DOCA_FLOW_IP4_FLAG_DONT_FRAGMENT);
	uint8_t encap_ttl = 17;
	doca_be32_t encap_vxlan_tun_id = BUILD_VNI(0xadadad);
	uint8_t src_mac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	uint8_t dst_mac[] = {0x94, 0x6d, 0xae, 0xe6, 0x90, 0x32};

	memset(&match, 0, sizeof(match));
	memset(&actions, 0, sizeof(actions));

	match.meta.pkt_meta = 1;

	SET_MAC_ADDR(actions.encap_cfg.encap.outer.eth.src_mac,
		     src_mac[0],
		     src_mac[1],
		     src_mac[2],
		     src_mac[3],
		     src_mac[4],
		     src_mac[5]);
	SET_MAC_ADDR(actions.encap_cfg.encap.outer.eth.dst_mac,
		     dst_mac[0],
		     dst_mac[1],
		     dst_mac[2],
		     dst_mac[3],
		     dst_mac[4],
		     dst_mac[5]);
	actions.encap_cfg.encap.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
	actions.encap_cfg.encap.outer.ip4.src_ip = encap_src_ip_addr;
	actions.encap_cfg.encap.outer.ip4.dst_ip = encap_dst_ip_addr;
	actions.encap_cfg.encap.outer.ip4.flags_fragment_offset = encap_flags_fragment_offset;
	actions.encap_cfg.encap.outer.ip4.ttl = encap_ttl;
	actions.encap_cfg.encap.tun.type = DOCA_FLOW_TUN_VXLAN;
	actions.encap_cfg.encap.tun.vxlan_tun_id = encap_vxlan_tun_id;
	actions.action_idx = 0;

	switch (vxlan_type) {
	case DOCA_FLOW_TUN_EXT_VXLAN_GBP:
		actions.encap_cfg.encap.tun.vxlan_group_policy_id = RTE_BE16(0x1234);
		break;
	case DOCA_FLOW_TUN_EXT_VXLAN_GPE:
		actions.encap_cfg.encap.tun.vxlan_next_protocol = DOCA_FLOW_VXLAN_GPE_TYPE_ETH;
		break;
	case DOCA_FLOW_TUN_EXT_VXLAN_STANDARD:
		break;
	default:
		DOCA_LOG_ERR("Failed to add vxlan encap entry: invalid vxlan type %d", vxlan_type);
		return DOCA_ERROR_INVALID_VALUE;
	}

	result = doca_flow_pipe_add_entry(0, pipe, &match, &actions, NULL, NULL, 0, status, entry);
	if (result != DOCA_SUCCESS)
		return result;

	return DOCA_SUCCESS;
}

/*
 * Run flow_vxlan_encap sample
 *
 * @nb_queues [in]: number of queues the sample will use
 * @vxlan_type [in]: vxlan type
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise.
 */
doca_error_t flow_vxlan_encap(int nb_queues, enum doca_flow_tun_ext_vxlan_type vxlan_type)
{
	int nb_ports = 2;
#ifdef ENABLE_COUNTER
	struct flow_resources resource = {.nr_counters = 64};
#else
	struct flow_resources resource = {0};
#endif	/* ENABLE_COUNTER */
	uint32_t nr_shared_resources[SHARED_RESOURCE_NUM_VALUES] = {0};
	struct doca_flow_port *ports[nb_ports];
	struct doca_dev *dev_arr[nb_ports];
	struct doca_flow_pipe *pipe;
	struct entries_status status_ingress;
	int num_of_entries_ingress = 190;
	struct entries_status status_egress;
	int num_of_entries_egress = 1;
	doca_error_t result;
	int port_id;

	result = init_doca_flow(nb_queues, "vnf,hws", &resource, nr_shared_resources);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to init DOCA Flow: %s", doca_error_get_descr(result));
		return result;
	}

	memset(dev_arr, 0, sizeof(struct doca_dev *) * nb_ports);
	result = init_doca_flow_ports(nb_ports, ports, true, dev_arr);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to init DOCA ports: %s", doca_error_get_descr(result));
		doca_flow_destroy();
		return result;
	}

	for (port_id = 0; port_id < nb_ports; port_id++) {
		memset(&status_ingress, 0, sizeof(status_ingress));
		memset(&status_egress, 0, sizeof(status_egress));

		result = create_match_pipe(ports[port_id], port_id, &pipe);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to create match pipe: %s", doca_error_get_descr(result));
			stop_doca_flow_ports(nb_ports, ports);
			doca_flow_destroy();
			return result;
		}

		for (int i = 0; i < 190; i++) {
			struct doca_flow_pipe_entry * entry;
			result = add_match_pipe_entry(pipe, i, &status_ingress, &entry);
			if (result != DOCA_SUCCESS) {
				DOCA_LOG_ERR("Failed to add entry to match pipe: %s", doca_error_get_descr(result));
				stop_doca_flow_ports(nb_ports, ports);
				doca_flow_destroy();
				return result;
			}
		}

		result = doca_flow_entries_process(ports[port_id], 0, DEFAULT_TIMEOUT_US, num_of_entries_ingress);
		if (result != DOCA_SUCCESS) {
			DOCA_LOG_ERR("Failed to process entries: %s", doca_error_get_descr(result));
			stop_doca_flow_ports(nb_ports, ports);
			doca_flow_destroy();
			return result;
		}

		if (status_ingress.nb_processed != num_of_entries_ingress || status_ingress.failure) {
			DOCA_LOG_ERR("Failed to process entries");
			stop_doca_flow_ports(nb_ports, ports);
			doca_flow_destroy();
			return DOCA_ERROR_BAD_STATE;
		}
	}

	DOCA_LOG_INFO("Wait few seconds for packets to arrive");
	while(1) {
#if 0
		sleep(1);
		struct doca_flow_resource_query stats;
		for (int port_id = 0; port_id < nb_ports; port_id++) {
			result = doca_flow_resource_query_entry(match_entry[port_id], &stats);
			if (result != DOCA_SUCCESS) {
				DOCA_LOG_ERR("Port %d failed to query match pipe entry: %s",
								port_id, doca_error_get_descr(result));
				return result;
			}
			DOCA_LOG_INFO("Port %d, match pipe entry received %lu packets", port_id, stats.counter.total_pkts);

			result = doca_flow_resource_query_entry(encap_entry[port_id], &stats);
			if (result != DOCA_SUCCESS) {
				DOCA_LOG_ERR("Port %d failed to query encap pipe entry: %s",
								port_id, doca_error_get_descr(result));
				return result;
			}
			DOCA_LOG_INFO("Port %d, encap pipe entry received %lu packets", port_id, stats.counter.total_pkts);
		}
#endif	/* ENABLE_COUNTER */
	}

	result = stop_doca_flow_ports(nb_ports, ports);
	doca_flow_destroy();
	return result;
}
