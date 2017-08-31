/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_MCDI_REG_H__
#define __MTK_MCDI_REG_H__

#include <mt-plat/sync_write.h>

extern void __iomem *sspm_base;
extern void __iomem *mcdi_mcupm_base;
extern void __iomem *mcdi_mcupm_sram_base;

#define SSPM_CFGREG_ACAO_INT_SET             (sspm_base + 0x00D8)
#define SSPM_CFGREG_ACAO_INT_CLR             (sspm_base + 0x00DC)
#define SSPM_CFGREG_RSV_RW_REG0              (sspm_base + 0x0100)

#define MCUPM_CFGREG_RSV_RW_REG1             (mcdi_mcupm_base + 0x0054)


#define STANDBYWFI_EN(n)                     (1 << (n +  8))
#define GIC_IRQOUT_EN(n)                     (1 << (n +  0))

#define mcdi_read(addr)                      __raw_readl((void __force __iomem *)(addr))
#define mcdi_write(addr, val)                mt_reg_sync_writel(val, addr)

/* MBOX: AP write, MCUPM read */
#define MCDI_MBOX_CLUSTER_0_ATF_ACTION_DONE     MCUPM_CFGREG_WFI_EN_SET
#define MCDI_MBOX_PAUSE_ACTION                  (mcdi_mcupm_sram_base + 0x1D00)
#define MCDI_MBOX_AVAIL_CPU_MASK                (mcdi_mcupm_sram_base + 0x1D08)
/* MBOX: AP read, MCUPM write */
#define MCDI_MBOX_PAUSE_ACK                     (mcdi_mcupm_sram_base + 0x1D04)
#define MCDI_MBOX_CPU_CLUSTER_PWR_STAT          MCUPM_CFGREG_RSV_RW_REG1
#define MCDI_MBOX_CLUSTER_0_CNT                 (mcdi_mcupm_sram_base + 0x1D10)
#define MCDI_MBOX_PENDING_ON_EVENT              (mcdi_mcupm_sram_base + 0x1D14)

#endif /* __MTK_MCDI_REG_H__ */
