/*
* Copyright (C) 2013 MediaTek Inc.
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

#include "inc/rgbw.h"
/* #include "inc/aal_control.h" */

struct rgbw_context *rgbw_context_obj;
static struct platform_device *pltfm_dev;


static struct rgbw_init_info *rgbw_init_list[MAX_CHOOSE_RGBW_NUM] = {0};

static bool rgbw_misc_dev_init;

int rgbw_data_report(struct input_dev *dev, int value, int value_g, int value_b, int value_w, int status)
{
	struct rgbw_context *cxt = NULL;

	cxt  = rgbw_context_obj;
	/* RGBW_LOG("rgbw_data_report: %d, %d\n", value, status); */
	/* force trigger data update after sensor enable. */
	if (cxt->is_get_valid_rgbw_data_after_enable == false) {
		input_report_abs(dev, EVENT_TYPE_RGBW_VALUE, value+1);
		input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_G, value_g+1);
		input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_B, value_b+1);
		input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_W, value_w+1);
		cxt->is_get_valid_rgbw_data_after_enable = true;
	}
	input_report_abs(dev, EVENT_TYPE_RGBW_VALUE, value);
	input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_G, value_g);
	input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_B, value_b);
	input_report_abs(dev, EVENT_TYPE_RGBW_VALUE_W, value_w);
	input_report_abs(dev, EVENT_TYPE_RGBW_STATUS, status);
	input_sync(dev);
	return 0;
}

static void rgbw_work_func(struct work_struct *work)
{
	struct rgbw_context *cxt = NULL;
	int value, value_g, value_b, value_w, status;
	int64_t  nt;
	struct timespec time;
	int err;

	cxt  = rgbw_context_obj;
	if (cxt->rgbw_data.get_data == NULL) {
		RGBW_ERR("rgbw driver not register data path\n");
		return;
	}

	time.tv_sec = time.tv_nsec = 0;
	time = get_monotonic_coarse();
	nt = time.tv_sec*1000000000LL+time.tv_nsec;
	/* add wake lock to make sure data can be read before system suspend */
	err = cxt->rgbw_data.get_data(&value, &value_g, &value_b, &value_w, &status);
	if (err) {
		RGBW_ERR("get rgbw data fails!!\n");
		goto rgbw_loop;
	} else {
		cxt->drv_data.rgbw_data.values[0] = value;
		cxt->drv_data.rgbw_data.values[1] = value_g;
		cxt->drv_data.rgbw_data.values[2] = value_b;
		cxt->drv_data.rgbw_data.values[3] = value_w;
		cxt->drv_data.rgbw_data.status = status;
		cxt->drv_data.rgbw_data.time = nt;
	}

	if (true ==  cxt->is_rgbw_first_data_after_enable) {
		cxt->is_rgbw_first_data_after_enable = false;
		/* filter -1 value */
		if (cxt->drv_data.rgbw_data.values[0] == RGBW_INVALID_VALUE ||
		    cxt->drv_data.rgbw_data.values[1] == RGBW_INVALID_VALUE ||
		    cxt->drv_data.rgbw_data.values[2] == RGBW_INVALID_VALUE ||
		    cxt->drv_data.rgbw_data.values[3] == RGBW_INVALID_VALUE) {
			RGBW_LOG(" read invalid data\n");
			goto rgbw_loop;
		}
	}
	/* RGBW_LOG(" rgbw data[%d]\n" , cxt->drv_data.rgbw_data.values[0]); */
	rgbw_data_report(cxt->idev,
	cxt->drv_data.rgbw_data.values[0],
	cxt->drv_data.rgbw_data.values[1],
	cxt->drv_data.rgbw_data.values[2],
	cxt->drv_data.rgbw_data.values[3],
	cxt->drv_data.rgbw_data.status);

rgbw_loop:
	if (true == cxt->is_rgbw_polling_run)
		mod_timer(&cxt->timer_rgbw, jiffies + atomic_read(&cxt->delay_rgbw)/(1000/HZ));
	else
		RGBW_LOG("cxt->is_rgbw_polling_run is false!\n");
}

static void rgbw_poll(unsigned long data)
{
	struct rgbw_context *obj = (struct rgbw_context *)data;

	if ((obj != NULL) && (obj->is_rgbw_polling_run))
		schedule_work(&obj->report_rgbw);
}

static struct rgbw_context *rgbw_context_alloc_object(void)
{
	struct rgbw_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	RGBW_LOG("rgbw_context_alloc_object++++\n");
	if (!obj) {
		RGBW_ERR("Alloc rgbw object error!\n");
		return NULL;
	}
	atomic_set(&obj->delay_rgbw, 200); /*5Hz, set work queue delay time 200ms */
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report_rgbw, rgbw_work_func);
	init_timer(&obj->timer_rgbw);
	obj->timer_rgbw.expires	= jiffies + atomic_read(&obj->delay_rgbw)/(1000/HZ);
	obj->timer_rgbw.function	= rgbw_poll;
	obj->timer_rgbw.data	= (unsigned long)obj;

	obj->is_rgbw_first_data_after_enable = false;
	obj->is_rgbw_polling_run = false;
	mutex_init(&obj->rgbw_op_mutex);
	obj->is_rgbw_batch_enable = false;/* for batch mode init */

	RGBW_LOG("rgbw_context_alloc_object----\n");
	return obj;
}

static int rgbw_real_enable(int enable)
{
	int err = 0;
	struct rgbw_context *cxt = NULL;

	cxt = rgbw_context_obj;
	if (enable == 1) {
		if (true == cxt->is_rgbw_active_data || true == cxt->is_rgbw_active_nodata) {
			err = cxt->rgbw_ctl.enable_nodata(1);
			if (err) {
				err = cxt->rgbw_ctl.enable_nodata(1);
				if (err) {
					err = cxt->rgbw_ctl.enable_nodata(1);
					if (err)
						RGBW_ERR("rgbw enable(%d) err 3 timers = %d\n", enable, err);
				}
			}
			RGBW_LOG("rgbw real enable\n");
		}
	}

	if (enable == 0) {
		if (false == cxt->is_rgbw_active_data && false == cxt->is_rgbw_active_nodata) {
/* Need To Modify */
/*			RGBW_LOG("AAL status is %d\n", aal_use);*/
/*			if (aal_use == 0) {*/
			err = cxt->rgbw_ctl.enable_nodata(0);
			if (err)
				RGBW_ERR("rgbw enable(%d) err = %d\n", enable, err);

/*			}*/
			RGBW_LOG("rgbw real disable\n");
		}
	}

	return err;
}

static int rgbw_enable_data(int enable)
{
	struct rgbw_context *cxt = NULL;

	cxt = rgbw_context_obj;
	if (cxt->rgbw_ctl.open_report_data == NULL) {
		RGBW_ERR("no rgbw control path\n");
		return -1;
	}

	if (enable == 1) {
		RGBW_LOG("RGBW enable data\n");
		cxt->is_rgbw_active_data = true;
		cxt->is_rgbw_first_data_after_enable = true;
		cxt->rgbw_ctl.open_report_data(1);
		rgbw_real_enable(enable);
		if (false == cxt->is_rgbw_polling_run && cxt->is_rgbw_batch_enable == false) {
			if (false == cxt->rgbw_ctl.is_report_input_direct) {
				cxt->is_get_valid_rgbw_data_after_enable = false;
				mod_timer(&cxt->timer_rgbw, jiffies + atomic_read(&cxt->delay_rgbw)/(1000/HZ));
				cxt->is_rgbw_polling_run = true;
				RGBW_LOG("RGBW enable, mod_timer\n");
			}
		}
	}

	if (enable == 0) {
		RGBW_LOG("RGBW disable\n");
		cxt->is_rgbw_active_data = false;
		cxt->rgbw_ctl.open_report_data(0);
		if (true == cxt->is_rgbw_polling_run) {
			if (false == cxt->rgbw_ctl.is_report_input_direct) {
				cxt->is_rgbw_polling_run = false;
				smp_mb();/* for memory barrier */
				del_timer_sync(&cxt->timer_rgbw);
				smp_mb();/* for memory barrier */
				cancel_work_sync(&cxt->report_rgbw);
				cxt->drv_data.rgbw_data.values[0] = RGBW_INVALID_VALUE;
				RGBW_LOG("RGBW disable, del_timer\n");
			}
		}
		rgbw_real_enable(enable);
	}
	return 0;
}

static ssize_t rgbw_store_active(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct rgbw_context *cxt = NULL;

	RGBW_LOG("rgbw_store_active buf=%s\n", buf);
	mutex_lock(&rgbw_context_obj->rgbw_op_mutex);
	cxt = rgbw_context_obj;

	if (!strncmp(buf, "1", 1))
		rgbw_enable_data(1);
	else if (!strncmp(buf, "0", 1))
		rgbw_enable_data(0);
	else
		RGBW_ERR(" rgbw_store_active error !!\n");

	mutex_unlock(&rgbw_context_obj->rgbw_op_mutex);
	RGBW_LOG(" rgbw_store_active done\n");
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t rgbw_show_active(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct rgbw_context *cxt = NULL;
	int div = 0;

	cxt = rgbw_context_obj;
	div = cxt->rgbw_data.vender_div;
	RGBW_LOG("rgbw vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div);
}

static ssize_t rgbw_store_delay(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int delay;
	int mdelay = 0;
	int ret = 0;
	struct rgbw_context *cxt = NULL;

	mutex_lock(&rgbw_context_obj->rgbw_op_mutex);
	cxt = rgbw_context_obj;
	if (cxt->rgbw_ctl.set_delay == NULL) {
		RGBW_LOG("rgbw_ctl set_delay NULL\n");
		mutex_unlock(&rgbw_context_obj->rgbw_op_mutex);
		return count;
	}

	ret = kstrtoint(buf, 10, &delay);
	if (ret != 0) {
		RGBW_ERR("invalid format!!\n");
		mutex_unlock(&rgbw_context_obj->rgbw_op_mutex);
		return count;
	}

	if (false == cxt->rgbw_ctl.is_report_input_direct) {
		mdelay = (int)delay/1000/1000;
		atomic_set(&rgbw_context_obj->delay_rgbw, mdelay);
	}
	cxt->rgbw_ctl.set_delay(delay);
	RGBW_LOG(" rgbw_delay %d ns\n", delay);
	mutex_unlock(&rgbw_context_obj->rgbw_op_mutex);
	return count;
}

static ssize_t rgbw_show_delay(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int len = 0;

	RGBW_LOG(" not support now\n");
	return len;
}


static ssize_t rgbw_store_batch(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct rgbw_context *cxt = NULL;

	RGBW_LOG("rgbw_store_batch buf=%s\n", buf);
	mutex_lock(&rgbw_context_obj->rgbw_op_mutex);
	cxt = rgbw_context_obj;
	if (cxt->rgbw_ctl.is_support_batch) {
		if (!strncmp(buf, "1", 1)) {
			cxt->is_rgbw_batch_enable = true;
			if (true == cxt->is_rgbw_polling_run) {
				cxt->is_rgbw_polling_run = false;
				del_timer_sync(&cxt->timer_rgbw);
				cancel_work_sync(&cxt->report_rgbw);
				cxt->drv_data.rgbw_data.values[0] = RGBW_INVALID_VALUE;
				cxt->drv_data.rgbw_data.values[1] = RGBW_INVALID_VALUE;
				cxt->drv_data.rgbw_data.values[2] = RGBW_INVALID_VALUE;
			}
		} else if (!strncmp(buf, "0", 1)) {
			cxt->is_rgbw_batch_enable = false;
			if (false == cxt->is_rgbw_polling_run) {
				if (false == cxt->rgbw_ctl.is_report_input_direct) {
					cxt->is_get_valid_rgbw_data_after_enable = false;
					mod_timer(&cxt->timer_rgbw, jiffies + atomic_read(&cxt->delay_rgbw)/(1000/HZ));
					cxt->is_rgbw_polling_run = true;
				}
			}
		} else
			RGBW_ERR(" rgbw_store_batch error !!\n");
	} else
		RGBW_LOG(" rgbw_store_batch not supported\n");

	mutex_unlock(&rgbw_context_obj->rgbw_op_mutex);
	RGBW_LOG(" rgbw_store_batch done: %d\n", cxt->is_rgbw_batch_enable);
	return count;
}

static ssize_t rgbw_show_batch(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t rgbw_store_flush(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	return count;
}

static ssize_t rgbw_show_flush(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}
/* need work around again */
static ssize_t rgbw_show_devnum(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	unsigned int devnum;
	const char *devname = NULL;
	int ret;

	devname = dev_name(&rgbw_context_obj->idev->dev);
	ret = sscanf(devname+5, "%d", &devnum);
	return snprintf(buf, PAGE_SIZE, "%d\n", devnum);
}
/*----------------------------------------------------------------------------*/
static int rgb_w_remove(struct platform_device *pdev)
{
	RGBW_LOG("rgb_w_remove\n");
	return 0;
}

static int rgb_w_probe(struct platform_device *pdev)
{
	RGBW_LOG("rgb_w_probe\n");
	pltfm_dev = pdev;
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rgbw_of_match[] = {
	{.compatible = "mediatek,rgbw",},
	{},
};
#endif

static struct platform_driver rgbw_driver = {
	.probe	  = rgb_w_probe,
	.remove	 = rgb_w_remove,
	.driver = {

		.name  = "rgbw",
	#ifdef CONFIG_OF
		.of_match_table = rgbw_of_match,
		#endif
	}
};

static int rgbw_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	RGBW_LOG(" rgbw_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_RGBW_NUM; i++) {
		RGBW_LOG("rgbw_real_driver_init i=%d\n", i);
		if (rgbw_init_list[i] != 0) {
			RGBW_LOG(" rgbw try to init driver %s\n", rgbw_init_list[i]->name);
			err = rgbw_init_list[i]->init();
			if (err == 0) {
				RGBW_LOG(" rgbw real driver %s probe ok\n", rgbw_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_RGBW_NUM) {
		RGBW_LOG(" rgbw_real_driver_init fail\n");
		err =  -1;
	}

	return err;
}

int rgbw_driver_add(struct rgbw_init_info *obj)
{
	int err = 0;
	int i = 0;

	RGBW_FUN();
	if (!obj) {
		RGBW_ERR("RGBW driver add fail, rgbw_init_info is NULL\n");
		return -1;
	}

	for (i = 0; i < MAX_CHOOSE_RGBW_NUM; i++) {
		if ((i == 0) && (rgbw_init_list[0] == NULL)) {
			RGBW_LOG("register rgbw driver for the first time\n");
			if (platform_driver_register(&rgbw_driver))
				RGBW_ERR("failed to register rgbw driver already exist\n");
		}

		if (rgbw_init_list[i] == NULL) {
			obj->platform_diver_addr = &rgbw_driver;
			rgbw_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_RGBW_NUM) {
		RGBW_ERR("RGBW driver add err\n");
		err =  -1;
	}

	return err;
}
EXPORT_SYMBOL_GPL(rgbw_driver_add);
struct platform_device *get_rgbw_platformdev(void)
{
	return pltfm_dev;
}

static int rgbw_misc_init(struct rgbw_context *cxt)
{
	int err = 0;

	cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = RGBW_MISC_DEV_NAME;
	err = misc_register(&cxt->mdev);
	if (err)
		RGBW_ERR("unable to register rgbw misc device!!\n");

	return err;
}


static int rgbw_input_init(struct rgbw_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (dev == NULL)
		return -ENOMEM;

	dev->name = RGBW_INPUTDEV_NAME;
	set_bit(EV_REL, dev->evbit);
	set_bit(EV_SYN, dev->evbit);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RGBW_VALUE);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RGBW_VALUE_G);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RGBW_VALUE_B);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RGBW_VALUE_W);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RGBW_STATUS);
	input_set_abs_params(dev, EVENT_TYPE_RGBW_VALUE, RGBW_VALUE_MIN, RGBW_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RGBW_VALUE_G, RGBW_VALUE_MIN, RGBW_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RGBW_VALUE_B, RGBW_VALUE_MIN, RGBW_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RGBW_VALUE_W, RGBW_VALUE_MIN, RGBW_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RGBW_STATUS, RGBW_STATUS_MIN, RGBW_STATUS_MAX, 0, 0);
	input_set_drvdata(dev, cxt);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev = dev;

	return 0;
}

DEVICE_ATTR(rgbwactive,		S_IWUSR | S_IRUGO, rgbw_show_active, rgbw_store_active);
DEVICE_ATTR(rgbwdelay,		S_IWUSR | S_IRUGO, rgbw_show_delay,  rgbw_store_delay);
DEVICE_ATTR(rgbwbatch,		S_IWUSR | S_IRUGO, rgbw_show_batch,  rgbw_store_batch);
DEVICE_ATTR(rgbwflush,		S_IWUSR | S_IRUGO, rgbw_show_flush,  rgbw_store_flush);
DEVICE_ATTR(rgbwdevnum,		S_IWUSR | S_IRUGO, rgbw_show_devnum,  NULL);

static struct attribute *rgbw_attribute[] = {
	&dev_attr_rgbwactive.attr,
	&dev_attr_rgbwdelay.attr,
	&dev_attr_rgbwbatch.attr,
	&dev_attr_rgbwflush.attr,
	&dev_attr_rgbwdevnum.attr,
	NULL
};

static struct attribute_group rgbw_attribute_group = {
	.attrs = rgbw_attribute
};

int rgbw_register_data_path(struct rgbw_data_path *data)
{
	struct rgbw_context *cxt = NULL;
	/* int err =0; */
	cxt = rgbw_context_obj;
	cxt->rgbw_data.get_data = data->get_data;
	cxt->rgbw_data.vender_div = data->vender_div;
	cxt->rgbw_data.rgbw_get_raw_data = data->rgbw_get_raw_data;
	RGBW_LOG("rgbw register data path vender_div: %d\n", cxt->rgbw_data.vender_div);
	if (cxt->rgbw_data.get_data == NULL) {
		RGBW_LOG("rgbw register data path fail\n");
		return -1;
	}
	return 0;
}

int rgbw_register_control_path(struct rgbw_control_path *ctl)
{
	struct rgbw_context *cxt = NULL;
	int err = 0;

	cxt = rgbw_context_obj;
	cxt->rgbw_ctl.set_delay = ctl->set_delay;
	cxt->rgbw_ctl.open_report_data = ctl->open_report_data;
	cxt->rgbw_ctl.enable_nodata = ctl->enable_nodata;
	cxt->rgbw_ctl.is_support_batch = ctl->is_support_batch;
	cxt->rgbw_ctl.is_report_input_direct = ctl->is_report_input_direct;
	cxt->rgbw_ctl.is_use_common_factory = ctl->is_use_common_factory;

	if (NULL == cxt->rgbw_ctl.set_delay || NULL == cxt->rgbw_ctl.open_report_data
		|| NULL == cxt->rgbw_ctl.enable_nodata) {
		RGBW_LOG("rgbw register control path fail\n");
		return -1;
	}

	if (!rgbw_misc_dev_init) {
		/* add misc dev for sensor hal control cmd */
		err = rgbw_misc_init(rgbw_context_obj);
		if (err) {
			RGBW_ERR("unable to register rgbw misc device!!\n");
			return -2;
		}
		err = sysfs_create_group(&rgbw_context_obj->mdev.this_device->kobj,
				&rgbw_attribute_group);
		if (err < 0) {
			RGBW_ERR("unable to create rgbw attribute file\n");
			return -3;
		}
		kobject_uevent(&rgbw_context_obj->mdev.this_device->kobj, KOBJ_ADD);
		rgbw_misc_dev_init = true;
	}
	return 0;
}

static int rgbw_probe(void)
{
	int err;

	RGBW_LOG("+++++++++++++rgbw_probe!!\n");
	rgbw_context_obj = rgbw_context_alloc_object();
	if (!rgbw_context_obj) {
		err = -ENOMEM;
		RGBW_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	/* init real rgbw sensor driver */
	err = rgbw_real_driver_init();
	if (err) {
		RGBW_ERR("rgbw real driver init fail\n");
		goto real_driver_init_fail;
	}
#if 0
	/* init rgbw common factory mode misc device */
	err = rgbw_factory_device_init();
	if (err)
		RGBW_ERR("rgbw factory device already registed\n");
#endif
	/* init input dev */
	err = rgbw_input_init(rgbw_context_obj);
	if (err) {
		RGBW_ERR("unable to register rgbw input device!\n");
		goto exit_alloc_input_dev_failed;
	}
	RGBW_LOG("----rgbw_probe OK !!\n");
	return 0;

real_driver_init_fail:
exit_alloc_input_dev_failed:
	kfree(rgbw_context_obj);
	rgbw_context_obj = NULL;
exit_alloc_data_failed:
	RGBW_ERR("----rgbw_probe fail !!!\n");
	return err;
}

static int rgbw_remove(void)
{
	RGBW_FUN(f);
	input_unregister_device(rgbw_context_obj->idev);
	sysfs_remove_group(&rgbw_context_obj->idev->dev.kobj,
				&rgbw_attribute_group);

	misc_deregister(&rgbw_context_obj->mdev);
	kfree(rgbw_context_obj);

	return 0;
}

static int __init rgbw_init(void)
{
	RGBW_FUN();

	if (rgbw_probe()) {
		RGBW_ERR("failed to register rgbw driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit rgbw_exit(void)
{
	rgbw_remove();
	platform_driver_unregister(&rgbw_driver);

}
late_initcall(rgbw_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RGBW device driver");
MODULE_AUTHOR("Mediatek");

