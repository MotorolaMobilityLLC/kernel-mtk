/*
 * drivers/misc/fs16xx.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <sound/initval.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/sched.h>
//#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>




	/* For MTK platform */
#define USING_MTK_PLATFORM

/* print the log message, should be closed when debugging is finished.. */
//#define FS16XX_DEBUG

/* Check the chip version and test I2C's operation is OK or not */
//#define CHECK_CHIP_VERSION

/**** user customize part for different project begain*****/

#define FS16XX_I2C_NAME   "smartpa_i2c"
#define FS_I2CADDR_1        0x34
#define FS_I2CADDR_2        0x35
#define FS_I2CADDR_3        0X36
#define FS_I2CADDR_4        0x37
    /* old MTK platform using I2C burst */
//#define I2C_USE_DMA
#define MAX_BUFFER_SIZE	           255

/**** user customize part for different project end*****/
//PATH = /sys/devices/f9964000.i2c/i2c-8/8-0034

#define FS16XX_WHOAMI 	           0x03
#define FOURSEMI_REV 			   0x201 // 0x101  //modified by cheguosheng
#define FS16XX_CALIB_MAX_TRY       7

#ifdef USING_MTK_PLATFORM

#include <linux/dma-mapping.h>

#ifdef I2C_USE_DMA
static u8 *I2CDMABuf_va = NULL;
static dma_addr_t I2CDMABuf_pa = NULL;
#endif
#endif

#ifdef FS16XX_DEBUG
#define DEBUG_LOG printk
#else
#define DEBUG_LOG(...)
#endif

static int probe_done = 0;

#define R0_DEFAULT                  8
#define FS16XX_CALIB_OTP_R25_STEP    ((float)(R0_DEFAULT / 2) / 128)

ssize_t fs16xx_read_proc(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static ssize_t  fs16xx_write_proc(struct file *file, const char *buffer, size_t count, loff_t *data);
struct fs16xx_dev
{
	struct mutex		lock;
	struct i2c_client	*client;
	struct miscdevice	fs16xx_device;
	bool deviceInit;
};

static struct fs16xx_dev *fs16xx;

static struct file_operations fs16xx_fops = {
    .read  = fs16xx_read_proc,
    .write = fs16xx_write_proc
};

static int fs16xx_read_debug(u8 regaddr, u8 *data, u8 len)
{
	u8 regbuf = regaddr;
	int ret = 0;
	struct i2c_msg msgs[2] = {{0}, {0} };


	msgs[0].addr = fs16xx->client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &regbuf;

	msgs[1].addr = fs16xx->client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;


	if (!fs16xx->client)
	{
		DEBUG_LOG("client is null!");
		return -EINVAL;
	}

	ret = i2c_transfer(fs16xx->client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (ret != 2) {
		DEBUG_LOG("i2c_transfer error: (%x %x)\n", fs16xx->client->addr, msgs[1].flags);
		return -EIO;
	}

    return 0;

}
ssize_t fs16xx_read_proc(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	char *page = NULL;
    char *ptr = NULL;
    int len, err = -1,ret = -1;
	int i = 0;
	char data[5] = {0};
	unsigned int val = 0;

	/*char reg[23] = {0x00, 0x01, 0x02, 0x03, 0x04,
				    0x05, 0x06, 0x07, 0x08, 0x09,
				    0x0a, 0x0b, 0x0f, 0x47, 0x49,
				    0x62, 0x70, 0x71, 0x72, 0x73,
				    0x80, 0x81, 0x8f};*/
    char reg[0xFF];
	page = kmalloc(PAGE_SIZE, GFP_KERNEL);
	ptr = page;

    DEBUG_LOG("fs16xx dump register on device 0x%x\n",fs16xx->client->addr);
	if (!page)
	{
		kfree(page);
		return -ENOMEM;
	}

	for (i=0; i<0xFF; i++)
	{
		ret = fs16xx_read_debug(reg[i],data,2);
		val = (data[0]<<8)|data[1];
		ptr += sprintf(ptr, "reg[0x%x]=0x%04x", (unsigned int)reg[i], val);
		//if(((i+1)%0x10) == 0) fprintf(stdout, "\n");
	}

	len = ptr - page;
	if(*ppos >= len)
	{
		kfree(page);
		return 0;
	}
	err = copy_to_user(buffer,(char *)page,len);
	*ppos += len;
	if(err)
	{
	    kfree(page);
		return err;
	}
	kfree(page);
	return len;
}

static int fs16xx_write_debug(const struct i2c_client *client, const char *buf, int count)
{
	int ret = 0;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;

	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);

	return (ret == 1) ? count : ret;
}

static ssize_t  fs16xx_write_proc(struct file *file, const char *buffer, size_t count, loff_t *data)
{

    char temp[64] = {'\0'};
    int addr = 0;
    int val = 0;
	char tmp[3] = {0};
	int ret = 0;

    u32 size = (count < (sizeof(temp) - 1)) ? (count) : (sizeof(temp) - 1);
    DEBUG_LOG("write in %s\n", __func__);
    if (copy_from_user(temp, buffer, size))
	{
		DEBUG_LOG("copy_from_user fail\n");
	    return -EFAULT;
	}

	ret = sscanf(temp, "0x%x 0x%x",  &addr, &val);
    if (ret == 2)
    {
		tmp[0] = (char)addr;
		tmp[1] = (char)((val&0xff00)>>8);
		tmp[2] = (char)(val&0xff);
		fs16xx_write_debug(fs16xx->client, tmp, 3);
    }

    return count;
}

int fs16xx_otp_bit_counter(unsigned short val) {
    // Count of bit 0 in val
    int count = 0, i = 0;
    while (i < FS16XX_CALIB_MAX_TRY)
    {
        if((val & (1 << i)) == 0)
        {
            count++;
        }
        i++;
    }
    return count;
}



static ssize_t fs16xx_read_calibration_value(struct device *dev,struct device_attribute *attr, char *buf)
{
   int ret;
   unsigned short cali_val=0;
   unsigned short valueC4=0;
   unsigned short r25=0;

	struct i2c_client *client = to_i2c_client(dev);
    mutex_lock(&fs16xx->lock);
	valueC4 = i2c_smbus_read_word_data(client, 0xC4);
    cali_val |= i2c_smbus_write_word_data(client, 0xC4, 0x000F);

    cali_val |= i2c_smbus_write_word_data(client, 0x0B, 0XCA00);

	cali_val = i2c_smbus_read_word_data(client, 0xE8);
	mutex_unlock(&fs16xx->lock);
	if(cali_val)
	{
	   cali_val = ((cali_val>>8) & 0x00FF) | ((cali_val<<8) & 0xFF00);
	//cali_count = FS16XX_CALIB_MAX_TRY - fs16xx_otp_bit_counter((value >> 8) & 0xFF);
       cali_val = cali_val&0xFF;
	}
	if((cali_val & 0x80) != 0) {
        // minus value
        r25 = R0_DEFAULT - FS16XX_CALIB_OTP_R25_STEP * (cali_val & 0x7F);
    }else{
        r25 = R0_DEFAULT + FS16XX_CALIB_OTP_R25_STEP * cali_val;
    }

     ret = i2c_smbus_write_word_data(client, 0xC4, valueC4);

    return (ret == 0) ? scnprintf(buf, PAGE_SIZE, "%u\n", cali_val) : ret;


}


static ssize_t fs16xx_force_calibration(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	//FIXME
  return 0;

}


static ssize_t fs16xx_read_register(struct device *dev,struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int reg=0;
    int value=0;
    int ret=0;
    //char str[]={"bad parameter"};

    DEBUG_LOG("%s: enter\n",__func__);

    ret = sscanf(buf,"%x",&reg);
    if(ret == 1 )
        value = i2c_smbus_read_word_data(client,reg);

    DEBUG_LOG("%s: register 0x%x value is 0x%x\n",__func__,reg,value);

    return scnprintf(buf, PAGE_SIZE, "%x\n", value);
}


static ssize_t fs16xx_calibration_done(struct device *dev, struct device_attribute *attr, char *buf)

{
  struct i2c_client *client = to_i2c_client(dev);
  int calib_value=0;

  DEBUG_LOG("%s: enter\n",__func__);

  calib_value = i2c_smbus_read_word_data(client, 0x80);
  if(calib_value)
  {
  	 DEBUG_LOG("%s: Calibration completed\n",__func__);
     return scnprintf(buf, PAGE_SIZE, "%d\n", calib_value);
  }else{
     DEBUG_LOG("%s: Calibration not completed\n",__func__);
	 return scnprintf(buf, PAGE_SIZE, "%d\n", calib_value);
  }

}

DEVICE_ATTR(fs16xx_calib_show, 0664, fs16xx_read_calibration_value, NULL);
DEVICE_ATTR(fs16xx_force_calib, 0664, NULL, fs16xx_force_calibration);
DEVICE_ATTR(fs16xx_read_reg, 0664, fs16xx_read_register, NULL);
DEVICE_ATTR(fs16xx_cali_done, 0664, fs16xx_calibration_done, NULL);


static struct attribute *fs16xx_attrs[] =
{
    &dev_attr_fs16xx_calib_show.attr,
    &dev_attr_fs16xx_force_calib.attr,
    &dev_attr_fs16xx_read_reg.attr,
    &dev_attr_fs16xx_cali_done.attr,
    NULL
};

static const struct attribute_group fs16xx_attr_group ={
  .attrs = fs16xx_attrs,
};

static int fs16xx_i2c_dma_read_write(struct i2c_client *client, const uint8_t *buf, int len, bool rw)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

#ifdef I2C_USE_DMA
	 msg.flags = 0;
	 msg.len = len;
	 msg.buf = (char *)buf;

     if(rw)
     {
	    msg.timing = 400;
		msg.flags |= I2C_M_RD;
     }
	 else
	 {
        //msg.timing = I2C_MASTER_CLOCK;
	 }

	if(len < 8)
	{
		msg.addr = ((client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
		msg.ext_flag = client->ext_flag;
	}
	else
	{
		msg.addr = (client->addr & I2C_MASK_FLAG);
		msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
	}
#endif

    DEBUG_LOG("%s: i2c_transfer\n",__func__);

	ret = i2c_transfer(adap, &msg, 1);

	return (ret == 1) ? len : ret;
}


static int fs16xx_distin_dma_normal(struct i2c_client *client, uint8_t *buf, size_t count, bool dma, bool rw)
{
     int ret;
     //int i;

	 if(dma)
	 {
	 	DEBUG_LOG("%s: DMA used\n",__func__);
		 mutex_lock(&fs16xx->lock);
		 if(count <= 8)
		 {
		 	if(rw)
		 	{
				 ret = fs16xx_i2c_dma_read_write(client, buf, count, true);
		 	}else{
                 ret = fs16xx_i2c_dma_read_write(client, buf, count, false);
		 	}
		 }else{
		    if(rw)
			{
				 ret = fs16xx_i2c_dma_read_write(client, buf, count, true);
		 	}else{
                 ret = fs16xx_i2c_dma_read_write(client, buf, count, false);
		 	}
		 }
		 mutex_unlock(&fs16xx->lock);
		 if (ret < 0)
		 {
		 	if(rw)
			    DEBUG_LOG("%s: read error return,error code = %d\n", __func__, ret);
			else
			    DEBUG_LOG("%s: write error return,error code = %d\n", __func__, ret);
		 }
	 }
	 else
	 {
	 	   DEBUG_LOG("%s: DMA not used\n",__func__);
	 	if(rw)
	 	{
           mutex_lock(&fs16xx->lock);
	       ret = i2c_master_recv(client, buf, count);
	       mutex_unlock(&fs16xx->lock);

           DEBUG_LOG("%s: in i2c_master_recv\n",__func__);

	 	}else{
	 	   mutex_lock(&fs16xx->lock);
	       ret = i2c_master_send(client, buf, count);
	       mutex_unlock(&fs16xx->lock);

		   DEBUG_LOG("%s: in i2c_master_send\n",__func__);
	    }
		   if(ret<0)
		   {
		   	  if(rw)
                  DEBUG_LOG("%s: read error return,error code = %d\n", __func__, ret);
			  else
			  	  DEBUG_LOG("%s: write error return,error code = %d\n", __func__, ret);
		   }

	 }
	 return ret;
}
static ssize_t fs16xx_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{

	char *tmp_data;
	int ret;

        #ifdef I2C_USE_DMA
        int i;
        #endif

	if (count > 8192)
		 count = 8192;

	tmp_data = kmalloc(count, GFP_KERNEL);
	if (tmp_data == NULL)
	   return -ENOMEM;

	DEBUG_LOG("%s : reading %zu bytes.\n", __func__, count);

#ifdef I2C_USE_DMA
      ret = fs16xx_distin_dma_normal(fs16xx->client, I2CDMABuf_va, count, true, true);
      for(i = 0; i < count; i++)
		{
			tmp_data[i] = I2CDMABuf_va[i];
		}
#else
      ret = fs16xx_distin_dma_normal(fs16xx->client, tmp_data, count, false, true);
#endif
	if ((ret > count)||(ret < 0))
	{
		if(ret>count)
		   DEBUG_LOG("%s: received too many bytes from device (number read bytes = %d)\n", __func__, ret);
		else
		   DEBUG_LOG("%s: read write error ret=%d\n", __func__, ret);
		   return -EIO;
	}
	if (copy_to_user(buf, tmp_data, ret))
	{
		DEBUG_LOG("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

	DEBUG_LOG("%s:read %d bytes sucessfully\n",__func__,ret);

	kfree(tmp_data);
	tmp_data = NULL;
	return ret;
}


static ssize_t fs16xx_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	struct fs16xx_dev  *fs16xx_dev;
	char *tmp_data;
	int ret;

	tmp_data = kmalloc(count,GFP_KERNEL);
	fs16xx_dev = filp->private_data;

	DEBUG_LOG("%s : writing %zu bytes.\n", __func__, count);

#ifdef I2C_USE_DMA

	if (copy_from_user(I2CDMABuf_va, buf, count))
	{
		DEBUG_LOG("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

    ret = fs16xx_distin_dma_normal(fs16xx->client, I2CDMABuf_va, count, true, false);

#else

	if (copy_from_user(tmp_data, buf, count))
	{
		DEBUG_LOG("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	ret = fs16xx_distin_dma_normal(fs16xx->client, tmp_data, count, false, false);

#endif

	if (ret < 0)
	{
		DEBUG_LOG("%s : write error return,error code= %d\n", __func__, ret);
		return ret;
	}

    DEBUG_LOG("%s: writing %d bytes sucessfully\n",__func__,ret);

    kfree(tmp_data);
    tmp_data = NULL;
	return ret;
}

static int fs16xx_dev_open(struct inode *inode, struct file *filp)
{

	struct fs16xx_dev *fs16xx_dev = container_of(filp->private_data, struct fs16xx_dev, fs16xx_device);

	if(fs16xx_dev->deviceInit == true)
		DEBUG_LOG("%s: fs16xx device has initialized\n",__func__);
	else
		DEBUG_LOG("%s: fs16xx device initialize not finished\n",__func__);

	   filp->private_data = fs16xx_dev;

	    DEBUG_LOG("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}



static long fs16xx_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	switch (cmd)
	{
		/* This part do not work ok now , it is used for stereo application. */
		case I2C_SLAVE:
		case I2C_SLAVE_FORCE:
			if((arg == FS_I2CADDR_1) ||(arg == FS_I2CADDR_2) ||(arg == FS_I2CADDR_3) ||(arg == FS_I2CADDR_4))
			{
				//change addr
				DEBUG_LOG("%s: current i2c address is 0x%x\n",__func__, (unsigned int)arg);
			    fs16xx->client->addr = arg;
			    return 0;
			}
			else
			{
				DEBUG_LOG("%s :wrong i2c address\n",__func__);
				return -1;
			}

		default:
		    	DEBUG_LOG("%s: bad ioctl parameters %u\n", __func__, cmd);
			    return -EINVAL;
	}
	      return 0;
}

static const struct file_operations fs16xx_dev_fops =
{
	.owner	= THIS_MODULE,
	.open	= fs16xx_dev_open,
	.unlocked_ioctl = fs16xx_dev_ioctl,
	.compat_ioctl = fs16xx_dev_ioctl,
	.llseek	= no_llseek,
	.read	= fs16xx_dev_read,
	.write	= fs16xx_dev_write,
};


static struct fs16xx_dev *fs16xx_dev_alloc_object(void)
{
    struct fs16xx_dev *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

    printk("fs16xx_dev_alloc_object start\n");

    if (!obj)
    {
        printk("fs16xx_dev_alloc_object:Alloc ges object error!\n");
        return NULL;
    }

    printk("fs16xx_dev_alloc_object end\n");

    return obj;
}

static int fs16xx_i2c_probe(struct i2c_client *client,
				      const struct i2c_device_id *id)
{
	int ret = 0;
#ifdef CHECK_CHIP_VERSION

	int read_id = 0;
	int rev_value = 0;
#endif
	DEBUG_LOG("%s: enter\n", __func__);

	if(probe_done == 1){
		DEBUG_LOG("%s has done\n", __func__);
		return 0;
		}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		DEBUG_LOG("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	//fs16xx = kzalloc(sizeof(*fs16xx), GFP_KERNEL);
	fs16xx = fs16xx_dev_alloc_object();
	DEBUG_LOG("%s: step 1\n", __func__);
	if (fs16xx == NULL)
	{
		DEBUG_LOG("%s : failed to allocate memory for module data\n", __func__);
		ret = -ENOMEM;
		goto err_exit;
	}

	i2c_set_clientdata(client, fs16xx);
	fs16xx->client   = client;

	/* init mutex and queues */
	mutex_init(&fs16xx->lock);

#ifdef I2C_USE_DMA

	I2CDMABuf_va = (u8 *)dma_alloc_coherent(&(client->dev), 4096, &I2CDMABuf_pa, GFP_KERNEL);
    	if(!I2CDMABuf_va)
    	{
			DEBUG_LOG("%s: Allocate fs16xx DMA I2C Buffer failed!\n", __func__);
			return -1;
    	}
#endif
    DEBUG_LOG("%s: step 2\n", __func__);

#ifdef CHECK_CHIP_VERSION
	mutex_lock(&fs16xx->lock);
	read_id = i2c_smbus_read_word_data(client, FS16XX_WHOAMI);
	rev_value = ((read_id & 0x00FF)<< 8) | ((read_id & 0xFF00)>> 8);
	mutex_unlock(&fs16xx->lock);

	DEBUG_LOG("%s:ID = 0x%x\n", __func__,rev_value);

	if(rev_value == FOURSEMI_REV)
	{
 		DEBUG_LOG("%s: FourSemi Chip fs16xx found !\n",__func__);
	}
	else
	{
		DEBUG_LOG("%s: can not find foursemi Device,id 0x%x != 0x0101\n",__func__, rev_value);
		ret = -1;
		goto i2c_error;
	}
#endif

        DEBUG_LOG("%s:step 3\n", __func__);

	fs16xx->fs16xx_device.minor = MISC_DYNAMIC_MINOR;
	fs16xx->fs16xx_device.name = FS16XX_I2C_NAME;
	fs16xx->fs16xx_device.fops = &fs16xx_dev_fops;

        DEBUG_LOG("%s: step 4\n", __func__);

	ret = misc_register(&fs16xx->fs16xx_device);
	if (ret)
	{
		DEBUG_LOG("%s: misc_register failed\n",__func__);
		ret = -1;
		goto err_misc_register;
	}

        DEBUG_LOG("%s: step 5\n", __func__);

	/*register sysfs hooks*/
	//ret = sysfs_create_group(&client->dev.kobj, &fs16xx_attr_group);
        ret = sysfs_create_group(&fs16xx->fs16xx_device.this_device->kobj,&fs16xx_attr_group);
        if (ret)
		DEBUG_LOG("%s: Error registering fs16xx sysfs\n", __func__);

        DEBUG_LOG("%s: step 6\n", __func__);

	//add debug node
	proc_create("fs16xx", 0664, NULL, &fs16xx_fops);
	if (fs16xx) {
		mutex_lock(&fs16xx->lock);
		fs16xx->deviceInit = true;
		mutex_unlock(&fs16xx->lock);
	}

	probe_done = 1;

	DEBUG_LOG("%s: end sucessfully!\n",__func__);
	return 0;

err_misc_register:
	misc_deregister(&fs16xx->fs16xx_device);
#ifdef CHECK_CHIP_VERSION
i2c_error:
	mutex_destroy(&fs16xx->lock);
	kfree(fs16xx);
	fs16xx = NULL;
#endif
err_exit:
	return ret;
}


static int fs16xx_i2c_remove(struct i2c_client *client)
{
	struct fs16xx_dev *fs16xx_dev;

	DEBUG_LOG("%s: Enter.\n", __func__);
#ifdef I2C_USE_DMA
    if(I2CDMABuf_va)
    {
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMABuf_va, I2CDMABuf_pa);
		I2CDMABuf_va = NULL;
		I2CDMABuf_pa = NULL;
    }
#endif
	fs16xx_dev = i2c_get_clientdata(client);
	misc_deregister(&fs16xx_dev->fs16xx_device);
	mutex_destroy(&fs16xx_dev->lock);
	kfree(fs16xx_dev);

	return 0;
}

static const struct of_device_id fs16xx_of_match[] = {
	{ .compatible = "fs16xx,smartpa_i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, fs16xx_of_match);

static const struct i2c_device_id fs16xx_i2c_id[] = {
	{ FS16XX_I2C_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fs16xx_i2c_id);

static struct i2c_driver fs16xx_i2c_driver = {
	.driver = {
		.name = FS16XX_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = fs16xx_of_match,
	},
	.probe =    fs16xx_i2c_probe,
	.remove =   fs16xx_i2c_remove,
	.id_table = fs16xx_i2c_id,
};

static int __init fs16xx_modinit(void)
{
	int ret = 0;

	DEBUG_LOG("%s: Loading fs16xx driver\n",__func__);

	ret = i2c_add_driver(&fs16xx_i2c_driver);
	if (ret != 0) {
	DEBUG_LOG("%s: Failed to register fs16xx I2C driver: %d\n", __func__,ret);
	}
	return ret;
}
module_init(fs16xx_modinit);
//module_initcall(fs16xx_modinit);//some 7.0 platform need use

static void __exit fs16xx_exit(void)
{
	i2c_del_driver(&fs16xx_i2c_driver);
}
module_exit(fs16xx_exit);

MODULE_AUTHOR("yingshun.jiang@foursemi.com>");
MODULE_DESCRIPTION("FS16XX SmartPA I2C driver");
MODULE_LICENSE("GPL");

