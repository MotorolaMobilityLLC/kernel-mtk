/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
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

#include<linux/kernel.h>
#include <linux/platform_device.h>
#include<linux/module.h>
#include<linux/types.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/mm.h>
#include<linux/sched.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<asm/io.h>
#include<asm/uaccess.h>
#include<linux/semaphore.h>
#include<linux/slab.h>
#include"TEEI.h"
#include "../tz_driver/include/teei_id.h"

#define VFS_SIZE        0x80000
#define MEM_CLEAR       0x1
#define VFS_MAJOR       253

#define TEEI_CONFIG_IOC_MAGIC 0x775B777E

#define TEEI_CONFIG_IOCTL_INIT_TEEI             _IOWR(TEEI_CONFIG_IOC_MAGIC, 3, int)
#define SOTER_TUI_ENTER                         _IOWR(TEEI_CONFIG_IOC_MAGIC, 0x70, int)
#define SOTER_TUI_LEAVE                         _IOWR(TEEI_CONFIG_IOC_MAGIC, 0x71, int)
#define TEEI_VFS_NOTIFY_DRM			_IOWR(TEEI_CONFIG_IOC_MAGIC, 0x75, int)

static int vfs_major = VFS_MAJOR;
static struct class *driver_class;
static dev_t devno;

struct vfs_dev {
	struct cdev cdev;
	unsigned char mem[VFS_SIZE];
	struct semaphore sem;
};

extern struct completion global_down_lock;

#ifdef MICROTRUST_TUI_DRIVER
extern int display_enter_tui(void);
extern int display_exit_tui(void);
extern int primary_display_trigger(int blocking, void *callback, int need_merge);
extern void mt_deint_leave(void);
extern void mt_deint_restore(void);
extern int tui_i2c_enable_clock(void);
extern int tui_i2c_disable_clock(void);
#endif
extern int tz_sec_drv_notification(unsigned int driver_id);

#ifdef VFS_RDWR_SEM
struct semaphore VFS_rd_sem;
EXPORT_SYMBOL_GPL(VFS_rd_sem);

struct semaphore VFS_wr_sem;
EXPORT_SYMBOL_GPL(VFS_wr_sem);
#else

DECLARE_COMPLETION(VFS_rd_comp);
EXPORT_SYMBOL_GPL(VFS_rd_comp);

DECLARE_COMPLETION(VFS_wr_comp);
EXPORT_SYMBOL_GPL(VFS_wr_comp);
#endif

struct vfs_dev *vfs_devp = NULL;

int tz_vfs_open(struct inode *inode, struct file *filp)
{
	if (vfs_devp == NULL)
		return -EINVAL;

	if (filp == NULL)
		return -EINVAL;

	if (strcmp("teei_daemon", current->comm) != 0)
		return -EINVAL;

	filp->private_data = vfs_devp;
	return 0;
}

int tz_vfs_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static long tz_vfs_ioctl(struct file *filp,
                         unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned int drm_driver_id;

	switch (cmd) {
#ifdef MICROTRUST_TUI_DRIVER
		case SOTER_TUI_ENTER:
			pr_debug("***************SOTER_TUI_ENTER\n");
			ret = tui_i2c_enable_clock();
			if (ret)
				pr_err("tui_i2c_enable_clock failed!!\n");

			mt_deint_leave();

			ret = display_enter_tui();
			if (ret)
				pr_err("display_enter_tui failed!!\n");

			break;

		case SOTER_TUI_LEAVE:
			pr_debug("***************SOTER_TUI_LEAVE\n");
			/*
			ret = tui_i2c_disable_clock();
			if(ret)
			        pr_err("tui_i2c_disable_clock failed!!\n");
			*/
			mt_deint_restore();

			ret = display_exit_tui();
			if (ret)
				pr_err("display_exit_tui failed!!\n");
			/* primary_display_trigger(0, NULL, 0); */

			break;
#endif
		case TEEI_VFS_NOTIFY_DRM:
			pr_debug("***************TEEI_VFS_NOTIFY_DRM\n");
			if (copy_from_user((void *)&drm_driver_id, (void *)arg, sizeof(unsigned int)))
				return -EFAULT;
			pr_debug("get notification from secure driver 0x%x\n", drm_driver_id);

			/* notify drm driver */
			ret = tz_sec_drv_notification(drm_driver_id);
			pr_debug("get notification done 0x%x, ret %d\n", drm_driver_id, ret);
			if (copy_to_user((void *)arg, (void *)&ret, sizeof(unsigned int)))
				return -EFAULT;
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

static ssize_t tz_vfs_read(struct file *filp, char __user *buf,
                           size_t size, loff_t *ppos)
{
	struct TEEI_vfs_command *vfs_p = NULL;
	int length = 0;
	int ret = 0;

	if (buf == NULL)
		return -EINVAL;

	if (daulOS_VFS_share_mem == NULL)
		return -EINVAL;

	if ((0 > size) || (size > VFS_SIZE))
		return -EINVAL;

	/*pr_debug("read begin cpu[%d]\n",cpu_id);*/
#ifdef VFS_RDWR_SEM
	down_interruptible(&VFS_rd_sem);
#else
	ret = wait_for_completion_interruptible(&VFS_rd_comp);

	if (ret == -ERESTARTSYS) {
		pr_debug("[%s][%d] ----------------wait_for_completion_interruptible_timeout interrupt----------------------- \n", __func__, __LINE__);
		complete(&global_down_lock);
		return ret;
	}
#endif

	vfs_p = (struct TEEI_vfs_command *)daulOS_VFS_share_mem;

#if 1

	if (vfs_p->cmd_size > size)
		length = size;
	else
		length = vfs_p->cmd_size;

#endif

	length = size;

	if (copy_to_user(buf, (void *)vfs_p, length))
		ret = -EFAULT;
	else
		ret = length;
	return ret;
}

static ssize_t tz_vfs_write(struct file *filp, const char __user *buf,
                            size_t size, loff_t *ppos)
{
	if (buf == NULL)
		return -EINVAL;

	if (daulOS_VFS_share_mem == NULL)
		return -EINVAL;

	if ((0 > size) || (size > VFS_SIZE))
		return -EINVAL;

	/*pr_debug("write begin cpu_id[%d]\n",cpu_id);*/
	if (copy_from_user((void *)daulOS_VFS_share_mem, buf, size))
		return -EFAULT;

	Flush_Dcache_By_Area((unsigned long)daulOS_VFS_share_mem, (unsigned long)daulOS_VFS_share_mem + size);

#ifdef VFS_RDWR_SEM
	up(&VFS_wr_sem);
#else
	complete(&VFS_wr_comp);
#endif
	return 0;
}

static loff_t tz_vfs_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;

	switch (orig) {
		case 0:
			if (offset < 0) {
				ret = -EINVAL;
				break;
			}

			if ((unsigned int)offset > VFS_SIZE) {
				ret = -EINVAL;
				break;
			}

			filp->f_pos = (unsigned int)offset;
			ret = filp->f_pos;
			break;

		case 1:
			if ((filp->f_pos + offset) > VFS_SIZE) {
				ret = -EINVAL;
				break;
			}

			if ((filp->f_pos + offset) < 0) {
				ret = -EINVAL;
				break;
			}

			filp->f_pos += offset;
			ret = filp->f_pos;
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static const struct file_operations vfs_fops = {
	.owner =                THIS_MODULE,
	.llseek =               tz_vfs_llseek,
	.read =                 tz_vfs_read,
	.write =                tz_vfs_write,
	.unlocked_ioctl = tz_vfs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = tz_vfs_ioctl,
#endif
	.open =                 tz_vfs_open,
	.release =              tz_vfs_release,
};

static void vfs_setup_cdev(struct vfs_dev *dev, int index)
{
	int err = 0;
	int devno = MKDEV(vfs_major, index);

	cdev_init(&dev->cdev, &vfs_fops);
	dev->cdev.owner = vfs_fops.owner;
	err = cdev_add(&dev->cdev, devno, 1);

	if (err)
		pr_err("Error %d adding socket %d.\n", err, index);
}




int vfs_init(void)
{
	int result = 0;
	struct device *class_dev = NULL;
	devno = MKDEV(vfs_major, 0);

	result = alloc_chrdev_region(&devno, 0, 1, "tz_vfs");
	vfs_major = MAJOR(devno);

	if (result < 0)
		return result;

	driver_class = class_create(THIS_MODULE, "tz_vfs");

	if (IS_ERR(driver_class)) {
		result = -ENOMEM;
		pr_err("class_create failed %d.\n", result);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, devno, NULL, "tz_vfs");

	if (!class_dev) {
		result = -ENOMEM;
		pr_err("class_device_create failed %d.\n", result);
		goto class_destroy;
	}

	vfs_devp = kmalloc(sizeof(struct vfs_dev), GFP_KERNEL);

	if (vfs_devp == NULL) {
		result = -ENOMEM;
		goto class_device_destroy;
	}

	memset(vfs_devp, 0, sizeof(struct vfs_dev));
	vfs_setup_cdev(vfs_devp, 0);
	sema_init(&vfs_devp->sem, 1);

#ifdef VFS_RDWR_SEM
	sema_init(&VFS_rd_sem, 0);
	sema_init(&VFS_wr_sem, 0);
#endif
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

void vfs_exit(void)
{
	device_destroy(driver_class, devno);
	class_destroy(driver_class);
	cdev_del(&vfs_devp->cdev);
	kfree(vfs_devp);
	unregister_chrdev_region(MKDEV(vfs_major, 0), 1);
}

MODULE_AUTHOR("MicroTrust");
MODULE_LICENSE("Dual BSD/GPL");

module_param(vfs_major, int, S_IRUGO);

module_init(vfs_init);
module_exit(vfs_exit);
