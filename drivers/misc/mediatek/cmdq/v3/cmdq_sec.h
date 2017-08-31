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

#ifndef __CMDQ_SEC_H__
#define __CMDQ_SEC_H__

#include "cmdq_core.h"

#if defined(CMDQ_SECURE_PATH_SUPPORT)
#include "tee_client_api.h"
#include "cmdq_sec_iwc_common.h"

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
#include "mobicore_driver_api.h"
#endif /* CONFIG_TRUSTONIC_TEE_SUPPORT */

#endif /* CMDQ_SECURE_PATH_SUPPORT */

/**
 * error code for CMDQ
 */

#define CMDQ_ERR_NULL_SEC_CTX_HANDLE (6000)
#define CMDQ_ERR_SEC_CTX_SETUP (6001)
#define CMDQ_ERR_SEC_CTX_TEARDOWN (6002)

/**
 * inter-world communication state
 */
enum CMDQ_IWC_STATE_ENUM {
	IWC_INIT = 0,
	IWC_CONTEXT_INITED = 1,
	IWC_WSM_ALLOCATED = 2,
	IWC_SES_OPENED = 3,
	IWC_SES_MSG_PACKAGED = 4,
	IWC_SES_TRANSACTED = 5,
	IWC_SES_ON_TRANSACTED = 6,
	IWC_END_OF_ENUM = 7,
};


/**
 * CMDQ secure context struct
 * note it is not global data, each process has its own CMDQ sec context
 */
struct cmdqSecContextStruct {
	struct list_head listEntry;

	/* basic info */
	uint32_t tgid;		/* tgid of process context */
	uint32_t referCount;	/* reference count for open cmdq device node */

	/* iwc state */
	enum CMDQ_IWC_STATE_ENUM state;

	/* iwc information */
	void *iwcMessage;	/* message buffer */
#if defined(CMDQ_SECURE_PATH_SUPPORT)
#if defined(CMDQ_GP_SUPPORT)

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
	struct TEEC_UUID uuid;	/* Universally Unique Identifier of secure tl/dr */
#else
	TEEC_UUID uuid;	/* Universally Unique Identifier of secure tl/dr */
#endif
	struct TEEC_Context gp_context; /* basic context */
	struct TEEC_Session sessionHandle; /* session handle */
	struct TEEC_SharedMemory shared_mem; /* shared memory */
#else
	struct mc_uuid_t uuid;	/* Universally Unique Identifier of secure tl/dr */
	struct mc_session_handle sessionHandle;	/* session handle */
	uint32_t openMobicoreByOther;	/* true if someone has opened mobicore device in this prpocess context */
#endif
#endif
};

int32_t cmdq_sec_init_allocate_resource_thread(void *data);

/**
 * Create and destroy non-cachable shared memory,
 * used to share data for CMDQ driver between NWd and SWd
 *
 * Be careful that we should not disvlose any information about secure buffer address of
 */
int32_t cmdq_sec_create_shared_memory(struct cmdqSecSharedMemoryStruct **pHandle, const uint32_t size);
int32_t cmdq_sec_destroy_shared_memory(struct cmdqSecSharedMemoryStruct *handle);

/**
 * Callback to fill message buffer for secure task
 *
 * Params:
 *     init32_t command id
 *     void*	pornter of TaskStruct
 *     int32_t  CMDQ HW thread id
 *     void*    the inter-world communication buffer
 * Return:
 *     >=0 for success;
 */
typedef int32_t(*CmdqSecFillIwcCB) (int32_t, void *, int32_t, void *);


/**
  * Entry secure world to handle secure path jobs
  * .submit task
  * .cancel error task
  */

int32_t cmdq_sec_exec_task_async_unlocked(struct TaskStruct *pTask, int32_t thread);
int32_t cmdq_sec_cancel_error_task_unlocked(struct TaskStruct *pTask, int32_t thread,
					    struct cmdqSecCancelTaskResultStruct *pResult);
int32_t cmdq_sec_allocate_path_resource_unlocked(bool throwAEE);


/**
  * secure path control
  */
void cmdq_sec_lock_secure_path(void);
void cmdq_sec_unlock_secure_path(void);

void cmdqSecInitialize(void);
void cmdqSecDeInitialize(void);

void cmdqSecEnableProfile(const bool enable);

/* function declaretion */
struct cmdqSecContextStruct *cmdq_sec_context_handle_create(uint32_t tgid);

#endif				/* __DDP_CMDQ_SEC_H__ */
