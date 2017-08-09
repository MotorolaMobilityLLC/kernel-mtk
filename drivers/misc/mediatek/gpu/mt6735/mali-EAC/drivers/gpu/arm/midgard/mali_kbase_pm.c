/*
 *
 * (C) COPYRIGHT 2010-2015 ARM Limited. All rights reserved.
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





/**
 * @file mali_kbase_pm.c
 * Base kernel power management APIs
 */
#include <mali_kbase.h>
#include <mali_midg_regmap.h>
#include <mali_kbase_config_defaults.h>
#include <mali_kbase_instr.h>

#include <mali_kbase_pm.h>

/* MTK GPU DVFS */
#include "mt_gpufreq.h"
#include "mt_chip.h"
//#include "mach/mt_devinfo.h"

extern u32 get_devinfo_with_index(u32 index);

extern unsigned int g_power_off_gpu_freq_idx;
extern unsigned int g_power_status;


unsigned int g_current_gpu_platform_id = 0;

mtk_gpu_freq_limit_data mt6735_gpu_freq_limit_data[MTK_MT6735_GPU_LIMIT_COUNT]=
{ {2, 2, (const int[]){0,1}}, // Denali-1;   
  {3, 3, (const int[]){0,1,2}}, // Denali-3(MT6753T);  
};


unsigned int g_type_T = 0;
mtk_gpu_thd_level_offset gpu_thd_level_off[3] = 
{
  	{0,0,0}, 
	{0,0,0}, 
	{0,0,0},
};


extern unsigned int (*mtk_get_gpu_loading_fp)(void);
extern void (*mtk_boost_gpu_freq_fp)(void);
extern void (*mtk_custom_boost_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern void (*mtk_set_bottom_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern unsigned int (*mtk_custom_get_gpu_freq_level_count_fp)(void);

/* for MTK GPU boost and power limit */
extern void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB);
extern void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB);


/* gpu SODI */
extern void (*mtk_gpu_sodi_entry_fp)(void);
extern void (*mtk_gpu_sodi_exit_fp)(void);
extern void mali_SODI_begin(void);
extern void mali_SODI_exit(void);

unsigned int mtk_get_current_gpu_platform_id(void)
{
    return g_current_gpu_platform_id;
}

void _mtk_gpu_dvfs_init(void)
{
    int i;
    unsigned int iCurrentFreqCount;
	unsigned int code;
    printk(KERN_EMERG "[MALI] _mtk_gpu_dvfs_init\n");
    
    iCurrentFreqCount = mt_gpufreq_get_dvfs_table_num();
    
    // get curent platform index
    for(i=0 ; i<MTK_MT6735_GPU_LIMIT_COUNT ; i++)
    {
        if(iCurrentFreqCount == mt6735_gpu_freq_limit_data[i].actual_freq_index_count)
        {
            g_current_gpu_platform_id = i;
            break;
        }
    }
	
	// check chip id:
	code = mt_get_chip_hw_code();
	if((0x337==code) && (iCurrentFreqCount==3)) // (53) && (T)
	{   
		g_type_T = 1;
		gpu_thd_level_off[0].def_count = 1;		// high
		gpu_thd_level_off[0].max_level = 80;
		gpu_thd_level_off[0].min_level = 60;

		gpu_thd_level_off[1].def_count = 3;		// middle
		gpu_thd_level_off[1].max_level = 80;
		gpu_thd_level_off[1].min_level = 25;

		gpu_thd_level_off[2].def_count = 3;		// low
		gpu_thd_level_off[2].max_level = 45;
		gpu_thd_level_off[2].min_level = 25;		
	} 
	else
	{
		g_type_T = 0;
		gpu_thd_level_off[0].def_count = 3;
		gpu_thd_level_off[0].max_level = 45;
		gpu_thd_level_off[0].min_level = 25;

		gpu_thd_level_off[1].def_count = 3;
		gpu_thd_level_off[1].max_level = 45;
		gpu_thd_level_off[1].min_level = 25;

		gpu_thd_level_off[2].def_count = 3;
		gpu_thd_level_off[2].max_level = 45;
		gpu_thd_level_off[2].min_level = 25;	
	}
	

    // init g_custom_gpu_boost_id and g_ged_gpu_boost_id as 0
    mtk_kbase_custom_boost_gpu_freq(0);
    mtk_kbase_ged_bottom_gpu_freq(0);

    g_power_off_gpu_freq_idx = 0;//mt6735_gpu_freq_limit_data[g_current_gpu_platform_id].virtual_freq_index_count-1;
    g_power_status = 0;

}

int kbase_pm_powerup(struct kbase_device *kbdev, unsigned int flags)
{
	return kbase_hwaccess_pm_powerup(kbdev, flags);
}

void kbase_pm_halt(struct kbase_device *kbdev)
{
	kbase_hwaccess_pm_halt(kbdev);
}

void kbase_pm_context_active(struct kbase_device *kbdev)
{
	(void)kbase_pm_context_active_handle_suspend(kbdev, KBASE_PM_SUSPEND_HANDLER_NOT_POSSIBLE);
}

int kbase_pm_context_active_handle_suspend(struct kbase_device *kbdev, enum kbase_pm_suspend_handler suspend_handler)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	int c;
	int old_count;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* Trace timeline information about how long it took to handle the decision
	 * to powerup. Sometimes the event might be missed due to reading the count
	 * outside of mutex, but this is necessary to get the trace timing
	 * correct. */
	old_count = kbdev->pm.active_count;
	if (old_count == 0)
		kbase_timeline_pm_send_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);
	if (kbase_pm_is_suspending(kbdev)) {
		switch (suspend_handler) {
		case KBASE_PM_SUSPEND_HANDLER_DONT_REACTIVATE:
			if (kbdev->pm.active_count != 0)
				break;
			/* FALLTHROUGH */
		case KBASE_PM_SUSPEND_HANDLER_DONT_INCREASE:
			mutex_unlock(&kbdev->pm.lock);
			mutex_unlock(&js_devdata->runpool_mutex);
			if (old_count == 0)
				kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);
			return 1;

		case KBASE_PM_SUSPEND_HANDLER_NOT_POSSIBLE:
			/* FALLTHROUGH */
		default:
			KBASE_DEBUG_ASSERT_MSG(false, "unreachable");
			break;
		}
	}
	c = ++kbdev->pm.active_count;
	KBASE_TIMELINE_CONTEXT_ACTIVE(kbdev, c);
	KBASE_TRACE_ADD_REFCOUNT(kbdev, PM_CONTEXT_ACTIVE, NULL, NULL, 0u, c);

	/* Trace the event being handled */
	if (old_count == 0)
		kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);

	if (c == 1)
		/* First context active: Power on the GPU and any cores requested by
		 * the policy */
		kbase_hwaccess_pm_gpu_active(kbdev);

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	return 0;
}

KBASE_EXPORT_TEST_API(kbase_pm_context_active);

void kbase_pm_context_idle(struct kbase_device *kbdev)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	int c;
	int old_count;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* Trace timeline information about how long it took to handle the decision
	 * to powerdown. Sometimes the event might be missed due to reading the
	 * count outside of mutex, but this is necessary to get the trace timing
	 * correct. */
	old_count = kbdev->pm.active_count;
	if (old_count == 0)
		kbase_timeline_pm_send_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_IDLE);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	c = --kbdev->pm.active_count;
	KBASE_TIMELINE_CONTEXT_ACTIVE(kbdev, c);
	KBASE_TRACE_ADD_REFCOUNT(kbdev, PM_CONTEXT_IDLE, NULL, NULL, 0u, c);

	KBASE_DEBUG_ASSERT(c >= 0);

	/* Trace the event being handled */
	if (old_count == 0)
		kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_IDLE);

	if (c == 0) {
		/* Last context has gone idle */
		kbase_hwaccess_pm_gpu_idle(kbdev);

		/* Wake up anyone waiting for this to become 0 (e.g. suspend). The
		 * waiters must synchronize with us by locking the pm.lock after
		 * waiting */
		wake_up(&kbdev->pm.zero_active_count_wait);
	}

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);
}

KBASE_EXPORT_TEST_API(kbase_pm_context_idle);

void kbase_pm_suspend(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev);

	mutex_lock(&kbdev->pm.lock);
	KBASE_DEBUG_ASSERT(!kbase_pm_is_suspending(kbdev));
	kbdev->pm.suspending = true;
	mutex_unlock(&kbdev->pm.lock);

	/* From now on, the active count will drop towards zero. Sometimes, it'll
	 * go up briefly before going down again. However, once it reaches zero it
	 * will stay there - guaranteeing that we've idled all pm references */

	/* Suspend job scheduler and associated components, so that it releases all
	 * the PM active count references */
	kbasep_js_suspend(kbdev);

	/* Suspend any counter collection that might be happening */
	kbase_instr_hwcnt_suspend(kbdev);

	/* Wait for the active count to reach zero. This is not the same as
	 * waiting for a power down, since not all policies power down when this
	 * reaches zero. */
	wait_event(kbdev->pm.zero_active_count_wait, kbdev->pm.active_count == 0);

	/* NOTE: We synchronize with anything that was just finishing a
	 * kbase_pm_context_idle() call by locking the pm.lock below */

	kbase_hwaccess_pm_suspend(kbdev);
}

void kbase_pm_resume(struct kbase_device *kbdev)
{
	/* MUST happen before any pm_context_active calls occur */
	kbase_hwaccess_pm_resume(kbdev);

	/* Initial active call, to power on the GPU/cores if needed */
	kbase_pm_context_active(kbdev);

	/* Re-enable instrumentation, if it was previously disabled */
	kbase_instr_hwcnt_resume(kbdev);

	/* Resume any blocked atoms (which may cause contexts to be scheduled in
	 * and dependent atoms to run) */
	kbase_resume_suspended_soft_jobs(kbdev);

	/* Resume the Job Scheduler and associated components, and start running
	 * atoms */
	kbasep_js_resume(kbdev);

	/* Matching idle call, to power off the GPU/cores if we didn't actually
	 * need it and the policy doesn't want it on */
	kbase_pm_context_idle(kbdev);
}

