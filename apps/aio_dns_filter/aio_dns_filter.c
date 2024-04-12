#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef __GNUC__
#define __GNUC__
#endif

#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__  __ORDER_LITTLE_ENDIAN__
#endif

#include <libaio.h>

#define PORT 53
#define BUFFER_SIZE 512

// struct aio_dns_filter_ctx {
//     int sockfd;
//     int len;
//     char data[BUFFER_SIZE];
// };

// struct io_context {
//     int a;
// };

// struct iocb {
//     int a;
// };

// typedef struct io_context * io_context_t;

// int io_setup(int nr_event, io_context_t ctx) {
//     printf("test\n");
//     return syscall(206);
// }

struct aio_dns_filter_ctx {
    int sockfd;
};

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
void parse_dns_query(const unsigned char* buffer, int size) {
    unsigned char domain_name[256] = {0};
    if (size < sizeof(struct dns_header)) {
        printf("Buffer too small for DNS header\n");
        return;
    }

    // Cast the buffer to the DNS header struct
    struct dns_header* dns = (struct dns_header*)buffer;

    // Print basic header information
    // printf("Transaction ID: 0x%X\n", ntohs(dns->id));
    // printf("Flags: 0x%X\n", ntohs(dns->flags));
    // printf("Questions: %d\n", ntohs(dns->qdcount));

    // Assuming there's at least one question, parse the first question
    int position = sizeof(struct dns_header); // Position in the buffer

    print_domain_name(buffer, &position, domain_name);
    // printf("Domain: %s\n", domain_name);

    // Assuming the question format is TYPE (2 bytes) then CLASS (2 bytes)
    // if (position + 4 <= size) {
    //     unsigned short qtype = ntohs(*(unsigned short*)(buffer + position));
    //     position += 2;
    //     unsigned short qclass = ntohs(*(unsigned short*)(buffer + position));
    //     position += 2;

    //     printf("Query Type: %d\n", qtype);
    //     printf("Query Class: %d\n", qclass);
    // }
}

void aio_dns_filter_fn(void * argp) {
    struct aio_dns_filter_ctx * ctx;
    uint8_t buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    ctx = (struct aio_dns_filter_ctx *)argp;

    ssize_t n = recvfrom(ctx->sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        return;
    }

    // parse_dns_query(buffer, n);

    sendto(ctx->sockfd, (const char *)buffer, n, 0, (const struct sockaddr *)&cliaddr, len);

    return;
}

int aio_dns_filter_init(void) {
    io_context_t ctx = 0;
    struct iocb cb, * cbs[1];
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    int ret;
    struct aio_dns_filter_ctx arg = {0};

    if (io_setup(16, &ctx) != 0) {
        perror("io_setup");
        exit(1);
    }

    // Create a socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    arg.sockfd = sockfd;

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Setup I/O control block
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = sockfd;
    cb.aio_lio_opcode = IO_CMD_PREAD;
    cb.data = (void *)aio_dns_filter_fn;
    cb.u.c.buf = (void *)&arg;
    cbs[0] = &cb;

    // Submit the AIO request
    ret = io_submit(ctx, 1, cbs);
    if (ret != 1) {
        if (ret < 0)
            perror("io_submit");
        else
            fprintf(stderr, "Could not submit IOs\n");
        exit(1);
    }

    sleep(20);

    printf("AIO write completed successfully.\n");
#if 0
    // Cleanup
    io_destroy(ctx);
    close(sockfd);
#endif
    return 0;
}