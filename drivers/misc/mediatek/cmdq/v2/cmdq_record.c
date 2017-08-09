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
#include <mt-plat/mt_lpae.h>

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

int32_t cmdq_rec_realloc_addr_metadata_buffer(cmdqRecHandle handle, const uint32_t size)
{
	void *pNewBuf = NULL;
	void *pOriginalBuf = (void *)CMDQ_U32_PTR(handle->secData.addrMetadatas);
	const uint32_t originalSize =
	    sizeof(cmdqSecAddrMetadataStruct) * (handle->secData.addrMetadataMaxCount);

	if (size <= originalSize)
		return 0;

	pNewBuf = kzalloc(size, GFP_KERNEL);
	if (NULL == pNewBuf) {
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
	handle->secData.addrMetadataMaxCount = size / sizeof(cmdqSecAddrMetadataStruct);

	return 0;
}

int cmdq_rec_realloc_cmd_buffer(cmdqRecHandle handle, uint32_t size)
{
	void *pNewBuf = NULL;

	if (size <= handle->bufferSize)
		return 0;

	pNewBuf = kzalloc(size, GFP_KERNEL);

	if (NULL == pNewBuf) {
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

static int32_t cmdq_rec_reset_profile_maker_data(cmdqRecHandle handle)
{
#ifdef CMDQ_PROFILE_MARKER_SUPPORT
	int32_t i = 0;

	if (NULL == handle)
		return -EFAULT;

	handle->profileMarker.count = 0;
	handle->profileMarker.hSlot = 0LL;

	for (i = 0; i < CMDQ_MAX_PROFILE_MARKER_IN_TASK; i++)
		handle->profileMarker.tag[i] = (cmdqU32Ptr_t) (unsigned long)(NULL);

	return 0;
#endif
	return 0;
}

int32_t cmdqRecCreate(CMDQ_SCENARIO_ENUM scenario, cmdqRecHandle *pHandle)
{
	cmdqRecHandle handle = NULL;

	if (scenario < 0 || scenario >= CMDQ_MAX_SCENARIO_COUNT) {
		CMDQ_ERR("Unknown scenario type %d\n", scenario);
		return -EINVAL;
	}

	handle = kzalloc(sizeof(uint8_t *) * sizeof(cmdqRecStruct), GFP_KERNEL);
	if (NULL == handle)
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
	handle->secData.isSecure = false;
	handle->secData.enginesNeedDAPC = 0LL;
	handle->secData.enginesNeedPortSecurity = 0LL;
	handle->secData.addrMetadatas = (cmdqU32Ptr_t) (unsigned long)NULL;
	handle->secData.addrMetadataMaxCount = 0;
	handle->secData.addrMetadataCount = 0;

	/* profile marker */
	cmdq_rec_reset_profile_maker_data(handle);

	/* CMD */
	if (0 != cmdq_rec_realloc_cmd_buffer(handle, CMDQ_INITIAL_CMD_BLOCK_SIZE)) {
		kfree(handle);
		return -ENOMEM;
	}

	*pHandle = handle;

	return 0;
}

#ifdef CMDQ_SECURE_PATH_SUPPORT
int32_t cmdq_append_addr_metadata(cmdqRecHandle handle, const cmdqSecAddrMetadataStruct *pMetadata)
{
	cmdqSecAddrMetadataStruct *pAddrs;
	int32_t status;
	uint32_t size;
	/* element index of the New appended addr metadat */
	const uint32_t index = handle->secData.addrMetadataCount;

	pAddrs = NULL;
	status = 0;

	if (0 >= handle->secData.addrMetadataMaxCount) {
		/* not init yet, initialize to allow max 8 addr metadata */
		size = sizeof(cmdqSecAddrMetadataStruct) * 8;
		status = cmdq_rec_realloc_addr_metadata_buffer(handle, size);
	} else if (handle->secData.addrMetadataCount >= (handle->secData.addrMetadataMaxCount)) {
		/* enlarge metadata buffer to twice as */
		size =
		    sizeof(cmdqSecAddrMetadataStruct) * (handle->secData.addrMetadataMaxCount) * 2;
		status = cmdq_rec_realloc_addr_metadata_buffer(handle, size);
	}

	if (0 > status)
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

/**
 * centralize the write/polling/read command for APB and GPR handle
 * this function must be called inside cmdq_append_command
 * because we ignore buffer and pre-fetch check here.
 * Parameter:
 *     same as cmdq_append_command
 * Return:
 *     same as cmdq_append_command
 */
static int32_t cmdq_append_wpr_command(cmdqRecHandle handle, CMDQ_CODE_ENUM code,
				       uint32_t argA, uint32_t argB, uint32_t argAType,
				       uint32_t argBType)
{
	int32_t status = 0;
	int32_t subsys;
	uint32_t *pCommand;
	bool bUseGPR = false;
	/* use new argA to present final inserted argA */
	uint32_t newArgA;
	uint32_t newArgAType = argAType;
	uint32_t argType = 0;

	/* be careful that subsys encoding position is different among platforms */
	const uint32_t subsysBit = cmdq_get_func()->getSubsysLSBArgA();

	pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	if (CMDQ_CODE_READ != code && CMDQ_CODE_WRITE != code && CMDQ_CODE_POLL != code) {
		CMDQ_ERR("Record 0x%p, flow error, should not append comment in wpr API", handle);
		return -EFAULT;
	}

	/* we must re-calculate current PC at first. */
	pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	CMDQ_VERBOSE("REC: 0x%p CMD: 0x%p, op: 0x%02x\n", handle, pCommand, code);
	CMDQ_VERBOSE("REC: 0x%p CMD: argA: 0x%08x, argB: 0x%08x, argAType: %d, argBType: %d\n",
		     handle, argA, argB, argAType, argBType);

	if (0 == argAType) {
		/* argA is the HW register address to read from */
		subsys = cmdq_core_subsys_from_phys_addr(argA);
		if (CMDQ_SPECIAL_SUBSYS_ADDR == subsys) {
#ifdef CMDQ_GPR_SUPPORT
			bUseGPR = true;
			CMDQ_MSG("REC: Special handle memory base address 0x%08x\n", argA);
			/* Wait and clear for GPR mutex token to enter mutex */
			*pCommand++ = ((1 << 31) | (1 << 15) | 1);
			*pCommand++ = (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_TOKEN_GPR_SET_4;
			handle->blockSize += CMDQ_INST_SIZE;
			/* Move extra handle APB address to GPR */
			*pCommand++ = argA;
			*pCommand++ = (CMDQ_CODE_MOVE << 24) |
			    ((CMDQ_DATA_REG_DEBUG & 0x1f) << 16) | (4 << 21);
			handle->blockSize += CMDQ_INST_SIZE;
			/* change final argA to GPR */
			newArgA = ((CMDQ_DATA_REG_DEBUG & 0x1f) << 16);
			if (argA & 0x1) {
				/* MASK case, set final bit to 1 */
				newArgA = newArgA | 0x1;
			}
			/* change argA type to 1 */
			newArgAType = 1;
#else
			CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
			status = -EFAULT;
#endif
		} else if (0 == argAType && 0 > subsys) {
			CMDQ_ERR("REC: Unsupported memory base address 0x%08x\n", argA);
			status = -EFAULT;
		} else {
			/* compose final argA according to subsys table */
			newArgA = (argA & 0xffff) | ((subsys & 0x1f) << subsysBit);
		}
	} else {
		/* compose final argA according GPR value */
		newArgA = ((argA & 0x1f) << 16);
	}

	if (status < 0)
		return status;

	argType = (newArgAType << 2) | (argBType << 1);

	/* newArgA is the HW register address to access from or GPR value store the HW register address */
	/* argB is the value or register id  */
	/* bit 55: argA type, 1 for GPR */
	/* bit 54: argB type, 1 for GPR */
	/* argType: ('newArgAType', 'argBType', '0') */
	*pCommand++ = argB;
	*pCommand++ = (code << 24) | newArgA | (argType << 21);
	handle->blockSize += CMDQ_INST_SIZE;

	if (bUseGPR) {
		/* Set for GPR mutex token to leave mutex */
		*pCommand++ = ((1 << 31) | (1 << 16));
		*pCommand++ = (CMDQ_CODE_WFE << 24) | CMDQ_SYNC_TOKEN_GPR_SET_4;
		handle->blockSize += CMDQ_INST_SIZE;
	}
	return 0;
}

int32_t cmdq_append_command(cmdqRecHandle handle, CMDQ_CODE_ENUM code,
			    uint32_t argA, uint32_t argB, uint32_t argAType, uint32_t argBType)
{
	uint32_t *pCommand;

	if (NULL == handle)
		return -EFAULT;

	pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	if (handle->finalized) {
		CMDQ_ERR("Finalized record 0x%p (scenario:%d)\n", handle, handle->scenario);
		CMDQ_ERR("    cannot add command (op: 0x%02x, argA: 0x%08x, argB: 0x%08x)\n",
			code, argA, argB);
		return -EBUSY;
	}

	/* check if we have sufficient buffer size */
	/* we leave a 4 instruction (4 bytes each) margin. */
	if ((handle->blockSize + 32) >= handle->bufferSize) {
		if (0 != cmdq_rec_realloc_cmd_buffer(handle, handle->bufferSize * 2))
			return -ENOMEM;
	}

	/* force insert MARKER if prefetch memory is full */
	/* GCE deadlocks if we don't do so */
	if (CMDQ_CODE_EOC != code && cmdq_get_func()->shouldEnablePrefetch(handle->scenario)) {
		uint32_t prefetchSize = 0;
		int32_t threadNo = cmdq_get_func()->getThreadID(handle->scenario, handle->secData.isSecure);

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
			if (1 <= handle->prefetchCount) {
				++handle->prefetchCount;
				CMDQ_VERBOSE("handle->prefetchCount: %d, %s, %d\n",
					     handle->prefetchCount, __func__, __LINE__);
			}
		}
	}

	/* we must re-calculate current PC because we may already insert MARKER inst. */
	pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + handle->blockSize);

	CMDQ_VERBOSE("REC: 0x%p CMD: 0x%p, op: 0x%02x, argA: 0x%08x, argB: 0x%08x\n", handle,
		     pCommand, code, argA, argB);

	if (CMDQ_CODE_READ == code || CMDQ_CODE_WRITE == code || CMDQ_CODE_POLL == code) {
		/* Because read/write/poll have similar format, handle them together */
		return cmdq_append_wpr_command(handle, code, argA, argB, argAType, argBType);
	}

	switch (code) {
	case CMDQ_CODE_MOVE:
		*pCommand++ = argB;
		*pCommand++ = CMDQ_CODE_MOVE << 24 | (argA & 0xffffff);
		break;
	case CMDQ_CODE_JUMP:
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_JUMP << 24) | (argA & 0x0FFFFFF);
		break;
	case CMDQ_CODE_WFE:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		*pCommand++ = ((1 << 31) | (1 << 15) | 1);
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_SET_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 1 */
		*pCommand++ = ((1 << 31) | (1 << 16));
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_WAIT_NO_CLEAR:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, false */
		*pCommand++ = ((0 << 31) | (1 << 15) | 1);
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_CLEAR_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		*pCommand++ = ((1 << 31) | (0 << 16));
		*pCommand++ = (CMDQ_CODE_WFE << 24) | argA;
		break;

	case CMDQ_CODE_EOC:
		*pCommand++ = argB;
		*pCommand++ = (CMDQ_CODE_EOC << 24) | (argA & 0x0FFFFFF);
		break;

	case CMDQ_CODE_RAW:
		*pCommand++ = argB;
		*pCommand++ = argA;
		break;

	default:
		return -EFAULT;
	}

	handle->blockSize += CMDQ_INST_SIZE;
	return 0;
}

int32_t cmdqRecSetEngine(cmdqRecHandle handle, uint64_t engineFlag)
{
	if (NULL == handle)
		return -EFAULT;

	CMDQ_VERBOSE("REC: %p, engineFlag: 0x%llx\n", handle, engineFlag);
	handle->engineFlag = engineFlag;

	return 0;
}

int32_t cmdqRecReset(cmdqRecHandle handle)
{
	if (NULL == handle)
		return -EFAULT;

	if (NULL != handle->pRunningTask)
		cmdqRecStopLoop(handle);

	handle->blockSize = 0;
	handle->prefetchCount = 0;
	handle->finalized = false;

	/* reset secure path data */
	handle->secData.isSecure = false;
	handle->secData.enginesNeedDAPC = 0LL;
	handle->secData.enginesNeedPortSecurity = 0LL;
	if (handle->secData.addrMetadatas) {
		kfree(CMDQ_U32_PTR(handle->secData.addrMetadatas));
		handle->secData.addrMetadatas = (cmdqU32Ptr_t) (unsigned long)NULL;
		handle->secData.addrMetadataMaxCount = 0;
		handle->secData.addrMetadataCount = 0;
	}

	/* profile marker */
	cmdq_rec_reset_profile_maker_data(handle);

	return 0;
}

int32_t cmdqRecSetSecure(cmdqRecHandle handle, const bool isSecure)
{
	if (NULL == handle)
		return -EFAULT;

	if (false == isSecure) {
		handle->secData.isSecure = isSecure;
		return 0;
	}
#ifdef CMDQ_SECURE_PATH_SUPPORT
	CMDQ_VERBOSE("REC: %p secure:%d\n", handle, isSecure);
	handle->secData.isSecure = isSecure;
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdqRecIsSecure(cmdqRecHandle handle)
{
	if (NULL == handle)
		return -EFAULT;

	return handle->secData.isSecure;
}

int32_t cmdqRecSecureEnableDAPC(cmdqRecHandle handle, const uint64_t engineFlag)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (NULL == handle)
		return -EFAULT;

	handle->secData.enginesNeedDAPC |= engineFlag;
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdqRecSecureEnablePortSecurity(cmdqRecHandle handle, const uint64_t engineFlag)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	if (NULL == handle)
		return -EFAULT;

	handle->secData.enginesNeedPortSecurity |= engineFlag;
	return 0;
#else
	CMDQ_ERR("%s failed since not support secure path\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdqRecMark(cmdqRecHandle handle)
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

	if (0 != status)
		return -EFAULT;

	return 0;
}

int32_t cmdqRecWrite(cmdqRecHandle handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t status;

	if (0xFFFFFFFF != mask) {
		status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask, 0, 0);
		if (0 != status)
			return status;

		addr = addr | 0x1;
	}

	status = cmdq_append_command(handle, CMDQ_CODE_WRITE, addr, value, 0, 0);
	if (0 != status)
		return status;

	return 0;
}

int32_t cmdqRecWriteSecure(cmdqRecHandle handle, uint32_t addr,
			   CMDQ_SEC_ADDR_METADATA_TYPE type,
			   uint32_t baseHandle, uint32_t offset, uint32_t size, uint32_t port)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status;
	int32_t writeInstrIndex;
	cmdqSecAddrMetadataStruct metadata;
	const uint32_t mask = 0xFFFFFFFF;

	/* append command */
	status = cmdqRecWrite(handle, addr, baseHandle, mask);
	if (0 != status)
		return status;

	/* append to metadata list */
	writeInstrIndex = (handle->blockSize) / CMDQ_INST_SIZE - 1;	/* start from 0 */

	memset(&metadata, 0, sizeof(cmdqSecAddrMetadataStruct));
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

int32_t cmdqRecPoll(cmdqRecHandle handle, uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t status;

	status = cmdq_append_command(handle, CMDQ_CODE_MOVE, 0, ~mask, 0, 0);
	if (0 != status)
		return status;

	status = cmdq_append_command(handle, CMDQ_CODE_POLL, (addr | 0x1), value, 0, 0);
	if (0 != status)
		return status;

	return 0;
}


int32_t cmdqRecWait(cmdqRecHandle handle, CMDQ_EVENT_ENUM event)
{
	uint32_t argA;

	if (CMDQ_SYNC_TOKEN_INVALID == event || CMDQ_SYNC_TOKEN_MAX <= event || 0 > event)
		return -EINVAL;

	argA = cmdq_core_get_event_value(event);
	return cmdq_append_command(handle, CMDQ_CODE_WFE, argA, 0, 0, 0);
}


int32_t cmdqRecWaitNoClear(cmdqRecHandle handle, CMDQ_EVENT_ENUM event)
{
	uint32_t argA;

	if (CMDQ_SYNC_TOKEN_INVALID == event || CMDQ_SYNC_TOKEN_MAX <= event || 0 > event)
		return -EINVAL;

	argA = cmdq_core_get_event_value(event);
	return cmdq_append_command(handle, CMDQ_CODE_WAIT_NO_CLEAR, argA, 0, 0, 0);
}

int32_t cmdqRecClearEventToken(cmdqRecHandle handle, CMDQ_EVENT_ENUM event)
{
	uint32_t argA;

	if (CMDQ_SYNC_TOKEN_INVALID == event || CMDQ_SYNC_TOKEN_MAX <= event || 0 > event)
		return -EINVAL;

	argA = cmdq_core_get_event_value(event);
	return cmdq_append_command(handle, CMDQ_CODE_CLEAR_TOKEN, argA, 1,	/* actually this param is ignored. */
				   0, 0);
}


int32_t cmdqRecSetEventToken(cmdqRecHandle handle, CMDQ_EVENT_ENUM event)
{
	uint32_t argA;

	if (CMDQ_SYNC_TOKEN_INVALID == event || CMDQ_SYNC_TOKEN_MAX <= event || 0 > event)
		return -EINVAL;

	argA = cmdq_core_get_event_value(event);
	return cmdq_append_command(handle, CMDQ_CODE_SET_TOKEN, argA, 1,	/* actually this param is ignored. */
				   0, 0);
}

int32_t cmdqRecReadToDataRegister(cmdqRecHandle handle, uint32_t hwRegAddr,
				  CMDQ_DATA_REGISTER_ENUM dstDataReg)
{
#ifdef CMDQ_GPR_SUPPORT
	/* read from hwRegAddr(argA) to dstDataReg(argB) */
	return cmdq_append_command(handle, CMDQ_CODE_READ, hwRegAddr, dstDataReg, 0, 1);

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdqRecWriteFromDataRegister(cmdqRecHandle handle,
				     CMDQ_DATA_REGISTER_ENUM srcDataReg, uint32_t hwRegAddr)
{
#ifdef CMDQ_GPR_SUPPORT
	/* write HW register(argA) with data of GPR data register(argB) */
	return cmdq_append_command(handle, CMDQ_CODE_WRITE, hwRegAddr, srcDataReg, 0, 1);
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Allocate 32-bit register backup slot
 *
 */
int32_t cmdqBackupAllocateSlot(cmdqBackupSlotHandle *phBackupSlot, uint32_t slotCount)
{
#ifdef CMDQ_GPR_SUPPORT

	dma_addr_t paStart = 0;
	int status = 0;

	if (NULL == phBackupSlot)
		return -EINVAL;

	status = cmdqCoreAllocWriteAddress(slotCount, &paStart);
	*phBackupSlot = paStart;

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
int32_t cmdqBackupReadSlot(cmdqBackupSlotHandle hBackupSlot, uint32_t slotIndex, uint32_t *value)
{
#ifdef CMDQ_GPR_SUPPORT

	if (NULL == value)
		return -EINVAL;

	if (0 == hBackupSlot) {
		CMDQ_ERR("%s, hBackupSlot is NULL\n", __func__);
		return -EINVAL;
	}

	*value = cmdqCoreReadWriteAddress(hBackupSlot + slotIndex * sizeof(uint32_t));

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdqBackupWriteSlot(cmdqBackupSlotHandle hBackupSlot, uint32_t slotIndex, uint32_t value)
{
#ifdef CMDQ_GPR_SUPPORT

	int status = 0;
	/* set the slot value directly */
	status = cmdqCoreWriteWriteAddress(hBackupSlot + slotIndex * sizeof(uint32_t), value);
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
int32_t cmdqBackupFreeSlot(cmdqBackupSlotHandle hBackupSlot)
{
#ifdef CMDQ_GPR_SUPPORT
	return cmdqCoreFreeWriteAddress(hBackupSlot);
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

/**
 *  Insert instructions to backup given 32-bit HW register
 *  to a backup slot.
 *  You can use cmdqBackupReadSlot() to retrieve the result
 *  AFTER cmdqRecFlush() returns, or INSIDE the callback of cmdqRecFlushAsyncCallback().
 *
 */
int32_t cmdqRecBackupRegisterToSlot(cmdqRecHandle handle,
				    cmdqBackupSlotHandle hBackupSlot,
				    uint32_t slotIndex, uint32_t regAddr)
{
#ifdef CMDQ_GPR_SUPPORT
	const CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = hBackupSlot + slotIndex * sizeof(uint32_t);
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdqRecWait(handle, regAccessToken);

	/* Load into 32-bit GPR (R0-R15) */
	cmdq_append_command(handle, CMDQ_CODE_READ, regAddr, valueRegId, 0, 1);

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);

	/* write value in GPR to memory pointed by GPR */
	cmdq_append_command(handle, CMDQ_CODE_WRITE, destRegId, valueRegId, 1, 1);
	/* release the GPR lock */
	cmdqRecSetEventToken(handle, regAccessToken);

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdqRecBackupWriteRegisterFromSlot(cmdqRecHandle handle,
					   cmdqBackupSlotHandle hBackupSlot,
					   uint32_t slotIndex, uint32_t addr)
{
#ifdef CMDQ_GPR_SUPPORT
	const CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const CMDQ_DATA_REGISTER_ENUM addrRegId = CMDQ_DATA_REG_DEBUG_DST;
	const CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = hBackupSlot + slotIndex * sizeof(uint32_t);
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdqRecWait(handle, regAccessToken);

	/* 1. MOVE slot address to addr GPR */

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((addrRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);	/* argA is GPR */

	/* 2. read value from src address, which is stroed in GPR, to valueRegId */
	cmdq_append_command(handle, CMDQ_CODE_READ, addrRegId, valueRegId, 1, 1);

	/* 3. write from data register */
	cmdqRecWriteFromDataRegister(handle, valueRegId, addr);

	/* release the GPR lock */
	cmdqRecSetEventToken(handle, regAccessToken);

	return 0;
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */
}

int32_t cmdqRecBackupUpdateSlot(cmdqRecHandle handle,
				cmdqBackupSlotHandle hBackupSlot,
				uint32_t slotIndex, uint32_t value)
{
#ifdef CMDQ_GPR_SUPPORT
	const CMDQ_DATA_REGISTER_ENUM valueRegId = CMDQ_DATA_REG_DEBUG;
	const CMDQ_DATA_REGISTER_ENUM destRegId = CMDQ_DATA_REG_DEBUG_DST;
	const CMDQ_EVENT_ENUM regAccessToken = CMDQ_SYNC_TOKEN_GPR_SET_4;
	const dma_addr_t dramAddr = hBackupSlot + slotIndex * sizeof(uint32_t);
	uint32_t argA;
	uint32_t highAddr = 0;

	/* lock GPR because we may access it in multiple CMDQ HW threads */
	cmdqRecWait(handle, regAccessToken);

	/* Assign 32-bit GRP with value */
	argA = (CMDQ_CODE_MOVE << 24) | (valueRegId << 16) | (4 << 21);	/* argA is GPR */
	cmdq_append_command(handle, CMDQ_CODE_RAW, argA, value, 0, 0);

	/* Note that <MOVE> argB is 48-bit */
	/* so writeAddress is split into 2 parts */
	/* and we store address in 64-bit GPR (P0-P7) */
	CMDQ_GET_HIGH_ADDR(dramAddr, highAddr);
	cmdq_append_command(handle, CMDQ_CODE_MOVE,
			    highAddr |
			    ((destRegId & 0x1f) << 16) | (4 << 21), (uint32_t) dramAddr, 0, 0);

	/* write value in GPR to memory pointed by GPR */
	cmdq_append_command(handle, CMDQ_CODE_WRITE, destRegId, valueRegId, 1, 1);

	/* release the GPR lock */
	cmdqRecSetEventToken(handle, regAccessToken);

	return 0;

#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't support GPR\n", __func__);
	return -EFAULT;
#endif				/* CMDQ_GPR_SUPPORT */

}

int32_t cmdqRecEnablePrefetch(cmdqRecHandle handle)
{
#ifdef _CMDQ_DISABLE_MARKER_
	/* disable pre-fetch marker feature but use auto prefetch mechanism */
	CMDQ_MSG("not allow enable prefetch, scenario: %d\n", handle->scenario);
	return true;
#else
	if (NULL == handle)
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

int32_t cmdqRecDisablePrefetch(cmdqRecHandle handle)
{
	uint32_t argB = 0;
	uint32_t argA = 0;
	int32_t status = 0;

	if (NULL == handle)
		return -EFAULT;

	if (!handle->finalized) {
		if (handle->prefetchCount > 0) {
			/* with prefetch threads we should end with */
			/* bit 48: no_inc_exec_cmds_cnt = 1 */
			/* bit 20: prefetch_mark = 1 */
			/* bit 17: prefetch_mark_en = 0 */
			/* bit 16: prefetch_en = 0 */
			argB = 0x00100000;
			argA = (0x1 << 16);	/* not increse execute counter */
			/* since we're finalized, no more prefetch */
			handle->prefetchCount = 0;
			status = cmdq_append_command(handle, CMDQ_CODE_EOC, argA, argB, 0, 0);
		}

		if (0 != status)
			return status;
	}

	CMDQ_MSG("cmdqRecDisablePrefetch, status:%d\n", status);
	return status;
}

int32_t cmdq_rec_finalize_command(cmdqRecHandle handle, bool loop)
{
	int32_t status = 0;
	uint32_t argB = 0;

	if (NULL == handle)
		return -EFAULT;

	if (!handle->finalized) {
		if ((handle->prefetchCount > 0)
		    && cmdq_get_func()->shouldEnablePrefetch(handle->scenario)) {
			CMDQ_ERR
			    ("not insert prefetch disble marker when prefetch enabled, prefetchCount:%d\n",
			     handle->prefetchCount);
			cmdqRecDumpCommand(handle);

			status = -EFAULT;
			return status;
		}

		/* insert EOF instruction */
		argB = 0x1;	/* generate IRQ for each command iteration */
#ifdef DISABLE_LOOP_IRQ
		if (loop && !cmdq_get_func()->forceLoopIRQ(handle->scenario))
			argB = 0x0;	/* no generate IRQ for loop thread to save power */
#endif
		status = cmdq_append_command(handle, CMDQ_CODE_EOC, 0, argB, 0, 0);

		if (0 != status)
			return status;

		/* insert JUMP to loop to beginning or as a scheduling mark(8) */
		status = cmdq_append_command(handle, CMDQ_CODE_JUMP, 0,	/* not absolute */
					     loop ? -handle->blockSize : 8, 0, 0);
		if (0 != status)
			return status;

		handle->finalized = true;
	}

	return status;
}

int32_t cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(cmdqCommandStruct *pDesc,
							      cmdqRecHandle handle)
{
	/* fill field from user's request */
	pDesc->secData.isSecure = handle->secData.isSecure;
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

int32_t cmdq_rec_setup_profile_marker_data(cmdqCommandStruct *pDesc, cmdqRecHandle handle)
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

int32_t cmdqRecFlush(cmdqRecHandle handle)
{
	int32_t status;
	cmdqCommandStruct desc = { 0 };

	status = cmdq_rec_finalize_command(handle, false);
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
	cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc);
}

int32_t cmdqRecFlushAndReadRegister(cmdqRecHandle handle, uint32_t regCount, uint32_t *addrArray,
				    uint32_t *valueArray)
{
	int32_t status;
	cmdqCommandStruct desc = { 0 };

	status = cmdq_rec_finalize_command(handle, false);
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
	cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	return cmdqCoreSubmitTask(&desc);
}


int32_t cmdqRecFlushAsync(cmdqRecHandle handle)
{
	int32_t status = 0;
	cmdqCommandStruct desc = { 0 };
	TaskStruct *pTask = NULL;

	status = cmdq_rec_finalize_command(handle, false);
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
	cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
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


int32_t cmdqRecFlushAsyncCallback(cmdqRecHandle handle, CmdqAsyncFlushCB callback,
				  uint32_t userData)
{
	int32_t status = 0;
	cmdqCommandStruct desc = { 0 };
	TaskStruct *pTask = NULL;

	status = cmdq_rec_finalize_command(handle, false);
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
	cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
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


static int32_t cmdqRecIRQCallback(unsigned long data)
{
	return 0;
}

int32_t cmdqRecStartLoop(cmdqRecHandle handle)
{
	return cmdqRecStartLoopWithCallback(handle, &cmdqRecIRQCallback, 0);
}

int32_t cmdqRecStartLoopWithCallback(cmdqRecHandle handle, CmdqInterruptCB loopCB, unsigned long loopData)
{
	int32_t status = 0;
	cmdqCommandStruct desc = { 0 };

	if (NULL == handle)
		return -EFAULT;

	if (NULL != handle->pRunningTask)
		return -EBUSY;

	status = cmdq_rec_finalize_command(handle, true);
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
	cmdq_rec_setup_sec_data_of_command_desc_by_rec_handle(&desc, handle);
	/* profile marker */
	cmdq_rec_setup_profile_marker_data(&desc, handle);

	status = cmdqCoreSubmitTaskAsync(&desc, loopCB, loopData, &handle->pRunningTask);
	return status;
}

int32_t cmdqRecStopLoop(cmdqRecHandle handle)
{
	int32_t status = 0;
	struct TaskStruct *pTask;

	if (NULL == handle)
		return -EFAULT;

	pTask = handle->pRunningTask;
	if (NULL == pTask)
		return -EFAULT;

	status = cmdqCoreReleaseTask(pTask);
	handle->pRunningTask = NULL;
	return status;
}

int32_t cmdqRecGetInstructionCount(cmdqRecHandle handle)
{
	int32_t InstructionCount;

	if (NULL == handle)
		return 0;

	InstructionCount = handle->blockSize / CMDQ_INST_SIZE;

	return InstructionCount;
}

int32_t cmdqRecProfileMarker(cmdqRecHandle handle, const char *tag)
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
		if ((0 == handle->profileMarker.count) && (0 == handle->profileMarker.hSlot)) {
			status =
			    cmdqCoreAllocWriteAddress(CMDQ_MAX_PROFILE_MARKER_IN_TASK,
						      &allocatedStartPA);
			if (0 > status) {
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

		cmdqRecBackupRegisterToSlot(handle, hSlot, index, CMDQ_APXGPT2_COUNT);

		handle->profileMarker.tag[index] = (cmdqU32Ptr_t) (unsigned long)tag;
		handle->profileMarker.count += 1;
	} while (0);

	return status;
#else
	CMDQ_ERR("func:%s failed since CMDQ doesn't enable profile marker\n", __func__);
	return -EFAULT;
#endif
}

int32_t cmdqRecDumpCommand(cmdqRecHandle handle)
{
	int32_t status = 0;
	struct TaskStruct *pTask;

	if (NULL == handle)
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

int32_t cmdqRecEstimateCommandExecTime(const cmdqRecHandle handle)
{
	int32_t time = 0;

	if (NULL == handle)
		return -EFAULT;

	CMDQ_LOG("======REC 0x%p command execution time ESTIMATE:\n", handle);
	time = cmdq_prof_estimate_command_exe_time(handle->pBuffer, handle->blockSize);
	CMDQ_LOG("======REC 0x%p  END\n", handle);

	return time;
}

void cmdqRecDestroy(cmdqRecHandle handle)
{
	if (NULL == handle)
		return;

	if (NULL != handle->pRunningTask)
		cmdqRecStopLoop(handle);

	/* Free command buffer */
	kfree(handle->pBuffer);
	handle->pBuffer = NULL;

	/* Free command handle */
	kfree(handle);
}

int32_t cmdqRecSetNOP(cmdqRecHandle handle, uint32_t index)
{
	uint32_t *pCommand;
	uint32_t offsetIndex = index * CMDQ_INST_SIZE;

	if (NULL == handle || offsetIndex > (handle->blockSize - CMDQ_INST_SIZE))
		return -EFAULT;

	CMDQ_MSG("======REC 0x%p Set NOP to index: %d\n", handle, index);
	pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + offsetIndex);
	*pCommand++ = 8;
	*pCommand++ = (CMDQ_CODE_JUMP << 24) | (0 & 0x0FFFFFF);
	CMDQ_MSG("======REC 0x%p  END\n", handle);

	return index;
}

int32_t cmdqRecQueryOffset(cmdqRecHandle handle, uint32_t startIndex, const CMDQ_CODE_ENUM opCode,
			   CMDQ_EVENT_ENUM event)
{
	int32_t Offset = -1;
	uint32_t argA, argB;
	uint32_t *pCommand;
	uint32_t QueryIndex, MaxIndex;

	if (NULL == handle || (startIndex * CMDQ_INST_SIZE) > (handle->blockSize - CMDQ_INST_SIZE))
		return -EFAULT;

	switch (opCode) {
	case CMDQ_CODE_WFE:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		argB = ((1 << 31) | (1 << 15) | 1);
		argA = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_SET_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 1 */
		argB = ((1 << 31) | (1 << 16));
		argA = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_WAIT_NO_CLEAR:
		/* bit 0-11: wait_value, 1 */
		/* bit 15: to_wait, true */
		/* bit 31: to_update, false */
		argB = ((0 << 31) | (1 << 15) | 1);
		argA = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_CLEAR_TOKEN:
		/* this is actually WFE(SYNC) but with different parameter */
		/* interpretation */
		/* bit 15: to_wait, false */
		/* bit 31: to_update, true */
		/* bit 16-27: update_value, 0 */
		argB = ((1 << 31) | (0 << 16));
		argA = (CMDQ_CODE_WFE << 24) | cmdq_core_get_event_value(event);
		break;
	case CMDQ_CODE_PREFETCH_ENABLE:
		/* this is actually MARKER but with different parameter */
		/* interpretation */
		/* bit 53: non_suspendable, true */
		/* bit 48: no_inc_exec_cmds_cnt, true */
		/* bit 20: prefetch_marker, true */
		/* bit 17: prefetch_marker_en, true */
		/* bit 16: prefetch_en, true */
		argB = ((1 << 20) | (1 << 17) | (1 << 16));
		argA = (CMDQ_CODE_EOC << 24) | (0x1 << (53 - 32)) | (0x1 << (48 - 32));
		break;
	case CMDQ_CODE_PREFETCH_DISABLE:
		/* this is actually MARKER but with different parameter */
		/* interpretation */
		/* bit 48: no_inc_exec_cmds_cnt, true */
		/* bit 20: prefetch_marker, true */
		argB = (1 << 20);
		argA = (CMDQ_CODE_EOC << 24) | (0x1 << (48 - 32));
		break;
	default:
		CMDQ_MSG("This offset of instruction can not be queried.\n");
		return -EFAULT;
	}

	MaxIndex = handle->blockSize / CMDQ_INST_SIZE;
	for (QueryIndex = startIndex; QueryIndex < MaxIndex; QueryIndex++) {
		pCommand = (uint32_t *) ((uint8_t *) handle->pBuffer + QueryIndex * CMDQ_INST_SIZE);
		if ((argB == *pCommand++) && (argA == *pCommand)) {
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

int32_t cmdqRecAcquireResource(cmdqRecHandle handle, CMDQ_EVENT_ENUM resourceEvent)
{
	bool acquireResult;

	acquireResult = cmdqCoreAcquireResource(resourceEvent);
	if (!acquireResult) {
		CMDQ_LOG("Acquire resource (event:%d) failed, handle:0x%p\n", resourceEvent, handle);
		return -EFAULT;
	}
	return 0;
}

int32_t cmdqRecWriteForResource(cmdqRecHandle handle, CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	bool acquireResult;

	acquireResult = cmdqCoreAcquireResource(resourceEvent);
	if (!acquireResult) {
		CMDQ_LOG("Acquire resource (event:%d) failed, handle:0x%p\n", resourceEvent, handle);
		return -EFAULT;
	}

	return cmdqRecWrite(handle, addr, value, mask);
}

int32_t cmdqRecReleaseResource(cmdqRecHandle handle, CMDQ_EVENT_ENUM resourceEvent)
{
	cmdqCoreReleaseResource(resourceEvent);
	return cmdqRecSetEventToken(handle, resourceEvent);
}

int32_t cmdqRecWriteAndReleaseResource(cmdqRecHandle handle, CMDQ_EVENT_ENUM resourceEvent,
							uint32_t addr, uint32_t value, uint32_t mask)
{
	int32_t result;

	cmdqCoreReleaseResource(resourceEvent);
	result = cmdqRecWrite(handle, addr, value, mask);
	if (result >= 0)
		return cmdqRecSetEventToken(handle, resourceEvent);

	CMDQ_ERR("Write instruction fail and not release resource!\n");
	return result;
}

