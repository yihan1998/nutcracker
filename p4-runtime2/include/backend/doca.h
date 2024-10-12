#ifndef _BACKEND_DOCA_H_
#define _BACKEND_DOCA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "flowPipeInternal.h"

int test_create_pipe();
int test_create_vxlan_encap_pipe();

int doca_create_hw_pipe(struct flow_pipe* pipe, struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss);
int doca_create_hw_control_pipe(struct flow_pipe* pipe, struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss);

int doca_hw_control_pipe_add_entry_for_port(int port_id, struct doca_flow_pipe* pipe, int priority, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd);
int doca_hw_control_pipe_add_entry(struct flow_pipe* pipe, int priority, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd);
int doca_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match *match, struct flow_actions* actions, struct flow_fwd* fwd);

struct dpdk_config;

int dpdk_init(int argc, char ** argv);
int doca_init();

int vxlan_encap_offloading();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _BACKEND_DOCA_H_ */