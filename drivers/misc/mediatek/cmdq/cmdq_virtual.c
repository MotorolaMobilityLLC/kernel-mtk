#include "cmdq_core.h"
#include "cmdq_virtual.h"

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
	return false;
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

	CMDQ_ERR("unrecognized subsys, msb=0x%04x, physAddr:0x%08x\n", msb, physAddr);
	return subsysID;
#undef DECLARE_CMDQ_SUBSYS
}

/* HW thread related */
const bool cmdq_virtual_is_a_secure_thread(const int32_t thread)
{
	return false;
}

const bool cmdq_virtual_is_valid_notify_thread_for_secure_path(const int32_t thread)
{
	return false;
}

/**
 * Scenario related
 *
 */
void cmdq_virtual_fix_command_scenario_for_user_space(cmdqCommandStruct *pCommand)
{
	/* do nothing */
}

bool cmdq_virtual_is_request_from_user_space(const CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_virtual_is_disp_scenario(const CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_virtual_should_enable_prefetch(CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_virtual_should_profile(CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

int cmdq_virtual_get_thread_index(CMDQ_SCENARIO_ENUM scenario, const bool secure)
{
	return CMDQ_INVALID_THREAD;
}

int cmdq_virtual_disp_thread(CMDQ_SCENARIO_ENUM scenario)
{
	return CMDQ_INVALID_THREAD;
}

CMDQ_HW_THREAD_PRIORITY_ENUM cmdq_virtual_priority_from_scenario(CMDQ_SCENARIO_ENUM scenario)
{
	return CMDQ_THR_PRIO_NORMAL;
}

bool cmdq_virtual_force_loop_irq(CMDQ_SCENARIO_ENUM scenario)
{
	return false;
}

bool cmdq_virtual_is_loop_scenario(CMDQ_SCENARIO_ENUM scenario, bool displayOnly)
{
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
	return 0;
}

void cmdq_virtual_print_status_seq_clock(struct seq_file *m)
{
	/* do nothing */
}

void cmdq_virtual_enable_common_clock_locked(bool enable)
{
	/* do nothing */
}

void cmdq_virtual_enable_gce_clock_locked(bool enable)
{
	/* do nothing */
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
	return 0;
}

void cmdq_virtual_dump_gpr(void)
{
	/* do nothing */
}

void cmdq_virtual_dump_secure_metadata(cmdqSecDataStruct *pSecData)
{
	/* do nothing */
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
	/* do nothing */
}

void cmdq_virtual_test_cleanup(void)
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
	pFunc->fixCommandScenarioUser = cmdq_virtual_fix_command_scenario_for_user_space;
	pFunc->isRequestUser = cmdq_virtual_is_request_from_user_space;
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
	pFunc->gceClockLocked = cmdq_virtual_enable_gce_clock_locked;
	pFunc->parseErrorModule = cmdq_virtual_parse_error_module_by_hwflag_impl;

	/**
	 * Debug
	 *
	 */
	pFunc->dumpMMSYSConfig = cmdq_virtual_dump_mmsys_config;
	pFunc->dumpClockGating = cmdq_virtual_dump_clock_gating;
	pFunc->dumpSMI = cmdq_virtual_dump_smi;
	pFunc->dumpGPR = cmdq_virtual_dump_gpr;
	pFunc->dumpSecureMetadata = cmdq_virtual_dump_secure_metadata;

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
}

cmdqCoreFuncStruct *cmdq_get_func(void)
{
	return &gFunctionPointer;
}
