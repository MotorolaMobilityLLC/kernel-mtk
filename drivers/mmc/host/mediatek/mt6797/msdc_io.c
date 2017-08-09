#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include "mt_sd.h"
#include "msdc_tune.h"
#include "board.h"
#include <linux/delay.h>

/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#ifndef CONFIG_MTK_LEGACY
#include <linux/regulator/consumer.h>
#endif

const struct of_device_id msdc_of_ids[] = {
	{   .compatible = "mediatek,MSDC0", },
	{   .compatible = "mediatek,MSDC1", },
	{   .compatible = "mediatek,MSDC2", },
	{   .compatible = "mediatek,MSDC3", },
	{ },
};

unsigned int msdc_host_enable[HOST_MAX_NUM] = {
#if defined(CFG_DEV_MSDC0)
	1,
#else
	0,
#endif
#if defined(CFG_DEV_MSDC1)
	1,
#else
	0,
#endif
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

struct device_node *gpio_node;
struct device_node *msdc0_io_cfg_node;
struct device_node *msdc1_io_cfg_node;
struct device_node *msdc2_io_cfg_node;
struct device_node *msdc3_io_cfg_node;

struct device_node *infracfg_ao_node;
struct device_node *infracfg_node;
struct device_node *pericfg_node;
struct device_node *emi_node;
struct device_node *toprgu_node;
struct device_node *apmixed_node;
struct device_node *topckgen_node;
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
void __iomem *emi_reg_base;
void __iomem *toprgu_reg_base;
void __iomem *apmixed_reg_base;
void __iomem *topckgen_reg_base;

void __iomem **msdc_io_cfg_bases[HOST_MAX_NUM] = {
	&msdc0_io_cfg_base,
	&msdc1_io_cfg_base,
	&msdc2_io_cfg_base,
	&msdc3_io_cfg_base
};

#if !defined(FPGA_PLATFORM) && !defined(CONFIG_MTK_LEGACY)
struct regulator *reg_VEMC;
struct regulator *reg_VMC;
struct regulator *reg_VMCH;
#endif

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

	/*parse pinctl settings*/
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

/*****************************************************************************/
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

/*****************************************************************************/
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

/*****************************************************************************/
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
		pr_err("[MSDC%d] register_setting is not found in DT.\n",
				host->id);
		return 1;
	}
	/* parse ett */
	if (of_property_read_u32(register_setting_node, "ett-hs200-cells",
				&host->hw->ett_hs200_count))
		pr_err("[MSDC] ett-hs200-cells is not found in DT.\n");
	host->hw->ett_hs200_settings =
		kzalloc(sizeof(struct msdc_ett_settings) *
				host->hw->ett_hs200_count, GFP_KERNEL);

	if (MSDC_EMMC == host->hw->host_function &&
		!of_property_read_u32_array(register_setting_node,
		"ett-hs200-customer", host->hw->ett_hs200_settings,
		host->hw->ett_hs200_count * 3)) {
		pr_err("[MSDC%d] hs200 ett for customer is found in DT.\n",
				host->id);
	} else if (MSDC_EMMC == host->hw->host_function
		&& !of_property_read_u32_array(register_setting_node,
		"ett-hs200-default", host->hw->ett_hs200_settings,
		host->hw->ett_hs200_count * 3)) {
		pr_err("[MSDC%d] hs200 ett for default is found in DT.\n",
				host->id);
	} else if (MSDC_EMMC == host->hw->host_function) {
		pr_err("[MSDC%d]error: hs200 ett is not found in DT.\n",
				host->id);
	}

	if (of_property_read_u32(register_setting_node, "ett-hs400-cells",
				&host->hw->ett_hs400_count))
		pr_err("[MSDC] ett-hs400-cells is not found in DT.\n");
	host->hw->ett_hs400_settings =
		kzalloc(sizeof(struct msdc_ett_settings) *
				host->hw->ett_hs400_count, GFP_KERNEL);

	if (MSDC_EMMC == host->hw->host_function &&
		!of_property_read_u32_array(register_setting_node,
		"ett-hs400-customer", host->hw->ett_hs400_settings,
		host->hw->ett_hs400_count * 3)) {
		pr_err("[MSDC%d] hs400 ett for customer is found in DT.\n",
				host->id);
	} else if (MSDC_EMMC == host->hw->host_function &&
		!of_property_read_u32_array(register_setting_node,
		"ett-hs400-default", host->hw->ett_hs400_settings,
		host->hw->ett_hs400_count * 3)) {
		pr_err("[MSDC%d] hs400 ett for default is found in DT.\n",
				host->id);
	} else if (MSDC_EMMC == host->hw->host_function) {
		pr_err("[MSDC%d]error: hs400 ett is not found in DT.\n",
				host->id);
	}
}

/**
 *	msdc_of_parse() - parse host's device-tree node
 *	@host: host whose node should be parsed.
 *
 */
int msdc_of_parse(struct mmc_host *mmc)
{
	struct device_node *np;
	struct msdc_host *host = mmc_priv(mmc);
	int ret, len, debug = 0;

	if (!mmc->parent || !mmc->parent->of_node)
		return 1;

	np = mmc->parent->of_node;
	host->mmc = mmc;  /* msdc_check_init_done() need */
	host->hw = kzalloc(sizeof(struct msdc_hw), GFP_KERNEL);

	/*basic settings*/
	if (0 == strcmp(np->name, "MSDC0"))
		host->id = 0;
	else if (0 == strcmp(np->name, "MSDC1"))
		host->id = 1;
	else if (0 == strcmp(np->name, "MSDC2"))
		host->id = 2;
	else
		host->id = 3;

	/* iomap register */
	host->base = of_iomap(np, 0);
	if (!host->base) {
		pr_err("can't of_iomap for msdc!!\n");
		return -ENOMEM;
	}
	pr_err("of_iomap for msdc @ 0x%p\n", host->base);

	/* get irq #  */
	host->irq = irq_of_parse_and_map(np, 0);
	pr_err("msdc%d get irq # %d\n", host->id, host->irq);
	BUG_ON(host->irq < 0);

	/* get clk_src */
	if (of_property_read_u8(np, "clk_src", &host->hw->clk_src)) {
		pr_err("[msdc%d] error: clk_src isn't found in DT.\n",
				host->id);
		BUG_ON(1);
	}

	/* get msdc flag(caps)*/
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	* -ENODATA if property does not have a value, and -EOVERFLOW if the
	* property data isn't large enough.*/

	if (of_property_read_u8(np, "host_function", &host->hw->host_function))
		pr_err("[msdc%d] host_function isn't found in DT\n", host->id);

	if (of_find_property(np, "bootable", &len))
		host->hw->boot = 1;

	/*get cd_level*/
	if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
		pr_err("[msdc%d] cd_level isn't found in DT\n", host->id);

	msdc_get_rigister_settings(host);
	msdc_get_pinctl_settings(host);

	return 0;

out:
	return ret;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc,
		unsigned int *cd_irq)
{
#if !defined(FPGA_PLATFORM) && !defined(CONFIG_MTK_LEGACY)
	struct device_node *msdc_backup_node;
#endif
	unsigned int host_id = 0;
	int i, ret;
	struct msdc_host *host = mmc_priv(mmc);
	static const char const *msdc_names[] = {"MSDC0", "MSDC1", "MSDC2", "MSDC3"};
	static const char const *ioconfig_names[] = {
		"mediatek,IOCFG_5", "mediatek,IOCFG_0",
		"mediatek,IOCFG_1", "mediatek,IOCFG_2"
	};

	for (i = 0; i < HOST_MAX_NUM; i++) {
		if (0 == strcmp(pdev->dev.of_node->name, msdc_names[i])) {
			if (msdc_host_enable[i] == 0)
				return 1;
			/* FIXME: msdc0 msdc1 prase parameter from device tree
			 * msdc2 msdc3 should be modified later
			 */
			if (i == 2) {
#ifdef CFG_DEV_MSDC2
				host->hw = &msdc2_hw;
				pr_err("platform_data hw:0x%p, is msdc2_hw\n",
					host->hw);
#endif
			} else if (i == 3) {
#ifdef CFG_DEV_MSDC3
				host->hw = &msdc3_hw;
				pr_err("platform_data hw:0x%p, is msdc3_hw\n",
					host->hw);
#endif
			}
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
		pr_err("%s: mmc of parse error!!\n", __func__);
		return ret;
	}
	ret = msdc_of_parse(mmc);
	if (ret) {
		pr_err("%s: msdc of parse error!!\n", __func__);
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
			"mediatek,INFRACFG_AO");
		infracfg_ao_reg_base = of_iomap(infracfg_ao_node, 0);
		pr_debug("of_iomap for infracfg_ao base @ 0x%p\n",
			infracfg_ao_reg_base);
	}

	if (infracfg_node == NULL) {
		infracfg_node = of_find_compatible_node(NULL, NULL,
			"mediatek,INFRACFG");
		infracfg_reg_base = of_iomap(infracfg_node, 0);
		pr_debug("of_iomap for infracfg base @ 0x%p\n",
			infracfg_reg_base);
	}

	if (pericfg_node == NULL) {
		pericfg_node = of_find_compatible_node(NULL, NULL,
			"mediatek,PERICFG");
		pericfg_reg_base = of_iomap(pericfg_node, 0);
		pr_debug("of_iomap for pericfg base @ 0x%p\n",
			pericfg_reg_base);
	}

	if (emi_node == NULL) {
		emi_node = of_find_compatible_node(NULL, NULL,
			"mediatek,EMI");
		emi_reg_base = of_iomap(emi_node, 0);
		pr_debug("of_iomap for emi base @ 0x%p\n",
			emi_reg_base);
	}

	if (toprgu_node == NULL) {
		toprgu_node = of_find_compatible_node(NULL, NULL,
			"mediatek,TOPRGU");
		toprgu_reg_base = of_iomap(toprgu_node, 0);
		pr_debug("of_iomap for toprgu base @ 0x%p\n",
			toprgu_reg_base);
	}

	if (apmixed_node == NULL) {
		apmixed_node = of_find_compatible_node(NULL, NULL,
			"mediatek,APMIXED");
		apmixed_reg_base = of_iomap(apmixed_node, 0);
		pr_debug("of_iomap for APMIXED base @ 0x%p\n",
			apmixed_reg_base);
	}

	if (topckgen_node == NULL) {
		topckgen_node = of_find_compatible_node(NULL, NULL,
			"mediatek,TOPCKGEN");
		topckgen_reg_base = of_iomap(topckgen_node, 0);
		pr_debug("of_iomap for TOPCKGEN base @ 0x%p\n",
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
#endif
	/* Get SD card detect eint irq */
	if ((host->hw->host_function == MSDC_SD) && (eint_node == NULL)) {
		eint_node = of_find_compatible_node(NULL, NULL,
			"mediatek, MSDC1_INS-eint");
		if (eint_node) {
			*cd_irq = irq_of_parse_and_map(eint_node, 0);
			if (*cd_irq == 0)
				pr_err("Parse map for sd detect eint fail\n");
			else
				pr_err("msdc1 eint get irq # %d\n", *cd_irq);
		} else {
			pr_err("can't find MSDC1_INS-eint compatible node\n");
		}
	}

	return 0;
}

u32 g_msdc0_io     = 0;
u32 g_msdc0_flash  = 0;
u32 g_msdc1_io     = 0;
u32 g_msdc1_flash  = 0;
u32 g_msdc2_io     = 0;
u32 g_msdc2_flash  = 0;
u32 g_msdc3_io     = 0;
u32 g_msdc3_flash  = 0;
/*
u32 g_msdc4_io     = 0;
u32 g_msdc4_flash  = 0;
*/
#if !defined(FPGA_PLATFORM)
/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#endif
#include <mach/upmu_common.h>
/* FIXME: change upmu_set_rg_vmc_184() to generic API and remove extern */
/* extern void upmu_set_rg_vmc_184(kal_uint8 x); */
/*workaround for VMC 1.8v -> 1.84v*/
/* extern bool hwPowerSetVoltage(unsigned int powerId,
	int powerVOlt, char *mode_name); */
#ifndef CONFIG_MTK_LEGACY
struct regulator *reg_VEMC;
struct regulator *reg_VMC;
struct regulator *reg_VMCH;

enum MSDC_LDO_POWER {
	POWER_LDO_VMCH,
	POWER_LDO_VMC,
	POWER_LDO_VEMC,
};

void msdc_get_regulators(struct device *dev)
{
	if (reg_VEMC == NULL)
		reg_VEMC = regulator_get(dev, "VEMC");
	if (reg_VMC == NULL)
		reg_VMC = regulator_get(dev, "VMC");
	if (reg_VMCH == NULL)
		reg_VMCH = regulator_get(dev, "VMCH");
}

bool msdc_hwPowerOn(unsigned int powerId, int powerVolt, char *mode_name)
{
	struct regulator *reg = NULL;

	if (powerId == POWER_LDO_VMCH)
		reg = reg_VMCH;
	else if (powerId == POWER_LDO_VMC)
		reg = reg_VMC;
	else if (powerId == POWER_LDO_VEMC)
		reg = reg_VEMC;
	if (reg == NULL)
		return FALSE;

	/* New API voltage use micro V */
	regulator_set_voltage(reg, powerVolt, powerVolt);
	regulator_enable(reg);
	pr_err("msdc_hwPoweron:%d: name:%s", powerId, mode_name);
	return TRUE;
}

bool msdc_hwPowerDown(unsigned int powerId, char *mode_name)
{
	struct regulator *reg = NULL;

	if (powerId == POWER_LDO_VMCH)
		reg = reg_VMCH;
	else if (powerId == POWER_LDO_VMC)
		reg = reg_VMC;
	else if (powerId == POWER_LDO_VEMC)
		reg = reg_VEMC;

	if (reg == NULL)
		return FALSE;

	/* New API voltage use micro V */
	regulator_disable(reg);
	pr_err("msdc_hwPowerOff:%d: name:%s", powerId, mode_name);

	return TRUE;
}

#else /*for CONFIG_MTK_LEGACY defined*/
#define	POWER_LDO_VMCH		MT6351_POWER_LDO_VMCH
#define	POWER_LDO_VMC		MT6351_POWER_LDO_VMC
#define	POWER_LDO_VEMC		MT6351_POWER_LDO_VEMC

#define msdc_hwPowerOn(powerId, powerVolt, name) \
	hwPowerOn(powerId, powerVolt, name)

#define msdc_hwPowerDown(powerId, name) \
	hwPowerDown(powerId, name)
#endif

#ifndef CONFIG_MTK_LEGACY
u32 msdc_ldo_power(u32 on, unsigned int powerId, int voltage_uv,
	u32 *status)
#else
u32 msdc_ldo_power(u32 on, MT65XX_POWER powerId, int voltage_uv,
	u32 *status)
#endif
{
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
		pmic_read_interface(REG_VEMC_EN, &ldo_en, MASK_VEMC_EN,
			SHIFT_VEMC_EN);
		pmic_read_interface(REG_VEMC_VOSEL, &ldo_vol, MASK_VEMC_VOSEL,
			SHIFT_VEMC_VOSEL);
		pr_err(" VEMC_EN=0x%x, should:bit1=1, VEMC_VOL=0x%x, should:1b'0(3.0V),1b'1(3.3V)\n",
			ldo_en, ldo_vol);
		/*
		pwrap_read(REG_VEMC_EN, &ldo_en);
		pwrap_read(REG_VEMC_VOSEL, &ldo_vol);
		SIMPLE_INIT_MSG(" VEMC_EN[0x%x]=0x%x, should:bit1=1, VEMC_VOL[0x%x]=0x%x,",
			REG_VEMC_EN, ldo_en, REG_VEMC_VOSEL, ldo_vol);
		SIMPLE_INIT_MSG(" should:bit[8:8]=1b'0(3.0V),1b'1(3.3V)\n");
		*/
		break;
	case 1:
		pmic_read_interface(REG_VMC_EN, &ldo_en, MASK_VMC_EN,
			SHIFT_VMC_EN);
		pmic_read_interface(REG_VMC_VOSEL, &ldo_vol, MASK_VMC_VOSEL,
			SHIFT_VMC_VOSEL);
		pr_err(" VMC_EN=0x%x, should:bit1=1, VMC_VOL=0x%x, should:3b'110(3.3V),3b'011(1.8V)\n",
			ldo_en, ldo_vol);
		pmic_read_interface(REG_VMCH_EN, &ldo_en, MASK_VMCH_EN,
			SHIFT_VEMC_EN);
		pmic_read_interface(REG_VMCH_VOSEL, &ldo_vol, MASK_VMCH_VOSEL,
			SHIFT_VMCH_VOSEL);
		pr_err(" VMCH_EN=0x%x, should:bit1=1, VMCH_VOL=0x%x, should:1b'0(3.0V),1b'1(3.3V)\n",
			ldo_en, ldo_vol);
		/*
		pwrap_read(REG_VMC_EN, &ldo_en);
		pwrap_read(REG_VMC_VOSEL, &ldo_vol);
		SIMPLE_INIT_MSG(" VMC_EN[0x%x]=0x%x,  should:bit1=1, VMC_VOL[0x%x]=0x%x,",
			REG_VMC_EN, ldo_en, REG_VMC_VOSEL, ldo_vol);
		SIMPLE_INIT_MSG(" should:bit[10:8]=3b'110(3.3V),3b'011(1.8V)\n");
		pwrap_read(REG_VMCH_EN, &ldo_en);
		pwrap_read(REG_VMCH_VOSEL, &ldo_vol);
		SIMPLE_INIT_MSG(" VMCH_EN[0x%x]==0x%x,should:bit1=1, "
			"VMCH_VOL[0x%x]=0x%x, should:bit[8:8]=1b'0(3.0V),"
			"1b'1(3.3V)\n",
			REG_VMCH_EN, ldo_en, REG_VMCH_VOSEL, ldo_vol);
		*/
		break;
	default:
		break;
	}
#endif
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	switch (host->id) {
	case 1:
		msdc_ldo_power(on, POWER_LDO_VMC, VOL_1800, &g_msdc1_io);
		/*
		if (on)
			upmu_set_rg_vmc_184(1);*/
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
		msdc_ldo_power(card_on, POWER_LDO_VMCH, VOL_3000,
			 &g_msdc1_flash);
		msdc_ldo_power(on, POWER_LDO_VMC, VOL_3000, &g_msdc1_io);
/* FIXME: Does MT6351 pmic have 1.8v drop problem ? */
#if 0
		if (on)
			/*upmu_set_rg_vmc_184(0);*/
			pmic_config_interface(REG_VEMC_VOSEL,
				VEMC_VOSEL_MINUS_100mV,
				MASK_VEMC_VOSEL_CAL,
				0);
#endif
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

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
/* clock source for host: global */
u32 hclks_msdc50[] = {26000000 , 400000000, 200000000, 156000000,
		      182000000, 156000000, 100000000, 624000000,
		      312000000};

u32 hclks_msdc30[] = {26000000 , 208000000, 100000000, 156000000,
		      182000000, 156000000, 178000000, 200000000};

u32 hclks_msdc30_3[] = {26000000 , 50000000, 100000000, 156000000,
		      48000000, 156000000, 178000000, 54000000,
		      25000000};

u32 *hclks_msdc = hclks_msdc30;

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_clkmgr.h>
enum cg_clk_id msdc_cg_clk_id[HOST_MAX_NUM] = {
	MT_CG_PERI_MSDC30_0,
	MT_CG_PERI_MSDC30_1,
	MT_CG_PERI_MSDC30_2,
	MT_CG_PERI_MSDC30_3
};
#endif

#ifndef CONFIG_MTK_LEGACY
/* FIX ME: check if mt6735-clk.h shall be renamed */
#include <dt-bindings/clock/mt6735-clk.h>
struct clk *g_msdc0_pll_sel;
struct clk *g_msdc0_pll_400m;
struct clk *g_msdc0_pll_200m;

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	if (pdev->id == 0) {
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC0-CLOCK");
		g_msdc0_pll_sel  = devm_clk_get(&pdev->dev, "MSDC0_PLL_SEL");
		g_msdc0_pll_400m = devm_clk_get(&pdev->dev, "MSDC0_PLL_400M");
		g_msdc0_pll_200m = devm_clk_get(&pdev->dev, "MSDC0_PLL_200M");
	} else if (pdev->id == 1) {
		host->clock_control = devm_clk_get(&pdev->dev, "MSDC1-CLOCK");
	} else if (pdev->id == 2) {
		/*host->clock_control = devm_clk_get(&pdev->dev,
			"MSDC2-CLOCK");*/
	} else if (pdev->id == 3) {
		/*host->clock_control = devm_clk_get(&pdev->dev,
			"MSDC3-CLOCK");*/
	}
	if (IS_ERR(host->clock_control)) {
		pr_err("can not get msdc%d clock control\n", pdev->id);
		return 1;
	}
	if (clk_prepare(host->clock_control)) {
		pr_err("can not prepare msdc%d clock control\n",
			pdev->id);
		return 1;
	}
	if (host->id == 0) {
		if (IS_ERR(g_msdc0_pll_sel) ||
		    IS_ERR(g_msdc0_pll_400m) || IS_ERR(g_msdc0_pll_200m)) {
			pr_err("msdc0 get pll error,g_msdc0_pll_sel=%p, g_msdc0_pll_400m=%p,g_msdc0_pll_200m=%p\n",
				g_msdc0_pll_sel,
				g_msdc0_pll_400m, g_msdc0_pll_200m);
			return 1;
		}
		if (clk_prepare(g_msdc0_pll_sel)) {
			pr_err("msdc%d can not prepare g_msdc0_pll_sel\n",
				pdev->id);
			return 1;
		}
	}

	return 0;
}
#endif

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	u32 *hclks;
#ifdef CONFIG_MTK_LEGACY
	char name[6];
#endif

	hclks = msdc_get_hclks(host->id);

	pr_err("[%s]: msdc%d change clk_src from %dKHz to %d:%dKHz\n", __func__,
		host->id, (host->hclk/1000), clksrc, (hclks[clksrc]/1000));

#ifdef CONFIG_MTK_LEGACY
	sprintf(name, "MSDC%d", host->id);
	clkmux_sel(MT_MUX_MSDC30_0 - host->id, clksrc, name);
#else
	if (host->id != 0) {
		pr_err("NOT Support msdc%d switch pll souce[%s]%d\n", host->id,
			__func__, __LINE__);
		return;
	}

	if (clksrc == MSDC50_CLKSRC_400MHZ) {
		clk = g_msdc0_pll_400m;
	} else if (clksrc == MSDC50_CLKSRC_200MHZ) {
		clk = g_msdc0_pll_200m;
	} else {
		pr_err("NOT Support msdc%d switch pll souce[%s]%d\n", host->id,
			__func__, __LINE__);
		return;
	}

	clk_enable(g_msdc0_pll_sel);
	ret = clk_set_parent(g_msdc0_pll_sel, clk);
	if (ret)
		pr_err("XXX MSDC%d switch clk source ERROR...[%s]%d\n",
			host->id, __func__, __LINE__);
	clk_disable(g_msdc0_pll_sel);
#endif

	host->hclk = hclks[clksrc];
	host->hw->clk_src = clksrc;
}

void msdc_dump_clock_sts(struct msdc_host *host)
{
#ifdef MTK_MSDC_BRINGUP_DEBUG
	if (apmixed_reg_base && topckgen_reg_base && pericfg_reg_base) {
		pr_err(" MSDCPLL_PWR_CON0[0x%p][bit0~1 should be 2b'01]=0x%x\n",
			(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET),
			MSDC_READ32(
				apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET));
		pr_err(" MSDCPLL_CON0    [0x%p][bit0 should be 1b'1]=0x%x\n",
			(apmixed_reg_base + MSDCPLL_CON0_OFFSET),
			MSDC_READ32(
				apmixed_reg_base + MSDCPLL_CON0_OFFSET));
		pr_err(" CLK_CFG_2       [0x%p][bit[31:24]should be 0x01]=0x%x\n",
			(topckgen_reg_base + MSDC_CLK_CFG_2_OFFSET),
			MSDC_READ32(
				topckgen_reg_base + MSDC_CLK_CFG_2_OFFSET));
		pr_err(" CLK_CFG_3       [0x%p][bit[15:0]should be 0x0202]=0x%x\n",
			(topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET),
			MSDC_READ32(
				topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET));
		pr_err(" PERI_PDN_STA0   [0x%p][bit13=msdc0, bit14=msdc1,0:on,1:off]=0x%x\n",
			(pericfg_reg_base + MSDC_PERI_PDN_STA0_OFFSET),
			MSDC_READ32(
				pericfg_reg_base + MSDC_PERI_PDN_STA0_OFFSET));
	} else {
		pr_err(" apmixed_reg_base = %p, topckgen_reg_base = %p, clk_pericfg_base = %p\n",
			apmixed_reg_base, topckgen_reg_base, pericfg_reg_base);
	}
#endif
}

/* do we need sync object or not */
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

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
void msdc_dump_padctl_by_id(u32 id)
{
	switch (id) {
	case 0:
		pr_err("MSDC0 MODE18[0x%p] =0x%8x\tshould:0x12-- ----\n",
			MSDC0_GPIO_MODE18, MSDC_READ32(MSDC0_GPIO_MODE18));
		pr_err("MSDC0 MODE19[0x%p] =0x%8x\tshould:0x1249 1249\n",
			MSDC0_GPIO_MODE18, MSDC_READ32(MSDC0_GPIO_MODE19));
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
		pr_err("MSDC2 MODE14[0x%p] =0x%8x\tshould:0x--12 2492\n",
			MSDC2_GPIO_MODE14, MSDC_READ32(MSDC2_GPIO_MODE14));
		/* To be implemented later
		IES
		SMT
		TDSEL
		RDSEL
		SR
		DRV
		PUPD
		*/
		break;
#endif
	}
}

void msdc_set_pin_mode(struct msdc_host *host)
{
	switch (host->id) {
	case 0:
		MSDC_SET_FIELD(MSDC0_GPIO_MODE18, (0x3F<<25), 0x9);
		MSDC_SET_FIELD(MSDC0_GPIO_MODE19, 0xFFFFFFFF, 0x12491249);
		break;

	case 1:
		MSDC_SET_FIELD(MSDC1_GPIO_MODE4, 0x0003FFFF, 0x0011249);
		break;

#ifdef CFG_DEV_MSDC2
	case 2:
		MSDC_SET_FIELD(MSDC2_GPIO_MODE14, 0x003FFFF8, 0x00122492);
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

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_IES_ADDR, MSDC2_IES_ALL_MASK,
			 (set_ies ? 0x7 : 0));
		*/
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

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_SMT_ADDR, MSDC2_SMT_ALL_MASK,
			(set_smt ? 0x7 : 0));
		*/
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

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL_ADDR, MSDC2_TDSEL_ALL_MASK , 0);
		*/
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

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL0_G0_ADDR, MSDC2_RDSEL_ALL_MASK,
			0);
		*/
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
#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL0_G0_ADDR,
			MSDC2_TDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL0_G0_ADDR,
			MSDC2_TDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_TDSEL0_G0_ADDR,
			MSDC2_TDSEL_DAT_MASK, value);
		*/
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
#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL0_G0_ADDR,
			MSDC2_RDSEL_CMD_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL0_G0_ADDR,
			MSDC2_RDSEL_CLK_MASK, value);
		MSDC_SET_FIELD(MSDC2_GPIO_RDSEL0_G0_ADDR,
			MSDC2_RDSEL_DAT_MASK, value);
		*/
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
#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_GET_FIELD(MSDC2_GPIO_TDSEL0_G0_ADDR,
			MSDC2_TDSEL_CMD_MASK, *value);
		*/
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
#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_GET_FIELD(MSDC2_GPIO_RDSEL0_G0_ADDR,
			MSDC2_RDSEL_CMD_MASK, *value);
		*/
		break;
#endif
	}
}

void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst,
	int ds)
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

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_SV_ADDR, MSDC2_SR_CMD_MASK,
			(cmd != 0));
		MSDC_SET_FIELD(MSDC2_GPIO_SV_ADDR, MSDC2_SR_CLK_MASK,
			(clk != 0));
		MSDC_SET_FIELD(MSDC2_GPIO_SR_ADDR, MSDC2_SR_DAT_MASK,
			(dat != 0));
		*/
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
		/* To be implementted later
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR,MSDC2_DRV_CMD_MASK,
			hw->cmd_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR,MSDC2_DRV_CLK_MASK,
			hw->clk_drv);
		MSDC_SET_FIELD(MSDC2_GPIO_DRV_ADDR,MSDC2_DRV_DAT_MASK,
			hw->dat_drv);
		*/
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}
}

void msdc_get_driving_by_id(u32 id, struct msdc_ioctl *msdc_ctl)
{
	switch (id) {
	case 0:
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CMD_MASK,
			msdc_ctl->cmd_pu_driving);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DSL_MASK,
			msdc_ctl->ds_pu_driving);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_CLK_MASK,
			msdc_ctl->clk_pu_driving);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_DAT_MASK,
			msdc_ctl->dat_pu_driving);
		MSDC_GET_FIELD(MSDC0_GPIO_DRV_ADDR, MSDC0_DRV_RSTB_MASK,
			msdc_ctl->rst_pu_driving);
		break;
	case 1:
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CMD_MASK,
			msdc_ctl->cmd_pu_driving);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_CLK_MASK,
			msdc_ctl->clk_pu_driving);
		MSDC_GET_FIELD(MSDC1_GPIO_DRV_ADDR, MSDC1_DRV_DAT_MASK,
			msdc_ctl->dat_pu_driving);
		msdc_ctl->rst_pu_driving = 0;
		msdc_ctl->ds_pu_driving = 0;
		break;

#ifdef CFG_DEV_MSDC2 /*FIXME: For 6630*/
	case 2:
		/* To be implementted later
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CMD_MASK,
			msdc_ctl->cmd_pu_driving);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_CLK_MASK,
			msdc_ctl->clk_pu_driving);
		MSDC_GET_FIELD(MSDC2_GPIO_DRV_ADDR, MSDC2_DRV_DAT_MASK,
			msdc_ctl->dat_pu_driving);
		msdc_ctl->rst_pu_driving = 0;
		msdc_ctl->ds_pu_driving = 0;
		*/
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
				MSDC0_PUPD0_MASK, 0x04444444);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x4444);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* cmd/clk/dat/(rstb)/dsl:pd-50k*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x06666666);
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD1_ADDR,
				MSDC0_PUPD1_MASK, 0x6666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* clk/dsl:pd-50k, cmd/dat:pu-10k, (rstb:pu-50k)*/
			MSDC_SET_FIELD(MSDC0_GPIO_PUPD0_ADDR,
				MSDC0_PUPD0_MASK, 0x01111161);
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
			MSDC_SET_FIELD(MSDC1_GPIO_PUPD0_ADDR,
				MSDC1_PUPD_MASK, 0x222262);
		}
		break;
#ifdef CFG_DEV_MSDC2
	case 2:
		/* To be implementted later */
		#if 0
		if (MSDC_PIN_PULL_NONE == mode) {
			/* high-Z */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD_CMD_CLK_DAT_MASK, 0x444444);
		} else if (MSDC_PIN_PULL_DOWN == mode) {
			/* cmd/clk/dat:pd-50k */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD_CMD_CLK_DAT_MASK, 0x666666);
		} else if (MSDC_PIN_PULL_UP == mode) {
			/* cmd/dat:pu-50k, clk:pd-50k, */
			MSDC_SET_FIELD(MSDC2_GPIO_PUPD0_ADDR,
				MSDC2_PUPD_CMD_CLK_DAT_MASK, 0x222262);
		}
		#endif
		break;
#endif

	default:
		pr_err("[%s] invlalid host->id!\n", __func__);
		break;
	}

}

/**************************************************************/
/* Section 5: MISC                                            */
/**************************************************************/

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
	unsigned int i = 0;
	unsigned int addr = 0;

	return; /*weiping check */

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
