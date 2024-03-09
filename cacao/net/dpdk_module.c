#include <fcntl.h>
#include <ifaddrs.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <rte_common.h>
#include <rte_eal.h>

#include "printk.h"
#include "net/netif.h"

int __init dpdk_init(void) {
    int ret;

    int argc = 9;
    char * argv[16] = { "",
                        "-l", "0",
                        "-n", "4",
                        "-a", "4b:00.0",
                        "--proc-type=auto",
                        ""};

    if ((ret = rte_eal_init(argc, argv)) < 0) {
        pr_emerg(" rte_eal_init() failed! ret: %d", ret);
        return -1;
    }

    return 0;
}
