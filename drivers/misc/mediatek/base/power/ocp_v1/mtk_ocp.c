/*
* Copyright (C) 2016 MediaTek Inc.
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

#define __MTK_OCP_C__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mach/mtk_cpufreq_api.h>
#ifdef CONFIG_THERMAL
#include <mach/mtk_thermal.h>
#endif
#include <mt-plat/sync_write.h>	/* mt_reg_sync_writel */
#include <mt-plat/mtk_io.h>	/* reg read, write */
#include <mt-plat/aee.h>	/* ram console */
#include "mtk_ocp_internal.h"


#if OCP_FEATURE_ENABLED
#if OCP_INTERRUPT_TEST
static void ocp_work_big(struct work_struct *work);
static void ocp_work_mp0(struct work_struct *work);
static void ocp_work_mp1(struct work_struct *work);
#endif
static int mt_ocp_pdrv_remove(struct platform_device *pdev);
static int mt_ocp_pdrv_probe(struct platform_device *pdev);

#ifdef CONFIG_OF
static const struct of_device_id ocp_of_ids[] = {
	{ .compatible = "mediatek,ocp_cfg", },
	{}
};
#endif

static struct ocp_data ocp_info = {
	.debug = 0,
	.cl_setting = {
		[OCP_LL] = {
			.is_enabled = false,
			.is_forced_off_by_user = false,
			.lock = __SPIN_LOCK_UNLOCKED(ocp_info.cl_setting[OCP_LL].lock),
#if OCP_INTERRUPT_TEST
			.work = __WORK_INITIALIZER(ocp_info.cl_setting[OCP_LL].work, ocp_work_mp0),
#endif
		},
		[OCP_L] = {
			.is_enabled = false,
			.is_forced_off_by_user = false,
			.lock = __SPIN_LOCK_UNLOCKED(ocp_info.cl_setting[OCP_L].lock),
#if OCP_INTERRUPT_TEST
			.work = __WORK_INITIALIZER(ocp_info.cl_setting[OCP_L].work, ocp_work_mp1),
#endif
		},
		[OCP_B] = {
			.is_enabled = false,
			.is_forced_off_by_user = false,
			.lock = __SPIN_LOCK_UNLOCKED(ocp_info.cl_setting[OCP_B].lock),
#if OCP_INTERRUPT_TEST
			.work = __WORK_INITIALIZER(ocp_info.cl_setting[OCP_B].work, ocp_work_big),
#endif
		},
	},
	.pdrv = {
		.probe = mt_ocp_pdrv_probe,
		.remove = mt_ocp_pdrv_remove,
		.driver = {
			.name = "mt_ocp",
			.pm = &ocp_info.pm_ops,
			.owner = THIS_MODULE,
#ifdef CONFIG_OF
			.of_match_table = ocp_of_ids,
#endif
		},
	},
	.auto_capture_cluster = NR_OCP_CLUSTER,
	.auto_capture_total_cnt = 0,
	.auto_capture_cur_cnt = 0,
};


unsigned int __attribute__((weak)) mt_cpufreq_get_cur_freq(unsigned int id)
{
	return 0;
}
unsigned int __attribute__((weak)) mt_cpufreq_get_cur_volt(unsigned int id)
{
	return 0;
}

#ifdef CONFIG_OCP_AEE_RR_REC
static void ocp_aee_init(void)
{
	int i;

	for_each_ocp_cluster(i)
		aee_rr_rec_ocp_target_limit(i, 0);

	aee_rr_rec_ocp_enable(0);
}
#endif

static unsigned int ocp_get_cluster_nr_online_cpu(enum ocp_cluster cluster)
{
	struct cpumask cluster_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return 0;
	}

	arch_get_cluster_cpus(&cluster_cpumask, (int)cluster);
	cpumask_and(&cpu_online_cpumask, &cluster_cpumask, cpu_online_mask);
	cpus = cpumask_weight(&cpu_online_cpumask);

	return cpus;
}

#if OCP_DVT
static int ocp_set_freq(enum ocp_cluster cluster, unsigned int freq_mhz)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	if (freq_mhz > OCP_FREQ_MAX)
		freq_mhz = OCP_FREQ_MAX;

	ocp_lock(cluster);
	/* status check */
	if (!ocp_is_available(cluster)) {
		ocp_unlock(cluster);
		return 0;
	}

	if (ocp_info.cl_setting[cluster].mode & MHZ_BYPASS)
		/* send to ATF */
		ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPFREQPCT, cluster, freq_mhz, 0);
	else
		ret = 0;

	if (!ret)
		ocp_dbg("%s: Set cluster %d freq=%d\n", __func__, cluster, freq_mhz);
	else
		ocp_err("%s: Set cluster %d freq=%d failed, ret=%d\n", __func__, cluster, freq_mhz, ret);
	ocp_unlock(cluster);

	return ret;
}

static int ocp_set_volt(enum ocp_cluster cluster, unsigned int volt_mv)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	volt_mv = (volt_mv > OCP_VOLTAGE_MAX) ? OCP_VOLTAGE_MAX : volt_mv;

	ocp_lock(cluster);
	/* status check */
	if (!ocp_is_available(cluster)) {
		ocp_unlock(cluster);
		return 0;
	}

	if (ocp_info.cl_setting[cluster].mode & VOLT_BYPASS)
		/* send to ATF */
		ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPVOLTAGE, cluster, volt_mv, 0);
	else
		ret = 0;

	if (!ret)
		ocp_dbg("%s: Set cluster %d volt=%dmV\n", __func__, cluster, volt_mv);
	else
		ocp_err("%s: Set cluster %d volt=%dmV failed, ret=%d\n", __func__, cluster, volt_mv, ret);
	ocp_unlock(cluster);

	return ret;
}

static int ocp_set_irq_holdoff(enum ocp_cluster cluster, enum ocp_int_select select, int window)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	ocp_lock(cluster);
	if (!ocp_is_available(cluster)) {
		ocp_err("%s: Cluster %d OCP is disabled!\n", __func__, cluster);
		goto end;
	}

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPIRQHOLDOFF, cluster, select, window);
	if (!ret)
		ocp_dbg("%s: Set cluster %d int_select=%d, window=%d\n", __func__, cluster, select, window);
	else
		ocp_err("%s: Set cluster %d int_select=%d window=%d failed, ret=%d\n",
			__func__, cluster, select, window, ret);

end:
	ocp_unlock(cluster);

	return ret;
}

static int ocp_set_config_mode(enum ocp_cluster cluster, enum ocp_mode mode)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	ocp_lock(cluster);
	if (!ocp_is_available(cluster)) {
		ocp_err("%s: Cluster %d OCP is disabled!\n", __func__, cluster);
		goto end;
	}

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPCONFIGMODE, cluster, mode, 0);
	if (!ret) {
		ocp_info.cl_setting[cluster].mode = mode;
		ocp_dbg("%s: Set cluster %d ConfigMode=%d\n", __func__, cluster, mode);
	} else
		ocp_err("%s: Set cluster %d ConfigMode=%d failed, ret=%d\n", __func__, cluster, mode, ret);

end:
	ocp_unlock(cluster);

	return ret;
}

static int ocp_set_leakage(enum ocp_cluster cluster, unsigned int leakage)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	ocp_lock(cluster);
	if (!ocp_is_available(cluster)) {
		ocp_err("%s: Cluster %d OCP is disabled!\n", __func__, cluster);
		goto end;
	}

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPLEAKAGE, cluster, leakage, 0);
	if (!ret)
		ocp_dbg("%s: Set cluster %d leakage=%d\n", __func__, cluster, leakage);
	else
		ocp_err("%s: Set cluster %d leakage=%d failed, ret=%d\n", __func__, cluster, leakage, ret);

end:
	ocp_unlock(cluster);

	return ret;
}
#endif

#if OCP_INTERRUPT_TEST
static int ocp_int_enable_locked(enum ocp_cluster cluster, unsigned int irq2,
				unsigned int irq1, unsigned int irq0)
{
	int ret = -1;
	unsigned long id = (cluster == OCP_B) ? MTK_SIP_KERNEL_OCPBIGINTENDIS
					: ((cluster == OCP_LL) ? MTK_SIP_KERNEL_OCPMP0INTENDIS
					: MTK_SIP_KERNEL_OCPMP1INTENDIS);

	ret = mt_secure_call_ocp(id, (irq2 & 0x7), (irq1 & 0x7), (irq0 & 0x7));

	if (!ret)
		ocp_dbg("%s: Set cluster %d IntEnDis for IRQ2/1/0=%d/%d/%d\n", __func__, cluster,
			irq2, irq1, irq0);
	else
		ocp_err("%s: Set cluster %d IntEnDis for IRQ2/1/0=%d/%d/%d failed, ret=%d\n",
			__func__, cluster, irq2, irq1, irq0, ret);

	return (ret) ? -1 : 0;
}

static int ocp_int_enable(enum ocp_cluster cluster, unsigned int irq2, unsigned int irq1,
			unsigned int irq0)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	ocp_lock(cluster);
	if (!ocp_is_available(cluster)) {
		ocp_err("%s: Cluster %d OCP is disabled!\n", __func__, cluster);
		goto end;
	}

	ret = ocp_int_enable_locked(cluster, irq2, irq1, irq0);

end:
	ocp_unlock(cluster);

	return ret;
}

static int ocp_int_limit(enum ocp_cluster cluster, enum ocp_int_select select, unsigned int limit)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}
	switch (select) {
	case IRQ_CLK_PCT_MIN:
		if (limit > OCP_CLK_PCT_MAX)
			limit = OCP_CLK_PCT_MAX;
		else if (limit < OCP_CLK_PCT_MIN)
			limit = OCP_CLK_PCT_MIN;
		break;
	case IRQ_WA_MAX:
	case IRQ_WA_MIN:
		limit = (limit > OCP_TARGET_MAX) ? OCP_TARGET_MAX : limit;
		break;
	default:
		ocp_err("%s: Invalid select type: %d\n", __func__, select);
		return -1;
	}

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPINTLIMIT, cluster, select, limit);

	if (!ret)
		ocp_dbg("%s: Set cluster %d IntLimit=%d, select=%d\n", __func__, cluster, limit, select);
	else
		ocp_err("%s: Set cluster %d IntLimit=%d, select=%d failed, ret=%d\n", __func__, cluster,
			limit, select, ret);

	return (ret) ? -1 : 0;
}

static int ocp_int_clear(enum ocp_cluster cluster, unsigned int irq2, unsigned int irq1,
			unsigned int irq0)
{
	int ret = -1;
	unsigned long id;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	id = (cluster == OCP_B) ? MTK_SIP_KERNEL_OCPBIGINTCLR
				: ((cluster == OCP_LL) ? MTK_SIP_KERNEL_OCPMP0INTCLR
				: MTK_SIP_KERNEL_OCPMP1INTCLR);


	ret = mt_secure_call_ocp(id, (irq2 & 0x7), (irq1 & 0x7), (irq0 & 0x7));

	if (!ret)
		ocp_dbg("%s: Set cluster %d IntClr for IRQ2/1/0=%d/%d/%d\n", __func__, cluster,
			irq2, irq1, irq0);
	else
		ocp_err("%s: Set cluster %d IntClr=%d/%d/%d failed, ret=%d\n", __func__, cluster,
			irq2, irq1, irq0, ret);

	return (ret) ? -1 : 0;
}

static void ocp_int_status(enum ocp_cluster cluster, unsigned int *irq2, unsigned int *irq1,
			unsigned int *irq0)
{
	unsigned int status = 0;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return;
	}

	switch (cluster) {
	case OCP_LL:
		status = ocp_sec_read(MP0_OCPAPB01);
		break;
	case OCP_L:
		status = ocp_sec_read(MP1_OCPAPB01);
		break;
	case OCP_B:
		status = ocp_sec_read(MP2_OCPAPB01);
		break;
	default:
		ocp_err("%s: Invalid OCP cluster value: %d\n", __func__, cluster);
		return;
	}

	*irq2 = GET_BITS_VAL_OCP(10:8, status);
	*irq1 = GET_BITS_VAL_OCP(6:4, status);
	*irq0 = GET_BITS_VAL_OCP(2:0, status);

	ocp_dbg("%s: cluster %d status for IRQ2/1/0 = 0x%x/0x%x/0x%x, status = 0x%x\n",
			__func__, cluster, *irq2, *irq1, *irq0, status);
}
#endif

static int ocp_val_status(enum ocp_cluster cluster, enum ocp_value_select select)
{
	int value = 0;

	ocp_lock(cluster);
	/* status check */
	if (!ocp_is_available(cluster)) {
		ocp_unlock(cluster);
		ocp_dbg("%s: Cluster %d OCP is not available! enable=%d, is_force_off=%d\n",
			__func__, cluster, ocp_is_enable(cluster), ocp_is_force_off(cluster));
		return -1;
	}

	switch (select) {
	case CLK_AVG:
	case WA_AVG:
	case TOP_RAW_LKG:
	case CPU0_RAW_LKG:
	case CPU1_RAW_LKG:
	case CPU2_RAW_LKG:
	case CPU3_RAW_LKG:
		value = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPVALUESTATUS, cluster, select, 0);
		break;
	default:
		ocp_err("%s: Invalid OCP select value: %d\n", __func__, select);
		goto end;
	}

end:
	ocp_unlock(cluster);

	ocp_dbg("Cluster %d select = %d, value = %d\n", cluster, select, value);

	return value;
}

static int ocp_enable_locked(enum ocp_cluster cluster, bool enable, enum ocp_mode mode)
{
	int ret = -1;

	/* status check */
	if (enable) {
		if (ocp_is_enable(cluster)) {
			ocp_dbg("%s: OCP for cluster %d is already enabled!\n", __func__, cluster);
			return 0;
		} else if (ocp_is_force_off(cluster)) {
			ocp_dbg("%s: OCP for cluster %d is forced off by user!\n", __func__, cluster);
			return 0;
		}
	} else {
		if (!ocp_is_enable(cluster)) {
			ocp_dbg("%s: OCP for cluster %d is already disabled!\n", __func__, cluster);
			return 0;
		}
	}

	ocp_info("%s: %s cluster %d OCP (mode = 0x%x)\n",
		__func__, (enable) ? "Enable" : "Disable", cluster, mode);

	/* send to ATF */
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPENDIS, cluster, enable, mode);
	if (!ret) {
		ocp_info.cl_setting[cluster].is_enabled = enable;
		ocp_info.cl_setting[cluster].mode = mode;
		if (enable) {
			ocp_info.cl_setting[cluster].target = OCP_TARGET_MAX;

#ifdef CONFIG_OCP_AEE_RR_REC
			aee_rr_rec_ocp_enable((aee_rr_curr_ocp_enable() & ~(1 << cluster)) | (1 << cluster));
			aee_rr_rec_ocp_target_limit(cluster, ocp_info.cl_setting[cluster].target);
#endif

			/* delay 2 window */
			udelay(OCP_ENABLE_DELAY_US);
		} else {
#ifdef CONFIG_OCP_AEE_RR_REC
			aee_rr_rec_ocp_enable(aee_rr_curr_ocp_enable() & ~(1 << cluster));
			aee_rr_rec_ocp_target_limit(cluster, 0);
#endif
		}
	} else {
		ocp_err("%s: %s cluster %d OCP failed, ret = %d\n",
			__func__, (enable) ? "Enable" : "Disable", cluster, ret);
	}

	return ret;
}

static int ocp_enable(enum ocp_cluster cluster, bool enable, enum ocp_mode mode)
{
	int ret = -1;
#ifdef OCP_SSPM_SUPPORT
	/* data[7:0] = cluster */
	/* data[8] = on/off */
	int data, ack_data = 0;
#endif

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	if (mode >= NR_OCP_MODE) {
		ocp_err("%s: Invalid OCP mode: %d\n", __func__, mode);
		return -1;
	}

	ocp_lock(cluster);
	ret = ocp_enable_locked(cluster, enable, mode);
	ocp_unlock(cluster);

#ifdef OCP_SSPM_SUPPORT
	/* enable/disable failed */
	if (ret)
		goto end;

	/* no need to update status to SSPM for E2 or later chip version */
	/* E2 all clusters use OCPv3 so no need to set freq/volt when it changes */
	if (ocp_info.hw_chip_version > 1)
		goto end;

	/* notify SSPM */
	data = ((int)enable << 8) | cluster;
	ret = sspm_ipi_send_sync(IPI_ID_OCP, IPI_OPT_POLLING, &data, 1, &ack_data, 1);
	if (ret != 0)
		ocp_err("@%s: set IPI failed, ret=%d\n", __func__, ret);
	else if (ack_data < 0) {
		ret = ack_data;
		ocp_err("@%s return fail = %d\n", __func__, ret);
	}

end:
#endif
	return ret;
}

int mt_ocp_set_target(enum ocp_cluster cluster, unsigned int target)
{
	int ret = -1;

	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	target = (target > OCP_TARGET_MAX) ? OCP_TARGET_MAX : target;

	ocp_lock(cluster);
	/* status check */
	if (!ocp_is_available(cluster)) {
		ocp_unlock(cluster);
		ocp_warn("%s: Cluster %d OCP is not available! enable=%d, is_force_off=%d\n",
			__func__, cluster, ocp_is_enable(cluster), ocp_is_force_off(cluster));
		return -1;
	}

	/* send to ATF */
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_OCPTARGET, cluster, target, 0);
	if (!ret) {
		ocp_info.cl_setting[cluster].target = target;
		ocp_dbg("%s: Set cluster %d target=%dmW\n", __func__, cluster, target);
#ifdef CONFIG_OCP_AEE_RR_REC
		aee_rr_rec_ocp_target_limit(cluster, target);
#endif
	} else
		ocp_err("%s: Set cluster %d target=%dmW failed, ret=%d\n", __func__, cluster, target, ret);
	ocp_unlock(cluster);

	return ret;
}

unsigned int mt_ocp_get_avgpwr(enum ocp_cluster cluster)
{
	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return -1;
	}

	return ocp_val_status(cluster, WA_AVG);
}

bool mt_ocp_get_status(enum ocp_cluster cluster)
{
	/* input check */
	if (cluster >= NR_OCP_CLUSTER) {
		ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
		return false;
	}

	return ocp_is_available(cluster);
}

unsigned int mt_ocp_get_mcusys_pwr(void)
{
	unsigned int volt = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_CCI) / 100;  /* mV */
	unsigned int freq = mt_cpufreq_get_cur_freq(MT_CPU_DVFS_CCI) / 1000; /* MHz */
	unsigned long power;

	power = ((33 * volt * volt) / (1000 * 1000)) * freq / 1000;

	ocp_dbg("%s: cci freq/volt/power = %d/%d/%ld\n", __func__, freq, volt, power);

	return (unsigned int)power;
}

/* capture function for HQA */
static void ocp_record_data(enum ocp_cluster cluster, unsigned int cnt)
{
	unsigned int data;
	unsigned long addr = (cluster == OCP_B) ? MP2_OCPAPB00
			: ((cluster == OCP_LL) ? MP0_OCPAPB00
			: MP1_OCPAPB00);

	data = ocp_sec_read(addr);
	ocp_info.cl_setting[cluster].status[cnt].clk_avg =
		(GET_BITS_VAL_OCP(23:23, data) == 0) ? GET_BITS_VAL_OCP(22:16, data) : 0;
	ocp_info.cl_setting[cluster].status[cnt].wa_avg = GET_BITS_VAL_OCP(15:0, data);
	ocp_info.cl_setting[cluster].status[cnt].top_raw_lkg = GET_BITS_VAL_OCP(31:24, data);

	data = ocp_sec_read(addr+8);
	ocp_info.cl_setting[cluster].status[cnt].cpu0_raw_lkg = GET_BITS_VAL_OCP(7:0, data);
	ocp_info.cl_setting[cluster].status[cnt].cpu1_raw_lkg = GET_BITS_VAL_OCP(15:8, data);
	ocp_info.cl_setting[cluster].status[cnt].cpu2_raw_lkg =
		(cluster == OCP_B) ? 0 : GET_BITS_VAL_OCP(23:16, data);
	ocp_info.cl_setting[cluster].status[cnt].cpu3_raw_lkg =
		(cluster == OCP_B) ? 0 : GET_BITS_VAL_OCP(31:24, data);

#ifdef CONFIG_THERMAL
	ocp_info.cl_setting[cluster].status[cnt].temp =
			(cluster == OCP_B) ? get_immediate_cpuB_wrap()
			: ((cluster == OCP_LL) ? get_immediate_cpuLL_wrap()
			: get_immediate_cpuL_wrap());
#endif
}

static enum hrtimer_restart ocp_hrtimer_cb(struct hrtimer *timer)
{
	unsigned int i;

	unsigned int cnt = ocp_info.auto_capture_cur_cnt;

	/* add print to show which CPU run hr_timer. */
	ocp_info(".\n");

	/* we assume no cluster on/off when hrtimer has been started. */
	/* so no need to acquire lock before access ocp_info data */
	if (ocp_info.auto_capture_cluster ==  NR_OCP_CLUSTER) {
		for_each_ocp_cluster(i)
			ocp_record_data((enum ocp_cluster)i, cnt);
	} else
		ocp_record_data(ocp_info.auto_capture_cluster, cnt);

	ocp_info.auto_capture_cur_cnt++;
	if (ocp_info.auto_capture_cur_cnt == ocp_info.auto_capture_total_cnt)
		return HRTIMER_NORESTART;
	else
		return HRTIMER_RESTART;
}

#if OCP_INTERRUPT_TEST
/* OCP work queue handle function */
static void ocp_work_big(struct work_struct *work)
{
	unsigned int i, irq0, irq1, irq2;

	ocp_lock(OCP_B);
	/* 1. get int status */
	ocp_int_status(OCP_B, &irq2, &irq1, &irq0);

	ocp_unlock(OCP_B);

	/* 2. notify PPM */

	ocp_lock(OCP_B);
	/* 3. clear int */
	ocp_int_clear(OCP_B, 7, 7, 7);

	/* 4. clear int limit */
	ocp_int_limit(OCP_B, IRQ_CLK_PCT_MIN, 0);
	ocp_int_limit(OCP_B, IRQ_WA_MAX, OCP_TARGET_MAX_V3);
	ocp_int_limit(OCP_B, IRQ_WA_MIN, 0);

	/* 5. re-enable int */
	for_each_ocp_isr(i)
		enable_irq(ocp_info.cl_setting[OCP_B].irq_num[i]);

	ocp_unlock(OCP_B);
}

static void ocp_work_mp0(struct work_struct *work)
{
	unsigned int i, irq0, irq1, irq2;

	ocp_lock(OCP_LL);
	/* 1. get int status */
	ocp_int_status(OCP_LL, &irq2, &irq1, &irq0);

	ocp_unlock(OCP_LL);

	/* 2. notify PPM */

	ocp_lock(OCP_LL);
	/* 3. clear int */
	ocp_int_clear(OCP_LL, 7, 7, 7);

	/* 4. clear int limit */
	ocp_int_limit(OCP_LL, IRQ_CLK_PCT_MIN, 0);
	ocp_int_limit(OCP_LL, IRQ_WA_MAX, OCP_TARGET_MAX);
	ocp_int_limit(OCP_LL, IRQ_WA_MIN, 0);

	/* 5. re-enable int */
	for_each_ocp_isr(i)
		enable_irq(ocp_info.cl_setting[OCP_LL].irq_num[i]);

	ocp_unlock(OCP_LL);
}

static void ocp_work_mp1(struct work_struct *work)
{
	unsigned int i, irq0, irq1, irq2;

	ocp_lock(OCP_L);
	/* 1. get int status */
	ocp_int_status(OCP_L, &irq2, &irq1, &irq0);

	ocp_unlock(OCP_L);

	/* 2. notify PPM */

	ocp_lock(OCP_L);
	/* 3. clear int */
	ocp_int_clear(OCP_L, 7, 7, 7);

	/* 4. clear int limit */
	ocp_int_limit(OCP_L, IRQ_CLK_PCT_MIN, 0);
	ocp_int_limit(OCP_L, IRQ_WA_MAX, OCP_TARGET_MAX);
	ocp_int_limit(OCP_L, IRQ_WA_MIN, 0);

	/* 5. re-enable int */
	for_each_ocp_isr(i)
		enable_irq(ocp_info.cl_setting[OCP_L].irq_num[i]);

	ocp_unlock(OCP_L);
}

/* OCP ISR Handler */
static irqreturn_t ocp_isr_big(int irq, void *dev_id)
{
	int i;

	/* mask all big's interrupt */
	for_each_ocp_isr(i)
		disable_irq_nosync(ocp_info.cl_setting[OCP_B].irq_num[i]);

	schedule_work(&ocp_info.cl_setting[OCP_B].work);

	return IRQ_HANDLED;
}

static irqreturn_t ocp_isr_mp0(int irq, void *dev_id)
{
	int i;

	/* mask all big's interrupt */
	for_each_ocp_isr(i)
		disable_irq_nosync(ocp_info.cl_setting[OCP_LL].irq_num[i]);

	schedule_work(&ocp_info.cl_setting[OCP_LL].work);

	return IRQ_HANDLED;
}

static irqreturn_t ocp_isr_mp1(int irq, void *dev_id)
{
	int i;

	/* mask all big's interrupt */
	for_each_ocp_isr(i)
		disable_irq_nosync(ocp_info.cl_setting[OCP_L].irq_num[i]);

	schedule_work(&ocp_info.cl_setting[OCP_L].work);

	return IRQ_HANDLED;
}
#endif

static int mt_ocp_pdrv_remove(struct platform_device *pdev)
{
	int i, j;

	for_each_ocp_cluster(i) {
		for_each_ocp_isr(j) {
			disable_irq(ocp_info.cl_setting[i].irq_num[j]);
			free_irq(ocp_info.cl_setting[i].irq_num[j], NULL);
		}
	}

	return 0;
}

static int mt_ocp_pdrv_probe(struct platform_device *pdev)
{
#if OCP_INTERRUPT_TEST
#define BUF_SIZE	16

	int err = 0, i, j;
	char str[BUF_SIZE];
#ifdef CONFIG_OF
	struct device_node *node = NULL;

	node = pdev->dev.of_node;
	if (!node) {
		ocp_err("error: cannot find ocp node in device tree!\n");
		WARN_ON(1);
	}

#if 0
	/* 0x10200000 0x4000, OCP */
	/* TODO: remove this? */
	ocp_info.ocp_base = of_iomap(node, 0);
	if (!ocp_info.ocp_base) {
		ocp_err("OCP get some base NULL.\n");
		WARN_ON(1);
	}
#endif

	/*get ocp irq num*/
	ocp_info.cl_setting[OCP_B].irq_num[0] = irq_of_parse_and_map(node, 0);
	ocp_info.cl_setting[OCP_B].irq_num[1] = irq_of_parse_and_map(node, 1);
	ocp_info.cl_setting[OCP_B].irq_num[2] = irq_of_parse_and_map(node, 2);
	ocp_info.cl_setting[OCP_LL].irq_num[0] = irq_of_parse_and_map(node, 3);
	ocp_info.cl_setting[OCP_LL].irq_num[1] = irq_of_parse_and_map(node, 4);
	ocp_info.cl_setting[OCP_LL].irq_num[2] = irq_of_parse_and_map(node, 5);
	ocp_info.cl_setting[OCP_L].irq_num[0] = irq_of_parse_and_map(node, 6);
	ocp_info.cl_setting[OCP_L].irq_num[1] = irq_of_parse_and_map(node, 7);
	ocp_info.cl_setting[OCP_L].irq_num[2] = irq_of_parse_and_map(node, 8);
#else
#define OCP_INT_NUM_BASE	429

	ocp_info.cl_setting[OCP_B].irq_num[0] = OCP_INT_NUM_BASE;
	ocp_info.cl_setting[OCP_B].irq_num[1] = OCP_INT_NUM_BASE + 1;
	ocp_info.cl_setting[OCP_B].irq_num[2] = OCP_INT_NUM_BASE + 2;
	ocp_info.cl_setting[OCP_LL].irq_num[0] = OCP_INT_NUM_BASE + 3;
	ocp_info.cl_setting[OCP_LL].irq_num[1] = OCP_INT_NUM_BASE + 4;
	ocp_info.cl_setting[OCP_LL].irq_num[2] = OCP_INT_NUM_BASE + 5;
	ocp_info.cl_setting[OCP_L].irq_num[0] = OCP_INT_NUM_BASE + 6;
	ocp_info.cl_setting[OCP_L].irq_num[1] = OCP_INT_NUM_BASE + 7;
	ocp_info.cl_setting[OCP_L].irq_num[2] = OCP_INT_NUM_BASE + 8;
#endif

	/* set OCP IRQ */
	for_each_ocp_cluster(i) {
		irq_handler_t handler = (i == OCP_LL) ? ocp_isr_mp0
				: ((i == OCP_L) ? ocp_isr_mp1 : ocp_isr_big);

		snprintf(str, BUF_SIZE, "ocp_cluster%d_isr", i);
		for_each_ocp_isr(j) {
			err = request_irq(ocp_info.cl_setting[i].irq_num[j],
					handler, IRQF_TRIGGER_HIGH, str, NULL);
			if (err) {
				ocp_err("OCP IRQ register failed: ocp%d_irq%d (ret=%d)\n", i, j, err);
				WARN_ON(1);
			} else
				ocp_info("register ocp cluster %d isr%d(%d) done\n",
					i, j, ocp_info.cl_setting[i].irq_num[j]);
		}
	}
#endif

#ifdef CONFIG_OCP_AEE_RR_REC
	ocp_aee_init();
#endif

	ocp_info("OCP Init done\n");

	return 0;
}


#ifdef CONFIG_PROC_FS
static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';
	return buf;

out:
	free_page((unsigned long)buf);
	return NULL;
}

static int ocp_status_dump_proc_show(struct seq_file *m, void *v)
{
	unsigned int i;

	for_each_ocp_cluster(i) {
		seq_puts(m, "-------------------------------------\n");
		ocp_lock(i);
		seq_printf(m, "Cluster %d is_enabled = %d\n", i, ocp_info.cl_setting[i].is_enabled);
		seq_printf(m, "Cluster %d is_forced_off_by_user = %d\n",
				i, ocp_info.cl_setting[i].is_forced_off_by_user);
		seq_printf(m, "Cluster %d mode = %d\n", i, ocp_info.cl_setting[i].mode);
		seq_printf(m, "Cluster %d freq = %d\n", i, mt_cpufreq_get_cur_freq(i));
		seq_printf(m, "Cluster %d volt = %dmV\n", i, mt_cpufreq_get_cur_volt(i));
		seq_printf(m, "Cluster %d target = %dmW\n", i, ocp_info.cl_setting[i].target);
		ocp_unlock(i);
		seq_printf(m, "Cluster %d clk_avg = %d\n", i, ocp_val_status(i, CLK_AVG));
		seq_printf(m, "Cluster %d wa_avg = %dmW\n", i, ocp_val_status(i, WA_AVG));
		seq_printf(m, "Cluster %d top_raw_lkg = %d\n", i, ocp_val_status(i, TOP_RAW_LKG));
		seq_printf(m, "Cluster %d cpu0_raw_lkg = %d\n", i, ocp_val_status(i, CPU0_RAW_LKG));
		seq_printf(m, "Cluster %d cpu1_raw_lkg = %d\n", i, ocp_val_status(i, CPU1_RAW_LKG));
		seq_printf(m, "Cluster %d cpu2_raw_lkg = %d\n", i, ocp_val_status(i, CPU2_RAW_LKG));
		seq_printf(m, "Cluster %d cpu3_raw_lkg = %d\n", i, ocp_val_status(i, CPU3_RAW_LKG));
		seq_puts(m, "-------------------------------------\n");
	}
	seq_printf(m, "MCUSYS power = %dmW\n", mt_ocp_get_mcusys_pwr());

	return 0;
}

static int ocp_auto_capture_proc_show(struct seq_file *m, void *v)
{
	unsigned int i, j;

	if (ocp_info.auto_capture_total_cnt == 0) {
		seq_puts(m, "OCP auto capture is not started yet!\n");
		goto end;
	}

	if (ocp_info.auto_capture_cur_cnt < ocp_info.auto_capture_total_cnt) {
		seq_puts(m, "OCP auto capture is ongoing, please dump result later!\n");
		seq_printf(m, "cluster = %d, cnt = %d, total = %d\n",
				ocp_info.auto_capture_cluster,
				ocp_info.auto_capture_cur_cnt,
				ocp_info.auto_capture_total_cnt);
		goto end;
	}

	/* dump result */
	for (i = 0; i < ocp_info.auto_capture_total_cnt; i++) {
		for_each_ocp_cluster(j) {
			if (!ocp_info.cl_setting[j].status)
				continue;

			seq_printf(m, "Cluster_%d_Freq        = %d Mhz\n", j, mt_cpufreq_get_cur_freq(j));
			seq_printf(m, "Cluster_%d_Vproc       = %d mV\n", j, mt_cpufreq_get_cur_volt(j));
			seq_printf(m, "Cluster_%d_Thermal     = %d mC\n",
					j, ocp_info.cl_setting[j].status[i].temp);
			seq_printf(m, "Cluster_%d_ClkAvg      = %d %%\n",
					j, ocp_info.cl_setting[j].status[i].clk_avg);
			seq_printf(m, "Cluster_%d_WAAvg       = %d mW\n",
					j, ocp_info.cl_setting[j].status[i].wa_avg);
			seq_printf(m, "Cluster_%d_TopRawLkg   = %d\n",
					j, ocp_info.cl_setting[j].status[i].top_raw_lkg);
			seq_printf(m, "Cluster_%d_CPU0RawLkg  = %d\n",
					j, ocp_info.cl_setting[j].status[i].cpu0_raw_lkg);
			seq_printf(m, "Cluster_%d_CPU1RawLkg  = %d\n",
					j, ocp_info.cl_setting[j].status[i].cpu1_raw_lkg);
			if (j != OCP_B) {
				seq_printf(m, "Cluster_%d_CPU2RawLkg  = %d\n",
						j, ocp_info.cl_setting[j].status[i].cpu2_raw_lkg);
				seq_printf(m, "Cluster_%d_CPU3RawLkg  = %d\n",
						j, ocp_info.cl_setting[j].status[i].cpu3_raw_lkg);
			}
		}
		seq_printf(m, "MCUSYS power           = %d mW\n", mt_ocp_get_mcusys_pwr());
	}

end:
	return 0;
}

static ssize_t ocp_auto_capture_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	unsigned int cluster, inteval, cnt;
	int i, j;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d", &cluster, &inteval, &cnt) == 3) {
		if (inteval == 0 || cnt == 0) {
			/* stop capture */
			if (ocp_info.auto_capture_total_cnt != 0) {
				hrtimer_cancel(&ocp_info.auto_capture_timer);
				if (ocp_info.auto_capture_cluster == NR_OCP_CLUSTER) {
					for_each_ocp_cluster(i) {
						kfree(ocp_info.cl_setting[i].status);
						ocp_info.cl_setting[i].status = NULL;
					}
				} else {
					kfree(ocp_info.cl_setting[ocp_info.auto_capture_cluster].status);
					ocp_info.cl_setting[ocp_info.auto_capture_cluster].status = NULL;
				}
				ocp_info.auto_capture_cluster = NR_OCP_CLUSTER;
				ocp_info.auto_capture_cur_cnt = 0;
				ocp_info.auto_capture_total_cnt = 0;
			}
		} else {
			if (cnt > MAX_AUTO_CAPTURE_CNT)
				cnt = MAX_AUTO_CAPTURE_CNT;

			/* record all cluster data */
			if (cluster >= NR_OCP_CLUSTER) {
				ocp_info.auto_capture_cluster = NR_OCP_CLUSTER;
				for_each_ocp_cluster(i) {
					ocp_info.cl_setting[i].status =
						kzalloc(sizeof(struct ocp_status) * cnt, GFP_KERNEL);
					if (!ocp_info.cl_setting[i].status)
						break;
				}

				if (i == NR_OCP_CLUSTER) {
					/* allocate mem success */
					ocp_info.auto_capture_cur_cnt = 0;
					ocp_info.auto_capture_total_cnt = cnt;
					hrtimer_start(&ocp_info.auto_capture_timer,
							ns_to_ktime((unsigned long)inteval),
							HRTIMER_MODE_REL);
				} else {
					/* allocate mem fail, free allocated mem */
					for (j = i-1; j >= 0; j--) {
						kfree(ocp_info.cl_setting[j].status);
						ocp_info.cl_setting[j].status = NULL;
					}
					ocp_err("Allocate mem for auto capture failed!\n");
				}
			} else {
				ocp_info.cl_setting[cluster].status =
						kzalloc(sizeof(struct ocp_status) * cnt, GFP_KERNEL);
				if (!ocp_info.cl_setting[cluster].status)
					/* allocate mem fail */
					ocp_err("Allocate mem for auto capture failed!\n");
				else {
					/* allocate mem success */
					ocp_info.auto_capture_cluster = (enum ocp_cluster)cluster;
					ocp_info.auto_capture_cur_cnt = 0;
					ocp_info.auto_capture_total_cnt = cnt;
					hrtimer_start(&ocp_info.auto_capture_timer,
							ns_to_ktime((unsigned long)inteval),
							HRTIMER_MODE_REL);
				}
			}
		}
	} else
		ocp_err("Usage: echo <cluster> <inteval_ns> <count> > /proc/ocp/ocp_auto_capture\n");

	free_page((unsigned long)buf);
	return count;
}

static int ocp_target_proc_show(struct seq_file *m, void *v)
{
	unsigned int i;

	for_each_ocp_cluster(i) {
		ocp_lock(i);
		seq_printf(m, "Cluster %d target = %dmW\n", i, ocp_info.cl_setting[i].target);
		ocp_unlock(i);
	}

	return 0;
}

static ssize_t ocp_target_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	unsigned int cluster, target;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d", &cluster, &target) == 2)
		mt_ocp_set_target(cluster, target);
	else
		ocp_err("Usage: echo <cluster> <budget> > /proc/ocp/ocp_target\n");

	free_page((unsigned long)buf);
	return count;
}

static int ocp_enable_proc_show(struct seq_file *m, void *v)
{
	unsigned int i;

	for_each_ocp_cluster(i) {
		ocp_lock(i);
		seq_printf(m, "Cluster %d is_enabled = %d\n", i, ocp_info.cl_setting[i].is_enabled);
		seq_printf(m, "Cluster %d is_forced_off_by_user = %d\n",
				i, ocp_info.cl_setting[i].is_forced_off_by_user);
		seq_printf(m, "Cluster %d mode = %d\n", i, ocp_info.cl_setting[i].mode);
		ocp_unlock(i);
	}

	return 0;
}

static ssize_t ocp_enable_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	unsigned int cluster, enable, mode, cpus;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d", &cluster, &enable, &mode) == 3) {
		if (cluster >= NR_OCP_CLUSTER) {
			ocp_err("%s: Invalid cluster id: %d\n", __func__, cluster);
			goto end;
		}

		if (mode >= NR_OCP_MODE) {
			ocp_err("%s: Invalid OCP mode: %d\n", __func__, mode);
			goto end;
		}

		ocp_lock(cluster);
		cpus = ocp_get_cluster_nr_online_cpu((enum ocp_cluster)cluster);
		if (enable > 0) {
			ocp_info.cl_setting[cluster].is_forced_off_by_user = false;
			if (cpus > 0 && !ocp_is_available(cluster)) {
				ocp_unlock(cluster);
				ocp_enable(cluster, true, mode);
				ocp_lock(cluster);
			}
		} else {
			/* turn off OCP first if it is enabled */
			if (ocp_is_available(cluster)) {
				ocp_unlock(cluster);
				ocp_enable(cluster, false, mode);
				ocp_lock(cluster);
			}
			ocp_info.cl_setting[cluster].is_forced_off_by_user = true;
		}
		ocp_unlock(cluster);
	} else
		ocp_err("Usage: echo <cluster> <enable> <mode> > /proc/ocp/ocp_enable\n");

end:
	free_page((unsigned long)buf);
	return count;
}

#if OCP_INTERRUPT_TEST
static int ocp_int_enable_proc_show(struct seq_file *m, void *v)
{
	unsigned int i, status = 0;
	unsigned int irq0, irq1, irq2;

	for_each_ocp_cluster(i) {
		ocp_lock(i);
		/* status check */
		if (ocp_is_available(i)) {
			switch (i) {
			case OCP_LL:
				status = ocp_sec_read(MP0_OCPAPB07);
				break;
			case OCP_L:
				status = ocp_sec_read(MP1_OCPAPB07);
				break;
			case OCP_B:
				status = ocp_sec_read(MP2_OCPAPB07);
				break;
			default:
				break;
			}

			irq2 = GET_BITS_VAL_OCP(10:8, status);
			irq1 = GET_BITS_VAL_OCP(6:4, status);
			irq0 = GET_BITS_VAL_OCP(2:0, status);

			seq_printf(m, "Cluster %d IrqEn 0/1/2 = 0x%x/0x%x/0x%x\n", i, irq0, irq1, irq2);
		} else
			seq_printf(m, "Cluster %d is off\n", i);

		ocp_unlock(i);
	}

	return 0;
}

static ssize_t ocp_int_enable_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	unsigned int cluster, irq0, irq1, irq2;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d %d", &cluster, &irq2, &irq1, &irq0) == 4)
		ocp_int_enable(cluster, irq2, irq1, irq0);
	else
		ocp_err("Usage: echo <cluster> <irq2> <irq1> <irq0> > /proc/ocp/ocp_int_enable\n");

	free_page((unsigned long)buf);
	return count;
}
#endif

static int ocp_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "OCP debug = %d\n", ocp_info.debug);

	return 0;
}

static ssize_t ocp_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	int dbg, rc;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &dbg);
	if (rc < 0)
		ocp_err("Usage: echo (0 or 1) > /proc/ocp/ocp_debug\n");
	else
		ocp_info.debug = dbg;

	free_page((unsigned long)buf);
	return count;
}


#if OCP_DVT
/* dvt_test */
#define OCPPROB				(0x102D06E0)

static unsigned int dvt_test_on;
static unsigned int dvt_test_set;
static unsigned int dvt_test_msb;
static unsigned int dvt_test_lsb;

static int dvt_test_proc_show(struct seq_file *m, void *v)
{
	int i;

	if (dvt_test_on == 1)
		seq_printf(m, "reg:0x%x [%d:%d] = %x\n", dvt_test_set, dvt_test_msb, dvt_test_lsb,
				ocp_sec_read_field(dvt_test_set, dvt_test_msb:dvt_test_lsb));

	switch (dvt_test_on) {
	case 80:
		for (i = 0; i < 172; i += 4)
			seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPSTATUS0 + i), ocp_sec_read(MP0_OCPSTATUS0 + i));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCP_GENERAL_CTRL), ocp_sec_read(MP0_OCP_GENERAL_CTRL));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL0), ocp_sec_read(MP0_OCPCPUPOST_CTRL0));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL1), ocp_sec_read(MP0_OCPCPUPOST_CTRL1));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL2), ocp_sec_read(MP0_OCPCPUPOST_CTRL2));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL3), ocp_sec_read(MP0_OCPCPUPOST_CTRL3));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL4), ocp_sec_read(MP0_OCPCPUPOST_CTRL4));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL5), ocp_sec_read(MP0_OCPCPUPOST_CTRL5));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL6), ocp_sec_read(MP0_OCPCPUPOST_CTRL6));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPCPUPOST_CTRL7), ocp_sec_read(MP0_OCPCPUPOST_CTRL7));
		seq_printf(m, "MP0 Addr: 0x%x = %x\n", (MP0_OCPNCPUPOST_CTRL), ocp_sec_read(MP0_OCPNCPUPOST_CTRL));
		break;
	case 81:
		for (i = 0; i < 172; i += 4)
			seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPSTATUS0 + i), ocp_sec_read(MP1_OCPSTATUS0 + i));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCP_GENERAL_CTRL), ocp_sec_read(MP1_OCP_GENERAL_CTRL));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL0), ocp_sec_read(MP1_OCPCPUPOST_CTRL0));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL1), ocp_sec_read(MP1_OCPCPUPOST_CTRL1));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL2), ocp_sec_read(MP1_OCPCPUPOST_CTRL2));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL3), ocp_sec_read(MP1_OCPCPUPOST_CTRL3));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL4), ocp_sec_read(MP1_OCPCPUPOST_CTRL4));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL5), ocp_sec_read(MP1_OCPCPUPOST_CTRL5));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL6), ocp_sec_read(MP1_OCPCPUPOST_CTRL6));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPCPUPOST_CTRL7), ocp_sec_read(MP1_OCPCPUPOST_CTRL7));
		seq_printf(m, "MP1 Addr: 0x%x = %x\n", (MP1_OCPNCPUPOST_CTRL), ocp_sec_read(MP1_OCPNCPUPOST_CTRL));
		break;
	case 82:
		for (i = 0; i < 96; i += 4)
			seq_printf(m, "MP2 Addr: 0x%x = %x\n", (MP2_OCPAPB00 + i), ocp_sec_read(MP2_OCPAPB00 + i));
		break;
	default:
		break;
	}

	return 0;
}


static ssize_t dvt_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	int function_id, val[4];
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) > 0) {
		switch (function_id) {
		case 1:
			/* register dump: LL=80, L=81, Big=82*/
			if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
				dvt_test_on = val[0];
			break;
		case 2:
			if (sscanf(buf, "%d %x %d %d %x", &function_id, &dvt_test_set, &dvt_test_msb,
				&dvt_test_lsb, &val[0]) == 5) {
				ocp_sec_write_field(dvt_test_set, dvt_test_msb:dvt_test_lsb, val[0]);
				dvt_test_on = 1;
			} else if (sscanf(buf, "%d %x %d %d", &function_id, &dvt_test_set,
				&dvt_test_msb, &dvt_test_lsb) == 4)
				dvt_test_on = 1;
			break;
		case 3:
#if OCP_INTERRUPT_TEST
			/* ocp_int_limit */
			/* val[0] = cluster */
			/* val[1] = IRQ_CLK_PCT_MIN(0)/IRQ_WA_MAX(1)/IRQ_WA_MIN(2) */
			/* val[2] = limit */
			if (sscanf(buf, "%d %d %d %d", &function_id, &val[0], &val[1], &val[2]) == 4)
				ocp_int_limit(val[0], val[1], val[2]);
#else
			ocp_err("interrupt test is not enabled!\n");
#endif
			break;
		case 4:
			/* AUXPMUX OUT switch, must turn on UDI debug */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
				switch (val[0]) {
				case 0: /* LL CPU */
					ocp_sec_write_field(OCPPROB, 4:4, ~val[1]);
					break;
				case 1: /* L CPU */
					ocp_sec_write_field(OCPPROB, 3:3, ~val[1]);
					break;
				case 2: /* BIG CPU */
					ocp_sec_write_field(OCPPROB, 5:5, ~val[1]);
					break;
				default:
					break;
				}
			}
			break;
		case 5:
			/* SPARK */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
				switch (val[0]) {
				case 0:
					ocp_sec_write_field(0x10201C04, 0:0, val[1]);
					break;
				case 1:
					ocp_sec_write_field(0x10201C04, 1:1, val[1]);
					break;
				case 2:
					ocp_sec_write_field(0x10201C04, 2:2, val[1]);
					break;
				case 3:
					ocp_sec_write_field(0x10201C04, 3:3, val[1]);
					break;
				case 4:
					ocp_sec_write_field(0x10203C04, 0:0, val[1]);
					break;
				case 5:
					ocp_sec_write_field(0x10203C04, 1:1, val[1]);
					break;
				case 6:
					ocp_sec_write_field(0x10203C04, 2:2, val[1]);
					break;
				case 7:
					ocp_sec_write_field(0x10203C04, 3:3, val[1]);
					break;
				case 8:
					ocp_sec_write_field(0x10202430, 0:0, val[1]);
					break;
				case 9:
					ocp_sec_write_field(0x10202438, 0:0, val[1]);
					break;
				default:
					break;
				}
			}
			break;
		case 6:
			/* ocp_set_irq_holdoff */
			/* val[0] = cluster */
			/* val[1] = IRQ_CLK_PCT_MIN(0)/IRQ_WA_MAX(1)/IRQ_WA_MIN(2) */
			/* val[2] = window 0~15 */
			if (sscanf(buf, "%d %d %d %d", &function_id, &val[0], &val[1], &val[2]) == 4)
				ocp_set_irq_holdoff(val[0], val[1], val[2]);
			break;
		case 7:
			/* ocp_set_config_mode */
			/* val[0] = cluster */
			/* val[1] = config mode 0 ~ 15 */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ocp_set_config_mode(val[0], val[1]);
			break;
		case 8:
			/* ocp_set_leakage */
			/* val[0] = cluster */
			/* val[1] = leakage 0 ~ 65535 */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ocp_set_leakage(val[0], val[1]);
			break;
		case 9:
			/* ocp_set_freq */
			/* val[0] = cluster */
			/* val[1] = freq_mhz 0 ~ 4095 */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ocp_set_freq(val[0], val[1]);
			break;
		case 10:
			/* ocp_set_volt */
			/* val[0] = cluster */
			/* val[1] = voltage 0 ~ 1999 */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ocp_set_volt(val[0], val[1]);
			break;
		case 11:
			/* test avg window */
			/* val[0] = window 0 ~ 15 */
			/* val[1] = loop */
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				while (val[1]) {
					ocp_info("avg window %d, CG = %d\n", val[0], ocp_val_status(OCP_B, CLK_AVG));
					ocp_info("avg window %d, AVG = %d\n", val[0], ocp_val_status(OCP_B, WA_AVG));
					val[1]--;
				}
			break;
		default:
			break;
		}
	}

	free_page((unsigned long)buf);
	return count;
}
#endif


#define PROC_FOPS_RW(name)                          \
static int name ## _proc_open(struct inode *inode, struct file *file)   \
{                                   \
	return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
}                                   \
static const struct file_operations name ## _proc_fops = {      \
	.owner          = THIS_MODULE,                  \
	.open           = name ## _proc_open,               \
	.read           = seq_read,                 \
	.llseek         = seq_lseek,                    \
	.release        = single_release,               \
	.write          = name ## _proc_write,              \
}

#define PROC_FOPS_RO(name)                          \
static int name ## _proc_open(struct inode *inode, struct file *file)   \
{                                   \
	return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
}                                   \
static const struct file_operations name ## _proc_fops = {      \
	.owner          = THIS_MODULE,                  \
	.open           = name ## _proc_open,               \
	.read           = seq_read,                 \
	.llseek         = seq_lseek,                    \
	.release        = single_release,               \
}

#define PROC_ENTRY(name)    {__stringify(name), &name ## _proc_fops}

PROC_FOPS_RO(ocp_status_dump);
PROC_FOPS_RW(ocp_auto_capture);
PROC_FOPS_RW(ocp_target);
PROC_FOPS_RW(ocp_enable);
#if OCP_INTERRUPT_TEST
PROC_FOPS_RW(ocp_int_enable);
#endif
PROC_FOPS_RW(ocp_debug);
#if OCP_DVT
PROC_FOPS_RW(dvt_test);
#endif

static int ocp_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;
	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(ocp_status_dump),
		PROC_ENTRY(ocp_auto_capture),
		PROC_ENTRY(ocp_target),
		PROC_ENTRY(ocp_enable),
#if OCP_INTERRUPT_TEST
		PROC_ENTRY(ocp_int_enable),
#endif
		PROC_ENTRY(ocp_debug),
#if OCP_DVT
		PROC_ENTRY(dvt_test),
#endif
	};

	dir = proc_mkdir("ocp", NULL);
	if (!dir) {
		ocp_err("fail to create /proc/ocp @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			ocp_err("%s(), create /proc/ocp/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}
#endif
/* CONFIG_PROC_FS */

static int ocp_cpu_hotplug_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct device *dev;
	int cluster_id;
	/* CPU mask - Get on-line cpus per-cluster */
	unsigned int cpus;

	cluster_id = arch_get_cluster_id(cpu);
	cpus = ocp_get_cluster_nr_online_cpu((enum ocp_cluster)cluster_id);

	/* ocp_dbg("%s(): cpu=%d, action=%lu, cpus=%d\n", __func__, cpu, action, cpus); */

	dev = get_cpu_device(cpu);
	if (dev) {
		switch (action & ~CPU_TASKS_FROZEN) {
		case CPU_ONLINE:
		case CPU_DOWN_FAILED:
			if (cpus == 1)
				ocp_enable(cluster_id, true, NO_BYPASS);
			break;
		case CPU_DOWN_PREPARE:
			if (cpus == 1)
				ocp_enable(cluster_id, false, NO_BYPASS);
			break;
		default:
			break;
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata ocp_cpu_hotplug_notifier = {
	.notifier_call = ocp_cpu_hotplug_callback,
};

/*
* Module driver
*/
static int __init ocp_init(void)
{
	unsigned int cpus, i;
	int err = 0;

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (ocp_create_procfs()) {
		err = -ENOMEM;
		goto end;
	}
#endif

	err = platform_driver_register(&ocp_info.pdrv);
	if (err) {
		ocp_err("%s(), OCP driver callback register failed..\n", __func__);
		platform_device_unregister(&ocp_info.pdev);
		goto end;
	}

	register_hotcpu_notifier(&ocp_cpu_hotplug_notifier);

	/* init hrtimer for HQA */
	hrtimer_init(&ocp_info.auto_capture_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ocp_info.auto_capture_timer.function = ocp_hrtimer_cb;

	/* check online cluster */
	for_each_ocp_cluster(i) {
		cpus = ocp_get_cluster_nr_online_cpu((enum ocp_cluster)i);
		if (cpus > 0)
			ocp_enable((enum ocp_cluster)i, true, NO_BYPASS);
	}

end:
	return err;
}

static void __exit ocp_exit(void)
{
	ocp_info("OCP de-initialization\n");

	unregister_hotcpu_notifier(&ocp_cpu_hotplug_notifier);
	platform_driver_unregister(&ocp_info.pdrv);
}

#else /* OCP_FEATURE_ENABLED */
int mt_ocp_set_target(enum ocp_cluster cluster, unsigned int target)
{
	return 0;
}

unsigned int mt_ocp_get_avgpwr(enum ocp_cluster cluster)
{
	return 0;
}

bool mt_ocp_get_status(enum ocp_cluster cluster)
{
	return false;
}

unsigned int mt_ocp_get_mcusys_pwr(void)
{
	return 0;
}

static int __init ocp_init(void)
{
	pr_crit("OCP_FEATURE_ENABLED is not defined!\n");
	return 0;
}

static void __exit ocp_exit(void)
{
}
#endif /* OCP_FEATURE_ENABLED */

module_init(ocp_init);
module_exit(ocp_exit);

MODULE_DESCRIPTION("MediaTek OCP Driver v0.2");
MODULE_LICENSE("GPL");


#undef __MT_OCP_C__

