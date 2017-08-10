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
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h>
	#include <platform/mt_pmic.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef BUILD_LK
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL, fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif


/*********************************************************
* Gate Driver
*********************************************************/

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/*#include <linux/jiffies.h>*/
#include <linux/uaccess.h>
/*#include <linux/delay.h>*/
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/*****************************************************************************
 * Define
 *****************************************************************************/
#ifndef CONFIG_FPGA_EARLY_PORTING

#ifdef CONFIG_MTK_LEGACY
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL/*for I2C channel 0*/
#endif
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
#ifdef CONFIG_MTK_LEGACY
static struct i2c_board_info tps65132_board_info  __initdata = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
#endif

#if !defined(CONFIG_MTK_LEGACY)
static const struct of_device_id lcm_of_match[] = {
		{ .compatible = "mediatek,i2c_lcd_bias" },
		{},
};
#endif

static struct i2c_client *tps65132_i2c_client;


/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct tps65132_dev	{
	struct i2c_client	*client;

};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

/*#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static struct i2c_client_address_data addr_data = { .forces = forces,};
#endif */
static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tps65132",
#if !defined(CONFIG_MTK_LEGACY)
		.of_match_table = lcm_of_match,
#endif
	},

};
/*****************************************************************************
 * Extern Area
 *****************************************************************************/



/*****************************************************************************
 * Function
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	pr_debug("tps65132_iic_probe\n");
	pr_debug("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	tps65132_i2c_client  = client;
	return 0;
}

static int tps65132_remove(struct i2c_client *client)
{
	pr_debug("tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

#if !defined(CONFIG_ARCH_MT6797)
static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2] = {0};

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		printk("tps65132 write data fail !!\n");
		return ret;
}
#endif

/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

	pr_debug("tps65132_iic_init\n");
#ifdef CONFIG_MTK_LEGACY
	i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
	pr_debug("tps65132_iic_init2\n");
#endif
	i2c_add_driver(&tps65132_iic_driver);
	pr_debug("tps65132_iic_init success\n");
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	pr_debug("tps65132_iic_exit\n");
	i2c_del_driver(&tps65132_iic_driver);
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");

#endif
#endif

#ifdef BUILD_LK
#ifndef CONFIG_FPGA_EARLY_PORTING

#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;/*I2C2; */
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
	TPS65132_i2c.mode = ST_MODE;
	TPS65132_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65132_i2c, write_data, len);
	/*printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);*/

	return ret_code;
}

#endif
#endif


/*********************************************************
* LCM  Driver
*********************************************************/

static const unsigned char LCD_MODULE_ID = 0x01; /* haobing modified 2013.07.11*/
/* ---------------------------------------------------------------------------
Local Constants
---------------------------------------------------------------------------*/
#define LCM_DSI_CMD_MODE 0
#define FRAME_WIDTH (1080)
#define FRAME_HEIGHT (1920)
/* physical size in um */
#define LCM_PHYSICAL_WIDTH									(74520)
#define LCM_PHYSICAL_HEIGHT									(132480)
#ifndef CONFIG_FPGA_EARLY_PORTING
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY 0xFFFC
#define REGFLAG_UDELAY 0xFFFB

#define REGFLAG_END_OF_TABLE 0xFFFD /* END OF REGISTERS MARKER*/
#define REGFLAG_RESET_LOW 0xFFFE
#define REGFLAG_RESET_HIGH 0xFFFF

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#define UFO_ON_3X_60
/*#define UFO_ON_3X_120
#define UFO_OFF_60*/

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/*static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
 ---------------------------------------------------------------------------
  Local Variables
 ---------------------------------------------------------------------------*/
static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;
#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n)        (lcm_util.mdelay(n))
#define UDELAY(n)        (lcm_util.udelay(n))

/* ---------------------------------------------------------------------------
  Local Functions
 ---------------------------------------------------------------------------*/

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V11(cmdq, pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq_V11(cmdq, pdata, queue_size, force_update)
#define set_gpio_lcd_enp(cmd) \
	lcm_util.set_gpio_lcd_enp_bias(cmd)

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

/*
static unsigned char od_table_33x33[] = {
0,   8,  19,  30,  43,  53,  61,  74,  84,  94, 104, 113, 122, 130, 139,
148, 156, 164, 172, 180, 188, 195, 203, 210, 217, 224, 230, 237, 243, 247, 252, 255, 255,
0,   8,  16,  26,  39,  50,  60,  72,  83,  94, 104, 113, 122, 130, 139,
148, 156, 164, 172, 180, 188, 196, 203, 210, 217, 224, 230, 236, 242, 245, 250, 253, 255,
0,   8,  16,  24,  36,  47,  59,  70,  81,  91, 102, 112, 120, 130, 138,
146, 154, 162, 170, 178, 186, 194, 202, 209, 216, 222, 229, 236, 242, 245, 250, 253, 255,
0,   8,  16,  24,  34,  45,  55,  66,  77,  88,  98, 109, 118, 128, 137,
146, 154, 162, 170, 178, 186, 194, 202, 209, 216, 222, 229, 236, 242, 245, 250, 253, 255,
0,   8,  16,  24,  32,  42,  53,  64,  75,  84,  96, 106, 116, 126, 136,
144, 152, 160, 169, 176, 184, 192, 200, 208, 214, 221, 228, 235, 240, 246, 249, 253, 255,
0,   8,  15,  23,  32,  40,  50,  62,  72,  84,  93, 104, 114, 125, 134,
144, 152, 160, 168, 176, 184, 192, 200, 207, 214, 221, 228, 234, 240, 245, 250, 253, 255,
0,   8,  15,  22,  30,  39,  48,  58,  69,  80,  89, 100, 111, 121, 132,
141, 150, 158, 168, 176, 183, 192, 200, 206, 214, 221, 228, 234, 240, 245, 250, 253, 255,
0,   8,  15,  20,  28,  36,  46,  56,  65,  77,  86,  98, 108, 119, 128,
140, 150, 158, 166, 174, 182, 190, 198, 206, 212, 220, 226, 234, 240, 245, 250, 253, 255,
0,   8,  14,  20,  26,  34,  44,  55,  64,  74,  85,  95, 106, 116, 128,
137, 148, 156, 165, 174, 181, 190, 198, 206, 212, 220, 226, 232, 238, 244, 249, 254, 255,
0,   8,  14,  19,  26,  33,  42,  52,  62,  72,  81,  93, 103, 114, 124,
136, 145, 156, 164, 172, 180, 188, 196, 204, 211, 218, 226, 232, 238, 244, 249, 254, 255,
0,   7,  14,  19,  25,  32,  40,  50,  60,  70,  80,  89, 101, 110, 122,
132, 142, 152, 162, 170, 179, 188, 196, 204, 210, 218, 225, 232, 238, 244, 247, 252, 255,
0,   7,  14,  18,  24,  30,  38,  47,  57,  66,  78,  88,  98, 109, 119,
130, 140, 150, 160, 170, 178, 186, 194, 202, 210, 218, 224, 232, 238, 243, 248, 251, 255,
0,   7,  14,  17,  22,  29,  37,  46,  54,  65,  74,  85,  96, 105, 117,
126, 138, 147, 158, 166, 176, 186, 194, 202, 210, 216, 224, 230, 236, 242, 248, 251, 255,
0,   7,  14,  16,  22,  29,  36,  44,  53,  62,  73,  82,  94, 104, 114,
126, 135, 146, 156, 166, 174, 184, 192, 201, 208, 216, 222, 230, 236, 242, 247, 252, 255,
0,   7,  14,  16,  22,  28,  34,  42,  50,  60,  70,  81,  90, 102, 112,
122, 134, 143, 154, 163, 172, 182, 192, 200, 208, 216, 222, 230, 236, 242, 247, 252, 255,
0,   7,  13,  17,  22,  28,  34,  42,  50,  58,  68,  78,  89,  99, 111,
120, 131, 142, 151, 162, 170, 180, 190, 199, 208, 214, 222, 230, 236, 242, 247, 252, 255,
0,   7,  13,  17,  22,  27,  33,  40,  46,  56,  64,  75,  86,  97, 106,
119, 128, 140, 151, 161, 169, 184, 190, 199, 208, 220, 228, 234, 238, 246, 252, 255, 255,
0,   7,  12,  16,  22,  27,  33,  40,  46,  54,  64,  74,  84,  94, 106,
116, 128, 136, 150, 161, 167, 182, 188, 199, 208, 218, 226, 234, 240, 246, 252, 255, 255,
0,   7,  12,  16,  21,  26,  31,  38,  44,  52,  60,  71,  82,  92, 102,
114, 124, 136, 144, 158, 166, 181, 188, 198, 208, 217, 226, 233, 240, 246, 253, 255, 255,
0,   7,  12,  16,  21,  26,  31,  38,  44,  52,  60,  68,  80,  90, 100,
110, 121, 131, 143, 152, 165, 176, 185, 196, 206, 215, 224, 232, 240, 246, 253, 255, 255,
0,   7,  12,  15,  20,  25,  30,  36,  42,  50,  58,  66,  76,  86,  96,
106, 116, 127, 138, 149, 160, 172, 185, 195, 205, 216, 224, 232, 240, 246, 252, 255, 255,
0,   7,  12,  16,  19,  24,  30,  36,  42,  49,  56,  65,  73,  82,  92,
102, 110, 122, 133, 145, 158, 168, 180, 191, 201, 212, 222, 230, 238, 246, 252, 255, 255,
0,   6,   9,  14,  18,  23,  28,  34,  41,  48,  56,  64,  72,  80,  89,
99, 107, 118, 129, 140, 153, 165, 176, 188, 201, 211, 221, 229, 238, 245, 252, 255, 255,
0,   6,   9,  14,  17,  22,  28,  33,  40,  47,  54,  63,  71,  79,  88,
97, 105, 115, 125, 136, 147, 159, 172, 184, 194, 207, 218, 228, 237, 245, 252, 255, 255,
0,   6,   9,  14,  18,  21,  26,  32,  38,  44,  52,  60,  68,  77,  86,
95, 104, 114, 124, 134, 145, 156, 167, 180, 192, 202, 216, 226, 236, 245, 252, 255, 255,
0,   6,   9,  12,  15,  20,  26,  31,  37,  44,  51,  59,  67,  76,  84,
93, 101, 110, 120, 130, 140, 151, 164, 175, 188, 200, 212, 225, 236, 245, 252, 255, 255,
0,   4,   8,  12,  13,  18,  23,  29,  35,  42,  50,  58,  66,  75,  83,
92, 100, 109, 118, 128, 138, 149, 160, 171, 183, 196, 208, 220, 233, 244, 252, 255, 255,
0,   4,   7,  10,  12,  17,  22,  27,  33,  40,  48,  56,  64,  74,  82,
91,  99, 108, 117, 126, 136, 146, 156, 166, 180, 191, 204, 216, 230, 241, 251, 255, 255,
0,   4,   7,  10,  11,  16,  20,  26,  32,  40,  48,  56,  64,  73,  81,
90,  97, 106, 114, 122, 132, 142, 152, 163, 176, 188, 201, 214, 224, 240, 251, 255, 255,
0,   4,   7,   9,  11,  16,  20,  25,  32,  40,  48,  56,  64,  72,  80,
88,  96, 104, 112, 121, 130, 140, 151, 162, 173, 185, 198, 211, 223, 232, 245, 254, 255,
0,   4,   7,   9,  11,  14,  19,  25,  32,  40,  48,  56,  64,  72,  80,
88,  96, 104, 112, 121, 130, 140, 150, 160, 171, 182, 195, 208, 220, 231, 240, 250, 255,
0,   4,   6,   7,   9,  13,  18,  24,  32,  40,  48,  56,  64,  72,  80,
88,  96, 104, 112, 120, 128, 138, 148, 158, 168, 178, 189, 200, 211, 224, 237, 248, 255,
0,   4,   6,   7,   8,  12,  17,  24,  32,  40,  48,  56,  64,  72,  80,
88,  96, 104, 112, 120, 128, 136, 146, 156, 164, 177, 188, 198, 211, 224, 236, 248, 255,
};*/  /*Format Modified to fulfill coding style*/

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },

	{REGFLAG_DELAY, 120, {} }
};

/*
static struct LCM_setting_table lcm_60fps_setting[] = {

	{0x00, 1, {0x00} },
	{0xFF, 3, {0x19, 0x06, 0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x19, 0x06} },

	{0x00, 1, {0x80} },
	{0xC0, 14, {0x00, 0x74, 0x07, 0xb0, 0x0f, 0x00, 0x74, 0x02, 0x06, 0x00, 0x74, 0x07, 0xb0, 0x0f} },

	{0x00, 1, {0x00} },
	{0xFB, 1, {0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x00, 0x00} },

	{0x00, 1, {0x00} },
	{0xFF, 3, {0x00, 0x00, 0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
*/

/*
static struct LCM_setting_table lcm_120fps_setting[] = {
	{0x00, 1, {0x00} },
	{0xFF, 3, {0x19, 0x06, 0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x19, 0x06} },

	{0x00, 1, {0x80} },
	{0xC0, 14, {0x00, 0x74, 0x00, 0x02, 0x06, 0x00, 0x74, 0x02, 0x06, 0x00, 0x74, 0x00, 0x02, 0x06} },

	{0x00, 1, {0x00} },
	{0xFB, 1, {0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x00, 0x00} },

	{0x00, 1, {0x00} },
	{0xFF, 3, {0x00, 0x00, 0x00} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }

};
*/

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0x00, 1, {0x00} },
	{0xFF, 3, {0x19, 0x06, 0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x19, 0x06} },

/*ufoe on*/
	{0x00, 1, {0x00} },
	{0x1C, 1, {0x04} },

#if (!LCM_DSI_CMD_MODE)
	{0x00, 1, {0xA2} },
	{0xC1, 1, {0x01} },

	{0x00, 1, {0x82} },
	{0xC1, 1, {0x01} },

	{0x00, 1, {0xC1} },
	{0xC5, 1, {0xBF} },
#endif

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x00, 0x00} },

	{0x00, 1, {0x00} },
	{0xFF, 3, {0x00, 0x00, 0x00} },
#if (defined UFO_ON_3X_60) || (defined UFO_OFF_60)
	{0x00, 1, {0x00} },
	{0xFF, 3, {0x19, 0x06, 0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x19, 0x06} },

	{0x00, 1, {0x80} },
	{0xC0, 14, {0x00, 0x74, 0x07, 0xb0, 0x0f, 0x00, 0x74, 0x02, 0x06, 0x00, 0x74, 0x07, 0xb0, 0x0f} },
#ifdef UFO_OFF_60
	{0x00, 1, {0x00} },
	/*ufoe off*/
	{0x1C, 1, {0x00} },
#endif
	{0x00, 1, {0x00} },
	{0xFB, 1, {0x01} },

	{0x00, 1, {0x80} },
	{0xFF, 2, {0x00, 0x00} },

	{0x00, 1, {0x00} },
	{0xFF, 3, {0x00, 0x00, 0x00} },
#endif

	{0x35, 1, {0x00} },
	{0x11, 0, {} },    /* Sleep out */
	{REGFLAG_DELAY, 130, {} },

	{0x29, 0, {} },    /* Display on*/
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;

		cmd = table[i].cmd;
		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* ---------------------------------------------------------------------------
  LCM Driver Implementations
 ---------------------------------------------------------------------------*/

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 0;

	/* DSI*/
	/* Command mode setting */
	params->dsi.LANE_NUM				    = LCM_FOUR_LANE;
	/*The following defined the fomat for data coming from LCD engine.*/
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability.*/
	params->dsi.packet_size = 256;

#if (LCM_DSI_CMD_MODE)
	params->dsi.packet_size_mult = 4;
#endif
	/*video mode timing*/

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 2;
	params->dsi.vertical_backporch					= 16;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 4;
	params->dsi.horizontal_backporch				= 42;
	params->dsi.horizontal_frontporch				= 42;

	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

/*command mode clock*/
#if (LCM_DSI_CMD_MODE)
#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	params->dsi.PLL_CLOCK = 140;/*260;*/
#else
	params->dsi.PLL_CLOCK = 500; /*this value must be in MTK suggested table*/
#endif
#endif

/*video mode clock*/
#if (!LCM_DSI_CMD_MODE)
#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	params->dsi.PLL_CLOCK = 173;
#else
	params->dsi.PLL_CLOCK = 450; /*this value must be in MTK suggested table*/
#endif
#endif

#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	params->dsi.ufoe_enable = 1;
	params->dsi.ufoe_params.compress_ratio = 3;
	params->dsi.ufoe_params.vlc_disable = 0;
	params->dsi.horizontal_active_pixel	= FRAME_WIDTH/3;
#endif
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	params->dsi.lane_swap_en = 1;

	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_3;

	/*params->od_table_size = 33 * 33;
	params->od_table = (void*)&od_table_33x33;*/
}

static void lcm_init_power(void)
{

}

static void lcm_suspend_power(void)
{

}

static void lcm_resume_power(void)
{

}

static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
#ifndef CONFIG_FPGA_EARLY_PORTING
	int ret = 0;
#endif

	cmd = 0x00;
	data = 0x0E;

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ONE);
#else
	set_gpio_lcd_enp(1);
#endif

	MDELAY(5);

#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]nt35595----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]nt35595----tps6132----cmd=%0x--i2c write success----\n", cmd);
#else
#if !defined(CONFIG_ARCH_MT6797)
	ret = tps65132_write_bytes(cmd, data);
#endif
	if (ret < 0)
		pr_debug("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write error-----\n", cmd);
	else
		pr_debug("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write success-----\n", cmd);
#endif

	cmd = 0x01;
	data = 0x0E;
#ifdef BUILD_LK
	ret = TPS65132_write_byte(cmd, data);
	if (ret)
		dprintf(0, "[LK]nt35595----tps6132----cmd=%0x--i2c write error----\n", cmd);
	else
		dprintf(0, "[LK]nt35595----tps6132----cmd=%0x--i2c write success----\n", cmd);
#else
#if !defined(CONFIG_ARCH_MT6797)
	ret = tps65132_write_bytes(cmd, data);
#endif
	if (ret < 0)
		pr_debug("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write error-----\n", cmd);
	else
		pr_debug("[KERNEL]nt35595----tps6132---cmd=%0x-- i2c write success-----\n", cmd);
#endif

#endif
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);
	push_table(0, lcm_initialization_setting,
	sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && defined(CONFIG_MTK_LEGACY)
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ZERO);
#endif
	push_table(0, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

#define LCM_OTM1906B_ID_ADD  (0xDA)
#define LCM_OTM1906B_ID     (0x40)

static unsigned int lcm_compare_id(void)
{

	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;/* read id return two byte,version and id*/
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(LCM_OTM1906B_ID_ADD, buffer, 2);
	id = buffer[0]; /*we only need ID*/
#ifdef BUILD_LK
	dprintf(0, "%s, LK otm1906a debug: otm1906a id = 0x%08x\n", __func__, id);
#else
	pr_debug("%s, kernel otm1906a horse debug: otm1906a id = 0x%08x\n", __func__, id);
#endif

	if (id == LCM_OTM1906B_ID)
		return 1;
	else
		return 0;

}

/* return TRUE: need recovery
return FALSE: No need recovery*/

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

if (buffer[0] != 0x24) {
	/*	pr_debug("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);*/
		return TRUE;

	} else {
	/*	pr_debug("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);*/
		return FALSE;
	}
#else
	return FALSE;
#endif

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH/4;
	unsigned int x1 = FRAME_WIDTH*3/4;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	/*pr_debug("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);*/
	data_array[0] = 0x0005390A;/*HS packet*/
	data_array[1] = (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;/* read id return two byte,version and id*/
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
		&& (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0>>8)&0xFF);
	x0_LSB = (x0&0xFF);
	x1_MSB = ((x1>>8)&0xFF);
	x1_LSB = (x1&0xFF);

	data_array[0] = 0x0005390A;/*HS packet*/
	data_array[1] = (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight(unsigned int level)
{
#ifdef BUILD_LK
	dprintf(0, "%s,lk nt35595 backlight: level = %d\n", __func__, level);
#else
	/*pr_debug("%s, kernel nt35595 backlight: level = %d\n", __func__, level);*/
#endif
	/* Refresh value of backlight level.*/
	lcm_backlight_level_setting[0].para_list[0] = level;

	push_table(0, lcm_backlight_level_setting,
	sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);

}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
/*customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register*/
	if (mode == 0) { /*V2C*/
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;/* mode control addr*/
		lcm_switch_mode_cmd.val[0] = 0x13;/*enabel GRAM firstly, ensure writing one frame to GRAM*/
		lcm_switch_mode_cmd.val[1] = 0x10;/*disable video mode secondly*/
	} else {/*C2V*/
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;/*disable GRAM and enable video mode*/
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)

static void lcm_send_60hz(void *cmdq)
{
	unsigned int array[] = {
	0x00001500,
	0x00042902,
	0x010619FF,
	0x80001500,
	0x00032902,
	0x000619FF,
	0x80001500,
	0x000F2902,
	0x077400C0,
	0x74000fb0,
	0x74000602,
	0x000fb007,
	0x00001500,
	0x01FB2300,
	0x80001500,
	0x00032902,
	0x000000FF,
	0x00001500,
	0x00042902,
	0x000000FF
	};
	dsi_set_cmdq_V11(cmdq, array, sizeof(array)/sizeof(unsigned int), 1);
	return;

}


static void lcm_send_120hz(void *cmdq)
{
	unsigned int array[] = {
		0x00001500,
		0x00042902,
		0x010619FF,
		0x80001500,
		0x00032902,
		0x000619FF,
		0x80001500,
		0x000F2902,
		0x007400C0,
		0x74000602,
		0x74000602,
		0x00060200,
		0x00001500,
		0x01FB2300,
		0x80001500,
		0x00032902,
		0x000000FF,
		0x00001500,
		0x00042902,
		0x000000FF
	};
	dsi_set_cmdq_V11(cmdq, array, sizeof(array)/sizeof(unsigned int), 1);
	return;

}

static int lcm_adjust_fps(void *cmdq, int fps, LCM_PARAMS *params)
{

#ifdef BUILD_LK
	dprintf(0, "%s:to %d\n", __func__, fps);
#else
	/*pr_debug("%s:to %d\n", __func__, fps);*/
#endif

	if (fps == 60) {
		/*push_table(cmdq, lcm_60fps_setting,
		sizeof(lcm_60fps_setting) / sizeof(struct LCM_setting_table), 1);*/
		lcm_send_60hz(cmdq);
		params->dsi.PLL_CLOCK = 140;
	} else if (fps == 120) {
		/*push_table(cmdq, lcm_120fps_setting,
		sizeof(lcm_120fps_setting) / sizeof(struct LCM_setting_table), 1);*/
		lcm_send_120hz(cmdq);
		params->dsi.PLL_CLOCK = 280;
	} else {
		return -1;
	}
	return 0;

}
#endif

LCM_DRIVER otm1906b_fhd_dsi_vdo_jdi_tps65132_mt6797_lcm_drv = {
	.name = "otm1906b_fhd_dsi_vdo_jdi_tps65132_mt6797",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	.adjust_fps        = lcm_adjust_fps,
#endif
	.esd_check = lcm_esd_check,
	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
};

