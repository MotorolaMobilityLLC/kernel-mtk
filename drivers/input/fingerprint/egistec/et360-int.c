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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
*
*  et360.spi.c
*  Date: 2016/03/16
*  Version: 0.9.0.1
*  Revise Date:  2016/03/24
*  Copyright (C) 2007-2016 Egis Technology Inc.
*
*/


#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/list.h>

//#include <mach/gpio.h>
//#include <plat/gpio-cfg.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <mt_spi_hal.h>
#include <mt_spi.h>
#include <mt_spi_hal.h>
#include <mt-plat/mt_gpio.h>
#include <linux/wakelock.h>
#include "et360.h"
#include "navi_input.h"

/* Lct add by zl for devinfo */
#include <mt-plat/lct_print.h>
#ifdef LCT_LOG_MOD
#undef LCT_LOG_MOD
#define LCT_LOG_MOD "[FPS]" 
#endif
/* Lct add end */
//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 begin
#ifdef CONFIG_WIND_DEVICE_INFO
extern char *g_fingerprint_name; 
#endif
//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 end

#define EGIS_NAVI_INPUT 1 // 1:open ; 0:close

struct wake_lock et360_wake_lock;

#define FP_SPI_DEBUG
#define EDGE_TRIGGER_FALLING    0x0
#define	EDGE_TRIGGER_RAISING    0x1
#define	LEVEL_TRIGGER_LOW       0x2
#define	LEVEL_TRIGGER_HIGH      0x3

#ifndef CONFIG_LCT_FPC_TEE_EGIS
/* odm different layout */
#define GPIO_PIN_IRQ  9
#define GPIO_PIN_RESET 125
#define GPIO_PIN_33V 119
#else
#define GPIO_PIN_IRQ  126 
#define GPIO_PIN_RESET 93
#define GPIO_PIN_33V 94
#endif

void mt_spi_enable_clk(struct mt_spi_t *ms);
void mt_spi_disable_clk(struct mt_spi_t *ms);
struct mt_spi_t *egistec_mt_spi_t;

/*
 * FPS interrupt table
 */
 
struct interrupt_desc fps_ints = {0 , 0, "BUT0" , 0};


unsigned int bufsiz = 4096;

int gpio_irq;
int request_irq_done = 0;
int egistec_platformInit_done = 0;

#define EDGE_TRIGGER_FALLING    0x0
#define EDGE_TRIGGER_RISING    0x1
#define LEVEL_TRIGGER_LOW       0x2
#define LEVEL_TRIGGER_HIGH      0x3

int egistec_platformInit(struct egistec_data *egistec);
int egistec_platformFree(struct egistec_data *egistec);

struct ioctl_cmd {
int int_mode;
int detect_period;
int detect_threshold;
}; 


static struct egistec_data *g_data;

DECLARE_BITMAP(minors, N_SPI_MINORS);
LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static struct of_device_id egistec_match_table[] = {
	{ .compatible = "egistec,et360",},
	{},
};

static struct of_device_id fpc1020_spi_of_match[] = {
	{ .compatible = "egistec,et360spi", },
	{}
};


MODULE_DEVICE_TABLE(of, fpc1020_spi_of_match);

// add for dual sensor start
#if 0
static struct of_device_id fpswitch_match_table[] = {
	{ .compatible = "fp_id,fp_id",},
	{},
};
#endif
//add for dual sensor end
MODULE_DEVICE_TABLE(of, egistec_match_table);

/* Lct add by zl for devinfo */
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define LCT_DEVINFO_FPS_DEBUG
#include  "dev_info.h"
struct devinfo_struct *s_DEVINFO_egis;    
static void devinfo_egis_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_egis =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_egis->device_type="FPS";
	s_DEVINFO_egis->device_module=module;
	s_DEVINFO_egis->device_vendor=vendor;
	s_DEVINFO_egis->device_ic="ET320";
	s_DEVINFO_egis->device_info=DEVINFO_NULL;
	s_DEVINFO_egis->device_version=version;
	s_DEVINFO_egis->device_used=used;
#ifdef LCT_DEVINFO_FPS_DEBUG
	lct_pr_err("[DEVINFO]register egis device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_egis->device_type,s_DEVINFO_egis->device_module,s_DEVINFO_egis->device_vendor,
				s_DEVINFO_egis->device_ic,s_DEVINFO_egis->device_version,s_DEVINFO_egis->device_info,s_DEVINFO_egis->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_egis->device_type,s_DEVINFO_egis->device_module,s_DEVINFO_egis->device_vendor,s_DEVINFO_egis->device_ic,s_DEVINFO_egis->device_version,s_DEVINFO_egis->device_info,s_DEVINFO_egis->device_used);
}
#endif
/* Lct add end */





/* ------------------------------ Interrupt -----------------------------*/
/*
 * Interrupt description
 */

#define FP_INT_DETECTION_PERIOD  10
#define FP_DETECTION_THRESHOLD	10

static DECLARE_WAIT_QUEUE_HEAD(interrupt_waitq);
/*
 *	FUNCTION NAME.
 *		interrupt_timer_routine
 *
 *	FUNCTIONAL DESCRIPTION.
 *		basic interrupt timer inital routine
 *
 *	ENTRY PARAMETERS.
 *		gpio - gpio address
 *
 *	EXIT PARAMETERS.
 *		Function Return
 */

void interrupt_timer_routine(unsigned long _data)
{
	struct interrupt_desc *bdata = (struct interrupt_desc *)_data;

	DEBUG_PRINT("FPS interrupt count = %d", bdata->int_count);
	if (bdata->int_count >= bdata->detect_threshold) {
		bdata->finger_on = 1;
		DEBUG_PRINT("FPS triggered !!!!!!!\n");
	} else {
		DEBUG_PRINT("FPS not triggered !!!!!!!\n");
	}

	bdata->int_count = 0;
	wake_up_interruptible(&interrupt_waitq);
}

static irqreturn_t fp_eint_func(int irq, void *dev_id)
{
	if (!fps_ints.int_count)
		mod_timer(&fps_ints.timer,jiffies + msecs_to_jiffies(fps_ints.detect_period));
	fps_ints.int_count++;
//	DEBUG_PRINT("-----------   zq fp fp_eint_func  , fps_ints.int_count=%d",fps_ints.int_count);
  	wake_lock_timeout(&et360_wake_lock, msecs_to_jiffies(900));
	return IRQ_HANDLED;
}

static irqreturn_t fp_eint_func_ll(int irq , void *dev_id)
{
	DEBUG_PRINT("[egis]fp_eint_func_ll\n");
	fps_ints.finger_on = 1;
	//fps_ints.int_count = 0;
	disable_irq_nosync(gpio_irq);
	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	wake_up_interruptible(&interrupt_waitq);
	wake_lock_timeout(&et360_wake_lock, msecs_to_jiffies(900));
	return IRQ_RETVAL(IRQ_HANDLED);
}



/*
 *	FUNCTION NAME.
 *		Interrupt_Init
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

int Interrupt_Init(struct egistec_data *egistec,int int_mode,int detect_period,int detect_threshold)
{

	int err = 0;
	int status = 0;
    struct device_node *node = NULL;
	
DEBUG_PRINT("FP --  %s mode = %d period = %d threshold = %d\n",__func__,int_mode,detect_period,detect_threshold);
DEBUG_PRINT("FP --  %s request_irq_done = %d gpio_irq = %d  pin = %d  \n",__func__,request_irq_done,gpio_irq, egistec->irqPin);


	fps_ints.detect_period = detect_period;
	fps_ints.detect_threshold = detect_threshold;
	fps_ints.int_count = 0;
	fps_ints.finger_on = 0;


	if (request_irq_done == 0)
	{
//		gpio_irq = gpio_to_irq(egistec->irqPin);
        node = of_find_matching_node(node, egistec_match_table);
//        printk("ttt-fp_irq number %d\n", node? 1:2);
 
        if (node){
                gpio_irq = irq_of_parse_and_map(node, 0);
                printk("fp_irq number %d\n", gpio_irq);
		}else
		printk("node = of_find_matching_node fail error  \n");
	
		if (gpio_irq < 0) {
			DEBUG_PRINT("%s gpio_to_irq failed\n", __func__);
			status = gpio_irq;
			goto done;
		}


		DEBUG_PRINT("[Interrupt_Init] flag current: %d disable: %d enable: %d\n",
		fps_ints.drdy_irq_flag, DRDY_IRQ_DISABLE, DRDY_IRQ_ENABLE);
			
		if (int_mode == EDGE_TRIGGER_RISING){
		DEBUG_PRINT("%s EDGE_TRIGGER_RISING\n", __func__);
		err = request_irq(gpio_irq, fp_eint_func,IRQ_TYPE_EDGE_RISING,"fp_detect-eint", egistec);
			if (err){
				pr_err("request_irq failed==========%s,%d\n", __func__,__LINE__);
				}				
		}
		else if (int_mode == EDGE_TRIGGER_FALLING){
			DEBUG_PRINT("%s EDGE_TRIGGER_FALLING\n", __func__);
			err = request_irq(gpio_irq, fp_eint_func,IRQ_TYPE_EDGE_FALLING,"fp_detect-eint", egistec);
			if (err){
				pr_err("request_irq failed==========%s,%d\n", __func__,__LINE__);
				}	
		}
		else if (int_mode == LEVEL_TRIGGER_LOW) {
			DEBUG_PRINT("%s LEVEL_TRIGGER_LOW\n", __func__);
			err = request_irq(gpio_irq, fp_eint_func_ll,IRQ_TYPE_LEVEL_LOW,"fp_detect-eint", egistec);
			if (err){
				pr_err("request_irq failed==========%s,%d\n", __func__,__LINE__);
				}
		}
		else if (int_mode == LEVEL_TRIGGER_HIGH){
			DEBUG_PRINT("%s LEVEL_TRIGGER_HIGH\n", __func__);
			err = request_irq(gpio_irq, fp_eint_func_ll,IRQ_TYPE_LEVEL_HIGH,"fp_detect-eint", egistec);
			if (err){
				pr_err("request_irq failed==========%s,%d\n", __func__,__LINE__);
				}
		}
		DEBUG_PRINT("[Interrupt_Init]:gpio_to_irq return: %d\n", gpio_irq);
		DEBUG_PRINT("[Interrupt_Init]:request_irq return: %d\n", err);
	
		fps_ints.drdy_irq_flag = DRDY_IRQ_ENABLE;
		enable_irq_wake(gpio_irq);
		request_irq_done = 1;
	}
		
		
	if (fps_ints.drdy_irq_flag == DRDY_IRQ_DISABLE){
		fps_ints.drdy_irq_flag = DRDY_IRQ_ENABLE;
		enable_irq_wake(gpio_irq);
		enable_irq(gpio_irq);
	}
done:
	return 0;
}

/*
 *	FUNCTION NAME.
 *		Interrupt_Free
 *
 *	FUNCTIONAL DESCRIPTION.
 *		free all interrupt resource
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

int Interrupt_Free(struct egistec_data *egistec)
{
	DEBUG_PRINT("%s\n", __func__);
	fps_ints.finger_on = 0;
	
	if (fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE) {
		DEBUG_PRINT("%s (DISABLE IRQ)\n", __func__);
		disable_irq_nosync(gpio_irq);

		del_timer_sync(&fps_ints.timer);
		fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	}
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

unsigned int fps_interrupt_poll(
struct file *file,
struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &interrupt_waitq, wait);
	if (fps_ints.finger_on) {
		mask |= POLLIN | POLLRDNORM;
	}
	return mask;
}

void fps_interrupt_abort(void)
{
	DEBUG_PRINT("%s\n", __func__);
	fps_ints.finger_on = 0;
	wake_up_interruptible(&interrupt_waitq);
}

/*-------------------------------------------------------------------------*/

static void egistec_reset(struct egistec_data *egistec)
{
	DEBUG_PRINT("%s\n", __func__);
	gpio_set_value(GPIO_PIN_RESET, 0);
	msleep(30);
	gpio_set_value(GPIO_PIN_RESET, 1);
	msleep(20);
}

static ssize_t egistec_read(struct file *filp,
	char __user *buf,
	size_t count,
	loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}

static ssize_t egistec_write(struct file *filp,
	const char __user *buf,
	size_t count,
	loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}

static long egistec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int retval = 0;
	struct egistec_data *egistec;
	struct ioctl_cmd data;
	int status = 0;

	memset(&data, 0, sizeof(data));
	printk("%s  ---------   zq  001  ---  cmd = 0x%X \n", __func__, cmd);	
	egistec = filp->private_data;


	if(!egistec_platformInit_done)
	/* platform init */
	status = egistec_platformInit(egistec);
	if (status != 0) {
		pr_err("%s platforminit failed\n", __func__);
		//goto egistec_probe_platformInit_failed;
	}



	switch (cmd) {
	case INT_TRIGGER_INIT:

		if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
			retval = -EFAULT;
		goto done;
		}

		//DEBUG_PRINT("fp_ioctl IOCTL_cmd.int_mode %u,
		//		IOCTL_cmd.detect_period %u,
		//		IOCTL_cmd.detect_period %u (%s)\n",
		//		data.int_mode,
		//		data.detect_period,
		//		data.detect_threshold, __func__);

		DEBUG_PRINT("fp_ioctl >>> fp Trigger function init\n");
		retval = Interrupt_Init(egistec, data.int_mode,data.detect_period,data.detect_threshold);
		DEBUG_PRINT("fp_ioctl trigger init = %x\n", retval);
	break;

	case FP_SENSOR_RESET:
			//gpio_request later
			DEBUG_PRINT("fp_ioctl ioc->opcode == FP_SENSOR_RESET --");
			egistec_reset(egistec);
		goto done;
	case INT_TRIGGER_CLOSE:
			DEBUG_PRINT("fp_ioctl <<< fp Trigger function close\n");
			retval = Interrupt_Free(egistec);
			DEBUG_PRINT("fp_ioctl trigger close = %x\n", retval);
		goto done;
	case INT_TRIGGER_ABORT:
			DEBUG_PRINT("fp_ioctl <<< fp Trigger function close\n");
			fps_interrupt_abort();
		goto done;
	case FP_FREE_GPIO:
			DEBUG_PRINT("fp_ioctl <<< FP_FREE_GPIO -------  \n");
			egistec_platformFree(egistec);
		goto done;
	case FP_SPICLK_ENABLE:
			DEBUG_PRINT("fp_ioctl <<< FP_SPICLK_ENABLE -------  \n");
			mt_spi_enable_clk(egistec_mt_spi_t);
		goto done;		
	case FP_SPICLK_DISABLE:
			DEBUG_PRINT("fp_ioctl <<< FP_SPICLK_DISABLE -------  \n");
			mt_spi_disable_clk(egistec_mt_spi_t);
		goto done;		

	default:
	retval = -ENOTTY;
	break;
	}
	
	
	
done:

	DEBUG_PRINT("%s ----------- zq done  \n", __func__);
	return (retval);
}

#ifdef CONFIG_COMPAT
static long egistec_compat_ioctl(struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	return egistec_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define egistec_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int egistec_open(struct inode *inode, struct file *filp)
{
	struct egistec_data *egistec;
	int			status = -ENXIO;

	DEBUG_PRINT("%s\n", __func__);
	printk("%s  ---------   zq    \n", __func__);
	
	mutex_lock(&device_list_lock);

	list_for_each_entry(egistec, &device_list, device_entry)
	{
		if (egistec->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (egistec->buffer == NULL) {
			egistec->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (egistec->buffer == NULL) {
//				dev_dbg(&egistec->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			egistec->users++;
			filp->private_data = egistec;
			nonseekable_open(inode, filp);
		}
	} else {
		pr_debug("%s nothing for minor %d\n"
			, __func__, iminor(inode));
	}
	mutex_unlock(&device_list_lock);
	return status;
}

static int egistec_release(struct inode *inode, struct file *filp)
{
	struct egistec_data *egistec;

	DEBUG_PRINT("%s\n", __func__);

	mutex_lock(&device_list_lock);
	egistec = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	egistec->users--;
	if (egistec->users == 0) {
		int	dofree;

		kfree(egistec->buffer);
		egistec->buffer = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&egistec->spi_lock);
		dofree = (egistec->pd == NULL);
		spin_unlock_irq(&egistec->spi_lock);

		if (dofree)
			kfree(egistec);		
	}
	mutex_unlock(&device_list_lock);
	return 0;

}

int egistec_platformFree(struct egistec_data *egistec)
{
	int status = 0;
	DEBUG_PRINT("%s\n", __func__);
	if (egistec_platformInit_done != 1)
	return status;
	if (egistec != NULL) {
		if (request_irq_done==1)
		{
		free_irq(gpio_irq, NULL);
		request_irq_done = 0;
		}
	gpio_free(egistec->irqPin);
	gpio_free(GPIO_PIN_RESET);
	}

	egistec_platformInit_done = 0;

	DEBUG_PRINT("%s successful status=%d\n", __func__, status);
	return status;
}


int egistec_platformInit(struct egistec_data *egistec)
{
	int status = 0;
	DEBUG_PRINT("%s\n", __func__);

	if (egistec != NULL) {

		/* Initial Reset Pin
		status = gpio_request(egistec->rstPin, "reset-gpio");
		if (status < 0) {
			pr_err("%s gpio_requset egistec_Reset failed\n",
				__func__);
			goto egistec_platformInit_rst_failed;
		}
		*/
		gpio_direction_output(egistec->rstPin, 1);
		if (status < 0) {
			pr_err("%s gpio_direction_output Reset failed\n",
					__func__);
			status = -EBUSY;
//			goto egistec_platformInit_rst_failed;
		}

#ifndef CONFIG_LCT_FPC_TEE_EGIS
		mt_set_gpio_mode(65,1);
		mt_set_gpio_mode(66,1);
		mt_set_gpio_mode(67,1);
		mt_set_gpio_mode(68,1);
		
		
		
		mt_set_gpio_mode(GPIO_PIN_RESET,0);
		
		gpio_set_value(GPIO_PIN_RESET, 1);           // modify by irick
		msleep(30);                                    // modify by irick
        
        gpio_direction_output(egistec->vcc_33v_Pin, 1);    // modify by irick
		gpio_set_value(egistec->vcc_33v_Pin, 1);      // modify by irick

		msleep(2);
#endif 
		
		gpio_set_value(GPIO_PIN_RESET, 0);
		msleep(30);
		gpio_set_value(GPIO_PIN_RESET, 1);
		msleep(20);

		/* initial 33V power pin */
#ifdef  CONFIG_LCT_FPC_TEE_EGIS
		gpio_direction_output(egistec->vcc_33v_Pin, 1);
		gpio_set_value(egistec->vcc_33v_Pin, 1);
#endif        

/*		status = gpio_request(egistec->vcc_33v_Pin, "33v-gpio");
		if (status < 0) {
			pr_err("%s gpio_requset egistec_Reset failed\n",
				__func__);
			goto egistec_platformInit_rst_failed;
		}
		gpio_direction_output(egistec->vcc_33v_Pin, 1);
		if (status < 0) {
			pr_err("%s gpio_direction_output Reset failed\n",
					__func__);
			status = -EBUSY;
			goto egistec_platformInit_rst_failed;
		}

		gpio_set_value(egistec->vcc_33v_Pin, 1);
*/


		/* Initial IRQ Pin
		status = gpio_request(egistec->irqPin, "irq-gpio");
		if (status < 0) {
			pr_err("%s gpio_request egistec_irq failed\n",
				__func__);
			goto egistec_platformInit_irq_failed;
		}
		*/
/*		
		status = gpio_direction_input(egistec->irqPin);
		if (status < 0) {
			pr_err("%s gpio_direction_input IRQ failed\n",
				__func__);
//			goto egistec_platformInit_gpio_init_failed;
		}
*/

	}

	egistec_platformInit_done = 1;

	DEBUG_PRINT("%s successful status=%d\n", __func__, status);
	return status;
/*
egistec_platformInit_gpio_init_failed:
	gpio_free(egistec->irqPin);
//	gpio_free(egistec->vcc_33v_Pin);
egistec_platformInit_irq_failed:
	gpio_free(egistec->rstPin);
egistec_platformInit_rst_failed:
*/
	pr_err("%s is failed\n", __func__);
	return status;
}



static int egistec_parse_dt(struct device *dev,
	struct egistec_data *data)
{
//	struct device_node *np = dev->of_node;
	int errorno = 0;
//	int gpio;
	
	data->rstPin = GPIO_PIN_RESET;
	data->irqPin = GPIO_PIN_IRQ;
	data->vcc_33v_Pin = GPIO_PIN_33V;
#if 0
	gpio = of_get_named_gpio(np, "egis,reset-gpio", 0);
	if (gpio < 0) {
		errorno = gpio;
		goto dt_exit;
	} else {
		data->rstPin = gpio;
		DEBUG_PRINT("%s: sleepPin=%d\n",
			__func__, data->rstPin);
	}
	
	gpio = of_get_named_gpio(np, "egis,irq-gpio", 0);
	if (gpio < 0) {
		errorno = gpio;
		goto dt_exit;
	} else {
		data->irqPin = gpio;
		DEBUG_PRINT("%s: drdyPin=%d\n",
			__func__, data->irqPin);
	}
#endif	
/*
	gpio = of_get_named_gpio(np, "egis,33v-gpio", 0);
	if (gpio < 0) {
		errorno = gpio;
		goto dt_exit;
	} else {
		data->vcc_33v_Pin = gpio;
		pr_info("%s: 33v power pin=%d\n",
			__func__, data->vcc_33v_Pin);
	}
*/
	DEBUG_PRINT("%s is successful\n", __func__);
	return errorno;
//dt_exit:
	pr_err("%s is failed\n", __func__);
	return errorno;
}

static const struct file_operations egistec_fops = {
	.owner = THIS_MODULE,
	.write = egistec_write,
	.read = egistec_read,
	.unlocked_ioctl = egistec_ioctl,
	.compat_ioctl = egistec_compat_ioctl,
	.open = egistec_open,
	.release = egistec_release,
	.llseek = no_llseek,
	.poll = fps_interrupt_poll
};

/*-------------------------------------------------------------------------*/

static struct class *egistec_class;

/*-------------------------------------------------------------------------*/

static int egistec_probe(struct platform_device *pdev);
static int egistec_remove(struct platform_device *pdev);



/*
typedef struct {
	struct spi_device      *spi;
	struct class           *class;
	struct device          *device;
//	struct cdev            cdev;
	dev_t                  devno;
	u8                     *huge_buffer;
	size_t                 huge_buffer_size;
	struct input_dev       *input_dev;
} fpc1020_data_t;

*/

/* -------------------------------------------------------------------- */
static int fpc1020_spi_probe(struct spi_device *spi)
{
//	struct device *dev = &spi->dev;
	int error = 0;
//	fpc1020_data_t *fpc1020 = NULL;
	/* size_t buffer_size; */

	printk(KERN_ERR "fpc1020_spi_probe enter++++++\n");
#if 0
	fpc1020 = kzalloc(sizeof(*fpc1020), GFP_KERNEL);
	if (!fpc1020) {
		/*
		dev_err(&spi->dev,
		"failed to allocate memory for struct fpc1020_data\n");
		*/
		return -ENOMEM;
	}
	printk(KERN_INFO"%s\n", __func__);

	spi_set_drvdata(spi, fpc1020);
#endif	
//	g_data->spi = spi;

    egistec_mt_spi_t=spi_master_get_devdata(spi->master);

	return error;
}

/* -------------------------------------------------------------------- */
static int fpc1020_spi_remove(struct spi_device *spi)
{
//	fpc1020_data_t *fpc1020 = spi_get_drvdata(spi);

//	pr_debug("%s\n", __func__);

//	fpc1020_manage_sysfs(fpc1020, spi, false);

	//fpc1020_sleep(fpc1020, true);

	//cdev_del(&fpc1020->cdev);

	//unregister_chrdev_region(fpc1020->devno, 1);

	//fpc1020_cleanup(fpc1020, spi);

	return 0;
}


static struct spi_driver spi_driver = {
	.driver = {
		.name	= "fpc1020",
		.owner	= THIS_MODULE,
		.of_match_table = fpc1020_spi_of_match,
        .bus	= &spi_bus_type,
	},
	.probe	= fpc1020_spi_probe,
	.remove	= fpc1020_spi_remove
};

struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias="fpc1020",
        .bus_num = 0,
        .chip_select=1,
        .mode = SPI_MODE_0,
    },
};


static struct platform_driver egistec_driver = {
	.driver = {
		.name		= "et360",
		.owner		= THIS_MODULE,
		.of_match_table = egistec_match_table,
	},
    .probe =    egistec_probe,
    .remove =   egistec_remove,
};


static int egistec_remove(struct platform_device *pdev)
{
	#if EGIS_NAVI_INPUT
	struct device *dev = &pdev->dev;
	struct egistec_data *egistec = dev_get_drvdata(dev);
	#endif
	
    DEBUG_PRINT("%s(#%d)\n", __func__, __LINE__);
	free_irq(gpio_irq, NULL);
	
	#if EGIS_NAVI_INPUT
	uinput_egis_destroy(egistec);
	sysfs_egis_destroy(egistec);
	#endif
	
	del_timer_sync(&fps_ints.timer);
	wake_lock_destroy(&et360_wake_lock);
	request_irq_done = 0;
    return 0;
}


static int egistec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct egistec_data *egistec;
	int status = 0;
	unsigned long minor;

	DEBUG_PRINT("%s initial\n", __func__);
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(ET360_MAJOR, "et360", &egistec_fops);
	if (status < 0) {
			pr_err("%s register_chrdev error.\n", __func__);
			return status;
	}

	egistec_class = class_create(THIS_MODULE, "et360");
	if (IS_ERR(egistec_class)) {
		pr_err("%s class_create error.\n", __func__);
		unregister_chrdev(ET360_MAJOR, egistec_driver.driver.name);
		return PTR_ERR(egistec_class);
	}

	/* Allocate driver data */
	egistec = kzalloc(sizeof(*egistec), GFP_KERNEL);
	if (egistec== NULL) {
		pr_err("%s - Failed to kzalloc\n", __func__);
		return -ENOMEM;
	}

	/* device tree call */
	if (pdev->dev.of_node) {
		status = egistec_parse_dt(&pdev->dev, egistec);
		if (status) {
			pr_err("%s - Failed to parse DT\n", __func__);
			goto egistec_probe_parse_dt_failed;
		}
	}

	
	egistec->rstPin = GPIO_PIN_RESET;
	egistec->irqPin = GPIO_PIN_IRQ;
	egistec->vcc_33v_Pin = GPIO_PIN_33V;
	
	
	
	/* Initialize the driver data */
	egistec->pd = pdev;
	g_data = egistec;

	spin_lock_init(&egistec->spi_lock);
	mutex_init(&egistec->buf_lock);
	mutex_init(&device_list_lock);

	INIT_LIST_HEAD(&egistec->device_entry);
#if 1	
	/* platform init */
	status = egistec_platformInit(egistec);
	if (status != 0) {
		pr_err("%s platforminit failed\n", __func__);
		goto egistec_probe_platformInit_failed;
	}
#endif
	
#if 0 //gpio_request later
	/* platform init */
	status = egistec_platformInit(egistec);
	if (status != 0) {
		pr_err("%s platforminit failed\n", __func__);
		goto egistec_probe_platformInit_failed;
	}
#endif

	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;

	/*
	 * If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *fdev;
		egistec->devt = MKDEV(ET360_MAJOR, minor);
		fdev = device_create(egistec_class, &pdev->dev, egistec->devt,
					egistec, "esfp0");
		status = IS_ERR(fdev) ? PTR_ERR(fdev) : 0;
	} else {
		dev_dbg(&pdev->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&egistec->device_entry, &device_list);
	}

	mutex_unlock(&device_list_lock);

	if (status == 0){
		dev_set_drvdata(dev, egistec);
	}
	else {
		goto egistec_probe_failed;
	}

	////gpio_request later
	//egistec_reset(egistec);

	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;

	/* the timer is for ET310 */
	setup_timer(&fps_ints.timer, interrupt_timer_routine,(unsigned long)&fps_ints);
	add_timer(&fps_ints.timer);
	wake_lock_init(&et360_wake_lock, WAKE_LOCK_SUSPEND, "et360_wake_lock");

	
	#if EGIS_NAVI_INPUT
	/*
	 * William Add.
	 */
	sysfs_egis_init(egistec);
	uinput_egis_init(egistec);
	#endif

	/* Lct add by zl for devinfo */
	#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	devinfo_egis_regchar("ET320","EGIS","EGIS",DEVINFO_USED);
	#endif
	/* Lct add end*/

	DEBUG_PRINT("%s : initialize success %d\n", __func__, status);	

	request_irq_done = 0;
	return status;

egistec_probe_failed:
	device_destroy(egistec_class, egistec->devt);
	class_destroy(egistec_class);

egistec_probe_platformInit_failed:
egistec_probe_parse_dt_failed:
	kfree(egistec);
	pr_err("%s is failed\n", __func__);
	return status;
}

static int __init egis360_init(void)
{
    int status = 0;
//add for dual sensor start
#if 0
    struct device_node *fp_id_np = NULL;
    int fp_id_gpio, fp_id_gpio_value;

    fp_id_np = of_find_matching_node(fp_id_np, fpswitch_match_table);
    if (fp_id_np)
	fp_id_gpio = of_get_named_gpio(fp_id_np, "egis,id-gpio", 0);
    if (fp_id_gpio < 0) {
	return status;
    } else {
	pr_info("%s: fp_id pin=%d\n",__func__, fp_id_gpio);
    }

    gpio_direction_input(fp_id_gpio);
    fp_id_gpio_value = gpio_get_value(fp_id_gpio);
    pr_info("%s, Get FP_ID from GPIO_PIN is / FP_ID = %d.\n", __func__,fp_id_gpio_value);

    if(fp_id_gpio_value==0){
        //register driver
        pr_info("%s, Need to register egistec FP driver\n", __func__);
    }else{
        // do not register driver, goto exit
        pr_info("%s, Don't need to register egistec FP driver\n", __func__);
        return status;
    }
#endif
//add for dual sensor end


	int rc = 0;
	printk(KERN_INFO "%s\n", __func__);
	

     status = platform_driver_register(&egistec_driver);	
	
	printk(KERN_INFO "%s   01  \n", __func__);
	printk(KERN_ERR "%s   01  \n", __func__);	
	
/*	fpc_irq_platform_device = platform_device_register_simple(
							FPC_IRQ_DEV_NAME,
							0,
							NULL,
							0);

	if (IS_ERR(fpc_irq_platform_device))
		return PTR_ERR(fpc_irq_platform_device); */
	rc = spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
	
	printk(KERN_ERR "%s  02  \n", __func__);

	if (rc)
	{
		printk(KERN_ERR "spi register board info%s\n", __func__);		
	        return -EINVAL;
	}

	if (spi_register_driver(&spi_driver))
	{
		printk(KERN_ERR "register spi driver fail%s\n", __func__);
		return -EINVAL;
	}

	//mt_spi_enable_clk(egistec_mt_spi_t);//temp
	printk(KERN_ERR "spi enabled----\n");

	//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 begin
	#ifdef CONFIG_WIND_DEVICE_INFO	
	/*Get Chip name*/
	g_fingerprint_name = "et360";	 	
	#endif
	//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 end
	
     return status;
}

static void __exit egis360_exit(void)
{

      platform_driver_unregister(&egistec_driver);
      spi_unregister_driver(&spi_driver);
}

module_init(egis360_init);
module_exit(egis360_exit);

MODULE_AUTHOR("Wang YuWei, <robert.wang@egistec.com>");
MODULE_DESCRIPTION("SPI Interface for ET360");
MODULE_LICENSE("GPL");

