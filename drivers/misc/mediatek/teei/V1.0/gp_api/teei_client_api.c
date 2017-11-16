#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <tee_client_api.h>
#include <gp_client.h>
#include <ut_gp_api.h>
#include <imsg_log.h>
#define ZERO_SIZE_MAP 32

#define printk(fmt, args...) printk("\033[;34m[TEEI][TZDriver]"fmt"\033[0m", ##args)

inline uint32_t params_null_check(void *paras, char *error_log)
{
	if (paras == NULL) {
		printk("[E] Error Illegal argument : %s \n", error_log);
		return -1;
	}
	return 0;
}

inline uint32_t release_share_memory(UT_TEEC_SharedMemory *memory, int *ret)
{
	struct session_shared_mem_info info;
	void *buff_addr = NULL;
	int retVal = 0;
	void *buffer = NULL;

	if (memory->allocated != 0)
		buff_addr = memory->buffer;
	else
		buff_addr = memory->saved_buffer;

	list_del(&(memory->head_ref));

	info.session_id = 0;
	info.user_mem_addr = (unsigned long)buffer;
	retVal = ut_client_shared_mem_free(memory->context->fd, (struct teei_session_shared_mem_info *)(&info));
	if (retVal != 0)
		return retVal;

	/* clear the memory strucuture */
	memory->buffer = NULL;
	memory->saved_buffer = NULL;
	memory->size = 0;
	memory->context = NULL;
	kfree(memory);

	return 0;
}


/**
* @brief Initialize Context
*
* @param name: A zero-terminated string that describes the TEE to connect to.
* If this parameter is set to NULL the Implementation MUST select a default TEE.
*
* @param context: A TEEC_Context structure that MUST be initialized by the
* Implementation.
*
* @return TEEC_Result:
* TEEC_SUCCESS: The initialization was successful. \n
* TEEC_ERROR_*: An implementation-defined error code for any other error.
*/
TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	UT_TEEC_Context *ut_context = NULL;
	struct ctx_data ctx;
	long dev_file_id = 0;
	char temp_name[256];
	int retVal = 0;

	if (NULL == context) {
		IMSG_ERROR("[%s][%d] context is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (name == NULL) {
		strncpy(temp_name, DEFAULT_TEE, 256);
	} else {
		strncpy(temp_name, name, 256);
	}

	/* open file */
	dev_file_id = ut_client_open_dev();
	if (dev_file_id < 0) {
		IMSG_ERROR("TEEC_InitializeContext: device open failed，error code:%ld\n", dev_file_id);
		return TEEC_ERROR_GENERIC;
	} else {
		/* initialize ut context */
		ut_context = kmalloc(sizeof(UT_TEEC_Context), GFP_KERNEL);
		if (ut_context == NULL)
			return TEEC_ERROR_OUT_OF_MEMORY;

		memset(ut_context, 0x0, sizeof(UT_TEEC_Context));
		context->imp = ut_context;
		context->imp->fd = dev_file_id;

		/* init share memory and session list */
		INIT_LIST_HEAD(&context->imp->shared_mem_list);
		INIT_LIST_HEAD(&context->imp->session_list);
	}

	/* send initialize context ioctl request */

	strncpy(ctx.name, temp_name, 256);
	IMSG_DEBUG("TEEC_InitializeContext context->imp->fd is : %d\n", context->imp->fd);

	retVal = ut_client_context_init(context->imp->fd, &ctx);
	if (retVal != 0) {
		IMSG_ERROR("Failed to send ioctl request initialize context. return value is : %d\n", retVal);
		return TEEC_ERROR_GENERIC;
	}

	IMSG_DEBUG("Init TEE Context:%s", ctx.name);
	IMSG_DEBUG("Init TEE Context return value:%d\n", ctx.return_value);

	strncpy(context->imp->tee_name, ctx.name, 256);
	context->imp->s_errno = ctx.return_value;

	IMSG_DEBUG("[TEEC_InitalizeContext] end with return value :%x\n", ctx.return_value);
	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);
	return ctx.return_value;
}

/**
 * @brief Finalizes an initialized TEE context.
 *
 * @param context: An initialized TEEC_Context structure which is to be
 * finalized.
 */
void TEEC_FinalizeContext(TEEC_Context *context)
{
	struct session_shared_mem_info info;
	struct ser_ses_id ses_close;
	struct ctx_data ctx;
	UT_TEEC_SharedMemory *memory_item = NULL;
	UT_TEEC_Session *session_item = NULL;

	void *buffer = NULL;
	uint32_t size = 0;
	int retVal = 0;
	int result = 0;

	if (context == NULL) {
		IMSG_ERROR("TEEC_FinalizeContext context is null\n");
		return;
	}

	if (context->imp == NULL) {
		IMSG_ERROR("TEEC_FinalizeContext context->imp is null\n");
		return;
	}

	/* release share memory in context list */
	list_for_each_entry(memory_item, &(context->imp->shared_mem_list), head_ref) {
		/* prepare parameters */
		if (memory_item->size == 0)
			size = ZERO_SIZE_MAP;
		else
			size = memory_item->size;

		if (memory_item->allocated)
			buffer = memory_item->buffer;
		else
			buffer = memory_item->saved_buffer;

		info.session_id = 0;
		info.user_mem_addr = (unsigned long)buffer;

		retVal = ut_client_shared_mem_free(memory_item->context->fd, (struct teei_session_shared_mem_info *)(&info));
		if (retVal != 0)
			result = 1;
	}
	if (result == 0)
		INIT_LIST_HEAD(&context->imp->shared_mem_list);

	result = 0;
	/* close session in context list */

	list_for_each_entry(session_item, &(context->imp->session_list), head) {
		ses_close.session_id = session_item->session_id ;
		retVal = ut_client_session_close(session_item->device->fd, &ses_close);
		if (retVal != 0) {
			result = 1;
			IMSG_ERROR("TEEC_CloseSession: Session client close request failed\n");
		}
	}

	/* clear the session_list */
	if (result == 0)
		INIT_LIST_HEAD(&context->imp->session_list);

	/* if share memory and session list is empty, send close context ioctl request */
	if (list_empty(&(context->imp->session_list)) && list_empty(&(context->imp->shared_mem_list))) {
		IMSG_DEBUG("Session list and share memory list are empty\n");
		strncpy(ctx.name, context->imp->tee_name, 256);
		retVal = ut_client_context_close(context->imp->fd, &ctx);
		if (retVal != 0) {
			IMSG_ERROR("[%s]:ioctl CLIENT_IOCTL_CLOSECONTEXT_REQ failed, return value is %d\n", __func__, retVal);
			return;
		}
		ut_client_release(context->imp->fd);
		context->imp->fd = 0;
	} else
		IMSG_ERROR("Session list or share memory list is not empty");

	kfree(context->imp);
	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);
	return;
}

/**
 * @brief  Allocate a shared memory block.
 *
 * @param context: Pointer to the context
 * @param sharedMem: Pointer to the shared memory
 *
 * @return TEEC_Result:
 * TEEC_SUCCESS: The allocation was successful. \n
 * TEEC_ERROR_*: An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context, TEEC_SharedMemory *sharedMem)
{
	UT_TEEC_SharedMemory *uTSharedMem = NULL;

	if (context == NULL) {
		IMSG_ERROR("TEEC_AllocateSharedMemory context is null\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (sharedMem == NULL) {
		IMSG_ERROR("TEEC_AllocateSharedMemory share memory is null\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if ((sharedMem->flags != TEEC_MEM_INPUT) &&
		(sharedMem->flags != TEEC_MEM_OUTPUT) &&
		(sharedMem->flags != (TEEC_MEM_INPUT | TEEC_MEM_OUTPUT))) {
		IMSG_ERROR("[%s] : Error Illegal argument, share memory flag not exist.\n", __func__);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* initialize ut share memory */
	uTSharedMem = kmalloc(sizeof(UT_TEEC_SharedMemory), GFP_KERNEL);
	if (uTSharedMem == NULL)
		return TEEC_ERROR_OUT_OF_MEMORY;

	uTSharedMem->allocated = 1;
	uTSharedMem->context = context->imp;
	sharedMem->imp = uTSharedMem;

	/* map memory */
	if (sharedMem->size == 0)
		sharedMem->buffer = ut_client_map_mem(context->imp->fd, ZERO_SIZE_MAP);
	else
		sharedMem->buffer = ut_client_map_mem(context->imp->fd, sharedMem->size);

	if (sharedMem->buffer == NULL) {
		IMSG_ERROR("TEEC_AllocateSharedMemory failed\n");
		sharedMem->buffer = NULL;
		sharedMem->imp->buffer = NULL;
		kfree(uTSharedMem);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	uTSharedMem->buffer = sharedMem->buffer;
	uTSharedMem->size = sharedMem->size;
	uTSharedMem->saved_buffer = NULL;

	INIT_LIST_HEAD(&sharedMem->imp->head_ref);
	list_add_tail(&context->imp->shared_mem_list, &sharedMem->imp->head_ref);
	IMSG_ERROR("[%s][%d] end!\n", __func__, __LINE__);
	return TEEC_SUCCESS;
}

/**
 * @brief Register a allocated shared memory block.
 *
 * @param context: Pointer to the context
 * @param sharedMem: Pointer to the shared memory
 *
 * @return TEEC_Result:
 * TEEC_SUCCESS: The device was successfully opened. \n
 * TEEC_ERROR_*: An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context, TEEC_SharedMemory *sharedMem)
{
	UT_TEEC_SharedMemory *uTSharedMem = NULL;

	if (NULL == sharedMem) {
		IMSG_ERROR("TEEC_RegisterSharedMemory share memory is null\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (NULL == context) {
		IMSG_ERROR("TEEC_RegisterSharedMemory context is null\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if ((sharedMem->flags != TEEC_MEM_INPUT) &&
		(sharedMem->flags != TEEC_MEM_OUTPUT) &&
		(sharedMem->flags != (TEEC_MEM_INPUT | TEEC_MEM_OUTPUT))) {
		IMSG_ERROR("[%s] : Error Illegal argument, share memory flag not exist \n", __func__);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (NULL == sharedMem->buffer) {
		IMSG_ERROR("TEEC_RegisterSharedMemory sharedMem->buffer is null\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* initialize ut share memory */
	uTSharedMem = kmalloc(sizeof(UT_TEEC_SharedMemory), GFP_KERNEL);
	if (uTSharedMem == NULL)
		return TEEC_ERROR_OUT_OF_MEMORY;

	uTSharedMem->size = sharedMem->size;
	uTSharedMem->buffer = NULL;
	sharedMem->imp = uTSharedMem;



	if (sharedMem->size == 0)
		sharedMem->imp->saved_buffer = ut_client_map_mem(context->imp->fd, ZERO_SIZE_MAP);
	else
		sharedMem->imp->saved_buffer = ut_client_map_mem(context->imp->fd, sharedMem->size);

	if (sharedMem->imp->saved_buffer == NULL) {
		IMSG_ERROR("TEEC_RegisterSharedMemory failed\n");
		sharedMem->imp->saved_buffer = NULL;
		kfree(uTSharedMem);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memcpy(sharedMem->imp->saved_buffer, sharedMem->buffer, sharedMem->size);

	/* 0: register(save buffer) 1: buffer */
	sharedMem->imp->allocated = 0;
	sharedMem->imp->context = context->imp;

	INIT_LIST_HEAD(&sharedMem->imp->head_ref);
	list_add_tail(&context->imp->shared_mem_list, &sharedMem->imp->head_ref);
	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);

	return TEEC_SUCCESS;
}

/**
 * @brief Release a shared memory block
 *
 * @param sharedMem: Pointer to the shared memory
 */
void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMem)
{
	int retVal = 0;
	int result = 0;

	IMSG_DEBUG("[%s][%d] start!\n", __func__, __LINE__);

	if (sharedMem == NULL) {
		IMSG_ERROR("TEEC_ReleaseSharedMemory sharedMem is null\n");
		return;
	}

	retVal  = release_share_memory(sharedMem->imp, &result);
	if (retVal != 0) {
		IMSG_ERROR("Failed to release share memory \n");
		return;
	} else
		IMSG_DEBUG("Succeed to release share memroy \n");

	sharedMem->size = 0;
	sharedMem->buffer = NULL;
	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);

	return;
}

inline uint32_t set_inout(uint32_t type, uint32_t flag)
{
	uint32_t inout = 0;

	if (type == TEEC_VALUE_INPUT || type == TEEC_MEMREF_TEMP_INPUT || type == TEEC_MEMREF_PARTIAL_INPUT)
		inout = 0;
	else if (type == TEEC_VALUE_OUTPUT || type == TEEC_MEMREF_TEMP_OUTPUT || type == TEEC_MEMREF_PARTIAL_OUTPUT)
		inout = 1;
	else if (type == TEEC_VALUE_INOUT || type == TEEC_MEMREF_TEMP_INOUT || type == TEEC_MEMREF_PARTIAL_INOUT)
		inout = 2;
	else if (type == TEEC_MEMREF_WHOLE) {
		if (flag == (TEEC_MEM_INPUT | TEEC_MEM_OUTPUT))
			inout = 2;
		else if (flag == TEEC_MEM_INPUT)
			inout = 0;
		else if (flag == TEEC_MEM_OUTPUT)
			inout = 1;
	}
	return inout;
}

inline uint32_t ut_enc_value(EncodeCmd *enc, int dev_file_id, uint32_t *returnOrigin, uint32_t inout)
{
	uint32_t retVal = 0;

	enc->len  = sizeof(uint32_t);

	if (inout == 0 || inout == 2) {
		enc->param_type = PARAM_IN;
		retVal = ut_client_encode_uint32_64bit(dev_file_id, (struct teei_client_encode_cmd *)enc);
		if (retVal != 0) {
			if (returnOrigin != NULL)
				*returnOrigin = TEEC_ORIGIN_COMMS;
			return retVal;
		}
	}

	if (inout == 1 || inout == 2) {
		enc->param_type = PARAM_OUT;
		retVal = ut_client_encode_uint32_64bit(dev_file_id, (struct teei_client_encode_cmd *)enc);
		if (retVal != 0) {
			if (returnOrigin != NULL)
				*returnOrigin = TEEC_ORIGIN_COMMS;
			return retVal;
		}
	}
	return 0;
}

uint32_t ut_dec_value(EncodeCmd *enc, int dev_file_id, uint32_t *returnOrigin)
{
	uint32_t retVal = 0;

	enc->len  = sizeof(uint32_t);
	retVal = ut_client_decode_uint32(dev_file_id, (struct teei_client_encode_cmd *)enc);
	if (retVal != 0) {
		if (returnOrigin != NULL)
			*returnOrigin = TEEC_ORIGIN_COMMS;
		IMSG_ERROR("ut decoding value in client driver failed\n");
	}

	return retVal;
}

uint32_t ut_enc_register_data(int dev_file_id, EncodeCmd *enc, TEEC_Parameter params, uint32_t *returnOrigin, uint8_t isTemp)
{
	uint32_t retVal = 0;
	unsigned long temp_enc_data = 0;

	if (isTemp != 0) {
		enc->offset = 0;
	} else {
		if (params.memref.parent->imp->allocated) {
			temp_enc_data = (unsigned long)(params.memref.parent->buffer);
			enc->data = temp_enc_data;
		} else {
			if (params.memref.parent->imp->saved_buffer) {
				memcpy(params.memref.parent->imp->saved_buffer, params.memref.parent->buffer, enc->len);
				temp_enc_data = (unsigned long)(params.memref.parent->imp->saved_buffer);
				enc->data = temp_enc_data;

			} else {
				IMSG_ERROR("Cannot find saved buffer.\n");
				return TEEC_ERROR_RESOURCE_LIMIT;
			}
		}
	}

	retVal = ut_client_encode_mem_ref_64bit(dev_file_id, (struct teei_client_encode_cmd *)enc);

	if (retVal != 0) {
		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_COMMS;
		IMSG_ERROR("ENC_MEM_REF request sent failed.\n");
	}

	return retVal;
}

inline uint32_t ut_enc_memory(int dev_file_id, EncodeCmd *enc, TEEC_Parameter params, uint32_t *returnOrigin, uint32_t inout, uint8_t isTemp)
{
	uint32_t retVal = 0;

	if (inout == 0 || inout == 2) {
		enc->flags = MEM_SERVICE_RO;
		enc->param_type = PARAM_IN;
		retVal = ut_enc_register_data(dev_file_id, enc, params, returnOrigin, isTemp);
		if (retVal != 0) {
			IMSG_ERROR("encoding data in client driver failed\n");
			return retVal;
		}
	}

	if (inout == 1 || inout == 2) {
		enc->flags = MEM_SERVICE_WO;
		enc->param_type = PARAM_OUT;
		retVal = ut_enc_register_data(dev_file_id, enc, params, returnOrigin, isTemp);
		if (retVal) {
			IMSG_ERROR("encoding data in client driver failed\n");
			return retVal;
		}
	}

	return retVal;
}

/**
 * @brief Open a session with a Trusted application
 *
 * @param context: Pointer to the context
 * @param session: Pointer to the session
 * @param destination: Service UUID
 * @param connectionMethod: Connection method
 * @param connectionData: Connection data used for authentication
 * @param operation: Pointer to optional operation structure
 * @param returnOrigin: Pointer to the return origin
 *
 * @return TEEC_Result:
 * TEEC_SUCCESS: The session was successfully opened. \n
 * TEEC_ERROR_*: An implementation-defined error code for any other error.
 */

TEEC_Result TEEC_OpenSession(
	TEEC_Context *context,
	TEEC_Session *session,
	const TEEC_UUID *destination,
	uint32_t connectionMethod,
	const void *connectionData,
	TEEC_Operation *operation,
	uint32_t *returnOrigin)
{

	struct ser_ses_id ses_open;
	struct user_ses_init ses_init;
	int retVal = 0;
	unsigned long temp_enc_data = 0;
	unsigned char inout = 0;
	uint32_t param_types[4];
	uint32_t param_count = 0;
	EncodeCmd enc;
	UT_TEEC_Session *imp = NULL;

	memset((void *)&ses_open, 0, sizeof(ses_open));
	memset((void *)&ses_init, 0, sizeof(ses_init));

	if ((context == NULL) || (context->imp == NULL) || (session == NULL)) {
		IMSG_ERROR("%s paraments error!\n", __func__);
		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_API;
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (destination == NULL) {
		IMSG_ERROR("TEEC_OpenSession destination is null\n");
		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_API;
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (connectionMethod != TEEC_LOGIN_PUBLIC &&
		connectionMethod != TEEC_LOGIN_USER &&
		connectionMethod != TEEC_LOGIN_GROUP &&
		connectionMethod != TEEC_LOGIN_APPLICATION &&
		connectionMethod != TEEC_LOGIN_USER_APPLICATION &&
		connectionMethod != TEEC_LOGIN_GROUP_APPLICATION) {

		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_ANY_NOT_TRUSTED_APP;
		return TEEC_UNDEFINED_ERROR;
	}

	if (connectionMethod == TEEC_LOGIN_PUBLIC ||
		connectionMethod == TEEC_LOGIN_USER ||
		connectionMethod == TEEC_LOGIN_APPLICATION ||
		connectionMethod == TEEC_LOGIN_USER_APPLICATION) {
		if (connectionData != NULL) {
			if (returnOrigin != NULL)
				*returnOrigin = TEEC_ORIGIN_TRUSTED_APP;
			return TEEC_ERROR_BAD_PARAMETERS;
		}
	} else if (connectionMethod == TEEC_LOGIN_GROUP || connectionMethod == TEEC_LOGIN_GROUP_APPLICATION) {
		if (connectionData == NULL) {
			if (returnOrigin != NULL)
				*returnOrigin = TEEC_ORIGIN_TRUSTED_APP;
			return TEEC_ERROR_BAD_PARAMETERS;
		}
	}

	/* send initialize session ioctl request */
	IMSG_DEBUG("[%s] init session\n", __func__);
	retVal = ut_client_session_init(context->imp->fd, &ses_init);
	if (retVal != 0)
		IMSG_ERROR("init failed [%x]\n", retVal);

	IMSG_DEBUG("init session id [%x]\n", ses_init.session_id);
	IMSG_DEBUG("[%s] init session end\n", __func__);
	memcpy(&ses_open.uuid, (const void *)destination, sizeof(*destination));

	if ((operation != NULL) && (operation->started == 0)) {
		IMSG_ERROR("[%s] : cancellation, operation->started is %d\n ", __func__, operation->started);
		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_API;

		return TEEC_SUCCESS;
	}

	/* init ut session */
	imp = kmalloc(sizeof(UT_TEEC_Session), GFP_KERNEL);
	memset(imp, 0x0, sizeof(UT_TEEC_Session));
	session->imp = imp;
	session->imp->device = context->imp;

	enc.encode_id = -1;
	enc.cmd_id = 0xFFFFEEEE;
	enc.session_id = ses_init.session_id;

	/* Encode the data */
	if ((operation != NULL) && (operation->paramTypes != 0)) {
		IMSG_DEBUG("[%s] encode params start\n", __func__);
		param_types[0] = operation->paramTypes & 0xf;
		param_types[1] = (operation->paramTypes >> 4) & 0xf;
		param_types[2] = (operation->paramTypes >> 8) & 0xf;
		param_types[3] = (operation->paramTypes >> 12) & 0xf;

		/* loop parameters */
		for (param_count = 0; param_count < 4; param_count++) {
			enc.param_pos = param_count;
			enc.param_pos_type = param_types[param_count];

			/* check value type */
			if ((param_types[param_count] == TEEC_VALUE_INPUT) || (param_types[param_count] == TEEC_VALUE_INOUT)
				|| (param_types[param_count] == TEEC_VALUE_OUTPUT)) {

				inout = set_inout(param_types[param_count], 0);
				temp_enc_data = (unsigned long)&operation->params[param_count].value.a;
				enc.data = temp_enc_data;
				enc.value_flag = PARAM_A;
				retVal = ut_enc_value(&enc, context->imp->fd, returnOrigin, inout);
				if (retVal != 0) {
					session->imp->s_errno = retVal;
					break;
				}

				temp_enc_data = (unsigned long)&operation->params[param_count].value.b;
				enc.data = temp_enc_data;
				enc.value_flag = PARAM_B;
				retVal = ut_enc_value(&enc, context->imp->fd, returnOrigin, inout);
				if (retVal != 0) {
					session->imp->s_errno = retVal;
					break;
				}

			/* check share memory type */
			} else if ((param_types[param_count] == TEEC_MEMREF_WHOLE) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_INPUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {

				IMSG_DEBUG("[%s] encode memref start", __func__);
				inout = set_inout(param_types[param_count], operation->params[param_count].memref.parent->flags);

				if (operation->params[param_count].memref.parent == NULL) {
					IMSG_ERROR("TEEC_OpenSession:memory reference parent is NULL\n");
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
					retVal = TEEC_ERROR_NO_DATA;
					break;
				}

				if (operation->params[param_count].memref.parent->buffer == NULL) {
					IMSG_ERROR("TEEC_OpenSession:memory reference parent data is NULL\n");
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;

					retVal = TEEC_ERROR_NO_DATA;
					break;
				}

				if ((param_types[param_count] == TEEC_MEMREF_PARTIAL_INPUT && (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_INPUT))) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT && !(operation->params[param_count].memref.parent->flags & TEEC_MEM_OUTPUT))) {
					IMSG_ERROR("TEEC_OpenSession:memory reference direction is invalid\n");
					if (returnOrigin)
						*returnOrigin = TEEC_ORIGIN_API;
					retVal = TEEC_ERROR_BAD_FORMAT;
					break;
				}

				if (param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) {
					if (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_INPUT) ||
						(!(operation->params[param_count].memref.parent->flags & TEEC_MEM_OUTPUT))) {
						IMSG_ERROR("TEEC_OpenSession:memory reference direction is invalid\n");
						if (returnOrigin != NULL)
							*returnOrigin = TEEC_ORIGIN_API;
						retVal = TEEC_ERROR_BAD_FORMAT;
						break;
					}
				}

				if (param_types[param_count] == TEEC_MEMREF_WHOLE)
					enc.offset = 0;
				else
					enc.offset = operation->params[param_count].memref.offset;

				if (operation->params[param_count].memref.parent->size > operation->params[param_count].memref.size)
					enc.len = operation->params[param_count].memref.size;
				else
					enc.len = operation->params[param_count].memref.parent->size;

				retVal = ut_enc_memory(context->imp->fd, &enc, operation->params[param_count], returnOrigin, inout, 0);
				if (retVal != 0) {
					session->imp->s_errno = retVal;
					break;
				}
				IMSG_DEBUG("[%s] encode memref end\n", __func__);

			/* check temp memory type */
			} else if ((param_types[param_count] == TEEC_MEMREF_TEMP_INPUT) || (param_types[param_count] == TEEC_MEMREF_TEMP_OUTPUT) ||
					(param_types[param_count] == TEEC_MEMREF_TEMP_INOUT)) {

				IMSG_DEBUG("[%s] encode mem temp start\n", __func__);
				inout = set_inout(param_types[param_count], 0);
				if (operation->params[param_count].tmpref.buffer == NULL) {
					IMSG_ERROR("TEEC_OpenSession:temporary memory reference buffer is NULL\n");
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
					retVal = TEEC_ERROR_NO_DATA;
					break;
				}

				/* This is a variation of API spec. */
				if (operation->params[param_count].tmpref.size == 0) {
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;

					retVal = TEEC_ERROR_NO_DATA;
					IMSG_ERROR("TEEC_OpenSession:temporary memory reference size zero is not supported\n");
					break;
				}
				enc.len = operation->params[param_count].tmpref.size;
				if (enc.len == 0) {
					temp_enc_data = (unsigned long)ut_client_map_mem(session->imp->device->fd, ZERO_SIZE_MAP);
					enc.data = temp_enc_data;
				} else {
					temp_enc_data = (unsigned long)ut_client_map_mem(session->imp->device->fd, enc.len);
					enc.data = temp_enc_data;
				}

				if (enc.data == 0) {
					IMSG_ERROR("alloc temp share memory failed\n");
					enc.data = 0;
					return TEEC_ERROR_OUT_OF_MEMORY;
				}

				memcpy((void *)(enc.data), operation->params[param_count].tmpref.buffer, operation->params[param_count].tmpref.size);
				retVal = ut_enc_memory(context->imp->fd, &enc, operation->params[param_count], returnOrigin, inout, 1);
				if (retVal != 0) {
					session->imp->s_errno = retVal;
					break;
				}

				IMSG_DEBUG("[%s] encode mem temp end\n", __func__);
			} /* end if for temp reference */
		} /* end for */
		IMSG_DEBUG("[%s] encode params end", __func__);
	} /* end paramtype */
	if (retVal != 0)
		IMSG_ERROR("error in encoding the data\n");

	IMSG_DEBUG("[%s] open session start\n", __func__);

	/* send open session ioctl request */
	ses_open.session_id = ses_init.session_id;
	ses_open.paramtype = enc.encode_id;
	retVal = ut_client_session_open(context->imp->fd, &ses_open);
	if (retVal == 0) {
		if (returnOrigin != NULL)
			*returnOrigin = ses_open.return_origin;
	} else {
		if (returnOrigin != NULL)
			*returnOrigin = TEEC_ORIGIN_COMMS;

		context->imp->s_errno = -(retVal);
		goto operation_release;

		IMSG_ERROR("TEEC_OpenSession: Session client open request failed\n");

		if (retVal == -ENOMEM)
			return TEEC_ERROR_OUT_OF_MEMORY;

		if (retVal == -EFAULT)
			return  TEEC_ERROR_ACCESS_DENIED;

		if (retVal == -EINVAL)
			return  TEEC_ERROR_BAD_PARAMETERS;

		return TEEC_ERROR_GENERIC;
	}
	enc.session_id = ses_open.session_id;

	/* Decode the data */
	if ((operation != NULL) && (operation->paramTypes != 0)) {
		for (param_count = 0; param_count < 4; param_count++) {
			if ((param_types[param_count] == TEEC_VALUE_INOUT) ||
				(param_types[param_count] == TEEC_VALUE_OUTPUT)) {

				retVal = ut_dec_value(&enc, context->imp->fd, returnOrigin);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				} else
					operation->params[param_count].value.a = *((uint32_t *)enc.data);

				retVal = ut_dec_value(&enc, context->imp->fd, returnOrigin);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				} else
					operation->params[param_count].value.b = *((uint32_t *)enc.data);

			} else if ((param_types[param_count] == TEEC_MEMREF_WHOLE) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT) || (param_types[param_count] == TEEC_MEMREF_TEMP_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_TEMP_OUTPUT)) {

				if (inout == 0)
					continue;

				retVal = ut_client_decode_array_space(context->imp->fd, (struct teei_client_encode_cmd *)(&enc));

				if (operation->params[param_count].memref.parent->imp->allocated == 0) {
					memcpy(operation->params[param_count].memref.parent->buffer,
						(const void *)enc.data, operation->params[param_count].memref.parent->size);
				}

				if (retVal != 0) {
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_COMMS;
					session->imp->s_errno = -(retVal);
					IMSG_ERROR("TEEC_InvokeCommand CLIENT_IOCTL_DEC_ARRAY_SPACE: decoding data in client driver failed\n");
					break;
				}

				if ((param_types[param_count] == TEEC_MEMREF_WHOLE) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {
					operation->params[param_count].memref.size = enc.len;
				} else
					operation->params[param_count].tmpref.size = enc.len;

			} /* end if for TEEC_MEM_REF check */
		} /* end for */
	} /* end paramtype */

	if (retVal != TEEC_SUCCESS)
		IMSG_ERROR("error in decoding the data\n");

operation_release:
	/* release the operation */
	if (operation != NULL) {
		retVal = ut_client_operation_release(context->imp->fd, (struct teei_client_encode_cmd *)&enc);
		if (retVal != 0) {
			if (returnOrigin != NULL)
				*returnOrigin = TEEC_ORIGIN_COMMS;
			IMSG_ERROR("Operation release failed\n");
		}
	}

	if (ses_open.return_value == TEEC_SUCCESS) {
		session->imp->session_id = ses_open.session_id;
		session->imp->device = context->imp;
		INIT_LIST_HEAD(&session->imp->head);
		list_add_tail(&context->imp->session_list, &session->imp->head);
	}

	IMSG_DEBUG("[%s] end, return value %x\n", __func__, ses_open.return_value);
	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);

	return ses_open.return_value;
}

/**
 * @brief Closes a session which has been opened with trusted application
 *
 * @param session: Pointer to the session structure
 */
void TEEC_CloseSession(TEEC_Session *session)
{
	int retVal = 0;
	struct ser_ses_id ses_close;

	IMSG_DEBUG("[%s][%d] start!\n", __func__, __LINE__);
	if (session == NULL) {
		IMSG_ERROR("TEEC_CloseSession session is null\n");
		return;
	}

	if (session->imp == NULL) {
		IMSG_ERROR("TEEC_CloseSession session->imp is null\n");
		return;
	}

	ses_close.session_id = session->imp->session_id;
	retVal = ut_client_session_close(session->imp->device->fd, &ses_close);

	if (retVal == 0) {
		list_del(&(session->imp->head));
		session->imp->device = NULL;
		session->imp->session_id = -1;
		kfree(session->imp);
	} else
		IMSG_ERROR("TEEC_CloseSession: Session client close request failed\n");

	IMSG_DEBUG("[%s][%d] end!\n", __func__, __LINE__);
}

/**
 * @brief Invokes a command within the specified session
 *
 * @param session: Pointer to session
 * @param commandID: Command ID
 * @param operation: Pointer to operation structure
 * @param returnOrigin: Pointer to the return origin
 *
 * @return TEEC_Result:
 * TEEC_SUCCESS: The command was successfully invoked. \n
 * TEEC_ERROR_*: An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_InvokeCommand(TEEC_Session *session, uint32_t commandID, TEEC_Operation *operation, uint32_t *returnOrigin)
{
	int retVal = TEEC_SUCCESS;
	unsigned char inout = 0; /* in = 0; out = 1; inout = 2 */
	uint32_t param_types[4], param_count;
	EncodeCmd enc;
	void *temp_buffer[4];
	unsigned long temp_enc_data = 0;

	IMSG_DEBUG("[%s][%d] start!\n", __func__, __LINE__);

	if (session == NULL) {
		IMSG_ERROR("TEEC_InvokeCommand session is NULL\n");
		if (returnOrigin)
			*returnOrigin = TEEC_ORIGIN_API;
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	/* Need to support cancellation in future releases */
	IMSG_DEBUG("session imp device %p\n", session->imp->device);

	if ((operation != NULL) && (operation->started == 0))
		IMSG_ERROR("[%s] : cancellation, operation->started is %d \n ", __func__, operation->started);

	enc.encode_id = -1;
	enc.cmd_id = commandID;
	enc.session_id = session->imp->session_id;

	/* Encode the data */
	IMSG_DEBUG("Encode the data\n");
	if ((operation != NULL) && (operation->paramTypes != 0)) {
		param_types[0] = operation->paramTypes & 0xf;
		param_types[1] = (operation->paramTypes >> 4) & 0xf;
		param_types[2] = (operation->paramTypes >> 8) & 0xf;
		param_types[3] = (operation->paramTypes >> 12) & 0xf;

		for (param_count = 0; param_count < 4; param_count++) {
			enc.param_pos = param_count;
			enc.param_pos_type = param_types[param_count];

			/* start if for TEEC_VALUE check */
			if ((param_types[param_count] == TEEC_VALUE_INPUT) || (param_types[param_count] == TEEC_VALUE_INOUT)
				|| (param_types[param_count] == TEEC_VALUE_OUTPUT)) {

				inout = set_inout(param_types[param_count], 0);
				temp_enc_data = (unsigned long)(&operation->params[param_count].value.a);
				enc.data = temp_enc_data;
				enc.len  = sizeof(uint32_t);
				enc.value_flag = PARAM_A;
				retVal = ut_enc_value(&enc, session->imp->device->fd, returnOrigin, inout);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				}

				temp_enc_data = (unsigned long)(&operation->params[param_count].value.b);
				enc.data = temp_enc_data;

				enc.len  = sizeof(uint32_t);
				enc.value_flag = PARAM_B;
				retVal = ut_enc_value(&enc, session->imp->device->fd, returnOrigin, inout);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				}
			} /* end if for TEEC_VALUE check */

			/* start if for TEEC_MEM_REF check */
			else if ((param_types[param_count] == TEEC_MEMREF_WHOLE) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_INPUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {

				if (operation->params[param_count].memref.parent == NULL) {
					IMSG_ERROR("TEEC_InvokeCommand: memory reference parent is NULL\n");
					retVal = TEEC_ERROR_NO_DATA;
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
				}

				if (operation->params[param_count].memref.parent->buffer == NULL) {
					IMSG_ERROR("TEEC_InvokeCommand: memory reference parent data is NULL\n");
					retVal = TEEC_ERROR_NO_DATA;
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
				}

				if (param_types[param_count] == TEEC_MEMREF_PARTIAL_INPUT) {
					if (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_INPUT)) {
						if (returnOrigin != NULL)
							*returnOrigin = TEEC_ORIGIN_API;

						retVal = TEEC_ERROR_BAD_FORMAT;
						IMSG_ERROR("TEEC_InvokeCommand: memory reference direction[INPUT] is invalid\n");
						break;
					}
				}

				if (param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT) {
					if (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_OUTPUT)) {
						if (returnOrigin != NULL)
							*returnOrigin = TEEC_ORIGIN_API;

						retVal = TEEC_ERROR_BAD_FORMAT;
						IMSG_ERROR("TEEC_InvokeCommand: memory reference direction[OUTPUT] is invalid\n");
						break;
					}
				}

				if (param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) {
					if (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_INPUT)) {
						if (returnOrigin != NULL)
							*returnOrigin = TEEC_ORIGIN_API;

						retVal = TEEC_ERROR_BAD_FORMAT;
						IMSG_ERROR("TEEC_InvokeCommand: memory reference direction[INOUT-IN] is invalid\n");
						break;
					}

					if (!(operation->params[param_count].memref.parent->flags & TEEC_MEM_OUTPUT)) {
						if (returnOrigin != NULL)
							*returnOrigin = TEEC_ORIGIN_API;
						retVal = TEEC_ERROR_BAD_FORMAT;
						IMSG_ERROR("TEEC_InvokeCommand: memory reference direction[INOUT-OUT] is invalid\n");
						break;
					}
				}

				if ((param_types[param_count] == TEEC_MEMREF_PARTIAL_INPUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {
					/*
					 *if ((operation->params[param_count].memref.offset + operation->params[param_count].memref.size >
					 *	operation->params[param_count].memref.parent->size)) {
					 *	if(returnOrigin)
					 *		*returnOrigin = TEEC_ORIGIN_API;
					 *	retVal = TEEC_ERROR_EXCESS_DATA;
					 *	IMSG_ERROR("TEEC_InvokeCommand: memory reference offset + size is greater than the actual memory size\n");
					 *	break;
					 *}
					*/
				}

				inout = set_inout(param_types[param_count], operation->params[param_count].memref.parent->flags);

				if (param_types[param_count] == TEEC_MEMREF_WHOLE)
					enc.offset = 0;
				else
					enc.offset = operation->params[param_count].memref.offset;

				if (operation->params[param_count].memref.parent->size > operation->params[param_count].memref.size)
					enc.len = operation->params[param_count].memref.size;
				else
					enc.len = operation->params[param_count].memref.parent->size;

				retVal = ut_enc_memory(session->imp->device->fd, &enc, operation->params[param_count], returnOrigin, inout, 0);

				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				}
			} /* end if for TEEC_MEM_REF check */
			else if ((param_types[param_count] == TEEC_MEMREF_TEMP_INPUT) ||
				(param_types[param_count] == TEEC_MEMREF_TEMP_OUTPUT) ||
				(param_types[param_count] == TEEC_MEMREF_TEMP_INOUT)) {

				if (operation->params[param_count].tmpref.buffer == NULL) {
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
					retVal = TEEC_ERROR_NO_DATA;
					break;
				}

				/* This is a variation of API spec. */
				IMSG_DEBUG("TEEC_InvokeCommand: temporary memory reference size is : %x\n", operation->params[param_count].tmpref.size);
				IMSG_DEBUG("TEEC_InvokeCommand: LINE %d the value is %s\n", __LINE__, (char *)(operation->params[param_count].tmpref.buffer));

				if (operation->params[param_count].tmpref.size == 0) {
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_API;
					retVal = TEEC_ERROR_NO_DATA;
					break;
				}

				inout = set_inout(param_types[param_count], 0);

				/* map temp share memory */
				enc.len  = operation->params[param_count].tmpref.size;
				if (enc.len == 0)
					temp_buffer[param_count] = ut_client_map_mem(session->imp->device->fd, ZERO_SIZE_MAP);
				else
					temp_buffer[param_count] = ut_client_map_mem(session->imp->device->fd, enc.len);

				if (temp_buffer[param_count] == NULL) {
					IMSG_ERROR("temp share memory failed\n");
					temp_buffer[param_count] = NULL;
					return TEEC_ERROR_OUT_OF_MEMORY;
				}

				memcpy(temp_buffer[param_count], operation->params[param_count].tmpref.buffer, enc.len);

				temp_enc_data = (unsigned long)(temp_buffer[param_count]);
				enc.data = temp_enc_data;
				IMSG_DEBUG("TEEC_InvokeCommand: LINE %d the value is %s\n", __LINE__, (char *)(enc.data));
				retVal = ut_enc_memory(session->imp->device->fd, &enc, operation->params[param_count], returnOrigin, inout, 1);

				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				}
			} /* end if for temp reference */
		} /* end for */
	} /* end paramtype */

	if (retVal != 0) {
		IMSG_ERROR("error in encoding the data \n");
		goto operation_release;
	}

	/* Invoke the command */
	retVal = ut_client_send_cmd(session->imp->device->fd, (struct teei_client_encode_cmd *)&enc);
	if (retVal != 0) {
		if (returnOrigin != NULL)
			*returnOrigin = TEEC_ORIGIN_COMMS;

		session->imp->s_errno = -(retVal);

		if (retVal == -EFAULT) {
			IMSG_DEBUG("-EFAULT\n");
			retVal = TEEC_ERROR_ACCESS_DENIED;
		}

		if (retVal == -EINVAL) {
			IMSG_DEBUG("-EINVAL\n");
			retVal = TEEC_ERROR_BAD_PARAMETERS;
		}

		IMSG_DEBUG("cmd id %d\n", enc.cmd_id);
		IMSG_DEBUG("TEEC_InvokeCommand send commend request enc.return_value: %x\n", enc.return_value);
		IMSG_ERROR("TEEC_InvokeCommand:command submission in client driver failed\n");

		goto operation_release;
	} else {
		IMSG_DEBUG("TEEC_InvokeCommand send commend request enc.return_value: %x\n", enc.return_value);
		if (returnOrigin != NULL)
			*returnOrigin = enc.return_origin;
	}

	/* Decode the data */
	IMSG_DEBUG("Decode the data\n");
	if ((operation != NULL) && (operation->paramTypes != 0)) {
		for (param_count = 0; param_count < 4; param_count++) {
			/* decode value type */
			if ((param_types[param_count] == TEEC_VALUE_INOUT) || (param_types[param_count] == TEEC_VALUE_OUTPUT)) {
				retVal = ut_dec_value(&enc, session->imp->device->fd, returnOrigin);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				} else
					operation->params[param_count].value.a = *((uint32_t *)enc.data);

				retVal = ut_dec_value(&enc, session->imp->device->fd, returnOrigin);
				if (retVal != 0) {
					session->imp->s_errno = -(retVal);
					break;
				} else
					operation->params[param_count].value.b = *((uint32_t *)enc.data);

			} else if ((param_types[param_count] == TEEC_MEMREF_WHOLE) || (param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT) || (param_types[param_count] == TEEC_MEMREF_TEMP_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_TEMP_OUTPUT)) {

				IMSG_DEBUG("[%s][%d] decode memory buffer\n", __func__, __LINE__);
				inout = 2;
				inout = set_inout(param_types[param_count], operation->params[param_count].memref.parent->flags);
				if (inout == 0)
					continue;
				retVal = ut_client_decode_array_space(session->imp->device->fd, (struct teei_client_encode_cmd *)(&enc));

				/* REGISTER MEMORY */
				if ((param_types[param_count] == TEEC_MEMREF_WHOLE) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {

					IMSG_DEBUG("[%s][%d] decode memory buffer\n", __func__, __LINE__);

					if (operation->params[param_count].memref.parent->imp->allocated == 0) {
						IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
						operation->params[param_count].memref.parent->size = enc.len;
						IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
						memcpy(operation->params[param_count].memref.parent->buffer, (const void *)enc.data, enc.len);
						IMSG_DEBUG("decode register memory buffer\n");
					}
				}

				/* TEMPORAY MEMORY */
				else {
					IMSG_DEBUG("decode temp memory buffer\n");

					operation->params[param_count].tmpref.size = enc.len;
					memcpy(operation->params[param_count].tmpref.buffer, temp_buffer[param_count], enc.len);

				}
				IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);

				if (retVal != 0) {
					if (returnOrigin != NULL)
						*returnOrigin = TEEC_ORIGIN_COMMS;
					session->imp->s_errno = -(retVal);
					IMSG_ERROR("TEEC_InvokeCommand CLIENT_IOCTL_DEC_ARRAY_SPACE: decoding data in client driver failed\n");
					break;
				}

				IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
				if ((param_types[param_count] == TEEC_MEMREF_WHOLE) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_INOUT) ||
					(param_types[param_count] == TEEC_MEMREF_PARTIAL_OUTPUT)) {

					operation->params[param_count].memref.size = enc.len;
					IMSG_DEBUG("(O)operation->params[%d].memref.size:0x%X\n", param_count, enc.len);
				} else {
					operation->params[param_count].tmpref.size = enc.len;
					IMSG_DEBUG("(O)operation->params[%d].tmpref.size:0x%X\n", param_count, enc.len);
				}
				IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
			} /* end if for TEEC_MEM_REF check */
		} /* end for */
	} /* end paramtype */
	else
		IMSG_DEBUG("else case of (operation && operation->paramTypes != 0)\n");

	IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
	if (retVal != 0)
		IMSG_ERROR("error in decoding the data\n");

operation_release:
	/* release the operation */
	IMSG_DEBUG("[%s][%d] --------------------------\n", __func__, __LINE__);
	retVal = ut_client_operation_release(session->imp->device->fd, (struct teei_client_encode_cmd *)(&enc));
	if (retVal != 0) {
		if (returnOrigin != NULL)
			*returnOrigin = TEEC_ORIGIN_COMMS;
		IMSG_ERROR("Operation release failed\n");
	}

	return enc.return_value;
}
