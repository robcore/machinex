#ifndef ___LINUX_XT_SOCKET_H
#define ___LINUX_XT_SOCKET_H

#include <uapi/linux/netfilter/xt_socket.h>

void xt_socket_put_sk(struct sock *sk);

#endif /* _XT_SOCKET_H */
