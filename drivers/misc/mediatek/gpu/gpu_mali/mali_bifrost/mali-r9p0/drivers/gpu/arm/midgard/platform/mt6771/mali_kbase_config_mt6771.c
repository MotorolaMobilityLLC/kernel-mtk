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
#include <mtk_gpu_log.h>

#define MALI_TAG						"[GPU/MALI]"
#define mali_pr_info(fmt, args...)		pr_info(MALI_TAG"[INFO]"fmt, ##args)
#define mali_pr_debug(fmt, args...)		pr_debug(MALI_TAG"[DEBUG]"fmt, ##args)

DEFINE_MUTEX(g_mfg_lock);

static void *g_MFG_base;
static void *g_DBGAPB_base;
static void *g_INFRA_AO_base;
static void *g_TOPCKGEN_base;
static int g_curFreqID;

/**
 * For GPU idle check
 */
static void _mtk_check_MFG_idle(void)
{
	u32 val;

	/* MFG_QCHANNEL_CON (0x130000b4) bit [1:0] = 0x1 */
	writel(0x00000001, g_MFG_base + 0xb4);
	/* mali_pr_debug("@%s: 0x130000b4 val = 0x%x\n", __func__, readl(g_MFG_base + 0xb4)); */

	if (mt_gpufreq_get_MMU_AS_ACTIVE() == 1) {
		/* Step 1: enable monitor, write address 0d0a_0044 bit[0] = 1 */
		val = readl(g_DBGAPB_base + 0xa0044);
		writel((val & ~(0x1)) | 0x1, g_DBGAPB_base + 0xa0044);
		/* Step 2: check address 1000_0188 bit[24] should be “0”  ;  if not 0, please set this bit to 0 */
		val = readl(g_TOPCKGEN_base + 0x188);
		writel(val & ~(0x1000000), g_TOPCKGEN_base + 0x188);

		/* for infra2mfg gals */
		mali_pr_info("@%s: infra2mfg gals ...\n", __func__);
		/* Step 6: write address 0d0a_0044 bit[8:0] = 9’b000111101 ; // (7'd30) <<1 ; */
		val = readl(g_DBGAPB_base + 0xa0044);
		writel((val & ~(0x1ff)) | 0x3d, g_DBGAPB_base + 0xa0044);
		mali_pr_info("@%s: 0x0d0a0044 val = 0x%x\n", __func__, readl(g_DBGAPB_base + 0xa0044));
		mali_pr_info("@%s: 0x0d0a0040 val = 0x%x\n", __func__, readl(g_DBGAPB_base + 0xa0040));
		/* Step 7: write address 0d0a_0044 bit[8:0] = 9’b000111111 ;  // (7'd31) <<1 ; */
		val = readl(g_DBGAPB_base + 0xa0044);
		writel((val & ~(0x1ff)) | 0x3f, g_DBGAPB_base + 0xa0044);
		mali_pr_info("@%s: 0x0d0a0044 val = 0x%x\n", __func__, readl(g_DBGAPB_base + 0xa0044));
		mali_pr_info("@%s: 0x0d0a0040 val = 0x%x\n", __func__, readl(g_DBGAPB_base + 0xa0040));

		/* bus protect related */
		mali_pr_info("@%s: bus protect related ...\n", __func__);
		/* INFRA_TOPAXI_PROTECTEN */
		mali_pr_info("@%s: 0x10001220 val = 0x%x\n", __func__, readl(g_INFRA_AO_base + 0x220));
		/* INFRA_TOPAXI_PROTECTEN_STA1 */
		mali_pr_info("@%s: 0x10001228 val = 0x%x\n", __func__, readl(g_INFRA_AO_base + 0x228));
		/* INFRA_TOPAXI_PROTECTEN_1 */
		mali_pr_info("@%s: 0x10001250 val = 0x%x\n", __func__, readl(g_INFRA_AO_base + 0x250));
		/* INFRA_TOPAXI_PROTECTEN_STA1_1 */
		mali_pr_info("@%s: 0x10001258 val = 0x%x\n", __func__, readl(g_INFRA_AO_base + 0x258));
		/* INFRA_TOPAXI_PROTECTSTA0_1 */
		mali_pr_info("@%s: 0x10001254 val = 0x%x\n", __func__, readl(g_INFRA_AO_base + 0x254));

		mt_gpufreq_set_MMU_AS_ACTIVE(0);
	}


	/* set register MFG_DEBUG_SEL (0x13000180) bit [7:0] = 0x03 */
	writel(0x00000003, g_MFG_base + 0x180);
	/* mali_pr_debug("@%s: 0x13000180 val = 0x%x\n", __func__, readl(g_MFG_base + 0x180)); */

	/* polling register MFG_DEBUG_TOP (0x13000188) bit 2 = 0x1 */
	/* => 1 for GPU (BUS) idle, 0 for GPU (BUS) non-idle */
	/* do not care about 0x13000184 */
	do {
		val = readl(g_MFG_base + 0x184);
		val = readl(g_MFG_base + 0x188);
		mali_pr_debug("@%s: 0x13000188 val = 0x%x\n", __func__, val);
	} while ((val & 0x4) != 0x4);
}

static int pm_callback_power_on_nolock(struct kbase_device *kbdev)
{
	if (mtk_get_vgpu_power_on_flag() == MTK_VGPU_POWER_ON)
		return 0;

	mali_pr_debug("@%s: power on ...\n", __func__);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x1 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn on GPU MTCMOS */
	mt_gpufreq_enable_MTCMOS();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x2 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* enable Clock Gating */
	mt_gpufreq_enable_CG();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x3 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Write 1 into 0x13000130 bit 0 to enable timestamp register (TIMESTAMP).*/
	/* TIMESTAMP will be used by clGetEventProfilingInfo.*/
	writel(0x00000001, g_MFG_base + 0x130);

	/* merge_w off */
	writel(0x0, g_MFG_base + 0x8b0);
	/* merge_r off */
	writel(0x0, g_MFG_base + 0x8a0);
	/* axi1to2 off */
	writel(0x0, g_MFG_base + 0x8f4);

	GPULOG("merge_w: 0x%08x merge_r: 0x%08x axi1to2: 0x%08x",
			readl(g_MFG_base + 0x8b0),
			readl(g_MFG_base + 0x8a0),
			readl(g_MFG_base + 0x8f4));

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x4 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Resume frequency before power off */
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);
	mtk_set_mt_gpufreq_target(g_curFreqID);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x5 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	return 1;
}

static void pm_callback_power_off_nolock(struct kbase_device *kbdev)
{
	if (mtk_get_vgpu_power_on_flag() == MTK_VGPU_POWER_OFF)
		return;

	mali_pr_debug("@%s: power off ...\n", __func__);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x6 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(0);
#endif

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x7 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);
	g_curFreqID = mtk_get_ged_dvfs_last_commit_idx();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x8 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	_mtk_check_MFG_idle();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0x9 | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* disable Clock Gating */
	mt_gpufreq_disable_CG();

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xA | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn off GPU MTCMOS */
	mt_gpufreq_disable_MTCMOS();
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 0;

	mutex_lock(&g_mfg_lock);
	ret = pm_callback_power_on_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);
	pm_callback_power_off_nolock(kbdev);
	mutex_unlock(&g_mfg_lock);
}

void pm_callback_power_suspend(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);

	mali_pr_debug("@%s: power suspend ...\n", __func__);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xB | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	pm_callback_power_off_nolock(kbdev);

	/* Turn off GPU PMIC Buck */
	mt_gpufreq_voltage_enable_set(0);

	mutex_unlock(&g_mfg_lock);
}

void pm_callback_power_resume(struct kbase_device *kbdev)
{
	mutex_lock(&g_mfg_lock);

	mali_pr_debug("@%s: power resume ...\n", __func__);

#ifdef MT_GPUFREQ_SRAM_DEBUG
	aee_rr_rec_gpu_dvfs_status(0xC | (aee_rr_curr_gpu_dvfs_status() & 0xF0));
#endif

	/* Turn on GPU PMIC Buck */
	mt_gpufreq_voltage_enable_set(1);

	pm_callback_power_on_nolock(kbdev);

	mutex_unlock(&g_mfg_lock);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
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

/**
 * MTK internal io map function
 *
 */
static void *_mtk_of_ioremap(const char *node_name, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, node_name);

	if (node)
		return of_iomap(node, idx);

	mali_pr_info("@%s: cannot find [%s] of_node\n", __func__, node_name);

	return NULL;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{

	if (!pdev || !kbdev) {
		mali_pr_info("@%s: input parameter is NULL\n", __func__);
		return -1;
	}

	g_MFG_base = _mtk_of_ioremap("mediatek,mfgcfg", 0);
	if (g_MFG_base == NULL) {
		mali_pr_info("@%s: fail to remap MGFCFG register\n", __func__);
		return -1;
	}

	g_INFRA_AO_base = _mtk_of_ioremap("mediatek,mfgcfg", 1);
	if (g_INFRA_AO_base == NULL) {
		mali_pr_info("@%s: fail to remap INFRA_AO register\n", __func__);
		return -1;
	}

	g_DBGAPB_base = _mtk_of_ioremap("mediatek,mfgcfg", 4);
	if (g_DBGAPB_base == NULL) {
		mali_pr_info("@%s: fail to remap DBGAPB register\n", __func__);
		return -1;
	}

	g_TOPCKGEN_base = _mtk_of_ioremap("mediatek,topckgen", 0);
	if (g_TOPCKGEN_base == NULL) {
		mali_pr_info("@%s: fail to remap TOPCKGEN register\n", __func__);
		return -1;
	}

	mali_pr_info("@%s: initialize successfully\n", __func__);

	return 0;
}
