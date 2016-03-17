
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/irq.h>


#include "teei_common.h"
#include "teei_id.h"
#include "teei_debug.h"
#include "SOCK.h"
#include "TEEI.h"
#include "tz_service.h"

#define VIRTUAL_PRINTER_FULL_PATH_DEV_NAME "/dev/teei_printer"
#define VIRTUAL_PRINTER_DEV "teei_printer"



static struct class *driver_class;
static dev_t virtual_printer_device_no;
static struct cdev virtual_printer_cdev;
static irqreturn_t drivers_interrupt(int irq, void *dummy)
{
#if 0
	/* TDEBUG("31 Interrupt !!\n"); */
	unsigned int p0, p1, p2, p3, p4, p5, p6;
#if 0
	/* Make interrupt pin level to low. */
	*((unsigned int *)(0xfe000000 + 0x1c)) = 31;
#endif
	n_get_param_in(&p0, &p1, &p2, &p3, &p4, &p5, &p6);

	if (p1 == 0xffff) {
		generic_handle_irq(p2);
		/* TDEBUG("input_interrupt %d, %d, %d\n", p2, p3, p4); */
		b_ack_smc_call(0, 0, 0, 0);
	} else {
		/* sys_call_no = p1; */
		pr_info("Set serivce_cmd_flag with sys_call_no %x.\n", sys_call_no);
#ifdef DEBUG
		pr_info("------------------- Add entry to Work queue. ---------------\n");
#endif
		work_ent.call_no = p1;
		INIT_WORK(&(work_ent.work), work_func);
		queue_work(secure_wq, &(work_ent.work));

		serivce_cmd_flag = 1;
	}

#endif
	return IRQ_HANDLED;
}
static int register_interrupt_handler(void)
{
	/**
	 * irq : 100  接收安全端服务调用，该中断号无特殊要求，需要与 /arch/arm/common/gic.c 对应
	 * { ... ;generic_handle_irq(100);...}
	 */
	int irq_no = 101;
	int ret = request_irq(irq_no, drivers_interrupt, 0,
				"virtual_printer_service", (void *)register_interrupt_handler);

	if (ret)
		TERR("ERROR for request_irq %d error code : %d ", irq_no, ret);
	else
		TINFO("request irq [ %d ] OK ", irq_no);

	return 0;
}
/**
 * @brief
 *
 * @param filp
 * @param vma
 *
 * @return
 */
static int virtual_printer_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}



/**
 * @brief
 *
 * @param file
 * @param cmd
 * @param arg
 *
 * @return
 */
static long virtual_printer_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	int ret = -EINVAL;
	void *argp = (void __user *) arg;

	return 0;
}

/**
 * @brief
 *
 * @param inode
 * @param file
 *
 * @return
 */
static int virtual_printer_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief
 *
 * @param inode
 * @param file
 *
 * @return
 */
static int virtual_printer_release(struct inode *inode, struct file *file)
{
	TDEBUG("virtual_printer_release\n");
	return 0;
}


/**
 * @brief
 */
static const struct file_operations virtual_printer_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = virtual_printer_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = virtual_printer_ioctl,
#endif
	.open = virtual_printer_open,
	.mmap = virtual_printer_mmap,
	.release = virtual_printer_release
};


/**
 * @brief TEEI Virtual Printer Driver 初始化
 * @return
 */
static int __init virtual_printer_init(void)
{
	int ret_code = 0;
	struct device *class_dev;
	TINFO("TEEI Virtual Printer Driver Module Init ...");
	ret_code = alloc_chrdev_region(&virtual_printer_device_no, 0, 1, VIRTUAL_PRINTER_DEV);

	if (ret_code < 0) {
		TERR("alloc_chrdev_region failed %d", ret_code);
		return ret_code;
	}

	driver_class = class_create(THIS_MODULE, VIRTUAL_PRINTER_DEV);

	if (IS_ERR(driver_class)) {
		ret_code = -ENOMEM;
		TERR("class_create failed %d", ret_code);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, virtual_printer_device_no, NULL,
				VIRTUAL_PRINTER_DEV);

	if (!class_dev) {
		TERR("class_device_create failed %d", ret_code);
		ret_code = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&virtual_printer_cdev, &virtual_printer_fops);
	virtual_printer_cdev.owner = THIS_MODULE;

	ret_code = cdev_add(&virtual_printer_cdev,
				MKDEV(MAJOR(virtual_printer_device_no), 0), 1);

	if (ret_code < 0) {
		TERR("cdev_add failed %d", ret_code);
		goto class_device_destroy;
	}

	register_interrupt_handler();
	goto return_fn;
class_device_destroy:
	device_destroy(driver_class, virtual_printer_device_no);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(virtual_printer_device_no, 1);
return_fn:
	TINFO("TEEI Agent Driver Module Init ...[OK]");
	return ret_code;
}

/**
 * @brief
 */
static void __exit virtual_printer_exit(void)
{
	TINFO("virtual_printer exit");
	device_destroy(driver_class, virtual_printer_device_no);
	class_destroy(driver_class);
	unregister_chrdev_region(virtual_printer_device_no, 1);
}


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("TEEI <www.microtrust.com>");
MODULE_DESCRIPTION("TEEI Virtual Printer Driver");
MODULE_VERSION("1.00");

module_init(virtual_printer_init);

module_exit(virtual_printer_exit);


