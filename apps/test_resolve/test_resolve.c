#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>

int test_resolve_init() {
    // Example DNS response packet (This should be obtained via network in real applications)
    unsigned char response[] = {
        0x12, 0x34, // Transaction ID
        0x81, 0x80, // Flags
        0x00, 0x01, // Questions
        0x00, 0x01, // Answer RRs
        0x00, 0x00, // Authority RRs
        0x00, 0x00, // Additional RRs
        // Question Section
        0x03, 'w', 'w', 'w',
        0x06, 'g', 'o', 'o', 'g', 'l', 'e',
        0x03, 'c', 'o', 'm',
        0x00, // End of string
        0x00, 0x01, // Type: A
        0x00, 0x01, // Class: IN
        // Answer Section
        0xc0, 0x0c, // Name (pointer to www.google.com)
        0x00, 0x01, // Type: A
        0x00, 0x01, // Class: IN
        0x00, 0x00, 0x00, 0x3c, // TTL
        0x00, 0x04, // Data length
        0xac, 0xd9, 0x1a, 0x80 // Address: 172.217.26.128
    };
    int response_length = sizeof(response);

    // Initialize ns_msg structure for parsing the DNS response
    ns_msg handle;
    if (ns_initparse(response, response_length, &handle) < 0) {
        fprintf(stderr, "Failed to initialize DNS parse\n");
        return 1;
    }

    // Print general information about the DNS message
    printf("Number of questions: %d\n", ns_msg_count(handle, ns_s_qd));
    printf("Number of answers: %d\n", ns_msg_count(handle, ns_s_an));

    // Parse and print details of each answer section
    ns_rr rr;
    for (int i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
        if (ns_parserr(&handle, ns_s_an, i, &rr) == 0) {
            printf("Answer %d: %s - %s\n", i + 1, ns_rr_name(rr), inet_ntoa(*(struct in_addr *)ns_rr_rdata(rr)));
        }
    }

    return 0;
}
