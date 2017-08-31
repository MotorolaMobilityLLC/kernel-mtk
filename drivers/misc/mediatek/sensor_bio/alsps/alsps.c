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

#include "inc/alsps.h"
#include "inc/aal_control.h"
struct alsps_context *alsps_context_obj/* = NULL*/;
struct platform_device *pltfm_dev;
int last_als_report_data = -1;

/* AAL default delay timer(nano seconds)*/
#define AAL_DELAY	200000000

static struct alsps_init_info *alsps_init_list[MAX_CHOOSE_ALSPS_NUM] = {0};

int als_data_report(int value, int status)
{
	int err = 0;
	struct alsps_context *cxt = NULL;
	struct sensor_event event;

	cxt  = alsps_context_obj;
	/*ALSPS_LOG(" +als_data_report! %d, %d\n", value, status);*/
	/* force trigger data update after sensor enable. */
	if (cxt->is_get_valid_als_data_after_enable == false) {
		event.flush_action = DATA_ACTION;
		event.word[0] = value + 1;
		err = sensor_input_event(cxt->als_mdev.minor, &event);
		if (err < 0)
			pr_err_ratelimited("event buffer full, so drop this data\n");
		cxt->is_get_valid_als_data_after_enable = true;
	}
	if (value != last_als_report_data) {
		event.flush_action = DATA_ACTION;
		event.word[0] = value;
		event.status = status;
		err = sensor_input_event(cxt->als_mdev.minor, &event);
		if (err < 0)
			pr_err_ratelimited("event buffer full, so drop this data\n");
		last_als_report_data = value;
	}
	return err;
}

int als_flush_report(void)
{
	struct sensor_event event;
	int err = 0;

	event.flush_action = FLUSH_ACTION;
	err = sensor_input_event(alsps_context_obj->als_mdev.minor, &event);
	if (err < 0)
		pr_err_ratelimited("event buffer full, so drop this data\n");
	else
		ALSPS_LOG("flush\n");
	return err;
}

int ps_data_report(int value, int status)
{
	int err = 0;
	struct sensor_event event;

	pr_warn("[ALS/PS]ps_data_report! %d, %d\n", value, status);
	event.flush_action = DATA_ACTION;
	event.word[0] = value + 1;
	event.status = status;
	err = sensor_input_event(alsps_context_obj->ps_mdev.minor, &event);
	if (err < 0)
		pr_err_ratelimited("event buffer full, so drop this data\n");
	return err;
}
int ps_flush_report(void)
{
	struct sensor_event event;
	int err = 0;

	event.flush_action = FLUSH_ACTION;
	err = sensor_input_event(alsps_context_obj->ps_mdev.minor, &event);
	if (err < 0)
		pr_err_ratelimited("event buffer full, so drop this data\n");
	else
		ALSPS_LOG("flush\n");
	return err;
}
static void als_work_func(struct work_struct *work)
{
	struct alsps_context *cxt = NULL;
	int value, status;
	int64_t  nt;
	struct timespec time;
	int err;

	cxt  = alsps_context_obj;
	if (cxt->als_data.get_data == NULL) {
		ALSPS_ERR("alsps driver not register data path\n");
		return;
	}

	time.tv_sec = time.tv_nsec = 0;
	time = get_monotonic_coarse();
	nt = time.tv_sec*1000000000LL+time.tv_nsec;
	/* add wake lock to make sure data can be read before system suspend */
	err = cxt->als_data.get_data(&value, &status);
	if (err) {
		ALSPS_ERR("get alsps data fails!!\n");
		goto als_loop;
	} else {
		cxt->drv_data.als_data.values[0] = value;
		cxt->drv_data.als_data.status = status;
		cxt->drv_data.als_data.time = nt;
	}

	if (true ==  cxt->is_als_first_data_after_enable) {
		cxt->is_als_first_data_after_enable = false;
		/* filter -1 value */
		if (cxt->drv_data.als_data.values[0] == ALSPS_INVALID_VALUE) {
			ALSPS_LOG(" read invalid data\n");
			goto als_loop;
		}
	}
	/* ALSPS_LOG(" als data[%d]\n" , cxt->drv_data.als_data.values[0]); */
	als_data_report(cxt->drv_data.als_data.values[0],
	cxt->drv_data.als_data.status);

als_loop:
	if (true == cxt->is_als_polling_run)
		mod_timer(&cxt->timer_als, jiffies + atomic_read(&cxt->delay_als)/(1000/HZ));
}

static void ps_work_func(struct work_struct *work)
{

	struct alsps_context *cxt = NULL;
	int value, status;
	int64_t  nt;
	struct timespec time;
	int err = 0;

	cxt  = alsps_context_obj;
	if (cxt->ps_data.get_data == NULL) {
		ALSPS_ERR("alsps driver not register data path\n");
		return;
	}

	time.tv_sec = time.tv_nsec = 0;
	time = get_monotonic_coarse();
	nt = time.tv_sec*1000000000LL+time.tv_nsec;

	/* add wake lock to make sure data can be read before system suspend */
	err = cxt->ps_data.get_data(&value, &status);
	if (err) {
		ALSPS_ERR("get alsps data fails!!\n");
		goto ps_loop;
	} else {
		cxt->drv_data.ps_data.values[0] = value;
		cxt->drv_data.ps_data.status = status;
		cxt->drv_data.ps_data.time = nt;
	}

	if (true ==  cxt->is_ps_first_data_after_enable) {
		cxt->is_ps_first_data_after_enable = false;
		/* filter -1 value */
		if (cxt->drv_data.ps_data.values[0] == ALSPS_INVALID_VALUE) {
			ALSPS_LOG(" read invalid data\n");
			goto ps_loop;
		}
	}

	if (cxt->is_get_valid_ps_data_after_enable == false) {
		if (cxt->drv_data.ps_data.values[0] != ALSPS_INVALID_VALUE)
			cxt->is_get_valid_ps_data_after_enable = true;
	}

	ps_data_report(cxt->drv_data.ps_data.values[0],
	cxt->drv_data.ps_data.status);

ps_loop:
	if (true == cxt->is_ps_polling_run) {
		if (cxt->ps_ctl.is_polling_mode || (cxt->is_get_valid_ps_data_after_enable == false))
			mod_timer(&cxt->timer_ps, jiffies + atomic_read(&cxt->delay_ps)/(1000/HZ));
	}
}

static void als_poll(unsigned long data)
{
	struct alsps_context *obj = (struct alsps_context *)data;

	if ((obj != NULL) && (obj->is_als_polling_run))
		schedule_work(&obj->report_als);
}

static void ps_poll(unsigned long data)
{
	struct alsps_context *obj = (struct alsps_context *)data;

	if (obj != NULL)
		schedule_work(&obj->report_ps);
}

static struct alsps_context *alsps_context_alloc_object(void)
{
	struct alsps_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	ALSPS_LOG("alsps_context_alloc_object++++\n");
	if (!obj) {
		ALSPS_ERR("Alloc alsps object error!\n");
		return NULL;
	}
	atomic_set(&obj->delay_als, 200); /*5Hz, set work queue delay time 200ms */
	atomic_set(&obj->delay_ps, 200); /* 5Hz,  set work queue delay time 200ms */
	atomic_set(&obj->wake, 0);
	INIT_WORK(&obj->report_als, als_work_func);
	INIT_WORK(&obj->report_ps, ps_work_func);
	init_timer(&obj->timer_als);
	init_timer(&obj->timer_ps);
	obj->timer_als.expires	= jiffies + atomic_read(&obj->delay_als)/(1000/HZ);
	obj->timer_als.function	= als_poll;
	obj->timer_als.data	= (unsigned long)obj;

	obj->timer_ps.expires	= jiffies + atomic_read(&obj->delay_ps)/(1000/HZ);
	obj->timer_ps.function	= ps_poll;
	obj->timer_ps.data	= (unsigned long)obj;

	obj->is_als_first_data_after_enable = false;
	obj->is_als_polling_run = false;
	obj->is_ps_first_data_after_enable = false;
	obj->is_ps_polling_run = false;
	mutex_init(&obj->alsps_op_mutex);
	obj->is_als_batch_enable = false;/* for batch mode init */
	obj->is_ps_batch_enable = false;/* for batch mode init */
	obj->als_power = 0;
	obj->als_enable = 0;
	obj->als_delay_ns = -1;
	obj->als_latency_ns = -1;
	obj->ps_power = 0;
	obj->ps_enable = 0;
	obj->ps_delay_ns = -1;
	obj->ps_latency_ns = -1;

	ALSPS_LOG("alsps_context_alloc_object----\n");
	return obj;
}

#ifndef CONFIG_NANOHUB
static int als_enable_and_batch(void)
{
	struct alsps_context *cxt = alsps_context_obj;
	int err;

	/* als_power on -> power off */
	if (cxt->als_power == 1 && cxt->als_enable == 0) {
		ALSPS_LOG("ALSPS disable\n");
		/* stop polling firstly, if needed */
		if (cxt->als_ctl.is_report_input_direct == false
			&& cxt->is_als_polling_run == true) {
			smp_mb();/* for memory barrier */
			del_timer_sync(&cxt->timer_als);
			smp_mb();/* for memory barrier */
			cancel_work_sync(&cxt->report_als);
			cxt->drv_data.als_data.values[0] = ALSPS_INVALID_VALUE;
			cxt->is_als_polling_run = false;
			ALSPS_LOG("als stop polling done\n");
		}
		/* turn off the als_power */
		ALSPS_LOG("AAL status is %d\n", aal_use);
		if (aal_use == 0)
			err = cxt->als_ctl.enable_nodata(0);
		else
			err = cxt->als_ctl.batch(0, AAL_DELAY, 0);
		if (err) {
			ALSPS_ERR("als turn off als_power err = %d\n", err);
			return -1;
		}
		ALSPS_LOG("als turn off als_power done\n");

		cxt->als_power = 0;
		cxt->als_delay_ns = -1;
		ALSPS_LOG("ALSPS disable done\n");
		return 0;
	}
	/* als_power off -> power on */
	if (cxt->als_power == 0 && cxt->als_enable == 1) {
		ALSPS_LOG("ALSPS als_power on\n");
		err = cxt->als_ctl.enable_nodata(1);
		if (err) {
			ALSPS_ERR("als turn on als_power err = %d\n", err);
			return -1;
		}
		ALSPS_LOG("als turn on als_power done\n");

		cxt->als_power = 1;
		ALSPS_LOG("ALSPS als_power on done\n");
	}
	/* rate change */
	if (cxt->als_power == 1 && cxt->als_delay_ns >= 0) {
		ALSPS_LOG("ALSPS set batch\n");
		/* set ODR, fifo timeout latency */
		if (cxt->als_ctl.is_support_batch)
			err = cxt->als_ctl.batch(0, cxt->als_delay_ns, cxt->als_latency_ns);
		else
			err = cxt->als_ctl.batch(0, cxt->als_delay_ns, 0);
		if (err) {
			ALSPS_ERR("als set batch(ODR) err %d\n", err);
			return -1;
		}
		ALSPS_LOG("als set ODR, fifo latency done\n");
		/* start polling, if needed */
		if (cxt->als_ctl.is_report_input_direct == false) {
			int mdelay = cxt->als_delay_ns;

			do_div(mdelay, 1000000);
			/* defaut max polling delay */
			if (mdelay < 10)
				mdelay = 10;
			atomic_set(&cxt->delay_als, mdelay);
			/* the first sensor start polling timer */
			if (cxt->is_als_polling_run == false) {
				mod_timer(&cxt->timer_als, jiffies + atomic_read(&cxt->delay_als)/(1000/HZ));
				cxt->is_als_polling_run = true;
				cxt->is_als_first_data_after_enable = true;
			}
			ALSPS_LOG("als set polling delay %d ms\n", atomic_read(&cxt->delay_als));
		}
		ALSPS_LOG("ALSPS batch done\n");
	}
	/* just for debug, remove it when everything is ok */
	if (cxt->als_power == 0 && cxt->als_delay_ns >= 0)
		ALSPS_ERR("batch will call firstly in API1.3, do nothing\n");

	return 0;
}
#endif

static ssize_t als_store_active(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = alsps_context_obj;
	int err = 0;

	ALSPS_LOG("als_store_active buf=%s\n", buf);
	mutex_lock(&alsps_context_obj->alsps_op_mutex);

	if (!strncmp(buf, "1", 1)) {
		cxt->als_enable = 1;
		last_als_report_data = -1;
		cxt->is_als_active_data = true;
	} else if (!strncmp(buf, "0", 1)) {
		cxt->als_enable = 0;
		cxt->is_als_active_data = false;
	} else {
		ALSPS_ERR(" alsps_store_active error !!\n");
		err = -1;
		goto err_out;
	}
#ifdef CONFIG_NANOHUB
	if (cxt->als_enable) {
		err = cxt->als_ctl.enable_nodata(cxt->als_enable);
		if (err) {
			ALSPS_ERR("als turn on als_power err = %d\n", err);
			return -1;
		}
	} else {
		if (aal_use == 0)
			err = cxt->als_ctl.enable_nodata(cxt->als_enable);
		else
			err = cxt->als_ctl.batch(0, AAL_DELAY, 0);
		if (err) {
			ALSPS_ERR("als turn off als_power err = %d\n", err);
			return -1;
		}
	}
#else
	err = als_enable_and_batch();
#endif

err_out:
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	ALSPS_LOG(" als_store_active done\n");
	return err;
}
/*----------------------------------------------------------------------------*/
static ssize_t als_show_active(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct alsps_context *cxt = NULL;
	int div = 0;

	cxt = alsps_context_obj;
	div = cxt->als_data.vender_div;
	ALSPS_LOG("als vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div);
}

static ssize_t als_store_batch(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = alsps_context_obj;
	int handle = 0, flag = 0, err = 0;

	ALSPS_LOG("als_store_batch %s\n", buf);
	err = sscanf(buf, "%d,%d,%lld,%lld", &handle, &flag,
			&cxt->als_delay_ns, &cxt->als_latency_ns);
	if (err != 4)
		ALSPS_ERR("als_store_batch param error: err = %d\n", err);

	if (aal_use)
		cxt->als_delay_ns = cxt->als_delay_ns < AAL_DELAY ? cxt->als_delay_ns : AAL_DELAY;

	mutex_lock(&alsps_context_obj->alsps_op_mutex);
#ifdef CONFIG_NANOHUB
	if (cxt->als_ctl.is_support_batch)
		err = cxt->als_ctl.batch(0, cxt->als_delay_ns, cxt->als_latency_ns);
	else
		err = cxt->als_ctl.batch(0, cxt->als_delay_ns, 0);
#else
	err = als_enable_and_batch();
#endif
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	ALSPS_LOG(" als_store_batch done: %d\n", cxt->is_als_batch_enable);
	return err;
}

static ssize_t als_show_batch(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t als_store_flush(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = NULL;
	int handle = 0, err = 0;

	err = kstrtoint(buf, 10, &handle);
	if (err != 0)
		ALSPS_ERR("als_store_flush param error: err = %d\n", err);

	ALSPS_ERR("als_store_flush param: handle %d\n", handle);

	mutex_lock(&alsps_context_obj->alsps_op_mutex);
	cxt = alsps_context_obj;
	if (cxt->als_ctl.flush != NULL)
		err = cxt->als_ctl.flush();
	else
		ALSPS_ERR("ALS DRIVER OLD ARCHITECTURE DON'T SUPPORT ALS COMMON VERSION FLUSH\n");
	if (err < 0)
		ALSPS_ERR("als enable flush err %d\n", err);
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	return count;
}

static ssize_t als_show_flush(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}
/* need work around again */
static ssize_t als_show_devnum(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

#ifndef CONFIG_NANOHUB
static int ps_enable_and_batch(void)
{
	struct alsps_context *cxt = alsps_context_obj;
	int err;

	/* ps_power on -> power off */
	if (cxt->ps_power == 1 && cxt->ps_enable == 0) {
		ALSPS_LOG("PS disable\n");
		/* stop polling firstly, if needed */
#if 0
		if (cxt->ps_ctl.is_report_input_direct == false
			&& cxt->is_ps_polling_run == true) {
			smp_mb();/* for memory barrier */
			del_timer_sync(&cxt->timer_ps);
			smp_mb();/* for memory barrier */
			cancel_work_sync(&cxt->report_ps);
			cxt->drv_data.ps_data.values[0] = ALSPS_INVALID_VALUE;
			cxt->is_ps_polling_run = false;
			ALSPS_LOG("ps stop polling done\n");
		}
#endif
		/* turn off the ps_power */
		err = cxt->ps_ctl.enable_nodata(0);
		if (err) {
			ALSPS_ERR("ps turn off ps_power err = %d\n", err);
			return -1;
		}
		ALSPS_LOG("ps turn off ps_power done\n");

		cxt->ps_power = 0;
		cxt->ps_delay_ns = -1;
		ALSPS_LOG("PS disable done\n");
		return 0;
	}
	/* ps_power off -> power on */
	if (cxt->ps_power == 0 && cxt->ps_enable == 1) {
		ALSPS_LOG("PS ps_power on\n");
		err = cxt->ps_ctl.enable_nodata(1);
		if (err) {
			ALSPS_ERR("ps turn on ps_power err = %d\n", err);
			return -1;
		}
		ALSPS_LOG("ps turn on ps_power done\n");

		cxt->ps_power = 1;
		ALSPS_LOG("PS ps_power on done\n");
	}
	/* rate change */
	if (cxt->ps_power == 1 && cxt->ps_delay_ns >= 0) {
		ALSPS_LOG("PS set batch\n");
		/* set ODR, fifo timeout latency */
		if (cxt->ps_ctl.is_support_batch)
			err = cxt->ps_ctl.batch(0, cxt->ps_delay_ns, cxt->ps_latency_ns);
		else
			err = cxt->ps_ctl.batch(0, cxt->ps_delay_ns, 0);
		if (err) {
			ALSPS_ERR("ps set batch(ODR) err %d\n", err);
			return -1;
		}
		ALSPS_LOG("ps set ODR, fifo latency done\n");
		/* start polling, if needed */
#if 0
		if (cxt->ps_ctl.is_report_input_direct == false) {
			int mdelay = cxt->ps_delay_ns;

			do_div(mdelay, 1000000);
			atomic_set(&cxt->delay_ps, mdelay);
			/* the first sensor start polling timer */
			if (cxt->is_ps_polling_run == false) {
				mod_timer(&cxt->timer_ps, jiffies + atomic_read(&cxt->delay_ps)/(1000/HZ));
				cxt->is_ps_polling_run = true;
				cxt->is_ps_first_data_after_enable = true;
			}
			ALSPS_LOG("ps set polling delay %d ms\n", atomic_read(&cxt->delay_ps));
		} else {
			/* report an default value firstly */
			ps_data_report(1, 3);
		}
#endif
		ps_data_report(1, SENSOR_STATUS_ACCURACY_HIGH);
		ALSPS_LOG("PS batch done\n");
	}
	/* just for debug, remove it when everything is ok */
	if (cxt->ps_power == 0 && cxt->ps_delay_ns >= 0)
		ALSPS_ERR("batch will call firstly in API1.3, do nothing\n");

	return 0;
}
#endif
static ssize_t ps_store_active(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = alsps_context_obj;
	int err = 0;

	ALSPS_LOG("ps_store_active buf=%s\n", buf);
	mutex_lock(&alsps_context_obj->alsps_op_mutex);

	if (!strncmp(buf, "1", 1))
		cxt->ps_enable = 1;
	else if (!strncmp(buf, "0", 1))
		cxt->ps_enable = 0;
	else {
		ALSPS_ERR(" ps_store_active error !!\n");
		err = -1;
		goto err_out;
	}
#ifdef CONFIG_NANOHUB
	err = cxt->ps_ctl.enable_nodata(cxt->ps_enable);
#else
	err = ps_enable_and_batch();
#endif
err_out:
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	ALSPS_LOG(" ps_store_active done\n");
	return err;
}
/*----------------------------------------------------------------------------*/
static ssize_t ps_show_active(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct alsps_context *cxt = NULL;
	int div = 0;

	cxt = alsps_context_obj;
	div = cxt->ps_data.vender_div;
	ALSPS_LOG("ps vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div);
}

static ssize_t ps_store_batch(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = alsps_context_obj;
	int handle = 0, flag = 0, err = 0;

	ALSPS_LOG("ps_store_batch %s\n", buf);
	err = sscanf(buf, "%d,%d,%lld,%lld", &handle, &flag,
			&cxt->ps_delay_ns, &cxt->ps_latency_ns);
	if (err != 4)
		ALSPS_ERR("ps_store_batch param error: err = %d\n", err);

	mutex_lock(&alsps_context_obj->alsps_op_mutex);
#ifdef CONFIG_NANOHUB
	if (cxt->ps_ctl.is_support_batch)
		err = cxt->ps_ctl.batch(0, cxt->ps_delay_ns, cxt->ps_latency_ns);
	else
		err = cxt->ps_ctl.batch(0, cxt->ps_delay_ns, 0);
#else
	err = ps_enable_and_batch();
#endif
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	ALSPS_LOG("ps_store_batch done: %d\n", cxt->is_ps_batch_enable);
	return err;
}

static ssize_t ps_show_batch(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t ps_store_flush(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct alsps_context *cxt = NULL;
	int handle = 0, err = 0;

	err = kstrtoint(buf, 10, &handle);
	if (err != 0)
		ALSPS_ERR("ps_store_flush param error: err = %d\n", err);

	ALSPS_ERR("ps_store_flush param: handle %d\n", handle);

	mutex_lock(&alsps_context_obj->alsps_op_mutex);
	cxt = alsps_context_obj;
	if (cxt->ps_ctl.flush != NULL)
		err = cxt->ps_ctl.flush();
	else
		ALSPS_ERR("PS DRIVER OLD ARCHITECTURE DON'T SUPPORT PS COMMON VERSION FLUSH\n");
	if (err < 0)
		ALSPS_ERR("ps enable flush err %d\n", err);
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	return count;
}

static ssize_t ps_show_flush(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}
/* need work around again */
static ssize_t ps_show_devnum(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}
static int als_ps_remove(struct platform_device *pdev)
{
	ALSPS_LOG("als_ps_remove\n");
	return 0;
}

static int als_ps_probe(struct platform_device *pdev)
{
	ALSPS_LOG("als_ps_probe\n");
	pltfm_dev = pdev;
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id als_ps_of_match[] = {
	{.compatible = "mediatek,als_ps",},
	{},
};
#endif

static struct platform_driver als_ps_driver = {
	.probe	  = als_ps_probe,
	.remove	 = als_ps_remove,
	.driver = {

		.name  = "als_ps",
	#ifdef CONFIG_OF
		.of_match_table = als_ps_of_match,
		#endif
	}
};

static int alsps_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	ALSPS_LOG(" alsps_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_ALSPS_NUM; i++) {
		ALSPS_LOG("alsps_real_driver_init i=%d\n", i);
		if (alsps_init_list[i] != 0) {
			ALSPS_LOG(" alsps try to init driver %s\n", alsps_init_list[i]->name);
			err = alsps_init_list[i]->init();
			if (err == 0) {
				ALSPS_LOG(" alsps real driver %s probe ok\n", alsps_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_ALSPS_NUM) {
		ALSPS_LOG(" alsps_real_driver_init fail\n");
		err =  -1;
	}

	return err;
}

int alsps_driver_add(struct alsps_init_info *obj)
{
	int err = 0;
	int i = 0;

	ALSPS_FUN();
	if (!obj) {
		ALSPS_ERR("ALSPS driver add fail, alsps_init_info is NULL\n");
		return -1;
	}

	for (i = 0; i < MAX_CHOOSE_ALSPS_NUM; i++) {
		if ((i == 0) && (alsps_init_list[0] == NULL)) {
			ALSPS_LOG("register alsps driver for the first time\n");
			if (platform_driver_register(&als_ps_driver))
				ALSPS_ERR("failed to register alsps driver already exist\n");
		}

		if (alsps_init_list[i] == NULL) {
			obj->platform_diver_addr = &als_ps_driver;
			alsps_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_ALSPS_NUM) {
		ALSPS_ERR("ALSPS driver add err\n");
		err =  -1;
	}

	return err;
}
EXPORT_SYMBOL_GPL(alsps_driver_add);
struct platform_device *get_alsps_platformdev(void)
{
	return pltfm_dev;
}


int ps_report_interrupt_data(int value)
{
	struct alsps_context *cxt = NULL;
	/* int err =0; */
	cxt = alsps_context_obj;
	pr_warn("[ALS/PS] [%s]:value=%d\n", __func__, value);
	if (cxt->is_get_valid_ps_data_after_enable == false) {
		if (value != ALSPS_INVALID_VALUE) {
			cxt->is_get_valid_ps_data_after_enable = true;
			smp_mb();/*for memory barriier*/
			del_timer_sync(&cxt->timer_ps);
			smp_mb();/*for memory barriier*/
			cancel_work_sync(&cxt->report_ps);
		}
	}

	if (cxt->is_ps_batch_enable == false)
		ps_data_report(value, 3);

	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(ps_report_interrupt_data);
DEVICE_ATTR(alsactive,		S_IWUSR | S_IRUGO, als_show_active, als_store_active);
DEVICE_ATTR(alsbatch,		S_IWUSR | S_IRUGO, als_show_batch,  als_store_batch);
DEVICE_ATTR(alsflush,		S_IWUSR | S_IRUGO, als_show_flush,  als_store_flush);
DEVICE_ATTR(alsdevnum,		S_IWUSR | S_IRUGO, als_show_devnum,  NULL);
DEVICE_ATTR(psactive,		S_IWUSR | S_IRUGO, ps_show_active, ps_store_active);
DEVICE_ATTR(psbatch,		S_IWUSR | S_IRUGO, ps_show_batch,  ps_store_batch);
DEVICE_ATTR(psflush,		S_IWUSR | S_IRUGO, ps_show_flush,  ps_store_flush);
DEVICE_ATTR(psdevnum,		S_IWUSR | S_IRUGO, ps_show_devnum,  NULL);

static struct attribute *als_attributes[] = {
	&dev_attr_alsactive.attr,
	&dev_attr_alsbatch.attr,
	&dev_attr_alsflush.attr,
	&dev_attr_alsdevnum.attr,
	NULL
};

static struct attribute *ps_attributes[] = {
	&dev_attr_psactive.attr,
	&dev_attr_psbatch.attr,
	&dev_attr_psflush.attr,
	&dev_attr_psdevnum.attr,
	NULL
};

static struct attribute_group als_attribute_group = {
	.attrs = als_attributes
};

static struct attribute_group ps_attribute_group = {
	.attrs = ps_attributes
};
static int light_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t light_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	ssize_t read_cnt = 0;

	read_cnt = sensor_event_read(alsps_context_obj->als_mdev.minor, file, buffer, count, ppos);

	return read_cnt;
}

static unsigned int light_poll(struct file *file, poll_table *wait)
{
	return sensor_event_poll(alsps_context_obj->als_mdev.minor, file, wait);
}

static const struct file_operations light_fops = {
	.owner = THIS_MODULE,
	.open = light_open,
	.read = light_read,
	.poll = light_poll,
};

static int als_misc_init(struct alsps_context *cxt)
{
	int err = 0;

	cxt->als_mdev.minor = ID_LIGHT;
	cxt->als_mdev.name  = ALS_MISC_DEV_NAME;
	cxt->als_mdev.fops = &light_fops;
	err = sensor_attr_register(&cxt->als_mdev);
	if (err)
		ALSPS_ERR("unable to register alsps misc device!!\n");

	return err;
}
static int proximity_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t proximity_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	ssize_t read_cnt = 0;

	read_cnt = sensor_event_read(alsps_context_obj->ps_mdev.minor, file, buffer, count, ppos);

	return read_cnt;
}

static unsigned int proximity_poll(struct file *file, poll_table *wait)
{
	return sensor_event_poll(alsps_context_obj->ps_mdev.minor, file, wait);
}

static const struct file_operations proximity_fops = {
	.owner = THIS_MODULE,
	.open = proximity_open,
	.read = proximity_read,
	.poll = proximity_poll,
};

static int ps_misc_init(struct alsps_context *cxt)
{
	int err = 0;

	cxt->ps_mdev.minor = ID_PROXIMITY;
	cxt->ps_mdev.name  = PS_MISC_DEV_NAME;
	cxt->ps_mdev.fops = &proximity_fops;
	err = sensor_attr_register(&cxt->ps_mdev);
	if (err)
		ALSPS_ERR("unable to register alsps misc device!!\n");

	return err;
}

int als_register_data_path(struct als_data_path *data)
{
	struct alsps_context *cxt = NULL;
	/* int err =0; */
	cxt = alsps_context_obj;
	cxt->als_data.get_data = data->get_data;
	cxt->als_data.vender_div = data->vender_div;
	cxt->als_data.als_get_raw_data = data->als_get_raw_data;
	ALSPS_LOG("alsps register data path vender_div: %d\n", cxt->als_data.vender_div);
	if (cxt->als_data.get_data == NULL) {
		ALSPS_LOG("als register data path fail\n");
		return -1;
	}
	return 0;
}

int ps_register_data_path(struct ps_data_path *data)
{
	struct alsps_context *cxt = NULL;
/* int err =0; */
	cxt = alsps_context_obj;
	cxt->ps_data.get_data = data->get_data;
	cxt->ps_data.vender_div = data->vender_div;
	cxt->ps_data.ps_get_raw_data = data->ps_get_raw_data;
	ALSPS_LOG("alsps register data path vender_div: %d\n", cxt->ps_data.vender_div);
	if (cxt->ps_data.get_data == NULL) {
		ALSPS_LOG("ps register data path fail\n");
		return -1;
	}
	return 0;
}

int als_register_control_path(struct als_control_path *ctl)
{
	struct alsps_context *cxt = NULL;
	int err = 0;

	cxt = alsps_context_obj;
	cxt->als_ctl.set_delay = ctl->set_delay;
	cxt->als_ctl.open_report_data = ctl->open_report_data;
	cxt->als_ctl.enable_nodata = ctl->enable_nodata;
	cxt->als_ctl.batch = ctl->batch;
	cxt->als_ctl.flush = ctl->flush;
	cxt->als_ctl.is_support_batch = ctl->is_support_batch;
	cxt->als_ctl.is_report_input_direct = ctl->is_report_input_direct;
	cxt->als_ctl.is_use_common_factory = ctl->is_use_common_factory;

	if (cxt->als_ctl.enable_nodata == NULL || cxt->als_ctl.batch == NULL ||
			cxt->als_ctl.flush == NULL) {
		ALSPS_LOG("als register control path fail\n");
		return -1;
	}

	/* add misc dev for sensor hal control cmd */
	err = als_misc_init(alsps_context_obj);
	if (err) {
		ALSPS_ERR("unable to register alsps misc device!!\n");
		return -2;
	}
	err = sysfs_create_group(&alsps_context_obj->als_mdev.this_device->kobj,
			&als_attribute_group);
	if (err < 0) {
		ALSPS_ERR("unable to create alsps attribute file\n");
		return -3;
	}
	kobject_uevent(&alsps_context_obj->als_mdev.this_device->kobj, KOBJ_ADD);
	return 0;
}

int ps_register_control_path(struct ps_control_path *ctl)
{
	struct alsps_context *cxt = NULL;
	int err = 0;

	cxt = alsps_context_obj;
	cxt->ps_ctl.set_delay = ctl->set_delay;
	cxt->ps_ctl.open_report_data = ctl->open_report_data;
	cxt->ps_ctl.enable_nodata = ctl->enable_nodata;
	cxt->ps_ctl.batch = ctl->batch;
	cxt->ps_ctl.flush = ctl->flush;
	cxt->ps_ctl.is_support_batch = ctl->is_support_batch;
	cxt->ps_ctl.is_report_input_direct = ctl->is_report_input_direct;
	cxt->ps_ctl.ps_calibration = ctl->ps_calibration;
	cxt->ps_ctl.ps_threshold_setting = ctl->ps_threshold_setting;
	cxt->ps_ctl.is_use_common_factory = ctl->is_use_common_factory;
	cxt->ps_ctl.is_polling_mode = ctl->is_polling_mode;

	if (cxt->ps_ctl.enable_nodata == NULL || cxt->ps_ctl.batch == NULL ||
			cxt->ps_ctl.flush == NULL) {
		ALSPS_LOG("ps register control path fail\n");
		return -1;
	}

	err = ps_misc_init(alsps_context_obj);
	if (err) {
		ALSPS_ERR("unable to register alsps misc device!!\n");
		return -2;
	}
	err = sysfs_create_group(&alsps_context_obj->ps_mdev.this_device->kobj,
			&ps_attribute_group);
	if (err < 0) {
		ALSPS_ERR("unable to create alsps attribute file\n");
		return -3;
	}
	kobject_uevent(&alsps_context_obj->ps_mdev.this_device->kobj, KOBJ_ADD);
	return 0;
}


/* AAL functions**************************************** */
int alsps_aal_enable(int enable)
{
	int ret = 0;
	struct alsps_context *cxt = NULL;

	if (!alsps_context_obj) {
		ALSPS_ERR("null pointer of alsps_context_obj!!\n");
		return -1;
	}

	if (alsps_context_obj->als_ctl.enable_nodata == NULL) {
		ALSPS_ERR("alsps context obj not exsit in alsps_aal_enable\n");
		return -1;
	}
	cxt = alsps_context_obj;

	if (enable == 1) {
		if (alsps_context_obj->is_als_active_data == false) {
			ret = cxt->als_ctl.batch(0, AAL_DELAY, 0);
			if (ret == 0)
				ret = cxt->als_ctl.enable_nodata(enable);
		}
	} else if (enable == 0) {
		if (alsps_context_obj->is_als_active_data == false)
			ret = cxt->als_ctl.enable_nodata(enable);
	}

	return ret;
}

int alsps_aal_get_status(void)
{
	return 0;
}

int alsps_aal_get_data(void)
{
	int ret = 0;
	struct alsps_context *cxt = NULL;
	int value = 0;
	int status = 0;

	if (!alsps_context_obj) {
		ALSPS_ERR("alsps_context_obj null pointer!!\n");
		return -1;
	}

	if (alsps_context_obj->als_data.get_data == NULL) {
		ALSPS_ERR("aal:get_data not exsit\n");
		return -1;
	}

	cxt = alsps_context_obj;
	ret = cxt->als_data.get_data(&value, &status);
	if (ret < 0)
		return -1;

	return value;
}
/* *************************************************** */

static int alsps_probe(void)
{
	int err;

	ALSPS_LOG("+++++++++++++alsps_probe!!\n");
	alsps_context_obj = alsps_context_alloc_object();
	if (!alsps_context_obj) {
		err = -ENOMEM;
		ALSPS_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	/* init real alspseleration driver */
	err = alsps_real_driver_init();
	if (err) {
		ALSPS_ERR("alsps real driver init fail\n");
		goto real_driver_init_fail;
	}
	ALSPS_LOG("----alsps_probe OK !!\n");
	return 0;

real_driver_init_fail:
	kfree(alsps_context_obj);
	alsps_context_obj = NULL;
exit_alloc_data_failed:
	ALSPS_ERR("----alsps_probe fail !!!\n");
	return err;
}

static int alsps_remove(void)
{
	int err = 0;

	ALSPS_FUN(f);
	sysfs_remove_group(&alsps_context_obj->als_mdev.this_device->kobj,
				&als_attribute_group);
	sysfs_remove_group(&alsps_context_obj->ps_mdev.this_device->kobj,
				&ps_attribute_group);

	err = sensor_attr_deregister(&alsps_context_obj->als_mdev);
	err = sensor_attr_deregister(&alsps_context_obj->ps_mdev);
	if (err)
		ALSPS_ERR("misc_deregister fail: %d\n", err);
	kfree(alsps_context_obj);

	return 0;
}

static int __init alsps_init(void)
{
	ALSPS_FUN();

	if (alsps_probe()) {
		ALSPS_ERR("failed to register alsps driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit alsps_exit(void)
{
	alsps_remove();
	platform_driver_unregister(&als_ps_driver);

}
late_initcall(alsps_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ALSPS device driver");
MODULE_AUTHOR("Mediatek");

