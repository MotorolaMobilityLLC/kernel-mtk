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

#include <linux/sched.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <mtk_leds_drv.h>
#include <cmdq_record.h>
#include <ddp_reg.h>
#include <ddp_drv.h>
#include <ddp_path.h>
#include <primary_display.h>
#include <disp_drv_platform.h>
#ifdef CONFIG_MTK_SMI_EXT
#include <mtk_smi.h>
#include <smi_public.h>
#endif
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS) || \
	defined(CONFIG_MACH_ELBRUS) || defined(CONFIG_MACH_MT6799)
#include <ddp_clkmgr.h>
#endif
#endif
#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS)
#include <disp_lowpower.h>
#include <disp_helper.h>
#endif
#include <ddp_aal.h>
#include <ddp_pwm.h>

#if defined(CONFIG_MACH_MT6799)
#include "mt-plat/mtk_chip.h"
#endif

#if defined(CONFIG_MACH_ELBRUS) || defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS) || \
	defined(CONFIG_MACH_MT6799)
#define AAL0_MODULE_NAMING (DISP_MODULE_AAL0)
#else
#define AAL0_MODULE_NAMING (DISP_MODULE_AAL)
#endif

#if defined(CONFIG_MACH_MT6799)
#define AAL0_CLK_NAMING (DISP0_DISP_AAL0)
#else
#define AAL0_CLK_NAMING (DISP0_DISP_AAL)
#endif

#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS) || \
	defined(CONFIG_MACH_MT6799)
#define AAL_SUPPORT_PARTIAL_UPDATE
#endif

/* To enable debug log: */
/* # echo aal_dbg:1 > /sys/kernel/debug/dispsys */
int aal_dbg_en;

#define AAL_ERR(fmt, arg...) pr_err("[AAL] " fmt "\n", ##arg)
#define AAL_NOTICE(fmt, arg...) pr_warn("[AAL] " fmt "\n", ##arg)
#define AAL_DBG(fmt, arg...) \
	do { if (aal_dbg_en) pr_warn("[AAL] " fmt "\n", ##arg); } while (0)

#ifdef CONFIG_MTK_AAL_SUPPORT
static int disp_aal_write_init_regs(enum DISP_MODULE_ENUM module, void *cmdq);
#endif
static int disp_aal_write_param_to_reg(enum DISP_MODULE_ENUM module, struct cmdqRecStruct *cmdq,
				const DISP_AAL_PARAM * param);

static DECLARE_WAIT_QUEUE_HEAD(g_aal_hist_wq);
static DEFINE_SPINLOCK(g_aal_hist_lock);
static DISP_AAL_HIST g_aal_hist = {
	.serviceFlags = 0,
	.backlight = -1
};
static DISP_AAL_HIST g_aal_hist_db;
static ddp_module_notify g_ddp_notify;
static volatile int g_aal_hist_available;
static volatile int g_aal_is_init_regs_valid;
static volatile int g_aal_backlight_notified = 1023;
static volatile int g_aal_initialed;
static volatile bool g_aal0_is_clock_on;
static atomic_t g_aal_allowPartial = ATOMIC_INIT(0);
static volatile int g_led_mode = MT65XX_LED_MODE_NONE;
static bool g_aal_hw_offset;
static bool g_aal_dre_offset_separate;

#define AAL0_OFFSET (0)
#define AAL1_OFFSET (DISPSYS_AAL1_BASE - DISPSYS_AAL0_BASE)

#if defined(CONFIG_MACH_MT6799)
#define AAL_TOTAL_MODULE_NUM (2)

#define aal_get_module_from_id(id) ((id == DISP_AAL0) ? AAL0_MODULE_NAMING : DISP_MODULE_AAL1)
#define aal_get_offset(module) ((module == AAL0_MODULE_NAMING) ? AAL0_OFFSET : AAL1_OFFSET)
#define index_of_aal(module) ((module == AAL0_MODULE_NAMING) ? 0 : 1)

/* Locked by  g_aal_module#_hist_lock */
static volatile int g_aal_dirty_frame_retrieved[AAL_TOTAL_MODULE_NUM] = {1, 1};
#else
#define AAL_TOTAL_MODULE_NUM (1)

#define aal_get_module_from_id(id) (AAL0_MODULE_NAMING)
#define aal_get_offset(module) (AAL0_OFFSET)
#define index_of_aal(module) (0)

/* Locked by  g_aal_module#_hist_lock */
static volatile int g_aal_dirty_frame_retrieved[AAL_TOTAL_MODULE_NUM] = {1};
#endif

#ifdef CONFIG_MTK_AAL_SUPPORT
static DEFINE_SPINLOCK(g_aal0_hist_lock);
static DEFINE_SPINLOCK(g_aal1_hist_lock);

#define aal_index_hist_spin_trylock(index, flags, getlock)				\
do {												\
	if (index == 0)				\
		getlock = spin_trylock_irqsave(&g_aal0_hist_lock, flags);	\
	else											\
		getlock = spin_trylock_irqsave(&g_aal1_hist_lock, flags);	\
} while (0)

#define aal_index_hist_spin_lock(index, flags)				\
do {												\
	if (index == 0)				\
		spin_lock_irqsave(&g_aal0_hist_lock, flags);	\
	else											\
		spin_lock_irqsave(&g_aal1_hist_lock, flags);	\
} while (0)

#define aal_index_hist_spin_unlock(index, flags)				\
do {												\
	if (index == 0)				\
		spin_unlock_irqrestore(&g_aal0_hist_lock, flags); \
	else											\
		spin_unlock_irqrestore(&g_aal1_hist_lock, flags); \
} while (0)

#define AAL_MAX_HIST_COUNT           (0xFFFFFFFF)

enum AAL_UPDATE_HIST {
	UPDATE_NONE = 0,
	UPDATE_SINGLE,
	UPDATE_MULTIPLE
};

static DISP_AAL_HIST g_aal_hist_multi_pipe;
static volatile bool g_aal_reset_count;
static volatile int g_aal_prev_pipe;
/* Locked by  g_aal#_hist_lock */
static unsigned int g_aal_module_hist_count[AAL_TOTAL_MODULE_NUM];
/* Locked by  g_aal_hist_lock */
static unsigned int g_aal_hist_count;
#endif			/* CONFIG_MTK_AAL_SUPPORT */

static int disp_aal_get_cust_led(void)
{
	struct device_node *led_node = NULL;
	int ret = 0;
	int led_mode;
	int pwm_config[5] = { 0 };

	led_node = of_find_compatible_node(NULL, NULL, "mediatek,lcd-backlight");
	if (!led_node) {
		ret = -1;
		AAL_ERR("Cannot find LED node from dts\n");
	} else {
		ret = of_property_read_u32(led_node, "led_mode", &led_mode);
		if (!ret)
			g_led_mode = led_mode;
		else
			AAL_ERR("led dts can not get led mode data.\n");

		ret = of_property_read_u32_array(led_node, "pwm_config", pwm_config,
						       ARRAY_SIZE(pwm_config));
	}

	if (ret)
		AAL_ERR("get pwm cust info fail");
	AAL_DBG("mode=%u", g_led_mode);

	return ret;
}

static void backlight_brightness_set_with_lock(int bl_1024)
{
	_primary_path_switch_dst_lock();
	primary_display_manual_lock();

	backlight_brightness_set(bl_1024);

	primary_display_manual_unlock();
	_primary_path_switch_dst_unlock();
}

static int disp_aal_exit_idle(const char *caller, int need_kick)
{
#ifdef MTK_DISP_IDLE_LP
	disp_exit_idle_ex(caller);
#endif
#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS) || \
	defined(CONFIG_MACH_MT6799)
	if (need_kick == 1)
		if (disp_helper_get_option(DISP_OPT_IDLEMGR_ENTER_ULPS))
			primary_display_idlemgr_kick(__func__, 1);
#endif
	return 0;
}

static int disp_aal_init(enum DISP_MODULE_ENUM module, int width, int height, void *cmdq)
{
	const int index = index_of_aal(module);

#ifdef CONFIG_MTK_AAL_SUPPORT
	/* Enable AAL histogram, engine */
	DISP_REG_MASK(cmdq, DISP_AAL_CFG + aal_get_offset(module), 0x3 << 1, (0x3 << 1) | 0x1);

	disp_aal_write_init_regs(module, cmdq);
#endif
#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS)
	/* disable stall cg for avoid display path hang */
	DISP_REG_MASK(cmdq, DISP_AAL_CFG + aal_get_offset(module), 0x1 << 4, 0x1 << 4);
#endif
	/* get lcd-backlight mode from dts */
	if (g_led_mode == MT65XX_LED_MODE_NONE)
		disp_aal_get_cust_led();

	g_aal_hist_available = 0;
	g_aal_dirty_frame_retrieved[index] = 1;

	return 0;
}


static void disp_aal_trigger_refresh(int latency)
{
	if (g_ddp_notify != NULL) {
		enum DISP_PATH_EVENT trigger_method = DISP_PATH_EVENT_TRIGGER;

#ifdef DISP_PATH_DELAYED_TRIGGER_33ms_SUPPORT
		if (latency == AAL_REFRESH_33MS)
			trigger_method = DISP_PATH_EVENT_DELAYED_TRIGGER_33ms;
#endif
		g_ddp_notify(AAL0_MODULE_NAMING, trigger_method);
		AAL_DBG("disp_aal_trigger_refresh: %d", trigger_method);
	}
}

static void disp_aal_set_interrupt_by_module(enum DISP_MODULE_ENUM module, int enabled)
{
#ifdef CONFIG_MTK_AAL_SUPPORT
	const int offset = aal_get_offset(module);

	if (enabled) {
		if (DISP_REG_GET(DISP_AAL_EN + offset) == 0)
			AAL_DBG("[WARNING] module(%d) DISP_AAL_EN not enabled!", module);

		/* Enable output frame end interrupt */
		DISP_CPU_REG_SET(DISP_AAL_INTEN + offset, 0x2);
		AAL_DBG("Module(%d) interrupt enabled", module);
	} else {
		if (g_aal_dirty_frame_retrieved[index_of_aal(module)]) {
			DISP_CPU_REG_SET(DISP_AAL_INTEN + offset, 0x0);
			AAL_DBG("Module(%d) interrupt disabled", module);
		} else {	/* Dirty histogram was not retrieved */
			/* Only if the dirty hist was retrieved, interrupt can be disabled. */
			/* Continue interrupt until AALService can get the latest histogram. */
		}
	}
#else
	AAL_ERR("AAL driver is not enabled");
#endif
}

static void disp_aal_set_interrupt(int enabled)
{
	enum DISP_MODULE_ENUM module = AAL0_MODULE_NAMING;
	int i;
	int config_module_num = 1;

#if defined(CONFIG_MACH_MT6799)
	if (primary_display_get_pipe_status() != SINGLE_PIPE)
		config_module_num = AAL_TOTAL_MODULE_NUM;
#endif
	for (i = 0; i < config_module_num; i++) {
		module += i;
		AAL_DBG("Set AAL%d interrupt enable (%d)", i, enabled);
		disp_aal_set_interrupt_by_module(module, enabled);
	}
}

static void disp_aal_notify_frame_dirty(enum DISP_MODULE_ENUM module)
{
#ifdef CONFIG_MTK_AAL_SUPPORT
	unsigned long flags;
	const int index = index_of_aal(module);

	AAL_DBG("Module(%d) disp_aal_notify_frame_dirty()", module);

	disp_aal_exit_idle("disp_aal_notify_frame_dirty", 0);

	aal_index_hist_spin_lock(index, flags);
	/* Interrupt can be disabled until dirty histogram is retrieved */
	g_aal_dirty_frame_retrieved[index] = 0;
	aal_index_hist_spin_unlock(index, flags);

	disp_aal_set_interrupt_by_module(module, 1);
#endif
}

static int disp_aal_wait_hist(unsigned long timeout)
{
	int ret = 0;

	AAL_DBG("disp_aal_wait_hist: available = %d", g_aal_hist_available);

	if (!g_aal_hist_available) {
		ret = wait_event_interruptible(g_aal_hist_wq, (g_aal_hist_available != 0));
		AAL_DBG("disp_aal_wait_hist: waken up, ret = %d", ret);
	} else {
		/* If g_aal_hist_available is already set, means AALService was delayed */
	}

	return ret;
}

#ifdef CONFIG_MTK_AAL_SUPPORT
static void disp_aal_clear_irq_only(enum DISP_MODULE_ENUM module, bool cleared)
{
	unsigned int intsta;
	unsigned long flags;
	const int index = index_of_aal(module);
	const int offset = aal_get_offset(module);
	int getlock;

	intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset);

	aal_index_hist_spin_trylock(index, flags, getlock);
	if (getlock > 0) {
		DISP_CPU_REG_SET(DISP_AAL_INTSTA + offset, (intsta & ~0x3));

		/* Allow to disable interrupt */
		g_aal_dirty_frame_retrieved[index] = 1;
		/* Check current irq status */
		intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset) & 0x3;
		if (intsta != 0) {
			/* print error message */
			AAL_ERR("intsta error:(0x%08x), Module(%d), process:(%d), in (%s)",
				intsta, module, cleared, __func__);
		}
		aal_index_hist_spin_unlock(index, flags);

		/*
		 * no need per-frame wakeup.
		 * We stop interrupt until next frame dirty.
		 */
		if (cleared == true)
			disp_aal_set_interrupt_by_module(module, 0);
	}

	AAL_NOTICE("disp_aal_clear_irq_only, Module(%d), process:(%d)", module, cleared);
}

static void disp_aal_single_pipe_hist_update(enum DISP_MODULE_ENUM module)
{
	unsigned int intsta;
	int i;
	unsigned long flags;
	const int index = index_of_aal(module);
	const int offset = aal_get_offset(module);
	int getlock;

	do {
		/* Only process AAL0 in single module state */
		if (module != AAL0_MODULE_NAMING) {
			disp_aal_clear_irq_only(module, true);
			break;
		}

		intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset);
		AAL_DBG("Module(%d) disp_aal_single_pipe_hist_update: intsta: 0x%x", module, intsta);

		/* Only process end of frame state */
		if ((intsta & 0x2) == 0x0)
			break;

		aal_index_hist_spin_trylock(index, flags, getlock);
		if (getlock <= 0)
			break;

		DISP_CPU_REG_SET(DISP_AAL_INTSTA + offset, (intsta & ~0x3));

		/* Allow to disable interrupt */
		g_aal_dirty_frame_retrieved[index] = 1;
		/* Check current irq status */
		intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset) & 0x3;
		if (intsta != 0) {
			/* print error message */
			AAL_ERR("intsta error:(0x%08x), Module(%d), in (%s)", intsta, module, __func__);
		}
		aal_index_hist_spin_unlock(index, flags);

		if (spin_trylock_irqsave(&g_aal_hist_lock, flags)) {
			for (i = 0; i < AAL_HIST_BIN; i++)
				g_aal_hist.maxHist[i] = DISP_REG_GET(DISP_AAL_STATUS_00 + (i << 2));
			g_aal_hist.colorHist = DISP_REG_GET(DISP_COLOR_TWO_D_W1_RESULT);

			g_aal_hist_available = 1;

			spin_unlock_irqrestore(&g_aal_hist_lock, flags);

			wake_up_interruptible(&g_aal_hist_wq);
		} else {
			/*
			 * Histogram was not be retrieved, but it's OK.
			 * Another interrupt will come until histogram available
			 * See: disp_aal_set_interrupt()
			 */
		}

		if (!g_aal_is_init_regs_valid) {
			/*
			 * AAL service is not running, not need per-frame wakeup.
			 * We stop interrupt until next frame dirty.
			 */
			disp_aal_set_interrupt_by_module(module, 0);
		}
	} while (0);
}

static void disp_aal_reset_count(void)
{
	int i;
	unsigned long flags;

	AAL_DBG("disp_aal_reset_count is triggered");
	for (i = 0; i < AAL_TOTAL_MODULE_NUM; i++) {
		aal_index_hist_spin_lock(i, flags);
		g_aal_module_hist_count[i] = 0;
		aal_index_hist_spin_unlock(i, flags);
	}

	spin_lock_irqsave(&g_aal_hist_lock, flags);
	g_aal_hist_count = 0;
	g_aal_reset_count = false;
	spin_unlock_irqrestore(&g_aal_hist_lock, flags);
}

static void disp_aal_multiple_pipe_hist_update(enum DISP_MODULE_ENUM module)
{
	unsigned int intsta;
	int i;
	unsigned long flags;
	unsigned int hist_count;
	unsigned int is_hist_available = 0;
	const int index = index_of_aal(module);
	const int offset = aal_get_offset(module);
	const int color_offset = (module == AAL0_MODULE_NAMING) ? 0 : (DISPSYS_COLOR1_BASE - DISPSYS_COLOR0_BASE);
	int getlock;

	do {
		intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset);
		AAL_DBG("Module(%d) disp_aal_multiple_pipe_hist_update: intsta: 0x%x", module, intsta);

		/* Only process end of frame state */
		if ((intsta & 0x2) == 0x0)
			break;

		aal_index_hist_spin_trylock(index, flags, getlock);
		if (getlock <= 0)
			break;

		DISP_CPU_REG_SET(DISP_AAL_INTSTA + offset, (intsta & ~0x3));

		/* Allow to disable interrupt */
		g_aal_dirty_frame_retrieved[index] = 1;

		g_aal_module_hist_count[index] = (g_aal_module_hist_count[index] + 1) & AAL_MAX_HIST_COUNT;

		hist_count = g_aal_module_hist_count[index];
		/* Check current irq status */
		intsta = DISP_REG_GET(DISP_AAL_INTSTA + offset) & 0x3;
		if (intsta != 0) {
			/* print error message */
			AAL_ERR("intsta error:(0x%08x), Module(%d), in (%s)", intsta, module, __func__);
		}
		aal_index_hist_spin_unlock(index, flags);

		if (spin_trylock_irqsave(&g_aal_hist_lock, flags)) {
			if ((hist_count-g_aal_hist_count) == 1 || (hist_count == 0 && g_aal_hist_count > 0)) {
				for (i = 0; i < AAL_HIST_BIN; i++) {
					g_aal_hist_multi_pipe.maxHist[i] =
						DISP_REG_GET(DISP_AAL_STATUS_00 + offset + (i << 2));
				}
				g_aal_hist_multi_pipe.colorHist =
					DISP_REG_GET(DISP_COLOR_TWO_D_W1_RESULT + color_offset);

				g_aal_hist_count = hist_count;
				AAL_DBG("disp_aal_hist_combination: copy histogram");
			} else if (hist_count == g_aal_hist_count) {
				for (i = 0; i < AAL_HIST_BIN; i++) {
					g_aal_hist.maxHist[i] = g_aal_hist_multi_pipe.maxHist[i] +
						DISP_REG_GET(DISP_AAL_STATUS_00 + offset + (i << 2));
				}
				g_aal_hist.colorHist = g_aal_hist_multi_pipe.colorHist +
					DISP_REG_GET(DISP_COLOR_TWO_D_W1_RESULT + color_offset);

				is_hist_available = g_aal_hist_available = 1;
				AAL_DBG("disp_aal_hist_combination: histogram is updated");
			} else {
				g_aal_reset_count = true;
				AAL_DBG("Can not update histogram now");
			}

			spin_unlock_irqrestore(&g_aal_hist_lock, flags);

			if (is_hist_available == 1)
				wake_up_interruptible(&g_aal_hist_wq);
		} else {
			/*
			 * Histogram was not be retrieved, but it's OK.
			 * Another interrupt will come until histogram available
			 * See: disp_aal_set_interrupt()
			 */
		}

		if (!g_aal_is_init_regs_valid) {
			/*
			 * AAL service is not running, not need per-frame wakeup.
			 * We stop interrupt until next frame dirty.
			 */
			disp_aal_set_interrupt_by_module(module, 0);
		}
	} while (0);
}
#endif				/* CONFIG_MTK_AAL_SUPPORT */

void disp_aal_on_end_of_frame_by_module(disp_aal_id_t id)
{
#ifdef CONFIG_MTK_AAL_SUPPORT
	int update_method = UPDATE_SINGLE;
	enum DISP_MODULE_ENUM module = aal_get_module_from_id(id);
#if defined(CONFIG_MACH_MT6799)
	int pipe_status = primary_display_get_pipe_status();

	if (pipe_status == SINGLE_PIPE) {
		update_method = UPDATE_SINGLE;
		AAL_DBG("single mode,  process Module(%d) in irq_handler", module);
	} else if (pipe_status == DUAL_PIPE) {
		update_method = UPDATE_MULTIPLE;
		AAL_DBG("dual mode,  process Module(%d) in irq_handler", module);
	} else {
		update_method = UPDATE_NONE;
		AAL_DBG("pipe_status (%d), process Module(%d) in irq_handler", pipe_status, module);
	}
#endif

	if (id < DISP_AAL0 || id >= DISP_AAL0 + AAL_TOTAL_MODULE_NUM)
		return;

	if (update_method == UPDATE_SINGLE) {
		/* Process single AAL engine */
		disp_aal_single_pipe_hist_update(module);
	} else if (update_method == UPDATE_MULTIPLE) {
		/* Process multiple AAL engine */
		if ((g_aal_prev_pipe != update_method) || (g_aal_reset_count == true))
			disp_aal_reset_count();
		disp_aal_multiple_pipe_hist_update(module);
	} else {
		disp_aal_clear_irq_only(module, false);
	}

	g_aal_prev_pipe = update_method;
#else
	/*
	 * We will not wake up AAL unless signals
	 */
#endif
}

void disp_aal_on_end_of_frame(void)
{
	disp_aal_on_end_of_frame_by_module(DISP_AAL0);
}


#define LOG_INTERVAL_TH 200
#define LOG_BUFFER_SIZE 4
static char g_aal_log_buffer[256] = "";
static int g_aal_log_index;
struct timeval g_aal_log_prevtime = {0};

static unsigned long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
	unsigned long msec;

	msec = (finishtime->tv_sec-starttime->tv_sec)*1000;
	msec += (finishtime->tv_usec-starttime->tv_usec)/1000;

	return msec;
}

static void disp_aal_notify_backlight_log(int bl_1024)
{
	struct timeval aal_time;
	unsigned long diff_mesc = 0;
	unsigned long tsec;
	unsigned long tusec;

	do_gettimeofday(&aal_time);
	tsec = (unsigned long)aal_time.tv_sec % 100;
	tusec = (unsigned long)aal_time.tv_usec / 1000;

	diff_mesc = timevaldiff(&g_aal_log_prevtime, &aal_time);
	AAL_DBG("time diff = %lu", diff_mesc);

	if (diff_mesc > LOG_INTERVAL_TH) {
		if (g_aal_log_index == 0) {
			pr_debug("disp_aal_notify_backlight_changed: %d/1023\n", bl_1024);
		} else {
			sprintf(g_aal_log_buffer + strlen(g_aal_log_buffer), ", %d/1023 %03lu.%03lu",
				bl_1024, tsec, tusec);
			pr_debug("%s\n", g_aal_log_buffer);
			g_aal_log_index = 0;
		}
	} else {
		if (g_aal_log_index == 0) {
			sprintf(g_aal_log_buffer,
				"disp_aal_notify_backlight_changed %d/1023 %03lu.%03lu",
				bl_1024, tsec, tusec);
			g_aal_log_index += 1;
		} else {
			sprintf(g_aal_log_buffer + strlen(g_aal_log_buffer), ", %d/1023 %03lu.%03lu",
				bl_1024, tsec, tusec);
			g_aal_log_index += 1;
		}

		if ((g_aal_log_index >= LOG_BUFFER_SIZE) || (bl_1024 == 0)) {
			pr_debug("%s\n", g_aal_log_buffer);
			g_aal_log_index = 0;
		}
	}

	memcpy(&g_aal_log_prevtime, &aal_time, sizeof(struct timeval));
}

void disp_aal_notify_backlight_changed(int bl_1024)
{
	unsigned long flags;
	int max_backlight;
	unsigned int service_flags;

	/* pr_debug("disp_aal_notify_backlight_changed: %d/1023", bl_1024); */
	disp_aal_notify_backlight_log(bl_1024);

	disp_aal_exit_idle("disp_aal_notify_backlight_changed", 1);

	max_backlight = disp_pwm_get_max_backlight(DISP_PWM0);
	if (bl_1024 > max_backlight)
		bl_1024 = max_backlight;

	g_aal_backlight_notified = bl_1024;

	service_flags = 0;
	if (bl_1024 == 0) {
		if (g_led_mode == MT65XX_LED_MODE_CUST_LCM)
			backlight_brightness_set_with_lock(0);
		else
			backlight_brightness_set(0);

		/* set backlight = 0 may be not from AAL, */
		/* we have to let AALService can turn on backlight on phone resumption */
		service_flags = AAL_SERVICE_FORCE_UPDATE;
	} else if (!g_aal_is_init_regs_valid) {
		/* AAL Service is not running */
		if (g_led_mode == MT65XX_LED_MODE_CUST_LCM)
			backlight_brightness_set_with_lock(bl_1024);
		else
			backlight_brightness_set(bl_1024);
	}
	AAL_DBG("led_mode=%d", g_led_mode);

	spin_lock_irqsave(&g_aal_hist_lock, flags);
	g_aal_hist.backlight = bl_1024;
	g_aal_hist.serviceFlags |= service_flags;
	spin_unlock_irqrestore(&g_aal_hist_lock, flags);

	if (g_aal_is_init_regs_valid) {
		disp_aal_set_interrupt(1);
		/* Backlight latency should be as smaller as possible */
		disp_aal_trigger_refresh(AAL_REFRESH_17MS);
	}
}


static int disp_aal_copy_hist_to_user(DISP_AAL_HIST __user *hist)
{
	unsigned long flags;
	int ret = -EFAULT;

	/* We assume only one thread will call this function */

	spin_lock_irqsave(&g_aal_hist_lock, flags);
	memcpy(&g_aal_hist_db, &g_aal_hist, sizeof(DISP_AAL_HIST));
	g_aal_hist.serviceFlags = 0;
	g_aal_hist_available = 0;
	spin_unlock_irqrestore(&g_aal_hist_lock, flags);

	if (copy_to_user(hist, &g_aal_hist_db, sizeof(DISP_AAL_HIST)) == 0)
		ret = 0;

	AAL_DBG("disp_aal_copy_hist_to_user: %d", ret);

	return ret;
}


#define CABC_GAINLMT(v0, v1, v2) (((v2) << 20) | ((v1) << 10) | (v0))

#ifdef CONFIG_MTK_AAL_SUPPORT
static DISP_AAL_INITREG g_aal_init_regs;
#endif
static DISP_AAL_PARAM g_aal_param;

static int disp_aal_set_init_reg(DISP_AAL_INITREG __user *user_regs, enum DISP_MODULE_ENUM module, void *cmdq)
{
	int ret = -EFAULT;
#ifdef CONFIG_MTK_AAL_SUPPORT
	DISP_AAL_INITREG *init_regs;

	init_regs = &g_aal_init_regs;

	ret = copy_from_user(init_regs, user_regs, sizeof(DISP_AAL_INITREG));
	if (ret == 0) {
		g_aal_is_init_regs_valid = 1;
		AAL_DBG("Set module(%d) init reg", module);
		ret = disp_aal_write_init_regs(module, cmdq);
	} else {
		AAL_ERR("disp_aal_set_init_reg: copy_from_user() failed");
	}

	AAL_DBG("disp_aal_set_init_reg: %d", ret);
#else
	AAL_ERR("disp_aal_set_init_reg: AAL not supported");
#endif

	return ret;
}

#ifdef CONFIG_MTK_AAL_SUPPORT
static int disp_aal_write_init_regs(enum DISP_MODULE_ENUM module, void *cmdq)
{
	int ret = -EFAULT;
	const int offset = aal_get_offset(module);

	if (g_aal_is_init_regs_valid) {
		DISP_AAL_INITREG *init_regs = &g_aal_init_regs;

		int i, j;
		int *gain;

		if (g_aal_hw_offset == true) {
#if defined(CONFIG_MACH_MT6799)
			DISP_REG_MASK(cmdq, DISP_AAL_DRE_MAPPING_00_2 + offset, (init_regs->dre_map_bypass << 4),
			      1 << 4);

			gain = init_regs->cabc_gainlmt;
			j = 0;
			for (i = 0; i <= 10; i++) {
				DISP_REG_SET(cmdq, DISP_AAL_CABC_GAINLMT_TBL_2(i) + offset,
					     CABC_GAINLMT(gain[j], gain[j + 1], gain[j + 2]));
				j += 3;
			}
#endif
		} else {
			DISP_REG_MASK(cmdq, DISP_AAL_DRE_MAPPING_00 + offset, (init_regs->dre_map_bypass << 4),
			      1 << 4);

			gain = init_regs->cabc_gainlmt;
			j = 0;
			for (i = 0; i <= 10; i++) {
				DISP_REG_SET(cmdq, DISP_AAL_CABC_GAINLMT_TBL(i) + offset,
					     CABC_GAINLMT(gain[j], gain[j + 1], gain[j + 2]));
				j += 3;
			}
		}

		AAL_DBG("Module(%d): disp_aal_write_init_regs: done", module);
		ret = 0;
	}

	return ret;
}
#endif

int disp_aal_set_param(DISP_AAL_PARAM __user *param, enum DISP_MODULE_ENUM module, void *cmdq)
{
	int ret = -EFAULT;
	int backlight_value = 0;

	/* Not need to protect g_aal_param, */
	/* since only AALService can set AAL parameters. */
	if (copy_from_user(&g_aal_param, param, sizeof(DISP_AAL_PARAM)) == 0) {
		backlight_value = g_aal_param.FinalBacklight;
#ifdef CONFIG_MTK_AAL_SUPPORT
		/* set cabc gain zero when detect backlight setting equal to zero */
		if (backlight_value == 0)
			g_aal_param.cabc_fltgain_force = 0;
#endif
		AAL_DBG("Set module(%d) parameter", module);
		ret = disp_aal_write_param_to_reg(module, cmdq, &g_aal_param);
		atomic_set(&g_aal_allowPartial, g_aal_param.allowPartial);
	}

	if (g_aal_backlight_notified == 0)
		backlight_value = 0;

	if (ret == 0)
		ret |= disp_pwm_set_backlight_cmdq(DISP_PWM0, backlight_value, cmdq);

	AAL_DBG("disp_aal_set_param(CABC = %d, DRE[0,8] = %d,%d, latency=%d): ret = %d",
		g_aal_param.cabc_fltgain_force, g_aal_param.DREGainFltStatus[0],
		g_aal_param.DREGainFltStatus[8], g_aal_param.refreshLatency, ret);

	backlight_brightness_set(backlight_value);

	disp_aal_trigger_refresh(g_aal_param.refreshLatency);

	return ret;
}


#define DRE_REG_2(v0, off0, v1, off1)           (((v1) << (off1)) | ((v0) << (off0)))
#define DRE_REG_3(v0, off0, v1, off1, v2, off2) (((v2) << (off2)) | (v1 << (off1)) | ((v0) << (off0)))

static int disp_aal_write_param_to_reg(enum DISP_MODULE_ENUM module, struct cmdqRecStruct *cmdq,
			      const DISP_AAL_PARAM *param)
{
	int i;
	const int *gain;
	const int offset = aal_get_offset(module);

	gain = param->DREGainFltStatus;
#if defined(CONFIG_MACH_MT6799)
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(0) + offset, DRE_REG_2(gain[0], 0, gain[1], 14), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(1) + offset, DRE_REG_2(gain[2], 0, gain[3], 13), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(2) + offset, DRE_REG_2(gain[4], 0, gain[5], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(3) + offset, DRE_REG_2(gain[6], 0, gain[7], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(4) + offset, DRE_REG_2(gain[8], 0, gain[9], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(5) + offset, DRE_REG_2(gain[10], 0, gain[11], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(6) + offset,
		      DRE_REG_3(gain[12], 0, gain[13], 11, gain[14], 22), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(7) + offset,
		      DRE_REG_3(gain[15], 0, gain[16], 10, gain[17], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(8) + offset,
		      DRE_REG_3(gain[18], 0, gain[19], 10, gain[20], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(9) + offset,
		      DRE_REG_3(gain[21], 0, gain[22], 9, gain[23], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(10) + offset,
		      DRE_REG_3(gain[24], 0, gain[25], 9, gain[26], 18), ~0);
	if (g_aal_dre_offset_separate == true) {
		/* Write dre curve to different register */
		DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE_11 + offset, DRE_REG_2(gain[27], 0, gain[28], 9), ~0);
	} else {
		/* Write dre curve to different register */
		DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(11) + offset, DRE_REG_2(gain[27], 0, gain[28], 9), ~0);
	}
#else
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(0) + offset, DRE_REG_2(gain[0], 0, gain[1], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(1) + offset, DRE_REG_2(gain[2], 0, gain[3], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(2) + offset, DRE_REG_2(gain[4], 0, gain[5], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(3) + offset,
		      DRE_REG_3(gain[6], 0, gain[7], 11, gain[8], 21), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(4) + offset,
		      DRE_REG_3(gain[9], 0, gain[10], 10, gain[11], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(5) + offset,
		      DRE_REG_3(gain[12], 0, gain[13], 10, gain[14], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(6) + offset,
		      DRE_REG_3(gain[15], 0, gain[16], 10, gain[17], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(7) + offset,
		      DRE_REG_3(gain[18], 0, gain[19], 9, gain[20], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(8) + offset,
		      DRE_REG_3(gain[21], 0, gain[22], 9, gain[23], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(9) + offset,
		      DRE_REG_3(gain[24], 0, gain[25], 9, gain[26], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(10) + offset, DRE_REG_2(gain[27], 0, gain[28], 9), ~0);
#endif
	DISP_REG_MASK(cmdq, DISP_AAL_CABC_00 + offset, 1 << 31, 1 << 31);
	DISP_REG_MASK(cmdq, DISP_AAL_CABC_02 + offset, ((1 << 26) | param->cabc_fltgain_force),
		      ((1 << 26) | 0x3ff));

	gain = param->cabc_gainlmt;
	for (i = 0; i <= 10; i++) {
		DISP_REG_MASK(cmdq, DISP_AAL_CABC_GAINLMT_TBL(i) + offset,
			      CABC_GAINLMT(gain[0], gain[1], gain[2]), ~0);
		gain += 3;
	}

	return 0;
}


static int aal_config(enum DISP_MODULE_ENUM module, struct disp_ddp_path_config *pConfig, void *cmdq)
{
	bool should_update = (module == AAL0_MODULE_NAMING ? true : false);

#if defined(CONFIG_MACH_MT6799)
	if (pConfig->is_dual)
		should_update = true;
#endif

	if (pConfig->dst_dirty) {
		int width, height;
		const int offset = aal_get_offset(module);

		width = pConfig->dst_w;
		height = pConfig->dst_h;

#ifdef DISP_PLATFORM_HAS_SHADOW_REG
		if (disp_helper_get_option(DISP_OPT_SHADOW_REGISTER)) {
			unsigned long aal_shadow_ctl;
#if defined(CONFIG_MACH_MT6799)
			if (g_aal_hw_offset == true)
				aal_shadow_ctl = DISP_AAL_SHADOW_CTL_2;
			else
				aal_shadow_ctl = DISP_AAL_SHADOW_CTL;
#else
			aal_shadow_ctl = DISP_AAL_SHADOW_CTL;
#endif
			if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 0) {
				/* full shadow mode*/
				DISP_REG_SET(cmdq, aal_shadow_ctl + offset, 0x0);
			} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 1) {
				/* force commit */
				DISP_REG_SET(cmdq, aal_shadow_ctl + offset, 0x2);
			} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 2) {
				/* bypass shadow */
				DISP_REG_SET(cmdq, aal_shadow_ctl + offset, 0x1);
			}
		}
#endif

		DISP_REG_SET(cmdq, DISP_AAL_SIZE + offset, (width << 16) | height);
		DISP_REG_MASK(cmdq, DISP_AAL_CFG + offset, 0x0, 0x1);	/* Disable relay mode */

		disp_aal_init(module, width, height, cmdq);

		DISP_REG_MASK(cmdq, DISP_AAL_EN + offset, 0x1, 0x1);

		AAL_DBG("module id:(%d), AAL_CFG = 0x%x, AAL_SIZE = 0x%x(%d, %d)",
			module, DISP_REG_GET(DISP_AAL_CFG + offset), DISP_REG_GET(DISP_AAL_SIZE + offset),
			width, height);
	}

	if ((pConfig->ovl_dirty || pConfig->rdma_dirty) && should_update == true)
		disp_aal_notify_frame_dirty(module);

	return 0;
}


/*****************************************************************************
 * AAL Backup / Restore function
 *****************************************************************************/
#if defined(CONFIG_MACH_MT6799)
#define DRE_FLT_NUM	(12)
#else
#define DRE_FLT_NUM	(11)
#endif

struct aal_backup { /* structure for backup AAL register value */
	unsigned int DRE_MAPPING;
	unsigned int DRE_FLT_FORCE[DRE_FLT_NUM];
	unsigned int CABC_00;
	unsigned int CABC_02;
	unsigned int CABC_GAINLMT[11];
};
struct aal_backup g_aal_backup;
static int g_aal_io_mask;

static void ddp_aal_backup(void)
{
	int i;

	g_aal_backup.CABC_00 = DISP_REG_GET(DISP_AAL_CABC_00);
	g_aal_backup.CABC_02 = DISP_REG_GET(DISP_AAL_CABC_02);
	if (g_aal_hw_offset == true) {
#if defined(CONFIG_MACH_MT6799)
		for (i = 0; i <= 10; i++)
			g_aal_backup.CABC_GAINLMT[i] = DISP_REG_GET(DISP_AAL_CABC_GAINLMT_TBL_2(i));

		g_aal_backup.DRE_MAPPING = DISP_REG_GET(DISP_AAL_DRE_MAPPING_00_2);
#endif
	} else {
		for (i = 0; i <= 10; i++)
			g_aal_backup.CABC_GAINLMT[i] = DISP_REG_GET(DISP_AAL_CABC_GAINLMT_TBL(i));

		g_aal_backup.DRE_MAPPING = DISP_REG_GET(DISP_AAL_DRE_MAPPING_00);
	}

	if (g_aal_dre_offset_separate == true) {
#if defined(CONFIG_MACH_MT6799)
		for (i = 0; i < (DRE_FLT_NUM-1); i++)
			g_aal_backup.DRE_FLT_FORCE[i] = DISP_REG_GET(DISP_AAL_DRE_FLT_FORCE(i));
		g_aal_backup.DRE_FLT_FORCE[11] = DISP_REG_GET(DISP_AAL_DRE_FLT_FORCE_11);
#endif
	} else {
		for (i = 0; i < DRE_FLT_NUM; i++)
			g_aal_backup.DRE_FLT_FORCE[i] = DISP_REG_GET(DISP_AAL_DRE_FLT_FORCE(i));
	}

	g_aal_initialed = 1;

}

static void ddp_aal_restore(enum DISP_MODULE_ENUM module, void *cmq_handle)
{
	int i;
	const int offset = aal_get_offset(module);

	if (g_aal_initialed != 1)
		return;

	AAL_DBG("ddp_aal_restore, module(%d)", module);
	DISP_REG_SET(cmq_handle, DISP_AAL_CABC_00 + offset, g_aal_backup.CABC_00);
	DISP_REG_SET(cmq_handle, DISP_AAL_CABC_02 + offset, g_aal_backup.CABC_02);
	if (g_aal_hw_offset == true) {
#if defined(CONFIG_MACH_MT6799)
		for (i = 0; i <= 10; i++)
			DISP_REG_SET(cmq_handle, DISP_AAL_CABC_GAINLMT_TBL_2(i) + offset, g_aal_backup.CABC_GAINLMT[i]);

		DISP_REG_SET(cmq_handle, DISP_AAL_DRE_MAPPING_00_2 + offset, g_aal_backup.DRE_MAPPING);
#endif
	} else {
		for (i = 0; i <= 10; i++)
			DISP_REG_SET(cmq_handle, DISP_AAL_CABC_GAINLMT_TBL(i) + offset, g_aal_backup.CABC_GAINLMT[i]);

		DISP_REG_SET(cmq_handle, DISP_AAL_DRE_MAPPING_00 + offset, g_aal_backup.DRE_MAPPING);
	}
	if (g_aal_dre_offset_separate == true) {
#if defined(CONFIG_MACH_MT6799)
		for (i = 0; i < (DRE_FLT_NUM-1); i++)
			DISP_REG_SET(cmq_handle, DISP_AAL_DRE_FLT_FORCE(i) + offset, g_aal_backup.DRE_FLT_FORCE[i]);
		DISP_REG_SET(cmq_handle, DISP_AAL_DRE_FLT_FORCE_11 + offset, g_aal_backup.DRE_FLT_FORCE[11]);
#endif
	} else {
		for (i = 0; i < DRE_FLT_NUM; i++)
			DISP_REG_SET(cmq_handle, DISP_AAL_DRE_FLT_FORCE(i) + offset, g_aal_backup.DRE_FLT_FORCE[i]);
	}
}

static int aal_clock_on(enum DISP_MODULE_ENUM module, void *cmq_handle)
{
#if defined(CONFIG_MACH_ELBRUS) || defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS)
	/* aal is DCM , do nothing */
#else
#ifdef ENABLE_CLK_MGR
	if (module == AAL0_MODULE_NAMING) {
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_AAL, "aal");
#else
		ddp_clk_enable(AAL0_CLK_NAMING);
		AAL_DBG("aal_clock_on CG 0x%x", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif		/* CONFIG_MTK_CLKMGR */
	}
#if defined(CONFIG_MACH_MT6799)
	else if (module == DISP_MODULE_AAL1) {
#ifndef CONFIG_MTK_CLKMGR
		ddp_clk_enable(DISP0_DISP_AAL1);
#endif		/* not define CONFIG_MTK_CLKMGR */
	}
#endif
#endif		/* ENABLE_CLK_MGR */
#endif

	if (module == AAL0_MODULE_NAMING)
		g_aal0_is_clock_on = true;
	else if (g_aal0_is_clock_on == true)
		ddp_aal_backup();

	ddp_aal_restore(module, cmq_handle);

	return 0;
}

static int aal_clock_off(enum DISP_MODULE_ENUM module, void *cmq_handle)
{
	if (module == AAL0_MODULE_NAMING)
		ddp_aal_backup();

	AAL_DBG("aal_clock_off");
#if defined(CONFIG_MACH_ELBRUS) || defined(CONFIG_MACH_MT6757) || defined(CONFIG_MACH_KIBOPLUS)
	/* aal is DCM , do nothing */
#else
#ifdef ENABLE_CLK_MGR
	if (module == AAL0_MODULE_NAMING) {
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_AAL, "aal");
#else
		ddp_clk_disable(AAL0_CLK_NAMING);
#endif		/* CONFIG_MTK_CLKMGR */
	}
#if defined(CONFIG_MACH_MT6799)
	else if (module == DISP_MODULE_AAL1) {
#ifndef CONFIG_MTK_CLKMGR
		ddp_clk_disable(DISP0_DISP_AAL1);
#endif		/* not define CONFIG_MTK_CLKMGR */
	}
#endif
#endif		/* ENABLE_CLK_MGR */
#endif

	if (module == AAL0_MODULE_NAMING)
		g_aal0_is_clock_on = false;
#ifdef CONFIG_MTK_AAL_SUPPORT
	g_aal_reset_count = true;
#endif
	return 0;
}

static int aal_init(enum DISP_MODULE_ENUM module, void *cmq_handle)
{
	aal_clock_on(module, cmq_handle);
#if defined(CONFIG_MACH_MT6799)
	if (mt_get_chip_sw_ver() >= CHIP_SW_VER_02)
		g_aal_hw_offset = true;
	else
		g_aal_dre_offset_separate = true;
#endif

	return 0;
}

static int aal_deinit(enum DISP_MODULE_ENUM module, void *cmq_handle)
{
	aal_clock_off(module, cmq_handle);
	return 0;
}

static int aal_set_listener(enum DISP_MODULE_ENUM module, ddp_module_notify notify)
{
	g_ddp_notify = notify;
	return 0;
}

int aal_bypass(enum DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass)
		relay = 1;

	DISP_REG_MASK(NULL, DISP_AAL_CFG + aal_get_offset(module), relay, 0x1);

	AAL_DBG("Module(%d) aal_bypass(bypass = %d)", module, bypass);

	return 0;
}

int aal_is_partial_support(void)
{
	int allowPartial;
#ifdef CONFIG_MTK_AAL_SUPPORT
	allowPartial = atomic_read(&g_aal_allowPartial);
#else
	allowPartial = 1;
#endif
	AAL_DBG("aal_is_partial_support=%d", allowPartial);

	return allowPartial;
}

int aal_request_partial_support(int partial)
{
	unsigned long flags;

	spin_lock_irqsave(&g_aal_hist_lock, flags);
	g_aal_hist.requestPartial = partial;
	spin_unlock_irqrestore(&g_aal_hist_lock, flags);

	AAL_DBG("aal_request_partial_support: %d", partial);

	return 0;
}

#ifdef AAL_SUPPORT_PARTIAL_UPDATE
static int _aal_partial_update(enum DISP_MODULE_ENUM module, void *arg, void *cmdq)
{
	struct disp_rect *roi = (struct disp_rect *) arg;
	int width = roi->width;
	int height = roi->height;

	DISP_REG_SET(cmdq, DISP_AAL_SIZE + aal_get_offset(module), (width << 16) | height);
	AAL_DBG("Module(%d) _aal_partial_update:w=%d h=%d", module, width, height);
	return 0;
}

static int aal_ioctl(enum DISP_MODULE_ENUM module, void *handle,
		enum DDP_IOCTL_NAME ioctl_cmd, void *params)
{
	int ret = -1;

	if (ioctl_cmd == DDP_PARTIAL_UPDATE) {
		_aal_partial_update(module, params, handle);
		ret = 0;
	}

	return ret;
}
#endif

static int aal_io(enum DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
	int ret = 0;

	if (g_aal_io_mask != 0) {
		AAL_DBG("aal_ioctl masked");
		return ret;
	}

	switch (msg) {
	case DISP_IOCTL_AAL_EVENTCTL:
		{
			int enabled;

			if (copy_from_user(&enabled, (void *)arg, sizeof(enabled))) {
				AAL_ERR("DISP_IOCTL_AAL_EVENTCTL: copy_from_user() failed");
				return -EFAULT;
			}

			disp_aal_set_interrupt_by_module(module, enabled);

			if (enabled)
				disp_aal_trigger_refresh(AAL_REFRESH_33MS);

			break;
		}
	case DISP_IOCTL_AAL_GET_HIST:
		{
			disp_aal_wait_hist(60);

			if (disp_aal_copy_hist_to_user((DISP_AAL_HIST *) arg) < 0) {
				AAL_ERR("DISP_IOCTL_AAL_GET_HIST: copy_to_user() failed");
				return -EFAULT;
			}
			break;
		}
	case DISP_IOCTL_AAL_INIT_REG:
		{
			if (disp_aal_set_init_reg((DISP_AAL_INITREG *) arg, module, cmdq) < 0) {
				AAL_ERR("DISP_IOCTL_AAL_INIT_REG: failed");
				return -EFAULT;
			}
			break;
		}
	case DISP_IOCTL_AAL_SET_PARAM:
		{
			if (disp_aal_set_param((DISP_AAL_PARAM *) arg, module, cmdq) < 0) {
				AAL_ERR("DISP_IOCTL_AAL_SET_PARAM: failed");
				return -EFAULT;
			}
			break;
		}
	}

	return ret;
}

struct DDP_MODULE_DRIVER ddp_driver_aal = {
	.init = aal_init,
	.deinit = aal_deinit,
	.config = aal_config,
	.start = NULL,
	.trigger = NULL,
	.stop = NULL,
	.reset = NULL,
	.power_on = aal_clock_on,
	.power_off = aal_clock_off,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = NULL,
	.bypass = aal_bypass,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
	.set_listener = aal_set_listener,
	.cmd = aal_io,
#ifdef AAL_SUPPORT_PARTIAL_UPDATE
	.ioctl = aal_ioctl,
#endif
};


/* ---------------------------------------------------------------------- */
/* Test code */
/* Will not be linked in user build. */
/* ---------------------------------------------------------------------- */

#define AAL_TLOG(fmt, arg...) pr_debug("[AAL] " fmt "\n", ##arg)

static void aal_test_en(enum DISP_MODULE_ENUM module, const char *cmd)
{
	int en = ((cmd[0] == '0') ? 0 : 1);
	const int offset = aal_get_offset(module);

	DISP_REG_SET(NULL, DISP_AAL_EN + offset, en);
	AAL_TLOG("EN = %d, read = %d", en, DISP_REG_GET(DISP_AAL_EN + offset));
}


static void aal_dump_histogram(void)
{
	unsigned long flags;
	DISP_AAL_HIST *hist;
	int i;

	hist = kmalloc(sizeof(DISP_AAL_HIST), GFP_KERNEL);
	if (hist != NULL) {
		spin_lock_irqsave(&g_aal_hist_lock, flags);
		memcpy(hist, &g_aal_hist, sizeof(DISP_AAL_HIST));
		spin_unlock_irqrestore(&g_aal_hist_lock, flags);

		for (i = 0; i + 8 < AAL_HIST_BIN; i += 8) {
			AAL_TLOG("Hist[%d..%d] = %6d %6d %6d %6d %6d %6d %6d %6d",
				 i, i + 7, hist->maxHist[i], hist->maxHist[i + 1],
				 hist->maxHist[i + 2], hist->maxHist[i + 3], hist->maxHist[i + 4],
				 hist->maxHist[i + 5], hist->maxHist[i + 6], hist->maxHist[i + 7]);
		}
		for (; i < AAL_HIST_BIN; i++)
			AAL_TLOG("Hist[%d] = %6d", i, hist->maxHist[i]);

		kfree(hist);
	}
}


static void aal_test_ink(enum DISP_MODULE_ENUM module, const char *cmd)
{
	int en = (cmd[0] - '0');
	const unsigned long cabc_04 = DISP_AAL_CABC_00 + 0x4 * 4;
	const int offset = aal_get_offset(module);

	switch (en) {
	case 1:
		DISP_REG_SET(NULL, cabc_04 + offset, (1 << 31) | (511 << 18));
		break;
	case 2:
		DISP_REG_SET(NULL, cabc_04 + offset, (1 << 31) | (511 << 9));
		break;
	case 3:
		DISP_REG_SET(NULL, cabc_04 + offset, (1 << 31) | (511 << 0));
		break;
	case 4:
		DISP_REG_SET(NULL, cabc_04 + offset, (1 << 31) | (511 << 18) | (511 << 9) | 511);
		break;
	default:
		DISP_REG_SET(NULL, cabc_04 + offset, 0);
		break;
	}
}


static void aal_ut_cmd(const char *cmd)
{
	if (strncmp(cmd, "reset", 5) == 0) {
		g_aal_initialed = 0;
		memset(&g_aal_backup, 0, sizeof(struct aal_backup));
		AAL_DBG("ut:reset");
	} else if (strncmp(cmd, "ioctl_on", 8) == 0) {
		g_aal_io_mask = 0;
		AAL_DBG("ut:ioctl on");
	} else if (strncmp(cmd, "ioctl_off", 9) == 0) {
		g_aal_io_mask = 1;
		AAL_DBG("ut:ioctl off");
	}
}

void aal_test(const char *cmd, char *debug_output)
{
	enum DISP_MODULE_ENUM module = AAL0_MODULE_NAMING;
	int i;
	int config_module_num = 1;
#if defined(CONFIG_MACH_MT6799)
	if (primary_display_get_pipe_status() == DUAL_PIPE)
		config_module_num = AAL_TOTAL_MODULE_NUM;
#endif
	debug_output[0] = '\0';
	AAL_TLOG("aal_test(%s)", cmd);

	if (strncmp(cmd, "en:", 3) == 0) {
		for (i = 0; i < config_module_num; i++) {
			module += i;
			aal_test_en(module, cmd + 3);
		}
	} else if (strncmp(cmd, "histogram", 5) == 0) {
		aal_dump_histogram();
	} else if (strncmp(cmd, "ink:", 4) == 0) {
		for (i = 0; i < config_module_num; i++) {
			module += i;
			aal_test_ink(module, cmd + 4);
		}
		disp_aal_trigger_refresh(AAL_REFRESH_17MS);
	} else if (strncmp(cmd, "bypass:", 7) == 0) {
		int bypass = (cmd[7] == '1');

		for (i = 0; i < config_module_num; i++) {
			module += i;
			aal_bypass(module, bypass);
		}
	} else if (strncmp(cmd, "ut:", 3) == 0) { /* debug command for UT */
		aal_ut_cmd(cmd + 3);
	}
}
