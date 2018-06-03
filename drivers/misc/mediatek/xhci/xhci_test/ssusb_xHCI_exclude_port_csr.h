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

#ifndef __SSUSB_XHCI_EXCLUDE_PORT_CSR__
#define __SSUSB_XHCI_EXCLUDE_PORT_CSR__

#define HSRAM_DBGCTL                    0x900
#define HSRAM_DBGMODE                   0x904
#define HSRAM_DBGSEL                    0x908
#define HSRAM_DBGADR                    0x90c
#define HSRAM_DBGDR                     0x910
#define HSRAM_DELSEL_0                  0x920
#define HSRAM_DELSEL_1                  0x924
#define AXI_ID                          0x92c
#define LS_EOF                          0x930
#define FS_EOF                          0x934
#define SYNC_HS_EOF                     0x938
#define SS_EOF                          0x93c
#define SOF_OFFSET                      0x940
#define HFCNTR_CFG                      0x944
#define XACT3_CFG                       0x948
#define XACT2_CFG                       0x94c
#define HDMA_CFG                        0x950
#define ASYNC_HS_EOF                    0x954
#define AXI_WR_DMA_CFG                  0x958
#define AXI_RD_DMA_CFG                  0x95c
#define HSCH_CFG1                       0x960
#define CMD_CFG                         0x964
#define EP_CFG                          0x968
#define EVT_CFG                         0x96c
#define TRBQ_CFG                        0x970
#define U3PORT_CFG                      0x974
#define U2PORT_CFG                      0x978
#define HSCH_CFG2                       0x97c
#define SW_ERDY                         0x980
#define SLOT_EP_STS0                    0x9a0
#define SLOT_EP_STS1                    0x9a4
#define SLOT_EP_STS2                    0x9a8
#define RST_CTRL0                       0x9b0
#define RST_CTRL1                       0x9b4
#define SPARE0                          0x9f0
#define SPARE1                          0x9f4

/* __SSUSB_XHCI_EXCLUDE_PORT_CSR__FIELD DEFINITON */

/*  HSRAM_DBGCTL                      */
#define SRAM_DBG_WP                     (0x1<<1)
#define SRAM_DBG_EN                     (0x1<<0)

/*  HSRAM_DBGMODE                     */
#define SRAM_DBG_MODE                   (0xffffffff<<0)

/*  HSRAM_DBGSEL                      */
#define SRAM_DBG_SEL                    (0xffffffff<<0)

/*  HSRAM_DBGADR                      */
#define SRAM_DBG_DWSEL                  (0x7<<0)
#define SRAM_DBG_ADDR                   (0x3fff<<3)

/*  HSRAM_DBGDR                       */
#define SRAM_DBG_DATA                   (0xffffffff<<0)

/*  HSRAM_DELSEL_0                    */
#define DELSEL_EP_DYN                   (0x3<<10)
#define DELSEL_EP_SLT                   (0x3<<6)
#define DELSEL_EP_MAP                   (0x3<<4)
#define DELSEL_EP_STA                   (0x3<<8)
#define DELSEL_ER                       (0x3<<0)
#define DELSEL_U2DMA_0                  (0x3<<14)
#define DELSEL_TRBQ                     (0x3<<12)
#define DELSEL_CMD                      (0x3<<2)

/*  HSRAM_DELSEL_1                    */
#define DELSEL_SCH3_OUT                 (0x3<<2)
#define DELSEL_SCH2_3                   (0x3<<10)
#define DELSEL_SCH2_0                   (0x3<<4)
#define DELSEL_SCH2_2                   (0x3<<8)
#define DELSEL_SCH2_4                   (0x3<<12)
#define DELSEL_SCH3_IN                  (0x3<<0)
#define DELSEL_SCH2_1                   (0x3<<6)

/*  AXI_ID                            */
#define AXI_DMA_MAS_ID                  (0xf<<4)
#define AXI_DMA_DATA_ID                 (0xf<<0)

/*  LS_EOF                            */
#define LS_EOF_OFFSET                   (0x1ff<<0)
#define LS_EOF_BANK                     (0x1f<<16)
#define LS_EOF_UFRAME                   (0x7<<24)

/*  FS_EOF                            */
#define FS_EOF_BANK                     (0x1f<<16)
#define FS_EOF_OFFSET                   (0x1ff<<0)
#define FS_EOF_UFRAME                   (0x7<<24)

/*  SYNC_HS_EOF                       */
#define SYNC_HS_EOF_BANK                (0x1f<<16)
#define SYNC_HS_EOF_OFFSET              (0x1ff<<0)

/*  SS_EOF                            */
#define SS_EOF_BANK                     (0x1f<<16)
#define SS_EOF_OFFSET                   (0x1ff<<0)

/*  SOF_OFFSET                        */
#define SOF_U3_PORT1                    (0xf<<4)
#define SOF_U2_PORT1                    (0xf<<12)
#define SOF_U3_PORT0                    (0xf<<0)
#define SOF_U2_PORT3                    (0xf<<20)
#define SOF_U2_PORT5                    (0xf<<28)
#define SOF_U2_PORT0                    (0xf<<8)
#define SOF_U2_PORT4                    (0xf<<24)
#define SOF_U2_PORT2                    (0xf<<16)

/*  HFCNTR_CFG                        */
#define INIT_FRMCNT_LEV2_FULL_RANGE     (0x1f<<17)
#define SOF_U2_PORT0_H                  (0x1<<26)
#define SOF_U2_PORT4_H                  (0x1<<30)
#define SOF_U2_PORT5_H                  (0x1<<31)
#define SOF_U3_PORT1_H                  (0x1<<25)
#define SOF_U2_PORT2_H                  (0x1<<28)
#define ITP_DELTA_CLK_RATIO             (0x1f<<1)
#define SOF_U2_PORT3_H                  (0x1<<29)
#define SOF_U3_PORT0_H                  (0x1<<24)
#define SOF_U2_PORT1_H                  (0x1<<27)
#define FCNTR_DIS                       (0x1<<0)
#define INIT_FRMCNT_LEV1_FULL_RANGE     (0x1ff<<8)

/*  XACT3_CFG                         */
#define XACT3_PM_TRANS_EN               (0x1<<24)
#define XACT3_ISO_IN_CRC_CHK_DIS        (0x1<<16)
#define XACT3_TMOUT                     (0xff<<0)
#define XACT3_ISO_OUT_TX_ZLP_DIS        (0x1<<17)

/*  XACT2_CFG                         */
#define XACT2_ISO_OUT_TX_ZLP_DIS        (0x1<<2)
#define XACT2_NON_SPLIT_ISO_IN_CRC_CHK_DIS  (0x1<<0)
#define XACT2_SPLIT_ISO_IN_CRC_CHK_DIS  (0x1<<1)

/*  HDMA_CFG                          */
#define DMAR_BURST                      (0x3<<2)
#define DMAW_LAST_NONBUF                (0x1<<15)
#define DMAU2_BUFFERABLE                (0x1<<17)
#define DMAU2_BURST                     (0x3<<18)
#define DMAR_LIMITER                    (0x7<<4)
#define DMAU2_LIMITER                   (0x7<<20)
#define DMAW_LIMITER                    (0x7<<12)
#define DMAW_FAKE                       (0x1<<8)
#define DMAW_BURST                      (0x3<<10)
#define DMAW_BUFFERABLE                 (0x1<<9)
#define DMAU2_FAKE                      (0x1<<16)
#define DMAR_FAKE                       (0x1<<0)
#define DYN_DRAM_CG_EN                  (0x1<<24)

/*  ASYNC_HS_EOF                      */
#define ASYNC_HS_EOF_BANK               (0x1f<<16)
#define ASYNC_HS_EOF_OFFSET             (0x1ff<<0)

/*  AXI_WR_DMA_CFG                    */
#define axi_wr_ultra_en                 (0x1<<16)
#define axi_wr_ultra_num                (0xff<<8)
#define axi_wr_outstand_num             (0xf<<20)
#define axi_wr_pre_ultra_num            (0xff<<0)
#define axi_wr_cacheable                (0x1<<17)
#define axi_wr_coherence                (0x1<<19)
#define axi_wr_iommu                    (0x1<<18)

/*  AXI_RD_DMA_CFG                    */
#define axi_rd_ultra_num                (0xff<<8)
#define axi_rd_cacheable                (0x1<<17)
#define axi_rd_ultra_en                 (0x1<<16)
#define axi_rd_outstand_num             (0xf<<20)
#define axi_rd_pre_ultra_num            (0xff<<0)
#define axi_rd_coherence                (0x1<<19)
#define axi_rd_iommu                    (0x1<<18)

/*  HSCH_CFG1                         */
#define SCH2_INT_PING_TD_CHK            (0x1<<11)
#define SCH3_TX_FIFO_DEPTH              (0x3<<18)
#define SCH2_BULK_PING_TD_CHK           (0x1<<10)
#define SCH_DIS_INT_CS_NYET_RST_BEC     (0x1<<14)
#define OUT_NUMP_REF                    (0x1<<4)
#define SCH3_RX_FIFO_DEPTH              (0x3<<20)
#define SCH_SPLIT_ISO_IN_RETRY_OPT      (0x3<<8)
#define BURST_IN_OFF                    (0x1<<3)
#define OUT_NUMP_REF_DYN                (0x1<<5)
#define SCH3_INT_PING_TD_CHK            (0x1<<12)
#define BURST_OUT_OFF                   (0x1<<1)
#define SCH2_FIFO_DEPTH                 (0x3<<16)
#define SCH_IN_ACK_RTY_EN               (0x1<<7)
#define PORT_IDLE_WITH_INT              (0x1<<13)
#define UPDATE_XACT_NUMP_INTIME         (0x1<<15)
#define SCH_ASYNC_NEXT_FRAME            (0x1<<6)
#define BURST_TD_OFF                    (0x1<<2)
#define PORT_IDLE_WITH_ISO              (0x1<<0)

/*  CMD_CFG                           */
#define PARAM_ERR_CHK_DIS               (0x1<<0)

/*  EP_CFG                            */
#define HUB_TTT_EN                      (0x1<<0)

/*  EVT_CFG                           */
#define INTR_BEI_EN                     (0x1<<0)

/*  TRBQ_CFG                          */
#define TRB_ERR_CHK_DIS                 (0x1<<0)

/*  U3PORT_CFG                        */
#define U3PORT_WRP_BY_HCRST_EN          (0x1<<0)

/*  U2PORT_CFG                        */
#define LPM_L1_EXIT_TIMER               (0xff<<0)

/*  HSCH_CFG2                         */
#define SCH_INT_NAK_ACTIVE_MASK_2P      (0x1<<10)
#define SCH_BULK_NYET_ACTIVE_MASK       (0x1<<0)
#define SCH_INT_NAK_ACTIVE_MASK         (0x1<<8)
#define SCH_INT_NAK_ACTIVE_MASK_1P      (0x1<<9)
#define SCH_BULK_NYET_ACTIVE_MASK_3P    (0x1<<3)
#define SCH_BULK_NYET_ACTIVE_MASK_2P    (0x1<<2)
#define SCH_INT_NAK_ACTIVE_MASK_3P      (0x1<<11)
#define SCH_BULK_NYET_ACTIVE_MASK_1P    (0x1<<1)
#define SCH_INT_NAK_ACTIVE_MASK_4P      (0x1<<12)
#define SCH_BULK_NYET_ACTIVE_MASK_4P    (0x1<<4)

/*  SW_ERDY                           */
#define SW_ERDY_SLOT_ID                 (0xff<<0)
#define SW_ERDY_DCI                     (0x1f<<8)
#define SW_ERDY_CMD                     (0x1<<15)

/*  SLOT_EP_STS0                      */
#define AVAIL_EP_NUM                    (0xff<<0)
#define AVAIL_SLOT_NUM                  (0xff<<8)

/*  SLOT_EP_STS1                      */
#define AVAIL_EP_BITMAP_LO              (0xffffffff<<0)

/*  SLOT_EP_STS2                      */
#define AVAIL_EP_BITMAP_HI              (0xffffffff<<0)

/*  RST_CTRL0                         */
#define HCRST_DRAM_EN                   (0x1<<3)
#define HCRST_SYS60_EN                  (0x1<<2)
#define HCRST_XHCI_EN                   (0x1<<0)
#define HCRST_SYS125_EN                 (0x1<<1)

/*  RST_CTRL1                         */
#define HCRST_U3_PHYD_EN_0P             (0x1<<1)
#define HCRST_U3_MAC_EN_0P              (0x1<<0)
#define HCRST_U2_MAC_EN_3P              (0x1<<22)
#define HCRST_U2_MAC_EN_4P              (0x1<<24)
#define HCRST_U2_MAC_EN_2P              (0x1<<20)
#define HCRST_U2_PHYD_EN_2P             (0x1<<21)
#define HCRST_U2_PHYD_EN_4P             (0x1<<25)
#define HCRST_U2_MAC_EN_0P              (0x1<<16)
#define HCRST_U2_MAC_EN_1P              (0x1<<18)
#define HCRST_U2_PHYD_EN_3P             (0x1<<23)
#define HCRST_U3_MAC_EN_1P              (0x1<<2)
#define HCRST_U2_PHYD_EN_0P             (0x1<<17)
#define HCRST_U3_PHYD_EN_1P             (0x1<<3)
#define HCRST_U2_PHYD_EN_1P             (0x1<<19)

/*  SPARE0                            */
#define SPARE0_FIELD                    (0xffffffff<<0)

/*  SPARE1                            */
#define SPARE1_FIELD                    (0xffffffff<<0)

#endif
