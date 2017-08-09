
#include "rotationvector.h"

static struct rotationvector_context *rotationvector_context_obj;


static struct rotationvector_init_info *rotationvectorsensor_init_list[MAX_CHOOSE_RV_NUM] = { 0 };	/* modified */

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
static void rotationvector_early_suspend(struct early_suspend *h);
static void rotationvector_late_resume(struct early_suspend *h);
#endif

static void rotationvector_work_func(struct work_struct *work)
{

	struct rotationvector_context *cxt = NULL;
	int x, y, z, scalar, status;
	int64_t nt;
	struct timespec time;
	int err;

	cxt = rotationvector_context_obj;

	if (NULL == cxt->rotationvector_data.get_data)
		RV_LOG("rotationvector driver not register data path\n");

	time.tv_sec = time.tv_nsec = 0;
	time = get_monotonic_coarse();
	nt = time.tv_sec * 1000000000LL + time.tv_nsec;

	err = cxt->rotationvector_data.get_data(&x, &y, &z, &scalar, &status);

	if (err) {
		RV_ERR("get rotationvector data fails!!\n");
		goto rotationvector_loop;
	} else {
		{
			if (0 == x && 0 == y && 0 == z)
				goto rotationvector_loop;

			cxt->drv_data.rotationvector_data.values[0] = x;
			cxt->drv_data.rotationvector_data.values[1] = y;
			cxt->drv_data.rotationvector_data.values[2] = z;
			cxt->drv_data.rotationvector_data.values[3] = scalar;
			cxt->drv_data.rotationvector_data.status = status;
			cxt->drv_data.rotationvector_data.time = nt;

		}
	}

	if (true == cxt->is_first_data_after_enable) {
		cxt->is_first_data_after_enable = false;
		/* filter -1 value */
		if (RV_INVALID_VALUE == cxt->drv_data.rotationvector_data.values[0] ||
		    RV_INVALID_VALUE == cxt->drv_data.rotationvector_data.values[1] ||
		    RV_INVALID_VALUE == cxt->drv_data.rotationvector_data.values[2] ||
		    RV_INVALID_VALUE == cxt->drv_data.rotationvector_data.values[3]
		    ) {
			RV_LOG(" read invalid data\n");
			goto rotationvector_loop;

		}
	}
	/* report data to input device */
	/* printk("new rotationvector work run....\n"); */
	/* RV_LOG("rotationvector data[%d,%d,%d]\n" ,cxt->drv_data.rotationvector_data.values[0], */
	/* cxt->drv_data.rotationvector_data.values[1],cxt->drv_data.rotationvector_data.values[2]); */

	rotationvector_data_report(cxt->drv_data.rotationvector_data.values[0],
				   cxt->drv_data.rotationvector_data.values[1],
				   cxt->drv_data.rotationvector_data.values[2],
				   cxt->drv_data.rotationvector_data.values[3],
				   cxt->drv_data.rotationvector_data.status);

rotationvector_loop:
	if (true == cxt->is_polling_run)
		mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay) / (1000 / HZ));
}

static void rotationvector_poll(unsigned long data)
{
	struct rotationvector_context *obj = (struct rotationvector_context *)data;

	if (obj != NULL)
		schedule_work(&obj->report);
}

static struct rotationvector_context *rotationvector_context_alloc_object(void)
{

	struct rotationvector_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	RV_LOG("rotationvector_context_alloc_object++++\n");
	if (!obj) {
		RV_ERR("Alloc rotationvector object error!\n");
		return NULL;
	}
	atomic_set(&obj->delay, 200);	/*5Hz set work queue delay time 200ms */
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report, rotationvector_work_func);
	init_timer(&obj->timer);
	obj->timer.expires = jiffies + atomic_read(&obj->delay) / (1000 / HZ);
	obj->timer.function = rotationvector_poll;
	obj->timer.data = (unsigned long)obj;
	obj->is_first_data_after_enable = false;
	obj->is_polling_run = false;
	mutex_init(&obj->rotationvector_op_mutex);
	obj->is_batch_enable = false;	/* for batch mode init */
	RV_LOG("rotationvector_context_alloc_object----\n");
	return obj;
}

static int rotationvector_real_enable(int enable)
{
	int err = 0;
	struct rotationvector_context *cxt = NULL;

	cxt = rotationvector_context_obj;
	if (1 == enable) {

		if (true == cxt->is_active_data || true == cxt->is_active_nodata) {
			err = cxt->rotationvector_ctl.enable_nodata(1);
			if (err) {
				err = cxt->rotationvector_ctl.enable_nodata(1);
				if (err) {
					err = cxt->rotationvector_ctl.enable_nodata(1);
					if (err)
						RV_ERR
						    ("rotationvector enable(%d) err 3 timers = %d\n",
						     enable, err);
				}
			}
			RV_LOG("rotationvector real enable\n");
		}

	}
	if (0 == enable) {
		if (false == cxt->is_active_data && false == cxt->is_active_nodata) {
			err = cxt->rotationvector_ctl.enable_nodata(0);
			if (err)
				RV_ERR("rotationvector enable(%d) err = %d\n", enable, err);

			RV_LOG("rotationvector real disable\n");
		}

	}

	return err;
}

static int rotationvector_enable_data(int enable)
{
	struct rotationvector_context *cxt = NULL;

	/* int err =0; */
	cxt = rotationvector_context_obj;
	if (NULL == cxt->rotationvector_ctl.open_report_data) {
		RV_ERR("no rotationvector control path\n");
		return -1;
	}

	if (1 == enable) {
		RV_LOG("RV enable data\n");
		cxt->is_active_data = true;
		cxt->is_first_data_after_enable = true;
		cxt->rotationvector_ctl.open_report_data(1);
		if (false == cxt->is_polling_run && cxt->is_batch_enable == false) {
			if (false == cxt->rotationvector_ctl.is_report_input_direct) {
				mod_timer(&cxt->timer,
					  jiffies + atomic_read(&cxt->delay) / (1000 / HZ));
				cxt->is_polling_run = true;
			}
		}
	}
	if (0 == enable) {
		RV_LOG("RV disable\n");

		cxt->is_active_data = false;
		cxt->rotationvector_ctl.open_report_data(0);
		if (true == cxt->is_polling_run) {
			if (false == cxt->rotationvector_ctl.is_report_input_direct) {
				cxt->is_polling_run = false;
				del_timer_sync(&cxt->timer);
				cancel_work_sync(&cxt->report);
				cxt->drv_data.rotationvector_data.values[0] = RV_INVALID_VALUE;
				cxt->drv_data.rotationvector_data.values[1] = RV_INVALID_VALUE;
				cxt->drv_data.rotationvector_data.values[2] = RV_INVALID_VALUE;
			}
		}

	}
	rotationvector_real_enable(enable);
	return 0;
}



int rotationvector_enable_nodata(int enable)
{
	struct rotationvector_context *cxt = NULL;

	/* int err =0; */
	cxt = rotationvector_context_obj;
	if (NULL == cxt->rotationvector_ctl.enable_nodata) {
		RV_ERR("rotationvector_enable_nodata:rotationvector ctl path is NULL\n");
		return -1;
	}

	if (1 == enable)
		cxt->is_active_nodata = true;

	if (0 == enable)
		cxt->is_active_nodata = false;

	rotationvector_real_enable(enable);
	return 0;
}


static ssize_t rotationvector_show_enable_nodata(struct device *dev,
						 struct device_attribute *attr, char *buf)
{
	int len = 0;

	RV_LOG(" not support now\n");
	return len;
}

static ssize_t rotationvector_store_enable_nodata(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	struct rotationvector_context *cxt = NULL;
	/* int err =0; */

	RV_LOG("rotationvector_store_enable nodata buf=%s\n", buf);
	mutex_lock(&rotationvector_context_obj->rotationvector_op_mutex);

	cxt = rotationvector_context_obj;
	if (NULL == cxt->rotationvector_ctl.enable_nodata) {
		RV_LOG("rotationvector_ctl enable nodata NULL\n");
		mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
		return count;
	}
	if (!strncmp(buf, "1", 1)) {
		/* cxt->rotationvector_ctl.enable_nodata(1); */
		rotationvector_enable_nodata(1);
	} else if (!strncmp(buf, "0", 1)) {
		/* cxt->rotationvector_ctl.enable_nodata(0); */
		rotationvector_enable_nodata(0);
	} else {
		RV_ERR(" rotationvector_store enable nodata cmd error !!\n");
	}
	mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);

	return 0;
}

static ssize_t rotationvector_store_active(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct rotationvector_context *cxt = NULL;
	/* int err =0; */

	RV_LOG("rotationvector_store_active buf=%s\n", buf);
	mutex_lock(&rotationvector_context_obj->rotationvector_op_mutex);
	cxt = rotationvector_context_obj;
	if (NULL == cxt->rotationvector_ctl.open_report_data) {
		RV_LOG("rotationvector_ctl enable NULL\n");
		mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
		return count;
	}
	if (!strncmp(buf, "1", 1)) {
		/* cxt->rotationvector_ctl.enable(1); */
		rotationvector_enable_data(1);

	} else if (!strncmp(buf, "0", 1)) {

		/* cxt->rotationvector_ctl.enable(0); */
		rotationvector_enable_data(0);
	} else {
		RV_ERR(" rotationvector_store_active error !!\n");
	}
	mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
	RV_LOG(" rotationvector_store_active done\n");
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t rotationvector_show_active(struct device *dev,
					  struct device_attribute *attr, char *buf)
{
	struct rotationvector_context *cxt = NULL;
	int div;

	cxt = rotationvector_context_obj;
	div = cxt->rotationvector_data.vender_div;

	RV_LOG("rotationvector vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div);
}

static ssize_t rotationvector_store_delay(struct device *dev, struct device_attribute *attr,
					  const char *buf, size_t count)
{
	/* struct rotationvector_context *devobj = (struct rotationvector_context*)dev_get_drvdata(dev); */
	int delay;
	int mdelay = 0;
	struct rotationvector_context *cxt = NULL;
	int res = 0;

	mutex_lock(&rotationvector_context_obj->rotationvector_op_mutex);
	/* int err =0; */
	cxt = rotationvector_context_obj;
	if (NULL == cxt->rotationvector_ctl.set_delay) {
		RV_LOG("rotationvector_ctl set_delay NULL\n");
		mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
		return count;
	}

	res = kstrtoint(buf, 10, &delay);
	if (res != 0) {
		RV_ERR("invalid format!!\n");
		mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
		return count;
	}

	if (false == cxt->rotationvector_ctl.is_report_input_direct) {
		mdelay = (int)delay / 1000 / 1000;
		atomic_set(&rotationvector_context_obj->delay, mdelay);
	}
	cxt->rotationvector_ctl.set_delay(delay);
	RV_LOG(" rotationvector_delay %d ns\n", delay);
	mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
	return count;
}

static ssize_t rotationvector_show_delay(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	int len = 0;

	RV_LOG(" not support now\n");
	return len;
}

static ssize_t rotationvector_show_sensordevnum(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct rotationvector_context *cxt = NULL;
	char *devname = NULL;

	cxt = rotationvector_context_obj;
	devname = (char *)dev_name(&cxt->idev->dev);
	return snprintf(buf, PAGE_SIZE, "%s\n", devname + 5);
}


static ssize_t rotationvector_store_batch(struct device *dev, struct device_attribute *attr,
					  const char *buf, size_t count)
{

	struct rotationvector_context *cxt = NULL;

	/* int err =0; */
	RV_LOG("rotationvector_store_batch buf=%s\n", buf);
	mutex_lock(&rotationvector_context_obj->rotationvector_op_mutex);
	cxt = rotationvector_context_obj;
	if (cxt->rotationvector_ctl.is_support_batch) {
		if (!strncmp(buf, "1", 1)) {
			cxt->is_batch_enable = true;
			/* MTK problem fix - start */
			if (cxt->is_active_data && cxt->is_polling_run) {
				cxt->is_polling_run = false;
				del_timer_sync(&cxt->timer);
				cancel_work_sync(&cxt->report);
			}
			/* MTK problem fix - end */
		} else if (!strncmp(buf, "0", 1)) {
			cxt->is_batch_enable = false;
			/* MTK problem fix - start */
			if (cxt->is_active_data)
				rotationvector_enable_data(true);
			/* MTK problem fix - end */
		} else {
			RV_ERR(" rotationvector_store_batch error !!\n");
		}
	} else {
		RV_LOG(" rotationvector_store_batch mot supported\n");
	}
	mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
	RV_LOG(" rotationvector_store_batch done: %d\n", cxt->is_batch_enable);
	return count;

}

static ssize_t rotationvector_show_batch(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t rotationvector_store_flush(struct device *dev, struct device_attribute *attr,
					  const char *buf, size_t count)
{
	/* struct rotationvector_context *devobj = (struct rotationvector_context*)dev_get_drvdata(dev); */
	mutex_lock(&rotationvector_context_obj->rotationvector_op_mutex);
	/* do read FIFO data function and report data immediately */
	mutex_unlock(&rotationvector_context_obj->rotationvector_op_mutex);
	return count;
}

static ssize_t rotationvector_show_flush(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static int rotationvectorsensor_remove(struct platform_device *pdev)
{
	RV_LOG("rotationvectorsensor_remove\n");
	return 0;
}

static int rotationvectorsensor_probe(struct platform_device *pdev)
{
	RV_LOG("rotationvectorsensor_probe\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rotationvectorsensor_of_match[] = {
	{.compatible = "mediatek,rotationvectorsensor",},
	{},
};
#endif

static struct platform_driver rotationvectorsensor_driver = {
	.probe = rotationvectorsensor_probe,
	.remove = rotationvectorsensor_remove,
	.driver = {
		   .name = "rotationvectorsensor",
#ifdef CONFIG_OF
		   .of_match_table = rotationvectorsensor_of_match,
#endif
		   }
};

static int rotationvector_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	RV_LOG(" rotationvector_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_RV_NUM; i++) {
		RV_LOG(" i=%d\n", i);
		if (0 != rotationvectorsensor_init_list[i]) {
			RV_LOG(" rotationvector try to init driver %s\n",
			       rotationvectorsensor_init_list[i]->name);
			err = rotationvectorsensor_init_list[i]->init();
			if (0 == err) {
				RV_LOG(" rotationvector real driver %s probe ok\n",
				       rotationvectorsensor_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_RV_NUM) {
		RV_LOG(" rotationvector_real_driver_init fail\n");
		err = -1;
	}
	return err;
}

static int rotationvector_misc_init(struct rotationvector_context *cxt)
{

	int err = 0;

	cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name = RV_MISC_DEV_NAME;

	err = misc_register(&cxt->mdev);
	if (err)
		RV_ERR("unable to register rotationvector misc device!!\n");

	/* dev_set_drvdata(cxt->mdev.this_device, cxt); */
	return err;
}

static void rotationvector_input_destroy(struct rotationvector_context *cxt)
{
	struct input_dev *dev = cxt->idev;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int rotationvector_input_init(struct rotationvector_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = RV_INPUTDEV_NAME;

	input_set_capability(dev, EV_ABS, EVENT_TYPE_RV_X);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RV_Y);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RV_Z);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_RV_SCALAR);
	input_set_capability(dev, EV_REL, EVENT_TYPE_RV_STATUS);

	input_set_abs_params(dev, EVENT_TYPE_RV_X, RV_VALUE_MIN, RV_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RV_Y, RV_VALUE_MIN, RV_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RV_Z, RV_VALUE_MIN, RV_VALUE_MAX, 0, 0);
	input_set_abs_params(dev, EVENT_TYPE_RV_SCALAR, RV_VALUE_MIN, RV_VALUE_MAX, 0, 0);
	input_set_drvdata(dev, cxt);

	input_set_events_per_packet(dev, 32);	/* test */

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev = dev;

	return 0;
}

DEVICE_ATTR(rvnablenodata, S_IWUSR | S_IRUGO, rotationvector_show_enable_nodata,
	    rotationvector_store_enable_nodata);
DEVICE_ATTR(rvactive, S_IWUSR | S_IRUGO, rotationvector_show_active, rotationvector_store_active);
DEVICE_ATTR(rvdelay, S_IWUSR | S_IRUGO, rotationvector_show_delay, rotationvector_store_delay);
DEVICE_ATTR(rvbatch, S_IWUSR | S_IRUGO, rotationvector_show_batch, rotationvector_store_batch);
DEVICE_ATTR(rvflush, S_IWUSR | S_IRUGO, rotationvector_show_flush, rotationvector_store_flush);
DEVICE_ATTR(rvdevnum, S_IWUSR | S_IRUGO, rotationvector_show_sensordevnum, NULL);

static struct attribute *rotationvector_attributes[] = {
	&dev_attr_rvnablenodata.attr,
	&dev_attr_rvactive.attr,
	&dev_attr_rvdelay.attr,
	&dev_attr_rvbatch.attr,
	&dev_attr_rvflush.attr,
	&dev_attr_rvdevnum.attr,
	NULL
};

static struct attribute_group rotationvector_attribute_group = {
	.attrs = rotationvector_attributes
};

int rotationvector_register_data_path(struct rotationvector_data_path *data)
{
	struct rotationvector_context *cxt = NULL;
	/* int err =0; */
	cxt = rotationvector_context_obj;
	cxt->rotationvector_data.get_data = data->get_data;
	cxt->rotationvector_data.vender_div = data->vender_div;
	RV_LOG("rotationvector register data path vender_div: %d\n",
	       cxt->rotationvector_data.vender_div);
	if (NULL == cxt->rotationvector_data.get_data) {
		RV_LOG("rotationvector register data path fail\n");
		return -1;
	}
	return 0;
}

int rotationvector_register_control_path(struct rotationvector_control_path *ctl)
{
	struct rotationvector_context *cxt = NULL;
	int err = 0;

	cxt = rotationvector_context_obj;
	cxt->rotationvector_ctl.set_delay = ctl->set_delay;
	cxt->rotationvector_ctl.open_report_data = ctl->open_report_data;
	cxt->rotationvector_ctl.enable_nodata = ctl->enable_nodata;
	cxt->rotationvector_ctl.is_support_batch = ctl->is_support_batch;
	cxt->rotationvector_ctl.is_report_input_direct = ctl->is_report_input_direct;

	if (NULL == cxt->rotationvector_ctl.set_delay
	    || NULL == cxt->rotationvector_ctl.open_report_data
	    || NULL == cxt->rotationvector_ctl.enable_nodata) {
		RV_LOG("rotationvector register control path fail\n");
		return -1;
	}
	/* add misc dev for sensor hal control cmd */
	err = rotationvector_misc_init(rotationvector_context_obj);
	if (err) {
		RV_ERR("unable to register rotationvector misc device!!\n");
		return -2;
	}
	err = sysfs_create_group(&rotationvector_context_obj->mdev.this_device->kobj,
				 &rotationvector_attribute_group);
	if (err < 0) {
		RV_ERR("unable to create rotationvector attribute file\n");
		return -3;
	}

	kobject_uevent(&rotationvector_context_obj->mdev.this_device->kobj, KOBJ_ADD);

	return 0;
}

int rotationvector_data_report(int x, int y, int z, int scalar, int status)
{
	/* RV_LOG("+rotationvector_data_report! %d, %d, %d, %d\n",x,y,z,status); */
	struct rotationvector_context *cxt = NULL;

	cxt = rotationvector_context_obj;
	input_report_abs(cxt->idev, EVENT_TYPE_RV_X, x);
	input_report_abs(cxt->idev, EVENT_TYPE_RV_Y, y);
	input_report_abs(cxt->idev, EVENT_TYPE_RV_Z, z);
	input_report_abs(cxt->idev, EVENT_TYPE_RV_SCALAR, scalar);
	/* input_report_rel(cxt->idev, EVENT_TYPE_RV_STATUS, status); */
	input_sync(cxt->idev);
	return 0;
}

static int rotationvector_probe(struct platform_device *pdev)
{

	int err;

	RV_LOG("+++++++++++++rotationvector_probe!!\n");
	rotationvector_context_obj = rotationvector_context_alloc_object();
	if (!rotationvector_context_obj) {
		err = -ENOMEM;
		RV_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	/* init real rotationvectoreleration driver */
	err = rotationvector_real_driver_init();
	if (err) {
		RV_ERR("rotationvector real driver init fail\n");
		goto real_driver_init_fail;
	}
	/* init input dev */
	err = rotationvector_input_init(rotationvector_context_obj);
	if (err) {
		RV_ERR("unable to register rotationvector input device!\n");
		goto exit_alloc_input_dev_failed;
	}
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
	atomic_set(&(rotationvector_context_obj->early_suspend), 0);
	rotationvector_context_obj->early_drv.level = 1;	/* EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1, */
	rotationvector_context_obj->early_drv.suspend = rotationvector_early_suspend,
	    rotationvector_context_obj->early_drv.resume = rotationvector_late_resume,
	    register_early_suspend(&rotationvector_context_obj->early_drv);
#endif

	RV_LOG("----rotationvector_probe OK !!\n");
	return 0;

	/* exit_hwmsen_create_attr_failed: */
	/* exit_misc_register_failed: */

	/* exit_err_sysfs: */

	if (err) {
		RV_ERR("sysfs node creation error\n");
		rotationvector_input_destroy(rotationvector_context_obj);
	}

real_driver_init_fail:
exit_alloc_input_dev_failed:
	kfree(rotationvector_context_obj);

exit_alloc_data_failed:


	RV_LOG("----rotationvector_probe fail !!!\n");
	return err;
}



static int rotationvector_remove(struct platform_device *pdev)
{
	int err = 0;

	RV_FUN(f);
	input_unregister_device(rotationvector_context_obj->idev);
	sysfs_remove_group(&rotationvector_context_obj->idev->dev.kobj,
			   &rotationvector_attribute_group);

	err = misc_deregister(&rotationvector_context_obj->mdev);
	if (err)
		RV_ERR("misc_deregister fail: %d\n", err);

	kfree(rotationvector_context_obj);

	return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_EARLYSUSPEND)
static void rotationvector_early_suspend(struct early_suspend *h)
{
	atomic_set(&(rotationvector_context_obj->early_suspend), 1);
	RV_LOG(" rotationvector_early_suspend ok------->hwm_obj->early_suspend=%d\n",
	       atomic_read(&(rotationvector_context_obj->early_suspend)));
}

/*----------------------------------------------------------------------------*/

static void rotationvector_late_resume(struct early_suspend *h)
{
	atomic_set(&(rotationvector_context_obj->early_suspend), 0);
	RV_LOG(" rotationvector_late_resume ok------->hwm_obj->early_suspend=%d\n",
	       atomic_read(&(rotationvector_context_obj->early_suspend)));
}
#endif

static int rotationvector_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static int rotationvector_resume(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id m_rv_pl_of_match[] = {
	{.compatible = "mediatek,m_rv_pl",},
	{},
};
#endif

static struct platform_driver rotationvector_driver = {
	.probe = rotationvector_probe,
	.remove = rotationvector_remove,
	.suspend = rotationvector_suspend,
	.resume = rotationvector_resume,
	.driver = {
		   .name = RV_PL_DEV_NAME,
#ifdef CONFIG_OF
		   .of_match_table = m_rv_pl_of_match,
#endif
		   }
};

int rotationvector_driver_add(struct rotationvector_init_info *obj)
{
	int err = 0;
	int i = 0;

	RV_FUN();

	for (i = 0; i < MAX_CHOOSE_RV_NUM; i++) {
		if ((i == 0) && (NULL == rotationvectorsensor_init_list[0])) {
			RV_LOG("register gensor driver for the first time\n");
			if (platform_driver_register(&rotationvectorsensor_driver))
				RV_ERR("failed to register gensor driver already exist\n");
		}

		if (NULL == rotationvectorsensor_init_list[i]) {
			obj->platform_diver_addr = &rotationvectorsensor_driver;
			rotationvectorsensor_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_RV_NUM) {
		RV_ERR("RV driver add err\n");
		err = -1;
	}
	return err;
} EXPORT_SYMBOL_GPL(rotationvector_driver_add);

static int __init rotationvector_init(void)
{
	RV_FUN();

	if (platform_driver_register(&rotationvector_driver)) {
		RV_ERR("failed to register rv driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit rotationvector_exit(void)
{
	platform_driver_unregister(&rotationvector_driver);
	platform_driver_unregister(&rotationvectorsensor_driver);
}

late_initcall(rotationvector_init);
/* module_init(rotationvector_init); */
/* module_exit(rotationvector_exit); */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RVCOPE device driver");
MODULE_AUTHOR("Mediatek");
