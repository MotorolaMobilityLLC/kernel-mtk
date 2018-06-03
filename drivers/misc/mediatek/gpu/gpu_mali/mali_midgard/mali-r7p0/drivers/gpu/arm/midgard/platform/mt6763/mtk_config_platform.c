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

/* mtk */
#include <platform/mtk_platform_common.h>
#include <mtk_mali_config.h>

#include "mtk_common.h"
#include "mtk_mfg_reg.h"

/* efuse */
#include "mtk_devinfo.h"

#ifdef ENABLE_COMMON_DVFS
#include <ged_dvfs.h>
#include <mtk_gpufreq.h>
#endif

#ifdef ENABLE_VCORE_DVFS_CONTROL
#include <mtk_vcorefs_manager.h>
#endif

#ifdef ENABLE_AXI_1TO2_CONTROL
#include "mtk_dramc.h"
#endif

struct mtk_config *g_config;
volatile void *g_MFG_base;

/* Record freq id before power off */
int g_curFreqID;
static int _mtk_pm_callback_power_on(void);

#ifdef ENABLE_MFG_PERF

extern bool (*mtk_enable_gpu_perf_monitor_fp)(bool enable);

bool _mtk_enable_gpu_perf_monitor(bool enable)
{
	struct mtk_config *config = g_config;

	if (config) {
		config->mfg_perf_enabled = enable;
		if (true == enable) {
			 _mtk_pm_callback_power_on();
		}
		return true;
	}

	return false;
}
#endif

/**
 * MTK internal power on procedure
 *
*/
static int _mtk_pm_callback_power_on(void)
{
	struct mtk_config *config = g_config;

	if (!config) {
		pr_alert("MALI: mtk_config is NULL\n");
		return -1;
	}
#ifdef ENABLE_COMMON_DVFS
	/* Step1: turn gpu pmic power */
	mt_gpufreq_voltage_enable_set(1);
#endif

	/* Step2: turn on clocks by sequence
	 * MFG_ASYNC -> MFG -> CORE 0 -> CORE 1
	*/
	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);

	/* Step3: turn on CG */
	MTKCLK_prepare_enable(clk_mfg_main);

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);
	/* Resume frequency before power off */
	mtk_set_mt_gpufreq_target(g_curFreqID);

	/* MFG register control */
	{
#ifdef ENABLE_MFG_PERF
		if (true == config->mfg_perf_enabled) {
			MTKCLK_prepare_enable(clk_mfg_f52m_sel);

			MFG_write32(MFG_PERF_EN_00, 0xFFFFFFFF);
			MFG_write32(MFG_PERF_EN_01, 0xFFFFFFFF);
			MFG_write32(MFG_PERF_EN_02, 0xFFFFFFFF);
			MFG_write32(MFG_PERF_EN_03, 0xFFFFFFFF);
			MFG_write32(MFG_PERF_EN_04, 0xFFFFFFFF);
		}
#endif
	}

#ifdef ENABLE_VCORE_DVFS_CONTROL
	if (vcorefs_request_dvfs_opp(KIR_GPU, OPPI_LOW_PWR) < -1 )
		pr_alert("[MALI]Fail to Vote LPM\n");
#endif

#ifdef ENABLE_AXI_1TO2_CONTROL
	/**
	 * LP3 DRAM, GPU access EMI only by M6 for performance efficiency
	 * 0x8f0[0] = 0, 0x8f0[4] = 0, 0x8f4[31:0] = 0x0000_0000
	 */
	if (get_ddr_type() == TYPE_LPDDR3) {
		MFG_write32(MFG_1to2_CFG_CON_00, MFG_read32(MFG_1to2_CFG_CON_00) & 0xFFFFFFEE);
		MFG_write32(MFG_1to2_CFG_CON_01, MFG_read32(MFG_1to2_CFG_CON_01) & 0x0);
		pr_debug("[MALI]CFG_CON_00(%x) CFG_CON_01(%x)\n",
		MFG_read32(MFG_1to2_CFG_CON_00), MFG_read32(MFG_1to2_CFG_CON_01));
	}
#endif

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	return 1;
}

/**
 * MTK internal power off procedure
 *
*/
static void _mtk_pm_callback_power_off(void)
{
	int polling_retry = 100000;
	struct mtk_config *config = g_config;
#ifdef ENABLE_COMMON_DVFS
	int dvfs_cnt = 0;
#endif

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(0);
#endif

#ifdef ENABLE_VCORE_DVFS_CONTROL
	vcorefs_request_dvfs_opp(KIR_GPU, OPPI_UNREQ);
#endif

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);

	/* polling mfg idle */
	MFG_write32(MFG_DEBUG_SEL, 0x3);
	while ((MFG_read32(MFG_DEBUG_A) & MFG_DEBUG_IDEL) != MFG_DEBUG_IDEL && --polling_retry) {
		udelay(1);
	}

	if (polling_retry <= 0) {
		pr_MTK_err("[dump] polling fail: idle rem:%d - MFG_DBUG_A=%x\n", polling_retry, MFG_read32(MFG_DEBUG_A));
	}

	/* Turn off clock by sequence */
	MTKCLK_disable_unprepare(clk_mfg_main);
	MTKCLK_disable_unprepare(clk_mfg_core1);
	MTKCLK_disable_unprepare(clk_mfg_core0);
	MTKCLK_disable_unprepare(clk_mfg);
	MTKCLK_disable_unprepare(clk_mfg_async);
#ifdef ENABLE_MFG_PERF
	if (true == config->mfg_perf_enabled) {
		MTKCLK_disable_unprepare(clk_mfg_f52m_sel);
	}
#endif

#ifdef ENABLE_COMMON_DVFS
	g_curFreqID = mt_gpufreq_get_cur_freq_index();
	dvfs_cnt = mt_gpufreq_get_dvfs_table_num();
	/* Fix to 0.6V when power off by request, no power off */
	mt_gpufreq_target(dvfs_cnt - 1);
#endif

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

	pr_MTK_err("cannot find [%s] of_node, please fix me\n", node_name);
	return NULL;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	_mtk_pm_callback_power_on();
	return 0;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	_mtk_pm_callback_power_off();
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
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
		pr_alert("input parameter is NULL \n");
		return -1;
	}

	config = (struct mtk_config *)kbdev->mtk_config;
	if (!config) {
		pr_alert("[MALI] Alloc mtk_config\n");
		config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);
		if (NULL == config) {
			pr_alert("[MALI] Fail to alloc mtk_config\n");
			return -1;
		}
		g_config = kbdev->mtk_config = config;
	}

	config->mfg_register = g_MFG_base = _mtk_of_ioremap("mediatek,g3d_config");
	if (g_MFG_base == NULL) {
		pr_alert("[MALI] Fail to remap MGF register\n");
		return -1;
	}

	config->clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
	if (IS_ERR(config->clk_mfg)) {
		pr_alert("cannot get mtcmos mfg\n");
		return PTR_ERR(config->clk_mfg);
	}
	config->clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
	if (IS_ERR(config->clk_mfg_async)) {
		pr_alert("cannot get mtcmos mfg-async\n");
		return PTR_ERR(config->clk_mfg_async);
	}
	config->clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
	if (IS_ERR(config->clk_mfg_core0)) {
		pr_alert("cannot get mtcmos-mfg-core0\n");
		return PTR_ERR(config->clk_mfg_core0);
	}
	config->clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
	if (IS_ERR(config->clk_mfg_core1)) {
		pr_alert("cannot get mtcmos-mfg-core1\n");
		return PTR_ERR(config->clk_mfg_core1);
	}
	config->clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");
	if (IS_ERR(config->clk_mfg_main)) {
		pr_alert("cannot get cg mfg-main\n");
		return PTR_ERR(config->clk_mfg_main);
	}
#ifdef ENABLE_MFG_PERF
	config->clk_mfg_f52m_sel = devm_clk_get(&pdev->dev, "mfg-f52m-sel");
	if (IS_ERR(config->clk_mfg_f52m_sel)) {
		pr_alert("cannot get cg mfg-f52m-sel\n");
		return PTR_ERR(config->clk_mfg_f52m_sel);
	}

	mtk_enable_gpu_perf_monitor_fp = _mtk_enable_gpu_perf_monitor;
	config->mfg_perf_enabled = false;
#endif

	{
		/*efuse
		* Spare 3 (0x1020604c). bit 6
		* MFG_DISABLE_SC1 (0: MP2, 1: MP1)
		*/
		const unsigned int spare3_idx = 7;
		unsigned int gpu_efuse;
		extern int g_mtk_gpu_efuse_set_already;

		gpu_efuse =  (get_devinfo_with_index(spare3_idx) >> 6) & 0x01;
		pr_alert("[Mali] get_devinfo_with_index = 0x%x , gpu_efuse = 0x%x  \n",
		get_devinfo_with_index(spare3_idx), gpu_efuse);

		if (gpu_efuse == 1) {
			kbdev->pm.debug_core_mask = (u64)1;
			g_mtk_gpu_efuse_set_already = 1;
		}

	}

	dev_MTK_err(kbdev->dev, "xxxx clk_mfg:%p\n", config->clk_mfg);
	dev_MTK_err(kbdev->dev, "xxxx clk_mfg_async:%p\n", config->clk_mfg_async);
	dev_MTK_err(kbdev->dev, "xxxx clk_mfg_core0:%p\n", config->clk_mfg_core0);
	dev_MTK_err(kbdev->dev, "xxxx clk_mfg_core1:%p\n", config->clk_mfg_core1);
	dev_MTK_err(kbdev->dev, "xxxx clk_mfg_main:%p\n", config->clk_mfg_main);

	return 0;
}
