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

#ifndef __DEVFINO_H__
#define __DEVFINO_H__

 /*****************************************************************************
 * MODULE DEFINITION
 *****************************************************************************/
#define MODULE_NAME	 "[devinfo]"
#define DEV_NAME		"devmap"
#define MAJOR_DEV_NUM	196

 /*****************************************************************************
 * IOCTL DEFINITION
 *****************************************************************************/
#define DEV_IOC_MAGIC	   'd'
#define READ_DEV_DATA	   _IOR(DEV_IOC_MAGIC,  1, unsigned int)
#define DEV_IOC_MAXNR	   (10)

 /*****************************************************************************
 * HRID DEFINITION
 *****************************************************************************/
#define HRID_SIZE_MAGIC_NUM              0x56AB0000
#define EFUSE_FIXED_HRID_SIZE_INDEX      10
#define HRID_DEFAULT_SIZE                2
#define HRID_MIN_ALLOWED_SIZE            2
#define HRID_MAX_ALLOWED_SIZE            8

/* devinfo_lk_atag_tag_header, devinfo_lk_atag_tag_devinfo_data and devinfo_lk_atag_tag_size
 * come from LK atag.c.
 * tag_header is renamed to devinfo_lk_atag_tag_header to avoid duplicated definition
 * tag_devinfo_data is renamed to devinfo_lk_atag_tag_devinfo_data to avoid duplicated definition
 * tag_size is renamed to devinfo_lk_atag_tag_size to avoid duplicated definition
 */
struct devinfo_lk_atag_tag_header {
	u32 size;
	u32 tag;
};

struct devinfo_lk_atag_tag_devinfo_data {
	u32 devinfo_data_size;
};

#define devinfo_lk_atag_tag_size(type)  ((sizeof(struct devinfo_lk_atag_tag_header) + sizeof(struct type)) >> 2)

#ifdef CONFIG_OF
/*device information data*/
struct devinfo_tag {
	u32 size;
	u32 tag;
	u32 data[0];	/* device information */
};
#endif

#endif /* end of __DEVFINO_H__ */

