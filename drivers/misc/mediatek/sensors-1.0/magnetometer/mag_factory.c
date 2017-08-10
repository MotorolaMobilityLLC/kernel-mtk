/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include "inc/mag_factory.h"

struct mag_factory_private {
	uint32_t gain;
	uint32_t sensitivity;
	struct mag_factory_fops *fops;
};

static struct mag_factory_private mag_factory;

static int mag_factory_open(struct inode *inode, struct file *file)
{
	file->private_data = mag_context_obj;

	if (file->private_data == NULL) {
		MAG_PR_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int mag_factory_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long mag_factory_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *ptr = (void __user *)arg;
	int err = 0, status = 0;
	uint32_t flag = 0;
	char strbuf[64];
	int32_t data_buf[3] = {0};

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		MAG_PR_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case MSENSOR_IOCTL_SENSOR_ENABLE:
			if (copy_from_user(&flag, ptr, sizeof(flag)))
				return -EFAULT;
			if (mag_factory.fops != NULL && mag_factory.fops->enable_sensor != NULL) {
				err = mag_factory.fops->enable_sensor(flag, 20);
			if (err < 0) {
					MAG_LOG("GSENSOR_IOCTL_INIT fail!\n");
					return -EINVAL;
			}
				MAG_LOG("GSENSOR_IOCTL_INIT, enable: %d, sample_period:%dms\n", flag, 5);
			} else {
				MAG_PR_ERR("GSENSOR_IOCTL_INIT NULL\n");
				return -EINVAL;
		}
			return 0;
	case MSENSOR_IOCTL_READ_SENSORDATA:
			if (mag_factory.fops != NULL && mag_factory.fops->get_data != NULL) {
				err = mag_factory.fops->get_data(data_buf, &status);
				if (err < 0) {
					MAG_LOG("GSENSOR_IOCTL_READ_SENSORDATA read data fail!\n");
					return -EINVAL;
				}
				sprintf(strbuf, "%x %x %x", data_buf[0], data_buf[1], data_buf[2]);
				MAG_LOG("GSENSOR_IOCTL_READ_SENSORDATA read strbuf : (%s)!\n", strbuf);
				if (copy_to_user(ptr, strbuf, strlen(strbuf)+1))
					return -EFAULT;
			} else {
					MAG_LOG("GSENSOR_IOCTL_READ_SENSORDATA NULL\n");
					return -EINVAL;
				}
			return 0;
	default:
			MAG_LOG("unknown IOCTL: 0x%08x\n", cmd);
			return -ENOIOCTLCMD;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_mag_factory_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		MAG_PR_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}

	switch (cmd) {
/*	case COMPAT_MSENSOR_IOCTL_INIT:
 *	case COMPAT_MSENSOR_IOCTL_SET_POSTURE:
 *	case COMPAT_MSENSOR_IOCTL_SET_CALIDATA:
 *	case COMPAT_MSENSOR_IOCTL_READ_CHIPINFO:
 */
	case COMPAT_MSENSOR_IOCTL_READ_SENSORDATA:
/*	case COMPAT_MSENSOR_IOCTL_READ_POSTUREDATA:
 *	case COMPAT_MSENSOR_IOCTL_READ_CALIDATA:
 *	case COMPAT_MSENSOR_IOCTL_READ_CONTROL:
 *	case COMPAT_MSENSOR_IOCTL_SET_CONTROL:
 *	case COMPAT_MSENSOR_IOCTL_SET_MODE:
 */
	case COMPAT_MSENSOR_IOCTL_SENSOR_ENABLE:
	case COMPAT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA: {
		MAG_LOG("compat_ion_ioctl : MSENSOR_IOCTL_XXX command is 0x%x\n", cmd);
		return filp->f_op->unlocked_ioctl(filp, cmd,
			(unsigned long)compat_ptr(arg));
	}
	default: {
		MAG_PR_ERR("compat_ion_ioctl : No such command!! 0x%x\n", cmd);
		return -ENOIOCTLCMD;
	}
	}
}
#endif
/*----------------------------------------------------------------------------*/
static const struct file_operations mag_factory_fops = {
	.open = mag_factory_open,
	.release = mag_factory_release,
	.unlocked_ioctl = mag_factory_unlocked_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = compat_mag_factory_unlocked_ioctl,
#endif
};

static struct miscdevice mag_factory_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msensor",
	.fops = &mag_factory_fops,
};

int mag_factory_device_register(struct mag_factory_public *dev)
{
	int err = 0;

	if (!dev || !dev->fops)
		return -1;
	mag_factory.gain = dev->gain;
	mag_factory.sensitivity = dev->sensitivity;
	mag_factory.fops = dev->fops;
	err = misc_register(&mag_factory_device);
	if (err) {
		MAG_LOG("accel_factory_device register failed\n");
		err = -1;
	}
	return err;
	}

int mag_factory_device_deregister(struct mag_factory_public *dev)
{
	mag_factory.fops = NULL;
	misc_deregister(&mag_factory_device);
	return 0;
}
