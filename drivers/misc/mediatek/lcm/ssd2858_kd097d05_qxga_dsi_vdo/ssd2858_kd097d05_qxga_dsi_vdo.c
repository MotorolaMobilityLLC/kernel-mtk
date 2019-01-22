#ifndef BUILD_LK
#include <linux/string.h>
#endif

#include "lcm_drv.h"
#include "ddp_hal.h"
#include "cmdq_record.h"
#include "cmdq_core.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
#endif
#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

#endif
/* ---------------------------------------------------------------------------*/
/*  Local Constants */
/* ---------------------------------------------------------------------------*/

#define FRAME_WIDTH  										(1536)
#define FRAME_HEIGHT 										(2048)

#define LCM_ID       										(0x69)
#define REGFLAG_DELAY             							0xAB
#define REGFLAG_END_OF_TABLE      							0xAA   /* END OF REGISTERS MARKER */
#define LCM_DSI_CMD_MODE									0

/* ---------------------------------------------------------------------------*/
/*  Local Variables */
/* ---------------------------------------------------------------------------*/
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

/* ---------------------------------------------------------------------------*/
/*  Local Functions */
/* ---------------------------------------------------------------------------*/
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)											lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

#define GPIO_OUT_ONE  1
#define GPIO_OUT_ZERO 0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern void DSI_clk_HS_mode(enum DISP_MODULE_ENUM module, struct cmdqRecStruct *cmdq, bool enter);

static void init_lcm_registers(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	printk("%s, kernel\n", __func__);
#endif

	data_array[0] = 0x00ff2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0xf4010800;
	data_array[2] = 0x00005101;	/* RGB888: 0x00005501@66.6fps, 0x00005401@65fps, 0x00005101@63fps,*/
	dsi_set_cmdq(data_array, 3, 1);/* RGB565: 0x00003c01@47fps 0x00003501@27fps  */

	data_array[0] = 0x00062902;
	data_array[1] = 0x00000c00;
	data_array[2] = 0x00000300;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x3d0c1400;	/*0x3d0c1400 for RGB888, 0x3d081400 for RGB666, 0x3d041400 for RGB565*/
	data_array[2] = 0x00000f80;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x92152000;
	data_array[2] = 0x00007d56;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00002400;
	data_array[2] = 0x00000030;
	dsi_set_cmdq(data_array, 3, 1);
	/* 2858 sleep out */
	data_array[0]=0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(1);
	/* Rx setting */
	data_array[0] = 0x00062902;
	data_array[1] = 0x20010810;
	data_array[2] = 0x00004504;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00001c10;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);

	/* VTCM setting */
	data_array[0] = 0x00062902;
	data_array[1] = 0x00000c20;
	data_array[2] = 0x00000400;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x4b001020;
	data_array[2] = 0x00004900;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x0000a020;
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);
	/* Tx setting */
	data_array[0] = 0x00062902;
	data_array[1] = 0xd9000860;
	data_array[2] = 0x00000800;	/* 0x00004800 for full screen */
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00011460;
	data_array[2] = 0x00000601;
	dsi_set_cmdq(data_array, 3, 1);
/* BIST
	data_array[0] = 0x00062902;
	data_array[1] = 0x00411460;
	data_array[2] = 0x00000281;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00c11460;
	data_array[2] = 0x00000281;
	dsi_set_cmdq(data_array, 3, 1);
//*/
	data_array[0] = 0x00062902;
	data_array[1] = 0x0000a060;
	data_array[2] = 0x00000f00;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x500c0c60;
	data_array[2] = 0x00001802;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00081060;
	data_array[2] = 0x00005014;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x00008460;
	data_array[2] = 0x00000003;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00062902;
	data_array[1] = 0x0000a460;
	data_array[2] = 0x00000003;
	dsi_set_cmdq(data_array, 3, 1);
	/*Panel Initial code */
	data_array[0] = 0x01ff2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(150);

	data_array[0]=0x00b02300;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	
	data_array[0] = 0x02b22300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x11b32300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00b42300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x80b62300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x80b82300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x43ba2300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x53bb2300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x0abc2300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x4abd2300;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x2fbe2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1abf2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x39f02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x21f12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02b02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00c02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x06c12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16c22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1cc32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x2bc42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x20c52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x20c62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1fc72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1cc82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15c92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16ca2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x11cb2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15cc2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1dcd2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1dce2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1dcf2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x23d02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x21d12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x21d22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x24d32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16d42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1fd52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00d62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x06d72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16d82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1cd92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x2bda2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x20db2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x20dc2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1fdd2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1cde2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15df2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16e02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x11e12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x15e22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1de32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1de42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1de52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x23e62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x21e72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x21e82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x24e92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x16ea2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x1feb2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01b02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x10c02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0fc12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0ec22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0dc32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0cc42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0bc52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0ac62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x09c72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x08c82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x07c92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x06ca2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x05cb2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00cc2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01cd2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02ce2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x03cf2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x04d02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x10d62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0fd72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0ed82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0dd92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0cda2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0bdb2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x0adc2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x09dd2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x08de2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x07df2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x06e02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x05e12300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00e22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01e32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02e42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x03e52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x04e62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00e72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xc0ec2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x03b02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01c02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x6fc22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x6fc32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x36c52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x08c82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x04c92300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x41ca2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x43cc2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x60cf2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x04d22300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x04d32300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x03d42300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02d52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01d62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00d72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01db2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x36de2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x6fe62300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x6fe72300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x06b02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xa5b82300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xa5c02300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x3fd52300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	/* 2858 display on and video on */
	data_array[0]=0x00ff2300;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]=0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
}


/* ---------------------------------------------------------------------------*/
/*  LCM Driver Implementations */
/* ---------------------------------------------------------------------------*/
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy((void*)&lcm_util, (void*)util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset((void*)params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE;	/* SYNC_PULSE_VDO_MODE;	SYNC_EVENT_VDO_MODE; */
#endif

	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	
	params->dsi.vertical_sync_active				= 2;
	params->dsi.vertical_backporch					= 10;
	params->dsi.vertical_frontporch					= 20;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 24;
	params->dsi.horizontal_backporch				= 56;
	params->dsi.horizontal_frontporch				= 80;
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;
	/* 750@66.6, 744@66, 733@65fps, 710@63fps, 700fps, 350@47fps, 338@30fps, 300@27fps, 564@50fps, 676@60fps; */
	params->dsi.PLL_CLOCK = 710;
	params->dsi.cont_clock = 1;
	params->dsi.ssc_disable = 1;
	params->dsi.HS_PRPR = 0x07;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;	
}

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
static void lcm_init(void)
{
#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	printk("%s, kernel\n", __func__);
#endif

	SET_RESET_PIN(1);
	MDELAY(5); 

	DSI_clk_HS_mode(DISP_MODULE_DSI0, NULL, 1);
	MDELAY(5);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	data_array[0]=0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);

	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	
	SET_RESET_PIN(0);
	MDELAY(10);
	
	SET_RESET_PIN(1);
	MDELAY(50);

	init_lcm_registers();
}

#if LCM_DSI_CMD_MODE
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
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

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#endif
/* ---------------------------------------------------------------------------*/
/*  Get LCM Driver Hooks */
/* ---------------------------------------------------------------------------*/
LCM_DRIVER ssd2858_kd097d05_qxga_dsi_vdo_lcm_drv = 
{
  .name			= "ssd2858_kd097d05_qxga_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};
