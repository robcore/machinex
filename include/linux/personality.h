/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PERSONALITY_H
#define _LINUX_PERSONALITY_H

#include <uapi/linux/personality.h>


#define set_personality(pers)	(current->personality = (pers))

#endif /* _LINUX_PERSONALITY_H */
