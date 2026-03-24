#undef TRACE_SYSTEM
#define TRACE_SYSTEM asoc
#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_ASOC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_ASOC_H

#include <trace/hooks/vendor_hooks.h>

DECLARE_HOOK(android_vh_put_volsw,
                TP_PROTO(int platform_max, int min, unsigned int val, int *err),
                TP_ARGS(platform_max, min, val, err));

#endif
#include <trace/define_trace.h>
