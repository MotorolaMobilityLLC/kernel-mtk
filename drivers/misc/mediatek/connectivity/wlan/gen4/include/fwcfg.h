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
/*! \file   "fwcfg.h"
*    \brief  The declaration of the functions of the fw cfg related
*
*    Detail description.
*/

#ifndef _FWCFG_H
#define _FWCFG_H
#include "precomp.h"
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#ifdef FW_CFG_SUPPORT
#define MAX_CMD_ITEM_MAX		4
#define MAX_CMD_NAME_MAX_LENGTH		32
#define MAX_CMD_VALUE_MAX_LENGTH	32

#define MAX_CMD_TYPE_LENGTH		1
#define MAX_CMD_RESERVE_LENGTH		1
#define MAX_CMD_STRING_LENGTH		1
#define MAX_CMD_VALUE_LENGTH		1

#define CMD_FORMAT_V1_LENGTH		(MAX_CMD_NAME_MAX_LENGTH + \
					MAX_CMD_VALUE_MAX_LENGTH + MAX_CMD_TYPE_LENGTH + \
					MAX_CMD_STRING_LENGTH + MAX_CMD_VALUE_LENGTH + MAX_CMD_RESERVE_LENGTH)

#define MAX_CMD_BUFFER_LENGTH		(CMD_FORMAT_V1_LENGTH * MAX_CMD_ITEM_MAX)

#define FW_CFG_FILE "/system/vendor/firmware/wifi_fw.cfg"
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum _CMD_VER_T {
	CMD_VER_1,
	CMD_VER_1_EXT
};

enum _CMD_TYPE_T {
	CMD_TYPE_QUERY,
	CMD_TYPE_SET
};

struct _CMD_FORMAT_V1_T {
	UINT_8 itemType;
	UINT_8 itemStringLength;
	UINT_8 itemValueLength;
	UINT_8 Reserved;
	UINT_8 itemString[MAX_CMD_NAME_MAX_LENGTH];
	UINT_8 itemValue[MAX_CMD_VALUE_MAX_LENGTH];
};

struct _CMD_HEADER_T {
	enum _CMD_VER_T cmdVersion;
	enum _CMD_TYPE_T cmdType;
	UINT_8 itemNum;
	UINT_16 cmdBufferLen;
	UINT_8 buffer[MAX_CMD_BUFFER_LENGTH];
};

struct WLAN_CFG_PARSE_STATE_S {
	CHAR *ptr;
	CHAR *text;
	INT_32 nexttoken;
	UINT_32 maxSize;
};

struct _FW_CFG {
	PUINT_8 key;
	PUINT_8 value;
};
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

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

INT_32 getFwCfgItemNum(void);

PUINT_8 getFwCfgItemKey(UINT_8 i);

PUINT_8 getFwCfgItemValue(UINT_8 i);

void wlanCfgFwSetParam(PUINT_8 fwBuffer, PCHAR cmdStr, PCHAR value, int num, int type);

WLAN_STATUS wlanCfgSetGetFw(IN P_ADAPTER_T prAdapter, const PCHAR fwBuffer, int cmdNum, enum _CMD_TYPE_T cmdType);

WLAN_STATUS wlanFwCfgParse(IN P_ADAPTER_T prAdapter, PUINT_8 pucConfigBuf);

WLAN_STATUS wlanFwArrayCfg(IN P_ADAPTER_T prAdpter);

WLAN_STATUS wlanFwFileCfg(IN P_ADAPTER_T prAdpter);
#endif
#endif
