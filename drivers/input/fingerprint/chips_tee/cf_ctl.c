/**
 * ChipSailing Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the ChipSailing fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks, controlling GPIOs such as SPI chip select, sensor reset line, sensor
 * IRQ line, MISO and MOSI lines.
 *
 * The driver will expose most of its available functionality in sysfs which
 * enables dynamic control of these features from eg. a user space process.
 *
 
 
 
 * The sensor's IRQ events will be pushed to Kernel's event handling system and
 * are exposed in the drivers event node. This makes it possible for a user
 * space process to poll the input node and receive IRQ events easily. Usually
 * this node is available under /dev/input/eventX where 'X' is a number given by
 * the event system. A user space process will need to traverse all the event
 * nodes and ask for its parent's name (through EVIOCGNAME) which should match
 * the value in device tree named input-device-name.
 *
 * This driver will NOT send any SPI commands to the sensor it only controls the
 * electrical parts.
 *
 *
 * Copyright (C) 2016 chipsailing Corporation. <http://www.chipsailing.com>
 * Copyright (C) 2016 XXX <mailto:xxx@chipsailing.com>
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General 
 * Public License for more details.
 **/

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
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/input.h>
#include <linux/wakelock.h>

#include "cf_ctl.h"

/*rsee*/
#include <linux/tee_drv.h>

#include "../../../spi/mediatek/mt6763/mtk_spi.h"

#define MODULE_NAME "cf_ctl"

typedef enum {
	CF_PIN_STATE_RST_HIGH,
	CF_PIN_STATE_RST_LOW,
	CF_PIN_STATE_INT,
	CF_PIN_STATE_MI,
	CF_PIN_STATE_MO,
	CF_PIN_STATE_CLK,
	CF_PIN_STATE_CS,
	CF_PIN_STATE_FP_DEFAULT,
	CF_PIN_STATE_MAX
} cf_pin_state_t;

static const char * const pctl_names[] = {
	
	"cs_finger_reset_en1",
	"cs_finger_reset_en0",
	"cs_finger_int_as_int",
	"cs_finger_spi0_mi_as_spi0_mi",
	"cs_finger_spi0_mo_as_spi0_mo",
	"cs_finger_spi0_clk_as_spi0_clk",
	"cs_finger_spi0_cs_as_spi0_cs",
	"cs_finger_pins_default"
};

/**
 * Define the driver version string.
 * There is NO need to modify 'rXXXX_yyyymmdd', it should be updated automatically
 * by the building script (see the 'Driver-revision' section in 'build.sh').
 */
#define CF_DRV_VERSION "v1.0 2018.2.2"

struct cf_device {
	struct spi_device *spi;
	struct cdev     cdev;
	struct class*    class;
	struct device*   device;
	dev_t             devno;
	struct pinctrl *fingerprint_pinctrl;
	struct pinctrl_state *pinctrl_state[CF_PIN_STATE_MAX];

	struct input_dev *input;
	struct work_struct work_queue;

	struct mt_chip_conf spi_mcc;
	struct mt_spi_t *mt_spi;
	struct platform_device* pf_dev;

	struct wake_lock ttw_wl;
	int irq;
	int irq_gpio;
	struct mutex lock;
	atomic_t wakeup_enabled;
	int irq_enabled;
};

/**************************debug******************************/
#define ERR_LOG  (0)
#define INFO_LOG (1)
#define DEBUG_LOG (2)

/* debug log setting */
u8 cf_debug_level = DEBUG_LOG;

#define cf_debug(level, fmt, args...) do { \
	if (cf_debug_level >= level) {\
		printk("[chipsailing]%s line:%d  "fmt, __func__, __LINE__, ##args);\
	} \
} while (0)

#define FUNC_ENTRY()  cf_debug(DEBUG_LOG, "entry\n")
#define FUNC_EXIT()  cf_debug(DEBUG_LOG, "exit\n")
//+add by hzb for ontim debug
#include <ontim/ontim_dev_dgb.h>
static char version[]="chipsailing ver 1.0";
static char vendor_name[50]="chipsailing";
DEV_ATTR_DECLARE(fpsensor)
DEV_ATTR_DEFINE("version",version)
DEV_ATTR_DEFINE("vendor",vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(fpsensor,fpsensor,8);
//-add by hzb for ontim debug
static int hw_reset(struct cf_device *cf_dev)
{

	pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[CF_PIN_STATE_RST_HIGH]);

	udelay(100);

	pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[CF_PIN_STATE_RST_LOW]);

	mdelay(5);

	pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[CF_PIN_STATE_RST_HIGH]);

	return 0;
}


/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device *device,
		struct device_attribute *attribute,
		char *buffer)
{
	struct cf_device *cf_dev = dev_get_drvdata(device);
	int irq = gpio_get_value(cf_dev->irq_gpio);
	cf_debug(INFO_LOG, "irq_get %d\n",irq);
	return scnprintf(buffer, PAGE_SIZE, "%i\n", irq);
}


/**
 * writing to the irq node will just drop a printk message
 * and return success, used for latency measurement.
 */
static ssize_t irq_ack(struct device *device,
		struct device_attribute *attribute,
		const char *buffer, size_t count)
{
	cf_debug(INFO_LOG, "irq_ack\n");
	return count;
}
static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |S_IROTH, irq_get, irq_ack);

static struct attribute *attributes[] = {
	&dev_attr_irq.attr,
	NULL
};

static const struct attribute_group attribute_group = {
	.attrs = attributes,
};

static void cf_disable_irq(struct cf_device *cf_dev)
{
    FUNC_ENTRY();
    if (cf_dev->irq_enabled) 
    {
        disable_irq_nosync(cf_dev->irq);
        cf_dev->irq_enabled = 0;
    }
    else
    {
		cf_debug(INFO_LOG, "irq already disabled\n");
    }
}


static void cf_enable_irq(struct cf_device *cf_dev)
{
    FUNC_ENTRY();
    if (!cf_dev->irq_enabled) 
    {
        enable_irq(cf_dev->irq);
        cf_dev->irq_enabled = 1;
    }
    else
    {
		cf_debug(INFO_LOG, "irq already enabled\n");
    }
}

static void cf_spi_clk_enable(struct cf_device *cf_dev, int bonoff)
{
	if (bonoff == 1) {
		mt_spi_enable_master_clk(cf_dev->spi);
	} else if (bonoff == 0) {
		mt_spi_disable_master_clk(cf_dev->spi);
	}
}

static void cf_device_event(struct work_struct *ws)
{
	struct cf_device *cf_dev = container_of(ws, struct cf_device, work_queue);

	sysfs_notify(&cf_dev->pf_dev->dev.kobj, NULL, dev_attr_irq.attr.name);
}

static irqreturn_t cf_irq_handler(int irq, void * dev_id)
{
	struct cf_device *cf_dev =(struct cf_device *)dev_id;
	FUNC_ENTRY();

	/* Make sure 'wakeup_enabled' is updated before using it
	 ** since this is interrupt context (other thread...) */
	smp_rmb();

	if (atomic_read(&cf_dev->wakeup_enabled)) {
		wake_lock_timeout(&cf_dev->ttw_wl, msecs_to_jiffies(1000));
	}
	schedule_work(&cf_dev->work_queue);

	return IRQ_HANDLED;
}

static int cf_gpio_init(struct cf_device *cf_dev)
{
	int i;
	int ret;
	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,cs_finger");
	if (node) {
		pdev = of_find_device_by_node(node);
		if (pdev) {
			cf_dev->fingerprint_pinctrl = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(cf_dev->fingerprint_pinctrl)) {
				ret = PTR_ERR(cf_dev->fingerprint_pinctrl);
				cf_debug(ERR_LOG, "can't find fingerprint pinctrl,ret=%d\n",ret);
				return ret;
			}
		} else {
			cf_debug(ERR_LOG, "platform device is null\n");
			return -1;
		}
	}
	else{
		cf_debug(ERR_LOG, "device node is null\n");
		return -1;
	}
	
	for (i = 0; i < CF_PIN_STATE_MAX; i++) 
	{
		cf_dev->pinctrl_state[i] = pinctrl_lookup_state(cf_dev->fingerprint_pinctrl, pctl_names[i]);
		if (IS_ERR(cf_dev->pinctrl_state[i])) 
		{
			cf_debug(ERR_LOG, "cannot find '%s'\n", pctl_names[i]);
			continue;
		}
		
		pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[i]);
		cf_debug(INFO_LOG, "found pin control %s\n", pctl_names[i]);
	}	
	return 0;
}

static int cf_irq_init(struct cf_device *cf_dev)
{
	int rc;
	struct device_node *node=NULL ;

	node = of_find_compatible_node(NULL, NULL, "mediatek,cs_finger");
	if (node==NULL) {
		cf_debug(ERR_LOG, "get node fail!!!\n");
		return -1;
	}
	
	cf_dev->irq = irq_of_parse_and_map(node, 0);
	cf_debug(INFO_LOG, "irq number is %d\n",cf_dev->irq);

	rc = request_threaded_irq(cf_dev->irq, NULL, cf_irq_handler,IRQF_TRIGGER_RISING | IRQF_ONESHOT, "cf_irq", cf_dev);

	if (rc) 
	{
		cf_debug(ERR_LOG, "could not request irq %d\n", cf_dev->irq);
	}
	else
	{
		cf_debug(ERR_LOG, "request irq success \n");
	}
	cf_dev->irq_enabled = 1;
	enable_irq_wake(cf_dev->irq);       //if this build error,try to use "enable_irq_wake(irq)"


	rc = of_get_named_gpio(node, "gpio-irq-std", 0);
	if (rc < 0) 
	{
		cf_debug(ERR_LOG, "failed to get gpio-irq-std return is %d \n",rc);
		return rc;
	}

	cf_dev->irq_gpio=rc;
	cf_debug(INFO_LOG, "irq gpio is %d\n",cf_dev->irq_gpio);
	if (gpio_is_valid(cf_dev->irq_gpio)) 
	{
		rc = gpio_request(cf_dev->irq_gpio, "gpio-irq-std");
		if (rc) 
		{
			cf_debug(ERR_LOG, "failed to request gpio %d\n", cf_dev->irq_gpio);
			return rc;
		}
		else
		{
			cf_debug(INFO_LOG, "success request gpio %d\n", cf_dev->irq_gpio);

		}
	} 
	else
	{
		cf_debug(ERR_LOG, "not valid gpio: %d\n", cf_dev->irq_gpio);
		rc = -EIO;
	}
	return 0;
}


static int cf_input_init(struct cf_device *cf_dev)
{
	int rc;

	FUNC_ENTRY();
	cf_dev->input = input_allocate_device();
	if (!cf_dev->input) 
	{
		cf_debug(ERR_LOG ,"input_allocate_device(..) fail\n");
		return (-ENOMEM);
	}

	cf_dev->input->name = "cf-keys";
	__set_bit(EV_KEY  , cf_dev->input->evbit );
	__set_bit(KEY_HOME, cf_dev->input->keybit);
	__set_bit(KEY_MENU, cf_dev->input->keybit);
	__set_bit(KEY_BACK, cf_dev->input->keybit);
	__set_bit(0x1c8, cf_dev->input->keybit);
	__set_bit(0x1c9, cf_dev->input->keybit);
	__set_bit(0x1ca, cf_dev->input->keybit);
	__set_bit(0x1cc, cf_dev->input->keybit);
	__set_bit(KEY_ENTER, cf_dev->input->evbit );
	__set_bit(0x1c4, cf_dev->input->keybit);
	__set_bit(0x1c6, cf_dev->input->keybit);
	__set_bit(0x1c7, cf_dev->input->keybit);
	__set_bit(0x1c5, cf_dev->input->keybit);
	__set_bit(KEY_WAKEUP, cf_dev->input->keybit);	

	rc = input_register_device(cf_dev->input);
	if(rc)
	{
		mutex_lock(&cf_dev->lock);
		input_free_device(cf_dev->input);
		cf_dev->input = NULL;
		mutex_unlock(&cf_dev->lock);
		cf_debug(ERR_LOG ,"input_register_device fail\n");
		return (-ENODEV);
	}
	FUNC_EXIT();
	return rc;
}


static int cf_reset_gpio_set_value(struct cf_device *cf_dev, unsigned char th)
{
	int rc =0;
	FUNC_ENTRY();

	if (!!th) 
	{
		rc = pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[CF_PIN_STATE_RST_HIGH]);
	}
	else 
	{
		rc = pinctrl_select_state(cf_dev->fingerprint_pinctrl, cf_dev->pinctrl_state[CF_PIN_STATE_RST_LOW]);
	}		
	return rc;
}

static int cf_report_key_event(struct input_dev* input, cf_key_event_t* kevent)
{
	int rc = 0;
	unsigned int key_code = KEY_UNKNOWN;
	FUNC_ENTRY();

	switch (kevent->key) {
		case CF_KEY_HOME:	  key_code = KEY_HOME;   break;
		case CF_KEY_MENU:	  key_code = KEY_MENU;   break;
		case CF_KEY_BACK:	  key_code = KEY_BACK;   break;
		case CF_KEY_DOWNUP:	key_code = 0x1cc;    break;
		case CF_KEY_ONETAP: key_code = 0x1c8;    break;
		case CF_KEY_DOUBLETAP: key_code = 0x1c9;    break;
		case CF_KEY_LONGTOUCH: key_code = 0x1ca;    break;
		case CF_KEY_ENTER:	key_code = KEY_ENTER;  break;
		case CF_KEY_UP: 	  key_code = 0x1c4;	   break;
		case CF_KEY_LEFT:	  key_code = 0x1c6;   break;
		case CF_KEY_RIGHT:	key_code = 0x1c7;  break;
		case CF_KEY_DOWN:	  key_code = 0x1c5;   break;
		case CF_KEY_WAKEUP: key_code = KEY_WAKEUP; break;

		default: break;
	}

	if (kevent->value == 2) 
	{
		input_report_key(input, key_code, 1);
		input_sync(input);	
		input_report_key(input, key_code, 0);
		input_sync(input);		
	} 
	else 
	{
		input_report_key(input, key_code, kevent->value);
		input_sync(input);		
	}
	FUNC_EXIT();
	return rc;	
}


static const char* cf_get_version(void)
{
	static char version[CF_DRV_VERSION_LEN] = {'\0', };
	strncpy(version, CF_DRV_VERSION, CF_DRV_VERSION_LEN);
	version[CF_DRV_VERSION_LEN - 1] = '\0';
	return (const char*)version;
}


static int cf_open(struct inode* inode, struct file* file)
{
	struct cf_device *cf_dev;
	FUNC_ENTRY();
	cf_dev = container_of(inode->i_cdev, struct cf_device, cdev);
	file->private_data = cf_dev;
	return 0;	
}

static int cf_release(struct inode* inode, struct file* file)
{
	FUNC_ENTRY();
	return 0;
}

static ssize_t cf_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	struct cf_device *cf_dev = file->private_data;
	FUNC_ENTRY();
	hw_reset(cf_dev);
	FUNC_EXIT();
	return count;
}

static long cf_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{

	struct cf_device *cf_dev = NULL;
	int err = 0;
	unsigned char th = 0;
	cf_key_event_t kevent;

	cf_dev = file->private_data;
	FUNC_ENTRY();

	if(_IOC_TYPE(cmd)!=CF_IOC_MAGIC)
		return -ENOTTY;

	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user*)arg,_IOC_SIZE(cmd));
	if(err==0 && _IOC_DIR(cmd)&_IOC_WRITE)
		err = !access_ok(VERIFY_READ,(void __user*)arg,_IOC_SIZE(cmd));
	if(err)
		return -EFAULT;	

	switch (cmd) {
		case CF_IOC_INIT_GPIO: {
			cf_debug(INFO_LOG, "CF_IOC_INIT_GPIO\n");
			mutex_lock(&cf_dev->lock);
			err = cf_gpio_init(cf_dev);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_DEINIT_GPIO: {
			cf_debug(INFO_LOG, "CF_IOC_DEINIT_GPIO :Do Nothing\n");
			break;
		}

		case CF_IOC_RESET_DEVICE: {
			cf_debug(INFO_LOG, "CF_IOC_RESET_DEVICE\n");
			if (__get_user(th, (u8 __user*)arg)) 
			{
				cf_debug(ERR_LOG, "copy_from_user(..) fail\n");
				err = (-EFAULT);
				break;
			}
			mutex_lock(&cf_dev->lock);
			cf_reset_gpio_set_value(cf_dev, th);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_ENABLE_IRQ: {
			cf_debug(INFO_LOG, "CF_IOC_ENABLE_IRQ\n");
			cf_enable_irq(cf_dev);
			break;
		}

		case CF_IOC_DISABLE_IRQ: {
			cf_debug(INFO_LOG, "CF_IOC_DISABLE_IRQ\n");
			mutex_lock(&cf_dev->lock);
			cf_disable_irq(cf_dev);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_INIT_IRQ: {
			cf_debug(INFO_LOG, "CF_IOC_REQ_IRQ\n");
			mutex_lock(&cf_dev->lock);
			err = cf_irq_init(cf_dev);   
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_ENABLE_SPI_CLK: {
			cf_debug(INFO_LOG, "CF_IOC_ENABLE_SPI_CLK\n");
			mutex_lock(&cf_dev->lock);
			cf_spi_clk_enable(cf_dev,1);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_DISABLE_SPI_CLK: {
			cf_debug(INFO_LOG, "CF_IOC_DISABLE_SPI_CLK\n");
			mutex_lock(&cf_dev->lock);
			cf_spi_clk_enable(cf_dev,0);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_ENABLE_POWER: {
			cf_debug(INFO_LOG, "CF_IOC_ENABLE_POWER :Do Nothing\n");
			break;
		}

		case CF_IOC_DISABLE_POWER: {
						    cf_debug(INFO_LOG, "CF_IOC_DISABLE_POWER :Do Nothing\n");
						    break;
					   }		

		case CF_IOC_REPORT_KEY_EVENT: {
			if (copy_from_user(&kevent, (cf_key_event_t*)arg, sizeof(cf_key_event_t))) {
				cf_debug(ERR_LOG, "copy_from_user(..) failed\n");
				err = (-EFAULT);
				break;
			}
			mutex_lock(&cf_dev->lock);
			err = cf_report_key_event(cf_dev->input, &kevent);
			mutex_unlock(&cf_dev->lock);
			break;
		}

		case CF_IOC_GET_VERSION: {
			if (copy_to_user((void*)arg, cf_get_version(), CF_DRV_VERSION_LEN)) {
				cf_debug(ERR_LOG, "copy_to_user(..) failed\n");
				err = (-EFAULT);
				break;
			}

			break;
		}
		case CF_IOC_GET_CHIPID:{



			break;

		}
		case CF_IOC_REMOVE: {
			if(cf_dev->input != NULL){
				mutex_lock(&cf_dev->lock);
				input_free_device(cf_dev->input);
				cf_dev->input = NULL;
				mutex_unlock(&cf_dev->lock);
			}
			if (cf_dev->irq) {
				free_irq(cf_dev->irq, cf_dev);
			}
			if(cf_dev->irq_gpio!=0)
			{
				gpio_free(cf_dev->irq_gpio);
			}
			
			cdev_del(&cf_dev->cdev);
			sysfs_remove_group(&cf_dev->spi->dev.kobj, &attribute_group);
			device_destroy(cf_dev->class, cf_dev->devno);
			unregister_chrdev_region(cf_dev->devno, 1);
			class_destroy(cf_dev->class);
			mutex_destroy(&cf_dev->lock);
			wake_lock_destroy(&cf_dev->ttw_wl);
			spi_set_drvdata(cf_dev->spi, NULL);
			cf_dev->spi = NULL;
			kfree(cf_dev);
			cf_dev = NULL;

			break;
		}
		default:
			err = (-EINVAL);
			break;		

	}
	return err;	
}

static const struct file_operations cf_fops =
{
	.owner			= THIS_MODULE,
	.open			= cf_open,
	.release		= cf_release,
	.unlocked_ioctl	= cf_ioctl,
	.write          = cf_write,
};

static struct mt_chip_conf cf_spi_conf =
{
	.setuptime = 7,//20,
	.holdtime = 7,//20,
	.high_time = 16,//50,
	.low_time = 17,//50,
	.cs_idletime = 3,// 5,
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.tx_endian = 0,
	.rx_endian = 0,
	.cpol = 0,
	.cpha = 0,
	.com_mod = FIFO_TRANSFER,
	.pause = 0,//1,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

static int cf_probe(struct spi_device *spi)
{
	int rc = 0;
	struct cf_device *cf_dev;
	//int vendor_id;
//+add by hzb for ontim debug
   if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
       return -EIO;
        }
//-add by hzb for ontim debug
	FUNC_ENTRY();

	cf_dev =  kzalloc(sizeof(struct cf_device), GFP_KERNEL);
	if (!cf_dev)
	{
		cf_debug(ERR_LOG,"failed to allocate memory for struct cf_device");
		rc = -ENOMEM;
		goto err;
	}
	
	cf_dev->spi = spi;
	
	//setup SPI
	cf_dev->spi->mode            = SPI_MODE_0;
	cf_dev->spi->bits_per_word   = 8;
	cf_dev->spi->max_speed_hz    = 1 * 1000 * 1000;

	memcpy(&cf_dev->spi_mcc, &cf_spi_conf, sizeof(struct mt_chip_conf));
	cf_dev->spi->controller_data = (void *)&cf_dev->spi_mcc;
	spi_setup(cf_dev->spi);

	spi_set_drvdata(spi, cf_dev);                  //save cf_dev to spi

	mutex_init(&cf_dev->lock);
	wake_lock_init(&cf_dev->ttw_wl, WAKE_LOCK_SUSPEND, "cf_ttw_wl");

	atomic_set(&cf_dev->wakeup_enabled, 1);
	
	INIT_WORK(&cf_dev->work_queue, cf_device_event);

	/*gpio init*/
	cf_gpio_init(cf_dev);
	hw_reset(cf_dev);

#if 0           //这一块后边需要删除，全部改用ca初始化中断
	/*rsee*/
	//rsee_client_get_fpid (&vendor_id);
	//cf_debug(ERR_LOG, "vendor_id=0x%x\n",vendor_id);
    vendor_id=0x131;/*set a fault value here, i don,t have this device*/
	if(vendor_id != 0x131){
		cf_debug(ERR_LOG, "venor id is not match.\n");
		rc = -ENODEV;
		goto err_class;
	}
	
		

	if(0!=cf_irq_init(cf_dev)){
		cf_debug(ERR_LOG, "Failed to init irq.\n");
		rc = -ENODEV;
		goto err_class;
	}

#endif
        
	/*create class*/
	cf_dev->class = class_create(THIS_MODULE, FP_CLASS_NAME);
	if (IS_ERR(cf_dev->class)) {
		cf_debug(ERR_LOG, "Failed to create class.\n");
		rc = -ENODEV;
		goto err_class;
	}
	
	rc = alloc_chrdev_region(&cf_dev->devno, 0, 1, FP_DEV_NAME);
	if (rc) 
	{
		cf_debug(ERR_LOG, "alloc_chrdev_region failed, error = %d\n", rc);
		goto err_devno;
	}
	
	cf_dev->device = device_create(cf_dev->class, NULL, cf_dev->devno, NULL, "%s", FP_DEV_NAME);
	if (IS_ERR(cf_dev->device)) {
		cf_debug(ERR_LOG, "Failed to create device.\n");
		rc = -ENODEV;
		goto err_device;
	} else {
		cf_debug(INFO_LOG, "device create success.\n");
	}

    cf_dev->pf_dev = platform_device_alloc(FP_DEV_NAME, -1);
    if (!cf_dev->pf_dev)
    {
        rc = -ENOMEM;
        cf_debug(ERR_LOG,"platform_device_alloc failed\n");
        goto err_sysfs;
    }
    rc = platform_device_add(cf_dev->pf_dev);
    if (rc)
    {
        cf_debug(ERR_LOG,"platform_device_add failed\n");
        platform_device_del(cf_dev->pf_dev);
        goto err_sysfs;
    }
	dev_set_drvdata(&cf_dev->pf_dev->dev, cf_dev);

	rc = sysfs_create_group(&cf_dev->pf_dev->dev.kobj, &attribute_group);
	if (rc)
	{
		cf_debug(ERR_LOG,"sysfs_create_group failed\n");
		goto err_sysfs;
	}

	/*cdev init and add*/
	cdev_init(&cf_dev->cdev, &cf_fops);
	cf_dev->cdev.owner = THIS_MODULE;

	rc = cdev_add(&cf_dev->cdev, cf_dev->devno, 1);
	if (rc) 
	{
		cf_debug(ERR_LOG, "cdev_add failed, error = %d\n", rc);
		goto err_cdev;
	}	

	rc = cf_input_init(cf_dev);
	if (rc) 
	{
		cf_debug(ERR_LOG, "could not register input\n");
		goto err_input;
	}

	FUNC_EXIT();
//+add by hzb for ontim debug
        REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//-add by hzb for ontim debug
	return 0;
err_input:
	cdev_del(&cf_dev->cdev);
err_cdev:
	sysfs_remove_group(&cf_dev->spi->dev.kobj, &attribute_group);
err_sysfs:
	device_destroy(cf_dev->class, cf_dev->devno);
err_device:	
	unregister_chrdev_region(cf_dev->devno, 1);
err_devno:
	class_destroy(cf_dev->class);
err_class:
	if(!IS_ERR(cf_dev->fingerprint_pinctrl)){
		cf_debug(ERR_LOG, "release pinctrl\n");
		devm_pinctrl_put(cf_dev->fingerprint_pinctrl);
	}
	if (cf_dev->irq) {
		free_irq(cf_dev->irq, cf_dev);
	}
	if(cf_dev->irq_gpio!=0)
	{
		gpio_free(cf_dev->irq_gpio);
	}

	mutex_destroy(&cf_dev->lock);
	wake_lock_destroy(&cf_dev->ttw_wl);
	spi_set_drvdata(cf_dev->spi, NULL);
	cf_dev->spi = NULL;
	kfree(cf_dev);
	cf_dev = NULL;
err:
	FUNC_EXIT();
	return rc;
}

static int cf_remove(struct spi_device *spi)
{
	struct cf_device *cf_dev = spi_get_drvdata(spi);

	if(cf_dev->input != NULL){
		mutex_lock(&cf_dev->lock);
		input_free_device(cf_dev->input);
		cf_dev->input = NULL;
		mutex_unlock(&cf_dev->lock);
	}
	if (cf_dev->irq) {
		free_irq(cf_dev->irq, cf_dev);
	}
	if(cf_dev->irq_gpio!=0)
	{
		gpio_free(cf_dev->irq_gpio);
	}
	
	cdev_del(&cf_dev->cdev);
	sysfs_remove_group(&cf_dev->spi->dev.kobj, &attribute_group);
	device_destroy(cf_dev->class, cf_dev->devno);
	unregister_chrdev_region(cf_dev->devno, 1);
	class_destroy(cf_dev->class);
	mutex_destroy(&cf_dev->lock);
	wake_lock_destroy(&cf_dev->ttw_wl);
	spi_set_drvdata(cf_dev->spi, NULL);
	cf_dev->spi = NULL;
	kfree(cf_dev);
	cf_dev = NULL;

	FUNC_EXIT();
	return 0;
}

/*为了兼容与其他厂家共享spi节点*/
static struct of_device_id cf_of_match[] = {
	{ .compatible = "mediatek,fpsensor", },
	{}
};
MODULE_DEVICE_TABLE(of, cf_of_match);

static struct spi_driver cf_driver = {
	.driver = {
		.name	= "chipsailing",
		.owner	= THIS_MODULE,
		.bus = &spi_bus_type,
		.of_match_table = cf_of_match,
	},
	.probe		= cf_probe,
	.remove		= cf_remove,
};

static int __init cf_driver_init(void)
{
	int rc;
	FUNC_ENTRY();

	rc = spi_register_driver(&cf_driver);
	if (!rc)
		cf_debug(ERR_LOG, "spi_register_driver(..) pass\n");
	else
		cf_debug(ERR_LOG, "spi_register_driver(..) fail, error = %d\n", rc);

	return rc;
}

static void __exit cf_driver_exit(void)
{
	FUNC_ENTRY();
	spi_unregister_driver(&cf_driver);
}

module_init(cf_driver_init);
//late_initcall(cf_driver_init);
module_exit(cf_driver_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zwp@chipsailing.com");
MODULE_DESCRIPTION("ChipSailing Fingerprint sensor device driver.");
