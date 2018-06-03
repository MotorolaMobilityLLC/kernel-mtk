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

#ifndef _MT_EAF_H
#define _MT_EAF_H

#include <linux/ioctl.h>

#ifdef CONFIG_COMPAT
/*64 bit*/
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*
* enforce kernel log enable
*/
#define KERNEL_LOG		/*enable debug log flag if defined */

#define _SUPPORT_MAX_EAF_FRAME_REQUEST_ 32
#define _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_ 32

#define SIG_ERESTARTSYS 512 /*ERESTARTSYS*/
/*******************************************************************************
*
********************************************************************************/
#define EAF_DEV_MAJOR_NUMBER    251
#define EAF_MAGIC               'e'
#define EAF_REG_RANGE           0x1000
#define EAF_BASE_HW             0x1502D000
/**
* CAM interrupt status
*/
/* normal siganl */
#define EAF_INT_ST           (1<<0) /* INT_STATUS 0 bit */

typedef struct {
	unsigned int module;
	unsigned int Addr;	/* register's addr */
	unsigned int Val;	/* register's value */
} EAF_REG_STRUCT;

typedef struct {
	EAF_REG_STRUCT *pData;	/* pointer to EAF_REG_STRUCT */
	unsigned int Count;	/* count */
} EAF_REG_IO_STRUCT;

/**
* interrupt clear type
*/
typedef enum {
	EAF_IRQ_CLEAR_NONE,	/*non-clear wait, clear after wait */
	EAF_IRQ_CLEAR_WAIT,	/*clear wait, clear before and after wait */
	EAF_IRQ_WAIT_CLEAR,	/*wait the signal and clear it, avoid the hw executime is too s hort. */
	EAF_IRQ_CLEAR_STATUS,	/*clear specific status only */
	EAF_IRQ_CLEAR_ALL	/*clear all status */
} EAF_IRQ_CLEAR_ENUM;


/**
* module's interrupt , each module should have its own isr.
* note:
* mapping to isr table,ISR_TABLE when using no device tree
*/
typedef enum {
	EAF_IRQ_TYPE_INT_EAF_ST,	/* EAF */
	EAF_IRQ_TYPE_AMOUNT
} EAF_IRQ_TYPE_ENUM;

typedef struct {
	EAF_IRQ_CLEAR_ENUM Clear;
	EAF_IRQ_TYPE_ENUM Type;
	unsigned int Status;	/*IRQ Status */
	unsigned int Timeout;
	int UserKey;		/* user key for doing interrupt operation */
	int ProcessID;		/* user ProcessID (will filled in kernel) */
	unsigned int bDumpReg;	/* check dump register or not */
} EAF_WAIT_IRQ_STRUCT;

typedef struct {
	EAF_IRQ_TYPE_ENUM Type;
	int UserKey;		/* user key for doing interrupt operation */
	unsigned int Status;	/*Input */
} EAF_CLEAR_IRQ_STRUCT;



typedef struct{
	unsigned int	EAF_MAIN_CFG0;
	unsigned int	EAF_MAIN_CFG1;
	unsigned int	EAF_MAIN_CFG2;
	unsigned int	EAF_MAIN_CFG3;
	unsigned int	EAF_HIST_CFG0;
	unsigned int	EAF_HIST_CFG1;
	unsigned int	EAF_SRZ_CFG0;
	unsigned int	EAF_BOXF_CFG0;
	unsigned int	EAF_DIV_CFG0;
	unsigned int	EAF_LKHF_CFG0;
	unsigned int	EAF_LKHF_CFG1;
	unsigned int	EAFI_MASK0;
	unsigned int	EAFI_MASK1;
	unsigned int	EAFI_MASK2;
	unsigned int	EAFI_MASK3;
	unsigned int	EAFI_CUR_Y0;
	unsigned int	EAFI_CUR_Y1;
	unsigned int	EAFI_CUR_Y2;
	unsigned int	EAFI_CUR_Y3;
	unsigned int	EAFI_CUR_UV0;
	unsigned int	EAFI_CUR_UV1;
	unsigned int	EAFI_CUR_UV2;
	unsigned int	EAFI_CUR_UV3;
	unsigned int	EAFI_PRE_Y0;
	unsigned int	EAFI_PRE_Y1;
	unsigned int	EAFI_PRE_Y2;
	unsigned int	EAFI_PRE_Y3;
	unsigned int	EAFI_PRE_UV0;
	unsigned int	EAFI_PRE_UV1;
	unsigned int	EAFI_PRE_UV2;
	unsigned int	EAFI_PRE_UV3;
	unsigned int	EAFI_DEP0;
	unsigned int	EAFI_DEP1;
	unsigned int	EAFI_DEP2;
	unsigned int	EAFI_DEP3;
	unsigned int	EAFI_LKH_WMAP0;
	unsigned int	EAFI_LKH_WMAP1;
	unsigned int	EAFI_LKH_WMAP2;
	unsigned int	EAFI_LKH_WMAP3;
	unsigned int	EAFI_LKH_EMAP0;
	unsigned int	EAFI_LKH_EMAP1;
	unsigned int	EAFI_LKH_EMAP2;
	unsigned int	EAFI_LKH_EMAP3;
	unsigned int	EAFI_PROB0;
	unsigned int	EAFI_PROB1;
	unsigned int	EAFI_PROB2;
	unsigned int	EAFI_PROB3;
	unsigned int	EAFO_FOUT0;
	unsigned int	EAFO_FOUT1;
	unsigned int	EAFO_FOUT2;
	unsigned int	EAFO_FOUT3;
	unsigned int	EAFO_FOUT4;
	unsigned int	EAFO_POUT0;
	unsigned int	EAFO_POUT1;
	unsigned int	EAFO_POUT2;
	unsigned int	EAFO_POUT3;
	unsigned int	EAFO_POUT4;
	unsigned int	EAF_MASK_LB_CTL0;
	unsigned int	EAF_MASK_LB_CTL1;
	unsigned int	EAF_PRE_Y_LB_CTL0;
	unsigned int	EAF_PRE_Y_LB_CTL1;
	unsigned int	EAF_PRE_UV_LB_CTL0;
	unsigned int	EAF_PRE_UV_LB_CTL1;
	unsigned int	EAF_CUR_UV_LB_CTL0;
	unsigned int	EAF_CUR_UV_LB_CTL1;
	unsigned int	EAF_DMA_DCM_DIS;
	unsigned int	EAF_MAIN_DCM_DIS;
	unsigned int	EAF_DMA_DCM_ST;
	unsigned int	EAF_MAIN_DCM_ST;
	unsigned int	EAF_INT_CTL;

	unsigned int	JBFR_CFG0;
	unsigned int	JBFR_CFG1;
	unsigned int	JBFR_CFG2;
	unsigned int	JBFR_CFG3;
	unsigned int	JBFR_CFG4;
	unsigned int	JBFR_CFG5;
	unsigned int	JBFR_SIZE;
	unsigned int	JBFR_CFG6;
	unsigned int	JBFR_CFG7;
	unsigned int	JBFR_CFG8;

	unsigned int	SRZ6_CONTROL;
	unsigned int	SRZ6_IN_IMG;
	unsigned int	SRZ6_OUT_IMG;
	unsigned int	SRZ6_HORI_STEP;
	unsigned int	SRZ6_VERT_STEP;
	unsigned int	SRZ6_HORI_INT_OFST;
	unsigned int	SRZ6_HORI_SUB_OFST;
	unsigned int	SRZ6_VERT_INT_OFST;
	unsigned int	SRZ6_VERT_SUB_OFST;

	unsigned int	EAF_DMA_SOFT_RSTSTAT;
	unsigned int	EAF_TDRI_BASE_ADDR;
	unsigned int	EAF_TDRI_OFST_ADDR;
	unsigned int	EAF_TDRI_XSIZE;
	unsigned int	EAF_VERTICAL_FLIP_EN;
	unsigned int	EAF_DMA_SOFT_RESET;
	unsigned int	EAF_LAST_ULTRA_EN;
	unsigned int	EAF_SPECIAL_FUN_EN;
	unsigned int	EAFO_BASE_ADDR;
	unsigned int	EAFO_OFST_ADDR;
	unsigned int	EAFO_XSIZE;
	unsigned int	EAFO_YSIZE;
	unsigned int	EAFO_STRIDE;
	unsigned int	EAFO_CON;
	unsigned int	EAFO_CON2;
	unsigned int	EAFO_CON3;
	unsigned int	EAFO_CROP;
	unsigned int	EAFI_BASE_ADDR;
	unsigned int	EAFI_OFST_ADDR;
	unsigned int	EAFI_XSIZE;
	unsigned int	EAFI_YSIZE;
	unsigned int	EAFI_STRIDE;
	unsigned int	EAFI_CON;
	unsigned int	EAFI_CON2;
	unsigned int	EAFI_CON3;
	unsigned int	EAF2I_BASE_ADDR;
	unsigned int	EAF2I_OFST_ADDR;
	unsigned int	EAF2I_XSIZE;
	unsigned int	EAF2I_YSIZE;
	unsigned int	EAF2I_STRIDE;
	unsigned int	EAF2I_CON;
	unsigned int	EAF2I_CON2;
	unsigned int	EAF2I_CON3;
	unsigned int	EAF3I_BASE_ADDR;
	unsigned int	EAF3I_OFST_ADDR;
	unsigned int	EAF3I_XSIZE;
	unsigned int	EAF3I_YSIZE;
	unsigned int	EAF3I_STRIDE;
	unsigned int	EAF3I_CON;
	unsigned int	EAF3I_CON2;
	unsigned int	EAF3I_CON3;
	unsigned int	EAF4I_BASE_ADDR;
	unsigned int	EAF4I_OFST_ADDR;
	unsigned int	EAF4I_XSIZE;
	unsigned int	EAF4I_YSIZE;
	unsigned int	EAF4I_STRIDE;
	unsigned int	EAF4I_CON;
	unsigned int	EAF4I_CON2;
	unsigned int	EAF4I_CON3;
	unsigned int	EAF5I_BASE_ADDR;
	unsigned int	EAF5I_OFST_ADDR;
	unsigned int	EAF5I_XSIZE;
	unsigned int	EAF5I_YSIZE;
	unsigned int	EAF5I_STRIDE;
	unsigned int	EAF5I_CON;
	unsigned int	EAF5I_CON2;
	unsigned int	EAF5I_CON3;
	unsigned int	EAF6I_BASE_ADDR;
	unsigned int	EAF6I_OFST_ADDR;
	unsigned int	EAF6I_XSIZE;
	unsigned int	EAF6I_YSIZE;
	unsigned int	EAF6I_STRIDE;
	unsigned int	EAF6I_CON;
	unsigned int	EAF6I_CON2;
	unsigned int	EAF6I_CON3;
} EAF_Config;
/*******************************************************************************
*
********************************************************************************/

typedef enum {
	EAF_CMD_RESET,		/*Reset */
	EAF_CMD_DUMP_REG,	/*Dump DPE Register */
	EAF_CMD_DUMP_ISR_LOG,	/*Dump DPE ISR log */
	EAF_CMD_READ_REG,	/*Read register from driver */
	EAF_CMD_WRITE_REG,	/*Write register to driver */
	EAF_CMD_WAIT_IRQ,	/*Wait IRQ */
	EAF_CMD_CLEAR_IRQ,	/*Clear IRQ */
	EAF_CMD_ENQUE_NUM,
	EAF_CMD_ENQUE,
	EAF_CMD_ENQUE_REQ,
	EAF_CMD_DEQUE_NUM,
	EAF_CMD_DEQUE,
	EAF_CMD_DEQUE_REQ,
	EAF_CMD_TOTAL,
} EAF_CMD_ENUM;

typedef struct{
	unsigned int  m_ReqNum;
	EAF_Config *m_pEafConfig;
} EAF_Request;


#ifdef CONFIG_COMPAT
typedef struct {
	compat_uptr_t pData;
	unsigned int Count;	/* count */
} compat_EAF_REG_IO_STRUCT;

typedef struct{
	unsigned int  m_ReqNum;
	compat_uptr_t m_pEafConfig;
} compat_EAF_Request;

#endif




#define EAF_RESET           _IO(EAF_MAGIC, EAF_CMD_RESET)
#define EAF_DUMP_REG        _IO(EAF_MAGIC, EAF_CMD_DUMP_REG)
#define EAF_DUMP_ISR_LOG    _IO(EAF_MAGIC, EAF_CMD_DUMP_ISR_LOG)


#define EAF_READ_REGISTER   _IOWR(EAF_MAGIC, EAF_CMD_READ_REG,	EAF_REG_IO_STRUCT)
#define EAF_WRITE_REGISTER  _IOWR(EAF_MAGIC, EAF_CMD_WRITE_REG, EAF_REG_IO_STRUCT)
#define EAF_WAIT_IRQ        _IOW(EAF_MAGIC, EAF_CMD_WAIT_IRQ,	EAF_WAIT_IRQ_STRUCT)
#define EAF_CLEAR_IRQ       _IOW(EAF_MAGIC, EAF_CMD_CLEAR_IRQ,	EAF_CLEAR_IRQ_STRUCT)

#define EAF_ENQNUE_NUM	_IOW(EAF_MAGIC, EAF_CMD_ENQUE_NUM,   int)
#define EAF_ENQUE		_IOWR(EAF_MAGIC, EAF_CMD_ENQUE,      EAF_Config)
#define EAF_ENQUE_REQ		_IOWR(EAF_MAGIC, EAF_CMD_ENQUE_REQ,  EAF_Request)

#define EAF_DEQUE_NUM		_IOR(EAF_MAGIC, EAF_CMD_DEQUE_NUM,   int)
#define EAF_DEQUE		_IOWR(EAF_MAGIC, EAF_CMD_DEQUE,      EAF_Config)
#define EAF_DEQUE_REQ		_IOWR(EAF_MAGIC, EAF_CMD_DEQUE_REQ,  EAF_Request)


#ifdef CONFIG_COMPAT
#define COMPAT_EAF_WRITE_REGISTER   _IOWR(EAF_MAGIC, EAF_CMD_WRITE_REG, compat_EAF_REG_IO_STRUCT)
#define COMPAT_EAF_READ_REGISTER    _IOWR(EAF_MAGIC, EAF_CMD_READ_REG, compat_EAF_REG_IO_STRUCT)

#define COMPAT_EAF_ENQUE_REQ		_IOWR(EAF_MAGIC, EAF_CMD_ENQUE_REQ,  compat_EAF_Request)
#define COMPAT_EAF_DEQUE_REQ		_IOWR(EAF_MAGIC, EAF_CMD_DEQUE_REQ,  compat_EAF_Request)

#endif

#endif

