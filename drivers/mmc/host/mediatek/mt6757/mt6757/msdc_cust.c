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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "[" KBUILD_MODNAME "]" fmt

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

#include "dbg.h"
#include "mtk_sd.h"

struct msdc_host *mtk_msdc_host[] = {NULL, NULL, NULL};
EXPORT_SYMBOL(mtk_msdc_host);

int g_dma_debug[HOST_MAX_NUM] = {0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM] = {0, 0, 0};

unsigned int msdc_latest_transfer_mode[HOST_MAX_NUM] = {
	/* 0 for PIO; 1 for DMA; 3 for nothing */
	MODE_NONE, MODE_NONE, MODE_NONE,
};

unsigned int msdc_latest_op[HOST_MAX_NUM] = {
	/* 0 for read; 1 for write; 2 for nothing */
	OPER_TYPE_NUM, OPER_TYPE_NUM, OPER_TYPE_NUM};

/* for debug zone */
unsigned int sd_debug_zone[HOST_MAX_NUM] = {0, 0, 0};
/* for enable/disable register dump */
unsigned int sd_register_zone[HOST_MAX_NUM] = {1, 1, 1};
/* mode select */
u32 dma_size[HOST_MAX_NUM] = {512, 512, 512};

u32 drv_mode[HOST_MAX_NUM] = {
	MODE_SIZE_DEP, /* using DMA or not depend on the size */
	MODE_SIZE_DEP, MODE_SIZE_DEP};

u8 msdc_clock_src[HOST_MAX_NUM] = {0, 0, 0};

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{
	.compatible = DT_COMPATIBLE_NAME,
	},
	{},
};

void __iomem *gpio_base;

void __iomem *infracfg_ao_reg_base;
void __iomem *infracfg_reg_base;
void __iomem *apmixed_reg_base;
void __iomem *topckgen_reg_base;

void __iomem *msdc_io_cfg_bases[HOST_MAX_NUM];

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
int msdc_regulator_set_and_enable(struct regulator *reg, int powerVolt)
{
	regulator_set_voltage(reg, powerVolt, powerVolt);
	return regulator_enable(reg);
}

void msdc_ldo_power(u32 on, struct regulator *reg, int voltage_mv, u32 *status)
{
	int voltage_uv = voltage_mv * 1000;

	if (reg == NULL)
		return;

	if (on) {		    /* want to power on */
		if (*status == 0) { /* can power on */
			/*Comment out to reduce log */
			/* pr_debug("msdc power on<%d>\n", voltage_uv); */
			msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		} else if (*status == voltage_uv) {
			pr_debug("msdc power on <%d> again!\n", voltage_uv);
		} else {
			pr_debug("msdc change<%d> to <%d>\n", *status,
				 voltage_uv);
			regulator_disable(reg);
			msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		}
	} else {		    /* want to power off */
		if (*status != 0) { /* has been powerred on */
			/* Comment out to reduce log */
			/* pr_info("msdc power off\n"); */
			regulator_disable(reg);
			*status = 0;
		} else {
			pr_debug("msdc not power on\n");
		}
	}
}

void msdc_dump_ldo_sts(struct msdc_host *host)
{
	u32 ldo_en = 0, ldo_vol = 0, ldo_cal = 0;
	u32 id = host->id;

	switch (id) {
	case 0:
		pmic_read_interface_nolock(REG_VEMC_EN, &ldo_en, MASK_VEMC_EN,
					   SHIFT_VEMC_EN);
		pmic_read_interface_nolock(REG_VEMC_VOSEL, &ldo_vol,
					   MASK_VEMC_VOSEL, SHIFT_VEMC_VOSEL);
		pmic_read_interface_nolock(REG_VEMC_VOSEL_CAL, &ldo_cal,
					   MASK_VEMC_VOSEL_CAL,
					   SHIFT_VEMC_VOSEL_CAL);
		pr_debug(" VEMC_EN=0x%x, VEMC_VOL=0x%x [1b'0(3V),1b'1(3.3V)], "
			 "VEMC_CAL=0x%x\n",
			 ldo_en, ldo_vol, ldo_cal);
		break;
	case 1:
		pmic_read_interface_nolock(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
					   SHIFT_VMC_EN);
		pmic_read_interface_nolock(REG_VMC_VOSEL, &ldo_vol,
					   MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
		pmic_read_interface_nolock(REG_VMCH_VOSEL_CAL, &ldo_cal,
					   MASK_VMCH_VOSEL_CAL,
					   SHIFT_VMCH_VOSEL_CAL);
		pr_debug(" VMC_EN=0x%x, VMC_VOL=0x%x "
			 "[3b'101(2.9V),3b'011(1.8V)], VMC_CAL=0x%x\n",
			 ldo_en, ldo_vol, ldo_cal);

		pmic_read_interface_nolock(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
					   SHIFT_VMCH_EN);
		pmic_read_interface_nolock(REG_VMCH_VOSEL, &ldo_vol,
					   MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
		pmic_read_interface_nolock(REG_VMC_VOSEL_CAL, &ldo_cal,
					   MASK_VMC_VOSEL_CAL,
					   SHIFT_VMC_VOSEL_CAL);
		pr_debug(" VMCH_EN=0x%x, VMCH_VOL=0x%x [1b'0(3V),1b'1(3.3V)], "
			 "VMC_CAL=0x%x\n",
			 ldo_en, ldo_vol, ldo_cal);
		break;
	default:
		break;
	}
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	unsigned int reg_val;

	if (host->id == 1) {
		if (on) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
			/* VMC calibration +60mV. According to SA's request. */
			reg_val = 6;
#else
			/* VMC calibration +40mV. According to SA's request. */
			reg_val = host->vmc_cal_default - 2;
			/* 5 bit, if overflow such as 00000 -> 11110 is okay*/
			reg_val = reg_val & 0x1F;
#endif
			pmic_config_interface(REG_VMC_VOSEL_CAL, reg_val,
					      MASK_VMC_VOSEL_CAL,
					      SHIFT_VMC_VOSEL_CAL);
			msdc_set_sr(host, 0, 0, 0, 0, 0);
		}

		msdc_ldo_power(on, host->mmc->supply.vqmmc, VOL_1800,
			       &host->power_io);
		msdc_set_tdsel(host, MSDC_TDRDSEL_1V8, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_1V8, 0);
		host->hw->driving_applied = &host->hw->driving_sd_sdr50;
		msdc_set_driving(host, host->hw->driving_applied);
	}
}

void msdc_sdio_power(struct msdc_host *host, u32 on)
{
	if (host->id == 2) {
		host->power_flash = VOL_1800 * 1000;
		host->power_io = VOL_1800 * 1000;
	}
}

void msdc_power_calibration_init(struct msdc_host *host)
{
	if (host->hw->host_function == MSDC_EMMC) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
/* No need to calibrate, target is 3.0V */
#else
		/* Set to 3.0V - 100mV
		 * 4'b0000: 0 mV
		 * 4'b0001: -20 mV
		 * 4'b0010: -40 mV
		 * 4'b0011: -60 mV
		 * 4'b0100: -80 mV
		 * 4'b0101: -100 mV
		 */
		pmic_config_interface(
		    REG_VEMC_VOSEL_CAL,
		    VEMC_VOSEL_CAL_mV(EMMC_VOL_ACTUAL - VOL_3000),
		    MASK_VEMC_VOSEL_CAL, SHIFT_VEMC_VOSEL_CAL);
#endif
	} else if (host->hw->host_function == MSDC_SD) {
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
/* No need to calibrate, target is 3.0V */
#else
		u32 val = 0;

		/* VMCH vosel is 3.0V and calibration default is 0mV.
		 * Use calibration -100mv to get target VMCH = 2.9V
		 */
		pmic_read_interface(REG_VMCH_VOSEL_CAL, &val,
				    MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);

		pr_debug("msdc1, 0xACE=%x\n", val);

		if (val < 5)
			val = 0x20 + val - 5;
		else
			val = val - 5;

		pmic_config_interface(REG_VMCH_VOSEL_CAL, val,
				      MASK_VMCH_VOSEL_CAL,
				      SHIFT_VMCH_VOSEL_CAL);

		pr_debug("msdc1, 0xACE=%x\n", val);

		/* VMC vosel is 2.8V with some calibration value,
		 * Add extra 100mV calibration value to get tar VMC = 2.9V
		 */
		pmic_read_interface(REG_VMC_VOSEL_CAL, &val, MASK_VMC_VOSEL_CAL,
				    SHIFT_VMC_VOSEL_CAL);

		pr_debug("msdc1, 0xAE2=%x\n", val);

		if (val < 0x1b)
			val = 0x20 + val - 0x1b;
		else
			val = val - 0x1b;

		host->vmc_cal_default = val;
		pmic_config_interface(REG_VMC_VOSEL_CAL, val,
				      MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
#endif
	}
}

int msdc_oc_check(struct msdc_host *host)
{
	u32 val = 0;

	if (host->id == 1) {
		pmic_read_interface(REG_VMCH_OC_STATUS, &val,
				    MASK_VMCH_OC_STATUS, SHIFT_VMCH_OC_STATUS);

		if (val) {
			host->block_bad_card = 1;
			pr_debug("msdc1 OC status = %x\n", val);
			host->power_control(host, 0);
			return -1;
		}
	}
	return 0;
}

void msdc_emmc_power(struct msdc_host *host, u32 on)
{
	void __iomem *base = host->base;

	if (on == 0) {
		if ((MSDC_READ32(MSDC_PS) & 0x10000) != 0x10000)
			emmc_sleep_failed = 1;
	} else {
		msdc_set_driving(host, &host->hw->driving);
		msdc_set_tdsel(host, MSDC_TDRDSEL_1V8, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_1V8, 0);
	}

	msdc_ldo_power(on, host->mmc->supply.vmmc, VOL_3000,
		       &host->power_flash);

#if !defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	/* MT6351 need config PMIC */
	if (on && EMMC_VOL_ACTUAL != VOL_3000) {
		pmic_config_interface(
		    REG_VEMC_VOSEL_CAL,
		    VEMC_VOSEL_CAL_mV(EMMC_VOL_ACTUAL - VOL_3000),
		    MASK_VEMC_VOSEL_CAL, SHIFT_VEMC_VOSEL_CAL);
	}
#endif

#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_ldo_sts(host);
#endif
}

void msdc_sd_power(struct msdc_host *host, u32 on)
{
	u32 card_on = on;

	switch (host->id) {
	case 1:
		msdc_set_driving(host, &host->hw->driving);
		msdc_set_tdsel(host, MSDC_TDRDSEL_3V, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_3V, 0);
		if (host->hw->flags & MSDC_SD_NEED_POWER)
			card_on = 1;
		/* VMCH VOLSEL */
		msdc_ldo_power(card_on, host->mmc->supply.vmmc, VOL_3000,
			       &host->power_flash);
		/* VMC VOLSEL */
		msdc_ldo_power(on, host->mmc->supply.vqmmc, VOL_3000,
			       &host->power_io);
		if (on)
			msdc_set_sr(host, 0, 0, 0, 0, 0);
		break;

	default:
		break;
	}

#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_ldo_sts(host);
#endif
}

void msdc_sd_power_off(void)
{
	struct msdc_host *host = mtk_msdc_host[1];

	if (host) {
		pr_debug("Power Off, SD card\n");

		/* power must be on */
		host->power_io = VOL_3000 * 1000;
		host->power_flash = VOL_3000 * 1000;

		host->power_control(host, 0);

		msdc_set_bad_card_and_remove(host);
	}
}
EXPORT_SYMBOL(msdc_sd_power_off);
#endif /*if !defined(FPGA_PLATFORM)*/

void msdc_set_host_power_control(struct msdc_host *host)
{
	if (host->hw->host_function == MSDC_EMMC) {
		host->power_control = msdc_emmc_power;
	} else if (host->hw->host_function == MSDC_SD) {
		host->power_control = msdc_sd_power;
		host->power_switch = msdc_sd_power_switch;

#if SD_POWER_DEFAULT_ON
		/* If SD card power is default on, turn it off so that
		 * removable card slot won't keep power when no card plugged
		 */
		if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE)) {
			/* turn on first to match HW/SW state*/
			msdc_sd_power(host, 1);
			mdelay(10);
			msdc_sd_power(host, 0);
		}
#endif
	} else if (host->hw->host_function == MSDC_SDIO) {
		host->power_control = msdc_sdio_power;
	}

	if (host->power_control != NULL) {
		msdc_power_calibration_init(host);
	} else {
		ERR_MSG("Host function defination error for msdc%d", host->id);
		WARN_ON(1);
	}
}

void msdc_pmic_force_vcore_pwm(bool enable)
{
#ifndef FPGA_PLATFORM
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	if (enable)
		pmic_set_register_value(PMIC_RG_VCORE_FPWM,
					0x1); /* enable force pwm */
	else
		pmic_set_register_value(PMIC_RG_VCORE_FPWM, 0x0);
#else
	if (enable)
		pmic_force_vcore_pwm(true); /* set PWM mode for MT6351 */
	else
		pmic_force_vcore_pwm(false); /* set non-PWM mode for MT6351 */
#endif
#endif
}

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
u32 hclks_msdc50[] = {MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2,
		      MSDC0_SRC_3, MSDC0_SRC_4, MSDC0_SRC_5,
		      MSDC0_SRC_6, MSDC0_SRC_7, MSDC0_SRC_8};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc30[] = {MSDC1_SRC_0, MSDC1_SRC_1, MSDC1_SRC_2, MSDC1_SRC_3,
		      MSDC1_SRC_4, MSDC1_SRC_5, MSDC1_SRC_6, MSDC1_SRC_7};

u32 *hclks_msdc_all[] = {
	hclks_msdc50, hclks_msdc30, hclks_msdc30,
};
u32 *hclks_msdc;

#include <mtk_clk_id.h>

int msdc_cg_clk_id[HOST_MAX_NUM] = {MSDC0_CG_NAME, MSDC1_CG_NAME,
				    MSDC2_CG_NAME};

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
			     struct msdc_host *host)
{
	static char const *const clk_names[] = {MSDC0_CLK_NAME, MSDC1_CLK_NAME,
						MSDC2_CLK_NAME};

	host->clock_control = devm_clk_get(&pdev->dev, clk_names[pdev->id]);

	if (IS_ERR(host->clock_control)) {
		pr_debug("[msdc%d] can not get clock control\n", pdev->id);
		return 1;
	}
	if (clk_prepare(host->clock_control)) {
		pr_debug("[msdc%d] can not prepare clock control\n", pdev->id);
		return 1;
	}

	return 0;
}

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	if (host->id != 0) {
		pr_debug("[msdc%d] NOT Support switch pll souce[%s]%d\n",
			 host->id, __func__, __LINE__);
		return;
	}

	host->hclk = msdc_get_hclk(host->id, clksrc);
	host->hw->clk_src = clksrc;

	pr_debug("[%s]: msdc%d select clk_src as %d(%dKHz)\n", __func__,
		 host->id, clksrc, host->hclk / 1000);
}

void msdc_dump_clock_sts(struct msdc_host *host)
{
	if (topckgen_reg_base) {
		/* CLK_CFG_3 control msdc clock source PLL */
		pr_debug(" CLK_CFG_3 register address is 0x%p\n",
			 topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET);
		pr_debug(" bit[9~8]=01b,     bit[15]=0b\n");
		pr_debug(" bit[19~16]=0001b, bit[23]=0b\n");
		pr_debug(" bit[26~24]=111b, bit[31]=0b\n");
		pr_debug(
		    " Read value is       0x%x\n",
		    MSDC_READ32(topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET));

		/* CLK_CFG_4 control msdc clock source PLL */
		pr_debug(" CLK_CFG_4 register address is 0x%p\n",
			 topckgen_reg_base + MSDC_CLK_CFG_4_OFFSET);
		pr_debug(" bit[2~0]=111b,    bit[7]=0b\n");
		pr_debug(
		    " Read value is		 0x%x\n",
		    MSDC_READ32(topckgen_reg_base + MSDC_CLK_CFG_4_OFFSET));
	}
	if (apmixed_reg_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		pr_debug(" MSDCPLL_CON0_OFFSET register address is 0x%p\n",
			 apmixed_reg_base + MSDCPLL_CON0_OFFSET);
		pr_debug(" bit[0]=1b\n");
		pr_debug(" Read value is       0x%x\n",
			 MSDC_READ32(apmixed_reg_base + MSDCPLL_CON0_OFFSET));

		pr_debug(" MSDCPLL_CON1_OFFSET register address is 0x%p\n",
			 apmixed_reg_base + MSDCPLL_CON1_OFFSET);
		pr_debug(" Read value is       0x%x\n",
			 MSDC_READ32(apmixed_reg_base + MSDCPLL_CON1_OFFSET));

		pr_debug(" MSDCPLL_CON2_OFFSET register address is 0x%p\n",
			 apmixed_reg_base + MSDCPLL_CON2_OFFSET);
		pr_debug(" Read value is       0x%x\n",
			 MSDC_READ32(apmixed_reg_base + MSDCPLL_CON2_OFFSET));

		pr_debug(" MSDCPLL_PWR_CON0 register address is 0x%p\n",
			 apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET);
		pr_debug(" bit[0]=1b\n");
		pr_debug(
		    " Read value is       0x%x\n",
		    MSDC_READ32(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET));
	}
}

/* FIX ME, consider to remove it */
#include <linux/seq_file.h>
void dbg_msdc_dump_clock_sts(struct seq_file *m, struct msdc_host *host)
{
	if (topckgen_reg_base) {
		/* CLK_CFG_3 control msdc clock source PLL */
		seq_printf(m, " CLK_CFG_3 register address is 0x%p\n\n",
			   topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET);
		seq_puts(m, " bit[9~8]=01b,     bit[15]=0b\n");
		seq_puts(m, " bit[19~16]=0001b, bit[23]=0b\n");
		seq_puts(m, " bit[26~24]=0010b, bit[31]=0b\n");
		seq_printf(
		    m, " Read value is       0x%x\n",
		    MSDC_READ32(topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET));
	}
	if (apmixed_reg_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		seq_printf(m,
			   " MSDCPLL_CON0_OFFSET register address is 0x%p\n\n",
			   apmixed_reg_base + MSDCPLL_CON0_OFFSET);
		seq_puts(m, " bit[0]=1b\n");
		seq_printf(m, " Read value is       0x%x\n",
			   MSDC_READ32(apmixed_reg_base + MSDCPLL_CON0_OFFSET));

		seq_printf(m,
			   " MSDCPLL_CON1_OFFSET register address is 0x%p\n\n",
			   apmixed_reg_base + MSDCPLL_CON1_OFFSET);
		seq_printf(m, " Read value is       0x%x\n",
			   MSDC_READ32(apmixed_reg_base + MSDCPLL_CON1_OFFSET));

		seq_printf(m,
			   " MSDCPLL_CON2_OFFSET register address is 0x%p\n\n",
			   apmixed_reg_base + MSDCPLL_CON2_OFFSET);
		seq_printf(m, " Read value is       0x%x\n",
			   MSDC_READ32(apmixed_reg_base + MSDCPLL_CON2_OFFSET));

		seq_printf(m, " MSDCPLL_PWR_CON0 register address is 0x%p\n\n",
			   apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET);
		seq_puts(m, " bit[0]=1b\n");
		seq_printf(
		    m, " Read value is       0x%x\n",
		    MSDC_READ32(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET));
	}
}

void msdc_clk_status(int *status)
{
	int clk_gate = 0;
	int i = 0;
	unsigned long flags;

	for (i = 0; i < HOST_MAX_NUM; i++) {
		if (!mtk_msdc_host[i])
			continue;

		spin_lock_irqsave(&mtk_msdc_host[i]->clk_gate_lock, flags);
		if (mtk_msdc_host[i]->clk_gate_count > 0)
			clk_gate |= 1 << msdc_cg_clk_id[i];
		spin_unlock_irqrestore(&mtk_msdc_host[i]->clk_gate_lock, flags);
	}
	*status = clk_gate;
}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
void msdc_dump_vcore(void)
{
	if (vcorefs_get_hw_opp() == OPPI_PERF)
		pr_debug("%s: Vcore 0.8V\n", __func__);
	else
		pr_debug("%s: Vcore 0.7V\n", __func__);
}

void msdc_dump_padctl_by_id(u32 id)
{
	if (!gpio_base || !msdc_io_cfg_bases[id]) {
		pr_debug("err: gpio_base=%p, msdc_io_cfg_bases[%d]=%p\n",
			 gpio_base, id, msdc_io_cfg_bases[id]);
		return;
	}
	switch (id) {
	case 0:
		pr_debug("MSDC0 MODE23[0x%p] =0x%8x\tshould:0x111111??\n",
			 MSDC0_GPIO_MODE23, MSDC_READ32(MSDC0_GPIO_MODE23));
		pr_debug("MSDC0 MODE24[0x%p] =0x%8x\tshould:0x??111111\n",
			 MSDC0_GPIO_MODE24, MSDC_READ32(MSDC0_GPIO_MODE24));
		pr_debug("MSDC0 IES   [0x%p] =0x%8x\tshould:0x??????1f\n",
			 MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
		pr_debug("MSDC0 SMT   [0x%p] =0x%8x\tshould:0x???????f\n",
			 MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
		pr_debug("MSDC0 TDSEL [0x%p] =0x%8x\tshould:0x????0000\n",
			 MSDC0_GPIO_TDSEL_ADDR,
			 MSDC_READ32(MSDC0_GPIO_TDSEL_ADDR));
		pr_debug("MSDC0 RDSEL [0x%p] =0x%8x\tshould:0x???00000\n",
			 MSDC0_GPIO_RDSEL_ADDR,
			 MSDC_READ32(MSDC0_GPIO_RDSEL_ADDR));
		pr_debug("MSDC0 DRV   [0x%p] =0x%8x\tshould: 1001001001b\n",
			 MSDC0_GPIO_DRV_ADDR, MSDC_READ32(MSDC0_GPIO_DRV_ADDR));
		pr_debug("PUPD/R1/R0: dat/cmd:0/0/1, clk/dst: 1/1/0\n");
		pr_debug("PUPD/R1/R0: rstb: 0/1/0\n");
		pr_debug(
		    "MSDC0 PUPD0 [0x%p] =0x%8x\tshould: 0x21111161 ??????b\n",
		    MSDC0_GPIO_PUPD0_ADDR, MSDC_READ32(MSDC0_GPIO_PUPD0_ADDR));
		pr_debug("MSDC0 PUPD1 [0x%p] =0x%8x\tshould: 0x6111 ??????b\n",
			 MSDC0_GPIO_PUPD1_ADDR,
			 MSDC_READ32(MSDC0_GPIO_PUPD1_ADDR));
		break;
	case 1:
		pr_debug("MSDC1 MODE4 [0x%p] =0x%8x\n", MSDC1_GPIO_MODE4,
			 MSDC_READ32(MSDC1_GPIO_MODE4));
		pr_debug("MSDC1 MODE5 [0x%p] =0x%8x\n", MSDC1_GPIO_MODE5,
			 MSDC_READ32(MSDC1_GPIO_MODE5));
		pr_debug("MSDC1 IES    [0x%p] =0x%8x\n", MSDC1_GPIO_IES_ADDR,
			 MSDC_READ32(MSDC1_GPIO_IES_ADDR));
		pr_debug("MSDC1 SMT    [0x%p] =0x%8x\n", MSDC1_GPIO_SMT_ADDR,
			 MSDC_READ32(MSDC1_GPIO_SMT_ADDR));

		pr_debug("MSDC1 TDSEL  [0x%p] =0x%8x\n", MSDC1_GPIO_TDSEL_ADDR,
			 MSDC_READ32(MSDC1_GPIO_TDSEL_ADDR));
		/* Note1=> For Vcore=0.7V sleep mode
		 * if TDSEL0~3 don't set to [1111]
		 * the digital output function will fail
		 */
		pr_debug("should 1.8v: sleep:0x?fff????, awake:0x?aaa????\n");
		pr_debug("MSDC1 RDSEL0  [0x%p] =0x%8x\n",
			 MSDC1_GPIO_RDSEL0_ADDR,
			 MSDC_READ32(MSDC1_GPIO_RDSEL0_ADDR));
		pr_debug("MSDC1 RDSEL1  [0x%p] =0x%8x\n",
			 MSDC1_GPIO_RDSEL1_ADDR,
			 MSDC_READ32(MSDC1_GPIO_RDSEL1_ADDR));
		pr_debug("1.8V: ??0000??, 2.9v: 0x??c30c??\n");

		pr_debug("MSDC1 DRV1    [0x%p] =0x%8x\n", MSDC1_GPIO_DRV1_ADDR,
			 MSDC_READ32(MSDC1_GPIO_DRV1_ADDR));

		pr_debug("MSDC1 PUPD   [0x%p] =0x%8x\n", MSDC1_GPIO_PUPD_ADDR,
			 MSDC_READ32(MSDC1_GPIO_PUPD_ADDR));
		break;

#if defined(CFG_DEV_MSDC2)
	case 2:
		pr_debug("MSDC2 MODE17 [0x%p] =0x%8x\n", MSDC2_GPIO_MODE17,
			 MSDC_READ32(MSDC2_GPIO_MODE17));
		pr_debug("MSDC2 MODE18 [0x%p] =0x%8x\n", MSDC2_GPIO_MODE18,
			 MSDC_READ32(MSDC2_GPIO_MODE18));
		pr_debug("MSDC2 IES   [0x%p] =0x%8x\tshould:0x??????38\n",
			 MSDC2_GPIO_IES_ADDR, MSDC_READ32(MSDC2_GPIO_IES_ADDR));
		pr_debug("MSDC2 SMT    [0x%p] =0x%8x\n", MSDC2_GPIO_SMT_ADDR,
			 MSDC_READ32(MSDC2_GPIO_SMT_ADDR));

		pr_debug("MSDC2 TDSEL  [0x%p] =0x%8x\n", MSDC2_GPIO_TDSEL_ADDR,
			 MSDC_READ32(MSDC2_GPIO_TDSEL_ADDR));
		/* Note1=> For Vcore=0.7V sleep mode
		 * if TDSEL0~3 don't set to [1111]
		 * the digital output function will fail
		 */
		pr_debug("MSDC2 RDSEL  [0x%p] =0x%8x\n", MSDC2_GPIO_RDSEL_ADDR,
			 MSDC_READ32(MSDC2_GPIO_RDSEL_ADDR));

		pr_debug("MSDC2 DRV    [0x%p] =0x%8x\n", MSDC2_GPIO_DRV_ADDR,
			 MSDC_READ32(MSDC2_GPIO_DRV_ADDR));

		pr_debug("MSDC2 PUPD0   [0x%p] =0x%8x\n", MSDC2_GPIO_PUPD0_ADDR,
			 MSDC_READ32(MSDC2_GPIO_PUPD0_ADDR));
		pr_debug("MSDC2 PUPD1   [0x%p] =0x%8x\n", MSDC2_GPIO_PUPD1_ADDR,
			 MSDC_READ32(MSDC2_GPIO_PUPD1_ADDR));
		break;
#endif

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_pin_mode(struct msdc_host *host)
{
	switch (host->id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_DAT6_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_DAT5_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_DAT1_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_CMD_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_CLK_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_DAT0_MASK, 0x1);

		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DSL_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT7_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT3_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT2_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_RSTB_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT4_MASK, 0x1);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, MSDC1_MODE_DAT3_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, MSDC1_MODE_CLK_MASK, 0x1);

		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT1_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT2_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT0_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_CMD_MASK, 0x1);
		break;

	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT0_MASK, 0x2);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT3_MASK, 0x2);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_CLK_MASK, 0x2);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_CMD_MASK, 0x2);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT1_MASK, 0x2);

		MSDC_SET_FIELD(MSDC2_GPIO_MODE18, MSDC2_MODE_DAT2_MASK, 0x2);
		break;

	default:
		pr_debug("[%s] invalid host->id = %d\n", __func__, host->id);
		break;
	}
}

void msdc_set_ies_by_id(u32 id, int set_ies)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_IES_ADDR, MSDC0_IES_ALL_MASK,
			       (set_ies ? 0x1F : 0));
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_IES_ADDR, MSDC1_IES_ALL_MASK,
			       (set_ies ? 0x7 : 0));
		break;

	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_IES_ADDR, MSDC2_IES_ALL_MASK,
			       (set_ies ? 0x7 : 0));
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_smt_by_id(u32 id, int set_smt)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
			       (set_smt ? 0x1F : 0));
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_SMT_ADDR, MSDC1_SMT_ALL_MASK,
			       (set_smt ? 0x7 : 0));
		break;

	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_SMT_ADDR, MSDC2_SMT_ALL_MASK,
			       (set_smt ? 0x7 : 0));
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_tdsel_by_id(u32 id, u32 flag, u32 value)
{
	u32 cust_val;

	switch (id) {
	case 0:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK,
			       cust_val);
		break;
	case 1:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK,
			       cust_val);
		break;

	case 2:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK,
			       cust_val);
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_rdsel_by_id(u32 id, u32 flag, u32 value)
{
	u32 cust_val;

	switch (id) {
	case 0:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK,
			       cust_val);
		break;
	case 1:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK,
			       cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK,
			       cust_val);
		break;

	case 2:
		if (flag == MSDC_TDRDSEL_CUST) {
			/* FIX ME, regard value as one pin value and then
			 * duplicate as all pin values
			 */
			cust_val = value;
		} else {
			cust_val = 0;
		}
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_ALL_MASK,
			       cust_val);
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_tdsel_by_id(u32 id, u32 *value)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK,
			       *value);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK,
			       *value);
		break;

	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK,
			       *value);
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_rdsel_by_id(u32 id, u32 *value)
{
	u32 val1, val2;

	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK,
			       *value);
		break;
	case 1:
		/* value[11:0] set MSDC1_GPIO_RDSEL0_ADDR[27:16]
		 * value[17:12] set MSDC1_GPIO_RDSEL1_ADDR[5:0]
		 */
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK,
			       val1);
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK,
			       val2);
		*value = ((val2 & 0x3f) << 12) | (val1 & 0xfff);
		break;

	case 2:
		/* FIX ME */
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst, int ds)
{
	switch (id) {
	case 0:
		/* no SR to set */
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_CMD_MASK,
			       (cmd != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_CLK_MASK,
			       (clk != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_DAT_MASK,
			       (dat != 0));
		break;

	case 2:
		/* FIX ME */
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_sr_by_id(u32 id, int *clk, int *cmd, int *dat, int *rst, int *ds)
{
	if (clk)
		*clk = -1;

	if (cmd)
		*cmd = -1;

	if (dat)
		*dat = -1;

	if (rst)
		*rst = -1;

	if (ds)
		*ds = -1;

	switch (id) {
	case 0:
		/* no SR to get */
		break;
	case 1:
		if (cmd)
			MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_CMD_MASK,
				       *cmd);
		if (clk)
			MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_CLK_MASK,
				       *clk);
		if (dat)
			MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_SR_DAT_MASK,
				       *dat);
		break;

	case 2:
		/* FIX ME */
		break;

	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			       driving->ds_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			       driving->rst_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			       driving->ds_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			       driving->rst_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			       driving->cmd_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			       driving->clk_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			       driving->dat_drv);
		break;
	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

/* msdc pin config
 * MSDC0
 * PUPD/R1/R0
 * 0/0/0: High-Z
 * 0/1/0: Pull-up with 50Kohm
 * 0/0/1: Pull-up with 10Kohm
 * 0/1/1: Pull-up with 50Kohm//10Kohm
 * 1/0/0: High-Z
 * 1/1/0: Pull-down with 50Kohm
 * 1/0/1: Pull-down with 10Kohm
 * 1/1/1: Pull-down with 50Kohm//10Kohm
 */
void msdc_pin_config_by_id(u32 id, u32 mode)
{
	switch (id) {
	case 0:
		/* Attention: don't pull CLK high; Don't toggle RST to prevent
		 * from entering boot mode
		 */
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC0_* to no ohm PU,
			 * MSDC0_RSTB to 50K ohm PU
			 * 0x10002AC0[32:0] = 0x20000000
			 * 0x10002AD0[15:0] = 0
			 */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
				       0x20000000);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
				       0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC0_* to 50K ohm PD, MSDC0_RSTB to 50K ohm
			* PU
			* 0x10002AC0[32:0] = 0x26666666
			* 0x10002AD0[15:0] = 0x6666
			*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
				       0x26666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
				       0x6666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC0_CLK to 50K ohm PD,
			 * MSDC0_CMD/MSDC0_DAT* to 10K ohm PU,
			 * MSDC0_RSTB to 50K ohm PU, MSDC0_DSL to 50K ohm PD
			 * 0x10002AC0[31:0] = 0x21111161
			 * 0x10002AD0[14:0] = 0x6111
			 */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK,
				       0x21111161);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK,
				       0x6111);
		}
		break;
	case 1:
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC1_* to no ohm PU
			* 0x100020C0[22:0] = 0
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK,
				       0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC1_* to 50K ohm PD
			* 0x100020C0[22:0] = 0x666666
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK,
				       0x666666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC1_CLK to 50K ohm PD,
			* MSDC1_CMD/MSDC1_DAT* to 10K ohm PU
			* 0x100020C0[22:0] = 0x111161
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK,
				       0x111161);
		}
		break;
	case 2:
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC2_* to 0 ohm PU
			 * 0x100028C0[31:12] = 0, 0x100028D0[3:0] = 0
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK,
				       0);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR, MSDC2_PUPD1_MASK,
				       0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC2_* to 50K ohm PD
			 * 0x100028C0[31:12] = 0x66666, 0x100028D0[3:0] = 0x6
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK,
				       0x66666);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD1_MASK,
				       0x6);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC2_CLK to 50K ohm PD,
			 * MSDC2_CMD/MSDC2_DAT* to 10K ohm PU
			 * 0x100028C0[31:12] = 0x11611, 0x100028D0[3:0] = 0x1
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK,
				       0x11611);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR, MSDC2_PUPD1_MASK,
				       0x1);
		}
		break;
	default:
		pr_info("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 5: Device Tree Init function                       */
/*            This function is placed here so that all	      */
/*            functions and variables used by it has already  */
/*            been declared                                   */
/**************************************************************/
/*
 * parse pinctl settings
 * Driver strength
 */
#if !defined(FPGA_PLATFORM)
static int msdc_get_pinctl_settings(struct msdc_host *host,
				    struct device_node *np)
{
	struct device_node *pinctl_node, *pins_node;
	static char const *const pinctl_names[] = {
	    "pinctl", "pinctl_sdr104", "pinctl_sdr50", "pinctl_ddr50"};

	/* sequence shall be the same as sequence in msdc_hw_driving */
	static char const *const pins_names[] = {
	    "pins_cmd", "pins_dat", "pins_clk", "pins_rst", "pins_ds"};
	unsigned char *pin_drv;
	int i, j;

	host->hw->driving_applied = &host->hw->driving;
	for (i = 0; i < ARRAY_SIZE(pinctl_names); i++) {
		pinctl_node = of_parse_phandle(np, pinctl_names[i], 0);

		if (strcmp(pinctl_names[i], "pinctl") == 0)
			pin_drv = (unsigned char *)&host->hw->driving;
		else if (strcmp(pinctl_names[i], "pinctl_sdr104") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_sdr104;
		else if (strcmp(pinctl_names[i], "pinctl_sdr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_sdr50;
		else if (strcmp(pinctl_names[i], "pinctl_ddr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sd_ddr50;
		else
			continue;

		for (j = 0; j < ARRAY_SIZE(pins_names); j++) {
			pins_node =
			    of_get_child_by_name(pinctl_node, pins_names[j]);

			if (pins_node)
				of_property_read_u8(pins_node, "drive-strength",
						    pin_drv);
			pin_drv++;
		}
	}

	return 0;
}
#endif

/* Get msdc register settings
 * 1. internal data delay for tuning, FIXME: can be removed when use data tune?
 * 2. sample edge
 */
static int msdc_get_register_settings(struct msdc_host *host,
				      struct device_node *np)
{
	struct device_node *register_setting_node = NULL;

	/* parse hw property settings */
	register_setting_node = of_parse_phandle(np, "register_setting", 0);
	if (register_setting_node) {
		of_property_read_u8(register_setting_node, "cmd_edge",
				    &host->hw->cmd_edge);
		of_property_read_u8(register_setting_node, "rdata_edge",
				    &host->hw->rdata_edge);
		of_property_read_u8(register_setting_node, "wdata_edge",
				    &host->hw->wdata_edge);
	} else {
		pr_debug("[msdc%d] register_setting is not found in DT\n",
			 host->id);
	}

	return 0;
}

/*
 *	msdc_of_parse() - parse host's device-tree node
 *	@host: host whose node should be parsed.
 *
 */
int msdc_of_parse(struct mmc_host *mmc)
{
	struct device_node *np;
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;
	int len = 0;

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_debug("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	np = mmc->parent->of_node; /* mmcx node in project dts */

	host->mmc = mmc;
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_debug("[msdc%d] of_iomap failed\n", mmc->index);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_debug("[msdc%d] get irq # %d\n", mmc->index, host->irq);
	WARN_ON(host->irq < 0);

#if !defined(FPGA_PLATFORM)
	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_debug(
		    "[msdc%d] error: clk_src isn't found in device tree.\n",
		    host->id);
		WARN_ON(1);
	}
#endif

	/* get msdc flag(caps)*/
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	 * -ENODATA if property does not have a value, and -EOVERFLOW if the
	 * property data isn't large enough.
	 */
	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_debug("[msdc%d] host_function isn't found in device tree\n",
			 host->id);

	if (of_find_property(np, "bootable", &len))
		host->hw->boot = 1;

	/* get cd_gpio and cd_level */
	if (of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio) == 0) {
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_notice(
			    "[msdc%d] cd_level isn't found in device tree\n",
			    host->id);
		if (of_property_read_u32(np, "cd-debounce", &cd_debounce) ==
		    0) {
			pr_notice("[msdc%d] get cd_debounce %d ms\n", host->id,
				  cd_debounce);

			pr_notice("[msdc%d] set gpio %d debounce %d ms\n",
				  host->id, cd_gpio, cd_debounce);
			ret = gpio_set_debounce(cd_gpio, cd_debounce * 1000);
			if (ret < 0)
				pr_notice(
				    "[msdc%d] set cd_debounce %d ms error\n",
				    host->id, cd_debounce);
		}
	}

	msdc_get_register_settings(host, np);

#if !defined(FPGA_PLATFORM)
	msdc_get_pinctl_settings(host, np);

	mmc->supply.vmmc = regulator_get(mmc_dev(mmc), "vmmc");
	mmc->supply.vqmmc = regulator_get(mmc_dev(mmc), "vqmmc");
#else
	msdc_fpga_pwr_init();
#endif

#if defined(CFG_DEV_MSDC2)
	if (host->hw->host_function == MSDC_SDIO) {
		host->hw->flags |= MSDC_EXT_SDIO_IRQ;
		host->hw->request_sdio_eirq = mt_sdio_ops[2].sdio_request_eirq;
		host->hw->enable_sdio_eirq = mt_sdio_ops[2].sdio_enable_eirq;
		host->hw->disable_sdio_eirq = mt_sdio_ops[2].sdio_disable_eirq;
		host->hw->register_pm = mt_sdio_ops[2].sdio_register_pm;
	}
#endif
	pr_debug("%s: host_function = %d\n", __func__, host->hw->host_function);

	if (host->hw->host_function == MSDC_EMMC)
		device_rename(mmc->parent, "bootdevice");
	else if (host->hw->host_function == MSDC_SD)
		device_rename(mmc->parent, "externdevice");

	return 0;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	unsigned int id = 0;
	int ret;
	static char const *const msdc_names[] = {"msdc0", "msdc1", "msdc2"};
#ifndef FPGA_PLATFORM
	static char const *const ioconfig_names[] = {
	    MSDC0_IOCFG_NAME, MSDC1_IOCFG_NAME, MSDC2_IOCFG_NAME};
	struct device_node *np;
#endif

	pr_debug("DT probe %s!\n", pdev->dev.of_node->name);

	for (id = 0; id < HOST_MAX_NUM; id++) {
		if (strcmp(pdev->dev.of_node->name, msdc_names[id]) == 0) {
			pdev->id = id;
			break;
		}
	}

	if (id == HOST_MAX_NUM) {
		pr_debug("%s: Can not find msdc host\n", __func__);
		return 1;
	}

	ret = msdc_of_parse(mmc);
	if (ret) {
		pr_debug("%s: msdc%d of parse error!!: %d\n", __func__, id,
			 ret);
		return ret;
	}

	host = mmc_priv(mmc);
	host->id = id;

#ifndef FPGA_PLATFORM
	if (gpio_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,GPIO");
		gpio_base = of_iomap(np, 0);
		pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
	}

	if (msdc_io_cfg_bases[id] == NULL) {
		np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
		msdc_io_cfg_bases[id] = of_iomap(np, 0);
		pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n", id,
			 msdc_io_cfg_bases[id]);
	}

	if (infracfg_ao_reg_base == NULL) {
		np =
		    of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
		infracfg_ao_reg_base = of_iomap(np, 0);
		pr_debug("of_iomap for infracfg_ao base @ 0x%p\n",
			 infracfg_ao_reg_base);
	}

	if (apmixed_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		apmixed_reg_base = of_iomap(np, 0);
		pr_debug("of_iomap for apmixed base @ 0x%p\n",
			 apmixed_reg_base);
	}

	if (topckgen_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
		topckgen_reg_base = of_iomap(np, 0);
		pr_debug("of_iomap for topckgen base @ 0x%p\n",
			 topckgen_reg_base);
	}

#endif

	return 0;
}

/**************************************************************/
/* Section 7: For msdc register dump                          */
/**************************************************************/
u16 msdc_offsets[] = {
	OFFSET_MSDC_CFG,
	OFFSET_MSDC_IOCON,
	OFFSET_MSDC_PS,
	OFFSET_MSDC_INT,
	OFFSET_MSDC_INTEN,
	OFFSET_MSDC_FIFOCS,
	OFFSET_SDC_CFG,
	OFFSET_SDC_CMD,
	OFFSET_SDC_ARG,
	OFFSET_SDC_STS,
	OFFSET_SDC_RESP0,
	OFFSET_SDC_RESP1,
	OFFSET_SDC_RESP2,
	OFFSET_SDC_RESP3,
	OFFSET_SDC_BLK_NUM,
	OFFSET_SDC_VOL_CHG,
	OFFSET_SDC_CSTS,
	OFFSET_SDC_CSTS_EN,
	OFFSET_SDC_DCRC_STS,
	OFFSET_SDC_CMD_STS,
	OFFSET_EMMC_CFG0,
	OFFSET_EMMC_CFG1,
	OFFSET_EMMC_STS,
	OFFSET_EMMC_IOCON,
	OFFSET_SDC_ACMD_RESP,
	OFFSET_SDC_ACMD19_TRG,
	OFFSET_SDC_ACMD19_STS,
	OFFSET_MSDC_DMA_SA_HIGH,
	OFFSET_MSDC_DMA_SA,
	OFFSET_MSDC_DMA_CA,
	OFFSET_MSDC_DMA_CTRL,
	OFFSET_MSDC_DMA_CFG,
	OFFSET_MSDC_DMA_LEN,
	OFFSET_MSDC_DBG_SEL,
	OFFSET_MSDC_DBG_OUT,
	OFFSET_MSDC_PATCH_BIT0,
	OFFSET_MSDC_PATCH_BIT1,
	OFFSET_MSDC_PATCH_BIT2,

	OFFSET_DAT0_TUNE_CRC,
	OFFSET_DAT1_TUNE_CRC,
	OFFSET_DAT2_TUNE_CRC,
	OFFSET_DAT3_TUNE_CRC,
	OFFSET_CMD_TUNE_CRC,
	OFFSET_SDIO_TUNE_WIND,

	OFFSET_MSDC_PAD_TUNE0,
	OFFSET_MSDC_PAD_TUNE1,
	OFFSET_MSDC_DAT_RDDLY0,
	OFFSET_MSDC_DAT_RDDLY1,
	OFFSET_MSDC_DAT_RDDLY2,
	OFFSET_MSDC_DAT_RDDLY3,
	OFFSET_MSDC_HW_DBG,
	OFFSET_MSDC_VERSION,

	OFFSET_EMMC50_PAD_DS_TUNE,
	OFFSET_EMMC50_PAD_CMD_TUNE,
	OFFSET_EMMC50_PAD_DAT01_TUNE,
	OFFSET_EMMC50_PAD_DAT23_TUNE,
	OFFSET_EMMC50_PAD_DAT45_TUNE,
	OFFSET_EMMC50_PAD_DAT67_TUNE,
	OFFSET_EMMC51_CFG0,
	OFFSET_EMMC50_CFG0,
	OFFSET_EMMC50_CFG1,
	OFFSET_EMMC50_CFG2,
	OFFSET_EMMC50_CFG3,
	OFFSET_EMMC50_CFG4,
	OFFSET_SDC_FIFO_CFG,

	OFFSET_MSDC_IOCON_1,
	OFFSET_MSDC_PATCH_BIT0_1,
	OFFSET_MSDC_PATCH_BIT1_1,
	OFFSET_MSDC_PATCH_BIT2_1,
	OFFSET_MSDC_PAD_TUNE0_1,
	OFFSET_MSDC_PAD_TUNE1_1,
	OFFSET_MSDC_DAT_RDDLY0_1,
	OFFSET_MSDC_DAT_RDDLY1_1,
	OFFSET_MSDC_DAT_RDDLY2_1,
	OFFSET_MSDC_DAT_RDDLY3_1,
	OFFSET_MSDC_IOCON_2,
	OFFSET_MSDC_PATCH_BIT0_2,
	OFFSET_MSDC_PATCH_BIT1_2,
	OFFSET_MSDC_PATCH_BIT2_2,
	OFFSET_MSDC_PAD_TUNE0_2,
	OFFSET_MSDC_PAD_TUNE1_2,
	OFFSET_MSDC_DAT_RDDLY0_2,
	OFFSET_MSDC_DAT_RDDLY1_2,
	OFFSET_MSDC_DAT_RDDLY2_2,
	OFFSET_MSDC_DAT_RDDLY3_2,

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	OFFSET_EMMC51_CFG0,
	OFFSET_EMMC50_CFG0,
	OFFSET_EMMC50_CFG1,
	OFFSET_EMMC50_CFG2,
	OFFSET_EMMC50_CFG3,
	OFFSET_EMMC50_CFG4,

	OFFSET_MTKCQ_ERR_ST,
	OFFSET_MTKCQ_CMD45_READY,
	OFFSET_MTKCQ_TASK_READY_ST,
	OFFSET_MTKCQ_TASK_DONE_ST,
	OFFSET_MTKCQ_ERR_ST_CLR,
	OFFSET_MTKCQ_CMD_DONE_CLR,
	OFFSET_MTKCQ_SW_CTL_CQ,
	OFFSET_MTKCQ_CMD44_RESP,
	OFFSET_MTKCQ_CMD45_RESP,
	OFFSET_MTKCQ_CMD13_RCA,
	OFFSET_EMMC51_CQCB_CFG3,
	OFFSET_EMMC51_CQCB_CMD44,
	OFFSET_EMMC51_CQCB_CMD45,
	OFFSET_EMMC51_CQCB_TIDMAP,
	OFFSET_EMMC51_CQCB_TIDMAPCLR,
	OFFSET_EMMC51_CQCB_CURCMD,
#endif

	0xFFFF /*as mark of end */
};
