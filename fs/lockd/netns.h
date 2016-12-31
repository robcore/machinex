#ifndef __LOCKD_NETNS_H__
#define __LOCKD_NETNS_H__

#include <net/netns/generic.h>

struct lockd_net {
	unsigned int nlmsvc_users;
	unsigned long next_gc;
	unsigned long nrhosts;
};

extern int lockd_net_id;

#endif
