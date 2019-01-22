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

#include <linux/uaccess.h>
#include "mtk_smi.h"
#include "mmdvfs_mgr.h"

mmdvfs_lcd_size_enum mmdvfs_get_lcd_resolution(void)
{
	mmdvfs_lcd_size_enum result = MMDVFS_LCD_SIZE_HD;
	long lcd_resolution = 0;
	long lcd_w = 0;
	long lcd_h = 0;
	int convert_err = -EINVAL;

#if defined(CONFIG_LCM_WIDTH) && defined(CONFIG_LCM_HEIGHT)
	convert_err = kstrtoul(CONFIG_LCM_WIDTH, 10, &lcd_w);
	if (!convert_err)
		convert_err = kstrtoul(CONFIG_LCM_HEIGHT, 10, &lcd_h);
#endif	/* CONFIG_LCM_WIDTH, CONFIG_LCM_HEIGHT */

	if (convert_err) {
		lcd_w = DISP_GetScreenWidth();
		lcd_h = DISP_GetScreenHeight();
	}

	lcd_resolution = lcd_w * lcd_h;

	if (lcd_resolution <= MMDVFS_DISPLAY_SIZE_HD)
		result = MMDVFS_LCD_SIZE_HD;
	else if (lcd_resolution <= MMDVFS_DISPLAY_SIZE_FHD)
		result = MMDVFS_LCD_SIZE_FHD;
	else
		result = MMDVFS_LCD_SIZE_WQHD;

	return result;
}
