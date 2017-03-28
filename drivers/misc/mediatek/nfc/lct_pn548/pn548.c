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

#define DEBUG	1

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
#include <linux/proc_fs.h> 
#include <linux/regulator/consumer.h>
#include <pn548.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* add lct_sku by changjingyang start */ 
#ifdef CONFIG_LCT_BOOT_REASON
extern int lct_get_sku(void);
#endif
/* add lct_sku by changjingyang end */ 

// add by zhaofei - 2016-11-09-17-31
//#define LCT_NFC_CLK_REQ
#define LCT_SYS_CLK
#ifdef LCT_NFC_CLK_REQ
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/of_platform.h>
#endif
#include <mt_clkbuf_ctl.h>	/*  for clock buffer */
//end add by zhaofei - 2016-11-09-17-31

#define PN544_DRVNAME		"pn547"

#define MAX_BUFFER_SIZE		512
#define I2C_ID_NAME		"pn544"

#define PN544_MAGIC		0xE9
#define PN544_SET_PWR		_IOW(PN544_MAGIC, 0x01, unsigned int)


#ifndef CONFIG_MTK_I2C_EXTENSION 
#define CONFIG_MTK_I2C_EXTENSION
#endif

//#undef CONFIG_MTK_I2C_EXTENSION // baker add ,without DMA Mode
/******************************************************************************
 * extern functions
 *******************************************************************************/
//extern void mt_eint_mask(unsigned int eint_num);
//extern void mt_eint_unmask(unsigned int eint_num);
//extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
//extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);

#ifdef CONFIG_MTK_BOARD_ID
#include "../../board_id/board_id.h"//huyunge@wind-mobi.com 20161209
extern int get_bid_gpio(void);
#endif

extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

struct pn544_dev	
{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	nfc_device;
	bool			irq_enabled;
	spinlock_t		irq_enabled_lock;

	int			irq_gpio;
	struct regulator	*reg;
};

//struct pn544_i2c_platform_data pn544_platform_data;

struct pinctrl *gpctrl = NULL;
struct pinctrl_state *st_ven_h = NULL;
struct pinctrl_state *st_ven_l = NULL;
struct pinctrl_state *st_dwn_h = NULL;
struct pinctrl_state *st_dwn_l = NULL;

struct pinctrl_state *st_eint_int = NULL;

/* For DMA */

#ifdef CONFIG_MTK_I2C_EXTENSION
static char *I2CDMAWriteBuf;	/*= NULL;*//* unnecessary initialise */
static unsigned int I2CDMAWriteBuf_pa;	/* = NULL; */
static char *I2CDMAReadBuf;	/*= NULL;*//* unnecessary initialise */
static unsigned int I2CDMAReadBuf_pa;	/* = NULL; */
#else
static char I2CDMAWriteBuf[MAX_BUFFER_SIZE];
static char I2CDMAReadBuf[MAX_BUFFER_SIZE];
#endif

// add by zhaofei - 2016-11-09-17-31
#ifdef LCT_NFC_CLK_REQ
struct pinctrl_state *clk_req_int = NULL;
static unsigned int nfc_clk_req_irq = 0;
static int transfer_clk_onoff = 0;
struct platform_device *nfc_clk_req_dev;
static struct work_struct  eint1_work;
#endif
#ifndef CONFIG_WIND_DEVICE_INFO
struct pinctrl_state *nfc_ldo_low_pinctrl = NULL;
struct pinctrl_state *nfc_ldo_high_pinctrl = NULL;
#endif
//end add by zhaofei - 2016-11-09-17-31

/* add LCT_DEVINFO by changjingyang start */
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define LCT_DEVINFO_NFC_DEBUG
#include  "dev_info.h"
struct devinfo_struct *s_DEVINFO_nfc;    
static void devinfo_nfc_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_nfc =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_nfc->device_type="NFC";
	s_DEVINFO_nfc->device_module=module;
	s_DEVINFO_nfc->device_vendor=vendor;
	s_DEVINFO_nfc->device_ic="PN548";
	s_DEVINFO_nfc->device_info=DEVINFO_NULL;
	s_DEVINFO_nfc->device_version=version;
	s_DEVINFO_nfc->device_used=used;
#ifdef LCT_DEVINFO_NFC_DEBUG
		printk("[DEVINFO NFC]registe nfc device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_nfc->device_type,s_DEVINFO_nfc->device_module,s_DEVINFO_nfc->device_vendor,
				s_DEVINFO_nfc->device_ic,s_DEVINFO_nfc->device_version,s_DEVINFO_nfc->device_info,s_DEVINFO_nfc->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_nfc->device_type,s_DEVINFO_nfc->device_module,s_DEVINFO_nfc->device_vendor,s_DEVINFO_nfc->device_ic,s_DEVINFO_nfc->device_version,s_DEVINFO_nfc->device_info,s_DEVINFO_nfc->device_used);
}
#endif
/* add LCT_DEVINFO by changjingyang end */

/*****************************************************************************
 * Function
 *****************************************************************************/

static void pn544_enable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;
	printk("%s\n", __func__);

	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	enable_irq(pn544_dev->client->irq);
	pn544_dev->irq_enabled = true;
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;

	printk("%s\n", __func__);


	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) 
	{
		//mt_eint_mask(EINT_NUM);
		disable_irq_nosync(pn544_dev->client->irq);
		pn544_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

#ifdef LCT_NFC_CLK_REQ
static irqreturn_t pn544_clk_request_on_handler(int irq, void *dev)
{
	//printk("pn544_clk_request_on_handler()\n");		

	disable_irq_nosync(nfc_clk_req_irq);
	/* Wake up waiting readers */
	schedule_work(&eint1_work);

	return IRQ_HANDLED;
}
#endif

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev)
{
	struct pn544_dev *pn544_dev = dev;

	printk("pn544_dev_irq_handler()\n");		

	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

	return IRQ_HANDLED;
}

static int pn544_platform_pinctrl_select(struct pinctrl *p, struct pinctrl_state *s)
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

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *offset)
{
	int i;
	struct pn544_dev *pn544_dev = filp->private_data;
	int ret=0;//baker_mod;
	char tmp_buf[256] = {0};
	printk("%s\n", __func__);

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	printk("pn544 %s : reading %zu bytes.\n", __func__, count);
	mutex_lock(&pn544_dev->read_mutex);
	if (!gpio_get_value(pn544_dev->irq_gpio)) {
		printk("pn544 read no event\n");		
		if (filp->f_flags & O_NONBLOCK) 
		{
			ret = -EAGAIN;
			goto fail;
		}
		
		printk("pn544 read wait event\n");		
		pn544_enable_irq(pn544_dev);
	
		printk("pn544 read1  pn544_dev->irq_gpio=%d\n", pn544_dev->irq_gpio);			
		printk("pn544 read2  gpio_get_value(pn544_dev->irq_gpio)=%d\n", gpio_get_value(pn544_dev->irq_gpio));		

		printk("fangzhihua pn544 read1  pn544_dev->irq_gpio=%d\n", pn544_dev->irq_gpio);			

		ret = wait_event_interruptible(pn544_dev->read_wq, gpio_get_value(pn544_dev->irq_gpio));

		printk("cjy pn544 read1  pn544_dev->irq_gpio=%d\n", pn544_dev->irq_gpio);	

		pn544_disable_irq(pn544_dev);

		if (ret) 
		{
			printk("pn544 read wait event error\n");
			goto fail;
		}
	}

#ifdef CONFIG_MTK_I2C_EXTENSION
    if (count < 8) {
         pn544_dev->client->addr = ((pn544_dev->client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
         pn544_dev->client->timing = 400;
          ret = i2c_master_recv(pn544_dev->client, tmp_buf, count);
          printk("[fangzhihua] %s ,defined  CONFIG_MTK_I2C_EXTENSION  R1 \n",__func__);
    }
    else
    {
   	pn544_dev->client->addr = (((pn544_dev->client->addr & I2C_MASK_FLAG) | (I2C_DMA_FLAG)) | (I2C_ENEXT_FLAG));
   	pn544_dev->client->timing = 400;
       ret = i2c_master_recv(pn544_dev->client, (unsigned char *)(uintptr_t)I2CDMAReadBuf_pa, count);
	if (ret < 0)
           return ret;
	memcpy(tmp_buf,I2CDMAReadBuf,count);
//	for (i = 0; i < count; i++)// add by zhaofei - 2016-11-29-10-43
//           tmp_buf[i] = I2CDMAReadBuf[i];
    printk("[fangzhihua] %s ,defined  CONFIG_MTK_I2C_EXTENSION R2 \n",__func__);
   }

#else
    if (count < 8)
          ret = i2c_master_recv(pn544_dev->client, tmp_buf, count);
         printk("[fangzhihua] %s ,defined  CONFIG_MTK_I2C_EXTENSION R3 \n",__func__);
    {
        ret = i2c_master_recv(pn544_dev->client, (unsigned char *)(uintptr_t)I2CDMAReadBuf, count);
       if (ret < 0)
               return ret;
	   
	memcpy(tmp_buf,I2CDMAReadBuf,count);
//       for (i = 0; i < count; i++)
//              tmp_buf[i] = I2CDMAReadBuf[i];
      printk("[fangzhihua] %s ,defined  CONFIG_MTK_I2C_EXTENSION R4 \n",__func__);

    }
#endif                            /* CONFIG_MTK_I2C_EXTENSION */

	if(copy_to_user(buf,tmp_buf,ret))// add by zhaofei - 2016-11-29-10-44
	{
		pr_err("%s failed to copy to user space, line = %d\n",__FUNCTION__,__LINE__);
		goto fail;
	}

	mutex_unlock(&pn544_dev->read_mutex);

	if (ret < 0) 
	{
		pr_err("pn544 %s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) 
	{
		pr_err("pn544 %s: received too many bytes from i2c (%d)\n", __func__, ret);
		return -EIO;
	}
	
	printk("pn544 IFD->PC:");
	for(i = 0; i < ret; i++) 
	{
		printk(" %02X", I2CDMAReadBuf[i]);
	}
	printk("\n");

	return ret;

fail:
	mutex_unlock(&pn544_dev->read_mutex);
	return ret;
}


static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev;
	int ret=0, i,idx = 0; //baker_mod_w=0;
	char tmp[MAX_BUFFER_SIZE];
	
	printk("%s\n", __func__);

	pn544_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;
	if (copy_from_user(tmp, buf, count)) 
	{
		printk("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(I2CDMAWriteBuf, &buf[(idx*255)], count)) 
	{
		printk(KERN_DEBUG "%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
	msleep(15);
	printk("pn544 %s : writing %zu bytes.\n", __func__, count);


#ifdef CONFIG_MTK_I2C_EXTENSION
   for (i = 0; i < count; i++){
		I2CDMAWriteBuf[i] = buf[i];
		printk("[Bert] buf[%d] = %d\n", i, buf[i]);
   	}
    if (count < 8) {
         pn544_dev->client->addr = ((pn544_dev->client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
	printk("[fangzhihua] pn544_dev->client->addr =%d\n", pn544_dev->client->addr);
         //pn544_dev->client->timing = 400;
        // ret = i2c_master_send(pn544_dev->client, buf, count);
        ret =  i2c_master_send(pn544_dev->client, tmp, count);
	  printk("[fangzhihua] %s ,CONFIG_MTK_I2C_EXTENSION    w1 , ret = %d \n",__func__, ret);

    }
    else
    {
   	pn544_dev->client->addr = (((pn544_dev->client->addr & I2C_MASK_FLAG) | (I2C_DMA_FLAG)) | (I2C_ENEXT_FLAG));
   	pn544_dev->client->timing = 400;
       ret = i2c_master_send(pn544_dev->client, (unsigned char *)(uintptr_t)I2CDMAWriteBuf_pa, count);
	printk("[fangzhihua] %s ,CONFIG_MTK_I2C_EXTENSION   w2 \n",__func__);
   }
#else
   for (i = 0; i < count; i++)
		I2CDMAWriteBuf[i] = buf[i];

    if (count < 8)
    {
          ret = i2c_master_send(pn544_dev->client, buf, count);
	   printk("[fangzhihua] %s ,CONFIG_MTK_I2C_EXTENSION   w3 \n",__func__);
    }
    else
    {
        ret = i2c_master_send(pn544_dev->client, (unsigned char *)(uintptr_t)I2CDMAWriteBuf, count);
        printk("[fangzhihua] %s ,CONFIG_MTK_I2C_EXTENSION   w4 \n",__func__);
    }
#endif                            /* CONFIG_MTK_I2C_EXTENSION */

	if (ret != count) 
	{
		pr_err("pn544 %s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}
	printk("pn544 PC->IFD:");
	for(i = 0; i < count; i++) 
	{
		printk(" %02X\n", I2CDMAWriteBuf[i]);
	}
	printk("\n");

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct pn544_dev *pn544_dev = container_of(filp->private_data, struct pn544_dev, nfc_device);
	
	printk("%s:pn544_dev=%p\n", __func__, pn544_dev);

	filp->private_data = pn544_dev;
	
	pr_debug("pn544 %s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return ret;
}

static long pn544_dev_unlocked_ioctl(struct file *filp, unsigned int cmd,
				      unsigned long arg)
{
	int ret;
	struct pn544_dev *pn544_dev = filp->private_data;

	printk("%s:cmd=%d, arg=%ld, pn544_dev=%p\n", __func__, cmd, arg, pn544_dev);

	switch (cmd) 
	{
		case PN544_SET_PWR:
			if (arg == 2) {
				/* power on with firmware download (requires hw reset) */
				printk("pn544 %s power on with firmware\n", __func__);
#ifndef CONFIG_WIND_DEVICE_INFO                
				ret = pn544_platform_pinctrl_select(gpctrl, nfc_ldo_high_pinctrl); // add by zhaofei - 2016-11-17-18-17
#endif                
				ret = pn544_platform_pinctrl_select(gpctrl, st_ven_h);
				ret = pn544_platform_pinctrl_select(gpctrl, st_dwn_h);
				msleep(10);
#ifndef CONFIG_WIND_DEVICE_INFO                
				ret = pn544_platform_pinctrl_select(gpctrl, nfc_ldo_low_pinctrl); // add by zhaofei - 2016-11-17-18-17
#endif                
				ret = pn544_platform_pinctrl_select(gpctrl, st_ven_l);
				msleep(50);
#ifndef CONFIG_WIND_DEVICE_INFO                
				ret = pn544_platform_pinctrl_select(gpctrl, nfc_ldo_high_pinctrl); // add by zhaofei - 2016-11-17-18-17
#endif                
				ret = pn544_platform_pinctrl_select(gpctrl, st_ven_h);
				msleep(10);
			} else if (arg == 1) {
				/* power on */
				printk("pn544 %s power on\n", __func__);
#ifdef LCT_SYS_CLK
				clk_buf_ctrl(CLK_BUF_NFC, 1);
#endif
#ifndef CONFIG_WIND_DEVICE_INFO
				ret = pn544_platform_pinctrl_select(gpctrl, nfc_ldo_high_pinctrl); // add by zhaofei - 2016-11-17-18-17
#endif                
				msleep(1);
				ret = pn544_platform_pinctrl_select(gpctrl, st_dwn_l);
				ret = pn544_platform_pinctrl_select(gpctrl, st_ven_h); 
				msleep(10);
			} else  if (arg == 0) {
				/* power off */
				printk("pn544 %s power off\n", __func__);
				ret = pn544_platform_pinctrl_select(gpctrl, st_dwn_l);
				ret = pn544_platform_pinctrl_select(gpctrl, st_ven_l);
				msleep(1);
#ifndef CONFIG_WIND_DEVICE_INFO                
				ret = pn544_platform_pinctrl_select(gpctrl, nfc_ldo_high_pinctrl); // add by zhaofei - 2016-11-17-18-17
#endif                
#ifdef LCT_SYS_CLK
				clk_buf_ctrl(CLK_BUF_NFC, 0);
#endif
				msleep(50);
			} else {
				printk("pn544 %s bad arg %lu\n", __func__, arg);
				return -EINVAL;
			}
			break;
		default:
			printk("pn544 %s bad ioctl %u\n", __func__, cmd);
			return -EINVAL;
	}

	return ret;
}


static const struct file_operations pn544_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pn544_dev_read,
	.write = pn544_dev_write,
	.open = pn544_dev_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pn544_dev_unlocked_ioctl,
#endif
	.unlocked_ioctl = pn544_dev_unlocked_ioctl,
};

static int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

#ifdef CONFIG_MTK_I2C_EXTENSION
	if (I2CDMAWriteBuf) {
		#if 1//def CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAWriteBuf,
				  I2CDMAWriteBuf_pa);
		#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAWriteBuf,
				  I2CDMAWriteBuf_pa);
		#endif
		I2CDMAWriteBuf = NULL;
		I2CDMAWriteBuf_pa = 0;
	}

	if (I2CDMAReadBuf) {
		#if 1//def CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAReadBuf,
				  I2CDMAReadBuf_pa);
		#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAReadBuf,
				  I2CDMAReadBuf_pa);
		#endif
		I2CDMAReadBuf = NULL;
		I2CDMAReadBuf_pa = 0;
	}
#endif

	pn544_dev = i2c_get_clientdata(client);
	misc_deregister(&pn544_dev->nfc_device);
	mutex_destroy(&pn544_dev->read_mutex);
	regulator_put(pn544_dev->reg);
	kfree(pn544_dev);
	return 0;
}

// add by zhaofei - 2016-11-09-17-48
#ifdef LCT_NFC_CLK_REQ
static void nfc_clk_req_on_thread(struct work_struct *work)
{
		if(transfer_clk_onoff == 0)
		{
			clk_buf_ctrl(CLK_BUF_NFC, 1);
			transfer_clk_onoff = 1;
			//irq_set_irq_type(nfc_clk_req_irq, IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_SHARED);
			irq_set_irq_type(nfc_clk_req_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_SHARED);
			//printk("nfc_clk_req_on_thread on\n");
		}
		else
		{
			clk_buf_ctrl(CLK_BUF_NFC, 0);
			transfer_clk_onoff = 0;
			//irq_set_irq_type(nfc_clk_req_irq, IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_SHARED);
			irq_set_irq_type(nfc_clk_req_irq, IRQF_TRIGGER_HIGH | IRQF_ONESHOT | IRQF_SHARED);
			//printk("nfc_clk_req_on_thread off\n");
		}
		enable_irq(nfc_clk_req_irq);
	
		return ;
}
#endif
// add by zhaofei - 2016-11-09-17-48

static int pn544_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	int ret=0;
	struct pn544_dev *pn544_dev;
	struct device_node *node;

	printk("%s: start...\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		pr_err("%s: need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	printk("%s: step02 is ok\n", __func__);

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	printk("pn544_dev=%p\n", pn544_dev);

	if (pn544_dev == NULL) 
	{
		dev_err(&client->dev, "pn544 failed to allocate memory for module data\n");
		return -ENOMEM;
	}

	memset(pn544_dev, 0, sizeof(struct pn544_dev));

	printk("%s: step03 is ok\n", __func__);

	pn544_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);
#if 1
	pn544_dev->nfc_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->nfc_device.name = PN544_DRVNAME;
	pn544_dev->nfc_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->nfc_device);
	if (ret) 
	{
		pr_err("%s: misc_register failed\n", __func__);
		goto err_misc_register;
	}
#endif    
	printk("%s: step04 is ok\n", __func__);
	
	/* request irq.  the irq is set whenever the chip has data available
	* for reading.  it is cleared when all data has been read.
	*/    
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#if 1//def CONFIG_64BIT
	I2CDMAWriteBuf =
	    (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAWriteBuf_pa,
				       GFP_KERNEL);
#else
	I2CDMAWriteBuf =
	    (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAWriteBuf_pa,
				       GFP_KERNEL);
#endif

	if (I2CDMAWriteBuf == NULL) {
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		mutex_destroy(&pn544_dev->read_mutex);
		//gpio_free(platform_data->sysrstb_gpio);
		return ret;
	}
#if 1//def CONFIG_64BIT
	I2CDMAReadBuf =
	    (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAReadBuf_pa,
				       GFP_KERNEL);
#else
	I2CDMAReadBuf =
	    (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				       (dma_addr_t *) &I2CDMAReadBuf_pa,
				       GFP_KERNEL);
#endif

	if (I2CDMAReadBuf == NULL) {
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		mutex_destroy(&pn544_dev->read_mutex);
		//gpio_free(platform_data->sysrstb_gpio);
		return ret;
	}
	pr_debug("%s :I2CDMAWriteBuf_pa %d, I2CDMAReadBuf_pa,%d\n", __func__,
		 I2CDMAWriteBuf_pa, I2CDMAReadBuf_pa);
#else
	memset(I2CDMAWriteBuf, 0x00, sizeof(I2CDMAWriteBuf));
	memset(I2CDMAReadBuf, 0x00, sizeof(I2CDMAReadBuf));
#endif

	printk("%s: step05 is ok\n", __func__);

	/*  NFC IRQ settings     */	
	node = of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");

	if (node) {
		of_property_read_u32_array(node, "gpio-irq",
					   &(pn544_dev->irq_gpio), 1);		
		printk("pn544_dev->irq_gpio = %d\n", pn544_dev->irq_gpio);
	} else {
		pr_debug("%s : get gpio num err.\n", __func__);
		return -1;
	}

	if (node) {
		client->irq = irq_of_parse_and_map(node, 0);
		printk("pn544 client->irq = %d\n", client->irq);

		ret =
		    request_irq(client->irq, pn544_dev_irq_handler,
				IRQF_TRIGGER_NONE, "nfc_eint_as_int", pn544_dev);

		if (ret) {
			pr_err("%s: EINT IRQ LINE NOT AVAILABLE, ret = %d\n", __func__, ret);
		} else {

			printk("%s: set EINT finished, client->irq=%d\n", __func__,
				 client->irq);

			pn544_dev->irq_enabled = true;
			pn544_disable_irq(pn544_dev);
		}

	} else {
		pr_err("%s: can not find NFC eint compatible node\n",
		       __func__);
	}

	// add by zhaofei - 2016-11-09-17-04
#ifdef LCT_NFC_CLK_REQ
	node = of_find_compatible_node(NULL, NULL, "mediatek, nfc_clk_req");
	if (node) {
		
		nfc_clk_req_irq = irq_of_parse_and_map(node, 0);
		pr_err("nfc_clk_irq = %u\n", nfc_clk_req_irq);

		nfc_clk_req_dev= kzalloc(sizeof(*nfc_clk_req_dev), GFP_KERNEL);
		printk("nfc_clk_req_dev=%p\n", nfc_clk_req_dev);

		if (nfc_clk_req_dev == NULL) 
		{
			dev_err(&client->dev, "pn544 failed to allocate memory for nfc clk data\n");
			return -ENOMEM;
		}

		nfc_clk_req_dev = of_find_device_by_node(node);

		INIT_WORK(&eint1_work, nfc_clk_req_on_thread);
		//ret = request_irq(nfc_clk_req_irq, pn544_clk_request_on_handler, IRQF_SHARED | IRQF_TRIGGER_RISING| IRQF_ONESHOT, "nfc_clk_req", nfc_clk_req_dev);
		//mt_eint_set_hw_debounce(nfc_clk_req_irq, 1);
		ret = request_irq(nfc_clk_req_irq, pn544_clk_request_on_handler, IRQF_SHARED | IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "nfc_clk_req", nfc_clk_req_dev);

		if (ret) {
			pr_err("%s: CLK_REQ_EINT IRQ LINE NOT AVAILABLE, ret = %d\n", __func__, ret);
		} else {
			printk("%s: set CLK_REQ_EINT finished, client->irq=%d\n", __func__,
				 nfc_clk_req_irq);
			enable_irq(nfc_clk_req_irq);
		}

	} else
		pr_err("%s can't find compatible node,line=%d\n", __func__,__LINE__);
	//clk_buf_ctrl(CLK_BUF_NFC, 1);
#else
#ifndef LCT_SYS_CLK			
	clk_buf_ctrl(CLK_BUF_NFC, 1);
#endif
#endif
// end add by zhaofei - 2016-11-09-17-05

/* add by changjingyang  start */
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
		devinfo_nfc_regchar("PN548","ATU","NXP",DEVINFO_USED);
#endif
/* add by changjingyang  end */

	pn544_platform_pinctrl_select(gpctrl, st_ven_h);

	i2c_set_clientdata(client, pn544_dev);
       printk("%s: step06 success\n", __func__);
	return 0;


err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
	return ret;
}

static const struct i2c_device_id pn544_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

static const struct of_device_id nfc_i2c_of_match[] = {
	{.compatible = "mediatek,nfc"},
	{},
};

static struct i2c_driver pn544_i2c_driver = 
{
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	/* .detect	= pn544_detect, */
	.driver		= {
		.name = "pn544",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nfc_i2c_of_match,
#endif
	},
};


static int pn544_platform_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s\n", __func__);

	gpctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(gpctrl)) {
		dev_err(&pdev->dev, "Cannot find pinctrl!");
		ret = PTR_ERR(gpctrl);
		goto end;
	}

	st_ven_h = pinctrl_lookup_state(gpctrl, "ven_high");
	if (IS_ERR(st_ven_h)) {
		ret = PTR_ERR(st_ven_h);
		printk("%s: pinctrl err, ven_high\n", __func__);
	}

	st_ven_l = pinctrl_lookup_state(gpctrl, "ven_low");
	if (IS_ERR(st_ven_l)) {
		ret = PTR_ERR(st_ven_l);
		printk("%s: pinctrl err, ven_low\n", __func__);
	}
#ifndef CONFIG_WIND_DEVICE_INFO    
//modify by tzf@meitu.begin
	st_dwn_h = pinctrl_lookup_state(gpctrl, "dwn_high");
    if (IS_ERR(st_dwn_h)) {
    	ret = PTR_ERR(st_dwn_h);
    	printk("%s: pinctrl err, dwn_high\n", __func__);
    }
    
    
	st_dwn_l = pinctrl_lookup_state(gpctrl, "dwn_low");
    if (IS_ERR(st_dwn_l)) {
    	ret = PTR_ERR(st_dwn_l);
    	printk("%s: pinctrl err, dwn_low\n", __func__);
    }
    
    
	st_eint_int = pinctrl_lookup_state(gpctrl, "eint_as_int");
    if (IS_ERR(st_eint_int)) {
    	ret = PTR_ERR(st_eint_int);
    	printk("%s: pinctrl err, st_eint_int\n", __func__);
    }
    pn544_platform_pinctrl_select(gpctrl, st_eint_int);
#else
	st_dwn_h = pinctrl_lookup_state(gpctrl, "rst_high");
    if (IS_ERR(st_dwn_h)) {
    	ret = PTR_ERR(st_dwn_h);
    	printk("%s: pinctrl err, dwn_high\n", __func__);
    }
    
    
	st_dwn_l = pinctrl_lookup_state(gpctrl, "rst_low");
    if (IS_ERR(st_dwn_l)) {
    	ret = PTR_ERR(st_dwn_l);
    	printk("%s: pinctrl err, dwn_low\n", __func__);
    }
    
    
	st_eint_int = pinctrl_lookup_state(gpctrl, "irq_init");
    if (IS_ERR(st_eint_int)) {
    	ret = PTR_ERR(st_eint_int);
    	printk("%s: pinctrl err, st_eint_pn548_int\n", __func__);
    }
    pn544_platform_pinctrl_select(gpctrl, st_eint_int);
#endif

// add by zhaofei - 2016-11-09-16-23
#ifdef LCT_NFC_CLK_REQ
	clk_req_int = pinctrl_lookup_state(gpctrl, "clk_req_int");
    if (IS_ERR(clk_req_int)) {
    	ret = PTR_ERR(clk_req_int);
    	printk("%s: pinctrl err, clk_req_int\n", __func__);
    }
    pn544_platform_pinctrl_select(gpctrl, clk_req_int);
#endif

#ifndef CONFIG_WIND_DEVICE_INFO
	nfc_ldo_high_pinctrl = pinctrl_lookup_state(gpctrl, "nfc_ldo_high");
    if (IS_ERR(nfc_ldo_high_pinctrl)) {
    	ret = PTR_ERR(nfc_ldo_high_pinctrl);
    	printk("%s: pinctrl err, nfc_ldo_high_pinctrl\n", __func__);
    }
    //pn544_platform_pinctrl_select(gpctrl, nfc_ldo_high_pinctrl);
	
	nfc_ldo_low_pinctrl = pinctrl_lookup_state(gpctrl, "nfc_ldo_low");
    if (IS_ERR(nfc_ldo_low_pinctrl)) {
    	ret = PTR_ERR(nfc_ldo_low_pinctrl);
    	printk("%s: pinctrl err, nfc_ldo_low_pinctrl\n", __func__);
    }
#endif    
    //pn544_platform_pinctrl_select(gpctrl, nfc_ldo_low_pinctrl);
//end add by zhaofei - 2016-11-09-16-23
	
end:
	return ret;
}

static int pn544_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s: &pdev=%p\n", __func__, pdev);

	/* pinctrl init */
	ret = pn544_platform_pinctrl_init(pdev);

	return ret;
}

static int pn544_platform_remove(struct platform_device *pdev)
{
	printk("%s: &pdev=%p\n", __func__, pdev);

	return 0;
}

/*  platform driver */
static const struct of_device_id pn544_platform_of_match[] = {
	{.compatible = "mediatek,nfc-gpio-v2",},
	{},
};

static struct platform_driver pn544_platform_driver = {
	.probe		= pn544_platform_probe,
	.remove		= pn544_platform_remove,
	.driver		= {
		.name = PN544_DRVNAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = pn544_platform_of_match,
#endif
	},
};

/*
 * module load/unload record keeping
 */
static int __init pn544_dev_init(void)
{
	int ret;
/* add lct_sku by changjingyang start */
#ifdef CONFIG_LCT_BOOT_REASON
	int lct_sku = 0;
#endif
/* add lct_sku by changjingyang end */ 

	printk("pn544_dev_init\n");

/* add lct_sku by changjingyang start */ 
#ifdef CONFIG_LCT_BOOT_REASON
	lct_sku = lct_get_sku()%5;
	printk("[sku] pn544_dev_init  lct_sku = %d \n", lct_sku);
	if((lct_sku!=1)&&(lct_sku!=2))
	{
		lct_sku = 0;
		printk("pn544_dev_init fail\n");
		return -1;
	}	
#endif

#ifdef CONFIG_MTK_BOARD_ID
	ret=get_bid_gpio();
    //huyunge@wind-mobi.com 20161209 start
    switch(ret){
            case EMEA_DS_NA_EVT:
            case EMEA_SS_NA_EVT:
            case EMEA_DS_NA_DVT:
            case EMEA_SS_NA_DVT:
            case LATAM_DS_NA_EVT:
            case LATAM_DS_NA_DVT:
            case ROLA_SS_NA_EVT:
            case ROLA_SS_NA_DVT:
            case AP_DS_NA_DVT2:
            case EMEA_DS_NA_DVT2:
            case LATAM_DS_NA_SKY77643_DVT2:       //0f
            case ROLA_SS_NA_SKY77643_DVT2:         //10
            case LATAM_DS_NA_SKY77638_DVT2:      //11 
            case ROLA_SS_NA_SKY77638_DVT2:  //12
            case AP_B28_DS_NA_DVT2:
//sunjingtao@wind-mobi.com at 20170119 begin
            case AP_DS_NA_DVT2_1:       //14
            case EMEA_DS_NA_DVT2_1:     //15
            case LATAM_DS_NA_SKY77643_DVT2_1:       //17
            case ROLA_SS_NA_SKY77643_DVT2_1:         //18
            case LATAM_DS_NA_SKY77638_DVT2_1:      //19
            case ROLA_SS_NA_SKY77638_DVT2_1:    //1a
            case AP_B28_DS_NA_DVT2_1:               //1b
//sunjingtao@wind-mobi.com at 20170119 end
//sunjingtao@wind-mobi.com at 20170322 begin
            case AP_B28_DS_SKY77638_DVT2_1:     //1c
            case AP_DS_NA_MP:       //1d
            case EMEA_DS_NA_MP:     //1e
            case LATAM_DS_NA_SKY77643_MP:       //20
            case ROLA_SS_NA_SKY77643_MP:         //21
            case LATAM_DS_NA_SKY77638_MP:      //22
            case ROLA_SS_NA_SKY77638_MP:    //23
            case AP_B28_DS_NA_MP:               //24
            case AP_B28_DS_SKY77638_MP:     //25
//sunjingtao@wind-mobi.com at 20170322 end
                    printk("this board is not support NFC\n");
                return 0;
            case EMEA_SS_NFC_DVT:
            case EMEA_SS_NFC_EVT:
            case EMEA_SS_NFC_DVT2:
//sunjingtao@wind-mobi.com at 20170119 begin
            case EMEA_SS_NFC_DVT2_1:        //16
//sunjingtao@wind-mobi.com at 20170119 end
//sunjingtao@wind-mobi.com at 20170322 begin
            case EMEA_SS_NFC_MP:        //1f
//sunjingtao@wind-mobi.com at 20170322 end
                printk("this board is support NFC\n");
                break;
            default:
                printk("this board is no support NFC\n");
                return 0;
    }
    //huyunge@wind-mobi.com 20161209 end

#endif

/* add lct_sku by changjingyang end */ 
	platform_driver_register(&pn544_platform_driver);

	ret = i2c_add_driver(&pn544_i2c_driver);
	printk("[bert] i2c_add_driver  ret = %d \n", ret);
	printk("pn544_dev_init success\n");

	return 0;
}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	printk("pn544_dev_exit\n");

	i2c_del_driver(&pn544_i2c_driver);
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("XXX");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");

