/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __CLDMA_REG_H__
#define __CLDMA_REG_H__

#include <mt-plat/sync_write.h>
/* INFRA */
#define INFRA_RST0_REG_PD (0x0150) /* rgu reset cldma reg */
#define INFRA_RST1_REG_PD (0x0154) /* rgu clear cldma reset reg */
#define CLDMA_PD_RST_MASK (1 << 2)
#define INFRA_RST0_REG_AO (0x0140)
#define INFRA_RST1_REG_AO (0x0144)
#define CLDMA_AO_RST_MASK (1 << 6)

#define INFRA_AO_MD_SRCCLKENA (0xF0C) /* SRC CLK ENA */
/*===========================CLDMA_AO_INDMA:
 * 10014004-1001404C==================================
 */
/* The start address of first TGPD descriptor for power-down back-up. */
#define CLDMA_AP_UL_START_ADDR_BK_0 (0x0004)
#define CLDMA_AP_UL_START_ADDR_BK_1 (0x0008)
#define CLDMA_AP_UL_START_ADDR_BK_2 (0x000C)
#define CLDMA_AP_UL_START_ADDR_BK_3 (0x0010)
#define CLDMA_AP_UL_START_ADDR_BK_4 (0x0014)
#define CLDMA_AP_UL_START_ADDR_BK_5 (0x0018)
#define CLDMA_AP_UL_START_ADDR_BK_6 (0x001C)
#define CLDMA_AP_UL_START_ADDR_BK_7 (0x0020)
/* The address of current processing TGPD descriptor for power-down back-up. */
#define CLDMA_AP_UL_CURRENT_ADDR_BK_0 (0x0028)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_1 (0x002C)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_2 (0x0030)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_3 (0x0034)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_4 (0x0038)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_5 (0x003C)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_6 (0x0040)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_7 (0x0044)
/*Support >32 bit address access, such as 6GBM*/
#define CLDMA_AP_UL_START_ADDR_BK_4MSB (0x0048)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_4MSB (0x004C)
/*===========================CLDMA_AO_OUTDMA:10014404-1001453C=============*/
#define CLDMA_AP_SO_CFG (0x0404)     /* Operation Configuration */
#define CLDMA_AP_SO_DUMMY_2 (0x0410) /* Dummy Register 2 */
#define CLDMA_AP_SO_DUMMY_3 (0x0414) /* Dummy Register 3 */
#define CLDMA_AP_SO_CHECKSUM_CHANNEL_ENABLE                                    \
	(0x0474) /* Per-channel checksum checking function enable */
#define CLDMA_AP_SO_START_ADDR_0                                               \
	(0x0478) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_1                                               \
	(0x047C) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_2                                               \
	(0x0480) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_3                                               \
	(0x0484) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_4                                               \
	(0x0488) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_5                                               \
	(0x048C) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_6                                               \
	(0x0490) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_7                                               \
	(0x0494) /* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_CURRENT_ADDR_0                                             \
	(0x0498) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_1                                             \
	(0x049C) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_2                                             \
	(0x04A0) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_3                                             \
	(0x04A4) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_4                                             \
	(0x04A8) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_5                                             \
	(0x04AC) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_6                                             \
	(0x04B0) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_7                                             \
	(0x04B4) /* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_STATUS (0x04B8)   /* SME OUT SBDMA operation status. */
#define CLDMA_AP_DEBUG_ID_EN (0x04C8) /* DEBUG_ID Enable */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_0                                         \
	(0x04D0) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_1                                         \
	(0x04D4) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_2                                         \
	(0x04D8) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_3                                         \
	(0x04DC) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_4                                         \
	(0x04E0) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_5                                         \
	(0x04E4) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_6                                         \
	(0x04E8) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_7                                         \
	(0x04EC) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_1 (0x04F0) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_2 (0x04F4) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_3 (0x04F8) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_4 (0x04FC) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_5 (0x0500) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_6 (0x0504) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_7 (0x0508) /* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_BUF_PTR_0 (0x0510)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_1 (0x0514)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_2 (0x0518)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_3 (0x051C)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_4 (0x0520)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_5 (0x0524)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_6 (0x0528)   /* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_7 (0x052C)   /* Next data buffer pointer */
/*Support >32 bit address access, such as 6GBM*/
#define CLDMA_AP_SO_START_ADDR_4MSB                                            \
	(0x0530) /* MSB 4 bits(35~32) of first RGPD */
#define CLDMA_AP_SO_CURRENT_ADDR_4MSB                                          \
	(0x0534) /* MSB 4 bits(35~32) of current processing RGPD*/
#define CLDMA_AP_SO_LAST_ADDR_4MSB                                             \
	(0x0538) /* MSB 4 bits(35~32) of last updated RGPD*/
#define CLDMA_AP_SO_NEXT_BUF_PTR_4MSB                                          \
	(0x053C) /* Next data buffer pointer MSB4 bits(35~32) */
/*=======================CLDMA_AO_MISC:
 * 10014858-10014950==================================
 */
#define CLDMA_AP_L2RIMR0                                                       \
	(0x0858) /* Level 2 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L2RIMR1                                                       \
	(0x085C) /* Level 2 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L2RIMCR0                                                      \
	(0x0860) /* Level 2 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L2RIMCR1                                                      \
	(0x0864) /* Level 2 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L2RIMSR0                                                      \
	(0x0868) /* Level 2 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_L2RIMSR1                                                      \
	(0x086C) /* Level 2 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_BUS_CFG (0x0890) /* LTEL2_BUS_INTF configuration register */
#define CLDMA_AP_CHNL_DISABLE (0x0894)  /* Dma channel disable register */
#define CLDMA_AP_HIGH_PRIORITY (0x0898) /* Dma channel high priority register  \
					 */
#define CLDMA_AP_ADDR_REMAP_FROM (0x0944) /* Address Remap From Which Bank */
#define CLDMA_AP_ADDR_REMAP_TO (0x0948)   /* Address Remap To Which Bank */
#define CLDMA_AP_ADDR_REMAP_MASK (0x094C) /* Address Remap Mask */
#define CLDMA_AP_DUMMY (0x0950)		  /* Dummy Register */
/*=======================CLDMA_PD_INDMA: 1021B000-1021B16C
 * ==================================
 */
#define CLDMA_AP_UL_SBDMA_CODA_VERSION                                         \
	(0x0000) /* ULSBDMA Version Control Register */
#define CLDMA_AP_UL_START_ADDR_0                                               \
	(0x0004) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_1                                               \
	(0x0008) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_2                                               \
	(0x000C) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_3                                               \
	(0x0010) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_4                                               \
	(0x0014) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_5                                               \
	(0x0018) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_6                                               \
	(0x001C) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_7                                               \
	(0x0020) /* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_CURRENT_ADDR_0                                             \
	(0x0028) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_1                                             \
	(0x002C) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_2                                             \
	(0x0030) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_3                                             \
	(0x0034) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_4                                             \
	(0x0038) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_5                                             \
	(0x003C) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_6                                             \
	(0x0040) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_7                                             \
	(0x0044) /* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_STATUS (0x0050)	    /* UL SBDMA operation status. */
#define CLDMA_AP_UL_START_CMD (0x0054)	 /* UL START SBDMA command. */
#define CLDMA_AP_UL_RESUME_CMD (0x0058)	/* UL RESUME SBDMA command. */
#define CLDMA_AP_UL_STOP_CMD (0x005C)	  /* UL STOP SBDMA command. */
#define CLDMA_AP_UL_ERROR (0x0060)	     /* ERROR */
#define CLDMA_AP_UL_CFG (0x0074)	       /* Operation Configuration */
#define CLDMA_AP_UL_DUMMY_0 (0x0080)	   /* Dummy Register 0 */
#define CLDMA_AP_UL_DUMMY_1 (0x0084)	   /* Dummy Register 1 */
#define CLDMA_AP_UL_DUMMY_2 (0x0088)	   /* Dummy Register 2 */
#define CLDMA_AP_UL_DUMMY_3 (0x008C)	   /* Dummy Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_0 (0x0090)  /* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_1 (0x0094)  /* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_2 (0x0098)  /* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_3 (0x009C)  /* Debug Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_4 (0x00A0)  /* Debug Register 4 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_5 (0x00A4)  /* Debug Register 5 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_6 (0x00A8)  /* Debug Register 6 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_7 (0x00AC)  /* Debug Register 7 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_8 (0x00B0)  /* Debug Register 8 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_0 (0x00B4)  /* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_1 (0x00B8)  /* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_2 (0x00BC)  /* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_3 (0x00C0)  /* Debug Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_4 (0x00C4)  /* Debug Register 4 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_5 (0x00C8)  /* Debug Register 5 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_6 (0x00CC)  /* Debug Register 6 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_7 (0x00D0)  /* Debug Register 7 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_8 (0x00D4)  /* Debug Register 8 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_9 (0x00D8)  /* Debug Register 9 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_10 (0x00DC) /* Debug Register 10 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_11 (0x00E0) /* Debug Register 11 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_12 (0x00E4) /* Debug Register 12 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_13 (0x00E8) /* Debug Register 13 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_14 (0x00EC) /* Debug Register 14 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_15 (0x00F0) /* Debug Register 15 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_0 (0x00F4)  /* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_1 (0x00F8)  /* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_2 (0x00FC)  /* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_0 (0x0100) /* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_1 (0x0104) /* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_2 (0x0108) /* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_3 (0x010C) /* Debug Register 3 */
#define CLDMA_AP_HPQTCR (0x0110)  /* High priority queue traffic control value \
				   */
#define CLDMA_AP_LPQTCR (0x0114)  /* Low priority queue traffic control value  \
				   */
#define CLDMA_AP_HPQR (0x0118)    /* High priority queue register */
#define CLDMA_AP_TCR0 (0x011C)    /* Traffic control value for TX queue 0 */
#define CLDMA_AP_TCR1 (0x0120)    /* Traffic control value for TX queue 1 */
#define CLDMA_AP_TCR2 (0x0124)    /* Traffic control value for TX queue 2 */
#define CLDMA_AP_TCR3 (0x0128)    /* Traffic control value for TX queue 3 */
#define CLDMA_AP_TCR4 (0x012C)    /* Traffic control value for TX queue 4 */
#define CLDMA_AP_TCR5 (0x0130)    /* Traffic control value for TX queue 5 */
#define CLDMA_AP_TCR6 (0x0134)    /* Traffic control value for TX queue 6 */
#define CLDMA_AP_TCR7 (0x0138)    /* Traffic control value for TX queue 7 */
#define CLDMA_AP_TCR_CMD (0x013C) /* Traffic control command register */
#define CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE                                    \
	(0x0140) /* Per-channel checksum checking function enable */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_0                                         \
	(0x0144) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_1                                         \
	(0x0148) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_2                                         \
	(0x014C) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_3                                         \
	(0x0150) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_4                                         \
	(0x0154) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_5                                         \
	(0x0158) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_6                                         \
	(0x015C) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_7                                         \
	(0x0160) /* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_START_ADDR_4MSB (0x0164)
#define CLDMA_AP_UL_CURR_ADDR_4MSB (0x0168)
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_4MSB (0x016c)
/*========================CLDMA_PD_OUTDMA:1021B400-1021B5C4================*/
#define CLDMA_AP_SO_OUTDMA_CODA_VERSION                                        \
	(0x0400)		     /* SOOUTDMA Version Control Register */
#define CLDMA_AP_SO_ERROR (0x0500)   /* ERROR */
#define CLDMA_AP_SO_DUMMY_0 (0x0508) /* Dummy Register 0 */
#define CLDMA_AP_SO_DUMMY_1 (0x050C) /* Dummy Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_0 (0x0518)    /* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_1 (0x051C)    /* Debug Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_2 (0x0520)    /* Debug Register 2 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_3 (0x0524)    /* Debug Register 3 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_4 (0x0528)    /* Debug Register 4 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_5 (0x052C)    /* Debug Register 5 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_6 (0x0530)    /* Debug Register 6 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_7 (0x0534)    /* Debug Register 7 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_8 (0x0538)    /* Debug Register 8 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_9 (0x053C)    /* Debug Register 9 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_10 (0x0540)   /* Debug Register 10 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_11 (0x0544)   /* Debug Register 11 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_12 (0x0548)   /* Debug Register 12 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_0 (0x054C)    /* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_1 (0x0550)    /* Debug Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_2 (0x0554)    /* Debug Register 2 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_3 (0x0558)    /* Debug Register 3 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_4 (0x055C)    /* Debug Register 4 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_5 (0x0560)    /* Debug Register 5 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_6 (0x0564)    /* Debug Register 6 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_7 (0x0568)    /* Debug Register 7 */
#define CLDMA_AP_SO_DEBUG_REG_REG_CTL_0 (0x056C) /* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_REG_CTL_1 (0x0570) /* Debug Register 1 */
#define CLDMA_AP_SO_START_CMD (0x05BC)  /* SME OUT SBDMA START command. */
#define CLDMA_AP_SO_RESUME_CMD (0x05C0) /* SO OUTDMA RESUME command. */
#define CLDMA_AP_SO_STOP_CMD (0x05C4)   /* SO OUTDMA STOP command. */
/*===============CLDMA_PD_MISC:1021B800-1021B8CC===========================*/
#define CLDMA_AP_CLDMA_CODA_VERSION (0x0800) /* CLDMAVersion Control Register  \
					      */
#define CLDMA_AP_HW_DVFS_CTRL (0x0804)       /*Hardware DVFS control Register*/

#define CLDMA_AP_L2TISAR0                                                      \
	(0x0810) /* Level 2 Interrupt Status and Acknowledgment Register (TX   \
		  *  Part) \
		  */
#define CLDMA_AP_L2TISAR1                                                      \
	(0x0814) /* Level 2 Interrupt Status and Acknowledgment Register (TX   \
		  * Part) \
		  */
#define CLDMA_AP_L2TIMR0                                                       \
	(0x0818) /* Level 2 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L2TIMR1                                                       \
	(0x081C) /* Level 2 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L2TIMCR0                                                      \
	(0x0820) /* Level 2 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L2TIMCR1                                                      \
	(0x0824) /* Level 2 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L2TIMSR0                                                      \
	(0x0828) /* Level 2 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L2TIMSR1                                                      \
	(0x082C) /* Level 2 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L3TISAR0                                                      \
	(0x0830) /* Level 3 Interrupt Status and Acknowledgment Register (TX   \
		  *  Part) \
		  */
#define CLDMA_AP_L3TISAR1                                                      \
	(0x0834) /* Level 3 Interrupt Status and Acknowledgment Register (TX   \
		  *  Part) \
		  */
#define CLDMA_AP_L3TIMR0                                                       \
	(0x0838) /* Level 3 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L3TIMR1                                                       \
	(0x083C) /* Level 3 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L3TIMCR0                                                      \
	(0x0840) /* Level 3 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L3TIMCR1                                                      \
	(0x0844) /* Level 3 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L3TIMSR0                                                      \
	(0x0848) /* Level 3 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L3TIMSR1                                                      \
	(0x084C) /* Level 3 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L2RISAR0                                                      \
	(0x0850) /* Level 2 Interrupt Status and Acknowledgment Register (RX   \
		  *  Part) \
		  */
#define CLDMA_AP_L2RISAR1                                                      \
	(0x0854) /* Level 2 Interrupt Status and Acknowledgment Register (RX   \
		  *  Part) \
		  */
#define CLDMA_AP_L3RISAR0                                                      \
	(0x0870) /* Level 3 Interrupt Status and Acknowledgment Register (RX   \
		  *  Part) \
		  */
#define CLDMA_AP_L3RISAR1                                                      \
	(0x0874) /* Level 3 Interrupt Status and Acknowledgment Register (RX   \
		  *  Part) \
		  */
#define CLDMA_AP_L3RIMR0                                                       \
	(0x0878) /* Level 3 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L3RIMR1                                                       \
	(0x087C) /* Level 3 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L3RIMCR0                                                      \
	(0x0880) /* Level 3 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L3RIMCR1                                                      \
	(0x0884) /* Level 3 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L3RIMSR0                                                      \
	(0x0888) /* Level 3 Interrupt MaskSet Register (RX Part) */
#define CLDMA_AP_L3RIMSR1                                                      \
	(0x088C) /* Level 3 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_BUS_STA (0x089C) /* LTEL2_BUS_INTF status register */
#define CLDMA_AP_BUS_DBG (0x08A0) /* Debug information of ltel2_axi_master */
#define CLDMA_AP_DBG_RDATA                                                     \
	(0x08A4) /* Debug information (rdata) of ltel2_axi_master */
#define CLDMA_AP_DBG_WADDR                                                     \
	(0x08A8) /* Debug information (waddr) of ltel2_axi_master */
#define CLDMA_AP_DBG_WDATA                                                     \
	(0x08AC) /* Debug information (wdata) of ltel2_axi_master */
#define CLDMA_AP_CHNL_IDLE (0x08B0)     /* Dma channel idle */
#define CLDMA_AP_CLDMA_IP_BUSY (0x08B4) /* Half DMA busy status */
#define CLDMA_AP_DBG_RADDR (0x08B8) /* Debug info(raddr) of ltel2_axi_master*/
#define CLDMA_AP_DBG_ADDR_4MSB                                                 \
	(0x08BC) /* Debug info(waddr/raddr MSB 4bits) of ltel2_axi_master*/

#define CLDMA_AP_L3TISAR2                                                      \
	(0x08C0) /* Level3 Interrupt Status and Acknowldgement Register(TX     \
		  *  Part) \
		  */
#define CLDMA_AP_L3TIMR2 (0x08C4) /* Level3 Interrupt Mask Register(TX Part)   \
				   */
#define CLDMA_AP_L3TIMCR2                                                      \
	(0x08C8) /* Level3 Interrupt Mask Clear Register(TX Part) */
#define CLDMA_AP_L3TIMSR2                                                      \
	(0x08CC) /* Level3 Interrupt Mask Set Register(TX Part) */

/*assistant macros*/
#define CLDMA_AP_TQSAR(i) (CLDMA_AP_UL_START_ADDR_0 + (4 * (i)))
#define CLDMA_AP_TQCPR(i) (CLDMA_AP_UL_CURRENT_ADDR_0 + (4 * (i)))
#define CLDMA_AP_RQSAR(i) (CLDMA_AP_SO_START_ADDR_0 + (4 * (i)))
#define CLDMA_AP_RQCPR(i) (CLDMA_AP_SO_CURRENT_ADDR_0 + (4 * (i)))
#define CLDMA_AP_TQTCR(i) (CLDMA_AP_TCR0 + (4 * (i)))
#define CLDMA_AP_TQSABAK(i) (CLDMA_AP_UL_START_ADDR_BK_0 + (4 * (i)))
#define CLDMA_AP_TQCPBAK(i) (CLDMA_AP_UL_CURRENT_ADDR_BK_0 + (4 * (i)))

#define cldma_write32(b, a, v) mt_reg_sync_writel(v, (b) + (a))
#define cldma_write16(b, a, v) mt_reg_sync_writew(v, (b) + (a))
#define cldma_write8(b, a, v) mt_reg_sync_writeb(v, (b) + (a))

#define cldma_read32(b, a) ioread32((void __iomem *)((b) + (a)))
#define cldma_read16(b, a) ioread16((void __iomem *)((b) + (a)))
#define cldma_read8(b, a) ioread8((void __iomem *)((b) + (a)))

/*bitmap*/
#define CLDMA_BM_INT_ALL 0xFFFFFFFF
/* L2 interrupt */
#define CLDMA_TX_INT_ACTIVE_START                                              \
	0xFF000000 /* trigger start command on one active queue */
#define CLDMA_TX_INT_ERROR                                                     \
	0x00FF0000 /* error occurred on the specified queue, check L3  for     \
		    *  detail \
		    */
#define CLDMA_TX_INT_QUEUE_EMPTY                                               \
	0x0000FF00 /* when there is no GPD to be transmitted on the specified  \
		    *  queue \
		    */
#define CLDMA_TX_QE_OFFSET 8
#define CLDMA_TX_INT_DONE                                                      \
	0x000000FF /* when the transmission if the GPD on the specified queue  \
		    * is done \
		    */
#define CLDMA_RX_INT_ACTIVE_START                                              \
	0xFF000000 /* trigger start command on one active queue */
#define CLDMA_RX_INT_ERROR                                                     \
	0x00FF0000 /* error occurred on the specified queue, check L3  for     \
		    *  detail \
		    */
#define CLDMA_RX_INT_QUEUE_EMPTY                                               \
	0x0000FF00 /* when there is no GPD to be transmitted on the specified  \
		    *  queue \
		    */
#define CLDMA_RX_QE_OFFSET 8
#define CLDMA_RX_INT_DONE                                                      \
	0x000000FF /* when the transmission if the GPD on the specified queue  \
		    *  is done \
		    */

#define CLDMA_BM_ALL_QUEUE 0x7F /* all 7 queues */

static inline void cldma_reg_set_4msb_val(void *base,
					  unsigned int reg_4msb_offset, int idx,
					  dma_addr_t addr)
{
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	unsigned int val = 0;

	val = cldma_read32(base, reg_4msb_offset);
	val &= ~(0xF << (idx * 4));		    /*clear 0xF << i*4 bit */
	val |= (((addr >> 32) & 0xF) << (idx * 4)); /*set 4msb bit*/
	cldma_write32(base, reg_4msb_offset, val);
#endif
}
static inline unsigned int
cldma_reg_get_4msb_val(void *base, unsigned int reg_4msb_offset, int idx)
{
	unsigned int val = 0;

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	val = cldma_read32(base, reg_4msb_offset);
	val &= (0xF << (idx * 4)); /*clear no need bit, keep 0xF << i*4 bit */
	val >>= (idx * 4);	 /* get 4msb bit*/
#endif
	return val;
}
static inline void cldma_reg_set_tx_start_addr(void *base, int idx,
					       dma_addr_t addr)
{
	cldma_write32(base, CLDMA_AP_TQSAR(idx), (u32)addr);
	cldma_reg_set_4msb_val(base, CLDMA_AP_UL_START_ADDR_4MSB, idx, addr);
}
static inline void cldma_reg_set_tx_start_addr_bk(void *base, int idx,
						  dma_addr_t addr)
{
	/*Set start bak address in power on domain*/
	cldma_write32(base, CLDMA_AP_TQSABAK(idx), (u32)addr);
	cldma_reg_set_4msb_val(base, CLDMA_AP_UL_START_ADDR_BK_4MSB, idx, addr);
}

static inline void cldma_reg_set_rx_start_addr(void *base, int idx,
					       dma_addr_t addr)
{
	cldma_write32(base, CLDMA_AP_RQSAR(idx), (u32)addr);
	cldma_reg_set_4msb_val(base, CLDMA_AP_SO_START_ADDR_4MSB, idx, addr);
}
#endif /* __CLDMA_REG_H__ */
