/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/string.h>
#include "lcm_drv.h"

/*****************************************************************************
 *  Local Constants
 *****************************************************************************/
#define FRAME_WIDTH                 (720)
#define FRAME_HEIGHT                (1280)
#define LCM_ID                      (0x69)
#define REGFLAG_DELAY               (0xAB)
#define REGFLAG_END_OF_TABLE        (0xAA)	/* END OF REGISTERS MARKER */
#define LCM_ID_NT35590              (0x90)

#define LCM_DSI_CMD_MODE            1
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*****************************************************************************
 *  Local Variables
 *****************************************************************************/
static LCM_UTIL_FUNCS lcm_util;
#define SET_RESET_PIN(v)            (lcm_util.set_reset_pin((v)))
#define UDELAY(n)                   (lcm_util.udelay(n))
#define MDELAY(n)                   (lcm_util.mdelay(n))

/*****************************************************************************
 *  Local Functions
 *****************************************************************************/
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)     lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                       lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                   lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                        lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

static unsigned int need_set_lcm_addr = 1;
struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};


/*****************************************************************************
 *  LCM Driver Implementations
 *****************************************************************************/
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy((void *)&lcm_util, (void *)util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset((void *)params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif
	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
#ifndef CONFIG_FPGA_EARLY_PORTING
	params->dsi.vertical_sync_active = 1;
	params->dsi.vertical_backporch = 1;
	params->dsi.vertical_frontporch = 2;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 2;
	params->dsi.horizontal_backporch = 12;
	params->dsi.horizontal_frontporch = 80;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.CLK_HS_POST = 26;
	params->dsi.compatibility_for_nvk = 0;

	params->dsi.PLL_CLOCK = 330;	/* dsi clock customization: should config clock value directly */
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 11;
#else
	params->dsi.vertical_sync_active = 1;
	params->dsi.vertical_backporch = 1;
	params->dsi.vertical_frontporch = 2;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 42;
	params->dsi.horizontal_frontporch = 52;
	params->dsi.horizontal_bllp = 85;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.compatibility_for_nvk = 0;

	params->dsi.PLL_CLOCK = 26;	/* dsi clock customization: should config clock value directly */
#endif
}

static void lcm_init(void)
{
	unsigned int data_array[16];

	MDELAY(40);
	SET_RESET_PIN(1);
	MDELAY(5);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000826;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000026;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);

	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(40);


	data_array[0] = 0x00023902;
#if (LCM_DSI_CMD_MODE)
	data_array[1] = 0x000008C2;	/* cmd mode   */
#else
	data_array[1] = 0x000003C2;	/* video mode */
#endif
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000002BA;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00033902;
	data_array[1] =
	    (((FRAME_HEIGHT / 2) & 0xFF) << 16) | (((FRAME_HEIGHT / 2) >> 8) << 8) | 0x44;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00005012;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00000213;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);

	need_set_lcm_addr = 1;
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000014F;
	dsi_set_cmdq(data_array, 2, 1);

}


static void lcm_resume(void)
{
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(50);

	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000826;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x00000026;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(2);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);

	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(40);

	data_array[0] = 0x00023902;
#if (LCM_DSI_CMD_MODE)
	data_array[1] = 0x000008C2;
#else
	data_array[1] = 0x000003C2;
#endif
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000002BA;	/* MIPI lane */
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00033902;
	data_array[1] =
	    (((FRAME_HEIGHT / 2) & 0xFF) << 16) | (((FRAME_HEIGHT / 2) >> 8) << 8) | 0x44;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00351500;	/* TE ON */
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);


	data_array[0] = 0x00023902;
	data_array[1] = 0x0000EEFF;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00005012;
	dsi_set_cmdq(data_array, 2, 1);


	data_array[0] = 0x00023902;
	data_array[1] = 0x00000213;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000000FF;
	dsi_set_cmdq(data_array, 2, 1);
	data_array[0] = 0x00023902;
	data_array[1] = 0x000001FB;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	need_set_lcm_addr = 1;

}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	/* need update at the first time */
	if (need_set_lcm_addr) {
		data_array[0] = 0x00053902;
		data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
		data_array[2] = (x1_LSB);
		dsi_set_cmdq(data_array, 3, 1);

		data_array[0] = 0x00053902;
		data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
		data_array[2] = (y1_LSB);
		dsi_set_cmdq(data_array, 3, 1);

		need_set_lcm_addr = 0;
	}

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

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

	array[0] = 0x00023700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0];

	pr_debug("%s, kernel nt35590 horse debug: nt35590 id = 0x%08x\n", __func__, id);

	if (id == LCM_ID_NT35590)
		return 1;
	else
		return 0;

}


static unsigned int lcm_esd_test = FALSE;	/* only for ESD test */

static unsigned int lcm_esd_check(void)
{
	char buffer[3];
	int array[4];
	int ret = 0;

	if (lcm_esd_test) {
		lcm_esd_test = FALSE;
		return 1;
	}

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0F, buffer, 1);
	if (buffer[0] != 0xc0) {
		pr_debug("[LCM ERROR] [0x0F]=0x%02x\n", buffer[0]);
		ret++;
	}

	read_reg_v2(0x05, buffer, 1);
	if (buffer[0] != 0x00) {
		pr_debug("[LCM ERROR] [0x05]=0x%02x\n", buffer[0]);
		ret++;
	}

	read_reg_v2(0x0A, buffer, 1);
	if ((buffer[0] & 0xf) != 0x0C) {
		pr_debug("[LCM ERROR] [0x0A]=0x%02x\n", buffer[0]);
		ret++;
	}
	/* return TRUE: need recovery     */
	/* return FALSE: No need recovery */
	if (ret)
		return 1;
	else
		return 0;
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	lcm_resume();

	return 1;
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	pr_debug("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700;	/* read id return two byte, version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	    && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	need_set_lcm_addr = 1;

	return ret;
}


LCM_DRIVER nt35590_hd720_dsi_cmd_auo_lcm_drv = {
	.name = "nt35590_hd720_dsi_cmd_auo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.ata_check = lcm_ata_check,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
