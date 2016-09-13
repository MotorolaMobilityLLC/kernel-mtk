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


#include "step_counter.h"
#include <generated/autoconf.h>

/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 start  */
#ifndef CONFIG_MTK_BMI120_NEW
#define STEP_COUNTER_INT_MODE_SUPPORT
#endif
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 end */

/*Lenovo-sw weimh1 add 2016-7-7 begin:record invalid count for steps*/
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
#define STEP_COUNTER_INVALID_COUNT 50
int invalid_count = 0;
int step_suspend = 0; 
int num_notify = 0;
#endif
/*Lenovo-sw weimh1 add 2016-7-7 end:record invalid count for steps*/

static struct step_c_context *step_c_context_obj = NULL;

static struct step_c_init_info *step_counter_init_list[MAX_CHOOSE_STEP_C_NUM] = { 0 };

static void step_c_work_func(struct work_struct *work)
{
	struct step_c_context *cxt = NULL;
	/* hwm_sensor_data sensor_data; */
	uint64_t value;
	int status;
	int64_t nt;
	struct timespec time;
	int err;
	u64 rawData; /* mtk step counter interface api requirement  --add by liaoxl.lenovo 5.12.2015 */

	cxt = step_c_context_obj;

	if (NULL == cxt->step_c_data.get_data) {
		STEP_C_LOG("step_c driver not register data path\n");
		goto step_c_loop;  /* fix system reboot caused by NULL pointer  --add by liaoxl.lenovo 7.12.2015 */
	}

	time.tv_sec = time.tv_nsec = 0;
	time = get_monotonic_coarse();
	nt = time.tv_sec * 1000000000LL + time.tv_nsec;

	/* add wake lock to make sure data can be read before system suspend */
	/* fix 64bit access overflow issue   -- modified by liaoxl.lenovo 5.12.2015 start*/
	err = cxt->step_c_data.get_data(&rawData, &status);
	/* fix 64bit access overflow issue   -- modified by liaoxl.lenovo 5.12.2015 end */
	if (err) {
		STEP_C_ERR("get step_c data fails!!\n");
		goto step_c_loop;
	} else {
		if (true == cxt->is_first_data_after_enable) {
			cxt->is_first_data_after_enable = false;
			/* filter -1 value */
			if (STEP_C_INVALID_VALUE == cxt->drv_data.counter) {
				STEP_C_LOG(" read invalid data\n");
				goto step_c_loop;
			}
		}
		
		/* step counter is a on-change type sensor, don't report when same value -- modified by liaoxl.lenovo 5.12.2015 start*/
		if (0 == (rawData >> 31)) {
			value = (int)rawData;
		}
		else {
			value = (int)(rawData & 0x000000007FFFFFFF);
			STEP_C_ERR("get step_c data overflow 31bit!!\n" );
		}
		
		if (value != cxt->drv_data.counter) {
			cxt->drv_data.counter = value;
			cxt->drv_data.status = status;
/*Lenovo-sw weimh1 add 2016-7-7 begin:record invalid count for steps*/
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
			invalid_count = 0;
#endif
/*Lenovo-sw weimh1 add 2016-7-7 end:record invalid count for steps*/
		} else {
/*Lenovo-sw weimh1 add 2016-7-7:just report once when step not changed*/
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
			invalid_count++;
			if (invalid_count < 2 && num_notify == 1) {
				STEP_C_ERR("get step_c data not change,counter=%d!!\n",cxt->drv_data.counter );
				num_notify = 0;
				step_c_data_report(cxt->idev, cxt->drv_data.counter, cxt->drv_data.status);
			}
#endif
/*Lenovo-sw weimh1 add 2016-7-7 end:just report once when step not changed*/

			/* no new steps, no need to update */
			goto step_c_loop;
		}
	/* step counter is a on-change type sensor, don't report when same value -- modified by liaoxl.lenovo 5.12.2015 start*/
	}

	/* report data to input device */
	step_c_data_report(cxt->idev, cxt->drv_data.counter, cxt->drv_data.status);

step_c_loop:
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 start  */
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
//	if (true == cxt->is_polling_run) {
	if (invalid_count < STEP_COUNTER_INVALID_COUNT && (step_suspend == 0))
		mod_timer(&cxt->timer, jiffies + atomic_read(&cxt->delay) / (1000 / HZ));
	else {
		cxt->is_polling_run = false;
		invalid_count = 0;
		step_suspend = 0;
		num_notify = 0;
		STEP_C_ERR("come here,can suspend!!\n" );
	}
//	}
#endif
	value = 0;
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 end  */

	STEP_C_LOG("%s step counter:%d\n", __func__, cxt->drv_data.counter);
}

static void step_c_poll(unsigned long data)
{
	struct step_c_context *obj = (struct step_c_context *)data;

	if (obj != NULL)
		schedule_work(&obj->report);
}

static struct step_c_context *step_c_context_alloc_object(void)
{
	struct step_c_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	STEP_C_LOG("step_c_context_alloc_object++++\n");
	if (!obj) {
		STEP_C_ERR("Alloc step_c object error!\n");
		return NULL;
	}
	atomic_set(&obj->delay, 500);	/*2Hz */
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report, step_c_work_func);
	init_timer(&obj->timer);
	obj->timer.expires = jiffies + atomic_read(&obj->delay) / (1000 / HZ);
	obj->timer.function = step_c_poll;
	obj->timer.data = (unsigned long)obj;
	obj->is_first_data_after_enable = false;
	obj->is_polling_run = false;
	mutex_init(&obj->step_c_op_mutex);
	obj->is_batch_enable = false;	/* for batch mode init */
	/* init sensor status, add by liaoxl.lenovo 5.12.2015 start*/
	obj->is_step_d_active = false;
	obj->is_sigmot_active = false;
	obj->is_active_data = false;
	obj->is_active_nodata = false;
	/* init sensor status, add by liaoxl.lenovo 5.12.2015 end*/

	STEP_C_LOG("step_c_context_alloc_object----\n");
	return obj;
}

/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 start  */
int  step_notify(STEP_NOTIFY_TYPE type)
{
	int err = 0;
	int value = 0;
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	STEP_C_LOG("step_notify++ with type=%d\n", type);

	if (type == TYPE_STEP_DETECTOR) {
		STEP_C_LOG("fwq TYPE_STEP_DETECTOR notify\n");
		/* cxt->step_c_data.get_data_step_d(&value); */
		/* step_c_data_report(cxt->idev,value,3); */
		value = 1;
		input_report_rel(cxt->idev, EVENT_TYPE_STEP_DETECTOR_VALUE, value);
		input_sync(cxt->idev);
	}
#ifdef LSM6DS3_SIGNIFICANT_MOTION
	else if (type == TYPE_SIGNIFICANT) {
		STEP_C_LOG("fwq TYPE_SIGNIFICANT notify\n");
		/* cxt->step_c_data.get_data_significant(&value); */
		value = 1;
		input_report_rel(cxt->idev, EVENT_TYPE_SIGNIFICANT_VALUE, value);
		input_sync(cxt->idev);
	}
#endif	
	else if (type == TYPE_STEP_COUNTER)	{
		STEP_C_LOG("fwq TYPE_STEP_COUNTER notify\n");

#ifndef STEP_COUNTER_INT_MODE_SUPPORT	
		invalid_count = 0;
		step_suspend = 0;
		num_notify = 1;
#endif
		step_c_work_func(0);
	}
	/*Lenovo-sw weimh1 add 2016-8-22 begin:*/
	else if (type == TYPE_STEP_SUSPEND)	{
		STEP_C_LOG("fwq TYPE_STEP_SUSPEND notify\n");

#ifndef STEP_COUNTER_INT_MODE_SUPPORT	
		invalid_count = 0;
		step_suspend = 1;
		num_notify = 1;
#endif
		step_c_work_func(0);
#if 0
		cxt->is_polling_run = false;
		del_timer_sync(&step_c_context_obj->timer);
		cancel_work_sync(&step_c_context_obj->report);
#endif
	}
	/*Lenovo-sw weimh1 add 2016-8-22 end*/

	return err;
}
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 end  */

/* fix step counter stop working after disable other sensor issue -- modified by liaoxl.lenovo 5.12.2015 start  */
static int step_d_real_enable(int enable)
{
	int err =0, old;
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	if (false == cxt->is_step_d_active)
		old = 0;
	else
		old = 1;
		
	if(old == enable)
		return 0;
		
	if (1 == enable) {
		err = cxt->step_c_ctl.enable_step_detect(1);
		if (err) {
			err = cxt->step_c_ctl.enable_step_detect(1);
			if (err) {
				err = cxt->step_c_ctl.enable_step_detect(1);
				if (err)
					STEP_C_ERR("step_d enable(%d) err 3 timers = %d\n", enable,
						   err);
			}
			else
	   		{
				cxt->is_step_d_active = true;
	    	}
		}
		else
	    {
			cxt->is_step_d_active = true;
	    }
		
		STEP_C_LOG("step_d real enable\n");
	}
	if (0 == enable) {

		err = cxt->step_c_ctl.enable_step_detect(0);
		if (err) {
			STEP_C_ERR("step_d enable(%d) err = %d\n", enable, err);
		}
		else {
			cxt->is_step_d_active = false;
		}
		STEP_C_LOG("step_d real disable  \n" );
	}

	return err;
}

#ifdef LSM6DS3_SIGNIFICANT_MOTION
static int significant_real_enable(int enable)
{
	int err =0, old;
	struct step_c_context *cxt = NULL;
	
	cxt = step_c_context_obj;
	if(false == cxt->is_sigmot_active)
		old = 0;
	else
		old = 1;
	if (old == enable)
		return 0;
		
	if (1 == enable) {
		err = cxt->step_c_ctl.enable_significant(1);
		if (err) {
			err = cxt->step_c_ctl.enable_significant(1);
			if (err) {
				err = cxt->step_c_ctl.enable_significant(1);
				if (err)
					STEP_C_ERR
					    ("enable_significant enable(%d) err 3 timers = %d\n",
					     enable, err);
			}
			else {
				cxt->is_sigmot_active = true;
	    	}
		}
		else {
			cxt->is_sigmot_active = true;
	    }
		
		STEP_C_LOG("enable_significant real enable\n");
	}
	if (0 == enable) {
		err = cxt->step_c_ctl.enable_significant(0);
		if (err)
		{ 
			STEP_C_ERR("enable_significantenable(%d) err = %d\n", enable, err);
		}
		else
		{
			cxt->is_sigmot_active = false;
		}

		STEP_C_LOG("enable_significant real disable  \n" );	 
	}
	return err;
}
#endif
/* fix step counter stop working after disable other sensor issue -- modified by liaoxl.lenovo 5.12.2015 end */

static int step_c_real_enable(int enable)
{
	int err = 0;
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	if (1 == enable) {
		if (true == cxt->is_active_data || true == cxt->is_active_nodata) {
			err = cxt->step_c_ctl.enable_nodata(1);
			if (err) {
				err = cxt->step_c_ctl.enable_nodata(1);
				if (err) {
					err = cxt->step_c_ctl.enable_nodata(1);
					if (err)
						STEP_C_ERR("step_c enable(%d) err 3 timers = %d\n",
							   enable, err);
				}
			}
			STEP_C_LOG("step_c real enable\n");
		}

	}
	if (0 == enable) {
		if (false == cxt->is_active_data && false == cxt->is_active_nodata) {
			err = cxt->step_c_ctl.enable_nodata(0);
			if (err)
				STEP_C_ERR("step_c enable(%d) err = %d\n", enable, err);
			STEP_C_LOG("step_c real disable\n");
		}

	}

	return err;
}

static int step_c_enable_data(int enable)
{
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	if (NULL == cxt->step_c_ctl.open_report_data) {
		STEP_C_ERR("no step_c control path\n");
		return -1;
	}

	if (1 == enable) {
		STEP_C_LOG("STEP_C enable data\n");
		cxt->is_active_data = true;
		cxt->is_first_data_after_enable = true;
		cxt->step_c_ctl.open_report_data(1);
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 start  */
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
		invalid_count = 0;
		if (false == cxt->is_polling_run && cxt->is_batch_enable == false) {
			if (false == cxt->step_c_ctl.is_report_input_direct) {
				mod_timer(&cxt->timer,
					  jiffies + atomic_read(&cxt->delay) / (1000 / HZ));
				cxt->is_polling_run = true;
			}
		}
#endif
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 end  */
	}
	if (0 == enable) {
		STEP_C_LOG("STEP_C disable\n");
		cxt->is_active_data = false;
		cxt->step_c_ctl.open_report_data(0);
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
		invalid_count = 0;
		if (true == cxt->is_polling_run) {
			if (false == cxt->step_c_ctl.is_report_input_direct) {
				cxt->is_polling_run = false;
				del_timer_sync(&cxt->timer);
				cancel_work_sync(&cxt->report);
//				cxt->drv_data.counter = STEP_C_INVALID_VALUE;
			}
		}
#endif
	}
	step_c_real_enable(enable);
	return 0;
}

int step_c_enable_nodata(int enable)
{
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	if (NULL == cxt->step_c_ctl.enable_nodata) {
		STEP_C_ERR("step_c_enable_nodata:step_c ctl path is NULL\n");
		return -1;
	}

	if (1 == enable)
		cxt->is_active_nodata = true;

	if (0 == enable)
		cxt->is_active_nodata = false;
	step_c_real_enable(enable);
	return 0;
}


static ssize_t step_c_show_enable_nodata(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	int len = 0;

	STEP_C_LOG(" not support now\n");
	return len;
}

static ssize_t step_c_store_enable_nodata(struct device *dev, struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int err = 0;
	struct step_c_context *cxt = NULL;

	STEP_C_LOG("step_c_store_enable nodata buf=%s\n", buf);
	mutex_lock(&step_c_context_obj->step_c_op_mutex);
	cxt = step_c_context_obj;
	if (NULL == cxt->step_c_ctl.enable_nodata) {
		STEP_C_LOG("step_c_ctl enable nodata NULL\n");
		mutex_unlock(&step_c_context_obj->step_c_op_mutex);
		return count;
	}
	if (!strncmp(buf, "1", 1))
		err = step_c_enable_nodata(1);
	else if (!strncmp(buf, "0", 1))
		err = step_c_enable_nodata(0);
	else
		STEP_C_ERR(" step_c_store enable nodata cmd error !!\n");
	mutex_unlock(&step_c_context_obj->step_c_op_mutex);
	return err;
}

static ssize_t step_c_store_active(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct step_c_context *cxt = NULL;
	int res = 0;
	int handle = 0;
	int en = 0;

	STEP_C_LOG("step_c_store_active buf=%s\n", buf);
	mutex_lock(&step_c_context_obj->step_c_op_mutex);

	cxt = step_c_context_obj;
	if (NULL == cxt->step_c_ctl.open_report_data) {
		STEP_C_LOG("step_c_ctl enable NULL\n");
		mutex_unlock(&step_c_context_obj->step_c_op_mutex);
		return count;
	}
	res = sscanf(buf, "%d,%d", &handle, &en);
	if (res != 2)
		STEP_C_LOG(" step_store_active param error: res = %d\n", res);
	STEP_C_LOG(" step_store_active handle=%d ,en=%d\n", handle, en);
	switch (handle) {
	case ID_STEP_COUNTER:
		if (1 == en)
			step_c_enable_data(1);
		else if (0 == en)
			step_c_enable_data(0);
		else
			STEP_C_ERR(" step_c_store_active error !!\n");
		break;
		
	case ID_STEP_DETECTOR:
		if (1 == en)
			step_d_real_enable(1);
		else if (0 == en)
			step_d_real_enable(0);
		else
			STEP_C_ERR(" step_d_real_enable error !!\n");
		break;
		
#ifdef LSM6DS3_SIGNIFICANT_MOTION		
	case ID_SIGNIFICANT_MOTION:
		if (1 == en)
			significant_real_enable(1);
		else if (0 == en)
			significant_real_enable(0);
		else
			STEP_C_ERR(" significant_real_enable error !!\n");
		break;
#endif
	}

	/*
	   if (!strncmp(buf, "1", 1))
	   {
	   step_c_enable_data(1);
	   }
	   else if (!strncmp(buf, "0", 1))
	   {
	   step_c_enable_data(0);
	   }
	   else
	   {
	   STEP_C_ERR(" step_c_store_active error !!\n");
	   }
	 */
	mutex_unlock(&step_c_context_obj->step_c_op_mutex);
	STEP_C_LOG(" step_c_store_active done\n");
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t step_c_show_active(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct step_c_context *cxt = NULL;
	int enc,div;

	cxt = step_c_context_obj;
	div = cxt->step_c_data.vender_div;
	STEP_C_LOG("step_c vender_div value: %d\n", div);
	enc = (int)cxt->is_active_data;
	return snprintf(buf, PAGE_SIZE, "%d - (%d)\n", div, enc); 
}

static ssize_t step_c_store_delay(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int delay;
	int mdelay = 0;
	struct step_c_context *cxt = NULL;

	mutex_lock(&step_c_context_obj->step_c_op_mutex);
	cxt = step_c_context_obj;
	if (NULL == cxt->step_c_ctl.set_delay) {
		STEP_C_LOG("step_c_ctl set_delay NULL\n");
		mutex_unlock(&step_c_context_obj->step_c_op_mutex);
		return count;
	}

	if (0 != kstrtoint(buf, 10, &delay)) {
		STEP_C_ERR("invalid format!!\n");
		mutex_unlock(&step_c_context_obj->step_c_op_mutex);
		return count;
	}

	if (false == cxt->step_c_ctl.is_report_input_direct) {
		mdelay = (int)delay / 1000 / 1000;
		atomic_set(&step_c_context_obj->delay, mdelay);
	}
	cxt->step_c_ctl.set_delay(delay);
	STEP_C_LOG(" step_c_delay %d ns\n", delay);
	mutex_unlock(&step_c_context_obj->step_c_op_mutex);
	return count;

}

static ssize_t step_c_show_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	STEP_C_LOG(" not support now\n");
	return len;
}


static ssize_t step_c_store_batch(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct step_c_context *cxt = NULL;

	STEP_C_LOG("step_c_store_batch buf=%s\n", buf);
	mutex_lock(&step_c_context_obj->step_c_op_mutex);

	cxt = step_c_context_obj;

	if (!strncmp(buf, "1", 1)) {
		cxt->is_batch_enable = true;
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
		invalid_count = 0;
		if (true == cxt->is_polling_run) {
			cxt->is_polling_run = false;
			del_timer_sync(&cxt->timer);
			cancel_work_sync(&cxt->report);
//			cxt->drv_data.counter = STEP_C_INVALID_VALUE;
		}
#endif
	} else if (!strncmp(buf, "0", 1)) {
		cxt->is_batch_enable = false;
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 start  */
#ifndef STEP_COUNTER_INT_MODE_SUPPORT
		invalid_count = 0;
		if (false == cxt->is_polling_run) {
			if (false == cxt->step_c_ctl.is_report_input_direct) {
				mod_timer(&cxt->timer,
					  jiffies + atomic_read(&cxt->delay) / (1000 / HZ));
				cxt->is_polling_run = true;
				num_notify = 1;
			}
		}
#endif
/* step counter sensor interrupt mode support -- modified by liaoxl.lenovo 7.12.2015 end  */
	} else {
		STEP_C_ERR(" step_c_store_batch error !!\n");
	}
	mutex_unlock(&step_c_context_obj->step_c_op_mutex);
	STEP_C_LOG(" step_c_store_batch done: %d\n", cxt->is_batch_enable);
	return count;
}

static ssize_t step_c_show_batch(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t step_c_store_flush(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	return count;
}

static ssize_t step_c_show_flush(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t step_c_show_devnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *devname = NULL;

	devname = dev_name(&step_c_context_obj->idev->dev);
	return snprintf(buf, PAGE_SIZE, "%s\n", devname + 5);
}

static int step_counter_remove(struct platform_device *pdev)
{
	STEP_C_LOG("step_counter_remove\n");
	return 0;
}

static int step_counter_probe(struct platform_device *pdev)
{
	STEP_C_LOG("step_counter_probe\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id step_counter_of_match[] = {
	{.compatible = "mediatek,step_counter",},
	{},
};
#endif

static struct platform_driver step_counter_driver = {
	.probe = step_counter_probe,
	.remove = step_counter_remove,
	.driver = {

		   .name = "step_counter",
#ifdef CONFIG_OF
		   .of_match_table = step_counter_of_match,
#endif
		   }
};

static int step_c_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	STEP_C_LOG(" step_c_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_STEP_C_NUM; i++) {
		STEP_C_LOG(" i=%d\n", i);
		if (0 != step_counter_init_list[i]) {
			STEP_C_LOG(" step_c try to init driver %s\n",
				   step_counter_init_list[i]->name);
			err = step_counter_init_list[i]->init();
			if (0 == err) {
				STEP_C_LOG(" step_c real driver %s probe ok\n",
					   step_counter_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_STEP_C_NUM) {
		STEP_C_LOG(" step_c_real_driver_init fail\n");
		err = -1;
	}
	return err;
}

int step_c_driver_add(struct step_c_init_info *obj)
{
	int err = 0;
	int i = 0;

	STEP_C_FUN();
	for (i = 0; i < MAX_CHOOSE_STEP_C_NUM; i++) {
		if (i == 0) {
			STEP_C_LOG("register step_counter driver for the first time\n");
			if (platform_driver_register(&step_counter_driver))
				STEP_C_ERR("failed to register gensor driver already exist\n");
		}

		if (NULL == step_counter_init_list[i]) {
			obj->platform_diver_addr = &step_counter_driver;
			step_counter_init_list[i] = obj;
			break;
		}
	}
	if (NULL == step_counter_init_list[i]) {
		STEP_C_ERR("STEP_C driver add err\n");
		err = -1;
	}

	return err;
} 
EXPORT_SYMBOL_GPL(step_c_driver_add);

static int step_c_misc_init(struct step_c_context *cxt)
{

	int err = 0;
	/* kernel-3.10\include\linux\Miscdevice.h */
	/* use MISC_DYNAMIC_MINOR exceed 64 */
	cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name = STEP_C_MISC_DEV_NAME;
	err = misc_register(&cxt->mdev);
	if (err)
		STEP_C_ERR("unable to register step_c misc device!!\n");
	return err;
}

static void step_c_input_destroy(struct step_c_context *cxt)
{
	struct input_dev *dev = cxt->idev;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int step_c_input_init(struct step_c_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = STEP_C_INPUTDEV_NAME;
	input_set_capability(dev, EV_REL, EVENT_TYPE_STEP_DETECTOR_VALUE);
	input_set_capability(dev, EV_REL, EVENT_TYPE_SIGNIFICANT_VALUE);
	input_set_capability(dev, EV_REL, EVENT_TYPE_STEP_C_VALUE);
	input_set_capability(dev, EV_ABS, EVENT_TYPE_STEP_C_STATUS);

	input_set_abs_params(dev, EVENT_TYPE_STEP_C_VALUE, STEP_C_VALUE_MIN, STEP_C_VALUE_MAX, 0,
			     0);
	input_set_abs_params(dev, EVENT_TYPE_STEP_C_STATUS, STEP_C_STATUS_MIN, STEP_C_STATUS_MAX, 0,
			     0);
	input_set_drvdata(dev, cxt);
	set_bit(EV_REL, dev->evbit);
	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev = dev;

	return 0;
}

DEVICE_ATTR(step_cenablenodata, S_IWUSR | S_IRUGO, step_c_show_enable_nodata,
	    step_c_store_enable_nodata);
DEVICE_ATTR(step_cactive, S_IWUSR | S_IRUGO, step_c_show_active, step_c_store_active);
DEVICE_ATTR(step_cdelay, S_IWUSR | S_IRUGO, step_c_show_delay, step_c_store_delay);
DEVICE_ATTR(step_cbatch, S_IWUSR | S_IRUGO, step_c_show_batch, step_c_store_batch);
DEVICE_ATTR(step_cflush, S_IWUSR | S_IRUGO, step_c_show_flush, step_c_store_flush);
DEVICE_ATTR(step_cdevnum, S_IWUSR | S_IRUGO, step_c_show_devnum, NULL);


static struct attribute *step_c_attributes[] = {
	&dev_attr_step_cenablenodata.attr,
	&dev_attr_step_cactive.attr,
	&dev_attr_step_cdelay.attr,
	&dev_attr_step_cbatch.attr,
	&dev_attr_step_cflush.attr,
	&dev_attr_step_cdevnum.attr,
	NULL
};

static struct attribute_group step_c_attribute_group = {
	.attrs = step_c_attributes
};

int step_c_register_data_path(struct step_c_data_path *data)
{
	struct step_c_context *cxt = NULL;

	cxt = step_c_context_obj;
	cxt->step_c_data.get_data = data->get_data;
	cxt->step_c_data.vender_div = data->vender_div;
#ifdef LSM6DS3_SIGNIFICANT_MOTION	
	cxt->step_c_data.get_data_significant = data->get_data_significant;
#endif
	cxt->step_c_data.get_data_step_d = data->get_data_step_d;
	STEP_C_LOG("step_c register data path vender_div: %d\n", cxt->step_c_data.vender_div);
	if (NULL == cxt->step_c_data.get_data
#ifdef LSM6DS3_SIGNIFICANT_MOTION		
	    || NULL == cxt->step_c_data.get_data_significant
#endif	    
	    || NULL == cxt->step_c_data.get_data_step_d) {
		STEP_C_LOG("step_c register data path fail\n");
		return -1;
	}
	return 0;
}

int step_c_register_control_path(struct step_c_control_path *ctl)
{
	struct step_c_context *cxt = NULL;
	int err = 0;

	cxt = step_c_context_obj;
	cxt->step_c_ctl.set_delay = ctl->set_delay;
	cxt->step_c_ctl.open_report_data = ctl->open_report_data;
	cxt->step_c_ctl.enable_nodata = ctl->enable_nodata;
	cxt->step_c_ctl.is_support_batch = ctl->is_support_batch;
	cxt->step_c_ctl.is_report_input_direct = ctl->is_report_input_direct;
	cxt->step_c_ctl.is_support_batch = ctl->is_support_batch;
#ifdef LSM6DS3_SIGNIFICANT_MOTION	
	cxt->step_c_ctl.enable_significant = ctl->enable_significant;
#endif
	cxt->step_c_ctl.enable_step_detect = ctl->enable_step_detect;

	if (NULL == cxt->step_c_ctl.set_delay || NULL == cxt->step_c_ctl.open_report_data
	    || NULL == cxt->step_c_ctl.enable_nodata
#ifdef LSM6DS3_SIGNIFICANT_MOTION	    
	    || NULL == cxt->step_c_ctl.enable_significant
#endif	    
	    || NULL == cxt->step_c_ctl.enable_step_detect) {
		STEP_C_LOG("step_c register control path fail\n");
		return -1;
	}

	/* add misc dev for sensor hal control cmd */
	err = step_c_misc_init(step_c_context_obj);
	if (err) {
		STEP_C_ERR("unable to register step_c misc device!!\n");
		return -2;
	}
	err = sysfs_create_group(&step_c_context_obj->mdev.this_device->kobj,
				 &step_c_attribute_group);
	if (err < 0) {
		STEP_C_ERR("unable to create step_c attribute file\n");
		return -3;
	}

	kobject_uevent(&step_c_context_obj->mdev.this_device->kobj, KOBJ_ADD);

	return 0;
}

int step_c_data_report(struct input_dev *dev, uint32_t value, int status)
{
	/* STEP_C_LOG("+step_c_data_report! %d, %d, %d, %d\n",x,y,z,status); */
	input_report_abs(dev, EVENT_TYPE_STEP_C_VALUE, value);
	input_report_abs(dev, EVENT_TYPE_STEP_C_STATUS, status);
	input_sync(dev);
	return 0;
}

static int step_c_probe(void)
{

	int err;

	STEP_C_LOG("+++++++++++++step_c_probe!!\n");

	step_c_context_obj = step_c_context_alloc_object();
	if (!step_c_context_obj) {
		err = -ENOMEM;
		STEP_C_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	/* init real step_c driver */
	err = step_c_real_driver_init();
	if (err) {
		STEP_C_ERR("step_c real driver init fail\n");
		goto real_driver_init_fail;
	}

	/* init input dev */
	err = step_c_input_init(step_c_context_obj);
	if (err) {
		STEP_C_ERR("unable to register step_c input device!\n");
		goto exit_alloc_input_dev_failed;
	}


	STEP_C_LOG("----step_c_probe OK !!\n");
	return 0;
exit_alloc_input_dev_failed:
	step_c_input_destroy(step_c_context_obj);
real_driver_init_fail:
	kfree(step_c_context_obj);
exit_alloc_data_failed:
	STEP_C_LOG("----step_c_probe fail !!!\n");
	return err;
}



static int step_c_remove(void)
{

	int err = 0;

	STEP_C_FUN(f);
	input_unregister_device(step_c_context_obj->idev);
	sysfs_remove_group(&step_c_context_obj->idev->dev.kobj, &step_c_attribute_group);

	err = misc_deregister(&step_c_context_obj->mdev);
	if (err)
		STEP_C_ERR("misc_deregister fail: %d\n", err);
	kfree(step_c_context_obj);

	return 0;
}

static int __init step_c_init(void)
{
	STEP_C_FUN();

	if (step_c_probe()) {
		STEP_C_ERR("failed to register step_c driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit step_c_exit(void)
{
	step_c_remove();
	platform_driver_unregister(&step_counter_driver);
}

late_initcall(step_c_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("STEP_CMETER device driver");
MODULE_AUTHOR("Mediatek");
