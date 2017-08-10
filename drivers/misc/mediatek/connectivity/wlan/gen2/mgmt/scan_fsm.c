/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/scan_fsm.c#1
*/

/*! \file   "scan_fsm.c"
    \brief  This file defines the state transition function for SCAN FSM.

    The SCAN FSM is part of SCAN MODULE and responsible for performing basic SCAN
    behavior as metioned in IEEE 802.11 2007 11.1.3.1 & 11.1.3.2 .
*/

/*
** Log: scan_fsm.c
 *
 * 06 13 2012 yuche.tsai
 * NULL
 * Update maintrunk driver.
 * Add support for driver compose assoc request frame.
 *
 * 11 24 2011 wh.su
 * [WCXRP00001078] [MT6620 Wi-Fi][Driver] Adding the mediatek log improment support : XLOG
 * Adjust code for DBG and CONFIG_XLOG.
 *
 * 11 14 2011 yuche.tsai
 * [WCXRP00001095] [Volunteer Patch][Driver] Always Scan before enable Hot-Spot.
 * Fix bug when unregister P2P network..
 *
 * 11 11 2011 wh.su
 * [WCXRP00001078] [MT6620 Wi-Fi][Driver] Adding the mediatek log improment support : XLOG
 * modify the xlog related code.
 *
 * 11 02 2011 wh.su
 * [WCXRP00001078] [MT6620 Wi-Fi][Driver] Adding the mediatek log improment support : XLOG
 * adding the code for XLOG.
 *
 * 08 11 2011 cp.wu
 * [WCXRP00000830] [MT6620 Wi-Fi][Firmware] Use MDRDY counter to detect empty channel for shortening scan time
 * sparse channel detection:
 * driver: collect sparse channel information with scan-done event
 *
 * 07 18 2011 cp.wu
 * [WCXRP00000858] [MT5931][Driver][Firmware] Add support for scan to search
 * for more than one SSID in a single scanning request
 * free mailbox message afte parsing is completed.
 *
 * 07 18 2011 cp.wu
 * [WCXRP00000858] [MT5931][Driver][Firmware] Add support for scan to search
 * for more than one SSID in a single scanning request
 * add framework in driver domain for supporting new SCAN_REQ_V2 for more than 1 SSID support
 * as well as uProbeDelay in NDIS 6.x driver model
 *
 * 04 18 2011 terry.wu
 * [WCXRP00000660] [MT6620 Wi-Fi][Driver] Remove flag CFG_WIFI_DIRECT_MOVED
 * Remove flag CFG_WIFI_DIRECT_MOVED.
 *
 * 03 29 2011 cp.wu
 * [WCXRP00000604] [MT6620 Wi-Fi][Driver] Surpress Klockwork Warning
 * surpress klock warning with code path rewritten
 *
 * 03 18 2011 cm.chang
 * [WCXRP00000576] [MT6620 Wi-Fi][Driver][FW] Remove P2P compile option in scan req/cancel command
 * As CR title
 *
 * 02 18 2011 yuche.tsai
 * [WCXRP00000478] [Volunteer Patch][MT6620][Driver] Probe request frame
 * during search phase do not contain P2P wildcard SSID.
 * Take P2P wildcard SSID into consideration.
 *
 * 01 27 2011 yuche.tsai
 * [WCXRP00000399] [Volunteer Patch][MT6620/MT5931][Driver] Fix scan side effect after P2P module separate.
 * Fix scan channel extension issue when p2p module is not registered.
 *
 * 01 26 2011 yuche.tsai
 * [WCXRP00000388] [Volunteer Patch][MT6620][Driver/Fw] change Station Type in station record.
 * .
 *
 * 01 25 2011 yuche.tsai
 * [WCXRP00000388] [Volunteer Patch][MT6620][Driver/Fw] change Station Type in station record.
 * Fix Compile Error when DBG is disabled.
 *
 * 12 07 2010 cm.chang
 * [WCXRP00000238] MT6620 Wi-Fi][Driver][FW] Support regulation domain setting from NVRAM and supplicant
 * 1. Country code is from NVRAM or supplicant
 * 2. Change band definition in CMD/EVENT.
 *
 * 09 03 2010 kevin.huang
 * NULL
 * Refine #include sequence and solve recursive/nested #include issue
 *
 * 08 30 2010 cp.wu
 * NULL
 * eliminate klockwork errors
 *
 * 08 16 2010 cp.wu
 * NULL
 * add interface for RLM to trigger OBSS-SCAN.
 *
 * 08 16 2010 yuche.tsai
 * NULL
 * Fix bug for processing queued scan request.
 *
 * 08 11 2010 yuche.tsai
 * NULL
 * Add a function for returning channel.
 *
 * 08 05 2010 yuche.tsai
 * NULL
 * Update SCAN FSM for support P2P Device discovery scan.
 *
 * 08 03 2010 cp.wu
 * NULL
 * surpress compilation warning.
 *
 * 07 26 2010 yuche.tsai
 *
 * Add option of channel extension while cancelling scan request.
 *
 * 07 21 2010 yuche.tsai
 *
 * Add P2P Scan & Scan Result Parsing & Saving.
 *
 * 07 20 2010 cp.wu
 *
 * pass band information for scan in an efficient way by mapping ENUM_BAND_T into UINT_8..
 *
 * 07 19 2010 cp.wu
 *
 * due to FW/DRV won't be sync. precisely, some strict assertions should be eased.
 *
 * 07 19 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration.
 * SCN module is now able to handle multiple concurrent scanning requests
 *
 * 07 16 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration.
 * bugfix for SCN migration
 * 1) modify QUEUE_CONCATENATE_QUEUES() so it could be used to concatence with an empty queue
 * 2) before AIS issues scan request, network(BSS) needs to be activated first
 * 3) only invoke COPY_SSID when using specified SSID for scan
 *
 * 07 15 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration.
 * driver no longer generates probe request frames
 *
 * 07 14 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration.
 * pass band with channel number information as scan parameter
 *
 * 07 14 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration.
 * remove timer in DRV-SCN.
 *
 * 07 09 2010 cp.wu
 *
 * 1) separate AIS_FSM state for two kinds of scanning. (OID triggered scan, and scan-for-connection)
 * 2) eliminate PRE_BSS_DESC_T, Beacon/PrebResp is now parsed in single pass
 * 3) implment DRV-SCN module, currently only accepts single scan request,
 * other request will be directly dropped by returning BUSY
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 07 08 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * take use of RLM module for parsing/generating HT IEs for 11n capability
 *
 * 07 02 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * when returning to SCAN_IDLE state, send a correct message to source FSM.
 *
 * 07 01 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * implementation of DRV-SCN and related mailbox message handling.
 *
 * 06 22 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * comment out RLM APIs by CFG_RLM_MIGRATION.
 *
 * 06 21 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * add scan_fsm into building.
 *
 * 05 14 2010 kevin.huang
 * [BORA00000794][WIFISYS][New Feature]Power Management Support
 * Refine the order of Stop TX Queue and Switch Channel
 *
 * 05 12 2010 kevin.huang
 * [BORA00000794][WIFISYS][New Feature]Power Management Support
 * Update pause/resume/flush API to new Bitmap API
 *
 * 05 12 2010 kevin.huang
 * [BORA00000794][WIFISYS][New Feature]Power Management Support
 * Add Power Management - Legacy PS-POLL support.
 *
 * 03 18 2010 kevin.huang
 * [BORA00000663][WIFISYS][New Feature] AdHoc Mode Support
 * Ignore the PROBE_DELAY state if the value of Probe Delay == 0
 *
 * 03 10 2010 kevin.huang
 * [BORA00000654][WIFISYS][New Feature] CNM Module - Ch Manager Support
 * Add Channel Manager for arbitration of JOIN and SCAN Req
 *
 * 02 23 2010 kevin.huang
 * [BORA00000603][WIFISYS] [New Feature] AAA Module Support
 * Add support scan channel 1~14 and update scan result's frequency infou1rwduu`wvpghlqg|n`slk+mpdkb
 *
 * 01 08 2010 kevin.huang
 * [BORA00000018]Integrate WIFI part into BORA for the 1st time
 * Add set RX Filter to receive BCN from different BSSID during SCAN
 *
 * 12 18 2009 cm.chang
 * [BORA00000018]Integrate WIFI part into BORA for the 1st time
 * .
 *
 * Nov 25 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Remove flag of CFG_TEST_MGMT_FSM
 *
 * Nov 20 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Change parameter of scanSendProbeReqFrames()
 *
 * Nov 16 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Update scnFsmSteps()
 *
 * Nov 5 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Fix typo
 *
 * Nov 5 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 *
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

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if DBG
/*lint -save -e64 Type mismatch */
static PUINT_8 apucDebugScanState[SCAN_STATE_NUM] = {
	(PUINT_8) DISP_STRING("SCAN_STATE_IDLE"),
	(PUINT_8) DISP_STRING("SCAN_STATE_SCANNING"),
};

/*lint -restore */
#endif /* DBG */

#define CURRENT_PSCN_VERSION 1
#define RSSI_MARGIN_DEFAULT  5
#define MAX_PERIOD 200000

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmSteps(IN P_ADAPTER_T prAdapter, IN ENUM_SCAN_STATE_T eNextState)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_HDR_T prMsgHdr;

	BOOLEAN fgIsTransition = (BOOLEAN) FALSE;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	do {

#if DBG
		DBGLOG(SCN, STATE, "TRANSITION: [%s] -> [%s]\n",
				    apucDebugScanState[prScanInfo->eCurrentState], apucDebugScanState[eNextState]);
#else
		DBGLOG(SCN, STATE, "[%d] TRANSITION: [%d] -> [%d]\n",
				    DBG_SCN_IDX, prScanInfo->eCurrentState, eNextState);
#endif

		/* NOTE(Kevin): This is the only place to change the eCurrentState(except initial) */
		prScanInfo->eCurrentState = eNextState;

		fgIsTransition = (BOOLEAN) FALSE;

		switch (prScanInfo->eCurrentState) {
		case SCAN_STATE_IDLE:
			/* check for pending scanning requests */
			if (!LINK_IS_EMPTY(&(prScanInfo->rPendingMsgList))) {
				/* load next message from pending list as scan parameters */
				LINK_REMOVE_HEAD(&(prScanInfo->rPendingMsgList), prMsgHdr, P_MSG_HDR_T);

				if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ
				    || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
					scnFsmHandleScanMsg(prAdapter, (P_MSG_SCN_SCAN_REQ) prMsgHdr);
				} else {
					scnFsmHandleScanMsgV2(prAdapter, (P_MSG_SCN_SCAN_REQ_V2) prMsgHdr);
				}

				/* switch to next state */
				eNextState = SCAN_STATE_SCANNING;
				fgIsTransition = TRUE;

				cnmMemFree(prAdapter, prMsgHdr);
			}
			break;

		case SCAN_STATE_SCANNING:
			if (prScanParam->fgIsScanV2 == FALSE)
				scnSendScanReq(prAdapter);
			else
				scnSendScanReqV2(prAdapter);
			break;

		default:
			ASSERT(0);
			break;

		}
	} while (fgIsTransition);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReqExtCh(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	/*CMD_SCAN_REQ_EXT_CH rCmdScanReq;*/
	P_CMD_SCAN_REQ_EXT_CH prCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prCmdScanReq = kalMemAlloc(sizeof(CMD_SCAN_REQ_EXT_CH), VIR_MEM_TYPE);
	if (prCmdScanReq == NULL)
		return;

	/* send command packet for scan */
	kalMemZero(prCmdScanReq, sizeof(CMD_SCAN_REQ_EXT_CH));

	prCmdScanReq->ucSeqNum = prScanParam->ucSeqNum;
	prCmdScanReq->ucNetworkType = (UINT_8) prScanParam->eNetTypeIndex;
	prCmdScanReq->ucScanType = (UINT_8) prScanParam->eScanType;
	prCmdScanReq->ucSSIDType = prScanParam->ucSSIDType;

	if (prScanParam->ucSSIDNum == 1) {
		COPY_SSID(prCmdScanReq->aucSSID,
			  prCmdScanReq->ucSSIDLength,
			  prScanParam->aucSpecifiedSSID[0], prScanParam->ucSpecifiedSSIDLen[0]);
	}

	prCmdScanReq->ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		prCmdScanReq->ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < prCmdScanReq->ucChannelListNum; i++) {
			prCmdScanReq->arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			prCmdScanReq->arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
		prCmdScanReq->u2ChannelDwellTime = prScanParam->u2PassiveListenInterval;
#endif

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdScanReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdScanReq->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prCmdScanReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdScanReq->u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ_EXT_CH, aucIE) + prCmdScanReq->u2IELen,
			    (PUINT_8) prCmdScanReq, NULL, 0);
	/* sanity check for some scan parameters */
	if (prCmdScanReq->ucScanType >= SCAN_TYPE_NUM)
		kalSendAeeWarning("wlan", "wrong scan type %d", prCmdScanReq->ucScanType);
	else if (prCmdScanReq->ucChannelType >= SCAN_CHANNEL_NUM)
		kalSendAeeWarning("wlan", "wrong channel type %d", prCmdScanReq->ucChannelType);
	else if (prCmdScanReq->ucChannelType != SCAN_CHANNEL_SPECIFIED &&
		prCmdScanReq->ucChannelListNum != 0)
		kalSendAeeWarning("wlan",
			"channel list is not NULL but channel type is not specified");
	else if (prCmdScanReq->ucNetworkType >= NETWORK_TYPE_INDEX_NUM)
		kalSendAeeWarning("wlan", "wrong network type %d", prCmdScanReq->ucNetworkType);
	else if (prCmdScanReq->ucSSIDType >= BIT(4)) /* ssid type is wrong */
		kalSendAeeWarning("wlan", "wrong ssid type %d", prCmdScanReq->ucSSIDType);
	else if (prCmdScanReq->ucSSIDLength > 32)
		kalSendAeeWarning("wlan", "wrong ssid length %d", prCmdScanReq->ucSSIDLength);

	kalMemFree(prCmdScanReq, VIR_MEM_TYPE, sizeof(CMD_SCAN_REQ_EXT_CH));
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReq(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	/*CMD_SCAN_REQ rCmdScanReq;*/
	P_CMD_SCAN_REQ prCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanParam->ucChannelListNum > 32) {
		scnSendScanReqExtCh(prAdapter);
	} else {
		prCmdScanReq = kalMemAlloc(sizeof(CMD_SCAN_REQ), VIR_MEM_TYPE);
		if (prCmdScanReq == NULL) {
			DBGLOG(SCN, INFO, "alloc CmdScanReq fail");
			return;
		}
		/* send command packet for scan */
		kalMemZero(prCmdScanReq, sizeof(CMD_SCAN_REQ));

		prCmdScanReq->ucSeqNum = prScanParam->ucSeqNum;
		prCmdScanReq->ucNetworkType = (UINT_8) prScanParam->eNetTypeIndex;
		prCmdScanReq->ucScanType = (UINT_8) prScanParam->eScanType;
		prCmdScanReq->ucSSIDType = prScanParam->ucSSIDType;

		if (prScanParam->ucSSIDNum == 1) {
			COPY_SSID(prCmdScanReq->aucSSID,
				  prCmdScanReq->ucSSIDLength,
				  prScanParam->aucSpecifiedSSID[0], prScanParam->ucSpecifiedSSIDLen[0]);
		}

		prCmdScanReq->ucChannelType = (UINT_8) prScanParam->eScanChannel;

		if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
			/* P2P would use:
			 * 1. Specified Listen Channel of passive scan for LISTEN state.
			 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state.
			 * (Target != NULL)
			 */
			prCmdScanReq->ucChannelListNum = prScanParam->ucChannelListNum;

			for (i = 0; i < prCmdScanReq->ucChannelListNum; i++) {
				prCmdScanReq->arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

				prCmdScanReq->arChannelList[i].ucChannelNum =
				    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
		if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
			prCmdScanReq->u2ChannelDwellTime = prScanParam->u2PassiveListenInterval;
#endif
#if CFG_ENABLE_FAST_SCAN
		if (prScanParam->eNetTypeIndex == NETWORK_TYPE_AIS_INDEX)
			prCmdScanReq->u2ChannelDwellTime = CFG_FAST_SCAN_DWELL_TIME;
#endif
		if (prScanParam->u2IELen <= MAX_IE_LENGTH)
			prCmdScanReq->u2IELen = prScanParam->u2IELen;
		else
			prCmdScanReq->u2IELen = MAX_IE_LENGTH;

		if (prScanParam->u2IELen)
			kalMemCopy(prCmdScanReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdScanReq->u2IELen);

		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SCAN_REQ,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    NULL,
				    OFFSET_OF(CMD_SCAN_REQ, aucIE) + prCmdScanReq->u2IELen,
				    (PUINT_8) prCmdScanReq, NULL, 0);
		/* sanity check for some scan parameters */
		if (prCmdScanReq->ucScanType >= SCAN_TYPE_NUM)
			kalSendAeeWarning("wlan", "wrong scan type %d", prCmdScanReq->ucScanType);
		else if (prCmdScanReq->ucChannelType >= SCAN_CHANNEL_NUM)
			kalSendAeeWarning("wlan", "wrong channel type %d", prCmdScanReq->ucChannelType);
		else if (prCmdScanReq->ucChannelType != SCAN_CHANNEL_SPECIFIED &&
			prCmdScanReq->ucChannelListNum != 0)
			kalSendAeeWarning("wlan",
				"channel list is not NULL but channel type is not specified");
		else if (prCmdScanReq->ucNetworkType >= NETWORK_TYPE_INDEX_NUM)
			kalSendAeeWarning("wlan", "wrong network type %d", prCmdScanReq->ucNetworkType);
		else if (prCmdScanReq->ucSSIDType >= BIT(4)) /* ssid type is wrong */
			kalSendAeeWarning("wlan", "wrong ssid type %d", prCmdScanReq->ucSSIDType);
		else if (prCmdScanReq->ucSSIDLength > 32)
			kalSendAeeWarning("wlan", "wrong ssid length %d", prCmdScanReq->ucSSIDLength);

		kalMemFree(prCmdScanReq, VIR_MEM_TYPE, sizeof(CMD_SCAN_REQ));
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ_V2 command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReqV2ExtCh(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	/*CMD_SCAN_REQ_V2_EXT_CH rCmdScanReq;*/
	P_CMD_SCAN_REQ_V2_EXT_CH prCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prCmdScanReq = kalMemAlloc(sizeof(CMD_SCAN_REQ_V2_EXT_CH), VIR_MEM_TYPE);
	if (prCmdScanReq == NULL)
		return;

	/* send command packet for scan */
	kalMemZero(prCmdScanReq, sizeof(CMD_SCAN_REQ_V2_EXT_CH));

	prCmdScanReq->ucSeqNum = prScanParam->ucSeqNum;
	prCmdScanReq->ucNetworkType = (UINT_8) prScanParam->eNetTypeIndex;
	prCmdScanReq->ucScanType = (UINT_8) prScanParam->eScanType;
	prCmdScanReq->ucSSIDType = prScanParam->ucSSIDType;

	for (i = 0; i < prScanParam->ucSSIDNum; i++) {
		COPY_SSID(prCmdScanReq->arSSID[i].aucSsid,
			  prCmdScanReq->arSSID[i].u4SsidLen,
			  prScanParam->aucSpecifiedSSID[i], prScanParam->ucSpecifiedSSIDLen[i]);
	}

	prCmdScanReq->u2ProbeDelayTime = (UINT_8) prScanParam->u2ProbeDelayTime;
	prCmdScanReq->ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		prCmdScanReq->ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < prCmdScanReq->ucChannelListNum; i++) {
			prCmdScanReq->arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			prCmdScanReq->arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}
#if CFG_ENABLE_WIFI_DIRECT
	if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
		prCmdScanReq->u2ChannelDwellTime = prScanParam->u2PassiveListenInterval;
#endif

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdScanReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdScanReq->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prCmdScanReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdScanReq->u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ_V2,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ_V2_EXT_CH, aucIE) + prCmdScanReq->u2IELen,
			    (PUINT_8) prCmdScanReq, NULL, 0);
	/* sanity check for some scan parameters */
	if (prCmdScanReq->ucScanType >= SCAN_TYPE_NUM)
		kalSendAeeWarning("wlan", "wrong scan type %d", prCmdScanReq->ucScanType);
	else if (prCmdScanReq->ucChannelType >= SCAN_CHANNEL_NUM)
		kalSendAeeWarning("wlan", "wrong channel type %d", prCmdScanReq->ucChannelType);
	else if (prCmdScanReq->ucChannelType != SCAN_CHANNEL_SPECIFIED &&
		prCmdScanReq->ucChannelListNum != 0)
		kalSendAeeWarning("wlan",
			"channel list is not NULL but channel type is not specified");
	else if (prCmdScanReq->ucNetworkType >= NETWORK_TYPE_INDEX_NUM)
		kalSendAeeWarning("wlan", "wrong network type %d", prCmdScanReq->ucNetworkType);
	else if (prCmdScanReq->ucSSIDType >= BIT(4)) /* ssid type is wrong */
		kalSendAeeWarning("wlan", "wrong ssid type %d", prCmdScanReq->ucSSIDType);

	kalMemFree(prCmdScanReq, VIR_MEM_TYPE, sizeof(CMD_SCAN_REQ_V2_EXT_CH));
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Generate CMD_ID_SCAN_REQ_V2 command
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnSendScanReqV2(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	/*CMD_SCAN_REQ_V2 rCmdScanReq;*/
	P_CMD_SCAN_REQ_V2 prCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanParam->ucChannelListNum > 32) {
		scnSendScanReqV2ExtCh(prAdapter);
	} else {
		prCmdScanReq = kalMemAlloc(sizeof(CMD_SCAN_REQ_V2), VIR_MEM_TYPE);
		if (prCmdScanReq == NULL) {
			DBGLOG(SCN, INFO, "alloc CmdScanReq v2 fail");
			return;
		}
		/* send command packet for scan */
		kalMemZero(prCmdScanReq, sizeof(CMD_SCAN_REQ_V2));

		prCmdScanReq->ucSeqNum = prScanParam->ucSeqNum;
		prCmdScanReq->ucNetworkType = (UINT_8) prScanParam->eNetTypeIndex;
		prCmdScanReq->ucScanType = (UINT_8) prScanParam->eScanType;
		prCmdScanReq->ucSSIDType = prScanParam->ucSSIDType;

		for (i = 0; i < prScanParam->ucSSIDNum; i++) {
			COPY_SSID(prCmdScanReq->arSSID[i].aucSsid,
				  prCmdScanReq->arSSID[i].u4SsidLen,
				  prScanParam->aucSpecifiedSSID[i], prScanParam->ucSpecifiedSSIDLen[i]);
		}

		prCmdScanReq->u2ProbeDelayTime = (UINT_8) prScanParam->u2ProbeDelayTime;
		prCmdScanReq->ucChannelType = (UINT_8) prScanParam->eScanChannel;

		if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
			/* P2P would use:
			 * 1. Specified Listen Channel of passive scan for LISTEN state.
			 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state.
			 * (Target != NULL)
			 */
			prCmdScanReq->ucChannelListNum = prScanParam->ucChannelListNum;

			for (i = 0; i < prCmdScanReq->ucChannelListNum; i++) {
				prCmdScanReq->arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

				prCmdScanReq->arChannelList[i].ucChannelNum =
				    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
			}
		}
#if CFG_ENABLE_WIFI_DIRECT
		if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
			prCmdScanReq->u2ChannelDwellTime = prScanParam->u2PassiveListenInterval;
#endif
		if (prScanParam->u2IELen <= MAX_IE_LENGTH)
			prCmdScanReq->u2IELen = prScanParam->u2IELen;
		else
			prCmdScanReq->u2IELen = MAX_IE_LENGTH;

		if (prScanParam->u2IELen)
			kalMemCopy(prCmdScanReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdScanReq->u2IELen);

		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SCAN_REQ_V2,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    NULL,
				    OFFSET_OF(CMD_SCAN_REQ_V2, aucIE) + prCmdScanReq->u2IELen,
				    (PUINT_8) prCmdScanReq, NULL, 0);
		/* sanity check for some scan parameters */
		if (prCmdScanReq->ucScanType >= SCAN_TYPE_NUM)
			kalSendAeeWarning("wlan", "wrong scan type %d", prCmdScanReq->ucScanType);
		else if (prCmdScanReq->ucChannelType >= SCAN_CHANNEL_NUM)
			kalSendAeeWarning("wlan", "wrong channel type %d", prCmdScanReq->ucChannelType);
		else if (prCmdScanReq->ucChannelType != SCAN_CHANNEL_SPECIFIED &&
			prCmdScanReq->ucChannelListNum != 0)
			kalSendAeeWarning("wlan",
				"channel list is not NULL but channel type is not specified");
		else if (prCmdScanReq->ucNetworkType >= NETWORK_TYPE_INDEX_NUM)
			kalSendAeeWarning("wlan", "wrong network type %d", prCmdScanReq->ucNetworkType);
		else if (prCmdScanReq->ucSSIDType >= BIT(4)) /* ssid type is wrong */
			kalSendAeeWarning("wlan", "wrong ssid type %d", prCmdScanReq->ucSSIDType);

		kalMemFree(prCmdScanReq, VIR_MEM_TYPE, sizeof(CMD_SCAN_REQ_V2));
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmMsgStart(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;

	ASSERT(prMsgHdr);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanInfo->eCurrentState == SCAN_STATE_IDLE) {
		if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
		    || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
		    || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
			scnFsmHandleScanMsg(prAdapter, (P_MSG_SCN_SCAN_REQ) prMsgHdr);
		} else if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			   || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			scnFsmHandleScanMsgV2(prAdapter, (P_MSG_SCN_SCAN_REQ_V2) prMsgHdr);
		} else {
			/* should not deliver to this function */
			ASSERT(0);
		}

		cnmMemFree(prAdapter, prMsgHdr);
		scnFsmSteps(prAdapter, SCAN_STATE_SCANNING);
	} else {
		LINK_INSERT_TAIL(&prScanInfo->rPendingMsgList, &prMsgHdr->rLinkEntry);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmMsgAbort(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_SCN_SCAN_CANCEL prScanCancel;
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	CMD_SCAN_CANCEL rCmdScanCancel;

	ASSERT(prMsgHdr);

	prScanCancel = (P_MSG_SCN_SCAN_CANCEL) prMsgHdr;
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	if (prScanInfo->eCurrentState != SCAN_STATE_IDLE) {
		if (prScanCancel->ucSeqNum == prScanParam->ucSeqNum &&
		    prScanCancel->ucNetTypeIndex == (UINT_8) prScanParam->eNetTypeIndex) {
			/* send cancel message to firmware domain */
			rCmdScanCancel.ucSeqNum = prScanParam->ucSeqNum;

#if CFG_ENABLE_WIFI_DIRECT
			if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
				rCmdScanCancel.ucIsExtChannel = (UINT_8) prScanCancel->fgIsChannelExt;
			else
				rCmdScanCancel.ucIsExtChannel = (UINT_8) FALSE;
#endif

			wlanSendSetQueryCmd(prAdapter,
					    CMD_ID_SCAN_CANCEL,
					    TRUE,
					    FALSE,
					    FALSE,
					    NULL, NULL, sizeof(CMD_SCAN_CANCEL), (PUINT_8) &rCmdScanCancel, NULL, 0);

			/* generate scan-done event for caller */
			scnFsmGenerateScanDoneMsg(prAdapter,
						  prScanParam->ucSeqNum,
						  (UINT_8) prScanParam->eNetTypeIndex, SCAN_STATUS_CANCELLED);

			/* switch to next pending scan */
			scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
		} else {
			scnFsmRemovePendingMsg(prAdapter, prScanCancel->ucSeqNum, prScanCancel->ucNetTypeIndex);
		}
	}

	cnmMemFree(prAdapter, prMsgHdr);

}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Scan Message Parsing (Legacy)
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmHandleScanMsg(IN P_ADAPTER_T prAdapter, IN P_MSG_SCN_SCAN_REQ prScanReqMsg)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	UINT_32 i;

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->eNetTypeIndex = (ENUM_NETWORK_TYPE_INDEX_T) prScanReqMsg->ucNetTypeIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	if (prScanParam->ucSSIDType & (SCAN_REQ_SSID_SPECIFIED | SCAN_REQ_SSID_P2P_WILDCARD)) {
		prScanParam->ucSSIDNum = 1;

		COPY_SSID(prScanParam->aucSpecifiedSSID[0],
			  prScanParam->ucSpecifiedSSIDLen[0], prScanReqMsg->aucSSID, prScanReqMsg->ucSSIDLength);

		/* reset SSID length to zero for rest array entries */
		for (i = 1; i < SCN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	} else {
		prScanParam->ucSSIDNum = 0;

		for (i = 0; i < SCN_SSID_MAX_NUM; i++)
			prScanParam->ucSpecifiedSSIDLen[i] = 0;
	}

	prScanParam->u2ProbeDelayTime = 0;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum <= MAXIMUM_OPERATION_CHANNEL_LIST)
			prScanParam->ucChannelListNum = prScanReqMsg->ucChannelListNum;
		else
			prScanParam->ucChannelListNum = MAXIMUM_OPERATION_CHANNEL_LIST;

		kalMemCopy(prScanParam->arChnlInfoList,
			   prScanReqMsg->arChnlInfoList, sizeof(RF_CHANNEL_INFO_T) * prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prScanParam->aucIE, prScanReqMsg->aucIE, prScanParam->u2IELen);
#if CFG_ENABLE_WIFI_DIRECT
	if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
		prScanParam->u2PassiveListenInterval = prScanReqMsg->u2ChannelDwellTime;
#endif
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;

	if (prScanReqMsg->rMsgHdr.eMsgId == MID_RLM_SCN_SCAN_REQ)
		prScanParam->fgIsObssScan = TRUE;
	else
		prScanParam->fgIsObssScan = FALSE;

	prScanParam->fgIsScanV2 = FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Scan Message Parsing - V2 with multiple SSID support
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmHandleScanMsgV2(IN P_ADAPTER_T prAdapter, IN P_MSG_SCN_SCAN_REQ_V2 prScanReqMsg)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	UINT_32 i;

	ASSERT(prAdapter);
	ASSERT(prScanReqMsg);
	ASSERT(prScanReqMsg->ucSSIDNum <= SCN_SSID_MAX_NUM);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanParam->eScanType = prScanReqMsg->eScanType;
	prScanParam->eNetTypeIndex = (ENUM_NETWORK_TYPE_INDEX_T) prScanReqMsg->ucNetTypeIndex;
	prScanParam->ucSSIDType = prScanReqMsg->ucSSIDType;
	prScanParam->ucSSIDNum = prScanReqMsg->ucSSIDNum;

	for (i = 0; i < prScanReqMsg->ucSSIDNum; i++) {
		COPY_SSID(prScanParam->aucSpecifiedSSID[i],
			  prScanParam->ucSpecifiedSSIDLen[i],
			  prScanReqMsg->prSsid[i].aucSsid, (UINT_8) prScanReqMsg->prSsid[i].u4SsidLen);
	}

	prScanParam->u2ProbeDelayTime = prScanReqMsg->u2ProbeDelay;
	prScanParam->eScanChannel = prScanReqMsg->eScanChannel;
	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		if (prScanReqMsg->ucChannelListNum <= MAXIMUM_OPERATION_CHANNEL_LIST)
			prScanParam->ucChannelListNum = prScanReqMsg->ucChannelListNum;
		else
			prScanParam->ucChannelListNum = MAXIMUM_OPERATION_CHANNEL_LIST;

		kalMemCopy(prScanParam->arChnlInfoList,
			   prScanReqMsg->arChnlInfoList, sizeof(RF_CHANNEL_INFO_T) * prScanParam->ucChannelListNum);
	}

	if (prScanReqMsg->u2IELen <= MAX_IE_LENGTH)
		prScanParam->u2IELen = prScanReqMsg->u2IELen;
	else
		prScanParam->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prScanParam->aucIE, prScanReqMsg->aucIE, prScanParam->u2IELen);
#if CFG_ENABLE_WIFI_DIRECT
	if (prScanParam->eNetTypeIndex == NETWORK_TYPE_P2P_INDEX)
		prScanParam->u2PassiveListenInterval = prScanReqMsg->u2ChannelDwellTime;
#endif
	prScanParam->ucSeqNum = prScanReqMsg->ucSeqNum;

	if (prScanReqMsg->rMsgHdr.eMsgId == MID_RLM_SCN_SCAN_REQ)
		prScanParam->fgIsObssScan = TRUE;
	else
		prScanParam->fgIsObssScan = FALSE;

	prScanParam->fgIsScanV2 = TRUE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief            Remove pending scan request
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnFsmRemovePendingMsg(IN P_ADAPTER_T prAdapter, IN UINT_8 ucSeqNum, IN UINT_8 ucNetTypeIndex)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_HDR_T prPendingMsgHdr, prPendingMsgHdrNext, prRemoveMsgHdr = NULL;
	P_LINK_ENTRY_T prRemoveLinkEntry = NULL;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* traverse through rPendingMsgList for removal */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingMsgHdr,
				 prPendingMsgHdrNext, &(prScanInfo->rPendingMsgList), rLinkEntry, MSG_HDR_T) {
		if (prPendingMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ
		    || prPendingMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ) {
			P_MSG_SCN_SCAN_REQ prScanReqMsg = (P_MSG_SCN_SCAN_REQ) prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsg->ucSeqNum && ucNetTypeIndex == prScanReqMsg->ucNetTypeIndex) {
				prRemoveLinkEntry = &(prScanReqMsg->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
			}
		} else if (prPendingMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			P_MSG_SCN_SCAN_REQ_V2 prScanReqMsgV2 = (P_MSG_SCN_SCAN_REQ_V2) prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsgV2->ucSeqNum && ucNetTypeIndex == prScanReqMsgV2->ucNetTypeIndex) {
				prRemoveLinkEntry = &(prScanReqMsgV2->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
			}
		}

		if (prRemoveLinkEntry) {
			/* generate scan-done event for caller */
			scnFsmGenerateScanDoneMsg(prAdapter, ucSeqNum, ucNetTypeIndex, SCAN_STATUS_CANCELLED);

			/* remove from pending list */
			LINK_REMOVE_KNOWN_ENTRY(&(prScanInfo->rPendingMsgList), prRemoveLinkEntry);
			cnmMemFree(prAdapter, prRemoveMsgHdr);

			break;
		}
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnEventScanDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_SCAN_DONE prScanDone)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* buffer empty channel information */
	if (prScanParam->eScanChannel == SCAN_CHANNEL_FULL || prScanParam->eScanChannel == SCAN_CHANNEL_2G4) {
		if (prScanDone->ucSparseChannelValid) {
			prScanInfo->fgIsSparseChannelValid = TRUE;
			prScanInfo->rSparseChannel.eBand = (ENUM_BAND_T) prScanDone->rSparseChannel.ucBand;
			prScanInfo->rSparseChannel.ucChannelNum = prScanDone->rSparseChannel.ucChannelNum;
		} else {
			prScanInfo->fgIsSparseChannelValid = FALSE;
		}
	}

	if (prScanInfo->eCurrentState == SCAN_STATE_SCANNING && prScanDone->ucSeqNum == prScanParam->ucSeqNum) {
		/* generate scan-done event for caller */
		scnFsmGenerateScanDoneMsg(prAdapter,
					  prScanParam->ucSeqNum, (UINT_8) prScanParam->eNetTypeIndex, SCAN_STATUS_DONE);

		/* switch to next pending scan */
		scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
	} else {
		DBGLOG(SCN, WARN, "Unexpected SCAN-DONE event: SeqNum = %d, Current State = %d\n",
				   prScanDone->ucSeqNum, prScanInfo->eCurrentState);
	}

}				/* end of scnEventScanDone */

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnFsmGenerateScanDoneMsg(IN P_ADAPTER_T prAdapter,
			  IN UINT_8 ucSeqNum, IN UINT_8 ucNetTypeIndex, IN ENUM_SCAN_STATUS eScanStatus)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_SCN_SCAN_DONE prScanDoneMsg;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	DBGLOG(SCN, INFO, "Rcv Scan Done, NetIdx %d, Obss %d, Status %d, Seq %d\n",
					ucNetTypeIndex, prScanParam->fgIsObssScan, eScanStatus, ucSeqNum);
	prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_DONE));
	if (!prScanDoneMsg) {
		ASSERT(0);	/* Can't indicate SCAN FSM Complete */
		return;
	}

	if (prScanParam->fgIsObssScan == TRUE) {
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_RLM_SCAN_DONE;
	} else {
		switch ((ENUM_NETWORK_TYPE_INDEX_T) ucNetTypeIndex) {
		case NETWORK_TYPE_AIS_INDEX:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_AIS_SCAN_DONE;
			break;

#if CFG_ENABLE_WIFI_DIRECT
		case NETWORK_TYPE_P2P_INDEX:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_P2P_SCAN_DONE;
			break;
#endif

#if CFG_ENABLE_BT_OVER_WIFI
		case NETWORK_TYPE_BOW_INDEX:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_BOW_SCAN_DONE;
			break;
#endif

		default:
			ASSERT(0);
			break;
		}
	}

	prScanDoneMsg->ucSeqNum = ucSeqNum;
	prScanDoneMsg->ucNetTypeIndex = ucNetTypeIndex;
	prScanDoneMsg->eScanStatus = eScanStatus;

	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prScanDoneMsg, MSG_SEND_METHOD_BUF);

}				/* end of scnFsmGenerateScanDoneMsg() */

/*----------------------------------------------------------------------------*/
/*!
* \brief        Query for most sparse channel
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnQuerySparseChannel(IN P_ADAPTER_T prAdapter, P_ENUM_BAND_T prSparseBand, PUINT_8 pucSparseChannel)
{
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prScanInfo->fgIsSparseChannelValid == TRUE) {
		if (prSparseBand)
			*prSparseBand = prScanInfo->rSparseChannel.eBand;

		if (pucSparseChannel)
			*pucSparseChannel = prScanInfo->rSparseChannel.ucChannelNum;

		return TRUE;
	} else {
		return FALSE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        Event handler for NLO done event
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID scnEventNloDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_NLO_DONE_T prNloDone)
{
	P_SCAN_INFO_T prScanInfo;
	P_NLO_PARAM_T prNloParam;
	P_SCAN_PARAM_T prScanParam;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	if (prScanInfo->fgNloScanning == TRUE) {
		DBGLOG(SCN, INFO, "scnEventNloDone Current State = %d\n", prScanInfo->eCurrentState);

		kalSchedScanResults(prAdapter->prGlueInfo);

		if (prNloParam->fgStopAfterIndication == TRUE)
			prScanInfo->fgNloScanning = FALSE;

		kalMemZero(&prNloParam->aprPendingBssDescToInd[0],
					CFG_SCAN_SSID_MATCH_MAX_NUM * sizeof(P_BSS_DESC_T));
	} else {
		DBGLOG(SCN, INFO, "Unexpected NLO-DONE event\n");
	}

}


/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for starting scheduled scan
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN
scnFsmSchedScanRequest(IN P_ADAPTER_T prAdapter,
		       IN UINT_8 ucSsidNum,
		       IN P_PARAM_SSID_T prSsid, IN UINT_32 u4IeLength, IN PUINT_8 pucIe, IN UINT_16 u2Interval)
{
	P_SCAN_INFO_T prScanInfo;
	P_NLO_PARAM_T prNloParam;
	P_SCAN_PARAM_T prScanParam;
	P_CMD_NLO_REQ prCmdNloReq;
	UINT_32 i, j;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	if (prScanInfo->fgNloScanning) {
		DBGLOG(SCN, INFO, "prScanInfo->fgNloScanning == TRUE  already scanning\n");
		return TRUE;
	}

	prScanInfo->fgNloScanning = TRUE;

	/* 1. load parameters */
	prScanParam->ucSeqNum++;
	/* prScanParam->ucBssIndex             = prAdapter->prAisBssInfo->ucBssIndex; */

	prNloParam->fgStopAfterIndication = TRUE;
	prNloParam->ucFastScanIteration = 0;
	prNloParam->u2FastScanPeriod = u2Interval;
	prNloParam->u2SlowScanPeriod = u2Interval;

	if (prScanParam->ucSSIDNum > CFG_SCAN_SSID_MAX_NUM)
		prScanParam->ucSSIDNum = CFG_SCAN_SSID_MAX_NUM;
	else
		prScanParam->ucSSIDNum = ucSsidNum;

	if (prNloParam->ucMatchSSIDNum > CFG_SCAN_SSID_MATCH_MAX_NUM)
		prNloParam->ucMatchSSIDNum = CFG_SCAN_SSID_MATCH_MAX_NUM;
	else
		prNloParam->ucMatchSSIDNum = ucSsidNum;

	for (i = 0; i < prNloParam->ucMatchSSIDNum; i++) {
		if (i < CFG_SCAN_SSID_MAX_NUM) {
			COPY_SSID(prScanParam->aucSpecifiedSSID[i],
				  prScanParam->ucSpecifiedSSIDLen[i], prSsid[i].aucSsid, (UINT_8) prSsid[i].u4SsidLen);
		}

		COPY_SSID(prNloParam->aucMatchSSID[i],
			  prNloParam->ucMatchSSIDLen[i], prSsid[i].aucSsid, (UINT_8) prSsid[i].u4SsidLen);

		prNloParam->aucCipherAlgo[i] = 0;
		prNloParam->au2AuthAlgo[i] = 0;

		for (j = 0; j < SCN_NLO_NETWORK_CHANNEL_NUM; j++)
			prNloParam->aucChannelHint[i][j] = 0;
	}

	/* 2. prepare command for sending */
	prCmdNloReq = (P_CMD_NLO_REQ) cnmMemAlloc(prAdapter, RAM_TYPE_BUF, sizeof(CMD_NLO_REQ) + prScanParam->u2IELen);

	if (!prCmdNloReq) {
		ASSERT(0);	/* Can't initiate NLO operation */
		return FALSE;
	}

	/* 3. send command packet for NLO operation */
	kalMemZero(prCmdNloReq, sizeof(CMD_NLO_REQ));

	prCmdNloReq->ucSeqNum = prScanParam->ucSeqNum;
	/* prCmdNloReq->ucBssIndex             = prScanParam->ucBssIndex; */

	prCmdNloReq->ucNetworkType = prScanParam->eNetTypeIndex;
	/* prCmdNloReq->ucScanType = (UINT_8) prScanParam->eScanType; */ /* sync to firmware */

	prCmdNloReq->fgStopAfterIndication = prNloParam->fgStopAfterIndication;
	prCmdNloReq->ucFastScanIteration = prNloParam->ucFastScanIteration;
	prCmdNloReq->u2FastScanPeriod = prNloParam->u2FastScanPeriod;
	prCmdNloReq->u2SlowScanPeriod = prNloParam->u2SlowScanPeriod;
	prCmdNloReq->ucEntryNum = prNloParam->ucMatchSSIDNum;
	for (i = 0; i < prNloParam->ucMatchSSIDNum; i++) {
		COPY_SSID(prCmdNloReq->arNetworkList[i].aucSSID,
			  prCmdNloReq->arNetworkList[i].ucSSIDLength,
			  prNloParam->aucMatchSSID[i], prNloParam->ucMatchSSIDLen[i]);

		prCmdNloReq->arNetworkList[i].ucCipherAlgo = prNloParam->aucCipherAlgo[i];
		prCmdNloReq->arNetworkList[i].u2AuthAlgo = prNloParam->au2AuthAlgo[i];

		for (j = 0; j < SCN_NLO_NETWORK_CHANNEL_NUM; j++)
			prCmdNloReq->arNetworkList[i].ucNumChannelHint[j] = prNloParam->aucChannelHint[i][j];
	}

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdNloReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdNloReq->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prCmdNloReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdNloReq->u2IELen);
#if !CFG_SUPPORT_GSCN

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_NLO_REQ,
			    TRUE,
			    FALSE,
			    TRUE,
			    nicCmdEventSetCommon,
			    nicOidCmdTimeoutCommon,
			    sizeof(CMD_NLO_REQ) + prCmdNloReq->u2IELen, (PUINT_8) prCmdNloReq, NULL, 0);

#else
	scnCombineParamsIntoPSCN(prAdapter, prCmdNloReq, NULL, NULL, NULL, FALSE, FALSE, FALSE);
	scnPSCNFsm(prAdapter, PSCN_RESET);
#endif
	cnmMemFree(prAdapter, (PVOID) prCmdNloReq);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for stopping scheduled scan
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmSchedScanStopRequest(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_NLO_PARAM_T prNloParam;
	P_SCAN_PARAM_T prScanParam;
	CMD_NLO_CANCEL rCmdNloCancel;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	/* send cancel message to firmware domain */
	rCmdNloCancel.ucSeqNum = prScanParam->ucSeqNum;

#if !CFG_SUPPORT_GSCN

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_NLO_CANCEL,
			    TRUE,
			    FALSE,
			    TRUE,
			    nicCmdEventSetStopSchedScan,
			    nicOidCmdTimeoutCommon, sizeof(CMD_NLO_CANCEL), (PUINT_8)(&rCmdNloCancel), NULL, 0);
#else
	scnCombineParamsIntoPSCN(prAdapter, NULL, NULL, NULL, NULL, TRUE, FALSE, FALSE);
	scnPSCNFsm(prAdapter, PSCN_RESET);
#endif

	prScanInfo->fgNloScanning = FALSE;

	return TRUE;
}

#if CFG_SUPPORT_GSCN
/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for Set PSCN action
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmPSCNAction(IN P_ADAPTER_T prAdapter, IN ENUM_PSCAN_ACT_T ucPscanAct)
{
	P_SCAN_INFO_T prScanInfo;
	CMD_SET_PSCAN_ENABLE rCmdPscnAction;

	DBGLOG(SCN, INFO, "scnFsmPSCNAction Act = %d\n", ucPscanAct);

	kalMemZero(&rCmdPscnAction, sizeof(CMD_SET_PSCAN_ENABLE));
	rCmdPscnAction.ucPscanAct = ucPscanAct;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	if (ucPscanAct == PSCAN_ACT_ENABLE)
		prScanInfo->fgPscnOngoing = TRUE;
	else
		prScanInfo->fgPscnOngoing = FALSE;

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SET_PSCN_ENABLE,
			    TRUE,
			    FALSE,
			    FALSE,
			    nicCmdEventSetCommon,
			    nicOidCmdTimeoutCommon,
			    sizeof(CMD_SET_PSCAN_ENABLE), (PUINT_8)&rCmdPscnAction, NULL, 0);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Set PSCN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmPSCNSetParam(IN P_ADAPTER_T prAdapter, IN P_CMD_SET_PSCAN_PARAM prCmdPscnParam)
{
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	/*prCmdPscnParam->u4BasePeriod = prCmdPscnParam->u4BasePeriod;*/

	DBGLOG(SCN, INFO, "fgNLOScnEnable=%d %d %d, basePeriod=%d\n",
		prCmdPscnParam->fgNLOScnEnable, prCmdPscnParam->fgBatchScnEnable,
		prCmdPscnParam->fgGScnEnable, prCmdPscnParam->u4BasePeriod);

	if (1 /*prScanInfo->fgPscnOnnning == FALSE */) {
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCAN_PARAM,
				    TRUE,
				    FALSE,
				    FALSE,
				    nicCmdEventSetCommon,
				    nicOidCmdTimeoutCommon,
				    sizeof(CMD_SET_PSCAN_PARAM), (PUINT_8) prCmdPscnParam, NULL, 0);

		return TRUE;
	}
	return FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Set hotlist
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmPSCNSetHotlist(IN P_ADAPTER_T prAdapter, IN P_CMD_SET_PSCAN_ADD_HOTLIST_BSSID prCmdPscnAddHotlist)
{
	CMD_SET_PSCAN_ADD_HOTLIST_BSSID rCmdPscnAddHotlist;
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	memcpy(&rCmdPscnAddHotlist.aucMacAddr, &(prCmdPscnAddHotlist->aucMacAddr), sizeof(MAC_ADDR_LEN));

	/* rCmdPscnAddHotlist.aucMacAddr = prCmdPscnAddHotlist->aucMacAddr; */
	rCmdPscnAddHotlist.ucFlags = prCmdPscnAddHotlist->ucFlags;

	if (prScanInfo->fgPscnOngoing && prScanInfo->prPscnParam->fgGScnEnable) {
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCN_ADD_HOTLIST_BSSID,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    nicOidCmdTimeoutCommon,
				    sizeof(CMD_SET_PSCAN_ADD_HOTLIST_BSSID), (PUINT_8) &rCmdPscnAddHotlist, NULL, 0);
		return TRUE;
	}
	/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
	return FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for Set CMD_ID_SET_PSCN_ADD_SW_BSSID
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmPSCNAddSWCBssId(IN P_ADAPTER_T prAdapter, IN P_CMD_SET_PSCAN_ADD_SWC_BSSID prCmdPscnAddSWCBssId)
{
	CMD_SET_PSCAN_ADD_SWC_BSSID rCmdPscnAddSWCBssId;
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	memcpy(&rCmdPscnAddSWCBssId.aucMacAddr, &(prCmdPscnAddSWCBssId->aucMacAddr), sizeof(MAC_ADDR_LEN));

	/* rCmdPscnAddSWCBssId.aucMacAddr = prCmdPscnAddSWCBssId->aucMacAddr; */
	rCmdPscnAddSWCBssId.i4RssiHighThreshold = prCmdPscnAddSWCBssId->i4RssiHighThreshold;
	rCmdPscnAddSWCBssId.i4RssiLowThreshold = prCmdPscnAddSWCBssId->i4RssiLowThreshold;

	if (prScanInfo->fgPscnOngoing && prScanInfo->prPscnParam->fgGScnEnable) {
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCN_ADD_SW_BSSID,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    nicOidCmdTimeoutCommon,
				    sizeof(CMD_SET_PSCAN_ADD_SWC_BSSID), (PUINT_8) &rCmdPscnAddSWCBssId, NULL, 0);
		return TRUE;
	}
	/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
	return FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for Set CMD_ID_SET_PSCN_MAC_ADDR
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmPSCNSetMacAddr(IN P_ADAPTER_T prAdapter, IN P_CMD_SET_PSCAN_MAC_ADDR prCmdPscnSetMacAddr)
{
	CMD_SET_PSCAN_MAC_ADDR rCmdPscnSetMacAddr;
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	/* rCmdPscnSetMacAddr.aucMacAddr = prCmdPscnSetMacAddr->aucMacAddr; */
	memcpy(&rCmdPscnSetMacAddr.aucMacAddr, &(prCmdPscnSetMacAddr->aucMacAddr), sizeof(MAC_ADDR_LEN));

	rCmdPscnSetMacAddr.ucFlags = prCmdPscnSetMacAddr->ucFlags;
	rCmdPscnSetMacAddr.ucVersion = prCmdPscnSetMacAddr->ucVersion;

	if (1 /* (prScanInfo->fgPscnOnnning == TRUE */) {
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCN_MAC_ADDR,
				    TRUE,
				    FALSE,
				    FALSE,
				    NULL,
				    nicOidCmdTimeoutCommon,
				    sizeof(CMD_SET_PSCAN_MAC_ADDR), (PUINT_8)&rCmdPscnSetMacAddr, NULL, 0);
		return TRUE;
	}
	/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
	return FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine PNO Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnSubCombineNLOtoPSCN(IN P_ADAPTER_T prAdapter, IN P_CMD_NLO_REQ prNewCmdNloReq)
{
	P_SCAN_INFO_T prScanInfo;
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam;

	if (prNewCmdNloReq) {
		prCmdPscnParam->fgNLOScnEnable = TRUE;
		kalMemCopy(&(prCmdPscnParam->rCmdNloReq), prNewCmdNloReq, sizeof(CMD_NLO_REQ));
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine Batch Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnSubCombineBatchSCNtoPSCN(IN P_ADAPTER_T prAdapter, IN P_CMD_BATCH_REQ_T prNewCmdBatchReq)
{
	P_SCAN_INFO_T prScanInfo;
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam;

	if (prNewCmdBatchReq) {
		prCmdPscnParam->fgBatchScnEnable = TRUE;
		kalMemCopy(&(prCmdPscnParam->rCmdBatchReq), prNewCmdBatchReq, sizeof(CMD_BATCH_REQ_T));
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine GSCN Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnSubCombineGSCNtoPSCN(IN P_ADAPTER_T prAdapter,
			IN P_CMD_GSCN_REQ_T prNewCmdGscnReq, IN P_CMD_GSCN_SCN_COFIG_T prNewCmdGscnConfig)
{
	P_SCAN_INFO_T prScanInfo;
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;
	UINT_32 ucPeriodMin = MAX_PERIOD;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam;
	prCmdPscnParam->fgGScnEnable = FALSE;

	DBGLOG(SCN, TRACE, "scnSubCombineGSCNtoPSCN fgGScnParamSet %d fgGScnConfigSet %d\n",
		prScanInfo->fgGScnParamSet, prScanInfo->fgGScnConfigSet);

	if (prNewCmdGscnReq) {
		DBGLOG(SCN, INFO, "setup prNewCmdGscnReq\n");
		prScanInfo->fgGScnParamSet = TRUE;
		kalMemCopy(&(prCmdPscnParam->rCmdGscnReq), prNewCmdGscnReq, sizeof(CMD_GSCN_REQ_T));
		if (prNewCmdGscnReq->u4BasePeriod < ucPeriodMin)
			prCmdPscnParam->u4BasePeriod = prNewCmdGscnReq->u4BasePeriod;
	} else if (prScanInfo->fgGScnParamSet) {
		DBGLOG(SCN, INFO, "no new prNewCmdGscnReq but there is an old one\n");
	}

	if (prNewCmdGscnConfig) {
		DBGLOG(SCN, INFO, "setup prNewCmdGscnConfig\n");
		prScanInfo->fgGScnConfigSet = TRUE;
		prCmdPscnParam->fgGScnEnable = TRUE;
		prCmdPscnParam->rCmdGscnReq.u4MaxApPerScan = prNewCmdGscnConfig->ucNumApPerScn;
		prCmdPscnParam->rCmdGscnReq.u4BufferThreshold = prNewCmdGscnConfig->u4BufferThreshold;
		prCmdPscnParam->rCmdGscnReq.ucNumScnToCache = (UINT_8) prNewCmdGscnConfig->u4NumScnToCache;
	} else if (prScanInfo->fgGScnConfigSet) {
		DBGLOG(SCN, INFO, "no new prNewCmdGscnConfig but there is an old one\n");
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine   GSCN Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID
scnRemoveFromPSCN(IN P_ADAPTER_T prAdapter,
		  IN BOOLEAN fgRemoveNLOfromPSCN,
		  IN BOOLEAN fgRemoveBatchSCNfromPSCN,
		  IN BOOLEAN fgRemoveGSCNfromPSCN)
{
	P_SCAN_INFO_T prScanInfo;
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam;

	DBGLOG(SCN, INFO, "remove NLO or Batch or GSCN from PSCN--->NLO=%d, BSN=%d, GSN=%d\n",
		fgRemoveNLOfromPSCN, fgRemoveBatchSCNfromPSCN, fgRemoveGSCNfromPSCN);

	if (fgRemoveNLOfromPSCN) {
		prCmdPscnParam->fgNLOScnEnable = FALSE;
		kalMemZero(&prCmdPscnParam->rCmdNloReq, sizeof(CMD_NLO_REQ));
	}
	if (fgRemoveBatchSCNfromPSCN) {
		prCmdPscnParam->fgBatchScnEnable = FALSE;
		kalMemZero(&prCmdPscnParam->rCmdBatchReq, sizeof(CMD_BATCH_REQ_T));
	}
	if (fgRemoveGSCNfromPSCN) {
		prCmdPscnParam->fgGScnEnable = FALSE;
		prScanInfo->fgGScnParamSet = FALSE;
		prScanInfo->fgGScnConfigSet = FALSE;
		kalMemZero(&prCmdPscnParam->rCmdGscnReq, sizeof(CMD_GSCN_REQ_T));
	}

	/* sync to firmware*/
	if (fgRemoveNLOfromPSCN || fgRemoveBatchSCNfromPSCN || fgRemoveGSCNfromPSCN) {
		prScanInfo->fgPscnOngoing = FALSE;
		scnFsmPSCNAction(prAdapter, PSCAN_ACT_DISABLE);
		scnFsmPSCNSetParam(prAdapter, prCmdPscnParam);
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine GSCN , Batch, PNO Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN
scnCombineParamsIntoPSCN(IN P_ADAPTER_T prAdapter,
			 IN P_CMD_NLO_REQ prNewCmdNloReq,
			 IN P_CMD_BATCH_REQ_T prNewCmdBatchReq,
			 IN P_CMD_GSCN_REQ_T prNewCmdGscnReq,
			 IN P_CMD_GSCN_SCN_COFIG_T prNewCmdGscnConfig,
			 IN BOOLEAN fgRemoveNLOfromPSCN,
			 IN BOOLEAN fgRemoveBatchSCNfromPSCN, IN BOOLEAN fgRemoveGSCNfromPSCN)
{
	P_SCAN_INFO_T prScanInfo;
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam;

	prCmdPscnParam->ucVersion = CURRENT_PSCN_VERSION;

	if (fgRemoveNLOfromPSCN || fgRemoveBatchSCNfromPSCN || fgRemoveGSCNfromPSCN) {
		scnRemoveFromPSCN(prAdapter,
				  fgRemoveNLOfromPSCN, fgRemoveBatchSCNfromPSCN, fgRemoveGSCNfromPSCN);
	} else {
		DBGLOG(SCN, TRACE, "combine GSCN or Batch or NLO to PSCN --->\n");

		scnSubCombineNLOtoPSCN(prAdapter, prNewCmdNloReq);
		scnSubCombineBatchSCNtoPSCN(prAdapter, prNewCmdBatchReq);
		scnSubCombineGSCNtoPSCN(prAdapter, prNewCmdGscnReq, prNewCmdGscnConfig);
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Set GSCN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnSetGSCNParam(IN P_ADAPTER_T prAdapter, IN P_PARAM_WIFI_GSCAN_CMD_PARAMS prCmdGscnParam)
{
	P_CMD_GSCN_REQ_T prCmdGscnReq;
	UINT_8 i = 0, j = 0; /*ucBucketIndex, ucChIndex*/

	ASSERT(prAdapter);
	prCmdGscnReq = kalMemAlloc(sizeof(CMD_GSCN_REQ_T), VIR_MEM_TYPE);
	if (prCmdGscnReq == NULL) {
		DBGLOG(SCN, ERROR, "alloc prCmdGscnReq fail\n");
		return FALSE;
	}
	kalMemZero(prCmdGscnReq, sizeof(CMD_GSCN_REQ_T));
	prCmdGscnReq->u4NumBuckets = prCmdGscnParam->num_buckets;
	prCmdGscnReq->u4BasePeriod = prCmdGscnParam->base_period;
	DBGLOG(SCN, INFO, "u4BasePeriod[%d], u4NumBuckets[%d]\n",
		prCmdGscnReq->u4BasePeriod, prCmdGscnReq->u4NumBuckets);

	for (i = 0; i < prCmdGscnReq->u4NumBuckets; i++) {
		prCmdGscnReq->arBucket[i].u2BucketIndex =
		    (UINT_16) prCmdGscnParam->buckets[i].bucket;
		prCmdGscnReq->arBucket[i].eBand = prCmdGscnParam->buckets[i].band;
		prCmdGscnReq->arBucket[i].ucBucketFreqMultiple =
		    (prCmdGscnParam->buckets[i].period / prCmdGscnParam->base_period);
		prCmdGscnReq->arBucket[i].ucReportFlag = prCmdGscnParam->buckets[i].report_events;
		prCmdGscnReq->arBucket[i].ucMaxBucketFreqMultiple =
		    (prCmdGscnParam->buckets[i].max_period / prCmdGscnParam->base_period);
		prCmdGscnReq->arBucket[i].ucStepCount = (UINT_8)prCmdGscnParam->buckets[i].step_count;

		prCmdGscnReq->arBucket[i].ucNumChannels =
		    (UINT_8)prCmdGscnParam->buckets[i].num_channels;
		DBGLOG(SCN, TRACE, "assign %d channels to bucket[%d]\n",
			prCmdGscnReq->arBucket[i].ucNumChannels,	i);
		for (j = 0; j < prCmdGscnParam->buckets[i].num_channels; i++) {
			prCmdGscnReq->arBucket[i].arChannelList[j].ucChannelNumber =
			    (UINT_8) nicFreq2ChannelNum(prCmdGscnParam->buckets[i].channels[j].channel * 1000);
			prCmdGscnReq->arBucket[i].arChannelList[j].ucPassive =
			    (UINT_8) prCmdGscnParam->buckets[i].channels[j].passive;
			prCmdGscnReq->arBucket[i].arChannelList[j].u4DwellTimeMs =
			    prCmdGscnParam->buckets[i].channels[j].dwellTimeMs;

			DBGLOG(SCN, TRACE, "[ucChannel %d, ucPassive %d, u4DwellTimeMs %d\n",
			       prCmdGscnReq->arBucket[i].arChannelList[j].ucChannelNumber,
			       prCmdGscnReq->arBucket[i].arChannelList[j].ucPassive,
			       prCmdGscnReq->arBucket[i].arChannelList[j].u4DwellTimeMs);

		}

	}

	scnCombineParamsIntoPSCN(prAdapter, NULL, NULL, prCmdGscnReq, NULL, FALSE, FALSE, FALSE);
	scnPSCNFsm(prAdapter, PSCN_RESET);

	kalMemFree(prCmdGscnReq, VIR_MEM_TYPE, sizeof(CMD_GSCN_REQ_T));
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for scnSetGSCNConfig
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnSetGSCNConfig(IN P_ADAPTER_T prAdapter, IN P_CMD_GSCN_SCN_COFIG_T prCmdGscnScnConfig)
{
	ASSERT(prAdapter);
	ASSERT(prCmdGscnScnConfig);

	scnCombineParamsIntoPSCN(prAdapter, NULL, NULL, NULL, prCmdGscnScnConfig, FALSE, FALSE, FALSE);
	scnPSCNFsm(prAdapter, PSCN_RESET);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for scnFsmGetGSCNResult
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmGetGSCNResult(IN P_ADAPTER_T prAdapter, IN P_CMD_GET_GSCAN_RESULT_T prGetGscnResultCmd,
					OUT PUINT_32 pu4SetInfoLen)
{
	CMD_GET_GSCAN_RESULT_T rGetGscnResultCmd;
	P_SCAN_INFO_T prScanInfo;
	P_PARAM_WIFI_GSCAN_RESULT_REPORT prGscnResult;
	struct wiphy *wiphy;
	UINT_32 u4SizeofGScanResults = 0;
	UINT8 ucBkt;
	static UINT8 scanId, numAp;

	ASSERT(prAdapter);
	wiphy = priv_to_wiphy(prAdapter->prGlueInfo);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	kalMemCopy(&rGetGscnResultCmd, prGetGscnResultCmd, sizeof(CMD_GET_GSCAN_RESULT_T));
	DBGLOG(SCN, INFO, "rGetGscnResultCmd: ucGetNum[%d], fgFlush[%d]\n",
		rGetGscnResultCmd.u4Num, rGetGscnResultCmd.ucFlush);
	rGetGscnResultCmd.u4Num = (rGetGscnResultCmd.u4Num <= PSCAN_MAX_AP_CACHE_PER_SCAN)
		? rGetGscnResultCmd.u4Num : PSCAN_MAX_AP_CACHE_PER_SCAN;
	if (rGetGscnResultCmd.u4Num == 0)
		rGetGscnResultCmd.u4Num = PSCAN_MAX_AP_CACHE_PER_SCAN;

#if 0 /* get GScan results from firmware */
	if (prScanInfo->fgPscnOngoing && prScanInfo->prPscnParam->fgGScnEnable) {
		wlanSendSetQueryCmd(prAdapter,
				CMD_ID_GET_GSCN_SCN_RESULT,
				FALSE,
				TRUE,
				TRUE,
				NULL,
				nicOidCmdTimeoutCommon,
				sizeof(CMD_GET_GSCAN_RESULT_T), (PUINT_8)&rGetGscnResultCmd, NULL, *pu4SetInfoLen);
		return TRUE;
	}
#else /* get GScan results from driver */
	scanId++;
	u4SizeofGScanResults = sizeof(PARAM_WIFI_GSCAN_RESULT_REPORT)
			+ sizeof(PARAM_WIFI_GSCAN_RESULT) * (rGetGscnResultCmd.u4Num - 1);
	prGscnResult = kalMemAlloc(u4SizeofGScanResults, VIR_MEM_TYPE);
	if (!prGscnResult) {
		DBGLOG(SCN, ERROR, "Can not alloc memory for PARAM_WIFI_GSCAN_RESULT_REPORT\n");
		return -ENOMEM;
	}
	kalMemZero(prGscnResult, u4SizeofGScanResults);

	prGscnResult->u4ScanId = scanId;
	prGscnResult->ucScanFlag = 3;
	for (ucBkt = 0; ucBkt < GSCAN_MAX_BUCKETS; ucBkt++)
		if (prScanInfo->prPscnParam->rCmdGscnReq.arBucket[ucBkt].ucReportFlag == REPORT_EVENTS_EACH_SCAN
		|| prScanInfo->prPscnParam->rCmdGscnReq.arBucket[ucBkt].ucReportFlag == REPORT_EVENTS_FULL_RESULTS)
			prGscnResult->u4BucketMask |= (1 << ucBkt);

	/* copy scan results */
	{
		P_PARAM_BSSID_EX_T prScanResults;
		UINT8 i = 0, remainAp = 0;

		if (numAp < prAdapter->rWlanInfo.u4ScanResultNum)
			remainAp = prAdapter->rWlanInfo.u4ScanResultNum - numAp;
		else
			return FALSE;
		rGetGscnResultCmd.u4Num =
			(rGetGscnResultCmd.u4Num <= remainAp) ? rGetGscnResultCmd.u4Num : remainAp;

		DBGLOG(SCN, INFO, "u4Num=%d, numAp=%d\n", rGetGscnResultCmd.u4Num, numAp);

		for (i = 0; i < rGetGscnResultCmd.u4Num; i++) {
			prScanResults = &(prAdapter->rWlanInfo.arScanResult[numAp]);
			prGscnResult->rResult[i].ts = kalGetBootTime();
			if (prScanResults->rSsid.u4SsidLen <= ELEM_MAX_LEN_SSID)
				kalMemCopy(prGscnResult->rResult[i].ssid,
					prScanResults->rSsid.aucSsid,
					prScanResults->rSsid.u4SsidLen);
			kalMemCopy(prGscnResult->rResult[i].bssid,
				prScanResults->arMacAddress, MAC_ADDR_LEN);
			prGscnResult->rResult[i].channel = prScanResults->rConfiguration.u4DSConfig / 1000;
			prGscnResult->rResult[i].rssi = prScanResults->rRssi;
			prGscnResult->rResult[i].rtt = 0;
			prGscnResult->rResult[i].rtt_sd = 0;
			prGscnResult->rResult[i].beacon_period = prScanResults->rConfiguration.u4BeaconPeriod;
			prGscnResult->rResult[i].capability = prScanResults->u2CapInfo;
			prGscnResult->rResult[i].ie_length = prScanResults->u4IELength;
			/* if (prScanResults->u4IELength <= CFG_RAW_BUFFER_SIZE)
				prGscnResult->rResult[i].ie_data = prAdapter->rWlanInfo.apucScanResultIEs[numAp]; */

			numAp++;
			DBGLOG(SCN, TRACE, "Report GScan SSID[%s][%d][" MACSTR "] u4IELength=%d u2CapInfo=0x%x\n",
					prGscnResult->rResult[i].ssid, prGscnResult->rResult[i].channel,
					MAC2STR(prGscnResult->rResult[i].bssid), prScanResults->u4IELength,
					prScanResults->u2CapInfo);
		}
		if (numAp >= prAdapter->rWlanInfo.u4ScanResultNum)
			numAp = 0;
	}

	prGscnResult->u4NumOfResults = rGetGscnResultCmd.u4Num;
	u4SizeofGScanResults += sizeof(struct nlattr) * 2;
	DBGLOG(SCN, INFO, "scan_id=%d, scan_flag =%d, %d, num=%d %d %d, u4SizeofGScanResults=%d\r\n",
		prGscnResult->u4ScanId,
		prGscnResult->ucScanFlag,
		prGscnResult->u4BucketMask, numAp,
		prGscnResult->u4NumOfResults, prAdapter->rWlanInfo.u4ScanResultNum,
		u4SizeofGScanResults);

	if (numAp == rGetGscnResultCmd.u4Num) /* start transfer*/
		mtk_cfg80211_vendor_gscan_results(wiphy, prAdapter->prGlueInfo->prDevHandler->ieee80211_ptr,
			prGscnResult, u4SizeofGScanResults, TRUE, FALSE);
	mtk_cfg80211_vendor_gscan_results(wiphy, prAdapter->prGlueInfo->prDevHandler->ieee80211_ptr,
		prGscnResult, u4SizeofGScanResults, FALSE, FALSE);
	if (numAp == 0) /* end transfer */
		mtk_cfg80211_vendor_gscan_results(wiphy, prAdapter->prGlueInfo->prDevHandler->ieee80211_ptr,
			prGscnResult, u4SizeofGScanResults, TRUE, TRUE);

	kalMemFree(prGscnResult, VIR_MEM_TYPE, u4SizeofGScanResults - sizeof(struct nlattr) * 2);

#endif /* get GScan results from driver */

	return TRUE;
}

BOOLEAN scnFsmGSCNResults(IN P_ADAPTER_T prAdapter, IN P_EVENT_GSCAN_RESULT_T prEventBuffer)
{
	P_PARAM_WIFI_GSCAN_RESULT_REPORT prGscnResult;
	struct wiphy *wiphy;
	UINT_32 u4SizeofGScanResults = 0;

	prGscnResult = kalMemAlloc(sizeof(PARAM_WIFI_GSCAN_RESULT_REPORT), VIR_MEM_TYPE);
	if (!prGscnResult) {
		DBGLOG(SCN, ERROR, "Can not alloc memory for PARAM_WIFI_GSCAN_RESULT_REPORT\n");
		return -ENOMEM;
	}

	prGscnResult->u4ScanId = (UINT_32)prEventBuffer->u2ScanId;
	prGscnResult->ucScanFlag = (UINT_8)prEventBuffer->u2ScanFlags;
	prGscnResult->u4BucketMask = 1;
	prGscnResult->u4NumOfResults = (UINT_32)prEventBuffer->u2NumOfResults;

	/* PARAM_WIFI_GSCAN_RESULT similar to  WIFI_GSCAN_RESULT_T*/
	kalMemCopy(prGscnResult->rResult, prEventBuffer->rResult,
		sizeof(PARAM_WIFI_GSCAN_RESULT) * (prGscnResult->u4NumOfResults));

	u4SizeofGScanResults = sizeof(PARAM_WIFI_GSCAN_RESULT_REPORT) + sizeof(struct nlattr) * 2
		+ sizeof(PARAM_WIFI_GSCAN_RESULT) * (prGscnResult->u4NumOfResults - 1);
	DBGLOG(SCN, INFO, "scan_id=%d, scan_flag=%d, %d, num=%d, u4SizeofGScanResults=%d\r\n",
		prGscnResult->u4ScanId,
		(UINT_32)prGscnResult->ucScanFlag,
		prGscnResult->u4BucketMask,
		prGscnResult->u4NumOfResults,
		u4SizeofGScanResults);

	wiphy = priv_to_wiphy(prAdapter->prGlueInfo);
	DBGLOG(SCN, INFO, "wiphy=0x%p, prAdapter=0x%p\n", wiphy, prAdapter);
	mtk_cfg80211_vendor_gscan_results(wiphy, prAdapter->prGlueInfo->prDevHandler->ieee80211_ptr,
		prGscnResult, u4SizeofGScanResults, FALSE, FALSE);

	kalMemFree(prGscnResult, VIR_MEM_TYPE, sizeof(PARAM_WIFI_GSCAN_RESULT_REPORT));

	return TRUE;
}

VOID scnPSCNFsm(IN P_ADAPTER_T prAdapter, IN ENUM_PSCAN_STATE_T eNextPSCNState)
{
	P_SCAN_INFO_T prScanInfo;
	BOOLEAN fgTransitionState = FALSE;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	do {
		fgTransitionState = FALSE;

		DBGLOG(SCN, STATE, "eCurrentPSCNState=%d, eNextPSCNState=%d\n",
			prScanInfo->eCurrentPSCNState, eNextPSCNState);

		prScanInfo->eCurrentPSCNState = eNextPSCNState;

		switch (prScanInfo->eCurrentPSCNState) {
		case PSCN_IDLE:
			DBGLOG(SCN, TRACE, "PSCN_IDLE.... PSCAN_ACT_DISABLE\n");
			scnFsmPSCNAction(prAdapter, PSCAN_ACT_DISABLE);
			eNextPSCNState = PSCN_IDLE;
			break;
		case PSCN_RESET:
			DBGLOG(SCN, TRACE, "PSCN_RESET.... PSCAN_ACT_DISABLE\n");
			scnFsmPSCNAction(prAdapter, PSCAN_ACT_DISABLE);
			scnFsmPSCNSetParam(prAdapter, prScanInfo->prPscnParam);

			if (prScanInfo->prPscnParam->fgNLOScnEnable) {
				eNextPSCNState = PSCN_SCANNING; /* upper layer trigger GSCN Action */
				DBGLOG(SCN, TRACE,
				       "PSCN_RESET->PSCN_SCANNING....fgNLOScnEnable/fgBatchScnEnable/fgGScnEnable ENABLE\n");
			} else {
				/* eNextPSCNState = PSCN_RESET; */
				DBGLOG(SCN, TRACE,
				       "PSCN_RESET->PSCN_RESET....fgNLOScnEnable/fgBatchScnEnable/fgGScnEnable DISABLE\n");
			}
			break;

		case PSCN_SCANNING:
			DBGLOG(SCN, TRACE, "PSCN_SCANNING.... PSCAN_ACT_ENABLE\n");
			/* nicActivateNetwork(); */
			if (prScanInfo->fgPscnOngoing)
				break;
			scnFsmPSCNAction(prAdapter, PSCAN_ACT_ENABLE);
			prScanInfo->fgPscnOngoing = TRUE;
			eNextPSCNState = PSCN_SCANNING;
			break;

		default:
			DBGLOG(SCN, WARN, "Unexpected state\n");
			ASSERT(0);
			break;
		}

		if (prScanInfo->eCurrentPSCNState != eNextPSCNState)
			fgTransitionState = TRUE;

	} while (fgTransitionState);

}
#endif
