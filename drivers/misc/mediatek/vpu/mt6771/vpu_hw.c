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

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/ctype.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#include <m4u.h>
#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>

#ifndef MTK_VPU_FPGA_PORTING
#include <mmdvfs_mgr.h>
/*#include <mtk_pmic_info.h>*/
#endif

#ifdef MTK_VPU_DVT
#define VPU_TRACE_ENABLED
#endif
/* #define BYPASS_M4U_DBG */

#include "vpu_hw.h"
#include "vpu_reg.h"
#include "vpu_cmn.h"
#include "vpu_algo.h"
#include "vpu_dbg.h"

#define CMD_WAIT_TIME_MS    (3 * 1000)
#define PWR_KEEP_TIME_MS    (500)
#define IOMMU_VA_START      (0x7DA00000)
#define IOMMU_VA_END        (0x82600000)


/* ion & m4u */
static m4u_client_t *m4u_client;
static struct ion_client *ion_client;
static struct vpu_device *vpu_dev;
static wait_queue_head_t cmd_wait;

/* structure */
struct vup_service_info {
	struct task_struct *vpu_service_task;
	struct sg_table sg_reset_vector;
	struct sg_table sg_main_program;
	struct sg_table sg_algo_binary_data;
	struct sg_table sg_iram_data;
	uint64_t vpu_base;
	uint64_t bin_base;
	uint64_t iram_data_mva;
	uint64_t algo_data_mva;
	vpu_id_t current_algo;
	int thread_variable;
	struct mutex cmd_mutex;
	bool is_cmd_done;
	bool is_running;
	struct vpu_shared_memory *work_buf; /* working buffer */
	struct vpu_shared_memory *exec_kernel_lib; /* execution kernel library */
};
static struct vup_service_info vpu_service_cores[MTK_VPU_CORE];
struct vpu_shared_memory *core_shared_data; /* shared data for all cores */


#ifndef MTK_VPU_FPGA_PORTING

/* clock */
static struct clk *clk_top_dsp_sel;
static struct clk *clk_top_dsp1_sel;
static struct clk *clk_top_dsp2_sel;
static struct clk *clk_top_ipu_if_sel;
static struct clk *clk_ipu_core0_jtag_cg;
static struct clk *clk_ipu_core0_axi_m_cg;
static struct clk *clk_ipu_core0_ipu_cg;
static struct clk *clk_ipu_core1_jtag_cg;
static struct clk *clk_ipu_core1_axi_m_cg;
static struct clk *clk_ipu_core1_ipu_cg;
static struct clk *clk_ipu_adl_cabgen;
static struct clk *clk_ipu_conn_dap_rx_cg;
static struct clk *clk_ipu_conn_apb2axi_cg;
static struct clk *clk_ipu_conn_apb2ahb_cg;
static struct clk *clk_ipu_conn_ipu_cab1to2;
static struct clk *clk_ipu_conn_ipu1_cab1to2;
static struct clk *clk_ipu_conn_ipu2_cab1to2;
static struct clk *clk_ipu_conn_cab3to3;
static struct clk *clk_ipu_conn_cab2to1;
static struct clk *clk_ipu_conn_cab3to1_slice;
static struct clk *clk_ipu_conn_ipu_cg;
static struct clk *clk_ipu_conn_ahb_cg;
static struct clk *clk_ipu_conn_axi_cg;
static struct clk *clk_ipu_conn_isp_cg;
static struct clk *clk_ipu_conn_cam_adl_cg;
static struct clk *clk_ipu_conn_img_adl_cg;
static struct clk *clk_top_mmpll_d6;
static struct clk *clk_top_mmpll_d7;
static struct clk *clk_top_univpll_d3;
static struct clk *clk_top_syspll_d3;
static struct clk *clk_top_univpll_d2_d2;
static struct clk *clk_top_syspll_d2_d2;
static struct clk *clk_top_univpll_d3_d2;
static struct clk *clk_top_syspll_d3_d2;

/* mtcmos */
static struct clk *mtcmos_dis;
static struct clk *mtcmos_vpu_top;
/*static struct clk *mtcmos_vpu_core0_dormant;*/
static struct clk *mtcmos_vpu_core0_shutdown;
/*static struct clk *mtcmos_vpu_core1_dormant;*/
static struct clk *mtcmos_vpu_core1_shutdown;

/* smi */
static struct clk *clk_mmsys_gals_ipu2mm;
static struct clk *clk_mmsys_gals_ipu12mm;
static struct clk *clk_mmsys_gals_comm0;
static struct clk *clk_mmsys_gals_comm1;
static struct clk *clk_mmsys_smi_common;
#endif

/* workqueue */
struct my_struct_t {
	int core;
	struct delayed_work my_work;
};
static struct workqueue_struct *wq;
static void vpu_power_counter_routine(struct work_struct *);
static struct my_struct_t power_counter_work[MTK_VPU_CORE];
/*static DECLARE_DELAYED_WORK(power_counter_work, vpu_power_counter_routine);*/

/* power */
static struct mutex power_mutex[MTK_VPU_CORE];
static bool is_power_debug_lock;
static bool is_power_dynamic = true;
static struct mutex power_counter_mutex[MTK_VPU_CORE];
static int power_counter[MTK_VPU_CORE];


/* dvfs */
static struct vpu_dvfs_opps opps;

/* jtag */
static bool is_jtag_enabled;

/* direct link */
static bool is_locked;
static struct mutex lock_mutex;
static wait_queue_head_t lock_wait;

/* isr handler */
static irqreturn_t vpu0_isr_handler(int irq, void *dev_id);
static irqreturn_t vpu1_isr_handler(int irq, void *dev_id);

typedef irqreturn_t (*ISR_CB)(int, void *);
struct ISR_TABLE {
	ISR_CB          isr_fp;
	unsigned int    int_number;
	char            device_name[16];
};
const struct ISR_TABLE VPU_ISR_CB_TBL[MTK_VPU_CORE] = {
	{vpu0_isr_handler,     0,  "ipu1"}, /* Must be the same name with that in device node. */
	{vpu1_isr_handler,     0,  "ipu2"}
};


static inline void lock_command(int core)
{
	mutex_lock(&(vpu_service_cores[core].cmd_mutex));
	vpu_service_cores[core].is_cmd_done = false;
}

static inline int wait_command(int core)
{
	return (wait_event_interruptible_timeout(
				cmd_wait, vpu_service_cores[core].is_cmd_done, msecs_to_jiffies(CMD_WAIT_TIME_MS)) > 0)
			? 0 : -ETIMEDOUT;
}

static inline void unlock_command(int core)
{
	mutex_unlock(&(vpu_service_cores[core].cmd_mutex));
}

#ifdef MTK_VPU_FPGA_PORTING
#define vpu_prepare_regulator_and_clock(...)   0
#define vpu_enable_regulator_and_clock(...)    0
#define vpu_disable_regulator_and_clock(...)   0
#define vpu_unprepare_regulator_and_clock(...)
#else
static int vpu_prepare_regulator_and_clock(struct device *pdev)
{
	int ret = 0;

#define PREPARE_VPU_MTCMOS(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find mtcmos: %s\n", #clk); \
		} \
	}

	PREPARE_VPU_MTCMOS(mtcmos_dis);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_top);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
#undef PREPARE_VPU_MTCMOS

#define PREPARE_VPU_CLK(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find clock: %s\n", #clk); \
		} else if (clk_prepare(clk)) { \
			ret = -EBADE; \
			LOG_ERR("fail to prepare clock: %s\n", #clk); \
		} \
	}

	PREPARE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	PREPARE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	PREPARE_VPU_CLK(clk_mmsys_gals_comm0);
	PREPARE_VPU_CLK(clk_mmsys_gals_comm1);
	PREPARE_VPU_CLK(clk_mmsys_smi_common);
	PREPARE_VPU_CLK(clk_ipu_adl_cabgen);
	PREPARE_VPU_CLK(clk_ipu_conn_dap_rx_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_apb2axi_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);
	PREPARE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);
	PREPARE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);
	PREPARE_VPU_CLK(clk_ipu_conn_cab3to3);
	PREPARE_VPU_CLK(clk_ipu_conn_cab2to1);
	PREPARE_VPU_CLK(clk_ipu_conn_cab3to1_slice);
	PREPARE_VPU_CLK(clk_ipu_conn_ipu_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_ahb_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_axi_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_isp_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_cam_adl_cg);
	PREPARE_VPU_CLK(clk_ipu_conn_img_adl_cg);
	PREPARE_VPU_CLK(clk_ipu_core0_jtag_cg);
	PREPARE_VPU_CLK(clk_ipu_core0_axi_m_cg);
	PREPARE_VPU_CLK(clk_ipu_core0_ipu_cg);
	PREPARE_VPU_CLK(clk_ipu_core1_jtag_cg);
	PREPARE_VPU_CLK(clk_ipu_core1_axi_m_cg);
	PREPARE_VPU_CLK(clk_ipu_core1_ipu_cg);
	PREPARE_VPU_CLK(clk_top_dsp_sel);
	PREPARE_VPU_CLK(clk_top_dsp1_sel);
	PREPARE_VPU_CLK(clk_top_dsp2_sel);
	PREPARE_VPU_CLK(clk_top_ipu_if_sel);
	PREPARE_VPU_CLK(clk_top_mmpll_d6);
	PREPARE_VPU_CLK(clk_top_mmpll_d7);
	PREPARE_VPU_CLK(clk_top_univpll_d3);
	PREPARE_VPU_CLK(clk_top_syspll_d3);
	PREPARE_VPU_CLK(clk_top_univpll_d2_d2);
	PREPARE_VPU_CLK(clk_top_syspll_d2_d2);
	PREPARE_VPU_CLK(clk_top_univpll_d3_d2);
	PREPARE_VPU_CLK(clk_top_syspll_d3_d2);
#undef PREPARE_VPU_CLK

	return ret;
}
#if 0
static int vpu_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	/* set dsp frequency - 0:546MHZ, 1:504HMz, 2:312MHZ, 3:273HMz*/
	switch (step) {
	case 0:
		clk_src = clk_top_syspll_d2;
		break;
	case 1:
		clk_src = clk_top_univpll_d5;
		break;
	case 2:
		clk_src = clk_top_univpll1_d4;
		break;
	case 3:
		clk_src = clk_top_syspll1_d2;
		break;
	default:
		return -EINVAL;
	}

	return clk_set_parent(clk, clk_src);
}
#endif

static int vpu_enable_regulator_and_clock(int core)
{
	int ret = 0;

	LOG_INF("[vpu_%d] en_rc +\n", core);
	vpu_trace_begin("vpu_enable_regulator_and_clock");
#if 0
	vpu_trace_begin("vcore:request");
	ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL, opps.vcore.index);
	vpu_trace_end();
	CHECK_RET("fail to request vcore, step=%d\n", opps.vcore.index);
#endif
#define ENABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_prepare_enable(clk)) \
				LOG_ERR("fail to prepare and enable mtcmos: %s\n", #clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}

#define ENABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_enable(clk)) \
				LOG_ERR("fail to enable clock: %s\n", #clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}

	vpu_trace_begin("clock:enable_source");
	ENABLE_VPU_CLK(clk_top_dsp_sel);
	ENABLE_VPU_CLK(clk_top_ipu_if_sel);
	ENABLE_VPU_CLK(clk_top_dsp1_sel);
	ENABLE_VPU_CLK(clk_top_dsp2_sel);
	vpu_trace_end();
	LOG_INF("[vpu_%d] en_rc 1\n", core);

	vpu_trace_begin("mtcmos:enable");
	ENABLE_VPU_MTCMOS(mtcmos_dis);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_top);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	LOG_INF("[vpu_%d] en_rc 2\n", core);
	vpu_trace_end();

	vpu_trace_begin("clock:enable");
	ENABLE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	ENABLE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	ENABLE_VPU_CLK(clk_mmsys_gals_comm0);
	ENABLE_VPU_CLK(clk_mmsys_gals_comm1);
	ENABLE_VPU_CLK(clk_mmsys_smi_common);
	ENABLE_VPU_CLK(clk_ipu_adl_cabgen);
	ENABLE_VPU_CLK(clk_ipu_conn_dap_rx_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_apb2axi_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);
	ENABLE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);
	ENABLE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);
	ENABLE_VPU_CLK(clk_ipu_conn_cab3to3);
	ENABLE_VPU_CLK(clk_ipu_conn_cab2to1);
	ENABLE_VPU_CLK(clk_ipu_conn_cab3to1_slice);
	/* recommened by wendell, do not control */
	/* it would be set on after enabling mtcmos*/
	ENABLE_VPU_CLK(clk_ipu_conn_ipu_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_ahb_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_axi_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_isp_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_cam_adl_cg);
	ENABLE_VPU_CLK(clk_ipu_conn_img_adl_cg);
	switch (core) {
	case 0:
	default:
		ENABLE_VPU_CLK(clk_ipu_core0_jtag_cg);
		ENABLE_VPU_CLK(clk_ipu_core0_axi_m_cg);
		ENABLE_VPU_CLK(clk_ipu_core0_ipu_cg);
		break;
	case 1:
		ENABLE_VPU_CLK(clk_ipu_core1_jtag_cg);
		ENABLE_VPU_CLK(clk_ipu_core1_axi_m_cg);
		ENABLE_VPU_CLK(clk_ipu_core1_ipu_cg);
		break;
	}
	vpu_trace_end();

#undef ENABLE_VPU_MTCMOS
#undef ENABLE_VPU_CLK
#if 0
	ret = vpu_set_clock_source(clk_top_dsp_sel, opps.dsp.index);
	CHECK_RET("fail to set dsp freq, step=%d, ret=%d\n", opps.dsp.index, ret);

	ret = vpu_set_clock_source(clk_top_ipu_if_sel, opps.ipu_if.index);
	CHECK_RET("fail to set ipu_if freq, step=%d, ret=%d\n", opps.ipu_if.index, ret);

out:
#endif
	vpu_trace_end();
	LOG_INF("[vpu_%d] en_rc -\n", core);
	return ret;
}



static int vpu_disable_regulator_and_clock(int core)
{
	int ret = 0;

	LOG_INF("[vpu_%d] dis_rc +\n", core);
#define DISABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable(clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}

	switch (core) {
	case 0:
	default:
		DISABLE_VPU_CLK(clk_ipu_core0_jtag_cg);
		DISABLE_VPU_CLK(clk_ipu_core0_axi_m_cg);
		DISABLE_VPU_CLK(clk_ipu_core0_ipu_cg);
		break;
	case 1:
		DISABLE_VPU_CLK(clk_ipu_core1_jtag_cg);
		DISABLE_VPU_CLK(clk_ipu_core1_axi_m_cg);
		DISABLE_VPU_CLK(clk_ipu_core1_ipu_cg);
		break;
	}
	DISABLE_VPU_CLK(clk_ipu_adl_cabgen);
	DISABLE_VPU_CLK(clk_ipu_conn_dap_rx_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_apb2axi_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_cab3to3);
	DISABLE_VPU_CLK(clk_ipu_conn_cab2to1);
	DISABLE_VPU_CLK(clk_ipu_conn_cab3to1_slice);
	/*DISABLE_VPU_CLK(clk_ipu_conn_ipu_cg);*/
	/*DISABLE_VPU_CLK(clk_ipu_conn_ahb_cg);*/
	/*DISABLE_VPU_CLK(clk_ipu_conn_axi_cg);*/
	/*DISABLE_VPU_CLK(clk_ipu_conn_isp_cg);*/
	/*DISABLE_VPU_CLK(clk_ipu_conn_cam_adl_cg);*/
	/*DISABLE_VPU_CLK(clk_ipu_conn_img_adl_cg);*/
	DISABLE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	DISABLE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	DISABLE_VPU_CLK(clk_mmsys_gals_comm0);
	DISABLE_VPU_CLK(clk_mmsys_gals_comm1);
	DISABLE_VPU_CLK(clk_mmsys_smi_common);
	LOG_INF("[vpu_%d] dis_rc flag4\n", core);

#define DISABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable_unprepare(clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}

	DISABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	LOG_INF("[vpu_%d] dis_rc flag4.1\n", core);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	LOG_INF("[vpu_%d] dis_rc flag4.2\n", core);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_top);
	LOG_INF("[vpu_%d] dis_rc flag4.3\n", core);
	DISABLE_VPU_MTCMOS(mtcmos_dis);
	LOG_INF("[vpu_%d] dis_rc flag4.4\n", core);

	DISABLE_VPU_CLK(clk_ipu_conn_ipu_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_ahb_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_axi_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_isp_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_cam_adl_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_img_adl_cg);

	DISABLE_VPU_CLK(clk_top_dsp_sel);
	LOG_INF("[vpu_%d] dis_rc test_5\n", core);
	DISABLE_VPU_CLK(clk_top_ipu_if_sel);
	LOG_INF("[vpu_%d] dis_rc test_5.1\n", core);
	DISABLE_VPU_CLK(clk_top_dsp1_sel);
	DISABLE_VPU_CLK(clk_top_dsp2_sel);
	LOG_INF("[vpu_%d] dis_rc test_5.2\n", core);

#undef DISABLE_VPU_MTCMOS
#undef DISABLE_VPU_CLK
#if 0
	ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL, MMDVFS_FINE_STEP_UNREQUEST);
	CHECK_RET("fail to unrequest vcore!\n");
out:
#endif
	LOG_INF("[vpu_%d] dis_rc -\n", core);
	return ret;

}

static void vpu_unprepare_regulator_and_clock(void)
{

#define UNPREPARE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_unprepare(clk); \
			clk = NULL; \
		} \
	}
	UNPREPARE_VPU_CLK(clk_ipu_core0_jtag_cg);
	UNPREPARE_VPU_CLK(clk_ipu_core0_axi_m_cg);
	UNPREPARE_VPU_CLK(clk_ipu_core0_ipu_cg);
	UNPREPARE_VPU_CLK(clk_ipu_core1_jtag_cg);
	UNPREPARE_VPU_CLK(clk_ipu_core1_axi_m_cg);
	UNPREPARE_VPU_CLK(clk_ipu_core1_ipu_cg);
	UNPREPARE_VPU_CLK(clk_ipu_adl_cabgen);
	UNPREPARE_VPU_CLK(clk_ipu_conn_dap_rx_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_apb2axi_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);
	UNPREPARE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);
	UNPREPARE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);
	UNPREPARE_VPU_CLK(clk_ipu_conn_cab3to3);
	UNPREPARE_VPU_CLK(clk_ipu_conn_cab2to1);
	UNPREPARE_VPU_CLK(clk_ipu_conn_cab3to1_slice);
	UNPREPARE_VPU_CLK(clk_ipu_conn_ipu_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_ahb_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_axi_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_isp_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_cam_adl_cg);
	UNPREPARE_VPU_CLK(clk_ipu_conn_img_adl_cg);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_comm0);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_comm1);
	UNPREPARE_VPU_CLK(clk_mmsys_smi_common);
	UNPREPARE_VPU_CLK(clk_top_dsp_sel);
	UNPREPARE_VPU_CLK(clk_top_dsp1_sel);
	UNPREPARE_VPU_CLK(clk_top_dsp2_sel);
	UNPREPARE_VPU_CLK(clk_top_ipu_if_sel);
	UNPREPARE_VPU_CLK(clk_top_mmpll_d6);
	UNPREPARE_VPU_CLK(clk_top_mmpll_d7);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3);
	UNPREPARE_VPU_CLK(clk_top_syspll_d3);
	UNPREPARE_VPU_CLK(clk_top_univpll_d2_d2);
	UNPREPARE_VPU_CLK(clk_top_syspll_d2_d2);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3_d2);
	UNPREPARE_VPU_CLK(clk_top_syspll_d3_d2);
#undef UNPREPARE_VPU_CLK

}
#endif

irqreturn_t vpu0_isr_handler(int irq, void *dev_id)
{
	LOG_DBG("vpu 0 received a interrupt\n");
	vpu_service_cores[0].is_cmd_done = true;
	wake_up_interruptible(&cmd_wait);
	vpu_write_field(0, FLD_APMCU_INT, 1);                   /* clear int */

	return IRQ_HANDLED;
}
irqreturn_t vpu1_isr_handler(int irq, void *dev_id)
{
	LOG_DBG("vpu 1 received a interrupt\n");
	vpu_service_cores[1].is_cmd_done = true;
	wake_up_interruptible(&cmd_wait);
	vpu_write_field(1, FLD_APMCU_INT, 1);                   /* clear int */

	return IRQ_HANDLED;
}

static bool service_pool_is_empty(int core)
{
	bool is_empty = true;

	mutex_lock(&vpu_dev->servicepool_mutex[core]);
	if (!list_empty(&vpu_dev->servicepool_list[core]))
		is_empty = false;

	mutex_unlock(&vpu_dev->servicepool_mutex[core]);

	return is_empty;
}
static bool common_pool_is_empty(void)
{
	bool is_empty = true;

	mutex_lock(&vpu_dev->commonpool_mutex);
	if (!list_empty(&vpu_dev->commonpool_list))
		is_empty = false;

	mutex_unlock(&vpu_dev->commonpool_mutex);

	return is_empty;
}

static int vpu_service_routine(void *arg)
{

	struct vpu_user *user = NULL;
	struct vpu_request *req = NULL;
	struct vpu_algo *algo = NULL;
	int *d = (int *)arg;
	int service_core = (*d);

	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	for (; !kthread_should_stop();) {
		/* wait for requests if there is no one in user's queue */
		add_wait_queue(&vpu_dev->req_wait, &wait);
		while (1) {
			if ((!service_pool_is_empty(service_core)) || (!common_pool_is_empty()))
				break;
			wait_woken(&wait, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
		}
		remove_wait_queue(&vpu_dev->req_wait, &wait);

		/* this thread will be stopped if start direct link */
		wait_event_interruptible(lock_wait, !is_locked);

		/* consume the user's queue */
		req = NULL;

		mutex_lock(&vpu_dev->servicepool_mutex[service_core]);
		if (!(list_empty(&vpu_dev->servicepool_list[service_core]))) {
			req = vlist_node_of(vpu_dev->servicepool_list[service_core].next, struct vpu_request);
			list_del_init(vlist_link(req, struct vpu_request));
			vpu_dev->servicepool_list_size[service_core] -= 1;
			LOG_DBG("[vpu] flag - : selfpool(%d)_size(%d)\n", service_core,
				vpu_dev->servicepool_list_size[service_core]);
			mutex_unlock(&vpu_dev->servicepool_mutex[service_core]);
			LOG_DBG("[vpu] flag - 2: get selfpool\n");
		} else {
			mutex_unlock(&vpu_dev->servicepool_mutex[service_core]);

			mutex_lock(&vpu_dev->commonpool_mutex);
			if (!(list_empty(&vpu_dev->commonpool_list))) {
				req = vlist_node_of(vpu_dev->commonpool_list.next, struct vpu_request);
				list_del_init(vlist_link(req, struct vpu_request));
				vpu_dev->commonpool_list_size -= 1;
				LOG_DBG("[vpu] flag - : common pool_size(%d)\n", vpu_dev->commonpool_list_size);
				LOG_DBG("[vpu] flag - 3: get common pool\n");
			}
			mutex_unlock(&vpu_dev->commonpool_mutex);
		}
		/* suppose that req is null would not happen */
		/* due to we check service_pool_is_empty and common_pool_is_empty */
		if (req != NULL) {
			LOG_DBG("[vpu] service core index...: %d/%d", service_core, (*d));
			user = (struct vpu_user *)req->user_id;
			LOG_DBG("[vpu] user...0x%lx/0x%lx/0x%lx/0x%lx\n", (unsigned long)user, (unsigned long)&user,
				(unsigned long)req->user_id, (unsigned long)&(req->user_id));
			mutex_lock(&vpu_dev->user_mutex);
			user->running = true; /* for flush request from queue, DL usage */
			LOG_INF("[vpu_%d] run, algo_id(%d/%d)\n", service_core,
				(int)(req->algo_id[service_core]), vpu_service_cores[service_core].current_algo);
			/* unlock for avoiding long time locking */
			mutex_unlock(&vpu_dev->user_mutex);
			if (req->algo_id[service_core] != vpu_service_cores[service_core].current_algo) {
				if (vpu_find_algo_by_id(service_core, req->algo_id[service_core], &algo)) {
					req->status = VPU_REQ_STATUS_INVALID;
						LOG_ERR("can not find the algo, id=%d\n", req->algo_id[service_core]);
						goto out;
				}
				if (vpu_hw_load_algo(service_core, algo)) {
					LOG_ERR("load algo failed, while enque\n");
					req->status = VPU_REQ_STATUS_FAILURE;
					goto out;
				}
			}
			LOG_DBG("[vpu] flag - 4: hw_enque_request\n");
			vpu_hw_enque_request(service_core, req);
			LOG_DBG("[vpu] flag - 5: hw enque_request done\n");
		} else {
			/* consider that only one req in common pool and all services get pass through */
			/* do nothing if the service do not get the request */
			continue;
		}
out:
		/* if req is null, we should not do anything of following codes */
		mutex_lock(&vpu_dev->user_mutex);
		LOG_DBG("[vpu] flag - 5.5 : ....\n");
		mutex_lock(&user->data_mutex);
		LOG_DBG("[vpu] flag - 6: add to deque list\n");
		req->occupied_core = (0x1 << service_core);
		list_add_tail(vlist_link(req, struct vpu_request), &user->deque_list);
		LOG_INF("[vpu_0x%x] add to deque list done\n", req->occupied_core);
		user->running = false;
		mutex_unlock(&user->data_mutex);
		mutex_unlock(&vpu_dev->user_mutex);
		wake_up_interruptible_all(&user->deque_wait);
		/* leave loop of round-robin */
		if (is_locked)
			break;
		/* release cpu for another operations */
		usleep_range(1, 10);
	}
	return 0;
}


#ifndef MTK_VPU_EMULATOR

static int vpu_map_mva_of_bin(int core, uint64_t bin_pa)
{
	int ret = 0;

#ifndef BYPASS_M4U_DBG
	uint32_t mva_reset_vector;
	uint32_t mva_main_program;
	uint32_t mva_algo_binary_data;
	uint32_t mva_iram_data;
	uint64_t binpa_reset_vector;
	uint64_t binpa_main_program;
	uint64_t binpa_iram_data;
	struct sg_table *sg;
	const uint64_t size_algos = VPU_SIZE_ALGO_AREA + VPU_SIZE_ALGO_AREA + VPU_SIZE_ALGO_AREA;

	LOG_DBG("vpu_map_mva_of_bin, bin_pa(0x%lx)\n", (unsigned long)bin_pa);

	switch (core) {
	case 0:
	default:
		mva_reset_vector = VPU_MVA_RESET_VECTOR;
		mva_main_program = VPU_MVA_MAIN_PROGRAM;
		binpa_reset_vector = bin_pa;
		binpa_main_program = bin_pa + VPU_OFFSET_MAIN_PROGRAM;
		binpa_iram_data = bin_pa + VPU_OFFSET_MAIN_PROGRAM_IMEM;
		break;
	case 1:
		mva_reset_vector = VPU2_MVA_RESET_VECTOR;
		mva_main_program = VPU2_MVA_MAIN_PROGRAM;
		binpa_reset_vector = bin_pa + VPU_DDR_SHIFT_RESET_VECTOR;
		binpa_main_program = binpa_reset_vector + VPU_OFFSET_MAIN_PROGRAM;
		binpa_iram_data = bin_pa + VPU_OFFSET_MAIN_PROGRAM_IMEM + VPU_DDR_SHIFT_IRAM_DATA;
		break;
	case 2:
		mva_reset_vector = VPU3_MVA_RESET_VECTOR;
		mva_main_program = VPU3_MVA_MAIN_PROGRAM;
		binpa_reset_vector = bin_pa + (VPU_DDR_SHIFT_RESET_VECTOR << 2);
		binpa_main_program = binpa_reset_vector + VPU_OFFSET_MAIN_PROGRAM;
		binpa_iram_data = bin_pa + VPU_OFFSET_MAIN_PROGRAM_IMEM + (VPU_DDR_SHIFT_IRAM_DATA << 2);
		break;
	}
	LOG_DBG("vpu_map_mva_of_bin(core:%d), pa resvec/mainpro(0x%lx/0x%lx)\n", core,
		(unsigned long)binpa_reset_vector, (unsigned long)binpa_main_program);

	/* 1. map reset vector */
	sg = &(vpu_service_cores[core].sg_reset_vector);
	ret = sg_alloc_table(sg, 1, GFP_KERNEL);
	CHECK_RET("fail to allocate sg table[reset]!\n");
	LOG_DBG("vpu...sg_alloc_table ok\n");

	sg_dma_address(sg->sgl) = binpa_reset_vector;
	LOG_DBG("vpu...sg_dma_address ok, bin_pa(0x%x)\n", (unsigned int)binpa_reset_vector);
	sg_dma_len(sg->sgl) = VPU_SIZE_RESET_VECTOR;
	LOG_DBG("vpu...sg_dma_len ok, VPU_SIZE_RESET_VECTOR(0x%x)\n", VPU_SIZE_RESET_VECTOR);
	ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
			0, sg,
			VPU_SIZE_RESET_VECTOR,
			M4U_PROT_READ | M4U_PROT_WRITE,
			M4U_FLAGS_START_FROM/*M4U_FLAGS_FIX_MVA*/, &mva_reset_vector);
	CHECK_RET("fail to allocate mva of reset vecter!\n");
	LOG_DBG("vpu...m4u_alloc_mva ok\n");

	/* 2. map main program */
	sg = &(vpu_service_cores[core].sg_main_program);
	ret = sg_alloc_table(sg, 1, GFP_KERNEL);
	CHECK_RET("fail to allocate sg table[main]!\n");
	LOG_DBG("vpu...sg_alloc_table main_program ok\n");

	sg_dma_address(sg->sgl) = binpa_main_program;
	sg_dma_len(sg->sgl) = VPU_SIZE_MAIN_PROGRAM;
	ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
			0, sg,
			VPU_SIZE_MAIN_PROGRAM,
			M4U_PROT_READ | M4U_PROT_WRITE,
			M4U_FLAGS_START_FROM/*M4U_FLAGS_FIX_MVA*/, &mva_main_program);
	if (ret) {
		LOG_ERR("fail to allocate mva of main program!\n");
		m4u_dealloc_mva(m4u_client, VPU_PORT_OF_IOMMU, mva_main_program);
		goto out;
	}
	LOG_DBG("vpu...m4u_alloc_mva main_program ok, (0x%x/0x%x)\n",
		(unsigned int)(binpa_main_program), (unsigned int)VPU_SIZE_MAIN_PROGRAM);

	/* 3. map all algo binary data(src addr for dps to copy) */
	/* no need reserved mva, use SG_READY*/
	if (core == 0) {
		sg = &(vpu_service_cores[core].sg_algo_binary_data);
		ret = sg_alloc_table(sg, 1, GFP_KERNEL);
		CHECK_RET("fail to allocate sg table[reset]!\n");
		LOG_DBG("vpu...sg_alloc_table algo_data ok, mva_algo_binary_data = 0x%x\n",
			mva_algo_binary_data);

		sg_dma_address(sg->sgl) = bin_pa + VPU_OFFSET_ALGO_AREA;
		sg_dma_len(sg->sgl) = size_algos;
		ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
				0, sg,
				size_algos,
				M4U_PROT_READ | M4U_PROT_WRITE,
				M4U_FLAGS_SG_READY, &mva_algo_binary_data);
		CHECK_RET("fail to allocate mva of reset vecter!\n");
		vpu_service_cores[core].algo_data_mva = mva_algo_binary_data;
		LOG_DBG("a vpu va_algo_data pa: 0x%x\n", (unsigned int)(bin_pa + VPU_OFFSET_ALGO_AREA));
		LOG_DBG("a vpu va_algo_data: 0x%x/0x%x, size: 0x%x\n", mva_algo_binary_data,
			(unsigned int)(vpu_service_cores[core].algo_data_mva), (unsigned int)size_algos);
	} else {
		vpu_service_cores[core].algo_data_mva = vpu_service_cores[0].algo_data_mva;
	}

	/* 4. map main program iram data */
	/* no need reserved mva, use SG_READY*/
	sg = &(vpu_service_cores[core].sg_iram_data);
	ret = sg_alloc_table(sg, 1, GFP_KERNEL);
	CHECK_RET("fail to allocate sg table[reset]!\n");
	LOG_DBG("vpu...sg_alloc_table iram_data ok, mva_iram_data = 0x%x\n", mva_iram_data);
	LOG_DBG("a vpu iram pa: 0x%lx\n", (unsigned long)(binpa_iram_data));
	sg_dma_address(sg->sgl) = binpa_iram_data;
	sg_dma_len(sg->sgl) = VPU_SIZE_MAIN_PROGRAM_IMEM;
	ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
			0, sg,
			VPU_SIZE_MAIN_PROGRAM_IMEM,
			M4U_PROT_READ | M4U_PROT_WRITE,
			M4U_FLAGS_SG_READY, &mva_iram_data);
	CHECK_RET("fail to allocate mva of iram data!\n");
	vpu_service_cores[core].iram_data_mva = (uint64_t)(mva_iram_data);
	LOG_DBG("a vpu va_iram_data: 0x%x, iram_data_mva: 0x%lx\n",
		mva_iram_data, (unsigned long)(vpu_service_cores[core].iram_data_mva));

out:
#endif
	return ret;
}
#endif

static int vpu_get_power(int core)
{
	int ret = 0;

	LOG_INF("[vpu_%d/%d] gp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	power_counter[core]++;
	ret = vpu_boot_up(core);
	mutex_unlock(&power_counter_mutex[core]);
	LOG_INF("[vpu_%d/%d] gp -\n", core, power_counter[core]);
	return ret;
}

static void vpu_put_power(int core)
{
	LOG_INF("[vpu_%d/%d] pp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	if (--power_counter[core] == 0)
		mod_delayed_work(wq, &(power_counter_work[core].my_work), msecs_to_jiffies(PWR_KEEP_TIME_MS));
	mutex_unlock(&power_counter_mutex[core]);
	LOG_INF("[vpu_%d/%d] pp -\n", core, power_counter[core]);
}

static void vpu_power_counter_routine(struct work_struct *work)
{
	int core = 0;
	struct my_struct_t *my_work_core = container_of(work, struct my_struct_t, my_work.work);

	core = my_work_core->core;
	LOG_INF("vpu_%d counterR +", core);

	mutex_lock(&power_counter_mutex[core]);
	if (power_counter[core] == 0)
		vpu_shut_down(core);
	mutex_unlock(&power_counter_mutex[core]);
	LOG_INF("vpu_%d counterR -", core);
}

int vpu_init_hw(int core, struct vpu_device *device)
{
	int ret, i;
	int param;

	struct vpu_shared_memory_param mem_param;

	LOG_INF("[vpu] core : %d\n", core);

	vpu_service_cores[core].vpu_base = device->vpu_base[core];
	vpu_service_cores[core].bin_base = device->bin_base;

#ifdef MTK_VPU_EMULATOR
	vpu_request_emulator_irq(device->irq_num[core], vpu_isr_handler);
#else
	m4u_client = m4u_create_client();
	ion_client = ion_client_create(g_ion_device, "vpu");

	ret = vpu_map_mva_of_bin(core, device->bin_pa);
	CHECK_RET("fail to map binary data!\n");

	ret = request_irq(device->irq_num[core], (irq_handler_t)VPU_ISR_CB_TBL[core].isr_fp,
		device->irq_trig_level, (const char *)VPU_ISR_CB_TBL[core].device_name, NULL);
	CHECK_RET("fail to request vpu irq!\n");
#endif


	if (core == 0) {
		init_waitqueue_head(&cmd_wait);
		mutex_init(&lock_mutex);
		init_waitqueue_head(&lock_wait);
		vpu_dev = device;

		for (i = 0 ; i < MTK_VPU_CORE ; i++) {
			power_counter[i] = 0;
			INIT_DELAYED_WORK(&(power_counter_work[i].my_work), vpu_power_counter_routine);
			power_counter_work[i].core = i;
			mutex_init(&(power_mutex[i]));
			mutex_init(&(power_counter_mutex[i]));

			vpu_service_cores[i].vpu_service_task =
				(struct task_struct *)kmalloc(sizeof(struct task_struct), GFP_KERNEL);
			if (vpu_service_cores[i].vpu_service_task != NULL) {
				param = i;
				vpu_service_cores[i].thread_variable = i;
				if (i == 0) {
					vpu_service_cores[i].vpu_service_task =
						kthread_create(vpu_service_routine,
						&(vpu_service_cores[i].thread_variable), "vpu0");
				} else if (i == 1) {
					vpu_service_cores[i].vpu_service_task =
						kthread_create(vpu_service_routine,
						&(vpu_service_cores[i].thread_variable), "vpu1");
				} else {
					vpu_service_cores[i].vpu_service_task =
						kthread_create(vpu_service_routine,
						&(vpu_service_cores[i].thread_variable), "vpu2");
				}
				if (IS_ERR(vpu_service_cores[i].vpu_service_task)) {
					ret = PTR_ERR(vpu_service_cores[i].vpu_service_task);
					goto out;
				}
			} else {
				LOG_ERR("allocate enque task(%d) fail", i);
				goto out;
			}
			wake_up_process(vpu_service_cores[i].vpu_service_task);

			mutex_init(&(vpu_service_cores[i].cmd_mutex));
			vpu_service_cores[i].is_cmd_done = false;
			vpu_service_cores[i].is_running = false;

			/* working buffer */
			/* no need in reserved region */
			mem_param.require_pa = true;
			mem_param.require_va = true;
			mem_param.size = VPU_SIZE_WORK_BUF;
			mem_param.fixed_addr = 0;
			ret = vpu_alloc_shared_memory(&(vpu_service_cores[i].work_buf), &mem_param);
			LOG_DBG("core(%d):work_buf va (0x%lx),pa(0x%x)",
				i, (unsigned long)(vpu_service_cores[i].work_buf->va),
				vpu_service_cores[i].work_buf->pa);
			CHECK_RET("fail to allocate working buffer!\n");

			/* execution kernel library */
			/* need in reserved region, set end as the end of reserved address, m4u use start-from */
			mem_param.require_pa = true;
			mem_param.require_va = true;
			mem_param.size = VPU_SIZE_ALGO_KERNEL_LIB;
			switch (i) {
			case 0:
			default:
				mem_param.fixed_addr = VPU_MVA_KERNEL_LIB;
				break;
			case 1:
				mem_param.fixed_addr = VPU2_MVA_KERNEL_LIB;
				break;
			case 2:
				mem_param.fixed_addr = VPU3_MVA_KERNEL_LIB;
				break;
			}

			ret = vpu_alloc_shared_memory(&(vpu_service_cores[i].exec_kernel_lib), &mem_param);
			LOG_DBG("core(%d):kernel_lib va (0x%lx),pa(0x%x)",
				i, (unsigned long)(vpu_service_cores[i].exec_kernel_lib->va),
				vpu_service_cores[i].exec_kernel_lib->pa);
			CHECK_RET("fail to allocate kernel_lib buffer!\n");

		}
		/* multi-core shared  */
		/* need in reserved region, set end as the end of reserved address, m4u use start-from */
		mem_param.require_pa = true;
		mem_param.require_va = true;
		mem_param.size = VPU_SIZE_SHARED_DATA;
		mem_param.fixed_addr = VPU_MVA_SHARED_DATA;
		ret = vpu_alloc_shared_memory(&(core_shared_data), &mem_param);
		LOG_DBG("shared_data va (0x%lx),pa(0x%x)",
			(unsigned long)(core_shared_data->va), core_shared_data->pa);
		CHECK_RET("fail to allocate working buffer!\n");


		wq = create_workqueue("vpu_wq");


		/* define the steps and OPP */
#define DEFINE_VPU_STEP(step, m, v0, v1, v2, v3) \
			{ \
				opps.step.index = m - 1; \
				opps.step.count = m; \
				opps.step.values[0] = v0; \
				opps.step.values[1] = v1; \
				opps.step.values[2] = v2; \
				opps.step.values[3] = v3; \
			}


#define DEFINE_VPU_OPP(i, v0, v1, v2) \
		{ \
			opps.vcore.opp_map[i]  = v0; \
			opps.dsp.opp_map[i]    = v1; \
			opps.ipu_if.opp_map[i] = v2; \
		}


		DEFINE_VPU_STEP(vcore,  2, 800000, 700000, 0, 0);
		DEFINE_VPU_STEP(dsp,    4, 546000, 504000, 312000, 274000);
		DEFINE_VPU_STEP(ipu_if, 4, 546000, 504000, 312000, 274000);

		DEFINE_VPU_OPP(0, 0, 0, 1);
		DEFINE_VPU_OPP(1, 1, 2, 3);

		opps.count = 2;
		opps.index = 1;

#undef DEFINE_VPU_OPP
#undef DEFINE_VPU_STEP

		ret = vpu_prepare_regulator_and_clock(vpu_dev->dev[core]);
		CHECK_RET("fail to prepare regulator or clock!\n");
	}
	return 0;

out:

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_service_cores[i].vpu_service_task != NULL) {
			kfree(vpu_service_cores[i].vpu_service_task);
			vpu_service_cores[i].vpu_service_task = NULL;
		}

		if (vpu_service_cores[i].work_buf)
			vpu_free_shared_memory(vpu_service_cores[i].work_buf);
	}

	return ret;
}

int vpu_uninit_hw(void)
{
	int i;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		cancel_delayed_work(&(power_counter_work[i].my_work));

		if (vpu_service_cores[i].vpu_service_task != NULL) {
			kthread_stop(vpu_service_cores[i].vpu_service_task);
			kfree(vpu_service_cores[i].vpu_service_task);
			vpu_service_cores[i].vpu_service_task = NULL;
		}

		if (vpu_service_cores[i].work_buf) {
			vpu_free_shared_memory(vpu_service_cores[i].work_buf);
			vpu_service_cores[i].work_buf = NULL;
		}
	}

	vpu_unprepare_regulator_and_clock();

	if (m4u_client) {
		m4u_destroy_client(m4u_client);
		m4u_client = NULL;
	}

	if (ion_client) {
		ion_client_destroy(ion_client);
		ion_client = NULL;
	}

	if (wq) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		wq = NULL;
	}

	return 0;
}


static int vpu_check_precond(int core)
{
	uint32_t status;
	size_t i;

	/* wait 1 seconds, if not ready or busy */
	for (i = 0; i < 50; i++) {
		status = vpu_read_field(core, FLD_XTENSA_INFO00);
		switch (status) {
		case VPU_STATE_READY:
		case VPU_STATE_IDLE:
		case VPU_STATE_ERROR:
			return 0;
		case VPU_STATE_NOT_READY:
		case VPU_STATE_BUSY:
			msleep(20);
			break;
		case VPU_STATE_TERMINATED:
			return -EBADFD;
		}
	}
	LOG_ERR("core(%d) still busy(%d) after wait 1 second.\n", core, status);
	return -EBUSY;
}

static int vpu_check_postcond(int core)
{
	uint32_t status = vpu_read_field(core, FLD_XTENSA_INFO00);

	LOG_DBG("vpu_check_postcond (0x%x)", status);

	switch (status) {
	case VPU_STATE_READY:
	case VPU_STATE_IDLE:
		return 0;
	case VPU_STATE_NOT_READY:
	case VPU_STATE_BUSY:
		return -EIO;
	default:
		return -EINVAL;
	}
}

int vpu_hw_enable_jtag(bool enabled)
{
	int ret;
	int TEMP_CORE = 0;

	vpu_get_power(TEMP_CORE);
#if 0
	ret = mt_set_gpio_mode(GPIO14 | 0x80000000, enabled ? GPIO_MODE_05 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO15 | 0x80000000, enabled ? GPIO_MODE_05 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO16 | 0x80000000, enabled ? GPIO_MODE_05 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO17 | 0x80000000, enabled ? GPIO_MODE_05 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO18 | 0x80000000, enabled ? GPIO_MODE_05 : GPIO_MODE_01);
#else
	ret = 0;
#endif

	CHECK_RET("fail to config gpio-jtag2!\n");

	vpu_write_field(TEMP_CORE, FLD_SPNIDEN, enabled);
	vpu_write_field(TEMP_CORE, FLD_SPIDEN, enabled);
	vpu_write_field(TEMP_CORE, FLD_NIDEN, enabled);
	vpu_write_field(TEMP_CORE, FLD_DBG_EN, enabled);

out:
	vpu_put_power(TEMP_CORE);
	return ret;
}

int vpu_hw_boot_sequence(int core)
{
	int ret;
	uint64_t ptr_ctrl;
	uint64_t ptr_reset;
	uint64_t ptr_axi_0;
	uint64_t ptr_axi_1;
	unsigned int reg_value = 0;

	vpu_trace_begin("vpu_hw_boot_sequence");
	LOG_INF("boot-seq core(%d)", core);
	LOG_DBG("CTRL(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x110));
	LOG_DBG("XTENSA_INT(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x114));
	LOG_DBG("CTL_XTENSA_INT(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x118));
	LOG_DBG("CTL_XTENSA_INT_CLR(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x11C));

	lock_command(core);
	ptr_ctrl = vpu_service_cores[core].vpu_base + g_vpu_reg_descs[REG_CTRL].offset;
	ptr_reset = vpu_service_cores[core].vpu_base + g_vpu_reg_descs[REG_SW_RST].offset;
	ptr_axi_0 = vpu_service_cores[core].vpu_base + g_vpu_reg_descs[REG_AXI_DEFAULT0].offset;
	ptr_axi_1 = vpu_service_cores[core].vpu_base + g_vpu_reg_descs[REG_AXI_DEFAULT1].offset;

	/* 1. write register */
	/* set specific address for reset vector in external boot */
	reg_value = vpu_read_field(core, FLD_CORE_XTENSA_ALTRESETVEC);
	LOG_INF("vpu bf ALTRESETVEC (0x%x), RV(0x%x)\n",
		reg_value, VPU_MVA_RESET_VECTOR);
	switch (core) {
	case 0:
	default:
		vpu_write_field(core, FLD_CORE_XTENSA_ALTRESETVEC, VPU_MVA_RESET_VECTOR);
		break;
	case 1:
		vpu_write_field(core, FLD_CORE_XTENSA_ALTRESETVEC, VPU2_MVA_RESET_VECTOR);
		break;
	case 2:
		vpu_write_field(core, FLD_CORE_XTENSA_ALTRESETVEC, VPU3_MVA_RESET_VECTOR);
		break;
	}
	reg_value = vpu_read_field(core, FLD_CORE_XTENSA_ALTRESETVEC);
	LOG_INF("vpu af ALTRESETVEC (0x%x), RV(0x%x)\n",
		reg_value, VPU_MVA_RESET_VECTOR);

	VPU_SET_BIT(ptr_ctrl, 31);      /* csr_p_debug_enable */
	VPU_SET_BIT(ptr_ctrl, 26);      /* debug interface cock gated enable */
	VPU_SET_BIT(ptr_ctrl, 19);      /* force to boot based on XTENSA_ALTRESETVEC */
	VPU_SET_BIT(ptr_ctrl, 23);      /* RUN_STALL pull up */
	VPU_SET_BIT(ptr_ctrl, 17);      /* pif gated enable */
	#if 0
	VPU_SET_BIT(ptr_ctrl, 29);     /* SHARE_SRAM_CONFIG: default: imem 196K, icache 0K*/
	VPU_CLR_BIT(ptr_ctrl, 28);
	VPU_CLR_BIT(ptr_ctrl, 27);
	#else
	/* SHARE_SRAM_CONFIG: default: imem 64, icache 128K*/
	VPU_CLR_BIT(ptr_ctrl, 29);
	VPU_CLR_BIT(ptr_ctrl, 28);
	VPU_CLR_BIT(ptr_ctrl, 27);
	#endif

	VPU_SET_BIT(ptr_reset, 12);     /* OCD_HALT_ON_RST pull up */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_reset, 12);     /* OCD_HALT_ON_RST pull down */
	VPU_SET_BIT(ptr_reset, 4);      /* B_RST pull up */
	VPU_SET_BIT(ptr_reset, 8);      /* D_RST pull up */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_reset, 4);      /* B_RST pull down */
	VPU_CLR_BIT(ptr_reset, 8);      /* D_RST pull down */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_ctrl, 17);      /* pif gated disable, to prevent unknown propagate to BUS */
#ifndef BYPASS_M4U_DBG
	VPU_SET_BIT(ptr_axi_0, 22);		/* AXI Request via M4U */
	VPU_SET_BIT(ptr_axi_0, 27);
	VPU_SET_BIT(ptr_axi_1, 4);       /* AXI Request via M4U */
	VPU_SET_BIT(ptr_axi_1, 9);
#endif
	LOG_DBG("REG_AXI_DEFAULT0(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x13C));
	LOG_DBG("REG_AXI_DEFAULT1(0x%x)", vpu_read_reg32(vpu_service_cores[core].vpu_base, CTRL_BASE_OFFSET + 0x140));

	/* 2. trigger to run */
	LOG_DBG("vpu dsp:running (%d/0x%x)", core, vpu_read_field(core, FLD_SRAM_CONFIGURE));
	vpu_trace_begin("dsp:running");

	VPU_CLR_BIT(ptr_ctrl, 23);      /* RUN_STALL pull down */


	/* 3. wait until done */
	ret = wait_command(core);
	vpu_trace_end();
	if (ret) {
		vpu_dump_register(NULL);
		vpu_aee("VPU Timeout", "timeout to external boot\n");
		goto out;
	}

	/* 4. check the result of boot sequence */
	ret = vpu_check_postcond(core);
	CHECK_RET("fail to boot vpu!\n");

out:
	unlock_command(core);
	vpu_trace_end();
	LOG_INF("vpu_hw_boot_sequence -");
	return ret;
}

int vpu_hw_set_debug(int core)
{
	int ret;
	struct timespec now;

	LOG_DBG("vpu_hw_set_debug (%d)+", core);
	vpu_trace_begin("vpu_hw_set_debug");

	lock_command(core);

	/* 1. set debug */
	getnstimeofday(&now);
	vpu_write_field(core, FLD_XTENSA_INFO01, VPU_CMD_SET_DEBUG);
	vpu_write_field(core, FLD_XTENSA_INFO19, vpu_service_cores[core].iram_data_mva);
	vpu_write_field(core, FLD_XTENSA_INFO21, vpu_service_cores[core].work_buf->pa + VPU_OFFSET_LOG);
	vpu_write_field(core, FLD_XTENSA_INFO22, VPU_SIZE_LOG_BUF);
	vpu_write_field(core, FLD_XTENSA_INFO23, now.tv_sec * 1000000 + now.tv_nsec / 1000);
	LOG_DBG("work_buf->pa + VPU_OFFSET_LOG (0x%lx)",
		(unsigned long)(vpu_service_cores[core].work_buf->pa + VPU_OFFSET_LOG));
	LOG_DBG("vpu_set ok, running");

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	vpu_write_field(core, FLD_CTL_INT, 1);
	LOG_DBG("debug timestamp: %.2lu:%.2lu:%.2lu:%.6lu\n", (now.tv_sec / 3600) % (24),
			(now.tv_sec / 60) % (60), now.tv_sec % 60, now.tv_nsec / 1000);
	/* 3. wait until done */
	ret = wait_command(core);
	vpu_trace_end();
	CHECK_RET("timeout of set debug\n");

	/* 4. check the result */
	ret = vpu_check_postcond(core);
	CHECK_RET("fail to set debug!\n");

out:
	unlock_command(core);
	vpu_trace_end();
	LOG_DBG("vpu_hw_set_debug -");
	return ret;
}

int vpu_get_name_of_algo(int core, int id, char **name)
{
	int i;
	int tmp = id;
	struct vpu_image_header *header;

	header = (struct vpu_image_header *) ((uintptr_t)vpu_service_cores[core].bin_base + (VPU_OFFSET_IMAGE_HEADERS));
	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		if (tmp > header[i].algo_info_count) {
			tmp -= header[i].algo_info_count;
			continue;
		}

		*name = header[i].algo_infos[tmp - 1].name;
		return 0;
	}

	*name = NULL;
	LOG_ERR("algo is not existed, id=%d\n", id);
	return -ENOENT;
}

int vpu_get_entry_of_algo(int core, char *name, int *id, unsigned int *mva, int *length)
{
	int i, j;
	int s = 1;
	unsigned int coreMagicNum;
	struct vpu_algo_info *algo_info;
	struct vpu_image_header *header;

	LOG_DBG("[vpu] vpu_get_entry_of_algo +\n");
	/* coreMagicNum = ( 0x60 | (0x01 << core) ); */
	/* ignore vpu version */
	coreMagicNum = (0x01 << core);

	header = (struct vpu_image_header *) ((uintptr_t)vpu_service_cores[core].bin_base + (VPU_OFFSET_IMAGE_HEADERS));
	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		for (j = 0; j < header[i].algo_info_count; j++) {
			algo_info = &header[i].algo_infos[j];
			LOG_INF("debug, algo name: %s/%s, core info:0x%x, input core:%d, magicNum: 0x%x, 0x%x\n",
				name, algo_info->name, (unsigned int)(algo_info->vpu_core),
				core, (unsigned int)coreMagicNum,
				algo_info->vpu_core & coreMagicNum);
			/* CHRISTODO */
			if ((strcmp(name, algo_info->name) == 0) &&
				(algo_info->vpu_core & coreMagicNum)) {
				LOG_DBG("algo_info->offset(0x%x)/0x%x",
					algo_info->offset, (unsigned int)(vpu_service_cores[core].algo_data_mva));
				*mva = algo_info->offset - VPU_OFFSET_ALGO_AREA + vpu_service_cores[core].algo_data_mva;
				LOG_DBG("*mva(0x%x/0x%lx), s(%d)", *mva, (unsigned long)(*mva), s);
				*length = algo_info->length;
				*id = s;
				return 0;
			}
			s++;
		}
	}

	*id = 0;
	LOG_ERR("algo is not existed, name=%s\n", name);
	return -ENOENT;
};


int vpu_ext_be_busy(void)
{
	int ret;
	/* CHRISTODO */
	int TEMP_CORE = 0;

	lock_command(TEMP_CORE);

	/* 1. write register */
	vpu_write_field(TEMP_CORE, FLD_XTENSA_INFO01, VPU_CMD_EXT_BUSY);            /* command: be busy */
	/* 2. trigger interrupt */

	vpu_write_field(TEMP_CORE, FLD_CTL_INT, 1);

	/* 3. wait until done */
	ret = wait_command(TEMP_CORE);

	unlock_command(TEMP_CORE);
	return ret;
}

int vpu_boot_up(int core)
{
	int ret;

	LOG_INF("[vpu_%d] boot_up +\n", core);
	mutex_lock(&power_mutex[core]);
	if (vpu_service_cores[core].is_running) {
		mutex_unlock(&power_mutex[core]);
		return 0;
	}
	LOG_INF("[vpu_%d] boot_up flag2\n", core);

	vpu_trace_begin("vpu_boot_up");

	ret = vpu_enable_regulator_and_clock(core);
	CHECK_RET("fail to enable regulator or clock\n");

	ret = vpu_hw_boot_sequence(core);
	CHECK_RET("fail to do boot sequence\n");
	LOG_DBG("[vpu_%d] vpu_hw_boot_sequence done\n", core);

	ret = vpu_hw_set_debug(core);
	CHECK_RET("fail to set debug\n");
	LOG_DBG("[vpu_%d] vpu_hw_set_debug done\n", core);

	vpu_service_cores[core].is_running = true;

out:

	if (ret) {
		ret = vpu_disable_regulator_and_clock(core);
		CHECK_RET("fail to disable regulator and clock\n");
	}

	vpu_trace_end();
	mutex_unlock(&power_mutex[core]);
	return ret;
}

int vpu_shut_down(int core)
{
	int ret;

	LOG_INF("[vpu_%d] shutdown +\n", core);
	mutex_lock(&power_mutex[core]);
	if (!vpu_service_cores[core].is_running) {
		mutex_unlock(&power_mutex[core]);
		return 0;
	}

	vpu_trace_begin("vpu_shut_down");

	ret = vpu_disable_regulator_and_clock(core);
	CHECK_RET("fail to disable regulator and clock\n");

	vpu_service_cores[core].current_algo = 0;
	vpu_service_cores[core].is_running = false;

out:
	vpu_trace_end();
	mutex_unlock(&power_mutex[core]);
	LOG_INF("[vpu_%d] shutdown -\n", core);
	return ret;
}

int vpu_change_power_mode(uint8_t mode)
{
	int ret = 0;
	/* CHRISTODO */
	int TEMP_CORE = 0;
	bool dynamic = mode == VPU_POWER_MODE_DYNAMIC;

	if (is_power_debug_lock)
		return 0;

	if (is_power_dynamic == dynamic)
		return 0;

	/* power on immediately if not dynamic, and vice versa */
	if (!dynamic)
		ret = vpu_get_power(TEMP_CORE);
	else
		vpu_put_power(TEMP_CORE);

	if (ret == 0)
		is_power_dynamic = dynamic;

	return ret;
}

int vpu_change_power_opp(uint8_t index)
{
	if (is_power_debug_lock)
		return 0;

	if (index == VPU_POWER_OPP_UNREQUEST)
		index = opps.count - 1;

	if (index >= opps.count) {
		LOG_ERR("opp is invalid, index:%d max:%d\n", index, opps.count - 1);
	    index = opps.count - 1;
	}

	opps.vcore.index = opps.vcore.opp_map[index];
	opps.dsp.index = opps.dsp.opp_map[index];
	opps.ipu_if.index = opps.ipu_if.opp_map[index];

	return 0;
}

int vpu_hw_load_algo(int core, struct vpu_algo *algo)
{
	int ret;

	LOG_DBG("[vpu_%d] vpu_hw_load_algo +\n", core);
	/* no need to reload algo if have same loaded algo*/
	if (vpu_service_cores[core].current_algo == algo->id[core])
		return 0;

	vpu_trace_begin("vpu_hw_load_algo(%d)", algo->id[core]);
	ret = vpu_get_power(core);
	CHECK_RET("fail to get power!\n");
	LOG_DBG("[vpu_%d] vpu_get_power done\n", core);

	lock_command(core);
	LOG_DBG("start to load algo\n");

	ret = vpu_check_precond(core);
	CHECK_RET("have wrong status before do loader!\n");
	LOG_DBG("[vpu_%d] vpu_check_precond done\n", core);

	LOG_DBG("[vpu_%d] algo ptr/length (0x%lx/0x%x)\n", core,
		(unsigned long)algo->bin_ptr, algo->bin_length);
	/* 1. write register */
	vpu_write_field(core, FLD_XTENSA_INFO01, VPU_CMD_DO_LOADER);           /* command: d2d */
	vpu_write_field(core, FLD_XTENSA_INFO12, algo->bin_ptr);              /* binary data's address */
	vpu_write_field(core, FLD_XTENSA_INFO13, algo->bin_length);           /* binary data's length */
	vpu_write_field(core, FLD_XTENSA_INFO15, opps.dsp.values[opps.dsp.index]);
	vpu_write_field(core, FLD_XTENSA_INFO16, opps.ipu_if.values[opps.ipu_if.index]);

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	LOG_DBG("[vpu_%d] dsp:running\n", core);
	vpu_write_field(core, FLD_CTL_INT, 1);

	/* 3. wait until done */
	ret = wait_command(core);
	vpu_trace_end();
	if (ret) {
		vpu_dump_mesg(NULL);
		vpu_dump_register(NULL);
		vpu_aee("VPU Timeout", "timeout to do loader, algo_id=%d\n", vpu_service_cores[core].current_algo);
		goto out;
	}

	/* 4. update the id of loaded algo */
	vpu_service_cores[core].current_algo = algo->id[core];

out:
	unlock_command(core);
	vpu_put_power(core);
	vpu_trace_end();
	LOG_DBG("[vpu] vpu_hw_load_algo -\n");
	return ret;
}

int vpu_hw_enque_request(int core, struct vpu_request *request)
{
	int ret;

	LOG_INF("[vpu_%d/%d] eq + ", core, request->algo_id[core]);
	vpu_trace_begin("vpu_hw_enque_request(%d)", request->algo_id[core]);
	ret = vpu_get_power(core);
	CHECK_RET("fail to get power!\n");

	lock_command(core);
	LOG_DBG("start to enque request\n");

	ret = vpu_check_precond(core);
	if (ret) {
		request->status = VPU_REQ_STATUS_BUSY;
		LOG_ERR("error state before enque request!\n");
		goto out;
	}

	memcpy((void *)(uintptr_t)vpu_service_cores[core].work_buf->va, request->buffers,
			sizeof(struct vpu_buffer) * request->buffer_count);

	if (g_vpu_log_level > 4)
		vpu_dump_buffer_mva(request);
	LOG_DBG("[vpu] vpu_hw_enque_request check_precond done ");
	/* 1. write register */
	/* command: d2d */
	vpu_write_field(core, FLD_XTENSA_INFO01, VPU_CMD_DO_D2D);
	/* buffer count */
	vpu_write_field(core, FLD_XTENSA_INFO12, request->buffer_count);
	/* pointer to array of struct vpu_buffer */
	vpu_write_field(core, FLD_XTENSA_INFO13, vpu_service_cores[core].work_buf->pa);
	/* pointer to property buffer */
	vpu_write_field(core, FLD_XTENSA_INFO14, request->sett_ptr);
	/* size of property buffer */
	vpu_write_field(core, FLD_XTENSA_INFO15, request->sett_length);

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	LOG_DBG("[vpu] vpu_hw_enque_request running... ");
	vpu_write_field(core, FLD_CTL_INT, 1);

	/* 3. wait until done */
	ret = wait_command(core);
	vpu_trace_end();
	if (ret) {
		request->status = VPU_REQ_STATUS_TIMEOUT;
		vpu_dump_buffer_mva(request);
		vpu_dump_mesg(NULL);
		vpu_dump_register(NULL);
		vpu_aee("VPU Timeout", "timeout to do d2d, algo_id=%d\n", vpu_service_cores[core].current_algo);
		goto out;
	}

	request->status = (vpu_check_postcond(core)) ? VPU_REQ_STATUS_FAILURE : VPU_REQ_STATUS_SUCCESS;

out:
	unlock_command(core);
	vpu_put_power(core);
	vpu_trace_end();
	LOG_DBG("[vpu] vpu_hw_enque_request - (%d)", request->status);
	return ret;

}

int vpu_hw_get_algo_info(int core, struct vpu_algo *algo)
{
	int ret = 0;
	int port_count = 0;
	int info_desc_count = 0;
	int sett_desc_count = 0;
	unsigned int ofs_ports, ofs_info, ofs_info_descs, ofs_sett_descs;
	int i;

	vpu_trace_begin("vpu_hw_get_algo_info(%d)", algo->id[core]);
	ret = vpu_get_power(core);
	CHECK_RET("fail to get power!\n");

	lock_command(core);
	LOG_DBG("start to get algo, algo_id=%d\n", algo->id[core]);

	ret = vpu_check_precond(core);
	CHECK_RET("have wrong status before get algo!\n");

	ofs_ports = 0;
	ofs_info = sizeof(((struct vpu_algo *)0)->ports);
	ofs_info_descs = ofs_info + algo->info_length;
	ofs_sett_descs = ofs_info_descs + sizeof(((struct vpu_algo *)0)->info_descs);
	LOG_DBG("[vpu] vpu_hw_get_algo_info check precond done\n");

	/* 1. write register */
	vpu_write_field(core, FLD_XTENSA_INFO01, VPU_CMD_GET_ALGO);   /* command: get algo */
	vpu_write_field(core, FLD_XTENSA_INFO06, vpu_service_cores[core].work_buf->pa + ofs_ports);
	vpu_write_field(core, FLD_XTENSA_INFO07, vpu_service_cores[core].work_buf->pa + ofs_info);
	vpu_write_field(core, FLD_XTENSA_INFO08, algo->info_length);
	vpu_write_field(core, FLD_XTENSA_INFO10, vpu_service_cores[core].work_buf->pa + ofs_info_descs);
	vpu_write_field(core, FLD_XTENSA_INFO12, vpu_service_cores[core].work_buf->pa + ofs_sett_descs);

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	LOG_DBG("[vpu] vpu_hw_get_algo_info running...\n");
	vpu_write_field(core, FLD_CTL_INT, 1);

	/* 3. wait until done */
	ret = wait_command(core);
	vpu_trace_end();
	if (ret) {
		vpu_dump_mesg(NULL);
		vpu_dump_register(NULL);
		vpu_aee("VPU Timeout", "timeout to get algo, algo_id=%d\n", vpu_service_cores[core].current_algo);
		goto out;
	}

	/* 4. get the return value */
	port_count = vpu_read_field(core, FLD_XTENSA_INFO05);
	info_desc_count = vpu_read_field(core, FLD_XTENSA_INFO09);
	sett_desc_count = vpu_read_field(core, FLD_XTENSA_INFO11);
	algo->port_count = port_count;
	algo->info_desc_count = info_desc_count;
	algo->sett_desc_count = sett_desc_count;

	LOG_DBG("end of get algo, port_count=%d, info_desc_count=%d, sett_desc_count=%d\n",
		port_count, info_desc_count, sett_desc_count);

	/* 5. write back data from working buffer */
	memcpy((void *)(uintptr_t)algo->ports, (void *)((uintptr_t)vpu_service_cores[core].work_buf->va + ofs_ports),
			sizeof(struct vpu_port) * port_count);

	for (i = 0 ; i < algo->port_count ; i++) {
		LOG_DBG("port %d.. id=%d, name=%s, dir=%d, usage=%d\n",
			i, algo->ports[i].id, algo->ports[i].name, algo->ports[i].dir, algo->ports[i].usage);
	}

	memcpy((void *)(uintptr_t)algo->info_ptr, (void *)((uintptr_t)vpu_service_cores[core].work_buf->va + ofs_info),
			algo->info_length);
	memcpy((void *)(uintptr_t)algo->info_descs,
			(void *)((uintptr_t)vpu_service_cores[core].work_buf->va + ofs_info_descs),
			sizeof(struct vpu_prop_desc) * info_desc_count);
	memcpy((void *)(uintptr_t)algo->sett_descs,
			(void *)((uintptr_t)vpu_service_cores[core].work_buf->va + ofs_sett_descs),
			sizeof(struct vpu_prop_desc) * sett_desc_count);

	LOG_DBG("end of get algo 2, port_count=%d, info_desc_count=%d, sett_desc_count=%d\n",
		algo->port_count, algo->info_desc_count, algo->sett_desc_count);

out:
	unlock_command(core);
	vpu_put_power(core);
	vpu_trace_end();
	LOG_DBG("[vpu] vpu_hw_get_algo_info -\n");
	return ret;
}

void vpu_hw_lock(struct vpu_user *user)
{
	/* CHRISTODO */
	int TEMP_CORE = 0;

	if (user->locked)
		LOG_ERR("double locking bug, pid=%d, tid=%d\n",
				user->open_pid, user->open_tgid);
	else {
		mutex_lock(&lock_mutex);
		is_locked = true;
		user->locked = true;
		vpu_get_power(TEMP_CORE);
	}
}

void vpu_hw_unlock(struct vpu_user *user)
{
	/* CHRISTODO */
	int TEMP_CORE = 0;

	if (user->locked) {
		vpu_put_power(TEMP_CORE);
		is_locked = false;
		user->locked = false;
		wake_up_interruptible(&lock_wait);
		mutex_unlock(&lock_mutex);
	} else
		LOG_ERR("should not unlock while unlocked, pid=%d, tid=%d\n",
				user->open_pid, user->open_tgid);
}

int vpu_alloc_shared_memory(struct vpu_shared_memory **shmem, struct vpu_shared_memory_param *param)
{
	int ret = 0;

	/* CHRISTODO */
	struct ion_mm_data mm_data;
	struct ion_sys_data sys_data;
	struct ion_handle *handle = NULL;

	*shmem = kzalloc(sizeof(struct vpu_shared_memory), GFP_KERNEL);
	ret = (*shmem == NULL);
	CHECK_RET("fail to kzalloc 'struct memory'!\n");

	handle = ion_alloc(ion_client, param->size, 0, ION_HEAP_MULTIMEDIA_MASK, 0);
	ret = (handle == NULL) ? -ENOMEM : 0;
	CHECK_RET("fail to alloc ion buffer, ret=%d\n", ret);
	(*shmem)->handle = (void *) handle;

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER_EXT;
	mm_data.config_buffer_param.kernel_handle = handle;
	mm_data.config_buffer_param.module_id = VPU_PORT_OF_IOMMU;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 1;
	if (param->fixed_addr) {
		mm_data.config_buffer_param.reserve_iova_start = param->fixed_addr;
		mm_data.config_buffer_param.reserve_iova_end = IOMMU_VA_END; /*param->fixed_addr;*/
	} else {
		/* CHRISTODO, need revise starting address for working buffer*/
		mm_data.config_buffer_param.reserve_iova_start = 0x60000000; /*IOMMU_VA_START*/
		mm_data.config_buffer_param.reserve_iova_end = IOMMU_VA_END;
	}
	ret = ion_kernel_ioctl(ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&mm_data);
	CHECK_RET("fail to config ion buffer, ret=%d\n", ret);

	/* map pa */
	LOG_DBG("vpu param->require_pa(%d)\n", param->require_pa);
	if (param->require_pa) {
		sys_data.sys_cmd = ION_SYS_GET_PHYS;
		sys_data.get_phys_param.kernel_handle = handle;
		sys_data.get_phys_param.phy_addr = VPU_PORT_OF_IOMMU << 24 | ION_FLAG_GET_FIXED_PHYS;
		sys_data.get_phys_param.len = ION_FLAG_GET_FIXED_PHYS;
		ret = ion_kernel_ioctl(ion_client, ION_CMD_SYSTEM, (unsigned long)&sys_data);
		CHECK_RET("fail to get ion phys, ret=%d\n", ret);
		(*shmem)->pa = sys_data.get_phys_param.phy_addr;
		(*shmem)->length = sys_data.get_phys_param.len;
	}

	/* map va */
	if (param->require_va) {
		(*shmem)->va = (uint64_t)(uintptr_t)ion_map_kernel(ion_client, handle);
		ret = ((*shmem)->va) ? 0 : -ENOMEM;
		CHECK_RET("fail to map va of buffer!\n");
	}

	return 0;

out:
	if (handle)
		ion_free(ion_client, handle);

	if (*shmem) {
		kfree(*shmem);
		*shmem = NULL;
	}

	return ret;
}

void vpu_free_shared_memory(struct vpu_shared_memory *shmem)
{
	struct ion_handle *handle;

	if (shmem == NULL)
		return;

	handle = (struct ion_handle *) shmem->handle;
	if (handle) {
		ion_unmap_kernel(ion_client, handle);
		ion_free(ion_client, handle);
	}

	kfree(shmem);
}

int vpu_dump_buffer_mva(struct vpu_request *request)
{
	struct vpu_buffer *buf;
	struct vpu_plane *plane;
	int i, j;

	LOG_DBG("dump request - setting: 0x%x, length: %d\n",
			(uint32_t) request->sett_ptr, request->sett_length);

	for (i = 0; i < request->buffer_count; i++) {
		buf = &request->buffers[i];
		LOG_DBG("  buffer[%d] - port: %d, size: %dx%d, format: %d\n",
				i, buf->port_id, buf->width, buf->height, buf->format);

		for (j = 0; j < buf->plane_count; j++) {
			plane = &buf->planes[j];
			LOG_DBG("	 plane[%d] - ptr: 0x%x, length: %d, stride: %d\n",
					j, (uint32_t) plane->ptr, plane->length, plane->stride);
		}
	}

	return 0;

}

int vpu_dump_register(struct seq_file *s)
{
	int i, j;
	bool first_row_of_field;
	struct vpu_reg_desc *reg;
	struct vpu_reg_field_desc *field;
	int TEMP_CORE = 0;

#define LINE_BAR "  +---------------+------+---+---+-------------------------+----------+\n"
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-15s|%-6s|%-3s|%-3s|%-25s|%-10s|\n",
				  "Register", "Offset", "MSB", "LSB", "Field", "Value");
	vpu_print_seq(s, LINE_BAR);


	for (i = 0; i < VPU_NUM_REGS; i++) {
		reg = &g_vpu_reg_descs[i];
#ifndef MTK_VPU_DVT
		if (reg->reg < REG_DEBUG_INFO00)
			continue;
#endif
		first_row_of_field = true;

		for (j = 0; j < VPU_NUM_REG_FIELDS; j++) {
			field = &g_vpu_reg_field_descs[j];
			if (reg->reg != field->reg)
				continue;

			if (first_row_of_field) {
				first_row_of_field = false;
				vpu_print_seq(s, "  |%-15s|0x%-4.4x|%-3d|%-3d|%-25s|0x%-8.8x|\n",
							  reg->name,
							  reg->offset,
							  field->msb,
							  field->lsb,
							  field->name,
							  vpu_read_field(TEMP_CORE, (enum vpu_reg_field) j));
			} else {
				vpu_print_seq(s, "  |%-15s|%-6s|%-3d|%-3d|%-25s|0x%-8.8x|\n",
							  "", "",
							  field->msb,
							  field->lsb,
							  field->name,
							  vpu_read_field(TEMP_CORE, (enum vpu_reg_field) j));
			}
		}
		vpu_print_seq(s, LINE_BAR);
	}
#undef LINE_BAR

	return 0;
}

int vpu_dump_image_file(struct seq_file *s)
{
	int i, j, id = 1;
	struct vpu_algo_info *algo_info;
	struct vpu_image_header *header;
	/* CHRISTODO */
	int TEMP_CORE = 0;

#define LINE_BAR "  +------+-----+--------------------------------+-----------+----------+\n"
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-6s|%-5s|%-32s|%-11s|%-10s|\n",
				  "Header", "Id", "Name", "MVA", "Length");
	vpu_print_seq(s, LINE_BAR);

	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		header = (struct vpu_image_header *) ((uintptr_t)vpu_service_cores[TEMP_CORE].bin_base +
			(VPU_OFFSET_IMAGE_HEADERS) + sizeof(struct vpu_image_header) * i);
		for (j = 0; j < header->algo_info_count; j++) {
			algo_info = &header->algo_infos[j];

			vpu_print_seq(s, "  |%-6d|%-5d|%-32s|0x%-9x|0x%-8x|\n",
						  (i + 1),
						  id,
						  algo_info->name,
						  algo_info->offset - VPU_OFFSET_MAIN_PROGRAM + VPU_MVA_MAIN_PROGRAM,
						  algo_info->length);
			id++;
		}
	}

	vpu_print_seq(s, LINE_BAR);
#undef LINE_BAR

#ifdef MTK_VPU_DUMP_BINARY
	{
		uint32_t dump_1k_size = (0x00000400);
		unsigned char *ptr = NULL;

		vpu_print_seq(s, "Reset Vector Data:\n");
		ptr = (unsigned char *) vpu_service_cores[TEMP_CORE].bin_base + VPU_OFFSET_RESET_VECTOR;
		for (i = 0; i < dump_1k_size / 2; i++, ptr++) {
			if (i % 16 == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			vpu_print_seq(s, "%02X ", *ptr);
		}
		vpu_print_seq(s, "\n");
		vpu_print_seq(s, "\n");
		vpu_print_seq(s, "Main Program Data:\n");
		ptr = (unsigned char *) vpu_service_cores[TEMP_CORE].bin_base + VPU_OFFSET_MAIN_PROGRAM;
		for (i = 0; i < dump_1k_size; i++, ptr++) {
			if (i % 16 == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			vpu_print_seq(s, "%02X ", *ptr);
		}
		vpu_print_seq(s, "\n");
	}
#endif

	return 0;
}

int vpu_dump_mesg(struct seq_file *s)
{
	char *ptr = NULL;
	char *log_head;
	/* CHRISTODO */
	int TEMP_CORE = 0;
	char *log_buf = (char *) ((uintptr_t)vpu_service_cores[TEMP_CORE].work_buf->va + VPU_OFFSET_LOG);

	if (g_vpu_log_level > 8) {
		int i;
		int line_pos = 0;
		char line_buffer[16 + 1] = {0};

		ptr = log_buf;
		vpu_print_seq(s, "Log Buffer:\n");
		for (i = 0; i < VPU_SIZE_LOG_BUF; i++, ptr++) {
			line_pos = i % 16;
			if (line_pos == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			line_buffer[line_pos] = isascii(*ptr) && isprint(*ptr) ? *ptr : '.';
			vpu_print_seq(s, "%02X ", *ptr);
			if (line_pos == 15)
				vpu_print_seq(s, " %s", line_buffer);

		}
		vpu_print_seq(s, "\n\n");
	}

	ptr = log_buf;

	/* set the last byte to '\0' */
	*(ptr + VPU_SIZE_LOG_BUF - 1) = '\0';

	/* skip the header part */
	ptr += VPU_SIZE_LOG_HEADER;
	log_head = strchr(ptr, '\0') + 1;

	vpu_print_seq(s, "vpu: print dsp log\n%s%s", log_head, ptr);

	return 0;
}

int vpu_dump_opp_table(struct seq_file *s)
{
	int i;

#define LINE_BAR "  +-----+------------+------------+------------+\n"
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-5s|%-12s|%-12s|%-12s|\n",
				  "OPP", "VCORE(uV)", "DSP(KHz)", "IPU_IF(KHz)");
	vpu_print_seq(s, LINE_BAR);

	for (i = 0; i < opps.count; i++) {
		vpu_print_seq(s, "  |%-5d|[%d]%-9d|[%d]%-9d|[%d]%-9d|\n",
				i,
				opps.vcore.opp_map[i],  opps.vcore.values[opps.vcore.opp_map[i]],
				opps.dsp.opp_map[i],    opps.dsp.values[opps.dsp.opp_map[i]],
				opps.ipu_if.opp_map[i], opps.ipu_if.values[opps.ipu_if.opp_map[i]]);
	}

	vpu_print_seq(s, LINE_BAR);
#undef LINE_BAR

	return 0;
}

int vpu_dump_power(struct seq_file *s)
{
	/* CHRISTODO */
	int TEMP_CORE = 0;

	vpu_print_seq(s, "dynamic(rw): %d\n",
			is_power_dynamic);

	vpu_print_seq(s, "dvfs_debug(rw): %d %d %d\n",
			opps.vcore.index,
			opps.dsp.index,
			opps.ipu_if.index);

	vpu_print_seq(s, "jtag(rw): %d\n", is_jtag_enabled);
	vpu_print_seq(s, "running(r): %d\n", vpu_service_cores[TEMP_CORE].is_running);
	vpu_print_seq(s, "lock(rw): %d\n", is_power_debug_lock);

	return 0;
}

int vpu_set_power_parameter(uint8_t param, int argc, int *args)
{
	int ret = 0;
	/* CHRISTODO */
	int TEMP_CORE = 0;

	switch (param) {
	case VPU_POWER_PARAM_DYNAMIC:
		ret = (argc == 1) ? 0 : -EINVAL;
		CHECK_RET("invalid argument, expected:1, received:%d\n", argc);

		/* should release the lock before vpu_change_power_mode() */
		is_power_debug_lock = false;
		ret = vpu_change_power_mode(args[0] ? VPU_POWER_MODE_DYNAMIC : VPU_POWER_MODE_ON);
		is_power_debug_lock = true;

		break;
	case VPU_POWER_PARAM_DVFS_DEBUG:
		ret = (argc == 3) ? 0 : -EINVAL;
		CHECK_RET("invalid argument, expected:3, received:%d\n", argc);

		/* step of vcore regulator */
		ret = args[0] >= opps.vcore.count;
		CHECK_RET("vcore's step is out-of-bound, count:%d\n", opps.vcore.count);

		/* step of dsp clock */
		ret = args[1] >= opps.dsp.count;
		CHECK_RET("dsp's stpe is out-of-bound, count:%d\n", opps.dsp.count);

		/* step of ipu if */
		ret = args[2] >= opps.ipu_if.count;
		CHECK_RET("ipu_if's step is out-of-bound, count:%d\n", opps.ipu_if.count);

		opps.vcore.index = args[0];
		opps.dsp.index = args[1];
		opps.ipu_if.index = args[2];

		is_power_debug_lock = true;

		if (is_power_dynamic)
			break;

		/* force to set the frequency and voltage */
		vpu_shut_down(TEMP_CORE);
		vpu_boot_up(TEMP_CORE);

		break;
	case VPU_POWER_PARAM_JTAG:
		ret = (argc == 1) ? 0 : -EINVAL;
		CHECK_RET("invalid argument, expected:1, received:%d\n", argc);

		is_jtag_enabled = args[0];
		ret = vpu_hw_enable_jtag(is_jtag_enabled);

		break;
	case VPU_POWER_PARAM_LOCK:
		ret = (argc == 1) ? 0 : -EINVAL;
		CHECK_RET("invalid argument, expected:1, received:%d\n", argc);

		is_power_debug_lock = args[0];

		break;
	default:
		LOG_ERR("unsupport the power parameter:%d\n", param);
		break;
	}

out:
	return ret;
}
