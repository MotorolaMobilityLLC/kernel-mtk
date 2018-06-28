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

#include "cmdq_mdp_common.h"
#include "cmdq_core.h"
#include "cmdq_device.h"
#include "cmdq_reg.h"
#ifdef CMDQ_PROFILE_MMP
#include "cmdq_mmp.h"
#endif
#ifdef CMDQ_COMMON_ENG_SUPPORT
#include "cmdq_engine_common.h"
#else
#include "cmdq_engine.h"
#endif
#include "smi_public.h"
#include <linux/slab.h>
#include <linux/pm_qos.h>
#include <linux/uaccess.h>
#include <linux/math64.h>
#include <linux/iopoll.h>
#ifdef CONFIG_MTK_QOS_SUPPORT
#include "cmdq_mdp_pmqos.h"
#include <mmdvfs_pmqos.h>
#endif

static struct cmdqMDPTaskStruct gCmdqMDPTask[MDP_MAX_TASK_NUM];
static int gCmdqMDPTaskIndex;
static long g_cmdq_mmsys_base;
atomic_t g_mdp_wrot0_usage;
#ifdef CONFIG_MTK_QOS_SUPPORT
static struct mdp_context gCmdqMdpContext;
static struct pm_qos_request mdp_bw_qos_request[MDP_TOTAL_THREAD];
static struct pm_qos_request mdp_clk_qos_request[MDP_TOTAL_THREAD];
static struct pm_qos_request isp_bw_qos_request[MDP_TOTAL_THREAD];
static struct pm_qos_request isp_clk_qos_request[MDP_TOTAL_THREAD];
static u64 g_freq_steps[MAX_FREQ_STEP];
static u32 step_size;
#endif

#define DP_TIMER_GET_DURATION_IN_US(start, end, duration)			\
do {										\
	uint64_t time1;								\
	uint64_t time2;								\
										\
	time1 = (uint64_t)(start.tv_sec) * 1000000 + (uint64_t)(start.tv_usec); \
	time2 = (uint64_t)(end.tv_sec) * 1000000   + (uint64_t)(end.tv_usec);   \
										\
	duration = (int32_t)(time2 - time1);					\
										\
	if (duration <= 0)							\
		duration = 1;							\
} while (0)

#define DP_BANDWIDTH(data, pixel, throughput, bandwidth)			\
do {										\
	uint64_t numerator;							\
	uint64_t denominator;							\
										\
	numerator = (uint64_t)(data) * (uint64_t)(throughput);			\
	denominator = (uint64_t)(pixel);					\
	if (denominator == 0)							\
		denominator = 1;						\
	bandwidth = (uint32_t)(div_s64(numerator, denominator));		\
} while (0)

void cmdq_mdp_init(void)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	s32 i = 0;
	s32 result = 0;

	INIT_LIST_HEAD(&gCmdqMdpContext.mdp_tasks);

	for (i = 0; i < MDP_TOTAL_THREAD; i++) {
		pm_qos_add_request(&mdp_bw_qos_request[i],  PM_QOS_MM_MEMORY_BANDWIDTH, PM_QOS_DEFAULT_VALUE);
		pm_qos_add_request(&mdp_clk_qos_request[i], PM_QOS_MDP_FREQ, PM_QOS_DEFAULT_VALUE);
		pm_qos_add_request(&isp_bw_qos_request[i],  PM_QOS_MM_MEMORY_BANDWIDTH, PM_QOS_DEFAULT_VALUE);
		pm_qos_add_request(&isp_clk_qos_request[i], PM_QOS_IMG_FREQ, PM_QOS_DEFAULT_VALUE);
		snprintf(mdp_bw_qos_request[i].owner, sizeof(mdp_bw_qos_request[i].owner) - 1, "mdp_bw_%d", i);
		snprintf(mdp_clk_qos_request[i].owner, sizeof(mdp_clk_qos_request[i].owner) - 1, "mdp_clk_%d", i);
		snprintf(isp_bw_qos_request[i].owner, sizeof(isp_bw_qos_request[i].owner) - 1, "isp_bw_%d", i);
		snprintf(isp_clk_qos_request[i].owner, sizeof(isp_clk_qos_request[i].owner) - 1, "isp_clk_%d", i);
	}
	/* Call mmdvfs_qos_get_freq_steps to get supported frequency */
	result = mmdvfs_qos_get_freq_steps(
					PM_QOS_MDP_FREQ,
					&g_freq_steps[0],
					&step_size);
	if (result < 0)
		CMDQ_ERR("get MMDVFS freq steps failed, result: %d\n", result);
#endif
}

void cmdq_mdp_deinit(void)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	s32 i = 0;

	for (i = 0; i < MDP_TOTAL_THREAD; i++) {

		pm_qos_remove_request(&isp_bw_qos_request[i]);
		pm_qos_remove_request(&isp_clk_qos_request[i]);
		pm_qos_remove_request(&mdp_bw_qos_request[i]);
		pm_qos_remove_request(&mdp_clk_qos_request[i]);
	}
#endif
}

/**************************************************************************************/
/*******************                    Platform dependent function                    ********************/
/**************************************************************************************/

struct RegDef {
	int offset;
	const char *name;
};

s32 cmdq_mdp_probe_virtual(void)
{
	return 0;
}

void cmdq_mdp_dump_mmsys_config_virtual(void)
{
	/* Do Nothing */
}

/* VENC callback function */
int32_t cmdqVEncDumpInfo_virtual(uint64_t engineFlag, int level)
{
	return 0;
}

/* Initialization & de-initialization MDP base VA */
void cmdq_mdp_init_module_base_VA_virtual(void)
{
	/* Do Nothing */
}

void cmdq_mdp_deinit_module_base_VA_virtual(void)
{
	/* Do Nothing */
}

/* query MDP clock is on  */
bool cmdq_mdp_clock_is_on_virtual(enum CMDQ_ENG_ENUM engine)
{
	return false;
}

/* enable MDP clock  */
void cmdq_mdp_enable_clock_virtual(bool enable, enum CMDQ_ENG_ENUM engine)
{
	/* Do Nothing */
}

/* Common Clock Framework */
void cmdq_mdp_init_module_clk_virtual(void)
{
	/* Do Nothing */
}

/* MDP engine dump */
void cmdq_mdp_dump_rsz_virtual(const unsigned long base, const char *label)
{
	uint32_t value[8] = { 0 };
	uint32_t request[8] = { 0 };
	uint32_t state = 0;

	value[0] = CMDQ_REG_GET32(base + 0x004);
	value[1] = CMDQ_REG_GET32(base + 0x00C);
	value[2] = CMDQ_REG_GET32(base + 0x010);
	value[3] = CMDQ_REG_GET32(base + 0x014);
	value[4] = CMDQ_REG_GET32(base + 0x018);
	CMDQ_REG_SET32(base + 0x040, 0x00000001);
	value[5] = CMDQ_REG_GET32(base + 0x044);
	CMDQ_REG_SET32(base + 0x040, 0x00000002);
	value[6] = CMDQ_REG_GET32(base + 0x044);
	CMDQ_REG_SET32(base + 0x040, 0x00000003);
	value[7] = CMDQ_REG_GET32(base + 0x044);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR("RSZ_CONTROL: 0x%08x, RSZ_INPUT_IMAGE: 0x%08x RSZ_OUTPUT_IMAGE: 0x%08x\n",
		 value[0], value[1], value[2]);
	CMDQ_ERR("RSZ_HORIZONTAL_COEFF_STEP: 0x%08x, RSZ_VERTICAL_COEFF_STEP: 0x%08x\n",
		 value[3], value[4]);
	CMDQ_ERR("RSZ_DEBUG_1: 0x%08x, RSZ_DEBUG_2: 0x%08x, RSZ_DEBUG_3: 0x%08x\n",
		 value[5], value[6], value[7]);

	/* parse state */
	/* .valid=1/request=1: upstream module sends data */
	/* .ready=1: downstream module receives data */
	state = value[6] & 0xF;
	request[0] = state & (0x1);	/* out valid */
	request[1] = (state & (0x1 << 1)) >> 1;	/* out ready */
	request[2] = (state & (0x1 << 2)) >> 2;	/* in valid */
	request[3] = (state & (0x1 << 3)) >> 3;	/* in ready */
	request[4] = (value[1] & 0x1FFF);	/* input_width */
	request[5] = (value[1] >> 16) & 0x1FFF;	/* input_height */
	request[6] = (value[2] & 0x1FFF);	/* output_width */
	request[7] = (value[2] >> 16) & 0x1FFF;	/* output_height */

	CMDQ_ERR("RSZ inRdy,inRsq,outRdy,outRsq: %d,%d,%d,%d (%s)\n",
		 request[3], request[2], request[1], request[0], cmdq_mdp_get_rsz_state(state));
	CMDQ_ERR("RSZ input_width,input_height,output_width,output_height: %d,%d,%d,%d\n",
		 request[4], request[5], request[6], request[7]);
}

void cmdq_mdp_dump_tdshp_virtual(const unsigned long base, const char *label)
{
	uint32_t value[8] = { 0 };

	value[0] = CMDQ_REG_GET32(base + 0x114);
	value[1] = CMDQ_REG_GET32(base + 0x11C);
	value[2] = CMDQ_REG_GET32(base + 0x104);
	value[3] = CMDQ_REG_GET32(base + 0x108);
	value[4] = CMDQ_REG_GET32(base + 0x10C);
	value[5] = CMDQ_REG_GET32(base + 0x120);
	value[6] = CMDQ_REG_GET32(base + 0x128);
	value[7] = CMDQ_REG_GET32(base + 0x110);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR("TDSHP INPUT_CNT: 0x%08x, OUTPUT_CNT: 0x%08x\n", value[0], value[1]);
	CMDQ_ERR("TDSHP INTEN: 0x%08x, INTSTA: 0x%08x, 0x10C: 0x%08x\n", value[2], value[3],
		 value[4]);
	CMDQ_ERR("TDSHP CFG: 0x%08x, IN_SIZE: 0x%08x, OUT_SIZE: 0x%08x\n", value[7], value[5],
		 value[6]);
}

/* MDP callback function */
int32_t cmdqMdpClockOn_virtual(uint64_t engineFlag)
{
	return 0;
}

int32_t cmdqMdpDumpInfo_virtual(uint64_t engineFlag, int level)
{
	return 0;
}

int32_t cmdqMdpResetEng_virtual(uint64_t engineFlag)
{
	return 0;
}

int32_t cmdqMdpClockOff_virtual(uint64_t engineFlag)
{
	return 0;
}

/* MDP Initialization setting */
void cmdqMdpInitialSetting_virtual(void)
{
	/* Do Nothing */
}

/* test MDP clock function */
uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr_virtual(void)
{
	return 0;
}

uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr_virtual(void)
{
	return 0;
}

uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr_virtual(void)
{
	return 0;
}

void testcase_clkmgr_mdp_virtual(void)
{
}

const char *cmdq_mdp_dispatch_virtual(uint64_t engineFlag)
{
	return "MDP";
}

void cmdq_mdp_trackTask_virtual(const struct TaskStruct *pTask)
{
	if (pTask) {
		memcpy(gCmdqMDPTask[gCmdqMDPTaskIndex].callerName,
			pTask->callerName, sizeof(pTask->callerName));
		if (pTask->userDebugStr)
			memcpy(gCmdqMDPTask[gCmdqMDPTaskIndex].userDebugStr,
				pTask->userDebugStr, (uint32_t)strlen(pTask->userDebugStr) + 1);
		else
			gCmdqMDPTask[gCmdqMDPTaskIndex].userDebugStr[0] = '\0';
	} else {
		gCmdqMDPTask[gCmdqMDPTaskIndex].callerName[0] = '\0';
		gCmdqMDPTask[gCmdqMDPTaskIndex].userDebugStr[0] = '\0';
	}

	CMDQ_MSG("cmdq_mdp_trackTask: caller: %s\n",
		gCmdqMDPTask[gCmdqMDPTaskIndex].callerName);
	CMDQ_MSG("cmdq_mdp_trackTask: DebugStr: %s\n",
		gCmdqMDPTask[gCmdqMDPTaskIndex].userDebugStr);
	CMDQ_MSG("cmdq_mdp_trackTask: Index: %d\n",
		gCmdqMDPTaskIndex);

	gCmdqMDPTaskIndex = (gCmdqMDPTaskIndex + 1) % MDP_MAX_TASK_NUM;
}

const char *cmdq_mdp_parse_error_module_by_hwflag_virtual(const struct TaskStruct *task)
{
	const char *module = NULL;
	const uint32_t ISP_ONLY[2] = {
		((1LL << CMDQ_ENG_ISP_IMGI) | (1LL << CMDQ_ENG_ISP_IMG2O)),
		((1LL << CMDQ_ENG_ISP_IMGI) | (1LL << CMDQ_ENG_ISP_IMG2O) |
		 (1LL << CMDQ_ENG_ISP_IMGO))
	};

	/* common part for both normal and secure path */
	/* for JPEG scenario, use HW flag is sufficient */
	if (task->engineFlag & (1LL << CMDQ_ENG_JPEG_ENC))
		module = "JPGENC";
	else if (task->engineFlag & (1LL << CMDQ_ENG_JPEG_DEC))
		module = "JPGDEC";
	else if ((ISP_ONLY[0] == task->engineFlag) || (ISP_ONLY[1] == task->engineFlag))
		module = "DIP_ONLY";

	/* for secure path, use HW flag is sufficient */
	do {
		if (module != NULL)
			break;

		if (!task->secData.is_secure) {
			/* normal path, need parse current running instruciton for more detail */
			break;
		} else if (CMDQ_ENG_MDP_GROUP_FLAG(task->engineFlag)) {
			module = "MDP";
			break;
		} else if (CMDQ_ENG_DPE_GROUP_FLAG(task->engineFlag)) {
			module = "DPE";
			break;
		} else if (CMDQ_ENG_RSC_GROUP_FLAG(task->engineFlag)) {
			module = "RSC";
			break;
		} else if (CMDQ_ENG_GEPF_GROUP_FLAG(task->engineFlag)) {
			module = "GEPF";
			break;
		}

		module = "CMDQ";
	} while (0);

	return module;
}

u64 cmdq_mdp_get_engine_group_bits_virtual(u32 engine_group)
{
	return 0;
}

void cmdq_mdp_error_reset_virtual(u64 engineFlag)
{
}

long cmdq_mdp_get_module_base_VA_MMSYS_CONFIG(void)
{
	return g_cmdq_mmsys_base;
}

static void cmdq_mdp_enable_common_clock_virtual(bool enable)
{
#ifdef CMDQ_PWR_AWARE
	if (enable) {
		/* Use SMI clock API */
		smi_bus_enable(SMI_LARB_MMSYS0, "CMDQ");
	} else {
		/* disable, reverse the sequence */
		smi_bus_disable(SMI_LARB_MMSYS0, "CMDQ");
	}
#endif	/* CMDQ_PWR_AWARE */
}

static u32 cmdq_mdp_meansure_bandwidth_virtual(u32 bandwidth)
{
	return bandwidth;
}

static void cmdq_mdp_isp_begin_task_virtual(struct TaskStruct *cmdq_task, struct TaskStruct *task_list[], u32 size)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	struct mdp_pmqos *isp_curr_pmqos;
	struct mdp_pmqos_record *pmqos_curr_record;
	struct timeval curr_time;
	int32_t diff;
	uint32_t thread_id = cmdq_task->thread - MDP_THREAD_START;
	uint32_t max_throughput = 0;
	uint32_t curr_bandwidth = 0;

	if (!(cmdq_task->engineFlag & (1LL << CMDQ_ENG_ISP_IMGI) &&
		cmdq_task->engineFlag & (1LL << CMDQ_ENG_ISP_IMG2O))) {
		return;
	}
	/* implement for pass2 only task */
	CMDQ_MSG("enter %s task:0x%p engine:0x%llx\n", __func__, cmdq_task, cmdq_task->engineFlag);
	pmqos_curr_record = kzalloc(sizeof(struct mdp_pmqos_record), GFP_KERNEL);
	cmdq_task->user_private = pmqos_curr_record;
	do_gettimeofday(&curr_time);

	if (!cmdq_task->prop_addr)
		return;

	isp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
	pmqos_curr_record->submit_tm = curr_time;
	pmqos_curr_record->end_tm.tv_sec = isp_curr_pmqos->tv_sec;
	pmqos_curr_record->end_tm.tv_usec = isp_curr_pmqos->tv_usec;

	CMDQ_MSG(
		"[MDP]isp %d pixel, isp %d byte, submit %06ld us, end %06ld us\n",
		isp_curr_pmqos->isp_total_pixel, isp_curr_pmqos->isp_bandwidth,
		pmqos_curr_record->submit_tm.tv_usec, pmqos_curr_record->end_tm.tv_usec);

	DP_TIMER_GET_DURATION_IN_US(pmqos_curr_record->submit_tm, pmqos_curr_record->end_tm, diff);
	max_throughput = isp_curr_pmqos->isp_total_pixel / diff;

	if (max_throughput > g_freq_steps[0]) {
		DP_BANDWIDTH(
			isp_curr_pmqos->isp_bandwidth,
			isp_curr_pmqos->isp_total_pixel,
			g_freq_steps[0],
			curr_bandwidth);
		DP_BANDWIDTH(
			isp_curr_pmqos->isp_bandwidth,
			isp_curr_pmqos->isp_total_pixel,
			g_freq_steps[0],
			curr_bandwidth);
	} else {
		DP_BANDWIDTH(
			isp_curr_pmqos->isp_bandwidth,
			isp_curr_pmqos->isp_total_pixel,
			max_throughput,
			curr_bandwidth);
		DP_BANDWIDTH(
			isp_curr_pmqos->isp_bandwidth,
			isp_curr_pmqos->isp_total_pixel,
			max_throughput,
			curr_bandwidth);
	}

	CMDQ_MSG("[MDP]ISP only curr_bandwidth %d, max_throughput %d\n", curr_bandwidth, max_throughput);
	/*update bandwidth*/
	pm_qos_update_request(&isp_bw_qos_request[thread_id], curr_bandwidth);
	/*update clock*/
	pm_qos_update_request(&isp_clk_qos_request[thread_id], max_throughput);
#endif
}

static void cmdq_mdp_begin_task_virtual(struct TaskStruct *cmdq_task, struct TaskStruct *task_list[], u32 size)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	struct mdp_pmqos *mdp_curr_pmqos;
	struct mdp_pmqos *mdp_list_pmqos;
	struct mdp_pmqos_record *pmqos_curr_record;
	struct mdp_pmqos_record *pmqos_list_record;
	s32 i = 0;
	struct timeval curr_time;
	int32_t numerator;
	int32_t denominator;
	uint32_t thread_id = cmdq_task->thread - MDP_THREAD_START;
	uint32_t max_throughput = 0;
	uint32_t isp_curr_bandwidth = 0;
	uint32_t isp_data_size = 0;
	uint32_t isp_curr_pixel_size = 0;
	uint32_t mdp_curr_bandwidth = 0;
	uint32_t mdp_data_size = 0;
	uint32_t mdp_curr_pixel_size = 0;
	bool first_task = true;
	/* For MET */
	uint32_t *addr1 = NULL;
	uint32_t *addr2 = NULL;

	pmqos_curr_record = kzalloc(sizeof(struct mdp_pmqos_record), GFP_KERNEL);
	cmdq_task->user_private = pmqos_curr_record;

	do_gettimeofday(&curr_time);

	CMDQ_MSG("enter %s with task:0x%p engine:0x%llx\n", __func__, cmdq_task, cmdq_task->engineFlag);

	if (!cmdq_task->prop_addr)
		return;

	mdp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
	pmqos_curr_record->submit_tm = curr_time;
	pmqos_curr_record->end_tm.tv_sec = mdp_curr_pmqos->tv_sec;
	pmqos_curr_record->end_tm.tv_usec = mdp_curr_pmqos->tv_usec;

	CMDQ_MSG(
		"[MDP]mdp %d pixel, mdp %d byte, isp %d pixel, isp %d byte, submit %06ld us, end %06ld us\n",
		mdp_curr_pmqos->mdp_total_pixel, mdp_curr_pmqos->mdp_bandwidth, mdp_curr_pmqos->isp_total_pixel,
		mdp_curr_pmqos->isp_bandwidth,
		pmqos_curr_record->submit_tm.tv_usec, pmqos_curr_record->end_tm.tv_usec);

	if (size > 1) {/*task_list includes the current task*/
		CMDQ_MSG("size %d thread_id = %d\n", size, thread_id);
		for (i = 0; i < size; i++) {
			struct TaskStruct *curTask = task_list[i];

			mdp_list_pmqos = (struct mdp_pmqos *)curTask->prop_addr;
			pmqos_list_record = (struct mdp_pmqos_record *)curTask->user_private;
			if (!pmqos_list_record)
				continue;

			if (first_task) {
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_list_record->submit_tm,
							pmqos_list_record->end_tm,
							denominator);
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_curr_record->submit_tm,
							pmqos_list_record->end_tm,
							numerator);
				pmqos_list_record->mdp_throughput = (mdp_list_pmqos->mdp_total_pixel / numerator) -
								(mdp_list_pmqos->mdp_total_pixel / denominator);
				max_throughput = pmqos_list_record->mdp_throughput;
				mdp_data_size = mdp_list_pmqos->mdp_bandwidth;
				isp_data_size = mdp_list_pmqos->isp_bandwidth;
				mdp_curr_pixel_size = mdp_list_pmqos->mdp_total_pixel;
				isp_curr_pixel_size = mdp_list_pmqos->isp_total_pixel;
				first_task = false;
			} else {
				struct TaskStruct *prevTask = task_list[i - 1];
				struct mdp_pmqos *mdp_prev_pmqos;
				struct mdp_pmqos_record *mdp_prev_record;

				mdp_prev_pmqos = (struct mdp_pmqos *)prevTask->prop_addr;
				mdp_prev_record = (struct mdp_pmqos_record *)prevTask->user_private;
				if (!mdp_prev_record)
					continue;
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_curr_record->submit_tm,
							pmqos_list_record->end_tm,
							denominator);
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_curr_record->submit_tm,
							mdp_prev_record->end_tm,
							numerator);

				pmqos_list_record->mdp_throughput = (mdp_prev_record->mdp_throughput *
								numerator / denominator) +
								(mdp_list_pmqos->mdp_total_pixel / denominator);
				if (pmqos_list_record->mdp_throughput > max_throughput)
					max_throughput = pmqos_list_record->mdp_throughput;
			}
			CMDQ_MSG(
				"[MDP]list mdp %d pixel, mdp %d byte, isp %d pixel, isp %d byte, submit %06ld us, end %06ld us\n",
				mdp_curr_pixel_size,
				mdp_data_size,
				isp_curr_pixel_size, isp_data_size,
				pmqos_list_record->submit_tm.tv_usec,
				pmqos_list_record->end_tm.tv_usec);

		}

		if (max_throughput > g_freq_steps[0]) {
			DP_BANDWIDTH(
				mdp_data_size,
				mdp_curr_pixel_size,
				g_freq_steps[0],
				mdp_curr_bandwidth);
			DP_BANDWIDTH(
				isp_data_size,
				isp_curr_pixel_size,
				g_freq_steps[0],
				isp_curr_bandwidth);
		} else {
			DP_BANDWIDTH(
				mdp_data_size,
				mdp_curr_pixel_size,
				max_throughput,
				mdp_curr_bandwidth);
			DP_BANDWIDTH(
				isp_data_size,
				isp_curr_pixel_size,
				max_throughput,
				isp_curr_bandwidth);
		}
	} else {
		DP_TIMER_GET_DURATION_IN_US(
					pmqos_curr_record->submit_tm,
					pmqos_curr_record->end_tm,
					denominator);
		pmqos_curr_record->mdp_throughput = mdp_curr_pmqos->mdp_total_pixel / denominator;

		if (pmqos_curr_record->mdp_throughput > g_freq_steps[0]) {
			DP_BANDWIDTH(
				mdp_curr_pmqos->mdp_bandwidth,
				mdp_curr_pmqos->mdp_total_pixel,
				g_freq_steps[0],
				mdp_curr_bandwidth);
			DP_BANDWIDTH(
				mdp_curr_pmqos->isp_bandwidth,
				mdp_curr_pmqos->isp_total_pixel,
				g_freq_steps[0],
				isp_curr_bandwidth);
		} else {
			DP_BANDWIDTH(
				mdp_curr_pmqos->mdp_bandwidth,
				mdp_curr_pmqos->mdp_total_pixel,
				pmqos_curr_record->mdp_throughput,
				mdp_curr_bandwidth);
			DP_BANDWIDTH(
				mdp_curr_pmqos->isp_bandwidth,
				mdp_curr_pmqos->isp_total_pixel,
				pmqos_curr_record->mdp_throughput,
				isp_curr_bandwidth);
		}
		max_throughput = pmqos_curr_record->mdp_throughput;
	}

	CMDQ_MSG(
		"[MDP]mdp_curr_bandwidth %d, isp_curr_bandwidth %d, max_throughput %d\n",
		mdp_curr_bandwidth,
		isp_curr_bandwidth,
		max_throughput);

	if (cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN) ||
		cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN2)) {
		/*update bandwidth*/
		pm_qos_update_request(&isp_bw_qos_request[thread_id], isp_curr_bandwidth);
		/*update clock*/
		pm_qos_update_request(&isp_clk_qos_request[thread_id], max_throughput);
	}

	/*update bandwidth*/
	if (mdp_curr_pmqos->mdp_bandwidth)
		pm_qos_update_request(&mdp_bw_qos_request[thread_id], mdp_curr_bandwidth);

	/*update clock*/
	if (mdp_curr_pmqos->mdp_total_pixel)
		pm_qos_update_request(&mdp_clk_qos_request[thread_id], max_throughput);

	/* For MET */

	if (!cmdq_task->prop_addr)
		return;
	mdp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
	do {
		if (mdp_curr_pmqos->ispMetStringSize <= 0)
			break;
		addr1 = kcalloc(mdp_curr_pmqos->ispMetStringSize,
			sizeof(char), GFP_KERNEL);

		if (!addr1) {
			CMDQ_MSG("[MDP] fail to alloc isp buf %d\n",
				mdp_curr_pmqos->ispMetStringSize);
			mdp_curr_pmqos->ispMetStringSize = 0;
			break;
		}
		if (copy_from_user
			(addr1, CMDQ_U32_PTR(mdp_curr_pmqos->ispMetString),
			mdp_curr_pmqos->ispMetStringSize * sizeof(char))) {
			mdp_curr_pmqos->ispMetStringSize = 0;
			CMDQ_MSG("[MDP] fail to copy user isp log\n");
			kfree(addr1);
			break;
		}
		mdp_curr_pmqos->ispMetString = (unsigned long)addr1;
	} while (0);

	do {
		if (mdp_curr_pmqos->mdpMetStringSize <= 0)
			break;
		addr2 = kcalloc(mdp_curr_pmqos->mdpMetStringSize,
			sizeof(char), GFP_KERNEL);
		if (!addr2) {
			mdp_curr_pmqos->mdpMetStringSize = 0;
			CMDQ_MSG("[MDP] fail to alloc mdp buf\n");
			break;
		}
		if (copy_from_user
			(addr2, CMDQ_U32_PTR(mdp_curr_pmqos->mdpMetString),
			mdp_curr_pmqos->mdpMetStringSize  * sizeof(char))) {
			mdp_curr_pmqos->mdpMetStringSize = 0;
			CMDQ_MSG("[MDP] fail to copy user mdp log\n");
			kfree(addr2);
			break;
		}
		mdp_curr_pmqos->mdpMetString = (unsigned long)addr2;
	} while (0);
#endif
#if IS_ENABLED(CONFIG_MTK_SMI_EXT) && IS_ENABLED(CONFIG_MACH_MT6771)
	smi_larb_mon_act_cnt();
#endif
}

static void cmdq_mdp_isp_end_task_virtual(struct TaskStruct *cmdq_task, struct TaskStruct *task_list[], u32 size)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	struct mdp_pmqos *isp_curr_pmqos;
	uint32_t thread_id = cmdq_task->thread - MDP_THREAD_START;

	if (!(cmdq_task->engineFlag & (1LL << CMDQ_ENG_ISP_IMGI) &&
		cmdq_task->engineFlag & (1LL << CMDQ_ENG_ISP_IMG2O))) {
		return;
	}

	if (!cmdq_task->prop_addr)
		return;

	CMDQ_MSG("enter %s with task:0x%p engine:0x%llx\n", __func__, cmdq_task, cmdq_task->engineFlag);
	isp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
	kfree(cmdq_task->user_private);
	cmdq_task->user_private = NULL;
	/*update bandwidth*/
	if (isp_curr_pmqos->isp_bandwidth)
		pm_qos_update_request(&isp_bw_qos_request[thread_id], 0);
	/*update clock*/
	if (isp_curr_pmqos->isp_total_pixel)
		pm_qos_update_request(&isp_clk_qos_request[thread_id], 0);
#endif
}

static void cmdq_mdp_end_task_virtual(struct TaskStruct *cmdq_task, struct TaskStruct *task_list[], u32 size)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	struct mdp_pmqos *mdp_curr_pmqos;
	struct mdp_pmqos *mdp_list_pmqos;
	struct mdp_pmqos_record *pmqos_curr_record;
	struct mdp_pmqos_record *pmqos_list_record;
	s32 i = 0;
	struct timeval curr_time;
	int32_t denominator;
	uint32_t thread_id = cmdq_task->thread - MDP_THREAD_START;
	uint32_t max_throughput = 0;
	uint32_t pre_throughput = 0;
	bool trigger = false;
	bool first_task = true;
	int32_t overdue;
	uint32_t isp_curr_bandwidth = 0;
	uint32_t isp_data_size = 0;
	uint32_t isp_curr_pixel_size = 0;
	uint32_t mdp_curr_bandwidth = 0;
	uint32_t mdp_data_size = 0;
	uint32_t mdp_curr_pixel_size = 0;
	bool update_isp_throughput = false;
	bool update_isp_bandwidth = false;

#if IS_ENABLED(CONFIG_MTK_SMI_EXT) && IS_ENABLED(CONFIG_MACH_MT6771)
	smi_larb_mon_act_cnt();
#endif
	do_gettimeofday(&curr_time);
	CMDQ_MSG("enter %s with task:0x%p engine:0x%llx\n", __func__, cmdq_task, cmdq_task->engineFlag);

	if (!cmdq_task->prop_addr || !cmdq_task->user_private)
		return;

	mdp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
	pmqos_curr_record = (struct mdp_pmqos_record *)cmdq_task->user_private;
	pmqos_curr_record->submit_tm = curr_time;

	for (i = 0; i < size; i++) {
		struct TaskStruct *curTask = task_list[i];

		mdp_list_pmqos = (struct mdp_pmqos *)curTask->prop_addr;
		pmqos_list_record = (struct mdp_pmqos_record *)curTask->user_private;
		if (!pmqos_list_record)
			continue;

		if (cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN) ||
			cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN2))
			update_isp_throughput = true;

		if (first_task) {
			mdp_curr_pixel_size = mdp_list_pmqos->mdp_total_pixel;
			isp_curr_pixel_size = mdp_list_pmqos->isp_total_pixel;
			mdp_data_size   = mdp_list_pmqos->mdp_bandwidth;
			isp_data_size   = mdp_list_pmqos->isp_bandwidth;
			if (cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN) ||
				cmdq_task->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN2))
				update_isp_bandwidth = true;

			first_task = false;
		}

		if (pmqos_curr_record->mdp_throughput < pmqos_list_record->mdp_throughput) {
			if (max_throughput < pmqos_list_record->mdp_throughput) {
				max_throughput = pmqos_list_record->mdp_throughput;

				if (max_throughput > g_freq_steps[0]) {
					DP_BANDWIDTH(
						mdp_data_size,
						mdp_curr_pixel_size,
						g_freq_steps[0],
						mdp_curr_bandwidth);
					DP_BANDWIDTH(
						isp_data_size,
						isp_curr_pixel_size,
						g_freq_steps[0],
						isp_curr_bandwidth);
				} else {
					DP_BANDWIDTH(
						mdp_data_size,
						mdp_curr_pixel_size,
						max_throughput,
						mdp_curr_bandwidth);
					DP_BANDWIDTH(
						isp_data_size,
						isp_curr_pixel_size,
						max_throughput,
						isp_curr_bandwidth);
				}
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_curr_record->submit_tm,
							pmqos_list_record->end_tm,
							overdue);
				if (overdue == 1) {
					trigger = true;
					break;
				}
			}
			continue;
		}
		trigger = true;
		break;
	}
	first_task = true;
	if (size > 0 && trigger) {/*task_list excludes the current task*/
		CMDQ_MSG("[MDP] curr submit %06ld us, end %06ld us\n",
			pmqos_curr_record->submit_tm.tv_usec,
			pmqos_curr_record->end_tm.tv_usec);
		for (i = 0; i < size; i++) {
			struct TaskStruct *curTask = task_list[i];

			mdp_list_pmqos = (struct mdp_pmqos *)curTask->prop_addr;
			pmqos_list_record = (struct mdp_pmqos_record *)curTask->user_private;
			if (!pmqos_list_record)
				continue;
			if (curTask->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN) ||
				curTask->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN2))
				update_isp_throughput = true; /*if there is any DL task in list*/

			if (first_task) {
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_list_record->submit_tm,
							pmqos_list_record->end_tm,
							denominator);
				pmqos_list_record->mdp_throughput = mdp_list_pmqos->mdp_total_pixel / denominator;
				max_throughput = pmqos_list_record->mdp_throughput;
				mdp_data_size = mdp_list_pmqos->mdp_bandwidth;
				isp_data_size = mdp_list_pmqos->isp_bandwidth;
				mdp_curr_pixel_size = mdp_list_pmqos->mdp_total_pixel;
				isp_curr_pixel_size = mdp_list_pmqos->isp_total_pixel;

				if (curTask->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN) ||
					curTask->engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN2))
					update_isp_bandwidth = true; /*next task is diect link*/

				first_task = false;
				pre_throughput = max_throughput;
			} else {
				if (!pre_throughput)
					continue;
				DP_TIMER_GET_DURATION_IN_US(
							pmqos_curr_record->submit_tm,
							pmqos_list_record->end_tm,
							denominator);
				pmqos_list_record->mdp_throughput = pre_throughput +
								(mdp_list_pmqos->mdp_total_pixel / denominator);
				pre_throughput = pmqos_list_record->mdp_throughput;
				if (pmqos_list_record->mdp_throughput > max_throughput)
					max_throughput = pmqos_list_record->mdp_throughput;
			}
			CMDQ_MSG(
				"[MDP]list mdp %d MHz, mdp %d pixel, mdp %d byte, mdp %d pixel, isp %d byte, submit %06ld us, end %06ld us\n",
				pmqos_list_record->mdp_throughput, mdp_list_pmqos->mdp_total_pixel,
				mdp_list_pmqos->mdp_bandwidth, mdp_list_pmqos->isp_total_pixel,
				mdp_list_pmqos->isp_bandwidth,
				pmqos_list_record->submit_tm.tv_usec, pmqos_list_record->end_tm.tv_usec);
		}

		if (max_throughput > g_freq_steps[0]) {
			DP_BANDWIDTH(mdp_data_size, mdp_curr_pixel_size, g_freq_steps[0], mdp_curr_bandwidth);
			DP_BANDWIDTH(isp_data_size, isp_curr_pixel_size, g_freq_steps[0], isp_curr_bandwidth);
		} else {
			DP_BANDWIDTH(mdp_data_size, mdp_curr_pixel_size, max_throughput, mdp_curr_bandwidth);
			DP_BANDWIDTH(isp_data_size, isp_curr_pixel_size, max_throughput, isp_curr_bandwidth);
		}
	}
	CMDQ_MSG(
		"[MDP]mdp_curr_bandwidth %d, isp_curr_bandwidth %d, max_throughput %d\n",
		mdp_curr_bandwidth, isp_curr_bandwidth, max_throughput);

	kfree(cmdq_task->user_private);
	cmdq_task->user_private = NULL;

	if (update_isp_throughput) {
		/*update clock*/
		pm_qos_update_request(&isp_clk_qos_request[thread_id], max_throughput);
	} else {
		/*update clock*/
		if (mdp_curr_pmqos->isp_total_pixel)
			pm_qos_update_request(&isp_clk_qos_request[thread_id], 0);
	}
	if (update_isp_bandwidth) {
		/*update bandwidth*/
		pm_qos_update_request(&isp_bw_qos_request[thread_id], isp_curr_bandwidth);
	} else {
		/*update bandwidth*/
		if (mdp_curr_pmqos->isp_bandwidth)
			pm_qos_update_request(&isp_bw_qos_request[thread_id], 0);
	}
	if (mdp_curr_pmqos->mdp_bandwidth)
		pm_qos_update_request(&mdp_bw_qos_request[thread_id], mdp_curr_bandwidth);

	/*update clock*/
	if (mdp_curr_pmqos->mdp_total_pixel)
		pm_qos_update_request(&mdp_clk_qos_request[thread_id], max_throughput);

	/*For MET*/
	if (cmdq_task->prop_addr) {
		mdp_curr_pmqos = (struct mdp_pmqos *)cmdq_task->prop_addr;
		if (mdp_curr_pmqos->ispMetStringSize > 0) {
			kfree(CMDQ_U32_PTR(mdp_curr_pmqos->ispMetString));
			mdp_curr_pmqos->ispMetString = 0;
			mdp_curr_pmqos->ispMetStringSize = 0;
		}
		if (mdp_curr_pmqos->mdpMetStringSize > 0) {
			kfree(CMDQ_U32_PTR(mdp_curr_pmqos->mdpMetString));
			mdp_curr_pmqos->mdpMetString = 0;
			mdp_curr_pmqos->mdpMetStringSize = 0;
		}
	}
#endif
}

void cmdq_mdp_start_task_atomic_virtual(const struct TaskStruct *task, u32 instr_size)
{
}

void cmdq_mdp_finish_task_atomic_virtual(const struct TaskStruct *task, u32 instr_size)
{
}
/**************************************************************************************/
/************************                      Common Code                      ************************/
/**************************************************************************************/
static struct cmdqMDPFuncStruct gMDPFunctionPointer;

void cmdq_mdp_map_mmsys_VA(void)
{
	g_cmdq_mmsys_base = cmdq_dev_alloc_reference_VA_by_name("mmsys_config");
}

void cmdq_mdp_unmap_mmsys_VA(void)
{
	cmdq_dev_free_module_base_VA(g_cmdq_mmsys_base);
}

void mdp_init(void)
{
	cmdq_mdp_get_func()->initModuleBaseVA();
}

void mdp_deinit(void)
{
	cmdq_mdp_get_func()->deinitModuleBaseVA();
}

void cmdq_mdp_virtual_function_setting(void)
{
	struct cmdqMDPFuncStruct *pFunc;

	pFunc = &(gMDPFunctionPointer);
	pFunc->mdp_probe = cmdq_mdp_probe_virtual;
	pFunc->dumpMMSYSConfig = cmdq_mdp_dump_mmsys_config_virtual;

	pFunc->vEncDumpInfo = cmdqVEncDumpInfo_virtual;

	pFunc->initModuleBaseVA = cmdq_mdp_init_module_base_VA_virtual;
	pFunc->deinitModuleBaseVA = cmdq_mdp_deinit_module_base_VA_virtual;

	pFunc->mdpClockIsOn = cmdq_mdp_clock_is_on_virtual;
	pFunc->enableMdpClock = cmdq_mdp_enable_clock_virtual;
	pFunc->initModuleCLK = cmdq_mdp_init_module_clk_virtual;

	pFunc->mdpDumpRsz = cmdq_mdp_dump_rsz_virtual;
	pFunc->mdpDumpTdshp = cmdq_mdp_dump_tdshp_virtual;

	pFunc->mdpClockOn = cmdqMdpClockOn_virtual;
	pFunc->mdpDumpInfo = cmdqMdpDumpInfo_virtual;
	pFunc->mdpResetEng = cmdqMdpResetEng_virtual;
	pFunc->mdpClockOff = cmdqMdpClockOff_virtual;

	pFunc->mdpInitialSet = cmdqMdpInitialSetting_virtual;

	pFunc->rdmaGetRegOffsetSrcAddr = cmdq_mdp_rdma_get_reg_offset_src_addr_virtual;
	pFunc->wrotGetRegOffsetDstAddr = cmdq_mdp_wrot_get_reg_offset_dst_addr_virtual;
	pFunc->wdmaGetRegOffsetDstAddr = cmdq_mdp_wdma_get_reg_offset_dst_addr_virtual;
	pFunc->testcaseClkmgrMdp = testcase_clkmgr_mdp_virtual;

	pFunc->dispatchModule = cmdq_mdp_dispatch_virtual;

	pFunc->trackTask = cmdq_mdp_trackTask_virtual;
	pFunc->parseErrModByEngFlag = cmdq_mdp_parse_error_module_by_hwflag_virtual;
	pFunc->getEngineGroupBits = cmdq_mdp_get_engine_group_bits_virtual;
	pFunc->errorReset = cmdq_mdp_error_reset_virtual;
	pFunc->mdpEnableCommonClock = cmdq_mdp_enable_common_clock_virtual;
	pFunc->meansureBandwidth = cmdq_mdp_meansure_bandwidth_virtual;
	pFunc->beginTask = cmdq_mdp_begin_task_virtual;
	pFunc->endTask = cmdq_mdp_end_task_virtual;
	pFunc->beginISPTask = cmdq_mdp_isp_begin_task_virtual;
	pFunc->endISPTask = cmdq_mdp_isp_end_task_virtual;
	pFunc->startTask_atomic = cmdq_mdp_start_task_atomic_virtual;
	pFunc->finishTask_atomic = cmdq_mdp_finish_task_atomic_virtual;
}

struct cmdqMDPFuncStruct *cmdq_mdp_get_func(void)
{
	return &gMDPFunctionPointer;
}

void cmdq_mdp_enable(uint64_t engineFlag, enum CMDQ_ENG_ENUM engine)
{
#ifdef CMDQ_PWR_AWARE
	CMDQ_VERBOSE("Test for ENG %d\n", engine);
	if (engineFlag & (1LL << engine))
		cmdq_mdp_get_func()->enableMdpClock(true, engine);
#endif
}

int cmdq_mdp_loop_reset_impl(const unsigned long resetReg,
			     const uint32_t resetWriteValue,
			     const unsigned long resetStateReg,
			     const uint32_t resetMask,
			     const uint32_t resetPollingValue, const int32_t maxLoopCount)
{
	u32 poll_value = 0;
	s32 ret;

	CMDQ_REG_SET32(resetReg, resetWriteValue);

	/* polling with 10ms timeout */
	ret = readl_poll_timeout_atomic((void *)resetStateReg, poll_value,
		(poll_value & resetMask) == resetPollingValue, 0, 10000);

	/* return polling result */
	if (ret == -ETIMEDOUT) {
		CMDQ_ERR(
			"%s failed Reg:0x%lx writeValue:0x%08x stateReg:0x%lx mask:0x%08x pollingValue:0x%08x\n",
			__func__, resetReg, resetWriteValue, resetStateReg,
			resetMask, resetPollingValue);
		return -EFAULT;
	}

	return 0;
}

int cmdq_mdp_loop_reset(enum CMDQ_ENG_ENUM engine,
			const unsigned long resetReg,
			const unsigned long resetStateReg,
			const uint32_t resetMask,
			const uint32_t resetValue, const bool pollInitResult)
{
#ifdef CMDQ_PWR_AWARE
	int resetStatus = 0;
	int initStatus = 0;

	if (cmdq_mdp_get_func()->mdpClockIsOn(engine)) {
		CMDQ_PROF_START(current->pid, __func__);
		CMDQ_PROF_MMP(cmdq_mmp_get_event()->MDP_reset,
			      MMPROFILE_FLAG_START, resetReg, resetStateReg);


		/* loop reset */
		resetStatus = cmdq_mdp_loop_reset_impl(resetReg, 0x1,
						       resetStateReg, resetMask, resetValue,
						       CMDQ_MAX_LOOP_COUNT);

		if (pollInitResult) {
			/* loop  init */
			initStatus = cmdq_mdp_loop_reset_impl(resetReg, 0x0,
							      resetStateReg, resetMask, 0x0,
							      CMDQ_MAX_LOOP_COUNT);
		} else {
			/* always clear to init state no matter what polling result */
			CMDQ_REG_SET32(resetReg, 0x0);
		}

		CMDQ_PROF_MMP(cmdq_mmp_get_event()->MDP_reset,
			      MMPROFILE_FLAG_END, resetReg, resetStateReg);
		CMDQ_PROF_END(current->pid, __func__);

		/* retrun failed if loop failed */
		if ((resetStatus < 0) || (initStatus < 0)) {
			CMDQ_ERR("Reset MDP %d failed, resetStatus:%d, initStatus:%d\n",
				 engine, resetStatus, initStatus);
			return -EFAULT;
		}
	}
#endif

	return 0;
};

void cmdq_mdp_loop_off(enum CMDQ_ENG_ENUM engine,
		       const unsigned long resetReg,
		       const unsigned long resetStateReg,
		       const uint32_t resetMask,
		       const uint32_t resetValue, const bool pollInitResult)
{
#ifdef CMDQ_PWR_AWARE
	int resetStatus = 0;
	int initStatus = 0;

	if (cmdq_mdp_get_func()->mdpClockIsOn(engine)) {

		/* loop reset */
		resetStatus = cmdq_mdp_loop_reset_impl(resetReg, 0x1,
						       resetStateReg, resetMask, resetValue,
						       CMDQ_MAX_LOOP_COUNT);

		if (pollInitResult) {
			/* loop init */
			initStatus = cmdq_mdp_loop_reset_impl(resetReg, 0x0,
							      resetStateReg, resetMask, 0x0,
							      CMDQ_MAX_LOOP_COUNT);
		} else {
			/* always clear to init state no matter what polling result */
			CMDQ_REG_SET32(resetReg, 0x0);
		}

		/* retrun failed if loop failed */
		if ((resetStatus < 0) || (initStatus < 0)) {
			CMDQ_AEE("MDP",
				 "Disable %ld engine failed, resetStatus:%d, initStatus:%d\n",
				 resetReg, resetStatus, initStatus);
		}

		cmdq_mdp_get_func()->enableMdpClock(false, engine);
	}
#endif
}

void cmdq_mdp_dump_venc(const unsigned long base, const char *label)
{
	if (base == 0L) {
		/* print error message */
		CMDQ_ERR("venc base VA [0x%lx] is not correct\n", base);
		return;
	}

	CMDQ_ERR("======== cmdq_mdp_dump_venc + ========\n");
	CMDQ_ERR("[0x%lx] to [0x%lx]\n", base, base + 0x1000 * 4);

	print_hex_dump(KERN_ERR, "[CMDQ][ERR][VENC]", DUMP_PREFIX_ADDRESS, 16, 4,
		       (void *)base, 0x1000, false);
	CMDQ_ERR("======== cmdq_mdp_dump_venc - ========\n");
}

const char *cmdq_mdp_get_rdma_state(uint32_t state)
{
	switch (state) {
	case 0x1:
		return "idle";
	case 0x2:
		return "wait sof";
	case 0x4:
		return "reg update";
	case 0x8:
		return "clear0";
	case 0x10:
		return "clear1";
	case 0x20:
		return "int0";
	case 0x40:
		return "int1";
	case 0x80:
		return "data running";
	case 0x100:
		return "wait done";
	case 0x200:
		return "warm reset";
	case 0x400:
		return "wait reset";
	default:
		return "";
	}
}

void cmdq_mdp_dump_rdma(const unsigned long base, const char *label)
{
	uint32_t value[17] = { 0 };
	uint32_t state = 0;
	uint32_t grep = 0;

	value[0] = CMDQ_REG_GET32(base + 0x030);
	value[1] = CMDQ_REG_GET32(base + cmdq_mdp_get_func()->rdmaGetRegOffsetSrcAddr());
	value[2] = CMDQ_REG_GET32(base + 0x060);
	value[3] = CMDQ_REG_GET32(base + 0x070);
	value[4] = CMDQ_REG_GET32(base + 0x078);
	value[5] = CMDQ_REG_GET32(base + 0x080);
	value[6] = CMDQ_REG_GET32(base + 0x100);
	value[7] = CMDQ_REG_GET32(base + 0x118);
	value[8] = CMDQ_REG_GET32(base + 0x130);
	value[9] = CMDQ_REG_GET32(base + 0x400);
	value[10] = CMDQ_REG_GET32(base + 0x408);
	value[11] = CMDQ_REG_GET32(base + 0x410);
	value[12] = CMDQ_REG_GET32(base + 0x420);
	value[13] = CMDQ_REG_GET32(base + 0x430);
	value[14] = CMDQ_REG_GET32(base + 0x440);
	value[15] = CMDQ_REG_GET32(base + 0x4D0);
	value[16] = CMDQ_REG_GET32(base + 0x0);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR
	    ("RDMA_SRC_CON: 0x%08x, RDMA_SRC_BASE_0: 0x%08x, RDMA_MF_BKGD_SIZE_IN_BYTE: 0x%08x\n",
	     value[0], value[1], value[2]);
	CMDQ_ERR("RDMA_MF_SRC_SIZE: 0x%08x, RDMA_MF_CLIP_SIZE: 0x%08x, RDMA_MF_OFFSET_1: 0x%08x\n",
		 value[3], value[4], value[5]);
	CMDQ_ERR("RDMA_SRC_END_0: 0x%08x, RDMA_SRC_OFFSET_0: 0x%08x, RDMA_SRC_OFFSET_W_0: 0x%08x\n",
		 value[6], value[7], value[8]);
	CMDQ_ERR("RDMA_MON_STA_0: 0x%08x, RDMA_MON_STA_1: 0x%08x, RDMA_MON_STA_2: 0x%08x\n",
		 value[9], value[10], value[11]);
	CMDQ_ERR("RDMA_MON_STA_4: 0x%08x, RDMA_MON_STA_6: 0x%08x, RDMA_MON_STA_8: 0x%08x\n",
		 value[12], value[13], value[14]);
	CMDQ_ERR("RDMA_MON_STA_26: 0x%08x\n",
		 value[15]);
	CMDQ_ERR("RDMA_EN: 0x%08x\n",
		 value[16]);

	/* parse state */
	CMDQ_ERR("RDMA ack:%d req:%d\n", (value[9] & (1 << 11)) >> 11,
		 (value[9] & (1 << 10)) >> 10);
	state = (value[10] >> 8) & 0x7FF;
	grep = (value[10] >> 20) & 0x1;
	CMDQ_ERR("RDMA state: 0x%x (%s)\n", state, cmdq_mdp_get_rdma_state(state));
	CMDQ_ERR("RDMA horz_cnt: %d vert_cnt:%d\n", value[15] & 0xFFF, (value[15] >> 16) & 0xFFF);

	CMDQ_ERR("RDMA grep:%d => suggest to ask SMI help:%d\n", grep, grep);
}

const char *cmdq_mdp_get_rsz_state(const uint32_t state)
{
	switch (state) {
	case 0x5:
		return "downstream hang";	/* 0,1,0,1 */
	case 0xa:
		return "upstream hang";	/* 1,0,1,0 */
	default:
		return "";
	}
}

void cmdq_mdp_dump_rot(const unsigned long base, const char *label)
{
	uint32_t value[47] = { 0 };

	value[0] = CMDQ_REG_GET32(base + 0x000);
	value[1] = CMDQ_REG_GET32(base + 0x008);
	value[2] = CMDQ_REG_GET32(base + 0x00C);
	value[3] = CMDQ_REG_GET32(base + 0x024);
	value[4] = CMDQ_REG_GET32(base + cmdq_mdp_get_func()->wrotGetRegOffsetDstAddr());
	value[5] = CMDQ_REG_GET32(base + 0x02C);
	value[6] = CMDQ_REG_GET32(base + 0x004);
	value[7] = CMDQ_REG_GET32(base + 0x030);
	value[8] = CMDQ_REG_GET32(base + 0x078);
	value[9] = CMDQ_REG_GET32(base + 0x070);
	CMDQ_REG_SET32(base + 0x018, 0x00000100);
	value[10] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000200);
	value[11] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000300);
	value[12] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000400);
	value[13] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000500);
	value[14] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000600);
	value[15] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000700);
	value[16] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000800);
	value[17] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000900);
	value[18] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000A00);
	value[19] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000B00);
	value[20] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000C00);
	value[21] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000D00);
	value[22] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000E00);
	value[23] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00000F00);
	value[24] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001000);
	value[25] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001100);
	value[26] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001200);
	value[27] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001300);
	value[28] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001400);
	value[29] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001500);
	value[30] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001600);
	value[31] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001700);
	value[32] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001800);
	value[33] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001900);
	value[34] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001A00);
	value[35] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001B00);
	value[36] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001C00);
	value[37] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001D00);
	value[38] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001E00);
	value[39] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00001F00);
	value[40] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00002000);
	value[41] = CMDQ_REG_GET32(base + 0x0D0);
	CMDQ_REG_SET32(base + 0x018, 0x00002100);
	value[42] = CMDQ_REG_GET32(base + 0x0D0);
	value[43] = CMDQ_REG_GET32(base + 0x01C);
	value[44] = CMDQ_REG_GET32(base + 0x07C);
	value[45] = CMDQ_REG_GET32(base + 0x010);
	value[46] = CMDQ_REG_GET32(base + 0x014);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR("ROT_CTRL: 0x%08x, ROT_MAIN_BUF_SIZE: 0x%08x, ROT_SUB_BUF_SIZE: 0x%08x\n",
		 value[0], value[1], value[2]);
	CMDQ_ERR("ROT_TAR_SIZE: 0x%08x, ROT_BASE_ADDR: 0x%08x, ROT_OFST_ADDR: 0x%08x\n",
		 value[3], value[4], value[5]);
	CMDQ_ERR("ROT_DMA_PERF: 0x%08x, ROT_STRIDE: 0x%08x, ROT_IN_SIZE: 0x%08x\n",
		 value[6], value[7], value[8]);
	CMDQ_ERR("ROT_EOL: 0x%08x, ROT_DBUGG_1: 0x%08x, ROT_DEBUBG_2: 0x%08x\n",
		 value[9], value[10], value[11]);
	CMDQ_ERR("ROT_DBUGG_3: 0x%08x, ROT_DBUGG_4: 0x%08x, ROT_DEBUBG_5: 0x%08x\n",
		 value[12], value[13], value[14]);
	CMDQ_ERR("ROT_DBUGG_6: 0x%08x, ROT_DBUGG_7: 0x%08x, ROT_DEBUBG_8: 0x%08x\n",
		 value[15], value[16], value[17]);
	CMDQ_ERR("ROT_DBUGG_9: 0x%08x, ROT_DBUGG_A: 0x%08x, ROT_DEBUBG_B: 0x%08x\n",
		 value[18], value[19], value[20]);
	CMDQ_ERR("ROT_DBUGG_C: 0x%08x, ROT_DBUGG_D: 0x%08x, ROT_DEBUBG_E: 0x%08x\n",
		 value[21], value[22], value[23]);
	CMDQ_ERR("ROT_DBUGG_F: 0x%08x, ROT_DBUGG_10: 0x%08x, ROT_DEBUBG_11: 0x%08x\n",
		 value[24], value[25], value[26]);
	CMDQ_ERR("ROT_DEBUG_12: 0x%08x, ROT_DBUGG_13: 0x%08x, ROT_DBUGG_14: 0x%08x\n",
		 value[27], value[28], value[29]);
	CMDQ_ERR("ROT_DEBUG_15: 0x%08x, ROT_DBUGG_16: 0x%08x, ROT_DBUGG_17: 0x%08x\n",
		 value[30], value[31], value[32]);
	CMDQ_ERR("ROT_DEBUG_18: 0x%08x, ROT_DBUGG_19: 0x%08x, ROT_DBUGG_1A: 0x%08x\n",
		 value[33], value[34], value[35]);
	CMDQ_ERR("ROT_DEBUG_1B: 0x%08x, ROT_DBUGG_1C: 0x%08x, ROT_DBUGG_1D: 0x%08x\n",
		 value[36], value[37], value[38]);
	CMDQ_ERR("ROT_DEBUG_1E: 0x%08x, ROT_DBUGG_1F: 0x%08x, ROT_DBUGG_20: 0x%08x\n",
		 value[39], value[40], value[41]);
	CMDQ_ERR("ROT_DEBUG_21: 0x%08x\n",
		 value[42]);
	CMDQ_ERR("VIDO_INT: 0x%08x, VIDO_ROT_EN: 0x%08x\n", value[43], value[44]);
	CMDQ_ERR("VIDO_SOFT_RST: 0x%08x, VIDO_SOFT_RST_STAT: 0x%08x\n", value[45], value[46]);
}

void cmdq_mdp_dump_color(const unsigned long base, const char *label)
{
	uint32_t value[13] = { 0 };

	value[0] = CMDQ_REG_GET32(base + 0x400);
	value[1] = CMDQ_REG_GET32(base + 0x404);
	value[2] = CMDQ_REG_GET32(base + 0x408);
	value[3] = CMDQ_REG_GET32(base + 0x40C);
	value[4] = CMDQ_REG_GET32(base + 0x410);
	value[5] = CMDQ_REG_GET32(base + 0x420);
	value[6] = CMDQ_REG_GET32(base + 0xC00);
	value[7] = CMDQ_REG_GET32(base + 0xC04);
	value[8] = CMDQ_REG_GET32(base + 0xC08);
	value[9] = CMDQ_REG_GET32(base + 0xC0C);
	value[10] = CMDQ_REG_GET32(base + 0xC10);
	value[11] = CMDQ_REG_GET32(base + 0xC50);
	value[12] = CMDQ_REG_GET32(base + 0xC54);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR("COLOR CFG_MAIN: 0x%08x\n", value[0]);
	CMDQ_ERR("COLOR PXL_CNT_MAIN: 0x%08x, LINE_CNT_MAIN: 0x%08x\n", value[1], value[2]);
	CMDQ_ERR("COLOR WIN_X_MAIN: 0x%08x, WIN_Y_MAIN: 0x%08x, DBG_CFG_MAIN: 0x%08x\n", value[3], value[4],
		 value[5]);
	CMDQ_ERR("COLOR START: 0x%08x, INTEN: 0x%08x, INTSTA: 0x%08x\n", value[6], value[7],
		 value[8]);
	CMDQ_ERR("COLOR OUT_SEL: 0x%08x, FRAME_DONE_DEL: 0x%08x\n", value[9], value[10]);
	CMDQ_ERR("COLOR INTERNAL_IP_WIDTH: 0x%08x, INTERNAL_IP_HEIGHT: 0x%08x\n", value[11], value[12]);
}

const char *cmdq_mdp_get_wdma_state(uint32_t state)
{
	switch (state) {
	case 0x1:
		return "idle";
	case 0x2:
		return "clear";
	case 0x4:
		return "prepare";
	case 0x8:
		return "prepare";
	case 0x10:
		return "data running";
	case 0x20:
		return "eof wait";
	case 0x40:
		return "soft reset wait";
	case 0x80:
		return "eof done";
	case 0x100:
		return "sof reset done";
	case 0x200:
		return "frame complete";
	default:
		return "";
	}
}

void cmdq_mdp_dump_wdma(const unsigned long base, const char *label)
{
	uint32_t value[56] = { 0 };
	uint32_t state = 0;
	uint32_t grep = 0;	/* grep bit = 1, WDMA has sent request to SMI, and not receive done yet */
	uint32_t isFIFOFull = 0;	/* 1 for WDMA FIFO full */

	value[0] = CMDQ_REG_GET32(base + 0x014);
	value[1] = CMDQ_REG_GET32(base + 0x018);
	value[2] = CMDQ_REG_GET32(base + 0x028);
	value[3] = CMDQ_REG_GET32(base + cmdq_mdp_get_func()->wdmaGetRegOffsetDstAddr());
	value[4] = CMDQ_REG_GET32(base + 0x078);
	value[5] = CMDQ_REG_GET32(base + 0x080);
	value[6] = CMDQ_REG_GET32(base + 0x0A0);
	value[7] = CMDQ_REG_GET32(base + 0x0A8);

	CMDQ_REG_SET32(base + 0x014, (value[0] & (0x0FFFFFFF)));
	value[8] = CMDQ_REG_GET32(base + 0x014);
	value[9] = CMDQ_REG_GET32(base + 0x0AC);
	value[40] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x10000000 | (value[0] & (0x0FFFFFFF)));
	value[10] = CMDQ_REG_GET32(base + 0x014);
	value[11] = CMDQ_REG_GET32(base + 0x0AC);
	value[41] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x20000000 | (value[0] & (0x0FFFFFFF)));
	value[12] = CMDQ_REG_GET32(base + 0x014);
	value[13] = CMDQ_REG_GET32(base + 0x0AC);
	value[42] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x30000000 | (value[0] & (0x0FFFFFFF)));
	value[14] = CMDQ_REG_GET32(base + 0x014);
	value[15] = CMDQ_REG_GET32(base + 0x0AC);
	value[43] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x40000000 | (value[0] & (0x0FFFFFFF)));
	value[16] = CMDQ_REG_GET32(base + 0x014);
	value[17] = CMDQ_REG_GET32(base + 0x0AC);
	value[44] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x50000000 | (value[0] & (0x0FFFFFFF)));
	value[18] = CMDQ_REG_GET32(base + 0x014);
	value[19] = CMDQ_REG_GET32(base + 0x0AC);
	value[45] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x60000000 | (value[0] & (0x0FFFFFFF)));
	value[20] = CMDQ_REG_GET32(base + 0x014);
	value[21] = CMDQ_REG_GET32(base + 0x0AC);
	value[46] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x70000000 | (value[0] & (0x0FFFFFFF)));
	value[22] = CMDQ_REG_GET32(base + 0x014);
	value[23] = CMDQ_REG_GET32(base + 0x0AC);
	value[47] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x80000000 | (value[0] & (0x0FFFFFFF)));
	value[24] = CMDQ_REG_GET32(base + 0x014);
	value[25] = CMDQ_REG_GET32(base + 0x0AC);
	value[48] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0x90000000 | (value[0] & (0x0FFFFFFF)));
	value[26] = CMDQ_REG_GET32(base + 0x014);
	value[27] = CMDQ_REG_GET32(base + 0x0AC);
	value[49] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xA0000000 | (value[0] & (0x0FFFFFFF)));
	value[28] = CMDQ_REG_GET32(base + 0x014);
	value[29] = CMDQ_REG_GET32(base + 0x0AC);
	value[50] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xB0000000 | (value[0] & (0x0FFFFFFF)));
	value[30] = CMDQ_REG_GET32(base + 0x014);
	value[31] = CMDQ_REG_GET32(base + 0x0AC);
	value[51] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xC0000000 | (value[0] & (0x0FFFFFFF)));
	value[32] = CMDQ_REG_GET32(base + 0x014);
	value[33] = CMDQ_REG_GET32(base + 0x0AC);
	value[52] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xD0000000 | (value[0] & (0x0FFFFFFF)));
	value[34] = CMDQ_REG_GET32(base + 0x014);
	value[35] = CMDQ_REG_GET32(base + 0x0AC);
	value[53] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xE0000000 | (value[0] & (0x0FFFFFFF)));
	value[36] = CMDQ_REG_GET32(base + 0x014);
	value[37] = CMDQ_REG_GET32(base + 0x0AC);
	value[54] = CMDQ_REG_GET32(base + 0x0B8);
	CMDQ_REG_SET32(base + 0x014, 0xF0000000 | (value[0] & (0x0FFFFFFF)));
	value[38] = CMDQ_REG_GET32(base + 0x014);
	value[39] = CMDQ_REG_GET32(base + 0x0AC);
	value[55] = CMDQ_REG_GET32(base + 0x0B8);

	CMDQ_ERR("=============== [CMDQ] %s Status ====================================\n", label);
	CMDQ_ERR("[CMDQ]WDMA_CFG: 0x%08x, WDMA_SRC_SIZE: 0x%08x, WDMA_DST_W_IN_BYTE = 0x%08x\n",
		 value[0], value[1], value[2]);
	CMDQ_ERR
	    ("[CMDQ]WDMA_DST_ADDR0: 0x%08x, WDMA_DST_UV_PITCH: 0x%08x, WDMA_DST_ADDR_OFFSET0 = 0x%08x\n",
	     value[3], value[4], value[5]);
	CMDQ_ERR("[CMDQ]WDMA_STATUS: 0x%08x, WDMA_INPUT_CNT: 0x%08x\n", value[6], value[7]);

	/* Dump Addtional WDMA debug info */
	CMDQ_ERR("WDMA_DEBUG_0 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[8], value[9], value[40]);
	CMDQ_ERR("WDMA_DEBUG_1 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[10], value[11], value[41]);
	CMDQ_ERR("WDMA_DEBUG_2 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[12], value[13], value[42]);
	CMDQ_ERR("WDMA_DEBUG_3 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[14], value[15], value[43]);
	CMDQ_ERR("WDMA_DEBUG_4 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[16], value[17], value[44]);
	CMDQ_ERR("WDMA_DEBUG_5 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[18], value[19], value[45]);
	CMDQ_ERR("WDMA_DEBUG_6 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[20], value[21], value[46]);
	CMDQ_ERR("WDMA_DEBUG_7 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[22], value[23], value[47]);
	CMDQ_ERR("WDMA_DEBUG_8 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[24], value[25], value[48]);
	CMDQ_ERR("WDMA_DEBUG_9 +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[26], value[27], value[49]);
	CMDQ_ERR("WDMA_DEBUG_A +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[28], value[29], value[50]);
	CMDQ_ERR("WDMA_DEBUG_B +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[30], value[31], value[51]);
	CMDQ_ERR("WDMA_DEBUG_C +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[32], value[33], value[52]);
	CMDQ_ERR("WDMA_DEBUG_D +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[34], value[35], value[53]);
	CMDQ_ERR("WDMA_DEBUG_E +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[36], value[37], value[54]);
	CMDQ_ERR("WDMA_DEBUG_F +014: 0x%08x , +0ac: 0x%08x , +0b8: 0x%08x\n", value[38], value[39], value[55]);

	/* parse WDMA state */
	state = value[6] & 0x3FF;
	grep = (value[6] >> 13) & 0x1;
	isFIFOFull = (value[6] >> 12) & 0x1;

	CMDQ_ERR("WDMA state:0x%x (%s)\n", state, cmdq_mdp_get_wdma_state(state));
	CMDQ_ERR("WDMA in_req:%d in_ack:%d\n", (value[6] >> 15) & 0x1, (value[6] >> 14) & 0x1);

	/* note WDMA send request(i.e command) to SMI first, then SMI takes request data from WDMA FIFO */
	/* if SMI dose not process request and upstream HWs */
	/* such as MDP_RSZ send data to WDMA, WDMA FIFO will full finally */
	CMDQ_ERR("WDMA grep:%d, FIFO full:%d\n", grep, isFIFOFull);
	CMDQ_ERR("WDMA suggest: Need SMI help:%d, Need check WDMA config:%d\n", (grep),
		 ((grep == 0) && (isFIFOFull == 1)));
}

void cmdq_mdp_check_TF_address(unsigned int mva, char *module)
{
	bool findTFTask = false;
	char *searchStr = NULL;
	char bufInfoKey[] = "x";
	char str2int[MDP_BUF_INFO_STR_LEN + 1] = "";
	char *callerNameEnd = NULL;
	char *callerNameStart = NULL;
	int callerNameLen = TASK_COMM_LEN;
	int taskIndex = 0;
	int bufInfoIndex = 0;
	int tfTaskIndex = -1;
	int planeIndex = 0;
	unsigned int bufInfo[MDP_PORT_BUF_INFO_NUM] = {0};
	unsigned int bufAddrStart = 0;
	unsigned int bufAddrEnd = 0;

	/* search track task */
	for (taskIndex = 0; taskIndex < MDP_MAX_TASK_NUM; taskIndex++) {
		searchStr = strpbrk(gCmdqMDPTask[taskIndex].userDebugStr, bufInfoKey);
		bufInfoIndex = 0;

		/* catch buffer info in string and transform to integer */
		/* bufInfo format: */
		/* [address1, address2, address3, size1, size2, size3] */
		while (searchStr != NULL && findTFTask != true) {
			strncpy(str2int, searchStr + 1, MDP_BUF_INFO_STR_LEN);
			if (kstrtoint(str2int, 16, &bufInfo[bufInfoIndex]) != 0) {
				CMDQ_ERR("[MDP] buf info transform to integer failed\n");
				CMDQ_ERR("[MDP] fail string: %s\n", str2int);
			}

			searchStr = strpbrk(searchStr + MDP_BUF_INFO_STR_LEN + 1, bufInfoKey);
			bufInfoIndex++;

			/* check TF mva in this port or not */
			if (bufInfoIndex == MDP_PORT_BUF_INFO_NUM) {
				for (planeIndex = 0; planeIndex < MDP_MAX_PLANE_NUM; planeIndex++) {
					bufAddrStart = bufInfo[planeIndex];
					bufAddrEnd = bufAddrStart + bufInfo[planeIndex + MDP_MAX_PLANE_NUM];
					if (mva >= bufAddrStart && mva < bufAddrEnd) {
						findTFTask = true;
						break;
					}
				}
				bufInfoIndex = 0;
			}
		}

		/* find TF task and keep task index */
		if (findTFTask == true) {
			tfTaskIndex = taskIndex;
			break;
		}
	}

	/* find TF task caller and return dispatch key */
	if (findTFTask == true) {
		CMDQ_ERR("[MDP] TF caller: %s\n", gCmdqMDPTask[tfTaskIndex].callerName);
		CMDQ_ERR("%s\n", gCmdqMDPTask[tfTaskIndex].userDebugStr);
		strncat(module, "_", 1);

		/* catch caller name only before - or _ */
		callerNameStart = gCmdqMDPTask[tfTaskIndex].callerName;
		callerNameEnd = strchr(gCmdqMDPTask[tfTaskIndex].callerName, '-');
		if (callerNameEnd != NULL)
			callerNameLen = callerNameEnd - callerNameStart;
		else {
			callerNameEnd = strchr(gCmdqMDPTask[tfTaskIndex].callerName, '_');
			if (callerNameEnd != NULL)
				callerNameLen = callerNameEnd - callerNameStart;
		}
		strncat(module, gCmdqMDPTask[tfTaskIndex].callerName, callerNameLen);
	} else {
		CMDQ_ERR("[MDP] TF Task not found\n");
		for (taskIndex = 0; taskIndex < MDP_MAX_TASK_NUM; taskIndex++) {
			CMDQ_ERR("[MDP] Task%d:\n", taskIndex);
			CMDQ_ERR("[MDP] Caller: %s\n", gCmdqMDPTask[taskIndex].callerName);
			CMDQ_ERR("%s\n", gCmdqMDPTask[taskIndex].userDebugStr);
		}
	}
}

const char *cmdq_mdp_parse_error_module_by_hwflag(const struct TaskStruct *task)
{
	return cmdq_mdp_get_func()->parseErrModByEngFlag(task);
}

#ifdef CMDQ_COMMON_ENG_SUPPORT
void cmdq_mdp_platform_function_setting(void)
{
}
#endif

s32 cmdq_mdp_dump_wrot0_usage(void)
{
	s32 usage = atomic_read(&g_mdp_wrot0_usage);

	if (!usage)
		return 0;
	CMDQ_LOG("[warn]MDP WROT0 ref:%d\n", usage);
	CMDQ_LOG("[warn]SMI LARB0 ref:%d\n", smi_clk_get_ref_count(0));

	return usage;
}


