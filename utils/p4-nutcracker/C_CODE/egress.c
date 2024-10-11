#include "egress.h"

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

struct sk_buff;

struct vxlanhdr {
    uint8_t  flags;
    uint8_t  reserved1[3];
    uint32_t vni_reserved2;
} __attribute__((__packed__));

struct table_entry {
    struct flow_pipe *action;
    struct flow_pipe *next;
} __attribute__((packed));

extern uint8_t* get_packet(struct sk_buff* skb);
extern uint32_t get_packet_size(struct sk_buff* skb);
extern struct sk_buff* prepend_packet(struct sk_buff* skb, uint32_t prepend_size);
extern struct flow_pipe* flow_get_pipe(const char* pipe_name);
extern int create_fsm_pipe(struct flow_pipe_cfg* pipe_cfg, struct flow_fwd *fwd, struct flow_pipe* pipe);
extern struct flow_pipe* fsm_table_lookup(struct flow_pipe* pipe, struct flow_match* match, struct sk_buff* skb);
extern int fsm_table_add_entry(struct flow_pipe* pipe, struct flow_match* match, struct flow_actions* action, struct flow_fwd* fwd);
int create_egress_vxlan_encap_tbl_2_fsm_pipe(struct flow_pipe* pipe);
int create_egress_encap_3_fsm_pipe(struct flow_pipe* pipe);
struct flow_pipe* run_egress_vxlan_encap_tbl_2_fsm_pipe(struct flow_pipe* pipe, struct sk_buff* skb);
struct flow_pipe* run_egress_encap_3_fsm_pipe(struct flow_pipe* pipe, struct sk_buff* skb, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni);
int add_egress_encap_3_fsm_pipe_entry(struct flow_pipe* pipe, const char* next, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni);
int add_egress_vxlan_encap_tbl_2_fsm_pipe_entry(struct flow_pipe* pipe, const char* next, uint16_t dest);

int add_egress_encap_3_fsm_pipe_entry(struct flow_pipe* pipe, const char* next, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni)
{
    struct flow_match match;
    struct flow_actions actions;
    struct flow_fwd fwd;
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    SET_MAC_ADDR(actions.outer.eth.h_dest,h_dest[0],h_dest[1],h_dest[2],h_dest[3],h_dest[4],h_dest[5]);
    SET_MAC_ADDR(actions.outer.eth.h_source,h_source[0],h_source[1],h_source[2],h_source[3],h_source[4],h_source[5]);
    actions.outer.ip4.saddr=saddr;
    actions.outer.ip4.daddr=daddr;
    actions.outer.udp.source=source;
    actions.outer.udp.dest=dest;
    actions.encap.tun.vxlan_tun_id=vni;
    fwd.type=FLOW_FWD_PIPE;
    fwd.next_pipe=flow_get_pipe(next);
    return fsm_table_add_entry(pipe,&match,&actions,&fwd);
}

int add_egress_vxlan_encap_tbl_2_fsm_pipe_entry(struct flow_pipe* pipe, const char* next, uint16_t dest)
{
    struct flow_match match;
    struct flow_actions actions;
    struct flow_fwd fwd;
    memset(&match,0,sizeof(match));
    memset(&actions,0,sizeof(actions));
    memset(&fwd,0,sizeof(fwd));
    match.outer.udp.dest=ntohs(dest);
    fwd.type=FLOW_FWD_PIPE;
    fwd.next_pipe=flow_get_pipe("egress_encap_3");
    return fsm_table_add_entry(pipe,&match,&actions,&fwd);
}

int create_egress_vxlan_encap_tbl_2_fsm_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    pipe_cfg.attr.name="egress_vxlan_encap_tbl_2";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=true;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_EGRESS;
    pipe_cfg.match=&match;
    match.outer.udp.dest=0xffff;
    return create_fsm_pipe(&pipe_cfg,0,pipe);
}

int create_egress_encap_3_fsm_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    pipe_cfg.attr.name="egress_encap_3";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_EGRESS;
    pipe_cfg.match=&match;
    return create_fsm_pipe(&pipe_cfg,0,pipe);  
}

int create_egress_fwd_port_4_fsm_pipe(struct flow_pipe* pipe)
{
    struct flow_pipe_cfg pipe_cfg;
    struct flow_match match;
    struct flow_fwd fwd;
    memset(&pipe_cfg,0,sizeof(pipe_cfg));
    memset(&match,0,sizeof(match));
    memset(&fwd,0,sizeof(fwd));
    pipe_cfg.attr.name="egress_fwd_port_4";
    pipe_cfg.attr.type=FLOW_PIPE_BASIC;
    pipe_cfg.attr.is_root=false;
    pipe_cfg.attr.domain=FLOW_PIPE_DOMAIN_EGRESS;
    pipe_cfg.match=&match;
    fwd.type=FLOW_FWD_PORT;
    return create_fsm_pipe(&pipe_cfg,&fwd,pipe);
}

struct flow_pipe* run_egress_encap_3_fsm_pipe(struct flow_pipe* pipe, struct sk_buff* skb, uint8_t h_dest[6], uint8_t h_source[6], uint32_t saddr, uint32_t daddr, uint16_t source, uint16_t dest, uint32_t vni)
{
    uint32_t pkt_size=get_packet_size(skb);
    uint32_t new_hdr_size=sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(struct vxlanhdr);
    int new_pkt_size=pkt_size+new_hdr_size;
    struct sk_buff *new_pkt=prepend_packet(skb,new_hdr_size);
    uint8_t *p=get_packet(new_pkt);
    struct ethhdr *new_eth_hdr = (struct ethhdr *)p;
    struct iphdr *new_ip_hdr = (struct iphdr *)&new_eth_hdr[1];
    struct udphdr *new_udp_hdr = (struct udphdr *)&new_ip_hdr[1];
    struct vxlanhdr *vxlan_hdr = (struct vxlanhdr*)&new_udp_hdr[1];
    new_eth_hdr->h_proto=htons(ETH_P_IP);
    SET_MAC_ADDR(new_eth_hdr->h_source,h_source[0],h_source[1],h_source[2],h_source[3],h_source[4],h_source[5]);
    SET_MAC_ADDR(new_eth_hdr->h_dest,h_dest[0],h_dest[1],h_dest[2],h_dest[3],h_dest[4],h_dest[5]);
    new_ip_hdr->version=4;
    new_ip_hdr->ihl=5;
    new_ip_hdr->tot_len=htons(pkt_size+sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(struct vxlanhdr));
    new_ip_hdr->protocol=IPPROTO_UDP;
    new_ip_hdr->saddr=saddr;
    new_ip_hdr->daddr=daddr;
    new_udp_hdr->source=htons(source);
    new_udp_hdr->dest=htons(dest);
    new_udp_hdr->len=htons(pkt_size+sizeof(struct udphdr)+sizeof(struct vxlanhdr));
    vxlan_hdr->flags=htonl(0x08000000);
    vxlan_hdr->vni_reserved2=htonl(0x123456);
    return 0;
}