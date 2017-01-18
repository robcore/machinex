#ifndef __LINUX_IP_NETFILTER_H
#define __LINUX_IP_NETFILTER_H

/* IPv4-specific defines for netfilter. 
 * (C)1998 Rusty Russell -- This code is GPL.
 */

#include <uapi/linux/netfilter_ipv4.h>

extern int ip_route_me_harder(struct sk_buff *skb, unsigned addr_type);
extern int ip_xfrm_me_harder(struct sk_buff *skb);
extern __sum16 nf_ip_checksum(struct sk_buff *skb, unsigned int hook,
				   unsigned int dataoff, u_int8_t protocol);

#endif /*__LINUX_IP_NETFILTER_H*/
