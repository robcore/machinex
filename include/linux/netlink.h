/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_NETLINK_H
#define __LINUX_NETLINK_H

#include <uapi/linux/netlink.h>

#include <linux/capability.h>
#include <linux/skbuff.h>
#include <linux/export.h>

struct net;

static inline struct nlmsghdr *nlmsg_hdr(const struct sk_buff *skb)
{
	return (struct nlmsghdr *)skb->data;
}

struct netlink_skb_parms {
	struct ucred		creds;		/* Skb credentials	*/
	__u32			pid;
	__u32			dst_group;
	struct sock		*ssk;
};

#define NETLINK_CB(skb)		(*(struct netlink_skb_parms*)&((skb)->cb))
#define NETLINK_CREDS(skb)	(&NETLINK_CB((skb)).creds)


extern void netlink_table_grab(void);
extern void netlink_table_ungrab(void);

extern struct sock *netlink_kernel_create(struct net *net,
					  int unit,unsigned int groups,
					  void (*input)(struct sk_buff *skb),
					  struct mutex *cb_mutex,
					  struct module *module);
extern void netlink_kernel_release(struct sock *sk);
extern int __netlink_change_ngroups(struct sock *sk, unsigned int groups);
extern int netlink_change_ngroups(struct sock *sk, unsigned int groups);
extern void __netlink_clear_multicast_users(struct sock *sk, unsigned int group);
extern void netlink_clear_multicast_users(struct sock *sk, unsigned int group);
extern void netlink_ack(struct sk_buff *in_skb, struct nlmsghdr *nlh, int err);
extern int netlink_has_listeners(struct sock *sk, unsigned int group);
extern int netlink_unicast(struct sock *ssk, struct sk_buff *skb, __u32 pid, int nonblock);
extern int netlink_broadcast(struct sock *ssk, struct sk_buff *skb, __u32 pid,
			     __u32 group, gfp_t allocation);
extern int netlink_broadcast_filtered(struct sock *ssk, struct sk_buff *skb,
	__u32 pid, __u32 group, gfp_t allocation,
	int (*filter)(struct sock *dsk, struct sk_buff *skb, void *data),
	void *filter_data);
extern int netlink_set_err(struct sock *ssk, __u32 pid, __u32 group, int code);
extern int netlink_register_notifier(struct notifier_block *nb);
extern int netlink_unregister_notifier(struct notifier_block *nb);

/* finegrained unicast helpers: */
struct sock *netlink_getsockbyfilp(struct file *filp);
int netlink_attachskb(struct sock *sk, struct sk_buff *skb,
		      long *timeo, struct sock *ssk);
void netlink_detachskb(struct sock *sk, struct sk_buff *skb);
int netlink_sendskb(struct sock *sk, struct sk_buff *skb);

/*
 *	skb should fit one page. This choice is good for headerless malloc.
 *	But we should limit to 8K so that userspace does not have to
 *	use enormous buffer sizes on recvmsg() calls just to avoid
 *	MSG_TRUNC when PAGE_SIZE is very large.
 */
#if PAGE_SIZE < 8192UL
#define NLMSG_GOODSIZE	SKB_WITH_OVERHEAD(PAGE_SIZE)
#else
#define NLMSG_GOODSIZE	SKB_WITH_OVERHEAD(8192UL)
#endif

#define NLMSG_DEFAULT_SIZE (NLMSG_GOODSIZE - NLMSG_HDRLEN)


struct netlink_callback {
	struct sk_buff		*skb;
	const struct nlmsghdr	*nlh;
	int			(*dump)(struct sk_buff * skb,
					struct netlink_callback *cb);
	int			(*done)(struct netlink_callback *cb);
	void			*data;
	/* the module that dump function belong to */
	struct module		*module;
	u16			family;
	u16			min_dump_alloc;
	unsigned int		prev_seq, seq;
	long			args[6];
};

struct netlink_notify {
	struct net *net;
	int pid;
	int protocol;
};

struct nlmsghdr *
__nlmsg_put(struct sk_buff *skb, u32 pid, u32 seq, int type, int len, int flags);

#define NLMSG_NEW(skb, pid, seq, type, len, flags) \
({	if (unlikely(skb_tailroom(skb) < (int)NLMSG_SPACE(len))) \
		goto nlmsg_failure; \
	__nlmsg_put(skb, pid, seq, type, len, flags); })

#define NLMSG_PUT(skb, pid, seq, type, len) \
	NLMSG_NEW(skb, pid, seq, type, len, 0)

struct netlink_dump_control {
	int (*dump)(struct sk_buff *skb, struct netlink_callback *);
	int (*done)(struct netlink_callback *);
	void *data;
	struct module *module;
	u16 min_dump_alloc;
};

extern int __netlink_dump_start(struct sock *ssk, struct sk_buff *skb,
				const struct nlmsghdr *nlh,
				struct netlink_dump_control *control);
static inline int netlink_dump_start(struct sock *ssk, struct sk_buff *skb,
				     const struct nlmsghdr *nlh,
				     struct netlink_dump_control *control)
{
	if (!control->module)
		control->module = THIS_MODULE;

	return __netlink_dump_start(ssk, skb, nlh, control);
}


#define NL_NONROOT_RECV 0x1
#define NL_NONROOT_SEND 0x2
extern void netlink_set_nonroot(int protocol, unsigned flag);

#endif	/* __LINUX_NETLINK_H */
