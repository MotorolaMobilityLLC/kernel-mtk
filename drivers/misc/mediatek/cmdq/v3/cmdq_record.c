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

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/memory.h>
#include <mt-plat/mtk_lpae.h>

#include "cmdq_record.h"
#include "cmdq_core.h"
#include "cmdq_sec.h"
#include "cmdq_virtual.h"
#include "cmdq_reg.h"
#include "cmdq_prof.h"

#ifdef CMDQ_SECURE_PATH_SUPPORT
#include "cmdq_sec_iwc_common.h"
#endif

#ifndef _CMDQ_DEBUG_
#define DISABLE_LOOP_IRQ
#endif

#define CMDQ_DATA_VAR			(CMDQ_BIT_VAR<<CMDQ_DATA_BIT)
#define CMDQ_TASK_TPR_VAR		(CMDQ_DATA_VAR | CMDQ_TPR_ID)
#define CMDQ_TASK_TEMP_CPR_VAR	(CMDQ_DATA_VAR | CMDQ_SPR_FOR_TEMP)
#define CMDQ_TASK_LOOP_DEBUG_VAR	(CMDQ_DATA_VAR | CMDQ_SPR_FOR_LOOP_DEBUG)
#define CMDQ_ARG_CPR_START	(CMDQ_DATA_VAR | CMDQ_CPR_STRAT_ID)
#define CMDQ_EVENT_ARGA(event_id)	(CMDQ_CODE_WFE << 24 | event_id)

#define CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE	(32)

static void cmdq_task_release_property(struct cmdqRecStruct *handle)
{
	if (!handle)
		return;

	kfree(handle->prop_addr);
	handle->prop_addr = NULL;
	handle->prop_size = 0;
}

/* push a value into a stack */
int32_t cmdq_op_condition_push(struct cmdq_stack_node **top_node, uint32_t position,
								enum CMDQ_STACK_TYPE_ENUM stack_type)
{
	/* allocate a new node for val */
	struct cmdq_stack_node *new_node = kmalloc(sizeof(struct cmdq_stack_node), GFP_KERNEL);

	if (new_node == NULL) {
		CMDQ_ERR("failed to alloc cmdq_stack_node\n");
		return -ENOMEM;
	}

	new_node->position = position;
	new_node->stack_type = stack_type;
	new_node->next = *top_node;
	*top_node = new_node;

	return 0;
}

/* pop a value out from the stack */
int32_t cmdq_op_condition_pop(struct cmdq_stack_node **top_node, uint32_t *position,
							enum CMDQ_STACK_TYPE_ENUM *stack_type)
{
	/* tmp for record the top node */
	struct cmdq_stack_node *temp_node;

	if (*top_node == NULL)
		return -1;

	/* get the value of the top */
	temp_node = *top_node;
	*position = temp_node->position;
	*stack_type = temp_node->stack_type;
	/* change top */
	*top_node = temp_node->next;
	kfree(temp_node);

	return 0;
}

/* query from the stack */
int32_t cmdq_op_condition_query(const struct cmdq_stack_node *top_node, int *position,
							enum CMDQ_STACK_TYPE_ENUM *stack_type)
{
	*stack_type = CMDQ_STACK_NULL;

	if (top_node == NULL)
		return -1;

	/* get the value of the top */
	*position = top_node->position;
	*stack_type = top_node->stack_type;

	return 0;
}

/* query op position from the stack by type bit */
s32 cmdq_op_condition_find_op_type(const struct cmdq_stack_node *top_node,
	const u32 position, u32 op_type_bit, const struct cmdq_stack_node **op_node)
{
	const struct cmdq_stack_node *temp_node = top_node;
	u32 got_position = position;

	/* get the value of the top */
	do {
		if (!temp_node)
			break;

		if ((1 << temp_node->stack_type) & op_type_bit) {
			got_position = temp_node->position;
			if (op_node)
				*op_node = temp_node;
			break;
		}

		temp_node = temp_node->next;
	} while (1);

	return (s32)(got_position - position);
}

static bool cmdq_is_cpr(uint32_t argument, uint32_t arg_type)
{
	if (arg_type == 1 &&
		argument >= CMDQ_THR_SPR_MAX &&
		argument < CMDQ_THR_VAR_MAX) {
		return true;
	}

	return false;
}

static void cmdq_save_op_variable_position(struct cmdqRecStruct *handle,
	u32 index)
{
	u32 *p_new_buffer = NULL;
	u32 *p_instr_position = NULL;
	u32 array_num = 0;

	/* Exceed max number of SPR, use CPR */
	if ((handle->replace_instr.number %
		CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE) == 0) {
		array_num = (handle->replace_instr.number +
			CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE) *
			sizeof(uint32_t);

		p_new_buffer = kzalloc(array_num, GFP_KERNEL);
		CMDQ_MSG("allocate array with size:%u p:0x%p\n",
			array_num, p_new_buffer);

		/* copy and release old buffer */
		if (handle->replace_instr.position !=
			(cmdqU32Ptr_t)(unsigned long)NULL) {
			memcpy(p_new_buffer,
				CMDQ_U32_PTR(handle->replace_instr.position),
				handle->replace_instr.number *
				sizeof(uint32_t));
			kfree(CMDQ_U32_PTR(handle->replace_instr.position));
		}
		handle->replace_instr.position =
			(cmdqU32Ptr_t)(unsigned long)p_new_buffer;
	}

	p_instr_position = CMDQ_U32_PTR(handle->replace_instr.position);
	p_instr_position[handle->replace_instr.number] = index;
	handle->replace_instr.number++;
	CMDQ_MSG("Add replace_instr: index:%u position:%u number:%u\n",
		index, p_instr_position[handle->replace_instr.number-1],
		handle->replace_instr.number);
}

static int32_t cmdq_var_data_type(CMDQ_VARIABLE arg_in, uint32_t *arg_out, uint32_t *arg_type)
{
	int32_t status = 0;

	switch (arg_in >> CMDQ_DATA_BIT) {
	case CMDQ_BIT_VALUE:
		*arg_type = 0;
		*arg_out = (uint32_t)(arg_in & 0xFFFFFFFF);
		break;
	case CMDQ_BIT_VAR:
		*arg_type = 1;
		*arg_out = (uint32_t)(arg_in & 0xFFFFFFFF);
		break;
	default:
		CMDQ_ERR("Incorrect CMDQ data type (0x%llx), can not append new command\n", arg_in);
		status = -EFAULT;
		break;
	}

	return status;
}

static int32_t cmdq_create_variable_if_need(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg)
{
	int32_t status = 0;
	u32 arg_value, arg_type;

	do {
		status = cmdq_var_data_type(*arg, &arg_value, &arg_type);
		if (status < 0)
			break;

		CMDQ_MSG("check CPR create: value:%d, type: %d\n", arg_value, arg_type);
		if (arg_type == 1) {
			/* Already be variable */
			break;
		}

		if (handle->local_var_num >= CMDQ_THR_FREE_USR_VAR_MAX) {
			CMDQ_ERR("Exceed max number of local variable in one task, please review your instructions.\n");
			status = -EFAULT;
			break;
		}

		*arg = ((CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | handle->local_var_num);
		handle->local_var_num++;
	} while (0);

	return status;
}

int32_t cmdq_reset_v3_struct(struct cmdqRecStruct *handle)
{
	uint32_t destroy_position;
	enum CMDQ_STACK_TYPE_ENUM destroy_stack_type;

	if (handle == NULL)
		return -EFAULT;

	/* reset local variable setting */
	handle->local_var_num = CMDQ_THR_SPR_START;
	handle->arg_value = CMDQ_TASK_CPR_INITIAL_VALUE;
	handle->arg_source = CMDQ_TASK_CPR_INITIAL_VALUE;
	handle->arg_timeout = CMDQ_TASK_CPR_INITIAL_VALUE;

	do {
		/* check if-else stack */
		if (handle->if_stack_node == NULL)
			break;

		/* pop all if-else stack out */
		cmdq_op_condition_pop(&handle->if_stack_node, &destroy_position, &destroy_stack_type);
	} while (1);

	do {
		/* check while stack */
		if (handle->while_stack_node == NULL)
			break;

		/* pop all while stack out */
		cmdq_op_condition_pop(&handle->while_stack_node, &destroy_position, &destroy_stack_type);
	} while (1);
	return 0;
}

int32_t cmdq_rec_realloc_addr_metadata_buffer(struct cmdqRecStruct *handle, const uint32_t size)
{
	void *pNewBuf = NULL;
	void *pOriginalBuf = (void *)CMDQ_U32_PTR(handle->secData.addrMetadatas);
	const uint32_t originalSize =
	    sizeof(struct cmdqSecAddrMetadataStruct) * (handle->secData.addrMetadataMaxCount);

	if (size <= originalSize)
		return 0;

	pNewBuf = kzalloc(size, GFP_KERNEL);
	if (pNewBuf == NULL) {
		CMDQ_ERR("REC: secAddrMetadata, kzalloc %d bytes addr_metadata buffer failed\n",
			 size);
		return -ENOMEM;
	}

	if (pOriginalBuf && originalSize > 0)
		memcpy(pNewBuf, pOriginalBuf, originalSize);

	CMDQ_VERBOSE("REC: secAddrMetadata, realloc size from %d to %d bytes\n", originalSize,
		     size);
	kfree(pOriginalBuf);
	handle->secData.addrMetadatas = (cmdqU32Ptr_t) (unsigned long)(pNewBuf);
	handle->secData.addrMetadataMaxCount = size / sizeof(struct cmdqSecAddrMetadataStruct);

	return 0;
}

int cmdq_rec_realloc_cmd_buffer(struct cmdqRecStruct *handle, uint32_t size)
{
	void *pNewBuf = NULL;

	if (size <= handle->bufferSize)
		return 0;

	pNewBuf = vzalloc(size);

	if (pNewBuf == NULL) {
		CMDQ_ERR("REC: kzalloc %d bytes cmd_buffer failed\n", size);
		return -ENOMEM;
	}

	memset(pNewBuf, 0, size);

	if (handle->pBuffer && handle->blockSize > 0)
		memcpy(pNewBuf, handle->pBuffer, handle->blockSize);

	CMDQ_VERBOSE("REC: realloc size from %d to %d bytes\n", handle->bufferSize, size);

	vfree(handle->pBuffer);
	handle->pBuffer = pNewBuf;
	handle->bufferSize = size;

	return 0;
}

static int32_t cmdq_reset_profile_maker_data(struct cmdqRecStruct *handle)
{
	int32_t i = 0;

	if (handle == NULL)
		return -EFAULT;

	handle->profileMarker.count = 0;
	handle->profileMarker.hSlot = 0LL;

	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		handle->profileMarker.tag[i] = (cmdqU32Ptr_t) (unsigned long)(NULL);

	return 0;
}

int32_t cmdq_task_create(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle)
{
	struct cmdqRecStruct *handle = NULL;

	if (pHandle == NULL) {
		CMDQ_ERR("Invalid empty handle\n");
		return -EINVAL;
	}

	*pHandle = NULL;

	if (scenario < 0 || scenario >= CMDQ_MAX_SCENARIO_COUNT) {
		CMDQ_ERR("Unknown scenario type %d\n", scenario);
		return -EINVAL;
	}

	handle = kzalloc(sizeof(struct cmdqRecStruct), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	handle->scenario = scenario;
	handle->pBuffer = NULL;
	handle->bufferSize = 0;
	handle->engineFlag = cmdq_get_func()->flagFromScenario(scenario);
	handle->priority = CMDQ_REC_DEFAULT_PRIORITY;
	handle->pRunningTask = NULL;
	handle->ext.ctrl = cmdq_core_get_controller();

	cmdq_task_reset(handle);

	/* CMD */
	if (cmdq_rec_realloc_cmd_buffer(handle, CMDQ_INITIAL_CMD_BLOCK_SIZE) != 0) {
		kfree(handle);
		return -ENOMEM;
	}

	*pHandle = handle;

	return 0;
}

#ifdef CMDQ_SECURE_PATH_SUPPORT
int32_t cmdq_append_addr_metadata(struct cmdqRecStruct *handle,
	const struct cmdqSecAddrMetadataStruct *pMetadata)
{
	struct cmdqSecAddrMetadataStruct *pAddrs;
	int32_t status;
	uint32_t size;
	/* element index of the New appended addr metadat */
	const uint32_t index = handle->secData.addrMetadataCount;

	pAddrs = NULL;
	status = 0;

	if (handle->secData.addrMetadataMaxCount <= 0) {
		/* not init yet, initialize to allow max 8 addr metadata */
		size = sizeof(struct cmdqSecAddrMetadataStruct) * 8;
		status = cmdq_rec_realloc_addr_metadata_buffer(handle, size);
	} else if (handle->secData.addrMetadataCount >= (handle->secData.addrMetadataMaxCount)) {
		/* enlarge metadata buffer to twice as */
		size =
		    sizeof(struct cmdqSecAddrMetadataStruct) * (handle->secData.addrMetadataMaxCount) * 2;
		status = cmdq_rec_realloc_addr_metadata_buffer(handle, size);
	}

	if (status < 0)
		return -ENOMEM;

	if (handle->secData.addrMetadataCount >= CMDQ_IWC_MAX_ADDR_LIST_LENGTH) {
		uint32_t maxMetaDataCount = CMDQ_IWC_MAX_ADDR_LIST_LENGTH;

		CMDQ_ERR("Metadata idx = %d reach the max allowed number = %d.\n",
			 handle->secData.addrMetadataCount, maxMetaDataCount);
		CMDQ_MSG("ADDR: type:%d, baseHandle:0x%llx, offset:%d, size:%d, port:%d\n",
			 pMetadata->type, pMetadata->baseHandle, pMetadata->offset, pMetadata->size,
			 pMetadata->port);
		status = -EFAULT;
	} else {
		pAddrs =
		    (struct cmdqSecAddrMetadataStruct *) (CMDQ_U32_PTR(handle->secData.addrMetadatas));
		/* append meatadata */
		pAddrs[index].instrIndex = pMetadata->instrIndex;
		pAddrs[index].baseHandle = pMetadata->baseHandle;
		pAddrs[index].offset = pMetadata->offset;
		pAddrs[index].size = pMetadata->size;
		pAddrs[index].port = pMetadata->port;
		pAddrs[index].type = pMetadata->type;

		/* meatadata count ++ */
		handle->secData.addrMetadataCount += 1;
	}

	return status;
}
#endif

int32_t cmdq_check_before_append(struct cmdqRecStruct *handle)
{
	if (handle == NULL)
		return -EFAULT;

	if (handle->finalized) {
		CMDQ_ERR("Finalized record 0x%p (scenario:%d)\n", handle, handle->scenario);
		return -EBUSY;
	}

	/* check if we have sufficient buffer size */
	/* we leave a 4 instruction (4 bytes each) margin. */
	if ((handle->blockSize + 32) >= handle->bufferSize) {
		if (cmdq_rec_realloc_cmd_buffer(handle, handle->bufferSize * 2) != 0)
			return -ENOMEM;
	}

	return 0;
}

/**
 * centralize the write/polling/read command for APB and GPR handle
 * this function must be called inside cmdq_append_command
 * because we ignore buffer and pre-fetch check here.
 * Parameter:
 *     same as cmdq_append_command
 * Return:
 *     same as cmdq_append_command
 */
static int32_t cmdq_append_wpr_command(struct cmdqRecStruct *handle, enum CMDQ_CODE_ENUM code,
				       uint32_t arg_a, uint32_t arg_b, uint32_t arg_a_type,
				       uint32_t arg_b_type)
{
	int32_t status = 0;
	int32_t subsys;
	uint32_t *p_command;
	bool bUseGPR = false;
	/* use new arg_a to present final inserted arg_a */
	uint32_t new_arg_a;
	uint32_t new_arg_a_type = arg_a_type;
	uint32_t arg_type = 0;

	/* be careful that subsys encoding position is different among platforms */
	const uint32_t subsys_bit = cmdq_get_func()->getSubsysLSBArgA();

	if (CMDQ_CODE_READ != code && CMDQ_CODE_WRITE != code && CMDQ_CODE_POLL != code) {
		CMDQ_ERR("Record 0x%p, flow error, should not append comment in wpr API", handle);
		return -EFAULT;
	}

	/* we must re-calculate current PC at first. */
	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	CMDQ_VERBOSE("REC: 0x%p CMD: 0x%p, op: 0x%02x\n", handle, p_command, code);
	CMDQ_VERBOSE("REC: 0x%p CMD: arg_a: 0x%08x, arg_b: 0x%08x, arg_a_type: %d, arg_b_type: %d\n",
		     handle, arg_a, arg_b, arg_a_type, arg_b_type);

	if (arg_a_type == 0) {
		/* arg_a is the HW register address to read from */
		subsys = cmdq_core_subsys_from_phys_addr(arg_a);
		if (subsys == CMDQ_SPECIAL_SUBSYS_ADDR) {
#ifdef CMDQ_GPR_SUPPORT
			bUseGPR = true;
			CMDQ_MSG("REC: Special handle memory base address 0x%08x\n", arg_a);
			/* Wait and clear for GPR mutex token to enter mutex */
			*p_command++ = ((1 << 31) | (1 << 15) | 1);
			*p_command++ = (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_TOKEN_GPR_SET_4;
			handle->blockSize += CMDQ_INST_SIZE;
			/* Move extra handle APB address to GPR */
			*p_command++ = arg_a;
			*p_command++ = (CMDQ_CODE_MOVE << 24) |
			    ((CMDQ_DATA_REG_DEBUG & 0x1f) << 16) | (4 << 21);
			handle->blockSize += CMDQ_INST_SIZE;
			/* change final arg_a to GPR */
			new_arg_a = ((CMDQ_DATA_REG_DEBUG & 0x1f) << subsys_bit);
			if (arg_a & 0x1) {
				/* MASK case, set final bit to 1 */
				new_arg_a = new_arg_a | 0x1;
			}
			/* change arg_a type to 1 */
			new_arg_a_type = 1;
#else
			CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
			status = -EFAULT;
#endif
		} else if (arg_a_type == 0 && subsys < 0) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", arg_a);
			status = -EFAULT;
		} else {
			/* compose final arg_a according to subsys table */
			new_arg_a = (arg_a & 0xffff) | ((subsys & 0x1f) << subsys_bit);
		}
	} else {
		/* compose final arg_a according GPR value */
		new_arg_a = ((arg_a & 0x1f) << subsys_bit);
	}

	if (status < 0)
		return status;

	arg_type = (new_arg_a_type << 2) | (arg_b_type << 1);

	/* new_arg_a is the HW register address to access from or GPR value store the HW register address */
	/* arg_b is the value or register id  */
	/* bit 55: arg_a type, 1 for GPR */
	/* bit 54: arg_b type, 1 for GPR */
	/* argType: ('new_arg_a_type', 'arg_b_type', '0') */
	*p_command++ = arg_b;
	*p_command++ = (code << 24) | new_arg_a | (arg_type << 21);
	handle->blockSize += CMDQ_INST_SIZE;

	if (bUseGPR) {
		/* Set for GPR mutex token to leave mutex */
		*p_command++ = ((1 << 31) | (1 << 16));
		*p_command++ = (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_TOKEN_GPR_SET_4;
		handle->blockSize += CMDQ_INST_SIZE;
	}
	return 0;
}

/**
 * centralize the read_s & write_s command for APB and GPR handle
 * this function must be called inside cmdq_append_command
 * because we ignore buffer and pre-fetch check here.
 * Parameter:
 *     same as cmdq_append_command
 * Return:
 *     same as cmdq_append_command
 */
static int32_t cmdq_append_rw_s_command(struct cmdqRecStruct *handle, enum CMDQ_CODE_ENUM code,
				       uint32_t arg_a, uint32_t arg_b, uint32_t arg_a_type,
				       uint32_t arg_b_type)
{
	int32_t status = 0;
	uint32_t *p_command;

	uint32_t new_arg_a, new_arg_b;
	uint32_t arg_addr, arg_value;
	uint32_t arg_addr_type, arg_value_type;
	uint32_t arg_type = 0;
	int32_t subsys = 0;

	/* be careful that subsys encoding position is different among platforms */
	const uint32_t subsys_bit = cmdq_get_func()->getSubsysLSBArgA();

	if (code == CMDQ_CODE_WRITE_S || code == CMDQ_CODE_WRITE_S_W_MASK) {
		/* For write_s command */
		arg_addr = arg_a;
		arg_addr_type = arg_a_type;
		arg_value = arg_b;
		arg_value_type = arg_b_type;
	} else if (code == CMDQ_CODE_READ_S) {
		/* For read_s command */
		arg_addr = arg_b;
		arg_addr_type = arg_b_type;
		arg_value = arg_a;
		arg_value_type = arg_a_type;
	} else {
		CMDQ_ERR("Record 0x%p, flow error, should not append comment in read_s & write_s API", handle);
		return -EFAULT;
	}

	/* we must re-calculate current PC at first. */
	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	CMDQ_VERBOSE("REC: 0x%p CMD: 0x%p, op: 0x%02x\n", handle, p_command, code);
	CMDQ_VERBOSE("REC: 0x%p CMD: arg_a: 0x%08x, arg_b: 0x%08x, arg_a_type: %d, arg_b_type: %d\n",
		     handle, arg_a, arg_b, arg_a_type, arg_b_type);

	if (arg_addr_type == 0) {
		/* arg_a is the HW register address to read from */
		subsys = cmdq_core_subsys_from_phys_addr(arg_addr);
		if (subsys == CMDQ_SPECIAL_SUBSYS_ADDR) {
			CMDQ_MSG("REC: Special handle memory base address 0x%08x\n", arg_a);
			/* Assign extra handle APB address to SPR */
			*p_command++ = arg_addr;
			*p_command++ = (CMDQ_CODE_LOGIC << 24) | (4 << 21) |
			    (CMDQ_LOGIC_ASSIGN << 16) | CMDQ_SPR_FOR_TEMP;
			handle->blockSize += CMDQ_INST_SIZE;
			/* change final arg_addr to GPR */
			subsys = 0;
			arg_addr = CMDQ_SPR_FOR_TEMP;
			/* change arg_addr type to 1 */
			arg_addr_type = 1;
		} else if (arg_addr_type == 0 && subsys < 0) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", arg_addr);
			status = -EFAULT;
		}
	}

	if (status < 0)
		return status;

	if (handle->ext.exclusive_thread != CMDQ_INVALID_THREAD) {
		u32 cpr_offset = CMDQ_CPR_STRAT_ID + CMDQ_THR_CPR_MAX *
			handle->ext.exclusive_thread;

		/* change cpr to thread cpr directly,
		 * if we already have exclusive thread.
		 */
		if (cmdq_is_cpr(arg_addr, arg_addr_type))
			arg_addr = cpr_offset + (arg_addr - CMDQ_THR_SPR_MAX);
		if (cmdq_is_cpr(arg_value, arg_value_type))
			arg_value = cpr_offset + (arg_value - CMDQ_THR_SPR_MAX);
	} else if (cmdq_is_cpr(arg_a, arg_a_type) ||
		cmdq_is_cpr(arg_b, arg_b_type)) {
		/* save local variable position */
		CMDQ_MSG(
			"save op: 0x%02x, CMD: arg_a: 0x%08x, arg_b: 0x%08x, arg_a_type: %d, arg_b_type: %d\n",
		     code, arg_a, arg_b, arg_a_type, arg_b_type);
		cmdq_save_op_variable_position(handle,
			cmdq_task_get_instruction_count(handle));
	}

	if (CMDQ_CODE_WRITE_S == code || CMDQ_CODE_WRITE_S_W_MASK == code) {
		/* For write_s command */
		new_arg_a = (arg_addr & 0xffff) | ((subsys & 0x1f) << subsys_bit);
		if (arg_value_type == 0)
			new_arg_b = arg_value;
		else
			new_arg_b = arg_value << 16;
		arg_type = (arg_addr_type << 2) | (arg_value_type << 1);
	} else {
		/* For read_s command */
		new_arg_a = (arg_value & 0xffff) | ((subsys & 0x1f) << subsys_bit);
		new_arg_b = (arg_addr & 0xffff) << 16;
		arg_type = (arg_value_type << 2) | (arg_addr_type << 1);
	}

	/* new_arg_a is the HW register address to access from or GPR value store the HW register address */
	/* arg_b is the value or register id  */
	/* bit 55: arg_a type, 1 for HW register */
	/* bit 54: arg_b type, 1 for HW register */
	/* argType: ('new_arg_a_type', 'arg_b_type', '0') */
	*p_command++ = new_arg_b;
	*p_command++ = (code << 24) | new_arg_a | (arg_type << 21);
	handle->blockSize += CMDQ_INST_SIZE;

	return 0;
}

int32_t cmdq_append_command(struct cmdqRecStruct *handle, enum CMDQ_CODE_ENUM code,
			    uint32_t arg_a, uint32_t arg_b, uint32_t arg_a_type, uint32_t arg_b_type)
{
	int32_t status;
	uint32_t *p_command;

	status = cmdq_check_before_append(handle);
	if (status < 0) {
		CMDQ_ERR("	  cannot add command (op: 0x%02x, arg_a: 0x%08x, arg_b: 0x%08x)\n",
			code, arg_a, arg_b);
		return status;
	}

	/* force insert MARKER if prefetch memory is full */
	/* GCE deadlocks if we don't do so */
	if (code != CMDQ_CODE_EOC && cmdq_get_func()->shouldEnablePrefetch(handle->scenario)) {
		uint32_t prefetchSize = 0;
		int32_t threadNo = cmdq_get_func()->getThreadID(handle->scenario, handle->secData.is_secure);

		prefetchSize = cmdq_core_thread_prefetch_size(threadNo);
		if (prefetchSize > 0 && handle->prefetchCount >= prefetchSize) {
			CMDQ_MSG
			    ("prefetchCount(%d) > %d, force insert disable prefetch marker\n",
			     handle->prefetchCount, prefetchSize);
			/* Mark END of prefetch section */
			cmdqRecDisablePrefetch(handle);
			/* BEGING of next prefetch section */
			cmdqRecMark(handle);
		} else {
			/* prefetch enabled marker exist */
			if (handle->prefetchCount >= 1) {
				++handle->prefetchCount;
				CMDQ_VERBOSE("handle->prefetchCount: %d, %s, %d\n",
					     handle->prefetchCount, __func__, __LINE__);
			}
		}
	}

	/* we must re-calculate current PC because we may already insert MARKER inst. */
	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	CMDQ_VERBOSE("REC: 0x%p CMD: 0x%p, op: 0x%02x, arg_a: 0x%08x, arg_b: 0x%08x\n", handle,
		     p_command, code, arg_a, arg_b);

	switch (code) {
	case CMDQ_CODE_READ:
	case CMDQ_CODE_WRITE:
	case CMDQ_CODE_POLL:
		/* Because read/write/poll have similar format, handle them together */
		return cmdq_append_wpr_command(handle, code, arg_a, arg_b, arg_a_type, arg_b_type);
	case CMDQ_CODE_READ_S:
	case CMDQ_CODE_WRITE_S:
	case CMDQ_CODE_WRITE_S_W_MASK:
		return cmdq_append_rw_s_command(handle, code, arg_a, arg_b, arg_a_type, arg_b_type);
	case CMDQ_CODE_MOVE:
		*p_command++ = arg_b;
		*p_command++ = CMDQ_CODE_MOVE << 24 | (arg_a & 0xffffff);
		break;
	case CMDQ_CODE_JUMP:
		*p_command++ = arg_b;
		*p_command++ = (CMDQ_CODE_JUMP << 24) | (arg_a & 0x0FFFFFF);
		break;
	case CMDQ_CODE_WFE:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		*p_command++ = ((1 << 31) | (1 << 15) | 1);
		*p_command++ = (CMDQ_CODE_WFE << 24) | arg_a;
		break;

	case CMDQ_CODE_SET_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 1 */
		*p_command++ = ((1 << 31) | (1 << 16));
		*p_command++ = (CMDQ_CODE_WFE << 24) | arg_a;
		break;

	case CMDQ_CODE_WAIT_NO_CLEAR:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, false */
		*p_command++ = ((0 << 31) | (1 << 15) | 1);
		*p_command++ = (CMDQ_CODE_WFE << 24) | arg_a;
		break;

	case CMDQ_CODE_CLEAR_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		*p_command++ = ((1 << 31) | (0 << 16));
		*p_command++ = (CMDQ_CODE_WFE << 24) | arg_a;
		break;

	case CMDQ_CODE_EOC:
		*p_command++ = arg_b;
		*p_command++ = (CMDQ_CODE_EOC << 24) | (arg_a & 0x0FFFFFF);
		break;

	case CMDQ_CODE_RAW:
		*p_command++ = arg_b;
		*p_command++ = arg_a;
		break;

	default:
		return -EFAULT;
	}

	if (code == CMDQ_CODE_JUMP && (arg_a & 0x1) == 0 &&
		handle->ext.ctrl->change_jump) {
		cmdq_save_op_variable_position(handle, cmdq_task_get_instruction_count(handle));
		CMDQ_MSG("save jump event: 0x%02x, CMD: arg_a: 0x%08x\n", code, arg_a);
	}

	handle->blockSize += CMDQ_INST_SIZE;
	return 0;
}

int32_t cmdq_task_set_engine(struct cmdqRecStruct *handle, uint64_t engineFlag)
{
	if (handle == NULL)
		return -EFAULT;

	CMDQ_VERBOSE("REC: %p, engineFlag: 0x%llx\n", handle, engineFlag);
	handle->engineFlag = engineFlag;

	return 0;
}

int32_t cmdq_task_reset(struct cmdqRecStruct *handle)
{
	if (handle == NULL)
		return -EFAULT;

	if (handle->pRunningTask != NULL)
		cmdq_task_stop_loop(handle);

	cmdq_reset_v3_struct(handle);

	handle->ext.ctrl = cmdq_core_get_controller();
	handle->blockSize = 0;
	handle->prefetchCount = 0;
	handle->finalized = false;
	handle->ext.res_engine_flag_acquire = 0;
	handle->ext.res_engine_flag_release = 0;

	/* reset secure path data */
	handle->secData.is_secure = false;
	handle->secData.enginesNeedDAPC = 0LL;
	handle->secData.enginesNeedPortSecurity = 0LL;
	if (handle->secData.addrMetadatas) {
		kfree(CMDQ_U32_PTR(handle->secData.addrMetadatas));
		handle->secData.addrMetadatas = (cmdqU32Ptr_t) (unsigned long)NULL;
		handle->secData.addrMetadataMaxCount = 0;
		handle->secData.addrMetadataCount = 0;
	}

	/* reset local variable setting */
	handle->replace_instr.number = 0;
	if (handle->replace_instr.position != (cmdqU32Ptr_t) (unsigned long)NULL) {
		kfree(CMDQ_U32_PTR(handle->replace_instr.position));
		handle->replace_instr.position = (cmdqU32Ptr_t) (unsigned long)NULL;
	}

	/* profile marker */
	cmdq_reset_profile_maker_data(handle);

	/* initialize property */
	cmdq_task_release_property(handle);

	return 0;
}

int32_t cmdq_task_set_secure(struct cmdqRecStruct *handle, const bool is_secure)
{
	if (handle == NULL)
		return -EFAULT;

	handle->secData.is_secure = is_secure;

	if (handle->blockSize > 0)
		CMDQ_MSG("[warn]set secure after record size:%u\n",
			handle->blockSize);

	if (!is_secure) {
		handle->ext.ctrl = cmdq_core_get_controller();
		handle->ext.exclusive_thread = CMDQ_INVALID_THREAD;
		return 0;
	}
#ifdef CMDQ_SECURE_PATH_SUPPORT
	handle->ext.ctrl = cmdq_sec_get_controller();
	handle->ext.exclusive_thread = cmdq_get_func()->getThreadID(
		handle->scenario, true);
	CMDQ_VERBOSE("REC:%p secure:%d exclusive thread:%d\n",
		handle, is_secure, handle->ext.exclusive_thread);
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_task_is_secure(struct cmdqRecStruct *handle)
{
	if (handle == NULL)
		return -EFAULT;

	return handle->secData.is_secure;
}

int32_t cmdq_task_secure_enable_dapc(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (handle == NULL)
		return -EFAULT;

	handle->secData.enginesNeedDAPC |= engineFlag;
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_task_secure_enable_port_security(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (handle == NULL)
		return -EFAULT;

	handle->secData.enginesNeedPortSecurity |= engineFlag;
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_op_write_reg(struct cmdqRecStruct *handle, uint32_t addr,
				   CMDQ_VARIABLE argument, uint32_t mask)
{
	int32_t status = 0;
	enum CMDQ_CODE_ENUM op_code;
	uint32_t arg_b_i, arg_b_type;

	if (mask != 0xFFFFFFFF) {
		status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask, 0, 0);
		if (status != 0)
			return status;
	}

	if (mask != 0xFFFFFFFF)
		op_code = CMDQ_CODE_WRITE_S_W_MASK;
	else
		op_code = CMDQ_CODE_WRITE_S;

	status = cmdq_var_data_type(argument, &arg_b_i, &arg_b_type);
	if (status < 0)
		return status;

	return cmdq_append_command(handle, op_code, addr, arg_b_i, 0, arg_b_type);
}

int32_t cmdq_op_write_reg_secure(struct cmdqRecStruct *handle, uint32_t addr,
			   enum CMDQ_SEC_ADDR_METADATA_TYPE type, uint64_t baseHandle,
			   uint32_t offset, uint32_t size, uint32_t port)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status;
	int32_t writeInstrIndex;
	struct cmdqSecAddrMetadataStruct metadata;
	const uint32_t mask = 0xFFFFFFFF;

	/* append command */
	status = cmdq_op_write_reg(handle, addr, baseHandle, mask);
	if (status != 0)
		return status;

	/* append to metadata list */
	writeInstrIndex = (handle->blockSize) / CMDQ_INST_SIZE - 1;	/* start from 0 */

	memset(&metadata, 0, sizeof(struct cmdqSecAddrMetadataStruct));
	metadata.instrIndex = writeInstrIndex;
	metadata.type = type;
	metadata.baseHandle = baseHandle;
	metadata.offset = offset;
	metadata.size = size;
	metadata.port = port;

	status = cmdq_append_addr_metadata(handle, &metadata);

	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_op_poll(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t status;

	status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask, 0, 0);
	if (status != 0)
		return status;

	status = cmdq_append_command(handle, CMDQ_CODE_POLL, (addr | 0x1), value, 0, 0);
	if (status != 0)
		return status;

	return 0;
}

/* Efficient Polling */
int32_t cmdq_op_poll_v3(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	/*
	 * Simulate Code
	 * do {
	 *   arg_temp = [addr] & mask
	 *   wait_and_clear (100us);
	 * } while (arg_temp != value);
	 */

	CMDQ_VARIABLE arg_loop_debug = CMDQ_TASK_LOOP_DEBUG_VAR;
	u32 condition_value = value & mask;

	cmdq_op_assign(handle, &handle->arg_value, condition_value);
	cmdq_op_assign(handle, &arg_loop_debug, 0);
	cmdq_op_do_while(handle);
		cmdq_op_read_reg(handle, addr, &handle->arg_source, mask);
		cmdq_op_add(handle, &arg_loop_debug, arg_loop_debug, 1);
		cmdq_op_wait(handle, CMDQ_EVENT_TIMER_00 + CMDQ_POLLING_TPR_MASK_BIT);
	cmdq_op_end_do_while(handle, handle->arg_value, CMDQ_NOT_EQUAL, handle->arg_source);
	return 0;
}

static s32 cmdq_get_event_op_id(enum CMDQ_EVENT_ENUM event)
{
	s32 event_id = 0;

	if (event < 0 || CMDQ_SYNC_TOKEN_MAX <= event) {
		CMDQ_ERR("Invalid input event:%d\n", (s32)event);
		return -EINVAL;
	}

	event_id = cmdq_core_get_event_value(event);
	if (event_id < 0) {
		CMDQ_ERR("Invalid event:%d ID:%d\n", (s32)event, (s32)event_id);
		return -EINVAL;
	}

	return event_id;
}

int32_t cmdq_op_wait(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a = cmdq_get_event_op_id(event);

	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_WFE, arg_a, 0, 0, 0);
}

int32_t cmdq_op_wait_no_clear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a = cmdq_get_event_op_id(event);

	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_WAIT_NO_CLEAR, arg_a, 0, 0, 0);
}

int32_t cmdq_op_clear_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a = cmdq_get_event_op_id(event);

	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_CLEAR_TOKEN, arg_a, 1,	/* actually this param is ignored. */
				   0, 0);
}

int32_t cmdq_op_set_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a = cmdq_get_event_op_id(event);

	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_SET_TOKEN, arg_a, 1,	/* actually this param is ignored. */
				   0, 0);
}

s32 cmdq_op_replace_overwrite_cpr(struct cmdqRecStruct *handle, u32 index,
	s32 new_arg_a, s32 new_arg_b, s32 new_arg_c)
{
	/* check instruction is wait or not */
	uint32_t *p_command;
	uint32_t offsetIndex = index * CMDQ_INST_SIZE;

	if (!handle)
		return -EFAULT;

	if (offsetIndex > (handle->blockSize - CMDQ_INST_SIZE)) {
		CMDQ_ERR("======REC overwrite offset (%d) invalid: %d\n", offsetIndex, handle->blockSize);
		return -EFAULT;
	}

	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + offsetIndex);
	if (new_arg_a >= 0)
		p_command[1] = (p_command[1] & 0xffff0000) | (new_arg_a & 0xffff);
	if (new_arg_b >= 0)
		p_command[0] = (p_command[0] & 0x0000ffff) | ((new_arg_b & 0xffff) << 16);
	if (new_arg_c >= 0)
		p_command[0] = (p_command[0] & 0xffff0000) | (new_arg_c & 0xffff);
	CMDQ_MSG("======REC 0x%p replace cpr cmd (%d): 0x%08x 0x%08x\n", handle, index, p_command[0], p_command[1]);

	return 0;
}

int32_t cmdq_op_read_to_data_register(struct cmdqRecStruct *handle, uint32_t hw_addr,
				  enum CMDQ_DATA_REGISTER_ENUM dst_data_reg)
{
#ifdef CMDQ_GPR_SUPPORT
	enum CMDQ_CODE_ENUM op_code;
	uint32_t arg_a_i, arg_b_i;
	uint32_t arg_a_type, arg_b_type;

	if (dst_data_reg < CMDQ_DATA_REG_JPEG_DST) {
		op_code = CMDQ_CODE_READ_S;
		arg_a_i = dst_data_reg + CMDQ_GPR_V3_OFFSET;
		arg_a_type = 1;
		arg_b_i = hw_addr;
		arg_b_type = 0;
	} else {
		op_code = CMDQ_CODE_READ;
		arg_a_i = hw_addr;
		arg_a_type = 0;
		arg_b_i = dst_data_reg;
		arg_b_type = 1;
	}

	/* read from hwRegAddr(arg_a) to dstDataReg(arg_b) */
	return cmdq_append_command(handle, op_code, arg_a_i, arg_b_i, arg_a_type, arg_b_type);
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdq_op_write_from_data_register(struct cmdqRecStruct *handle,
				     enum CMDQ_DATA_REGISTER_ENUM src_data_reg, uint32_t hw_addr)
{
#ifdef CMDQ_GPR_SUPPORT
	enum CMDQ_CODE_ENUM op_code;
	uint32_t arg_b_i;

	if (src_data_reg < CMDQ_DATA_REG_JPEG_DST) {
		op_code = CMDQ_CODE_WRITE_S;
		arg_b_i = src_data_reg + CMDQ_GPR_V3_OFFSET;
	} else {
		op_code = CMDQ_CODE_WRITE;
		arg_b_i = src_data_reg;
	}

	/* write HW register(arg_a) with data of GPR data register(arg_b) */
	return cmdq_append_command(handle, op_code, hw_addr, arg_b_i, 0, 1);
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Allocate 32-bit register backup slot
 *
 */
int32_t cmdq_alloc_mem(cmdqBackupSlotHandle *p_h_backup_slot, uint32_t slotCount)
{
#ifdef CMDQ_GPR_SUPPORT

	dma_addr_t paStart = 0;
	int status = 0;

	if (p_h_backup_slot == NULL)
		return -EINVAL;

	status = cmdqCoreAllocWriteAddress(slotCount, &paStart, NULL);
	*p_h_backup_slot = paStart;

	return status;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Read 32-bit register backup slot by index
 *
 */
int32_t cmdq_cpu_read_mem(cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index,
			   uint32_t *value)
{
#ifdef CMDQ_GPR_SUPPORT

	if (value == NULL)
		return -EINVAL;

	if (h_backup_slot == 0) {
		CMDQ_ERR("%s, h_backup_slot is NULL\n", __func__);
		return -EINVAL;
	}

	*value = cmdqCoreReadWriteAddress(h_backup_slot + slot_index * sizeof(uint32_t));

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdq_cpu_write_mem(cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index,
			    uint32_t value)
{
#ifdef CMDQ_GPR_SUPPORT

	int status = 0;
	/* set the slot value directly */
	status = cmdqCoreWriteWriteAddress(h_backup_slot + slot_index * sizeof(uint32_t), value);
	return status;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Free allocated backup slot. DO NOT free them before corresponding
 *  task finishes. Becareful on AsyncFlush use cases.
 *
 */
int32_t cmdq_free_mem(cmdqBackupSlotHandle h_backup_slot)
{
#ifdef CMDQ_GPR_SUPPORT
	return cmdqCoreFreeWriteAddress(h_backup_slot);
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Insert instructions to backup given 32-bit HW register
 *  to a backup slot.
 *  You can use cmdq_cpu_read_mem() to retrieve the result
 *  AFTER cmdq_task_flush() returns, or INSIDE the callback of cmdq_task_flush_async_callback().
 *
 */
s32 cmdq_op_read_reg_to_mem(struct cmdqRecStruct *handle,
	cmdqBackupSlotHandle h_backup_slot, u32 slot_index, u32 addr)
{
	const dma_addr_t dram_addr = h_backup_slot + slot_index * sizeof(u32);
	CMDQ_VARIABLE var_mem_addr = CMDQ_TASK_TEMP_CPR_VAR;
	s32 status;

	do {
		status = cmdq_op_read_reg(handle, addr,
			&handle->arg_value, ~0);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_op_assign(handle, &var_mem_addr, (u32)dram_addr);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_append_command(handle, CMDQ_CODE_WRITE_S,
			(u32)(var_mem_addr & 0xFFFF),
			(u32)(handle->arg_value & 0xFFFF),
			1, 1);
	} while (0);

	return status;
}

s32 cmdq_op_read_mem_to_reg(struct cmdqRecStruct *handle,
	cmdqBackupSlotHandle h_backup_slot, u32 slot_index, u32 addr)
{
	const dma_addr_t dram_addr = h_backup_slot + slot_index * sizeof(u32);
	CMDQ_VARIABLE var_mem_addr = CMDQ_TASK_TEMP_CPR_VAR;
	s32 status;

	do {
		status = cmdq_create_variable_if_need(handle,
			&handle->arg_value);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_op_assign(handle, &var_mem_addr, (u32)dram_addr);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		/* read dram to temp var */
		status = cmdq_append_command(handle, CMDQ_CODE_READ_S,
			(u32)(handle->arg_value & 0xFFFF),
			(u32)(var_mem_addr & 0xFFFF), 1, 1);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_op_write_reg(handle, addr,
			handle->arg_value, ~0);
	} while (0);

	return status;
}

s32 cmdq_op_write_mem(struct cmdqRecStruct *handle,
	cmdqBackupSlotHandle h_backup_slot, u32 slot_index, u32 value)
{
	const dma_addr_t dram_addr = h_backup_slot + slot_index * sizeof(u32);
	CMDQ_VARIABLE var_mem_addr = CMDQ_TASK_TEMP_CPR_VAR;
	s32 status;

	do {
		status = cmdq_op_assign(handle, &handle->arg_value, value);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_op_assign(handle, &var_mem_addr, (u32)dram_addr);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_append_command(handle, CMDQ_CODE_WRITE_S,
			var_mem_addr, handle->arg_value, 1, 1);
	} while (0);

	return status;
}

int32_t cmdq_op_finalize_command(struct cmdqRecStruct *handle, bool loop)
{
	int32_t status = 0;
	uint32_t arg_b = 0;

	if (handle == NULL)
		return -EFAULT;

	if (handle->if_stack_node != NULL) {
		CMDQ_ERR("Incorrect if-else statement, please review your if-else instructions.");
		return -EFAULT;
	}

	if (handle->while_stack_node != NULL) {
		CMDQ_ERR("Incorrect while statement, please review your while instructions.");
		return -EFAULT;
	}

	if (!handle->finalized) {
		if ((handle->prefetchCount > 0)
		    && cmdq_get_func()->shouldEnablePrefetch(handle->scenario)) {
			CMDQ_ERR
			    ("not insert prefetch disble marker when prefetch enabled, prefetchCount:%d\n",
			     handle->prefetchCount);
			cmdq_task_dump_command(handle);

			status = -EFAULT;
			return status;
		}

		/* insert EOF instruction */
		arg_b = 0x1;	/* generate IRQ for each command iteration */
#ifdef DISABLE_LOOP_IRQ
		if (loop == true && cmdq_get_func()->force_loop_irq(handle->scenario) == false)
			arg_b = 0x0;	/* no generate IRQ for loop thread to save power */
#endif
		/* no generate IRQ for delay loop thread */
		if (handle->scenario == CMDQ_SCENARIO_TIMER_LOOP)
			arg_b = 0x0;

		status = cmdq_append_command(handle, CMDQ_CODE_EOC, 0, arg_b, 0, 0);

		if (status != 0)
			return status;

		/* insert JUMP to loop to beginning or as a scheduling mark(8) */
		status = cmdq_append_command(handle, CMDQ_CODE_JUMP, 0,	/* not absolute */
					     loop ? -handle->blockSize : 8, 0, 0);
		if (status != 0)
			return status;

		handle->finalized = true;
	}

	return status;
}

int32_t cmdq_setup_sec_data_of_command_desc_by_rec_handle(struct cmdqCommandStruct *pDesc,
							      struct cmdqRecStruct *handle)
{
	/* fill field from user's request */
	pDesc->secData.is_secure = handle->secData.is_secure;
	pDesc->secData.enginesNeedDAPC = handle->secData.enginesNeedDAPC;
	pDesc->secData.enginesNeedPortSecurity = handle->secData.enginesNeedPortSecurity;

	pDesc->secData.addrMetadataCount = handle->secData.addrMetadataCount;
	pDesc->secData.addrMetadatas = handle->secData.addrMetadatas;
	pDesc->secData.addrMetadataMaxCount = handle->secData.addrMetadataMaxCount;

	/* init reserved field */
	pDesc->secData.resetExecCnt = false;
	pDesc->secData.waitCookie = 0;

	return 0;
}

int32_t cmdq_setup_replace_of_command_desc_by_rec_handle(struct cmdqCommandStruct *pDesc,
							      struct cmdqRecStruct *handle)
{
	/* fill field from user's request */
	pDesc->replace_instr.number = handle->replace_instr.number;
	pDesc->replace_instr.position = handle->replace_instr.position;

	return 0;
}

int32_t cmdq_rec_setup_profile_marker_data(struct cmdqCommandStruct *pDesc, struct cmdqRecStruct *handle)
{
	uint32_t i;

	pDesc->profileMarker.count = handle->profileMarker.count;
	pDesc->profileMarker.hSlot = handle->profileMarker.hSlot;

	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		pDesc->profileMarker.tag[i] = handle->profileMarker.tag[i];

	return 0;
}

int32_t cmdq_task_flush(struct cmdqRecStruct *handle)
{
	int32_t status;
	struct cmdqCommandStruct desc = { 0 };

	status = cmdq_op_finalize_command(handle, false);
	if (status < 0)
		return status;

	CMDQ_MSG("Submit task scenario: %d, priority: %d, engine: 0x%llx, buffer: 0x%p, size: %d\n",
		 handle->scenario, handle->priority, handle->engineFlag, handle->pBuffer,
		 handle->blockSize);

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.blockSize = handle->blockSize;
	desc.prop_addr = (cmdqU32Ptr_t) (unsigned long)handle->prop_addr;
	desc.prop_size = handle->prop_size;
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc, &handle->ext);
}

int32_t cmdq_task_flush_and_read_register(struct cmdqRecStruct *handle, uint32_t regCount,
			    uint32_t *addrArray, uint32_t *valueArray)
{
	int32_t status;
	struct cmdqCommandStruct desc = { 0 };

	status = cmdq_op_finalize_command(handle, false);
	if (status < 0)
		return status;

	CMDQ_MSG("Submit task scenario: %d, priority: %d, engine: 0x%llx, buffer: 0x%p, size: %d\n",
		 handle->scenario, handle->priority, handle->engineFlag, handle->pBuffer,
		 handle->blockSize);

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.blockSize = handle->blockSize;
	desc.regRequest.count = regCount;
	desc.regRequest.regAddresses = (cmdqU32Ptr_t) (unsigned long)addrArray;
	desc.regValue.count = regCount;
	desc.regValue.regValues = (cmdqU32Ptr_t) (unsigned long)valueArray;
	desc.prop_addr = (cmdqU32Ptr_t) (unsigned long)handle->prop_addr;
	desc.prop_size = handle->prop_size;
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc, &handle->ext);
}

int32_t cmdq_task_flush_async(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	struct cmdqCommandStruct desc = { 0 };
	struct TaskStruct *pTask = NULL;

	status = cmdq_op_finalize_command(handle, false);
	if (status < 0)
		return status;

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.blockSize = handle->blockSize;
	desc.regRequest.count = 0;
	desc.regRequest.regAddresses = (cmdqU32Ptr_t) (unsigned long)NULL;
	desc.regValue.count = 0;
	desc.regValue.regValues = (cmdqU32Ptr_t) (unsigned long)NULL;
	desc.prop_addr = (cmdqU32Ptr_t) (unsigned long)handle->prop_addr;
	desc.prop_size = handle->prop_size;
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, &handle->ext, NULL, 0, &pTask);

	CMDQ_MSG
	    ("[Auto Release] Submit ASYNC task scenario: %d, priority: %d, engine: 0x%llx, buffer: 0x%p, size: %d\n",
	     handle->scenario, handle->priority, handle->engineFlag, handle->pBuffer,
	     handle->blockSize);

	if (pTask) {
		pTask->flushCallback = NULL;
		pTask->flushData = 0;
	}

	/* insert the task into auto-release queue */
	if (pTask)
		status = cmdqCoreAutoReleaseTask(pTask);
	else
		status = -ENOMEM;

	return status;
}

int32_t cmdq_task_flush_async_callback(struct cmdqRecStruct *handle,
	CmdqAsyncFlushCB callback, u64 user_data)
{
	int32_t status = 0;
	struct cmdqCommandStruct desc = { 0 };
	struct TaskStruct *pTask = NULL;

	status = cmdq_op_finalize_command(handle, false);
	if (status < 0)
		return status;

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.blockSize = handle->blockSize;
	desc.regRequest.count = 0;
	desc.regRequest.regAddresses = (cmdqU32Ptr_t) (unsigned long)NULL;
	desc.regValue.count = 0;
	desc.regValue.regValues = (cmdqU32Ptr_t) (unsigned long)NULL;
	desc.prop_addr = (cmdqU32Ptr_t) (unsigned long)handle->prop_addr;
	desc.prop_size = handle->prop_size;
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, &handle->ext, NULL, 0, &pTask);

	/* insert the callback here. */
	/* note that, the task may be already completed at this point. */
	if (pTask) {
		pTask->flushCallback = callback;
		pTask->flushData = user_data;
	}

	CMDQ_MSG
	    ("[Auto Release] Submit ASYNC task scenario: %d, priority: %d, engine: 0x%llx, buffer: 0x%p, size: %d\n",
	     handle->scenario, handle->priority, handle->engineFlag, handle->pBuffer,
	     handle->blockSize);

	/* insert the task into auto-release queue */
	if (pTask)
		status = cmdqCoreAutoReleaseTask(pTask);
	else
		status = -ENOMEM;

	return status;
}

static int32_t cmdq_dummy_irq_callback(unsigned long data)
{
	return 0;
}

s32 _cmdq_task_start_loop_callback(struct cmdqRecStruct *handle,
								CmdqInterruptCB loopCB, unsigned long loopData,
								const char *SRAM_owner_name)
{
	int32_t status = 0;
	struct cmdqCommandStruct desc = { 0 };

	if (handle == NULL)
		return -EFAULT;

	if (handle->pRunningTask != NULL)
		return -EBUSY;

	status = cmdq_op_finalize_command(handle, true);
	if (status < 0)
		return status;

	CMDQ_MSG("Submit task loop: scenario: %d, priority: %d, engine: 0x%llx,",
			   handle->scenario, handle->priority, handle->engineFlag);
	CMDQ_MSG("Submit task loop: buffer: 0x%p, size: %d, callback: 0x%p, data: %ld\n",
			   handle->pBuffer, handle->blockSize, loopCB, loopData);

	desc.scenario = handle->scenario;
	desc.priority = handle->priority;
	desc.engineFlag = handle->engineFlag;
	desc.blockSize = handle->blockSize;
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.prop_addr = (cmdqU32Ptr_t) (unsigned long)handle->prop_addr;
	desc.prop_size = handle->prop_size;
	if (strlen(SRAM_owner_name) > 0) {
		CMDQ_MSG("Submit task loop in SRAM: %s\n", SRAM_owner_name);
		desc.use_sram_buffer = true;
		strncpy(desc.sram_owner_name, SRAM_owner_name, sizeof(desc.sram_owner_name) - 1);
	}
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTaskAsync(&desc, &handle->ext, loopCB, loopData, &handle->pRunningTask);
}

int32_t cmdq_task_start_loop(struct cmdqRecStruct *handle)
{
	return _cmdq_task_start_loop_callback(handle, &cmdq_dummy_irq_callback, 0, "");
}

int32_t cmdq_task_start_loop_callback(struct cmdqRecStruct *handle, CmdqInterruptCB loopCB, unsigned long loopData)
{
	return _cmdq_task_start_loop_callback(handle, loopCB, loopData, "");
}

s32 cmdq_task_start_loop_sram(struct cmdqRecStruct *handle, const char *SRAM_owner_name)
{
	if (handle->secData.is_secure) {
		CMDQ_ERR("SRAM task does not support secure task\n");
		return -EINVAL;
	}

	return _cmdq_task_start_loop_callback(handle, &cmdq_dummy_irq_callback, 0, SRAM_owner_name);
}

int32_t cmdq_task_stop_loop(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	struct TaskStruct *pTask;

	if (handle == NULL)
		return -EFAULT;

	pTask = handle->pRunningTask;
	if (pTask == NULL)
		return -EFAULT;

	status = cmdqCoreReleaseTask(pTask);
	handle->pRunningTask = NULL;
	return status;
}

s32 cmdq_task_copy_to_sram(dma_addr_t pa_src, u32 sram_dest, size_t size)
{
	struct cmdqRecStruct *handle;
	u32 i;
	s32 status;
	unsigned long long duration;
	CMDQ_VARIABLE pa_cpr, sram_cpr;

	CMDQ_MSG("%s DRAM addr: 0x%pa, SRAM addr: %d\n", __func__, &pa_src, sram_dest);

	cmdq_op_init_variable(&pa_cpr);
	cmdq_task_create(CMDQ_SCENARIO_MOVE, &handle);
	cmdq_task_reset(handle);
	handle->priority = CMDQ_REC_MAX_PRIORITY;

	for (i = 0; i < size / sizeof(u32); i++) {
		cmdq_op_assign(handle, &pa_cpr, (u32)pa_src + i * sizeof(u32));
		cmdq_op_init_global_cpr_variable(&sram_cpr, sram_dest + i);
		cmdq_append_command(handle, CMDQ_CODE_READ_S,
			(u32)sram_cpr, (u32)pa_cpr, 1, 1);
	}

	duration = sched_clock();
	status = cmdq_task_flush(handle);

	duration = sched_clock() - duration;
	CMDQ_MSG("%s result: %d, cost time: %u us\n", __func__, status, (u32)duration);

	cmdq_task_destroy(handle);
	return status;
}

s32 cmdq_task_copy_from_sram(dma_addr_t pa_dest, u32 sram_src, size_t size)
{
	struct cmdqRecStruct *handle;
	u32 i;
	unsigned long long duration;
	CMDQ_VARIABLE pa_cpr, sram_cpr;

	CMDQ_MSG("%s DRAM addr: 0x%pa, SRAM addr: %d\n", __func__, &pa_dest, sram_src);
	cmdq_op_init_variable(&pa_cpr);
	cmdq_task_create(CMDQ_SCENARIO_MOVE, &handle);
	cmdq_task_reset(handle);

	for (i = 0; i < size / sizeof(u32); i++) {
		cmdq_op_assign(handle, &pa_cpr, (u32)pa_dest + i * sizeof(u32));
		sram_cpr = CMDQ_ARG_CPR_START + sram_src + i;
		cmdq_append_command(handle, CMDQ_CODE_WRITE_S,
			(u32)pa_cpr, (u32)sram_cpr, 1, 1);
	}
	/*cmdq_task_dump_command(handle);*/

	duration = sched_clock();
	cmdq_task_flush(handle);

	duration = sched_clock() - duration;
	CMDQ_MSG("%s cost time: %u us\n", __func__, (u32)duration);

	cmdq_task_destroy(handle);
	return 0;
}

int32_t cmdq_task_get_instruction_count(struct cmdqRecStruct *handle)
{
	int32_t instruction_count;

	if (handle == NULL)
		return 0;

	instruction_count = handle->blockSize / CMDQ_INST_SIZE;

	return instruction_count;
}

int32_t cmdq_op_profile_marker(struct cmdqRecStruct *handle, const char *tag)
{
	int32_t status;
	int32_t index;
	cmdqBackupSlotHandle hSlot;
	dma_addr_t allocatedStartPA;

	do {
		allocatedStartPA = 0;
		status = 0;

		/* allocate temp slot for GCE to store timestamp info */
		/* those timestamp info will copy to record strute after task execute done */
		if ((handle->profileMarker.count == 0) && (handle->profileMarker.hSlot == 0)) {
			status =
			    cmdqCoreAllocWriteAddress(CMDQ_MAX_PROFILE_MARKER_IN_TASK,
						      &allocatedStartPA, NULL);
			if (status < 0) {
				CMDQ_ERR("[REC][PROF_MARKER]allocate failed, status:%d\n", status);
				break;
			}

			handle->profileMarker.hSlot = 0LL | (allocatedStartPA);

			CMDQ_VERBOSE
			    ("[REC][PROF_MARKER]update handle(%p) slot start PA:%pa(0x%llx)\n",
			     handle, &allocatedStartPA, handle->profileMarker.hSlot);
		}

		/* insert instruciton */
		index = handle->profileMarker.count;
		hSlot = (cmdqBackupSlotHandle) (handle->profileMarker.hSlot);

		if (index >= CMDQ_MAX_PROFILE_MARKER_IN_TASK) {
			CMDQ_ERR
			    ("[REC][PROF_MARKER]insert profile maker failed since already reach max count\n");
			status = -EFAULT;
			break;
		}

		CMDQ_VERBOSE
		    ("[REC][PROF_MARKER]inserting profile instr, handle:%p, slot:%pa(0x%llx), index:%d, tag:%s\n",
		     handle, &hSlot, handle->profileMarker.hSlot, index, tag);

		cmdq_op_backup_TPR(handle, hSlot, index);

		handle->profileMarker.tag[index] = (cmdqU32Ptr_t) (unsigned long)tag;
		handle->profileMarker.count += 1;
	} while (0);

	return status;
}

int32_t cmdq_task_dump_command(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	struct TaskStruct *pTask;

	if (handle == NULL)
		return -EFAULT;

	pTask = handle->pRunningTask;
	if (pTask) {
		/* running, so dump from core task direct */
		status = cmdqCoreDebugDumpCommand(pTask);
	} else {
		/* not running, dump from rec->pBuffer */
		const uint32_t *pCmd = NULL;
		static char textBuf[128] = { 0 };
		int i = 0;

		CMDQ_LOG("======REC 0x%p command buffer:\n", handle->pBuffer);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 16, 4,
			       handle->pBuffer, handle->blockSize, false);

		CMDQ_LOG("======REC 0x%p command buffer END\n", handle->pBuffer);
		CMDQ_LOG("REC 0x%p command buffer TRANSLATED:\n", handle->pBuffer);
		for (i = 0, pCmd = handle->pBuffer; i < handle->blockSize; i += 8, pCmd += 2) {
			cmdq_core_parse_instruction(pCmd, textBuf, 128);
			CMDQ_LOG("%s", textBuf);
		}
		CMDQ_LOG("======REC 0x%p command END\n", handle->pBuffer);

		return 0;
	}

	return status;
}

int32_t cmdq_task_estimate_command_exec_time(const struct cmdqRecStruct *handle)
{
	int32_t time = 0;

	if (handle == NULL)
		return -EFAULT;

	CMDQ_LOG("======REC 0x%p command execution time ESTIMATE:\n", handle);
	time = cmdq_prof_estimate_command_exe_time(handle->pBuffer, handle->blockSize);
	CMDQ_LOG("======REC 0x%p  END\n", handle);

	return time;
}

int32_t cmdq_task_destroy(struct cmdqRecStruct *handle)
{
	s32 status = 0;

	status = cmdq_task_reset(handle);
	if (status < 0)
		return status;

	/* Free command buffer */
	vfree(handle->pBuffer);
	handle->pBuffer = NULL;

	cmdq_task_release_property(handle);

	kfree(handle);

	return 0;
}

int32_t cmdq_op_set_nop(struct cmdqRecStruct *handle, uint32_t index)
{
	uint32_t *p_command;
	uint32_t offsetIndex = index * CMDQ_INST_SIZE;

	if (handle == NULL || offsetIndex > (handle->blockSize - CMDQ_INST_SIZE))
		return -EFAULT;

	CMDQ_MSG("======REC 0x%p Set NOP to index: %d\n", handle, index);
	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + offsetIndex);
	*p_command++ = 8;
	*p_command++ = (CMDQ_CODE_JUMP << 24) | (0 & 0x0FFFFFF);
	CMDQ_MSG("======REC 0x%p  END\n", handle);

	return index;
}

int32_t cmdq_task_query_offset(struct cmdqRecStruct *handle, uint32_t startIndex,
				  const enum CMDQ_CODE_ENUM opCode, enum CMDQ_EVENT_ENUM event)
{
	int32_t Offset = -1;
	uint32_t arg_a, arg_b;
	uint32_t *p_command;
	uint32_t QueryIndex, MaxIndex;

	if (handle == NULL || (startIndex * CMDQ_INST_SIZE) > (handle->blockSize - CMDQ_INST_SIZE))
		return -EFAULT;

	switch (opCode) {
	case CMDQ_CODE_WFE:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		arg_b = ((1 << 31) | (1 << 15) | 1);
		arg_a = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_SET_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 1 */
		arg_b = ((1 << 31) | (1 << 16));
		arg_a = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_WAIT_NO_CLEAR:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, false */
		arg_b = ((0 << 31) | (1 << 15) | 1);
		arg_a = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_CLEAR_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		arg_b = ((1 << 31) | (0 << 16));
		arg_a = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_PREFETCH_ENABLE:
		/* this is actually MARKER but with different parameter */
		/* interpretation */
		/* bit 53: non_suspendable, true */
		/* bit 48: no_inc_exec_cmds_cnt, true */
		/* bit 20: prefetch_marker, true */
		/* bit 17: prefetch_marker_en, true */
		/* bit 16: prefetch_en, true */
		arg_b = ((1 << 20) | (1 << 17) | (1 << 16));
		arg_a = (CMDQ_CODE_EOC << 24) | (0x1 << (53 - 32)) | (0x1 << (48 - 32));
		break;
	case CMDQ_CODE_PREFETCH_DISABLE:
		/* this is actually MARKER but with different parameter */
		/* interpretation */
		/* bit 48: no_inc_exec_cmds_cnt, true */
		/* bit 20: prefetch_marker, true */
		arg_b = (1 << 20);
		arg_a = (CMDQ_CODE_EOC << 24) | (0x1 << (48 - 32));
		break;
	default:
		CMDQ_MSG("This offset of instruction can not be queried.\n");
		return -EFAULT;
	}

	MaxIndex = handle->blockSize / CMDQ_INST_SIZE;
	for (QueryIndex = startIndex; QueryIndex < MaxIndex; QueryIndex++) {
		p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + QueryIndex * CMDQ_INST_SIZE);
		if ((arg_b == *p_command++) && (arg_a == *p_command)) {
			Offset = (int32_t) QueryIndex;
			CMDQ_MSG("Get offset = %d\n", Offset);
			break;
		}
	}
	if (Offset < 0) {
		/* Can not find the offset of desired instruction */
		CMDQ_LOG("Can not find the offset of desired instruction\n");
	}

	return Offset;
}

int32_t cmdq_resource_acquire(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	bool result = false;

	result = cmdqCoreAcquireResource(resourceEvent, &handle->ext.res_engine_flag_acquire);
	if (!result) {
		CMDQ_MSG("Acquire resource (event:%d) failed, handle:0x%p\n", resourceEvent, handle);
		return -EFAULT;
	}
	return 0;
}

int32_t cmdq_resource_acquire_and_write(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	s32 status = cmdq_resource_acquire(handle, resourceEvent);

	if (status < 0)
		return status;

	return cmdq_op_write_reg(handle, addr, value, mask);
}

int32_t cmdq_resource_release(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	cmdqCoreReleaseResource(resourceEvent, &handle->ext.res_engine_flag_release);
	return cmdq_op_set_event(handle, resourceEvent);
}

int32_t cmdq_resource_release_and_write(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t result;

	cmdqCoreReleaseResource(resourceEvent, &handle->ext.res_engine_flag_release);
	result = cmdq_op_write_reg(handle, addr, value, mask);
	if (result >= 0)
		return cmdq_op_set_event(handle, resourceEvent);

	CMDQ_ERR("Write instruction fail and not release resource!\n");
	return result;
}

s32 cmdq_task_create_delay_thread_dram(void **pp_delay_thread_buffer, u32 *buffer_size)
{
	struct cmdqRecStruct *handle;
	void *p_new_buffer = NULL;
	uint32_t i;
	CMDQ_VARIABLE arg_delay_cpr_start, arg_delay_set_cpr_start;
	CMDQ_VARIABLE arg_delay_set_start, arg_delay_set_duration, arg_delay_set_result;
	CMDQ_VARIABLE temp_cpr = CMDQ_ARG_CPR_START + CMDQ_DELAY_THREAD_ID * CMDQ_THR_CPR_MAX;
	CMDQ_VARIABLE arg_tpr = CMDQ_TASK_TPR_VAR;
	CMDQ_VARIABLE spr0 = CMDQ_TASK_TEMP_CPR_VAR,
				  spr1 = CMDQ_TASK_TEMP_CPR_VAR + 1,
				  spr2 = CMDQ_TASK_TEMP_CPR_VAR + 2,
				  spr3 = CMDQ_TASK_TEMP_CPR_VAR + 3;

	cmdq_task_create(CMDQ_SCENARIO_TIMER_LOOP, &handle);
	cmdq_task_reset(handle);
	arg_delay_cpr_start = CMDQ_ARG_CPR_START + cmdq_core_get_delay_start_cpr();

	cmdq_op_wait(handle, CMDQ_SYNC_TOKEN_TIMER);
	cmdq_op_write_reg(handle, CMDQ_TPR_MASK_PA, CMDQ_DELAY_TPR_MASK_VALUE,
		CMDQ_DELAY_TPR_MASK_VALUE);

	cmdq_op_wait(handle, CMDQ_EVENT_TIMER_00 + CMDQ_DELAY_TPR_MASK_BIT);
	for (i = 0; i < CMDQ_DELAY_MAX_SET; i++) {
		arg_delay_set_cpr_start = arg_delay_cpr_start + CMDQ_DELAY_SET_MAX_CPR * i;
		arg_delay_set_start = arg_delay_set_cpr_start + CMDQ_DELAY_SET_START_CPR;
		arg_delay_set_duration = arg_delay_set_cpr_start + CMDQ_DELAY_SET_DURATION_CPR;
		arg_delay_set_result = arg_delay_set_cpr_start + CMDQ_DELAY_SET_RESULT_CPR;

		cmdq_op_if(handle, arg_delay_set_duration, CMDQ_GREATER_THAN, 0);
		cmdq_op_subtract(handle, &temp_cpr, arg_tpr, arg_delay_set_start);
		cmdq_op_add(handle, &spr0, arg_delay_set_start, 0);
		cmdq_op_add(handle, &spr1, arg_delay_set_duration, 0);
		cmdq_op_add(handle, &spr2, arg_tpr, 0);
		cmdq_op_add(handle, &spr3, temp_cpr, 0);
		cmdq_op_if(handle, temp_cpr, CMDQ_GREATER_THAN_AND_EQUAL, arg_delay_set_duration);
		cmdq_op_set_event(handle, CMDQ_SYNC_TOKEN_DELAY_SET0+i);
		cmdq_op_assign(handle, &arg_delay_set_duration, 0);
		cmdq_op_add(handle, &arg_delay_set_result, temp_cpr, 0);
		cmdq_op_end_if(handle);
		cmdq_op_end_if(handle);
	}

	cmdq_op_finalize_command(handle, true);
	*buffer_size = handle->blockSize;

	if (handle->blockSize <= 0 || handle->pBuffer == NULL) {
		CMDQ_ERR("REC: create delay thread fail, block_size = %d\n", handle->blockSize);
		cmdq_task_destroy(handle);
		return -EFAULT;
	}

	p_new_buffer = kzalloc(handle->blockSize, GFP_KERNEL);
	if (p_new_buffer == NULL) {
		CMDQ_ERR("REC: kzalloc %d bytes delay_thread_buffer failed\n", handle->blockSize);
		cmdq_task_destroy(handle);
		return -ENOMEM;
	}

	memcpy(p_new_buffer, handle->pBuffer, handle->blockSize);

	if (*pp_delay_thread_buffer != NULL)
		kfree(*pp_delay_thread_buffer);
	*pp_delay_thread_buffer = p_new_buffer;

	cmdq_task_destroy(handle);

	return 0;
}

s32 cmdq_task_create_delay_thread_sram(void **pp_delay_thread_buffer, u32 *buffer_size, u32 *cpr_offset)
{
#define INIT_MIN_DELAY (0xffffffff)
	struct cmdqRecStruct *handle;
	void *p_new_buffer = NULL;
	u32 i;
	u32 wait_tpr_index = 0;
	u32 wait_tpr_arg_a = 0;
	u32 replace_overwrite_index[2] = {0};
	u32 replace_overwrite_debug = 0;
	CMDQ_VARIABLE arg_delay_cpr_start, arg_delay_set_cpr_start;
	CMDQ_VARIABLE arg_delay_set_start, arg_delay_set_duration, arg_delay_set_result;
	CMDQ_VARIABLE temp_cpr = CMDQ_ARG_CPR_START + CMDQ_DELAY_THREAD_ID * CMDQ_THR_CPR_MAX;
	CMDQ_VARIABLE arg_tpr = CMDQ_TASK_TPR_VAR;
	CMDQ_VARIABLE spr_delay = CMDQ_TASK_TEMP_CPR_VAR,
			spr_temp = CMDQ_TASK_TEMP_CPR_VAR + 1,
			spr_min_delay = CMDQ_TASK_TEMP_CPR_VAR + 2,
			spr_debug = CMDQ_TASK_TEMP_CPR_VAR + 3;

	if (pp_delay_thread_buffer == NULL || buffer_size == NULL || cpr_offset == NULL)
		return -EINVAL;

	cmdq_task_create(CMDQ_SCENARIO_TIMER_LOOP, &handle);
	cmdq_task_reset(handle);

	arg_delay_cpr_start = CMDQ_ARG_CPR_START + cmdq_core_get_delay_start_cpr();

	cmdq_op_wait(handle, CMDQ_SYNC_TOKEN_TIMER);
	wait_tpr_index = cmdq_task_get_instruction_count(handle) - 1;
	wait_tpr_arg_a = wait_tpr_index * 2 + 1;
	cmdq_op_write_reg(handle, CMDQ_TPR_MASK_PA, CMDQ_DELAY_TPR_MASK_VALUE,
		CMDQ_DELAY_TPR_MASK_VALUE);
	cmdq_op_assign(handle, &spr_min_delay, INIT_MIN_DELAY);
	cmdq_op_assign(handle, &temp_cpr, INIT_MIN_DELAY);

	for (i = 0; i < CMDQ_DELAY_MAX_SET; i++) {
		/* check and update min delay */
		arg_delay_set_cpr_start = arg_delay_cpr_start + CMDQ_DELAY_SET_MAX_CPR * i;
		arg_delay_set_start = arg_delay_set_cpr_start + CMDQ_DELAY_SET_START_CPR;
		arg_delay_set_duration = arg_delay_set_cpr_start + CMDQ_DELAY_SET_DURATION_CPR;
		arg_delay_set_result = arg_delay_set_cpr_start + CMDQ_DELAY_SET_RESULT_CPR;

		cmdq_op_if(handle, arg_delay_set_duration, CMDQ_GREATER_THAN, 0);
			cmdq_op_subtract(handle, &spr_temp, arg_tpr, arg_delay_set_start);
			cmdq_op_if(handle, spr_temp, CMDQ_GREATER_THAN_AND_EQUAL, arg_delay_set_duration);
				cmdq_op_set_event(handle, CMDQ_SYNC_TOKEN_DELAY_SET0 + i);
				cmdq_op_assign(handle, &arg_delay_set_duration, 0);
				cmdq_op_add(handle, &arg_delay_set_result, temp_cpr, 0);
			cmdq_op_else(handle);
				cmdq_op_subtract(handle, &spr_delay, arg_delay_set_duration, spr_temp);
				cmdq_op_if(handle, spr_delay, CMDQ_LESS_THAN, spr_min_delay);
					cmdq_op_add(handle, &spr_min_delay, spr_delay, 0);
				cmdq_op_end_if(handle);
			cmdq_op_end_if(handle);
		cmdq_op_end_if(handle);
	}

	cmdq_op_if(handle, spr_min_delay, CMDQ_EQUAL, temp_cpr);
		/* no one is waiting, reset wait event value */
		cmdq_op_write_reg(handle, CMDQ_TPR_MASK_PA, 0,
			CMDQ_DELAY_TPR_MASK_VALUE);
		cmdq_op_assign(handle, &spr_temp, CMDQ_EVENT_ARGA(CMDQ_SYNC_TOKEN_TIMER));
		replace_overwrite_index[0] = cmdq_task_get_instruction_count(handle) - 1;
	cmdq_op_else(handle);
		/* wait precisely */
		cmdq_op_assign(handle, &spr_temp, CMDQ_EVENT_ARGA(CMDQ_EVENT_TIMER_11));
		replace_overwrite_index[1] = cmdq_task_get_instruction_count(handle) - 1;
	cmdq_op_end_if(handle);

	cmdq_op_add(handle, &spr_debug, spr_temp, 0);
	replace_overwrite_debug = cmdq_task_get_instruction_count(handle) - 1;

	cmdq_op_finalize_command(handle, true);

	*buffer_size = handle->blockSize;
	if (handle->blockSize <= 0 || handle->pBuffer == NULL) {
		CMDQ_ERR("REC: create delay thread fail, block_size = %d\n", handle->blockSize);
		cmdq_task_destroy(handle);
		return -EFAULT;
	}

	if (cmdq_core_alloc_sram_buffer(handle->blockSize, "DELAY_THREAD", cpr_offset) < 0) {
		CMDQ_ERR("REC: create SRAM fail, block_size = %d\n", handle->blockSize);
		return -EFAULT;
	}

	CMDQ_LOG("overwrite delay function with cpr_offset = %u\n", *cpr_offset);
	for (i = 0; i < ARRAY_SIZE(replace_overwrite_index); i++) {
		cmdq_op_replace_overwrite_cpr(handle, replace_overwrite_index[i],
			CMDQ_CPR_STRAT_ID + *cpr_offset + wait_tpr_arg_a, -1, -1);
	}

	if (replace_overwrite_debug > 0) {
		cmdq_op_replace_overwrite_cpr(handle, replace_overwrite_debug,
			-1, CMDQ_CPR_STRAT_ID + *cpr_offset + wait_tpr_arg_a, -1);
	}

	cmdq_task_dump_command(handle);

	p_new_buffer = kzalloc(handle->blockSize, GFP_KERNEL);
	if (p_new_buffer == NULL) {
		CMDQ_ERR("REC: kzalloc %d bytes delay_thread_buffer failed\n", handle->blockSize);
		cmdq_task_destroy(handle);
		return -ENOMEM;
	}

	memcpy(p_new_buffer, handle->pBuffer, handle->blockSize);

	if (*pp_delay_thread_buffer != NULL)
		kfree(*pp_delay_thread_buffer);
	*pp_delay_thread_buffer = p_new_buffer;

	cmdq_task_destroy(handle);
	return 0;
}

static int32_t cmdq_append_logic_command(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_a,
			    CMDQ_VARIABLE arg_b, enum CMDQ_LOGIC_ENUM s_op, CMDQ_VARIABLE arg_c)
{
	int32_t status = 0;
	uint32_t arg_a_i, arg_b_i, arg_c_i;
	uint32_t arg_a_type, arg_b_type, arg_c_type, arg_abc_type;
	uint32_t *p_command;

	status = cmdq_check_before_append(handle);
	if (status < 0) {
		CMDQ_ERR("	  cannot add logic command (s_op: %d, arg_b: 0x%08x, arg_c: 0x%08x)\n",
			s_op, (uint32_t)(arg_b & 0xFFFFFFFF), (uint32_t)(arg_c & 0xFFFFFFFF));
		return status;
	}

	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	do {
		/* get actual arg_b_i & arg_b_type */
		status = cmdq_var_data_type(arg_b, &arg_b_i, &arg_b_type);
		if (status < 0)
			break;

		/* get actual arg_c_i & arg_c_type */
		status = cmdq_var_data_type(arg_c, &arg_c_i, &arg_c_type);
		if (status < 0)
			break;

		/* get arg_a register by using module storage manager */
		status = cmdq_create_variable_if_need(handle, arg_a);
		if (status < 0)
			break;

		/* get actual arg_a_i & arg_a_type */
		status = cmdq_var_data_type(*arg_a, &arg_a_i, &arg_a_type);
		if (status < 0)
			break;

		/* arg_a always be SW register */
		arg_abc_type = (1 << 2) | (arg_b_type << 1) | (arg_c_type);

		if (handle->ext.exclusive_thread != CMDQ_INVALID_THREAD) {
			u32 cpr_offset = CMDQ_CPR_STRAT_ID + CMDQ_THR_CPR_MAX *
				handle->ext.exclusive_thread;

			/* change cpr to thread cpr directly,
			 * if we already have exclusive thread.
			 */
			if (cmdq_is_cpr(arg_a_i, arg_a_type))
				arg_a_i = cpr_offset + (arg_a_i -
					CMDQ_THR_SPR_MAX);
			if (cmdq_is_cpr(arg_b_i, arg_b_type))
				arg_b_i = cpr_offset + (arg_b_i -
					CMDQ_THR_SPR_MAX);
			if (cmdq_is_cpr(arg_c_i, arg_c_type))
				arg_c_i = cpr_offset + (arg_c_i -
					CMDQ_THR_SPR_MAX);
		} else if (cmdq_is_cpr(arg_a_i, arg_a_type) ||
			cmdq_is_cpr(arg_b_i, arg_b_type) ||
			cmdq_is_cpr(arg_c_i, arg_c_type)) {
			/* save local variable position */
			CMDQ_MSG(
				"save logic: sop:%d arg_a:0x%08x arg_b:0x%08x arg_c:0x%08x arg_abc_type:%d\n",
				 s_op, arg_a_i, arg_b_i, arg_c_i,
				 arg_abc_type);
			cmdq_save_op_variable_position(handle,
				cmdq_task_get_instruction_count(handle));
		}

		*p_command++ = (arg_b_i << 16) | (arg_c_i);
		*p_command++ = (CMDQ_CODE_LOGIC << 24) | (arg_abc_type << 21) |
			(s_op << 16) | (*arg_a & 0xFFFFFFFF);

		handle->blockSize += CMDQ_INST_SIZE;
	} while (0);

	return status;
}

void cmdq_op_init_variable(CMDQ_VARIABLE *arg)
{
	*arg = CMDQ_TASK_CPR_INITIAL_VALUE;
}

void cmdq_op_init_global_cpr_variable(CMDQ_VARIABLE *arg, u32 cpr_offset)
{
	*arg = CMDQ_ARG_CPR_START + cpr_offset;
}

int32_t cmdq_op_assign(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out, CMDQ_VARIABLE arg_in)
{
	CMDQ_VARIABLE arg_b, arg_c;

	if (CMDQ_BIT_VALUE == (arg_in >> CMDQ_DATA_BIT)) {
		arg_c = (arg_in & 0x0000FFFF);
		arg_b = ((arg_in>>16) & 0x0000FFFF);
	} else {
		CMDQ_ERR("Assign only use value, can not append new command");
		return -EFAULT;
	}

	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_ASSIGN, arg_c);
}

int32_t cmdq_op_add(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_ADD, arg_c);
}

int32_t cmdq_op_subtract(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_SUBTRACT, arg_c);
}

int32_t cmdq_op_multiply(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_MULTIPLY, arg_c);
}

int32_t cmdq_op_xor(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_XOR, arg_c);
}

int32_t cmdq_op_not(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out, CMDQ_VARIABLE arg_b)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_NOT, 0);
}

int32_t cmdq_op_or(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_OR, arg_c);
}

int32_t cmdq_op_and(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_AND, arg_c);
}

int32_t cmdq_op_left_shift(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_LEFT_SHIFT, arg_c);
}

int32_t cmdq_op_right_shift(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
				   CMDQ_VARIABLE arg_b, CMDQ_VARIABLE arg_c)
{
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_RIGHT_SHIFT, arg_c);
}

s32 cmdq_op_delay_us(struct cmdqRecStruct *handle, u32 delay_time)
{
	s32 status = 0;
	const CMDQ_VARIABLE arg_tpr = CMDQ_TASK_TPR_VAR;
	CMDQ_VARIABLE arg_delay_cpr_start = CMDQ_ARG_CPR_START + cmdq_core_get_delay_start_cpr();
	CMDQ_VARIABLE delay_set_start_cpr, delay_set_duration_cpr;
	u32 delay_TPR_value = delay_time*26;
	s32 delay_id = cmdq_get_delay_id_by_scenario(handle->scenario);

	if (delay_id < 0) {
		CMDQ_ERR("Handle scenario does not support delay.");
		return -EINVAL;
	}

	if (delay_time > 80000000) {
		CMDQ_ERR("delay time should not over 80s");
		return -EINVAL;
	}

	CMDQ_MSG("Delay: %u, TPR value: 0x%x\n", delay_time, delay_TPR_value);
	delay_set_start_cpr = arg_delay_cpr_start + CMDQ_DELAY_SET_MAX_CPR * delay_id
							+ CMDQ_DELAY_SET_START_CPR;
	delay_set_duration_cpr = arg_delay_cpr_start + CMDQ_DELAY_SET_MAX_CPR * delay_id
							+ CMDQ_DELAY_SET_DURATION_CPR;

	do {
		/* Insert TPR addition instruction */
		status = cmdq_op_add(handle, &delay_set_start_cpr, arg_tpr, 0);
		status = cmdq_op_clear_event(handle, CMDQ_SYNC_TOKEN_DELAY_SET0 + delay_id);
		status = cmdq_op_assign(handle, &delay_set_duration_cpr, delay_TPR_value);
		status = cmdq_op_set_event(handle, CMDQ_SYNC_TOKEN_TIMER);
		/* Insert dummy wait event */
		status = cmdq_op_wait(handle, CMDQ_SYNC_TOKEN_DELAY_SET0 + delay_id);
	} while (0);

	return status;
}

s32 cmdq_op_backup_CPR(struct cmdqRecStruct *handle, CMDQ_VARIABLE cpr,
	cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index)
{
	s32 status = 0;
	CMDQ_VARIABLE pa_cpr = CMDQ_TASK_TEMP_CPR_VAR;
	const dma_addr_t dramAddr = h_backup_slot + slot_index * sizeof(u32);

	cmdq_op_assign(handle, &pa_cpr, (u32)dramAddr);
	cmdq_append_command(handle, CMDQ_CODE_WRITE_S,
		(u32)pa_cpr, (u32)cpr, 1, 1);

	return status;
}

s32 cmdq_op_backup_TPR(struct cmdqRecStruct *handle,
	cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index)
{
	s32 status = 0;
	CMDQ_VARIABLE pa_cpr = CMDQ_TASK_TEMP_CPR_VAR;
	const CMDQ_VARIABLE arg_tpr = (CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_TPR_ID;
	const dma_addr_t dramAddr = h_backup_slot + slot_index * sizeof(u32);

	cmdq_op_assign(handle, &pa_cpr, (u32)dramAddr);
	cmdq_append_command(handle, CMDQ_CODE_WRITE_S,
		(u32)pa_cpr, (u32)arg_tpr, 1, 1);

	return status;
}

enum CMDQ_CONDITION_ENUM cmdq_reverse_op_condition(enum CMDQ_CONDITION_ENUM arg_condition)
{
	enum CMDQ_CONDITION_ENUM instr_condition = CMDQ_CONDITION_ERROR;

	switch (arg_condition) {
	case CMDQ_EQUAL:
		instr_condition = CMDQ_NOT_EQUAL;
		break;
	case CMDQ_NOT_EQUAL:
		instr_condition = CMDQ_EQUAL;
		break;
	case CMDQ_GREATER_THAN_AND_EQUAL:
		instr_condition = CMDQ_LESS_THAN;
		break;
	case CMDQ_LESS_THAN_AND_EQUAL:
		instr_condition = CMDQ_GREATER_THAN;
		break;
	case CMDQ_GREATER_THAN:
		instr_condition = CMDQ_LESS_THAN_AND_EQUAL;
		break;
	case CMDQ_LESS_THAN:
		instr_condition = CMDQ_GREATER_THAN_AND_EQUAL;
		break;
	default:
		CMDQ_ERR("Incorrect CMDQ condition statement (%d), can not append new command\n",
			arg_condition);
		break;
	}

	return instr_condition;
}

int32_t cmdq_append_jump_c_command(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   enum CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	s32 status = 0;
	const s32 dummy_address = 8;
	enum CMDQ_CONDITION_ENUM instr_condition;
	u32 arg_a_i, arg_b_i, arg_c_i;
	u32 arg_a_type, arg_b_type, arg_c_type, arg_abc_type;
	u32 *p_command;
	CMDQ_VARIABLE arg_temp_cpr = CMDQ_TASK_TEMP_CPR_VAR;
	bool always_jump_abs = handle->scenario != CMDQ_SCENARIO_TIMER_LOOP;

	if (likely(always_jump_abs)) {
		/*
		 * Insert write_s to write address to SPR1,
		 * since we may change relative to absolute jump later,
		 * and use this write_s instruction to set destination PA address.
		 */
		status = cmdq_op_assign(handle, &arg_temp_cpr, dummy_address);
		if (status < 0)
			return status;
	}

	status = cmdq_check_before_append(handle);
	if (status < 0) {
		CMDQ_ERR("	  cannot add jump_c command (condition: %d, arg_b: 0x%08x, arg_c: 0x%08x)\n",
			arg_condition, (u32)(arg_b & 0xFFFFFFFF), (u32)(arg_c & 0xFFFFFFFF));
		return status;
	}

	p_command = (u32 *) ((u8 *) handle->pBuffer + handle->blockSize);

	do {
		/* reverse condition statement */
		instr_condition = cmdq_reverse_op_condition(arg_condition);
		if (instr_condition < 0) {
			status = -EFAULT;
			break;
		}

		if (likely(always_jump_abs)) {
			/* arg_a always be register SPR1 */
			status = cmdq_var_data_type(arg_temp_cpr, &arg_a_i, &arg_a_type);
			if (status < 0)
				break;
		} else {
			arg_a_type = 0;
			arg_a_i = dummy_address;
		}

		/* get actual arg_b_i & arg_b_type */
		status = cmdq_var_data_type(arg_b, &arg_b_i, &arg_b_type);
		if (status < 0)
			break;

		/* get actual arg_c_i & arg_c_type */
		status = cmdq_var_data_type(arg_c, &arg_c_i, &arg_c_type);
		if (status < 0)
			break;

		arg_abc_type = (arg_a_type << 2) | (arg_b_type << 1) | (arg_c_type);
		if (cmdq_is_cpr(arg_b_i, arg_b_type) || cmdq_is_cpr(arg_c_i, arg_c_type)) {
			/* save local variable position */
			CMDQ_MSG("save jump_c: condition:%d, arg_b: 0x%08x, arg_c: 0x%08x, arg_abc_type: %d\n",
				 arg_condition, arg_b_i, arg_c_i, arg_abc_type);
		}
		if ((arg_c_i & 0xFFFF0000) != 0)
			CMDQ_ERR("jump_c arg_c value is over 16 bit: 0x%08x\n", arg_c_i);

		*p_command++ = (arg_b_i << 16) | (arg_c_i);
		*p_command++ = (CMDQ_CODE_JUMP_C_RELATIVE << 24) | (arg_abc_type << 21) |
			(instr_condition << 16) | (arg_a_i);

		/* save position to replace write value later and cpu use in jump_c */
		cmdq_save_op_variable_position(handle, cmdq_task_get_instruction_count(handle));

		handle->blockSize += CMDQ_INST_SIZE;
	} while (0);

	return status;
}

s32 cmdq_op_rewrite_jump_c(struct cmdqRecStruct *handle, u32 rewritten_position, u32 new_value)
{
	u32 op, op_arg_type, op_jump;
	u32 *p_command;

	if (handle == NULL)
		return -EFAULT;

	p_command = (u32 *) ((u8 *) handle->pBuffer + rewritten_position);

	if (likely(handle->scenario != CMDQ_SCENARIO_TIMER_LOOP)) {
		/* reserve condition statement */
		op = (p_command[1] & 0xFF000000) >> 24;
		op_jump = (p_command[3] & 0xFF000000) >> 24;
		if (op != CMDQ_CODE_LOGIC || op_jump != CMDQ_CODE_JUMP_C_RELATIVE)
			return -EFAULT;

		/* rewrite actual jump value */
		op_arg_type = (p_command[0] & 0xFFFF0000) >> 16;
		p_command[0] = (op_arg_type << 16) | (new_value - rewritten_position - CMDQ_INST_SIZE);
	} else {
		op = (p_command[1] & 0xFF000000) >> 24;
		if (op != CMDQ_CODE_JUMP_C_RELATIVE)
			return -EFAULT;

		/* rewrite actual jump value */
		op_arg_type = (p_command[1] & 0xFFFF0000) >> 16;
		p_command[1] = (op_arg_type << 16) | (((s32)new_value - (s32)rewritten_position) & 0xFFFF);
	}

	return 0;
}

int32_t cmdq_op_if(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   enum CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	int32_t status = 0;
	uint32_t current_position;

	if (handle == NULL)
		return -EFAULT;

	do {
		current_position = handle->blockSize;

		/* append conditional jump instruction */
		status = cmdq_append_jump_c_command(handle, arg_b, arg_condition, arg_c);
		if (status < 0)
			break;

		/* handle if-else stack */
		status = cmdq_op_condition_push(&handle->if_stack_node, current_position, CMDQ_STACK_TYPE_IF);
	} while (0);

	return status;
}

int32_t cmdq_op_end_if(struct cmdqRecStruct *handle)
{
	int32_t status = 0, ifCount = 1;
	uint32_t rewritten_position = 0;
	enum CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

	if (handle == NULL)
		return -EFAULT;

	do {
		/* check if-else stack */
		status = cmdq_op_condition_query(handle->if_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0)
			break;

		if (rewritten_stack_type == CMDQ_STACK_TYPE_IF) {
			if (ifCount <= 0)
				break;
		} else if (rewritten_stack_type != CMDQ_STACK_TYPE_ELSE)
			break;
		ifCount--;

		/* handle if-else stack */
		status = cmdq_op_condition_pop(&handle->if_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0) {
			CMDQ_ERR("failed to pop cmdq_stack_node\n");
			break;
		}

		cmdq_op_rewrite_jump_c(handle, rewritten_position, handle->blockSize);
	} while (1);

	return status;
}

int32_t cmdq_op_else(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	uint32_t current_position, rewritten_position;
	enum CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

	if (handle == NULL)
		return -EFAULT;

	do {
		current_position = handle->blockSize;

		/* check if-else stack */
		status = cmdq_op_condition_query(handle->if_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0) {
			CMDQ_ERR("failed to query cmdq_stack_node\n");
			break;
		}

		if (rewritten_stack_type != CMDQ_STACK_TYPE_IF) {
			CMDQ_ERR("Incorrect command, please review your if-else instructions.");
			status = -EFAULT;
			break;
		}

		/* append conditional jump instruction */
		status = cmdq_append_jump_c_command(handle, 1, CMDQ_NOT_EQUAL, 1);
		if (status < 0)
			break;

		/* handle if-else stack */
		status = cmdq_op_condition_pop(&handle->if_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0) {
			CMDQ_ERR("failed to pop cmdq_stack_node\n");
			break;
		}

		cmdq_op_rewrite_jump_c(handle, rewritten_position, handle->blockSize);

		status = cmdq_op_condition_push(&handle->if_stack_node, current_position, CMDQ_STACK_TYPE_ELSE);
	} while (0);

	return status;
}

int32_t cmdq_op_else_if(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   enum CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	int32_t status = 0;

	if (handle == NULL)
		return -EFAULT;

	do {
		/* handle else statement */
		status = cmdq_op_else(handle);
		if (status < 0)
			break;

		/* handle if statement */
		status = cmdq_op_if(handle, arg_b, arg_condition, arg_c);
	} while (0);

	return status;
}

int32_t cmdq_op_while(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   enum CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	int32_t status = 0;
	uint32_t current_position;

	if (handle == NULL)
		return -EFAULT;

	do {
		current_position = handle->blockSize;

		/* append conditional jump instruction */
		status = cmdq_append_jump_c_command(handle, arg_b, arg_condition, arg_c);
		if (status < 0)
			break;

		/* handle while stack */
		status = cmdq_op_condition_push(&handle->while_stack_node, current_position, CMDQ_STACK_TYPE_WHILE);
	} while (0);

	return status;
}

s32 cmdq_op_continue(struct cmdqRecStruct *handle)
{
	const u32 op_while_bit = 1 << CMDQ_STACK_TYPE_WHILE;
	const u32 op_do_while_bit = 1 << CMDQ_STACK_TYPE_DO_WHILE;
	s32 status = 0;
	u32 current_position;
	s32 rewritten_position;
	u32 *p_command;
	const struct cmdq_stack_node *op_node = NULL;

	if (handle == NULL)
		return -EFAULT;

	p_command = (u32 *)((u8 *) handle->pBuffer + handle->blockSize);

	do {
		current_position = handle->blockSize;

		/* query while/do while position from the stack */
		rewritten_position = cmdq_op_condition_find_op_type(handle->while_stack_node, current_position,
			op_while_bit | op_do_while_bit, &op_node);

		if (!op_node) {
			status = -EFAULT;
			break;
		}

		if (op_node->stack_type == CMDQ_STACK_TYPE_WHILE) {
			/*
			 * use jump command to start of while statement,
			 * since jump_c cannot process negative number
			 */
			status = cmdq_append_command(handle, CMDQ_CODE_JUMP, 0, rewritten_position, 0, 0);
		} else if (op_node->stack_type == CMDQ_STACK_TYPE_DO_WHILE) {
			/* append conditional jump instruction to jump end op while */
			status = cmdq_append_jump_c_command(handle, 1, CMDQ_NOT_EQUAL, 1);
			if (status < 0)
				break;
			status = cmdq_op_condition_push(&handle->while_stack_node, current_position,
				CMDQ_STACK_TYPE_CONTINUE);
		}
	} while (0);

	return status;
}

s32 cmdq_op_break(struct cmdqRecStruct *handle)
{
	const u32 op_while_bit = 1 << CMDQ_STACK_TYPE_WHILE;
	const u32 op_do_while_bit = 1 << CMDQ_STACK_TYPE_DO_WHILE;
	const struct cmdq_stack_node *op_node = NULL;
	s32 status = 0;
	u32 current_position;
	s32 while_position;

	if (handle == NULL)
		return -EFAULT;

	do {
		current_position = handle->blockSize;

		/* query while position from the stack */
		while_position = cmdq_op_condition_find_op_type(handle->while_stack_node, current_position,
			op_while_bit | op_do_while_bit, &op_node);
		if (while_position >= 0) {
			CMDQ_ERR("Incorrect break command, please review your while statement.");
			status = -EFAULT;
			break;
		}

		/* append conditional jump instruction */
		status = cmdq_append_jump_c_command(handle, 1, CMDQ_NOT_EQUAL, 1);
		if (status < 0)
			break;

		/* handle while stack */
		status = cmdq_op_condition_push(&handle->while_stack_node, current_position, CMDQ_STACK_TYPE_BREAK);

	} while (0);

	return status;
}

int32_t cmdq_op_end_while(struct cmdqRecStruct *handle)
{
	int32_t status = 0, whileCount = 1;
	uint32_t rewritten_position;
	enum CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

	if (handle == NULL)
		return -EFAULT;

	/* append command to loop start position */
	status = cmdq_op_continue(handle);
	if (status < 0) {
		CMDQ_ERR("Cannot append end while, please review your while statement.");
		return status;
	}

	do {
		/* check while stack */
		status = cmdq_op_condition_query(handle->while_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0)
			break;

		if (rewritten_stack_type == CMDQ_STACK_TYPE_DO_WHILE) {
			CMDQ_ERR("Mix with while and do while in while loop\n");
			status = -EFAULT;
			break;
		} else if (rewritten_stack_type == CMDQ_STACK_TYPE_WHILE) {
			if (whileCount <= 0)
				break;
			whileCount--;
		} else if (rewritten_stack_type != CMDQ_STACK_TYPE_BREAK)
			break;

		/* handle while stack */
		status = cmdq_op_condition_pop(&handle->while_stack_node, &rewritten_position, &rewritten_stack_type);
		if (status < 0) {
			CMDQ_ERR("failed to pop cmdq_stack_node\n");
			break;
		}

		cmdq_op_rewrite_jump_c(handle, rewritten_position, handle->blockSize);
	} while (1);

	return status;
}

s32 cmdq_op_do_while(struct cmdqRecStruct *handle)
{
	s32 status = 0;
	u32 current_position;

	if (handle == NULL)
		return -EFAULT;

	current_position = handle->blockSize;
	/* handle while stack */
	status = cmdq_op_condition_push(&handle->while_stack_node, current_position, CMDQ_STACK_TYPE_DO_WHILE);
	return status;
}

s32 cmdq_op_end_do_while(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
	enum CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	s32 status = 0;
	u32 stack_op_position, condition_position;
	enum CMDQ_STACK_TYPE_ENUM stack_op_type;

	if (handle == NULL)
		return -EFAULT;

	/* mark position of end do while for continue */
	condition_position = handle->blockSize;

	/*
	 * Append conditional jump instruction and rewrite later.
	 * Reverse op since cmdq_append_jump_c_command always do reverse and jump.
	 */
	status = cmdq_append_jump_c_command(handle, arg_b, cmdq_reverse_op_condition(arg_condition), arg_c);

	do {
		u32 destination_position = handle->blockSize;

		/* check while stack */
		status = cmdq_op_condition_query(handle->while_stack_node, &stack_op_position, &stack_op_type);
		if (status < 0)
			break;

		if (stack_op_type == CMDQ_STACK_TYPE_WHILE) {
			CMDQ_ERR("Mix with while and do while in do-while loop\n");
			status = -EFAULT;
			break;
		} else if (stack_op_type == CMDQ_STACK_TYPE_DO_WHILE) {
			/* close do-while loop by jump to begin of do-while */
			status = cmdq_op_condition_pop(&handle->while_stack_node, &stack_op_position,
				&stack_op_type);
			cmdq_op_rewrite_jump_c(handle, condition_position, stack_op_position);
			break;
		} else if (stack_op_type == CMDQ_STACK_TYPE_CONTINUE) {
			/* jump to while condition to do check again */
			destination_position = condition_position;
		} else if (stack_op_type == CMDQ_STACK_TYPE_BREAK) {
			/* jump after check to skip current loop */
			destination_position = handle->blockSize;
		} else {
			/* unknown error type */
			CMDQ_ERR("Unknown stack type in do-while loop: %d\n", stack_op_type);
			break;
		}

		/* handle continue/break case in stack */
		status = cmdq_op_condition_pop(&handle->while_stack_node, &stack_op_position, &stack_op_type);
		if (status < 0) {
			CMDQ_ERR("failed to pop cmdq_stack_node in do-while loop\n");
			break;
		}
		cmdq_op_rewrite_jump_c(handle, stack_op_position, destination_position);
	} while (1);

	return status;
}

int32_t cmdq_op_read_reg(struct cmdqRecStruct *handle, uint32_t addr,
				   CMDQ_VARIABLE *arg_out, uint32_t mask)
{
	int32_t status = 0;
	CMDQ_VARIABLE mask_var = CMDQ_TASK_TEMP_CPR_VAR;
	uint32_t arg_a_i, arg_a_type;

	/* get arg_a register by using module storage manager */
	do {
		status = cmdq_create_variable_if_need(handle, arg_out);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		/* get actual arg_a_i & arg_a_type */
		status = cmdq_var_data_type(*arg_out, &arg_a_i, &arg_a_type);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		status = cmdq_append_command(handle, CMDQ_CODE_READ_S, arg_a_i, addr, arg_a_type, 0);
		CMDQ_CHECK_AND_BREAK_STATUS(status);

		if (mask != 0xFFFFFFFF) {
			if ((mask >> 16) > 0) {
				status = cmdq_op_assign(handle, &mask_var, mask);
				CMDQ_CHECK_AND_BREAK_STATUS(status);
				status = cmdq_op_and(handle, arg_out, *arg_out, mask_var);
			} else {
				status = cmdq_op_and(handle, arg_out, *arg_out, mask);
			}
		}
	} while (0);

	return status;
}

int32_t cmdq_op_read_mem(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			    uint32_t slot_index, CMDQ_VARIABLE *arg_out)
{
	return 0;
}

s32 cmdq_op_wait_event_timeout(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out,
		enum CMDQ_EVENT_ENUM wait_event, u32 timeout_time)
{
	/*
	* Simulate Code
	* start = TPR;
	* while (true) {
	*   arg_out = TPR - start
	*   x = wait_event;
	*   if (x == 1) {
	*     break;
	*   } else if (arg_out > timeout_time) {
	*     arg_out = 0;
	*     break;
	*   } else {
	*     delay (100 us);
	*   }
	* }
	*/

	CMDQ_VARIABLE arg_loop_debug = CMDQ_TASK_LOOP_DEBUG_VAR;
	const CMDQ_VARIABLE arg_tpr = CMDQ_TASK_TPR_VAR;
	u32 timeout_TPR_value = timeout_time*26;
	u32 wait_event_id = cmdq_core_get_event_value(wait_event);

	cmdq_op_add(handle, &handle->arg_value, arg_tpr, 0);
	cmdq_op_assign(handle, &handle->arg_timeout, timeout_TPR_value);
	cmdq_op_assign(handle, &arg_loop_debug, 0);
	cmdq_op_while(handle, 0, CMDQ_EQUAL, 0);
		cmdq_op_add(handle, &arg_loop_debug, arg_loop_debug, 1);
		cmdq_op_subtract(handle, arg_out, arg_tpr, handle->arg_value);
		/* get event value by APB register write and read */
		cmdq_op_write_reg(handle, CMDQ_SYNC_TOKEN_ID_PA, wait_event_id, 0x3FF);
		cmdq_op_read_reg(handle, CMDQ_SYNC_TOKEN_VAL_PA, &handle->arg_source, ~0);
		cmdq_op_if(handle, handle->arg_source, CMDQ_EQUAL, 1);
			cmdq_op_break(handle);
		cmdq_op_else_if(handle, *arg_out, CMDQ_GREATER_THAN_AND_EQUAL, handle->arg_timeout);
			cmdq_op_assign(handle, arg_out, 0);
			cmdq_op_break(handle);
		cmdq_op_else(handle);
			cmdq_op_delay_us(handle, 100);
		cmdq_op_end_if(handle);
	cmdq_op_end_while(handle);
	return 0;
}

int32_t cmdqRecCreate(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle)
{
	return cmdq_task_create(scenario, pHandle);
}

int32_t cmdqRecSetEngine(struct cmdqRecStruct *handle, uint64_t engineFlag)
{
	return cmdq_task_set_engine(handle, engineFlag);
}

int32_t cmdqRecReset(struct cmdqRecStruct *handle)
{
	return cmdq_task_reset(handle);
}

int32_t cmdqRecSetSecure(struct cmdqRecStruct *handle, const bool is_secure)
{
	return cmdq_task_set_secure(handle, is_secure);
}

int32_t cmdqRecIsSecure(struct cmdqRecStruct *handle)
{
	return cmdq_task_is_secure(handle);
}

int32_t cmdqRecSecureEnableDAPC(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	return cmdq_task_secure_enable_dapc(handle, engineFlag);
}

int32_t cmdqRecSecureEnablePortSecurity(struct cmdqRecStruct *handle, const uint64_t engineFlag)
{
	return cmdq_task_secure_enable_port_security(handle, engineFlag);
}

int32_t cmdqRecMark(struct cmdqRecStruct *handle)
{
	int32_t status;

	/* Do not check prefetch-ability here. */
	/* because cmdqRecMark may have other purposes. */

	/* bit 53: non-suspendable. set to 1 because we don't want */
	/* CPU suspend this thread during pre-fetching. */
	/* If CPU change PC, then there will be a mess, */
	/* because prefetch buffer is not properly cleared. */
	/* bit 48: do not increase CMD_COUNTER (because this is not the end of the task) */
	/* bit 20: prefetch_marker */
	/* bit 17: prefetch_marker_en */
	/* bit 16: prefetch_en */
	/* bit 0:  irq_en (set to 0 since we don't want EOC interrupt) */
	status = cmdq_append_command(handle,
				     CMDQ_CODE_EOC,
				     (0x1 << (53 - 32)) | (0x1 << (48 - 32)), 0x00130000, 0, 0);

	/* if we're in a prefetch region, */
	/* this ends the region so set count to 0. */
	/* otherwise we start the region by setting count to 1. */
	handle->prefetchCount = 1;

	if (status != 0)
		return -EFAULT;

	return 0;
}

int32_t cmdqRecWrite(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	return cmdq_op_write_reg(handle, addr, (CMDQ_VARIABLE)value, mask);
}

int32_t cmdqRecWriteSecure(struct cmdqRecStruct *handle, uint32_t addr,
			   enum CMDQ_SEC_ADDR_METADATA_TYPE type,
			   uint64_t baseHandle, uint32_t offset, uint32_t size, uint32_t port)
{
	return cmdq_op_write_reg_secure(handle, addr, type, baseHandle, offset, size, port);
}

int32_t cmdqRecPoll(struct cmdqRecStruct *handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	return cmdq_op_poll(handle, addr, value, mask);
}

int32_t cmdqRecWait(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	return cmdq_op_wait(handle, event);
}

int32_t cmdqRecWaitNoClear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	return cmdq_op_wait_no_clear(handle, event);
}

int32_t cmdqRecClearEventToken(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	return cmdq_op_clear_event(handle, event);
}

int32_t cmdqRecSetEventToken(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	return cmdq_op_set_event(handle, event);
}

int32_t cmdqRecReadToDataRegister(struct cmdqRecStruct *handle, uint32_t hw_addr,
				  enum CMDQ_DATA_REGISTER_ENUM dst_data_reg)
{
	return cmdq_op_read_to_data_register(handle, hw_addr, dst_data_reg);
}

int32_t cmdqRecWriteFromDataRegister(struct cmdqRecStruct *handle,
				     enum CMDQ_DATA_REGISTER_ENUM src_data_reg, uint32_t hw_addr)
{
	return cmdq_op_write_from_data_register(handle, src_data_reg, hw_addr);
}

int32_t cmdqBackupAllocateSlot(cmdqBackupSlotHandle *p_h_backup_slot, uint32_t slotCount)
{
	return cmdq_alloc_mem(p_h_backup_slot, slotCount);
}

int32_t cmdqBackupReadSlot(cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index, uint32_t *value)
{
	return cmdq_cpu_read_mem(h_backup_slot, slot_index, value);
}

int32_t cmdqBackupWriteSlot(cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index, uint32_t value)
{
	return cmdq_cpu_write_mem(h_backup_slot, slot_index, value);
}

int32_t cmdqBackupFreeSlot(cmdqBackupSlotHandle h_backup_slot)
{
	return cmdq_free_mem(h_backup_slot);
}

int32_t cmdqRecBackupRegisterToSlot(struct cmdqRecStruct *handle,
				    cmdqBackupSlotHandle h_backup_slot,
				    uint32_t slot_index, uint32_t regAddr)
{
	return cmdq_op_read_reg_to_mem(handle, h_backup_slot, slot_index, regAddr);
}

int32_t cmdqRecBackupWriteRegisterFromSlot(struct cmdqRecStruct *handle,
					   cmdqBackupSlotHandle h_backup_slot,
					   uint32_t slot_index, uint32_t addr)
{
	return cmdq_op_read_mem_to_reg(handle, h_backup_slot, slot_index, addr);
}

int32_t cmdqRecBackupUpdateSlot(struct cmdqRecStruct *handle,
				cmdqBackupSlotHandle h_backup_slot,
				uint32_t slot_index, uint32_t value)
{
	return cmdq_op_write_mem(handle, h_backup_slot, slot_index, value);
}

int32_t cmdqRecEnablePrefetch(struct cmdqRecStruct *handle)
{
#ifdef _CMDQ_DISABLE_MARKER_
	/* disable pre-fetch marker feature but use auto prefetch mechanism */
	CMDQ_MSG("not allow enable prefetch, scenario: %d\n", handle->scenario);
	return true;
#else
	if (handle == NULL)
		return -EFAULT;

	if (cmdq_get_func()->shouldEnablePrefetch(handle->scenario)) {
		/* enable prefetch */
		CMDQ_VERBOSE("REC: enable prefetch\n");
		cmdqRecMark(handle);
		return true;
	}
	CMDQ_ERR("not allow enable prefetch, scenario: %d\n", handle->scenario);
	return -EFAULT;
#endif
}

int32_t cmdqRecDisablePrefetch(struct cmdqRecStruct *handle)
{
	uint32_t arg_b = 0;
	uint32_t arg_a = 0;
	int32_t status = 0;

	if (handle == NULL)
		return -EFAULT;

	if (!handle->finalized) {
		if (handle->prefetchCount > 0) {
			/* with prefetch threads we should end with */
			/* bit 48: no_inc_exec_cmds_cnt = 1 */
			/* bit 20: prefetch_mark = 1 */
			/* bit 17: prefetch_mark_en = 0 */
			/* bit 16: prefetch_en = 0 */
			arg_b = 0x00100000;
			arg_a = (0x1 << 16);	/* not increse execute counter */
			/* since we're finalized, no more prefetch */
			handle->prefetchCount = 0;
			status = cmdq_append_command(handle, CMDQ_CODE_EOC, arg_a, arg_b, 0, 0);
		}

		if (status != 0)
			return status;
	}

	CMDQ_MSG("cmdqRecDisablePrefetch, status:%d\n", status);
	return status;
}

int32_t cmdqRecFlush(struct cmdqRecStruct *handle)
{
	return cmdq_task_flush(handle);
}

int32_t cmdqRecFlushAndReadRegister(struct cmdqRecStruct *handle, uint32_t regCount, uint32_t *addrArray,
				    uint32_t *valueArray)
{
	return cmdq_task_flush_and_read_register(handle, regCount, addrArray, valueArray);
}

int32_t cmdqRecFlushAsync(struct cmdqRecStruct *handle)
{
	return cmdq_task_flush_async(handle);
}

int32_t cmdqRecFlushAsyncCallback(struct cmdqRecStruct *handle,
	CmdqAsyncFlushCB callback, u64 user_data)
{
	return cmdq_task_flush_async_callback(handle, callback, user_data);
}

int32_t cmdqRecStartLoop(struct cmdqRecStruct *handle)
{
	return cmdq_task_start_loop(handle);
}

int32_t cmdqRecStartLoopWithCallback(struct cmdqRecStruct *handle, CmdqInterruptCB loopCB, unsigned long loopData)
{
	return cmdq_task_start_loop_callback(handle, loopCB, loopData);
}

int32_t cmdqRecStopLoop(struct cmdqRecStruct *handle)
{
	return cmdq_task_stop_loop(handle);
}

int32_t cmdqRecGetInstructionCount(struct cmdqRecStruct *handle)
{
	return cmdq_task_get_instruction_count(handle);
}

int32_t cmdqRecProfileMarker(struct cmdqRecStruct *handle, const char *tag)
{
	return cmdq_op_profile_marker(handle, tag);
}

int32_t cmdqRecDumpCommand(struct cmdqRecStruct *handle)
{
	return cmdq_task_dump_command(handle);
}

int32_t cmdqRecEstimateCommandExecTime(const struct cmdqRecStruct *handle)
{
	return cmdq_task_estimate_command_exec_time(handle);
}

void cmdqRecDestroy(struct cmdqRecStruct *handle)
{
	cmdq_task_destroy(handle);
}

int32_t cmdqRecSetNOP(struct cmdqRecStruct *handle, uint32_t index)
{
	return cmdq_op_set_nop(handle, index);
}

int32_t cmdqRecQueryOffset(struct cmdqRecStruct *handle, uint32_t startIndex, const enum CMDQ_CODE_ENUM opCode,
			   enum CMDQ_EVENT_ENUM event)
{
	return cmdq_task_query_offset(handle, startIndex, opCode, event);
}

int32_t cmdqRecAcquireResource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	return cmdq_resource_acquire(handle, resourceEvent);
}

int32_t cmdqRecWriteForResource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	return cmdq_resource_acquire_and_write(handle, resourceEvent, addr, value, mask);
}

int32_t cmdqRecReleaseResource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	return cmdq_resource_release(handle, resourceEvent);
}

int32_t cmdqRecWriteAndReleaseResource(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	return cmdq_resource_release_and_write(handle, resourceEvent, addr, value, mask);
}

s32 cmdq_task_update_property(struct cmdqRecStruct *handle, void *prop_addr, u32 prop_size)
{
	void *pprop_addr = NULL;

	if (!handle || !prop_addr || !prop_size) {
		CMDQ_ERR("Invalid input: handle=%p, prop_addr=%p, prop_size=%d\n",
			handle, prop_addr, prop_size);
		return -EINVAL;
	}

	cmdq_task_release_property(handle);

	/* copy another buffer so that we can release after used */
	pprop_addr = kzalloc(prop_size, GFP_KERNEL);
	if (!pprop_addr) {
		CMDQ_ERR("alloc pprop_addr memory failed\n");
		return -ENOMEM;
	}

	memcpy(pprop_addr, prop_addr, prop_size);
	handle->prop_addr = pprop_addr;
	handle->prop_size = prop_size;

	return 0;
}
