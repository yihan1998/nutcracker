#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/skbuff.h>

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#define NR_CORE 8

EVP_MD_CTX * mdctx[NR_CORE];

struct sha256pkt {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t send_start;
    uint64_t completion_time;
    uint32_t len;
    uint8_t data[0];
};

void handleErrors(void) {
    ERR_print_errors_fp(stderr);
    abort();
}

int create_mdctx(void) {
    for (int i = 0; i < NR_CORE; i++) {
        if((mdctx[i] = EVP_MD_CTX_new()) == NULL) {
            printf("Failed to init EVP context for core %d\n", i);
        }
    }

    return 0;
}

void calculate_sha256(EVP_MD_CTX *mdctx, const unsigned char *data, size_t data_len, unsigned char *hash) {
    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
            handleErrors();
        }

    if(1 != EVP_DigestUpdate(mdctx, data, data_len)) {
        handleErrors();
    }

    unsigned int hash_len;
    if(1 != EVP_DigestFinal_ex(mdctx, hash, &hash_len)) {
        handleErrors();
    }
}

int verify_pkt(uint8_t * data, int len) {
    struct sha256pkt * request;
    unsigned char hash[EVP_MAX_MD_SIZE];
    int lcore_id = sched_getcpu();

    request = (struct sha256pkt *)data;

    calculate_sha256(mdctx[lcore_id], request->data, request->len, hash);

    // // Print the hash for the current chunk
    // printf("Calculated chunk hash: ");
    // for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    //     printf("%02x", hash[i]);
    // }
    // printf("\n");

    // // Print the hash for the current chunk
    // printf("Received chunk hash: ");
    // for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    //     printf("%02x", request->hash[i]);
    // }
    // printf("\n");

    if (memcmp(hash, request->hash, SHA256_DIGEST_LENGTH) != 0) {
        printf("Verification failed\n");
        return NF_MISS;
    }

    return NF_MATCH;
}

static unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct ethhdr * ethhdr;
    struct iphdr * iphdr;
    uint16_t iphdr_hlen;
    uint8_t iphdr_protocol;
    struct udphdr * udphdr;
	uint16_t ulen, len;
	uint8_t * data;

    ethhdr = (struct ethhdr *)skb->ptr;

    iphdr = (struct iphdr *)&ethhdr[1];
    iphdr_hlen = iphdr->ihl;
    iphdr_hlen <<= 2;

    udphdr = (struct udphdr *)((uint8_t *)iphdr + iphdr_hlen);
    ulen = ntohs(udphdr->len);
	len = ulen - sizeof(struct udphdr);
	data = (uint8_t *)udphdr + sizeof(struct udphdr);

    return verify_pkt(data, len);
}

static unsigned int check_cond(struct sk_buff *skb) {
    struct ethhdr * ethhdr;
    struct iphdr * iphdr;
    uint16_t iphdr_hlen;
    uint8_t iphdr_protocol;
    struct udphdr * udphdr;

    ethhdr = (struct ethhdr *)skb->ptr;

    iphdr = (struct iphdr *)&ethhdr[1];
    iphdr_hlen = iphdr->ihl;
    iphdr_hlen <<= 2;
    iphdr_protocol = iphdr->protocol;

    if (iphdr_protocol != IPPROTO_UDP) return NF_MISS;

    udphdr = (struct udphdr *)((uint8_t *)iphdr + iphdr_hlen);
    if (ntohs(udphdr->dest) != 4321) return NF_MISS;

    return NF_MATCH; // Accept the packet
}

static struct nf_hook_ops nfho = {
    .cond       = check_cond,
    .hook       = hook_func,
    .hooknum    = NF_INET_PRE_ROUTING,
    .pf         = NFPROTO_INET,
};

int entrypoint(void) {
    if (create_mdctx() != 0) {
        return -1;
    }
    nf_register_net_hook(NULL, &nfho);
    return 0;
}
