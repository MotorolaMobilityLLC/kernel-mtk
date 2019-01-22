/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program
 * If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef AUDDRV_NXPSPK_H
#define AUDDRV_NXPSPK_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <asm/div64.h>
#include <linux/i2c.h>

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/


/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/
static char const *const kAudioNXPSpkName = "/dev/nxpspk";

typedef struct {
	unsigned char data0;
	unsigned char data1;
	unsigned char data2;
	unsigned char data3;
} Aud_Buffer_Control;

/*below is control message*/
#define AUD_NXP_IOC_MAGIC 'C'

#define SET_NXP_REG         _IOWR(AUD_NXP_IOC_MAGIC, 0x00, Aud_Buffer_Control*)
#define GET_NXP_REG         _IOWR(AUD_NXP_IOC_MAGIC, 0x01, Aud_Buffer_Control*)

/* Pre-defined definition */
#define NXP_DEBUG_ON
#define NXP_DEBUG_ARRAY_ON
#define NXP_DEBUG_FUNC_ON


/*Log define*/
#define NXP_INFO(fmt, arg...)           pr_debug("<<-NXP-INFO->> "fmt"\n", ##arg)
#define NXP_ERROR(fmt, arg...)          pr_debug("<<-NXP-ERROR->> "fmt"\n", ##arg)

/****************************PART4:UPDATE define*******************************/
#define TFA9890_DEVICEID   0x0080

/*error no*/
#define ERROR_NO_FILE           2   /*ENOENT*/
#define ERROR_FILE_READ         23  /*ENFILE*/
#define ERROR_FILE_TYPE         21  /*EISDIR*/
#define ERROR_GPIO_REQUEST      4   /*EINTR*/
#define ERROR_I2C_TRANSFER      5   /*EIO*/
#define ERROR_NO_RESPONSE       16  /*EBUSY*/
#define ERROR_TIMEOUT           110 /*ETIMEDOUT*/

#endif


