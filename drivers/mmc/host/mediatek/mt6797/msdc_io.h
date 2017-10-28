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

#ifndef _MSDC_IO_H_
#define _MSDC_IO_H_

/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
extern const struct of_device_id msdc_of_ids[];
extern struct msdc_hw *p_msdc_hw[];
extern unsigned int cd_gpio;
extern struct device_node *eint_node;

extern void __iomem *gpio_base;
extern void __iomem *msdc0_io_cfg_base;
extern void __iomem *msdc1_io_cfg_base;
extern void __iomem *msdc2_io_cfg_base;
extern void __iomem *msdc3_io_cfg_base;

extern void __iomem *infracfg_ao_reg_base;
/*
extern void __iomem *infracfg_reg_base;
*/
extern void __iomem *pericfg_reg_base;
/*
extern void __iomem *emi_reg_base;
extern void __iomem *toprgu_reg_base;
*/
extern void __iomem *apmixed_reg_base;
extern void __iomem *topckgen_reg_base;


int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc,
		unsigned int *cd_irq);

#ifdef FPGA_PLATFORM
void msdc_fpga_pwr_init(void);
#endif

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#include <mt-plat/upmu_common.h>

#define REG_VEMC_VOSEL_CAL              MT6351_PMIC_RG_VEMC_CAL_ADDR
#define REG_VEMC_VOSEL                  MT6351_PMIC_RG_VEMC_VOSEL_ADDR
#define REG_VEMC_EN                     MT6351_PMIC_RG_VEMC_EN_ADDR
#define REG_VMC_VOSEL_CAL		MT6351_PMIC_RG_VMC_CAL_ADDR
#define REG_VMC_VOSEL                   MT6351_PMIC_RG_VMC_VOSEL_ADDR
#define REG_VMC_EN                      MT6351_PMIC_RG_VMC_EN_ADDR
#define REG_VMCH_VOSEL_CAL              MT6351_PMIC_RG_VMCH_CAL_ADDR
#define REG_VMCH_VOSEL                  MT6351_PMIC_RG_VMCH_VOSEL_ADDR
#define REG_VMCH_EN                     MT6351_PMIC_RG_VMCH_EN_ADDR

#define MASK_VEMC_VOSEL_CAL             MT6351_PMIC_RG_VEMC_CAL_MASK
#define SHIFT_VEMC_VOSEL_CAL            MT6351_PMIC_RG_VEMC_CAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL            (MASK_VEMC_VOSEL_CAL \
						<< SHIFT_VEMC_VOSEL_CAL)
#define MASK_VEMC_VOSEL                 MT6351_PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL                MT6351_PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL                (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)
#define MASK_VEMC_EN                    MT6351_PMIC_RG_VEMC_EN_MASK
#define SHIFT_VEMC_EN                   MT6351_PMIC_RG_VEMC_EN_SHIFT
#define FIELD_VEMC_EN                   (MASK_VEMC_EN << SHIFT_VEMC_EN)
#define MASK_VMC_VOSEL_CAL		MT6351_PMIC_RG_VMC_CAL_MASK
#define SHIFT_VMC_VOSEL_CAL		MT6351_PMIC_RG_VMC_CAL_SHIFT
#define MASK_VMC_VOSEL                  MT6351_PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL                 MT6351_PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL                 (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)
#define MASK_VMC_EN                     MT6351_PMIC_RG_VMC_EN_MASK
#define SHIFT_VMC_EN                    MT6351_PMIC_RG_VMC_EN_SHIFT
#define FIELD_VMC_EN                    (MASK_VMC_EN << SHIFT_VMC_EN)
#define MASK_VMCH_VOSEL_CAL             MT6351_PMIC_RG_VMCH_CAL_MASK
#define SHIFT_VMCH_VOSEL_CAL            MT6351_PMIC_RG_VMCH_CAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL            (MASK_VMCH_VOSEL_CAL \
						<< SHIFT_VMCH_VOSEL_CAL)
#define MASK_VMCH_VOSEL                 MT6351_PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL                MT6351_PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL                (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)
#define MASK_VMCH_EN                    MT6351_PMIC_RG_VMCH_EN_MASK
#define SHIFT_VMCH_EN                   MT6351_PMIC_RG_VMCH_EN_SHIFT
#define FIELD_VMCH_EN                   (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define VEMC_VOSEL_CAL_mV(cal)          ((cal <= 0) ? \
						((0-(cal))/20) : (32-(cal)/20))
#define VEMC_VOSEL_3V                   (0)
#define VEMC_VOSEL_3V3                  (1)
#define VMC_VOSEL_1V8                   (3)
#define VMC_VOSEL_2V8                   (5)
#define VMC_VOSEL_3V                    (6)
#define VMC_VOSEL_CAL_mV(cal)          ((cal <= 0) ? \
						((0-(cal))/20) : (32-(cal)/20))
#define VMCH_VOSEL_CAL_mV(cal)          ((cal <= 0) ? \
						((0-(cal))/20) : (32-(cal)/20))
#define VMCH_VOSEL_3V                   (0)
#define VMCH_VOSEL_3V3                  (1)

#define REG_VCORE_VOSEL_SW		MT6351_PMIC_BUCK_VCORE_VOSEL_ADDR
#define	VCORE_VOSEL_SW_MASK		MT6351_PMIC_BUCK_VCORE_VOSEL_MASK
#define	VCORE_VOSEL_SW_SHIFT		MT6351_PMIC_BUCK_VCORE_VOSEL_SHIFT
#define REG_VCORE_VOSEL_HW		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_ADDR
#define	VCORE_VOSEL_HW_MASK		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_MASK
#define	VCORE_VOSEL_HW_SHIFT		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_SHIFT
#define REG_VIO18_CAL			MT6351_PMIC_RG_VIO18_CAL_ADDR
#define VIO18_CAL_MASK			MT6351_PMIC_RG_VIO18_CAL_MASK
#define VIO18_CAL_SHIFT			MT6351_PMIC_RG_VIO18_CAL_SHIFT

#define CARD_VOL_ACTUAL			VOL_2900

void msdc_sd_power_switch(struct msdc_host *host, u32 on);
#ifndef FPGA_PLATFORM
int msdc_io_check(struct msdc_host *host);
void msdc_emmc_power(struct msdc_host *host, u32 on);
void msdc_sd_power(struct msdc_host *host, u32 on);
void msdc_sdio_power(struct msdc_host *host, u32 on);
void msdc_dump_ldo_sts(struct msdc_host *host);
#else
void msdc_fpga_power(struct msdc_host *host, u32 on);
bool hwPowerOn_fpga(void);
bool hwPowerDown_fpga(void);
bool hwPowerDown_fpga(void);
#define msdc_emmc_power			msdc_fpga_power
#define msdc_sd_power			msdc_fpga_power
#define msdc_sdio_power			msdc_fpga_power
#define msdc_dump_ldo_sts(host)
#endif

extern u32 g_msdc0_io;
extern u32 g_msdc0_flash;
extern u32 g_msdc1_io;
extern u32 g_msdc1_flash;
extern u32 g_msdc2_io;
extern u32 g_msdc2_flash;
extern u32 g_msdc3_io;
extern u32 g_msdc3_flash;

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/

#define PLLCLK_50M  50000000
#define PLLCLK_400M 400000000
#define PLLCLK_200M 200000000
#define PLLCLK_208M 208000000
#define PLLCLK_364M 364000000
#define PLLCLK_156M 156000000
#define PLLCLK_182M 182000000
#define PLLCLK_312M 312000000
#define PLLCLK_273M 273000000
#define PLLCLK_178M 178290000
#define PLLCLK_416M 416000000
#define PLLCLK_78M  78000000

#ifdef FPGA_PLATFORM

extern  u32 hclks_msdc[];
#define msdc_dump_clock_sts(host)
#define msdc_get_hclks(host)		hclks_msdc
#define msdc_clk_enable(host) (0)
#define msdc_clk_disable(host)
#define msdc_get_ccf_clk_pointer(pdev, host)
#else /* FPGA_PLATFORM */

void msdc_dump_clock_sts(struct msdc_host *host);
/* MSDCPLL register offset */
#define MSDCPLL_CON0_OFFSET             (0x250)
#define MSDCPLL_CON1_OFFSET             (0x254)
#define MSDCPLL_CON2_OFFSET             (0x258)
#define MSDCPLL_PWR_CON0_OFFSET         (0x25c)
/* Clock config register offset */
#define MSDC_CLK_CFG_3_OFFSET           (0x070)

#define MSDC_PERI_PDN_SET0_OFFSET       (0x0008)
#define MSDC_PERI_PDN_CLR0_OFFSET       (0x0010)
#define MSDC_PERI_PDN_STA0_OFFSET       (0x0018)

extern u32 hclks_msdc50_0[];
extern u32 hclks_msdc30_1[];
extern u32 hclks_msdc30_2[];

#define msdc_get_hclks(id) \
	((id == 0) ? hclks_msdc50_0 : \
	((id == 1) ? hclks_msdc30_1 : hclks_msdc30_2))

unsigned int msdc_clk_enable(struct msdc_host *host);
#define msdc_clk_disable(host) clk_disable(host->clock_control)
int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
		struct msdc_host *host);
#endif /* FPGA_PLATFORM */

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*******************************************************************************
 *PINMUX and GPIO definition
 ******************************************************************************/
#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)
/* add pull down/up mode define */
#define MSDC_GPIO_PULL_UP       (0)
#define MSDC_GPIO_PULL_DOWN     (1)
/*--------------------------------------------------------------------------*/
/* MSDC0~1 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE		gpio_base
#define MSDC0_IO_PAD_BASE	(io_cfg_b_base)
#define MSDC1_IO_PAD_BASE	(io_cfg_b_base)
/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
#define MSDC0_GPIO_MODE14	(MSDC_GPIO_BASE   +  0x3e0)
#define MSDC0_GPIO_MODE15	(MSDC_GPIO_BASE   +  0x3f0)
#define MSDC1_GPIO_MODE16	(MSDC_GPIO_BASE   +  0x400)

#define MSDC0_GPIO_SMT_ADDR	(MSDC0_IO_PAD_BASE + 0x060)
#define MSDC0_GPIO_IES_ADDR	(MSDC0_IO_PAD_BASE + 0x020)
#define MSDC0_GPIO_PUPD_ADDR	(MSDC0_IO_PAD_BASE + 0x100)
#define MSDC0_GPIO_R0_ADDR	(MSDC0_IO_PAD_BASE + 0x110)
#define MSDC0_GPIO_R1_ADDR	(MSDC0_IO_PAD_BASE + 0x120)
#define MSDC0_GPIO_SR_ADDR	(MSDC0_IO_PAD_BASE + 0x030)
#define MSDC0_GPIO_TDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x0d0)
#define MSDC0_GPIO_RDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x080)
#define MSDC0_GPIO_DRV_ADDR	(MSDC0_IO_PAD_BASE + 0x1a0)
/* msdc1 smt is in IO_CFG_R register map */
#define MSDC1_GPIO_SMT_ADDR	(io_cfg_r_base     + 0x030)
#define MSDC1_GPIO_IES_ADDR	(MSDC1_IO_PAD_BASE + 0x020)
#define MSDC1_GPIO_PUPD_ADDR	(MSDC1_IO_PAD_BASE + 0x100)
#define MSDC1_GPIO_R0_ADDR	(MSDC1_IO_PAD_BASE + 0x110)
#define MSDC1_GPIO_R1_ADDR	(MSDC1_IO_PAD_BASE + 0x120)
#define MSDC1_GPIO_SR_ADDR	(MSDC1_IO_PAD_BASE + 0x030)
#define MSDC1_GPIO_TDSEL_ADDR	(MSDC1_IO_PAD_BASE + 0x0c0)
#define MSDC1_GPIO_RDSEL_ADDR	(MSDC1_IO_PAD_BASE + 0x0a0)
#define MSDC1_GPIO_DRV_ADDR	(MSDC1_IO_PAD_BASE + 0x1b0)
/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register Mask                                               */
/*--------------------------------------------------------------------------*/
/* MSDC0_GPIO_MODE14, 001b is msdc mode*/
#define MSDC0_MODE_DAT5_MASK            (0xf << 28)
#define MSDC0_MODE_DAT4_MASK            (0xf << 24)
#define MSDC0_MODE_DAT3_MASK            (0xf << 20)
#define MSDC0_MODE_DAT2_MASK            (0xf << 16)
#define MSDC0_MODE_DAT1_MASK            (0xf << 12)
#define MSDC0_MODE_DAT0_MASK            (0xf << 8)
/* MSDC0_GPIO_MODE15, 001b is msdc mode */
#define MSDC0_MODE_RSTB_MASK            (0xf << 20)
#define MSDC0_MODE_DSL_MASK             (0xf << 16)
#define MSDC0_MODE_CLK_MASK             (0xf << 12)
#define MSDC0_MODE_CMD_MASK             (0xf << 8)
#define MSDC0_MODE_DAT7_MASK            (0xf << 4)
#define MSDC0_MODE_DAT6_MASK            (0xf << 0)
/* MSDC1_GPIO_MODE16, 0001b is msdc mode */
#define MSDC1_MODE_CMD_MASK             (0xf << 4)
#define MSDC1_MODE_DAT0_MASK            (0xf << 8)
#define MSDC1_MODE_DAT1_MASK            (0xf << 12)
#define MSDC1_MODE_DAT2_MASK            (0xf << 16)
#define MSDC1_MODE_DAT3_MASK            (0xf << 20)
#define MSDC1_MODE_CLK_MASK             (0xf << 24)

/* MSDC0 SMT mask*/
#define MSDC0_SMT_DAT3_0_MASK            (0x1  <<  0)
#define MSDC0_SMT_DAT7_4_MASK            (0x1  <<  1)
#define MSDC0_SMT_CMD_DSL_RSTB_MASK      (0x1  <<  2)
#define MSDC0_SMT_CLK_MASK               (0x1  <<  3)
#define MSDC0_SMT_ALL_MASK               (0xf <<  0)
/* MSDC1 SMT mask. is in IO_CFG_R register map*/
#define MSDC1_SMT_CMD_MASK             (0x1 << 0)
#define MSDC1_SMT_DAT_MASK             (0x1 << 1)
#define MSDC1_SMT_CLK_MASK             (0x1 << 2)
#define MSDC1_SMT_ALL_MASK             (0x7 << 0)

/* MSDC0 IES mask*/
#define MSDC0_IES_DAT0_MASK              (0x1  <<  0)
#define MSDC0_IES_DAT1_MASK              (0x1  <<  1)
#define MSDC0_IES_DAT2_MASK              (0x1  <<  2)
#define MSDC0_IES_DAT3_MASK              (0x1  <<  3)
#define MSDC0_IES_DAT4_MASK              (0x1  <<  4)
#define MSDC0_IES_DAT5_MASK              (0x1  <<  5)
#define MSDC0_IES_DAT6_MASK              (0x1  <<  6)
#define MSDC0_IES_DAT7_MASK              (0x1  <<  7)
#define MSDC0_IES_CMD_MASK               (0x1  <<  8)
#define MSDC0_IES_CLK_MASK               (0x1  <<  9)
#define MSDC0_IES_DSL_MASK               (0x1  <<  10)
#define MSDC0_IES_RSTB_MASK              (0x1  <<  11)
#define MSDC0_IES_ALL_MASK               (0xfff << 0)
/* MSDC1 IES mask*/
#define MSDC1_IES_CMD_MASK             (0x1 << 25)
#define MSDC1_IES_DAT0_MASK            (0x1 << 26)
#define MSDC1_IES_DAT1_MASK            (0x1 << 27)
#define MSDC1_IES_DAT2_MASK            (0x1 << 28)
#define MSDC1_IES_DAT3_MASK            (0x1 << 29)
#define MSDC1_IES_CLK_MASK             (0x1 << 30)
#define MSDC1_IES_ALL_MASK             (0x3f << 25)

/* MSDC0 PUPD mask*/
#define MSDC0_PUPD_DAT0_MASK            (0x1  << 6)
#define MSDC0_PUPD_DAT1_MASK            (0x1  << 7)
#define MSDC0_PUPD_DAT2_MASK            (0x1  << 8)
#define MSDC0_PUPD_DAT3_MASK            (0x1  << 9)
#define MSDC0_PUPD_DAT4_MASK            (0x1  << 10)
#define MSDC0_PUPD_DAT5_MASK            (0x1  << 11)
#define MSDC0_PUPD_DAT6_MASK            (0x1  << 12)
#define MSDC0_PUPD_DAT7_MASK            (0x1  << 13)
#define MSDC0_PUPD_CMD_MASK             (0x1  << 14)
#define MSDC0_PUPD_CLK_MASK             (0x1  << 15)
#define MSDC0_PUPD_DSL_MASK             (0x1  << 16)
#define MSDC0_PUPD_RSTB_MASK            (0x1  << 17)
#define MSDC0_PUPD_DAT_MASK             (0xff << 6)
#define MSDC0_PUPD_CMD_DAT_MASK         (0x1FF << 6)
#define MSDC0_PUPD_CLK_DSL_MASK         (0x3  << 15)
#define MSDC0_PUPD_ALL_MASK             (0xfff << 6)
/* MSDC0 R0 mask*/
#define MSDC0_R0_DAT0_MASK            (0x1  << 6)
#define MSDC0_R0_DAT1_MASK            (0x1  << 7)
#define MSDC0_R0_DAT2_MASK            (0x1  << 8)
#define MSDC0_R0_DAT3_MASK            (0x1  << 9)
#define MSDC0_R0_DAT4_MASK            (0x1  << 10)
#define MSDC0_R0_DAT5_MASK            (0x1  << 11)
#define MSDC0_R0_DAT6_MASK            (0x1  << 12)
#define MSDC0_R0_DAT7_MASK            (0x1  << 13)
#define MSDC0_R0_CMD_MASK             (0x1  << 14)
#define MSDC0_R0_CLK_MASK             (0x1  << 15)
#define MSDC0_R0_DSL_MASK             (0x1  << 16)
#define MSDC0_R0_RSTB_MASK            (0x1  << 17)
#define MSDC0_R0_DAT_MASK             (0xff << 6)
#define MSDC0_R0_CMD_DAT_MASK         (0x1FF << 6)
#define MSDC0_R0_CLK_DSL_MASK         (0x3  << 15)
#define MSDC0_R0_ALL_MASK             (0xfff << 6)
/* MSDC0 R1 mask*/
#define MSDC0_R1_DAT0_MASK            (0x1  << 6)
#define MSDC0_R1_DAT1_MASK            (0x1  << 7)
#define MSDC0_R1_DAT2_MASK            (0x1  << 8)
#define MSDC0_R1_DAT3_MASK            (0x1  << 9)
#define MSDC0_R1_DAT4_MASK            (0x1  << 10)
#define MSDC0_R1_DAT5_MASK            (0x1  << 11)
#define MSDC0_R1_DAT6_MASK            (0x1  << 12)
#define MSDC0_R1_DAT7_MASK            (0x1  << 13)
#define MSDC0_R1_CMD_MASK             (0x1  << 14)
#define MSDC0_R1_CLK_MASK             (0x1  << 15)
#define MSDC0_R1_DSL_MASK             (0x1  << 16)
#define MSDC0_R1_RSTB_MASK            (0x1  << 17)
#define MSDC0_R1_DAT_MASK             (0xff << 6)
#define MSDC0_R1_CMD_DAT_MASK         (0x1FF << 6)
#define MSDC0_R1_CLK_DSL_MASK         (0x3  << 15)
#define MSDC0_R1_ALL_MASK             (0xfff << 6)
/* MSDC1 PUPD mask*/
#define MSDC1_PUPD_CMD_MASK             (0x1  << 18)
#define MSDC1_PUPD_DAT0_MASK            (0x1  << 19)
#define MSDC1_PUPD_DAT1_MASK            (0x1  << 20)
#define MSDC1_PUPD_DAT2_MASK            (0x1  << 21)
#define MSDC1_PUPD_DAT3_MASK            (0x1  << 22)
#define MSDC1_PUPD_CMD_DAT_MASK         (0x1f << 18)
#define MSDC1_PUPD_CLK_MASK             (0x1  << 23)
#define MSDC1_PUPD_ALL_MASK             (0x3f << 18)
/* MSDC1 R0 mask*/
#define MSDC1_R0_CMD_MASK             (0x1  << 18)
#define MSDC1_R0_DAT0_MASK            (0x1  << 19)
#define MSDC1_R0_DAT1_MASK            (0x1  << 20)
#define MSDC1_R0_DAT2_MASK            (0x1  << 21)
#define MSDC1_R0_DAT3_MASK            (0x1  << 22)
#define MSDC1_R0_CMD_DAT_MASK         (0x1f << 18)
#define MSDC1_R0_CLK_MASK             (0x1  << 23)
#define MSDC1_R0_ALL_MASK             (0x3f << 18)
/* MSDC1 R1 mask*/
#define MSDC1_R1_CMD_MASK             (0x1  << 18)
#define MSDC1_R1_DAT0_MASK            (0x1  << 19)
#define MSDC1_R1_DAT1_MASK            (0x1  << 20)
#define MSDC1_R1_DAT2_MASK            (0x1  << 21)
#define MSDC1_R1_DAT3_MASK            (0x1  << 22)
#define MSDC1_R1_CMD_DAT_MASK         (0x1f << 18)
#define MSDC1_R1_CLK_MASK             (0x1  << 23)
#define MSDC1_R1_ALL_MASK             (0x3f << 18)

/* MSDC0 SR mask*/
#define MSDC0_SR_DAT3_0_MASK            (0x1  << 15)
#define MSDC0_SR_DAT7_4_MASK            (0x1  << 16)
#define MSDC0_SR_CMD_DSL_RSTB_MASK      (0x1  << 17)
#define MSDC0_SR_CLK_MASK               (0x1  << 18)
#define MSDC0_SR_ALL_MASK               (0xf  << 15)
/* MSDC1 SR mask*/
#define MSDC1_SR_CMD_MASK               (0x1 << 29)
#define MSDC1_SR_DAT_MASK               (0x1 << 30)
#define MSDC1_SR_CLK_MASK               (0x1 << 31)

/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_DAT3_0_MASK         (0xf  <<  0)
#define MSDC0_TDSEL_DAT7_4_MASK         (0xf  <<  4)
#define MSDC0_TDSEL_CMD_DSL_RSTB_MASK   (0xf  <<  8)
#define MSDC0_TDSEL_CLK_MASK            (0xf  <<  12)
#define MSDC0_TDSEL_ALL_MASK            (0xffff << 0)
/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_CMD_MASK            (0xf << 16)
#define MSDC1_TDSEL_DAT_MASK            (0xf << 20)
#define MSDC1_TDSEL_CLK_MASK            (0xf << 24)
#define MSDC1_TDSEL_ALL_MASK            (0xfff << 16)

/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_DAT3_0_MASK         (0x3f <<  0)
#define MSDC0_RDSEL_DAT7_4_MASK         (0x3f <<  6)
#define MSDC0_RDSEL_CLK_CMD_DSL_RSTB_MASK  (0x3f <<  12)
#define MSDC0_RDSEL_ALL_MASK            (0x3ffff << 0)
/* MSDC1 RDSEL mask*/
#define MSDC1_RDSEL_CMD_MASK            (0x3f << 8)
#define MSDC1_RDSEL_DAT_MASK            (0x3f << 14)
#define MSDC1_RDSEL_CLK_MASK            (0x3f << 20)
#define MSDC1_RDSEL_ALL_MASK            (0x3ffff << 8)

/* MSDC0 DRV mask*/
#define MSDC0_DRV_DAT3_0_MASK           (0x7  <<  0)
#define MSDC0_DRV_DAT7_4_MASK           (0x7  <<  3)
#define MSDC0_DRV_DAT_MASK              (0x3f <<  0)
#define MSDC0_DRV_CMD_DSL_RSTB_MASK     (0x7  <<  6)
#define MSDC0_DRV_CLK_MASK              (0x7  <<  9)
#define MSDC0_DRV_ALL_MASK              (0xfff << 0)
/* MSDC1 DRV mask*/
#define MSDC1_DRV_CMD_MASK            (0x7 << 21)
#define MSDC1_DRV_DAT_MASK            (0x7 << 24)
#define MSDC1_DRV_CLK_MASK            (0x7 << 27)
#define MSDC1_DRV_ALL_MASK            (0x1ff << 21)
/* MSDC1 BIAS Tune mask */
#define MSDC1_BIAS_MASK               (0x1f << 16)

#ifndef FPGA_PLATFORM
void msdc_set_driving_by_id(u32 id, struct msdc_hw *hw, bool sd_18);
void msdc_set_driving(struct msdc_host *host, struct msdc_hw *hw, bool sd_18);
void msdc_get_driving_by_id(u32 id, struct msdc_hw *hw);
void msdc_set_ies_by_id(u32 id, int set_ies);
void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat);
void msdc_set_smt_by_id(u32 id, int set_smt);
void msdc_set_tdsel_by_id(u32 id, bool sleep, bool sd_18);
void msdc_set_rdsel_by_id(u32 id, bool sleep, bool sd_18);
void msdc_set_tdsel_dbg_by_id(u32 id, u32 value);
void msdc_set_rdsel_dbg_by_id(u32 id, u32 value);
void msdc_get_tdsel_dbg_by_id(u32 id, u32 *value);
void msdc_get_rdsel_dbg_by_id(u32 id, u32 *value);
void msdc_dump_padctl_by_id(u32 id);
void msdc_pin_config_by_id(u32 id, u32 mode);
#else
#define msdc_set_driving_by_id(id, hw, sd_18)
#define msdc_set_driving(host, hw, sd_18)
#define msdc_get_driving_by_id(host, hw)
#define msdc_set_ies_by_id(id, set_ies)
#define msdc_set_sr_by_id(id, clk, cmd, dat)
#define msdc_set_smt_by_id(id, set_smt)
#define msdc_set_tdsel_by_id(id, sleep, sd_18)
#define msdc_set_rdsel_by_id(id, sleep, sd_18)
#define msdc_set_tdsel_dbg_by_id(id, value)
#define msdc_set_rdsel_dbg_by_id(id, value)
#define msdc_get_tdsel_dbg_by_id(id, value)
#define msdc_get_rdsel_dbg_by_id(id, value)
#define msdc_dump_padctl_by_id(id)
#define msdc_pin_config_by_id(id, mode)
#define msdc_set_pin_mode(host)
#endif

#define msdc_get_driving(host, hw) \
	msdc_get_driving_by_id(host->id, hw)

#define msdc_set_ies(host, set_ies) \
	msdc_set_ies_by_id(host->id, set_ies)

#define msdc_set_sr(host, clk, cmd, dat) \
	msdc_set_sr_by_id(host->id, clk, cmd, dat)

#define msdc_set_smt(host, set_smt) \
	msdc_set_smt_by_id(host->id, set_smt)

#define msdc_set_tdsel(host, sleep, sd_18) \
	msdc_set_tdsel_by_id(host->id, sleep, sd_18)

#define msdc_set_rdsel(host, sleep, sd_18) \
	msdc_set_rdsel_by_id(host->id, sleep, sd_18)

#define msdc_set_tdsel_dbg(host, value) \
	msdc_set_tdsel_dbg_by_id(host->id, value)

#define msdc_set_rdsel_dbg(host, value) \
	msdc_set_rdsel_dbg_by_id(host->id, value)

#define msdc_get_tdsel_dbg(host, value) \
	msdc_get_tdsel_dbg_by_id(host->id, value)

#define msdc_get_rdsel_dbg(host, value) \
	msdc_get_rdsel_dbg_by_id(host->id, value)

#define msdc_dump_padctl(host) \
	msdc_dump_padctl_by_id(host->id)

#define msdc_pin_config(host, mode) \
	msdc_pin_config_by_id(host->id, mode)

/**************************************************************/
/* Section 5: MISC                                            */
/**************************************************************/
void dump_axi_bus_info(void);
void dump_emi_info(void);
void msdc_polling_axi_status(int line, int dead);

#endif /* end of _MSDC_IO_H_ */
