/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef _MSDC_CUST_MT6763_H_
#define _MSDC_CUST_MT6763_H_

#include <dt-bindings/mmc/mt6763-msdc.h>
#include <dt-bindings/clock/mt6763-clk.h>

#include <spm_v4/mtk_spm_resource_req.h>

/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
/* Names used for device tree lookup */
#define DT_COMPATIBLE_NAME      "mediatek,msdc"
#define MSDC0_CLK_NAME          "msdc0-clock"
#define MSDC0_HCLK_NAME         "msdc0-hclock"
#define MSDC1_CLK_NAME          "msdc1-clock"
#define MSDC1_HCLK_NAME         "msdc1-hclock"
#define MSDC2_CLK_NAME          "msdc2-clock"
#define MSDC2_HCLK_NAME         "msdc2-hclock"
#define MSDC0_IOCFG_NAME        "mediatek,iocfg_tl"
#define MSDC1_IOCFG_NAME        "mediatek,iocfg_lm"
#define MSDC2_IOCFG_NAME        "mediatek,iocfg_rt"


/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
#include <mt-plat/upmu_common.h>

#define REG_VEMC_VOSEL_CAL      PMIC_RG_VEMC_VOCAL_ADDR
#define MASK_VEMC_VOSEL_CAL     PMIC_RG_VEMC_VOCAL_MASK
#define SHIFT_VEMC_VOSEL_CAL    PMIC_RG_VEMC_VOCAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL    (MASK_VEMC_VOSEL_CAL \
					<< SHIFT_VEMC_VOSEL_CAL)

#define REG_VEMC_VOSEL          PMIC_RG_VEMC_VOSEL_ADDR
#define MASK_VEMC_VOSEL         PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL        PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL        (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)

#define REG_VEMC_EN             PMIC_RG_LDO_VEMC_EN_ADDR
#define MASK_VEMC_EN            PMIC_RG_LDO_VEMC_EN_MASK
#define SHIFT_VEMC_EN           PMIC_RG_LDO_VEMC_EN_SHIFT
#define FIELD_VEMC_EN           (MASK_VEMC_EN << SHIFT_VEMC_EN)

#define REG_VMC_VOSEL_CAL       PMIC_RG_VMC_VOCAL_ADDR
#define MASK_VMC_VOSEL_CAL      PMIC_RG_VMC_VOCAL_MASK
#define SHIFT_VMC_VOSEL_CAL     PMIC_RG_VMC_VOCAL_SHIFT
#define FIELD_VMC_VOSEL_CAL     (MASK_VMC_VOSEL_CAL \
					<< SHIFT_VMC_VOSEL_CAL)

#define REG_VMC_VOSEL           PMIC_RG_VMC_VOSEL_ADDR
#define MASK_VMC_VOSEL          PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL         PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL         (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)

#define REG_VMC_EN              PMIC_RG_LDO_VMC_EN_ADDR
#define MASK_VMC_EN             PMIC_RG_LDO_VMC_EN_MASK
#define SHIFT_VMC_EN            PMIC_RG_LDO_VMC_EN_SHIFT
#define FIELD_VMC_EN            (MASK_VMC_EN << SHIFT_VMC_EN)

#define REG_VMCH_VOSEL_CAL      PMIC_RG_VMCH_VOCAL_ADDR
#define MASK_VMCH_VOSEL_CAL     PMIC_RG_VMCH_VOCAL_MASK
#define SHIFT_VMCH_VOSEL_CAL    PMIC_RG_VMCH_VOCAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL    (MASK_VMCH_VOSEL_CAL \
					<< SHIFT_VMCH_VOSEL_CAL)

#define REG_VMCH_VOSEL          PMIC_RG_VMCH_VOSEL_ADDR
#define MASK_VMCH_VOSEL         PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL        PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL        (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)

#define REG_VMCH_EN             PMIC_RG_LDO_VMCH_EN_ADDR
#define MASK_VMCH_EN            PMIC_RG_LDO_VMCH_EN_MASK
#define SHIFT_VMCH_EN           PMIC_RG_LDO_VMCH_EN_SHIFT
#define FIELD_VMCH_EN           (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define REG_VMCH_OC_RAW_STATUS      PMIC_RG_INT_RAW_STATUS_VMCH_OC_ADDR
#define MASK_VMCH_OC_RAW_STATUS     PMIC_RG_INT_RAW_STATUS_VMCH_OC_MASK
#define SHIFT_VMCH_OC_RAW_STATUS    PMIC_RG_INT_RAW_STATUS_VMCH_OC_SHIFT
#define FIELD_VMCH_OC_RAW_STATUS    (MASK_VMCH_OC_RAW_STATUS << SHIFT_VMCH_OC_RAW_STATUS)

#define REG_VMCH_OC_STATUS      PMIC_RG_INT_STATUS_VMCH_OC_ADDR
#define MASK_VMCH_OC_STATUS     PMIC_RG_INT_STATUS_VMCH_OC_MASK
#define SHIFT_VMCH_OC_STATUS    PMIC_RG_INT_STATUS_VMCH_OC_SHIFT
#define FIELD_VMCH_OC_STATUS    (MASK_VMCH_OC_STATUS << SHIFT_VMCH_OC_STATUS)

#define VEMC_VOSEL_CAL_mV(cal)  ((cal <= 0) ? (0) : ((cal)/10))
#define VEMC_VOSEL_2V9          (0xa)
#define VEMC_VOSEL_3V           (0xb)
#define VEMC_VOSEL_3V3          (0xd)
#define VMC_VOSEL_CAL_mV(cal)   ((cal <= 0) ? (0) : ((cal)/10))
#define VMC_VOSEL_1V8           (0x4)
#define VMC_VOSEL_2V9           (0xa)
#define VMC_VOSEL_3V            (0xb)
#define VMC_VOSEL_3V3           (0xd)
#define VMCH_VOSEL_CAL_mV(cal)  ((cal <= 0) ? (0) : ((cal)/10))
#define VMCH_VOSEL_2V9          (0xa)
#define VMCH_VOSEL_3V           (0xb)
#define VMCH_VOSEL_3V3          (0xd)
#endif

#define REG_VCORE_VOSEL         0xBE8
#define MASK_VCORE_VOSEL        0x7F
#define SHIFT_VCORE_VOSEL       0

#define SD_POWER_DEFAULT_ON     (0)
#define EMMC_VOL_ACTUAL         VOL_3000
#define SD_VOL_ACTUAL           VOL_3000

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/* MSDCPLL register offset */
#define MSDCPLL_CON0_OFFSET     (0x250)
#define MSDCPLL_CON1_OFFSET     (0x254)
#define MSDCPLL_CON2_OFFSET     (0x258)
#define MSDCPLL_PWR_CON0_OFFSET (0x25c)
#endif

#define MSDCPLL_FREQ            400000000

#define MSDC0_SRC_0             26000000
#define MSDC0_SRC_1             MSDCPLL_FREQ
#define MSDC0_SRC_2             (MSDCPLL_FREQ/2)
#define MSDC0_SRC_3             156000000
#define MSDC0_SRC_4             182000000
#define MSDC0_SRC_5             312000000

#define MSDC1_SRC_0             26000000
#define MSDC1_SRC_1             208000000
#define MSDC1_SRC_2             182000000
#define MSDC1_SRC_3             0
#define MSDC1_SRC_4             (MSDCPLL_FREQ/2)

#define MSDC2_SRC_0             26000000
#define MSDC2_SRC_1             208000000
#define MSDC2_SRC_2             182000000
#define MSDC2_SRC_3             0
#define MSDC2_SRC_4             (MSDCPLL_FREQ/2)

#define MSDC_SRC_FPGA           12000000

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*--------------------------------------------------------------------------*/
/* MSDC0~1 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE          gpio_base               /* 0x10005000 */
#define MSDC0_IO_PAD_BASE       (msdc_io_cfg_bases[0])  /* 0x11F30000 TL */
#define MSDC1_IO_PAD_BASE       (msdc_io_cfg_bases[1])  /* 0x11E80000 TM */
#define MSDC2_IO_PAD_BASE       (msdc_io_cfg_bases[2])  /* 0x11C50000 */

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
/* MSDC0 */
#define MSDC0_GPIO_MODE16       (MSDC_GPIO_BASE + 0x3F0)
#define MSDC0_GPIO_MODE17       (MSDC_GPIO_BASE + 0x400)
#define MSDC0_GPIO_IES_ADDR     (MSDC0_IO_PAD_BASE + 0x0)
#define MSDC0_GPIO_SMT_ADDR     (MSDC0_IO_PAD_BASE + 0x10)
#define MSDC0_GPIO_TDSEL0_ADDR  (MSDC0_IO_PAD_BASE + 0x20)
#define MSDC0_GPIO_RDSEL0_ADDR  (MSDC0_IO_PAD_BASE + 0x40)
#define MSDC0_GPIO_DRV0_ADDR    (MSDC0_IO_PAD_BASE + 0xA0)
#define MSDC0_GPIO_PUPD0_ADDR   (MSDC0_IO_PAD_BASE + 0xC0)
#define MSDC0_GPIO_PUPD1_ADDR   (MSDC0_IO_PAD_BASE + 0xD0)

/* MSDC1 */
#define MSDC1_GPIO_MODE4        (MSDC_GPIO_BASE + 0x330)
#define MSDC1_GPIO_MODE5        (MSDC_GPIO_BASE + 0x340)
#define MSDC1_GPIO_IES_ADDR     (MSDC1_IO_PAD_BASE + 0x0)
#define MSDC1_GPIO_SMT_ADDR     (MSDC1_IO_PAD_BASE + 0x10)
#define MSDC1_GPIO_TDSEL0_ADDR  (MSDC1_IO_PAD_BASE + 0x20)
#define MSDC1_GPIO_RDSEL0_ADDR  (MSDC1_IO_PAD_BASE + 0x40)
#define MSDC1_GPIO_DRV0_ADDR    (MSDC1_IO_PAD_BASE + 0xA0)
#define MSDC1_GPIO_PUPD0_ADDR   (MSDC1_IO_PAD_BASE + 0xC0)

/* MSDC2 */
#define MSDC2_GPIO_MODE15       (MSDC_GPIO_BASE    + 0x3E0)
#define MSDC2_GPIO_MODE16       (MSDC_GPIO_BASE    + 0x3F0)
#define MSDC2_GPIO_IES_ADDR     (MSDC2_IO_PAD_BASE + 0x0)
#define MSDC2_GPIO_SMT_ADDR     (MSDC2_IO_PAD_BASE + 0x10)
#define MSDC2_GPIO_TDSEL_ADDR   (MSDC2_IO_PAD_BASE + 0x20)
#define MSDC2_GPIO_RDSEL_ADDR   (MSDC2_IO_PAD_BASE + 0x40)
#define MSDC2_GPIO_DRV_ADDR     (MSDC2_IO_PAD_BASE + 0xA0)
#define MSDC2_GPIO_PUPD0_ADDR   (MSDC2_IO_PAD_BASE + 0xC0)
#define MSDC2_GPIO_PUPD1_ADDR   (MSDC2_IO_PAD_BASE + 0xD0)

/* MSDC0_GPIO_MODE16, 001b is msdc mode*/
#define MSDC0_MODE_DAT6_AUX1_MASK    (0x7 << 28)
#define MSDC0_MODE_DAT4_AUX1_MASK    (0x7 << 24)
#define MSDC0_MODE_DAT2_AUX1_MASK    (0x7 << 20)
#define MSDC0_MODE_CLK_AUX1_MASK     (0x7 << 16)
#define MSDC0_MODE_DAT0_AUX1_MASK    (0x7 << 12)
#define MSDC0_MODE_CMD_AUX1_MASK     (0x7 << 8)
/* MSDC0_GPIO_MODE16, 002b is msdc mode, Aux2*/
#define MSDC0_MODE_DAT3_AUX2_MASK MSDC0_MODE_DAT6_AUX1_MASK
#define MSDC0_MODE_DAT2_AUX2_MASK MSDC0_MODE_DAT4_AUX1_MASK
#define MSDC0_MODE_DAT5_AUX2_MASK MSDC0_MODE_DAT2_AUX1_MASK
#define MSDC0_MODE_DAT4_AUX2_MASK MSDC0_MODE_DAT0_AUX1_MASK
/* MSDC0_GPIO_MODE17, 001b is msdc mode */
#define MSDC0_MODE_RSTB_AUX1_MASK    (0x7 << 20)
#define MSDC0_MODE_DAT3_AUX1_MASK    (0x7 << 16)
#define MSDC0_MODE_DSL_AUX1_MASK     (0x7 << 12)
#define MSDC0_MODE_DAT7_AUX1_MASK    (0x7 << 8)
#define MSDC0_MODE_DAT5_AUX1_MASK    (0x7 << 4)
#define MSDC0_MODE_DAT1_AUX1_MASK    (0x7 << 0)
/* MSDC0_GPIO_MODE17, 002b is msdc mode, Aux2 */
#define MSDC0_MODE_DAT1_AUX2_MASK MSDC0_MODE_DAT3_AUX1_MASK
#define MSDC0_MODE_DAT0_AUX2_MASK MSDC0_MODE_DAT5_AUX1_MASK
#define MSDC0_MODE_DAT6_AUX2_MASK MSDC0_MODE_DAT1_AUX1_MASK

/* MSDC1_GPIO_MODE4, 0001b is msdc mode */
#define MSDC1_MODE_CMD_MASK     (0x7 << 28)
#define MSDC1_MODE_DAT3_MASK    (0x7 << 24)
#define MSDC1_MODE_CLK_MASK     (0x7 << 20)
/* MSDC1_GPIO_MODE5, 0001b is msdc mode */
#define MSDC1_MODE_DAT1_MASK    (0x7 << 8)
#define MSDC1_MODE_DAT2_MASK    (0x7 << 4)
#define MSDC1_MODE_DAT0_MASK    (0x7 << 0)

/* MSDC2_GPIO_MODE15, 0010b is msdc mode */
#define MSDC2_MODE_DAT1_MASK    (0x7 << 16)
#define MSDC2_MODE_CMD_MASK     (0x7 << 20)
#define MSDC2_MODE_CLK_MASK     (0x7 << 24)
#define MSDC2_MODE_DAT3_MASK    (0x7 << 28)
/* MSDC2_GPIO_MODE16, 0010b is msdc mode */
#define MSDC2_MODE_DAT0_MASK    (0x7 << 0)
#define MSDC2_MODE_DAT2_MASK    (0x7 << 4)

/* MSDC0 IES mask*/
#define MSDC0_IES_RSTB_MASK     (0x1  <<  4)
#define MSDC0_IES_DSL_MASK      (0x1  <<  3)
#define MSDC0_IES_CLK_MASK      (0x1  <<  2)
#define MSDC0_IES_DAT_MASK      (0x1  <<  1)
#define MSDC0_IES_CMD_MASK      (0x1  <<  0)
#define MSDC0_IES_ALL_MASK      (0x1F <<  0)
/* MSDC0 SMT mask*/
#define MSDC0_SMT_RSTB_MASK     (0x1 <<   4)
#define MSDC0_SMT_DSL_MASK      (0x1 <<   3)
#define MSDC0_SMT_CLK_MASK      (0x1 <<   2)
#define MSDC0_SMT_DAT_MASK      (0x1 <<   1)
#define MSDC0_SMT_CMD_MASK      (0x1 <<   0)
#define MSDC0_SMT_ALL_MASK      (0x1F <<  0)
/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL0_RSTB_MASK  (0xF  << 16)
#define MSDC0_TDSEL0_DSL_MASK   (0xF  << 12)
#define MSDC0_TDSEL0_CLK_MASK   (0xF  <<  8)
#define MSDC0_TDSEL0_DAT_MASK   (0xF  <<  4)
#define MSDC0_TDSEL0_CMD_MASK   (0xF  <<  0)
#define MSDC0_TDSEL0_ALL_MASK   (0xFFFFF << 0)
/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL0_RSTB_MASK  (0x3F << 24)
#define MSDC0_RDSEL0_DSL_MASK   (0x3F << 18)
#define MSDC0_RDSEL0_CLK_MASK   (0x3F << 12)
#define MSDC0_RDSEL0_DAT_MASK   (0x3F <<  6)
#define MSDC0_RDSEL0_CMD_MASK   (0x3F <<  0)
#define MSDC0_RDSEL0_ALL_MASK   (0x3FFFFFFF << 0)
/* MSDC0 SR mask*/
#define MSDC0_SR_DAT_MASK       (0x1  << 19)
#define MSDC0_SR_CMD_MASK       (0x1  << 15)
#define MSDC0_SR_CLK_MASK       (0x1  << 11)
#define MSDC0_SR_DSL_MASK       (0x1  <<  7)
#define MSDC0_SR_RSTB_MASK      (0x1  <<  3)
/* MSDC0 DRV mask*/
#define MSDC0_DRV0_RSTB_MASK     (0x7  << 16)
#define MSDC0_DRV0_DSL_MASK      (0x7  << 12)
#define MSDC0_DRV0_CLK_MASK      (0x7  <<  8)
#define MSDC0_DRV0_DAT_MASK      (0x7  <<  4)
#define MSDC0_DRV0_CMD_MASK      (0x7  <<  0)
#define MSDC0_DRV0_ALL_MASK      (0xFFFFF << 0)
/* MSDC0 PUPD mask */
#define MSDC0_PUPD0_DAT5_MASK    (0x7  << 28)
#define MSDC0_PUPD0_DAT1_MASK    (0x7  << 24)
#define MSDC0_PUPD0_DAT6_MASK    (0x7  << 20)
#define MSDC0_PUPD0_DAT4_MASK    (0x7  << 16)
#define MSDC0_PUPD0_DAT2_MASK    (0x7  << 12)
#define MSDC0_PUPD0_CLK_MASK     (0x7  <<  8)
#define MSDC0_PUPD0_DAT0_MASK    (0x7  <<  4)
#define MSDC0_PUPD0_CMD_MASK     (0x7  <<  0)
#define MSDC0_PUPD0_MASK         (0xFFFFFFFF << 0)
#define MSDC0_PUPD1_RSTB_MASK    (0x7  <<  12)
#define MSDC0_PUPD1_DAT3_MASK    (0x7  <<   8)
#define MSDC0_PUPD1_DSL_MASK     (0x7  <<   4)
#define MSDC0_PUPD1_DAT7_MASK    (0x7  <<   0)
#define MSDC0_PUPD1_MASK         (0x7FFF << 0)

/* MSDC1 IES mask*/
#define MSDC1_IES_CMD_MASK      (0x1 <<  6)
#define MSDC1_IES_DAT_MASK      (0x1 <<  5)
#define MSDC1_IES_CLK_MASK      (0x1 <<  4)
#define MSDC1_IES_ALL_MASK      (0x7 <<  4)
/* MSDC1 SMT mask*/
#define MSDC1_SMT_CMD_MASK      (0x1 <<  6)
#define MSDC1_SMT_DAT_MASK      (0x1 <<  5)
#define MSDC1_SMT_CLK_MASK      (0x1 <<  4)
#define MSDC1_SMT_ALL_MASK      (0x7 <<  4)
/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL0_CMD_MASK   (0xF << 24)
#define MSDC1_TDSEL0_DAT_MASK   (0xF << 20)
#define MSDC1_TDSEL0_CLK_MASK   (0xF << 16)
#define MSDC1_TDSEL0_ALL_MASK   (0xFFF << 16)
/* MSDC1 RDSEL mask*/
#define MSDC1_RDSEL0_CMD_MASK   (0x3F << 20)
#define MSDC1_RDSEL0_DAT_MASK   (0x3F << 14)
#define MSDC1_RDSEL0_CLK_MASK   (0x3F << 8)
#define MSDC1_RDSEL0_ALL_MASK   (0x3FFFF << 8)
/* MSDC1 SR mask*/
#define MSDC1_SR_CMD_MASK       (0x1 << 27)
#define MSDC1_SR_DAT_MASK       (0x1 << 23)
#define MSDC1_SR_CLK_MASK       (0x1 << 19)
/* MSDC1 DRV mask*/
#define MSDC1_DRV0_CMD_MASK     (0x7 << 24)
#define MSDC1_DRV0_DAT_MASK     (0x7 << 20)
#define MSDC1_DRV0_CLK_MASK     (0x7 << 16)
#define MSDC1_DRV0_ALL_MASK     (0xFFF << 16)
/* MSDC1 PUPD mask */
#define MSDC1_PUPD0_DAT1_MASK   (0x7 << 20)
#define MSDC1_PUPD0_DAT2_MASK   (0x7 << 16)
#define MSDC1_PUPD0_DAT0_MASK   (0x7 << 12)
#define MSDC1_PUPD0_CMD_MASK    (0x7 << 8)
#define MSDC1_PUPD0_DAT3_MASK   (0x7 << 4)
#define MSDC1_PUPD0_CLK_MASK    (0x7 << 0)
#define MSDC1_PUPD0_MASK        (0xFFFFFF << 0)

/* MSDC2 IES mask*/
#define MSDC2_IES_DAT_MASK      (0x1 <<  3)
#define MSDC2_IES_CMD_MASK      (0x1 <<  4)
#define MSDC2_IES_CLK_MASK      (0x1 <<  5)
#define MSDC2_IES_ALL_MASK      (0x7 <<  3)
/* MSDC2 SMT mask*/
#define MSDC2_SMT_DAT_MASK      (0x1 <<  3)
#define MSDC2_SMT_CMD_MASK      (0x1 <<  4)
#define MSDC2_SMT_CLK_MASK      (0x1 <<  5)
#define MSDC2_SMT_ALL_MASK      (0x7 <<  3)
/* MSDC2 TDSEL mask */
#define MSDC2_TDSEL_DAT_MASK    (0xF << 12)
#define MSDC2_TDSEL_CMD_MASK    (0xF << 16)
#define MSDC2_TDSEL_CLK_MASK    (0xF << 20)
#define MSDC2_TDSEL_ALL_MASK    (0xFFF << 12)
/* MSDC2 RDSEL mask */
#define MSDC2_RDSEL_DAT_MASK    (0x3F << 10)
#define MSDC2_RDSEL_CMD_MASK    (0x3F << 16)
#define MSDC2_RDSEL_CLK_MASK    (0x3F << 22)
#define MSDC2_RDSEL_ALL_MASK    (0x3FFFF << 10)
/* MSDC2 SR mask */
/* Note: 16nm 1.8V IO does not have SR control */
/* MSDC2 DRV mask */
#define MSDC2_DRV_DAT_MASK      (0x7 << 12)
#define MSDC2_DRV_CMD_MASK      (0x7 << 16)
#define MSDC2_DRV_CLK_MASK      (0x7 << 20)
#define MSDC2_DRV_ALL_MASK      (0x3FF << 12)
/* MSDC2 PUPD mask */
#define MSDC2_PUPD_DAT0_MASK    (0x7 << 12)
#define MSDC2_PUPD_CMD_MASK     (0x7 << 16)
#define MSDC2_PUPD_CLK_MASK     (0x7 << 20)
#define MSDC2_PUPD_DAT1_MASK    (0x7 << 24)
#define MSDC2_PUPD_DAT3_MASK    (0x7 << 28)
#define MSDC2_PUPD0_MASK        (0xFFFFF << 12)
#define MSDC2_PUPD_DAT2_MASK    (0x7 <<  0)
#define MSDC2_PUPD1_MASK        (0xF <<  0)

#define MSDC_HW_TRAPPING_ADDR   (MSDC_GPIO_BASE + 0x6E0)
#define MSDC_HW_TRAPPING_MASK   (0x3 << 25) /* 0: Aux2, other:Aux1 */

/* FOR msdc_io_check() */
#define MSDC1_PUPD_DAT0_ADDR    (MSDC1_IO_PAD_BASE + 0xC0)
#define MSDC1_PUPD_DAT1_ADDR    (MSDC1_IO_PAD_BASE + 0xC0)
#define MSDC1_PUPD_DAT2_ADDR    (MSDC1_IO_PAD_BASE + 0xC0)
#define MSDC_PU_10K             1
#define MSDC_PU_50K             2
#define MSDC_PD_10K             5
#define MSDC_PD_50K             6


/**************************************************************/
/* Section 5: Adjustable Driver Parameter                     */
/**************************************************************/
#define HOST_MAX_BLKSZ          (2048)

#define MSDC_OCR_AVAIL          (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33)
/* data timeout counter. 1048576 * 3 sclk. */
#define DEFAULT_DTOC            (3)

#define MAX_DMA_CNT             (4 * 1024 * 1024)
/* a WIFI transaction may be 50K */
#define MAX_DMA_CNT_SDIO        (0xFFFFFFFF - 255)
/* a LTE  transaction may be 128K */

#define MAX_HW_SGMTS            (MAX_BD_NUM)
#define MAX_PHY_SGMTS           (MAX_BD_NUM)
#define MAX_SGMT_SZ             (MAX_DMA_CNT)
#define MAX_SGMT_SZ_SDIO        (MAX_DMA_CNT_SDIO)

#define HOST_MAX_NUM            (3)
#ifdef CONFIG_PWR_LOSS_MTK_TEST
#define MAX_REQ_SZ              (512 * 65536)
#else
#define MAX_REQ_SZ              (4 * 1024 * 1024)
#endif

#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK           (200000000)
#else
#define HOST_MAX_MCLK           (200000000)
#endif
#define HOST_MIN_MCLK           (260000)


/* SD card, bad card handling settings */

/* if continuous data timeout reach the limit */
/* driver will force remove card */
#define MSDC_MAX_DATA_TIMEOUT_CONTINUOUS (100)

/* if continuous power cycle fail reach the limit */
/* driver will force remove card */
#define MSDC_MAX_POWER_CYCLE_FAIL_CONTINUOUS (3)


/* #define MSDC_HQA */
/* #define SDIO_HQA */

/**************************************************************/
/* Section 6: BBChip-depenent Tunnig Parameter                */
/**************************************************************/
#define EMMC_MAX_FREQ_DIV               4 /* lower frequence to 12.5M */
#define MSDC_CLKTXDLY                   0

#define MSDC0_DDR50_DDRCKD              1 /* FIX ME: may be removed */

#define VOL_CHG_CNT_DEFAULT_VAL         0x1F4 /* =500 */

#define MSDC_PB0_DEFAULT_VAL            0x403C0006
#define MSDC_PB1_DEFAULT_VAL            0xFFE20349

#define MSDC_PB2_DEFAULT_RESPWAITCNT    0x3
#define MSDC_PB2_DEFAULT_RESPSTENSEL    0x1
#define MSDC_PB2_DEFAULT_CRCSTSENSEL    0x1

/**************************************************************/
/* Section 7: SDIO host                                       */
/**************************************************************/
#ifdef CONFIG_MTK_COMBO_COMM
#include <mt-plat/mtk_wcn_cmb_stub.h>
#define CFG_DEV_SDIO                    2
#endif

#endif /* _MSDC_CUST_MT6763_H_ */
