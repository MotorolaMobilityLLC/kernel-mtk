/*
 * Copyright (C) 2016 MediaTek Inc.
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
#define  MET_USER_EVENT_SUPPORT /*Turn met user event*/

#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <mt-plat/met_drv.h>
#include "vpu_cmn.h"
#include "vpu_reg.h"

/* MET: define to enable MET */
#if defined(VPU_MET_READY)
#define CREATE_TRACE_POINTS
#include "met_vpusys_events.h"
#endif


#define DEFAULT_POLLING_PERIOD_NS (5000000)
#define COUNTER_PID (0) /*(65535)*/ /*task_pid_nr(current)*/
#define BEGINEND_PID (0) /*(65535)*/ /*task_pid_nr(current)*/
#define ASYNCBEGINEND_PID (0) /*(65535)*/ /*task_pid_nr(current)*/



static int m_vpu_profile_state;
static struct hrtimer hr_timer;
static uint64_t vpu_base[MTK_VPU_CORE];
static bool vpu_on[MTK_VPU_CORE];
static struct mutex profile_mutex;
static int profiling_counter;
static int stop_result;
/*
 * mini trace system
 * [KATRACE] mini trace system
 */
#define KATRACE_MESSAGE_LENGTH 1024

static noinline int tracing_mark_write(const char *buf)
{
	TRACE_PUTS(buf);
	return 0;
}

/*
 * [KATRACE] Begin-End
 */
#define KATRACE_BEGIN(name) katrace_begin_body(name)
void katrace_begin_body(const char *name)
{
	char buf[KATRACE_MESSAGE_LENGTH];
	int len = snprintf(buf, sizeof(buf), "B|%d|%s", BEGINEND_PID, name);

	if (len >= (int) sizeof(buf)) {
		LOG_DBG("Truncated name in %s: %s\n", __func__, name);
		len = sizeof(buf) - 1;
	}
	tracing_mark_write(buf);
}

#define KATRACE_END() katrace_end()
inline void katrace_end(void)
{
	char c = 'E';

	tracing_mark_write(&c);
}


#define WRITE_MSG(format_begin, format_end, pid, name, value) { \
	char buf[KATRACE_MESSAGE_LENGTH]; \
	int len = snprintf(buf, sizeof(buf), format_begin "%s" format_end, pid, \
		name, value); \
	if (len >= (int) sizeof(buf)) { \
		/* Given the sizeof(buf), and all of the current format buffers, */ \
		/* it is impossible for name_len to be < 0 if len >= sizeof(buf). */ \
		int name_len = strlen(name) - (len - sizeof(buf)) - 1; \
		/* Truncate the name to make the message fit. */ \
		LOG_DBG("Truncated name in %s: %s\n", __func__, name); \
		len = snprintf(buf, sizeof(buf), format_begin "%.*s" format_end, pid, \
			name_len, name, value); \
	} \
	tracing_mark_write(buf); \
}

/*
 * [KATRACE] Counter Integer
 */
#define KATRACE_INT(name, value) katrace_int_body(name, value)
void katrace_int_body(const char *name, int32_t value)
{
	WRITE_MSG("C|%d|", "|%d", COUNTER_PID, name, value);
}

/*
 * [KATRACE] Async Begin-End
 */
#define KATRACE_ASYNC_BEGIN(name, cookie) \
	katrace_async_begin_body(name, cookie)
void katrace_async_begin_body(const char *name, int32_t cookie)
{
	WRITE_MSG("S|%d|", "|%d", ASYNCBEGINEND_PID, name, cookie);
}

#define KATRACE_ASYNC_END(name, cookie) katrace_async_end_body(name, cookie)
void katrace_async_end_body(const char *name, int32_t cookie)
{
	WRITE_MSG("F|%d|", "|%d", ASYNCBEGINEND_PID, name, cookie);
}


/*
* VPU event based MET funcs
*/
void vpu_met_event_enter(int core, int algo_id, int vcore_opp,
	int dsp_freq, int ipu_if_freq, int dsp1_freq, int dsp2_freq)
{
	#if defined(VPU_MET_READY)
	trace_VPU__D2D_enter(core, algo_id, vcore_opp, dsp_freq, ipu_if_freq, dsp1_freq, dsp2_freq);
	#endif
}

void vpu_met_event_leave(int core, int algo_id)
{
	#if defined(VPU_MET_READY)
	trace_VPU__D2D_leave(core, algo_id, 0);
	#endif
}


/*
 * VPU counter reader
 */
static void vpu_profile_core_read(int core)
{
	uint64_t ptr_ctrl;
	uint64_t ptr_axi_2;
	int value1 = 0, value2 = 0, value3 = 0, value4 = 0;

	ptr_ctrl = vpu_base[core] + g_vpu_reg_descs[REG_CTRL].offset;
	ptr_axi_2 = vpu_base[core] + g_vpu_reg_descs[REG_AXI_DEFAULT2].offset;

	/* enable */
	VPU_SET_BIT(ptr_axi_2, 0);
	VPU_SET_BIT(ptr_axi_2, 1);
	VPU_SET_BIT(ptr_axi_2, 2);
	VPU_SET_BIT(ptr_axi_2, 3);
	/* the following two are set after vpu boot-up */
	/*VPU_SET_BIT(ptr_ctrl, 31);*/
	/*VPU_SET_BIT(ptr_ctrl, 26);*/

	/* read register and send to met */
	value1 = vpu_read_reg32(vpu_base[core], DEBUG_BASE_OFFSET + 0x80);
	value2 = vpu_read_reg32(vpu_base[core], DEBUG_BASE_OFFSET + 0x84);
	value3 = vpu_read_reg32(vpu_base[core], DEBUG_BASE_OFFSET + 0x88);
	value4 = vpu_read_reg32(vpu_base[core], DEBUG_BASE_OFFSET + 0x8C);
	LOG_DBG("[vpu_profile_%d] read %d/%d/%d/%d\n", core, value1, value2, value3, value4);
	#if defined(VPU_MET_READY)
	trace_VPU__polling(core, value1, value2, value3, value4);
	#endif
}

static void vpu_profile_register_read(void)
{
	int i = 0;

	for (i = 0 ; i < MTK_VPU_CORE; i++)
		if (vpu_on[i])
			vpu_profile_core_read(i);
}




/*
 * VPU Polling Function
 */
static enum hrtimer_restart vpu_profile_polling(struct hrtimer *timer)
{
	LOG_DBG("vpu_profile_polling +\n");
	/*call functions need to be called periodically*/
	vpu_profile_register_read();

	hrtimer_forward_now(&hr_timer, ns_to_ktime(DEFAULT_POLLING_PERIOD_NS));
	LOG_DBG("vpu_profile_polling -\n");
	return HRTIMER_RESTART;
}



static int vpu_profile_timer_start(void)
{
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = vpu_profile_polling;
	hrtimer_start(&hr_timer, ns_to_ktime(DEFAULT_POLLING_PERIOD_NS), HRTIMER_MODE_REL);
	return 0;
}

static int vpu_profile_timer_try_stop(void)
{
	int ret = 0;

	ret = hrtimer_try_to_cancel(&hr_timer);
	return ret;
}

static int vpu_profile_timer_stop(void)
{
	hrtimer_cancel(&hr_timer);
	return 0;
}


static int vpu_profile_start(void)
{
	LOG_DBG("vpu_profile_start +\n");
	vpu_profile_timer_start();
	LOG_DBG("vpu_profile_start -\n");
	return 0;
}

static int vpu_profile_stop(int type)
{
	LOG_INF("vpu_profile_stop (%d)+\n", type);
	if (type)
		stop_result = vpu_profile_timer_try_stop();
	else
		vpu_profile_timer_stop();

	LOG_DBG("vpu_profile_stop (%d/%d)-\n", type, stop_result);
	return 0;
}


int vpu_profile_state_set(int core, int val)
{
	switch (val) {
	case 0:
		mutex_lock(&profile_mutex);
		profiling_counter--;
		vpu_on[core] = false;
		LOG_INF("[vpu_profile_%d->%d] (stop) counter(%d, %d)\n",
			core, vpu_on[core], m_vpu_profile_state, profiling_counter);
		if (profiling_counter == 0) {
			m_vpu_profile_state = val;
			mutex_unlock(&profile_mutex);
			vpu_profile_stop(0);
		} else {
			mutex_unlock(&profile_mutex);
		}
		break;
	case 1:
		mutex_lock(&profile_mutex);
		profiling_counter++;
		vpu_on[core] = true;
		LOG_INF("[vpu_profile_%d->%d] (start) counter(%d, %d)\n",
			core, vpu_on[core], m_vpu_profile_state, profiling_counter);
		if (profiling_counter == 1 && m_vpu_profile_state == 0) {
			m_vpu_profile_state = val;
			mutex_unlock(&profile_mutex);
			vpu_profile_start();
		} else {
			mutex_unlock(&profile_mutex);
		}
		break;
	default:
		/*unsupported command*/
		return -1;
	}

	LOG_DBG("vpu_profile (%d) -\n", val);
	return 0;
}

int vpu_profile_state_get(void)
{
	return m_vpu_profile_state;
}


int vpu_init_profile(int core, struct vpu_device *device)
{
	int i = 0;

	m_vpu_profile_state = 0;
	stop_result = 0;
	vpu_base[core] = device->vpu_base[core];
	for (i = 0 ; i < MTK_VPU_CORE; i++)
		vpu_on[i] = false;
	profiling_counter = 0;
	mutex_init(&profile_mutex);

	return 0;
}

int vpu_uninit_profile(void)
{
	int i = 0;

	for (i = 0 ; i < MTK_VPU_CORE; i++)
		vpu_on[i] = false;
	if (stop_result)
		vpu_profile_stop(0);

	return 0;
}

