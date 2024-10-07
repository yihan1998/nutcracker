#ifndef _NETSTATS_H_
#define _NETSTATS_H_

#include <atomic>

struct NetStats {
    std::atomic<bool> running{true};
    std::atomic<int> secRecv{0};
    std::atomic<int> secSend{0};
    std::atomic<int> totalRecv{0};
    std::atomic<int> totalSend{0};
    std::atomic<double> last_sec_recv_rate{0.0};
    std::atomic<double> last_sec_send_rate{0.0};
};

#endif  /* _NETSTATS_H_ */