/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/include/gl_rst.h#1
*/

/*! \file   gl_rst.h
*    \brief  Declaration of functions and finite state machine for
*	    MT6620 Whole-Chip Reset Mechanism
*/

#ifndef _GL_RST_H
#define _GL_RST_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "gl_typedef.h"

#if 0
#include "mtk_porting.h"
#endif
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#if (MTK_WCN_HIF_SDIO)
#define CFG_WMT_RESET_API_SUPPORT   1
#else
#define CFG_WMT_RESET_API_SUPPORT   0
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef enum _ENUM_RESET_STATUS_T {
	RESET_FAIL,
	RESET_SUCCESS
} ENUM_RESET_STATUS_T;

typedef struct _RESET_STRUCT_T {
	ENUM_RESET_STATUS_T rst_data;
	struct work_struct rst_work;
	struct work_struct rst_trigger_work;
} RESET_STRUCT_T;

/* duplicated from wmt_exp.h for better driver isolation */
typedef enum _ENUM_WMTDRV_TYPE_T {
	WMTDRV_TYPE_BT = 0,
	WMTDRV_TYPE_FM = 1,
	WMTDRV_TYPE_GPS = 2,
	WMTDRV_TYPE_WIFI = 3,
	WMTDRV_TYPE_WMT = 4,
	WMTDRV_TYPE_STP = 5,
	WMTDRV_TYPE_SDIO1 = 6,
	WMTDRV_TYPE_SDIO2 = 7,
	WMTDRV_TYPE_LPBK = 8,
	WMTDRV_TYPE_MAX
} ENUM_WMTDRV_TYPE_T, *P_ENUM_WMTDRV_TYPE_T;

typedef enum _ENUM_WMTMSG_TYPE_T {
	WMTMSG_TYPE_POWER_ON = 0,
	WMTMSG_TYPE_POWER_OFF = 1,
	WMTMSG_TYPE_RESET = 2,
	WMTMSG_TYPE_STP_RDY = 3,
	WMTMSG_TYPE_HW_FUNC_ON = 4,
	WMTMSG_TYPE_MAX
} ENUM_WMTMSG_TYPE_T, *P_ENUM_WMTMSG_TYPE_T;

typedef enum _ENUM_WMTRSTMSG_TYPE_T {
	WMTRSTMSG_RESET_START = 0x0,
	WMTRSTMSG_RESET_END = 0x1,
	WMTRSTMSG_RESET_END_FAIL = 0x2,
	WMTRSTMSG_RESET_MAX,
	WMTRSTMSG_RESET_INVALID = 0xff
} ENUM_WMTRSTMSG_TYPE_T, *P_ENUM_WMTRSTMSG_TYPE_T;

typedef enum _ENUM_CHIP_RESET_REASON_TYPE_T {
	RST_PROCESS_ABNORMAL_INT = 1,
	RST_DRV_OWN_FAIL,
	RST_GROUP3_NULL,
	RST_GROUP4_NULL,
	RST_OID_TIMEOUT,
	RST_REASON_MAX
} ENUM_CHIP_RESET_REASON_TYPE_T, *P_ENUM_CHIP_RESET_REASON_TYPE_T;

typedef void (*PF_WMT_CB) (ENUM_WMTDRV_TYPE_T,	/* Source driver type */
			   ENUM_WMTDRV_TYPE_T,	/* Destination driver type */
			   ENUM_WMTMSG_TYPE_T,	/* Message type */
			   void *,
			   /* READ-ONLY buffer. Buffer is allocated and freed by WMT_drv. Client
			    * can't touch this buffer after this function return.
			    */
			   unsigned int);	/* Buffer size in unit of byte */

typedef int MTK_WCN_BOOL;
typedef unsigned int UINT32, *PUINT32;


/*******************************************************************************
*                    E X T E R N A L   F U N C T I O N S
********************************************************************************
*/

#if CFG_CHIP_RESET_SUPPORT

extern MTK_WCN_BOOL mtk_wcn_wmt_assert(ENUM_WMTDRV_TYPE_T type, UINT32 reason);
extern int mtk_wcn_wmt_msgcb_reg(ENUM_WMTDRV_TYPE_T eType, PF_WMT_CB pCb);
extern int mtk_wcn_wmt_msgcb_unreg(ENUM_WMTDRV_TYPE_T eType);
extern int wifi_reset_start(void);
extern int wifi_reset_end(ENUM_RESET_STATUS_T);
#endif

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#if CFG_CHIP_RESET_SUPPORT
#define glDoCoreDump() \
		do { \
			DBGLOG(INIT, INFO, "do core dump in %s\n", __func__); \
			mtk_wcn_wmt_assert(WMTDRV_TYPE_WIFI, 0x40); \
		} while (0)
#endif

extern UINT_64 u8ResetTime;
extern ENUM_CHIP_RESET_REASON_TYPE_T eResetReason;
/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if CFG_WMT_RESET_API_SUPPORT
extern MTK_WCN_BOOL mtk_wcn_wmt_do_reset(ENUM_WMTDRV_TYPE_T type);
#endif
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

VOID glResetInit(VOID);

VOID glResetUninit(VOID);

VOID glSendResetRequest(VOID);

BOOLEAN kalIsResetting(VOID);

BOOLEAN glResetTrigger(P_ADAPTER_T prAdapter);

VOID glGetRstReason(ENUM_CHIP_RESET_REASON_TYPE_T eReason);

#endif /* _GL_RST_H */
