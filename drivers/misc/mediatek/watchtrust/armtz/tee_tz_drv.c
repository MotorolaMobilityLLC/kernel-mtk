/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <net/sock.h>
#include <net/netlink.h>

#include "wd_tee/tee_core.h"
#include "wd_tee/tee_ioc.h"
#include "log.h"

#include "tee_shm.h"
#include "tee_supp_com.h"
#include "tee_mutex_wait.h"
#include <tee_wait_queue.h>
#include "arm_common/teesmc.h"
#include "arm_common/teesmc_st.h"

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
/* MTK header */
#ifdef CONFIG_MTK_CLKMGR
#include "mach/mt_clkmgr.h"
#else
#include <linux/clk.h>
#endif

#ifdef CONFIG_TEE_PM_WAKE
#include <linux/wakelock.h>
#endif
#ifdef TUI_ENABLE
#include "tui_hal.h"
#endif
#include "tee_mem.h"
#include "tee_tz_op.h"
#include "tee_tz_priv.h"
#include "handle.h"
#include "wdteesmc.h"
#include <asm/cacheflush.h>
#define _TEE_TZ_NAME "armtz"
#define DEV (ptee->tee->dev)

#ifndef USE_LIB_WDTEE
#include "tee_module.h"
#endif

#define  WD_DEV_DBG(...)

/* #define TEE_STRESS_OUTERCACHE_FLUSH */

/* magic config: bit 1 is set, Secure TEE shall handler NSec IRQs */
#define SEC_ROM_NO_FLAG_MASK	0x0000
#define SEC_ROM_IRQ_ENABLE_MASK	0x0001
#define SEC_ROM_DEFAULT		SEC_ROM_IRQ_ENABLE_MASK
#define TEE_RETURN_BUSY		0x3
#define ALLOC_ALIGN		SZ_4K

#define CAPABLE(tee) !(tee->conf & TEE_CONF_FW_NOT_CAPABLE)

static struct tee_tz *tee_tz;
static void *log_vaddr;
#define NETLINK_WDTEE_LOG 25
static struct handle_db shm_handle_db = HANDLE_DB_INITIALIZER;

/* Temporary workaround until we're only using post 3.13 kernels */
#define ioremap_cache(addr, size) \
		__ioremap((addr), (size), __pgprot(PROT_NORMAL))

#ifdef CONFIG_TEE_PM_WAKE
struct wake_lock wdtee_suspend_lock;
#endif

/*******************************************************************
 * Calling TEE
 *******************************************************************/

static void e_lock_teez(struct tee_tz *ptee)
{
	mutex_lock(&ptee->mutex);
}

static void e_lock_wait_completion_teez(struct tee_tz *ptee)
{
	/*
	 * Release the lock until "something happens" and then reacquire it
	 * again.
	 *
	 * This is needed when TEE returns "busy" and we need to try again
	 * later.
	 */
	ptee->c_waiters++;
	/*
	 * Wait at most one second. Secure world is normally never busy
	 * more than that so we should normally never timeout.
	 */
	wait_for_completion_timeout(&ptee->c, HZ);
	ptee->c_waiters--;
}

static void e_unlock_teez(struct tee_tz *ptee)
{
	/*
	 * If at least one thread is waiting for "something to happen" let
	 * one thread know that "something has happened".
	 */
	if (ptee->c_waiters)
		complete(&ptee->c);
	mutex_unlock(&ptee->mutex);
}

static void handle_rpc_func_cmd_mutex_wait(struct tee_tz *ptee,
					   struct teesmc32_arg *arg32)
{
	struct teesmc32_param *params;

	if (arg32->num_params != 2)
		goto bad;

	params = TEESMC32_GET_PARAMS(arg32);

	if ((params[0].attr & TEESMC_ATTR_TYPE_MASK) !=
	    TEESMC_ATTR_TYPE_VALUE_INPUT)
		goto bad;
	if ((params[1].attr & TEESMC_ATTR_TYPE_MASK) !=
	    TEESMC_ATTR_TYPE_VALUE_INPUT)
		goto bad;

	switch (params[0].u.value.a) {
	case TEE_MUTEX_WAIT_SLEEP:
		tee_mutex_wait_sleep(DEV, &ptee->mutex_wait,
				     params[1].u.value.a, params[1].u.value.b);
		break;
	case TEE_MUTEX_WAIT_WAKEUP:
		tee_mutex_wait_wakeup(DEV, &ptee->mutex_wait,
				      params[1].u.value.a, params[1].u.value.b);
		break;
	case TEE_MUTEX_WAIT_DELETE:
		tee_mutex_wait_delete(DEV, &ptee->mutex_wait,
				      params[1].u.value.a);
		break;
	default:
		goto bad;
	}

	arg32->ret = TEEC_SUCCESS;
	return;
bad:
	arg32->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_func_cmd_wait_queue(struct tee_tz *ptee,
					   struct teesmc32_arg *arg32)
{
	struct teesmc32_param *params;

	if (arg32->num_params != 2)
		goto bad;

	params = TEESMC32_GET_PARAMS(arg32);

	if ((params[0].attr & TEESMC_ATTR_TYPE_MASK) !=
	    TEESMC_ATTR_TYPE_VALUE_INPUT)
		goto bad;
	if ((params[1].attr & TEESMC_ATTR_TYPE_MASK) != TEESMC_ATTR_TYPE_NONE)
		goto bad;

	switch (arg32->cmd) {
	case TEE_RPC_WAIT_QUEUE_SLEEP:
		tee_wait_queue_sleep(DEV, &ptee->wait_queue,
				     params[0].u.value.a);
		break;
	case TEE_RPC_WAIT_QUEUE_WAKEUP:
		tee_wait_queue_wakeup(DEV, &ptee->wait_queue,
				      params[0].u.value.a);
		break;
	default:
		goto bad;
	}

	arg32->ret = TEEC_SUCCESS;
	return;
bad:
	arg32->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_func_cmd_wait(struct teesmc32_arg *arg32)
{
	struct teesmc32_param *params;
	u32 msec_to_wait;

	if (arg32->num_params != 1)
		goto bad;

	params = TEESMC32_GET_PARAMS(arg32);
	msec_to_wait = params[0].u.value.a;

	/* set task's state to interruptible sleep */
	set_current_state(TASK_INTERRUPTIBLE);

	/* take a nap */
	schedule_timeout(msecs_to_jiffies(msec_to_wait));

	arg32->ret = TEEC_SUCCESS;
	return;
bad:
	arg32->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_func_cmd_to_supplicant(struct tee_tz *ptee,
					      struct teesmc32_arg *arg32)
{
	struct teesmc32_param *params;
	struct tee_rpc_invoke inv;
	size_t n;
	uint32_t ret;

	if (arg32->num_params > TEE_RPC_BUFFER_NUMBER) {
		arg32->ret = TEEC_ERROR_GENERIC;
		return;
	}

	params = TEESMC32_GET_PARAMS(arg32);

	memset(&inv, 0, sizeof(inv));
	inv.cmd = arg32->cmd;
	inv.session = arg32->session;
	/*
	 * Set a suitable error code in case tee-supplicant
	 * ignores the request.
	 */
	inv.res = TEEC_ERROR_NOT_IMPLEMENTED;
	inv.nbr_bf = arg32->num_params;
	WD_DEV_DBG(ptee->tee->dev, "%s: num_params %d\n", __func__,
		arg32->num_params);
	for (n = 0; n < arg32->num_params; n++) {
		WD_DEV_DBG(ptee->tee->dev, "%s attr %d\n", __func__,
			params[n].attr & TEESMC_ATTR_TYPE_MASK);
		switch (params[n].attr & TEESMC_ATTR_TYPE_MASK) {
		case TEESMC_ATTR_TYPE_VALUE_INPUT:
		case TEESMC_ATTR_TYPE_VALUE_INOUT:
			inv.cmds[n].fd = (int)params[n].u.value.a;
			/* Fall through */
		case TEESMC_ATTR_TYPE_VALUE_OUTPUT:
			inv.cmds[n].type = TEE_RPC_VALUE;
			break;
		case TEESMC_ATTR_TYPE_MEMREF_INPUT:
		case TEESMC_ATTR_TYPE_MEMREF_OUTPUT:
		case TEESMC_ATTR_TYPE_MEMREF_INOUT:
			inv.cmds[n].buffer =
			    (void *)(uintptr_t) params[n].u.memref.buf_ptr;
			inv.cmds[n].size = params[n].u.memref.size;
			inv.cmds[n].type = TEE_RPC_BUFFER;
			WD_DEV_DBG(ptee->tee->dev,
				"%s size %d type %d buffer %p\n", __func__,
				inv.cmds[n].size, inv.cmds[n].type,
				inv.cmds[n].buffer);
			break;
		default:
			arg32->ret = TEEC_ERROR_GENERIC;
			return;
		}
	}

	WD_DEV_DBG(ptee->tee->dev, "%s: > tee_supp_cmd\n", __func__);

	WD_DEV_DBG(ptee->tee->dev, "%s: inv addr %p\n", __func__, &inv);

	ret = tee_supp_cmd(ptee->tee, TEE_RPC_ICMD_INVOKE, &inv, sizeof(inv));
	WD_DEV_DBG(ptee->tee->dev,
		"%s: < tee_supp_cmd ret %d\n", __func__, ret);
	if (ret == TEEC_RPC_OK)
		arg32->ret = inv.res;

	for (n = 0; n < arg32->num_params; n++) {
		switch (params[n].attr & TEESMC_ATTR_TYPE_MASK) {
		case TEESMC_ATTR_TYPE_MEMREF_OUTPUT:
		case TEESMC_ATTR_TYPE_MEMREF_INOUT:
			/*
			 * Allow supplicant to assign a new pointer
			 * to an out-buffer. Needed when the
			 * supplicant allocates a new buffer, for
			 * instance when loading a TA.
			 */
			params[n].u.memref.buf_ptr =
			    (uint32_t) (uintptr_t) inv.cmds[n].buffer;
			params[n].u.memref.size = inv.cmds[n].size;
			break;
		case TEESMC_ATTR_TYPE_VALUE_OUTPUT:
		case TEESMC_ATTR_TYPE_VALUE_INOUT:
			params[n].u.value.a = inv.cmds[n].fd;
			break;
		default:
			break;
		}
	}
}

static void handle_rpc_func_cmd(struct tee_tz *ptee, u32 parg32)
{
	struct teesmc32_arg *arg32;

	arg32 = tee_shm_pool_p2v(DEV, ptee->shm_pool, parg32);
	if (!arg32)
		return;

	switch (arg32->cmd) {
	case TEE_RPC_MUTEX_WAIT:
		handle_rpc_func_cmd_mutex_wait(ptee, arg32);
		break;
	case TEE_RPC_WAIT_QUEUE_SLEEP:
	case TEE_RPC_WAIT_QUEUE_WAKEUP:
		handle_rpc_func_cmd_wait_queue(ptee, arg32);
		break;
	case TEE_RPC_WAIT:
		handle_rpc_func_cmd_wait(arg32);
		break;
	default:
		handle_rpc_func_cmd_to_supplicant(ptee, arg32);
	}
}

static struct tee_shm *handle_rpc_alloc(struct tee_tz *ptee, size_t size)
{
	struct tee_rpc_alloc rpc_alloc;

	WD_DEV_DBG(ptee->tee->dev, "%s: > tee_supp_cmd\n", __func__);
	rpc_alloc.size = size;
	tee_supp_cmd(ptee->tee, TEE_RPC_ICMD_ALLOCATE,
		     &rpc_alloc, sizeof(rpc_alloc));
	return rpc_alloc.shm;
}

static void handle_rpc_free(struct tee_tz *ptee, struct tee_shm *shm)
{
	struct tee_rpc_free rpc_free;

	WD_DEV_DBG(ptee->tee->dev, "%s: > tee_supp_cmd\n", __func__);
	if (!shm)
		return;
	rpc_free.shm = shm;
	tee_supp_cmd(ptee->tee, TEE_RPC_ICMD_FREE, &rpc_free, sizeof(rpc_free));
}

static u32 handle_rpc(struct tee_tz *ptee, struct smc_param *param,
		      uintptr_t parg32)
{
	struct tee_shm *shm;
	int cookie;

	WD_DEV_DBG(ptee->tee->dev, "%s: > cmd:%08llx\n", __func__, param->a0);

	switch (TEESMC_RETURN_GET_RPC_FUNC(param->a0)) {
	case TEESMC_RPC_FUNC_ALLOC_ARG:
		param->a1 = tee_shm_pool_alloc(DEV, ptee->shm_pool,
					       param->a1, 4);
		WD_DEV_DBG(ptee->tee->dev,
			"%s: > TEESMC_RPC_FUNC_ALLOC_ARG 0x%llx\n", __func__,
			param->a1);
		break;
	case TEESMC_RPC_FUNC_ALLOC_PAYLOAD:
		/* Can't support payload shared memory with this interface */
		param->a2 = 0;
		WD_DEV_DBG(ptee->tee->dev,
			"%s: > TEESMC_RPC_FUNC_ALLOC_PAYLOAD 0x%llx\n",
			__func__, param->a2);
		break;
	case TEESMC_RPC_FUNC_FREE_ARG:
		tee_shm_pool_free(DEV, ptee->shm_pool, param->a1, 0);
		WD_DEV_DBG(ptee->tee->dev,
			"%s: > TEESMC_RPC_FUNC_FREE_ARG 0x%llx\n", __func__,
			param->a1);
		break;
	case TEESMC_RPC_FUNC_FREE_PAYLOAD:
		/* Can't support payload shared memory with this interface */
		WD_DEV_DBG(ptee->tee->dev,
			"%s: > TEESMC_RPC_FUNC_FREE_PAYLOAD\n",
				__func__);
		break;
	case TEESMC_ST_RPC_FUNC_ALLOC_PAYLOAD:
		shm = handle_rpc_alloc(ptee, param->a1);
		if (IS_ERR_OR_NULL(shm)) {
			dev_err(ptee->tee->dev,
				"%s: > err cmd:%08llx  IS_ERR_OR_NULL\n",
				__func__, param->a0);
			param->a1 = 0;
			param->a3 = tee_shm_pool_check(DEV, ptee->shm_pool);
			break;
		}
		cookie = handle_get(&shm_handle_db, shm);
		if (cookie < 0) {
			dev_err(ptee->tee->dev,
				"%s: > err cmd:%08llx get cookie err\n",
				__func__, param->a0);
			handle_rpc_free(ptee, shm);
			param->a1 = 0;
			break;
		}
		param->a1 = shm->paddr;
		param->a2 = cookie;
		WD_DEV_DBG(ptee->tee->dev, "%s: > shm->addr 0x%p, cookie %d\n",
			__func__, (void *)shm->paddr, cookie);
		break;
	case TEESMC_ST_RPC_FUNC_FREE_PAYLOAD:
		WD_DEV_DBG(ptee->tee->dev,
			"%s: > TEESMC_ST_RPC_FUNC_FREE_PAYLOAD, cookie %lld\n",
			__func__, param->a1);
		shm = handle_put(&shm_handle_db, param->a1);
		if (shm == NULL)
			break;
		handle_rpc_free(ptee, shm);
		break;
	case TEESMC_RPC_FUNC_IRQ:
		WD_DEV_DBG(ptee->tee->dev, "%s: > TEESMC_RPC_FUNC_IRQ\n",
			__func__);
		break;
	case TEESMC_RPC_FUNC_CMD:
		WD_DEV_DBG(ptee->tee->dev, "%s: > TEESMC_RPC_FUNC_CMD\n",
			__func__);
		handle_rpc_func_cmd(ptee, param->a1);
		break;
	default:
		dev_err(DEV, "Unknown RPC func 0x%x\n",
			(u32) TEESMC_RETURN_GET_RPC_FUNC(param->a0));
		break;
	}
	WD_DEV_DBG(ptee->tee->dev, "%s: <\n", __func__);
	if (irqs_disabled())
		return TEESMC32_FASTCALL_RETURN_FROM_RPC;
	else
		return TEESMC32_CALL_RETURN_FROM_RPC;
}

static void call_tee(struct tee_tz *ptee,
		     uintptr_t parg32, struct teesmc32_arg *arg32)
{
	u32 ret;
	u32 funcid;
	struct smc_param param = { 0 };

	if (irqs_disabled())
		funcid = TEESMC32_FASTCALL_WITH_ARG;
	else
		funcid = TEESMC32_CALL_WITH_ARG;

	/*
	 * Commented out elements used to visualize the layout dynamic part
	 * of the struct. Note that these fields are not available at all
	 * if num_params == 0.
	 *
	 * params is accessed through the macro TEESMC32_GET_PARAMS
	 */

	/* struct teesmc32_param params[num_params]; */

	param.a1 = parg32;
	e_lock_teez(ptee);
#ifdef CONFIG_TEE_PM_WAKE
	wake_lock(&wdtee_suspend_lock);
#endif
	while (true) {

		wdtee_log_thread_read();
		param.a0 = funcid;
		do {
#ifndef SMP_ENABLE
			work_on_cpu(0, (long (*)(void *))tee_smc_call,
				    (void *)&param);
#else
			tee_smc_call(&param);
#endif
			if (param.a0 != SMC_CMD_IRQ)
				break;

			if (param.a1 == 0) {
				wd_log_read();
				param.a0 = SMC_CMD_IRQ_RET;
			} else if (param.a1 == 1) {
				if (log_vaddr != NULL) {
					pr_err("wdtee %s\n",
					       (char *)log_vaddr);
				}
				param.a0 = SMC_CMD_IRQ_RET;
			} else if (param.a1 == 3) {
				wd_log_read();
				param.a0 = 0;
				schedule_timeout_interruptible(1);
			}
		} while (1);
		wdtee_log_thread_read();
		ret = param.a0;

		if (ret == TEESMC_RETURN_EBUSY) {
			/*
			 * Since secure world returned busy, release the
			 * lock we had when entering this function and wait
			 * for "something to happen" (something else to
			 * exit from secure world and needed resources may
			 * have become available).
			 */
			e_lock_wait_completion_teez(ptee);
		} else if (TEESMC_RETURN_IS_RPC(ret)) {
			/* Process the RPC. */
			funcid = handle_rpc(ptee, &param, parg32);
		} else {
			break;
		}
	}
#ifdef CONFIG_TEE_PM_WAKE
	wake_unlock(&wdtee_suspend_lock);
#endif
	e_unlock_teez(ptee);

	switch (ret) {
	case TEESMC_RETURN_UNKNOWN_FUNCTION:
		break;
	case TEESMC_RETURN_OK:
		/* arg32->ret set by secure world */
		break;
	default:
		/* Should not happen */
		dev_err(DEV, "call tee return err %d\n", ret);
		arg32->ret = TEEC_ERROR_COMMUNICATION;
		arg32->ret_origin = TEEC_ORIGIN_COMMS;
		break;
	}
}

/*******************************************************************
 * TEE service invoke formating
 *******************************************************************/

/* allocate tee service argument buffer and return virtual address */
static void *alloc_tee_arg(struct tee_tz *ptee, unsigned long *p, size_t l)
{
	void *vaddr;

	WD_DEV_DBG(DEV, ">\n");

	if ((p == NULL) || (l == 0))
		return NULL;

	/* assume a 4 bytes aligned is sufficient */
	*p = tee_shm_pool_alloc(DEV, ptee->shm_pool, l, ALLOC_ALIGN);
	if (*p == 0)
		return NULL;

	vaddr = tee_shm_pool_p2v(DEV, ptee->shm_pool, *p);

	WD_DEV_DBG(DEV, "< %p\n", vaddr);

	return vaddr;
}

/* free tee service argument buffer (from its physical address) */
static void free_tee_arg(struct tee_tz *ptee, unsigned long p)
{
	WD_DEV_DBG(DEV, ">\n");

	if (p)
		tee_shm_pool_free(DEV, ptee->shm_pool, p, 0);

	WD_DEV_DBG(DEV, "<\n");
}

static uint32_t get_cache_attrs(struct tee_tz *ptee)
{
	if (tee_shm_pool_is_cached(ptee->shm_pool))
		return TEESMC_ATTR_CACHE_DEFAULT << TEESMC_ATTR_CACHE_SHIFT;
	else
		return TEESMC_ATTR_CACHE_NONCACHE << TEESMC_ATTR_CACHE_SHIFT;
}

static uint32_t param_type_teec2teesmc(uint8_t type)
{
	switch (type) {
	case TEEC_NONE:
		return TEESMC_ATTR_TYPE_NONE;
	case TEEC_VALUE_INPUT:
		return TEESMC_ATTR_TYPE_VALUE_INPUT;
	case TEEC_VALUE_OUTPUT:
		return TEESMC_ATTR_TYPE_VALUE_OUTPUT;
	case TEEC_VALUE_INOUT:
		return TEESMC_ATTR_TYPE_VALUE_INOUT;
	case TEEC_MEMREF_TEMP_INPUT:
	case TEEC_MEMREF_PARTIAL_INPUT:
		return TEESMC_ATTR_TYPE_MEMREF_INPUT;
	case TEEC_MEMREF_TEMP_OUTPUT:
	case TEEC_MEMREF_PARTIAL_OUTPUT:
		return TEESMC_ATTR_TYPE_MEMREF_OUTPUT;
	case TEEC_MEMREF_WHOLE:
	case TEEC_MEMREF_TEMP_INOUT:
	case TEEC_MEMREF_PARTIAL_INOUT:
		return TEESMC_ATTR_TYPE_MEMREF_INOUT;
	default:
		WARN_ON(true);
		return 0;
	}
}

static void set_params(struct tee_tz *ptee,
		       struct teesmc32_param
		       params32[TEEC_CONFIG_PAYLOAD_REF_COUNT],
		       uint32_t param_types, struct tee_data *data)
{
	size_t n;
	struct tee_shm *shm;
	struct TEEC_Value *value;

	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		uint32_t type = TEEC_PARAM_TYPE_GET(param_types, n);

		params32[n].attr = param_type_teec2teesmc(type);
		if (params32[n].attr == TEESMC_ATTR_TYPE_NONE)
			continue;
		if (params32[n].attr < TEESMC_ATTR_TYPE_MEMREF_INPUT) {
			value = (struct TEEC_Value *)&data->params[n];
			params32[n].u.value.a = value->a;
			params32[n].u.value.b = value->b;
			continue;
		}
		shm = data->params[n].shm;
		params32[n].attr |= get_cache_attrs(ptee);
		params32[n].u.memref.size = shm->size_req;
		if (shm->uaddr == 0)
			params32[n].u.memref.buf_ptr = 0;
		else
			params32[n].u.memref.buf_ptr = shm->paddr;
	}
}

static void get_params(struct tee_data *data,
		       struct teesmc32_param
		       params32[TEEC_CONFIG_PAYLOAD_REF_COUNT])
{
	size_t n;
	struct tee_shm *shm;
	struct TEEC_Value *value;

	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		if (params32[n].attr == TEESMC_ATTR_TYPE_NONE)
			continue;
		if (params32[n].attr < TEESMC_ATTR_TYPE_MEMREF_INPUT) {
			value = (struct TEEC_Value *)&data->params[n].value;
			value->a = params32[n].u.value.a;
			value->b = params32[n].u.value.b;
			continue;
		}
		shm = data->params[n].shm;
		shm->size_req = params32[n].u.memref.size;
	}
}

/*
 * tee_open_session - invoke TEE to open a GP TEE session
 */
static int tz_open(struct tee_session *sess, struct tee_cmd *cmd)
{
	struct tee *tee;
	struct tee_tz *ptee;
	int ret = 0;

	struct teesmc32_arg *arg32;
	struct teesmc32_param *params32;
	struct teesmc_meta_open_session *meta;
	uintptr_t parg32;
	uintptr_t pmeta;
	size_t num_meta = 1;
	uint8_t *ta;
	struct TEEC_UUID *uuid;

	tee = sess->ctx->tee;
	ptee = tee->priv;

	if (cmd->uuid)
		uuid = cmd->uuid->kaddr;
	else
		uuid = NULL;

	WD_DEV_DBG(tee->dev, "> ta kaddr %p, uuid=%08x-%04x-%04x\n",
		(cmd->ta) ? cmd->ta->kaddr : NULL,
		((uuid) ? uuid->timeLow : 0xDEAD),
		((uuid) ? uuid->timeMid : 0xDEAD),
		((uuid) ? uuid->timeHiAndVersion : 0xDEAD));

	if (!CAPABLE(ptee->tee)) {
		WD_DEV_DBG(tee->dev, "< not capable\n");
		return -EBUSY;
	}

	/* case ta binary is inside the open request */
	ta = NULL;
	if (cmd->ta)
		ta = cmd->ta->kaddr;
	if (ta)
		num_meta++;

	arg32 =
	    alloc_tee_arg(ptee, &parg32,
			  TEESMC32_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT +
						num_meta));
	meta = alloc_tee_arg(ptee, &pmeta, sizeof(*meta));

	if ((arg32 == NULL) || (meta == NULL)) {
		free_tee_arg(ptee, parg32);
		free_tee_arg(ptee, pmeta);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memset(arg32, 0, sizeof(*arg32));
	memset(meta, 0, sizeof(*meta));
	arg32->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT + num_meta;
	params32 = TEESMC32_GET_PARAMS(arg32);

	arg32->cmd = TEESMC_CMD_OPEN_SESSION;

	params32[0].u.memref.buf_ptr = pmeta;
	params32[0].u.memref.size = sizeof(*meta);
	params32[0].attr = TEESMC_ATTR_TYPE_MEMREF_INPUT |
	    TEESMC_ATTR_META | get_cache_attrs(ptee);

	if (ta) {
		params32[1].u.memref.buf_ptr =
		    tee_shm_pool_v2p(DEV, ptee->shm_pool, cmd->ta->kaddr);
		params32[1].u.memref.size = cmd->ta->size_req;
		params32[1].attr = TEESMC_ATTR_TYPE_MEMREF_INPUT |
		    TEESMC_ATTR_META | get_cache_attrs(ptee);
	}

	if (uuid != NULL)
		memcpy(meta->uuid, uuid, TEESMC_UUID_LEN);
	meta->clnt_login = 0;	/* FIXME: is this reliable ? used ? */

	params32 += num_meta;
	set_params(ptee, params32, cmd->param.type, &cmd->param);

	call_tee(ptee, parg32, arg32);

	get_params(&cmd->param, params32);

	if (arg32->ret != TEEC_ERROR_COMMUNICATION) {
		sess->sessid = arg32->session;
		cmd->err = arg32->ret;
		cmd->origin = arg32->ret_origin;
	} else
		ret = -EBUSY;

	free_tee_arg(ptee, parg32);
	free_tee_arg(ptee, pmeta);

	WD_DEV_DBG(DEV, "< %x:%d\n", arg32->ret, ret);
	return ret;
}

/*
 * tee_invoke_command - invoke TEE to invoke a GP TEE command
 */
static int tz_invoke(struct tee_session *sess, struct tee_cmd *cmd)
{
	struct tee *tee;
	struct tee_tz *ptee;
	int ret = 0;

	struct teesmc32_arg *arg32;
	uintptr_t parg32;
	struct teesmc32_param *params32;

	tee = sess->ctx->tee;
	ptee = tee->priv;

	WD_DEV_DBG(DEV, "> sessid %x cmd %x type %x\n",
		sess->sessid, cmd->cmd, cmd->param.type);

	if (!CAPABLE(tee)) {
		WD_DEV_DBG(tee->dev, "< not capable\n");
		return -EBUSY;
	}

	arg32 = (typeof(arg32)) alloc_tee_arg(ptee, &parg32,
					      TEESMC32_GET_ARG_SIZE
					      (TEEC_CONFIG_PAYLOAD_REF_COUNT));
	if (!arg32) {
		free_tee_arg(ptee, parg32);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memset(arg32, 0, sizeof(*arg32));
	arg32->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;
	params32 = TEESMC32_GET_PARAMS(arg32);

	arg32->cmd = TEESMC_CMD_INVOKE_COMMAND;
	arg32->session = sess->sessid;
	arg32->ta_func = cmd->cmd;

	set_params(ptee, params32, cmd->param.type, &cmd->param);

	call_tee(ptee, parg32, arg32);

	get_params(&cmd->param, params32);

	if (arg32->ret != TEEC_ERROR_COMMUNICATION) {
		cmd->err = arg32->ret;
		cmd->origin = arg32->ret_origin;
	} else
		ret = -EBUSY;

	free_tee_arg(ptee, parg32);

	WD_DEV_DBG(DEV, "< %x:%d\n", arg32->ret, ret);
	return ret;
}

/*
 * tee_cancel_command - invoke TEE to cancel a GP TEE command
 */
static int tz_cancel(struct tee_session *sess, struct tee_cmd *cmd)
{
	struct tee *tee;
	struct tee_tz *ptee;
	int ret = 0;

	struct teesmc32_arg *arg32;
	uintptr_t parg32;
	struct smc_param param = { 0 };

	tee = sess->ctx->tee;
	ptee = tee->priv;

	WD_DEV_DBG(DEV, "cancel on sessid=%08x\n", sess->sessid);

	arg32 = alloc_tee_arg(ptee, &parg32, TEESMC32_GET_ARG_SIZE(1));
	if (arg32 == NULL) {
		WD_DEV_DBG(tee->dev, "< alloc_tee_arg %p\n", arg32);
		free_tee_arg(ptee, parg32);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	memset(arg32, 0, sizeof(*arg32));
	arg32->cmd = TEESMC_CMD_CANCEL;
	arg32->session = sess->sessid;

	param.a0 = TEESMC_RPC_FUNC_CMD;
	param.a1 = parg32;
	handle_rpc(ptee, &param, parg32);

	if (arg32->ret == TEEC_ERROR_COMMUNICATION)
		ret = -EBUSY;

	free_tee_arg(ptee, parg32);

	WD_DEV_DBG(DEV, "< %x:%d\n", arg32->ret, ret);
	return ret;
}

/*
 * tee_close_session - invoke TEE to close a GP TEE session
 */
static int tz_close(struct tee_session *sess)
{
	struct tee *tee;
	struct tee_tz *ptee;
	int ret = 0;

	struct teesmc32_arg *arg32;
	uintptr_t parg32;

	tee = sess->ctx->tee;
	ptee = tee->priv;

	WD_DEV_DBG(DEV, "close on sessid=%08x\n", sess->sessid);

	if (!CAPABLE(tee)) {
		WD_DEV_DBG(tee->dev, "< not capable\n");
		return -EBUSY;
	}

	arg32 = alloc_tee_arg(ptee, &parg32, TEESMC32_GET_ARG_SIZE(0));
	if (arg32 == NULL) {
		free_tee_arg(ptee, parg32);
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	WD_DEV_DBG(DEV, "> [%x]\n", sess->sessid);

	memset(arg32, 0, sizeof(*arg32));
	arg32->cmd = TEESMC_CMD_CLOSE_SESSION;
	arg32->session = sess->sessid;

	call_tee(ptee, parg32, arg32);

	if (arg32->ret == TEEC_ERROR_COMMUNICATION)
		ret = -EBUSY;

	free_tee_arg(ptee, parg32);

	WD_DEV_DBG(DEV, "< %x:%d\n", arg32->ret, ret);
	return ret;
}

static struct tee_shm *tz_alloc(struct tee *tee, size_t size, uint32_t flags)
{
	struct tee_shm *shm = NULL;
	struct tee_tz *ptee;
	size_t size_aligned;

	ptee = tee->priv;

	WD_DEV_DBG(DEV, "%s: s=%d,flags=0x%08x\n", __func__, (int)size, flags);

	size_aligned = ((size / SZ_4K) + 1) * SZ_4K;
	if (unlikely(size_aligned == 0)) {
		dev_err(DEV, "[%s] requested size too big\n", __func__);
		return NULL;
	}

	shm = devm_kzalloc(tee->dev, sizeof(struct tee_shm), GFP_KERNEL);
	if (!shm) {
		/* dev_err(tee->dev, "%s: kzalloc failed\n", __func__); */
		return ERR_PTR(-ENOMEM);
	}

	shm->size_alloc = ((size / SZ_4K) + 1) * SZ_4K;
	shm->size_req = size;
	shm->paddr = tee_shm_pool_alloc(tee->dev, ptee->shm_pool,
					shm->size_alloc, ALLOC_ALIGN);
	if (!shm->paddr) {
		dev_err(tee->dev, "%s: cannot alloc memory, size 0x%lx\n",
			__func__, (unsigned long)shm->size_alloc);
		devm_kfree(tee->dev, shm);
		return ERR_PTR(-ENOMEM);
	}
	shm->kaddr = tee_shm_pool_p2v(tee->dev, ptee->shm_pool, shm->paddr);
	if (!shm->kaddr) {
		dev_err(tee->dev, "%s: p2v(%p)=0\n", __func__,
			(void *)shm->paddr);
		tee_shm_pool_free(tee->dev, ptee->shm_pool, shm->paddr, NULL);
		devm_kfree(tee->dev, shm);
		return ERR_PTR(-EFAULT);
	}
	shm->flags = flags;
	if (ptee->shm_cached)
		shm->flags |= TEE_SHM_CACHED;

	WD_DEV_DBG(tee->dev, "%s: kaddr=%p, paddr=%p, shm=%p, size %x:%x\n",
		__func__, shm->kaddr, (void *)shm->paddr, shm,
		(unsigned int)shm->size_req, (unsigned int)shm->size_alloc);

	return shm;
}

static void tz_free(struct tee_shm *shm)
{
	size_t size;
	int ret;
	struct tee *tee;
	struct tee_tz *ptee;

	tee = shm->tee;
	ptee = tee->priv;

	WD_DEV_DBG(tee->dev, "%s: shm=%p\n", __func__, shm);

	ret = tee_shm_pool_free(tee->dev, ptee->shm_pool, shm->paddr, &size);
	if (!ret) {
		devm_kfree(tee->dev, shm);
		shm = NULL;
	}
}

static int tz_shm_inc_ref(struct tee_shm *shm)
{
	struct tee *tee;
	struct tee_tz *ptee;

	tee = shm->tee;
	ptee = tee->priv;

	return tee_shm_pool_incref(tee->dev, ptee->shm_pool, shm->paddr);
}

#ifdef CONFIG_OUTER_CACHE
/*
 * Synchronised outer cache maintenance support
 */
#ifndef CONFIG_ARM_TZ_SUPPORT
/* weak outer_tz_mutex in case not supported by kernel */
bool __weak outer_tz_mutex(unsigned long *p)
{
	pr_err("weak outer_tz_mutex");
	if (p != NULL)
		return false;
	return true;
}
#endif

/* register_outercache_mutex - Negotiate/Disable outer cache shared mutex */
static int register_outercache_mutex(struct tee_tz *ptee, bool reg)
{
	unsigned long *vaddr = NULL;
	int ret = 0;
	struct smc_param param;
	uintptr_t paddr = 0;

	WD_DEV_DBG(ptee->tee->dev, ">\n");

	if ((reg == true) && (ptee->tz_outer_cache_mutex != NULL)) {
		dev_err(DEV, "outer cache shared mutex already registered\n");
		return -EINVAL;
	}
	if ((reg == false) && (ptee->tz_outer_cache_mutex == NULL))
		return 0;

	mutex_lock(&ptee->mutex);

	if (reg == false) {
		vaddr = ptee->tz_outer_cache_mutex;
		ptee->tz_outer_cache_mutex = NULL;
		goto out;
	}

	memset(&param, 0, sizeof(param));
	param.a0 = TEESMC32_ST_FASTCALL_L2CC_MUTEX;
	param.a1 = TEESMC_ST_L2CC_MUTEX_GET_ADDR;
	wdtee_log_thread_read();
	do {
#ifndef SMP_ENABLE
		work_on_cpu(0, (long (*)(void *))tee_smc_call, (void *)&param);
#else
		tee_smc_call(&param);
#endif
		if (param.a0 != SMC_CMD_IRQ)
			break;

		if (param.a1 == 0) {
			wd_log_read();
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 1) {
			if (log_vaddr != NULL) {
				pr_err("wdtee %s\n",
				       (char *)log_vaddr);
			}
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 3) {
			wd_log_read();
			param.a0 = 0;
		}

		param.a0 = SMC_CMD_IRQ_RET;
	} while (1);
	wdtee_log_thread_read();

	if (param.a0 != TEESMC_RETURN_OK) {
		dev_warn(DEV, "no TZ l2cc mutex service supported\n");
		goto out;
	}
	paddr = param.a2;
	WD_DEV_DBG(DEV, "outer cache shared mutex paddr 0x%lx\n", paddr);

	vaddr = ioremap_cache(paddr, sizeof(u32));
	if (vaddr == NULL) {
		dev_warn(DEV, "TZ l2cc mutex disabled: ioremap failed\n");
		ret = -ENOMEM;
		goto out;
	}

	WD_DEV_DBG(DEV, "outer cache shared mutex vaddr %p\n", vaddr);
	if (outer_tz_mutex(vaddr) == false) {
		dev_warn(DEV, "TZ l2cc mutex disabled: outer cache refused\n");
		goto out;
	}

	memset(&param, 0, sizeof(param));
	param.a0 = TEESMC32_ST_FASTCALL_L2CC_MUTEX;
	param.a1 = TEESMC_ST_L2CC_MUTEX_ENABLE;
	wdtee_log_thread_read();
	do {
#ifndef SMP_ENABLE
		work_on_cpu(0, (long (*)(void *))tee_smc_call, (void *)&param);
#else
		tee_smc_call(&param);
#endif
		if (param.a0 != SMC_CMD_IRQ)
			break;

		if (param.a1 == 0) {
			wd_log_read();
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 1) {
			if (log_vaddr != NULL) {
				pr_err("wdtee %s\n",
				       (char *)log_vaddr);
			}
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 3) {
			wd_log_read();
			param.a0 = 0;
		}
		param.a0 = SMC_CMD_IRQ_RET;
	} while (1);
	wdtee_log_thread_read();

	if (param.a0 != TEESMC_RETURN_OK) {

		dev_warn(DEV, "TZ l2cc mutex disabled: TZ enable failed\n");
		goto out;
	}
	ptee->tz_outer_cache_mutex = vaddr;

out:
	if (ptee->tz_outer_cache_mutex == NULL) {
		memset(&param, 0, sizeof(param));
		param.a0 = TEESMC32_ST_FASTCALL_L2CC_MUTEX;
		param.a1 = TEESMC_ST_L2CC_MUTEX_DISABLE;

		wdtee_log_thread_read();
		do {
#ifndef SMP_ENABLE
			work_on_cpu(0, (long (*)(void *))tee_smc_call,
				    (void *)&param);
#else
			tee_smc_call(&param);
#endif
			if (param.a0 != SMC_CMD_IRQ)
				break;
			if (param.a1 == 0) {
				wd_log_read();
				param.a0 = SMC_CMD_IRQ_RET;
			} else if (param.a1 == 1) {
				if (log_vaddr != NULL) {
					pr_err("wdtee %s\n",
					       (char *)log_vaddr);
				}
				param.a0 = SMC_CMD_IRQ_RET;
			} else if (param.a1 == 3) {
				wd_log_read();
				param.a0 = 0;
			}

			param.a0 = SMC_CMD_IRQ_RET;
		} while (1);
		wdtee_log_thread_read();

		outer_tz_mutex(NULL);
		if (vaddr)
			iounmap(vaddr);
		WD_DEV_DBG(DEV, "outer cache shared mutex disabled\n");
	}

	mutex_unlock(&ptee->mutex);
	WD_DEV_DBG(DEV,
		"< teetz outer mutex: ret=%d pa=0x%lX va=0x%p %sabled\n",
		ret, paddr, vaddr, ptee->tz_outer_cache_mutex ? "en" : "dis");
	return ret;
}
#endif

/* configure_shm - Negotiate Shared Memory configuration with teetz. */
static int configure_shm(struct tee_tz *ptee)
{
	struct smc_param param = { 0 };
	int ret = 0;

	WD_DEV_DBG(DEV, ">\n");

	mutex_lock(&ptee->mutex);
	param.a0 = TEESMC32_ST_FASTCALL_GET_SHM_CONFIG;

	wdtee_log_thread_read();
	do {
#ifndef SMP_ENABLE
		work_on_cpu(0, (long (*)(void *))tee_smc_call, (void *)&param);
#else
		tee_smc_call(&param);
#endif
		if (param.a0 != SMC_CMD_IRQ)
			break;

		if (param.a1 == 0) {
			wd_log_read();
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 1) {
			if (log_vaddr != NULL) {
				pr_err("wdtee %s\n",
				       (char *)log_vaddr);
			}
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 3) {
			wd_log_read();
			param.a0 = 0;
		}

	} while (1);
	wdtee_log_thread_read();

	mutex_unlock(&ptee->mutex);

	if (param.a0 != TEESMC_RETURN_OK) {
		dev_err(DEV, "shm service not available: %X", (uint) param.a0);
		ret = -EINVAL;
		goto out;
	}

	ptee->shm_paddr = param.a1;
	ptee->shm_size = param.a2;
	ptee->shm_cached = true;

	WD_DEV_DBG(DEV, " shm paddr 0x%p\n", (void *)ptee->shm_paddr);
	WD_DEV_DBG(DEV, " shm size 0x%x\n", (int)ptee->shm_size);
	WD_DEV_DBG(DEV, " shm shm_cached %d\n", ptee->shm_cached);
	if (ptee->shm_cached)
		ptee->shm_vaddr =
		    ioremap_cache(ptee->shm_paddr, ptee->shm_size);
	else
		ptee->shm_vaddr =
		    ioremap_nocache(ptee->shm_paddr, ptee->shm_size);

	dev_err(DEV, "shm ioremap %p\n", ptee->shm_vaddr);
	if (ptee->shm_vaddr == NULL) {
		dev_err(DEV, "shm ioremap failed\n");
		ret = -ENOMEM;
		goto out;
	}

	ptee->shm_pool = tee_shm_pool_create(DEV, ptee->shm_size,
					     ptee->shm_vaddr, ptee->shm_paddr);

	if (!ptee->shm_pool) {
		dev_err(DEV, "shm pool creation failed (%zu)", ptee->shm_size);
		ret = -EINVAL;
		goto out;
	}

	if (ptee->shm_cached)
		tee_shm_pool_set_cached(ptee->shm_pool);
out:
	WD_DEV_DBG(DEV, "< ret=%d pa=0x%lX va=0x%p size=%zu, %scached",
		ret, ptee->shm_paddr, ptee->shm_vaddr, ptee->shm_size,
		(ptee->shm_cached == 1) ? "" : "un");
	return ret;
}

static int tz_start(struct tee *tee)
{
	struct tee_tz *ptee;
	int ret;
	uint32_t a1;
	struct smc_param param = { 0 };

	WD_DEV_DBG(tee->dev, ">\n");
	if (!CAPABLE(tee)) {
		WD_DEV_DBG(tee->dev, "< not capable\n");
		return -EBUSY;
	}
	WD_DEV_DBG(tee->dev, "> smc to tee by null cmd\n");
	wdtee_log_thread_read();
	do {
#ifndef SMP_ENABLE
		work_on_cpu(0, (long (*)(void *))tee_smc_call, (void *)&param);
#else
		tee_smc_call(&param);
#endif
		if (param.a0 != SMC_CMD_IRQ)
			break;
		a1 = param.a1 & 0xffffffff;
		if (a1 == 0) {
			wd_log_read();
			param.a0 = SMC_CMD_IRQ_RET;
			param.a1 = 0;
		} else if (a1 == 1) {
			if (log_vaddr != NULL) {
				pr_err("wdtee %s\n",
				       (char *)log_vaddr);
			}
			param.a0 = SMC_CMD_IRQ_RET;
		} else if (param.a1 == 3) {
			wd_log_read();
			param.a0 = 0;
		}

	} while (1);
	wdtee_log_thread_read();
	WD_DEV_DBG(tee->dev, "< smc return to  ree by null cmd\n");
	ptee = tee->priv;
	ptee->started = true;

#ifdef TUI_ENABLE
	tpd_enter_tui();
	i2c_tui_enable_clock(0);
	display_enter_tui();
#endif
	ret = configure_shm(ptee);
#ifdef TUI_ENABLE
	tpd_exit_tui();
	i2c_tui_disable_clock(0);
	display_exit_tui();
#endif
	if (ret)
		goto exit;

#ifdef CONFIG_OUTER_CACHE
	ret = register_outercache_mutex(ptee, true);
	if (ret)
		goto exit;
#endif

exit:
	if (ret)
		ptee->started = false;

	WD_DEV_DBG(tee->dev, "< ret=%d dev=%s\n", ret, tee->name);
	return ret;
}

static int tz_stop(struct tee *tee)
{
	struct tee_tz *ptee;

	ptee = tee->priv;

	WD_DEV_DBG(tee->dev, "> dev=%s\n", tee->name);
	if (!CAPABLE(tee)) {
		WD_DEV_DBG(tee->dev, "< not capable\n");
		return -EBUSY;
	}
#ifdef CONFIG_OUTER_CACHE
	register_outercache_mutex(ptee, false);
#endif
	tee_shm_pool_destroy(tee->dev, ptee->shm_pool);
	iounmap(ptee->shm_vaddr);

	ptee->started = false;

	WD_DEV_DBG(tee->dev, "< ret=0 dev=%s\n", tee->name);
	return 0;
}

/******************************************************************************/

const struct tee_ops tz_fops = {
	.type = "tz",
	.owner = THIS_MODULE,
	.start = tz_start,
	.stop = tz_stop,
	.invoke = tz_invoke,
	.cancel = tz_cancel,
	.open = tz_open,
	.close = tz_close,
	.alloc = tz_alloc,
	.free = tz_free,
	.shm_inc_ref = tz_shm_inc_ref,
};

static int tz_tee_init(struct platform_device *pdev)
{
	int ret = 0;

	struct tee *tee = platform_get_drvdata(pdev);
	struct tee_tz *ptee = tee->priv;

	tee_tz = ptee;

#if 0
	/* To replace by a syscall */
#ifndef CONFIG_ARM_TZ_SUPPORT
	dev_err(tee->dev,
		"%s: dev=%s, TZ fw is not loaded: TEE TZ is not supported.\n",
		__func__, tee->name);
	tee->conf = TEE_CONF_FW_NOT_CAPABLE;
	return 0;
#endif
#endif

	ptee->started = false;
	ptee->sess_id = 0xAB000000;
	mutex_init(&ptee->mutex);
	init_completion(&ptee->c);
	ptee->c_waiters = 0;

	tee_wait_queue_init(&ptee->wait_queue);
	ret = tee_mutex_wait_init(&ptee->mutex_wait);

	if (ret)
		dev_err(tee->dev, "%s: dev=%s, Secure arm failed (%d)\n",
			__func__, tee->name, ret);
	else
		WD_DEV_DBG(tee->dev, "%s: dev=%s, Secure arm\n",
			__func__, tee->name);
	return ret;
}

static void tz_tee_deinit(struct platform_device *pdev)
{
	struct tee *tee = platform_get_drvdata(pdev);
	struct tee_tz *ptee = tee->priv;

	if (!CAPABLE(tee))
		return;

	tee_mutex_wait_exit(&ptee->mutex_wait);
	tee_wait_queue_exit(&ptee->wait_queue);

	WD_DEV_DBG(tee->dev, "%s: dev=%s, Secure arm started=%d\n", __func__,
		tee->name, ptee->started);
}

static int tz_tee_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct tee *tee;
	struct tee_tz *ptee;
	uint64_t log_paddr = 0;

	pr_info("%s: name=\"%s\", id=%d, pdev_name=\"%s\"\n", __func__,
		pdev->name, pdev->id, dev_name(dev));
#ifdef _TEE_DEBUG
	pr_debug("- dev=%p\n", dev);
	pr_debug("- dev->driver=%p\n", dev->driver);
#endif

	if (of_machine_is_compatible("mediatek,MT6735M")) {
		log_paddr = (unsigned long)0x50100000;
	} else if (of_machine_is_compatible("mediatek,MT6735")) {
		log_paddr = (unsigned long)0x50100000;
	} else if (of_machine_is_compatible("mediatek,MT6750")) {
		log_paddr = (unsigned long)0x70100000;
	} else if (of_machine_is_compatible("mediatek,MT6795")) {
		log_paddr = (unsigned long)0x70100000;
	} else if (of_machine_is_compatible("mediatek,MT6797")) {
		log_paddr = (unsigned long)0x70100000;
	} else if (of_machine_is_compatible("mediatek,MT6763")) {
		log_paddr = (unsigned long)0x70100000;
	} else if (of_machine_is_compatible("mediatek,mt6763")) {
		log_paddr = (unsigned long)0x70100000;
	} else {
		pr_err("map TEE log share buff err: unknown Soc type\n");
		goto bail3;
	}
	log_vaddr = ioremap_nocache(log_paddr, 4096);

	if (log_vaddr != NULL) {
		pr_err("dbg pshm addr ioremap addr %p\n", log_vaddr);
		/* create kernel thread */
		log_kthread(ioremap_cache(log_paddr+4096, 4096));
	} else {
		pr_err("dbg pshm addr ioremap failed\n");
	}

bail3:
	tee = tee_core_alloc(dev, _TEE_TZ_NAME, pdev->id, &tz_fops,
			     sizeof(struct tee_tz));
	if (!tee)
		return -ENOMEM;

	ptee = tee->priv;
	ptee->tee = tee;

	platform_set_drvdata(pdev, tee);

	ret = tz_tee_init(pdev);
	if (ret)
		goto bail0;

	ret = tee_core_add(tee);
	if (ret)
		goto bail1;

#ifdef _TEE_DEBUG
	pr_debug("- tee=%p, id=%d, iminor=%d\n", tee, tee->id,
		 tee->miscdev.minor);
#endif
	return 0;

bail1:
	tz_tee_deinit(pdev);
bail0:
	tee_core_free(tee);
	return ret;
}

static int tz_tee_remove(struct platform_device *pdev)
{
	struct tee *tee = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	/*struct tee_tz *ptee; */

	pr_info("%s: name=\"%s\", id=%d, pdev_name=\"%s\"\n", __func__,
		pdev->name, pdev->id, dev_name(dev));
#ifdef _TEE_DEBUG
	pr_debug("- tee=%p, id=%d, iminor=%d, name=%s\n",
		 tee, tee->id, tee->miscdev.minor, tee->name);
#endif

	tz_tee_deinit(pdev);
	tee_core_del(tee);
	return 0;
}

const struct of_device_id tz_tee_match[] = {
	{
	 .compatible = "watchdata,wdtee",
	 },
	{},
};

static struct platform_driver tz_tee_driver = {
	.probe = tz_tee_probe,
	.remove = tz_tee_remove,
	.driver = {
		   .name = "wdtee",
		   .owner = THIS_MODULE,
		   .of_match_table = tz_tee_match,
		   },
};

static struct platform_device tz_0_plt_device = {
	.name = "wdtee",
	.id = 0,
	.dev = {
/*  .platform_data = tz_0_tee_data, */
		},
};

#ifndef USE_LIB_WDTEE
static int tee_tz_init(void)
#else
int tee_tz_init(void)
#endif
{
	int rc;

	pr_info("watchdata TEE secure Driver initialization\n");

#ifdef _TEE_DEBUG
	pr_debug("- Register the platform_driver \"%s\" %p\n",
		 tz_tee_driver.driver.name, &tz_tee_driver.driver);
#endif

	rc = platform_driver_register(&tz_tee_driver);
	if (rc != 0) {
		pr_err("failed to register the platform driver (rc=%d)\n", rc);
		goto bail0;
	}

	rc = platform_device_register(&tz_0_plt_device);
	if (rc != 0) {
		pr_err("failed to register the platform devices 0 (rc=%d)\n",
		       rc);
		goto bail1;
	}

	return rc;

bail1:
	platform_driver_unregister(&tz_tee_driver);
bail0:
	return rc;
}

#ifndef USE_LIB_WDTEE
static void tee_tz_exit(void)
#else
void tee_tz_exit(void)
#endif
{
	pr_info("watchdata TEE secure Driver de-initialization\n");
	if (log_thread != NULL)
		kthread_stop(log_thread);

	if (log_vaddr)
		iounmap(log_vaddr);

	platform_device_unregister(&tz_0_plt_device);
	platform_driver_unregister(&tz_tee_driver);
}

#ifndef USE_LIB_WDTEE

static int __init wd_tee_tz_init(void)
{
	int rc;

	rc = tee_core_init();
	if (rc)
		return rc;

	rc = tee_tz_init();
#ifdef CONFIG_TEE_PM_WAKE
	wake_lock_init(&wdtee_suspend_lock, WAKE_LOCK_SUSPEND, "TEE wakelock");
#endif

	return rc;
}

static void __exit wd_tee_tz_exit(void)
{
	tee_core_exit();
	tee_tz_exit();
}

module_init(wd_tee_tz_init);
module_exit(wd_tee_tz_exit);

MODULE_AUTHOR("WATCHDATA");
MODULE_DESCRIPTION("WATCHDATA Secure TEE ARM TZ driver");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
#endif
