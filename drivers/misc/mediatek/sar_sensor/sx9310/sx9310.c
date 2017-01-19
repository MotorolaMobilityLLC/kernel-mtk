/*!file sx9310.c
 *brief  SX9310 Driver
 *
 * Driver for the SX9310
 * Copyright (c) 2011 Semtech Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>

#include <linux/device.h>
#include <linux/interrupt.h>

#include <mach/irqs.h>
#include <linux/kthread.h>
/* #include <linux/rtpm_prio.h> */
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>

#define DEBUG
#define DRIVER_NAME "sx9310"

#define MAX_WRITE_ARRAY_SIZE 32
int sar_debug_en = 1;

#include "sx9310.h"		/* main struct, interrupt,init,pointers */

#define IDLE 0
#define ACTIVE 1

/*! struct sx9310
 * Specialized struct containing input event data, platform data, and
 * last cap state read if needed.
 */

static struct _buttonInfo psmtcButtons[] = {
	{
	 .keycode = KEY_SAR,
	 .mask = SX9310_TCHCMPSTAT_TCHSTAT0_FLAG,
	 },
};

/* Define Registers that need to be initialized to values different than
 * default
 */
static struct smtc_reg_data sx9310_i2c_reg_setup[] = {
	{
	 .reg = SX9310_IRQ_ENABLE_REG,
	 .val = 0x70,
	 },
	{
	 .reg = SX9310_IRQFUNC_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL1_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL2_REG,
	 .val = 0x04,
	 },
	{
	 .reg = SX9310_CPS_CTRL3_REG,
	 .val = 0x0A,
	 },
	{
	 .reg = SX9310_CPS_CTRL4_REG,
	 .val = 0x0D,
	 },
	{
	 .reg = SX9310_CPS_CTRL5_REG,
	 .val = 0xC1,
	 },
	{
	 .reg = SX9310_CPS_CTRL6_REG,
	 .val = 0x20,
	 },
	{
	 .reg = SX9310_CPS_CTRL7_REG,
	 .val = 0x4C,
	 },
	{
	 .reg = SX9310_CPS_CTRL8_REG,
	 .val = 0x78,
	 },
	{
	 .reg = SX9310_CPS_CTRL9_REG,
	 .val = 0x7D,
	 },
	{
	 .reg = SX9310_CPS_CTRL10_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL11_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL12_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL13_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL14_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL15_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL16_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL17_REG,
	 .val = 0x04,
	 },
	{
	 .reg = SX9310_CPS_CTRL18_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_CPS_CTRL19_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_SAR_CTRL0_REG,
	 .val = 0x00,
	 },
	{
	 .reg = SX9310_SAR_CTRL1_REG,
	 .val = 0x80,
	 },
	{
	 .reg = SX9310_SAR_CTRL2_REG,
	 .val = 0x0C,
	 },
	{
	 .reg = SX9310_CPS_CTRL0_REG,
	 .val = 0x57,
	 },
};

struct input_dev *sx9310_input_device;
unsigned int sar_sensor_irq = 0;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
unsigned int sar_sensor_flag = 0;
struct task_struct *thread_sar_sensor;
struct sx9310_data *psx9310_data = 0;

static int sx9310_irq_registration(void);

static void ForcetoTouched(struct sx9310_data *this)
{
	struct input_dev *input = NULL;
	struct _buttonInfo *pCurrentButton = NULL;

	if (this && (sx9310_input_device)) {
		SAR_INFO("ForcetoTouched()\n");
		pCurrentButton = psmtcButtons;
		input = sx9310_input_device;
		input_report_key(input, pCurrentButton->keycode, 1);
		pCurrentButton->state = ACTIVE;
		input_sync(input);
		SAR_INFO("Leaving ForcetoTouched()\n");
	}
}

/*! fn static int write_register(struct sx9310_data * this, u8 address, u8 value)
 *  brief Sends a write register to the device
 *  param this Pointer to main parent struct
 *  param address 8-bit register address
 *  param value   8-bit register value to write to address
 *  return Value from i2c_master_send
 */
static int write_register(struct sx9310_data *this, u8 address, u8 value)
{
	struct i2c_client *i2c = 0;
	char buffer[2];
	int returnValue = 0;

	buffer[0] = address;
	buffer[1] = value;
	returnValue = -ENOMEM;
	if (this && this->bus) {
		i2c = this->bus;
		returnValue = i2c_master_send(i2c, buffer, 2);
		SAR_INFO("write_register Address: 0x%x Value: 0x%x Return: %d\n", address, value, returnValue);
	}
	if (returnValue < 0) {
		ForcetoTouched(this);
		SAR_INFO("Write_register-ForcetoTouched()\n");
	}
	return returnValue;
}

/*! fn static int read_register(struct sx9310_data * this, u8 address, u8 *value)
* brief Reads a register's value from the device
* param this Pointer to main parent struct
* param address 8-Bit address to read from
* param value Pointer to 8-bit value to save register value to
* return Value from i2c_smbus_read_byte_data if < 0. else 0
*/
static int read_register(struct sx9310_data *this, u8 address, u8 *value)
{
	struct i2c_client *i2c = 0;
	s32 returnValue = 0;

	if (this && value && this->bus) {
		i2c = this->bus;
		returnValue = i2c_smbus_read_byte_data(i2c, address);
		SAR_INFO("read_register Address: 0x%x Return: 0x%x\n", address, returnValue);
		if (returnValue >= 0) {
			*value = returnValue;
			return 0;
		} else {
			return returnValue;
		}
	}
	ForcetoTouched(this);
	SAR_INFO("read_register-ForcetoTouched()\n");
	return -ENOMEM;
}

/*********************************************************************/
/*!brief Perform a manual offset calibration
*param this Pointer to main parent struct
*return Value return value from the write register
 */
static int manual_offset_calibration(struct sx9310_data *this)
{
	s32 returnValue = 0;

	returnValue = write_register(this, SX9310_IRQSTAT_REG, 0xFF);
	return returnValue;
}

/*!brief sysfs show function for manual calibration which currently just
 * returns register value.
 */
static ssize_t manual_offset_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 reg_value = 0;
	struct sx9310_data *this = dev_get_drvdata(dev);

	SAR_INFO("Reading IRQSTAT_REG\n");
	read_register(this, SX9310_IRQSTAT_REG, &reg_value);
	return sprintf(buf, "%d\n", reg_value);
}

/*!brief sysfs store function for manual calibration
 */
static ssize_t manual_offset_calibration_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sx9310_data *this = dev_get_drvdata(dev);
	/* unsigned long val; */
	/* if (val) { */
	SAR_INFO("Performing manual_offset_calibration()\n");
	manual_offset_calibration(this);
	/* } */
	return count;
}

static DEVICE_ATTR(calibrate, 0660, manual_offset_calibration_show, manual_offset_calibration_store);
static struct attribute *sx9310_attributes[] = { &dev_attr_calibrate.attr, NULL,
};

static struct attribute_group sx9310_attr_group = {.attrs = sx9310_attributes,
};

/*********************************************************************/

/*!fn static int read_regStat(struct sx9310_data * this)
 *brief Shortcut to read what caused interrupt.
 *details This is to keep the drivers a unified
 * function that will read whatever register(s)
 * provide information on why the interrupt was caused.
 *param this Pointer to main parent struct
 *return If successful, Value of bit(s) that cause interrupt, else 0
 */
static int read_regStat(struct sx9310_data *this)
{
	u8 data = 0;

	if (this) {
		if (read_register(this, SX9310_IRQSTAT_REG, &data) == 0)
			return (data & 0x00FF);
	}
	return 0;
}

/*!brief  Initialize I2C config from platform data
 *param this Pointer to main parent struct
 */
static void hw_init(struct sx9310_data *this)
{
	int i = 0;

	/* configure device */
	SAR_INFO("Going to Setup I2C Registers\n");
	if (this) {
		while (i < ARRAY_SIZE(sx9310_i2c_reg_setup)) {

			/* Write all registers/values contained in i2c_reg */
			SAR_INFO("Going to Write Reg: 0x%x Value: 0x%x\n", sx9310_i2c_reg_setup[i].reg, sx9310_i2c_reg_setup[i].val);

/* msleep(3); */
			write_register(this, sx9310_i2c_reg_setup[i].reg, sx9310_i2c_reg_setup[i].val);
			i++;
		}
	} else {

		/* Force to touched if error */
		ForcetoTouched(this);
		SAR_INFO("Hardware_init-ForcetoTouched()\n");
	}
}

/*********************************************************************/

/*!fn static int initialize(struct sx9310_data * this)
 *brief Performs all initialization needed to configure the device
 *param this Pointer to main parent struct
 *return Last used command's return value (negative if error)
 */
static int initialize(struct sx9310_data *this)
{
	if (this) {

		/* prepare reset by disabling any irq handling */
		this->irq_disabled = 1;
		disable_irq(sar_sensor_irq);

		/* perform a reset */
		write_register(this, SX9310_SOFTRESET_REG, SX9310_SOFTRESET);

		/* wait until the reset has finished by monitoring NIRQ */
		SAR_INFO("Sent Software Reset. Waiting until device is back from reset to continue.\n");

		/* just sleep for awhile instead of using a loop with reading irq status */
		msleep(300);

		hw_init(this);
		msleep(100);	/* make sure everything is running */
		manual_offset_calibration(this);

		/* re-enable interrupt handling */
		enable_irq(sar_sensor_irq);
		this->irq_disabled = 0;

		/* make sure no interrupts are pending since enabling irq will only
		 * work on next falling edge */
		read_regStat(this);
		SAR_INFO("Exiting initialize().\n");
		return 0;
	}
	return -ENOMEM;
}

/*!
 *brief Handle what to do when a touch occurs
 *param this Pointer to main parent struct
 */
static void touchProcess(struct sx9310_data *this)
{
	int counter = 0;
	u8 i = 0;
	int numberOfButtons = 0;
	struct _buttonInfo *buttons = NULL;
	struct input_dev *input = NULL;
	struct _buttonInfo *pCurrentButton = NULL;

	if (this) {
		SAR_INFO("Inside touchProcess()\n");
		read_register(this, SX9310_STAT0_REG, &i);
		buttons = psmtcButtons;
		input = sx9310_input_device;
		numberOfButtons = ARRAY_SIZE(psmtcButtons);
		if (unlikely((buttons == NULL) || (input == NULL))) {
			SAR_ERROR("ERROR!! buttons or input NULL!!!\n");
			return;
		}
		for (counter = 0; counter < numberOfButtons; counter++) {
			pCurrentButton = &buttons[counter];
			if (pCurrentButton == NULL) {
				SAR_ERROR("ERROR!! current button at index: %d NULL!!!\n", counter);
				return;	/* ERRORR!!!! */
			}
			switch (pCurrentButton->state) {
			case IDLE:	/* Button is not being touched! */
				if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {

					/* User pressed button */
					SAR_INFO("cap button %d touched\n", counter);
					input_report_key(input, pCurrentButton->keycode, 1);
					pCurrentButton->state = ACTIVE;
				} else {
					SAR_INFO("Button %d already released.\n", counter);
				}
				break;
			case ACTIVE:	/* Button is being touched! */
				if (((i & pCurrentButton->mask) != pCurrentButton->mask)) {

					/* User released button */
					SAR_INFO("cap button %d released\n", counter);
					input_report_key(input, pCurrentButton->keycode, 0);
					pCurrentButton->state = IDLE;
				} else {
					SAR_INFO("Button %d still touched.\n", counter);
				}
				break;
			default:	/* Shouldn't be here, device only allowed ACTIVE or IDLE */
				break;
			};
		}
		input_sync(input);
		SAR_INFO("Leaving touchProcess()\n");
	}
}

void sx9310_suspend(struct sx9310_data *this)
{
	if (this)
		disable_irq(sar_sensor_irq);
}

void sx9310_resume(struct sx9310_data *this)
{
	if (this) {

		if (this->init)
			this->init(this);
		enable_irq(sar_sensor_irq);
	}
}

static irqreturn_t sx9310_eint_interrupt_handler(int irq, void *dev_id)
{
	sar_sensor_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
}

/*****************************************************************************
*  Name: sp9310_event_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int sx9310_event_handler(void *unused)
{
	int status = 0;
	int counter = 0;
	struct sched_param param = {.sched_priority = 4 };	/* RTPM_PRIO_TPD */

	sched_setscheduler(current, SCHED_RR, &param);

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, sar_sensor_flag != 0);

		sar_sensor_flag = 0;

		set_current_state(TASK_RUNNING);

		status = psx9310_data->refreshStatus(psx9310_data);
		counter = -1;
		SAR_INFO("Worker - Refresh Status %d\n", status);
		while ((++counter) < MAX_NUM_STATUS_BITS) {	/* counter start from MSB */
			SAR_INFO("Looping Counter %d\n", counter);
			if (((status >> counter) & 0x01) && (psx9310_data->statusFunc[counter])) {
				SAR_INFO("Function Pointer Found. Calling\n");
				psx9310_data->statusFunc[counter] (psx9310_data);
			}
		}

	} while (!kthread_should_stop());

	return 0;
}

int sx9310_init(struct sx9310_data *this)
{

	if (this) {

		/* initialize spin lock */
		spin_lock_init(&this->lock);
		sx9310_irq_registration();

		if (this->init)
			return this->init(this);
		SAR_ERROR("No init function!!!!\n");
	}
	return -ENOMEM;
}

int sx9310_remove(struct sx9310_data *this)
{
	if (this) {

		kfree(this);
		return 0;
	}
	return -ENOMEM;
}

/*!fn static int sx9310_probe(struct i2c_client *client, const struct i2c_device_id *id)
 *brief Probe function
 *param client pointer to i2c_client
 *param id pointer to i2c_device_id
 *return Whether probe was successful
 */
static int sx9310_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i = 0;
	int retval = 0;
	struct input_dev *input = NULL;

	SAR_INFO("sx9310_probe()\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;
	psx9310_data = kzalloc(sizeof(struct sx9310_data), GFP_KERNEL);	/* create memory for main struct */
	SAR_INFO("\t Initialized Main Memory: 0x%p\n", psx9310_data);
	if (psx9310_data == NULL) {
		SAR_ERROR("\t sx9310_data kzalloc Fail\n");
		return -1;
	}

	/* In case we need to reinitialize data
	 * (e.q. if suspend reset device) */
	psx9310_data->init = initialize;

	/* shortcut to read status of interrupt */
	psx9310_data->refreshStatus = read_regStat;

	/* pointer to function from platform data to get pendown
	 * (1->NIRQ=0, 0->NIRQ=1) */
	/* psx9310_data->get_nirq_low = pplatData->get_is_nirq_low; */

	/* save irq in case we need to reference it */
	psx9310_data->irq = client->irq;


	/* Setup function to call on corresponding reg irq source bit */
	if (MAX_NUM_STATUS_BITS >= 8) {
		psx9310_data->statusFunc[0] = 0;	/* TXEN_STAT */
		psx9310_data->statusFunc[1] = 0;	/* UNUSED */
		psx9310_data->statusFunc[2] = 0;	/* UNUSED */
		psx9310_data->statusFunc[3] = 0;	/* CONV_STAT */
		psx9310_data->statusFunc[4] = 0;	/* COMP_STAT */
		psx9310_data->statusFunc[5] = touchProcess;	/* RELEASE_STAT */
		psx9310_data->statusFunc[6] = touchProcess;	/* TOUCH_STAT  */
		psx9310_data->statusFunc[7] = 0;	/* RESET_STAT */
	}

	/* setup i2c communication */
	psx9310_data->bus = client;
	i2c_set_clientdata(client, psx9310_data);

	/* record device struct */
	psx9310_data->pdev = &client->dev;

	/* for accessing items in user data (e.g. calibrate) */
	if (sysfs_create_group(&client->dev.kobj, &sx9310_attr_group) != 0)
		return -ENOMEM;

	/* Create the input device */
	input = input_allocate_device();
	if (!input)
		return -ENOMEM;

	/* Set all the keycodes */
	__set_bit(EV_KEY, input->evbit);
	for (i = 0; i < ARRAY_SIZE(psmtcButtons); i++) {
		__set_bit(psmtcButtons[i].keycode, input->keybit);
		psmtcButtons[i].state = IDLE;
	}

	/* save the input pointer and finish initialization */
	sx9310_input_device = input;
	input->name = "SX9310 Sar Sensor";
	input->id.bustype = BUS_I2C;
	if (input_register_device(input)) {
		SAR_ERROR("[TPD]Failed to create sx9310 input device!");
		return -ENOMEM;
	}

	thread_sar_sensor = kthread_run(sx9310_event_handler, 0, DRIVER_NAME);
	if (IS_ERR(thread_sar_sensor)) {
		retval = PTR_ERR(thread_sar_sensor);
		SAR_ERROR("[TPD]Failed to create kernel thread_sar_sensor,ret=%d!", retval);
	}

	sx9310_init(psx9310_data);
	return 0;

}

/*!fn static int sx9310_remove(struct i2c_client *client)
 *brief Called when device is to be removed
 *param client Pointer to i2c_client struct
 *return Value from sx9310_i2c_remove()
 */
static int sx9310_i2c_remove(struct i2c_client *client)
{
	struct sx9310_data *this = i2c_get_clientdata(client);

	if (this) {
		input_unregister_device(sx9310_input_device);
		sysfs_remove_group(&client->dev.kobj, &sx9310_attr_group);
	}
	return sx9310_remove(this);
}


/*====================================================*/
/***** Kernel Suspend *****/
static int sx9310_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct sx9310_data *this = i2c_get_clientdata(client);
	SAR_FUNC_ENTER();
	sx9310_suspend(this);
	SAR_FUNC_EXIT();
	return 0;
}

/***** Kernel Resume *****/
static int sx9310_i2c_resume(struct i2c_client *client)
{
	struct sx9310_data *this = i2c_get_clientdata(client);

	SAR_FUNC_ENTER();
	sx9310_resume(this);
	SAR_FUNC_EXIT();
	return 0;
}

/*====================================================*/
static const struct i2c_device_id sx9310_i2c_id[] = { {DRIVER_NAME, 0}, {} };

static const struct of_device_id sx9310_dt_match[] = {
	{.compatible = "mediatek,sarsensor"}, {},
};

MODULE_DEVICE_TABLE(of, sx9310_dt_match);
static struct i2c_driver sx9310_i2c_driver = {
	.driver = {
		   .name = DRIVER_NAME,
		   .of_match_table = of_match_ptr(sx9310_dt_match),
		   },
	.probe = sx9310_i2c_probe,
	.remove = sx9310_i2c_remove,
	.id_table = sx9310_i2c_id,
	.suspend = sx9310_i2c_suspend,
	.resume = sx9310_i2c_resume,
};


/*================sar sensor platform driver====================================*/
static const struct of_device_id sar_sensor_dt_match[] = {
	{.compatible = "mediatek,sar_sensor"}, {},
};

MODULE_DEVICE_TABLE(of, sar_sensor_dt_match);
static int sx9310_irq_registration(void)
{
	struct device_node *node = NULL;
	int ret = 0;
	u32 ints[2] = { 0, 0 };

	SAR_FUNC_ENTER();
	node = of_find_matching_node(node, sar_sensor_dt_match);
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);

		sar_sensor_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(sar_sensor_irq, sx9310_eint_interrupt_handler, IRQF_TRIGGER_FALLING, "Sar Sensor-eint", NULL);
		if (ret > 0)
			SAR_ERROR("sar sensor request_irq IRQ LINE NOT AVAILABLE!.");
		else
			SAR_INFO("IRQ request succussfully, irq=%d", sar_sensor_irq);
	} else {
		SAR_ERROR("Can not find touch eint device node!");
	}
	SAR_FUNC_EXIT();
	return 0;
}
static int sar_sensor_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	/*configure to GPIO function, external interrupt */
	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		SAR_ERROR("Cannot find alsps pinctrl!\n");
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		SAR_ERROR("Cannot find alsps pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		SAR_ERROR("Cannot find alsps pinctrl pin_cfg!\n");

	}
	pinctrl_select_state(pinctrl, pins_cfg);

	return i2c_add_driver(&sx9310_i2c_driver);
}

static int sar_sensor_remove(struct platform_device *pdev)
{
	SAR_FUNC_ENTER();
	i2c_del_driver(&sx9310_i2c_driver);
	SAR_FUNC_EXIT();
	return 0;
}





MODULE_DEVICE_TABLE(of, sx9310_dt_match);
const struct dev_pm_ops sar_sensor_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
};

static struct platform_driver sar_sensor_driver = {
	.remove = sar_sensor_remove,
	.shutdown = NULL,
	.probe = sar_sensor_probe,
	.driver = {
		   .name = "sar sensor",
		   .pm = &sar_sensor_pm_ops,
		   .owner = THIS_MODULE,
		   .of_match_table = sar_sensor_dt_match,
		   },
};

static int __init sx9310_platform_driver_init(void)
{
	if (platform_driver_register(&sar_sensor_driver) != 0)
		return -1;
	return 0;
}

static void __exit sx9310_platform_driver_exit(void)
{

	platform_driver_unregister(&sar_sensor_driver);
}

module_init(sx9310_platform_driver_init);

module_exit(sx9310_platform_driver_exit);
MODULE_AUTHOR("Semtech Corp. (http://www.semtech.com/)");
MODULE_DESCRIPTION("SX9310 Capacitive Touch Controller Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
