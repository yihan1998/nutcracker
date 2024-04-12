#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_tables.h>
#include <libmnl/libmnl.h>
#include <libnftnl/table.h>
#include <libnftnl/chain.h>
#include <libnftnl/rule.h>
#include <libnftnl/expr.h>

static void add_payload(struct nftnl_rule *r, uint32_t base, uint32_t dreg,
			uint32_t offset, uint32_t len)
{
	struct nftnl_expr *e;

	e = nftnl_expr_alloc("payload");
	if (e == NULL) {
		perror("expr payload oom");
		exit(EXIT_FAILURE);
	}

	nftnl_expr_set_u32(e, NFTNL_EXPR_PAYLOAD_BASE, base);
	nftnl_expr_set_u32(e, NFTNL_EXPR_PAYLOAD_DREG, dreg);
	nftnl_expr_set_u32(e, NFTNL_EXPR_PAYLOAD_OFFSET, offset);
	nftnl_expr_set_u32(e, NFTNL_EXPR_PAYLOAD_LEN, len);

	nftnl_rule_add_expr(r, e);
}

static void add_cmp(struct nftnl_rule *r, uint32_t sreg, uint32_t op,
		    const void *data, uint32_t data_len)
{
	struct nftnl_expr *e;

	e = nftnl_expr_alloc("cmp");
	if (e == NULL) {
		perror("expr cmp oom");
		exit(EXIT_FAILURE);
	}

	nftnl_expr_set_u32(e, NFTNL_EXPR_CMP_SREG, sreg);
	nftnl_expr_set_u32(e, NFTNL_EXPR_CMP_OP, op);
	nftnl_expr_set(e, NFTNL_EXPR_CMP_DATA, data, data_len);

	nftnl_rule_add_expr(r, e);
}

static void add_counter(struct nftnl_rule *r)
{
	struct nftnl_expr *e;

	e = nftnl_expr_alloc("counter");
	if (e == NULL) {
		perror("expr counter oom");
		exit(EXIT_FAILURE);
	}

	nftnl_rule_add_expr(r, e);
}

static struct nftnl_rule *setup_rule() {
	struct nftnl_rule *r = NULL;
	uint8_t proto;
	uint16_t dport;

    r = nftnl_rule_alloc();
	if (r == NULL) {
		perror("OOM");
		exit(EXIT_FAILURE);
	}

    nftnl_rule_set_str(r, NFTNL_RULE_TABLE, "filter");
	nftnl_rule_set_str(r, NFTNL_RULE_CHAIN, "input");
	nftnl_rule_set_u32(r, NFTNL_RULE_FAMILY, AF_INET);

    proto = IPPROTO_TCP;
	add_payload(r, NFT_PAYLOAD_NETWORK_HEADER, NFT_REG_1,
		    offsetof(struct iphdr, protocol), sizeof(uint8_t));
	add_cmp(r, NFT_REG_1, NFT_CMP_EQ, &proto, sizeof(uint8_t));

	dport = htons(22);
	add_payload(r, NFT_PAYLOAD_TRANSPORT_HEADER, NFT_REG_1,
		    offsetof(struct tcphdr, dest), sizeof(uint16_t));
	add_cmp(r, NFT_REG_1, NFT_CMP_EQ, &dport, sizeof(uint16_t));

	add_counter(r);

	return r;
}

int main() {
    struct mnl_socket *nl;
	struct nftnl_rule *r = NULL;
	struct nlmsghdr *nlh;
	struct mnl_nlmsg_batch *batch;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    unsigned int seq = 0;
	int ret;

    r = setup_rule();

    nl = mnl_socket_open(NETLINK_NETFILTER);
	if (nl == NULL) {
		perror("mnl_socket_open");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		perror("mnl_socket_bind");
		exit(EXIT_FAILURE);
	}

	batch = mnl_nlmsg_batch_start(buf, sizeof(buf));

	nftnl_batch_begin(mnl_nlmsg_batch_current(batch), seq++);
	mnl_nlmsg_batch_next(batch);

	nlh = nftnl_rule_nlmsg_build_hdr(mnl_nlmsg_batch_current(batch),
			NFT_MSG_NEWRULE,
			nftnl_rule_get_u32(r, NFTNL_RULE_FAMILY),
			NLM_F_APPEND|NLM_F_CREATE|NLM_F_ACK, seq++);

	nftnl_rule_nlmsg_build_payload(nlh, r);
	nftnl_rule_free(r);
	mnl_nlmsg_batch_next(batch);

	nftnl_batch_end(mnl_nlmsg_batch_current(batch), seq++);
	mnl_nlmsg_batch_next(batch);

	ret = mnl_socket_sendto(nl, mnl_nlmsg_batch_head(batch),
				mnl_nlmsg_batch_size(batch));
	if (ret == -1) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	mnl_nlmsg_batch_stop(batch);

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		exit(EXIT_FAILURE);
	}

	ret = mnl_cb_run(buf, ret, 0, mnl_socket_get_portid(nl), NULL, NULL);
	if (ret < 0) {
		perror("mnl_cb_run failed");
		exit(EXIT_FAILURE);
	}

	mnl_socket_close(nl);

    printf("DNS filter rule has been set up.\n");

    return 0;
}