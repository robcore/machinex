/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/spinlock.h>
#include <linux/videodev2.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>

#include "msm.h"

#ifdef CONFIG_MSM_CAMERA_DEBUG
#define D(fmt, args...) pr_debug("msm_isp: " fmt, ##args)
#else
#define D(fmt, args...) do {} while (0)
#endif

#define PAD_TO_WORD(a)	  (((a) + 3) & ~3)

#define __CONTAINS(r, v, l, field) ({			   \
	typeof(r) __r = r;				  \
	typeof(v) __v = v;				  \
	typeof(v) __e = __v + l;				\
	int res = __v >= __r->field &&			  \
		__e <= __r->field + __r->len;		   \
	res;							\
})

#define CONTAINS(r1, r2, field) ({			  \
	typeof(r2) __r2 = r2;				   \
	__CONTAINS(r1, __r2->field, __r2->len, field);	  \
})

#define IN_RANGE(r, v, field) ({				\
	typeof(r) __r = r;				  \
	typeof(v) __vv = v;				 \
	int res = ((__vv >= __r->field) &&		  \
		(__vv < (__r->field + __r->len)));	  \
	res;							\
})

#define OVERLAPS(r1, r2, field) ({			  \
	typeof(r1) __r1 = r1;				   \
	typeof(r2) __r2 = r2;				   \
	typeof(__r2->field) __v = __r2->field;		  \
	typeof(__v) __e = __v + __r2->len - 1;		  \
	int res = (IN_RANGE(__r1, __v, field) ||		\
		IN_RANGE(__r1, __e, field));				 \
	res;							\
})

static DEFINE_MUTEX(hlist_mut);
