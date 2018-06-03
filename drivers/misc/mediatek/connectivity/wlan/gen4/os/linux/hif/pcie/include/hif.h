/*! \file   "hif.h"
*    \brief  Functions for the driver to register bus and setup the IRQ
*
*    Functions for the driver to register bus and setup the IRQ
*/

/*
**
** 09 30 2015 th3.huang
** [BORA00005104] [MT6632 Wi-Fi] Fix coding style.
** 1 fixed coding style issue by auto tool.
**
** 09 24 2015 litien.chang
** [BORA00005127] MT6632
** [WiFi] usb/sdio/pcie 3 interface integration
**
** 09 08 2015 terry.wu
** [BORA00004513] [MT6632 Wi-Fi] Development
** 1. Fix build error for SDIO/PCIE
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
 *					  MTK propietary and enable MTK HIF by default
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

#include "hif_pci.h"

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
#if defined(_HIF_PCIE)
#define HIF_NAME "PCIE"
#else
#error "No HIF defined!"
#endif

#define HIF_TX_PREALLOC_DATA_BUFFER			1

#define HIF_NUM_OF_QM_RX_PKT_NUM			4096
#define HIF_IST_LOOP_COUNT					32
#define HIF_IST_TX_THRESHOLD				1 /* Min msdu count to trigger Tx during INT polling state */

#define HIF_TX_BUFF_COUNT_TC0				4096
#define HIF_TX_BUFF_COUNT_TC1				4096
#define HIF_TX_BUFF_COUNT_TC2				4096
#define HIF_TX_BUFF_COUNT_TC3				4096
#define HIF_TX_BUFF_COUNT_TC4				(TX_RING_SIZE - 1)
#define HIF_TX_BUFF_COUNT_TC5				4096

#define HIF_TX_RESOURCE_CTRL                1 /* enable/disable TX resource control */

#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2		11
#define HIF_TX_PAGE_SIZE					2048	/* in unit of bytes */

#define HIF_EXTRA_IO_BUFFER_SIZE			0

#define HIF_TX_COALESCING_BUFFER_SIZE		(TX_BUFFER_NORMSIZE)
#define HIF_RX_COALESCING_BUFFER_SIZE		(RX_BUFFER_AGGRESIZE)

#define HIF_CR4_FWDL_SECTION_NUM			1
#define HIF_IMG_DL_STATUS_PORT_IDX			1

#define HIF_TX_INIT_CMD_PORT				TX_RING_FWDL_IDX_3

#define HIF_TX_MSDU_TOKEN_NUM				8192

#define HIF_TX_PAYLOAD_LENGTH				72
#define HIF_TX_DESC_PAYLOAD_LENGTH			(NIC_TX_HEAD_ROOM + HIF_TX_PAYLOAD_LENGTH)

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef struct _MSDU_TOKEN_ENTRY_T {
	UINT_32 u4Token;
	BOOLEAN fgInUsed;
	P_MSDU_INFO_T prMsduInfo;
	P_NATIVE_PACKET prPacket;
	dma_addr_t rDmaAddr;
	UINT_32 u4DmaLength;
	dma_addr_t rPktDmaAddr;
	UINT_32 u4PktDmaLength;
}	MSDU_TOKEN_ENTRY_T, *P_MSDU_TOKEN_ENTRY_T;

typedef struct _MSDU_TOKEN_INFO_T {
	INT_32 i4UsedCnt;
	P_MSDU_TOKEN_ENTRY_T aprTokenStack[HIF_TX_MSDU_TOKEN_NUM];
	spinlock_t rTokenLock;

	MSDU_TOKEN_ENTRY_T arToken[HIF_TX_MSDU_TOKEN_NUM];
} MSDU_TOKEN_INFO_T, *P_MSDU_TOKEN_INFO_T;

/* host interface's private data structure, which is attached to os glue
** layer info structure.
 */
typedef struct _GL_HIF_INFO_T {
	struct pci_dev *pdev;

	PUCHAR CSRBaseAddress;	/* PCI MMIO Base Address, all access will use */

	/* Shared memory of all 1st pre-allocated
	 * TxBuf associated with each TXD
	 */
	RTMP_DMABUF TxBufSpace[NUM_OF_TX_RING];
	RTMP_DMABUF TxDescRing[NUM_OF_TX_RING];	/* Shared memory for Tx descriptors */
	RTMP_TX_RING TxRing[NUM_OF_TX_RING];	/* AC0~3 + HCCA */
	spinlock_t TxRingLock[NUM_OF_TX_RING];	/* Rx Ring spinlock */

	RTMP_DMABUF RxDescRing[2];	/* Shared memory for RX descriptors */
	RTMP_RX_RING RxRing[NUM_OF_RX_RING];
	spinlock_t RxRingLock[NUM_OF_RX_RING];	/* Rx Ring spinlock */

	BOOLEAN fgIntReadClear;
	BOOLEAN fgMbxReadClear;

	UINT_32 u4IntStatus;

	MSDU_TOKEN_INFO_T rTokenInfo;

	spinlock_t rDynMapRegLock;
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

VOID halWpdmaAllocRing(P_GLUE_INFO_T prGlueInfo);
VOID halWpdmaFreeRing(P_GLUE_INFO_T prGlueInfo);
VOID halWpdmaInitRing(P_GLUE_INFO_T prGlueInfo);
VOID halWpdmaProcessCmdDmaDone(IN P_GLUE_INFO_T prGlueInfo, IN UINT_16 u2Port);
VOID halWpdmaProcessDataDmaDone(IN P_GLUE_INFO_T prGlueInfo, IN UINT_16 u2Port);
UINT_32 halWpdmaGetRxDmaDoneCnt(IN P_GLUE_INFO_T prGlueInfo, IN UINT_8 ucRingNum);
VOID kalPciUnmapToDev(IN P_GLUE_INFO_T prGlueInfo, IN dma_addr_t rDmaAddr, IN UINT_32 u4Length);

VOID halInitMsduTokenInfo(IN P_ADAPTER_T prAdapter);
VOID halUninitMsduTokenInfo(IN P_ADAPTER_T prAdapter);
UINT_32 halGetMsduTokenFreeCnt(IN P_ADAPTER_T prAdapter);
P_MSDU_TOKEN_ENTRY_T halGetMsduTokenEntry(IN P_ADAPTER_T prAdapter, UINT_32 u4TokenNum);
P_MSDU_TOKEN_ENTRY_T halAcquireMsduToken(IN P_ADAPTER_T prAdapter);
VOID halReturnMsduToken(IN P_ADAPTER_T prAdapter, UINT_32 u4TokenNum);

VOID halTxUpdateCutThroughDesc(P_GLUE_INFO_T prGlueInfo, P_MSDU_INFO_T prMsduInfo,
	P_MSDU_TOKEN_ENTRY_T prToken);
BOOLEAN halIsStaticMapBusAddr(IN UINT_32 u4Addr);
BOOLEAN halChipToStaticMapBusAddr(IN UINT_32 u4ChipAddr, OUT PUINT_32 pu4BusAddr);
BOOLEAN halGetDynamicMapReg(IN P_GL_HIF_INFO_T prHifInfo, IN UINT_32 u4ChipAddr, OUT PUINT_32 pu4Value);
BOOLEAN halSetDynamicMapReg(IN P_GL_HIF_INFO_T prHifInfo, IN UINT_32 u4ChipAddr, IN UINT_32 u4Value);
VOID halEnhancedWpdmaConfig(P_GLUE_INFO_T prGlueInfo, BOOLEAN enable);
VOID halWpdmaConfig(P_GLUE_INFO_T prGlueInfo, BOOLEAN enable);

BOOL kalDevReadData(IN P_GLUE_INFO_T prGlueInfo, IN UINT_16 u2Port, IN OUT P_SW_RFB_T prSwRfb);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#endif /* _HIF_H */
