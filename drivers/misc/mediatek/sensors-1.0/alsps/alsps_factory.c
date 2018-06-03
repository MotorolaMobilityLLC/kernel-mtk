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

#include "inc/alsps_factory.h"

struct alsps_factory_private {
	uint32_t gain;
	uint32_t sensitivity;
	struct alsps_factory_fops *fops;
};

static struct alsps_factory_private alsps_factory;

static int alsps_factory_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int alsps_factory_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long alsps_factory_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;
	void __user *ptr = (void __user *)arg;
	int data = 0;
	uint32_t enable = 0;
	int threshold_data[2] = {0, 0};

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		ALSPS_LOG("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case ALSPS_SET_PS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable)))
			return -EFAULT;
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_enable_sensor != NULL) {
			err = alsps_factory.fops->ps_enable_sensor(enable, 200);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_SET_PS_MODE fail!\n");
				return -EINVAL;
			}
			ALSPS_LOG("ALSPS_SET_PS_MODE, enable: %d, sample_period:%dms\n", enable, 200);
		} else {
			ALSPS_PR_ERR("ALSPS_SET_PS_MODE NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_GET_PS_RAW_DATA:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_get_raw_data != NULL) {
			err = alsps_factory.fops->ps_get_raw_data(&data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_GET_PS_RAW_DATA read data fail!\n");
				return -EINVAL;
			}
			if (copy_to_user(ptr, &data, sizeof(data)))
				return -EFAULT;
		} else {
			ALSPS_PR_ERR("ALSPS_GET_PS_RAW_DATA NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_SET_ALS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable)))
			return -EFAULT;
		if (alsps_factory.fops != NULL && alsps_factory.fops->als_enable_sensor != NULL) {
			err = alsps_factory.fops->als_enable_sensor(enable, 200);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_SET_ALS_MODE fail!\n");
				return -EINVAL;
			}
			ALSPS_LOG("ALSPS_SET_ALS_MODE, enable: %d, sample_period:%dms\n", enable, 200);
		} else {
			ALSPS_PR_ERR("ALSPS_SET_ALS_MODE NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_GET_ALS_RAW_DATA:
		if (alsps_factory.fops != NULL && alsps_factory.fops->als_get_raw_data != NULL) {
			err = alsps_factory.fops->als_get_raw_data(&data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_GET_ALS_RAW_DATA read data fail!\n");
				return -EINVAL;
			}
			if (copy_to_user(ptr, &data, sizeof(data)))
				return -EFAULT;
		} else {
			ALSPS_PR_ERR("ALSPS_GET_ALS_RAW_DATA NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_ALS_ENABLE_CALI:
		if (alsps_factory.fops != NULL && alsps_factory.fops->als_enable_calibration != NULL) {
			err = alsps_factory.fops->als_enable_calibration();
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_ALS_ENABLE_CALI FAIL!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_ALS_ENABLE_CALI NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_GET_PS_TEST_RESULT:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_get_data != NULL) {
			err = alsps_factory.fops->ps_get_data(&data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_GET_PS_TEST_RESULT read data fail!\n");
				return -EINVAL;
			}
			if (copy_to_user(ptr, &data, sizeof(data)))
				return -EFAULT;
		} else {
			ALSPS_PR_ERR("ALSPS_GET_PS_TEST_RESULT NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_GET_PS_THRESHOLD_HIGH:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_get_threashold != NULL) {
			err = alsps_factory.fops->ps_get_threashold(threshold_data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_GET_PS_THRESHOLD_HIGH read data fail!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_GET_PS_THRESHOLD_HIGH NULL\n");
			return -EINVAL;
		}
		if (copy_to_user(ptr, &threshold_data[0], sizeof(threshold_data[0])))
			return -EFAULT;
		return 0;
	case ALSPS_GET_PS_THRESHOLD_LOW:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_get_threashold != NULL) {
			err = alsps_factory.fops->ps_get_threashold(threshold_data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_GET_PS_THRESHOLD_HIGH read data fail!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_GET_PS_THRESHOLD_HIGH NULL\n");
			return -EINVAL;
		}
		if (copy_to_user(ptr, &threshold_data[1], sizeof(threshold_data[1])))
			return -EFAULT;
		return 0;
	case ALSPS_SET_PS_THRESHOLD:
		if (copy_from_user(threshold_data, ptr, sizeof(threshold_data)))
			return -EFAULT;
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_set_threashold != NULL) {
			err = alsps_factory.fops->ps_set_threashold(threshold_data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_SET_PS_THRESHOLD read data fail!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_SET_PS_THRESHOLD NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_IOCTL_SET_CALI:
		if (copy_from_user(&data, ptr, sizeof(data)))
			return -EFAULT;
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_set_cali != NULL) {
			err = alsps_factory.fops->ps_set_cali(data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_IOCTL_SET_CALI read data fail!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_IOCTL_SET_CALI NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_IOCTL_GET_CALI:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_get_cali != NULL) {
			err = alsps_factory.fops->ps_get_cali(&data);
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_IOCTL_GET_CALI FAIL!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_IOCTL_GET_CALI NULL\n");
			return -EINVAL;
		}
		if (copy_to_user(ptr, &data, sizeof(data)))
			return -EFAULT;
		return 0;
	case ALSPS_IOCTL_CLR_CALI:
		if (copy_from_user(&data, ptr, sizeof(data)))
			return -EFAULT;
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_clear_cali != NULL) {
			err = alsps_factory.fops->ps_clear_cali();
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_IOCTL_CLR_CALI FAIL!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_IOCTL_CLR_CALI NULL\n");
			return -EINVAL;
		}
		return 0;
	case ALSPS_PS_ENABLE_CALI:
		if (alsps_factory.fops != NULL && alsps_factory.fops->ps_enable_calibration != NULL) {
			err = alsps_factory.fops->ps_enable_calibration();
			if (err < 0) {
				ALSPS_PR_ERR("ALSPS_PS_ENABLE_CALI FAIL!\n");
				return -EINVAL;
			}
		} else {
			ALSPS_PR_ERR("ALSPS_PS_ENABLE_CALI NULL\n");
			return -EINVAL;
		}
		return 0;
	default:
		ALSPS_PR_ERR("unknown IOCTL: 0x%08x\n", cmd);
		return -ENOIOCTLCMD;
	}
	return 0;
}
#ifdef CONFIG_COMPAT
static long alsps_factory_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_ALSPS_SET_PS_MODE:
	case COMPAT_ALSPS_GET_PS_RAW_DATA:
	case COMPAT_ALSPS_SET_ALS_MODE:
	case COMPAT_ALSPS_GET_ALS_RAW_DATA:
	case COMPAT_ALSPS_GET_PS_TEST_RESULT:
	case COMPAT_ALSPS_GET_PS_THRESHOLD_HIGH:
	case COMPAT_ALSPS_GET_PS_THRESHOLD_LOW:
	case COMPAT_ALSPS_SET_PS_THRESHOLD:
	case COMPAT_ALSPS_IOCTL_SET_CALI:
	case COMPAT_ALSPS_IOCTL_GET_CALI:
	case COMPAT_ALSPS_IOCTL_CLR_CALI:
	case COMPAT_ALSPS_ALS_ENABLE_CALI:
	case COMPAT_ALSPS_PS_ENABLE_CALI:
		err = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
		break;
	default:
		ALSPS_PR_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;
	}

	return err;
}
#endif

static const struct file_operations _alsps_factory_fops = {
	.open = alsps_factory_open,
	.release = alsps_factory_release,
	.unlocked_ioctl = alsps_factory_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = alsps_factory_compat_ioctl,
#endif
};

static struct miscdevice alsps_factory_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &_alsps_factory_fops,
};

int alsps_factory_device_register(struct alsps_factory_public *dev)
{
	int err = 0;

	if (!dev || !dev->fops)
		return -1;
	alsps_factory.gain = dev->gain;
	alsps_factory.sensitivity = dev->sensitivity;
	alsps_factory.fops = dev->fops;
	err = misc_register(&alsps_factory_device);
	if (err) {
		ALSPS_PR_ERR("alsps_factory_device register failed\n");
		err = -1;
	}
	return err;
}

int alsps_factory_device_deregister(struct alsps_factory_public *dev)
{
	alsps_factory.fops = NULL;
	misc_deregister(&alsps_factory_device);
	return 0;
}

