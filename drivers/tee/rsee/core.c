#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "rsee_private.h"
#include "rsee_smc.h"
#include "rsee_log.h"

#define DRIVER_NAME "rsee"

#define RSEE_SHM_NUM_PRIV_PAGES	1

char rsee_init_data[RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE];

/**
 * rsee_from_msg_param() - convert from RSEE_MSG parameters to
 *			    struct tee_param
 * @params:	subsystem internal parameter representation
 * @num_params:	number of elements in the parameter arrays
 * @msg_params:	RSEE_MSG parameters
 * Returns 0 on success or <0 on failure
 */
int rsee_from_msg_param(struct tee_param *params, size_t num_params,
			 const struct rsee_msg_param *msg_params)
{
	int rc;
	size_t n;
	struct tee_shm *shm;
	phys_addr_t pa;

	for (n = 0; n < num_params; n++) {
		struct tee_param *p = params + n;
		const struct rsee_msg_param *mp = msg_params + n;
		u32 attr = mp->attr & RSEE_MSG_ATTR_TYPE_MASK;

		switch (attr) {
		case RSEE_MSG_ATTR_TYPE_NONE:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
			memset(&p->u, 0, sizeof(p->u));
			break;
		case RSEE_MSG_ATTR_TYPE_VALUE_INPUT:
		case RSEE_MSG_ATTR_TYPE_VALUE_OUTPUT:
		case RSEE_MSG_ATTR_TYPE_VALUE_INOUT:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT +
				  attr - RSEE_MSG_ATTR_TYPE_VALUE_INPUT;
			p->u.value.a = mp->u.value.a;
			p->u.value.b = mp->u.value.b;
			p->u.value.c = mp->u.value.c;
			break;
		case RSEE_MSG_ATTR_TYPE_TMEM_INPUT:
		case RSEE_MSG_ATTR_TYPE_TMEM_OUTPUT:
		case RSEE_MSG_ATTR_TYPE_TMEM_INOUT:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT +
				  attr - RSEE_MSG_ATTR_TYPE_TMEM_INPUT;
			p->u.memref.size = mp->u.tmem.size;
			shm = (struct tee_shm *)(unsigned long)
				mp->u.tmem.shm_ref;
			if (!shm) {
				p->u.memref.shm_offs = 0;
				p->u.memref.shm = NULL;
				break;
			}
			rc = tee_shm_get_pa(shm, 0, &pa);
			if (rc)
				return rc;
			p->u.memref.shm_offs = mp->u.tmem.buf_ptr - pa;
			p->u.memref.shm = shm;

			/* Check that the memref is covered by the shm object */
			if (p->u.memref.size) {
				size_t o = p->u.memref.shm_offs +
					   p->u.memref.size - 1;

				rc = tee_shm_get_pa(shm, o, NULL);
				if (rc && attr == RSEE_MSG_ATTR_TYPE_TMEM_INPUT) //output param may update size larger than original size
                {
					printk("%s %d",__FUNCTION__,__LINE__);
                    return rc;
                }            
					
			}
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}

/**
 * rsee_to_msg_param() - convert from struct tee_params to RSEE_MSG parameters
 * @msg_params:	RSEE_MSG parameters
 * @num_params:	number of elements in the parameter arrays
 * @params:	subsystem itnernal parameter representation
 * Returns 0 on success or <0 on failure
 */
int rsee_to_msg_param(struct rsee_msg_param *msg_params, size_t num_params,
		       const struct tee_param *params)
{
	int rc;
	size_t n;
	phys_addr_t pa;

	for (n = 0; n < num_params; n++) {
		const struct tee_param *p = params + n;
		struct rsee_msg_param *mp = msg_params + n;

		switch (p->attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_NONE:
			mp->attr = TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
			memset(&mp->u, 0, sizeof(mp->u));
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			mp->attr = RSEE_MSG_ATTR_TYPE_VALUE_INPUT + p->attr -
				   TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
			mp->u.value.a = p->u.value.a;
			mp->u.value.b = p->u.value.b;
			mp->u.value.c = p->u.value.c;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			mp->attr = RSEE_MSG_ATTR_TYPE_TMEM_INPUT +
				   p->attr -
				   TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
			mp->u.tmem.shm_ref = (unsigned long)p->u.memref.shm;
			mp->u.tmem.size = p->u.memref.size;
			if (!p->u.memref.shm) {
				mp->u.tmem.buf_ptr = 0;
				break;
			}
			rc = tee_shm_get_pa(p->u.memref.shm,
					    p->u.memref.shm_offs, &pa);
			if (rc)
				return rc;
			mp->u.tmem.buf_ptr = pa;
			mp->attr |= RSEE_MSG_ATTR_CACHE_PREDEFINED <<
					RSEE_MSG_ATTR_CACHE_SHIFT;
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}

static void rsee_get_version(struct tee_device *teedev,
			      struct tee_ioctl_version_data *vers)
{
	struct _v_info
	{
		__u32 model_name_size;
		__u32 v_str_size;
	}*v_info;
	
	struct tee_ioctl_version_data v = {
		.impl_id = TEE_IMPL_ID_RSEE,
		.impl_caps = TEE_RSEE_CAP_TZ,
		.gen_caps = TEE_GEN_CAP_GP,
	};
	*vers = v;
	v_info = (struct _v_info*)&rsee_init_data[0];
	
//	memcpy(&vers->tee_os_vstring[0], rsee_init_data, strlen(rsee_init_data) + 1);
	memcpy(&vers->tee_os_vstring[0], rsee_init_data, sizeof(*v_info) + v_info->model_name_size + v_info->v_str_size);
}

static int rsee_open(struct tee_context *ctx)
{
	struct rsee_context_data *ctxdata;
	struct tee_device *teedev = ctx->teedev;
	struct rsee *rsee = tee_get_drvdata(teedev);

	ctxdata = kzalloc(sizeof(*ctxdata), GFP_KERNEL);
	if (!ctxdata)
		return -ENOMEM;

	if (teedev == rsee->supp_teedev) {
		bool busy = true;

		mutex_lock(&rsee->supp.mutex);
		if (!rsee->supp.ctx) {
			busy = false;
			rsee->supp.ctx = ctx;
		}
		mutex_unlock(&rsee->supp.mutex);
		if (busy) {
			kfree(ctxdata);
			return -EBUSY;
		}
	}

	mutex_init(&ctxdata->mutex);
	INIT_LIST_HEAD(&ctxdata->sess_list);

	ctx->data = ctxdata;
	return 0;
}

static void rsee_release(struct tee_context *ctx)
{
	struct rsee_context_data *ctxdata = ctx->data;
	struct tee_device *teedev = ctx->teedev;
	struct rsee *rsee = tee_get_drvdata(teedev);
	struct tee_shm *shm;
	struct rsee_msg_arg *arg = NULL;
	phys_addr_t parg;
	struct rsee_session *sess;
	struct rsee_session *sess_tmp;

	if (!ctxdata)
		return;

	shm = tee_shm_alloc(ctx, sizeof(struct rsee_msg_arg), TEE_SHM_MAPPED);
	if (!IS_ERR(shm)) {
		arg = tee_shm_get_va(shm, 0);
		/*
		 * If va2pa fails for some reason, we can't call
		 * rsee_close_session(), only free the memory. Secure OS
		 * will leak sessions and finally refuse more sessions, but
		 * we will at least let normal world reclaim its memory.
		 */
		if (!IS_ERR(arg))
			tee_shm_va2pa(shm, arg, &parg);
	}

	list_for_each_entry_safe(sess, sess_tmp, &ctxdata->sess_list,
				 list_node) {
		list_del(&sess->list_node);
		if (!IS_ERR_OR_NULL(arg)) {
			memset(arg, 0, sizeof(*arg));
			arg->cmd = RSEE_MSG_CMD_CLOSE_SESSION;
			arg->session = sess->session_id;
			rsee_do_call_with_arg(ctx, parg);
		}
		kfree(sess);
	}
	kfree(ctxdata);

	if (!IS_ERR(shm))
		tee_shm_free(shm);

	ctx->data = NULL;

	if (teedev == rsee->supp_teedev)
		rsee_supp_release(&rsee->supp);
}

static struct tee_driver_ops rsee_ops = {
	.get_version = rsee_get_version,
	.open = rsee_open,
	.release = rsee_release,
	.open_session = rsee_open_session,
	.close_session = rsee_close_session,
	.invoke_func = rsee_invoke_func,
	.cancel_req = rsee_cancel_req,
};

static struct tee_desc rsee_desc = {
	.name = DRIVER_NAME "-clnt",
	.ops = &rsee_ops,
	.owner = THIS_MODULE,
};

static struct tee_driver_ops rsee_supp_ops = {
	.get_version = rsee_get_version,
	.open = rsee_open,
	.release = rsee_release,
	.supp_recv = rsee_supp_recv,
	.supp_send = rsee_supp_send,
};

static struct tee_desc rsee_supp_desc = {
	.name = DRIVER_NAME "-supp",
	.ops = &rsee_supp_ops,
	.owner = THIS_MODULE,
	.flags = TEE_DESC_PRIVILEGED,
};

static bool rsee_msg_api_uid_is_rsee_api(rsee_invoke_fn *invoke_fn)
{
	struct arm_smccc_res res;

	invoke_fn(RSEE_SMC_CALLS_UID, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 == RSEE_MSG_UID_0 && res.a1 == RSEE_MSG_UID_1 &&
	    res.a2 == RSEE_MSG_UID_2 && res.a3 == RSEE_MSG_UID_3)
		return true;
	return false;
}

static bool rsee_msg_api_revision_is_compatible(rsee_invoke_fn *invoke_fn)
{
	union {
		struct arm_smccc_res smccc;
		struct rsee_smc_calls_revision_result result;
	} res;

	invoke_fn(RSEE_SMC_CALLS_REVISION, 0, 0, 0, 0, 0, 0, 0, &res.smccc);

	if (res.result.major == RSEE_MSG_REVISION_MAJOR &&
	    (int)res.result.minor >= RSEE_MSG_REVISION_MINOR)
		return true;
	return false;
}

static bool rsee_msg_exchange_capabilities(rsee_invoke_fn *invoke_fn,
					    u32 *sec_caps)
{
	union {
		struct arm_smccc_res smccc;
		struct rsee_smc_exchange_capabilities_result result;
	} res;
	u32 a1 = 0;

	/*
	 * TODO This isn't enough to tell if it's UP system (from kernel
	 * point of view) or not, is_smp() returns the the information
	 * needed, but can't be called directly from here.
	 */
#ifndef CONFIG_SMP
	a1 |= RSEE_SMC_NSEC_CAP_UNIPROCESSOR;
#endif

	invoke_fn(RSEE_SMC_EXCHANGE_CAPABILITIES, a1, 0, 0, 0, 0, 0, 0,
		  &res.smccc);

	if (res.result.status != RSEE_SMC_RETURN_OK)
		return false;

	*sec_caps = res.result.capabilities;
	return true;
}

static struct tee_shm_pool *
rsee_config_shm_ioremap(struct device *dev, rsee_invoke_fn *invoke_fn,
			 void __iomem **ioremaped_shm)
{
	union {
		struct arm_smccc_res smccc;
		struct rsee_smc_get_shm_config_result result;
	} res;
	struct tee_shm_pool *pool;
	unsigned long vaddr;
	phys_addr_t paddr;
	size_t size;
	phys_addr_t begin;
	phys_addr_t end;
	void __iomem *va;
	struct tee_shm_pool_mem_info priv_info;
	struct tee_shm_pool_mem_info dmabuf_info;

	invoke_fn(RSEE_SMC_GET_SHM_CONFIG, 0, 0, 0, 0, 0, 0, 0, &res.smccc);
	if (res.result.status != RSEE_SMC_RETURN_OK) {
		dev_info(dev, "shm service not available\n");
		return ERR_PTR(-ENOENT);
	}

	if (res.result.settings != RSEE_SMC_SHM_CACHED) {
		dev_err(dev, "only normal cached shared memory supported\n");
		return ERR_PTR(-EINVAL);
	}

	begin = roundup(res.result.start, PAGE_SIZE);
	end = rounddown(res.result.start + res.result.size, PAGE_SIZE);
    
	paddr = begin;
	size = end - begin;

	if (size < 2 * RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE) {
		dev_err(dev, "too small shared memory area\n");
		return ERR_PTR(-EINVAL);
	}

	va = ioremap_cache(paddr, size);
	if (!va) {
		dev_err(dev, "shared memory ioremap failed\n");
		return ERR_PTR(-EINVAL);
	}
	vaddr = (unsigned long)va;

	memcpy((void*)rsee_init_data, (void*)vaddr, sizeof(rsee_init_data));
	
	priv_info.vaddr = vaddr;
	priv_info.paddr = paddr;
	priv_info.size = RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE;
	dmabuf_info.vaddr = vaddr + RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE;
	dmabuf_info.paddr = paddr + RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE;
	dmabuf_info.size = size - RSEE_SHM_NUM_PRIV_PAGES * PAGE_SIZE;

	pool = tee_shm_pool_alloc_res_mem(&priv_info, &dmabuf_info);
	if (IS_ERR(pool)) {
		iounmap(va);
		goto out;
	}

	*ioremaped_shm = va;
out:
	return pool;
}

static int get_invoke_func(struct device *dev, rsee_invoke_fn **invoke_fn)
{
	struct device_node *np = dev->of_node;
	const char *method;

	dev_info(dev, "probing for conduit method from DT.\n");

	if (of_property_read_string(np, "method", &method)) {
		dev_warn(dev, "missing \"method\" property\n");
		return -ENXIO;
	}

	if (!strcmp("hvc", method)) {
		*invoke_fn = arm_smccc_hvc;
	} else if (!strcmp("smc", method)) {
		*invoke_fn = arm_smccc_smc;
	} else {
		dev_warn(dev, "invalid \"method\" property: %s\n", method);
		return -EINVAL;
	}
	return 0;
}

static int rsee_probe(struct platform_device *pdev)
{
	rsee_invoke_fn *invoke_fn;
	struct tee_shm_pool *pool;
	struct rsee *rsee = NULL;
	void __iomem *ioremaped_shm = NULL;
	struct tee_device *teedev;
	u32 sec_caps;
	int rc;

	rc = get_invoke_func(&pdev->dev, &invoke_fn);
	if (rc)
		return rc;

	if (!rsee_msg_api_uid_is_rsee_api(invoke_fn)) {
		dev_warn(&pdev->dev, "api uid mismatch\n");
		return -EINVAL;
	}

	if (!rsee_msg_api_revision_is_compatible(invoke_fn)) {
		dev_warn(&pdev->dev, "api revision mismatch\n");
		return -EINVAL;
	}

	if (!rsee_msg_exchange_capabilities(invoke_fn, &sec_caps)) {
		dev_warn(&pdev->dev, "capabilities mismatch\n");
		return -EINVAL;
	}

	/*
	 * We have no other option for shared memory, if secure world
	 * doesn't have any reserved memory we can use we can't continue.
	 */
	if (!(sec_caps & RSEE_SMC_SEC_CAP_HAVE_RESERVERED_SHM))
		return -EINVAL;

	pool = rsee_config_shm_ioremap(&pdev->dev, invoke_fn, &ioremaped_shm);
	if (IS_ERR(pool))
		return PTR_ERR(pool);

	rsee = devm_kzalloc(&pdev->dev, sizeof(*rsee), GFP_KERNEL);
	if (!rsee) {
		rc = -ENOMEM;
		goto err;
	}

	rsee->dev = &pdev->dev;
	rsee->invoke_fn = invoke_fn;

	teedev = tee_device_alloc(&rsee_desc, &pdev->dev, pool, rsee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err;
	}
	rsee->teedev = teedev;

	teedev = tee_device_alloc(&rsee_supp_desc, &pdev->dev, pool, rsee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err;
	}
	rsee->supp_teedev = teedev;

	rc = tee_device_register(rsee->teedev);
	if (rc)
		goto err;

	rc = tee_device_register(rsee->supp_teedev);
	if (rc)
		goto err;

	mutex_init(&rsee->call_queue.mutex);
	INIT_LIST_HEAD(&rsee->call_queue.waiters);
	rsee_wait_queue_init(&rsee->wait_queue);
	rsee_supp_init(&rsee->supp);
	rsee->ioremaped_shm = ioremaped_shm;
	rsee->pool = pool;

	platform_set_drvdata(pdev, rsee);

	rsee_enable_shm_cache(rsee);

	tee_log_init();
	
	dev_info(&pdev->dev, "initialized driver\n");
	return 0;
err:
	if (rsee) {
		/*
		 * tee_device_unregister() is safe to call even if the
		 * devices hasn't been registered with
		 * tee_device_register() yet.
		 */
		tee_device_unregister(rsee->supp_teedev);
		tee_device_unregister(rsee->teedev);
	}
	if (pool)
		tee_shm_pool_free(pool);
	if (ioremaped_shm)
		iounmap(ioremaped_shm);
	return rc;
}

static int rsee_remove(struct platform_device *pdev)
{
	struct rsee *rsee = platform_get_drvdata(pdev);

	/*
	 * Ask RSEE to free all cached shared memory objects to decrease
	 * reference counters and also avoid wild pointers in secure world
	 * into the old shared memory range.
	 */
	rsee_disable_shm_cache(rsee);

	/*
	 * The two devices has to be unregistered before we can free the
	 * other resources.
	 */
	tee_device_unregister(rsee->supp_teedev);
	tee_device_unregister(rsee->teedev);

	tee_shm_pool_free(rsee->pool);
	if (rsee->ioremaped_shm)
		iounmap(rsee->ioremaped_shm);
	rsee_wait_queue_exit(&rsee->wait_queue);
	rsee_supp_uninit(&rsee->supp);
	mutex_destroy(&rsee->call_queue.mutex);

	return 0;
}

static const struct of_device_id rsee_match[] = {
	{ .compatible = "rongcard,rsee-tz" },
	{},
};

static struct platform_driver rsee_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = rsee_match,
	},
	.probe = rsee_probe,
	.remove = rsee_remove,
};

static int __init rsee_driver_init(void)
{
	struct device_node *node;

	/*
	 * Preferred path is /firmware/rsee, but it's the matching that
	 * matters.
	 */
	for_each_matching_node(node, rsee_match)
		of_platform_device_create(node, NULL, NULL);

	return platform_driver_register(&rsee_driver);
}

//late_initcall (rsee_driver_init);
subsys_initcall(rsee_driver_init);

static void __exit rsee_driver_exit(void)
{
	platform_driver_unregister(&rsee_driver);
}
module_exit(rsee_driver_exit);

MODULE_AUTHOR("Rongcard");
MODULE_DESCRIPTION("RSEE driver");
MODULE_SUPPORTED_DEVICE("");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
