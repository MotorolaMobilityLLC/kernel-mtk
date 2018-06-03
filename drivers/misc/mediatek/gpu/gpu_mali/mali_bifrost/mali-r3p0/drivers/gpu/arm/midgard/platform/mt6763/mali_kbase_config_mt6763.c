/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */





#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "mali_kbase_cpu_mt6763.h"
#include "mtk_config_mt6763.h"
#include "platform/mtk_platform_common.h"

#include "mtk_gpufreq.h"

#define HARD_RESET_AT_POWER_OFF 0
/* #define GPU_DVFS_DEBUG */
#define MTCMOS_READY

#define MTCMOS_ON 1
#define MTCMOS_OFF 0

struct mtk_config *g_config;
volatile void *g_MFG_base;
int g_curFreqID;
static int g_MTCMOS_flag;


#if 0
/**
 * For GPU idle check
 *
*/

static int mtk_check_MFG_idle(void)
{
	u32 val;

	val = readl(g_MFG_base + 0xb0);
	writel(val & ~(0x1), g_MFG_base + 0xb0);
#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] 0x130000b0 val = 0x%x\n", readl(g_MFG_base + 0xb0));
#endif

	val = readl(g_MFG_base + 0x10);
	writel(val | (0x1 << 17), g_MFG_base + 0x10);
#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] 0x13000010 val = 0x%x\n", readl(g_MFG_base + 0x10));
#endif

	val = readl(g_MFG_base + 0x180);
	writel((val & ~(0xFF)) | 0x3, g_MFG_base + 0x180);
#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] 0x13000180 val = 0x%x\n", readl(g_MFG_base + 0x180));
#endif

	do {
		val = readl(g_MFG_base + 0x188);
#ifdef GPU_DVFS_DEBUG
		pr_alert("[MALI] 0x13000188 val = 0x%x\n", val);
#endif
	} while ((val & 0x4) != 0x4);

	return 0;
}
#endif

#if 0
/**
 * For GPU suspend/resume
 *
*/

static void _mtk_pm_callback_power_suspend(void)
{
	struct mtk_config *config = g_config;

	if (!config) {
		pr_alert("[MALI] power_suspend: mtk_config is NULL\n");
		return;
	}

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_suspend: start power off, MTCMOS Status: %d\n", g_MTCMOS_flag);
#endif /*GPU_DVFS_DEBUG*/

	g_curFreqID = mt_gpufreq_get_cur_freq_index();
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);

#if 0
	if (g_MTCMOS_flag == MTCMOS_ON) {
		mtk_check_MFG_idle();

#ifdef MTCMOS_READY
		MTKCLK_disable_unprepare(clk_mfg_cg);
		MTKCLK_disable_unprepare(clk_mfg3);
		MTKCLK_disable_unprepare(clk_mfg2);
		MTKCLK_disable_unprepare(clk_mfg1);
		MTKCLK_disable_unprepare(clk_mfg0);
#endif
		g_MTCMOS_flag = MTCMOS_OFF;
	}
#endif

	mt_gpufreq_voltage_enable_set(0);

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_suspend: power off!\n");
#endif /*GPU_DVFS_DEBUG*/
}

static int _mtk_pm_callback_power_resume(void)
{
	struct mtk_config *config = g_config;

	if (!config) {
		pr_alert("[MALI] power_resume: mtk_config is NULL\n");
		return -1;
	}

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_resume: start power on, MTCMOS Status: %d\n", g_MTCMOS_flag);
#endif /*GPU_DVFS_DEBUG*/

	/* Step1: turn gpu pmic power */
	mt_gpufreq_voltage_enable_set(1);

#if 0
	if (g_MTCMOS_flag == MTCMOS_OFF) {
		/* Step2: turn on clocks by sequence */
		MTKCLK_prepare_enable(clk_mfg0);
		MTKCLK_prepare_enable(clk_mfg1);
		MTKCLK_prepare_enable(clk_mfg2);
		MTKCLK_prepare_enable(clk_mfg3);

		/* Step3: turn on CG */
		MTKCLK_prepare_enable(clk_mfg_cg);
		g_MTCMOS_flag = MTCMOS_ON;
	}
#endif

	/* set g_vgpu_power_on_flag to MTK_VGPU_POWER_ON */
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);

	/* resume frequency before power off */
	mtk_set_mt_gpufreq_target(g_curFreqID);

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_resume: power on!\n");
#endif /*GPU_DVFS_DEBUG*/

	return 1;
}
#endif

/**
 * For VGPU Low Power Mode On/Off
 *
*/
static void _mtk_pm_callback_power_off(void)
{
	struct mtk_config *config = g_config;

	if (!config) {
		pr_alert("[MALI] power_off: mtk_config is NULL\n");
		return;
	}

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_off: start lpm on, MTCMOS Status: %d\n", g_MTCMOS_flag);
#endif /*GPU_DVFS_DEBUG*/

	g_curFreqID = mt_gpufreq_get_cur_freq_index();
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);

	if (g_MTCMOS_flag == MTCMOS_ON) {
		mdelay(3);

#ifdef MTCMOS_READY
		MTKCLK_disable_unprepare(clk_mfg_cg);
		MTKCLK_disable_unprepare(clk_mfg3);
		MTKCLK_disable_unprepare(clk_mfg2);
		MTKCLK_disable_unprepare(clk_mfg1);
		MTKCLK_disable_unprepare(clk_mfg0);
#endif
		g_MTCMOS_flag = MTCMOS_OFF;
	}

	/* mt_gpufreq_voltage_lpm_set(1); */
	mt_gpufreq_voltage_enable_set(0);

	/* Avoid the current pulse of PMIC IC need 3 ms delay, *
	 * should follow the mt_gpufreq_voltage_enable_set(0)  *
	 */
	mdelay(3);

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_off: lpm on!\n");
#endif /*GPU_DVFS_DEBUG*/
}

static int _mtk_pm_callback_power_on(void)
{
	struct mtk_config *config = g_config;

	if (!config) {
		pr_alert("[MALI] power_on: mtk_config is NULL\n");
		return -1;
	}

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_on: start lpm off, MTCMOS Status: %d\n", g_MTCMOS_flag);
#endif /*GPU_DVFS_DEBUG*/

	/* Step1: turn gpu pmic power */
	/* mt_gpufreq_voltage_lpm_set(0); */
	mt_gpufreq_voltage_enable_set(1);

	if (g_MTCMOS_flag == MTCMOS_OFF) {
		/* Step2: turn on clocks by sequence */
		MTKCLK_prepare_enable(clk_mfg0);
		MTKCLK_prepare_enable(clk_mfg1);
		MTKCLK_prepare_enable(clk_mfg2);
		MTKCLK_prepare_enable(clk_mfg3);

		/* Step3: turn on CG */
		MTKCLK_prepare_enable(clk_mfg_cg);
		g_MTCMOS_flag = MTCMOS_ON;
	}

	/* set g_vgpu_power_on_flag to MTK_VGPU_POWER_ON */
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);

#ifdef GPU_DVFS_DEBUG
	pr_alert("[MALI] power_on: lpm off!\n");
#endif /*GPU_DVFS_DEBUG*/

	return 1;
}

/**
 * MTK internal io map function
 *
*/
static void *_mtk_of_ioremap(const char *node_name)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, node_name);

	if (node)
		return of_iomap(node, 0);

	pr_err("[MALI] cannot find [%s] of_node, please fix me\n", node_name);
	return NULL;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	_mtk_pm_callback_power_on();
	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	_mtk_pm_callback_power_off();
}

#if 0
static void pm_callback_power_suspend(struct kbase_device *kbdev)
{
#if HARD_RESET_AT_POWER_OFF
		/* Cause a GPU hard reset to test whether we have actually idled the GPU
		 * and that we properly reconfigure the GPU on power up.
		 * Usually this would be dangerous, but if the GPU is working correctly it should
		 * be completely safe as the GPU should not be active at this point.
		 * However this is disabled normally because it will most likely interfere with
		 * bus logging etc.
		 */
		KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
		kbase_os_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif
	_mtk_pm_callback_power_suspend();
}

static void pm_callback_power_resume(struct kbase_device *kbdev)
{
		_mtk_pm_callback_power_resume();
}
#endif

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback	 = NULL,
	.power_resume_callback = NULL
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
	/* Nothing needed at this stage */
	return 0;
}

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{
	struct mtk_config *config;

	if (!pdev || !kbdev) {
		pr_alert("[MALI] input parameter is NULL\n");
		return -1;
	}

	config = (struct mtk_config *)kbdev->mtk_config;
	if (!config) {
		pr_alert("[MALI] Alloc mtk_config\n");
		config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);
		if (config == NULL) {
			pr_alert("[MALI] Fail to alloc mtk_config\n");
			return -1;
		}
		g_config = kbdev->mtk_config = config;
	}

	config->mfg_register = g_MFG_base = _mtk_of_ioremap("mediatek,mfgcfg");
	if (g_MFG_base == NULL) {
		pr_alert("[MALI] Fail to remap MGF register\n");
		return -1;
	}

	config->clk_mfg0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
	if (IS_ERR(config->clk_mfg0)) {
		pr_alert("cannot get mtcmos-mfg-async\n");
		return PTR_ERR(config->clk_mfg0);
	}

	config->clk_mfg1 = devm_clk_get(&pdev->dev, "mtcmos-mfg");
	if (IS_ERR(config->clk_mfg1)) {
		pr_alert("cannot get mtcmos-mfg\n");
		return PTR_ERR(config->clk_mfg1);
	}

	config->clk_mfg2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
	if (IS_ERR(config->clk_mfg2)) {
		pr_alert("cannot get mtcmos-mfg-core0\n");
		return PTR_ERR(config->clk_mfg2);
	}

	config->clk_mfg3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
	if (IS_ERR(config->clk_mfg3)) {
		pr_alert("cannot get mtcmos-mfg-core1\n");
		return PTR_ERR(config->clk_mfg3);
	}

	config->clk_mfg_cg = devm_clk_get(&pdev->dev, "subsys-mfg-cg");
	if (IS_ERR(config->clk_mfg_cg)) {
		pr_alert("cannot get subsys-mfg-cg\n");
		return PTR_ERR(config->clk_mfg_cg);
	}

	/* Init MTCMOS flag */
	g_MTCMOS_flag = MTCMOS_OFF;

	pr_alert("xxxx clk_mfg0:%p\n", config->clk_mfg0);
	pr_alert("xxxx clk_mfg1:%p\n", config->clk_mfg1);
	pr_alert("xxxx clk_mfg2:%p\n", config->clk_mfg2);
	pr_alert("xxxx clk_mfg3:%p\n", config->clk_mfg3);
	pr_alert("xxxx clk_mfg_cg:%p\n", config->clk_mfg_cg);

	return 0;
}
