#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/skbuff.h>

#define MAX_CORES   8
#define MAX_RULES   100
#define MAX_REGEX_LENGTH 256

/* DOCA compatible */
regex_t * compiled_rules[MAX_CORES];
int rule_count[MAX_CORES];

int compile_regex_rules(regex_t * rules) {
    char regex[MAX_REGEX_LENGTH];
    int ret, count = 0;
    FILE *file = fopen("/home/ubuntu/Nutcracker/apps/nf_dns/dns_filter_rules_50.txt", "r");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    while (fgets(regex, MAX_REGEX_LENGTH, file)) {
        if (regex[strlen(regex) - 1] == '\n') {
            regex[strlen(regex) - 1] = '\0';  // Remove newline character
        }

        printf("Regex rule %d: %s\n", count, regex);

        ret = regcomp(&rules[count], regex, REG_EXTENDED);
        if (ret) {
            fprintf(stderr, "Could not compile regex: %s\n", regex);
            continue;
        }
        count++;
        if (count >= MAX_RULES) break;
    }

    fclose(file);

    return count;
}

int load_regex_rules() {
    regex_t * rules;
    int nr_rules;

    for (int i = 0; i < MAX_CORES; i++) {
        rules = (regex_t *)calloc(MAX_RULES, sizeof(regex_t));
        nr_rules = compile_regex_rules(rules);
        compiled_rules[i] = rules;
        rule_count[i] = nr_rules;
    }

    return 0;
}

int find_matching_rule(const char * domain_name) {
    int result;
    regex_t * rules = compiled_rules[sched_getcpu()];
    int count = rule_count[sched_getcpu()];

    // Iterate through all compiled rules
    for (int i = 0; i < count; i++) {
        result = regexec(&rules[i], domain_name, 0, NULL, 0);
        if (result == 0) {
            // printf("Match found with rule %d: %s\n", i, domain_name);
            return i;  // Return the index of the first matching rule
        }
    }

    // printf("No match found for: %s\n", domain_name);
    return -1;  // Return -1 if no match is found
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
    if (ntohs(udphdr->dest) != 53) return NF_MISS;

    return NF_MATCH; // Accept the packet
}

struct dns_header {
    uint16_t id; // Transaction ID
    uint16_t flags; // DNS flags
    uint16_t qdcount; // Number of questions
    uint16_t ancount; // Number of answers
    uint16_t nscount; // Number of authority records
    uint16_t arcount; // Number of additional records
};

// Function to print a domain name from a DNS query
void print_domain_name(const unsigned char * buffer, int* position, unsigned char * domain_name) {
    int len = buffer[(*position)++];
    while (len > 0) {
        for (int i = 0; i < len; i++) {
            *(domain_name++) = buffer[(*position)++];
        }
        len = buffer[(*position)++];
        if (len > 0) {
            *(domain_name++) = '.';
        }
    }
}

// Parse and print details from a DNS query
int parse_dns_query(const unsigned char * buffer, int size) {
    unsigned char domain_name[256] = {0};
    // Cast the buffer to the DNS header struct
    struct dns_header* dns = (struct dns_header*)buffer;
    int position = sizeof(struct dns_header); // Position in the buffer
    print_domain_name(buffer, &position, domain_name);
   if (find_matching_rule(domain_name) < 0) {
       return NF_MISS;
   }
    return NF_MATCH;
}

// Function to be called by hook
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

    return parse_dns_query(data, len);
}

static struct nf_hook_ops nfho = {
    .cond       = check_cond,
    .hook       = hook_func,
    .hooknum    = NF_INET_PRE_ROUTING,
    .pf         = NFPROTO_INET,
};

int nf_dns_init(void) {
    if (load_regex_rules() != 0) {
        return -1;
    }
    nf_register_net_hook(NULL, &nfho);
    return 0;
}