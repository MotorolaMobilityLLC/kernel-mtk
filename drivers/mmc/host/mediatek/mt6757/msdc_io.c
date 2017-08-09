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

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>

#include "mt_sd.h"
#include "msdc_tune.h"
#include "dbg.h"

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = DT_COMPATIBLE_NAME, },
	{ },
};

void __iomem *gpio_base;

void __iomem *infracfg_ao_reg_base;
void __iomem *infracfg_reg_base;
void __iomem *pericfg_reg_base;
void __iomem *emi_reg_base;
/*void __iomem *toprgu_reg_base;*/
void __iomem *apmixed_reg_base;
void __iomem *topckgen_reg_base;

void __iomem *msdc_io_cfg_bases[HOST_MAX_NUM];

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
u32 g_msdc0_io;
u32 g_msdc0_flash;
u32 g_msdc1_io;
u32 g_msdc1_flash;
u32 g_msdc2_io;
u32 g_msdc2_flash;
u32 g_msdc3_io;
u32 g_msdc3_flash;

#if !defined(FPGA_PLATFORM)
#include <mt-plat/upmu_common.h>

struct regulator *reg_vemc;
struct regulator *reg_vmc;
struct regulator *reg_vmch;

enum MSDC_LDO_POWER {
	POWER_LDO_VMCH,
	POWER_LDO_VMC,
	POWER_LDO_VEMC,
};

void msdc_get_regulators(struct device *dev)
{
	if (reg_vemc == NULL)
		reg_vemc = regulator_get(dev, "vemc");
	if (reg_vmc == NULL)
		reg_vmc = regulator_get(dev, "vmc");
	if (reg_vmch == NULL)
		reg_vmch = regulator_get(dev, "vmch");
}

bool msdc_hwPowerOn(unsigned int powerId, int powerVolt, char *mode_name)
{
	struct regulator *reg = NULL;
	int ret = 0;

	switch (powerId) {
	case POWER_LDO_VMCH:
		reg = reg_vmch;
		break;
	case POWER_LDO_VMC:
		reg = reg_vmc;
		break;
	case POWER_LDO_VEMC:
		reg = reg_vemc;
		break;
	default:
		pr_err("[msdc] Invalid power id\n");
		return false;
	}
	if (reg == NULL) {
		pr_err("[msdc] regulator pointer is NULL\n");
		return false;
	}
	/* New API voltage use micro V */
	regulator_set_voltage(reg, powerVolt, powerVolt);
	ret = regulator_enable(reg);
	if (ret) {
		pr_err("msdc regulator_enable failed: %d\n", ret);
		return false;
	}
	pr_err("msdc_hwPoweron:%d: name:%s", powerId, mode_name);
	return true;
}

bool msdc_hwPowerDown(unsigned int powerId, char *mode_name)
{
	struct regulator *reg = NULL;

	switch (powerId) {
	case POWER_LDO_VMCH:
		reg = reg_vmch;
		break;
	case POWER_LDO_VMC:
		reg = reg_vmc;
		break;
	case POWER_LDO_VEMC:
		reg = reg_vemc;
		break;
	default:
		pr_err("[msdc] Invalid power id\n");
		return false;
	}
	if (reg == NULL) {
		pr_err("[msdc] regulator pointer is NULL\n");
		return false;
	}
	/* New API voltage use micro V */
	regulator_disable(reg);
	pr_err("msdc_hwPowerOff:%d: name:%s", powerId, mode_name);

	return true;
}

u32 msdc_ldo_power(u32 on, unsigned int powerId, int voltage_mv,
	u32 *status)
{
	int voltage_uv = voltage_mv * 1000;

	if (on) { /* want to power on */
		if (*status == 0) {  /* can power on */
			/* pr_err("msdc LDO<%d> power on<%d>\n", powerId,
				voltage_uv); */
			msdc_hwPowerOn(powerId, voltage_uv, "msdc");
			*status = voltage_uv;
		} else if (*status == voltage_uv) {
			pr_err("msdc LDO<%d><%d> power on again!\n", powerId,
				voltage_uv);
		} else { /* for sd3.0 later */
			pr_warn("msdc LDO<%d> change<%d> to <%d>\n", powerId,
				*status, voltage_uv);
			msdc_hwPowerDown(powerId, "msdc");
			msdc_hwPowerOn(powerId, voltage_uv, "msdc");
			/*hwPowerSetVoltage(powerId, voltage_uv, "msdc");*/
			*status = voltage_uv;
		}
	} else {  /* want to power off */
		if (*status != 0) {  /* has been powerred on */
			pr_warn("msdc LDO<%d> power off\n", powerId);
			msdc_hwPowerDown(powerId, "msdc");
			*status = 0;
		} else {
			pr_err("LDO<%d> not power on\n", powerId);
		}
	}

	return 0;
}

void msdc_dump_ldo_sts(struct msdc_host *host)
{
	u32 ldo_en = 0, ldo_vol = 0;
	u32 id = host->id;

	switch (id) {
	case 0:
		pmic_read_interface_nolock(REG_VEMC_EN, &ldo_en, MASK_VEMC_EN,
			SHIFT_VEMC_EN);
		pmic_read_interface_nolock(REG_VEMC_VOSEL, &ldo_vol,
			MASK_VEMC_VOSEL, SHIFT_VEMC_VOSEL);
		pr_err(" VEMC_EN=0x%x, be:1b'1,VEMC_VOL=0x%x, be:1b'0(3.0V),1b'1(3.3V)\n",
			ldo_en, ldo_vol);
		break;
	case 1:
		pmic_read_interface_nolock(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
			SHIFT_VMC_EN);
		pmic_read_interface_nolock(REG_VMC_VOSEL, &ldo_vol,
			MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
		pr_err(" VMC_EN=0x%x, be:bit1=1,VMC_VOL=0x%x, be:3b'101(2.8V),3b'011(1.8V)\n",
			ldo_en, ldo_vol);

		pmic_read_interface_nolock(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
			SHIFT_VMCH_EN);
		pmic_read_interface_nolock(REG_VMCH_VOSEL, &ldo_vol,
			MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
		pr_err(" VMCH_EN=0x%x, be:bit1=1, VMCH_VOL=0x%x, be:1b'0(3V),1b'1(3.3V)\n",
			ldo_en, ldo_vol);
		break;
	default:
		break;
	}
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	unsigned int reg_val;

	switch (host->id) {
	case 1:
		if (on) {
			/* VMC calibration +40mV. According to SA's request. */
			reg_val = host->vmc_cal_default - 2;

			pmic_config_interface(REG_VMC_VOSEL_CAL,
				reg_val,
				MASK_VMC_VOSEL_CAL,
				SHIFT_VMC_VOSEL_CAL);
		}

		msdc_ldo_power(on, POWER_LDO_VMC, VOL_1800, &g_msdc1_io);

		msdc_set_tdsel(host, 0, 1);
		msdc_set_rdsel(host, 0, 1);
		msdc_set_driving(host, host->hw, 1);
		break;
	case 2:
		msdc_ldo_power(on, POWER_LDO_VMC, VOL_1800, &g_msdc2_io);
		msdc_set_tdsel(host, 0, 1);
		msdc_set_rdsel(host, 0, 1);
		msdc_set_driving(host, host->hw, 1);
		break;
	default:
		break;
	}
}

void msdc_sdio_power(struct msdc_host *host, u32 on)
{
	switch (host->id) {
#if defined(CFG_DEV_MSDC2)
	case 2:
		g_msdc2_flash = g_msdc2_io;
		break;
#endif
	default:
		/* if host_id is 3, it uses default 1.8v setting,
		which always turns on */
		break;
	}

}

void msdc_emmc_power(struct msdc_host *host, u32 on)
{
	unsigned long tmo = 0;
	void __iomem *base = host->base;
	unsigned int sa_timeout;

	/* if MMC_CAP_WAIT_WHILE_BUSY not set, mmc core layer will wait for
	 sa_timeout */
	if (host->mmc && host->mmc->card && (on == 0) &&
	    host->mmc->caps & MMC_CAP_WAIT_WHILE_BUSY) {
		/* max timeout: 1000ms */
		sa_timeout = host->mmc->card->ext_csd.sa_timeout;
		if ((DIV_ROUND_UP(sa_timeout, 10000)) < 1000)
			tmo = DIV_ROUND_UP(sa_timeout, 10000000/HZ) + HZ/100;
		else
			tmo = HZ;
		tmo += jiffies;

		while ((MSDC_READ32(MSDC_PS) & 0x10000) != 0x10000) {
			if (time_after(jiffies, tmo)) {
				pr_err("Dat0 keep low before power off,sa_timeout = 0x%x",
					sa_timeout);
				emmc_sleep_failed = 1;
				break;
			}
		}
	}

	/* Set to 3.0V - 100mV
	 * 4'b0000: 0 mV
	 * 4'b0001: -20 mV
	 * 4'b0010: -40 mV
	 * 4'b0011: -60 mV
	 * 4'b0100: -80 mV
	 * 4'b0101: -100 mV
	*/
	msdc_ldo_power(on, POWER_LDO_VEMC, VOL_3000, &g_msdc0_flash);
	/* cal 3.0V - 100mv */
	if (on && CARD_VOL_ACTUAL != VOL_3000) {
		pmic_config_interface(REG_VEMC_VOSEL_CAL,
			VEMC_VOSEL_CAL_mV(CARD_VOL_ACTUAL - VOL_3000),
			MASK_VEMC_VOSEL_CAL,
			SHIFT_VEMC_VOSEL_CAL);
	}

#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_ldo_sts(host);
#endif
}

void msdc_sd_power(struct msdc_host *host, u32 on)
{
	u32 card_on = on;

	switch (host->id) {
	case 1:
		msdc_set_driving(host, host->hw, 0);
		msdc_set_tdsel(host, 0, 0);
		msdc_set_rdsel(host, 0, 0);
		if (host->hw->flags & MSDC_SD_NEED_POWER)
			card_on = 1;
		/* VMCH VOLSEL */
		msdc_ldo_power(card_on, POWER_LDO_VMCH, VOL_3000,
			&g_msdc1_flash);
		/* VMC VOLSEL */
		msdc_ldo_power(on, POWER_LDO_VMC, VOL_3000, &g_msdc1_io);
		break;
	case 2:
		msdc_set_driving(host, host->hw, 0);
		msdc_set_tdsel(host, 0, 0);
		msdc_set_rdsel(host, 0, 0);
		msdc_ldo_power(on, POWER_LDO_VMCH, VOL_3000, &g_msdc2_flash);
		msdc_ldo_power(on, POWER_LDO_VMC, VOL_3000, &g_msdc2_io);
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

	pr_err("Power Off, SD card\n");

	/* power must be on */
	g_msdc1_io = VOL_3000 * 1000;
	g_msdc1_flash = VOL_3000 * 1000;

	/* power off */
	msdc_ldo_power(0, POWER_LDO_VMC, VOL_3000, &g_msdc1_io);
	msdc_ldo_power(0, POWER_LDO_VMCH, VOL_3000, &g_msdc1_flash);

	if (host != NULL)
		msdc_set_bad_card_and_remove(host);
}
EXPORT_SYMBOL(msdc_sd_power_off);
#endif /*if !defined(FPGA_PLATFORM)*/

#if !defined(FPGA_PLATFORM)
/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
/*  msdc clock source
 *  Clock manager will set a reference value in CCF
 *  Normal code doesn't change clock source
 *  Lower frequence will change clock source
 */
u32 hclks_msdc50[] = { MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2, MSDC0_SRC_3,
			   MSDC0_SRC_4, MSDC0_SRC_5, MSDC0_SRC_6, MSDC0_SRC_7,
			   MSDC0_SRC_8};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc30[] = { MSDC1_SRC_0, MSDC1_SRC_1, MSDC1_SRC_2, MSDC1_SRC_3,
			   MSDC1_SRC_4, MSDC1_SRC_5, MSDC1_SRC_6, MSDC1_SRC_7};

u32 *hclks_msdc_all[] = {
	hclks_msdc50,
	hclks_msdc30,
	hclks_msdc30,
};
u32 *hclks_msdc = NULL;

#include <dt-bindings/clock/mt6757-clk.h>
#include <mt_clk_id.h>

int msdc_cg_clk_id[HOST_MAX_NUM] = {
	MSDC0_CG_NAME,
	MSDC1_CG_NAME,
	MSDC2_CG_NAME
};

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	static char const * const clk_names[] = {
		MSDC0_CLK_NAME, MSDC1_CLK_NAME, MSDC2_CLK_NAME
	};

	host->clock_control = devm_clk_get(&pdev->dev, clk_names[pdev->id]);

	if (IS_ERR(host->clock_control)) {
		pr_err("[msdc%d] can not get clock control\n", pdev->id);
		return 1;
	}
	if (clk_prepare(host->clock_control)) {
		pr_err("[msdc%d] can not prepare clock control\n", pdev->id);
		return 1;
	}
	return 0;
}

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	u32 *hclks;

	hclks = msdc_get_hclks(host->id);

	pr_err("[%s]: msdc%d change clk_src from %dKHz to %d:%dKHz\n", __func__,
		host->id, (host->hclk/1000), clksrc, (hclks[clksrc]/1000));

	if (host->id != 0) {
		pr_err("[msdc%d] NOT Support switch pll souce[%s]%d\n",
			host->id, __func__, __LINE__);
		return;
	}
	host->hclk = hclks[clksrc];
	host->hw->clk_src = clksrc;
}

void msdc_dump_clock_sts(struct msdc_host *host)
{
	if (topckgen_reg_base) {
		/* CLK_CFG_3 control msdc clock source PLL */
		pr_err(" CLK_CFG_3 register address is 0x%p\n",
			topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET);
		pr_err(" bit[9~8]=01b,     bit[15]=0b\n");
		pr_err(" bit[19~16]=0001b, bit[23]=0b\n");
		pr_err(" bit[26~24]=0010b, bit[31]=0b\n");
		pr_err(" Read value is       0x%x\n",
			MSDC_READ32(topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET));
	}
	if (apmixed_reg_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		pr_err(" MSDCPLL_CON0_OFFSET register address is 0x%p\n",
			apmixed_reg_base + MSDCPLL_CON0_OFFSET);
		pr_err(" bit[0]=1b\n");
		pr_err(" Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON0_OFFSET));

		pr_err(" MSDCPLL_CON1_OFFSET register address is 0x%p\n",
			apmixed_reg_base + MSDCPLL_CON1_OFFSET);
		pr_err(" Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON1_OFFSET));

		pr_err(" MSDCPLL_CON2_OFFSET register address is 0x%p\n",
			apmixed_reg_base + MSDCPLL_CON2_OFFSET);
		pr_err(" Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON2_OFFSET));

		pr_err(" MSDCPLL_PWR_CON0 register address is 0x%p\n",
			apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET);
		pr_err(" bit[0]=1b\n");
		pr_err(" Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET));
	}
}

void msdc_clk_status(int *status)
{
	int g_clk_gate = 0;
	int i = 0;
	unsigned long flags;

	for (i = 0; i < HOST_MAX_NUM; i++) {
		if (!mtk_msdc_host[i])
			continue;

		spin_lock_irqsave(&mtk_msdc_host[i]->clk_gate_lock, flags);
		if (mtk_msdc_host[i]->clk_gate_count > 0)
			g_clk_gate |= 1 << msdc_cg_clk_id[i];
		spin_unlock_irqrestore(&mtk_msdc_host[i]->clk_gate_lock, flags);
	}
	*status = g_clk_gate;
}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
void msdc_dump_padctl_by_id(u32 id)
{
	if (!gpio_base || !msdc_io_cfg_bases[id]) {
		pr_err("err: gpio_base=%p, msdc_io_cfg_bases[%d]=%p\n",
			gpio_base, id, msdc_io_cfg_bases[id]);
		return;
	}
	switch (id) {
	case 0:
		pr_err("MSDC0 MODE23[0x%p] =0x%8x\tshould:0x111111??\n",
			MSDC0_GPIO_MODE23, MSDC_READ32(MSDC0_GPIO_MODE23));
		pr_err("MSDC0 MODE24[0x%p] =0x%8x\tshould:0x??111111\n",
			MSDC0_GPIO_MODE24, MSDC_READ32(MSDC0_GPIO_MODE24));
		pr_err("MSDC0 IES   [0x%p] =0x%8x\tshould:0x??????1f\n",
			MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
		pr_err("MSDC0 SMT   [0x%p] =0x%8x\tshould:0x???????f\n",
			MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
		pr_err("MSDC0 TDSEL [0x%p] =0x%8x\tshould:0x????0000\n",
			MSDC0_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_TDSEL_ADDR));
		pr_err("MSDC0 RDSEL [0x%p] =0x%8x\tshould:0x???00000\n",
			MSDC0_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_RDSEL_ADDR));
		pr_err("MSDC0 DRV   [0x%p] =0x%8x\tshould: 1001001001b\n",
			MSDC0_GPIO_DRV_ADDR,
			MSDC_READ32(MSDC0_GPIO_DRV_ADDR));
		pr_err("PUPD/R1/R0: dat/cmd:0/0/1, clk/dst: 1/1/0\n");
		pr_err("PUPD/R1/R0: rstb: 0/1/0\n");
		pr_err("MSDC0 PUPD0 [0x%p] =0x%8x\tshould: 0x21111161 ??????b\n",
			MSDC0_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD0_ADDR));
		pr_err("MSDC0 PUPD1 [0x%p] =0x%8x\tshould: 0x6111 ??????b\n",
			MSDC0_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD1_ADDR));
		break;
	case 1:
		pr_err("MSDC1 MODE4 [0x%p] =0x%8x\n",
			MSDC1_GPIO_MODE4, MSDC_READ32(MSDC1_GPIO_MODE4));
		pr_err("MSDC1 MODE5 [0x%p] =0x%8x\n",
			MSDC1_GPIO_MODE5, MSDC_READ32(MSDC1_GPIO_MODE5));
		pr_err("MSDC1 IES    [0x%p] =0x%8x\n",
			MSDC1_GPIO_IES_ADDR, MSDC_READ32(MSDC1_GPIO_IES_ADDR));
		pr_err("MSDC1 SMT    [0x%p] =0x%8x\n",
			MSDC1_GPIO_SMT_ADDR, MSDC_READ32(MSDC1_GPIO_SMT_ADDR));

		pr_err("MSDC1 TDSEL  [0x%p] =0x%8x\n",
			MSDC1_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC1_GPIO_TDSEL_ADDR));
		/* Note1=> For Vcore=0.7V sleep mode
		 * if TDSEL0~3 don't set to [1111]
		 * the digital output function will fail
		 */
		pr_err("should 1.8v: sleep:0x?fff????, awake:0x?aaa????\n");
		pr_err("MSDC1 RDSEL0  [0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL0_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL0_ADDR));
		pr_err("MSDC1 RDSEL1  [0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL1_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL1_ADDR));
		pr_err("1.8V: ??0000??, 2.9v: 0x??c30c??\n");

		pr_err("MSDC1 DRV1    [0x%p] =0x%8x\n",
			MSDC1_GPIO_DRV1_ADDR, MSDC_READ32(MSDC1_GPIO_DRV1_ADDR));

		pr_err("MSDC1 PUPD   [0x%p] =0x%8x\n",
			MSDC1_GPIO_PUPD_ADDR,
			MSDC_READ32(MSDC1_GPIO_PUPD_ADDR));
		break;
	case 2:
		pr_err("MSDC2 MODE17 [0x%p] =0x%8x\n",
			MSDC2_GPIO_MODE17, MSDC_READ32(MSDC2_GPIO_MODE17));
		pr_err("MSDC2 MODE18 [0x%p] =0x%8x\n",
			MSDC2_GPIO_MODE18, MSDC_READ32(MSDC2_GPIO_MODE18));
		pr_err("MSDC2 IES   [0x%p] =0x%8x\tshould:0x??????1f\n",
			MSDC2_GPIO_IES_ADDR, MSDC_READ32(MSDC2_GPIO_IES_ADDR));
		pr_err("MSDC2 SMT    [0x%p] =0x%8x\n",
			MSDC2_GPIO_SMT_ADDR, MSDC_READ32(MSDC2_GPIO_SMT_ADDR));

		pr_err("MSDC2 TDSEL  [0x%p] =0x%8x\n",
			MSDC2_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC2_GPIO_TDSEL_ADDR));
		/* Note1=> For Vcore=0.7V sleep mode
		 * if TDSEL0~3 don't set to [1111]
		 * the digital output function will fail
		 */
		pr_err("MSDC2 RDSEL  [0x%p] =0x%8x\n",
			MSDC2_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC2_GPIO_RDSEL_ADDR));

		pr_err("MSDC2 DRV    [0x%p] =0x%8x\n",
			MSDC2_GPIO_DRV_ADDR, MSDC_READ32(MSDC2_GPIO_DRV_ADDR));

		pr_err("MSDC2 PUPD0   [0x%p] =0x%8x\n",
			MSDC2_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC2_GPIO_PUPD0_ADDR));
		pr_err("MSDC2 PUPD1   [0x%p] =0x%8x\n",
			MSDC2_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC2_GPIO_PUPD1_ADDR));
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
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
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_CMD_MASK,  0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_CLK_MASK,  0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE23, MSDC0_MODE_DAT0_MASK, 0x1);

		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DSL_MASK,  0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT7_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT3_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT2_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_RSTB_MASK, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE24, MSDC0_MODE_DAT4_MASK, 0x1);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, MSDC1_MODE_DAT3_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, MSDC1_MODE_CLK_MASK,  0x1);

		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT1_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT2_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_DAT0_MASK, 0x1);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, MSDC1_MODE_CMD_MASK,  0x1);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT0_MASK, 0x1);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT3_MASK, 0x1);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_CLK_MASK,  0x1);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_CMD_MASK,  0x1);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE17, MSDC2_MODE_DAT1_MASK, 0x1);

		MSDC_SET_FIELD(MSDC2_GPIO_MODE18, MSDC2_MODE_DAT2_MASK, 0x1);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, host->id);
		break;
	}
}

/* set pin ies:
 * 0: Disable
 * 1: Enable
 */
void msdc_set_ies_by_id(u32 id, int set_ies)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_IES_ADDR, MSDC0_IES_ALL_MASK,
			(set_ies ? 0x1f : 0));
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_IES_ADDR, MSDC1_IES_ALL_MASK,
			(set_ies ? 0x7 : 0));
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_IES_ADDR, MSDC2_IES_ALL_MASK,
			(set_ies ? 0x1f : 0));
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_smt_by_id(u32 id, int set_smt)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
			(set_smt ? 0x1f : 0));
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
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_tdsel_by_id(u32 id, bool sleep, bool sd_18)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK, 0);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK, 0);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK, 0);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_rdsel_by_id(u32 id, bool sleep, bool sd_18)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK, 0);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK, 0);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK, 0);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_ALL_MASK, 0);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_tdsel_dbg_by_id(u32 id, u32 value)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK,
				value);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK,
				value);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK,
				value);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_rdsel_dbg_by_id(u32 id, u32 value)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK,
				value);
		break;
	case 1:
		/* value[11:0] set MSDC1_GPIO_RDSEL0_ADDR[27:16]
		 * value[17:12] set MSDC1_GPIO_RDSEL1_ADDR[5:0] */
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK,
			value & 0xfff);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK,
			(value >> 12) & 0x3f);
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_ALL_MASK,
				value);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_tdsel_dbg_by_id(u32 id, u32 *value)
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
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_get_rdsel_dbg_by_id(u32 id, u32 *value)
{
	u32 val1, val2;

	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_ALL_MASK,
				*value);
		break;
	case 1:
		/* value[11:0] set MSDC1_GPIO_RDSEL0_ADDR[27:16]
		 * value[17:12] set MSDC1_GPIO_RDSEL1_ADDR[5:0] */
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK,
			val1);
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK,
			val2);
		*value = ((val2 << 12) & 0x3f) | (val1 & 0xfff);
		break;
	case 2:
		/* FIX ME */
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat)
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
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

/*
 * Set pin driving:
 * MSDC0,MSDC1
 * 000: 2mA
 * 001: 4mA
 * 010: 6mA
 * 011: 8mA
 * 100: 10mA
 * 101: 12mA
 * 110: 14mA
 * 111: 16mA
 */
void msdc_set_driving_by_id(u32 id, struct msdc_hw *hw, bool sd_18)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			hw->ds_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			hw->rst_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			hw->dat_drv);
		break;
	case 1:
		if (sd_18) {
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv_sd_18);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv_sd_18);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv_sd_18);
		} else {
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv);
		}
		break;
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			hw->dat_drv);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
		break;
	}
}

void msdc_set_driving(struct msdc_host *host, struct msdc_hw *hw, bool sd_18)
{
	msdc_set_driving_by_id(host->id, hw, sd_18);
}

void msdc_get_driving_by_id(u32 id, struct msdc_hw *hw)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			hw->ds_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			hw->rst_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			hw->dat_drv);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV1_ADDR, MSDC1_DRV_DAT_MASK,
			hw->dat_drv);
		break;
	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			hw->dat_drv);
		break;
	default:
		pr_err("[%s] invalid host->id = %d\n", __func__, id);
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
		/*Attention: don't pull CLK high; Don't toggle RST to prevent
		  from entering boot mode */
		if (MSDC_PIN_PULL_NONE == mode) {
			/* Switch MSDC0_* to no ohm PU, MSDC0_RSTB to 50K ohm PU
			* 0x10002AC0[32:0] = 0x20000000
			* 0x10002AD0[15:0] = 0
			*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x20000000);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* Switch MSDC0_* to 50K ohm PD, MSDC0_RSTB to 50K ohm PU
			* 0x10002AC0[32:0] = 0x26666666
			* 0x10002AD0[15:0] = 0x6666
			*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x26666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0x6666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* Switch MSDC0_CLK to 50K ohm PD, MSDC0_CMD/MSDC0_DAT*
			* to 10K ohm PU, MSDC0_RSTB to 50K ohm PU, MSDC0_DSL to 50K ohm PD
			* 0x10002AC0[31:0] = 0x21111161
			* 0x10002AD0[14:0] = 0x6111
			*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x21111161);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0x6111);
		}
		break;
	case 1:
		if (MSDC_PIN_PULL_NONE == mode) {
			/* Switch MSDC1_* to no ohm PU
			* 0x100020C0[22:0] = 0
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK, 0);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* Switch MSDC1_* to 50K ohm PD
			* 0x100020C0[22:0] = 0x666666
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK, 0x666666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* Switch MSDC1_CLK to 50K ohm PD,
			* MSDC1_CMD/MSDC1_DAT* to 10K ohm PU
			* 0x100020C0[22:0] = 0x111161
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR, MSDC1_PUPD_MASK, 0x111161);
		}
		break;
	case 2:
		if (MSDC_PIN_PULL_NONE == mode) {
			/* Switch MSDC2_* to 0 ohm PU
			 * 0x100028C0[31:12] = 0, 0x100028D0[3:0] = 0
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK, 0);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR, MSDC2_PUPD1_MASK, 0);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* Switch MSDC2_* to 50K ohm PD
			 * 0x100028C0[31:12] = 0x66666, 0x100028D0[3:0] = 0x6
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK, 0x66666);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD1_MASK, 0x6);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* Switch MSDC2_CLK to 50K ohm PD,
			 * MSDC2_CMD/MSDC2_DAT* to 10K ohm PU
			 * 0x100028C0[31:12] = 0x11611, 0x100028D0[3:0] = 0x1
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD0_MASK, 0x11611);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR, MSDC2_PUPD1_MASK, 0x1);
		}
		break;
	default:
		pr_err("[%s] invalid host->id!\n", __func__);
		break;
	}

}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 5: MISC                                            */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
void dump_axi_bus_info(void)
{
	pr_err("=============== AXI BUS INFO =============");
	return; /*weiping check*/

	if (infracfg_ao_reg_base && infracfg_reg_base && pericfg_reg_base) {
		pr_err("reg[0x10001224]=0x%x",
			MSDC_READ32(infracfg_ao_reg_base + 0x224));
		pr_err("reg[0x10201000]=0x%x",
			MSDC_READ32(infracfg_reg_base + 0x000));
		pr_err("reg[0x10201018]=0x%x",
			MSDC_READ32(infracfg_reg_base + 0x018));
		pr_err("reg[0x1000320c]=0x%x",
			MSDC_READ32(pericfg_reg_base + 0x20c));
		pr_err("reg[0x10003210]=0x%x",
			MSDC_READ32(pericfg_reg_base + 0x210));
		pr_err("reg[0x10003214]=0x%x",
			MSDC_READ32(pericfg_reg_base + 0x214));
	} else {
		pr_err("infracfg_ao_reg_base = %p, infracfg_reg_base = %p, pericfg_reg_base = %p\n",
			infracfg_ao_reg_base,
			infracfg_reg_base,
			pericfg_reg_base);
	}
}

void dump_emi_info(void)
{
	return; /*weiping check */

#if 0
	unsigned int i = 0;
	unsigned int addr = 0;

	pr_err("=============== EMI INFO =============");
	if (!emi_reg_base) {
		pr_err("emi_reg_base = %p\n", emi_reg_base);
		return;
	}

	pr_err("before, reg[0x102034e8]=0x%x",
		MSDC_READ32(emi_reg_base + 0x4e8));
	pr_err("before, reg[0x10203400]=0x%x",
		MSDC_READ32(emi_reg_base + 0x400));
	MSDC_WRITE32(emi_reg_base + 0x4e8, 0x2000000);
	MSDC_WRITE32(emi_reg_base + 0x400, 0xff0001);
	pr_err("after, reg[0x102034e8]=0x%x",
		MSDC_READ32(emi_reg_base + 0x4e8));
	pr_err("after, reg[0x10203400]=0x%x",
		MSDC_READ32(emi_reg_base + 0x400));

	for (i = 0; i < 5; i++) {
		for (addr = 0; addr < 0x78; addr += 4) {
			pr_err("reg[0x%x]=0x%x", (0x10203500 + addr),
				MSDC_READ32((emi_reg_base + 0x500 + addr)));
			if (addr % 0x10 == 0)
				mdelay(1);
		}
	}
#endif
}

void msdc_polling_axi_status(int line, int dead)
{
	int i = 0;

	if (!pericfg_reg_base) {
		pr_err("pericfg_reg_base = %p\n", pericfg_reg_base);
		return;
	}

	while (MSDC_READ32(pericfg_reg_base + 0x214) & 0xc) {
		if (++i < 300) {
			mdelay(10);
			continue;
		}
		pr_err("[%s]: check peri-bus: 0x%x at %d\n", __func__,
			MSDC_READ32(pericfg_reg_base + 0x214), line);

		pr_err("############## AXI bus hang! start ##################");
		dump_emi_info();
		mdelay(10);

		dump_axi_bus_info();
		mdelay(10);

		/*dump_audio_info();
		mdelay(10);*/

		msdc_dump_info(0);
		mdelay(10);

		msdc_dump_gpd_bd(0);
		pr_err("############## AXI bus hang! end ####################");
		if (dead == 0)
			break;
		i = 0;
	}
}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 6: Device Tree Init function                       */
/*            This function is placed here so that all		  */
/*            functions and variables used by it has already  */
/*            been declared                                   */
/**************************************************************/
/*
 * parse pinctl settings
 * Driver strength
 */
static int msdc_get_pinctl_settings(struct msdc_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct device_node *np = mmc->parent->of_node;
	struct device_node *pinctl_node;
	struct device_node *pins_cmd_node;
	struct device_node *pins_dat_node;
	struct device_node *pins_clk_node;
	struct device_node *pins_rst_node;
	struct device_node *pins_ds_node;
	struct device_node *pinctl_sdr104_node;
	struct device_node *pinctl_sdr50_node;
	struct device_node *pinctl_ddr50_node;

	pinctl_node = of_parse_phandle(np, "pinctl", 0);
	pins_cmd_node = of_get_child_by_name(pinctl_node, "pins_cmd");
	of_property_read_u8(pins_cmd_node, "drive-strength",
			&host->hw->cmd_drv);

	pins_dat_node = of_get_child_by_name(pinctl_node, "pins_dat");
	of_property_read_u8(pins_dat_node, "drive-strength",
			&host->hw->dat_drv);

	pins_clk_node = of_get_child_by_name(pinctl_node, "pins_clk");
	of_property_read_u8(pins_clk_node, "drive-strength",
			&host->hw->clk_drv);

	pins_rst_node = of_get_child_by_name(pinctl_node, "pins_rst");
	of_property_read_u8(pins_rst_node, "drive-strength",
			&host->hw->rst_drv);

	pins_ds_node = of_get_child_by_name(pinctl_node, "pins_ds");
	of_property_read_u8(pins_ds_node, "drive-strength", &host->hw->ds_drv);

	/* SDR104 for SDcard */
	pinctl_sdr104_node = of_parse_phandle(np, "pinctl_sdr104", 0);
	pins_cmd_node = of_get_child_by_name(pinctl_sdr104_node, "pins_cmd");
	of_property_read_u8(pins_cmd_node, "drive-strength",
			&host->hw->cmd_drv_sd_18);

	pins_dat_node = of_get_child_by_name(pinctl_sdr104_node, "pins_dat");
	of_property_read_u8(pins_dat_node, "drive-strength",
			&host->hw->dat_drv_sd_18);

	pins_clk_node = of_get_child_by_name(pinctl_sdr104_node, "pins_clk");
	of_property_read_u8(pins_clk_node, "drive-strength",
			&host->hw->clk_drv_sd_18);
	/* SDR50 for SDcard */
	pinctl_sdr50_node = of_parse_phandle(np, "pinctl_sdr50", 0);
	pins_cmd_node = of_get_child_by_name(pinctl_sdr50_node, "pins_cmd");
	of_property_read_u8(pins_cmd_node, "drive-strength",
			&host->hw->cmd_drv_sd_18_sdr50);

	pins_dat_node = of_get_child_by_name(pinctl_sdr50_node, "pins_dat");
	of_property_read_u8(pins_dat_node, "drive-strength",
			&host->hw->dat_drv_sd_18_sdr50);

	pins_clk_node = of_get_child_by_name(pinctl_sdr50_node, "pins_clk");
	of_property_read_u8(pins_clk_node, "drive-strength",
			&host->hw->clk_drv_sd_18_sdr50);

	/* DDR50 for SDcard */
	pinctl_ddr50_node = of_parse_phandle(np, "pinctl_ddr50", 0);
	pins_cmd_node = of_get_child_by_name(pinctl_ddr50_node, "pins_cmd");
	of_property_read_u8(pins_cmd_node, "drive-strength",
			&host->hw->cmd_drv_sd_18_ddr50);

	pins_dat_node = of_get_child_by_name(pinctl_ddr50_node, "pins_dat");
	of_property_read_u8(pins_dat_node, "drive-strength",
			&host->hw->dat_drv_sd_18_ddr50);

	pins_clk_node = of_get_child_by_name(pinctl_ddr50_node, "pins_clk");
	of_property_read_u8(pins_clk_node, "drive-strength",
			&host->hw->clk_drv_sd_18_ddr50);

	return 0;
}

/* Get msdc register settings
 * 1. internal data delay for tuning, FIXME: can be removed when use data tune?
 * 2. sample edge
 */
static int msdc_get_rigister_settings(struct msdc_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct device_node *np = mmc->parent->of_node;
	struct device_node *register_setting_node = NULL;

	/*parse hw property settings*/
	register_setting_node = of_parse_phandle(np, "register_setting", 0);
	if (register_setting_node) {
		of_property_read_u8(register_setting_node, "dat0rddly",
				&host->hw->dat0rddly);
		of_property_read_u8(register_setting_node, "dat1rddly",
				&host->hw->dat1rddly);
		of_property_read_u8(register_setting_node, "dat2rddly",
				&host->hw->dat2rddly);
		of_property_read_u8(register_setting_node, "dat3rddly",
				&host->hw->dat3rddly);
		of_property_read_u8(register_setting_node, "dat4rddly",
				&host->hw->dat4rddly);
		of_property_read_u8(register_setting_node, "dat5rddly",
				&host->hw->dat5rddly);
		of_property_read_u8(register_setting_node, "dat6rddly",
				&host->hw->dat6rddly);
		of_property_read_u8(register_setting_node, "dat7rddly",
				&host->hw->dat7rddly);

		of_property_read_u8(register_setting_node, "datwrddly",
				&host->hw->datwrddly);
		of_property_read_u8(register_setting_node, "cmdrrddly",
				&host->hw->cmdrrddly);
		of_property_read_u8(register_setting_node, "cmdrddly",
				&host->hw->cmdrddly);

		of_property_read_u8(register_setting_node, "cmd_edge",
				&host->hw->cmd_edge);
		of_property_read_u8(register_setting_node, "rdata_edge",
				&host->hw->rdata_edge);
		of_property_read_u8(register_setting_node, "wdata_edge",
				&host->hw->wdata_edge);
	} else {
		pr_err("[msdc%d] register_setting is not found in DT.\n",
				host->id);
		return 1;
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
		pr_err("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	if (!mmc->parent || !mmc->parent->of_node)
		return 1;

	np = mmc->parent->of_node;  /* mmcx node in project dts */
	host->mmc = mmc;  /* msdc_check_init_done() need */
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_err("[msdc%d] of_iomap failed\n", mmc->index);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_err("[msdc%d] get irq # %d\n", mmc->index, host->irq);
	BUG_ON(host->irq < 0);

#ifndef FPGA_PLATFORM
	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_err("[msdc%d] error: clk_src isn't found in device tree.\n",
				host->id);
		BUG_ON(1);
	}
#endif

	/* get msdc flag(caps)*/
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	* -ENODATA if property does not have a value, and -EOVERFLOW if the
	* property data isn't large enough.*/

	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_err("[msdc%d] host_function isn't found in device tree\n",
				host->id);

	if (of_find_property(np, "bootable", &len))
		host->hw->boot = 1;

	/* get cd_gpio and cd_level */
	if (of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio) == 0) {
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_err("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	}

	msdc_get_rigister_settings(host);
	msdc_get_pinctl_settings(host);

	return ret;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	unsigned int id = 0;
	int ret;
	static const char const *msdc_names[] = {
		"msdc0", "msdc1", "msdc2"};
#ifndef FPGA_PLATFORM
	struct device_node *np;
	static char const * const ioconfig_names[] = {
		MSDC0_IOCFG_NAME, MSDC1_IOCFG_NAME,
		MSDC2_IOCFG_NAME
	};
#endif

	pr_err("DT probe %s!\n", pdev->dev.of_node->name);

	for (id = 0; id < HOST_MAX_NUM; id++) {
		if (0 == strcmp(pdev->dev.of_node->name, msdc_names[id])) {
			pdev->id = id;
			break;
		}
	}

	if (id == HOST_MAX_NUM) {
		pr_err("%s: Can not find msdc host\n", __func__);
		return 1;
	}

	host = mmc_priv(mmc);
	host->id = id;

	ret = msdc_of_parse(mmc);
	if (ret) {
		pr_err("%s: msdc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

#ifndef FPGA_PLATFORM
	if (gpio_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
		if (np) {
			gpio_base = of_iomap(np, 0);
			pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
		}
	}

	if (msdc_io_cfg_bases[id] == NULL) {
		np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
		if (np) {
			msdc_io_cfg_bases[id] = of_iomap(np, 0);
			pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
				id, msdc_io_cfg_bases[id]);
		}
	}

	if (infracfg_ao_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL,
			"mediatek,infracfg_ao");
		if (np) {
			infracfg_ao_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for infracfg_ao base @ 0x%p\n",
				infracfg_ao_reg_base);
		}
	}

/*
	if (toprgu_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,toprgu");
		if (np) {
			toprgu_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for toprgu base @ 0x%p\n",
				toprgu_reg_base);
		}
	}
*/

	if (apmixed_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		if (np) {
			apmixed_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for apmixed base @ 0x%p\n",
				apmixed_reg_base);
		}
	}

	if (topckgen_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL,
			"mediatek,topckgen");
		if (np) {
			topckgen_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for topckgen base @ 0x%p\n",
				topckgen_reg_base);
		}
	}

	if (emi_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL,
			"mediatek,emi");
		if (np) {
			emi_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for emi base @ 0x%p\n",
				emi_reg_base);
		}
	}

	if (infracfg_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL,
			"mediatek,infracfg");
		if (np) {
			infracfg_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for infracfg base @ 0x%p\n",
				infracfg_reg_base);
		}
	}

	if (pericfg_reg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL,
			"mediatek,pericfg");
		if (np) {
			pericfg_reg_base = of_iomap(np, 0);
			pr_debug("of_iomap for pericfg base @ 0x%p\n",
				pericfg_reg_base);
		}
	}

	/* backup original dev.of_node */
	np = pdev->dev.of_node;
	/* get regulator supply node */
	pdev->dev.of_node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt_pmic_regulator_supply");
	msdc_get_regulators(&(pdev->dev));
	/* restore original dev.of_node */
	pdev->dev.of_node = np;
#endif

	return 0;
}
