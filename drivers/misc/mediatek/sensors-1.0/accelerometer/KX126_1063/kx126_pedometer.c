/* 
 * kx126 sensor driver
 * Copyright (C) 2016 Invensense, Inc.
 *
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

#include <step_counter.h>
#include <cust_acc.h>
#include "kx126_1063.h"
#include <accel.h>

static int kx126_sc_init_flag =  -1;

static int kx126_sc_local_init(void);
static int kx126_sc_remove(void);
static struct step_c_init_info kx126_sc_init_info = {
	.name = "kx126_SC",
	.init = kx126_sc_local_init,
	.uninit = kx126_sc_remove,
};


static int kx126_sc_init_client(bool enable)
{
	int err = 0;
	err = KX126_1063_init_pedometer(1);
	if (0 != err) {
		STEP_C_PR_ERR("KX126_1063_init_pedometer fail\n");
		return 1;
	}
	return 0;
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;

	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{

	return count;
}


static ssize_t show_reg_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
    int i =0;
    u8 databuf[2] = {0};
    int len =0;
    
    len += snprintf(buf + len, PAGE_SIZE, "kx126 Register Dump\n");

    for(i = 0; i < 0x60; i++)
    {
        /* Don't read memory r/w register */
        if(i == 0x4d || i == 0x18 || i==0x33 || ((i >= 0x38)&&(i <= 0x3b)) || i==0x5e)
            databuf[0] = 0;
        else
        {
            res = kx126_i2c_read_step(i, databuf, 1);
            if (res < 0) {
                STEP_C_PR_ERR("read register err!\n");
                return 0;
            }
        }
        len += snprintf(buf + len, PAGE_SIZE, "0x%02x: 0x%02x\n", i, databuf[0]);
    }
    return len;
}

static ssize_t store_reg_value(struct device_driver *ddri, const char *buf, size_t count)
{
    int addr=0,cmd=0;
    int ret =0;
    u8 data  = 0; 
    if( 2 != sscanf(buf,"%x %x",&addr,&cmd)  ) {
        STEP_C_PR_ERR("invalid format: '%s' \n",buf);
        return 0;

    }
    data = (u8)cmd;
    STEP_C_LOG("addr=0x%x cmd=0x%x,data=0x%x\n",addr,cmd,data);
    ret = kx126_i2c_write_step(addr,&data,1);  
    if(ret){
        STEP_C_PR_ERR("i2c error!! ret =%d\n",ret);
        return 0;
    }
    return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value,   store_trace_value);
static DRIVER_ATTR(reg, S_IWUSR | S_IRUGO, show_reg_value,   store_reg_value);


static struct driver_attribute *kx126_sc_attr_list[] = {
	&driver_attr_trace,		/*trace log*/
	&driver_attr_reg,		/*reg*/
};
/*----------------------------------------------------------------------------*/
static int kx126_sc_create_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(kx126_sc_attr_list)/sizeof(kx126_sc_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		res = driver_create_file(driver, kx126_sc_attr_list[idx]);
		if (0 != res) {
			STEP_C_PR_ERR("driver_create_file (%s) = %d\n", kx126_sc_attr_list[idx]->attr.name, res);
			break;
		}
	}
	return res;
}

static int kx126_sc_delete_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(kx126_sc_attr_list)/sizeof(kx126_sc_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, kx126_sc_attr_list[idx]);

	return res;
}

/*=======================================================================================*/
/* Misc - Factory Mode (IOCTL) Device Driver Section					 */
/*=======================================================================================*/

static struct miscdevice kx126_sc_device = {
	.minor 		= MISC_DYNAMIC_MINOR,
	.name 		= "step_counter",
};

/*=======================================================================================*/
/* Misc - I2C HAL Support Section				 			 */
/*=======================================================================================*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int kx126_sc_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */


static int kx126_sc_enable_nodata(int en)
{
	int err = 0;
      u8 buf[2] = {0};	
	  
	printk("kx126_sc_enable_nodata\n");
	err = KX126_1063_SetPowerMode_step(false);
		
	if (err != KX126_1063_SUCCESS) {
		STEP_C_LOG("KX126_1063_enable_nodata fail!\n");
		return -1;
	}

	err = kx126_i2c_read_step(0x1a, buf, 0x01);
	if (err) {
		STEP_C_PR_ERR("KX126_1063_ReadData error: %d\n", err);
		return err;	
	}

	if (en != 0)
	{
		printk("kx126_sc_enable_nodata1\n");
		 buf[1] = buf[0] | 0x02; 
     
    		err = kx126_i2c_write_step(0x1a, &buf[1], 0x01);
		if (err) {
			STEP_C_PR_ERR("KX126_1063_Write Data error: %d %d\n", err,buf[1]);
       	 	return err;	
   		 }
	}
	else
	{
		printk("kx126_sc_enable_nodata2\n");
		 buf[1] = buf[0] & (~0x02); 
     
    		err = kx126_i2c_write_step(0x1a, &buf[1], 0x01);
		if (err) {
			STEP_C_PR_ERR("KX126_1063_write Data error: %d %d\n", err,buf[1]);
       		return err;	
   		 }
	}
    
	err = KX126_1063_SetPowerMode_step(true);
	
	STEP_C_LOG("KX126_1063_enable_nodata OK! buf = [%x] \n",buf[1]);
   	return 0;
}

static int kx126_sc_step_c_set_delay(u64 delay)
{
	return 0;
}

 
static int kx126_sc_step_d_set_delay(u64 delay)// function pointer
{
    return 0;
}
static int kx126_sc_enable_significant(int en)// function pointer
{
    return 0;
}
static int kx126_sc_enable_step_detect(int en)// function pointer
{
    return 0;
}


extern int KX126_1063_suspend_flag;
static int kx126_sc_get_data_step_c(uint32_t *value, int *status)
{
	u8 step_buf[2] = { 0 };
    static unsigned int step_count = 0;
    int err = 0;	
    unsigned int tmp_step = 0;	
    if( (KX126_1063_suspend_flag)){ 
        STEP_C_PR_ERR("kx126_sc_get_data_step_c: gsensor resume not ok\n");
        printk("=====kx126_sc_get_data_step_c kx126 step_count=%d=====\n",step_count);	
	    *value = step_count;
	    *status = SENSOR_STATUS_ACCURACY_MEDIUM;    
        return 0;
    } 
	
    err = kx126_i2c_read_step(PED_STP_L, step_buf, 0x02);
	if (err) {
		STEP_C_PR_ERR("KX126_1063_ReadData error: %d\n", err);
        return err;	
    }
	
	tmp_step = (step_buf[1] << 8) | step_buf[0];
    step_count += tmp_step; 

    printk("=====kx126 step_count=%d=====\n",step_count);	
	*value = step_count;
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	
	return 0;
}

static int kx126_sc_get_data_step_d(uint32_t *value,int *status)
{
    return 0;
}

static int kx126_sc_get_data_significant(uint32_t *value,int *status)
{
    return 0;
}


//+ add by fanxzh
static int kx126_step_c_flush(void)
{
	int err = 0;

	err = step_c_flush_report();
	pr_err("add for modify:flush complete \n");
	return err;
}
//- add by fanxzh

/*=======================================================================================*/
/* HAL Attribute Registration Section							 */
/*=======================================================================================*/

static int kx126_sc_attr_create(void)
{
	struct step_c_control_path ctl = {0};
	struct step_c_data_path data = {0};
	int res = 0;

	STEP_C_LOG();

	res = kx126_sc_init_client(false);
	if (res)
		goto exit_init_failed;

	// misc_register() for factory mode, engineer mode and so on
	res = misc_register(&kx126_sc_device);
	if (res) {
		STEP_C_PR_ERR("kx126_a_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}

	// Crate platform_driver attribute
	res = kx126_sc_create_attr(&(kx126_sc_init_info.platform_diver_addr->driver));
	if (res) {
		STEP_C_PR_ERR("kx126_sc create attribute err = %d\n", res);
		goto exit_create_attr_failed;
	}
	


	/*-----------------------------------------------------------*/
	// Fill and Register the step_c_control_path and step_c_data_path
	/*-----------------------------------------------------------*/

	// Fill the step_c_control_path
	ctl.is_report_input_direct = false;
	ctl.is_counter_support_batch = false;
	ctl.open_report_data = kx126_sc_open_report_data;		// function pointer
	ctl.enable_nodata = kx126_sc_enable_nodata;				// function pointer
	ctl.step_c_set_delay  = kx126_sc_step_c_set_delay;		// function pointer
    
        
    ctl.step_d_set_delay  = kx126_sc_step_d_set_delay;// function pointer
    ctl.enable_significant  = kx126_sc_enable_significant;// function pointer
    ctl.enable_step_detect  = kx126_sc_enable_step_detect;// function pointer

//+ add by fanxzh
	ctl.step_c_flush = kx126_step_c_flush;
//- add by fanxzh

	// Register the step_c_control_path
	res = step_c_register_control_path(&ctl);
	if (res) {
		STEP_C_PR_ERR("register step_c control path err\n");
		goto exit_kfree;
	}

	
	// Fill the step_c_data_path
	data.get_data = kx126_sc_get_data_step_c;			// function pointer
    data.get_data_step_d = kx126_sc_get_data_step_d;
    data.get_data_significant = kx126_sc_get_data_significant;
    data.vender_div = 1000; // Taly : May
	
    // Register the step_c_data_path
	res = step_c_register_data_path(&data);
	if (res) {
		STEP_C_PR_ERR("register step_c data path fail = %d\n", res);
		goto exit_kfree;
	}

	// Set init_flag = 0 and return
	kx126_sc_init_flag = 0;

	STEP_C_LOG("%s: OK\n", __func__);
	return 0;
exit_init_failed:

exit_misc_device_register_failed:
	misc_deregister(&kx126_sc_device);
exit_create_attr_failed:
exit_kfree:
//exit:
	kx126_sc_init_flag =  -1;
	STEP_C_PR_ERR("%s: err = %d\n", __func__, res);
	return res;
}

static int kx126_sc_attr_remove(void)
{
	int res = 0;

	res = kx126_sc_delete_attr(&(kx126_sc_init_info.platform_diver_addr->driver));
	if (res)
		STEP_C_PR_ERR("kx126_sc_delete_attr fail: %d\n", res);
	else
	  	res = 0;

	misc_deregister(&kx126_sc_device);

	return res;
}

/*=======================================================================================*/
/* Kernel Module Section								 */
/*=======================================================================================*/

static int kx126_sc_remove(void)
{
	kx126_sc_attr_remove();

	return 0;
}

extern int KX126_1063_init_flag;
static int kx126_sc_local_init(void)
{
	if (-1 == KX126_1063_init_flag)
                return -1;

	kx126_sc_attr_create();

	if (-1 == kx126_sc_init_flag)		
		return -1;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init kx126_sc_init(void)
{
	STEP_C_FUN();
	step_c_driver_add(&kx126_sc_init_info);

	return 0;
}

static void __exit kx126_sc_exit(void)
{
	STEP_C_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(kx126_sc_init);
module_exit(kx126_sc_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("KIONIX");
MODULE_DESCRIPTION("kx126 step counter driver");
