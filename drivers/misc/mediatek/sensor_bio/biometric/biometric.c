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

#include "inc/biometric.h"
#include <linux/kthread.h>

struct bio_context *bio_context_obj;
int debug_latency;
module_param(debug_latency, int, 0644);

static struct bio_init_info *bio_init_list[MAX_CHOOSE_BIO_NUM] = { 0 };
static struct task_struct *bio_tsk;
static DECLARE_WAIT_QUEUE_HEAD(bio_thread_wq);
static unsigned int polling_delay;

u32 ekg_data[1024];		/* /// */
u32 ppg1_data[1024];		/* /// */
u32 ppg2_data[1024];		/* /// */
u32 ppg1_amb_data[1024];
u32 ppg2_amb_data[1024];
u32 ppg1_agc_data[1024];
u32 ppg2_agc_data[1024];

static int64_t getCurNS(void)
{
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;

	return ns;
}

static void initTimer(struct hrtimer *timer, enum hrtimer_restart (*callback) (struct hrtimer *))
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	timer->function = callback;
}

static void startTimer(struct hrtimer *timer, int64_t ndelay, bool first)
{
	struct bio_context *obj =
	    (struct bio_context *)container_of(timer, struct bio_context, hrTimer);
	static int count;

	if (obj == NULL) {
		BIO_PR_ERR("NULL pointer\n");
		return;
	}

	if (first) {
		obj->target_ktime = ktime_add_ns(ktime_get(), ndelay);
		count = 0;
	} else {
		obj->target_ktime = ktime_add_ns(obj->target_ktime, ndelay);
		count++;
	}

	hrtimer_start(timer, obj->target_ktime, HRTIMER_MODE_ABS);
}

#if 1
static void stopTimer(struct hrtimer *timer)
{
	hrtimer_cancel(timer);
}
#endif

static int bio_thread(void *arg)
{
	struct sched_param param = {.sched_priority = 99 };
	struct bio_context *cxt = NULL;
	/* static int x; */
	int64_t cur_ns;
	static int64_t pre_ns;
	int64_t wait_t = 0, read_t;
	int sensor_id = (int64_t)arg;

	cxt = bio_context_obj;

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	for (;;) {
		if (kthread_should_stop())
			break;

		wait_event_interruptible(bio_thread_wq, (cxt->active & (1LL << sensor_id)));

		cur_ns = sched_clock();

		if (pre_ns != 0)
			wait_t = cur_ns - pre_ns;

		cxt->drv_data.bio_data.time = cur_ns;
		switch (sensor_id) {
		case ID_EKG:
			ekg_data_report(NULL);
			break;
		case ID_PPG1:
			ppg1_data_report(NULL);
			break;
		case ID_PPG2:
			ppg2_data_report(NULL);
			break;
		}

		pre_ns = sched_clock();
		read_t = pre_ns - cur_ns;
		msleep_interruptible(polling_delay);
		/* while(sched_clock() - pre_ns < 10000000); */
		/* BIO_LOG("wait_t = %lld, read_t = %lld\n", wait_t, read_t); */
	}
	return 0;
}
static void bio_work_func(struct work_struct *work)
{
#if 0
	struct bio_context *cxt = NULL;
	int64_t cur_ns;
	static int64_t pre_ns;
	int64_t wait_t = 0, read_t;

	return;

	cxt = bio_context_obj;

	cur_ns = getCurNS();

	if (pre_ns != 0)
		wait_t = cur_ns - pre_ns;

	cxt->drv_data.bio_data.time = cur_ns;

	if (cxt->active & (1LL << ID_EKG))
		ekg_data_report(NULL);
	if (cxt->active & (1LL << ID_PPG1))
		ppg1_data_report(NULL);
	if (cxt->active & (1LL << ID_PPG2))
		ppg2_data_report(NULL);

	pre_ns = getCurNS();
	read_t = pre_ns - cur_ns;
	BIO_LOG("wait_t = %lld, read_t = %lld\n", wait_t, read_t);

	if (true == cxt->is_polling_run)
		startTimer(&cxt->hrTimer, cxt->ndelay, false);
#endif
}

enum hrtimer_restart bio_poll(struct hrtimer *timer)
{
	struct bio_context *obj =
	    (struct bio_context *)container_of(timer, struct bio_context, hrTimer);

	queue_work(obj->bio_workqueue, &obj->report);

	/* BIO_LOG("cur_ns = %lld\n", getCurNS()); */

	return HRTIMER_NORESTART;
}

static struct bio_context *bio_context_alloc_object(void)
{
	struct bio_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	BIO_LOG("bio_context_alloc_object++++\n");
	if (!obj) {
		BIO_PR_ERR("Alloc biometric object error!\n");
		return NULL;
	}
	/* obj->ndelay = 1953125; */
	obj->ndelay = 20000000;	/* 50Hz */
	INIT_WORK(&obj->report, bio_work_func);
	obj->bio_workqueue = NULL;
	obj->bio_workqueue = create_workqueue("biometric_polling");
	if (!obj->bio_workqueue) {
		kfree(obj);
		return NULL;
	}
	initTimer(&obj->hrTimer, bio_poll);
	obj->is_polling_run = false;
	obj->active = 0;
	mutex_init(&obj->bio_op_mutex);
	polling_delay = 20;
	BIO_LOG("bio_context_alloc_object----\n");
	return obj;
}

static int bio_enable_data(int handle, int enable)
{
	struct bio_context *cxt = bio_context_obj;

	BIO_LOG("BIO %s handle:%d\n", enable ? "enable" : "disable", handle);

	switch (handle) {
	case ID_EKG:
		cxt->ekg_sn = 0;
		cxt->ekg_read_time = 0;
		cxt->bio_ctl.enable_ekg(enable);
		break;
	case ID_PPG1:
		cxt->ppg1_sn = 0;
		cxt->ppg1_read_time = 0;
		cxt->bio_ctl.enable_ppg1(enable);
		break;
	case ID_PPG2:
		cxt->ppg2_sn = 0;
		cxt->ppg2_read_time = 0;
		cxt->bio_ctl.enable_ppg2(enable);
		break;
	}

	return 0;
}

static ssize_t bio_store_active(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct bio_context *cxt = NULL;
	int res = 0;
	int handle = 0;
	int en = 0;

	BIO_LOG("bio_store_active buf=%s\n", buf);
	mutex_lock(&bio_context_obj->bio_op_mutex);
	cxt = bio_context_obj;

	res = sscanf(buf, "%d,%d", &handle, &en);
	if (res != 2)
		BIO_LOG(" bio_store_active param error: buf = %s\n", buf);

	if (handle != ID_EKG && handle != ID_PPG1 && handle != ID_PPG2) {
		BIO_PR_ERR("Invalid handle %d, the value should be %d %d %d\n", handle, ID_EKG,
			ID_PPG1, ID_PPG2);
		mutex_unlock(&bio_context_obj->bio_op_mutex);
		return 0;
	}

	if (en && (cxt->active & (1LL << handle)) == 0) {	/* off=>on */
		bio_enable_data(handle, 1);
		cxt->active |= 1LL << handle;
		wake_up_interruptible(&bio_thread_wq);
	} else if ((en == 0) && (cxt->active & (1LL << handle))) {	/* on->off */
		bio_enable_data(handle, 0);
		cxt->active &= ~(1LL << handle);
	} else			/* do nothing */
		BIO_LOG("BIO sensor handle:%d has already %s\n", handle, en ? "on" : "off");

#if 1
	if (cxt->active) {
		if (false == cxt->is_polling_run) {
			cxt->is_polling_run = true;
			startTimer(&cxt->hrTimer, cxt->ndelay, true);
		}
	} else {
		if (true == cxt->is_polling_run) {
			cxt->is_polling_run = false;
			smp_mb();	/* for memory barrier */
			stopTimer(&cxt->hrTimer);
			smp_mb();	/* for memory barrier */
			cancel_work_sync(&cxt->report);
			cxt->drv_data.bio_data.values[0] = BIO_INVALID_VALUE;
		}
	}
#endif
	mutex_unlock(&bio_context_obj->bio_op_mutex);
	BIO_LOG("bio_store_active done buf=%s\n", buf);
	return count;
}

static ssize_t bio_show_active(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%llx\n", bio_context_obj->active);
}

static ssize_t bio_store_delay(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct bio_context *cxt = NULL;

	mutex_lock(&bio_context_obj->bio_op_mutex);
	cxt = bio_context_obj;

	/* bio_context_obj->ndelay = 1953125; */
	/* bio_context_obj->ndelay = 1953125; */
	cxt->ndelay = 20000000;	/* 50Hz */

	mutex_unlock(&bio_context_obj->bio_op_mutex);
	return count;
}

static ssize_t bio_show_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	BIO_LOG(" not support now\n");
	return len;
}

/* need work around again */
static ssize_t bio_show_sensordevnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t bio_store_batch(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct bio_context *cxt = bio_context_obj;

	/* BIO_LOG("bio_store_batch buf=%s\n", buf); */
	mutex_lock(&cxt->bio_op_mutex);

	cxt->ndelay = 20000000;	/* 50Hz */
	BIO_LOG(" bio_store_batch not supported\n");

	mutex_unlock(&cxt->bio_op_mutex);
	return count;

}

static ssize_t bio_show_batch(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t bio_store_flush(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	return count;
}

static ssize_t bio_show_flush(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t bio_store_polling_delay(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int delayTime = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &delayTime);

	BIO_LOG("(%d)\n", delayTime);
	if (0 != ret && 0 <= delayTime)
		polling_delay = delayTime;

	return count;
}

static ssize_t bio_show_polling_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "polling delay = %d\n", polling_delay);
}

static int biometric_remove(struct platform_device *pdev)
{
	BIO_LOG("biometric_remove\n");
	return 0;
}

static int biometric_probe(struct platform_device *pdev)
{
	BIO_LOG("biometric_probe\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id biometric_of_match[] = {
	{.compatible = "mediatek,biometric",},
	{},
};
#endif

static struct platform_driver biometric_driver = {
	.probe	  = biometric_probe,
	.remove	 = biometric_remove,
	.driver = {

		.name  = "biometric",
	#ifdef CONFIG_OF
		.of_match_table = biometric_of_match,
	#endif
	}
};


static int bio_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	BIO_LOG(" bio_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_BIO_NUM; i++) {
		BIO_LOG(" i=%d\n", i);
		if (bio_init_list[i] != 0) {
			BIO_LOG(" bio try to init driver %s\n", bio_init_list[i]->name);
			err = bio_init_list[i]->init();
			if (err == 0) {
				BIO_LOG(" bio real driver %s probe ok\n", bio_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_BIO_NUM) {
		BIO_LOG(" bio_real_driver_init fail\n");
		err = -1;
	}
	return err;
}

int bio_driver_add(struct bio_init_info *obj)
{
	int err = 0;
	int i = 0;

	if (!obj) {
		BIO_PR_ERR("BIO driver add fail, bio_init_info is NULL\n");
		return -1;
	}
	for (i = 0; i < MAX_CHOOSE_BIO_NUM; i++) {
		if ((i == 0) && (bio_init_list[0] == NULL)) {
			BIO_LOG("register biometric driver for the first time\n");
			if (platform_driver_register(&biometric_driver))
				BIO_PR_ERR("failed to register biometric driver already exist\n");
		}

		if (bio_init_list[i] == NULL) {
			obj->platform_diver_addr = &biometric_driver;
			bio_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_BIO_NUM) {
		BIO_PR_ERR("BIO driver add err\n");
		err = -1;
	}

	return err;

}
EXPORT_SYMBOL_GPL(bio_driver_add);

static int biometric_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t biometric_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	ssize_t read_cnt = 0;

	read_cnt = sensor_event_read(bio_context_obj->mdev.minor, file, buffer, count, ppos);

	return read_cnt;
}

static unsigned int biometric_poll(struct file *file, poll_table *wait)
{
	return sensor_event_poll(bio_context_obj->mdev.minor, file, wait);
}

static const struct file_operations biometric_fops = {
	.owner = THIS_MODULE,
	.open = biometric_open,
	.read = biometric_read,
	.poll = biometric_poll,
};

static int bio_misc_init(struct bio_context *cxt)
{

	int err = 0;

	cxt->mdev.minor = ID_EKG;
	cxt->mdev.name = BIO_MISC_DEV_NAME;
	cxt->mdev.fops = &biometric_fops;
	err = sensor_attr_register(&cxt->mdev);
	BIO_LOG("bio_fops address: %p!!\n", &biometric_fops);
	if (err)
		BIO_PR_ERR("unable to register bio misc device!!\n");

	return err;
}

DEVICE_ATTR(bioactive, S_IWUSR | S_IRUGO, bio_show_active, bio_store_active);
DEVICE_ATTR(biodelay, S_IWUSR | S_IRUGO, bio_show_delay, bio_store_delay);
DEVICE_ATTR(biobatch, S_IWUSR | S_IRUGO, bio_show_batch, bio_store_batch);
DEVICE_ATTR(bioflush, S_IWUSR | S_IRUGO, bio_show_flush, bio_store_flush);
DEVICE_ATTR(biodevnum, S_IWUSR | S_IRUGO, bio_show_sensordevnum, NULL);
DEVICE_ATTR(polling_delay, S_IWUSR | S_IRUGO, bio_show_polling_delay, bio_store_polling_delay);

static struct attribute *bio_attributes[] = {
	&dev_attr_bioactive.attr,
	&dev_attr_biodelay.attr,
	&dev_attr_biobatch.attr,
	&dev_attr_bioflush.attr,
	&dev_attr_biodevnum.attr,
	&dev_attr_polling_delay.attr,
	NULL
};

static struct attribute_group bio_attribute_group = {
	.attrs = bio_attributes
};

int ekg_data_report(struct bio_data *data)
{
	struct sensor_event event;
	/* BIO_LOG("+bio_data_report! %d\n",x); */
	struct bio_context *cxt = bio_context_obj;
	u32 length;
	int i, err = 0;
	int64_t cur_time;

	cur_time = getCurNS();
	if (cxt->ekg_read_time != 0) {
		if ((cur_time - cxt->ekg_read_time) > 300000000)
			BIO_LOG("%lld, %lld\n", cur_time, cxt->ekg_read_time);
	}
	cxt->ekg_read_time = cur_time;

	cxt->bio_data.get_data_ekg(ekg_data, &length);
	/* BIO_LOG("%s ====> length:%d\n", __func__, length); */

	for (i = 0; i < length; i++) {
		event.time_stamp = 0; /* data->timestamp; */
		event.handle = ID_EKG;
		event.flush_action = DATA_ACTION;
		event.word[0] = ekg_data[i];
		event.word[1] = cxt->ekg_sn;
		cxt->ekg_sn = (cxt->ekg_sn + 1) % 10000000;

		/* BIO_LOG("%s ====> length:%d ekg_data:%d, count = %d\n", __func__, length, ekg_data[i], count); */
		err = sensor_input_event(bio_context_obj->mdev.minor, &event);
		if (err < 0)
			BIO_PR_ERR("failed due to event buffer full\n");
	}
	return err;
}

int ppg1_data_report(struct bio_data *data)
{
	struct sensor_event event;
	/* BIO_LOG("+bio_data_report! %d\n",x); */
	struct bio_context *cxt = bio_context_obj;
	u32 length;
	int i, err = 0;
	int64_t cur_time;

	cur_time = getCurNS();
	if (cxt->ppg1_read_time != 0) {
		if ((cur_time - cxt->ppg1_read_time) > 300000000)
			BIO_LOG("%lld, %lld\n", cur_time, cxt->ppg1_read_time);
	}
	cxt->ppg1_read_time = cur_time;

	cxt->bio_data.get_data_ppg1(ppg1_data, ppg1_amb_data, ppg1_agc_data, &length);

	for (i = 0; i < length; i++) {
		event.time_stamp = 0; /* data->timestamp; */
		event.handle = ID_PPG1;
		event.flush_action = DATA_ACTION;
		event.word[0] = ppg1_data[i];
		event.word[1] = cxt->ppg1_sn;
		event.word[2] = ppg1_amb_data[i];
		event.word[3] = ppg1_agc_data[i];
		cxt->ppg1_sn = (cxt->ppg1_sn + 1) % 10000000;

		/* BIO_LOG("%s ====> length:%d, %d\n", __func__, length, ppg1_data[i]); */
		err = sensor_input_event(bio_context_obj->mdev.minor, &event);
		if (err < 0)
			BIO_PR_ERR("failed due to event buffer full\n");
	}
	return err;
}

int ppg2_data_report(struct bio_data *data)
{
	struct sensor_event event;
	/* BIO_LOG("+bio_data_report! %d\n",x); */
	struct bio_context *cxt = bio_context_obj;
	u32 length = 0;
	int i, err = 0;
	int64_t cur_time;

	cur_time = getCurNS();
	if (cxt->ppg2_read_time != 0) {
		if ((cur_time - cxt->ppg2_read_time) > 300000000)
			BIO_LOG("%lld, %lld\n", cur_time, cxt->ppg2_read_time);
	}
	cxt->ppg2_read_time = cur_time;

	cxt->bio_data.get_data_ppg2(ppg2_data, ppg2_amb_data, ppg2_agc_data, &length);
	/* BIO_LOG("%s ====> length:%d\n", __func__, length); */

	for (i = 0; i < length; i++) {
		event.time_stamp = 0; /* data->timestamp; */
		event.handle = ID_PPG2;
		event.flush_action = DATA_ACTION;
		event.word[0] = ppg2_data[i];
		event.word[1] = cxt->ppg2_sn;
		event.word[2] = ppg2_amb_data[i];
		event.word[3] = ppg2_agc_data[i];
		cxt->ppg2_sn = (cxt->ppg2_sn + 1) % 10000000;

		err = sensor_input_event(bio_context_obj->mdev.minor, &event);
		if (err < 0)
			BIO_PR_ERR("failed due to event buffer full\n");
	}
	return err;
}

int bio_register_control_path(struct bio_control_path *ctl)
{
	struct bio_context *cxt = bio_context_obj;

	cxt->bio_ctl.enable_ekg = ctl->enable_ekg;
	cxt->bio_ctl.enable_ppg1 = ctl->enable_ppg1;
	cxt->bio_ctl.enable_ppg2 = ctl->enable_ppg2;
	cxt->bio_ctl.set_delay_ekg = ctl->set_delay_ekg;
	cxt->bio_ctl.set_delay_ppg1 = ctl->set_delay_ppg1;
	cxt->bio_ctl.set_delay_ppg2 = ctl->set_delay_ppg2;
	return 0;
}

int bio_register_data_path(struct bio_data_path *data)
{
	struct bio_context *cxt = bio_context_obj;

	cxt->bio_data.get_data_ekg = data->get_data_ekg;
	cxt->bio_data.get_data_ppg1 = data->get_data_ppg1;
	cxt->bio_data.get_data_ppg2 = data->get_data_ppg2;
	return 0;
}

static int bio_probe(void)
{
	int err;

	BIO_LOG("+++++++++++++bio_probe!!\n");

	bio_context_obj = bio_context_alloc_object();
	if (!bio_context_obj) {
		err = -ENOMEM;
		BIO_PR_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	/* init real biometric sensor driver */
	err = bio_real_driver_init();
	if (err) {
		BIO_PR_ERR("bio real driver init fail\n");
		goto real_driver_init_fail;
	}

	init_waitqueue_head(&bio_thread_wq);
	bio_tsk = kthread_run(bio_thread, (void *)ID_EKG, "EKG");
	if (IS_ERR(bio_tsk))
		BIO_PR_ERR("create EKG thread fail\n");
	bio_tsk = kthread_run(bio_thread, (void *)ID_PPG1, "PPG1");
	if (IS_ERR(bio_tsk))
		BIO_PR_ERR("create PPG1 thread fail\n");
	bio_tsk = kthread_run(bio_thread, (void *)ID_PPG2, "PPG2");
	if (IS_ERR(bio_tsk))
		BIO_PR_ERR("create PPG2 thread fail\n");

	/* add misc dev for sensor hal control cmd */
	err = bio_misc_init(bio_context_obj);
	if (err) {
		BIO_PR_ERR("unable to register bio misc device!!\n");
		goto bio_misc_init_fail;
	}
	err = sysfs_create_group(&bio_context_obj->mdev.this_device->kobj, &bio_attribute_group);
	if (err < 0) {
		BIO_PR_ERR("unable to create bio attribute file\n");
		goto sysfs_create_group_failed;
	}
	kobject_uevent(&bio_context_obj->mdev.this_device->kobj, KOBJ_ADD);

	BIO_LOG("----bio_probe OK !!\n");
	return 0;

sysfs_create_group_failed:
sensor_attr_deregister(&bio_context_obj->mdev);
bio_misc_init_fail:
real_driver_init_fail:
	kfree(bio_context_obj);
exit_alloc_data_failed:

	BIO_PR_ERR("----bio_probe fail !!!\n");
	return err;
}



static int bio_remove(void)
{
	input_unregister_device(bio_context_obj->idev);
	sysfs_remove_group(&bio_context_obj->idev->dev.kobj, &bio_attribute_group);

	sensor_attr_deregister(&bio_context_obj->mdev);

	kfree(bio_context_obj);

	return 0;
}

static int __init bio_init(void)
{
	BIO_LOG("bio_init\n");

	if (bio_probe()) {
		BIO_PR_ERR("failed to register bio driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit bio_exit(void)
{
	bio_remove();
}
late_initcall(bio_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BIOMETRIC device driver");
MODULE_AUTHOR("Mediatek");
