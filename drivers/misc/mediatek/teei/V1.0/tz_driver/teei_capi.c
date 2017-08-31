/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "teei_smc_struct.h"
#include "teei_capi.h"
#include "teei_client.h"
#include "teei_id.h"
#include "teei_debug.h"
#include "teei_common.h"
#ifdef CONFIG_ARM64
#include <linux/compat.h>
#else
static inline void __user *compat_ptr(unsigned int * uptr)
{
	return (void __user *)(unsigned long)uptr;
}
#endif

int teei_client_close_session_for_service(void *private_data, struct teei_session *temp_ses);

extern struct semaphore api_lock;
extern struct mutex pm_mutex;
extern int device_file_cnt;
extern void ut_pm_mutex_lock(struct mutex *lock);
extern void ut_pm_mutex_unlock(struct mutex *lock);

struct teei_context *teei_create_context(int dev_count)
{
	struct teei_context *cont = NULL;
	cont = kmalloc(sizeof(struct teei_context), GFP_KERNEL);

	if (cont == NULL)
		return NULL;

	cont->cont_id = dev_count;
	cont->sess_cnt = 0;
	memset(cont->tee_name, 0, TEE_NAME_SIZE);
	INIT_LIST_HEAD(&(cont->link));
	INIT_LIST_HEAD(&(cont->sess_link));
	INIT_LIST_HEAD(&(cont->shared_mem_list));

	list_add(&(cont->link), &(teei_contexts_head.context_list));
	teei_contexts_head.dev_file_cnt++;
	return cont;
}

struct teei_session *teei_create_session(struct teei_context *cont)
{
	struct teei_session *sess = NULL;
	sess = kmalloc(sizeof(struct teei_session), GFP_KERNEL);
	if (sess == NULL)
		return NULL;

	sess->sess_id = (unsigned long)sess;
	sess->parent_cont = cont;
	INIT_LIST_HEAD(&(sess->link));
	INIT_LIST_HEAD(&(sess->encode_list));
	INIT_LIST_HEAD(&(sess->shared_mem_list));

	list_add(&(sess->link), &(cont->sess_link));
	cont->sess_cnt = cont->sess_cnt + 1;

	return sess;
}

int __teei_client_context_init(unsigned long dev_file_id, struct ctx_data *ctx)
{
	struct teei_context *temp_cont = NULL;
	int dev_found = 0;
	int error_code = 0;
	int *resp_flag = NULL;
	int *name = NULL;
	int retVal = 0;

	down_write(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			dev_found = 1;
			break;
		}
	}

	up_write(&(teei_contexts_head.teei_contexts_sem));

	if (dev_found != 1)
		return -EINVAL;

	resp_flag = tz_malloc_shared_mem(4, GFP_KERNEL);
	if (resp_flag == NULL) {
		pr_err("[%s][%d] ========== No memory for resp_flag ============\n", __func__, __LINE__);
		return -ENOMEM;
	}

	name = tz_malloc_shared_mem(sizeof(ctx->name), GFP_KERNEL);
	if (name == NULL) {
		tz_free_shared_mem(resp_flag, 4);
		pr_err("[%s][%d] ========== No memory for name ============\n", __func__, __LINE__);
		return -ENOMEM;
	}

	memcpy(name, ctx->name, sizeof(ctx->name));
	Flush_Dcache_By_Area((unsigned long)name, (unsigned long)name + sizeof(ctx->name));

	memcpy(temp_cont->tee_name, ctx->name, sizeof(ctx->name));

	retVal = teei_smc_call(TEEI_CMD_TYPE_INITILIZE_CONTEXT, dev_file_id,
				0, 0, 0, 0, name, 255, resp_flag, 4, NULL,
				NULL, 0, NULL, &error_code, &(temp_cont->cont_lock));

	ctx->ctx_ret = !(*resp_flag);

	tz_free_shared_mem(resp_flag, 4);
	tz_free_shared_mem(name, sizeof(ctx->name));

	return retVal;
}

/*************************************************************************
 * Kernel client_context_init interface.
 *************************************************************************/
int ut_client_context_init(unsigned long dev_file_id, struct ctx_data *ctx)
{
        int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_context_init(dev_file_id, ctx);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_context_init interface.
 *************************************************************************/
int teei_client_context_init(void *private_data, void *argp)
{
	int retVal = 0;
	unsigned long dev_file_id = (unsigned long)private_data;
	struct ctx_data ctx;

	if (copy_from_user(&ctx, argp, sizeof(ctx))) {
		pr_err("[%s][%d] copy from user failed.\n ", __func__, __LINE__);
		return -EFAULT;
	}
	retVal = __teei_client_context_init(dev_file_id, &ctx);

	if (copy_to_user(argp, &ctx, sizeof(ctx))) {
		pr_err("[%s][%d]copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_context_close(unsigned long dev_file_id, struct ctx_data *ctx)
{
	struct teei_context *temp_cont = NULL;
	int *resp_flag = NULL;
	int error_code = 0;
	int dev_found = 0;
	int retVal = 0;

	down_write(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			dev_found = 1;
			break;
		}
	}

	up_write(&(teei_contexts_head.teei_contexts_sem));

	if (dev_found != 1)
		return -EINVAL;

	resp_flag = tz_malloc_shared_mem(4, GFP_KERNEL);
	if (resp_flag == NULL) {
		pr_err("[%s][%d] ========== No memory for resp_flag ============\n", __func__, __LINE__);
		return -ENOMEM;
	}

	retVal = teei_smc_call(TEEI_CMD_TYPE_FINALIZE_CONTEXT, dev_file_id,
				0, 0, 0, 0,
				NULL, 0, resp_flag, 4, NULL, NULL,
				0, NULL, &error_code, &(temp_cont->cont_lock));

	ctx->ctx_ret = *resp_flag;
	tz_free_shared_mem(resp_flag, 4);

	return retVal;
}

/*************************************************************************
 * Kernel client_context_close interface.
 *************************************************************************/
int ut_client_context_close(unsigned long dev_file_id, struct ctx_data *ctx)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_context_close(dev_file_id, ctx);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_context_close interface.
 *************************************************************************/
int teei_client_context_close(void *private_data, void *argp)
{
	int retVal = 0;
	unsigned long dev_file_id = (unsigned long)private_data;
	struct ctx_data ctx;

	if (copy_from_user(&ctx, argp, sizeof(ctx))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_context_close(dev_file_id, &(ctx));

	if (copy_to_user(argp, &ctx, sizeof(ctx))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_session_init(unsigned long dev_file_id, struct user_ses_init *ses_init)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *ses_new = NULL;
	int ctx_found = 0;

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}

	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (ctx_found != 1)
		return -EINVAL;

	ses_new = (struct teei_session *)tz_malloc(sizeof(struct teei_session), GFP_KERNEL);
	if (ses_new == NULL) {
		pr_err("[%s][%d] failed to malloc ses_new.\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ses_init->session_id = (unsigned long)ses_new;
	ses_new->sess_id = (unsigned long)ses_new;
	ses_new->parent_cont = temp_cont;

	INIT_LIST_HEAD(&ses_new->link);
	INIT_LIST_HEAD(&ses_new->encode_list);
	INIT_LIST_HEAD(&ses_new->shared_mem_list);
	list_add_tail(&ses_new->link, &temp_cont->sess_link);

	return 0;
}

/*************************************************************************
 * Kernel client_session_init interface.
 *************************************************************************/
int ut_client_session_init(unsigned long dev_file_id, struct user_ses_init *ses_init)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_session_init(dev_file_id, ses_init);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_session_init interface.
 *************************************************************************/
int teei_client_session_init(void *private_data, void *argp)
{
	struct user_ses_init ses_init;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	if (copy_from_user(&ses_init, argp, sizeof(ses_init))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return  -EFAULT;
	}

	retVal = __teei_client_session_init(dev_file_id, &ses_init);

	if (copy_to_user(argp, &ses_init, sizeof(ses_init))) {
		pr_err("[%s][%d] copy to user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_session_open(unsigned long dev_file_id, struct ser_ses_id *ses_open)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *ses_new = NULL;
	struct teei_encode *enc_temp = NULL;
	struct ser_ses_id *ses_open_shared = NULL;

	int ctx_found = 0;
	int sess_found = 0;
	int enc_found = 0;
	int retVal = 0;

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}

	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (ctx_found != 1)
		return -EINVAL;

	list_for_each_entry(ses_new, &temp_cont->sess_link, link) {
		if (ses_new->sess_id == ses_open->session_id) {
			sess_found = 1;
			break;
		}
	}

	if (sess_found != 1)
		return -EINVAL;

	ses_open_shared = (struct ser_ses_id *)tz_malloc_shared_mem(sizeof(struct ser_ses_id), GFP_KERNEL);
	if (ses_open_shared == NULL) {
		pr_err("[%s][%d] ========== No memory for ses_open ============\n", __func__, __LINE__);
		return -ENOMEM;
	}

	memcpy(ses_open_shared, ses_open, sizeof(struct ser_ses_id));

	list_for_each_entry(enc_temp, &ses_new->encode_list, head) {
		if (enc_temp->encode_id == ses_open->paramtype) {
			enc_found = 1;
			break;
		}
	}

	/* Invoke the smc_call */
	if (enc_found) {
		/* This session had been encoded. */
		retVal = teei_smc_call(TEEI_CMD_TYPE_OPEN_SESSION,
				dev_file_id,
				0,
				0,
				0,
				0,
				enc_temp->ker_req_data_addr,
				enc_temp->enc_req_offset,
				enc_temp->ker_res_data_addr,
				enc_temp->enc_res_offset,
				enc_temp->meta,
				ses_open_shared,
				sizeof(struct ser_ses_id),
				NULL,
				NULL,
				&(temp_cont->cont_lock));

	} else {
		/* This session didn't have been encoded */
		retVal = teei_smc_call(TEEI_CMD_TYPE_OPEN_SESSION,
				dev_file_id,
				0,
				0,
				0,
				0,
				NULL,
				0,
				NULL,
				0,
				NULL,
				ses_open_shared,
				sizeof(struct ser_ses_id),
				NULL,
				NULL,
				&(temp_cont->cont_lock));
	}

	if (retVal != SMC_SUCCESS) {
		pr_err("[%s][%d] open session smc error!\n", __func__, __LINE__);

	}

	if (ses_open_shared->session_id == -1)
		pr_err("[%s][%d] invalid session id!\n", __func__, __LINE__);

	/* Copy the result back to the user space */
	ses_new->sess_id = ses_open_shared->session_id;
	memcpy(ses_open, ses_open_shared, sizeof(struct ser_ses_id));

	tz_free_shared_mem(ses_open_shared, sizeof(struct ser_ses_id));

	return retVal;
}

/*************************************************************************
 * Kernel client_session_open interface.
 *************************************************************************/
int ut_client_session_open(unsigned long dev_file_id, struct ser_ses_id *ses_open)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_session_open(dev_file_id, ses_open);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_session_open interface.
 *************************************************************************/
int teei_client_session_open(void *private_data, void *argp)
{
	struct ser_ses_id ses_open;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	/* Get the paraments about this session from user space. */
	if (copy_from_user(&ses_open, argp, sizeof(struct ser_ses_id))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_session_open(dev_file_id, &ses_open);

	/* Copy the result back to the user space */
	if (copy_to_user(argp, &ses_open, sizeof(struct ser_ses_id))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_session_close(unsigned long dev_file_id, struct ser_ses_id *ses_close)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int retVal = -EINVAL;

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == ses_close->session_id) {
					retVal = teei_client_close_session_for_service(dev_file_id, temp_ses);
					goto out;
				}
			}
		}
	}

out:
	up_read(&(teei_contexts_head.teei_contexts_sem));

	return retVal;

}

/*************************************************************************
 * Kernel client_session_close interface.
 *************************************************************************/
int ut_client_session_close(unsigned long dev_file_id, struct ser_ses_id *ses_close)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_session_close(dev_file_id, ses_close);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_session_close interface.
 *************************************************************************/
int teei_client_session_close(void *private_data, void *argp)
{
	struct ser_ses_id ses_close;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;


	if (copy_from_user(&ses_close, argp, sizeof(ses_close))) {
		pr_err("[%s][%d] copy from user failed.\n ", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_session_close(dev_file_id, &ses_close);

	if (copy_to_user(argp, &ses_close, sizeof(ses_close))) {
		pr_err("[%s][%d] copy from user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int teei_client_prepare_encode(void *private_data,
                               struct teei_client_encode_cmd *enc,
                               struct teei_encode **penc_context,
                               struct teei_session **psession)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *enc_context = NULL;
	int session_found = 0;
	int enc_found = 0;
	int retVal = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	/* search the context session with private_data */
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == enc->session_id) {
					session_found = 1;
					break;
				}
			}
		}

		if (session_found == 1)
			break;
	}

	if (!session_found) {
		pr_err("[%s][%d] session (ID: %x) not found!\n", __func__, __LINE__, enc->session_id);
		return -EINVAL;
	}

	/*
	 * check if the enc struct had been inited.
	 */

	if (enc->encode_id != -1) {
		list_for_each_entry(enc_context, &temp_ses->encode_list, head) {
			if (enc_context->encode_id == enc->encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	/* create one command parament block */
	if (!enc_found) {
		enc_context = (struct teei_encode *)tz_malloc(sizeof(struct teei_encode), GFP_KERNEL);

		if (enc_context == NULL) {
			pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}

		enc_context->meta = tz_malloc_shared_mem(sizeof(struct teei_encode_meta) *
		                    (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS),
		                    GFP_KERNEL);

		if (enc_context->meta == NULL) {
			pr_err("[%s][%d] enc_context->meta is NULL!\n", __func__, __LINE__);
			kfree(enc_context);
			return -ENOMEM;
		}

		memset(enc_context->meta, 0, sizeof(struct teei_encode_meta) *
		       (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));

		enc_context->encode_id = (unsigned long)enc_context;
		enc_context->ker_req_data_addr = NULL;
		enc_context->ker_res_data_addr = NULL;
		enc_context->enc_req_offset = 0;
		enc_context->enc_res_offset = 0;
		enc_context->enc_req_pos = 0;
		enc_context->enc_res_pos = TEEI_MAX_REQ_PARAMS;
		enc_context->dec_res_pos = TEEI_MAX_REQ_PARAMS;
		enc_context->dec_offset = 0;
		list_add_tail(&enc_context->head, &temp_ses->encode_list);
		enc->encode_id = enc_context->encode_id;
	}

	/* return the enc_context & temp_ses */
	*penc_context = enc_context;
	*psession = temp_ses;

	return retVal;
}

int __teei_client_send_cmd(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *enc_temp = NULL;
	int ctx_found = 0;
	int sess_found = 0;
	int enc_found = 0;
	int retVal = 0;

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}

	}

	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (ctx_found == 0) {
		pr_err("[%s][%d] can't find context data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
		if (temp_ses->sess_id == enc->session_id) {
			sess_found = 1;
			break;
		}
	}

	if (sess_found == 0) {
		pr_err("[%s][%d] can't find session data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (enc->encode_id != -1) {
		list_for_each_entry(enc_temp, &temp_ses->encode_list, head) {
			if (enc_temp->encode_id == enc->encode_id) {
				enc_found = 1;
				break;
			}
		}
	} else {
		retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_temp, &temp_ses);
		if (retVal == 0)
			enc_found = 1;
	}

	if (enc_found == 0) {
		pr_err("[%s][%d] can't find encode data!\n", __func__, __LINE__);
		return -EINVAL;
	}

	retVal = teei_smc_call(TEEI_CMD_TYPE_INVOKE_COMMAND,
			dev_file_id,
			0,
			enc->cmd_id,
			enc->session_id,
			enc->encode_id,
			enc_temp->ker_req_data_addr,
			enc_temp->enc_req_offset,
			enc_temp->ker_res_data_addr,
			enc_temp->enc_res_offset,
			enc_temp->meta,
			NULL,
			0,
			&enc->return_value,
			&enc->return_origin,
			&(temp_cont->cont_lock));

	if (retVal != SMC_SUCCESS)
		pr_err("[%s][%d] send cmd secure call failed!\n", __func__, __LINE__);

	return retVal;

}

/*************************************************************************
 * Kernel client_send_cmd interface.
 *************************************************************************/
int ut_client_send_cmd(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_send_cmd(dev_file_id, enc);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_send_cmd interface.
 *************************************************************************/
int teei_client_send_cmd(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	unsigned long dev_file_id = 0;
	int retVal = 0;

	dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_send_cmd(dev_file_id, &enc);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("[%s][%d] copy to user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_operation_release(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	struct teei_encode *enc_context = NULL;
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int ctx_found = 0;
	int session_found = 0;
	int enc_found = 0;
	int retVal = 0;

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			ctx_found = 1;
			break;
		}
	}

	up_read(&(teei_contexts_head.teei_contexts_sem));

	if (ctx_found == 0) {
		pr_err("[%s][%d] ctx_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
		if (temp_ses->sess_id == enc->session_id) {
			session_found = 1;
			break;
		}
	}

	if (session_found == 0) {
		pr_err("[%s][%d] session_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (enc->encode_id != -1) {
		list_for_each_entry(enc_context, &temp_ses->encode_list, head) {
			if (enc_context->encode_id == enc->encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	if (enc_found == 0) {
		pr_err("[%s][%d] enc_found failed!\n", __func__, __LINE__);
		return -EINVAL;
	} else {
		if (enc_context->ker_req_data_addr)
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);

		if (enc_context->ker_res_data_addr)
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);

		list_del(&enc_context->head);
		/* kfree(enc_context->meta); */
		tz_free_shared_mem(enc_context->meta, sizeof(struct teei_encode_meta) *
				(TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS));
		kfree(enc_context);
	}

	return 0;
}

/*************************************************************************
 * Kernel client_operation_release interface.
 *************************************************************************/
int ut_client_operation_release(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_operation_release(dev_file_id, enc);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;


}


/*************************************************************************
 * Native client_operation_release interface.
 *************************************************************************/
int teei_client_operation_release(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_operation_release(dev_file_id, &enc);

	return retVal;
}

int __teei_client_encode_uint32(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	struct teei_session *session = NULL;
	struct teei_encode *enc_context = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);
	if (retVal != 0) {
		pr_err("[%s][%d]  failed!\n", __func__, __LINE__);
		return retVal;
	}

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (enc_context->ker_req_data_addr == NULL) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (enc_context->ker_req_data_addr == NULL) {
				pr_err("[%s][%d] enc_context->ker_req_data_addr is NULL!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			u64 addr = enc->data;
			u32 value = 0;

			if (kernel_range == 0) {
				void __user *pt = compat_ptr((unsigned int *)addr);
				copy_from_user(&value, pt, 4);
			} else {
				memcpy(&value, addr, 4);
			}

			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset) = value;
			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_req_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].value_flag = enc->value_flag;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;
			enc_context->enc_req_pos++;
		} else {
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] enc_context->ker_res_data_addr is NULL\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc->data != NULL) {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = (unsigned int)enc->data;
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_res_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].value_flag = enc->value_flag;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;
			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			return  -ENOMEM;
		}
	}

	return 0;
}

/*************************************************************************
 * Native client_encode_uint32 interface.
 *************************************************************************/
int ut_client_encode_uint32(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_uint32(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_encode_uint43 interface.
 *************************************************************************/
int teei_client_encode_uint32(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_uint32((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("copy from user failed\n");
		retVal = -EFAULT;
	}

	return retVal;
}

int __teei_client_encode_uint32_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	struct teei_session *session = NULL;
	struct teei_encode *enc_context = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);
	if (retVal != 0) {
		pr_err("[%s][%d] failed!\n", __func__, __LINE__);
		return retVal;
	}

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (enc_context->ker_req_data_addr == NULL) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (enc_context->ker_req_data_addr == NULL) {
				pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
		   (enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			u64 addr = enc->data;
			u32 value = 0;
			if (kernel_range == 0) {
				void __user *pt = compat_ptr((unsigned int *)addr);
				copy_from_user(&value, pt, 4);

			} else
				memcpy(&value, addr, 4);
			//chengxin modify if user space is 64 bit, enc.data is a 64 bit addr, do not change to 32 bit
			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset) = *(u32 *)enc->data;
			//*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset) = value;

			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_req_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].value_flag = enc->value_flag;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;
			enc_context->enc_req_pos++;
		} else {
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] tz_malloc failed\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
		    (enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc->data != NULL) {
				u64 addr = enc->data;
				void __user *pt = compat_ptr((unsigned int *)addr);
				/*
				u32 value = 0;
				copy_from_user(&value, pt, 4);
				TZDebug("value %x", value);
				*/
				enc_context->meta[enc_context->enc_res_pos].usr_addr = enc->data;//this data should not change to 32bit in 64api system
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_UINT32;
			enc_context->meta[enc_context->enc_res_pos].len = sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].value_flag = enc->value_flag;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;
			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	}

	return 0;
}

/*************************************************************************
 * Kernel client_encode_uint32_64bit interface.
 *************************************************************************/
int ut_client_encode_uint32_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_uint32_64bit(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

/*************************************************************************
 * Native client_encode_uint32_64bit interface.
 *************************************************************************/
int teei_client_encode_uint32_64bit(void *private_data, void *argp)
{

	struct teei_client_encode_cmd enc;
	int retVal = 0;


	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d]copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_uint32_64bit((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("copy from user failed!\n");
		retVal = -EFAULT;
	}

	return retVal;
}

int __teei_client_encode_array(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);
	if (retVal != 0)
		return retVal;

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			/* pr_err("[%s][%d] allocate req data data.\n", __func__, __LINE__); */
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_req_data_addr) {
				pr_err("[%s][%d] enc_context->ker_req_data_addr is NULL!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + enc->len <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {

			if (kernel_range == 0) {
				if (copy_from_user((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset,
						(unsigned int)enc->data , enc->len)) {
					pr_err("[%s][%d] copy from user failed.\n", __func__, __LINE__);
					return -EFAULT;
				}
			} else {
				memcpy((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset, (char *)enc->data , enc->len);
			}
			enc_context->enc_req_offset += enc->len;

			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_req_pos].len = enc->len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_req_pos++;
		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (NULL == enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] enc_context->ker_res_data_addr is NULL!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + enc->len <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos <
				 (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc->data != NULL) {
				enc_context->meta[enc_context->enc_res_pos].usr_addr
					= (unsigned int)enc->data;
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += enc->len;
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_res_pos].len = enc->len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	}

	return 0;
}

int ut_client_encode_array(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_array(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}


int teei_client_encode_array(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_array((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_encode_array_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);
	if (retVal != 0)
		return -EFAULT;

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			/* pr_err("[%s][%d] allocate req data data.\n", __func__, __LINE__); */
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_req_data_addr) {
				pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + enc->len <= TEEI_1K_SIZE) &&
		   (enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			if (kernel_range == 0) {
				if (copy_from_user((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset,
			       				enc->data , enc->len)) {
					pr_err("[%s][%d] copy from user failed.\n", __func__, __LINE__);
					return -EFAULT;
				}
			} else {
				memcpy((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset, (char *)enc->data, enc->len);
			}
			enc_context->enc_req_offset += enc->len;

			enc_context->meta[enc_context->enc_req_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_req_pos].len = enc->len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_req_pos++;
		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (NULL == enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + enc->len <= TEEI_1K_SIZE) &&
		   (enc_context->enc_res_pos <
		    (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			if (enc->data != NULL) {
				enc_context->meta[enc_context->enc_res_pos].usr_addr
				    = enc->data;
			} else {
				enc_context->meta[enc_context->enc_res_pos].usr_addr = 0;
			}
			enc_context->enc_res_offset += enc->len;
			enc_context->meta[enc_context->enc_res_pos].type = TEEI_ENC_ARRAY;
			enc_context->meta[enc_context->enc_res_pos].len = enc->len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_res_pos++;
		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			return -ENOMEM;
		}
	}

	return 0;
}

int ut_client_encode_array_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_array_64bit(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_encode_array_64bit(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_array_64bit((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_encode_mem_ref(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;
	struct teei_shared_mem *temp_shared_mem = NULL;
	int shared_mem_found = 0;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);

	if (retVal != 0)
		return retVal;

	list_for_each_entry(temp_shared_mem, &session->shared_mem_list, s_head) {
		u64 addr = enc->data;
		if (temp_shared_mem && temp_shared_mem->index == (unsigned int)addr) {
			pr_err("[%s][%d] found the shared mem\n", __func__, __LINE__);
			shared_mem_found = 1;
			break;
		}
	}

	pr_err("[%s][%d] enc->data = %lx\n", __func__, __LINE__, enc->data);
	pr_err("[%s][%d] enc->data = %x\n", __func__, __LINE__, (u32)enc->data);
	pr_err("[%s][%d] dev_file_id = %ld\n", __func__, __LINE__, dev_file_id);
	if (shared_mem_found == 0) {
		struct teei_context *temp_cont = NULL;
		list_for_each_entry(temp_cont,
				&teei_contexts_head.context_list,
				link) {
			pr_err("[%s][%d] cont_id = %ld\n", __func__, __LINE__, temp_cont->cont_id);
			if (temp_cont->cont_id == (unsigned long)dev_file_id) {
				list_for_each_entry(temp_shared_mem,
						&temp_cont->shared_mem_list,
						head) {
					pr_err("[%s][%d] temp_shared_mem->index = %x\n", __func__, __LINE__, temp_shared_mem->index);
					if (temp_shared_mem->index == (u32)enc->data) {
						shared_mem_found = 1;
						break;
					}
				}

				break;
			}
			if (shared_mem_found == 1)
				break;
		}
	}

	if (!shared_mem_found) {
		pr_err("shared_mem_found is 0\n");
		return -EINVAL;
	}

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_req_data_addr) {
				pr_err("[%s][%d] enc_context->ker_req_data_addr is NULL!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset)
				= virt_to_phys((char *)temp_shared_mem->k_addr + enc->offset);

			Flush_Dcache_By_Area((unsigned long)(temp_shared_mem->k_addr + enc->offset),
					(unsigned long)temp_shared_mem->k_addr + enc->offset + enc->len);
			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].usr_addr
				= (unsigned long)((char *)temp_shared_mem->u_addr + enc->offset);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_req_pos].len = enc->len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_req_pos++;

		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			pr_err("[%s][%d] no space left!\n");
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] enc_context->ker_res_data_addr is NULL!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
				(enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			*(u32 *)((char *)enc_context->ker_res_data_addr +
					enc_context->enc_res_offset)
				= virt_to_phys((char *)temp_shared_mem->k_addr + enc->offset);

			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].usr_addr
				= (unsigned long)((char *)temp_shared_mem->u_addr + enc->offset);
			enc_context->meta[enc_context->enc_res_pos].type
				=  TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_res_pos].len = enc->len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_res_pos++;


		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			pr_err("[%s][%d] no space left!\n");
			return -ENOMEM;
		}
	}

	return 0;
}

int ut_client_encode_mem_ref(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_mem_ref(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;

}

int teei_client_encode_mem_ref(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;

	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_mem_ref((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_encode_mem_ref_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc, int kernel_range)
{
	int shared_mem_found = 0;
	struct teei_encode *enc_context = NULL;
	struct teei_session *session = NULL;
	struct teei_shared_mem *temp_shared_mem = NULL;
	unsigned int temp_addr;
	int retVal = 0;

	retVal = teei_client_prepare_encode(dev_file_id, enc, &enc_context, &session);
	if (retVal != 0)
		return retVal;

	list_for_each_entry(temp_shared_mem, &session->shared_mem_list, s_head) {
		u64 addr = enc->data;
		if (temp_shared_mem && temp_shared_mem->index == enc->data) {
			shared_mem_found = 1;
			break;
		}
	}
	if (shared_mem_found == 0) {
		struct teei_context *temp_cont = NULL;
		list_for_each_entry(temp_cont,
				&teei_contexts_head.context_list,
				link) {
			if (temp_cont->cont_id == (unsigned long)dev_file_id) {
				list_for_each_entry(temp_shared_mem,
						&temp_cont->shared_mem_list,
						head) {
					if (((temp_shared_mem->index)) == enc->data) {
						shared_mem_found = 1;
						break;
					}
				}
			}
			if (shared_mem_found == 1)
				break;
		}
	}
	if (!shared_mem_found) {
		pr_err("[%s][%d]shared_mem_found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (enc->param_type == TEEIC_PARAM_IN) {
		if (NULL == enc_context->ker_req_data_addr) {
			enc_context->ker_req_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (NULL == enc_context->ker_req_data_addr) {
				pr_err("[%s][%d]tz_malloc failed!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}

		if ((enc_context->enc_req_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
		   (enc_context->enc_req_pos < TEEI_MAX_REQ_PARAMS)) {
			*(u32 *)((char *)enc_context->ker_req_data_addr + enc_context->enc_req_offset)
			    = virt_to_phys((char *)temp_shared_mem->k_addr + enc->offset);

			Flush_Dcache_By_Area((unsigned long)(temp_shared_mem->k_addr + enc->offset),
						(unsigned long)temp_shared_mem->k_addr + enc->offset + enc->len);
			enc_context->enc_req_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_req_pos].usr_addr
			    = (unsigned long)((char *)temp_shared_mem->u_addr + enc->offset);
			enc_context->meta[enc_context->enc_req_pos].type = TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_req_pos].len = enc->len;
			enc_context->meta[enc_context->enc_req_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_req_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_req_pos++;

		} else {
			/* kfree(enc_context->ker_req_data_addr); */
			tz_free_shared_mem(enc_context->ker_req_data_addr, TEEI_1K_SIZE);
			pr_err("[%s][%d]failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}
	} else if (enc->param_type == TEEIC_PARAM_OUT) {
		if (!enc_context->ker_res_data_addr) {
			enc_context->ker_res_data_addr = tz_malloc_shared_mem(TEEI_1K_SIZE, GFP_KERNEL);
			if (!enc_context->ker_res_data_addr) {
				pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
				return -ENOMEM;
			}
		}
		if ((enc_context->enc_res_offset + sizeof(u32) <= TEEI_1K_SIZE) &&
		   (enc_context->enc_res_pos < (TEEI_MAX_RES_PARAMS + TEEI_MAX_REQ_PARAMS))) {
			*(u32 *)((char *)enc_context->ker_res_data_addr +
					enc_context->enc_res_offset)
			    = virt_to_phys((char *)temp_shared_mem->k_addr + enc->offset);

			enc_context->enc_res_offset += sizeof(u32);
			enc_context->meta[enc_context->enc_res_pos].usr_addr
			    = (unsigned long)((char *)temp_shared_mem->u_addr + enc->offset);
			enc_context->meta[enc_context->enc_res_pos].type
			    =  TEEI_MEM_REF;
			enc_context->meta[enc_context->enc_res_pos].len = enc->len;
			enc_context->meta[enc_context->enc_res_pos].param_pos = enc->param_pos;
			enc_context->meta[enc_context->enc_res_pos].param_pos_type = enc->param_pos_type;

			enc_context->enc_res_pos++;


		} else {
			/* kfree(enc_context->ker_res_data_addr); */
			tz_free_shared_mem(enc_context->ker_res_data_addr, TEEI_1K_SIZE);
			pr_err("[%s][%d] failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}
	}

	return 0;
}

int ut_client_encode_mem_ref_64bit(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_encode_mem_ref_64bit(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_encode_mem_ref_64bit(void *private_data, void *argp)
{
	struct teei_client_encode_cmd enc;
	int retVal = 0;


	if (copy_from_user(&enc, argp, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_encode_mem_ref_64bit((unsigned long)private_data, &enc, 0);

	if (copy_to_user(argp, &enc, sizeof(enc))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

static int print_context(void)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_sess = NULL;
	struct teei_encode *dec_context = NULL;

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		pr_debug("[%s][%d] context id [%lx]\n", __func__, __LINE__, temp_cont->cont_id);
		list_for_each_entry(temp_sess, &temp_cont->sess_link, link) {
			pr_debug("[%s][%d] session id [%x]\n", __func__, __LINE__, temp_sess->sess_id);
			list_for_each_entry(dec_context, &temp_sess->encode_list, head) {
				pr_debug("[%s][%d] encode_id [%x]\n", __func__, __LINE__, dec_context->encode_id);
			}
		}
	}
	return 0;
}


int teei_client_prepare_decode(void *private_data,
                               struct teei_client_encode_cmd *dec,
                               struct teei_encode **pdec_context)
{
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_encode *dec_context = NULL;
	int session_found = 0;
	int enc_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == dec->session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (0 == session_found) {
		pr_err("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (dec->encode_id != -1) {
		list_for_each_entry(dec_context, &temp_ses->encode_list, head) {
			if (dec_context->encode_id == dec->encode_id) {
				enc_found = 1;
				break;
			}
		}
	}

	/* print_context(); */
	if (0 == enc_found) {
		pr_err("[%s][%d] encode[%x] not found!\n", __func__, __LINE__, dec->encode_id);
		return -EINVAL;
	}

	*pdec_context = dec_context;

	return 0;
}

int __teei_client_decode_uint32(unsigned long dev_file_id, struct teei_client_encode_cmd *dec, int kernel_range)
{
	struct teei_encode *dec_context = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_decode(dev_file_id, dec, &dec_context);

	if (retVal != 0) {
		pr_err("[%s][%d] teei_client_prepare_decode failed!\n", __func__, __LINE__);
		return retVal;
	}

	if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
	   (dec_context->meta[dec_context->dec_res_pos].type == TEEI_ENC_UINT32)) {
		if (dec_context->meta[dec_context->dec_res_pos].usr_addr) {
			dec->data = (void*)((uint64_t)(dec_context->meta[dec_context->dec_res_pos].usr_addr));
		}

		unsigned int value1 = 0;
		u64 addr = 0;
		void __user *pt = NULL;

		if (((u32 *)dec->data) == NULL) {
			pr_err("[%s][%d] error decode dec.data addr11111 is NULL!\n", __func__, __LINE__);
		}
		if(((u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset) == NULL)) {
			pr_err("[%s][%d] decode data decode addr11111 is NULL!\n", __func__, __LINE__);
		} else {
			value1 = *((u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset));
			if (kernel_range == 0) {
				addr = dec->data;
				copy_to_user(addr, &value1, 4);
			} else
				*(unsigned int *)dec->data = value1;
		}

		dec_context->dec_offset += sizeof(u32);
		dec_context->dec_res_pos++;
	}

	return 0;
}

int ut_client_decode_uint32(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_decode_uint32(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_decode_uint32(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;


	if (copy_from_user(&dec, argp, sizeof(dec))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_decode_uint32((unsigned long)private_data, &dec, 0);

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		pr_err("[%s][%d] copy to user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_decode_array_space(unsigned long dev_file_id, struct teei_client_encode_cmd *dec, int kernel_range)
{
	struct teei_encode *dec_context = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_decode(dev_file_id, dec, &dec_context);
	if (retVal != 0)
		return retVal;

	if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
			(dec_context->meta[dec_context->dec_res_pos].type
			 == TEEI_ENC_ARRAY)) {
		if (dec_context->meta[dec_context->dec_res_pos].len >=
				dec_context->meta[dec_context->dec_res_pos].ret_len) {
			if (dec_context->meta[dec_context->dec_res_pos].usr_addr)
				dec->data = (void *)dec_context->meta[dec_context->dec_res_pos].usr_addr;

			if (kernel_range == 0) {
				if (copy_to_user(dec->data, (char *)dec_context->ker_res_data_addr + dec_context->dec_offset,
						dec_context->meta[dec_context->dec_res_pos].ret_len)) {
					pr_err("[%s][%d] copy from user failed while copying array!\n", __func__, __LINE__);
					return -EFAULT;
				}
			} else {
				memcpy(dec->data, (char *)dec_context->ker_res_data_addr + dec_context->dec_offset,
						dec_context->meta[dec_context->dec_res_pos].ret_len);
			}
		} else {

			pr_err("[%s][%d] buffer length is small. Length required %x and supplied length %x,pos %x ",
					__func__, __LINE__,
					dec_context->meta[dec_context->dec_res_pos].ret_len,
					dec_context->meta[dec_context->dec_res_pos].len,
					dec_context->dec_res_pos);

			return -EFAULT;
		}

		dec->len = dec_context->meta[dec_context->dec_res_pos].ret_len;
		dec_context->dec_offset += dec_context->meta[dec_context->dec_res_pos].len;
		dec_context->dec_res_pos++;
	} else if ((dec_context->dec_res_pos <= dec_context->enc_res_pos) &&
			(dec_context->meta[dec_context->dec_res_pos].type == TEEI_MEM_REF)) {
		if (dec_context->meta[dec_context->dec_res_pos].len >=
				dec_context->meta[dec_context->dec_res_pos].ret_len) {
			dec->data = (void *)dec_context->meta[dec_context->dec_res_pos].usr_addr;
			unsigned long pmem = *(u32 *)((char *)dec_context->ker_res_data_addr + dec_context->dec_offset);
			char *mem = NULL;
			unsigned long addr = (unsigned long)phys_to_virt(pmem);
			mem = (char *)addr;
			Invalidate_Dcache_By_Area((unsigned long)mem,
					(unsigned long)mem+dec_context->meta[dec_context->dec_res_pos].ret_len);
		} else {

			pr_err("[%s][%d] buffer length is small. Length required %x and supplied length %x",
					__func__, __LINE__,
					dec_context->meta[dec_context->dec_res_pos].ret_len,
					dec_context->meta[dec_context->dec_res_pos].len);

			return -EFAULT;
		}

		dec->len = dec_context->meta[dec_context->dec_res_pos].ret_len;
		dec_context->dec_offset += sizeof(u32);
		dec_context->dec_res_pos++;
	}

	else {
		pr_err("[%s][%d] invalid data type or decoder at wrong position!\n", __func__, __LINE__);
		return -EINVAL;
	}

	return 0;
}

int ut_client_decode_array_space(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_decode_array_space(dev_file_id, enc, 1);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_decode_array_space(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;

	if (copy_from_user(&dec, argp, sizeof(dec))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_decode_array_space((unsigned long)private_data, &dec, 0);

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int __teei_client_get_decode_type(unsigned long dev_file_id, struct teei_client_encode_cmd *dec)
{
	struct teei_encode *dec_context = NULL;
	int retVal = 0;

	retVal = teei_client_prepare_decode(dev_file_id, dec, &dec_context);
	if (retVal != 0)
		return retVal;

	if (dec_context->dec_res_pos <= dec_context->enc_res_pos)
		dec->data = (unsigned long)dec_context->meta[dec_context->dec_res_pos].type;
	else
		return -EINVAL;

	return 0;
}

int ut_client_get_decode_type(unsigned long dev_file_id, struct teei_client_encode_cmd *enc)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_get_decode_type(dev_file_id, enc);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_get_decode_type(void *private_data, void *argp)
{
	struct teei_client_encode_cmd dec;
	int retVal = 0;

	if (copy_from_user(&dec, argp, sizeof(dec))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_get_decode_type((unsigned long)private_data, &dec);

	if (copy_to_user(argp, &dec, sizeof(dec))) {
		pr_err("[%s][%d] copy to user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	return retVal;
}

int teei_client_shared_mem_alloc(void *private_data, void *argp)
{
#if 0
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_session_shared_mem_info mem_info;

	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	int  session_found = 0;
	unsigned long dev_file_id = (unsigned long)private_data;

	if (copy_from_user(&mem_info, argp, sizeof(mem_info))) {
		pr_err("[%s][%d] copy from user failed!\n", __func__, __LINE__);
		return -EFAULT;
	}

	/*
	pr_debug("[%s][%d] service id 0x%lx session id 0x%lx user mem addr 0x%p.\n",
	                __func__, __LINE__,
	                mem_info.service_id,
	                mem_info.session_id,
	                mem_info.user_mem_addr);
	*/
	list_for_each_entry(temp_cont,
	                    &teei_contexts_head.context_list,
	                    link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				if (temp_ses->sess_id == mem_info.session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (session_found == 0) {
		pr_err("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_shared_mem, &temp_cont->shared_mem_list, head) {
		if (temp_shared_mem->index == (void *)mem_info.user_mem_addr) {
			list_del(&temp_shared_mem->head);
			temp_cont->shared_mem_cnt--;
			list_add_tail(&temp_shared_mem->s_head, &temp_ses->shared_mem_list);
			break;
		}
	}
#endif

	return 0;
}

int __teei_client_shared_mem_free(unsigned long dev_file_id, struct teei_session_shared_mem_info *mem_info)
{
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_context *temp_cont = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_shared_mem *temp_pos = NULL;
	int session_found = 0;
	int retVal = 0;

	list_for_each_entry(temp_cont,
			&teei_contexts_head.context_list,
			link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry_safe(temp_shared_mem,
					temp_pos,
					&temp_cont->shared_mem_list,
					head) {
				if (temp_shared_mem && temp_shared_mem->u_addr == mem_info->user_mem_addr) {
					list_del(&temp_shared_mem->head);
					if (temp_shared_mem->k_addr)
						free_pages((u64)temp_shared_mem->k_addr,
								get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));
					kfree(temp_shared_mem);
				}
			}
		}
	}

#if 0
	list_for_each_entry(temp_cont, &teei_contexts_head.context_list, link) {
		if (temp_cont->cont_id == dev_file_id) {
			list_for_each_entry(temp_ses, &temp_cont->sess_link, link) {
				pr_debug("list:session id %x.\n", temp_ses->sess_id);
				if (temp_ses->sess_id == mem_info.session_id) {
					session_found = 1;
					break;
				}
			}
			break;
		}
	}

	if (session_found == 0) {
		pr_err("[%s][%d] session not found!\n", __func__, __LINE__);
		return -EINVAL;
	}

	list_for_each_entry(temp_shared_mem, &temp_ses->shared_mem_list, s_head) {
		if (temp_shared_mem && temp_shared_mem->index == (void *)mem_info.user_mem_addr) {
			list_del(&temp_shared_mem->s_head);

			if (temp_shared_mem->k_addr)
				free_pages((unsigned long)temp_shared_mem->k_addr,
						get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));

			kfree(temp_shared_mem);
			break;
		}
	}
#endif
	return 0;
}

int ut_client_shared_mem_free(unsigned long dev_file_id, struct teei_session_shared_mem_info *mem_info)
{
	int retVal = 0;

	down(&api_lock);
	ut_pm_mutex_lock(&pm_mutex);

	retVal = __teei_client_shared_mem_free(dev_file_id, mem_info);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&api_lock);

	return retVal;
}

int teei_client_shared_mem_free(void *private_data, void *argp)
{
	struct teei_session_shared_mem_info mem_info;
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	if (copy_from_user(&mem_info, argp, sizeof(mem_info))) {
		pr_err("[%s][%d] copy from user failed.\n", __func__, __LINE__);
		return -EFAULT;
	}

	retVal = __teei_client_shared_mem_free(dev_file_id, &mem_info);

	return retVal;
}

int __teei_client_close_session_for_service_plus(unsigned long dev_file_id, struct teei_session *temp_ses)
{
	struct teei_context *curr_cont = NULL;
	struct teei_encode *temp_encode = NULL;
	struct teei_encode *enc_context = NULL;
	struct teei_shared_mem *shared_mem = NULL;
	struct teei_shared_mem *temp_shared = NULL;

	if (temp_ses == NULL)
		return -EINVAL;

	curr_cont = temp_ses->parent_cont;

	if (!list_empty(&temp_ses->encode_list)) {
		list_for_each_entry_safe(enc_context,
				temp_encode,
				&temp_ses->encode_list,
				head) {
			if (enc_context) {
				list_del(&enc_context->head);
				kfree(enc_context);
			}
		}
	}

	if (!list_empty(&temp_ses->shared_mem_list)) {
		list_for_each_entry_safe(shared_mem,
				temp_shared,
				&temp_ses->shared_mem_list,
				s_head) {
			list_del(&shared_mem->s_head);

			if (shared_mem->k_addr) {
				free_pages((unsigned long)shared_mem->k_addr,
						get_order(ROUND_UP(shared_mem->len, SZ_4K)));
			}
			kfree(shared_mem);
		}
	}

	list_del(&temp_ses->link);
	curr_cont->sess_cnt = curr_cont->sess_cnt - 1;
	kfree(temp_ses);

	return 0;
}

static int teei_client_close_session_for_service_plus(
		void *private_data,
		struct teei_session *temp_ses)
{
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	retVal = __teei_client_close_session_for_service_plus(dev_file_id, temp_ses);

	return retVal;
}

int __teei_client_close_session_for_service(unsigned long dev_file_id, struct teei_session *temp_ses)
{
	struct ser_ses_id *ses_close = NULL;
	struct teei_context *curr_cont = NULL;
	struct teei_encode *temp_encode = NULL;
	struct teei_encode *enc_context = NULL;
	struct teei_shared_mem *shared_mem = NULL;
	struct teei_shared_mem *temp_shared = NULL;
	int *res = NULL;

	int retVal = 0;
	int error_code = 0;

	if (temp_ses == NULL) {
		pr_err("[%s][%d] ======== temp_ses is NULL =========\n", __func__, __LINE__);
		return -EINVAL;
	}

	ses_close = (struct ser_ses_id *)tz_malloc_shared_mem(sizeof(struct ser_ses_id), GFP_KERNEL);
	if (ses_close == NULL) {
		pr_err("[%s][%d] ======== ses_close is NULL =========\n", __func__, __LINE__);
		return -ENOMEM;
	}

	res = (int *)tz_malloc_shared_mem(4, GFP_KERNEL);
	if (res == NULL) {
		pr_err("[%s][%d] ======== res is NULL =========\n", __func__, __LINE__);
		tz_free_shared_mem(ses_close, sizeof(struct ser_ses_id));
		return -ENOMEM;
	}

	ses_close->session_id = temp_ses->sess_id;
	curr_cont = temp_ses->parent_cont;

	retVal = teei_smc_call(TEEI_CMD_TYPE_CLOSE_SESSION,
			dev_file_id,
			0,
			TEEI_GLOBAL_CMD_ID_CLOSE_SESSION,
			0,
			0,
			ses_close,
			sizeof(struct ser_ses_id),
			res,
			sizeof(int),
			NULL,
			NULL,
			0,
			NULL,
			&error_code,
			&(curr_cont->cont_lock));

	if (!list_empty(&temp_ses->encode_list)) {

		list_for_each_entry_safe(enc_context,
				temp_encode,
				&temp_ses->encode_list,
				head) {
			if (enc_context) {
				list_del(&enc_context->head);
				kfree(enc_context);
			}
		}
	}

	if (!list_empty(&temp_ses->shared_mem_list)) {

		list_for_each_entry_safe(shared_mem,
				temp_shared,
				&temp_ses->shared_mem_list,
				s_head) {
			list_del(&shared_mem->s_head);

			if (shared_mem->k_addr)
				free_pages((unsigned long)shared_mem->k_addr,
						get_order(ROUND_UP(shared_mem->len, SZ_4K)));

			kfree(shared_mem);
		}
	}

	list_del(&temp_ses->link);
	curr_cont->sess_cnt = curr_cont->sess_cnt - 1;

	kfree(temp_ses);
	tz_free_shared_mem(res, 4);
	tz_free_shared_mem(ses_close, sizeof(struct ser_ses_id));

	return retVal;
}

int teei_client_close_session_for_service(void *private_data, struct teei_session *temp_ses)
{
	unsigned long dev_file_id = (unsigned long)private_data;
	int retVal = 0;

	retVal = __teei_client_close_session_for_service(dev_file_id, temp_ses);

	return retVal;
}


int teei_client_service_exit(void *private_data)
{
	struct teei_shared_mem *temp_shared_mem = NULL;
	struct teei_shared_mem *temp_pos = NULL;
	struct teei_context *temp_context = NULL;
	struct teei_context *temp_context_pos = NULL;
	struct teei_session *temp_ses = NULL;
	struct teei_session *temp_ses_pos = NULL;
	unsigned long dev_file_id = 0;

	dev_file_id = (unsigned long)(private_data);
	down_write(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry_safe(temp_context,
	                         temp_context_pos,
	                         &teei_contexts_head.context_list,
	                         link) {
		if (temp_context->cont_id == dev_file_id) {

			list_for_each_entry_safe(temp_shared_mem,
			                         temp_pos,
			                         &temp_context->shared_mem_list,
			                         head) {
				if (temp_shared_mem) {
					list_del(&(temp_shared_mem->head));

					if (temp_shared_mem->k_addr) {
						free_pages((unsigned long)temp_shared_mem->k_addr,
						           get_order(ROUND_UP(temp_shared_mem->len, SZ_4K)));
					}

					kfree(temp_shared_mem);
				}
			}

			if (!list_empty(&temp_context->sess_link)) {
				list_for_each_entry_safe(temp_ses,
				                         temp_ses_pos,
				                         &temp_context->sess_link,
				                         link)
				teei_client_close_session_for_service_plus(private_data, temp_ses);
			}

			list_del(&temp_context->link);
			kfree(temp_context);
			up_write(&(teei_contexts_head.teei_contexts_sem));
			return 0;
		}
	}

	return -EINVAL;
}

long __teei_client_open_dev(void)
{
	struct teei_context *new_context = NULL;

	device_file_cnt++;

	new_context = (struct teei_context *)tz_malloc(sizeof(struct teei_context), GFP_KERNEL);
	if (new_context == NULL) {
		pr_err("tz_malloc failed for new dev file allocation!\n");
		return -ENOMEM;
	}

	new_context->cont_id = device_file_cnt;
	INIT_LIST_HEAD(&(new_context->sess_link));
	INIT_LIST_HEAD(&(new_context->link));

	new_context->shared_mem_cnt = 0;
	INIT_LIST_HEAD(&(new_context->shared_mem_list));

	sema_init(&(new_context->cont_lock), 0);

	down_write(&(teei_contexts_head.teei_contexts_sem));
	list_add(&(new_context->link), &(teei_contexts_head.context_list));
	teei_contexts_head.dev_file_cnt++;
	up_write(&(teei_contexts_head.teei_contexts_sem));

	return device_file_cnt;
}

long ut_client_open_dev(void)
{
	return __teei_client_open_dev();
}
EXPORT_SYMBOL(ut_client_open_dev);

void *__teei_client_map_mem(int dev_file_id, unsigned long size, unsigned long user_addr)
{
	void *alloc_addr = NULL;
	struct teei_shared_mem *share_mem_entry = NULL;
	int context_found = 0;
	struct teei_context *cont = NULL;

	pr_debug("[%s][%d] dev_file_id is %d\n", __func__, __LINE__, dev_file_id);

	down_read(&(teei_contexts_head.teei_contexts_sem));

	list_for_each_entry(cont, &(teei_contexts_head.context_list), link) {
		if (cont->cont_id == (unsigned long)dev_file_id) {
			context_found = 1;
			break;
		}

	}

	if (context_found == 0) {
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return NULL;
	}

	/* Alloc one teei_share_mem structure */
	share_mem_entry = tz_malloc(sizeof(struct teei_shared_mem), GFP_KERNEL);
	if (share_mem_entry == NULL) {
		pr_err("[%s][%d] tz_malloc failed!\n", __func__, __LINE__);
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return NULL;
	}

	/* Get free pages from Kernel. */
#ifdef UT_DMA_ZONE
	alloc_addr =  (void *)__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(ROUND_UP(size, SZ_4K)));
#else
	alloc_addr =  (void *)__get_free_pages(GFP_KERNEL, get_order(ROUND_UP(size, SZ_4K)));
#endif
	if (alloc_addr == NULL) {
		pr_err("[%s][%d] get free pages failed!\n", __func__, __LINE__);
		kfree(share_mem_entry);
		up_read(&(teei_contexts_head.teei_contexts_sem));
		return NULL;
	}

	pr_debug("[%s][%d] alloc_addr = %p\n", __func__, __LINE__, alloc_addr);

	/* Add the teei_share_mem into the teei_context struct */
	share_mem_entry->k_addr = (void *)alloc_addr;
	share_mem_entry->len = size;

	if (user_addr != NULL)
		share_mem_entry->u_addr = (void *)user_addr;
	else
		share_mem_entry->u_addr = (void *)alloc_addr;

	share_mem_entry->index = share_mem_entry->u_addr;
	pr_debug("[%s][%d] share_mem_entry->index = %p\n", __func__, __LINE__, share_mem_entry->index);

	cont->shared_mem_cnt++;
	list_add_tail(&(share_mem_entry->head), &(cont->shared_mem_list));

	up_read(&(teei_contexts_head.teei_contexts_sem));

	return alloc_addr;
}

void *ut_client_map_mem(int dev_file_id, unsigned long size)
{
	return __teei_client_map_mem(dev_file_id, size, NULL);
}

int ut_client_release(long dev_file_id)
{
	int retVal = 0;

	retVal = teei_client_service_exit((void *)dev_file_id);

	return retVal;
}

