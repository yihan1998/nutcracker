#ifndef _NETFILTER_H_
#define _NETFILTER_H_

#include <stdint.h>
#include <sys/socket.h>
#if 0
#include <Python.h>
#endif

#include "linux/netfilter_def.h"

struct net_device;
struct sk_buff;
struct nf_hook_ops;
struct sock;
struct net;

struct lib_callback_ops;
struct python_callback_ops;

struct nf_hook_state {
	unsigned int hook;
	uint8_t pf;
	struct net_device *in;
	struct net_device *out;
	struct sock *sk;
	struct net *net;
	int (*okfn)(struct net *, struct sock *, struct sk_buff *);
};

typedef unsigned int nf_condfn(struct sk_buff *skb);
typedef unsigned int nf_hookfn(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

#if 0
typedef enum {
	LIB_CALLBACK	= 0,
	PYTHON_CALLBACK,
} callback_t;
#endif

struct lib_callback_ops {
	nf_hookfn   *hook;
	nf_condfn	*cond;
};

#if 0
struct python_callback_ops {
	PyObject * pHook;
	PyObject * pCond;
};
#endif

struct nf_hook_entry {
#if 0
	callback_t	type;
	union {
		struct lib_callback_ops lib_cb;
		struct python_callback_ops py_cb;
	};
#endif
	nf_hookfn   *hook;
	nf_condfn	*cond;
	void        *priv;
};

struct nf_hook_entries {
	uint16_t num_hook_entries;
	struct nf_hook_entry hooks[];
};

struct nfcb_task_struct {
	struct nf_hook_entry entry;
	struct sk_buff * skb;
};

/* Used for register */
struct nf_hook_ops {
	/* User fills in from here down. */
	nf_hookfn   *hook;
	struct net_device   *dev;
	void        *priv;
	nf_condfn	*cond;
	uint8_t     pf;
	unsigned int    hooknum;
	int         priority;
};

#if 0
struct nf_hook_python_ops {
	/* User fills in from here down. */
	char		hook[16];
	struct net_device   *dev;
	void        *priv;
	char		cond[16];
	uint8_t     pf;
	unsigned int    hooknum;
	int         priority;
};
#endif

int nf_register_net_hook(struct net *net, const struct nf_hook_ops *ops);
void nf_unregister_net_hook(struct net *net, const struct nf_hook_ops *ops);

#if 0
int nf_register_net_python_hook(struct net * net, const struct nf_hook_python_ops * reg, PyObject * pHook, PyObject * pCond);
#endif

#endif  /* _NETFILTER_H_ */