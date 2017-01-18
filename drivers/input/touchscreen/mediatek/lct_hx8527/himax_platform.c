/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2014 Himax Corporation.
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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/stringify.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/device.h>
//#include <linux/rtpm_prio.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/dma-mapping.h>

#include <linux/input/mt.h>
#include "tpd.h"
//#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <linux/suspend.h>
#include "himax_platform.h"
#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#define D(x...) printk("[HXTP] " x)
#define I(x...) printk("[HXTP] " x)
#define W(x...) printk("[HXTP][WARNING] " x)
#define E(x...) printk("[HXTP][ERROR] " x)
#endif

int irq_enable_count = 0;

DEFINE_MUTEX(hx_wr_access);

#ifdef MTK
u8 *gpDMABuf_va = NULL;
u8 *gpDMABuf_pa = NULL;
#ifdef MTK_KERNEL_318
static int himax_tpd_int_gpio = 5;
extern int himax_tpd_rst_gpio_number;
extern int himax_tpd_int_gpio_number;
#endif
uint8_t himax_int_gpio_read(int pinnum)
{
#ifdef MTK_KERNEL_318
	return  gpio_get_value(himax_tpd_int_gpio);
#endif
}

#ifdef MTK_KERNEL_318
int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int ret=0;
	s32 retry = 0;
	u8 buffer[1];

	struct i2c_msg msg[] =
	{
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.flags = 0,
			.buf = buffer,
			.len = 1,
			.timing = 400
		},
		{
			.addr = (client->addr & I2C_MASK_FLAG),
			.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			.flags = I2C_M_RD,
			.buf = gpDMABuf_pa,
			.len = length,
			.timing = 400
		},
	};
	mutex_lock(&hx_wr_access);
	buffer[0] = command;

	if (data == NULL){
		mutex_unlock(&hx_wr_access);
		return -1;
	}
	for (retry = 0; retry < toRetry; ++retry)
	{
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
		{
			continue;
		}
		memcpy(data, gpDMABuf_va, length);
		mutex_unlock(&hx_wr_access);
		return 0;
	}
	printk("Dma I2C Read Error: %d byte(s), err-code: %d", length, ret);//E
	mutex_unlock(&hx_wr_access);
	return ret;
}

int i2c_himax_write(struct i2c_client *client, uint8_t command, uint8_t *buf, uint8_t len, uint8_t toRetry)
{
	int rc=0,retry=0;
	u8 *pWriteData = gpDMABuf_va;

	struct i2c_msg msg[]={
		{
		.addr = (client->addr & I2C_MASK_FLAG),
		.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
		.flags = 0,
		.buf = gpDMABuf_pa,
		.len = len+1,
		.timing = 400
		},
	};
	mutex_lock(&hx_wr_access);
	if(!pWriteData)
	{
		printk("dma_alloc_coherent failed!\n");//E
		mutex_unlock(&hx_wr_access);
		return -1;
	}

	gpDMABuf_va[0] = command;

	memcpy(gpDMABuf_va+1, buf, len);

	for (retry = 0; retry < toRetry; ++retry)
	{
		rc = i2c_transfer(client->adapter, &msg[0], 1);
		if (rc < 0)
		{
			continue;
		}
		mutex_unlock(&hx_wr_access);
		return 0;
	}

	printk("Dma I2C master write Error: %d byte(s), err-code: %d", len, rc);//
	mutex_unlock(&hx_wr_access);
	return rc;
}

int i2c_himax_read_command(struct i2c_client *client, uint8_t len, uint8_t *buf, uint8_t *readlength, uint8_t toRetry)
{
	return 0;
}

int i2c_himax_write_command(struct i2c_client *client, uint8_t command, uint8_t toRetry)
{
	return i2c_himax_write(client, command, NULL, 0, toRetry);
}

int i2c_himax_master_write(struct i2c_client *client, uint8_t *buf, uint8_t len, uint8_t toRetry)
{
	int rc=0, retry=0;
	u8 *pWriteData = gpDMABuf_va;

	struct i2c_msg msg[] ={
		{
		.addr = (client->addr & I2C_MASK_FLAG),
		.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
		.flags = 0,
		.buf = gpDMABuf_pa,
		.len = len,
		.timing = 400
		},
	};

	mutex_lock(&hx_wr_access);
	if(!pWriteData)
	{
		printk("dma_alloc_coherent failed!\n");//E
		mutex_unlock(&hx_wr_access);
		return -1;
	}

	memcpy(gpDMABuf_va, buf, len);
	for (retry = 0; retry < toRetry; ++retry)
	{
		rc = i2c_transfer(client->adapter, &msg[0], 1);
		if (rc < 0)
		{
			continue;
		}
		mutex_unlock(&hx_wr_access);
		return 0;
	}
	printk("Dma I2C master write Error: %d byte(s), err-code: %d", len, rc);//E
	mutex_unlock(&hx_wr_access);
	return rc;
}
#endif

#if 0 //MTK_KERNEL_318
int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	#ifdef I2C_USE_AUTO_DMA 
	int retry;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		msleep(10);
	}
	if (retry == toRetry) {
		printk("%s: i2c_read_block retry over %d\n",
			__func__, toRetry);
		return -EIO;
	}
	#endif
	char buf[2] = {command,0};	
    i2c_himax_read_1(client,buf,1,data,length);
	return 0;	

}
#define __MSG_DMA_MODE__
#ifdef  __MSG_DMA_MODE__
	u8 *g_dma_buff_va = NULL;
	dma_addr_t g_dma_buff_pa = 0;

    u8 *tpd_i2c_dma_va = NULL;
    dma_addr_t tpd_i2c_dma_pa = 0;
#endif

#ifdef __MSG_DMA_MODE__

	void msg_dma_alloct(void)
	{
	    if (NULL == g_dma_buff_va)
    		{
       		 tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
       		 g_dma_buff_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 128, &g_dma_buff_pa, GFP_KERNEL);
    		}

	    	if(!g_dma_buff_va)
		    {
	          TPD_DMESG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	    	}

	    if (NULL == tpd_i2c_dma_va)
          {
          tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
          tpd_i2c_dma_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 250, &tpd_i2c_dma_pa, GFP_KERNEL);
          }
        if (!tpd_i2c_dma_va)
		TPD_DMESG("TPD dma_alloc_coherent error!\n");
	}
#endif
int i2c_himax_read_1(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
  int ret=0;

	// for DMA I2c transfer

	//mutex_lock(&i2c_rw_access);

	if((NULL!=client) && (writelen>0) && (writelen<=128))
	{
		// DMA Write
		memcpy(g_dma_buff_va, writebuf, writelen);
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
		if((ret=i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
		//TPD_DMESG("###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,g_dma_buff_pa);	
		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	// DMA Read

	if((NULL!=client) && (readlen>0) && (readlen<=128))
	{
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;

		ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

		memcpy(readbuf, g_dma_buff_va, readlen);

		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	//mutex_unlock(&i2c_rw_access);

	return ret;

}
int i2c_himax_write_1(struct i2c_client *client, uint8_t *writebuf, uint8_t writelen)
{
	int ret = 0;
	if (writelen <= 8) {
	    client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		ret=i2c_master_send(client, writebuf, writelen);
	}
	else if((writelen > 8)&&(NULL != tpd_i2c_dma_va))
	{
        memcpy(tpd_i2c_dma_va,writebuf,writelen);
		client->addr = (client->addr & I2C_MASK_FLAG )| I2C_DMA_FLAG;
	    ret = i2c_master_send(client, (unsigned char *)tpd_i2c_dma_pa, writelen);
	    client->addr = client->addr & I2C_MASK_FLAG;	
	}
	return ret;

}
int i2c_himax_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	#ifdef I2C_USE_AUTO_DMA
	int retry/*, loop_i*/;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = command;
	memcpy(buf+1, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}

	if (retry == toRetry) {
		printk("%s: i2c_write_block retry over %d\n",
			__func__, toRetry);
		return -EIO;
	}
	#endif
	uint8_t writebuf[length+1];
	memset(writebuf,0,length+1);
    memcpy(writebuf+1,data,length);
    writebuf[0]=command;
	i2c_himax_write_1(client, writebuf,length+1);
	return 0;

}

int i2c_himax_read_command(struct i2c_client *client, uint8_t length, uint8_t *data, uint8_t *readlength, uint8_t toRetry)
{
	int retry;
	struct i2c_msg msg[] = {
		{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = length,
		.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}
	if (retry == toRetry) {
		printk("%s: i2c_read_block retry over %d\n",
		       __func__, toRetry);
		return -EIO;
	}
	return 0;
}

int i2c_himax_write_command(struct i2c_client *client, uint8_t command, uint8_t toRetry)
{
	return i2c_himax_write(client, command, NULL, 0, toRetry);
}

int i2c_himax_master_write(struct i2c_client *client, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	#ifdef I2C_USE_AUTO_DMA
	int retry/*, loop_i*/;
	uint8_t buf[length];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = buf,
		}
	};

	memcpy(buf, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(10);
	}

	if (retry == toRetry) {
		printk("%s: i2c_write_block retry over %d\n",
		       __func__, toRetry);
		return -EIO;
	}
	#endif
	int ret=0;
	if (length <= 8) {
	    client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		ret=i2c_master_send(client, data, length);
	}
	else if((length > 8)&&(NULL != tpd_i2c_dma_va))
	{
		//for (i = 0; i < length; i++)
			//tpd_i2c_dma_va[i] = data[i];
        memcpy(tpd_i2c_dma_va,data,length);
		client->addr = (client->addr & I2C_MASK_FLAG )| I2C_DMA_FLAG;
	    ret = i2c_master_send(client, (unsigned char *)tpd_i2c_dma_pa, length);
	    client->addr = client->addr & I2C_MASK_FLAG;		
	}
	return ret;
}

#endif
void himax_int_enable(int irqnum, int enable, int log_print)
{
	if (enable == 1 && irq_enable_count == 0) {
		irq_enable_count=1;
#ifdef CONFIG_OF_TOUCH
		enable_irq(irqnum);
#else
		mt_eint_unmask(irqnum);
#endif
	} else if (enable == 0 && irq_enable_count == 1) {
		irq_enable_count=0;
#ifdef CONFIG_OF_TOUCH
		disable_irq_nosync(irqnum);
#else
		mt_eint_mask(irqnum);
#endif
	}
if(log_print)
	printk("irq_enable_count = %d\n", irq_enable_count);
}

void himax_rst_gpio_set(int pinnum, uint8_t value)
{
#ifdef MTK_KERNEL_318
	if(value)
		tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
	else
		tpd_gpio_output(himax_tpd_rst_gpio_number, 0);
#else
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	if(value)
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
#endif	
}

#ifdef MTK_KERNEL_318
int himax_gpio_power_config(struct i2c_client *client )
{
	int error=0;

	error = regulator_enable(tpd->reg);
	if (error != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", error);
	msleep(50);//100
	
    tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
	msleep(30);
	tpd_gpio_output(himax_tpd_rst_gpio_number, 0);
	msleep(30);
	tpd_gpio_output(himax_tpd_rst_gpio_number, 1);
	msleep(50);//120

	TPD_DMESG("mtk_tpd: himax reset over \n");
	
	/* set INT mode */

	tpd_gpio_as_int(himax_tpd_int_gpio_number);
	return 0;
}
#endif
#endif
