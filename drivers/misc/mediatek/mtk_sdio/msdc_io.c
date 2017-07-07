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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

#include "mt_sd.h"
#include "dbg.h"

/**************************************************************/
/* Section 1: Device Tree Global Variables                    */
/**************************************************************/
const struct of_device_id msdc_of_ids[] = {
	{   .compatible = MT2701_DT_COMPATIBLE_NAME, },
	{   .compatible = MT2712_DT_COMPATIBLE_NAME, },
};

void __iomem *gpio_base;
void __iomem *infracfg_ao_reg_base;
void __iomem *infracfg_reg_base;
void __iomem *pericfg_reg_base;
void __iomem *toprgu_reg_base;
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

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
u32 hclks_msdc50[] = { MSDC0_SRC_0, MSDC0_SRC_1, MSDC0_SRC_2, MSDC0_SRC_3,
		       MSDC0_SRC_4, MSDC0_SRC_5, MSDC0_SRC_6, MSDC0_SRC_7,
		       MSDC0_SRC_8};

/* msdc1/2 clock source reference value is 200M */
u32 hclks_msdc30[] = { MSDC1_SRC_0, MSDC1_SRC_1, MSDC1_SRC_2, MSDC1_SRC_3,
		       MSDC1_SRC_4, MSDC1_SRC_5, MSDC1_SRC_6, MSDC1_SRC_7};

u32 hclks_msdc30_3[] = { MSDC3_SRC_0, MSDC3_SRC_1, MSDC3_SRC_2, MSDC3_SRC_3,
			 MSDC3_SRC_4, MSDC3_SRC_5, MSDC3_SRC_6, MSDC3_SRC_7,
			 MSDC3_SRC_8};

u32 *hclks_msdc_all[] = {
	hclks_msdc50,
	hclks_msdc30,
	hclks_msdc30,
	hclks_msdc30_3
};
u32 *hclks_msdc = hclks_msdc30;


/* #include <dt-bindings/clock/mt2701-clk.h> */
/* #include <mt_clk_id.h> */

#if 0
int msdc_cg_clk_id[HOST_MAX_NUM] = {
	MSDC0_CG_NAME,
	MSDC1_CG_NAME,
	MSDC2_CG_NAME,
	MSDC3_CG_NAME
};
#endif

int msdc_get_ccf_clk_pointer(struct platform_device *pdev,
	struct msdc_host *host)
{
	static char const * const clk_names[] = {
		MSDC0_CLK_NAME, MSDC1_CLK_NAME, MSDC2_CLK_NAME, MSDC3_CLK_NAME
	};

	host->clock_control = devm_clk_get(&pdev->dev, clk_names[pdev->id]);

	if (IS_ERR(host->clock_control)) {
		pr_err("can not get msdc%d clock control\n", pdev->id);
		return 1;
	}

	host->clock_hclk = devm_clk_get(&pdev->dev, "hclk");
	if (IS_ERR(host->clock_hclk))
		return 1;

	host->clock_source_cg = devm_clk_get(&pdev->dev, "source_cg");
	if (IS_ERR(host->clock_source_cg))
		return 1;

	host->clk_on = false;
	msdc_prepare_clk(host);
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

static int msdc_get_pinctl_settings(struct msdc_host *host, struct device_node *np)
{
	struct device_node *pinctl_node, *pins_node;
	static char const * const pinctl_names[] = {
		"pinctl", "pinctl_sdr104", "pinctl_sdr50", "pinctl_ddr50"
	};

	static char const * const pins_names[] = {
		"pins_cmd", "pins_dat", "pins_clk", "pins_rst", "pins_ds"
	};
	unsigned char *pins_drv[5];
	int i, j;

	for (i = 0; i < ARRAY_SIZE(pinctl_names); i++) {
		pinctl_node = of_parse_phandle(np, pinctl_names[i], 0);

		if (strcmp(pinctl_names[i], "pinctl") == 0) {
			pins_drv[0] = &host->hw->cmd_drv;
			pins_drv[1] = &host->hw->dat_drv;
			pins_drv[2] = &host->hw->clk_drv;
			pins_drv[3] = &host->hw->rst_drv;
			pins_drv[4] = &host->hw->ds_drv;
		} else if (strcmp(pinctl_names[i], "pinctl_sdr104") == 0) {
			pins_drv[0] = &host->hw->cmd_drv_sd_18;
			pins_drv[1] = &host->hw->dat_drv_sd_18;
			pins_drv[2] = &host->hw->clk_drv_sd_18;
		} else if (strcmp(pinctl_names[i], "pinctl_sdr50") == 0) {
			pins_drv[0] = &host->hw->cmd_drv_sd_18_sdr50;
			pins_drv[1] = &host->hw->dat_drv_sd_18_sdr50;
			pins_drv[2] = &host->hw->clk_drv_sd_18_sdr50;
		} else if (strcmp(pinctl_names[i], "pinctl_ddr50") == 0) {
			pins_drv[0] = &host->hw->cmd_drv_sd_18_ddr50;
			pins_drv[1] = &host->hw->dat_drv_sd_18_ddr50;
			pins_drv[2] = &host->hw->clk_drv_sd_18_ddr50;
		} else {
			continue;
		}

		for (j = 0; j < ARRAY_SIZE(pins_names); j++) {
			pins_node = of_get_child_by_name(pinctl_node,
				pins_names[j]);
			if (pins_node)
				of_property_read_u8(pins_node,
					"drive-strength", pins_drv[j]);
		}
	}

	return 0;
}

/* Get msdc register settings
 * 1. internal data delay for tuning, FIXME: can be removed when use data tune?
 * 2. sample edge
 */
static int msdc_get_register_settings(struct msdc_host *host, struct device_node *np)
{
	struct device_node *register_setting_node = NULL;
	int i;
	int ret;

	/* parse hw property settings */
	register_setting_node = of_parse_phandle(np, "register_setting", 0);
	if (register_setting_node) {
		for (i = 0; i < 8; i++) {
			of_property_read_u8_array(register_setting_node,
				"datrddly", host->hw->datrddly, 8);
		}

		ret = of_property_read_u8(register_setting_node, "datwrddly",
				&host->hw->datwrddly);
		ret = of_property_read_u8(register_setting_node, "cmdrrddly",
				&host->hw->cmdrrddly);
		ret = of_property_read_u8(register_setting_node, "cmdrddly",
				&host->hw->cmdrddly);

		ret = of_property_read_u8(register_setting_node, "cmd_edge",
				&host->hw->cmd_edge);
		ret = of_property_read_u8(register_setting_node, "rdata_edge",
				&host->hw->rdata_edge);
		ret = of_property_read_u8(register_setting_node, "wdata_edge",
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
int msdc_of_parse(struct mmc_host *mmc)
{
	struct device_node *np;
	struct msdc_host *host = mmc_priv(mmc);
	int len;
	int ret;
	int read_tmp;

	ret = mmc_of_parse(mmc);
	if (ret) {
		pr_err("%s: mmc of parse error!!: %d\n", __func__, ret);
		return ret;
	}

	np = mmc->parent->of_node; /* mmcx node in projectdts */

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
	WARN_ON(host->irq < 0);

	/* get clk_src */
	#if !defined(FPGA_PLATFORM)
	if (of_property_read_u32(np, "clk_src", &read_tmp)) {
		pr_err("[msdc%d] error: clk_src isn't found in device tree.\n",
			host->id);
		WARN_ON(1);
	} else {
		host->hw->clk_src = read_tmp;
	}
	#endif

	/* get msdc flag(caps) */
	if (of_find_property(np, "msdc-sys-suspend", &len))
		host->hw->flags |= MSDC_SYS_SUSPEND;

	/* Returns 0 on success, -EINVAL if the property does not exist,
	 * -ENODATA if property does not have a value, and -EOVERFLOW if the
	 * property data isn't large enough.
	 */

	if (of_property_read_u32(np, "host_function", &read_tmp)) {
		pr_err("[msdc%d] host_function isn't found in device tree\n",
			host->id);
	} else {
		host->hw->host_function = read_tmp;
	}

	if (of_find_property(np, "bootable", &len))
		host->hw->boot = 1;

	/* get cd_gpio and cd_level */
	if (of_property_read_u32_index(np, "cd-gpios", 1, &cd_gpio) == 0) {
		if (of_property_read_u8(np, "cd_level", &host->hw->cd_level))
			pr_err("[msdc%d] cd_level isn't found in device tree\n",
				host->id);
	}

	ret = msdc_get_register_settings(host, np);

	#if !defined(FPGA_PLATFORM)
	ret = msdc_get_pinctl_settings(host, np);

	mmc->supply.vmmc = regulator_get(mmc_dev(mmc), "vmmc");
	mmc->supply.vqmmc = regulator_get(mmc_dev(mmc), "vqmmc");

	#else
	msdc_fpga_pwr_init();
	#endif

	pr_err("[msdc%d] host_function:%d, clk_src=%d\n", host->id, host->hw->host_function, host->hw->clk_src);
#ifdef CFG_DEV_MSDC2
	if (host->hw->host_function == MSDC_SDIO) {
		host->hw->flags |= MSDC_EXT_SDIO_IRQ;
		mmc->pm_flags |= MMC_PM_KEEP_POWER;
		mmc->pm_caps |= MMC_PM_KEEP_POWER;
		mmc->caps |= MMC_CAP_NONREMOVABLE;
		host->hw->request_sdio_eirq = mt_sdio_ops[host->id].sdio_request_eirq;
		host->hw->enable_sdio_eirq = mt_sdio_ops[host->id].sdio_enable_eirq;
		host->hw->disable_sdio_eirq = mt_sdio_ops[host->id].sdio_disable_eirq;
		host->hw->register_pm = mt_sdio_ops[host->id].sdio_register_pm;
	}
#endif

	return 0;
}

int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	unsigned int id = 0;
	int ret;
	static char const * const msdc_names[] = {
		"msdc0", "msdc1", "sdio", "msdc3_sdio"};
	static char const * const ioconfig_names[] = { MSDC0_IOCFG_NAME, MSDC1_IOCFG_NAME,
		MSDC2_IOCFG_NAME, MSDC3_IOCFG_NAME
	};
	struct device_node *np;

	pr_err("DT probe %s!\n", pdev->dev.of_node->name);

	for (id = 0; id < HOST_MAX_NUM; id++) {
		if (strcmp(pdev->dev.of_node->name, msdc_names[id]) == 0) {
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
		pr_err("msdc%d of parse fail!!: %d\n", id, ret);
		return ret;
	}

	/* host_id = 2 indicate mt2701 */
	if (host->id == 2) {
		if (gpio_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-pctl-a-syscfg");
			gpio_base = of_iomap(np, 0);
			pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
		}

		if (msdc_io_cfg_bases[id] == NULL) {
			np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
			msdc_io_cfg_bases[id] = of_iomap(np, 0);
			pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
				id, msdc_io_cfg_bases[id]);
		}

#ifndef FPGA_PLATFORM
		if (infracfg_ao_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL,
				"mediatek,mt2701-dcm");
			infracfg_ao_reg_base = of_iomap(np, 1);
			pr_err("of_iomap for infracfg_ao base @ 0x%p\n",
				infracfg_ao_reg_base);
		}

		if (toprgu_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-rgu");
			toprgu_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for toprgu base @ 0x%p\n",
				toprgu_reg_base);
		}

		if (apmixed_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2701-apmixedsys");
			apmixed_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for apmixed base @ 0x%p\n",
				apmixed_reg_base);
		}

		if (topckgen_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL,
				"mediatek,mt2701-topckgen");
			topckgen_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for topckgen base @ 0x%p\n",
				topckgen_reg_base);
		}
#endif
	} else if (host->id == 3) {/* host_id = 3 indicates mt2712 */
		if (gpio_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2712-pctl-a-syscfg");
			gpio_base = of_iomap(np, 0);
			pr_debug("of_iomap for gpio base @ 0x%p\n", gpio_base);
		}

		if (msdc_io_cfg_bases[id] == NULL) {
			np = of_find_compatible_node(NULL, NULL, ioconfig_names[id]);
			msdc_io_cfg_bases[id] = of_iomap(np, 0);
			pr_debug("of_iomap for MSDC%d IOCFG base @ 0x%p\n",
				id, msdc_io_cfg_bases[id]);
		}

#ifndef FPGA_PLATFORM
		if (infracfg_ao_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL,
				"mediatek,mt2712-infracfg");
			infracfg_ao_reg_base = of_iomap(np, 1);
			pr_err("of_iomap for infracfg_ao base @ 0x%p\n",
				infracfg_ao_reg_base);
		}

		if (toprgu_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2712-rgu");
			toprgu_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for toprgu base @ 0x%p\n",
				toprgu_reg_base);
		}

		if (apmixed_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL, "mediatek,mt2712-apmixedsys");
			apmixed_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for apmixed base @ 0x%p\n",
				apmixed_reg_base);
		}

		if (topckgen_reg_base == NULL) {
			np = of_find_compatible_node(NULL, NULL,
				"mediatek,mt2712-topckgen");
			topckgen_reg_base = of_iomap(np, 0);
			pr_err("of_iomap for topckgen base @ 0x%p\n",
				topckgen_reg_base);
		}
#endif
	}

	return 0;
}

