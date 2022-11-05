/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
*
*  etxxx_fp.c
*  Date: 2016/03/16
*  Version: 0.9.0.1
*  Revise Date:  2020/11/25
*  Copyright (C) 2007-2019 Egis Technology Inc.
* -----------------  version history ------------------------
* <Author>		<Data>			<Desc>
*Kevin.Liang	20181102		add powersetup for IOC
*Jacob.Kung		20201125		modify for support all sensor
*Jacob.Kung		20201126		modify struct define naming
*Jacob.Kung		20201130		add check_ioctl_permission
*Jacob.Kung		20201207		register platform driver with MTK platform
*Jacob.Kung		20210331		add register FB notifier
*Jacob.Kung		20210512		add register DRM notifier
* -----------------------------------------------------------
*
**/
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/pm_wakeup.h>
#include "etxxx_fp.h"
#include <linux/input.h>


extern void mt_spi_enable_master_clk(struct spi_device *spidev);
extern void mt_spi_disable_master_clk(struct spi_device *spidev);

struct egisfp_dev_t *g_data = NULL;
DECLARE_BITMAP(minors, N_SPI_MINORS);
LIST_HEAD(device_list);
DEFINE_MUTEX(device_list_lock);

static struct egis_key_map_t key_maps[] = {
	{EV_KEY, EGIS_NAV_INPUT_UP},
	{EV_KEY, EGIS_NAV_INPUT_DOWN},
	{EV_KEY, EGIS_NAV_INPUT_LEFT},
	{EV_KEY, EGIS_NAV_INPUT_RIGHT},
	{EV_KEY, EGIS_NAV_INPUT_CLICK},
	{EV_KEY, EGIS_NAV_INPUT_DOUBLE_CLICK},
	{EV_KEY, EGIS_NAV_INPUT_LONG_PRESS},
	{EV_KEY, EGIS_NAV_INPUT_FINGER_DOWN},
	{EV_KEY, EGIS_NAV_INPUT_FINGER_UP},
};

int egisfp_probe(struct platform_device *pdev);
int egisfp_remove(struct platform_device *pdev);

int egisfp_spi_probe(struct spi_device *spi);
int egisfp_spi_remove(struct spi_device *spi);
/* -------------------------------------------------------------------- */

struct of_device_id egistec_match_table[] = {
	{
		.compatible = "fp-egistec",
	},
	{
		.compatible = "mediatek,fingerprint",
	},
	{},
};

static struct platform_driver egisfp_driver = {
	.driver = {
		.name = EGIS_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = egistec_match_table,
	},
	.probe = egisfp_probe,
	.remove = egisfp_remove,
};
static struct spi_driver egisfp_spi_driver = {
	.driver = {
		.name = EGIS_SPI_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = egistec_match_table,
	},
	.probe = egisfp_spi_probe,
	.remove = egisfp_spi_remove,
};
int egisfp_spi_probe(struct spi_device *spi)
{
	int error = 0;
	struct egisfp_spi_dev_t *egis_spi_dev = NULL;
	/* size_t buffer_size; */
	INFO_PRINT(" %s \n", __func__);
	egis_spi_dev = kzalloc(sizeof(struct egisfp_spi_dev_t), GFP_KERNEL);
	if (egis_spi_dev == NULL)
	{
		ERROR_PRINT(" %s : Failed to kzalloc \n", __func__);
		return -ENOMEM;
	}
	spi_set_drvdata(spi, egis_spi_dev);
	g_data->spi = spi;
	return error;
}
/* -------------------------------------------------------------------- */
int egisfp_spi_remove(struct spi_device *spi)
{
	struct egisfp_spi_dev_t *egis_spi_dev = spi_get_drvdata(spi);
	g_data->spi = NULL;
	kfree(egis_spi_dev);
	return 0;
}

/* add for clk enable from kernel*/

void spi_clk_enable(struct egisfp_dev_t *egis_dev, int bonoff)
{
	if (!egis_dev->spi)
	{
		ERROR_PRINT(" %s : enable/disable spi clk fail: device is null \n", __func__);
		return;
	}
	if (egis_dev->clk_enabled == bonoff)
	{
		INFO_PRINT(" %s : bonoff = %d, duplicate enable/disable spi clk \n", __func__, bonoff);
		return;
	}
	if (bonoff)
	{
		DEBUG_PRINT(" %s : enable spi clk \n", __func__);
		mt_spi_enable_master_clk(egis_dev->spi);
		egis_dev->clk_enabled = 1;
	}
	else
	{
		DEBUG_PRINT(" %s : disable spi clk \n", __func__);
		mt_spi_disable_master_clk(egis_dev->spi);
		egis_dev->clk_enabled = 0;
	}
}

/* add for clk enable from kernel*/

/* ------------------------------ Interrupt -----------------------------*/

static DECLARE_WAIT_QUEUE_HEAD(interrupt_waitq);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void egisfp_interrupt_timer_call(struct timer_list *t)
{
	struct egisfp_interrupt_desc_t *fps_int = from_timer(fps_int, t, timer);
	struct egisfp_dev_t *egis_dev;

	egis_dev = container_of(fps_int, struct egisfp_dev_t, fps_ints);
	INFO_PRINT(" %s : interrupt count = %d \n", __func__, egis_dev->fps_ints.int_count);

	if (egis_dev->fps_ints.int_count >= egis_dev->fps_ints.detect_threshold)
	{
		egis_dev->fps_ints.finger_on = 1;
		INFO_PRINT(" %s : interrupt triggered \n", __func__);
	}
	else
	{
		INFO_PRINT(" %s : interrupt not triggered \n", __func__);
	}
	egis_dev->fps_ints.int_count = 0;
	wake_up_interruptible(&interrupt_waitq);
}
#else
void egisfp_interrupt_timer_call(unsigned long _data)
{
	struct egisfp_interrupt_desc_t *fps_int = (struct egisfp_interrupt_desc_t *)_data;
	struct egisfp_dev_t *egis_dev;

	egis_dev = container_of(fps_int, struct egisfp_dev_t, fps_ints);
	INFO_PRINT(" %s : interrupt count = %d \n", __func__, egis_dev->fps_ints.int_count);

	if (egis_dev->fps_ints.int_count >= egis_dev->fps_ints.detect_threshold)
	{
		egis_dev->fps_ints.finger_on = 1;
		INFO_PRINT(" %s : interrupt triggered \n", __func__);
	}
	else
	{
		INFO_PRINT(" %s : interrupt not triggered \n", __func__);
	}
	egis_dev->fps_ints.int_count = 0;
	wake_up_interruptible(&interrupt_waitq);
}
#endif
irqreturn_t fp_eint_func(int irq, void *dev_id)
{
	struct egisfp_dev_t *egis_dev = dev_id;
	if (!egis_dev->fps_ints.int_count)
		mod_timer(&egis_dev->fps_ints.timer, jiffies + msecs_to_jiffies(egis_dev->fps_ints.detect_period));
	egis_dev->fps_ints.int_count++;
	INFO_PRINT(" %s : fps_ints.int_count=%d \n", __func__, egis_dev->fps_ints.int_count);
	pm_wakeup_event(&egis_dev->dd->dev, WAKE_HOLD_TIME);
	return IRQ_HANDLED;
}

irqreturn_t fp_eint_func_ll(int irq, void *dev_id)
{
	struct egisfp_dev_t *egis_dev = dev_id;
	unsigned long irqflags;
	INFO_PRINT(" %s \n", __func__);
	spin_lock_irqsave(&egis_dev->irq_lock, irqflags);
	if (egis_dev->fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE)
	{
		egis_dev->fps_ints.finger_on = 1;
		disable_irq_nosync(egis_dev->gpio_irq);
		INFO_PRINT(" %s : DRDY_IRQ_DISABLE \n", __func__);
		egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	}
	spin_unlock_irqrestore(&egis_dev->irq_lock, irqflags);
	wake_up_interruptible(&interrupt_waitq);
	pm_wakeup_event(&egis_dev->dd->dev, WAKE_HOLD_TIME);

	return IRQ_RETVAL(IRQ_HANDLED);
}

/*
 *	FUNCTION NAME.
 *		egisfp_interrupt_init
 *
 *	FUNCTIONAL DESCRIPTION.
 *		button initial routine
 *
 *	ENTRY PARAMETERS.
 *		int_mode - determine trigger mode
 *			EDGE_TRIGGER_FALLING    0x0
 *			EDGE_TRIGGER_RAISING    0x1
 *			LEVEL_TRIGGER_LOW        0x2
 *			LEVEL_TRIGGER_HIGH       0x3
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

int egisfp_interrupt_init(struct egisfp_dev_t *egis_dev, int int_mode, int detect_period, int detect_threshold)
{

	int err = 0;
	unsigned long irqflags;

	INFO_PRINT(" %s :  mode = %d period = %d threshold = %d \n", __func__, int_mode, detect_period, detect_threshold);
	INFO_PRINT(" %s :  request_irq_done = %d gpio_irq = %d  pin = %d \n", __func__, egis_dev->request_irq_done, egis_dev->gpio_irq, egis_dev->irqPin);

	egis_dev->fps_ints.detect_period = detect_period;
	egis_dev->fps_ints.detect_threshold = detect_threshold;
	egis_dev->fps_ints.int_count = 0;
	egis_dev->fps_ints.finger_on = 0;
	egis_dev->fps_ints.drdy_irq_abort = 0;

	if (egis_dev->request_irq_done == 0)
	{
		if (gpio_is_valid(egis_dev->irqPin))
		{
			egis_dev->gpio_irq = gpio_to_irq(egis_dev->irqPin);
			INFO_PRINT(" %s : fp_irq number %d \n", __func__, egis_dev->gpio_irq);
		}
		else
		{
			ERROR_PRINT(" %s : irqPin is not valid \n", __func__);
			return -EIO;
		}

		if (egis_dev->gpio_irq < 0)
		{
			ERROR_PRINT(" %s : gpio_to_irq failed \n", __func__);
			return -EIO;
		}

		INFO_PRINT(" %s : current flag : %d disable: %d enable: %d \n", __func__, egis_dev->fps_ints.drdy_irq_flag, DRDY_IRQ_DISABLE, DRDY_IRQ_ENABLE);
		if (int_mode == EDGE_TRIGGER_RISING)
		{
			INFO_PRINT(" %s : EDGE_TRIGGER_RISING \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func, IRQ_TYPE_EDGE_RISING, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == EDGE_TRIGGER_FALLING)
		{
			INFO_PRINT(" %s : EDGE_TRIGGER_FALLING \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func, IRQ_TYPE_EDGE_FALLING, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == LEVEL_TRIGGER_LOW)
		{
			INFO_PRINT(" %s : LEVEL_TRIGGER_LOW \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func_ll, IRQ_TYPE_LEVEL_LOW, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == LEVEL_TRIGGER_HIGH)
		{
			INFO_PRINT(" %s : LEVEL_TRIGGER_HIGH \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func_ll, IRQ_TYPE_LEVEL_HIGH, "fp_detect-eint", egis_dev);
		}
		else
		{
			ERROR_PRINT(" %s : Invalid argument \n", __func__);
			return -EINVAL;
		}

		if (err)
		{
			ERROR_PRINT(" %s : request_irq failed return %d \n", __func__, err);
			return err;
		}

		disable_irq_nosync(egis_dev->gpio_irq);
		INFO_PRINT(" %s : request_irq return %d \n", __func__, err);
		egis_dev->request_irq_done = 1;
		egis_dev->fps_ints.irq_mode = int_mode;
	} 
	else if (egis_dev->fps_ints.irq_mode != int_mode)
	{
		if (egis_dev->fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE)
		{
			disable_irq_nosync(egis_dev->gpio_irq);
			egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
		}

		free_irq(egis_dev->gpio_irq, egis_dev);
		egis_dev->request_irq_done = 0;
		if (int_mode == EDGE_TRIGGER_RISING)
		{
			INFO_PRINT(" %s : EDGE_TRIGGER_RISING \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func, IRQ_TYPE_EDGE_RISING, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == EDGE_TRIGGER_FALLING)
		{
			INFO_PRINT(" %s : EDGE_TRIGGER_FALLING \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func, IRQ_TYPE_EDGE_FALLING, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == LEVEL_TRIGGER_LOW)
		{
			INFO_PRINT(" %s : LEVEL_TRIGGER_LOW \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func_ll, IRQ_TYPE_LEVEL_LOW, "fp_detect-eint", egis_dev);
		}
		else if (int_mode == LEVEL_TRIGGER_HIGH)
		{
			INFO_PRINT(" %s : LEVEL_TRIGGER_HIGH \n", __func__);
			err = request_irq(egis_dev->gpio_irq, fp_eint_func_ll, IRQ_TYPE_LEVEL_HIGH, "fp_detect-eint", egis_dev);
		}
		else
		{
			ERROR_PRINT(" %s : Invalid argument \n", __func__);
			return -EINVAL;
		}

		if (err)
		{
			ERROR_PRINT(" %s : request_irq failed return %d \n", __func__, err);
			return err;
		}

		disable_irq_nosync(egis_dev->gpio_irq);
		INFO_PRINT(" %s : request_irq return %d \n", __func__, err);
		egis_dev->request_irq_done = 1;
		egis_dev->fps_ints.irq_mode = int_mode;
	}

	INFO_PRINT(" %s : current flag : %d disable: %d enable: %d \n", __func__, egis_dev->fps_ints.drdy_irq_flag, DRDY_IRQ_DISABLE, DRDY_IRQ_ENABLE);
	spin_lock_irqsave(&egis_dev->irq_lock, irqflags);
	if (egis_dev->fps_ints.drdy_irq_flag == DRDY_IRQ_DISABLE)
	{
		egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_ENABLE;
		INFO_PRINT(" %s : enable irq with mode = %d \n", __func__, int_mode);
		enable_irq_wake(egis_dev->gpio_irq);
		enable_irq(egis_dev->gpio_irq);
	}
	spin_unlock_irqrestore(&egis_dev->irq_lock, irqflags);

	return err;
}

/*
 *	FUNCTION NAME.
 *		egisfp_interrupt_free
 *
 *	FUNCTIONAL DESCRIPTION.
 *		free all interrupt resource
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

int egisfp_interrupt_free(struct egisfp_dev_t *egis_dev)
{
	unsigned long irqflags;
	DEBUG_PRINT(" %s \n", __func__);
	spin_lock_irqsave(&egis_dev->irq_lock, irqflags);
	egis_dev->fps_ints.finger_on = 0;
	if (egis_dev->fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE)
	{
		DEBUG_PRINT(" %s : irq disable \n", __func__);
		disable_irq_nosync(egis_dev->gpio_irq);
		del_timer_sync(&egis_dev->fps_ints.timer);
		egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	}
	spin_unlock_irqrestore(&egis_dev->irq_lock, irqflags);
	return 0;
}

/*
 *	FUNCTION NAME.
 *		fps_interrupt_re d
 *
 *	FUNCTIONAL DESCRIPTION.
 *		FPS interrupt read status
 *
 *	ENTRY PARAMETERS.
 *		wait poll table structure
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

unsigned int egisfp_interrupt_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct egisfp_dev_t *egis_dev;
	egis_dev = file->private_data;
	poll_wait(file, &interrupt_waitq, wait);
	if (egis_dev->fps_ints.finger_on)
	{
		mask |= POLLIN | POLLRDNORM;
	}
	else if (egis_dev->fps_ints.drdy_irq_abort == 1)
	{
		mask |= 0x400 | POLLRDNORM;
		egis_dev->fps_ints.drdy_irq_abort = 0;
	}
	return mask;
}

void egisfp_interrupt_abort(struct egisfp_dev_t *egis_dev)
{
	DEBUG_PRINT(" %s \n", __func__);
	egis_dev->fps_ints.finger_on = 0;
	egis_dev->fps_ints.drdy_irq_abort = 1;
	wake_up_interruptible(&interrupt_waitq);
}

/*-------------------------------------------------------------------------*/

static void send_navi_event(struct egisfp_dev_t *egis_dev, int nav_event)
{
	uint32_t input_event;

	switch (nav_event)
	{
	case NAVI_EVENT_ON:
		DEBUG_PRINT(" %s : finger down \n", __func__);
		input_event = EGIS_NAV_INPUT_FINGER_DOWN;
		break;
	case NAVI_EVENT_OFF:
		DEBUG_PRINT(" %s : finger up \n", __func__);
		input_event = EGIS_NAV_INPUT_FINGER_UP;
		break;
	case NAVI_EVENT_UP:
		DEBUG_PRINT(" %s : nav swip up \n", __func__);
		input_event = EGIS_NAV_INPUT_UP;
		break;
	case NAVI_EVENT_DOWN:
		DEBUG_PRINT(" %s : nav swip down \n", __func__);
		input_event = EGIS_NAV_INPUT_DOWN;
		break;
	case NAVI_EVENT_LEFT:
		DEBUG_PRINT(" %s : nav swip left \n", __func__);
		input_event = EGIS_NAV_INPUT_LEFT;
		break;
	case NAVI_EVENT_RIGHT:
		DEBUG_PRINT(" %s : nav swip right \n", __func__);
		input_event = EGIS_NAV_INPUT_RIGHT;
		break;
	case NAVI_EVENT_CLICK:
		DEBUG_PRINT(" %s : nav finger click \n", __func__);
		input_event = EGIS_NAV_INPUT_CLICK;
		break;
	case NAVI_EVENT_DOUBLE_CLICK:
		DEBUG_PRINT(" %s : nav finger double click \n", __func__);
		input_event = EGIS_NAV_INPUT_DOUBLE_CLICK;
		break;
	case NAVI_EVENT_LONG_PRESS:
		DEBUG_PRINT(" %s : nav finger long press \n", __func__);
		input_event = EGIS_NAV_INPUT_LONG_PRESS;
		break;
	default:
		DEBUG_PRINT(" %s : unknown nav event: %d \n", __func__, nav_event);
		input_event = 0;
		break;
	}

	if (input_event)
	{
		input_report_key(egis_dev->input_dev, input_event, 1);
		input_sync(egis_dev->input_dev);
		input_report_key(egis_dev->input_dev, input_event, 0);
		input_sync(egis_dev->input_dev);
	}
}

int do_egisfp_reset(struct egisfp_dev_t *egis_dev)
{
	int ret = 0;
	ret |= pinctrl_select_state(egis_dev->pinctrl, egis_dev->reset_low);
	mdelay(Rst_off_delay);
	ret |= pinctrl_select_state(egis_dev->pinctrl, egis_dev->reset_high);
	mdelay(Rst_on_delay);
	if (ret)
		ERROR_PRINT(" %s : failed ret = %d \n", __func__, ret);
	else
		DEBUG_PRINT(" %s : reset_pin = %d \n", __func__, gpio_get_value(egis_dev->rstPin));
	return ret;
}

int egisfp_set_screen_onoff(int on)
{
	DEBUG_PRINT(" %s \n", __func__);

	if (!g_data)
		return -ENXIO;

	g_data->screen_onoff = on;

	DEBUG_PRINT(" %s screen_onoff %d \n", __func__, g_data->screen_onoff);

	return 0;
}
EXPORT_SYMBOL(egisfp_set_screen_onoff);

int do_egisfp_power_onoff(struct egisfp_dev_t *egis_dev, struct egisfp_ioctl_cmd_t *ioctl_data)
{
	int ret = 0;
	uint32_t power_onoff = ioctl_data->power_on;

	if (!egis_dev->ctrl_power)
	{
		INFO_PRINT(" %s : power can not control by driver \n", __func__);
		return ret;
	}

	DEBUG_PRINT(" %s : power_onoff = %d \n", __func__, power_onoff);

	if (egis_dev->power_enable == power_onoff)
	{
		INFO_PRINT(" %s : duplicate set power on/off \n", __func__);
		return ret;
	}

	if (power_onoff)
	{
		if (egis_dev->pwr_by_gpio)
			ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->vcc_high);
		else
			ret = regulator_enable(egis_dev->vcc);

		if (ret)
			goto do_egisfp_power_onoff_failed;

		msleep(Tpwr_on_delay);
		egis_dev->power_enable = 1;
	}
	else
	{
		if (egis_dev->pwr_by_gpio)
			ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->vcc_low);
		else
			ret = regulator_disable(egis_dev->vcc);

		if (ret)
			goto do_egisfp_power_onoff_failed;

		msleep(Tpwr_off_delay);
		egis_dev->power_enable = 0;
	}
	if (egis_dev->pwr_by_gpio)
		DEBUG_PRINT(" %s : power_pin value = %d \n", __func__, gpio_get_value(egis_dev->vcc_33v_Pin));
	else
		DEBUG_PRINT(" %s : regulator enable = %d output voltage %d \n", __func__, regulator_is_enabled(egis_dev->vcc), regulator_get_voltage(egis_dev->vcc));

	return ret;

do_egisfp_power_onoff_failed:
	ERROR_PRINT(" %s : failed ret = %d \n", __func__, ret);
	return ret;
}
int do_egisfp_reset_set(struct egisfp_dev_t *egis_dev, int reset_high_low)
{
	int ret = 0;
	DEBUG_PRINT(" %s : reset_high_low = %d \n", __func__, reset_high_low);

	if (reset_high_low)
	{
		ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->reset_high);
		mdelay(Rst_on_delay);
	}
	else
	{
		ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->reset_low);
		mdelay(Rst_off_delay);
	}

	if (ret)
		ERROR_PRINT(" %s : failed ret = %d \n", __func__, ret);
	else
		DEBUG_PRINT(" %s : reset_pin value = %d \n", __func__, gpio_get_value(egis_dev->rstPin));
	return ret;
}

static void egis_get_io_stus(struct egisfp_dev_t *egis_dev)
{
	INFO_PRINT(" %s \n", __func__);
	if (egis_dev->ctrl_power)
	{
		if (egis_dev->pwr_by_gpio)
			INFO_PRINT(" %s : power_pin value = %d \n", __func__, gpio_get_value(egis_dev->vcc_33v_Pin));
		else
			INFO_PRINT(" %s : regulator vcc enable = %d \n", __func__, regulator_is_enabled(egis_dev->vcc));
	}

	INFO_PRINT(" %s : reset_pin value = %d irq_pin value = %d \n", __func__, gpio_get_value(egis_dev->rstPin), gpio_get_value(egis_dev->irqPin));
}

int set_egisfp_spi_pin_active(struct egisfp_dev_t *egis_dev, int en)
{
	int ret;
	DEBUG_PRINT(" %s : en = 0x%X \n", __func__, en);
	if (en)
	{
		ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->spi_active);
	}
	else
	{
		ret = pinctrl_select_state(egis_dev->pinctrl, egis_dev->spi_default);
	}
	if (ret)
		ERROR_PRINT(" %s : failed ret = %d \n", __func__, ret);
	return ret;
}

long egisfp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	struct egisfp_dev_t *egis_dev = NULL;
	struct egisfp_ioctl_cmd_t data = {0};

	INFO_PRINT(" %s : cmd = 0x%X \n", __func__, cmd);
	egis_dev = (struct egisfp_dev_t *)filp->private_data;

	if (!egis_dev->pars_dtsi_done)
	{
		ERROR_PRINT(" %s : egis_dev is NULL \n", __func__);
		return -ENODEV;
	}

	if (egisfp_check_ioctl_permission(egis_dev, cmd))
		return -EACCES;

	switch (cmd)
	{
	case FP_GET_RESOURCE:
		DEBUG_PRINT(" %s : FP_GET_RESOURCE \n", __func__);
		if (egisfp_platforminit(egis_dev))
			retval = -EIO;
		break;
	case FP_FREE_RESOURCE:
		DEBUG_PRINT(" %s : FP_FREE_RESOURCE \n", __func__);
		egisfp_platformfree(egis_dev);
		break;
	case FP_SET_SPI_CLOCK:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			return -EFAULT;
		}
		egis_dev->clk_speed = data.int_mode * 1000000;
		DEBUG_PRINT(" %s : FP_SET_SPI_CLOCK %d \n", __func__, egis_dev->clk_speed);
		break;
	case INT_TRIGGER_INIT:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			return -EFAULT;
		}
		retval = egisfp_interrupt_init(egis_dev, data.int_mode, data.detect_period, data.detect_threshold);
		DEBUG_PRINT(" %s : INT_TRIGGER_INIT %x \n", __func__, retval);
		break;
	case FP_SENSOR_RESET:
		DEBUG_PRINT(" %s : FP_SENSOR_RESET \n", __func__);
		do_egisfp_reset(egis_dev);
		break;
	case FP_POWER_ONOFF:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			return -EFAULT;
		}
		DEBUG_PRINT(" %s : FP_POWER_ONOFF \n", __func__);
		retval = do_egisfp_power_onoff(egis_dev, &data);
		break;
	case FP_RESET_SET:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			return -EFAULT;
		}
		DEBUG_PRINT(" %s : FP_RESET_SET \n", __func__);
		retval = do_egisfp_reset_set(egis_dev, data.int_mode); // Use data.int_mode as reset setting. 1 = on, 0 = off.
		break;
	case FP_WAKELOCK_ENABLE:
		DEBUG_PRINT(" %s : FP_WAKELOCK_ENABLE \n", __func__);
		pm_stay_awake(&egis_dev->dd->dev);
		break;
	case FP_WAKELOCK_DISABLE:
		DEBUG_PRINT(" %s : FP_WAKELOCK_DISABLE \n", __func__);
		pm_relax(&egis_dev->dd->dev);
		break;
	case GET_IO_STUS:
		DEBUG_PRINT(" %s : GET_IO_STUS \n", __func__);
		egis_get_io_stus(egis_dev);
		break;
	case SEND_NAVI_EVENT:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			return -EFAULT;
		}
		DEBUG_PRINT(" %s : SEND_NAVI_EVENT \n", __func__);
		send_navi_event(egis_dev, data.int_mode);
		break;
	case FP_SPIPIN_SETTING:
		DEBUG_PRINT(" %s : FP_SPIPIN_SETTING \n", __func__);
		retval = set_egisfp_spi_pin_active(egis_dev, 1);
		break;
	case FP_SPIPIN_PULLLOW:
		DEBUG_PRINT(" %s : FP_SPIPIN_PULLLOW \n", __func__);
		retval = set_egisfp_spi_pin_active(egis_dev, 0);
		break;
	case INT_TRIGGER_CLOSE:
		retval = egisfp_interrupt_free(egis_dev);
		DEBUG_PRINT(" %s : INT_TRIGGER_CLOSE %d  \n", __func__, retval);
		break;
	case INT_TRIGGER_ABORT:
		DEBUG_PRINT(" %s : INT_TRIGGER_ABORT \n", __func__);
		egisfp_interrupt_abort(egis_dev);
		break;
	case FP_FREE_GPIO:
		DEBUG_PRINT(" %s : FP_FREE_GPIO \n", __func__);
		break;
	case FP_WAKELOCK_TIMEOUT_ENABLE: //0Xb1
		if (copy_from_user(&data, (int __user *)arg, sizeof(data)))
		{
			data.int_mode = WAKE_HOLD_TIME;
		}
		if (data.int_mode == 0)
			data.int_mode = WAKE_HOLD_TIME;

		DEBUG_PRINT(" %s : FP_WAKELOCK_TIMEOUT_ENABLE %d ms \n", __func__, data.int_mode);
		pm_wakeup_event(&egis_dev->dd->dev, data.int_mode);
		break;
	case FP_WAKELOCK_TIMEOUT_DISABLE: //0Xb2
		DEBUG_PRINT(" %s : FP_WAKELOCK_TIMEOUT_DISABLE \n", __func__);
		break;
	case FP_SPICLK_ENABLE:
		DEBUG_PRINT(" %s : FP_SPICLK_ENABLE \n", __func__);
		spi_clk_enable(egis_dev, 1);
		break;
	case FP_SPICLK_DISABLE:
		DEBUG_PRINT(" %s : FP_SPICLK_DISABLE \n", __func__);
		spi_clk_enable(egis_dev, 0);
		break;
	case DELETE_DEVICE_NODE:
		DEBUG_PRINT(" %s : DELETE_DEVICE_NODE \n", __func__);
		//delete_device_node();
		break;
	case GET_SCREEN_ONOFF:
		DEBUG_PRINT(" %s : GET_SCREEN_ONOFF \n", __func__);
		data.int_mode = egis_dev->screen_onoff;
		if (copy_to_user((int __user *)arg, &data, sizeof(data)))
		{
			return -EFAULT;
		}
		break;
	default:
		retval = 0;
		break;
	}
	DEBUG_PRINT(" %s done \n", __func__);
	return retval;
}

#ifdef CONFIG_COMPAT
long egisfp_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return egisfp_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define egisfp_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

int egisfp_open(struct inode *inode, struct file *filp)
{
	struct egisfp_dev_t *egis_dev = NULL;
	int status = -ENXIO;
	DEBUG_PRINT(" %s \n", __func__);

	mutex_lock(&device_list_lock);
	list_for_each_entry(egis_dev, &device_list, device_entry)
	{
		if (egis_dev->devt == inode->i_rdev)
		{
			status = 0;
			break;
		}
	}

	if (status == 0)
	{
		/* device tree call */
		if (egis_dev->dd->dev.of_node && !egis_dev->pars_dtsi_done)
		{
			status = egisfp_parse_dt(egis_dev);
		}

		if (!status)
		{
			egis_dev->pars_dtsi_done = 1;
			egis_dev->users++;
			filp->private_data = egis_dev;
			nonseekable_open(inode, filp);
			INFO_PRINT(" %s : open ioctl user count = %d \n", __func__, egis_dev->users);
		}
		else
		{
			ERROR_PRINT(" %s : egisfp_parse_dt fail, status = %d \n", __func__, status);
		}
	}
	else
	{
		ERROR_PRINT(" %s : nothing for minor %d \n", __func__, iminor(inode));
	}
	mutex_unlock(&device_list_lock);
	return status;
}

int egisfp_release(struct inode *inode, struct file *filp)
{
	struct egisfp_dev_t *egis_dev;
	DEBUG_PRINT(" %s \n", __func__);
	mutex_lock(&device_list_lock);
	egis_dev = filp->private_data;
	filp->private_data = NULL;
	/* last close? */
	egis_dev->users--;
	if (egis_dev->users == 0)
	{
		INFO_PRINT(" %s : egis_dev->users == %d \n", __func__, egis_dev->users);
		egisfp_platformfree(egis_dev);
	}
	mutex_unlock(&device_list_lock);
	return 0;
}
int egisfp_platformfree(struct egisfp_dev_t *egis_dev)
{
	int status = 0;
	struct egisfp_ioctl_cmd_t ioctl_data = {0};
	DEBUG_PRINT(" %s : enter \n", __func__);
	if (egis_dev->platforminit_done != 1)
		return status;
	if (egis_dev != NULL)
	{
		if (egis_dev->request_irq_done == 1)
		{
			if (egis_dev->fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE)
			{
				disable_irq_nosync(egis_dev->gpio_irq);
				egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
			}
			free_irq(egis_dev->gpio_irq, egis_dev);
			egis_dev->request_irq_done = 0;
		}
		if (egis_dev->clk_enabled)
		{
			spi_clk_enable(egis_dev, 0);
			set_egisfp_spi_pin_active(egis_dev, 0);
		}
		status = do_egisfp_reset_set(egis_dev, 0);

		if (egis_dev->power_enable && egis_dev->ctrl_power)
			status = do_egisfp_power_onoff(egis_dev, &ioctl_data);

		gpio_free(egis_dev->irqPin);
		gpio_free(egis_dev->rstPin);
		if (egis_dev->pwr_by_gpio)
			gpio_free(egis_dev->vcc_33v_Pin);

		if (egis_dev->vcc)
		{
			DEBUG_PRINT(" %s : devm_regulator_put \n", __func__);
			devm_regulator_put(egis_dev->vcc);
			egis_dev->vcc = NULL;
		}


		if (egis_dev->pinctrl)
		{
			DEBUG_PRINT(" %s : devm_pinctrl_put \n", __func__);
			devm_pinctrl_put(egis_dev->pinctrl);
			egis_dev->pinctrl = NULL;
		}
		spi_unregister_driver(&egisfp_spi_driver);
	}
	egis_dev->platforminit_done = 0;
	DEBUG_PRINT(" %s : successful status = %d \n", __func__, status);
	return status;
}

int egisfp_platforminit(struct egisfp_dev_t *egis_dev)
{
	int status;
	INFO_PRINT(" %s : Version %s \n", __func__, DRIVER_VERSION);
	if (egis_dev != NULL)
	{
		if (!egis_dev->platforminit_done)
		{
			status = spi_register_driver(&egisfp_spi_driver);
			if (status)
			{
				ERROR_PRINT(" %s : spi_register_driver failed \n", __func__);
				goto egisfp_spidriver_register_fail;
			}
			if (egis_dev->ctrl_power)
			{
				if (egis_dev->pwr_by_gpio)
				{
					/* initial 33V power pin */
					status = gpio_request(egis_dev->vcc_33v_Pin, "egis_dev-33v-gpio");
					if (status < 0)
					{
						ERROR_PRINT(" %s : gpio_requset vcc_33v_Pin pin failed \n", __func__);
						goto egisfp_power_request_fail;
					}

					status = gpio_direction_output(egis_dev->vcc_33v_Pin, 0);
				}
				else
				{
					egis_dev->vcc = devm_regulator_get(&egis_dev->dd->dev, "vcc_fp");
					if (IS_ERR(egis_dev->vcc))
					{
						status = PTR_ERR(egis_dev->vcc);
						ERROR_PRINT(" %s : get vcc regulator failed %d \n", __func__, status);
						goto egisfp_power_request_fail;
					}

					if (regulator_count_voltages(egis_dev->vcc) > 0)
					{
						status = regulator_set_voltage(egis_dev->vcc, egis_dev->regulator_voltage_min, egis_dev->regulator_voltage_max);
						if (status)
						{
							ERROR_PRINT(" %s : set vcc regulator voltage failed %d \n", __func__, status);
							goto egisfp_power_setup_fail;
						}

						status = regulator_set_load(egis_dev->vcc, egis_dev->regulator_current);
						if (status)
						{
							ERROR_PRINT(" %s : set vcc regulator current failed %d \n", __func__, status);
							goto egisfp_power_setup_fail;
						}
					}
				}
			}

			/* Initial Reset Pin */
			status = gpio_request(egis_dev->rstPin, "egis_dev-reset-gpio");
			if (status < 0)
			{
				ERROR_PRINT(" %s : gpio_requset reset pin failed \n", __func__);
				goto egisfp_reset_request_fail;
			}

			status = gpio_direction_output(egis_dev->rstPin, 0);

			status = gpio_request(egis_dev->irqPin, "egis_dev-irq-gpio");
			if (status < 0)
			{
				ERROR_PRINT(" %s : gpio_requset irq pin failed \n", __func__);
				goto egisfp_irq_request_fail;
			}

			gpio_direction_input(egis_dev->irqPin);

			if (egis_dev->dd)
			{
				INFO_PRINT(" %s : find node enter \n", __func__);
				egis_dev->pinctrl = devm_pinctrl_get(&egis_dev->dd->dev);
				if (IS_ERR(egis_dev->pinctrl))
				{
					status = PTR_ERR(egis_dev->pinctrl);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				egis_dev->reset_high = pinctrl_lookup_state(egis_dev->pinctrl, "egis_rst_high");
				if (IS_ERR(egis_dev->reset_high))
				{
					status = PTR_ERR(egis_dev->reset_high);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_rst_high \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				egis_dev->reset_low = pinctrl_lookup_state(egis_dev->pinctrl, "egis_rst_low");
				if (IS_ERR(egis_dev->reset_low))
				{
					status = PTR_ERR(egis_dev->reset_low);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_rst_low \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				if (pinctrl_select_state(egis_dev->pinctrl, egis_dev->reset_low))
					goto egisfp_pinctrl_fail;
				egis_dev->irq_active = pinctrl_lookup_state(egis_dev->pinctrl, "egis_irq_active");
				if (IS_ERR(egis_dev->irq_active))
				{
					status = PTR_ERR(egis_dev->irq_active);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_irq_active \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				if (pinctrl_select_state(egis_dev->pinctrl, egis_dev->irq_active))
					goto egisfp_pinctrl_fail;
				if (egis_dev->pwr_by_gpio && egis_dev->ctrl_power)
				{
					egis_dev->vcc_high = pinctrl_lookup_state(egis_dev->pinctrl, "egis_vcc_high");
					if (IS_ERR(egis_dev->vcc_high))
					{
						status = PTR_ERR(egis_dev->vcc_high);
						ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_vcc_high \n", __func__);
						goto egisfp_pinctrl_fail;
					}
					egis_dev->vcc_low = pinctrl_lookup_state(egis_dev->pinctrl, "egis_vcc_low");
					if (IS_ERR(egis_dev->vcc_low))
					{
						status = PTR_ERR(egis_dev->vcc_low);
						ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_vcc_low \n", __func__);
						goto egisfp_pinctrl_fail;
					}
					if (pinctrl_select_state(egis_dev->pinctrl, egis_dev->vcc_low))
						goto egisfp_pinctrl_fail;
				}
			}

			if (egis_dev->dd)
			{
				INFO_PRINT(" %s : find node enter \n", __func__);
				if (!egis_dev->pinctrl)
				{
					egis_dev->pinctrl = devm_pinctrl_get(&egis_dev->dd->dev);
					if (IS_ERR(egis_dev->pinctrl))
					{
						status = PTR_ERR(egis_dev->pinctrl);
						ERROR_PRINT(" %s : can't find fingerprint pinctrl \n", __func__);
						goto egisfp_pinctrl_fail;
					}
				}
				egis_dev->spi_active = pinctrl_lookup_state(egis_dev->pinctrl, "egis_spi_active");
				if (IS_ERR(egis_dev->spi_active))
				{
					status = PTR_ERR(egis_dev->spi_active);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_spi_active \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				egis_dev->spi_default = pinctrl_lookup_state(egis_dev->pinctrl, "egis_spi_default");
				if (IS_ERR(egis_dev->spi_default))
				{
					status = PTR_ERR(egis_dev->spi_default);
					ERROR_PRINT(" %s : can't find fingerprint pinctrl egis_spi_default \n", __func__);
					goto egisfp_pinctrl_fail;
				}
				if (pinctrl_select_state(egis_dev->pinctrl, egis_dev->spi_default))
					goto egisfp_pinctrl_fail;
			}


			egis_dev->platforminit_done = 1;
			if (egis_dev->pwr_by_gpio && egis_dev->ctrl_power)
				INFO_PRINT(" %s : successful status = %d gpio num -> vcc = %d rst = %d Irq = %d \n", __func__, status, egis_dev->vcc_33v_Pin, egis_dev->rstPin, egis_dev->irqPin);
			else
				INFO_PRINT(" %s : successful status = %d gpio num -> rst = %d Irq = %d \n", __func__, status, egis_dev->rstPin, egis_dev->irqPin);
		}
		else
		{
			INFO_PRINT(" %s : platform already init \n", __func__);
		}
		return 0;
	}
	else
	{
		ERROR_PRINT(" %s : device node is null \n", __func__);
		return -ENODEV;
	}
egisfp_pinctrl_fail:
	ERROR_PRINT(" %s : devm_pinctrl_put \n", __func__);
	devm_pinctrl_put(egis_dev->pinctrl);
	egis_dev->pinctrl = NULL;
	gpio_free(egis_dev->irqPin);
egisfp_irq_request_fail:
	gpio_free(egis_dev->rstPin);
egisfp_reset_request_fail:
egisfp_power_setup_fail:
	if (egis_dev->ctrl_power)
	{
		if (egis_dev->pwr_by_gpio)
			gpio_free(egis_dev->vcc_33v_Pin);
		else
			devm_regulator_put(egis_dev->vcc);
	}
egisfp_power_request_fail:
	spi_unregister_driver(&egisfp_spi_driver);
egisfp_spidriver_register_fail:
	return -EIO;
}

int egisfp_check_ioctl_permission(struct egisfp_dev_t *egis_dev, unsigned int cmd)
{

	if (cmd == FP_GET_RESOURCE || cmd == FP_FREE_RESOURCE)
	{
		return 0;
	}
	else if (egis_dev->platforminit_done)
	{
		return 0;
	}
	else
	{
		ERROR_PRINT(" %s : need get resource fisrt \n", __func__);
		return -EACCES;
	}
}

int egisfp_parse_dt(struct egisfp_dev_t *egis_dev)
{
	int ret;
	struct device_node *node = egis_dev->dd->dev.of_node;
	u32 voltage_supply[2];
	u32 current_supply;
	INFO_PRINT(" %s : start \n", __func__);

	if (node)
	{
		egis_dev->ctrl_power = of_property_read_bool(node, "fp-ctrl-power");
		if (egis_dev->ctrl_power)
		{
			egis_dev->pwr_by_gpio = of_property_read_bool(node, "fp-gpio-vcc-enable");
			if (egis_dev->pwr_by_gpio)
			{
				egis_dev->vcc_33v_Pin = of_get_named_gpio(node, "egistec,gpio_vcc_en", 0);
				INFO_PRINT(" %s : vcc_33v_pin gpio num is %d \n", __func__, egis_dev->vcc_33v_Pin);
				if (!gpio_is_valid(egis_dev->vcc_33v_Pin))
				{
					ERROR_PRINT(" %s : vcc_33v_pin gpio is invalid \n", __func__);
					return -ENODEV;
				}
			}
			else
			{
				ret = of_property_read_u32_array(node, "egis-fp,vcc-voltage", voltage_supply, 2);
				if (ret < 0)
				{
					ERROR_PRINT(" %s : fail to get vcc regulator voltage \n", __func__);
					return -ENODEV;
				}
				INFO_PRINT(" %s : vcc regulator voltage get Max = %d, Min = %d \n", __func__, voltage_supply[1], voltage_supply[0]);
				egis_dev->regulator_voltage_max = voltage_supply[1];
				egis_dev->regulator_voltage_min = voltage_supply[0];

				ret = of_property_read_u32_array(node, "egis-fp,vcc-current", &current_supply, 1);
				if (ret < 0)
				{
					ERROR_PRINT("  %s : fail to get vcc regulator current_supply \n", __func__);
					return -ENODEV;
				}
				INFO_PRINT(" %s : vcc regulator current get %d \n", __func__, current_supply);
				egis_dev->regulator_current = current_supply;
			}
		}

		egis_dev->rstPin = of_get_named_gpio(node, "egistec,gpio_reset", 0);
		INFO_PRINT(" %s : rst pin gpio num is %d \n", __func__, egis_dev->rstPin);
		if (!gpio_is_valid(egis_dev->rstPin))
		{
			ERROR_PRINT(" %s : rst pin gpio is invalid \n", __func__);
			return -ENODEV;
		}

		egis_dev->irqPin = of_get_named_gpio(node, "egistec,gpio_irq", 0);
		INFO_PRINT(" %s : irq pin gpio num is %d \n", __func__, egis_dev->irqPin);
		if (!gpio_is_valid(egis_dev->irqPin))
		{
			ERROR_PRINT(" %s : irq pin gpio is invalid \n", __func__);
			return -ENODEV;
		}
	}
	else
	{
		ERROR_PRINT(" %s : device node is null \n", __func__);
		return -ENODEV;
	}
	INFO_PRINT(" %s : successful \n", __func__);

	return 0;
}

const struct file_operations egisfp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = egisfp_ioctl,
	.compat_ioctl = egisfp_compat_ioctl,
	.open = egisfp_open,
	.release = egisfp_release,
	.llseek = no_llseek,
	.poll = egisfp_interrupt_poll};

/*-------------------------------------------------------------------------*/
struct class *egisfp_class;
/*-------------------------------------------------------------------------*/

int egisfp_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct egisfp_dev_t *egis_dev = dev_get_drvdata(dev);
	INFO_PRINT(" %s : driver remove \n", __func__);
	if (egis_dev->request_irq_done)
	{
		free_irq(egis_dev->gpio_irq, egis_dev);
	}
	del_timer_sync(&egis_dev->fps_ints.timer);

	device_init_wakeup(&egis_dev->dd->dev, 0);

	egis_dev->request_irq_done = 0;

	if (egis_dev->input_dev)
	{
		input_unregister_device(egis_dev->input_dev);
	}

	device_destroy(egisfp_class, egis_dev->devt);

	list_del(&egis_dev->device_entry);

	class_destroy(egisfp_class);

	unregister_chrdev(EGIS_FP_MAJOR, egisfp_driver.driver.name);

	g_data = NULL;
	return 0;
}

int egisfp_probe(struct platform_device *pdev)
{
	struct egisfp_dev_t *egis_dev;
	int status, i;
	unsigned long minor;

	INFO_PRINT(" %s : driver init \n", __func__);
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(EGIS_FP_MAJOR, EGIS_CHRD_DRIVER_NAME, &egisfp_fops);
	if (status < 0)
	{
		ERROR_PRINT(" %s : register_chrdev error \n", __func__);
		return status;
	}
	egisfp_class = class_create(THIS_MODULE, EGIS_CLASS_NAME);
	if (IS_ERR(egisfp_class))
	{
		ERROR_PRINT(" %s : class_create error \n", __func__);
		unregister_chrdev(EGIS_FP_MAJOR, egisfp_driver.driver.name);
		return PTR_ERR(egisfp_class);
	}
	/* Allocate driver data */
	egis_dev = kzalloc(sizeof(struct egisfp_dev_t), GFP_KERNEL);
	if (egis_dev == NULL)
	{
		ERROR_PRINT(" %s : Failed to kzalloc \n", __func__);
		return -ENOMEM;
	}
/* Initialize the driver data */
	dev_set_drvdata(&pdev->dev, egis_dev);
	egis_dev->dd = pdev;


	spin_lock_init(&egis_dev->irq_lock);

	device_init_wakeup(&egis_dev->dd->dev, 1);

	mutex_init(&device_list_lock);

	INIT_LIST_HEAD(&egis_dev->device_entry);

	/* init status */
	egis_dev->pars_dtsi_done = 0;
	egis_dev->fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	egis_dev->clk_enabled = 0;
	egis_dev->request_irq_done = 0;
	egis_dev->platforminit_done = 0;
	egis_dev->power_enable = 0;
	egis_dev->screen_onoff = 1;	// dafault set screen on
	egis_dev->call_back_registered = 0;
	egis_dev->fps_ints.irq_mode = -1;
	egis_dev->spi = NULL;
	/*
	 * If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS)
	{
		struct device *fdev;
		egis_dev->devt = MKDEV(EGIS_FP_MAJOR, minor);
		fdev = device_create(egisfp_class, &egis_dev->dd->dev, egis_dev->devt,
							 egis_dev, EGIS_DEV_NAME);
		status = IS_ERR(fdev) ? PTR_ERR(fdev) : 0;
	}
	else
	{
		ERROR_PRINT(" %s : no minor number available \n", __func__);
		status = -ENODEV;
	}
	if (status == 0)
	{
		set_bit(minor, minors);
		list_add(&egis_dev->device_entry, &device_list);
	}

	mutex_unlock(&device_list_lock);

	if (status)
	{
		goto egistec_probe_failed;
	}

	egis_dev->input_dev = input_allocate_device();
	if (egis_dev->input_dev == NULL)
	{
		ERROR_PRINT(" %s : failed to allocate input device \n", __func__);
		status = -ENOMEM;
		goto egistec_probe_failed;
	}
	for (i = 0; i < ARRAY_SIZE(key_maps); i++)
		input_set_capability(egis_dev->input_dev, key_maps[i].type, key_maps[i].code);

	egis_dev->input_dev->name = EGIS_INPUT_NAME;
	status = input_register_device(egis_dev->input_dev);
	if (status)
	{
		ERROR_PRINT(" %s : failed to register input device %d \n", __func__, status);
		goto egistec_input_failed;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&egis_dev->fps_ints.timer, egisfp_interrupt_timer_call, 0);
#else
	setup_timer(&egis_dev->fps_ints.timer, egisfp_interrupt_timer_call, (unsigned long)&egis_dev->fps_ints);
#endif


	g_data = egis_dev;

	DEBUG_PRINT(" %s : initialize success %d\n", __func__, status);

	return status;

egistec_input_failed:
	if (egis_dev->input_dev != NULL)
		input_free_device(egis_dev->input_dev);
egistec_probe_failed:
	device_destroy(egisfp_class, egis_dev->devt);
	class_destroy(egisfp_class);
	kfree(egis_dev);
	ERROR_PRINT(" %s : driver probe failed %d \n", __func__, status);
	return status;
}

int __init egisfp_init(void)
{
	int status;
	INFO_PRINT(" %s : module init \n", __func__);
	status = platform_driver_register(&egisfp_driver);
	if (status)
	{
		ERROR_PRINT(" %s : register Egis driver fail \n", __func__);
		return -EINVAL;
	}
	INFO_PRINT(" %s : module init OK ! \n", __func__);
	return status;
}
void __exit egisfp_exit(void)
{
	INFO_PRINT("module exit \n");
	platform_driver_unregister(&egisfp_driver);
}

late_initcall(egisfp_init);
module_exit(egisfp_exit);

MODULE_DESCRIPTION("egis fingerprint driver");
MODULE_LICENSE("GPL");
