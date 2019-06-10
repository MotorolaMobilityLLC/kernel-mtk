
#ifndef RSEE_PRIVATE_H
#define RSEE_PRIVATE_H

#include <linux/arm-smccc.h>
#include <linux/semaphore.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include "rsee_msg.h"

#define RSEE_MAX_ARG_SIZE	1024

/* Some Global Platform error codes used in this driver */
#define TEEC_SUCCESS			0x00000000
#define TEEC_ERROR_BAD_PARAMETERS	0xFFFF0006
#define TEEC_ERROR_COMMUNICATION	0xFFFF000E
#define TEEC_ERROR_OUT_OF_MEMORY	0xFFFF000C

#define TEEC_ORIGIN_COMMS		0x00000002

typedef void (rsee_invoke_fn)(unsigned long, unsigned long, unsigned long,
				unsigned long, unsigned long, unsigned long,
				unsigned long, unsigned long,
				struct arm_smccc_res *);

struct rsee_call_queue {
	/* Serializes access to this struct */
	struct mutex mutex;
	struct list_head waiters;
};

struct rsee_wait_queue {
	/* Serializes access to this struct */
	struct mutex mu;
	struct list_head db;
};

/**
 * struct rsee_supp - supplicant synchronization struct
 * @ctx			the context of current connected supplicant.
 *			if !NULL the supplicant device is available for use,
 *			else busy
 * @mutex:		held while accessing content of this struct
 * @req_id:		current request id if supplicant is doing synchronous
 *			communication, else -1
 * @reqs:		queued request not yet retrieved by supplicant
 * @idr:		IDR holding all requests currently being processed
 *			by supplicant
 * @reqs_c:		completion used by supplicant when waiting for a
 *			request to be queued.
 */
struct rsee_supp {
	/* Serializes access to this struct */
	struct mutex mutex;
	struct tee_context *ctx;

	int req_id;
	struct list_head reqs;
	struct idr idr;
	struct completion reqs_c;
};

/**
 * struct rsee - main service struct
 * @supp_teedev:	supplicant device
 * @teedev:		client device
 * @dev:		probed device
 * @invoke_fn:		function to issue smc or hvc
 * @call_queue:		queue of threads waiting to call @invoke_fn
 * @wait_queue:		queue of threads from secure world waiting for a
 *			secure world sync object
 * @supp:		supplicant synchronization struct for RPC to supplicant
 * @pool:		shared memory pool
 * @ioremaped_shm	virtual address of memory in shared memory pool
 */
struct rsee {
	struct tee_device *supp_teedev;
	struct tee_device *teedev;
	struct device *dev;
	rsee_invoke_fn *invoke_fn;
	struct rsee_call_queue call_queue;
	struct rsee_wait_queue wait_queue;
	struct rsee_supp supp;
	struct tee_shm_pool *pool;
	void __iomem *ioremaped_shm;
};

struct rsee_session {
	struct list_head list_node;
	u32 session_id;
};

struct rsee_context_data {
	/* Serializes access to this struct */
	struct mutex mutex;
	struct list_head sess_list;
};

struct rsee_rpc_param {
	u32	a0;
	u32	a1;
	u32	a2;
	u32	a3;
	u32	a4;
	u32	a5;
	u32	a6;
	u32	a7;
};

void rsee_handle_rpc(struct tee_context *ctx, struct rsee_rpc_param *param);

void rsee_wait_queue_init(struct rsee_wait_queue *wq);
void rsee_wait_queue_exit(struct rsee_wait_queue *wq);

u32 rsee_supp_thrd_req(struct tee_context *ctx, u32 func, size_t num_params,
			struct tee_param *param);

int rsee_supp_read(struct tee_context *ctx, void __user *buf, size_t len);
int rsee_supp_write(struct tee_context *ctx, void __user *buf, size_t len);
void rsee_supp_init(struct rsee_supp *supp);
void rsee_supp_uninit(struct rsee_supp *supp);
void rsee_supp_release(struct rsee_supp *supp);

int rsee_supp_recv(struct tee_context *ctx, u32 *func, u32 *num_params,
		    struct tee_param *param);
int rsee_supp_send(struct tee_context *ctx, u32 ret, u32 num_params,
		    struct tee_param *param);

u32 rsee_do_call_with_arg(struct tee_context *ctx, phys_addr_t parg);
int rsee_open_session(struct tee_context *ctx,
		       struct tee_ioctl_open_session_arg *arg,
		       struct tee_param *param);
int rsee_close_session(struct tee_context *ctx, u32 session);
int rsee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg,
		      struct tee_param *param);
int rsee_cancel_req(struct tee_context *ctx, u32 cancel_id, u32 session);

void rsee_enable_shm_cache(struct rsee *rsee);
void rsee_disable_shm_cache(struct rsee *rsee);

int rsee_from_msg_param(struct tee_param *params, size_t num_params,
			 const struct rsee_msg_param *msg_params);
int rsee_to_msg_param(struct rsee_msg_param *msg_params, size_t num_params,
		       const struct tee_param *params);

/*
 * Small helpers
 */

static inline void *reg_pair_to_ptr(u32 reg0, u32 reg1)
{
	return (void *)(unsigned long)(((u64)reg0 << 32) | reg1);
}

static inline void reg_pair_from_64(u32 *reg0, u32 *reg1, u64 val)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

#endif /*RSEE_PRIVATE_H*/
