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

#include <linux/types.h>
#include <linux/uaccess.h>
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"
#include "disp_drv_log.h"
#include "disp_helper.h"
#include "disp_cmdq.h"
#include "primary_display.h"


/* display cmdq function state */
static int disp_cmdq_func_state[DISP_CMDQ_FUNC_NUM] = {
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_CREATE = 0, */
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_DESTROY, */
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_RESET, */
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_SET_SECURE, */
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY, */
	DISP_CMDQ_STATE_BASIC,	/* DISP_CMDQ_FUNC_ENABLE_DAPC, */
	DISP_CMDQ_STATE_CONFIG,	/* DISP_CMDQ_FUNC_WRITE_REG, */
	DISP_CMDQ_STATE_CONFIG, /* DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK, */
	DISP_CMDQ_STATE_DEBUG,	/* DISP_CMDQ_FUNC_WRITE_SLOT, */
	DISP_CMDQ_STATE_CONFIG,	/* DISP_CMDQ_FUNC_WRITE_REG_SECURE, */
	DISP_CMDQ_STATE_CONFIG,	/* DISP_CMDQ_FUNC_POLL_REG, */
	DISP_CMDQ_STATE_DEBUG,	/* DISP_CMDQ_FUNC_READ_REG_TO_SLOT, */
	DISP_CMDQ_STATE_EVENT,	/* DISP_CMDQ_FUNC_WAIT_EVENT, */
	DISP_CMDQ_STATE_EVENT,	/* DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR, */
	DISP_CMDQ_STATE_EVENT,	/* DISP_CMDQ_FUNC_CLEAR_EVENT, */
	DISP_CMDQ_STATE_EVENT,	/* DISP_CMDQ_FUNC_SET_EVENT, */
	DISP_CMDQ_STATE_FLUSH,	/* DISP_CMDQ_FUNC_FLUSH, */
	DISP_CMDQ_STATE_FLUSH,	/* DISP_CMDQ_FUNC_FLUSH_ASYNC, */
	DISP_CMDQ_STATE_FLUSH,	/* DISP_CMDQ_FUNC_FLUSH_ASYNC_CALLBACK, */
	DISP_CMDQ_STATE_FLUSH,	/* DISP_CMDQ_FUNC_START_LOOP, */
	DISP_CMDQ_STATE_FLUSH,	/* DISP_CMDQ_FUNC_STOP_LOOP, */
	DISP_CMDQ_STATE_RESOURCE,	/* DISP_CMDQ_FUNC_ACQUIRE_RESOURCE, */
	DISP_CMDQ_STATE_RESOURCE,	/* DISP_CMDQ_FUNC_RELEASE_RESOURCE, */
	DISP_CMDQ_STATE_RESOURCE,	/* DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE, */
	DISP_CMDQ_STATE_DEBUG,	/* DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT, */
	DISP_CMDQ_STATE_DEBUG,	/* DISP_CMDQ_FUNC_DUMP_COMMAND, */
};

static int disp_cmdq_thread_check[DISP_CMDQ_THREAD_NUM];
static char disp_cmdq_thread_caller[DISP_CMDQ_THREAD_NUM][DISP_CMDQ_THREAD_NAME_LENGTH];
static struct cmdqRecStruct *disp_cmdq_thread_map[DISP_CMDQ_THREAD_NUM];
static int disp_cmdq_state_current[DISP_CMDQ_THREAD_NUM];
static int disp_cmdq_state_stack[DISP_CMDQ_THREAD_NUM][DISP_CMDQ_STATE_STACK_NUM];
static int disp_cmdq_instruction_count[DISP_CMDQ_THREAD_NUM][DISP_CMDQ_FUNC_NUM];


static char *_disp_cmdq_get_state_name(enum DISP_CMDQ_STATE state)
{
	switch (state) {
	case DISP_CMDQ_STATE_BASIC:
		return "basic";
	case DISP_CMDQ_STATE_CONFIG:
		return "config";
	case DISP_CMDQ_STATE_EVENT:
		return "event";
	case DISP_CMDQ_STATE_FLUSH:
		return "flush";
	case DISP_CMDQ_STATE_RESOURCE:
		return "resource";
	case DISP_CMDQ_STATE_DEBUG:
		return "debug";
	case DISP_CMDQ_STATE_UNKNOWN:
		return "unknown";
	default:
		DISPERR("invalid state=%d\n", state);
		return "unknown";
	}
}

static char *_disp_cmdq_get_function_name(enum DISP_CMDQ_FUNC function)
{
	switch (function) {
	case DISP_CMDQ_FUNC_CREATE:
		return "create";
	case DISP_CMDQ_FUNC_DESTROY:
		return "destroy";
	case DISP_CMDQ_FUNC_RESET:
		return "reset";
	case DISP_CMDQ_FUNC_SET_SECURE:
		return "set_secure";
	case DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY:
		return "enable_port_security";
	case DISP_CMDQ_FUNC_ENABLE_DAPC:
		return "enable_dapc";
	case DISP_CMDQ_FUNC_WRITE_REG:
		return "write_reg";
	case DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK:
		return "write_reg_with_mask";
	case DISP_CMDQ_FUNC_WRITE_SLOT:
		return "write_slot";
	case DISP_CMDQ_FUNC_WRITE_REG_SECURE:
		return "write_reg_secure";
	case DISP_CMDQ_FUNC_POLL_REG:
		return "poll_reg";
	case DISP_CMDQ_FUNC_READ_REG_TO_SLOT:
		return "read_reg_to_slot";
	case DISP_CMDQ_FUNC_WAIT_EVENT:
		return "wait_event";
	case DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR:
		return "wait_event_no_clear";
	case DISP_CMDQ_FUNC_CLEAR_EVENT:
		return "clear_event";
	case DISP_CMDQ_FUNC_SET_EVENT:
		return "set_event";
	case DISP_CMDQ_FUNC_FLUSH:
		return "flush";
	case DISP_CMDQ_FUNC_FLUSH_ASYNC:
		return "flush_async";
	case DISP_CMDQ_FUNC_FLUSH_ASYNC_CALLBACK:
		return "flush_async_callback";
	case DISP_CMDQ_FUNC_START_LOOP:
		return "start_loop";
	case DISP_CMDQ_FUNC_STOP_LOOP:
		return "stop_loop";
	case DISP_CMDQ_FUNC_ACQUIRE_RESOURCE:
		return "acquire_resource";
	case DISP_CMDQ_FUNC_RELEASE_RESOURCE:
		return "release_resource";
	case DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE:
		return "write_for_resource";
	case DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT:
		return "get_instruction_count";
	case DISP_CMDQ_FUNC_DUMP_COMMAND:
		return "dump_command";
	default:
		DISPERR("invalid function=%d\n", function);
		return "unknown";
	}
}

static int _disp_cmdq_get_function(enum DISP_CMDQ_INSTRUCTION instruction)
{
	switch (instruction) {
	case DISP_CMDQ_INSTRUCTION_WRITE_REG:
		return DISP_CMDQ_FUNC_WRITE_REG;
	case DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK:
		return DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK;
	case DISP_CMDQ_INSTRUCTION_WRITE_SLOT:
		return DISP_CMDQ_FUNC_WRITE_SLOT;
	case DISP_CMDQ_INSTRUCTION_WRITE_REG_SECURE:
		return DISP_CMDQ_FUNC_WRITE_REG_SECURE;
	case DISP_CMDQ_INSTRUCTION_POLL_REG:
		return DISP_CMDQ_FUNC_POLL_REG;
	case DISP_CMDQ_INSTRUCTION_READ_REG_TO_SLOT:
		return DISP_CMDQ_FUNC_READ_REG_TO_SLOT;
	default:
		DISPERR("invalid instruction=%d\n", instruction);
		return -1;
	}
}

static int _disp_cmdq_find_index_by_handle(struct cmdqRecStruct *handle)
{
	int index = 0;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* handle is NULL */
	if (handle == NULL) {
		DISPERR("DISP CMDQ handle is NULL!!\n");
		return -1;
	}

	index = 0;
	while (disp_cmdq_thread_map[index] != handle) {
		index++;
		if (index >= DISP_CMDQ_THREAD_NUM) {
			DISPERR("DISP CMDQ find handle failed:%p\n", handle);
			break;
		}
	}

	return index;
}

static int _disp_cmdq_dump_state(int index, const char *func, int line)
{
	int i;
	int state, total;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	total = disp_cmdq_state_current[index];
	DISPERR("%s:%d, dump state total:%d\n", func, line, total);
	for (i = 0; i < total; i++) {
		state = disp_cmdq_state_stack[index][i];
		DISPERR("%d:%s\n", i, _disp_cmdq_get_state_name(state));
	}

	return 0;
}

static int _disp_cmdq_dump_instruction_count(int index, const char *func, int line)
{
	int i;
	int total = 0;
	int count = 0;
	int type = 0;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	DISPDBG("%s:%d, dump instruction count\n", func, line);
	for (i = 0; i < DISP_CMDQ_FUNC_NUM; i++) {
		count = disp_cmdq_instruction_count[index][i];
		if (count) {
			type++;
			total = total + count;
			DISPDBG("%s:%d\n", _disp_cmdq_get_function_name(i), count);
		}
	}
	DISPDBG("total:%d,%d\n", type, total);

	return total;
}

static int _disp_cmdq_insert_state(int index, int state)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* no need to check for certain state */
	switch (state) {
	case DISP_CMDQ_STATE_RESOURCE:
	case DISP_CMDQ_STATE_DEBUG:
	case DISP_CMDQ_STATE_UNKNOWN:
		return 0;
	}
	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	/* skip the same state */
	if (disp_cmdq_state_current[index] > 0) {
		if (disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1] == state)
			return 0;
	}
	/* state overflow */
	if (disp_cmdq_state_current[index] >= (DISP_CMDQ_STATE_STACK_NUM - 1)) {
		DISPERR("DISP CMDQ state overflow:%d, %d, from %s\n", index, state, disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, __func__, __LINE__);
		return -1;
	}

	disp_cmdq_state_stack[index][disp_cmdq_state_current[index]] = state;
	disp_cmdq_state_current[index]++;

	return 0;
}

static int _disp_cmdq_insert_instruction_count(int index, enum DISP_CMDQ_FUNC function)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	disp_cmdq_instruction_count[index][function]++;

	return 0;
}

static int _disp_cmdq_remove_all_state(int index)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	memset(disp_cmdq_state_stack[index], -1, sizeof(int) * DISP_CMDQ_STATE_STACK_NUM);
	disp_cmdq_state_current[index] = 0;
	disp_cmdq_thread_check[index] = DISP_CMDQ_CHECK_NORMAL;

	return 0;
}

static int _disp_cmdq_reset_instruction_count(int index)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	memset(disp_cmdq_instruction_count[index], 0, sizeof(int) * DISP_CMDQ_FUNC_NUM);

	return 0;
}

static int _disp_cmdq_get_instruction_count(int index, enum DISP_CMDQ_INSTRUCTION inst)
{
	int count = 0;
	int func_index;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;
	if (inst >= DISP_CMDQ_INSTRUCTION_NUM)
		return -1;

	func_index = _disp_cmdq_get_function(inst);
	if ((func_index < 0) || (func_index >= DISP_CMDQ_FUNC_NUM))
		DISPERR("invalid instruction=%d,%d\n", inst, func_index);
	count = disp_cmdq_instruction_count[index][func_index];

	return count;
}

static int _disp_cmdq_check_state(int index, const char *func, int line)
{
	/* int i; */

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	/* check state */
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_BYPASS) {
		/* check bypass */
		return 0;
	}
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_ERROR) {
		DISPERR("%s:%d, thread error!! from %s\n", func, line,
			disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][0] != DISP_CMDQ_STATE_BASIC) {
		DISPERR("%s:%d, state0 error:%d, from %s\n", func, line,
			disp_cmdq_state_stack[index][0], disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][1] != DISP_CMDQ_STATE_EVENT) {
		DISPERR("%s:%d, state1 error:%d, from %s\n", func, line,
			disp_cmdq_state_stack[index][1], disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1] !=
	    DISP_CMDQ_STATE_FLUSH) {
		DISPERR("%s:%d, state%d error:%d, from %s\n", func, line,
			disp_cmdq_state_current[index] - 1,
			disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1],
			disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, func, line);
		return -1;
	}
	/* for (i = 2; i < disp_cmdq_state_current[index]; i++) { */
	/*      if (disp_cmdq_state_current[i] == DISP_CMDQ_STATE_CONFIG) */
	/*              break; */
	/*      i++; */
	/* } */
	/* if (i == (disp_cmdq_state_current[index]-1)) { */
	/*      DISPERR("%s:%d, state no config error!! from %s\n", func, line, disp_cmdq_thread_caller[index]); */
	/*      _disp_cmdq_dump_state(index, func, line); */
	/*      return -1; */
	/* } */

	return 0;
}

static int _disp_cmdq_check_lock(int index, const char *func, int line)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	/* check state */
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_BYPASS) {
		/* check bypass */
		return 0;
	}
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_ERROR) {
		DISPERR("%s:%d, thread error!! from %s\n", func, line,
			disp_cmdq_thread_caller[index]);
		_disp_cmdq_dump_state(index, __func__, __LINE__);
		return -1;
	}
	/* no lock before access */
	if (primary_display_get_lock_id() == NULL) {
		DISPERR("%s:%d, no lock before access:%d, from %s\n", func, line, index,
			disp_cmdq_thread_caller[index]);
		return -1;
	}

	return 0;
}

static int _disp_cmdq_set_check_state(int index, int state)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* index overflow */
	if ((index < 0) || (index >= DISP_CMDQ_THREAD_NUM))
		return -1;

	disp_cmdq_thread_check[index] = state;

	return 0;
}

char *disp_cmdq_get_event_name(enum CMDQ_EVENT_ENUM event)
{
	switch (event) {
	/* Display start frame */
	case CMDQ_EVENT_DISP_OVL0_SOF:
		return "event_disp_ovl0_sof";
	case CMDQ_EVENT_DISP_OVL1_SOF:
		return "event_disp_ovl1_sof";
	case CMDQ_EVENT_DISP_2L_OVL0_SOF:
		return "event_disp_ovl0_2l_sof";
	case CMDQ_EVENT_DISP_2L_OVL1_SOF:
		return "event_disp_ovl1_2l_sof";
	case CMDQ_EVENT_DISP_RDMA0_SOF:
		return "event_disp_rdma0_sof";
	case CMDQ_EVENT_DISP_RDMA1_SOF:
		return "event_disp_rdma1_sof";
	case CMDQ_EVENT_DISP_RDMA2_SOF:
		return "event_disp_rdma2_sof";
	case CMDQ_EVENT_DISP_WDMA0_SOF:
		return "event_disp_wdma0_sof";
	case CMDQ_EVENT_DISP_WDMA1_SOF:
		return "event_disp_wdma1_sof";
	case CMDQ_EVENT_DISP_COLOR_SOF:
		return "event_disp_color_sof";
	case CMDQ_EVENT_DISP_COLOR0_SOF:
		return "event_disp_color0_sof";
	case CMDQ_EVENT_DISP_COLOR1_SOF:
		return "event_disp_color1_sof";
	case CMDQ_EVENT_DISP_CCORR_SOF:
		return "event_disp_ccorr_sof";
	case CMDQ_EVENT_DISP_CCORR0_SOF:
		return "event_disp_ccorr0_sof";
	case CMDQ_EVENT_DISP_CCORR1_SOF:
		return "event_disp_ccorr1_sof";
	case CMDQ_EVENT_DISP_AAL_SOF:
		return "event_disp_aal_sof";
	case CMDQ_EVENT_DISP_AAL0_SOF:
		return "event_disp_aal0_sof";
	case CMDQ_EVENT_DISP_AAL1_SOF:
		return "event_disp_aal1_sof";
	case CMDQ_EVENT_DISP_GAMMA_SOF:
		return "event_disp_gamma_sof";
	case CMDQ_EVENT_DISP_GAMMA0_SOF:
		return "event_disp_gamma0_sof";
	case CMDQ_EVENT_DISP_GAMMA1_SOF:
		return "event_disp_gamma1_sof";
	case CMDQ_EVENT_DISP_DITHER_SOF:
		return "event_disp_dither_sof";
	case CMDQ_EVENT_DISP_DITHER0_SOF:
		return "event_disp_dither0_sof";
	case CMDQ_EVENT_DISP_DITHER1_SOF:
		return "event_disp_dither1_sof";
	case CMDQ_EVENT_DISP_UFOE_SOF:
		return "event_disp_ufoe_sof";
	case CMDQ_EVENT_DISP_PWM0_SOF:
		return "event_disp_pwm0_sof";
	case CMDQ_EVENT_DISP_PWM1_SOF:
		return "event_disp_pwm1_sof";
	case CMDQ_EVENT_DISP_OD_SOF:
		return "event_disp_od_sof";
	case CMDQ_EVENT_DISP_DSC_SOF:
		return "event_disp_dsc_sof";

	/* Display frame done */
	case CMDQ_EVENT_DISP_OVL0_EOF:
		return "event_disp_ovl0_eof";
	case CMDQ_EVENT_DISP_OVL1_EOF:
		return "event_disp_ovl1_eof";
	case CMDQ_EVENT_DISP_2L_OVL0_EOF:
		return "event_disp_ovl0_2l_eof";
	case CMDQ_EVENT_DISP_2L_OVL1_EOF:
		return "event_disp_ovl1_2l_eof";
	case CMDQ_EVENT_DISP_RDMA0_EOF:
		return "event_disp_rdma0_eof";
	case CMDQ_EVENT_DISP_RDMA1_EOF:
		return "event_disp_rdma1_eof";
	case CMDQ_EVENT_DISP_RDMA2_EOF:
		return "event_disp_rdma2_eof";
	case CMDQ_EVENT_DISP_WDMA0_EOF:
		return "event_disp_wdma0_eof";
	case CMDQ_EVENT_DISP_WDMA1_EOF:
		return "event_disp_wdma1_eof";
	case CMDQ_EVENT_DISP_COLOR_EOF:
		return "event_disp_color_eof";
	case CMDQ_EVENT_DISP_COLOR0_EOF:
		return "event_disp_color0_eof";
	case CMDQ_EVENT_DISP_COLOR1_EOF:
		return "event_disp_color1_eof";
	case CMDQ_EVENT_DISP_CCORR_EOF:
		return "event_disp_ccorr_eof";
	case CMDQ_EVENT_DISP_CCORR0_EOF:
		return "event_disp_ccorr0_eof";
	case CMDQ_EVENT_DISP_CCORR1_EOF:
		return "event_disp_ccorr1_eof";
	case CMDQ_EVENT_DISP_AAL_EOF:
		return "event_disp_aal_eof";
	case CMDQ_EVENT_DISP_AAL0_EOF:
		return "event_disp_aal0_eof";
	case CMDQ_EVENT_DISP_AAL1_EOF:
		return "event_disp_aal1_eof";
	case CMDQ_EVENT_DISP_GAMMA_EOF:
		return "event_disp_gamma_eof";
	case CMDQ_EVENT_DISP_GAMMA0_EOF:
		return "event_disp_gamm0_eof";
	case CMDQ_EVENT_DISP_GAMMA1_EOF:
		return "event_disp_gamma1_eof";
	case CMDQ_EVENT_DISP_DITHER_EOF:
		return "event_disp_dither_eof";
	case CMDQ_EVENT_DISP_DITHER0_EOF:
		return "event_disp_dither0_eof";
	case CMDQ_EVENT_DISP_DITHER1_EOF:
		return "event_disp_dither1_eof";
	case CMDQ_EVENT_DISP_UFOE_EOF:
		return "event_disp_ufoe_eof";
	case CMDQ_EVENT_DISP_OD_EOF:
		return "event_disp_od_eof";
	case CMDQ_EVENT_DISP_OD_RDMA_EOF:
		return "event_disp_od_rdma_eof";
	case CMDQ_EVENT_DISP_OD_WDMA_EOF:
		return "event_disp_od_wdma_eof";
	case CMDQ_EVENT_DISP_DSC_EOF:
		return "event_disp_dsc_eof";
	case CMDQ_EVENT_DISP_DSI0_EOF:
		return "event_disp_dsi0_eof";
	case CMDQ_EVENT_DISP_DSI1_EOF:
		return "event_disp_dsi1_eof";
	case CMDQ_EVENT_DISP_DPI0_EOF:
		return "event_disp_dpi0_eof";

	/* Mutex frame done */
	case CMDQ_EVENT_MUTEX0_STREAM_EOF:
		return "event_mutex0_stream_eof";
	case CMDQ_EVENT_MUTEX1_STREAM_EOF:
		return "event_mutex1_stream_eof";
	case CMDQ_EVENT_MUTEX2_STREAM_EOF:
		return "event_mutex2_stream_eof";
	case CMDQ_EVENT_MUTEX3_STREAM_EOF:
		return "event_mutex3_stream_eof";
	case CMDQ_EVENT_MUTEX4_STREAM_EOF:
		return "event_mutex4_stream_eof";

	/* Display underrun */
	case CMDQ_EVENT_DISP_RDMA0_UNDERRUN:
		return "event_disp_rdma0_underrun";
	case CMDQ_EVENT_DISP_RDMA1_UNDERRUN:
		return "event_disp_rdma1_underrun";
	case CMDQ_EVENT_DISP_RDMA2_UNDERRUN:
		return "event_disp_rdma2_underrun";

	/* Display TE */
	case CMDQ_EVENT_DSI_TE:
		return "event_dsi_te";
	case CMDQ_EVENT_DSI0_TE:
		return "event_dsi0_te";
	case CMDQ_EVENT_DSI1_TE:
		return "event_dsi1_te";
	case CMDQ_EVENT_DISP_DSI0_SOF:
		return "event_disp_dsi0_sof";
	case CMDQ_EVENT_DISP_DSI1_SOF:
		return "event_disp_dsi1_sof";

	/* Reset Event */
	case CMDQ_EVENT_DISP_WDMA0_RST_DONE:
		return "event_disp_wdma0_rst_done";
	case CMDQ_EVENT_DISP_WDMA1_RST_DONE:
		return "event_disp_wdma1_rst_done";

	/* 6799 New Event */
	case CMDQ_EVENT_DISP_DSC1_SOF:
		return "event_disp_dsc1_sof";
	case CMDQ_EVENT_DISP_DSC2_SOF:
		return "event_disp_dsc2_sof";
	case CMDQ_EVENT_DISP_RSZ0_SOF:
		return "event_disp_rsz0_sof";
	case CMDQ_EVENT_DISP_RSZ1_SOF:
		return "event_disp_rsz1_sof";
	case CMDQ_EVENT_DISP_DSC0_EOF:
		return "event_disp_dsc0_eof";
	case CMDQ_EVENT_DISP_DSC1_EOF:
		return "event_disp_dsc1_eof";
	case CMDQ_EVENT_DISP_RSZ0_EOF:
		return "event_disp_rez0_eof";
	case CMDQ_EVENT_DISP_RSZ1_EOF:
		return "event_disp_rsz1_eof";
	case CMDQ_EVENT_DISP_OVL0_RST_DONE:
		return "event_disp_ovl0_rst_done";
	case CMDQ_EVENT_DISP_OVL1_RST_DONE:
		return "event_disp_ovl1_rst_done";
	case CMDQ_EVENT_DISP_OVL0_2L_RST_DONE:
		return "event_disp_ovl0_2l_rst_done";
	case CMDQ_EVENT_DISP_OVL1_2L_RST_DONE:
		return "event_disp_ovl1_2l_rst_done";

	/* SW Sync Tokens (Pre-defined) */
	case CMDQ_SYNC_TOKEN_CONFIG_DIRTY:
		return "sync_token_config_dirty";
	case CMDQ_SYNC_TOKEN_STREAM_EOF:
		return "sync_token_stream_eof";
	case CMDQ_SYNC_TOKEN_ESD_EOF:
		return "sync_token_esd_eof";
	case CMDQ_SYNC_TOKEN_CABC_EOF:
		return "sync_token_cabc_eof";

	/* Secure video path notify SW token */
	case CMDQ_SYNC_DISP_OVL0_2NONSEC_END:
		return "sync_disp_ovl0_2nonsec_end";
	case CMDQ_SYNC_DISP_OVL1_2NONSEC_END:
		return "sync_disp_ovl1_2nonsec_end";
	case CMDQ_SYNC_DISP_2LOVL0_2NONSEC_END:
		return "sync_disp_ovl0_2l_2nonsec_end";
	case CMDQ_SYNC_DISP_2LOVL1_2NONSEC_END:
		return "sync_disp_ovl1_2l_2nonsec_end";
	case CMDQ_SYNC_DISP_RDMA0_2NONSEC_END:
		return "sync_disp_rdma0_2nonsec_end";
	case CMDQ_SYNC_DISP_RDMA1_2NONSEC_END:
		return "sync_disp_rdma1_2nonsec_end";
	case CMDQ_SYNC_DISP_WDMA0_2NONSEC_END:
		return "sync_disp_wdma0_2nonsec_end";
	case CMDQ_SYNC_DISP_WDMA1_2NONSEC_END:
		return "sync_disp_wdma1_2nonsec_end";
	case CMDQ_SYNC_DISP_EXT_STREAM_EOF:
		return "sync_disp_ext_stream_eof";

	/* Resource lock event to control resource in GCE thread */
	case CMDQ_SYNC_RESOURCE_WROT0:
		return "sync_resource_wrot0";
	case CMDQ_SYNC_RESOURCE_WROT1:
		return "sync_resource_wrot1";

	default:
		DISPERR("invalid event=%d\n", event);
		return "unknown";
	}
}


int disp_cmdq_init(void)
{
	int i;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	memset(disp_cmdq_thread_check, DISP_CMDQ_CHECK_NORMAL, sizeof(int) * DISP_CMDQ_THREAD_NUM);
	memset(disp_cmdq_thread_map, 0x0, sizeof(struct cmdqRecStruct *) * DISP_CMDQ_THREAD_NUM);
	for (i = 0; i < DISP_CMDQ_THREAD_NUM; i++)
		disp_cmdq_thread_caller[i][0] = '\0';

	memset(disp_cmdq_state_current, -1, sizeof(int) * DISP_CMDQ_THREAD_NUM);
	memset(disp_cmdq_state_stack, -1,
	       sizeof(int) * DISP_CMDQ_THREAD_NUM * DISP_CMDQ_STATE_STACK_NUM);
	memset(disp_cmdq_instruction_count, 0,
	       sizeof(int) * DISP_CMDQ_THREAD_NUM * DISP_CMDQ_FUNC_NUM);

	return 0;
}

int disp_cmdq_create(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle, const char *func)
{
	int index;
	int ret;

	ret = cmdqRecCreate(scenario, pHandle);

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return ret;

	/* cmdq create fail */
	if (ret) {
		DISPERR("DISP CMDQ create failed:%d, %d\n", ret, scenario);
		return ret;
	}

	index = 0;
	while (disp_cmdq_thread_map[index] != NULL) {
		index++;
		if (index >= DISP_CMDQ_THREAD_NUM) {
			DISPERR("DISP CMDQ create thread overflow:%d, %p\n", scenario, *pHandle);
			break;
		}
	}
	/* index overflow */
	if (index >= DISP_CMDQ_THREAD_NUM)
		return ret;

	disp_cmdq_thread_check[index] = DISP_CMDQ_CHECK_NORMAL;
	strncpy(disp_cmdq_thread_caller[index], func, sizeof(char) * DISP_CMDQ_THREAD_NAME_LENGTH);
	disp_cmdq_thread_map[index] = *pHandle;
	disp_cmdq_state_current[index] = 0;
	memset(disp_cmdq_state_stack[index], -1, sizeof(int) * DISP_CMDQ_STATE_STACK_NUM);
	memset(disp_cmdq_instruction_count[index], 0, sizeof(int) * DISP_CMDQ_FUNC_NUM);

	return ret;
}

int disp_cmdq_destroy(struct cmdqRecStruct *handle, const char *func, int line)
{
	int index;

	cmdqRecDestroy(handle);

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	index = 0;
	while (disp_cmdq_thread_map[index] != handle) {
		index++;
		if (index >= DISP_CMDQ_THREAD_NUM) {
			DISPERR("DISP CMDQ find handle failed:%p\n", handle);
			break;
		}
	}
	/* index overflow */
	if (index >= DISP_CMDQ_THREAD_NUM)
		return -1;
	/* check state */
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_BYPASS) {
		/* check bypass */
		goto done;
	}
	if (disp_cmdq_state_current[index] != 0) {
		DISPERR("DISP CMDQ destroy not clean:%d, from %s\n", disp_cmdq_state_current[index],
			disp_cmdq_thread_caller[index]);
		goto done;
	}

done:
	disp_cmdq_thread_map[index] = NULL;
	disp_cmdq_thread_caller[index][0] = '\0';
	disp_cmdq_thread_check[index] = DISP_CMDQ_CHECK_NORMAL;

	return 0;
}

int disp_cmdq_set_check_state(struct cmdqRecStruct *handle, enum DISP_CMDQ_CHECK state)
{
	int ret;
	int index;

	/* index overflow */
	if (state >= DISP_CMDQ_CHECK_NUM)
		return -1;

	index = _disp_cmdq_find_index_by_handle(handle);
	ret = _disp_cmdq_set_check_state(index, state);

	return ret;
}

int disp_cmdq_reset(struct cmdqRecStruct *handle)
{
	int ret;
	int index;

	ret = cmdqRecReset(handle);

	/* cmdq reset fail */
	if (ret) {
		DISPERR("DISP CMDQ reset failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_remove_all_state(index);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_RESET]);
	_disp_cmdq_reset_instruction_count(index);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_RESET);

	return ret;
}

int disp_cmdq_set_secure(struct cmdqRecStruct *handle, const bool is_secure)
{
	int ret;
	int index;

	ret = cmdqRecSetSecure(handle, is_secure);

	/* cmdq set secure fail */
	if (ret) {
		DISPERR("DISP CMDQ set secure failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_SET_SECURE]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_SET_SECURE);

	return ret;
}

int disp_cmdq_enable_port_security(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	int ret;
	int index;

	ret = cmdqRecSecureEnablePortSecurity(handle, engineFlag);

	/* cmdq enable port security fail */
	if (ret) {
		DISPERR("DISP CMDQ enable port security failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY);

	return ret;
}

int disp_cmdq_enable_dapc(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	int ret;
	int index;

	ret = cmdqRecSecureEnableDAPC(handle, engineFlag);

	/* cmdq enable dapc fail */
	if (ret) {
		DISPERR("DISP CMDQ enable dapc failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_ENABLE_DAPC]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_ENABLE_DAPC);

	return ret;
}

int disp_cmdq_write_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;
	int index;

	ret = cmdqRecWrite(handle, addr, value, mask);

	/* cmdq write reg fail */
	if (ret) {
		DISPERR("DISP CMDQ write reg failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	if (mask == ((unsigned int)~0)) {
		_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG]);
		_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WRITE_REG);
	} else {
		_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK]);
		_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK);
	}

	return ret;
}

int disp_cmdq_write_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			 uint32_t slot_index, uint32_t value)
{
	int ret;
	int index;

	ret = cmdqRecBackupUpdateSlot(handle, h_backup_slot, slot_index, value);

	/* cmdq write slot fail */
	if (ret) {
		DISPERR("DISP CMDQ write slot failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_SLOT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WRITE_SLOT);

	return ret;
}

int disp_cmdq_write_reg_secure(struct cmdqRecStruct *handle, uint32_t addr,
			       enum CMDQ_SEC_ADDR_METADATA_TYPE type, uint32_t baseHandle,
			       uint32_t offset, uint32_t size, uint32_t port)
{
	int ret;
	int index;

	ret = cmdqRecWriteSecure(handle, addr, type, baseHandle, offset, size, port);

	/* cmdq write reg secure fail */
	if (ret) {
		DISPERR("DISP CMDQ write reg secure failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG_SECURE]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WRITE_REG_SECURE);

	return ret;
}

int disp_cmdq_poll_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;
	int index;

	ret = cmdqRecPoll(handle, addr, value, mask);

	/* cmdq poll reg fail */
	if (ret) {
		DISPERR("DISP CMDQ poll reg failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_POLL_REG]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_POLL_REG);

	return ret;
}

int disp_cmdq_read_reg_to_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			       uint32_t slot_index, uint32_t regAddr)
{
	int ret;
	int index;

	ret = cmdqRecBackupRegisterToSlot(handle, h_backup_slot, slot_index, regAddr);

	/* cmdq read reg to slot fail */
	if (ret) {
		DISPERR("DISP CMDQ read reg to slot failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_READ_REG_TO_SLOT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_READ_REG_TO_SLOT);

	return ret;
}

int disp_cmdq_wait_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;
	int index;

	ret = cmdqRecWait(handle, event);

	/* cmdq wait event fail */
	if (ret) {
		DISPERR("DISP CMDQ wait event failed:%d, %s\n", ret, disp_cmdq_get_event_name(event));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WAIT_EVENT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WAIT_EVENT);

	return ret;
}

int disp_cmdq_wait_event_no_clear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;
	int index;

	ret = cmdqRecWaitNoClear(handle, event);

	/* cmdq wait event no clear fail */
	if (ret) {
		DISPERR("DISP CMDQ wait event no clear failed:%d, %s\n", ret, disp_cmdq_get_event_name(event));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR);

	return ret;
}

int disp_cmdq_clear_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;
	int index = 0;

	ret = cmdqRecClearEventToken(handle, event);

	/* cmdq clear event fail */
	if (ret) {
		DISPERR("DISP CMDQ clear event failed:%d, %s\n", ret, disp_cmdq_get_event_name(event));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_CLEAR_EVENT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_CLEAR_EVENT);

	return ret;
}

int disp_cmdq_set_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;
	int index;

	ret = cmdqRecSetEventToken(handle, event);

	/* cmdq set event fail */
	if (ret) {
		DISPERR("DISP CMDQ set event failed:%d, %s\n", ret, disp_cmdq_get_event_name(event));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_check_lock(index, __func__, __LINE__);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_SET_EVENT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_SET_EVENT);

	return ret;
}

int disp_cmdq_flush(struct cmdqRecStruct *handle, const char *func, int line)
{
	int ret;
	int index;
	int count[3];

	index = _disp_cmdq_find_index_by_handle(handle);
	count[0] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlush(handle);

	/* cmdq flush fail */
	if (ret) {
		DISPERR("%s:%d, flush failed:%d\n", func, line, ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(index);
		_disp_cmdq_reset_instruction_count(index);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 index, ret);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH]);
	_disp_cmdq_check_state(index, func, line);
	_disp_cmdq_check_lock(index, func, line);
	_disp_cmdq_remove_all_state(index);
	_disp_cmdq_reset_instruction_count(index);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 index, ret);

	return ret;
}

int disp_cmdq_flush_async(struct cmdqRecStruct *handle, const char *func, int line)
{
	int ret;
	int index;
	int count[3];

	index = _disp_cmdq_find_index_by_handle(handle);
	count[0] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlushAsync(handle);

	/* cmdq flush async fail */
	if (ret) {
		DISPERR("%s:%d, flush async failed:%d\n", func, line, ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(index);
		_disp_cmdq_reset_instruction_count(index);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 index, ret);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH_ASYNC]);
	_disp_cmdq_check_state(index, func, line);
	_disp_cmdq_check_lock(index, func, line);
	_disp_cmdq_remove_all_state(index);
	_disp_cmdq_reset_instruction_count(index);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 index, ret);

	return ret;
}

int disp_cmdq_flush_async_callback(struct cmdqRecStruct *handle, CmdqAsyncFlushCB callback,
				   uint32_t userData, const char *func, int line)
{
	int ret;
	int index;
	int count[3];

	index = _disp_cmdq_find_index_by_handle(handle);
	count[0] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(index, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlushAsyncCallback(handle, callback, userData);

	/* cmdq flush async callback fail */
	if (ret) {
		DISPERR("%s:%d, flush async callback failed:%d\n", func, line, ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(index);
		_disp_cmdq_reset_instruction_count(index);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 index, ret);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH_ASYNC_CALLBACK]);
	_disp_cmdq_check_state(index, func, line);
	_disp_cmdq_check_lock(index, func, line);
	_disp_cmdq_remove_all_state(index);
	_disp_cmdq_reset_instruction_count(index);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 index, ret);

	return ret;
}

int disp_cmdq_start_loop(struct cmdqRecStruct *handle)
{
	int ret;
	int index;

	ret = cmdqRecStartLoop(handle);

	/* cmdq start loop fail */
	if (ret) {
		DISPERR("DISP CMDQ start loop failed:%d\n", ret);
		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_START_LOOP]);

	return ret;
}

int disp_cmdq_stop_loop(struct cmdqRecStruct *handle)
{
	int ret;
	int index;

	ret = cmdqRecStopLoop(handle);

	/* cmdq stop loop fail */
	if (ret) {
		DISPERR("DISP CMDQ stop loop failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_STOP_LOOP]);

	return ret;
}

int disp_cmdq_acquire_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	int ret;
	int index;

	ret = cmdqRecAcquireResource(handle, resourceEvent);

	/* cmdq acquire resource fail */
	if (ret) {
		DISPERR("DISP CMDQ acquire resource failed:%d, %s\n", ret, disp_cmdq_get_event_name(resourceEvent));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_ACQUIRE_RESOURCE]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_ACQUIRE_RESOURCE);

	return ret;
}

int disp_cmdq_release_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	int ret;
	int index;

	ret = cmdqRecReleaseResource(handle, resourceEvent);

	/* cmdq release resource fail */
	if (ret) {
		DISPERR("DISP CMDQ release resource failed:%d, %s\n", ret, disp_cmdq_get_event_name(resourceEvent));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_RELEASE_RESOURCE]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_RELEASE_RESOURCE);

	return ret;
}

int disp_cmdq_write_for_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
				 uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;
	int index;

	ret = cmdqRecWriteForResource(handle, resourceEvent, addr, value, mask);

	/* cmdq write for resource fail */
	if (ret) {
		DISPINFO("DISP CMDQ write for resource failed:%d, %s\n", ret, disp_cmdq_get_event_name(resourceEvent));

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE);

	return ret;
}

int disp_cmdq_get_instruction_count(struct cmdqRecStruct *handle)
{
	int ret;
	int index;

	ret = cmdqRecGetInstructionCount(handle);

	/* cmdq get instruction count fail */
	if (handle == NULL) {
		DISPERR("DISP CMDQ get instruction count failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	/* bypass debug operation */
	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_BYPASS);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT);

	return ret;
}

int disp_cmdq_dump_command(struct cmdqRecStruct *handle)
{
	int ret;
	int index;

	ret = cmdqRecDumpCommand(handle);

	/* cmdq dump command fail */
	if (ret) {
		DISPERR("DISP CMDQ dump command failed:%d\n", ret);

		index = _disp_cmdq_find_index_by_handle(handle);
		_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	/* bypass debug operation */
	index = _disp_cmdq_find_index_by_handle(handle);
	_disp_cmdq_set_check_state(index, DISP_CMDQ_CHECK_BYPASS);
	_disp_cmdq_insert_state(index, disp_cmdq_func_state[DISP_CMDQ_FUNC_DUMP_COMMAND]);
	_disp_cmdq_insert_instruction_count(index, DISP_CMDQ_FUNC_DUMP_COMMAND);

	return ret;
}

int disp_cmdq_insert_instruction_count(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot)
{
	int i;
	int index = 0;
	int total = 0;
	int count = 0;
	int func_index;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* handle is NULL */
	if (handle == NULL) {
		DISPERR("DISP CMDQ handle is NULL!!\n");
		return -1;
	}
	index = _disp_cmdq_find_index_by_handle(handle);
	/* index overflow */
	if (index >= DISP_CMDQ_THREAD_NUM)
		return -1;

	for (i = 0; i < DISP_CMDQ_INSTRUCTION_NUM; i++) {
		func_index = _disp_cmdq_get_function(i);
		if ((func_index < 0) || (func_index >= DISP_CMDQ_FUNC_NUM)) {
			DISPERR("invalid instruction=%d,%d\n", i, func_index);
			break;
		}
		count = disp_cmdq_instruction_count[index][func_index];
		total = total + count;
		disp_cmdq_write_slot(handle, h_backup_slot, i, count);
	}
	disp_cmdq_write_slot(handle, h_backup_slot, i, total);

	return 0;
}

int disp_cmdq_dump_instruction_count(struct cmdqRecStruct *handle, const char *func, int line)
{
	int ret;
	int index;

	index = _disp_cmdq_find_index_by_handle(handle);
	ret = _disp_cmdq_dump_instruction_count(index, func, line);

	return ret;
}

