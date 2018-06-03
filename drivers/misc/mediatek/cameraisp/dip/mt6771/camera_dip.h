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

/* M4U not ready, mark this temporarily
 * m4u_callback_ret_t DIP_M4U_TranslationFault_callback(int port, unsigned int mva, void *data);
*/

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

#define CAM_A_BASE_HW   0x1A004000
#define CAM_B_BASE_HW   0x1A005000
#define CAMSV_0_BASE_HW 0x1A050000
#define CAMSV_1_BASE_HW 0x1A051000
#define CAMSV_2_BASE_HW 0x1A052000
#define CAMSV_3_BASE_HW 0x1A053000
#define CAMSV_4_BASE_HW 0x1A054000
#define CAMSV_5_BASE_HW 0x1A055000
#define DIP_A_BASE_HW   0x15022000
#define UNI_A_BASE_HW   0x1A003000
#define SENINF_BASE_HW  0x1A040000
#define MIPI_RX_BASE_HW 0x11C80000
#define GPIO_BASE_HW    0x11D20000

#define DIP_REG_RANGE           (PAGE_SIZE)
#define DIP_REG_PER_DIP_RANGE   (PAGE_SIZE*6)

/* In order with the suquence of device nodes defined in dtsi */
enum DIP_DEV_NODE_ENUM {
	/*DIP_IMGSYS_CONFIG_IDX = 0,*/
	DIP_DIP_A_IDX, /* Remider: Add this device node manually in .dtsi */
	DIP_DEV_NODE_NUM
};

/* defined if want to support multiple dequne and enque or camera 3.0 */
/**
 * support multiple deque and enque if defined.
 * note: still en/de que 1 buffer each time only
 * e.g:
 * deque();
 * deque();
 * enque();
 * enque();
 */
/* #define _rtbc_buf_que_2_0_ */
/**
 * frame status
 */
enum CAM_FrameST {
	CAM_FST_NORMAL = 0, CAM_FST_DROP_FRAME = 1, CAM_FST_LAST_WORKING_FRAME = 2,
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

enum DIP_ST_ENUM {
	SIGNAL_INT = 0, DMA_INT, DIP_IRQ_ST_AMOUNT
};

struct DIP_IRQ_TIME_STRUCT {
	unsigned int tLastSig_sec; /* time stamp of the latest occurring signal*/
	unsigned int tLastSig_usec; /* time stamp of the latest occurring signal*/
	unsigned int tMark2WaitSig_sec; /* time period from marking a signal to user try to wait and get the signal*/
	unsigned int tMark2WaitSig_usec; /* time period from marking a signal to user try to wait and get the signal*/
	unsigned int tLastSig2GetSig_sec; /* time period from latest signal to user try to wait and get the signal*/
	unsigned int tLastSig2GetSig_usec; /* time period from latest signal to user try to wait and get the signal*/
	int passedbySigcnt; /* the count for the signal passed by  */
};

struct DIP_WAIT_IRQ_ST {
	DIP_IRQ_CLEAR_ENUM Clear;
	DIP_ST_ENUM St_type;
	unsigned int Status; /*ref. enum:ENUM_CAM_INT / ENUM_CAM_DMA_INT ...etc in dip_drv_stddef.h*/
	int UserKey; /* user key for doing interrupt operation */
	unsigned int Timeout;
	DIP_IRQ_TIME_STRUCT TimeInfo;
};

struct DIP_WAIT_IRQ_STRUCT {
	DIP_IRQ_TYPE_ENUM Type;
	unsigned int bDumpReg;
	DIP_WAIT_IRQ_ST EventInfo;
};

struct DIP_REGISTER_USERKEY_STRUCT {
	int userKey;
	char userName[32]; /* this size must the same as the icamiopipe api - registerIrq(...) */
};

struct DIP_CLEAR_IRQ_ST {
	int UserKey; /* user key for doing interrupt operation */
	DIP_ST_ENUM St_type;
	unsigned int Status;
};

struct DIP_CLEAR_IRQ_STRUCT {
	DIP_IRQ_TYPE_ENUM Type;
	DIP_CLEAR_IRQ_ST EventInfo;
};

struct DIP_REG_STRUCT {
	unsigned int module; /*plz refer to DIP_DEV_NODE_ENUM */
	unsigned int Addr; /* register's addr */
	unsigned int Val; /* register's value */
};

struct DIP_REG_IO_STRUCT {
	DIP_REG_STRUCT *pData; /* pointer to DIP_REG_STRUCT */
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
/* length of the two memory areas */
#define P1_DEQUE_CNT    1
#define RT_BUF_TBL_NPAGES 16
#define DIP_RT_BUF_SIZE 16
#define DIP_RT_CQ0C_BUF_SIZE (DIP_RT_BUF_SIZE)/* (DIP_RT_BUF_SIZE>>1) */
/* pass1 setting sync index */
#define DIP_REG_P1_CFG_IDX 0x4090
/* how many clk levels */
#define DIP_CLK_LEVEL_CNT 10

enum _dip_tg_enum_ {
	_cam_tg_ = 0,
	_cam_tg2_,
	_camsv_tg_,
	_camsv2_tg_,
	_cam_tg_max_
};


enum _dip_dma_enum_ {
	_imgi_ = 0,
	_imgbi_,
	_imgci_,
	_vipi_,
	_vip2i_,
	_vip3i_, /* 5 */
	_ufdi_,
	_lcei_,
	_dmgi_,
	_depi_,
	_img2o_, /* 10 */
	_img2bo_,
	_img3o_,
	_img3bo_,
	_img3co_,
	_mfbo_, /* 15 */
	_feo_,
	_wrot_,
	_wdma_,
	_jpeg_,
	_venc_stream_, /* 20 */
	_dip_max,
	_rt_dma_max_ = _dip_max,

	_imgo_ = 0,
	_rrzo_,
	_ufeo_,
	_aao_,
	_afo_,
	_lcso_, /* 5 */
	_pdo_,
	_eiso_,
	_flko_,
	_rsso_,
	_pso_,  /* 10*/
	_bpci_,
	_lsci_,
	_rawi_,
	_cam_max_,

	_camsv_imgo_ = _imgo_,
	_camsv_max_,
};

/* for keep ion handle */
enum DIP_WRDMA_ENUM {
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
};

struct DIP_DEV_ION_NODE_STRUCT {
	unsigned int       devNode; /*plz refer to DIP_DEV_NODE_ENUM*/
	DIP_WRDMA_ENUM     dmaPort;
	int                memID;
};

struct DIP_LARB_MMU_STRUCT {
	unsigned int LarbNum;
	unsigned int regOffset;
	unsigned int regVal;
};

struct DIP_RT_IMAGE_INFO_STRUCT {
	unsigned int w; /* tg size */
	unsigned int h;
	unsigned int xsize; /* dmao xsize */
	unsigned int stride;
	unsigned int fmt;
	unsigned int pxl_id;
	unsigned int wbn;
	unsigned int ob;
	unsigned int lsc;
	unsigned int rpg;
	unsigned int m_num_0;
	unsigned int frm_cnt;
	unsigned int bus_size;
};

struct DIP_RT_RRZ_INFO_STRUCT {
	unsigned int srcX; /* crop window start point */
	unsigned int srcY;
	unsigned int srcW; /* crop window size */
	unsigned int srcH;
	unsigned int dstW; /* rrz out size */
	unsigned int dstH;
};

struct DIP_RT_DMAO_CROPPING_STRUCT {
	unsigned int x; /* in pix */
	unsigned int y; /* in pix */
	unsigned int w; /* in byte */
	unsigned int h; /* in byte */
};

struct DIP_RT_BUF_INFO_STRUCT {
	unsigned int memID;
	unsigned int size;
	long long base_vAddr;
	unsigned int base_pAddr;
	unsigned int timeStampS;
	unsigned int timeStampUs;
	unsigned int bFilled;
	unsigned int bProcessRaw;
	DIP_RT_IMAGE_INFO_STRUCT image;
	DIP_RT_RRZ_INFO_STRUCT rrzInfo;
	DIP_RT_DMAO_CROPPING_STRUCT dmaoCrop; /* imgo */
	unsigned int bDequeued;
	signed int bufIdx; /* used for replace buffer */
};

struct DIP_DEQUE_BUF_INFO_STRUCT {
	unsigned int count;
	unsigned int sof_cnt; /* cnt for current sof */
	unsigned int img_cnt; /* cnt for mapping to which sof */
	/* support only deque 1 image at a time */
	/* DIP_RT_BUF_INFO_STRUCT  data[DIP_RT_BUF_SIZE]; */
	DIP_RT_BUF_INFO_STRUCT data[P1_DEQUE_CNT];
};

struct DIP_RT_RING_BUF_INFO_STRUCT {
	unsigned int start; /* current DMA accessing buffer */
	unsigned int total_count; /* total buffer number.Include Filled and empty */
	unsigned int empty_count; /* total empty buffer number include current DMA accessing buffer */
	unsigned int pre_empty_count; /* previous total empty buffer number include current DMA accessing buffer */
	unsigned int active;
	unsigned int read_idx;
	unsigned int img_cnt; /* cnt for mapping to which sof */
	DIP_RT_BUF_INFO_STRUCT data[DIP_RT_BUF_SIZE];
};

enum DIP_RT_BUF_CTRL_ENUM {
	DIP_RT_BUF_CTRL_DMA_EN, DIP_RT_BUF_CTRL_CLEAR, DIP_RT_BUF_CTRL_MAX
};

enum DIP_RTBC_STATE_ENUM {
	DIP_RTBC_STATE_INIT,
	DIP_RTBC_STATE_SOF,
	DIP_RTBC_STATE_DONE,
	DIP_RTBC_STATE_MAX
};

enum DIP_RTBC_BUF_STATE_ENUM {
	DIP_RTBC_BUF_EMPTY,
	DIP_RTBC_BUF_FILLED,
	DIP_RTBC_BUF_LOCKED,
};

enum DIP_RAW_TYPE_ENUM {
	DIP_RROCESSED_RAW,
	DIP_PURE_RAW,
};

struct DIP_RT_BUF_STRUCT {
	DIP_RTBC_STATE_ENUM state;
	unsigned long dropCnt;
	DIP_RT_RING_BUF_INFO_STRUCT ring_buf[_cam_max_];
};

struct DIP_BUFFER_CTRL_STRUCT {
	DIP_RT_BUF_CTRL_ENUM ctrl;
	DIP_IRQ_TYPE_ENUM module;
	_dip_dma_enum_ buf_id;
	/* unsigned int            data_ptr; */
	/* unsigned int            ex_data_ptr; exchanged buffer */
	DIP_RT_BUF_INFO_STRUCT *data_ptr;
	DIP_RT_BUF_INFO_STRUCT *ex_data_ptr; /* exchanged buffer */
	unsigned char *pExtend;
};

/* reference count */
#define _use_kernel_ref_cnt_

enum DIP_REF_CNT_CTRL_ENUM {
	DIP_REF_CNT_GET,
	DIP_REF_CNT_INC,
	DIP_REF_CNT_DEC,
	DIP_REF_CNT_DEC_AND_RESET_P1_P2_IF_LAST_ONE,
	DIP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE,
	DIP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE,
	DIP_REF_CNT_MAX
};

enum DIP_REF_CNT_ID_ENUM {
	DIP_REF_CNT_ID_IMEM,
	DIP_REF_CNT_ID_DIP_FUNC,
	DIP_REF_CNT_ID_GLOBAL_PIPE,
	DIP_REF_CNT_ID_P1_PIPE,
	DIP_REF_CNT_ID_P2_PIPE,
	DIP_REF_CNT_ID_MAX,
};

struct DIP_REF_CNT_CTRL_STRUCT {
	DIP_REF_CNT_CTRL_ENUM ctrl;
	DIP_REF_CNT_ID_ENUM id;
	signed int *data_ptr;
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

enum DIP_P2_BUFQUE_STRUCT {
	DIP_P2_BUFQUE_PROPERTY_DIP = 0,
	DIP_P2_BUFQUE_PROPERTY_NUM = 1,
	DIP_P2_BUFQUE_PROPERTY_WARP
} DIP_P2_BUFQUE_PROPERTY;

struct DIP_P2_BUFQUE_STRUCT {
	DIP_P2_BUFQUE_CTRL_ENUM ctrl;
	DIP_P2_BUFQUE_PROPERTY property;
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

struct DIP_CLK_INFO {
	unsigned char clklevelcnt; /* how many clk levels */
	unsigned int clklevel[DIP_CLK_LEVEL_CNT]; /* Reocrd each clk level */
};

/********************************************************************************************
 * pass1 real time buffer control use cq0c
 ********************************************************************************************/

#define _rtbc_use_cq0c_

#define _MAGIC_NUM_ERR_HANDLING_

#ifdef CONFIG_COMPAT


struct compat_DIP_BUFFER_CTRL_STRUCT {
	DIP_RT_BUF_CTRL_ENUM ctrl;
	DIP_IRQ_TYPE_ENUM module;
	_dip_dma_enum_ buf_id;
	compat_uptr_t data_ptr;
	compat_uptr_t ex_data_ptr; /* exchanged buffer */
	compat_uptr_t pExtend;
} compat_DIP_BUFFER_CTRL_STRUCT;

struct compat_DIP_REF_CNT_CTRL_STRUCT {
	DIP_REF_CNT_CTRL_ENUM ctrl;
	DIP_REF_CNT_ID_ENUM id;
	compat_uptr_t data_ptr;
};

#endif


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
	DIP_CMD_DUMP_REG, /* Dump DIP registers , for debug usage */
	DIP_CMD_RT_BUF_CTRL, /* for pass buffer control */
	DIP_CMD_REF_CNT, /* get imem reference count */
	DIP_CMD_DEBUG_FLAG, /* Dump message level */
	DIP_CMD_P2_BUFQUE_CTRL,
	DIP_CMD_UPDATE_REGSCEN,
	DIP_CMD_QUERY_REGSCEN,
	DIP_CMD_UPDATE_BURSTQNUM,
	DIP_CMD_QUERY_BURSTQNUM,
	DIP_CMD_DUMP_ISR_LOG, /* dump isr log */
	DIP_CMD_GET_CUR_SOF,
	DIP_CMD_GET_DMA_ERR,
	DIP_CMD_GET_INT_ERR,
	DIP_CMD_GET_DROP_FRAME, /* dump current frame informaiton, 1 for drop frmae, 2 for last working frame */
	DIP_CMD_WAKELOCK_CTRL,
	DIP_CMD_REGISTER_IRQ_USER_KEY, /* register for a user key to do irq operation */
	DIP_CMD_MARK_IRQ_REQUEST, /* mark for a specific register before wait for the interrupt if needed */
	DIP_CMD_GET_MARK2QUERY_TIME, /* query time information between read and mark */
	DIP_CMD_FLUSH_IRQ_REQUEST, /* flush signal */
	DIP_CMD_GET_START_TIME,
	DIP_CMD_DFS_UPDATE, /* Update clock at run time */
	DIP_CMD_GET_SUPPORTED_DIP_CLOCKS, /* Get supported dip clocks on current platform */
	DIP_CMD_GET_CUR_DIP_CLOCK, /* Get cur dip clock level */
	DIP_CMD_VF_LOG, /* dbg only, prt log on kernel when vf_en is driven */
	DIP_CMD_GET_VSYNC_CNT,
	DIP_CMD_RESET_VSYNC_CNT,
	DIP_CMD_ION_IMPORT, /* get ion handle */
	DIP_CMD_ION_FREE,  /* free ion handle */
	DIP_CMD_CQ_SW_PATCH,  /* sim cq update behavior as atomic behavior */
	DIP_CMD_ION_FREE_BY_HWMODULE,  /* free all ion handle */
	DIP_CMD_LARB_MMU_CTL, /* toggle mmu config for smi larb ports of dip */
	DIP_CMD_DUMP_BUFFER,
	DIP_CMD_GET_DUMP_INFO,
	DIP_CMD_SET_MEM_INFO
};

enum DIP_HALT_DMA_ENUM {
	DIP_HALT_DMA_IMGO = 0,
	DIP_HALT_DMA_RRZO,
	DIP_HALT_DMA_AAO,
	DIP_HALT_DMA_AFO,
	DIP_HALT_DMA_LSCI,
	DIP_HALT_DMA_RSSO,
	DIP_HALT_DMA_CAMSV0,
	DIP_HALT_DMA_CAMSV1,
	DIP_HALT_DMA_CAMSV2,
	DIP_HALT_DMA_LSCO,
	DIP_HALT_DMA_UFEO,
	DIP_HALT_DMA_BPCI,
	DIP_HALT_DMA_PDO,
	DIP_HALT_DMA_RAWI,
	DIP_HALT_DMA_AMOUNT
};


/* Everest reset ioctl */
#define DIP_RESET_BY_HWMODULE    _IOW(DIP_MAGIC, DIP_CMD_RESET_BY_HWMODULE, unsigned long)

/* read phy reg  */
#define DIP_READ_REGISTER        _IOWR(DIP_MAGIC, DIP_CMD_READ_REG, DIP_REG_IO_STRUCT)

/* write phy reg */
#define DIP_WRITE_REGISTER       _IOWR(DIP_MAGIC, DIP_CMD_WRITE_REG, DIP_REG_IO_STRUCT)

#define DIP_WAIT_IRQ        _IOW(DIP_MAGIC, DIP_CMD_WAIT_IRQ,      DIP_WAIT_IRQ_STRUCT)
#define DIP_CLEAR_IRQ       _IOW(DIP_MAGIC, DIP_CMD_CLEAR_IRQ,     DIP_CLEAR_IRQ_STRUCT)
#define DIP_DUMP_REG        _IO(DIP_MAGIC, DIP_CMD_DUMP_REG)
#define DIP_BUFFER_CTRL     _IOWR(DIP_MAGIC, DIP_CMD_RT_BUF_CTRL,   DIP_BUFFER_CTRL_STRUCT)
#define DIP_REF_CNT_CTRL    _IOWR(DIP_MAGIC, DIP_CMD_REF_CNT,       DIP_REF_CNT_CTRL_STRUCT)
#define DIP_DEBUG_FLAG      _IOW(DIP_MAGIC, DIP_CMD_DEBUG_FLAG,    unsigned char*)
#define DIP_P2_BUFQUE_CTRL     _IOWR(DIP_MAGIC, DIP_CMD_P2_BUFQUE_CTRL, DIP_P2_BUFQUE_STRUCT)
#define DIP_UPDATE_REGSCEN     _IOWR(DIP_MAGIC, DIP_CMD_UPDATE_REGSCEN, unsigned int)
#define DIP_QUERY_REGSCEN     _IOR(DIP_MAGIC, DIP_CMD_QUERY_REGSCEN,   unsigned int)
#define DIP_UPDATE_BURSTQNUM _IOW(DIP_MAGIC, DIP_CMD_UPDATE_BURSTQNUM,   int)
#define DIP_QUERY_BURSTQNUM _IOR(DIP_MAGIC, DIP_CMD_QUERY_BURSTQNUM,    int)
#define DIP_DUMP_ISR_LOG    _IOWR(DIP_MAGIC, DIP_CMD_DUMP_ISR_LOG,         unsigned long)
#define DIP_GET_CUR_SOF     _IOWR(DIP_MAGIC, DIP_CMD_GET_CUR_SOF,          unsigned char*)
#define DIP_GET_DMA_ERR     _IOWR(DIP_MAGIC, DIP_CMD_GET_DMA_ERR,          unsigned char*)
#define DIP_GET_INT_ERR     _IOR(DIP_MAGIC, DIP_CMD_GET_INT_ERR,        unsigned char*)
#define DIP_GET_DROP_FRAME  _IOWR(DIP_MAGIC, DIP_CMD_GET_DROP_FRAME,    unsigned long)
#define DIP_GET_START_TIME  _IOWR(DIP_MAGIC, DIP_CMD_GET_START_TIME,    unsigned char*)
#define DIP_DFS_UPDATE              _IOWR(DIP_MAGIC, DIP_CMD_DFS_UPDATE, unsigned int)
#define DIP_GET_SUPPORTED_DIP_CLOCKS   _IOWR(DIP_MAGIC, DIP_CMD_GET_SUPPORTED_DIP_CLOCKS, DIP_CLK_INFO)
#define DIP_GET_CUR_DIP_CLOCK   _IOWR(DIP_MAGIC, DIP_CMD_GET_CUR_DIP_CLOCK, unsigned int)

#define DIP_REGISTER_IRQ_USER_KEY   _IOR(DIP_MAGIC, DIP_CMD_REGISTER_IRQ_USER_KEY, DIP_REGISTER_USERKEY_STRUCT)

#define DIP_MARK_IRQ_REQUEST        _IOWR(DIP_MAGIC, DIP_CMD_MARK_IRQ_REQUEST, DIP_WAIT_IRQ_STRUCT)
#define DIP_GET_MARK2QUERY_TIME     _IOWR(DIP_MAGIC, DIP_CMD_GET_MARK2QUERY_TIME, DIP_WAIT_IRQ_STRUCT)
#define DIP_FLUSH_IRQ_REQUEST       _IOW(DIP_MAGIC, DIP_CMD_FLUSH_IRQ_REQUEST, DIP_WAIT_IRQ_STRUCT)

#define DIP_WAKELOCK_CTRL           _IOWR(DIP_MAGIC, DIP_CMD_WAKELOCK_CTRL, unsigned long)
#define DIP_VF_LOG                  _IOW(DIP_MAGIC, DIP_CMD_VF_LOG,         unsigned char*)
#define DIP_GET_VSYNC_CNT           _IOWR(DIP_MAGIC, DIP_CMD_GET_VSYNC_CNT,      unsigned int)
#define DIP_RESET_VSYNC_CNT         _IOW(DIP_MAGIC, DIP_CMD_RESET_VSYNC_CNT,      unsigned int)
#define DIP_ION_IMPORT              _IOW(DIP_MAGIC, DIP_CMD_ION_IMPORT, DIP_DEV_ION_NODE_STRUCT)
#define DIP_ION_FREE                _IOW(DIP_MAGIC, DIP_CMD_ION_FREE,   DIP_DEV_ION_NODE_STRUCT)
#define DIP_ION_FREE_BY_HWMODULE    _IOW(DIP_MAGIC, DIP_CMD_ION_FREE_BY_HWMODULE, unsigned int)
#define DIP_CQ_SW_PATCH             _IOW(DIP_MAGIC, DIP_CMD_CQ_SW_PATCH, unsigned int)
#define DIP_LARB_MMU_CTL            _IOW(DIP_MAGIC, DIP_CMD_LARB_MMU_CTL, DIP_LARB_MMU_STRUCT)
#define DIP_DUMP_BUFFER             _IOWR(DIP_MAGIC, DIP_CMD_DUMP_BUFFER, DIP_DUMP_BUFFER_STRUCT)
#define DIP_GET_DUMP_INFO           _IOWR(DIP_MAGIC, DIP_CMD_GET_DUMP_INFO, DIP_GET_DUMP_INFO_STRUCT)
#define DIP_SET_MEM_INFO            _IOWR(DIP_MAGIC, DIP_CMD_SET_MEM_INFO, DIP_MEM_INFO_STRUCT)

#ifdef CONFIG_COMPAT
#define COMPAT_DIP_READ_REGISTER    _IOWR(DIP_MAGIC, DIP_CMD_READ_REG,      compat_DIP_REG_IO_STRUCT)
#define COMPAT_DIP_WRITE_REGISTER   _IOWR(DIP_MAGIC, DIP_CMD_WRITE_REG,     compat_DIP_REG_IO_STRUCT)
#define COMPAT_DIP_DEBUG_FLAG      _IOW(DIP_MAGIC, DIP_CMD_DEBUG_FLAG,     compat_uptr_t)
#define COMPAT_DIP_GET_DMA_ERR     _IOWR(DIP_MAGIC, DIP_CMD_GET_DMA_ERR,   compat_uptr_t)
#define COMPAT_DIP_GET_INT_ERR     _IOR(DIP_MAGIC, DIP_CMD_GET_INT_ERR,  compat_uptr_t)

#define COMPAT_DIP_BUFFER_CTRL     _IOWR(DIP_MAGIC, DIP_CMD_RT_BUF_CTRL,    compat_DIP_BUFFER_CTRL_STRUCT)
#define COMPAT_DIP_REF_CNT_CTRL    _IOWR(DIP_MAGIC, DIP_CMD_REF_CNT,        compat_DIP_REF_CNT_CTRL_STRUCT)
#define COMPAT_DIP_GET_START_TIME  _IOWR(DIP_MAGIC, DIP_CMD_GET_START_TIME, compat_uptr_t)

#define COMPAT_DIP_WAKELOCK_CTRL    _IOWR(DIP_MAGIC, DIP_CMD_WAKELOCK_CTRL, compat_uptr_t)
#define COMPAT_DIP_GET_DROP_FRAME   _IOWR(DIP_MAGIC, DIP_CMD_GET_DROP_FRAME, compat_uptr_t)
#define COMPAT_DIP_GET_CUR_SOF      _IOWR(DIP_MAGIC, DIP_CMD_GET_CUR_SOF,   compat_uptr_t)
#define COMPAT_DIP_DUMP_ISR_LOG     _IOWR(DIP_MAGIC, DIP_CMD_DUMP_ISR_LOG,  compat_uptr_t)
#define COMPAT_DIP_RESET_BY_HWMODULE _IOW(DIP_MAGIC, DIP_CMD_RESET_BY_HWMODULE, compat_uptr_t)
#define COMPAT_DIP_VF_LOG           _IOW(DIP_MAGIC, DIP_CMD_VF_LOG,         compat_uptr_t)
#define COMPAT_DIP_CQ_SW_PATCH      _IOW(DIP_MAGIC, DIP_CMD_CQ_SW_PATCH,         compat_uptr_t)
#define COMPAT_DIP_DUMP_BUFFER      _IOWR(DIP_MAGIC, DIP_CMD_DUMP_BUFFER, compat_DIP_DUMP_BUFFER_STRUCT)
#define COMPAT_DIP_SET_MEM_INFO     _IOWR(DIP_MAGIC, DIP_CMD_SET_MEM_INFO, compat_DIP_MEM_INFO_STRUCT)

#endif

int32_t DIP_MDPClockOnCallback(uint64_t engineFlag);
int32_t DIP_MDPDumpCallback(uint64_t engineFlag, int level);
int32_t DIP_MDPResetCallback(uint64_t engineFlag);

int32_t DIP_MDPClockOffCallback(uint64_t engineFlag);

int32_t DIP_BeginGCECallback(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress);
int32_t DIP_EndGCECallback(uint32_t taskID, uint32_t regCount, uint32_t *regValues);

#endif

