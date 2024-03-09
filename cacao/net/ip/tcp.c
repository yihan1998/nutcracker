#include "opt.h"
#include "printk.h"
#include "net/tcp.h"
#include "net/sock.h"

struct proto tcp_prot = {
	.name	    = "TCP",
};
