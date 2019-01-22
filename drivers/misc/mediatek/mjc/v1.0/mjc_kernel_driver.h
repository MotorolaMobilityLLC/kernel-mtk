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

#ifndef __MJC_KERNEL_DRIVER_H__
#define __MJC_KERNEL_DRIVER_H__

extern u32 get_devinfo_with_index(u32 index);
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);


struct MJC_EVENT_T {
	void *pvWaitQueue;	/* /< [IN]     The waitqueue discription */
	unsigned int u4TimeoutMs;	/* /< [IN]     The timeout ms */
	void *pvFlag;		/* /< [IN/OUT] flag */
};

struct MJC_CONTEXT_T {
	struct MJC_EVENT_T rEvent;
};

/* ************************************ */
/* IO control structure */
/* ************************************ */
struct MJC_IOCTL_LOCK_HW_T {
	unsigned int u4StructSize;
};

struct MJC_IOCTL_ISR_T {
	unsigned int u4StructSize;
	unsigned int u4TimeoutMs;
};

struct MJC_WRITE_REG_T {
	unsigned long reg;
	unsigned int val;
	unsigned int mask;
};

struct MJC_READ_REG_T {
	unsigned long reg;
	unsigned int val;
	unsigned int mask;
};

struct MJC_IOCTL_SRC_CLK_T {
	unsigned int u4StructSize;
	unsigned short u2OutputFramerate;
};

struct MJC_IOCTL_REG_INFO_T {
	unsigned int u4StructSize;
	unsigned long ulRegPAddress;
	unsigned long ulRegPSize;
};

#define MJC_IOC_MAGIC    'N'

#define MJC_LOCKHW                           _IOW(MJC_IOC_MAGIC, 0x00, struct MJC_IOCTL_LOCK_HW_T)
#define MJC_WAITISR                          _IOW(MJC_IOC_MAGIC, 0x01, struct MJC_IOCTL_ISR_T)
#define MJC_READ_REG                         _IOW(MJC_IOC_MAGIC, 0x02, struct MJC_READ_REG_T)
#define MJC_WRITE_REG                        _IOW(MJC_IOC_MAGIC, 0x03, struct MJC_WRITE_REG_T)
#define MJC_WRITE_REG_TBL                   _IOW(MJC_IOC_MAGIC, 0x04, int)
#define MJC_CLEAR_REG_TBL                   _IOW(MJC_IOC_MAGIC, 0x05, int)
#define MJC_SOURCE_CLK                      _IOW(MJC_IOC_MAGIC, 0x06, struct MJC_IOCTL_SRC_CLK_T)
#define MJC_REG_INFO                      _IOW(MJC_IOC_MAGIC, 0x07, struct MJC_IOCTL_REG_INFO_T)

#endif				/* __MJC_KERNEL_DRIVER_H__ */
