#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include "fan49101.h"


#define PMIC_DEBUG_PR_DBG
/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define FAN49101_SLAVE_ADDR_WRITE	0xE0
#define FAN49101_SLAVE_ADDR_READ	0xE1

#define fan49101_BUSNUM			0

static struct i2c_client *new_client;
static const struct i2c_device_id fan49101_i2c_id[] = { {"fan49101", 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id fan49101_of_ids[] = {
	{.compatible = "mediatek,buck_boost"},
	{},
};
#endif

static int fan49101_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver fan49101_driver = {
	.driver = {
		   .name = "fan49101",
#ifdef CONFIG_OF
		   .of_match_table = fan49101_of_ids,
#endif
		   },
	.probe = fan49101_driver_probe,
	.id_table = fan49101_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(fan49101_i2c_access);
static DEFINE_MUTEX(fan49101_lock_mutex);

int g_fan49101_driver_ready = 0;
int g_fan49101_hw_exist = 0;

unsigned int g_fan49101_cid = 0;

#define PMICTAG                "[FAN49101] "
#if defined(PMIC_DEBUG_PR_DBG)
#define PMICLOG1(fmt, arg...)   pr_err(PMICTAG fmt, ##arg)
#else
#define PMICLOG1(fmt, arg...)
#endif

/**********************************************************
  *
  *   [I2C Function For Read/Write fan49101]
  *
  *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int fan49101_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&fan49101_i2c_access);


	/*
	   new_client->ext_flag =
	   ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
	 */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_PUSHPULL_FLAG |
	    I2C_HS_FLAG;
	new_client->timing = 400;


	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		PMICLOG1("[fan49101_read_byte] ret=%d\n", ret);

		new_client->ext_flag = 0;
		mutex_unlock(&fan49101_i2c_access);
		return ret;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	new_client->ext_flag = 0;

	mutex_unlock(&fan49101_i2c_access);
	return 1;
}

unsigned int fan49101_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&fan49101_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;


	/* new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG; */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG | I2C_PUSHPULL_FLAG |
	    I2C_HS_FLAG;
	new_client->timing = 400;


	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		PMICLOG1("[fan49101_write_byte] ret=%d\n", ret);

		new_client->ext_flag = 0;
		mutex_unlock(&fan49101_i2c_access);
		return ret;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&fan49101_i2c_access);
	return 1;
}
#else
unsigned int fan49101_read_byte(unsigned char cmd, unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&fan49101_i2c_access);

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

		if (ret == -ENXIO) {
			PMICLOG1("skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&fan49101_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int fan49101_write_byte(unsigned char cmd, unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];

	mutex_lock(&fan49101_i2c_access);

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

		if (ret == -ENXIO) {
			PMICLOG1("skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&fan49101_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif

/*
 *   [Read / Write Function]
 */
unsigned int fan49101_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
				     unsigned char SHIFT)
{
	unsigned char fan49101_reg = 0;
	unsigned int ret = 0;

	/* PMICLOG1("--------------------------------------------------\n"); */

	ret = fan49101_read_byte(RegNum, &fan49101_reg);

	/* PMICLOG1("[fan49101_read_interface] Reg[%x]=0x%x\n", RegNum, fan49101_reg); */

	fan49101_reg &= (MASK << SHIFT);
	*val = (fan49101_reg >> SHIFT);

	/* PMICLOG1("[fan49101_read_interface] val=0x%x\n", *val); */

	return ret;
}

unsigned int fan49101_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
				       unsigned char SHIFT)
{
	unsigned char fan49101_reg = 0;
	unsigned int ret = 0;

	/*PMICLOG1("--------------------------------------------------\n"); */

	ret = fan49101_read_byte(RegNum, &fan49101_reg);
	/* PMICLOG1("[fan49101_config_interface] Reg[%x]=0x%x\n", RegNum, fan49101_reg); */

	fan49101_reg &= ~(MASK << SHIFT);
	fan49101_reg |= (val << SHIFT);

	ret = fan49101_write_byte(RegNum, fan49101_reg);
	/*PMICLOG1("[fan49101_config_interface] write Reg[%x]=0x%x\n", RegNum, fan49101_reg); */

	/* Check */
	/*ret = fan49101_read_byte(RegNum, &fan49101_reg);
	   PMICLOG1("[fan49101_config_interface] Check Reg[%x]=0x%x\n", RegNum, fan49101_reg);
	 */

	return ret;
}

void fan49101_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = fan49101_config_interface((unsigned char)reg, (unsigned char)reg_val, 0xFF, 0x0);
}

unsigned int fan49101_get_reg_value(unsigned int reg)
{
	unsigned int ret = 0;
	unsigned char reg_val = 0;

	ret = fan49101_read_interface((unsigned char)reg, &reg_val, 0xFF, 0x0);

	return reg_val;
}

/*
 *   [APIs]
 */
void fan49101_lock(void)
{
	mutex_lock(&fan49101_lock_mutex);
}

void fan49101_unlock(void)
{
	mutex_unlock(&fan49101_lock_mutex);
}


/*
 *   [Internal Function]
 */
void fan49101_dump_register(void)
{
	unsigned char i = 0;

	PMICLOG1("[fan49101] ");
	for (i = FAN49101_SOFTRESET; i < FAN49101_CONTROL; i++)
		PMICLOG1("[0x%x]=0x%x\n", i, fan49101_get_reg_value(i));

	for (i = FAN49101_ID1; i < FAN49101_ID2; i++)
		PMICLOG1("[0x%x]=0x%x\n", i, fan49101_get_reg_value(i));

}

int get_fan49101_i2c_ch_num(void)
{
	return fan49101_BUSNUM;
}

void fan49101_hw_init(void)
{
	PMICLOG1("[fan49101_hw_init]\n");
#if 0
	unsigned int ret = 0;

	ret = fan49101_config_interface(0x01, 0xB2, 0xFF, 0);	/* VSEL=high, 1.1V */
	/*
	   if(g_vproc_vsel_gpio_number!=0)
	   {
	   ext_buck_vproc_vsel(1);
	   pr_notice( "[fan49101_hw_init] ext_buck_vproc_vsel(1)\n");
	   }
	 */
	ret = fan49101_config_interface(0x00, 0x8A, 0xFF, 0);	/* VSEL=low, 0.7V */
	ret = fan49101_config_interface(0x02, 0xA0, 0xFF, 0);

	pr_notice("[fan49101_hw_init] Done\n");
#endif
}

void fan49101_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char m_id = 0;
	unsigned char d_id = 0;

	ret = fan49101_read_interface(FAN49101_ID1, &m_id, 0xFF, 0);
	ret = fan49101_read_interface(FAN49101_ID2, &d_id, 0xFF, 0);

	/* check Manufacture ID */
	if (m_id == FAN49101_VENDOR_FAIRCHILD)
		g_fan49101_hw_exist = 1;
	else
		g_fan49101_hw_exist = 0;

	PMICLOG1("[fan49101_hw_component_detect] exist=%d, Reg[%x][7:0]=0x%x Reg[%x][7:0]=0x%x\n",
		 g_fan49101_hw_exist, FAN49101_ID1, m_id, FAN49101_ID2, d_id);
}

int is_fan49101_sw_ready(void)
{
	/*PMICLOG1("g_fan49101_driver_ready=%d\n", g_fan49101_driver_ready); */

	return g_fan49101_driver_ready;
}

int is_fan49101_exist(void)
{
	/*PMICLOG1("g_fan49101_hw_exist=%d\n", g_fan49101_hw_exist); */

	return g_fan49101_hw_exist;
}

int fan49101_vosel(unsigned long val)
{
	int ret = 1;
	unsigned long reg_val = 0;

	/* 0.603~1.411V (step 12.826mv) */
	reg_val = (((val * 1000) - 603000) + 6413) / 12826;

	if (reg_val > 63)
		reg_val = 63;

	reg_val = reg_val | 0x80;
	ret = fan49101_write_byte(0x01, reg_val);

	pr_notice("[fan49101_vosel] val=%ld, reg_val=%ld\n", val, reg_val);

	return ret;
}

static int fan49101_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	PMICLOG1("[fan49101_driver_probe]\n");
	new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (new_client == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	new_client = client;

	/* --------------------- */
	fan49101_hw_component_detect();
	if (g_fan49101_hw_exist == 1) {
		fan49101_hw_init();
		fan49101_dump_register();
	}
	g_fan49101_driver_ready = 1;

	PMICLOG1("[fan49101_driver_probe] g_fan49101_hw_exist=%d, g_fan49101_driver_ready=%d\n",
		 g_fan49101_hw_exist, g_fan49101_driver_ready);

	if (g_fan49101_hw_exist == 0) {
		PMICLOG1("[fan49101_driver_probe] return err\n");
		return err;
	}

	return 0;

exit:
	PMICLOG1("[fan49101_driver_probe] exit: return err\n");
	return err;
}

/*
 *   [platform_driver API]
 */
#ifdef FAN49101_AUTO_DETECT_DISABLE
    /* TBD */
#else
/*
 * fan49101_access
 */
unsigned char g_reg_value_fan49101 = 0;
static ssize_t show_fan49101_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG1("[show_fan49101_access] 0x%x\n", g_reg_value_fan49101);
	return sprintf(buf, "%u\n", g_reg_value_fan49101);
}

static ssize_t store_fan49101_access(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t size)
{
	int ret;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err("[store_fan49101_access]\n");

	if (buf != NULL && size != 0) {
		/*PMICLOG1("[store_fan49101_access] buf is %s and size is %d\n",buf,size); */
		/*reg_address = simple_strtoul(buf, &pvalue, 16); */

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);
		/*ret = kstrtoul(buf, 16, (unsigned long *)&reg_address); */

		if (size > 5) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16); */
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err
			    ("[store_fan49101_access] write fan49101 reg 0x%x with value 0x%x !\n",
			     reg_address, reg_value);

			ret = fan49101_config_interface(reg_address, reg_value, 0xFF, 0x0);
		} else {
			ret =
			    fan49101_read_interface(reg_address, &g_reg_value_fan49101, 0xFF, 0x0);

			pr_err("[store_fan49101_access] read fan49101 reg 0x%x with value 0x%x !\n",
			       reg_address, g_reg_value_fan49101);
			pr_err
			    ("[store_fan49101_access] use \"cat fan49101_access\" to get value(decimal)\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(fan49101_access, 0664, show_fan49101_access, store_fan49101_access);	/* 664 */

/*
 * fan49101_user_space_probe
 */
static int fan49101_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	PMICLOG1("******** fan49101_user_space_probe!! ********\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_fan49101_access);

	return 0;
}

struct platform_device fan49101_user_space_device = {
	.name = "fan49101-user",
	.id = -1,
};

static struct platform_driver fan49101_user_space_driver = {
	.probe = fan49101_user_space_probe,
	.driver = {
		   .name = "fan49101-user",
		   },
};

/*
static struct i2c_board_info __initdata i2c_fan49101 =
{  I2C_BOARD_INFO("XXXXXXXX", (FAN49101_SLAVE_ADDR_WRITE >> 1)) };
*/

#endif

static int __init fan49101_init(void)
{
#ifdef FAN49101_AUTO_DETECT_DISABLE

	PMICLOG1("[fan49101_init] FAN49101_AUTO_DETECT_DISABLE\n");
	g_fan49101_hw_exist = 0;
	g_fan49101_driver_ready = 1;

#else

	int ret = 0;

	/* if (g_vproc_vsel_gpio_number != 0) { */
	if (1) {
		PMICLOG1("[fan49101_init] init start. ch=%d!!\n", fan49101_BUSNUM);

		/* i2c_register_board_info(fan49101_BUSNUM, &i2c_fan49101, 1); */

		if (i2c_add_driver(&fan49101_driver) != 0)
			PMICLOG1("[fan49101_init] failed to register fan49101 i2c driver.\n");
		else
			PMICLOG1("[fan49101_init] Success to register fan49101 i2c driver.\n");

		/* fan49101 user space access interface */
		ret = platform_device_register(&fan49101_user_space_device);
		if (ret) {
			PMICLOG1("****[fan49101_init] Unable to device register(%d)\n", ret);
			return ret;
		}
		ret = platform_driver_register(&fan49101_user_space_driver);
		if (ret) {
			PMICLOG1("****[fan49101_init] Unable to register driver (%d)\n", ret);
			return ret;
		}
	} else {
		pr_notice("[fan49101_init] DCT no define EXT BUCK\n");
		g_fan49101_hw_exist = 0;
		g_fan49101_driver_ready = 1;
	}

#endif

	return 0;
}

static void __exit fan49101_exit(void)
{
	if (new_client != NULL)
		kfree(new_client);

	i2c_del_driver(&fan49101_driver);
}
module_init(fan49101_init);
module_exit(fan49101_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C fan49101 Driver");
MODULE_AUTHOR("Wilma Wu");
