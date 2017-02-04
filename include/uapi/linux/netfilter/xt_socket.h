#ifndef __UAPI_LINUX_XT_SOCKET_H
#define __UAPI_LINUX_XT_SOCKET_H

#include <linux/types.h>

enum {
	XT_SOCKET_TRANSPARENT = 1 << 0,
};

struct xt_socket_mtinfo1 {
	__u8 flags;
};

struct sock *xt_socket_get4_sk(const struct sk_buff *skb,
			       struct xt_action_param *par);
struct sock *xt_socket_get6_sk(const struct sk_buff *skb,
			       struct xt_action_param *par);

#endif /* _XT_SOCKET_H */
