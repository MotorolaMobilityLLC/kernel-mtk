#ifndef __DRV_CLK_DEBUG_H
#define __DRV_CLK_DEBUG_H

/*
 * This is a private header file. DO NOT include it except clk-*.c.
 */

#ifdef CLK_DEBUG
#undef CLK_DEBUG
#endif

#define CLK_DEBUG		1

#define TRACE(fmt, args...) \
	pr_warn("[clk-mtk] %s():%d: " fmt, __func__, __LINE__, ##args)

#define PR_ERR(fmt, args...) pr_warn("%s(): " fmt, __func__, ##args)

#ifdef pr_debug
#undef pr_debug
#define pr_debug TRACE
#endif

#ifdef pr_err
#undef pr_err
#define pr_err PR_ERR
#endif

#endif /* __DRV_CLK_DEBUG_H */
