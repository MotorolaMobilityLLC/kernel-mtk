#ifndef __CCCI_DEBUG_H__
#define __CCCI_DEBUG_H__

/* tag defination */
#define CORE "cor"
#define BM "bfm"
#define NET "net"
#define CHAR "chr"
#define SYSFS "sfs"
#define KERN "ken"
#define IPC "ipc"
#define RPC "rpc"

extern unsigned int ccci_debug_enable;
extern int ccci_log_write(const char *fmt, ...);

/* for detail log, default off (ccci_debug_enable == 0) */
#define CCCI_DBG_MSG(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)
/* for normal log, print to RAM by default, print to UART when needed */
#define CCCI_INF_MSG(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)

/* for traffic log, print to RAM by default, print to UART when needed */
#define CCCI_REP_MSG(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)

/* for critical init log*/
#define CCCI_NOTICE_MSG(idx, tag, fmt, args...) pr_notice("[md%d/ntc]" fmt, (idx+1), ##args)
/* for error log */
#define CCCI_ERR_MSG(idx, tag, fmt, args...) pr_err("[md%d/err]" fmt, (idx+1), ##args)
/* exception log: to uart */
#define CCCI_EXP_MSG(idx, tag, fmt, args...) pr_warn("[md%d/err]" fmt, (idx+1), ##args)
/* exception log: to kernel or ccci_dump of SYS_CCCI_DUMP in DB */
#define CCCI_EXP_INF_MSG(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)
/* exception log: to kernel or ccci_dump of SYS_CCCI_DUMP in DB, which had time stamp as head of dump */
#define CCCI_DUMP_MSG1(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)
/* exception log: to kernel or ccci_dump of SYS_CCCI_DUMP in DB, which had no time stamp */
#define CCCI_DUMP_MSG2(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_warn("[md%d]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[md%d]" fmt, (idx+1), ##args); \
} while (0)

#define CCCI_DBG_COM_MSG(fmt, args...)		 CCCI_DBG_MSG(0, "com", fmt, ##args)
#define CCCI_INF_COM_MSG(fmt, args...)		 CCCI_INF_MSG(0, "com", fmt, ##args)
#define CCCI_ERR_COM_MSG(fmt, args...)		 CCCI_ERR_MSG(0, "com", fmt, ##args)

#define CLDMA_TRACE
/* #define PORT_NET_TRACE */
#define CCCI_SKB_TRACE
/* #define CCCI_BM_TRACE */

#endif				/* __CCCI_DEBUG_H__ */
