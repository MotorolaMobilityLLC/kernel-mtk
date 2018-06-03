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
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/gl_wext_priv.c#8
*/

/*! \file gl_wext_priv.c
*    \brief This file includes private ioctl support.
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"
#include "gl_os.h"
#include "gl_wext_priv.h"

#if CFG_SUPPORT_QA_TOOL
#include "gl_ate_agent.h"
#include "gl_qa_agent.h"
#endif

#if CFG_SUPPORT_WAPI
#include "gl_sec.h"
#endif
#if CFG_ENABLE_WIFI_DIRECT
#include "gl_p2p_os.h"
#endif

/*
* #if CFG_SUPPORT_QA_TOOL
* extern UINT_16 g_u2DumpIndex;
* #endif
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define	NUM_SUPPORTED_OIDS      (sizeof(arWlanOidReqTable) / sizeof(WLAN_REQ_ENTRY))
#define	CMD_OID_BUF_LENGTH	4096


/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int
priv_get_ndis(IN struct net_device *prNetDev, IN NDIS_TRANSPORT_STRUCT * prNdisReq, OUT PUINT_32 pu4OutputLen);

static int
priv_set_ndis(IN struct net_device *prNetDev, IN NDIS_TRANSPORT_STRUCT * prNdisReq, OUT PUINT_32 pu4OutputLen);

#if 0				/* CFG_SUPPORT_WPS */
static int
priv_set_appie(IN struct net_device *prNetDev,
	       IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, OUT char *pcExtra);

static int
priv_set_filter(IN struct net_device *prNetDev,
		IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, OUT char *pcExtra);
#endif /* CFG_SUPPORT_WPS */

static BOOLEAN reqSearchSupportedOidEntry(IN UINT_32 rOid, OUT P_WLAN_REQ_ENTRY * ppWlanReqEntry);

#if 0
static WLAN_STATUS
reqExtQueryConfiguration(IN P_GLUE_INFO_T prGlueInfo,
			 OUT PVOID pvQueryBuffer, IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen);

static WLAN_STATUS
reqExtSetConfiguration(IN P_GLUE_INFO_T prGlueInfo,
		       IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);
#endif

static WLAN_STATUS
reqExtSetAcpiDevicePowerState(IN P_GLUE_INFO_T prGlueInfo,
			      IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen);

/*******************************************************************************
*                       P R I V A T E   D A T A
********************************************************************************
*/
static UINT_8 aucOidBuf[CMD_OID_BUF_LENGTH] = { 0 };

/* OID processing table */
/* Order is important here because the OIDs should be in order of
 *  increasing value for binary searching.
 */
static WLAN_REQ_ENTRY arWlanOidReqTable[] = {
#if 0
	   {(NDIS_OID)rOid,
	   (PUINT_8)pucOidName,
	   fgQryBufLenChecking, fgSetBufLenChecking, fgIsHandleInGlueLayerOnly, u4InfoBufLen,
	   pfOidQueryHandler,
	   pfOidSetHandler}
#endif
	/* General Operational Characteristics */

	/* Ethernet Operational Characteristics */
	{OID_802_3_CURRENT_ADDRESS,
	 DISP_STRING("OID_802_3_CURRENT_ADDRESS"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, 6,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryCurrentAddr,
	 NULL},

	/* OID_802_3_MULTICAST_LIST */
	/* OID_802_3_MAXIMUM_LIST_SIZE */
	/* Ethernet Statistics */

	/* NDIS 802.11 Wireless LAN OIDs */
	{OID_802_11_SUPPORTED_RATES,
	 DISP_STRING("OID_802_11_SUPPORTED_RATES"),
	 TRUE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_RATES_EX),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQuerySupportedRates,
	 NULL}
	,
	/*
	 *  {OID_802_11_CONFIGURATION,
	 *  DISP_STRING("OID_802_11_CONFIGURATION"),
	 *  TRUE, TRUE, ENUM_OID_GLUE_EXTENSION, sizeof(PARAM_802_11_CONFIG_T),
	 *  (PFN_OID_HANDLER_FUNC_REQ)reqExtQueryConfiguration,
	 *  (PFN_OID_HANDLER_FUNC_REQ)reqExtSetConfiguration},
	 */
	{OID_PNP_SET_POWER,
	 DISP_STRING("OID_PNP_SET_POWER"),
	 TRUE, FALSE, ENUM_OID_GLUE_EXTENSION, sizeof(PARAM_DEVICE_POWER_STATE),
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) reqExtSetAcpiDevicePowerState}
	,

	/* Custom OIDs */
	{OID_CUSTOM_OID_INTERFACE_VERSION,
	 DISP_STRING("OID_CUSTOM_OID_INTERFACE_VERSION"),
	 TRUE, FALSE, ENUM_OID_DRIVER_CORE, 4,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryOidInterfaceVersion,
	 NULL}
	,
#if 0
	   #if PTA_ENABLED
	   {OID_CUSTOM_BT_COEXIST_CTRL,
	   DISP_STRING("OID_CUSTOM_BT_COEXIST_CTRL"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_BT_COEXIST_T),
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetBtCoexistCtrl},
	   #endif

	   {OID_CUSTOM_POWER_MANAGEMENT_PROFILE,
	   DISP_STRING("OID_CUSTOM_POWER_MANAGEMENT_PROFILE"),
	   FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryPwrMgmtProfParam,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetPwrMgmtProfParam},
	   {OID_CUSTOM_PATTERN_CONFIG,
	   DISP_STRING("OID_CUSTOM_PATTERN_CONFIG"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_PATTERN_SEARCH_CONFIG_STRUCT_T),
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetPatternConfig},
	   {OID_CUSTOM_BG_SSID_SEARCH_CONFIG,
	   DISP_STRING("OID_CUSTOM_BG_SSID_SEARCH_CONFIG"),
	   FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetBgSsidParam},
	   {OID_CUSTOM_VOIP_SETUP,
	   DISP_STRING("OID_CUSTOM_VOIP_SETUP"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryVoipConnectionStatus,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetVoipConnectionStatus},
	   {OID_CUSTOM_ADD_TS,
	   DISP_STRING("OID_CUSTOM_ADD_TS"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidAddTS},
	   {OID_CUSTOM_DEL_TS,
	   DISP_STRING("OID_CUSTOM_DEL_TS"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidDelTS},

	   #if CFG_LP_PATTERN_SEARCH_SLT
	   {OID_CUSTOM_SLT,
	   DISP_STRING("OID_CUSTOM_SLT"),
	   FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQuerySltResult,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetSltMode},
	   #endif

	   {OID_CUSTOM_ROAMING_EN,
	   DISP_STRING("OID_CUSTOM_ROAMING_EN"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryRoamingFunction,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetRoamingFunction},
	   {OID_CUSTOM_WMM_PS_TEST,
	   DISP_STRING("OID_CUSTOM_WMM_PS_TEST"),
	   TRUE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetWiFiWmmPsTest},
	   {OID_CUSTOM_COUNTRY_STRING,
	   DISP_STRING("OID_CUSTOM_COUNTRY_STRING"),
	   FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryCurrentCountry,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetCurrentCountry},

	   #if CFG_SUPPORT_802_11D
	   {OID_CUSTOM_MULTI_DOMAIN_CAPABILITY,
	   DISP_STRING("OID_CUSTOM_MULTI_DOMAIN_CAPABILITY"),
	   FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryMultiDomainCap,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetMultiDomainCap},
	   #endif

	   {OID_CUSTOM_GPIO2_MODE,
	   DISP_STRING("OID_CUSTOM_GPIO2_MODE"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(ENUM_PARAM_GPIO2_MODE_T),
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetGPIO2Mode},
	   {OID_CUSTOM_CONTINUOUS_POLL,
	   DISP_STRING("OID_CUSTOM_CONTINUOUS_POLL"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CONTINUOUS_POLL_T),
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryContinuousPollInterval,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetContinuousPollProfile},
	   {OID_CUSTOM_DISABLE_BEACON_DETECTION,
	   DISP_STRING("OID_CUSTOM_DISABLE_BEACON_DETECTION"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryDisableBeaconDetectionFunc,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetDisableBeaconDetectionFunc},

	/* WPS */
	   {OID_CUSTOM_DISABLE_PRIVACY_CHECK,
	   DISP_STRING("OID_CUSTOM_DISABLE_PRIVACY_CHECK"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	   NULL,
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidSetDisablePriavcyCheck},
#endif

	{OID_CUSTOM_MCR_RW,
	 DISP_STRING("OID_CUSTOM_MCR_RW"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_MCR_RW_STRUCT_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryMcrRead,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetMcrWrite}
	,

	{OID_CUSTOM_EEPROM_RW,
	 DISP_STRING("OID_CUSTOM_EEPROM_RW"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_EEPROM_RW_STRUCT_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryEepromRead,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetEepromWrite}
	,

	{OID_CUSTOM_SW_CTRL,
	 DISP_STRING("OID_CUSTOM_SW_CTRL"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQuerySwCtrlRead,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetSwCtrlWrite}
	,

	{OID_CUSTOM_MEM_DUMP,
	 DISP_STRING("OID_CUSTOM_MEM_DUMP"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryMemDump,
	 NULL}
	,

	{OID_CUSTOM_TEST_MODE,
	 DISP_STRING("OID_CUSTOM_TEST_MODE"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetTestMode}
	,

#if 0
	   {OID_CUSTOM_TEST_RX_STATUS,
	   DISP_STRING("OID_CUSTOM_TEST_RX_STATUS"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T),
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryRfTestRxStatus,
	   NULL},
	   {OID_CUSTOM_TEST_TX_STATUS,
	   DISP_STRING("OID_CUSTOM_TEST_TX_STATUS"),
	   FALSE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T),
	   (PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryRfTestTxStatus,
	   NULL},
#endif
	{OID_CUSTOM_ABORT_TEST_MODE,
	 DISP_STRING("OID_CUSTOM_ABORT_TEST_MODE"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetAbortTestMode}
	,
	{OID_CUSTOM_MTK_WIFI_TEST,
	 DISP_STRING("OID_CUSTOM_MTK_WIFI_TEST"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestQueryAutoTest,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetAutoTest}
	,
	{OID_CUSTOM_TEST_ICAP_MODE,
	 DISP_STRING("OID_CUSTOM_TEST_ICAP_MODE"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidRftestSetTestIcapMode}
	,

	/* OID_CUSTOM_EMULATION_VERSION_CONTROL */

	/* BWCS */
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS
	{OID_CUSTOM_BWCS_CMD,
	 DISP_STRING("OID_CUSTOM_BWCS_CMD"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(PTA_IPC_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryBT,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetBT}
	,
#endif
#if 0
	{OID_CUSTOM_SINGLE_ANTENNA,
	DISP_STRING("OID_CUSTOM_SINGLE_ANTENNA"),
	FALSE, FALSE, ENUM_OID_DRIVER_CORE, 4,
	(PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryBtSingleAntenna,
	(PFN_OID_HANDLER_FUNC_REQ)wlanoidSetBtSingleAntenna},
	{OID_CUSTOM_SET_PTA,
	DISP_STRING("OID_CUSTOM_SET_PTA"),
	FALSE, FALSE, ENUM_OID_DRIVER_CORE, 4,
	(PFN_OID_HANDLER_FUNC_REQ)wlanoidQueryPta,
	(PFN_OID_HANDLER_FUNC_REQ)wlanoidSetPta},
#endif

	{OID_CUSTOM_MTK_NVRAM_RW,
	 DISP_STRING("OID_CUSTOM_MTK_NVRAM_RW"),
	 TRUE, TRUE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_CUSTOM_NVRAM_RW_STRUCT_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryNvramRead,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetNvramWrite}
	,

	{OID_CUSTOM_CFG_SRC_TYPE,
	 DISP_STRING("OID_CUSTOM_CFG_SRC_TYPE"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(ENUM_CFG_SRC_TYPE_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryCfgSrcType,
	 NULL}
	,

	{OID_CUSTOM_EEPROM_TYPE,
	 DISP_STRING("OID_CUSTOM_EEPROM_TYPE"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(ENUM_EEPROM_TYPE_T),
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidQueryEepromType,
	 NULL}
	,

#if CFG_SUPPORT_WAPI
	{OID_802_11_WAPI_MODE,
	 DISP_STRING("OID_802_11_WAPI_MODE"),
	 FALSE, TRUE, ENUM_OID_DRIVER_CORE, 4,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiMode}
	,
	{OID_802_11_WAPI_ASSOC_INFO,
	 DISP_STRING("OID_802_11_WAPI_ASSOC_INFO"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiAssocInfo}
	,
	{OID_802_11_SET_WAPI_KEY,
	 DISP_STRING("OID_802_11_SET_WAPI_KEY"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, sizeof(PARAM_WPI_KEY_T),
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWapiKey}
	,
#endif

#if CFG_SUPPORT_WPS2
	{OID_802_11_WSC_ASSOC_INFO,
	 DISP_STRING("OID_802_11_WSC_ASSOC_INFO"),
	 FALSE, FALSE, ENUM_OID_DRIVER_CORE, 0,
	 NULL,
	 (PFN_OID_HANDLER_FUNC_REQ) wlanoidSetWSCAssocInfo}
	,
#endif
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Dispatching function for private ioctl region (SIOCIWFIRSTPRIV ~
*   SIOCIWLASTPRIV).
*
* \param[in] prNetDev Net device requested.
* \param[in] prIfReq Pointer to ifreq structure.
* \param[in] i4Cmd Command ID between SIOCIWFIRSTPRIV and SIOCIWLASTPRIV.
*
* \retval 0 for success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EFAULT For fail.
*
*/
/*----------------------------------------------------------------------------*/
int priv_support_ioctl(IN struct net_device *prNetDev, IN OUT struct ifreq *prIfReq, IN int i4Cmd)
{
	/* prIfReq is verified in the caller function wlanDoIOCTL() */
	struct iwreq *prIwReq = (struct iwreq *)prIfReq;
	struct iw_request_info rIwReqInfo;

	/* prNetDev is verified in the caller function wlanDoIOCTL() */

	/* Prepare the call */
	rIwReqInfo.cmd = (__u16) i4Cmd;
	rIwReqInfo.flags = 0;

	switch (i4Cmd) {
	case IOCTL_SET_INT:
		/* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need copy_from/to_user() */
		return priv_set_int(prNetDev, &rIwReqInfo, &(prIwReq->u), (char *)&(prIwReq->u));

	case IOCTL_GET_INT:
		/* NOTE(Kevin): 1/3 INT Type <= IFNAMSIZ, so we don't need copy_from/to_user() */
		return priv_get_int(prNetDev, &rIwReqInfo, &(prIwReq->u), (char *)&(prIwReq->u));

	case IOCTL_SET_STRUCT:
	case IOCTL_SET_STRUCT_FOR_EM:
		return priv_set_struct(prNetDev, &rIwReqInfo, &prIwReq->u, (char *)&(prIwReq->u));

	case IOCTL_GET_STRUCT:
		return priv_get_struct(prNetDev, &rIwReqInfo, &prIwReq->u, (char *)&(prIwReq->u));

#if (CFG_SUPPORT_QA_TOOL)
	case IOCTL_QA_TOOL_DAEMON:
		return priv_qa_agent(prNetDev, &rIwReqInfo, &(prIwReq->u), (char *)&(prIwReq->u));
#endif

	case IOCTL_GET_STR:

	default:
		return -EOPNOTSUPP;

	}			/* end of switch */

}				/* priv_support_ioctl */

#if CFG_SUPPORT_BATCH_SCAN

EVENT_BATCH_RESULT_T g_rEventBatchResult[CFG_BATCH_MAX_MSCAN];

UINT_32 batchChannelNum2Freq(UINT_32 u4ChannelNum)
{
	UINT_32 u4ChannelInMHz;

	if (u4ChannelNum >= 1 && u4ChannelNum <= 13)
		u4ChannelInMHz = 2412 + (u4ChannelNum - 1) * 5;
	else if (u4ChannelNum == 14)
		u4ChannelInMHz = 2484;
	else if (u4ChannelNum == 133)
		u4ChannelInMHz = 3665;	/* 802.11y */
	else if (u4ChannelNum == 137)
		u4ChannelInMHz = 3685;	/* 802.11y */
	else if (u4ChannelNum >= 34 && u4ChannelNum <= 165)
		u4ChannelInMHz = 5000 + u4ChannelNum * 5;
	else if (u4ChannelNum >= 183 && u4ChannelNum <= 196)
		u4ChannelInMHz = 4000 + u4ChannelNum * 5;
	else
		u4ChannelInMHz = 0;

	return u4ChannelInMHz;
}

#define TMP_TEXT_LEN_S 40
#define TMP_TEXT_LEN_L 60
static UCHAR text1[TMP_TEXT_LEN_S], text2[TMP_TEXT_LEN_L], text3[TMP_TEXT_LEN_L];	/* A safe len */

WLAN_STATUS
batchConvertResult(IN P_EVENT_BATCH_RESULT_T prEventBatchResult,
		   OUT PVOID pvBuffer, IN UINT_32 u4MaxBufferLen, OUT PUINT_32 pu4RetLen)
{
	CHAR *p = pvBuffer;
	CHAR ssid[ELEM_MAX_LEN_SSID + 1];
	INT_32 nsize, nsize1, nsize2, nsize3, scancount;
	INT_32 i, j, nleft;
	UINT_32 freq;

	P_EVENT_BATCH_RESULT_ENTRY_T prEntry;
	P_EVENT_BATCH_RESULT_T pBr;

	nsize = 0;
	nleft = u4MaxBufferLen - 5;	/* -5 for "----\n" */

	pBr = prEventBatchResult;
	scancount = 0;
	for (j = 0; j < CFG_BATCH_MAX_MSCAN; j++) {
		scancount += pBr->ucScanCount;
		pBr++;
	}

	nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "scancount=%ld\nnextcount=%ld\n", scancount, scancount);
	if (nsize1 < nleft) {
		p += nsize1 = kalSprintf(p, "%s", text1);
		nleft -= nsize1;
	} else
		goto short_buf;

	pBr = prEventBatchResult;
	for (j = 0; j < CFG_BATCH_MAX_MSCAN; j++) {
		DBGLOG(SCN, TRACE, "convert mscan = %d, apcount=%d, nleft=%d\n", j, pBr->ucScanCount, nleft);

		if (pBr->ucScanCount == 0) {
			pBr++;
			continue;
		}

		nleft -= 5;	/* -5 for "####\n" */

		/* We only support one round scan result now. */
		nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "apcount=%d\n", pBr->ucScanCount);
		if (nsize1 < nleft) {
			p += nsize1 = kalSprintf(p, "%s", text1);
			nleft -= nsize1;
		} else
			goto short_buf;

		for (i = 0; i < pBr->ucScanCount; i++) {
			prEntry = &pBr->arBatchResult[i];

			nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "bssid=" MACSTR "\n",
						MAC2STR(prEntry->aucBssid));
			kalMemCopy(ssid,
				   prEntry->aucSSID,
				   (prEntry->ucSSIDLen < ELEM_MAX_LEN_SSID ? prEntry->ucSSIDLen : ELEM_MAX_LEN_SSID));
			ssid[(prEntry->ucSSIDLen <
			      (ELEM_MAX_LEN_SSID - 1) ? prEntry->ucSSIDLen : (ELEM_MAX_LEN_SSID - 1))] = '\0';
			nsize2 = kalSnprintf(text2, TMP_TEXT_LEN_L, "ssid=%s\n", ssid);

			freq = batchChannelNum2Freq(prEntry->ucFreq);
			nsize3 =
			    kalSnprintf(text3, TMP_TEXT_LEN_L,
					"freq=%lu\nlevel=%d\ndist=%lu\ndistSd=%lu\n====\n", freq,
					prEntry->cRssi, prEntry->u4Dist, prEntry->u4Distsd);

			nsize = nsize1 + nsize2 + nsize3;
			if (nsize < nleft) {

				kalStrnCpy(p, text1, TMP_TEXT_LEN_S);
				p += nsize1;

				kalStrnCpy(p, text2, TMP_TEXT_LEN_L);
				p += nsize2;

				kalStrnCpy(p, text3, TMP_TEXT_LEN_L);
				p += nsize3;

				nleft -= nsize;
			} else {
				DBGLOG(SCN, TRACE, "Warning: Early break! (%d)\n", i);
				break;	/* discard following entries, TODO: apcount? */
			}
		}

		nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "%s", "####\n");
		p += kalSprintf(p, "%s", text1);

		pBr++;
	}

	nsize1 = kalSnprintf(text1, TMP_TEXT_LEN_S, "%s", "----\n");
	kalSprintf(p, "%s", text1);

	*pu4RetLen = u4MaxBufferLen - nleft;
	DBGLOG(SCN, TRACE, "total len = %d (max len = %d)\n", *pu4RetLen, u4MaxBufferLen);

	return WLAN_STATUS_SUCCESS;

short_buf:
	DBGLOG(SCN, TRACE,
	       "Short buffer issue! %d > %d, %s\n", u4MaxBufferLen + (nsize - nleft), u4MaxBufferLen, pvBuffer);
	return WLAN_STATUS_INVALID_LENGTH;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl set int handler.
*
* \param[in] prNetDev Net device requested.
* \param[in] prIwReqInfo Pointer to iwreq structure.
* \param[in] prIwReqData The ioctl data structure, use the field of sub-command.
* \param[in] pcExtra The buffer with input value
*
* \retval 0 For success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EINVAL If a value is out of range.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_set_int(IN struct net_device *prNetDev,
	     IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
	UINT_32 u4SubCmd;
	PUINT_32 pu4IntBuf;
	P_NDIS_TRANSPORT_STRUCT prNdisReq;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4BufLen = 0;
	int status = 0;
	P_PTA_IPC_T prPtaIpc;

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);

	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->mode;
	pu4IntBuf = (PUINT_32) pcExtra;

	switch (u4SubCmd) {
	case PRIV_CMD_TEST_MODE:
		/* printk("TestMode=%ld\n", pu4IntBuf[1]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_MODE;
		} else if (pu4IntBuf[1] == 0) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_ABORT_TEST_MODE;
		} else if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY_ICAP) {
			prNdisReq->ndisOidCmd = OID_CUSTOM_TEST_ICAP_MODE;
		} else {
			status = 0;
			break;
		}
		prNdisReq->inNdisOidlength = 0;
		prNdisReq->outNdisOidLength = 0;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

	case PRIV_CMD_TEST_CMD:
		/* printk("CMD=0x%08lx, data=0x%08lx\n", pu4IntBuf[1], pu4IntBuf[2]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

#if CFG_SUPPORT_PRIV_MCR_RW
	case PRIV_CMD_ACCESS_MCR:
		/* printk("addr=0x%08lx, data=0x%08lx\n", pu4IntBuf[1], pu4IntBuf[2]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		if (pu4IntBuf[1] == PRIV_CMD_TEST_MAGIC_KEY) {
			if (pu4IntBuf[2] == PRIV_CMD_TEST_MAGIC_KEY)
				prGlueInfo->fgMcrAccessAllowed = TRUE;
			status = 0;
			break;
		}
		if (prGlueInfo->fgMcrAccessAllowed) {
			kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

			prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
			prNdisReq->inNdisOidlength = 8;
			prNdisReq->outNdisOidLength = 8;

			/* Execute this OID */
			status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		}
		break;
#endif

	case PRIV_CMD_SW_CTRL:
		/* printk("addr=0x%08lx, data=0x%08lx\n", pu4IntBuf[1], pu4IntBuf[2]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

#if 0
	case PRIV_CMD_BEACON_PERIOD:
		/* pu4IntBuf[0] is used as input SubCmd */
		rStatus = wlanSetInformation(prGlueInfo->prAdapter, wlanoidSetBeaconInterval, (PVOID)&pu4IntBuf[1],
					     sizeof(UINT_32), &u4BufLen);
		break;
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	case PRIV_CMD_CSUM_OFFLOAD:
		{
			UINT_32 u4CSUMFlags;

			if (pu4IntBuf[1] == 1)
				u4CSUMFlags = CSUM_OFFLOAD_EN_ALL;
			else if (pu4IntBuf[1] == 0)
				u4CSUMFlags = 0;
			else
				return -EINVAL;

			if (kalIoctl(prGlueInfo,
				     wlanoidSetCSUMOffload,
				     (PVOID)&u4CSUMFlags,
				     sizeof(UINT_32), FALSE, FALSE, TRUE, &u4BufLen) == WLAN_STATUS_SUCCESS) {
				if (pu4IntBuf[1] == 1)
					prNetDev->features |= NETIF_F_HW_CSUM;
				else if (pu4IntBuf[1] == 0)
					prNetDev->features &= ~NETIF_F_HW_CSUM;
			}
		}
		break;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	case PRIV_CMD_POWER_MODE:
		{
			PARAM_POWER_MODE_T rPowerMode;
			P_BSS_INFO_T prBssInfo = prGlueInfo->prAdapter->prAisBssInfo;

			if (!prBssInfo)
				break;

			rPowerMode.ePowerMode = (PARAM_POWER_MODE) pu4IntBuf[1];
			rPowerMode.ucBssIdx = prBssInfo->ucBssIndex;

			/* pu4IntBuf[0] is used as input SubCmd */
			kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile, &rPowerMode,
				 sizeof(PARAM_POWER_MODE_T), FALSE, FALSE, TRUE, &u4BufLen);
		}
		break;

	case PRIV_CMD_WMM_PS:
		{
			PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T rWmmPsTest;

			rWmmPsTest.bmfgApsdEnAc = (UINT_8) pu4IntBuf[1];
			rWmmPsTest.ucIsEnterPsAtOnce = (UINT_8) pu4IntBuf[2];
			rWmmPsTest.ucIsDisableUcTrigger = (UINT_8) pu4IntBuf[3];
			rWmmPsTest.reserved = 0;

			kalIoctl(prGlueInfo,
				 wlanoidSetWiFiWmmPsTest,
				 (PVOID)&rWmmPsTest,
				 sizeof(PARAM_CUSTOM_WMM_PS_TEST_STRUCT_T), FALSE, FALSE, TRUE, &u4BufLen);
		}
		break;

#if 0
	case PRIV_CMD_ADHOC_MODE:
		/* pu4IntBuf[0] is used as input SubCmd */
		rStatus = wlanSetInformation(prGlueInfo->prAdapter, wlanoidSetAdHocMode, (PVOID)&pu4IntBuf[1],
					     sizeof(UINT_32), &u4BufLen);
		break;
#endif

	case PRIV_CUSTOM_BWCS_CMD:

		DBGLOG(REQ, INFO,
		       "pu4IntBuf[1] = %x, size of PTA_IPC_T = %d.\n", pu4IntBuf[1], sizeof(PARAM_PTA_IPC_T));

		prPtaIpc = (P_PTA_IPC_T) aucOidBuf;
		prPtaIpc->u.aucBTPParams[0] = (UINT_8) (pu4IntBuf[1] >> 24);
		prPtaIpc->u.aucBTPParams[1] = (UINT_8) (pu4IntBuf[1] >> 16);
		prPtaIpc->u.aucBTPParams[2] = (UINT_8) (pu4IntBuf[1] >> 8);
		prPtaIpc->u.aucBTPParams[3] = (UINT_8) (pu4IntBuf[1]);

		DBGLOG(REQ, INFO,
		       "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[0] = %02x, aucBTPParams[1] = %02x.\n",
		       prPtaIpc->u.aucBTPParams[0], prPtaIpc->u.aucBTPParams[1]);
		DBGLOG(REQ, INFO,
		       "BCM BWCS CMD : PRIV_CUSTOM_BWCS_CMD : aucBTPParams[2] = %02x, aucBTPParams[3] = %02x.\n",
		       prPtaIpc->u.aucBTPParams[2], prPtaIpc->u.aucBTPParams[3]);

#if 0
		status = wlanSetInformation(prGlueInfo->prAdapter,
					    wlanoidSetBT, (PVOID)&aucOidBuf[0], u4CmdLen, &u4BufLen);
#endif

		status = wlanoidSetBT(prGlueInfo->prAdapter,
				      (PVOID)&aucOidBuf[0], sizeof(PARAM_PTA_IPC_T), &u4BufLen);

		if (status != WLAN_STATUS_SUCCESS)
			status = -EFAULT;

		break;

	case PRIV_CMD_BAND_CONFIG:
		{
			DBGLOG(INIT, INFO, "CMD set_band = %lu\n", (UINT_32) pu4IntBuf[1]);
		}
		break;

#if CFG_ENABLE_WIFI_DIRECT
	case PRIV_CMD_P2P_MODE:
		{
			PARAM_CUSTOM_P2P_SET_STRUCT_T rSetP2P;
			WLAN_STATUS rWlanStatus = WLAN_STATUS_SUCCESS;

			rSetP2P.u4Enable = pu4IntBuf[1];
			rSetP2P.u4Mode = pu4IntBuf[2];
#if 1
			if (!rSetP2P.u4Enable)
				p2pNetUnregister(prGlueInfo, TRUE);

			/* pu4IntBuf[0] is used as input SubCmd */
			rWlanStatus = kalIoctl(prGlueInfo, wlanoidSetP2pMode, (PVOID)&rSetP2P,
					       sizeof(PARAM_CUSTOM_P2P_SET_STRUCT_T), FALSE, FALSE, TRUE, &u4BufLen);

			if ((rSetP2P.u4Enable) && (rWlanStatus == WLAN_STATUS_SUCCESS))
				p2pNetRegister(prGlueInfo, TRUE);
#endif

		}
		break;
#endif

#if (CFG_MET_PACKET_TRACE_SUPPORT == 1)
	case PRIV_CMD_MET_PROFILING:
		{
			/* PARAM_CUSTOM_WFD_DEBUG_STRUCT_T rWfdDebugModeInfo; */
			/* rWfdDebugModeInfo.ucWFDDebugMode=(UINT_8)pu4IntBuf[1]; */
			/* rWfdDebugModeInfo.u2SNPeriod=(UINT_16)pu4IntBuf[2]; */
			/* DBGLOG(REQ, INFO, ("WFD Debug Mode:%d Period:%d\n",
			 *  rWfdDebugModeInfo.ucWFDDebugMode, rWfdDebugModeInfo.u2SNPeriod));
			 */
			prGlueInfo->fgMetProfilingEn = (UINT_8) pu4IntBuf[1];
			prGlueInfo->u2MetUdpPort = (UINT_16) pu4IntBuf[2];
			/* DBGLOG(INIT, INFO, ("MET_PROF: Enable=%d UDP_PORT=%d\n",
			 *  prGlueInfo->fgMetProfilingEn, prGlueInfo->u2MetUdpPort);
			 */

		}
		break;

#endif

	default:
		return -EOPNOTSUPP;
	}

	return status;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl get int handler.
*
* \param[in] pDev Net device requested.
* \param[out] pIwReq Pointer to iwreq structure.
* \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
* \param[out] pcExtra The buffer with put the return value
*
* \retval 0 For success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EFAULT For fail.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_get_int(IN struct net_device *prNetDev,
	     IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
	UINT_32 u4SubCmd;
	PUINT_32 pu4IntBuf;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4BufLen = 0;
	int status = 0;
	P_NDIS_TRANSPORT_STRUCT prNdisReq;
	INT_32 ch[50];

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);
	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->mode;
	pu4IntBuf = (PUINT_32) pcExtra;

	switch (u4SubCmd) {
	case PRIV_CMD_TEST_CMD:
		/* printk("CMD=0x%08lx, data=0x%08lx\n", pu4IntBuf[1], pu4IntBuf[2]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MTK_WIFI_TEST;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			/* printk("Result=%ld\n", *(PUINT_32)&prNdisReq->ndisOidContent[4]); */
			prIwReqData->mode = *(PUINT_32) &prNdisReq->ndisOidContent[4];
			/*
			 *  if (copy_to_user(prIwReqData->data.pointer,
			 *  &prNdisReq->ndisOidContent[4], 4)) {
			 *  printk(KERN_NOTICE "priv_get_int() copy_to_user oidBuf fail(3)\n");
			 *  return -EFAULT;
			 *  }
			 */
		}
		return status;

#if CFG_SUPPORT_PRIV_MCR_RW
	case PRIV_CMD_ACCESS_MCR:
		/* printk("addr=0x%08lx\n", pu4IntBuf[1]); */
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		if (!prGlueInfo->fgMcrAccessAllowed) {
			status = 0;
			return status;
		}

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MCR_RW;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			/* printk("Result=%ld\n", *(PUINT_32)&prNdisReq->ndisOidContent[4]); */
			prIwReqData->mode = *(PUINT_32) &prNdisReq->ndisOidContent[4];
		}
		return status;
#endif

	case PRIV_CMD_DUMP_MEM:
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

#if 1
		if (!prGlueInfo->fgMcrAccessAllowed) {
			status = 0;
			return status;
		}
#endif
		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_MEM_DUMP;
		prNdisReq->inNdisOidlength = sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T);
		prNdisReq->outNdisOidLength = sizeof(PARAM_CUSTOM_MEM_DUMP_STRUCT_T);

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0)
			prIwReqData->mode = *(PUINT_32) &prNdisReq->ndisOidContent[0];
		return status;

	case PRIV_CMD_SW_CTRL:
		/* printk(" addr=0x%08lx\n", pu4IntBuf[1]); */

		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		kalMemCopy(&prNdisReq->ndisOidContent[0], &pu4IntBuf[1], 8);

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			/* printk("Result=%ld\n", *(PUINT_32)&prNdisReq->ndisOidContent[4]); */
			prIwReqData->mode = *(PUINT_32) &prNdisReq->ndisOidContent[4];
		}
		return status;

#if 0
	case PRIV_CMD_BEACON_PERIOD:
		status = wlanQueryInformation(prGlueInfo->prAdapter,
					      wlanoidQueryBeaconInterval,
					      (PVOID) pu4IntBuf, sizeof(UINT_32), &u4BufLen);
		return status;

	case PRIV_CMD_POWER_MODE:
		status = wlanQueryInformation(prGlueInfo->prAdapter,
					      wlanoidQuery802dot11PowerSaveProfile,
					      (PVOID) pu4IntBuf, sizeof(UINT_32), &u4BufLen);
		return status;

	case PRIV_CMD_ADHOC_MODE:
		status = wlanQueryInformation(prGlueInfo->prAdapter,
					      wlanoidQueryAdHocMode, (PVOID) pu4IntBuf, sizeof(UINT_32), &u4BufLen);
		return status;
#endif

	case PRIV_CMD_BAND_CONFIG:
		DBGLOG(INIT, INFO, "CMD get_band=\n");
		prIwReqData->mode = 0;
		return status;

	default:
		break;
	}

	u4SubCmd = (UINT_32) prIwReqData->data.flags;

	switch (u4SubCmd) {
	case PRIV_CMD_GET_CH_LIST:
		{
			UINT_16 i, j = 0;
			UINT_8 NumOfChannel = 50;
			UINT_8 ucMaxChannelNum = 50;
			RF_CHANNEL_INFO_T aucChannelList[50];

			DBGLOG(RLM, INFO, "Domain: Query Channel List.\n");
			kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum, &NumOfChannel, aucChannelList);
			if (NumOfChannel > 50)
				NumOfChannel = 50;

			if (kalIsAPmode(prGlueInfo)) {
				for (i = 0; i < NumOfChannel; i++) {
					if ((aucChannelList[i].ucChannelNum <= 13) ||
					    (aucChannelList[i].ucChannelNum == 36
					     || aucChannelList[i].ucChannelNum == 40
					     || aucChannelList[i].ucChannelNum == 44
					     || aucChannelList[i].ucChannelNum == 48)) {
						ch[j] = (INT_32) aucChannelList[i].ucChannelNum;
						j++;
					}
				}
			} else {
				for (j = 0; j < NumOfChannel; j++)
					ch[j] = (INT_32) aucChannelList[j].ucChannelNum;
			}

			prIwReqData->data.length = j;
			if (copy_to_user(prIwReqData->data.pointer, ch, NumOfChannel * sizeof(INT_32)))
				return -EFAULT;
			else
				return status;
		}
	default:
		return -EOPNOTSUPP;
	}

	return status;
}				/* priv_get_int */

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl set int array handler.
*
* \param[in] prNetDev Net device requested.
* \param[in] prIwReqInfo Pointer to iwreq structure.
* \param[in] prIwReqData The ioctl data structure, use the field of sub-command.
* \param[in] pcExtra The buffer with input value
*
* \retval 0 For success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EINVAL If a value is out of range.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_set_ints(IN struct net_device *prNetDev,
	      IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
	UINT_16 i = 0;
	UINT_32 u4SubCmd, u4BufLen, u4CmdLen;
	P_GLUE_INFO_T prGlueInfo;
	INT_32  setting[4] = {0};
	int status = 0;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_SET_TXPWR_CTRL_T prTxpwr;

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);

	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->data.flags;
	u4CmdLen = (UINT_32) prIwReqData->data.length;

	switch (u4SubCmd) {
	case PRIV_CMD_SET_TX_POWER:
		{
			if (u4CmdLen > 4)
				return -EINVAL;
			if (copy_from_user(setting, prIwReqData->data.pointer, u4CmdLen))
				return -EFAULT;

#if 0
			DBGLOG(INIT, INFO, "Tx power num = %d\n", prIwReqData->data.length);

			DBGLOG(INIT, INFO,
			       "Tx power setting = %d %d %d %d\n", setting[0], setting[1], setting[2], setting[3]);
#endif
			prTxpwr = &prGlueInfo->rTxPwr;
			if (setting[0] == 0 && prIwReqData->data.length == 4 /* argc num */) {
				/* 0 (All networks), 1 (legacy STA), 2 (Hotspot AP), 3 (P2P), 4 (BT over Wi-Fi) */
				if (setting[1] == 1 || setting[1] == 0) {
					if (setting[2] == 0 || setting[2] == 1)
						prTxpwr->c2GLegacyStaPwrOffset = setting[3];
					if (setting[2] == 0 || setting[2] == 2)
						prTxpwr->c5GLegacyStaPwrOffset = setting[3];
				}
				if (setting[1] == 2 || setting[1] == 0) {
					if (setting[2] == 0 || setting[2] == 1)
						prTxpwr->c2GHotspotPwrOffset = setting[3];
					if (setting[2] == 0 || setting[2] == 2)
						prTxpwr->c5GHotspotPwrOffset = setting[3];
				}
				if (setting[1] == 3 || setting[1] == 0) {
					if (setting[2] == 0 || setting[2] == 1)
						prTxpwr->c2GP2pPwrOffset = setting[3];
					if (setting[2] == 0 || setting[2] == 2)
						prTxpwr->c5GP2pPwrOffset = setting[3];
				}
				if (setting[1] == 4 || setting[1] == 0) {
					if (setting[2] == 0 || setting[2] == 1)
						prTxpwr->c2GBowPwrOffset = setting[3];
					if (setting[2] == 0 || setting[2] == 2)
						prTxpwr->c5GBowPwrOffset = setting[3];
				}
			} else if (setting[0] == 1 && prIwReqData->data.length == 2) {
				prTxpwr->ucConcurrencePolicy = setting[1];
			} else if (setting[0] == 2 && prIwReqData->data.length == 3) {
				if (setting[1] == 0) {
					for (i = 0; i < 14; i++)
						prTxpwr->acTxPwrLimit2G[i] = setting[2];
				} else if (setting[1] <= 14)
					prTxpwr->acTxPwrLimit2G[setting[1] - 1] = setting[2];
			} else if (setting[0] == 3 && prIwReqData->data.length == 3) {
				if (setting[1] == 0) {
					for (i = 0; i < 4; i++)
						prTxpwr->acTxPwrLimit5G[i] = setting[2];
				} else if (setting[1] <= 4)
					prTxpwr->acTxPwrLimit5G[setting[1] - 1] = setting[2];
			} else if (setting[0] == 4 && prIwReqData->data.length == 2) {
				if (setting[1] == 0)
					wlanDefTxPowerCfg(prGlueInfo->prAdapter);
				rStatus = kalIoctl(prGlueInfo,
						   wlanoidSetTxPower,
						   prTxpwr, sizeof(SET_TXPWR_CTRL_T), FALSE, FALSE, TRUE, &u4BufLen);
			} else
				return -EFAULT;
		}
		return status;
	default:
		break;
	}

	return status;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl get int array handler.
*
* \param[in] pDev Net device requested.
* \param[out] pIwReq Pointer to iwreq structure.
* \param[in] prIwReqData The ioctl req structure, use the field of sub-command.
* \param[out] pcExtra The buffer with put the return value
*
* \retval 0 For success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EFAULT For fail.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_get_ints(IN struct net_device *prNetDev,
	      IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
	UINT_32 u4SubCmd;
	P_GLUE_INFO_T prGlueInfo;
	int status = 0;
	INT_32 ch[50];

	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);
	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->data.flags;

	switch (u4SubCmd) {
	case PRIV_CMD_GET_CH_LIST:
		{
			UINT_16 i;
			UINT_8 NumOfChannel = 50;
			UINT_8 ucMaxChannelNum = 50;
			RF_CHANNEL_INFO_T aucChannelList[50];

			kalGetChannelList(prGlueInfo, BAND_NULL, ucMaxChannelNum, &NumOfChannel, aucChannelList);
			if (NumOfChannel > 50)
				NumOfChannel = 50;

			for (i = 0; i < NumOfChannel; i++)
				ch[i] = (INT_32) aucChannelList[i].ucChannelNum;

			prIwReqData->data.length = NumOfChannel;
			if (copy_to_user(prIwReqData->data.pointer, ch, NumOfChannel * sizeof(INT_32)))
				return -EFAULT;
			else
				return status;
		}
	default:
		break;
	}

	return status;
}				/* priv_get_int */

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl set structure handler.
*
* \param[in] pDev Net device requested.
* \param[in] prIwReqData Pointer to iwreq_data structure.
*
* \retval 0 For success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EINVAL If a value is out of range.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_set_struct(IN struct net_device *prNetDev,
		IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
	UINT_32 u4SubCmd = 0;
	int status = 0;
	/* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
	UINT_32 u4CmdLen = 0;
	P_NDIS_TRANSPORT_STRUCT prNdisReq;

	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 u4BufLen = 0;

	ASSERT(prNetDev);
	/* ASSERT(prIwReqInfo); */
	ASSERT(prIwReqData);
	/* ASSERT(pcExtra); */

	kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

	if (GLUE_CHK_PR2(prNetDev, prIwReqData) == FALSE)
		return -EINVAL;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->data.flags;

#if 0
	DBGLOG(INIT, INFO, "priv_set_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n", prIwReqInfo->cmd, u4SubCmd);
#endif

	switch (u4SubCmd) {
#if 0				/* PTA_ENABLED */
	case PRIV_CMD_BT_COEXIST:
		u4CmdLen = prIwReqData->data.length * sizeof(UINT_32);
		ASSERT(sizeof(PARAM_CUSTOM_BT_COEXIST_T) >= u4CmdLen);
		if (sizeof(PARAM_CUSTOM_BT_COEXIST_T) < u4CmdLen)
			return -EFAULT;

		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer, u4CmdLen)) {
			status = -EFAULT;	/* return -EFAULT; */
			break;
		}

		rStatus = wlanSetInformation(prGlueInfo->prAdapter,
					     wlanoidSetBtCoexistCtrl, (PVOID)&aucOidBuf[0], u4CmdLen, &u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS)
			status = -EFAULT;
		break;
#endif

	case PRIV_CUSTOM_BWCS_CMD:
		u4CmdLen = prIwReqData->data.length * sizeof(UINT_32);
		ASSERT(sizeof(PARAM_PTA_IPC_T) >= u4CmdLen);
		if (sizeof(PARAM_PTA_IPC_T) < u4CmdLen)
			return -EFAULT;
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
		DBGLOG(REQ, INFO,
		       "ucCmdLen = %d, size of PTA_IPC_T = %d, prIwReqData->data = 0x%x.\n",
		       u4CmdLen, sizeof(PARAM_PTA_IPC_T), prIwReqData->data);

		DBGLOG(REQ, INFO, "priv_set_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n", prIwReqInfo->cmd,
		       u4SubCmd);
		DBGLOG(REQ, INFO, "*pcExtra = 0x%x\n", *pcExtra);
#endif

		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer, u4CmdLen)) {
			status = -EFAULT;	/* return -EFAULT; */
			break;
		}
#if CFG_SUPPORT_BCM && CFG_SUPPORT_BCM_BWCS && CFG_SUPPORT_BCM_BWCS_DEBUG
		DBGLOG(REQ, INFO, "priv_set_struct(): BWCS CMD = %02x%02x%02x%02x\n", aucOidBuf[2], aucOidBuf[3],
		       aucOidBuf[4], aucOidBuf[5]);
#endif

#if 0
		status = wlanSetInformation(prGlueInfo->prAdapter,
					    wlanoidSetBT, (PVOID)&aucOidBuf[0], u4CmdLen, &u4BufLen);
#endif

#if 1
		status = wlanoidSetBT(prGlueInfo->prAdapter, (PVOID)&aucOidBuf[0], u4CmdLen, &u4BufLen);
#endif

		if (status != WLAN_STATUS_SUCCESS)
			status = -EFAULT;

		break;

#if CFG_SUPPORT_WPS2
	case PRIV_CMD_WSC_PROBE_REQ:
		{
			/* retrieve IE for Probe Request */
			u4CmdLen = prIwReqData->data.length;
			if (u4CmdLen > GLUE_INFO_WSCIE_LENGTH) {
				DBGLOG(REQ, ERROR, "Input data length is invalid %u\n", u4CmdLen);
				return -EINVAL;
			}

			if (prIwReqData->data.length > 0) {
				if (copy_from_user(prGlueInfo->aucWSCIE,
						   prIwReqData->data.pointer,
						   u4CmdLen)) {
					status = -EFAULT;
					break;
				}
				prGlueInfo->u2WSCIELen = u4CmdLen;
			} else {
				prGlueInfo->u2WSCIELen = 0;
			}
		}
		break;
#endif
	case PRIV_CMD_OID:
		u4CmdLen = prIwReqData->data.length;
		if (u4CmdLen > CMD_OID_BUF_LENGTH) {
			DBGLOG(REQ, ERROR, "Input data length is invalid %u\n", u4CmdLen);
			return -EINVAL;
		}
		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer, u4CmdLen)) {
			status = -EFAULT;
			break;
		}
		if (!kalMemCmp(&aucOidBuf[0], pcExtra, u4CmdLen)) {
			/* ToDo:: DBGLOG */
			DBGLOG(REQ, INFO, "pcExtra buffer is valid\n");
		} else {
			DBGLOG(REQ, INFO, "pcExtra 0x%p\n", pcExtra);
		}
		/* Execute this OID */
		status = priv_set_ndis(prNetDev, (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0], &u4BufLen);
		/* Copy result to user space */
		((P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0])->outNdisOidLength = u4BufLen;

		if (copy_to_user(prIwReqData->data.pointer,
				 &aucOidBuf[0], OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
			DBGLOG(REQ, INFO, "copy_to_user oidBuf fail\n");
			status = -EFAULT;
		}

		break;

	case PRIV_CMD_SW_CTRL:
		u4CmdLen = prIwReqData->data.length;
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		if (u4CmdLen > sizeof(prNdisReq->ndisOidContent)) {
			DBGLOG(REQ, ERROR, "Input data length is invalid %u\n", u4CmdLen);
			return -EINVAL;
		}

		if (copy_from_user(&prNdisReq->ndisOidContent[0],
			prIwReqData->data.pointer, u4CmdLen)) {
			status = -EFAULT;
			break;
		}
		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		/* Execute this OID */
		status = priv_set_ndis(prNetDev, prNdisReq, &u4BufLen);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return status;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl get struct handler.
*
* \param[in] pDev Net device requested.
* \param[out] pIwReq Pointer to iwreq structure.
* \param[in] cmd Private sub-command.
*
* \retval 0 For success.
* \retval -EFAULT If copy from user space buffer fail.
* \retval -EOPNOTSUPP Parameter "cmd" not recognized.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_get_struct(IN struct net_device *prNetDev,
		IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
	UINT_32 u4SubCmd = 0;
	P_NDIS_TRANSPORT_STRUCT prNdisReq = NULL;

	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 u4BufLen = 0;
	PUINT_32 pu4IntBuf = NULL;
	int status = 0;

	kalMemZero(&aucOidBuf[0], sizeof(aucOidBuf));

	ASSERT(prNetDev);
	ASSERT(prIwReqData);
	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO, "priv_get_struct(): invalid param(0x%p, 0x%p)\n", prNetDev, prIwReqData);
		return -EINVAL;
	}

	u4SubCmd = (UINT_32) prIwReqData->data.flags;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO, "priv_get_struct(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev, *((P_GLUE_INFO_T *) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_get_struct(): prIwReqInfo->cmd(0x%X), u4SubCmd(%ld)\n", prIwReqInfo->cmd, u4SubCmd);
#endif
	memset(aucOidBuf, 0, sizeof(aucOidBuf));

	switch (u4SubCmd) {
	case PRIV_CMD_OID:
		if (copy_from_user(&aucOidBuf[0], prIwReqData->data.pointer, sizeof(NDIS_TRANSPORT_STRUCT))) {
			DBGLOG(REQ, INFO, "priv_get_struct() copy_from_user oidBuf fail\n");
			return -EFAULT;
		}

		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];
#if 0
		DBGLOG(INIT, INFO, "\n priv_get_struct cmd 0x%02x len:%d OID:0x%08x OID Len:%d\n", cmd,
		       pIwReq->u.data.length, ndisReq->ndisOidCmd, ndisReq->inNdisOidlength);
#endif
		if (priv_get_ndis(prNetDev, prNdisReq, &u4BufLen) == 0) {
			prNdisReq->outNdisOidLength = u4BufLen;
			if (copy_to_user(prIwReqData->data.pointer,
					 &aucOidBuf[0],
					 u4BufLen + sizeof(NDIS_TRANSPORT_STRUCT) -
					 sizeof(prNdisReq->ndisOidContent))) {
				DBGLOG(REQ, INFO, "priv_get_struct() copy_to_user oidBuf fail(1)\n");
				return -EFAULT;
			}
			return 0;
		}
		prNdisReq->outNdisOidLength = u4BufLen;
		if (copy_to_user(prIwReqData->data.pointer,
				 &aucOidBuf[0], OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
			DBGLOG(REQ, INFO, "priv_get_struct() copy_to_user oidBuf fail(2)\n");
		}
		return -EFAULT;

	case PRIV_CMD_SW_CTRL:
		pu4IntBuf = (PUINT_32) prIwReqData->data.pointer;
		prNdisReq = (P_NDIS_TRANSPORT_STRUCT) &aucOidBuf[0];

		if (prIwReqData->data.length > (sizeof(aucOidBuf) - OFFSET_OF(NDIS_TRANSPORT_STRUCT, ndisOidContent))) {
			DBGLOG(REQ, INFO, "priv_get_struct() exceeds length limit\n");
			return -EFAULT;
		}

		if (copy_from_user(&prNdisReq->ndisOidContent[0], prIwReqData->data.pointer,
			prIwReqData->data.length)) {
			DBGLOG(REQ, INFO, "priv_get_struct() copy_from_user oidBuf fail\n");
			return -EFAULT;
		}

		prNdisReq->ndisOidCmd = OID_CUSTOM_SW_CTRL;
		prNdisReq->inNdisOidlength = 8;
		prNdisReq->outNdisOidLength = 8;

		status = priv_get_ndis(prNetDev, prNdisReq, &u4BufLen);
		if (status == 0) {
			prNdisReq->outNdisOidLength = u4BufLen;
			/* printk("len=%d Result=%08lx\n", u4BufLen, *(PUINT_32)&prNdisReq->ndisOidContent[4]); */

			if (copy_to_user(prIwReqData->data.pointer, &prNdisReq->ndisOidContent[4], 4))
				DBGLOG(REQ, INFO, "priv_get_struct() copy_to_user oidBuf fail(2)\n");
		}
		return 0;
	default:
		DBGLOG(REQ, WARN, "get struct cmd:0x%lx\n", u4SubCmd);
		return -EOPNOTSUPP;
	}
}				/* priv_get_struct */

/*----------------------------------------------------------------------------*/
/*!
* \brief The routine handles a set operation for a single OID.
*
* \param[in] pDev Net device requested.
* \param[in] ndisReq Ndis request OID information copy from user.
* \param[out] outputLen_p If the call is successful, returns the number of
*                         bytes written into the query buffer. If the
*                         call failed due to invalid length of the query
*                         buffer, returns the amount of storage needed..
*
* \retval 0 On success.
* \retval -EOPNOTSUPP If cmd is not supported.
*
*/
/*----------------------------------------------------------------------------*/
static int
priv_set_ndis(IN struct net_device *prNetDev, IN NDIS_TRANSPORT_STRUCT * prNdisReq, OUT PUINT_32 pu4OutputLen)
{
	P_WLAN_REQ_ENTRY prWlanReqEntry = NULL;
	WLAN_STATUS status = WLAN_STATUS_SUCCESS;
	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 u4SetInfoLen = 0;

	ASSERT(prNetDev);
	ASSERT(prNdisReq);
	ASSERT(pu4OutputLen);

	if (!prNetDev || !prNdisReq || !pu4OutputLen) {
		DBGLOG(REQ, INFO, "priv_set_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
		       prNetDev, prNdisReq, pu4OutputLen);
		return -EINVAL;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO, "priv_set_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev, *((P_GLUE_INFO_T *) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_set_ndis(): prNdisReq->ndisOidCmd(0x%lX)\n", prNdisReq->ndisOidCmd);
#endif

	if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd, &prWlanReqEntry) == FALSE) {
		/* WARNLOG(("Set OID: 0x%08lx (unknown)\n", prNdisReq->ndisOidCmd)); */
		return -EOPNOTSUPP;
	}

	if (prWlanReqEntry->pfOidSetHandler == NULL) {
		/* WARNLOG(("Set %s: Null set handler\n", prWlanReqEntry->pucOidName)); */
		return -EOPNOTSUPP;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_set_ndis(): %s\n", prWlanReqEntry->pucOidName);
#endif

	if (prWlanReqEntry->fgSetBufLenChecking) {
		if (prNdisReq->inNdisOidlength != prWlanReqEntry->u4InfoBufLen) {
			DBGLOG(REQ, WARN, "Set %s: Invalid length (current=%ld, needed=%ld)\n",
			       prWlanReqEntry->pucOidName, prNdisReq->inNdisOidlength, prWlanReqEntry->u4InfoBufLen);

			*pu4OutputLen = prWlanReqEntry->u4InfoBufLen;
			return -EINVAL;
		}
	}

	if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
		/* GLUE sw info only */
		status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
							 prNdisReq->ndisOidContent,
							 prNdisReq->inNdisOidlength, &u4SetInfoLen);
	} else if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_EXTENSION) {
		/* multiple sw operations */
		status = prWlanReqEntry->pfOidSetHandler(prGlueInfo,
							 prNdisReq->ndisOidContent,
							 prNdisReq->inNdisOidlength, &u4SetInfoLen);
	} else if (prWlanReqEntry->eOidMethod == ENUM_OID_DRIVER_CORE) {
		/* driver core */

		status = kalIoctl(prGlueInfo,
				  (PFN_OID_HANDLER_FUNC) prWlanReqEntry->pfOidSetHandler,
				  prNdisReq->ndisOidContent,
				  prNdisReq->inNdisOidlength, FALSE, FALSE, TRUE, &u4SetInfoLen);
	} else {
		DBGLOG(REQ, INFO, "priv_set_ndis(): unsupported OID method:0x%x\n", prWlanReqEntry->eOidMethod);
		return -EOPNOTSUPP;
	}

	*pu4OutputLen = u4SetInfoLen;

	switch (status) {
	case WLAN_STATUS_SUCCESS:
		break;

	case WLAN_STATUS_INVALID_LENGTH:
		/* WARNLOG(("Set %s: Invalid length (current=%ld, needed=%ld)\n", */
		/* prWlanReqEntry->pucOidName, */
		/* prNdisReq->inNdisOidlength, */
		/* u4SetInfoLen)); */
		break;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return -EFAULT;

	return 0;
}				/* priv_set_ndis */

/*----------------------------------------------------------------------------*/
/*!
* \brief The routine handles a query operation for a single OID. Basically we
*   return information about the current state of the OID in question.
*
* \param[in] pDev Net device requested.
* \param[in] ndisReq Ndis request OID information copy from user.
* \param[out] outputLen_p If the call is successful, returns the number of
*                        bytes written into the query buffer. If the
*                        call failed due to invalid length of the query
*                        buffer, returns the amount of storage needed..
*
* \retval 0 On success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EINVAL invalid input parameters
*
*/
/*----------------------------------------------------------------------------*/
static int
priv_get_ndis(IN struct net_device *prNetDev, IN NDIS_TRANSPORT_STRUCT * prNdisReq, OUT PUINT_32 pu4OutputLen)
{
	P_WLAN_REQ_ENTRY prWlanReqEntry = NULL;
	UINT_32 u4BufLen = 0;
	WLAN_STATUS status = WLAN_STATUS_SUCCESS;
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prNetDev);
	ASSERT(prNdisReq);
	ASSERT(pu4OutputLen);

	if (!prNetDev || !prNdisReq || !pu4OutputLen) {
		DBGLOG(REQ, INFO, "priv_get_ndis(): invalid param(0x%p, 0x%p, 0x%p)\n",
		       prNetDev, prNdisReq, pu4OutputLen);
		return -EINVAL;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO, "priv_get_ndis(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev, *((P_GLUE_INFO_T *) netdev_priv(prNetDev)));
		return -EINVAL;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_get_ndis(): prNdisReq->ndisOidCmd(0x%lX)\n", prNdisReq->ndisOidCmd);
#endif

	if (reqSearchSupportedOidEntry(prNdisReq->ndisOidCmd, &prWlanReqEntry) == FALSE) {
		/* WARNLOG(("Query OID: 0x%08lx (unknown)\n", prNdisReq->ndisOidCmd)); */
		return -EOPNOTSUPP;
	}

	if (prWlanReqEntry->pfOidQueryHandler == NULL) {
		/* WARNLOG(("Query %s: Null query handler\n", prWlanReqEntry->pucOidName)); */
		return -EOPNOTSUPP;
	}
#if 0
	DBGLOG(INIT, INFO, "priv_get_ndis(): %s\n", prWlanReqEntry->pucOidName);
#endif

	if (prWlanReqEntry->fgQryBufLenChecking) {
		if (prNdisReq->inNdisOidlength < prWlanReqEntry->u4InfoBufLen) {
			/* Not enough room in InformationBuffer. Punt */
			/* WARNLOG(("Query %s: Buffer too short (current=%ld, needed=%ld)\n", */
			/* prWlanReqEntry->pucOidName, */
			/* prNdisReq->inNdisOidlength, */
			/* prWlanReqEntry->u4InfoBufLen)); */

			*pu4OutputLen = prWlanReqEntry->u4InfoBufLen;

			status = WLAN_STATUS_INVALID_LENGTH;
			return -EINVAL;
		}
	}

	if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_ONLY) {
		/* GLUE sw info only */
		status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
							   prNdisReq->ndisOidContent,
							   prNdisReq->inNdisOidlength, &u4BufLen);
	} else if (prWlanReqEntry->eOidMethod == ENUM_OID_GLUE_EXTENSION) {
		/* multiple sw operations */
		status = prWlanReqEntry->pfOidQueryHandler(prGlueInfo,
							   prNdisReq->ndisOidContent,
							   prNdisReq->inNdisOidlength, &u4BufLen);
	} else if (prWlanReqEntry->eOidMethod == ENUM_OID_DRIVER_CORE) {
		/* driver core */

		status = kalIoctl(prGlueInfo,
				  (PFN_OID_HANDLER_FUNC) prWlanReqEntry->pfOidQueryHandler,
				  prNdisReq->ndisOidContent, prNdisReq->inNdisOidlength, TRUE, TRUE, TRUE, &u4BufLen);
	} else {
		DBGLOG(REQ, INFO, "priv_set_ndis(): unsupported OID method:0x%x\n", prWlanReqEntry->eOidMethod);
		return -EOPNOTSUPP;
	}

	*pu4OutputLen = u4BufLen;

	switch (status) {
	case WLAN_STATUS_SUCCESS:
		break;

	case WLAN_STATUS_INVALID_LENGTH:
		/* WARNLOG(("Set %s: Invalid length (current=%ld, needed=%ld)\n", */
		/* prWlanReqEntry->pucOidName, */
		/* prNdisReq->inNdisOidlength, */
		/* u4BufLen)); */
		break;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return -EOPNOTSUPP;

	return 0;
}				/* priv_get_ndis */

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
* \brief The routine handles ATE set operation.
*
* \param[in] pDev Net device requested.
* \param[in] ndisReq Ndis request OID information copy from user.
* \param[out] outputLen_p If the call is successful, returns the number of
*                         bytes written into the query buffer. If the
*                         call failed due to invalid length of the query
*                         buffer, returns the amount of storage needed..
*
* \retval 0 On success.
* \retval -EOPNOTSUPP If cmd is not supported.
* \retval -EFAULT If copy from user space buffer fail.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_ate_set(IN struct net_device *prNetDev,
	     IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN char *pcExtra)
{
	P_GLUE_INFO_T GlueInfo;
	INT_32 i4Status;
	UINT_8 *InBuf;
	/* UINT_8 *addr_str, *value_str; */
	UINT_32 InBufLen;
	UINT_32 u4SubCmd;
	/* BOOLEAN isWrite = 0;
	*UINT_32 u4BufLen = 0;
	*P_NDIS_TRANSPORT_STRUCT prNdisReq;
	*UINT_32 pu4IntBuf[2];
	*/

	/* sanity check */
	ASSERT(prNetDev);
	ASSERT(prIwReqInfo);
	ASSERT(prIwReqData);
	ASSERT(pcExtra);

	/* init */
	DBGLOG(REQ, INFO, "priv_set_string (%s)(%d)\n",
	       (UINT_8 *) prIwReqData->data.pointer, (INT_32) prIwReqData->data.length);

	if (GLUE_CHK_PR3(prNetDev, prIwReqData, pcExtra) == FALSE)
		return -EINVAL;

	GlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	u4SubCmd = (UINT_32) prIwReqData->data.flags;

	DBGLOG(REQ, INFO, "MT6632 : priv_ate_set u4SubCmd = %d\n", u4SubCmd);

	switch (u4SubCmd) {
	case PRIV_QACMD_SET:
		DBGLOG(REQ, INFO, "MT6632 : priv_ate_set PRIV_QACMD_SET\n");
		InBuf = aucOidBuf;
		InBufLen = prIwReqData->data.length;
		i4Status = 0;

		if (copy_from_user(InBuf, prIwReqData->data.pointer, prIwReqData->data.length))
			return -EFAULT;
		i4Status = AteCmdSetHandle(prNetDev, InBuf, InBufLen);
		break;

	default:
		return -EOPNOTSUPP;
	}
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to search desired OID.
*
* \param rOid[in]               Desired NDIS_OID
* \param ppWlanReqEntry[out]    Found registered OID entry
*
* \retval TRUE: Matched OID is found
* \retval FALSE: No matched OID is found
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN reqSearchSupportedOidEntry(IN UINT_32 rOid, OUT P_WLAN_REQ_ENTRY *ppWlanReqEntry)
{
	INT_32 i, j, k;

	i = 0;
	j = NUM_SUPPORTED_OIDS - 1;

	while (i <= j) {
		k = (i + j) / 2;

		if (rOid == arWlanOidReqTable[k].rOid) {
			*ppWlanReqEntry = &arWlanOidReqTable[k];
			return TRUE;
		} else if (rOid < arWlanOidReqTable[k].rOid) {
			j = k - 1;
		} else {
			i = k + 1;
		}
	}

	return FALSE;
}				/* reqSearchSupportedOidEntry */

/*----------------------------------------------------------------------------*/
/*!
* \brief Private ioctl driver handler.
*
* \param[in] pDev Net device requested.
* \param[out] pIwReq Pointer to iwreq structure.
* \param[in] cmd Private sub-command.
*
* \retval 0 For success.
* \retval -EFAULT If copy from user space buffer fail.
* \retval -EOPNOTSUPP Parameter "cmd" not recognized.
*
*/
/*----------------------------------------------------------------------------*/
int
priv_set_driver(IN struct net_device *prNetDev,
		IN struct iw_request_info *prIwReqInfo, IN union iwreq_data *prIwReqData, IN OUT char *pcExtra)
{
	UINT_32 u4SubCmd = 0;
	UINT_16 u2Cmd = 0;

	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4BytesWritten = 0;

	ASSERT(prNetDev);
	ASSERT(prIwReqData);
	if (!prNetDev || !prIwReqData) {
		DBGLOG(REQ, INFO, "priv_set_driver(): invalid param(0x%p, 0x%p)\n", prNetDev, prIwReqData);
		return -EINVAL;
	}

	u2Cmd = prIwReqInfo->cmd;
	DBGLOG(REQ, INFO, "prIwReqInfo->cmd %u\n", u2Cmd);

	u4SubCmd = (UINT_32) prIwReqData->data.flags;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, INFO, "priv_set_driver(): invalid prGlueInfo(0x%p, 0x%p)\n",
		       prNetDev, *((P_GLUE_INFO_T *) netdev_priv(prNetDev)));
		return -EINVAL;
	}

	/* trick,hack in ./net/wireless/wext-priv.c ioctl_private_iw_point */
	/* because the cmd number is odd (get), the input string will not be copy_to_user */

	DBGLOG(REQ, INFO, "prIwReqData->data.length %u\n", prIwReqData->data.length);

	/* Use GET type becauase large data by iwpriv. */

	ASSERT(IW_IS_GET(u2Cmd));
	if (prIwReqData->data.length != 0) {
		if (!access_ok(VERIFY_READ, prIwReqData->data.pointer, prIwReqData->data.length)) {
			DBGLOG(REQ, INFO, "%s access_ok Read fail written = %d\n", __func__, i4BytesWritten);
			return -EFAULT;
		}
		if (copy_from_user(pcExtra, prIwReqData->data.pointer, prIwReqData->data.length)) {
			DBGLOG(REQ, INFO, "%s copy_form_user fail written = %d\n", __func__, prIwReqData->data.length);
			return -EFAULT;
		}
	}

	if (pcExtra) {
		DBGLOG(REQ, INFO, "pcExtra %s\n", pcExtra);
		/* Please check max length in rIwPrivTable */
		DBGLOG(REQ, INFO, "%s prIwReqData->data.length = %d\n", __func__, prIwReqData->data.length);
		i4BytesWritten = priv_driver_cmds(prNetDev, pcExtra, 2000 /*prIwReqData->data.length */);
		DBGLOG(REQ, INFO, "%s i4BytesWritten = %d\n", __func__, i4BytesWritten);
	}

	DBGLOG(REQ, INFO, "pcExtra done\n");

	if (i4BytesWritten > 0) {

		if (i4BytesWritten > 2000)
			i4BytesWritten = 2000;
		prIwReqData->data.length = i4BytesWritten;	/* the iwpriv will use the length */

	} else if (i4BytesWritten == 0) {
		prIwReqData->data.length = i4BytesWritten;
	}
#if 0
	/* trick,hack in ./net/wireless/wext-priv.c ioctl_private_iw_point */
	/* because the cmd number is even (set), the return string will not be copy_to_user */
	ASSERT(IW_IS_SET(u2Cmd));
	if (!access_ok(VERIFY_WRITE, prIwReqData->data.pointer, i4BytesWritten)) {
		DBGLOG(REQ, INFO, "%s access_ok Write fail written = %d\n", __func__, i4BytesWritten);
		return -EFAULT;
	}
	if (copy_to_user(prIwReqData->data.pointer, pcExtra, i4BytesWritten)) {
		DBGLOG(REQ, INFO, "%s copy_to_user fail written = %d\n", __func__, i4BytesWritten);
		return -EFAULT;
	}
	DBGLOG(RSN, INFO, "%s copy_to_user written = %d\n", __func__, i4BytesWritten);
#endif
	return 0;

}				/* priv_set_driver */
#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to query the radio configuration used in IBSS
*        mode and RF test mode.
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[out] pvQueryBuffer     Pointer to the buffer that holds the result of the query.
* \param[in] u4QueryBufferLen   The length of the query buffer.
* \param[out] pu4QueryInfoLen   If the call is successful, returns the number of
*                               bytes written into the query buffer. If the call
*                               failed due to invalid length of the query buffer,
*                               returns the amount of storage needed.
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
*/
/*----------------------------------------------------------------------------*/
static WLAN_STATUS
reqExtQueryConfiguration(IN P_GLUE_INFO_T prGlueInfo,
			 OUT PVOID pvQueryBuffer, IN UINT_32 u4QueryBufferLen, OUT PUINT_32 pu4QueryInfoLen)
{
	P_PARAM_802_11_CONFIG_T prQueryConfig = (P_PARAM_802_11_CONFIG_T) pvQueryBuffer;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4QueryInfoLen = 0;

	DEBUGFUNC("wlanoidQueryConfiguration");

	ASSERT(prGlueInfo);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(PARAM_802_11_CONFIG_T);
	if (u4QueryBufferLen < sizeof(PARAM_802_11_CONFIG_T))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvQueryBuffer);

	kalMemZero(prQueryConfig, sizeof(PARAM_802_11_CONFIG_T));

	/* Update the current radio configuration. */
	prQueryConfig->u4Length = sizeof(PARAM_802_11_CONFIG_T);

#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidSetBeaconInterval,
			       &prQueryConfig->u4BeaconPeriod, sizeof(UINT_32), TRUE, TRUE, &u4QueryInfoLen);
#else
	rStatus = wlanQueryInformation(prGlueInfo->prAdapter,
				       wlanoidQueryBeaconInterval,
				       &prQueryConfig->u4BeaconPeriod, sizeof(UINT_32), &u4QueryInfoLen);
#endif
	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;
#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidQueryAtimWindow,
			       &prQueryConfig->u4ATIMWindow, sizeof(UINT_32), TRUE, TRUE, &u4QueryInfoLen);
#else
	rStatus = wlanQueryInformation(prGlueInfo->prAdapter,
				       wlanoidQueryAtimWindow,
				       &prQueryConfig->u4ATIMWindow, sizeof(UINT_32), &u4QueryInfoLen);
#endif
	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;
#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidQueryFrequency,
			       &prQueryConfig->u4DSConfig, sizeof(UINT_32), TRUE, TRUE, &u4QueryInfoLen);
#else
	rStatus = wlanQueryInformation(prGlueInfo->prAdapter,
				       wlanoidQueryFrequency,
				       &prQueryConfig->u4DSConfig, sizeof(UINT_32), &u4QueryInfoLen);
#endif
	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;

	prQueryConfig->rFHConfig.u4Length = sizeof(PARAM_802_11_CONFIG_FH_T);

	return rStatus;

}				/* end of reqExtQueryConfiguration() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to set the radio configuration used in IBSS
*        mode.
*
* \param[in] prGlueInfo     Pointer to the GLUE_INFO_T structure.
* \param[in] pvSetBuffer    A pointer to the buffer that holds the data to be set.
* \param[in] u4SetBufferLen The length of the set buffer.
* \param[out] pu4SetInfoLen If the call is successful, returns the number of
*                           bytes read from the set buffer. If the call failed
*                           due to invalid length of the set buffer, returns
*                           the amount of storage needed.
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_LENGTH
* \retval WLAN_STATUS_NOT_ACCEPTED
*/
/*----------------------------------------------------------------------------*/
static WLAN_STATUS
reqExtSetConfiguration(IN P_GLUE_INFO_T prGlueInfo,
		       IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen)
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_PARAM_802_11_CONFIG_T prNewConfig = (P_PARAM_802_11_CONFIG_T) pvSetBuffer;
	UINT_32 u4SetInfoLen = 0;

	DEBUGFUNC("wlanoidSetConfiguration");

	ASSERT(prGlueInfo);
	ASSERT(pu4SetInfoLen);

	*pu4SetInfoLen = sizeof(PARAM_802_11_CONFIG_T);

	if (u4SetBufferLen < *pu4SetInfoLen)
		return WLAN_STATUS_INVALID_LENGTH;

	/* OID_802_11_CONFIGURATION. If associated, NOT_ACCEPTED shall be returned. */
	if (prGlueInfo->eParamMediaStateIndicated == PARAM_MEDIA_STATE_CONNECTED)
		return WLAN_STATUS_NOT_ACCEPTED;

	ASSERT(pvSetBuffer);

#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidSetBeaconInterval,
			       &prNewConfig->u4BeaconPeriod, sizeof(UINT_32), FALSE, TRUE, &u4SetInfoLen);
#else
	rStatus = wlanSetInformation(prGlueInfo->prAdapter,
				     wlanoidSetBeaconInterval,
				     &prNewConfig->u4BeaconPeriod, sizeof(UINT_32), &u4SetInfoLen);
#endif
	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;
#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidSetAtimWindow,
			       &prNewConfig->u4ATIMWindow, sizeof(UINT_32), FALSE, TRUE, &u4SetInfoLen);
#else
	rStatus = wlanSetInformation(prGlueInfo->prAdapter,
				     wlanoidSetAtimWindow, &prNewConfig->u4ATIMWindow, sizeof(UINT_32), &u4SetInfoLen);
#endif
	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;
#if defined(_HIF_SDIO)
	rStatus = sdio_io_ctrl(prGlueInfo,
			       wlanoidSetFrequency,
			       &prNewConfig->u4DSConfig, sizeof(UINT_32), FALSE, TRUE, &u4SetInfoLen);
#else
	rStatus = wlanSetInformation(prGlueInfo->prAdapter,
				     wlanoidSetFrequency, &prNewConfig->u4DSConfig, sizeof(UINT_32), &u4SetInfoLen);
#endif

	if (rStatus != WLAN_STATUS_SUCCESS)
		return rStatus;

	return rStatus;

}				/* end of reqExtSetConfiguration() */
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to set beacon detection function enable/disable state
*        This is mainly designed for usage under BT inquiry state (disable function).
*
* \param[in] pvAdapter Pointer to the Adapter structure
* \param[in] pvSetBuffer A pointer to the buffer that holds the data to be set
* \param[in] u4SetBufferLen The length of the set buffer
* \param[out] pu4SetInfoLen If the call is successful, returns the number of
*   bytes read from the set buffer. If the call failed due to invalid length of
*   the set buffer, returns the amount of storage needed.
*
* \retval WLAN_STATUS_SUCCESS
* \retval WLAN_STATUS_INVALID_DATA If new setting value is wrong.
* \retval WLAN_STATUS_INVALID_LENGTH
*
*/
/*----------------------------------------------------------------------------*/
static WLAN_STATUS
reqExtSetAcpiDevicePowerState(IN P_GLUE_INFO_T prGlueInfo,
			      IN PVOID pvSetBuffer, IN UINT_32 u4SetBufferLen, OUT PUINT_32 pu4SetInfoLen)
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prGlueInfo);
	ASSERT(pvSetBuffer);
	ASSERT(pu4SetInfoLen);

	/* WIFI is enabled, when ACPI is D0 (ParamDeviceStateD0 = 1). And vice versa */

	/* rStatus = wlanSetInformation(prGlueInfo->prAdapter, */
	/* wlanoidSetAcpiDevicePowerState, */
	/* pvSetBuffer, */
	/* u4SetBufferLen, */
	/* pu4SetInfoLen); */
	return rStatus;
}

#define CMD_START				"START"
#define CMD_STOP				"STOP"
#define CMD_SCAN_ACTIVE			"SCAN-ACTIVE"
#define CMD_SCAN_PASSIVE		"SCAN-PASSIVE"
#define CMD_RSSI				"RSSI"
#define CMD_LINKSPEED			"LINKSPEED"
#define CMD_RXFILTER_START		"RXFILTER-START"
#define CMD_RXFILTER_STOP		"RXFILTER-STOP"
#define CMD_RXFILTER_ADD		"RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE		"RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP		"BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE			"BTCOEXMODE"
#define CMD_SETSUSPENDOPT		"SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE		"SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR		"P2P_DEV_ADDR"
#define CMD_SETFWPATH			"SETFWPATH"
#define CMD_SETBAND				"SETBAND"
#define CMD_GETBAND				"GETBAND"
#define CMD_AP_START			"AP_START"

#if CFG_SUPPORT_QA_TOOL
#define CMD_GET_RX_STATISTICS	"GET_RX_STATISTICS"
#endif
#define CMD_GET_STAT		"GET_STAT"
#define CMD_GET_BSS_STATISTICS	"GET_BSS_STATISTICS"
#define CMD_GET_STA_STATISTICS	"GET_STA_STATISTICS"
#define CMD_GET_WTBL_INFO	"GET_WTBL"
#define CMD_GET_MIB_INFO	"GET_MIB"
#define CMD_GET_STA_INFO	"GET_STA"
#define CMD_SET_FW_LOG		"SET_FWLOG"
#define CMD_GET_QUE_INFO	"GET_QUE"
#define CMD_GET_MEM_INFO	"GET_MEM"
#define CMD_GET_HIF_INFO	"GET_HIF"
#define CMD_GET_TP_INFO	"GET_TP"
#define CMD_GET_STA_KEEP_CNT    "KEEPCOUNTER"
#define CMD_STAT_RESET_CNT      "RESETCOUNTER"

#define CMD_SET_TXPOWER			"SET_TXPOWER"
#define CMD_COUNTRY				"COUNTRY"
#define CMD_P2P_SET_NOA			"P2P_SET_NOA"
#define CMD_P2P_GET_NOA			"P2P_GET_NOA"
#define CMD_P2P_SET_PS			"P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE	"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE	"SETROAMMODE"
#define CMD_MIRACAST		"MIRACAST"

#define CMD_PNOSSIDCLR_SET	"PNOSSIDCLR"
#define CMD_PNOSETUP_SET	"PNOSETUP "
#define CMD_PNOENABLE_SET	"PNOFORCE"
#define CMD_PNODEBUG_SET	"PNODEBUG"
#define CMD_WLS_BATCHING	"WLS_BATCHING"

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"

#define CMD_SETMONITOR		"MONITOR"
#define CMD_SETBUFMODE		"BUFFER_MODE"

#define CMD_USB_INFO		"USB_INFO"

/* miracast related definition */
#define MIRACAST_MODE_OFF	0
#define MIRACAST_MODE_SOURCE	1
#define MIRACAST_MODE_SINK	2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE	8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif

#define	CMD_BAND_AUTO	0
#define	CMD_BAND_5G		1
#define	CMD_BAND_2G		2
#define	CMD_BAND_ALL	3

/* Mediatek private command */
#define CMD_SET_MCR				"SET_MCR"
#define CMD_GET_MCR	            "GET_MCR"
#define CMD_SET_DRV_MCR			"SET_DRV_MCR"
#define CMD_GET_DRV_MCR			"GET_DRV_MCR"
#define CMD_SET_SW_CTRL	        "SET_SW_CTRL"
#define CMD_GET_SW_CTRL         "GET_SW_CTRL"
#define CMD_SET_CFG             "SET_CFG"
#define CMD_GET_CFG             "GET_CFG"
#define CMD_SET_CHIP            "SET_CHIP"
#define CMD_GET_CHIP            "GET_CHIP"
#define CMD_SET_DBG_LEVEL       "SET_DBG_LEVEL"
#define CMD_GET_DBG_LEVEL       "GET_DBG_LEVEL"
#define PRIV_CMD_SIZE 512
#define CMD_SET_FIXED_RATE      "FixedRate"
#define CMD_GET_VERSION         "VER"
#define CMD_SET_TEST_MODE		"SET_TEST_MODE"
#define CMD_SET_TEST_CMD		"SET_TEST_CMD"
#define CMD_GET_TEST_RESULT		"GET_TEST_RESULT"
#define CMD_GET_STA_STAT        "STAT"

#if CFG_WOW_SUPPORT
#define CMD_SET_WOW_ENABLE		"SET_WOW_ENABLE"
#define CMD_SET_WOW_PAR			"SET_WOW_PAR"
#endif

#define CMD_GET_CNM				"GET_CNM"
#define CMD_SET_DBDC			"SET_DBDC"

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
#define CMD_SET_CALBACKUP_TEST_DRV_FW		"SET_CALBACKUP_TEST_DRV_FW"
#endif

static UINT_8 g_ucMiracastMode = MIRACAST_MODE_OFF;

typedef struct cmd_tlv {
	char prefix;
	char version;
	char subver;
	char reserved;
} cmd_tlv_t;

typedef struct priv_driver_cmd_s {
	char buf[PRIV_CMD_SIZE];
	int used_len;
	int total_len;
} priv_driver_cmd_t;


int priv_driver_get_dbg_level(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4DbgIdx, u4DbgMask;
	BOOLEAN fgIsCmdAccept = FALSE;
	INT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* u4DbgIdx = kalStrtoul(apcArgv[1], NULL, 0); */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4DbgIdx);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);

		if (wlanGetDebugLevel(u4DbgIdx, &u4DbgMask) == WLAN_STATUS_SUCCESS) {
			fgIsCmdAccept = TRUE;
			i4BytesWritten =
			    snprintf(pcCommand, i4TotalLen,
				     "Get DBG module[%lu] log level => [0x%02x]!", u4DbgIdx, (UINT_8) u4DbgMask);
		}
	}

	if (!fgIsCmdAccept)
		i4BytesWritten = snprintf(pcCommand, i4TotalLen, "Get DBG module log level failed!");

	return i4BytesWritten;

}				/* priv_driver_get_sw_ctrl */


#if CFG_SUPPORT_QA_TOOL
#if CFG_SUPPORT_BUFFER_MODE
static int priv_driver_set_efuse_buffer_mode(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4Argc = 0;
	INT_32 i4BytesWritten = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	PARAM_CUSTOM_EFUSE_BUFFER_MODE_T *prSetEfuseBufModeInfo;
#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 0)
	BIN_CONTENT_T *pBinContent;
	int i = 0;
#endif
	PUINT_8 pucConfigBuf;
	UINT_32 u4ConfigReadLen;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	pucConfigBuf = (PUINT_8) kalMemAlloc(2048, VIR_MEM_TYPE);
	kalMemZero(pucConfigBuf, 2048);
	u4ConfigReadLen = 0;

	if (pucConfigBuf) {
		if (kalReadToFile("/MT6632_eFuse_usage_table.xlsm.bin", pucConfigBuf, 2048, &u4ConfigReadLen) == 0) {
			/* ToDo:: Nothing */
		} else {
			DBGLOG(INIT, INFO, "can't find file\n");
			return -1;
		}

		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, 2048);
	}
	/* pucConfigBuf */
	prSetEfuseBufModeInfo =
	(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T *) kalMemAlloc(sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T), VIR_MEM_TYPE);
	if (prSetEfuseBufModeInfo == NULL)
		return WLAN_STATUS_FAILURE;
	kalMemZero(prSetEfuseBufModeInfo, sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T));

	prSetEfuseBufModeInfo->ucSourceMode = 1;
	prSetEfuseBufModeInfo->ucCount = (UINT_8)EFUSE_CONTENT_SIZE;

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 0)
	pBinContent = (BIN_CONTENT_T *)prSetEfuseBufModeInfo->aBinContent;
	for (i = 0; i < EFUSE_CONTENT_SIZE; i++) {
		pBinContent->u2Addr  = i;
		pBinContent->ucValue = *(pucConfigBuf + i);

		pBinContent++;
	}

	for (i = 0; i < 20; i++)
		DBGLOG(INIT, INFO, "%x\n", prSetEfuseBufModeInfo->aBinContent[i].ucValue);
#endif

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetEfusBufferMode,
			   prSetEfuseBufModeInfo, sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T), FALSE, FALSE, TRUE,
			   &u4BufLen);
	kalMemFree(prSetEfuseBufModeInfo, VIR_MEM_TYPE, sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T));

	i4BytesWritten =
		snprintf(pcCommand, i4TotalLen, "set buffer mode %s",
			 (rStatus == WLAN_STATUS_SUCCESS) ? "success" : "fail");

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_BUFFER_MODE */

static int priv_driver_get_rx_statistics(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	INT_32 u4Ret = 0;
	PARAM_CUSTOM_ACCESS_RX_STAT rRxStatisticsTest;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(INIT, ERROR, "MT6632 : priv_driver_get_rx_statistics\n");

	if (i4Argc >= 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRxStatisticsTest.u4SeqNum));
		rRxStatisticsTest.u4TotalNum = sizeof(PARAM_RX_STAT_T) / 4;

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryRxStatistics,
				   &rRxStatisticsTest, sizeof(rRxStatisticsTest), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;
}
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_MSP
#if 0
static int priv_driver_get_stat(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	INT_32 i4ArgNum = 2;
	PARAM_GET_STA_STA_STATISTICS rQueryStaStatistics;
	PARAM_RSSI rRssi;
	UINT_16 u2LinkSpeed;
	UINT_32 u4Per;
	UINTT_8 i;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));

	if (i4Argc >= i4ArgNum) {
		wlanHwAddrToBin(apcArgv[1], &rQueryStaStatistics.aucMacAddr[0]);

		rQueryStaStatistics.ucReadClear = TRUE;

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryStaStatistics,
				   &rQueryStaStatistics,
				   sizeof(rQueryStaStatistics), TRUE, FALSE, TRUE, &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
			u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ? 0 : rQueryStaStatistics.u2LinkSpeed/2;

			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nSTA Stat:\n");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"CurrentTemperature            = %d\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx success                    = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx fail count                 = %ld, PER=%ld.%1ld%%\n", 0, 0, 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx success                    = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx with CRC                   = %ld, PER=%ld.%1ld%%\n", 0, 0, 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx with PhyErr                = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx with PlcpErr               = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx drop due to out of resource= %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx duplicate frame            = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"False CCA                     = %lu\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"RSSI                          = %d %d %d %d\n", 0, 0, 0, 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Last TX Rate	       = %s, %s, %s, %s, %s\n", "NA", "NA", "NA", "NA", "NA");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Last RX Rate	       = %s, %s, %s, %s, %s\n", "NA", "NA", "NA", "NA", "NA");

			for (i = 0; i < 2 /* band num */; i++) {
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"BandIdx:	       = %d\n", i);
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s", "\tRange:  1   2~5   6~15   16~22   23~33   34~49   50~57   58~64\n");
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
					0, 0, 0, 0, 0, 0, 0, 0);
			}
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx success	       = %ld\n",
				rQueryStaStatistics.u4TransmitCount - rQueryStaStatistics.u4TransmitFailCount);

			u4Per = rQueryStaStatistics.u4TransmitFailCount == 0 ? 0 :
				(1000 * (rQueryStaStatistics.u4TransmitFailCount)) /
				rQueryStaStatistics.u4TransmitCount;
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx fail count	       = %ld, PER=%ld.%1ld%%\n",
				rQueryStaStatistics.u4TransmitFailCount, u4Per/10, u4Per%10);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"RSSI		       = %d\n", rRssi);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"LinkSpeed	       = %d\n", u2LinkSpeed);
		}
	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nNo STA Stat:\n");

	return i4BytesWritten;
}
#endif


static int priv_driver_get_sta_statistics(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	INT_32 i4ArgNum = 3;
	PARAM_GET_STA_STA_STATISTICS rQueryStaStatistics;
	PARAM_RSSI rRssi;
	UINT_16 u2LinkSpeed;
	UINT_32 u4Per;

	ASSERT(prNetDev);

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
	rQueryStaStatistics.ucReadClear = TRUE;

	if (i4Argc >= i4ArgNum) {
		if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT,
			strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[2],
			&rQueryStaStatistics.aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		} else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT,
			strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[1],
			&rQueryStaStatistics.aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		}
	} else {
		/* Get AIS AP address for no argument */
		if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP) {
			COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr,
				prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP->aucMacAddr);
			DBGLOG(RSN, INFO, "use ais ap "MACSTR"\n",
				MAC2STR(prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP->aucMacAddr));
		} else {
			DBGLOG(RSN, INFO, "not connect to ais ap %x\n",
				prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP);
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nNo STA Stat:\n");
			return i4BytesWritten;
		}

		if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT, strlen(CMD_GET_STA_KEEP_CNT)) == 0)
				rQueryStaStatistics.ucReadClear = FALSE;
		}
	}

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryStaStatistics,
			   &rQueryStaStatistics,
			   sizeof(rQueryStaStatistics), TRUE, FALSE, TRUE, &u4BufLen);

	if (rStatus == WLAN_STATUS_SUCCESS) {
		rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
		u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ? 0 : rQueryStaStatistics.u2LinkSpeed/2;

		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nSTA Stat:\n");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"Tx total cnt           = %ld\n",
			rQueryStaStatistics.u4TransmitCount);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"Tx success	       = %ld\n",
			rQueryStaStatistics.u4TransmitCount - rQueryStaStatistics.u4TransmitFailCount);

		u4Per = rQueryStaStatistics.u4TransmitCount == 0 ? 0 :
			(1000 * (rQueryStaStatistics.u4TransmitFailCount)) /
			rQueryStaStatistics.u4TransmitCount;
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"Tx fail count	       = %ld, PER=%ld.%1ld%%\n",
			rQueryStaStatistics.u4TransmitFailCount, u4Per/10, u4Per%10);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"RSSI		       = %d\n", rRssi);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"LinkSpeed	       = %d\n", u2LinkSpeed);

	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nNo STA Stat:\n");

	return i4BytesWritten;

}


static int priv_driver_get_bss_statistics(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus;
	PARAM_MAC_ADDRESS arBssid;
	UINT_32 u4BufLen;
	INT_32 i4Rssi;
	PARAM_GET_BSS_STATISTICS rQueryBssStatistics;
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;
	UINT_8 ucBssIndex;
	INT_32 i4BytesWritten = 0;
#if 0
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	INT_32 i4Argc = 0;
	UINT_32	u4Index;
#endif

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	ASSERT(prGlueInfo);

	kalMemZero(arBssid, MAC_ADDR_LEN);
	wlanQueryInformation(prGlueInfo->prAdapter, wlanoidQueryBssid, &arBssid[0], sizeof(arBssid), &u4BufLen);

#if 0 /* Todo:: Get the none-AIS statistics */
	if (i4Argc >= 2)
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Index);
#endif

	/* 2. fill RSSI */
	if (prGlueInfo->eParamMediaStateIndicated != PARAM_MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(REQ, WARN, "not yet connected\n");
		return WLAN_STATUS_SUCCESS;
	}
	rStatus = kalIoctl(prGlueInfo, wlanoidQueryRssi, &i4Rssi,
		sizeof(i4Rssi), TRUE, FALSE, FALSE, &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "unable to retrieve rssi\n");


	/* 3 get per-BSS link statistics */
	if (rStatus == WLAN_STATUS_SUCCESS) {
		/* get Bss Index from ndev */
		prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prNetDev);
		ASSERT(prNetDevPrivate->prGlueInfo == prGlueInfo);
		ucBssIndex = prNetDevPrivate->ucBssIdx;

		kalMemZero(&rQueryBssStatistics, sizeof(rQueryBssStatistics));
		rQueryBssStatistics.ucBssIndex = ucBssIndex;

		rQueryBssStatistics.ucReadClear = TRUE;

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryBssStatistics,
				   &rQueryBssStatistics,
				   sizeof(rQueryBssStatistics), TRUE, FALSE, TRUE, &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nStat:\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "CurrentTemperature    = -\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx success	       = %ld\n",
				rQueryBssStatistics.u4TransmitCount - rQueryBssStatistics.u4TransmitFailCount);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx fail count	       = %ld\n", rQueryBssStatistics.u4TransmitFailCount);
#if 0
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx success	       = %ld\n", 0);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx with CRC	       = %ld\n", prStatistics->rFCSErrorCount.QuadPart);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "Rx with PhyErr	     = 0\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "Rx with PlcpErr	     = 0\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "Rx drop due to out of resource	= 0\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rx duplicate frame    = %ld\n", prStatistics->rFrameDuplicateCount.QuadPart);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "False CCA	     = 0\n");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"RSSI		       = %d\n", i4Rssi);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Last TX Rate	       = %s, %s, %s, %s, %s\n", "NA", "NA", "NA", "NA", "NA");
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Last RX Rate	       = %s, %s, %s, %s, %s\n", "NA", "NA", "NA", "NA", "NA");
#endif

		}

	} else {
		DBGLOG(REQ, WARN, "unable to retrieve per-BSS link statistics\n");
	}


	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	return i4BytesWritten;

}

#define HW_TX_RATE_TO_MODE(_x)			(((_x) & (0x7 << 6)) >> 6)
#define HW_TX_RATE_TO_MCS(_x, _mode)		((_x) & (0x3f))
#define HW_TX_RATE_TO_NSS(_x)			(((_x) & (0x3 << 9)) >> 9)
#define HW_TX_RATE_TO_STBC(_x)			(((_x) & (0x1 << 11)) >> 11)

#define TX_VECTOR_GET_TX_RATE(_txv)     (((_txv)->u4TxVector1) & BITS(0, 6))
#define TX_VECTOR_GET_TX_LDPC(_txv)     ((((_txv)->u4TxVector1) >> 7) & BIT(0))
#define TX_VECTOR_GET_TX_STBC(_txv)     ((((_txv)->u4TxVector1) >> 8) & BITS(0, 1))
#define TX_VECTOR_GET_TX_FRMODE(_txv)   ((((_txv)->u4TxVector1) >> 10) & BITS(0, 1))
#define TX_VECTOR_GET_TX_MODE(_txv)     ((((_txv)->u4TxVector1) >> 12) & BITS(0, 2))
#define TX_VECTOR_GET_TX_NSTS(_txv)     ((((_txv)->u4TxVector1) >> 21) & BITS(0, 1))
#define TX_VECTOR_GET_TX_PWR(_txv)      ((((_txv)->u4TxVector1) >> 24) & BITS(0, 6))
#define TX_VECTOR_GET_BF_EN(_txv)       ((((_txv)->u4TxVector2) >> 31) & BIT(0))
#define TX_VECTOR_GET_DYN_BW(_txv)      ((((_txv)->u4TxVector4) >> 31) & BIT(0))
#define TX_VECTOR_GET_NO_SOUNDING(_txv) ((((_txv)->u4TxVector4) >> 28) & BIT(0))
#define TX_VECTOR_GET_TX_SGI(_txv)      ((((_txv)->u4TxVector4) >> 27) & BIT(0))

#define TX_RATE_MODE_CCK	0
#define TX_RATE_MODE_OFDM	1
#define TX_RATE_MODE_HTMIX	2
#define TX_RATE_MODE_HTGF	3
#define TX_RATE_MODE_VHT	4
#define MAX_TX_MODE 5

static char *HW_TX_MODE_STR[] = {"CCK", "OFDM", "MM", "GF", "VHT", "N/A"};
static char *HW_TX_RATE_CCK_STR[] = {"1M", "2M", "5.5M", "11M", "N/A"};
static char *HW_TX_RATE_OFDM_STR[] = {"6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M", "N/A"};
static char *HW_TX_RATE_BW[] = {"BW20", "BW40", "BW80", "BW160/BW8080", "N/A"};

static char *RATE_TBLE[] = {"B", "G", "N", "N_2SS", "AC", "AC_2SS", "N/A"};
#if 0
static char *AR_STATE[] = {"NULL", "STEADY", "PROBE", "N/A"};
static char *AR_ACTION[] = {"NULL", "INDEX", "RATE_UP", "RATE_DOWN", "RATE_GRP", "RATE_BACK",
							"GI", "SGI_EN", "SGI_DIS", "PWR", "PWR_UP", "PWR_DOWN",
							"PWR_RESET_UP", "BF", "BF_EN", "BF_DIS", "N/A"};
#endif
#define BW_20		0
#define BW_40		1
#define BW_80		2
#define BW_160		3
#define BW_10		4
#define BW_5		6
#define BW_8080		7
#define BW_ALL		0xFF

static char *hw_rate_ofdm_str(UINT_16 ofdm_idx)
{
	switch (ofdm_idx) {
	case 11: /* 6M */
		return HW_TX_RATE_OFDM_STR[0];
	case 15: /* 9M */
		return HW_TX_RATE_OFDM_STR[1];
	case 10: /* 12M */
		return HW_TX_RATE_OFDM_STR[2];
	case 14: /* 18M */
		return HW_TX_RATE_OFDM_STR[3];
	case 9: /* 24M */
		return HW_TX_RATE_OFDM_STR[4];
	case 13: /* 36M */
		return HW_TX_RATE_OFDM_STR[5];
	case 8: /* 48M */
		return HW_TX_RATE_OFDM_STR[6];
	case 12: /* 54M */
		return HW_TX_RATE_OFDM_STR[7];
	default:
		return HW_TX_RATE_OFDM_STR[8];
	}
}

static BOOL priv_driver_get_sgi_info(IN P_PARAM_PEER_CAP_T prWtblPeerCap)
{
	if (!prWtblPeerCap)
		return FALSE;

	switch (prWtblPeerCap->ucFrequencyCapability) {
	case BW_20:
		return prWtblPeerCap->fgG2;
	case BW_40:
		return prWtblPeerCap->fgG4;
	case BW_80:
		return prWtblPeerCap->fgG8;
	case BW_160:
		return prWtblPeerCap->fgG16;
	default:
		return FALSE;
	}
}

static BOOL priv_driver_get_ldpc_info(IN P_PARAM_TX_CONFIG_T prWtblTxConfig)
{
	if (!prWtblTxConfig)
		return FALSE;

	if (prWtblTxConfig->fgIsVHT)
		return prWtblTxConfig->fgVhtLDPC;
	else
		return prWtblTxConfig->fgLDPC;
}

INT_32 priv_driver_rate_to_string(IN char *pcCommand, IN int i4TotalLen, UINT_8 TxRx,
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo)
{
	UINT_8 i, txmode, rate, stbc;
	UINT_8 nss;
	INT_32 i4BytesWritten = 0;

	for (i = 0; i < AUTO_RATE_NUM; i++) {

		txmode = HW_TX_RATE_TO_MODE(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
		if (txmode >= MAX_TX_MODE)
			txmode = MAX_TX_MODE;
		rate = HW_TX_RATE_TO_MCS(prHwWlanInfo->rWtblRateInfo.au2RateCode[i], txmode);
		nss = HW_TX_RATE_TO_NSS(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) + 1;
		stbc = HW_TX_RATE_TO_STBC(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRate index[%d] ", i);

		if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
			if (TxRx == 0)
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s", "[Last RX Rate] ");
			else
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s", "[Last TX Rate] ");
		} else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "               ");

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? HW_TX_RATE_CCK_STR[rate] : HW_TX_RATE_CCK_STR[4]);
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", hw_rate_ofdm_str(rate));
		else {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"NSS%d_MCS%d, ", nss, rate);
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, ", HW_TX_RATE_BW[prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", priv_driver_get_sgi_info(&prHwWlanInfo->rWtblPeerCap) == 0 ? "LGI" : "SGI");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s%s %s\n", HW_TX_MODE_STR[txmode],
			stbc ? "STBC" : " ",
			priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig) == 0 ? "BCC" : "LDPC");
	}

	return i4BytesWritten;
}

static INT_32 priv_driver_dump_helper_wtbl_info(IN char *pcCommand, IN int i4TotalLen,
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo)
{
	UINT_8 i;
	INT_32 i4BytesWritten = 0;

	ASSERT(pcCommand);

	i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nwtbl:\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"Dump WTBL info of WLAN_IDX	    = %d\n", prHwWlanInfo->u4Index);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tAddr="MACSTR"\n", MAC2STR(prHwWlanInfo->rWtblTxConfig.aucPA));
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tMUAR_Idx	 = %d\n", prHwWlanInfo->rWtblSecConfig.ucMUARIdx);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\trc_a1/rc_a2:%d/%d\n", prHwWlanInfo->rWtblSecConfig.fgRCA1,  prHwWlanInfo->rWtblSecConfig.fgRCA2);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tKID:%d/RCID:%d/RKV:%d/RV:%d/IKV:%d/WPI_FLAG:%d\n", prHwWlanInfo->rWtblSecConfig.ucKeyID,
		prHwWlanInfo->rWtblSecConfig.fgRCID, prHwWlanInfo->rWtblSecConfig.fgRKV,
		prHwWlanInfo->rWtblSecConfig.fgRV, prHwWlanInfo->rWtblSecConfig.fgIKV,
		prHwWlanInfo->rWtblSecConfig.fgEvenPN);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "\tGID_SU:NA");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tsw/DIS_RHTR:%d/%d\n", prHwWlanInfo->rWtblTxConfig.fgSW,  prHwWlanInfo->rWtblTxConfig.fgDisRxHdrTran);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tHT/VHT/HT-LDPC/VHT-LDPC/DYN_BW/MMSS:%d/%d/%d/%d/%d/%d\n",
		prHwWlanInfo->rWtblTxConfig.fgIsHT,  prHwWlanInfo->rWtblTxConfig.fgIsVHT,
		prHwWlanInfo->rWtblTxConfig.fgLDPC, prHwWlanInfo->rWtblTxConfig.fgVhtLDPC,
		prHwWlanInfo->rWtblTxConfig.fgDynBw, prHwWlanInfo->rWtblPeerCap.ucMMSS);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tFCAP/G2/G4/G8/G16/CBRN:%d/%d/%d/%d/%d/%d\n",
		prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability, prHwWlanInfo->rWtblPeerCap.fgG2,
		prHwWlanInfo->rWtblPeerCap.fgG4, prHwWlanInfo->rWtblPeerCap.fgG8, prHwWlanInfo->rWtblPeerCap.fgG16,
		prHwWlanInfo->rWtblPeerCap.ucChangeBWAfterRateN);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tHT-TxBF(tibf/tebf):%d/%d, VHT-TxBF(tibf/tebf):%d/%d, PFMU_IDX=%d\n",
		prHwWlanInfo->rWtblTxConfig.fgTIBF, prHwWlanInfo->rWtblTxConfig.fgTEBF,
		prHwWlanInfo->rWtblTxConfig.fgVhtTIBF, prHwWlanInfo->rWtblTxConfig.fgVhtTEBF,
		prHwWlanInfo->rWtblTxConfig.ucPFMUIdx);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "\tSPE_IDX=NA\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tBA Enable:0x%x, BAFail Enable:%d\n", prHwWlanInfo->rWtblBaConfig.ucBaEn,
		prHwWlanInfo->rWtblTxConfig.fgBAFEn);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tQoS Enable:%d\n", prHwWlanInfo->rWtblTxConfig.fgIsQoS);
	if (prHwWlanInfo->rWtblTxConfig.fgIsQoS) {
		for (i = 0; i < 8; i += 2) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\t\tBA WinSize: TID 0 - %d, TID 1 - %d\n",
				(prHwWlanInfo->rWtblBaConfig.u4BaWinSize >> (i * 3)) & BITS(0, 2),
				(prHwWlanInfo->rWtblBaConfig.u4BaWinSize >> ((i + 1) * 3)) & BITS(0, 2));
		}
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tpartial_aid:%d\n", prHwWlanInfo->rWtblTxConfig.u2PartialAID);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\twpi_even:%d\n", prHwWlanInfo->rWtblSecConfig.fgEvenPN);
	i4BytesWritten += scnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tAAD_OM/CipherSuit:%d/%d\n", prHwWlanInfo->rWtblTxConfig.fgAADOM,
		prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\taf:%d\n", prHwWlanInfo->rWtblPeerCap.ucAmpduFactor);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\trdg_ba:%d/rdg capability:%d\n", prHwWlanInfo->rWtblTxConfig.fgRdgBA,
		prHwWlanInfo->rWtblTxConfig.fgRDG);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tcipher_suit:%d\n", prHwWlanInfo->rWtblSecConfig.ucCipherSuit);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tFromDS:%d\n", prHwWlanInfo->rWtblTxConfig.fgIsFromDS);
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tToDS:%d\n", prHwWlanInfo->rWtblTxConfig.fgIsToDS);
#if 0
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tRCPI = %d %d %d %d\n", prHwWlanInfo->rWtblRxCounter.ucRxRcpi0,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi1,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi2,
		prHwWlanInfo->rWtblRxCounter.ucRxRcpi3);
#endif
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"\tRSSI = %d %d %d %d\n", RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "\tRate Info\n");

	i4BytesWritten += priv_driver_rate_to_string(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten, 1,
		prHwWlanInfo);

#if 0
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "\t===Key======\n");
	for (i = 0; i < 32; i += 8) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\t0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 0], prHwWlanInfo->rWtblKeyConfig.aucKey[i + 1],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 2], prHwWlanInfo->rWtblKeyConfig.aucKey[i + 3],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 4], prHwWlanInfo->rWtblKeyConfig.aucKey[i + 5],
			prHwWlanInfo->rWtblKeyConfig.aucKey[i + 6], prHwWlanInfo->rWtblKeyConfig.aucKey[i + 7]);
	}
#endif

	return i4BytesWritten;
}

static int priv_driver_get_wtbl_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	INT_32 u4Ret = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	/* DBGLOG(RSN, INFO, "MT6632 : priv_driver_get_wtbl_info\n"); */

	prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
	if (!prHwWlanInfo)
		return -1;

	if (i4Argc >= 2) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &prHwWlanInfo->u4Index);

		DBGLOG(REQ, INFO, "MT6632 : index = %d\n", prHwWlanInfo->u4Index);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryWlanInfo,
				   prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, INFO, "rStatus %u u4BufLen = %d\n", rStatus, u4BufLen);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));
			return -1;
		}
		i4BytesWritten = priv_driver_dump_helper_wtbl_info(pcCommand, i4TotalLen, prHwWlanInfo);
	}

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));

	return i4BytesWritten;
}

static int priv_driver_get_sta_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_8 aucMacAddr[MAC_ADDR_LEN];
	UINT_8 ucWlanIndex;
	PUINT_8 pucMacAddr = NULL;
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo;
	PARAM_GET_STA_STA_STATISTICS rQueryStaStatistics;
	PARAM_RSSI rRssi;
	UINT_16 u2LinkSpeed;
	UINT_32 u4Per;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	kalMemZero(&rQueryStaStatistics, sizeof(rQueryStaStatistics));
	rQueryStaStatistics.ucReadClear = TRUE;

	/* DBGLOG(RSN, INFO, "MT6632 : priv_driver_get_sta_info\n"); */
	if (i4Argc >= 3) {
		if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT, strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		} else if (strnicmp(apcArgv[2], CMD_GET_STA_KEEP_CNT, strlen(CMD_GET_STA_KEEP_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
			rQueryStaStatistics.ucReadClear = FALSE;
		}

		if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, &aucMacAddr[0], &ucWlanIndex))
			return i4BytesWritten;
	} else {
		/* Get AIS AP address for no argument */
		if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP)
			ucWlanIndex = prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP->ucWlanIndex;
		else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL, &ucWlanIndex)) /* try get a peer */
			return i4BytesWritten;

		if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_GET_STA_KEEP_CNT, strlen(CMD_GET_STA_KEEP_CNT)) == 0)
				rQueryStaStatistics.ucReadClear = FALSE;
		}
	}

	prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
	prHwWlanInfo->u4Index = ucWlanIndex;

	DBGLOG(REQ, INFO, "MT6632 : index = %d i4TotalLen = %d\n", prHwWlanInfo->u4Index, i4TotalLen);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryWlanInfo,
			   prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T), TRUE, TRUE, TRUE, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));
		return -1;
	}

	i4BytesWritten = priv_driver_dump_helper_wtbl_info(pcCommand, i4TotalLen, prHwWlanInfo);

	pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex);
	if (pucMacAddr) {
		COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, pucMacAddr);
		/* i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		*	"\tAddr="MACSTR"\n", MAC2STR(rQueryStaStatistics.aucMacAddr));
		*/

		rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryStaStatistics,
			   &rQueryStaStatistics,
			   sizeof(rQueryStaStatistics), TRUE, FALSE, TRUE, &u4BufLen);

		if (rStatus == WLAN_STATUS_SUCCESS) {
			rRssi = RCPI_TO_dBm(rQueryStaStatistics.ucRcpi);
			u2LinkSpeed = rQueryStaStatistics.u2LinkSpeed == 0 ? 0 : rQueryStaStatistics.u2LinkSpeed/2;

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s", "\n\nSTA Stat:\n");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx total cnt           = %ld\n",
				rQueryStaStatistics.u4TransmitCount);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx success	       = %ld\n",
				rQueryStaStatistics.u4TransmitCount - rQueryStaStatistics.u4TransmitFailCount);

			u4Per = rQueryStaStatistics.u4TransmitCount == 0 ? 0 :
				(1000 * (rQueryStaStatistics.u4TransmitFailCount)) /
				rQueryStaStatistics.u4TransmitCount;
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Tx fail count	       = %ld, PER=%ld.%1ld%%\n",
				rQueryStaStatistics.u4TransmitFailCount, u4Per/10, u4Per%10);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"RSSI		       = %d\n", rRssi);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"LinkSpeed	       = %d\n", u2LinkSpeed);
		}
	}
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));

	return i4BytesWritten;
}


static int priv_driver_get_mib_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	UINT_8 i;
	UINT_32 u4Per;
	INT_32 u4Ret = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	P_PARAM_HW_MIB_INFO_T prHwMibInfo;
	P_RX_CTRL_T prRxCtrl;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	prRxCtrl = &prGlueInfo->prAdapter->rRxCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(REQ, INFO, "MT6632 : priv_driver_get_mib_info\n");

	prHwMibInfo = (P_PARAM_HW_MIB_INFO_T)kalMemAlloc(sizeof(PARAM_HW_MIB_INFO_T), VIR_MEM_TYPE);
	if (!prHwMibInfo)
		return -1;

	if (i4Argc == 1)
		prHwMibInfo->u4Index = 0;

	if (i4Argc >= 2)
		u4Ret = kalkStrtou32(apcArgv[1], 0, &prHwMibInfo->u4Index);

	DBGLOG(REQ, INFO, "MT6632 : index = %d\n", prHwMibInfo->u4Index);

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryMibInfo,
			prHwMibInfo, sizeof(PARAM_HW_MIB_INFO_T), TRUE, TRUE, TRUE, &u4BufLen);

	DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwMibInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_MIB_INFO_T));
		return -1;
	}

	if (prHwMibInfo->u4Index < 2) {
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\n\nmib state:\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"Dump MIB info of IDX         = %d\n", prHwMibInfo->u4Index);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "===Rx Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx with CRC=%ld\n", prHwMibInfo->rHwMibCnt.u4RxFcsErrCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx drop due to out of resource=%ld\n", prHwMibInfo->rHwMibCnt.u4RxFifoFullCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx Mpdu=%ld\n", prHwMibInfo->rHwMibCnt.u4RxMpduCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx AMpdu=%ld\n", prHwMibInfo->rHwMibCnt.u4RxAMPDUCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx PF Drop=%ld\n", prHwMibInfo->rHwMibCnt.u4PFDropCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx Len Mismatch=%ld\n", prHwMibInfo->rHwMibCnt.u4RxLenMismatchCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx data indicate total=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_INDICATION_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx data retain total=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_RETAINED_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx drop by SW total=%ld\n", RX_GET_CNT(prRxCtrl, RX_DROP_TOTAL_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx reorder miss=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx reorder within=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx reorder ahead=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx reorder behind=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_BEHIND_COUNT));

		do {
			UINT_32 u4AmsduCntx100 = 0;

			if (RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT))
				u4AmsduCntx100 = (UINT_32)div64_u64(RX_GET_CNT(prRxCtrl,
					RX_DATA_MSDU_IN_AMSDU_COUNT) * 100,
					RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT));

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tRx avg MSDU in AMSDU=%1ld.%02ld\n", u4AmsduCntx100 / 100, u4AmsduCntx100 % 100);
		} while (FALSE);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx total MSDU in AMSDU=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_MSDU_IN_AMSDU_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx AMSDU=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx AMSDU miss=%ld\n", RX_GET_CNT(prRxCtrl, RX_DATA_AMSDU_MISS_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx no StaRec drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_NO_STA_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx inactive BSS drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_INACTIVE_BSS_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx HS20 drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_HS20_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx low SwRfb drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_LESS_SW_RFB_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx dupicate drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_DUPICATE_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx MIC err drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_MIC_ERROR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx BAR handle=%ld\n", RX_GET_CNT(prRxCtrl, RX_BAR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx non-interest drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_NO_INTEREST_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx type err drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_TYPE_ERR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx class err drop=%ld\n", RX_GET_CNT(prRxCtrl, RX_CLASS_ERR_DROP_COUNT));
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "===Phy/Timing Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tChannelIdleCnt=%ld\n", prHwMibInfo->rHwMibCnt.u4ChannelIdleCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tCCA_NAV_Tx_Time=%ld\n", prHwMibInfo->rHwMibCnt.u4CcaNavTx);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tRx_MDRDY_CNT=%ld\n", prHwMibInfo->rHwMibCnt.u4MdrdyCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tCCK_MDRDY=%ld, OFDM_MDRDY=0x%lx, OFDM_GREEN_MDRDY=0x%lx\n",
			prHwMibInfo->rHwMibCnt.u4CCKMdrdyCnt, prHwMibInfo->rHwMibCnt.u4OFDMLGMixMdrdy,
			prHwMibInfo->rHwMibCnt.u4OFDMGreenMdrdy);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tPrim CCA Time=%ld\n", prHwMibInfo->rHwMibCnt.u4PCcaTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tSec CCA Time=%ld\n", prHwMibInfo->rHwMibCnt.u4SCcaTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tPrim ED Time=%ld\n", prHwMibInfo->rHwMibCnt.u4PEDTime);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "===Tx Related Counters(Generic)===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tBeaconTxCnt=%ld\n", prHwMibInfo->rHwMibCnt.u4BeaconTxCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tTx 40MHz Cnt=%ld\n", prHwMibInfo->rHwMib2Cnt.u4Tx40MHzCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tTx 80MHz Cnt=%ld\n", prHwMibInfo->rHwMib2Cnt.u4Tx80MHzCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tTx 160MHz Cnt=%ld\n", prHwMibInfo->rHwMib2Cnt.u4Tx160MHzCnt);
		for (i = 0; i < BSSID_NUM; i++) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\t===BSSID[%d] Related Counters===\n", i);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tBA Miss Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4BaMissedCnt[i]);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tRTS Tx Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4RtsTxCnt[i]);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tFrame Retry Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4FrameRetryCnt[i]);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tFrame Retry 2 Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4FrameRetry2Cnt[i]);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tRTS Retry Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4RtsRetryCnt[i]);
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"\tAck Failed Cnt=%ld\n", prHwMibInfo->rHwMibCnt.au4AckFailedCnt[i]);
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "===AMPDU Related Counters===\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tTx AMPDU_Pkt_Cnt=%ld\n", prHwMibInfo->rHwTxAmpduMts.u2TxAmpduCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tTx AMPDU_MPDU_Pkt_Cnt=%ld\n", prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tAMPDU SuccessCnt=%ld\n", prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tAMPDU Tx success      = %ld\n", prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt);

		u4Per = prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt == 0 ? 0 :
			(1000 * (prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt - prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt)) /
			prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt;
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\tAMPDU Tx fail count   = %ld, PER=%ld.%1ld%%\n",
			prHwMibInfo->rHwTxAmpduMts.u4TxSfCnt - prHwMibInfo->rHwTxAmpduMts.u4TxAckSfCnt,
			u4Per/10, u4Per%10);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "\tTx Agg\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s", "\tRange:  1    2~5   6~15    16~22   23~33    34~49    50~57    58~64\n");
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"\t\t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d\n",
			prHwMibInfo->rHwTxAmpduMts.u2TxRange1AmpduCnt, prHwMibInfo->rHwTxAmpduMts.u2TxRange2AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange3AmpduCnt, prHwMibInfo->rHwTxAmpduMts.u2TxRange4AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange5AmpduCnt, prHwMibInfo->rHwTxAmpduMts.u2TxRange6AmpduCnt,
			prHwMibInfo->rHwTxAmpduMts.u2TxRange7AmpduCnt, prHwMibInfo->rHwTxAmpduMts.u2TxRange8AmpduCnt);
	} else
		i4BytesWritten = kalSnprintf(pcCommand, i4TotalLen, "%s", "\nClear All Statistics\n");

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwMibInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_MIB_INFO_T));

	nicRxClearStatistics(prGlueInfo->prAdapter);

	return i4BytesWritten;
}

static int priv_driver_set_fw_log(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4McuDest = 0;
	UINT_32 u4LogType = 0;
	P_CMD_FW_LOG_2_HOST_CTRL_T prFwLog2HostCtrl;
	UINT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(RSN, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	DBGLOG(RSN, INFO, "MT6632 : priv_driver_set_fw_log\n");

	prFwLog2HostCtrl = (P_CMD_FW_LOG_2_HOST_CTRL_T)kalMemAlloc(sizeof(CMD_FW_LOG_2_HOST_CTRL_T), VIR_MEM_TYPE);
	if (!prFwLog2HostCtrl)
		return -1;

	if (i4Argc == 3) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4McuDest);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4McuDest error u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &u4LogType);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4LogType error u4Ret=%d\n", u4Ret);

		prFwLog2HostCtrl->ucMcuDest = (UINT_8)u4McuDest;
		prFwLog2HostCtrl->ucFwLog2HostCtrl = (UINT_8)u4LogType;

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetFwLog2Host,
				   prFwLog2HostCtrl, sizeof(CMD_FW_LOG_2_HOST_CTRL_T), TRUE, TRUE, TRUE, &u4BufLen);

	    DBGLOG(REQ, INFO, "%s: command result is %s (%d %d)\n", __func__, pcCommand, u4McuDest, u4LogType);
		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			kalMemFree(prFwLog2HostCtrl, VIR_MEM_TYPE, sizeof(CMD_FW_LOG_2_HOST_CTRL_T));
			return -1;
		}
	} else {
		DBGLOG(REQ, ERROR, "argc %i is not equal to 3\n", i4Argc);
		return -1;
	}

	return i4BytesWritten;
}
#endif

static int priv_driver_get_mcr(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret;
	INT_32 i4ArgNum = 2;
	CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_mcr error (Address) u4Ret=%d\n", u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x", (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_mcr */

int priv_driver_set_mcr(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	UINT_32 u4Ret;
	INT_32 i4ArgNum = 3;
	CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_mcr error (Address) u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_mcr error (Data) u4Ret=%d\n", u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		/* rCmdAccessReg.u4Data = kalStrtoul(apcArgv[2], NULL, 0); */

		rStatus = kalIoctl(prGlueInfo,
					wlanoidSetMcrWrite,
					&rCmdAccessReg, sizeof(rCmdAccessReg), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}

static int priv_driver_set_test_mode(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret;
	INT_32 i4ArgNum = 2, u4MagicKey;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(u4MagicKey));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse Magic Key error u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, LOUD, "The Set Test Mode Magic Key is %d\n", u4MagicKey);

		if (u4MagicKey == PRIV_CMD_TEST_MAGIC_KEY) {
			rStatus = kalIoctl(prGlueInfo,
					   wlanoidRftestSetTestMode,
					   NULL, 0, FALSE, FALSE, TRUE, &u4BufLen);
		} else if (u4MagicKey == 0) {
			rStatus = kalIoctl(prGlueInfo,
					   wlanoidRftestSetAbortTestMode,
					   NULL, 0, FALSE, FALSE, TRUE, &u4BufLen);
		}

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;

}				/* priv_driver_set_test_mode */

static int priv_driver_set_test_cmd(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret;
	INT_32 i4ArgNum = 3;
	PARAM_MTK_WIFI_TEST_STRUCT_T rRfATInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "Parse Test CMD Data error u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, LOUD, "Set Test CMD FuncIndex = %d, FuncData = %d\n",
			rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidRftestSetAutoTest,
				   &rRfATInfo, sizeof(rRfATInfo), FALSE, FALSE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;

}				/* priv_driver_set_test_cmd */

static int priv_driver_get_test_result(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret;
	UINT_32 u4Data = 0;
	INT_32 i4ArgNum = 3;
	PARAM_MTK_WIFI_TEST_STRUCT_T rRfATInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rRfATInfo.u4FuncIndex));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "Parse Test CMD Index error u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rRfATInfo.u4FuncData));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "Parse Test CMD Data error u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, LOUD, "Get Test CMD FuncIndex = %d, FuncData = %d\n",
			rRfATInfo.u4FuncIndex, rRfATInfo.u4FuncData);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidRftestQueryAutoTest,
				   &rRfATInfo, sizeof(rRfATInfo), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
		u4Data = (unsigned int)rRfATInfo.u4FuncData;
		i4BytesWritten = snprintf(pcCommand, i4TotalLen, "%d[0x%08x]", u4Data, u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_test_result */

INT_32 priv_driver_tx_rate_info(IN char *pcCommand, IN int i4TotalLen, BOOLEAN fgDumpAll,
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo, P_PARAM_GET_STA_STATISTICS prQueryStaStatistics)
{
	UINT_8 i, txmode, rate, stbc;
	UINT_8 nsts;
	INT_32 i4BytesWritten = 0;

	for (i = 0; i < AUTO_RATE_NUM; i++) {

		txmode = HW_TX_RATE_TO_MODE(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);
		if (txmode >= MAX_TX_MODE)
			txmode = MAX_TX_MODE;
		rate = HW_TX_RATE_TO_MCS(prHwWlanInfo->rWtblRateInfo.au2RateCode[i], txmode);
		nsts = HW_TX_RATE_TO_NSS(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]) + 1;
		stbc = HW_TX_RATE_TO_STBC(prHwWlanInfo->rWtblRateInfo.au2RateCode[i]);

		if (fgDumpAll) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"Rate index[%d]    ", i);

			if (prHwWlanInfo->rWtblRateInfo.ucRateIdx == i) {
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s", "--> ");
			} else {
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s", "    ");
			}
		}

		if (!fgDumpAll) {
			if (prHwWlanInfo->rWtblRateInfo.ucRateIdx != i)
				continue;
			else
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%-20s%s", "Last TX Rate", " = ");
		}

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? HW_TX_RATE_CCK_STR[rate] : HW_TX_RATE_CCK_STR[4]);
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", hw_rate_ofdm_str(rate));
		else if ((txmode == TX_RATE_MODE_HTMIX) || (txmode == TX_RATE_MODE_HTGF))
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"MCS%d, ", rate);
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"NSS%d_MCS%d, ", nsts, rate);

		if (prQueryStaStatistics->ucSkipAr) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability < 4 ?
				HW_TX_RATE_BW[prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability] : HW_TX_RATE_BW[4]);
		} else {
			if ((txmode == TX_RATE_MODE_CCK) || (txmode == TX_RATE_MODE_OFDM))
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s, ", HW_TX_RATE_BW[0]);
			else
				i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
					"%s, ", prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability < 4 ?
					HW_TX_RATE_BW[prHwWlanInfo->rWtblPeerCap.ucFrequencyCapability] :
					HW_TX_RATE_BW[4]);
		}

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", priv_driver_get_sgi_info(&prHwWlanInfo->rWtblPeerCap) == 0 ? "LGI" : "SGI");

		if (prQueryStaStatistics->ucSkipAr) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s%s%s\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
				stbc ? ", STBC, " : ", ",
				priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig) == 0 ? "BCC" : "LDPC");
		} else if (prQueryStaStatistics->aucArRatePer[prQueryStaStatistics->aucRateEntryIndex[i]] == 0xFF) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s%s%s   (--)\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
				stbc ? ", STBC, " : ", ",
				((priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig) == 0) ||
				(txmode == TX_RATE_MODE_CCK) || (txmode == TX_RATE_MODE_OFDM)) ? "BCC" : "LDPC");
		} else {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s%s%s   (%d)\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
				stbc ? ", STBC, " : ", ",
				((priv_driver_get_ldpc_info(&prHwWlanInfo->rWtblTxConfig) == 0) ||
				(txmode == TX_RATE_MODE_CCK) || (txmode == TX_RATE_MODE_OFDM)) ? "BCC" : "LDPC",
				prQueryStaStatistics->aucArRatePer[prQueryStaStatistics->aucRateEntryIndex[i]]);
		}

		if (!fgDumpAll)
			break;
	}

	return i4BytesWritten;
}

INT_32 priv_driver_rx_rate_info(P_ADAPTER_T prAdapter, IN char *pcCommand, IN int i4TotalLen,
	IN UINT_8 ucWlanIdx)
{
	UINT_32 txmode, rate, frmode, sgi, nsts, ldpc, stbc, groupid, mu;
	INT_32 i4BytesWritten = 0;
	UINT_32 u4RxVector0 = 0, u4RxVector1 = 0;
	UINT_8 ucStaIdx;

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) == WLAN_STATUS_SUCCESS) {
		u4RxVector0 = prAdapter->arStaRec[ucStaIdx].u4RxVector0;
		u4RxVector1 = prAdapter->arStaRec[ucStaIdx].u4RxVector1;
		DBGLOG(REQ, LOUD, "****** RX Vector0 = 0x%08x ******\n", u4RxVector0);
		DBGLOG(REQ, LOUD, "****** RX Vector1 = 0x%08x ******\n", u4RxVector1);
	} else {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s", "Last RX Rate", " = NOT SUPPORT");
		return i4BytesWritten;
	}

	txmode = (u4RxVector0 & RX_VT_RX_MODE_MASK) >> RX_VT_RX_MODE_OFFSET;
	rate = (u4RxVector0 & RX_VT_RX_RATE_MASK) >> RX_VT_RX_RATE_OFFSET;
	frmode = (u4RxVector0 & RX_VT_FR_MODE_MASK) >> RX_VT_FR_MODE_OFFSET;
	nsts = ((u4RxVector1 & RX_VT_NSTS_MASK) >> RX_VT_NSTS_OFFSET);
	stbc = (u4RxVector0 & RX_VT_STBC_MASK) >> RX_VT_STBC_OFFSET;
	sgi = u4RxVector0 & RX_VT_SHORT_GI;
	ldpc = u4RxVector0 & RX_VT_LDPC;
	groupid = (u4RxVector1 & RX_VT_GROUP_ID_MASK) >> RX_VT_GROUP_ID_OFFSET;

	if (groupid && groupid != 63) {
		mu = 1;
	} else {
		mu = 0;
		nsts += 1;
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s", "Last RX Rate", " = ");

	if (txmode == TX_RATE_MODE_CCK)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, ", rate < 4 ? HW_TX_RATE_CCK_STR[rate] : HW_TX_RATE_CCK_STR[4]);
	else if (txmode == TX_RATE_MODE_OFDM)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, ", hw_rate_ofdm_str(rate));
	else if ((txmode == TX_RATE_MODE_HTMIX) || (txmode == TX_RATE_MODE_HTGF))
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"MCS%d, ", rate);
	else
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"NSS%d_MCS%d, ", nsts, rate);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s, ", frmode < 4 ? HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

	if (txmode == TX_RATE_MODE_CCK)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, ", rate < 4 ? "LP" : "SP");
	else if (txmode == TX_RATE_MODE_OFDM)
		;
	else
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s,", sgi == 0 ? "LGI" : "SGI");

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", stbc == 0 ? " " : "STBC, ");

	if (mu) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, %s, %s (%d)\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
			ldpc == 0 ? "BCC" : "LDPC", "MU", groupid);
	} else {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, %s\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
			ldpc == 0 ? "BCC" : "LDPC");
	}

	return i4BytesWritten;
}

INT_32 priv_driver_tx_vector_info(IN char *pcCommand, IN int i4TotalLen,
	IN P_TX_VECTOR_BBP_LATCH_T prTxV)
{
	UINT_8 rate, txmode, frmode, sgi, ldpc, nsts, stbc, txpwr;
	INT_32 i4BytesWritten = 0;

	rate = TX_VECTOR_GET_TX_RATE(prTxV);
	txmode = TX_VECTOR_GET_TX_MODE(prTxV);
	frmode = TX_VECTOR_GET_TX_FRMODE(prTxV);
	nsts = TX_VECTOR_GET_TX_NSTS(prTxV) + 1;
	sgi = TX_VECTOR_GET_TX_SGI(prTxV);
	ldpc = TX_VECTOR_GET_TX_LDPC(prTxV);
	stbc = TX_VECTOR_GET_TX_STBC(prTxV);
	txpwr = TX_VECTOR_GET_TX_PWR(prTxV);

	if (prTxV->u4TxVector1 == 0xFFFFFFFF) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%s\n", "TX Info", " = ", "N/A");
	} else {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s", "TX Info", " = ");

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? HW_TX_RATE_CCK_STR[rate] : HW_TX_RATE_CCK_STR[4]);
		else if (txmode == TX_RATE_MODE_OFDM)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", hw_rate_ofdm_str(rate));
		else if ((txmode == TX_RATE_MODE_HTMIX) || (txmode == TX_RATE_MODE_HTGF))
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"MCS%d, ", rate);
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"NSS%d_MCS%d, ", nsts, rate);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s, ", frmode < 4 ? HW_TX_RATE_BW[frmode] : HW_TX_RATE_BW[4]);

		if (txmode == TX_RATE_MODE_CCK)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", rate < 4 ? "LP" : "SP");
		else if (txmode == TX_RATE_MODE_OFDM)
			;
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%s, ", sgi == 0 ? "LGI" : "SGI");

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%s%s%s\n", txmode < 5 ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[5],
			stbc ? ", STBC, " : ", ", ldpc == 0 ? "BCC" : "LDPC");
	}

	return i4BytesWritten;
}

static INT_32 priv_driver_dump_stat_info(P_ADAPTER_T prAdapter, IN char *pcCommand, IN int i4TotalLen,
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo, P_PARAM_GET_STA_STATISTICS prQueryStaStatistics,
	BOOLEAN fgResetCnt)
{
	INT_32 i4BytesWritten = 0;
	PARAM_RSSI rRssi;
	UINT_16 u2LinkSpeed;
	UINT_32 u4Per, u4RxPer[ENUM_BAND_NUM], u4AmpduPer[ENUM_BAND_NUM], u4InstantPer;
	UINT_8 ucDbdcIdx, ucStaIdx;
	UINT_8 ucSkipAr;
	static UINT_32 u4TotalTxCnt, u4TotalFailCnt;
	static UINT_32 u4Rate1TxCnt, u4Rate1FailCnt;
	static UINT_32 au4RxMpduCnt[ENUM_BAND_NUM] = {0};
	static UINT_32 au4FcsError[ENUM_BAND_NUM] = {0};
	static UINT_32 au4RxFifoCnt[ENUM_BAND_NUM] = {0};
	static UINT_32 au4AmpduTxSfCnt[ENUM_BAND_NUM] = {0};
	static UINT_32 au4AmpduTxAckSfCnt[ENUM_BAND_NUM] = {0};
	P_RX_CTRL_T prRxCtrl;

	ucSkipAr = prQueryStaStatistics->ucSkipAr;
	prRxCtrl = &prAdapter->rRxCtrl;

	if (ucSkipAr) {
		u4TotalTxCnt += prHwWlanInfo->rWtblTxCounter.u2CurBwTxCnt +
							prHwWlanInfo->rWtblTxCounter.u2OtherBwTxCnt;
		u4TotalFailCnt += prHwWlanInfo->rWtblTxCounter.u2CurBwFailCnt +
							prHwWlanInfo->rWtblTxCounter.u2OtherBwFailCnt;
		u4Rate1TxCnt += prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt;
		u4Rate1FailCnt += prHwWlanInfo->rWtblTxCounter.u2Rate1FailCnt;
	}

	if (ucSkipAr) {
		u4Per = (prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt == 0) ?
					(0) : (1000 * u4Rate1FailCnt / u4Rate1TxCnt);

		u4InstantPer = (prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt == 0) ?
					(0) : (1000 * (prHwWlanInfo->rWtblTxCounter.u2Rate1FailCnt) /
					(prHwWlanInfo->rWtblTxCounter.u2Rate1TxCnt));
	} else {
		u4Per = (prQueryStaStatistics->u4Rate1TxCnt == 0) ?
					(0) : (1000 * (prQueryStaStatistics->u4Rate1FailCnt) /
					(prQueryStaStatistics->u4Rate1TxCnt));

		u4InstantPer = (prQueryStaStatistics->ucPer == 0) ?
					(0) : (prQueryStaStatistics->ucPer);
	}

	for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
		au4RxMpduCnt[ucDbdcIdx] += prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4RxMpduCnt;
		au4FcsError[ucDbdcIdx] += prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4FcsError;
		au4RxFifoCnt[ucDbdcIdx] += prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4RxFifoFull;
		au4AmpduTxSfCnt[ucDbdcIdx] += prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4AmpduTxSfCnt;
		au4AmpduTxAckSfCnt[ucDbdcIdx] += prQueryStaStatistics->rMibInfo[ucDbdcIdx].u4AmpduTxAckSfCnt;

		u4RxPer[ucDbdcIdx] = ((au4RxMpduCnt[ucDbdcIdx] + au4FcsError[ucDbdcIdx]) == 0) ?
							(0) : (1000 * au4FcsError[ucDbdcIdx] /
							(au4RxMpduCnt[ucDbdcIdx] + au4FcsError[ucDbdcIdx]));

		u4AmpduPer[ucDbdcIdx] = (au4AmpduTxSfCnt[ucDbdcIdx] == 0) ?
					(0) : (1000 * (au4AmpduTxSfCnt[ucDbdcIdx] - au4AmpduTxAckSfCnt[ucDbdcIdx]) /
					au4AmpduTxSfCnt[ucDbdcIdx]);
	}

	rRssi = RCPI_TO_dBm(prQueryStaStatistics->ucRcpi);
	u2LinkSpeed = (prQueryStaStatistics->u2LinkSpeed == 0) ? 0 : prQueryStaStatistics->u2LinkSpeed / 2;

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "\n\nSTA Stat:\n");

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "CurrTemperature", " = ",
		prQueryStaStatistics->ucTemperature);

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Tx Total cnt", " = ", ucSkipAr ?
		(u4TotalTxCnt):(prQueryStaStatistics->u4TransmitCount));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Tx Fail Cnt", " = ", ucSkipAr ?
		(u4TotalFailCnt):(prQueryStaStatistics->u4TransmitFailCount));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Rate1 Tx Cnt", " = ", ucSkipAr ?
		(u4Rate1TxCnt):(prQueryStaStatistics->u4Rate1TxCnt));

	if (ucSkipAr)
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d, PER = %d.%1d%%, instant PER = %d.%1d%%\n", "Rate1 Fail Cnt", " = ",
			u4Rate1FailCnt, u4Per/10, u4Per%10, u4InstantPer/10, u4InstantPer%10);
	else
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d, PER = %d.%1d%%, instant PER = %d%%\n", "Rate1 Fail Cnt", " = ",
			prQueryStaStatistics->u4Rate1FailCnt, u4Per/10, u4Per%10, u4InstantPer);

	if ((ucSkipAr) && (fgResetCnt)) {
		u4TotalTxCnt = 0;
		u4TotalFailCnt = 0;
		u4Rate1TxCnt = 0;
		u4Rate1FailCnt = 0;
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "----- MIB Info -----\n");

	for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"[DBDC_%d] :\n", ucDbdcIdx);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX Success", " = ", au4RxMpduCnt[ucDbdcIdx]);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d, PER = %d.%1d%%\n", "RX with CRC", " = ",
			au4FcsError[ucDbdcIdx], u4RxPer[ucDbdcIdx]/10, u4RxPer[ucDbdcIdx]%10);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "RX drop FIFO full", " = ", au4RxFifoCnt[ucDbdcIdx]);
#if 0
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d\n", "AMPDU Tx success", " = ", au4AmpduTxSfCnt[ucDbdcIdx]);

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d, PER = %d.%1d%%\n", "AMPDU Tx fail count", " = ",
			(au4AmpduTxSfCnt[ucDbdcIdx] - au4AmpduTxAckSfCnt[ucDbdcIdx]),
			u4AmpduPer[ucDbdcIdx]/10, u4AmpduPer[ucDbdcIdx]%10);
#endif
	}

	if (fgResetCnt) {
		kalMemZero(au4RxMpduCnt, sizeof(au4RxMpduCnt));
		kalMemZero(au4FcsError, sizeof(au4RxMpduCnt));
		kalMemZero(au4RxFifoCnt, sizeof(au4RxMpduCnt));
		kalMemZero(au4AmpduTxSfCnt, sizeof(au4RxMpduCnt));
		kalMemZero(au4AmpduTxAckSfCnt, sizeof(au4RxMpduCnt));
	}

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "--------------------\n");

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d %d %d %d\n", "RSSI", " = ",
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi0),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi1),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi2),
		RCPI_TO_dBm(prHwWlanInfo->rWtblRxCounter.ucRxRcpi3));

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "LinkSpeed", " = ", u2LinkSpeed);

	/* Last TX Rate */
	i4BytesWritten += priv_driver_tx_rate_info(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, FALSE, prHwWlanInfo, prQueryStaStatistics);

	/* Last RX Rate */
	i4BytesWritten += priv_driver_rx_rate_info(prAdapter, pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, (UINT_8)(prHwWlanInfo->u4Index));

	/* TxV */
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "----- TXV Info -----\n");

	for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"[DBDC_%d] :\n", ucDbdcIdx);

		i4BytesWritten += priv_driver_tx_vector_info(pcCommand + i4BytesWritten,
			i4TotalLen - i4BytesWritten, &prQueryStaStatistics->rTxVector[ucDbdcIdx]);

		if (prQueryStaStatistics->rTxVector[ucDbdcIdx].u4TxVector1 == 0xFFFFFFFF)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "MAC TX Power", " = ", "N/A");
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%ld.%1ld dBm\n", "MAC TX Power", " = ",
				TX_VECTOR_GET_TX_PWR(&prQueryStaStatistics->rTxVector[ucDbdcIdx]) >> 1,
				5 * (TX_VECTOR_GET_TX_PWR(&prQueryStaStatistics->rTxVector[ucDbdcIdx]) % 2));

		if (prQueryStaStatistics->rTxVector[ucDbdcIdx].u4TxVector2 == 0xFFFFFFFF)
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%s\n", "Beamform Enable", " = ", "N/A");
		else
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "Beamform Enable", " = ",
				TX_VECTOR_GET_BF_EN(&prQueryStaStatistics->rTxVector[ucDbdcIdx]) ? TRUE : FALSE);

		if (prQueryStaStatistics->rTxVector[ucDbdcIdx].u4TxVector4 == 0xFFFFFFFF) {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Dynamic BW", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Sounding Pkt", " = ", "N/A");
		} else {
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "Dynamic BW", " = ",
				TX_VECTOR_GET_DYN_BW(&prQueryStaStatistics->rTxVector[ucDbdcIdx]) ? TRUE : FALSE);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "Sounding Pkt", " = ",
				TX_VECTOR_GET_NO_SOUNDING(&prQueryStaStatistics->rTxVector[ucDbdcIdx]) ? FALSE : TRUE);
		}
	}

    /* RX Reorder */
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "------ RX Info -----\n");
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Rx reorder miss", " = ", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_MISS_COUNT));
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Rx reorder within", " = ", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_WITHIN_COUNT));
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Rx reorder ahead", " = ", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_AHEAD_COUNT));
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-20s%s%d\n", "Rx reorder behind", " = ", RX_GET_CNT(prRxCtrl, RX_DATA_REORDER_BEHIND_COUNT));

	/* AR info */
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "------ AR Info -----\n");

	if (!prQueryStaStatistics->ucSkipAr) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%s\n", "RateTable", " = ", prQueryStaStatistics->ucArTableIdx < 6 ?
			RATE_TBLE[prQueryStaStatistics->ucArTableIdx] : RATE_TBLE[6]);

		if (wlanGetStaIdxByWlanIdx(prAdapter, (UINT_8)(prHwWlanInfo->u4Index), &ucStaIdx) ==
			WLAN_STATUS_SUCCESS){
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "2G Support 256QAM TX", " = ",
				(prAdapter->arStaRec[ucStaIdx].u4Flags & MTK_SYNERGY_CAP_SUPPORT_24G_MCS89) ?
				1 : 0);
		}

		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%-20s%s%d%%\n", "Rate1 instantPer", " = ", u4InstantPer);

		if (prQueryStaStatistics->ucAvePer == 0xFF) {
#if 0
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "average RSSI", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Rate1 AvePer", " = ", "N/A");
#endif
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Train Down", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Train Up", " = ", "N/A");
#if 0
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Highest Rate Cnt", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Lowest Rate Cnt", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "SGI Pass Cnt", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR State Prev", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR State Curr", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR Action Type", " = ", "N/A");

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "Rate Entry Idx", " = ", "N/A");
#endif
		} else {
#if 0
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "average RSSI", " = ", rRssi);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d%%\n", "Rate1 AvePer", " = ", prQueryStaStatistics->ucAvePer);
#endif
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d -> %d\n", "Train Down", " = ",
				(prQueryStaStatistics->u2TrainDown) & BITS(0, 7),
				((prQueryStaStatistics->u2TrainDown) >> 8) & BITS(0, 7));

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d -> %d\n", "Train Up", " = ",
				(prQueryStaStatistics->u2TrainUp) & BITS(0, 7),
				((prQueryStaStatistics->u2TrainUp) >> 8) & BITS(0, 7));
#if 0
			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "Highest Rate Cnt", " = ", prQueryStaStatistics->ucHighestRateCnt);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "Lowest Rate Cnt", " = ", prQueryStaStatistics->ucLowestRateCnt);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d\n", "SGI Pass Cnt", " = ", prQueryStaStatistics->ucTxSgiDetectPassCnt);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR State Prev", " = ",
				prQueryStaStatistics->ucArStatePrev < 3 ?
				AR_STATE[prQueryStaStatistics->ucArStatePrev] : AR_STATE[3]);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR State Curr", " = ",
				prQueryStaStatistics->ucArStateCurr < 3 ?
				AR_STATE[prQueryStaStatistics->ucArStateCurr] : AR_STATE[3]);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%s\n", "AR Action Type", " = ",
				prQueryStaStatistics->ucArActionType < 16 ?
				AR_ACTION[prQueryStaStatistics->ucArActionType] : AR_ACTION[16]);

			i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
				"%-20s%s%d -> %d\n", "Rate Entry Idx", " = ",
				(prQueryStaStatistics->ucRateEntryIdxPrev == AR_RATE_ENTRY_INDEX_NULL) ?
				(prQueryStaStatistics->ucRateEntryIdx) : (prQueryStaStatistics->ucRateEntryIdxPrev),
				prQueryStaStatistics->ucRateEntryIdx);
#endif
		}
	}

	/* Rate1~Rate8 */
	i4BytesWritten += priv_driver_tx_rate_info(pcCommand + i4BytesWritten,
		i4TotalLen - i4BytesWritten, TRUE, prHwWlanInfo, prQueryStaStatistics);

    /* Tx Agg */
	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%s", "------ TX AGG -----\n");

	i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
		"%-12s%s", "Range:", "1     2~5     6~15    16~22    23~33    34~49    50~57    58~64\n");

	for (ucDbdcIdx = 0; ucDbdcIdx < ENUM_BAND_NUM; ucDbdcIdx++) {
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"DBDC%d:", ucDbdcIdx);
		i4BytesWritten += kalSnprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
			"%7d%8d%9d%9d%9d%9d%9d%9d\n",
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange1AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange2AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange3AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange4AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange5AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange6AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange7AmpduCnt,
			prQueryStaStatistics->rMibInfo[ucDbdcIdx].u2TxRange8AmpduCnt);
	}

	return i4BytesWritten;
}

static int priv_driver_get_sta_stat(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_8 aucMacAddr[MAC_ADDR_LEN];
	UINT_8 ucWlanIndex;
	PUINT_8 pucMacAddr = NULL;
	P_PARAM_HW_WLAN_INFO_T prHwWlanInfo;
	P_PARAM_GET_STA_STATISTICS prQueryStaStatistics;
	BOOLEAN fgResetCnt = FALSE;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 3) {
		if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT, strlen(CMD_STAT_RESET_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[2], &aucMacAddr[0]);
			fgResetCnt = TRUE;
		} else if (strnicmp(apcArgv[2], CMD_STAT_RESET_CNT, strlen(CMD_STAT_RESET_CNT)) == 0) {
			wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
			fgResetCnt = TRUE;
		} else {
			wlanHwAddrToBin(apcArgv[1], &aucMacAddr[0]);
			fgResetCnt = FALSE;
		}

		if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, &aucMacAddr[0], &ucWlanIndex))
			return i4BytesWritten;
	} else {
		/* Get AIS AP address for no argument */
		if (prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP)
			ucWlanIndex = prGlueInfo->prAdapter->prAisBssInfo->prStaRecOfAP->ucWlanIndex;
		else if (!wlanGetWlanIdxByAddress(prGlueInfo->prAdapter, NULL, &ucWlanIndex)) /* try get a peer */
			return i4BytesWritten;

		if (i4Argc == 2) {
			if (strnicmp(apcArgv[1], CMD_STAT_RESET_CNT, strlen(CMD_STAT_RESET_CNT)) == 0)
				fgResetCnt = TRUE;
		}
	}

	prHwWlanInfo = (P_PARAM_HW_WLAN_INFO_T)kalMemAlloc(sizeof(PARAM_HW_WLAN_INFO_T), VIR_MEM_TYPE);
	if (!prHwWlanInfo)
		return -1;

	prHwWlanInfo->u4Index = ucWlanIndex;

	DBGLOG(REQ, INFO, "MT6632 : index = %d i4TotalLen = %d\n", prHwWlanInfo->u4Index, i4TotalLen);

	/* Get WTBL info */
	rStatus = kalIoctl(prGlueInfo,
					wlanoidQueryWlanInfo,
					prHwWlanInfo, sizeof(PARAM_HW_WLAN_INFO_T), TRUE, TRUE, TRUE, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));
		return -1;
	}

	/* Get Statistics info */
	prQueryStaStatistics =
		(P_PARAM_GET_STA_STATISTICS)kalMemAlloc(sizeof(PARAM_GET_STA_STA_STATISTICS), VIR_MEM_TYPE);
	if (!prQueryStaStatistics)
		return -1;

	prQueryStaStatistics->ucResetCounter = fgResetCnt;

	pucMacAddr = wlanGetStaAddrByWlanIdx(prGlueInfo->prAdapter, ucWlanIndex);
	COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, pucMacAddr);

	rStatus = kalIoctl(prGlueInfo,
					wlanoidQueryStaStatistics,
					prQueryStaStatistics,
					sizeof(PARAM_GET_STA_STA_STATISTICS), TRUE, TRUE, TRUE, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE, sizeof(PARAM_GET_STA_STA_STATISTICS));
		return -1;
	}

	if (pucMacAddr) {
		i4BytesWritten = priv_driver_dump_stat_info(prAdapter, pcCommand, i4TotalLen,
			prHwWlanInfo, prQueryStaStatistics, fgResetCnt);
	}
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);

	kalMemFree(prHwWlanInfo, VIR_MEM_TYPE, sizeof(PARAM_HW_WLAN_INFO_T));
	kalMemFree(prQueryStaStatistics, VIR_MEM_TYPE, sizeof(PARAM_GET_STA_STA_STATISTICS));

	if (fgResetCnt)
		nicRxClearStatistics(prGlueInfo->prAdapter);

	return i4BytesWritten;
}

static int priv_driver_get_drv_mcr(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret;
	/* INT_32 i4ArgNum_with_ant_sel = 3; */	/* Add Antenna Selection Input */
	INT_32 i4ArgNum = 2;

	CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_drv_mcr error (Address) u4Ret=%d\n", u4Ret);

		/* rCmdAccessReg.u4Address = kalStrtoul(apcArgv[1], NULL, 0); */
		rCmdAccessReg.u4Data = 0;

		DBGLOG(REQ, LOUD, "address is %x\n", rCmdAccessReg.u4Address);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryDrvMcrRead,
				   &rCmdAccessReg, sizeof(rCmdAccessReg), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x", (unsigned int)rCmdAccessReg.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_drv_mcr */

int priv_driver_set_drv_mcr(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	UINT_32 u4Ret;
	/* INT_32 i4ArgNum_with_ant_sel = 4; */	/* Add Antenna Selection Input */
	INT_32 i4ArgNum = 3;

	CMD_ACCESS_REG rCmdAccessReg;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rCmdAccessReg.u4Address));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_drv_mcr error (Address) u4Ret=%d\n", u4Ret);

		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rCmdAccessReg.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse get_drv_mcr error (Data) u4Ret=%d\n", u4Ret);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetDrvMcrWrite,
				   &rCmdAccessReg, sizeof(rCmdAccessReg), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}

static int priv_driver_get_sw_ctrl(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	INT_32 u4Ret = 0;

	PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* rSwCtrlInfo.u4Id = kalStrtoul(apcArgv[1], NULL, 0); */
		rSwCtrlInfo.u4Data = 0;
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

		DBGLOG(REQ, LOUD, "id is %x\n", rSwCtrlInfo.u4Id);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQuerySwCtrlRead,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo), TRUE, TRUE, TRUE, &u4BufLen);

		DBGLOG(REQ, LOUD, "rStatus %u\n", rStatus);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

		i4BytesWritten = snprintf(pcCommand, i4TotalLen, "0x%08x", (unsigned int)rSwCtrlInfo.u4Data);
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	}

	return i4BytesWritten;

}				/* priv_driver_get_sw_ctrl */


int priv_driver_set_sw_ctrl(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	INT_32 u4Ret = 0;

	PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 3) {
		/* rSwCtrlInfo.u4Id = kalStrtoul(apcArgv[1], NULL, 0);
		 *  rSwCtrlInfo.u4Data = kalStrtoul(apcArgv[2], NULL, 0);
		 */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSwCtrlInfo.u4Id));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);
		u4Ret = kalkStrtou32(apcArgv[2], 0, &(rSwCtrlInfo.u4Data));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse rSwCtrlInfo error u4Ret=%d\n", u4Ret);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetSwCtrlWrite,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	}

	return i4BytesWritten;

}				/* priv_driver_set_sw_ctrl */



int priv_driver_set_fixed_rate(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	/* INT_32 u4Ret = 0; */
	UINT_32 u4WCID = 0;
	UINT_32 u4Mode = 0, u4Bw = 0, u4Mcs = 0, u4VhtNss = 0;
	UINT_32 u4SGI = 0, u4Preamble = 0, u4STBC = 0, u4LDPC = 0, u4SpeEn = 0;
	INT_32 i4Recv = 0;
	CHAR *this_char = NULL;
	UINT_32 u4Id = 0xa0610000;
	UINT_32 u4Data = 0x80000000;
	UINT_32 u4Id2 = 0xa0600000;
	UINT_8 u4Nsts = 1;
	BOOLEAN fgStatus = TRUE;

	PARAM_CUSTOM_SW_CTRL_STRUCT_T rSwCtrlInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %d, apcArgv[0] = %s\n\n", i4Argc, *apcArgv);

	this_char = kalStrStr(*apcArgv, "=");
	this_char++;

	DBGLOG(REQ, LOUD, "string = %s\n", this_char);

	if (strnicmp(this_char, "auto", strlen("auto")) == 0) {
		i4Recv = 1;
	} else {
		i4Recv = sscanf(this_char, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d", &(u4WCID),
					&(u4Mode), &(u4Bw), &(u4Mcs), &(u4VhtNss),
					&(u4SGI), &(u4Preamble), &(u4STBC), &(u4LDPC), &(u4SpeEn));

		DBGLOG(REQ, LOUD, "u4WCID=%d\nu4Mode=%d\nu4Bw=%d\n", u4WCID, u4Mode, u4Bw);
	    DBGLOG(REQ, LOUD, "u4Mcs=%d\nu4VhtNss=%d\nu4SGI=%d\n", u4Mcs, u4VhtNss, u4SGI);
	    DBGLOG(REQ, LOUD, "u4Preamble=%d\nu4STBC=%d\n", u4Preamble, u4STBC);
	    DBGLOG(REQ, LOUD, "u4LDPC=%d\nu4SpeEn=%d\n", u4LDPC, u4SpeEn);
	}

	if (i4Recv == 1) {
		rSwCtrlInfo.u4Id = u4Id2;
		rSwCtrlInfo.u4Data = 0;

		rStatus = kalIoctl(prGlueInfo,
					wlanoidSetSwCtrlWrite,
					&rSwCtrlInfo, sizeof(rSwCtrlInfo),
					FALSE, FALSE, TRUE, &u4BufLen);
	} else if (i4Recv == 10) {
		rSwCtrlInfo.u4Id = u4Id;
		rSwCtrlInfo.u4Data = u4Data;

		if (u4SGI)
			rSwCtrlInfo.u4Data |= BIT(30);
		if (u4LDPC)
			rSwCtrlInfo.u4Data |= BIT(29);
		if (u4SpeEn)
			rSwCtrlInfo.u4Data |= BIT(28);
		if (u4STBC)
			rSwCtrlInfo.u4Data |= BIT(11);

		if (u4Bw <= 3)
			rSwCtrlInfo.u4Data |= ((u4Bw << 26) & BITS(26, 27));
		else {
			fgStatus = FALSE;
			DBGLOG(INIT, ERROR, "Wrong BW! BW20=0, BW40=1, BW80=2,BW160=3\n");
		}
		if (u4Mode <= 4) {
			rSwCtrlInfo.u4Data |= ((u4Mode << 6) & BITS(6, 8));

			switch (u4Mode) {
			case 0:
				if (u4Mcs <= 3)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR, "CCK mode but wrong MCS!\n");
				}

			if (u4Preamble)
				rSwCtrlInfo.u4Data |= BIT(2);
			else
				rSwCtrlInfo.u4Data &= ~BIT(2);

			break;
			case 1:
				switch (u4Mcs) {
				case 0:
					/* 6'b001011 */
					rSwCtrlInfo.u4Data |= 11;
					break;
				case 1:
					/* 6'b001111 */
					rSwCtrlInfo.u4Data |= 15;
					break;
				case 2:
					/* 6'b001010 */
					rSwCtrlInfo.u4Data |= 10;
					break;
				case 3:
					/* 6'b001110 */
					rSwCtrlInfo.u4Data |= 14;
					break;
				case 4:
					/* 6'b001001 */
					rSwCtrlInfo.u4Data |= 9;
					break;
				case 5:
					/* 6'b001101 */
					rSwCtrlInfo.u4Data |= 13;
					break;
				case 6:
					/* 6'b001000 */
					rSwCtrlInfo.u4Data |= 8;
					break;
				case 7:
					/* 6'b001100 */
					rSwCtrlInfo.u4Data |= 12;
					break;
				default:
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR, "OFDM mode but wrong MCS!\n");
				break;
				}
			break;
			case 2:
			case 3:
				if (u4Mcs <= 32)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
					fgStatus = FALSE;
					DBGLOG(INIT, ERROR, "HT mode but wrong MCS!\n");
				}

				if (u4Mcs != 32) {
					u4Nsts += (u4Mcs >> 3);
					if (u4STBC && (u4Nsts == 1))
						u4Nsts++;
				}
				break;
			case 4:
				if (u4Mcs <= 9)
					rSwCtrlInfo.u4Data |= u4Mcs;
				else {
				fgStatus = FALSE;
				DBGLOG(INIT, ERROR, "VHT mode but wrong MCS!\n");
				}
				if (u4STBC && (u4VhtNss == 1))
					u4Nsts++;
				else
					u4Nsts = u4VhtNss;
			break;
			default:
				break;
			}
		} else {
			fgStatus = FALSE;
			DBGLOG(INIT, ERROR, "Wrong TxMode! CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
		}

		rSwCtrlInfo.u4Data |= (((u4Nsts - 1) << 9) & BITS(9, 10));

		if (fgStatus) {
			rStatus = kalIoctl(prGlueInfo,
								wlanoidSetSwCtrlWrite,
								&rSwCtrlInfo, sizeof(rSwCtrlInfo),
								FALSE, FALSE, TRUE, &u4BufLen);
		}

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver FixedRate=Option\n");
		DBGLOG(INIT, ERROR,
			"Option:[WCID]-[Mode]-[BW]-[MCS]-[VhtNss]-[SGI]-[Preamble]-[STBC]-[LDPC]-[SPE_EN]\n");
		DBGLOG(INIT, ERROR, "[WCID]Wireless Client ID\n");
		DBGLOG(INIT, ERROR, "[Mode]CCK=0, OFDM=1, HT=2, GF=3, VHT=4\n");
		DBGLOG(INIT, ERROR, "[BW]BW20=0, BW40=1, BW80=2,BW160=3\n");
		DBGLOG(INIT, ERROR, "[MCS]CCK=0~3, OFDM=0~7, HT=0~32, VHT=0~9\n");
		DBGLOG(INIT, ERROR, "[VhtNss]VHT=1~4, Other=ignore\n");
		DBGLOG(INIT, ERROR, "[Preamble]Long=0, Other=Short\n");
	}

	return i4BytesWritten;
}				/* priv_driver_set_fixed_rate */

int priv_driver_set_cfg(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };

	PARAM_CUSTOM_KEY_CFG_STRUCT_T rKeyCfgInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;

	kalMemZero(&rKeyCfgInfo, sizeof(rKeyCfgInfo));

	if (i4Argc >= 3) {

		CHAR ucTmp[WLAN_CFG_VALUE_LEN_MAX];
		PUINT_8 pucCurrBuf = ucTmp;
		UINT_8	i = 0;

		pucCurrBuf = ucTmp;
		kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);

		if (i4Argc == 3) {
			/*no space for it, driver can't accept space in the end of the line*/
			/*ToDo: skip the space when parsing*/
			SPRINTF(pucCurrBuf, ("%s", apcArgv[2]));
		} else {
			for (i = 2; i < i4Argc; i++)
				SPRINTF(pucCurrBuf, ("%s ", apcArgv[i]));
		}

		DBGLOG(INIT, WARN, "Update to driver temp buffer as [%s]\n", ucTmp);

		/* wlanCfgSet(prAdapter, apcArgv[1], apcArgv[2], 0); */
		/* Call by  wlanoid because the set_cfg will trigger callback */
		kalStrnCpy(rKeyCfgInfo.aucKey, apcArgv[1], WLAN_CFG_KEY_LEN_MAX);
		kalStrnCpy(rKeyCfgInfo.aucValue, ucTmp, WLAN_CFG_KEY_LEN_MAX);
		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetKeyCfg, &rKeyCfgInfo, sizeof(rKeyCfgInfo), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}

	return i4BytesWritten;

}				/* priv_driver_set_cfg  */

int priv_driver_get_cfg(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	CHAR aucValue[WLAN_CFG_VALUE_LEN_MAX];

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);
	prAdapter = prGlueInfo->prAdapter;

	if (i4Argc >= 2) {
		/* by wlanoid ? */
		if (wlanCfgGet(prAdapter, apcArgv[1], aucValue, "", 0) == WLAN_STATUS_SUCCESS) {
			kalStrnCpy(pcCommand, aucValue, WLAN_CFG_VALUE_LEN_MAX);
			i4BytesWritten = kalStrnLen(pcCommand, WLAN_CFG_VALUE_LEN_MAX);
		}
	}

	return i4BytesWritten;

}				/* priv_driver_get_cfg  */

int priv_driver_set_chip_config(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	UINT_32 u4CmdLen = 0;
	UINT_32 u4PrefixLen = 0;
	/* INT_32 i4Argc = 0; */
	/* PCHAR  apcArgv[WLAN_CFG_ARGV_MAX] = {0}; */

	PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T rChipConfigInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	/* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
	/* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */
	/*  */
	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
	u4PrefixLen = kalStrLen(CMD_SET_CHIP) + 1 /*space */;

	kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

	/* if(i4Argc >= 2) { */
	if (u4CmdLen > u4PrefixLen) {

		rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
		/* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
		rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
		/* kalStrnCpy(rChipConfigInfo.aucCmd,apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
		kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen, CHIP_CONFIG_RESP_SIZE);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetChipConfig,
				   &rChipConfigInfo, sizeof(rChipConfigInfo), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__, rStatus);
			i4BytesWritten = -1;
		}
	}

	return i4BytesWritten;

}				/* priv_driver_set_chip_config  */

void
priv_driver_get_chip_config_16(PUINT_8 pucStartAddr, UINT_32 u4Length, UINT_32 u4Line, int i4TotalLen,
				INT_32 i4BytesWritten, char *pcCommand)
{

	while (u4Length >= 16) {
		if (i4TotalLen > i4BytesWritten) {
			i4BytesWritten +=
			    snprintf(pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"%04lx %02x %02x %02x %02x  %02x %02x %02x %02x - %02x %02x %02x %02x  %02x %02x %02x %02x\n",
					u4Line, pucStartAddr[0],
					pucStartAddr[1],
					pucStartAddr[2],
					pucStartAddr[3],
					pucStartAddr[4],
					pucStartAddr[5],
					pucStartAddr[6],
					pucStartAddr[7],
					pucStartAddr[8],
					pucStartAddr[9],
					pucStartAddr[10],
					pucStartAddr[11],
					pucStartAddr[12], pucStartAddr[13], pucStartAddr[14], pucStartAddr[15]);
		}

		pucStartAddr += 16;
		u4Length -= 16;
		u4Line += 16;
	}			/* u4Length */
}


void
priv_driver_get_chip_config_4(PUINT_32 pu4StartAddr, UINT_32 u4Length, UINT_32 u4Line, int i4TotalLen,
			      INT_32 i4BytesWritten, char *pcCommand)
{
	while (u4Length >= 16) {
		if (i4TotalLen > i4BytesWritten) {
			i4BytesWritten +=
			    snprintf(pcCommand +
				     i4BytesWritten,
				     i4TotalLen -
				     i4BytesWritten,
				     "%04lx %08lx %08lx %08lx %08lx\n",
				     u4Line, pu4StartAddr[0], pu4StartAddr[1], pu4StartAddr[2], pu4StartAddr[3]);
		}

		pu4StartAddr += 4;
		u4Length -= 16;
		u4Line += 4;
	}			/* u4Length */
}

int priv_driver_get_chip_config(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	INT_32 i4BytesWritten = 0;
	UINT_32 u4BufLen = 0;
	UINT_32 u2MsgSize = 0;
	UINT_32 u4CmdLen = 0;
	UINT_32 u4PrefixLen = 0;
	/* INT_32 i4Argc = 0; */
	/* PCHAR  apcArgv[WLAN_CFG_ARGV_MAX]; */

	PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T rChipConfigInfo;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	/* wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv); */
	/* DBGLOG(REQ, LOUD,("argc is %i\n",i4Argc)); */

	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);
	u4PrefixLen = kalStrLen(CMD_GET_CHIP) + 1 /*space */;

	/* if(i4Argc >= 2) { */
	if (u4CmdLen > u4PrefixLen) {
		rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_ASCII;
		/* rChipConfigInfo.u2MsgSize = kalStrnLen(apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
		rChipConfigInfo.u2MsgSize = u4CmdLen - u4PrefixLen;
		/* kalStrnCpy(rChipConfigInfo.aucCmd,apcArgv[1],CHIP_CONFIG_RESP_SIZE); */
		kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand + u4PrefixLen, CHIP_CONFIG_RESP_SIZE);
		rStatus = kalIoctl(prGlueInfo,
				   wlanoidQueryChipConfig,
				   &rChipConfigInfo, sizeof(rChipConfigInfo), TRUE, TRUE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, INFO, "%s: kalIoctl ret=%d\n", __func__, rStatus);
			return -1;
		}

		/* Check respType */
		u2MsgSize = rChipConfigInfo.u2MsgSize;
		DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__, rChipConfigInfo.ucRespType);
		DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__, rChipConfigInfo.u2MsgSize);

		if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
			DBGLOG(REQ, INFO, "%s: u2MsgSize error ret=%u\n", __func__, rChipConfigInfo.u2MsgSize);
			return -1;
		}

		if (u2MsgSize > 0) {

			if (rChipConfigInfo.ucRespType == CHIP_CONFIG_TYPE_ASCII) {
				i4BytesWritten =
				    snprintf(pcCommand + i4BytesWritten, i4TotalLen, "%s", rChipConfigInfo.aucCmd);
			} else {
				UINT_32 u4Length;
				UINT_32 u4Line;

				if (rChipConfigInfo.ucRespType == CHIP_CONFIG_TYPE_MEM8) {
					PUINT_8 pucStartAddr = NULL;

					pucStartAddr = (PUINT_8) rChipConfigInfo.aucCmd;
					/* align 16 bytes because one print line is 16 bytes */
					u4Length = (((u2MsgSize + 15) >> 4)) << 4;
					u4Line = 0;
					priv_driver_get_chip_config_16(pucStartAddr, u4Length, u4Line, i4TotalLen,
									i4BytesWritten, pcCommand);
				} else {
					PUINT_32 pu4StartAddr = NULL;

					pu4StartAddr = (PUINT_32) rChipConfigInfo.aucCmd;
					/* align 16 bytes because one print line is 16 bytes */
					u4Length = (((u2MsgSize + 15) >> 4)) << 4;
					u4Line = 0;

					if (IS_ALIGN_4((ULONG) pu4StartAddr)) {
						priv_driver_get_chip_config_4(pu4StartAddr, u4Length, u4Line,
									      i4TotalLen, i4BytesWritten, pcCommand);
					} else {
						DBGLOG(REQ, INFO,
							"%s: rChipConfigInfo.aucCmd is not 4 bytes alignment %p\n",
							__func__, rChipConfigInfo.aucCmd);
					}
				}	/* ChipConfigInfo.ucRespType */
			}
		}
		/* u2MsgSize > 0 */
		DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	}
	/* i4Argc */
	return i4BytesWritten;

}				/* priv_driver_get_chip_config  */



int priv_driver_set_ap_start(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{

	PARAM_CUSTOM_P2P_SET_STRUCT_T rSetP2P;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	UINT_32 u4Ret;
	INT_32 i4ArgNum = 2;


	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &(rSetP2P.u4Mode));
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ap-start error (u4Enable) u4Ret=%d\n", u4Ret);

		if (rSetP2P.u4Mode >= RUNNING_P2P_MODE_NUM) {
			rSetP2P.u4Mode = 0;
			rSetP2P.u4Enable = 0;
		} else
			rSetP2P.u4Enable = 1;

		set_p2p_mode_handler(prNetDev, rSetP2P);
	}

	return 0;
}

int priv_driver_get_linkspeed(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	UINT_32 u4Rate = 0;
	UINT_32 u4LinkSpeed = 0;
	INT_32 i4BytesWritten = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	if (!netif_carrier_ok(prNetDev))
		return -1;

	rStatus = kalIoctl(prGlueInfo, wlanoidQueryLinkSpeed, &u4Rate, sizeof(u4Rate), TRUE, TRUE, TRUE, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	u4LinkSpeed = u4Rate * 100;
	i4BytesWritten = snprintf(pcCommand, i4TotalLen, "LinkSpeed %u", (unsigned int)u4LinkSpeed);
	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	return i4BytesWritten;

}				/* priv_driver_get_linkspeed */

int priv_driver_set_band(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_ADAPTER_T prAdapter = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4Argc = 0;
	UINT_32 ucBand = 0;
	UINT_8 ucBssIndex;
	ENUM_BAND_T eBand = BAND_NULL;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	INT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	if (i4Argc >= 2) {
		/* ucBand = kalStrtoul(apcArgv[1], NULL, 0); */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &ucBand);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucBand error u4Ret=%d\n", u4Ret);

		ucBssIndex = wlanGetAisBssIndex(prGlueInfo->prAdapter);
		eBand = BAND_NULL;
		if (ucBand == CMD_BAND_5G)
			eBand = BAND_5G;
		else if (ucBand == CMD_BAND_2G)
			eBand = BAND_2G4;
		prAdapter->aePreferBand[ucBssIndex] = eBand;
		/* XXX call wlanSetPreferBandByNetwork directly in different thread */
		/* wlanSetPreferBandByNetwork (prAdapter, eBand, ucBssIndex); */
	}

	return 0;
}

int priv_driver_set_txpower(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	P_SET_TXPWR_CTRL_T prTxpwr;
	UINT_16 i;
	INT_32 u4Ret = 0;
	INT_32 ai4Setting[4];

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prTxpwr = &prGlueInfo->rTxPwr;

	if (i4Argc >= 3 && i4Argc <= 5) {
		for (i = 0; i < (i4Argc - 1); i++) {
			/* ai4Setting[i] = kalStrtol(apcArgv[i + 1], NULL, 0); */
			u4Ret = kalkStrtos32(apcArgv[i + 1], 0, &(ai4Setting[i]));
			if (u4Ret)
				DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
			/* printk("PeiHsuan setting[%d] = %d\n", i, setting[i]); */
		}
	} else {
		DBGLOG(REQ, INFO, "set_txpower wrong argc : %d\n", i4Argc);
		return -1;
	}

	/*
	 *  ai4Setting[0]
	 *  0 : Set TX power offset for specific network
	 *  1 : Set TX power offset policy when multiple networks are in the same channel
	 *  2 : Set TX power limit for specific channel in 2.4GHz band
	 *  3 : Set TX power limit of specific sub-band in 5GHz band
	 *  4 : Enable or reset setting
	 */
	if (ai4Setting[0] == 0 && (i4Argc - 1) == 4 /* argc num */) {
		/* ai4Setting[1] : 0 (All networks), 1 (legacy STA), 2 (Hotspot AP), 3 (P2P), 4 (BT over Wi-Fi) */
		/* ai4Setting[2] : 0 (All bands),1 (2.4G), 2 (5G) */
		/* ai4Setting[3] : -30 ~ 20 in unit of 0.5dBm (default: 0) */
		if (ai4Setting[1] == 1 || ai4Setting[1] == 0) {
			if (ai4Setting[2] == 0 || ai4Setting[2] == 1)
				prTxpwr->c2GLegacyStaPwrOffset = ai4Setting[3];
			if (ai4Setting[2] == 0 || ai4Setting[2] == 2)
				prTxpwr->c5GLegacyStaPwrOffset = ai4Setting[3];
		}
		if (ai4Setting[1] == 2 || ai4Setting[1] == 0) {
			if (ai4Setting[2] == 0 || ai4Setting[2] == 1)
				prTxpwr->c2GHotspotPwrOffset = ai4Setting[3];
			if (ai4Setting[2] == 0 || ai4Setting[2] == 2)
				prTxpwr->c5GHotspotPwrOffset = ai4Setting[3];
		}
		if (ai4Setting[1] == 3 || ai4Setting[1] == 0) {
			if (ai4Setting[2] == 0 || ai4Setting[2] == 1)
				prTxpwr->c2GP2pPwrOffset = ai4Setting[3];
			if (ai4Setting[2] == 0 || ai4Setting[2] == 2)
				prTxpwr->c5GP2pPwrOffset = ai4Setting[3];
		}
		if (ai4Setting[1] == 4 || ai4Setting[1] == 0) {
			if (ai4Setting[2] == 0 || ai4Setting[2] == 1)
				prTxpwr->c2GBowPwrOffset = ai4Setting[3];
			if (ai4Setting[2] == 0 || ai4Setting[2] == 2)
				prTxpwr->c5GBowPwrOffset = ai4Setting[3];
		}
	} else if (ai4Setting[0] == 1 && (i4Argc - 1) == 2) {
		/* ai4Setting[1] : 0 (highest power is used) (default), 1 (lowest power is used) */
		prTxpwr->ucConcurrencePolicy = ai4Setting[1];
	} else if (ai4Setting[0] == 2 && (i4Argc - 1) == 3) {
		/* ai4Setting[1] : 0 (all channels in 2.4G), 1~14 */
		/* ai4Setting[2] : 10 ~ 46 in unit of 0.5dBm (default: 46) */
		if (ai4Setting[1] == 0) {
			for (i = 0; i < 14; i++)
				prTxpwr->acTxPwrLimit2G[i] = ai4Setting[2];
		} else if (ai4Setting[1] <= 14)
			prTxpwr->acTxPwrLimit2G[ai4Setting[1] - 1] = ai4Setting[2];
	} else if (ai4Setting[0] == 3 && (i4Argc - 1) == 3) {
		/* ai4Setting[1] : 0 (all sub-bands in 5G),
		 *  1 (5000 ~ 5250MHz),
		 *  2 (5255 ~ 5350MHz),
		 *  3 (5355 ~ 5725MHz),
		 *  4 (5730 ~ 5825MHz)
		 */
		/* ai4Setting[2] : 10 ~ 46 in unit of 0.5dBm (default: 46) */
		if (ai4Setting[1] == 0) {
			for (i = 0; i < 4; i++)
				prTxpwr->acTxPwrLimit5G[i] = ai4Setting[2];
		} else if (ai4Setting[1] <= 4)
			prTxpwr->acTxPwrLimit5G[ai4Setting[1] - 1] = ai4Setting[2];
	} else if (ai4Setting[0] == 4 && (i4Argc - 1) == 2) {
		/* ai4Setting[1] : 1 (enable), 0 (reset and disable) */
		if (ai4Setting[1] == 0)
			wlanDefTxPowerCfg(prGlueInfo->prAdapter);

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetTxPower, prTxpwr, sizeof(SET_TXPWR_CTRL_T), FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	} else
		return -EFAULT;

	return 0;
}

int priv_driver_set_country(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{

	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_8 aucCountry[2];

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* command like "COUNTRY US", "COUNTRY EU" and "COUNTRY JP" */
		aucCountry[0] = apcArgv[1][0];
		aucCountry[1] = apcArgv[1][1];

		rStatus = kalIoctl(prGlueInfo, wlanoidSetCountryCode, &aucCountry[0], 2, FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;
	}
	return 0;
}

int priv_driver_set_miracast(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{

	P_ADAPTER_T prAdapter = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 i4BytesWritten = 0;
	/* WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS; */
	/* UINT_32 u4BufLen = 0; */
	INT_32 i4Argc = 0;
	UINT_32 ucMode = 0;
	P_WFD_CFG_SETTINGS_T prWfdCfgSettings = (P_WFD_CFG_SETTINGS_T) NULL;
	P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T prMsgWfdCfgUpdate = (P_MSG_WFD_CONFIG_SETTINGS_CHANGED_T) NULL;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	INT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	prAdapter = prGlueInfo->prAdapter;
	if (i4Argc >= 2) {
		/* ucMode = kalStrtoul(apcArgv[1], NULL, 0); */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &ucMode);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucMode error u4Ret=%d\n", u4Ret);

		if (g_ucMiracastMode == (UINT_8) ucMode) {
			/* XXX: continue or skip */
			/* XXX: continue or skip */
		}

		g_ucMiracastMode = (UINT_8) ucMode;
		prMsgWfdCfgUpdate = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_WFD_CONFIG_SETTINGS_CHANGED_T));

		if (prMsgWfdCfgUpdate != NULL) {

			prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
			prMsgWfdCfgUpdate->rMsgHdr.eMsgId = MID_MNY_P2P_WFD_CFG_UPDATE;
			prMsgWfdCfgUpdate->prWfdCfgSettings = prWfdCfgSettings;

			if (ucMode == MIRACAST_MODE_OFF) {
				prWfdCfgSettings->ucWfdEnable = 0;
				snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mira 0");
			} else if (ucMode == MIRACAST_MODE_SOURCE) {
				prWfdCfgSettings->ucWfdEnable = 1;
				snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mira 1");
			} else if (ucMode == MIRACAST_MODE_SINK) {
				prWfdCfgSettings->ucWfdEnable = 2;
				snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mira 2");
			} else {
				prWfdCfgSettings->ucWfdEnable = 0;
				snprintf(pcCommand, i4TotalLen, CMD_SET_CHIP " mira 0");
			}

			mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prMsgWfdCfgUpdate, MSG_SEND_METHOD_BUF);

			priv_driver_set_chip_config(prNetDev, pcCommand, i4TotalLen);
		} /* prMsgWfdCfgUpdate */
		else {
			ASSERT(FALSE);
			i4BytesWritten = -1;
		}
	}

	/* i4Argc */
	return i4BytesWritten;
}

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
/*
 * Memo
 * 00 : Reset All Cal Data in Driver
 * 01 : Trigger All Cal Function
 * 02 : Get Thermal Temp from FW
 * 03 : Get Cal Data Size from FW
 * 04 : Get Cal Data from FW (Rom)
 * 05 : Get Cal Data from FW (Ram)
 * 06 : Print Cal Data in Driver (Rom)
 * 07 : Print Cal Data in Driver (Ram)
 * 08 : Print Cal Data in FW (Rom)
 * 09 : Print Cal Data in FW (Ram)
 * 10 : Send Cal Data to FW (Rom)
 * 11 : Send Cal Data to FW (Ram)
 */
static int priv_driver_set_calbackup_test_drv_fw(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	UINT_32 u4Ret, u4GetInput;
	INT_32 i4ArgNum = 2;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(RFTEST, INFO, "%s\r\n", __func__);

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= i4ArgNum) {
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4GetInput);
		if (u4Ret)
			DBGLOG(RFTEST, INFO, "priv_driver_set_calbackup_test_drv_fw Parsing Fail\n");

		if (u4GetInput == 0) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#0 : Reset All Cal Data in Driver.\n");
			/* (New Flow 20160720) Step 0 : Reset All Cal Data Structure */
			memset(&g_rBackupCalDataAllV2, 1, sizeof(RLM_CAL_RESULT_ALL_V2_T));
			g_rBackupCalDataAllV2.u4MagicNum1 = 6632;
			g_rBackupCalDataAllV2.u4MagicNum2 = 6632;
		} else if (u4GetInput == 1) {
			DBGLOG(RFTEST, INFO, "CMD#1 : Trigger FW Do All Cal.\n");
			/* Step 1 : Trigger All Cal Function */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 1, 2, 0);
			DBGLOG(RFTEST, INFO, "Trigger FW Do All Cal, rStatus = 0x%08x\n", rStatus);
		} else if (u4GetInput == 2) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#2 : Get Thermal Temp from FW.\n");
			/* (New Flow 20160720) Step 2 : Get Thermal Temp from FW */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 0, 0, 0);
			DBGLOG(RFTEST, INFO, "Get Thermal Temp from FW, rStatus = 0x%08x\n", rStatus);

		} else if (u4GetInput == 3) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#3 : Get Cal Data Size from FW.\n");
			/* (New Flow 20160720) Step 3 : Get Cal Data Size from FW */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 0, 1, 0);
			DBGLOG(RFTEST, INFO, "Get Rom Cal Data Size, rStatus = 0x%08x\n", rStatus);

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 0, 1, 1);
			DBGLOG(RFTEST, INFO, "Get Ram Cal Data Size, rStatus = 0x%08x\n", rStatus);

		} else if (u4GetInput == 4) {
#if 1
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#4 : Print Cal Data in FW (Ram) (Part 1 - [0]~[3327]).\n");
			/* Debug Use : Print Cal Data in FW (Ram) */
			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 4, 6, 1);
			DBGLOG(RFTEST, INFO, "Print Cal Data in FW (Ram), rStatus = 0x%08x\n", rStatus);
#else		/* For Temp Use this Index */
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#4 : Get Cal Data from FW (Rom). Start!!!!!!!!!!!\n");
			DBGLOG(RFTEST, INFO, "Thermal Temp = %d\n", g_rBackupCalDataAllV2.u4ThermalInfo);
			DBGLOG(RFTEST, INFO, "Total Length (Rom) = %d\n",
				g_rBackupCalDataAllV2.u4ValidRomCalDataLength);
			/* (New Flow 20160720) Step 3 : Get Cal Data from FW */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 2, 4, 0);
			DBGLOG(RFTEST, INFO, "Get Cal Data from FW (Rom), rStatus = 0x%08x\n", rStatus);
#endif
		} else if (u4GetInput == 5) {
#if 1
			DBGLOG(RFTEST, INFO,
				"(New Flow) CMD#5 : Print RAM Cal Data in Driver (Part 1 - [0]~[3327]).\n");
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			/* RFTEST_INFO_LOGDUMP32(&(g_rBackupCalDataAllV2.au4RamCalData[0]), 3328*sizeof(UINT_32)); */
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			DBGLOG(RFTEST, INFO, "Dumped Ram Cal Data Szie : %d bytes\n", 3328*sizeof(UINT_32));
			DBGLOG(RFTEST, INFO, "Total Ram Cal Data Szie : %d bytes\n",
				g_rBackupCalDataAllV2.u4ValidRamCalDataLength);
			DBGLOG(RFTEST, INFO, "==================================================================\n");
#else		/* For Temp Use this Index */
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#5 : Get Cal Data from FW (Ram). Start!!!!!!!!!!!\n");
			DBGLOG(RFTEST, INFO, "Thermal Temp = %d\n", g_rBackupCalDataAllV2.u4ThermalInfo);
			DBGLOG(RFTEST, INFO, "Total Length (Ram) = %d\n",
				g_rBackupCalDataAllV2.u4ValidRamCalDataLength);
			/* (New Flow 20160720) Step 3 : Get Cal Data from FW */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 2, 4, 1);
			DBGLOG(RFTEST, INFO, "Get Cal Data from FW (Ram), rStatus = 0x%08x\n", rStatus);
#endif
		} else if (u4GetInput == 6) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#6 : Print ROM Cal Data in Driver.\n");
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			/* RFTEST_INFO_LOGDUMP32(&(g_rBackupCalDataAllV2.au4RomCalData[0]), */
			/* g_rBackupCalDataAllV2.u4ValidRomCalDataLength); */
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			DBGLOG(RFTEST, INFO, "Total Rom Cal Data Szie : %d bytes\n",
				g_rBackupCalDataAllV2.u4ValidRomCalDataLength);
			DBGLOG(RFTEST, INFO, "==================================================================\n");
		} else if (u4GetInput == 7) {
			DBGLOG(RFTEST, INFO,
				"(New Flow) CMD#7 : Print RAM Cal Data in Driver (Part 2 - [3328]~[6662]).\n");
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			/* RFTEST_INFO_LOGDUMP32(&(g_rBackupCalDataAllV2.au4RamCalData[3328]), */
			/*(g_rBackupCalDataAllV2.u4ValidRamCalDataLength - 3328*sizeof(UINT_32))); */
			DBGLOG(RFTEST, INFO, "==================================================================\n");
			DBGLOG(RFTEST, INFO, "Dumped Ram Cal Data Szie : %d bytes\n",
				(g_rBackupCalDataAllV2.u4ValidRamCalDataLength - 3328*sizeof(UINT_32)));
			DBGLOG(RFTEST, INFO, "Total Ram Cal Data Szie : %d bytes\n",
				g_rBackupCalDataAllV2.u4ValidRamCalDataLength);
			DBGLOG(RFTEST, INFO, "==================================================================\n");
		} else if (u4GetInput == 8) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#8 : Print Cal Data in FW (Rom).\n");
			/* Debug Use : Print Cal Data in FW (Rom) */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 4, 6, 0);
			DBGLOG(RFTEST, INFO, "Print Cal Data in FW (Rom), rStatus = 0x%08x\n", rStatus);

		} else if (u4GetInput == 9) {
			DBGLOG(RFTEST, INFO,
				"(New Flow) CMD#9 : Print Cal Data in FW (Ram) (Part 2 - [3328]~[6662]).\n");
			/* Debug Use : Print Cal Data in FW (Ram) */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 4, 6, 2);
			DBGLOG(RFTEST, INFO, "Print Cal Data in FW (Ram), rStatus = 0x%08x\n", rStatus);

		} else if (u4GetInput == 10) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#10 : Send Cal Data to FW (Rom).\n");
			/* Send Cal Data to FW (Rom) */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 3, 5, 0);
			DBGLOG(RFTEST, INFO, "Send Cal Data to FW (Rom), rStatus = 0x%08x\n", rStatus);

		} else if (u4GetInput == 11) {
			DBGLOG(RFTEST, INFO, "(New Flow) CMD#11 : Send Cal Data to FW (Ram).\n");
			/* Send Cal Data to FW (Ram) */

			rStatus = rlmCalBackup(prGlueInfo->prAdapter, 3, 5, 1);
			DBGLOG(RFTEST, INFO, "Send Cal Data to FW (Ram), rStatus = 0x%08x\n", rStatus);

		}
	}

	return i4BytesWritten;
}				/* priv_driver_set_calbackup_test_drv_fw */
#endif

#if CFG_WOW_SUPPORT
static int priv_driver_set_wow_enable(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_WOW_CTRL_T pWOW_CTRL = NULL;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	UINT_32 u4Ret = 0;
	UINT_32 Enable = 0;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	u4Ret = kalkStrtou32(apcArgv[1], 0, &Enable);

	if (u4Ret)
		DBGLOG(REQ, LOUD, "parse bEnable error u4Ret=%d\n", u4Ret);

	pWOW_CTRL->fgEnable = Enable;

	DBGLOG(INIT, INFO, "CMD set_wow_enable = %d\n", pWOW_CTRL->fgEnable);
	DBGLOG(INIT, INFO, "Scenario ID %d\n", pWOW_CTRL->ucScenarioId);
	DBGLOG(INIT, INFO, "ucBlockCount %d\n", pWOW_CTRL->ucBlockCount);
	DBGLOG(INIT, INFO, "interface %d\n", pWOW_CTRL->astWakeHif[0].ucWakeupHif);
	DBGLOG(INIT, INFO, "gpio_pin %d\n", pWOW_CTRL->astWakeHif[0].ucGpioPin);
	DBGLOG(INIT, INFO, "gpio_level 0x%x\n", pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
	DBGLOG(INIT, INFO, "gpio_timer %d\n", pWOW_CTRL->astWakeHif[0].u4GpioInterval);
	kalWowProcess(prGlueInfo, pWOW_CTRL->fgEnable);

	return 0;
}

static int priv_driver_set_wow_par(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_WOW_CTRL_T pWOW_CTRL = NULL;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	INT_32 u4Ret = 0;
	UINT_8	ucWakeupHif = 0, GpioPin = 0, ucGpioLevel = 0, ucBlockCount, ucScenario = 0;
	UINT_32 u4GpioTimer = 0;

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	pWOW_CTRL = &prGlueInfo->prAdapter->rWowCtrl;

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc > 3) {

		u4Ret = kalkStrtou8(apcArgv[1], 0, &ucWakeupHif);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucWakeupHif error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->astWakeHif[0].ucWakeupHif = ucWakeupHif;

		u4Ret = kalkStrtou8(apcArgv[2], 0, &GpioPin);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse GpioPin error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->astWakeHif[0].ucGpioPin = GpioPin;

		u4Ret = kalkStrtou8(apcArgv[3], 0, &ucGpioLevel);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse Gpio level error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->astWakeHif[0].ucTriggerLvl = ucGpioLevel;

		u4Ret = kalkStrtou32(apcArgv[4], 0, &u4GpioTimer);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4GpioTimer error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->astWakeHif[0].u4GpioInterval = u4GpioTimer;

		u4Ret = kalkStrtou8(apcArgv[5], 0, &ucScenario);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucScenario error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->ucScenarioId = ucScenario;

		u4Ret = kalkStrtou8(apcArgv[6], 0, &ucBlockCount);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse ucBlockCnt error u4Ret=%d\n", u4Ret);
		pWOW_CTRL->ucBlockCount = ucBlockCount;

		DBGLOG(INIT, INFO, "gpio_scenario%d\n", pWOW_CTRL->ucScenarioId);
		DBGLOG(INIT, INFO, "interface %d\n", pWOW_CTRL->astWakeHif[0].ucWakeupHif);
		DBGLOG(INIT, INFO, "gpio_pin %d\n", pWOW_CTRL->astWakeHif[0].ucGpioPin);
		DBGLOG(INIT, INFO, "gpio_level %d\n", pWOW_CTRL->astWakeHif[0].ucTriggerLvl);
		DBGLOG(INIT, INFO, "gpio_timer %d\n", pWOW_CTRL->astWakeHif[0].u4GpioInterval);

		return 0;
	} else
		return -1;


}
#endif


int priv_driver_set_suspend_mode(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	BOOLEAN fgEnable;
	UINT_32 u4Enable;
	INT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(REQ, LOUD, "argc is %i\n", i4Argc);

	if (i4Argc >= 2) {
		/* fgEnable = (kalStrtoul(apcArgv[1], NULL, 0) == 1) ? TRUE : FALSE; */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Enable);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse u4Enable error u4Ret=%d\n", u4Ret);
		if (u4Enable == 1)
			fgEnable = TRUE;
		else
			fgEnable = FALSE;

		if (prGlueInfo->fgIsInSuspendMode == fgEnable) {
			DBGLOG(REQ, TRACE, "%s: Already in suspend mode [%u], SKIP!\n", __func__, fgEnable);
			return 0;
		}

		DBGLOG(REQ, TRACE, "Set suspend mode [%u]\n", fgEnable);

		prGlueInfo->fgIsInSuspendMode = fgEnable;

		wlanSetSuspendMode(prGlueInfo, fgEnable);
		p2pSetSuspendMode(prGlueInfo, fgEnable);
	}

	return 0;
}

#if CFG_SUPPORT_SNIFFER
int priv_driver_set_monitor(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4Argc = 0;
	INT_32 i4BytesWritten = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX];
	PARAM_CUSTOM_MONITOR_SET_STRUCT_T rMonitorSetInfo;
	UINT_8 ucEnable = 0;
	UINT_8 ucPriChannel = 0;
	UINT_8 ucChannelWidth = 0;
	UINT_8 ucExt = 0;
	UINT_8 ucSco = 0;
	UINT_8 ucChannelS1 = 0;
	UINT_8 ucChannelS2 = 0;
	BOOLEAN fgIsLegalChannel = FALSE;
	BOOLEAN fgError = FALSE;
	BOOLEAN fgEnable = FALSE;
	ENUM_BAND_T eBand = BAND_NULL;
	UINT_32 u4Parse = 0;
	INT_32 u4Ret = 0;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 5) {
		/* ucEnable = (UINT_8) (kalStrtoul(apcArgv[1], NULL, 0));
		 *  ucPriChannel = (UINT_8) (kalStrtoul(apcArgv[2], NULL, 0));
		 *  ucChannelWidth = (UINT_8) (kalStrtoul(apcArgv[3], NULL, 0));
		 *  ucExt = (UINT_8) (kalStrtoul(apcArgv[4], NULL, 0));
		 */
		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
		ucEnable = (UINT_8) u4Parse;
		u4Ret = kalkStrtou32(apcArgv[2], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
		ucPriChannel = (UINT_8) u4Parse;
		u4Ret = kalkStrtou32(apcArgv[3], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
		ucChannelWidth = (UINT_8) u4Parse;
		u4Ret = kalkStrtou32(apcArgv[4], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);
		ucExt = (UINT_8) u4Parse;

		eBand = (ucPriChannel <= 14) ? BAND_2G4 : BAND_5G;
		fgIsLegalChannel = rlmDomainIsLegalChannel(prAdapter, eBand, ucPriChannel);

		if (fgIsLegalChannel == FALSE) {
			i4BytesWritten = snprintf(pcCommand, i4TotalLen, "Illegal primary channel %d", ucPriChannel);
			return i4BytesWritten;
		}

		switch (ucChannelWidth) {
		case 160:
			ucChannelWidth = (UINT_8) CW_160MHZ;
			ucSco = (UINT_8) CHNL_EXT_SCN;

			if (ucPriChannel >= 36 && ucPriChannel <= 64)
				ucChannelS2 = 50;
			else if (ucPriChannel >= 100 && ucPriChannel <= 128)
				ucChannelS2 = 114;
			else
				fgError = TRUE;
			break;

		case 80:
			ucChannelWidth = (UINT_8) CW_80MHZ;
			ucSco = (UINT_8) CHNL_EXT_SCN;

			if (ucPriChannel >= 36 && ucPriChannel <= 48)
				ucChannelS1 = 42;
			else if (ucPriChannel >= 52 && ucPriChannel <= 64)
				ucChannelS1 = 58;
			else if (ucPriChannel >= 100 && ucPriChannel <= 112)
				ucChannelS1 = 106;
			else if (ucPriChannel >= 116 && ucPriChannel <= 128)
				ucChannelS1 = 122;
			else if (ucPriChannel >= 132 && ucPriChannel <= 144)
				ucChannelS1 = 138;
			else if (ucPriChannel >= 149 && ucPriChannel <= 161)
				ucChannelS1 = 155;
			else
				fgError = TRUE;
			break;

		case 40:
			ucChannelWidth = (UINT_8) CW_20_40MHZ;
			ucSco = (ucExt) ? (UINT_8) CHNL_EXT_SCA : (UINT_8) CHNL_EXT_SCB;
			break;

		case 20:
			ucChannelWidth = (UINT_8) CW_20_40MHZ;
			ucSco = (UINT_8) CHNL_EXT_SCN;
			break;

		default:
			fgError = TRUE;
			break;
		}

		if (fgError) {
			i4BytesWritten =
			    snprintf(pcCommand, i4TotalLen, "Invalid primary channel %d with bandwidth %d",
				     ucPriChannel, ucChannelWidth);
			return i4BytesWritten;
		}

		fgEnable = (ucEnable) ? TRUE : FALSE;

		if (prGlueInfo->fgIsEnableMon != fgEnable) {
			prGlueInfo->fgIsEnableMon = fgEnable;
			schedule_work(&prGlueInfo->monWork);
		}

		kalMemZero(&rMonitorSetInfo, sizeof(rMonitorSetInfo));

		rMonitorSetInfo.ucEnable = ucEnable;
		rMonitorSetInfo.ucPriChannel = ucPriChannel;
		rMonitorSetInfo.ucSco = ucSco;
		rMonitorSetInfo.ucChannelWidth = ucChannelWidth;
		rMonitorSetInfo.ucChannelS1 = ucChannelS1;
		rMonitorSetInfo.ucChannelS2 = ucChannelS2;

		rStatus = kalIoctl(prGlueInfo,
				   wlanoidSetMonitor,
				   &rMonitorSetInfo, sizeof(rMonitorSetInfo), FALSE, FALSE, TRUE, &u4BufLen);

		i4BytesWritten =
		    snprintf(pcCommand, i4TotalLen, "set monitor config %s",
			     (rStatus == WLAN_STATUS_SUCCESS) ? "success" : "fail");

		return i4BytesWritten;
	}

	i4BytesWritten = snprintf(pcCommand, i4TotalLen, "monitor [Enable][PriChannel][ChannelWidth][Sco]");

	return i4BytesWritten;
}
#endif

static int priv_driver_get_version(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter;
	INT_32 i4BytesWritten = 0;
	UINT_32 u4Offset = 0;
	P_WIFI_VER_INFO_T prVerInfo;
	tailer_format_t *prTailer;
	UINT_8 aucBuf[32], aucDate[32];

	ASSERT(prNetDev);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	prAdapter = prGlueInfo->prAdapter;
	prVerInfo = &prAdapter->rVerInfo;

	wlanPrintVersion(prAdapter);

	kalStrnCpy(aucBuf, prVerInfo->aucFwBranchInfo, 4);
	aucBuf[4] = '\0';
	kalStrnCpy(aucDate, prVerInfo->aucFwDateCode, 16);
	aucDate[16] = '\0';
	u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
	"\nN9 FW version %s-%u.%u.%u[DEC] (%s)\n", aucBuf, (prVerInfo->u2FwOwnVersion >> 8),
	(prVerInfo->u2FwOwnVersion & BITS(0, 7)), prVerInfo->ucFwBuildNumber, aucDate);
#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	if (prVerInfo->fgIsN9CompressedFW) {
		tailer_format_t_2 *prTailer;

		prTailer = &prVerInfo->rN9Compressedtailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"N9  tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);
	} else {
		prTailer = &prVerInfo->rN9tailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"N9  tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);
	}
	if (prVerInfo->fgIsCR4CompressedFW) {
		tailer_format_t_2 *prTailer;

		prTailer = &prVerInfo->rCR4Compressedtailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"CR4 tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);
	} else {
		prTailer = &prVerInfo->rCR4tailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"CR4 tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);
	}
#else
		prTailer = &prVerInfo->rN9tailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"N9  tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);

		prTailer = &prVerInfo->rCR4tailer;
		kalMemCopy(aucBuf, prTailer->ram_version, 10);
		aucBuf[10] = '\0';
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"CR4 tailer version %s (%s) info %u:E%u\n",
		aucBuf, prTailer->ram_built_date, prTailer->chip_info,
		prTailer->eco_code + 1);
#endif

	if (!prVerInfo->fgPatchIsDlByDrv) {
		u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
			"MCU patch is not downloaded by wlan driver, read patch info\n");
		wlanGetPatchInfo(prAdapter);
	}

	kalStrnCpy(aucBuf, prVerInfo->rPatchHeader.aucPlatform, 4);
	aucBuf[4] = '\0';
	kalStrnCpy(aucDate, prVerInfo->rPatchHeader.aucBuildDate, 16);
	aucDate[16] = '\0';
	u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"Patch platform %s version 0x%04X %s\n",
		aucBuf, prVerInfo->rPatchHeader.u4PatchVersion, aucDate);

	u4Offset += snprintf(pcCommand + u4Offset, i4TotalLen - u4Offset,
		"Drv version %u.%u[DEC]", (prVerInfo->u2FwPeerVersion >> 8),
		(prVerInfo->u2FwPeerVersion & BITS(0, 7)));

	i4BytesWritten = (INT_32)u4Offset;

	return i4BytesWritten;
}

static int priv_driver_get_cnm(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	P_PARAM_GET_CNM_T prCnmInfo = NULL;

	ENUM_DBDC_BN_T	eDbdcIdx, eDbdcIdxMax;
	UINT_8			ucBssIdx;

	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	prCnmInfo = (P_PARAM_GET_CNM_T)kalMemAlloc(sizeof(PARAM_GET_CNM_T), VIR_MEM_TYPE);
	if (prCnmInfo == NULL)
		return -1;

	kalMemZero(prCnmInfo, sizeof(PARAM_GET_CNM_T));

	rStatus = kalIoctl(prGlueInfo,
					wlanoidQueryCnm,
					prCnmInfo,
					sizeof(PARAM_GET_CNM_T),
					TRUE,
					TRUE,
					TRUE,
					&u4BufLen);

	DBGLOG(REQ, INFO, "%s: command result is %s\n", __func__, pcCommand);
	if (rStatus != WLAN_STATUS_SUCCESS)
		return -1;

	i4BytesWritten += snprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
								"\n[CNM Info]\n");
	i4BytesWritten += snprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
								"DBDC Mode : %s\n\n",
								(prCnmInfo->fgIsDbdcEnable)?"Enable":"Disable");

	eDbdcIdxMax = (prCnmInfo->fgIsDbdcEnable)?ENUM_BAND_NUM:ENUM_BAND_1;
	for (eDbdcIdx = ENUM_BAND_0; eDbdcIdx < eDbdcIdxMax; eDbdcIdx++) {
		i4BytesWritten += snprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
								"Band %u OPCH %d [%u, %u, %u]\n",
								eDbdcIdx,
								prCnmInfo->ucOpChNum[eDbdcIdx],
								prCnmInfo->ucChList[eDbdcIdx][0],
								prCnmInfo->ucChList[eDbdcIdx][1],
								prCnmInfo->ucChList[eDbdcIdx][2]);
	}
	i4BytesWritten += snprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
								"\n");

	for (ucBssIdx = BSSID_0; ucBssIdx < (BSSID_NUM+1); ucBssIdx++) {

		i4BytesWritten += snprintf(pcCommand + i4BytesWritten, i4TotalLen - i4BytesWritten,
								"BSS%u Inuse%u Act%u ConnStat%u [CH %3u][DBDC b%u][WMM%u b%u][OMAC%u b%u]\n",
								ucBssIdx,
								prCnmInfo->ucInuse[ucBssIdx],
								prCnmInfo->ucActive[ucBssIdx],
								prCnmInfo->ucConnectState[ucBssIdx],
								prCnmInfo->ucBssCh[ucBssIdx],
								prCnmInfo->ucBssDBDCBand[ucBssIdx],
								prCnmInfo->ucBssWmmSet[ucBssIdx],
								prCnmInfo->ucBssWmmDBDCBand[ucBssIdx],
								prCnmInfo->ucBssOMACSet[ucBssIdx],
								prCnmInfo->ucBssOMACDBDCBand[ucBssIdx]
								);
	}

	kalMemFree(prCnmInfo, VIR_MEM_TYPE, sizeof(PARAM_GET_CNM_T));
	return i4BytesWritten;
}

#if CFG_SUPPORT_DBDC
int priv_driver_set_dbdc(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	UINT_32 u4BufLen = 0;
	INT_32 i4BytesWritten = 0;
	INT_32 i4Argc = 0;
	PCHAR apcArgv[WLAN_CFG_ARGV_MAX] = {0};

	UINT_32 u4Ret, u4Parse;

	UINT_8 ucDBDCEnable;
	/*UINT_8 ucBssIndex;*/
	/*P_BSS_INFO_T prBssInfo;*/


	ASSERT(prNetDev);
	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
#if 0
	for (ucBssIndex = 0; ucBssIndex < (HW_BSSID_NUM+1); ucBssIndex++) {
		prBssInfo = prGlueInfo->prAdapter->aprBssInfo[ucBssIndex];
		pr_info("****BSS %u inUse %u active %u Mode %u priCh %u state %u rfBand %u\n",
			ucBssIndex,
			prBssInfo->fgIsInUse,
			prBssInfo->fgIsNetActive,
			prBssInfo->eCurrentOPMode,
			prBssInfo->ucPrimaryChannel,
			prBssInfo->eConnectionState,
			prBssInfo->eBand);
	}
#endif
	if (prGlueInfo->prAdapter->rWifiVar.ucDbdcMode != DBDC_MODE_DYNAMIC) {
		DBGLOG(REQ, LOUD, "Current DBDC mode %u cannot enable/disable DBDC!!\n",
			prGlueInfo->prAdapter->rWifiVar.ucDbdcMode);
		return -1;
	}

	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2) {

		u4Ret = kalkStrtou32(apcArgv[1], 0, &u4Parse);
		if (u4Ret)
			DBGLOG(REQ, LOUD, "parse apcArgv error u4Ret=%d\n", u4Ret);

		ucDBDCEnable = (UINT_8) u4Parse;
		if ((!prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn && !ucDBDCEnable) ||
			(prGlueInfo->prAdapter->rWifiVar.fgDbDcModeEn && ucDBDCEnable))
			return i4BytesWritten;

		rStatus = kalIoctl(prGlueInfo,
						wlanoidSetDbdcEnable,
						&ucDBDCEnable, 1,
						FALSE, FALSE, TRUE, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -1;

	} else {
		DBGLOG(INIT, ERROR, "iwpriv wlanXX driver SET_DBDC <enable>\n");
		DBGLOG(INIT, ERROR, "<enable> 1: enable. 0: disable.\n");
	}

	return i4BytesWritten;
}
#endif /*CFG_SUPPORT_DBDC*/

#if CFG_SUPPORT_BATCH_SCAN
#define CMD_BATCH_SET           "WLS_BATCHING SET"
#define CMD_BATCH_GET           "WLS_BATCHING GET"
#define CMD_BATCH_STOP          "WLS_BATCHING STOP"
#endif

static int priv_driver_get_que_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	return qmDumpQueueStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

static int priv_driver_get_mem_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	return cnmDumpMemoryStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

static int priv_driver_get_hif_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	return halDumpHifStatus(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

static int priv_driver_get_tp_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prNetDev);
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	return kalPerMonGetInfo(prGlueInfo->prAdapter, pcCommand, i4TotalLen);
}

#if defined(_HIF_USB)
static int priv_driver_get_usb_info(IN struct net_device *prNetDev, IN char *pcCommand, IN int i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_32 i4BytesWritten = 0;
	UCHAR pBuffer[512] = {0};

	ASSERT(prNetDev);
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\n");
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\tVenderID: %04x\n",
		glGetUsbDeviceVendorId(prGlueInfo->rHifInfo.udev));
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\tProductID: %04x\n",
		glGetUsbDeviceProductId(prGlueInfo->rHifInfo.udev));

	glGetUsbDeviceManufacturerName(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\tManufacturer: %s\n",
		pBuffer);

	glGetUsbDeviceProductName(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\tProduct: %s\n", pBuffer);

	glGetUsbDeviceSerialNumber(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pcCommand, i4TotalLen, i4BytesWritten, "\tSerialNumber: %s\n",
		pBuffer);

	return i4BytesWritten;
}
#endif

INT_32 priv_driver_cmds(IN struct net_device *prNetDev, IN PCHAR pcCommand, IN INT_32 i4TotalLen)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4BytesWritten = 0;
	INT_32 i4CmdFound = 0;

	if (GLUE_CHK_PR2(prNetDev, pcCommand) == FALSE)
		return -1;
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));

	if (i4CmdFound == 0) {
		i4CmdFound = 1;
		if (strnicmp(pcCommand, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
			/* i4BytesWritten =
			 *  wl_android_get_rssi(net, command, i4TotalLen);
			 */
		} else if (strnicmp(pcCommand, CMD_AP_START, strlen(CMD_AP_START)) == 0) {
			i4BytesWritten = priv_driver_set_ap_start(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) == 0) {
			i4BytesWritten = priv_driver_get_linkspeed(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_PNOSSIDCLR_SET, strlen(CMD_PNOSSIDCLR_SET)) == 0) {
			/* ToDo:: Nothing */
		} else if (strnicmp(pcCommand, CMD_PNOSETUP_SET, strlen(CMD_PNOSETUP_SET)) == 0) {
			/* ToDo:: Nothing */
		} else if (strnicmp(pcCommand, CMD_PNOENABLE_SET, strlen(CMD_PNOENABLE_SET)) == 0) {
			/* ToDo:: Nothing */
		} else if (strnicmp(pcCommand, CMD_SETSUSPENDOPT, strlen(CMD_SETSUSPENDOPT)) == 0) {
			/* i4BytesWritten = wl_android_set_suspendopt(net, pcCommand, i4TotalLen); */
		} else if (strnicmp(pcCommand, CMD_SETSUSPENDMODE, strlen(CMD_SETSUSPENDMODE)) == 0) {
			i4BytesWritten = priv_driver_set_suspend_mode(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
			i4BytesWritten = priv_driver_set_band(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
			/* i4BytesWritten = wl_android_get_band(net, pcCommand, i4TotalLen); */
		} else if (strnicmp(pcCommand, CMD_SET_TXPOWER, strlen(CMD_SET_TXPOWER)) == 0) {
			i4BytesWritten = priv_driver_set_txpower(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
			i4BytesWritten = priv_driver_set_country(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_MIRACAST, strlen(CMD_MIRACAST)) == 0) {
			i4BytesWritten = priv_driver_set_miracast(prNetDev, pcCommand, i4TotalLen);
		}
		/* Mediatek private command  */
		else if (strnicmp(pcCommand, CMD_SET_SW_CTRL, strlen(CMD_SET_SW_CTRL)) == 0) {
			i4BytesWritten = priv_driver_set_sw_ctrl(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_FIXED_RATE, strlen(CMD_SET_FIXED_RATE)) == 0) {
			i4BytesWritten = priv_driver_set_fixed_rate(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_SW_CTRL, strlen(CMD_GET_SW_CTRL)) == 0) {
			i4BytesWritten = priv_driver_get_sw_ctrl(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_MCR, strlen(CMD_SET_MCR)) == 0) {
			i4BytesWritten = priv_driver_set_mcr(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_MCR, strlen(CMD_GET_MCR)) == 0) {
			i4BytesWritten = priv_driver_get_mcr(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_DRV_MCR, strlen(CMD_SET_DRV_MCR)) == 0) {
			i4BytesWritten = priv_driver_set_drv_mcr(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_DRV_MCR, strlen(CMD_GET_DRV_MCR)) == 0) {
			i4BytesWritten = priv_driver_get_drv_mcr(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_TEST_MODE, strlen(CMD_SET_TEST_MODE)) == 0) {
			i4BytesWritten = priv_driver_set_test_mode(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_TEST_CMD, strlen(CMD_SET_TEST_CMD)) == 0) {
			i4BytesWritten = priv_driver_set_test_cmd(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_TEST_RESULT, strlen(CMD_GET_TEST_RESULT)) == 0) {
			i4BytesWritten = priv_driver_get_test_result(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_STA_STAT, strlen(CMD_GET_STA_STAT)) == 0) {
			i4BytesWritten = priv_driver_get_sta_stat(prNetDev, pcCommand, i4TotalLen);
		}
#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
		else if (strnicmp(pcCommand,
			CMD_SET_CALBACKUP_TEST_DRV_FW, strlen(CMD_SET_CALBACKUP_TEST_DRV_FW)) == 0)
			i4BytesWritten = priv_driver_set_calbackup_test_drv_fw(prNetDev, pcCommand, i4TotalLen);
#endif
#if CFG_WOW_SUPPORT
		else if (strnicmp(pcCommand, CMD_SET_WOW_ENABLE, strlen(CMD_SET_WOW_ENABLE)) == 0)
			i4BytesWritten = priv_driver_set_wow_enable(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_SET_WOW_PAR, strlen(CMD_SET_WOW_PAR)) == 0)
			i4BytesWritten = priv_driver_set_wow_par(prNetDev, pcCommand, i4TotalLen);
#endif
#if CFG_SUPPORT_QA_TOOL
		else if (strnicmp(pcCommand, CMD_GET_RX_STATISTICS, strlen(CMD_GET_RX_STATISTICS)) == 0)
			i4BytesWritten = priv_driver_get_rx_statistics(prNetDev, pcCommand, i4TotalLen);
#if CFG_SUPPORT_BUFFER_MODE
		else if (strnicmp(pcCommand, CMD_SETBUFMODE, strlen(CMD_SETBUFMODE)) == 0)
			i4BytesWritten = priv_driver_set_efuse_buffer_mode(prNetDev, pcCommand, i4TotalLen);
#endif
#endif
#if CFG_SUPPORT_MSP
#if 0
		else if (strnicmp(pcCommand, CMD_GET_STAT, strlen(CMD_GET_STAT)) == 0)
			i4BytesWritten = priv_driver_get_stat(prNetDev, pcCommand, i4TotalLen);
#endif
		else if (strnicmp(pcCommand, CMD_GET_STA_STATISTICS, strlen(CMD_GET_STA_STATISTICS)) == 0)
			i4BytesWritten = priv_driver_get_sta_statistics(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_BSS_STATISTICS, strlen(CMD_GET_BSS_STATISTICS)) == 0)
			i4BytesWritten = priv_driver_get_bss_statistics(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_STA_INFO, strlen(CMD_GET_STA_INFO)) == 0)
			i4BytesWritten = priv_driver_get_sta_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_WTBL_INFO, strlen(CMD_GET_WTBL_INFO)) == 0)
			i4BytesWritten = priv_driver_get_wtbl_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_MIB_INFO, strlen(CMD_GET_MIB_INFO)) == 0)
			i4BytesWritten = priv_driver_get_mib_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_SET_FW_LOG, strlen(CMD_SET_FW_LOG)) == 0)
			i4BytesWritten = priv_driver_set_fw_log(prNetDev, pcCommand, i4TotalLen);
#endif
		else if (strnicmp(pcCommand, CMD_SET_CFG, strlen(CMD_SET_CFG)) == 0) {
			i4BytesWritten = priv_driver_set_cfg(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_CFG, strlen(CMD_GET_CFG)) == 0) {
			i4BytesWritten = priv_driver_get_cfg(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_SET_CHIP, strlen(CMD_SET_CHIP)) == 0) {
			i4BytesWritten = priv_driver_set_chip_config(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_CHIP, strlen(CMD_GET_CHIP)) == 0) {
			i4BytesWritten = priv_driver_get_chip_config(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_VERSION, strlen(CMD_GET_VERSION)) == 0) {
			i4BytesWritten = priv_driver_get_version(prNetDev, pcCommand, i4TotalLen);
		} else if (strnicmp(pcCommand, CMD_GET_CNM, strlen(CMD_GET_CNM)) == 0) {
			i4BytesWritten = priv_driver_get_cnm(prNetDev, pcCommand, i4TotalLen);
#if CFG_SUPPORT_DBDC

		} else if (strnicmp(pcCommand, CMD_SET_DBDC, strlen(CMD_SET_DBDC)) == 0) {
			i4BytesWritten = priv_driver_set_dbdc(prNetDev, pcCommand, i4TotalLen);
#endif /*CFG_SUPPORT_DBDC*/
#if CFG_SUPPORT_BATCH_SCAN
		} else if (strnicmp(pcCommand, CMD_BATCH_SET, strlen(CMD_BATCH_SET)) == 0) {
			kalIoctl(prGlueInfo,
				 wlanoidSetBatchScanReq,
				 (PVOID) pcCommand, i4TotalLen, FALSE, FALSE, TRUE, &i4BytesWritten);
		} else if (strnicmp(pcCommand, CMD_BATCH_GET, strlen(CMD_BATCH_GET)) == 0) {
			/* strcpy(pcCommand, "BATCH SCAN DATA FROM FIRMWARE"); */
			/* i4BytesWritten = strlen("BATCH SCAN DATA FROM FIRMWARE") + 1; */
			/* i4BytesWritten = priv_driver_get_linkspeed (prNetDev, pcCommand, i4TotalLen); */

			UINT_32 u4BufLen;
			int i;
			/* int rlen=0; */

			for (i = 0; i < CFG_BATCH_MAX_MSCAN; i++) {
				g_rEventBatchResult[i].ucScanCount = i + 1;	/* for get which mscan */
				kalIoctl(prGlueInfo,
					 wlanoidQueryBatchScanResult,
					 (PVOID)&g_rEventBatchResult[i],
					 sizeof(EVENT_BATCH_RESULT_T), TRUE, TRUE, TRUE, &u4BufLen);
			}

#if 0
			DBGLOG(SCN, INFO, "Batch Scan Results, scan count = %u\n", g_rEventBatchResult.ucScanCount);
			for (i = 0; i < g_rEventBatchResult.ucScanCount; i++) {
				prEntry = &g_rEventBatchResult.arBatchResult[i];
				DBGLOG(SCN, INFO, "Entry %u\n", i);
				DBGLOG(SCN, INFO, "	 BSSID = " MACSTR "\n", MAC2STR(prEntry->aucBssid));
				DBGLOG(SCN, INFO, "	 SSID = %s\n", prEntry->aucSSID);
				DBGLOG(SCN, INFO, "	 SSID len = %u\n", prEntry->ucSSIDLen);
				DBGLOG(SCN, INFO, "	 RSSI = %d\n", prEntry->cRssi);
				DBGLOG(SCN, INFO, "	 Freq = %u\n", prEntry->ucFreq);
			}
#endif

			batchConvertResult(&g_rEventBatchResult[0], pcCommand, i4TotalLen, &i4BytesWritten);

			/* Dump for debug */
			/* print_hex_dump(KERN_INFO,
			 *  "BATCH", DUMP_PREFIX_ADDRESS, 16, 1, pcCommand, i4BytesWritten, TRUE);
			 */

		} else if (strnicmp(pcCommand, CMD_BATCH_STOP, strlen(CMD_BATCH_STOP)) == 0) {
			kalIoctl(prGlueInfo,
				 wlanoidSetBatchScanReq,
				 (PVOID) pcCommand, i4TotalLen, FALSE, FALSE, TRUE, &i4BytesWritten);
#endif
		}
#if CFG_SUPPORT_SNIFFER
		else if (strnicmp(pcCommand, CMD_SETMONITOR, strlen(CMD_SETMONITOR)) == 0)
			i4BytesWritten = priv_driver_set_monitor(prNetDev, pcCommand, i4TotalLen);
#endif
		else if (strnicmp(pcCommand, CMD_GET_QUE_INFO, strlen(CMD_GET_QUE_INFO)) == 0)
			i4BytesWritten = priv_driver_get_que_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_MEM_INFO, strlen(CMD_GET_MEM_INFO)) == 0)
			i4BytesWritten = priv_driver_get_mem_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_HIF_INFO, strlen(CMD_GET_HIF_INFO)) == 0)
			i4BytesWritten = priv_driver_get_hif_info(prNetDev, pcCommand, i4TotalLen);
		else if (strnicmp(pcCommand, CMD_GET_TP_INFO, strlen(CMD_GET_TP_INFO)) == 0)
			i4BytesWritten = priv_driver_get_tp_info(prNetDev, pcCommand, i4TotalLen);
#if defined(_HIF_USB)
		else if (strnicmp(pcCommand, CMD_USB_INFO, strlen(CMD_USB_INFO)) == 0)
			i4BytesWritten = priv_driver_get_usb_info(prNetDev, pcCommand, i4TotalLen);
#endif
		else
			i4CmdFound = 0;
	}
	/* i4CmdFound */
	if (i4CmdFound == 0)
		DBGLOG(REQ, INFO, "Unknown driver command %s - ignored\n", pcCommand);

	if (i4BytesWritten >= 0) {
		if ((i4BytesWritten == 0) && (i4TotalLen > 0)) {
			/* reset the command buffer */
			pcCommand[0] = '\0';
		}

		if (i4BytesWritten >= i4TotalLen) {
			DBGLOG(REQ, INFO,
			       "%s: i4BytesWritten %d > i4TotalLen < %d\n", __func__, i4BytesWritten, i4TotalLen);
			i4BytesWritten = i4TotalLen;
		} else {
			pcCommand[i4BytesWritten] = '\0';
			i4BytesWritten++;
		}
	}

	return i4BytesWritten;

}				/* priv_driver_cmds */

int priv_support_driver_cmd(IN struct net_device *prNetDev, IN OUT struct ifreq *prReq, IN int i4Cmd)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	int ret = 0;
	char *pcCommand = NULL;
	priv_driver_cmd_t *priv_cmd = NULL;
	int i4BytesWritten = 0;
	int i4TotalLen = 0;

	if (!prReq->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prNetDev));
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN, "No glue info\n");
		ret = -EFAULT;
		goto exit;
	}
	if (prGlueInfo->u4ReadyFlag == 0) {
		ret = -EINVAL;
		goto exit;
	}

	priv_cmd = kzalloc(sizeof(priv_driver_cmd_t), GFP_KERNEL);
	if (!priv_cmd) {
		DBGLOG(REQ, WARN, "%s, alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(priv_cmd, prReq->ifr_data, sizeof(priv_driver_cmd_t))) {
		DBGLOG(REQ, INFO, "%s: copy_from_user fail\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

	i4TotalLen = priv_cmd->total_len;

	if (i4TotalLen <= 0) {
		ret = -EINVAL;
		DBGLOG(REQ, INFO, "%s: i4TotalLen invalid\n", __func__);
		goto exit;
	}

	pcCommand = priv_cmd->buf;

	DBGLOG(REQ, INFO, "driver cmd \"%s\" on %s\n", pcCommand, prReq->ifr_name);

	i4BytesWritten = priv_driver_cmds(prNetDev, pcCommand, i4TotalLen);

	if (i4BytesWritten < 0) {
		DBGLOG(REQ, INFO, "%s: command %s Written is %d\n", __func__, pcCommand, i4BytesWritten);
		if (i4TotalLen >= 3) {
			snprintf(pcCommand, 3, "OK");
			i4BytesWritten = strlen("OK");
		}
	}

exit:
	kfree(priv_cmd);

	return ret;
}				/* priv_support_driver_cmd */
