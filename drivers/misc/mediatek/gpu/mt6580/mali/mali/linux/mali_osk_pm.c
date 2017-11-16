/**
 * Copyright (C) 2010-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include "mali_pm.h"
#include "platform_pmm.h"

/// For MFG sub-system clock control API
#include <mach/mt_clkmgr.h> 
#include <linux/spinlock.h>

static _mali_osk_timer_t* pm_timer = NULL;
static _mali_osk_atomic_t mali_suspend_called;
static _mali_osk_atomic_t mali_pm_ref_count;
static _mali_osk_mutex_t* pm_lock;


#if MALI_LICENSE_IS_GPL
static struct workqueue_struct *mali_pm_wq = NULL;
#endif
static struct work_struct mali_pm_wq_work_handle;

static void mali_bottom_half_pm ( struct work_struct *work )
{
    _mali_osk_mutex_wait(pm_lock);     
     
    if((_mali_osk_atomic_read(&mali_pm_ref_count) == 0) &&
       (_mali_osk_atomic_read(&mali_suspend_called) == 0))
    {
        mali_pm_runtime_suspend();
        _mali_osk_atomic_inc(&mali_suspend_called);
        mali_platform_power_mode_change(MALI_POWER_MODE_DEEP_SLEEP);        
    }
     
    _mali_osk_mutex_signal(pm_lock);
}

_mali_osk_errcode_t _mali_osk_pm_delete_callback_timer(void)
{
   if (NULL != pm_timer)
   {
      _mali_osk_timer_del(pm_timer);
      pm_timer = NULL;
   }

#if MALI_LICENSE_IS_GPL
   if (mali_pm_wq)
   {
       flush_workqueue(mali_pm_wq);
   }
#else
   flush_scheduled_work();
#endif
   return _MALI_OSK_ERR_OK;
}

void _mali_pm_callback(void *arg)
{
#if MALI_LICENSE_IS_GPL
    if (mali_pm_wq)
    {
        queue_work(mali_pm_wq, &mali_pm_wq_work_handle);
    }
    else
    {
        MALI_PRINTF(("mali_pm_wq is NULL !!!\n"));
        mali_bottom_half_pm(NULL);
    }
#else
    schedule_work(&mali_pm_wq_work_handle);
#endif
}

void _mali_osk_pm_dev_enable(void)
{
	_mali_osk_atomic_init(&mali_pm_ref_count, 0);
	_mali_osk_atomic_init(&mali_suspend_called, 0);
	if (NULL == pm_timer)
	{
	    pm_timer = _mali_osk_timer_init();
	}
	
    if (NULL != pm_timer)
    {
	    _mali_osk_timer_setcallback(pm_timer, _mali_pm_callback, NULL);	
	}

    pm_lock = _mali_osk_mutex_init(_MALI_OSK_LOCKFLAG_ORDERED, 0);

#if MALI_LICENSE_IS_GPL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    mali_pm_wq = alloc_workqueue("mali_pm", WQ_UNBOUND, 0);
#else
    mali_pm_wq = create_workqueue("mali_pm");
#endif
    if(NULL == mali_pm_wq)
    {
        MALI_PRINT_ERROR(("Unable to create Mali pm workqueue\n"));
    }
#endif
    INIT_WORK( &mali_pm_wq_work_handle, mali_bottom_half_pm );
}

void _mali_osk_pm_dev_disable(void)
{
#if MALI_LICENSE_IS_GPL
    if (mali_pm_wq)
    {
        flush_workqueue(mali_pm_wq);
        destroy_workqueue(mali_pm_wq);
        mali_pm_wq = NULL;
    }
#else
    flush_scheduled_work();
#endif
	_mali_osk_atomic_term(&mali_pm_ref_count);
	_mali_osk_atomic_term(&mali_suspend_called);

    if (NULL != pm_timer)
    {
	    _mali_osk_timer_term(pm_timer);
	    pm_timer = NULL;
    }

	_mali_osk_mutex_term(pm_lock);
}

/* Can NOT run in atomic context */
_mali_osk_errcode_t _mali_osk_pm_dev_ref_add(void)
{
#ifdef CONFIG_PM_RUNTIME
	int err;
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
	err = pm_runtime_get_sync(&(mali_platform_device->dev));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	pm_runtime_mark_last_busy(&(mali_platform_device->dev));
#endif
	if (0 > err)
	{
		MALI_PRINT_ERROR(("Mali OSK PM: pm_runtime_get_sync() returned error code %d\n", err));
		return _MALI_OSK_ERR_FAULT;
	}
	_mali_osk_atomic_inc(&mali_pm_ref_count);
	MALI_DEBUG_PRINT(4, ("Mali OSK PM: Power ref taken (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));
#else /// CONFIG_PM_RUNTIME  

    _mali_osk_pm_delete_callback_timer();
	
	_mali_osk_mutex_wait(pm_lock);
	
   mali_platform_power_mode_change(MALI_POWER_MODE_ON);
   	
   if(_mali_osk_atomic_read(&mali_suspend_called))
   {	      		
		mali_pm_runtime_resume();

        _mali_osk_atomic_dec(&mali_suspend_called);
	}
	
	_mali_osk_atomic_inc(&mali_pm_ref_count);       
   
    _mali_osk_mutex_signal(pm_lock);
   
   MALI_DEBUG_PRINT(4, ("Mali OSK PM: Power ref taken (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));		  

#endif
	return _MALI_OSK_ERR_OK;
}

/* Can run in atomic context */
void _mali_osk_pm_dev_ref_dec(void)
{
#ifdef CONFIG_PM_RUNTIME
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
	_mali_osk_atomic_dec(&mali_pm_ref_count);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	pm_runtime_mark_last_busy(&(mali_platform_device->dev));
	pm_runtime_put_autosuspend(&(mali_platform_device->dev));
#else
	pm_runtime_put(&(mali_platform_device->dev));
#endif
	MALI_DEBUG_PRINT(4, ("Mali OSK PM: Power ref released (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));

#else /// CONFIG_PM_RUNTIME
      	
	if(_mali_osk_atomic_dec_return(&mali_pm_ref_count) == 0)
	{
	    if (NULL != pm_timer)
	    {
		    _mali_osk_timer_mod(pm_timer, _mali_osk_time_mstoticks(mali_pm_wq ? 15 : 3000));
		}
		else
		{
#if MALI_LICENSE_IS_GPL
            if (mali_pm_wq)
            {
                queue_work(mali_pm_wq, &mali_pm_wq_work_handle);
            }
            else
            {
                MALI_PRINTF(("mali_pm_wq is NULL !!!\n"));
                mali_bottom_half_pm(NULL);
            }
#else
            schedule_work(&mali_pm_wq_work_handle);
#endif
	    }
	}

	MALI_DEBUG_PRINT(4, ("Mali OSK PM: Power ref released (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));
#endif
}

/* Can run in atomic context */
mali_bool _mali_osk_pm_dev_ref_add_no_power_on(void)
{
#ifdef CONFIG_PM_RUNTIME
	u32 ref;
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
	pm_runtime_get_noresume(&(mali_platform_device->dev));
	ref = _mali_osk_atomic_read(&mali_pm_ref_count);
	MALI_DEBUG_PRINT(4, ("Mali OSK PM: No-power ref taken (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));
	return ref > 0 ? MALI_TRUE : MALI_FALSE;
#else
   _mali_osk_mutex_wait(pm_lock);     
	return _mali_osk_atomic_read(&mali_suspend_called) == 0 ? MALI_TRUE : MALI_FALSE;
#endif
}

/* Can run in atomic context */
void _mali_osk_pm_dev_ref_dec_no_power_on(void)
{
#ifdef CONFIG_PM_RUNTIME
	MALI_DEBUG_ASSERT_POINTER(mali_platform_device);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	pm_runtime_put_autosuspend(&(mali_platform_device->dev));
#else
	pm_runtime_put(&(mali_platform_device->dev));
#endif
	MALI_DEBUG_PRINT(4, ("Mali OSK PM: No-power ref released (%u)\n", _mali_osk_atomic_read(&mali_pm_ref_count)));
#else
   _mali_osk_mutex_signal(pm_lock);	
#endif
}

void _mali_osk_pm_dev_barrier(void)
{
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_barrier(&(mali_platform_device->dev));
#endif
}
