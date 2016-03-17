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

#ifndef DEVFINO_H
#define DEVFINO_H

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

#ifdef CONFIG_OF
/*device information data*/
#define DEVINFO_MAX_SIZE 50
struct devinfo_tag {
	u32 size;
	u32 tag;
	u32 data[DEVINFO_MAX_SIZE];	/* device information */
	u32 data_size;  /* device information size */
};
#endif

#endif /* end of DEVFINO_H */

