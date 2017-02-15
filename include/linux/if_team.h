/*
 * include/linux/if_team.h - Network team device driver header
 * Copyright (c) 2011 Jiri Pirko <jpirko@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _LINUX_IF_TEAM_H_
#define _LINUX_IF_TEAM_H_

#include <uapi/linux/if_team.h>

struct team_pcpu_stats {
	u64			rx_packets;
	u64			rx_bytes;
	u64			rx_multicast;
	u64			tx_packets;
	u64			tx_bytes;
	struct u64_stats_sync	syncp;
	u32			rx_dropped;
	u32			tx_dropped;
};

struct team;

struct team_port {
	struct net_device *dev;
	struct hlist_node hlist; /* node in hash list */
	struct list_head list; /* node in ordinary list */
	struct team *team;
	int index;

	/*
	 * A place for storing original values of the device before it
	 * become a port.
	 */
	struct {
		unsigned char dev_addr[MAX_ADDR_LEN];
		unsigned int mtu;
	} orig;

	bool linkup;
	u32 speed;
	u8 duplex;

	/* Custom gennetlink interface related flags */
	bool changed;
	bool removed;

	struct rcu_head rcu;
};

struct team_mode_ops {
	int (*init)(struct team *team);
	void (*exit)(struct team *team);
	rx_handler_result_t (*receive)(struct team *team,
				       struct team_port *port,
				       struct sk_buff *skb);
	bool (*transmit)(struct team *team, struct sk_buff *skb);
	int (*port_enter)(struct team *team, struct team_port *port);
	void (*port_leave)(struct team *team, struct team_port *port);
	void (*port_change_mac)(struct team *team, struct team_port *port);
};

enum team_option_type {
	TEAM_OPTION_TYPE_U32,
	TEAM_OPTION_TYPE_STRING,
};

struct team_option {
	struct list_head list;
	const char *name;
	enum team_option_type type;
	int (*getter)(struct team *team, void *arg);
	int (*setter)(struct team *team, void *arg);

	/* Custom gennetlink interface related flags */
	bool changed;
	bool removed;
};

struct team_mode {
	struct list_head list;
	const char *kind;
	struct module *owner;
	size_t priv_size;
	const struct team_mode_ops *ops;
};

#define TEAM_PORT_HASHBITS 4
#define TEAM_PORT_HASHENTRIES (1 << TEAM_PORT_HASHBITS)

#define TEAM_MODE_PRIV_LONGS 4
#define TEAM_MODE_PRIV_SIZE (sizeof(long) * TEAM_MODE_PRIV_LONGS)

struct team {
	struct net_device *dev; /* associated netdevice */
	struct team_pcpu_stats __percpu *pcpu_stats;

	struct mutex lock; /* used for overall locking, e.g. port lists write */

	/*
	 * port lists with port count
	 */
	int port_count;
	struct hlist_head port_hlist[TEAM_PORT_HASHENTRIES];
	struct list_head port_list;

	struct list_head option_list;

	const struct team_mode *mode;
	struct team_mode_ops ops;
	bool port_mtu_change_allowed;
	long mode_priv[TEAM_MODE_PRIV_LONGS];
};

static inline struct hlist_head *team_port_index_hash(struct team *team,
						      int port_index)
{
	return &team->port_hlist[port_index & (TEAM_PORT_HASHENTRIES - 1)];
}

static inline struct team_port *team_get_port_by_index(struct team *team,
						       int port_index)
{
	struct team_port *port;
	struct hlist_head *head = team_port_index_hash(team, port_index);

	hlist_for_each_entry(port, head, hlist)
		if (port->index == port_index)
			return port;
	return NULL;
}
static inline struct team_port *team_get_port_by_index_rcu(struct team *team,
							   int port_index)
{
	struct team_port *port;
	struct hlist_head *head = team_port_index_hash(team, port_index);

	hlist_for_each_entry_rcu(port, head, hlist)
		if (port->index == port_index)
			return port;
	return NULL;
}

extern int team_port_set_team_mac(struct team_port *port);
extern int team_options_register(struct team *team,
				 const struct team_option *option,
				 size_t option_count);
extern void team_options_unregister(struct team *team,
				    const struct team_option *option,
				    size_t option_count);
extern int team_mode_register(struct team_mode *mode);
extern int team_mode_unregister(struct team_mode *mode);

#endif /* _LINUX_IF_TEAM_H_ */
