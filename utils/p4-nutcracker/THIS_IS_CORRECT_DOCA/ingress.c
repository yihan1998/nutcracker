#include "ingress.h"

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

extern int flow_create_hw_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd* fwd, struct flow_fwd* fwd_miss, struct flow_pipe* pipe);
extern struct flow_pipe* flow_get_pipe(const char* pipe_name);
extern int flow_get_pipe_id_by_name(const char* pipe_name);
extern int flow_hw_control_pipe_add_entry(struct flow_pipe* pipe, uint8_t priority, struct flow_match* match, struct flow_actions* actions, struct flow_fwd* fwd);
extern int flow_hw_pipe_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* actions, struct flow_fwd* fwd);
int create_ingress_hairpin_2_hw_pipe(struct flow_pipe* pipe);
int create_ingress_rss_0_hw_pipe(struct flow_pipe* pipe);
int create_ingress_udp_tbl_0_hw_pipe(struct flow_pipe* pipe);
int create_ingress_rss_1_hw_pipe(struct flow_pipe* pipe);
int create_tbl_rss_1_hw_pipe(struct flow_pipe* pipe);
int create_node_2_0_hw_pipe(struct flow_pipe* pipe);
int add_ingress_udp_tbl_0_hw_pipe_entry(struct flow_pipe* pipe, const char* next_action, uint16_t dest);
int add_tbl_rss_1_hw_pipe_entry(struct flow_pipe* pipe, const char* next_action);
int init_node_2_0_hw_pipe(struct flow_pipe* pipe);

int add_ingress_udp_tbl_0_hw_pipe_entry(struct flow_pipe* pipe, const char* next_action, uint16_t dest)
{
    struct flow_match match;
    struct flow_actions actions;
    struct flow_fwd fwd;
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    match.outer.l4_type_ext=FLOW_L4_TYPE_EXT_UDP;
    match.outer.l3_type=FLOW_L3_TYPE_IP4;
    match.outer.udp.dest=dest;
    actions.meta.pkt_meta=flow_get_pipe_id_by_name(next_action);
    fwd.type=FLOW_FWD_PIPE;
    fwd.next_pipe=flow_get_pipe(next_action);
    return flow_hw_pipe_add_entry(pipe,&match,&actions,&fwd);
}

int add_tbl_rss_1_hw_pipe_entry(struct flow_pipe* pipe, const char* next_action)
{
    struct flow_match match;
    struct flow_actions actions;
    struct flow_fwd fwd;
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    actions.meta.pkt_meta=flow_get_pipe_id_by_name(next_action);
    fwd.type=FLOW_FWD_PIPE;
    fwd.next_pipe=flow_get_pipe(next_action);
    return flow_hw_pipe_add_entry(pipe,&match,&actions,&fwd);
}

int create_ingress_hairpin_2_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_actions* actions_arr[1];
    struct flow_fwd fwd;
    struct flow_fwd fwd_miss;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    actions_arr[0]=&actions;
    pipe_cfg.attr.name="ingress_hairpin_2";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_INGRESS;
    pipe_cfg.match=&match;
    pipe_cfg.actions=actions_arr;
    pipe_cfg.attr.nb_actions=1;
    actions.meta.pkt_meta=flow_get_pipe_id_by_name("ingress_hairpin_2");
    fwd.type=FLOW_FWD_HAIRPIN;
    fwd_miss.type=FLOW_FWD_RSS;
    return flow_create_hw_pipe(&pipe_cfg,&fwd,&fwd_miss,pipe);
}

int create_ingress_rss_0_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_actions* actions_arr[1];
    struct flow_fwd fwd;
    struct flow_fwd fwd_miss;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    actions_arr[0]=&actions;
    pipe_cfg.attr.name="ingress_rss_0";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_INGRESS;
    pipe_cfg.match=&match;
    pipe_cfg.actions=actions_arr;
    pipe_cfg.attr.nb_actions=1;
    actions.meta.pkt_meta=flow_get_pipe_id_by_name("ingress_rss_0");
    fwd.type=FLOW_FWD_RSS;
    fwd_miss.type=FLOW_FWD_RSS;
    return flow_create_hw_pipe(&pipe_cfg,&fwd,&fwd_miss,pipe);
}

int create_ingress_rss_1_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_actions* actions_arr[1];
    struct flow_fwd fwd;
    struct flow_fwd fwd_miss;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    actions_arr[0]=&actions;
    pipe_cfg.attr.name="ingress_rss_1";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_INGRESS;
    pipe_cfg.match=&match;
    pipe_cfg.actions=actions_arr;
    pipe_cfg.attr.nb_actions=1;
    actions.meta.pkt_meta=flow_get_pipe_id_by_name("ingress_rss_1");
    fwd.type=FLOW_FWD_RSS;
    fwd_miss.type=FLOW_FWD_RSS;
    return flow_create_hw_pipe(&pipe_cfg,&fwd,&fwd_miss,pipe);
}

int create_ingress_udp_tbl_0_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_actions* actions_arr[1];
    struct flow_fwd fwd_miss;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    actions_arr[0]=&actions;
    pipe_cfg.attr.name="ingress_udp_tbl_0";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_INGRESS;
    pipe_cfg.match=&match;
    pipe_cfg.actions=actions_arr;
    pipe_cfg.attr.nb_actions=1;
    // match.meta.pkt_meta=0xffffffff;
    match.outer.l4_type_ext=FLOW_L4_TYPE_EXT_UDP;
    match.outer.l3_type=FLOW_L3_TYPE_IP4;
    match.outer.udp.dest=0xffff;
    actions.meta.pkt_meta=0xffffffff;
    fwd_miss.type=FLOW_FWD_DROP;
    flow_create_hw_pipe(&pipe_cfg,0,&fwd_miss,pipe);
    return 0;
}

int create_node_2_0_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_fwd fwd_miss;
    struct flow_pipe* fwd_miss_pipe;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    pipe_cfg.attr.name="node_2_0";
    pipe_cfg.attr.type=FLOW_PIPE_CONTROL;
    pipe_cfg.attr.is_root=true;
    fwd_miss.type=FLOW_FWD_RSS;
    flow_create_hw_pipe(&pipe_cfg,0,&fwd_miss,pipe);
    return 0;
}

int create_tbl_rss_1_hw_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_actions* actions_arr[1];
    struct flow_fwd fwd;
    struct flow_fwd fwd_miss;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    memset(&fwd_miss,0,sizeof(fwd_miss));
    actions_arr[0]=&actions;
    pipe_cfg.attr.name="tbl_rss_1";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_INGRESS;
    pipe_cfg.match=&match;
    pipe_cfg.actions=actions_arr;
    pipe_cfg.attr.nb_actions=1;
    // match.meta.pkt_meta=0xffffffff;
    actions.meta.pkt_meta=flow_get_pipe_id_by_name("ingress_rss_1");
    fwd.type=FLOW_FWD_PIPE;
    fwd.next_pipe=flow_get_pipe("ingress_rss_1");
    fwd_miss.type=FLOW_FWD_RSS;
    flow_create_hw_pipe(&pipe_cfg,&fwd,&fwd_miss,pipe);
    return 0;
}

int init_node_2_0_hw_pipe(struct flow_pipe* pipe)
{
    uint8_t priority;
    struct flow_match match;
    struct flow_actions actions;
    struct flow_fwd fwd;
    {
        struct flow_pipe* next_pipe;
        priority=0;
        memset(&match,0,sizeof(match));
        memset(&actions,0,sizeof(actions));
        memset(&fwd,0,sizeof(fwd));
        match.outer.l3_type=FLOW_L3_TYPE_IP4;
        match.outer.ip4.protocol=0x11;
        actions.meta.pkt_meta=flow_get_pipe_id_by_name("ingress_udp_tbl_0");
        next_pipe=flow_get_pipe("ingress_udp_tbl_0");
        fwd.type=FLOW_FWD_PIPE;
        fwd.next_pipe=next_pipe;
        flow_hw_control_pipe_add_entry(pipe,priority,&match,&actions,&fwd);
    }
    {
        struct flow_pipe* next_pipe;
        priority=1;
        memset(&match,0,sizeof(match));
        memset(&actions,0,sizeof(actions));
        memset(&fwd,0,sizeof(fwd));
        actions.meta.pkt_meta=flow_get_pipe_id_by_name("tbl_rss_1");
        next_pipe=flow_get_pipe("tbl_rss_1");
        fwd.type=FLOW_FWD_PIPE;
        fwd.next_pipe=next_pipe;
        flow_hw_control_pipe_add_entry(pipe,priority,&match,&actions,&fwd);
    }
    return 0;
}
