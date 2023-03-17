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

#define DRIVER_NAME "mot_manaus_dw9800v"
#define MOT_MANAUS_DW9800V_I2C_SLAVE_ADDR 0x18

#define LOG_INF(format, args...)                                               \
	pr_info(DRIVER_NAME " [%s] " format, __func__, ##args)

#define MOT_MANAUS_DW9800V_NAME				"mot_manaus_dw9800v"
#define MOT_MANAUS_DW9800V_MAX_FOCUS_POS			1023
/*
 * This sets the minimum granularity for the focus positions.
 * A value of 1 gives maximum accuracy for a desired focus position
 */
#define MOT_MANAUS_DW9800V_FOCUS_STEPS			1
#define MOT_MANAUS_DW9800V_SET_POSITION_ADDR		0x03

#define MOT_MANAUS_DW9800V_CMD_DELAY			0xff
#define MOT_MANAUS_DW9800V_CTRL_DELAY_US			1000
/*
 * This acts as the minimum granularity of lens movement.
 * Keep this value power of 2, so the control steps can be
 * uniformly adjusted for gradual lens movement, with desired
 * number of control steps.
 */
#define MOT_MANAUS_DW9800V_MOVE_STEPS			32
#define MOT_MANAUS_DW9800V_MOVE_DELAY_US			10000
#define MOT_MANAUS_DW9800V_STABLE_TIME_US			2000

/* mot_manaus_dw9800v device structure */
struct mot_manaus_dw9800v_device {
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_subdev sd;
	struct v4l2_ctrl *focus;
	struct regulator *vin;
	struct regulator *vdd;
	struct pinctrl *vcamaf_pinctrl;
	struct pinctrl_state *vcamaf_on;
	struct pinctrl_state *vcamaf_off;
};

static inline struct mot_manaus_dw9800v_device *to_mot_manaus_dw9800v_vcm(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct mot_manaus_dw9800v_device, ctrls);
}

static inline struct mot_manaus_dw9800v_device *sd_to_mot_manaus_dw9800v_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct mot_manaus_dw9800v_device, sd);
}

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};


static int mot_manaus_dw9800v_set_position(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&mot_manaus_dw9800v->sd);

	return i2c_smbus_write_word_data(client, MOT_MANAUS_DW9800V_SET_POSITION_ADDR,
					 swab16(val));
}

static int mot_manaus_dw9800v_release(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	int ret, val;

	struct i2c_client *client = v4l2_get_subdevdata(&mot_manaus_dw9800v->sd);
	for (val = round_down(mot_manaus_dw9800v->focus->val, MOT_MANAUS_DW9800V_MOVE_STEPS);
	     val >= 400; val -= MOT_MANAUS_DW9800V_MOVE_STEPS) {
		ret = mot_manaus_dw9800v_set_position(mot_manaus_dw9800v, val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
		usleep_range(MOT_MANAUS_DW9800V_MOVE_DELAY_US,
			     MOT_MANAUS_DW9800V_MOVE_DELAY_US + 500);
	}
	i2c_smbus_write_byte_data(client, 0x02, 0x20);

	/*
	 * Wait for the motor to stabilize after the last movement
	 * to prevent the motor from shaking.
	 */
	usleep_range(MOT_MANAUS_DW9800V_STABLE_TIME_US - MOT_MANAUS_DW9800V_MOVE_DELAY_US,
		     MOT_MANAUS_DW9800V_STABLE_TIME_US - MOT_MANAUS_DW9800V_MOVE_DELAY_US + 500);
	return 0;
}

static int mot_manaus_dw9800v_init(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	struct i2c_client *client = v4l2_get_subdevdata(&mot_manaus_dw9800v->sd);
	int ret = 0;
	char puSendCmdArray[6][2] = {
	{0x02, 0x01}, {0x02, 0x00}, {0xFE, 0xFE},
	{0x02, 0x02}, {0x06, 0x40}, {0x07, 0x7E},
	};
	unsigned char cmd_number;

	LOG_INF("+\n");

	client->addr = MOT_MANAUS_DW9800V_I2C_SLAVE_ADDR >> 1;
	//ret = i2c_smbus_read_byte_data(client, 0x02);

	LOG_INF("Check HW version: %x\n", ret);

	for (cmd_number = 0; cmd_number < 6; cmd_number++) {
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
static int mot_manaus_dw9800v_power_off(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	int ret;

	LOG_INF("%s +\n", __func__);

	ret = mot_manaus_dw9800v_release(mot_manaus_dw9800v);
	if (ret)
		LOG_INF("mot_manaus_dw9800v release failed!\n");

	ret = regulator_disable(mot_manaus_dw9800v->vin);
	if (ret)
		return ret;

	ret = regulator_disable(mot_manaus_dw9800v->vdd);
	if (ret)
		return ret;

	if (mot_manaus_dw9800v->vcamaf_pinctrl && mot_manaus_dw9800v->vcamaf_off)
		ret = pinctrl_select_state(mot_manaus_dw9800v->vcamaf_pinctrl,
					mot_manaus_dw9800v->vcamaf_off);
	LOG_INF("%s -\n", __func__);
	return ret;
}

static int mot_manaus_dw9800v_power_on(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	int ret;

	LOG_INF("%s\n", __func__);

	ret = regulator_enable(mot_manaus_dw9800v->vin);
	if (ret < 0)
		return ret;

	ret = regulator_enable(mot_manaus_dw9800v->vdd);
	if (ret < 0)
		return ret;

	if (mot_manaus_dw9800v->vcamaf_pinctrl && mot_manaus_dw9800v->vcamaf_on)
		ret = pinctrl_select_state(mot_manaus_dw9800v->vcamaf_pinctrl,
					mot_manaus_dw9800v->vcamaf_on);

	if (ret < 0)
		return ret;

	/*
	 * TODO(b/139784289): Confirm hardware requirements and adjust/remove
	 * the delay.
	 */
	usleep_range(MOT_MANAUS_DW9800V_CTRL_DELAY_US, MOT_MANAUS_DW9800V_CTRL_DELAY_US + 100);

	ret = mot_manaus_dw9800v_init(mot_manaus_dw9800v);
	if (ret < 0)
		goto fail;

	return 0;

fail:
	regulator_disable(mot_manaus_dw9800v->vin);
	regulator_disable(mot_manaus_dw9800v->vdd);
	if (mot_manaus_dw9800v->vcamaf_pinctrl && mot_manaus_dw9800v->vcamaf_off) {
		pinctrl_select_state(mot_manaus_dw9800v->vcamaf_pinctrl,
				mot_manaus_dw9800v->vcamaf_off);
	}

	return ret;
}

static int mot_manaus_dw9800v_set_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct mot_manaus_dw9800v_device *mot_manaus_dw9800v = to_mot_manaus_dw9800v_vcm(ctrl);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {
		LOG_INF("pos(%d)\n", ctrl->val);
		ret = mot_manaus_dw9800v_set_position(mot_manaus_dw9800v, ctrl->val);
		if (ret) {
			LOG_INF("%s I2C failure: %d",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}

static const struct v4l2_ctrl_ops mot_manaus_dw9800v_vcm_ctrl_ops = {
	.s_ctrl = mot_manaus_dw9800v_set_ctrl,
};

static int mot_manaus_dw9800v_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
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

static int mot_manaus_dw9800v_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	LOG_INF("%s\n", __func__);

	pm_runtime_put(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops mot_manaus_dw9800v_int_ops = {
	.open = mot_manaus_dw9800v_open,
	.close = mot_manaus_dw9800v_close,
};

static const struct v4l2_subdev_ops mot_manaus_dw9800v_ops = { };

static void mot_manaus_dw9800v_subdev_cleanup(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	v4l2_async_unregister_subdev(&mot_manaus_dw9800v->sd);
	v4l2_ctrl_handler_free(&mot_manaus_dw9800v->ctrls);
#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&mot_manaus_dw9800v->sd.entity);
#endif
}

static int mot_manaus_dw9800v_init_controls(struct mot_manaus_dw9800v_device *mot_manaus_dw9800v)
{
	struct v4l2_ctrl_handler *hdl = &mot_manaus_dw9800v->ctrls;
	const struct v4l2_ctrl_ops *ops = &mot_manaus_dw9800v_vcm_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	mot_manaus_dw9800v->focus = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, MOT_MANAUS_DW9800V_MAX_FOCUS_POS, MOT_MANAUS_DW9800V_FOCUS_STEPS, 0);

	if (hdl->error)
		return hdl->error;

	mot_manaus_dw9800v->sd.ctrl_handler = hdl;

	return 0;
}

static int mot_manaus_dw9800v_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct mot_manaus_dw9800v_device *mot_manaus_dw9800v;
	int ret;

	LOG_INF("%s\n", __func__);

	mot_manaus_dw9800v = devm_kzalloc(dev, sizeof(*mot_manaus_dw9800v), GFP_KERNEL);
	if (!mot_manaus_dw9800v)
		return -ENOMEM;

	mot_manaus_dw9800v->vin = devm_regulator_get(dev, "vin");
	if (IS_ERR(mot_manaus_dw9800v->vin)) {
		ret = PTR_ERR(mot_manaus_dw9800v->vin);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vin regulator\n");
		return ret;
	}

	mot_manaus_dw9800v->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(mot_manaus_dw9800v->vdd)) {
		ret = PTR_ERR(mot_manaus_dw9800v->vdd);
		if (ret != -EPROBE_DEFER)
			LOG_INF("cannot get vdd regulator\n");
		return ret;
	}

	mot_manaus_dw9800v->vcamaf_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(mot_manaus_dw9800v->vcamaf_pinctrl)) {
		ret = PTR_ERR(mot_manaus_dw9800v->vcamaf_pinctrl);
		mot_manaus_dw9800v->vcamaf_pinctrl = NULL;
		LOG_INF("cannot get pinctrl\n");
	} else {
		mot_manaus_dw9800v->vcamaf_on = pinctrl_lookup_state(
			mot_manaus_dw9800v->vcamaf_pinctrl, "vcamaf_on");

		if (IS_ERR(mot_manaus_dw9800v->vcamaf_on)) {
			ret = PTR_ERR(mot_manaus_dw9800v->vcamaf_on);
			mot_manaus_dw9800v->vcamaf_on = NULL;
			LOG_INF("cannot get vcamaf_on pinctrl\n");
		}

		mot_manaus_dw9800v->vcamaf_off = pinctrl_lookup_state(
			mot_manaus_dw9800v->vcamaf_pinctrl, "vcamaf_off");

		if (IS_ERR(mot_manaus_dw9800v->vcamaf_off)) {
			ret = PTR_ERR(mot_manaus_dw9800v->vcamaf_off);
			mot_manaus_dw9800v->vcamaf_off = NULL;
			LOG_INF("cannot get vcamaf_off pinctrl\n");
		}
	}

	v4l2_i2c_subdev_init(&mot_manaus_dw9800v->sd, client, &mot_manaus_dw9800v_ops);
	mot_manaus_dw9800v->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	mot_manaus_dw9800v->sd.internal_ops = &mot_manaus_dw9800v_int_ops;

	ret = mot_manaus_dw9800v_init_controls(mot_manaus_dw9800v);
	if (ret)
		goto err_cleanup;

#if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	ret = media_entity_pads_init(&mot_manaus_dw9800v->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;

	mot_manaus_dw9800v->sd.entity.function = MEDIA_ENT_F_LENS;
#endif

	ret = v4l2_async_register_subdev(&mot_manaus_dw9800v->sd);
	if (ret < 0)
		goto err_cleanup;

	pm_runtime_enable(dev);

	return 0;

err_cleanup:
	mot_manaus_dw9800v_subdev_cleanup(mot_manaus_dw9800v);
	return ret;
}

static int mot_manaus_dw9800v_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_manaus_dw9800v_device *mot_manaus_dw9800v = sd_to_mot_manaus_dw9800v_vcm(sd);

	LOG_INF("%s\n", __func__);

	mot_manaus_dw9800v_subdev_cleanup(mot_manaus_dw9800v);
	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		mot_manaus_dw9800v_power_off(mot_manaus_dw9800v);
	pm_runtime_set_suspended(&client->dev);

	return 0;
}

static int __maybe_unused mot_manaus_dw9800v_vcm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_manaus_dw9800v_device *mot_manaus_dw9800v = sd_to_mot_manaus_dw9800v_vcm(sd);

	return mot_manaus_dw9800v_power_off(mot_manaus_dw9800v);
}

static int __maybe_unused mot_manaus_dw9800v_vcm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct mot_manaus_dw9800v_device *mot_manaus_dw9800v = sd_to_mot_manaus_dw9800v_vcm(sd);

	return mot_manaus_dw9800v_power_on(mot_manaus_dw9800v);
}

static const struct i2c_device_id mot_manaus_dw9800v_id_table[] = {
	{ MOT_MANAUS_DW9800V_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mot_manaus_dw9800v_id_table);

static const struct of_device_id mot_manaus_dw9800v_of_table[] = {
	{ .compatible = "mediatek,mot_manaus_dw9800v" },
	{ },
};
MODULE_DEVICE_TABLE(of, mot_manaus_dw9800v_of_table);

static const struct dev_pm_ops mot_manaus_dw9800v_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(mot_manaus_dw9800v_vcm_suspend, mot_manaus_dw9800v_vcm_resume, NULL)
};

static struct i2c_driver mot_manaus_dw9800v_i2c_driver = {
	.driver = {
		.name = MOT_MANAUS_DW9800V_NAME,
		.pm = &mot_manaus_dw9800v_pm_ops,
		.of_match_table = mot_manaus_dw9800v_of_table,
	},
	.probe_new  = mot_manaus_dw9800v_probe,
	.remove = mot_manaus_dw9800v_remove,
	.id_table = mot_manaus_dw9800v_id_table,
};

module_i2c_driver(mot_manaus_dw9800v_i2c_driver);

MODULE_AUTHOR("Po-Hao Huang <Po-Hao.Huang@mediatek.com>");
MODULE_DESCRIPTION("MOT_MANAUS_DW9800V VCM driver");
MODULE_LICENSE("GPL v2");
