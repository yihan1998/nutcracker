#include <netinet/in.h>

#include "opt.h"
#include "printk.h"
#include "net/netfmt.h"
#include "net/net.h"
#include "net/udp.h"
#include "net/sock.h"
#include "net/flow.h"

int udp_init_sock(struct sock * sk) {
	return 0;
}

int udp_bind(struct sock * sk, const struct sockaddr * uaddr, int addr_len) {
	struct sockaddr_in *addr = (struct sockaddr_in *)uaddr;
	struct sock * try = NULL;
	unsigned short snum;
	struct flowi4 key = {0};
	int ret;

	snum = ntohs(addr->sin_port);

	pr_debug(SOCKET_DEBUG, "%s: lookup if this port is already taken\n", __func__);

	/* Lookup if this port is already taken */
	key.daddr = 0;
	key.saddr = addr->sin_addr.s_addr;
	key.fl4_dport = 0;
	key.fl4_sport = htons(snum);

	ret = rte_hash_lookup_data(udp_table, (const void *)&key, (void **)&try);
	if (ret < 0) {
		pr_err("Error happened in hash table!(err: %d)", ret);
		return -1;
	}

	pr_debug(SOCKET_DEBUG, "%s: port is not take!\n", __func__);

	sk->sk_daddr = 0;
	sk->sk_rcv_saddr = addr->sin_addr.s_addr;
	sk->sk_dport = 0;
	sk->sk_num = htons(snum);

    pr_debug(SOCKET_DEBUG, "%s: sock: %p(src ip: " IP_STRING ", dst ip: " IP_STRING " src port: %u, dst port: %u)\n", 
			__func__, sk, NET_IP_FMT(sk->sk_rcv_saddr), NET_IP_FMT(sk->sk_daddr), ntohs(sk->sk_num), ntohs(sk->sk_dport));

	ret = rte_hash_add_key_data(udp_table, (const void *)&key, (void *)sk);
	if (ret < 0) {
		pr_err("Failed to add key (pos0=%d)", ret);
		return -1;
	}

	return ret;
}

struct proto udp_prot = {
	.name	= "UDP",
	.init	= udp_init_sock,
	.bind	= udp_bind,
};
