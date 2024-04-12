#ifndef _AIO_H_
#define _AIO_H_

#include "iocontext.h"

#include <stdint.h>

typedef struct io_context * io_context_t;
#if 0
typedef enum io_iocb_cmd {
	IO_CMD_PREAD = 0,
	IO_CMD_PWRITE = 1,

	IO_CMD_FSYNC = 2,
	IO_CMD_FDSYNC = 3,

	IO_CMD_POLL = 5, /* Never implemented in mainline, see io_prep_poll */
	IO_CMD_NOOP = 6,
	IO_CMD_PREADV = 7,
	IO_CMD_PWRITEV = 8,
} io_iocb_cmd_t;

struct iocb;

struct io_iocb_poll {
	int events;
};	/* result code is the set of result flags or -'ve errno */

struct io_iocb_sockaddr {
	struct sockaddr * addr;
	int		len;
};	/* result code is the length of the sockaddr, or -'ve errno */

struct io_iocb_common {
	void		* buf;
	long long	offset;
	long long	__pad3;
	unsigned	flags;
	unsigned	resfd;
};	/* result code is the amount read or -'ve errno */

struct io_iocb_vector {
	const struct iovec	*vec;
	int			nr;
	long long		offset;
};	/* result code is the amount read or -'ve errno */

struct iocb {
	void * data;

	short	aio_lio_opcode;	
	short	aio_reqprio;
	int		aio_fildes;

	union {
		struct io_iocb_common		c;
		struct io_iocb_vector		v;
		struct io_iocb_poll			poll;
		struct io_iocb_sockaddr	saddr;
	} u;
};
#endif
#include </usr/include/libaio.h>

#endif  /* _AIO_H_ */