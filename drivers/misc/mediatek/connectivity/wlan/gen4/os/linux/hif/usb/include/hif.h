/*! \file   "hif.h"
*    \brief  Functions for the driver to register bus and setup the IRQ
*
*    Functions for the driver to register bus and setup the IRQ
*/

/*
** 09 30 2015 th3.huang
** [BORA00005104] [MT6632 Wi-Fi] Fix coding style.
** 1 fixed coding style issue by auto tool.
**
** 09 24 2015 litien.chang
** [BORA00005127] MT6632
** [WiFi] usb/sdio/pcie 3 interface integration
**
** 09 22 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** 1. Increase Rx DATA urb count from 2 to 32 to avoid FIFO full.
**
** 09 10 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** 1. Increase USB Tx urb buffer size to 32k
** 2. Increase Tx resource data type from 16 bits to 32 bits
** 3. Add USB urb buffer check function
**
** 09 04 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** 1. Add USB sending command cancel mechanism
** 2. Refine CMD allocation for USB
**
** 09 03 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** Add HIF suffix to CR4 FW
**
** 09 01 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** 1. Reduce memecopy while tx packet with Linux kernel API
** 2. Sync change from //BORA/DEV/develop/MT6632_USB_M2M/wifi_driver CL29543, 29611
**
** 08 17 2015 terry.wu
** 1. Reduce USB vendor request retry count & timeout
** 2. Refine WIFI_CUNC_READY
**
** 08 07 2015 terry.wu
** 1. Remove unused code
**
** 07 24 2015 litien.chang
** [BORA00004481] [MT6632]
** 1. add TC to USB endpoint mapping
** 2. to support 3.11 or higher
**
** 07 16 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** Redesign USB TX aggregation to support multiple URB submitted simultaneously
**
** 07 08 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** 1. Add USB TX aggregation support (CFG_USB_TX_AGG)
** 2. Add USB RX aggregation support (in nicUSBInit())
** 3. Refine USB URB management
**
** 06 23 2015 litien.chang
** [BORA00004481] [MT6632]
** [WIFI] add one more urb for RX data path
**
** 05 29 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** 1. Remove all USB code in SDIO files.
** 2. Move zeroing CMD TXD+header to cmdBufAllocateCmdInfo
**
** 05 19 2015 litien.chang
** [BORA00004481] [MT6632]
** [USB] Add RX data path with URB
**
** 05 11 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** 1. Support sending CMD & receiving EVENT using URB.
** 2. Clean up some code.
**
** 05 07 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** Support N9+CR4 FW download
**
** 05 05 2015 desmond.lin
** [BORA00004078] [USB][HostDriver]
** 1. insmod module OK.
** 2. USB probe OK
** 3. Add USB read/write CR (HAL_MCR_RD/HAL_MCR_WR)
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
 * [WCXRP00000120] [MT6620 Wi-Fi][Driver] Refine linux kernel module to
 * the license of MTK propietary and enable MTK HIF by default
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
**  Refine driver unloading and clean up procedure. Block requests,
**  stop main thread and clean up queued requests, and then stop hw.
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

#include "nic_cmd_event.h"
#include "wlan_typedef.h"
#include "nic_tx.h"

typedef enum _ENUM_USB_END_POINT_T {
	USB_DATA_BULK_OUT_EP4 = 4,
	USB_DATA_BULK_OUT_EP5,
	USB_DATA_BULK_OUT_EP6,
	USB_DATA_BULK_OUT_EP7,
	USB_DATA_BULK_OUT_EP8,
	USB_DATA_BULK_OUT_EP9,
} ENUM_USB_END_POINT_T;

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#if defined(_HIF_USB)
#define HIF_NAME "USB"
#else
#error "No HIF defined!"
#endif
#define HIF_CR4_FWDL_SECTION_NUM         2
#define HIF_IMG_DL_STATUS_PORT_IDX       0
#define HIF_IST_LOOP_COUNT              (4)
#define HIF_IST_TX_THRESHOLD            (1) /* Min msdu count to trigger Tx during INT polling state */

#define HIF_NUM_OF_QM_RX_PKT_NUM        (6144)

#define HIF_TX_BUFF_COUNT_TC0            256
#define HIF_TX_BUFF_COUNT_TC1            256
#define HIF_TX_BUFF_COUNT_TC2            256
#define HIF_TX_BUFF_COUNT_TC3            256
#define HIF_TX_BUFF_COUNT_TC4            256
#define HIF_TX_BUFF_COUNT_TC5            256

#define HIF_TX_RESOURCE_CTRL             0 /* enable/disable TX resource control */

#if CFG_USB_TX_AGG
#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2   0
#define HIF_TX_PAGE_SIZE                 1	/* in unit of bytes */
#else
#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2   11
#define HIF_TX_PAGE_SIZE                 2048	/* in unit of bytes */
#endif

#define USB_CMD_EP_OUT                  (USB_DATA_BULK_OUT_EP8)
#define USB_EVENT_EP_IN                 (0x85)
#define USB_DATA_EP_IN                  (0x84)

#define HIF_TX_INIT_CMD_PORT             USB_CMD_EP_OUT

#ifdef CFG_USB_REQ_TX_DATA_CNT
#define USB_REQ_TX_DATA_CNT             (CFG_USB_REQ_TX_DATA_CNT)	/* platform specific USB_REQ_TX_DATA_CNT */
#else
#if CFG_USB_TX_AGG
#define USB_REQ_TX_DATA_CNT             (32*4)	/* must be >= 2 */
#else
#define USB_REQ_TX_DATA_CNT             (CFG_TX_MAX_PKT_NUM)
#endif
#endif

#define USB_REQ_TX_CMD_CNT              (CFG_TX_MAX_CMD_PKT_NUM)
#define USB_REQ_RX_EVENT_CNT            (1)
#define USB_REQ_RX_DATA_CNT             (32)

#define USB_RX_AGGREGTAION_LIMIT        (15)	/* Unit: K-bytes */
#define USB_RX_AGGREGTAION_TIMEOUT      (100)	/* Unit: us */
#define USB_RX_AGGREGTAION_PKT_LIMIT    (10)

#define USB_TX_CMD_BUF_SIZE             (1600)
#if CFG_USB_TX_AGG && 0
extern const UINT_32 USB_TX_DATA_BUF_SIZE[TC_NUM];
#else
/* #define USB_TX_DATA_BUF_SIZE            (NIC_TX_DESC_AND_PADDING_LENGTH + NIC_TX_DESC_HEADER_PADDING_LENGTH +
 *					    NIC_TX_MAX_SIZE_PER_FRAME + LEN_USB_UDMA_TX_TERMINATOR)
 */
#define USB_TX_DATA_BUFF_SIZE			(32768/4)
#endif
#define USB_RX_EVENT_BUF_SIZE           max(USB_RX_AGGREGTAION_LIMIT * 1024, \
					    USB_RX_AGGREGTAION_PKT_LIMIT * \
						(CFG_RX_MAX_PKT_SIZE + 3 + LEN_USB_RX_PADDING_CSO) + 4)
#define USB_RX_DATA_BUF_SIZE            max(USB_RX_AGGREGTAION_LIMIT * 1024, \
					    USB_RX_AGGREGTAION_PKT_LIMIT * \
						(CFG_RX_MAX_PKT_SIZE + 3 + LEN_USB_RX_PADDING_CSO) + 4)

#define LEN_USB_UDMA_TX_TERMINATOR      (4)	/*HW design spec */
#define LEN_USB_RX_PADDING_CSO          (4)	/*HW design spec */

#define USB_RX_EVENT_RFB_RSV_CNT        (0)
#define USB_RX_DATA_RFB_RSV_CNT         (4)

#define DEVICE_VENDOR_REQUEST_IN        (0xc0)
#define DEVICE_VENDOR_REQUEST_OUT       (0x40)
#define VENDOR_TIMEOUT_MS               (2000)
#define BULK_TIMEOUT_MS                 (15000)
#define SW_RFB_TIMEOUT_MS				(3000)

/* Vendor Request */
#define VND_REQ_POWER_ON_WIFI           (0x4)
#define VND_REQ_REG_READ                (0x63)
#define VND_REQ_REG_WRITE               (0x66)

#define USB_TX_CMD_QUEUE_MASK           (BITS(2, 4))   /* For H2CDMA Tx CMD mapping */

#define USB_DBDC1_TC                    (TC_NUM)/* for DBDC1 */
#define USB_TC_NUM                      (TC_NUM + 1)/* for DBDC1 */

#define HIF_EXTRA_IO_BUFFER_SIZE        (0)

#define HIF_TX_COALESCING_BUFFER_SIZE   (USB_TX_CMD_BUF_SIZE)
#define HIF_RX_COALESCING_BUFFER_SIZE   (USB_RX_EVENT_BUF_SIZE)

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/* host interface's private data structure, which is attached to os glue
** layer info structure.
 */

enum usb_state {
	USB_STATE_LINK_DOWN,
	USB_STATE_LINK_UP,
	USB_STATE_PRE_SUSPEND_DONE,
	USB_STATE_PRE_SUSPEND_FAIL,
	USB_STATE_SUSPEND,
	USB_STATE_PRE_RESUME,
	USB_STATE_WIFI_OFF /* Hif power off wifi */
};

typedef struct _BUF_CTRL_T {
	UINT_8 *pucBuf;
	UINT_32 u4BufSize;
	UINT_32 u4WrIdx;
	UINT_32 u4ReadSize;
} BUF_CTRL_T, *P_BUF_CTRL_T;

typedef struct _GL_HIF_INFO_T {
	struct usb_interface *intf;
	struct usb_device *udev;

	P_GLUE_INFO_T prGlueInfo;
	enum usb_state state;

	spinlock_t rQLock;
	PVOID prTxCmdReqHead;
	PVOID arTxDataReqHead[USB_TC_NUM];
	PVOID prRxEventReqHead;
	PVOID prRxDataReqHead;
	struct list_head rTxCmdFreeQ;
	struct list_head rTxCmdSendingQ;
#if CFG_USB_TX_AGG
	struct list_head rTxDataFreeQ[USB_TC_NUM];
	struct list_head rTxDataSendingQ[USB_TC_NUM];
#else
	struct list_head rTxDataFreeQ;
#endif
	struct list_head rRxEventFreeQ;
	struct list_head rRxEventRunningQ;
	struct list_head rRxDataFreeQ;
	struct list_head rRxDataRunningQ;
#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	struct list_head rRxEventCompleteQ;
	struct list_head rRxDataCompleteQ;
#endif
#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	struct list_head rTxCmdCompleteQ;
	struct list_head rTxDataCompleteQ;
#endif

	BUF_CTRL_T rTxCmdBufCtrl[USB_REQ_TX_CMD_CNT];
#if CFG_USB_TX_AGG
	BUF_CTRL_T rTxDataBufCtrl[USB_TC_NUM][USB_REQ_TX_DATA_CNT];
#else
	BUF_CTRL_T rTxDataBufCtrl[USB_REQ_TX_DATA_CNT];
#endif
	BUF_CTRL_T rRxEventBufCtrl[USB_REQ_RX_EVENT_CNT];
	BUF_CTRL_T rRxDataBufCtrl[USB_REQ_RX_DATA_CNT];

	struct mutex vendor_req_sem;
	BOOLEAN fgIntReadClear;
	BOOLEAN fgMbxReadClear;
} GL_HIF_INFO_T, *P_GL_HIF_INFO_T;

typedef struct _USB_REQ_T {
	struct list_head list;
	struct urb *prUrb;
	P_BUF_CTRL_T prBufCtrl;
	P_GL_HIF_INFO_T prHifInfo;
	PVOID prPriv;
	QUE_T rSendingDataMsduInfoList;
} USB_REQ_T, *P_USB_REQ_T;

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

#define USB_TRANS_MSDU_TC(_prMsduInfo) ((_prMsduInfo)->ucWmmQueSet ? USB_DBDC1_TC:(_prMsduInfo)->ucTC)

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

BOOL mtk_usb_vendor_request(IN P_GLUE_INFO_T prGlueInfo, IN UCHAR uEndpointAddress, IN UCHAR RequestType,
			    IN UCHAR Request, IN UINT_16 Value, IN UINT_16 Index, IN PVOID TransferBuffer,
			    IN UINT_32 TransferBufferLength);

VOID glUsbEnqueueReq(P_GL_HIF_INFO_T prHifInfo, struct list_head *prHead, P_USB_REQ_T prUsbReq, BOOLEAN fgHead);
P_USB_REQ_T glUsbDequeueReq(P_GL_HIF_INFO_T prHifInfo, struct list_head *prHead);

WLAN_STATUS halTxUSBSendCmd(IN P_GLUE_INFO_T prGlueInfo, IN UINT_8 ucTc, IN P_CMD_INFO_T prCmdInfo);
VOID halTxUSBSendCmdComplete(struct urb *urb);
VOID halTxUSBProcessCmdComplete(IN P_ADAPTER_T prAdapter, P_USB_REQ_T prUsbReq);

WLAN_STATUS halTxUSBSendData(IN P_GLUE_INFO_T prGlueInfo, IN P_MSDU_INFO_T prMsduInfo);
WLAN_STATUS halTxUSBKickData(IN P_GLUE_INFO_T prGlueInfo);
VOID halTxUSBSendDataComplete(struct urb *urb);
VOID halTxUSBProcessDataComplete(IN P_ADAPTER_T prAdapter, P_USB_REQ_T prUsbReq);

UINT_32 halRxUSBEnqueueRFB(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucBuf, IN UINT_32 u4Length,
	IN UINT_32 u4MinRfbCnt);
WLAN_STATUS halRxUSBReceiveEvent(IN P_ADAPTER_T prAdapter);
VOID halRxUSBReceiveEventComplete(struct urb *urb);
WLAN_STATUS halRxUSBReceiveData(IN P_ADAPTER_T prAdapter);
VOID halRxUSBReceiveDataComplete(struct urb *urb);

VOID halUSBPreSuspendCmd(IN P_ADAPTER_T prAdapter);
VOID halUSBPreSuspendDone(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf);
VOID halUSBPreSuspendTimeout(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo);

void glGetDev(PVOID ctx, struct device **dev);
void glGetHifDev(P_GL_HIF_INFO_T prHif, struct device **dev);

VOID halGetCompleteStatus(IN P_ADAPTER_T prAdapter, OUT PUINT_32 pu4IntStatus);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#endif /* _HIF_H */
