/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef _MDLA_HW_REG_H_
#define _MDLA_HW_REG_H_


#define APU2_IRQ_ID			(321+32)
#define MDLA_IRQ_SWCMD_TILECNT_INT	(1 << 1)
#define MDLA_IRQ_TILECNT_DONE		(1 << 1)
#define MDLA_IRQ_SWCMD_DONE		(1 << 2)
#define MDLA_IRQ_PMU_INTE		(1 << 9)
#define MDLA_IRQ_MASK			(0x1FFFFF)

/* MDLA config */
#define MDLA_CG_CON        (0x000)
#define MDLA_CG_SET        (0x004)
#define MDLA_CG_CLR        (0x008)
#define MDLA_SW_RST        (0x00C)
#define MDLA_MBIST_MODE0   (0x010)
#define MDLA_MBIST_MODE1   (0x014)
#define MDLA_MBIST_CTL     (0x018)
#define MDLA_RP_OK0        (0x01C)
#define MDLA_RP_OK1        (0x020)
#define MDLA_RP_OK2        (0x024)
#define MDLA_RP_OK3        (0x028)
#define MDLA_RP_FAIL0      (0x02C)
#define MDLA_RP_FAIL1      (0x030)
#define MDLA_RP_FAIL2      (0x034)
#define MDLA_RP_FAIL3      (0x038)
#define MDLA_MBIST_FAIL0   (0x03C)
#define MDLA_MBIST_FAIL1   (0x040)
#define MDLA_MBIST_FAIL2   (0x044)
#define MDLA_MBIST_FAIL3   (0x048)
#define MDLA_MBIST_FAIL4   (0x04C)
#define MDLA_MBIST_FAIL5   (0x050)
#define MDLA_MBIST_DONE0   (0x054)
#define MDLA_MBIST_DONE1   (0x058)
#define MDLA_MBIST_DEFAULT_DELSEL (0x05C)
#define MDLA_SRAM_DELSEL0  (0x060)
#define MDLA_SRAM_DELSEL1  (0x064)
#define MDLA_RP_RST        (0x068)
#define MDLA_RP_CON        (0x06C)
#define MDLA_RP_PRE_FUSE   (0x070)
#define MDLA_SPARE_0       (0x074)
#define MDLA_SPARE_1       (0x078)
#define MDLA_SPARE_2       (0x07C)
#define MDLA_SPARE_3       (0x080)
#define MDLA_SPARE_4       (0x084)
#define MDLA_SPARE_5       (0x088)
#define MDLA_SPARE_6       (0x08C)
#define MDLA_SPARE_7       (0x090)

/* MDLA command */
#define MREG_TOP_G_REV     (0x0500)
#define MREG_TOP_G_INTP0   (0x0504)
#define MREG_TOP_G_INTP1   (0x0508)
#define MREG_TOP_G_INTP2   (0x050C)
#define MREG_TOP_G_CDMA0   (0x0510)
#define MREG_TOP_G_CDMA1   (0x0514)
#define MREG_TOP_G_CDMA2   (0x0518)
#define MREG_TOP_G_CDMA3   (0x051C)
#define MREG_TOP_G_CDMA4   (0x0520)
#define MREG_TOP_G_CDMA5   (0x0524)
#define MREG_TOP_G_CDMA6   (0x0528)
#define MREG_TOP_G_CUR0    (0x052C)
#define MREG_TOP_G_CUR1    (0x0530)
#define MREG_TOP_G_FIN0    (0x0534)
#define MREG_TOP_G_FIN1    (0x0538)
#define MREG_TOP_G_STREAM0 (0x053C)
#define MREG_TOP_G_STREAM1 (0x0540)
#define MREG_TOP_G_IDLE    (0x0544)


/* MDLA PMU */

#define CFG_PMCR_DEFAULT   (0x1F021)
#define PMU_CNT_SHIFT      (0x0010)
#define PMU_CLR_CMDE_SHIFT (0x5)

#define PMU_CFG_PMCR       (0x0E00)
#define PMU_CYCLE          (0x0E04)
#define PMU_START_TSTAMP   (0x0E08)
#define PMU_END_TSTAMP     (0x0E0C)
#define PMU_EVENT_OFFSET   (0x0E10)
#define PMU_CNT_OFFSET     (0x0E14)

#endif
