/**
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2015 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_osk_pm.c
 * Implementation of the callback functions from common power management
 */

#include <linux/sched.h>

#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif /* CONFIG_PM_RUNTIME */
#include <linux/platform_device.h>
#include <linux/version.h>
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_kernel_linux.h"

/* Can NOT run in atomic context */
_mali_osk_errcode_t _mali_osk_pm_dev_ref_get_sync(void)
{
#ifdef CONFIG_PM_RUNTIME
	int err;
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
	err = pm_runtime_get_sync(&(mali_platform_device->dev));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	pm_runtime_mark_last_busy(&(mali_platform_device->dev));
#endif
	if (0 > err) {
		MALI_PRINT_ERROR(("Mali OSK PM: pm_runtime_get_sync() returned error code %d\n", err));
		return _MALI_OSK_ERR_FAULT;
	}
#endif
	return _MALI_OSK_ERR_OK;
}

/* Can run in atomic context */
_mali_osk_errcode_t _mali_osk_pm_dev_ref_get_async(void)
{
#ifdef CONFIG_PM_RUNTIME
	int err;
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
	err = pm_runtime_get(&(mali_platform_device->dev));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	pm_runtime_mark_last_busy(&(mali_platform_device->dev));
#endif
	if (0 > err && -EINPROGRESS != err) {
		MALI_PRINT_ERROR(("Mali OSK PM: pm_runtime_get() returned error code %d\n", err));
		return _MALI_OSK_ERR_FAULT;
	}
#endif
	return _MALI_OSK_ERR_OK;
}


/* Can run in atomic context */
void _mali_osk_pm_dev_ref_put(void)
{
#ifdef CONFIG_PM_RUNTIME
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	pm_runtime_mark_last_busy(&(mali_platform_device->dev));
	pm_runtime_put_autosuspend(&(mali_platform_device->dev));
#else
	pm_runtime_put(&(mali_platform_device->dev));
#endif
#endif
}

void _mali_osk_pm_dev_barrier(void)
{
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_barrier(&(mali_platform_device->dev));
#endif
}
