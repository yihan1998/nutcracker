#ifndef _NETIF_H_
#define _NETIF_H_

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "err.h"
#include "init.h"
#include "percpu.h"
#include "list.h"

extern int __init dpdk_init(void);

#endif  /* _NETIF_H_ */