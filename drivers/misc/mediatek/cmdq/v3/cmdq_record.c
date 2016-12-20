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
#include <linux/errno.h>
#include <linux/memory.h>
#include <mt-plat/mtk_lpae.h>

#include "cmdq_record.h"
#include "cmdq_core.h"
#include "cmdq_virtual.h"
#include "cmdq_reg.h"
#include "cmdq_prof.h"

#ifdef CMDQ_SECURE_PATH_SUPPORT
#include "cmdq_sec_iwc_common.h"
#endif

#ifdef _MTK_USER_
#define DISABLE_LOOP_IRQ
#endif

#ifdef CMDQ_RECORD_V3
#define CMDQ_TASK_CPR_INITIAL_VALUE	(0)
#define CMDQ_TASK_CPR_LOWER_BOUND	((CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | 0)
#define CMDQ_TASK_CPR_UPPER_BOUND	((CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_THREAD_CPR_NUMBER)
#define CMDQ_TASK_DUMMY_DELAY_CPR	((CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_TASK_DUMMY_DELAY_VARIABLE)
#define CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE	(32)

/* push a value into a stack */
int32_t cmdq_op_condition_push(cmdq_stack_node **top_node, uint32_t position, CMDQ_STACK_TYPE_ENUM stack_type)
{
	/* allocate a new node for val */
	cmdq_stack_node *new_node = kmalloc(sizeof(cmdq_stack_node), GFP_KERNEL);

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
int32_t cmdq_op_condition_pop(cmdq_stack_node **top_node, uint32_t *position, CMDQ_STACK_TYPE_ENUM *stack_type)
{
	/* tmp for record the top node */
	cmdq_stack_node *temp_node;

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
int32_t cmdq_op_condition_query(const cmdq_stack_node *top_node, int *position, CMDQ_STACK_TYPE_ENUM *stack_type)
{
	*stack_type = CMDQ_STACK_NULL;

	if (top_node == NULL)
		return -1;

	/* get the value of the top */
	*position = top_node->position;
	*stack_type = top_node->stack_type;

	return 0;
}

/* query while position from the stack */
int32_t cmdq_op_condition_find_while(const cmdq_stack_node *top_node, const uint32_t position)
{
	const cmdq_stack_node *temp_node = top_node;
	uint32_t got_position = position;

	/* get the value of the top */
	do {
		if (top_node == NULL)
			break;

		if (temp_node->stack_type == CMDQ_STACK_TYPE_WHILE) {
			got_position = temp_node->position;
			break;
		}

		if (temp_node->next == NULL)
			break;

		temp_node = temp_node->next;
	} while (1);

	return (int32_t)(got_position - position);
}

static bool cmdq_is_cpr(uint32_t argument, uint32_t arg_type)
{
	if (arg_type == 1) {
		if (argument >= CMDQ_THREAD_SPR_NUMBER && argument < CMDQ_THREAD_MAX_LOCAL_VARIABLE)
			return true;

		if (argument == CMDQ_TASK_DUMMY_DELAY_VARIABLE)
			return true;
	}

	return false;
}

static void cmdq_save_op_variable_position(struct cmdqRecStruct *handle)
{
	uint32_t *p_new_buffer = NULL;
	uint32_t *p_instr_position = NULL;
	uint32_t array_num = 0;

	/* Exceed max number of SPR, use CPR */
	if ((handle->replace_instr.number % CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE) == 0) {
		array_num = (handle->replace_instr.number + CMDQ_TASK_CPR_POSITION_ARRAY_UNIT_SIZE) * sizeof(uint32_t);

		p_new_buffer = kzalloc(array_num, GFP_KERNEL);

		/* copy and release old buffer */
		if (handle->replace_instr.position != (cmdqU32Ptr_t) (unsigned long)NULL) {
			memcpy(p_new_buffer, CMDQ_U32_PTR(handle->replace_instr.position),
				handle->replace_instr.number * sizeof(uint32_t));
			kfree(CMDQ_U32_PTR(handle->replace_instr.position));
		}
		handle->replace_instr.position = (cmdqU32Ptr_t) (unsigned long)p_new_buffer;
	}

	p_instr_position = CMDQ_U32_PTR(handle->replace_instr.position);
	p_instr_position[handle->replace_instr.number] = cmdq_task_get_instruction_count(handle);
	handle->replace_instr.number++;
}

static int32_t cmdq_create_variable_if_need(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg)
{
	int32_t status = 0;

	do {
		if ((*arg >= CMDQ_TASK_CPR_LOWER_BOUND) && (*arg < CMDQ_TASK_CPR_UPPER_BOUND)) {
			/* Already dispatched */
			break;
		}

		if (*arg == CMDQ_TASK_DUMMY_DELAY_CPR) {
			/* Used for CMDQ delay */
			break;
		}

		if (handle->local_var_num > CMDQ_THREAD_MAX_LOCAL_VARIABLE) {
			CMDQ_ERR("Exceed max number of local variable in one task, please review your instructions.");
			status = -EFAULT;
			break;
		}

		*arg = ((CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | handle->local_var_num);
		handle->local_var_num++;
	} while (0);

	return status;
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
		CMDQ_ERR("Incorrect CMDQ data type, can not append new command");
		status = -EFAULT;
		break;
	}

	return status;
}
#endif				/* CMDQ_RECORD_V3 */

int32_t cmdq_reset_v3_struct(struct cmdqRecStruct *handle)
{
#ifdef CMDQ_RECORD_V3
	uint32_t destroy_position;
	CMDQ_STACK_TYPE_ENUM destroy_stack_type;

	if (handle == NULL)
		return -EFAULT;

	/* reset local variable setting */
	handle->local_var_num = 0;

	/* destroy sub-function data */
	if (handle->sub_function.reference_cnt > 0) {
		/* TODO */
		/* not implement now */
	}

	if (handle->sub_function.in_arg != NULL) {
		kfree(handle->sub_function.in_arg);
		handle->sub_function.in_arg = NULL;
	}

	if (handle->sub_function.out_arg != NULL) {
		kfree(handle->sub_function.out_arg);
		handle->sub_function.out_arg = NULL;
	}

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
#endif				/* CMDQ_RECORD_V3 */
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

	pNewBuf = kzalloc(size, GFP_KERNEL);

	if (pNewBuf == NULL) {
		CMDQ_ERR("REC: kzalloc %d bytes cmd_buffer failed\n", size);
		return -ENOMEM;
	}

	memset(pNewBuf, 0, size);

	if (handle->pBuffer && handle->blockSize > 0)
		memcpy(pNewBuf, handle->pBuffer, handle->blockSize);

	CMDQ_VERBOSE("REC: realloc size from %d to %d bytes\n", handle->bufferSize, size);

	kfree(handle->pBuffer);
	handle->pBuffer = pNewBuf;
	handle->bufferSize = size;

	return 0;
}

static int32_t cmdq_reset_profile_maker_data(struct cmdqRecStruct *handle)
{
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	int32_t i = 0;

	if (handle == NULL)
		return -EFAULT;

	handle->profileMarker.count = 0;
	handle->profileMarker.hSlot = 0LL;

	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		handle->profileMarker.tag[i] = (cmdqU32Ptr_t) (unsigned long)(NULL);

	return 0;
#endif
	return 0;
}

int32_t cmdq_task_create(enum CMDQ_SCENARIO_ENUM scenario, struct cmdqRecStruct **pHandle)
{
	struct cmdqRecStruct *handle = NULL;

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
	handle->blockSize = 0;
	handle->engineFlag = cmdq_get_func()->flagFromScenario(scenario);
	handle->priority = CMDQ_THR_PRIO_NORMAL;
	handle->prefetchCount = 0;
	handle->finalized = false;
	handle->pRunningTask = NULL;

	/* secure path */
	handle->secData.is_secure = false;
	handle->secData.enginesNeedDAPC = 0LL;
	handle->secData.enginesNeedPortSecurity = 0LL;
	handle->secData.addrMetadatas = (cmdqU32Ptr_t) (unsigned long)NULL;
	handle->secData.addrMetadataMaxCount = 0;
	handle->secData.addrMetadataCount = 0;

	/* profile marker */
	cmdq_reset_profile_maker_data(handle);

	/* CMD */
	if (cmdq_rec_realloc_cmd_buffer(handle, CMDQ_INITIAL_CMD_BLOCK_SIZE) != 0) {
		kfree(handle);
		return -ENOMEM;
	}

	*pHandle = handle;

	return 0;
}

#ifdef CMDQ_SECURE_PATH_SUPPORT
int32_t cmdq_append_addr_metadata(struct cmdqRecStruct *handle, const cmdqSecAddrMetadataStruct *pMetadata)
{
	cmdqSecAddrMetadataStruct *pAddrs;
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
		CMDQ_MSG("ADDR: type:%d, baseHandle:%x, offset:%d, size:%d, port:%d\n",
			 pMetadata->type, pMetadata->baseHandle, pMetadata->offset, pMetadata->size,
			 pMetadata->port);
		status = -EFAULT;
	} else {
		pAddrs =
		    (cmdqSecAddrMetadataStruct *) (CMDQ_U32_PTR(handle->secData.addrMetadatas));
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
#ifdef CMDQ_RECORD_V3
	int32_t status = 0;
	uint32_t *p_command;

	uint32_t new_arg_a, new_arg_b;
	uint32_t arg_addr, arg_value;
	uint32_t arg_addr_type, arg_value_type;
	uint32_t arg_type = 0;
	int32_t subsys = 0;

	/* be careful that subsys encoding position is different among platforms */
	const uint32_t subsys_bit = cmdq_get_func()->getSubsysLSBArgA();

	if (CMDQ_CODE_WRITE_S == code || CMDQ_CODE_WRITE_S_W_MASK == code) {
		/* For write_s command */
		arg_addr = arg_a;
		arg_addr_type = arg_a_type;
		arg_value = arg_b;
		arg_value_type = arg_b_type;
	} else if (CMDQ_CODE_READ_S == code || CMDQ_CODE_READ_S_W_MASK == code) {
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
			    (CMDQ_LOGIC_ASSIGN << 16) | CMDQ_SPR_USE_FOR_BACKUP;
			handle->blockSize += CMDQ_INST_SIZE;
			/* change final arg_addr to GPR */
			subsys = 0;
			arg_addr = CMDQ_SPR_USE_FOR_BACKUP;
			/* change arg_addr type to 1 */
			arg_addr_type = 1;
		} else if (arg_addr_type == 0 && subsys < 0) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", arg_addr);
			status = -EFAULT;
		}
	}

	if (status < 0)
		return status;

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

	if (true == cmdq_is_cpr(arg_a, arg_a_type)
		|| true == cmdq_is_cpr(arg_b, arg_b_type)) {
		/* save local variable position */
		CMDQ_LOG("op: 0x%02x, CMD: arg_a: 0x%08x, arg_b: 0x%08x, arg_a_type: %d, arg_b_type: %d\n",
		     code, arg_a, arg_b, arg_a_type, arg_b_type);
		cmdq_save_op_variable_position(handle);
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
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support v3 command\n", __func__);
	return -EFAULT;
#endif
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
	case CMDQ_CODE_READ_S_W_MASK:
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

	handle->blockSize = 0;
	handle->prefetchCount = 0;
	handle->finalized = false;

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

	return 0;
}

int32_t cmdq_task_set_secure(struct cmdqRecStruct *handle, const bool is_secure)
{
	if (handle == NULL)
		return -EFAULT;

	if (false == is_secure) {
		handle->secData.is_secure = is_secure;
		return 0;
	}
#ifdef CMDQ_SECURE_PATH_SUPPORT
	CMDQ_VERBOSE("REC: %p secure:%d\n", handle, is_secure);
	handle->secData.is_secure = is_secure;
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

#ifdef CMDQ_RECORD_V3
	if (mask != 0xFFFFFFFF)
		op_code = CMDQ_CODE_WRITE_S_W_MASK;
	else
		op_code = CMDQ_CODE_WRITE_S;

	status = cmdq_var_data_type(argument, &arg_b_i, &arg_b_type);
	if (status < 0)
		return status;
#else
	if (mask != 0xFFFFFFFF)
		addr = addr | 0x1;

	op_code = CMDQ_CODE_WRITE;
	arg_b_type = 0;
	arg_b_i = (uint32_t)(argument & 0xFFFFFFFF);
#endif

	return cmdq_append_command(handle, op_code, addr, arg_b_i, 0, arg_b_type);
}

int32_t cmdq_op_write_reg_secure(struct cmdqRecStruct *handle, uint32_t addr,
			   enum CMDQ_SEC_ADDR_METADATA_TYPE type, uint32_t baseHandle,
			   uint32_t offset, uint32_t size, uint32_t port)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status;
	int32_t writeInstrIndex;
	cmdqSecAddrMetadataStruct metadata;
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

int32_t cmdq_op_wait(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a;

	if (event < 0 || CMDQ_SYNC_TOKEN_MAX <= event)
		return -EINVAL;

	arg_a = cmdq_core_get_event_value(event);
	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_WFE, arg_a, 0, 0, 0);
}

int32_t cmdq_op_wait_no_clear(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a;

	if (event < 0 || CMDQ_SYNC_TOKEN_MAX <= event)
		return -EINVAL;

	arg_a = cmdq_core_get_event_value(event);
	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_WAIT_NO_CLEAR, arg_a, 0, 0, 0);
}

int32_t cmdq_op_clear_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a;

	if (event < 0 || CMDQ_SYNC_TOKEN_MAX <= event)
		return -EINVAL;

	arg_a = cmdq_core_get_event_value(event);
	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_CLEAR_TOKEN, arg_a, 1,	/* actually this param is ignored. */
				   0, 0);
}

int32_t cmdq_op_set_event(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM event)
{
	int32_t arg_a;

	if (event < 0 || CMDQ_SYNC_TOKEN_MAX <= event)
		return -EINVAL;

	arg_a = cmdq_core_get_event_value(event);
	if (arg_a < 0)
		return -EINVAL;

	return cmdq_append_command(handle, CMDQ_CODE_SET_TOKEN, arg_a, 1,	/* actually this param is ignored. */
				   0, 0);
}

int32_t cmdq_op_read_to_data_register(struct cmdqRecStruct *handle, uint32_t hw_addr,
				  enum CMDQ_DATA_REGISTER_ENUM dst_data_reg)
{
#ifdef CMDQ_GPR_SUPPORT
	enum CMDQ_CODE_ENUM op_code;
	uint32_t arg_a_i, arg_b_i;
	uint32_t arg_a_type, arg_b_type;

#ifdef CMDQ_RECORD_V3
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
#else
	op_code = CMDQ_CODE_READ;
	arg_a_i = hw_addr;
	arg_a_type = 0;
	arg_b_i = dst_data_reg;
	arg_b_type = 1;
#endif

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

#ifdef CMDQ_RECORD_V3
	if (src_data_reg < CMDQ_DATA_REG_JPEG_DST) {
		op_code = CMDQ_CODE_WRITE_S;
		arg_b_i = src_data_reg + CMDQ_GPR_V3_OFFSET;
	} else {
		op_code = CMDQ_CODE_WRITE;
		arg_b_i = src_data_reg;
	}
#else
	op_code = CMDQ_CODE_WRITE;
	arg_b_i = src_data_reg;
#endif

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

	status = cmdqCoreAllocWriteAddress(slotCount, &paStart);
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
int32_t cmdq_op_read_reg_to_mem(struct cmdqRecStruct *handle,
			    cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index, uint32_t addr)
{
#ifdef CMDQ_GPR_SUPPORT
	const enum CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const enum CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const enum CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = h_backup_slot + slot_index * sizeof(uint32_t);
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdq_op_wait(handle, regAccessToken);

	/* Load into 32-bit GPR (R0-R15) */
	cmdq_append_command(handle, CMDQ_CODE_READ, addr, valueRegId, 0, 1);

	/* Note that <MOVE> arg_b is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);

	/* write value in GPR to memory pointed by GPR */
	cmdq_append_command(handle, CMDQ_CODE_WRITE, destRegId, valueRegId, 1, 1);
	/* release the GPR lock */
	cmdq_op_set_event(handle, regAccessToken);

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdq_op_read_mem_to_reg(struct cmdqRecStruct *handle,
			    cmdqBackupSlotHandle h_backup_slot, uint32_t slot_index, uint32_t addr)
{
#ifdef CMDQ_GPR_SUPPORT
	const enum CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const enum CMDQ_DATA_REGISTER_ENUM addrRegId = CMDQ_DATA_REG_DEBUG_DST;
	const enum CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = h_backup_slot + slot_index * sizeof(uint32_t);
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdq_op_wait(handle, regAccessToken);

	/* 1. MOVE slot address to addr GPR */

	/* Note that <MOVE> arg_b is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((addrRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);	/* arg_a is GPR */

	/* 2. read value from src address, which is stroed in GPR, to valueRegId */
	cmdq_append_command(handle, CMDQ_CODE_READ, addrRegId, valueRegId, 1, 1);

	/* 3. write from data register */
	cmdq_op_write_from_data_register(handle, valueRegId, addr);

	/* release the GPR lock */
	cmdq_op_set_event(handle, regAccessToken);

	return 0;
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdq_op_write_mem(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			    uint32_t slot_index, uint32_t value)
{
#ifdef CMDQ_GPR_SUPPORT
	const enum CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const enum CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const enum CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = h_backup_slot + slot_index * sizeof(uint32_t);
	uint32_t arg_a;
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdq_op_wait(handle, regAccessToken);

	/* Assign 32-bit GRP with value */
	arg_a = (CMDQ_CODE_MOVE << 24) | (valueRegId << 16) | (4 << 21);	/* arg_a is GPR */
	cmdq_append_command(handle, CMDQ_CODE_RAW, arg_a, value, 0, 0);

	/* Note that <MOVE> arg_b is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);

	/* write value in GPR to memory pointed by GPR */
	cmdq_append_command(handle, CMDQ_CODE_WRITE, destRegId, valueRegId, 1, 1);

	/* release the GPR lock */
	cmdq_op_set_event(handle, regAccessToken);

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdq_op_finalize_command(struct cmdqRecStruct *handle, bool loop)
{
	int32_t status = 0;
	uint32_t arg_b = 0;

	if (handle == NULL)
		return -EFAULT;

#ifdef CMDQ_RECORD_V3
	if (handle->if_stack_node != NULL) {
		CMDQ_ERR("Incorrect if-else statement, please review your if-else instructions.");
		return -EFAULT;
	}

	if (handle->while_stack_node != NULL) {
		CMDQ_ERR("Incorrect while statement, please review your while instructions.");
		return -EFAULT;
	}
#endif

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
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	uint32_t i;

	pDesc->profileMarker.count = handle->profileMarker.count;
	pDesc->profileMarker.hSlot = handle->profileMarker.hSlot;

	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		pDesc->profileMarker.tag[i] = handle->profileMarker.tag[i];
#endif
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
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc);
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
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc);
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
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, NULL, 0, &pTask);

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

int32_t cmdq_task_flush_async_callback(struct cmdqRecStruct *handle, CmdqAsyncFlushCB callback,
				  uint32_t userData)
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
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, NULL, 0, &pTask);

	/* insert the callback here. */
	/* note that, the task may be already completed at this point. */
	if (pTask) {
		pTask->flushCallback = callback;
		pTask->flushData = userData;
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

int32_t cmdq_task_start_loop(struct cmdqRecStruct *handle)
{
	return cmdq_task_start_loop_callback(handle, &cmdq_dummy_irq_callback, 0);
}

int32_t cmdq_task_start_loop_callback(struct cmdqRecStruct *handle, CmdqInterruptCB loopCB, unsigned long loopData)
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
	desc.pVABase = (cmdqU32Ptr_t) (unsigned long)handle->pBuffer;
	desc.blockSize = handle->blockSize;
	/* secure path */
	cmdq_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* replace instuction position */
	cmdq_setup_replace_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, loopCB, loopData, &handle->pRunningTask);
	return status;
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
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
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
						      &allocatedStartPA);
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

		cmdq_op_read_reg_to_mem(handle, hSlot, index, CMDQ_APXGPT2_COUNT);

		handle->profileMarker.tag[index] = (cmdqU32Ptr_t) (unsigned long)tag;
		handle->profileMarker.count += 1;
	} while (0);

	return status;
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't enable profile marker\n", __func__);
	return -EFAULT;
#endif
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
	if (handle == NULL)
		return -EFAULT;

	if (handle->pRunningTask != NULL)
		return cmdq_task_stop_loop(handle);

	cmdq_reset_v3_struct(handle);

	/* reset local variable setting */
	handle->replace_instr.number = 0;
	if (handle->replace_instr.position != (cmdqU32Ptr_t) (unsigned long)NULL) {
		kfree(CMDQ_U32_PTR(handle->replace_instr.position));
		handle->replace_instr.position = (cmdqU32Ptr_t) (unsigned long)NULL;
	}

	/* Free command buffer */
	kfree(handle->pBuffer);
	handle->pBuffer = NULL;

	/* Free command handle */
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
	bool acquireResult;

	acquireResult = cmdqCoreAcquireResource(resourceEvent);
	if (!acquireResult) {
		CMDQ_LOG("Acquire resource (event:%d) failed, handle:0x%p\n", resourceEvent, handle);
		return -EFAULT;
	}
	return 0;
}

int32_t cmdq_resource_acquire_and_write(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	bool acquireResult;

	acquireResult = cmdqCoreAcquireResource(resourceEvent);
	if (!acquireResult) {
		CMDQ_LOG("Acquire resource (event:%d) failed, handle:0x%p\n", resourceEvent, handle);
		return -EFAULT;
	}

	return cmdq_op_write_reg(handle, addr, value, mask);
}

int32_t cmdq_resource_release(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent)
{
	cmdqCoreReleaseResource(resourceEvent);
	return cmdq_op_set_event(handle, resourceEvent);
}

int32_t cmdq_resource_release_and_write(struct cmdqRecStruct *handle, enum CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t result;

	cmdqCoreReleaseResource(resourceEvent);
	result = cmdq_op_write_reg(handle, addr, value, mask);
	if (result >= 0)
		return cmdq_op_set_event(handle, resourceEvent);

	CMDQ_ERR("Write instruction fail and not release resource!\n");
	return result;
}

#ifdef CMDQ_RECORD_V3
int32_t cmdq_task_create_delay_thread(void **pp_delay_thread_buffer, int32_t *buffer_size)
{
	struct cmdqRecStruct *handle;
	void *p_new_buffer = NULL;
	uint32_t i;
	CMDQ_VARIABLE arg_cpr_start = (CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_CPR_DELAY_STRAT_ID;
	const CMDQ_VARIABLE arg_tpr = (CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_TPR_ID;

	cmdq_task_create(CMDQ_SCENARIO_TIMER_LOOP, &handle);
	cmdq_task_reset(handle);

	cmdq_op_wait_no_clear(handle, CMDQ_SYNC_TOKEN_TIMER);
	for (i = 0; i < CMDQ_MAX_THREAD_COUNT; i++) {
		if (i == CMDQ_DELAY_THREAD_ID)
			continue;
		cmdq_op_if(handle, arg_tpr, CMDQ_GREATER_THAN_AND_EQUAL, arg_cpr_start+i);
		cmdq_op_set_event(handle, CMDQ_SYNC_TOKEN_DELAY_THR0+i);
		cmdq_op_end_if(handle);
	}

	cmdq_op_finalize_command(handle, true);

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
	*buffer_size = handle->blockSize;

	cmdq_task_destroy(handle);
	return 0;
}

static int32_t cmdq_append_logic_command(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_a,
			    CMDQ_VARIABLE arg_b, CMDQ_LOGIC_ENUM s_op, CMDQ_VARIABLE arg_c)
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
		if (true == cmdq_is_cpr(arg_a_i, arg_a_type)
			|| true == cmdq_is_cpr(arg_b_i, arg_b_type)
			|| true == cmdq_is_cpr(arg_c_i, arg_c_type)) {
			/* save local variable position */
			cmdq_save_op_variable_position(handle);
		}

		*p_command++ = (arg_b_i << 16) | (arg_c_i);
		*p_command++ = (CMDQ_CODE_LOGIC << 24) | (arg_abc_type << 21) |
			(s_op << 16) | (*arg_a & 0xFFFFFFFF);

		handle->blockSize += CMDQ_INST_SIZE;
	} while (0);

	return status;
}

void cmdq_init_op_variable(CMDQ_VARIABLE *arg)
{
	*arg = CMDQ_TASK_CPR_INITIAL_VALUE;
}

int32_t cmdq_op_assign(struct cmdqRecStruct *handle, CMDQ_VARIABLE *arg_out, CMDQ_VARIABLE arg_in)
{
	CMDQ_VARIABLE arg_b, arg_c;

	if (CMDQ_BIT_VALUE == (arg_in >> CMDQ_DATA_BIT)) {
		arg_c = (arg_in & 0xFFFFFFFF);
		arg_b = ((arg_in>>16) & 0xFFFFFFFF);
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
	return cmdq_append_logic_command(handle, arg_out, arg_b, CMDQ_LOGIC_SUDSTRACT, arg_c);
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

int32_t cmdq_op_delay_us(struct cmdqRecStruct *handle, CMDQ_VARIABLE delay_time)
{
	int32_t status = 0;
	const CMDQ_VARIABLE arg_tpr = (CMDQ_BIT_VAR<<CMDQ_DATA_BIT) | CMDQ_TPR_ID;
	CMDQ_VARIABLE dummy_delay = CMDQ_TASK_DUMMY_DELAY_CPR;

	do {
		/* Insert TPR addition instruction */
		status = cmdq_op_add(handle, &dummy_delay, arg_tpr, delay_time);
		if (status < 0)
			break;
		/* Insert dummy wait event */
		status = cmdq_op_wait(handle, CMDQ_SYNC_TOKEN_DELAY_THR0);
	} while (0);

	return status;
}

CMDQ_CONDITION_ENUM cmdq_reverse_op_condition(CMDQ_CONDITION_ENUM arg_condition)
{
	CMDQ_CONDITION_ENUM instr_condition = CMDQ_CONDITION_ERROR;

	switch (arg_condition) {
	case CMDQ_EQUAL:
		instr_condition = CMDQ_CONDITION_NOT_EQUAL;
		break;
	case CMDQ_CONDITION_NOT_EQUAL:
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
		CMDQ_ERR("Incorrect CMDQ condition statement, can not append new command");
		break;
	}

	return instr_condition;
}

int32_t cmdq_append_jump_c_command(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
{
	int32_t status = 0;
	const uint32_t dummy_address = 8;
	CMDQ_CONDITION_ENUM instr_condition;
	uint32_t arg_b_i, arg_c_i;
	uint32_t arg_b_type, arg_c_type, arg_abc_type;
	uint32_t *p_command;

	status = cmdq_check_before_append(handle);
	if (status < 0) {
		CMDQ_ERR("	  cannot add jump_c command (condition: %d, arg_b: 0x%08x, arg_c: 0x%08x)\n",
			arg_condition, (uint32_t)(arg_b & 0xFFFFFFFF), (uint32_t)(arg_c & 0xFFFFFFFF));
		return status;
	}

	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	do {
		/* reverse condition statement */
		instr_condition = cmdq_reverse_op_condition(arg_condition);
		if (instr_condition < 0) {
			status = -EFAULT;
			break;
		}

		/* get actual arg_b_i & arg_b_type */
		status = cmdq_var_data_type(arg_b, &arg_b_i, &arg_b_type);
		if (status < 0)
			break;

		/* get actual arg_c_i & arg_c_type */
		status = cmdq_var_data_type(arg_c, &arg_c_i, &arg_c_type);
		if (status < 0)
			break;

		/* arg_a always be value */
		arg_abc_type = (arg_b_type << 1) | (arg_c_type);
		if (true == cmdq_is_cpr(arg_b_i, arg_b_type)
			|| true == cmdq_is_cpr(arg_c_i, arg_c_type)) {
			/* save local variable position */
			cmdq_save_op_variable_position(handle);
		}

		*p_command++ = (arg_b_i << 16) | (arg_c_i);
		*p_command++ = (CMDQ_CODE_JUMP_C_RELATIVE << 24) | (arg_abc_type << 21) |
			(instr_condition << 16) | (dummy_address);

		handle->blockSize += CMDQ_INST_SIZE;
	} while (0);

	return status;
}

int32_t cmdq_op_rewrite_jump_c(struct cmdqRecStruct *handle, uint32_t rewritten_position, uint32_t new_value)
{
	int32_t status = 0;
	uint32_t op, op_arg_type;
	uint32_t *p_command;

	if (handle == NULL)
		return -EFAULT;

	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + rewritten_position);

	do {
		/* reserve condition statement */
		op = (p_command[1] & 0xFF000000) >> 24;
		if (op != CMDQ_CODE_JUMP_C_RELATIVE)
			break;

		op_arg_type = (p_command[1] & 0xFFFF0000) >> 16;

		/* rewrite actual jump value */
		p_command[1] = (op_arg_type << 16) | (new_value - rewritten_position);
	} while (0);

	return status;
}

int32_t cmdq_op_if(struct cmdqRecStruct *handle, CMDQ_VARIABLE arg_b,
			   CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
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
	CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

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
	CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

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
		status = cmdq_append_jump_c_command(handle, 1, CMDQ_CONDITION_NOT_EQUAL, 1);
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
			   CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
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
			   CMDQ_CONDITION_ENUM arg_condition, CMDQ_VARIABLE arg_c)
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

int32_t cmdq_op_continue(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	uint32_t current_position;
	int32_t rewritten_position;
	uint32_t *p_command;

	if (handle == NULL)
		return -EFAULT;

	p_command = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	do {
		current_position = handle->blockSize;

		/* query while position from the stack */
		rewritten_position = cmdq_op_condition_find_while(handle->while_stack_node, current_position);
		if (rewritten_position >= 0) {
			status = -EFAULT;
			break;
		}

		/* use jump command to start of while statement, since jump_c cannot process negative number */
		cmdq_append_command(handle, CMDQ_CODE_JUMP, 0, rewritten_position, 0, 0);
	} while (0);

	return status;
}

int32_t cmdq_op_break(struct cmdqRecStruct *handle)
{
	int32_t status = 0;
	uint32_t current_position;
	int32_t while_position;

	if (handle == NULL)
		return -EFAULT;

	do {
		current_position = handle->blockSize;

		/* query while position from the stack */
		while_position = cmdq_op_condition_find_while(handle->while_stack_node, current_position);
		if (while_position >= 0) {
			CMDQ_ERR("Incorrect break command, please review your while statement.");
			status = -EFAULT;
			break;
		}

		/* append conditional jump instruction */
		status = cmdq_append_jump_c_command(handle, 1, CMDQ_CONDITION_NOT_EQUAL, 1);
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
	CMDQ_STACK_TYPE_ENUM rewritten_stack_type;

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

		if (rewritten_stack_type == CMDQ_STACK_TYPE_WHILE) {
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

int32_t cmdq_func_create(struct cmdqRecStruct **handle, uint32_t in_num, uint32_t out_num)
{
	int32_t status = 0;
	struct cmdqRecStruct *sub_handle = NULL;

	do {
		status = cmdq_task_create(CMDQ_SCENARIO_DEBUG, &sub_handle);
		if (status < 0)
			break;

		cmdq_task_reset(sub_handle);
		if (status < 0)
			break;

		sub_handle->sub_function.is_subfunction = true;
		sub_handle->sub_function.reference_cnt = -1;
		sub_handle->sub_function.in_num = in_num;
		sub_handle->sub_function.out_num = out_num;
		sub_handle->sub_function.in_arg = NULL;
		sub_handle->sub_function.out_arg = NULL;

		if (in_num > 0) {
			sub_handle->sub_function.in_arg = kmalloc_array(in_num, sizeof(CMDQ_VARIABLE), GFP_ATOMIC);
			if (sub_handle->sub_function.in_arg == NULL) {
				cmdq_task_destroy(sub_handle);
				status = -ENOMEM;
				break;
			}
		}

		if (out_num > 0) {
			sub_handle->sub_function.out_arg = kmalloc_array(out_num, sizeof(CMDQ_VARIABLE), GFP_ATOMIC);
			if (sub_handle->sub_function.out_arg == NULL) {
				cmdq_task_destroy(sub_handle);
				status = -ENOMEM;
				break;
			}
		}

		/* TODO - Insert Wait Event */

		*handle = sub_handle;
	} while (0);

	return status;
}

int32_t cmdq_func_end(struct cmdqRecStruct *handle)
{
	int32_t status = 0;

	do {
		if (handle == NULL) {
			status = -EFAULT;
			break;
		}

		if (false == handle->sub_function.is_subfunction) {
			status = -EFAULT;
			break;
		}

		if (handle->sub_function.reference_cnt > 0) {
			status = -EFAULT;
			break;
		}

		handle->sub_function.reference_cnt = 0;
	} while (0);

	return status;
}

int32_t cmdq_func_call(struct cmdqRecStruct *handle, struct cmdqRecStruct *sub_func_handle, ...)
{
	int32_t status = 0;
	int i;
	va_list argptr;

	do {
		if (handle == NULL) {
			status = -EFAULT;
			break;
		}

		if (sub_func_handle == NULL) {
			status = -EFAULT;
			break;
		}

		if (false == sub_func_handle->sub_function.is_subfunction) {
			status = -EFAULT;
			break;
		}

		if (sub_func_handle->sub_function.reference_cnt < 0) {
			status = -EFAULT;
			break;
		}

		va_start(argptr, sub_func_handle);
		for (i = 0; i < sub_func_handle->sub_function.in_num; i++)
			sub_func_handle->sub_function.in_arg[i] = va_arg(argptr, CMDQ_VARIABLE);
		va_end(argptr);

		/* TODO - Insert Jump to sub function */

		sub_func_handle->sub_function.reference_cnt++;
	} while (0);

	return status;
}

int32_t cmdq_func_destroy(struct cmdqRecStruct *handle)
{
	return cmdq_task_destroy(handle);
}

int32_t cmdq_op_read_reg(struct cmdqRecStruct *handle, uint32_t addr,
				   CMDQ_VARIABLE *arg_out, uint32_t mask)
{
	int32_t status = 0;
	CMDQ_CODE_ENUM op_code;
	CMDQ_VARIABLE arg_a;
	uint32_t arg_a_i, arg_a_type;

	if (mask != 0xFFFFFFFF) {
		status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask, 0, 0);
		if (status != 0)
			return status;
	}

	if (mask != 0xFFFFFFFF)
		op_code = CMDQ_CODE_READ_S_W_MASK;
	else
		op_code = CMDQ_CODE_READ_S;

	cmdq_init_op_variable(&arg_a);
	/* get arg_a register by using module storage manager */
	status = cmdq_create_variable_if_need(handle, &arg_a);
	if (status < 0)
		return status;

	/* get actual arg_a_i & arg_a_type */
	status = cmdq_var_data_type(arg_a, &arg_a_i, &arg_a_type);
	if (status < 0)
		return status;

	status = cmdq_append_command(handle, op_code, arg_a_i, addr, arg_a_type, 0);
	if (status < 0)
		return status;

	*arg_out = arg_a;

	return status;
}

int32_t cmdq_op_read_mem(struct cmdqRecStruct *handle, cmdqBackupSlotHandle h_backup_slot,
			    uint32_t slot_index, CMDQ_VARIABLE *arg_out)
{
	return 0;
}
#endif				/* CMDQ_RECORD_V3 */

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
			   uint32_t baseHandle, uint32_t offset, uint32_t size, uint32_t port)
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

int32_t cmdqRecFlushAsyncCallback(struct cmdqRecStruct *handle, CmdqAsyncFlushCB callback,
				  uint32_t userData)
{
	return cmdq_task_flush_async_callback(handle, callback, userData);
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
