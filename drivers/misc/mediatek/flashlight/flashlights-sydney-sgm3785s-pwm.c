/*
 * Copyright (C) 2025 MediaTek Inc.
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
#include <linux/pinctrl/consumer.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include "../include/mt-plat/mtk_pwm.h"
#include "flashlight-core.h"
#include "flashlight-dt.h"

/* define device tree */
#ifndef SGM3785S_PWM_DTNAME
#define SGM3785S_PWM_DTNAME "mediatek,flashlights_sydney_sgm3785s_pwm"
#endif

/* define driver name */
#define SGM3785S_FLASHLIGHT_NAME "flashlights-sydney-sgm3785s-pwm"

/* define pwm parameters */
#define SGM3785S_PWM_NUMBER               1
#define SGM3785S_PWM_PERIOD               1000

/* define current, level */
#define FLASH_FIRE_HIGH_MAXCURRENT      1400
#define FLASH_FIRE_LOW_MAXCURRENT       260
#define SGM3785S_LEVEL_NUM                26
#define SGM3785S_LEVEL_TORCH              7

#define SGM3785S_TIMEOUT 300 /* ms */

enum sgm3785s_flash_opcode {
    SGM3785S_FLASH_OP_NULL,
    SGM3785S_FLASH_OP_FIRELOW,
    SGM3785S_FLASH_OP_FIREHIGH
};

enum sgm3785s_flash_pwm {
    SGM3785S_FLASH_PWM_OFF,
    SGM3785S_FLASH_PWM_ON
};

static struct regmap *pwm_src_regmap;

/* define mutex and work queue */
static DEFINE_MUTEX(sgm3785s_flashlight_mutex);
static struct work_struct sgm3785s_work;

/* define pinctrl */
#define SGM3785S_PINCTRL_PIN_GPIO         1
#define SGM3785S_PINCTRL_PIN_PWM          0
#define SGM3785S_PINCTRL_PIN_PWM_EN       2
#define SGM3785S_PINCTRL_PINSTATE_LOW     0
#define SGM3785S_PINCTRL_PINSTATE_HIGH    1
#define SGM3785S_PINCTRL_STATE_GPIO_HIGH  "flashlight_gpio_high"
#define SGM3785S_PINCTRL_STATE_GPIO_LOW   "flashlight_gpio_low"
#define SGM3785S_PINCTRL_STATE_PWM_HIGH   "flashlight_pwm_high"
#define SGM3785S_PINCTRL_STATE_PWM_LOW    "flashlight_pwm_low"
#define SGM3785S_PINCTRL_STATE_PWM_EN     "flashlight_pwm_en"
static  struct pinctrl                  *sgm3785s_pinctrl;
static  struct pinctrl_state            *sgm3785s_gpio_high;
static  struct pinctrl_state            *sgm3785s_gpio_low;
static  struct pinctrl_state            *sgm3785s_pwm_high;
static  struct pinctrl_state            *sgm3785s_pwm_low;
static  struct pinctrl_state            *sgm3785s_pwm_en;

/* define usage count */
static int use_count;

/* platform data */
struct sgm3785s_flashlight_platform_data {
    int channel_num;
    struct flashlight_device_id *dev_id;
};

/* flash operation data */
struct sgm3785s_flash_operation_data {
    enum sgm3785s_flash_opcode opcode;
    enum sgm3785s_flash_pwm pwm_state;
    u32 flash_current;
};

/* define flash operation data */
static struct sgm3785s_flash_operation_data flash_opdata = {
    .opcode        = SGM3785S_FLASH_OP_NULL,
    .pwm_state     = SGM3785S_FLASH_PWM_OFF,
    .flash_current = 0,
};

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int sgm3785s_flashlight_pinctrl_init(struct platform_device *pdev)
{
    int ret = 0;
    pr_debug("%s: E.\n",__func__);

    /* get pinctrl */
    sgm3785s_pinctrl = devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(sgm3785s_pinctrl)) {
        pr_err("Failed to get flashlight pinctrl.\n");
        ret = PTR_ERR(sgm3785s_pinctrl);
        return ret;
    }

    /* Flashlight ENM pin initialization */
    sgm3785s_gpio_high = pinctrl_lookup_state(
            sgm3785s_pinctrl, SGM3785S_PINCTRL_STATE_GPIO_HIGH);
    if (IS_ERR(sgm3785s_gpio_high)) {
        pr_err("Failed to init (%s)\n", SGM3785S_PINCTRL_STATE_GPIO_HIGH);
        ret = PTR_ERR(sgm3785s_gpio_high);
    }
    sgm3785s_gpio_low = pinctrl_lookup_state(
            sgm3785s_pinctrl, SGM3785S_PINCTRL_STATE_GPIO_LOW);
    if (IS_ERR(sgm3785s_gpio_low)) {
        pr_err("Failed to init (%s)\n", SGM3785S_PINCTRL_STATE_GPIO_LOW);
        ret = PTR_ERR(sgm3785s_gpio_low);
    }

    /* Flashlight ENF pin initialization */
    sgm3785s_pwm_high = pinctrl_lookup_state(
            sgm3785s_pinctrl, SGM3785S_PINCTRL_STATE_PWM_HIGH);
    if (IS_ERR(sgm3785s_pwm_high)) {
        pr_err("Failed to init (%s)\n", SGM3785S_PINCTRL_STATE_PWM_HIGH);
        ret = PTR_ERR(sgm3785s_pwm_high);
    }
    sgm3785s_pwm_low = pinctrl_lookup_state(
            sgm3785s_pinctrl, SGM3785S_PINCTRL_STATE_PWM_LOW);
    if (IS_ERR(sgm3785s_pwm_low)) {
        pr_err("Failed to init (%s)\n", SGM3785S_PINCTRL_STATE_PWM_LOW);
        ret = PTR_ERR(sgm3785s_pwm_low);
    }

    sgm3785s_pwm_en = pinctrl_lookup_state(
            sgm3785s_pinctrl, SGM3785S_PINCTRL_STATE_PWM_EN);
    if (IS_ERR(sgm3785s_pwm_en)) {
        pr_err("Failed to init (%s)\n", SGM3785S_PINCTRL_STATE_PWM_EN);
        ret = PTR_ERR(sgm3785s_pwm_en);
    }

    pr_debug("%s: X.\n",__func__);

    return ret;
}

static int sgm3785s_pinctrl_set(int pin, int state)
{
    int ret = 0;

    if (IS_ERR(sgm3785s_pinctrl)) {
        pr_err("pinctrl is not available\n");
        return -1;
    }

    switch (pin) {
    case SGM3785S_PINCTRL_PIN_GPIO:
        if (state == SGM3785S_PINCTRL_PINSTATE_LOW &&
                !IS_ERR(sgm3785s_gpio_low))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_gpio_low);
        else if (state == SGM3785S_PINCTRL_PINSTATE_HIGH &&
                !IS_ERR(sgm3785s_gpio_high))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_gpio_high);
        else
            pr_err("set err, pin(%d) state(%d)\n", pin, state);
        break;
    case SGM3785S_PINCTRL_PIN_PWM:
        if (state == SGM3785S_PINCTRL_PINSTATE_LOW &&
                !IS_ERR(sgm3785s_pwm_low))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_pwm_low);
        else if (state == SGM3785S_PINCTRL_PINSTATE_HIGH &&
                !IS_ERR(sgm3785s_pwm_high))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_pwm_high);
        else
            pr_err("set err, pin(%d) state(%d)\n", pin, state);
        break;
    case SGM3785S_PINCTRL_PIN_PWM_EN:
        if (state == SGM3785S_PINCTRL_PINSTATE_LOW &&
                !IS_ERR(sgm3785s_pwm_low))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_pwm_low);
        else if (state == SGM3785S_PINCTRL_PINSTATE_HIGH &&
                !IS_ERR(sgm3785s_pwm_en))
            pinctrl_select_state(sgm3785s_pinctrl, sgm3785s_pwm_en);
        else
            pr_err("set err, pin(%d) state(%d)\n", pin, state);
        break;
    default:
        pr_err("set err, pin(%d) state(%d)\n", pin, state);
        break;
    }
    pr_debug("pin(%d) state(%d)\n", pin, state);

    return ret;
}

void mt_pwm_26M_clk_sel(void)
{
	regmap_update_bits(pwm_src_regmap, 0x24, 0x3 << 18 | 0x3 << 12, 0x1 << 18 | 0x1 << 12);
	return;
}

static int sgm3785s_flashlight_set_pwm(int pwm_num, u32 flash_current, u32 flash_maxcurrent)
{
    struct pwm_spec_config pwm_setting;
    memset(&pwm_setting, 0, sizeof(struct pwm_spec_config));
    flash_opdata.pwm_state                    = SGM3785S_FLASH_PWM_ON;

    pwm_setting.pwm_no                        = pwm_num;
    pwm_setting.mode                          = PWM_MODE_OLD;
    pwm_setting.pmic_pad                      = 0;
    pwm_setting.clk_div                       = CLK_DIV1;
    pwm_setting.clk_src                       = PWM_CLK_OLD_MODE_BLOCK;
    pwm_setting.PWM_MODE_OLD_REGS.IDLE_VALUE  = 0;
    pwm_setting.PWM_MODE_OLD_REGS.GUARD_VALUE = 0;
    pwm_setting.PWM_MODE_OLD_REGS.GDURATION   = 0;
    pwm_setting.PWM_MODE_OLD_REGS.WAVE_NUM    = 0;
    pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH  = SGM3785S_PWM_PERIOD;
    pwm_setting.PWM_MODE_OLD_REGS.THRESH      =
        SGM3785S_PWM_PERIOD * flash_current / flash_maxcurrent ;

    mt_pwm_26M_clk_sel();
    pwm_set_spec_config(&pwm_setting);

    pr_err("Set pwm_no = %d,PWM thresh = %u, period = %u, flash_current = %u, flash_maxcurrent = %u",
        pwm_setting.pwm_no,pwm_setting.PWM_MODE_OLD_REGS.THRESH, pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH,
        flash_current, flash_maxcurrent);
    return 0;
}

/******************************************************************************
 * sgm3785s pwm-flashlight operations
 *****************************************************************************/
static const u32 sgm3785s_torch_current[SGM3785S_LEVEL_NUM] = {
    10, 30, 50, 70, 80, 90, 100, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const u32 sgm3785s_flash_current[SGM3785S_LEVEL_NUM] = {
    10, 30, 50, 70, 80, 90, 100, 150, 200, 250, 300,
    350, 400, 450, 500, 550, 600, 650, 700, 750, 800,
    850, 900, 1000, 1150, 1172};

static void os_mdelay(unsigned long ms)
{
    unsigned long us = ms*1000;
    usleep_range(us, us+2000);
}

static int sgm3785s_is_torch(int level)
{
    if (level >= SGM3785S_LEVEL_TORCH)
        return -1;

    return 0;
}

static int sgm3785s_verify_level(int level)
{
    if (level < 0)
        level = 0;
    else if (level >= SGM3785S_LEVEL_NUM)
        level = SGM3785S_LEVEL_NUM - 1;

    return level;
}

/* set flashlight level */
static int sgm3785s_set_level(int level, struct sgm3785s_flash_operation_data *fl_opdata)
{
    /* wrap set level function */
    level = sgm3785s_verify_level(level);
    if(!sgm3785s_is_torch(level)) {
        fl_opdata->opcode        = SGM3785S_FLASH_OP_FIRELOW;
        fl_opdata->flash_current = sgm3785s_torch_current[level];
        pr_debug("set level %d current = %u, opcode = %d will enter movie mode",
            level, fl_opdata->flash_current, fl_opdata->opcode);
    } else {
        fl_opdata->opcode        = SGM3785S_FLASH_OP_FIREHIGH;
        fl_opdata->flash_current = sgm3785s_flash_current[level];
        pr_debug("set level %d current = %u, opcode = %d will enter flash mode",
            level, fl_opdata->flash_current, fl_opdata->opcode);
    }
    return 0;
}

/* flashlight enable function */
static int sgm3785s_enable(struct sgm3785s_flash_operation_data *fl_opdata)
{
    enum sgm3785s_flash_opcode opcode = fl_opdata->opcode;
    u32 flash_current = fl_opdata->flash_current;

    /* wrap enable function */
    switch (opcode)
    {
    case SGM3785S_FLASH_OP_FIRELOW:
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_GPIO, SGM3785S_PINCTRL_PINSTATE_LOW);
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM, SGM3785S_PINCTRL_PINSTATE_HIGH);
        os_mdelay(6);
        sgm3785s_flashlight_set_pwm(SGM3785S_PWM_NUMBER, flash_current, FLASH_FIRE_LOW_MAXCURRENT);
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM_EN, SGM3785S_PINCTRL_PINSTATE_HIGH);
        break;
    case SGM3785S_FLASH_OP_FIREHIGH:
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_GPIO, SGM3785S_PINCTRL_PINSTATE_HIGH);
        sgm3785s_flashlight_set_pwm(SGM3785S_PWM_NUMBER, flash_current, FLASH_FIRE_HIGH_MAXCURRENT);
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM_EN, SGM3785S_PINCTRL_PINSTATE_HIGH);
        break;
    default:
        pr_err("Flash opcode is error,failed to enable flashlight.\n");
        return -1;
    }
    return 0;
}

/* flashlight disable function */
static int sgm3785s_disable(struct sgm3785s_flash_operation_data *fl_opdata)
{
    pr_debug("%s: E.\n",__func__);
    /* wrap disable function */
    if (fl_opdata->pwm_state == SGM3785S_FLASH_PWM_ON) {
        mt_pwm_disable(SGM3785S_PWM_NUMBER, true);
        fl_opdata->pwm_state = SGM3785S_FLASH_PWM_OFF;
        sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM_EN, SGM3785S_PINCTRL_PINSTATE_LOW);
    }
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM, SGM3785S_PINCTRL_PINSTATE_LOW);
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_GPIO, SGM3785S_PINCTRL_PINSTATE_LOW);
    pr_debug("%s: X.\n",__func__);
    return 0;
}

/* flashlight init */
static int sgm3785s_init(void)
{
    pr_debug("%s: E.\n",__func__);
    /* wrap init function */
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM, 0);
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_GPIO, 0);
    pr_debug("%s: X.\n",__func__);
    return 0;
}

/* flashlight uninit */
static int sgm3785s_uninit(void)
{
    pr_debug("%s: E.\n",__func__);
    /* wrap uninit function */
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_PWM, 0);
    sgm3785s_pinctrl_set(SGM3785S_PINCTRL_PIN_GPIO, 0);
    pr_debug("%s: X.\n",__func__);
    return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer sgm3785s_timer;
static unsigned int sgm3785s_timeout_ms;

static void sgm3785s_flashlight_work_disable(struct work_struct *data)
{
    pr_debug("%s: E.\n",__func__);
    pr_debug("work queue callback\n");
    sgm3785s_disable(&flash_opdata);
}

static enum hrtimer_restart sgm3785s_timer_func(struct hrtimer *timer)
{
    pr_debug("%s: E.\n",__func__);
    schedule_work(&sgm3785s_work);
    return HRTIMER_NORESTART;
}

/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int sgm3785s_ioctl(unsigned int cmd, unsigned long arg)
{
    struct flashlight_dev_arg *fl_arg;
    int channel;
    ktime_t ktime;
    unsigned int s;
    unsigned int ns;

    pr_debug("%s: E.\n",__func__);

    fl_arg = (struct flashlight_dev_arg *)arg;
    channel = fl_arg->channel;

    switch (cmd) {
    case FLASH_IOC_SET_TIME_OUT_TIME_MS:
        pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
                channel, (int)fl_arg->arg);
        sgm3785s_timeout_ms = fl_arg->arg;
        break;

    case FLASH_IOC_SET_DUTY:
        pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
                channel, (int)fl_arg->arg);
        sgm3785s_set_level(fl_arg->arg, &flash_opdata);
        break;

    case FLASH_IOC_SET_ONOFF:
        pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
                channel, (int)fl_arg->arg);
        if (fl_arg->arg == 1) {
            if (sgm3785s_timeout_ms) {
                if(sgm3785s_timeout_ms > SGM3785S_TIMEOUT)
                        sgm3785s_timeout_ms = SGM3785S_TIMEOUT;
                pr_debug("sgm3785s_timeout_ms(%d)\n", sgm3785s_timeout_ms);
                s = sgm3785s_timeout_ms / 1000;
                ns = sgm3785s_timeout_ms % 1000 * 1000000;
                ktime = ktime_set(s, ns);
                hrtimer_start(&sgm3785s_timer, ktime,
                        HRTIMER_MODE_REL);
            }
            sgm3785s_enable(&flash_opdata);
        } else {
            sgm3785s_disable(&flash_opdata);
            hrtimer_cancel(&sgm3785s_timer);
        }
        break;
    case FLASH_IOC_GET_DUTY_NUMBER:
		pr_debug("FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = SGM3785S_LEVEL_NUM;
		break;
    case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_debug("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = SGM3785S_LEVEL_TORCH;
		break;
    default:
        pr_err("No such command and arg(%d): (%d, %d)\n",
                channel, _IOC_NR(cmd), (int)fl_arg->arg);
        return -ENOTTY;
    }

    pr_debug("%s: X.\n",__func__);

    return 0;
}

static int sgm3785s_open(void)
{
    /* Move to set driver for saving power */
    return 0;
}

static int sgm3785s_release(void)
{
    /* Move to set driver for saving power */
    return 0;
}

static int sgm3785s_flashlight_set_driver(int set)
{
    int ret = 0;

    pr_debug("%s: E.\n",__func__);

    /* set chip and usage count */
    mutex_lock(&sgm3785s_flashlight_mutex);
    if (set) {
        if (!use_count)
            ret = sgm3785s_init();
        use_count++;
        pr_debug("Set driver: %d\n", use_count);
    } else {
        use_count--;
        if (!use_count)
            ret = sgm3785s_uninit();
        if (use_count < 0)
            use_count = 0;
        pr_err("Unset driver: %d\n", use_count);
    }
    mutex_unlock(&sgm3785s_flashlight_mutex);

    pr_debug("%s: X.\n",__func__);

    return ret;
}

static ssize_t sgm3785s_strobe_store(struct flashlight_arg arg)
{
    pr_debug("%s: E.\n",__func__);
    sgm3785s_flashlight_set_driver(1);
    sgm3785s_set_level(arg.level, &flash_opdata);
    sgm3785s_timeout_ms = 0;
    sgm3785s_enable(&flash_opdata);
    msleep(arg.dur);
    sgm3785s_disable(&flash_opdata);
    sgm3785s_flashlight_set_driver(0);
    pr_debug("%s: X.\n",__func__);

    return 0;
}

static struct flashlight_operations sgm3785s_flashlight_ops = {
    sgm3785s_open,
    sgm3785s_release,
    sgm3785s_ioctl,
    sgm3785s_strobe_store,
    sgm3785s_flashlight_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int sgm3785s_chip_init(void)
{
    /* NOTE: Chip initialication move to "set driver" for power saving.
     * sgm3785s_init();
     */

    return 0;
}

static int sgm3785s_parse_dt(struct device *dev,
        struct sgm3785s_flashlight_platform_data *pdata)
{
    struct device_node *np, *cnp;
    u32 decouple = 0;
    int i = 0;

    pr_debug("%s: E.\n",__func__);

    if (!dev || !dev->of_node || !pdata)
        return -ENODEV;

    np = dev->of_node;

    pdata->channel_num = of_get_child_count(np);
    if (!pdata->channel_num) {
        pr_err("Parse no dt, node.\n");
        return 0;
    }
    pr_err("Channel number(%d).\n", pdata->channel_num);

    if (of_property_read_u32(np, "decouple", &decouple))
        pr_err("Parse no dt, decouple.\n");

    pdata->dev_id = devm_kzalloc(dev,
            pdata->channel_num *
            sizeof(struct flashlight_device_id),
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
        snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
                SGM3785S_FLASHLIGHT_NAME);
        pdata->dev_id[i].channel = i;
        pdata->dev_id[i].decouple = decouple;

        pr_err("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
                pdata->dev_id[i].type, pdata->dev_id[i].ct,
                pdata->dev_id[i].part, pdata->dev_id[i].name,
                pdata->dev_id[i].channel,
                pdata->dev_id[i].decouple);
        i++;
    }

    pr_debug("%s: X.\n",__func__);

    return 0;

err_node_put:
    of_node_put(cnp);
    return -EINVAL;
}

static int sgm3785s_flashlight_probe(struct platform_device *pdev)
{
    struct sgm3785s_flashlight_platform_data *pdata = dev_get_platdata(&pdev->dev);
    int err;
    int i;

    pr_debug("%s:Probe start E.\n",__func__);

    /* init pinctrl */
    if (sgm3785s_flashlight_pinctrl_init(pdev)) {
        pr_err("Failed to init pinctrl.\n");
        err = -EFAULT;
        goto err;
    }

	pwm_src_regmap = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
		"srcclk");
	if (IS_ERR(pwm_src_regmap)) {
		dev_err(&pdev->dev, "Cannot find pwm src controller: %ld\n",
		PTR_ERR(pwm_src_regmap));
	}

    /* init platform data */
    if (!pdata) {
        pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
        if (!pdata) {
            err = -ENOMEM;
            goto err;
        }
        pdev->dev.platform_data = pdata;
        err = sgm3785s_parse_dt(&pdev->dev, pdata);
        if (err)
            goto err;
    }

    /* init work queue */
    INIT_WORK(&sgm3785s_work, sgm3785s_flashlight_work_disable);

    /* init timer */
    hrtimer_init(&sgm3785s_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    sgm3785s_timer.function = sgm3785s_timer_func;
    sgm3785s_timeout_ms = 100;

    /* init chip hw */
    sgm3785s_chip_init();

    /* clear usage count */
    use_count = 0;

    /* register flashlight device */
    if (pdata->channel_num) {
        for (i = 0; i < pdata->channel_num; i++)
            if (flashlight_dev_register_by_device_id(
                        &pdata->dev_id[i],
                        &sgm3785s_flashlight_ops)) {
                err = -EFAULT;
                goto err;
            }
    } else {
        if (flashlight_dev_register(SGM3785S_FLASHLIGHT_NAME, &sgm3785s_flashlight_ops)) {
            err = -EFAULT;
            goto err;
        }
    }

    pr_debug("%s:Probe done X.\n",__func__);

    return 0;
err:
    return err;
}

static int sgm3785s_flashlight_remove(struct platform_device *pdev)
{
    struct sgm3785s_flashlight_platform_data *pdata = dev_get_platdata(&pdev->dev);
    int i;

    pr_debug("%s:Remove start E.\n",__func__);

    pdev->dev.platform_data = NULL;

    /* unregister flashlight device */
    if (pdata && pdata->channel_num)
        for (i = 0; i < pdata->channel_num; i++)
            flashlight_dev_unregister_by_device_id(
                    &pdata->dev_id[i]);
    else
        flashlight_dev_unregister(SGM3785S_FLASHLIGHT_NAME);

    /* flush work queue */
    flush_work(&sgm3785s_work);

    pr_debug("%s:Remove done X.\n",__func__);

    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sgm3785s_pwm_of_match[] = {
    {.compatible = SGM3785S_PWM_DTNAME},
    {},
};
MODULE_DEVICE_TABLE(of, sgm3785s_pwm_of_match);
#else
static struct platform_device sgm3785s_pwm_platform_device[] = {
    {
        .name = SGM3785S_FLASHLIGHT_NAME,
        .id = 0,
        .dev = {}
    },
    {}
};
MODULE_DEVICE_TABLE(platform, sgm3785s_pwm_platform_device);
#endif

static struct platform_driver sgm3785s_pwm_platform_driver = {
    .probe = sgm3785s_flashlight_probe,
    .remove = sgm3785s_flashlight_remove,
    .driver = {
        .name = SGM3785S_FLASHLIGHT_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = sgm3785s_pwm_of_match,
#endif
    },
};

static int __init flashlight_sgm3785s_init(void)
{
    int ret;

    pr_debug("Init start.\n");

#ifndef CONFIG_OF
    ret = platform_device_register(&sgm3785s_pwm_platform_device);
    if (ret) {
        pr_err("Failed to register platform device\n");
        return ret;
    }
#endif

    ret = platform_driver_register(&sgm3785s_pwm_platform_driver);
    if (ret) {
        pr_err("Failed to register platform driver\n");
        return ret;
    }

    pr_debug("Init done.\n");

    return 0;
}

static void __exit flashlight_sgm3785s_exit(void)
{
    pr_debug("Exit start.\n");

    platform_driver_unregister(&sgm3785s_pwm_platform_driver);

    pr_debug("Exit done.\n");
}

module_init(flashlight_sgm3785s_init);
module_exit(flashlight_sgm3785s_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LIU fulin <liufulin2@longcheer.com>");
MODULE_DESCRIPTION("SGM3785S MTK PWM Flashlight Drive");
