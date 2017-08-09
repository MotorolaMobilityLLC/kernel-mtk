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
		pr_debug("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_notice("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 6) \
		pr_err("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
} while (0)
/* for normal log, print to RAM by default, print to UART when needed */
#define CCCI_INF_MSG(idx, tag, fmt, args...) \
do { \
	if (ccci_debug_enable == 0 || ccci_debug_enable == 1) \
		pr_debug("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 2) \
		pr_notice("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
	else if (ccci_debug_enable == 5 || ccci_debug_enable == 6) \
		pr_err("[ccci%d/" tag "]" fmt, (idx+1), ##args); \
} while (0)

/* for critical init log*/
#define CCCI_NOTICE_MSG(idx, tag, fmt, args...) pr_notice("[ccci%d/ntc/" tag "]" fmt, (idx+1), ##args)
/* for error log */
#define CCCI_ERR_MSG(idx, tag, fmt, args...) pr_err("[ccci%d/err/" tag "]" fmt, (idx+1), ##args)

#define CCCI_DBG_COM_MSG(fmt, args...)		 CCCI_DBG_MSG(0, "com", fmt, ##args)
#define CCCI_INF_COM_MSG(fmt, args...)		 CCCI_INF_MSG(0, "com", fmt, ##args)
#define CCCI_ERR_COM_MSG(fmt, args...)		 CCCI_ERR_MSG(0, "com", fmt, ##args)

#define CLDMA_TRACE
/* #define PORT_NET_TRACE */
#define CCCI_SKB_TRACE
/* #define CCCI_BM_TRACE */

#endif				/* __CCCI_DEBUG_H__ */
