/*
 *
 * (C) COPYRIGHT 2011-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_cpu_mt6771.h"
#include "mali_kbase_config_platform.h"
#include "platform/mtk_platform_common.h"
#include "ged_dvfs.h"
#include "mtk_gpufreq.h"

#define HARD_RESET_AT_POWER_OFF 0
/* #define GPU_DVFS_DEBUG */

#ifdef GPU_DVFS_DEBUG
#define MFG_DEBUG(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#else
#define MFG_DEBUG(...)
#endif

DEFINE_MUTEX(g_mfg_lock);

volatile void *g_MFG_base;
int g_curFreqID;

/**
 * For GPU idle check
 */
static void _mtk_check_MFG_idle(void)
{
	u32 val;

	/* MFG_QCHANNEL_CON (0x130000b4) bit [1:0] = 0x1 */
	writel(0x00000001, g_MFG_base + 0xb4);
	MFG_DEBUG("[MALI] 0x130000b4 val = 0x%x\n", readl(g_MFG_base + 0xb4));

	/* set register MFG_DEBUG_SEL (0x13000180) bit [7:0] = 0x03 */
	writel(0x00000003, g_MFG_base + 0x180);
	MFG_DEBUG("[MALI] 0x13000180 val = 0x%x\n", readl(g_MFG_base + 0x180));

	/* polling register MFG_DEBUG_TOP (0x13000188) bit 2 = 0x1 */
	/* => 1 for GPU (BUS) idle, 0 for GPU (BUS) non-idle */
	/* do not care about 0x13000184 */
	do {
		val = readl(g_MFG_base + 0x184);
		val = readl(g_MFG_base + 0x188);
		MFG_DEBUG("[MALI] 0x13000188 val = 0x%x\n", val);
	} while ((val & 0x4) != 0x4);
}

/**
 * For VGPU Low Power Mode On/Off
*/
static void _mtk_pm_callback_power_off(void)
{
	mutex_lock(&g_mfg_lock);

	MFG_DEBUG("[MALI] power off ....\n");

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(0);
#endif

	g_curFreqID = mt_gpufreq_get_cur_freq_index();
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);

	_mtk_check_MFG_idle();

	/* Disable clock gating */
	mt_gpufreq_disable_CG();

	/* Turn off GPU MTCMOS by sequence */
	/* mt_gpufreq_disable_MTCMOS(); */

	/* Turn off GPU PMIC Buck */
	/* mt_gpufreq_voltage_enable_set(0); */

	MFG_DEBUG("[MALI] power off successfully\n");

	mutex_unlock(&g_mfg_lock);
}

static int _mtk_pm_callback_power_on(void)
{
	mutex_lock(&g_mfg_lock);

	MFG_DEBUG("[MALI] power on ....\n");

	/* Turn on GPU PMIC Buck */
	mt_gpufreq_voltage_enable_set(1);

	/* Turn on GPU MTCMOS by sequence */
	mt_gpufreq_enable_MTCMOS();

	/* Enable clock gating */
	mt_gpufreq_enable_CG();

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);

	/* Write 1 into 0x13000130 bit 0 to enable timestamp register (TIMESTAMP).*/
	/* TIMESTAMP will be used by clGetEventProfilingInfo.*/
	writel(0x00000001, g_MFG_base + 0x130);

	/* Resume frequency before power off */
	mtk_set_mt_gpufreq_target(g_curFreqID);

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	MFG_DEBUG("[MALI] power on successfully\n");

	mutex_unlock(&g_mfg_lock);

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

	MFG_DEBUG("[MALI] cannot find [%s] of_node, please fix me\n", node_name);
	return NULL;
}


static int pm_callback_power_on(struct kbase_device *kbdev)
{
	_mtk_pm_callback_power_on();
	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	if (!mtk_kbase_is_gpu_always_on())
		_mtk_pm_callback_power_off();
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL,
	.mtk_power_suspend_callback = NULL,
	.mtk_power_resume_callback = NULL
};

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
		.start = 0xFC010000,
		.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

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

	if (!pdev || !kbdev) {
		MFG_DEBUG("[MALI] input parameter is NULL\n");
		return -1;
	}

	g_MFG_base = _mtk_of_ioremap("mediatek,mfgcfg");
	if (g_MFG_base == NULL) {
		MFG_DEBUG("[MALI] Fail to remap MGF register\n");
		return -1;
	}

	return 0;
}
