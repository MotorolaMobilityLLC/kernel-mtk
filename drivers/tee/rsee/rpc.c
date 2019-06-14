#include <linux/device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include "rsee_private.h"
#include "rsee_log.h"
#include "rsee_smc.h"
#include "clk_ctrl.h"

struct wq_entry {
	struct list_head link;
	struct completion c;
	u32 key;
};

void rsee_wait_queue_init(struct rsee_wait_queue *priv)
{
	mutex_init(&priv->mu);
	INIT_LIST_HEAD(&priv->db);
}

void rsee_wait_queue_exit(struct rsee_wait_queue *priv)
{
	mutex_destroy(&priv->mu);
}

static void handle_rpc_func_cmd_get_time(struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params;
	struct timespec64 ts;

	if (arg->num_params != 1)
		goto bad;
	params = RSEE_MSG_GET_PARAMS(arg);
	if ((params->attr & RSEE_MSG_ATTR_TYPE_MASK) !=
			RSEE_MSG_ATTR_TYPE_VALUE_OUTPUT)
		goto bad;

	getnstimeofday64(&ts);
	params->u.value.a = ts.tv_sec;
	params->u.value.b = ts.tv_nsec;

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}


static void handle_rpc_func_cmd_dbg_msg(struct tee_context *ctx,
					  struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params = RSEE_MSG_GET_PARAMS(arg);
	void *va;
	struct tee_shm *shm;
	size_t sz;
	//size_t n;
	dbg_log_header_t *logheader;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	
	if (!arg->num_params ||
	    params->attr != RSEE_MSG_ATTR_TYPE_TMEM_OUTPUT) {
				printk("%s:  1.5 failed, %x %x\n",	__func__, arg->num_params,  (unsigned int)params->attr);
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	sz = params->u.value.b;
	shm = (struct tee_shm *)(size_t)params->u.tmem.shm_ref;
	
	if (IS_ERR(shm)) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}
	
	va = tee_shm_get_va(shm, 0);
	if (IS_ERR(va)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		goto bad;
	}

	logheader = (dbg_log_header_t *)va;
//	printk("%s: 4 %d %d %s, \n",__func__, logheader->msg_type, logheader->msg_len, (char*)(va + sizeof(*logheader)));
	tee_log_add((char*)(va + sizeof(*logheader)),  logheader->msg_len);
	arg->ret = TEEC_SUCCESS;
	return;
bad:
	tee_shm_free(shm);
}


static struct wq_entry *wq_entry_get(struct rsee_wait_queue *wq, u32 key)
{
	struct wq_entry *w;

	mutex_lock(&wq->mu);

	list_for_each_entry(w, &wq->db, link)
		if (w->key == key)
			goto out;

	w = kmalloc(sizeof(*w), GFP_KERNEL);
	if (w) {
		init_completion(&w->c);
		w->key = key;
		list_add_tail(&w->link, &wq->db);
	}
out:
	mutex_unlock(&wq->mu);
	return w;
}

static void wq_sleep(struct rsee_wait_queue *wq, u32 key)
{
	struct wq_entry *w = wq_entry_get(wq, key);

	if (w) {
		wait_for_completion(&w->c);
		mutex_lock(&wq->mu);
		list_del(&w->link);
		mutex_unlock(&wq->mu);
		kfree(w);
	}
}

static void wq_wakeup(struct rsee_wait_queue *wq, u32 key)
{
	struct wq_entry *w = wq_entry_get(wq, key);

	if (w)
		complete(&w->c);
}

static void handle_rpc_func_cmd_wq(struct rsee *rsee,
				   struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params;

	if (arg->num_params != 1)
		goto bad;

	params = RSEE_MSG_GET_PARAMS(arg);
	if ((params->attr & RSEE_MSG_ATTR_TYPE_MASK) !=
			RSEE_MSG_ATTR_TYPE_VALUE_INPUT)
		goto bad;

	switch (params->u.value.a) {
	case RSEE_MSG_RPC_WAIT_QUEUE_SLEEP:
		wq_sleep(&rsee->wait_queue, params->u.value.b);
		break;
	case RSEE_MSG_RPC_WAIT_QUEUE_WAKEUP:
		wq_wakeup(&rsee->wait_queue, params->u.value.b);
		break;
	default:
		goto bad;
	}

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_func_cmd_wait(struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params;
	u32 msec_to_wait;

	if (arg->num_params != 1)
		goto bad;

	params = RSEE_MSG_GET_PARAMS(arg);
	if ((params->attr & RSEE_MSG_ATTR_TYPE_MASK) !=
			RSEE_MSG_ATTR_TYPE_VALUE_INPUT)
		goto bad;

	msec_to_wait = params->u.value.a;

	/* set task's state to interruptible sleep */
	set_current_state(TASK_INTERRUPTIBLE);

	/* take a nap */
	schedule_timeout(msecs_to_jiffies(msec_to_wait));

	arg->ret = TEEC_SUCCESS;
	return;
bad:
	arg->ret = TEEC_ERROR_BAD_PARAMETERS;
}

static void handle_rpc_supp_cmd(struct tee_context *ctx,
				struct rsee_msg_arg *arg)
{
	struct tee_param *params;
	struct rsee_msg_param *msg_params = RSEE_MSG_GET_PARAMS(arg);

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	params = kmalloc_array(arg->num_params, sizeof(struct tee_param),
			       GFP_KERNEL);
	if (!params) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	if (rsee_from_msg_param(params, arg->num_params, msg_params)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	arg->ret = rsee_supp_thrd_req(ctx, arg->cmd, arg->num_params, params);

	if (rsee_to_msg_param(msg_params, arg->num_params, params))
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
out:
	kfree(params);
}

static struct tee_shm *cmd_alloc_suppl(struct tee_context *ctx, size_t sz)
{
	u32 ret;
	struct tee_param param;
	struct rsee *rsee = tee_get_drvdata(ctx->teedev);
	struct tee_shm *shm;

	param.attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.u.value.a = RSEE_MSG_RPC_SHM_TYPE_APPL;
	param.u.value.b = sz;
	param.u.value.c = 0;

	ret = rsee_supp_thrd_req(ctx, RSEE_MSG_RPC_CMD_SHM_ALLOC, 1, &param);
	if (ret)
		return ERR_PTR(-ENOMEM);

	mutex_lock(&rsee->supp.mutex);
	/* Increases count as secure world doesn't have a reference */
	shm = tee_shm_get_from_id(rsee->supp.ctx, param.u.value.c);
	mutex_unlock(&rsee->supp.mutex);
	return shm;
}

static void handle_rpc_func_cmd_shm_alloc(struct tee_context *ctx,
					  struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params = RSEE_MSG_GET_PARAMS(arg);
	phys_addr_t pa;
	struct tee_shm *shm;
	size_t sz;
	size_t n;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (!arg->num_params ||
	    params->attr != RSEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	for (n = 1; n < arg->num_params; n++) {
		if (params[n].attr != RSEE_MSG_ATTR_TYPE_NONE) {
			arg->ret = TEEC_ERROR_BAD_PARAMETERS;
			return;
		}
	}

	sz = params->u.value.b;
	switch (params->u.value.a) {
	case RSEE_MSG_RPC_SHM_TYPE_APPL:
		shm = cmd_alloc_suppl(ctx, sz);
		break;
	case RSEE_MSG_RPC_SHM_TYPE_KERNEL:
		shm = tee_shm_alloc(ctx, sz, TEE_SHM_MAPPED);
		break;
	default:
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	if (IS_ERR(shm)) {
		arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		return;
	}

	if (tee_shm_get_pa(shm, 0, &pa)) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		goto bad;
	}

	params[0].attr = RSEE_MSG_ATTR_TYPE_TMEM_OUTPUT;
	params[0].u.tmem.buf_ptr = pa;
	params[0].u.tmem.size = sz;
	params[0].u.tmem.shm_ref = (unsigned long)shm;
	arg->ret = TEEC_SUCCESS;
	return;
bad:
	tee_shm_free(shm);
}

static void cmd_free_suppl(struct tee_context *ctx, struct tee_shm *shm)
{
	struct tee_param param;

	param.attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param.u.value.a = RSEE_MSG_RPC_SHM_TYPE_APPL;
	param.u.value.b = tee_shm_get_id(shm);
	param.u.value.c = 0;

	/*
	 * Match the tee_shm_get_from_id() in cmd_alloc_suppl() as secure
	 * world has released its reference.
	 *
	 * It's better to do this before sending the request to supplicant
	 * as we'd like to let the process doing the initial allocation to
	 * do release the last reference too in order to avoid stacking
	 * many pending fput() on the client process. This could otherwise
	 * happen if secure world does many allocate and free in a single
	 * invoke.
	 */
	tee_shm_put(shm);

	rsee_supp_thrd_req(ctx, RSEE_MSG_RPC_CMD_SHM_FREE, 1, &param);
}

static void handle_rpc_func_cmd_shm_free(struct tee_context *ctx,
					 struct rsee_msg_arg *arg)
{
	struct rsee_msg_param *params = RSEE_MSG_GET_PARAMS(arg);
	struct tee_shm *shm;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (arg->num_params != 1 ||
	    params->attr != RSEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	shm = (struct tee_shm *)(unsigned long)params->u.value.b;
	switch (params->u.value.a) {
	case RSEE_MSG_RPC_SHM_TYPE_APPL:
		cmd_free_suppl(ctx, shm);
		break;
	case RSEE_MSG_RPC_SHM_TYPE_KERNEL:
		tee_shm_free(shm);
		break;
	default:
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
	}
	arg->ret = TEEC_SUCCESS;
}

static void handle_rpc_func_cmd(struct tee_context *ctx, struct rsee *rsee,
				struct tee_shm *shm)
{
	struct rsee_msg_arg *arg;

	arg = tee_shm_get_va(shm, 0);
	if (IS_ERR(arg)) {
		dev_err(rsee->dev, "%s: tee_shm_get_va %p failed\n",
			__func__, shm);
		return;
	}

	switch (arg->cmd) {
	case RSEE_MSG_RPC_CMD_GET_TIME:
		handle_rpc_func_cmd_get_time(arg);
		break;
	case RSEE_MSG_RPC_CMD_WAIT_QUEUE:
		handle_rpc_func_cmd_wq(rsee, arg);
		break;
	case RSEE_MSG_RPC_CMD_SUSPEND:
		handle_rpc_func_cmd_wait(arg);
		break;
	case RSEE_MSG_RPC_CMD_SHM_ALLOC:
		handle_rpc_func_cmd_shm_alloc(ctx, arg);
		break;
	case RSEE_MSG_RPC_CMD_SHM_FREE:
		handle_rpc_func_cmd_shm_free(ctx, arg);
		break;
	case RSEE_MSG_RPC_CMD_SEC_SPI_CLK_ENABLE:
		arg->ret = rsee_spi_clk_enable();
		break;
	case RSEE_MSG_RPC_CMD_SEC_SPI_CLK_DISABLE:
		arg->ret = rsee_spi_clk_disable();
		break;
	case RSEE_MSG_RPC_CMD_SEND_DEBUG_INFO:
		///printk("%s: RSEE_MSG_RPC_CMD_SEND_DEBUG_INFO\n", __func__);
		handle_rpc_func_cmd_dbg_msg(ctx, arg);
		arg->ret = TEEC_SUCCESS;
		break;
	default:
		handle_rpc_supp_cmd(ctx, arg);
	}
}

/**
 * rsee_handle_rpc() - handle RPC from secure world
 * @ctx:	context doing the RPC
 * @param:	value of registers for the RPC
 *
 * Result of RPC is written back into @param.
 */
void rsee_handle_rpc(struct tee_context *ctx, struct rsee_rpc_param *param)
{
	struct tee_device *teedev = ctx->teedev;
	struct rsee *rsee = tee_get_drvdata(teedev);
	struct tee_shm *shm;
	phys_addr_t pa;

	switch (RSEE_SMC_RETURN_GET_RPC_FUNC(param->a0)) {
	case RSEE_SMC_RPC_FUNC_ALLOC:
		shm = tee_shm_alloc(ctx, param->a1, TEE_SHM_MAPPED);
		if (!IS_ERR(shm) && !tee_shm_get_pa(shm, 0, &pa)) {
			reg_pair_from_64(&param->a1, &param->a2, pa);
			reg_pair_from_64(&param->a4, &param->a5,
					 (unsigned long)shm);
		} else {
			param->a1 = 0;
			param->a2 = 0;
			param->a4 = 0;
			param->a5 = 0;
		}
		break;
	case RSEE_SMC_RPC_FUNC_FREE:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		tee_shm_free(shm);
		break;
	case RSEE_SMC_RPC_FUNC_IRQ:
		/*
		 * An IRQ was raised while secure world was executing,
		 * since all IRQs are handled in Linux a dummy RPC is
		 * performed to let Linux take the IRQ through the normal
		 * vector.
		 */
		break;
	case RSEE_SMC_RPC_FUNC_CMD:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		handle_rpc_func_cmd(ctx, rsee, shm);
		break;
	default:
		dev_warn(rsee->dev, "Unknown RPC func 0x%x\n",
			 (u32)RSEE_SMC_RETURN_GET_RPC_FUNC(param->a0));
		break;
	}

	param->a0 = RSEE_SMC_CALL_RETURN_FROM_RPC;
}
