/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __CCU_DRV_H__
#define __CCU_DRV_H__

#include <linux/types.h>
#include <linux/ioctl.h>
#include "ccu_mailbox_extif.h"

#ifdef CONFIG_COMPAT
/*64 bit*/
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#define CCU_NUM_PORTS 32

typedef uint8_t ccu_id_t;

/* the last byte of string must be '/0' */
typedef char ccu_name_t[32];

#define SIG_ERESTARTSYS 512
/*---------------------------------------------------------------------------*/
/*  CCU IRQ                                                                */
/*---------------------------------------------------------------------------*/
#define CCU_REG_BASE_HW           0x18000000
/*#define CCU_A_BASE_HW   0x1A057400*/ /*kibo+*/
#define CCU_A_BASE_HW           0x180B1000	/*whitney */
#define CCU_CAMSYS_BASE_HW  0x18000000
#define CCU_REG_PER_CCU_RANGE   (PAGE_SIZE*5)

#define CCU_PMEM_BASE_HW        0x180A0000
#define CCU_PMEM_RANGE           (PAGE_SIZE*6)	/* 24KB */
#define CCU_PMEM_SIZE           (24*1024)	/* 24KB */

#define CCU_DMEM_BASE_HW        0x18080000
#define CCU_DMEM_RANGE           (PAGE_SIZE*8)	/* 32KB */
#define CCU_DMEM_SIZE           (32*1024)	/* 32KB */

#define CCU_MMAP_LOG0       0x10000000
#define CCU_MMAP_LOG1       0x10100000
#define CCU_MMAP_LOG2       0x10200000

/*---------------------------------------------------------------------------*/
/*  CCU IRQ                                                                */
/*---------------------------------------------------------------------------*/
/* In order with the suquence of device nodes defined in dtsi */
typedef enum {
	CCU_A_IDX = 0,
	CCU_CAMSYS_IDX,		/* Remider: Add this device node manually in .dtsi */
	CCU_DEV_NODE_NUM
} CCU_DEV_NODE_ENUM;

/*---------------------------------------------------------------------------*/
/*  CCU IRQ                                                                */
/*---------------------------------------------------------------------------*/
#define IRQ_USER_NUM_MAX        32
#define SUPPORT_MAX_IRQ         32


typedef enum {
	CCU_IRQ_CLEAR_NONE,	/* non-clear wait, clear after wait */
	CCU_IRQ_CLEAR_WAIT,	/* clear wait, clear before and after wait */
	CCU_IRQ_CLEAR_STATUS,	/* clear specific status only */
	CCU_IRQ_CLEAR_ALL	/* clear all status */
} CCU_IRQ_CLEAR_ENUM;

typedef enum {
	CCU_IRQ_TYPE_INT_CCU_ST,
	CCU_IRQ_TYPE_INT_CCU_A_ST,
	CCU_IRQ_TYPE_INT_CCU_B_ST,
	CCU_IRQ_TYPE_AMOUNT
} CCU_IRQ_TYPE_ENUM;

typedef enum {
	CCU_SIGNAL_INT = 0, CCU_DMA_INT, CCU_IRQ_ST_AMOUNT
} CCU_ST_ENUM;

typedef struct {
	unsigned int tLastSig_sec;
	/* time stamp of the latest occurring signal */
	unsigned int tLastSig_usec;
	/* time stamp of the latest occurring signal */
	unsigned int tMark2WaitSig_sec;
	/* time period from marking a signal to user try to wait and get the signal */
	unsigned int tMark2WaitSig_usec;
	/* time period from marking a signal to user try to wait and get the signal */
	unsigned int tLastSig2GetSig_sec;
	/* time period from latest signal to user try to wait and get the signal */
	unsigned int tLastSig2GetSig_usec;
	/* time period from latest signal to user try to wait and get the signal */
	int passedbySigcnt;	/* the count for the signal passed by  */
} CCU_IRQ_TIME_STRUCT;

typedef struct {
	CCU_IRQ_CLEAR_ENUM Clear;
	CCU_ST_ENUM St_type;
	unsigned int Status;
	int UserKey;		/* user key for doing interrupt operation */
	unsigned int Timeout;
	CCU_IRQ_TIME_STRUCT TimeInfo;
} CCU_WAIT_IRQ_ST;

typedef struct {
	int userKey;
	char userName[32];
} CCU_REGISTER_USERKEY_STRUCT;

typedef struct {
	CCU_IRQ_TYPE_ENUM Type;
	unsigned int bDumpReg;
	CCU_WAIT_IRQ_ST EventInfo;
} CCU_WAIT_IRQ_STRUCT;

typedef struct {
	int UserKey;		/* user key for doing interrupt operation */
	CCU_ST_ENUM St_type;
	unsigned int Status;
} CCU_CLEAR_IRQ_ST;

typedef struct {
	CCU_IRQ_TYPE_ENUM Type;
	CCU_CLEAR_IRQ_ST EventInfo;
} CCU_CLEAR_IRQ_STRUCT;

typedef struct {
	unsigned int module;
	unsigned int Addr;	/* register's addr */
	unsigned int Val;	/* register's value */
} CCU_REG_STRUCT;

typedef struct {
	CCU_REG_STRUCT *pData;	/* pointer to CCU_REG_STRUCT */
	unsigned int Count;	/* count */
} CCU_REG_IO_STRUCT;

typedef struct {
	/* Add an extra index for status type in Everest -> signal or dma */
	volatile unsigned int Status[CCU_IRQ_TYPE_AMOUNT][CCU_IRQ_ST_AMOUNT][IRQ_USER_NUM_MAX];
	unsigned int Mask[CCU_IRQ_TYPE_AMOUNT][CCU_IRQ_ST_AMOUNT];
	unsigned int ErrMask[CCU_IRQ_TYPE_AMOUNT][CCU_IRQ_ST_AMOUNT];
	signed int WarnMask[CCU_IRQ_TYPE_AMOUNT][CCU_IRQ_ST_AMOUNT];
	/* flag for indicating that user do mark for a interrupt or not */
	volatile unsigned int MarkedFlag[CCU_IRQ_TYPE_AMOUNT][CCU_IRQ_ST_AMOUNT][IRQ_USER_NUM_MAX];
	/* time for marking a specific interrupt */
	volatile unsigned int MarkedTime_sec[CCU_IRQ_TYPE_AMOUNT][32][IRQ_USER_NUM_MAX];
	/* time for marking a specific interrupt */
	volatile unsigned int MarkedTime_usec[CCU_IRQ_TYPE_AMOUNT][32][IRQ_USER_NUM_MAX];
	/* number of a specific signal that passed by */
	volatile signed int PassedBySigCnt[CCU_IRQ_TYPE_AMOUNT][32][IRQ_USER_NUM_MAX];
	/* */
	volatile unsigned int LastestSigTime_sec[CCU_IRQ_TYPE_AMOUNT][32];
	/* latest time for each interrupt */
	volatile unsigned int LastestSigTime_usec[CCU_IRQ_TYPE_AMOUNT][32];
	/* latest time for each interrupt */
} CCU_IRQ_INFO_STRUCT;

#define CCU_ISR_MAX_NUM 32
#define INT_ERR_WARN_TIMER_THREAS 1000
#define INT_ERR_WARN_MAX_TIME 3
typedef struct {
	unsigned int m_err_int_cnt[CCU_IRQ_TYPE_AMOUNT][CCU_ISR_MAX_NUM];	/* cnt for each err int # */
	unsigned int m_warn_int_cnt[CCU_IRQ_TYPE_AMOUNT][CCU_ISR_MAX_NUM];	/* cnt for each warning int # */
	unsigned int m_err_int_mark[CCU_IRQ_TYPE_AMOUNT];	/* mark for err int, where its cnt > threshold */
	unsigned int m_warn_int_mark[CCU_IRQ_TYPE_AMOUNT];	/* mark for warn int, where its cnt > threshold */
	unsigned long m_int_usec[CCU_IRQ_TYPE_AMOUNT];
} CCU_IRQ_ERR_WAN_CNT_STRUCT;

#define CCU_BUF_SIZE            (4096)
#define CCU_BUF_SIZE_WRITE      1024
#define CCU_BUF_WRITE_AMOUNT    6
typedef enum {
	CCU_BUF_STATUS_EMPTY,
	CCU_BUF_STATUS_HOLD,
	CCU_BUF_STATUS_READY
} CCU_BUF_STATUS_ENUM;

typedef struct {
	volatile CCU_BUF_STATUS_ENUM Status;
	volatile unsigned int Size;
	unsigned char *pData;
} CCU_BUF_STRUCT;

typedef struct {
	CCU_BUF_STRUCT Read;
	CCU_BUF_STRUCT Write[CCU_BUF_WRITE_AMOUNT];
} CCU_BUF_INFO_STRUCT;

typedef struct {
	unsigned int Vd;
	unsigned int Expdone;
	unsigned int WorkQueueVd;
	unsigned int WorkQueueExpdone;
	unsigned int TaskletVd;
	unsigned int TaskletExpdone;
} CCU_TIME_LOG_STRUCT;

#ifdef MTK_VPU_KERNEL
typedef struct {
	spinlock_t SpinLockCcuRef;
	spinlock_t SpinLockCcu;
	spinlock_t SpinLockIrq[CCU_IRQ_TYPE_AMOUNT];
	spinlock_t SpinLockIrqCnt[CCU_IRQ_TYPE_AMOUNT];
	spinlock_t SpinLockRTBC;
	spinlock_t SpinLockClock;

	wait_queue_head_t WaitQueueHead;
	volatile wait_queue_head_t WaitQHeadList[SUPPORT_MAX_IRQ];

	unsigned int UserCount;
	unsigned int DebugMask;
	signed int IrqNum;
	CCU_IRQ_INFO_STRUCT IrqInfo;
	CCU_IRQ_ERR_WAN_CNT_STRUCT IrqCntInfo;
	CCU_BUF_INFO_STRUCT BufInfo;
	CCU_TIME_LOG_STRUCT TimeLog;
} CCU_INFO_STRUCT;
#endif

/*---------------------------------------------------------------------------*/
/*  CCU Attrs                                                                */
/*---------------------------------------------------------------------------*/
typedef enum ccu_attr_type_e {
	CCU_ATTR_TYPE_BYTE,
	CCU_ATTR_TYPE_INT32,
	CCU_ATTR_TYPE_INT64,
	CCU_ATTR_TYPE_FLOAT,
	CCU_ATTR_TYPE_DOUBLE,
	CCU_NUM_ATTR_TYPES
} ccu_attr_type_t;

typedef enum ccu_attr_access_e {
	CCU_ATTR_ACCESS_RDONLY,
	CCU_ATTR_ACCESS_RDWR
} ccu_attr_access_t;

/*
 * The description of attributes contains the information about attribute values,
 * which are stored as compact memory. With the offset, it can get the specific value
 * from compact data.
 *
 * The example of ccu_attr_desc_t is as follows:
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   id   | name                | offset | type   | count | access |
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   0    | reserved            | 0      | int32  | 64    | RDONLY |
 *   +--------+---------------------+--------+--------+-------+--------+
 *
 */
typedef struct ccu_attr_desc_s {
	ccu_id_t id;
	ccu_name_t name;
	/* offset = previous offset + previous size */
	uint32_t offset;
	/* size = sizeof(type) x count */
	ccu_attr_type_t type;
	uint32_t count;
	/* directional data exchange */
	ccu_attr_access_t access;
} ccu_attr_desc_t;

/*
 * The attribute's value is a compact-format data, which could be read by attr_desc.
 * According to the attr_desc's offset, it could locate to the start address of value.
 *
 * The example of compact-format memory is as follows:
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *   |    0   |    8   |   16   |   24   |   32   |   40   |   48   |   56   |
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *   | alg-ver|       working-buffer-size         | cnt-mem|                 |
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *
 */
typedef struct ccu_attrs_s {
	/* use the global attribute description */
	uint64_t ptr;
	uint32_t length;
#if 0
	/* if each algo has its own attr_desc */
	uint8_t attr_desc_count;
	ccu_attr_desc_t *attr_descs;
#endif
} ccu_attrs_t;


/*---------------------------------------------------------------------------*/
/*  CCU Ports                                                                */
/*---------------------------------------------------------------------------*/
typedef enum ccu_buf_type_e {
	CCU_BUF_TYPE_IMAGE,
	CCU_BUF_TYPE_DATA,
	CCU_NUM_BUF_TYPES
} ccu_buf_type_t;

typedef enum ccu_port_dir_e {
	CCU_PORT_DIR_IN,
	CCU_PORT_DIR_OUT,
	CCU_PORT_DIR_IN_OUT,
	CCU_NUM_PORT_DIRS
} ccu_port_dir_t;

/*
 * The ports contains the information about algorithm's input and output.
 * The each buffer on the ccu_command should be assigned a port id,
 * to let algorithm recognize every buffer's purpose.
 *
 * The example of ccu_ports_t is as follows:
 *   +--------+---------------------+--------+--------+
 *   |   id   | name                | type   | dir    |
 *   +--------+---------------------+--------+--------+
 *   |   0    | image-in            | IMAGE  | IN     |
 *   +--------+---------------------+--------+--------+
 *   |   1    | data-in             | DATA   | IN     |
 *   +--------+---------------------+--------+--------+
 *   |   2    | image-out           | IMAGE  | OUT    |
 *   +--------+---------------------+--------+--------+
 *   |   3    | image-temp          | IMAGE  | INOUT  |
 *   +--------+---------------------+--------+--------+
 *
 */
typedef struct ccu_port_s {
	ccu_id_t id;
	ccu_name_t name;
	ccu_buf_type_t type;
	ccu_port_dir_t dir;
} ccu_port_t;

typedef struct ccu_ports_s {
	uint8_t port_count;
	ccu_port_t ports[CCU_NUM_PORTS];
} ccu_ports_t;


/*---------------------------------------------------------------------------*/
/*  CCU Algo                                                                 */
/*---------------------------------------------------------------------------*/
typedef struct ccu_algo_s {
	ccu_id_t id;
	ccu_name_t name;
	ccu_attrs_t attrs;
	ccu_ports_t ports;
	/* mva of kernel loader, the address is CCU accessible */
	uint64_t ptr;
	uint32_t length;
} ccu_algo_t;

/*---------------------------------------------------------------------------*/
/*  CCU Register                                                             */
/*---------------------------------------------------------------------------*/
typedef struct ccu_reg_value_s {
	uint64_t ptr;
	uint32_t value;
} ccu_reg_value_t;

typedef struct ccu_reg_values_s {
	ccu_reg_value_t *regs;
	uint8_t reg_count;
} ccu_reg_values_t;


/*---------------------------------------------------------------------------*/
/*  CCU working buffer                                                                */
/*---------------------------------------------------------------------------*/
#define SIZE_32BYTE        (32)
#define SIZE_1MB        (1024*1024)
#define SIZE_1MB_PWR2PAGE   (8)
#define MAX_I2CBUF_NUM         1
#define MAX_MAILBOX_NUM 2
#define MAX_LOG_BUF_NUM 3

#define MAILBOX_SEND 0
#define MAILBOX_GET 1

typedef struct ccu_working_buffer_s {
	uint8_t *va_pool;
	uint32_t mva_pool;
	uint32_t sz_pool;

	uint8_t *va_i2c;	/* i2c buffer mode */
	uint32_t mva_i2c;
	uint32_t sz_i2c;

	uint8_t *va_mb[MAX_MAILBOX_NUM];	/* mailbox              */
	uint32_t mva_mb[MAX_MAILBOX_NUM];
	uint32_t sz_mb[MAX_MAILBOX_NUM];

	char *va_log[MAX_LOG_BUF_NUM];	/* log buffer           */
	uint32_t mva_log[MAX_LOG_BUF_NUM];
	uint32_t sz_log[MAX_LOG_BUF_NUM];
	int32_t fd_log[MAX_LOG_BUF_NUM];
} ccu_working_buffer_t;

#ifdef CONFIG_COMPAT
typedef struct compat_ccu_working_buffer_s {
	compat_uptr_t va_pool;
	uint32_t mva_pool;
	uint32_t sz_pool;

	compat_uptr_t va_i2c;	/* i2c buffer mode */
	uint32_t mva_i2c;
	uint32_t sz_i2c;

	compat_uptr_t va_mb[MAX_MAILBOX_NUM];	/* mailbox              */
	uint32_t mva_mb[MAX_MAILBOX_NUM];
	uint32_t sz_mb[MAX_MAILBOX_NUM];

	compat_uptr_t va_log[MAX_LOG_BUF_NUM];	/* log buffer           */
	uint32_t mva_log[MAX_LOG_BUF_NUM];
	uint32_t sz_log[MAX_LOG_BUF_NUM];
	int32_t fd_log[MAX_LOG_BUF_NUM];
} compat_ccu_working_buffer_t;
#endif
/*---------------------------------------------------------------------------*/
/*  CCU Power                                                                */
/*---------------------------------------------------------------------------*/
typedef struct ccu_power_s {
	uint32_t bON;
	uint32_t freq;
	uint32_t power;
	ccu_working_buffer_t workBuf;
} ccu_power_t;

#ifdef CONFIG_COMPAT
typedef struct compat_ccu_power_s {
	uint32_t bON;
	uint32_t freq;
	uint32_t power;
	compat_ccu_working_buffer_t workBuf;
} compat_ccu_power_t;
#endif
/*---------------------------------------------------------------------------*/
/*  CCU command                                                              */
/*---------------------------------------------------------------------------*/
typedef struct ccu_plane_s {
	/* if buffer type is image */
	uint32_t stride;
	/* mva, the address is CCU accessible */
	uint64_t ptr;
	uint32_t length;
} ccu_plane_t;

typedef enum ccu_buf_format_e {
	CCU_BUF_FORMAT_METADATA,
	CCU_BUF_FORMAT_IMG_YV12,
	CCU_BUF_FORMAT_IMG_NV21,
	CCU_BUF_FORMAT_IMG_YUY2,
} ccu_buf_format_t;

typedef struct ccu_buffer_s {
	uint8_t plane_count;
	ccu_plane_t planes[3];
	ccu_buf_format_t format;
	uint32_t width;
	uint32_t height;
	ccu_id_t port_id;
} ccu_buffer_t;


typedef enum ccu_eng_status_e {
	CCU_ENG_STATUS_SUCCESS,
	CCU_ENG_STATUS_BUSY,
	CCU_ENG_STATUS_TIMEOUT,
	CCU_ENG_STATUS_INVALID,
	CCU_ENG_STATUS_FLUSH,
	CCU_ENG_STATUS_FAILURE,
} ccu_eng_status_t;

/*js_remove*/
#if 0
typedef struct ccu_cmd_s {
	uint8_t buffer_count;
	ccu_buffer_t buffers[CCU_NUM_PORTS];
	ccu_id_t algo_id;
	ccu_eng_status_t status;
	uint64_t priv;
} ccu_cmd_st;
#endif

/*---------------------------------------------------------------------------*/
/*  CCU Command                                                                */
/*---------------------------------------------------------------------------*/
typedef struct ccu_cmd_s {
	ccu_msg_t task;
	ccu_eng_status_t status;
} ccu_cmd_st;

/*---------------------------------------------------------------------------*/
/*  IOCTL Command                                                            */
/*---------------------------------------------------------------------------*/
#define CCU_MAGICNO             'c'
#define CCU_IOCTL_SET_POWER                _IOW(CCU_MAGICNO,   0, int)
#define CCU_IOCTL_ENQUE_COMMAND            _IOW(CCU_MAGICNO,   1, int)
#define CCU_IOCTL_DEQUE_COMMAND            _IOWR(CCU_MAGICNO,  2, int)
#define CCU_IOCTL_FLUSH_COMMAND            _IOW(CCU_MAGICNO,   3, int)
#define CCU_IOCTL_WAIT_IRQ                 _IOW(CCU_MAGICNO,   9, int)
#define CCU_IOCTL_SEND_CMD                 _IOWR(CCU_MAGICNO, 10, int)
#define CCU_IOCTL_SET_RUN                  _IO(CCU_MAGICNO,   11)

#define CCU_CLEAR_IRQ                      _IOW(CCU_MAGICNO,  12, int)
#define CCU_REGISTER_IRQ_USER_KEY          _IOR(CCU_MAGICNO,  13, int)
#define CCU_READ_REGISTER                  _IOWR(CCU_MAGICNO, 14, int)
#define CCU_WRITE_REGISTER                 _IOWR(CCU_MAGICNO, 15, int)

#define CCU_IOCTL_SET_WORK_BUF             _IOW(CCU_MAGICNO,  18, int)
#define CCU_IOCTL_FLUSH_LOG                _IOW(CCU_MAGICNO,  19, int)

#define CCU_IOCTL_GET_I2C_DMA_BUF_ADDR     _IOR(CCU_MAGICNO,  20, int)
#define CCU_IOCTL_SET_I2C_MODE             _IOW(CCU_MAGICNO,  21, int)
#define CCU_IOCTL_SET_I2C_CHANNEL          _IOW(CCU_MAGICNO,  22, int)
#define CCU_IOCTL_GET_CURRENT_FPS          _IOR(CCU_MAGICNO,  23, int)



/*---------------------------------------------------------------------------*/
/*        i2c interface from ccu_drv.c */
/*---------------------------------------------------------------------------*/
enum CCU_I2C_CHANNEL {
	CCU_I2C_CHANNEL_UNDEF = 0x0,
	CCU_I2C_CHANNEL_MAINCAM = 0x1,
	CCU_I2C_CHANNEL_SUBCAM = 0x2
};

#if 1
struct ccu_i2c_arg {
	unsigned int i2c_write_id;
	unsigned int transfer_len;
};

extern void i2c_get_dma_buffer_addr(void **va);
extern int ccu_i2c_buf_mode_en(int enable);
extern int ccu_init_i2c_buf_mode(unsigned short i2cId);
extern int ccu_config_i2c_buf_mode(int transfer_len);
extern int ccu_i2c_frame_reset(void);
extern int ccu_trigger_i2c(int transac_len, MBOOL do_dma_en);
#endif

#endif
