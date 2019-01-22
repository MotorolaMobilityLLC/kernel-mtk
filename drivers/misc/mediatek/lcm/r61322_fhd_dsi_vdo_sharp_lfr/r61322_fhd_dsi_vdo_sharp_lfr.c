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
 * static struct i2c_client_address_data addr_data = { .forces = forces,};
 * #endif
 */
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
 *Local Constants
 *---------------------------------------------------------------------------
 */
#define LCM_DSI_CMD_MODE 0
#define FRAME_WIDTH (1080)
#define FRAME_HEIGHT (1920)
/* physical size in um */
#define LCM_PHYSICAL_WIDTH									(74520)
#define LCM_PHYSICAL_HEIGHT									(132480)
#ifndef CONFIG_FPGA_EARLY_PORTING
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY 0xFFFA
#define REGFLAG_UDELAY 0xFFFB

#define REGFLAG_END_OF_TABLE 0xFFFD /* END OF REGISTERS MARKER*/
#define REGFLAG_RESET_LOW 0xFFFE
#define REGFLAG_RESET_HIGH 0xFFFF

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/*static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
 * ---------------------------------------------------------------------------
 *  Local Variables
 * ---------------------------------------------------------------------------
 */
static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;
#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n)        (lcm_util.mdelay(n))
#define UDELAY(n)        (lcm_util.udelay(n))

/* ---------------------------------------------------------------------------
 *  Local Functions
 *---------------------------------------------------------------------------
 */

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

#define OD_TBL_M_SIZE (33 * 33)
static const unsigned char od_table_33x33[OD_TBL_M_SIZE] = {
	  0,   8,  17,  33,  46,  56,  69,  80,  93, 102, 110, 120, 128, 136, 144, 152, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 253, 255, 255, 255, 255,
	  0,   8,  17,  32,  43,  54,  66,  78,  90, 102, 111, 120, 128, 136, 144, 152, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 253, 255, 255, 255, 255,
	  0,   8,  17,  31,  40,  51,  63,  74,  86,  98, 110, 119, 128, 136, 144, 152, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 253, 255, 255, 255, 255,
	  0,   8,  14,  24,  36,  48,  59,  71,  83,  95, 107, 117, 127, 136, 144, 152, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 247, 252, 254, 255, 255, 255,
	  0,   7,  11,  20,  32,  44,  56,  67,  80,  90, 102, 112, 125, 135, 144, 152, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 247, 252, 254, 255, 255, 255,
	  0,   6,   9,  17,  28,  40,  52,  64,  76,  87,  98, 110, 120, 133, 143, 151, 160,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 246, 251, 254, 255, 255, 255,
	  0,   5,   9,  14,  24,  36,  48,  60,  72,  83,  94, 106, 118, 128, 140, 150, 159,
	     168, 176, 184, 192, 200, 208, 216, 224, 232, 239, 246, 251, 254, 255, 255, 255,
	  0,   4,   6,  11,  22,  32,  44,  56,  68,  80,  90, 102, 114, 126, 136, 148, 158,
	     167, 175, 184, 192, 200, 208, 216, 224, 232, 239, 245, 251, 254, 255, 255, 255,
	  0,   2,   6,  10,  18,  29,  40,  52,  64,  76,  88,  99, 111, 122, 134, 144, 156,
	     166, 174, 183, 191, 200, 208, 216, 223, 231, 238, 245, 250, 254, 255, 255, 255,
	  0,   2,   4,   9,  16,  25,  37,  48,  60,  72,  84,  96, 107, 119, 130, 142, 152,
	     164, 173, 182, 190, 199, 208, 215, 223, 230, 237, 244, 250, 253, 255, 255, 255,
	  0,   0,   1,   8,  14,  22,  33,  45,  56,  68,  80,  92, 104, 115, 127, 138, 150,
	     160, 172, 181, 189, 198, 206, 215, 222, 230, 237, 244, 250, 253, 255, 255, 255,
	  0,   0,   1,   7,  12,  20,  30,  41,  53,  64,  76,  88, 100, 112, 123, 135, 146,
	     158, 168, 178, 188, 197, 205, 214, 221, 229, 236, 243, 250, 253, 255, 255, 255,
	  0,   0,   1,   6,  10,  18,  26,  38,  49,  60,  72,  84,  96, 108, 120, 131, 142,
	     154, 166, 175, 186, 196, 205, 213, 220, 228, 235, 243, 249, 253, 255, 255, 255,
	  0,   0,   0,   4,  10,  17,  23,  34,  46,  56,  68,  80,  92, 104, 116, 128, 139,
	     150, 162, 172, 182, 194, 203, 211, 219, 227, 235, 242, 247, 253, 255, 255, 255,
	  0,   0,   0,   3,   9,  14,  21,  30,  42,  52,  64,  77,  88, 100, 112, 124, 136,
	     147, 158, 168, 180, 191, 203, 210, 218, 226, 234, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   2,   6,  11,  19,  29,  37,  49,  60,  72,  85,  96, 108, 120, 132,
	     143, 155, 167, 177, 190, 203, 210, 218, 226, 234, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   0,   4,   9,  17,  27,  33,  45,  56,  69,  81,  93, 104, 116, 128,
	     137, 152, 165, 181, 191, 201, 212, 222, 230, 234, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   0,   3,  10,  16,  25,  32,  41,  52,  64,  77,  89, 101, 112, 124,
	     135, 148, 162, 180, 190, 199, 210, 219, 229, 233, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  40,  50,  61,  72,  86,  96, 109, 119,
	     130, 144, 157, 171, 186, 198, 207, 217, 226, 233, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  43,  52,  62,  72,  82,  93, 104, 114,
	     125, 138, 152, 166, 179, 191, 204, 215, 225, 233, 241, 247, 253, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  40,  48,  56,  66,  77,  88, 101, 110,
	     120, 132, 147, 160, 173, 184, 200, 211, 222, 232, 241, 247, 252, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  40,  48,  56,  66,  75,  85,  95, 104,
	     116, 128, 141, 155, 168, 180, 192, 205, 218, 227, 239, 247, 252, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  40,  48,  56,  64,  72,  83,  92, 100,
	     111, 122, 136, 150, 163, 175, 188, 200, 213, 226, 237, 246, 251, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  16,  24,  32,  40,  48,  56,  64,  73,  81,  89,  97,
	     105, 116, 130, 142, 153, 170, 184, 196, 208, 222, 236, 245, 250, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  18,  28,  36,  43,  51,  59,  68,  77,  85,  93, 101,
	     110, 118, 128, 139, 149, 162, 179, 192, 204, 216, 231, 244, 249, 255, 255, 255,
	  0,   0,   0,   0,   1,   8,  19,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 128, 139, 149, 162, 174, 188, 200, 212, 226, 241, 248, 255, 255, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 147, 159, 170, 183, 196, 208, 226, 240, 247, 255, 255, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 155, 166, 178, 192, 204, 216, 233, 246, 253, 255, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 155, 166, 177, 188, 200, 213, 225, 242, 252, 255, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 155, 165, 175, 186, 197, 211, 223, 233, 246, 255, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 156, 166, 176, 186, 198, 211, 221, 232, 241, 252, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 157, 169, 179, 190, 201, 211, 221, 231, 240, 248, 255,
	  0,   0,   0,   0,   1,  10,  21,  29,  37,  45,  53,  61,  69,  77,  85,  93, 101,
	     110, 118, 127, 136, 146, 157, 169, 179, 190, 201, 211, 221, 231, 240, 248, 255,
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },

	{REGFLAG_DELAY, 120, {} }
};

#if 0
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
#endif

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xB0, 1, {0x04} },
	{0xD6, 1, {0x01} },
	/* =====LFR Setting===== */
	/*{0xB3, 8, {0x94, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }, */
	{0xDD, 26, {0x21, 0x31, 0x5F, 0x02, 0x0A,
						0x18, 0x00, 0xC5, 0xC0, 0xFF,
						0xFB, 0xC1, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x01, 0x00, 0x00,
						0x00, 0x01, 0x10, 0x01, 0x00, 0x08} },
	/* =====LFR Setting===== */
	{0x29, 0, {} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x51, 1, {0xFF} },
	{0x53, 1, {0x2C} },
	{0x54, 1, {0x2C} },
	{0x55, 1, {0x00} },
	{REGFLAG_DELAY, 40, {} },
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
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* ---------------------------------------------------------------------------
 * LCM Driver Implementations
 *---------------------------------------------------------------------------
 */

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
	params->physical_width = LCM_PHYSICAL_WIDTH;
	params->physical_height = LCM_PHYSICAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 1;

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

		params->dsi.vertical_sync_active			= 2; /* //2;//2; */
		params->dsi.vertical_backporch				= 5; /* //6;//5;//5;//5 */
		params->dsi.vertical_frontporch				= 7; /* //14;//10;//10;//13 */
		params->dsi.vertical_active_line			= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active			= 10; /* //8;//8;//10;//8 */
		params->dsi.horizontal_backporch			= 52; /* //16;//16;//20;//16 */
		params->dsi.horizontal_frontporch			= 216; /* //72;//40;//40;//72 */
		params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

params->dsi.PLL_CLOCK = 450;

		params->dsi.lfr_enable = 1;
		params->dsi.lfr_mode = 3; /* mode :1--static mode,2---dynamic mode,3----both mode */
		params->dsi.lfr_type = 0;
		params->dsi.lfr_skip_num = 1;

#if 0
/*command mode clock*/
#if (LCM_DSI_CMD_MODE)
#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	params->dsi.PLL_CLOCK = 168;/*260;*/
#else
	params->dsi.PLL_CLOCK = 500; /*this value must be in MTK suggested table*/
#endif
#endif

/*video mode clock*/
#if (!LCM_DSI_CMD_MODE)
#if (defined UFO_ON_3X_60) || (defined UFO_ON_3X_120)
	params->dsi.PLL_CLOCK = 168;
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
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_CK;

	params->od_table_size = OD_TBL_M_SIZE;
	params->od_table = (void *)&od_table_33x33;
#endif
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
 * return FALSE: No need recovery
 */

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	if (buffer[0] != 0x24) {
		pr_debug("[LCM ERROR] [0x53]=0x%02x\n", buffer[0]);
		return TRUE;

	} else {
		pr_debug("[LCM NORMAL] [0x53]=0x%02x\n", buffer[0]);
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

	pr_debug("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
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
	pr_debug("%s, kernel nt35595 backlight: level = %d\n", __func__, level);
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
	pr_debug("%s:to %d\n", __func__, fps);
#endif

	if (params->dsi.mode == CMD_MODE) {
		if (fps == 60) {
			lcm_send_60hz(cmdq);
			params->dsi.PLL_CLOCK = 168;
		} else if (fps == 120) {
			lcm_send_120hz(cmdq);
			params->dsi.PLL_CLOCK = 300;
		} else {
			return -1;
		}
	} else {
		if (fps == 60)
			params->dsi.PLL_CLOCK = 168;
		else if (fps == 120)
			params->dsi.PLL_CLOCK = 350;
		else
			return -1;
	}
	return 0;
}
#endif

LCM_DRIVER r61322_fhd_dsi_vdo_sharp_lfr_lcm_drv = {
	.name = "r61322_fhd_dsi_vdo_sharp_lfr",
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

