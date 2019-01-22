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

#ifndef __CMDQ_MDP_COMMON_H__
#define __CMDQ_MDP_COMMON_H__

#include "cmdq_def.h"
#include "cmdq_core.h"

#include <linux/types.h>

/* for debug share sram clock */
extern atomic_t g_mdp_wrot0_usage;

/* runtime pm to probe MDP device */
typedef s32 (*CmdqMDPProbe) (void);

void mdp_init(void);
void mdp_deinit(void);

/* dump mmsys config */
typedef void (*CmdqDumpMMSYSConfig) (void);

/* VENC callback function */
typedef int32_t(*CmdqVEncDumpInfo) (uint64_t engineFlag, int level);

/* query MDP clock is on  */
typedef bool(*CmdqMdpClockIsOn) (enum CMDQ_ENG_ENUM engine);

/* enable MDP clock  */
typedef void (*CmdqEnableMdpClock) (bool enable, enum CMDQ_ENG_ENUM engine);

/* Common Clock Framework */
typedef void (*CmdqMdpInitModuleClk) (void);

/* MDP callback function */
typedef int32_t(*CmdqMdpClockOn) (uint64_t engineFlag);

typedef int32_t(*CmdqMdpDumpInfo) (uint64_t engineFlag, int level);

typedef int32_t(*CmdqMdpResetEng) (uint64_t engineFlag);

typedef int32_t(*CmdqMdpClockOff) (uint64_t engineFlag);

/* MDP Initialization setting */
typedef void(*CmdqMdpInitialSet) (void);

/* Initialization & de-initialization MDP base VA */
typedef void (*CmdqMdpInitModuleBaseVA) (void);

typedef void (*CmdqMdpDeinitModuleBaseVA) (void);

/* MDP engine dump */
typedef void (*CmdqMdpDumpRSZ) (const unsigned long base, const char *label);

typedef void (*CmdqMdpDumpTDSHP) (const unsigned long base, const char *label);

/* test MDP clock function */
typedef uint32_t(*CmdqMdpRdmaGetRegOffsetSrcAddr) (void);

typedef uint32_t(*CmdqMdpWrotGetRegOffsetDstAddr) (void);

typedef uint32_t(*CmdqMdpWdmaGetRegOffsetDstAddr) (void);

typedef void (*CmdqTestcaseClkmgrMdp) (void);

typedef const char*(*CmdqDispatchModule) (uint64_t engineFlag);

typedef void (*CmdqTrackTask) (const struct TaskStruct *pTask);

typedef const char *(*CmdqPraseErrorModByEngFlag) (const struct TaskStruct *task);

typedef u64 (*CmdqMdpGetEngineGroupBits) (u32 engine_group);

typedef void (*CmdqMdpEnableCommonClock) (bool enable);
/* meansure task bandwidth for pmqos */
typedef u32(*CmdqMdpMeansureBandwidth) (u32 bandwidth);

struct cmdqMDPFuncStruct {
	CmdqMDPProbe mdp_probe;
	CmdqDumpMMSYSConfig dumpMMSYSConfig;
	CmdqVEncDumpInfo vEncDumpInfo;
	CmdqMdpInitModuleBaseVA initModuleBaseVA;
	CmdqMdpDeinitModuleBaseVA deinitModuleBaseVA;
	CmdqMdpClockIsOn mdpClockIsOn;
	CmdqEnableMdpClock enableMdpClock;
	CmdqMdpInitModuleClk initModuleCLK;
	CmdqMdpDumpRSZ mdpDumpRsz;
	CmdqMdpDumpTDSHP mdpDumpTdshp;
	CmdqMdpClockOn mdpClockOn;
	CmdqMdpDumpInfo mdpDumpInfo;
	CmdqMdpResetEng mdpResetEng;
	CmdqMdpClockOff mdpClockOff;
	CmdqMdpInitialSet mdpInitialSet;
	CmdqMdpRdmaGetRegOffsetSrcAddr rdmaGetRegOffsetSrcAddr;
	CmdqMdpWrotGetRegOffsetDstAddr wrotGetRegOffsetDstAddr;
	CmdqMdpWdmaGetRegOffsetDstAddr wdmaGetRegOffsetDstAddr;
	CmdqTestcaseClkmgrMdp testcaseClkmgrMdp;
	CmdqDispatchModule dispatchModule;
	CmdqTrackTask trackTask;
	CmdqPraseErrorModByEngFlag parseErrModByEngFlag;
	CmdqMdpGetEngineGroupBits getEngineGroupBits;
	CmdqErrorResetCB errorReset;
	CmdqMdpEnableCommonClock mdpEnableCommonClock;
	CmdqMdpMeansureBandwidth meansureBandwidth;
	CmdqBeginTaskCB beginTask;
	CmdqEndTaskCB endTask;
	CmdqBeginTaskCB beginISPTask;
	CmdqEndTaskCB endISPTask;
	CmdqStartTaskCB_ATOMIC startTask_atomic;
	CmdqFinishTaskCB_ATOMIC finishTask_atomic;
};

struct mdp_task {
	struct list_head entry;
	struct TaskStruct *cmdq_task;
	u32 bandwidth;
};

struct mdp_context {
	struct list_head mdp_tasks;
	struct mdp_task *bandwidth_task;
};

struct mdp_pmqos_record {
	uint32_t mdp_throughput;
	struct timeval submit_tm;
	struct timeval end_tm;
};

/* track MDP task */
#define DEBUG_STR_LEN 1024 /* debug str length */
#define MDP_MAX_TASK_NUM 5 /* num of tasks to be keep */
#define MDP_MAX_PLANE_NUM 3 /* max color format plane num */
#define MDP_PORT_BUF_INFO_NUM (MDP_MAX_PLANE_NUM * 2) /* each plane has 2 info address and size */
#define MDP_BUF_INFO_STR_LEN 8 /* each buf info length */
#define MDP_DISPATCH_KEY_STR_LEN (TASK_COMM_LEN + 5) /* dispatch key format is MDP_(ThreadName) */
#define MDP_TOTAL_THREAD 8
#ifdef CMDQ_SECURE_PATH_SUPPORT
#define MDP_THREAD_START (CMDQ_MIN_SECURE_THREAD_ID + 2)
#else
#define MDP_THREAD_START CMDQ_DYNAMIC_THREAD_ID_START
#endif

struct cmdqMDPTaskStruct {
	char callerName[TASK_COMM_LEN];
	char userDebugStr[DEBUG_STR_LEN];
};

#ifdef __cplusplus
extern "C" {
#endif
	void cmdq_mdp_init(void);
	void cmdq_mdp_deinit(void);

	void cmdq_mdp_virtual_function_setting(void);
	void cmdq_mdp_map_mmsys_VA(void);
	long cmdq_mdp_get_module_base_VA_MMSYS_CONFIG(void);
	void cmdq_mdp_unmap_mmsys_VA(void);
	struct cmdqMDPFuncStruct *cmdq_mdp_get_func(void);

	void cmdq_mdp_enable(uint64_t engineFlag, enum CMDQ_ENG_ENUM engine);

	int cmdq_mdp_loop_reset(enum CMDQ_ENG_ENUM engine,
				const unsigned long resetReg,
				const unsigned long resetStateReg,
				const uint32_t resetMask,
				const uint32_t resetValue, const bool pollInitResult);

	void cmdq_mdp_loop_off(enum CMDQ_ENG_ENUM engine,
			       const unsigned long resetReg,
			       const unsigned long resetStateReg,
			       const uint32_t resetMask,
			       const uint32_t resetValue, const bool pollInitResult);

	const char *cmdq_mdp_get_rsz_state(const uint32_t state);

	void cmdq_mdp_dump_venc(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rdma(const unsigned long base, const char *label);
	void cmdq_mdp_dump_rot(const unsigned long base, const char *label);
	void cmdq_mdp_dump_color(const unsigned long base, const char *label);
	void cmdq_mdp_dump_wdma(const unsigned long base, const char *label);

	void cmdq_mdp_check_TF_address(unsigned int mva, char *module);
	const char *cmdq_mdp_parse_error_module_by_hwflag(const struct TaskStruct *task);

	s32 cmdq_mdp_dump_wrot0_usage(void);

/**************************************************************************************/
/*******************                    Platform dependent function                    ********************/
/**************************************************************************************/

	int32_t cmdqMdpClockOn(uint64_t engineFlag);

	int32_t cmdqMdpDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqVEncDumpInfo(uint64_t engineFlag, int level);

	int32_t cmdqMdpResetEng(uint64_t engineFlag);

	int32_t cmdqMdpClockOff(uint64_t engineFlag);

	uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr(void);
	uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr(void);
	uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr(void);

	void testcase_clkmgr_mdp(void);

	/* Platform virtual function setting */
	void cmdq_mdp_platform_function_setting(void);

#ifdef __cplusplus
}
#endif
#endif				/* __CMDQ_MDP_COMMON_H__ */
