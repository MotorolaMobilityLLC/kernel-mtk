/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include "fp_func.h"
#include "../tz_driver/include/teei_fp.h"
#include "../tz_driver/include/teei_id.h"
#include "../tz_driver/include/tz_service.h"
#include "../tz_driver/include/nt_smc_call.h"
#include "../tz_driver/include/utdriver_macro.h"
#include "../tz_driver/include/teei_client_main.h"
#include "../tz_driver/include/teei_gatekeeper.h"
#include <imsg_log.h>

/*#define FP_DEBUG*/
struct fp_dev {
	struct cdev cdev;
	unsigned char mem[MICROTRUST_FP_SIZE];
	struct semaphore sem;
};

static int fp_major = FP_MAJOR;
static dev_t devno;
static int wait_teei_config_flag_count;

static struct class *driver_class;
struct semaphore fp_api_lock;
struct fp_dev *fp_devp;
struct semaphore daulOS_rd_sem;
struct semaphore daulOS_wr_sem;
/**
*EXPORT_SYMBOL_GPL(daulOS_rd_sem);
*EXPORT_SYMBOL_GPL(daulOS_wr_sem);
*/
DECLARE_WAIT_QUEUE_HEAD(__fp_open_wq);
int fp_open(struct inode *inode, struct file *filp)
{
	int ret = -1;

	if (wait_teei_config_flag_count != 1 && teei_config_flag == 0) {
		ret = wait_event_timeout(__fp_open_wq, (teei_config_flag == 1),
						msecs_to_jiffies(1000*20));
		IMSG_ERROR("[TEE] open wait for %u msecs in first time\n",
						(1000*20-jiffies_to_msecs(ret)));
	}

	wait_teei_config_flag_count = 1;
	ret = wait_event_timeout(__fp_open_wq, (teei_config_flag == 1),
						msecs_to_jiffies(1000*20));

	if (ret <= 0) {
		IMSG_INFO("[TEE] error, tee load not finished yet,and wait timeout\n");
		return -1;
	}

	filp->private_data = fp_devp;

	return 0;
}

int fp_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static long fp_ioctl(struct file *filp, unsigned cmd, unsigned long arg)
{
	unsigned int args_len = 0;
	unsigned int fp_cid = 0xFF;
	unsigned int fp_fid = 0xFF;
	unsigned char args[16] = {0};
	unsigned int buff_len = 0;

	if (_IOC_TYPE(cmd) != TEEI_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > TEEI_IOC_MAXNR)
		return -ENOTTY;
	down(&fp_api_lock);
#ifdef FP_DEBUG
	IMSG_DEBUG("##################################\n");
	IMSG_DEBUG("fp ioctl received received cmd is: %x arg is %x\n",
					cmd, (unsigned int)arg);
	IMSG_DEBUG("CMD_MEM_CLEAR is: %x CMD_FP_CMD is %x\n",
				CMD_MEM_CLEAR, CMD_FP_CMD);
#endif
	switch (cmd) {
	case CMD_MEM_CLEAR:
		IMSG_INFO("CMD MEM CLEAR.\n");
		break;
	case CMD_FP_CMD:
		if (copy_from_user((void *)args, (void *)arg, FP_BUFFER_OFFSET)) {
			IMSG_ERROR("copy args from user failed.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
		/* TODO compute args length */
		/* [11-15] is the length of data */
		args_len = *((unsigned int *)(args + 12));
		if (args_len > FP_LEN_MAX || (args_len <= FP_LEN_MIN)) {
			IMSG_ERROR("args_len is invalid %d !\n", args_len);
			up(&fp_api_lock);
			return -EFAULT;
		}
		buff_len = args_len + FP_BUFFER_OFFSET;
		/* [0-3] is cmd id */
		fp_cid = *((unsigned int *)(args));
		/* [4-7] is function id */
		fp_fid = *((unsigned int *)(args + 4));
#ifdef FP_DEBUG
		IMSG_DEBUG("invoke fp cmd CMD_FP_CMD: arg's address is %x, args's length %d\n",
					(unsigned int)arg, args_len);
		IMSG_DEBUG("invoke fp cmd fp_cid is %d fp_fid is %d\n", fp_cid, fp_fid);
#endif
		if (!fp_buff_addr) {
			IMSG_ERROR("fp_buiff_addr is invalid!.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
		memset((void *)fp_buff_addr, 0, buff_len);
		if (copy_from_user((void *)fp_buff_addr, (void *)arg, buff_len)) {
			IMSG_ERROR("copy from user failed.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}

		Flush_Dcache_By_Area((unsigned long)fp_buff_addr, (unsigned long)fp_buff_addr + MICROTRUST_FP_SIZE);
		/*send command data to TEEI*/
		send_fp_command(FP_DRIVER_ID);
#ifdef FP_DEBUG
		IMSG_DEBUG("back from TEEI try copy share mem to user\n");
		IMSG_DEBUG("result in share memory %d\n", *((unsigned int *)fp_buff_addr));
		IMSG_DEBUG("[%s][%d] fp_buff_addr 88 - 91 = %d\n",
					__func__, args_len, *((unsigned int *)(fp_buff_addr + 88)));
#endif
		if (copy_to_user((void *)arg, (void *)fp_buff_addr, buff_len)) {
			IMSG_ERROR("copy from user failed.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
#ifdef FP_DEBUG
		IMSG_DEBUG("result after copy %d\n", *((unsigned int *)arg));
		IMSG_DEBUG("invoke fp cmd end.\n");
#endif
		break;
	case CMD_GATEKEEPER_CMD:
		#ifdef FP_DEBUG
		IMSG_DEBUG("case CMD_GATEKEEPER_CMD\n");
		#endif
		if (!gatekeeper_buff_addr) {
			IMSG_ERROR("gatekeeper_buff_addr is invalid!.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
#ifdef FP_DEBUG
		IMSG_DEBUG("varify gatekeeper_buff_addr  ok\n");
		IMSG_DEBUG("the value of gatekeeper_buff_addr is %lu\n", gatekeeper_buff_addr);
#endif
		memset((void *)gatekeeper_buff_addr, 0, 0x1000);
#ifdef FP_DEBUG
		IMSG_DEBUG("memset  ok\n");
#endif
		if (copy_from_user((void *)gatekeeper_buff_addr, (void *)arg, 0x1000)) {
			IMSG_ERROR("copy from user failed.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
#ifdef FP_DEBUG
		IMSG_DEBUG("copy_from_user  ok\n");
#endif
		Flush_Dcache_By_Area((unsigned long)gatekeeper_buff_addr, (unsigned long)gatekeeper_buff_addr + 0x1000);
#ifdef FP_DEBUG
		IMSG_DEBUG("Flush_Dcache_By_Area  ok\n");
#endif
		send_gatekeeper_command(GK_DRIVER_ID);
#ifdef FP_DEBUG
		IMSG_DEBUG("send_gatekeeper_command  ok\n");
#endif
		if (copy_to_user((void *)arg, (void *)gatekeeper_buff_addr, 0x1000)) {
			IMSG_ERROR("copy from user failed.\n");
			up(&fp_api_lock);
			return -EFAULT;
		}
#ifdef FP_DEBUG
		IMSG_DEBUG("copy_to_user  ok\n");
#endif
		break;
	case CMD_LOAD_TEE:
#ifdef FP_DEBUG
		IMSG_DEBUG("case CMD_LOAD_TEE\n");
#endif
		up(&boot_decryto_lock);
		break;
	default:
		up(&fp_api_lock);
		return -EINVAL;
	}
	up(&fp_api_lock);
	return 0;
}

static ssize_t fp_read(struct file *filp, char __user *buf,
		size_t size, loff_t *ppos)
{
	int ret = 0;
	return ret;
}

static ssize_t fp_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static loff_t fp_llseek(struct file *filp, loff_t offset, int orig)
{
	return 0;
}

static const struct file_operations fp_fops = {
	.owner = THIS_MODULE,
	.llseek = fp_llseek,
	.read = fp_read,
	.write = fp_write,
	.unlocked_ioctl = fp_ioctl,
	.compat_ioctl = fp_ioctl,
	.open = fp_open,
	.release = fp_release,
};

static void fp_setup_cdev(struct fp_dev *dev, int index)
{
	int err = 0;
	int devno = MKDEV(fp_major, index);

	cdev_init(&dev->cdev, &fp_fops);
	dev->cdev.owner = fp_fops.owner;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		IMSG_ERROR("Error %d adding fp %d.\n", err, index);

}

int fp_init(void)
{
	int result = 0;

	struct device *class_dev = NULL;

	devno = MKDEV(fp_major, 0);
	result = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
	fp_major = MAJOR(devno);
	sema_init(&(fp_api_lock), 1);

	if (result < 0)
		return result;

	driver_class = NULL;
	driver_class = class_create(THIS_MODULE, DEV_NAME);

	if (IS_ERR(driver_class)) {
		result = -ENOMEM;
		IMSG_ERROR("class_create failed %d.\n", result);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, devno, NULL, DEV_NAME);

	if (!class_dev) {
		result = -ENOMEM;
		IMSG_ERROR("class_device_create failed %d.\n", result);
		goto class_destroy;
	}

	fp_devp = NULL;
	fp_devp = kmalloc(sizeof(struct fp_dev), GFP_KERNEL);

	if (fp_devp == NULL) {
		result = -ENOMEM;
		goto class_device_destroy;
	}

	memset(fp_devp, 0, sizeof(struct fp_dev));
	fp_setup_cdev(fp_devp, 0);
	sema_init(&fp_devp->sem, 1);
	sema_init(&daulOS_rd_sem, 1);
	sema_init(&daulOS_wr_sem, 1);

	IMSG_DEBUG("[%s][%d]create the teei_fp device node successfully!\n", __func__, __LINE__);
	goto return_fn;

class_device_destroy:
	device_destroy(driver_class, devno);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(devno, 1);
return_fn:
	return result;
}

void fp_exit(void)
{
	device_destroy(driver_class, devno);
	class_destroy(driver_class);
	cdev_del(&fp_devp->cdev);
	kfree(fp_devp);
	unregister_chrdev_region(MKDEV(fp_major, 0), 1);
}

MODULE_AUTHOR("Microtrust");
MODULE_LICENSE("Dual BSD/GPL");

module_param(fp_major, int, S_IRUGO);

module_init(fp_init);
module_exit(fp_exit);
