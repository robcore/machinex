#undef TRACE_SYSTEM
#define TRACE_SYSTEM writeback

#if !defined(_TRACE_WRITEBACK_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_WRITEBACK_H

#include <linux/backing-dev.h>
#include <linux/writeback.h>

#define show_inode_state(state) {}

#define WB_WORK_REASON {}

#endif /* _TRACE_WRITEBACK_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
