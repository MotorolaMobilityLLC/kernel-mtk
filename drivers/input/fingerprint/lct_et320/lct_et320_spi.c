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

/******************************************************************************\
|*                                                                            *|
|*  et320.spi.c                                                               *|
|*  Date: 2016/02/25                                                          *|
|*  Version: 0.9.0.0                                                          *|
|*  Revise Date:  2016/02/25                                                  *|                                                           
|*  Copyright (C) 2007-2016 Egis Technology Inc.                              *|
|*                                                                            *|
\******************************************************************************/


#include <linux/interrupt.h>
/* Lct add by zl */
#include <linux/proc_fs.h>// for the proc filesystem
#include <linux/seq_file.h>// for sequence files
#include <linux/jiffies.h>// for jiffies
#include "lct_print.h"
/* Lct add end */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif  
//prize-lixuefeng-20150512-start
#if defined(CONFIG_PRIZE_HARDWARE_INFO)
#include "../../../hardware_info/hardware_info.h"
extern struct hardware_info current_fingerprint_info;
#endif
//prize-lixuefeng-20150512-end

#include "lct_et320_spi.h"
///////////////////////
#ifndef SPI_PACKET_SIZE
#define SPI_PACKET_SIZE 	 0x400
#define SPI_PACKET_COUNT    0x100
#define SPI_FIFO_SIZE 32
#endif
////////////////////////

/* Lct add by zl */
#ifdef LCT_LOG_MOD
#undef LCT_LOG_MOD
#define LCT_LOG_MOD "[FPS]" 
#endif
/* Lct add end */


#ifdef FP_SPI_DEBUG
#define DEBUG_PRINT(fmt, args...)	pr_err(fmt, ## args)
#else
/* Don't do anything in release builds */
#define DEBUG_PRINT(fmt, args...)
#endif

#define LGE_TEST 1
/* MTK mt6975_evb reset & interrupt pin */
//#define GPIO_FPS_RESET_PIN	 11//(GPIO90 | 0x80000000)
//#define CUST_EINT_FPS_EINT_NUM	CUST_EINT_FP_NUM	//5
//#define TRIGGER_FPS_GPIO_PIN	GPIO_FP_EINT_PIN	//(GPIO5 | 0x80000000)

//#define P_GPIO_FPS_PWR_PIN  122

#define FPC_BTP_SPI_CLOCK_SPEED			11*1000*1000
#define MASS_READ_TRANSMITION_SIZE 2304

#define EGIS_MASS_READ_SEGMENT_COUNT 10

unsigned int fp_detect_irq = 0;
#ifdef CONFIG_OF
static const struct of_device_id fp_of_match[] = {
	{ .compatible = "mediatek,fingerprint",},
	{},
};
#endif

#define EDGE_TRIGGER_FALLING    0x0
#define	EDGE_TRIGGER_RISING    0x1
#define	LEVEL_TRIGGER_LOW       0x2
#define	LEVEL_TRIGGER_HIGH      0x3

#define __WORDSIZE (__SIZEOF_LONG__ * 8)// add by zhaofei - 2016-11-16-09-14
/* Lct add sys node by zl*/
//#define LCT_FPS_ET_SYS_NODE_SUP
#ifdef LCT_FPS_ET_SYS_NODE_SUP
int fpc_create_device_node(void);
static ssize_t et320_get_ID(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t et320_set_ID(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static DEVICE_ATTR(et320_id, 0660, et320_get_ID, et320_set_ID);
#endif

static struct proc_dir_entry* fp_proc;
static int fp_device_show(struct seq_file *m, void *v);

/* Lct add end*/


static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static unsigned bufsiz = 4096;
struct device *esfp0_dev;
/*-----------------MTK spi configure-----------------*/
static struct mt_chip_conf spi_conf =
{
	.setuptime = 1,
	.holdtime = 1,
	.high_time = 5,
	.low_time = 5,
	.cs_idletime = 2,
	.ulthgh_thrsh = 0,
	.cpol = 1,
	.cpha = 1,
	.rx_mlsb = SPI_MSB, 
	.tx_mlsb = SPI_MSB,
	.tx_endian = 0,
	.rx_endian = 0,
#ifdef TRANSFER_MODE_DMA
	.com_mod = DMA_TRANSFER,
#else
	.com_mod = FIFO_TRANSFER,
#endif
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

/*
static struct mt_chip_conf spi_conf_fifo =
{
	.setuptime = 1,
	.holdtime = 1,
	.high_time = 4,
	.low_time = 4,
	.cs_idletime = 2,
	.ulthgh_thrsh = 0,
	.cpol = 1,
	.cpha = 1,
	.rx_mlsb = SPI_MSB, 
	.tx_mlsb = SPI_MSB,
	.tx_endian = 0,
	.rx_endian = 0,
	.com_mod = FIFO_TRANSFER,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};
*/
static struct mt_chip_conf spi_conf_dma =
{
	.setuptime = 1,
	.holdtime = 1,
	.high_time = 5,
	.low_time = 5,
	.cs_idletime = 2,
	.ulthgh_thrsh = 0,
	.cpol = 1,
	.cpha = 1,
	.rx_mlsb = SPI_MSB, 
	.tx_mlsb = SPI_MSB,
	.tx_endian = 0,
	.rx_endian = 0,
	.com_mod = DMA_TRANSFER,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

static struct spi_device *spidev;

#define FPC_DEVICE "fpc"

enum fpc_gpio_type {
	GPIO_DEFAULT = 0,
	GPIO_EINT,
	GPIO_EINT_HIGH,
	GPIO_EINT_LOW,	
	GPIO_RST_HIGH,
	GPIO_RST_LOW,
	GPIO_PWR_HIGH,
	GPIO_PWR_LOW,
	GPIO_SPI_MODE0,
	GPIO_SPI_MODE1,
	GPIO_NUM
};

struct pinctrl *pinctrlfpc;

struct fpc_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

/*
"default", "fpc_eint_as_int", "fpc_eint_high", "fpc_eint_low", "fpc_rst_pullhigh", 
"fpc_rst_pulllow", "fpc_pwr_pullhigh", "fpc_pwr_pulllow", "fpc_spi_mode0", "fpc_spi_mode1";
*/
static struct fpc_gpio_attr fpc_gpios[GPIO_NUM] = {
	[GPIO_DEFAULT] = {"default", false, NULL},
	[GPIO_EINT] = {"fpc_eint_as_int", false, NULL},
	[GPIO_EINT_HIGH] = {"fpc_eint_high", false, NULL},
	[GPIO_EINT_LOW] = {"fpc_eint_low", false, NULL},
	[GPIO_RST_HIGH] = {"fpc_rst_pullhigh", false, NULL},
	[GPIO_RST_LOW] = {"fpc_rst_pulllow", false, NULL},
	[GPIO_PWR_HIGH] = {"fpc_pwr_pullhigh", false, NULL},
	[GPIO_PWR_LOW] = {"fpc_pwr_pulllow", false, NULL},
	[GPIO_SPI_MODE0] = {"fpc_spi_mode0", false, NULL},
	[GPIO_SPI_MODE1] = {"fpc_spi_mode1", false, NULL},
};

void fpc_get_dts_info(struct platform_device *pdev)
{
	int ret;
	int i = 0;
#if 0
	struct platform_device *pdev_t;
	struct device_node *node;
#if 1
	node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
	if(node){
		pdev_t = of_find_device_by_node(node);
		if(!pdev_t){
			printk("%s platform device is null!",__func__);
		}
	}else{
		printk("%s dts node is null!",__func__);
	}
#else
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-usb20");
	if(node){
		pdev_t = of_find_device_by_node(node);
		if(!pdev_t){
			printk("%s platform device is null!",__func__);
		}
	}else{
		printk("%s dts node is null!",__func__);
	}
#endif
#endif
	printk("%s\n", __func__);

	pinctrlfpc = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrlfpc)) {
		ret = PTR_ERR(pinctrlfpc);
		printk("Cannot find pinctrlaud!\n");
		return;
	}
#if 0
#if 1
	fpc_gpios[GPIO_RST_LOW].gpioctrl = pinctrl_lookup_state(pinctrlfpc,"fpc_rst_pulllow");
	if (IS_ERR(fpc_gpios[GPIO_RST_LOW].gpioctrl)) {
		ret = PTR_ERR(fpc_gpios[GPIO_RST_LOW].gpioctrl);
		printk("%s can't find fingerprint pinctrl irq line=%d\n", __func__,__LINE__);
	}
#else
	fpc_gpios[GPIO_RST_LOW].gpioctrl = pinctrl_lookup_state(pinctrlfpc,"drvvbus_low");
	if (IS_ERR(fpc_gpios[GPIO_RST_LOW].gpioctrl)) {
		ret = PTR_ERR(fpc_gpios[GPIO_RST_LOW].gpioctrl);
		printk("%s can't find drvvbus_low irq line=%d\n", __func__,__LINE__);
	}

#endif
#else
	for (i = 1; i < ARRAY_SIZE(fpc_gpios); i++) {
		fpc_gpios[i].gpioctrl = pinctrl_lookup_state(pinctrlfpc, fpc_gpios[i].name);
		if (IS_ERR(fpc_gpios[i].gpioctrl)) {
			ret = PTR_ERR(fpc_gpios[i].gpioctrl);
			printk("%s pinctrl_lookup_state %s fail %d\n", __func__, fpc_gpios[i].name,
			       ret);
		} else {
			fpc_gpios[i].gpio_prepare = true;
		}
	}
#endif
}

int fpc_gpio_spi_select(int bEnable)
{
	int retval = 0;
	
	printk("%s bEnable:%d\n", __func__, bEnable);
	if (bEnable == 1) {
		if (fpc_gpios[GPIO_SPI_MODE1].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_SPI_MODE1].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPI_MODE1] pins\n");
		}
	} else {
		if (fpc_gpios[GPIO_SPI_MODE0].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_SPI_MODE0].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_SPI_MODE0] pins\n");
		}

	}

	return retval;
}

int fpc_gpio_rst_select(int bEnable)
{
	int retval = 0;
	printk("%s bEnable:%d\n", __func__, bEnable);
	if (bEnable == 1) {
		if (fpc_gpios[GPIO_RST_HIGH].gpio_prepare) {
				retval = pinctrl_select_state(pinctrlfpc,
						fpc_gpios[GPIO_RST_HIGH].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_RST_HIGH] pins\n");
		}
	} else {
		if (fpc_gpios[GPIO_RST_LOW].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_RST_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_RST_LOW] pins\n");
		}

	}
	return retval;
}

int fpc_gpio_pwr_select(int bEnable)
{
	int retval = 0;
//	pr_err("could not set aud_gpios[GPIO_PWR_HIGH] pins\n");
	printk("%s bEnable:%d\n", __func__, bEnable);
	if (bEnable == 1) {
		if (fpc_gpios[GPIO_PWR_HIGH].gpio_prepare) {
				retval = pinctrl_select_state(pinctrlfpc,
						fpc_gpios[GPIO_PWR_HIGH].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_PWR_HIGH] pins\n");
		}
	} else {
		if (fpc_gpios[GPIO_PWR_LOW].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_PWR_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_PWR_LOW] pins\n");
		}

	}
	return retval;
}

int fpc_gpio_eint_select(int bEnable, int mode)
{
	int retval = 0;
	printk("%s bEnable:%d\n", __func__, bEnable);
	if (bEnable == 1) {
		if (fpc_gpios[GPIO_EINT].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_EINT].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_EINT] pins\n");
		}
	} else {	
		if ((mode == 1) && (fpc_gpios[GPIO_EINT_HIGH].gpio_prepare)) {			
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_EINT_HIGH].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_EINT_HIGH] pins\n");
		}
		else if ((mode == 0) && (fpc_gpios[GPIO_EINT_LOW].gpio_prepare))
		{
			retval =
			    pinctrl_select_state(pinctrlfpc, fpc_gpios[GPIO_EINT_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_EINT_LOW] pins\n");
		}
	}
	return retval;
}




int fp_read_id(u8 addr, u8 *buf)
{
	
	ssize_t status=-1;
#if 0
	struct spi_device *spi = spidev;
	struct spi_message m;

	/*Set address*/
	u8 write_addr[] = {FP_WRITE_ADDRESS, addr};
	u8 read_value[] = {FP_READ_DATA, 0x00};

	u8 result[] = {0xFF, 0xFF};

	struct spi_transfer t1 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = read_value,
		.rx_buf = result,
		.len = 2,
	};

	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status == 0) {
		*buf = result[1];
		DEBUG_PRINT("fp_read_register address = %x result = %x %x\n"
					, addr, result[0], result[1]);
	} else
#if __WORDSIZE==32
		pr_err("%s read data error status = %d\n"
				, __func__, status);
#elif __WORDSIZE==64
		pr_err("%s read data error status = %ld\n"
				, __func__, status);
#endif
#endif
	return status;
}

/* Lct add by zl*/
#ifdef LCT_FPS_ET_SYS_NODE_SUP
static ssize_t et320_get_ID(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t status;
	struct spi_device *spi = spidev;
	struct spi_message m;

	/*Set address*/
	//u8 write_addr[] = {FP_WRITE_ADDRESS, 0x00};
	u8 write_addr[] = {FP_WRITE_ADDRESS, 0x0b};
	u8 read_value[] = {FP_READ_DATA, 0x00};
	u8 result[] = {0xFF, 0xFF};

	struct spi_transfer t1 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = read_value,
		.rx_buf = result,
		.len = 2,
	};

	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status == 0) {
		sprintf(buf,"%x = %x ",result[0],result[1]);
		DEBUG_PRINT("et320_get_ID address = %x result = %x\n"
					,result[0], result[1]);
	} else
#if __WORDSIZE==32
		pr_err("%s read data error status = %d\n"
				, __func__, status);
#elif  __WORDSIZE==64
		pr_err("%s read data error status = %ld\n"
				, __func__, status);
#endif
	return status;
}
static ssize_t et320_set_ID(struct device* cd, struct device_attribute *attr,const char* buf, size_t len)
{
	ssize_t status;
	struct spi_device *spi = spidev;
	struct spi_message m;
	
	/*Set address*/
	u8 write_addr[] = {FP_WRITE_ADDRESS, 0x00};
	u8 set_value[] = {FP_WRITE_DATA, 0x00};

	struct spi_transfer t1 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.len = 2,
	};

	if(2!=sscanf(buf,"%x %x",(unsigned int *)&write_addr[1],(unsigned int *)&set_value[1]))
	{
		//sprintf(buf,"%s sscanf transfer error!",__func__);
		return status=-1;	
	}
	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status == 0) {
		DEBUG_PRINT("et320_set_IDr address = %x result =%x\n"
					,write_addr[1], set_value[1]);
	} else
#if __WORDSIZE==32	
		pr_err("%s set data error status = %d\n"
				, __func__, status);
#elif __WORDSIZE==64
		pr_err("%s set data error status = %ld\n"
				, __func__, status);
#endif
	return status;
}
#endif

/*--------------------------- Data Transfer -----------------------------*/

//static struct spi_transfer msg_xfer[EGIS_MASS_READ_SEGMENT_COUNT];
int fp_mass_read(struct fp_data *fp, u8 addr, u8 *buf, int read_len)
{
	ssize_t status;
	struct spi_device *spi;
	struct spi_message m;
	//u8 write_addr[2];
	u8 write_addr[] = {FP_WRITE_ADDRESS, addr};
	/* Set start address */
	u8 *read_data = buf;
	struct spi_transfer t_set_addr = {
		.tx_buf = NULL,
		.len = 2,
	};

	/* Write and read data in one data query section */
	struct spi_transfer t_read_data = {
		.tx_buf = NULL,
		.rx_buf = NULL,
		.len = read_len,
	};

	DEBUG_PRINT("%s read_len = %d\n", __func__, read_len);

	read_data[0] = FP_READ_DATA;

	t_set_addr.tx_buf = write_addr;
	t_read_data.tx_buf = t_read_data.rx_buf = read_data;

	spi = fp->spi;

	spi_message_init(&m);
	spi_message_add_tail(&t_set_addr, &m);
	spi_message_add_tail(&t_read_data, &m);
	status = spi_sync(spi, &m);

	return status;
}

/*
 * Read io register
 */
int fp_io_read_register(struct fp_data *fp, u8 *addr, u8 *buf)
{
	ssize_t status = 0;
	struct spi_device *spi;
	struct spi_message m;
	int read_len = 1;

	u8 write_addr[] = {FP_WRITE_ADDRESS, 0x00};
	u8 read_value[] = {FP_READ_DATA, 0x00};
	u8 val, addrval;

	u8 result[] = {0xFF, 0xFF};
	struct spi_transfer t_set_addr = {
	
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = read_value,
		.rx_buf = result,
		.len = 2,
	};

	if (copy_from_user(&addrval, (const u8 __user *) (uintptr_t) addr
		, read_len)) {
		pr_err("%s buffer copy_from_user fail", __func__);
		status = -EFAULT;
		return status;
	}

	DEBUG_PRINT("%s read_len = %d\n", __func__, read_len);

	spi = fp->spi;

	/*Set address*/
	write_addr[1] = addrval;

	/*Start to read*/
	spi_message_init(&m);
	spi_message_add_tail(&t_set_addr, &m);
	spi_message_add_tail(&t, &m);
	status = spi_sync(spi, &m);
	if (status < 0) {
#if __WORDSIZE==32	
		pr_err("%s read data error status = %d\n"
				, __func__, status);
#elif __WORDSIZE==64
		pr_err("%s read data error status = %ld\n"
				, __func__, status);
#endif
		return status;
	}

	val = result[1];

	DEBUG_PRINT("%s Read add_val = %x buf = 0x%x\n", __func__,
			addrval, val);

	if (copy_to_user((u8 __user *) (uintptr_t) buf, &val, read_len)) {
		pr_err("%s buffer copy_to_user fail status", __func__);
		status = -EFAULT;
		return status;
	}

	return status;
}

/*
 * Write data to register
 */
int fp_io_write_register(struct fp_data *fp, u8 *buf)
{
	ssize_t status = 0;
	struct spi_device *spi;
	int write_len = 2;
	struct spi_message m;

	u8 write_addr[] = {FP_WRITE_ADDRESS, 0x00};
	u8 write_value[] = {FP_WRITE_DATA, 0x00};
	u8 val[2];

	struct spi_transfer t1 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_value,
		.len = 2,
	};

	if (copy_from_user(val, (const u8 __user *) (uintptr_t) buf
		, write_len)) {
		pr_err("%s buffer copy_from_user fail", __func__);
		status = -EFAULT;
		return status;
	}

	DEBUG_PRINT("%s write_len = %d\n", __func__, write_len);
	DEBUG_PRINT("%s address = %x data = %x\n", __func__, val[0], val[1]);

	spi = fp->spi;

	/*Set address*/
	write_addr[1] = val[0];
	/*Set value*/
	write_value[1] = val[1];

	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status < 0) {
#if __WORDSIZE==32
		pr_err("%s write data error status = %d\n",
				__func__, status);
#elif __WORDSIZE==64
		pr_err("%s write data error status = %ld\n",
				__func__, status);
#endif
		return status;
	}

	return status;
}

int fp_read_register(struct fp_data *fp, u8 addr, u8 *buf)
{
	ssize_t status;
	struct spi_device *spi;
	struct spi_message m;

	/*Set address*/
	u8 write_addr[] = {FP_WRITE_ADDRESS, addr};
	u8 read_value[] = {FP_READ_DATA, 0x00};
	u8 result[] = {0xFF, 0xFF};

	struct spi_transfer t1 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.speed_hz = FPC_BTP_SPI_CLOCK_SPEED,
		.tx_buf = read_value,
		.rx_buf = result,
		.len = 2,
	};

	spi = fp->spi;
	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status == 0) {
		*buf = result[1];
		DEBUG_PRINT("fp_read_register address = %x result = %x %x\n"
					, addr, result[0], result[1]);
	} else
#if __WORDSIZE==32
		pr_err("%s read data error status = %d\n"
				, __func__, status);
#elif __WORDSIZE==64
		pr_err("%s read data error status = %ld\n"
				, __func__, status);
#endif

	return status;
}

int fp_set_single_register(struct fp_data *fp, u8 addr, u8 val)
{
	ssize_t status = 0;
	struct spi_device *spi;
	struct spi_message m;

	u8 write_addr[] = {FP_WRITE_ADDRESS, addr};
	u8 write_value[] = {FP_WRITE_DATA, val};

	struct spi_transfer t1 = {
		.tx_buf = write_addr,
		.len = 2,
	};
	struct spi_transfer t2 = {
		.tx_buf = write_value,
		.len = 2,
	};

	spi = fp->spi;

	spi_message_init(&m);
	spi_message_add_tail(&t1, &m);
	spi_message_add_tail(&t2, &m);
	status = spi_sync(spi, &m);
	if (status < 0) {
#if __WORDSIZE==32
		pr_err("%s write data error status = %d\n",
				__func__, status);
#elif __WORDSIZE==64
		pr_err("%s write data error status = %ld\n",
				__func__, status);
#endif
		return status;
	}

	return status;
}

int fp_io_get_one_image(
	struct fp_data *fp,
	u8 *buf,
	u8 *image_buf
	)
{
	uint8_t read_val,
			*tx_buf = (uint8_t *)buf,
			*work_buf = NULL,
			val[2];
	int32_t status;
	uint32_t frame_not_ready_count = 0, read_count;
	u32 spi_transfer_len;

	DEBUG_PRINT("%s\n", __func__);

	if (copy_from_user(val, (const u8 __user *) (uintptr_t) tx_buf, 2)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}

	/* total pixel , width * hight */
	read_count = val[0] * val[1];

	while (1) {
		status = fp_read_register
				(fp, FSTATUS_FP_ADDR, &read_val);
		if (status < 0)
			goto end;

		if (read_val & FRAME_READY_MASK)
			break;

		if (frame_not_ready_count >= 250) {
			pr_err("frame_not_ready_count = %d\n",
					frame_not_ready_count);
			break;
		}
		frame_not_ready_count++;
	}

	spi_transfer_len = (read_count + 3) > SPI_PACKET_SIZE ? (read_count + 3 + SPI_PACKET_SIZE - 1) / SPI_PACKET_SIZE * SPI_PACKET_SIZE : read_count + 3;

	work_buf = kzalloc(spi_transfer_len, GFP_KERNEL);
	if (work_buf == NULL) {
		status = -ENOMEM;
		goto end;
	}
	status = fp_mass_read(fp, FDATA_FP_ADDR, work_buf, spi_transfer_len);
	if (status < 0) {
		pr_err("%s call fp_mass_read error status = %d\n"
				, __func__, status);
		goto end;
	}

	if (copy_to_user((u8 __user *) (uintptr_t) image_buf,
		work_buf+3, read_count)) {
		pr_err("buffer copy_to_user fail status = %d\n", status);
		status = -EFAULT;
	}
end:
	kfree(work_buf);
	return status;
}

/* ------------------------------ Interrupt -----------------------------*/

/*
 * Interrupt description
 */

#define FP_INT_DETECTION_PERIOD  10
#define FP_DETECTION_THRESHOLD	10

struct interrupt_desc fps_ints;
/*
 * FPS interrupt table
 */
//struct interrupt_desc fps_ints={TRIGGER_FPS_GPIO_PIN , 0, "BUT0" , 0};
// struct interrupt_desc fps_ints[] = {
// #if LGE_TEST
// 	{GPIO8 , 0, "BUT0" , 0} /* TINY4412CON15 XEINT12 pin */
// #else
// 	{GPIO8 , 0, "BUT0" , 0} /* TINY4412CON15 XEINT12 pin */
// #endif
// };
//int fps_ints_size = ARRAY_SIZE(fps_ints);

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

void interrupt_timer_routine(
	unsigned long _data
)
{
	struct interrupt_desc *bdata = (struct interrupt_desc *)_data;
	pr_info("FPS interrupt count = %d" , bdata->int_count);
	if (bdata->int_count >= bdata->detect_threshold) {
		bdata->finger_on = 1;
		pr_info("FPS triggered !!!!!!!\n");
	} else
		pr_info("FPS not triggered !!!!!!!\n");
	//bdata->int_count = 0;
		 fps_ints.int_count = 0;
	wake_up_interruptible(&interrupt_waitq);
}

/*
 *	FUNCTION NAME.
 *		fp_interrupt_edge
 *		fp_interrupt_level
 *
 *	FUNCTIONAL DESCRIPTION.
 *		finger print interrupt callback routine
 *		fp_interrupt_edge : for edge trigger
 *		fp_interrupt_level : for level trigger
 *
 */

static irqreturn_t fp_interrupt_edge(int irq, void *dev_id)
{
	//disable_irq_nosync(fp_detect_irq);
	if (!fps_ints.int_count)
		mod_timer(&fps_ints.timer,jiffies + msecs_to_jiffies(fps_ints.detect_period));
	fps_ints.int_count++;
	printk("fps_ints.int_count++; = %d\n",fps_ints.int_count);
	//disable_irq_nosync(fp_detect_irq);
	//mt_eint_unmask(CUST_EINT_FPS_EINT_NUM);
	//enable_irq(fp_detect_irq);
	return IRQ_HANDLED;
}
static irqreturn_t fp_interrupt_level(int irq, void *dev_id)
{	
	disable_irq_nosync(fp_detect_irq);
	printk("fp->isr\n");
	fps_ints.finger_on = 1;
	wake_up_interruptible(&interrupt_waitq);
	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	return IRQ_HANDLED;
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
 *			EDGE_TRIGGER_RISING    0x1
 *			LEVEL_TRIGGER_LOW       0x2
 *			LEVEL_TRIGGER_HIGH      0x3
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

int Interrupt_Init(int int_mode, int detect_period, int detect_threshold)
{
        //int i, irq;
        int ret = 0;
        struct device_node *node = NULL;
 
        pr_info("FP %s int_mode = %d detect_period = %d detect_threshold = %d\n",
                                __func__,
                                int_mode,
                                detect_period,
                                detect_threshold);
 
	fps_ints.detect_period = detect_period;
	fps_ints.detect_threshold = detect_threshold;
    
        fps_ints.int_count = 0;
        fps_ints.finger_on = 0;
 

	if(fps_ints.initialized_flag)
	{
		printk("fp interrput enable \n");
		fps_ints.initialized_flag = DRDY_IRQ_ENABLE;
		enable_irq(fp_detect_irq);
		return 0;	
	}
 	fps_ints.initialized_flag = true;

	
     //   if(fps_ints.drdy_irq_flag == DRDY_IRQ_DISABLE){
 
        node = of_find_matching_node(node, fp_of_match);
 
        if (node){
                fp_detect_irq = irq_of_parse_and_map(node, 0);
                printk("fp_irq number %d\n", fp_detect_irq);
                if (int_mode == EDGE_TRIGGER_RISING){
                        ret = request_irq(fp_detect_irq,fp_interrupt_edge, IRQF_TRIGGER_RISING,"fp_detect-eint", NULL);
                }
				else if(int_mode == EDGE_TRIGGER_FALLING) {
                        ret = request_irq(fp_detect_irq,fp_interrupt_edge, IRQF_TRIGGER_FALLING, "fp_detect-eint", NULL);
                }
                else if(int_mode == LEVEL_TRIGGER_LOW) {
                        ret = request_irq(fp_detect_irq,fp_interrupt_level, IRQ_TYPE_LEVEL_LOW, "fp_detect-eint", NULL);
                }
                else if(int_mode == LEVEL_TRIGGER_HIGH) {
                        ret = request_irq(fp_detect_irq,fp_interrupt_level, IRQ_TYPE_LEVEL_HIGH, "fp_detect-eint", NULL);
                }				
                if (ret > 0){
                        printk("fingerprint request_irq IRQ LINE NOT AVAILABLE!.");
                }
        }else{
                printk("fingerprint request_irq can not find fp eint device node!.");
        }
		
		DEBUG_PRINT("[Interrupt_Init]:gpio_to_irq return: %d\n", fp_detect_irq);

        enable_irq(fp_detect_irq);
        fps_ints.drdy_irq_flag = DRDY_IRQ_ENABLE;
      //  }
 
        return 0;
}

/*
 *	FUNCTION NAME.
 *		Interrupt_disable
 *
 *	FUNCTIONAL DESCRIPTION.
 *		free all interrupt resource
 *
 *	EXIT PARAMETERS.
 *		Function Return int
 */

int Interrupt_Disable(void)
{

	if(fps_ints.drdy_irq_flag == DRDY_IRQ_ENABLE)
	{
		disable_irq_nosync(fp_detect_irq);
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
	struct poll_table_struct *wait
)
{
	unsigned int mask = 0;
	//int i = 0;
	pr_info("%s\n", __func__);

	fps_ints.int_count = 0;
	poll_wait(file, &interrupt_waitq, wait);
	if (fps_ints.finger_on) 
	{
		mask |= POLLIN | POLLRDNORM;
		fps_ints.finger_on = 0;
		printk("fp -> FOE \n");
	}
	return mask;
}

void fps_interrupt_abort(void)
{
	fps_ints.finger_on = 0;
	wake_up_interruptible(&interrupt_waitq);
}

/*-------------------------------------------------------------------------*/

static void fp_reset(void)
{
	DEBUG_PRINT("%s\n", __func__);
	#if 0
	//mt_set_gpio_out(GPIO_FPS_RESET_PIN, GPIO_OUT_ZERO);
	gpio_set_value(GPIO_FPS_RESET_PIN, 0);
	msleep(30);
	//mt_set_gpio_out(GPIO_FPS_RESET_PIN, GPIO_OUT_ONE);
	gpio_set_value(GPIO_FPS_RESET_PIN, 1);
	msleep(20);
	#else
	fpc_gpio_rst_select(0);
	msleep(30);	
	fpc_gpio_rst_select(1);
	msleep(20);
	#endif
}

static void fp_reset_set(int enable)
{
	DEBUG_PRINT("%s\n", __func__);
	DEBUG_PRINT("%s enable %d\n", __func__, enable);
	if (enable == 0) {
		#if 0
		//mt_set_gpio_out(GPIO_FPS_RESET_PIN, GPIO_OUT_ZERO);
		gpio_set_value(GPIO_FPS_RESET_PIN, 0);
		msleep(30);
		#else
		fpc_gpio_rst_select(0);
		#endif
	} else {
		#if 0
		//mt_set_gpio_out(GPIO_FPS_RESET_PIN, GPIO_OUT_ONE);
		gpio_set_value(GPIO_FPS_RESET_PIN, 1);
		#else
		fpc_gpio_rst_select(1);
		#endif
		msleep(20);
	}
}

static ssize_t fp_read(struct file *filp,
						char __user *buf,
						size_t count,
						loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}

static ssize_t fp_write(struct file *filp,
						const char __user *buf,
						size_t count,
						loff_t *f_pos)
{
	/*Implement by vendor if needed*/
	return 0;
}

static long fp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int retval = 0;
	struct fp_data *fp;
	struct spi_device *spi;
	u32 tmp;
	struct egis_ioc_transfer *ioc = NULL;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != EGIS_IOC_MAGIC) {
		pr_err("%s _IOC_TYPE(cmd) != EGIS_IOC_MAGIC", __func__);
		return -ENOTTY;
	}

	/*
	 * Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
						(void __user *)arg,
						_IOC_SIZE(cmd));
	if (err) {
		pr_err("%s err", __func__);
		return -EFAULT;
	}

	/*
	 * guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	fp = filp->private_data;
	spin_lock_irq(&fp->spi_lock);
	spi = spi_dev_get(fp->spi);
	spin_unlock_irq(&fp->spi_lock);

	if (spi == NULL) {
		pr_err("%s spi == NULL", __func__);
		return -ESHUTDOWN;
	}

	mutex_lock(&fp->buf_lock);

	/* segmented and/or full-duplex I/O request */
	if (_IOC_NR(cmd) != _IOC_NR(EGIS_IOC_MESSAGE(0))
					|| _IOC_DIR(cmd) != _IOC_WRITE) {
		retval = -ENOTTY;
		goto out;
	}

	tmp = _IOC_SIZE(cmd);
	if ((tmp == 0) || (tmp % sizeof(struct egis_ioc_transfer)) != 0) {
		retval = -EINVAL;
		goto out;
	}

	/* copy into scratch area */
	ioc = kmalloc(tmp, GFP_KERNEL);
	if (ioc == NULL) {
		retval = -ENOMEM;
		goto out;
	}
	if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
		retval = -EFAULT;
		goto out;
	}

	/*
	 * Read register
	 * tx_buf include register address will be read
	 */
	if (ioc->opcode == FP_REGISTER_READ) {
		u8 *address = ioc->tx_buf;
		u8 *result = ioc->rx_buf;
		DEBUG_PRINT("fp FP_REGISTER_READ\n");

		retval = fp_io_read_register(fp, address, result);
		if (retval < 0)	{
			pr_err("%s FP_REGISTER_READ error retval = %d\n"
			, __func__, retval);
			goto out;
		}
	}

	/*
	 * Write data to register
	 * tx_buf includes address and value will be wrote
	 */
	if (ioc->opcode == FP_REGISTER_WRITE) {
		u8 *buf = ioc->tx_buf;
		DEBUG_PRINT("fp FP_REGISTER_WRITE");

		retval = fp_io_write_register(fp, buf);
		if (retval < 0) {
			pr_err("%s FP_REGISTER_WRITE error retval = %d\n"
			, __func__, retval);
			goto out;
		}
	}

	/*
	 * Get one frame data from sensor
	 */
	if (ioc->opcode == FP_GET_ONE_IMG) {
		u8 *buf = ioc->tx_buf;
		u8 *image_buf = ioc->rx_buf;
		DEBUG_PRINT("fp FP_GET_ONE_IMG\n");

        spi->controller_data = (void *) &spi_conf_dma;          // mtk configuration
        spi_setup(spi);

		retval = fp_io_get_one_image(fp, buf, image_buf);
		
		//modify by hxl start
		spi->controller_data = (void *) &spi_conf;				
        spi_setup(spi);
		//modify by hxl end

		if (retval < 0) {
			pr_err("%s FP_GET_ONE_IMG error retval = %d\n"
			, __func__, retval);
			goto out;
		}
	}

	if (ioc->opcode == FP_SENSOR_RESET)
		fp_reset();

	if (ioc->opcode == FP_RESET_SET) {
		pr_info("%s FP_SENSOR_RESET\n", __func__);
		pr_info("%s status = %d\n", __func__, ioc->len);
		fp_reset_set(ioc->len);
	}

	if (ioc->opcode == FP_SET_SPI_CLOCK) {
		__u32 current_speed = spi->max_speed_hz;
		pr_info("%s FP_SET_SPI_CLOCK\n", __func__);
		pr_info("%s speed_hz = %d\n", __func__, ioc->speed_hz);
		pr_info("%s current_speed = %d\n", __func__, current_speed);

		spi->max_speed_hz = ioc->speed_hz;
		retval = spi_setup(spi);
		if (retval < 0) {
			pr_err("%s spi_setup error %d\n", __func__, retval);
			spi->max_speed_hz = current_speed;
		}
		pr_info("%s spi speed_hz = %d\n", __func__, spi->max_speed_hz);
	}

	if (ioc->opcode == FP_POWER_ONOFF)
		pr_info("power control status = %d\n", ioc->len);

	/*
	 * Trigger inital routine
	 */
	if (ioc->opcode == INT_TRIGGER_INIT) {
		pr_info(">>> fp Trigger function init\n");
		retval = Interrupt_Init(
				(int)ioc->pad[0],
				(int)ioc->pad[1],
				(int)ioc->pad[2]);
		pr_info("trigger init = %d\n", retval);
	}

	/*
	 * trigger
	 */
	if (ioc->opcode == INT_TRIGGER_CLOSE) {
		pr_info("<<< fp Trigger function close\n");
		retval = Interrupt_Disable();
		pr_info("trigger close = %d\n", retval);
	}

	/*
	 * read interrupt status
	 */
	if (ioc->opcode == INT_TRIGGER_ABORT)
		fps_interrupt_abort();

out:
	if (ioc != NULL)
		kfree(ioc);

	mutex_unlock(&fp->buf_lock);
	spi_dev_put(spi);
	if (retval < 0)
		pr_err("%s retval = %d", __func__, retval);
	return retval;
}

#ifdef CONFIG_COMPAT
static long fp_compat_ioctl(struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	return fp_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define fp_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int fp_open(struct inode *inode, struct file *filp)
{
	struct fp_data *fp;
	int			status = -ENXIO;

	DEBUG_PRINT("%s\n", __func__);
	mutex_lock(&device_list_lock);

	list_for_each_entry(fp, &device_list, device_entry)
	{
		if (fp->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (fp->buffer == NULL) {
			fp->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (fp->buffer == NULL) {
				dev_dbg(&fp->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			fp->users++;
			filp->private_data = fp;
			nonseekable_open(inode, filp);
		}
	} else
		pr_debug("fp: nothing for minor %d\n", iminor(inode));
	
	mutex_unlock(&device_list_lock);
	return status;
}

static int fp_release(struct inode *inode, struct file *filp)
{
	struct fp_data *fp;

	DEBUG_PRINT("%s\n", __func__);
	mutex_lock(&device_list_lock);
	fp = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	fp->users--;
	if (fp->users == 0) {
		int	dofree;

		kfree(fp->buffer);
		fp->buffer = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&fp->spi_lock);
		dofree = (fp->spi == NULL);
		spin_unlock_irq(&fp->spi_lock);

		if (dofree)
			kfree(fp);
	}
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct file_operations fp_fops = {
	.owner = THIS_MODULE,
	.write = fp_write,
	.read = fp_read,
	.unlocked_ioctl = fp_ioctl,
	.compat_ioctl = fp_compat_ioctl,
	.open = fp_open,
	.release = fp_release,
	.llseek = no_llseek,
	.poll = fps_interrupt_poll
};


/*-------------------------------------------------------------------------*/
/* Lct add device node by zl */
/* sys node */
#ifdef LCT_FPS_ET_SYS_NODE_SUP
int fpc_create_device_node(void)
{
	int ret;
	DEBUG_PRINT("%s\n",__func__);
	ret = device_create_file(&spidev->dev, &dev_attr_et320_id);
	if (ret != 0) {
		dev_err(&spidev->dev,
				"Failed to create xxx sysfs files: %d\n", ret);
		return ret;
	}
	return ret;
}
#endif
/* proc node */
static int fp_device_show(struct seq_file *m, void *v)
{
	u8 buf1=0,buf2=0;
	struct fp_data *fp = spi_get_drvdata(spidev);

	DEBUG_PRINT("%s\n",__func__);
	if(fp == NULL){
		DEBUG_PRINT("%s: spi_get_drvdata error!\n",__func__);
		return -1;
	}
	fp_read_register(fp,0x11,&buf1);
	fp_read_register(fp,0x13,&buf2);
	
	if((buf1 == 0x38)&&(buf2 == 0x71))
		seq_printf(m, "%s\n","et320");
	else
		seq_printf(m, "%s\n","false");
	return 0;
}

static int fp_proc_open(struct inode *inode, struct file *filp)
{

	 return single_open(filp, fp_device_show, NULL);
}


static const struct file_operations fp_proc_fops = {
	.owner= THIS_MODULE,
	.open= fp_proc_open,
	.read= seq_read,
	.llseek= seq_lseek,
	.release= single_release,
};
/* Lct add end*/
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/

static struct class *fp_class;

/*-------------------------------------------------------------------------*/

static int __init fp_probe(struct spi_device *spi)
{
	struct fp_data *fp;
	int status;
	unsigned long minor;
	//int i;
	//int ret;
	struct spi_device *spi_initialize;
	unsigned char version[4];

	DEBUG_PRINT("%s initial\n", __func__);

	/* Allocate driver data */
	fp = kzalloc(sizeof(*fp), GFP_KERNEL);
	if (fp == NULL)
		return -ENOMEM;

	/* Initialize the driver data */
	fp->spi = spi;
	spidev = spi;

	//hwPowerOn(MT6331_POWER_LDO_VMCH, VOL_3300,"ET320_3V3");	//mtk 3.3V power-on
	spi->controller_data = (void *) &spi_conf;          // mtk configuration
    spi->max_speed_hz = FPC_BTP_SPI_CLOCK_SPEED;
    spi_setup(spi);
	spin_lock_init(&fp->spi_lock);
	mutex_init(&fp->buf_lock);

	INIT_LIST_HEAD(&fp->device_entry);

	/*
	 * If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;
		fp->devt = MKDEV(FP_MAJOR, minor);
		dev = device_create(fp_class, &spi->dev, fp->devt,
					fp, "esfp0");
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else{
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&fp->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
#if 0
	ret = gpio_request(GPIO_FPS_RESET_PIN, "ars_rst");
	if (ret)
		printk("[ars3022]  : gpio_request (%d)fail\n", GPIO_FPS_RESET_PIN);

	ret = gpio_direction_output(GPIO_FPS_RESET_PIN, 0);
	if (ret)
		printk("[ars3022]  : gpio_direction_output (%d)fail\n",GPIO_FPS_RESET_PIN);

	gpio_set_value(GPIO_FPS_RESET_PIN, 1);
	msleep(20);

	ret = gpio_request(P_GPIO_FPS_PWR_PIN, "ars_pwr");
	if (ret)
		printk("[ars3022]  : gpio_request (%d)fail\n", P_GPIO_FPS_PWR_PIN);

	ret = gpio_direction_output(P_GPIO_FPS_PWR_PIN, 0);
	if (ret)
		printk("[ars3022]gpio_direction_output (%d)fail\n",P_GPIO_FPS_PWR_PIN);

	gpio_set_value(P_GPIO_FPS_PWR_PIN, 1);
	//mt_set_gpio_dir(GPIO_FPS_RESET_PIN, GPIO_DIR_OUT);
	#else
	fpc_gpio_rst_select(0);
	fpc_gpio_rst_select(1);
	
	msleep(20);
	
	fpc_gpio_pwr_select(0);
	fpc_gpio_pwr_select(1);
	#endif
	spin_lock_irq(&fp->spi_lock);
	spi_initialize= spi_dev_get(fp->spi);
	spin_unlock_irq(&fp->spi_lock);	

	spi_initialize->mode = SPI_MODE_3;
	spi_initialize->bits_per_word = 8;

	spi_setup(spi_initialize);

	fp_reset();
	if (status == 0)
		spi_set_drvdata(spi, fp);
	else
		kfree(fp);

	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;
	fps_ints.initialized_flag = false;
	
	setup_timer(&fps_ints.timer, interrupt_timer_routine, (unsigned long)&fps_ints);
	add_timer(&fps_ints.timer);

	DEBUG_PRINT("%s : initialize success %d\n", __func__, status);


/*	fp_read_id(0x10,&version[0]);
	fp_read_id(0x11,&version[1]);
	fp_read_id(0x12,&version[2]);
	fp_read_id(0x13,&version[3]);*///modify by hxl
	printk("%s version:%02x.%02x.%02x.%02x\n", __func__, version[0],version[1],version[2],version[3]);
		
	//Interrupt_Init(0,20,10);

		//prize-lixuefeng-20150512-start
#if defined(CONFIG_PRIZE_HARDWARE_INFO)
		sprintf(current_fingerprint_info.chip,"et320 %02x.%02x.%02x.%02x",version[0],version[1],version[2],version[3]);
		sprintf(current_fingerprint_info.id,"0x%04x",spi->master->bus_num);
		strcpy(current_fingerprint_info.vendor,"et320");
		strcpy(current_fingerprint_info.more,"fingerprint");
#endif
		//prize-lixuefeng-20150512-end
	

	/* Lct add by zl */
#ifdef LCT_FPS_ET_SYS_NODE_SUP
	fpc_create_device_node();
#endif
	fp_proc = proc_create("fpc", 0, NULL, &fp_proc_fops);
	if (!fp_proc) {
		DEBUG_PRINT("%s : cannot create proc node : %d\n", __func__, status);
		return -ENOMEM;
	}
	/* Lct add end */
	return status;
}

static int fp_remove(struct spi_device *spi)
{
	struct fp_data *fp = spi_get_drvdata(spi);
	DEBUG_PRINT("%s\n", __func__);
	
	//modify by zl
	remove_proc_entry("et320", NULL);
	
	//hwPowerDown(MT6331_POWER_LDO_VMCH,"ET320_3V3");	// mtk 3.3v power-off

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&fp->spi_lock);
	fp->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&fp->spi_lock);

	fps_ints.drdy_irq_flag = DRDY_IRQ_DISABLE;

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&fp->device_entry);
	device_destroy(fp_class, fp->devt);
	clear_bit(MINOR(fp->devt), minors);
	if (fp->users == 0)
		kfree(fp);
	mutex_unlock(&device_list_lock);

	return 0;
}

struct spi_device_id et320_id_table = {"et320", 0};

static struct spi_driver fp_spi_driver = {

	.driver = {
		.name = "et320",
		.bus	= &spi_bus_type,
		.owner = THIS_MODULE,

	},
	.probe = fp_probe,
	.remove = fp_remove,
	.id_table = &et320_id_table,

};

static struct spi_board_info spi_board_devs[] __initdata = {
	[0] = {
			.modalias="et320",
			.bus_num = 0,
			.chip_select=0,
			.mode = SPI_MODE_3,
		},
};

/*-------------------------------------------------------------------------*/

static int __init fp_init(void)
{
	int status;
	DEBUG_PRINT("%s\n", __func__);

	/*
	 * Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */

	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(FP_MAJOR, "et320", &fp_fops);
	if (status < 0) {
		DEBUG_PRINT("%s : cannot register chrdev : %d\n", __func__, status);
		return status;
	}
	
	fp_class = class_create(THIS_MODULE, "fp");
	if (IS_ERR(fp_class)) {
		DEBUG_PRINT("%s : cannot create class\n", __func__);
		unregister_chrdev(FP_MAJOR, fp_spi_driver.driver.name);
		return PTR_ERR(fp_class);
	}

	spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
	status = spi_register_driver(&fp_spi_driver);
	if (status < 0) {
		DEBUG_PRINT("%s : cannot spi register : %d\n", __func__, status);
		class_destroy(fp_class);
		unregister_chrdev(FP_MAJOR, fp_spi_driver.driver.name);
	}
	

	return status;
}
//module_init(fp_init);

static void __exit fp_exit(void)
{
	DEBUG_PRINT("%s\n", __func__);
	//gpio_free(GPIO_FPS_RESET_PIN);
	spi_unregister_driver(&fp_spi_driver);
	class_destroy(fp_class);
	unregister_chrdev(FP_MAJOR, fp_spi_driver.driver.name);
}
//module_exit(fp_exit);


static int fpc_probe(struct platform_device *pdev)
{
	printk("%s: dev name ===%s===\n", __func__, dev_name(&pdev->dev));
	fpc_get_dts_info(pdev);
	//modify by zl
	fpc_gpio_spi_select(1);
	fpc_gpio_eint_select(1, 0);
	fp_init();
	return 0;
}

static int fpc_remove(struct platform_device *pdev)
{
	printk("%s\n", __func__);
	fp_exit();

	return 0;
}

static struct platform_driver fpc_driver = {
	.probe = fpc_probe,
	.remove = fpc_remove,
	.driver = {
			.name = FPC_DEVICE,
			.owner = THIS_MODULE,
			.of_match_table = fp_of_match,
	},
};




static int __init fpc_platform_init(void)
{
	int ret;

	printk("%s\n", __func__);

	ret = platform_driver_register(&fpc_driver);
	return ret;

}
module_init(fpc_platform_init);

static void __exit fpc_platform_exit(void)
{
	printk("%s\n", __func__);

	platform_driver_unregister(&fpc_driver);
}
module_exit(fpc_platform_exit);



MODULE_AUTHOR("Wang YuWei, <robert.wang@egistec.com>");
MODULE_DESCRIPTION("SPI Interface for ET320");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
