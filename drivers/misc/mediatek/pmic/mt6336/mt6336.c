/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <mach/mtk_charging.h>
#if (CONFIG_MTK_GAUGE_VERSION == 30)
#else
#include <mt-plat/battery_common.h>
#endif
#include <mt-plat/charging.h>

#include "mt6336.h"

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/

#ifdef CONFIG_OF
#else
#define mt6336_SLAVE_ADDR_WRITE   0xD6
#define mt6336_SLAVE_ADDR_Read    0xD7

#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define mt6336_BUSNUM I2C_SWITHING_CHARGER_CHANNEL
#else
#define mt6336_BUSNUM 0
#endif

#endif
static struct i2c_client *new_client;
static const struct i2c_device_id mt6336_i2c_id[] = { {"mt6336", 0}, {} };
kal_bool chargin_hw_init_done;
static DEFINE_MUTEX(mt6336_i2c_access);
int g_mt6336_hw_exist;
unsigned char g_mt6336_hwcid;
struct mt6336_ctrl *core_ctrl;


/**********************************************************
  *
  *   [I2C Function For Read/Write mt6336]
  *
  *********************************************************/
#define CODA_ADDR_WIDTH 0x100
#define SLV_BASE_ADDR 0x52

static int mt6336_read_device(unsigned int reg, unsigned char *returnData, unsigned int len)
{
	new_client->addr = reg / CODA_ADDR_WIDTH + SLV_BASE_ADDR;
	return i2c_smbus_read_i2c_block_data(new_client, (reg % CODA_ADDR_WIDTH), len, returnData);
}

static int mt6336_write_device(unsigned int reg, unsigned char *writeData, unsigned int len)
{
	new_client->addr = reg / CODA_ADDR_WIDTH + SLV_BASE_ADDR;
	return i2c_smbus_write_i2c_block_data(new_client, (reg % CODA_ADDR_WIDTH), len, writeData);
}

unsigned int mt6336_read_byte(unsigned int reg, unsigned char *returnData)
{
	int ret = 0;

	mutex_lock(&mt6336_i2c_access);
	ret = mt6336_read_device(reg, returnData, 1);
	mutex_unlock(&mt6336_i2c_access);
	return ret < 0 ? 1 : 0;
}

unsigned int mt6336_read_bytes(unsigned int reg, unsigned char *returnData, unsigned int len)
{
	int ret = 0;
	unsigned char buffer[64] = {0};
	unsigned char *returnData_more = buffer;

	mutex_lock(&mt6336_i2c_access);
	if (g_mt6336_hwcid == 0x20) {
		/* MT6336 E2, burst read not ready */
		if (len <= 62) {
			ret = mt6336_read_device(reg - 0x2, returnData_more, len + 2);
			memcpy(returnData, returnData_more + 2, len);
		} else {
			returnData_more = kmalloc_array(len + 2, sizeof(unsigned char), GFP_KERNEL);
			ret = mt6336_read_device(reg - 0x2, returnData_more, len + 2);
			memcpy(returnData, returnData_more + 2, len);
			kfree(returnData_more);
			returnData_more = NULL;
		}
	} else {
		/* Other chip version */
		ret = mt6336_read_device(reg, returnData, len);
	}

	mutex_unlock(&mt6336_i2c_access);
	return ret < 0 ? 1 : 0;
}

unsigned int mt6336_write_byte(unsigned int reg, unsigned char writeData)
{
	int ret = 0;

	mutex_lock(&mt6336_i2c_access);
	ret = mt6336_write_device(reg, &writeData, 1);
	mutex_unlock(&mt6336_i2c_access);
	return ret < 0 ? 1 : 0;
}

unsigned int mt6336_write_bytes(unsigned int reg, unsigned char *writeData, unsigned int len)
{
	int ret = 0;

	new_client->addr = reg / CODA_ADDR_WIDTH + SLV_BASE_ADDR;
	mutex_lock(&mt6336_i2c_access);
	ret = mt6336_write_device(reg, writeData, len);
	mutex_unlock(&mt6336_i2c_access);
	return ret < 0 ? 1 : 0;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
unsigned int mt6336_read_interface(unsigned int RegNum, unsigned char *val, unsigned char MASK,
				  unsigned char SHIFT)
{
	unsigned char mt6336_reg = 0;
	unsigned int ret = 0;
	unsigned char reg_val;

	mutex_lock(&mt6336_i2c_access);
	ret = mt6336_read_device(RegNum, &mt6336_reg, 1);
	reg_val = mt6336_reg;
	if (ret < 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x] mt6336 read data fail, MASK=0x%x, SHIFT=%d\n",
			 __func__, ret, RegNum, MASK, SHIFT);
		return ret;
	}

	mt6336_reg &= (MASK << SHIFT);
	*val = (mt6336_reg >> SHIFT);

	MT6336LOG("[%s] Reg[0x%x]=0x%x val=0x%x device_id=0x%x\n", __func__,
		RegNum, reg_val, *val, RegNum / CODA_ADDR_WIDTH + SLV_BASE_ADDR);
	mutex_unlock(&mt6336_i2c_access);
	return ret;
}

unsigned int mt6336_config_interface(unsigned int RegNum, unsigned char val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char mt6336_reg = 0;
	unsigned int ret = 0;
	unsigned char reg_val;

	mutex_lock(&mt6336_i2c_access);
	ret = mt6336_read_device(RegNum, &mt6336_reg, 1);
	reg_val = mt6336_reg;
	if (ret < 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x] mt6336 read data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, ret, RegNum, val, MASK, SHIFT);
		mutex_unlock(&mt6336_i2c_access);
		return ret;
	}

	mt6336_reg &= ~(MASK << SHIFT);
	mt6336_reg |= (val << SHIFT);

	ret = mt6336_write_device(RegNum, &mt6336_reg, 1);
	if (ret < 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x]=0x%x mt6336 write data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, ret, RegNum, mt6336_reg, val, MASK, SHIFT);
		mutex_unlock(&mt6336_i2c_access);
		return ret;
	}
	MT6336LOG("[mt6336_config_interface] write Reg[0x%x] from 0x%x to 0x%x device_id=0x%x\n", RegNum,
		    reg_val, mt6336_reg, RegNum / CODA_ADDR_WIDTH + SLV_BASE_ADDR);
	mutex_unlock(&mt6336_i2c_access);
	return ret;
}

/* write one register directly */
unsigned int mt6336_set_register_value(unsigned int RegNum, unsigned char val)
{
	unsigned int ret = 0;

	ret = mt6336_write_byte(RegNum, val);

	return ret;
}

unsigned int mt6336_get_register_value(unsigned int RegNum)
{
	unsigned int ret = 0;
	unsigned char reg_val = 0;

	ret = mt6336_read_byte(RegNum, &reg_val);

	return reg_val;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void mt6336_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = mt6336_read_interface(0x00, &val, 0xFF, 0x0);

	if (val != 0x36)
		g_mt6336_hw_exist = 0;
	else
		g_mt6336_hw_exist = 1;

	MT6336LOG("[mt6336_hw_component_detect] exist=%d, Reg[0x03]=0x%x\n",
		 g_mt6336_hw_exist, val);
}

int is_mt6336_exist(void)
{
	MT6336LOG("[is_mt6336_exist] g_mt6336_hw_exist=%d\n", g_mt6336_hw_exist);

	return g_mt6336_hw_exist;
}

void mt6336_dump_register(void)
{
}

void mt6336_hw_init(void)
{
	/*MT6336LOG("[mt6336_hw_init] After HW init\n");*/
	mt6336_dump_register();
}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
unsigned char g_reg_value_mt6336;
static ssize_t show_mt6336_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	MT6336LOG("[show_mt6336_access] 0x%x\n", g_reg_value_mt6336);
	return sprintf(buf, "0x%x\n", g_reg_value_mt6336);
}

static ssize_t store_mt6336_access(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	MT6336LOG("[store_mt6336_access]\n");
	if (buf != NULL && size != 0) {
		MT6336LOG("[store_mt6336_access] size is %d, buf is %s\n", (int)size, buf);
		/*reg_address = simple_strtoul(buf, &pvalue, 16);*/

		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		val = strsep(&pvalue, " ");
		if (addr)
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		if (val) {
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			MT6336LOG("[store_mt6336_access] write MT6336 reg 0x%x with value 0x%x\n",
				reg_address, reg_value);
			mt6336_ctrl_enable(core_ctrl);
			ret = mt6336_config_interface(reg_address, reg_value, 0xFF, 0x0);
			mt6336_ctrl_disable(core_ctrl);
		} else {
			ret = mt6336_read_interface(reg_address, &g_reg_value_mt6336, 0xFF, 0x0);
			MT6336LOG("[store_mt6336_access] read MT6336 reg 0x%x with value 0x%x !\n",
				reg_address, g_reg_value_mt6336);
			MT6336LOG("[store_mt6336_access] use \"cat mt6336_access\" to get value\n");
		}
	}
	return size;
}

static DEVICE_ATTR(mt6336_access, 0664, show_mt6336_access, store_mt6336_access);	/* 664 */


/*****************************************************************************
 * HW Setting
 ******************************************************************************/
static int proc_dump_register_show(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "********** dump mt6336 registers**********\n");

	for (i = 0; i <= 0x0737; i = i + 10) {
		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i, mt6336_get_register_value(i), i + 1, mt6336_get_register_value(i + 1), i + 2,
			mt6336_get_register_value(i + 2), i + 3, mt6336_get_register_value(i + 3), i + 4,
			mt6336_get_register_value(i + 4));

		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i + 5, mt6336_get_register_value(i + 5), i + 6, mt6336_get_register_value(i + 6),
			i + 7, mt6336_get_register_value(i + 7), i + 8, mt6336_get_register_value(i + 8),
			i + 9, mt6336_get_register_value(i + 9));
	}

	return 0;
}

static int proc_dump_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dump_register_show, NULL);
}

static const struct file_operations mt6336_dump_register_proc_fops = {
	.open = proc_dump_register_open,
	.read = seq_read,
};

void mt6336_debug_init(void)
{
	struct proc_dir_entry *mt6336_dir;

	mt6336_dir = proc_mkdir("mt6336", NULL);
	if (!mt6336_dir) {
		MT6336LOG("fail to mkdir /proc/mt6336\n");
		return;
	}

	proc_create("dump_mt6336_reg", S_IRUGO | S_IWUSR, mt6336_dir, &mt6336_dump_register_proc_fops);
	MT6336LOG("proc_create pmic_dump_register_proc_fops\n");
}

/*****************************************************************************
 * system function
 ******************************************************************************/
static int mt6336_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	MT6336LOG("******** mt6336_user_space_probe!! ********\n");
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6336_access);
	MT6336LOG("[MT6336] device_create_file for EM : done.\n");

	mt6336_debug_init();
	MT6336LOG("[MT6336] mt6336_debug_init : done.\n");

	return 0;
}

struct platform_device mt6336_user_space_device = {
	.name = "mt6336-user",
	.id = -1,
};

static struct platform_driver mt6336_user_space_driver = {
	.probe = mt6336_user_space_probe,
	.driver = {
		   .name = "mt6336-user",
		   },
};

static int mt6336_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{


	pr_err(MT6336TAG "[mt6336_driver_probe]\n");
	new_client = client;

	core_ctrl = mt6336_ctrl_get("mt6336_core");
	/* Here will show Warning log since LK will enable MT6336 by hard-code, don't care */
	mt6336_ctrl_enable(core_ctrl);
	mt6336_hw_component_detect();
	mt6336_dump_register();

	/* Check MT6336 chip version */
	g_mt6336_hwcid = mt6336_get_flag_register_value(MT6336_HWCID);

	/*MT6336 Interrupt Service*/
	MT6336_EINT_SETTING();
	MT6336LOG("[MT6336_EINT_SETTING] Done\n");

	/* mt6336_hw_init(); //move to charging_hw_xxx.c */
	chargin_hw_init_done = true;
#if defined(CONFIG_MTK_SMART_BATTERY) && (CONFIG_MTK_GAUGE_VERSION != 30)
	/* Hook chr_control_interface with battery's interface */
	battery_charging_control = chr_control_interface;
#endif
	mt6336_ctrl_disable(core_ctrl);
	pr_err(MT6336TAG "[mt6336_driver_probe] Done\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt6336_of_match[] = {
	{.compatible = "mediatek,sw_charger"},
	{},
};
#else
static struct i2c_board_info i2c_mt6336 __initdata = {
	I2C_BOARD_INFO("mt6336", (mt6336_SLAVE_ADDR_WRITE >> 1))
};
#endif

static struct i2c_driver mt6336_driver = {
	.driver = {
		   .name = "mt6336",
#ifdef CONFIG_OF
		   .of_match_table = mt6336_of_match,
#endif
		   },
	.probe = mt6336_driver_probe,
	.id_table = mt6336_i2c_id,
};

/*****************************************************************************
 * MT6336 mudule init/exit
 ******************************************************************************/
static int __init mt6336_init(void)
{
	int ret = 0;

	pmic_init_wake_lock(&mt6336Thread_lock, "BatThread_lock_mt6336 wakelock");

	/* i2c registeration using DTS instead of boardinfo*/
#ifdef CONFIG_OF
	MT6336LOG("[mt6336_init] init start with i2c DTS");
#else
	MT6336LOG("[mt6336_init] init start. ch=%d\n", mt6336_BUSNUM);
	i2c_register_board_info(mt6336_BUSNUM, &i2c_mt6336, 1);
#endif
	if (i2c_add_driver(&mt6336_driver) != 0)
		MT6336LOG("[mt6336_init] failed to register mt6336 i2c driver.\n");
	else
		MT6336LOG("[mt6336_init] Success to register mt6336 i2c driver.\n");

	/* mt6336 user space access interface */
	ret = platform_device_register(&mt6336_user_space_device);
	if (ret) {
		MT6336LOG("****[mt6336_init] Unable to device register(%d)\n",
			    ret);
		return ret;
	}
	ret = platform_driver_register(&mt6336_user_space_driver);
	if (ret) {
		MT6336LOG("****[mt6336_init] Unable to register driver (%d)\n",
			    ret);
		return ret;
	}

	return 0;
}

static void __exit mt6336_exit(void)
{
	i2c_del_driver(&mt6336_driver);
}
module_init(mt6336_init);
module_exit(mt6336_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C MT6336 Driver");
MODULE_AUTHOR("Jeter Chen");
