//tuwenzan@wind-mobi.com modify at 20161111 begin
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
#define DEBUG
#define DRIVER_NAME "sx9310"

#define MAX_WRITE_ARRAY_SIZE 32
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/string.h> //tuwenzan@wind-mobi.com add at 20161121


#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* main struct, interrupt,init,pointers */
#include "sx9310.h" 

#define IDLE 0
#define ACTIVE 1
#define USED 0
static int flag_mode = 1;	//tuwenzan@wind-mobi.com add at 20170114 begin
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
      dev_dbg(this->pdev, "ForcetoTouched()\n");
    
      pCurrentButton = pDevice->pbuttonInformation->buttons;
      input = pDevice->pbuttonInformation->input;

      input_report_key(input, pCurrentButton->keycode, 1);
      pCurrentButton->state = ACTIVE;

      input_sync(input);

	  dev_dbg(this->pdev, "Leaving ForcetoTouched()\n");
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
	  dev_dbg(&i2c->dev,"write_register Address: 0x%x Value: 0x%x Return: %d\n",
        address,value,returnValue);
  }
  if(returnValue < 0){
  	ForcetoTouched(this);
	dev_info( this->pdev, "Write_register-ForcetoTouched()\n");
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
	  dev_dbg(&i2c->dev, "twz read_register Address: 0x%x Return: 0x%x\n",address,returnValue);
    if (returnValue >= 0) {
      *value = returnValue;
      return 0;
    } else {
      return returnValue;
    }
  }
  ForcetoTouched(this);
  dev_info( this->pdev, "read_register-ForcetoTouched()\n");
  return -ENOMEM;
}


/*! \brief Sends a write register range to the device
 * \param this Pointer to main parent struct 
 * \param reg 8-bit register address (base address)
 * \param data pointer to 8-bit register values
 * \param size size of the data pointer
 * \return Value from i2c_master_send
 */
#if USED
static int write_registerEx(psx93XX_t this, unsigned char reg,
				unsigned char *data, int size)
{
  struct i2c_client *i2c = 0;
	u8 tx[MAX_WRITE_ARRAY_SIZE];
	int ret = 0;

  if (this && (i2c = this->bus) && data && (size <= MAX_WRITE_ARRAY_SIZE))
  {
    dev_dbg(this->pdev, "inside write_registerEx()\n");
    tx[0] = reg;
    dev_dbg(this->pdev, "going to call i2c_master_send(0x%p, 0x%x ",
            (void *)i2c,tx[0]);
    for (ret = 0; ret < size; ret++)
    {
      tx[ret+1] = data[ret];
      dev_dbg(this->pdev, "0x%x, ",tx[ret+1]);
    }
    dev_dbg(this->pdev, "\n");

    ret = i2c_master_send(i2c, tx, size+1 );
	  if (ret < 0)
	  	dev_err(this->pdev, "I2C write error\n");
  }
  dev_dbg(this->pdev, "leaving write_registerEx()\n");
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
    dev_dbg(this->pdev, "inside read_registerEx()\n");
    dev_dbg(this->pdev,
        "going to call i2c_master_send(0x%p,0x%p,1) Reg: 0x%x\n",
                                                               (void *)i2c,(void *)tx,tx[0]);
  	ret = i2c_master_send(i2c,tx,1);
  	if (ret >= 0) {
      dev_dbg(this->pdev, "going to call i2c_master_recv(0x%p,0x%p,%x)\n",
                                                              (void *)i2c,(void *)data,size);
  		ret = i2c_master_recv(i2c, data, size);
    }
  }
	if (unlikely(ret < 0))
		dev_err(this->pdev, "I2C read error\n");
  	dev_dbg(this->pdev, "leaving read_registerEx()\n");
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

  	dev_dbg(this->pdev, "Reading IRQSTAT_REG\n");
  	read_register(this,SX9310_IRQSTAT_REG,&reg_value);
//  	printk("twz reg_calue = 0x%x\n",reg_value);
	return snprintf(buf, 2,"0x%x\n", reg_value);  //sprintf(buf, "0x%x\n", reg_value);
}
//tuwenzan@wind-mobi.com add at 20170114 begin
static ssize_t all_register_value_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	psx9310_t pDevice = 0;
  	psx9310_platform_data_t pdata = 0;
	int i = 0;
	char* cache_buf = NULL;
	char* single_buf;
	u8 reg_value = 0;
	unsigned char reg_addr[] = {0x00,0x01,0x02,0x03,0x04,0x10,0x11,0x12,0x13,0x14,
					 0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,
					 0x1F,0x20,0x21,0x22,0x23,0x2A,0x2B,0x2C,0x30,0x31,
					 0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x40,
					 0x41,0x42,0x7F};
	psx93XX_t this = dev_get_drvdata(dev);
	//printk("twz enter all_register_value_show\n");
	cache_buf = (char *)kmalloc(1024,GFP_KERNEL);
	memset(cache_buf,0,1024);
	single_buf = (char *)kmalloc(20,GFP_KERNEL);
	memset(single_buf,0,20);
  	if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw))
  	{
   	   while ( i < ARRAY_SIZE(reg_addr)) {
      	/* read all registers/values contained in i2c_reg */    
      read_register(this, reg_addr[i],&reg_value);
	  sprintf(single_buf,"REG[0x%x] = 0x%x\n",reg_addr[i],reg_value);
	  strcat(cache_buf,single_buf);
      i++;
    }
  } else {
  	printk("twz ERROR! platform data 0x%p\n",pDevice->hw);
  	}
 	return sprintf(buf,"As follow is SX9301 all init reg list:\n%s\n",cache_buf);
}

static ssize_t sar_enable_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%d\n",flag_mode);
}

//tuwenzan@wind-mobi.com add at 20170114 end


/*! \brief sysfs store function for manual calibration
 */
static ssize_t manual_offset_calibration_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long val;
	psx93XX_t this = dev_get_drvdata(dev);
	if (kstrtoul(buf, 0, &val))  // twz  strict_strtoul kstrtoul
		return -EINVAL;
  if (val) {
    dev_info( this->pdev, "Performing manual_offset_calibration()\n");
    manual_offset_calibration(this);
  }
	return count;
}
//tuwenzan@wind-mobi.com add at 20170114 begin
static ssize_t operation_single_store(struct device *dev,
					struct device_attribute *attr,const char *buf, size_t count)
{
	char* tab = ",";
	char* second_buf;
	u8 reg = 0;
	u8 reg_value = 0;
	psx93XX_t this = dev_get_drvdata(dev);
	//printk("twz enter operation_single_strore\n");
	second_buf = strpbrk(buf,tab) + 1;
	reg = simple_strtoul(buf,0,16);
	reg_value = simple_strtoul(second_buf,0,16);
	printk("twz 0x%x 0x%x\n",reg,reg_value);
	write_register(this,reg,reg_value);
	return count;
}

static ssize_t sar_enable_store(struct device *dev,
					struct device_attribute *attr,const char *buf, size_t count)
{
	u8 enable = 1;
	u8 disable = 0;
	psx93XX_t this = dev_get_drvdata(dev);
	if(strncmp(buf,"1",1) == 0){
		printk("tuwenzan sar you echo 1\n");
		write_register(this,SX9310_SAR_MODE_REG,enable);
		enable_irq(this->irq);
		flag_mode = 1;
		return count;
	}
	if(strncmp(buf,"0",1) == 0){
		printk("tuwenzan sar you echo 0\n");
		write_register(this,SX9310_SAR_MODE_REG,disable);
		disable_irq(this->irq);
		flag_mode = 0;
		return count;
	}
	return count;
}

static DEVICE_ATTR(calibrate, 0644, manual_offset_calibration_show,
                                manual_offset_calibration_store);
static DEVICE_ATTR(all_register_value,0644,all_register_value_show,NULL);
static DEVICE_ATTR(operation_single_reg,0644,NULL,operation_single_store);
static DEVICE_ATTR(sar_enable,0644,sar_enable_show,sar_enable_store);


static struct attribute *sx9310_attributes[] = {
	&dev_attr_calibrate.attr,
	&dev_attr_all_register_value.attr,
	&dev_attr_operation_single_reg.attr,
	&dev_attr_sar_enable.attr,
	NULL,
};
//tuwenzan@wind-mobi.com add at 20170114 end
static struct attribute_group sx9310_attr_group = {
	.attrs = sx9310_attributes,
};


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
    if (read_register(this,SX9310_IRQSTAT_REG,&data) == 0)
      return (data & 0x00FF);
  }
  return 0;
}


/*! \brief  Initialize I2C config from platform data
 * \param this Pointer to main parent struct 
 */
static void hw_init(psx93XX_t this)
{
  psx9310_t pDevice = 0;
  psx9310_platform_data_t pdata = 0;
	int i = 0;
	/* configure device */
  dev_dbg(this->pdev, "Going to Setup I2C Registers\n");
  if (this && (pDevice = this->pDevice) && (pdata = pDevice->hw))
  {
    while ( i < pdata->i2c_reg_num) {
      /* Write all registers/values contained in i2c_reg */
      dev_dbg(this->pdev, "Going to Write Reg: 0x%x Value: 0x%x\n",
                pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
//      msleep(3);        
      write_register(this, pdata->pi2c_reg[i].reg,pdata->pi2c_reg[i].val);
      i++;
    }
  } else {
  dev_err(this->pdev, "ERROR! platform data 0x%p\n",pDevice->hw);
  //Force to touched if error
  ForcetoTouched(this);
  dev_info( this->pdev, "Hardware_init-ForcetoTouched()\n");
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
  if (this) {
    /* prepare reset by disabling any irq handling */
    this->irq_disabled = 1;
    disable_irq(this->irq);
    /* perform a reset */
    write_register(this,SX9310_SOFTRESET_REG,SX9310_SOFTRESET);
    /* wait until the reset has finished by monitoring NIRQ */
    dev_dbg(this->pdev, "Sent Software Reset. Waiting until device is back from reset to continue.\n");
    /* just sleep for awhile instead of using a loop with reading irq status */
    msleep(300);
//    while(this->get_nirq_low && this->get_nirq_low()) { read_regStat(this); }
    dev_dbg(this->pdev, "Device is back from the reset, continuing. NIRQ = %d\n",this->get_nirq_low());
    hw_init(this);
    msleep(100); /* make sure everything is running */
    manual_offset_calibration(this);
    
    /* re-enable interrupt handling */
    enable_irq(this->irq);
    this->irq_disabled = 0;
   
    /* make sure no interrupts are pending since enabling irq will only
     * work on next falling edge */
    read_regStat(this);
    dev_dbg(this->pdev, "Exiting initialize(). NIRQ = %d\n",this->get_nirq_low());
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
    dev_dbg(this->pdev, "9310 Inside touchProcess()\n");
    read_register(this, SX9310_STAT0_REG, &i);

    buttons = pDevice->pbuttonInformation->buttons;
    input = pDevice->pbuttonInformation->input;
    numberOfButtons = pDevice->pbuttonInformation->buttonSize;
    
    if (unlikely( (buttons==NULL) || (input==NULL) )) {
      dev_err(this->pdev, "ERROR!! buttons or input NULL!!!\n");
      return;
    }

    for (counter = 0; counter < numberOfButtons; counter++) {
      pCurrentButton = &buttons[counter];
      if (pCurrentButton==NULL) {
        dev_err(this->pdev,"ERROR!! current button at index: %d NULL!!!\n",
                                                                      counter);
        return; // ERRORR!!!!
      }
      switch (pCurrentButton->state) {
        case IDLE: /* Button is not being touched! */
          if (((i & pCurrentButton->mask) == pCurrentButton->mask)) {
            /* User pressed button */
            dev_info(this->pdev, "cap button %d touched\n", counter);
            input_report_key(input, pCurrentButton->keycode, 1);
/******add by tuwenzan@wind-mobi.cm----20170114----start************/			
			input_sync(input);
			input_report_key(input, pCurrentButton->keycode, 0);
			input_sync(input);
/******add by tuwenzan@wind-mobi.cm----20170114----end************/			
            pCurrentButton->state = ACTIVE;
          } else {
            dev_dbg(this->pdev, "Button %d already released.\n",counter);
          }
          break;
        case ACTIVE: /* Button is being touched! */ 
          if (((i & pCurrentButton->mask) != pCurrentButton->mask)) {
            /* User released button */
            dev_info(this->pdev, "cap button %d released\n",counter);
/******add by tuwenzan@wind-mobi.cm----20170114----start************/			
			input_report_key(input, KEY_BRL_DOT6, 1);
			input_sync(input);
            input_report_key(input, KEY_BRL_DOT6, 0);
			input_sync(input);
			//input_report_key(input, pCurrentButton->keycode, 0);
/******add by tuwenzan@wind-mobi.cm----20170114----end************/			
            pCurrentButton->state = IDLE;
          } else {
            dev_dbg(this->pdev, "Button %d still touched.\n",counter);
          }
          break;
        default: /* Shouldn't be here, device only allowed ACTIVE or IDLE */
          break;
      };
    }
/******add by tuwenzan@wind-mobi.cm----20170114----start************/		
//    input_sync(input);
/******add by tuwenzan@wind-mobi.cm----20170114----end************/	
	  dev_dbg(this->pdev, "Leaving touchProcess()\n");
  }
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
	int retval = 0;
 	psx93XX_t this = 0;
 	psx9310_t pDevice = 0;
	psx9310_platform_data_t pplatData = 0;
 	struct input_dev *input = NULL;
	printk("twz enter sx9310 probe client addr = %x\n",client->addr);
	dev_info(&client->dev, "sx9310_probe()\n");
 	//pplatData = client->dev.platform_data;
 	pplatData = &sx9310_config;
	if (!pplatData) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

  	this = kzalloc(sizeof(sx93XX_t), GFP_KERNEL); /* create memory for main struct */
	dev_dbg(&client->dev, "\t Initialized Main Memory: 0x%p\n",this);
  
  if (this)
  {
    /* In case we need to reinitialize data 
     * (e.q. if suspend reset device) */
    this->init = initialize;
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
      this->statusFunc[3] = 0; /* CONV_STAT */
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
    this->pDevice = pDevice = kzalloc(sizeof(sx9310_t), GFP_KERNEL);
	  dev_dbg(&client->dev, "\t Initialized Device Specific Memory: 0x%p\n",pDevice);

    if (pDevice)
    {
      /* for accessing items in user data (e.g. calibrate) */
      retval = sysfs_create_group(&client->dev.kobj, &sx9310_attr_group);
	if (retval < 0) {
		printk("twz fail to creat sysfs file\n");
		return 0;
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
        __set_bit(pDevice->pbuttonInformation->buttons[i].keycode, 
                                                        input->keybit);
        pDevice->pbuttonInformation->buttons[i].state = IDLE;
      }
      /* save the input pointer and finish initialization */
      pDevice->pbuttonInformation->input = input;
      input->name = "SX9310 Cap Touch";
      input->id.bustype = BUS_I2C;
      if(input_register_device(input))
        return -ENOMEM;
    }
	client->dev.driver_data = this;  //tuwenzan@wind-mobi.com add this at 20161112
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

    sysfs_remove_group(&client->dev.kobj, &sx9310_attr_group);
    //pplatData = client->dev.platform_data;
    pplatData = &sx9310_config;
    if (pplatData && pplatData->exit_platform_hw)
      pplatData->exit_platform_hw();
    kfree(this->pDevice);
  }
	return sx93XX_remove(this);
}


/***** Kernel Suspend *****/
static int sx9310_suspend(struct i2c_client *client,pm_message_t mesg)
{
  psx93XX_t this = i2c_get_clientdata(client);
  sx93XX_suspend(this);
 return 0;
}

/***** Kernel Resume *****/
static int sx9310_resume(struct i2c_client *client)
{
  psx93XX_t this = i2c_get_clientdata(client);
  sx93XX_resume(this);
  return 0;
}



static struct i2c_device_id sx9310_idtable[] = {
	{ DRIVER_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static const struct of_device_id sar_of_match[] = {
{.compatible = "mediatek,sar"},
	{},
};

struct of_device_id sar_eint_of_match[] = {
	{ .compatible =  "mediatek, sar-eint", },
	{},
};
#endif

MODULE_DEVICE_TABLE(i2c, sx9310_idtable);

static struct i2c_driver sx9310_driver = {
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = sar_of_match,
#endif
		   },
	.id_table = sx9310_idtable,
	.probe	  = sx9310_probe,
	.remove   = sx9310_remove,
    .suspend  = sx9310_suspend,
    .resume   = sx9310_resume,
};


#ifdef USE_THREADED_IRQ
static void sx93XX_process_interrupt(psx93XX_t this,u8 nirqlow)
{
  int status = 0;
  int counter = 0;
  if (!this) {
    printk(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
    return;
  }
    /* since we are not in an interrupt don't need to disable irq. */
  status = this->refreshStatus(this);
  counter = -1;
	dev_dbg(this->pdev, "Worker - Refresh Status %d\n",status);

  while((++counter) < MAX_NUM_STATUS_BITS) { /* counter start from MSB */
	  dev_dbg(this->pdev, "Looping Counter %d\n",counter);
    if (((status>>counter) & 0x01) && (this->statusFunc[counter])) {
	    dev_dbg(this->pdev, "Function Pointer Found. Calling\n");
      this->statusFunc[counter](this);
    }
  }
  if (unlikely(this->useIrqTimer && nirqlow)) {
    /* In case we need to send a timer for example on a touchscreen
     * checking penup, perform this here
     */
    cancel_delayed_work(&this->dworker);
    schedule_delayed_work(&this->dworker,msecs_to_jiffies(this->irqTimeout));
    dev_info(this->pdev,"Schedule Irq timer");
  } 
}


static void sx93XX_worker_func(struct work_struct *work)
{
  psx93XX_t this = 0;
  if (work) {
    this = container_of(work,sx93XX_t,dworker.work);
    if (!this) {
      printk(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
      return;
    }
    if ((!this->get_nirq_low) || (!this->get_nirq_low())) {
      /* only run if nirq is high */
      sx93XX_process_interrupt(this,0);
    }
  } else {
    printk(KERN_ERR "sx93XX_worker_func, NULL work_struct\n");
  }
}


static irqreturn_t sx93XX_interrupt_thread(int irq, void *data)
{
  psx93XX_t this = 0;
  this = data;
  mutex_lock(&this->mutex);
  dev_dbg(this->pdev, "sx93XX_irq\n");
  if ((!this->get_nirq_low) || this->get_nirq_low()) {
    sx93XX_process_interrupt(this,1);
  }
  else
	  dev_err(this->pdev, "sx93XX_irq - nirq read high\n");
  mutex_unlock(&this->mutex);
  return IRQ_HANDLED;
}
#else
static void sx93XX_schedule_work(psx93XX_t this, unsigned long delay)
{
  unsigned long flags;
  if (this) {
	 dev_dbg(this->pdev, "sx93XX_schedule_work()\n");
   spin_lock_irqsave(&this->lock,flags);
   /* Stop any pending penup queues */
   cancel_delayed_work(&this->dworker);
   //after waiting for a delay, this put the job in the kernel-global workqueue. so no need to create new thread in work queue.
   schedule_delayed_work(&this->dworker,delay);
   spin_unlock_irqrestore(&this->lock,flags);
  }
  else
    printk(KERN_ERR "sx93XX_schedule_work, NULL psx93XX_t\n");
} 

static irqreturn_t sx93XX_irq(int irq, void *pvoid)
{
  psx93XX_t this = 0;
  if (pvoid) {
    this = (psx93XX_t)pvoid;
	  dev_dbg(this->pdev, "sx93XX_irq\n");
    if ((!this->get_nirq_low) || this->get_nirq_low()) {
	    dev_dbg(this->pdev, "sx93XX_irq - Schedule Work\n");
      sx93XX_schedule_work(this,0);
    }
    else
	    dev_err(this->pdev, "sx93XX_irq - nirq read high\n");
  }
  else
    printk(KERN_ERR "sx93XX_irq, NULL pvoid\n");
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
      printk(KERN_ERR "sx93XX_worker_func, NULL sx93XX_t\n");
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
	  dev_dbg(this->pdev, "Worker - Refresh Status %d\n",status);
    while((++counter) < MAX_NUM_STATUS_BITS) { /* counter start from MSB */
	    dev_dbg(this->pdev, "Looping Counter %d\n",counter);
      if (((status>>counter) & 0x01) && (this->statusFunc[counter])) {
	      dev_dbg(this->pdev, "Function Pointer Found. Calling\n");
        this->statusFunc[counter](this);
      }
    }
    if (unlikely(this->useIrqTimer && nirqLow))
    { /* Early models and if RATE=0 for newer models require a penup timer */
      /* Queue up the function again for checking on penup */
      sx93XX_schedule_work(this,msecs_to_jiffies(this->irqTimeout));
    }
  } else {
    printk(KERN_ERR "sx93XX_worker_func, NULL work_struct\n");
  }
}
#endif

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

//tuwenzan@wind-mobi.com modify at 20170114 begin
void sx93XX_suspend(psx93XX_t this)
{
  if (this){
  	write_register(this,SX9310_SAR_MODE_REG,0);
    disable_irq(this->irq);
 }
}

void sx93XX_resume(psx93XX_t this)
{
  if (this && flag_mode) {
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
//tuwenzan@wind-mobi.com modify at 20170114 end


int sx93XX_init(psx93XX_t this)
{
	int err = 0;
	struct device_node *node = NULL;
	u32 ints[2] = { 0, 0 };
	
	node = of_find_matching_node(node, sar_eint_of_match);
//	printk("twz enter sx93XX_init\n");
	if (node && this && this->pDevice) {

		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);
		this->irq = irq_of_parse_and_map(node, 0);
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
	  err = request_irq(this->irq, sx93XX_irq, IRQF_TRIGGER_FALLING,
		                          	this->pdev->driver->name, this);
#endif
	  if (err) {
		  dev_err(this->pdev, "irq %d busy?\n", this->irq);
		  return err;
    }
    /* call init function pointer (this should initialize all registers */
    if (this->init)
      return this->init(this);
    dev_err(this->pdev,"No init function!!!!\n");
	} 
	return -ENOMEM;
}

static int __init sx9310_init(void)
{
	return i2c_add_driver(&sx9310_driver);
}
static void __exit sx9310_exit(void)
{
	i2c_del_driver(&sx9310_driver);
}

module_init(sx9310_init);
module_exit(sx9310_exit);

MODULE_AUTHOR("tuwenzan@wind-mobi.com");
MODULE_DESCRIPTION("SX9310 Capacitive Touch Controller Driver");
MODULE_LICENSE("GPL");
//tuwenzan@wind-mobi.com modify at 20161111 end

