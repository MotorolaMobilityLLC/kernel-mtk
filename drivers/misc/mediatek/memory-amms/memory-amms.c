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

#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>       /* min() */
#include <linux/uaccess.h>      /* copy_to_user() */
#include <linux/sched.h>        /* TASK_INTERRUPTIBLE/signal_pending/schedule */
#include <linux/poll.h>
#include <linux/io.h>           /* ioremap() */
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/seq_file.h>
#include <asm/setup.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/syscore_ops.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#ifdef CONFIG_ARM64
#define MTK_SIP_SMC_AARCH_BIT			0x40000000
static noinline unsigned long long mt_secure_call_ret(u64 function_id,
	u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	unsigned long long ret = 0;

	asm volatile ("smc    #0\n" : "+r" (reg0) :
		"r"(reg1), "r"(reg2), "r"(reg3));

	ret = reg0;
	return ret;
}
/* AMMS related SMC call */
#define MTK_SIP_KERNEL_AMMS_GET_FREE_ADDR (0x82000271 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_AMMS_GET_FREE_LENGTH (0x82000272 | MTK_SIP_SMC_AARCH_BIT)
/* SIP SMC Call 64 */

struct work_struct *amms_work;
bool amms_static_free;
static const struct of_device_id amms_of_ids[] = {
	{ .compatible = "mediatek,amms", },
	{}
};

static irqreturn_t amms_irq_handler(int irq, void *dev_id)
{
	if (amms_work)
		schedule_work(amms_work);
	else
		pr_alert("%s:amms_work is null\n", __func__);
	return IRQ_HANDLED;
}


static void amms_work_handler(struct work_struct *work)
{

	unsigned long long addr = 0, length = 0;

	/*below part is for staic memory free */
	if (!amms_static_free) {
		addr = mt_secure_call_ret(MTK_SIP_KERNEL_AMMS_GET_FREE_ADDR, 0, 0, 0);
		length = mt_secure_call_ret(MTK_SIP_KERNEL_AMMS_GET_FREE_LENGTH, 0, 0, 0);
		if (pfn_valid(__phys_to_pfn(addr)) && pfn_valid(__phys_to_pfn(addr + length - 1))) {
			pr_info("%s:addr = 0x%llx length=0x%llx\n", __func__, addr, length);
			free_reserved_memory(addr, addr+length);
			amms_static_free = true;
		} else {
			pr_alert("AMMS: error addr and length is not set properly\n");
			pr_alert("can not free_reserved_memory\n");
		}
	} else {
		pr_alert("amms: static memory already free, should not happened\n");
	}
}

static int __init amms_probe(struct platform_device *pdev)
{
	int irq_num;

	irq_num = platform_get_irq(pdev, 0);
	if (irq_num == -ENXIO) {
		pr_err("Fail to get amms irq number from device tree\n");
		WARN_ON(irq_num == -ENXIO);
		return -EINVAL;
	}

	pr_debug("amms irq num %d.\n", irq_num);

	if (request_irq(irq_num, (irq_handler_t)amms_irq_handler, IRQF_TRIGGER_NONE, "amms_irq", NULL) != 0) {
		pr_crit("Fail to request amms_irq interrupt!\n");
		return -EBUSY;
	}

	amms_work = kmalloc(sizeof(struct work_struct), GFP_KERNEL);

	if (!amms_work) {
		pr_alert("%s: failed to allocate work queue\n", __func__);
		return -ENOMEM;
	}

	INIT_WORK(amms_work, amms_work_handler);

	return 0;
}

static void __exit amms_exit(void)
{
	kfree(amms_work);
	pr_notice("amms: exited");
}
static int amms_remove(struct platform_device *dev)
{
	return 0;
}

/* variable with __init* or __refdata (see linux/init.h) or */
/* name the variable *_template, *_timer, *_sht, *_ops, *_probe, */
/* *_probe_one, *_console */
static struct platform_driver amms_driver_probe = {
	.probe = amms_probe,
	.remove = amms_remove,
	.driver = {
		.name = "amms",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = amms_of_ids,
#endif
	},
};

static int __init amms_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&amms_driver_probe);
	if (ret)
		pr_err("amms init FAIL, ret 0x%x!!!\n", ret);

	return ret;
}


module_init(amms_init);
module_exit(amms_exit);

MODULE_DESCRIPTION("MEDIATEK Module AMMS Driver");
MODULE_AUTHOR("<chun.fan@mediatek.com>");
#endif
