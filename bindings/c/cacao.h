#ifndef _CACAO_H_
#define _CACAO_H_

typedef struct {
    int sockfd;
} Context;

// ssize_t cacao_recv(int sockfd, void *buf, size_t len, int flags);

// ssize_t cacao_recvfrom(int sockfd, void *buf, size_t len, int flags,
//                 struct sockaddr *src_addr, socklen_t *addrlen);

// ssize_t cacao_recvmsg(int sockfd, struct msghdr *msg, int flags);

#endif  /* _CACAO_H_ */