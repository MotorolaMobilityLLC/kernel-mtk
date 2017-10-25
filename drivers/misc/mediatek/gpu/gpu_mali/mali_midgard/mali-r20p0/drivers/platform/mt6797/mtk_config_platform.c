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

#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <platform/mtk_platform_common.h>

#include "mali_kbase_config_platform.h"
#include "mtk_config.h"

#ifdef ENABLE_COMMON_DVFS
#include <ged_dvfs.h>
#endif

#include <ged_log.h>

#include <mt_gpufreq.h>
#include <fan53555.h>
#ifdef CONFIG_REGULATOR_RT5735
#include <mtk_gpuregulator_intf.h>
#endif

struct mtk_config *g_config;

static void *g_ldo_base;
static void *g_MFG_base;

static spinlock_t mtk_power_lock;
static unsigned long mtk_power_lock_flags;

static int g_is_power_on;

static void power_status_acquire(void)
{
	spin_lock_irqsave(&mtk_power_lock, mtk_power_lock_flags);
}
static void power_status_release(void)
{
	spin_unlock_irqrestore(&mtk_power_lock, mtk_power_lock_flags);
}

static int mtk_pm_callback_power_on(void);
static void mtk_pm_callback_power_off(void);

#define MTKCLK_prepare_enable(clk) \
do {\
	if (config->clk && clk_prepare_enable(config->clk))\
		pr_alert("MALI: clk_prepare_enable failed when enabling " #clk);\
} while (0)

#define MTKCLK_disable_unprepare(clk) \
do {\
	if (config->clk)\
		clk_disable_unprepare(config->clk);\
} while (0)

int dvfs_gpu_pm_spin_lock_for_vgpu(void)
{
	int ret = 0;
	return ret;
}

int dvfs_gpu_pm_spin_unlock_for_vgpu(void)
{
	int ret = 0;
	return ret;
}

static void toprgu_mfg_reset(void)
{
	mtk_wdt_swsysret_config(MTK_WDT_SWSYS_RST_MFG_RST, 1);
	udelay(1);
	mtk_wdt_swsysret_config(MTK_WDT_SWSYS_RST_MFG_RST, 0);
}

static int mtk_pm_callback_power_on(void)
{
	struct mtk_config *config = g_config;

	base_write32(g_ldo_base+0xfbc, 0x1ff);
	mt_gpufreq_voltage_enable_set(1);

	MTKCLK_prepare_enable(clk_mfg52m_vcg);

	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);
	MTKCLK_prepare_enable(clk_mfg_core2);
	MTKCLK_prepare_enable(clk_mfg_core3);
	MTKCLK_prepare_enable(clk_mfg_main);

	/* Release MFG PROT */
	udelay(100);
	spm_topaxi_protect(MFG_PROT_MASK, 0);

	/* timing */
	MFG_write32(0x1c, MFG_read32(0x1c) | config->async_value);

	/* enable PMU */
	MFG_write32(0x3e0, 0xffffffff);
	MFG_write32(0x3e4, 0xffffffff);
	MFG_write32(0x3e8, 0xffffffff);
	MFG_write32(0x3ec, 0xffffffff);
	MFG_write32(0x3f0, 0xffffffff);

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);
#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	return 1;
}

static void mtk_pm_callback_power_off(void)
{
	int polling_retry = 100000;
	struct mtk_config *config = g_config;

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(0);
#endif
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);

	/* polling mfg idle */
	MFG_write32(MFG_DEBUG_SEL, 0x3);
	while ((MFG_read32(MFG_DEBUG_A) & MFG_DEBUG_IDEL) != MFG_DEBUG_IDEL && --polling_retry)
		udelay(1);
	if (polling_retry <= 0)
		pr_MTK_err("[dump] polling fail: idle rem:%d - MFG_DBUG_A=%x\n",
		polling_retry, MFG_read32(MFG_DEBUG_A));

	/* Set MFG PROT */
	spm_topaxi_protect(MFG_PROT_MASK, 1);

	MTKCLK_disable_unprepare(clk_mfg_main);
	MTKCLK_disable_unprepare(clk_mfg_core3);
	MTKCLK_disable_unprepare(clk_mfg_core2);
	MTKCLK_disable_unprepare(clk_mfg_core1);
	MTKCLK_disable_unprepare(clk_mfg_core0);
	MTKCLK_disable_unprepare(clk_mfg);
	MTKCLK_disable_unprepare(clk_mfg_async);

	MTKCLK_disable_unprepare(clk_mfg52m_vcg);

	mt_gpufreq_voltage_enable_set(0);
	base_write32(g_ldo_base+0xfbc, 0x0);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 0;

	if (g_is_power_on != 1) {
		ret = mtk_pm_callback_power_on();

		power_status_acquire();
		g_is_power_on = 1;
		power_status_release();
	}

	return ret;
}
static void pm_callback_power_off(struct kbase_device *kbdev)
{
	if (g_is_power_on != 0) {
		power_status_acquire();
		g_is_power_on = 0;
		power_status_release();

		mtk_pm_callback_power_off();
	}
}
static void pm_callback_suspend(struct kbase_device *kbdev)
{
	pm_callback_power_off(kbdev);
}
static void pm_callback_resume(struct kbase_device *kbdev)
{
	pm_callback_power_on(kbdev);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume
};

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Make sure the ptpod is ready (not 0) */
	if (!get_eem_status_for_gpu()) {
		pr_warn("ptpod is not ready\n");
		return -EPROBE_DEFER;
	}

	/* Make sure the power control is ready */
	if (is_fan53555_exist() && !is_fan53555_sw_ready()) {
		pr_warn("fan53555 is not ready\n");
		return -EPROBE_DEFER;
	}
#ifdef CONFIG_REGULATOR_RT5735
	if (rt_is_hw_exist() && !rt_is_sw_ready()) {
		pr_warn("rt5735 is not ready\n");
		return -EPROBE_DEFER;
	}
#endif

	return 0;
}

static void *_mtk_of_ioremap(const char *node_name)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, node_name);

	if (node)
		return of_iomap(node, 0);

	pr_MTK_err("cannot find [%s] of_node, please fix me\n", node_name);
	return NULL;
}

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{
	struct mtk_config *config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);

	if (config == NULL)
		return -1;

	memset(config, 0, sizeof(struct mtk_config));

	spin_lock_init(&mtk_power_lock);

	g_ldo_base =        _mtk_of_ioremap("mediatek,infracfg_ao");
	g_MFG_base =        _mtk_of_ioremap("mediatek,g3d_config");

	config->clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
	config->clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
	config->clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
	config->clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
	config->clk_mfg_core2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core2");
	config->clk_mfg_core3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core3");
	config->clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");
	config->clk_mfg52m_vcg = devm_clk_get(&pdev->dev, "mfg52m-vcg");

	config->mux_mfg52m = devm_clk_get(&pdev->dev, "mux-mfg52m");
	config->mux_mfg52m_52m = devm_clk_get(&pdev->dev, "mux-univpll2-d8");

	dev_MTK_err(kbdev->dev, "xxxx mfg_async:%p\n", config->clk_mfg_async);
	dev_MTK_err(kbdev->dev, "xxxx mfg:%p\n", config->clk_mfg);
	dev_MTK_err(kbdev->dev, "xxxx mfg_core0:%p\n", config->clk_mfg_core0);
	dev_MTK_err(kbdev->dev, "xxxx mfg_core1:%p\n", config->clk_mfg_core1);
	dev_MTK_err(kbdev->dev, "xxxx mfg_core2:%p\n", config->clk_mfg_core2);
	dev_MTK_err(kbdev->dev, "xxxx mfg_core3:%p\n", config->clk_mfg_core3);
	dev_MTK_err(kbdev->dev, "xxxx mfg_main:%p\n", config->clk_mfg_main);

	config->max_volt = 1150;
	config->max_freq = mt_gpufreq_get_freq_by_idx(0);
	config->min_volt = 850;
	config->min_freq = mt_gpufreq_get_freq_by_idx(mt_gpufreq_get_dvfs_table_num());

	config->async_value = config->max_freq >= 780000 ? 0xa : 0x5;

	g_config = kbdev->mtk_config = config;

	/* RG_GPULDO_RSV_H_0-8 = 0x8 */
	base_write32(g_ldo_base+0xfd8, 0x88888888);
	base_write32(g_ldo_base+0xfe4, 0x00000008);

	base_write32(g_ldo_base+0xfc0, 0x0f0f0f0f);
	base_write32(g_ldo_base+0xfc4, 0x0f0f0f0f);
	base_write32(g_ldo_base+0xfc8, 0x0f);

	clk_prepare_enable(config->mux_mfg52m);
	clk_set_parent(config->mux_mfg52m, config->mux_mfg52m_52m);
	clk_disable_unprepare(config->mux_mfg52m);

	toprgu_mfg_reset();

	/* Set MFG PROT */
	spm_topaxi_protect(MFG_PROT_MASK, 1);

	return 0;
}

