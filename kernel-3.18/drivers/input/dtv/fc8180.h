//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8180.h

	Description : Header file of Driver

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/

#ifndef __ISDBT_H__
#define __ISDBT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/list.h>

#include "fci_types.h"
#include "fci_ringbuffer.h"

#define CTL_TYPE 0
#define TS_TYPE 1

#define MAX_OPEN_NUM 		8

#define IOCTL_MAGIC	't'

#define DRV_VER "R-Wind_mobi-8180-20161024"

struct ioctl_info{
	uint32_t size;
	uint32_t buff[128];
};

#define IOCTL_MAXNR                     25

#define IOCTL_ISDBT_RESET	\
	_IO(IOCTL_MAGIC, 0)
#define IOCTL_ISDBT_PROBE	\
	_IO(IOCTL_MAGIC, 1)
#define IOCTL_ISDBT_INIT	\
	_IO(IOCTL_MAGIC, 2)
#define IOCTL_ISDBT_DEINIT	\
	_IO(IOCTL_MAGIC, 3)

#define IOCTL_ISDBT_BYTE_READ	\
	_IOWR(IOCTL_MAGIC, 4, struct ioctl_info)
#define IOCTL_ISDBT_WORD_READ	\
	_IOWR(IOCTL_MAGIC, 5, struct ioctl_info)
#define IOCTL_ISDBT_LONG_READ	\
	_IOWR(IOCTL_MAGIC, 6, struct ioctl_info)
#define IOCTL_ISDBT_BULK_READ	\
	_IOWR(IOCTL_MAGIC, 7, struct ioctl_info)

#define IOCTL_ISDBT_BYTE_WRITE	\
	_IOW(IOCTL_MAGIC, 8, struct ioctl_info)
#define IOCTL_ISDBT_WORD_WRITE	\
	_IOW(IOCTL_MAGIC, 9, struct ioctl_info)
#define IOCTL_ISDBT_LONG_WRITE	\
	_IOW(IOCTL_MAGIC, 10, struct ioctl_info)
#define IOCTL_ISDBT_BULK_WRITE	\
	_IOW(IOCTL_MAGIC, 11, struct ioctl_info)

#define IOCTL_ISDBT_TUNER_READ	\
	_IOWR(IOCTL_MAGIC, 12, struct ioctl_info)
#define IOCTL_ISDBT_TUNER_WRITE	\
	_IOW(IOCTL_MAGIC, 13, struct ioctl_info)

#define IOCTL_ISDBT_TUNER_SET_FREQ	\
	_IOW(IOCTL_MAGIC, 14, struct ioctl_info)
#define IOCTL_ISDBT_TUNER_SELECT	\
	_IOW(IOCTL_MAGIC, 15, struct ioctl_info)
#define IOCTL_ISDBT_TUNER_DESELECT	\
	_IO(IOCTL_MAGIC, 16)

#define IOCTL_ISDBT_SCAN_STATUS	\
	_IO(IOCTL_MAGIC, 17)
#define IOCTL_ISDBT_TS_START	\
	_IO(IOCTL_MAGIC, 18)
#define IOCTL_ISDBT_TS_STOP	\
	_IO(IOCTL_MAGIC, 19)

#define IOCTL_ISDBT_TUNER_GET_RSSI	\
	_IOWR(IOCTL_MAGIC, 20, struct ioctl_info)

#define IOCTL_ISDBT_HOSTIF_SELECT	\
	_IOW(IOCTL_MAGIC, 21, struct ioctl_info)
#define IOCTL_ISDBT_HOSTIF_DESELECT	\
	_IO(IOCTL_MAGIC, 22)

#define IOCTL_ISDBT_POWER_ON	\
	_IO(IOCTL_MAGIC, 23)
#define IOCTL_ISDBT_POWER_OFF	\
	_IO(IOCTL_MAGIC, 24)

struct ISDBT_OPEN_INFO_T{
	HANDLE				*hInit;
	struct list_head		hList;
	struct fci_ringbuffer		RingBuffer;
	u8				*buf;
	u8				isdbttype;
};

struct ISDBT_INIT_INFO_T{
	struct list_head		hHead;
};

enum ISDBT_MODE{
	ISDBT_POWERON       = 0,
	ISDBT_POWEROFF	    = 1,
	ISDBT_DATAREAD		= 2
};

extern int isdbt_open(struct inode *inode, struct file *filp);
extern long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
extern int isdbt_release(struct inode *inode, struct file *filp);
extern ssize_t isdbt_read(struct file *filp
	, char *buf, size_t count, loff_t *f_pos);

#ifdef __cplusplus
}
#endif

#endif
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
