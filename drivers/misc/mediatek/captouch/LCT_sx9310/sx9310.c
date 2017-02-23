/* drivers/hwmon/mt6516/amit/IQS128.c - IQS128/PS driver
 * 
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>

#include <linux/io.h>
#include "sx9310.h"
#include "upmu_sw.h"
#include "upmu_common.h"
#include <linux/gpio.h>
#include <linux/of_irq.h>

#include <linux/wakelock.h>
#include <linux/sched.h>

#include <linux/dma-mapping.h>
#include <linux/switch.h>
#include <captouch.h>

/******************************************************************************
 * configuration
*******************************************************************************/
#define SX9311_DEV_NAME     "SX9310"

#define SX9311_STATUS_NEAR 112
#define SX9311_STATUS_FAR 113
/******************************************************************************
 * extern functions
*******************************************************************************/
static DEFINE_MUTEX(SX9311_mutex);

/*----------------------------------------------------------------------------*/
#define CAPTOUCH_EINT_TOUCH	(1)
#define CAPTOUCH_EINT_NO_TOUCH	(0)
#define SX9311_SUPPORT_I2C_DMA

/*LCT add capsensor by cly */
#define CAPSENSOR_NEED_ENABLE_SWITCH

#ifdef CAPSENSOR_NEED_ENABLE_SWITCH
#define CAPSENSOR_ENABLE_PROC_FILE "capsensor_enable"
#endif
/*END LCT*/
#ifdef LCT_LOG_MOD
#undef LCT_LOG_MOD
#define LCT_LOG_MOD "[CAPSENSOR]"
#endif

//lct_pr_debug("%s buf: %s \n",__func__,buf);
/*----------------------------------------------------------------------------*/
static struct work_struct captouch_eint_work;
int captouch_eint_status=0;
int captouch_irq;
unsigned int captouch_gpiopin;
unsigned int captouch_debounce;
unsigned int captouch_eint_type;
//static int captouch_init_flag = 0;

static struct i2c_client *SX9311_i2c_client;

static const struct i2c_device_id SX9311_i2c_id[] = { {"SX9310", 0}, {} };

//add for capsensor for switch_dev
static struct switch_dev capsensor_data;

static struct mutex mtx_eint_status;

static int read_regStat(void); 
enum capsensor_report_state{
       CAPSENSOR_ALL_FAR = 0,
       CAPSENSOR_OLY_WIFI_NEAR = 1,
       CAPSENSOR_OLY_RF_NEAR= 2,
       CAPSENSOR_ALL_NEAR =3,
};


static int SX9311_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int SX9311_i2c_remove(struct i2c_client *client);
static int SX9311_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
//static int SX9311_i2c_suspend(struct i2c_client *client, pm_message_t msg);
//static int SX9311_i2c_resume(struct i2c_client *client);

static int captouch_local_init(void);
static int captouch_remove(void);

#ifdef CONFIG_OF
static const struct of_device_id captouch_of_match[] = {
	{.compatible = "mediatek,capsensor"},
	{},
};
#endif

static struct i2c_driver SX9311_i2c_driver = {
	.probe = SX9311_i2c_probe,
	.remove = SX9311_i2c_remove,
	.detect = SX9311_i2c_detect,
	#if 0
	.suspend = SX9311_i2c_suspend,
	.resume = SX9311_i2c_resume,
	#endif
	.id_table = SX9311_i2c_id,
	.driver = {
		   .name = SX9311_DEV_NAME,
#ifdef CONFIG_OF
			.of_match_table = captouch_of_match,
#endif
		   },
};

/*----------------------------------------------------------------------------*/
static struct captouch_init_info SX9311_init_info = {
		.name = SX9311_DEV_NAME,
		.init = captouch_local_init,
		.uninit = captouch_remove,
	
};

#if defined(SX9311_SUPPORT_I2C_DMA)

static int read_register(struct i2c_client *i2c , u8 address, u8 *value)
{
	s32 returnValue = 0;
	
	if (value && i2c) {

    	returnValue = i2c_smbus_read_byte_data(i2c,address);
	  	CAPTOUCH_LOG("read_register Address: 0x%x Return: 0x%x\n",address,returnValue);
    	if (returnValue >= 0) {
      		*value = returnValue;
      		return 0;
    	} else {
      		return returnValue;
    	}
  	}
	
  	return -ENOMEM;
}


static int write_register(struct i2c_client *i2c, u8 address, u8 value)
{
	char buffer[2];
	int returnValue = 0;
	buffer[0] = address;
	buffer[1] = value;
	returnValue = -ENOMEM;
	
	if (i2c) {
		returnValue = i2c_master_send(i2c,buffer,2);
	  	CAPTOUCH_LOG("write_register Address: 0x%x Value: 0x%x Return: %d\n",
        					address,value,returnValue);
  	}
	
  	return returnValue;
}

static int SX9311_i2c_write_dma(struct i2c_client *client, uint8_t regaddr, uint8_t txbyte, uint8_t *data)
{
	int ret = 0;
	mutex_lock(&SX9311_mutex);

	ret = write_register(client,regaddr,*data);

	mutex_unlock(&SX9311_mutex);

	return ret;
}

static int SX9311_i2c_read_dma(struct i2c_client *client, uint8_t regaddr, uint8_t rxbyte, uint8_t *data) 
{
	int ret = 0;
	mutex_lock(&SX9311_mutex);

	ret = read_register(client ,regaddr, data);

	mutex_unlock(&SX9311_mutex);

	return ret;
}

#else

#if 0
static int SX9311_i2c_write(struct i2c_client *client, uint8_t regaddr, uint8_t txbyte, uint8_t *data)
{
	uint8_t buffer[8];
	int ret = 0;

	mutex_lock(&SX9311_mutex);

	buffer[0] = regaddr;
	buffer[1] = data[0];
	buffer[2] = data[1];
	buffer[3] = data[2];
	buffer[4] = data[3];
	buffer[5] = data[4];
	buffer[6] = data[5];
	buffer[7] = data[6];

	ret = i2c_master_send(client, buffer, txbyte+1);

	if (ret < 0)
	{
		CAPTOUCH_LOG("SX9311 i2c write %x error %d\r\n", regaddr, ret);

		mutex_unlock(&SX9311_mutex);

		return ret;
	}
		
	mutex_unlock(&SX9311_mutex);

	return ret;
}

static int SX9311_i2c_read(struct i2c_client *client, uint8_t regaddr, uint8_t rxbyte, uint8_t *data)
{
	int ret = 0;
	struct i2c_msg msg;

	memset(&msg, 0, sizeof(struct i2c_msg));

	mutex_lock(&SX9311_mutex);

	data[0] = regaddr;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = ((rxbyte & 0x1f)<<8) | 1;
	msg.buf = data;
	msg.ext_flag = I2C_WR_FLAG | I2C_RS_FLAG;

	ret = i2c_transfer(client->adapter, &msg, 1);

	if (ret != 1)
	{
		CAPTOUCH_LOG("SX9311_i2c_read ret=%d\r\n", ret);
	}
	
	mutex_unlock(&SX9311_mutex);

	return ret;
}
#endif
#endif

static irqreturn_t SX9311_eint_func(int irq, void *desc)
{
    CAPTOUCH_LOG("[%s] irq=[%d]",__func__,irq);
 
 	disable_irq_nosync(captouch_irq);
	schedule_work(&captouch_eint_work);

	return IRQ_HANDLED;
}


/*----------------------------------------------------------------------------*/



int SX9311_gpio_config(void)
{
	int ret;
	u32 ints[2] = { 0, 0 };
	u32 ints1[2] = { 0, 0 };	
	struct device_node *node = NULL;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_eint_as_int;
	struct platform_device *captouch_pdev = get_captouch_platformdev();


	CAPTOUCH_LOG("SX9311_gpio_config START \n");

	/* gpio setting */
	pinctrl = devm_pinctrl_get(&captouch_pdev->dev);
	if (IS_ERR(pinctrl))
	{
		ret = PTR_ERR(pinctrl);
		CAPTOUCH_ERR("Cannot find captouch pinctrl!\n");
		return ret;
	}

	pins_default = pinctrl_lookup_state(pinctrl, "default");
	if (IS_ERR(pins_default))
	{
		ret = PTR_ERR(pins_default);
		CAPTOUCH_ERR("Cannot find captouch pinctrl default!\n");
	}

	pins_eint_as_int = pinctrl_lookup_state(pinctrl, "state_eint_as_int");
	if (IS_ERR(pins_eint_as_int))
	{
		ret = PTR_ERR(pins_eint_as_int);
		CAPTOUCH_ERR("Cannot find captouch pinctrl state_eint_as_int!\n");
	}
	pinctrl_select_state(pinctrl, pins_eint_as_int);


	node = of_find_compatible_node(NULL, NULL, "mediatek, capsenso-eint");

	if (node)
	{
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		of_property_read_u32_array(node, "interrupts", ints1, ARRAY_SIZE(ints1));
		captouch_gpiopin = ints[0];
		captouch_debounce = ints[1];
		captouch_eint_type = ints1[1];
		CAPTOUCH_LOG("ints[0] = %d, ints[1] = %d, ints1[1] = %d!!\n", ints[0], ints[1], ints1[1]);
		gpio_set_debounce(captouch_gpiopin, captouch_debounce);

		captouch_irq = irq_of_parse_and_map(node, 0);


		if (!captouch_irq) {

            CAPTOUCH_ERR(" captouch_irq irq_of_parse_and_map fail!!\n");	
			return -EINVAL;
		}

		if (request_irq(captouch_irq, SX9311_eint_func, IRQF_TRIGGER_FALLING, "capsenso-eint", NULL)) 
                {
                    CAPTOUCH_ERR("IRQ LINE NOT AVAILABLE!! ---FALLING \n");	
			return -EINVAL;
		}else{
                    CAPTOUCH_ERR("IRQ LINE NOT AVAILABLE!! ---FALLING  success\n");	

			//enable_irq(captouch_irq); //add enable 
// add by zhaofei - 2016-12-27-20-12
		}

	}
	else
	{
		CAPTOUCH_ERR("%s gpio config can't find compatible node\n", __func__);
	}

	return 0;
}  
/*----------------------------------------------------------------------------*/

static uint8_t  sx9310_interrupt_state(void){
  uint8_t buffer[8]={0};
  unsigned char  valstate = 0, valtype=0;
  int err = 0; 
  uint8_t state = 0;

	err = SX9311_i2c_read_dma(SX9311_i2c_client, SX9310_IRQSTAT_REG, 1, buffer); //SX9310_IRQSTAT_REG  0x00
	
  
        valstate = buffer[0]; 

        
        CAPTOUCH_LOG("[%s] 0x00  buffer[0]=[%x],  valstate=[%x] \r\n",__func__,buffer[0], valstate);

        err = SX9311_i2c_read_dma(SX9311_i2c_client, SX9310_STAT0_REG, 1, buffer); //SX9310_IRQSTAT_REG  0x01
        CAPTOUCH_LOG("[%s] 0x01  buffer[0]=[%x],   \r\n",__func__,buffer[0]);
        

        valtype = buffer[0];  /*0x01  bit2 cs2,  bit1 cs1*/
        CAPTOUCH_LOG("[%s] 0x01  valtype = [%x],   \r\n",__func__,valtype);

         /*
            reg 0x00: eint  0x40 near  0x20 far,only read one time,and set 00 
            reg 0x01: 
                     0x00: channl no data mean FAR
                     0x02/0x42: near
         */              

    valtype &= 0x0f;
    switch(valtype){
    	case 0:			//cs0=cs2=0
    		state = CAPSENSOR_ALL_FAR;
    		break;
    	case 1:			//cs0=1;cs2=0
    		state = CAPSENSOR_OLY_RF_NEAR;
    		break;
    	case 4:			//cs0=0;cs2=1;
    		state = CAPSENSOR_OLY_WIFI_NEAR;
    		break;
    	case 5:			//cs0=cs2=1;
    		state = CAPSENSOR_ALL_NEAR;
    		break;
    	default:
    		CAPTOUCH_ERR("please check if cs1 enabled!\n");
    		return -1;
    }
 
       CAPTOUCH_LOG("[%s]  state=[%x]\n",__func__, state);

        return state;      

}
static void SX9311_eint_work(struct work_struct *work)
{

        uint8_t value = 0;
        //read_regStat();        
        value = sx9310_interrupt_state();
        if(-1 == value)
        {
        	CAPTOUCH_ERR("interrupt state error!\n");
        	return ;
        }

        CAPTOUCH_LOG("[%s]  entry!\n",__func__);        

	mutex_lock(&mtx_eint_status);


	switch_set_state((struct switch_dev *)&capsensor_data, value); 

	mutex_unlock(&mtx_eint_status);
	enable_irq(captouch_irq);
}


/*! \brief  Initialize I2C config from platform data
 * \param this Pointer to main parent struct 
 */
static void hw_init(void)
{

	int i = 0;
        int reg_num = ARRAY_SIZE(sx9310_i2c_reg_setup);
        int err=0;
        uint8_t buffer[8]={0};

        while ( i < reg_num) {
      /* Write all registers/values contained in i2c_reg */
        
	buffer[0] = sx9310_i2c_reg_setup[i].val;
#if defined(SX9311_SUPPORT_I2C_DMA)
	err = SX9311_i2c_write_dma(SX9311_i2c_client, sx9310_i2c_reg_setup[i].reg, 1,buffer);
#else
	err = SX9311_i2c_write(SX9311_i2c_client, 0x0d, 1, buffer);
#endif
//CAPTOUCH_LOG("SX9311 write  hw_init reg=%x, val=%x \n",sx9310_i2c_reg_setup[i].reg,sx9310_i2c_reg_setup[i].val  );
 
       i++;
      }

}

static int read_regStat(void)
{
  uint8_t buffer[8]={0};
  int err = 0; 
	CAPTOUCH_LOG("read_regStat \n");

	#if defined(SX9311_SUPPORT_I2C_DMA)
	 err = SX9311_i2c_read_dma(SX9311_i2c_client, SX9310_IRQSTAT_REG, 1, buffer); //SX9310_IRQSTAT_REG  0x00
	 err = SX9311_i2c_read_dma(SX9311_i2c_client, SX9310_STAT0_REG, 1, buffer); //SX9310_IRQSTAT_REG  0x01
	#else
	err = SX9311_i2c_read(SX9311_i2c_client, 0x42, 1, buffer);
	#endif

  return 0;
}

static int manual_offset_calibration(void)
{
  int returnValue = 0;
  uint8_t buffer[8]={0};
          buffer[0]=0xFF;
  returnValue = SX9311_i2c_write_dma(SX9311_i2c_client, SX9310_IRQSTAT_REG, 1, buffer);
  //returnValue = write_register(this,SX9310_IRQSTAT_REG,0xFF);
  return returnValue;
}


static void  sx9310_reg_config(void){

CAPTOUCH_LOG("start to  sx9310_reg_config   \n");  
    //disable_irq(captouch_irq);
    /* perform a reset */
   // write_register(this,SX9310_SOFTRESET_REG,SX9310_SOFTRESET);
    /* just sleep for awhile instead of using a loop with reading irq status */
    msleep(300);
 
    hw_init();

    msleep(100);

    manual_offset_calibration();

   /* re-enable interrupt handling */
   // enable_irq(captouch_irq);

    read_regStat();
    
}

static ssize_t SX9311_show_enable(struct device_driver *ddri, char *buf)
{
	CAPTOUCH_LOG("SX9311_show_enable \n");

	enable_irq(captouch_irq);

	return sprintf(buf, "%u\n", 2);
}

static DRIVER_ATTR(sx9310_enable, S_IWUSR | S_IRUGO, SX9311_show_enable, NULL);

/*----------------------------------------------------------------------------*/
static ssize_t SX9311_show_disable(struct device_driver *ddri, char *buf)
{
	disable_irq(captouch_irq);

	return sprintf(buf, "%u\n", 3);
}


/*show sx9310 chipid */
static ssize_t SX9311_show_chipid(struct device_driver *ddri, char *buf)
{
    uint8_t buffer[8]={0};
    int err = 0; 

	#if defined(SX9311_SUPPORT_I2C_DMA)
	err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x42, 1, buffer);
	#else
	err = SX9311_i2c_read(SX9311_i2c_client, 0x42, 1, buffer);
	#endif
   
	return sprintf(buf, "%u\n", buffer[0]);
}

//read  sarvalue  for engineer
static ssize_t sx9310_show_sar(struct device_driver *ddri, char *buf)
{
    uint8_t buffer[8]={0};
    int err = 0; 
    ssize_t len;
	#if defined(SX9311_SUPPORT_I2C_DMA)
	err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x35, 1, buffer);
	#else
	err = SX9311_i2c_read(SX9311_i2c_client, 0x42, 1, buffer);
	#endif

        len = sprintf(buf,"0x35= %d," ,buffer[0]);    

        #if defined(SX9311_SUPPORT_I2C_DMA)
	err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x36, 1, buffer);
	#else
	err = SX9311_i2c_read(SX9311_i2c_client, 0x42, 1, buffer);
	#endif

	len += sprintf(buf + len,"0x36= %d \r\n" ,buffer[0]);    

	return len ;
}

static ssize_t sx9311_read_reg(struct device_driver *ddri, char *buf){

	uint8_t buffer[8]={0};
	ssize_t len = 0;
	int i = 0;

	int reg_num = ARRAY_SIZE(sx9310_i2c_reg_setup);
	int err=0;

    while ( i < reg_num){        
		buffer[0] = sx9310_i2c_reg_setup[i].val;

		err = SX9311_i2c_read_dma(SX9311_i2c_client, sx9310_i2c_reg_setup[i].reg, 1,buffer);

		len += sprintf(buf + len,"%x , %x \r\n", sx9310_i2c_reg_setup[i].reg,buffer[0]); 
		
		i++;
    }
    
	err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x30, 1,buffer);

    len += sprintf(buf + len,"0x30, %x \r\n" ,buffer[0]); 

    err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x35, 1,buffer);

    len += sprintf(buf + len,"0x35 , %x \r\n", buffer[0]); 

    err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x36, 1,buffer);

    len += sprintf(buf + len," 0x36, %x\r\n",buffer[0]); 

    err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x37, 1,buffer);

    len += sprintf(buf + len,"0x37, %x \r\n", buffer[0]); 

    err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x38, 1,buffer);

    len += sprintf(buf + len,"0x38, %x \r\n", buffer[0]); 

    err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x00, 1,buffer);

    len += sprintf(buf + len,"0x00, %x \r\n", buffer[0]); 

	err = SX9311_i2c_read_dma(SX9311_i2c_client, 0x01, 1,buffer);
    len += sprintf(buf + len,"0x01, %x \r\n", buffer[0]); 
   
    return len ;
}


static ssize_t sx9311_write_reg(struct device_driver *ddri, const char *buf, size_t count){

	int  value;
	unsigned int reg;
	uint8_t buffer[8]={0};
	int err=0;

	CAPTOUCH_FUN();
	CAPTOUCH_LOG("write value is %s,and write return %d\n",buf,sscanf(buf,"%x %x ",&reg,&value));
	if ( 2==sscanf(buf,"%x %x ",&reg,&value)){
	        CAPTOUCH_LOG("%s  reg=%x  value=%x\n",__func__, reg, value);	
		buffer[0]=value;
		err = SX9311_i2c_write_dma(SX9311_i2c_client, reg, 1, buffer);
	} 

	return count; 
}



static DRIVER_ATTR(sarvalue, S_IWUSR | S_IRUGO, sx9310_show_sar, NULL); //add for sarvalue 
static DRIVER_ATTR(reg, S_IWUSR | S_IRUGO, sx9311_read_reg, sx9311_write_reg);
static DRIVER_ATTR(sx9310_disable, S_IWUSR | S_IRUGO, SX9311_show_disable, NULL);
static DRIVER_ATTR(sx9310_chipid, S_IWUSR | S_IRUGO, SX9311_show_chipid, NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *SX9311_attr_list[] =
{
	&driver_attr_sx9310_enable,
	&driver_attr_sx9310_disable,
        &driver_attr_sx9310_chipid,
        &driver_attr_reg,
        &driver_attr_sarvalue,
};
/*----------------------------------------------------------------------------*/
static int SX9311_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(SX9311_attr_list) / sizeof(SX9311_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, SX9311_attr_list[idx]);
		if (err) {
			CAPTOUCH_ERR("driver_create_file (%s) = %d\n", SX9311_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int SX9311_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(SX9311_attr_list) / sizeof(SX9311_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, SX9311_attr_list[idx]);

	return err;
}

/*----------------------------------------------------------------------------*/
static int SX9311_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct captouch_control_path cap_ctl={0};
	int err = 0;
        int ret = 0;

	CAPTOUCH_FUN();

	SX9311_i2c_client = client;
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		CAPTOUCH_ERR("i2c_check_functionality was failed");
		return -1;
  	}
  	i2c_set_clientdata(client, SX9311_i2c_client);

	INIT_WORK(&captouch_eint_work, SX9311_eint_work);
/*LCT add for switch dev  capsensor*/
	capsensor_data.name = "capsensor";
	capsensor_data.index = 0;
	capsensor_data.state = CAPSENSOR_ALL_FAR;
	
	ret = switch_dev_register(&capsensor_data);
	if (ret) {
		CAPTOUCH_ERR("[capsensor]switch_dev_register returned:%d!\n", ret);
		return 1;
	}
/*END LCT*/

	mutex_init(&mtx_eint_status);


	/*init */
	sx9310_reg_config();

	SX9311_gpio_config();

	CAPTOUCH_LOG("SX9311_init_client   OK!\n");

	read_regStat();

	if((err = SX9311_create_attr(&SX9311_init_info.platform_diver_addr->driver)))
	{
		CAPTOUCH_ERR("SX9311 create attribute err = %d\n", err);
		return err;
	}

	err = captouch_register_control_path(&cap_ctl);
	if(err)
	{
		CAPTOUCH_ERR("captouch register fail = %d\n", err);
		return err;
	}

	CAPTOUCH_LOG("%s: OK\n", __func__);
	return 0;
}

/*----------------------------------------------------------------------------*/


static int SX9311_i2c_remove(struct i2c_client *client)
{
	int err;

	err = SX9311_delete_attr(&SX9311_init_info.platform_diver_addr->driver);
	if (err)
	{
		CAPTOUCH_ERR("SX9311_delete_attr fail: %d\n", err);
	}

	i2c_unregister_device(client);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int SX9311_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, SX9311_DEV_NAME);
	return 0;
}
/*----------------------------------------------------------------------------*/

 void sx9310_calibration(void){
   uint8_t buffer[8]={0};
   int err = 0; 
	//cly add for  0x00=0xff, do re calibrate  20161128
    buffer[0] = 0xff;
    err = SX9311_i2c_write_dma(SX9311_i2c_client, 0x00, 1, buffer);


}
#if 0
/*for 0x41 reg  captouch  1->active, 0->sleep mode cly add 20161117*/
static void sx9310_sleep(void){

   uint8_t buffer[8]={0};
   int err = 0; 

    buffer[0] = 0x0;
    err = SX9311_i2c_write_dma(SX9311_i2c_client, 0x41, 1, buffer);
     
}

static void sx9310_active(void){
   uint8_t buffer[8]={0};
   int err = 0; 

    buffer[0] = 0x1;
    err = SX9311_i2c_write_dma(SX9311_i2c_client, 0x41, 1, buffer);
	//cly add for  0x00=0xff, do re calibrate  20161128
    buffer[0] = 0xff;
    err = SX9311_i2c_write_dma(SX9311_i2c_client, 0x00, 1, buffer);
}
/*end cly*/
#endif
//-- Modified for close Captouch by shentaotao 2016.07.02
/*----------------------------------------------------------------------------*/
#if 0
static int SX9311_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	int err = 0;

	CAPTOUCH_FUN();
        sx9310_sleep();
	return err;
}
/*----------------------------------------------------------------------------*/
static int SX9311_i2c_resume(struct i2c_client *client)
{
	int err = 0;

	CAPTOUCH_FUN();
        sx9310_active();
	return err;
}
/*----------------------------------------------------------------------------*/
#endif
static int captouch_local_init(void) 
{
	CAPTOUCH_FUN();

	if (i2c_add_driver(&SX9311_i2c_driver))
	{
		CAPTOUCH_ERR("add driver error\n");
		return -1;
	}

	CAPTOUCH_ERR("add driver SX9311_i2c_driver ok!\n");
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int captouch_remove(void)
{
	CAPTOUCH_FUN();
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init SX9311_init(void)
{
	CAPTOUCH_FUN();

	CAPTOUCH_LOG("sx9310 init func address is : %p\n",&SX9311_init_info);
	captouch_driver_add(&SX9311_init_info);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit SX9311_exit(void)
{
	CAPTOUCH_FUN();	
}
/*----------------------------------------------------------------------------*/
module_init(SX9311_init);
module_exit(SX9311_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Dexiang Liu");
MODULE_DESCRIPTION("SX9311 driver");
MODULE_LICENSE("GPL");
