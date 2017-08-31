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

static int _disp_cmdq_dump_state(struct cmdqRecStruct *handle, const char *func, int line)
{
	int i;
	int index = 0;
	int state, total;

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

	total = disp_cmdq_state_current[index];
	DISPERR("%s:%d, dump state total:%d\n", func, line, total);
	for (i = 0; i < total; i++) {
		state = disp_cmdq_state_stack[index][i];
		DISPERR("%d:%s\n", i, _disp_cmdq_get_state_name(state));
	}

	return 0;
}

static int _disp_cmdq_dump_instruction_count(struct cmdqRecStruct *handle, const char *func, int line)
{
	int i;
	int index = 0;
	int total = 0;
	int count = 0;
	int type = 0;

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

static int _disp_cmdq_insert_state(struct cmdqRecStruct *handle, int state)
{
	int index = 0;

	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	/* no need to check for certain state */
	switch (state) {
	case DISP_CMDQ_STATE_RESOURCE:
	case DISP_CMDQ_STATE_DEBUG:
	case DISP_CMDQ_STATE_UNKNOWN:
		return 0;
	}

	/* handle is NULL */
	if (handle == NULL) {
		DISPERR("DISP CMDQ handle is NULL!!\n");
		return -1;
	}
	index = _disp_cmdq_find_index_by_handle(handle);
	/* index overflow */
	if (index >= DISP_CMDQ_THREAD_NUM)
		return -1;
	/* skip the same state */
	if (disp_cmdq_state_current[index] > 0) {
		if (disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1] == state)
			return 0;
	}
	/* state overflow */
	if (disp_cmdq_state_current[index] >= (DISP_CMDQ_STATE_STACK_NUM - 1)) {
		DISPERR("DISP CMDQ state overflow:%p, %d\n", handle, state);
		_disp_cmdq_dump_state(handle, __func__, __LINE__);
		return -1;
	}

	disp_cmdq_state_stack[index][disp_cmdq_state_current[index]] = state;
	disp_cmdq_state_current[index]++;

	return 0;
}

static int _disp_cmdq_insert_instruction_count(struct cmdqRecStruct *handle, enum DISP_CMDQ_FUNC function)
{
	int index = 0;

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

	disp_cmdq_instruction_count[index][function]++;

	return 0;
}

static int _disp_cmdq_remove_all_state(struct cmdqRecStruct *handle)
{
	int index = 0;

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

	memset(disp_cmdq_state_stack[index], -1, sizeof(int) * DISP_CMDQ_STATE_STACK_NUM);
	disp_cmdq_state_current[index] = 0;
	disp_cmdq_thread_check[index] = DISP_CMDQ_CHECK_NORMAL;

	return 0;
}

static int _disp_cmdq_reset_instruction_count(struct cmdqRecStruct *handle)
{
	int index = 0;

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

	memset(disp_cmdq_instruction_count[index], 0, sizeof(int) * DISP_CMDQ_FUNC_NUM);

	return 0;
}

static int _disp_cmdq_get_instruction_count(struct cmdqRecStruct *handle, enum DISP_CMDQ_INSTRUCTION inst)
{
	int index = 0;
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
	if (inst >= DISP_CMDQ_INSTRUCTION_NUM)
		return -1;

	func_index = _disp_cmdq_get_function(inst);
	if ((func_index < 0) || (func_index >= DISP_CMDQ_FUNC_NUM))
		DISPERR("invalid instruction=%d,%d\n", inst, func_index);
	count = disp_cmdq_instruction_count[index][func_index];

	return count;
}

static int _disp_cmdq_check_state(struct cmdqRecStruct *handle, const char *func, int line)
{
	/* int i; */
	int index = 0;

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

	/* check state */
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_BYPASS) {
		/* check bypass */
		return 0;
	}
	if (disp_cmdq_thread_check[index] == DISP_CMDQ_CHECK_ERROR) {
		DISPERR("%s:%d, thread error!!\n", func, line);
		_disp_cmdq_dump_state(handle, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][0] != DISP_CMDQ_STATE_BASIC) {
		DISPERR("%s:%d, state0 error:%d\n", func, line, disp_cmdq_state_stack[index][0]);
		_disp_cmdq_dump_state(handle, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][1] != DISP_CMDQ_STATE_EVENT) {
		DISPERR("%s:%d, state1 error:%d\n", func, line, disp_cmdq_state_stack[index][1]);
		_disp_cmdq_dump_state(handle, func, line);
		return -1;
	}
	if (disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1] !=
	    DISP_CMDQ_STATE_FLUSH) {
		DISPERR("%s:%d, state%d error:%d\n", func, line, disp_cmdq_state_current[index] - 1,
			disp_cmdq_state_stack[index][disp_cmdq_state_current[index] - 1]);
		_disp_cmdq_dump_state(handle, func, line);
		return -1;
	}
	/* for (i = 2; i < disp_cmdq_state_current[index]; i++) { */
	/*      if (disp_cmdq_state_current[i] == DISP_CMDQ_STATE_CONFIG) */
	/*              break; */
	/*      i++; */
	/* } */
	/* if (i == (disp_cmdq_state_current[index]-1)) { */
	/*      DISPERR("%s:%d, state no config error!!\n", func, line); */
	/*      _disp_cmdq_dump_state(handle, func, line); */
	/*      return -1; */
	/* } */

	return 0;
}

static int _disp_cmdq_set_check_state(struct cmdqRecStruct *handle, int state)
{
	int index = 0;

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

	disp_cmdq_thread_check[index] = state;

	return 0;
}


int disp_cmdq_init(void)
{
	if (disp_helper_get_option(DISP_OPT_CHECK_CMDQ_COMMAND) == 0)
		return 0;

	memset(disp_cmdq_thread_check, DISP_CMDQ_CHECK_NORMAL, sizeof(int) * DISP_CMDQ_THREAD_NUM);
	memset(disp_cmdq_thread_map, 0x0, sizeof(struct cmdqRecStruct *) * DISP_CMDQ_THREAD_NUM);
	memset(disp_cmdq_state_current, -1, sizeof(int) * DISP_CMDQ_THREAD_NUM);
	memset(disp_cmdq_state_stack, -1,
	       sizeof(int) * DISP_CMDQ_THREAD_NUM * DISP_CMDQ_STATE_STACK_NUM);
	memset(disp_cmdq_instruction_count, 0,
	       sizeof(int) * DISP_CMDQ_THREAD_NUM * DISP_CMDQ_FUNC_NUM);

	return 0;
}

int disp_cmdq_create(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle)
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
		DISPERR("DISP CMDQ destroy not clean:%d\n", disp_cmdq_state_current[index]);
		goto done;
	}

done:
	disp_cmdq_thread_map[index] = NULL;
	disp_cmdq_thread_check[index] = DISP_CMDQ_CHECK_NORMAL;

	return 0;
}

int disp_cmdq_set_check_state(struct cmdqRecStruct *handle, enum DISP_CMDQ_CHECK state)
{
	int ret;

	/* index overflow */
	if (state >= DISP_CMDQ_CHECK_NUM)
		return -1;

	ret = _disp_cmdq_set_check_state(handle, state);

	return ret;
}

int disp_cmdq_reset(struct cmdqRecStruct *handle)
{
	int ret;

	ret = cmdqRecReset(handle);

	/* cmdq reset fail */
	if (ret) {
		DISPERR("DISP CMDQ reset failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_remove_all_state(handle);
	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_RESET]);
	_disp_cmdq_reset_instruction_count(handle);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_RESET);

	return ret;
}

int disp_cmdq_set_secure(struct cmdqRecStruct *handle, const bool is_secure)
{
	int ret;

	ret = cmdqRecSetSecure(handle, is_secure);

	/* cmdq set secure fail */
	if (ret) {
		DISPERR("DISP CMDQ set secure failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_SET_SECURE]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_SET_SECURE);

	return ret;
}

int disp_cmdq_enable_port_security(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	int ret;

	ret = cmdqRecSecureEnablePortSecurity(handle, engineFlag);

	/* cmdq enable port security fail */
	if (ret) {
		DISPERR("DISP CMDQ enable port security failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY);

	return ret;
}

int disp_cmdq_enable_dapc(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	int ret;

	ret = cmdqRecSecureEnableDAPC(handle, engineFlag);

	/* cmdq enable dapc fail */
	if (ret) {
		DISPERR("DISP CMDQ enable dapc failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_ENABLE_DAPC]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_ENABLE_DAPC);

	return ret;
}

int disp_cmdq_write_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;

	ret = cmdqRecWrite(handle, addr, value, mask);

	/* cmdq write reg fail */
	if (ret) {
		DISPERR("DISP CMDQ write reg failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	if (mask == ((unsigned int)~0)) {
		_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG]);
		_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WRITE_REG);
	} else {
		_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK]);
		_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK);
	}

	return ret;
}

int disp_cmdq_write_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			 uint32_t slot_index, uint32_t value)
{
	int ret;

	ret = cmdqRecBackupUpdateSlot(handle, h_backup_slot, slot_index, value);

	/* cmdq write slot fail */
	if (ret) {
		DISPERR("DISP CMDQ write slot failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_SLOT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WRITE_SLOT);

	return ret;
}

int disp_cmdq_write_reg_secure(struct cmdqRecStruct *handle, uint32_t addr,
			       enum CMDQ_SEC_ADDR_METADATA_TYPE type, uint32_t baseHandle,
			       uint32_t offset, uint32_t size, uint32_t port)
{
	int ret;

	ret = cmdqRecWriteSecure(handle, addr, type, baseHandle, offset, size, port);

	/* cmdq write reg secure fail */
	if (ret) {
		DISPERR("DISP CMDQ write reg secure failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_REG_SECURE]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WRITE_REG_SECURE);

	return ret;
}

int disp_cmdq_poll_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;

	ret = cmdqRecPoll(handle, addr, value, mask);

	/* cmdq poll reg fail */
	if (ret) {
		DISPERR("DISP CMDQ poll reg failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_POLL_REG]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_POLL_REG);

	return ret;
}

int disp_cmdq_read_reg_to_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			       uint32_t slot_index, uint32_t regAddr)
{
	int ret;

	ret = cmdqRecBackupRegisterToSlot(handle, h_backup_slot, slot_index, regAddr);

	/* cmdq read reg to slot fail */
	if (ret) {
		DISPERR("DISP CMDQ read reg to slot failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_READ_REG_TO_SLOT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_READ_REG_TO_SLOT);

	return ret;
}

int disp_cmdq_wait_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;

	ret = cmdqRecWait(handle, event);

	/* cmdq wait event fail */
	if (ret) {
		DISPERR("DISP CMDQ wait event failed:%d, %d\n", ret, event);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WAIT_EVENT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WAIT_EVENT);

	return ret;
}

int disp_cmdq_wait_event_no_clear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;

	ret = cmdqRecWaitNoClear(handle, event);

	/* cmdq wait event no clear fail */
	if (ret) {
		DISPERR("DISP CMDQ wait event no clear failed:%d, %d\n", ret, event);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR);

	return ret;
}

int disp_cmdq_clear_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;

	ret = cmdqRecClearEventToken(handle, event);

	/* cmdq clear event fail */
	if (ret) {
		DISPERR("DISP CMDQ clear event failed:%d, %d\n", ret, event);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_CLEAR_EVENT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_CLEAR_EVENT);

	return ret;
}

int disp_cmdq_set_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int ret;

	ret = cmdqRecSetEventToken(handle, event);

	/* cmdq set event fail */
	if (ret) {
		DISPERR("DISP CMDQ set event failed:%d, %d\n", ret, event);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_SET_EVENT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_SET_EVENT);

	return ret;
}

int disp_cmdq_flush(struct cmdqRecStruct *handle, const char *func, int line)
{
	int ret;
	int count[3];

	count[0] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlush(handle);

	/* cmdq flush fail */
	if (ret) {
		DISPERR("%s:%d, flush failed:%d\n", func, line, ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(handle);
		_disp_cmdq_reset_instruction_count(handle);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 (unsigned long)handle, ret);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH]);
	_disp_cmdq_check_state(handle, func, line);
	_disp_cmdq_remove_all_state(handle);
	_disp_cmdq_reset_instruction_count(handle);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 (unsigned long)handle, ret);

	return ret;
}

int disp_cmdq_flush_async(struct cmdqRecStruct *handle, const char *func, int line)
{
	int ret;
	int count[3];

	count[0] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlushAsync(handle);

	/* cmdq flush async fail */
	if (ret) {
		DISPERR("%s:%d, flush async failed:%d\n", func, line, ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(handle);
		_disp_cmdq_reset_instruction_count(handle);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 (unsigned long)handle, ret);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH_ASYNC]);
	_disp_cmdq_check_state(handle, func, line);
	_disp_cmdq_remove_all_state(handle);
	_disp_cmdq_reset_instruction_count(handle);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 (unsigned long)handle, ret);

	return ret;
}

int disp_cmdq_flush_async_callback(struct cmdqRecStruct *handle, CmdqAsyncFlushCB callback,
				   uint32_t userData, const char *func, int line)
{
	int ret;
	int count[3];

	count[0] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG);
	count[1] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK);
	count[2] = _disp_cmdq_get_instruction_count(handle, DISP_CMDQ_INSTRUCTION_WRITE_SLOT);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_START,
			 count[0] | (count[1] << 16), count[2]);
	ret = cmdqRecFlushAsyncCallback(handle, callback, userData);

	/* cmdq flush async callback fail */
	if (ret) {
		DISPERR("%s:%d, flush async callback failed:%d\n", func, line, ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);
		_disp_cmdq_remove_all_state(handle);
		_disp_cmdq_reset_instruction_count(handle);
		mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
				 (unsigned long)handle, ret);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_FLUSH_ASYNC_CALLBACK]);
	_disp_cmdq_check_state(handle, func, line);
	_disp_cmdq_remove_all_state(handle);
	_disp_cmdq_reset_instruction_count(handle);
	mmprofile_log_ex(ddp_mmp_get_events()->primary_cmdq_done, MMPROFILE_FLAG_END,
			 (unsigned long)handle, ret);

	return ret;
}

int disp_cmdq_start_loop(struct cmdqRecStruct *handle)
{
	int ret;

	ret = cmdqRecStartLoop(handle);

	/* cmdq start loop fail */
	if (ret) {
		DISPERR("DISP CMDQ start loop failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_START_LOOP]);

	return ret;
}

int disp_cmdq_stop_loop(struct cmdqRecStruct *handle)
{
	int ret;

	ret = cmdqRecStopLoop(handle);

	/* cmdq stop loop fail */
	if (ret) {
		DISPERR("DISP CMDQ stop loop failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_STOP_LOOP]);

	return ret;
}

int disp_cmdq_acquire_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	int ret;

	ret = cmdqRecAcquireResource(handle, resourceEvent);

	/* cmdq acquire resource fail */
	if (ret) {
		DISPERR("DISP CMDQ acquire resource failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_ACQUIRE_RESOURCE]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_ACQUIRE_RESOURCE);

	return ret;
}

int disp_cmdq_release_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	int ret;

	ret = cmdqRecReleaseResource(handle, resourceEvent);

	/* cmdq release resource fail */
	if (ret) {
		DISPERR("DISP CMDQ release resource failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_RELEASE_RESOURCE]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_RELEASE_RESOURCE);

	return ret;
}

int disp_cmdq_write_for_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
				 uint32_t addr, uint32_t value, uint32_t mask)
{
	int ret;

	ret = cmdqRecWriteForResource(handle, resourceEvent, addr, value, mask);

	/* cmdq write for resource fail */
	if (ret) {
		DISPERR("DISP CMDQ write for resource failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE);

	return ret;
}

int disp_cmdq_get_instruction_count(struct cmdqRecStruct *handle)
{
	int ret;

	ret = cmdqRecGetInstructionCount(handle);

	/* cmdq get instruction count fail */
	if (handle == NULL) {
		DISPERR("DISP CMDQ get instruction count failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	/* bypass debug operation */
	_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_BYPASS);
	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT);

	return ret;
}

int disp_cmdq_dump_command(struct cmdqRecStruct *handle)
{
	int ret;

	ret = cmdqRecDumpCommand(handle);

	/* cmdq dump command fail */
	if (ret) {
		DISPERR("DISP CMDQ dump command failed:%d\n", ret);
		_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_ERROR);

		return ret;
	}

	/* bypass debug operation */
	_disp_cmdq_set_check_state(handle, DISP_CMDQ_CHECK_BYPASS);
	_disp_cmdq_insert_state(handle, disp_cmdq_func_state[DISP_CMDQ_FUNC_DUMP_COMMAND]);
	_disp_cmdq_insert_instruction_count(handle, DISP_CMDQ_FUNC_DUMP_COMMAND);

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

	ret = _disp_cmdq_dump_instruction_count(handle, func, line);

	return ret;
}

