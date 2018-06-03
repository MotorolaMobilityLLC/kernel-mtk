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

#ifndef _MT_DIP_H
#define _MT_DIP_H

#include <linux/ioctl.h>

#ifndef CONFIG_OF
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

m4u_callback_ret_t DIP_M4U_TranslationFault_callback(int port, unsigned int mva, void *data);

/**
 * enforce kernel log enable
 */
#define KERNEL_LOG
#define ISR_LOG_ON

#define SIG_ERESTARTSYS 512
/*******************************************************************************
 *
 ********************************************************************************/
#define DIP_DEV_MAJOR_NUMBER    251
#define DIP_MAGIC               'D'

#define DIP_A_BASE_HW   0x15022000

#define DIP_REG_RANGE           (PAGE_SIZE)
#define DIP_REG_PER_DIP_RANGE   (PAGE_SIZE*6)

/* In order with the suquence of device nodes defined in dtsi */
enum DIP_DEV_NODE_ENUM {
	/*DIP_IMGSYS_CONFIG_IDX = 0,*/
	DIP_DIP_A_IDX, /* Remider: Add this device node manually in .dtsi */
	DIP_DEV_NODE_NUM
};

/**
 * interrupt clear type
 */
enum DIP_IRQ_CLEAR_ENUM {
	DIP_IRQ_CLEAR_NONE, /* non-clear wait, clear after wait */
	DIP_IRQ_CLEAR_WAIT, /* clear wait, clear before and after wait */
	DIP_IRQ_CLEAR_STATUS, /* clear specific status only */
	DIP_IRQ_CLEAR_ALL /* clear all status */
};

/**
 * module's interrupt , each module should have its own isr.
 * note:
 * mapping to isr table,ISR_TABLE when using no device tree
 */
enum DIP_IRQ_TYPE_ENUM {
	DIP_IRQ_TYPE_INT_DIP_A_ST,
	DIP_IRQ_TYPE_AMOUNT
};

struct DIP_WAIT_IRQ_ST {
	enum DIP_IRQ_CLEAR_ENUM Clear;
	unsigned int Status; /*ref. enum:ENUM_CAM_INT / ENUM_CAM_DMA_INT ...etc in dip_drv_stddef.h*/
	int UserKey; /* user key for doing interrupt operation */
	unsigned int Timeout;
};

struct DIP_WAIT_IRQ_STRUCT {
	enum DIP_IRQ_TYPE_ENUM Type;
	unsigned int bDumpReg;
	struct DIP_WAIT_IRQ_ST EventInfo;
};

struct DIP_REGISTER_USERKEY_STRUCT {
	int userKey;
	char userName[32]; /* this size must the same as the icamiopipe api - registerIrq(...) */
};

struct DIP_CLEAR_IRQ_ST {
	int UserKey; /* user key for doing interrupt operation */
	unsigned int Status;
};

struct DIP_CLEAR_IRQ_STRUCT {
	enum DIP_IRQ_TYPE_ENUM Type;
	struct DIP_CLEAR_IRQ_ST EventInfo;
};

struct DIP_REG_STRUCT {
	unsigned int module; /*plz refer to DIP_DEV_NODE_ENUM */
	unsigned int Addr; /* register's addr */
	unsigned int Val; /* register's value */
};

struct DIP_REG_IO_STRUCT {
	struct DIP_REG_STRUCT *pData; /* pointer to DIP_REG_STRUCT */
	unsigned int Count; /* count */
};

#ifdef CONFIG_COMPAT
struct compat_DIP_REG_IO_STRUCT {
	compat_uptr_t pData;
	unsigned int Count; /* count */
};
#endif

enum DIP_DUMP_CMD {
	DIP_DUMP_TPIPEBUF_CMD = 0,
	DIP_DUMP_TUNINGBUF_CMD,
	DIP_DUMP_DIPVIRBUF_CMD,
	DIP_DUMP_CMDQVIRBUF_CMD
};


struct DIP_DUMP_BUFFER_STRUCT {
	unsigned int DumpCmd;
	unsigned int *pBuffer;
	unsigned int BytesofBufferSize;
};

struct DIP_GET_DUMP_INFO_STRUCT {
	unsigned int extracmd;
	unsigned int imgi_baseaddr;
	unsigned int tdri_baseaddr;
	unsigned int dmgi_baseaddr;
};

enum DIP_MEMORY_INFO_CMD {
	DIP_MEMORY_INFO_TPIPE_CMD = 1,
	DIP_MEMORY_INFO_CMDQ_CMD
};

struct DIP_MEM_INFO_STRUCT {
	unsigned int MemInfoCmd;
	unsigned int MemPa;
	unsigned int *MemVa;
	unsigned int MemSizeDiff;
};

#ifdef CONFIG_COMPAT
struct compat_DIP_DUMP_BUFFER_STRUCT {
	unsigned int DumpCmd;
	compat_uptr_t pBuffer;
	unsigned int BytesofBufferSize;
};
struct compat_DIP_MEM_INFO_STRUCT {
	unsigned int MemInfoCmd;
	unsigned int MemPa;
	compat_uptr_t MemVa;
	unsigned int MemSizeDiff;
};

#endif

#define DIP_DIP_PHYSICAL_REG_SIZE (4096*3)
#define DIP_DIP_REG_SIZE (4096*4)
#define MAX_TILE_TOT_NO (256)
#define MAX_DIP_DUMP_HEX_PER_TILE (256)
#define MAX_DIP_TILE_TDR_HEX_NO (MAX_TILE_TOT_NO*MAX_DIP_DUMP_HEX_PER_TILE)
#define MAX_DIP_CMDQ_BUFFER_SIZE (0x1000)

/* for keep ion handle */
enum DIP_WRDMA_ENUM {
#if 0
	_dma_cq0i_ = 0,/* 0168 */
	_dma_cq0i_vir, /* 0168 */
	_dma_cq1i_,    /* 0174 */
	_dma_cq2i_,    /* 0180 */
	_dma_cq3i_,    /* 018c */
	_dma_cq4i_,    /* 0198 *//*5*/
	_dma_cq5i_,    /* 01a4 */
	_dma_cq6i_,    /* 01b0 */
	_dma_cq7i_,    /* 01bc */
	_dma_cq8i_,    /* 01c8 */
	_dma_cq9i_,    /* 01d4 *//*10*/
	_dma_cq10i_,   /* 01e0 */
	_dma_cq11i_,   /* 01ec */
	_dma_cq12i_,   /* 01f8 */
	_dma_bpci_,    /* 0370 */
	_dma_caci_,    /* 03a0 *//*15*/
	_dma_lsci_,    /* 03d0 */
	_dma_imgo_,    /* 0220 */
	_dma_rrzo_,    /* 0250 */
	_dma_aao_,     /* 0280 */
	_dma_afo_,     /* 02b0 *//*20*/
	_dma_lcso_,    /* 02e0 */
	_dma_ufeo_,    /* 0310 */
	_dma_pdo_,     /* 0340 */
	_dma_eiso_,    /* 0220 */
	_dma_flko_,    /* 0250 *//*25*/
	_dma_rsso_,    /* 0280 */
	_dma_pso_,     /* 0D80 */
	_dma_imgo_fh_, /* 0c04 */
	_dma_rrzo_fh_, /* 0c08 */
	_dma_aao_fh_,  /* 0c0c *//*30*/
	_dma_afo_fh_,  /* 0c10 */
	_dma_lcso_fh_, /* 0c14 */
	_dma_ufeo_fh_, /* 0c18 */
	_dma_pdo_fh_,  /* 0c1c */
	_dma_eiso_fh_,  /* 03C4 *//*35*/
	_dma_flko_fh_, /* 03C8 */
	_dma_rsso_fh_, /* 03CC */
	_dma_pso_fh_,  /* 0E20 */
	_dma_max_wr_
#endif
	_dip_dma_max_wr_
};

struct DIP_DEV_ION_NODE_STRUCT {
	unsigned int       devNode; /*plz refer to DIP_DEV_NODE_ENUM*/
	enum DIP_WRDMA_ENUM     dmaPort;
	int                memID;
};


/* struct for enqueue/dequeue control in ihalpipe wrapper */
enum DIP_P2_BUFQUE_CTRL_ENUM {
	DIP_P2_BUFQUE_CTRL_ENQUE_FRAME = 0, /* 0,signal that a specific buffer is enqueued */
	DIP_P2_BUFQUE_CTRL_WAIT_DEQUE, /* 1,a dequeue thread is waiting to do dequeue */
	DIP_P2_BUFQUE_CTRL_DEQUE_SUCCESS, /* 2,signal that a buffer is dequeued (success) */
	DIP_P2_BUFQUE_CTRL_DEQUE_FAIL, /* 3,signal that a buffer is dequeued (fail) */
	DIP_P2_BUFQUE_CTRL_WAIT_FRAME, /* 4,wait for a specific buffer */
	DIP_P2_BUFQUE_CTRL_WAKE_WAITFRAME, /* 5,wake all slept users to check buffer is dequeued or not */
	DIP_P2_BUFQUE_CTRL_CLAER_ALL, /* 6,free all recored dequeued buffer */
	DIP_P2_BUFQUE_CTRL_MAX
};

enum DIP_P2_BUFQUE_PROPERTY {
	DIP_P2_BUFQUE_PROPERTY_DIP = 0,
	DIP_P2_BUFQUE_PROPERTY_NUM = 1,
	DIP_P2_BUFQUE_PROPERTY_WARP
};

struct DIP_P2_BUFQUE_STRUCT {
	enum DIP_P2_BUFQUE_CTRL_ENUM ctrl;
	enum DIP_P2_BUFQUE_PROPERTY property;
	unsigned int processID; /* judge multi-process */
	unsigned int callerID; /* judge multi-thread and different kinds of buffer type */
	int frameNum; /* total frame number in the enque request */
	int cQIdx; /* cq index */
	int dupCQIdx; /* dup cq index */
	int burstQIdx; /* burst queue index */
	unsigned int timeoutIns; /* timeout for wait buffer */
};

enum DIP_P2_BUF_STATE_ENUM {
	DIP_P2_BUF_STATE_NONE = -1,
	DIP_P2_BUF_STATE_ENQUE = 0,
	DIP_P2_BUF_STATE_RUNNING,
	DIP_P2_BUF_STATE_WAIT_DEQUE_FAIL,
	DIP_P2_BUF_STATE_DEQUE_SUCCESS,
	DIP_P2_BUF_STATE_DEQUE_FAIL
};

enum DIP_P2_BUFQUE_LIST_TAG {
	DIP_P2_BUFQUE_LIST_TAG_PACKAGE = 0, DIP_P2_BUFQUE_LIST_TAG_UNIT
};

enum DIP_P2_BUFQUE_MATCH_TYPE {
	DIP_P2_BUFQUE_MATCH_TYPE_WAITDQ = 0, /* waiting for deque */
	DIP_P2_BUFQUE_MATCH_TYPE_WAITFM, /* wait frame from user */
	DIP_P2_BUFQUE_MATCH_TYPE_FRAMEOP, /* frame operaetion */
	DIP_P2_BUFQUE_MATCH_TYPE_WAITFMEQD /* wait frame enqueued for deque */
};


/********************************************************************************************
 *
 ********************************************************************************************/

/*******************************************************************************
 *
 ********************************************************************************/
enum DIP_CMD_ENUM {
	DIP_CMD_RESET_BY_HWMODULE,
	DIP_CMD_READ_REG, /* Read register from driver */
	DIP_CMD_WRITE_REG, /* Write register to driver */
	DIP_CMD_WAIT_IRQ, /* Wait IRQ */
	DIP_CMD_CLEAR_IRQ, /* Clear IRQ */
	DIP_CMD_DEBUG_FLAG, /* Dump message level */
	DIP_CMD_P2_BUFQUE_CTRL,
	DIP_CMD_WAKELOCK_CTRL,
	DIP_CMD_REGISTER_IRQ_USER_KEY, /* register for a user key to do irq operation */
	DIP_CMD_FLUSH_IRQ_REQUEST, /* flush signal */
	DIP_CMD_ION_IMPORT, /* get ion handle */
	DIP_CMD_ION_FREE,  /* free ion handle */
	DIP_CMD_ION_FREE_BY_HWMODULE,  /* free all ion handle */
	DIP_CMD_DUMP_BUFFER,
	DIP_CMD_GET_DUMP_INFO,
	DIP_CMD_SET_MEM_INFO
};


/* Everest reset ioctl */
#define DIP_RESET_BY_HWMODULE    _IOW(DIP_MAGIC, DIP_CMD_RESET_BY_HWMODULE, unsigned long)

/* read phy reg  */
#define DIP_READ_REGISTER        _IOWR(DIP_MAGIC, DIP_CMD_READ_REG, struct DIP_REG_IO_STRUCT)

/* write phy reg */
#define DIP_WRITE_REGISTER       _IOWR(DIP_MAGIC, DIP_CMD_WRITE_REG, struct DIP_REG_IO_STRUCT)

#define DIP_WAIT_IRQ        _IOW(DIP_MAGIC, DIP_CMD_WAIT_IRQ,      struct DIP_WAIT_IRQ_STRUCT)
#define DIP_CLEAR_IRQ       _IOW(DIP_MAGIC, DIP_CMD_CLEAR_IRQ,     struct DIP_CLEAR_IRQ_STRUCT)
#define DIP_REGISTER_IRQ_USER_KEY   _IOR(DIP_MAGIC, DIP_CMD_REGISTER_IRQ_USER_KEY, struct DIP_REGISTER_USERKEY_STRUCT)
#define DIP_FLUSH_IRQ_REQUEST       _IOW(DIP_MAGIC, DIP_CMD_FLUSH_IRQ_REQUEST, struct DIP_WAIT_IRQ_STRUCT)
#define DIP_DEBUG_FLAG      _IOW(DIP_MAGIC, DIP_CMD_DEBUG_FLAG,    unsigned char*)
#define DIP_P2_BUFQUE_CTRL     _IOWR(DIP_MAGIC, DIP_CMD_P2_BUFQUE_CTRL, struct DIP_P2_BUFQUE_STRUCT)

#define DIP_WAKELOCK_CTRL           _IOWR(DIP_MAGIC, DIP_CMD_WAKELOCK_CTRL, unsigned long)

#define DIP_DUMP_BUFFER             _IOWR(DIP_MAGIC, DIP_CMD_DUMP_BUFFER, struct DIP_DUMP_BUFFER_STRUCT)
#define DIP_GET_DUMP_INFO           _IOWR(DIP_MAGIC, DIP_CMD_GET_DUMP_INFO, struct DIP_GET_DUMP_INFO_STRUCT)
#define DIP_SET_MEM_INFO            _IOWR(DIP_MAGIC, DIP_CMD_SET_MEM_INFO, struct DIP_MEM_INFO_STRUCT)

#define DIP_ION_IMPORT              _IOW(DIP_MAGIC, DIP_CMD_ION_IMPORT, struct DIP_DEV_ION_NODE_STRUCT)
#define DIP_ION_FREE                _IOW(DIP_MAGIC, DIP_CMD_ION_FREE,   struct DIP_DEV_ION_NODE_STRUCT)
#define DIP_ION_FREE_BY_HWMODULE    _IOW(DIP_MAGIC, DIP_CMD_ION_FREE_BY_HWMODULE, unsigned int)

#ifdef CONFIG_COMPAT
#define COMPAT_DIP_RESET_BY_HWMODULE _IOW(DIP_MAGIC, DIP_CMD_RESET_BY_HWMODULE, compat_uptr_t)
#define COMPAT_DIP_READ_REGISTER    _IOWR(DIP_MAGIC, DIP_CMD_READ_REG,      struct compat_DIP_REG_IO_STRUCT)
#define COMPAT_DIP_WRITE_REGISTER   _IOWR(DIP_MAGIC, DIP_CMD_WRITE_REG,     struct compat_DIP_REG_IO_STRUCT)
#define COMPAT_DIP_DEBUG_FLAG      _IOW(DIP_MAGIC, DIP_CMD_DEBUG_FLAG,     compat_uptr_t)

#define COMPAT_DIP_WAKELOCK_CTRL    _IOWR(DIP_MAGIC, DIP_CMD_WAKELOCK_CTRL, compat_uptr_t)
#define COMPAT_DIP_DUMP_BUFFER      _IOWR(DIP_MAGIC, DIP_CMD_DUMP_BUFFER, struct compat_DIP_DUMP_BUFFER_STRUCT)
#define COMPAT_DIP_SET_MEM_INFO     _IOWR(DIP_MAGIC, DIP_CMD_SET_MEM_INFO, struct compat_DIP_MEM_INFO_STRUCT)

#endif

int32_t DIP_MDPClockOnCallback(uint64_t engineFlag);
int32_t DIP_MDPDumpCallback(uint64_t engineFlag, int level);
int32_t DIP_MDPResetCallback(uint64_t engineFlag);

int32_t DIP_MDPClockOffCallback(uint64_t engineFlag);

int32_t DIP_BeginGCECallback(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress);
int32_t DIP_EndGCECallback(uint32_t taskID, uint32_t regCount, uint32_t *regValues);

#endif

