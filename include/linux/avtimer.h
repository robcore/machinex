#ifndef __LINUX_AVTIMER_H
#define _LINUX_AVTIMER_H

#include <uapi/linux/avtimer.h>

int avcs_core_open(void);
int avcs_core_disable_power_collapse(int disable);/* true or false */

#endif
