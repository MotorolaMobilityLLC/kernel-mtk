// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
#include <linux/pm_wakeup.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pwm.h>
#include <mt-plat/mtk_pwm.h>
#include "flashlight-core.h"
#include "flashlight-dt.h"

#define TAG_NAME "[flashligh_ocp8135b_drv]"
#define PK_DBG_NONE(fmt, arg...)  do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)  pr_info(TAG_NAME "%s: " fmt, __func__, ##arg)
#define PK_ERR(fmt, arg...)       pr_info(TAG_NAME "%s: " fmt, __func__, ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_INF(fmt, arg...)       pr_info(TAG_NAME "%s is called.\n", __func__)
#define PK_DBG                    PK_DBG_FUNC
#else
#define PK_INF(fmt, arg...)       do {} while (0)
#define PK_DBG(a, ...)
#endif


/* define device tree */
#ifndef OCP8135B_DTNAME
#define OCP8135B_DTNAME "mediatek,flashlights_ocp8135b"
#endif

#define OCP8135B_NAME "flashlights-ocp8135b"

/* define pinctrl */
#define OCP8135B_PINCTRL_PIN_HWENF 0
#define OCP8135B_PINCTRL_PIN_HWENM 1
#define OCP8135B_PINCTRL_PIN_PWM 2
#define OCP8135B_PINCTRL_PINSTATE_LOW 0
#define OCP8135B_PINCTRL_PINSTATE_HIGH 1
#define OCP8135B_PINCTRL_STATE_HWENF_HIGH "hwenf_high"
#define OCP8135B_PINCTRL_STATE_HWENF_LOW  "hwenf_low"
#define OCP8135B_PINCTRL_STATE_HWENM_HIGH "hwenm_high"
#define OCP8135B_PINCTRL_STATE_HWENM_LOW  "hwenm_low"
#define OCP8135B_PINCTRL_STATE_PWM        "ocp8135bpwm"
#define OCP8135B_LEVEL_TORCH 8
#define OCP8135B_LEVEL_NUM 32

static struct pinctrl *ocp8135b_pinctrl;
static struct pinctrl_state *ocp8135b_hwenf_high;
static struct pinctrl_state *ocp8135b_hwenf_low;
static struct pinctrl_state *ocp8135b_hwenm_high;
static struct pinctrl_state *ocp8135b_hwenm_low;
static struct pinctrl_state *ocp8135b_pwm;

/* define mutex and work queue */
static DEFINE_MUTEX(ocp8135b_mutex);
static struct work_struct ocp8135b_work;
static struct wakeup_source *suspend_lock;
static int g_level = 0;
/* define usage count */
static int use_count;

/* platform data */
struct ocp8135b_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};

int ocp8135b_pwm_set_config(int level)
{
	struct pwm_spec_config pwm_setting;

	pr_info("ocp8135b_set_pwm: level=%d\n", level);

	memset(&pwm_setting, 0, sizeof(struct pwm_spec_config));
	pwm_setting.pwm_no = PWM1;
	pwm_setting.mode = PWM_MODE_FIFO;
	pwm_setting.clk_div = CLK_DIV16;
	pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	pwm_setting.pmic_pad = false;

	pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = 4;
	pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = 4;
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 31;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

	if (level > 0 && level <= 32) {
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 = (1 << level) - 1;
		pwm_set_spec_config(&pwm_setting);
	}

	return 0;
}

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int ocp8135b_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	ocp8135b_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(ocp8135b_pinctrl)) {
		pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(ocp8135b_pinctrl);
	}

	/* Flashlight HWEN pin initialization */
	ocp8135b_hwenf_high = pinctrl_lookup_state(ocp8135b_pinctrl,
		OCP8135B_PINCTRL_STATE_HWENF_HIGH);
	if (IS_ERR(ocp8135b_hwenf_high)) {
		pr_err("Failed to init (%s)\n", OCP8135B_PINCTRL_STATE_HWENF_HIGH);
		ret = PTR_ERR(ocp8135b_hwenf_high);
		goto exit;
	}
	ocp8135b_hwenf_low = pinctrl_lookup_state(ocp8135b_pinctrl,
		OCP8135B_PINCTRL_STATE_HWENF_LOW);
	if (IS_ERR(ocp8135b_hwenf_low)) {
		pr_err("Failed to init (%s)\n", OCP8135B_PINCTRL_STATE_HWENF_LOW);
		ret = PTR_ERR(ocp8135b_hwenf_low);
		goto exit;
	}
	ocp8135b_hwenm_high = pinctrl_lookup_state(ocp8135b_pinctrl,
		OCP8135B_PINCTRL_STATE_HWENM_HIGH);
	if (IS_ERR(ocp8135b_hwenm_high)) {
		pr_err("Failed to init (%s)\n", OCP8135B_PINCTRL_STATE_HWENM_HIGH);
		ret = PTR_ERR(ocp8135b_hwenm_high);
		goto exit;
	}
	ocp8135b_hwenm_low = pinctrl_lookup_state(ocp8135b_pinctrl,
		OCP8135B_PINCTRL_STATE_HWENM_LOW);
	if (IS_ERR(ocp8135b_hwenm_low)) {
		pr_err("Failed to init (%s)\n", OCP8135B_PINCTRL_STATE_HWENM_LOW);
		ret = PTR_ERR(ocp8135b_hwenm_low);
		goto exit;
	}
	ocp8135b_pwm = pinctrl_lookup_state(ocp8135b_pinctrl,
		OCP8135B_PINCTRL_STATE_PWM);
	if (IS_ERR(ocp8135b_hwenm_low)) {
		pr_err("Failed to init (%s)\n", OCP8135B_PINCTRL_STATE_PWM);
		ret = PTR_ERR(ocp8135b_pwm);
		goto exit;
	}
exit:
	return ret;
}

static int ocp8135b_pinctrl_set(int pin, int state)
{
	int ret = 0;
	pr_info("%s in\n", __func__);

	if (IS_ERR(ocp8135b_pinctrl)) {
		pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case OCP8135B_PINCTRL_PIN_HWENF:
		if (state == OCP8135B_PINCTRL_PINSTATE_LOW &&
			!IS_ERR(ocp8135b_hwenf_low))
			pinctrl_select_state(ocp8135b_pinctrl, ocp8135b_hwenf_low);
		else if (state == OCP8135B_PINCTRL_PINSTATE_HIGH &&
			!IS_ERR(ocp8135b_hwenf_high))
			pinctrl_select_state(ocp8135b_pinctrl, ocp8135b_hwenf_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case OCP8135B_PINCTRL_PIN_HWENM:
		if (state == OCP8135B_PINCTRL_PINSTATE_LOW &&
			!IS_ERR(ocp8135b_hwenm_low))
			pinctrl_select_state(ocp8135b_pinctrl, ocp8135b_hwenm_low);
		else if (state == OCP8135B_PINCTRL_PINSTATE_HIGH &&
			!IS_ERR(ocp8135b_hwenm_high))
			pinctrl_select_state(ocp8135b_pinctrl, ocp8135b_hwenm_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case OCP8135B_PINCTRL_PIN_PWM:
		if (!IS_ERR(ocp8135b_pwm)){
				ocp8135b_pwm = pinctrl_lookup_state(ocp8135b_pinctrl, OCP8135B_PINCTRL_STATE_PWM);
				pinctrl_select_state(ocp8135b_pinctrl, ocp8135b_pwm);
			}
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_info("pin(%d) state(%d)\n", pin, state);
	pr_info("%s out\n", __func__);
	return ret;
}


/******************************************************************************
 * ocp8135b operations
 *****************************************************************************/

static int ocp8135b_set_torch_brightness(int val)
{
	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENM,
		OCP8135B_PINCTRL_PINSTATE_HIGH);

	mdelay(5);

	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENM,
		OCP8135B_PINCTRL_PINSTATE_LOW);

	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_PWM,
		OCP8135B_PINCTRL_PINSTATE_HIGH);

	ocp8135b_pwm_set_config(val*2-1);

	return 0;
}

static int ocp8135b_set_strobe_brightness(int val)
{
	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_PWM,
		OCP8135B_PINCTRL_PINSTATE_HIGH);
	ocp8135b_pwm_set_config(val*12/10+1);

	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENF,
		OCP8135B_PINCTRL_PINSTATE_HIGH);
	//mdelay(450);
	//ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENF,
		//OCP8135B_PINCTRL_PINSTATE_LOW);

	return 0;
}

static int ocp8135b_is_torch(int level)
{
	if (level >= OCP8135B_LEVEL_TORCH)
		return -1;

	return 0;
}

static int ocp8135b_verify_level(int level)
{
	if (level <= 0)
		level = 1;
	else if (level >= OCP8135B_LEVEL_NUM)
		level = OCP8135B_LEVEL_NUM;
	else
		level = level + 1;

	return level;
}

/* set flashlight level */
static int ocp8135b_set_level(int level)
{
	g_level = ocp8135b_verify_level(level);
	return 0;
}

/* flashlight enable function */

static int ocp8135b_enable(void)
{
	int pin_flash = OCP8135B_PINCTRL_PIN_HWENF;
	int pin_torch = OCP8135B_PINCTRL_PIN_HWENM;

	ocp8135b_pinctrl_set(pin_flash, OCP8135B_PINCTRL_PINSTATE_LOW);
	ocp8135b_pinctrl_set(pin_torch, OCP8135B_PINCTRL_PINSTATE_LOW);

	if (!ocp8135b_is_torch(g_level)){
		ocp8135b_set_torch_brightness(g_level);
	}else{
		ocp8135b_set_strobe_brightness(g_level);
	}

	__pm_stay_awake(suspend_lock);
	return 0;
}

/* flashlight disable function */
static int ocp8135b_disable(void)
{
	int pin_flash = OCP8135B_PINCTRL_PIN_HWENF;
	int pin_torch = OCP8135B_PINCTRL_PIN_HWENM;
	int state = OCP8135B_PINCTRL_PINSTATE_LOW;

	mt_pwm_disable(PWM1, false);
	ocp8135b_pinctrl_set(pin_torch, state);
	ocp8135b_pinctrl_set(pin_flash, state);

	__pm_relax(suspend_lock);
	return 0;
}

/* flashlight init */
static int ocp8135b_init(void)
{
	int pin_flash = OCP8135B_PINCTRL_PIN_HWENF;
	int pin_torch = OCP8135B_PINCTRL_PIN_HWENM;
	int state = OCP8135B_PINCTRL_PINSTATE_LOW;

	ocp8135b_pinctrl_set(pin_torch, state);
	ocp8135b_pinctrl_set(pin_flash, state);

	return 0;
}

/* flashlight uninit */
static int ocp8135b_uninit(void)
{
	ocp8135b_disable();
	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer ocp8135b_timer;
static unsigned int ocp8135b_timeout_ms;

static void ocp8135b_work_disable(struct work_struct *data)
{
	PK_DBG("work queue callback\n");
	ocp8135b_disable();
}

static enum hrtimer_restart ocp8135b_timer_func(struct hrtimer *timer)
{
	schedule_work(&ocp8135b_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int ocp8135b_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp8135b_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp8135b_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (ocp8135b_timeout_ms) {
				s = ocp8135b_timeout_ms / 1000;
				ns = ocp8135b_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&ocp8135b_timer, ktime,
						HRTIMER_MODE_REL);
			}
			ocp8135b_enable();
		} else {
			ocp8135b_disable();
			hrtimer_cancel(&ocp8135b_timer);
		}
		break;
	default:
		PK_DBG("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int ocp8135b_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp8135b_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp8135b_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&ocp8135b_mutex);
	if (set) {
		if (!use_count)
			ret = ocp8135b_init();
		use_count++;
		PK_DBG("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = ocp8135b_uninit();
		if (use_count < 0)
			use_count = 0;
		PK_DBG("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&ocp8135b_mutex);

	return ret;
}

static ssize_t ocp8135b_strobe_store(struct flashlight_arg arg)
{
	pr_info("%s in, arg.level = %d\n", __func__, arg.level);

	ocp8135b_set_driver(1);
	ocp8135b_set_level(arg.level);
	ocp8135b_timeout_ms = 0;
	ocp8135b_enable();
	mdelay(arg.dur);
	ocp8135b_disable();
	ocp8135b_set_driver(0);

	return 0;
}

static struct flashlight_operations ocp8135b_ops = {
	ocp8135b_open,
	ocp8135b_release,
	ocp8135b_ioctl,
	ocp8135b_strobe_store,
	ocp8135b_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int ocp8135b_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * ocp8135b_init();
	 */

	return 0;
}
/*
static int ocp8135b_parse_dt(struct device *dev,
		struct ocp8135b_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		PK_INF("Parse no dt, node.\n");
		return 0;
	}
	PK_INF("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		PK_INF("Parse no dt, decouple.\n");

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
				OCP8135B_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}
*/

static ssize_t show_torch_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ocp8135b_flashlight level = %d\n", g_level);
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
			ocp8135b_set_driver(1);
			ocp8135b_set_level(6);
			ocp8135b_timeout_ms = 0;
			ocp8135b_enable();
		}else {
			ocp8135b_disable();
			ocp8135b_set_driver(0);
		}
		PK_DBG("Set torch val:%d", val);
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

static int ocp8135b_probe(struct platform_device *pdev)
{
	struct ocp8135b_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err = 0;
	int i;

	PK_DBG("Probe start.\n");

	/* init pinctrl */
	if (ocp8135b_pinctrl_init(pdev)) {
		PK_DBG("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENF, OCP8135B_PINCTRL_PINSTATE_LOW);
	ocp8135b_pinctrl_set(OCP8135B_PINCTRL_PIN_HWENM, OCP8135B_PINCTRL_PINSTATE_LOW);

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		//err = ocp8135b_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&ocp8135b_work, ocp8135b_work_disable);

	/* init timer */
	hrtimer_init(&ocp8135b_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ocp8135b_timer.function = ocp8135b_timer_func;
	ocp8135b_timeout_ms = 100;

	suspend_lock =
		wakeup_source_register(NULL, "ocp8135b suspend wakelock");
	/* init chip hw */
	ocp8135b_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&ocp8135b_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(OCP8135B_NAME, &ocp8135b_ops)) {
			err = -EFAULT;
			goto err;
		}
	}

	if (sysfs_create_file(kernel_kobj, &torch_status_attr.attr)) {
		PK_DBG("sysfs_create_file torch status fail\n");
		err = -EFAULT;
		goto err;
	}

	PK_DBG("Probe done.\n");

	return 0;
err:
	return err;
}

static int ocp8135b_remove(struct platform_device *pdev)
{
	struct ocp8135b_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	PK_DBG("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(OCP8135B_NAME);

	/* flush work queue */
	flush_work(&ocp8135b_work);

	PK_DBG("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ocp8135b_gpio_of_match[] = {
	{.compatible = OCP8135B_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, ocp8135b_gpio_of_match);
#else
static struct platform_device ocp8135b_gpio_platform_device[] = {
	{
		.name = OCP8135B_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, ocp8135b_gpio_platform_device);
#endif

static struct platform_driver ocp8135b_platform_driver = {
	.probe = ocp8135b_probe,
	.remove = ocp8135b_remove,
	.driver = {
		.name = OCP8135B_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = ocp8135b_gpio_of_match,
#endif
	},
};

static int __init flashlight_ocp8135b_init(void)
{
	int ret;

	PK_DBG("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&ocp8135b_gpio_platform_device);
	if (ret) {
		PK_ERR("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&ocp8135b_platform_driver);
	if (ret) {
		PK_ERR("Failed to register platform driver\n");
		return ret;
	}

	PK_DBG("Init done.\n");

	return 0;
}

static void __exit flashlight_ocp8135b_exit(void)
{
	PK_DBG("Exit start.\n");

	platform_driver_unregister(&ocp8135b_platform_driver);

	PK_DBG("Exit done.\n");
}

module_init(flashlight_ocp8135b_init);
module_exit(flashlight_ocp8135b_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Dongchun Zhu <dongchun.zhu@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight OCP8135B GPIO Driver");

