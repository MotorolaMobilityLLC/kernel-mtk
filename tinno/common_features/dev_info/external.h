/*
 * include/linux/spi/spidev.h
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
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

#ifndef __XX_EXTERNAL_H__
#define __XX_EXTERNAL_H__

#ifdef CONFIG_TINNO_PRODUCT_INFO

#include <linux/types.h>

#define MAX_DRIVERS 5

typedef struct g_cam_info_show {
char name[64];
int id;
int h;
int w;
} G_CAM_INFO_S; 

// typedef struct {
    // char vendorName[16];
    // char id[32];
    // char ramSize[8];
    // char romSize[8];
// }FLASH_INFO_SETTINGS;

extern G_CAM_INFO_S g_cam_info_struct[MAX_DRIVERS];
extern int get_camera_info(char *buf, void *arg0);

extern int get_mmc_chip_info(char *buf, void *arg0);

//For MTK secureboot.add by yinglong.tang
extern int get_mtk_secboot_info(void) ;
extern int get_mtk_secboot_cb(char *buf, void *args);
#endif
#endif /* __XX_EXTERNAL_H__ */
