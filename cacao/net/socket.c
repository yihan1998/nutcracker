#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "opt.h"
#include "printk.h"
#include "syscall.h"
#include "fs/file.h"
#include "kernel/sched.h"
#include "kernel/util.h"
#include "net/net.h"
#include "syscall.h"

#include <rte_mempool.h>

static const struct net_proto_family * net_families[NR_PROTO];

static int sock_close(struct file * file) {
    return 0;
}

static ssize_t sock_read(struct file * file, char * buf, size_t len, loff_t * pos) {
    return 0;
}

static ssize_t sock_write(struct file * file, const char * buf, size_t len, loff_t * pos) {
    return 0;
}

static const struct file_operations socket_file_ops = {  
	.release = sock_close,
    .read   = sock_read,
    .write  = sock_write,
};

static struct socket * sock_from_file(struct file * file, int * err) {
    return file->f_priv_data.ptr;
}

int sock_register(const struct net_proto_family * ops) {
	int err;

	if (ops->family >= NR_PROTO) {
		pr_err("protocol %d >= NR_PROTO(%d)\n", ops->family, NR_PROTO);
		return -ENOBUFS;
	}
	
    net_families[ops->family] = ops;
    err = 0;

	pr_info("Registered protocol family %d\n", ops->family);

	return err;
}

struct file * sock_alloc_file(struct socket * sock, int flags, const char * path) {
    struct file * file;

    file = alloc_file(flags, &socket_file_ops);
    if (!file) {
        pr_err("Failed to allocate new file!\n");
        return file;
    }

    if (path) {
        memcpy(file->f_path, path, strlen(path));
    }

    sock->file = file;
    file->f_priv_data.ptr = sock;

    return file;
}

static int sock_map_fd(struct socket * sock, int flags) {
    struct file * newfile;
	int fd = get_unused_fd_flags(flags);
	if (unlikely(fd < 0)) {
		// sock_release(sock);
        errno = -fd;
	    return -1;
	}

    newfile = sock_alloc_file(sock, flags, NULL);
    if (!newfile) {
        errno = EMFILE;
	    return -1;
    }

    /* Install fd into fd table */
    fd_install(fd, newfile);

    return fd;
}

struct socket * sockfd_lookup(int fd, int * err) {
	struct file * file = fget(fd);
	struct socket * sock;

	*err = -EBADF;
	if (file) {
		sock = sock_from_file(file, err);
        if (sock) {
			return sock;
		}
	}

	return NULL;
}

void sock_release(struct socket * sock) {
	// __sock_release(sock);
}

int sock_create(int domain, int type, int protocol, struct socket ** res) {
    int err;
    const struct net_proto_family * pf;
    struct socket * sock;

	rte_mempool_get(socket_mp, (void *)&sock);
    if (!sock) {
        return -EMFILE;
    }

    sock->type = type;

	pf = net_families[domain];
	err = -EAFNOSUPPORT;
    if (!pf) {
		pr_warn("no net protocol for domain %d\n", domain);
        errno = EINVAL;
        goto out_release;
    }

    err = pf->create(sock, protocol);
	if (err < 0) {
		pr_err("fail to create socket with domain %d\n", domain);
        goto out_release;
    }

    *res = sock;

    return 0;

out_release:
    sock_release(sock);
    return err;
}

/* -------------------------------------------------------------------------- */
static int __do_socket(int domain, int type, int protocol) {
    pr_debug(SOCKET_DEBUG, "%s: domain: %d, type: %x, protocol: %d\n", __func__, domain, type, protocol);
	int retval;
	struct socket *sock;
    int flags;

	flags = type & ~SOCK_TYPE_MASK;
	if (flags & ~(SOCK_CLOEXEC | SOCK_NONBLOCK)) {
		return -EINVAL;
    }

	type &= SOCK_TYPE_MASK;

    if (SOCK_NONBLOCK != O_NONBLOCK && (flags & SOCK_NONBLOCK)) {
		flags = (flags & ~SOCK_NONBLOCK) | O_NONBLOCK;
    }

    retval = sock_create(domain, type, protocol, &sock);
	if (retval < 0) {
		return retval;
    }

	return sock_map_fd(sock, flags & (O_CLOEXEC | O_NONBLOCK));
}

int socket(int domain, int type, int protocol) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_socket(domain, type, protocol);
    }

    pr_debug(SOCKET_DEBUG, "%s: domain: %d, type: %x, protocol: %d\n", __func__, domain, type, protocol);
    return __do_socket(domain, type, protocol);
}

/* -------------------------------------------------------------------------- */
static int __do_bind(int fd, const struct sockaddr * addr, int addrlen) {
    int err;
    struct socket * sock;
    sock = sockfd_lookup(fd, &err);
    if (!sock) {
        errno = EINVAL;
        return -1;
    }

    err = sock->ops->bind(sock, addr, addrlen);

    return err;
}

int bind(int fd, const struct sockaddr * addr, socklen_t addrlen) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_bind(fd, addr, addrlen);
    }

    pr_debug(SOCKET_DEBUG, "%s: fd: %d, addr: %p, addrlen: %d\n", __func__, fd, addr, addrlen);
    return __do_bind(fd, addr, addrlen);
}

/* -------------------------------------------------------------------------- */
int connect(int fd, const struct sockaddr * addr, socklen_t addrlen) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_connect(fd, addr, addrlen);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int accept(int fd, struct sockaddr * addr, socklen_t * addrlen) {
    int flags = 0;

    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_accept4(fd, addr, addrlen, flags);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int accept4(int fd, struct sockaddr * addr, socklen_t * addrlen, int flags) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_accept4(fd, addr, addrlen, flags);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int listen(int fd, int backlog) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_listen(fd, backlog);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int getpeername(int fd, struct sockaddr * usockaddr, socklen_t * usockaddr_len) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_getpeername(fd, usockaddr, usockaddr_len);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
ssize_t sendmsg(int fd, const struct msghdr * msg, int flags) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_sendmsg(fd, msg, flags);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
ssize_t sendto(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_sendto(fd, buf, len, flags, addr, addrlen);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
ssize_t send(int fd, const void * buf, size_t len, int flags) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_send(fd, buf, len, flags);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
ssize_t recvmsg(int fd, struct msghdr * msg, int flags) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_recvmsg(fd, msg, flags);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static ssize_t __do_recvfrom(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) {
    int err;
    struct socket * sock;
    struct msghdr msg = {0};
    struct iovec iov = {.iov_base = buf, .iov_len = len};

    sock = sockfd_lookup(fd, &err);
    if (!sock) {
        pr_err("fail to lookup fd %d in fd table\n", fd);
        return -1;
    }

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (sock->flags & O_NONBLOCK) {
		flags |= MSG_DONTWAIT;
    }

    msg.msg_name = (addr) ? addr : NULL;
	msg.msg_flags = flags;

    err = sock->ops->recvmsg(sock, &msg, len, flags);
    if (err >= 0 && addr != NULL) {
        *addrlen = msg.msg_namelen;
    }

    return err;
}

ssize_t recvfrom(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_recvfrom(fd, buf, len, flags, addr, addrlen);
    }

    return __do_recvfrom(fd, buf, len, flags, addr, addrlen);
}

/* -------------------------------------------------------------------------- */
ssize_t recv(int fd, void * buf, size_t len, int flags) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_recv(fd, buf, len, flags);
    }

    return 0;
}
