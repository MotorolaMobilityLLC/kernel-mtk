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


#include "geofence.h"

#define GEOFENCE_DEVNAME "mtk_geofence"

struct geofence_dev {
	struct class *cls;
	struct device *dev;
	dev_t devno;
	struct cdev chdev;
};

static struct semaphore wr_mtx, rd_mtx;
struct geofence_dev *devobj;
static int geofence_result;
static int geofence_source;
static int geofence_op_mode;
static int geofence_wait_status;
static DECLARE_WAIT_QUEUE_HEAD(geofence_wait);
static DEFINE_SPINLOCK(geofence_lock);
static int geofence_enable_data(int enable)
{

	if (enable == 1) {
		GEOFENCE_LOG("geofence enable\n");
		sensor_batch_to_hub(ID_GEOFENCE, 0, 0, 0);
		sensor_enable_to_hub(ID_GEOFENCE, enable);
	}
	if (enable == 0) {
		GEOFENCE_LOG("geofence disable\n");
		sensor_enable_to_hub(ID_GEOFENCE, enable);

	}
	return 0;
}

static int geofence_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t geofence_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	int ret = 0;
	struct geofence_database geofence_data = {0};
	/* this is process context, use spin_lock_irq */
	spin_lock_irq(&geofence_lock);
	geofence_data.source = geofence_source;
	geofence_data.result = geofence_result;
	geofence_data.mode = geofence_op_mode;
	geofence_wait_status = 0;
	spin_unlock_irq(&geofence_lock);
	GEOFENCE_LOG("IOCTL_GEOFENCE_GET_DATA, %d, %d, %d\n", geofence_data.source,
		geofence_data.result, geofence_data.mode);
	ret = copy_to_user(buffer, &geofence_data, sizeof(geofence_data));
	if (ret != 0)
		ret = -EFAULT;
	else
		ret = sizeof(geofence_data);
	return ret;
}

ssize_t geofence_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned char o_buf[64];
	int retval = 0, ret = 0;

	down(&wr_mtx);

	if (count > 0) {
		if (copy_from_user(&o_buf[0], &buf[0], count)) {
			retval = -EFAULT;
			up(&wr_mtx);
			return retval;
		}
		GEOFENCE_LOG("rcv geofence write %zu %s\n", count, o_buf);
		retval = count;

		if ((o_buf[0] == 't') && (o_buf[1] == 'e') && (o_buf[2] == 's')
			&& (o_buf[3] == 't') && (o_buf[4] == '1')) {
			GEOFENCE_LOG("enable geofence test\n");
			ret = geofence_enable_data(true);
			GEOFENCE_LOG("IOCTL_GEOFENCE_ENABLE =%d\n", ret);
		} else if ((o_buf[0] == 't') && (o_buf[1] == 'e') && (o_buf[2] == 's')
			&& (o_buf[3] == 't') && (o_buf[4] == '0')) {
			GEOFENCE_LOG("disable geofence test\n");
			ret = geofence_enable_data(false);
			GEOFENCE_LOG("IOCTL_GEOFENCE_DISABLE =%d\n", ret);
		}
	}
	up(&wr_mtx);
	return retval;
}

static unsigned int geofence_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &geofence_wait, wait);
	if (geofence_wait_status)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

long geofence_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	struct geofence_database geofence_data = {0};

	GEOFENCE_LOG("cmd (%d),arg(%ld)\n", cmd, arg);

	switch (cmd) {
	case IOCTL_GEOFENCE_ENABLE:
		retval = geofence_enable_data(true);
		GEOFENCE_LOG("IOCTL_GEOFENCE_ENABLE =%d\n", retval);
		break;

	case IOCTL_GEOFENCE_GET_DATA:
		/* this is process context, use spin_lock_irq */
		spin_lock_irq(&geofence_lock);
		geofence_data.source = geofence_source;
		geofence_data.result = geofence_result;
		geofence_data.mode = geofence_op_mode;
		geofence_wait_status = 0;
		spin_unlock_irq(&geofence_lock);
		GEOFENCE_LOG("IOCTL_GEOFENCE_GET_DATA, %d, %d, %d\n", geofence_data.source,
		    geofence_data.result, geofence_data.mode);
		if (copy_to_user((int __user *)arg, &geofence_data, sizeof(geofence_data)))
			retval = -EFAULT;
		break;

	case IOCTL_GEOFENCE_INJECT_CMD:
		GEOFENCE_LOG("IOCTL_GEOFENCE_INJECT_CMD currently not support\n");
		break;

	case IOCTL_GEOFENCE_DISABLE:
		retval = geofence_enable_data(false);
		GEOFENCE_LOG("IOCTL_GEOFENCE_DISABLE =%d\n", retval);
		break;

	default:
		GEOFENCE_LOG("unknown cmd (%d)\n", cmd);
		retval = 0;
		break;
	}
	return retval;
}

static long geofence_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	void __user *arg32 = compat_ptr(arg);

	GEOFENCE_LOG("compat cmd (%x)\n", cmd);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	GEOFENCE_LOG("compat cmd (%x) OK\n", cmd);

	ret = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
	if (ret) {
		GEOFENCE_LOG("compat cmd %x error\n", cmd);
		return ret;
	}
	GEOFENCE_LOG("compat cmd %x OK\n", cmd);

	return ret;
}

static const struct file_operations geofence_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = geofence_unlocked_ioctl,
	.compat_ioctl = geofence_compat_ioctl,
	.open = geofence_open,
	.read = geofence_read,
	.write = geofence_write,
	.poll = geofence_poll,
};

static int geofence_recv_data(struct data_unit_t *event, void *reserved)
{
	int err = 0;
	/* this is interrupt context, use spin_lock */
	spin_lock(&geofence_lock);
	geofence_source = ((event->geofence_data_t.state) & 0xF00) >> 8;
	geofence_result = ((event->geofence_data_t.state) & 0xF0) >> 4;
	geofence_op_mode = (event->geofence_data_t.state) & 0xF;
	geofence_wait_status = 1;
	spin_unlock(&geofence_lock);
	GEOFENCE_LOG("geofence_recv_data: %d, %d, %d\n", geofence_source, geofence_result, geofence_op_mode);
	wake_up(&geofence_wait);
	return err;
}
static int geofence_probe(void)
{
	int ret = 0, err = 0;

	GEOFENCE_LOG("geofence_probe\n");
	devobj = kzalloc(sizeof(*devobj), GFP_KERNEL);
	if (devobj == NULL) {
		ret = -ENOMEM;
		err = -ENOMEM;
		goto err_out;
	}
	sema_init(&wr_mtx, 1);
	sema_init(&rd_mtx, 1);
	GEOFENCE_LOG("alloc devobj\n");

	ret = alloc_chrdev_region(&devobj->devno, 0, 1, GEOFENCE_DEVNAME);
	if (ret) {
		GEOFENCE_ERR("alloc_chrdev_region fail: %d\n", err);
		kfree(devobj);
		err = -ENOMEM;
		goto err_out;
	} else {
		GEOFENCE_LOG("major: %d, minor: %d\n", MAJOR(devobj->devno), MINOR(devobj->devno));
	}
	cdev_init(&devobj->chdev, &geofence_fops);
	devobj->chdev.owner = THIS_MODULE;
	err = cdev_add(&devobj->chdev, devobj->devno, 1);
	if (err) {
		GEOFENCE_ERR("cdev_add fail: %d\n", ret);
		kfree(devobj);
		goto err_out;
	}
	devobj->cls = class_create(THIS_MODULE, "geofence");
	if (IS_ERR(devobj->cls)) {
		GEOFENCE_ERR("Unable to create class, err = %d\n", (int)PTR_ERR(devobj->cls));
		kfree(devobj);
		goto err_out;
	}
	devobj->dev = device_create(devobj->cls, NULL, devobj->devno, devobj, "geofence");
	err = scp_sensorHub_data_registration(ID_GEOFENCE, geofence_recv_data);
	if (err < 0) {
		GEOFENCE_ERR("scp_sensorHub_data_registration failed\n");
		goto err_out;
	}
	GEOFENCE_LOG("----geofence_probe OK !!\n");
	return 0;

err_out:
	if (err == 0) {
		cdev_del(&devobj->chdev);
		GEOFENCE_ERR("delete dev\n");
	}
	if (ret == 0) {
		unregister_chrdev_region(devobj->devno, 1);
		GEOFENCE_ERR("unregister dev\n");
	}
	return -1;
}

static int geofence_remove(void)
{
	GEOFENCE_FUN(f);
	if (!devobj) {
		GEOFENCE_ERR("null pointer: %p\n", devobj);
		return -1;
	}

	GEOFENCE_LOG("Unregistering chardev\n");
	cdev_del(&devobj->chdev);
	unregister_chrdev_region(devobj->devno, 1);
	device_destroy(devobj->cls, devobj->devno);
	class_destroy(devobj->cls);
	kfree(devobj);
	GEOFENCE_LOG("geofence remove done\n");
	return 0;
}
static int __init geofence_init(void)
{
	if (geofence_probe()) {
		GEOFENCE_ERR("failed to register geofence driver\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit geofence_exit(void)
{
	geofence_remove();
}

late_initcall(geofence_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("geofence device driver");
MODULE_AUTHOR("Mediatek");
