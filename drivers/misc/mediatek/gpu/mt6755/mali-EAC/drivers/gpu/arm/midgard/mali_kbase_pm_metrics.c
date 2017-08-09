/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
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
 * @file mali_kbase_pm_metrics.c
 * Metrics for power management
 */

#include <mali_kbase.h>
#include <mali_kbase_pm.h>
#include <mali_kbase_config_defaults.h>

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
//#define NO_DVFS_FOR_BRINGUP

#include <linux/of.h>
#include <linux/of_address.h>
#if KBASE_PM_EN

/* When VSync is being hit aim for utilisation between 70-90% */
#define KBASE_PM_VSYNC_MIN_UTILISATION          30//70
#define KBASE_PM_VSYNC_MAX_UTILISATION          50//90
/* Otherwise aim for 10-40% */
#define KBASE_PM_NO_VSYNC_MIN_UTILISATION       10
#define KBASE_PM_NO_VSYNC_MAX_UTILISATION       40

#define KBASE_TIMER_THRESHOLD 3000
#ifdef CONFIG_MALI_MIDGARD_DVFS

int g_current_sample_gl_utilization = 0;
int g_current_sample_cl_utilization[2] = {0};

/* MTK GPU DVFS */
#include "mt_gpufreq.h"
#include "random.h"

int g_dvfs_enabled = 1;
int g_input_boost_enabled = 1;
enum kbase_pm_dvfs_action g_current_action = KBASE_PM_DVFS_NOP;
int g_dvfs_freq = DEFAULT_PM_DVFS_FREQ;
int g_dvfs_threshold_max = KBASE_PM_VSYNC_MAX_UTILISATION;
int g_dvfs_threshold_min = KBASE_PM_VSYNC_MIN_UTILISATION;
int g_dvfs_deferred_count = 4;
#endif // CONFIG_MALI_MIDGARD_DVFS
int g_touch_boost_flag = 0;
int g_touch_boost_id = 0;

struct timeval g_tv_timer_start;

static enum hrtimer_restart dvfs_callback(struct hrtimer *timer);

int g_vgpu_power_on_flag = 0;   // on:1, off:0
int g_power_on_freq_flag = 0;
int g_power_on_freq_id = 0;
void __iomem *g_clk_topck_base = 0x0;
void __iomem *gb_DFP_base = 0x0;


void mali_SODI_begin(void)
{
	  struct list_head *entry;
	  const struct list_head *kbdev_list;    
	  kbdev_list = kbase_dev_list_get();
	  
	  list_for_each(entry, kbdev_list) 
	  {
	  	struct kbase_device *kbdev = NULL;
	  	kbdev = list_entry(entry, struct kbase_device, entry);	  	
	  	
	  	if (MALI_TRUE == kbdev->pm.metrics.timer_active)
	  	{
				 unsigned long flags;	
				
				 spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
				 kbdev->pm.metrics.timer_active = MALI_FALSE;				
				 spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
				 
				 hrtimer_cancel(&kbdev->pm.metrics.timer);
	  	}
	  }	  
	  kbase_dev_list_put(kbdev_list);	  	  
}
KBASE_EXPORT_TEST_API(mali_SODI_begin);


void mali_SODI_exit(void)
{	  
	  struct list_head *entry;
	  const struct list_head *kbdev_list;	  	    
	  struct timeval tv_timer_end;
      long timer_time_elapse;	
      
	  kbdev_list = kbase_dev_list_get();	    
	  list_for_each(entry, kbdev_list) 
	  {
	  	 struct kbase_device *kbdev = NULL;
	  	 kbdev = list_entry(entry, struct kbase_device, entry);	  	

	  	 if((MALI_FALSE == kbdev->pm.metrics.timer_active) )
	  	 {		
	  	 	 unsigned long flags;	
	  	 	 
			 	 spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
  		 	 kbdev->pm.metrics.timer_active = MALI_TRUE;
  		 	 spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

             // exit SODI, calculate the length of timer before SODI cancelled the timer.
             do_gettimeofday(&tv_timer_end);
             timer_time_elapse = (tv_timer_end.tv_sec - g_tv_timer_start.tv_sec)*1000000 + (tv_timer_end.tv_usec - g_tv_timer_start.tv_usec);


             if( timer_time_elapse > (mtk_get_dvfs_freq()*1000 - KBASE_TIMER_THRESHOLD) )
             {
                 // remain time is too short, calcute loading immediately
                 dvfs_callback(&kbdev->pm.metrics.timer);
             }
             else if( (timer_time_elapse <= (mtk_get_dvfs_freq()*1000 - KBASE_TIMER_THRESHOLD)) && (timer_time_elapse > 0))
             {
                 hrtimer_init(&kbdev->pm.metrics.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
                 kbdev->pm.platform_dvfs_frequency = mtk_get_dvfs_freq() - timer_time_elapse/1000;   /* set timer length to: original - elasped time */
                 kbdev->pm.metrics.timer.function = dvfs_callback;
                 do_gettimeofday( &g_tv_timer_start);
                 hrtimer_start(&kbdev->pm.metrics.timer, HR_TIMER_DELAY_MSEC(kbdev->pm.platform_dvfs_frequency), HRTIMER_MODE_REL);
             }
             else
             {
                 // unknown case
                 hrtimer_init(&kbdev->pm.metrics.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
                 kbdev->pm.platform_dvfs_frequency = mtk_get_dvfs_freq();
      		 	 kbdev->pm.metrics.timer.function = dvfs_callback;
    			 do_gettimeofday( &g_tv_timer_start);
      		 	 hrtimer_start(&kbdev->pm.metrics.timer, HR_TIMER_DELAY_MSEC(kbdev->pm.platform_dvfs_frequency), HRTIMER_MODE_REL);
             }

  		 }
  	}
  	kbase_dev_list_put(kbdev_list);  	
}
KBASE_EXPORT_TEST_API(mali_SODI_exit);

/* Shift used for kbasep_pm_metrics_data.time_busy/idle - units of (1 << 8) ns
   This gives a maximum period between samples of 2^(32+8)/100 ns = slightly under 11s.
   Exceeding this will cause overflow */
#define KBASE_PM_TIME_SHIFT			8

#ifdef CONFIG_MALI_MIDGARD_DVFS

void mtk_get_touch_boost_flag(int *touch_boost_flag, int *touch_boost_id)
{
    *touch_boost_flag = g_touch_boost_flag;
    *touch_boost_id = g_touch_boost_id;
    return;
}

void mtk_set_touch_boost_flag(int touch_boost_id)
{
    g_touch_boost_flag = 1;
    g_touch_boost_id = touch_boost_id;
    return;
}

void mtk_clear_touch_boost_flag(void)
{
    g_touch_boost_flag = 0;
    return;
}

int mtk_get_input_boost_enabled()
{
    return g_input_boost_enabled;
}

int mtk_get_dvfs_enabled()
{
    return g_dvfs_enabled;
}

int mtk_get_dvfs_freq()
{
    return g_dvfs_freq;
}

int mtk_get_dvfs_threshold_max()
{
    return g_dvfs_threshold_max;
}

int mtk_get_dvfs_threshold_min()
{
    return g_dvfs_threshold_min;
}

int mtk_get_dvfs_deferred_count()
{
    return g_dvfs_deferred_count;
}

void mtk_set_vgpu_power_on_flag(int power_on_id)
{
    g_vgpu_power_on_flag = power_on_id;
    return;
}
int mtk_get_vgpu_power_on_flag(void)
{
    return g_vgpu_power_on_flag;
}

int mtk_set_mt_gpufreq_target(int freq_id)
{
    if(MTK_VGPU_POWER_ON == mtk_get_vgpu_power_on_flag()){
       return mt_gpufreq_target(freq_id);
    }else{
        mtk_set_power_on_freq_flag(freq_id);
        pr_alert("MALI: VGPU power is off, ignore set freq: %d. \n",freq_id);
    }
    return -1;

}

void mtk_get_power_on_freq_flag(int *power_on_freq_flag, int *power_on_freq_id)
{
    *power_on_freq_flag = g_power_on_freq_flag;
    *power_on_freq_id = g_power_on_freq_id;
    return;
}

void mtk_set_power_on_freq_flag(int power_on_freq_id)
{
    g_power_on_freq_flag = 1;
    g_power_on_freq_id = power_on_freq_id;
    return;
}

void mtk_clear_power_on_freq_flag(void)
{
    g_power_on_freq_flag = 0;
    return;
}
enum kbase_pm_dvfs_action mtk_get_dvfs_action()
{
    return g_current_action;
}

void mtk_clear_dvfs_action()
{
    g_current_action = KBASE_PM_DVFS_NONSENSE;    
}

static enum hrtimer_restart dvfs_callback(struct hrtimer *timer)
{
	unsigned long flags;
	enum kbase_pm_dvfs_action action;
	struct kbasep_pm_metrics_data *metrics;

	KBASE_DEBUG_ASSERT(timer != NULL);

	metrics = container_of(timer, struct kbasep_pm_metrics_data, timer);
	action = kbase_pm_get_dvfs_action(metrics->kbdev);

	g_current_action = action;
	spin_lock_irqsave(&metrics->lock, flags);
    metrics->kbdev->pm.platform_dvfs_frequency = mtk_get_dvfs_freq();

	if (metrics->timer_active)
	{
		do_gettimeofday( &g_tv_timer_start);
		hrtimer_start(timer,
					  HR_TIMER_DELAY_MSEC(metrics->kbdev->pm.platform_dvfs_frequency),
					  HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&metrics->lock, flags);

	return HRTIMER_NORESTART;
}
#endif /* CONFIG_MALI_MIDGARD_DVFS */

mali_error kbasep_pm_metrics_init(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	kbdev->pm.metrics.kbdev = kbdev;
	kbdev->pm.metrics.vsync_hit = 1; /* [MTK] vsync notifiy is not implemented yet, assumed always hit. */
	kbdev->pm.metrics.utilisation = 0;
	kbdev->pm.metrics.util_cl_share[0] = 0;
	kbdev->pm.metrics.util_cl_share[1] = 0;
	kbdev->pm.metrics.util_gl_share = 0;

	kbdev->pm.metrics.time_period_start = ktime_get();
	kbdev->pm.metrics.time_busy = 0;
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.gpu_active = MALI_TRUE;
	kbdev->pm.metrics.active_cl_ctx[0] = 0;
	kbdev->pm.metrics.active_cl_ctx[1] = 0;
	kbdev->pm.metrics.active_gl_ctx = 0;
	kbdev->pm.metrics.busy_cl[0] = 0;
	kbdev->pm.metrics.busy_cl[1] = 0;
	kbdev->pm.metrics.busy_gl = 0;

	spin_lock_init(&kbdev->pm.metrics.lock);

#ifdef CONFIG_MALI_MIDGARD_DVFS
#ifndef ENABLE_COMMON_DVFS	
	kbdev->pm.metrics.timer_active = MALI_TRUE;
	hrtimer_init(&kbdev->pm.metrics.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	kbdev->pm.metrics.timer.function = dvfs_callback;

	do_gettimeofday( &g_tv_timer_start);
	hrtimer_start(&kbdev->pm.metrics.timer, HR_TIMER_DELAY_MSEC(kbdev->pm.platform_dvfs_frequency), HRTIMER_MODE_REL);
#endif
#endif /* CONFIG_MALI_MIDGARD_DVFS */

	kbase_pm_register_vsync_callback(kbdev);

	return MALI_ERROR_NONE;
}

KBASE_EXPORT_TEST_API(kbasep_pm_metrics_init)

void kbasep_pm_metrics_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_DVFS
	unsigned long flags;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	kbdev->pm.metrics.timer_active = MALI_FALSE;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

	hrtimer_cancel(&kbdev->pm.metrics.timer);
#endif /* CONFIG_MALI_MIDGARD_DVFS */

	kbase_pm_unregister_vsync_callback(kbdev);
}

KBASE_EXPORT_TEST_API(kbasep_pm_metrics_term)

/*caller needs to hold kbdev->pm.metrics.lock before calling this function*/
void kbasep_pm_record_job_status(struct kbase_device *kbdev)
{
	ktime_t now;
	ktime_t diff;
	u32 ns_time;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	now = ktime_get();
	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	ns_time = (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
	kbdev->pm.metrics.time_busy += ns_time;
	kbdev->pm.metrics.busy_gl += ns_time * kbdev->pm.metrics.active_gl_ctx;
	kbdev->pm.metrics.busy_cl[0] += ns_time * kbdev->pm.metrics.active_cl_ctx[0];
	kbdev->pm.metrics.busy_cl[1] += ns_time * kbdev->pm.metrics.active_cl_ctx[1];
	kbdev->pm.metrics.time_period_start = now;
}

KBASE_EXPORT_TEST_API(kbasep_pm_record_job_status)

void kbasep_pm_record_gpu_idle(struct kbase_device *kbdev)
{
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	KBASE_DEBUG_ASSERT(kbdev->pm.metrics.gpu_active == MALI_TRUE);

	kbdev->pm.metrics.gpu_active = MALI_FALSE;

	kbasep_pm_record_job_status(kbdev);

	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbasep_pm_record_gpu_idle)

void kbasep_pm_record_gpu_active(struct kbase_device *kbdev)
{
	unsigned long flags;
	ktime_t now;
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	KBASE_DEBUG_ASSERT(kbdev->pm.metrics.gpu_active == MALI_FALSE);

	kbdev->pm.metrics.gpu_active = MALI_TRUE;

	now = ktime_get();
	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	kbdev->pm.metrics.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
	kbdev->pm.metrics.time_period_start = now;

	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbasep_pm_record_gpu_active)

void kbase_pm_report_vsync(struct kbase_device *kbdev, int buffer_updated)
{
	unsigned long flags;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	kbdev->pm.metrics.vsync_hit = buffer_updated;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbase_pm_report_vsync)

/*caller needs to hold kbdev->pm.metrics.lock before calling this function*/
static void kbase_pm_get_dvfs_utilisation_calc(struct kbase_device *kbdev, ktime_t now)
{
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	if (kbdev->pm.metrics.gpu_active) {
		u32 ns_time = (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
		kbdev->pm.metrics.time_busy += ns_time;
		kbdev->pm.metrics.busy_cl[0] += ns_time * kbdev->pm.metrics.active_cl_ctx[0];
		kbdev->pm.metrics.busy_cl[1] += ns_time * kbdev->pm.metrics.active_cl_ctx[1];
		kbdev->pm.metrics.busy_gl += ns_time * kbdev->pm.metrics.active_gl_ctx;
	} else {
		kbdev->pm.metrics.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
	}
}

/*caller needs to hold kbdev->pm.metrics.lock before calling this function*/
static void kbase_pm_get_dvfs_utilisation_reset(struct kbase_device *kbdev, ktime_t now)
{
	kbdev->pm.metrics.time_period_start = now;
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;
	kbdev->pm.metrics.busy_cl[0] = 0;
	kbdev->pm.metrics.busy_cl[1] = 0;
	kbdev->pm.metrics.busy_gl = 0;
}

void kbase_pm_get_dvfs_utilisation(struct kbase_device *kbdev, unsigned long *total, unsigned long *busy, bool reset)
{
	ktime_t now = ktime_get();
	unsigned long tmp, flags;

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	kbase_pm_get_dvfs_utilisation_calc(kbdev, now);

	tmp = kbdev->pm.metrics.busy_gl;
	tmp += kbdev->pm.metrics.busy_cl[0];
	tmp += kbdev->pm.metrics.busy_cl[1];

	*busy = tmp;
	*total = tmp + kbdev->pm.metrics.time_idle;

	if (reset)
		kbase_pm_get_dvfs_utilisation_reset(kbdev, now);
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

#ifdef CONFIG_MALI_MIDGARD_DVFS

/*caller needs to hold kbdev->pm.metrics.lock before calling this function*/
int kbase_pm_get_dvfs_utilisation_old(struct kbase_device *kbdev, int *util_gl_share, int util_cl_share[2])
{
	int utilisation;
	int busy;
	ktime_t now = ktime_get();

	kbase_pm_get_dvfs_utilisation_calc(kbdev, now);

	if (kbdev->pm.metrics.time_idle + kbdev->pm.metrics.time_busy == 0) {
		/* No data - so we return NOP */
		utilisation = -1;
		if (util_gl_share)
			*util_gl_share = -1;
		if (util_cl_share) {
			util_cl_share[0] = -1;
			util_cl_share[1] = -1;
		}
		goto out;
	}

	utilisation = (100 * kbdev->pm.metrics.time_busy) /
			(kbdev->pm.metrics.time_idle +
			 kbdev->pm.metrics.time_busy);

	busy = kbdev->pm.metrics.busy_gl +
		kbdev->pm.metrics.busy_cl[0] +
		kbdev->pm.metrics.busy_cl[1];

	if (busy != 0) {
		if (util_gl_share)
			*util_gl_share =
				(100 * kbdev->pm.metrics.busy_gl) / busy;
		if (util_cl_share) {
			util_cl_share[0] =
				(100 * kbdev->pm.metrics.busy_cl[0]) / busy;
			util_cl_share[1] =
				(100 * kbdev->pm.metrics.busy_cl[1]) / busy;
		}
	} else {
		if (util_gl_share)
			*util_gl_share = -1;
		if (util_cl_share) {
			util_cl_share[0] = -1;
			util_cl_share[1] = -1;
		}
	}

out:
	kbase_pm_get_dvfs_utilisation_reset(kbdev, now);

	return utilisation;
}

void MTKCalGpuUtilization(unsigned int* pui32Loading , unsigned int* pui32Block,unsigned int* pui32Idle)
{
   struct kbase_device *kbdev = MaliGetMaliData();
   
	unsigned long flags;
	int utilisation, util_gl_share;
	int util_cl_share[2];
	int random_action;
	enum kbase_pm_dvfs_action action;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	utilisation = kbase_pm_get_dvfs_utilisation_old(kbdev, &util_gl_share, util_cl_share);
	
	if(pui32Loading)
	*pui32Loading = utilisation;
    if(pui32Idle)
        *pui32Idle = 100 - utilisation;
    
    /*
    if(pui32Block)
        *pui32Block = 0; // no ref value in r5px
     */

	if (utilisation < 0 || util_gl_share < 0 || util_cl_share[0] < 0 || util_cl_share[1] < 0) {
		action = KBASE_PM_DVFS_NOP;
		utilisation = 0;
		util_gl_share = 0;
		util_cl_share[0] = 0;
		util_cl_share[1] = 0;
		goto out;
	}

	if (kbdev->pm.metrics.vsync_hit) {
		/* VSync is being met */
		if (utilisation < mtk_get_dvfs_threshold_min())
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > mtk_get_dvfs_threshold_max())
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	} else {
		/* VSync is being missed */
		if (utilisation < KBASE_PM_NO_VSYNC_MIN_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > KBASE_PM_NO_VSYNC_MAX_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	}

	// get a radom action for stress test
	if (mtk_get_dvfs_enabled() == 2)
	{
		get_random_bytes( &random_action, sizeof(random_action));
        random_action = random_action%3;
        pr_debug("[MALI] GPU DVFS stress test - genereate random action here: action = %d", random_action);
        action = random_action;
	}

	kbdev->pm.metrics.utilisation = utilisation;
	kbdev->pm.metrics.util_cl_share[0] = util_cl_share[0];
	kbdev->pm.metrics.util_cl_share[1] = util_cl_share[1];
	kbdev->pm.metrics.util_gl_share = util_gl_share;

	g_current_sample_gl_utilization = utilisation;
	g_current_sample_cl_utilization[0] = util_cl_share[0];
	g_current_sample_cl_utilization[1] = util_cl_share[1];
   
out:
#if 0 /// def CONFIG_MALI_MIDGARD_DVFS
	kbase_platform_dvfs_event(kbdev, utilisation, util_gl_share, util_cl_share);
#endif				/*CONFIG_MALI_MIDGARD_DVFS */
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;
	kbdev->pm.metrics.busy_cl[0] = 0;
	kbdev->pm.metrics.busy_cl[1] = 0;
	kbdev->pm.metrics.busy_gl = 0;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);	
}
#define base_write32(addr, value) writel(value, addr)
#define base_read32(addr)         readl(addr)
void mtk_Power_Enable_MFG(void)
{
    struct kbase_device *kbdev = MaliGetMaliData();
    void __iomem *clk_mfgsys_base = kbdev->mfg_register;
#define    MFG_PERF_EN_0  (clk_mfgsys_base +0x1B0)
#define    MFG_PERF_EN_1  (clk_mfgsys_base +0x1B4)
#define    MFG_PERF_EN_2  (clk_mfgsys_base +0x1B8)
    if (!clk_mfgsys_base){
        pr_alert("[Mali] mtk_Power_Enable_MFG failed\n");
    }else{
        base_write32(MFG_PERF_EN_0, 0xffffffff);
        ///pr_alert("[Mali] MFG_PERF_EN_0 = %08x\n",base_read32(MFG_PERF_EN_0));
        base_write32(MFG_PERF_EN_1, 0xffffffff);
        ///pr_alert("[Mali] MFG_PERF_EN_1 = %08x\n",base_read32(MFG_PERF_EN_1));
        base_write32(MFG_PERF_EN_2, 0xffffffff);
        ///pr_alert("[Mali] MFG_PERF_EN_2 = %08x\n",base_read32(MFG_PERF_EN_2));
    }
}

void mtk_Power_Enable_Topck(void)
{
	struct device_node *node;
	void __iomem *clk_topck_base;
	
#define  CLK_MISC_CFG_0  (clk_topck_base + 0x104)

    if(g_clk_topck_base == 0x0){
        pr_alert("[Mali][pmeter]g_clk_topck_base: %p \n",g_clk_topck_base);
    	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-topckgen");
    	if (!node) {
    		pr_alert("[Mali]mtk_Power_Enable_TOPCK find node failed\n");
    	}
    	clk_topck_base = of_iomap(node, 0);
        g_clk_topck_base= clk_topck_base;
    }else{
        clk_topck_base = g_clk_topck_base;
    }
	if (!clk_topck_base){
		pr_alert("[Mali] mtk_Power_Enable_TOP_CK failed\n");
	}else{
			base_write32(CLK_MISC_CFG_0, base_read32(CLK_MISC_CFG_0) & 0xfffffffe);
			///pr_alert("[Mali] CLK_MISC_CFG_0 : %08x\n",base_read32(CLK_MISC_CFG_0));
	}

}


/* TODO: config by CONFIG_OF */
int dfp_weights[] = {
    173,    9,      1,      9,      2726,   793,    1277,   0,      8318,   4434,
    211,    8338,   0,      2572,   2163,   3254,   687,    0,      39499,  0,
    0,      0,      1988,   673,    0,      0,      890,    352,    0,      4803,
    611,    1051,   82,     0,      215,    211,    8338,   0,      2572,   2163,
    3254,   697,    0,      39499,  44,     42,     206,    17,     2,      31,
    26,     6,      566,    39,     52,     5,      0,      1171,   2232,   328,
    126,    776,    97,     17451,  33052,  65131,  1623,   66,     43,     91,
    19,     133,    12,     0,      3,      54,     3,      35,     217,    600,
    487,    2146,   572,    359,    13009,  253,    16889, 399,     1,      10,
    79,     22,     299,    304,    9,      1164,   412,    361,    61,     22,
    3,      2,      80,     29795,  46,     1009,   167,    390,    232,    113
};                                                                                        

#define DFP_ID                      0x0
#define DFP_CTRL                    0x4
//#define DFP_EVENT_ACCUM_CTRL       0x8
#define DFP_PM_VALUE                0x38
#define DFP_LP_CONSTANT            0x3c
#define DFP_LEAKAGE_VALUE          0x40
#define DFP_SCALING_FACTOR         0x44 // 0x50               
#define DFP_VOLT_FACTOR            0x48 // 0x54
#define DFP_PIECE_PWR              0x50
#define DFP_PM_PERIOD               0x5c
#define DFP_COUNTER_EN_0            0x60
#define DFP_COUNTER_EN_1            0x64
#define DFP_LINAER_A0               0x68
#define DFP_WEIGHT_(x)              (0x70+4*x)

#define DFP_DEBUG 0

unsigned int MTKCalPowerIndex(void){
    struct device_node *node;
    void __iomem *g_DFP_base;
    unsigned int dep =0;

    if(gb_DFP_base==0x0){
        pr_alert("[Mali][pmeter]gb_DFP_base: %p \n",gb_DFP_base);
        node = of_find_compatible_node(NULL, NULL, "mediatek,mfg_dfp");
        if (!node) {
            pr_alert("[Mali]mfg_dfp find node failed\n");
        }
        g_DFP_base = of_iomap(node, 0);
        gb_DFP_base = g_DFP_base;
    }else{
        g_DFP_base =gb_DFP_base;
    }
    if (!g_DFP_base){
        pr_alert("[Mali] g_DFP_base failed\n");
    }else{    

        
#define DFP_write32(addr, value)   base_write32(g_DFP_base+addr, value)
#define DFP_read32(addr)            base_read32(g_DFP_base+addr)

    int i,id,pm_period,linear_a0, scaling_factor,vlotage_factor,leakage_val;

    pm_period = 0;
    linear_a0 = 0;
    vlotage_factor = 1;
    scaling_factor = 14;
    leakage_val = 10;

    // turn on clock
    mtk_Power_Enable_Topck();
    //   GPU perforamcne clock gated
    mtk_Power_Enable_MFG();

    //pr_alert("[MALI]MTKCalPowerIndex g_DFP_base  = 0x%08x \n", (int)g_DFP_base);    

    DFP_write32(DFP_CTRL, DFP_read32(DFP_CTRL) | 0x2); // bit[0] -> dpm enable,  bit[1] ->  dcm enable

    DFP_write32(DFP_ID, 0x55aa3344);
    id = DFP_read32(DFP_ID);
    
    DFP_write32(DFP_LEAKAGE_VALUE, leakage_val);
    DFP_write32(DFP_VOLT_FACTOR, vlotage_factor);
    DFP_write32(DFP_SCALING_FACTOR, scaling_factor);
    DFP_write32(DFP_PM_PERIOD, pm_period);
    DFP_write32(DFP_LP_CONSTANT, 9);
    DFP_write32(DFP_LINAER_A0, linear_a0);

    if(DFP_DEBUG){
        pr_alert("[MALI]MTKCalPowerIndex ID_value = 0x%08x \n", id);
        pr_alert("[MALI]MTKCalPowerIndex Leakage_value = 0x%08x \n", DFP_read32(DFP_LEAKAGE_VALUE));
        pr_alert("[MALI]MTKCalPowerIndex DFP_VOLT_FACTOR = 0x%08x \n", DFP_read32(DFP_VOLT_FACTOR));        
        pr_alert("[MALI]MTKCalPowerIndex DFP_SCALING_FACTOR = 0x%08x \n", DFP_read32(DFP_SCALING_FACTOR));
        pr_alert("[MALI]MTKCalPowerIndex DFP_PM_PERIOD = 0x%08x \n", DFP_read32(DFP_PM_PERIOD));        
        pr_alert("[MALI]MTKCalPowerIndex DFP_LP_CONSTANT = 0x%08x \n", DFP_read32(DFP_LP_CONSTANT));        
        pr_alert("[MALI]MTKCalPowerIndex DFP_LINAER_A0 = 0x%08x \n", DFP_read32(DFP_LINAER_A0));
        pr_alert("[MALI]MTKCalPowerIndex DFP_COUNTER_EN_0 = 0x%08x \n", DFP_read32(DFP_COUNTER_EN_0));   
        pr_alert("[MALI]MTKCalPowerIndex DFP_COUNTER_EN_1 = 0x%08x \n", DFP_read32(DFP_COUNTER_EN_1));           
        pr_alert("[MALI]MTKCalPowerIndex g_vgpu_power_on_flag = 0x%08x \n", g_vgpu_power_on_flag);   

    }



    for (i = 0; i < 60; ++i)
    {
        DFP_write32(DFP_WEIGHT_(i), dfp_weights[i]);
    }

    DFP_write32(DFP_CTRL, DFP_read32(DFP_CTRL) | 0x1); // bit[0] -> dpm enable,  bit[1] ->  dcm enable
    if(DFP_DEBUG){
        pr_alert("[MALI]MTKCalPowerIndex DFP_CTRL 1= 0x%08x \n", DFP_read32(DFP_CTRL));  
    }


    DFP_write32(DFP_CTRL, 0x3 ); // bit[0] -> dpm enable,  bit[1] ->  dcm enable
    if(DFP_DEBUG){
        pr_alert("[MALI]MTKCalPowerIndex DFP_CTRL 2 = 0x%08x \n", DFP_read32(DFP_CTRL));  
    }

    
    for(i=0;i<100;i++)
    {
           dep = DFP_read32(DFP_PIECE_PWR);
           ///pr_alert("[MALI]MTKCalPowerIndex normal piece_pwr = 0x%08x \n", dep);
    }
       
    
    dep = DFP_read32(DFP_PM_VALUE); //MFG_DFP_60_PM_VALUE
    pr_alert("[MALI]MTKCalPowerIndex normal PM_value = %d \n", DFP_read32(DFP_PM_VALUE));
  

    }
    
    
    return dep;
}


enum kbase_pm_dvfs_action kbase_pm_get_dvfs_action(struct kbase_device *kbdev)
{
	unsigned long flags;
	int utilisation, util_gl_share;
	int util_cl_share[2];
	int random_action;
	enum kbase_pm_dvfs_action action;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	utilisation = kbase_pm_get_dvfs_utilisation_old(kbdev, &util_gl_share, util_cl_share);

	if (utilisation < 0 || util_gl_share < 0 || util_cl_share[0] < 0 || util_cl_share[1] < 0) {
		action = KBASE_PM_DVFS_NOP;
		utilisation = 0;
		util_gl_share = 0;
		util_cl_share[0] = 0;
		util_cl_share[1] = 0;
		goto out;
	}

	if (kbdev->pm.metrics.vsync_hit) {
		/* VSync is being met */
		if (utilisation < mtk_get_dvfs_threshold_min())
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > mtk_get_dvfs_threshold_max())
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	} else {
		/* VSync is being missed */
		if (utilisation < KBASE_PM_NO_VSYNC_MIN_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > KBASE_PM_NO_VSYNC_MAX_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	}

	// get a radom action for stress test
	if (mtk_get_dvfs_enabled() == 2)
	{
		get_random_bytes( &random_action, sizeof(random_action));
        random_action = random_action%3;
        pr_debug("[MALI] GPU DVFS stress test - genereate random action here: action = %d", random_action);
        action = random_action;
	}

	kbdev->pm.metrics.utilisation = utilisation;
	kbdev->pm.metrics.util_cl_share[0] = util_cl_share[0];
	kbdev->pm.metrics.util_cl_share[1] = util_cl_share[1];
	kbdev->pm.metrics.util_gl_share = util_gl_share;

	g_current_sample_gl_utilization = utilisation;
	g_current_sample_cl_utilization[0] = util_cl_share[0];
	g_current_sample_cl_utilization[1] = util_cl_share[1];
   
out:
#if 0 /// def CONFIG_MALI_MIDGARD_DVFS
	kbase_platform_dvfs_event(kbdev, utilisation, util_gl_share, util_cl_share);
#endif				/*CONFIG_MALI_MIDGARD_DVFS */
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;
	kbdev->pm.metrics.busy_cl[0] = 0;
	kbdev->pm.metrics.busy_cl[1] = 0;
	kbdev->pm.metrics.busy_gl = 0;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

	return action;
}
KBASE_EXPORT_TEST_API(kbase_pm_get_dvfs_action)

mali_bool kbase_pm_metrics_is_active(struct kbase_device *kbdev)
{
	mali_bool isactive;
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	isactive = (kbdev->pm.metrics.timer_active == MALI_TRUE);
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

	return isactive;
}

KBASE_EXPORT_TEST_API(kbase_pm_metrics_is_active)

u32 kbasep_get_gl_utilization(void)
{
	return g_current_sample_gl_utilization;
}
KBASE_EXPORT_TEST_API(kbasep_get_gl_utilization)

u32 kbasep_get_cl_js0_utilization(void)
{
	return g_current_sample_cl_utilization[0];
}
KBASE_EXPORT_TEST_API(kbasep_get_cl_js0_utilization)

u32 kbasep_get_cl_js1_utilization(void)
{
	return g_current_sample_cl_utilization[1];
}
KBASE_EXPORT_TEST_API(kbasep_get_cl_js1_utilization)


//extern unsigned int (*mtk_get_gpu_loading_fp)(void) = kbasep_get_gl_utilization;

static unsigned int _mtk_gpu_dvfs_index_to_frequency(int iFreq)
{
    unsigned int iCurrentFreqCount;
        iCurrentFreqCount =mt_gpufreq_get_dvfs_table_num();
    if(iCurrentFreqCount == 8) // MT6755
    {
        switch(iFreq)
        {
            case 0:
                return 728000;
            case 1:
                return 650000;
            case 2:
                return 598000;
            case 3:
                return 520000;
            case 4:
                return 468000;
            case 5:
                return 429000;
            case 6:
                return 390000;
            case 7:
                return 351000;
            default:
                return 351000;
        }
    }
    return 351000;
        
}

///=====================================================================================
///  The below function is added by Mediatek
///  In order to provide the debug sysfs command for change parameter dynamiically
///===================================================================================== 

/// 1. For GPU memory usage
static int proc_gpu_memoryusage_show(struct seq_file *m, void *v)
{
	ssize_t ret = 0;
   int total_size_in_bytes;
   int peak_size_in_bytes;

   total_size_in_bytes = kbase_report_gpu_memory_usage();   
   peak_size_in_bytes = kbase_report_gpu_memory_peak();
  
   ret = seq_printf(m, "curr: %10u, peak %10u\n", total_size_in_bytes, peak_size_in_bytes);

   return ret;
}

static int kbasep_gpu_memoryusage_debugfs_open(struct inode *in, struct file *file)
{
    return single_open(file, proc_gpu_memoryusage_show, NULL);
}

static const struct file_operations kbasep_gpu_memory_usage_debugfs_open = {
    .open    = kbasep_gpu_memoryusage_debugfs_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};



/// 2. For GL/CL utilization
static int proc_gpu_utilization_show(struct seq_file *m, void *v)
{
    unsigned long gl, cl0, cl1;
    unsigned int iCurrentFreq;

        iCurrentFreq =mt_gpufreq_get_cur_freq_index();
    
    gl  = kbasep_get_gl_utilization();
    cl0 = kbasep_get_cl_js0_utilization();
    cl1 = kbasep_get_cl_js1_utilization();

    seq_printf(m, "gpu/cljs0/cljs1=%lu/%lu/%lu, frequency=%d(kHz)\n", gl, cl0, cl1, _mtk_gpu_dvfs_index_to_frequency(iCurrentFreq));

    return 0;
}

static int kbasep_gpu_utilization_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_utilization_show , NULL);
}

static const struct file_operations kbasep_gpu_utilization_debugfs_fops = {
	.open    = kbasep_gpu_utilization_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int proc_gpu_powermeter_show(struct seq_file *m, void *v)
{

    seq_printf(m, "gpu/pm =%d \n", MTKCalPowerIndex());

    return 0;
}
static int kbasep_gpu_powermeter_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_powermeter_show , NULL);
}
static const struct file_operations kbasep_gpu_powermeter_debugfs_fops = {
	.open    = kbasep_gpu_powermeter_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};
/// 3. For query GPU frequency
static int proc_gpu_frequency_show(struct seq_file *m, void *v)
{

    unsigned int iCurrentFreq;

        iCurrentFreq =mt_gpufreq_get_cur_freq_index();

    seq_printf(m, "GPU Frequency: %u(kHz)\n", _mtk_gpu_dvfs_index_to_frequency(iCurrentFreq));

    return 0;
}

static int kbasep_gpu_frequency_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_frequency_show , NULL);
}

static const struct file_operations kbasep_gpu_frequency_debugfs_fops = {
	.open    = kbasep_gpu_frequency_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};



/// 4. For query GPU dynamically enable DVFS
static int proc_gpu_dvfs_enabled_show(struct seq_file *m, void *v)
{

    int dvfs_enabled;

    dvfs_enabled = mtk_get_dvfs_enabled();

    seq_printf(m, "dvfs_enabled: %d\n", dvfs_enabled);

    return 0;
}

static int kbasep_gpu_dvfs_enable_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_dvfs_enabled_show , NULL);
}

static ssize_t kbasep_gpu_dvfs_enable_write(struct file *file, const char __user *buffer, 
                size_t count, loff_t *data)
{
    char desc[32]; 
    int len = 0;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) {
        return 0;
    }
    desc[len] = '\0';

    if(!strncmp(desc, "1", 1))
        g_dvfs_enabled = 1;
    else if(!strncmp(desc, "0", 1))
        g_dvfs_enabled = 0;
    else if(!strncmp(desc, "2", 1))
        g_dvfs_enabled = 2;

    return count;
}

static const struct file_operations kbasep_gpu_dvfs_enable_debugfs_fops = {
	.open    = kbasep_gpu_dvfs_enable_debugfs_open,
	.read    = seq_read,
	.write   = kbasep_gpu_dvfs_enable_write,
	.release = single_release,
};



/// 5. For query GPU dynamically enable input boost
static int proc_gpu_input_boost_show(struct seq_file *m, void *v)
{

    int input_boost_enabled;

    input_boost_enabled = mtk_get_input_boost_enabled();

    seq_printf(m, "GPU input boost enabled: %d\n", input_boost_enabled);

    return 0;
}

static int kbasep_gpu_input_boost_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_input_boost_show , NULL);
}

static ssize_t kbasep_gpu_input_boost_write(struct file *file, const char __user *buffer, 
                size_t count, loff_t *data)
{
    char desc[32]; 
    int len = 0;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) {
        return 0;
    }
    desc[len] = '\0';

    if(!strncmp(desc, "1", 1))
        g_input_boost_enabled = 1;
    else if(!strncmp(desc, "0", 1))
        g_input_boost_enabled = 0;

    return count;
}

static const struct file_operations kbasep_gpu_input_boost_debugfs_fops = {
	.open    = kbasep_gpu_input_boost_debugfs_open,
	.read    = seq_read,
	.write   = kbasep_gpu_input_boost_write,
	.release = single_release,
};



/// 6. For query GPU dynamically set dvfs frequency (ms)
static int proc_gpu_dvfs_freq_show(struct seq_file *m, void *v)
{

    int dvfs_freq;

    dvfs_freq = mtk_get_dvfs_freq();

    seq_printf(m, "GPU DVFS freq : %d(ms)\n", dvfs_freq);

    return 0;
}

static int kbasep_gpu_dvfs_freq_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_dvfs_freq_show , NULL);
}

static ssize_t kbasep_gpu_dvfs_freq_write(struct file *file, const char __user *buffer, 
                size_t count, loff_t *data)
{
    char desc[32]; 
    int len = 0;
    int dvfs_freq = 0;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d", &dvfs_freq) == 1)
        g_dvfs_freq = dvfs_freq;
    else 
        pr_debug("[MALI] warning! echo [dvfs_freq(ms)] > /proc/mali/dvfs_freq\n");

    return count;
}

static const struct file_operations kbasep_gpu_dvfs_freq_debugfs_fops = {
	.open    = kbasep_gpu_dvfs_freq_debugfs_open,
	.read    = seq_read,
	.write   = kbasep_gpu_dvfs_freq_write,
	.release = single_release,
};



/// 7.For query GPU dynamically set dvfs threshold
static int proc_gpu_dvfs_threshold_show(struct seq_file *m, void *v)
{

    int threshold_max, threshold_min;

    threshold_max = mtk_get_dvfs_threshold_max();
    threshold_min = mtk_get_dvfs_threshold_min();

    seq_printf(m, "GPU DVFS threshold : max:%d min:%d\n", threshold_max, threshold_min);

    return 0;
}

static int kbasep_gpu_dvfs_threshold_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_dvfs_threshold_show , NULL);
}

static ssize_t kbasep_gpu_dvfs_threshold_write(struct file *file, const char __user *buffer, 
                size_t count, loff_t *data)
{
    char desc[32]; 
    int len = 0;
    int threshold_max, threshold_min;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d %d", &threshold_max, &threshold_min) == 2)
    {
        g_dvfs_threshold_max = threshold_max;
        g_dvfs_threshold_min = threshold_min;
    }
    else 
        pr_debug("[MALI] warning! echo [dvfs_threshold_max] [dvfs_threshold_min] > /proc/mali/dvfs_threshold\n");

    return count;
}

static const struct file_operations kbasep_gpu_dvfs_threshold_debugfs_fops = {
	.open    = kbasep_gpu_dvfs_threshold_debugfs_open,
	.read    = seq_read,
	.write   = kbasep_gpu_dvfs_threshold_write,
	.release = single_release,
};



/// 8.For query GPU dynamically set dvfs deferred count
static int proc_gpu_dvfs_deferred_count_show(struct seq_file *m, void *v)
{

    int deferred_count;

    deferred_count = mtk_get_dvfs_deferred_count();

    seq_printf(m, "GPU DVFS deferred_count : %d\n", deferred_count);

    return 0;
}

static int kbasep_gpu_dvfs_deferred_count_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_dvfs_deferred_count_show , NULL);
}

static ssize_t kbasep_gpu_dvfs_deferred_count_write(struct file *file, const char __user *buffer, 
                size_t count, loff_t *data)
{
    char desc[32]; 
    int len = 0;
    int dvfs_deferred_count;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d", &dvfs_deferred_count) == 1)
        g_dvfs_deferred_count = dvfs_deferred_count;
    else 
        pr_debug("[MALI] warning! echo [dvfs_deferred_count] > /proc/mali/dvfs_deferred_count\n");

    return count;
}

static const struct file_operations kbasep_gpu_dvfs_deferred_count_debugfs_fops = {
	.open    = kbasep_gpu_dvfs_deferred_count_debugfs_open,
	.read    = seq_read,
	.write   = kbasep_gpu_dvfs_deferred_count_write,
	.release = single_release,
};


/// 9. For query the support command
static int proc_gpu_help_show(struct seq_file *m, void *v)
{
    seq_printf(m, "======================================================================\n");
    seq_printf(m, "A.For Query GPU/CPU related Command:\n");
    seq_printf(m, "  cat /proc/mali/utilization\n");
    seq_printf(m, "  cat /proc/mali/frequency\n");
    seq_printf(m, "  cat /proc/mali/memory_usage\n");
    seq_printf(m, "  cat /proc/mali/powermeter\n");
    seq_printf(m, "  cat /proc/gpufreq/gpufreq_var_dump\n");
    seq_printf(m, "  cat /proc/pm_init/ckgen_meter_test\n");
    seq_printf(m, "  cat /proc/cpufreq/cpufreq_cur_freq\n");
    seq_printf(m, "======================================================================\n");
    seq_printf(m, "B.For Fix GPU Frequency:\n");
    seq_printf(m, "  echo > (728000, 520000, 312000) /proc/gpufreq/gpufreq_opp_freq\n");
    seq_printf(m, "  echo 0 > /proc/gpufreq/gpufreq_opp_freq(re-enable GPU DVFS)\n");
    seq_printf(m, "C.For Turn On/Off CPU core number:\n");
    seq_printf(m, "  echo (1, 0) > /sys/devices/system/cpu/cpu1/online\n");
    seq_printf(m, "  echo (1, 0) > /sys/devices/system/cpu/cpu2/online\n");
    seq_printf(m, "  echo (1, 0) > /sys/devices/system/cpu/cpuN/online\n");    
    seq_printf(m, "D.For CPU Performance mode(Force CPU to run at highest speed:\n");
    seq_printf(m, " echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\n");
    seq_printf(m, " echo interactive > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor(re-enable CPU DVFS)\n");
    seq_printf(m, "==============================================================================================\n");
    seq_printf(m, "E.For GPU advanced debugging command:\n");
    seq_printf(m, " echo [dvfs_freq(ms)] > /proc/mali/dvfs_freq\n");
    seq_printf(m, " echo [dvfs_thr_max] [dvfs_thr_min] > /proc/mali/dvfs_threshold\n");
    seq_printf(m, " echo [dvfs_deferred_count] > /proc/mali/dvfs_deferred_count\n");    
    seq_printf(m, "==============================================================================================\n");

    return 0;
}

static int kbasep_gpu_help_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, proc_gpu_help_show , NULL);
}

static const struct file_operations kbasep_gpu_help_debugfs_fops = {
	.open    = kbasep_gpu_help_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *mali_pentry;

void proc_mali_register(void)
{    
    mali_pentry = proc_mkdir("mali", NULL);    
   
    if (!mali_pentry)
        return;
         
    proc_create("help", 0, mali_pentry, &kbasep_gpu_help_debugfs_fops);        
    proc_create("memory_usage", 0, mali_pentry, &kbasep_gpu_memory_usage_debugfs_open);
    proc_create("utilization", 0, mali_pentry, &kbasep_gpu_utilization_debugfs_fops);
    proc_create("frequency", 0, mali_pentry, &kbasep_gpu_frequency_debugfs_fops);
    proc_create("powermeter", 0, mali_pentry, &kbasep_gpu_powermeter_debugfs_fops);
    proc_create("dvfs_enable", S_IRUGO | S_IWUSR, mali_pentry, &kbasep_gpu_dvfs_enable_debugfs_fops);
    proc_create("input_boost", S_IRUGO | S_IWUSR, mali_pentry, &kbasep_gpu_input_boost_debugfs_fops);
    proc_create("dvfs_freq", S_IRUGO | S_IWUSR, mali_pentry, &kbasep_gpu_dvfs_freq_debugfs_fops);
    proc_create("dvfs_threshold", S_IRUGO | S_IWUSR, mali_pentry, &kbasep_gpu_dvfs_threshold_debugfs_fops);
    proc_create("dvfs_deferred_count", S_IRUGO | S_IWUSR, mali_pentry, &kbasep_gpu_dvfs_deferred_count_debugfs_fops);
}
void proc_mali_unregister(void)
{
    if (!mali_pentry)
        return;

    remove_proc_entry("help", mali_pentry);
    remove_proc_entry("memory_usage", mali_pentry);
    remove_proc_entry("utilization", mali_pentry);
    remove_proc_entry("frequency", mali_pentry);
    remove_proc_entry("powermeter", mali_pentry);
    remove_proc_entry("mali", NULL);
    mali_pentry = NULL;
}
#else
#define proc_mali_register() do{}while(0)
#define proc_mali_unregister() do{}while(0)
#endif /// CONFIG_PROC_FS

#endif /* CONFIG_MALI_MIDGARD_DVFS */

#endif  /* KBASE_PM_EN */
