/* Copyright (C) 2017 MediaTek Inc.
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
#include <linux/of_gpio.h>

#include "mtk_sd.h"
#include "dbg.h"
#include "include/pmic_regulator.h"


struct msdc_host *mtk_msdc_host[HOST_MAX_NUM];
EXPORT_SYMBOL(mtk_msdc_host);

int g_dma_debug[HOST_MAX_NUM];
u32 latest_int_status[HOST_MAX_NUM];

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
	512,
	512
};

u32 drv_mode[HOST_MAX_NUM] = {
	MODE_SIZE_DEP, /* using DMA or not depend on the size */
	MODE_SIZE_DEP,
	MODE_SIZE_DEP
};

int dma_force[HOST_MAX_NUM];

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = DT_COMPATIBLE_NAME, },
	{ },
};

const struct of_device_id msdc2_of_ids[] = {
	{   .compatible = DT_COMPATIBLE_NAME2, },
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
			/* Comment out to reduce log */
			/* pr_info("msdc power on<%d>\n", voltage_uv); */
			(void)msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		} else if (*status == voltage_uv) {
			pr_notice("msdc power on <%d> again!\n", voltage_uv);
		} else {
			regulator_disable(reg);
			(void)msdc_regulator_set_and_enable(reg, voltage_uv);
			*status = voltage_uv;
		}
	} else {  /* want to power off */
		if (*status != 0) {  /* has been powerred on */
			/* Comment out to reduce log */
			/* pr_info("msdc power off\n"); */
			(void)regulator_disable(reg);
			*status = 0;
		} else {
			pr_notice("msdc not power on\n");
		}
	}
}

void msdc_dump_ldo_sts(char **buff, unsigned long *size,
		struct seq_file *m, struct msdc_host *host)
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
		pr_info(" VEMC_EN=0x%x, VEMC_VOL=0x%x [2b'01(2V9),2b'10(3V),3b'11(3V3)], VEMC_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);
		break;
	case 1:
		pmic_read_interface_nolock(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
			SHIFT_VMC_EN);
		pmic_read_interface_nolock(REG_VMC_VOSEL, &ldo_vol,
			MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
		pmic_read_interface_nolock(REG_VMCH_VOSEL_CAL, &ldo_cal,
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);
		pr_info(" VMC_EN=0x%x, VMC_VOL=0x%x [2b'00(1V86),2b'01(2V9),2b'10(3V),2b'11(3V3)], VMC_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);

		pmic_read_interface_nolock(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
			SHIFT_VMCH_EN);
		pmic_read_interface_nolock(REG_VMCH_VOSEL, &ldo_vol,
			MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
		pmic_read_interface_nolock(REG_VMC_VOSEL_CAL, &ldo_cal,
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
		pr_info(" VMCH_EN=0x%x, VMCH_VOL=0x%x [2b'01(2V9),2b'10(3V),3b'11(3V3)], VMCH_CAL=0x%x\n",
			ldo_en, ldo_vol, ldo_cal);
		break;
	default:
		break;
	}
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	if (host->id == 1) {
		/* VMC cal +60mV when power on */
		if (on)
			pmic_set_register_value(PMIC_RG_VMC_VOCAL, 0x6);

		msdc_ldo_power(on, host->mmc->supply.vqmmc, VOL_1860,
			&host->power_io);
		msdc_set_tdsel(host, MSDC_TDRDSEL_CUST, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_CUST, 0);
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

	if (host->id != 1)
		goto out;

	if (en) {
		/*need mask & enable int for 6335*/
		pmic_mask_interrupt(INT_VMCH_OC, "VMCH_OC");
		pmic_enable_interrupt(INT_VMCH_OC, 1, "VMCH_OC");

		pmic_read_interface(REG_VMCH_OC_RAW_STATUS, &val,
			MASK_VMCH_OC_RAW_STATUS, SHIFT_VMCH_OC_RAW_STATUS);

		if (val) {
			pr_notice("msdc1 OC status = %x\n", val);
			host->power_control(host, 0);
			msdc_set_bad_card_and_remove(host);

			/*need clear status for 6335*/
			upmu_set_reg_value(REG_VMCH_OC_STATUS,
				FIELD_VMCH_OC_STATUS);

			ret = 1;
		}
	} else {
		/*need disable & unmask int for 6335*/
		pmic_enable_interrupt(INT_VMCH_OC, 0, "VMCH_OC");
		pmic_unmask_interrupt(INT_VMCH_OC, "VMCH_OC");
	}

out:
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
		msdc_set_tdsel(host, MSDC_TDRDSEL_CUST, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_CUST, 0);
	}

	msdc_ldo_power(on, host->mmc->supply.vmmc, VOL_3000,
		&host->power_flash);

	pr_info("msdc%d power %s\n", host->id, (on ? "on" : "off"));

#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_ldo_sts(NULL, 0, NULL, host);
#endif
}

void msdc_sd_power(struct msdc_host *host, u32 on)
{
	u32 card_on = on;

	switch (host->id) {
	case 1:
		msdc_set_driving(host, &host->hw->driving);
		msdc_set_tdsel(host, MSDC_TDRDSEL_CUST, 0);
		msdc_set_rdsel(host, MSDC_TDRDSEL_CUST, 0);
		if (host->hw->flags & MSDC_SD_NEED_POWER)
			card_on = 1;
		/* VMCH VOLSEL */
		msdc_ldo_power(card_on, host->mmc->supply.vmmc, VOL_3000,
			&host->power_flash);
		/* VMC VOLSEL */
		msdc_ldo_power(on, host->mmc->supply.vqmmc, VOL_3000,
			&host->power_io);

		/* Clear VMC cal when power off */
		if (!on)
			pmic_set_register_value(PMIC_RG_VMC_VOCAL, 0x0);
		pr_info("msdc%d power %s\n", host->id, (on ? "on" : "off"));
		break;

	default:
		break;
	}

#ifdef MTK_MSDC_BRINGUP_DEBUG
	msdc_dump_ldo_sts(NULL, 0, NULL, host);
#endif
}

void msdc_sd_power_off(void)
{
	struct msdc_host *host = mtk_msdc_host[1];

	if (host) {
		pr_info("Power Off, SD card\n");

		/* power must be on */
		host->power_io = VOL_3000 * 1000;
		host->power_flash = VOL_3000 * 1000;

		host->power_control(host, 0);

		msdc_set_bad_card_and_remove(host);
	}
}
EXPORT_SYMBOL(msdc_sd_power_off);

void msdc_dump_vcore(char **buff, unsigned long *size, struct seq_file *m)
{
	pr_info("%s: Vcore %d\n", __func__, vcorefs_get_hw_opp());
}

/* FIXME: check if sleep_base is correct */
void msdc_dump_dvfs_reg(char **buff, unsigned long *size, struct seq_file *m,
	struct msdc_host *host)
{
	void __iomem *base = host->base;

	pr_info("SDC_STS:0x%x", MSDC_READ32(SDC_STS));
	if (sleep_base) {
		/* bit24 high is in dvfs request status, which cause sdc busy */
		pr_info("DVFS_REQUEST@0x%p = 0x%x, bit[24] shall 0b\n",
			sleep_base + 0x478,
			MSDC_READ32(sleep_base + 0x478));
	}
}

void msdc_pmic_force_vcore_pwm(bool enable)
{
	/* Temporarily disable force pwm */
	/* buck_set_mode(VCORE, enable); */
}
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

#if defined(MSDC_HQA) && defined(SDIO_HQA)
#error shall not define both MSDC_HQA and SDIO_HQA
#endif
#if defined(MSDC_HQA) || defined(SDIO_HQA)
/* #define MSDC_HQA_HV */
/* #define MSDC_HQA_NV */
#define MSDC_HQA_LV

void msdc_HQA_set_voltage(struct msdc_host *host)
{
#if defined(MSDC_HQA_HV) || defined(MSDC_HQA_LV)
	u32 vcore, vio = 0, val_delta;
#endif
	u32 vio_cal = 0;
	static int vcore_orig = -1, vio_orig = -1;

	if (host->is_autok_done == 1)
		return;

	if (vcore_orig < 0)
		pmic_read_interface(0xBE8, &vcore_orig, 0x7F, 0);
	if (vio_orig < 0)
		pmic_read_interface(0x1296, &vio_orig, 0xF, 8);
	pmic_read_interface(0x1296, &vio_cal, 0xF, 0);
	pr_info("[MSDC%d HQA] orig Vcore 0x%x, Vio 0x%x, Vio_cal 0x%x\n",
		host->id, vcore_orig, vio_orig, vio_cal);

#if defined(MSDC_HQA_HV) || defined(MSDC_HQA_LV)
	val_delta = (500000 + vcore_orig * 6250) / 20 / 6250;

	#ifdef MSDC_HQA_HV
	vcore = vcore_orig + val_delta;
	vio_cal = 0xa;
	#endif

	#ifdef MSDC_HQA_LV
	vcore = vcore_orig - val_delta;
	vio_cal = 0;
	vio = vio_orig - 1;
	#endif

	pmic_config_interface(0xBE8, vcore, 0x7F, 0);

	if (vio_cal)
		pmic_config_interface(0x1296, vio_cal, 0xF, 0);
	else
		pmic_config_interface(0x1296, vio, 0xF, 8);

	pr_info("[MSDC%d HQA] adj Vcore 0x%x, Vio 0x%x, Vio_cal 0x%x\n",
		host->id, vcore, vio, vio_cal);
#endif

}
#endif


/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
u32 hclks_msdc0[] = { MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2, MSDC0_SRC_3,
		      MSDC0_SRC_4, MSDC0_SRC_5};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc1[] = { MSDC1_SRC_0, MSDC1_SRC_1, MSDC1_SRC_2, MSDC1_SRC_3,
		      MSDC1_SRC_4};

u32 hclks_msdc2[] = { MSDC2_SRC_0, MSDC2_SRC_1, MSDC2_SRC_2, MSDC2_SRC_3,
		      MSDC2_SRC_4};

u32 *hclks_msdc_all[] = {
	hclks_msdc0,
	hclks_msdc1,
	hclks_msdc2,
};
u32 *hclks_msdc;

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	static char const * const clk_names[] = {
		MSDC0_CLK_NAME, MSDC1_CLK_NAME, MSDC2_CLK_NAME
	};
	static char const * const hclk_names[] = {
		MSDC0_HCLK_NAME, MSDC1_HCLK_NAME, MSDC2_HCLK_NAME
	};

	if  (clk_names[pdev->id]) {
		host->clk_ctl = devm_clk_get(&pdev->dev, clk_names[pdev->id]);
		if (IS_ERR(host->clk_ctl)) {
			pr_notice("[msdc%d] cannot get clk ctrl\n", pdev->id);
			return 1;
		}
		if (clk_prepare(host->clk_ctl)) {
			pr_notice("[msdc%d] cannot prepare clk ctrl\n",
				pdev->id);
			return 1;
		}
	}

	if  (hclk_names[pdev->id]) {
		host->hclk_ctl = devm_clk_get(&pdev->dev, hclk_names[pdev->id]);
		if (IS_ERR(host->hclk_ctl)) {
			pr_notice("[msdc%d] cannot get hclk ctrl\n", pdev->id);
			return 1;
		}
		if (clk_prepare(host->hclk_ctl)) {
			pr_notice("[msdc%d] cannot prepare hclk ctrl\n",
				pdev->id);
			return 1;
		}
	}

	host->hclk = clk_get_rate(host->clk_ctl);

	pr_info("[msdc%d] hclk:%d, clk_ctl:%p, hclk_ctl:%p\n",
		pdev->id, host->hclk, host->clk_ctl, host->hclk_ctl);

	return 0;
}

#include <linux/seq_file.h>
static void msdc_dump_clock_sts_core(char **buff, unsigned long *size,
	struct seq_file *m, struct msdc_host *host)
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
			"MSDC2 CLK_MUX@0x%p= 0x%x, CLK_CG[0x%p] = %d\n",
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
			pr_info("%s", buffer);
		else
			seq_printf(m, "%s", buffer);
	}

	buf_ptr = buffer;
	if (apmixed_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		buf_ptr += sprintf(buf_ptr,
			"MSDCPLL_CON0@0x%p = 0x%x,bit[0] shall 1b\n",
			apmixed_base + MSDCPLL_CON0_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON0_OFFSET));

		buf_ptr += sprintf(buf_ptr, "MSDCPLL_CON1@0x%p = 0x%x\n",
			apmixed_base + MSDCPLL_CON1_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON1_OFFSET));

		buf_ptr += sprintf(buf_ptr, "MSDCPLL_CON2@0x%p = 0x%x\n",
			apmixed_base + MSDCPLL_CON2_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_CON2_OFFSET));

		buf_ptr += sprintf(buf_ptr,
			"MSDCPLL_PWR_CON0@0x%p = 0x%x,bit[0] shall 1b\n",
			apmixed_base + MSDCPLL_PWR_CON0_OFFSET,
			MSDC_READ32(apmixed_base + MSDCPLL_PWR_CON0_OFFSET));
		*buf_ptr = '\0';
		if (!m)
			pr_info("%s", buffer);
		else
			seq_printf(m, "%s", buffer);
	}
}

void msdc_dump_clock_sts(char **buff, unsigned long *size,
	struct seq_file *m, struct msdc_host *host)
{
	msdc_dump_clock_sts_core(buff, size, m, host);
}

void msdc_clk_enable_and_stable(struct msdc_host *host)
{
	void __iomem *base = host->base;
	u32 div, mode, hs400_div_dis;
	u32 val;

	msdc_clk_enable(host);

	/* udelay(10); */

	/* MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC); */

	val = MSDC_READ32(MSDC_CFG);
	GET_FIELD(val, CFG_CKDIV_SHIFT, CFG_CKDIV_MASK, div);
	GET_FIELD(val, CFG_CKMOD_SHIFT, CFG_CKMOD_MASK, mode);
	GET_FIELD(val, CFG_CKMOD_HS400_SHIFT, CFG_CKMOD_HS400_MASK,
		hs400_div_dis);
	msdc_clk_stable(host, mode, div, hs400_div_dis);
}

#endif /*if !defined(FPGA_PLATFORM)*/
/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/*
 * Power off card on the 2 bad card conditions:
 * 1. if dat pins keep high when pulled low or
 * 2. dat pins alway keeps high
 */
int msdc_io_check(struct msdc_host *host)
{
	int i;
	void __iomem *base = host->base;
	unsigned long polling_tmo = 0;
	void __iomem *pupd_addr[3] = {
		MSDC1_PUPD_DAT0_ADDR,
		MSDC1_PUPD_DAT1_ADDR,
		MSDC1_PUPD_DAT2_ADDR
	};
	u32 pupd_mask[3] = {
		MSDC1_PUPD0_DAT0_MASK,
		MSDC1_PUPD0_DAT1_MASK,
		MSDC1_PUPD0_DAT2_MASK
	};
	u32 check_patterns[3] = {0xE0000, 0xD0000, 0xB0000};
	u32 orig_pull;

	if (host->id != 1)
		return 0;

	if (host->block_bad_card)
		goto SET_BAD_CARD;

	for (i = 0; i < 3; i++) {
		MSDC_GET_FIELD(pupd_addr[i], pupd_mask[i], orig_pull);
		MSDC_SET_FIELD(pupd_addr[i], pupd_mask[i], MSDC_PD_50K);
		polling_tmo = jiffies + POLLING_PINS;
		while ((MSDC_READ32(MSDC_PS) & 0xF0000) != check_patterns[i]) {
			if (time_after(jiffies, polling_tmo)) {
				/* Exception handling for some good card with
				 * pull up strength greater than pull up
				 * strength
				 * of gpio.
				 */
				if ((MSDC_READ32(MSDC_PS) & 0xF0000) == 0xF0000)
					break;
				pr_notice("msdc%d DAT%d pin get wrong, ps = 0x%x!\n",
					host->id, i, MSDC_READ32(MSDC_PS));
				goto SET_BAD_CARD;
			}
		}
		MSDC_SET_FIELD(pupd_addr[i], pupd_mask[i], orig_pull);
	}

	return 0;

SET_BAD_CARD:
	msdc_set_bad_card_and_remove(host);
	return 1;
}

void msdc_dump_padctl_by_id(char **buff, unsigned long *size,
		struct seq_file *m, u32 id)
{
	if (!gpio_base || !msdc_io_cfg_bases[id]) {
		pr_info("err: gpio_base=%p, msdc_io_cfg_bases[%d]=%p\n",
			gpio_base, id, msdc_io_cfg_bases[id]);
		return;
	}

	if (id == 0) {
		pr_info("MSDC0 MODE16 [0x%p] =0x%8x\tshould: 0x222121?? or 0x111111??\n",
			MSDC0_GPIO_MODE16, MSDC_READ32(MSDC0_GPIO_MODE16));
		pr_info("MSDC0 MODE17 [0x%p] =0x%8x\tshould: 0x??121122 or 0x??111111\n",
			MSDC0_GPIO_MODE17, MSDC_READ32(MSDC0_GPIO_MODE17));
		pr_info("MSDC0 IES    [0x%p] =0x%8x\tshould: 0x??????1f\n",
			MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
		pr_info("MSDC0 SMT    [0x%p] =0x%8x\tshould: 0x??????1f\n",
			MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
		pr_info("MSDC0 TDSEL0 [0x%p] =0x%8x\n",
			MSDC0_GPIO_TDSEL0_ADDR,
			MSDC_READ32(MSDC0_GPIO_TDSEL0_ADDR));
		pr_info("MSDC0 RDSEL0 [0x%p] =0x%8x\n",
			MSDC0_GPIO_RDSEL0_ADDR,
			MSDC_READ32(MSDC0_GPIO_RDSEL0_ADDR));
		pr_info("MSDC0 DRV0   [0x%p] =0x%8x\n",
			MSDC0_GPIO_DRV0_ADDR,
			MSDC_READ32(MSDC0_GPIO_DRV0_ADDR));
		pr_info("PUPD/R1/R0: dat/cmd:0/0/1, clk/dst: 1/1/0\n");
		pr_info("MSDC0 PUPD0  [0x%p] =0x%8x\tshould: 0x11111611\n",
			MSDC0_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD0_ADDR));
		pr_info("MSDC0 PUPD1  [0x%p] =0x%8x\tshould: 0x????1161\n",
			MSDC0_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD1_ADDR));

	} else if (id == 1) {
		pr_info("MSDC1 MODE4  [0x%p] =0x%8x\tshould: 0x111?????\n",
			MSDC1_GPIO_MODE4, MSDC_READ32(MSDC1_GPIO_MODE4));
		pr_info("MSDC1 MODE5  [0x%p] =0x%8x\tshould: 0x?????111\n",
			MSDC1_GPIO_MODE5, MSDC_READ32(MSDC1_GPIO_MODE5));
		pr_info("MSDC1 IES    [0x%p] =0x%8x\tshould: 0x??????7?\n",
			MSDC1_GPIO_IES_ADDR, MSDC_READ32(MSDC1_GPIO_IES_ADDR));
		pr_info("MSDC1 SMT    [0x%p] =0x%8x\tshould: 0x??????7?\n",
			MSDC1_GPIO_SMT_ADDR, MSDC_READ32(MSDC1_GPIO_SMT_ADDR));
		pr_info("MSDC1 TDSEL0 [0x%p] =0x%8x\n",
			MSDC1_GPIO_TDSEL0_ADDR,
			MSDC_READ32(MSDC1_GPIO_TDSEL0_ADDR));
		pr_info("should 1.8v: sleep: TBD, awake: TBD\n");
		pr_info("MSDC1 RDSEL0 [0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL0_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL0_ADDR));
		pr_info("1.8V: TBD, 2.9v: TBD\n");
		pr_info("MSDC1 DRV0   [0x%p] =0x%8x\n",
			MSDC1_GPIO_DRV0_ADDR,
			MSDC_READ32(MSDC1_GPIO_DRV0_ADDR));
		pr_info("MSDC1 PUPD0  [0x%p] =0x%8x\tshould: 0x??222226\n",
			MSDC1_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC1_GPIO_PUPD0_ADDR));

	} else if (id == 2) {
		pr_info("MSDC2 MODE15 [0x%p] =0x%8x\tshould: 0x2222????\n",
			MSDC2_GPIO_MODE15, MSDC_READ32(MSDC2_GPIO_MODE15));
		pr_info("MSDC2 MODE16 [0x%p] =0x%8x\tshould: 0x??????22\n",
			MSDC2_GPIO_MODE16, MSDC_READ32(MSDC2_GPIO_MODE16));
		pr_info("MSDC2 IES   [0x%p] =0x%8x\tshould: bit3~5\n",
			MSDC2_GPIO_IES_ADDR, MSDC_READ32(MSDC2_GPIO_IES_ADDR));
		pr_info("MSDC2 SMT   [0x%p] =0x%8x\tshould: bit3~5\n",
			MSDC2_GPIO_SMT_ADDR, MSDC_READ32(MSDC2_GPIO_SMT_ADDR));
		pr_info("MSDC2 TDSEL  [0x%p] =0x%8x\tshould: bit12~23\n",
			MSDC2_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC2_GPIO_TDSEL_ADDR));
		pr_info("MSDC2 RDSEL  [0x%p] =0x%8x\tshould: bit10~27\n",
			MSDC2_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC2_GPIO_RDSEL_ADDR));
		pr_info("MSDC2 DRV    [0x%p] =0x%8x\tshould: bit12~15\n",
			MSDC2_GPIO_DRV_ADDR, MSDC_READ32(MSDC2_GPIO_DRV_ADDR));
		pr_info("MSDC2 PUPD0   [0x%p] =0x%8x\tshould: TBD\n",
			MSDC2_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC2_GPIO_PUPD0_ADDR));
		pr_info("MSDC2 PUPD1   [0x%p] =0x%8x\tshould: TBD\n",
			MSDC2_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC2_GPIO_PUPD1_ADDR));

	}
}

void msdc_set_pin_mode(struct msdc_host *host)
{
	static int chk_pin_sta = -1;

	if (host->id == 0) {
		if (gpio_base && (chk_pin_sta == -1)) {
			MSDC_GET_FIELD(MSDC_HW_TRAPPING_ADDR,
				MSDC_HW_TRAPPING_MASK, host->pin_state);
			pr_info("msdc0: pin state init (%d)\n",
				host->pin_state);
			chk_pin_sta = 1;
		}
		if (chk_pin_sta == 1) {
			if (host->pin_state == 0x0) { /* 0: Aux2, other:Aux1 */
				MSDC_SET_FIELD(MSDC0_GPIO_MODE16,
					0xFFFFFF00, 0x222121);
				MSDC_SET_FIELD(MSDC0_GPIO_MODE17,
					0x00FFFFFF, 0x121122);
			} else {
				MSDC_SET_FIELD(MSDC0_GPIO_MODE16,
					0xFFFFFF00, 0x111111);
				MSDC_SET_FIELD(MSDC0_GPIO_MODE17,
					0x00FFFFFF, 0x111111);
			}
		} else {
			pr_notice("msdc0: error! pin state not init\n");
		}
	} else if (host->id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, 0xFFF00000, 0x111);
		MSDC_SET_FIELD(MSDC1_GPIO_MODE5, 0x00000FFF, 0x111);
	} else if (host->id == 2) {
		MSDC_SET_FIELD(MSDC2_GPIO_MODE15,
			0xFFFF0000, 0x2222);
		MSDC_SET_FIELD(MSDC2_GPIO_MODE16, 0x000000FF, 0x22);
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
		MSDC_SET_FIELD(MSDC2_GPIO_IES_ADDR, MSDC2_IES_ALL_MASK,
			(set_ies ? 0x7 : 0));
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
		MSDC_SET_FIELD(MSDC2_GPIO_SMT_ADDR, MSDC2_SMT_ALL_MASK,
			(set_smt ? 0x7 : 0));
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
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_CLK_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_RSTB_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_DSL_MASK,
			cust_val);
	} else if (id == 1) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL0_ADDR, MSDC1_TDSEL0_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL0_ADDR, MSDC1_TDSEL0_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL0_ADDR, MSDC1_TDSEL0_CLK_MASK,
			cust_val);
	} else if (id == 2) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_CLK_MASK,
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
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CLK_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_RSTB_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_DSL_MASK,
			cust_val);
	} else if (id == 1) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_CLK_MASK,
			cust_val);
	} else if (id == 2) {
		if (flag == MSDC_TDRDSEL_CUST)
			cust_val = value;
		else
			cust_val = 0;
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_CMD_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_DAT_MASK,
			cust_val);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_CLK_MASK,
			cust_val);
	}
}

void msdc_get_tdsel_by_id(u32 id, u32 *value)
{
	if (id == 0) {
		MSDC_GET_FIELD(MSDC0_GPIO_TDSEL0_ADDR, MSDC0_TDSEL0_CMD_MASK,
			*value);
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_TDSEL0_ADDR, MSDC1_TDSEL0_CMD_MASK,
			*value);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_CMD_MASK,
			*value);
	}
}

void msdc_get_rdsel_by_id(u32 id, u32 *value)
{
	if (id == 0) {
		MSDC_GET_FIELD(MSDC0_GPIO_RDSEL0_ADDR, MSDC0_RDSEL0_CMD_MASK,
			*value);
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_CMD_MASK,
			*value);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_CMD_MASK,
			*value);
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst, int ds)
{
	if (id == 0) {
		/* do nothing since 16nm does not have SR control for 1.8V */
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_SR_DAT_MASK,
			(dat != 0));
	} else if (id == 2) {
		/* do nothing since 16nm does not have SR control for 1.8V */
	}
}

void msdc_set_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	if (id == 0) {
		MSDC_SET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_DSL_MASK,
			driving->ds_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_RSTB_MASK,
			driving->rst_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_CMD_MASK,
			driving->cmd_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_CLK_MASK,
			driving->clk_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_DAT_MASK,
			driving->dat_drv);
	} else if (id == 1) {
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_CMD_MASK,
			driving->cmd_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_CLK_MASK,
			driving->clk_drv);
		MSDC_SET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_DAT_MASK,
			driving->dat_drv);
	} else if (id == 2) {
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			driving->cmd_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			driving->clk_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			driving->dat_drv);
	}
}

void msdc_get_driving_by_id(u32 id, struct msdc_hw_driving *driving)
{
	if (id == 0) {
		MSDC_GET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_DSL_MASK,
			driving->ds_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_RSTB_MASK,
			driving->rst_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_CMD_MASK,
			driving->cmd_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_CLK_MASK,
			driving->clk_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV0_ADDR, MSDC0_DRV0_DAT_MASK,
			driving->dat_drv);
	} else if (id == 1) {
		MSDC_GET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_CMD_MASK,
			driving->cmd_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_CLK_MASK,
			driving->clk_drv);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV0_ADDR, MSDC1_DRV0_DAT_MASK,
			driving->dat_drv);
	} else if (id == 2) {
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			driving->dat_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			driving->cmd_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			driving->clk_drv);
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
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x0);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC0_* to 50K ohm PD */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x66666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x6666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC0_CLK to 50K ohm PD,
			 * MSDC0_CMD/MSDC0_DAT* to 10K ohm PU,
			 * MSDC0_DSL to 50K ohm PD
			 */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x11111611);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x1161);
		}
	} else if (id == 1) {
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC1_* to no ohm PU */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR,
				MSDC1_PUPD0_MASK, 0x0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC1_* to 50K ohm PD */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR,
				MSDC1_PUPD0_MASK, 0x66);
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR,
				MSDC1_PUPD0_MASK, 0x666666);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC1_CLK to 50K ohm PD,
			 * MSDC1_CMD/MSDC1_DAT* to 50K ohm PU
			 */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR,
				MSDC1_PUPD0_MASK, 0x222226);
		}
	} else if (id == 2) {
		if (mode == MSDC_PIN_PULL_NONE) {
			/* Switch MSDC2_* to 0 ohm PU
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD0_MASK, 0);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR,
				MSDC2_PUPD1_MASK, 0);
		} else if (mode == MSDC_PIN_PULL_DOWN) {
			/* Switch MSDC2_* to 50K ohm PD
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD0_MASK, 0x6666);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR,
				MSDC2_PUPD1_MASK, 0x6);
		} else if (mode == MSDC_PIN_PULL_UP) {
			/* Switch MSDC2_CLK/DSL to 50K ohm PD,
			 * MSDC2_CMD/MSDC2_DAT* to 10K ohm PU
			 */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD0_MASK, 0x11611);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD1_ADDR,
				MSDC2_PUPD1_MASK, 0x1);
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
		pr_notice("[msdc%d] register_setting is not found in DT\n",
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
	const char *dup_name;

	np = mmc->parent->of_node; /* mmcx node in project dts */

	if (of_property_read_u8(np, "index", &id)) {
		pr_notice("[%s] host index not specified in device tree\n",
			pdev->dev.of_node->name);
		return -1;
	}
	host->id = id;
	pdev->id = id;

	pr_info("DT probe %s%d!\n", pdev->dev.of_node->name, id);

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_notice("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	host->mmc = mmc;
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_notice("[msdc%d] of_iomap failed\n", mmc->index);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_info("[msdc%d] get irq # %d\n", host->id, host->irq);
	WARN_ON(host->irq < 0);

#if !defined(FPGA_PLATFORM)
	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_notice("[msdc%d] error: clk_src isn't found in device tree.\n",
			host->id);
		return -1;
	}
	host->hclk = msdc_get_hclk(host->id, host->hw->clk_src);
#endif

	/* get msdc flag(caps)*/
/*
 *	if (of_find_property(np, "msdc-sys-suspend", &len))
 *		host->hw->flags |= MSDC_SYS_SUSPEND;
 */
	if (of_find_property(np, "sd-uhs-ddr208", &len))
		host->hw->flags |= MSDC_SDIO_DDR208;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	 * -ENODATA if property does not have a value, and -EOVERFLOW if the
	 * property data isn't large enough.
	 */
	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_notice("[msdc%d] host_function isn't found in device tree\n",
			host->id);

	/* get cd_gpio and cd_level */
	ret = of_get_named_gpio(np, "cd-gpios", 0);
	if (ret >= 0) {
		cd_gpio = ret;
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_notice("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	}

	ret = of_property_read_u8(np, "hw_dvfs", &hw_dvfs_support);
	if (ret || (hw_dvfs_support == 0))
		host->use_hw_dvfs = 0;
	else
		host->use_hw_dvfs = 1;

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
		np = of_find_compatible_node(NULL, NULL, "mediatek,msdc0_top");
		host->base_top = of_iomap(np, 0);
	} else if (host->hw->host_function == MSDC_SD)  {
		np = of_find_compatible_node(NULL, NULL, "mediatek,msdc1_top");
		host->base_top = of_iomap(np, 0);
	}
	if (host->base_top)
		pr_debug("of_iomap for MSDC%d TOP base @ 0x%p\n",
			host->id, host->base_top);
#endif

#if defined(CFG_DEV_SDIO)
	if (host->hw->host_function == MSDC_SDIO) {
		host->hw->flags |= MSDC_EXT_SDIO_IRQ;
		host->hw->request_sdio_eirq = mt_sdio_ops[CFG_DEV_SDIO].sdio_request_eirq;
		host->hw->enable_sdio_eirq = mt_sdio_ops[CFG_DEV_SDIO].sdio_enable_eirq;
		host->hw->disable_sdio_eirq = mt_sdio_ops[CFG_DEV_SDIO].sdio_disable_eirq;
		host->hw->register_pm = mt_sdio_ops[CFG_DEV_SDIO].sdio_register_pm;
	}
#endif

	/* fix uaf(use afer free) issue:backup pdev->name,
	 * device_rename will free pdev->name
	 */
	pdev->name = kstrdup(pdev->name, GFP_KERNEL);
	/* device rename */
	if (host->id == 0)
		device_rename(mmc->parent, "bootdevice");
	else if (host->id == 1)
		device_rename(mmc->parent, "externdevice");

	dup_name = pdev->name;
	pdev->name = pdev->dev.kobj.name;
	kfree_const(dup_name);

	return host->id;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	int id;

#ifndef FPGA_PLATFORM
	static char const * const ioconfig_names[] = {
		MSDC0_IOCFG_NAME, MSDC1_IOCFG_NAME,
		MSDC2_IOCFG_NAME
	};
	struct device_node *np;
#endif

	id = msdc_of_parse(pdev, mmc);
	if (id < 0) {
		pr_notice("%s: msdc_of_parse error!!: %d\n", __func__, id);
		return id;
	}

#ifndef FPGA_PLATFORM
	if (gpio_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
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
		np = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		apmixed_base = of_iomap(np, 0);
		pr_debug("of_iomap for apmixed base @ 0x%p\n",
			apmixed_base);
	}

	if (topckgen_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
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
		np = of_find_compatible_node(NULL, NULL, "mediatek,pericfg");
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
	OFFSET_SDC_ADV_CFG0,
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

#ifdef _MTK_HW_FDE_DEBUG
	OFFSET_EMMC52_AES_EN,
	OFFSET_EMMC52_AES_CFG_GP0,
	OFFSET_EMMC52_AES_IV0_GP0,
	OFFSET_EMMC52_AES_IV1_GP0,
	OFFSET_EMMC52_AES_IV2_GP0,
	OFFSET_EMMC52_AES_IV3_GP0,
	OFFSET_EMMC52_AES_CTR0_GP0,
	OFFSET_EMMC52_AES_CTR1_GP0,
	OFFSET_EMMC52_AES_CTR2_GP0,
	OFFSET_EMMC52_AES_CTR3_GP0,
	OFFSET_EMMC52_AES_KEY0_GP0,
	OFFSET_EMMC52_AES_KEY1_GP0,
	OFFSET_EMMC52_AES_KEY2_GP0,
	OFFSET_EMMC52_AES_KEY3_GP0,
	OFFSET_EMMC52_AES_KEY4_GP0,
	OFFSET_EMMC52_AES_KEY5_GP0,
	OFFSET_EMMC52_AES_KEY6_GP0,
	OFFSET_EMMC52_AES_KEY7_GP0,
	OFFSET_EMMC52_AES_TKEY0_GP0,
	OFFSET_EMMC52_AES_TKEY1_GP0,
	OFFSET_EMMC52_AES_TKEY2_GP0,
	OFFSET_EMMC52_AES_TKEY3_GP0,
	OFFSET_EMMC52_AES_TKEY4_GP0,
	OFFSET_EMMC52_AES_TKEY5_GP0,
	OFFSET_EMMC52_AES_TKEY6_GP0,
	OFFSET_EMMC52_AES_TKEY7_GP0,
	OFFSET_EMMC52_AES_SWST,
	OFFSET_EMMC52_AES_CFG_GP1,
	OFFSET_EMMC52_AES_IV0_GP1,
	OFFSET_EMMC52_AES_IV1_GP1,
	OFFSET_EMMC52_AES_IV2_GP1,
	OFFSET_EMMC52_AES_IV3_GP1,
	OFFSET_EMMC52_AES_CTR0_GP1,
	OFFSET_EMMC52_AES_CTR1_GP1,
	OFFSET_EMMC52_AES_CTR2_GP1,
	OFFSET_EMMC52_AES_CTR3_GP1,
	OFFSET_EMMC52_AES_KEY0_GP1,
	OFFSET_EMMC52_AES_KEY1_GP1,
	OFFSET_EMMC52_AES_KEY2_GP1,
	OFFSET_EMMC52_AES_KEY3_GP1,
	OFFSET_EMMC52_AES_KEY4_GP1,
	OFFSET_EMMC52_AES_KEY5_GP1,
	OFFSET_EMMC52_AES_KEY6_GP1,
	OFFSET_EMMC52_AES_KEY7_GP1,
	OFFSET_EMMC52_AES_TKEY0_GP1,
	OFFSET_EMMC52_AES_TKEY1_GP1,
	OFFSET_EMMC52_AES_TKEY2_GP1,
	OFFSET_EMMC52_AES_TKEY3_GP1,
	OFFSET_EMMC52_AES_TKEY4_GP1,
	OFFSET_EMMC52_AES_TKEY5_GP1,
	OFFSET_EMMC52_AES_TKEY6_GP1,
	OFFSET_EMMC52_AES_TKEY7_GP1,
#endif

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
