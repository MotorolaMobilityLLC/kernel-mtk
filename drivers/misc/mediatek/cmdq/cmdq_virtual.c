#include "cmdq_core.h"
#include "cmdq_reg.h"
#include "cmdq_virtual.h"
#include <linux/seq_file.h>
#ifdef CMDQ_CONFIG_SMI
#include "smi_debug.h"
#endif
#ifdef CMDQ_CG_M4U_LARB0
#include "m4u.h"
#endif

static cmdqCoreFuncStruct gFunctionPointer;

/*
 * GCE capability
 */
const bool cmdq_virtual_support_sync_non_suspendable(void)
{
	return false;
}

const bool cmdq_virtual_support_wait_and_receive_event_in_same_tick(void)
{
	return true;
}

const uint32_t cmdq_virtual_get_subsys_LSB_in_argA(void)
{
	return 16;
}

int32_t cmdq_virtual_subsys_from_phys_addr(uint32_t physAddr)
{
	const int32_t msb = (physAddr & 0x0FFFF0000) >> 16;
	int32_t subsysID = -1;

#undef DECLARE_CMDQ_SUBSYS
#define DECLARE_CMDQ_SUBSYS(addr, id, grp, base) { if (addr == msb) {subsysID = id; break; } }
	do {
#include "cmdq_subsys.h"
	} while (0);
#undef DECLARE_CMDQ_SUBSYS

	if (-1 == subsysID) {
		/* printf error message */
		CMDQ_ERR("unrecognized subsys, msb=0x%04x, physAddr:0x%08x\n", msb, physAddr);
	}

	return subsysID;
}

/* HW thread related */
const bool cmdq_virtual_is_a_secure_thread(const int32_t thread)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if ((CMDQ_MIN_SECURE_THREAD_ID <= thread) &&
	    (CMDQ_MIN_SECURE_THREAD_ID + CMDQ_MAX_SECURE_THREAD_COUNT > thread)) {
		return true;
	}
#endif
	return false;
}

const bool cmdq_virtual_is_valid_notify_thread_for_secure_path(const int32_t thread)
{
#if defined(CMDQ_SECURE_PATH_SUPPORT) && !defined(CMDQ_SECURE_PATH_NORMAL_IRQ)
	return (15 == thread) ? (true) : (false);
#else
	return false;
#endif
}

/**
 * Scenario related
 *
 */
bool cmdq_virtual_is_disp_scenario(const CMDQ_SCENARIO_ENUM scenario)
{
	bool dispScenario = false;

	switch (scenario) {
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
	case CMDQ_SCENARIO_PRIMARY_ALL:
	case CMDQ_SCENARIO_SUB_DISP:
	case CMDQ_SCENARIO_SUB_MEMOUT:
	case CMDQ_SCENARIO_SUB_ALL:
	case CMDQ_SCENARIO_MHL_DISP:
	case CMDQ_SCENARIO_RDMA0_DISP:
	case CMDQ_SCENARIO_RDMA1_DISP:
	case CMDQ_SCENARIO_RDMA2_DISP:
	case CMDQ_SCENARIO_RDMA0_COLOR0_DISP:
	case CMDQ_SCENARIO_TRIGGER_LOOP:
	case CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP:
	case CMDQ_SCENARIO_LOWP_TRIGGER_LOOP:
	case CMDQ_SCENARIO_DISP_CONFIG_AAL:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PWM:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PWM:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PQ:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PQ:
	case CMDQ_SCENARIO_DISP_ESD_CHECK:
	case CMDQ_SCENARIO_DISP_SCREEN_CAPTURE:
	case CMDQ_SCENARIO_DISP_MIRROR_MODE:
	case CMDQ_SCENARIO_DISP_CONFIG_OD:
		/* color path */
	case CMDQ_SCENARIO_DISP_COLOR:
	case CMDQ_SCENARIO_USER_DISP_COLOR:
		/* secure path */
	case CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH:
	case CMDQ_SCENARIO_DISP_SUB_DISABLE_SECURE_PATH:
		dispScenario = true;
		break;
	default:
		break;
	}
	/* freely dispatch */
	return dispScenario;
}

bool cmdq_virtual_should_enable_prefetch(CMDQ_SCENARIO_ENUM scenario)
{
	bool shouldPrefetch = false;

	switch (scenario) {
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_ALL:
	case CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP:
	case CMDQ_SCENARIO_DEBUG_PREFETCH:	/* HACK: force debug into 0/1 thread */
		/* any path that connects to Primary DISP HW */
		/* should enable prefetch. */
		/* MEMOUT scenarios does not. */
		/* Also, since thread 0/1 shares one prefetch buffer, */
		/* we allow only PRIMARY path to use prefetch. */
		shouldPrefetch = true;
		break;
	default:
		break;
	}

	return shouldPrefetch;
}

bool cmdq_virtual_should_profile(CMDQ_SCENARIO_ENUM scenario)
{
	bool shouldProfile = false;

#ifdef CMDQ_GPR_SUPPORT
	switch (scenario) {
	case CMDQ_SCENARIO_DEBUG_PREFETCH:
	case CMDQ_SCENARIO_DEBUG:
		return true;
	default:
		break;
	}
#else
	/* note command profile method depends on GPR */
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
#endif
	return shouldProfile;
}

int cmdq_virtual_disp_thread(CMDQ_SCENARIO_ENUM scenario)
{
	switch (scenario) {
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_ALL:
	case CMDQ_SCENARIO_DISP_CONFIG_AAL:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PWM:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PQ:
	case CMDQ_SCENARIO_DISP_CONFIG_OD:
	case CMDQ_SCENARIO_RDMA0_DISP:
	case CMDQ_SCENARIO_RDMA0_COLOR0_DISP:
	case CMDQ_SCENARIO_DEBUG_PREFETCH:	/* HACK: force debug into 0/1 thread */
		/* primary config: thread 0 */
		return 0;

	case CMDQ_SCENARIO_SUB_DISP:
	case CMDQ_SCENARIO_SUB_ALL:
	case CMDQ_SCENARIO_MHL_DISP:
	case CMDQ_SCENARIO_RDMA1_DISP:
	case CMDQ_SCENARIO_RDMA2_DISP:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PQ:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PWM:
	case CMDQ_SCENARIO_SUB_MEMOUT:
		/* when HW thread 0 enables pre-fetch, */
		/* any thread 1 operation will let HW thread 0's behavior abnormally */
		/* forbid thread 1 */
		return 5;

	case CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP:
		return 2;

	case CMDQ_SCENARIO_DISP_ESD_CHECK:
		return 6;

	case CMDQ_SCENARIO_DISP_SCREEN_CAPTURE:
	case CMDQ_SCENARIO_DISP_MIRROR_MODE:
		return 3;

	case CMDQ_SCENARIO_DISP_COLOR:
	case CMDQ_SCENARIO_USER_DISP_COLOR:
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
		return 4;
	default:
		/* freely dispatch */
		return CMDQ_INVALID_THREAD;
	}
	/* freely dispatch */
	return CMDQ_INVALID_THREAD;
}

int cmdq_virtual_get_thread_index(CMDQ_SCENARIO_ENUM scenario, const bool secure)
{
#if defined(CMDQ_SECURE_PATH_SUPPORT) && !defined(CMDQ_SECURE_PATH_NORMAL_IRQ)
	if (!secure && CMDQ_SCENARIO_SECURE_NOTIFY_LOOP == scenario)
		return 15;
#endif

	if (!secure)
		return cmdq_get_func()->dispThread(scenario);

	/* dispatch secure thread according to scenario */
	switch (scenario) {
	case CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH:
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_ALL:
	case CMDQ_SCENARIO_RDMA0_DISP:
	case CMDQ_SCENARIO_DEBUG_PREFETCH:
		/* CMDQ_MIN_SECURE_THREAD_ID */
		return 12;
	case CMDQ_SCENARIO_DISP_SUB_DISABLE_SECURE_PATH:
	case CMDQ_SCENARIO_SUB_DISP:
	case CMDQ_SCENARIO_SUB_ALL:
	case CMDQ_SCENARIO_MHL_DISP:
		/* because mirror mode and sub disp never use at the same time in secure path, */
		/* dispatch to same HW thread */
	case CMDQ_SCENARIO_DISP_MIRROR_MODE:
	case CMDQ_SCENARIO_DISP_COLOR:
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
		return 13;
	case CMDQ_SCENARIO_USER_MDP:
	case CMDQ_SCENARIO_USER_SPACE:
	case CMDQ_SCENARIO_DEBUG:
		/* because there is one input engine for MDP, reserve one secure thread is enough */
		return 14;
	default:
		CMDQ_ERR("no dedicated secure thread for senario:%d\n", scenario);
		return CMDQ_INVALID_THREAD;
	}
}

CMDQ_HW_THREAD_PRIORITY_ENUM cmdq_virtual_priority_from_scenario(CMDQ_SCENARIO_ENUM scenario)
{
	switch (scenario) {
	case CMDQ_SCENARIO_PRIMARY_DISP:
	case CMDQ_SCENARIO_PRIMARY_ALL:
	case CMDQ_SCENARIO_SUB_MEMOUT:
	case CMDQ_SCENARIO_SUB_DISP:
	case CMDQ_SCENARIO_SUB_ALL:
	case CMDQ_SCENARIO_RDMA1_DISP:
	case CMDQ_SCENARIO_RDMA2_DISP:
	case CMDQ_SCENARIO_MHL_DISP:
	case CMDQ_SCENARIO_RDMA0_DISP:
	case CMDQ_SCENARIO_RDMA0_COLOR0_DISP:
	case CMDQ_SCENARIO_DISP_MIRROR_MODE:
	case CMDQ_SCENARIO_PRIMARY_MEMOUT:
	case CMDQ_SCENARIO_DISP_CONFIG_AAL:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_GAMMA:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_DITHER:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PWM:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PWM:
	case CMDQ_SCENARIO_DISP_CONFIG_PRIMARY_PQ:
	case CMDQ_SCENARIO_DISP_CONFIG_SUB_PQ:
	case CMDQ_SCENARIO_DISP_CONFIG_OD:
		/* color path */
	case CMDQ_SCENARIO_DISP_COLOR:
	case CMDQ_SCENARIO_USER_DISP_COLOR:
		/* secure path */
	case CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH:
	case CMDQ_SCENARIO_DISP_SUB_DISABLE_SECURE_PATH:
		/* currently, a prefetch thread is always in high priority. */
		return CMDQ_THR_PRIO_DISPLAY_CONFIG;

		/* HACK: force debug into 0/1 thread */
	case CMDQ_SCENARIO_DEBUG_PREFETCH:
		return CMDQ_THR_PRIO_DISPLAY_CONFIG;

	case CMDQ_SCENARIO_DISP_ESD_CHECK:
	case CMDQ_SCENARIO_DISP_SCREEN_CAPTURE:
		return CMDQ_THR_PRIO_DISPLAY_ESD;

	case CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP:
		return CMDQ_THR_PRIO_SUPERHIGH;

	case CMDQ_SCENARIO_LOWP_TRIGGER_LOOP:
		return CMDQ_THR_PRIO_SUPERLOW;

	default:
		/* other cases need exta logic, see below. */
		break;
	}

	if (cmdq_get_func()->isLoopScenario(scenario, true))
		return CMDQ_THR_PRIO_DISPLAY_TRIGGER;
	else
		return CMDQ_THR_PRIO_NORMAL;
}

bool cmdq_virtual_force_loop_irq(CMDQ_SCENARIO_ENUM scenario)
{
	bool forceLoop = false;

#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (CMDQ_SCENARIO_SECURE_NOTIFY_LOOP == scenario) {
		/* For secure notify loop, we need IRQ to update secure task */
		forceLoop = true;
	}
#endif
	if (CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP == scenario
		|| CMDQ_SCENARIO_LOWP_TRIGGER_LOOP == scenario) {
		/* For monitor thread loop, we need IRQ to set callback function */
		forceLoop = true;
	}

	return forceLoop;
}

bool cmdq_virtual_is_loop_scenario(CMDQ_SCENARIO_ENUM scenario, bool displayOnly)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (!displayOnly && CMDQ_SCENARIO_SECURE_NOTIFY_LOOP == scenario)
		return true;
#endif

	if (CMDQ_SCENARIO_TRIGGER_LOOP == scenario
		|| CMDQ_SCENARIO_HIGHP_TRIGGER_LOOP == scenario
		|| CMDQ_SCENARIO_LOWP_TRIGGER_LOOP == scenario)
		return true;

	return false;
}

/**
 * Module dependent
 *
 */
void cmdq_virtual_get_reg_id_from_hwflag(uint64_t hwflag, CMDQ_DATA_REGISTER_ENUM *valueRegId,
					 CMDQ_DATA_REGISTER_ENUM *destRegId,
					 CMDQ_EVENT_ENUM *regAccessToken)
{
	/* do nothing */
}

const char *cmdq_virtual_module_from_event_id(const int32_t event)
{
	const char *module = "CMDQ";
	return module;
}

const char *cmdq_virtual_parse_module_from_reg_addr(uint32_t reg_addr)
{
	const char *module = "CMDQ";
	return module;
}

const int32_t cmdq_virtual_can_module_entry_suspend(EngineStruct *engineList)
{
	return 0;
}

ssize_t cmdq_virtual_print_status_clock(char *buf)
{
	int32_t length = 0;
	char *pBuffer = buf;

#ifdef CMDQ_PWR_AWARE
	/* MT_CG_DISP0_MUTEX_32K is removed in this platform */
	pBuffer += sprintf(pBuffer, "MT_CG_INFRA_GCE: %d\n", cmdq_dev_gce_clock_is_enable());
	length = pBuffer - buf;
#endif
	return length;
}

void cmdq_virtual_print_status_seq_clock(struct seq_file *m)
{
#ifdef CMDQ_PWR_AWARE
	/* MT_CG_DISP0_MUTEX_32K is removed in this platform */
	seq_printf(m, "MT_CG_INFRA_GCE: %d\n", cmdq_dev_gce_clock_is_enable());
#endif
}

void cmdq_virtual_enable_common_clock_locked(bool enable)
{
#ifdef CMDQ_PWR_AWARE
	if (enable) {
		CMDQ_VERBOSE("[CLOCK] Enable SMI & LARB0 Clock\n");
		cmdq_dev_enable_clock_SMI_COMMON(enable);
#ifdef CMDQ_CG_M4U_LARB0
		m4u_larb0_enable("CMDQ_MDP");
#else
		cmdq_dev_enable_clock_SMI_LARB0(enable);
#endif
#ifdef CMDQ_CG_DISP0_MUTEX_32K
		CMDQ_VERBOSE("[CLOCK] enable MT_CG_DISP0_MUTEX_32K\n");
		cmdq_dev_enable_clock_MUTEX_32K(enable);
#endif
	} else {
		CMDQ_VERBOSE("[CLOCK] Disable SMI & LARB0 Clock\n");
		/* disable, reverse the sequence */
#ifdef CMDQ_CG_M4U_LARB0
		m4u_larb0_disable("CMDQ_MDP");
#else
		cmdq_dev_enable_clock_SMI_LARB0(enable);
#endif
		cmdq_dev_enable_clock_SMI_COMMON(enable);
#ifdef CMDQ_CG_DISP0_MUTEX_32K
		CMDQ_VERBOSE("[CLOCK] disable MT_CG_DISP0_MUTEX_32K\n");
		cmdq_dev_enable_clock_MUTEX_32K(enable);
#endif
	}
#endif				/* CMDQ_PWR_AWARE */
}

void cmdq_virtual_enable_gce_clock_locked(bool enable)
{
#ifdef CMDQ_PWR_AWARE
	if (enable) {
		CMDQ_VERBOSE("[CLOCK] Enable CMDQ(GCE) Clock\n");
		cmdq_dev_enable_gce_clock(enable);
	} else {
		CMDQ_VERBOSE("[CLOCK] Disable CMDQ(GCE) Clock\n");
		cmdq_dev_enable_gce_clock(enable);
	}
#endif
}

const char *cmdq_virtual_parse_error_module_by_hwflag_impl(const TaskStruct *pTask)
{
	const char *module = "CMDQ";
	return module;
}

/**
 * Debug
 *
 */
void cmdq_virtual_dump_mmsys_config(void)
{
	/* do nothing */
}

void cmdq_virtual_dump_clock_gating(void)
{
	/* do nothing */
}

int cmdq_virtual_dump_smi(const int showSmiDump)
{
	int isSMIHang = 0;

#if defined(CMDQ_CONFIG_SMI) && !defined(CONFIG_MTK_FPGA)
	isSMIHang =
	    smi_debug_bus_hanging_detect_ext(SMI_DBG_DISPSYS | SMI_DBG_VDEC | SMI_DBG_IMGSYS |
					     SMI_DBG_VENC | SMI_DBG_MJC, showSmiDump, 1);
	CMDQ_ERR("SMI Hang? = %d\n", isSMIHang);
#else
	CMDQ_LOG("[WARNING]not enable SMI dump now\n");
#endif

	return isSMIHang;
}

void cmdq_virtual_dump_gpr(void)
{
	int i = 0;
	long offset = 0;
	uint32_t value = 0;

	CMDQ_LOG("========= GPR dump =========\n");
	for (i = 0; i < 16; i++) {
		offset = CMDQ_GPR_R32(i);
		value = CMDQ_REG_GET32(offset);
		CMDQ_LOG("[GPR %2d]+0x%lx = 0x%08x\n", i, offset, value);
	}
	CMDQ_LOG("========= GPR dump =========\n");
}


/**
 * Record usage
 *
 */

uint64_t cmdq_virtual_flag_from_scenario(CMDQ_SCENARIO_ENUM scn)
{
	return 0LL;
}

/**
 * Event backup
 *
 */
void cmdq_virtual_initial_backup_event(void)
{
	/* do nothing */
}

void cmdq_virtual_event_backup(void)
{
	/* do nothing */
}

void cmdq_virtual_event_restore(void)
{
	/* do nothing */
}

/**
 * Test
 *
 */
void cmdq_virtual_test_setup(void)
{
	/* unconditionally set CMDQ_SYNC_TOKEN_CONFIG_ALLOW and mutex STREAM_DONE */
	/* so that DISPSYS scenarios may pass check. */
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_STREAM_EOF);
	cmdqCoreSetEvent(CMDQ_EVENT_MUTEX0_STREAM_EOF);
	cmdqCoreSetEvent(CMDQ_EVENT_MUTEX1_STREAM_EOF);
	cmdqCoreSetEvent(CMDQ_EVENT_MUTEX2_STREAM_EOF);
	cmdqCoreSetEvent(CMDQ_EVENT_MUTEX3_STREAM_EOF);
}

void cmdq_virtual_test_cleanup(void)
{
	/* do nothing */
}

void cmdq_virtual_init_module_PA_stat(void)
{
	/* do nothing */
}

void cmdq_virtual_function_setting(void)
{
	cmdqCoreFuncStruct *pFunc;

	pFunc = &(gFunctionPointer);

	/*
	 * GCE capability
	 */
	pFunc->syncNonSuspendable = cmdq_virtual_support_sync_non_suspendable;
	pFunc->waitAndReceiveEvent = cmdq_virtual_support_wait_and_receive_event_in_same_tick;
	pFunc->getSubsysLSBArgA = cmdq_virtual_get_subsys_LSB_in_argA;
	pFunc->subsysPA = cmdq_virtual_subsys_from_phys_addr;

	/* HW thread related */
	pFunc->isSecureThread = cmdq_virtual_is_a_secure_thread;
	pFunc->isValidNotifyThread = cmdq_virtual_is_valid_notify_thread_for_secure_path;

	/**
	 * Scenario related
	 *
	 */
	pFunc->isDispScenario = cmdq_virtual_is_disp_scenario;
	pFunc->shouldEnablePrefetch = cmdq_virtual_should_enable_prefetch;
	pFunc->shouldProfile = cmdq_virtual_should_profile;
	pFunc->dispThread = cmdq_virtual_disp_thread;
	pFunc->getThreadID = cmdq_virtual_get_thread_index;
	pFunc->priority = cmdq_virtual_priority_from_scenario;
	pFunc->forceLoopIRQ = cmdq_virtual_force_loop_irq;
	pFunc->isLoopScenario = cmdq_virtual_is_loop_scenario;

	/**
	 * Module dependent
	 *
	 */
	pFunc->getRegID = cmdq_virtual_get_reg_id_from_hwflag;
	pFunc->moduleFromEvent = cmdq_virtual_module_from_event_id;
	pFunc->parseModule = cmdq_virtual_parse_module_from_reg_addr;
	pFunc->moduleEntrySuspend = cmdq_virtual_can_module_entry_suspend;
	pFunc->printStatusClock = cmdq_virtual_print_status_clock;
	pFunc->printStatusSeqClock = cmdq_virtual_print_status_seq_clock;
	pFunc->enableCommonClockLocked = cmdq_virtual_enable_common_clock_locked;
	pFunc->enableGCEClockLocked = cmdq_virtual_enable_gce_clock_locked;
	pFunc->parseErrorModule = cmdq_virtual_parse_error_module_by_hwflag_impl;

	/**
	 * Debug
	 *
	 */
	pFunc->dumpMMSYSConfig = cmdq_virtual_dump_mmsys_config;
	pFunc->dumpClockGating = cmdq_virtual_dump_clock_gating;
	pFunc->dumpSMI = cmdq_virtual_dump_smi;
	pFunc->dumpGPR = cmdq_virtual_dump_gpr;

	/**
	 * Record usage
	 *
	 */
	pFunc->flagFromScenario = cmdq_virtual_flag_from_scenario;

	/**
	 * Event backup
	 *
	 */
	pFunc->initialBackupEvent = cmdq_virtual_initial_backup_event;
	pFunc->eventBackup = cmdq_virtual_event_backup;
	pFunc->eventRestore = cmdq_virtual_event_restore;

	/**
	 * Test
	 *
	 */
	pFunc->testSetup = cmdq_virtual_test_setup;
	pFunc->testCleanup = cmdq_virtual_test_cleanup;
	pFunc->initModulePAStat = cmdq_virtual_init_module_PA_stat;
}

cmdqCoreFuncStruct *cmdq_get_func(void)
{
	return &gFunctionPointer;
}
