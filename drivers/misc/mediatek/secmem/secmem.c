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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

#include "secmem.h"

/* only available for trustonic */
#include "mobicore_driver_api.h"
#include "tlsecmem_api.h"

#define DEFAULT_HANDLES_NUM (64)
#define MAX_OPEN_SESSIONS   (0xffffffff - 1)

/* Debug message event */
#define DBG_EVT_NONE        (0)	/* No event */
#define DBG_EVT_CMD         (1 << 0)	/* SEC CMD related event */
#define DBG_EVT_FUNC        (1 << 1)	/* SEC function event */
#define DBG_EVT_INFO        (1 << 2)	/* SEC information event */
#define DBG_EVT_WRN         (1 << 30)	/* Warning event */
#define DBG_EVT_ERR         (1 << 31)	/* Error event */
#define DBG_EVT_ALL         (0xffffffff)

#define DBG_EVT_MASK        (DBG_EVT_ALL)

#define MSG(evt, fmt, args...) \
do { \
	if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
		pr_debug("[%s] "fmt, SECMEM_NAME, ##args); \
	} \
} while (0)

#define MSG_FUNC() MSG(FUNC, "%s\n", __func__)

struct secmem_handle {
	u32 id;
	u32 type;
};

struct secmem_context {
	spinlock_t lock;
	struct secmem_handle *handles;
	u32 handle_num;
};

static DEFINE_MUTEX(secmem_lock);
static const struct mc_uuid_t secmem_uuid = { TL_SECMEM_UUID };
static struct mc_session_handle secmem_session = { 0 };

static u32 secmem_session_ref;
static u32 secmem_k_session_opened;
static u32 secmem_devid = MC_DEVICE_ID_DEFAULT;
static tciMessage_t *secmem_tci;

static int secmem_execute(u32 cmd, struct secmem_param *param)
{
	enum mc_result mc_ret;
#ifdef SECMEM_DEBUG_DUMP
	int len;
#endif

	mutex_lock(&secmem_lock);

	if (NULL == secmem_tci) {
		mutex_unlock(&secmem_lock);
		MSG(ERR, "secmem_tci not exist\n");
		return -ENODEV;
	}

	secmem_tci->cmd_secmem.header.commandId = (tciCommandId_t) cmd;
	secmem_tci->cmd_secmem.len = 0;
	secmem_tci->sec_handle = param->sec_handle;
	secmem_tci->alignment = param->alignment;
	secmem_tci->size = param->size;
	secmem_tci->refcount = param->refcount;
#ifdef SECMEM_DEBUG_DUMP
	secmem_tci->sender.id = param->id;
	secmem_tci->sender.name[0] = 0;
	if (param->owner[0] != 0) {
		len = param->owner_len > MAX_NAME_SZ ? MAX_NAME_SZ : param->owner_len;
		memcpy(secmem_tci->sender.name, param->owner, len);
		secmem_tci->sender.name[MAX_NAME_SZ - 1] = 0;
	}
#endif

	mc_ret = mc_notify(&secmem_session);

	if (MC_DRV_OK != mc_ret) {
		MSG(ERR, "mc_notify failed: %d\n", mc_ret);
		goto exit;
	}

	mc_ret = mc_wait_notification(&secmem_session, -1);

	if (MC_DRV_OK != mc_ret) {
		MSG(ERR, "mc_wait_notification failed: %d\n", mc_ret);
		goto exit;
	}

	/* correct handle should be get after return from secure world. */
	param->sec_handle = secmem_tci->sec_handle;
	param->refcount = secmem_tci->refcount;
	param->alignment = secmem_tci->alignment;
	param->size = secmem_tci->size;

	if (RSP_ID(cmd) != secmem_tci->rsp_secmem.header.responseId) {
		MSG(ERR, "trustlet did not send a response: %d\n",
		    secmem_tci->rsp_secmem.header.responseId);
		mc_ret = MC_DRV_ERR_INVALID_RESPONSE;
		goto exit;
	}

	if (MC_DRV_OK != secmem_tci->rsp_secmem.header.returnCode) {
		MSG(ERR, "trustlet did not send a valid return code: %d\n",
		    secmem_tci->rsp_secmem.header.returnCode);
		mc_ret = secmem_tci->rsp_secmem.header.returnCode;
	}

exit:

	mutex_unlock(&secmem_lock);

	if (MC_DRV_OK != mc_ret)
		return -ENOSPC;

	return 0;
}

static int secmem_handle_register(struct secmem_context *ctx, u32 type, u32 id)
{
	struct secmem_handle *handle;
	u32 i, num, nspace;

	spin_lock(&ctx->lock);

	num = ctx->handle_num;
	handle = ctx->handles;

	/* find empty space. */
	for (i = 0; i < num; i++, handle++) {
		if (handle->id == 0) {
			handle->id = id;
			handle->type = type;
			spin_unlock(&ctx->lock);
			return 0;
		}
	}

	/* try grow the space */
	nspace = num * 2;
	handle = (struct secmem_handle *)krealloc(ctx->handles,
						  nspace * sizeof(struct secmem_handle),
						  GFP_KERNEL);
	if (handle == NULL) {
		spin_unlock(&ctx->lock);
		return -ENOMEM;
	}
	ctx->handle_num = nspace;
	ctx->handles = handle;

	handle += num;

	memset(handle, 0, (nspace - num) * sizeof(struct secmem_handle));

	handle->id = id;
	handle->type = type;

	spin_unlock(&ctx->lock);

	return 0;
}

static void secmem_handle_unregister_check(struct secmem_context *ctx, u32 type, u32 id)
{
	struct secmem_handle *handle;
	u32 i, num;

	spin_lock(&ctx->lock);

	num = ctx->handle_num;
	handle = ctx->handles;

	/* find empty space. */
	for (i = 0; i < num; i++, handle++) {
		if (handle->id == id) {
			if (handle->type != type) {
				MSG(ERR,
				    "unref check failed, type mismatched (%d!=%d), handle=0x%x\n",
				    _IOC_NR(handle->type), _IOC_NR(type), handle->id);
			}
			break;
		}
	}

	spin_unlock(&ctx->lock);
}

static int secmem_handle_unregister(struct secmem_context *ctx, u32 id)
{
	struct secmem_handle *handle;
	u32 i, num;

	spin_lock(&ctx->lock);

	num = ctx->handle_num;
	handle = ctx->handles;

	/* find empty space. */
	for (i = 0; i < num; i++, handle++) {
		if (handle->id == id) {
			memset(handle, 0, sizeof(struct secmem_handle));
			break;
		}
	}

	spin_unlock(&ctx->lock);

	return 0;
}

static int secmem_handle_cleanup(struct secmem_context *ctx)
{
	int ret = 0;
	u32 i, num, cmd = 0;
	struct secmem_handle *handle;
	struct secmem_param param = { 0 };

	spin_lock(&ctx->lock);

	num = ctx->handle_num;
	handle = ctx->handles;

	for (i = 0; i < num; i++, handle++) {
		if (handle->id != 0) {
			param.sec_handle = handle->id;
			switch (handle->type) {
			case SECMEM_MEM_ALLOC:
				cmd = CMD_SEC_MEM_UNREF;
				break;
			case SECMEM_MEM_REF:
				cmd = CMD_SEC_MEM_UNREF;
				break;
			case SECMEM_MEM_ALLOC_TBL:
				cmd = CMD_SEC_MEM_UNREF_TBL;
				break;
			default:
				MSG(ERR, "secmem_handle_cleanup: incorrect type=%d (ioctl:%d)\n",
				    handle->type, _IOC_NR(handle->type));
				goto error;
			}
			spin_unlock(&ctx->lock);
			ret = secmem_execute(cmd, &param);
			MSG(INFO, "secmem_handle_cleanup: id=0x%x type=%d (ioctl:%d)\n",
			    handle->id, handle->type, _IOC_NR(handle->type));
			spin_lock(&ctx->lock);
		}
	}

error:
	spin_unlock(&ctx->lock);

	return ret;
}

static int secmem_session_open(void)
{
	enum mc_result mc_ret = MC_DRV_OK;

	mutex_lock(&secmem_lock);

	do {
		/* sessions reach max numbers ? */
		if (secmem_session_ref > MAX_OPEN_SESSIONS) {
			MSG(WRN, "secmem_session > 0x%x\n", MAX_OPEN_SESSIONS);
			break;
		}

		if (secmem_session_ref > 0) {
			secmem_session_ref++;
			break;
		}

		/* open device */
		mc_ret = mc_open_device(secmem_devid);
		if (MC_DRV_OK != mc_ret) {
			MSG(ERR, "mc_open_device failed: %d\n", mc_ret);
			break;
		}

		/* allocating WSM for DCI */
		mc_ret = mc_malloc_wsm(secmem_devid, 0, sizeof(tciMessage_t),
				       (uint8_t **) &secmem_tci, 0);
		if (MC_DRV_OK != mc_ret) {
			mc_close_device(secmem_devid);
			MSG(ERR, "mc_malloc_wsm failed: %d\n", mc_ret);
			break;
		}

		/* open session */
		secmem_session.device_id = secmem_devid;
		mc_ret = mc_open_session(&secmem_session, &secmem_uuid,
					 (uint8_t *) secmem_tci, sizeof(tciMessage_t));

		if (MC_DRV_OK != mc_ret) {
			mc_free_wsm(secmem_devid, (uint8_t *) secmem_tci);
			mc_close_device(secmem_devid);
			secmem_tci = NULL;
			MSG(ERR, "mc_open_session failed: %d\n", mc_ret);
			break;
		}
		secmem_session_ref = 1;

	} while (0);

	MSG(INFO, "secmem_session_open: ret=%d, ref=%d\n", mc_ret, secmem_session_ref);

	mutex_unlock(&secmem_lock);

	if (MC_DRV_OK != mc_ret)
		return -ENXIO;

	return 0;
}

static int secmem_session_close(void)
{
	enum mc_result mc_ret = MC_DRV_OK;

	mutex_lock(&secmem_lock);

	do {
		/* session is already closed ? */
		if (secmem_session_ref == 0) {
			MSG(WRN, "secmem_session already closed\n");
			break;
		}

		if (secmem_session_ref > 1) {
			secmem_session_ref--;
			break;
		}

		/* close session */
		mc_ret = mc_close_session(&secmem_session);
		if (MC_DRV_OK != mc_ret) {
			MSG(ERR, "mc_close_session failed: %d\n", mc_ret);
			break;
		}

		/* free WSM for DCI */
		mc_ret = mc_free_wsm(secmem_devid, (uint8_t *) secmem_tci);
		if (MC_DRV_OK != mc_ret) {
			MSG(ERR, "mc_free_wsm failed: %d\n", mc_ret);
			break;
		}
		secmem_tci = NULL;
		secmem_session_ref = 0;

		/* close device */
		mc_ret = mc_close_device(secmem_devid);
		if (MC_DRV_OK != mc_ret)
			MSG(ERR, "mc_close_device failed: %d\n", mc_ret);

	} while (0);

	MSG(INFO, "secmem_session_close: ret=%d, ref=%d\n", mc_ret, secmem_session_ref);

	mutex_unlock(&secmem_lock);

	if (MC_DRV_OK != mc_ret)
		return -ENXIO;

	return 0;

}

static int secmem_open(struct inode *inode, struct file *file)
{
	struct secmem_context *ctx;

	/* allocate session context */
	ctx = kmalloc(sizeof(struct secmem_context), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->handle_num = DEFAULT_HANDLES_NUM;
	ctx->handles = kzalloc(sizeof(struct secmem_handle) * DEFAULT_HANDLES_NUM, GFP_KERNEL);
	spin_lock_init(&ctx->lock);

	if (!ctx->handles) {
		kfree(ctx);
		return -ENOMEM;
	}

	/* open session */
	if (secmem_session_open() < 0) {
		kfree(ctx->handles);
		kfree(ctx);
		return -ENXIO;
	}

	file->private_data = (void *)ctx;

	return 0;
}

static int secmem_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct secmem_context *ctx = (struct secmem_context *)file->private_data;

	if (ctx) {
		/* release session context */
		secmem_handle_cleanup(ctx);
		kfree(ctx->handles);
		kfree(ctx);
		file->private_data = NULL;

		/* close session */
		ret = secmem_session_close();
	}
	return ret;
}

static long secmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct secmem_context *ctx = (struct secmem_context *)file->private_data;
	struct secmem_param param;
	u32 handle;

	if (_IOC_TYPE(cmd) != SECMEM_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > SECMEM_IOC_MAXNR)
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err)
		return -EFAULT;

	err = copy_from_user(&param, (void *)arg, sizeof(param));

	if (err)
		return -EFAULT;

	switch (cmd) {
	case SECMEM_MEM_ALLOC:
		if (!(file->f_mode & FMODE_WRITE))
			return -EROFS;
		err = secmem_execute(CMD_SEC_MEM_ALLOC, &param);
		if (!err)
			secmem_handle_register(ctx, SECMEM_MEM_ALLOC, param.sec_handle);
		break;
	case SECMEM_MEM_REF:
		err = secmem_execute(CMD_SEC_MEM_REF, &param);
		if (!err)
			secmem_handle_register(ctx, SECMEM_MEM_REF, param.sec_handle);
		break;
	case SECMEM_MEM_UNREF:
		handle = param.sec_handle;
		secmem_handle_unregister_check(ctx, SECMEM_MEM_ALLOC, handle);
		err = secmem_execute(CMD_SEC_MEM_UNREF, &param);
		if (!err)
			secmem_handle_unregister(ctx, handle);
		break;
	case SECMEM_MEM_ALLOC_TBL:
		if (!(file->f_mode & FMODE_WRITE))
			return -EROFS;
		err = secmem_execute(CMD_SEC_MEM_ALLOC_TBL, &param);
		if (!err)
			secmem_handle_register(ctx, SECMEM_MEM_ALLOC_TBL, param.sec_handle);
		break;
	case SECMEM_MEM_UNREF_TBL:
		handle = param.sec_handle;
		secmem_handle_unregister_check(ctx, SECMEM_MEM_ALLOC_TBL, handle);
		err = secmem_execute(CMD_SEC_MEM_UNREF_TBL, &param);
		if (!err)
			secmem_handle_unregister(ctx, handle);
		break;
	case SECMEM_MEM_USAGE_DUMP:
		if (!(file->f_mode & FMODE_WRITE))
			return -EROFS;
		err = secmem_execute(CMD_SEC_MEM_USAGE_DUMP, &param);
		break;
#ifdef SECMEM_DEBUG_DUMP
	case SECMEM_MEM_DUMP_INFO:
		if (!(file->f_mode & FMODE_WRITE))
			return -EROFS;
		err = secmem_execute(CMD_SEC_MEM_DUMP_INFO, &param);
		break;
#endif
	default:
		return -ENOTTY;
	}

	if (!err)
		err = copy_to_user((void *)arg, &param, sizeof(param));

	return err;
}

static inline int secmem_kernel_open(void)
{
	if (!secmem_k_session_opened) {
		if (secmem_session_open() < 0)
			return -ENXIO;
		secmem_k_session_opened = 1;
	}

	return 0;
}



#if defined(CONFIG_CMA) && defined(CONFIG_MTK_SVP)
int secmem_api_enable(u32 start, u32 size)
{
	int err = 0;
	struct secmem_param param = { 0 };

	secmem_kernel_open();

	param.sec_handle = start;
	param.size = size;
	err = secmem_execute(CMD_SEC_MEM_ENABLE, &param);

	if (err)
		MSG(ERR, "secmem_api_enable failed: %d\n", err);

	return err;
}
EXPORT_SYMBOL(secmem_api_enable);

int secmem_api_disable(void)
{
	int err = 0;
	struct secmem_param param = { 0 };

	secmem_kernel_open();

	err = secmem_execute(CMD_SEC_MEM_DISABLE, &param);
	if (err)
		MSG(ERR, "secmem_api_disable failed: %d\n", err);

	return err;
}
EXPORT_SYMBOL(secmem_api_disable);

int secmem_api_query(u32 *allocate_size)
{
	int err = 0;
	struct secmem_param param = { 0 };

	secmem_kernel_open();

	err = secmem_execute(CMD_SEC_MEM_ALLOCATED, &param);
	if (err) {
		MSG(ERR, "secmem_api_query failed: %d\n", err);
		*allocate_size = -1;
	} else {
		*allocate_size = param.size;
	}

	if (*allocate_size)
		secmem_execute(CMD_SEC_MEM_DUMP_INFO, &param);

	return err;
}
EXPORT_SYMBOL(secmem_api_query);
#endif

#ifdef SECMEM_KERNEL_API
int secmem_api_alloc(u32 alignment, u32 size, u32 *refcount, u32 *sec_handle, uint8_t *owner,
		     uint32_t id)
{
	int ret = 0;
	struct secmem_param param;

	secmem_kernel_open();

	memset(&param, 0, sizeof(param));
	param.alignment = alignment;
	param.size = size;
	param.refcount = 0;
	param.sec_handle = 0;
	param.id = id;
	if (owner) {
		param.owner_len = strlen(owner) > MAX_NAME_SZ ? MAX_NAME_SZ : strlen(owner);
		memcpy(param.owner, owner, param.owner_len);
		param.owner[MAX_NAME_SZ - 1] = 0;
	}

	ret = secmem_execute(CMD_SEC_MEM_ALLOC, &param);
	if (ret != 0) {
		MSG(ERR, "%s: secmem_execute alloc failed!\n", __func__);
		return ret;
	}

	*refcount = param.refcount;
	*sec_handle = param.sec_handle;

	return 0;
}
EXPORT_SYMBOL(secmem_api_alloc);

int secmem_api_unref(u32 sec_handle, uint8_t *owner, uint32_t id)
{
	int ret = 0;
	struct secmem_param param;

	secmem_kernel_open();

	memset(&param, 0, sizeof(param));
	param.sec_handle = sec_handle;
	param.id = id;
	if (owner) {
		param.owner_len = strlen(owner) > MAX_NAME_SZ ? MAX_NAME_SZ : strlen(owner);
		memcpy(param.owner, owner, param.owner_len);
		param.owner[MAX_NAME_SZ - 1] = 0;
	}

	ret = secmem_execute(CMD_SEC_MEM_UNREF, &param);
	if (ret != 0) {
		MSG(ERR, "%s: secmem_execute unref failed!\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(secmem_api_unref);

int secmem_api_alloc_pa(u32 alignment, u32 size, u32 *refcount, u32 *sec_handle, uint8_t *owner,
	uint32_t id)
{
	int ret = 0;
	struct secmem_param param;

	secmem_kernel_open();

	memset(&param, 0, sizeof(param));
	param.alignment = alignment;
	param.size = size;
	param.refcount = 0;
	param.sec_handle = 0;
	param.id = id;
	if (owner) {
		param.owner_len = strlen(owner) > MAX_NAME_SZ ? MAX_NAME_SZ : strlen(owner);
		memcpy(param.owner, owner, param.owner_len);
		param.owner[MAX_NAME_SZ - 1] = 0;
	}

	ret = secmem_execute(CMD_SEC_MEM_ALLOC_PA, &param);
	if (ret != 0) {
		MSG(ERR, "%s: secmem_execute alloc failed!\n", __func__);
		return ret;
	}

	*refcount = param.refcount;
	*sec_handle = param.sec_handle;

	return 0;
}
EXPORT_SYMBOL(secmem_api_alloc_pa);

int secmem_api_unref_pa(u32 sec_handle, uint8_t *owner, uint32_t id)
{
	int ret = 0;
	struct secmem_param param;

	secmem_kernel_open();

	memset(&param, 0, sizeof(param));
	param.sec_handle = sec_handle;
	param.id = id;
	if (owner) {
		param.owner_len = strlen(owner) > MAX_NAME_SZ ? MAX_NAME_SZ : strlen(owner);
		memcpy(param.owner, owner, param.owner_len);
		param.owner[MAX_NAME_SZ - 1] = 0;
	}

	ret = secmem_execute(CMD_SEC_MEM_UNREF_PA, &param);
	if (ret != 0) {
		MSG(ERR, "%s: secmem_execute unref failed!\n", __func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(secmem_api_unref_pa);
#endif

#ifdef SECMEM_DEBUG_INTERFACE
#include <mach/emi_mpu.h>
#include <mach/mt_secure_api.h>
static int secmem_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	char cmd[10];

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%s", cmd) == 1) {
		if (!strcmp(cmd, "1")) {
			pr_debug("[SECMEM] - test for command 1\n");
			tbase_trigger_aee_dump();
		} else if (!strcmp(cmd, "2")) {
			pr_debug("[SECMEM] - test for command 2\n");
		}
	}

	return count;
}
#endif

static const struct file_operations secmem_fops = {
	.owner = THIS_MODULE,
	.open = secmem_open,
	.release = secmem_release,
	.unlocked_ioctl = secmem_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = secmem_ioctl,
#endif
#ifdef SECMEM_DEBUG_INTERFACE
	.write = secmem_write,
#else
	.write = NULL,
#endif
	.read = NULL,
};

static int __init secmem_init(void)
{
	proc_create("secmem0", (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH), NULL,
		    &secmem_fops);

#ifdef SECMEM_DEBUG_INTERFACE
	{
		unsigned int sec_mem_mpu_attr =
		    SET_ACCESS_PERMISSON(FORBIDDEN, SEC_RW, SEC_RW, FORBIDDEN);
		unsigned int set_mpu_ret = 0;

		set_mpu_ret =
		    emi_mpu_set_region_protection(0xF6000000, 0xFFFFFFFF, 0, sec_mem_mpu_attr);
		pr_debug("[SECMEM] - test for set EMI MPU on region 0, ret:%d\n", set_mpu_ret);
	}
#endif

	return 0;
}
late_initcall(secmem_init);
