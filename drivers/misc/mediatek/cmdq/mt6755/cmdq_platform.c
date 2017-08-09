#include "cmdq_platform.h"
#include "cmdq_core.h"
#include "cmdq_virtual.h"
#include "cmdq_reg.h"
#include "../cmdq_device.h"
#include <linux/vmalloc.h>

#ifndef CMDQ_USE_CCF
#include <mach/mt_clkmgr.h>
#endif				/* !defined(CMDQ_USE_CCF) */

#define MMSYS_CONFIG_BASE  cmdq_dev_get_module_base_VA_MMSYS_CONFIG()

typedef struct RegDef {
	int offset;
	const char *name;
} RegDef;

const bool cmdq_platform_support_sync_non_suspendable(void)
{
	return true;
}

int32_t cmdq_platform_subsys_from_phys_addr(uint32_t physAddr)
{
#define DISP_PWM_BASE 0x1100E000

	const int32_t msb = (physAddr & 0x0FFFF0000) >> 16;
	int32_t subsysID = -1;

#undef DECLARE_CMDQ_SUBSYS
#define DECLARE_CMDQ_SUBSYS(addr, id, grp, base) { if (addr == msb) {subsysID = id; break; } }
	do {
#include "cmdq_subsys.h"
		/* Extra handle for HW registers which not in GCE subsys table, should check and add by case */
		if (DISP_PWM_BASE == (physAddr & 0xFFFFF000)) {
			CMDQ_MSG("Special handle subsys (PWM), physAddr:0x%08x\n", physAddr);
			subsysID = CMDQ_SPECIAL_SUBSYS_ADDR;
			break;
		}
	} while (0);
#undef DECLARE_CMDQ_SUBSYS

	if (-1 == subsysID) {
		/* printf error message */
		CMDQ_ERR("unrecognized subsys, msb=0x%04x, physAddr:0x%08x\n", msb, physAddr);
	}
	return subsysID;
}

void cmdq_platform_get_reg_id_from_hwflag(uint64_t hwflag,
					  CMDQ_DATA_REGISTER_ENUM *valueRegId,
					  CMDQ_DATA_REGISTER_ENUM *destRegId,
					  CMDQ_EVENT_ENUM *regAccessToken)
{
	*regAccessToken = CMDQ_SYNC_TOKEN_INVALID;

	if (hwflag & (1LL << CMDQ_ENG_JPEG_ENC)) {
		*valueRegId = CMDQ_DATA_REG_JPEG;
		*destRegId = CMDQ_DATA_REG_JPEG_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_0;
	} else if (hwflag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		*valueRegId = CMDQ_DATA_REG_2D_SHARPNESS_0;
		*destRegId = CMDQ_DATA_REG_2D_SHARPNESS_0_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_1;
	} else if (hwflag & (1LL << CMDQ_ENG_DISP_COLOR0)) {
		*valueRegId = CMDQ_DATA_REG_PQ_COLOR;
		*destRegId = CMDQ_DATA_REG_PQ_COLOR_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_3;
	} else {
		/* assume others are debug cases */
		*valueRegId = CMDQ_DATA_REG_DEBUG;
		*destRegId = CMDQ_DATA_REG_DEBUG_DST;
		*regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	}
}

const char *cmdq_platform_module_from_event_id(const int32_t event)
{
	const char *module = "CMDQ";

	switch (event) {
	case CMDQ_EVENT_DISP_RDMA0_SOF:
	case CMDQ_EVENT_DISP_RDMA1_SOF:
	case CMDQ_EVENT_DISP_RDMA0_EOF:
	case CMDQ_EVENT_DISP_RDMA1_EOF:
	case CMDQ_EVENT_DISP_RDMA0_UNDERRUN:
	case CMDQ_EVENT_DISP_RDMA1_UNDERRUN:
		module = "DISP_RDMA";
		break;

	case CMDQ_EVENT_DISP_WDMA0_SOF:
	case CMDQ_EVENT_DISP_WDMA0_EOF:
		module = "DISP_WDMA";
		break;

	case CMDQ_EVENT_DISP_OVL0_SOF:
	case CMDQ_EVENT_DISP_OVL0_EOF:
		module = "DISP_OVL";
		break;

	case CMDQ_EVENT_DSI_TE:
	case CMDQ_EVENT_DISP_COLOR_SOF ... CMDQ_EVENT_DISP_PWM0_SOF:
	case CMDQ_EVENT_DISP_COLOR_EOF ... CMDQ_EVENT_DISP_DPI0_EOF:
	case CMDQ_EVENT_MUTEX0_STREAM_EOF ... CMDQ_EVENT_MUTEX4_STREAM_EOF:
		module = "DISP";
		break;
	case CMDQ_SYNC_TOKEN_CONFIG_DIRTY:
	case CMDQ_SYNC_TOKEN_STREAM_EOF:
		module = "DISP";
		break;

	case CMDQ_EVENT_MDP_RDMA0_SOF ... CMDQ_EVENT_MDP_RSZ1_SOF:
	case CMDQ_EVENT_MDP_TDSHP_SOF:
	case CMDQ_EVENT_MDP_WDMA_SOF ... CMDQ_EVENT_MDP_WROT_SOF:
	case CMDQ_EVENT_MDP_RDMA0_EOF ... CMDQ_EVENT_MDP_WROT_READ_EOF:
	case CMDQ_EVENT_MUTEX5_STREAM_EOF ... CMDQ_EVENT_MUTEX9_STREAM_EOF:
		module = "MDP";
		break;

	case CMDQ_EVENT_ISP_PASS2_2_EOF:
	case CMDQ_EVENT_ISP_PASS2_1_EOF:
	case CMDQ_EVENT_ISP_PASS2_0_EOF:
	case CMDQ_EVENT_ISP_PASS1_1_EOF:
	case CMDQ_EVENT_ISP_PASS1_0_EOF:
	case CMDQ_EVENT_ISP_CAMSV_2_PASS1_DONE:
	case CMDQ_EVENT_ISP_CAMSV_1_PASS1_DONE:
	case CMDQ_EVENT_ISP_SENINF_CAM1_2_3_FULL:
	case CMDQ_EVENT_ISP_SENINF_CAM0_FULL:
		module = "ISP";
		break;

	case CMDQ_EVENT_JPEG_ENC_EOF:
	case CMDQ_EVENT_JPEG_DEC_EOF:
		module = "JPGE";
		break;

	case CMDQ_EVENT_VENC_EOF:
	case CMDQ_EVENT_VENC_MB_DONE:
	case CMDQ_EVENT_VENC_128BYTE_CNT_DONE:
		module = "VENC";
		break;

	default:
		module = "CMDQ";
		break;
	}

	return module;
}

const char *cmdq_platform_parse_module_from_reg_addr(uint32_t reg_addr)
{
	const uint32_t addr_base_and_page = (reg_addr & 0xFFFFF000);

	/* for well-known base, we check them with 12-bit mask */
	/* defined in mt_reg_base.h */
	/* TODO: comfirm with SS if IO_VIRT_TO_PHYS workable when enable device tree? */
	switch (addr_base_and_page) {
	case 0x14001000: /* MDP_RDMA */
	case 0x14002000: /* MDP_RSZ0 */
	case 0x14003000: /* MDP_RSZ1 */
	case 0x14004000: /* MDP_WDMA */
	case 0x14005000: /* MDP_WROT */
	case 0x14006000: /* MDP_TDSHP */
		return "MDP";
	case 0x1400C000: /* DISP_COLOR */
		return "COLOR";
	case 0x1400D000: /* DISP_COLOR */
		return "CCORR";
	case 0x14007000: /* DISP_OVL0 */
		return "OVL0";
	case 0x14008000: /* DISP_OVL1 */
		return "OVL1";
	case 0x1400E000: /* DISP_AAL */
	case 0x1400F000: /* DISP_GAMMA */
		return "AAL";
	case 0x17002FFF: /* VENC */
		return "VENC";
	case 0x17003FFF: /* JPGENC */
		return "JPGENC";
	case 0x17004FFF: /* JPGDEC */
		return "JPGDEC";
	}

	/* for other register address we rely on GCE subsys to group them with */
	/* 16-bit mask. */
	return cmdq_core_parse_subsys_from_reg_addr(reg_addr);
}

const int32_t cmdq_platform_can_module_entry_suspend(EngineStruct *engineList)
{
	int32_t status = 0;
	int i;
	CMDQ_ENG_ENUM e = 0;

	CMDQ_ENG_ENUM mdpEngines[] = {
		CMDQ_ENG_ISP_IMGI,
		CMDQ_ENG_MDP_RDMA0,
		CMDQ_ENG_MDP_RSZ0,
		CMDQ_ENG_MDP_RSZ1,
		CMDQ_ENG_MDP_TDSHP0,
		CMDQ_ENG_MDP_WROT0,
		CMDQ_ENG_MDP_WDMA
	};

	for (i = 0; i < (sizeof(mdpEngines) / sizeof(CMDQ_ENG_ENUM)); ++i) {
		e = mdpEngines[i];
		if (0 != engineList[e].userCount) {
			CMDQ_ERR("suspend but engine %d has userCount %d, owner=%d\n",
				 e, engineList[e].userCount, engineList[e].currOwner);
			status = -EBUSY;
		}
	}

	return status;
}

const char *cmdq_platform_parse_error_module_by_hwflag_impl(const TaskStruct *pTask)
{
	const char *module = NULL;
	const uint32_t ISP_ONLY[2] = {
		((1LL << CMDQ_ENG_ISP_IMGI) | (1LL << CMDQ_ENG_ISP_IMG2O)),
		((1LL << CMDQ_ENG_ISP_IMGI) | (1LL << CMDQ_ENG_ISP_IMG2O) |
		 (1LL << CMDQ_ENG_ISP_IMGO))
	};

	/* common part for both normal and secure path */
	/* for JPEG scenario, use HW flag is sufficient */
	if (pTask->engineFlag & (1LL << CMDQ_ENG_JPEG_ENC))
		module = "JPGENC";
	else if (pTask->engineFlag & (1LL << CMDQ_ENG_JPEG_DEC))
		module = "JPGDEC";
	else if ((ISP_ONLY[0] == pTask->engineFlag) || (ISP_ONLY[1] == pTask->engineFlag))
		module = "ISP_ONLY";
	else if (cmdq_get_func()->isDispScenario(pTask->scenario))
		module = "DISP";

	/* for secure path, use HW flag is sufficient */
	do {
		if (NULL != module)
			break;

		if (false == pTask->secData.isSecure) {
			/* normal path, need parse current running instruciton for more detail */
			break;
		}

		else if (CMDQ_ENG_MDP_GROUP_FLAG(pTask->engineFlag)) {
			module = "MDP";
			break;
		}

		module = "CMDQ";
	} while (0);

	/* other case, we need to analysis instruction for more detail */
	return module;
}

void cmdq_platform_dump_mmsys_config(void)
{
	int i = 0;
	uint32_t value = 0;

	static const struct RegDef configRegisters[] = {
		{0x01c, "ISP_MOUT_EN"},
		{0x020, "MDP_RDMA_MOUT_EN"},
		{0x024, "MDP_PRZ0_MOUT_EN"},
		{0x028, "MDP_PRZ1_MOUT_EN"},
		{0x02C, "MDP_TDSHP_MOUT_EN"},
		{0x030, "DISP_OVL0_MOUT_EN"},
		{0x034, "DISP_OVL1_MOUT_EN"},
		{0x038, "DISP_DITHER_MOUT_EN"},
		{0x03C, "DISP_UFOE_MOUT_EN"},
		/* {0x040, "MMSYS_MOUT_RST"}, */
		{0x044, "MDP_PRZ0_SEL_IN"},
		{0x048, "MDP_PRZ1_SEL_IN"},
		{0x04C, "MDP_TDSHP_SEL_IN"},
		{0x050, "MDP_WDMA_SEL_IN"},
		{0x054, "MDP_WROT_SEL_IN"},
		{0x058, "DISP_COLOR_SEL_IN"},
		{0x05C, "DISP_WDMA_SEL_IN"},
		{0x060, "DISP_UFOE_SEL_IN"},
		{0x064, "DSI0_SEL_IN"},
		{0x068, "DPI0_SEL_IN"},
		{0x06C, "DISP_RDMA0_SOUT_SEL_IN"},
		{0x070, "DISP_RDMA1_SOUT_SEL_IN"},
		{0x0F0, "MMSYS_MISC"},
		/* ACK and REQ related */
		{0x8a0, "DISP_DL_VALID_0"},
		{0x8a4, "DISP_DL_VALID_1"},
		{0x8a8, "DISP_DL_READY_0"},
		{0x8ac, "DISP_DL_READY_1"},
		{0x8b0, "MDP_DL_VALID_0"},
		{0x8b4, "MDP_DL_READY_0"}
	};

	for (i = 0; i < sizeof(configRegisters) / sizeof(configRegisters[0]); ++i) {
		value = CMDQ_REG_GET16(MMSYS_CONFIG_BASE + configRegisters[i].offset);
		CMDQ_ERR("%s: 0x%08x\n", configRegisters[i].name, value);
	}
}

void cmdq_platform_dump_clock_gating(void)
{
	uint32_t value[3] = { 0 };

	value[0] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x100);
	value[1] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x110);
	/* value[2] = CMDQ_REG_GET32(MMSYS_CONFIG_BASE + 0x890); */
	CMDQ_ERR("MMSYS_CG_CON0(deprecated): 0x%08x, MMSYS_CG_CON1: 0x%08x\n", value[0], value[1]);
	/* CMDQ_ERR("MMSYS_DUMMY_REG: 0x%08x\n", value[2]); */
#if !defined(CMDQ_USE_CCF) && !defined(CONFIG_MTK_FPGA)
	CMDQ_ERR("ISPSys clock state %d\n", subsys_is_on(SYS_ISP));
	CMDQ_ERR("DisSys clock state %d\n", subsys_is_on(SYS_DIS));
	CMDQ_ERR("VDESys clock state %d\n", subsys_is_on(SYS_VDE));
#endif
}

uint64_t cmdq_platform_flag_from_scenario(CMDQ_SCENARIO_ENUM scn)
{
	uint64_t flag = 0;

	switch (scn) {
	case CMDQ_SCENARIO_JPEG_DEC:
		flag = (1LL << CMDQ_ENG_JPEG_DEC);
		break;
	case CMDQ_SCENARIO_PRIMARY_DISP:
		flag = (1LL << CMDQ_ENG_DISP_OVL0) |
		    (1LL << CMDQ_ENG_DISP_COLOR0) |
		    (1LL << CMDQ_ENG_DISP_AAL) |
		    (1LL << CMDQ_ENG_DISP_GAMMA) |
		    (1LL << CMDQ_ENG_DISP_RDMA0) |
		    (1LL << CMDQ_ENG_DISP_UFOE) | (1LL << CMDQ_ENG_DISP_DSI0_CMD);
		break;
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
		flag = 0LL;
		break;
	case CMDQ_SCENARIO_PRIMARY_ALL:
		flag = ((1LL << CMDQ_ENG_DISP_OVL0) |
			(1LL << CMDQ_ENG_DISP_WDMA0) |
			(1LL << CMDQ_ENG_DISP_COLOR0) |
			(1LL << CMDQ_ENG_DISP_AAL) |
			(1LL << CMDQ_ENG_DISP_GAMMA) |
			(1LL << CMDQ_ENG_DISP_RDMA0) |
			(1LL << CMDQ_ENG_DISP_UFOE) | (1LL << CMDQ_ENG_DISP_DSI0_CMD));
		break;
	case CMDQ_SCENARIO_SUB_DISP:
		flag = ((1LL << CMDQ_ENG_DISP_OVL1) |
			(1LL << CMDQ_ENG_DISP_RDMA1) | (1LL << CMDQ_ENG_DISP_DPI));
		break;
	case CMDQ_SCENARIO_SUB_MEMOUT:
		flag = ((1LL << CMDQ_ENG_DISP_OVL1) | (1LL << CMDQ_ENG_DISP_WDMA1));
		break;
	case CMDQ_SCENARIO_SUB_ALL:
		flag = ((1LL << CMDQ_ENG_DISP_OVL1) |
			(1LL << CMDQ_ENG_DISP_WDMA1) |
			(1LL << CMDQ_ENG_DISP_RDMA1) | (1LL << CMDQ_ENG_DISP_DPI));
		break;
	case CMDQ_SCENARIO_RDMA0_DISP:
		flag = ((1LL << CMDQ_ENG_DISP_RDMA0) | (1LL << CMDQ_ENG_DISP_DSI0_CMD));
		break;
	case CMDQ_SCENARIO_RDMA0_COLOR0_DISP:
		flag = ((1LL << CMDQ_ENG_DISP_RDMA0) |
			(1LL << CMDQ_ENG_DISP_COLOR0) |
			(1LL << CMDQ_ENG_DISP_AAL) |
			(1LL << CMDQ_ENG_DISP_GAMMA) |
			(1LL << CMDQ_ENG_DISP_UFOE) | (1LL << CMDQ_ENG_DISP_DSI0_CMD));
		break;
	case CMDQ_SCENARIO_MHL_DISP:
	case CMDQ_SCENARIO_RDMA1_DISP:
		flag = ((1LL << CMDQ_ENG_DISP_RDMA1) | (1LL << CMDQ_ENG_DISP_DPI));
		break;

	default:
		flag = cmdq_core_flag_from_scenario(scn);
		break;
	}

	return flag;
}

void cmdq_platform_init_module_PA_stat(void)
{
#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_INSTRUCTION_COUNT)
	/* Do Nothing */
#endif
}

void cmdq_platform_function_setting(void)
{
	cmdqCoreFuncStruct *pFunc;

	pFunc = cmdq_get_func();

	/*
	 * GCE capability
	 */
	pFunc->syncNonSuspendable = cmdq_platform_support_sync_non_suspendable;
	pFunc->subsysPA = cmdq_platform_subsys_from_phys_addr;

	/**
	 * Module dependent
	 *
	 */
	pFunc->getRegID = cmdq_platform_get_reg_id_from_hwflag;
	pFunc->moduleFromEvent = cmdq_platform_module_from_event_id;
	pFunc->parseModule = cmdq_platform_parse_module_from_reg_addr;
	pFunc->moduleEntrySuspend = cmdq_platform_can_module_entry_suspend;
	pFunc->parseErrorModule = cmdq_platform_parse_error_module_by_hwflag_impl;

	/**
	 * Debug
	 *
	 */
	pFunc->dumpMMSYSConfig = cmdq_platform_dump_mmsys_config;
	pFunc->dumpClockGating = cmdq_platform_dump_clock_gating;

	/**
	 * Record usage
	 *
	 */
	pFunc->flagFromScenario = cmdq_platform_flag_from_scenario;

	/**
	 * Test
	 *
	 */
	pFunc->initModulePAStat = cmdq_platform_init_module_PA_stat;
}
