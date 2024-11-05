#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

int find_matching_rule(char * domain_name) {
	const char * regex_rules[] = {
		"^.*.google.com$",
		"^.*.facebook.com$",
		"^.*.youtube.com$",
		"^.*.amazon.com$",
		"^.*.twitter.com$"
	};

    for (int i = 0; i < (int)(sizeof(regex_rules) / sizeof(regex_rules[0])); i++) {
		if (match(domain_name, regex_rules[i])) return 1;
	}
	// flexio_dev_print("Failed to find a match for %s\n", domain_name);

    return 0;
}

struct dns_header {
    unsigned short id; // Transaction ID
    unsigned short flags; // DNS flags
    unsigned short qdcount; // Number of questions
    unsigned short ancount; // Number of answers
    unsigned short nscount; // Number of authority records
    unsigned short arcount; // Number of additional records
};

// Convert a 16-bit number from host to network byte order
unsigned short htons(unsigned short hostshort) {
    return (hostshort << 8) | (hostshort >> 8);
}

// Convert a 16-bit number from network to host byte order
unsigned short ntohs(unsigned short netshort) {
    return htons(netshort);  // Same operation as my_htons
}

// Function to match text with pattern
int match(char *text, const char *pattern);

// Helper function to match from current position
int match_here(char *text, const char *pattern);

// Helper function to match character with possible *
int match_star(char c, char *text, const char *pattern);

int match(char *text, const char *pattern) {
    if (pattern[0] == '^') {
        return match_here(text, pattern + 1);
    }
    do {
        if (match_here(text, pattern)) {
            return 1;
        }
    } while (*text++ != '\0');
    return 0;
}

int match_here(char *text, const char *pattern) {
    if (pattern[0] == '\0') {
        return 1;
    }
    if (pattern[1] == '*') {
        return match_star(pattern[0], text, pattern + 2);
    }
    if (pattern[1] == '?') {
        return (match_here(text, pattern + 2) || 
               (*text != '\0' && (pattern[0] == '.' || pattern[0] == *text) && match_here(text + 1, pattern + 2)));
    }
    if (pattern[0] == '$' && pattern[1] == '\0') {
        return *text == '\0';
    }
    if (*text != '\0' && (pattern[0] == '.' || pattern[0] == *text)) {
        return match_here(text + 1, pattern + 1);
    }
    return 0;
}

int match_star(char c, char *text, const char *pattern) {
    do {
        if (match_here(text, pattern)) {
            return 1;
        }
    } while (*text != '\0' && (*text++ == c || c == '.'));
    return 0;
}

// Function to print a domain name from a DNS query
void print_domain_name(unsigned char * buffer, int * position, char * domain_name) {
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

int parse_dns_query(unsigned char * buffer) {
    char domain_name[256] = {0};
    int position = sizeof(struct dns_header); // Position in the buffer
    print_domain_name(buffer, &position, domain_name);
	return find_matching_rule(domain_name);
}

int do_dns_filter(uint8_t * packet) {
    struct ethhdr *ethhdr = (struct ethhdr *) packet;
    struct iphdr *iphdr = (struct iphdr *)&ethhdr[1];
    struct udphdr *udphdr = (struct udphdr *)&iphdr[1];
    uint8_t *payload = (uint8_t *)&udphdr[1];
    
    return 0;
}