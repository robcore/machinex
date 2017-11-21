/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_RTNETLINK_H
#define __LINUX_RTNETLINK_H

#include <uapi/linux/rtnetlink.h>

#include <linux/mutex.h>
#include <linux/netdevice.h>

static __inline__ int rtattr_strcmp(const struct rtattr *rta, const char *str)
{
	int len = strlen(str) + 1;
	return len > rta->rta_len || memcmp(RTA_DATA(rta), str, len);
}

extern int rtnetlink_send(struct sk_buff *skb, struct net *net, u32 pid, u32 group, int echo);
extern int rtnl_unicast(struct sk_buff *skb, struct net *net, u32 pid);
extern void rtnl_notify(struct sk_buff *skb, struct net *net, u32 pid,
			u32 group, struct nlmsghdr *nlh, gfp_t flags);
extern void rtnl_set_sk_err(struct net *net, u32 group, int error);
extern int rtnetlink_put_metrics(struct sk_buff *skb, u32 *metrics);
extern int rtnl_put_cacheinfo(struct sk_buff *skb, struct dst_entry *dst,
			      u32 id, u32 ts, u32 tsage, long expires,
			      u32 error);

extern void __rta_fill(struct sk_buff *skb, int attrtype, int attrlen, const void *data);

#define RTA_PUT(skb, attrtype, attrlen, data) \
({	if (unlikely(skb_tailroom(skb) < (int)RTA_SPACE(attrlen))) \
		 goto rtattr_failure; \
   	__rta_fill(skb, attrtype, attrlen, data); }) 

#define RTA_APPEND(skb, attrlen, data) \
({	if (unlikely(skb_tailroom(skb) < (int)(attrlen))) \
		goto rtattr_failure; \
	memcpy(skb_put(skb, attrlen), data, attrlen); })

#define RTA_PUT_NOHDR(skb, attrlen, data) \
({	RTA_APPEND(skb, RTA_ALIGN(attrlen), data); \
	memset(skb_tail_pointer(skb) - (RTA_ALIGN(attrlen) - attrlen), 0, \
	       RTA_ALIGN(attrlen) - attrlen); })

#define RTA_PUT_U8(skb, attrtype, value) \
({	u8 _tmp = (value); \
	RTA_PUT(skb, attrtype, sizeof(u8), &_tmp); })

#define RTA_PUT_U16(skb, attrtype, value) \
({	u16 _tmp = (value); \
	RTA_PUT(skb, attrtype, sizeof(u16), &_tmp); })

#define RTA_PUT_U32(skb, attrtype, value) \
({	u32 _tmp = (value); \
	RTA_PUT(skb, attrtype, sizeof(u32), &_tmp); })

#define RTA_PUT_U64(skb, attrtype, value) \
({	u64 _tmp = (value); \
	RTA_PUT(skb, attrtype, sizeof(u64), &_tmp); })

#define RTA_PUT_SECS(skb, attrtype, value) \
	RTA_PUT_U64(skb, attrtype, (value) / HZ)

#define RTA_PUT_MSECS(skb, attrtype, value) \
	RTA_PUT_U64(skb, attrtype, jiffies_to_msecs(value))

#define RTA_PUT_STRING(skb, attrtype, value) \
	RTA_PUT(skb, attrtype, strlen(value) + 1, value)

#define RTA_PUT_FLAG(skb, attrtype) \
	RTA_PUT(skb, attrtype, 0, NULL);

#define RTA_NEST(skb, type) \
({	struct rtattr *__start = (struct rtattr *)skb_tail_pointer(skb); \
	RTA_PUT(skb, type, 0, NULL); \
	__start;  })

#define RTA_NEST_END(skb, start) \
({	(start)->rta_len = skb_tail_pointer(skb) - (unsigned char *)(start); \
	(skb)->len; })

#define RTA_NEST_COMPAT(skb, type, attrlen, data) \
({	struct rtattr *__start = (struct rtattr *)skb_tail_pointer(skb); \
	RTA_PUT(skb, type, attrlen, data); \
	RTA_NEST(skb, type); \
	__start; })

#define RTA_NEST_COMPAT_END(skb, start) \
({	struct rtattr *__nest = (void *)(start) + NLMSG_ALIGN((start)->rta_len); \
	(start)->rta_len = skb_tail_pointer(skb) - (unsigned char *)(start); \
	RTA_NEST_END(skb, __nest); \
	(skb)->len; })

#define RTA_NEST_CANCEL(skb, start) \
({	if (start) \
		skb_trim(skb, (unsigned char *) (start) - (skb)->data); \
	-1; })

#define RTA_GET_U8(rta) \
({	if (!rta || RTA_PAYLOAD(rta) < sizeof(u8)) \
		goto rtattr_failure; \
	*(u8 *) RTA_DATA(rta); })

#define RTA_GET_U16(rta) \
({	if (!rta || RTA_PAYLOAD(rta) < sizeof(u16)) \
		goto rtattr_failure; \
	*(u16 *) RTA_DATA(rta); })

#define RTA_GET_U32(rta) \
({	if (!rta || RTA_PAYLOAD(rta) < sizeof(u32)) \
		goto rtattr_failure; \
	*(u32 *) RTA_DATA(rta); })

#define RTA_GET_U64(rta) \
({	u64 _tmp; \
	if (!rta || RTA_PAYLOAD(rta) < sizeof(u64)) \
		goto rtattr_failure; \
	memcpy(&_tmp, RTA_DATA(rta), sizeof(_tmp)); \
	_tmp; })

#define RTA_GET_FLAG(rta) (!!(rta))

#define RTA_GET_SECS(rta) ((unsigned long) RTA_GET_U64(rta) * HZ)
#define RTA_GET_MSECS(rta) (msecs_to_jiffies((unsigned long) RTA_GET_U64(rta)))
		
static inline struct rtattr *
__rta_reserve(struct sk_buff *skb, int attrtype, int attrlen)
{
	struct rtattr *rta;
	int size = RTA_LENGTH(attrlen);

	rta = (struct rtattr*)skb_put(skb, RTA_ALIGN(size));
	rta->rta_type = attrtype;
	rta->rta_len = size;
	memset(RTA_DATA(rta) + attrlen, 0, RTA_ALIGN(size) - size);
	return rta;
}

#define __RTA_PUT(skb, attrtype, attrlen) \
({ 	if (unlikely(skb_tailroom(skb) < (int)RTA_SPACE(attrlen))) \
		goto rtattr_failure; \
   	__rta_reserve(skb, attrtype, attrlen); })

extern void rtmsg_ifinfo(int type, struct net_device *dev, unsigned change);

/* RTNL is used as a global lock for all changes to network configuration  */
extern void rtnl_lock(void);
extern void rtnl_unlock(void);
extern int rtnl_trylock(void);
extern int rtnl_is_locked(void);
#ifdef CONFIG_PROVE_LOCKING
extern int lockdep_rtnl_is_held(void);
#endif /* #ifdef CONFIG_PROVE_LOCKING */

/**
 * rcu_dereference_rtnl - rcu_dereference with debug checking
 * @p: The pointer to read, prior to dereferencing
 *
 * Do an rcu_dereference(p), but check caller either holds rcu_read_lock()
 * or RTNL. Note : Please prefer rtnl_dereference() or rcu_dereference()
 */
#define rcu_dereference_rtnl(p)					\
	rcu_dereference_check(p, lockdep_rtnl_is_held())

/**
 * rtnl_dereference - fetch RCU pointer when updates are prevented by RTNL
 * @p: The pointer to read, prior to dereferencing
 *
 * Return the value of the specified RCU-protected pointer, but omit
 * both the smp_read_barrier_depends() and the ACCESS_ONCE(), because
 * caller holds RTNL.
 */
#define rtnl_dereference(p)					\
	rcu_dereference_protected(p, lockdep_rtnl_is_held())

static inline struct netdev_queue *dev_ingress_queue(struct net_device *dev)
{
	return rtnl_dereference(dev->ingress_queue);
}

extern struct netdev_queue *dev_ingress_queue_create(struct net_device *dev);

extern void rtnetlink_init(void);
extern void __rtnl_unlock(void);

#define ASSERT_RTNL() do { \
	if (unlikely(!rtnl_is_locked())) { \
		printk(KERN_ERR "RTNL: assertion failed at %s (%d)\n", \
		       __FILE__,  __LINE__); \
		dump_stack(); \
	} \
} while(0)

static inline u32 rtm_get_table(struct rtattr **rta, u8 table)
{
	return RTA_GET_U32(rta[RTA_TABLE-1]);
rtattr_failure:
	return table;
}

#endif	/* __LINUX_RTNETLINK_H */
