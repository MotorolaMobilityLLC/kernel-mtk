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

#ifndef __DISP_CMDQ_H__
#define __DISP_CMDQ_H__

#include <linux/types.h>
#include <linux/uaccess.h>
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"


enum DISP_CMDQ_FUNC {
	DISP_CMDQ_FUNC_CREATE = 0,
	DISP_CMDQ_FUNC_DESTROY,
	DISP_CMDQ_FUNC_RESET,
	DISP_CMDQ_FUNC_SET_SECURE,
	DISP_CMDQ_FUNC_ENABLE_PORT_SECURITY,
	DISP_CMDQ_FUNC_ENABLE_DAPC,
	DISP_CMDQ_FUNC_WRITE_REG,
	DISP_CMDQ_FUNC_WRITE_REG_WITH_MASK,
	DISP_CMDQ_FUNC_WRITE_SLOT,
	DISP_CMDQ_FUNC_WRITE_REG_SECURE,
	DISP_CMDQ_FUNC_POLL_REG,
	DISP_CMDQ_FUNC_READ_REG_TO_SLOT,
	DISP_CMDQ_FUNC_WAIT_EVENT,
	DISP_CMDQ_FUNC_WAIT_EVENT_NO_CLEAR,
	DISP_CMDQ_FUNC_CLEAR_EVENT,
	DISP_CMDQ_FUNC_SET_EVENT,
	DISP_CMDQ_FUNC_FLUSH,
	DISP_CMDQ_FUNC_FLUSH_ASYNC,
	DISP_CMDQ_FUNC_FLUSH_ASYNC_CALLBACK,
	DISP_CMDQ_FUNC_START_LOOP,
	DISP_CMDQ_FUNC_STOP_LOOP,
	DISP_CMDQ_FUNC_ACQUIRE_RESOURCE,
	DISP_CMDQ_FUNC_RELEASE_RESOURCE,
	DISP_CMDQ_FUNC_WRITE_FOR_RESOURCE,
	DISP_CMDQ_FUNC_GET_INSTRUCTION_COUNT,
	DISP_CMDQ_FUNC_DUMP_COMMAND,
	DISP_CMDQ_FUNC_NUM,
};

enum DISP_CMDQ_STATE {
	DISP_CMDQ_STATE_BASIC = 0,
	DISP_CMDQ_STATE_CONFIG,
	DISP_CMDQ_STATE_EVENT,
	DISP_CMDQ_STATE_FLUSH,
	DISP_CMDQ_STATE_RESOURCE,
	DISP_CMDQ_STATE_DEBUG,
	DISP_CMDQ_STATE_UNKNOWN,
	DISP_CMDQ_STATE_NUM,
};

enum DISP_CMDQ_CHECK {
	DISP_CMDQ_CHECK_NORMAL = 0,
	DISP_CMDQ_CHECK_BYPASS,
	DISP_CMDQ_CHECK_ERROR,
	DISP_CMDQ_CHECK_NUM,
};

enum DISP_CMDQ_INSTRUCTION {
	DISP_CMDQ_INSTRUCTION_WRITE_REG = 0,
	DISP_CMDQ_INSTRUCTION_WRITE_REG_WITH_MASK,
	DISP_CMDQ_INSTRUCTION_WRITE_SLOT,
	DISP_CMDQ_INSTRUCTION_WRITE_REG_SECURE,
	DISP_CMDQ_INSTRUCTION_POLL_REG,
	DISP_CMDQ_INSTRUCTION_READ_REG_TO_SLOT,
	DISP_CMDQ_INSTRUCTION_NUM,
};

#define DISP_CMDQ_THREAD_NUM      (20)
#define DISP_CMDQ_THREAD_NAME_LENGTH      (64)
#define DISP_CMDQ_STATE_STACK_NUM      (DISP_CMDQ_STATE_NUM * 2)


char *disp_cmdq_get_event_name(enum CMDQ_EVENT_ENUM event);
int disp_cmdq_init(void);
int disp_cmdq_create(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle, const char *func);
int disp_cmdq_destroy(struct cmdqRecStruct *handle, const char *func, int line);
int disp_cmdq_reset(struct cmdqRecStruct *handle);
int disp_cmdq_set_check_state(struct cmdqRecStruct *handle, enum DISP_CMDQ_CHECK state);
int disp_cmdq_set_secure(struct cmdqRecStruct *handle, const bool is_secure);
int disp_cmdq_enable_port_security(struct cmdqRecStruct *handle, const uint64_t engineFlag);
int disp_cmdq_enable_dapc(struct cmdqRecStruct *handle, const uint64_t engineFlag);
int disp_cmdq_write_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask);
int disp_cmdq_write_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			 uint32_t slot_index, uint32_t value);
int disp_cmdq_write_reg_secure(struct cmdqRecStruct *handle, uint32_t addr,
			       enum CMDQ_SEC_ADDR_METADATA_TYPE type, uint32_t baseHandle,
			       uint32_t offset, uint32_t size, uint32_t port);
int disp_cmdq_poll_reg(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask);
int disp_cmdq_read_reg_to_slot(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			       uint32_t slot_index, uint32_t regAddr);
int disp_cmdq_wait_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event);
int disp_cmdq_wait_event_no_clear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event);
int disp_cmdq_clear_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event);
int disp_cmdq_set_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event);
int disp_cmdq_flush(struct cmdqRecStruct *handle, const char *func, int line);
int disp_cmdq_flush_async(struct cmdqRecStruct *handle, const char *func, int line);
int disp_cmdq_flush_async_callback(struct cmdqRecStruct *handle, CmdqAsyncFlushCB callback,
				   uint32_t userData, const char *func, int line);
int disp_cmdq_start_loop(struct cmdqRecStruct *handle);
int disp_cmdq_stop_loop(struct cmdqRecStruct *handle);
int disp_cmdq_acquire_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent);
int disp_cmdq_release_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent);
int disp_cmdq_write_for_resource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
				 uint32_t addr, uint32_t value, uint32_t mask);
int disp_cmdq_get_instruction_count(struct cmdqRecStruct *handle);
int disp_cmdq_dump_command(struct cmdqRecStruct *handle);
int disp_cmdq_insert_instruction_count(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot);
int disp_cmdq_dump_instruction_count(struct cmdqRecStruct *handle, const char *func, int line);

#endif
