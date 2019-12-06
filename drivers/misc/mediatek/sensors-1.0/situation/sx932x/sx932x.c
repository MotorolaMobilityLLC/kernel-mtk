/*! \file sx932x.c
 * \brief  SX932x Driver
 *
 * Driver for the SX932x 
 * Copyright (c) 2011 Semtech Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
//#define DEBUG
#define DRIVER_NAME "sx932x"

#define MAX_WRITE_ARRAY_SIZE 32

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
//#include <linux/wakelock.h>
#include <linux/uaccess.h>
#include <linux/sort.h> 
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <situation.h>

#include "sx932x.h" 	/* main struct, interrupt,init,pointers */
#include "../sar/sar_factory.h"
/*****************************************************************************
 *** ontim gsensor hwinfo
 *****************************************************************************/
 #define ontim_debug_info
#ifdef ontim_debug_info
#include <ontim/ontim_dev_dgb.h>
static char sx932x_version[]="sx932x";
static char sx932x_vendor_name[20]="sx932x";
    DEV_ATTR_DECLARE(sar_sensor)
    DEV_ATTR_DEFINE("version",sx932x_version)
    DEV_ATTR_DEFINE("vendor",sx932x_vendor_name)
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(sar_sensor,sar_sensor,8);
#endif


#define IDLE			0
#define ACTIVE			1
#ifdef MULTILEVEL
#define BODY			2
#endif

#define SX932x_NIRQ		34

#define MAIN_SENSOR		1 //CS1

/* Failer Index */
#define SX932x_ID_ERROR 	1
#define SX932x_NIRQ_ERROR	2
#define SX932x_CONN_ERROR	3
#define SX932x_I2C_ERROR	4

#define PROXOFFSET_LOW			1500

#define SX932x_ANALOG_GAIN		1
#define SX932x_DIGITAL_GAIN		1
#define SX932x_ANALOG_RANGE		2.65

#define	TOUCH_CHECK_REF_AMB      0 // 44523
#define	TOUCH_CHECK_SLOPE        0 // 50
#define	TOUCH_CHECK_MAIN_AMB     0 // 151282

#define SX932x_GPIO_INT		9

#define SX932X_TAG					"[SX932X]"
#define SX932X_FUN(f)				pr_err(SX932X_TAG"[%s]\n", __FUNCTION__)
#define SX932X_ERR(fmt, args...)	pr_err(SX932X_TAG"[ERROR][%s][%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#define SX932X_LOG(fmt, args...)	pr_err(SX932X_TAG"[INFO][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)
//#define SX932X_DBG(fmt, args...)	ontim_dev_dbg(7, SX932X_TAG"[DEBUG][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)
#define SX932X_DBG(fmt, args...)	pr_err(SX932X_TAG"[DEBUG][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)


/*! \struct sx932x
 * Specialized struct containing input event data, platform data, and
 * last cap state read if needed.
 */
typedef struct sx932x
{
	pbuttonInformation_t pbuttonInformation;
	psx932x_platform_data_t hw;		/* specific platform data settings */
} sx932x_t, *psx932x_t;

static void sx93XX_schedule_work(psx93XX_t this, unsigned long delay);

static int irq_gpio_num;
static psx93XX_t g_this;
static int sx932x_init_flag = -1; // 0<==>OK -1 <==> fail

/*! \fn static int write_register(psx93XX_t this, u8 address, u8 value)
 * \brief Sends a write register to the device
 * \param this Pointer to main parent struct 
 * \param address 8-bit register address
 * \param value   8-bit register value to write to address
 * \return Value from i2c_master_send
 */
static int write_register(psx93XX_t this, u8 address, u8 value)
{
	struct i2c_client *i2c = 0;
	char buffer[2];
	int returnValue = 0;

	buffer[0] = address;
	buffer[1] = value;
	returnValue = -ENOMEM;

	if (this && this->bus) {
		i2c = this->bus;
		returnValue = i2c_master_send(i2c,buffer,2);
		#ifdef DEBUG
		SX932X_DBG("write_register Address: 0x%x Value: 0x%x Return: %d\n",
														address,value,returnValue);
		#endif
	}
	return returnValue;
}

/*! \fn static int read_register(psx93XX_t this, u8 address, u8 *value) 
* \brief Reads a register's value from the device
* \param this Pointer to main parent struct 
* \param address 8-Bit address to read from
* \param value Pointer to 8-bit value to save register value to 
* \return Value from i2c_smbus_read_byte_data if < 0. else 0
*/
static int read_register(psx93XX_t this, u8 address, u8 *value)
{
	struct i2c_client *i2c = 0;
	s32 returnValue = 0;

	if (this && value && this->bus) {
		i2c = this->bus;
		returnValue = i2c_smbus_read_byte_data(i2c,address);
		
		#ifdef DEBUG
		SX932X_DBG("read_register Address: 0x%x Return: 0x%x\n",
														address,returnValue);
		#endif

		if (returnValue >= 0) {
			*value = returnValue;
			return 0;
		} 
		else {
			return returnValue;
		}
	}
	return -ENOMEM;
}

//static int sx932x_set_mode(psx93XX_t this, unsigned char mode);

/*! \fn static int read_regStat(psx93XX_t this)
 * \brief Shortcut to read what caused interrupt.
 * \details This is to keep the drivers a unified
 * function that will read whatever register(s) 
 * provide information on why the interrupt was caused.
 * \param this Pointer to main parent struct 
 * \return If successful, Value of bit(s) that cause interrupt, else 0
 */
static int read_regStat(psx93XX_t this)
{
	u8 data = 0;
	if (this) {
		if (read_register(this,SX932x_IRQSTAT_REG,&data) == 0)
		return (data & 0x00FF);
	}
	return 0;
}

/*********************************************************************/
/*! \brief Perform a manual offset calibration
* \param this Pointer to main parent struct 
* \return Value return value from the write register
 */
static int manual_offset_calibration(psx93XX_t this)
{
	s32 returnValue = 0;
	returnValue = write_register(this,SX932x_STAT2_REG,0x0F);
	return returnValue;
}
/*! \brief sysfs show function for manual calibration which currently just
 * returns register value.
 */
static ssize_t manual_offset_calibration_show(struct device *dev,
								struct device_attribute *attr, char *buf)
{
	u8 reg_value = 0;
	psx93XX_t this = dev_get_drvdata(dev);

	SX932X_LOG("Reading IRQSTAT_REG\n");
	read_register(this,SX932x_IRQSTAT_REG,&reg_value);
	return sprintf(buf, "%d\n", reg_value);
}

/*! \brief sysfs store function for manual calibration
 */
static ssize_t manual_offset_calibration_store(struct device *dev,
			struct device_attribute *attr,const char *buf, size_t count)
{
	psx93XX_t this = dev_get_drvdata(dev);
	unsigned long val;
	if (kstrtoul(buf, 0, &val))
	return -EINVAL;
	if (val) {
		SX932X_LOG("Performing manual_offset_calibration()\n");
		manual_offset_calibration(this);
	}
	return count;
}

static int sx932x_Hardware_Check(psx93XX_t this)
{
	int ret;
	u8 failcode = 0;
	u8 loop = 0;
	this->failStatusCode = 0;
	
	//Check th IRQ Status
	while(this->get_nirq_low && this->get_nirq_low()){
		ret = read_regStat(this);
		SX932X_ERR("irq reg status:%d\n", ret);
		msleep(100);
		if(++loop >10){
			this->failStatusCode = SX932x_NIRQ_ERROR;
			break;
		}
	}
	
	//Check I2C Connection
	ret = read_register(this, SX932x_WHOAMI_REG, &failcode);
	if(ret < 0){
		this->failStatusCode = SX932x_I2C_ERROR;
	}

	SX932X_LOG("failcode = 0x%x\n",failcode);
		
	if(failcode!= SX932x_WHOAMI_VALUE){
		this->failStatusCode = SX932x_ID_ERROR;
	}
	
	SX932X_LOG("failStatusCode = 0x%x\n",this->failStatusCode);
	return (int)this->failStatusCode;
}


/*********************************************************************/
static int sx932x_global_variable_init(psx93XX_t this)
{
	this->irq_disabled = 0;
	this->failStatusCode = 0;

	this->reg_in_dts = false;/*false use .h and true use dts*/

	return 0;
}

static ssize_t sx932x_register_write_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	int reg_address = 0, val = 0;
	psx93XX_t this = dev_get_drvdata(dev);

	if (sscanf(buf, "%x,%x", &reg_address, &val) != 2) {
		SX932X_ERR("%s - The number of data are wrong\n",__func__);
		return -EINVAL;
	}

	write_register(this, (unsigned char)reg_address, (unsigned char)val);
	SX932X_LOG("%s - Register(0x%x) data(0x%x)\n",__func__, reg_address, val);

	return count;
}
//read registers not include the advanced one
static ssize_t sx932x_register_read_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val=0;
	int regist = 0;
	psx93XX_t this = dev_get_drvdata(dev);

	SX932X_FUN();

	if (sscanf(buf, "%x", &regist) != 1) {
		SX932X_ERR("%s - The number of data are wrong\n",__func__);
		return -EINVAL;
	}

	read_register(this, regist, &val);
	SX932X_LOG("%s - Register(0x%2x) data(0x%2x)\n",__func__, regist, val);

	return count;
}

static void read_rawData(psx93XX_t this)
{
	u8 msb=0, lsb=0;
	u8 csx;
	s32 useful;
	s32 average;
	s32 diff;
	u16 offset;
#ifdef RFDEBUG
	u8 i = 0;
	int32_t value[3] = {0};

	read_register(this, SX932x_STAT0_REG, &i);
                
       	if ((i & 0x01) == 0x01) {
               	value[0] = 1;
      	} else {
		value[0] = 0;
	}
	
#endif
	if(this){
#ifdef RFDEBUG
		for(csx =0; csx<1; csx++)
#else
		for(csx =0; csx<4; csx++)
#endif
		{
			write_register(this,SX932x_CPSRD,csx);//here to check the CS1, also can read other channel		
			read_register(this,SX932x_USEMSB,&msb);
			read_register(this,SX932x_USELSB,&lsb);
			useful = (s32)((msb << 8) | lsb);
			
			read_register(this,SX932x_AVGMSB,&msb);
			read_register(this,SX932x_AVGLSB,&lsb);
			average = (s32)((msb << 8) | lsb);
			
			read_register(this,SX932x_DIFFMSB,&msb);
			read_register(this,SX932x_DIFFLSB,&lsb);
			diff = (s32)((msb << 8) | lsb);
#ifdef RFDEBUG
                        value[1] = diff;
                        sar_data_report(value);
#endif		
			read_register(this,SX932x_OFFSETMSB,&msb);
			read_register(this,SX932x_OFFSETLSB,&lsb);
			offset = (u16)((msb << 8) | lsb);
			if (useful > 32767)
				useful -= 65536;
			if (average > 32767)
				average -= 65536;
			if (diff > 32767)
				diff -= 65536;
			SX932X_LOG(" [CS: %d] Useful = %d Average = %d, DIFF = %d Offset = %d \n",csx,useful,average,diff,offset);
		}
	}
}

static ssize_t sx932x_raw_data_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	psx93XX_t this = dev_get_drvdata(dev);
	read_rawData(this);
	return 0;
}

static DEVICE_ATTR(manual_calibrate, 0664, manual_offset_calibration_show,manual_offset_calibration_store);
static DEVICE_ATTR(register_write,  0664, NULL,sx932x_register_write_store);
static DEVICE_ATTR(register_read,0664, NULL,sx932x_register_read_store);
static DEVICE_ATTR(raw_data,0664,sx932x_raw_data_show,NULL);
static struct attribute *sx932x_attributes[] = {
	&dev_attr_manual_calibrate.attr,
	&dev_attr_register_write.attr,
	&dev_attr_register_read.attr,
	&dev_attr_raw_data.attr,
	NULL,
};
static struct attribute_group sx932x_attr_group = {
	.attrs = sx932x_attributes,
};

/****************************************************/
/*! \brief  Initialize I2C config from platform data
 * \param this Pointer to main parent struct 
 */
static void sx932x_reg_init(psx93XX_t this)
{
	psx932x_t pDevice = 0;
	psx932x_platform_data_t pdata = 0;
	int i = 0;

	SX932X_LOG("Going to Setup I2C Registers\n");
	/* configure device */ 
	if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw))
	{
		/*******************************************************************************/
		// try to initialize from device tree!
		/*******************************************************************************/
		if (this->reg_in_dts == true) {
			while ( i < pdata->i2c_reg_num) {
				/* Write all registers/values contained in i2c_reg */
				SX932X_LOG("Going to Write Reg from dts: 0x%x Value: 0x%x\n",
							pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
				write_register(this, pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
				i++;
			}
		} else { // use static ones!!
			while ( i < ARRAY_SIZE(sx932x_i2c_reg_setup)) {
				/* Write all registers/values contained in i2c_reg */
				SX932X_LOG("Going to Write Reg: 0x%x Value: 0x%x\n",
							sx932x_i2c_reg_setup[i].reg,sx932x_i2c_reg_setup[i].val);
				write_register(this, sx932x_i2c_reg_setup[i].reg,sx932x_i2c_reg_setup[i].val);
				i++;
			}
		}
	/*******************************************************************************/
	} else {
		SX932X_ERR("ERROR! platform data 0x%p\n",pDevice->hw);
	}

}


/*! \fn static int initialize(psx93XX_t this)
 * \brief Performs all initialization needed to configure the device
 * \param this Pointer to main parent struct 
 * \return Last used command's return value (negative if error)
 */
static int sx932x_initialize(psx93XX_t this)
{
	int ret;

	SX932X_FUN();

	if (this) {
		/* prepare reset by disabling any irq handling */
		this->irq_disabled = 1;
		//disable_irq(this->irq);
		/* perform a reset */
		write_register(this,SX932x_SOFTRESET_REG,SX932x_SOFTRESET);
		/* wait until the reset has finished by monitoring NIRQ */
		dev_info(this->pdev, "Sent Software Reset. Waiting until device is back from reset to continue.\n");
		/* just sleep for awhile instead of using a loop with reading irq status */
		msleep(100);
		
		ret = sx932x_global_variable_init(this);
		
		sx932x_reg_init(this);
		msleep(100); /* make sure everything is running */
		manual_offset_calibration(this);

		/* re-enable interrupt handling */
		enable_irq(this->irq);
		
		/* make sure no interrupts are pending since enabling irq will only
		* work on next falling edge */
		ret = read_regStat(this);
		SX932X_LOG("%s: irq reg status:%d\n", __func__, ret);

		return 0;
	}
	return -ENOMEM;
}

/*! 
 * \brief Handle what to do when a touch occurs
 * \param this Pointer to main parent struct 
 */
static void touchProcess(psx93XX_t this)
{
	int counter = 0;
	u8 i = 0;
#ifdef MULTILEVEL
	u8 j = 0;
#endif
	int numberOfButtons = 0;
	psx932x_t pDevice = NULL;
	struct _buttonInfo *buttons = NULL;
	struct input_dev *input = NULL;

	struct _buttonInfo *pCurrentButton  = NULL;

	if (this && (pDevice = this->pDevice))
	{
		read_register(this, SX932x_STAT0_REG, &i);
#ifdef MULTILEVEL
		read_register(this, SX932x_STAT1_REG, &j);
#endif
		buttons = pDevice->pbuttonInformation->buttons;
		input = pDevice->pbuttonInformation->input;
		numberOfButtons = pDevice->pbuttonInformation->buttonSize;

		if (unlikely( (buttons==NULL) || (input==NULL) )) {
			SX932X_ERR("ERROR!! buttons or input NULL!!!\n");
			return;
		}

		for (counter = 0; counter < numberOfButtons; counter++) {
			pCurrentButton = &buttons[counter];
			if (pCurrentButton==NULL) {
				SX932X_ERR("ERROR!! current button at index: %d NULL!!!\n", counter);
				return; // ERRORR!!!!
			}

			switch (pCurrentButton->state) {
				case IDLE: /* Button is not being touched! */
#ifdef MULTILEVEL
					if (((j & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
                                                SX932X_LOG("cap body %d touched\n", counter);
                                                sar_report_interrupt_data(2);
                                                pCurrentButton->state = BODY;

                                        } else if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
                                                SX932X_LOG("cap button %d touched\n", counter);
                                                sar_report_interrupt_data(1);
                                                pCurrentButton->state = ACTIVE;

                                        } else {
                                                SX932X_LOG("Button %d already released.\n",counter);
                                        }
#else
					if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
						/* User pressed button */
						SX932X_LOG("cap button %d touched\n", counter);
						sar_report_interrupt_data(1);
						pCurrentButton->state = ACTIVE;
	
					} else {
						SX932X_LOG("Button %d already released.\n",counter);
					}
#endif
					break;
				case ACTIVE: /* Button is being touched! */ 
#ifdef MULTILEVEL
					if (((j & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
                                                SX932X_LOG("cap body %d touched\n", counter);
                                                sar_report_interrupt_data(2);
                                                pCurrentButton->state = BODY;

                                        } else if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
						SX932X_LOG("Button %d still touched.\n",counter);
                                        } else {
						/* User released button */
                                                SX932X_LOG("cap button %d released\n",counter);
                                                sar_report_interrupt_data(0);
                                                pCurrentButton->state = IDLE;
                                        }
#else
					if (((i & pCurrentButton->mask) != pCurrentButton->mask)) {
						/* User released button */
						SX932X_LOG("cap button %d released\n",counter);
						sar_report_interrupt_data(0);
						pCurrentButton->state = IDLE;
					} else {
						SX932X_LOG("Button %d still touched.\n",counter);
					}
#endif
					break;
#ifdef MULTILEVEL
				case BODY: /* Body is being touched! */
                                        if (((j & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
                                                SX932X_LOG("Body %d still touched\n", counter);
                                        } else if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
                                                /* User pressed button */
                                                SX932X_LOG("cap button %d touched\n", counter);
                                                sar_report_interrupt_data(1);
                                                pCurrentButton->state = ACTIVE;

                                        } else {
                                                /* User released button */
                                                SX932X_LOG("cap button %d released\n",counter);
                                                sar_report_interrupt_data(0);
                                                pCurrentButton->state = IDLE;
                                        }
                                        break;
#endif
				default: /* Shouldn't be here, device only allowed ACTIVE or IDLE */
					break;
			};
		}
	}
}

static int sx932x_parse_dt(struct sx932x_platform_data *pdata, struct device *dev)
{
	struct device_node *dNode = dev->of_node;

//add by fanxzh begin
	//enum of_gpio_flags flags;
	//int ret;
//add by fanxzh end
	if (dNode == NULL)
		return -ENODEV;
	
//change by fanxzh
#if 0
	pdata->irq_gpio= of_get_named_gpio_flags(dNode,
											"Semtech,nirq-gpio", 0, &flags);
#else
	pdata->irq_gpio = irq_of_parse_and_map(dNode, 0);
#endif
//change by fanxzh end
	SX932X_LOG("%s:%d pdata->irq_gpio = %d\n", __func__, __LINE__, pdata->irq_gpio);
	irq_gpio_num = pdata->irq_gpio;
	if (pdata->irq_gpio < 0) {
		SX932X_ERR("get irq_gpio error\n");
		return -ENODEV;
	}
	
	/***********************************************************************/
	// load in registers from device tree
	of_property_read_u32(dNode,"Semtech,reg-num",&pdata->i2c_reg_num);
	// layout is register, value, register, value....
	// if an extra item is after just ignore it. reading the array in will cause it to fail anyway
	SX932X_LOG("%s -  size of elements %d \n", __func__,pdata->i2c_reg_num);
	if (pdata->i2c_reg_num > 0) {
		 // initialize platform reg data array
		 pdata->pi2c_reg = devm_kzalloc(dev,sizeof(struct smtc_reg_data)*pdata->i2c_reg_num, GFP_KERNEL);
		 if (unlikely(pdata->pi2c_reg == NULL)) {
			return -ENOMEM;
		}

	 // initialize the array
		if (of_property_read_u8_array(dNode,"Semtech,reg-init",(u8*)&(pdata->pi2c_reg[0]),sizeof(struct smtc_reg_data)*pdata->i2c_reg_num))
		return -ENOMEM;
	}
	/***********************************************************************/
	SX932X_LOG("%s -[%d] parse_dt complete\n", __func__,pdata->irq_gpio);
	return 0;
}

/* get the NIRQ state (1->NIRQ-low, 0->NIRQ-high) */
static int sx932x_init_platform_hw(struct i2c_client *client)
{
	psx93XX_t this = i2c_get_clientdata(client);
	struct sx932x *pDevice = NULL;
	struct sx932x_platform_data *pdata = NULL;
	int rc = 0;

	SX932X_FUN();

	if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw)) {
		if (gpio_is_valid(SX932x_GPIO_INT)) {
			/*
			rc = gpio_request(pdata->irq_gpio, "sx932x_irq_gpio");
			if (rc < 0) {
				dev_err(this->pdev, "SX932x Request gpio. Fail![%d]\n", rc);
				return rc;
			}
			rc = gpio_direction_input(pdata->irq_gpio);
			if (rc < 0) {
				dev_err(this->pdev, "SX932x Set gpio direction. Fail![%d]\n", rc);
				return rc;
			}
			*/ 
			this->irq = client->irq = pdata->irq_gpio;//gpio_to_irq(pdata->irq_gpio);
		}
		else {
			SX932X_ERR("SX932x Invalid irq gpio num.(init)\n");
		}
	}
	else {
		SX932X_ERR("Do not init platform HW");
	}

	return rc;
}

static void sx932x_exit_platform_hw(struct i2c_client *client)
{
	/*
	psx93XX_t this = i2c_get_clientdata(client);
	struct sx932x *pDevice = NULL;
	struct sx932x_platform_data *pdata = NULL;

	if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw)) {
		if (gpio_is_valid(pdata->irq_gpio)) {
			gpio_free(pdata->irq_gpio);
		}
		else {
			dev_err(this->pdev, "Invalid irq gpio num.(exit)\n");
		}
	}
*/
	return;
}

static int sx932x_get_nirq_state(void)
{
	return  !gpio_get_value(irq_gpio_num);
}

static int sx93XX_pinctl_init(struct i2c_client *client)
{
	int err;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	
	SX932X_FUN();

	/* gpio setting */
	pinctrl = devm_pinctrl_get(&client->dev);	
	if (IS_ERR(pinctrl)) {
		err = PTR_ERR(pinctrl);
		SX932X_ERR("Cannot find sar pinctrl!\n");
		return err;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		err = PTR_ERR(pins_default);
		SX932X_ERR("Cannot find sar pinctrl default!\n");
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		err = PTR_ERR(pins_cfg);
		SX932X_ERR("Cannot find sar pinctrl pin_cfg!\n");
		return err;
	}
	
	pinctrl_select_state(pinctrl, pins_cfg);
	
	return 0;
}

static int sx932x_situation_open_report_data(int open)
{
	SX932X_FUN();
    
    if (!open)
    {
		SX932X_LOG("%s close report data, line:%d.\n", __func__, __LINE__);
		if (g_this)
		{
			disable_irq(g_this->irq);
			write_register(g_this,SX932x_CTRL1_REG,0x20);//make sx932x in Sleep mode
		}
	}
	else
	{
		SX932X_LOG("%s open report data, line:%d.\n", __func__, __LINE__);
		if (g_this) {
			sx93XX_schedule_work(g_this,0);
			if (g_this->init)
				g_this->init(g_this);
			enable_irq(g_this->irq);
		}
	}

    return 0;
}

static int sx932x_situation_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	SX932X_FUN();
    return 0;
}

static int sx932x_situation_flush(void)
{
	SX932X_FUN();
    return situation_flush_report(ID_SAR);
}


static int sx932x_situation_get_data(int *probability, int *status)
{
	SX932X_FUN();
	return 0;
}

static int sar_factory_enable_sensor(bool enabledisable,
                                         int64_t sample_periods_ms)
{
	SX932X_FUN();
	return 0;
}

static int sar_factory_get_data(int32_t sensor_data[3])
{
	SX932X_FUN();
	return 0;
}

static int sar_factory_enable_calibration(void)
{
	SX932X_FUN();
	write_register(g_this,SX932x_CTRL1_REG,0x20);
	msleep(10);
	write_register(g_this,SX932x_CTRL1_REG,0x21);
        return 0;
}

static int sar_factory_get_cali(int32_t data[3])
{
	return 0;
}

static struct sar_factory_fops sx932x_factory_fops = {
        .enable_sensor = sar_factory_enable_sensor,
        .get_data = sar_factory_get_data,
        .enable_calibration = sar_factory_enable_calibration,
        .get_cali = sar_factory_get_cali,
};

static struct sar_factory_public sx932x_factory_device = {
        .gain = 1,
        .sensitivity = 1,
        .fops = &sx932x_factory_fops,
};

static int sx932x_situation_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = sx932x_situation_open_report_data;
	ctl.batch = sx932x_situation_batch;
	ctl.flush = sx932x_situation_flush;
	ctl.is_support_wake_lock = false;
	ctl.is_support_batch = false;
	err = situation_register_control_path(&ctl, ID_SAR);
	if (err) {
		SX932X_ERR("register situation control path err\n");
		goto exit;
	}

	data.get_data = sx932x_situation_get_data;
	err = situation_register_data_path(&data, ID_SAR);
	if (err) {
		SX932X_ERR("register situation data path err\n");
		goto exit;
	}

	err = sar_factory_device_register(&sx932x_factory_device);
        if (err) {
                SX932X_ERR("sar_factory_device register failed\n");
                goto exit;
        }

	return 0;

exit:
	return err;
}

static void sx93XX_schedule_work(psx93XX_t this, unsigned long delay)
{
	unsigned long flags;

	if (this) {
		spin_lock_irqsave(&this->lock,flags);
		/* Stop any pending penup queues */
		cancel_delayed_work(&this->dworker);
		//after waiting for a delay, this put the job in the kernel-global workqueue. so no need to create new thread in work queue.
		schedule_delayed_work(&this->dworker,delay);
		spin_unlock_irqrestore(&this->lock,flags);
	}
	else
		SX932X_ERR("sx93XX_schedule_work, NULL psx93XX_t\n");
} 

static irqreturn_t sx93XX_irq_func(int irq, void *pvoid)
{
	psx93XX_t this = 0;
	
	if (pvoid) {
		this = (psx93XX_t)pvoid;
		if ((!this->get_nirq_low) || this->get_nirq_low()) {
			sx93XX_schedule_work(this,0);
		}
		else{
			SX932X_ERR("sx93XX_irq_func - nirq read high\n");
		}
	}
	else{
		SX932X_ERR("sx93XX_irq_func, NULL pvoid\n");
	}

	return IRQ_HANDLED;
}

static void sx93XX_worker_func(struct work_struct *work)
{
	psx93XX_t this = 0;
	int status = 0;
	int counter = 0;
	u8 nirqLow = 0;

	if (work) {
		this = container_of(work,sx93XX_t,dworker.work);

		if (!this) {
			SX932X_ERR("sx93XX_worker_func, NULL sx93XX_t\n");
			return;
		}
		if (unlikely(this->useIrqTimer)) {
			if ((!this->get_nirq_low) || this->get_nirq_low()) {
				nirqLow = 1;
			}
		}
		/* since we are not in an interrupt don't need to disable irq. */
		status = this->refreshStatus(this);
		counter = -1;
		//dev_info(this->pdev, "Worker - Refresh Status %d\n",status);
		
		while((++counter) < MAX_NUM_STATUS_BITS) { /* counter start from MSB */
			if (((status>>counter) & 0x01) && (this->statusFunc[counter])) {
				//dev_info(this->pdev, "SX932x Function Pointer Found. Calling\n");
				this->statusFunc[counter](this);
			}
		}
		if (unlikely(this->useIrqTimer && nirqLow))
		{	/* Early models and if RATE=0 for newer models require a penup timer */
			/* Queue up the function again for checking on penup */
			sx93XX_schedule_work(this,msecs_to_jiffies(this->irqTimeout));
		}
	} else {
		SX932X_ERR("sx93XX_worker_func, NULL work_struct\n");
	}
}

static int sx93XX_irq_init(psx93XX_t this)
{
	int err = 0;

	SX932X_FUN();

	if (this && this->pDevice)
	{
		/* initialize spin lock */
		spin_lock_init(&this->lock);
		/* initialize worker function */
		INIT_DELAYED_WORK(&this->dworker, sx93XX_worker_func);
		/* initailize interrupt reporting */
		this->irq_disabled = 0;
		err = request_irq(this->irq, sx93XX_irq_func, IRQF_TRIGGER_FALLING,
							this->pdev->driver->name, this);
		if (err) {
			SX932X_ERR("irq %d busy?\n", this->irq);
			return err;
		}
		SX932X_LOG("registered with irq (%d)\n", this->irq);
	}
	return -ENOMEM;
}

/*! \fn static int sx932x_probe(struct i2c_client *client, const struct i2c_device_id *id)
 * \brief Probe function
 * \param client pointer to i2c_client
 * \param id pointer to i2c_device_id
 * \return Whether probe was successful
 */
static int sx932x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{    
	int i = 0;
	int err = 0;
	//u8 val = 0;

	psx93XX_t this = 0;
	psx932x_t pDevice = 0;
	psx932x_platform_data_t pplatData = 0;
	struct totalButtonInformation *pButtonInformationData = NULL;
	struct input_dev *input = NULL;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	SX932X_FUN();

#ifdef ontim_debug_info
	CHECK_THIS_DEV_DEBUG_AREADY_EXIT();
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		SX932X_ERR("Check i2c functionality.Fail!\n");
		err = -EIO;
		return err;
	}

	this = devm_kzalloc(&client->dev,sizeof(sx93XX_t), GFP_KERNEL); /* create memory for main struct */
	SX932X_LOG("Initialized Main Memory: 0x%p\n",this);

	pButtonInformationData = devm_kzalloc(&client->dev , sizeof(struct totalButtonInformation), GFP_KERNEL);
	if (!pButtonInformationData) {
		SX932X_ERR("Failed to allocate memory(totalButtonInformation)\n");
		err = -ENOMEM;
		goto exit_malloc_btn_data_failed;
	}

	pButtonInformationData->buttonSize = ARRAY_SIZE(psmtcButtons);
	pButtonInformationData->buttons =  psmtcButtons;
	pplatData = devm_kzalloc(&client->dev,sizeof(struct sx932x_platform_data), GFP_KERNEL);
	if (!pplatData) {
		SX932X_ERR("platform data is required!\n");
		err = -EINVAL;
		goto exit_malloc_platform_data_failed;
	}
	pplatData->get_is_nirq_low = sx932x_get_nirq_state;
	pplatData->pbuttonInformation = pButtonInformationData;
 
	client->dev.platform_data = pplatData; 
	err = sx932x_parse_dt(pplatData, &client->dev);
	if (err) {
		SX932X_ERR("could not setup pin\n");
		err = -ENODEV;
		goto exit_parse_dt_failed;
	}

	pplatData->init_platform_hw = sx932x_init_platform_hw;
	SX932X_LOG("SX932x init_platform_hw done!\n");
/*
	//Check I2C Connection
	err = read_register(this, SX932x_WHOAMI_REG, &val);
	if(err < 0){
		SX932X_ERR("check i2c connection failed.\n");
		err = -ENODEV;
		goto exit_check_device_failed;
	}
*/	
	if (this){
		/* In case we need to reinitialize data 
		* (e.q. if suspend reset device) */
		this->init = sx932x_initialize;
		/* shortcut to read status of interrupt */
		this->refreshStatus = read_regStat;
		/* pointer to function from platform data to get pendown 
		* (1->NIRQ=0, 0->NIRQ=1) */
		this->get_nirq_low = pplatData->get_is_nirq_low;
		/* save irq in case we need to reference it */
		this->irq = client->irq;
		/* do we need to create an irq timer after interrupt ? */
		this->useIrqTimer = 0;

		/* Setup function to call on corresponding reg irq source bit */
		if (MAX_NUM_STATUS_BITS>= 8)
		{
			this->statusFunc[0] = 0; /* TXEN_STAT */
			this->statusFunc[1] = 0; /* UNUSED */
			this->statusFunc[2] = 0; /* UNUSED */
			this->statusFunc[3] = read_rawData; /* CONV_STAT */
			this->statusFunc[4] = 0; /* COMP_STAT */
			this->statusFunc[5] = touchProcess; /* RELEASE_STAT */
			this->statusFunc[6] = touchProcess; /* TOUCH_STAT  */
			this->statusFunc[7] = 0; /* RESET_STAT */
		}

		/* setup i2c communication */
		this->bus = client;
		i2c_set_clientdata(client, this);

		/* record device struct */
		this->pdev = &client->dev;

		/* create memory for device specific struct */
		this->pDevice = pDevice = devm_kzalloc(&client->dev,sizeof(sx932x_t), GFP_KERNEL);
		SX932X_LOG("Initialized Device Specific Memory: 0x%p\n",pDevice);

		if (pDevice){
			/* for accessing items in user data (e.g. calibrate) */
			err = sysfs_create_group(&client->dev.kobj, &sx932x_attr_group);
			//sysfs_create_group(client, &sx932x_attr_group);

			/* Add Pointer to main platform data struct */
			pDevice->hw = pplatData;

			/* Check if we hava a platform initialization function to call*/
			if (pplatData->init_platform_hw)
				pplatData->init_platform_hw(client);

			/* Initialize the button information initialized with keycodes */
			pDevice->pbuttonInformation = pplatData->pbuttonInformation;
			/* Create the input device */
			input = input_allocate_device();
			if (!input) {
				err = -ENOMEM;
				goto exit_init_failed;
			}
			/* Set all the keycodes */
			__set_bit(EV_KEY, input->evbit);
			#if 1
			for (i = 0; i < pButtonInformationData->buttonSize; i++) {
				__set_bit(pButtonInformationData->buttons[i].keycode,input->keybit);
				pButtonInformationData->buttons[i].state = IDLE;
			}
			#endif
			/* save the input pointer and finish initialization */
			pButtonInformationData->input = input;
			input->name = "SX932x Cap Touch";
			input->id.bustype = BUS_I2C;
			if(input_register_device(input)){
				err = -ENOMEM;
				goto exit_init_failed;
			}
		}

		sx93XX_irq_init(this);
		
		sx93XX_pinctl_init(client);

		/* call init function pointer (this should initialize all registers */
		if (this->init){
			this->init(this);
		}else{
			dev_err(this->pdev,"No init function!!!!\n");
			err = -ENOMEM;
			goto exit_init_failed;
		}
	}else{
		return -1;
	}

	sx932x_Hardware_Check(this);
	pplatData->exit_platform_hw = sx932x_exit_platform_hw;
	g_this = this;
	disable_irq(this->irq);

	/* set sx932x to sleep mode */
	write_register(this,SX932x_CTRL1_REG,0x20);

	sx932x_situation_init();
	sx932x_init_flag = 0;

#ifdef ontim_debug_info
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
#endif

	SX932X_LOG("sx932x_probe() Done\n");

	return 0;

exit_init_failed:
//exit_check_device_failed:
exit_parse_dt_failed:
	devm_kfree(&client->dev, pplatData);
exit_malloc_platform_data_failed:
	devm_kfree(&client->dev, pButtonInformationData);
exit_malloc_btn_data_failed:
	devm_kfree(&client->dev, this);
	SX932X_ERR(" probe failed, err = %d\n", err);
	return err;
}

int sx93XX_remove(psx93XX_t this)
{
	if (this) {
		cancel_delayed_work_sync(&this->dworker); /* Cancel the Worker Func */
		/*destroy_workqueue(this->workq); */
		free_irq(this->irq, this);
		kfree(this);
		return 0;
	}
	return -ENOMEM;
}

/*! \fn static int sx932x_remove(struct i2c_client *client)
 * \brief Called when device is to be removed
 * \param client Pointer to i2c_client struct
 * \return Value from sx93XX_remove()
 */
//static int __devexit sx932x_remove(struct i2c_client *client)
static int sx932x_remove(struct i2c_client *client)
{
	psx932x_platform_data_t pplatData =0;
	psx932x_t pDevice = 0;
	psx93XX_t this = i2c_get_clientdata(client);

	if (this && (pDevice = this->pDevice))
	{
		input_unregister_device(pDevice->pbuttonInformation->input);

		sysfs_remove_group(&client->dev.kobj, &sx932x_attr_group);
		pplatData = client->dev.platform_data;
		if (pplatData && pplatData->exit_platform_hw)
			pplatData->exit_platform_hw(client);
		kfree(this->pDevice);
	}
	sx932x_init_flag = -1;

	return sx93XX_remove(this);
}

#if 1//def CONFIG_PM
static int sx932x_suspend(struct device *dev)
{
	psx93XX_t this = dev_get_drvdata(dev);
	
	if (this)
		disable_irq(this->irq);

	//write_register(this,SX932x_CTRL1_REG,0x20);//make sx932x in Sleep mode
	
	return 0;
}

static int sx932x_resume(struct device *dev)
{
	psx93XX_t this = dev_get_drvdata(dev);
	
	if (this) {
		sx93XX_schedule_work(this,0);
		if (this->init)
			this->init(this);
		enable_irq(this->irq);
	}
	
	return 0;
}
#else
	#define sx932x_suspend		NULL
	#define sx932x_resume		NULL
#endif /* CONFIG_PM */

static struct i2c_device_id sx932x_idtable[] = {
	{ DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sx932x_idtable);
#ifdef CONFIG_OF
static struct of_device_id sx932x_match_table[] = {
	{ .compatible = "mediatek,sar",},
	{ },
};
#else
#define sx932x_match_table NULL
#endif

static const struct dev_pm_ops sx932x_pm_ops = {
	.suspend = sx932x_suspend,
	.resume = sx932x_resume,
};

static struct i2c_driver sx932x_driver = {
	.driver = {
		.owner			= THIS_MODULE,
		.name			= DRIVER_NAME,
		.of_match_table	= sx932x_match_table,
		.pm				= &sx932x_pm_ops,
	},
	.id_table		= sx932x_idtable,
	.probe			= sx932x_probe,
	.remove			= sx932x_remove,
};

static int sx932x_local_init(void)
{
	SX932X_FUN();
	
	if(i2c_add_driver(&sx932x_driver))
	{
		SX932X_ERR("add driver error\n");
		return -1;
	}

	if(-1 == sx932x_init_flag)
	{
	   return -1;
	}
	
	return 0;
}

static int sx932x_local_uninit(void)
{
	SX932X_FUN();

	i2c_del_driver(&sx932x_driver);
	sx932x_init_flag = -1;

	return 0;
}

static struct situation_init_info sx932x_init_info = {
	.name = "sx932x",
	.init = sx932x_local_init,
	.uninit = sx932x_local_uninit,
};

static int __init sx932x_init(void)
{
	SX932X_FUN();
	situation_driver_add(&sx932x_init_info, ID_SAR);
	return 0;
}

static void __exit sx932x_exit(void)
{
	SX932X_FUN();
}

module_init(sx932x_init);
module_exit(sx932x_exit);

MODULE_AUTHOR("Semtech Corp. (http://www.semtech.com/)");
MODULE_DESCRIPTION("SX932x Capacitive Touch Controller Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
