/* Copyright (C) 2015 MediaTek Inc.
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include "mtk_sd.h"
#include "dbg.h"
#include "include/pmic_regulator.h"


struct msdc_host *mtk_msdc_host[] = { NULL, NULL, NULL};
EXPORT_SYMBOL(mtk_msdc_host);

int g_dma_debug[HOST_MAX_NUM] = { 0, 0, 0};
u32 latest_int_status[HOST_MAX_NUM] = { 0, 0, 0};

unsigned int msdc_latest_transfer_mode[HOST_MAX_NUM] = {
	/* 0 for PIO; 1 for DMA; 3 for nothing */
	MODE_NONE,
	MODE_NONE,
	MODE_NONE,
};

unsigned int msdc_latest_op[HOST_MAX_NUM] = {
	/* 0 for read; 1 for write; 2 for nothing */
	OPER_TYPE_NUM,
	OPER_TYPE_NUM,
	OPER_TYPE_NUM
};

/* for debug zone */
unsigned int sd_debug_zone[HOST_MAX_NUM] = {
	0,
	0,
	0
};
/* for enable/disable register dump */
unsigned int sd_register_zone[HOST_MAX_NUM] = {
	1,
	1,
	1
};
/* mode select */
u32 dma_size[HOST_MAX_NUM] = {
	512,
	8,
	512
};

u32 drv_mode[HOST_MAX_NUM] = {
	MODE_SIZE_DEP, /* using DMA or not depend on the size */
	MODE_SIZE_DEP,
	MODE_SIZE_DEP
};

u8 msdc_clock_src[HOST_MAX_NUM] = {
	0,
	0,
	0
};

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = DT_COMPATIBLE_NAME, },
	{ },
};

#if !defined(FPGA_PLATFORM)
static void __iomem *gpio_base;

static void __iomem *pericfg_base;
static void __iomem *apmixed_base;
static void __iomem *topckgen_base;
static void __iomem *sleep_base;
#endif

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

	if (on) { /* want to power on */
		if (*status == 0) {  /* can power on */
			/*Comment out to reduce log */
			/* pr_warn("msdc power on<%d>\n", voltage_uv); */
			(void)msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		} else if (*status == voltage_uv) {
			pr_err("msdc power on <%d> again!\n", voltage_uv);
		} else {
			pr_warn("msdc change<%d> to <%d>\n",
				*status, voltage_uv);
			regulator_disable(reg);
			(void)msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		}
	} else {  /* want to power off */
		if (*status != 0) {  /* has been powerred on */
			pr_warn("msdc power off\n");
			(void)regulator_disable(reg);
			*status = 0;
		} else {
			pr_err("msdc not power on\n");
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
			MASK_VEMC_VOSEL_CAL, SHIFT_VEMC_VOSEL_CAL);
		pr_err(" VEMC_EN=0x%x, VEMC_VOL=0x%x [2b'01(2V9),2b'10(3V),3b'11(3V3)], VEMC_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);
		break;
	case 1:
		pmic_read_interface_nolock(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
			SHIFT_VMC_EN);
		pmic_read_interface_nolock(REG_VMC_VOSEL, &ldo_vol,
			MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
		pmic_read_interface_nolock(REG_VMCH_VOSEL_CAL, &ldo_cal,
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);
		pr_err(" VMC_EN=0x%x, VMC_VOL=0x%x [2b'00(1V86),2b'01(2V9),2b'10(3V),2b'11(3V3)], VMC_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);

		pmic_read_interface_nolock(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
			SHIFT_VMCH_EN);
		pmic_read_interface_nolock(REG_VMCH_VOSEL, &ldo_vol,
			MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
		pmic_read_interface_nolock(REG_VMC_VOSEL_CAL, &ldo_cal,
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
		pr_err(" VMCH_EN=0x%x, VMCH_VOL=0x%x [2b'01(2V9),2b'10(3V),3b'11(3V3)], VMCH_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);
		break;
	default:
		break;
	}
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	if (host->id == 1) {
		msdc_ldo_power(on, host->mmc->supply.vqmmc, VOL_1860,
			&host->power_io);
		msdc_set_tdsel(host, MSDC_TDRDSEL_1V8, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_1V8, 0);
		host->hw->driving_applied = &host->hw->driving_sdr50;
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
		pmic_config_interface(REG_VEMC_VOSEL_CAL,
			VEMC_VOSEL_CAL_mV(EMMC_VOL_ACTUAL - VOL_3000),
			MASK_VEMC_VOSEL_CAL, SHIFT_VEMC_VOSEL_CAL);

	} else if (host->hw->host_function == MSDC_SD) {
		pmic_config_interface(REG_VMCH_VOSEL_CAL,
			VMCH_VOSEL_CAL_mV(SD_VOL_ACTUAL - VOL_3000),
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);
		pmic_config_interface(REG_VMC_VOSEL_CAL,
			VMC_VOSEL_CAL_mV(SD_VOL_ACTUAL - VOL_3000),
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
	}
}

int msdc_oc_check(struct msdc_host *host, u32 en)
{
	u32 val = 0;
	int ret = 0;

	if (host->id == 1) {
		if (en) {
			/*need mask & enable int for 6335*/
			pmic_mask_interrupt(INT_VMCH_OC, "VMCH_OC");
			pmic_enable_interrupt(INT_VMCH_OC, 1, "VMCH_OC");

			pmic_read_interface(REG_VMCH_OC_RAW_STATUS, &val,
				MASK_VMCH_OC_RAW_STATUS, SHIFT_VMCH_OC_RAW_STATUS);

			if (val) {
				host->block_bad_card = 1;
				pr_err("msdc1 OC status = %x\n", val);
				host->power_control(host, 0);

				/*need clear status for 6335*/
				upmu_set_reg_value(REG_VMCH_OC_STATUS, FIELD_VMCH_OC_STATUS);

				ret = -1;
			}
		} else {
			/*need disable & unmask int for 6335*/
			pmic_enable_interrupt(INT_VMCH_OC, 0, "VMCH_OC");
			pmic_unmask_interrupt(INT_VMCH_OC, "VMCH_OC");
		}
	}

	return ret;
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

	msdc_ldo_power(on, host->mmc->supply.vmmc, VOL_3000, &host->power_flash);

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

		if (!on)
			msdc_oc_check(host, 0);
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
		pr_err("Power Off, SD card\n");

		/* power must be on */
		host->power_io = VOL_3000 * 1000;
		host->power_flash = VOL_3000 * 1000;

		host->power_control(host, 0);

		msdc_set_bad_card_and_remove(host);
	}
}
EXPORT_SYMBOL(msdc_sd_power_off);
#else /*else !defined(FPGA_PLATFORM)*/
void msdc_sd_power_off(void)
{
}
EXPORT_SYMBOL(msdc_sd_power_off);
#endif /*endif !defined(FPGA_PLATFORM)*/

void msdc_pmic_force_vcore_pwm(bool enable)
{
#if !defined(FPGA_PLATFORM)
	/* Temporarily disable force pwm */
	/* buck_set_mode(VCORE, enable); */
#endif
}

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


/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
u32 hclks_msdc0[] = { MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2, MSDC0_SRC_3,
		      MSDC0_SRC_4, MSDC0_SRC_5, MSDC0_SRC_6, MSDC0_SRC_7};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc1[] = { MSDC1_SRC_0, MSDC1_SRC_1, MSDC1_SRC_2, MSDC1_SRC_3,
		      MSDC1_SRC_4, MSDC1_SRC_5, MSDC1_SRC_6, MSDC1_SRC_7};

u32 hclks_msdc3[] = { MSDC3_SRC_0, MSDC3_SRC_1, MSDC3_SRC_2, MSDC3_SRC_3,
		      MSDC3_SRC_4, MSDC3_SRC_5, MSDC3_SRC_6, MSDC3_SRC_7};

u32 *hclks_msdc_all[] = {
	hclks_msdc0,
	hclks_msdc1,
	hclks_msdc3,
};
u32 *hclks_msdc;

#include <mtk_clk_id.h>

int msdc_cg_clk_id[HOST_MAX_NUM] = {
	MSDC0_CG_NAME,
	MSDC1_CG_NAME,
	MSDC3_CG_NAME
};

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	static char const * const clk_names[] = {
		MSDC0_CLK_NAME, MSDC1_CLK_NAME, MSDC3_CLK_NAME
	};
	static char const * const hclk_names[] = {
		MSDC0_HCLK_NAME, MSDC1_HCLK_NAME, MSDC3_HCLK_NAME
	};

	host->clk_ctl = devm_clk_get(&pdev->dev, clk_names[pdev->id]);
	if  (hclk_names[pdev->id])
		host->hclk_ctl = devm_clk_get(&pdev->dev, hclk_names[pdev->id]);

	if (IS_ERR(host->clk_ctl)) {
		pr_err("[msdc%d] can not get clock control\n", pdev->id);
		return 1;
	}
	if (clk_prepare(host->clk_ctl)) {
		pr_err("[msdc%d] can not prepare clock control\n", pdev->id);
		return 1;
	}

	if (hclk_names[pdev->id] && IS_ERR(host->hclk_ctl)) {
		pr_err("[msdc%d] can not get clock control\n", pdev->id);
		return 1;
	}
	if (hclk_names[pdev->id] && clk_prepare(host->hclk_ctl)) {
		pr_err("[msdc%d] can not prepare hclock control\n", pdev->id);
		return 1;
	}

	return 0;
}

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	if (host->id != 0) {
		pr_err("[msdc%d] NOT Support switch pll souce[%s]%d\n",
			host->id, __func__, __LINE__);
		return;
	}

	host->hclk = msdc_get_hclk(host->id, clksrc);
	host->hw->clk_src = clksrc;

	pr_err("[%s]: msdc%d select clk_src as %d(%dKHz)\n", __func__,
		host->id, clksrc, host->hclk/1000);
}

#include <linux/seq_file.h>
static void msdc_dump_clock_sts_core(struct msdc_host *host, struct seq_file *m)
{
	char buffer[512];
	char *buf_ptr = buffer;

	if (topckgen_base && pericfg_base) {
		buf_ptr += sprintf(buf_ptr,
			"MSDC0 CLK_MUX@0x%p= 0x%x, CLK_CG[0x%p] = %d, %d\n",
			topckgen_base + 0x140,
			/* mux at bits16~18 */
			(MSDC_READ32(topckgen_base + 0x140) >> 16) & 7,
			pericfg_base + 0x290,
			/* cg at bit 0, 24 */
			(MSDC_READ32(pericfg_base + 0x290) >> 0) & 1,
			(MSDC_READ32(pericfg_base + 0x290) >> 24) & 1);
		buf_ptr += sprintf(buf_ptr,
			"MSDC1 CLK_MUX@0x%p= 0x%x, CLK_CG[0x%p] = %d\n",
			topckgen_base + 0x140,
			/* mux at bits24~26 */
			(MSDC_READ32(topckgen_base + 0x140) >> 24) & 7,
			pericfg_base + 0x290,
			/* mux at bit 1 */
			(MSDC_READ32(pericfg_base + 0x290) >> 1) & 1);
		buf_ptr += sprintf(buf_ptr,
			"MSDC3 CLK_MUX@0x%p= 0x%x, CLK_CG[0x%p] = %d\n",
			topckgen_base + 0x150,
			/* mux at bits8~10 */
			(MSDC_READ32(topckgen_base + 0x150) >> 8) & 7,
			pericfg_base + 0x290,
			/* mux at bit 3 */
			(MSDC_READ32(pericfg_base + 0x290) >> 3) & 1);
		buf_ptr += sprintf(buf_ptr,
			"CLK_CFG_4= 0x%x, CLK_CFG_5= 0x%x\n",
			MSDC_READ32(topckgen_base + 0x140),
			MSDC_READ32(topckgen_base + 0x150));

		*buf_ptr = '\0';
		if (!m)
			pr_err("%s", buffer);
		else
			seq_printf(m, "%s", buffer);
	}

	buf_ptr = buffer;
	if (apmixed_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		buf_ptr += sprintf(buf_ptr, "MSDCPLL_CON0@0x%p = 0x%x, bit[0] shall 1b\n",
			apmixed_base + MSDCPLL_CON0_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON0_OFFSET));

		buf_ptr += sprintf(buf_ptr, "MSDCPLL_CON1@0x%p = 0x%x\n",
			apmixed_base + MSDCPLL_CON1_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON1_OFFSET));

		buf_ptr += sprintf(buf_ptr, "MSDCPLL_CON2@0x%p = 0x%x\n",
			apmixed_base + MSDCPLL_CON2_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON2_OFFSET));

		buf_ptr += sprintf(buf_ptr, "MSDCPLL_PWR_CON0@0x%p = 0x%x, bit[0] shall 1b\n",
			apmixed_base + MSDCPLL_PWR_CON0_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_PWR_CON0_OFFSET));
		*buf_ptr = '\0';
		if (!m)
			pr_err("%s", buffer);
		else
			seq_printf(m, "%s", buffer);
	}
}

void msdc_dump_dvfs_reg(struct msdc_host *host)
{
	void __iomem *base = host->base;

	pr_err("SDC_STS:0x%x", MSDC_READ32(SDC_STS));
	if (sleep_base) {
		/* bit24 high is in dvfs request status, which cause sdc busy */
		pr_err("DVFS_REQUEST@0x%p = 0x%x, bit[24] shall 0b\n",
			sleep_base + 0x478,
			MSDC_READ32(sleep_base + 0x478));
	}
}

void msdc_dump_clock_sts(struct msdc_host *host)
{
	msdc_dump_clock_sts_core(host, NULL);
}

/* FIX ME, consider to remove it */
void dbg_msdc_dump_clock_sts(struct seq_file *m, struct msdc_host *host)
{
	msdc_dump_clock_sts_core(host, m);
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
	pr_err("%s: Vcore %d\n", __func__, vcorefs_get_hw_opp());
}

void msdc_dump_padctl_by_id(u32 id)
{
	if (!gpio_base || !msdc_io_cfg_bases[id]) {
		pr_err("err: gpio_base=%p, msdc_io_cfg_bases[%d]=%p\n",
			gpio_base, id, msdc_io_cfg_bases[id]);
		return;
	}

	if (id == 0) {
		pr_err("MSDC0 MODE10[0x%p] =0x%8x\tshould: 0x1???????\n",
			MSDC0_GPIO_MODE10, MSDC_READ32(MSDC0_GPIO_MODE10));
		pr_err("MSDC0 MODE11[0x%p] =0x%8x\tshould: 0x11111111\n",
			MSDC0_GPIO_MODE11, MSDC_READ32(MSDC0_GPIO_MODE11));
		pr_err("MSDC0 MODE12[0x%p] =0x%8x\tshould: 0x?????111\n",
			MSDC0_GPIO_MODE12, MSDC_READ32(MSDC0_GPIO_MODE12));
		pr_err("MSDC0 IES   [0x%p] =0x%8x\tshould: 0x??????1f\n",
			MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
		pr_err("MSDC0 SMT   [0x%p] =0x%8x\tshould: 0x??????1f\n",
			MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
		pr_err("MSDC0 TDSEL [0x%p] =0x%8x\tshould: 0x???00000\n",
			MSDC0_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_TDSEL_ADDR));
		pr_err("MSDC0 RDSEL [0x%p] =0x%8x\tshould: 0x00000000\n",
			MSDC0_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_RDSEL_ADDR));
		pr_err("MSDC0 DRV   [0x%p] =0x%8x\tshould: 0x???TTTTT\n",
			MSDC0_GPIO_DRV_ADDR,
			MSDC_READ32(MSDC0_GPIO_DRV_ADDR));
		pr_err("PUPD/R1/R0: dat/cmd:0/0/1, clk/dst: 1/1/0\n");
		pr_err("MSDC0 PUPD0 [0x%p] =0x%8x\tshould: 0x1111166?\n",
			MSDC0_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD0_ADDR));
		pr_err("MSDC0 PUPD1 [0x%p] =0x%8x\tshould: 0x????1111\n",
			MSDC0_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD1_ADDR));

	} else if (id == 1) {
		pr_err("MSDC1 MODE18 [0x%p] =0x%8x\tshould: 0x1111????\n",
			MSDC1_GPIO_MODE18, MSDC_READ32(MSDC1_GPIO_MODE18));
		pr_err("MSDC1 MODE19 [0x%p] =0x%8x\tshould: 0x??????11\n",
			MSDC1_GPIO_MODE19, MSDC_READ32(MSDC1_GPIO_MODE19));
		pr_err("MSDC1 IES    [0x%p] =0x%8x\tshould: 0x??????1C\n",
			MSDC1_GPIO_IES_ADDR, MSDC_READ32(MSDC1_GPIO_IES_ADDR));
		pr_err("MSDC1 SMT    [0x%p] =0x%8x\tshould: 0x??????1C\n",
			MSDC1_GPIO_SMT_ADDR, MSDC_READ32(MSDC1_GPIO_SMT_ADDR));
		pr_err("MSDC1 TDSEL  [0x%p] =0x%8x\n",
			MSDC1_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC1_GPIO_TDSEL_ADDR));
		pr_err("should 1.8v: sleep: TBD, awake: TBD\n");
		pr_err("MSDC1 RDSEL0  [0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL_ADDR));
		pr_err("1.8V: TBD, 2.9v: TBD\n");
		pr_err("MSDC1 DRV    [0x%p] =0x%8x\tshould: 0x???TTT??\n",
			MSDC1_GPIO_DRV_ADDR,
			MSDC_READ32(MSDC1_GPIO_DRV_ADDR));
		pr_err("MSDC1 PUPD   [0x%p] =0x%8x\tshould: 0x26??????\n",
			MSDC1_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC1_GPIO_PUPD0_ADDR));
		pr_err("MSDC1 PUPD   [0x%p] =0x%8x\tshould: 0x????2222\n",
			MSDC1_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC1_GPIO_PUPD1_ADDR));

	} else if (id == 2) {
		pr_err("MSDC3 MODE7 [0x%p] =0x%8x\tshould: 0x11??????\n",
			MSDC3_GPIO_MODE7, MSDC_READ32(MSDC3_GPIO_MODE7));
		pr_err("MSDC3 MODE8 [0x%p] =0x%8x\tshould: 0x???11111\n",
			MSDC3_GPIO_MODE8, MSDC_READ32(MSDC3_GPIO_MODE8));
		pr_err("MSDC3 IES   [0x%p] =0x%8x\tshould: bit3\n",
			MSDC3_GPIO_IES_ADDR, MSDC_READ32(MSDC3_GPIO_IES_ADDR));
		pr_err("MSDC3 SMT   [0x%p] =0x%8x\tshould: bit3\n",
			MSDC3_GPIO_SMT_ADDR, MSDC_READ32(MSDC3_GPIO_SMT_ADDR));
		pr_err("MSDC3 TDSEL  [0x%p] =0x%8x\tshould: bit12~15\n",
			MSDC3_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC3_GPIO_TDSEL_ADDR));
		pr_err("MSDC3 RDSEL  [0x%p] =0x%8x\tshould: bit6~11\n",
			MSDC3_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC3_GPIO_RDSEL_ADDR));
		pr_err("MSDC3 DRV    [0x%p] =0x%8x\tshould: bit12~15\n",
			MSDC3_GPIO_DRV_ADDR, MSDC_READ32(MSDC3_GPIO_DRV_ADDR));
		pr_err("MSDC3 PUPD0   [0x%p] =0x%8x\n",
			MSDC3_GPIO_PUPD_ADDR,
			MSDC_READ32(MSDC3_GPIO_PUPD_ADDR));

	}
}

void msdc_set_pin_mode(struct msdc_host *host)
{
	if (host->id == 0) {
		MSDC_SET_FIELD(MSDC0_GPIO_MODE10, 0xF0000000, 0x1);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE11, 0xFFFFFFFF, 0x11111111);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE12, 0xFFF, 0x111);
	} else if (host->id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_MODE18, 0xFFFF0000, 0x1111);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE19, 0x000000FF, 0x11);
	} else if (host->id == 2) {
		MSDC_SET_FIELD(MSDC3_GPIO_MODE7, 0xFF000000, 0x11);
		MSDC_SET_FIELD(MSDC3_GPIO_MODE8, 0x000FFFFF, 0x11111);
	}
}

void msdc_set_ies_by_id(u32 id, int set_ies)
{
	if (id == 0) {
		MSDC_SET_FIELD(MSDC0_GPIO_IES_ADDR, MSDC0_IES_ALL_MASK,
			(set_ies ? 0x1F : 0));
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_IES_ADDR, MSDC1_IES_ALL_MASK,
			(set_ies ? 0x7 : 0));
	} else if (id == 2) {
		MSDC_SET_FIELD(MSDC3_GPIO_IES_ADDR, MSDC3_IES_ALL_MASK,
			(set_ies ? 0x1 : 0));
	}
}

void msdc_set_smt_by_id(u32 id, int set_smt)
{
	if (id == 0) {
		MSDC_SET_FIELD(MSDC0_GPIO_SMT_ADDR, MSDC0_SMT_ALL_MASK,
			(set_smt ? 0x1F : 0));
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_SMT_ADDR, MSDC1_SMT_ALL_MASK,
			(set_smt ? 0x7 : 0));
	} else if (id == 2) {
		MSDC_SET_FIELD(MSDC3_GPIO_SMT_ADDR, MSDC3_SMT_ALL_MASK,
			(set_smt ? 0x1 : 0));
	}
}

void msdc_set_tdsel_by_id(u32 id, u32 flag, u32 value)
{
	u32 cust_val;

	if (id == 0) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_CLK_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_RSTB_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_DSL_MASK,
			cust_val);
	} else if (id == 1) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_CLK_MASK,
			cust_val);
	} else if (id == 2) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC3_GPIO_TDSEL_ADDR, MSDC3_TDSEL_ALL_MASK,
			cust_val);
	}
}

void msdc_set_rdsel_by_id(u32 id, u32 flag, u32 value)
{
	u32 cust_val;

	if (id == 0) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_CLK_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_RSTB_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_DSL_MASK,
			cust_val);
	} else if (id == 1) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL_ADDR, MSDC1_RDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL_ADDR, MSDC1_RDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL_ADDR, MSDC1_RDSEL_CLK_MASK,
			cust_val);
	} else if (id == 2) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC3_GPIO_RDSEL_ADDR, MSDC3_RDSEL_ALL_MASK,
			cust_val);
	}
}

void msdc_get_tdsel_by_id(u32 id, u32 *value)
{
	if (id == 0) {
		MSDC_GET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_CMD_MASK,
			*value);
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_CMD_MASK,
			*value);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC3_GPIO_TDSEL_ADDR, MSDC3_TDSEL_ALL_MASK,
			*value);
	}
}

void msdc_get_rdsel_by_id(u32 id, u32 *value)
{
	if (id == 0) {
		MSDC_GET_FIELD(MSDC0_GPIO_RDSEL_ADDR, MSDC0_RDSEL_CMD_MASK,
			*value);
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL_ADDR, MSDC1_RDSEL_CMD_MASK,
			*value);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC3_GPIO_RDSEL_ADDR, MSDC3_RDSEL_ALL_MASK,
			*value);
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst, int ds)
{
	if (id == 0) {
		/* do nothing since 10nm does not have SR control for 1.8V */
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_SR_DAT_MASK,
			(dat != 0));
	} else if (id == 2) {
		/* do nothing since 10nm does not have SR control for 1.8V */
	}
}

void msdc_set_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	pr_err("msdc%d set driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
		id,
		driving->clk_drv,
		driving->cmd_drv,
		driving->dat_drv,
		driving->rst_drv,
		driving->ds_drv);

	if (id == 0) {
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
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
			driving->cmd_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
			driving->clk_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
			driving->dat_drv);
	} else if (id == 2) {
		MSDC_SET_FIELD(MSDC3_GPIO_DRV_ADDR, MSDC3_DRV_ALL_MASK,
			driving->dat_drv);
	}
}

void msdc_get_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	if (id == 0) {
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
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
			driving->cmd_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
			driving->clk_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
			driving->dat_drv);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC3_GPIO_DRV_ADDR, MSDC3_DRV_ALL_MASK,
			driving->dat_drv);
		MSDC_GET_FIELD(MSDC3_GPIO_DRV_ADDR, MSDC3_DRV_ALL_MASK,
			driving->cmd_drv);
		MSDC_GET_FIELD(MSDC3_GPIO_DRV_ADDR, MSDC3_DRV_ALL_MASK,
			driving->clk_drv);
		MSDC_GET_FIELD(MSDC3_GPIO_DRV_ADDR, MSDC3_DRV_ALL_MASK,
			driving->ds_drv);
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
	if (id == 0) {
		/* 1. don't pull CLK high;
		 * 2. Don't toggle RST to prevent from entering boot mode
		 */
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC0_* to no ohm PU */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x0000000);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0x0000);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC0_* to 50K ohm PD */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x6666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0x6666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC0_CLK to 50K ohm PD,
			 * MSDC0_CMD/MSDC0_DAT* to 10K ohm PU,
			 * MSDC0_DSL to 50K ohm PD
			 */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR, MSDC0_PUPD0_MASK, 0x1111166);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR, MSDC0_PUPD1_MASK, 0x1111);
		}
	} else if (id == 1) {
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC1_* to no ohm PU */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR, MSDC1_PUPD0_MASK, 0x00);
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD1_ADDR, MSDC1_PUPD1_MASK, 0x0000);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC1_* to 50K ohm PD */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR, MSDC1_PUPD0_MASK, 0x66);
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD1_ADDR, MSDC1_PUPD1_MASK, 0x6666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC1_CLK to 50K ohm PD,
			* MSDC1_CMD/MSDC1_DAT* to 50K ohm PU
			*/
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR, MSDC1_PUPD0_MASK, 0x26);
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD1_ADDR, MSDC1_PUPD1_MASK, 0x2222);
		}
	} else if (id == 2) {
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC3_* to 0 ohm PU
			 */
			MSDC_SET_FIELD(MSDC3_GPIO_PUPD_ADDR, MSDC3_PUPD_MASK, 0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC3_* to 50K ohm PD
			 */
			MSDC_SET_FIELD(MSDC3_GPIO_PUPD_ADDR, MSDC3_PUPD_MASK, 0x6666666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC3_CLK/DSL to 50K ohm PD,
			 * MSDC3_CMD/MSDC3_DAT* to 10K ohm PU
			 */
			MSDC_SET_FIELD(MSDC3_GPIO_PUPD_ADDR, MSDC3_PUPD_MASK, 0x1111616);
		}
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
	static char const * const pinctl_names[] = {
		"pinctl", "pinctl_sdr104", "pinctl_sdr50", "pinctl_ddr50"
	};

	/* sequence shall be the same as sequence in msdc_hw_driving */
	static char const * const pins_names[] = {
		"pins_cmd", "pins_dat", "pins_clk", "pins_rst", "pins_ds"
	};
	unsigned char *pin_drv;
	int i, j;

	host->hw->driving_applied = &host->hw->driving;
	for (i = 0; i < ARRAY_SIZE(pinctl_names); i++) {
		pinctl_node = of_parse_phandle(np, pinctl_names[i], 0);

		if (strcmp(pinctl_names[i], "pinctl") == 0)
			pin_drv = (unsigned char *)&host->hw->driving;
		else if (strcmp(pinctl_names[i], "pinctl_sdr104") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sdr104;
		else if (strcmp(pinctl_names[i], "pinctl_sdr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_sdr50;
		else if (strcmp(pinctl_names[i], "pinctl_ddr50") == 0)
			pin_drv = (unsigned char *)&host->hw->driving_ddr50;
		else
			continue;

		for (j = 0; j < ARRAY_SIZE(pins_names); j++) {
			pins_node = of_get_child_by_name(pinctl_node,
				pins_names[j]);

			if (pins_node)
				of_property_read_u8(pins_node,
					"drive-strength", pin_drv);
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
static int msdc_get_register_settings(struct msdc_host *host, struct device_node *np)
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
		pr_err("[msdc%d] register_setting is not found in DT\n",
			host->id);
	}

	return 0;
}

/*
 *	msdc_of_parse() - parse host's device-tree node
 *	@host: host whose node should be parsed.
 *
 */
int msdc_of_parse(struct platform_device *pdev, struct mmc_host *mmc)
{
	struct device_node *np;
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;
	int len = 0;
	u8 hw_dvfs_support, id;

	np = mmc->parent->of_node; /* mmcx node in project dts */

	if (of_property_read_u8(np, "index", &id)) {
		pr_err("[%s] host index not specified in device tree\n",
			pdev->dev.of_node->name);
		return -1;
	}
	host->id = id;
	pdev->id = id;

	pr_err("DT probe %s%d!\n", pdev->dev.of_node->name, id);

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_err("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	host->mmc = mmc;
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_err("[msdc%d] of_iomap failed\n", mmc->index);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_err("[msdc%d] get irq # %d\n", host->id, host->irq);
	WARN_ON(host->irq < 0);

#if !defined(FPGA_PLATFORM)
	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_err("[msdc%d] error: clk_src isn't found in device tree.\n",
			host->id);
		WARN_ON(1);
	}
#endif

	/* get msdc flag(caps)*/
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	if (of_find_property(np, "sd-uhs-ddr208", &len))
		host->hw->flags |= MSDC_SDIO_DDR208;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	 * -ENODATA if property does not have a value, and -EOVERFLOW if the
	 * property data isn't large enough.
	 */
	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_err("[msdc%d] host_function isn't found in device tree\n",
			host->id);

	/* get cd_gpio and cd_level */
	if (of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio) == 0) {
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_err("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	}

	ret = of_property_read_u8(np, "hw_dvfs", &hw_dvfs_support);
	if (ret || (hw_dvfs_support == 0))
		host->use_hw_dvfs = 0xFF;
	else
		host->use_hw_dvfs = 0;

	msdc_get_register_settings(host, np);

#if !defined(FPGA_PLATFORM)
	msdc_get_pinctl_settings(host, np);

	mmc->supply.vmmc = regulator_get(mmc_dev(mmc), "vmmc");
	mmc->supply.vqmmc = regulator_get(mmc_dev(mmc), "vqmmc");
#else
	msdc_fpga_pwr_init();
#endif

#if !defined(FPGA_PLATFORM)
	if (host->hw->host_function == MSDC_EMMC) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,msdc_top");
		host->base_top = of_iomap(np, 0);
		pr_debug("of_iomap for MSDC%d TOP base @ 0x%p\n",
			host->id, host->base_top);
	}
#endif

#if defined(CFG_DEV_MSDC3)
	if (host->hw->host_function == MSDC_SDIO) {
		host->hw->flags |= MSDC_EXT_SDIO_IRQ;
		host->hw->request_sdio_eirq = mt_sdio_ops[3].sdio_request_eirq;
		host->hw->enable_sdio_eirq = mt_sdio_ops[3].sdio_enable_eirq;
		host->hw->disable_sdio_eirq = mt_sdio_ops[3].sdio_disable_eirq;
		host->hw->register_pm = mt_sdio_ops[3].sdio_register_pm;
	}
#endif

	if (host->id == 0)
		device_rename(mmc->parent, "bootdevice");
	else if (host->id == 1)
		device_rename(mmc->parent, "externdevice");

	return host->id;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	int id;

#ifndef FPGA_PLATFORM
	static char const * const ioconfig_names[] = {
		MSDC0_IOCFG_NAME, MSDC1_IOCFG_NAME,
		MSDC3_IOCFG_NAME
	};
	struct device_node *np;
#endif

	id = msdc_of_parse(pdev, mmc);
	if (id < 0) {
		pr_err("%s: msdc_of_parse error!!: %d\n", __func__, id);
		return id;
	}

#ifndef FPGA_PLATFORM
	if (gpio_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,GPIO");
		gpio_base = of_iomap(np, 0);
		pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
	}

	if (msdc_io_cfg_bases[id] == NULL) {
		np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
		msdc_io_cfg_bases[id] = of_iomap(np, 0);
		pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
			id, msdc_io_cfg_bases[id]);
	}

	if (apmixed_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-apmixedsys");
		apmixed_base = of_iomap(np, 0);
		pr_debug("of_iomap for apmixed base @ 0x%p\n",
			apmixed_base);
	}

	if (topckgen_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-topckgen");
		topckgen_base = of_iomap(np, 0);
		pr_debug("of_iomap for topckgen base @ 0x%p\n",
			topckgen_base);
	}

	if (sleep_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
		sleep_base = of_iomap(np, 0);
		pr_debug("of_iomap for sleep base @ 0x%p\n",
			sleep_base);
	}

	if (pericfg_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-pericfg");
		pericfg_base = of_iomap(np, 0);
		pr_debug("of_iomap for pericfg base @ 0x%p\n",
			pericfg_base);
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
	OFFSET_MSDC_PAD_TUNE0,
	OFFSET_MSDC_PAD_TUNE1,
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
	OFFSET_MSDC_AES_SEL,

	0xFFFF /*as mark of end */
};

u16 msdc_offsets_top[] = {
	OFFSET_EMMC_TOP_CONTROL,
	OFFSET_EMMC_TOP_CMD,
	OFFSET_TOP_EMMC50_PAD_CTL0,
	OFFSET_TOP_EMMC50_PAD_DS_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT0_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT1_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT2_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT3_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT4_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT5_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT6_TUNE,
	OFFSET_TOP_EMMC50_PAD_DAT7_TUNE,

	0xFFFF /*as mark of end */
};
