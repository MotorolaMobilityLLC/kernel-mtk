/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>

#if defined CONFIG_MTK_LEGACY
#include <mt-plat/mt_gpio.h>
#endif
#include <mt-plat/mt_boot.h>
/*#include <mach/eint.h> TBD*/

#include <mt-plat/upmu_common.h>
#include "mt6311.h"

#include <mach/mt_pmic.h>
#include "pmic.h"

#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#if defined(CONFIG_MTK_FPGA)
#else
#if defined CONFIG_MTK_LEGACY
/*#include <cust_i2c.h> TBD*/
/*#include <cust_eint.h> TBD*/
#endif
#endif

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define mt6311_SLAVE_ADDR_WRITE   0xD6
#define mt6311_SLAVE_ADDR_Read    0xD7

#define I2C_EXT_BUCK_CHANNEL 5 /*TBD for bring up only, need to change to DTS*/

#ifdef I2C_EXT_BUCK_CHANNEL
#define mt6311_BUSNUM I2C_EXT_BUCK_CHANNEL
#else
#define mt6311_BUSNUM 5
#endif

static struct i2c_client *new_client;
static const struct i2c_device_id mt6311_i2c_id[] = { {"mt6311", 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id mt6311_of_ids[] = {
			{.compatible = "mediatek,vproc_buck"},
			{},
};
#endif

static int mt6311_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver mt6311_driver = {
	.driver = {
			.name = "mt6311",
#ifdef CONFIG_OF
			.of_match_table = mt6311_of_ids,
#endif
		   },
	.probe = mt6311_driver_probe,
	.id_table = mt6311_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(mt6311_i2c_access);
static DEFINE_MUTEX(mt6311_lock_mutex);

int g_mt6311_driver_ready = 0;
int g_mt6311_hw_exist = 0;

unsigned int g_mt6311_cid = 0;
unsigned int g_mt6311_logger;
unsigned int g_mt6311_dbgaddr;
unsigned char g_reg_value_mt6311 = 0;
/*
extern unsigned int upmu_get_reg_value(unsigned int reg);
extern void battery_oc_protect_reinit(void);
*/
/* #define PMICTAG                "[MT6311] "*/

/*----------------debug machanism-----------------------*/
unsigned int mt6311_logger_level_set(unsigned int level, unsigned int addr)
{
	g_mt6311_logger = level >= MT6311LOGLV ? level : MT6311LOGLV;
	g_mt6311_dbgaddr = addr;
	return 0;
}

static ssize_t mt6311_logger_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	char *info, *pdbg_lv, *paddr;
	unsigned int dbg_lv_value = 0;
	unsigned int reg_addr = 0;
	int ret = 0;

	info = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, size);

	if (copy_from_user(info, buf, size))
		return -EFAULT;

	info[size-1] = '\0';

	/* pr_err(PMICTAG "[logger_write] buf is %s, info is %s, size is %zu\n", buf, info, size); */

	if (size != 0) {
		if (size > 2) {
			pdbg_lv = strsep(&info, " ");
			/* pr_err(PMICTAG "[logger_write] pdbg_lv is %s, info is %s\n", pdbg_lv, info); */
			ret = kstrtou32(pdbg_lv, 16, (unsigned int *)&dbg_lv_value);
		}

		if (size > 2) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16);*/
			/*pvalue = (char *)buf + 1;*/
			paddr = strsep(&info, " ");
			/* pr_err(PMICTAG "[logger_write] paddr is %s, info is %s\n", paddr, info); */
			ret = kstrtou32(paddr, 16, (unsigned int *)&reg_addr);
		}
	}

	pr_err(PMICTAG "[logger_write] dbg_lv_value = %d, addr = 0x%x\n", dbg_lv_value, reg_addr);

	mt6311_logger_level_set(dbg_lv_value, reg_addr);

	pr_err(PMICTAG "mt6311_logger = %d\n", g_mt6311_logger);
	pr_err(PMICTAG "mt6311_dbg_addr = 0x%x\n", g_mt6311_dbgaddr);

	kfree(info);

	return size;
}

static ssize_t mt6311_access_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	char *info, *addr = NULL, *val = NULL;
	unsigned int reg_value = 0;
	unsigned int reg_addr = 0;
	int ret = 0;

	info = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, size);

	if (copy_from_user(info, buf, size))
		return -EFAULT;

	info[size-1] = '\0';

	/* pr_err(PMICTAG "[access_write] buf is %s, info is %s, size is %zu\n", buf, info, size); */

	if (size != 0) {
		if (size > 5) {
			addr = strsep(&info, " ");
			/* pr_err(PMICTAG "[access_write] paddr is %s, info is %s\n", addr, info); */
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_addr);
		} else
			ret = kstrtou32(info, 16, (unsigned int *)&reg_addr);

		if (size > 5) {
			val = strsep(&info, " ");
			/* pr_err(PMICTAG "[access_write] pvalue is %s, info is %s\n", val, info); */
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			ret = mt6311_config_interface(reg_addr, reg_value, 0xFF, 0x0);
			g_reg_value_mt6311 = reg_value;
		} else {
			ret = mt6311_read_interface(reg_addr, &g_reg_value_mt6311, 0xFF, 0x0);

			pr_err(PMICTAG "[access_write] read mt6311 reg 0x%x with value 0x%x !\n",
				reg_addr, g_reg_value_mt6311);
		}
	}
	pr_err(PMICTAG "[access_write] addr = 0x%x, value = 0x%x\n", reg_addr, reg_value);
	pr_err(PMICTAG "[store_mt6311_access] use \"cat mt6311_access\"\r\n");

	kfree(info);

	return size;
}

static ssize_t mt6311_dump_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	mt6311_dump_register();
	return size;
}

static ssize_t mt6311_buck_ut_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	char *info, *pmode, *penable, *pvoltage;
	unsigned int mode_value = 0;
	unsigned int enable_value = 0;
	int voltage_value = 0;
	int ret = 0;

	info = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, size);

	if (copy_from_user(info, buf, size))
		return -EFAULT;

	info[size-1] = '\0';

	/* pr_err(PMICTAG "[logger_write] buf is %s, info is %s, size is %zu\n", buf, info, size); */

	if (size != 0) {
		if (size > 3) {
			pmode = strsep(&info, " ");
			/* pr_err(PMICTAG "[logger_write] pdbg_lv is %s, info is %s\n", pdbg_lv, info); */
			ret = kstrtou32(pmode, 16, (unsigned int *)&mode_value);
		}

		if (size > 3) {
			penable = strsep(&info, " ");
			/* pr_err(PMICTAG "[logger_write] pdbg_lv is %s, info is %s\n", pdbg_lv, info); */
			ret = kstrtou32(penable, 16, (unsigned int *)&enable_value);
		}

		if (size > 3) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16);*/
			/*pvalue = (char *)buf + 1;*/
			pvoltage = strsep(&info, " ");
			/* pr_err(PMICTAG "[logger_write] paddr is %s, info is %s\n", paddr, info); */
			ret = kstrtoint(pvoltage, 10, (int *)&voltage_value);
		}
	}

	pr_err(PMICTAG "[mt6311_buck_ut] mode = %d, enable = %d, volt = %d\n", mode_value, enable_value, voltage_value);

	pr_err("mt6311_set_voltage ret::%x\n", mt6311_set_voltage(VPROC, voltage_value));
	pr_err("mt6311_enable ret:%d\n", mt6311_enable(VPROC, enable_value));
	pr_err("mt6311_is_enabled ret:%d\n", mt6311_is_enabled(VPROC));
	pr_err("mt6311_set_mode, ret:%d\n", mt6311_set_mode(VPROC, mode_value));
	pr_err("mt6311_get_voltage:0x%x\n", mt6311_get_voltage(VPROC));

	kfree(info);

	return size;
}
static int mt6311_logger_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "usage: echo dbg_lv addr > mt6311_logger\n");
	seq_puts(s, "     dvb_lv: 1-7, default=4\n");
	seq_puts(s, "     addr: addr need to monitor\n");
	seq_printf(s, "mt6311_logger  = %d\n", g_mt6311_logger);
	seq_printf(s, "mt6311_dbg_addr = 0x%x\n", g_mt6311_dbgaddr);
	return 0;
}

static int mt6311_logger_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt6311_logger_show, NULL);
}

static int mt6311_dump_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "usage: echo > mt6311_dump_all_reg\n");
	return 0;
}

static int mt6311_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt6311_dump_show, NULL);
}

static int mt6311_access_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "echo addr value > mt6311_access\n");
	seq_puts(s, "     addr: hex mode, not need 0x\n");
	seq_puts(s, "     value: hex mode, not need 0x\n");
	seq_puts(s, "usage: echo 8d 34 > mt6311_access. write 0x8d to 0x34\n");
	seq_puts(s, "echo addr > mt6311_access\n");
	seq_puts(s, "usage: echo 8d > mt6311_access. read 0x8d to 0x34\n");
	seq_printf(s, "[show_mt6311_access] 0x%x\n", g_reg_value_mt6311);
	return 0;
}

static int mt6311_buck_ut_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "echo mode enable voltage > mt6311_buck_ut\n");
	seq_puts(s, "     mode: 1:force pwm, 0:auto\n");
	seq_puts(s, "     enable 1:sw enable, 0:sw disable\n");
	seq_puts(s, "     voltage: expect voltage uV\n");
	return 0;
}

static int mt6311_access_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt6311_access_show, NULL);
}

static int mt6311_buck_ut_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt6311_buck_ut_show, NULL);
}

static const struct file_operations mt6311_logger_op = {
	.open    = mt6311_logger_open,
	.read    = seq_read,
	.write   = mt6311_logger_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt6311_dump_op = {
	.open    = mt6311_dump_open,
	.read    = seq_read,
	.write   = mt6311_dump_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt6311_access_op = {
	.open    = mt6311_access_open,
	.read    = seq_read,
	.write   = mt6311_access_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static const struct file_operations mt6311_buck_ut_op = {
	.open    = mt6311_buck_ut_open,
	.read    = seq_read,
	.write   = mt6311_buck_ut_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init mt6311_debugfs_init(void)
{
	if (mtk_extbuck != NULL) {
		debugfs_create_file("mt6311_logger", S_IFREG | S_IRUGO, mtk_extbuck, NULL, &mt6311_logger_op);
		debugfs_create_file("mt6311_dump_all_reg", S_IFREG | S_IRUGO, mtk_extbuck, NULL, &mt6311_dump_op);
		debugfs_create_file("mt6311_access", S_IFREG | S_IRUGO, mtk_extbuck, NULL, &mt6311_access_op);
		debugfs_create_file("mt6311_buck_ut", S_IFREG | S_IRUGO, mtk_extbuck, NULL, &mt6311_buck_ut_op);
	} else
		pr_err(PMICTAG "reference mtk_extbuck fail\n");
	return 0;
}

subsys_initcall(mt6311_debugfs_init);
/*----------------debug machanism-----------------------*/
/**********************************************************
  *
  *   [I2C Function For Read/Write mt6311]
  *
  *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int mt6311_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&mt6311_i2c_access);
#if 1
	new_client->ext_flag =
	    ((new_client->
	      ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_PUSHPULL_FLAG | I2C_HS_FLAG;
	new_client->timing = 3400;
#else
	new_client->ext_flag =
	    ((new_client->
	      ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_PUSHPULL_FLAG;
	new_client->timing = 100;
#endif

	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		pr_err(PMICTAG "[mt6311_read_byte] ret=%d\n", ret);

		new_client->ext_flag = 0;
		mutex_unlock(&mt6311_i2c_access);
		return ret;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	new_client->ext_flag = 0;

	mutex_unlock(&mt6311_i2c_access);
	return 1;
}

unsigned int mt6311_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&mt6311_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

#if 1
	new_client->ext_flag =
	    ((new_client->
	      ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG | I2C_PUSHPULL_FLAG | I2C_HS_FLAG;
	new_client->timing = 3400;
#else
	new_client->ext_flag =
	    ((new_client->
	      ext_flag) & I2C_MASK_FLAG) | I2C_PUSHPULL_FLAG;
	new_client->timing = 100;
#endif

	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		pr_err(PMICTAG "[mt6311_write_byte] ret=%d\n", ret);
		pr_err(PMICTAG "skipping non-existent adapter %s\n", new_client->adapter->name);

		new_client->ext_flag = 0;
		mutex_unlock(&mt6311_i2c_access);
		return ret;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&mt6311_i2c_access);
	return 1;
}
#else
unsigned int mt6311_read_byte(unsigned char cmd, unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&mt6311_i2c_access);

	do {
		struct i2c_msg msgs[2] = {
			{
			 .addr = new_client->addr,
			 .flags = 0,
			 .len = 1,
			 .buf = &cmd,
			 }, {

			     .addr = new_client->addr,
			     .flags = I2C_M_RD,
			     .len = 1,
			     .buf = returnData,
			     }
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret < 0) {
			pr_err(PMICTAG "skipping non-existent adapter %s\n", new_client->adapter->name);
			pr_err(PMICTAG "i2c_transfer error: (%d) %d\n", cmd, ret);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&mt6311_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int mt6311_write_byte(unsigned char cmd, unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];


	mutex_lock(&mt6311_i2c_access);

	buf[0] = cmd;
	memcpy(&buf[1], &writeData, 1);

	do {
		struct i2c_msg msgs[1] = {
			{
			 .addr = new_client->addr,
			 .flags = 0,
			 .len = 1 + 1,
			 .buf = buf,
			 },
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret < 0) {
			pr_err(PMICTAG "skipping non-existent adapter %s\n", new_client->adapter->name);
			pr_err(PMICTAG "i2c_transfer error: (%d) %d\n", cmd, ret);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&mt6311_i2c_access);

	return ret == xfers ? 1 : -1;
}

#endif
/*
 *   [Read / Write Function]
 */
unsigned int mt6311_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK, unsigned char SHIFT)
{
#if 0
	PMICLOG("[mt6311_read_interface] HW no mt6311\n");
	*val = 0;
	return 1;
#else
	unsigned char mt6311_reg = 0;
	unsigned int ret = 0;

	/* PMICLOG("--------------------------------------------------\n"); */

	ret = mt6311_read_byte(RegNum, &mt6311_reg);

	/* PMICLOG("[mt6311_read_interface] Reg[%x]=0x%x\n", RegNum, mt6311_reg); */

	mt6311_reg &= (MASK << SHIFT);
	*val = (mt6311_reg >> SHIFT);

	/* PMICLOG("[mt6311_read_interface] val=0x%x\n", *val); */

	return ret;
#endif
}

unsigned int mt6311_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK, unsigned char SHIFT)
{
#if 0
	PMICLOG("[mt6311_config_interface] HW no mt6311\n");
	return 1;
#else
	unsigned char mt6311_reg = 0;
	unsigned int ret = 0;

	/*PMICLOG("--------------------------------------------------\n"); */

	ret = mt6311_read_byte(RegNum, &mt6311_reg);
	/* PMICLOG("[mt6311_config_interface] Reg[%x]=0x%x\n", RegNum, mt6311_reg);*/

	mt6311_reg &= ~(MASK << SHIFT);
	mt6311_reg |= (val << SHIFT);

	MT6311LOGGER(RegNum, mt6311_reg, val, MASK, SHIFT);

	ret = mt6311_write_byte(RegNum, mt6311_reg);
	/*PMICLOG("[mt6311_config_interface] write Reg[%x]=0x%x\n", RegNum, mt6311_reg);*/

	/* Check*/
	/*ret = mt6311_read_byte(RegNum, &mt6311_reg);
	PMICLOG("[mt6311_config_interface] Check Reg[%x]=0x%x\n", RegNum, mt6311_reg);
	*/

	return ret;
#endif
}

void mt6311_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = mt6311_config_interface((unsigned char) reg, (unsigned char) reg_val, 0xFF, 0x0);
}

unsigned int mt6311_get_reg_value(unsigned int reg)
{
	unsigned int ret = 0;
	unsigned char reg_val = 0;

	ret = mt6311_read_interface((unsigned char) reg, &reg_val, 0xFF, 0x0);

	return reg_val;
}


/*
 *   mt6311 special API
 */
unsigned char mt6311_get_register_value(mt6311_flag pmureg)
{
	unsigned char val;
	unsigned int ret;

	ret = mt6311_read_interface(pmureg.addr, &val, pmureg.mask, pmureg.shift);

	return val;
}

unsigned int mt6311_set_register_value(mt6311_flag pmureg, unsigned int val)
{
	unsigned int ret;

	ret = mt6311_config_interface(pmureg.addr, val, pmureg.mask, pmureg.shift);

	return ret;
}

/*
 *   mt6311 buck control start
 */
struct mt6311_bucks_t mt6311_bucks_class[] = {
	MT6311_BUCK_GEN1(VPROC,
			MT6311_VDVFS11_CON9, MT6311_VDVFS11_CON7,
			MT6311_VDVFS1_ANA_CON2, MT6311_VDVFS11_CON12,
			MT6311_VDVFS11_CON13, MT6311_VDVFS11_CON14,
			MT6311_VDVFS11_CON9, MT6311_VDVFS11_CON15,
			600000, 1350000, 6250),
};

static unsigned int mt6311_bucks_size = ARRAY_SIZE(mt6311_bucks_class);

unsigned char mt6311_is_enabled(MT6311_BUCK_TYPE type)
{
	if (type >= mt6311_bucks_size) {
		pr_err("[MT6311] Set Wrong buck type\n");
		return -1;
	}

	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8a, mt6311_get_reg_value(0x8a));
	return mt6311_get_qi_vdvfs11_en();

}
/* en = 1 enable */
/* en = 0 disable */
unsigned char mt6311_enable(MT6311_BUCK_TYPE type, unsigned char en)
{
	if (type >= mt6311_bucks_size) {
		pr_err("[MT6311] Set Wrong buck type\n");
		WARN_ON(1);
		return -1;
	}

	if (en > 1) {
		pr_err("[MT6311] Set en > 1, only 0 or 1!!!!\n");
		return -1;
	}

	/*--EN SW CTRL--*/
	mt6311_set_vdvfs11_en_ctrl(0x0);
	/*--EN SW --*/
	mt6311_set_vdvfs11_en(en);

	if (mt6311_is_enabled(type) == en)
		PMICLOG("%s Set en pass: 0x%x\n", mt6311_bucks_class[type].name, en);
	else
		PMICLOG("%s Set en fail: 0x%x\n", mt6311_bucks_class[type].name, en);

	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x88, mt6311_get_reg_value(0x88));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8d, mt6311_get_reg_value(0x8d));
	/*--return mt6311_get_register_value(mt6311_bucks_class[type].da_qi_en);--*/
	return 0;

}
/* pmode = 1 force PWM mode */
/* pmode = 0 auto mode      */
unsigned char mt6311_set_mode(MT6311_BUCK_TYPE type, unsigned char pmode)
{
	if (type >= mt6311_bucks_size) {
		pr_err("[MT6311] Set Wrong buck type\n");
		WARN_ON(1);
		return -1;
	}

	if (pmode > 1) {
		pr_err("[MT6311] Set mode > 1, only 0 or 1!!!!\n");
		return -1;
	}

	/*--MODESET--*/
	mt6311_set_rg_vdvfs11_modeset(pmode);

	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x7c, mt6311_get_reg_value(0x7c));
	return 0;
}

unsigned char mt6311_set_voltage(MT6311_BUCK_TYPE type, int voltage)
{
	int value = 0;

	if (type >= mt6311_bucks_size) {
		pr_err("[MT6311] Set Wrong buck type\n");
		return -1;
	}

	if ((voltage < mt6311_bucks_class[type].min_uV) || (voltage > mt6311_bucks_class[type].max_uV)) {
		pr_err("unsupportable voltage range: %d-%duV set_volt:%d\n", mt6311_bucks_class[type].min_uV,
			mt6311_bucks_class[type].max_uV, voltage);
		return -1;
	}

	value = (voltage - mt6311_bucks_class[type].min_uV)/(mt6311_bucks_class[type].uV_step);

	pr_err("%s Expected voltage: 0x%x\n", mt6311_bucks_class[type].name, value);

	/*--VOSEL SW CTRL--*/
	mt6311_set_vdvfs11_vosel(value);
	/*--VOSEL HW CTRL--*/
	mt6311_set_vdvfs11_vosel_on(value);

	pr_err("[mt6311] [0x%x]=0x%x\n", 0x8d, mt6311_get_reg_value(0x8d));
	pr_err("[mt6311] [0x%x]=0x%x\n", 0x8e, mt6311_get_reg_value(0x8e));
	return 0;
}

unsigned char mt6311_get_voltage(MT6311_BUCK_TYPE type)
{
	if (type >= mt6311_bucks_size) {
		pr_err("Get Wrong buck type\n");
		return 0;
	}

	pr_err("[mt6311] [0x%x]=0x%x\n", 0x90, mt6311_get_reg_value(0x90));
	return mt6311_get_ni_vdvfs11_vosel();
}
/*-----mt6311 buck control end---*/

/*
 *   [APIs]
 */
void mt6311_lock(void)
{
	mutex_lock(&mt6311_lock_mutex);
}

void mt6311_unlock(void)
{
	mutex_unlock(&mt6311_lock_mutex);
}

unsigned char mt6311_get_cid(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_CID),
				    (&val),
				    (unsigned char) (MT6311_PMIC_CID_MASK),
				    (unsigned char) (MT6311_PMIC_CID_SHIFT)
	    );
	if (ret < 0) {
		pr_err(PMICTAG "[mt6311_get_cid] ret=%d\n", ret);
		mt6311_unlock();
		return ret;
	}
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_swcid(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_SWCID),
				    (&val),
				    (unsigned char) (MT6311_PMIC_SWCID_MASK),
				    (unsigned char) (MT6311_PMIC_SWCID_SHIFT)
	    );
	if (ret < 0) {
		pr_err(PMICTAG "[mt6311_get_swcid] ret=%d\n", ret);
		mt6311_unlock();
		return ret;
	}
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_hwcid(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_HWCID),
				    (&val),
				    (unsigned char) (MT6311_PMIC_HWCID_MASK),
				    (unsigned char) (MT6311_PMIC_HWCID_SHIFT)
	    );
	if (ret < 0) {
		pr_err(PMICTAG "[mt6311_get_hwcid] ret=%d\n", ret);
		mt6311_unlock();
		return ret;
	}
	mt6311_unlock();

	return val;
}

void mt6311_set_gpio0_dir(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO0_DIR_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO0_DIR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio1_dir(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO1_DIR_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO1_DIR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio0_dinv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO0_DINV_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO0_DINV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio1_dinv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO1_DINV_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO1_DINV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio0_dout(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO0_DOUT_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO0_DOUT_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio1_dout(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_CFG),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO1_DOUT_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO1_DOUT_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_gpio0_din(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_GPIO_CFG),
				    (&val),
				    (unsigned char) (MT6311_PMIC_GPIO0_DIN_MASK),
				    (unsigned char) (MT6311_PMIC_GPIO0_DIN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_gpio1_din(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_GPIO_CFG),
				    (&val),
				    (unsigned char) (MT6311_PMIC_GPIO1_DIN_MASK),
				    (unsigned char) (MT6311_PMIC_GPIO1_DIN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_gpio0_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_MODE),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO0_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO0_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_gpio1_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_GPIO_MODE),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_GPIO1_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_GPIO1_MODE_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_test_out(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TEST_OUT),
				    (&val),
				    (unsigned char) (MT6311_PMIC_TEST_OUT_MASK),
				    (unsigned char) (MT6311_PMIC_TEST_OUT_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_mon_grp_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TEST_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_MON_GRP_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_MON_GRP_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_mon_flag_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TEST_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_MON_FLAG_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_MON_FLAG_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_dig_testmode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TEST_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_DIG_TESTMODE_MASK),
				      (unsigned char) (MT6311_PMIC_DIG_TESTMODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_pmu_testmode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TEST_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_PMU_TESTMODE_MASK),
				      (unsigned char) (MT6311_PMIC_PMU_TESTMODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_srclken_in_hw_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_HW_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_HW_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_srclken_in_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_lp_hw_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_LP_HW_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_LP_HW_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_lp_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_LP_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_LP_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_osc_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_OSC_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_OSC_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_osc_en_hw_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_OSC_EN_HW_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_OSC_EN_HW_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_srclken_in_sync_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_SYNC_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SRCLKEN_IN_SYNC_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_rsv_hw_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_RSV_HW_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_RSV_HW_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_ref_ck_tstsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_REF_CK_TSTSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_REF_CK_TSTSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_fqmtr_ck_tstsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_CK_TSTSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_CK_TSTSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_smps_ck_tstsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SMPS_CK_TSTSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SMPS_CK_TSTSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_pmu75k_ck_tstsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_PMU75K_CK_TSTSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_PMU75K_CK_TSTSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_smps_ck_tst_dis(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SMPS_CK_TST_DIS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SMPS_CK_TST_DIS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_pmu75k_ck_tst_dis(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_PMU75K_CK_TST_DIS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_PMU75K_CK_TST_DIS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_ana_auto_off_dis(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKTST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_ANA_AUTO_OFF_DIS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_ANA_AUTO_OFF_DIS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_ref_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_REF_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_REF_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_1m_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_1M_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_1M_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_intrp_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_INTRP_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_INTRP_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_75k_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_75K_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_75K_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_ana_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_ANA_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_ANA_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_trim_75k_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_TRIM_75K_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_TRIM_75K_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_auxadc_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_auxadc_1m_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_1M_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_1M_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_stb_75k_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STB_75K_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STB_75K_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_fqmtr_ck_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_CK_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_CK_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_top_ckpdn_con2_rsv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKPDN_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_TOP_CKPDN_CON2_RSV_MASK),
				      (unsigned char) (MT6311_PMIC_TOP_CKPDN_CON2_RSV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_buck_1m_ck_pdn_hwen(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKHWEN_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_1M_CK_PDN_HWEN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BUCK_1M_CK_PDN_HWEN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_ck_pdn_hwen(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CKHWEN_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_CK_PDN_HWEN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_CK_PDN_HWEN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_auxadc_rst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_RST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_AUXADC_RST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_fqmtr_rst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_RST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_FQMTR_RST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_clk_trim_rst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_CLK_TRIM_RST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_CLK_TRIM_RST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_man_rst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_MAN_RST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_MAN_RST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_wdtrstb_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_WDTRSTB_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_WDTRSTB_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_wdtrstb_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_WDTRSTB_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_WDTRSTB_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_wdtrstb_status_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_RST_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_WDTRSTB_STATUS_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_WDTRSTB_STATUS_CLR_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_wdtrstb_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_RST_CON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_WDTRSTB_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_WDTRSTB_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_int_pol(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_INT_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_INT_POL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_INT_POL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_int_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_INT_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_INT_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_INT_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_i2c_config(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_INT_CON),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_I2C_CONFIG_MASK),
				      (unsigned char) (MT6311_PMIC_I2C_CONFIG_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rg_lbat_min_int_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_INT_MON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_LBAT_MIN_INT_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_LBAT_MIN_INT_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_lbat_max_int_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_INT_MON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_LBAT_MAX_INT_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_LBAT_MAX_INT_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_thr_l_int_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_INT_MON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_THR_L_INT_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_THR_L_INT_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_thr_h_int_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_INT_MON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_THR_H_INT_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_THR_H_INT_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_buck_oc_int_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_INT_MON),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_BUCK_OC_INT_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_BUCK_OC_INT_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_thr_det_dis(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_THR_DET_DIS_MASK),
				      (unsigned char) (MT6311_PMIC_THR_DET_DIS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_thr_hwpdn_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_THR_HWPDN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_THR_HWPDN_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_dig0_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG0_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG0_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_usbdl_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_USBDL_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_USBDL_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_test_strup(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_TEST_STRUP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_TEST_STRUP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_test_strup_thr_in(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_TEST_STRUP_THR_IN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_TEST_STRUP_THR_IN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_dig1_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG1_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG1_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_thr_test(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_THR_TEST_MASK),
				      (unsigned char) (MT6311_PMIC_THR_TEST_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_pmu_thr_deb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_PMU_THR_DEB_MASK),
				    (unsigned char) (MT6311_PMIC_PMU_THR_DEB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_pmu_thr_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_PMU_THR_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_PMU_THR_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_strup_pwron(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_pwron_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_bias_gen_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_bias_gen_en_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rtc_xosc32_enb_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RTC_XOSC32_ENB_SW_MASK),
				      (unsigned char) (MT6311_PMIC_RTC_XOSC32_ENB_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rtc_xosc32_enb_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RTC_XOSC32_ENB_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RTC_XOSC32_ENB_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_dig_io_pg_force(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG_IO_PG_FORCE_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_DIG_IO_PG_FORCE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_dduvlo_deb_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_DDUVLO_DEB_EN_MASK),
				      (unsigned char) (MT6311_PMIC_DDUVLO_DEB_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_pwrbb_deb_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_PWRBB_DEB_EN_MASK),
				      (unsigned char) (MT6311_PMIC_PWRBB_DEB_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_osc_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_OSC_EN_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_OSC_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_osc_en_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_OSC_EN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_OSC_EN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_ft_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_FT_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_FT_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_pwron_force(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_FORCE_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PWRON_FORCE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_bias_gen_en_force(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_FORCE_MASK),
				      (unsigned char) (MT6311_PMIC_BIAS_GEN_EN_FORCE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_pg_h2l_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_PG_H2L_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_PG_H2L_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_pg_h2l_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_PG_H2L_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_PG_H2L_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vbiasn_pg_h2l_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VBIASN_PG_H2L_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VBIASN_PG_H2L_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_pg_enb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_PG_ENB_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_PG_ENB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_pg_enb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_PG_ENB_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_PG_ENB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vbiasn_pg_enb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VBIASN_PG_ENB_MASK),
				      (unsigned char) (MT6311_PMIC_VBIASN_PG_ENB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_ext_pmic_en_pg_enb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EXT_PMIC_EN_PG_ENB_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EXT_PMIC_EN_PG_ENB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_pre_pwron_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_PRE_PWRON_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_PRE_PWRON_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_pre_pwron_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_PRE_PWRON_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_PRE_PWRON_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_clr_just_rst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_CLR_JUST_RST_MASK),
				      (unsigned char) (MT6311_PMIC_CLR_JUST_RST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_uvlo_l2h_deb_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_UVLO_L2H_DEB_EN_MASK),
				      (unsigned char) (MT6311_PMIC_UVLO_L2H_DEB_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_test_ckin_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_CKIN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_CKIN_EN_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_osc_en(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON7),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_OSC_EN_MASK),
				    (unsigned char) (MT6311_PMIC_QI_OSC_EN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_strup_pmu_pwron_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PMU_PWRON_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PMU_PWRON_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_pmu_pwron_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PMU_PWRON_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PMU_PWRON_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_auxadc_start_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_START_SW_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_START_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_auxadc_rstb_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_RSTB_SW_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_RSTB_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_auxadc_start_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_START_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_START_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_auxadc_rstb_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_RSTB_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_AUXADC_RSTB_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_pwroff_preoff_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PWROFF_PREOFF_EN_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PWROFF_PREOFF_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_strup_pwroff_seq_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PWROFF_SEQ_EN_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PWROFF_SEQ_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_sys_latch_en_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SYS_LATCH_EN_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SYS_LATCH_EN_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_sys_latch_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SYS_LATCH_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SYS_LATCH_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_onoff_en_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_ONOFF_EN_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_ONOFF_EN_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_onoff_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_ONOFF_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_ONOFF_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_pwron_cond_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PWRON_COND_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PWRON_COND_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_pwron_cond_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PWRON_COND_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_PWRON_COND_EN_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_strup_pg_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON11),
				    (&val),
				    (unsigned char) (MT6311_PMIC_STRUP_PG_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_STRUP_PG_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_strup_pg_status_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_STRUP_PG_STATUS_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_STRUP_PG_STATUS_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_rsv_swreg(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_RSV_SWREG_MASK),
				      (unsigned char) (MT6311_PMIC_RG_RSV_SWREG_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_vdvfs11_pg_deb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON13),
				    (&val),
				    (unsigned char) (MT6311_PMIC_VDVFS11_PG_DEB_MASK),
				    (unsigned char) (MT6311_PMIC_VDVFS11_PG_DEB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_vdvfs12_pg_deb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON13),
				    (&val),
				    (unsigned char) (MT6311_PMIC_VDVFS12_PG_DEB_MASK),
				    (unsigned char) (MT6311_PMIC_VDVFS12_PG_DEB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_vbiasn_pg_deb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON13),
				    (&val),
				    (unsigned char) (MT6311_PMIC_VBIASN_PG_DEB_MASK),
				    (unsigned char) (MT6311_PMIC_VBIASN_PG_DEB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_strup_ro_rsv0(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON13),
				    (&val),
				    (unsigned char) (MT6311_PMIC_STRUP_RO_RSV0_MASK),
				    (unsigned char) (MT6311_PMIC_STRUP_RO_RSV0_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_strup_thr_110_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_thr_125_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_thr_110_irq_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_IRQ_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_IRQ_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_thr_125_irq_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_IRQ_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_IRQ_EN_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rg_strup_thr_110_irq_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON14),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_IRQ_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_STRUP_THR_110_IRQ_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_strup_thr_125_irq_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_STRUP_CON14),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_IRQ_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RG_STRUP_THR_125_IRQ_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_thermal_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_THERMAL_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_THERMAL_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_thermal_en_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_THERMAL_EN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_THERMAL_EN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rg_osc_75k_trim(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_TOP_CLK_TRIM0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_MASK),
				    (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_osc_75k_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CLK_TRIM1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_OSC_75K_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_OSC_75K_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_osc_75k_trim_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CLK_TRIM1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_osc_75k_trim_rate(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_TOP_CLK_TRIM1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_RATE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_OSC_75K_TRIM_RATE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_addr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_ADDR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_ADDR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_din(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DIN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DIN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_dm(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_pgm(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_pgm_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_prog_pkey(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PROG_PKEY_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PROG_PKEY_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_rd_pkey(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_PKEY_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_PKEY_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_pgm_src(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_SRC_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_PGM_SRC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_din_src(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DIN_SRC_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_DIN_SRC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_rd_trig(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_TRIG_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_TRIG_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_rd_rdy_bypass(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_RD_RDY_BYPASS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_RD_RDY_BYPASS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_skip_efuse_out(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_SKIP_EFUSE_OUT_MASK),
				      (unsigned char) (MT6311_PMIC_RG_SKIP_EFUSE_OUT_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rg_efuse_rd_ack(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_CON12),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_ACK_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_ACK_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_rd_busy(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_CON12),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_BUSY_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_RD_BUSY_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_efuse_write_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_WRITE_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_WRITE_MODE_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rg_efuse_dout_0_7(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_0_7),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_0_7_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_0_7_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_8_15(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_8_15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_8_15_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_8_15_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_16_23(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_16_23),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_16_23_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_16_23_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_24_31(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_24_31),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_24_31_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_24_31_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_32_39(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_32_39),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_32_39_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_32_39_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_40_47(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_40_47),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_40_47_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_40_47_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_48_55(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_48_55),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_48_55_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_48_55_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_56_63(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_56_63),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_56_63_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_56_63_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_64_71(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_64_71),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_64_71_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_64_71_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_72_79(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_72_79),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_72_79_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_72_79_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_80_87(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_80_87),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_80_87_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_80_87_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_88_95(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_88_95),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_88_95_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_88_95_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_96_103(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_96_103),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_96_103_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_96_103_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_104_111(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_104_111),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_104_111_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_104_111_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_112_119(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_112_119),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_112_119_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_112_119_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rg_efuse_dout_120_127(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_EFUSE_DOUT_120_127),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_120_127_MASK),
				    (unsigned char) (MT6311_PMIC_RG_EFUSE_DOUT_120_127_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_efuse_val_0_7(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_0_7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_0_7_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_0_7_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_8_15(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_8_15),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_8_15_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_8_15_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_16_23(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_16_23),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_16_23_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_16_23_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_24_31(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_24_31),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_24_31_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_24_31_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_32_39(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_32_39),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_32_39_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_32_39_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_40_47(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_40_47),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_40_47_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_40_47_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_48_55(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_48_55),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_48_55_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_48_55_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_56_63(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_56_63),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_56_63_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_56_63_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_64_71(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_64_71),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_64_71_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_64_71_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_72_79(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_72_79),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_72_79_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_72_79_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_80_87(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_80_87),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_80_87_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_80_87_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_88_95(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_88_95),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_88_95_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_88_95_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_96_103(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_96_103),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_96_103_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_96_103_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_104_111(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_104_111),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_104_111_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_104_111_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_112_119(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_112_119),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_112_119_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_112_119_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_efuse_val_120_127(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_EFUSE_VAL_120_127),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_120_127_MASK),
				      (unsigned char) (MT6311_PMIC_RG_EFUSE_VAL_120_127_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_dig0_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_DIG0_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_DIG0_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vsleep_src0_8(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC0_8_MASK),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC0_8_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vsleep_src1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC1_MASK),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vsleep_src0_7_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC0_7_0_MASK),
				      (unsigned char) (MT6311_PMIC_VSLEEP_SRC0_7_0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_r2r_src0_8(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_R2R_SRC0_8_MASK),
				      (unsigned char) (MT6311_PMIC_R2R_SRC0_8_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_r2r_src1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_R2R_SRC1_MASK),
				      (unsigned char) (MT6311_PMIC_R2R_SRC1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_r2r_src0_7_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_R2R_SRC0_7_0_MASK),
				      (unsigned char) (MT6311_PMIC_R2R_SRC0_7_0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_osc_sel_src0_8(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_OSC_SEL_SRC0_8_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_OSC_SEL_SRC0_8_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_srclken_dly_src1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_SRCLKEN_DLY_SRC1_MASK),
				      (unsigned char) (MT6311_PMIC_SRCLKEN_DLY_SRC1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_osc_sel_src0_7_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_OSC_SEL_SRC0_7_0_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_OSC_SEL_SRC0_7_0_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vdvfs12_dig_mon(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_ALL_CON7),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_DIG_MON_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_DIG_MON_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vdvfs11_dig_mon(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_ALL_CON8),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_DIG_MON_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_DIG_MON_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_oc_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_deg_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_DEG_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_DEG_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_wnd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_WND_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_WND_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_thd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_THD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_THD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_deg_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_DEG_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_DEG_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_wnd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_WND_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_WND_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_thd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_THD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_THD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_flag_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_FLAG_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_FLAG_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_flag_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_FLAG_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_FLAG_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_rg_status_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_RG_STATUS_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_RG_STATUS_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_rg_status_clr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_RG_STATUS_CLR_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_RG_STATUS_CLR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_oc_flag_clr_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_FLAG_CLR_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_FLAG_CLR_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_flag_clr_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_FLAG_CLR_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_FLAG_CLR_SEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_vdvfs11_oc_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_ALL_CON20),
				    (&val),
				    (unsigned char) (MT6311_PMIC_VDVFS11_OC_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_VDVFS11_OC_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_vdvfs12_oc_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_ALL_CON20),
				    (&val),
				    (unsigned char) (MT6311_PMIC_VDVFS12_OC_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_VDVFS12_OC_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_oc_int_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON21),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_INT_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_OC_INT_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_oc_int_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON21),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_INT_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_OC_INT_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_en_oc_sdn_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON22),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_OC_SDN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_OC_SDN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_en_oc_sdn_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON22),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_OC_SDN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_OC_SDN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_test_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON23),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_TEST_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_TEST_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_dig1_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON23),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_DIG1_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_DIG1_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_qi_vdvfs11_vsleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON24),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_QI_VDVFS11_VSLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_QI_VDVFS11_VSLEEP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_qi_vdvfs12_vsleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_ALL_CON24),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_QI_VDVFS12_VSLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_QI_VDVFS12_VSLEEP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_buck_ana_dig0_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_ANA_RSV_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_BUCK_ANA_DIG0_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_BUCK_ANA_DIG0_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_thrdet_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_THRDET_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_THRDET_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_thr_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_THR_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_thr_tmode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_THR_TMODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_THR_TMODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_iref_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_IREF_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_IREF_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_uvlo_vthl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_UVLO_VTHL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_UVLO_VTHL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_uvlo_vthh(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_UVLO_VTHH_MASK),
				      (unsigned char) (MT6311_PMIC_RG_UVLO_VTHH_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_unchop(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_UNCHOP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_UNCHOP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_unchop_ph(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_UNCHOP_PH_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_UNCHOP_PH_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_rsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_RSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_RSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_test_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_test_rstb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_RSTB_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_TEST_RSTB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_trimh(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRIMH_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRIMH_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_triml(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRIML_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRIML_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_trimh(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRIMH_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRIMH_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_triml(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRIML_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRIML_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_vsleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_VSLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_VSLEEP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_vsleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_VSLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_VSLEEP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_bgr_osc_cal(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_BGR_OSC_CAL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_BGR_OSC_CAL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_strup_rsv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_RSV_MASK),
				      (unsigned char) (MT6311_PMIC_RG_STRUP_RSV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vref_lp_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VREF_LP_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VREF_LP_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_testmode_swen(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_TESTMODE_SWEN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_TESTMODE_SWEN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdig18_vosel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDIG18_VOSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDIG18_VOSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdig18_cal(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDIG18_CAL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDIG18_CAL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_osc_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_STRUP_ANA_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_OSC_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_OSC_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_ndis_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VBIASN_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_NDIS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_NDIS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_vosel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VBIASN_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_VOSEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_VOSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_rc(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_RC_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_RC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_rc(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_RC_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_RC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_csr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_csr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_pfm_csr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_PFM_CSR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_PFM_CSR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_pfm_csr(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_PFM_CSR_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_PFM_CSR_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_slp(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_SLP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_SLP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_slp(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_SLP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_SLP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_uvp_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_UVP_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_UVP_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_uvp_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_UVP_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_UVP_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_modeset(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_MODESET_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_MODESET_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_modeset(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_MODESET_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_MODESET_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_ndis_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_NDIS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_NDIS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_ndis_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_NDIS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_NDIS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_trans_bst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRANS_BST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TRANS_BST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_trans_bst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRANS_BST_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_TRANS_BST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_csm_n(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSM_N_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSM_N_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_csm_p(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSM_P_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_CSM_P_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_csm_n(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSM_N_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSM_N_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_csm_p(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSM_P_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_CSM_P_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_zxos_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_ZXOS_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_ZXOS_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_zxos_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_ZXOS_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_ZXOS_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_oc_off(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_OC_OFF_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_OC_OFF_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_oc_off(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_OC_OFF_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_OC_OFF_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_phs_shed_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_PHS_SHED_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_PHS_SHED_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_f2phs(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_F2PHS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_F2PHS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_rs_force_off(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_RS_FORCE_OFF_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_RS_FORCE_OFF_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs12_rs_force_off(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_RS_FORCE_OFF_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS12_RS_FORCE_OFF_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_tm_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TM_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TM_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs11_tm_ugsns(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TM_UGSNS_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS11_TM_UGSNS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vdvfs1_fbn_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS1_ANA_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS1_FBN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VDVFS1_FBN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_rgs_vdvfs11_enpwm_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS1_ANA_CON12),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RGS_VDVFS11_ENPWM_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RGS_VDVFS11_ENPWM_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_rgs_vdvfs12_enpwm_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS1_ANA_CON12),
				    (&val),
				    (unsigned char) (MT6311_PMIC_RGS_VDVFS12_ENPWM_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_RGS_VDVFS12_ENPWM_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_ni_vdvfs1_count(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS1_ANA_CON12),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS1_COUNT_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS1_COUNT_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_dig0_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_en_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_vosel_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_dig0_rsv1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV1_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_burst_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_en_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_vosel_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_dig0_rsv2(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV2_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_DIG0_RSV2_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_burst_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_EN_SHIFT)
	    );
	mt6311_unlock();
	udelay(200);
}

void mt6311_set_vdvfs11_stbtd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_STBTD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_STBTD_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vdvfs11_stb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_STB_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_STB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vdvfs11_en(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_EN_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_EN_SHIFT)
	    );

	return val;
}

unsigned char mt6311_get_qi_vdvfs11_oc_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_OC_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_OC_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_sfchg_rrate(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_RRATE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_RRATE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_sfchg_ren(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_REN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_REN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_sfchg_frate(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_FRATE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_FRATE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_sfchg_fen(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_FEN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_SFCHG_FEN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_vosel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_SHIFT)
	    );
	mt6311_unlock();
	udelay(200);
}

void mt6311_set_vdvfs11_vosel_on(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_ON_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_ON_SHIFT)
	    );
	mt6311_unlock();
	udelay(200);
}

void mt6311_set_vdvfs11_vosel_sleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_SLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VOSEL_SLEEP_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs11_vosel(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VOSEL_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VOSEL_SHIFT)
	    );

	return val;
}

void mt6311_set_vdvfs11_burst_sleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON16),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_SLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_SLEEP_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vdvfs11_burst(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON16),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_BURST_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS11_BURST_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_burst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON17),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_burst_on(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON17),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_ON_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_BURST_ON_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_vsleep_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VSLEEP_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VSLEEP_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_r2r_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_R2R_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_R2R_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_vsleep_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VSLEEP_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_VSLEEP_SEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs11_r2r_pdn(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON18),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_R2R_PDN_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_R2R_PDN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_ni_vdvfs11_vsleep_sel(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON18),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VSLEEP_SEL_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VSLEEP_SEL_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs11_trans_td(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_TD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_TD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_trans_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs11_trans_once(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS11_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_ONCE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS11_TRANS_ONCE_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs11_vosel_trans(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS11_CON19),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VOSEL_TRANS_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS11_VOSEL_TRANS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs12_dig0_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_en_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vosel_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_dig0_rsv1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV1_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_burst_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_en_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vosel_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_dig0_rsv2(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV2_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_DIG0_RSV2_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_burst_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_stbtd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_STBTD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_STBTD_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vdvfs12_stb(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_STB_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_STB_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vdvfs12_en(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_EN_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_EN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vdvfs12_oc_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON9),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_OC_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_OC_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs12_sfchg_rrate(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_RRATE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_RRATE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_sfchg_ren(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_REN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_REN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_sfchg_frate(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_FRATE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_FRATE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_sfchg_fen(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_FEN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_SFCHG_FEN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vosel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vosel_on(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_ON_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_ON_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vosel_sleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_SLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VOSEL_SLEEP_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs12_vosel(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VOSEL_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VOSEL_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs12_burst_sleep(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON16),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_SLEEP_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_SLEEP_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vdvfs12_burst(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON16),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_BURST_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VDVFS12_BURST_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs12_burst(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON17),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_burst_on(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON17),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_ON_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_BURST_ON_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vsleep_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VSLEEP_EN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VSLEEP_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_r2r_pdn(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_R2R_PDN_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_R2R_PDN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_vsleep_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VSLEEP_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_VSLEEP_SEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs12_r2r_pdn(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON18),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_R2R_PDN_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_R2R_PDN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_ni_vdvfs12_vsleep_sel(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON18),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VSLEEP_SEL_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VSLEEP_SEL_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_vdvfs12_trans_td(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_TD_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_TD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_trans_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_vdvfs12_trans_once(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_VDVFS12_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_ONCE_MASK),
				      (unsigned char) (MT6311_PMIC_VDVFS12_TRANS_ONCE_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ni_vdvfs12_vosel_trans(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_VDVFS12_CON19),
				    (&val),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VOSEL_TRANS_MASK),
				    (unsigned char) (MT6311_PMIC_NI_VDVFS12_VOSEL_TRANS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_k_rst_done(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_RST_DONE_MASK),
				      (unsigned char) (MT6311_PMIC_K_RST_DONE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_map_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_MAP_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_K_MAP_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_once_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_ONCE_EN_MASK),
				      (unsigned char) (MT6311_PMIC_K_ONCE_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_once(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_ONCE_MASK),
				      (unsigned char) (MT6311_PMIC_K_ONCE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_start_manual(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_START_MANUAL_MASK),
				      (unsigned char) (MT6311_PMIC_K_START_MANUAL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_src_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_SRC_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_K_SRC_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_auto_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_AUTO_EN_MASK),
				      (unsigned char) (MT6311_PMIC_K_AUTO_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_inv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_INV_MASK),
				      (unsigned char) (MT6311_PMIC_K_INV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_control_smps(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_CONTROL_SMPS_MASK),
				      (unsigned char) (MT6311_PMIC_K_CONTROL_SMPS_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_smps_osc_cal(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_K_CON2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_SMPS_OSC_CAL_MASK),
				    (unsigned char) (MT6311_PMIC_QI_SMPS_OSC_CAL_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_k_result(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_K_CON3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_K_RESULT_MASK),
				    (unsigned char) (MT6311_PMIC_K_RESULT_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_k_done(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_K_CON3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_K_DONE_MASK),
				    (unsigned char) (MT6311_PMIC_K_DONE_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_k_control(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_BUCK_K_CON3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_K_CONTROL_MASK),
				    (unsigned char) (MT6311_PMIC_K_CONTROL_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_k_buck_ck_cnt_8(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_BUCK_CK_CNT_8_MASK),
				      (unsigned char) (MT6311_PMIC_K_BUCK_CK_CNT_8_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_k_buck_ck_cnt_7_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_BUCK_K_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_K_BUCK_CK_CNT_7_0_MASK),
				      (unsigned char) (MT6311_PMIC_K_BUCK_CK_CNT_7_0_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_auxadc_adc_out_ch0(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CH0_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CH0_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_rdy_ch0(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CH0_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CH0_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_out_ch1(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC1),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CH1_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CH1_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_rdy_ch1(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC1),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CH1_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CH1_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_out_csm(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CSM_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_CSM_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_rdy_csm(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CSM_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_CSM_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_out_div2(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_DIV2_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_OUT_DIV2_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_rdy_div2(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_ADC3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_DIV2_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_RDY_DIV2_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_adc_busy_in(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_STA0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_BUSY_IN_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_ADC_BUSY_IN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_rqst_ch0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_RQST0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH0_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_rqst_ch1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_RQST0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH1_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_rqst_ch2(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_RQST0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH2_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_RQST_CH2_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_en_csm_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_EN_CSM_SW_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_EN_CSM_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_en_csm_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_EN_CSM_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_EN_CSM_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_test_auxadc(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_TEST_AUXADC_MASK),
				      (unsigned char) (MT6311_PMIC_RG_TEST_AUXADC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_ck_aon_gps(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_GPS_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_GPS_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_ck_aon_md(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_MD_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_MD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_ck_aon(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_AON_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_ck_on_extd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_ON_EXTD_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CK_ON_EXTD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_spl_num(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_SPL_NUM_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_SPL_NUM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_avg_num_small(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_SMALL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_SMALL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_avg_num_large(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_LARGE_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_LARGE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_avg_num_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_AVG_NUM_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_trim_ch0_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH0_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH0_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_trim_ch1_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH1_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH1_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_trim_ch2_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH2_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH2_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_trim_ch3_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON5),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH3_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_TRIM_CH3_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_con6_rsv0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CON6_RSV0_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CON6_RSV0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_adc_2s_comp_enb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_ADC_2S_COMP_ENB_MASK),
				      (unsigned char) (MT6311_PMIC_RG_ADC_2S_COMP_ENB_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_adc_trim_comp(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_ADC_TRIM_COMP_MASK),
				      (unsigned char) (MT6311_PMIC_RG_ADC_TRIM_COMP_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_out_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_OUT_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_OUT_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adc_pwdb_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADC_PWDB_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADC_PWDB_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_qi_vdvfs1_csm_en_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS1_CSM_EN_SW_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS1_CSM_EN_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_qi_vdvfs11_csm_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS11_CSM_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS11_CSM_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_qi_vdvfs12_csm_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON6),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS12_CSM_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_QI_VDVFS12_CSM_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_sw_gain_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON7),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_SW_GAIN_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_SW_GAIN_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_sw_offset_trim(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON8),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_SW_OFFSET_TRIM_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_SW_OFFSET_TRIM_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_rng_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_RNG_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_RNG_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_data_reuse_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DATA_REUSE_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DATA_REUSE_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_test_mode(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_TEST_MODE_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_TEST_MODE_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_bit_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_BIT_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_BIT_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_start_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_START_SW_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_START_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_start_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_START_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_START_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adc_pwdb(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON9),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADC_PWDB_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADC_PWDB_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_ad_auxadc_comp(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON10),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AD_AUXADC_COMP_MASK),
				    (unsigned char) (MT6311_PMIC_AD_AUXADC_COMP_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_da_dac_swctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON10),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DA_DAC_SWCTRL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DA_DAC_SWCTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_da_dac(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON11),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DA_DAC_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DA_DAC_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_swctrl_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_SWCTRL_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_SWCTRL_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_chsel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_CHSEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_CHSEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adcin_baton_ted_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON12),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_BATON_TED_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_BATON_TED_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adcin_chrin_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_CHRIN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_CHRIN_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adcin_batsns_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_BATSNS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_BATSNS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_adcin_cs_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON13),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_CS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ADCIN_CS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_dac_extd_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DAC_EXTD_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DAC_EXTD_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_dac_extd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DAC_EXTD_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DAC_EXTD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_dig1_rsv1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON14),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DIG1_RSV1_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DIG1_RSV1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_dig0_rsv1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON15),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_DIG0_RSV1_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_DIG0_RSV1_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_auxadc_ro_rsv1(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_RO_RSV1_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_RO_RSV1_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_lbat_max_irq(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_LBAT_MAX_IRQ_MASK),
				    (unsigned char) (MT6311_PMIC_LBAT_MAX_IRQ_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_lbat_min_irq(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON15),
				    (&val),
				    (unsigned char) (MT6311_PMIC_LBAT_MIN_IRQ_MASK),
				    (unsigned char) (MT6311_PMIC_LBAT_MIN_IRQ_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_autorpt_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON15),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_AUTORPT_EN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_AUTORPT_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_autorpt_prd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON16),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_AUTORPT_PRD_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_AUTORPT_PRD_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_debt_min(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON17),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBT_MIN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBT_MIN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_debt_max(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON18),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBT_MAX_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBT_MAX_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_det_prd_7_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON19),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_7_0_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_7_0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_det_prd_15_8(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON20),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_15_8_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_15_8_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_det_prd_19_16(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON21),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_19_16_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DET_PRD_19_16_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_auxadc_lbat_max_irq_b(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON22),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_MAX_IRQ_B_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_MAX_IRQ_B_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_lbat_en_max(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON22),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_EN_MAX_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_EN_MAX_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_irq_en_max(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON22),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_IRQ_EN_MAX_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_IRQ_EN_MAX_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_volt_max_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON22),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MAX_0_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MAX_0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_volt_max_1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON23),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MAX_1_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MAX_1_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_auxadc_lbat_min_irq_b(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON24),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_MIN_IRQ_B_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_MIN_IRQ_B_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_lbat_en_min(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON24),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_EN_MIN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_EN_MIN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_irq_en_min(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON24),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_IRQ_EN_MIN_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_IRQ_EN_MIN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_volt_min_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON24),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MIN_0_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MIN_0_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_lbat_volt_min_1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON25),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MIN_1_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_LBAT_VOLT_MIN_1_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_auxadc_lbat_debounce_count_max(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON26),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_auxadc_lbat_debounce_count_min(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_AUXADC_CON27),
				    (&val),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN_MASK),
				    (unsigned char) (MT6311_PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_auxadc_enpwm1_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON28),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM1_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM1_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_enpwm1_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON28),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM1_SW_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM1_SW_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_enpwm2_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON28),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM2_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM2_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_auxadc_enpwm2_sw(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_AUXADC_CON28),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM2_SW_MASK),
				      (unsigned char) (MT6311_PMIC_AUXADC_ENPWM2_SW_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vbiasn_oc_status(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_LDO_CON0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_OC_STATUS_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_OC_STATUS_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_vbiasn_on_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_ON_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_ON_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_mode_set(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_MODE_SET_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_MODE_SET_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_mode_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_MODE_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_MODE_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_stbtd(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_STBTD_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_STBTD_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vbiasn_mode(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_LDO_CON0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_MODE_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_MODE_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vbiasn_en(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_LDO_CON0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_EN_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_EN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_qi_vbiasn_ocfb_en(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_LDO_OCFB0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_OCFB_EN_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_OCFB_EN_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_vbiasn_ocfb_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_OCFB0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_OCFB_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_OCFB_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_ldo_degtd_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_OCFB0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_LDO_DEGTD_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_LDO_DEGTD_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_dis_sel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_DIS_SEL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_DIS_SEL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_trans_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_trans_ctrl(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_CTRL_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_CTRL_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_rg_vbiasn_trans_once(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_ONCE_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_TRANS_ONCE_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_qi_vbiasn_chr(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_LDO_CON2),
				    (&val),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_CHR_MASK),
				    (unsigned char) (MT6311_PMIC_QI_VBIASN_CHR_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_rg_vbiasn_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON3),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_EN_MASK),
				      (unsigned char) (MT6311_PMIC_RG_VBIASN_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_ldo_rsv(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_LDO_CON4),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_LDO_RSV_MASK),
				      (unsigned char) (MT6311_PMIC_LDO_RSV_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_fqmtr_tcksel(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_FQMTR_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_FQMTR_TCKSEL_MASK),
				      (unsigned char) (MT6311_PMIC_FQMTR_TCKSEL_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_fqmtr_busy(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_FQMTR_CON0),
				    (&val),
				    (unsigned char) (MT6311_PMIC_FQMTR_BUSY_MASK),
				    (unsigned char) (MT6311_PMIC_FQMTR_BUSY_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

void mt6311_set_fqmtr_en(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_FQMTR_CON0),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_FQMTR_EN_MASK),
				      (unsigned char) (MT6311_PMIC_FQMTR_EN_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_fqmtr_winset_1(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_FQMTR_CON1),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_FQMTR_WINSET_1_MASK),
				      (unsigned char) (MT6311_PMIC_FQMTR_WINSET_1_SHIFT)
	    );
	mt6311_unlock();
}

void mt6311_set_fqmtr_winset_0(unsigned char val)
{
	unsigned char ret = 0;

	mt6311_lock();
	ret = mt6311_config_interface((unsigned char) (MT6311_FQMTR_CON2),
				      (unsigned char) (val),
				      (unsigned char) (MT6311_PMIC_FQMTR_WINSET_0_MASK),
				      (unsigned char) (MT6311_PMIC_FQMTR_WINSET_0_SHIFT)
	    );
	mt6311_unlock();
}

unsigned char mt6311_get_fqmtr_data_1(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_FQMTR_CON3),
				    (&val),
				    (unsigned char) (MT6311_PMIC_FQMTR_DATA_1_MASK),
				    (unsigned char) (MT6311_PMIC_FQMTR_DATA_1_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

unsigned char mt6311_get_fqmtr_data_0(void)
{
	unsigned char ret = 0;
	unsigned char val = 0;

	mt6311_lock();
	ret = mt6311_read_interface((unsigned char) (MT6311_FQMTR_CON4),
				    (&val),
				    (unsigned char) (MT6311_PMIC_FQMTR_DATA_0_MASK),
				    (unsigned char) (MT6311_PMIC_FQMTR_DATA_0_SHIFT)
	    );
	mt6311_unlock();

	return val;
}

/*
 *   [Internal Function]
 */
void mt6311_dump_register(void)
{
	int i = 0;
	int i_max = 0xD0;	/*0xD5*/

	for (i = 0; i <= i_max; i += 8) {
		pr_err(PMICTAG "[0x%x]=0x%x, [0x%x]=0x%x, [0x%x]=0x%x, [0x%x]=0x%x\n",
			i, mt6311_get_reg_value(i),
			i+1, mt6311_get_reg_value(i+1),
			i+2, mt6311_get_reg_value(i+2),
			i+3, mt6311_get_reg_value(i+3));
		pr_err(PMICTAG "[0x%x]=0x%x, [0x%x]=0x%x, [0x%x]=0x%x, [0x%x]=0x%x\n",
			i+4, mt6311_get_reg_value(i+4),
			i+5, mt6311_get_reg_value(i+5),
			i+6, mt6311_get_reg_value(i+6),
			i+7, mt6311_get_reg_value(i+7));
	}
}

int get_mt6311_i2c_ch_num(void)
{
	return mt6311_BUSNUM;
}

unsigned int update_mt6311_chip_id(void)
{
	unsigned int id = 0;
	unsigned int id_l = 0;
	unsigned int id_r = 0;

	id_l = mt6311_get_cid();
	if (id_l < 0) {
		pr_err(PMICTAG "[update_mt6311_chip_id] id_l=%d\n", id_l);
		return id_l;
	}
	id_r = mt6311_get_swcid();
	if (id_r < 0) {
		pr_err(PMICTAG "[update_mt6311_chip_id] id_r=%d\n", id_r);
		return id_r;
	}
	id = ((id_l << 8) | (id_r));

	g_mt6311_cid = id;

	PMICLOG("[update_mt6311_chip_id] id_l=0x%x, id_r=0x%x, id=0x%x\n", id_l, id_r, id);

	return id;
}

unsigned int mt6311_get_chip_id(void)
{
	unsigned int ret = 0;

	if (g_mt6311_cid == 0) {
		ret = update_mt6311_chip_id();
		if (ret < 0) {
			g_mt6311_hw_exist = 0;
			pr_err(PMICTAG "[mt6311_get_chip_id] ret=%d hw_exist:%d\n", ret, g_mt6311_hw_exist);
			return ret;
		}
	}

	PMICLOG("[mt6311_get_chip_id] g_mt6311_cid=0x%x\n", g_mt6311_cid);

	return g_mt6311_cid;
}

void mt6311_hw_init(void)
{
	unsigned int ret = 0;

	PMICLOG("[mt6311_hw_init] 20140513, CC Lee\n");

	/*put init setting from DE/SA*/
	/*ret=mt6311_config_interface(0x04,0x11,0xFF,0); set pin to interrupt, DVT only*/

	if (mt6311_get_chip_id() >= PMIC6311_E1_CID_CODE) {
		pr_err(PMICTAG "[mt6311_hw_init] 6311 PMIC Chip = 0x%x\n", mt6311_get_chip_id());
		pr_err(PMICTAG "[mt6311_hw_init] 2016-04-26\n");

	/*put init setting from DE/SA*/

	ret = mt6311_config_interface(0x4, 0x2, 0x7, 3); /* [5:3]: GPIO1_MODE; CC, initial INT function*/
	ret = mt6311_config_interface(0x1F, 0x0, 0x1, 0); /*  [0:0]: VDVFS11_PG_H2L_EN; Ricky*/
	ret = mt6311_config_interface(0x1F, 0x0, 0x1, 1); /*  [1:1]: VDVFS12_PG_H2L_EN; Ricky*/
	ret = mt6311_config_interface(0x1F, 0x0, 0x1, 2); /*  [2:2]: VBIASN_PG_H2L_EN; Ricky*/
	ret = mt6311_config_interface(0x6D, 0x3, 0x3, 5);
	/* [6:5]: RG_UVLO_VTHL; Ricky, for K2/D3T UVLO issues_0.2V for PCB drop. 20150306*/
	ret = mt6311_config_interface(0x6E, 0x3, 0x3, 0);
	/* [1:0]: RG_UVLO_VTHH; Ricky, for K2/D3T UVLO issues_0.2V for PCB drop. 20150306*/
	ret = mt6311_config_interface(0x8B, 0x1, 0x7F, 0);
	/* [6:0]: VDVFS11_SFCHG_RRATE; Johnson, for DVFS slew rate rising=0.67us,20150305*/
	ret = mt6311_config_interface(0x8C, 0x5, 0x7F, 0);
	/* [6:0]: VDVFS11_VOSEL_ON; Setting by DVFS owner, 1.15V forD3T init. Johnson, 20150409*/
	ret = mt6311_config_interface(0x94, 0x3, 0x3, 0);
	/* [1:0]: VDVFS11_TRANS_TD; Johnson, for DVFS sof change, falling 50us,,20150305 */
	ret = mt6311_config_interface(0x94, 0x1, 0x3, 4);
	ret = mt6311_config_interface(0xCF, 0x0, 0x1, 0);
	ret = mt6311_config_interface(0x8E, 0x34, 0x7F, 0);
	ret = mt6311_config_interface(0x8F, 0x0, 0x7F, 0);
	ret = mt6311_config_interface(0x8D, 0x34, 0x7F, 0);
	ret = mt6311_config_interface(0x93, 0x1, 0x1, 0);
	ret = mt6311_config_interface(0x88, 0x0, 0x1, 0);
	ret = mt6311_config_interface(0x88, 0x1, 0x1, 1);
#if 1
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x04, mt6311_get_reg_value(0x04));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x15, mt6311_get_reg_value(0x15));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x1F, mt6311_get_reg_value(0x1F));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x6A, mt6311_get_reg_value(0x6A));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8B, mt6311_get_reg_value(0x8B));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8C, mt6311_get_reg_value(0x8C));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x94, mt6311_get_reg_value(0x94));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x93, mt6311_get_reg_value(0x93));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0xCF, mt6311_get_reg_value(0xCF));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x88, mt6311_get_reg_value(0x88));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8d, mt6311_get_reg_value(0x8d));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x8e, mt6311_get_reg_value(0x8e));
	PMICLOG("[mt6311] [0x%x]=0x%x\n", 0x90, mt6311_get_reg_value(0x90));
#endif
	} else {
		pr_err(PMICTAG "[mt6311_hw_init] Unknown PMIC Chip (0x%x)\n", mt6311_get_chip_id());
	}
}

unsigned int mt6311_hw_component_detect(void)
{
	unsigned int ret = 0, chip_id = 0;

	ret = update_mt6311_chip_id();
	if (ret < 0) {
		g_mt6311_hw_exist = 0;
		pr_err(PMICTAG "[update_mt6311_chip_id] ret=%d hw_exist:%d\n", ret, g_mt6311_hw_exist);
		return ret;
	}

	chip_id = mt6311_get_chip_id();
	if (chip_id < 0) {
		pr_err(PMICTAG "[mt6311_get_chip_id] ret=%d\n", chip_id);
		return chip_id;
	}
	if ((chip_id == PMIC6311_E1_CID_CODE) ||
	(chip_id == PMIC6311_E2_CID_CODE) ||
	(chip_id == PMIC6311_E3_CID_CODE)
	){
		g_mt6311_hw_exist = 1;
	} else
		g_mt6311_hw_exist = 0;
	PMICLOG("[mt6311_hw_component_detect] exist=%d\n", g_mt6311_hw_exist);
	return 0;
}

int is_mt6311_sw_ready(void)
{
	/*PMICLOG("g_mt6311_driver_ready=%d\n", g_mt6311_driver_ready);*/

	return g_mt6311_driver_ready;
}

int is_mt6311_exist(void)
{
	/*PMICLOG("g_mt6311_hw_exist=%d\n", g_mt6311_hw_exist);*/

	return g_mt6311_hw_exist;
}

/*
 * mt6311 interrupt
 */
#if 1

int g_mt6311_irq = 0;

#ifdef CUST_EINT_EXT_BUCK_OC_NUM
unsigned int g_eint_pmic_mt6311_num = CUST_EINT_EXT_BUCK_OC_NUM;
#else
unsigned int g_eint_pmic_mt6311_num = 14;	/*FPGA:0, phn:14*/
#endif

#ifdef CUST_EINT_EXT_BUCK_OC_DEBOUNCE_CN
unsigned int g_cust_eint_mt_pmic_mt6311_debounce_cn = CUST_EINT_EXT_BUCK_OC_DEBOUNCE_CN;
#else
unsigned int g_cust_eint_mt_pmic_mt6311_debounce_cn = 1;
#endif

#ifdef CUST_EINT_EXT_BUCK_OC_TYPE
unsigned int g_cust_eint_mt_pmic_mt6311_type = CUST_EINT_EXT_BUCK_OC_TYPE;
#else
unsigned int g_cust_eint_mt_pmic_mt6311_type = 4;
#endif

#ifdef CUST_EINT_EXT_BUCK_OC_DEBOUNCE_EN
unsigned int g_cust_eint_mt_pmic_mt6311_debounce_en = CUST_EINT_EXT_BUCK_OC_DEBOUNCE_EN;
#else
unsigned int g_cust_eint_mt_pmic_mt6311_debounce_en = 1;
#endif

static DEFINE_MUTEX(pmic_mutex_mt6311);
static struct task_struct *pmic_6311_thread_handle;
struct wake_lock pmicThread_lock_mt6311;

void wake_up_pmic_mt6311(void)
{
	PMICLOG("[wake_up_pmic_mt6311]\n");
	wake_up_process(pmic_6311_thread_handle);
	wake_lock(&pmicThread_lock_mt6311);
}
EXPORT_SYMBOL(wake_up_pmic_mt6311);

void mt_pmic_eint_irq_mt6311(void)
{
	PMICLOG("[mt_pmic_eint_irq_mt6311] receive interrupt\n");
	wake_up_pmic_mt6311();
}

void mt6311_int_test(void)
{
	pr_debug("[mt6311_int_test] start\n");

	mt6311_config_interface(0x20, 0x0F, 0xFF, 0);	/* pg dis */
	mt6311_set_rg_auxadc_ck_pdn(0);
	mt6311_set_rg_auxadc_1m_ck_pdn(0);
	mt6311_config_interface(0xB5, 0xC0, 0xFF, 0);	/* cc EN */
	mt6311_config_interface(0xAE, 0x03, 0xFF, 0);	/* ADC EN */
	mt6311_config_interface(0xAE, 0x00, 0xFF, 0);	/* ADC CLR */

	mt6311_set_auxadc_lbat_irq_en_max(0);
	mt6311_set_auxadc_lbat_irq_en_min(0);
	mt6311_set_auxadc_lbat_en_max(0);
	mt6311_set_auxadc_lbat_en_min(0);

	mt6311_set_auxadc_lbat_volt_max_1(0);
	mt6311_set_auxadc_lbat_volt_max_0(0);
	mt6311_set_auxadc_lbat_volt_min_1(0);
	mt6311_set_auxadc_lbat_volt_min_0(0);
	mt6311_set_auxadc_lbat_det_prd_19_16(0);
	mt6311_set_auxadc_lbat_det_prd_15_8(0);
	mt6311_set_auxadc_lbat_det_prd_7_0(0x1);
	mt6311_set_auxadc_lbat_debt_min(0x1);
	mt6311_set_auxadc_lbat_debt_max(0x1);

	mt6311_set_rg_int_pol(0);	/* high active */
	mt6311_set_rg_int_en(1);

	mt6311_set_auxadc_lbat_irq_en_max(1);
	mt6311_set_auxadc_lbat_irq_en_min(0);
	mt6311_set_auxadc_lbat_en_max(1);
	mt6311_set_auxadc_lbat_en_min(0);

	pr_debug("[mt6311_int_test] done, wait for interrupt..\n");
}

void cust_pmic_interrupt_en_setting_mt6311(void)
{
#if 1
	mt6311_set_rg_int_pol(0);	/* high active */
	mt6311_set_rg_int_en(1);
#endif

#if 0
	mt6311_int_test();
#endif
}

void mt6311_lbat_min_int_handler(void)
{
	/*unsigned int ret=0;*/
	PMICLOG("[mt6311_lbat_min_int_handler]....\n");
	/*ret=mt6311_config_interface(MT6311_TOP_INT_MON,0x1,0x1,0);*/
}

void mt6311_lbat_max_int_handler(void)
{
	/*unsigned int ret=0;*/
	PMICLOG("[mt6311_lbat_max_int_handler]....\n");

#if 0
	mt6311_set_auxadc_lbat_irq_en_max(0);
	mt6311_set_auxadc_lbat_irq_en_min(0);
	mt6311_set_auxadc_lbat_en_max(0);
	mt6311_set_auxadc_lbat_en_min(0);
	mt6311_set_rg_int_en(0);
#endif

	/*ret=mt6311_config_interface(MT6311_TOP_INT_MON,0x1,0x1,1);*/
}

unsigned int thr_l_int_status = 0;
unsigned int thr_h_int_status = 0;

void mt6311_clr_thr_l_int_status(void)
{
	thr_l_int_status = 0;
	PMICLOG("[mt6311_clr_thr_l_int_status]....\n");
}

void mt6311_clr_thr_h_int_status(void)
{
	thr_h_int_status = 0;
	PMICLOG("[mt6311_clr_thr_h_int_status]....\n");
}

unsigned int mt6311_get_thr_l_int_status(void)
{
	PMICLOG("[mt6311_get_thr_l_int_status]....\n");

	return thr_l_int_status;
}

unsigned int mt6311_get_thr_h_int_status(void)
{
	PMICLOG("[mt6311_get_thr_h_int_status]....\n");

	return thr_h_int_status;
}

void mt6311_thr_l_int_handler(void)
{
	/*unsigned int ret=0;*/
	thr_l_int_status = 1;
	PMICLOG("[mt6311_thr_l_int_handler]....\n");
	/*return thr_l_int_status;*/

	/*ret=mt6311_config_interface(MT6311_TOP_INT_MON,0x1,0x1,2);*/
}

void mt6311_thr_h_int_handler(void)
{
	/*unsigned int ret=0;*/
	thr_h_int_status = 1;
	PMICLOG("[mt6311_thr_h_int_handler]....\n");
	/*ret=mt6311_config_interface(MT6311_TOP_INT_MON,0x1,0x1,3);*/
}

void mt6311_buck_oc_int_handler(void)
{
	/*unsigned int ret=0;*/
	PMICLOG("[mt6311_buck_oc_int_handler]....\n");
	/*ret=mt6311_config_interface(MT6311_TOP_INT_MON,0x1,0x1,4);*/
}

static void mt6311_int_handler(void)
{
	unsigned int ret = 0;
	unsigned char mt6311_int_status_val_0 = 0;

	/*--------------------------------------------------------------------------------*/
	ret = mt6311_read_interface(MT6311_TOP_INT_MON, (&mt6311_int_status_val_0), 0xFF, 0x0);
	PMICLOG("[MT6311_INT] mt6311_int_status_val_0=0x%x\n", mt6311_int_status_val_0);

	if ((((mt6311_int_status_val_0) & (0x01)) >> 0) == 1)
		mt6311_lbat_min_int_handler();
	if ((((mt6311_int_status_val_0) & (0x02)) >> 1) == 1)
		mt6311_lbat_max_int_handler();
	if ((((mt6311_int_status_val_0) & (0x04)) >> 2) == 1)
		mt6311_thr_l_int_handler();
	if ((((mt6311_int_status_val_0) & (0x08)) >> 3) == 1)
		mt6311_thr_h_int_handler();
	if ((((mt6311_int_status_val_0) & (0x10)) >> 4) == 1)
		mt6311_buck_oc_int_handler();
}

static int pmic_thread_kthread_mt6311(void *x)
{
	unsigned int ret = 0;
	unsigned char mt6311_int_status_val_0 = 0;
	struct sched_param param = {.sched_priority = 98 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	PMICLOG("[MT6311_INT] enter\n");

	/* Run on a process content */
	while (1) {
		mutex_lock(&pmic_mutex_mt6311);

		mt6311_int_handler();

		cust_pmic_interrupt_en_setting_mt6311();

		ret =
		    mt6311_read_interface(MT6311_TOP_INT_MON, (&mt6311_int_status_val_0), 0xFF,
					  0x0);

		PMICLOG("[MT6311_INT] after ,mt6311_int_status_val_0=0x%x\n",
			mt6311_int_status_val_0);

		mdelay(1);

		mutex_unlock(&pmic_mutex_mt6311);
		wake_unlock(&pmicThread_lock_mt6311);

		set_current_state(TASK_INTERRUPTIBLE);

		/* mt_eint_unmask(g_eint_pmic_mt6311_num);*/
		if (g_mt6311_irq != 0)
			enable_irq(g_mt6311_irq);

		schedule();
	}

	return 0;
}

irqreturn_t mt6311_eint_handler(int irq, void *desc)
{
	mt_pmic_eint_irq_mt6311();

	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

void mt6311_eint_setting(void)
{
	/*ON/OFF interrupt*/
	int ret;

	cust_pmic_interrupt_en_setting_mt6311();

#if 1
/*	g_mt6311_irq = mt_gpio_to_irq(g_eint_pmic_mt6311_num);*/

/*	mt_gpio_set_debounce(g_eint_pmic_mt6311_num, g_cust_eint_mt_pmic_mt6311_debounce_cn);*/

	ret = request_irq(g_mt6311_irq, mt6311_eint_handler, g_cust_eint_mt_pmic_mt6311_type, "mt6311-eint", NULL);
	if (ret)
		PMICLOG("[CUST_EINT] Fail to register an irq=%d , err=%d\n", g_mt6311_irq, ret);
	enable_irq_wake(g_mt6311_irq);

	PMICLOG("[CUST_EINT] CUST_EINT_MT_PMIC_MT6311_NUM=%d\n", g_eint_pmic_mt6311_num);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n",
		g_cust_eint_mt_pmic_mt6311_debounce_cn);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt_pmic_mt6311_type);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n",
		g_cust_eint_mt_pmic_mt6311_debounce_en);
#else
	mt_eint_set_hw_debounce(g_eint_pmic_mt6311_num, g_cust_eint_mt_pmic_mt6311_debounce_cn);

	mt_eint_registration(g_eint_pmic_mt6311_num, g_cust_eint_mt_pmic_mt6311_type,
			     mt_pmic_eint_irq_mt6311, 0);

	mt_eint_unmask(g_eint_pmic_mt6311_num);

	PMICLOG("[CUST_EINT] CUST_EINT_MT_PMIC_MT6311_NUM=%d\n", g_eint_pmic_mt6311_num);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n",
		g_cust_eint_mt_pmic_mt6311_debounce_cn);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt_pmic_mt6311_type);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n",
		g_cust_eint_mt_pmic_mt6311_debounce_en);
#endif

	/*for all interrupt events, turn on interrupt module clock*/
#if 0
	/*mt6311_set_rg_intrp_ck_pdn(0);*/ /* not used in mt6311      */
#else
	mt6311_set_rg_int_pol(0);	/* high active*/
	mt6311_set_rg_int_en(1);
#endif
}

void mt6311_eint_init(void)
{
	/*---------------------*/
#if defined(CONFIG_MTK_FPGA)
	PMICLOG("[MT6311_EINT] disable when CONFIG_MTK_FPGA\n");
#else
	/*PMIC Interrupt Service*/
	pmic_6311_thread_handle =
	    kthread_create(pmic_thread_kthread_mt6311, (void *)NULL, "pmic_6311_thread");
	if (IS_ERR(pmic_6311_thread_handle)) {
		pmic_6311_thread_handle = NULL;
		PMICLOG("[MT6311_EINT] creation fails\n");
	} else {
		wake_up_process(pmic_6311_thread_handle);
		PMICLOG("[MT6311_EINT] kthread_create Done\n");
	}

	/*mt6311_eint_setting();*/
	PMICLOG("[MT6311_EINT] TBD\n");
#endif

}

#endif				/* EINT */

/*
 * mt6311 probe
 */

static int mt6311_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	unsigned int ret = 0;

	PMICLOG("[mt6311_driver_probe]\n");

	new_client = client;

	/*---------------------        */
    /* force change GPIO to SDA/SCA mode */

	ret = mt6311_hw_component_detect();
	if (ret < 0) {
		err = -ENOMEM;
		goto exit;
	}
	if (g_mt6311_hw_exist == 1) {
		mt6311_hw_init();
		/*debug only mt6311_dump_register();*/

		mt6311_eint_init();

	}
	g_mt6311_driver_ready = 1;

	pr_err(PMICTAG "[mt6311_driver_probe] g_mt6311_hw_exist=%d, g_mt6311_driver_ready=%d\n",
		g_mt6311_hw_exist, g_mt6311_driver_ready);

	PMIC_INIT_SETTING_V1();

	if (g_mt6311_hw_exist == 0) {
#ifdef BATTERY_OC_PROTECT
		/*re-init battery oc protect point for platform without extbuck*/
		battery_oc_protect_reinit();
#endif
		pr_err(PMICTAG "[mt6311_driver_probe] return err\n");
		return err;
	}

	return 0;

exit:
	PMICLOG("[mt6311_driver_probe] exit: return err\n");
	PMIC_INIT_SETTING_V1();
	return err;
}

/*
 *   [platform_driver API]
 */
#ifdef mt6311_AUTO_DETECT_DISABLE
    /* TBD */
#else
/*
 * mt6311_access
 */
static ssize_t show_mt6311_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_mt6311_access] 0x%x\n", g_reg_value_mt6311);
	return sprintf(buf, "%u\n", g_reg_value_mt6311);
}

static ssize_t store_mt6311_access(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int ret;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err(PMICTAG "[store_mt6311_access]\n");

	if (buf != NULL && size != 0) {
		/*PMICLOG("[store_mt6311_access] buf is %s and size is %d\n",buf,size);*/
		/*reg_address = simple_strtoul(buf, &pvalue, 16);*/

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);
		/*ret = kstrtoul(buf, 16, (unsigned long *)&reg_address);*/

		if (size > 5) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16);*/
			val =  strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err(PMICTAG "[store_mt6311_access] write mt6311 reg 0x%x with value 0x%x !\n",
				reg_address, reg_value);

			ret = mt6311_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret = mt6311_read_interface(reg_address, &g_reg_value_mt6311, 0xFF, 0x0);

			pr_err(PMICTAG "[store_mt6311_access] read mt6311 reg 0x%x with value 0x%x !\n",
				reg_address, g_reg_value_mt6311);
			pr_err
			    ("[store_mt6311_access] use \"cat mt6311_access\" to get value(decimal)\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(mt6311_access, 0664, show_mt6311_access, store_mt6311_access);	/*664*/

/*
 * mt6311_vosel_pin
 */
int g_mt6311_vosel_pin = 0;
static ssize_t show_mt6311_vosel_pin(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err(PMICTAG "[show_mt6311_vosel_pin] g_mt6311_vosel_pin=%d\n", g_mt6311_vosel_pin);
	return sprintf(buf, "%u\n", g_mt6311_vosel_pin);
}

static ssize_t store_mt6311_vosel_pin(struct device *dev, struct device_attribute *attr,
				      const char *buf, size_t size)
{
	int val = 0, ret;
	char *pvalue = NULL;

	pr_err(PMICTAG "[store_mt6311_vosel_pin]\n");

	/*val = simple_strtoul(buf, &pvalue, 16);*/
	pvalue = (char *)buf;
	ret = kstrtou32(pvalue, 16, (unsigned int *)&val);

	g_mt6311_vosel_pin = val;

	pr_err(PMICTAG "[store_mt6311_vosel_pin] g_mt6311_vosel_pin(%d)\n", g_mt6311_vosel_pin);

	return size;
}

static DEVICE_ATTR(mt6311_vosel_pin, 0664, show_mt6311_vosel_pin, store_mt6311_vosel_pin);	/*664*/

/*
 * mt6311_user_space_probe
 */
static int mt6311_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	PMICLOG("******** mt6311_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6311_access);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6311_vosel_pin);

	return 0;
}

struct platform_device mt6311_user_space_device = {
	.name = "mt6311-user",
	.id = -1,
};

static struct platform_driver mt6311_user_space_driver = {
	.probe = mt6311_user_space_probe,
	.driver = {
		   .name = "mt6311-user",
		   },
};

/*static struct i2c_board_info i2c_mt6311 __initdata =
{ I2C_BOARD_INFO("mt6311", (mt6311_SLAVE_ADDR_WRITE >> 1)) };*/ /* auto add  by dts */

#endif

static int __init mt6311_init(void)
{
#ifdef mt6311_AUTO_DETECT_DISABLE

	PMICLOG("[mt6311_init] mt6311_AUTO_DETECT_DISABLE\n");
	g_mt6311_hw_exist = 0;
	g_mt6311_driver_ready = 1;

#else

	int ret = 0;

	wake_lock_init(&pmicThread_lock_mt6311, WAKE_LOCK_SUSPEND,
		       "pmicThread_lock_mt6311 wakelock");
	{
		PMICLOG("[mt6311_init] init start. ch=%d!!\n", mt6311_BUSNUM);

		/*i2c_register_board_info(mt6311_BUSNUM, &i2c_mt6311, 1);*/

		if (i2c_add_driver(&mt6311_driver) != 0)
			PMICLOG("[mt6311_init] failed to register mt6311 i2c driver.\n");
		else
			PMICLOG("[mt6311_init] Success to register mt6311 i2c driver.\n");

		/* mt6311 user space access interface*/
		ret = platform_device_register(&mt6311_user_space_device);
		if (ret) {
			pr_err(PMICTAG "****[mt6311_init] Unable to device register(%d)\n", ret);
			return ret;
		}
		ret = platform_driver_register(&mt6311_user_space_driver);
		if (ret) {
			pr_err(PMICTAG "****[mt6311_init] Unable to register driver (%d)\n", ret);
			return ret;
		}
	}
#endif

	return 0;
}

static void __exit mt6311_exit(void)
{
	i2c_del_driver(&mt6311_driver);
}
module_init(mt6311_init);
module_exit(mt6311_exit);

MODULE_AUTHOR("Argus Lin");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

