#ifndef _LINUX_OMNIBOOST_H
#define _LINUX_OMNIBOOST_H

#include <linux/notifier.h>

#define BOOST_ON 0x01
#define BOOST_OFF 0x02

int register_omniboost(struct notifier_block *nb);
int unregister_omniboost(struct notifier_block *nb);
#endif /* _LINUX_OMNIBOOST_H */
