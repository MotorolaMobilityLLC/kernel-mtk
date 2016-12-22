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

#include "situation.h"

static struct situation_context *situation_context_obj;

static struct situation_init_info *situation_init_list[max_situation_support] = {0};

static struct situation_context *situation_context_alloc_object(void)
{
	struct situation_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	SITUATION_LOG("situation_context_alloc_object++++\n");
	if (!obj) {
		SITUATION_ERR("Alloc situ object error!\n");
		return NULL;
	}
	mutex_init(&obj->situation_op_mutex);

	SITUATION_LOG("situation_context_alloc_object----\n");
	return obj;
}
static int handle_to_index(int handle)
{
	int index = -1;

	switch (handle) {
	case ID_IN_POCKET:
		index = inpocket;
		break;
	case ID_STATIONARY_DETECT:
		index = stationary;
		break;
	case ID_WAKE_GESTURE:
		index = wake_gesture;
		break;
	case ID_GLANCE_GESTURE:
		index = glance_gesture;
		break;
	case ID_PICK_UP_GESTURE:
		index = pickup_gesture;
		break;
	case ID_ANSWER_CALL:
		index = answer_call;
		break;
	case ID_MOTION_DETECT:
		index = motion_detect;
		break;
	case ID_DEVICE_ORIENTATION:
		index = device_orientation;
		break;
	case ID_TILT_DETECTOR:
		index = tilt_detector;
		break;
	default:
		index = -1;
		SITUATION_ERR("handle_to_index invalid handle:%d, index:%d\n", handle, index);
		return index;
	}
	SITUATION_ERR("handle_to_index handle:%d, index:%d\n", handle, index);
	return index;
}
int situation_data_report(int handle, uint32_t one_sample_data)
{
	int err = 0, index = -1;
	struct sensor_event event;
	struct situation_context *cxt = situation_context_obj;

	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid index\n", __func__);
		return -1;
	}

	SITUATION_LOG("situation_notify handle:%d, index:%d\n", handle, index);

	event.handle = handle;
	event.flush_action = DATA_ACTION;
	event.word[0] = one_sample_data;
	err = sensor_input_event(situation_context_obj->mdev.minor, &event);
	if (err < 0)
		SITUATION_ERR("event buffer full, so drop this data\n");
	if (cxt->ctl_context[index].situation_ctl.open_report_data != NULL &&
		cxt->ctl_context[index].situation_ctl.is_support_wake_lock)
		wake_lock_timeout(&cxt->wake_lock[index], msecs_to_jiffies(250));
	return err;
}
int situation_notify(int handle)
{
	return situation_data_report(handle, 1);
}

int situation_flush_report(int handle)
{
	struct sensor_event event;
	int err = 0;

	SITUATION_LOG("flush, handle:%d\n", handle);
	event.handle = handle;
	event.flush_action = FLUSH_ACTION;
	err = sensor_input_event(situation_context_obj->mdev.minor, &event);
	if (err < 0)
		SITUATION_ERR("failed due to event buffer full\n");
	return err;
}
static int situation_real_enable(int enable, int handle)
{
	int err = 0;
	int index = -1;
	struct situation_context *cxt = NULL;

	cxt = situation_context_obj;
	index = handle_to_index(handle);

	if (cxt->ctl_context[index].situation_ctl.open_report_data == NULL) {
		SITUATION_ERR("GESTURE DRIVER OLD ARCHITECTURE DON'T SUPPORT GESTURE COMMON VERSION ENABLE\n");
		return -1;
	}

	if (1 == enable) {
		if (false == cxt->ctl_context[index].is_active_data) {
			err = cxt->ctl_context[index].situation_ctl.open_report_data(1);
			if (err) {
				err = cxt->ctl_context[index].situation_ctl.open_report_data(1);
				if (err) {
					err = cxt->ctl_context[index].situation_ctl.open_report_data(1);
					if (err) {
						SITUATION_ERR
							("enable_situation enable(%d) err 3 timers = %d\n",
							 enable, err);
						return err;
					}
				}
			}
			cxt->ctl_context[index].is_active_data = true;
			SITUATION_LOG("enable_situation real enable\n");
		}
	} else if (0 == enable) {
		if (cxt->ctl_context[index].situation_ctl.open_report_data != NULL) {
			err = cxt->ctl_context[index].situation_ctl.open_report_data(0);
			if (err)
				SITUATION_ERR("enable_situationenable(%d) err = %d\n", enable, err);
			cxt->ctl_context[index].is_active_data = false;
			SITUATION_LOG("enable_situation real disable\n");
		}
	}
	return err;
}

int situation_enable_nodata(int enable, int handle)
{
	struct situation_context *cxt = NULL;
	int index;

	index = handle_to_index(handle);
	cxt = situation_context_obj;
	if (NULL == cxt->ctl_context[index].situation_ctl.open_report_data) {
		SITUATION_ERR("situation_enable_nodata:situ ctl path is NULL\n");
		return -1;
	}

	if (1 == enable)
		cxt->ctl_context[index].is_active_nodata = true;

	if (0 == enable)
		cxt->ctl_context[index].is_active_nodata = false;
	situation_real_enable(enable, handle);
	return 0;
}

static ssize_t situation_show_enable_nodata(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct situation_context *cxt = NULL;
	int i;
	int s_len = 0;

	cxt = situation_context_obj;
	for (i = 0; i < max_situation_support; i++) {
		SITUATION_LOG("situ handle:%d active: %d\n", i, cxt->ctl_context[i].is_active_nodata);
		s_len += snprintf(buf + s_len, PAGE_SIZE, "id:%d, en:%d\n", i, cxt->ctl_context[i].is_active_nodata);
	}
	return s_len;
}

static ssize_t situation_store_enable_nodata(struct device *dev, struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct situation_context *cxt = NULL;
	int handle, en;
	int err = -1;
	int index = -1;

	SITUATION_LOG("situation_store_enable nodata buf=%s\n", buf);
	mutex_lock(&situation_context_obj->situation_op_mutex);
	cxt = situation_context_obj;

	err = sscanf(buf, "%d : %d", &handle, &en);
	if (err < 0) {
		SITUATION_ERR("[%s] sscanf fail\n", __func__);
		return count;
	}
	SITUATION_LOG("[%s] handle=%d, en=%d\n", __func__, handle, en);
	index = handle_to_index(handle);

	if (NULL == cxt->ctl_context[index].situation_ctl.open_report_data) {
		SITUATION_LOG("situation_ctl enable nodata NULL\n");
		mutex_unlock(&situation_context_obj->situation_op_mutex);
		return count;
	}
	SITUATION_LOG("[%s] handle=%d, en=%d\n", __func__, handle, en);
	situation_enable_nodata(en, handle);

	/* for debug */
	situation_notify(handle);

	mutex_unlock(&situation_context_obj->situation_op_mutex);
	return count;
}

static ssize_t situation_store_active(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct situation_context *cxt = NULL;
	int res = 0;
	int en = 0;
	int handle = -1;

	mutex_lock(&situation_context_obj->situation_op_mutex);

	cxt = situation_context_obj;

	res = sscanf(buf, "%d : %d", &handle, &en);
	if (res < 0) {
		SITUATION_LOG("situation_store_active param error: res = %d\n", res);
		return count;
	}
	SITUATION_LOG("situation_store_active handle=%d, en=%d\n", handle, en);

	if (1 == en)
		situation_real_enable(1, handle);
	else if (0 == en)
		situation_real_enable(0, handle);
	else
		SITUATION_ERR("situation_store_active error !!\n");

	mutex_unlock(&situation_context_obj->situation_op_mutex);
	SITUATION_LOG("situation_store_active done\n");
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t situation_show_active(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct situation_context *cxt = NULL;
	int i;
	int s_len = 0;

	cxt = situation_context_obj;
	for (i = 0; i < max_situation_support; i++) {
		SITUATION_LOG("situ handle:%d active: %d\n", i, cxt->ctl_context[i].is_active_data);
		s_len += snprintf(buf + s_len, PAGE_SIZE, "id:%d, en:%d\n", i, cxt->ctl_context[i].is_active_data);
	}
	return s_len;
}

static ssize_t situation_store_delay(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int len = 0;

	SITUATION_LOG(" not support now\n");
	return len;
}


static ssize_t situation_show_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	SITUATION_LOG(" not support now\n");
	return len;
}


static ssize_t situation_store_batch(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int len = 0, index = -1;
	struct situation_context *cxt = NULL;
	int handle = 0, flag = 0, err = 0;
	int64_t samplingPeriodNs = 0, maxBatchReportLatencyNs = 0;

	err = sscanf(buf, "%d,%d,%lld,%lld", &handle, &flag, &samplingPeriodNs, &maxBatchReportLatencyNs);
	if (err != 4)
		SITUATION_ERR("situation_store_batch param error: err = %d\n", err);

	SITUATION_LOG("handle %d, flag:%d samplingPeriodNs:%lld, maxBatchReportLatencyNs: %lld\n",
			handle, flag, samplingPeriodNs, maxBatchReportLatencyNs);
	mutex_lock(&situation_context_obj->situation_op_mutex);
	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid index\n", __func__);
		mutex_unlock(&situation_context_obj->situation_op_mutex);
		return  -1;
	}
	cxt = situation_context_obj;
	if (cxt->ctl_context[index].situation_ctl.is_support_batch == false)
		maxBatchReportLatencyNs = 0;
	if (NULL != cxt->ctl_context[index].situation_ctl.batch)
		err = cxt->ctl_context[index].situation_ctl.batch(flag, samplingPeriodNs, maxBatchReportLatencyNs);
	else
		SITUATION_ERR("GESTURE DRIVER OLD ARCHITECTURE DON'T SUPPORT GESTURE COMMON VERSION BATCH\n");
	if (err < 0)
		SITUATION_ERR("situ enable batch err %d\n", err);
	mutex_unlock(&situation_context_obj->situation_op_mutex);
	return len;
}

static ssize_t situation_show_batch(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	SITUATION_LOG(" not support now\n");
	return len;
}

static ssize_t situation_store_flush(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct situation_context *cxt = NULL;
	int index = -1, handle = 0, err = 0;

	err = kstrtoint(buf, 10, &handle);
	if (err != 0)
		SITUATION_ERR("situation_store_flush param error: err = %d\n", err);

	SITUATION_ERR("situation_store_flush param: handle %d\n", handle);

	mutex_lock(&situation_context_obj->situation_op_mutex);
	cxt = situation_context_obj;
	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid index\n", __func__);
		mutex_unlock(&situation_context_obj->situation_op_mutex);
		return  -1;
	}
	if (NULL != cxt->ctl_context[index].situation_ctl.flush)
		err = cxt->ctl_context[index].situation_ctl.flush();
	else
		SITUATION_ERR("SITUATION DRIVER OLD ARCHITECTURE DON'T SUPPORT SITUATION COMMON VERSION FLUSH\n");
	if (err < 0)
		SITUATION_ERR("situation enable flush err %d\n", err);
	mutex_unlock(&situation_context_obj->situation_op_mutex);
	return count;
}

static ssize_t situation_show_flush(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	SITUATION_LOG(" not support now\n");
	return len;
}

static ssize_t situation_show_devnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);	/* TODO: why +5? */
}


static int situation_real_driver_init(void)
{
	int err = 0, i = 0;

	SITUATION_LOG(" situation_real_driver_init +\n");

	for (i = 0; i < max_situation_support; i++) {
		if (NULL != situation_init_list[i]) {
			SITUATION_LOG(" situ try to init driver %s\n", situation_init_list[i]->name);
			err = situation_init_list[i]->init();
			if (0 == err)
				SITUATION_LOG(" situ real driver %s probe ok\n", situation_init_list[i]->name);
		} else
			continue;
	}
	return err;
}


int situation_driver_add(struct situation_init_info *obj, int handle)
{
	int err = 0;
	int index = -1;

	SITUATION_LOG("register situation handle=%d\n", handle);

	if (!obj) {
		SITUATION_ERR("[%s] fail, situation_init_info is NULL\n", __func__);
		return -1;
	}

	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid index\n", __func__);
		return  -1;
	}

	if (NULL == situation_init_list[index])
		situation_init_list[index] = obj;

	return err;
}
EXPORT_SYMBOL_GPL(situation_driver_add);
static int situation_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t situation_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	ssize_t read_cnt = 0;

	read_cnt = sensor_event_read(situation_context_obj->mdev.minor, file, buffer, count, ppos);

	return read_cnt;
}

static unsigned int situation_poll(struct file *file, poll_table *wait)
{
	return sensor_event_poll(situation_context_obj->mdev.minor, file, wait);
}

static const struct file_operations situation_fops = {
	.owner = THIS_MODULE,
	.open = situation_open,
	.read = situation_read,
	.poll = situation_poll,
};

static int situation_misc_init(struct situation_context *cxt)
{
	int err = 0;
	/* kernel-3.10\include\linux\Miscdevice.h */
	/* use MISC_DYNAMIC_MINOR exceed 64 */
	/* As MISC_DYNAMIC_MINOR is just support max 64 minors_ID(0-63),
		assign the minor_ID manually that range of  64-254 */
	cxt->mdev.minor = ID_WAKE_GESTURE;/* MISC_DYNAMIC_MINOR; */
	cxt->mdev.name = SITU_MISC_DEV_NAME;
	cxt->mdev.fops = &situation_fops;
	err = sensor_attr_register(&cxt->mdev);
	if (err)
		SITUATION_ERR("unable to register situ misc device!!\n");

	return err;
}

DEVICE_ATTR(situenablenodata, S_IWUSR | S_IRUGO, situation_show_enable_nodata, situation_store_enable_nodata);
DEVICE_ATTR(situactive, S_IWUSR | S_IRUGO, situation_show_active, situation_store_active);
DEVICE_ATTR(situdelay, S_IWUSR | S_IRUGO, situation_show_delay, situation_store_delay);
DEVICE_ATTR(situbatch, S_IWUSR | S_IRUGO, situation_show_batch, situation_store_batch);
DEVICE_ATTR(situflush, S_IWUSR | S_IRUGO, situation_show_flush, situation_store_flush);
DEVICE_ATTR(situdevnum, S_IWUSR | S_IRUGO, situation_show_devnum, NULL);


static struct attribute *situation_attributes[] = {
	&dev_attr_situenablenodata.attr,
	&dev_attr_situactive.attr,
	&dev_attr_situdelay.attr,
	&dev_attr_situbatch.attr,
	&dev_attr_situflush.attr,
	&dev_attr_situdevnum.attr,
	NULL
};

static struct attribute_group situation_attribute_group = {
	.attrs = situation_attributes
};

int situation_register_data_path(struct situation_data_path *data, int handle)
{
	struct situation_context *cxt = NULL;
	int index = -1;

	if (NULL == data || NULL == data->get_data) {
		SITUATION_LOG("situ register data path fail\n");
		return -1;
	}

	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid handle\n", __func__);
		return -1;
	}
	cxt = situation_context_obj;
	cxt->ctl_context[index].situation_data.get_data = data->get_data;

	return 0;
}

int situation_register_control_path(struct situation_control_path *ctl, int handle)
{
	struct situation_context *cxt = NULL;
	int index = -1;

	SITUATION_FUN();
	if (NULL == ctl || NULL == ctl->open_report_data) {
		SITUATION_LOG("situ register control path fail\n");
		return -1;
	}

	index = handle_to_index(handle);
	if (index < 0) {
		SITUATION_ERR("[%s] invalid handle\n", __func__);
		return -1;
	}
	cxt = situation_context_obj;
	cxt->ctl_context[index].situation_ctl.open_report_data = ctl->open_report_data;
	cxt->ctl_context[index].situation_ctl.batch = ctl->batch;
	cxt->ctl_context[index].situation_ctl.flush = ctl->flush;
	cxt->ctl_context[index].situation_ctl.is_support_wake_lock = ctl->is_support_wake_lock;
	cxt->ctl_context[index].situation_ctl.is_support_batch = ctl->is_support_batch;

	cxt->wake_lock_name[index] = kzalloc(64, GFP_KERNEL);
	if (!cxt->wake_lock_name[index]) {
		SITUATION_ERR("Alloc wake_lock_name error!\n");
		return -1;
	}
	sprintf(cxt->wake_lock_name[index], "situation_wakelock-%d", index);
	wake_lock_init(&cxt->wake_lock[index], WAKE_LOCK_SUSPEND, cxt->wake_lock_name[index]);

	return 0;
}

static int situation_probe(void)
{
	int err;

	SITUATION_LOG("+++++++++++++situation_probe!!\n");

	situation_context_obj = situation_context_alloc_object();
	if (!situation_context_obj) {
		err = -ENOMEM;
		SITUATION_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	/* init real situ driver */
	err = situation_real_driver_init();
	if (err) {
		SITUATION_ERR("situ real driver init fail\n");
		goto real_driver_init_fail;
	}

	/* add misc dev for sensor hal control cmd */
	err = situation_misc_init(situation_context_obj);
	if (err) {
		SITUATION_ERR("unable to register situ misc device!!\n");
		goto real_driver_init_fail;
	}
	err = sysfs_create_group(&situation_context_obj->mdev.this_device->kobj, &situation_attribute_group);
	if (err < 0) {
		SITUATION_ERR("unable to create situ attribute file\n");
		goto real_driver_init_fail;
	}
	kobject_uevent(&situation_context_obj->mdev.this_device->kobj, KOBJ_ADD);


	SITUATION_LOG("----situation_probe OK !!\n");
	return 0;

real_driver_init_fail:
	kfree(situation_context_obj);
exit_alloc_data_failed:
	SITUATION_LOG("----situation_probe fail !!!\n");
	return err;
}

static int situation_remove(void)
{
	int err = 0;

	SITUATION_FUN();
	sysfs_remove_group(&situation_context_obj->mdev.this_device->kobj, &situation_attribute_group);

	err = sensor_attr_deregister(&situation_context_obj->mdev);
	if (err)
		SITUATION_ERR("misc_deregister fail: %d\n", err);

	kfree(situation_context_obj);
	return 0;
}
static int __init situation_init(void)
{
	SITUATION_FUN();

	if (situation_probe()) {
		SITUATION_ERR("failed to register situ driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit situation_exit(void)
{
	situation_remove();
}

late_initcall(situation_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("situation sensor driver");
MODULE_AUTHOR("hongxu.zhao@mediatek.com");
