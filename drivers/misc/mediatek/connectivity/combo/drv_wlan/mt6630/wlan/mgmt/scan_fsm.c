/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/scan_fsm.c#2
*/

/*! \file   "scan_fsm.c"
    \brief  This file defines the state transition function for SCAN FSM.

    The SCAN FSM is part of SCAN MODULE and responsible for performing basic SCAN
    behavior as metioned in IEEE 802.11 2007 11.1.3.1 & 11.1.3.2 .
*/

/*
** Log: scan_fsm.c
**
** 08 15 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** enlarge  match_ssid_num to 16 for PNO support
**
** 08 09 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** 1. integrate scheduled scan functionality
** 2. condition compilation for linux-3.4 & linux-3.8 compatibility
** 3. correct CMD queue access to reduce lock scope
**
** 02 19 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** take use of GET_BSS_INFO_BY_INDEX() and MAX_BSS_INDEX macros
** for correctly indexing of BSS-INFO pointers
**
** 02 05 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** 1. change to use long format (FT=1) for initial command
** 2. fix a typo
**
** 01 22 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** .add driver side NLO state machine
**
** 01 22 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** modification for ucBssIndex migration
**
** 01 03 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** carry timeout value and channel dwell time value to scan module
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
**
** 08 31 2012 cp.wu
** [WCXRP00001269] [MT6620 Wi-Fi][Driver] cfg80211 porting merge back to DaVinci
** do not use fgIsP2PRegistered for checking but use network index
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

#define CURRENT_PSCN_VERSION 1
#define RSSI_MARGIN_DEFAULT  5
#define MAX_PERIOD 200000

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

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else if (prMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
					   || prMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
					scnFsmHandleScanMsgV2(prAdapter, (P_MSG_SCN_SCAN_REQ_V2) prMsgHdr);

					eNextState = SCAN_STATE_SCANNING;
					fgIsTransition = TRUE;
				} else {
					/* should not happen */
					ASSERT(0);
				}

				/* switch to next state */
				cnmMemFree(prAdapter, prMsgHdr);
			}
			break;

		case SCAN_STATE_SCANNING:
			if (prScanParam->fgIsScanV2 == FALSE)
				scnSendScanReq(prAdapter);
			else
				scnSendScanReqV2(prAdapter);
			cnmTimerStartTimer(prAdapter, &prScanInfo->rScanDoneTimer,
					   SEC_TO_MSEC(AIS_SCN_DONE_TIMEOUT_SEC));
			/* prScanInfo->ucScanDoneTimeoutCnt = 0; */
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
VOID scnSendScanReq(IN P_ADAPTER_T prAdapter)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	CMD_SCAN_REQ rCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* send command packet for scan */
	kalMemZero(&rCmdScanReq, sizeof(CMD_SCAN_REQ));

	rCmdScanReq.ucSeqNum = prScanParam->ucSeqNum;
	rCmdScanReq.ucBssIndex = prScanParam->ucBssIndex;
	rCmdScanReq.ucScanType = (UINT_8) prScanParam->eScanType;
	rCmdScanReq.ucSSIDType = prScanParam->ucSSIDType;

	if (prScanParam->ucSSIDNum == 1) {
		COPY_SSID(rCmdScanReq.aucSSID,
			  rCmdScanReq.ucSSIDLength,
			  prScanParam->aucSpecifiedSSID[0], prScanParam->ucSpecifiedSSIDLen[0]);
	}

	rCmdScanReq.ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		rCmdScanReq.ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < rCmdScanReq.ucChannelListNum; i++) {
			rCmdScanReq.arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			rCmdScanReq.arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}

	rCmdScanReq.u2ChannelDwellTime = prScanParam->u2ChannelDwellTime;
	rCmdScanReq.u2TimeoutValue = prScanParam->u2TimeoutValue;

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		rCmdScanReq.u2IELen = prScanParam->u2IELen;
	else
		rCmdScanReq.u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(rCmdScanReq.aucIE, prScanParam->aucIE, sizeof(UINT_8) * rCmdScanReq.u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ, aucIE) + rCmdScanReq.u2IELen, (PUINT_8)&rCmdScanReq, NULL, 0);
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
	CMD_SCAN_REQ_V2 rCmdScanReq;
	UINT_32 i;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	/* send command packet for scan */
	kalMemZero(&rCmdScanReq, sizeof(CMD_SCAN_REQ_V2));

	rCmdScanReq.ucSeqNum = prScanParam->ucSeqNum;
	rCmdScanReq.ucBssIndex = prScanParam->ucBssIndex;
	rCmdScanReq.ucScanType = (UINT_8) prScanParam->eScanType;
	rCmdScanReq.ucSSIDType = prScanParam->ucSSIDType;

	for (i = 0; i < prScanParam->ucSSIDNum; i++) {
		COPY_SSID(rCmdScanReq.arSSID[i].aucSsid,
			  rCmdScanReq.arSSID[i].u4SsidLen,
			  prScanParam->aucSpecifiedSSID[i], prScanParam->ucSpecifiedSSIDLen[i]);
	}

	rCmdScanReq.u2ProbeDelayTime = (UINT_8) prScanParam->u2ProbeDelayTime;
	rCmdScanReq.ucChannelType = (UINT_8) prScanParam->eScanChannel;

	if (prScanParam->eScanChannel == SCAN_CHANNEL_SPECIFIED) {
		/* P2P would use:
		 * 1. Specified Listen Channel of passive scan for LISTEN state.
		 * 2. Specified Listen Channel of Target Device of active scan for SEARCH state. (Target != NULL)
		 */
		rCmdScanReq.ucChannelListNum = prScanParam->ucChannelListNum;

		for (i = 0; i < rCmdScanReq.ucChannelListNum; i++) {
			rCmdScanReq.arChannelList[i].ucBand = (UINT_8) prScanParam->arChnlInfoList[i].eBand;

			rCmdScanReq.arChannelList[i].ucChannelNum =
			    (UINT_8) prScanParam->arChnlInfoList[i].ucChannelNum;
		}
	}

	rCmdScanReq.u2ChannelDwellTime = prScanParam->u2ChannelDwellTime;
	rCmdScanReq.u2TimeoutValue = prScanParam->u2TimeoutValue;

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		rCmdScanReq.u2IELen = prScanParam->u2IELen;
	else
		rCmdScanReq.u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(rCmdScanReq.aucIE, prScanParam->aucIE, sizeof(UINT_8) * rCmdScanReq.u2IELen);

	wlanSendSetQueryCmd(prAdapter,
			    CMD_ID_SCAN_REQ_V2,
			    TRUE,
			    FALSE,
			    FALSE,
			    NULL,
			    NULL,
			    OFFSET_OF(CMD_SCAN_REQ_V2, aucIE) + rCmdScanReq.u2IELen, (PUINT_8)&rCmdScanReq, NULL, 0);

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
		    prScanCancel->ucBssIndex == prScanParam->ucBssIndex) {
			/* send cancel message to firmware domain */
			rCmdScanCancel.ucSeqNum = prScanParam->ucSeqNum;
			rCmdScanCancel.ucIsExtChannel = (UINT_8) prScanCancel->fgIsChannelExt;

			wlanSendSetQueryCmd(prAdapter,
					    CMD_ID_SCAN_CANCEL,
					    TRUE,
					    FALSE,
					    FALSE,
					    NULL, NULL, sizeof(CMD_SCAN_CANCEL), (PUINT_8)&rCmdScanCancel, NULL, 0);

			/* generate scan-done event for caller */
			scnFsmGenerateScanDoneMsg(prAdapter,
						  prScanParam->ucSeqNum,
						  prScanParam->ucBssIndex, SCAN_STATUS_CANCELLED);

			/* switch to next pending scan */
			scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
		} else {
			scnFsmRemovePendingMsg(prAdapter, prScanCancel->ucSeqNum, prScanCancel->ucBssIndex);
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
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
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

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
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
	prScanParam->ucBssIndex = prScanReqMsg->ucBssIndex;
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

	prScanParam->u2ChannelDwellTime = prScanReqMsg->u2ChannelDwellTime;
	prScanParam->u2TimeoutValue = prScanReqMsg->u2TimeoutValue;
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
VOID scnFsmRemovePendingMsg(IN P_ADAPTER_T prAdapter, IN UINT_8 ucSeqNum, IN UINT_8 ucBssIndex)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_HDR_T prPendingMsgHdr, prPendingMsgHdrNext, prRemoveMsgHdr = NULL;
	P_LINK_ENTRY_T prRemoveLinkEntry = NULL;
	BOOLEAN fgIsRemovingScan = FALSE;

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

			if (ucSeqNum == prScanReqMsg->ucSeqNum && ucBssIndex == prScanReqMsg->ucBssIndex) {
				prRemoveLinkEntry = &(prScanReqMsg->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		} else if (prPendingMsgHdr->eMsgId == MID_AIS_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_BOW_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_P2P_SCN_SCAN_REQ_V2
			   || prPendingMsgHdr->eMsgId == MID_RLM_SCN_SCAN_REQ_V2) {
			P_MSG_SCN_SCAN_REQ_V2 prScanReqMsgV2 = (P_MSG_SCN_SCAN_REQ_V2) prPendingMsgHdr;

			if (ucSeqNum == prScanReqMsgV2->ucSeqNum && ucBssIndex == prScanReqMsgV2->ucBssIndex) {
				prRemoveLinkEntry = &(prScanReqMsgV2->rMsgHdr.rLinkEntry);
				prRemoveMsgHdr = prPendingMsgHdr;
				fgIsRemovingScan = TRUE;
			}
		}

		if (prRemoveLinkEntry) {
			if (fgIsRemovingScan == TRUE) {
				/* generate scan-done event for caller */
				scnFsmGenerateScanDoneMsg(prAdapter, ucSeqNum, ucBssIndex, SCAN_STATUS_CANCELLED);
			}

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
VOID scnEventScanDone(IN P_ADAPTER_T prAdapter, IN P_EVENT_SCAN_DONE prScanDone, BOOLEAN fgIsNewVersion)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;
	cnmTimerStopTimer(prAdapter, &prScanInfo->rScanDoneTimer);
	prScanInfo->ucScanDoneTimeoutCnt = 0;
	if (fgIsNewVersion) {
		DBGLOG(SCN, INFO,
		       "New scnEventScanDone Version%d!size of ScanDone%d,ucCompleteChanCount[%d],ucCurrentState%d\n",
		       prScanDone->ucScanDoneVersion, sizeof(EVENT_SCAN_DONE),
		       prScanDone->ucCompleteChanCount, prScanDone->ucCurrentState);

		if (prScanDone->ucScanDoneVersion > 2) {
			DBGLOG(SCN, INFO, "New u4ScanDurBcnCnt[%lu]!!! fgIsPNOenabled[%d]\n",
			       prScanDone->u4ScanDurBcnCnt, prScanDone->fgIsPNOenabled);
		}

		if (prScanDone->ucCurrentState != FW_SCAN_STATE_SCAN_DONE) {
			DBGLOG(SCN, INFO,
			       "FW Scan timeout!generate ScanDone event at State%d complete chan count%d ucChannelListNum%d\n",
			       prScanDone->ucCurrentState, prScanDone->ucCompleteChanCount,
			       prScanParam->ucChannelListNum);

		} else {
			DBGLOG(SCN, INFO, " scnEventScanDone at FW_SCAN_STATE_SCAN_DONE state\n");
		}
	} else {
		DBGLOG(SCN, INFO, "Old scnEventScanDone Version\n");
	}

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
		scnFsmGenerateScanDoneMsg(prAdapter, prScanParam->ucSeqNum, prScanParam->ucBssIndex, SCAN_STATUS_DONE);

		/* switch to next pending scan */
		scnFsmSteps(prAdapter, SCAN_STATE_IDLE);
	} else {
		DBGLOG(SCN, INFO, "Unexpected SCAN-DONE event: SeqNum = %d, Current State = %d\n",
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
			  IN UINT_8 ucSeqNum, IN UINT_8 ucBssIndex, IN ENUM_SCAN_STATUS eScanStatus)
{
	P_SCAN_INFO_T prScanInfo;
	P_SCAN_PARAM_T prScanParam;
	P_MSG_SCN_SCAN_DONE prScanDoneMsg;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prScanParam = &prScanInfo->rScanParam;

	prScanDoneMsg = (P_MSG_SCN_SCAN_DONE) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(MSG_SCN_SCAN_DONE));
	if (!prScanDoneMsg) {
		ASSERT(0);	/* Can't indicate SCAN FSM Complete */
		return;
	}

	if (prScanParam->fgIsObssScan == TRUE) {
		prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_RLM_SCAN_DONE;
	} else {
		switch (GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType) {
		case NETWORK_TYPE_AIS:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_AIS_SCAN_DONE;
			break;

		case NETWORK_TYPE_P2P:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_P2P_SCAN_DONE;
			break;

		case NETWORK_TYPE_BOW:
			prScanDoneMsg->rMsgHdr.eMsgId = MID_SCN_BOW_SCAN_DONE;
			break;

		default:
			DBGLOG(SCN, LOUD,
			       "Unexpected Network Type: %d\n",
			       GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex)->eNetworkType);
			ASSERT(0);
			break;
		}
	}

	prScanDoneMsg->ucSeqNum = ucSeqNum;
	prScanDoneMsg->ucBssIndex = ucBssIndex;
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

	if (prScanInfo->fgNloScanning == TRUE && prNloDone->ucSeqNum == prScanParam->ucSeqNum) {

		DBGLOG(SCN, INFO, "scnEventNloDone reporting to uplayer\n");

		kalSchedScanResults(prAdapter->prGlueInfo);

		if (prNloParam->fgStopAfterIndication == TRUE)
			prScanInfo->fgNloScanning = FALSE;
	} else {
		DBGLOG(SCN, INFO, "Unexpected NLO-DONE event: SeqNum = %d, Current State = %d\n",
		       prNloDone->ucSeqNum, prScanInfo->eCurrentState);
	}

}

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief        OID handler for starting scheduled scan
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

	DBGLOG(SCN, INFO, "scnFsmSchedScanRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	/* ASSERT(prScanInfo->fgNloScanning == FALSE); */
	if (prScanInfo->fgNloScanning) {
		DBGLOG(SCN, INFO, ("prScanInfo->fgNloScanning already scanning\n"));
		return FALSE;
	}

	prScanInfo->fgNloScanning = TRUE;

	/* 1. load parameters */
	prScanParam->ucSeqNum++;
	prScanParam->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	prNloParam->fgStopAfterIndication = TRUE;
	prNloParam->ucFastScanIteration = 0;

	if (u2Interval < SCAN_NLO_DEFAULT_INTERVAL) {
		u2Interval = SCAN_NLO_DEFAULT_INTERVAL;
		DBGLOG(SCN, INFO, "force interval to SCAN_NLO_DEFAULT_INTERVAL\n");
	}
	prAdapter->prAisBssInfo->fgIsPNOEnable = TRUE;

	if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
		SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		DBGLOG(SCN, INFO, "ACTIVE AIS from INACTIVE to enable PNO\n");
		/* sync with firmware */
		nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	}
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

		/*  for linux the Ciper,Auth Algo will be zero  */
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
	prCmdNloReq->ucBssIndex = prScanParam->ucBssIndex;
	prCmdNloReq->fgStopAfterIndication = prNloParam->fgStopAfterIndication;
	prCmdNloReq->ucFastScanIteration = prNloParam->ucFastScanIteration;
	prCmdNloReq->u2FastScanPeriod = prNloParam->u2FastScanPeriod;
	prCmdNloReq->u2SlowScanPeriod = prNloParam->u2SlowScanPeriod;
	prCmdNloReq->ucEntryNum = prNloParam->ucMatchSSIDNum;

#ifdef LINUX
	prCmdNloReq->ucFlag = SCAN_NLO_CHECK_SSID_ONLY;
	DBGLOG(SCN, INFO, "LINUX only check SSID for PNO SCAN\n");
#endif
	for (i = 0; i < prNloParam->ucMatchSSIDNum; i++) {
		COPY_SSID(prCmdNloReq->arNetworkList[i].aucSSID,
			  prCmdNloReq->arNetworkList[i].ucSSIDLength,
			  prNloParam->aucMatchSSID[i], prNloParam->ucMatchSSIDLen[i]);

		prCmdNloReq->arNetworkList[i].ucCipherAlgo = prNloParam->aucCipherAlgo[i];
		prCmdNloReq->arNetworkList[i].u2AuthAlgo = prNloParam->au2AuthAlgo[i];
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].aucSSID %s\n", prCmdNloReq->arNetworkList[i].aucSSID);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].ucSSIDLength %d\n",
		       prCmdNloReq->arNetworkList[i].ucSSIDLength);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].ucCipherAlgo %d\n",
		       prCmdNloReq->arNetworkList[i].ucCipherAlgo);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].u2AuthAlgo %d\n",
		       prCmdNloReq->arNetworkList[i].u2AuthAlgo);

		for (j = 0; j < SCN_NLO_NETWORK_CHANNEL_NUM; j++)
			prCmdNloReq->arNetworkList[i].ucNumChannelHint[j] = prNloParam->aucChannelHint[i][j];
	}

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdNloReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdNloReq->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prCmdNloReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdNloReq->u2IELen);

	if (wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NLO_REQ,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(CMD_NLO_REQ) + prCmdNloReq->u2IELen,
				(PUINT_8) prCmdNloReq, NULL, 0) != WLAN_STATUS_FAILURE) {
		cnmMemFree(prAdapter, (PVOID) prCmdNloReq);
	} else {
		cnmMemFree(prAdapter, (PVOID) prCmdNloReq);
		return FALSE;
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        OID handler for stopping scheduled scan
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
	WLAN_STATUS rStatus;

	ASSERT(prAdapter);
	DBGLOG(SCN, INFO, "scnFsmSchedScanStopRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	if (prAdapter->prAisBssInfo->fgIsNetRequestInActive && prAdapter->prAisBssInfo->fgIsPNOEnable) {
		UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		DBGLOG(SCN, INFO, "INACTIVE  AIS from ACTIVE to DISABLE PNO\n");
		/* sync with firmware */
		nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	} else {
		DBGLOG(SCN, INFO,
		       "fgIsNetRequestInActive %d, fgIsPNOEnable %d\n",
		       prAdapter->prAisBssInfo->fgIsNetRequestInActive, prAdapter->prAisBssInfo->fgIsPNOEnable);
	}

	prAdapter->prAisBssInfo->fgIsPNOEnable = FALSE;

	/* send cancel message to firmware domain */
	rCmdNloCancel.ucSeqNum = prScanParam->ucSeqNum;

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_SET_NLO_CANCEL, TRUE, FALSE, TRUE, nicCmdEventSetStopSchedScan,
				      nicOidCmdTimeoutCommon,
				      sizeof(CMD_NLO_CANCEL), (PUINT_8) &rCmdNloCancel, NULL, 0);

	prScanInfo->fgNloScanning = FALSE;
	if (rStatus != WLAN_STATUS_FAILURE)
		return TRUE;
	else
		return FALSE;
}

#endif

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

	DBGLOG(SCN, INFO, "scnFsmSchedScanRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	/* ASSERT(prScanInfo->fgNloScanning == FALSE); */
	if (prScanInfo->fgNloScanning) {
		DBGLOG(SCN, INFO, "prScanInfo->fgNloScanning == FALSE  already scanning\n");
		return FALSE;
	}

	prScanInfo->fgNloScanning = TRUE;

	/* 1. load parameters */
	prScanParam->ucSeqNum++;
	prScanParam->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	prNloParam->fgStopAfterIndication = TRUE;
	prNloParam->ucFastScanIteration = 0;

	if (u2Interval < SCAN_NLO_DEFAULT_INTERVAL) {
		u2Interval = SCAN_NLO_DEFAULT_INTERVAL;
		DBGLOG(SCN, INFO, "force interval to SCAN_NLO_DEFAULT_INTERVAL\n");
	}
	prAdapter->prAisBssInfo->fgIsPNOEnable = TRUE;
#if !CFG_SUPPORT_SCN_PSCN
	if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
		SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		DBGLOG(SCN, INFO, "ACTIVE AIS from INACTIVE to enable PNO\n");
		/* sync with firmware */
		nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	}
#endif
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

		/*  for linux the Ciper,Auth Algo will be zero  */
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
	prCmdNloReq->ucBssIndex = prScanParam->ucBssIndex;
	prCmdNloReq->fgStopAfterIndication = prNloParam->fgStopAfterIndication;
	prCmdNloReq->ucFastScanIteration = prNloParam->ucFastScanIteration;
	prCmdNloReq->u2FastScanPeriod = prNloParam->u2FastScanPeriod;
	prCmdNloReq->u2SlowScanPeriod = prNloParam->u2SlowScanPeriod;
	prCmdNloReq->ucEntryNum = prNloParam->ucMatchSSIDNum;

#ifdef LINUX
	prCmdNloReq->ucFlag = SCAN_NLO_CHECK_SSID_ONLY;
	DBGLOG(SCN, INFO, "LINUX only check SSID for PNO SCAN\n");
#endif
	for (i = 0; i < prNloParam->ucMatchSSIDNum; i++) {
		COPY_SSID(prCmdNloReq->arNetworkList[i].aucSSID,
			  prCmdNloReq->arNetworkList[i].ucSSIDLength,
			  prNloParam->aucMatchSSID[i], prNloParam->ucMatchSSIDLen[i]);

		prCmdNloReq->arNetworkList[i].ucCipherAlgo = prNloParam->aucCipherAlgo[i];
		prCmdNloReq->arNetworkList[i].u2AuthAlgo = prNloParam->au2AuthAlgo[i];
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].aucSSID %s\n", prCmdNloReq->arNetworkList[i].aucSSID);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].ucSSIDLength %d\n",
		       prCmdNloReq->arNetworkList[i].ucSSIDLength);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].ucCipherAlgo %d\n",
		       prCmdNloReq->arNetworkList[i].ucCipherAlgo);
		DBGLOG(SCN, INFO, "prCmdNloReq->arNetworkList[i].u2AuthAlgo %d\n",
		       prCmdNloReq->arNetworkList[i].u2AuthAlgo);

		for (j = 0; j < SCN_NLO_NETWORK_CHANNEL_NUM; j++)
			prCmdNloReq->arNetworkList[i].ucNumChannelHint[j] = prNloParam->aucChannelHint[i][j];
	}

	if (prScanParam->u2IELen <= MAX_IE_LENGTH)
		prCmdNloReq->u2IELen = prScanParam->u2IELen;
	else
		prCmdNloReq->u2IELen = MAX_IE_LENGTH;

	if (prScanParam->u2IELen)
		kalMemCopy(prCmdNloReq->aucIE, prScanParam->aucIE, sizeof(UINT_8) * prCmdNloReq->u2IELen);
#if !CFG_SUPPORT_SCN_PSCN
	if (wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NLO_REQ,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetCommon,
				nicOidCmdTimeoutCommon,
				sizeof(CMD_NLO_REQ) + prCmdNloReq->u2IELen,
				(PUINT_8) prCmdNloReq, NULL, 0) != WLAN_STATUS_FAILURE)
		return TRUE;
	else
		return FALSE;
#else
	scnPSCNFsm(prAdapter, PSCN_RESET, prCmdNloReq, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE);
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
	DBGLOG(SCN, INFO, "scnFsmSchedScanStopRequest\n");

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prNloParam = &prScanInfo->rNloParam;
	prScanParam = &prNloParam->rScanParam;

	if (prAdapter->prAisBssInfo->fgIsNetRequestInActive && prAdapter->prAisBssInfo->fgIsPNOEnable) {
		UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		DBGLOG(SCN, INFO, "INACTIVE  AIS from ACTIVE to DISABLE PNO\n");
		/* sync with firmware */
		nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	} else {
		DBGLOG(SCN, INFO, "fgIsNetRequestInActive %d, fgIsPNOEnable %d\n",
		       prAdapter->prAisBssInfo->fgIsNetRequestInActive, prAdapter->prAisBssInfo->fgIsPNOEnable);
	}

	prAdapter->prAisBssInfo->fgIsPNOEnable = FALSE;

	/* send cancel message to firmware domain */
	rCmdNloCancel.ucSeqNum = prScanParam->ucSeqNum;

#if !CFG_SUPPORT_SCN_PSCN

	if (wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NLO_CANCEL,
				TRUE,
				FALSE,
				TRUE,
				nicCmdEventSetStopSchedScan,
				nicOidCmdTimeoutCommon,
				sizeof(CMD_NLO_CANCEL), (PUINT_8) (&rCmdNloCancel), NULL, 0) != WLAN_STATUS_FAILURE)
		return TRUE;
	else
		return FALSE;
#else

	scnPSCNFsm(prAdapter, PSCN_RESET, NULL, NULL, NULL, NULL, TRUE, FALSE, FALSE, FALSE);

	prScanInfo->fgNloScanning = FALSE;

	return TRUE;

#endif
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
BOOLEAN scnFsmPSCNAction(IN P_ADAPTER_T prAdapter, IN UINT_8 ucPscanAct)
{
	CMD_SET_PSCAN_ENABLE rCmdPscnAction;
	P_SCAN_INFO_T prScanInfo;

	rCmdPscnAction.ucPscanAct = ucPscanAct;
	DBGLOG(SCN, INFO, "scnFsmPSCNAction Act = %d\n", ucPscanAct);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (ucPscanAct == DISABLE)
		prScanInfo->fgPscnOnnning = FALSE;
	if (ucPscanAct == ENABLE)
		prScanInfo->fgPscnOnnning = TRUE;

	wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_PSCN_ENABLE,
				TRUE,
				FALSE, TRUE, NULL, NULL, sizeof(CMD_SET_PSCAN_ENABLE),
				(PUINT_8) (&rCmdPscnAction), NULL, 0);

	DBGLOG(SCN, INFO, "scnFsmPSCNAction Act = %d is Set to FW\n", ucPscanAct);
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
	UINT_8 i, j;

	DEBUGFUNC("scnFsmPSCNSetParam()");
	DBGLOG(SCN, INFO, "scnFsmPSCNSetParam\n");
	ASSERT(prAdapter);

	prCmdPscnParam->u4BasePeriod = prCmdPscnParam->u4BasePeriod;
#if 1
	DBGLOG(SCN, INFO,
			"rCmdPscnParam.rCmdGscnParam : u4BasePeriod[%u], ucNumScnToCache[%u],  u4BufferThreshold[%u], u4NumBuckets[%u],fgGSCN[%d] fgNLO[%d] fgBatch[%d]\n",
			prCmdPscnParam->rCmdGscnReq.u4BasePeriod,
			prCmdPscnParam->rCmdGscnReq.ucNumScnToCache,
			prCmdPscnParam->rCmdGscnReq.u4BufferThreshold,
			prCmdPscnParam->rCmdGscnReq.u4NumBuckets, prCmdPscnParam->fgGScnEnable,
			prCmdPscnParam->fgNLOScnEnable, prCmdPscnParam->fgBatchScnEnable);

	i = j = 0;
	for (i = 0; i < prCmdPscnParam->rCmdGscnReq.u4NumBuckets; i++) {

		DBGLOG(SCN, INFO,
		       "rCmdPscnParam.rCmdGscnReq.arChannelBucket[%d] : band[%u], arChannelBucket[%u] ucNumChannels[%u], ucBucketFreqMultiple[%u] ucReportFlag[%u]\n",
		       i, prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].eBand,
		       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].u2BucketIndex,
		       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucNumChannels,
		       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucBucketFreqMultiple,
		       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucReportFlag);

		DBGLOG(SCN, INFO, "rCmdPscnParam.rCmdGscnParam.arChannelBucket[%d] has channel: ", i);

		for (j = 0; j < prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucNumChannels; j++) {
			DBGLOG(SCN, INFO, " %d,  ",
			       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].arChannelList[j].ucChannel);
		}
		DBGLOG(SCN, INFO, "\n");
	}
#endif

	if (1 /*prScanInfo->fgPscnOnnning == FALSE */) {

		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCAN_PARAM,
				    TRUE,
				    FALSE,
				    TRUE, NULL, NULL, sizeof(CMD_SET_PSCAN_PARAM), (PUINT_8) prCmdPscnParam, NULL, 0);

		DBGLOG(SCN, INFO, "CMD_ID_SET_PSCAN_PARAM  is set to FW !!!!!!!!!!\n");

		return TRUE;
	} else
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

	if (prScanInfo->fgPscnOnnning && prScanInfo->prPscnParam->fgGScnEnable) {

		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCN_ADD_HOTLIST_BSSID,
				    TRUE,
				    FALSE,
				    TRUE,
				    NULL,
				    NULL,
				    sizeof(CMD_SET_PSCAN_ADD_HOTLIST_BSSID), (PUINT_8) (&rCmdPscnAddHotlist), NULL, 0);

	} else {

		/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
		return FALSE;
	}

	return TRUE;
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

	if (prScanInfo->fgPscnOnnning && prScanInfo->prPscnParam->fgGScnEnable) {

		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_PSCN_ADD_SW_BSSID,
				    TRUE,
				    FALSE,
				    TRUE,
				    NULL,
				    NULL,
				    sizeof(CMD_SET_PSCAN_ADD_SWC_BSSID), (PUINT_8) (&rCmdPscnAddSWCBssId), NULL, 0);

	} else {

		/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
		return FALSE;
	}

	return TRUE;
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
				    TRUE,
				    NULL,
				    NULL, sizeof(CMD_SET_PSCAN_MAC_ADDR), (PUINT_8) (&rCmdPscnSetMacAddr), NULL, 0);

	} else {

		/* debug msg, No PSCN, Sched SCAN no need to add the hotlist ??? */
		return FALSE;
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

	CMD_GSCN_REQ_T rCmdGscnParam;
	P_SCAN_INFO_T prScanInfo;
	UINT_8 ucChannelBuckIndex = 0;
	UINT_8 i = 0;

	DBGLOG(SCN, INFO, "scnSetGSCNParam\n");
	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	rCmdGscnParam.u4NumBuckets = prCmdGscnParam->num_buckets;
	rCmdGscnParam.u4BasePeriod = prCmdGscnParam->base_period;
	DBGLOG(SCN, INFO, "u4BasePeriod[%d], u4NumBuckets[%d]: ", rCmdGscnParam.u4BasePeriod,
	       rCmdGscnParam.u4NumBuckets);
	DBGLOG(SCN, INFO, "\n");
	for (ucChannelBuckIndex = 0; ucChannelBuckIndex < prCmdGscnParam->num_buckets; ucChannelBuckIndex++) {

		DBGLOG(SCN, INFO, "assign channels to bucket[%d]:\n", ucChannelBuckIndex);

		for (i = 0; i < prCmdGscnParam->buckets[ucChannelBuckIndex].num_channels; i++) {

			rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].ucChannel =
			    (UINT_8) nicFreq2ChannelNum(prCmdGscnParam->buckets[ucChannelBuckIndex].
							channels[i].channel * 1000);
			rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].ucPassive =
			    (UINT_8) prCmdGscnParam->buckets[ucChannelBuckIndex].channels[i].passive;
			rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].u4DwellTimeMs =
			    (UINT_8) prCmdGscnParam->buckets[ucChannelBuckIndex].channels[i].dwellTimeMs;

			DBGLOG(SCN, INFO, " [ucChannel %d, ucPassive %d, u4DwellTimeMs %d\n",
			       rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].ucChannel,
			       rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].ucPassive,
			       rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].arChannelList[i].u4DwellTimeMs);

		}
		rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].u2BucketIndex =
		    (UINT_16) prCmdGscnParam->buckets[ucChannelBuckIndex].bucket;
		rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].eBand =
		    prCmdGscnParam->buckets[ucChannelBuckIndex].band;
		rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].ucBucketFreqMultiple =
		    (prCmdGscnParam->buckets[ucChannelBuckIndex].period / prCmdGscnParam->base_period);
		rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].ucNumChannels =
		    prCmdGscnParam->buckets[ucChannelBuckIndex].num_channels;
		rCmdGscnParam.arChannelBucket[ucChannelBuckIndex].ucReportFlag =
		    prCmdGscnParam->buckets[ucChannelBuckIndex].report_events;

		/* printk("\n"); */
		/* TODO: This is not make sense at all, always return true*/
	}

	DBGLOG(SCN, INFO, "scnSetGSCNParam  ---> scnPSCNFsm PSCN_RESET\n");
	DBGLOG(SCN, INFO, "/*  setup new PSCN cmd */\n");

	DBGLOG(SCN, INFO, "scnSetGSCNParam  ---> scnPSCNFsm PSCN_RESET\n");
	DBGLOG(SCN, INFO, "/*  setup new PSCN cmd */\n");

	scnPSCNFsm(prAdapter, PSCN_RESET, NULL, NULL, &rCmdGscnParam, NULL, FALSE, FALSE, FALSE, FALSE);

	DBGLOG(SCN, INFO, "scnSetGSCNParam <-----\n");

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine   PNO Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/

VOID
scnSubCombineNLOtoPSCN(IN P_ADAPTER_T prAdapter,
		       IN P_CMD_NLO_REQ prNewCmdNloReq, IN P_CMD_SET_PSCAN_PARAM prCmdPscnParam)
{
	P_SCAN_INFO_T prScanInfo;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	DBGLOG(SCN, INFO, " scnSubCombineNLOtoPSCN\n");
	ASSERT(prAdapter);

	if (prNewCmdNloReq) {
		prCmdPscnParam->fgNLOScnEnable = TRUE;
		memcpy(&prCmdPscnParam->rCmdNloReq, prNewCmdNloReq, sizeof(CMD_NLO_REQ));
	} else if (prScanInfo->prPscnParam->fgNLOScnEnable) {
		memcpy(&prCmdPscnParam->rCmdNloReq, &prScanInfo->prPscnParam->rCurrentCmdNloReq, sizeof(CMD_NLO_REQ));
	} else
		prCmdPscnParam->fgNLOScnEnable = FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief        handler for Combine   Batcht Scan params into PSCAN param
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/

VOID
scnSubCombineBatchSCNtoPSCN(IN P_ADAPTER_T prAdapter,
			    IN P_CMD_BATCH_REQ_T prNewCmdBatchReq, IN P_CMD_SET_PSCAN_PARAM prCmdPscnParam)
{
	P_SCAN_INFO_T prScanInfo;

	DBGLOG(SCN, INFO, " scnSubCombineBatchSCNtoPSCN\n");
	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (prNewCmdBatchReq) {
		prCmdPscnParam->fgBatchScnEnable = TRUE;
		memcpy(&prCmdPscnParam->rCmdBatchReq, prNewCmdBatchReq, sizeof(CMD_BATCH_REQ_T));
	} else if (prScanInfo->prPscnParam->fgBatchScnEnable) {
		memcpy(&prCmdPscnParam->rCmdBatchReq, &prScanInfo->prPscnParam->rCurrentCmdBatchReq,
		       sizeof(CMD_BATCH_REQ_T));
	} else
		prCmdPscnParam->fgBatchScnEnable = FALSE;

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
scnSubCombineGSCNtoPSCN(IN P_ADAPTER_T prAdapter,
			IN P_CMD_GSCN_REQ_T prNewCmdGscnReq,
			IN P_CMD_GSCN_SCN_COFIG_T prNewCmdGscnConfig, IN P_CMD_SET_PSCAN_PARAM prCmdPscnParam)
{
	P_SCAN_INFO_T prScanInfo;
	UINT_32 ucPeriodMin = MAX_PERIOD;

	DBGLOG(SCN, INFO, " scnSubCombineGSCNtoPSCN\n");

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	prCmdPscnParam->fgGScnEnable = FALSE;

	DBGLOG(SCN, INFO, "scnSubCombineGSCNtoPSCN fgGScnParamSet %d fgGScnConfigSet %d\n",
	       prScanInfo->fgGScnParamSet, prScanInfo->fgGScnConfigSet);

	if (prNewCmdGscnReq) {
		DBGLOG(SCN, INFO, "setup prNewCmdGscnReq\n");
		prScanInfo->fgGScnParamSet = TRUE;
		memcpy(&prCmdPscnParam->rCmdGscnReq, prNewCmdGscnReq, sizeof(CMD_GSCN_REQ_T));

		if (ucPeriodMin > prNewCmdGscnReq->u4BasePeriod)
			prCmdPscnParam->u4BasePeriod = prNewCmdGscnReq->u4BasePeriod;

	} else if (prScanInfo->fgGScnParamSet) {
		DBGLOG(SCN, INFO, "no new prNewCmdGscnReq but there is a old one\n");
		memcpy(&prCmdPscnParam->rCmdGscnReq, &prScanInfo->prPscnParam->rCurrentCmdGscnReq,
		       sizeof(CMD_GSCN_REQ_T));
	} else
		prScanInfo->fgGScnParamSet = FALSE;

	if (prNewCmdGscnConfig) {
		DBGLOG(SCN, INFO, "set up prNewCmdGscnConfig\n");
		memcpy(prCmdPscnParam, prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
		prScanInfo->fgGScnConfigSet = TRUE;
		prCmdPscnParam->rCmdGscnReq.u4BufferThreshold = prNewCmdGscnConfig->u4BufferThreshold;
		prCmdPscnParam->rCmdGscnReq.ucNumScnToCache = (UINT_8) prNewCmdGscnConfig->u4NumScnToCache;
		prCmdPscnParam->rCmdGscnReq.u4MaxApPerScan = (UINT_8) prNewCmdGscnConfig->ucNumApPerScn;
	} else if (prScanInfo->fgGScnConfigSet) {
		DBGLOG(SCN, INFO, "no new prNewCmdGscnConfig but there is a old one\n");
		memcpy(prCmdPscnParam, prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
	} else
		prScanInfo->fgGScnConfigSet = FALSE;

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
		  IN BOOLEAN fgRemoveGSCNfromPSCN, IN P_CMD_SET_PSCAN_PARAM prCmdPscnParam)
{
	UINT_8 ucPscanAct = DISABLE;
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	{
		DBGLOG(SCN, INFO, "remove NLO or Batch or GSCN from PSCN--->\n");

		if (fgRemoveNLOfromPSCN) {
			DBGLOG(SCN, INFO, "DisableNLO\n");
			memcpy(prCmdPscnParam, prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
			prCmdPscnParam->fgNLOScnEnable = FALSE;
			kalMemZero(&prCmdPscnParam->rCmdNloReq, sizeof(CMD_NLO_REQ));
			kalMemZero(&prScanInfo->prPscnParam->rCurrentCmdNloReq, sizeof(CMD_NLO_REQ));
		}
		if (fgRemoveBatchSCNfromPSCN) {
			DBGLOG(SCN, INFO, "DisableBatch\n");
			memcpy(prCmdPscnParam, prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
			prCmdPscnParam->fgBatchScnEnable = FALSE;
			kalMemZero(&prCmdPscnParam->rCmdBatchReq, sizeof(CMD_BATCH_REQ_T));
			kalMemZero(&prScanInfo->prPscnParam->rCurrentCmdBatchReq, sizeof(CMD_BATCH_REQ_T));
		}
		if (fgRemoveGSCNfromPSCN) {
			DBGLOG(SCN, INFO, "DisableGSCN\n");
			memcpy(prCmdPscnParam, prScanInfo->prPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
			prCmdPscnParam->fgGScnEnable = FALSE;
			prScanInfo->fgGScnParamSet = FALSE;
			prScanInfo->fgGScnConfigSet = FALSE;
			kalMemZero(&prCmdPscnParam->rCmdGscnReq, sizeof(CMD_GSCN_REQ_T));
			kalMemZero(&prScanInfo->prPscnParam->rCurrentCmdGscnReq, sizeof(CMD_GSCN_REQ_T));
		}

		if (!fgRemoveNLOfromPSCN && !fgRemoveBatchSCNfromPSCN && !fgRemoveGSCNfromPSCN) {
			/* prCmdPscnParam->fgIsPeriodicallyScn = FALSE; */
			prScanInfo->fgPscnOnnning = FALSE;
			scnFsmPSCNSetParam(prAdapter, prCmdPscnParam);
			ucPscanAct = DISABLE;
			scnFsmPSCNAction(prAdapter, ucPscanAct);
			DBGLOG(SCN, INFO,
			       "(!fgRemoveNLOfromPSCN && !fgRemoveBatchSCNfromPSCN && !fgRemoveGSCNfromPSCN)<-----\n");
		} else {
			/* prCmdPscnParam->fgIsPeriodicallyScn = TRUE; */
			scnFsmPSCNSetParam(prAdapter, prCmdPscnParam);
			DBGLOG(SCN, INFO, " disable NLO or GSCN or Batch bu fgIsPeriodicallyScn = TRUE <-----\n");
		}
	}

}

#if 1

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
	/* CMD_SET_PSCAN_PARAM rCmdPscnParam; */
	P_CMD_SET_PSCAN_PARAM prCmdPscnParam;
	UINT_8 i, j = 0;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prCmdPscnParam = (P_CMD_SET_PSCAN_PARAM) kalMemAlloc(sizeof(CMD_SET_PSCAN_PARAM), VIR_MEM_TYPE);
	if (!prCmdPscnParam) {
		DBGLOG(INIT, INFO, "Can not alloc memory for PARAM_WIFI_GSCAN_CMD_PARAMS\n");
		return -ENOMEM;
	}

	prCmdPscnParam->ucVersion = CURRENT_PSCN_VERSION;

	DBGLOG(SCN, INFO, "scnCombineParamsIntoPSCN\n");

	if (fgRemoveNLOfromPSCN || fgRemoveBatchSCNfromPSCN || fgRemoveGSCNfromPSCN) {
		DBGLOG(INIT, TRACE, "remove NLO or Batch or GSCN from PSCN--->\n");
		scnRemoveFromPSCN(prAdapter,
				  fgRemoveNLOfromPSCN, fgRemoveBatchSCNfromPSCN, fgRemoveGSCNfromPSCN, prCmdPscnParam);

	} else {

		DBGLOG(SCN, INFO, "combine GSCN or Batch or NLO to PSCN --->\n");

		scnSubCombineNLOtoPSCN(prAdapter, prNewCmdNloReq, prCmdPscnParam);
		scnSubCombineBatchSCNtoPSCN(prAdapter, prNewCmdBatchReq, prCmdPscnParam);
		scnSubCombineGSCNtoPSCN(prAdapter, prNewCmdGscnReq, prNewCmdGscnConfig, prCmdPscnParam);

		/* scnFsmPSCNSetParam(prAdapter, prCmdPscnParam); */

		DBGLOG(SCN, INFO, "combine GSCN or Batch or NLO to PSCN <---\n");

		DBGLOG(SCN, INFO,
				"rCmdPscnParam.rCmdGscnParam : u4BasePeriod[%u], ucNumScnToCache[%u],  u4BufferThreshold[%u], u4NumBuckets[%u],fgGSCN[%d] fgNLO[%d] fgBatch[%d]\n",
				prCmdPscnParam->rCmdGscnReq.u4BasePeriod,
				prCmdPscnParam->rCmdGscnReq.ucNumScnToCache,
				prCmdPscnParam->rCmdGscnReq.u4BufferThreshold,
				prCmdPscnParam->rCmdGscnReq.u4NumBuckets, prCmdPscnParam->fgGScnEnable,
				prCmdPscnParam->fgNLOScnEnable, prCmdPscnParam->fgBatchScnEnable);
		for (i = 0; i < prCmdPscnParam->rCmdGscnReq.u4NumBuckets; i++) {

			DBGLOG(SCN, INFO,
				"rCmdPscnParam.rCmdGscnReq.arChannelBucket[%d] : band[%u], arChannelBucket[%u] ucNumChannels[%u], ucBucketFreqMultiple[%u] ucReportFlag[%u]\n",
				i, prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].eBand,
				prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].u2BucketIndex,
				prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucNumChannels,
				prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucBucketFreqMultiple,
				prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucReportFlag);

			DBGLOG(SCN, INFO, "rCmdPscnParam.rCmdGscnParam.arChannelBucket[%d] has channel: ", i);

			for (j = 0; j < prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].ucNumChannels; j++) {
				DBGLOG(SCN, INFO, " %d,  ",
				       prCmdPscnParam->rCmdGscnReq.arChannelBucket[i].arChannelList[j].ucChannel);
			}
			DBGLOG(SCN, INFO, "\n");
		}

	}

	memcpy(prScanInfo->prPscnParam, prCmdPscnParam, sizeof(CMD_SET_PSCAN_PARAM));
	DBGLOG(INIT, TRACE, "scnCombineParamsIntoPSCN <----\n");

	return TRUE;

}

#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief         handler for Set CMD_ID_SET_PSCN_MAC_ADDR
*
* \param[in]
*
* \return none  Since it return none, why doesn't use void instead of BOOLEAN
*/
/*----------------------------------------------------------------------------*/
BOOLEAN scnFsmSetGSCNConfig(IN P_ADAPTER_T prAdapter, IN P_CMD_GSCN_SCN_COFIG_T prCmdGscnScnConfig)
{
	CMD_GSCN_SCN_COFIG_T rCmdGscnScnConfig;
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	memcpy(&rCmdGscnScnConfig, prCmdGscnScnConfig, sizeof(CMD_GSCN_SCN_COFIG_T));
	DBGLOG(SCN, INFO,
	       "rCmdGscnScnConfig: u4BufferThreshold; [%d]  ucNumApPerScn [%d] ucNumScnToCache [%d]\n",
	       rCmdGscnScnConfig.u4BufferThreshold, rCmdGscnScnConfig.ucNumApPerScn, rCmdGscnScnConfig.u4NumScnToCache);

	scnPSCNFsm(prAdapter, PSCN_RESET, NULL, NULL, NULL, &rCmdGscnScnConfig, FALSE, FALSE, FALSE, FALSE);

	return TRUE;
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
BOOLEAN scnFsmGetGSCNResult(IN P_ADAPTER_T prAdapter, IN P_CMD_GET_GSCAN_RESULT_T prGetGscnScnResultCmd)
{

	CMD_GET_GSCAN_RESULT_T rGetGscnScnResultCmd;
	P_SCAN_INFO_T prScanInfo;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);
	memcpy(&rGetGscnScnResultCmd, prGetGscnScnResultCmd, sizeof(CMD_GET_GSCAN_RESULT_T));
	DBGLOG(SCN, INFO, "rGetGscnScnResultCmd: ucGetNum [%d]  fgFlush [%d]\n",
	       rGetGscnScnResultCmd.u4Num, rGetGscnScnResultCmd.ucFlush);
	ASSERT(prAdapter);
	prScanInfo->fgGscnGetResWaiting = TRUE;

	if (prScanInfo->fgPscnOnnning && prScanInfo->prPscnParam->fgGScnEnable) {
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_GET_GSCAN_RESULT,
				    TRUE,
				    FALSE,
				    TRUE,
				    NULL,
				    NULL, sizeof(CMD_GET_GSCAN_RESULT_T), (PUINT_8) (&rGetGscnScnResultCmd), NULL, 0);

		DBGLOG(SCN, INFO,
		       "SET to FW !! rGetGscnScnResultCmd: ucGetNum [%d]  fgFlush [%d]\n",
		       rGetGscnScnResultCmd.u4Num, rGetGscnScnResultCmd.ucFlush);

	} else {

		/* debug msg, No PSCN, Sched SCAN GSCN ongoing ??? */
		return FALSE;
	}

	return FALSE;
}

BOOLEAN
scnPSCNFsm(IN P_ADAPTER_T prAdapter,
	   ENUM_PSCAN_STATE_T eNextPSCNState,
	   IN P_CMD_NLO_REQ prCmdNloReq,
	   IN P_CMD_BATCH_REQ_T prCmdBatchReq,
	   IN P_CMD_GSCN_REQ_T prCmdGscnReq,
	   IN P_CMD_GSCN_SCN_COFIG_T prNewCmdGscnConfig,
	   IN BOOLEAN fgRemoveNLOfromPSCN,
	   IN BOOLEAN fgRemoveBatchSCNfromPSCN, IN BOOLEAN fgRemoveGSCNfromPSCN, IN BOOLEAN fgEnableGSCN)
{
	P_SCAN_INFO_T prScanInfo;
	BOOLEAN fgTransitionState = FALSE;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	ASSERT(prScanInfo);
	ASSERT(prAdapter);

	do {
		fgTransitionState = FALSE;
		DBGLOG(SCN, INFO, "scnPSCNFsmt eCurrentPSCNState %d\n", prScanInfo->eCurrentPSCNState);
		switch (prScanInfo->eCurrentPSCNState) {
		case PSCN_IDLE:
			DBGLOG(SCN, INFO, "PSCN_IDLE\n");
			if (eNextPSCNState == PSCN_RESET) {
				if (fgRemoveNLOfromPSCN || fgRemoveBatchSCNfromPSCN || fgRemoveGSCNfromPSCN) {
					DBGLOG(SCN, INFO, "Unexpected remove NLO/BATCH/GSCN request\n");
					eNextPSCNState = PSCN_IDLE;
					break;
				}

				if (prCmdNloReq || prCmdBatchReq) {	/*  */
					DBGLOG(SCN, INFO, "PSCN_IDLE->PSCN_RESET,.... scnFsmPSCNActionDISABLE\n");
					/*TBD check PSCAN is ongoing */
					scnFsmPSCNAction(prAdapter, DISABLE);
					break;
				}

			} else if (eNextPSCNState == PSCN_SCANNING) {
				if (fgEnableGSCN) {
					if (prScanInfo->fgPscnOnnning)
						scnFsmPSCNAction(prAdapter, DISABLE);
					if (prScanInfo->fgGScnParamSet) {
						DBGLOG(SCN, INFO,
							"PSCN_IDLE->PSCN_SCANNING,.... scnFsmPSCNActionENABLE\n");
						prScanInfo->prPscnParam->fgGScnEnable = TRUE;
						scnFsmPSCNSetParam(prAdapter, (P_CMD_SET_PSCAN_PARAM)
								   prScanInfo->prPscnParam);
						scnFsmPSCNAction(prAdapter, ENABLE);
						eNextPSCNState = PSCN_SCANNING;
					}
				}
			}

			break;

		case PSCN_RESET:
			DBGLOG(SCN, INFO, "PSCN_RESET\n");
			scnCombineParamsIntoPSCN(prAdapter,
						 prCmdNloReq,
						 prCmdBatchReq,
						 prCmdGscnReq,
						 prNewCmdGscnConfig,
						 fgRemoveNLOfromPSCN, fgRemoveBatchSCNfromPSCN, fgRemoveGSCNfromPSCN);

			if (!prScanInfo->prPscnParam->fgNLOScnEnable
			    && !prScanInfo->prPscnParam->fgBatchScnEnable && !prScanInfo->prPscnParam->fgGScnEnable) {
				DBGLOG(SCN, INFO,
				       "PSCN_RESET->PSCN_IDLE,.... fgNLOScnEnable/fgBatchScnEnable/fgGScnEnable false\n");
				eNextPSCNState = PSCN_IDLE;
			} else {
				if (prScanInfo->prPscnParam->fgNLOScnEnable
				    || prScanInfo->prPscnParam->fgBatchScnEnable) {
					scnFsmPSCNSetParam(prAdapter, (P_CMD_SET_PSCAN_PARAM) prScanInfo->prPscnParam);
					scnFsmPSCNAction(prAdapter, ENABLE);
					eNextPSCNState = PSCN_SCANNING;
					DBGLOG(SCN, INFO,
					       "PSCN_RESET->PSCN_SCANNING,.... fgNLOScnEnable/fgBatchScnEnable ENABLE\n");
				}
			}

			break;

		case PSCN_SCANNING:
			DBGLOG(SCN, INFO, "PSCN_SCANNING\n");
			if (eNextPSCNState == PSCN_RESET) {
				if (fgRemoveNLOfromPSCN || fgRemoveBatchSCNfromPSCN || fgRemoveGSCNfromPSCN) {
					DBGLOG(SCN, INFO,
					       "PSCN_SCANNING->PSCN_RESET,.... fgRemoveNLOfromPSCN/fgRemoveBatchSCNfromPSCN/fgRemoveGSCNfromPSCN\n");
					scnFsmPSCNAction(prAdapter, DISABLE);
					break;
				}

				if (prCmdNloReq || prCmdBatchReq || prCmdGscnReq || prNewCmdGscnConfig) {
					DBGLOG(SCN, INFO,
					       "PSCN_SCANNING->PSCN_RESET,.... prCmdNloReq/prCmdBatchReq/prCmdGscnReq/prNewCmdGscnConfig\n");
					scnFsmPSCNAction(prAdapter, DISABLE);
					break;
				}

			} else if (eNextPSCNState == PSCN_SCANNING) {
				if (fgEnableGSCN) {
					if (prScanInfo->prPscnParam->fgGScnEnable && (!prScanInfo->fgPscnOnnning)) {
						DBGLOG(SCN, INFO,
						       "PSCN_SCANNING->PSCN_SCANNING,.... fgGScnEnable/!fgPscnOnnning\n");
						/* scnFsmPSCNAction(prAdapter, ENABLE); */
						eNextPSCNState = PSCN_SCANNING;
					} else {

						DBGLOG(SCN, INFO,
						       "PSCN_SCANNING->PSCN_SCANNING,.... fgGScnEnable/!fgPscnOnnning\n");
					}

				}
			}

			eNextPSCNState = PSCN_SCANNING;
			break;

		default:
			DBGLOG(SCN, INFO, "Unexpected state\n");
			ASSERT(0);
			break;

		}

		DBGLOG(SCN, INFO, "eCurrentState %d , eNextPSCNState %d\n",
		       prScanInfo->eCurrentPSCNState, eNextPSCNState);
		if (prScanInfo->eCurrentPSCNState != eNextPSCNState)
			fgTransitionState = TRUE;

		prScanInfo->eCurrentPSCNState = eNextPSCNState;
	} while (fgTransitionState);

	return prScanInfo->eCurrentState;
}

VOID scnGscnGetResultReplyCheck(P_ADAPTER_T prAdapter)
{

	P_SCAN_INFO_T prScanInfo;
	UINT_32 ucCounter = 0;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	cnmTimerStartTimer(prAdapter, &prScanInfo->rWaitForGscanResutsTimer, SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));

	while (prScanInfo->fgGscnGetResWaiting) {
		if (ucCounter > 1000)
			DBGLOG(SCN, INFO, "%s wait for Results are replyed \r\n", __func__);
		ucCounter++;
	}
}

VOID scnGscnGetResultReplyCheckTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{
	P_SCAN_INFO_T prScanInfo;

	ASSERT(prAdapter);
	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	prScanInfo->fgGscnGetResWaiting = FALSE;
	DBGLOG(SCN, INFO, "%s wait for Results are replyed timeout\r\n", __func__);

}
#endif

VOID scnScanDoneTimeout(IN P_ADAPTER_T prAdapter, ULONG ulParamPtr)
{

	P_SCAN_INFO_T prScanInfo;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	DBGLOG(SCN, WARN, "scnScanDoneTimeout %d \r\n", prScanInfo->ucScanDoneTimeoutCnt);

	prScanInfo->ucScanDoneTimeoutCnt++;
	/* whole chip reset check */
	if (prScanInfo->ucScanDoneTimeoutCnt > SCAN_DONE_TIMEOUT_THRESHOLD) {

		DBGLOG(SCN, ERROR,
		       " meet SCAN_DONE_TIMEOUT_THRESHOLD %d, trigger whole chip reset !! \r\n",
		       SCAN_DONE_TIMEOUT_THRESHOLD);
#if CFG_CHIP_RESET_SUPPORT
		glResetTrigger(prAdapter);
#endif
	}
}
