#include "net/sock.h"

/*
 *	Get a socket option on an socket.
 */
int sock_common_getsockopt(struct socket * sock, int level, int optname, char * optval, int * optlen) {
	return 0;
}

int sock_common_recvmsg(struct socket * sock, struct msghdr *msg, size_t size, int flags) {
    return 0;
}

/*
 *	Set socket options on an inet socket.
 */
int sock_common_setsockopt(struct socket * sock, int level, int optname, char * optval, unsigned int optlen) {
    return 0;
}
