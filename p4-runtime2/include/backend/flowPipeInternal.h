#ifndef _FLOWPIPE_INTERNAL_H_
#define _FLOWPIPE_INTERNAL_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/* AUTOGEN HEADER */

struct flow_pipe;

struct flow_meta
{
    uint32_t pkt_meta;
    uint32_t u32[4];
} __attribute__((packed));

struct flow_header_eth
{
    uint8_t h_dest[6];
    uint8_t h_source[6];
    uint16_t h_proto;
} __attribute__((packed));

enum flow_l3_type
{
    FLOW_L3_TYPE_NONE=0,
    FLOW_L3_TYPE_IP4,
};

struct flow_header_ip4
{
    uint8_t version:4;
    uint8_t ihl:4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed));

enum flow_l4_type_ext
{
    FLOW_L4_TYPE_EXT_NONE=0,
    FLOW_L4_TYPE_EXT_TCP,
    FLOW_L4_TYPE_EXT_UDP,
};

struct flow_header_udp
{
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
} __attribute__((packed));

struct flow_header_tcp
{
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t doff:4;
    uint8_t res1:4;
    uint8_t flags;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
} __attribute__((packed));

struct flow_header_format
{
    struct flow_header_eth eth;
    uint16_t l2_valid_headers;
    enum flow_l3_type l3_type;
    struct flow_header_ip4 ip4;
    enum flow_l4_type_ext l4_type_ext;
    union {
        struct flow_header_udp udp;
        struct flow_header_tcp tcp;
    };
} __attribute__((packed));

enum flow_tun_type
{
    FLOW_TUN_NONE=0,
    FLOW_TUN_VXLAN,
};

struct flow_tun
{
    enum flow_tun_type type;
    uint32_t vxlan_tun_id;
} __attribute__((packed));

struct flow_match
{
    struct flow_meta meta;
    struct flow_header_format outer;
} __attribute__((packed));

struct flow_encap_action
{
    struct flow_header_format outer;
    struct flow_tun tun;
} __attribute__((packed));

struct flow_actions
{
    struct flow_meta meta;
    struct flow_header_format outer;
    bool has_encap;
    struct flow_encap_action encap;
} __attribute__((packed));

enum flow_fwd_type
{
    FLOW_FWD_NONE=0,
    FLOW_FWD_RSS,
    FLOW_FWD_HAIRPIN,
    FLOW_FWD_PORT,
    FLOW_FWD_PIPE,
    FLOW_FWD_DROP,
};

struct flow_fwd
{
    enum flow_fwd_type type;
    struct flow_pipe* next_pipe;
} __attribute__((packed));

enum flow_pipe_type
{
    FLOW_PIPE_BASIC=0,
    FLOW_PIPE_CONTROL,
};

enum flow_pipe_domain
{
    FLOW_PIPE_DOMAIN_INGRESS=0,
    FLOW_PIPE_DOMAIN_EGRESS,
};

struct flow_pipe_attr
{
    char* name;
    enum flow_pipe_type type;
    enum flow_pipe_domain domain;
    bool is_root;
    uint8_t nb_actions;
} __attribute__((packed));

struct flow_pipe_cfg
{
    struct flow_pipe_attr attr;
    struct flow_match* match;
    struct flow_actions** actions;
} __attribute__((packed));

/* END AUTOGEN HEADER */

#include "drivers/doca/common.h"
struct sk_buff;

struct pipe_operations {
    int (*create_pipe)(struct flow_pipe * pipe);
    int (*init_pipe)(struct flow_pipe * pipe);
    int (*get_actions_list)(struct flow_pipe * pipe);
    int (*add_pipe_entry)(struct flow_pipe * pipe, const char * action_name, ...);
    struct flow_pipe *(*run)(struct flow_pipe * pipe, struct sk_buff *skb, ...);
};

#define DOCA_MAX_PORTS  2

struct hw_flow_pipe {
    struct doca_flow_pipe * pipe[DOCA_MAX_PORTS];
    struct pipe_operations ops;
};

struct sw_flow_pipe {
    void * pipe;
    struct pipe_operations ops;
    struct list_head list;
};

struct actionEntry {
    char name[32];
    int id;
};

struct flow_pipe {
    uint32_t id;
    char name[64];
    int nb_actions;
    struct actionEntry ** actions;
    struct hw_flow_pipe hwPipe;
    struct sw_flow_pipe swPipe;
};

extern uint32_t pipe_id;

struct flow_pipe * instantialize_flow_pipe(const char * name);

uint8_t * get_packet_internal(struct sk_buff * skb);
uint32_t get_packet_size_internal(struct sk_buff * skb);
struct sk_buff* prepend_packet_internal(struct sk_buff* skb, uint32_t prepend_size);
bool fsm_table_lookup_internal(struct flow_match* pipe_match, struct flow_match* match, struct sk_buff* skb);

struct flow_pipe* fsm_table_lookup(struct flow_pipe* pipe, struct sk_buff* skb);

struct flow_pipe* flow_get_pipe(char* pipe_name);
int flow_get_pipe_id_by_name(char* pipe_name);
int flow_create_hw_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss, struct flow_pipe* pipe);
int flow_hw_control_pipe_add_entry(struct flow_pipe* pipe, uint8_t priority, struct flow_match* match, struct flow_actions* action, struct flow_fwd* fwd);
int flow_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* actions, struct flow_fwd* fwd);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _FLOWPIPE_INTERNAL_H_ */