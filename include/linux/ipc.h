#ifndef _LINUX_IPC_H
#define _LINUX_IPC_H

#include <uapi/linux/ipc.h>

#include <linux/spinlock.h>

#define IPCMNI 32768  /* <= MAX_INT limit for ipc arrays (including sysctl changes) */

/* used by in-kernel data structures */
struct kern_ipc_perm
{
	spinlock_t	lock;
	int		deleted;
	int		id;
	key_t		key;
	uid_t		uid;
	gid_t		gid;
	uid_t		cuid;
	gid_t		cgid;
	umode_t		mode; 
	unsigned long	seq;
	void		*security;
};

#endif /* _LINUX_IPC_H */
