#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "opt.h"
#include "printk.h"
#include "syscall.h"
#include "fs/fs.h"
#include "fs/file.h"
#include "kernel/sched.h"
#include "kernel/poll.h"
#include "net/net.h"
#include "net/flow.h"

static const struct net_proto_family * net_families[NR_PROTO];

static inline struct socket * sock_from_file(struct file * file, int * err) {
    return (struct socket *)(file->f_priv_data.ptr);
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

static int sock_close(struct file * file) {
    return 0;
}

static __poll_t sock_poll(struct file * file, poll_table * wait) {
    int err = -EINVAL;
    struct socket * sock = sock_from_file(file, &err);
    return sock->ops->poll(file, sock, wait);
}

static ssize_t sock_read(struct file * file, char * buf, size_t len, loff_t * pos) {
    return 0;
}

static ssize_t sock_write(struct file * file, const char * buf, size_t len, loff_t * pos) {
    return 0;
}

static const struct file_operations socket_file_ops = {  
	.release    = sock_close,
    .poll       = sock_poll,
    .read       = sock_read,
    .write      = sock_write,
};

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

    sock->wq.flags = 0;
    init_waitqueue_head(&sock->wq.wait);

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
    if (fd < 0) {
        return fd;
    }

    newfile = sock_alloc_file(sock, flags, NULL);
    if (!newfile) {
        return -EMFILE;
    }

    /* Install fd into fd table */
    fd_install(fd, newfile);

    return fd;
}

/* -------------------------------------------------------------------------- */
static int __socket(int domain, int type, int protocol) {
	int retval;
    int flags;
    struct socket * sock;

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
        pr_err("SOCKET: sock_create return %d\n", retval);
        errno = -retval;
		return -1;
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

    pr_debug(SOCKET_DEBUG, "%s: domain: %d, type: %d, protocol: %d\n", __func__, domain, type, protocol);
    return __socket(domain, type, protocol);
}

/* -------------------------------------------------------------------------- */
static int __bind(int fd, const struct sockaddr * addr, int addrlen) {
    int err;
    struct socket * sock;
    sock = sockfd_lookup(fd, &err);
    if (!sock) {
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
    return __bind(fd, addr, addrlen);
}

/* -------------------------------------------------------------------------- */
static ssize_t __sendto(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) {
    int err;
    struct socket * sock;
    sock = sockfd_lookup(fd, &err);
    if (!sock) {
        pr_err( "fail to lookup fd %d in fd table\n", fd);
        return -1;
    }

    struct msghdr msg = {0};

    struct iovec iov = {.iov_base = (void *)buf, .iov_len = len};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (addr) {
        msg.msg_name = (void *)addr;
		msg.msg_namelen = addrlen;
    } else {
        msg.msg_name = NULL;
		msg.msg_namelen = 0;
    }
    
    if (sock->file->f_flags & O_NONBLOCK) {
		flags |= MSG_DONTWAIT;
    }

    msg.msg_flags = flags;

    err = sock->ops->sendmsg(sock, &msg, msg.msg_iov->iov_len);
    return err;
}

ssize_t sendto(int fd, const void * buf, size_t len, int flags, const struct sockaddr * addr, socklen_t addrlen) {
    if (unlikely(!syscall_hooked)) {
        syscall_hook_init();
    }

    if (unlikely(kernel_early_boot)) {
        return libc_sendto(fd, buf, len, flags, addr, addrlen);
    }

    pr_debug(SOCKET_DEBUG, "%s: fd: %d, buf: %p, len: %lu, flags: %d\n", __func__, fd, buf, len, flags);
    return __sendto(fd, buf, len, flags, addr, addrlen);
}

/* -------------------------------------------------------------------------- */
static ssize_t __recvfrom(int fd, void * buf, size_t len, int flags, struct sockaddr * addr, socklen_t * addrlen) {
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

    if (sock->file->f_flags & O_NONBLOCK) {
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
    
    pr_debug(SOCKET_DEBUG, "%s: fd: %d, buf: %p, len: %lu, flags: %d\n", __func__, fd, buf, len, flags);
    return __recvfrom(fd, buf, len, flags, addr, addrlen);
}