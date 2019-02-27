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

#ifndef _HAL_TYPES_PRIVATE_H_
#define _HAL_TYPES_PRIVATE_H_

#define DumpReg__   /* /< Dump Reg for debug */
#ifdef DumpReg__
#include <stdio.h>
#endif

#include "val_types_private.h"
#include "hal_types_public.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADD_QUEUE(queue, index, q_type, q_address, q_offset, q_value, q_mask)\
	{                                                                    \
		queue[index].type     = q_type;                              \
		queue[index].address  = q_address;                           \
		queue[index].offset   = q_offset;                            \
		queue[index].value    = q_value;                             \
		queue[index].mask     = q_mask;                              \
		index = index + 1;                                           \
	}       /* /< ADD QUEUE command */


/**
 * @par Enumeration
 *   HAL_CODEC_TYPE_T
 * @par Description
 *   This is the item used for codec type
 */
enum HAL_CODEC_TYPE_T {
	HAL_CODEC_TYPE_VDEC,                /* /< VDEC */
	HAL_CODEC_TYPE_VENC,                /* /< VENC */
	HAL_CODEC_TYPE_MAX = 0xFFFFFFFF     /* /< MAX Value */
};

/**
 * @par Enumeration
 *   HAL_CMD_T
 * @par Description
 *   This is the item used for hal command type
 */
enum HAL_CMD_T {
	HAL_CMD_SET_CMD_QUEUE,              /* /< set command queue */
	HAL_CMD_SET_POWER,                  /* /< set power */
	HAL_CMD_SET_ISR,                    /* /< set ISR */
	HAL_CMD_GET_CACHE_CTRL_ADDR,        /* /< get cahce control address */
	HAL_CMD_MAX = 0xFFFFFFFF            /* /< MAX value */
};


/**
 * @par Enumeration
 *   REGISTER_GROUP_T
 * @par Description
 *   This is the item used for register group
 */
enum REGISTER_GROUP_T {
	VDEC_SYS,           /* /< VDEC_SYS */
	VDEC_MISC,          /* /< VDEC_MISC */
	VDEC_VLD,           /* /< VDEC_VLD */
	VDEC_VLD_TOP,       /* /< VDEC_VLD_TOP */
	VDEC_MC,            /* /< VDEC_MC */
	VDEC_AVC_VLD,       /* /< VDEC_AVC_VLD */
	VDEC_AVC_MV,        /* /< VDEC_AVC_MV */
	VDEC_HEVC_VLD,      /* /< VDEC_HEVC_VLD */
	VDEC_HEVC_MV,       /* /< VDEC_HEVC_MV */
	VDEC_PP,            /* /< VDEC_PP */
	/* VDEC_SQT, */
	VDEC_VP8_VLD,       /* /< VDEC_VP8_VLD */
	VDEC_VP6_VLD,       /* /< VDEC_VP6_VLD */
	VDEC_VP8_VLD2,      /* /< VDEC_VP8_VLD2 */
	VENC_HW_BASE,       /* /< VENC_HW_BASE */
	VENC_LT_HW_BASE,    /* /< VENC_HW_LT_BASE */
	VENC_MP4_HW_BASE,   /* /< VENC_MP4_HW_BASE */
	VDEC_VP9_VLD,       /* /< VDEC_VP9_VLD*/
	VDEC_UFO,           /* /< VDEC_UFO*/
	VCODEC_MAX          /* /< VCODEC_MAX */
};


/**
 * @par Enumeration
 *   REGISTER_GROUP_T
 * @par Description
 *   This is the item used for driver command type
 */
enum VCODEC_DRV_CMD_TYPE {
	ENABLE_HW_CMD,              /* /< ENABLE_HW_CMD */
	DISABLE_HW_CMD,             /* /< DISABLE_HW_CMD */
	WRITE_REG_CMD,              /* /< WRITE_REG_CMD */
	READ_REG_CMD,               /* /< READ_REG_CMD */
	WRITE_SYSRAM_CMD,           /* /< WRITE_SYSRAM_CMD */
	READ_SYSRAM_CMD,            /* /< READ_SYSRAM_CMD */
	MASTER_WRITE_CMD,           /* /< MASTER_WRITE_CMD */
	WRITE_SYSRAM_RANGE_CMD,     /* /< WRITE_SYSRAM_RANGE_CMD */
	READ_SYSRAM_RANGE_CMD,      /* /< READ_SYSRAM_RANGE_CMD */
	SETUP_ISR_CMD,              /* /< SETUP_ISR_CMD */
	WAIT_ISR_CMD,               /* /< WAIT_ISR_CMD */
	TIMEOUT_CMD,                /* /< TIMEOUT_CMD */
	MB_CMD,                     /* /< MB_CMD */
	POLL_REG_STATUS_CMD,        /* /< POLL_REG_STATUS_CMD */
	END_CMD                     /* /< END_CMD */
};


/**
 * @par Structure
 *  VCODEC_DRV_CMD_T
 * @par Description
 *  driver command information
 */
struct VCODEC_DRV_CMD_T {
	unsigned int   type;          /* /< type */
	unsigned long  address;       /* /< address */
	unsigned long  offset;        /* /< offset */
	unsigned long  value;         /* /< value */
	unsigned long  mask;          /* /< mask */
};


/**
 * @par Structure
 *  HAL_HANDLE_T
 * @par Description
 *  hal handle information
 */
struct HAL_HANDLE_T_ {
	int		fd_vdec;            /* /< fd_vdec */
	int		fd_venc;            /* /< fd_venc */
	VAL_MEMORY_T	rHandleMem;         /* /< rHandleMem */
	unsigned long	mmap[VCODEC_MAX];   /* /< mmap[VCODEC_MAX] */
	VAL_DRIVER_TYPE_T	driverType; /* /< driverType */
	unsigned int	u4TimeOut;          /* /< u4TimeOut */
	unsigned int	u4FrameCount;       /* /< u4FrameCount */
#ifdef DumpReg__
	FILE *pf_out;
#endif
	char		bProfHWTime;        /* /< bProfHWTime */
	unsigned long long	u8HWTime[2];/* /< u8HWTime */
};


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _HAL_TYPES_PRIVATE_H_ */
