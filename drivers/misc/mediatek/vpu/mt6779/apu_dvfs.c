/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/devfreq.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <mt-plat/upmu_common.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>


//#include "governor.h"

#include <mt-plat/aee.h>
#include <spm/mtk_spm.h>

#include "apu_dvfs.h"
#include "vpu_cmn.h"

#include <linux/regulator/consumer.h>

/*regulator id*/
static struct regulator *vvpu_reg_id;
static struct regulator *vmdla_reg_id;


static bool vvpu_DVFS_is_paused_by_ptpod;
static bool vmdla_DVFS_is_paused_by_ptpod;


static bool vpu_opp_ready;
static bool mdla_opp_ready;
#define VPU_DVFS_OPP_MAX	  (16)
#define MDLA_DVFS_OPP_MAX	  (16)


#define VPU_DVFS_FREQ0	 (700000)	/* KHz */
#define VPU_DVFS_FREQ1	 (624000)	/* KHz */
#define VPU_DVFS_FREQ2	 (606000)	/* KHz */
#define VPU_DVFS_FREQ3	 (594000)	/* KHz */
#define VPU_DVFS_FREQ4	 (560000)	/* KHz */
#define VPU_DVFS_FREQ5	 (525000)	/* KHz */
#define VPU_DVFS_FREQ6	 (450000)	/* KHz */
#define VPU_DVFS_FREQ7	 (416000)	/* KHz */
#define VPU_DVFS_FREQ8	 (364000)	/* KHz */
#define VPU_DVFS_FREQ9	 (312000)	/* KHz */
#define VPU_DVFS_FREQ10	  (273000)	 /* KHz */
#define VPU_DVFS_FREQ11	  (208000)	 /* KHz */
#define VPU_DVFS_FREQ12	  (137000)	 /* KHz */
#define VPU_DVFS_FREQ13	  (104000)	 /* KHz */
#define VPU_DVFS_FREQ14	  (52000)	 /* KHz */
#define VPU_DVFS_FREQ15	  (26000)	 /* KHz */


#define VPU_DVFS_VOLT0	 (82500)	/* mV x 100 */
#define VPU_DVFS_VOLT1	 (82500)	/* mV x 100 */
#define VPU_DVFS_VOLT2	 (82500)	/* mV x 100 */
#define VPU_DVFS_VOLT3	 (82500)	/* mV x 100 */
#define VPU_DVFS_VOLT4	 (82500)	/* mV x 100 */
#define VPU_DVFS_VOLT5	 (72500)	/* mV x 100 */
#define VPU_DVFS_VOLT6	 (72500)	/* mV x 100 */
#define VPU_DVFS_VOLT7	 (72500) /* mV x 100 */
#define VPU_DVFS_VOLT8	 (72500) /* mV x 100 */
#define VPU_DVFS_VOLT9	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT10	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT11	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT12	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT13	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT14	 (65000)	/* mV x 100 */
#define VPU_DVFS_VOLT15	 (65000)	/* mV x 100 */

#define MDLA_DVFS_FREQ0	 (788000)	/* KHz */
#define MDLA_DVFS_FREQ1	 (700000)	/* KHz */
#define MDLA_DVFS_FREQ2	 (624000)	/* KHz */
#define MDLA_DVFS_FREQ3	 (606000)	/* KHz */
#define MDLA_DVFS_FREQ4	 (594000)	/* KHz */
#define MDLA_DVFS_FREQ5	 (546000)	/* KHz */
#define MDLA_DVFS_FREQ6	 (525000)	/* KHz */
#define MDLA_DVFS_FREQ7	 (450000)	/* KHz */
#define MDLA_DVFS_FREQ8	 (416000)	/* KHz */
#define MDLA_DVFS_FREQ9	 (364000)	/* KHz */
#define MDLA_DVFS_FREQ10	  (312000)	 /* KHz */
#define MDLA_DVFS_FREQ11	  (273000)	 /* KHz */
#define MDLA_DVFS_FREQ12	  (208000)	 /* KHz */
#define MDLA_DVFS_FREQ13	  (137000)	 /* KHz */
#define MDLA_DVFS_FREQ14	  (52000)	 /* KHz */
#define MDLA_DVFS_FREQ15	  (26000)	 /* KHz */


#define MDLA_DVFS_VOLT0	 (82500)	/* mV x 100 */
#define MDLA_DVFS_VOLT1	 (82500)	/* mV x 100 */
#define MDLA_DVFS_VOLT2	 (82500)	/* mV x 100 */
#define MDLA_DVFS_VOLT3	 (72500)	/* mV x 100 */
#define MDLA_DVFS_VOLT4	 (72500)	/* mV x 100 */
#define MDLA_DVFS_VOLT5	 (72500)	/* mV x 100 */
#define MDLA_DVFS_VOLT6	 (72500)	/* mV x 100 */
#define MDLA_DVFS_VOLT7	 (72500) /* mV x 100 */
#define MDLA_DVFS_VOLT8	 (72500) /* mV x 100 */
#define MDLA_DVFS_VOLT9	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT10	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT11	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT12	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT13	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT14	 (65000)	/* mV x 100 */
#define MDLA_DVFS_VOLT15	 (65000)	/* mV x 100 */

#define VPUOP(khz, volt, idx) \
{ \
	.vpufreq_khz = khz, \
	.vpufreq_volt = volt, \
	.vpufreq_idx = idx, \
}
#define MDLAOP(khz, volt, idx) \
{ \
	.mdlafreq_khz = khz, \
	.mdlafreq_volt = volt, \
	.mdlafreq_idx = idx, \
}

static struct vpu_opp_table_info vpu_opp_table_default[] = {
	VPUOP(VPU_DVFS_FREQ0, VPU_DVFS_VOLT0,  0),
	VPUOP(VPU_DVFS_FREQ1, VPU_DVFS_VOLT1,  1),
	VPUOP(VPU_DVFS_FREQ2, VPU_DVFS_VOLT2,  2),
	VPUOP(VPU_DVFS_FREQ3, VPU_DVFS_VOLT3,  3),
	VPUOP(VPU_DVFS_FREQ4, VPU_DVFS_VOLT4,  4),
	VPUOP(VPU_DVFS_FREQ5, VPU_DVFS_VOLT5,  5),
	VPUOP(VPU_DVFS_FREQ6, VPU_DVFS_VOLT6,  6),
	VPUOP(VPU_DVFS_FREQ7, VPU_DVFS_VOLT7,  7),
	VPUOP(VPU_DVFS_FREQ8, VPU_DVFS_VOLT8,  8),
	VPUOP(VPU_DVFS_FREQ9, VPU_DVFS_VOLT9,  9),
	VPUOP(VPU_DVFS_FREQ10, VPU_DVFS_VOLT10,  10),
	VPUOP(VPU_DVFS_FREQ11, VPU_DVFS_VOLT11,  11),
	VPUOP(VPU_DVFS_FREQ12, VPU_DVFS_VOLT12,  12),
	VPUOP(VPU_DVFS_FREQ13, VPU_DVFS_VOLT13,  13),
	VPUOP(VPU_DVFS_FREQ14, VPU_DVFS_VOLT14,  14),
	VPUOP(VPU_DVFS_FREQ15, VPU_DVFS_VOLT15,  15),
};

static struct vpu_opp_table_info vpu_opp_table[] = {
	VPUOP(VPU_DVFS_FREQ0, VPU_DVFS_VOLT0,  0),
	VPUOP(VPU_DVFS_FREQ1, VPU_DVFS_VOLT1,  1),
	VPUOP(VPU_DVFS_FREQ2, VPU_DVFS_VOLT2,  2),
	VPUOP(VPU_DVFS_FREQ3, VPU_DVFS_VOLT3,  3),
	VPUOP(VPU_DVFS_FREQ4, VPU_DVFS_VOLT4,  4),
	VPUOP(VPU_DVFS_FREQ5, VPU_DVFS_VOLT5,  5),
	VPUOP(VPU_DVFS_FREQ6, VPU_DVFS_VOLT6,  6),
	VPUOP(VPU_DVFS_FREQ7, VPU_DVFS_VOLT7,  7),
	VPUOP(VPU_DVFS_FREQ8, VPU_DVFS_VOLT8,  8),
	VPUOP(VPU_DVFS_FREQ9, VPU_DVFS_VOLT9,  9),
	VPUOP(VPU_DVFS_FREQ10, VPU_DVFS_VOLT10,  10),
	VPUOP(VPU_DVFS_FREQ11, VPU_DVFS_VOLT11,  11),
	VPUOP(VPU_DVFS_FREQ12, VPU_DVFS_VOLT12,  12),
	VPUOP(VPU_DVFS_FREQ13, VPU_DVFS_VOLT13,  13),
	VPUOP(VPU_DVFS_FREQ14, VPU_DVFS_VOLT14,  14),
	VPUOP(VPU_DVFS_FREQ15, VPU_DVFS_VOLT15,  15),
};

static struct mdla_opp_table_info mdla_opp_table_default[] = {
	MDLAOP(VPU_DVFS_FREQ0, VPU_DVFS_VOLT0,  0),
	MDLAOP(VPU_DVFS_FREQ1, VPU_DVFS_VOLT1,  1),
	MDLAOP(VPU_DVFS_FREQ2, VPU_DVFS_VOLT2,  2),
	MDLAOP(VPU_DVFS_FREQ3, VPU_DVFS_VOLT3,  3),
	MDLAOP(VPU_DVFS_FREQ4, VPU_DVFS_VOLT4,  4),
	MDLAOP(VPU_DVFS_FREQ5, VPU_DVFS_VOLT5,  5),
	MDLAOP(VPU_DVFS_FREQ6, VPU_DVFS_VOLT6,  6),
	MDLAOP(VPU_DVFS_FREQ7, VPU_DVFS_VOLT7,  7),
	MDLAOP(VPU_DVFS_FREQ8, VPU_DVFS_VOLT8,  8),
	MDLAOP(VPU_DVFS_FREQ9, VPU_DVFS_VOLT9,  9),
	MDLAOP(VPU_DVFS_FREQ10, VPU_DVFS_VOLT10,  10),
	MDLAOP(VPU_DVFS_FREQ11, VPU_DVFS_VOLT11,  11),
	MDLAOP(VPU_DVFS_FREQ12, VPU_DVFS_VOLT12,  12),
	MDLAOP(VPU_DVFS_FREQ13, VPU_DVFS_VOLT13,  13),
	MDLAOP(VPU_DVFS_FREQ14, VPU_DVFS_VOLT14,  14),
	MDLAOP(VPU_DVFS_FREQ15, VPU_DVFS_VOLT15,  15),
};

static struct mdla_opp_table_info mdla_opp_table[] = {
	MDLAOP(VPU_DVFS_FREQ0, VPU_DVFS_VOLT0,  0),
	MDLAOP(VPU_DVFS_FREQ1, VPU_DVFS_VOLT1,  1),
	MDLAOP(VPU_DVFS_FREQ2, VPU_DVFS_VOLT2,  2),
	MDLAOP(VPU_DVFS_FREQ3, VPU_DVFS_VOLT3,  3),
	MDLAOP(VPU_DVFS_FREQ4, VPU_DVFS_VOLT4,  4),
	MDLAOP(VPU_DVFS_FREQ5, VPU_DVFS_VOLT5,  5),
	MDLAOP(VPU_DVFS_FREQ6, VPU_DVFS_VOLT6,  6),
	MDLAOP(VPU_DVFS_FREQ7, VPU_DVFS_VOLT7,  7),
	MDLAOP(VPU_DVFS_FREQ8, VPU_DVFS_VOLT8,  8),
	MDLAOP(VPU_DVFS_FREQ9, VPU_DVFS_VOLT9,  9),
	MDLAOP(VPU_DVFS_FREQ10, VPU_DVFS_VOLT10,  10),
	MDLAOP(VPU_DVFS_FREQ11, VPU_DVFS_VOLT11,  11),
	MDLAOP(VPU_DVFS_FREQ12, VPU_DVFS_VOLT12,  12),
	MDLAOP(VPU_DVFS_FREQ13, VPU_DVFS_VOLT13,  13),
	MDLAOP(VPU_DVFS_FREQ14, VPU_DVFS_VOLT14,  14),
	MDLAOP(VPU_DVFS_FREQ15, VPU_DVFS_VOLT15,  15),
};

static struct apu_dvfs *dvfs;

static DEFINE_MUTEX(vpu_opp_lock);
static DEFINE_MUTEX(mdla_opp_lock);

static void dvfs_get_timestamp(char *p)
{
	u64 sec = local_clock();
	u64 usec = do_div(sec, 1000000000);

	do_div(usec, 1000000);
	sprintf(p, "%llu.%llu", sec, usec);
}

void dump_opp_table(void)
{
	int i;

	LOG_DBG("%s start\n", __func__);
	for (i = 0; i < VPU_DVFS_OPP_MAX; i++) {
		LOG_DBG("opp:%d, vol:%d, freq:%d\n", i
			, vpu_opp_table[i].vpufreq_volt
		, vpu_opp_table[i].vpufreq_khz);
	}
	LOG_DBG("%s end\n", __func__);
}
/************************************************
 * return current Vvpu voltage mV*100
 *************************************************/
unsigned int vvpu_get_cur_volt(void)
{
	return (regulator_get_voltage(vvpu_reg_id)/10);
}
EXPORT_SYMBOL(vvpu_get_cur_volt);

unsigned int vmdla_get_cur_volt(void)
{
	return (regulator_get_voltage(vmdla_reg_id)/10);
}
EXPORT_SYMBOL(vmdla_get_cur_volt);

unsigned int vvpu_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
	int i;			/* , idx; */

	mutex_lock(&vpu_opp_lock);

	for (i = 0; i < array_size; i++) {
		vpu_opp_table[i].vpufreq_volt = pmic_volt[i];
		LOG_DBG("%s opp:%d vol:%d", __func__, i, pmic_volt[i]);
		}
	dump_opp_table();

	vpu_opp_ready = true;
//
	mutex_unlock(&vpu_opp_lock);

	return 0;
}
EXPORT_SYMBOL(vvpu_update_volt);

unsigned int vmdla_update_volt(unsigned int pmic_volt[],
					unsigned int array_size)
{
	int i;

	mutex_lock(&mdla_opp_lock);

	for (i = 0; i < array_size; i++) {
		mdla_opp_table[i].mdlafreq_volt = pmic_volt[i];
		LOG_DBG("%s opp:%d vol:%d", __func__, i, pmic_volt[i]);
		}
	dump_opp_table();

	mdla_opp_ready = true;

	mutex_unlock(&mdla_opp_lock);

	return 0;
}
EXPORT_SYMBOL(vmdla_update_volt);

void vvpu_restore_default_volt(void)
{
	int i;

	mutex_lock(&vpu_opp_lock);

	for (i = 0; i < VPU_DVFS_OPP_MAX; i++) {
		vpu_opp_table[i].vpufreq_volt =
			vpu_opp_table_default[i].vpufreq_volt;
	}
	dump_opp_table();

	mutex_unlock(&vpu_opp_lock);
}
EXPORT_SYMBOL(vvpu_restore_default_volt);

void vmdla_restore_default_volt(void)
{
	int i;

	mutex_lock(&mdla_opp_lock);

	for (i = 0; i < MDLA_DVFS_OPP_MAX; i++) {
		mdla_opp_table[i].mdlafreq_volt =
			mdla_opp_table_default[i].mdlafreq_volt;
	}
	dump_opp_table();

	mutex_unlock(&mdla_opp_lock);
}
EXPORT_SYMBOL(vmdla_restore_default_volt);

/* API : get frequency via OPP table index */
unsigned int vpu_get_freq_by_idx(unsigned int idx)
{
	if (idx < VPU_DVFS_OPP_MAX)
		return vpu_opp_table[idx].vpufreq_khz;
	else
		return 0;
}
EXPORT_SYMBOL(vpu_get_freq_by_idx);

/* API : get voltage via OPP table index */
unsigned int vpu_get_volt_by_idx(unsigned int idx)
{
	if (idx < VPU_DVFS_OPP_MAX)
		return vpu_opp_table[idx].vpufreq_volt;
	else
		return 0;
}
EXPORT_SYMBOL(vpu_get_volt_by_idx);

/* API : get frequency via OPP table index */
unsigned int mdla_get_freq_by_idx(unsigned int idx)
{
	if (idx < MDLA_DVFS_OPP_MAX)
		return mdla_opp_table[idx].mdlafreq_khz;
	else
		return 0;
}
EXPORT_SYMBOL(mdla_get_freq_by_idx);

/* API : get voltage via OPP table index */
unsigned int mdla_get_volt_by_idx(unsigned int idx)
{
	if (idx < MDLA_DVFS_OPP_MAX)
		return mdla_opp_table[idx].mdlafreq_volt;
	else
		return 0;
}
EXPORT_SYMBOL(mdla_get_volt_by_idx);

/*
 * API : disable DVFS for PTPOD initializing
 */
void vpu_disable_by_ptpod(void)
{
	int ret = 0;

	/* Pause VPU DVFS */
	vvpu_DVFS_is_paused_by_ptpod = true;
	/*fix vvpu to 0.8V*/
	LOG_DBG("%s\n", __func__);
	mutex_lock(&vpu_opp_lock);
	/*--Set voltage--*/
	ret = regulator_set_voltage(vvpu_reg_id,
				10*VVPU_PTPOD_FIX_VOLT,
				10*VVPU_DVFS_VOLT0);
	if (ret)
	LOG_ERR("regulator_set_voltage  vvpu_reg_id  failed\n");
	mutex_unlock(&vpu_opp_lock);
#if 0
	/* Fix GPU @ 0.8V */
	for (i = 0; i < g_opp_idx_num; i++) {
		if (g_opp_table_default[i].gpufreq_volt
		    <= GPU_DVFS_PTPOD_DISABLE_VOLT) {
			target_idx = i;
			break;
		}
	}
	g_DVFS_off_by_ptpod_idx = (unsigned int)target_idx;
	mt_gpufreq_target(target_idx);

	/* Set GPU Buck to enter PWM mode */
	__mt_gpufreq_vgpu_set_mode(REGULATOR_MODE_FAST);

	gpufreq_pr_debug("@%s: DVFS is disabled by ptpod\n", __func__);
#endif
}
EXPORT_SYMBOL(vpu_disable_by_ptpod);

/*
 * API : enable DVFS for PTPOD initializing
 */
void vpu_enable_by_ptpod(void)
{
	/* Freerun VPU DVFS */
	vvpu_DVFS_is_paused_by_ptpod = false;
}
EXPORT_SYMBOL(vpu_enable_by_ptpod);


bool get_vvpu_DVFS_is_paused_by_ptpod(void)
{
	/* Freerun VPU DVFS */
	return vvpu_DVFS_is_paused_by_ptpod;
}
EXPORT_SYMBOL(get_vvpu_DVFS_is_paused_by_ptpod);

/*
 * API : disable DVFS for PTPOD initializing
 */
void mdla_disable_by_ptpod(void)
{
	int ret = 0;

	/* Pause VPU DVFS */
	vmdla_DVFS_is_paused_by_ptpod = true;
	LOG_DBG("%s\n", __func__);
	mutex_lock(&mdla_opp_lock);
	/*--Set voltage--*/
	ret = regulator_set_voltage(vmdla_reg_id,
				10*VMDLA_PTPOD_FIX_VOLT,
				10*VMDLA_DVFS_VOLT0);
	if (ret)
	LOG_ERR("regulator_set_voltage  vmdla_reg_id  failed\n");
	mutex_unlock(&mdla_opp_lock);
}
EXPORT_SYMBOL(mdla_disable_by_ptpod);

/*
 * API : enable DVFS for PTPOD initializing
 */
void mdla_enable_by_ptpod(void)
{
	/* Freerun VPU DVFS */
	vmdla_DVFS_is_paused_by_ptpod = false;
}
EXPORT_SYMBOL(mdla_enable_by_ptpod);

bool get_vmdla_DVFS_is_paused_by_ptpod(void)
{
	/* Freerun MDLA DVFS */
	return vmdla_DVFS_is_paused_by_ptpod;
}
EXPORT_SYMBOL(get_vmdla_DVFS_is_paused_by_ptpod);

static int commit_data(int type, int data)
{
	int ret = 0;
	int level = 16, opp = 16;

	switch (type) {

	case PM_QOS_VVPU_OPP:
		mutex_lock(&vpu_opp_lock);
		if (get_vvpu_DVFS_is_paused_by_ptpod())
			LOG_INF("PM_QOS_VVPU_OPP paused by ptpod %d\n", data);
		else {
		    LOG_DBG("%s PM_QOS_VVPU_OPP %d\n", __func__, data);
		/*--Set voltage--*/
		ret = regulator_set_voltage(vvpu_reg_id,
			10*(vpu_opp_table[data].vpufreq_volt),
			10*(vpu_opp_table[0].vpufreq_volt));
		if (ret)
		LOG_ERR("regulator_set_voltage  vvpu_reg_id  failed\n");
		}
		mutex_unlock(&vpu_opp_lock);
		break;
	case PM_QOS_VMDLA_OPP:
		mutex_lock(&mdla_opp_lock);
		if (get_vmdla_DVFS_is_paused_by_ptpod())
			LOG_INF("PM_QOS_VMDLA_OPP paused by ptpod %d\n", data);
		else {
		LOG_DBG("%s PM_QOS_VMDLA_OPP %d\n", __func__, data);
		/*--Set voltage--*/
		ret = regulator_set_voltage(vmdla_reg_id,
			10*(mdla_opp_table[data].mdlafreq_volt),
			10*(mdla_opp_table[0].mdlafreq_volt));
		if (ret)
		LOG_ERR("regulator_set_voltage  vmdla_reg_id  failed\n");
		}
		mutex_unlock(&mdla_opp_lock);
		break;

	default:
		LOG_DBG("unsupported type of commit data\n");
		break;
	}

	if (ret < 0) {
		pr_info("%s: type: 0x%x, data: 0x%x, opp: %d, level: %d\n",
				__func__, type, data, opp, level);
		apu_dvfs_dump_reg(NULL);
		aee_kernel_warning("dvfs", "%s: failed.", __func__);
	}

	return ret;
}

static void get_pm_qos_info(char *p)
{
	char timestamp[20];

	dvfs_get_timestamp(timestamp);
	p += sprintf(p, "%-24s: 0x%x\n",
			"PM_QOS_VVPU_OPP",
			pm_qos_request(PM_QOS_VVPU_OPP));
	p += sprintf(p, "%-24s: 0x%x\n",
			"PM_QOS_VMDLA_OPP",
			pm_qos_request(PM_QOS_VMDLA_OPP));
	p += sprintf(p, "%-24s: %s\n",
			"Current Timestamp", timestamp);
	p += sprintf(p, "%-24s: %s\n",
			"Force Start Timestamp", dvfs->force_start);
	p += sprintf(p, "%-24s: %s\n",
			"Force End Timestamp", dvfs->force_end);
}

char *apu_dvfs_dump_reg(char *ptr)
{
	char buf[1024];

	get_pm_qos_info(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	return ptr;
}

static struct devfreq_dev_profile apu_devfreq_profile = {
	.polling_ms	= 0,
};
static int pm_qos_vvpu_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	commit_data(PM_QOS_VVPU_OPP, l);

	return NOTIFY_OK;
}
static int pm_qos_vmdla_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	commit_data(PM_QOS_VMDLA_OPP, l);

	return NOTIFY_OK;
}


static void pm_qos_notifier_register(void)
{

	dvfs->pm_qos_vvpu_opp_nb.notifier_call =
		pm_qos_vvpu_opp_notify;
	pm_qos_add_notifier(PM_QOS_VVPU_OPP,
			&dvfs->pm_qos_vvpu_opp_nb);

	dvfs->pm_qos_vmdla_opp_nb.notifier_call =
		pm_qos_vmdla_opp_notify;
	pm_qos_add_notifier(PM_QOS_VMDLA_OPP,
			&dvfs->pm_qos_vmdla_opp_nb);
}

static int apu_dvfs_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	dvfs = devm_kzalloc(&pdev->dev, sizeof(*dvfs), GFP_KERNEL);
	if (!dvfs)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	LOG_DBG("%s\n", __func__);

	dvfs->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dvfs->regs))
		return PTR_ERR(dvfs->regs);
	platform_set_drvdata(pdev, dvfs);

	dvfs->devfreq = devm_devfreq_add_device(&pdev->dev,
						 &apu_devfreq_profile,
						 "apu_dvfs",
						 NULL);
	dvfs->dvfs_node = of_find_compatible_node(
		NULL, NULL, "mediatek,apu_dvfs");

	/*enable Vvpu Vmdla*/
	/*--Get regulator handle--*/
		vvpu_reg_id = regulator_get(&pdev->dev, "vpu");
		if (!vvpu_reg_id)
			LOG_ERR("regulator_get vvpu_reg_id failed\n");
		vmdla_reg_id = regulator_get(&pdev->dev, "mt6360,bulk1");
		if (!vmdla_reg_id)
			LOG_ERR("regulator_get vmdla_reg_id failed\n");

	ret = apu_dvfs_add_interface(&pdev->dev);
	if (ret)
		return ret;

	pm_qos_notifier_register();
	pr_info("%s: init done\n", __func__);

	return 0;
}

static int apu_dvfs_remove(struct platform_device *pdev)
{
	apu_dvfs_remove_interface(&pdev->dev);
	return 0;
}

static const struct of_device_id apu_dvfs_of_match[] = {
	{ .compatible = "mediatek,apu_dvfs" },
	{ },
};

MODULE_DEVICE_TABLE(of, apu_dvfs_of_match);

static __maybe_unused int apu_dvfs_suspend(struct device *dev)
{
	int ret = 0;

	ret = devfreq_suspend_device(dvfs->devfreq);
	if (ret < 0) {
		LOG_DBG("%s failed to suspend the devfreq devices\n", __func__);
		return ret;
	}

	return 0;
}

static __maybe_unused int apu_dvfs_resume(struct device *dev)
{
	int ret = 0;

	ret = devfreq_resume_device(dvfs->devfreq);
	if (ret < 0) {
		LOG_DBG("%s failed to resume the devfreq devices\n", __func__);
		return ret;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(apu_dvfs_pm, apu_dvfs_suspend,
			 apu_dvfs_resume);

static struct platform_driver apu_dvfs_driver = {
	.probe	= apu_dvfs_probe,
	.remove	= apu_dvfs_remove,
	.driver = {
		.name = "apu_dvfs",
		.pm	= &apu_dvfs_pm,
		.of_match_table = apu_dvfs_of_match,
	},
};

static int __init apu_dvfs_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&apu_dvfs_driver);
	return ret;
}
late_initcall_sync(apu_dvfs_init)

static void __exit apu_dvfs_exit(void)
{
	//int ret = 0;

	platform_driver_unregister(&apu_dvfs_driver);
}
module_exit(apu_dvfs_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("apu dvfs driver");
