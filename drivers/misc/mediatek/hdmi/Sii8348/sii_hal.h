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
#if !defined(SII_HAL_H)
#define SII_HAL_H
#include <linux/kernel.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifndef	FALSE
#define	FALSE	false
#endif

#ifndef	TRUE
#define	TRUE	true
#endif

#define MHL_PRODUCT_NUM 8348
#define MHL_DRIVER_NAME "sii8348drv"
#define MHL_DEVICE_NAME "sii-8348"

#define CONFIG_DEBUG_DRIVER
#define RCP_INPUTDEV_SUPPORT
#define ENABLE_GEN2
#define MHL2_ENHANCED_MODE_SUPPORT
#define MEDIA_DATA_TUNNEL_SUPPORT
#define ENABLE_EDID_INFO_PRINT
#define ENABLE_EDID_DEBUG_PRINT
///#define ENABLE_DUMP_INFOFRAME

#ifdef __cplusplus
}
#endif  

#endif 
