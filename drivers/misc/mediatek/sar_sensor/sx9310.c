/*! \file sx9310.c
 * \brief  SX9310 Driver
 *
 * Driver for the SX9310 
 * Copyright (c) 2011 Semtech Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
 
//#define DEBUG
#define DRIVER_NAME "mediatek,sx9310"

#define MAX_WRITE_ARRAY_SIZE 32
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/input/sx9310.h> /* main struct, interrupt,init,pointers */

#include <linux/of_irq.h>
//#include "mt_boot_common.h"
#include "mt_boot_common.h"

#define IDLE 0
#define ACTIVE 1

/*! \struct sx9310
 * Specialized struct containing input event data, platform data, and
 * last cap state read if needed.
 */

typedef struct sx9310
{
  pbuttonInformation_t pbuttonInformation;
	psx9310_platform_data_t hw; /* specific platform data settings */
} sx9310_t, *psx9310_t;

static void ForcetoTouched(psx93XX_t this)
{
  psx9310_t pDevice = NULL;
  struct input_dev *input = NULL;
  struct _buttonInfo *pCurrentButton  = NULL;

  if (this && (pDevice = this->pDevice))
  {
      pr_debug("ForcetoTouched()\n");
    
      pCurrentButton = pDevice->pbuttonInformation->buttons;
      input = pDevice->pbuttonInformation->input;

      input_report_key(input, pCurrentButton->keycode, 1);
      pCurrentButton->state = ACTIVE;

      input_sync(input);

	  pr_debug("Leaving ForcetoTouched()\n");
  }
}

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
	  pr_debug("write_register Address: 0x%x Value: 0x%x Return: %d\n",
        address,value,returnValue);
  }
  if(returnValue < 0){
  	ForcetoTouched(this);
	pr_debug("Write_register-ForcetoTouched()\n");
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
	  pr_debug( "read_register Address: 0x%x Return: 0x%x\n",address,returnValue);
    if (returnValue >= 0) {
      *value = returnValue;
      return 0;
    } else {
      return returnValue;
    }
  }
  ForcetoTouched(this);
  pr_debug("read_register-ForcetoTouched()\n");
  return -ENOMEM;
}
/*! \brief Sends a write register range to the device
 * \param this Pointer to main parent struct 
 * \param reg 8-bit register address (base address)
 * \param data pointer to 8-bit register values
 * \param size size of the data pointer
 * \return Value from i2c_master_send
 */
 #if 0
static int write_registerEx(psx93XX_t this, unsigned char reg,
				unsigned char *data, int size)
{
  struct i2c_client *i2c = 0;
	u8 tx[MAX_WRITE_ARRAY_SIZE];
	int ret = 0;

  if (this && (i2c = this->bus) && data && (size <= MAX_WRITE_ARRAY_SIZE))
  {
    pr_debug("inside write_registerEx()\n");
    tx[0] = reg;
    pr_debug("going to call i2c_master_send(0x%p, 0x%x ",
            (void *)i2c,tx[0]);
    for (ret = 0; ret < size; ret++)
    {
      tx[ret+1] = data[ret];
      pr_debug("0x%x, ",tx[ret+1]);
    }
    pr_err("\n");

    ret = i2c_master_send(i2c, tx, size+1 );
	  if (ret < 0)
	  	pr_err("I2C write error\n");
  }
  pr_debug("leaving write_registerEx()\n");


	return ret;
}
/*! \brief Reads a group of registers from the device
* \param this Pointer to main parent struct 
* \param reg 8-Bit address to read from (base address)
* \param data Pointer to 8-bit value array to save registers to 
* \param size size of array
* \return Value from i2c_smbus_read_byte_data if < 0. else 0
*/
static int read_registerEx(psx93XX_t this, unsigned char reg,
				unsigned char *data, int size)
{
  struct i2c_client *i2c = 0;
	int ret = 0;
	u8 tx[] = {
		reg
	};
  if (this && (i2c = this->bus) && data && (size <= MAX_WRITE_ARRAY_SIZE))
  {
    pr_debug("inside read_registerEx()\n");
    dev_dbg(this->pdev,
        "going to call i2c_master_send(0x%p,0x%p,1) Reg: 0x%x\n",
                                                               (void *)i2c,(void *)tx,tx[0]);
  	ret = i2c_master_send(i2c,tx,1);
  	if (ret >= 0) {
      pr_debug("going to call i2c_master_recv(0x%p,0x%p,%x)\n",
                                                              (void *)i2c,(void *)data,size);
  		ret = i2c_master_recv(i2c, data, size);
    }
  }
	if (unlikely(ret < 0))
		pr_err("I2C read error\n");
  pr_debug("leaving read_registerEx()\n");
	return ret;
}
#endif
/*********************************************************************/
/*! \brief Perform a manual offset calibration
* \param this Pointer to main parent struct 
* \return Value return value from the write register
 */
static int manual_offset_calibration(psx93XX_t this)
{
  s32 returnValue = 0;
  returnValue = write_register(this,SX9310_IRQSTAT_REG,0xFF);
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

  pr_err("Reading IRQSTAT_REG\n");
  read_register(this,SX9310_IRQSTAT_REG,&reg_value);
	return sprintf(buf, "%d\n", reg_value);
}

/*! \brief sysfs store function for manual calibration
 */
static ssize_t manual_offset_calibration_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	psx93XX_t this = dev_get_drvdata(dev);

	if (kstrtoul(buf, 16,&val))
		return -EINVAL;
	if (val) 
	{
		pr_debug("Performing manual_offset_calibration()\n");
		manual_offset_calibration(this);
     }
	return count;
}

static unsigned int reg_addr;

// echo 0x3,0x60 >sx9310_reg 
static ssize_t sx9310_register_write_store(struct device *dev,	struct device_attribute *attr, const char *buf, size_t count)
{
	
	unsigned int regist = 0, val = 0;
	
	 psx93XX_t data = (psx93XX_t )dev_get_drvdata(dev);

	if (sscanf(buf, "%x,%x", &regist, &val) != 2)
	{
		pr_err("[SX9310]: %s - The number of data are wrong\n",
		__func__);
		return -EINVAL;
	}
	 reg_addr=regist;
	write_register(data, (unsigned char)regist, (unsigned char)val);

	return count;
}
//cat sx9310_reg
static ssize_t sx9310_register_read_show(struct device *dev,struct device_attribute *attr,  char *buf)
{
	u8 reg_value = 0; 
	psx93XX_t this = dev_get_drvdata(dev);
	
	read_register(this,reg_addr & 0xff,&reg_value);
	return sprintf(buf, "sx9310 reg( 0x%x ) = 0x%x\n",(unsigned int)reg_addr & 0xff,reg_value);
#if 0
	for (i=0;i<29;i++)
	{
	reg_address = i+0x10;
	read_register(this,reg_address,&reg_value);
	sprintf(buf, "sx9310 register( 0x%x ) = 0x%x\n",reg_address,reg_value);
	}

#endif
	return 0;

}

static void read_rawData(psx93XX_t this)
{
	u8 msb=0, lsb=0;
	if(this)
	{
		write_register(this,SX9310_CPSRD,1);//here to check the CS1, also can read other channel
		msleep(100);
		read_register(this,SX9310_USEMSB,&msb);
		read_register(this,SX9310_USELSB,&lsb);
		dev_dbg(this->pdev, "sx9310 raw data USEFUL msb = 0x%x, lsb = 0x%x\n",msb,lsb);
		read_register(this,SX9310_AVGMSB,&msb);
		read_register(this,SX9310_AVGLSB,&lsb);
		dev_dbg(this->pdev, "sx9310 raw data AVERAGE msb = 0x%x, lsb = 0x%x\n",msb,lsb);
		read_register(this,SX9310_DIFFMSB,&msb);
		read_register(this,SX9310_DIFFLSB,&lsb);
		dev_dbg(this->pdev, "sx9310 raw data DIFF msb = 0x%x, lsb = 0x%x\n",msb,lsb);
		read_register(this,SX9310_OFFSETMSB,&msb);
		read_register(this,SX9310_OFFSETLSB,&lsb);
		dev_dbg(this->pdev, "sx9310 raw data OFFSET msb = 0x%x, lsb = 0x%x\n",msb,lsb);
	}

}

//In probe function , please add as below.

static DEVICE_ATTR(sx9310_reg, 0664,sx9310_register_read_show, sx9310_register_write_store);
static DEVICE_ATTR(calibrate, 0664, manual_offset_calibration_show,  manual_offset_calibration_store);

static struct attribute *sx9310_attributes[] = {
	&dev_attr_calibrate.attr,
	&dev_attr_sx9310_reg.attr, //Only add this line in original driver
	NULL
};
static  struct attribute_group sx9310_attr_group = {
	.attrs = sx9310_attributes,
};

/*********************************************************************/





/*! \fn static int read_regStat(psx93XX_t this)
 * \brief Shortcut to read what caused interrupt.
 * \details This is to keep the drivers a unified
 * function that will read whatever register(s) 
 * provide information on why the interrupt was caused.
 * \param this Pointer to main parent struct 
 * \return If successful, Value of bit(s) that cause interrupt, else 0
 */
 static int read_regStat_v1(psx93XX_t this)
{
	u8 data = 0;
	if (this) {
		if (read_register(this,SX9310_IRQSTAT_REG,&data) == 0)
		return (data & 0x00FF);
	}
	return 0;
}
static void read_regStat(psx93XX_t this)
{
	u8 data = 0;
	if (this) {
		if (read_register(this,SX9310_IRQSTAT_REG,&data) == 0)
			 data=data & 0x00FF;
		//(data & 0x00FF);
	}
	//return 0;
}

/*! \brief  Initialize I2C config from platform data
 * \param this Pointer to main parent struct 
 */
static void hw_init(psx93XX_t this)
{
	psx9310_t pDevice = 0;
	psx9310_platform_data_t pdata = 0;
	int i = 0;
	//int return_data = 0;
	//int return_data[50] = 0;
	/* configure device */
	printk("yang Going to Setup I2C Registers\n");
	if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw))
	{
		while ( i < pdata->i2c_reg_num) {
		/* Write all registers/values contained in i2c_reg */
		//pr_err("Going to Write Reg: 0x%x Value: 0x%x\n",
		    //pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
		//      msleep(3);        
		write_register(this, pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
		i++;
		}
	} else {
		pr_err("ERROR! platform data 0x%p\n",pDevice->hw);
		//Force to touched if error
		ForcetoTouched(this);
		pr_err("Hardware_init-ForcetoTouched()\n");
	}
}
/*********************************************************************/




/*! \fn static int initialize(psx93XX_t this)
 * \brief Performs all initialization needed to configure the device
 * \param this Pointer to main parent struct 
 * \return Last used command's return value (negative if error)
 */
static int initialize(psx93XX_t this)
{
	//int reg_value = 0;
  if (this) {
    /* prepare reset by disabling any irq handling */
    this->irq_disabled = 1;
    disable_irq(this->irq);
    /* perform a reset */
    write_register(this,SX9310_SOFTRESET_REG,SX9310_SOFTRESET);
    /* wait until the reset has finished by monitoring NIRQ */
    pr_debug("Sent Software Reset. Waiting until device is back from reset to continue.\n");
    /* just sleep for awhile instead of using a loop with reading irq status */
    msleep(300);
//    while(this->get_nirq_low && this->get_nirq_low()) { read_regStat(this); }
    //pr_err("Device is back from the reset, continuing. NIRQ = %d\n",this->get_nirq_low());
    hw_init(this);
    msleep(100); /* make sure everything is running */
    manual_offset_calibration(this);
    
    /* re-enable interrupt handling */
    enable_irq(this->irq);
    this->irq_disabled = 0;
   
    /* make sure no interrupts are pending since enabling irq will only
     * work on next falling edge */
    read_regStat(this);
   // pr_err("Exiting initialize(). NIRQ = %d\n",this->get_nirq_low());
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
  int numberOfButtons = 0;
  psx9310_t pDevice = NULL;
  struct _buttonInfo *buttons = NULL;
  struct input_dev *input = NULL;
  struct _buttonInfo *pCurrentButton  = NULL;


  if (this && (pDevice = this->pDevice))
  {
    pr_debug("Inside touchProcess()\n");
    read_register(this, SX9310_STAT0_REG, &i);

    buttons = pDevice->pbuttonInformation->buttons;
    input = pDevice->pbuttonInformation->input;
    numberOfButtons = pDevice->pbuttonInformation->buttonSize;
    
    if (unlikely( (buttons==NULL) || (input==NULL) )) {
      pr_err("ERROR!! buttons or input NULL!!!\n");
      return;
    }

    for (counter = 0; counter < numberOfButtons; counter++) {
      pCurrentButton = &buttons[counter];
      if (pCurrentButton==NULL) {
        pr_err("ERROR!! current button at index: %d NULL!!!\n",counter);
        return; // ERRORR!!!!
      }
      switch (pCurrentButton->state) {
        case IDLE: /* Button is not being touched! */
          if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
            /* User pressed button */
            pr_err( " IDLE cap button %d touched %d pCurrentButton-> keycode %d i:\n", counter,pCurrentButton->keycode,i);
            input_report_key(input, pCurrentButton->keycode, 1);
	          input_sync(input);
            pCurrentButton->state = ACTIVE;
           // msleep(1);
	         input_report_key(input, pCurrentButton->keycode, 0);
	         input_sync(input);
          } else {
            pr_err("IDLE Button %d already released.\n",counter);
          }
          break;
        case ACTIVE: /* Button is being touched! */ 
          if (((i & pCurrentButton->mask) != pCurrentButton->mask)) {
            /* User released button */
            pr_err( "fan1 cap button %d released %d  pCurrentButton->keycode: %d i \n",counter,pCurrentButton->keycode,i);
            input_report_key(input, pCurrentButton->keycode_release, 1);
	          input_sync(input);
            pCurrentButton->state = IDLE;
            //msleep(1);
            input_report_key(input, pCurrentButton->keycode_release, 0);
	        input_sync(input);
          } else {
            pr_err("Button %d still touched.\n",counter);
          }
          break;
        default: /* Shouldn't be here, device only allowed ACTIVE or IDLE */
          break;
      };
    }
   // input_sync(input);

	  pr_debug("Leaving touchProcess()\n");
  }
}


static unsigned int sx9310_irq_gpio = 0;
static int of_get_sx9310_platform_data(struct device *dev)
{
	struct device_node *node = NULL;
	pr_debug("enter of_get_sx9310_platform_data\n");
	node = of_find_compatible_node(NULL, NULL, "mediatek,sx9310");
	if (node) {
		sx9310_irq_gpio = irq_of_parse_and_map(node, 0);
		if (sx9310_irq_gpio < 0) {
				pr_err("sx9310 get interrupts fail!\n");	
				return -1;
		} else {
				pr_err("sx9310 get interrupts success sx9310_irq_gpio = %d!\n", sx9310_irq_gpio);	
		}

	}
	return 0;
}

static int sx9310_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, DRIVER_NAME);
	return 0;
}


/*! \fn static int sx9310_probe(struct i2c_client *client, const struct i2c_device_id *id)
 * \brief Probe function
 * \param client pointer to i2c_client
 * \param id pointer to i2c_device_id
 * \return Whether probe was successful
 */
static int sx9310_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i = 0;
	psx93XX_t this = 0;
	psx9310_t pDevice = 0;
	psx9310_platform_data_t pplatData = 0;
	struct input_dev *input = NULL;

	
// fanhonggen check i2c  begin 20160308
	int err,wrtval = 0;
	unsigned char dbbus_tx_data =0x01;
	pr_debug("sx9310_probe()\n");
	//client->addr=0x28;
	wrtval=i2c_master_send(client, &dbbus_tx_data, 1);
		if(wrtval <0)
		{
		  pr_err("sx9310_probe  i2c_master_send error:\n");
		  return -EIO;
		}
		else
		{
		    pr_debug("sx9310_probe  i2c_master_send ok:\n");
		}
// fanhonggen check i2c  end 20160308

	of_get_sx9310_platform_data(&client->dev);
	pplatData = &sx9310_config;//client->dev.platform_data;

	if (!pplatData) {
		pr_err("platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	this = kzalloc(sizeof(sx93XX_t), GFP_KERNEL); /* create memory for main struct */
	//pr_err("\t Initialized Main Memory: 0x%p\n",this);
  
	if (this)
	{
		/* In case we need to reinitialize data 
		* (e.q. if suspend reset device) */
		this->init = initialize;
		/* shortcut to read status of interrupt */
		this->refreshStatus = read_regStat_v1;
		/* pointer to function from platform data to get pendown 
		* (1->NIRQ=0, 0->NIRQ=1) */
		
	//	this->get_nirq_low = pplatData->get_is_nirq_low;
		
		/* save irq in case we need to reference it */
		this->irq = sx9310_irq_gpio;//client->irq;
		/* do we need to create an irq timer after interrupt ? */
		this->useIrqTimer = 0;

		/* Setup function to call on corresponding reg irq source bit */
		if (MAX_NUM_STATUS_BITS>= 8)
		{
			this->statusFunc[0] = 0; /* TXEN_STAT */
			this->statusFunc[1] = 0; /* UNUSED */
			this->statusFunc[2] = 0; /* UNUSED */
			this->statusFunc[3] = read_rawData; /* CONV_STAT */
			this->statusFunc[4] = read_regStat; /* COMP_STAT */
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
		this->pDevice = pDevice = kzalloc(sizeof(sx9310_t), GFP_KERNEL);
		pr_debug("\t Initialized Device Specific Memory: 0x%p\n",pDevice);

		if (pDevice)
		{
			/* for accessing items in user data (e.g. calibrate) */
			err =sysfs_create_group(&client->dev.kobj, &sx9310_attr_group);
		 	if (err)
			{
				dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed.\n", __func__);
				//sysfs_remove_group(&client->dev.kobj, &sx9310_attr_group);
				return -EIO;
			}
			/* Check if we hava a platform initialization function to call*/
			if (pplatData->init_platform_hw)
			pplatData->init_platform_hw();

			/* Add Pointer to main platform data struct */
			pDevice->hw = pplatData;

			/* Initialize the button information initialized with keycodes */
			pDevice->pbuttonInformation = pplatData->pbuttonInformation;

			/* Create the input device */
			input = input_allocate_device();
			if (!input) {
				return -ENOMEM;
			}

			/* Set all the keycodes */
			__set_bit(EV_KEY, input->evbit);
			for (i = 0; i < pDevice->pbuttonInformation->buttonSize; i++) {
				__set_bit(pDevice->pbuttonInformation->buttons[i].keycode, input->keybit);
				__set_bit(pDevice->pbuttonInformation->buttons[i].keycode_release, input->keybit);
				pDevice->pbuttonInformation->buttons[i].state = IDLE;
			}
			/* save the input pointer and finish initialization */
			pDevice->pbuttonInformation->input = input;
			input->name = "SX9310_Cap_Touch";
			input->id.bustype = BUS_I2C;
			if(input_register_device(input))
				return -ENOMEM;
		}
		sx93XX_init(this);
		return  0;
	}
	return -1;
}

/*! \fn static int sx9310_remove(struct i2c_client *client)
 * \brief Called when device is to be removed
 * \param client Pointer to i2c_client struct
 * \return Value from sx93XX_remove()
 */
static int sx9310_remove(struct i2c_client *client)
{
	psx9310_platform_data_t pplatData =0;
	psx9310_t pDevice = 0;
	psx93XX_t this = i2c_get_clientdata(client);
	if (this && (pDevice = this->pDevice))
	{
		input_unregister_device(pDevice->pbuttonInformation->input);
		//sysfs_remove_group(&client->dev.kobj, &sx9310_attr_group);
		//device_remove_file(&client->dev, &dev_attr_calibrate);
		//	device_remove_file(&client->dev, &dev_attr_sx9310_reg);
		pplatData = &sx9310_config;//client->dev.platform_data;
		if (pplatData && pplatData->exit_platform_hw)
			pplatData->exit_platform_hw();
		kfree(this->pDevice);
	}
	return sx93XX_remove(this);
}

#if 0
/*====================================================*/
/***** Kernel Suspend *****/
static int sx9310_suspend(struct i2c_client *client,pm_message_t mesg)
{
  //psx93XX_t this = i2c_get_clientdata(client);
 // manual_offset_calibration(this);
// sx93XX_suspend(this);
 return 0;
}
/***** Kernel Resume *****/
static int sx9310_resume(struct i2c_client *client)
{
  //psx93XX_t this = i2c_get_clientdata(client);
 // manual_offset_calibration(this);
  //sx93XX_resume(this);
  return 0;
}
#endif

#if 1
/*====================================================*/
static struct i2c_device_id sx9310_idtable[] = {
	{ DRIVER_NAME, 0 },
	{ }
};
//MODULE_DEVICE_TABLE(i2c, sx9310_idtable);
#endif

static const struct of_device_id sx9310_dt_ids[] = {
	{ .compatible = "mediatek,sx9310", },
	{ }
};

static unsigned short force[] = { 0, 0x28, I2C_CLIENT_END, I2C_CLIENT_END };
static const unsigned short *const forces[] = { force, NULL };


static struct i2c_driver sx9310_driver = {
	.probe = sx9310_probe,
	.remove = sx9310_remove,
	.detect = sx9310_i2c_detect,
	.driver.name = DRIVER_NAME,
	.driver = {
		   .name = DRIVER_NAME,
		   .of_match_table = sx9310_dt_ids,
		   },
	.id_table = sx9310_idtable,
	.address_list = (const unsigned short *)forces,
#if 0// defined(USE_KERNEL_SUSPEND)
  .suspend  = sx9310_suspend,
  .resume   = sx9310_resume,
#endif
};


#ifdef USE_THREADED_IRQ
static void sx93XX_process_interrupt(psx93XX_t this,u8 nirqlow)
{
  int status = 0;
  int counter = 0;
  if (!this) {
    pr_err(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
    return;
  }
    /* since we are not in an interrupt don't need to disable irq. */
  status = this->refreshStatus(this);
  counter = -1;
	pr_err("Worker - Refresh Status %d\n",status);

  while((++counter) < MAX_NUM_STATUS_BITS) { /* counter start from MSB */
	  pr_err("Looping Counter %d\n",counter);
    if (((status>>counter) & 0x01) && (this->statusFunc[counter])) {
	    pr_err("Function Pointer Found. Calling\n");
      this->statusFunc[counter](this);
    }
  }
  if (unlikely(this->useIrqTimer && nirqlow)) {
    /* In case we need to send a timer for example on a touchscreen
     * checking penup, perform this here
     */
    cancel_delayed_work(&this->dworker);
    schedule_delayed_work(&this->dworker,msecs_to_jiffies(this->irqTimeout));
    pr_err("Schedule Irq timer");
  } 
}


static void sx93XX_worker_func(struct work_struct *work)
{
  psx93XX_t this = 0;
  if (work) {
    this = container_of(work,sx93XX_t,dworker.work);
    if (!this) {
      pr_err(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
      return;
    }
    if ((!this->get_nirq_low) || (!this->get_nirq_low())) {
      /* only run if nirq is high */
      sx93XX_process_interrupt(this,0);
    }
  } else {
    pr_err(KERN_ERR "sx93XX_worker_func, NULL work_struct\n");
  }
}
static irqreturn_t sx93XX_interrupt_thread(int irq, void *data)
{
  psx93XX_t this = 0;
  this = data;
  mutex_lock(&this->mutex);
  pr_err("sx93XX_irq\n");
  if ((!this->get_nirq_low) || this->get_nirq_low()) {
    sx93XX_process_interrupt(this,1);
  }
  else
	  pr_err("sx93XX_irq - nirq read high\n");
  mutex_unlock(&this->mutex);
  return IRQ_HANDLED;
}
#else
static void sx93XX_schedule_work(psx93XX_t this, unsigned long delay)
{
  unsigned long flags;
  if (this) {
	 pr_debug("sx93XX_schedule_work()\n");
   spin_lock_irqsave(&this->lock,flags);
   /* Stop any pending penup queues */
   cancel_delayed_work(&this->dworker);
   //after waiting for a delay, this put the job in the kernel-global workqueue. so no need to create new thread in work queue.
   schedule_delayed_work(&this->dworker,delay);
   spin_unlock_irqrestore(&this->lock,flags);
  }
  else
    pr_err(KERN_ERR "sx93XX_schedule_work, NULL psx93XX_t\n");
} 

static irqreturn_t sx93XX_irq(int irq, void *pvoid)
{
	
	psx93XX_t this ;
	pr_debug("enter sx93XX_irq\n");
	if (pvoid) {
		this = (psx93XX_t)pvoid;
		//if ((!this->get_nirq_low) || this->get_nirq_low()) {
		pr_err("sx93XX_irq - Schedule Work\n");
		sx93XX_schedule_work(this,0);
		//}
		// else
		// pr_err("sx93XX_irq - nirq read high\n");
	}
	else
		pr_err(KERN_ERR "sx93XX_irq, NULL pvoid\n");
	return IRQ_HANDLED;
}

static void sx93XX_worker_func(struct work_struct *work)
{
  psx93XX_t this = 0;
  int status = 0;
  int counter = 0;
  //u8 nirqLow = 0;
  if (work) {
    this = container_of(work,sx93XX_t,dworker.work);
    if (!this) {
      pr_err(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
      return;
    }
	/*
    if (unlikely(this->useIrqTimer)) {
      if ((!this->get_nirq_low) || this->get_nirq_low()) {
        nirqLow = 1;
      }
    }*/
    /* since we are not in an interrupt don't need to disable irq. */
    status = this->refreshStatus(this);
    counter = -1;
    pr_err("Worker - Refresh Status %d\n",status);
    while((++counter) < MAX_NUM_STATUS_BITS) { /* counter start from MSB */
	  pr_debug("Looping Counter %d\n",counter);
      if (((status>>counter) & 0x01) && (this->statusFunc[counter])) {
			pr_debug("kecode counter %d Function Pointer Found. Calling\n",counter);
			this->statusFunc[counter](this);
      }
    }
//    if (unlikely(this->useIrqTimer && nirqLow))
   if (unlikely(this->useIrqTimer ))
    { /* Early models and if RATE=0 for newer models require a penup timer */
      /* Queue up the function again for checking on penup */
		sx93XX_schedule_work(this,msecs_to_jiffies(this->irqTimeout));
    }
  } else {
    pr_err(KERN_ERR "sx93XX_worker_func, NULL work_struct\n");
  }
}
#endif




void sx93XX_suspend(psx93XX_t this)
{
  if (this)
    disable_irq(this->irq);
}
void sx93XX_resume(psx93XX_t this)
{
  if (this) {
#ifdef USE_THREADED_IRQ
  mutex_lock(&this->mutex);
  /* Just in case need to reset any uncaught interrupts */
  sx93XX_process_interrupt(this,0);
  mutex_unlock(&this->mutex);
#else
  sx93XX_schedule_work(this,0);
#endif
    if (this->init)
      this->init(this);
    enable_irq(this->irq);
  }
}

#if 0//def CONFIG_HAS_WAKELOCK
/*TODO: Should actually call the device specific suspend/resume
 * As long as the kernel suspend/resume is setup, the device
 * specific ones will be called anyways
 */
extern suspend_state_t get_suspend_state(void);
void sx93XX_early_suspend(struct early_suspend *h)
{
	psx93XX_t this = 0;
  pr_err("inside sx93XX_early_suspend()\n");
	this = container_of(h, sx93XX_t, early_suspend);
  sx93XX_suspend(this);
  pr_err("exit sx93XX_early_suspend()\n");
}

void sx93XX_late_resume(struct early_suspend *h)
{
	psx93XX_t this = 0;
  pr_err("inside sx93XX_late_resume()\n");
	this = container_of(h, sx93XX_t, early_suspend);
  sx93XX_resume(this);
  pr_err("exit sx93XX_late_resume()\n");
}
#endif

int sx93XX_init(psx93XX_t this)
{
  int err = 0;
  if (this && this->pDevice)
  {

#ifdef USE_THREADED_IRQ

    /* initialize worker function */
	  INIT_DELAYED_WORK(&this->dworker, sx93XX_worker_func);


    /* initialize mutex */
    mutex_init(&this->mutex);
    /* initailize interrupt reporting */
    this->irq_disabled = 0;
	  err = request_threaded_irq(this->irq, NULL, sx93XX_interrupt_thread,
                              IRQF_TRIGGER_FALLING, this->pdev->driver->name,
                              this);
#else
    /* initialize spin lock */
  	spin_lock_init(&this->lock);

    /* initialize worker function */
	INIT_DELAYED_WORK(&this->dworker, sx93XX_worker_func);

    /* initailize interrupt reporting */
    this->irq_disabled = 0;
	err = request_irq(this->irq, sx93XX_irq, IRQF_TRIGGER_FALLING,this->pdev->driver->name, this);
#endif
	  if (err) {
		  pr_err("irq %d busy?\n", this->irq);
		  return err;
    }
#ifdef USE_THREADED_IRQ
    pr_err( "registered with threaded irq (%d)\n", this->irq);
#else
    pr_err( "registered with irq (%d)\n", this->irq);
#endif
#if 0//def CONFIG_HAS_WAKELOCK	
    this->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    this->early_suspend.suspend = sx93XX_early_suspend;
    this->early_suspend.resume = sx93XX_late_resume;
    register_early_suspend(&this->early_suspend);
    if (has_wake_lock(WAKE_LOCK_SUSPEND) == 0 && 
                            get_suspend_state() == PM_SUSPEND_ON)
     	sx93XX_early_suspend(&this->early_suspend);
#endif //CONFIG_HAS_WAKELOCK
    /* call init function pointer (this should initialize all registers */
    if (this->init)
      return this->init(this);
    pr_err("No init function!!!!\n");
	}
	return -ENOMEM;
}

int sx93XX_remove(psx93XX_t this)
{
  if (this) {
    cancel_delayed_work_sync(&this->dworker); /* Cancel the Worker Func */
    /*destroy_workqueue(this->workq); */
#if 0//def CONFIG_HAS_WAKELOCK
    unregister_early_suspend(&this->early_suspend);
#endif
    free_irq(this->irq, this);
    kfree(this);
    return 0;
  }
  return -ENOMEM;
}

static int __init sx9310_init(void)
{
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
	return 0;
	
	pr_debug("sx9310_init, ---here\n");
	return 	i2c_add_driver(&sx9310_driver);
}
static void __exit sx9310_exit(void)
{
	pr_debug("sx9310_exit, ---here\n");
	i2c_del_driver(&sx9310_driver);
}

module_init(sx9310_init);
//late_initcall(sx9310_init);
module_exit(sx9310_exit);

MODULE_AUTHOR("Semtech Corp. (http://www.semtech.com/)");
MODULE_DESCRIPTION("mediatek,sx9310");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

