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

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"

/* device tree should be defined in flashlight-dt.h */
#ifndef AW3643_DTNAME
#define AW3643_DTNAME "mediatek,flashlights_aw3643"
#endif
#ifndef AW3643_DTNAME_I2C
#define AW3643_DTNAME_I2C "mediatek,flashlights_aw3643_i2c"
#endif




#define FILE_NAME   "flashlights-aw3643.c"
#define LOG_INF(format, args...)  pr_err( FILE_NAME"[%s](%d) " format, __FUNCTION__, __LINE__, ##args)

#define AW3643_NAME "flashlights-aw3643"

/* define registers */
#define AW3643_REG_ENABLE (0x01)
#define AW3643_MASK_ENABLE_LED2 (0x01)
#define AW3643_MASK_ENABLE_LED1 (0x02)
#define AW3643_DISABLE (0x00)
#define AW3643_ENABLE_LED2 (0x01)
#define AW3643_ENABLE_LED2_TORCH (0x09)
#define AW3643_ENABLE_LED2_FLASH (0x0D)
#define AW3643_ENABLE_LED1 (0x02)
#define AW3643_ENABLE_LED1_TORCH (0x0A)
#define AW3643_ENABLE_LED1_FLASH (0x0E)

#define AW3643_REG_TORCH_LEVEL_LED2 (0x05)
#define AW3643_REG_FLASH_LEVEL_LED2 (0x03)
#define AW3643_REG_TORCH_LEVEL_LED1 (0x06)
#define AW3643_REG_FLASH_LEVEL_LED1 (0x04)

#define AW3643_REG_TIMING_CONF (0x08)
#define AW3643_TORCH_RAMP_TIME (0x00)
#define AW3643_FLASH_TIMEOUT   (0x0F)
#define AW3643TT_FLASH_TIMEOUT (0x09)

#define AW3643_REG_DEVICE_ID (0x0C)

/* define channel, level */
#define AW3643_CHANNEL_NUM 2
#define AW3643_CHANNEL_CH1 0
#define AW3643_CHANNEL_CH2 1

#define AW3643_LEVEL_NUM 39
#define AW3643_LEVEL_TORCH 7

#define AW3643_HW_TIMEOUT 400 /* ms */

/* define mutex and work queue */
static DEFINE_MUTEX(aw3643_mutex);
static struct work_struct aw3643_work_ch1;
static struct work_struct aw3643_work_ch2;

/* define pinctrl */
#define AW3643_PINCTRL_PIN_HWEN 0
#define AW3643_PINCTRL_PINSTATE_LOW 0
#define AW3643_PINCTRL_PINSTATE_HIGH 1
#define AW3643_PINCTRL_STATE_HWEN_HIGH "hwen_high"
#define AW3643_PINCTRL_STATE_HWEN_LOW  "hwen_low"
static struct pinctrl *aw3643_pinctrl;
static struct pinctrl_state *aw3643_hwen_high;
static struct pinctrl_state *aw3643_hwen_low;

/* aw3643 revision */
static int is_aw3643tt;

/*SY7806*/
static int is_SY7806 = 0;
#define SY7806_CHIP_ID (24)

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *aw3643_i2c_client;

static unsigned int torch_led_status = 0;

static unsigned int torch_led_level_support = 0; // [zhuqiupei add] Add torch_level_support

/* platform data */
struct aw3643_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};

/* aw3643 chip data */
struct aw3643_chip_data {
	struct i2c_client *client;
	struct aw3643_platform_data *pdata;
	struct mutex lock;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int aw3643_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	aw3643_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(aw3643_pinctrl)) {
		LOG_INF("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(aw3643_pinctrl);
	}

	/* Flashlight HWEN pin initialization */
	aw3643_hwen_high = pinctrl_lookup_state(aw3643_pinctrl, AW3643_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(aw3643_hwen_high)) {
		LOG_INF("Failed to init (%s)\n", AW3643_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(aw3643_hwen_high);
	}
	aw3643_hwen_low = pinctrl_lookup_state(aw3643_pinctrl, AW3643_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(aw3643_hwen_low)) {
		LOG_INF("Failed to init (%s)\n", AW3643_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(aw3643_hwen_low);
	}
    ret = 0;
	return ret;
}

static int aw3643_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(aw3643_pinctrl)) {
		LOG_INF("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case AW3643_PINCTRL_PIN_HWEN:
		if (state == AW3643_PINCTRL_PINSTATE_LOW && !IS_ERR(aw3643_hwen_low))
			pinctrl_select_state(aw3643_pinctrl, aw3643_hwen_low);
		else if (state == AW3643_PINCTRL_PINSTATE_HIGH && !IS_ERR(aw3643_hwen_high))
			pinctrl_select_state(aw3643_pinctrl, aw3643_hwen_high);
		else
			LOG_INF("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		LOG_INF("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	LOG_INF("pin(%d) state(%d)\n", pin, state);

	return ret;
}


/******************************************************************************
 * aw3643 operations
 *****************************************************************************/
static const int aw3643_current[AW3643_LEVEL_NUM] = {
	 22,  46,  70,  93, 116, 140, 163, 187,  210, 234,
	257, 281, 304, 327, 351, 374, 398, 421,  445, 468,
	492, 515, 539, 562, 585, 609, 632, 656,  679, 703,
	726, 761, 796, 832, 869, 902, 937, 972, 1008
};

static const unsigned char aw3643tt_torch_level[AW3643_LEVEL_NUM] = {// 39
	0x07, 0x10, 0x18, 0x21, 0x29, 0x31, 0x3A, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char aw3643_torch_level[AW3643_LEVEL_NUM] = {
	0x0F, 0x20, 0x31, 0x42, 0x52, 0x63, 0x74, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char aw3643_flash_level[AW3643_LEVEL_NUM] = {
	0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x10, 0x12, 0x14,
	0x17, 0x1A, 0x1E, 0x21, 0x24, 0x27, 0x2B, 0x2F, 0x33, 0x37,
	0x3B, 0x3F, 0x43, 0x47, 0x4B, 0x4F, 0x53, 0x57, 0x5B, 0x5F,
	0x63, 0x67, 0x6B, 0x6F, 0x73, 0x77, 0x7B, 0x7F, 0x83
}; //[lihehe] Modify flash current code

static unsigned char aw3643_reg_enable;
static int aw3643_level_ch1 = -1;
static int aw3643_level_ch2 = -1;

static int aw3643_is_torch(int level)
{
	if (level >= AW3643_LEVEL_TORCH)// 7
		return -1;

	return 0;
}

static int aw3643_verify_level(int level)
{
	if (level < 0)
    {
		level = 0;
    }
	else if (level >= AW3643_LEVEL_NUM)// 39
    {
		level = AW3643_LEVEL_NUM - 1;
    }
	return level;
}

/* i2c wrapper function */
static int aw3643_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct aw3643_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		LOG_INF("failed writing at 0x%02x\n", reg);

	return ret;
}

static int aw3643_read_reg(struct i2c_client *client, u8 reg)
{
	int val = 0;
	struct aw3643_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);

	return val;
}

/* flashlight enable function */
static int aw3643_enable_ch1(void)
{
	unsigned char reg, val;

	reg = AW3643_REG_ENABLE;

	if (!aw3643_is_torch(aw3643_level_ch1)) 
    {
		LOG_INF("ontim_flashlight enable ch1 torch mode\n");
		/* torch mode */
		aw3643_reg_enable |= AW3643_ENABLE_LED1_TORCH;
	} 
    else 
    {
		LOG_INF("ontim_flashlight enable ch1 flash mode\n");
		/* flash mode */
		aw3643_reg_enable |= AW3643_ENABLE_LED1_FLASH;
	}
	val = aw3643_reg_enable;
        

	return aw3643_write_reg(aw3643_i2c_client, reg, val);
}

static int aw3643_enable_ch2(void)
{
	unsigned char reg, val;

	reg = AW3643_REG_ENABLE;

	if (!aw3643_is_torch(aw3643_level_ch2)) 
    {
		LOG_INF("ontim_flashlight enable ch2 torch mode\n");
		/* torch mode */
		aw3643_reg_enable |= AW3643_ENABLE_LED2_TORCH; //[lihehe] Enable mode switching
	} 
    else 
    {
		LOG_INF("ontim_flashlight enable ch2 flash mode\n");
		/* flash mode */
		aw3643_reg_enable |= AW3643_ENABLE_LED2_FLASH; //[lihehe] Enable mode switching
	}
        //aw3643_reg_enable |= AW3643_ENABLE_LED2_TORCH; //[lihehe] Enable mode switching
	val = aw3643_reg_enable;

	return aw3643_write_reg(aw3643_i2c_client, reg, val);
}

static int aw3643_enable(int channel)
{
	if (channel == AW3643_CHANNEL_CH1)// 0
		aw3643_enable_ch1();
	else if (channel == AW3643_CHANNEL_CH2)
		aw3643_enable_ch2();
	else 
    {
		LOG_INF("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight disable function */
static int aw3643_disable_ch1(void)
{
	unsigned char reg, val;

	reg = AW3643_REG_ENABLE;
	if (aw3643_reg_enable & AW3643_MASK_ENABLE_LED2) {
		/* if LED 2 is enable, disable LED 1 */
		aw3643_reg_enable &= (~AW3643_ENABLE_LED1);
	} else {
		/* if LED 2 is disable, disable LED 1 and clear mode */
		aw3643_reg_enable &= (~AW3643_ENABLE_LED1_FLASH);
	}
	val = aw3643_reg_enable;

	torch_led_status = 0;

        LOG_INF("ontim disabl_ch1  write reg:0x%02x   val:0x%02X\n",reg,val);

	return aw3643_write_reg(aw3643_i2c_client, reg, val);
}

static int aw3643_disable_ch2(void)
{
	unsigned char reg, val;

	reg = AW3643_REG_ENABLE;
	if (aw3643_reg_enable & AW3643_MASK_ENABLE_LED1) {
		/* if LED 1 is enable, disable LED 2 */
		aw3643_reg_enable &= (~AW3643_ENABLE_LED2);
	} else {
		/* if LED 1 is disable, disable LED 2 and clear mode */
		aw3643_reg_enable &= (~AW3643_ENABLE_LED2_FLASH);
	}
	val = aw3643_reg_enable;

	return aw3643_write_reg(aw3643_i2c_client, reg, val);
}

static int aw3643_disable(int channel)
{
	if (channel == AW3643_CHANNEL_CH1)
		aw3643_disable_ch1();
	else if (channel == AW3643_CHANNEL_CH2)
		aw3643_disable_ch2();
	else {
		LOG_INF("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int aw3643_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;
    
	LOG_INF("ontim_flashlight  level=%d\n", level);
	level = aw3643_verify_level(level);

	/* set torch brightness level */
	reg = AW3643_REG_TORCH_LEVEL_LED1;

	if (is_aw3643tt)
		val = aw3643tt_torch_level[level];
	else
		val = aw3643_torch_level[level];
    val = 0x18;
	LOG_INF("is_aw3643tt=%d  level=%d  val=%d\n", is_aw3643tt, level, val);
        LOG_INF("ontim_flashlight  set_level_ch1 write torch reg val: 0x%02x\n",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);
	aw3643_level_ch1 = level;

	/* set flash brightness level */
	reg = AW3643_REG_FLASH_LEVEL_LED1;
	val = aw3643_flash_level[level];
    //if(level == 11)val = 0x5d;// 1100mA
        //level = 7-11
        //val: 0x0F, 0x11, 0x13,	0x15, 0x17,
        //i = val * 11.72 + 11.35 
        //val += 0x5A; //[lihehe] Disable current hop
        if(val>0x7F) val = 0x7F;
        LOG_INF("ontim_flashlight set_level_ch1 write flash reg val: 0x%02x\n cur:?",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

        /*set light on timing  torch 0ms  flash 400ms*/
        ret |= aw3643_write_reg(aw3643_i2c_client, AW3643_REG_TIMING_CONF, 0x09);
	return ret;
}

static int aw3643_set_level_ch2(int level)
{
	int ret;
	unsigned char reg, val;
        LOG_INF("ontim_flashlight  level=%d\n", level);

	/* [linyimin start] Fix #53872: Fix on torch mode if TPTorch on*/
	if (torch_led_status && aw3643_is_torch(level)) {
		level = AW3643_LEVEL_TORCH - 1;
		LOG_INF("ontim_flashlight  TPTorch on, reset level=%d\n", level);
	}
	/* [linyimin end] Fix #53872 Fix on torch mode if TPTorch on*/

	level = aw3643_verify_level(level);

	/* set torch brightness level */
	reg = AW3643_REG_TORCH_LEVEL_LED2;
	/* [lihehe start] Enable multi current levels */
	if (is_aw3643tt)
		val = aw3643tt_torch_level[level];
	else
		val = aw3643_torch_level[level];
	/* [lihehe end] Enable multi current levels */
        // i = 30*2.91+ 2.55 = 90mA
        LOG_INF("ontim_flashlight aw3643 set_level_ch2 write torch reg val: 0x%02x\n",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

	aw3643_level_ch2 = level;

	/* set flash brightness level */
	reg = AW3643_REG_FLASH_LEVEL_LED2;
	val = aw3643_flash_level[level]; 
        //if(val < 0x011) val = 0x11;//min:0x11*11.72+11.35 = 198mA //[lihehe] Disable current limit
        if(val > 0x1A) val = 0x1A;//max:0x1A 320mA
         LOG_INF("ontim_flashlight set_level_ch2 write flash reg val: 0x%02x  cur:?",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

        /*set light on timing  torch 1ms  flash 400s*/
        ret |= aw3643_write_reg(aw3643_i2c_client, AW3643_REG_TIMING_CONF, 0x09);
	return ret;
}

/* set flashlight level */
static int sy7806_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;
    
	LOG_INF("ontim_flashlight  level=%d\n", level);
	level = aw3643_verify_level(level);

	/* set torch brightness level */
	reg = AW3643_REG_TORCH_LEVEL_LED1;

	val = aw3643tt_torch_level[level]*2;
	val = 0x31;
        LOG_INF("ontim_flashlight sy7806  set_level_ch1 write torch reg val: 0x%02x\n",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);
	aw3643_level_ch1 = level;

	/* set flash brightness level */
	reg = AW3643_REG_FLASH_LEVEL_LED1;
	val = aw3643_flash_level[level];
    //if(level == 11)val = 0x5d;// 1100mA
        //level = 7-11
        //val: 0x0F, 0x11, 0x13,	0x15, 0x17,
        //i = (val +1)*11.725mA 
        //i:1242mA->1336mA
        //val += 0x5A; //[lihehe] Disable current hop
        if(val>0x7F) val = 0x7F;
        LOG_INF("ontim_flashlight  sy7806 set_level_ch1 write flash reg val: 0x%02x\n cur:?",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

        /*set light on timing  torch 1024ms  flash  400s*/
        ret |= aw3643_write_reg(aw3643_i2c_client, AW3643_REG_TIMING_CONF, 0x0F);
	return ret;
}

static int sy7806_set_level_ch2(int level)
{
	int ret;
	unsigned char reg, val;
        LOG_INF("ontim_flashlight  level=%d\n", level);

	/* [linyimin start] Fix #53872: Fix on torch mode if TPTorch on*/
	if (torch_led_status && aw3643_is_torch(level)) {
		level = AW3643_LEVEL_TORCH - 1;
		LOG_INF("ontim_flashlight  TPTorch on, reset level=%d\n", level);
	}
	/* [linyimin end] Fix #53872 Fix on torch mode if TPTorch on*/

	level = aw3643_verify_level(level);

	/* set torch brightness level */
	reg = AW3643_REG_TORCH_LEVEL_LED2;
	if (is_aw3643tt)
		val = aw3643tt_torch_level[level] * 2;
	else
		val = aw3643_torch_level[level] * 2;
        //val = 0x20; // [lihehe] Enable multi level current for front LED
        //i = (63+1)*1.4 = 90mA
        LOG_INF("ontim_flashlight sy7806 set_level_ch2 write torch reg val: 0x%02x\n",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

	aw3643_level_ch2 = level;

	/* set flash brightness level */
	reg = AW3643_REG_FLASH_LEVEL_LED2;
	val = aw3643_flash_level[level]; 
        //if(val < 0x011) val = 0x11;//min:0x11*11.72+11.35 = 198mA //[lihehe] Disable current limit
        if(val > 0x1A) val = 0x1A;//max:0x1A 320mA
         LOG_INF("ontim_flashlight  sy7806 set_level_ch2 write flash reg val: 0x%02x  cur:?",val);
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

        /*set light on timing  torch 1024ms  flash 400ms*/
        ret |= aw3643_write_reg(aw3643_i2c_client, AW3643_REG_TIMING_CONF, 0x0F);
	return ret;
}


static int flashlight_set_level(int channel, int level)
{
    LOG_INF("channel=%d, level=%d\n", channel, level);

    if(is_SY7806)
        {
            if (channel == AW3643_CHANNEL_CH1)// 0
                sy7806_set_level_ch1(level);
            else if (channel == AW3643_CHANNEL_CH2)// 1
                sy7806_set_level_ch2(level);
            else
                return -1;
    }
    else{
            if (channel == AW3643_CHANNEL_CH1)// 0
                aw3643_set_level_ch1(level);
            else if (channel == AW3643_CHANNEL_CH2)// 1
                aw3643_set_level_ch2(level);
            else
                return -1; 
    }
	return 0;
}

/* flashlight init */
int aw3643_init(void)
{
	int ret;
	unsigned char reg, val;

	aw3643_pinctrl_set(AW3643_PINCTRL_PIN_HWEN, AW3643_PINCTRL_PINSTATE_HIGH);
	msleep(20);

	/* get silicon revision */
	is_aw3643tt = aw3643_read_reg(aw3643_i2c_client, AW3643_REG_DEVICE_ID);
	LOG_INF("AW3643(TT) revision(%d).\n", is_aw3643tt);
        if(is_aw3643tt == SY7806_CHIP_ID )// 24
            {
            LOG_INF("ontim flashlight CHIP:SY7806");
            is_SY7806 = 1;
        }

	/* clear enable register */
	reg = AW3643_REG_ENABLE;
	val = AW3643_DISABLE;
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

	aw3643_reg_enable = val;

	/* set torch current ramp time an dflash timeout */
	reg = AW3643_REG_TIMING_CONF;
	if (is_aw3643tt)
		val = AW3643_TORCH_RAMP_TIME | AW3643TT_FLASH_TIMEOUT;
	else
		val = AW3643_TORCH_RAMP_TIME | AW3643_FLASH_TIMEOUT;
	ret = aw3643_write_reg(aw3643_i2c_client, reg, val);

//  bit7  need be 0
	ret = aw3643_write_reg(aw3643_i2c_client, AW3643_REG_FLASH_LEVEL_LED2, 0x3f);
	ret = aw3643_write_reg(aw3643_i2c_client, AW3643_REG_TORCH_LEVEL_LED2, 0x3f);

	return ret;
}

/* flashlight uninit */
int aw3643_uninit(void)
{
	aw3643_disable(AW3643_CHANNEL_CH1);
	aw3643_disable(AW3643_CHANNEL_CH2);
	aw3643_pinctrl_set(AW3643_PINCTRL_PIN_HWEN, AW3643_PINCTRL_PINSTATE_LOW);

	return 0;
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer aw3643_timer_ch1;
static struct hrtimer aw3643_timer_ch2;
static unsigned int aw3643_timeout_ms[AW3643_CHANNEL_NUM];// 2

static void aw3643_work_disable_ch1(struct work_struct *data)
{
	LOG_INF("ht work queue callback\n");
	aw3643_disable_ch1();
}

static void aw3643_work_disable_ch2(struct work_struct *data)
{
	LOG_INF("lt work queue callback\n");
	aw3643_disable_ch2();
}

static enum hrtimer_restart aw3643_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&aw3643_work_ch1);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart aw3643_timer_func_ch2(struct hrtimer *timer)
{
	schedule_work(&aw3643_work_ch2);
	return HRTIMER_NORESTART;
}

static int aw3643_timer_start(int channel, ktime_t ktime)
{
	if (channel == AW3643_CHANNEL_CH1)
    {
		hrtimer_start(&aw3643_timer_ch1, ktime, HRTIMER_MODE_REL);
    }

	else if (channel == AW3643_CHANNEL_CH2)
    {
		hrtimer_start(&aw3643_timer_ch2, ktime, HRTIMER_MODE_REL);
    }
	else 
    {
		LOG_INF("Error channel\n");
		return -1;
	}

	return 0;
}

static int aw3643_timer_cancel(int channel)
{
	if (channel == AW3643_CHANNEL_CH1)
		hrtimer_cancel(&aw3643_timer_ch1);
	else if (channel == AW3643_CHANNEL_CH2)
		hrtimer_cancel(&aw3643_timer_ch2);
	else {
		LOG_INF("Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int aw3643_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= AW3643_CHANNEL_NUM) 
    {
		LOG_INF("Failed with error channel=%d\n", channel);
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		LOG_INF("case FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",channel, (int)fl_arg->arg);
		aw3643_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		LOG_INF("case FLASH_IOC_SET_DUTY(%d): %d\n",channel, (int)fl_arg->arg);
		flashlight_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		LOG_INF("case FLASH_IOC_SET_ONOFF(%d): %d\n",channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) 
        {
			if (aw3643_timeout_ms[channel]) 
            {
				ktime = ktime_set(aw3643_timeout_ms[channel] / 1000, (aw3643_timeout_ms[channel] % 1000) * 1000000);
				aw3643_timer_start(channel, ktime);
			}
			aw3643_enable(channel);
		} 
        else
        {
			aw3643_disable(channel);
			aw3643_timer_cancel(channel);
		}
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
		LOG_INF("case FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = AW3643_LEVEL_NUM;
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		LOG_INF("case FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = AW3643_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = aw3643_verify_level(fl_arg->arg);
		LOG_INF("case FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",channel, (int)fl_arg->arg);
		fl_arg->arg = aw3643_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		LOG_INF("case FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = AW3643_HW_TIMEOUT;
		break;

	default:
		LOG_INF("No such command and arg(%d): (%d, %d)\n",channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int aw3643_open(void)
{
	/* Move to set driver for saving power */
	LOG_INF("start.\n");
	return 0;
}

static int aw3643_release(void)
{
	/* Move to set driver for saving power */
	LOG_INF("start.\n");
	return 0;
}

static int aw3643_set_driver(int set)
{
	int ret = 0;
	LOG_INF("start.\n");
	/* set chip and usage count */
	mutex_lock(&aw3643_mutex);
	if (set) 
    {
		if (!use_count)
			ret = aw3643_init();
		use_count++;
		LOG_INF("Set driver: %d\n", use_count);
	} 
    else 
    {
		use_count--;
		if (!use_count)
			ret = aw3643_uninit();
		if (use_count < 0)
			use_count = 0;
		LOG_INF("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&aw3643_mutex);

	return ret;
}

static ssize_t aw3643_strobe_store(struct flashlight_arg arg)
{
	LOG_INF("start.\n");
	aw3643_set_driver(1);
	flashlight_set_level(arg.channel, arg.level);
	aw3643_timeout_ms[arg.channel] = 0;
	aw3643_enable(arg.channel);
	msleep(arg.dur);
	aw3643_disable(arg.channel);
	aw3643_set_driver(0);

	return 0;
}

static struct flashlight_operations aw3643_ops = {
	aw3643_open,
	aw3643_release,
	aw3643_ioctl,
	aw3643_strobe_store,
	aw3643_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int aw3643_chip_init(struct aw3643_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * aw3643_init();
	 */

	return 0;
}

static int aw3643_parse_dt(struct device *dev,
		struct aw3643_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		LOG_INF("Parse no dt, node.\n");
		return 0;
	}
	LOG_INF("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		LOG_INF("Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num * sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE, AW3643_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		LOG_INF("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel, pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int aw3643_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw3643_platform_data *pdata = dev_get_platdata(&client->dev);
	struct aw3643_chip_data *chip;
	int err;
	int i;

	LOG_INF("Probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		LOG_INF("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct aw3643_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err_free;
		}
		client->dev.platform_data = pdata;
		err = aw3643_parse_dt(&client->dev, pdata);
		if (err)
			goto err_free;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	aw3643_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init work queue */
	INIT_WORK(&aw3643_work_ch1, aw3643_work_disable_ch1);
	INIT_WORK(&aw3643_work_ch2, aw3643_work_disable_ch2);

	/* init timer */
	hrtimer_init(&aw3643_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw3643_timer_ch1.function = aw3643_timer_func_ch1;
	hrtimer_init(&aw3643_timer_ch2, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw3643_timer_ch2.function = aw3643_timer_func_ch2;
	aw3643_timeout_ms[AW3643_CHANNEL_CH1] = 100;
	aw3643_timeout_ms[AW3643_CHANNEL_CH2] = 100;

	/* init chip hw */
	aw3643_chip_init(chip);

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(&pdata->dev_id[i], &aw3643_ops)) {
				err = -EFAULT;
				goto err_free;
			}
	} else {
		if (flashlight_dev_register(AW3643_NAME, &aw3643_ops)) {
			err = -EFAULT;
			goto err_free;
		}
	}

	LOG_INF("return 0\n");
	return 0;

err_free:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
err_out:
	return err;
}

static int aw3643_i2c_remove(struct i2c_client *client)
{
	struct aw3643_platform_data *pdata = dev_get_platdata(&client->dev);
	struct aw3643_chip_data *chip = i2c_get_clientdata(client);
	int i;

	LOG_INF("Remove start.\n");

	client->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(AW3643_NAME);

	/* flush work queue */
	flush_work(&aw3643_work_ch1);
	flush_work(&aw3643_work_ch2);

	/* free resource */
	kfree(chip);

	LOG_INF("return 0\n");

	return 0;
}

static const struct i2c_device_id aw3643_i2c_id[] = {
	{AW3643_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id aw3643_i2c_of_match[] = {
	{.compatible = AW3643_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver aw3643_i2c_driver = {
	.driver = {
		.name = AW3643_NAME,
#ifdef CONFIG_OF
		.of_match_table = aw3643_i2c_of_match,
#endif
	},
	.probe = aw3643_i2c_probe,
	.remove = aw3643_i2c_remove,
	.id_table = aw3643_i2c_id,
};

// [zhuqiupei start] Add torch_level_support
static ssize_t show_torch_level_support(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", torch_led_level_support);
}

// [zhuqiupei end]

static ssize_t show_torch_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", torch_led_status);
}

static ssize_t store_torch_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if (val) {
			/* Torch LED ON */
			switch (val) {
			case 1:
				aw3643_set_driver(1);
				if(is_SY7806) {
					sy7806_set_level_ch1(6);
				}
				else {
					aw3643_set_level_ch1(6);// 163mA
				}
				aw3643_enable_ch1();
				break;
			default:
				aw3643_set_driver(1);
				if(is_SY7806) {
					sy7806_set_level_ch1(6);
				}
				else {
					aw3643_set_level_ch1(6);// 163mA
				}
				aw3643_enable_ch1();
				break;
			}
			torch_led_status = val;
		}else {
			aw3643_disable_ch1();
			aw3643_set_driver(0);
			torch_led_status = 0;
		}
		LOG_INF("Set torch val:%d", val);
	}
	return size;
}

static const struct device_attribute torch_status_attr = {
	.attr = {
		.name = "torch_status",
		.mode = 0666,
	},
	.show = show_torch_status,
	.store = store_torch_status,
};

// [zhuqiupei add] Add torch_level_support
static const struct device_attribute torch_level_support_attr = {
	.attr = {
		.name = "torch_level_support",
		.mode = 0444,
	},
	.show = show_torch_level_support,
	.store = NULL,
};
// [zhuqiupei end]

//static const struct device_attribute rt_led_pwm_mode_attrs

//static DEVICE_ATTR(torch_status, 0666, show_torch_status, store_torch_status);

/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int aw3643_probe(struct platform_device *dev)
{
	LOG_INF("Probe start.\n");

	LOG_INF("platform_device dev->name=%s\n", dev->name);
	/* init pinctrl */
	if (aw3643_pinctrl_init(dev)) {
		LOG_INF("Failed to init pinctrl.\n");
		return -1;
	}

	if (i2c_add_driver(&aw3643_i2c_driver)) {
		LOG_INF("Failed to add i2c driver.\n");
		return -1;
	}

	if (sysfs_create_file(kernel_kobj, &torch_status_attr.attr)) {
		LOG_INF("sysfs_create_file torch status fail\n");
		return -1;
	}

	// [zhuqiupei start] Add torch_level_support
	if (sysfs_create_file(kernel_kobj, &torch_level_support_attr.attr)) {
		LOG_INF("sysfs_create_file torch level support fail\n");
		return -1;
	}
	// [zhuqiupei end]

	torch_led_status = 0;

	LOG_INF("Probe done.\n");

	return 0;
}

static int aw3643_remove(struct platform_device *dev)
{
	LOG_INF("Remove start.\n");

	i2c_del_driver(&aw3643_i2c_driver);

	sysfs_remove_file(kernel_kobj, &torch_status_attr.attr);

	sysfs_remove_file(kernel_kobj, &torch_level_support_attr.attr); // [zhuqiupei add] Add torch_level_support

	LOG_INF("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw3643_of_match[] = {
	{.compatible = AW3643_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, aw3643_of_match);
#else
static struct platform_device aw3643_platform_device[] = {
	{
		.name = AW3643_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, aw3643_platform_device);
#endif

static struct platform_driver aw3643_platform_driver = {
	.probe = aw3643_probe,
	.remove = aw3643_remove,
	.driver = {
		.name = AW3643_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw3643_of_match,
#endif
	},
};

static int __init flashlight_aw3643_init(void)
{
	int ret;

	LOG_INF("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&aw3643_platform_device);
	if (ret) {
		LOG_INF("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&aw3643_platform_driver);
	if (ret) {
		LOG_INF("Failed to register platform driver\n");
		return ret;
	}

	LOG_INF("Init done.\n");

	return 0;
}

static void __exit flashlight_aw3643_exit(void)
{
	LOG_INF("Exit start.\n");

	platform_driver_unregister(&aw3643_platform_driver);

	LOG_INF("Exit done.\n");
}

module_init(flashlight_aw3643_init);
module_exit(flashlight_aw3643_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ziyu Jiang <jiangziyu@meizu.com>");
MODULE_DESCRIPTION("MTK Flashlight AW3643 Driver");

