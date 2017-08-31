/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/hif/sdio/include/hif.h#1
*/

/*! \file   "hif.h"
*    \brief  Functions for the driver to register bus and setup the IRQ
*
*    Functions for the driver to register bus and setup the IRQ
*/

/*
** Log: hif.h
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
 *
 * 11 01 2010 yarco.yang
 * [WCXRP00000149] [MT6620 WI-Fi][Driver]Fine tune performance on MT6516 platform
 * Add GPIO debug function
 *
 * 10 19 2010 jeffrey.chang
 * [WCXRP00000120] [MT6620 Wi-Fi][Driver] Refine linux kernel module to the license of
 * MTK propietary and enable MTK HIF by default
 * Refine linux kernel module to the license of MTK and enable MTK HIF
 *
 * 08 18 2010 jeffrey.chang
 * NULL
 * support multi-function sdio
 *
 * 08 17 2010 cp.wu
 * NULL
 * add ENE SDIO host workaround for x86 linux platform.
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 06 06 2010 kevin.huang
 * [WPD00003832][MT6620 5931] Create driver base
 * [MT6620 5931] Create driver base
 *
 * 03 24 2010 jeffrey.chang
 * [WPD00003826]Initial import for Linux port
 * initial import for Linux port
**  \main\maintrunk.MT5921\4 2009-10-20 17:38:28 GMT mtk01090
**  Refine driver unloading and clean up procedure. Block requests, stop main thread and clean up queued requests,
**  and then stop hw.
**  \main\maintrunk.MT5921\3 2009-09-28 20:19:20 GMT mtk01090
**  Add private ioctl to carry OID structures. Restructure public/private ioctl interfaces to Linux kernel.
**  \main\maintrunk.MT5921\2 2009-08-18 22:57:05 GMT mtk01090
**  Add Linux SDIO (with mmc core) support.
**  Add Linux 2.6.21, 2.6.25, 2.6.26.
**  Fix compile warning in Linux.
**  \main\maintrunk.MT5921\2 2008-09-22 23:18:17 GMT mtk01461
**  Update driver for code review
** Revision 1.1  2007/07/05 07:25:33  MTK01461
** Add Linux initial code, modify doc, add 11BB, RF init code
**
** Revision 1.3  2007/06/27 02:18:51  MTK01461
** Update SCAN_FSM, Initial(Can Load Module), Proc(Can do Reg R/W), TX API
**
*/

#ifndef _HIF_H
#define _HIF_H

#if MTK_WCN_HIF_SDIO
#include "hif_sdio.h"
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/
#if MTK_WCN_HIF_SDIO
#define HIF_SDIO_SUPPORT_GPIO_SLEEP_MODE			0
#else
#define HIF_SDIO_SUPPORT_GPIO_SLEEP_MODE			0
#endif

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#if defined(_HIF_SDIO)
#define HIF_NAME "SDIO"
#else
#error "No HIF defined!"
#endif


/* Enable driver timing profiling */
#define CFG_SDIO_TIMING_PROFILING       0

#define SDIO_X86_WORKAROUND_WRITE_MCR   0x00C4
#define HIF_NUM_OF_QM_RX_PKT_NUM        512

#define HIF_TX_INIT_CMD_PORT            TX_RING_FWDL_IDX_3

#define HIF_IST_LOOP_COUNT              128
#define HIF_IST_TX_THRESHOLD            32 /* Min msdu count to trigger Tx during INT polling state */

#define HIF_TX_MAX_AGG_LENGTH           (511 * 512) /* 511 blocks x 512 */

#define HIF_RX_MAX_AGG_NUM              16
/*!< Setting the maximum RX aggregation number 0: no limited (16) */

#define HIF_TX_BUFF_COUNT_TC0           8
#define HIF_TX_BUFF_COUNT_TC1           167
#define HIF_TX_BUFF_COUNT_TC2           8
#define HIF_TX_BUFF_COUNT_TC3           8
#define HIF_TX_BUFF_COUNT_TC4           7
#define HIF_TX_BUFF_COUNT_TC5           0

#define HIF_TX_RESOURCE_CTRL            1 /* enable/disable TX resource control */

#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2      11
#define HIF_TX_PAGE_SIZE                    2048	/* in unit of bytes */

#define HIF_EXTRA_IO_BUFFER_SIZE \
	(sizeof(ENHANCE_MODE_DATA_STRUCT_T) + HIF_RX_COALESCING_BUF_COUNT * HIF_RX_COALESCING_BUFFER_SIZE)

#define HIF_CR4_FWDL_SECTION_NUM            2
#define HIF_IMG_DL_STATUS_PORT_IDX          0

#define HIF_RX_ENHANCE_MODE_PAD_LEN         4

#define HIF_TX_TERMINATOR_LEN               4

#if CFG_SDIO_TX_AGG
#define HIF_TX_COALESCING_BUFFER_SIZE       (HIF_TX_MAX_AGG_LENGTH)
#else
#define HIF_TX_COALESCING_BUFFER_SIZE       (CFG_TX_MAX_PKT_SIZE)
#endif

#if CFG_SDIO_RX_AGG
#define HIF_RX_COALESCING_BUFFER_SIZE       ((HIF_RX_MAX_AGG_NUM  + 1) * CFG_RX_MAX_PKT_SIZE)
#else
#define HIF_RX_COALESCING_BUFFER_SIZE       (CFG_RX_MAX_PKT_SIZE)
#endif

#define HIF_RX_COALESCING_BUF_COUNT         16

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef struct _ENHANCE_MODE_DATA_STRUCT_T SDIO_CTRL_T, *P_SDIO_CTRL_T;

typedef struct _SDIO_STAT_COUNTER_T {
	/* Tx data */
	UINT_32 u4DataPortWriteCnt;
	UINT_32 u4DataPktWriteCnt;
	UINT_32 u4DataPortKickCnt;

	/* Tx command */
	UINT_32 u4CmdPortWriteCnt;
	UINT_32 u4CmdPktWriteCnt;

	/* Tx done interrupt */
	UINT_32 u4TxDoneCnt[16];
	UINT_32 u4TxDoneIntCnt[16];
	UINT_32 u4TxDoneIntTotCnt;
	UINT_32 u4TxDonePendingPktCnt;

	UINT_32 u4IntReadCnt;
	UINT_32 u4IntCnt;

	/* Rx data/cmd*/
	UINT_32 u4PortReadCnt[2];
	UINT_32 u4PktReadCnt[2];

	UINT_32 u4RxBufUnderFlowCnt;

#if CFG_SDIO_TIMING_PROFILING
	UINT_32 u4TxDataCpTime;
	UINT_32 u4TxDataFreeTime;

	UINT_32 u4RxDataCpTime;
	UINT_32 u4PortReadTime;

	UINT_32 u4TxDoneIntTime;
	UINT_32 u4IntReadTime;
#endif
} SDIO_STAT_COUNTER_T, *P_SDIO_STAT_COUNTER_T;

typedef struct _SDIO_RX_COALESCING_BUF_T {
	QUE_ENTRY_T rQueEntry;
	PVOID pvRxCoalescingBuf;
	UINT_32 u4BufSize;
	UINT_32 u4PktCount;
} SDIO_RX_COALESCING_BUF_T, *P_SDIO_RX_COALESCING_BUF_T;

/* host interface's private data structure, which is attached to os glue
** layer info structure.
 */
typedef struct _GL_HIF_INFO_T {
#if MTK_WCN_HIF_SDIO
	MTK_WCN_HIF_SDIO_CLTCTX cltCtx;

	const MTK_WCN_HIF_SDIO_FUNCINFO *prFuncInfo;
#else
	struct sdio_func *func;
#endif

	P_SDIO_CTRL_T prSDIOCtrl;

	BOOLEAN fgIntReadClear;
	BOOLEAN fgMbxReadClear;
	QUE_T rFreeQueue;
	BOOLEAN fgIsPendingInt;

	/* Statistic counter */
	SDIO_STAT_COUNTER_T rStatCounter;

	SDIO_RX_COALESCING_BUF_T rRxCoalesingBuf[HIF_RX_COALESCING_BUF_COUNT];

	QUE_T rRxDeAggQueue;
	QUE_T rRxFreeBufQueue;

	struct mutex rRxFreeBufQueMutex;
	struct mutex rRxDeAggQueMutex;
} GL_HIF_INFO_T, *P_GL_HIF_INFO_T;

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#if CFG_SDIO_TIMING_PROFILING
#define SDIO_TIME_INTERVAL_DEC()			KAL_TIME_INTERVAL_DECLARATION()
#define SDIO_REC_TIME_START()				KAL_REC_TIME_START()
#define SDIO_REC_TIME_END()					KAL_REC_TIME_END()
#define SDIO_GET_TIME_INTERVAL()			KAL_GET_TIME_INTERVAL()
#define SDIO_ADD_TIME_INTERVAL(_Interval)	KAL_ADD_TIME_INTERVAL(_Interval)
#else
#define SDIO_TIME_INTERVAL_DEC()
#define SDIO_REC_TIME_START()
#define SDIO_REC_TIME_END()
#define SDIO_GET_TIME_INTERVAL()
#define SDIO_ADD_TIME_INTERVAL(_Interval)
#endif

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

WLAN_STATUS glRegisterBus(probe_card pfProbe, remove_card pfRemove);

VOID glUnregisterBus(remove_card pfRemove);

VOID glSetHifInfo(P_GLUE_INFO_T prGlueInfo, ULONG ulCookie);

VOID glClearHifInfo(P_GLUE_INFO_T prGlueInfo);

BOOL glBusInit(PVOID pvData);

VOID glBusRelease(PVOID pData);

INT_32 glBusSetIrq(PVOID pvData, PVOID pfnIsr, PVOID pvCookie);

VOID glBusFreeIrq(PVOID pvData, PVOID pvCookie);

VOID glSetPowerState(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 ePowerMode);

void glGetDev(PVOID ctx, struct device **dev);

void glGetHifDev(P_GL_HIF_INFO_T prHif, struct device **dev);

BOOLEAN glWakeupSdio(P_GLUE_INFO_T prGlueInfo);

#if !CFG_SDIO_INTR_ENHANCE
VOID halRxSDIOReceiveRFBs(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxReadBuffer(IN P_ADAPTER_T prAdapter, IN OUT P_SW_RFB_T prSwRfb);

#else
VOID halRxSDIOEnhanceReceiveRFBs(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxEnhanceReadBuffer(IN P_ADAPTER_T prAdapter, IN UINT_32 u4DataPort,
	IN UINT_16 u2RxLength, IN OUT P_SW_RFB_T prSwRfb);

VOID halProcessEnhanceInterruptStatus(IN P_ADAPTER_T prAdapter);

#endif /* CFG_SDIO_INTR_ENHANCE */

#if CFG_SDIO_RX_AGG
VOID halRxSDIOAggReceiveRFBs(IN P_ADAPTER_T prAdapter);
#endif

VOID halPutMailbox(IN P_ADAPTER_T prAdapter, IN UINT_32 u4MailboxNum, IN UINT_32 u4Data);
VOID halGetMailbox(IN P_ADAPTER_T prAdapter, IN UINT_32 u4MailboxNum, OUT PUINT_32 pu4Data);
VOID halDeAggRxPkt(P_ADAPTER_T prAdapter, P_SDIO_RX_COALESCING_BUF_T prRxBuf);
VOID halPollDbgCr(IN P_ADAPTER_T prAdapter, IN UINT_32 u4LoopCount);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#endif /* _HIF_H */
