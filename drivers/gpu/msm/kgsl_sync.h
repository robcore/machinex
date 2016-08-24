/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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
#ifndef __KGSL_SYNC_H
#define __KGSL_SYNC_H

#include <linux/sync.h>
#include "kgsl_device.h"

struct kgsl_sync_timeline {
	struct sync_timeline timeline;
	unsigned int last_timestamp;
	struct kgsl_device *device;
	u32 context_id;
};

struct kgsl_sync_pt {
	struct sync_pt pt;
	unsigned int timestamp;
};

#if defined(CONFIG_SYNC)
struct sync_pt *kgsl_sync_pt_create(struct sync_timeline *timeline,
	unsigned int timestamp);
void kgsl_sync_pt_destroy(struct sync_pt *pt);
int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner);
int kgsl_sync_timeline_create(struct kgsl_context *context);
void kgsl_sync_timeline_signal(struct sync_timeline *timeline,
	unsigned int timestamp);
void kgsl_sync_timeline_destroy(struct kgsl_context *context);
#else
static inline struct sync_pt
*kgsl_sync_pt_create(struct sync_timeline *timeline, unsigned int timestamp)
{
	return NULL;
}

static inline void kgsl_sync_pt_destroy(struct sync_pt *pt)
{
}

static inline int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	return -EINVAL;
}

static int kgsl_sync_timeline_create(struct kgsl_context *context)
{
	context->timeline = NULL;
	return 0;
}

static inline void
kgsl_sync_timeline_signal(struct sync_timeline *timeline,
	unsigned int timestamp)
{
}

static inline void kgsl_sync_timeline_destroy(struct kgsl_context *context)
{
}
#endif

#endif /* __KGSL_SYNC_H */
