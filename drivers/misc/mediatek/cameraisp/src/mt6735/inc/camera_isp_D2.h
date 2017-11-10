/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MT_ISP_H
#define _MT_ISP_H

#include <linux/ioctl.h>

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/* 89serial IC , for HW FBC */
#define _89SERIAL_

/**
    external function declaration
*/
/**
extern MVOID mt_irq_set_sens(MUINT32 irq, MUINT32 sens);
extern MVOID mt_irq_set_polarity(MUINT32 irq, MUINT32 polarity);
*/
/*******************************************************************************
*
********************************************************************************/
#define ISP_DEV_MAJOR_NUMBER    251
#define ISP_MAGIC               'k'
/*******************************************************************************
*
********************************************************************************/
/* CAM_CTL_INT_STATUS */
#define ISP_IRQ_INT_STATUS_VS1_ST               ((unsigned int)1 << 0)
#define ISP_IRQ_INT_STATUS_TG1_ST1              ((unsigned int)1 << 1)
#define ISP_IRQ_INT_STATUS_TG1_ST2              ((unsigned int)1 << 2)
#define ISP_IRQ_INT_STATUS_EXPDON1_ST           ((unsigned int)1 << 3)
#define ISP_IRQ_INT_STATUS_TG1_ERR_ST           ((unsigned int)1 << 4)
#define ISP_IRQ_INT_STATUS_PASS1_TG1_DON_ST     ((unsigned int)1 << 10)
#define ISP_IRQ_INT_STATUS_SOF1_INT_ST          ((unsigned int)1 << 12)
#define ISP_IRQ_INT_STATUS_CQ_ERR_ST            ((unsigned int)1 << 13)
#define ISP_IRQ_INT_STATUS_PASS2_DON_ST         ((unsigned int)1 << 14)
#define ISP_IRQ_INT_STATUS_TPIPE_DON_ST         ((unsigned int)1 << 15)
#define ISP_IRQ_INT_STATUS_AF_DON_ST            ((unsigned int)1 << 16)
#define ISP_IRQ_INT_STATUS_FLK_DON_ST           ((unsigned int)1 << 17)
#define ISP_IRQ_INT_STATUS_CQ_DON_ST            ((unsigned int)1 << 19)
#define ISP_IRQ_INT_STATUS_IMGO_ERR_ST          ((unsigned int)1 << 20)
#define ISP_IRQ_INT_STATUS_AAO_ERR_ST           ((unsigned int)1 << 21)
#define ISP_IRQ_INT_STATUS_IMG2O_ERR_ST         ((unsigned int)1 << 23)
#define ISP_IRQ_INT_STATUS_ESFKO_ERR_ST         ((unsigned int)1 << 24)
#define ISP_IRQ_INT_STATUS_FLK_ERR_ST           ((unsigned int)1 << 25)
#define ISP_IRQ_INT_STATUS_LSC_ERR_ST           ((unsigned int)1 << 26)
#define ISP_IRQ_INT_STATUS_FBC_IMGO_DONE_ST     ((unsigned int)1 << 28)
#define ISP_IRQ_INT_STATUS_DMA_ERR_ST           ((unsigned int)1 << 30)
/* CAM_CTL_DMA_INT */
#define ISP_IRQ_DMA_INT_IMGO_DONE_ST            ((unsigned int)1 << 0)
#define ISP_IRQ_DMA_INT_IMG2O_DONE_ST           ((unsigned int)1 << 1)
#define ISP_IRQ_DMA_INT_AAO_DONE_ST             ((unsigned int)1 << 2)
/* #define ISP_IRQ_DMA_INT_LCSO_DONE_ST            ((unsigned int)1 << 3) */
#define ISP_IRQ_DMA_INT_ESFKO_DONE_ST           ((unsigned int)1 << 4)
/* #define ISP_IRQ_DMA_INT_DISPO_DONE_ST           ((unsigned int)1 << 5) */
/* #define ISP_IRQ_DMA_INT_VIDO_DONE_ST            ((unsigned int)1 << 6) */
/* #define ISP_IRQ_DMA_INT_VRZO_DONE_ST            ((unsigned int)1 << 7) */
#define ISP_IRQ_DMA_INT_CQ0_ERR_ST              ((unsigned int)1 << 8)
#define ISP_IRQ_DMA_INT_CQ0_DONE_ST             ((unsigned int)1 << 9)
/* #define ISP_IRQ_DMA_INT_SOF2_INT_ST             ((unsigned int)1 << 10) */
/* #define ISP_IRQ_DMA_INT_BUF_OVL_ST              ((unsigned int)1 << 11) */
#define ISP_IRQ_DMA_INT_TG1_GBERR_ST            ((unsigned int)1 << 12)
/* #define ISP_IRQ_DMA_INT_TG2_GBERR_ST            ((unsigned int)1 << 13) */
#define ISP_IRQ_DMA_INT_CQ0C_DONE_ST            ((unsigned int)1 << 14)
#define ISP_IRQ_DMA_INT_CQ0B_DONE_ST            ((unsigned int)1 << 15)
/* CAM_CTL_INTB_STATUS */
#define ISP_IRQ_INTB_STATUS_CQ_ERR_ST           ((unsigned int)1 << 13)
#define ISP_IRQ_INTB_STATUS_PASS2_DON_ST        ((unsigned int)1 << 14)
#define ISP_IRQ_INTB_STATUS_TPIPE_DON_ST        ((unsigned int)1 << 15)
#define ISP_IRQ_INTB_STATUS_CQ_DON_ST           ((unsigned int)1 << 19)
#define ISP_IRQ_INTB_STATUS_IMGO_ERR_ST         ((unsigned int)1 << 20)
#define ISP_IRQ_INTB_STATUS_LCSO_ERR_ST         ((unsigned int)1 << 22)
#define ISP_IRQ_INTB_STATUS_IMG2O_ERR_ST        ((unsigned int)1 << 23)
#define ISP_IRQ_INTB_STATUS_LSC_ERR_ST          ((unsigned int)1 << 26)
#define ISP_IRQ_INTB_STATUS_BPC_ERR_ST          ((unsigned int)1 << 28)
#define ISP_IRQ_INTB_STATUS_LCE_ERR_ST          ((unsigned int)1 << 29)
#define ISP_IRQ_INTB_STATUS_DMA_ERR_ST          ((unsigned int)1 << 30)
#define ISP_IRQ_INTB_STATUS_INT_WCLR_ST         ((unsigned int)1 << 31)

/* CAM_CTL_DMAB_INT */
#define ISP_IRQ_DMAB_INT_IMGO_DONE_ST           ((unsigned int)1 << 0)
#define ISP_IRQ_DMAB_INT_IMG2O_DONE_ST          ((unsigned int)1 << 1)
#define ISP_IRQ_DMAB_INT_AAO_DONE_ST            ((unsigned int)1 << 2)
#define ISP_IRQ_DMAB_INT_LCSO_DONE_ST           ((unsigned int)1 << 3)
#define ISP_IRQ_DMAB_INT_ESFKO_DONE_ST          ((unsigned int)1 << 4)
#define ISP_IRQ_DMAB_INT_DISPO_DONE_ST          ((unsigned int)1 << 5)
#define ISP_IRQ_DMAB_INT_VIDO_DONE_ST           ((unsigned int)1 << 6)
/* #define ISP_IRQ_DMAB_INT_VRZO_DONE_ST           ((unsigned int)1 << 7) */
/* #define ISP_IRQ_DMAB_INT_NR3O_DONE_ST           ((unsigned int)1 << 8) */
/* #define ISP_IRQ_DMAB_INT_NR3O_ERR_ST            ((unsigned int)1 << 9) */
/* CAM_CTL_INTC_STATUS */
#define ISP_IRQ_INTC_STATUS_CQ_ERR_ST           ((unsigned int)1 << 13)
#define ISP_IRQ_INTC_STATUS_PASS2_DON_ST        ((unsigned int)1 << 14)
#define ISP_IRQ_INTC_STATUS_TPIPE_DON_ST        ((unsigned int)1 << 15)
#define ISP_IRQ_INTC_STATUS_CQ_DON_ST           ((unsigned int)1 << 19)
#define ISP_IRQ_INTC_STATUS_IMGO_ERR_ST         ((unsigned int)1 << 20)
#define ISP_IRQ_INTC_STATUS_LCSO_ERR_ST         ((unsigned int)1 << 22)
#define ISP_IRQ_INTC_STATUS_IMG2O_ERR_ST        ((unsigned int)1 << 23)
#define ISP_IRQ_INTC_STATUS_LSC_ERR_ST          ((unsigned int)1 << 26)
#define ISP_IRQ_INTC_STATUS_BPC_ERR_ST          ((unsigned int)1 << 28)
#define ISP_IRQ_INTC_STATUS_LCE_ERR_ST          ((unsigned int)1 << 29)
#define ISP_IRQ_INTC_STATUS_DMA_ERR_ST          ((unsigned int)1 << 30)
/* CAM_CTL_DMAC_INT */
#define ISP_IRQ_DMAC_INT_IMGO_DONE_ST           ((unsigned int)1 << 0)
#define ISP_IRQ_DMAC_INT_IMG2O_DONE_ST          ((unsigned int)1 << 1)
#define ISP_IRQ_DMAC_INT_AAO_DONE_ST            ((unsigned int)1 << 2)
#define ISP_IRQ_DMAC_INT_LCSO_DONE_ST           ((unsigned int)1 << 3)
#define ISP_IRQ_DMAC_INT_ESFKO_DONE_ST          ((unsigned int)1 << 4)
#define ISP_IRQ_DMAC_INT_DISPO_DONE_ST          ((unsigned int)1 << 5)
#define ISP_IRQ_DMAC_INT_VIDO_DONE_ST           ((unsigned int)1 << 6)
/* #define ISP_IRQ_DMAC_INT_VRZO_DONE_ST           ((unsigned int)1 << 7) */
/* #define ISP_IRQ_DMAC_INT_NR3O_DONE_ST           ((unsigned int)1 << 8) */
/* #define ISP_IRQ_DMAC_INT_NR3O_ERR_ST            ((unsigned int)1 << 9) */
/* CAM_CTL_INT_STATUSX */
#define ISP_IRQ_INTX_STATUS_VS1_ST              ((unsigned int)1 << 0)
#define ISP_IRQ_INTX_STATUS_TG1_ST1             ((unsigned int)1 << 1)
#define ISP_IRQ_INTX_STATUS_TG1_ST2             ((unsigned int)1 << 2)
#define ISP_IRQ_INTX_STATUS_EXPDON1_ST          ((unsigned int)1 << 3)
#define ISP_IRQ_INTX_STATUS_TG1_ERR_ST          ((unsigned int)1 << 4)
#define ISP_IRQ_INTX_STATUS_VS2_ST              ((unsigned int)1 << 5)
#define ISP_IRQ_INTX_STATUS_TG2_ST1             ((unsigned int)1 << 6)
#define ISP_IRQ_INTX_STATUS_TG2_ST2             ((unsigned int)1 << 7)
#define ISP_IRQ_INTX_STATUS_EXPDON2_ST          ((unsigned int)1 << 8)
#define ISP_IRQ_INTX_STATUS_TG2_ERR_ST          ((unsigned int)1 << 9)
#define ISP_IRQ_INTX_STATUS_PASS1_TG1_DON_ST    ((unsigned int)1 << 10)
#define ISP_IRQ_INTX_STATUS_PASS1_TG2_DON_ST    ((unsigned int)1 << 11)
/* #define ISP_IRQ_INTX_STATUS_VEC_DON_ST          ((unsigned int)1 << 12) */
#define ISP_IRQ_INTX_STATUS_CQ_ERR_ST           ((unsigned int)1 << 13)
#define ISP_IRQ_INTX_STATUS_PASS2_DON_ST        ((unsigned int)1 << 14)
#define ISP_IRQ_INTX_STATUS_TPIPE_DON_ST        ((unsigned int)1 << 15)
#define ISP_IRQ_INTX_STATUS_AF_DON_ST           ((unsigned int)1 << 16)
#define ISP_IRQ_INTX_STATUS_FLK_DON_ST          ((unsigned int)1 << 17)
#define ISP_IRQ_INTX_STATUS_FMT_DON_ST          ((unsigned int)1 << 18)
#define ISP_IRQ_INTX_STATUS_CQ_DON_ST           ((unsigned int)1 << 19)
#define ISP_IRQ_INTX_STATUS_IMGO_ERR_ST         ((unsigned int)1 << 20)
#define ISP_IRQ_INTX_STATUS_AAO_ERR_ST          ((unsigned int)1 << 21)
#define ISP_IRQ_INTX_STATUS_LCSO_ERR_ST         ((unsigned int)1 << 22)
#define ISP_IRQ_INTX_STATUS_IMG2O_ERR_ST        ((unsigned int)1 << 23)
#define ISP_IRQ_INTX_STATUS_ESFKO_ERR_ST        ((unsigned int)1 << 24)
#define ISP_IRQ_INTX_STATUS_FLK_ERR_ST          ((unsigned int)1 << 25)
#define ISP_IRQ_INTX_STATUS_LSC_ERR_ST          ((unsigned int)1 << 26)
/* #define ISP_IRQ_INTX_STATUS_LSC2_ERR_ST         ((unsigned int)1 << 27) */
#define ISP_IRQ_INTX_STATUS_BPC_ERR_ST          ((unsigned int)1 << 28)
#define ISP_IRQ_INTX_STATUS_LCE_ERR_ST          ((unsigned int)1 << 29)
#define ISP_IRQ_INTX_STATUS_DMA_ERR_ST          ((unsigned int)1 << 30)
/* CAM_CTL_DMA_INTX */
#define ISP_IRQ_DMAX_INT_IMGO_DONE_ST           ((unsigned int)1 << 0)
#define ISP_IRQ_DMAX_INT_IMG2O_DONE_ST          ((unsigned int)1 << 1)
#define ISP_IRQ_DMAX_INT_AAO_DONE_ST            ((unsigned int)1 << 2)
#define ISP_IRQ_DMAX_INT_LCSO_DONE_ST           ((unsigned int)1 << 3)
#define ISP_IRQ_DMAX_INT_ESFKO_DONE_ST          ((unsigned int)1 << 4)
#define ISP_IRQ_DMAX_INT_DISPO_DONE_ST          ((unsigned int)1 << 5)
#define ISP_IRQ_DMAX_INT_VIDO_DONE_ST           ((unsigned int)1 << 6)
#define ISP_IRQ_DMAX_INT_VRZO_DONE_ST           ((unsigned int)1 << 7)
#define ISP_IRQ_DMAX_INT_NR3O_DONE_ST           ((unsigned int)1 << 8)
#define ISP_IRQ_DMAX_INT_NR3O_ERR_ST            ((unsigned int)1 << 9)
#define ISP_IRQ_DMAX_INT_CQ_ERR_ST              ((unsigned int)1 << 10)
#define ISP_IRQ_DMAX_INT_BUF_OVL_ST             ((unsigned int)1 << 11)
#define ISP_IRQ_DMAX_INT_TG1_GBERR_ST           ((unsigned int)1 << 12)
#define ISP_IRQ_DMAX_INT_TG2_GBERR_ST           ((unsigned int)1 << 13)

/*******************************************************************************
*
********************************************************************************/
typedef enum {
	ISP_IRQ_CLEAR_NONE,
	ISP_IRQ_CLEAR_WAIT,
	ISP_IRQ_CLEAR_ALL
} ISP_IRQ_CLEAR_ENUM;

typedef enum {
	ISP_IRQ_TYPE_INT,
	ISP_IRQ_TYPE_DMA,
	ISP_IRQ_TYPE_INTB,
	ISP_IRQ_TYPE_DMAB,
	ISP_IRQ_TYPE_INTC,
	ISP_IRQ_TYPE_DMAC,
	ISP_IRQ_TYPE_INTX,
	ISP_IRQ_TYPE_DMAX,
	ISP_IRQ_TYPE_AMOUNT
} ISP_IRQ_TYPE_ENUM;

typedef struct {
	ISP_IRQ_CLEAR_ENUM Clear;
	ISP_IRQ_TYPE_ENUM Type;
	unsigned int Status;
	unsigned int Timeout;
} ISP_WAIT_IRQ_STRUCT;

typedef struct {
	ISP_IRQ_TYPE_ENUM Type;
	unsigned int Status;
} ISP_READ_IRQ_STRUCT;

typedef struct {
	ISP_IRQ_TYPE_ENUM Type;
	unsigned int Status;
} ISP_CLEAR_IRQ_STRUCT;

typedef enum {
	ISP_HOLD_TIME_VD,
	ISP_HOLD_TIME_EXPDONE
} ISP_HOLD_TIME_ENUM;

typedef struct {
	unsigned int Addr;	/* register's addr */
	unsigned int Val;	/* register's value */
} ISP_REG_STRUCT;

typedef struct {
	/* unsigned int Data;   // pointer to ISP_REG_STRUCT */
	ISP_REG_STRUCT *pData;
	unsigned int Count;	/* count */
} ISP_REG_IO_STRUCT;

typedef void (*pIspCallback) (void);

typedef enum {
	/* Work queue. It is interruptible, so there can be "Sleep" in work queue function. */
	ISP_CALLBACK_WORKQUEUE_VD,
	ISP_CALLBACK_WORKQUEUE_EXPDONE,
	ISP_CALLBACK_WORKQUEUE_SENINF,
	/* Tasklet. It is uninterrupted, so there can NOT be "Sleep" in tasklet function. */
	ISP_CALLBACK_TASKLET_VD,
	ISP_CALLBACK_TASKLET_EXPDONE,
	ISP_CALLBACK_TASKLET_SENINF,
	ISP_CALLBACK_AMOUNT
} ISP_CALLBACK_ENUM;

typedef struct {
	ISP_CALLBACK_ENUM Type;
	pIspCallback Func;
} ISP_CALLBACK_STRUCT;

/*  */
/* length of the two memory areas */
#define RT_BUF_TBL_NPAGES 16
#define ISP_RT_BUF_SIZE 16
/* #define ISP_RT_CQ0C_BUF_SIZE (ISP_RT_BUF_SIZE>>2) */
#define ISP_RT_CQ0C_BUF_SIZE (6)	/* special for 6582 */


/*  */
typedef enum {
	_imgi_ = 0,
	_imgci_,		/* 1 */
	_vipi_,			/* 2 */
	_vip2i_,		/* 3 */
	_imgo_,			/* 4 */
	_img2o_,		/* 5 */
	_dispo_,		/* 6 */
	_vido_,			/* 7 */
	_fdo_,			/* 8 */
	_lsci_,			/* 9 */
	_lcei_,			/* 10 */
	_rt_dma_max_
} _isp_dma_enum_;
/*  */
typedef struct {
	unsigned int memID;
	unsigned int size;
	long long base_vAddr;
	unsigned int base_pAddr;
	unsigned int timeStampS;
	unsigned int timeStampUs;
	unsigned int bFilled;
} ISP_RT_BUF_INFO_STRUCT;
/*  */
typedef struct {
	unsigned int count;
	ISP_RT_BUF_INFO_STRUCT data[ISP_RT_BUF_SIZE];
} ISP_DEQUE_BUF_INFO_STRUCT;
/*  */
typedef struct {
	unsigned int start;	/* current DMA accessing buffer */
	unsigned int total_count;	/* total buffer number.Include Filled and empty */
	unsigned int empty_count;	/* total empty buffer number include current DMA accessing buffer */
	unsigned int pre_empty_count;	/* previous total empty buffer number include current DMA accessing buffer */
	unsigned int active;
	ISP_RT_BUF_INFO_STRUCT data[ISP_RT_BUF_SIZE];
} ISP_RT_RING_BUF_INFO_STRUCT;
/*  */
typedef enum {
	ISP_RT_BUF_CTRL_ENQUE,	/* 0 */
	ISP_RT_BUF_CTRL_EXCHANGE_ENQUE,	/* 1 */
	ISP_RT_BUF_CTRL_DEQUE,	/* 2 */
	ISP_RT_BUF_CTRL_IS_RDY,	/* 3 */
	ISP_RT_BUF_CTRL_GET_SIZE,	/* 4 */
	ISP_RT_BUF_CTRL_CLEAR,	/* 5 */
	ISP_RT_BUF_CTRL_MAX
} ISP_RT_BUF_CTRL_ENUM;
/*  */
typedef enum {
	ISP_RTBC_STATE_INIT,	/* 0 */
	ISP_RTBC_STATE_SOF,
	ISP_RTBC_STATE_DONE,
	ISP_RTBC_STATE_MAX
} ISP_RTBC_STATE_ENUM;
/*  */
typedef enum {
	ISP_RTBC_BUF_EMPTY,	/* 0 */
	ISP_RTBC_BUF_FILLED,	/* 1 */
	ISP_RTBC_BUF_LOCKED,	/* 2 */
} ISP_RTBC_BUF_STATE_ENUM;
/*  */
typedef struct {
	ISP_RTBC_STATE_ENUM state;
	unsigned long dropCnt;
	ISP_RT_RING_BUF_INFO_STRUCT ring_buf[_rt_dma_max_];
} ISP_RT_BUF_STRUCT;
/*  */
typedef struct {
	ISP_RT_BUF_CTRL_ENUM ctrl;
	_isp_dma_enum_ buf_id;
	/* unsigned int            data_ptr; */
	/* unsigned int            ex_data_ptr; //exchanged buffer */
	ISP_RT_BUF_INFO_STRUCT *data_ptr;
	ISP_RT_BUF_INFO_STRUCT *ex_data_ptr;	/* exchanged buffer */
	unsigned char *pExtend;
} ISP_BUFFER_CTRL_STRUCT;
/*  */
/* reference count */
#define _use_kernel_ref_cnt_
/*  */
typedef enum {
	ISP_REF_CNT_GET,	/* 0 */
	ISP_REF_CNT_INC,	/* 1 */
	ISP_REF_CNT_DEC,	/* 2 */
	ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE,	/* 3 */
    ISP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE,    /* 4 */
    ISP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE,    /* 5 */
	ISP_REF_CNT_MAX
} ISP_REF_CNT_CTRL_ENUM;
/*  */
typedef enum {
	ISP_REF_CNT_ID_IMEM,	/* 0 */
	ISP_REF_CNT_ID_ISP_FUNC,	/* 1 */
	ISP_REF_CNT_ID_GLOBAL_PIPE,	/* 2 */
    ISP_REF_CNT_ID_P1_PIPE,     /* 3 */
    ISP_REF_CNT_ID_P2_PIPE,     /* 4 */
	ISP_REF_CNT_ID_MAX,
} ISP_REF_CNT_ID_ENUM;
/*  */
typedef struct {
	ISP_REF_CNT_CTRL_ENUM ctrl;
	ISP_REF_CNT_ID_ENUM id;
	signed int *data_ptr;
} ISP_REF_CNT_CTRL_STRUCT;


/********************************************************************************************
 pass1 real time buffer control use cq0c
********************************************************************************************/
/*  */
#define _rtbc_use_cq0c_

#if defined(_rtbc_use_cq0c_)
/*  */
typedef volatile union _CQ_RTBC_FBC_ {
	volatile struct {
		unsigned long FBC_CNT:4;
		unsigned long DROP_INT_EN:1;
		unsigned long rsv_5:6;
		unsigned long RCNT_INC:1;
		unsigned long rsv_12:2;
		unsigned long FBC_EN:1;
		unsigned long LOCK_EN:1;
		unsigned long FB_NUM:4;
		unsigned long RCNT:4;
		unsigned long WCNT:4;
		unsigned long DROP_CNT:4;
	} Bits;
	unsigned long Reg_val;
} CQ_RTBC_FBC;

typedef struct _cq_cmd_st_ {
	unsigned int inst;
	unsigned int data_ptr_pa;
} CQ_CMD_ST;
/*
typedef struct _cq_cmd_rtbc_st_
{
    CQ_CMD_ST imgo;
    CQ_CMD_ST img2o;
    CQ_CMD_ST cq0ci;
    CQ_CMD_ST end;
}CQ_CMD_RTBC_ST;
*/
typedef struct _cq_info_rtbc_st_ {
	CQ_CMD_ST imgo;
	CQ_CMD_ST img2o;
	CQ_CMD_ST next_cq0ci;
	CQ_CMD_ST end;
	unsigned int imgo_base_pAddr;
	unsigned int img2o_base_pAddr;
} CQ_INFO_RTBC_ST;
typedef struct _cq_ring_cmd_st_ {
	CQ_INFO_RTBC_ST cq_rtbc;
	unsigned long next_pa;
	struct _cq_ring_cmd_st_ *pNext;
} CQ_RING_CMD_ST;
typedef struct _cq_rtbc_ring_st_ {
	CQ_RING_CMD_ST rtbc_ring[ISP_RT_CQ0C_BUF_SIZE];
	unsigned int imgo_ring_size;
	unsigned int img2o_ring_size;
} CQ_RTBC_RING_ST;
#endif


#ifdef CONFIG_COMPAT

typedef struct {
	compat_uptr_t pData;
	unsigned int Count;	/* count */
} compat_ISP_REG_IO_STRUCT;

typedef struct {
	ISP_RT_BUF_CTRL_ENUM ctrl;
	_isp_dma_enum_ buf_id;
	compat_uptr_t data_ptr;
	compat_uptr_t ex_data_ptr;	/* exchanged buffer */
	compat_uptr_t pExtend;
} compat_ISP_BUFFER_CTRL_STRUCT;

typedef struct {
	ISP_REF_CNT_CTRL_ENUM ctrl;
	ISP_REF_CNT_ID_ENUM id;
	compat_uptr_t data_ptr;
} compat_ISP_REF_CNT_CTRL_STRUCT;


#endif
/*  */
/********************************************************************************************

********************************************************************************************/


/*******************************************************************************
*
********************************************************************************/
typedef enum {
	ISP_CMD_RESET,		/* Reset */
	ISP_CMD_RESET_BUF,
	ISP_CMD_READ_REG,	/* Read register from driver */
	ISP_CMD_WRITE_REG,	/* Write register to driver */
	ISP_CMD_HOLD_TIME,
	ISP_CMD_HOLD_REG,	/* Hold reg write to hw, on/off */
	ISP_CMD_WAIT_IRQ,	/* Wait IRQ */
	ISP_CMD_READ_IRQ,	/* Read IRQ */
	ISP_CMD_CLEAR_IRQ,	/* Clear IRQ */
	ISP_CMD_DUMP_REG,	/* Dump ISP registers , for debug usage */
	ISP_CMD_SET_USER_PID,	/* for signal */
	ISP_CMD_RT_BUF_CTRL,	/* for pass buffer control */
	ISP_CMD_REF_CNT,	/* get imem reference count */
	ISP_CMD_DEBUG_FLAG,	/* Dump message level */
	ISP_CMD_WAKELOCK_CTRL,	/* isp wakelock control */
	ISP_CMD_SENSOR_FREQ_CTRL	/* sensor frequence control */
} ISP_CMD_ENUM;
/*  */
#define ISP_RESET           _IO(ISP_MAGIC, ISP_CMD_RESET)
#define ISP_RESET_BUF       _IO(ISP_MAGIC, ISP_CMD_RESET_BUF)
#define ISP_READ_REGISTER   _IOWR(ISP_MAGIC, ISP_CMD_READ_REG,      ISP_REG_IO_STRUCT)
#define ISP_WRITE_REGISTER  _IOWR(ISP_MAGIC, ISP_CMD_WRITE_REG,     ISP_REG_IO_STRUCT)
#define ISP_HOLD_REG_TIME   _IOW(ISP_MAGIC, ISP_CMD_HOLD_TIME,     ISP_HOLD_TIME_ENUM)
#define ISP_HOLD_REG        _IOW(ISP_MAGIC, ISP_CMD_HOLD_REG,      bool)
#define ISP_WAIT_IRQ        _IOW(ISP_MAGIC, ISP_CMD_WAIT_IRQ,      ISP_WAIT_IRQ_STRUCT)
#define ISP_READ_IRQ        _IOR(ISP_MAGIC, ISP_CMD_READ_IRQ,      ISP_READ_IRQ_STRUCT)
#define ISP_CLEAR_IRQ       _IOW(ISP_MAGIC, ISP_CMD_CLEAR_IRQ,     ISP_CLEAR_IRQ_STRUCT)
#define ISP_DUMP_REG        _IO(ISP_MAGIC, ISP_CMD_DUMP_REG)
#define ISP_SET_USER_PID    _IOW(ISP_MAGIC, ISP_CMD_SET_USER_PID,  unsigned long)
#define ISP_BUFFER_CTRL     _IOWR(ISP_MAGIC, ISP_CMD_RT_BUF_CTRL,   ISP_BUFFER_CTRL_STRUCT)
#define ISP_REF_CNT_CTRL    _IOWR(ISP_MAGIC, ISP_CMD_REF_CNT,       ISP_REF_CNT_CTRL_STRUCT)
#define ISP_DEBUG_FLAG      _IOW(ISP_MAGIC, ISP_CMD_DEBUG_FLAG,    unsigned long)
#define ISP_WAKELOCK_CTRL   _IOWR(ISP_MAGIC, ISP_CMD_WAKELOCK_CTRL, unsigned long)
#define ISP_SENSOR_FREQ_CTRL  _IOW(ISP_MAGIC, ISP_CMD_SENSOR_FREQ_CTRL, unsigned long)

#ifdef CONFIG_COMPAT
#define COMPAT_ISP_READ_REGISTER    _IOWR(ISP_MAGIC, ISP_CMD_READ_REG,      compat_ISP_REG_IO_STRUCT)
#define COMPAT_ISP_WRITE_REGISTER   _IOWR(ISP_MAGIC, ISP_CMD_WRITE_REG,     compat_ISP_REG_IO_STRUCT)
#define COMPAT_ISP_BUFFER_CTRL      _IOWR(ISP_MAGIC, ISP_CMD_RT_BUF_CTRL,   compat_ISP_BUFFER_CTRL_STRUCT)
#define COMPAT_ISP_REF_CNT_CTRL     _IOWR(ISP_MAGIC, ISP_CMD_REF_CNT,       compat_ISP_REF_CNT_CTRL_STRUCT)
#define COMPAT_ISP_SET_USER_PID     _IOW(ISP_MAGIC, ISP_CMD_SET_USER_PID,  compat_uptr_t)
#define COMPAT_ISP_DEBUG_FLAG       _IOW(ISP_MAGIC, ISP_CMD_DEBUG_FLAG,    compat_uptr_t)
#define COMPAT_ISP_WAKELOCK_CTRL    _IOWR(ISP_MAGIC, ISP_CMD_WAKELOCK_CTRL, compat_uptr_t)
#define COMPAT_ISP_SENSOR_FREQ_CTRL  _IOW(ISP_MAGIC, ISP_CMD_SENSOR_FREQ_CTRL, compat_uptr_t)
#endif

/*  */
bool ISP_RegCallback(ISP_CALLBACK_STRUCT *pCallback);
bool ISP_UnregCallback(ISP_CALLBACK_ENUM Type);

bool ISP_ControlMdpClock(bool en);
int32_t ISP_MDPClockOnCallback(uint64_t engineFlag);
int32_t ISP_MDPDumpCallback(uint64_t engineFlag, int level);
int32_t ISP_MDPResetCallback(uint64_t engineFlag);

int32_t ISP_MDPClockOffCallback(uint64_t engineFlag);

/*  */
#endif
