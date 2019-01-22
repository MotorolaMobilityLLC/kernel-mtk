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
#include <linux/delay.h>

#include "cmdq_sec.h"
#include "cmdq_def.h"
#include "cmdq_virtual.h"
#include "cmdq_device.h"
#ifdef CMDQ_MET_READY
#include <linux/met_drv.h>
#endif

static DEFINE_MUTEX(gCmdqSecExecLock);	/* lock to protect atomic secure task execution */
static DEFINE_MUTEX(gCmdqSecProfileLock);	/* ensure atomic enable/disable secure path profile */
static struct list_head gCmdqSecContextList;	/* secure context list. note each porcess has its own sec context */

#if defined(CMDQ_SECURE_PATH_SUPPORT)

/* sectrace interface */
#ifdef CMDQ_SECTRACE_SUPPORT
#include <linux/sectrace.h>
#endif
/* secure path header */
#include "cmdqsectl_api.h"

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#include "isee_kernel_api.h"
#endif

static struct cmdqSecContextStruct *gCmdqSecContextHandle;	/* secure context to cmdqSecTL */

/* internal control */

/* Set 1 to open once for each process context, because of below reasons:
 * 1. mc_open_session is too slow (major)
 * 2. we entry secure world for config and trigger GCE, and wait in normal world
 */
#define CMDQ_OPEN_SESSION_ONCE (1)

#define UUID_STR "09010000000000000000000000000000"

/********************************************************************************
 * operator API
 *******************************************************************************/
int32_t cmdq_sec_init_context_impl(struct cmdqSecContextStruct *handle)
{
#if defined(CMDQ_GP_SUPPORT)
	int32_t status;

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
	while (!is_teei_ready()) {
		CMDQ_MSG("[SEC] TEE is not ready, wait...\n");
		msleep(1000);
	}
	CMDQ_LOG("[SEC] TEE is ready\n");
#endif

	status = TEEC_InitializeContext(UUID_STR, &(handle->gp_context));

	if (status != TEEC_SUCCESS) {
		/* print error message */
		CMDQ_ERR("[SEC]TEEC_InitializeContext fail: status[0x%x]\n", status);
	} else {
		CMDQ_LOG("[SEC]TEEC_InitializeContext: status[0x%x]\n", status);
	}

	return status;
#else
	int32_t status;
	enum mc_result mcRet = MC_DRV_ERR_UNKNOWN;
	int retry_cnt = 0, max_retry = 30;

	do {
		status = 0;
		mcRet = mc_open_device(MC_DEVICE_ID_DEFAULT);

		/* Currently, a process context limits to open mobicore device once, */
		/* and mc_open_device dose not support reference cout */
		/* so skip the false alarm error.... */
		if (mcRet == MC_DRV_ERR_INVALID_OPERATION) {
			CMDQ_MSG("[SEC]_MOBICORE_OPEN: already opened, continue to execution\n");
			status = 0;
			handle->openMobicoreByOther = 1;
		} else if (mcRet != MC_DRV_OK) {
			CMDQ_ERR("[SEC]_MOBICORE_OPEN: err[0x%x], retry[%d]\n", mcRet, retry_cnt);
			status = -1;
			msleep_interruptible(2000);
			retry_cnt++;
			continue;
		}
		break;
	} while (retry_cnt < max_retry);

	if (retry_cnt >= max_retry) {
		/* print error message */
		CMDQ_ERR("[SEC]_MOBICORE_OPEN fail: status[%d], mcRet[0x%x], retry[%d]\n", status, mcRet, retry_cnt);
	} else {
		CMDQ_MSG("[SEC]_MOBICORE_OPEN: status[%d], mcRet[0x%x], retry[%d]\n", status, mcRet, retry_cnt);
	}

	return status;
#endif
}

int32_t cmdq_sec_deinit_context_impl(struct cmdqSecContextStruct *handle)
{
#if defined(CMDQ_GP_SUPPORT)
	TEEC_FinalizeContext(&(handle->gp_context));
	return 0;
#else
	int32_t status = 0;
	enum mc_result mcRet = 0;

	if (handle->openMobicoreByOther == 1) {
		/* do nothing */
		/* let last user to close mobicore.... */
		CMDQ_MSG("[SEC]_MOBICORE_CLOSE: opened by other, bypass device close\n");
	} else {
		mcRet = mc_close_device(MC_DEVICE_ID_DEFAULT);
		CMDQ_MSG("[SEC]_MOBICORE_CLOSE: status[%d], ret[%0x], openMobicoreByOther[%d]\n",
			 status, mcRet, handle->openMobicoreByOther);
		if (mcRet != MC_DRV_OK) {
			CMDQ_ERR("[SEC]_MOBICORE_CLOSE: err[0x%x]\n", mcRet);
			status = -1;
		}
	}

	return status;
#endif
}

int32_t cmdq_sec_allocate_wsm_impl(struct cmdqSecContextStruct *handle)
{
	int32_t status = 0;

	if (handle->iwcMessage != NULL) {
		CMDQ_ERR("[SEC]_WSM_ALLOC: err[pWsm is not NULL]");
		return -1;
	}

	do {
#if defined(CMDQ_GP_SUPPORT)
		handle->shared_mem.size = sizeof(struct iwcCmdqMessage_t);
		handle->shared_mem.flags = TEEC_MEM_INPUT;
		status = TEEC_AllocateSharedMemory(&(handle->gp_context), &(handle->shared_mem));
		if (status != TEEC_SUCCESS) {
			CMDQ_ERR("[SEC]TEEC_WSM_ALLOC: err[0x%x]\n", status);
			status = -1;
		} else {
			CMDQ_MSG("[SEC]TEEC_WSM_ALLOC: status[0x%x], pWsm: 0x%p\n", status, handle->shared_mem.buffer);
			handle->iwcMessage = handle->shared_mem.buffer;
		}
#else
		enum mc_result mcRet = MC_DRV_OK;

		/* because world shared mem(WSM) will ba managed by mobicore device, not linux kernel */
		/* instead of vmalloc/kmalloc, call mc_malloc_wasm to alloc WSM to prvent error such as */
		/* "can not resolve tci physicall address" etc */
		mcRet = mc_malloc_wsm(MC_DEVICE_ID_DEFAULT, 0,
			sizeof(struct iwcCmdqMessage_t), (uint8_t **)&(handle->iwcMessage), 0);
		if (mcRet != MC_DRV_OK) {
			CMDQ_ERR("[SEC]_WSM_ALLOC: err[0x%x]\n", mcRet);
			status = -1;
		} else {
			CMDQ_MSG("[SEC]_WSM_ALLOC: status[%d], *ppWsm: 0x%p\n", status, handle->iwcMessage);
		}
#endif
	} while (false);

	return status;
}

int32_t cmdq_sec_free_wsm_impl(struct cmdqSecContextStruct *handle)
{
#if defined(CMDQ_GP_SUPPORT)
	TEEC_ReleaseSharedMemory(&(handle->shared_mem));
	handle->iwcMessage = NULL;
	return 0;
#else
	int32_t status = 0;
	enum mc_result mcRet = mc_free_wsm(MC_DEVICE_ID_DEFAULT, handle->iwcMessage);

	if (mcRet != MC_DRV_OK) {
		CMDQ_ERR("_WSM_FREE: err[0x%x]", mcRet);
		status = -1;
	} else {
		handle->iwcMessage = NULL;
		CMDQ_VERBOSE("_WSM_FREE: ret[0x%x]\n", mcRet);
	}

	return status;
#endif
}

int32_t cmdq_sec_open_session_impl(struct cmdqSecContextStruct *handle)
{
	int32_t status = 0;

	if (!handle->iwcMessage) {
		CMDQ_ERR("[SEC]_SESSION_OPEN: invalid param, pWsm[0x%p]\n", handle->iwcMessage);
		return -1;
	}

	do {
#if defined(CMDQ_GP_SUPPORT)
		status = TEEC_OpenSession(&handle->gp_context, &handle->sessionHandle,
			&handle->uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);

		if (status != TEEC_SUCCESS) {
			/* print error message */
			CMDQ_ERR("[SEC]TEEC_SESSION_OPEN fail: status[0x%x]\n", status);
		} else {
			CMDQ_MSG("[SEC]TEEC_SESSION_OPEN: status[0x%x]\n", status);
		}
#else
		int retry_cnt = 0, max_retry = 30;
		enum mc_result mcRet = MC_DRV_OK;

		handle->sessionHandle.device_id = MC_DEVICE_ID_DEFAULT;

		do {
			mcRet = mc_open_session(&handle->sessionHandle, &handle->uuid,
				handle->iwcMessage, sizeof(struct iwcCmdqMessage_t));
			if (mcRet != MC_DRV_OK) {
				CMDQ_ERR("[SEC]_SESSION_OPEN: err[0x%x], retry[%d]\n", mcRet, retry_cnt);
				retry_cnt++;
				msleep_interruptible(2000);
				status = -1;
				continue;
			}

			/* Open Session success */
			status = 0;
			break;
		} while (retry_cnt < max_retry);

		if (retry_cnt >= max_retry) {
			/* print error message */
			CMDQ_ERR("[SEC]_SESSION_OPEN fail: status[%d], mcRet[0x%x], retry[%d]\n",
				status, mcRet, retry_cnt);
		} else {
			CMDQ_MSG("[SEC]_SESSION_OPEN: status[%d], mcRet[0x%x], retry[%d]\n",
				status, mcRet, retry_cnt);
		}
#endif
	} while (0);

	return status;
}

int32_t cmdq_sec_close_session_impl(struct cmdqSecContextStruct *handle)
{
#if defined(CMDQ_GP_SUPPORT)
	TEEC_CloseSession(&(handle->sessionHandle));
	return 0;
#else
	int32_t status = 0;
	enum mc_result mcRet = mc_close_session(&(handle->sessionHandle));

	if (mcRet != MC_DRV_OK) {
		CMDQ_ERR("_SESSION_CLOSE: err[0x%x]", mcRet);
		status = -1;
	}
	return status;
#endif
}

int32_t cmdq_sec_init_session_unlocked(struct cmdqSecContextStruct *handle)
{
	int32_t openRet = 0;
	int32_t status = 0;

	CMDQ_MSG("[SEC]-->SESSION_INIT: iwcState[%d]\n", (handle->state));

	do {
#if CMDQ_OPEN_SESSION_ONCE
		if (handle->state >= IWC_SES_OPENED) {
			CMDQ_MSG("SESSION_INIT: already opened\n");
			break;
		}

		CMDQ_MSG("[SEC]SESSION_INIT: open new session[%d]\n", handle->state);
#endif
		CMDQ_VERBOSE
		    ("[SEC]SESSION_INIT: sessionHandle: 0x%p\n", &(handle->sessionHandle));

		CMDQ_PROF_START(current->pid, "CMDQ_SEC_INIT");

		if (handle->state < IWC_CONTEXT_INITED) {
			/* open mobicore device */
			openRet = cmdq_sec_init_context_impl(handle);
			if (openRet < 0) {
				status = -1;
				break;
			}
			handle->state = IWC_CONTEXT_INITED;
		}

		if (handle->state < IWC_WSM_ALLOCATED) {
			/* allocate world shared memory */
			if (cmdq_sec_allocate_wsm_impl(handle) < 0) {
				status = -1;
				break;
			}
			handle->state = IWC_WSM_ALLOCATED;
		}

		/* open a secure session */
		if (cmdq_sec_open_session_impl(handle) < 0) {
			status = -1;
			break;
		}
		handle->state = IWC_SES_OPENED;

		CMDQ_PROF_END(current->pid, "CMDQ_SEC_INIT");
	} while (0);

	CMDQ_MSG("[SEC]<--SESSION_INIT[%d]\n", status);
	return status;
}

void cmdq_sec_deinit_session_unlocked(struct cmdqSecContextStruct *handle)
{
	CMDQ_MSG("[SEC]-->SESSION_DEINIT\n");
	do {
		switch (handle->state) {
		case IWC_SES_ON_TRANSACTED:
		case IWC_SES_TRANSACTED:
		case IWC_SES_MSG_PACKAGED:
			/* continue next clean-up */
		case IWC_SES_OPENED:
			cmdq_sec_close_session_impl(handle);
			/* continue next clean-up */
		case IWC_WSM_ALLOCATED:
			cmdq_sec_free_wsm_impl(handle);
			/* continue next clean-up */
		case IWC_CONTEXT_INITED:
			cmdq_sec_deinit_context_impl(handle);
			/* continue next clean-up */
			break;
		case IWC_INIT:
			/* CMDQ_ERR("open secure driver failed\n"); */
			break;
		default:
			break;
		}

	} while (0);

	CMDQ_MSG("[SEC]<--SESSION_DEINIT\n");
}

int32_t cmdq_sec_execute_session_unlocked(struct cmdqSecContextStruct *handle,
									int32_t timeout_ms)
{
#if defined(CMDQ_GP_SUPPORT)
	int32_t status = 0;
	struct TEEC_Operation operation;
	u32 cmd = ((struct iwcCmdqMessage_t *)(handle->iwcMessage))->cmd;

	memset(&operation, 0, sizeof(struct TEEC_Operation));
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
	operation.param_types = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
					TEEC_NONE, TEEC_NONE, TEEC_NONE);
#else
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
				TEEC_NONE, TEEC_NONE, TEEC_NONE);
#endif
	operation.params[0].memref.parent = &(handle->shared_mem);
	operation.params[0].memref.size = sizeof(struct iwcCmdqMessage_t);
	operation.params[0].memref.offset = 0;

	status = TEEC_InvokeCommand(&(handle->sessionHandle), cmd, &operation, NULL);
	if (status != TEEC_SUCCESS) {
		/* print error and keep state */
		CMDQ_ERR("[SEC]EXEC: TEEC_InvokeCommand %u err:%d\n", cmd, status);
	} else {
		CMDQ_MSG("[SEC]EXEC: TEEC_InvokeCommand %u ret:%d\n", cmd, status);
		/* only update state in success */
		handle->state = IWC_SES_ON_TRANSACTED;
	}

	return status;
#else
	enum mc_result mcRet;
	int32_t status = 0;
	const int32_t secureWoldTimeout_ms = timeout_ms > 0 ?
		timeout_ms : MC_INFINITE_TIMEOUT;

	CMDQ_PROF_START(current->pid, "CMDQ_SEC_EXE");

	do {
		/* notify to secure world */
		mcRet = mc_notify(&(handle->sessionHandle));
		if (mcRet != MC_DRV_OK) {
			CMDQ_ERR("[SEC]EXEC: mc_notify err[0x%x]\n", mcRet);
			status = -1;
			break;
		}

		CMDQ_MSG("[SEC]EXEC: mc_notify ret[0x%x]\n", mcRet);

		handle->state = IWC_SES_TRANSACTED;

		/* wait respond */
		mcRet = mc_wait_notification(&(handle->sessionHandle), secureWoldTimeout_ms);
		if (mcRet == MC_DRV_ERR_TIMEOUT) {
			CMDQ_ERR
			    ("[SEC]EXEC: mc_wait_notification timeout, err[0x%x], secureWoldTimeout_ms[%d]\n",
			     mcRet, secureWoldTimeout_ms);
			status = -ETIMEDOUT;
			break;
		}

		if (mcRet != MC_DRV_OK) {
			CMDQ_ERR("[SEC]EXEC: mc_wait_notification err[0x%x]\n", mcRet);
			status = -1;
			break;
		}

		CMDQ_MSG("[SEC]EXEC: mc_wait_notification err[%d]\n", mcRet);

		handle->state = IWC_SES_ON_TRANSACTED;
	} while (0);

	CMDQ_PROF_END(current->pid, "CMDQ_SEC_EXE");

	return status;
#endif
}

int32_t cmdq_sec_fill_iwc_command_basic_unlocked(int32_t iwcCommand, void *_pTask, int32_t thread,
						 void *_pIwc)
{
	struct iwcCmdqMessage_t *pIwc;

	pIwc = (struct iwcCmdqMessage_t *) _pIwc;

	/* specify command id only, don't care other other */
	memset(pIwc, 0x0, sizeof(struct iwcCmdqMessage_t));
	pIwc->cmd = iwcCommand;

	/* medatada: debug config */
	pIwc->debug.logLevel = (cmdq_core_should_print_msg()) ? (1) : (0);
	pIwc->debug.enableProfile = cmdq_core_profile_enabled();

	return 0;
}

int32_t cmdq_sec_fill_iwc_command_msg_unlocked(int32_t iwcCommand, void *_pTask, int32_t thread,
					       void *_pIwc)
{
	int32_t status;

	const struct TaskStruct *pTask = (struct TaskStruct *) _pTask;
	struct iwcCmdqMessage_t *pIwc;
	/* cmdqSecDr will insert some instr */
	const uint32_t reservedCommandSize = 4 * CMDQ_INST_SIZE;
	struct CmdBufferStruct *cmd_buffer = NULL;

	/* check task first */
	if (!pTask) {
		CMDQ_ERR("[SEC]SESSION_MSG: Unable to fill message by empty task.\n");
		return -EFAULT;
	}
	status = 0;
	pIwc = (struct iwcCmdqMessage_t *) _pIwc;

	/* check command size first */
	if ((pTask->commandSize + reservedCommandSize) > CMDQ_TZ_CMD_BLOCK_SIZE) {
		CMDQ_ERR("[SEC]SESSION_MSG: pTask %p commandSize %d > %d\n",
			 pTask, pTask->commandSize, CMDQ_TZ_CMD_BLOCK_SIZE);
		return -EFAULT;
	}

	CMDQ_MSG("[SEC]-->SESSION_MSG: cmdId[%d]\n", iwcCommand);

	/* fill message buffer for inter world communication */
	memset(pIwc, 0x0, sizeof(struct iwcCmdqMessage_t));
	pIwc->cmd = iwcCommand;

	/* metadata */
	pIwc->command.metadata.enginesNeedDAPC = pTask->secData.enginesNeedDAPC;
	pIwc->command.metadata.enginesNeedPortSecurity = pTask->secData.enginesNeedPortSecurity;

	if (thread != CMDQ_INVALID_THREAD) {
		u8 *current_va = (u8 *)pIwc->command.pVABase;
		/* basic data */
		pIwc->command.scenario = pTask->scenario;
		pIwc->command.thread = thread;
		pIwc->command.priority = pTask->priority;
		pIwc->command.engineFlag = pTask->engineFlag;
		pIwc->command.hNormalTask = 0LL | ((unsigned long)pTask);

		list_for_each_entry(cmd_buffer, &pTask->cmd_buffer_list, listEntry) {
			bool is_last = list_is_last(&cmd_buffer->listEntry, &pTask->cmd_buffer_list);
			uint32_t copy_size = is_last ?
				CMDQ_CMD_BUFFER_SIZE - pTask->buf_available_size :
				CMDQ_CMD_BUFFER_SIZE - CMDQ_INST_SIZE;
			uint32_t *end_va = (u32 *)(current_va + copy_size);

			memcpy(current_va, (cmd_buffer->pVABase), (copy_size));

			/* we must reset the jump inst since now buffer is continues */
			if (is_last && ((end_va[-1] >> 24) & 0xff) == CMDQ_CODE_JUMP &&
				(end_va[-1] & 0x1) == 1) {
				end_va[-1] = CMDQ_CODE_JUMP << 24;
				end_va[-2] = 0x8;
			}

			current_va += copy_size;
		}

		pIwc->command.commandSize = (u32)(current_va - (u8 *)pIwc->command.pVABase);

		/* cookie */
		pIwc->command.waitCookie = pTask->secData.waitCookie;
		pIwc->command.resetExecCnt = pTask->secData.resetExecCnt;

		CMDQ_MSG("[SEC]SESSION_MSG: task:0x%p thread:%d size:%d flag:0x%016llx size:%zu\n",
			pTask, thread, pTask->commandSize, pTask->engineFlag, sizeof(pIwc->command));

		CMDQ_VERBOSE("[SEC]SESSION_MSG: addrList[%d][0x%p]\n",
			     pTask->secData.addrMetadataCount,
			     CMDQ_U32_PTR(pTask->secData.addrMetadatas));
		if (pTask->secData.addrMetadataCount > 0) {
			pIwc->command.metadata.addrListLength = pTask->secData.addrMetadataCount;
			memcpy((pIwc->command.metadata.addrList), CMDQ_U32_PTR(pTask->secData.addrMetadatas),
			       (pTask->secData.addrMetadataCount) * sizeof(struct iwcCmdqAddrMetadata_t));
		}

		/* medatada: debug config */
		pIwc->debug.logLevel = (cmdq_core_should_print_msg()) ? (1) : (0);
		pIwc->debug.enableProfile = cmdq_core_profile_enabled();
	} else {
		/* relase resource, or debug function will go here */
		CMDQ_VERBOSE("[SEC]-->SESSION_MSG: no task, cmdId[%d]\n", iwcCommand);
		pIwc->command.commandSize = 0;
		pIwc->command.metadata.addrListLength = 0;
	}

	CMDQ_MSG("[SEC]<--SESSION_MSG[%d]\n", status);
	return status;
}

/* TODO: when do release secure command buffer */
int32_t cmdq_sec_fill_iwc_resource_msg_unlocked(int32_t iwcCommand, void *_pTask, int32_t thread,
						void *_pIwc)
{
	struct iwcCmdqMessage_t *pIwc;
	struct cmdqSecSharedMemoryStruct *pSharedMem;

	pSharedMem = cmdq_core_get_secure_shared_memory();
	if (pSharedMem == NULL) {
		CMDQ_ERR("FILL:RES, NULL shared memory\n");
		return -EFAULT;
	}

	if (pSharedMem && pSharedMem->pVABase == NULL) {
		CMDQ_ERR("FILL:RES, %p shared memory has not init\n", pSharedMem);
		return -EFAULT;
	}

	pIwc = (struct iwcCmdqMessage_t *) _pIwc;
	memset(pIwc, 0x0, sizeof(struct iwcCmdqMessage_t));

	pIwc->cmd = iwcCommand;
	pIwc->pathResource.shareMemoyPA = 0LL | (pSharedMem->MVABase);
	pIwc->pathResource.size = pSharedMem->size;
#if defined(CMDQ_SECURE_PATH_NORMAL_IRQ) || defined(CMDQ_SECURE_PATH_HW_LOCK)
	pIwc->pathResource.useNormalIRQ = 1;
#else
	pIwc->pathResource.useNormalIRQ = 0;
#endif

	CMDQ_MSG("FILL:RES, shared memory:%pa(0x%llx), size:%d\n",
		 &(pSharedMem->MVABase), pIwc->pathResource.shareMemoyPA, pSharedMem->size);

	/* medatada: debug config */
	pIwc->debug.logLevel = (cmdq_core_should_print_msg()) ? (1) : (0);
	pIwc->debug.enableProfile = cmdq_core_profile_enabled();

	return 0;
}

int32_t cmdq_sec_fill_iwc_cancel_msg_unlocked(int32_t iwcCommand, void *_pTask, int32_t thread,
					      void *_pIwc)
{
	const struct TaskStruct *pTask = (struct TaskStruct *) _pTask;
	struct iwcCmdqMessage_t *pIwc;

	pIwc = (struct iwcCmdqMessage_t *) _pIwc;
	memset(pIwc, 0x0, sizeof(struct iwcCmdqMessage_t));

	pIwc->cmd = iwcCommand;
	pIwc->cancelTask.waitCookie = pTask->secData.waitCookie;
	pIwc->cancelTask.thread = thread;

	/* medatada: debug config */
	pIwc->debug.logLevel = (cmdq_core_should_print_msg()) ? (1) : (0);
	pIwc->debug.enableProfile = cmdq_core_profile_enabled();

	CMDQ_LOG("FILL:CANCEL_TASK: task: %p, thread:%d, cookie:%d\n",
		 pTask, thread, pTask->secData.waitCookie);

	return 0;
}

/********************************************************************************
 * context handle
 *******************************************************************************/
int32_t cmdq_sec_setup_context_session(struct cmdqSecContextStruct *handle)
{
	int32_t status = 0;

	/* init iwc parameter */
	if (handle->state == IWC_INIT) {
#if defined(CMDQ_GP_SUPPORT)
		/*09010000 0000 0000 0000000000000000*/
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
		handle->uuid = (struct TEEC_UUID) { 0x09010000, 0x0, 0x0,
			{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };
#else
		handle->uuid = (TEEC_UUID) { 0x09010000, 0x0, 0x0,
			{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };
#endif

#else
		handle->uuid = (struct mc_uuid_t){ { 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };
#endif
	}

	/* init secure session */
	status = cmdq_sec_init_session_unlocked(handle);
	CMDQ_MSG("SEC_SETUP: status[%d], tgid[%d]\n", status, handle->tgid);
	return status;
}

int32_t cmdq_sec_teardown_context_session(struct cmdqSecContextStruct *handle)
{
	int32_t status = 0;

	if (handle) {
		CMDQ_MSG("[SEC]SEC_TEARDOWN: state: %d, iwcMessage:0x%p\n",
			 handle->state, handle->iwcMessage);

		cmdq_sec_deinit_session_unlocked(handle);

		/* clrean up handle's attritubes */
		handle->state = IWC_INIT;

	} else {
		CMDQ_ERR("[SEC]SEC_TEARDOWN: null secCtxHandle\n");
		status = -1;
	}
	return status;
}

void cmdq_sec_handle_attach_status(struct TaskStruct *pTask, uint32_t iwcCommand,
	const struct iwcCmdqMessage_t *pIwc, int32_t sec_status_code, char **dispatch_mod_ptr)
{
	int index = 0;
	const struct iwcCmdqSecStatus_t *secStatus = NULL;

	if (!pIwc || !dispatch_mod_ptr)
		return;

	/* assign status ptr to print without task */
	secStatus = &pIwc->secStatus;

	if (pTask) {
		if (pTask->secStatus) {
			if (iwcCommand == CMD_CMDQ_TL_CANCEL_TASK) {
				const struct iwcCmdqSecStatus_t *last_sec_status = pTask->secStatus;

				/* cancel task uses same errored task thus secStatus may exist */
				CMDQ_ERR(
					"Last secure status: %d step: 0x%08x args: 0x%08x 0x%08x 0x%08x 0x%08x dispatch: %s task: 0x%p\n",
					last_sec_status->status, last_sec_status->step,
					last_sec_status->args[0], last_sec_status->args[1],
					last_sec_status->args[2], last_sec_status->args[3],
					last_sec_status->dispatch, pTask);
			} else {
				/* task should not send to secure twice, aee it */
				CMDQ_AEE("CMDQ", "Last secure status still exists, task: 0x%p command: %u\n",
					pTask, iwcCommand);
			}
			kfree(pTask->secStatus);
			pTask->secStatus = NULL;
		}

		pTask->secStatus = kzalloc(sizeof(struct iwcCmdqSecStatus_t), GFP_KERNEL);
		if (pTask->secStatus) {
			memcpy(pTask->secStatus, &pIwc->secStatus, sizeof(struct iwcCmdqSecStatus_t));
			secStatus = pTask->secStatus;
		}
	}

	if (secStatus->status != 0 || sec_status_code < 0) {
		/* secure status may contains debug information */
		CMDQ_ERR(
			"Secure status: %d (%d) step: 0x%08x args: 0x%08x 0x%08x 0x%08x 0x%08x dispatch: %s task: 0x%p\n",
			secStatus->status, sec_status_code, secStatus->step,
			secStatus->args[0], secStatus->args[1],
			secStatus->args[2], secStatus->args[3],
			secStatus->dispatch, pTask);
		for (index = 0; index < secStatus->inst_index; index += 2) {
			CMDQ_ERR("Secure instruction %d: 0x%08x:%08x\n", (index / 2),
				secStatus->sec_inst[index],
				secStatus->sec_inst[index+1]);
		}

		switch (secStatus->status) {
		case -CMDQ_ERR_ADDR_CONVERT_HANDLE_2_PA:
			*dispatch_mod_ptr = "TEE";
			break;
		case -CMDQ_ERR_ADDR_CONVERT_ALLOC_MVA:
		case -CMDQ_ERR_ADDR_CONVERT_ALLOC_MVA_N2S:
			switch (pIwc->command.thread) {
			case CMDQ_THREAD_SEC_PRIMARY_DISP:
			case CMDQ_THREAD_SEC_SUB_DISP:
				*dispatch_mod_ptr = "DISP";
				break;
			case CMDQ_THREAD_SEC_MDP:
				*dispatch_mod_ptr = "MDP";
				break;
			}
			break;
		}
	}
}

int32_t cmdq_sec_handle_session_reply_unlocked(const struct iwcCmdqMessage_t *pIwc,
					       const int32_t iwcCommand, struct TaskStruct *pTask,
					       void *data)
{
	int32_t status;
	int32_t iwcRsp;
	struct cmdqSecCancelTaskResultStruct *pCancelResult = NULL;

	/* get secure task execution result */
	iwcRsp = (pIwc)->rsp;
	status = iwcRsp;

	if (iwcCommand == CMD_CMDQ_TL_CANCEL_TASK) {
		pCancelResult = (struct cmdqSecCancelTaskResultStruct *) data;
		if (pCancelResult) {
			pCancelResult->throwAEE = pIwc->cancelTask.throwAEE;
			pCancelResult->hasReset = pIwc->cancelTask.hasReset;
			pCancelResult->irqFlag = pIwc->cancelTask.irqFlag;
			pCancelResult->errInstr[0] = pIwc->cancelTask.errInstr[0];	/* arg_b */
			pCancelResult->errInstr[1] = pIwc->cancelTask.errInstr[1];	/* arg_a */
			pCancelResult->regValue = pIwc->cancelTask.regValue;
			pCancelResult->pc = pIwc->cancelTask.pc;
		}

		/* for WFE, we specifically dump the event value */
		if (((pIwc->cancelTask.errInstr[1] & 0xFF000000) >> 24) == CMDQ_CODE_WFE) {
			const uint32_t eventID = 0x3FF & pIwc->cancelTask.errInstr[1];

			CMDQ_ERR
			    ("=============== [CMDQ] Error WFE Instruction Status ===============\n");
			CMDQ_ERR("CMDQ_SYNC_TOKEN_VAL of %s is %d\n",
				 cmdq_core_get_event_name(eventID), pIwc->cancelTask.regValue);
		}

		CMDQ_ERR
		    ("CANCEL_TASK: pTask %p, INST:(0x%08x, 0x%08x), throwAEE:%d, hasReset:%d, pc:0x%08x\n",
		     pTask, pIwc->cancelTask.errInstr[1], pIwc->cancelTask.errInstr[0],
		     pIwc->cancelTask.throwAEE, pIwc->cancelTask.hasReset, pIwc->cancelTask.pc);
	} else if (iwcCommand == CMD_CMDQ_TL_PATH_RES_ALLOCATE
		   || iwcCommand == CMD_CMDQ_TL_PATH_RES_RELEASE) {
		/* do nothing */
	} else {
		/* note we etnry SWd to config GCE, and wait execution result in NWd */
		/* update taskState only if config failed */
		if (pTask && iwcRsp < 0)
			pTask->taskState = TASK_STATE_ERROR;
	}

	/* log print */
	if (status < 0) {
		CMDQ_ERR("SEC_SEND: status[%d], cmdId[%d], iwcRsp[%d]\n",
			 status, iwcCommand, iwcRsp);
	} else {
		CMDQ_MSG("SEC_SEND: status[%d], cmdId[%d], iwcRsp[%d]\n",
			 status, iwcCommand, iwcRsp);
	}

	return status;
}


CmdqSecFillIwcCB cmdq_sec_get_iwc_msg_fill_cb_by_iwc_command(uint32_t iwcCommand)
{
	CmdqSecFillIwcCB cb = NULL;

	switch (iwcCommand) {
	case CMD_CMDQ_TL_CANCEL_TASK:
		cb = cmdq_sec_fill_iwc_cancel_msg_unlocked;
		break;
	case CMD_CMDQ_TL_PATH_RES_ALLOCATE:
	case CMD_CMDQ_TL_PATH_RES_RELEASE:
		cb = cmdq_sec_fill_iwc_resource_msg_unlocked;
		break;
	case CMD_CMDQ_TL_SUBMIT_TASK:
		cb = cmdq_sec_fill_iwc_command_msg_unlocked;
		break;
	case CMD_CMDQ_TL_TEST_HELLO_TL:
	case CMD_CMDQ_TL_TEST_DUMMY:
	default:
		cb = cmdq_sec_fill_iwc_command_basic_unlocked;
		break;
	};

	return cb;
}

int32_t cmdq_sec_send_context_session_message(struct cmdqSecContextStruct *handle,
			uint32_t iwcCommand, struct TaskStruct *pTask, int32_t thread, void *data)
{
	int32_t status;
	const int32_t timeout_ms = (3 * 1000);
	const CmdqSecFillIwcCB iwcFillCB =
		cmdq_sec_get_iwc_msg_fill_cb_by_iwc_command(iwcCommand);

	do {
		/* fill message bufer */
		status = iwcFillCB(iwcCommand, pTask, thread, (void *)(handle->iwcMessage));

		if (status < 0) {
			CMDQ_ERR("[SEC]fill msg buffer failed[%d], pid[%d:%d], cmdId[%d]\n",
				 status, current->tgid, current->pid, iwcCommand);
			break;
		}

		/* send message */
		status = cmdq_sec_execute_session_unlocked(handle, timeout_ms);
		if (status != 0) {
			CMDQ_ERR("[SEC]cmdq_sec_execute_session_unlocked failed[%d], pid[%d:%d]\n",
				 status, current->tgid, current->pid);
			break;
		}

		status =
		    cmdq_sec_handle_session_reply_unlocked(handle->iwcMessage, iwcCommand, pTask,
							   data);
	} while (0);

	return status;
}

void cmdq_sec_track_task_record(const uint32_t iwcCommand,
	struct TaskStruct *pTask, CMDQ_TIME *pEntrySec, CMDQ_TIME *pExitSec)
{
	if (!pTask)
		return;

	if (iwcCommand != CMD_CMDQ_TL_SUBMIT_TASK)
		return;

	/* record profile data */
	/* tbase timer/time support is not enough currently, */
	/* so we treats entry/exit timing to secure world as the trigger timing */
	pTask->entrySec = *pEntrySec;
	pTask->exitSec = *pExitSec;
	pTask->trigger = *pEntrySec;
}

int32_t cmdq_sec_submit_to_secure_world_async_unlocked(uint32_t iwcCommand,
		struct TaskStruct *pTask, int32_t thread, void *data, bool throwAEE)
{
	const int32_t tgid = current->tgid;
	const int32_t pid = current->pid;
	struct cmdqSecContextStruct *handle = NULL;
	int32_t status = 0;
	int32_t duration = 0;
	char long_msg[CMDQ_LONGSTRING_MAX];
	u32 msg_offset;
	s32 msg_max_size;
	char *dispatch_mod = "CMDQ";

	CMDQ_TIME tEntrySec;
	CMDQ_TIME tExitSec;

	CMDQ_MSG("[SEC]-->SEC_SUBMIT: tgid[%d:%d]\n", tgid, pid);
	do {
		/* find handle first */

		/* Unlike tBase user space API,
		 * tBase kernel API maintains a GLOBAL table to control mobicore device reference count.
		 * For kernel spece user, mc_open_device and session_handle don't depend on the process context.
		 * Therefore we use global secssion handle to inter-world commumication.
		 */
		if (gCmdqSecContextHandle == NULL)
			gCmdqSecContextHandle = cmdq_sec_context_handle_create(current->tgid);

		handle = gCmdqSecContextHandle;

		if (!handle) {
			CMDQ_ERR("SEC_SUBMIT: tgid %d err[NULL secCtxHandle]\n", tgid);
			status = -(CMDQ_ERR_NULL_SEC_CTX_HANDLE);
			break;
		}

		if (cmdq_sec_setup_context_session(handle) < 0) {
			status = -(CMDQ_ERR_SEC_CTX_SETUP);
			break;
		}

		tEntrySec = sched_clock();

		status =
		    cmdq_sec_send_context_session_message(handle, iwcCommand, pTask, thread, data);

		tExitSec = sched_clock();
		CMDQ_GET_TIME_IN_US_PART(tEntrySec, tExitSec, duration);
		cmdq_sec_track_task_record(iwcCommand, pTask, &tEntrySec, &tExitSec);

		cmdq_sec_handle_attach_status(pTask, iwcCommand, handle->iwcMessage, status, &dispatch_mod);

		/* release resource */
#if !(CMDQ_OPEN_SESSION_ONCE)
		cmdq_sec_teardown_context_session(handle)
#endif
		    /* Note we entry secure for config only and wait result in normal world. */
		    /* No need reset module HW for config failed case */
	} while (0);

	if (status == -ETIMEDOUT) {
		/* t-base strange issue, mc_wait_notification false timeout when secure world has done */
		/* because retry may failed, give up retry method */
		cmdq_long_string_init(true, long_msg, &msg_offset, &msg_max_size);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   "[SEC]<--SEC_SUBMIT: err[%d][mc_wait_notification timeout], pTask[0x%p], THR[%d],",
				   status, pTask, thread);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   " tgid[%d:%d], config_duration_ms[%d], cmdId[%d]\n",
				   tgid, pid, duration, iwcCommand);

		if (throwAEE)
			CMDQ_AEE("TEE", "%s", long_msg);

		cmdq_core_turnon_first_dump(pTask);
		cmdq_core_dump_secure_task_status();
		CMDQ_ERR("%s", long_msg);
	} else if (status < 0) {
		cmdq_long_string_init(true, long_msg, &msg_offset, &msg_max_size);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   "[SEC]<--SEC_SUBMIT: err[%d], pTask[0x%p], THR[%d], tgid[%d:%d],",
				   status, pTask, thread, tgid, pid);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   " config_duration_ms[%d], cmdId[%d]\n", duration, iwcCommand);

		if (throwAEE)
			CMDQ_AEE(dispatch_mod, "%s", long_msg);

		cmdq_core_turnon_first_dump(pTask);
		CMDQ_ERR("%s", long_msg);

		/* dump metadata first */
		if (pTask)
			cmdq_core_dump_secure_metadata(&(pTask->secData));
	} else {
		cmdq_long_string_init(false, long_msg, &msg_offset, &msg_max_size);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   "[SEC]<--SEC_SUBMIT: err[%d], pTask[0x%p], THR[%d], tgid[%d:%d],",
				   status, pTask, thread, tgid, pid);
		cmdq_long_string(long_msg, &msg_offset, &msg_max_size,
				   " config_duration_ms[%d], cmdId[%d]\n", duration, iwcCommand);
		CMDQ_MSG("%s", long_msg);
	}
	return status;
}

int32_t cmdq_sec_init_allocate_resource_thread(void *data)
{
	int32_t status = 0;

	cmdq_sec_lock_secure_path();

	gCmdqSecContextHandle = NULL;
	status = cmdq_sec_allocate_path_resource_unlocked(false);

	cmdq_sec_unlock_secure_path();

	return status;
}
#endif				/* CMDQ_SECURE_PATH_SUPPORT */

/********************************************************************************
 * common part: for general projects
 *******************************************************************************/
int32_t cmdq_sec_create_shared_memory(struct cmdqSecSharedMemoryStruct **pHandle, const uint32_t size)
{
	struct cmdqSecSharedMemoryStruct *handle = NULL;
	void *pVA = NULL;
	dma_addr_t PA = 0;

	handle = kzalloc(sizeof(uint8_t *) * sizeof(struct cmdqSecSharedMemoryStruct), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	CMDQ_LOG("%s\n", __func__);

	/* allocate non-cachable memory */
	pVA = cmdq_core_alloc_hw_buffer(cmdq_dev_get(), size, &PA, GFP_KERNEL);

	CMDQ_MSG("%s, MVA:%pa, pVA:%p, size:%d\n", __func__, &PA, pVA, size);

	if (pVA == NULL) {
		kfree(handle);
		return -ENOMEM;
	}

	/* update memory information */
	handle->size = size;
	handle->pVABase = pVA;
	handle->MVABase = PA;

	*pHandle = handle;

	return 0;
}

int32_t cmdq_sec_destroy_shared_memory(struct cmdqSecSharedMemoryStruct *handle)
{

	if (handle && handle->pVABase) {
		cmdq_core_free_hw_buffer(cmdq_dev_get(), handle->size,
					 handle->pVABase, handle->MVABase);
	}

	kfree(handle);
	handle = NULL;

	return 0;
}

int32_t cmdq_sec_exec_task_async_unlocked(struct TaskStruct *pTask, int32_t thread)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status = 0;

	status =
		cmdq_sec_submit_to_secure_world_async_unlocked(CMD_CMDQ_TL_SUBMIT_TASK, pTask, thread,
						NULL, true);
	if (status < 0) {
		/* Error status print */
		CMDQ_ERR("%s[%d]\n", __func__, status);
	}

	return status;

#else
	CMDQ_ERR("secure path not support\n");
	return -EFAULT;
#endif
}

int32_t cmdq_sec_cancel_error_task_unlocked(struct TaskStruct *pTask, int32_t thread,
					    struct cmdqSecCancelTaskResultStruct *pResult)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status = 0;

	if ((pTask == NULL) || (cmdq_get_func()->isSecureThread(thread) == false)
	    || (pResult == NULL)) {

		CMDQ_ERR("%s invalid param, pTask:%p, thread:%d, pResult:%p\n",
			 __func__, pTask, thread, pResult);
		return -EFAULT;
	}

	status = cmdq_sec_submit_to_secure_world_async_unlocked(CMD_CMDQ_TL_CANCEL_TASK,
								pTask, thread, (void *)pResult, true);
	return status;
#else
	CMDQ_ERR("secure path not support\n");
	return -EFAULT;
#endif
}

#ifdef CMDQ_SECURE_PATH_SUPPORT
static atomic_t gCmdqSecPathResource = ATOMIC_INIT(0);
#endif

int32_t cmdq_sec_allocate_path_resource_unlocked(bool throwAEE)
{
#ifdef CMDQ_SECURE_PATH_SUPPORT
	int32_t status = 0;

	CMDQ_MSG("%s throwAEE: %s", __func__, throwAEE ? "true" : "false");

	if (atomic_cmpxchg(&gCmdqSecPathResource, 0, 1) != 0) {
		/* has allocated successfully */
		return status;
	}

	status =
	    cmdq_sec_submit_to_secure_world_async_unlocked(CMD_CMDQ_TL_PATH_RES_ALLOCATE, NULL,
							   CMDQ_INVALID_THREAD, NULL, throwAEE);
	if (status < 0) {
		/* Error status print */
		CMDQ_ERR("%s[%d] reset context\n", __func__, status);

		/* in fail case, we want function alloc again */
		atomic_set(&gCmdqSecPathResource, 0);
	}

	return status;

#else
	CMDQ_ERR("secure path not support\n");
	return -EFAULT;
#endif
}

/********************************************************************************
 * common part: SecContextHandle handle
 *******************************************************************************/
struct cmdqSecContextStruct *cmdq_sec_context_handle_create(uint32_t tgid)
{
	struct cmdqSecContextStruct *handle = NULL;

	handle = kzalloc(sizeof(uint8_t *) * sizeof(struct cmdqSecContextStruct), GFP_ATOMIC);
	if (handle) {
		handle->state = IWC_INIT;
		handle->tgid = tgid;
	} else {
		CMDQ_ERR("SecCtxHandle_CREATE: err[LOW_MEM], tgid[%d]\n", tgid);
	}

	CMDQ_MSG("SecCtxHandle_CREATE: create new, H[0x%p], tgid[%d]\n", handle, tgid);
	return handle;
}

/********************************************************************************
 * common part: init, deinit, path
 *******************************************************************************/
void cmdq_sec_lock_secure_path(void)
{
	mutex_lock(&gCmdqSecExecLock);
	/* set memory barrier for lock */
	smp_mb();
}

void cmdq_sec_unlock_secure_path(void)
{
	mutex_unlock(&gCmdqSecExecLock);
}

void cmdqSecDeInitialize(void)
{
}

void cmdqSecInitialize(void)
{
	INIT_LIST_HEAD(&gCmdqSecContextList);
}

void cmdqSecEnableProfile(const bool enable)
{
}

