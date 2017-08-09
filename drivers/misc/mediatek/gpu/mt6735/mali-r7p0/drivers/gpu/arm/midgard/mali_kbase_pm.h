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
 * @file mali_kbase_pm.h
 * Power management API definitions
 */

#ifndef _KBASE_PM_H_
#define _KBASE_PM_H_

#include "mali_kbase_hwaccess_pm.h"

#define PM_ENABLE_IRQS       0x01
#define PM_HW_ISSUES_DETECT  0x02


/* MTK DVFS */
#define MTK_MT6735_GPU_LIMIT_COUNT 2

typedef struct _mtk_gpu_freq_limit_data{
    const int actual_freq_index_count;
    const int virtual_freq_index_count;
    const int *virtual_freq_index;
}mtk_gpu_freq_limit_data;

extern mtk_gpu_freq_limit_data mt6735_gpu_freq_limit_data[];

typedef struct _mtk_gpu_thd_level_offset
{
    unsigned int max_level;
	unsigned int min_level;
	unsigned int def_count;
}mtk_gpu_thd_level_offset;

/** Actions for DVFS.
 *
 * kbase_pm_get_dvfs_action will return one of these enumerated values to
 * describe the action that the DVFS system should take.
 */
enum kbase_pm_dvfs_action {
        KBASE_PM_DVFS_NOP,          /**< No change in clock frequency is requested */
        KBASE_PM_DVFS_CLOCK_UP,     /**< The clock frequency should be increased if possible */
        KBASE_PM_DVFS_CLOCK_DOWN,    /**< The clock frequency should be decreased if possible */
        KBASE_PM_DVFS_NONSENSE       /* [MTK] clear state, no need to do dvfs action */
};


/** Initialize the power management framework.
 *
 * Must be called before any other power management function
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 *
 * @return 0 if the power management framework was successfully initialized.
 */
int kbase_pm_init(struct kbase_device *kbdev);

/** Power up GPU after all modules have been initialized and interrupt handlers installed.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 *
 * @param flags     Flags to pass on to kbase_pm_init_hw
 *
 * @return 0 if powerup was successful.
 */
int kbase_pm_powerup(struct kbase_device *kbdev, unsigned int flags);

/**
 * Halt the power management framework.
 * Should ensure that no new interrupts are generated,
 * but allow any currently running interrupt handlers to complete successfully.
 * The GPU is forced off by the time this function returns, regardless of
 * whether or not the active power policy asks for the GPU to be powered off.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_halt(struct kbase_device *kbdev);

/** Terminate the power management framework.
 *
 * No power management functions may be called after this
 * (except @ref kbase_pm_init)
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_term(struct kbase_device *kbdev);

/** Increment the count of active contexts.
 *
 * This function should be called when a context is about to submit a job. It informs the active power policy that the
 * GPU is going to be in use shortly and the policy is expected to start turning on the GPU.
 *
 * This function will block until the GPU is available.
 *
 * This function ASSERTS if a suspend is occuring/has occurred whilst this is
 * in use. Use kbase_pm_contect_active_unless_suspending() instead.
 *
 * @note a Suspend is only visible to Kernel threads; user-space threads in a
 * syscall cannot witness a suspend, because they are frozen before the suspend
 * begins.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_context_active(struct kbase_device *kbdev);


/** Handler codes for doing kbase_pm_context_active_handle_suspend() */
enum kbase_pm_suspend_handler {
	/** A suspend is not expected/not possible - this is the same as
	 * kbase_pm_context_active() */
	KBASE_PM_SUSPEND_HANDLER_NOT_POSSIBLE,
	/** If we're suspending, fail and don't increase the active count */
	KBASE_PM_SUSPEND_HANDLER_DONT_INCREASE,
	/** If we're suspending, succeed and allow the active count to increase iff
	 * it didn't go from 0->1 (i.e., we didn't re-activate the GPU).
	 *
	 * This should only be used when there is a bounded time on the activation
	 * (e.g. guarantee it's going to be idled very soon after) */
	KBASE_PM_SUSPEND_HANDLER_DONT_REACTIVATE
};

/** Suspend 'safe' variant of kbase_pm_context_active()
 *
 * If a suspend is in progress, this allows for various different ways of
 * handling the suspend. Refer to @ref enum kbase_pm_suspend_handler for details.
 *
 * We returns a status code indicating whether we're allowed to keep the GPU
 * active during the suspend, depending on the handler code. If the status code
 * indicates a failure, the caller must abort whatever operation it was
 * attempting, and potentially queue it up for after the OS has resumed.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 * @param suspend_handler The handler code for how to handle a suspend that might occur
 * @return zero     Indicates success
 * @return non-zero Indicates failure due to the system being suspending/suspended.
 */
int kbase_pm_context_active_handle_suspend(struct kbase_device *kbdev, enum kbase_pm_suspend_handler suspend_handler);

/** Decrement the reference count of active contexts.
 *
 * This function should be called when a context becomes idle. After this call the GPU may be turned off by the power
 * policy so the calling code should ensure that it does not access the GPU's registers.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_context_idle(struct kbase_device *kbdev);

/**
 * Suspend the GPU and prevent any further register accesses to it from Kernel
 * threads.
 *
 * This is called in response to an OS suspend event, and calls into the various
 * kbase components to complete the suspend.
 *
 * @note the mechanisms used here rely on all user-space threads being frozen
 * by the OS before we suspend. Otherwise, an IOCTL could occur that powers up
 * the GPU e.g. via atom submission.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_suspend(struct kbase_device *kbdev);

/**
 * Resume the GPU, allow register accesses to it, and resume running atoms on
 * the GPU.
 *
 * This is called in response to an OS resume event, and calls into the various
 * kbase components to complete the resume.
 *
 * @param kbdev     The kbase device structure for the device (must be a valid pointer)
 */
void kbase_pm_resume(struct kbase_device *kbdev);

/**
 * kbase_pm_vsync_callback - vsync callback
 *
 * @buffer_updated: 1 if a new frame was displayed, 0 otherwise
 * @data: Pointer to the kbase device as returned by kbase_find_device()
 *
 * Callback function used to notify the power management code that a vsync has
 * occurred on the display.
 */
void kbase_pm_vsync_callback(int buffer_updated, void *data);

void proc_mali_register(void);
void proc_mali_unregister(void);

u32 kbasep_get_gl_utilization(void);
u32 kbasep_get_cl_js0_utilization(void);
u32 kbasep_get_cl_js1_utilization(void);

enum kbase_pm_dvfs_action mtk_get_dvfs_action(void);

void mtk_clear_dvfs_action(void);
int mtk_get_dvfs_enabled(void);
unsigned int mtk_get_current_gpu_platform_id(void);

void mtk_gpu_input_boost_CB(unsigned int ui32BoostFreqID);
void mtk_gpu_power_limit_CB(unsigned int ui32LimitFreqID);

int mtk_get_input_boost_enabled(void);

void mtk_kbase_boost_gpu_freq(void);

void mtk_kbase_custom_boost_gpu_freq(unsigned int ui32FreqLevel);
void mtk_kbase_ged_bottom_gpu_freq(unsigned int ui32FreqLevel);
unsigned int mtk_kbase_custom_get_gpu_freq_level_count(void);

int mtk_get_dvfs_freq(void);
int mtk_get_dvfs_threshold_max(void);
int mtk_get_dvfs_threshold_min(void);
int mtk_get_dvfs_deferred_count(void);

void mtk_get_touch_boost_flag(int *touch_boost_flag, int *touch_boost_id);
void mtk_set_touch_boost_flag(int boost_id);
void mtk_clear_touch_boost_flag(void);

#endif				/* _KBASE_PM_H_ */
