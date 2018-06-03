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

#ifndef _MSDC_CUST_MT6763_H_
#define _MSDC_CUST_MT6763_H_

#include <dt-bindings/mmc/mt6763-msdc.h>
#include <dt-bindings/clock/mt6763-clk.h>



/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
/* Names used for device tree lookup */
#define DT_COMPATIBLE_NAME      "mediatek,mt6763-mmc"
#define MSDC0_CLK_NAME          "MSDC0-CLOCK"
#define MSDC1_CLK_NAME          "MSDC1-CLOCK"
#define MSDC2_CLK_NAME          "MSDC2-CLOCK"
#define MSDC0_IOCFG_NAME        "mediatek,iocfg_5"
#define MSDC1_IOCFG_NAME        "mediatek,iocfg_0"
#define MSDC2_IOCFG_NAME        "mediatek,iocfg_4"


/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#define SD_POWER_DEFAULT_ON	(0)

#include <mt-plat/upmu_common.h>

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
/*mt6355_hw.h*/
#define REG_VEMC_VOSEL_CAL      PMIC_RG_VEMC_VOCAL_ADDR
#define REG_VEMC_VOSEL          PMIC_RG_VEMC_VOSEL_ADDR
#define REG_VEMC_EN             PMIC_RG_LDO_VEMC_EN_ADDR
#define REG_VMC_VOSEL_CAL       PMIC_RG_VMC_VOCAL_ADDR
#define REG_VMC_VOSEL           PMIC_RG_VMC_VOSEL_ADDR
#define REG_VMC_EN              PMIC_RG_LDO_VMC_EN_ADDR
#define REG_VMCH_VOSEL_CAL      PMIC_RG_VMCH_VOCAL_ADDR
#define REG_VMCH_VOSEL          PMIC_RG_VMCH_VOSEL_ADDR
#define REG_VMCH_EN             PMIC_RG_LDO_VMCH_EN_ADDR

#define MASK_VEMC_VOSEL_CAL     PMIC_RG_VEMC_VOCAL_MASK
#define SHIFT_VEMC_VOSEL_CAL    PMIC_RG_VEMC_VOCAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL    (MASK_VEMC_VOSEL_CAL << SHIFT_VEMC_VOSEL_CAL)
#define MASK_VEMC_VOSEL         PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL        PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL        (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)
#define MASK_VEMC_EN            PMIC_RG_LDO_VEMC_EN_MASK
#define SHIFT_VEMC_EN           PMIC_RG_LDO_VEMC_EN_SHIFT
#define FIELD_VEMC_EN           (MASK_VEMC_EN << SHIFT_VEMC_EN)
#define MASK_VMC_VOSEL_CAL      PMIC_RG_VMC_VOCAL_MASK
#define SHIFT_VMC_VOSEL_CAL     PMIC_RG_VMC_VOCAL_SHIFT
#define MASK_VMC_VOSEL          PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL         PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL         (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)
#define MASK_VMC_EN             PMIC_RG_LDO_VMC_EN_MASK
#define SHIFT_VMC_EN            PMIC_RG_LDO_VMC_EN_SHIFT
#define FIELD_VMC_EN            (MASK_VMC_EN << SHIFT_VMC_EN)
#define MASK_VMCH_VOSEL_CAL     PMIC_RG_VMCH_VOCAL_MASK
#define SHIFT_VMCH_VOSEL_CAL    PMIC_RG_VMCH_VOCAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL    (MASK_VMCH_VOSEL_CAL << SHIFT_VMCH_VOSEL_CAL)
#define MASK_VMCH_VOSEL         PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL        PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL        (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)
#define MASK_VMCH_EN            PMIC_RG_LDO_VMCH_EN_MASK
#define SHIFT_VMCH_EN           PMIC_RG_LDO_VMCH_EN_SHIFT
#define FIELD_VMCH_EN           (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define REG_VMCH_OC_STATUS      PMIC_RG_INT_STATUS_VMCH_OC_ADDR
#define MASK_VMCH_OC_STATUS     PMIC_RG_INT_STATUS_VMCH_OC_MASK
#define SHIFT_VMCH_OC_STATUS    PMIC_RG_INT_STATUS_VMCH_OC_SHIFT
#define FIELD_VMCH_OC_STATUS    (MASK_VMCH_OC_STATUS << SHIFT_VMCH_OC_STATUS)

#define VEMC_VOSEL_3V           (11)
#define VEMC_VOSEL_3V3          (13)
#define VMC_VOSEL_1V8           (4)
#define VMC_VOSEL_2V8           (9)
#define VMC_VOSEL_3V            (11)
#define VMCH_VOSEL_3V           (11)
#define VMCH_VOSEL_3V3          (13)

#define REG_VCORE_VOSEL_SW      PMIC_RG_BUCK_VCORE_VOSEL_ADDR
#define VCORE_VOSEL_SW_MASK     PMIC_RG_BUCK_VCORE_VOSEL_MASK
#define VCORE_VOSEL_SW_SHIFT    PMIC_RG_BUCK_VCORE_VOSEL_SHIFT
#define REG_VIO18_CAL           PMIC_RG_VIO18_VOCAL_ADDR
#define VIO18_CAL_MASK          PMIC_RG_VIO18_VOCAL_MASK
#define VIO18_CAL_SHIFT         PMIC_RG_VIO18_VOCAL_SHIFT

#define EMMC_VOL_ACTUAL         VOL_3000
#define SD_VOL_ACTUAL           VOL_3000

#else

#define REG_VEMC_VOSEL_CAL      MT6351_PMIC_RG_VEMC_CAL_ADDR
#define REG_VEMC_VOSEL          MT6351_PMIC_RG_VEMC_VOSEL_ADDR
#define REG_VEMC_EN             MT6351_PMIC_RG_VEMC_EN_ADDR
#define REG_VMC_VOSEL_CAL       MT6351_PMIC_RG_VMC_CAL_ADDR
#define REG_VMC_VOSEL           MT6351_PMIC_RG_VMC_VOSEL_ADDR
#define REG_VMC_EN              MT6351_PMIC_RG_VMC_EN_ADDR
#define REG_VMCH_VOSEL_CAL      MT6351_PMIC_RG_VMCH_CAL_ADDR
#define REG_VMCH_VOSEL          MT6351_PMIC_RG_VMCH_VOSEL_ADDR
#define REG_VMCH_EN             MT6351_PMIC_RG_VMCH_EN_ADDR

#define MASK_VEMC_VOSEL_CAL     MT6351_PMIC_RG_VEMC_CAL_MASK
#define SHIFT_VEMC_VOSEL_CAL    MT6351_PMIC_RG_VEMC_CAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL    (MASK_VEMC_VOSEL_CAL << SHIFT_VEMC_VOSEL_CAL)
#define MASK_VEMC_VOSEL         MT6351_PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL        MT6351_PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL        (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)
#define MASK_VEMC_EN            MT6351_PMIC_RG_VEMC_EN_MASK
#define SHIFT_VEMC_EN           MT6351_PMIC_RG_VEMC_EN_SHIFT
#define FIELD_VEMC_EN           (MASK_VEMC_EN << SHIFT_VEMC_EN)
#define MASK_VMC_VOSEL_CAL      MT6351_PMIC_RG_VMC_CAL_MASK
#define SHIFT_VMC_VOSEL_CAL     MT6351_PMIC_RG_VMC_CAL_SHIFT
#define MASK_VMC_VOSEL          MT6351_PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL         MT6351_PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL         (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)
#define MASK_VMC_EN             MT6351_PMIC_RG_VMC_EN_MASK
#define SHIFT_VMC_EN            MT6351_PMIC_RG_VMC_EN_SHIFT
#define FIELD_VMC_EN            (MASK_VMC_EN << SHIFT_VMC_EN)
#define MASK_VMCH_VOSEL_CAL     MT6351_PMIC_RG_VMCH_CAL_MASK
#define SHIFT_VMCH_VOSEL_CAL    MT6351_PMIC_RG_VMCH_CAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL    (MASK_VMCH_VOSEL_CAL << SHIFT_VMCH_VOSEL_CAL)
#define MASK_VMCH_VOSEL         MT6351_PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL        MT6351_PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL        (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)
#define MASK_VMCH_EN            MT6351_PMIC_RG_VMCH_EN_MASK
#define SHIFT_VMCH_EN           MT6351_PMIC_RG_VMCH_EN_SHIFT
#define FIELD_VMCH_EN           (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define REG_VMCH_OC_STATUS      MT6351_PMIC_OC_STATUS_VMCH_ADDR
#define MASK_VMCH_OC_STATUS     MT6351_PMIC_OC_STATUS_VMCH_MASK
#define SHIFT_VMCH_OC_STATUS    MT6351_PMIC_OC_STATUS_VMCH_SHIFT
#define FIELD_VMCH_OC_STATUS    (MASK_VMCH_OC_STATUS << SHIFT_VMCH_OC_STATUS)

#define VEMC_VOSEL_CAL_mV(cal)  ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VEMC_VOSEL_3V           (0)
#define VEMC_VOSEL_3V3          (1)
#define VMC_VOSEL_1V8           (3)
#define VMC_VOSEL_2V8           (5)
#define VMC_VOSEL_3V            (6)
#define VMC_VOSEL_CAL_mV(cal)   ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VMCH_VOSEL_CAL_mV(cal)  ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VMCH_VOSEL_3V           (0)
#define VMCH_VOSEL_3V3          (1)

#define REG_VCORE_VOSEL_SW      MT6351_PMIC_BUCK_VCORE_VOSEL_ADDR
#define VCORE_VOSEL_SW_MASK     MT6351_PMIC_BUCK_VCORE_VOSEL_MASK
#define VCORE_VOSEL_SW_SHIFT    MT6351_PMIC_BUCK_VCORE_VOSEL_SHIFT
#define REG_VCORE_VOSEL_HW      MT6351_PMIC_BUCK_VCORE_VOSEL_ON_ADDR
#define VCORE_VOSEL_HW_MASK     MT6351_PMIC_BUCK_VCORE_VOSEL_ON_MASK
#define VCORE_VOSEL_HW_SHIFT    MT6351_PMIC_BUCK_VCORE_VOSEL_ON_SHIFT
#define REG_VIO18_CAL           MT6351_PMIC_RG_VIO18_CAL_ADDR
#define VIO18_CAL_MASK          MT6351_PMIC_RG_VIO18_CAL_MASK
#define VIO18_CAL_SHIFT         MT6351_PMIC_RG_VIO18_CAL_SHIFT

#define EMMC_VOL_ACTUAL         VOL_2900
#define SD_VOL_ACTUAL           VOL_3000

#endif

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/* MSDCPLL register offset */
#define MSDCPLL_CON0_OFFSET     (0x250)
#define MSDCPLL_CON1_OFFSET     (0x254)
#define MSDCPLL_CON2_OFFSET     (0x258)
#define MSDCPLL_PWR_CON0_OFFSET (0x25c)
/* Clock config register offset */
#define MSDC_CLK_CFG_3_OFFSET   (0x070)
#define MSDC_CLK_CFG_4_OFFSET   (0x080)

#define MSDC_PERI_PDN_SET0_OFFSET       (0x0008)
#define MSDC_PERI_PDN_CLR0_OFFSET       (0x0010)
#define MSDC_PERI_PDN_STA0_OFFSET       (0x0018)
#endif

#define MSDC0_SRC_0             260000
#define MSDC0_SRC_1             400000000
#define MSDC0_SRC_2             200000000
#define MSDC0_SRC_3             156000000
#define MSDC0_SRC_4             182000000
#define MSDC0_SRC_5             156000000
#define MSDC0_SRC_6             100000000
#define MSDC0_SRC_7             624000000
#define MSDC0_SRC_8             312000000

#define MSDC1_SRC_0             260000
#define MSDC1_SRC_1             208000000
#define MSDC1_SRC_2             100000000
#define MSDC1_SRC_3             156000000
#define MSDC1_SRC_4             182000000
#define MSDC1_SRC_5             156000000
#define MSDC1_SRC_6             178000000
#define MSDC1_SRC_7             200000000

#define MSDC_SRC_FPGA		12000000

#define MSDC0_CG_NAME           MT_CG_INFRA0_MSDC0_CG_STA
#define MSDC1_CG_NAME           MT_CG_INFRA0_MSDC1_CG_STA
#define MSDC2_CG_NAME           MT_CG_INFRA0_MSDC2_CG_STA


/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*--------------------------------------------------------------------------*/
/* MSDC0~1 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE          gpio_base
#define MSDC0_IO_PAD_BASE       (msdc_io_cfg_bases[0])
#define MSDC1_IO_PAD_BASE       (msdc_io_cfg_bases[1])
#define MSDC2_IO_PAD_BASE       (msdc_io_cfg_bases[2])

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
/* MSDC0 */
#define MSDC0_GPIO_MODE23       (MSDC_GPIO_BASE   +  0x460)
#define MSDC0_GPIO_MODE24       (MSDC_GPIO_BASE   +  0x470)
#define MSDC0_GPIO_SMT_ADDR     (MSDC0_IO_PAD_BASE + 0x10)
#define MSDC0_GPIO_IES_ADDR     (MSDC0_IO_PAD_BASE + 0x0)
#define MSDC0_GPIO_TDSEL_ADDR   (MSDC0_IO_PAD_BASE + 0x20)
#define MSDC0_GPIO_RDSEL_ADDR   (MSDC0_IO_PAD_BASE + 0x40)
#define MSDC0_GPIO_DRV_ADDR     (MSDC0_IO_PAD_BASE + 0xa0)
#define MSDC0_GPIO_PUPD0_ADDR   (MSDC0_IO_PAD_BASE + 0xc0)
#define MSDC0_GPIO_PUPD1_ADDR   (MSDC0_IO_PAD_BASE + 0xd0)

/* MSDC1 */
#define MSDC1_GPIO_MODE4        (MSDC_GPIO_BASE   +  0x330)
#define MSDC1_GPIO_MODE5        (MSDC_GPIO_BASE   +  0x340)
#define MSDC1_GPIO_IES_ADDR     (MSDC1_IO_PAD_BASE + 0x0)
#define MSDC1_GPIO_SMT_ADDR     (MSDC1_IO_PAD_BASE + 0x10)
#define MSDC1_GPIO_TDSEL_ADDR   (MSDC1_IO_PAD_BASE + 0x30)
#define MSDC1_GPIO_RDSEL0_ADDR  (MSDC1_IO_PAD_BASE + 0x40)
#define MSDC1_GPIO_RDSEL1_ADDR  (MSDC1_IO_PAD_BASE + 0x50)
/* SR is in MSDC1_GPIO_DRV1_ADDR */
#define MSDC1_GPIO_DRV1_ADDR    (MSDC1_IO_PAD_BASE + 0xb0)
#define MSDC1_GPIO_PUPD_ADDR    (MSDC1_IO_PAD_BASE + 0xc0)

/* MSDC2 */
#define MSDC2_GPIO_MODE17       (MSDC_GPIO_BASE + 0x400)
#define MSDC2_GPIO_MODE18       (MSDC_GPIO_BASE + 0x410)
#define MSDC2_GPIO_SMT_ADDR     (MSDC2_IO_PAD_BASE + 0x10)
#define MSDC2_GPIO_IES_ADDR     (MSDC2_IO_PAD_BASE + 0x0)
#define MSDC2_GPIO_TDSEL_ADDR   (MSDC2_IO_PAD_BASE + 0x20)
#define MSDC2_GPIO_RDSEL_ADDR   (MSDC2_IO_PAD_BASE + 0x40)
#define MSDC2_GPIO_DRV_ADDR     (MSDC2_IO_PAD_BASE + 0xA0)
#define MSDC2_GPIO_PUPD0_ADDR   (MSDC2_IO_PAD_BASE + 0xC0)
#define MSDC2_GPIO_PUPD1_ADDR   (MSDC2_IO_PAD_BASE + 0xD0)

/* MSDC0_GPIO_MODE23, 001b is msdc mode*/
#define MSDC0_MODE_DAT6_MASK    (0x7 << 28)
#define MSDC0_MODE_DAT5_MASK    (0x7 << 24)
#define MSDC0_MODE_DAT1_MASK    (0x7 << 20)
#define MSDC0_MODE_CMD_MASK     (0x7 << 16)
#define MSDC0_MODE_CLK_MASK     (0x7 << 12)
#define MSDC0_MODE_DAT0_MASK    (0x7 << 8)

/* MSDC0_GPIO_MODE24, 001b is msdc mode */
#define MSDC0_MODE_DSL_MASK     (0x7 << 20)
#define MSDC0_MODE_DAT7_MASK    (0x7 << 16)
#define MSDC0_MODE_DAT3_MASK    (0x7 << 12)
#define MSDC0_MODE_DAT2_MASK    (0x7 << 8)
#define MSDC0_MODE_RSTB_MASK    (0x7 << 4)
#define MSDC0_MODE_DAT4_MASK    (0x7 << 0)

/* MSDC1_GPIO_MODE4, 0001b is msdc mode */
#define MSDC1_MODE_DAT3_MASK    (0x7 << 28)
#define MSDC1_MODE_CLK_MASK     (0x7 << 24)

/* MSDC1_GPIO_MODE5, 0001b is msdc mode */
#define MSDC1_MODE_DAT1_MASK    (0x7 << 12)
#define MSDC1_MODE_DAT2_MASK    (0x7 << 8)
#define MSDC1_MODE_DAT0_MASK    (0x7 << 4)
#define MSDC1_MODE_CMD_MASK     (0x7 << 0)

/* MSDC2_GPIO_MODE17, 0002b is msdc mode */
#define MSDC2_MODE_DAT0_MASK    (0x7 << 28)
#define MSDC2_MODE_DAT3_MASK    (0x7 << 24)
#define MSDC2_MODE_CLK_MASK     (0x7 << 20)
#define MSDC2_MODE_CMD_MASK     (0x7 << 16)
#define MSDC2_MODE_DAT1_MASK    (0x7 << 12)

/* MSDC2_GPIO_MODE18, 0002b is msdc mode */
#define MSDC2_MODE_DAT2_MASK    (0x7 << 0)

/* MSDC0 SMT mask*/
#define MSDC0_SMT_DSL_MASK      (0x1  <<  4)
#define MSDC0_SMT_RSTB_MASK     (0x1  <<  3)
#define MSDC0_SMT_CMD_MASK      (0x1  <<  2)
#define MSDC0_SMT_CLK_MASK      (0x1  <<  1)
#define MSDC0_SMT_DAT_MASK      (0x1  <<  0)
#define MSDC0_SMT_ALL_MASK      (0x1f <<  0)
/* MSDC1 SMT mask*/
#define MSDC1_SMT_DAT_MASK      (0x1 << 10)
#define MSDC1_SMT_CMD_MASK      (0x1 << 9)
#define MSDC1_SMT_CLK_MASK      (0x1 << 8)
#define MSDC1_SMT_ALL_MASK      (0x7 << 8)
/* MSDC2 SMT mask */
#define MSDC2_SMT_CLK_MASK      (0x1 << 5)
#define MSDC2_SMT_CMD_MASK      (0x1 << 4)
#define MSDC2_SMT_DAT_MASK      (0x1 << 3)
#define MSDC2_SMT_ALL_MASK      (0x7 << 3)

/* MSDC0 PUPD0 mask */
/* MSDC0 PUPD1 mask */
/* MSDC1 PUPD mask*/
/* No need add pin define, because it change everytime */
/* MSDC1 BIAS Tune mask */
#define MSDC1_BIAS_MASK         (0xf << 24)

/* MSDC0 IES mask*/
#define MSDC0_IES_DSL_MASK      (0x1  <<  4)
#define MSDC0_IES_RSTB_MASK     (0x1  <<  3)
#define MSDC0_IES_CMD_MASK      (0x1  <<  2)
#define MSDC0_IES_CLK_MASK      (0x1  <<  1)
#define MSDC0_IES_DAT_MASK      (0x1  <<  0)
#define MSDC0_IES_ALL_MASK      (0x1f <<  0)
/* MSDC1 IES mask*/
#define MSDC1_IES_DAT_MASK      (0x1 << 10)
#define MSDC1_IES_CMD_MASK      (0x1 <<  9)
#define MSDC1_IES_CLK_MASK      (0x1 <<  8)
#define MSDC1_IES_ALL_MASK      (0x7 <<  8)
/* MSDC2 IES mask*/
#define MSDC2_IES_CLK_MASK      (0x1 <<  5)
#define MSDC2_IES_CMD_MASK      (0x1 <<  4)
#define MSDC2_IES_DAT_MASK      (0x1 <<  3)
#define MSDC2_IES_ALL_MASK      (0x7 <<  3)

/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_DSL_MASK    (0xf  << 16)
#define MSDC0_TDSEL_RSTB_MASK   (0xf  << 12)
#define MSDC0_TDSEL_CMD_MASK    (0xf  <<  8)
#define MSDC0_TDSEL_CLK_MASK    (0xf  <<  4)
#define MSDC0_TDSEL_DAT_MASK    (0xf  <<  0)
#define MSDC0_TDSEL_ALL_MASK    (0xfffff << 0)
/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_DAT_MASK    (0xf <<  8)
#define MSDC1_TDSEL_CMD_MASK    (0xf <<  4)
#define MSDC1_TDSEL_CLK_MASK    (0xf <<  0)
#define MSDC1_TDSEL_ALL_MASK    (0xfff << 0)
/* MSDC2 TDSEL mask */
#define MSDC2_TDSEL_ALL_MASK    (0xfff << 12)

/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_DSL_MASK    (0x3f << 24)
#define MSDC0_RDSEL_RSTB_MASK   (0x3f << 18)
#define MSDC0_RDSEL_CMD_MASK    (0x3f << 12)
#define MSDC0_RDSEL_CLK_MASK    (0x3f <<  6)
#define MSDC0_RDSEL_DAT_MASK    (0x3F <<  0)
#define MSDC0_RDSEL_ALL_MASK    (0x3fffffff << 0)
/* MSDC1 RDSEL mask*/
#define MSDC1_RDSEL_CMD_MASK    (0x3f << 22)
#define MSDC1_RDSEL_CLK_MASK    (0x3f << 16)
#define MSDC1_RDSEL0_ALL_MASK   (0xfff << 16)
#define MSDC1_RDSEL_DAT_MASK    (0x3f << 0)
#define MSDC1_RDSEL1_ALL_MASK   (0x3f << 0)
/* MSDC2 RDSEL mask */
#define MSDC2_RDSEL_ALL_MASK    (0x3f << 6)

/* MSDC0 DRV mask*/
#define MSDC0_DRV_DSL_MASK      (0xf  <<  16)
#define MSDC0_DRV_RSTB_MASK     (0xf  <<  12)
#define MSDC0_DRV_CMD_MASK      (0xf  <<  8)
#define MSDC0_DRV_CLK_MASK      (0xf  <<  4)
#define MSDC0_DRV_DAT_MASK      (0xf  <<  0)
#define MSDC0_DRV_ALL_MASK      (0xfffff << 0)
/* MSDC1 SR mask*//* SR is in MSDC1_GPIO_DRV1_ADDR */
#define MSDC1_SR_CMD_MASK       (0x1 << 11)
#define MSDC1_SR_DAT_MASK       (0x1 << 7)
#define MSDC1_SR_CLK_MASK       (0x1 << 3)
/* MSDC1 DRV mask*/
#define MSDC1_DRV_CMD_MASK      (0x7 << 8)
#define MSDC1_DRV_DAT_MASK      (0x7 << 4)
#define MSDC1_DRV_CLK_MASK      (0x7 << 0)
#define MSDC1_DRV_ALL_MASK      (0xfff << 0)
/* MSDC2 DRV mask */
#define MSDC2_DRV_CLK_MASK      (0x7 << 20)
#define MSDC2_DRV_CMD_MASK      (0x7 << 16)
#define MSDC2_DRV_DAT_MASK      (0x7 << 12)
#define MSDC2_DRV_ALL_MASK      (0xfff << 12)

/* MSDC0 PUPD mask */
#define MSDC0_PUPD0_MASK        (0xFFFFFFFF << 0)
#define MSDC0_PUPD1_MASK        (0x7FFF << 0)
/* MSDC1 PUPD mask */
#define MSDC1_PUPD_MASK         (0x7FFFFF << 0)
/* MSDC2 PUPD mask */
#define MSDC2_PUPD0_MASK        (0xFFFFF << 12)
#define MSDC2_PUPD1_MASK        (0xF << 0)


/**************************************************************/
/* Section 5: Adjustable Driver Parameter                     */
/**************************************************************/
#define HOST_MAX_BLKSZ          (2048)

#define MSDC_OCR_AVAIL          (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 \
				| MMC_VDD_31_32 | MMC_VDD_32_33)
/* data timeout counter. 1048576 * 3 sclk. */
#define DEFAULT_DTOC            (3)

#define MAX_DMA_CNT             (64 * 1024 - 512)
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
#define MAX_REQ_SZ              (512 * 1024)
#endif

#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK           (200000000)
#else
#define HOST_MAX_MCLK           (200000000)
#endif
#define HOST_MIN_MCLK           (260000)

#endif /* _MSDC_CUST_MT6763_H_ */
