#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "rsee_private.h"
#include "rsee_smc.h"

struct rsee_call_waiter {
	struct list_head list_node;
	struct completion c;
	bool completed;
};

static void rsee_cq_wait_init(struct rsee_call_queue *cq,
			       struct rsee_call_waiter *w)
{
	mutex_lock(&cq->mutex);

	/*
	 * We add ourselves to the queue, but we don't wait. This
	 * guarantees that we don't lose a completion if secure world
	 * returns busy and another thread just exited and try to complete
	 * someone.
	 */
	w->completed = false;
	init_completion(&w->c);
	list_add_tail(&w->list_node, &cq->waiters);

	mutex_unlock(&cq->mutex);
}

static void rsee_cq_wait_for_completion(struct rsee_call_queue *cq,
					 struct rsee_call_waiter *w)
{
	wait_for_completion(&w->c);

	mutex_lock(&cq->mutex);

	/* Move to end of list to get out of the way for other waiters */
	list_del(&w->list_node);
	w->completed = false;
	reinit_completion(&w->c);
	list_add_tail(&w->list_node, &cq->waiters);

	mutex_unlock(&cq->mutex);
}

static void rsee_cq_complete_one(struct rsee_call_queue *cq)
{
	struct rsee_call_waiter *w;

	list_for_each_entry(w, &cq->waiters, list_node) {
		if (!w->completed) {
			complete(&w->c);
			w->completed = true;
			break;
		}
	}
}

static void rsee_cq_wait_final(struct rsee_call_queue *cq,
				struct rsee_call_waiter *w)
{
	mutex_lock(&cq->mutex);

	/* Get out of the list */
	list_del(&w->list_node);

	rsee_cq_complete_one(cq);
	/*
	 * If we're completed we've got a completion that some other task
	 * could have used instead.
	 */
	if (w->completed)
		rsee_cq_complete_one(cq);

	mutex_unlock(&cq->mutex);
}

/* Requires the filpstate mutex to be held */
static struct rsee_session *find_session(struct rsee_context_data *ctxdata,
					  u32 session_id)
{
	struct rsee_session *sess;

	list_for_each_entry(sess, &ctxdata->sess_list, list_node)
		if (sess->session_id == session_id)
			return sess;

	return NULL;
}

/**
 * rsee_do_call_with_arg() - Do an SMC to RSEE in secure world
 * @ctx:	calling context
 * @parg:	physical address of message to pass to secure world
 *
 * Does and SMC to RSEE in secure world and handles eventual resulting
 * Remote Procedure Calls (RPC) from RSEE.
 *
 * Returns return code from secure world, 0 is OK
 */
u32 rsee_do_call_with_arg(struct tee_context *ctx, phys_addr_t parg)
{
	struct rsee *rsee = tee_get_drvdata(ctx->teedev);
	struct rsee_call_waiter w;
	struct rsee_rpc_param param = { };
	u32 ret;

	param.a0 = RSEE_SMC_CALL_WITH_ARG;
	reg_pair_from_64(&param.a1, &param.a2, parg);
	/* Initialize waiter */
	rsee_cq_wait_init(&rsee->call_queue, &w);
	while (true) {
		struct arm_smccc_res res;

		rsee->invoke_fn(param.a0, param.a1, param.a2, param.a3,
				 param.a4, param.a5, param.a6, param.a7,
				 &res);

		if (res.a0 == RSEE_SMC_RETURN_ETHREAD_LIMIT) {
			/*
			 * Out of threads in secure world, wait for a thread
			 * become available.
			 */
			rsee_cq_wait_for_completion(&rsee->call_queue, &w);
		} else if (RSEE_SMC_RETURN_IS_RPC(res.a0)) {
			param.a0 = res.a0;
			param.a1 = res.a1;
			param.a2 = res.a2;
			param.a3 = res.a3;
			rsee_handle_rpc(ctx, &param);
		} else {
			ret = res.a0;
			break;
		}
	}

	/*
	 * We're done with our thread in secure world, if there's any
	 * thread waiters wake up one.
	 */
	rsee_cq_wait_final(&rsee->call_queue, &w);

	return ret;
}

static struct tee_shm *get_msg_arg(struct tee_context *ctx, size_t num_params,
				   struct rsee_msg_arg **msg_arg,
				   phys_addr_t *msg_parg)
{
	int rc;
	struct tee_shm *shm;
	struct rsee_msg_arg *ma;

	shm = tee_shm_alloc(ctx, RSEE_MSG_GET_ARG_SIZE(num_params),
			    TEE_SHM_MAPPED);
	if (IS_ERR(shm))
		return shm;

	ma = tee_shm_get_va(shm, 0);
	if (IS_ERR(ma)) {
		rc = PTR_ERR(ma);
		goto out;
	}

	rc = tee_shm_get_pa(shm, 0, msg_parg);
	if (rc)
		goto out;

	memset(ma, 0, RSEE_MSG_GET_ARG_SIZE(num_params));
	ma->num_params = num_params;
	*msg_arg = ma;
out:
	if (rc) {
		tee_shm_free(shm);
		return ERR_PTR(rc);
	}

	return shm;
}

int rsee_open_session(struct tee_context *ctx,
		       struct tee_ioctl_open_session_arg *arg,
		       struct tee_param *param)
{
	struct rsee_context_data *ctxdata = ctx->data;
	int rc;
	struct tee_shm *shm;
	struct rsee_msg_arg *msg_arg;
	phys_addr_t msg_parg;
	struct rsee_msg_param *msg_param;
	struct rsee_session *sess = NULL;

	/* +2 for the meta parameters added below */
	shm = get_msg_arg(ctx, arg->num_params + 2, &msg_arg, &msg_parg);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	msg_arg->cmd = RSEE_MSG_CMD_OPEN_SESSION;
	msg_arg->cancel_id = arg->cancel_id;
	msg_param = RSEE_MSG_GET_PARAMS(msg_arg);

	/*
	 * Initialize and add the meta parameters needed when opening a
	 * session.
	 */
	msg_param[0].attr = RSEE_MSG_ATTR_TYPE_VALUE_INPUT |
			    RSEE_MSG_ATTR_META;
	msg_param[1].attr = RSEE_MSG_ATTR_TYPE_VALUE_INPUT |
			    RSEE_MSG_ATTR_META;
	memcpy(&msg_param[0].u.value, arg->uuid, sizeof(arg->uuid));
	memcpy(&msg_param[1].u.value, arg->uuid, sizeof(arg->clnt_uuid));
	msg_param[1].u.value.c = arg->clnt_login;

	rc = rsee_to_msg_param(msg_param + 2, arg->num_params, param);
	if (rc)
		goto out;

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess) {
		rc = -ENOMEM;
		goto out;
	}

	if (rsee_do_call_with_arg(ctx, msg_parg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	if (msg_arg->ret == TEEC_SUCCESS) {
		/* A new session has been created, add it to the list. */
		sess->session_id = msg_arg->session;
		mutex_lock(&ctxdata->mutex);
		list_add(&sess->list_node, &ctxdata->sess_list);
		mutex_unlock(&ctxdata->mutex);
	} else {
		kfree(sess);
	}

	if (rsee_from_msg_param(param, arg->num_params, msg_param + 2)) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		/* Close session again to avoid leakage */
		rsee_close_session(ctx, msg_arg->session);
	} else {
		arg->session = msg_arg->session;
		arg->ret = msg_arg->ret;
		arg->ret_origin = msg_arg->ret_origin;
	}
out:
	tee_shm_free(shm);

	return rc;
}

int rsee_close_session(struct tee_context *ctx, u32 session)
{
	struct rsee_context_data *ctxdata = ctx->data;
	struct tee_shm *shm;
	struct rsee_msg_arg *msg_arg;
	phys_addr_t msg_parg;
	struct rsee_session *sess;

	/* Check that the session is valid and remove it from the list */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, session);
	if (sess)
		list_del(&sess->list_node);
	mutex_unlock(&ctxdata->mutex);
	if (!sess)
		return -EINVAL;
	kfree(sess);

	shm = get_msg_arg(ctx, 0, &msg_arg, &msg_parg);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	msg_arg->cmd = RSEE_MSG_CMD_CLOSE_SESSION;
	msg_arg->session = session;
	rsee_do_call_with_arg(ctx, msg_parg);

	tee_shm_free(shm);
	return 0;
}

int rsee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg,
		      struct tee_param *param)
{
	struct rsee_context_data *ctxdata = ctx->data;
	struct tee_shm *shm;
	struct rsee_msg_arg *msg_arg;
	phys_addr_t msg_parg;
	struct rsee_msg_param *msg_param;
	struct rsee_session *sess;
	int rc;

	/* Check that the session is valid */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, arg->session);
	mutex_unlock(&ctxdata->mutex);
	if (!sess)
		return -EINVAL;

	shm = get_msg_arg(ctx, arg->num_params, &msg_arg, &msg_parg);
	if (IS_ERR(shm))
		return PTR_ERR(shm);
	msg_arg->cmd = RSEE_MSG_CMD_INVOKE_COMMAND;
	msg_arg->func = arg->func;
	msg_arg->session = arg->session;
	msg_arg->cancel_id = arg->cancel_id;
	msg_param = RSEE_MSG_GET_PARAMS(msg_arg);

	rc = rsee_to_msg_param(msg_param, arg->num_params, param);
	if (rc)
		goto out;

	if (rsee_do_call_with_arg(ctx, msg_parg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	if (rsee_from_msg_param(param, arg->num_params, msg_param)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	arg->ret = msg_arg->ret;
	arg->ret_origin = msg_arg->ret_origin;
out:
	tee_shm_free(shm);
	return rc;
}

int rsee_cancel_req(struct tee_context *ctx, u32 cancel_id, u32 session)
{
	struct rsee_context_data *ctxdata = ctx->data;
	struct tee_shm *shm;
	struct rsee_msg_arg *msg_arg;
	phys_addr_t msg_parg;
	struct rsee_session *sess;

	/* Check that the session is valid */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, session);
	mutex_unlock(&ctxdata->mutex);
	if (!sess)
		return -EINVAL;

	shm = get_msg_arg(ctx, 0, &msg_arg, &msg_parg);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	msg_arg->cmd = RSEE_MSG_CMD_CANCEL;
	msg_arg->session = session;
	msg_arg->cancel_id = cancel_id;
	rsee_do_call_with_arg(ctx, msg_parg);

	tee_shm_free(shm);
	return 0;
}

/**
 * rsee_enable_shm_cache() - Enables caching of some shared memory allocation
 *			      in RSEE
 * @rsee:	main service struct
 */
void rsee_enable_shm_cache(struct rsee *rsee)
{
	struct rsee_call_waiter w;

	/* We need to retry until secure world isn't busy. */
	rsee_cq_wait_init(&rsee->call_queue, &w);
	while (true) {
		struct arm_smccc_res res;

		rsee->invoke_fn(RSEE_SMC_ENABLE_SHM_CACHE, 0, 0, 0, 0, 0, 0,
				 0, &res);
		if (res.a0 == RSEE_SMC_RETURN_OK)
			break;
		rsee_cq_wait_for_completion(&rsee->call_queue, &w);
	}
	rsee_cq_wait_final(&rsee->call_queue, &w);
}

/**
 * rsee_enable_shm_cache() - Disables caching of some shared memory allocation
 *			      in RSEE
 * @rsee:	main service struct
 */
void rsee_disable_shm_cache(struct rsee *rsee)
{
	struct rsee_call_waiter w;

	/* We need to retry until secure world isn't busy. */
	rsee_cq_wait_init(&rsee->call_queue, &w);
	while (true) {
		union {
			struct arm_smccc_res smccc;
			struct rsee_smc_disable_shm_cache_result result;
		} res;

		rsee->invoke_fn(RSEE_SMC_DISABLE_SHM_CACHE, 0, 0, 0, 0, 0, 0,
				 0, &res.smccc);
		if (res.result.status == RSEE_SMC_RETURN_ENOTAVAIL)
			break; /* All shm's freed */
		if (res.result.status == RSEE_SMC_RETURN_OK) {
			struct tee_shm *shm;

			shm = reg_pair_to_ptr(res.result.shm_upper32,
					      res.result.shm_lower32);
			tee_shm_free(shm);
		} else {
			rsee_cq_wait_for_completion(&rsee->call_queue, &w);
		}
	}
	rsee_cq_wait_final(&rsee->call_queue, &w);
}
