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
#include <linux/slab.h>
#include "mt_sd.h"
#include <mt-plat/sd_misc.h>
#include "msdc_tune.h"
#include "dbg.h"

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
/* FIX ME: check if it can be removed */
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = "mediatek,mt6755-mmc", },
	{ },
};

unsigned int msdc_host_enable[HOST_MAX_NUM] = {
	1,
	1,
/* FIXME: if msdc2 msdc3 modify device tree standard, msdc_host_enable can be removed */
#if defined(CFG_DEV_MSDC2)
	1,
#else
	0,
#endif
#if defined(CFG_DEV_MSDC3)
	1,
#else
	0,
#endif
};

/* GPIO, IO_CONFIG node */
struct device_node *gpio_node;
struct device_node *msdc0_io_cfg_node;
struct device_node *msdc1_io_cfg_node;
struct device_node *msdc2_io_cfg_node;
struct device_node *msdc3_io_cfg_node;

/* debig node */
struct device_node *infracfg_ao_node;
struct device_node *toprgu_node;
struct device_node *apmixed_node;
struct device_node *topckgen_node;

/* eint node */
struct device_node *eint_node;

struct device_node **msdc_io_cfg_nodes[HOST_MAX_NUM] = {
	&msdc0_io_cfg_node,
	&msdc1_io_cfg_node,
	&msdc2_io_cfg_node,
	&msdc3_io_cfg_node
};

void __iomem *gpio_base;
void __iomem *msdc0_io_cfg_base;
void __iomem *msdc1_io_cfg_base;
void __iomem *msdc2_io_cfg_base;
void __iomem *msdc3_io_cfg_base;

void __iomem *infracfg_ao_reg_base;
void __iomem *infracfg_reg_base;
void __iomem *pericfg_reg_base;
void __iomem *toprgu_reg_base;
void __iomem *apmixed_reg_base;
void __iomem *topckgen_reg_base;

void __iomem **msdc_io_cfg_bases[HOST_MAX_NUM] = {
	&msdc0_io_cfg_base,
	&msdc1_io_cfg_base,
	&msdc2_io_cfg_base,
	&msdc3_io_cfg_base
};


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
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#endif
#include <mt-plat/upmu_common.h>

#ifndef CONFIG_MTK_LEGACY
struct regulator *reg_VEMC;
struct regulator *reg_VMC;
struct regulator *reg_VMCH;

void msdc_get_regulators(struct device *dev)
{
	if (reg_VEMC == NULL)
		reg_VEMC = regulator_get(dev, "vemc");
	if (reg_VMC == NULL)
		reg_VMC = regulator_get(dev, "vmc");
	if (reg_VMCH == NULL)
		reg_VMCH = regulator_get(dev, "vmch");
}

bool msdc_hwPowerOn(unsigned int powerId, int powerVolt, char *mode_name)
{
	struct regulator *reg = NULL;
	int ret;

	if (powerId == POWER_LDO_VMCH)
		reg = reg_VMCH;
	else if (powerId == POWER_LDO_VMC)
		reg = reg_VMC;
	else if (powerId == POWER_LDO_VEMC)
		reg = reg_VEMC;
	if (reg == NULL)
		return false;

	regulator_set_voltage(reg, powerVolt, powerVolt);
	ret = regulator_enable(reg);
	if (ret < 0)
		return false;

	return true;
}

bool msdc_hwPowerDown(unsigned int powerId, char *mode_name)
{
	struct regulator *reg = NULL;
	int ret;

	if (powerId == POWER_LDO_VMCH)
		reg = reg_VMCH;
	else if (powerId == POWER_LDO_VMC)
		reg = reg_VMC;
	else if (powerId == POWER_LDO_VEMC)
		reg = reg_VEMC;

	if (reg == NULL)
		return false;

	ret = regulator_disable(reg);
	if (ret < 0)
		return false;

	return true;
}
#endif

#ifndef CONFIG_MTK_LEGACY
u32 msdc_ldo_power(u32 on, unsigned int powerId, int voltage_mv,
	u32 *status)
#else
u32 msdc_ldo_power(u32 on, MT65XX_POWER powerId, int voltage_mv,
	u32 *status)
#endif
{
	int voltage_uv = voltage_mv * 1000;

	if (on) { /* want to power on */
		if (*status == 0) {  /* can power on */
			pr_warn("msdc LDO<%d> power on<%d>\n", powerId,
				voltage_uv);
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
#ifdef MTK_MSDC_BRINGUP_DEBUG
	u32 ldo_en = 0, ldo_vol = 0;
	u32 id = host->id;

	switch (id) {
	case 0:
		pmic_read_interface_nolock(REG_VEMC_EN, &ldo_en, MASK_VEMC_EN,
			SHIFT_VEMC_EN);
		pmic_read_interface_nolock(REG_VEMC_VOSEL, &ldo_vol,
			MASK_VEMC_VOSEL, SHIFT_VEMC_VOSEL);
		pr_err(" VEMC_EN=0x%x, VEMC_VOL=0x%x [1b'0(3V),1b'1(3.3V)]\n",
			ldo_en, ldo_vol);
		break;
	case 1:
		pmic_read_interface_nolock(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
			SHIFT_VMC_EN);
		pmic_read_interface_nolock(REG_VMC_VOSEL, &ldo_vol,
			MASK_VMC_VOSEL, SHIFT_VMC_VOSEL);
		pr_err(" VMC_EN=0x%x, VMC_VOL=0x%x [3b'101(2.9V),3b'011(1.8V)]\n",
			ldo_en, ldo_vol);
		pmic_read_interface_nolock(REG_VMC_VOSEL_CAL, &ldo_vol,
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
		pr_err(" VMC_CAL=0x%x\n", ldo_vol);

		pmic_read_interface_nolock(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
			SHIFT_VMCH_EN);
		pmic_read_interface_nolock(REG_VMCH_VOSEL, &ldo_vol,
			MASK_VMCH_VOSEL, SHIFT_VMCH_VOSEL);
		pr_err(" VMCH_EN=0x%x, VMCH_VOL=0x%x [1b'0(3V),1b'1(3.3V)]\n",
			ldo_en, ldo_vol);

		pmic_read_interface_nolock(REG_VMCH_VOSEL_CAL, &ldo_vol,
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);
		pr_err(" VMCH_CAL=0x%x\n", ldo_vol);

		break;
	default:
		break;
	}
#endif
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

#if defined(CFG_DEV_MSDC3)
	case 3:
		break;
#endif

	default:
		/* if host_id is 3, it uses default 1.8v setting,
		which always turns on */
		break;
	}

}

void msdc_power_calibration_init(struct msdc_host *host)
{
	u32 val = 0;

	if (MSDC_EMMC == host->hw->host_function) {
		/* Set to 3.0V - 100mV
		 * 4'b0000: 0 mV
		 * 4'b0001: -20 mV
		 * 4'b0010: -40 mV
		 * 4'b0011: -60 mV
		 * 4'b0100: -80 mV
		 * 4'b0101: -100 mV*/
		pmic_config_interface(REG_VEMC_VOSEL_CAL,
			VEMC_VOSEL_CAL_mV(CARD_VOL_ACTUAL - VOL_3000),
			MASK_VEMC_VOSEL_CAL,
			SHIFT_VEMC_VOSEL_CAL);
	} else if (MSDC_SD == host->hw->host_function) {
		/* VMCH vosel is 3.0V and calibration default is 0mV.
		   Use calibration -100mv to get target VMCH = 2.9V */
		pmic_read_interface(REG_VMCH_VOSEL_CAL, &val,
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);
		val -= 5;
		pmic_config_interface(REG_VMCH_VOSEL_CAL, val,
			MASK_VMCH_VOSEL_CAL, SHIFT_VMCH_VOSEL_CAL);

		/* VMC vosel is 2.8V with some calibration value,
		   Add extra 100mV calibration value to get tar VMC = 2.9V */
		pmic_read_interface(REG_VMC_VOSEL_CAL, &val,
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
		val += 5;
		host->vmc_cal_default = val;
		pmic_config_interface(REG_VMC_VOSEL_CAL, val,
			MASK_VMC_VOSEL_CAL, SHIFT_VMC_VOSEL_CAL);
	}
}

void msdc_oc_check(struct msdc_host *host)
{
	u32 val = 0;

	if (host->id == 1) {
		pmic_read_interface(REG_VMCH_OC_STATUS, &val,
			MASK_VMCH_OC_STATUS, SHIFT_VMCH_OC_STATUS);

		if (val) {
			pr_err("msdc1 OC status = %x\n", val);
			host->power_control(host, 0);
		}
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

	msdc_ldo_power(on, POWER_LDO_VEMC, VOL_3000, &g_msdc0_flash);

	msdc_dump_ldo_sts(host);
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

	msdc_dump_ldo_sts(host);
}

void msdc_sd_power_off(void)
{
	struct msdc_host *host = mtk_msdc_host[1];

	if (host) {
		pr_err("Power Off, SD card\n");

		/* power must be on */
		g_msdc1_io = VOL_3000 * 1000;
		g_msdc1_flash = VOL_3000 * 1000;

		host->power_control(host, 0);

		msdc_set_bad_card_and_remove(host);
	}
}
EXPORT_SYMBOL(msdc_sd_power_off);
#endif

#ifdef MSDC_HQA
/*#define HQA_VCORE  0x48*/ /* HVcore, 1.05V */
#define HQA_VCORE  0x40 /* NVcore, 1.0V */
/*#define HQA_VCORE  0x38*/ /* LVcore, 0.95V */

void msdc_HQA_set_vcore(struct msdc_host *host)
{
	if (host->first_tune_done == 0) {
		pmic_config_interface(REG_VCORE_VOSEL_SW, HQA_VCORE,
			VCORE_VOSEL_SW_MASK, 0);
		pmic_config_interface(REG_VCORE_VOSEL_HW, HQA_VCORE,
			VCORE_VOSEL_HW_MASK, 0);
	}
}
#endif


/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/*  msdc clock source
 *  Clock manager will set a reference value in CCF
 *  Normal code doesn't change clock source
 *  Lower frequence will change clock source
 */
/* msdc50_0_hclk reference value is 273M */
/* clock source for host: global */
/* msdc0 clock source reference value is 400M */
u32 hclks_msdc50[] = {26000000 , 400000000, 200000000, 156000000,
		      182000000, 156000000, 100000000, 624000000,
		      312000000};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc30[] = {26000000 , 208000000, 100000000, 156000000,
		      182000000, 156000000, 178000000, 200000000};

u32 hclks_msdc30_3[] = {26000000 , 50000000, 100000000, 156000000,
		      48000000, 156000000, 178000000, 54000000,
		      25000000};

u32 *hclks_msdc = hclks_msdc30;

/* FIX ME: check if the following 10 lines can be removed */
#include <dt-bindings/clock/mt6755-clk.h>
/*#include <mach/mt_clk_id.h>*/
#include "mt_clk_id.h" /* workaround before mt_clk_id.d placed into correct path */
/*#include <mach/mt_idle.h>*/

int msdc_cg_clk_id[HOST_MAX_NUM] = {
	MT_CG_INFRA0_MSDC0_CG_STA,
	MT_CG_INFRA0_MSDC1_CG_STA,
	MT_CG_INFRA0_MSDC2_CG_STA,
	MT_CG_INFRA0_MSDC3_CG_STA
};

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	if (pdev->id == 0)
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC0-CLOCK");
	else if (pdev->id == 1)
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC1-CLOCK");
	else if (pdev->id == 2)
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC2-CLOCK");
	else if (pdev->id == 3)
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC3-CLOCK");

	if (IS_ERR(host->clock_control)) {
		pr_err("can not get msdc%d clock control\n", pdev->id);
		return 1;
	}
	if (clk_prepare(host->clock_control)) {
		pr_err("can not prepare msdc%d clock control\n", pdev->id);
		return 1;
	}

	return 0;
}

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	u32 *hclks;

	hclks = msdc_get_hclks(host->id);

	pr_err("[%s]: msdc%d select clk_src as %d(%dKHz)\n", __func__,
		host->id, clksrc, (hclks[clksrc]/1000));

	host->hclk = hclks[clksrc];
	host->hw->clk_src = clksrc;
}

void msdc_dump_clock_sts(void)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
	void __iomem *reg;

	if (apmixed_reg_base && topckgen_reg_base && infracfg_ao_reg_base) {
		reg = apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET;
		pr_err(" MSDCPLL_PWR_CON0[0x%p][bit0~1 should be 2b'01]=0x%x\n",
			reg, MSDC_READ32(reg));
		reg = apmixed_reg_base + MSDCPLL_CON0_OFFSET;
		pr_err(" MSDCPLL_CON0    [0x%p][bit0 should be 1b'1]=0x%x\n",
			reg, MSDC_READ32(reg));

		reg = topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET;
		pr_err(" CLK_CFG_3       [0x%p][bit[19:16]should be 0x1]=0x%x\n",
			reg, MSDC_READ32(reg));
		pr_err(" CLK_CFG_3       [0x%p][bit[27:24]should be 0x7]=0x%x\n",
			reg, MSDC_READ32(reg));
		reg = topckgen_reg_base + MSDC_CLK_CFG_4_OFFSET;
		pr_err(" CLK_CFG_4       [0x%p][bit[3:0]should be 0x7]=0x%x\n",
			reg, MSDC_READ32(reg));

		reg = infracfg_ao_reg_base + MSDC_INFRA_PDN_STA1_OFFSET;
		pr_err(" MODULE_SW_CG_1  [0x%p][bit2=msdc0, bit4=msdc1, bit5=msdc2, 0:on,1:off]=0x%x\n",
			reg, MSDC_READ32(reg));
	} else {
		pr_err(" apmixed_reg_base = %p, topckgen_reg_base = %p, clk_infra_base = %p\n",
			apmixed_reg_base, topckgen_reg_base, infracfg_ao_reg_base);
	}
#endif
}

/* FIX ME: check if this function can be removed */
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
#endif

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
void msdc_dump_padctl_by_id(u32 id)
{
	switch (id) {
	case 0:
		pr_err("MSDC0 MODE18[0x%p] =0x%8x\tshould:0x12-- ----\n",
			MSDC0_GPIO_MODE18, MSDC_READ32(MSDC0_GPIO_MODE18));
		pr_err("MSDC0 MODE19[0x%p] =0x%8x\tshould:0x1249 1249\n",
			MSDC0_GPIO_MODE19, MSDC_READ32(MSDC0_GPIO_MODE19));
		pr_err("MSDC0 IES   [0x%p] =0x%8x\tshould:0x---- --1F\n",
			MSDC0_GPIO_IES_ADDR, MSDC_READ32(MSDC0_GPIO_IES_ADDR));
		pr_err("MSDC0 SMT   [0x%p] =0x%8x\tshould:0x---- --1F\n",
			MSDC0_GPIO_SMT_ADDR, MSDC_READ32(MSDC0_GPIO_SMT_ADDR));
		pr_err("MSDC0 TDSEL [0x%p] =0x%8x\tshould:0x---0 0000\n",
			MSDC0_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_TDSEL_ADDR));
		pr_err("MSDC0 RDSEL [0x%p] =0x%8x\tshould:0x0000 0000\n",
			MSDC0_GPIO_RDSEL_ADDR,
			MSDC_READ32(MSDC0_GPIO_RDSEL_ADDR));
		pr_err("MSDC0 SR    [0x%p] =0x%8x\n",
			MSDC0_GPIO_SR_ADDR,
			MSDC_READ32(MSDC0_GPIO_SR_ADDR));
		pr_err("MSDC0 DRV   [0x%p] =0x%8x\n",
			MSDC0_GPIO_DRV_ADDR,
			MSDC_READ32(MSDC0_GPIO_DRV_ADDR));
		pr_err("MSDC0 PULL0 [0x%p] =0x%8x\n",
			MSDC0_GPIO_PUPD0_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD0_ADDR));
		pr_err("P-NONE: 0x4444 4444, PU:0x2111 1161, PD:0x6666 6666\n");
		pr_err("MSDC0 PULL1 [0x%p] =0x%8x\n",
			MSDC0_GPIO_PUPD1_ADDR,
			MSDC_READ32(MSDC0_GPIO_PUPD1_ADDR));
		pr_err("P-NONE: 0x---- 4444, PU:0x---- 6111, PD:0x---- 6666\n");
		break;
	case 1:
		pr_err("MSDC1 MODE4 [0x%p] =0x%8x\tshould:0x---1 1249\n",
			MSDC1_GPIO_MODE4, MSDC_READ32(MSDC1_GPIO_MODE4));
		pr_err("MSDC1 IES   [0x%p] =0x%8x\tshould:0x----   -7--\n",
			MSDC1_GPIO_IES_ADDR, MSDC_READ32(MSDC1_GPIO_IES_ADDR));
		pr_err("MSDC1 SMT   [0x%p] =0x%8x\tshould:0x----   -7--\n",
			MSDC1_GPIO_SMT_ADDR, MSDC_READ32(MSDC1_GPIO_SMT_ADDR));
		pr_err("MSDC1 TDSEL [0x%p] =0x%8x\n",
			MSDC1_GPIO_TDSEL_ADDR,
			MSDC_READ32(MSDC1_GPIO_TDSEL_ADDR));
		/* FIX ME: Check why sleep shall set as F */
		pr_err("should 1.8v: sleep:0x---- -FFF, awake:0x---- -AAA\n");
		pr_err("MSDC1 RDSEL0[0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL0_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL0_ADDR));
		pr_err("1.8V: 0x-000 ----, 3.3v: 0x-30C ----\n");
		pr_err("MSDC1 RDSEL1[0x%p] =0x%8x\n",
			MSDC1_GPIO_RDSEL1_ADDR,
			MSDC_READ32(MSDC1_GPIO_RDSEL1_ADDR));
		pr_err("should 1.8V: 0x-000 ----, 3.3v: 0x---- ---C\n");
		pr_err("MSDC1 SR    [0x%p] =0x%8x\n",
			MSDC1_GPIO_SR_ADDR, MSDC_READ32(MSDC1_GPIO_SR_ADDR));
		pr_err("MSDC1 DRV   [0x%p] =0x%8x\n",
			MSDC1_GPIO_DRV_ADDR, MSDC_READ32(MSDC1_GPIO_DRV_ADDR));
		pr_err("MSDC1 PULL  [0x%p] =0x%8x\n",
			MSDC1_GPIO_PUPD_ADDR,
			MSDC_READ32(MSDC1_GPIO_PUPD_ADDR));
		pr_err("P-NONE: 0x--44 4444, PU:0x--22 2262, PD:0x--66 6666\n");
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		pr_err("MSDC2 MODE14[0x%p][21:3]  =0x%8x\tshould:0x--0x12 2490\n",
			MSDC2_GPIO_MODE14, MSDC_READ32(MSDC2_GPIO_MODE14));
		pr_err("MSDC2 IES   [0x%p][5:3]   =0x%8x\tshould:0x---- -38-\n",
			MSDC2_GPIO_IES_ADDR, MSDC_READ32(MSDC2_GPIO_IES_ADDR));
		pr_err("MSDC2 SMT   [0x%p][5:3]   =0x%8x\tshould:0x---- -38-\n",
			MSDC2_GPIO_SMT_ADDR, MSDC_READ32(MSDC2_GPIO_SMT_ADDR));
		pr_err("MSDC2 TDSEL [0x%p][23:12] =0x%8xtshould:0x--0x--00 0---\n",
			MSDC2_GPIO_TDSEL_ADDR, MSDC_READ32(MSDC2_GPIO_TDSEL_ADDR));
		pr_err("MSDC2 RDSEL0[0x%p][11:6]  =0x%8xtshould:0x--0x---- -00-\n",
			MSDC2_GPIO_RDSEL_ADDR, MSDC_READ32(MSDC2_GPIO_RDSEL_ADDR));
		pr_err("MSDC2 SR DRV[0x%p][23:12] =0x%8x\n",
			MSDC2_GPIO_SR_ADDR, MSDC_READ32(MSDC2_GPIO_SR_ADDR));
		pr_err("MSDC2 PULL  [0x%p][31:12] =0x%8x\n",
			MSDC2_GPIO_PUPD_ADDR0, MSDC_READ32(MSDC2_GPIO_PUPD_ADDR0));
		pr_err("P-NONE: 0x--44444, PU:0x--11611, PD:0x--66666\n");
		pr_err("MSDC2 PULL  [0x%p][2:0] =0x%8x\n",
			MSDC2_GPIO_PUPD_ADDR1, MSDC_READ32(MSDC2_GPIO_PUPD_ADDR1));
		pr_err("P-NONE: 0x--4, PU:0x--1, PD:0x--6\n");
		break;
#endif
	}
}

void msdc_set_pin_mode(struct msdc_host *host)
{
	switch (host->id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_MODE18, (0x3F << 25), 0x9);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE19, 0xFFFFFFFF, 0x12491249);
		break;

	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, 0x0007FFFF, 0x0011249);
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_MODE14, 0x003FFFF8, 0x24492);
		break;

#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
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

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_IES_ADDR, MSDC2_IES_ALL_MASK,
			(set_ies ? 0x7 : 0));
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
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

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_SMT_ADDR, MSDC2_SMT_ALL_MASK,
			(set_smt ? 0x7 : 0));
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}
}

void msdc_set_tdsel_by_id(u32 id, bool sleep, bool sd_18)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR, MSDC0_TDSEL_ALL_MASK , 0);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR, MSDC1_TDSEL_ALL_MASK,
			(sleep ? 0xFFF : 0xAAA));
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK , 0);
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
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
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR, MSDC1_RDSEL0_ALL_MASK,
			(sd_18 ? 0 : 0x30C));
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL1_ADDR, MSDC1_RDSEL1_ALL_MASK,
			(sd_18 ? 0 : 0xC));
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR, MSDC2_RDSEL_ALL_MASK, 0);
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}
}

void msdc_set_tdsel_dbg_by_id(u32 id, u32 value)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_DSL_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_DAT_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_RSTB_MASK, value);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR,
			MSDC1_TDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR,
			MSDC1_TDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC1_GPIO_TDSEL_ADDR,
			MSDC1_TDSEL_DAT_MASK, value);
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR,
			MSDC2_TDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR,
			MSDC2_TDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR,
			MSDC2_TDSEL_DAT_MASK, value);
		break;
#endif
	}
}

void msdc_set_rdsel_dbg_by_id(u32 id, u32 value)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_DSL_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_DAT_MASK, value);
		MSDC_SET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_RSTB_MASK, value);
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR,
			MSDC1_RDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL0_ADDR,
			MSDC1_RDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC1_GPIO_RDSEL1_ADDR,
			MSDC1_RDSEL_DAT_MASK, value);
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR,
			MSDC2_RDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR,
			MSDC2_RDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL_ADDR,
			MSDC2_RDSEL_DAT_MASK, value);
		break;
#endif
	}
}

void msdc_get_tdsel_dbg_by_id(u32 id, u32 *value)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_TDSEL_ADDR,
			MSDC0_TDSEL_CMD_MASK, *value);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_TDSEL_ADDR,
			MSDC1_TDSEL_CMD_MASK, *value);
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_TDSEL_ADDR,
			MSDC2_TDSEL_CMD_MASK, *value);
		break;
#endif
	}
}

void msdc_get_rdsel_dbg_by_id(u32 id, u32 *value)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_RDSEL_ADDR,
			MSDC0_RDSEL_CMD_MASK, *value);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_RDSEL0_ADDR,
			MSDC1_RDSEL_CMD_MASK, *value);
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_RDSEL_ADDR,
			MSDC2_RDSEL_CMD_MASK, *value);
		break;
#endif
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst, int ds)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_DSL_MASK,
			(ds != 0));
		MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_DAT_MASK,
			(dat != 0));
		MSDC_SET_FIELD(MSDC0_GPIO_SR_ADDR, MSDC0_SR_RSTB_MASK,
			(rst != 0));
		break;
	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC1_GPIO_SR_ADDR, MSDC1_SR_DAT_MASK,
			(dat != 0));
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_SR_ADDR, MSDC2_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC2_GPIO_SR_ADDR, MSDC2_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC2_GPIO_SR_ADDR, MSDC2_SR_DAT_MASK,
			(dat != 0));
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}

}

void msdc_set_driving_by_id(u32 id, struct msdc_hw *hw, bool sd_18)
{
	switch (id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			hw->ds_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			hw->dat_drv);
		MSDC_SET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			hw->rst_drv);
		break;
	case 1:
		if (sd_18) {
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv_sd_18);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv_sd_18);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv_sd_18);
		} else {
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv);
			MSDC_SET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv);
		}
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			hw->dat_drv);
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
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
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			hw->ds_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			hw->dat_drv);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			hw->rst_drv);
		break;
	case 1:
		if (g_msdc1_io == 1800000) {
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv_sd_18);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv_sd_18);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv_sd_18);
		} else {
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
				hw->cmd_drv);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
				hw->clk_drv);
			MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
				hw->dat_drv);
		}
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			hw->dat_drv);
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}
}

void msdc_pin_config_by_id(u32 id, u32 mode)
{
	switch (id) {
	case 0:
		/*Attention: don't pull CLK high; Don't toggle RST to prevent
		  from entering boot mode */
		if (MSDC_PIN_PULL_NONE == mode) {
			/* high-Z */
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x4444444);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x4444);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* cmd/clk/dat/(rstb)/dsl:pd-50k*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x6666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x6666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* clk/dsl:pd-50k, cmd/dat:pu-10k, (rstb:pu-50k)*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x1111161);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x6111);
		}
		break;
	case 1:
		if (MSDC_PIN_PULL_NONE == mode) {
			/* high-Z */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				MSDC1_PUPD_MASK, 0x444444);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* cmd/clk/dat:pd-50k */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				MSDC1_PUPD_MASK, 0x666666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* cmd/dat:pu-50k, clk:pd-50k */
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD_ADDR,
				MSDC1_PUPD_MASK, 0x222262);
		}
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		if (MSDC_PIN_PULL_NONE == mode) {
			/* high-Z */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR0,
				MSDC2_PUPD_MASK0, 0x44444);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR1,
				MSDC2_PUPD_MASK1, 0x4);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* cmd/clk/dat:pd-50k */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR0,
				MSDC2_PUPD_MASK0, 0x66666);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR1,
				MSDC2_PUPD_MASK1, 0x6);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* cmd/dat:pu-10k, clk:pd-50k, */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR0,
				MSDC2_PUPD_MASK0, 0x11611);
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD_ADDR1,
				MSDC2_PUPD_MASK1, 0x1);
		}
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}

}
#endif /*if !defined(FPGA_PLATFORM)*/

/**************************************************************/
/* Section 5: Device Tree Init function                       */
/*            This function is placed here so that all        */
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

	/* parse hw property settings */
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
	int len;

	if (!mmc->parent || !mmc->parent->of_node)
		return 1;

	np = mmc->parent->of_node; /* mmcx node in projectdts */
	host->mmc = mmc;  /* msdc_check_init_done() need */
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_err("[msdc%d] of_iomap failed\n", host->id);
		return -ENOMEM;
	}

	/* get irq # */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_err("[msdc%d] get irq # %d\n", mmc->index, host->irq);
	BUG_ON(host->irq < 0);

	/* get clk_src */
	#if !defined(FPGA_PLATFORM)
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

	/* get cd_level */
	if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
		pr_err("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	/* get cd_gpio */
	of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio);

	msdc_get_rigister_settings(host);

	#if !defined(FPGA_PLATFORM)
	msdc_get_pinctl_settings(host);
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
	return 0;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc,
		unsigned int *cd_irq)
{
#if !defined(FPGA_PLATFORM) && !defined(CONFIG_MTK_LEGACY)
	struct device_node *msdc_backup_node;
#endif
	struct msdc_host *host = mmc_priv(mmc);
	unsigned int host_id = 0;
	int i, ret;
	static char const * const msdc_names[] = {
		"msdc0", "msdc1", "msdc2", "msdc3"};
	static char const * const ioconfig_names[] = {
		"mediatek,iocfg_5", "mediatek,iocfg_0",
		"mediatek,iocfg_4", "NOT EXIST"
	};

	for (i = 0; i < HOST_MAX_NUM; i++) {
		if (0 == strcmp(pdev->dev.of_node->name, msdc_names[i])) {
			if (msdc_host_enable[i] == 0)
				return 1;
			pdev->id = i;
			host_id = i;
			break;
		}
	}

	if (i == HOST_MAX_NUM) {
		pr_err("%s: Can not find msdc host\n", __func__);
		return 1;
	}

	pr_err("msdc%d of msdc DT probe %s!\n",
		host_id, pdev->dev.of_node->name);

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_err("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	host = mmc_priv(mmc);
	host->id = host_id;

	ret = msdc_of_parse(mmc);
	if (ret) {
		pr_err("%s: msdc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	if (gpio_node == NULL) {
		gpio_node = of_find_compatible_node(NULL, NULL,
			"mediatek,GPIO");
		gpio_base = of_iomap(gpio_node, 0);
		pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
	}

	if (*msdc_io_cfg_nodes[host_id] == NULL) {
		*msdc_io_cfg_nodes[host_id] = of_find_compatible_node(
			NULL, NULL, ioconfig_names[host_id]);
		*msdc_io_cfg_bases[host_id] =
			of_iomap(*msdc_io_cfg_nodes[host_id], 0);
		pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
			host_id, *msdc_io_cfg_bases[host_id]);
	}

#ifndef FPGA_PLATFORM
	/*FIX ME, correct node match string */
	if (infracfg_ao_node == NULL) {
		infracfg_ao_node = of_find_compatible_node(NULL, NULL,
			"mediatek,infracfg_ao");
		infracfg_ao_reg_base = of_iomap(infracfg_ao_node, 0);
		pr_debug("of_iomap for infracfg_ao base @ 0x%p\n",
			infracfg_ao_reg_base);
	}

	if (toprgu_node == NULL) {
		toprgu_node = of_find_compatible_node(NULL, NULL,
			"mediatek,toprgu");
		toprgu_reg_base = of_iomap(toprgu_node, 0);
		pr_debug("of_iomap for toprgu base @ 0x%p\n",
			toprgu_reg_base);
	}

	if (apmixed_node == NULL) {
		apmixed_node = of_find_compatible_node(NULL, NULL,
			"mediatek,apmixed");
		apmixed_reg_base = of_iomap(apmixed_node, 0);
		pr_err("of_iomap for APMIXED base @ 0x%p\n",
			apmixed_reg_base);
	}

	if (topckgen_node == NULL) {
		topckgen_node = of_find_compatible_node(NULL, NULL,
			"mediatek,mt6755-topckgen");
		if (!topckgen_node) {
			topckgen_node = of_find_compatible_node(NULL, NULL,
			"mediatek,topckgen");
		}
		topckgen_reg_base = of_iomap(topckgen_node, 0);
		pr_err("of_iomap for TOPCKGEN base @ 0x%p\n",
			topckgen_reg_base);
	}

#ifndef CONFIG_MTK_LEGACY
	/* backup original dev.of_node */
	msdc_backup_node = pdev->dev.of_node;
	/* get regulator supply node */
	pdev->dev.of_node = of_find_compatible_node(NULL, NULL,
		"mediatek,mt_pmic_regulator_supply");
	msdc_get_regulators(&(pdev->dev));
	/* restore original dev.of_node */
	pdev->dev.of_node = msdc_backup_node;
#endif

	/* FIXME
	if (host_id == 1)
		pmic_ldo_oc_int_register();
	*/

#endif

#ifdef FPGA_PLATFORM
	msdc_fpga_pwr_init(&(pdev->dev));
#endif

	return 0;
}

