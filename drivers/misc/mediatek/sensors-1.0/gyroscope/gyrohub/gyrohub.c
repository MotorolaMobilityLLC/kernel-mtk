/* GYRO_HUB motion sensor driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <hwmsensor.h>
#include "gyrohub.h"
#include <gyroscope.h>
#include <SCP_sensorHub.h>
#include "SCP_power_monitor.h"

#define GYROHUB_DEV_NAME        "gyro_hub"	/* name must different with gsensor gyrohub */

#define GYROS_TAG					"[GYRO] "
#define GYROS_FUN(f)				pr_debug(GYROS_TAG"%s\n", __func__)
#define GYROS_PR_ERR(fmt, args...)		pr_err(GYROS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GYROS_LOG(fmt, args...)		pr_debug(GYROS_TAG fmt, ##args)


static struct gyro_init_info gyrohub_init_info;
struct platform_device *gyroPltFmDev;
static int gyrohub_init_flag = -1;

typedef enum {
	GYRO_TRC_FILTER = 0x01,
	GYRO_TRC_RAWDATA = 0x02,
	GYRO_TRC_IOCTL = 0x04,
	GYRO_TRC_CALI = 0X08,
	GYRO_TRC_INFO = 0X10,
	GYRO_TRC_DATA = 0X20,
} GYRO_TRC;
struct gyrohub_ipi_data {
	int direction;
	atomic_t trace;
	atomic_t suspend;
	int32_t static_cali[GYROHUB_AXES_NUM];
	int32_t dynamic_cali[GYROHUB_AXES_NUM];
	struct work_struct init_done_work;
	/*data */
	atomic_t scp_init_done;
	atomic_t first_ready_after_boot;
};
static struct gyrohub_ipi_data *obj_ipi_data;

static int gyrohub_get_data(int *x, int *y, int *z, int *status);

static int gyrohub_write_rel_calibration(struct gyrohub_ipi_data *obj, int dat[GYROHUB_AXES_NUM])
{
	obj->static_cali[GYROHUB_AXIS_X] = dat[GYROHUB_AXIS_X];
	obj->static_cali[GYROHUB_AXIS_Y] = dat[GYROHUB_AXIS_Y];
	obj->static_cali[GYROHUB_AXIS_Z] = dat[GYROHUB_AXIS_Z];


	if (atomic_read(&obj->trace) & GYRO_TRC_CALI) {
		GYROS_LOG("write gyro calibration data  (%5d, %5d, %5d)\n",
			 obj->static_cali[GYROHUB_AXIS_X], obj->static_cali[GYROHUB_AXIS_Y],
			 obj->static_cali[GYROHUB_AXIS_Z]);
	}

	return 0;
}

static int gyrohub_ResetCalibration(void)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	unsigned char buf[2] = {0};
	int err = 0;

	err = sensor_set_cmd_to_hub(ID_GYROSCOPE, CUST_ACTION_RESET_CALI, buf);
	if (err < 0)
		GYROS_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
			ID_GYROSCOPE, CUST_ACTION_RESET_CALI);

	memset(obj->static_cali, 0x00, sizeof(obj->static_cali));
	GYROS_LOG("gyro clear cali\n");
	return err;
}

static int gyrohub_ReadCalibration(int dat[GYROHUB_AXES_NUM])
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	dat[GYROHUB_AXIS_X] = obj->static_cali[GYROHUB_AXIS_X];
	dat[GYROHUB_AXIS_Y] = obj->static_cali[GYROHUB_AXIS_Y];
	dat[GYROHUB_AXIS_Z] = obj->static_cali[GYROHUB_AXIS_Z];

	GYROS_LOG("Read gyro calibration data  (%5d, %5d, %5d)\n",
		 dat[GYROHUB_AXIS_X], dat[GYROHUB_AXIS_Y], dat[GYROHUB_AXIS_Z]);
	return 0;
}

static int gyrohub_WriteCalibration_scp(int dat[GYROHUB_AXES_NUM])
{
	int err = 0;

	err = sensor_set_cmd_to_hub(ID_GYROSCOPE, CUST_ACTION_SET_CALI, dat);
	if (err < 0)
		GYROS_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
			ID_GYROSCOPE, CUST_ACTION_SET_CALI);
	return err;
}

static int gyrohub_WriteCalibration(int dat[GYROHUB_AXES_NUM])
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	int err = 0;
	int cali[GYROHUB_AXES_NUM];

	GYROS_FUN();

	if (!obj || !dat) {
		GYROS_PR_ERR("null ptr!!\n");
		return -EINVAL;
	}

	err = gyrohub_WriteCalibration_scp(dat);
	if (err < 0) {
		GYROS_PR_ERR("gyrohub_WriteCalibration_scp fail\n");
		return -1;
	}

	cali[GYROHUB_AXIS_X] = obj->static_cali[GYROHUB_AXIS_X];
	cali[GYROHUB_AXIS_Y] = obj->static_cali[GYROHUB_AXIS_Y];
	cali[GYROHUB_AXIS_Z] = obj->static_cali[GYROHUB_AXIS_Z];

	cali[GYROHUB_AXIS_X] += dat[GYROHUB_AXIS_X];
	cali[GYROHUB_AXIS_Y] += dat[GYROHUB_AXIS_Y];
	cali[GYROHUB_AXIS_Z] += dat[GYROHUB_AXIS_Z];

	GYROS_LOG("write gyro calibration data  (%5d, %5d, %5d)-->(%5d, %5d, %5d)\n",
		 dat[GYROHUB_AXIS_X], dat[GYROHUB_AXIS_Y], dat[GYROHUB_AXIS_Z],
		 cali[GYROHUB_AXIS_X], cali[GYROHUB_AXIS_Y], cali[GYROHUB_AXIS_Z]);

	return gyrohub_write_rel_calibration(obj, cali);
}


static int gyrohub_SetPowerMode(bool enable)
{
	int err = 0;

	err = sensor_enable_to_hub(ID_GYROSCOPE, enable);
	if (err < 0)
		GYROS_PR_ERR("sensor_enable_to_hub fail!\n");

	return err;
}

static int gyrohub_ReadGyroData(char *buf, int bufsize)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	struct data_unit_t data;
	uint64_t time_stamp = 0;
	uint64_t time_stamp_gpt = 0;
	int gyro[GYROHUB_AXES_NUM];
	int err = 0;
	int status = 0;

	if (atomic_read(&obj->suspend))
		return -3;

	if (buf == NULL)
		return -1;
	err = sensor_get_data_from_hub(ID_GYROSCOPE, &data);
	if (err < 0) {
		GYROS_PR_ERR("sensor_get_data_from_hub fail!\n");
		return err;
	}

	time_stamp				= data.time_stamp;
	time_stamp_gpt			= data.time_stamp_gpt;
	gyro[GYROHUB_AXIS_X]	= data.gyroscope_t.x;
	gyro[GYROHUB_AXIS_Y]	= data.gyroscope_t.y;
	gyro[GYROHUB_AXIS_Z]	= data.gyroscope_t.z;
	status					= data.gyroscope_t.status;
	GYROS_LOG("recv ipi: timestamp: %lld, timestamp_gpt: %lld, x: %d, y: %d, z: %d!\n", time_stamp, time_stamp_gpt,
		gyro[GYROHUB_AXIS_X], gyro[GYROHUB_AXIS_Y], gyro[GYROHUB_AXIS_Z]);


	sprintf(buf, "%04x %04x %04x %04x", gyro[GYROHUB_AXIS_X], gyro[GYROHUB_AXIS_Y], gyro[GYROHUB_AXIS_Z], status);

	if (atomic_read(&obj->trace) & GYRO_TRC_DATA)
		GYROS_LOG("gsensor data: %s!\n", buf);

	return 0;

}

static int gyrohub_ReadChipInfo(char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8) * 10);

	if ((buf == NULL) || (bufsize <= 30))
		return -1;

	sprintf(buf, "GYROHUB Chip");
	return 0;
}

static int gyrohub_ReadAllReg(char *buf, int bufsize)
{
	int err = 0;

	err = gyrohub_SetPowerMode(true);
	if (err)
		GYROS_PR_ERR("Power on mpu6050 error %d!\n", err);
	msleep(50);
	err = sensor_set_cmd_to_hub(ID_GYROSCOPE, CUST_ACTION_SHOW_REG, buf);
	if (err < 0) {
		GYROS_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n", ID_GYROSCOPE, CUST_ACTION_SHOW_REG);
		return 0;
	}
	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	char strbuf[GYROHUB_BUFSIZE];
	int err = 0;

	if (obj == NULL) {
		GYROS_PR_ERR("obj is null!!\n");
		return 0;
	}
	err = gyrohub_ReadAllReg(strbuf, GYROHUB_BUFSIZE);
	if (err < 0) {
		GYROS_LOG("gyrohub_ReadAllReg fail!!\n");
		return 0;
	}
	err = gyrohub_ReadChipInfo(strbuf, GYROHUB_BUFSIZE);
	if (err < 0) {
		GYROS_LOG("gyrohub_ReadChipInfo fail!!\n");
		return 0;
	}
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	char strbuf[GYROHUB_BUFSIZE];
	int err = 0;

	if (obj == NULL) {
		GYROS_PR_ERR("obj is null!!\n");
		return 0;
	}

	err = gyrohub_ReadGyroData(strbuf, GYROHUB_BUFSIZE);
	if (err < 0) {
		GYROS_LOG("gyrohub_ReadGyroData fail!!\n");
		return 0;
	}
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	if (obj == NULL) {
		GYROS_PR_ERR(" obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	int trace = 0;
	int res = 0;

	if (obj == NULL) {
		GYROS_PR_ERR("obj is null!!\n");
		return 0;
	}
	if (sscanf(buf, "0x%x", &trace) == 1) {
		atomic_set(&obj->trace, trace);
		res = sensor_set_cmd_to_hub(ID_GYROSCOPE, CUST_ACTION_SET_TRACE, &trace);
		if (res < 0) {
			GYROS_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
				ID_GYROSCOPE, CUST_ACTION_SET_TRACE);
			return 0;
		}
	} else {
		GYROS_PR_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	if (obj == NULL) {
		GYROS_PR_ERR(" obj is null!!\n");
		return 0;
	}

	return len;
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *buf)
{
	ssize_t _tLength = 0;
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	_tLength = snprintf(buf, PAGE_SIZE, "default direction = %d\n", obj->direction);

	return _tLength;
}

static ssize_t store_chip_orientation(struct device_driver *ddri, const char *buf, size_t tCount)
{
	int _nDirection = 0, ret = 0;
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	if (obj == NULL)
		return 0;
	ret = kstrtoint(buf, 10, &_nDirection);
	if (ret == 0) {
		obj->direction = _nDirection;
		ret = sensor_set_cmd_to_hub(ID_GYROSCOPE, CUST_ACTION_SET_DIRECTION, &_nDirection);
		if (ret < 0) {
			GYROS_PR_ERR("sensor_set_cmd_to_hub fail, (ID: %d),(action: %d)\n",
				ID_GYROSCOPE, CUST_ACTION_SET_DIRECTION);
			return 0;
		}
	}

	GYROS_LOG("[%s] set direction: %d\n", __func__, _nDirection);

	return tCount;
}

static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);

static struct driver_attribute *gyrohub_attr_list[] = {
	&driver_attr_chipinfo,	/*chip information */
	&driver_attr_sensordata,	/*dump sensor data */
	&driver_attr_trace,	/*trace log */
	&driver_attr_status,
	&driver_attr_orientation,
};

static int gyrohub_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(gyrohub_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, gyrohub_attr_list[idx]);
		if (err != 0) {
			GYROS_PR_ERR("driver_create_file (%s) = %d\n", gyrohub_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int gyrohub_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(gyrohub_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, gyrohub_attr_list[idx]);

	return err;
}

static void scp_init_work_done(struct work_struct *work)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	int err = 0;

	if (atomic_read(&obj->scp_init_done) == 0) {
		GYROS_PR_ERR("scp is not ready to send cmd\n");
	} else {
		if (atomic_read(&obj->first_ready_after_boot) == 0) {
			atomic_set(&obj->first_ready_after_boot, 1);
		} else {
			err = gyrohub_WriteCalibration_scp(obj->static_cali);
			if (err < 0)
				GYROS_PR_ERR("gyrohub_WriteCalibration_scp fail\n");
			err = sensor_cfg_to_hub(ID_GYROSCOPE, (uint8_t *)obj->dynamic_cali, sizeof(obj->dynamic_cali));
			if (err < 0)
				GYROS_PR_ERR("sensor_cfg_to_hub fail\n");
		}
	}
}

static int gyro_recv_data(struct data_unit_t *event, void *reserved)
{
	int err = 0;
	struct gyrohub_ipi_data *obj = obj_ipi_data;
	struct gyro_data data;

	data.x = event->gyroscope_t.x;
	data.y = event->gyroscope_t.y;
	data.z = event->gyroscope_t.z;
	data.status = event->gyroscope_t.status;
	data.timestamp = (int64_t)(event->time_stamp + event->time_stamp_gpt);
	data.reserved[0] = event->reserve[0];

	if (event->flush_action == FLUSH_ACTION)
		err = gyro_flush_report();
	else if (event->flush_action == DATA_ACTION)
		err = gyro_data_report(&data);
	else if (event->flush_action == BIAS_ACTION) {
		data.x = event->gyroscope_t.x_bias;
		data.y = event->gyroscope_t.y_bias;
		data.z = event->gyroscope_t.z_bias;
		err = gyro_bias_report(&data);
		obj->dynamic_cali[GYROHUB_AXIS_X] = event->gyroscope_t.x_bias;
		obj->dynamic_cali[GYROHUB_AXIS_Y] = event->gyroscope_t.y_bias;
		obj->dynamic_cali[GYROHUB_AXIS_Z] = event->gyroscope_t.z_bias;
	}
	return err;
}
static int gyrohub_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err = 0;

	if (enabledisable == true) {
		err = sensor_set_delay_to_hub(ID_GYROSCOPE, sample_periods_ms);
		if (err) {
			GYROS_PR_ERR("sensor_set_delay_to_hub failed!\n");
			return -1;
		}
	}
	err = sensor_enable_to_hub(ID_GYROSCOPE, enabledisable);
	if (err) {
		GYROS_PR_ERR("sensor_enable_to_hub failed!\n");
		return -1;
	}
	return 0;
}
static int gyrohub_factory_get_data(int32_t data[3], int *status)
{
	return gyrohub_get_data(&data[0], &data[1], &data[2], status);
}
static int gyrohub_factory_get_raw_data(int32_t data[3])
{
	GYROS_LOG("don't support gyrohub_factory_get_raw_data!\n");
	return 0;
}
static int gyrohub_factory_enable_calibration(void)
{
	return sensor_calibration_to_hub(ID_GYROSCOPE);
}
static int gyrohub_factory_clear_cali(void)
{
	int err = 0;

	err = gyrohub_ResetCalibration();
	if (err) {
		GYROS_PR_ERR("gyrohub_ResetCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int gyrohub_factory_set_cali(int32_t data[3])
{
	int err = 0;

	err = gyrohub_WriteCalibration(data);
	if (err) {
		GYROS_PR_ERR("gyrohub_WriteCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int gyrohub_factory_get_cali(int32_t data[3])
{
	int err = 0;

	err = gyrohub_ReadCalibration(data);
	if (err) {
		GYROS_PR_ERR("gyrohub_ReadCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int gyrohub_factory_do_self_test(void)
{
	return 0;
}

static struct gyro_factory_fops gyrohub_factory_fops = {
	.enable_sensor = gyrohub_factory_enable_sensor,
	.get_data = gyrohub_factory_get_data,
	.get_raw_data = gyrohub_factory_get_raw_data,
	.enable_calibration = gyrohub_factory_enable_calibration,
	.clear_cali = gyrohub_factory_clear_cali,
	.set_cali = gyrohub_factory_set_cali,
	.get_cali = gyrohub_factory_get_cali,
	.do_self_test = gyrohub_factory_do_self_test,
};

static struct gyro_factory_public gyrohub_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &gyrohub_factory_fops,
};

static int gyrohub_open_report_data(int open)
{
	return 0;
}

static int gyrohub_enable_nodata(int en)
{
	int res = 0;
	bool power = false;

	if (en == 1)
		power = true;
	if (en == 0)
		power = false;

	res = gyrohub_SetPowerMode(power);
	if (res < 0) {
		GYROS_PR_ERR("GYROHUB_SetPowerMode fail\n");
		return res;
	}
	GYROS_LOG("gyrohub_enable_nodata OK!\n");
	return 0;

}

static int gyrohub_set_delay(u64 ns)
{
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	int err = 0;
	int value = 0;
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	value = (int)ns / 1000 / 1000;
	err = sensor_set_delay_to_hub(ID_GYROSCOPE, value);
	if (err < 0) {
		GYROS_PR_ERR("sensor_set_delay_to_hub fail!\n");
		return err;
	}

	GYROS_LOG("gyro_set_delay (%d)\n", value);
	return err;
#elif defined CONFIG_NANOHUB
	return 0;
#else
	return 0;
#endif
}
static int gyrohub_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	gyrohub_set_delay(samplingPeriodNs);
#endif
	return sensor_batch_to_hub(ID_GYROSCOPE, flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int gyrohub_flush(void)
{
	return sensor_flush_to_hub(ID_GYROSCOPE);
}

static int gyrohub_set_cali(uint8_t *data, uint8_t count)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	memcpy(obj->dynamic_cali, data, count);
	return sensor_cfg_to_hub(ID_GYROSCOPE, data, count);
}

static int gpio_config(void)
{
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;

	if (gyroPltFmDev == NULL) {
		GYRO_PR_ERR("Cannot find gyro device!\n");
		return 0;
	}

	pinctrl = devm_pinctrl_get(&gyroPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GYRO_PR_ERR("Cannot find gyro pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		GYRO_PR_ERR("Cannot find gyro pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GYRO_PR_ERR("Cannot find gyro pinctrl pin_cfg!\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);

	return 0;
}

static int gyrohub_get_data(int *x, int *y, int *z, int *status)
{
	char buff[GYROHUB_BUFSIZE];
	int err = 0;

	err = gyrohub_ReadGyroData(buff, GYROHUB_BUFSIZE);
	if (err < 0) {
		GYROS_PR_ERR("gyrohub_ReadGyroData fail!!\n");
		return -1;
	}
	err = sscanf(buff, "%x %x %x %x", x, y, z, status);
	if (err != 4) {
		GYROS_PR_ERR("sscanf fail!!\n");
		return -1;
	}
	return 0;
}
static int scp_ready_event(uint8_t event, void *ptr)
{
	struct gyrohub_ipi_data *obj = obj_ipi_data;

	switch (event) {
	case SENSOR_POWER_UP:
	    atomic_set(&obj->scp_init_done, 1);
		schedule_work(&obj->init_done_work);
	    break;
	case SENSOR_POWER_DOWN:
	    atomic_set(&obj->scp_init_done, 0);
	    break;
	}
	return 0;
}
static struct scp_power_monitor scp_ready_notifier = {
	.name = "gyro",
	.notifier_call = scp_ready_event,
};
static int gyrohub_probe(struct platform_device *pdev)
{
	struct gyrohub_ipi_data *obj;
	int err = 0;
	struct gyro_control_path ctl = { 0 };
	struct gyro_data_path data = { 0 };

	GYROS_FUN();
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct gyrohub_ipi_data));

	obj_ipi_data = obj;
	platform_set_drvdata(pdev, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	atomic_set(&obj->first_ready_after_boot, 0);
	atomic_set(&obj->scp_init_done, 0);
	INIT_WORK(&obj->init_done_work, scp_init_work_done);

	err = gpio_config();
	if (err < 0) {
		GYROS_PR_ERR("gpio_config failed\n");
		goto exit_kfree;
	}
	scp_power_monitor_register(&scp_ready_notifier);
	err = scp_sensorHub_data_registration(ID_GYROSCOPE, gyro_recv_data);
	if (err < 0) {
		GYROS_PR_ERR("scp_sensorHub_data_registration failed\n");
		goto exit_kfree;
	}
	err = gyro_factory_device_register(&gyrohub_factory_device);
	if (err) {
		GYROS_PR_ERR("gyro_factory_device_register fail err = %d\n", err);
		goto exit_kfree;
	}
	ctl.is_use_common_factory = true;

	err = gyrohub_create_attr(&(gyrohub_init_info.platform_diver_addr->driver));
	if (err) {
		GYROS_PR_ERR("gyrohub create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = gyrohub_open_report_data;
	ctl.enable_nodata = gyrohub_enable_nodata;
	ctl.set_delay = gyrohub_set_delay;
	ctl.batch = gyrohub_batch;
	ctl.flush = gyrohub_flush;
	ctl.set_cali = gyrohub_set_cali;
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	ctl.is_report_input_direct = true;
	ctl.is_support_batch = false;
#elif defined CONFIG_NANOHUB
	ctl.is_report_input_direct = true;
	ctl.is_support_batch = false;
#else
#endif

	err = gyro_register_control_path(&ctl);
	if (err) {
		GYROS_PR_ERR("register gyro control path err\n");
		goto exit_create_attr_failed;
	}

	data.get_data = gyrohub_get_data;
	data.vender_div = DEGREE_TO_RAD;
	err = gyro_register_data_path(&data);
	if (err) {
		GYROS_PR_ERR("gyro_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	gyrohub_init_flag = 0;

	GYROS_LOG("%s: OK\n", __func__);
	return 0;
exit_create_attr_failed:
	gyrohub_delete_attr(&(gyrohub_init_info.platform_diver_addr->driver));
exit_kfree:
	kfree(obj);
	obj_ipi_data = NULL;
exit:
	gyrohub_init_flag = -1;
	GYROS_PR_ERR("%s: err = %d\n", __func__, err);
	return err;
}

static int gyrohub_remove(struct platform_device *pdev)
{
	int err = 0;

	err = gyrohub_delete_attr(&(gyrohub_init_info.platform_diver_addr->driver));
	if (err)
		GYROS_PR_ERR("gyrohub_delete_attr fail: %d\n", err);

	gyro_factory_device_deregister(&gyrohub_factory_device);

	kfree(platform_get_drvdata(pdev));
	return 0;
}

static int gyrohub_suspend(struct platform_device *pdev, pm_message_t msg)
{
	return 0;
}

static int gyrohub_resume(struct platform_device *pdev)
{
	return 0;
}
static struct platform_device gyrohub_device = {
	.name = GYROHUB_DEV_NAME,
	.id = -1,
};
static struct platform_driver gyrohub_driver = {
	.driver = {
		   .name = GYROHUB_DEV_NAME,
	},
	.probe = gyrohub_probe,
	.remove = gyrohub_remove,
	.suspend = gyrohub_suspend,
	.resume = gyrohub_resume,
};

static int gyrohub_local_remove(void)
{
	platform_driver_unregister(&gyrohub_driver);
	return 0;
}

static int gyrohub_local_init(struct platform_device *pdev)
{
	gyroPltFmDev = pdev;

	if (platform_driver_register(&gyrohub_driver)) {
		GYROS_PR_ERR("add driver error\n");
		return -1;
	}
	if (-1 == gyrohub_init_flag)
		return -1;
	return 0;
}
static struct gyro_init_info gyrohub_init_info = {
	.name = "gyrohub",
	.init = gyrohub_local_init,
	.uninit = gyrohub_local_remove,
};

static int __init gyrohub_init(void)
{

	if (platform_device_register(&gyrohub_device)) {
		GYROS_PR_ERR("platform device error\n");
		return -1;
	}
	gyro_driver_add(&gyrohub_init_info);

	return 0;
}

static void __exit gyrohub_exit(void)
{
	GYROS_FUN();
}

module_init(gyrohub_init);
module_exit(gyrohub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GYROHUB gyroscope driver");
MODULE_AUTHOR("hongxu.zhao@mediatek.com");
