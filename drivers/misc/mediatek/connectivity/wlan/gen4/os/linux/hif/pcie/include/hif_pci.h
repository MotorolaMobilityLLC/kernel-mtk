/*
 ***************************************************************************
 * MediaTek Inc.
 *
 * All rights reserved. source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek, Inc. is obtained.
 ***************************************************************************

 Module Name:
 hif_pci.h
 */

#ifndef __HIF_PCI_H__
#define __HIF_PCI_H__

#define NUM_OF_TX_RING				4
#define NUM_OF_RX_RING				2

#define TX_DMA_1ST_BUFFER_SIZE		128	/* only the 1st physical buffer is pre-allocated */

#define TX_RING_SIZE				4095
#define RX_RING_SIZE				256	/* Max Rx ring size */

#define RX_RING0_SIZE				256	/* Data Rx ring */
#define RX_RING1_SIZE				16	/* Event/MSDU_report Rx ring */

#define TXD_SIZE					16	/* TXD_SIZE = TxD + TxInfo */
#define RXD_SIZE					16

#define RX_BUFFER_AGGRESIZE			3840
#define RX_BUFFER_NORMSIZE			3840
#define TX_BUFFER_NORMSIZE			3840

/* Client context provided by hif_pci driver for the following function call */
typedef UINT_32 MTK_HIF_PCI_CLTCTX;

#define PCICLTCTX_CID(ctx) (((ctx) >> 16) & 0xFFFF)

/* Function info provided by client driver */
typedef struct _MTK_HIF_PCI_FUNCINFO {
	UINT_16 vendor_id;	/* Vendor ID */
	UINT_16 device_id;	/* Device ID */
	UINT_16 func_num;	/* Function Number */
} MTK_HIF_PCI_FUNCINFO;


/* New device inserted */
typedef INT_32 (*MTK_HIF_PCI_PROBE)(MTK_HIF_PCI_CLTCTX ctx, struct pci_dev *dev);
/* Device removed (NULL if not a hot-plug capable driver) */
typedef VOID (*MTK_HIF_PCI_REMOVE)(MTK_HIF_PCI_CLTCTX ctx, struct pci_dev *dev);
/* Device suspended */
typedef INT_32 (*MTK_HIF_PCI_SUSPEND)(MTK_HIF_PCI_CLTCTX ctx, struct pci_dev *dev, pm_message_t state);
/* Device woken up */
typedef INT_32 (*MTK_HIF_PCI_RESUME)(MTK_HIF_PCI_CLTCTX ctx, struct pci_dev *dev);
/* Device irq handler */
typedef irqreturn_t (*MTK_HIF_PCI_IRQ)(MTK_HIF_PCI_CLTCTX ctx, INT_32 irq, struct pci_dev *dev);


/* Client info provided by client driver */
typedef struct _MTK_HIF_PCI_CLTINFO {
	const MTK_HIF_PCI_FUNCINFO *func_tbl;	/* supported function info table */
	UINT_32 func_tbl_size;			/* supported function table info element number */
	MTK_HIF_PCI_PROBE hif_clt_probe;	/* callback function for probing */
	MTK_HIF_PCI_REMOVE hif_clt_remove;	/* callback function for removing */
	MTK_HIF_PCI_SUSPEND hif_clt_suspend;	/* callback function for suspend */
	MTK_HIF_PCI_RESUME hif_clt_resume;	/* callback function for resume */
	MTK_HIF_PCI_IRQ hif_clt_irq;		/* callback function for irq */
} MTK_HIF_PCI_CLTINFO;



#define INC_RING_INDEX(_idx, _RingSize) \
{ \
	(_idx) = (_idx+1) % (_RingSize); \
}

#define RTMP_IO_READ32(_A, _R, _pV) \
{ \
	(*(_pV) = readl((void *)((_A)->CSRBaseAddress + (_R)))); \
}

#define RTMP_IO_WRITE32(_A, _R, _V) \
{ \
	writel(_V, (void *)((_A)->CSRBaseAddress + (_R))); \
}

#define MAX_PCIE_BUS_STATIC_MAP_ADDR		0x00040000

#define MT_RINGREG_DIFF		0x10

#define MT_TX_RING_BASE		WPDMA_TX_RING0_CTRL0
#define MT_TX_RING_PTR		WPDMA_TX_RING0_CTRL0
#define MT_TX_RING_CNT		WPDMA_TX_RING0_CTRL1
#define MT_TX_RING_CIDX		WPDMA_TX_RING0_CTRL2
#define MT_TX_RING_DIDX		WPDMA_TX_RING0_CTRL3

#define MT_RX_RING_BASE		WPDMA_RX_RING0_CTRL0
#define MT_RX_RING_PTR		WPDMA_RX_RING0_CTRL0
#define MT_RX_RING_CNT		WPDMA_RX_RING0_CTRL1
#define MT_RX_RING_CIDX		WPDMA_RX_RING0_CTRL2
#define MT_RX_RING_DIDX		WPDMA_RX_RING0_CTRL3

typedef enum _ENUM_TX_RING_IDX_T {
	TX_RING_DATA0_IDX_0 = 0,
	TX_RING_DATA1_IDX_1,
	TX_RING_CMD_IDX_2,
	TX_RING_FWDL_IDX_3,
} ENUM_TX_RING_IDX__T;

typedef enum _ENUM_RX_RING_IDX_T {
	RX_RING_DATA_IDX_0 = 0,
	RX_RING_EVT_IDX_1
} ENUM_RX_RING_IDX_T;

/* ============================================================================
*   PCI/RBUS TX / RX Frame Descriptors format
*
*   Memory Layout
*
*   1. Tx Descriptor
*   TxD (12 bytes) + TXINFO (4 bytes)
*   2. Packet Buffer
*   TXWI + 802.11
*   31                                                                         0
*   +--------------------------------------------------------------------------+
*   |                                   SDP0[31:0]                             |
*   +-+--+---------------------+-+--+------------------------------------------+
*   |D |L0|       SDL0[13:0]              |B|L1|                    SDL1[13:0] |
*   +-+--+---------------------+-+--+------------------------------------------+
*   |                                   SDP1[31:0]                             |
*   +--------------------------------------------------------------------------+
*   |                                        TXINFO                            |
*   +--------------------------------------------------------------------------+
*   =========================================================================
*/
/*
 *  TX descriptor format for Tx Data/Mgmt Rings
 */
typedef struct _TXD_STRUCT {
	/* Word 0 */
	UINT_32 SDPtr0;

	/* Word 1 */
	UINT_32 SDLen1:14;
	UINT_32 LastSec1:1;
	UINT_32 Burst:1;
	UINT_32 SDLen0:14;
	UINT_32 LastSec0:1;
	UINT_32 DMADONE:1;
	/*Word2 */
	UINT_32 SDPtr1;
} TXD_STRUCT;

/*
 *  Rx descriptor format for Rx Rings
 */
typedef struct _RXD_STRUCT {
	/* Word 0 */
	UINT_32 SDP0;

	/* Word 1 */
	UINT_32 SDL1:14;
	UINT_32 LS1:1;
	UINT_32 BURST:1;
	UINT_32 SDL0:14;
	UINT_32 LS0:1;
	UINT_32 DDONE:1;

	/* Word 2 */
	UINT_32 SDP1;
} RXD_STRUCT;

/*
*	Data buffer for DMA operation, the buffer must be contiguous physical memory
*	Both DMA to / from CPU use the same structure.
*/
typedef struct _RTMP_DMABUF {
	ULONG AllocSize;
	PVOID AllocVa;		/* TxBuf virtual address */
	dma_addr_t AllocPa;	/* TxBuf physical address */
} RTMP_DMABUF, *PRTMP_DMABUF;

/*
*	Control block (Descriptor) for all ring descriptor DMA operation, buffer must be
*	contiguous physical memory. NDIS_PACKET stored the binding Rx packet descriptor
*	which won't be released, driver has to wait until upper layer return the packet
*	before giveing up this rx ring descriptor to ASIC. NDIS_BUFFER is assocaited pair
*	to describe the packet buffer. For Tx, NDIS_PACKET stored the tx packet descriptor
*	which driver should ACK upper layer when the tx is physically done or failed.
*/
typedef struct _RTMP_DMACB {
	ULONG AllocSize;	/* Control block size */
	PVOID AllocVa;		/* Control block virtual address */
	dma_addr_t AllocPa;	/* Control block physical address */
	PVOID pPacket;
	PVOID pBuffer;
	dma_addr_t PacketPa;
	RTMP_DMABUF DmaBuf;	/* Associated DMA buffer structure */
} RTMP_DMACB, *PRTMP_DMACB;

typedef struct _RTMP_TX_RING {
	RTMP_DMACB Cell[TX_RING_SIZE];
	UINT_32 TxCpuIdx;
	UINT_32 TxDmaIdx;
	UINT_32 TxSwUsedIdx;
	UINT_32 u4UsedCnt;
	UINT_32 hw_desc_base;
	UINT_32 hw_cidx_addr;
	UINT_32 hw_didx_addr;
	UINT_32 hw_cnt_addr;
} RTMP_TX_RING, *P_RTMP_TX_RING;

typedef struct _RTMP_RX_RING {
	RTMP_DMACB Cell[RX_RING_SIZE];
	UINT_32 RxCpuIdx;
	UINT_32 RxDmaIdx;
	INT_32 RxSwReadIdx;	/* software next read index */
	UINT_32 u4BufSize;
	UINT_32 u4RingSize;
	BOOLEAN fgRxSegPkt;

	UINT_32 hw_desc_base;
	UINT_32 hw_cidx_addr;
	UINT_32 hw_didx_addr;
	UINT_32 hw_cnt_addr;
} RTMP_RX_RING, *P_RTMP_RX_RING;

typedef struct _PCIE_CHIP_CR_MAPPING {
	UINT_32 u4ChipAddr;
	UINT_32 u4BusAddr;
	UINT_32 u4Range;
} PCIE_CHIP_CR_MAPPING, *P_PCIE_CHIP_CR_MAPPING;

extern PVOID mtk_hif_pci_get_drvdata(MTK_HIF_PCI_CLTCTX ctx);
extern VOID mtk_hif_pci_set_drvdata(MTK_HIF_PCI_CLTCTX ctx, PVOID private_data_p);
extern INT_32 mtk_hif_pci_client_reg(const MTK_HIF_PCI_CLTINFO *pinfo);
extern INT_32 mtk_hif_pci_client_unreg(const MTK_HIF_PCI_CLTINFO *pinfo);
#endif /* __HIF_PCI_H__ */
