/*
* aw99703.c   aw99703 backlight module
*
* Copyright (c) 2019 AWINIC Technology CO., LTD
*
*  Author: Joseph <zhangzetao@awinic.com.cn>
*
* This program is free software; you can redistribute  it and/or modify it
* under  the terms of  the GNU General  Public License as published by the
* Free Software Foundation;  either version 2 of the  License, or (at your
* option) any later version.
*/

#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/leds_aw99703.h>

#define AW99703_LED_DEV "aw99703-bl"
#define AW99703_NAME "aw99703-bl"

#define AW99703_DRIVER_VERSION "v1.0.5"
#define AW_I2C_RETRIES		5
#define AW_I2C_RETRY_DELAY	2

struct aw99703_data *g_aw99703_data;
static unsigned char chipid;

static int platform_read_i2c_block(struct i2c_client *client, char *writebuf,
	int writelen, char *readbuf, int readlen)
{
	int ret;
	unsigned char cnt = 0;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = 0,
				 .len = writelen,
				 .buf = writebuf,
			 },
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};

		while (cnt < AW_I2C_RETRIES) {
			ret = i2c_transfer(client->adapter, msgs, 2);
			if (ret < 0)
				dev_err(&client->dev, "%s: i2c read error\n",
								__func__);
			else
				break;

			cnt++;
			mdelay(AW_I2C_RETRY_DELAY);
		}
	} else {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		while (cnt < AW_I2C_RETRIES) {
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0)
				dev_err(&client->dev, "%s:i2c read error.\n",
								__func__);
			else
				break;

			cnt++;
			mdelay(AW_I2C_RETRY_DELAY);
		}
	}
	return ret;
}

static int aw99703_i2c_read(struct i2c_client *client, u8 addr, u8 *val)
{
	return platform_read_i2c_block(client, &addr, 1, val, 1);
}

static int platform_write_i2c_block(struct i2c_client *client,
		char *writebuf, int writelen)
{
	int ret;
	unsigned char cnt = 0;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c write error.\n",
								__func__);
		else
			break;

		cnt++;
		mdelay(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static int aw99703_i2c_write(struct i2c_client *client, u8 addr, const u8 val)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = val;

	return platform_write_i2c_block(client, buf, sizeof(buf));
}

static void aw99703_hwen_pin_ctrl(struct aw99703_data *drvdata, int en)
{
	if (gpio_is_valid(drvdata->hwen_gpio)) {
		if (en) {
			pr_info("hwen pin is going to be high!---<%d>\n", en);
			gpio_set_value(drvdata->hwen_gpio, true);
			usleep_range(3500, 4000);
		} else {
			pr_info("hwen pin is going to be low!---<%d>\n", en);
			gpio_set_value(drvdata->hwen_gpio, false);
			usleep_range(1000, 2000);
		}
	}
}

static int aw99703_gpio_init(struct aw99703_data *drvdata)
{

	int ret;

	if (gpio_is_valid(drvdata->hwen_gpio)) {
		ret = gpio_request(drvdata->hwen_gpio, "hwen_gpio");
		if (ret < 0) {
			pr_err("failed to request gpio\n");
			return -1;
		}
		ret = gpio_direction_output(drvdata->hwen_gpio, 0);
		pr_info(" request gpio init\n");
		if (ret < 0) {
			pr_err("failed to set output");
			gpio_free(drvdata->hwen_gpio);
			return ret;
		}
		pr_info("gpio is valid!\n");
		aw99703_hwen_pin_ctrl(drvdata, 1);
	}

	return 0;
}

static int aw99703_i2c_write_bit(struct i2c_client *client,
	unsigned int reg_addr, unsigned int  mask, unsigned char reg_data)
{
	unsigned char reg_val = 0;

	aw99703_i2c_read(client, reg_addr, &reg_val);
	reg_val &= mask;
	reg_val |= reg_data;
	aw99703_i2c_write(client, reg_addr, reg_val);

	return 0;
}

static int aw99703_brightness_map(unsigned int level)
{
	/* MAX_LEVEL_256 */
	if (g_aw99703_data->bl_map == 1) {
		if (level == 255)
			return 2047;
		return level * 8;
	}
	/* MAX_LEVEL_1024 */
	if (g_aw99703_data->bl_map == 2)
		return level * 2;
	/* MAX_LEVEL_2048 */
	if (g_aw99703_data->bl_map == 3)
		return level;

	return  level;
}

static int aw99703_bl_enable_channel(struct aw99703_data *drvdata)
{
	int ret = 0;

	if (drvdata->channel == 3) {
		pr_info("%s turn all channel on!\n", __func__);
		ret = aw99703_i2c_write_bit(drvdata->client,
						AW99703_REG_LEDCUR,
						AW99703_LEDCUR_CHANNEL_MASK,
						AW99703_LEDCUR_CH3_ENABLE |
						AW99703_LEDCUR_CH2_ENABLE |
						AW99703_LEDCUR_CH1_ENABLE);
	} else if (drvdata->channel == 2) {
		pr_info("%s turn two channel on!\n", __func__);
		ret = aw99703_i2c_write_bit(drvdata->client,
						AW99703_REG_LEDCUR,
						AW99703_LEDCUR_CHANNEL_MASK,
						AW99703_LEDCUR_CH2_ENABLE |
						AW99703_LEDCUR_CH1_ENABLE);
	} else if (drvdata->channel == 1) {
		pr_info("%s turn one channel on!\n", __func__);
		ret = aw99703_i2c_write_bit(drvdata->client,
						AW99703_REG_LEDCUR,
						AW99703_LEDCUR_CHANNEL_MASK,
						AW99703_LEDCUR_CH1_ENABLE);
	} else {
		pr_info("%s all channels are going to be disabled\n", __func__);
		ret = aw99703_i2c_write_bit(drvdata->client,
						AW99703_REG_LEDCUR,
						AW99703_LEDCUR_CHANNEL_MASK,
						0x98);
	}

	return ret;
}

static void aw99703_pwm_mode_enable(struct aw99703_data *drvdata)
{
	if (drvdata->pwm_mode) {
		aw99703_i2c_write_bit(drvdata->client,
					AW99703_REG_MODE,
					AW99703_MODE_PDIS_MASK,
					AW99703_MODE_PDIS_DISABLE);
		pr_info("%s pwm_mode is disable\n", __func__);
	} else {
		aw99703_i2c_write_bit(drvdata->client,
					AW99703_REG_MODE,
					AW99703_MODE_PDIS_MASK,
					AW99703_MODE_PDIS_ENABLE);
		pr_info("%s pwm_mode is enable\n", __func__);
	}
}

static void aw99703_ramp_setting(struct aw99703_data *drvdata)
{
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_TURNCFG,
				AW99703_TURNCFG_ON_TIM_MASK,
				drvdata->ramp_on_time << 4);
	pr_info("%s drvdata->ramp_on_time is 0x%x\n",
		__func__, drvdata->ramp_on_time);

	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_TURNCFG,
				AW99703_TURNCFG_OFF_TIM_MASK,
				drvdata->ramp_off_time);
	pr_info("%s drvdata->ramp_off_time is 0x%x\n",
		__func__, drvdata->ramp_off_time);

}

static void aw99703_transition_ramp(struct aw99703_data *drvdata)
{

	pr_info("%s enter\n", __func__);
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_TRANCFG,
				AW99703_TRANCFG_PWM_TIM_MASK,
				drvdata->pwm_trans_dim);
	pr_info("%s drvdata->pwm_trans_dim is 0x%x\n", __func__,
		drvdata->pwm_trans_dim);

	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_TRANCFG,
				AW99703_TRANCFG_I2C_TIM_MASK,
				drvdata->i2c_trans_dim);
	pr_info("%s drvdata->i2c_trans_dim is 0x%x\n",
		__func__, drvdata->i2c_trans_dim);

}

static void aw99703_ovp_level_setting(struct aw99703_data *drvdata)
{
	int ovp_level;

	pr_info("%s enter\n", __func__);

	switch (drvdata->ovp_level) {
	case 0:
		ovp_level = AW99703_BSTCTR1_OVPSEL_17P5V;
		break;
	case 1:
		ovp_level = AW99703_BSTCTR1_OVPSEL_24V;
		break;
	case 2:
		ovp_level = AW99703_BSTCTR1_OVPSEL_31V;
		break;
	case 3:
		ovp_level = AW99703_BSTCTR1_OVPSEL_38V;
		break;
	default:
		ovp_level = AW99703_BSTCTR1_OVPSEL_41P5V;
		break;
	}
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OVPSEL_MASK,
				ovp_level);
	pr_info("%s drvdata->ovp_level is 0x%x\n", __func__,
		drvdata->ovp_level);

}

static void aw99703_ocp_level_setting(struct aw99703_data *drvdata)
{
	pr_info("%s enter\n", __func__);
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OCPSEL_MASK,
				drvdata->ocp_level);
	pr_info("%s drvdata->ocp_level is 0x%x\n", __func__,
		drvdata->ocp_level);
}

static void aw99703_switching_freq_setting(struct aw99703_data *drvdata)
{
	int frequency;

	pr_info("%s enter\n", __func__);
	if (drvdata->frequency)
		frequency = AW99703_BSTCTR1_SF_1000KHZ;
	else
		frequency = AW99703_BSTCTR1_SF_500KHZ;

	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_SF_MASK,
				frequency);
	pr_info("%s drvdata->frequency is 0x%x\n", __func__,
		drvdata->frequency);
}

static void aw99703_bl_full_scale_setting(struct aw99703_data *drvdata)
{
	pr_info("%s enter.\n", __func__);
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_LEDCUR,
				AW99703_LEDCUR_BLFS_MASK,
				drvdata->full_scale_led << 3);
	pr_info("%s drvdata->full_scale_led is 0x%x\n", __func__,
		drvdata->full_scale_led);
}

static void aw99703_set_work_mode(struct aw99703_data *drvdata, int work_mode)
{
	int mode;

	pr_info("%s enter.\n", __func__);
	switch (work_mode) {
	case 0:
		mode = AW99703_MODE_WORKMODE_STANDBY;
		break;
	case 1:
		mode = AW99703_MODE_WORKMODE_BACKLIGHT;
		break;
	default:
		mode = AW99703_MODE_WORKMODE_FLASH;
		break;
	}
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_MODE,
				AW99703_MODE_WORKMODE_MASK,
				mode);
	pr_info("%s drvdata->work_mode is 0x%x\n", __func__,
		work_mode);
}

static void aw99703_map_type_setting(struct aw99703_data *drvdata)
{
	int map_type;

	pr_info("%s enter.\n", __func__);
	if (drvdata->map_type)
		map_type = AW99703_MODE_MAP_LINEAR;
	else
		map_type = AW99703_MODE_MAP_EXPONENTIAL;

	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_MODE,
				AW99703_MODE_MAP_MASK,
				map_type);
	pr_info("%s drvdata->map_type is 0x%x\n", __func__,
		drvdata->map_type);
}

static void aw99703_flash_timeout_setting(struct aw99703_data *drvdata)
{
	pr_info("%s enter.\n", __func__);
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_FLASH,
				AW99703_FLASH_FLTO_TIM_MASK,
				drvdata->flash_timeout << 4);
	pr_info("%s drvdata->flash_timeout is 0x%x\n", __func__,
		drvdata->flash_timeout);
}

static void aw99703_flash_current_setting(struct aw99703_data *drvdata)
{
	pr_info("%s enter.\n", __func__);
	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_FLASH,
				AW99703_FLASH_FL_S_MASK,
				drvdata->flash_current);
	pr_info("%s drvdata->flash_current is 0x%x\n", __func__,
		drvdata->flash_current);
}

static void aw99703_max_brightness_setting(struct aw99703_data *drvdata)
{
	pr_info("%s enter.\n", __func__);
	aw99703_i2c_write(drvdata->client,
				AW99703_REG_LEDLSB,
				MAX_BRIGHTNESS & 0x07);
	pr_info("%s led lsb is 0x%x\n", __func__,
		MAX_BRIGHTNESS & 0x07);
	aw99703_i2c_write(drvdata->client,
				AW99703_REG_LEDMSB,
				(MAX_BRIGHTNESS >> 3) & 0xff);
	pr_info("%s led msb is 0x%x\n", __func__,
		(MAX_BRIGHTNESS >> 3) & 0xff);
}

static void aw99703_parse_efuse_dt_data(struct device *dev,
	struct aw99703_data *drvdata)
{
	int rc, i;
	struct device_node *np = dev->of_node;

	rc = of_property_read_u32_array(np, "aw99703,efuse-config",
					drvdata->efuse_config,
					ARRAY_SIZE(drvdata->efuse_config));
	if (rc != 0)
		pr_err("%s efuse config not found!\n", __func__);
	else
		for (i = 0; i < ARRAY_SIZE(drvdata->efuse_config); i++)
			pr_info("%s efuse_config[%d]=%d\n", __func__, i,
				drvdata->efuse_config[i]);
}

static int aw99703_backlight_init(struct aw99703_data *drvdata)
{
	u8 value = 0;

	pr_info("%s enter.\n", __func__);

	aw99703_i2c_read(g_aw99703_data->client, 0x0e, &value);
	aw99703_i2c_read(g_aw99703_data->client, 0x0f, &value);

	/* Optimize chip PFM performance. */
	aw99703_i2c_write(drvdata->client, AW99703_REG_WPRT1,
			drvdata->efuse_config[0]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_BSTCTR3,
			drvdata->efuse_config[1]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_BSTCTR5,
			drvdata->efuse_config[2]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_BSTCTR4,
			drvdata->efuse_config[3]);

	/* The startup process enters AUTO_FREQUENCE. */
	aw99703_switching_freq_setting(drvdata);
	aw99703_ovp_level_setting(drvdata);
	aw99703_ocp_level_setting(drvdata);

	aw99703_i2c_write(drvdata->client, AW99703_REG_AFHIGH,
				drvdata->auto_freq_high);
	pr_info("%s drvdata->auto_freq_high is 0x%x\n", __func__,
		drvdata->auto_freq_high);
	aw99703_i2c_write(drvdata->client, AW99703_REG_AFLOW,
				drvdata->auto_freq_low);
	pr_info("%s drvdata->auto_freq_low is 0x%x\n", __func__,
		drvdata->auto_freq_low);

	aw99703_bl_full_scale_setting(drvdata);
	aw99703_bl_enable_channel(drvdata);

	aw99703_ramp_setting(drvdata);
	aw99703_transition_ramp(drvdata);

	aw99703_i2c_write(drvdata->client, AW99703_REG_WPRT2,
			drvdata->efuse_config[4]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_TEST,
			drvdata->efuse_config[5]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_FLASH,
			drvdata->efuse_config[6]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_LEDLSB,
			drvdata->efuse_config[7]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_LEDMSB,
			drvdata->efuse_config[8]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_BSTCTR2,
			drvdata->efuse_config[9]);

	//aw99703_pwm_mode_enable(drvdata);
	//aw99703_set_work_mode(drvdata, drvdata->work_mode);
	//aw99703_map_type_setting(drvdata);
	aw99703_i2c_write(drvdata->client, AW99703_REG_MODE,
			0x15);
	mdelay(6);

	/* Exit AUTO_FREQUENCE when working normally. */
	aw99703_i2c_write(drvdata->client, AW99703_REG_TEST,
			drvdata->efuse_config[10]);
	aw99703_flash_timeout_setting(drvdata);
	aw99703_flash_current_setting(drvdata);
	aw99703_i2c_write(drvdata->client, AW99703_REG_WPRT2,
			drvdata->efuse_config[11]);
	aw99703_i2c_write(drvdata->client, AW99703_REG_BSTCTR2,
			drvdata->efuse_config[12]);

	//aw99703_max_brightness_setting(drvdata);

	return 0;
}

static int aw99703_backlight_enable(struct aw99703_data *drvdata)
{
	pr_info("%s enter.\n", __func__);

	aw99703_i2c_write_bit(drvdata->client,
				AW99703_REG_MODE,
				AW99703_MODE_WORKMODE_MASK,
				AW99703_MODE_WORKMODE_BACKLIGHT);

	drvdata->enable = true;

	return 0;
}

int aw99703_set_brightness(struct aw99703_data *drvdata, int brt_val)
{
	pr_info("%s brt_val is %d\n", __func__, brt_val);

	if (drvdata->enable == false) {
		aw99703_backlight_init(drvdata);
		aw99703_backlight_enable(drvdata);
	}

	brt_val = aw99703_brightness_map(brt_val);

	if (brt_val > 0) {
		/* enalbe bl mode */
		/* set backlight brt_val */
		aw99703_i2c_write(drvdata->client,
				AW99703_REG_LEDLSB,
				brt_val&0x0007);
		aw99703_i2c_write(drvdata->client,
				AW99703_REG_LEDMSB,
				(brt_val >> 3)&0xff);

		/* backlight enable */
		aw99703_i2c_write_bit(drvdata->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_BACKLIGHT);
	} else {
		/* standby mode */
		aw99703_i2c_write_bit(drvdata->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_STANDBY);
	}

	drvdata->brightness = brt_val;

	if (drvdata->brightness == 0)
		drvdata->enable = false;
	return 0;
}

#ifdef KERNEL_ABOVE_4_14
static int aw99703_bl_get_brightness(struct backlight_device *bl_dev)
{
		return bl_dev->props.brightness;
}

static int aw99703_bl_update_status(struct backlight_device *bl_dev)
{
		struct aw99703_data *drvdata = bl_get_data(bl_dev);
		int brt;

		if (bl_dev->props.state & (BL_CORE_SUSPENDED << 1))
				bl_dev->props.brightness = 0;

		brt = bl_dev->props.brightness;
		/*
		 * Brightness register should always be written
		 * not only register based mode but also in PWM mode.
		 */
		return aw99703_set_brightness(drvdata, brt);
}

static const struct backlight_ops aw99703_bl_ops = {
		//.update_status = aw99703_bl_update_status,
		.get_brightness = aw99703_bl_get_brightness,
};
#endif

static int aw99703_read_chipid(struct aw99703_data *drvdata)
{
	int ret = -1;
	u8 value = 0;
	unsigned char cnt = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret = aw99703_i2c_read(drvdata->client, 0x00, &value);
		if (ret < 0) {
			pr_err("%s: failed to read reg AW99703_REG_ID: %d\n",
				__func__, ret);
		}
		switch (value) {
		case 0x03:
			pr_info("%s aw99703 detected\n", __func__);
			chipid = value;
			return 0;
		default:
			pr_info("%s unsupported device revision (0x%x)\n",
				__func__, value);
			break;
		}
		cnt++;

		msleep(AW_READ_CHIPID_RETRY_DELAY);
	}

	return -EINVAL;
}

int is_aw99703_chip_exist(void)
{
	if (chipid == 0x3)
		return 1;
	else
		return 0;
}

int chargepump_set_backlight_level(unsigned int level)
{

	pr_info("%s aw99703_brt_level is %d\n", __func__, level);
	if(g_aw99703_data == NULL) {
		pr_err("aw99703 driver load error.\n");
		return 0;
	}
	if (g_aw99703_data->enable == false) {
		aw99703_backlight_init(g_aw99703_data);
		aw99703_backlight_enable(g_aw99703_data);
	}

	level = aw99703_brightness_map(level);

	if (level > 0) {
		/* enalbe bl mode */
		/* set backlight level */
		aw99703_i2c_write(g_aw99703_data->client,
				AW99703_REG_LEDLSB,
				level & 0x0007);
		aw99703_i2c_write(g_aw99703_data->client,
				AW99703_REG_LEDMSB,
				(level >> 3) & 0xff);

		/* backlight enable */
		aw99703_i2c_write_bit(g_aw99703_data->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_BACKLIGHT);
	} else {
		/* standby mode */
		aw99703_i2c_write_bit(g_aw99703_data->client,
					AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_STANDBY);
	}

	g_aw99703_data->brightness = level;

	if (g_aw99703_data->brightness == 0)
		g_aw99703_data->enable = false;

	return 0;
}

static void __aw99703_work(struct aw99703_data *led,
				enum led_brightness value)
{
	mutex_lock(&led->lock);
	aw99703_set_brightness(led, value);
	mutex_unlock(&led->lock);
}

static void aw99703_work(struct work_struct *work)
{
	struct aw99703_data *drvdata = container_of(work,
					struct aw99703_data, work);

	__aw99703_work(drvdata, drvdata->led_dev.brightness);
}


static void aw99703_brightness_set(struct led_classdev *led_cdev,
			enum led_brightness brt_val)
{
	struct aw99703_data *drvdata;

	drvdata = container_of(led_cdev, struct aw99703_data, led_dev);
	schedule_work(&drvdata->work);
}

static void aw99703_get_dt_data(struct device *dev,
	struct aw99703_data *drvdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	u32 bl_channel, temp;

	drvdata->hwen_gpio = of_get_named_gpio(np, "aw99703,hwen-gpio", 0);
	pr_info("%s drvdata->hwen_gpio --<%d>\n", __func__, drvdata->hwen_gpio);
	drvdata->hwen_gpio = 22;
	rc = of_property_read_u32(np, "aw99703,bl-map", &drvdata->bl_map);
	if (rc != 0)
		pr_err("%s bl_map not found!\n", __func__);
	else
		pr_info("%s bl_map=%d\n", __func__, drvdata->bl_map);

	drvdata->using_lsb = of_property_read_bool(np, "aw99703,using-lsb");
	pr_info("%s using_lsb --<%d>\n", __func__, drvdata->using_lsb);

	if (drvdata->using_lsb) {
		drvdata->default_brightness = 0x7ff;
		drvdata->max_brightness = 2047;
	} else {
		drvdata->default_brightness = 0xff;
		drvdata->max_brightness = 255;
	}

	rc = of_property_read_u32(np, "aw99703,ovp-level", &temp);
	if (rc) {
		pr_err("Invalid backlight over voltage protect level!\n");
	} else {
		drvdata->ovp_level = temp;
		pr_info("%s over voltage protect level --<%d >\n",
			__func__, drvdata->ovp_level);
	}

	rc = of_property_read_u32(np, "aw99703,ocp-level", &temp);
	if (rc) {
		pr_err("Invalid backlight over current protect level!\n");
	} else {
		drvdata->ocp_level = temp;
		pr_info("%s over current protect level --<%d >\n",
			__func__, drvdata->ocp_level);
	}

	rc = of_property_read_u32(np, "aw99703,switch-freq", &temp);
	if (rc) {
		pr_err("Invalid backlight switching frequency!\n");
	} else {
		drvdata->frequency = temp;
		pr_info("%s switching frequency  --<%d >\n",
			__func__, drvdata->frequency);
	}

	rc = of_property_read_u32(np, "aw99703,auto-freq-high", &temp);
	if (rc) {
		pr_err("Invalid backlight auto frequency high threshold!\n");
	} else {
		drvdata->auto_freq_high = temp;
		pr_info("%s auto frequency high threshold value  --<%d >\n",
			__func__, drvdata->auto_freq_high);
	}

	rc = of_property_read_u32(np, "aw99703,auto-freq-low", &temp);
	if (rc) {
		pr_err("Invalid backlight auto frequency low threshold!\n");
	} else {
		drvdata->auto_freq_low = temp;
		pr_info("%s auto frequency low threshold value  --<%d >\n",
			__func__, drvdata->auto_freq_low);
	}

	rc = of_property_read_u32(np, "aw99703,bl-fscal-led", &temp);
	if (rc) {
		pr_err("Invalid backlight full-scale led current!\n");
	} else {
		drvdata->full_scale_led = temp;
		pr_info("%s full-scale led current --<%d mA>\n",
			__func__, drvdata->full_scale_led);
	}

	rc = of_property_read_u32(np, "aw99703,bl-channel", &bl_channel);
	if (rc) {
		pr_err("Invalid channel setup!\n");
	} else {
		drvdata->channel = bl_channel;
		pr_info("%s bl-channel --<%x>\n", __func__, drvdata->channel);
	}

	rc = of_property_read_u32(np, "aw99703,turn-on-ramp", &temp);
	if (rc) {
		pr_err("Invalid ramp timing, turnon!\n");
	} else {
		drvdata->ramp_on_time = temp;
		pr_info("%s ramp on time --<%d ms>\n",
			__func__, drvdata->ramp_on_time);
	}

	rc = of_property_read_u32(np, "aw99703,turn-off-ramp", &temp);
	if (rc) {
		pr_err("Invalid ramp timing, turnoff!\n");
	} else {
		drvdata->ramp_off_time = temp;
		pr_info("%s ramp off time --<%d ms>\n",
			__func__, drvdata->ramp_off_time);
	}

	rc = of_property_read_u32(np, "aw99703,pwm-trans-dim", &temp);
	if (rc) {
		pr_err("Invalid pwm-tarns-dim value!\n");
	} else {
		drvdata->pwm_trans_dim = temp;
		pr_info("%s pwm trnasition dimming	--<%d ms>\n",
			__func__, drvdata->pwm_trans_dim);
	}

	rc = of_property_read_u32(np, "aw99703,i2c-trans-dim", &temp);
	if (rc) {
		pr_err("Invalid i2c-trans-dim value!\n");
	} else {
		drvdata->i2c_trans_dim = temp;
		pr_info("%s i2c transition dimming --<%d ms>\n",
			__func__, drvdata->i2c_trans_dim);
	}

	rc = of_property_read_u32(np, "aw99703,pwm-mode", &drvdata->pwm_mode);
	if (rc != 0)
		pr_err("%s pwm-mode not found!\n", __func__);
	else
		pr_info("%s pwm_mode=%d\n", __func__, drvdata->pwm_mode);

	rc = of_property_read_u32(np, "aw99703,map-type", &temp);
	if (rc) {
		pr_err("Invalid map type!\n");
	} else {
		drvdata->map_type = temp;
		pr_info("%s map type  --<%x>\n", __func__, drvdata->map_type);
	}

	rc = of_property_read_u32(np, "aw99703,work-mode", &temp);
	if (rc) {
		pr_err("Invalid work mode!\n");
	} else {
		drvdata->work_mode = temp;
		pr_info("%s work mode  --<%x>\n", __func__, drvdata->work_mode);
	}

	rc = of_property_read_u32(np, "aw99703,flash-timeout-time", &temp);
	if (rc) {
		pr_err("Invalid flash timeout time!\n");
	} else {
		drvdata->flash_timeout = temp;
		pr_info("%s flash timeout time --<%x>\n", __func__,
			drvdata->flash_timeout);
	}

	rc = of_property_read_u32(np, "aw99703,flash-current", &temp);
	if (rc) {
		pr_err("Invalid flash current!\n");
	} else {
		drvdata->flash_current = temp;
		pr_info("%s  flash current --<%x>\n", __func__,
			drvdata->flash_current);
	}
	aw99703_parse_efuse_dt_data(dev, drvdata);
}

/******************************************************
 *
 * sys group attribute: reg
 *
 ******************************************************/
static ssize_t aw99703_i2c_reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw99703_data *aw99703 = dev_get_drvdata(dev);

	unsigned int databuf[2] = {0, 0};

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		aw99703_i2c_write(aw99703->client,
				(unsigned char)databuf[0],
				(unsigned char)databuf[1]);
	}

	return count;
}

static ssize_t aw99703_i2c_reg_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct aw99703_data *aw99703 = dev_get_drvdata(dev);
	ssize_t len = 0;
	unsigned char i = 0;
	unsigned char reg_val = 0;

	for (i = 0; i < AW99703_REG_MAX; i++) {
		if (!(aw99703_reg_access[i]&REG_RD_ACCESS))
			continue;
		aw99703_i2c_read(aw99703->client, i, &reg_val);
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%02x=0x%02x\n",
				i, reg_val);
	}

	return len;
}

static DEVICE_ATTR(reg, 0664, aw99703_i2c_reg_show, aw99703_i2c_reg_store);

static struct attribute *aw99703_attributes[] = {
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group aw99703_attribute_group = {
	.attrs = aw99703_attributes
};

static int aw99703_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct aw99703_data *drvdata;
#ifdef KERNEL_ABOVE_4_14
	struct backlight_device *bl_dev;
	struct backlight_properties props;
#endif
	int err = 0;

	pr_info("%s enter! driver version %s\n", __func__,
		AW99703_DRIVER_VERSION);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : I2C_FUNC_I2C not supported\n", __func__);
		err = -EIO;
		goto err_out;
	}

	if (!client->dev.of_node) {
		pr_err("%s : no device node\n", __func__);
		err = -ENOMEM;
		goto err_out;
	}

	drvdata = kzalloc(sizeof(struct aw99703_data), GFP_KERNEL);
	if (drvdata == NULL) {
		pr_err("%s : kzalloc failed\n", __func__);
		err = -ENOMEM;
		goto err_out;
	}

	drvdata->client = client;
	drvdata->adapter = client->adapter;
	drvdata->addr = client->addr;
	drvdata->brightness = LED_OFF;
	drvdata->enable = true;
	drvdata->led_dev.default_trigger = "bkl-trigger";
	drvdata->led_dev.name = AW99703_LED_DEV;
	drvdata->led_dev.brightness_set = aw99703_brightness_set;
	drvdata->led_dev.max_brightness = MAX_BRIGHTNESS;
	mutex_init(&drvdata->lock);
	INIT_WORK(&drvdata->work, aw99703_work);
	aw99703_get_dt_data(&client->dev, drvdata);
	i2c_set_clientdata(client, drvdata);
	aw99703_gpio_init(drvdata);

	err = aw99703_read_chipid(drvdata);
	if (err < 0) {
		pr_err("%s : ID idenfy failed\n", __func__);
		goto err_init;
	}

	err = led_classdev_register(&client->dev, &drvdata->led_dev);
	if (err < 0) {
		pr_err("%s : Register led class failed\n", __func__);
		err = -ENODEV;
		goto err_init;
	} else {
		pr_debug("%s: Register led class successful\n", __func__);
	}

#ifdef KERNEL_ABOVE_4_14
	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.brightness = MAX_BRIGHTNESS;
	props.max_brightness = MAX_BRIGHTNESS;
	bl_dev = backlight_device_register(AW99703_NAME, &client->dev,
					drvdata, &aw99703_bl_ops, &props);
#endif

	g_aw99703_data = drvdata;

	err = sysfs_create_group(&client->dev.kobj, &aw99703_attribute_group);
	if (err < 0) {
		dev_info(&client->dev, "%s error creating sysfs attr files\n",
			__func__);
		goto err_sysfs;
	}
	pr_info("%s exit\n", __func__);
	return 0;

err_sysfs:
err_init:
	kfree(drvdata);
err_out:
	return err;
}

static int aw99703_remove(struct i2c_client *client)
{
	struct aw99703_data *drvdata = i2c_get_clientdata(client);

	led_classdev_unregister(&drvdata->led_dev);

	kfree(drvdata);
	return 0;
}

static int aw99703_suspend(struct device *dev)
{
	struct aw99703_data *drvdata = dev_get_drvdata(dev);

	pr_info("%s.\n", __func__);
	aw99703_set_work_mode(drvdata, 0);
	return 0;
}

static int aw99703_resume(struct device *dev)
{
	struct aw99703_data *drvdata = dev_get_drvdata(dev);

	pr_info("%s.\n", __func__);
	aw99703_backlight_init(drvdata);
	return 0;
}

static const struct dev_pm_ops aw99703_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(aw99703_suspend, aw99703_resume)
};

static const struct i2c_device_id aw99703_id[] = {
	{AW99703_NAME, 0},
	{}
};
static struct of_device_id match_table[] = {
	{.compatible = "awinic,aw99703-bl",}
};

MODULE_DEVICE_TABLE(i2c, aw99703_id);

static struct i2c_driver aw99703_i2c_driver = {
	.probe = aw99703_probe,
	.remove = aw99703_remove,
	.id_table = aw99703_id,
	.driver = {
		.name = AW99703_NAME,
		.owner = THIS_MODULE,
		.of_match_table = match_table,
		//.pm = &aw99703_pm_ops,
	},
};

module_i2c_driver(aw99703_i2c_driver);
MODULE_DESCRIPTION("Back Light driver for aw99703");
MODULE_LICENSE("GPL v2");

