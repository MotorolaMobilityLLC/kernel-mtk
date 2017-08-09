/*
 *
 * (C) COPYRIGHT 2011-2015 ARM Limited. All rights reserved.
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
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_cpu_vexpress.h"
#include "mali_kbase_config_platform.h"

#ifdef MTK_mt6797
#include <../drivers/misc/mediatek/power/mt6797/fan53555.h>
#endif

#define HARD_RESET_AT_POWER_OFF 0

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

#define MTKCLK_prepare_enable(clk) \
    if (kbdev->clk && clk_prepare_enable(kbdev->clk)) \
        pr_alert("MALI: clk_prepare_enable failed when enabling " #clk );

#define MTKCLK_disable_unprepare(clk) \
    if (kbdev->clk) clk_disable_unprepare(kbdev->clk);

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);
	MTKCLK_prepare_enable(clk_mfg_core2);
	MTKCLK_prepare_enable(clk_mfg_core3);
	MTKCLK_prepare_enable(clk_mfg_main);

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
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

	MTKCLK_disable_unprepare(clk_mfg_main);
	MTKCLK_disable_unprepare(clk_mfg_core3);
	MTKCLK_disable_unprepare(clk_mfg_core2);
	MTKCLK_disable_unprepare(clk_mfg_core1);
	MTKCLK_disable_unprepare(clk_mfg_core0);
	MTKCLK_disable_unprepare(clk_mfg);
	MTKCLK_disable_unprepare(clk_mfg_async);
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
#ifdef MTK_mt6797
	volatile void * g_ldo_base = NULL;

	g_ldo_base = ioremap_nocache(0x10001000, 0x1000);

	if (!IS_ERR_OR_NULL((void*)g_ldo_base))
	{
		/* MTK: init LDO voltage
		 * vgpu_sram=1.2v
		 */
		*(volatile uint32_t*)(g_ldo_base+0xfc0) = (uint32_t)0x0f0f0f0f;
		*(volatile uint32_t*)(g_ldo_base+0xfc4) = (uint32_t)0x0f0f0f0f;
		*(volatile uint32_t*)(g_ldo_base+0xfbc) = (uint32_t)0xff;

		iounmap(g_ldo_base);
	}

	fan53555_config_interface(0x0, 0xA7, 0xff, 0x0);
	fan53555_config_interface(0x1, 0xA7, 0xff, 0x0);
#endif

	return 0;
}
