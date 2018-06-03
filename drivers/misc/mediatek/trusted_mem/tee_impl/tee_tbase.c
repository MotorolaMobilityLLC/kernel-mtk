/*
 * Copyright (C) 2018 MediaTek Inc.
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

#define PR_FMT_HEADER_MUST_BE_INCLUDED_BEFORE_ALL_HDRS
#include "private/tmem_pr_fmt.h" PR_FMT_HEADER_MUST_BE_INCLUDED_BEFORE_ALL_HDRS

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "private/mld_helper.h"
#include "private/tmem_device.h"
#include "private/tmem_error.h"
#include "private/tmem_utils.h"
#include "tee_impl/tee_priv.h"
#include "tee_impl/tee_common.h"
#include "tee_impl/tee_tbase_def.h"
#include "mobicore_driver_api.h"

/* clang-format off */
#define SECMEM_TL_LEGACY_UUID \
	{0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
/* clang-format on */

struct TBASE_LEGACY_SESSION_DATA {
	struct mc_session_handle session;
	struct secmem_tci_msg_t *tci_msg_data;
	uint32_t device_id;
	struct mutex lock;
};

#define LOCK_BY_CALLEE (0)
#if LOCK_BY_CALLEE
#define TEE_SESSION_LOCK() mutex_lock(&sess_data->lock)
#define TEE_SESSION_UNLOCK() mutex_unlock(&sess_data->lock)
#else
#define TEE_SESSION_LOCK()
#define TEE_SESSION_UNLOCK()
#endif

static struct TBASE_LEGACY_SESSION_DATA *tbase_create_session_data(void)
{
	struct TBASE_LEGACY_SESSION_DATA *data;

	data = mld_kmalloc(sizeof(struct TBASE_LEGACY_SESSION_DATA),
			   GFP_KERNEL);
	if (INVALID(data))
		return NULL;

	memset(data, 0x0, sizeof(struct TBASE_LEGACY_SESSION_DATA));
	data->device_id = MC_DEVICE_ID_DEFAULT;
	mutex_init(&data->lock);
	return data;
}

static void
tbase_destroy_session_data(struct TBASE_LEGACY_SESSION_DATA *sess_data)
{
	if (VALID(sess_data))
		mld_kfree(sess_data);
}

int tbase_session_open(void **tee_data, void *priv)
{
	enum mc_result mc_ret = MC_DRV_OK;
	struct mc_uuid_t secmem_uuid = {SECMEM_TL_LEGACY_UUID};
	struct TBASE_LEGACY_SESSION_DATA *sess_data;

	UNUSED(priv);

	sess_data = tbase_create_session_data();
	if (INVALID(sess_data)) {
		pr_err("Create session data failed: out of memory!\n");
		return TMEM_TEE_CREATE_SESSION_DATA_FAILED;
	}

	TEE_SESSION_LOCK();

	mc_ret = mc_open_device(sess_data->device_id);
	if (mc_ret != MC_DRV_OK) {
		pr_err("mc_open_device failed: %d\n", mc_ret);
		goto err_open_device;
	}

	mc_ret = mc_malloc_wsm(sess_data->device_id, 0,
			       sizeof(struct secmem_tci_msg_t),
			       (uint8_t **)&sess_data->tci_msg_data, 0);
	if (mc_ret != MC_DRV_OK) {
		pr_err("mc_malloc_wsm failed: %d\n", mc_ret);
		goto err_register_shared_memory;
	}

	sess_data->session.device_id = sess_data->device_id;
	mc_ret = mc_open_session(&sess_data->session, &secmem_uuid,
				 (uint8_t *)sess_data->tci_msg_data,
				 sizeof(struct secmem_tci_msg_t));

	if (mc_ret != MC_DRV_OK) {
		pr_err("mc_open_session failed: %d\n", mc_ret);
		goto err_open_session;
	}

	*tee_data = (void *)sess_data;
	TEE_SESSION_UNLOCK();
	return TMEM_OK;

err_open_session:
	pr_err("mc_free_wsm\n");
	mc_free_wsm(sess_data->device_id, (uint8_t *)sess_data->tci_msg_data);
	sess_data->tci_msg_data = NULL;

err_register_shared_memory:
	pr_err("mc_close_device\n");
	mc_close_device(sess_data->device_id);

err_open_device:
	TEE_SESSION_UNLOCK();
	tbase_destroy_session_data(sess_data);

	return TMEM_TEE_CREATE_SESSION_FAILED;
}

int tbase_session_close(void *tee_data, void *priv)
{
	struct TBASE_LEGACY_SESSION_DATA *sess_data =
		(struct TBASE_LEGACY_SESSION_DATA *)tee_data;

	UNUSED(priv);
	TEE_SESSION_LOCK();

	mc_close_session(&sess_data->session);
	mc_free_wsm(sess_data->device_id, (uint8_t *)sess_data->tci_msg_data);
	mc_close_device(sess_data->device_id);

	TEE_SESSION_UNLOCK();

	tbase_destroy_session_data(sess_data);
	return TMEM_OK;
}

static int tbase_secmem_execute(struct mc_session_handle *session,
				struct secmem_tci_msg_t *tci_msg_data,
				struct TBASE_LEGACY_SESSION_DATA *sess_data,
				u32 cmd, struct secmem_param *param)
{
	enum mc_result mc_ret;

	TEE_SESSION_LOCK();

	tci_msg_data->cmd_secmem.header.commandId = (uint32_t)cmd;
	tci_msg_data->cmd_secmem.len = 0;
	tci_msg_data->sec_handle = param->sec_handle;
	tci_msg_data->alignment = param->alignment;
	tci_msg_data->size = param->size;
	tci_msg_data->refcount = param->refcount;

	mc_ret = mc_notify(session);
	if (mc_ret != MC_DRV_OK) {
		pr_err("mc_notify failed: %d\n", mc_ret);
		goto err_exit;
	}

	mc_ret = mc_wait_notification(session, -1);
	if (mc_ret != MC_DRV_OK) {
		pr_err("mc_wait_notification failed! cmd:%d, ret:0x%x\n", cmd,
		       mc_ret);
		goto err_exit;
	}

	param->sec_handle = tci_msg_data->sec_handle;
	param->refcount = tci_msg_data->refcount;
	param->alignment = tci_msg_data->alignment;
	param->size = tci_msg_data->size;

	if (RSP_ID(cmd) != tci_msg_data->rsp_secmem.header.responseId) {
		pr_err("trustlet did not send a response: 0x%x\n",
		       tci_msg_data->rsp_secmem.header.responseId);
		mc_ret = MC_DRV_ERR_INVALID_RESPONSE;
		goto err_exit;
	}

	if (tci_msg_data->rsp_secmem.header.returnCode != MC_DRV_OK) {
		pr_err("trustlet did not send a valid return code: 0x%x\n",
		       tci_msg_data->rsp_secmem.header.returnCode);
		mc_ret = tci_msg_data->rsp_secmem.header.returnCode;
		goto err_exit;
	}

	return TMEM_OK;

err_exit:
	TEE_SESSION_UNLOCK();

	return mc_ret;
}

int tbase_alloc(u32 alignment, u32 size, u32 *refcount, u32 *sec_handle,
		u8 *owner, u32 id, u32 clean, void *tee_data, void *priv)
{
	int ret;
	struct secmem_param param = {0};
	struct TBASE_LEGACY_SESSION_DATA *sess_data =
		(struct TBASE_LEGACY_SESSION_DATA *)tee_data;
	u32 tee_ta_cmd = (clean ? get_tee_cmd(TEE_OP_ALLOC_ZERO, priv)
				: get_tee_cmd(TEE_OP_ALLOC, priv));

	UNUSED(owner);
	UNUSED(id);
	UNUSED(priv);

	*refcount = 0;
	*sec_handle = 0;

	param.alignment = alignment;
	param.size = size;
	param.refcount = 0;
	param.sec_handle = 0;

	ret = tbase_secmem_execute(&sess_data->session, sess_data->tci_msg_data,
				   sess_data, tee_ta_cmd, &param);
	if (ret)
		return TMEM_TEE_ALLOC_CHUNK_FAILED;

	*refcount = param.refcount;
	*sec_handle = param.sec_handle;
	pr_debug("ref cnt: 0x%x, sec_handle: 0x%llx\n", param.refcount,
		 param.sec_handle);
	return TMEM_OK;
}

int tbase_free(u32 sec_handle, u8 *owner, u32 id, void *tee_data, void *priv)
{
	struct secmem_param param = {0};
	struct TBASE_LEGACY_SESSION_DATA *sess_data =
		(struct TBASE_LEGACY_SESSION_DATA *)tee_data;
	u32 tee_ta_cmd = get_tee_cmd(TEE_OP_FREE, priv);

	UNUSED(owner);
	UNUSED(id);
	UNUSED(priv);

	param.sec_handle = sec_handle;

	if (tbase_secmem_execute(&sess_data->session, sess_data->tci_msg_data,
				 sess_data, tee_ta_cmd, &param))
		return TMEM_TEE_FREE_CHUNK_FAILED;

	return TMEM_OK;
}

int tbase_mem_reg_add(u64 pa, u32 size, void *tee_data, void *priv)
{
	struct secmem_param param = {0};
	struct TBASE_LEGACY_SESSION_DATA *sess_data =
		(struct TBASE_LEGACY_SESSION_DATA *)tee_data;
	u32 tee_ta_cmd = get_tee_cmd(TEE_OP_REGION_ENABLE, priv);

	UNUSED(priv);
	param.sec_handle = pa;
	param.size = size;

	if (tbase_secmem_execute(&sess_data->session, sess_data->tci_msg_data,
				 sess_data, tee_ta_cmd, &param))
		return TMEM_TEE_APPEND_MEMORY_FAILED;

	return TMEM_OK;
}

int tbase_mem_reg_remove(void *tee_data, void *priv)
{
	struct secmem_param param = {0};
	struct TBASE_LEGACY_SESSION_DATA *sess_data =
		(struct TBASE_LEGACY_SESSION_DATA *)tee_data;
	u32 tee_ta_cmd = get_tee_cmd(TEE_OP_REGION_DISABLE, priv);

	UNUSED(priv);

	if (tbase_secmem_execute(&sess_data->session, sess_data->tci_msg_data,
				 sess_data, tee_ta_cmd, &param))
		return TMEM_TEE_RELEASE_MEMORY_FAILED;

	return TMEM_OK;
}

static int tbase_invoke_command(struct trusted_driver_cmd_params *invoke_params,
				void *peer_data, void *priv)
{
	UNUSED(invoke_params);
	UNUSED(peer_data);
	UNUSED(priv);

	pr_err("%s:%d operation is not implemented yet!\n", __func__, __LINE__);
	return TMEM_OPERATION_NOT_IMPLEMENTED;
}

static struct trusted_driver_operations tee_tbase_legacy_peer_ops = {
	.session_open = tbase_session_open,
	.session_close = tbase_session_close,
	.memory_alloc = tbase_alloc,
	.memory_free = tbase_free,
	.memory_grant = tbase_mem_reg_add,
	.memory_reclaim = tbase_mem_reg_remove,
	.invoke_cmd = tbase_invoke_command,
};

void get_tee_peer_ops(struct trusted_driver_operations **ops)
{
	pr_info("SECMEM_TBASE_LEGACY_OPS\n");
	*ops = &tee_tbase_legacy_peer_ops;
}
