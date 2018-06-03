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
#include <linux/wait.h>

#include "lcm_drv.h"
#include "ddp_irq.h"

/**
 * Local Constants
 */
#define FRAME_WIDTH		(240)
#define FRAME_HEIGHT		(240)

#define PHYSICAL_WIDTH  (29)	/* mm */
#define PHYSICAL_HEIGHT (29)	/* mm */


#define REGFLAG_DELAY		0xFE
#define REGFLAG_END_OF_TABLE	0xFF	/* END OF REGISTERS MARKER */

#define LCM_PRINT printk

/**
 * Local Variables
 */
static LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

static inline void send_ctrl_cmd(unsigned int cmd)
{
	lcm_util.send_cmd(cmd);
}

static inline void send_data_cmd(unsigned int data)
{
	lcm_util.send_data(data & 0xFF);
}


#ifndef ASSERT
#define ASSERT(expr)					\
	do {						\
		if (expr)				\
			break;				\
		pr_debug("DDP ASSERT FAILED %s, %d\n",	\
		       __FILE__, __LINE__);		\
	} while (0)
#endif


/**
 * LCM Driver Implementations
 */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	LCM_PRINT("lcm_set_util_funcs\n");
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	LCM_PRINT("lcm_get_params\n");
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type = LCM_TYPE_DBI;
	params->ctrl = LCM_CTRL_SERIAL_DBI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->physical_width = PHYSICAL_WIDTH;
	params->physical_height = PHYSICAL_HEIGHT;

	params->dbi.ctrl = LCM_CTRL_SERIAL_DBI;
	params->dbi.port = 0;
	params->dbi.clock_freq = LCM_DBI_CLOCK_FREQ_125M;
	params->dbi.data_width = LCM_DBI_DATA_WIDTH_8BITS;
	params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dbi.data_format.trans_seq = LCM_DBI_TRANS_SEQ_MSB_FIRST;
	params->dbi.data_format.padding = LCM_DBI_PADDING_ON_LSB;
	params->dbi.data_format.format = LCM_DBI_FORMAT_RGB565;
	params->dbi.data_format.width = LCM_DBI_DATA_WIDTH_8BITS;
	params->dbi.cpu_write_bits = LCM_DBI_CPU_WRITE_8_BITS;
	params->dbi.io_driving_current = LCM_DRIVING_CURRENT_16MA;

	/* ToDo: need check whether need set this params */
	/* msb_io_driving_current */
	/* ctrl_io_driving_current */

	params->dbi.serial.cs_polarity = LCM_POLARITY_RISING;
	params->dbi.serial.clk_polarity = LCM_POLARITY_RISING;
	params->dbi.serial.clk_phase = LCM_CLOCK_PHASE_0;
	params->dbi.serial.is_non_dbi_mode = 0;
	params->dbi.serial.clock_base = LCM_SERIAL_CLOCK_FREQ_125M;
	params->dbi.serial.clock_div = LCM_SERIAL_CLOCK_DIV_2;
	params->dbi.serial.wire_num = LCM_DBI_C_3WIRE;
	params->dbi.serial.datapin_num = LCM_DBI_C_1DATA_PIN;

	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{

	unsigned short x0, y0, x1, y1;
	unsigned short h_X_start, l_X_start, h_X_end, l_X_end, h_Y_start, l_Y_start, h_Y_end,
	    l_Y_end;
	LCM_PRINT("lcm_update,x=%d,y=%d,width=%d,height=%d\n", x, y, width, height);

	x0 = (unsigned short)x;
	y0 = (unsigned short)y;
	x1 = (unsigned short)x + width - 1;
	y1 = (unsigned short)y + height - 1;

	h_X_start = (x0 & 0xFF00) >> 8;
	l_X_start = x0 & 0x00FF;
	h_X_end = (x1 & 0xFF00) >> 8;
	l_X_end = x1 & 0x00FF;

	h_Y_start = (y0 & 0xFF00) >> 8;
	l_Y_start = y0 & 0x00FF;
	h_Y_end = (y1 & 0xFF00) >> 8;
	l_Y_end = y1 & 0x00FF;

	send_ctrl_cmd(0x2A);
	send_data_cmd(h_X_start);
	send_data_cmd(l_X_start);
	send_data_cmd(h_X_end);
	send_data_cmd(l_X_end);

	send_ctrl_cmd(0x2B);
	send_data_cmd(h_Y_start);
	send_data_cmd(l_Y_start);
	send_data_cmd(h_Y_end);
	send_data_cmd(l_Y_end);

	send_ctrl_cmd(0x2C);
}

static void init_lcm_registers(void)
{

	LCM_PRINT("init_lcm_registers\n");
	send_ctrl_cmd(0x11);
	MDELAY(120);		/* wait 120.ms */
	send_ctrl_cmd(0x36);
	send_data_cmd(0x00);

	send_ctrl_cmd(0x35);
	send_data_cmd(0x00);	/* te on */

	send_ctrl_cmd(0x2a);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0xef);

	send_ctrl_cmd(0x2b);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0xef);

	send_ctrl_cmd(0x3A);	/* Control interface color format */
	send_data_cmd(0x55);	/* RGB565 16bit/pixel */

	send_ctrl_cmd(0xB2);
	send_data_cmd(0x1C);
	send_data_cmd(0x1C);
	send_data_cmd(0x01);
	send_data_cmd(0xFF);
	send_data_cmd(0x33);

	send_ctrl_cmd(0xB3);
	send_data_cmd(0x10);
	send_data_cmd(0xFF);
	send_data_cmd(0x0F);

	send_ctrl_cmd(0xB4);
	send_data_cmd(0x0B);

	send_ctrl_cmd(0xB5);
	send_data_cmd(0x9F);

	send_ctrl_cmd(0xB7);
	send_data_cmd(0x35);

	send_ctrl_cmd(0xB8);
	send_data_cmd(0x28);

	send_ctrl_cmd(0xBC);
	send_data_cmd(0xEC);

	send_ctrl_cmd(0xBD);
	send_data_cmd(0xFE);

	send_ctrl_cmd(0xC0);
	send_data_cmd(0x2C);

	send_ctrl_cmd(0xC2);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xC3);
	send_data_cmd(0x1E);

	send_ctrl_cmd(0xC4);
	send_data_cmd(0x20);

	send_ctrl_cmd(0xC6);
	send_data_cmd(0x0F);

	send_ctrl_cmd(0xD0);
	send_data_cmd(0xA4);
	send_data_cmd(0xA1);

	send_ctrl_cmd(0xE0);
	send_data_cmd(0xD0);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x08);
	send_data_cmd(0x07);
	send_data_cmd(0x05);
	send_data_cmd(0x29);
	send_data_cmd(0x54);
	send_data_cmd(0x41);
	send_data_cmd(0x3C);
	send_data_cmd(0x17);
	send_data_cmd(0x15);
	send_data_cmd(0x1A);
	send_data_cmd(0x20);

	send_ctrl_cmd(0xE1);
	send_data_cmd(0xD0);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x08);
	send_data_cmd(0x07);
	send_data_cmd(0x04);
	send_data_cmd(0x29);
	send_data_cmd(0x44);
	send_data_cmd(0x42);
	send_data_cmd(0x3B);
	send_data_cmd(0x16);
	send_data_cmd(0x15);
	send_data_cmd(0x1B);
	send_data_cmd(0x1F);

	/* data-pin control */
	send_ctrl_cmd(0xE7);
	send_data_cmd(0x00);	/* one-data-pin */
	/* send_data_cmd(0x10); //two-data-pin */

	send_ctrl_cmd(0x2C);	/* clear the screen with black color */


	lcm_update(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	send_ctrl_cmd(0x29);
	MDELAY(10);
	send_ctrl_cmd(0x2C);
}

static void lcm_init(void)
{
	LCM_PRINT("lcm_init\n");
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(10);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	LCM_PRINT("lcm_suspend\n");

	send_ctrl_cmd(0x28);
	send_ctrl_cmd(0x10);
	MDELAY(5);
}



static void lcm_resume(void)
{
	LCM_PRINT("lcm_resume\n");

	send_ctrl_cmd(0x11);
	MDELAY(120);
	send_ctrl_cmd(0x29);

}

static void lcm_setbacklight(unsigned int level)
{
	LCM_PRINT("lcm_setbacklight = %d\n", level);

	if (level > 255)
		level = 255;

	send_ctrl_cmd(0x51);
	send_data_cmd(level);
}

LCM_DRIVER st7789h2_dbi_c_3wire_lcm_drv = {
	.name = "st7789h2_dbi_c_3wire",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,

	.set_backlight = lcm_setbacklight,
	/* .set_pwm        = lcm_setpwm, */
	/* .get_pwm        = lcm_getpwm, */
	.update = lcm_update
};
