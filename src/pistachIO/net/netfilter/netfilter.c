#include <errno.h>

#include "err.h"
#include "kernel.h"
#include "opt.h"
#include "printk.h"
#include "fs/fs.h"
#include "linux/netfilter.h"
#include "kernel/sched.h"
#include "net/dpdk_module.h"
#include "net/net_namespace.h"
#include "net/net.h"
#if 0
static int __nf_register_net_python_hook(struct net * net, int pf, const struct nf_hook_python_ops * reg, PyObject * pHook, PyObject * pCond) {
    struct rte_tailq_entry_head * tbl;
    // struct rte_tailq_entry new_entry;
    struct rte_tailq_entry * new_entry = calloc(1, sizeof(struct rte_tailq_entry));
	struct nf_hook_entry * p = calloc(1, sizeof(struct nf_hook_entry));

    switch(reg->hooknum) {
        case NF_INET_PRE_ROUTING:
            tbl = pre_routing_table;
            break;
        case NF_INET_LOCAL_IN:
            tbl = local_in_table;
            break;
        case NF_INET_FORWARD:
            tbl = forward_table;
            break;
        case NF_INET_LOCAL_OUT:
            tbl = local_out_table;
            break;
        case NF_INET_POST_ROUTING:
            tbl = post_routing_table;
            break;
        default:
            pr_warn("Unkown hooknum!\n");
            return 0;
    }

    p->py_cb.pHook = pHook;
    p->priv = reg->priv;
    p->py_cb.pCond = pCond;
    p->type = PYTHON_CALLBACK;

    new_entry->data = (void *)p;
	TAILQ_INSERT_TAIL(tbl, new_entry, next);
    pr_debug(NF_DEBUG, "NEW entry: %p, hook: %p, priv: %p, tailq entry: %p\n", p, p->lib_cb.hook, p->priv, new_entry);

	return 0;
}

int nf_register_net_python_hook(struct net * net, const struct nf_hook_python_ops * reg, PyObject * pHook, PyObject * pCond) {
	int err;
	if (reg->pf == NFPROTO_INET) {
        pr_debug(NF_DEBUG, "registering hook...\n");
		err = __nf_register_net_python_hook(net, NFPROTO_IPV4, reg, pHook, pCond);
		if (err < 0) {
			return err;
        }
	}

	return 0;
}
#endif
double get_elapsed_time(struct timespec start, struct timespec end) {
    double start_time = start.tv_sec * 1.0e6 + start.tv_nsec / 1.0e3;
    double end_time = end.tv_sec * 1.0e6 + end.tv_nsec  / 1.0e3;
    return end_time - start_time;
}
#if 0
int invoke_lib_cond(struct nf_hook_entry * p, struct sk_buff * skb) {
    return p->lib_cb.cond(skb);
}

int invoke_lib_hook(struct nf_hook_entry * p, struct sk_buff * skb) {
    return p->lib_cb.hook(p->priv, skb, NULL);
}

int invoke_python_cond(struct nf_hook_entry * p, PyObject *pBuf) {
    int result = NF_MISS;
    // struct timespec start, end;
    // clock_gettime(CLOCK_MONOTONIC, &start);
    PyObject *pArgs = PyTuple_New(1);
    if (!pArgs) {
        PyErr_Print();
        Py_DECREF(pBuf);
        return 0; // Handle the error appropriately
    }

    if (PyTuple_SetItem(pArgs, 0, pBuf) != 0) { // PyTuple_SetItem steals the reference to pBuf
        PyErr_Print();
        Py_DECREF(pArgs); // This will also decrement the reference count of pBuf
        return 0; // Handle the error appropriately
    }

    PyObject *pValue = PyObject_CallObject(p->py_cb.pCond, pArgs);
    if (pValue) {
        if (PyLong_Check(pValue)) {
            result = PyLong_AsLong(pValue);
        } else {
            fprintf(stderr, "Unexpected return type from Python function\n");
        }
        Py_DECREF(pValue);
    } else {
        PyErr_Print();
    }

    // Py_DECREF(pArgs); // This will also decrement the reference count of pBuf
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // // Calculate and print elapsed time
    // double elapsed = get_elapsed_time(start, end);
    // printf("%.2f us\n", elapsed);
    return result;
}

int invoke_python_hook(struct nf_hook_entry * p, PyObject *pBuf) {
    int result = NF_DROP;
    Py_INCREF(pBuf); // Increment reference count before reusing pBuf
    PyObject *pArgs2 = PyTuple_New(1);
    if (!pArgs2) {
        PyErr_Print();
        Py_DECREF(pBuf);
        return 0; // Handle the error appropriately
    }

    if (PyTuple_SetItem(pArgs2, 0, pBuf) != 0) { // PyTuple_SetItem steals the reference to pBuf
        PyErr_Print();
        Py_DECREF(pArgs2);
        return 0; // Handle the error appropriately
    }

    PyObject *pResult2 = PyObject_CallObject(p->py_cb.pHook, pArgs2);
    if (pResult2) {
        if (PyLong_Check(pResult2)) {
            result = PyLong_AsLong(pResult2);
        }
        Py_DECREF(pResult2);
    } else {
        PyErr_Print();
    }
    Py_DECREF(pArgs2); // This will also decrement the reference count of pBuf
    return result;
}
#endif
#if 0
int nf_hook(unsigned int hook, struct sk_buff * skb) {
    struct nf_hook_entry * p;
	struct rte_tailq_entry * entry;
    struct rte_tailq_entry_head * tbl = NULL;

    switch(hook) {
        case NF_INET_PRE_ROUTING:
            tbl = pre_routing_table;
            break;
        case NF_INET_LOCAL_IN:
            tbl = local_in_table;
            break;
        case NF_INET_FORWARD:
            tbl = forward_table;
            break;
        case NF_INET_LOCAL_OUT:
            tbl = local_out_table;
            break;
        case NF_INET_POST_ROUTING:
            tbl = post_routing_table;
            break;
        default:
            pr_warn("Unkown hooknum!\n");
            return 0;
    }

    TAILQ_FOREACH(entry, tbl, next) {
        p = (struct nf_hook_entry *)entry->data;
        switch (p->type) {
            case LIB_CALLBACK:
                if (p->lib_cb.cond && invoke_lib_cond(p, skb) == NF_MATCH) {
#ifdef TEST_INLINE
                    if (invoke_lib_hook(p, skb) == NF_ACCEPT) {
                        while (rte_ring_enqueue(fwd_queue, skb) < 0) {
                        // while (rte_ring_enqueue(nf_cq, skb) < 0) {
                            printf("Failed to enqueue into nf_cq\n");
                        }
                    }
#else
                    struct nfcb_task_struct * new_entry = NULL;
                    do {
                        rte_mempool_get(nftask_mp, (void **)&new_entry);
                        if (new_entry) {
                            new_entry->entry = *p;
                            new_entry->skb = skb;
                            while (rte_ring_enqueue(nf_rq, new_entry) < 0) {
                                printf("Failed to enqueue into nf_rq\n");
                            }
                        }
                    } while (!new_entry);
#endif
                } else {
                    pr_debug(NF_DEBUG, "Entry: %p, hook: %p, priv: %p\n", p, p->lib_cb.hook, p->priv);
                }
                break;
            case PYTHON_CALLBACK:
                PyObject *pBuf = PyBytes_FromStringAndSize((const char *)skb->ptr, skb->len);
                if (!pBuf) {
                    PyErr_Print();
                    return 0; // Handle the error appropriately
                }

                if (invoke_python_cond(p, pBuf) == NF_MATCH) {
#ifdef TEST_INLINE
                    if (invoke_python_hook(p, pBuf) == NF_ACCEPT) {
                        while (rte_ring_enqueue(fwd_queue, skb) < 0) {
                        // while (rte_ring_enqueue(nf_cq, skb) < 0) {
                            printf("Failed to enqueue into nf_cq\n");
                        }
                    }
#else
                    struct nfcb_task_struct * new_entry = NULL;
                    do {
                        rte_mempool_get(nftask_mp, (void **)&new_entry);
                        if (new_entry) {
                            new_entry->entry = *p;
                            new_entry->skb = skb;
                            while (rte_ring_enqueue(nf_rq, new_entry) < 0) {
                                printf("Failed to enqueue into nf_rq\n");
                            }
                        }
                    } while (!new_entry);
#endif
                }
                break;
            default:
                break;
        }
    }

    return NET_RX_SUCCESS;
}
#endif

int nf_hook(unsigned int hook, struct sk_buff * skb) {
    struct nf_hook_entry * p;
	struct rte_tailq_entry * entry;
    struct rte_tailq_entry_head * tbl = NULL;

    switch(hook) {
        case NF_INET_PRE_ROUTING:
            tbl = pre_routing_table;
            break;
        case NF_INET_LOCAL_IN:
            tbl = local_in_table;
            break;
        case NF_INET_FORWARD:
            tbl = forward_table;
            break;
        case NF_INET_LOCAL_OUT:
            tbl = local_out_table;
            break;
        case NF_INET_POST_ROUTING:
            tbl = post_routing_table;
            break;
        default:
            pr_warn("Unkown hooknum!\n");
            return 0;
    }

    TAILQ_FOREACH(entry, tbl, next) {
        p = (struct nf_hook_entry *)entry->data;
        if (p->cond && p->cond(skb) == NF_MATCH) {
#ifdef TEST_INLINE
            if (p->hook(p->priv, skb, NULL) == NF_ACCEPT) {
                while (rte_ring_enqueue(fwd_queue, skb) < 0) {
                // while (rte_ring_enqueue(nf_cq, skb) < 0) {
                    printf("Failed to enqueue into nf_cq\n");
                }
            }
#else
            struct nfcb_task_struct * new_entry = NULL;
            do {
                rte_mempool_get(nftask_mp, (void **)&new_entry);
                if (new_entry) {
                    new_entry->entry = *p;
                    new_entry->skb = skb;
                    while (rte_ring_enqueue(nf_rq, new_entry) < 0) {
                        printf("Failed to enqueue into nf_rq\n");
                    }
                }
            } while (!new_entry);
            break;
#endif
        } else {
            pr_debug(NF_DEBUG, "Entry: %p, hook: %p, priv: %p\n", p, p->hook, p->priv);
        }
    }

    return NET_RX_SUCCESS;
}