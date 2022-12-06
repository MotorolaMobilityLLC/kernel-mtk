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

#define DRIVER_NAME "mot_aion_gt9764"
#define MOT_AION_GT9764_I2C_SLAVE_ADDR 0x18

#define LOG_INF(format, args...)                                               \
	pr_info(DRIVER_NAME " [%s] " format, __func__, ##args)

#define MOT_AION_GT9764_NAME				"mot_aion_gt9764"
#define MOT_AION_GT9764_MAX_FOCUS_POS			1023
/*
 * This sets the minimum granularity for the focus positions.
 * A value of 1 gives maximum accuracy for a desired focus position
 */
#define MOT_AION_GT9764_FOCUS_STEPS			1
#define MOT_AION_GT9764_SET_POSITION_ADDR		0x03

#define MOT_AION_GT9764_CMD_DELAY			0xff
#define MOT_AION_GT9764_CTRL_DELAY_US			5000
/*
 * This acts as the minimum granularity of lens movement.
 * Keep this value power of 2, so the control steps can be
 * uniformly adjusted for gradual lens movement, with desired
 * number of control steps.
 */
#define MOT_AION_GT9764_MOVE_STEPS			100
#define MOT_AION_GT9764_MOVE_DELAY_US			5000
#define MOT_AION_GT9764_STABLE_TIME_US			6000

/* mot_aion_gt9764 device structure */
struct mot_aion_gt9764_device {
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_subdev sd;
	struct v4l2_ctrl *focus;
	struct regulator *vin;
	struct regulator *vdd;
	struct pinctrl *vcamaf_pinctrl;
	struct pinctrl_state *vcamaf_on;
	struct pinctrl_state *vcamaf_off;
};

static inline struct mot_aion_gt9764_device *to_mot_aion_gt9764_vcm(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct mot_aion_gt9764_device, ctrls);
}

static inline struct mot_aion_gt9764_device *sd_to_mot_aion_gt9764_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct mot_aion_gt9764_device, sd);
}

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};


static int mot_aion_gt9764_set_position(struct mot_aion_gt9764_device *mot_aion_gt9764, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&mot_aion_gt9764->sd);

	return i2c_smbus_write_word_data(client, MOT_AION_GT9764_SET_POSITION_ADDR,
					 swab16(val));
}

static int mot_aion_gt9764_release(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	int ret, val;
	struct i2c_client *client = v4l2_get_subdevdata(&mot_aion_gt9764->sd);

	for (val = round_down(mot_aion_gt9764->focus->val, MOT_AION_GT9764_MOVE_STEPS);
	     val >= 0; val -= MOT_AION_GT9764_MOVE_STEPS) {
		ret = mot_aion_gt9764_set_position(mot_aion_gt9764, val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
		usleep_range(MOT_AION_GT9764_MOVE_DELAY_US,
			     MOT_AION_GT9764_MOVE_DELAY_US + 500);
	}

	i2c_smbus_write_byte_data(client, 0x02, 0x20);

	/*
	 * Wait for the motor to stabilize after the last movement
	 * to prevent the motor from shaking.
	 */
	usleep_range(MOT_AION_GT9764_STABLE_TIME_US - MOT_AION_GT9764_MOVE_DELAY_US,
		     MOT_AION_GT9764_STABLE_TIME_US - MOT_AION_GT9764_MOVE_DELAY_US + 500);

	return 0;
}

static int mot_aion_gt9764_init(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	struct i2c_client *client = v4l2_get_subdevdata(&mot_aion_gt9764->sd);
	int ret = 0;
	char puSendCmdArray[3][2] = {
	{0x02, 0x02}, {0x06, 0x40}, {0x07, 0x05},
	};
	unsigned char cmd_number;

	LOG_INF("+\n");

	client->addr = MOT_AION_GT9764_I2C_SLAVE_ADDR >> 1;
	//ret = i2c_smbus_read_byte_data(client, 0x02);

	LOG_INF("Check HW version: %x\n", ret);

	for (cmd_number = 0; cmd_number < 3; cmd_number++) {
		if (puSendCmdArray[cmd_number][0] != 0xFE) {
			ret = i2c_smbus_write_byte_data(client,
					puSendCmdArray[cmd_number][0],
					puSendCmdArray[cmd_number][1]);

			if (ret < 0)
				return -1;
		} else {
			mdelay(1);
		}
	}

	LOG_INF("-\n");

	return ret;
}

/* Power handling */
static int mot_aion_gt9764_power_off(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = mot_aion_gt9764_release(mot_aion_gt9764);
	if (ret)
		LOG_INF("mot_aion_gt9764 release failed!\n");

	ret = regulator_disable(mot_aion_gt9764->vin);
	if (ret)
		return ret;

	ret = regulator_disable(mot_aion_gt9764->vdd);
	if (ret)
		return ret;

	if (mot_aion_gt9764->vcamaf_pinctrl && mot_aion_gt9764->vcamaf_off)
		ret = pinctrl_select_state(mot_aion_gt9764->vcamaf_pinctrl,
					mot_aion_gt9764->vcamaf_off);

	return ret;
}

static int mot_aion_gt9764_power_on(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = regulator_enable(mot_aion_gt9764->vin);
	if (ret < 0)
		return ret;

	ret = regulator_enable(mot_aion_gt9764->vdd);
	if (ret < 0)
		return ret;

	if (mot_aion_gt9764->vcamaf_pinctrl && mot_aion_gt9764->vcamaf_on)
		ret = pinctrl_select_state(mot_aion_gt9764->vcamaf_pinctrl,
					mot_aion_gt9764->vcamaf_on);

	if (ret < 0)
		return ret;

	/*
	 * TODO(b/139784289): Confirm hardware requirements and adjust/remove
	 * the delay.
	 */
	usleep_range(MOT_AION_GT9764_CTRL_DELAY_US, MOT_AION_GT9764_CTRL_DELAY_US + 100);

	ret = mot_aion_gt9764_init(mot_aion_gt9764);
	if (ret < 0)
		goto fail;

	return 0;

fail:
	regulator_disable(mot_aion_gt9764->vin);
	regulator_disable(mot_aion_gt9764->vdd);
	if (mot_aion_gt9764->vcamaf_pinctrl && mot_aion_gt9764->vcamaf_off) {
		pinctrl_select_state(mot_aion_gt9764->vcamaf_pinctrl,
				mot_aion_gt9764->vcamaf_off);
	}

	return ret;
}

static int mot_aion_gt9764_set_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct mot_aion_gt9764_device *mot_aion_gt9764 = to_mot_aion_gt9764_vcm(ctrl);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {
		LOG_INF("pos(%d)\n", ctrl->val);
		ret = mot_aion_gt9764_set_position(mot_aion_gt9764, ctrl->val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}

static const struct v4l2_ctrl_ops mot_aion_gt9764_vcm_ctrl_ops = {
	.s_ctrl = mot_aion_gt9764_set_ctrl,
};

static int mot_aion_gt9764_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = pm_runtime_get_sync(sd->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(sd->dev);
		return ret;
	}

	return 0;
}

static int mot_aion_gt9764_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	LOG_INF("%s\n", __func__);

	pm_runtime_put(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops mot_aion_gt9764_int_ops = {
	.open = mot_aion_gt9764_open,
	.close = mot_aion_gt9764_close,
};

static const struct v4l2_subdev_ops mot_aion_gt9764_ops = { };

static void mot_aion_gt9764_subdev_cleanup(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	v4l2_async_unregister_subdev(&mot_aion_gt9764->sd);
	v4l2_ctrl_handler_free(&mot_aion_gt9764->ctrls);
#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&mot_aion_gt9764->sd.entity);
#endif
}

static int mot_aion_gt9764_init_controls(struct mot_aion_gt9764_device *mot_aion_gt9764)
{
	struct v4l2_ctrl_handler *hdl = &mot_aion_gt9764->ctrls;
	const struct v4l2_ctrl_ops *ops = &mot_aion_gt9764_vcm_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	mot_aion_gt9764->focus = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, MOT_AION_GT9764_MAX_FOCUS_POS, MOT_AION_GT9764_FOCUS_STEPS, 0);

	if (hdl->error)
		return hdl->error;

	mot_aion_gt9764->sd.ctrl_handler = hdl;

	return 0;
}

static int mot_aion_gt9764_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct mot_aion_gt9764_device *mot_aion_gt9764;
	int ret;

	LOG_INF("%s\n", __func__);

	mot_aion_gt9764 = devm_kzalloc(dev, sizeof(*mot_aion_gt9764), GFP_KERNEL);
	if (!mot_aion_gt9764)
		return -ENOMEM;

	mot_aion_gt9764->vin = devm_regulator_get(dev, "vin");
	if (IS_ERR(mot_aion_gt9764->vin)) {
		ret = PTR_ERR(mot_aion_gt9764->vin);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vin regulator\n");
		return ret;
	}

	mot_aion_gt9764->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(mot_aion_gt9764->vdd)) {
		ret = PTR_ERR(mot_aion_gt9764->vdd);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vdd regulator\n");
		return ret;
	}

	mot_aion_gt9764->vcamaf_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(mot_aion_gt9764->vcamaf_pinctrl)) {
		ret = PTR_ERR(mot_aion_gt9764->vcamaf_pinctrl);
		mot_aion_gt9764->vcamaf_pinctrl = NULL;
		LOG_INF("cannot get pinctrl\n");
	} else {
		mot_aion_gt9764->vcamaf_on = pinctrl_lookup_state(
			mot_aion_gt9764->vcamaf_pinctrl, "vcamaf_on");

		if (IS_ERR(mot_aion_gt9764->vcamaf_on)) {
			ret = PTR_ERR(mot_aion_gt9764->vcamaf_on);
			mot_aion_gt9764->vcamaf_on = NULL;
			LOG_INF("cannot get vcamaf_on pinctrl\n");
		}

		mot_aion_gt9764->vcamaf_off = pinctrl_lookup_state(
			mot_aion_gt9764->vcamaf_pinctrl, "vcamaf_off");

		if (IS_ERR(mot_aion_gt9764->vcamaf_off)) {
			ret = PTR_ERR(mot_aion_gt9764->vcamaf_off);
			mot_aion_gt9764->vcamaf_off = NULL;
			LOG_INF("cannot get vcamaf_off pinctrl\n");
		}
	}

	v4l2_i2c_subdev_init(&mot_aion_gt9764->sd, client, &mot_aion_gt9764_ops);
	mot_aion_gt9764->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	mot_aion_gt9764->sd.internal_ops = &mot_aion_gt9764_int_ops;

	ret = mot_aion_gt9764_init_controls(mot_aion_gt9764);
	if (ret)
		goto err_cleanup;

#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	ret = media_entity_pads_init(&mot_aion_gt9764->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;

	mot_aion_gt9764->sd.entity.function = MEDIA_ENT_F_LENS;
#endif

	ret = v4l2_async_register_subdev(&mot_aion_gt9764->sd);
	if (ret < 0)
		goto err_cleanup;

	pm_runtime_enable(dev);

	return 0;

err_cleanup:
	mot_aion_gt9764_subdev_cleanup(mot_aion_gt9764);
	return ret;
}

static int mot_aion_gt9764_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_aion_gt9764_device *mot_aion_gt9764 = sd_to_mot_aion_gt9764_vcm(sd);

	LOG_INF("%s\n", __func__);

	mot_aion_gt9764_subdev_cleanup(mot_aion_gt9764);
	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		mot_aion_gt9764_power_off(mot_aion_gt9764);
	pm_runtime_set_suspended(&client->dev);

	return 0;
}

static int __maybe_unused mot_aion_gt9764_vcm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_aion_gt9764_device *mot_aion_gt9764 = sd_to_mot_aion_gt9764_vcm(sd);

	return mot_aion_gt9764_power_off(mot_aion_gt9764);
}

static int __maybe_unused mot_aion_gt9764_vcm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_aion_gt9764_device *mot_aion_gt9764 = sd_to_mot_aion_gt9764_vcm(sd);

	return mot_aion_gt9764_power_on(mot_aion_gt9764);
}

static const struct i2c_device_id mot_aion_gt9764_id_table[] = {
	{ MOT_AION_GT9764_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mot_aion_gt9764_id_table);

static const struct of_device_id mot_aion_gt9764_of_table[] = {
	{ .compatible = "mediatek,mot_aion_gt9764" },
	{ },
};
MODULE_DEVICE_TABLE(of, mot_aion_gt9764_of_table);

static const struct dev_pm_ops mot_aion_gt9764_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(mot_aion_gt9764_vcm_suspend, mot_aion_gt9764_vcm_resume, NULL)
};

static struct i2c_driver mot_aion_gt9764_i2c_driver = {
	.driver = {
		.name = MOT_AION_GT9764_NAME,
		.pm = &mot_aion_gt9764_pm_ops,
		.of_match_table = mot_aion_gt9764_of_table,
	},
	.probe_new  = mot_aion_gt9764_probe,
	.remove = mot_aion_gt9764_remove,
	.id_table = mot_aion_gt9764_id_table,
};

module_i2c_driver(mot_aion_gt9764_i2c_driver);

MODULE_AUTHOR("Po-Hao Huang <Po-Hao.Huang@mediatek.com>");
MODULE_DESCRIPTION("MOT_AION_GT9764 VCM driver");
MODULE_LICENSE("GPL v2");
