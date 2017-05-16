/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _STP_DEBUG_H_
#define _STP_DEBUG_H_

#include <linux/time.h>
#include "stp_btif.h"
#include "osal.h"
#include "wmt_exp.h"

#define CONFIG_LOG_STP_INTERNAL

#if 1				/* #ifndef CONFIG_LOG_STP_INTERNAL */
#define STP_PKT_SZ  16
#define STP_DMP_SZ 2048
#define STP_PKT_NO 2048

#define STP_DBG_LOG_ENTRY_NUM 1024
#define STP_DBG_LOG_ENTRY_SZ  2048

#else

#define STP_PKT_SZ  16
#define STP_DMP_SZ 16
#define STP_PKT_NO 16

#define STP_DBG_LOG_ENTRY_NUM 28
#define STP_DBG_LOG_ENTRY_SZ 64

#endif

#define PFX_STP_DBG                      "[STPDbg]"
#define STP_DBG_LOG_LOUD                 4
#define STP_DBG_LOG_DBG                  3
#define STP_DBG_LOG_INFO                 2
#define STP_DBG_LOG_WARN                 1
#define STP_DBG_LOG_ERR                  0

extern UINT32 gStpDbgDbgLevel;

#define STP_DBG_LOUD_FUNC(fmt, arg...) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_LOUD) \
		pr_warn(PFX_STP_DBG "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_DBG_DBG_FUNC(fmt, arg...) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_DBG) \
		pr_warn(PFX_STP_DBG "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_DBG_INFO_FUNC(fmt, arg...) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_INFO) \
		pr_warn(PFX_STP_DBG "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_DBG_WARN_FUNC(fmt, arg...) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_WARN) \
		pr_warn(PFX_STP_DBG "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_DBG_ERR_FUNC(fmt, arg...) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_ERR) \
		pr_err(PFX_STP_DBG "%s: "   fmt, __func__, ##arg); \
} while (0)
#define STP_DBG_TRC_FUNC(f) \
do { \
	if (gStpDbgDbgLevel >= STP_DBG_LOG_DBG) \
		pr_warn(PFX_STP_DBG "<%s> <%d>\n", __func__, __LINE__); \
} while (0)

enum STP_DBG_OP_T {
	STP_DBG_EN = 0,
	STP_DBG_PKT = 1,
	STP_DBG_DR = 2,
	STP_DBG_FW_ASSERT = 3,
	STP_DBG_FW_LOG = 4,
	STP_DBG_FW_DMP = 5,
	STP_DBG_MAX
};

enum STP_DBG_PKT_FIL_T {
	STP_DBG_PKT_FIL_ALL = 0,
	STP_DBG_PKT_FIL_BT = 1,
	STP_DBG_PKT_FIL_GPS = 2,
	STP_DBG_PKT_FIL_FM = 3,
	STP_DBG_PKT_FIL_WMT = 4,
	STP_DBG_PKT_FIL_MAX
};

static PINT8 const comboStpDbgType[] = {
	"< BT>",
	"< FM>",
	"<GPS>",
	"<WiFi>",
	"<WMT>",
	"<STP>",
	"<DBG>",
	"<ANT>",
	"<SDIO_OWN_SET>",
	"<SDIO_OWN_CLR>",
	"<WAKEINT>",
	"<UNKNOWN>"
};

static PINT8 const socStpDbgType[] = {
	"< BT>",
	"< FM>",
	"<GPS>",
	"<WiFi>",
	"<WMT>",
	"<STP>",
	"<DBG>",
	"<WAKEINT>",
	"<UNKNOWN>"
};


enum STP_DBG_DR_FIL_T {
	STP_DBG_DR_MAX = 0,
};

enum STP_DBG_FW_FIL_T {
	STP_DBG_FW_MAX = 0,
};

enum STP_DBG_PKT_DIR_T{
	PKT_DIR_RX = 0,
	PKT_DIR_TX
};

/*simple log system ++*/

struct MTKSTP_LOG_ENTRY_T {
	/*type: 0. pkt trace 1. fw info
	* 2. assert info 3. trace32 dump .
	* -1. linked to the the previous
	*/
	INT32 id;
	INT32 len;
	INT8 buffer[STP_DBG_LOG_ENTRY_SZ];
};

struct MTKSTP_LOG_SYS_T {
	struct MTKSTP_LOG_ENTRY_T queue[STP_DBG_LOG_ENTRY_NUM];
	UINT32 size;
	UINT32 in;
	UINT32 out;
	spinlock_t lock;
};
/*--*/

struct STP_DBG_HDR_T {
	/* packet information */
	UINT32 sec;
	UINT32 usec;
	UINT32 dbg_type;
	UINT32 last_dbg_type;
	UINT32 dmy;
	UINT32 no;
	UINT32 dir;

	/* packet content */
	UINT32 type;
	UINT32 len;
	UINT32 ack;
	UINT32 seq;
	UINT32 chs;
	UINT32 crc;
	UINT64 l_sec;
	ULONG l_nsec;
};

struct STP_PACKET_T {
	struct STP_DBG_HDR_T hdr;
	UINT8 raw[STP_DMP_SZ];
};

struct MTKSTP_DBG_T {
	/*log_sys */
	INT32 pkt_trace_no;
	PVOID btm;
	INT32 is_enable;
	struct MTKSTP_LOG_SYS_T *logsys;
};

/* extern void aed_combo_exception(const int *, int, const int *, int, const char *); */

#define STP_CORE_DUMP_TIMEOUT (1*60*1000)	/* default 1 minutes */
#define STP_OJB_NAME_SZ 20
#define STP_CORE_DUMP_INFO_SZ 500
#define STP_CORE_DUMP_INIT_SIZE 512
enum WCN_COMPRESS_ALG_T {
	GZIP = 0,
	BZIP2 = 1,
	RAR = 2,
	LMA = 3,
	MAX
};

typedef INT32 (*COMPRESS_HANDLER) (PVOID worker, UINT8 *in_buf, INT32 in_sz, PUINT8 out_buf, PINT32 out_sz,
				  INT32 finish);
struct WCN_COMPRESSOR_T {
	/* current object name */
	UINT8 name[STP_OJB_NAME_SZ + 1];

	/* buffer for raw data, named L1 */
	PUINT8 L1_buf;
	INT32 L1_buf_sz;
	INT32 L1_pos;

	/* target buffer, named L2 */
	PUINT8 L2_buf;
	INT32 L2_buf_sz;
	INT32 L2_pos;

	/* compress state */
	UINT8 f_done;
	UINT16 reserved;
	UINT32 uncomp_size;
	UINT32 crc32;

	/* compress algorithm */
	UINT8 f_compress_en;
	enum WCN_COMPRESS_ALG_T compress_type;
	PVOID worker;
	COMPRESS_HANDLER handler;
};

enum CORE_DUMP_STA {
	CORE_DUMP_INIT = 0,
	CORE_DUMP_DOING,
	CORE_DUMP_TIMEOUT,
	CORE_DUMP_DONE,
	CORE_DUMP_MAX
};

struct WCN_CORE_DUMP_T {
	/* compress dump data and buffered */
	struct WCN_COMPRESSOR_T *compressor;

	/* timer for monitor timeout */
	struct OSAL_TIMER dmp_timer;
	UINT32 timeout;
	LONG dmp_num;
	UINT32 count;
	struct OSAL_SLEEPABLE_LOCK dmp_lock;

	/* state machine for core dump flow */
	enum CORE_DUMP_STA sm;

	/* dump info */
	INT8 info[STP_CORE_DUMP_INFO_SZ + 1];

	PUINT8 p_head;
	UINT32 head_len;
};

enum ENUM_STP_FW_ISSUE_TYPE {
	STP_FW_ISSUE_TYPE_INVALID = 0x0,
	STP_FW_ASSERT_ISSUE = 0x1,
	STP_FW_NOACK_ISSUE = 0x2,
	STP_FW_WARM_RST_ISSUE = 0x3,
	STP_DBG_PROC_TEST = 0x4,
	STP_HOST_TRIGGER_FW_ASSERT = 0x5,
	STP_HOST_TRIGGER_ASSERT_TIMEOUT = 0x6,
	STP_FW_ABT = 0x7,
	STP_FW_ISSUE_TYPE_MAX
};

/* this was added for support dmareg's issue */
enum ENUM_DMA_ISSUE_TYPE {
	CONNSYS_CLK_GATE_STATUS = 0x00,
	CONSYS_EMI_STATUS,
	SYSRAM1,
	SYSRAM2,
	SYSRAM3,
	DMA_REGS_MAX
};
#define STP_PATCH_TIME_SIZE 12
#define STP_DBG_CPUPCR_NUM 512
#define STP_DBG_DMAREGS_NUM 16
#define STP_PATCH_BRANCH_SZIE 8
#define STP_ASSERT_INFO_SIZE 164
#define STP_DBG_ROM_VER_SIZE 4
#define STP_ASSERT_TYPE_SIZE 32

struct STP_DBG_HOST_ASSERT_T {
	UINT32 drv_type;
	UINT32 reason;
	UINT32 assert_from_host;
};

struct STP_DBG_CPUPCR_T {
	UINT32 chipId;
	UINT8 romVer[STP_DBG_ROM_VER_SIZE];
	UINT8 patchVer[STP_PATCH_TIME_SIZE];
	UINT8 branchVer[STP_PATCH_BRANCH_SZIE];
	UINT32 wifiVer;
	UINT32 count;
	UINT32 stop_flag;
	UINT32 buffer[STP_DBG_CPUPCR_NUM];
	UINT8 assert_info[STP_ASSERT_INFO_SIZE];
	UINT32 fwTaskId;
	UINT32 fwRrq;
	UINT32 fwIsr;
	struct STP_DBG_HOST_ASSERT_T host_assert_info;
	UINT8 assert_type[STP_ASSERT_TYPE_SIZE];
	enum ENUM_STP_FW_ISSUE_TYPE issue_type;
	struct OSAL_SLEEPABLE_LOCK lock;
};

struct STP_DBG_DMAREGS_T {
	UINT32 count;
	UINT32 dmaIssue[DMA_REGS_MAX][STP_DBG_DMAREGS_NUM];
	struct OSAL_SLEEPABLE_LOCK lock;
};

enum ENUM_ASSERT_INFO_PARSER_TYPE {
	STP_DBG_ASSERT_INFO = 0x0,
	STP_DBG_FW_TASK_ID = 0x1,
	STP_DBG_FW_ISR = 0x2,
	STP_DBG_FW_IRQ = 0x3,
	STP_DBG_ASSERT_TYPE = 0x4,
	STP_DBG_PARSER_TYPE_MAX
};

INT32 stp_dbg_core_dump_deinit_gcoredump(VOID);
INT32 stp_dbg_core_dump_flush(INT32 rst, MTK_WCN_BOOL coredump_is_timeout);
INT32 stp_dbg_core_dump(INT32 dump_sink);
INT32 stp_dbg_trigger_collect_ftrace(PUINT8 pbuf, INT32 len);
#if BTIF_RXD_BE_BLOCKED_DETECT
MTK_WCN_BOOL stp_dbg_is_btif_rxd_be_blocked(VOID);
#endif
INT32 stp_dbg_enable(struct MTKSTP_DBG_T *stp_dbg);
INT32 stp_dbg_disable(struct MTKSTP_DBG_T *stp_dbg);
INT32 stp_dbg_dmp_print(struct MTKSTP_DBG_T *stp_dbg);
INT32 stp_dbg_dmp_out(struct MTKSTP_DBG_T *stp_dbg, PINT8 buf, PINT32 len);

INT32 stp_dbg_dmp_out_ex(PINT8 buf, PINT32 len);
INT32 stp_dbg_log_pkt(struct MTKSTP_DBG_T *stp_dbg, INT32 dbg_type,
		      INT32 type, INT32 ack_no, INT32 seq_no, INT32 crc, INT32 dir, INT32 len,
		      const PUINT8 body);
INT32 stp_dbg_log_ctrl(UINT32 on);
INT32 stp_dbg_aee_send(PUINT8 aucMsg, INT32 len, INT32 cmd);
INT32 stp_dbg_dump_num(LONG dmp_num);
INT32 stp_dbg_nl_send(PINT8 aucMsg, UINT8 cmd, INT32 len);
INT32 stp_dbg_dump_send_retry_handler(PINT8 tmp, INT32 len);
VOID stp_dbg_set_coredump_timer_state(enum CORE_DUMP_STA state);
INT32 stp_dbg_poll_cpupcr(UINT32 times, UINT32 sleep, UINT32 cmd);
INT32 stp_dbg_poll_dmaregs(UINT32 times, UINT32 sleep);
INT32 stp_dbg_poll_cpupcr_ctrl(UINT32 en);
INT32 stp_dbg_set_version_info(UINT32 chipid, PUINT8 pRomVer, PUINT8 pPatchVer, PUINT8 pPatchBrh);
INT32 stp_dbg_set_wifiver(UINT32 wifiver);
INT32 stp_dbg_set_host_assert_info(UINT32 drv_type, UINT32 reason, UINT32 en);
UINT32 stp_dbg_get_host_trigger_assert(VOID);
INT32 stp_dbg_set_fw_info(PUINT8 issue_info, UINT32 len, enum ENUM_STP_FW_ISSUE_TYPE issue_type);
INT32 stp_dbg_cpupcr_infor_format(PPUINT8 buf, PUINT32 str_len);
PUINT8 stp_dbg_id_to_task(UINT32 id);
struct MTKSTP_DBG_T *stp_dbg_init(PVOID btm_half);
INT32 stp_dbg_deinit(struct MTKSTP_DBG_T *stp_dbg);

#endif /* end of _STP_DEBUG_H_ */
