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

#ifndef _MDLA_DVFS_H_
#define _MDLA_DVFS_H_

/* Performance Measure */
#ifdef MDLA_TRACE_ENABLED
#include <linux/kallsyms.h>
#include <linux/trace_events.h>
#include <linux/preempt.h>
static unsigned long __read_mostly mdla_tracing_writer;
#define mdla_trace_begin(format, args...) \
{ \
	if (mdla_tracing_writer == 0) \
		mdla_tracing_writer = \
			kallsyms_lookup_name("tracing_mark_write"); \
	preempt_disable(); \
	event_trace_printk(mdla_tracing_writer, \
			"B|%d|" format "\n", current->tgid, ##args); \
	preempt_enable(); \
}

#define mdla_trace_end() \
{ \
	preempt_disable(); \
	event_trace_printk(mdla_tracing_writer, "E\n"); \
	preempt_enable(); \
}
#else
#define mdla_trace_begin(...)
#define mdla_trace_end()
#endif

/* ++++++++++++++++++++++++++++++++++*/
/* |opp_index  |   mdla frequency  |        power             */
/* ------------------------------------------*/
/* |      0         |   788 MHz          |        336 mA           */
/* ------------------------------------------*/
/* |      1         |   700 MHz          |        250 mA           */
/* ------------------------------------------*/
/* |      2         |   624 MHz          |        221 mA           */
/* ------------------------------------------*/
/* |      3         |   606 MHz          |        208 mA           */
/* ------------------------------------------*/
/* |      4         |   594 MHz          |        140 mA           */
/* ------------------------------------------*/
/* |      5         |   546 MHz          |        120 mA           */
/* ------------------------------------------*/
/* |      6         |   525 MHz          |        114 mA           */
/* ------------------------------------------*/
/* |      7         |   450 MHz          |         84 mA           */
/* ------------------------------------------*/
/* |      8         |   416 MHz          |        336 mA           */
/* ------------------------------------------*/
/* |      9         |   364 MHz          |        250 mA           */
/* ------------------------------------------*/
/* |      10         |   312 MHz          |        221 mA           */
/* ------------------------------------------*/
/* |      11         |   273 MHz          |        208 mA           */
/* ------------------------------------------*/
/* |      12         |   208 MHz          |        140 mA           */
/* ------------------------------------------*/
/* |      13         |   137 MHz          |        120 mA           */
/* ------------------------------------------*/
/* |      14         |   52 MHz          |        114 mA           */
/* ------------------------------------------*/
/* |      15         |   26 MHz          |        114 mA           */
/* ------------------------------------------*/
/* ++++++++++++++++++++++++++++++++++*/

enum MDLA_OPP_INDEX {
	MDLA_OPP_0 = 0,
	MDLA_OPP_1 = 1,
	MDLA_OPP_2 = 2,
	MDLA_OPP_3 = 3,
	MDLA_OPP_4 = 4,
	MDLA_OPP_5 = 5,
	MDLA_OPP_6 = 6,
	MDLA_OPP_7 = 7,
	MDLA_OPP_8 = 8,
	MDLA_OPP_9 = 9,
	MDLA_OPP_10 = 10,
	MDLA_OPP_11 = 11,
	MDLA_OPP_12 = 12,
	MDLA_OPP_13 = 13,
	MDLA_OPP_14 = 14,
	MDLA_OPP_15 = 15,
	MDLA_OPP_NUM
};

struct MDLA_OPP_INFO {
	enum MDLA_OPP_INDEX opp_index;
	int power;	/*mW*/
};

#define MDLA_MAX_NUM_STEPS               (16)
#define MDLA_MAX_NUM_OPPS                (16)
//#define MTK_MDLA_CORE (1)
#define MTK_VPU_CORE (2)
struct mdla_dvfs_steps {
	uint32_t values[MDLA_MAX_NUM_STEPS];
	uint8_t count;
	uint8_t index;
	uint8_t opp_map[MDLA_MAX_NUM_OPPS];
};

struct mdla_dvfs_opps {
	struct mdla_dvfs_steps vcore;
	struct mdla_dvfs_steps vvpu;
	struct mdla_dvfs_steps vmdla;
	struct mdla_dvfs_steps dsp;	/* ipu_conn */
	struct mdla_dvfs_steps dspcore[MTK_VPU_CORE];	/* ipu_core# */
	struct mdla_dvfs_steps mdlacore;	/* ipu_core# */
	struct mdla_dvfs_steps ipu_if;	/* ipusys_vcore, interface */
	uint8_t index;
	uint8_t count;
};
enum mdlaPowerOnType {
	/* power on previously by setPower */
	VPT_PRE_ON		= 1,

	/* power on by enque */
	VPT_ENQUE_ON	= 2,

	/* power on by enque, but want to immediately off(when exception) */
	VPT_IMT_OFF		= 3,
};

struct mdla_power {
	uint8_t opp_step;
	uint8_t freq_step;
	uint32_t bw; /* unit: MByte/s */

	/* align with core index defined in user space header file */
	unsigned int core;
};


extern struct MDLA_OPP_INFO mdla_power_table[MDLA_OPP_NUM];
extern int32_t mdla_thermal_en_throttle_cb(uint8_t vmdla_opp, uint8_t mdla_opp);
extern int32_t mdla_thermal_dis_throttle_cb(void);
int mdla_set_power(struct mdla_power *power);
int mdla_quick_suspend(int core);
int mdla_boot_up(int core);
int mdla_shut_down(int core);

#endif
