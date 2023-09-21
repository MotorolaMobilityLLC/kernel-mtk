// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include "dw9781.h"
#include "dw9781_i2c.h"
#include "ois_ext_cmd.h"
#define DRIVER_NAME "ak7377a"
#define AK7377A_I2C_SLAVE_ADDR 0x18

#define LOG_INF(format, args...)                                               \
	pr_err(DRIVER_NAME " [%s] " format, __func__, ##args)

#define AK7377A_NAME				"mot_dubai_ak7377a"
#define AK7377A_MAX_FOCUS_POS			1023

#define AFIOC_G_AFPOS _IOWR('A', 35, int)
/*
 * This sets the minimum granularity for the focus positions.
 * A value of 1 gives maximum accuracy for a desired focus position
 */
#define AK7377A_FOCUS_STEPS			1
#define AK7377A_SET_POSITION_ADDR		0x00

#define AK7377A_CMD_DELAY			0xff
#define AK7377A_CTRL_DELAY_US			10000
/*
 * This acts as the minimum granularity of lens movement.
 * Keep this value power of 2, so the control steps can be
 * uniformly adjusted for gradual lens movement, with desired
 * number of control steps.
 */
#define AK7377A_MOVE_STEPS			16
#define AK7377A_MOVE_DELAY_US			8400
#define AK7377A_STABLE_TIME_US			20000
extern void dw9781_set_i2c_client(struct i2c_client *i2c_client);
extern int dw9781c_download_ois_fw(void);
static struct i2c_client *g_pstAF_I2Cclient;
typedef struct {
	motOISExtInfType ext_state;
	motOISExtIntf ext_data;
	struct work_struct ext_work;
} ois_ext_work_struct;

typedef struct {
	int max_val;
	int min_val;
} motAfTestData;
static struct workqueue_struct *ois_ext_workqueue;
static ois_ext_work_struct ois_ext_work;

static DEFINE_MUTEX(ois_mutex);
/* ak7377a device structure */
struct ak7377a_device {
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_subdev sd;
	struct v4l2_ctrl *focus;
	struct regulator *vin;
	struct regulator *vdd;
    	struct regulator *dovdd;
	struct pinctrl *vcamaf_pinctrl;
	struct pinctrl_state *vcamaf_on;
	struct pinctrl_state *vcamaf_off;
};
#define DW9781_I2C_SLAVE_ADDR 0xE4
static motOISExtData *pDW9781TestResult = NULL;

int MOT_DW9781CAF_EXT_CMD_HANDLER(motOISExtIntf *pExtCmd)
{
	dw9781_set_i2c_client(g_pstAF_I2Cclient);

	switch (pExtCmd->cmd) {
		case OIS_SART_FW_DL:
			LOG_INF("Kernel OIS_SART_FW_DL\n");
			break;
		case OIS_START_HEA_TEST:
			{
				if (!pDW9781TestResult) {
					pDW9781TestResult = (motOISExtData *)kzalloc(sizeof(motOISExtData), GFP_NOWAIT);
				}

				LOG_INF("Kernel OIS_START_HEA_TEST\n");
				if (pDW9781TestResult) {
					LOG_INF("OIS raius:%d,accuracy:%d,step/deg:%d,wait0:%d,wait1:%d,wait2:%d, ref_stroke:%d",
					        pExtCmd->data.hea_param.radius,
					        pExtCmd->data.hea_param.accuracy,
					        pExtCmd->data.hea_param.steps_in_degree,
					        pExtCmd->data.hea_param.wait0,
					        pExtCmd->data.hea_param.wait1,
					        pExtCmd->data.hea_param.wait2,
					        pExtCmd->data.hea_param.ref_stroke);
					square_motion_test(pExtCmd->data.hea_param.radius,
					                   pExtCmd->data.hea_param.accuracy,
					                   pExtCmd->data.hea_param.steps_in_degree,
					                   pExtCmd->data.hea_param.wait0,
					                   pExtCmd->data.hea_param.wait1,
					                   pExtCmd->data.hea_param.wait2,
					                   pExtCmd->data.hea_param.ref_stroke,
					                   pDW9781TestResult);
					LOG_INF("OIS HALL NG points:%d", pDW9781TestResult->hea_result.ng_points);
				} else {
					LOG_INF("FATAL: Kernel OIS_START_HEA_TEST memory error!!!\n");
				}
				break;
			}
		case OIS_START_GYRO_OFFSET_CALI:
			LOG_INF("Kernel OIS_START_GYRO_OFFSET_CALI\n");
			gyro_offset_calibrtion();
			break;
		default:
			LOG_INF("Kernel OIS invalid cmd\n");
			break;
	}
	return 0;
}

int MOT_DW9781CAF_GET_TEST_RESULT(motOISExtIntf *pExtCmd)
{
	switch (pExtCmd->cmd) {
		case OIS_QUERY_FW_INFO:
			LOG_INF("Kernel OIS_QUERY_FW_INFO\n");
			memset(&pExtCmd->data.fw_info, 0xff, sizeof(motOISFwInfo));
			break;
		case OIS_QUERY_HEA_RESULT:
			{
				LOG_INF("Kernel OIS_QUERY_HEA_RESULT\n");
				if (pDW9781TestResult) {
					memcpy(&pExtCmd->data.hea_result, &pDW9781TestResult->hea_result, sizeof(motOISHeaResult));
					LOG_INF("OIS NG points:%d, Ret:%d", pDW9781TestResult->hea_result.ng_points, pExtCmd->data.hea_result.ng_points);
				}
				break;
			}
		case OIS_QUERY_GYRO_OFFSET_RESULT:
			{
				motOISGOffsetResult *gOffset = dw9781_get_gyro_offset_result();
				LOG_INF("Kernel OIS_QUERY_GYRO_OFFSET_RESULT\n");
				memcpy(&pExtCmd->data.gyro_offset_result, gOffset, sizeof(motOISGOffsetResult));
				break;
			}
		default:
			LOG_INF("Kernel OIS invalid cmd\n");
			break;
	}
	return 0;
}

int MOT_DW9781CAF_SET_CALIBRATION(motOISExtIntf *pExtCmd)
{
	switch (pExtCmd->cmd) {
		case OIS_SET_GYRO_OFFSET:
			{
				if (pExtCmd->data.gyro_offset_result.is_success == 0 &&
				    pExtCmd->data.gyro_offset_result.x_offset != 0 &&
				    pExtCmd->data.gyro_offset_result.y_offset != 0) {
					motOISGOffsetResult *gOffset = dw9781_get_gyro_offset_result();

					//Update the gyro offset
					gOffset->is_success = 0;
					gOffset->x_offset = pExtCmd->data.gyro_offset_result.x_offset;
					gOffset->y_offset = pExtCmd->data.gyro_offset_result.y_offset;

					//Check if gyro offset update needed
					gyro_offset_check_update();
					LOG_INF("[%s] OIS update gyro_offset: %d,%d\n", __func__, gOffset->x_offset, gOffset->y_offset);
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

/* OIS extended interfaces */
static void ois_ext_interface(struct work_struct *data)
{
	ois_ext_work_struct *pwork = container_of(data, ois_ext_work_struct, ext_work);

	if (!pwork) return;

	mutex_lock(&ois_mutex);

	MOT_DW9781CAF_EXT_CMD_HANDLER(&pwork->ext_data);
	mutex_unlock(&ois_mutex);
	LOG_INF("OIS ext_data:%p, cmd:%d", pwork->ext_data, pwork->ext_data.cmd);
}

static inline struct ak7377a_device *to_ak7377a_vcm(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct ak7377a_device, ctrls);
}

static inline struct ak7377a_device *sd_to_ak7377a_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct ak7377a_device, sd);
}

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};


static int ak7377a_set_position(struct ak7377a_device *ak7377a, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ak7377a->sd);
	int retry = 3;
	int ret;

	while (--retry > 0) {
		ret = i2c_smbus_write_word_data(client, AK7377A_SET_POSITION_ADDR,
					 swab16(val << 6));
		/*
		Write AF DAC value into AF drift compensation register for DW9781C,
		MTK platform only support 10bit DAC value, OIS need 12 bit one.
		*/
		write_reg_16bit_value_16bit(0x7329, val<<2);
		if (ret < 0) {
			usleep_range(AK7377A_MOVE_DELAY_US,
				     AK7377A_MOVE_DELAY_US + 1000);
		} else {
			break;
		}
	}
	return ret;
}

static int ak7377a_release(struct ak7377a_device *ak7377a)
{
	int ret, val;
	struct i2c_client *client = v4l2_get_subdevdata(&ak7377a->sd);

	for (val = round_down(ak7377a->focus->val, AK7377A_MOVE_STEPS);
	     val >= 0; val -= AK7377A_MOVE_STEPS) {
		ret = ak7377a_set_position(ak7377a, val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
		usleep_range(AK7377A_MOVE_DELAY_US,
			     AK7377A_MOVE_DELAY_US + 1000);
	}

	i2c_smbus_write_byte_data(client, 0x02, 0x20);
	msleep(20);

	/*
	 * Wait for the motor to stabilize after the last movement
	 * to prevent the motor from shaking.
	 */
	usleep_range(AK7377A_STABLE_TIME_US - AK7377A_MOVE_DELAY_US,
		     AK7377A_STABLE_TIME_US - AK7377A_MOVE_DELAY_US + 1000);
	flush_work(&ois_ext_work.ext_work);
	if (ois_ext_workqueue) {
		flush_workqueue(ois_ext_workqueue);
		destroy_workqueue(ois_ext_workqueue);
		ois_ext_workqueue = NULL;
	}
	if (pDW9781TestResult) {
		kfree(pDW9781TestResult);
		pDW9781TestResult = NULL;
	}
	return 0;
}
extern void ois_ready_check(void);
static int ak7377a_init(struct ak7377a_device *ak7377a)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ak7377a->sd);
	int ret = 0;
	unsigned short lock_ois = 0;
	LOG_INF("+\n");

	client->addr = AK7377A_I2C_SLAVE_ADDR >> 1;
	//ret = i2c_smbus_read_byte_data(client, 0x02);

	LOG_INF("Check HW version: %x\n", ret);

	/* 00:active mode , 10:Standby mode , x1:Sleep mode */
	ret = i2c_smbus_write_byte_data(client, 0x02, 0x00);
	client->addr = DW9781_I2C_SLAVE_ADDR >> 1;
	dw9781c_download_ois_fw();
	ois_reset();
	write_reg_16bit_value_16bit(0x73D4, 0x8000); //Set as tripod mode
	write_reg_16bit_value_16bit(0x7015, 0x00);
	read_reg_16bit_value_16bit(0x7015, &lock_ois); /* lock_ois: 0x7015 */
	LOG_INF("Check HW lock_ois: %x\n", lock_ois);
	client->addr = AK7377A_I2C_SLAVE_ADDR >> 1;
	LOG_INF("-\n");
	return 0;
}

/* Power handling */
static int ak7377a_power_off(struct ak7377a_device *ak7377a)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = ak7377a_release(ak7377a);
	if (ret)
		LOG_INF("ak7377a release failed!\n");

	ret = regulator_disable(ak7377a->vin);
	if (ret)
		return ret;

	ret = regulator_disable(ak7377a->vdd);
	if (ret)
		return ret;
	ret = regulator_disable(ak7377a->dovdd);
	if (ret)
		return ret;
	if (ak7377a->vcamaf_pinctrl && ak7377a->vcamaf_off)
		ret = pinctrl_select_state(ak7377a->vcamaf_pinctrl,
					ak7377a->vcamaf_off);

	return ret;
}

static int ak7377a_power_on(struct ak7377a_device *ak7377a)
{
	int ret;

	LOG_INF("%s\n", __func__);
	ret = regulator_enable(ak7377a->dovdd);
	if (ret < 0)
		return ret;
	ret = regulator_enable(ak7377a->vin);
	if (ret < 0)
		return ret;

	ret = regulator_enable(ak7377a->vdd);
	if (ret < 0)
		return ret;

	if (ak7377a->vcamaf_pinctrl && ak7377a->vcamaf_on)
		ret = pinctrl_select_state(ak7377a->vcamaf_pinctrl,
					ak7377a->vcamaf_on);

	if (ret < 0)
		return ret;

	/*
	 * TODO(b/139784289): Confirm hardware requirements and adjust/remove
	 * the delay.
	 */
	usleep_range(AK7377A_CTRL_DELAY_US, AK7377A_CTRL_DELAY_US + 100);

	ret = ak7377a_init(ak7377a);
	if (ret < 0)
		goto fail;

	return 0;

fail:
	regulator_disable(ak7377a->vin);
	regulator_disable(ak7377a->vdd);
	regulator_disable(ak7377a->dovdd);
	if (ak7377a->vcamaf_pinctrl && ak7377a->vcamaf_off) {
		pinctrl_select_state(ak7377a->vcamaf_pinctrl,
				ak7377a->vcamaf_off);
	}

	return ret;
}
static int ak7377a_test_hall_set_position(struct v4l2_subdev  * sd, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int retry = 3;
	int ret;

	while (--retry > 0) {
		ret = i2c_smbus_write_word_data(client, AK7377A_SET_POSITION_ADDR,
					 swab16(val << 4));
		if (ret < 0) {
			usleep_range(AK7377A_MOVE_DELAY_US,
				     AK7377A_MOVE_DELAY_US + 1000);
		} else {
			break;
		}
	}
	return ret;
}

static int ak7377a_test_hall_GetResult(struct v4l2_subdev  * sd,int mode)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int retry = 10;
	int ret;
	int val;
	int val_L = 0;
	int val_H = 0;
	int max_val =0;
	int min_val=0;
	msleep(2);
	while (retry > 0)
	{
		ret = i2c_smbus_read_word_data(client, 0x84);
		msleep(2);
		val_L=(ret & 0xf000) >> 12;
		val_H = (ret & 0x00ff) << 4;
		val = val_L | val_H;
		LOG_INF("value =%d",val);
		if(mode ==0)
		{
			if(val  > max_val)
			{
				max_val=val;
			}
		}else{
			if(val  < min_val)
			{
				min_val=val;
			}
		}
		retry--;
	}
	if(mode ==0)
	{
		return max_val;
	}else{
		return min_val;
	}
}

static long ak7377a_ops_core_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	motAfTestData  afdata;
	switch(cmd) {
	case VIDIOC_MTK_S_OIS_MODE:
	{
		int *ois_mode = arg;
		LOG_INF("VIDIOC_MTK_S_OIS_MODE mode = %d\n",*ois_mode);
		if (*ois_mode) {
			write_reg_16bit_value_16bit(0x73D4, 0x8000); //Set as tripod mode
			write_reg_16bit_value_16bit(0x7015, 0x00);
			LOG_INF("VIDIOC_MTK_S_OIS_MODE Enable\n");
		} else {
			write_reg_16bit_value_16bit(0x7015, 0x01);
			LOG_INF("VIDIOC_MTK_S_OIS_MODE Disable\n");
		}
	}
	break;
	case AFIOC_G_OISEXTINTF: {
		motOISExtIntf *pOisExtData = arg;
		memcpy(&ois_ext_work.ext_data, pOisExtData, sizeof(motOISExtIntf));
		if ((ois_ext_work.ext_data.cmd > OIS_EXT_INVALID_CMD) && (ois_ext_work.ext_data.cmd <= OIS_ACTION_MAX)) {
			//Raise new thread to avoid long execution time block capture requests
			LOG_INF("OIS ext_state:%d, cmd:%d", ois_ext_work.ext_state, ois_ext_work.ext_data.cmd);
			if (ois_ext_work.ext_state != ois_ext_work.ext_data.cmd) {

				if (ois_ext_workqueue) {
					LOG_INF("OIS queue ext work...");
					queue_work(ois_ext_workqueue, &ois_ext_work.ext_work);
				}

				ois_ext_work.ext_state = ois_ext_work.ext_data.cmd;
			}
		} else if ((ois_ext_work.ext_data.cmd > OIS_ACTION_MAX) && (ois_ext_work.ext_data.cmd <= OIS_EXT_INTF_MAX)) {
			//wait till result ready
			{
					motOISExtIntf * pResultData = NULL;

					pResultData =  (motOISExtIntf *) kzalloc(sizeof(motOISExtIntf), GFP_KERNEL);

				        if (!pResultData) {
						ret = -ENOMEM;
						break;
					}

					pResultData->cmd = ois_ext_work.ext_data.cmd;
					mutex_lock(&ois_mutex);
					MOT_DW9781CAF_GET_TEST_RESULT(pResultData);
					mutex_unlock(&ois_mutex);
					memcpy((void *)arg, pResultData, sizeof(motOISExtIntf));
					ois_ext_work.ext_state = ois_ext_work.ext_data.cmd;

					if (pResultData) {
						kfree(pResultData);
						pResultData = NULL;
					}

			}
		} else if (ois_ext_work.ext_data.cmd == OIS_SET_GYRO_OFFSET) {
				mutex_lock(&ois_mutex);
				MOT_DW9781CAF_SET_CALIBRATION(&ois_ext_work.ext_data);
				mutex_unlock(&ois_mutex);
				LOG_INF("OIS update gyro_offset to %d, %d",
						ois_ext_work.ext_data.data.gyro_offset_result.x_offset,
						ois_ext_work.ext_data.data.gyro_offset_result.y_offset);
		}
	}
	break;
	case AFIOC_G_AFPOS:
	{
		ret = ak7377a_test_hall_set_position(sd,0xfff);       //4095
		afdata.max_val= ak7377a_test_hall_GetResult(sd,0);    //mdoe: 0  macro
		ret = ak7377a_test_hall_set_position(sd,0x00);        //0
		afdata.min_val=ak7377a_test_hall_GetResult(sd,1);     //mdoe: 1  inf
		LOG_INF("max =%d min =%d ",afdata.max_val,afdata.min_val);
		memcpy((void *)arg, &afdata, sizeof(motAfTestData));
	}
	break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

static int ak7377a_set_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct ak7377a_device *ak7377a = to_ak7377a_vcm(ctrl);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {
		LOG_INF("pos(%d)\n", ctrl->val);
		ret = ak7377a_set_position(ak7377a, ctrl->val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}

static const struct v4l2_ctrl_ops ak7377a_vcm_ctrl_ops = {
	.s_ctrl = ak7377a_set_ctrl,
};

static int ak7377a_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = pm_runtime_get_sync(sd->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(sd->dev);
		return ret;
	}
	/* OIS ext interfaces for test and firmware checking */
	INIT_WORK(&ois_ext_work.ext_work, ois_ext_interface);
	if (ois_ext_workqueue == NULL) {
		ois_ext_workqueue = create_singlethread_workqueue("ois_ext_intf");
	}
	/* ------------------------- */
	return 0;
}

static int ak7377a_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	LOG_INF("%s\n", __func__);

	pm_runtime_put(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops ak7377a_int_ops = {
	.open = ak7377a_open,
	.close = ak7377a_close,
};

static struct v4l2_subdev_core_ops ak7377a_ops_core = {
	.ioctl = ak7377a_ops_core_ioctl,
};

static const struct v4l2_subdev_ops ak7377a_ops = {
	.core = &ak7377a_ops_core,
};

static void ak7377a_subdev_cleanup(struct ak7377a_device *ak7377a)
{
	v4l2_async_unregister_subdev(&ak7377a->sd);
	v4l2_ctrl_handler_free(&ak7377a->ctrls);
#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&ak7377a->sd.entity);
#endif
}

static int ak7377a_init_controls(struct ak7377a_device *ak7377a)
{
	struct v4l2_ctrl_handler *hdl = &ak7377a->ctrls;
	const struct v4l2_ctrl_ops *ops = &ak7377a_vcm_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	ak7377a->focus = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, AK7377A_MAX_FOCUS_POS, AK7377A_FOCUS_STEPS, 0);

	if (hdl->error)
		return hdl->error;

	ak7377a->sd.ctrl_handler = hdl;

	return 0;
}


static int ak7377a_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct ak7377a_device *ak7377a;
	int ret;

	LOG_INF("%s\n", __func__);
	g_pstAF_I2Cclient = client;
	ak7377a = devm_kzalloc(dev, sizeof(*ak7377a), GFP_KERNEL);
	if (!ak7377a)
		return -ENOMEM;

	ak7377a->dovdd = devm_regulator_get(dev, "dovdd");
	if (IS_ERR(ak7377a->dovdd)) {
		ret = PTR_ERR(ak7377a->dovdd);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vin regulator\n");
		return ret;
	}

	ak7377a->vin = devm_regulator_get(dev, "vin");
	if (IS_ERR(ak7377a->vin)) {
		ret = PTR_ERR(ak7377a->vin);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vin regulator\n");
		return ret;
	}

	ak7377a->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(ak7377a->vdd)) {
		ret = PTR_ERR(ak7377a->vdd);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vdd regulator\n");
		return ret;
	}

	ak7377a->vcamaf_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(ak7377a->vcamaf_pinctrl)) {
		ret = PTR_ERR(ak7377a->vcamaf_pinctrl);
		ak7377a->vcamaf_pinctrl = NULL;
		LOG_INF("cannot get pinctrl\n");
	} else {
		ak7377a->vcamaf_on = pinctrl_lookup_state(
			ak7377a->vcamaf_pinctrl, "vcamaf_on");

		if (IS_ERR(ak7377a->vcamaf_on)) {
			ret = PTR_ERR(ak7377a->vcamaf_on);
			ak7377a->vcamaf_on = NULL;
			LOG_INF("cannot get vcamaf_on pinctrl\n");
		}

		ak7377a->vcamaf_off = pinctrl_lookup_state(
			ak7377a->vcamaf_pinctrl, "vcamaf_off");

		if (IS_ERR(ak7377a->vcamaf_off)) {
			ret = PTR_ERR(ak7377a->vcamaf_off);
			ak7377a->vcamaf_off = NULL;
			LOG_INF("cannot get vcamaf_off pinctrl\n");
		}
	}

	v4l2_i2c_subdev_init(&ak7377a->sd, client, &ak7377a_ops);
	ak7377a->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ak7377a->sd.internal_ops = &ak7377a_int_ops;
	dw9781_set_i2c_client(g_pstAF_I2Cclient);
	ret = ak7377a_init_controls(ak7377a);
	if (ret)
		goto err_cleanup;

#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	ret = media_entity_pads_init(&ak7377a->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;

	ak7377a->sd.entity.function = MEDIA_ENT_F_LENS;
#endif

	ret = v4l2_async_register_subdev(&ak7377a->sd);
	if (ret < 0)
		goto err_cleanup;

	pm_runtime_enable(dev);
	return 0;

err_cleanup:
	ak7377a_subdev_cleanup(ak7377a);
	return ret;
}

static int ak7377a_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ak7377a_device *ak7377a = sd_to_ak7377a_vcm(sd);

	LOG_INF("%s\n", __func__);

	ak7377a_subdev_cleanup(ak7377a);
	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		ak7377a_power_off(ak7377a);
	pm_runtime_set_suspended(&client->dev);

	return 0;
}

static int __maybe_unused ak7377a_vcm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ak7377a_device *ak7377a = sd_to_ak7377a_vcm(sd);

	return ak7377a_power_off(ak7377a);
}

static int __maybe_unused ak7377a_vcm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ak7377a_device *ak7377a = sd_to_ak7377a_vcm(sd);

	return ak7377a_power_on(ak7377a);
}

static const struct i2c_device_id ak7377a_id_table[] = {
	{ AK7377A_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ak7377a_id_table);

static const struct of_device_id ak7377a_of_table[] = {
	{ .compatible = "mediatek,mot_dubai_ak7377a" },
	{ },
};
MODULE_DEVICE_TABLE(of, ak7377a_of_table);

static const struct dev_pm_ops ak7377a_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(ak7377a_vcm_suspend, ak7377a_vcm_resume, NULL)
};

static struct i2c_driver ak7377a_i2c_driver = {
	.driver = {
		.name = AK7377A_NAME,
		.pm = &ak7377a_pm_ops,
		.of_match_table = ak7377a_of_table,
	},
	.probe_new  = ak7377a_probe,
	.remove = ak7377a_remove,
	.id_table = ak7377a_id_table,
};

module_i2c_driver(ak7377a_i2c_driver);

MODULE_AUTHOR("Po-Hao Huang <Po-Hao.Huang@mediatek.com>");
MODULE_DESCRIPTION("AK7377A VCM driver");
MODULE_LICENSE("GPL v2");
