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
#define INFRA_RST0_REG_PD (0x0150) /*(0x0030)*/	/* rgu reset cldma reg */
#define INFRA_RST1_REG_PD (0x0154) /*(0x0034)*/	/* rgu clear cldma reset reg */
#define CLDMA_PD_RST_MASK (1<<2)
#define INFRA_RST0_REG_AO (0x0140)
#define INFRA_RST1_REG_AO (0x0144)
#define CLDMA_AO_RST_MASK (1<<6)

#define INFRA_AO_MD_SRCCLKENA    (0xF0C) /* SRC CLK ENA */
/*===========================CLDMA_AO_INDMA: 10014804-10014844==================================*/
#define CLDMA_AP_UL_START_ADDR_BK_0      (0x0804)
	/* The start address of first TGPD descriptor for power-down back-up. */
#define CLDMA_AP_UL_START_ADDR_BK_1      (0x0808)
#define CLDMA_AP_UL_START_ADDR_BK_2      (0x080C)
#define CLDMA_AP_UL_START_ADDR_BK_3      (0x0810)
#define CLDMA_AP_UL_START_ADDR_BK_4      (0x0814)
#define CLDMA_AP_UL_START_ADDR_BK_5      (0x0818)
#define CLDMA_AP_UL_START_ADDR_BK_6      (0x081C)
#define CLDMA_AP_UL_START_ADDR_BK_7      (0x0820)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_0    (0x0828)
	/* The address of current processing TGPD descriptor for power-down back-up. */
#define CLDMA_AP_UL_CURRENT_ADDR_BK_1    (0x082C)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_2    (0x0830)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_3    (0x0834)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_4    (0x0838)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_5    (0x083C)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_6    (0x0840)
#define CLDMA_AP_UL_CURRENT_ADDR_BK_7    (0x0844)
/*===========================CLDMA_AO_OUTDMA:10014A04-10014B2C==================================*/
#define CLDMA_AP_SO_CFG                     (0x0A04)	/* Operation Configuration */
#define CLDMA_AP_SO_DUMMY_2                 (0x0A10)	/* Dummy Register 2 */
#define CLDMA_AP_SO_DUMMY_3                 (0x0A14)	/* Dummy Register 3 */
#define CLDMA_AP_SO_CHECKSUM_CHANNEL_ENABLE (0x0A74)	/* Per-channel checksum checking function enable */
#define CLDMA_AP_SO_START_ADDR_0            (0x0A78)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_1            (0x0A7C)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_2            (0x0A80)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_3            (0x0A84)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_4            (0x0A88)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_5            (0x0A8C)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_6            (0x0A90)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_START_ADDR_7            (0x0A94)	/* The start address of first RGPD descriptor */
#define CLDMA_AP_SO_CURRENT_ADDR_0          (0x0A98)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_1          (0x0A9C)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_2          (0x0AA0)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_3          (0x0AA4)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_4          (0x0AA8)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_5          (0x0AAC)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_6          (0x0AB0)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_CURRENT_ADDR_7          (0x0AB4)	/* The address of current processing RGPD descriptor. */
#define CLDMA_AP_SO_STATUS                  (0x0AB8)	/* SME OUT SBDMA operation status. */
#define CLDMA_AP_DEBUG_ID_EN                (0x0AC8)	/* DEBUG_ID Enable */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_0         (0x0AD0)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_1         (0x0AD4)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_2         (0x0AD8)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_3         (0x0ADC)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_4         (0x0AE0)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_5         (0x0AE4)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_6         (0x0AE8)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_LAST_UPDATE_ADDR_7         (0x0AEC)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_1           (0x0AF0)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_2           (0x0AF4)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_3           (0x0AF8)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_4           (0x0AFC)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_5           (0x0B00)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_6           (0x0B04)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_ALLOW_LEN_7           (0x0B08)	/* Next allow buffer length */
#define CLDMA_AP_SO_NEXT_BUF_PTR_0             (0x0B10)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_1             (0x0B14)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_2             (0x0B18)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_3             (0x0B1C)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_4             (0x0B20)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_5             (0x0B24)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_6             (0x0B28)	/* Next data buffer pointer */
#define CLDMA_AP_SO_NEXT_BUF_PTR_7             (0x0B2C)	/* Next data buffer pointer */
/*=======================CLDMA_AO_MISC:  10014C58-10014D50==================================*/
#define CLDMA_AP_L2RIMR0                    (0x0C58)	/* Level 2 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L2RIMR1                    (0x0C5C)	/* Level 2 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L2RIMCR0                   (0x0C60)	/* Level 2 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L2RIMCR1                   (0x0C64)	/* Level 2 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L2RIMSR0                   (0x0C68)	/* Level 2 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_L2RIMSR1                   (0x0C6C)	/* Level 2 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_BUS_CFG                    (0x0C90)	/* LTEL2_BUS_INTF configuration register */
#define CLDMA_AP_CHNL_DISABLE               (0x0C94)	/* Dma channel disable register */
#define CLDMA_AP_HIGH_PRIORITY              (0x0C98)	/* Dma channel high priority register */
#define CLDMA_AP_ADDR_REMAP_FROM            (0x0D44)	/* Address Remap From Which Bank */
#define CLDMA_AP_ADDR_REMAP_TO              (0x0D48)	/* Address Remap To Which Bank */
#define CLDMA_AP_ADDR_REMAP_MASK            (0x0D4C)	/* Address Remap Mask */
#define CLDMA_AP_DUMMY                      (0x0D50)	/* Dummy Register */
/*=======================CLDMA_PD_INDMA: 1021B000-1021B160==================================*/
#define CLDMA_AP_UL_SBDMA_CODA_VERSION      (0x0000)	/* ULSBDMA Version Control Register */
#define CLDMA_AP_UL_START_ADDR_0            (0x0004)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_1            (0x0008)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_2            (0x000C)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_3            (0x0010)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_4            (0x0014)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_5            (0x0018)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_6            (0x001C)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_START_ADDR_7            (0x0020)	/* The start address of first TGPD descriptor */
#define CLDMA_AP_UL_CURRENT_ADDR_0          (0x0028)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_1          (0x002C)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_2          (0x0030)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_3          (0x0034)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_4          (0x0038)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_5          (0x003C)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_6          (0x0040)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_CURRENT_ADDR_7          (0x0044)	/* The address of current processing TGPD descriptor. */
#define CLDMA_AP_UL_STATUS                  (0x0050)	/* UL SBDMA operation status. */
#define CLDMA_AP_UL_START_CMD               (0x0054)	/* UL START SBDMA command. */
#define CLDMA_AP_UL_RESUME_CMD              (0x0058)	/* UL RESUME SBDMA command. */
#define CLDMA_AP_UL_STOP_CMD                (0x005C)	/* UL STOP SBDMA command. */
#define CLDMA_AP_UL_ERROR                   (0x0060)	/* ERROR */
#define CLDMA_AP_UL_CFG                     (0x0074)	/* Operation Configuration */
#define CLDMA_AP_UL_DUMMY_0                 (0x0080)	/* Dummy Register 0 */
#define CLDMA_AP_UL_DUMMY_1                 (0x0084)	/* Dummy Register 1 */
#define CLDMA_AP_UL_DUMMY_2                 (0x0088)	/* Dummy Register 2 */
#define CLDMA_AP_UL_DUMMY_3                 (0x008C)	/* Dummy Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_0        (0x0090)	/* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_1        (0x0094)	/* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_2        (0x0098)	/* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_3        (0x009C)	/* Debug Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_4        (0x00A0)	/* Debug Register 4 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_5        (0x00A4)	/* Debug Register 5 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_6        (0x00A8)	/* Debug Register 6 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_7        (0x00AC)	/* Debug Register 7 */
#define CLDMA_AP_UL_DEBUG_REG_LCMU_8        (0x00B0)	/* Debug Register 8 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_0        (0x00B4)	/* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_1        (0x00B8)	/* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_2        (0x00BC)	/* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_3        (0x00C0)	/* Debug Register 3 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_4        (0x00C4)	/* Debug Register 4 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_5        (0x00C8)	/* Debug Register 5 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_6        (0x00CC)	/* Debug Register 6 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_7        (0x00D0)	/* Debug Register 7 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_8        (0x00D4)	/* Debug Register 8 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_9        (0x00D8)	/* Debug Register 9 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_10       (0x00DC)	/* Debug Register 10 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_11       (0x00E0)	/* Debug Register 11 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_12       (0x00E4)	/* Debug Register 12 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_13       (0x00E8)	/* Debug Register 13 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_14       (0x00EC)	/* Debug Register 14 */
#define CLDMA_AP_UL_DEBUG_REG_LDMU_15       (0x00F0)	/* Debug Register 15 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_0        (0x00F4)	/* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_1        (0x00F8)	/* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_LDMA_2        (0x00FC)	/* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_0     (0x0100)	/* Debug Register 0 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_1     (0x0104)	/* Debug Register 1 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_2     (0x0108)	/* Debug Register 2 */
#define CLDMA_AP_UL_DEBUG_REG_REG_CTL_3     (0x010C)	/* Debug Register 3 */
#define CLDMA_AP_HPQTCR                     (0x0110)	/* High priority queue traffic control value */
#define CLDMA_AP_LPQTCR                     (0x0114)	/* Low priority queue traffic control value */
#define CLDMA_AP_HPQR                       (0x0118)	/* High priority queue register */
#define CLDMA_AP_TCR0                       (0x011C)	/* Traffic control value for TX queue 0 */
#define CLDMA_AP_TCR1                       (0x0120)	/* Traffic control value for TX queue 1 */
#define CLDMA_AP_TCR2                       (0x0124)	/* Traffic control value for TX queue 2 */
#define CLDMA_AP_TCR3                       (0x0128)	/* Traffic control value for TX queue 3 */
#define CLDMA_AP_TCR4                       (0x012C)	/* Traffic control value for TX queue 4 */
#define CLDMA_AP_TCR5                       (0x0130)	/* Traffic control value for TX queue 5 */
#define CLDMA_AP_TCR6                       (0x0134)	/* Traffic control value for TX queue 6 */
#define CLDMA_AP_TCR7                       (0x0138)	/* Traffic control value for TX queue 7 */
#define CLDMA_AP_TCR_CMD                    (0x013C)	/* Traffic control command register */
#define CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE (0x0140)	/* Per-channel checksum checking function enable */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_0      (0x0144)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_1      (0x0148)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_2      (0x014C)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_3      (0x0150)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_4      (0x0154)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_5      (0x0158)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_6      (0x015C)	/* The address of last updated TGPD descriptor. */
#define CLDMA_AP_UL_LAST_UPDATE_ADDR_7      (0x0160)	/* The address of last updated TGPD descriptor. */

/*========================CLDMA_PD_OUTDMA:1021B200-1021B3C4==================================*/
#define CLDMA_AP_SO_OUTDMA_CODA_VERSION     (0x0200)	/* SOOUTDMA Version Control Register */
#define CLDMA_AP_SO_ERROR                   (0x0300)	/* ERROR */
#define CLDMA_AP_SO_DUMMY_0                 (0x0308)	/* Dummy Register 0 */
#define CLDMA_AP_SO_DUMMY_1                 (0x030C)	/* Dummy Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_0        (0x0318)	/* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_1        (0x031C)	/* Debug Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_2        (0x0320)	/* Debug Register 2 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_3        (0x0324)	/* Debug Register 3 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_4        (0x0328)	/* Debug Register 4 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_5        (0x032C)	/* Debug Register 5 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_6        (0x0330)	/* Debug Register 6 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_7        (0x0334)	/* Debug Register 7 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_8        (0x0338)	/* Debug Register 8 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_9        (0x033C)	/* Debug Register 9 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_10       (0x0340)	/* Debug Register 10 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_11       (0x0344)	/* Debug Register 11 */
#define CLDMA_AP_SO_DEBUG_REG_LDMU_12       (0x0348)	/* Debug Register 12 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_0        (0x034C)	/* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_1        (0x0350)	/* Debug Register 1 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_2        (0x0354)	/* Debug Register 2 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_3        (0x0358)	/* Debug Register 3 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_4        (0x035C)	/* Debug Register 4 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_5        (0x0360)	/* Debug Register 5 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_6        (0x0364)	/* Debug Register 6 */
#define CLDMA_AP_SO_DEBUG_REG_LDMA_7        (0x0368)	/* Debug Register 7 */
#define CLDMA_AP_SO_DEBUG_REG_REG_CTL_0     (0x036C)	/* Debug Register 0 */
#define CLDMA_AP_SO_DEBUG_REG_REG_CTL_1     (0x0370)	/* Debug Register 1 */
#define CLDMA_AP_SO_START_CMD               (0x03BC)	/* SME OUT SBDMA START command. */
#define CLDMA_AP_SO_RESUME_CMD              (0x03C0)	/* SO OUTDMA RESUME command. */
#define CLDMA_AP_SO_STOP_CMD                (0x03C4)	/* SO OUTDMA STOP command. */
/*===========================CLDMA_PD_MISC:  1021B400-1021B4B4================================*/
#define CLDMA_AP_CLDMA_CODA_VERSION	(0x0400) /* CLDMAVersion Control Register */
#define CLDMA_AP_L2TISAR0		(0x0410) /* Level 2 Interrupt Status and Acknowledgment Register (TX Part) */
#define CLDMA_AP_L2TISAR1		(0x0414) /* Level 2 Interrupt Status and Acknowledgment Register (TX Part) */
#define CLDMA_AP_L2TIMR0		(0x0418) /* Level 2 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L2TIMR1		(0x041C) /* Level 2 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L2TIMCR0		(0x0420) /* Level 2 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L2TIMCR1		(0x0424) /* Level 2 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L2TIMSR0		(0x0428) /* Level 2 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L2TIMSR1		(0x042C) /* Level 2 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L3TISAR0		(0x0430) /* Level 3 Interrupt Status and Acknowledgment Register (TX Part) */
#define CLDMA_AP_L3TISAR1		(0x0434) /* Level 3 Interrupt Status and Acknowledgment Register (TX Part) */
#define CLDMA_AP_L3TIMR0		(0x0438) /* Level 3 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L3TIMR1		(0x043C) /* Level 3 Interrupt Mask Register (TX Part) */
#define CLDMA_AP_L3TIMCR0		(0x0440) /* Level 3 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L3TIMCR1		(0x0444) /* Level 3 Interrupt Mask Clear Register (TX Part) */
#define CLDMA_AP_L3TIMSR0		(0x0448) /* Level 3 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L3TIMSR1		(0x044C) /* Level 3 Interrupt Mask Set Register (TX Part) */
#define CLDMA_AP_L2RISAR0		(0x0450) /* Level 2 Interrupt Status and Acknowledgment Register (RX Part) */
#define CLDMA_AP_L2RISAR1		(0x0454) /* Level 2 Interrupt Status and Acknowledgment Register (RX Part) */
#define CLDMA_AP_L3RISAR0		(0x0470) /* Level 3 Interrupt Status and Acknowledgment Register (RX Part) */
#define CLDMA_AP_L3RISAR1		(0x0474) /* Level 3 Interrupt Status and Acknowledgment Register (RX Part) */
#define CLDMA_AP_L3RIMR0		(0x0478) /* Level 3 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L3RIMR1		(0x047C) /* Level 3 Interrupt Mask Register (RX Part) */
#define CLDMA_AP_L3RIMCR0		(0x0480) /* Level 3 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L3RIMCR1		(0x0484) /* Level 3 Interrupt Mask Clear Register (RX Part) */
#define CLDMA_AP_L3RIMSR0		(0x0488) /* Level 3 Interrupt MaskSet Register (RX Part) */
#define CLDMA_AP_L3RIMSR1		(0x048C) /* Level 3 Interrupt Mask Set Register (RX Part) */
#define CLDMA_AP_BUS_STA		(0x049C) /* LTEL2_BUS_INTF status register */
#define CLDMA_AP_BUS_DBG		(0x04A0) /* Debug information of ltel2_axi_master */
#define CLDMA_AP_DBG_RDATA		(0x04A4) /* Debug information (rdata) of ltel2_axi_master */
#define CLDMA_AP_DBG_WADDR		(0x04A8) /* Debug information (waddr) of ltel2_axi_master */
#define CLDMA_AP_DBG_WDATA		(0x04AC) /* Debug information (wdata) of ltel2_axi_master */
#define CLDMA_AP_CHNL_IDLE		(0x04B0) /* Dma channel idle */
#define CLDMA_AP_CLDMA_IP_BUSY		(0x04B4) /* Half DMA busy status */
#define CLDMA_AP_L3TISAR2               (0x04C0) /* Level3 Interrupt Status and Acknowledgment Register(TX Part) */
#define CLDMA_AP_L3TIMR2                (0x04C4) /* Level3 Interrupt Mask Register(TX Part) */
#define CLDMA_AP_L3TIMCR2               (0x04C8) /* Level3 Interrupt Mask Clear Register(TX Part) */
#define CLDMA_AP_L3TIMSR2               (0x04CC) /* Level3 Interrupt Mask Set Register(TX Part) */

/*assistant macros*/
#define CLDMA_AP_TQSAR(i)  (CLDMA_AP_UL_START_ADDR_0   + (4 * (i)))
#define CLDMA_AP_TQCPR(i)  (CLDMA_AP_UL_CURRENT_ADDR_0 + (4 * (i)))
#define CLDMA_AP_RQSAR(i)  (CLDMA_AP_SO_START_ADDR_0   + (4 * (i)))
#define CLDMA_AP_RQCPR(i)  (CLDMA_AP_SO_CURRENT_ADDR_0 + (4 * (i)))
#define CLDMA_AP_TQTCR(i)  (CLDMA_AP_TCR0 + (4 * (i)))
#define CLDMA_AP_TQSABAK(i)  (CLDMA_AP_UL_START_ADDR_BK_0 + (4 * (i)))
#define CLDMA_AP_TQCPBAK(i)  (CLDMA_AP_UL_CURRENT_ADDR_BK_0 + (4 * (i)))

#define cldma_write32(b, a, v)			mt_reg_sync_writel(v, (b)+(a))
#define cldma_write16(b, a, v)			mt_reg_sync_writew(v, (b)+(a))
#define cldma_write8(b, a, v)			mt_reg_sync_writeb(v, (b)+(a))

#define cldma_read32(b, a)				ioread32((void __iomem *)((b)+(a)))
#define cldma_read16(b, a)				ioread16((void __iomem *)((b)+(a)))
#define cldma_read8(b, a)				ioread8((void __iomem *)((b)+(a)))

static inline void cldma_reg_set_tx_start_addr(void *base, int idx, dma_addr_t addr)
{
	cldma_write32(base, CLDMA_AP_TQSAR(idx), (u32)addr);
}
static inline void cldma_reg_set_tx_start_addr_bk(void *base, int idx, dma_addr_t addr)
{
	/*Set start bak address in power on domain*/
	cldma_write32(base, CLDMA_AP_TQSABAK(idx), (u32)addr);
}

static inline void cldma_reg_set_rx_start_addr(void *base, int idx, dma_addr_t addr)
{
	cldma_write32(base, CLDMA_AP_RQSAR(idx), (u32)addr);
}
/*bitmap*/
#define CLDMA_BM_INT_ALL			0xFFFFFFFF
/* L2 interrupt */
#define CLDMA_BM_INT_ACTIVE_START	0xFF000000 /* trigger start command on one active queue */
#define CLDMA_BM_INT_ERROR		0x00FF0000
	/* error occurred on the specified queue, check L3 interrupt register for detail */
#define CLDMA_BM_INT_QUEUE_EMPTY	0x0000FF00 /* when there is no GPD to be transmitted on the specified queue */
#define CLDMA_BM_INT_DONE		0x000000FF /* when the transmission if the GPD on the specified queue is done */
#define CLDMA_BM_INT_ACTIVE_LD_TC	0x000000FF /* modify TC register when one Tx channel is active */
#define CLDMA_BM_INT_INACTIVE_ERR	0x000000FF /* asserted when a specified Rx queue is inactive */
/* L3 interrupt */
#define CLDMA_BM_INT_BD_LEN_ERR		0xFF000000 /* asserted when a length fild in BD is not configured correctly */
#define CLDMA_BM_INT_GPD_LEN_ERR	0x00FF0000 /* asserted when a length fild in GPD is not configured correctly */
#define CLDMA_BM_INT_BD_CSERR		0x0000FF00 /* asserted when the BD checksum error happen */
#define CLDMA_BM_INT_GPD_CSERR		0x000000FF /* asserted when the GPD checksum error happen */
#define CLDMA_BM_INT_DATA_LEN_MIS	0x00FF0000 /* TGPD data length mismatch error happen */
#define CLDMA_BM_INT_BD_64KERR		0x0000FF00 /* asserted when the TBD length is more than 64K */
#define CLDMA_BM_INT_GPD_64KERR		0x000000FF /* asserted when the TGPD length is more than 64K */
#define CLDMA_BM_INT_RBIDX_ERR		0x80000000 /* internal error for Rx queue */
#define CLDMA_BM_INT_FIFO_LEN_MIS	0x0000FF00 /* internal error for Rx queue */
#define CLDMA_BM_INT_ALLEN			0x000000FF
	/* asserted when the RGPD/RBD allow data buffer length is not enough */

#define CLDMA_BM_ALL_QUEUE 0x7F	/* all 7 queues */

#endif				/* __CLDMA_REG_H__ */
