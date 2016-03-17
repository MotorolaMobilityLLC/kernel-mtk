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
#include "mali_kbase_config_platform.h"

#include <linux/of.h>
#include <linux/of_address.h>

/* mtk */
#include <platform/mtk_platform_common.h>
#include "mtk_common.h"
#include "mtk_mfg_reg.h"
#include "mt_gpufreq.h"

#ifdef ENABLE_COMMON_DVFS
#include <ged_dvfs.h>
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

/**
 * Get MFG register map
 *
*/
static int mtk_mfg_reg_map(struct kbase_device *kbdev)
{
	int err = -ENOMEM;
	struct device_node *dnode;
	const char *mfg_device_tree_name = "mediatek,mt6755-mfgsys";
	struct mtk_config *config;
    
    if (!kbdev) {
        pr_alert("MALI: input parameter is NULL \n");
        goto out_ioremap_err;
    }
    
    config = (struct mtk_config *)kbdev->mtk_config;

	dnode = of_find_compatible_node(NULL, NULL, mfg_device_tree_name);
	if (!dnode) {
        pr_alert("[Mali][mtk_mfg_reg_map] mfgsys find node failed\n");
        goto out_ioremap_err;
 	}else{
		if (!config) {
			pr_alert("[Mal][mtk_mfg_reg_map] mtk_config is NULL \n");
			goto out_ioremap_err;
		}
		config->mfg_register = of_iomap(dnode, 0);
	}
       
	if (!config->mfg_register) {
		dev_err(kbdev->dev, "[Mali][mtk_mfg_reg_map] Can't remap mfgsys register \n");
		err = -EINVAL;
		goto out_ioremap_err;
	}

	return 0;

out_ioremap_err:
	return err;

}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
    int ret;
    struct mtk_config *config;
    void __iomem *clk_mfgsys_base;
    
    if (!kbdev) {
        pr_alert("MALI: input parameter is NULL \n");
        return -1;
    }
    
    config = (struct mtk_config *)kbdev->mtk_config;
    if (!config) {
        pr_alert("MALI: mtk_config is NULL \n");
        return -1;
    }
    clk_mfgsys_base = config->mfg_register;

	mt_gpufreq_voltage_enable_set(1);
 	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_ON);

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif
    
	ret = clk_prepare_enable(config->clk_display_scp);
	if (ret) {
		pr_alert("MALI: clk_prepare_enable failed when enabling display MTCMOS");
	}
		
	ret = clk_prepare_enable(config->clk_smi_common);
	if (ret) {
		pr_alert("MALI: clk_prepare_enable failed when enabling display smi_common clock");
	}
		
	ret = clk_prepare_enable(config->clk_mfg_scp);
	if (ret) {
		pr_alert("MALI: clk_prepare_enable failed when enabling mfg MTCMOS");
	}
		
	ret = clk_prepare_enable(config->clk_mfg);
	if (ret) {
		pr_alert("MALI: clk_prepare_enable failed when enabling mfg clock");
	}
	

	MFG_WRITE32(0x7, clk_mfgsys_base+ 0x1C);

	pr_debug("MALI :[Power on] get GPU ID : 0x%x \n", kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_ID)) );
	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#define DELAY_LOOP_COUNT	100000
    
	volatile int polling_count = 100000;
	volatile int i = 0;
    struct mtk_config *config;
	void __iomem *clk_mfgsys_base;
    
    if (!kbdev) {
        pr_alert("[MALI] input parameter is NULL \n");
		return;
    }
    config = (struct mtk_config *)kbdev->mtk_config;
    clk_mfgsys_base = config->mfg_register;
    
	if (!clk_mfgsys_base) {
		pr_alert("[Mali] mfg_register find node failed\n");
	} else {

		/* 1. Delay 0.01ms before power off */
		for (i = 0; i < DELAY_LOOP_COUNT; i++);
		if (DELAY_LOOP_COUNT != i) {
			pr_alert("[MALI] power off delay error!\n");
		}

		/* 2. Polling MFG_DEBUG_REG for checking GPU idle before MTCNOW power off (0.1ms) */
		MFG_WRITE32(0x3, MFG_DEBUG_CTRL_REG(clk_mfgsys_base));

		do {

			/*
			 * 0x13000184[2]
 			 * 1'b1: bus idle
			 * 1'b0: bus busy
			*/
			if (MFG_READ32(MFG_DEBUG_STAT_REG(clk_mfgsys_base)) & MFG_BUS_IDLE_BIT) {
				break;
			}
		} while (polling_count--);

		if (0 >= polling_count) {
			pr_alert("[MALI]!!!!MFG(GPU) subsys is still BUSY!!!!!, polling_count=%d\n", polling_count);
		}
	}	
	
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

	/* Polling the MFG_DEBUG_REG for checking GPU IDLE before MTCMOS power off (0.1ms) */
	MFG_WRITE32(0x3, MFG_DEBUG_CTRL_REG(clk_mfgsys_base));
	
	do {
        /*
         * 0x13000184[2]
         * 1'b1: bus idle
         * 1'b0: bus busy
         */
		if (MFG_READ32(MFG_DEBUG_STAT_REG(clk_mfgsys_base)) & MFG_BUS_IDLE_BIT)
		{
			break;
		}
	} while (polling_count--);

	if (0 >= polling_count) {
		pr_alert("[MALI]!!!!MFG(GPU) subsys is still BUSY2!!!!!, polling_count=%d\n", polling_count);
	}

	clk_disable_unprepare(config->clk_mfg);
	clk_disable_unprepare(config->clk_mfg_scp);
	clk_disable_unprepare(config->clk_smi_common);
	clk_disable_unprepare(config->clk_display_scp);

	mt_gpufreq_voltage_enable_set(0);
#ifdef ENABLE_COMMON_DVFS    
	ged_dvfs_gpu_clock_switch_notify(0);
#endif
	mtk_set_vgpu_power_on_flag(MTK_VGPU_POWER_OFF);
    
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
    unsigned int gpu_efuse;
    extern int g_mtk_gpu_efuse_set_already;
	
    if (!pdev || !kbdev) {
        pr_alert("input parameter is NULL \n");
        return -1;
    }
    
    config = (struct mtk_config *)kbdev->mtk_config;
    if (!config) {
        pr_alert("[MALI] Alloc mtk_config\n");
        config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);
        if (NULL == config) {
            pr_alert("[MALI] Fail to alloc mtk_config \n");
            return -1;
        }
        kbdev->mtk_config = config;
    }
    
	if (0 != mtk_mfg_reg_map(kbdev)) {
		return -1;
	}
    
	config->clk_mfg = devm_clk_get(&pdev->dev, "mfg-main");
	
    if (IS_ERR(config->clk_mfg)) {
		pr_alert("cannot get mfg main clock\n");
		return PTR_ERR(config->clk_mfg);
	}
	config->clk_smi_common = devm_clk_get(&pdev->dev, "mfg-smi-common");
	if (IS_ERR(config->clk_smi_common)) {
		pr_alert("cannot get smi common clock\n");
		return PTR_ERR(config->clk_smi_common);
	}
	config->clk_mfg_scp = devm_clk_get(&pdev->dev, "mtcmos-mfg");
	if (IS_ERR(config->clk_mfg_scp)) {
		pr_alert("cannot get mtcmos mfg\n");
		return PTR_ERR(config->clk_mfg_scp);
	}
	config->clk_display_scp = devm_clk_get(&pdev->dev, "mtcmos-display");
	if (IS_ERR(config->clk_display_scp)) {
		pr_alert("cannot get mtcmos display\n");
		return PTR_ERR(config->clk_display_scp);
	}
	
	gpu_efuse =  (get_devinfo_with_index(17) >> 6)&0x01;
	pr_alert("[Mali] get_devinfo_with_index = 0x%x , gpu_efuse = 0x%x  \n", get_devinfo_with_index(17), gpu_efuse);
	if( gpu_efuse == 1 )
		kbdev->pm.debug_core_mask = (u64)1;	 // 1-core
	else
		kbdev->pm.debug_core_mask = (u64)3;	 // 2-core

	g_mtk_gpu_efuse_set_already = 1;
    
    return 0;
}
