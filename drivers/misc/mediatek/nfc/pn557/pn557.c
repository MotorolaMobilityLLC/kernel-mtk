/*
 * Copyright (C) 2010 Trusted Logic S.A.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
//#define NFC_DEBUG_LOG

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
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include "pn557.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* for clock buffer */

#define PN557_DRVNAME		"pn557"

/* add for linux-4.4 */
#ifndef I2C_MASK_FLAG
#define I2C_MASK_FLAG  (0x00ff)
#define I2C_DMA_FLAG   (0x2000)
#define I2C_ENEXT_FLAG (0x0200)
#endif

#define MAX_BUFFER_SIZE		512
#define I2C_ID_NAME		"pn557"

struct pn557_dev
{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn557_device;
	unsigned int irq_gpio;
	bool			irq_enabled;
	spinlock_t		irq_enabled_lock;
	struct regulator	*reg;
};

// static  struct pn557_i2c_platform_data pn557_platform_data;

static  struct pinctrl *gpctrl = NULL;
static  struct pinctrl_state *st_ven_h = NULL;
static  struct pinctrl_state *st_ven_l = NULL;
static  struct pinctrl_state *st_dwn_h = NULL;
static  struct pinctrl_state *st_dwn_l = NULL;
//static  struct pinctrl_state *st_eint_int = NULL;

/* For DMA */
static unsigned char I2CDMAWriteBuf[MAX_BUFFER_SIZE];
static unsigned char I2CDMAReadBuf[MAX_BUFFER_SIZE];

/*****************************************************************************
 * Function
 *****************************************************************************/
static void pn557_enable_irq(struct pn557_dev *pn557_dev)
{
	unsigned long flags;

	printk("%s\n", __func__);
	spin_lock_irqsave(&pn557_dev->irq_enabled_lock, flags);
	if (!pn557_dev->irq_enabled) {
		pn557_dev->irq_enabled = true;
		enable_irq(pn557_dev->client->irq);
	}
	spin_unlock_irqrestore(&pn557_dev->irq_enabled_lock, flags);
}

static void pn557_disable_irq(struct pn557_dev *pn557_dev)
{
	unsigned long flags;

	printk("%s\n", __func__);
	spin_lock_irqsave(&pn557_dev->irq_enabled_lock, flags);
	if (pn557_dev->irq_enabled) {
		disable_irq_nosync(pn557_dev->client->irq);
		pn557_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn557_dev->irq_enabled_lock, flags);
}

//static int pn557_flag = 0;

static irqreturn_t pn557_dev_irq_handler(int irq, void *dev)
{
	struct pn557_dev *pn557_dev = dev;

	printk("pn557_dev_irq_handler()\n");
	pn557_disable_irq(pn557_dev);
	//pn547_flag = 1;
	/* Wake up waiting readers */
	wake_up(&pn557_dev->read_wq);

	return IRQ_HANDLED;
}

static int pn557_irq_wakeup_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pn557_dev *pn557_dev = i2c_get_clientdata(client);

    if (!pn557_dev) {
        printk("%s: device doesn't exist anymore\n", __func__);
        return -ENODEV;
    }

    enable_irq_wake(client->irq);

    printk("%s\n", __func__);
    return 0;
}

static int pn557_irq_wakeup_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pn557_dev *pn557_dev = i2c_get_clientdata(client);

    if (!pn557_dev) {
        printk("%s: device doesn't exist anymore\n", __func__);
        return -ENODEV;
    }

    disable_irq_wake(client->irq);

    printk("%s\n", __func__);
    return 0;
}

static const struct dev_pm_ops pn557_pm_ops = {
    .suspend = pn557_irq_wakeup_suspend,
    .resume = pn557_irq_wakeup_resume,
};

static int pn557_platform_pinctrl_select(struct pinctrl *p,
										 struct pinctrl_state *s)
{
	int ret = 0;

	printk("%s\n", __func__);
	if (p != NULL && s != NULL) {
		ret = pinctrl_select_state(p, s);
	} else {
		printk("%s: pinctrl_select err\n", __func__);
		ret = -1;
	}

	return ret;
}

static ssize_t pn557_dev_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *offset)
{
        struct pn557_dev *pn557_dev = filp->private_data;
        int ret = 0; // baker_mod;
#ifdef NFC_DEBUG_LOG
        int i;
#endif

        printk("%s\n", __func__);

        if (count > MAX_BUFFER_SIZE)
            count = MAX_BUFFER_SIZE;

        printk("pn557 %s : reading %zu bytes.\n", __func__, count);
        mutex_lock(&pn557_dev->read_mutex);

        if (!gpio_get_value(pn557_dev->irq_gpio)) {
            if (filp->f_flags & O_NONBLOCK) {
                ret = -EAGAIN;
                goto fail;
            }

            pn557_enable_irq(pn557_dev);

            ret = wait_event_interruptible(pn557_dev->read_wq,gpio_get_value(pn557_dev->irq_gpio));

            pn557_disable_irq(pn557_dev);
            if (ret)
                goto fail;
        }

        /*read data*/
        ret = i2c_master_recv(pn557_dev->client, I2CDMAReadBuf, count);
        mutex_unlock(&pn557_dev->read_mutex);

        if (ret < 0) {
            printk("%s: i2c_master_recv returned %d\n", __func__, ret);
            return ret;
        }

        if (ret > count) {
            printk("%s: received too many bytes from i2c (%d)\n", __func__, ret);
            return -EIO;
        }

        if (copy_to_user(buf, I2CDMAReadBuf, ret)) {
            printk("%s : failed to copy to user space\n", __func__);
            return -EFAULT;
        }

#ifdef NFC_DEBUG_LOG
        for(i = 0; i < ret; i++) {
            printk("read[%d] %02X\n", i, I2CDMAReadBuf[i]);
        }
#endif

fail:
        mutex_unlock(&pn557_dev->read_mutex);
        return ret;
}

static ssize_t pn557_dev_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offset)
{
	struct pn557_dev *pn557_dev;
	int ret = 0, idx = 0; // baker_mod_w=0;
#ifdef NFC_DEBUG_LOG
	int i;
#endif

	printk("%s\n", __func__);

	pn557_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(I2CDMAWriteBuf, &buf[(idx*255)], count)) {
		printk("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
	msleep(1);

#ifdef NFC_DEBUG_LOG
	for(i = 0; i < count; i++) {
		printk("write[%d] %02X", i,I2CDMAWriteBuf[i]);
	}
#endif

	printk("pn557 %s : writing %zu bytes.\n", __func__, count);

	ret = i2c_master_send(pn557_dev->client, I2CDMAWriteBuf, count);
	if (ret != count) {
		printk("pn557 %s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}

	udelay(1000);

	return ret;

}

static int pn557_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct pn557_dev *pn557_dev =
		container_of(filp->private_data, struct pn557_dev, pn557_device);

	printk("%s:pn557_dev=%p\n", __func__, pn557_dev);

	filp->private_data = pn557_dev;

	printk("pn557 %s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return ret;
}

static long pn557_dev_unlocked_ioctl(struct file *filp, unsigned int cmd,
				      unsigned long arg)
{
	int ret;
	struct pn557_dev *pn557_dev = filp->private_data;

	printk("%s:cmd=%d, arg=%ld, pn557_dev=%p\n", __func__, cmd, arg, pn557_dev);

	switch (cmd) {
		case PN557_SET_PWR:
			if (arg == 2) {
				/* power on with firmware download (requires hw reset) */
				printk("pn557 %s power on with firmware\n", __func__);
				ret = pn557_platform_pinctrl_select(gpctrl, st_ven_h);
				ret = pn557_platform_pinctrl_select(gpctrl, st_dwn_h);
				msleep(10);
				ret = pn557_platform_pinctrl_select(gpctrl, st_ven_l);
				msleep(50);
				ret = pn557_platform_pinctrl_select(gpctrl, st_ven_h);
				msleep(10);
			} else if (arg == 1) {
				/* power on */
				printk("pn557 %s power on\n", __func__);
				ret = pn557_platform_pinctrl_select(gpctrl, st_dwn_l);
				ret = pn557_platform_pinctrl_select(gpctrl, st_ven_h);
				msleep(10);
			} else if (arg == 0) {
				/* power off */
				printk("pn557 %s power off\n", __func__);
				ret = pn557_platform_pinctrl_select(gpctrl, st_dwn_l);
				ret = pn557_platform_pinctrl_select(gpctrl, st_ven_l);
				msleep(50);
			} else {
				printk("pn557 %s bad arg %lu\n", __func__, arg);
				return -EINVAL;
			}
			break;
		default:
			printk("pn557 %s bad ioctl %u\n", __func__, cmd);
			return -EINVAL;
	}

	return ret;
}


static const struct file_operations pn557_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pn557_dev_read,
	.write = pn557_dev_write,
	.open = pn557_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pn557_dev_unlocked_ioctl,
#endif
	.unlocked_ioctl = pn557_dev_unlocked_ioctl,
};

static int pn557_remove(struct i2c_client *client)
{
	struct pn557_dev *pn557_dev;

	pn557_dev = i2c_get_clientdata(client);
	free_irq(client->irq, pn557_dev);

	if (gpio_is_valid(pn557_dev->irq_gpio)) {
		gpio_free(pn557_dev->irq_gpio);
	} else {
		printk("%s Invalid irq gpio num.\n", __func__);
	}

	misc_deregister(&pn557_dev->pn557_device);
	mutex_destroy(&pn557_dev->read_mutex);
	kfree(pn557_dev);
	return 0;
}

static int pn557_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	int ret = 0;
	struct pn557_dev *pn557_dev;
	struct device *pn_dev = &client->dev;
	struct device_node *dNode = pn_dev->of_node;
	enum of_gpio_flags flags;

	printk("pn557_probe\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
	}

	pn557_dev = kzalloc(sizeof(*pn557_dev), GFP_KERNEL);
	printk("pn557_dev=%p\n", pn557_dev);

	if (pn557_dev == NULL) {
		printk("pn557 failed to allocate memory for module data\n");
		ret = -ENOMEM;
	}

	memset(pn557_dev, 0, sizeof(struct pn557_dev));
	pn557_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn557_dev->read_wq);
	mutex_init(&pn557_dev->read_mutex);
	spin_lock_init(&pn557_dev->irq_enabled_lock);
#if 1
	pn557_dev->pn557_device.minor = MISC_DYNAMIC_MINOR;
	pn557_dev->pn557_device.name = PN557_DRVNAME;
	pn557_dev->pn557_device.fops = &pn557_dev_fops;

	ret = misc_register(&pn557_dev->pn557_device);
	if (ret) {
		printk("%s: misc_register failed\n", __func__);
		goto err_misc_register;
	}
#endif

	/* request irq.  the irq is set whenever the chip has data available
	* for reading.  it is cleared when all data has been read.
	*/
	memset(I2CDMAWriteBuf, 0x00, sizeof(I2CDMAWriteBuf));
	memset(I2CDMAReadBuf, 0x00, sizeof(I2CDMAReadBuf));

    pn557_dev->irq_gpio = of_get_named_gpio_flags(dNode,
                     "pn557,irq-gpio", 0, &flags);
	printk("pn557_dev->irq_gpio = %d\n", pn557_dev->irq_gpio);
	if (pn557_dev->irq_gpio < 0) {
		printk("[pn557]: %s - get irq_gpio error\n", __func__);
	}

	if (gpio_is_valid(pn557_dev->irq_gpio)) {
		ret = gpio_request(pn557_dev->irq_gpio, "pn557_irq_gpio");
		if (ret < 0) {
			printk("pn557 Request gpio. Fail![%d]\n", ret);
		}
		ret = gpio_direction_input(pn557_dev->irq_gpio);
		if (ret < 0) {
			printk("pn557 Set gpio direction. Fail![%d]\n", ret);
		}
		client->irq = gpio_to_irq(pn557_dev->irq_gpio);
	} else {
		printk("pn557 Invalid irq gpio num.(init)\n");
	}

#if 0
	/*  NFC IRQ settings     */
	node = of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");

	if (node) {
		of_property_read_u32_array(node, "gpio-irq",
								   &(pn547_dev->irq_gpio), 1);
		printk("pn547_dev->irq_gpio = %d\n", pn547_dev->irq_gpio);
	} else {
		printk("%s : get gpio num err.\n", __func__);
		return -1;
	}
#endif


#if 0
	node = of_find_compatible_node(NULL, NULL, "mediatek,irq_nfc-eint");
	if (node) {
		client->irq = irq_of_parse_and_map(node, 0);
#endif
	printk("pn557 client->irq = %d\n", client->irq);

	ret = request_irq(client->irq, pn557_dev_irq_handler,
					  IRQF_TRIGGER_HIGH, "nfc_eint_as_int", pn557_dev);
	if (ret) {
		printk("%s: EINT IRQ LINE NOT AVAILABLE, ret = %d\n", __func__, ret);
	} else {
		printk("%s: set EINT finished, client->irq=%d\n", __func__,
			   client->irq);
		pn557_dev->irq_enabled = true;
		pn557_disable_irq(pn557_dev);
	}
#if 0
	} else {
		printk("%s: can not find NFC eint compatible node\n", __func__);
	}
#endif

	// Temp soultion by Zhao Fei
	//clk_buf_ctrl(CLK_BUF_NFC, 1);

	//ret= nfcc_hw_check(client, pn547_dev);
	/*		by holly
	if (ret) {
		ret = pn547_platform_pinctrl_select(gpctrl, st_ven_l);
		goto err_request_hw_check_failed;
	}
	*/

	i2c_set_clientdata(client, pn557_dev);
	printk("%s, probing pn547 driver exited successfully\n", __func__);

	return 0;

//err_request_hw_check_failed:
//	free_irq(client->irq, pn557_dev);

err_misc_register:
	mutex_destroy(&pn557_dev->read_mutex);
	kfree(pn557_dev);

	return ret;
}

static const struct i2c_device_id pn557_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

static const struct of_device_id nfc_i2c_of_match[] = {
	{.compatible = "mediatek,nfc"},
	{},
};

static struct i2c_driver pn557_driver =
{
	.id_table	= pn557_id,
	.probe		= pn557_probe,
	.remove		= pn557_remove,
	.driver		= {
		.name = "pn557",
		.owner = THIS_MODULE,
		.pm = &pn557_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = nfc_i2c_of_match,
#endif
	},
};


static int pn557_platform_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s\n", __func__);

	gpctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(gpctrl)) {
		printk("Cannot find pinctrl!");
		ret = PTR_ERR(gpctrl);
		goto end;
	}

	st_ven_h = pinctrl_lookup_state(gpctrl, "ven_high");
	if (IS_ERR(st_ven_h)) {
		ret = PTR_ERR(st_ven_h);
		printk("%s: pinctrl err, st_ven_h\n", __func__);
		goto end;
	}

	st_ven_l = pinctrl_lookup_state(gpctrl, "ven_low");
	if (IS_ERR(st_ven_l)) {
		ret = PTR_ERR(st_ven_l);
		printk("%s: pinctrl err, st_ven_l\n", __func__);
		goto end;
	}
	//modify by tzf@meitu.begin
	st_dwn_h = pinctrl_lookup_state(gpctrl, "dwn_high");
	if (IS_ERR(st_dwn_h)) {
		ret = PTR_ERR(st_dwn_h);
		printk("%s: pinctrl err, st_dwn_h\n", __func__);
		goto end;
	}

	st_dwn_l = pinctrl_lookup_state(gpctrl, "dwn_low");
	if (IS_ERR(st_dwn_l)) {
		ret = PTR_ERR(st_dwn_l);
		printk("%s: pinctrl err, st_dwn_l\n", __func__);
		goto end;
	}

	/* select state */
	ret = pn557_platform_pinctrl_select(gpctrl, st_ven_l);
	usleep_range(900, 1000);

	ret = pn557_platform_pinctrl_select(gpctrl, st_dwn_l);
	usleep_range(900, 1000);


end:
	return ret;
}

static int pn557_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s: &pdev=%p\n", __func__, pdev);

	/* pinctrl init */
	ret = pn557_platform_pinctrl_init(pdev);

	return ret;
}

static int pn557_platform_remove(struct platform_device *pdev)
{
	printk("%s: &pdev=%p\n", __func__, pdev);

	return 0;
}

/*  platform driver */
static const struct of_device_id pn557_platform_of_match[] = {
	{.compatible = "mediatek,nfc-gpio-v2",},
	{},
};

static struct platform_driver pn557_platform_driver = {
	.probe		= pn557_platform_probe,
	.remove		= pn557_platform_remove,
	.driver		= {
		.name = PN557_DRVNAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = pn557_platform_of_match,
#endif
	},
};

/*
 * module load/unload record keeping
 */
static int __init pn557_dev_init(void)
{
	int ret = 0;
	int ret1 = 0;
	printk("pn557_dev_init\n");

        /* nfc sku config */
        if (strnstr(saved_command_line, "androidboot.product.vendor.sku=SKUB", sizeof(saved_command_line))) {
            printk("platform_driver_register not support nfc\n");
            return -EPERM;
        }

        ret1 = platform_driver_register(&pn557_platform_driver);
	printk(" platform_driver_register:%d\n",ret1);

	ret = i2c_add_driver(&pn557_driver);

	printk("i2c_add_driver :%d\n",ret);

	return 0;
}
module_init(pn557_dev_init);

static void __exit pn557_dev_exit(void)
{
	printk("Unloading pn557 driver\n");

	i2c_del_driver(&pn557_driver);
}
module_exit(pn557_dev_exit);

MODULE_AUTHOR("NXP");
MODULE_DESCRIPTION("NFC PN557 driver");
MODULE_LICENSE("GPL");


