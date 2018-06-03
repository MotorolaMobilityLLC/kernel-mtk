/*
 * Copyright(C)2015 MediaTek Inc.
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

#ifndef _MT_GEPF_H
#define _MT_GEPF_H

#include <linux/ioctl.h>

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*
*   enforce kernel log enable
*/
#define KERNEL_LOG  /* enable debug log flag if defined */

#define _SUPPORT_MAX_GEPF_FRAME_REQUEST_ 32
#define _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_ 32


#define SIG_ERESTARTSYS 512 /* ERESTARTSYS */
/*******************************************************************************
*
********************************************************************************/
#define GEPF_DEV_MAJOR_NUMBER    251
/*TODO: r selected*/
#define GEPF_MAGIC               'g'

#define GEPF_REG_RANGE           (0x1000)
/*TODO: GEPF base address : 0x1502C000
*       for GCE to access physical register addresses
*/
#define GEPF_BASE_HW   0x1502C000

/*This macro is for setting irq status represnted
* by a local variable,GEPFInfo.IrqInfo.Status[GEPF_IRQ_TYPE_INT_GEPF_ST]
*/
#define GEPF_INT_ST                 ((unsigned int)1<<1)


typedef struct {
	unsigned int module;
	unsigned int Addr;   /* register's addr */
	unsigned int Val;    /* register's value */
} GEPF_REG_STRUCT;

typedef struct {
	GEPF_REG_STRUCT *pData;   /* pointer to GEPF_REG_STRUCT */
	unsigned int Count;  /* count */
} GEPF_REG_IO_STRUCT;

/*
*    interrupt clear type
*/
typedef enum {
	GEPF_IRQ_CLEAR_NONE,     /* non-clear wait, clear after wait */
	GEPF_IRQ_CLEAR_WAIT,     /* clear wait, clear before and after wait */
	GEPF_IRQ_WAIT_CLEAR,     /* wait the signal and clear it, avoid the hw executime is too s hort. */
	GEPF_IRQ_CLEAR_STATUS,   /* clear specific status only */
	GEPF_IRQ_CLEAR_ALL       /* clear all status */
} GEPF_IRQ_CLEAR_ENUM;


/*
*    module's interrupt , each module should have its own isr.
*    note:
*        mapping to isr table,ISR_TABLE when using no device tree
*/
typedef enum {
	GEPF_IRQ_TYPE_INT_GEPF_ST,    /* GEPF */
	GEPF_IRQ_TYPE_AMOUNT
} GEPF_IRQ_TYPE_ENUM;

typedef struct {
	GEPF_IRQ_CLEAR_ENUM  Clear;
	GEPF_IRQ_TYPE_ENUM   Type;
	unsigned int        Status;         /*IRQ Status*/
	unsigned int        Timeout;
	int                 UserKey;        /* user key for doing interrupt operation */
	int                 ProcessID;      /* user ProcessID (will filled in kernel) */
	unsigned int        bDumpReg;       /* check dump register or not*/
} GEPF_WAIT_IRQ_STRUCT;

typedef struct {
	GEPF_IRQ_TYPE_ENUM     Type;
	int                    UserKey;        /* user key for doing interrupt operation */
	unsigned int           Status; /* Input */
} GEPF_CLEAR_IRQ_STRUCT;



/*TODO: update GEPF register map*/
typedef struct {
	unsigned int GEPF_CRT_0;
	unsigned int GEPF_CRT_1;
	unsigned int GEPF_CRT_2;
	unsigned int GEPF_FOCUS_BASE_ADDR;
	unsigned int GEPF_FOCUS_OFFSET;
	unsigned int GEPF_Y_BASE_ADDR;
	unsigned int GEPF_YUV_IMG_SIZE;
	unsigned int GEPF_UV_BASE_ADDR;
	unsigned int GEPF_DEPTH_BASE_ADDR;
	unsigned int GEPF_DEPTH_IMG_SIZE;
	unsigned int GEPF_BLUR_BASE_ADDR;
	unsigned int GEPF_YUV_BASE_ADDR;
	unsigned int TEMP_PRE_Y_BASE_ADDR;
	unsigned int TEMP_Y_BASE_ADDR;
	unsigned int TEMP_DEPTH_BASE_ADDR;
	unsigned int TEMP_PRE_DEPTH_BASE_ADDR;
	unsigned int BYPASS_DEPTH_BASE_ADDR;
	unsigned int BYPASS_DEPTH_WR_BASE_ADDR;
	unsigned int BYPASS_BLUR_BASE_ADDR;
	unsigned int BYPASS_BLUR_WR_BASE_ADDR;
	unsigned int TEMP_BLUR_BASE_ADDR;
	unsigned int TEMP_PRE_BLUR_BASE_ADDR;
	unsigned int TEMP_BLUR_WR_BASE_ADDR;
	unsigned int TEMP_DEPTH_WR_BASE_ADDR;
	unsigned int GEPF_DEPTH_WR_BASE_ADDR;
	unsigned int GEPF_BLUR_WR_BASE_ADDR;
	unsigned int TEMP_CTR_1;
	unsigned int GEPF_480_Y_ADDR;
	unsigned int GEPF_480_UV_ADDR;
	unsigned int GEPF_STRIDE_Y_UV;
	unsigned int GEPF_STRIDE_DEPTH;
	unsigned int TEMP_STRIDE_DEPTH;
	unsigned int GEPF_STRIDE_Y_UV_480;
} GEPF_Config;

/*******************************************************************************
*
********************************************************************************/
typedef enum {
	GEPF_CMD_RESET,            /* Reset */
	GEPF_CMD_DUMP_REG,         /* Dump GEPF Register */
	GEPF_CMD_DUMP_ISR_LOG,     /* Dump GEPF ISR log */
	GEPF_CMD_READ_REG,         /* Read register from driver */
	GEPF_CMD_WRITE_REG,        /* Write register to driver */
	GEPF_CMD_WAIT_IRQ,         /* Wait IRQ */
	GEPF_CMD_CLEAR_IRQ,        /* Clear IRQ */
	GEPF_CMD_ENQUE_NUM,        /* WMFE Enque Number */
	GEPF_CMD_ENQUE,            /* WMFE Enque */
	GEPF_CMD_ENQUE_REQ,        /* WMFE Enque Request */
	GEPF_CMD_DEQUE_NUM,        /* WMFE Deque Number */
	GEPF_CMD_DEQUE,            /* WMFE Deque */
	GEPF_CMD_DEQUE_REQ,        /* WMFE Deque Request */
	GEPF_CMD_TOTAL,
} GEPF_CMD_ENUM;

typedef struct {
	unsigned int  m_ReqNum;
	GEPF_Config *m_pGepfConfig;
} GEPF_Request;






#ifdef CONFIG_COMPAT
typedef struct {
	compat_uptr_t pData;
	unsigned int Count;  /* count */
} compat_GEPF_REG_IO_STRUCT;

typedef struct {
	unsigned int  m_ReqNum;
	compat_uptr_t m_pGepfConfig;
} compat_GEPF_Request;


#endif




#define GEPF_RESET           _IO(GEPF_MAGIC, GEPF_CMD_RESET)
#define GEPF_DUMP_REG        _IO(GEPF_MAGIC, GEPF_CMD_DUMP_REG)
#define GEPF_DUMP_ISR_LOG    _IO(GEPF_MAGIC, GEPF_CMD_DUMP_ISR_LOG)


#define GEPF_READ_REGISTER   _IOWR(GEPF_MAGIC, GEPF_CMD_READ_REG,        GEPF_REG_IO_STRUCT)
#define GEPF_WRITE_REGISTER  _IOWR(GEPF_MAGIC, GEPF_CMD_WRITE_REG,       GEPF_REG_IO_STRUCT)
#define GEPF_WAIT_IRQ        _IOW(GEPF_MAGIC, GEPF_CMD_WAIT_IRQ,        GEPF_WAIT_IRQ_STRUCT)
#define GEPF_CLEAR_IRQ       _IOW(GEPF_MAGIC, GEPF_CMD_CLEAR_IRQ,       GEPF_CLEAR_IRQ_STRUCT)

#define GEPF_ENQNUE_NUM  _IOW(GEPF_MAGIC, GEPF_CMD_ENQUE_NUM,    int)
#define GEPF_ENQUE      _IOWR(GEPF_MAGIC, GEPF_CMD_ENQUE,      GEPF_Config)
#define GEPF_ENQUE_REQ  _IOWR(GEPF_MAGIC, GEPF_CMD_ENQUE_REQ,  GEPF_Request)

#define GEPF_DEQUE_NUM  _IOR(GEPF_MAGIC, GEPF_CMD_DEQUE_NUM,    int)
#define GEPF_DEQUE      _IOWR(GEPF_MAGIC, GEPF_CMD_DEQUE,      GEPF_Config)
#define GEPF_DEQUE_REQ  _IOWR(GEPF_MAGIC, GEPF_CMD_DEQUE_REQ,  GEPF_Request)


#ifdef CONFIG_COMPAT
#define COMPAT_GEPF_WRITE_REGISTER   _IOWR(GEPF_MAGIC, GEPF_CMD_WRITE_REG,     compat_GEPF_REG_IO_STRUCT)
#define COMPAT_GEPF_READ_REGISTER    _IOWR(GEPF_MAGIC, GEPF_CMD_READ_REG,      compat_GEPF_REG_IO_STRUCT)

#define COMPAT_GEPF_ENQUE_REQ   _IOWR(GEPF_MAGIC, GEPF_CMD_ENQUE_REQ,  compat_GEPF_Request)
#define COMPAT_GEPF_DEQUE_REQ   _IOWR(GEPF_MAGIC, GEPF_CMD_DEQUE_REQ,  compat_GEPF_Request)

#endif

#endif

