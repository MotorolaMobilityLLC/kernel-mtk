/*
* Copyright (C) 2016 MediaTek Inc.
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

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/aaa_fsm.c#3 $
*/

/*
 * ! \file   "aaa_fsm.c"
 *  \brief  This file defines the FSM for AAA MODULE.
 *
 *   This file defines the FSM for AAA MODULE.
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
#if 0
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will send Event to AIS/BOW/P2P
*
* @param[in] rJoinStatus        To indicate JOIN success or failure.
* @param[in] prStaRec           Pointer to the STA_RECORD_T
* @param[in] prSwRfb            Pointer to the SW_RFB_T

* @return none
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS aaaFsmSendEventJoinComplete(WLAN_STATUS rJoinStatus, P_STA_RECORD_T prStaRec, P_SW_RFB_T prSwRfb)
{
	P_MSG_SAA_JOIN_COMP_T prJoinCompMsg;

	ASSERT(prStaRec);

	prJoinCompMsg = cnmMemAlloc(RAM_TYPE_TCM, sizeof(MSG_SAA_JOIN_COMP_T));
	if (!prJoinCompMsg)
		return WLAN_STATUS_RESOURCES;

	if (IS_STA_IN_AIS(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_AIS_JOIN_COMPLETE;
	else if (IS_STA_IN_P2P(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_P2P_JOIN_COMPLETE;
	else if (IS_STA_IN_BOW(prStaRec))
		prJoinCompMsg->rMsgHdr.eMsgId = MID_SAA_BOW_JOIN_COMPLETE;
	else
		ASSERT(0);

	prJoinCompMsg->rJoinStatus = rJoinStatus;
	prJoinCompMsg->prStaRec = prStaRec;
	prJoinCompMsg->prSwRfb = prSwRfb;

	mboxSendMsg(MBOX_ID_0, (P_MSG_HDR_T) prJoinCompMsg, MSG_SEND_METHOD_BUF);

	return WLAN_STATUS_SUCCESS;

}				/* end of saaFsmSendEventJoinComplete() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Start Event to AAA FSM.
*
* @param[in] prMsgHdr   Message of Join Request for a particular STA.
*
* @return none
*/
/*----------------------------------------------------------------------------*/
VOID aaaFsmRunEventStart(IN P_MSG_HDR_T prMsgHdr)
{
	P_MSG_SAA_JOIN_REQ_T prJoinReqMsg;
	P_STA_RECORD_T prStaRec;
	P_AIS_BSS_INFO_T prAisBssInfo;

	ASSERT(prMsgHdr);

	prJoinReqMsg = (P_MSG_SAA_JOIN_REQ_T) prMsgHdr;
	prStaRec = prJoinReqMsg->prStaRec;

	ASSERT(prStaRec);

	DBGLOG(SAA, LOUD, "EVENT-START: Trigger SAA FSM\n");

	cnmMemFree(prMsgHdr);

	/* 4 <1> Validation of SAA Start Event */
	if (!IS_AP_STA(prStaRec->eStaType)) {

		DBGLOG(SAA, ERROR, "EVENT-START: STA Type - %d was not supported.\n", prStaRec->eStaType);

		/* Ignore the return value because don't care the prSwRfb */
		saaFsmSendEventJoinComplete(WLAN_STATUS_FAILURE, prStaRec, NULL);

		return;
	}
	/* 4 <2> The previous JOIN process is not completed ? */
	if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
		DBGLOG(SAA, ERROR, "EVENT-START: Reentry of SAA Module.\n");
		prStaRec->eAuthAssocState = AA_STATE_IDLE;
	}
	/* 4 <3> Reset Status Code and Time */
	/* Update Station Record - Status/Reason Code */
	prStaRec->u2StatusCode = STATUS_CODE_SUCCESSFUL;

	/* Update the record join time. */
	GET_CURRENT_SYSTIME(&prStaRec->rLastJoinTime);

	prStaRec->ucTxAuthAssocRetryCount = 0;

	if (prStaRec->prChallengeText) {
		cnmMemFree(prStaRec->prChallengeText);
		prStaRec->prChallengeText = (P_IE_CHALLENGE_TEXT_T) NULL;
	}

	cnmTimerStopTimer(&prStaRec->rTxReqDoneOrRxRespTimer);

	prStaRec->ucStaState = STA_STATE_1;

	/* Trigger SAA MODULE */
	saaFsmSteps(prStaRec, SAA_STATE_SEND_AUTH1, (P_SW_RFB_T) NULL);

}				/* end of saaFsmRunEventStart() */
#endif

#if CFG_SUPPORT_AAA
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will process the Rx Auth Request Frame and then
*        trigger AAA FSM.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
VOID aaaFsmRunEventRxAuth(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T) NULL;
	P_STA_RECORD_T prStaRec = (P_STA_RECORD_T) NULL;
	UINT_16 u2StatusCode;
	BOOLEAN fgReplyAuth = FALSE;
	P_WLAN_AUTH_FRAME_T prAuthFrame = (P_WLAN_AUTH_FRAME_T) NULL;

	ASSERT(prAdapter);

	do {
		prAuthFrame = (P_WLAN_AUTH_FRAME_T) prSwRfb->pvHeader;

		DBGLOG(AAA, INFO, "Recv Auth from [" MACSTR "]\n", MAC2STR(prAuthFrame->aucSrcAddr));

#if CFG_ENABLE_WIFI_DIRECT
		prBssInfo = p2pFuncBSSIDFindBssInfo(prAdapter, prAuthFrame->aucBSSID);

		/* 4 <1> Check P2P network conditions */
		if (prBssInfo && prAdapter->fgIsP2PRegistered) {

			if (prBssInfo->fgIsNetActive) {

				/* 4 <1.1> Validate Auth Frame by Auth Algorithm/Transation Seq */
				if (WLAN_STATUS_SUCCESS ==
				    authProcessRxAuth1Frame(prAdapter,
							    prSwRfb,
							    prBssInfo->aucBSSID,
							    AUTH_ALGORITHM_NUM_OPEN_SYSTEM,
							    AUTH_TRANSACTION_SEQ_1, &u2StatusCode)) {

					if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
						/* 4 <1.2> Validate Auth Frame for Network Specific Conditions */
						fgReplyAuth = p2pFuncValidateAuth(prAdapter,
										  prBssInfo,
										  prSwRfb, &prStaRec, &u2StatusCode);
					} else
						fgReplyAuth = TRUE;

					break;
				}
			}
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

		/* 4 <2> Check BOW network conditions */
#if CFG_ENABLE_BT_OVER_WIFI
		{
			P_BOW_FSM_INFO_T prBowFsmInfo = (P_BOW_FSM_INFO_T) NULL;

			prBowFsmInfo = &(prAdapter->rWifiVar.rBowFsmInfo);
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prBowFsmInfo->ucBssIndex);

			if ((prBssInfo->fgIsNetActive) && (prBssInfo->eCurrentOPMode == OP_MODE_BOW)) {

				/* 4 <2.1> Validate Auth Frame by Auth Algorithm/Transation Seq */
				/* Check if for this BSSID */
				if (WLAN_STATUS_SUCCESS ==
				    authProcessRxAuth1Frame(prAdapter,
							    prSwRfb,
							    prBssInfo->aucBSSID,
							    AUTH_ALGORITHM_NUM_OPEN_SYSTEM,
							    AUTH_TRANSACTION_SEQ_1, &u2StatusCode)) {

					if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

						/* 4 <2.2> Validate Auth Frame for Network Specific Conditions */
						fgReplyAuth =
						    bowValidateAuth(prAdapter, prSwRfb, &prStaRec, &u2StatusCode);

					} else
						fgReplyAuth = TRUE;

					/* TODO(Kevin): Allocate a STA_RECORD_T for new client */
					break;
				}
			}
		}
#endif /* CFG_ENABLE_BT_OVER_WIFI */

		return;
	} while (FALSE);

	if (prStaRec) {
		/* update RCPI */
		ASSERT(prSwRfb->prRxStatusGroup3);
		prStaRec->ucRCPI = (UINT_8) HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3);
	}
	/* 4 <3> Update STA_RECORD_T and reply Auth_2(Response to Auth_1) Frame */
	if (fgReplyAuth) {

		if (prStaRec) {

			if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
				if (prStaRec->eAuthAssocState != AA_STATE_IDLE) {
					DBGLOG(AAA, WARN,
					       "Previous AuthAssocState (%d) != IDLE.\n", prStaRec->eAuthAssocState);
				}

				prStaRec->eAuthAssocState = AAA_STATE_SEND_AUTH2;
			} else {
				prStaRec->eAuthAssocState = AA_STATE_IDLE;

				/* NOTE(Kevin): Change to STATE_1 */
				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);
			}

			/* Update the record join time. */
			GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

			/* Update Station Record - Status/Reason Code */
			prStaRec->u2StatusCode = u2StatusCode;

			prStaRec->ucAuthAlgNum = AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
		} else {
			/* NOTE(Kevin): We should have STA_RECORD_T if the status code was successful */
			ASSERT(!(u2StatusCode == STATUS_CODE_SUCCESSFUL));
		}

		/* NOTE: Ignore the return status for AAA */
		/* 4 <4> Reply Auth */
		authSendAuthFrame(prAdapter,
				  prStaRec, prBssInfo->ucBssIndex, prSwRfb, AUTH_TRANSACTION_SEQ_2, u2StatusCode);

	} else if (prStaRec)
		cnmStaRecFree(prAdapter, prStaRec);

}				/* end of aaaFsmRunEventRxAuth() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will process the Rx (Re)Association Request Frame and then
*        trigger AAA FSM.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to the SW_RFB_T structure.
*
* @retval WLAN_STATUS_SUCCESS           Always return success
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS aaaFsmRunEventRxAssoc(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb)
{
	P_BSS_INFO_T prBssInfo;
	P_STA_RECORD_T prStaRec = (P_STA_RECORD_T) NULL;
	UINT_16 u2StatusCode = STATUS_CODE_RESERVED;
	BOOLEAN fgReplyAssocResp = FALSE;

	ASSERT(prAdapter);

	do {

		/* 4 <1> Check if we have the STA_RECORD_T for incoming Assoc Req */
		prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
		if (!prStaRec) {
			nicRxMgmtNoWTBLHandling(prAdapter, prSwRfb);
			prStaRec = prSwRfb->prStaRec;
		}
		/* We should have the corresponding Sta Record. */
		if (!prStaRec || !prStaRec->fgIsInUse) {
			/* Not to reply association response with failure code due to lack of STA_REC */
			DBGLOG(AAA, WARN, "Recv Assoc Req w/o corresponding StaRec, wlanIdx[%d]\n",
			       prSwRfb->ucWlanIdx);
			break;
		}

		if (!IS_CLIENT_STA(prStaRec))
			break;

		DBGLOG(AAA, INFO, "Recv Assoc Req from [" MACSTR "], eAuthAssocState: %d\n",
		       MAC2STR(prStaRec->aucMacAddr), prStaRec->eAuthAssocState);

		if (prStaRec->ucStaState == STA_STATE_3) {
			/* Do Reassocation */
		} else if ((prStaRec->ucStaState == STA_STATE_2) &&
			(prStaRec->eAuthAssocState == AAA_STATE_SEND_AUTH2)) {
			/* Normal case */
		} else {
			DBGLOG(AAA, WARN, "Previous AuthAssocState (%d) != SEND_AUTH2 or StaState (%d) == STATE_1.\n",
			       prStaRec->eAuthAssocState, prStaRec->ucStaState);

			/* Maybe Auth Response TX fail, but actually it success. */
			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
		}

		/* update RCPI */
		ASSERT(prSwRfb->prRxStatusGroup3);
		prStaRec->ucRCPI = (UINT_8) HAL_RX_STATUS_GET_RCPI(prSwRfb->prRxStatusGroup3);

		/* 4 <2> Check P2P network conditions */
#if CFG_ENABLE_WIFI_DIRECT
		if ((prAdapter->fgIsP2PRegistered) && (IS_STA_IN_P2P(prStaRec))) {

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

			if (prBssInfo->fgIsNetActive) {

				/* 4 <2.1> Validate Assoc Req Frame and get Status Code */
				/* Check if for this BSSID */
				if (WLAN_STATUS_SUCCESS ==
				    assocProcessRxAssocReqFrame(prAdapter, prSwRfb, &u2StatusCode)) {

					if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {
						/* 4 <2.2> Validate Assoc Req  Frame for Network Specific Conditions */
						fgReplyAssocResp =
						    p2pFuncValidateAssocReq(prAdapter, prSwRfb,
									    (PUINT_16)&u2StatusCode);
					} else
						fgReplyAssocResp = TRUE;

					break;
				}
			}
		}
#endif /* CFG_ENABLE_WIFI_DIRECT */

		/* 4 <3> Check BOW network conditions */
#if CFG_ENABLE_BT_OVER_WIFI
		if (IS_STA_BOW_TYPE(prStaRec)) {

			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

			if ((prBssInfo->fgIsNetActive) && (prBssInfo->eCurrentOPMode == OP_MODE_BOW)) {

				/* 4 <3.1> Validate Auth Frame by Auth Algorithm/Transation Seq */
				/* Check if for this BSSID */
				if (assocProcessRxAssocReqFrame(prAdapter, prSwRfb, &u2StatusCode) ==
				    WLAN_STATUS_SUCCESS) {

					if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

						/* 4 <3.2> Validate Auth Frame for Network Specific Conditions */
						fgReplyAssocResp =
						    bowValidateAssocReq(prAdapter, prSwRfb, &u2StatusCode);

					} else
						fgReplyAssocResp = TRUE;

					/* TODO(Kevin): Allocate a STA_RECORD_T for new client */
					break;
				}
			}
		}
#endif /* CFG_ENABLE_BT_OVER_WIFI */

		return WLAN_STATUS_SUCCESS;	/* To release the SW_RFB_T */
	} while (FALSE);

	/* 4 <4> Update STA_RECORD_T and reply Assoc Resp Frame */
	if (fgReplyAssocResp) {
		UINT_16 u2IELength;
		PUINT_8 pucIE;

		if ((((P_WLAN_ASSOC_REQ_FRAME_T) (prSwRfb->pvHeader))->u2FrameCtrl & MASK_FRAME_TYPE) ==
		    MAC_FRAME_REASSOC_REQ) {

			u2IELength = prSwRfb->u2PacketLen -
			    (UINT_16) OFFSET_OF(WLAN_REASSOC_REQ_FRAME_T, aucInfoElem[0]);

			pucIE = ((P_WLAN_REASSOC_REQ_FRAME_T) (prSwRfb->pvHeader))->aucInfoElem;
		} else {
			u2IELength = prSwRfb->u2PacketLen - (UINT_16) OFFSET_OF(WLAN_ASSOC_REQ_FRAME_T, aucInfoElem[0]);

			pucIE = ((P_WLAN_ASSOC_REQ_FRAME_T) (prSwRfb->pvHeader))->aucInfoElem;
		}

		rlmProcessAssocReq(prAdapter, prSwRfb, pucIE, u2IELength);

		/* 4 <4.1> Assign Association ID */
		if (u2StatusCode == STATUS_CODE_SUCCESSFUL) {

#if CFG_ENABLE_WIFI_DIRECT
			if ((prAdapter->fgIsP2PRegistered) && (IS_STA_IN_P2P(prStaRec))) {
				prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
				if (p2pRoleFsmRunEventAAAComplete(prAdapter, prStaRec, prBssInfo) ==
				    WLAN_STATUS_SUCCESS) {
					/* prStaRec->u2AssocId = bssAssignAssocID(prStaRec); */
					/* NOTE(Kevin): for TX done */
					prStaRec->eAuthAssocState = AAA_STATE_SEND_ASSOC2;
					/* NOTE(Kevin): Method A: Change to STATE_3 before handle TX Done */
					/* cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3); */
				} else {
					/* Client List FULL. */
					u2StatusCode = STATUS_CODE_REQ_DECLINED;

					prStaRec->u2AssocId = 0;	/* Invalid Association ID */

					/* If(Re)association fail,remove sta record and use class error to handle sta */
					prStaRec->eAuthAssocState = AA_STATE_IDLE;

					/* NOTE(Kevin): Better to change state here, not at TX Done */
					cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
				}
			}
#endif

#if CFG_ENABLE_BT_OVER_WIFI
			if ((IS_STA_BOW_TYPE(prStaRec))) {
				/* if (bowRunEventAAAComplete(prAdapter, prStaRec) == WLAN_STATUS_SUCCESS) { */
				prStaRec->u2AssocId = bssAssignAssocID(prStaRec);
				prStaRec->eAuthAssocState = AAA_STATE_SEND_ASSOC2;	/* NOTE(Kevin): for TX done */

				/* NOTE(Kevin): Method A: Change to STATE_3 before handle TX Done */
				/* cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3); */
			}
#endif
		} else {
			prStaRec->u2AssocId = 0;	/* Invalid Association ID */

			/* If (Re)association fail, remove sta record and use class error to handle sta */
			prStaRec->eAuthAssocState = AA_STATE_IDLE;

			/* NOTE(Kevin): Better to change state here, not at TX Done */
			cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
		}

		/* Update the record join time. */
		GET_CURRENT_SYSTIME(&prStaRec->rUpdateTime);

		/* Update Station Record - Status/Reason Code */
		prStaRec->u2StatusCode = u2StatusCode;

		/* NOTE: Ignore the return status for AAA */
		/* 4 <4.2> Reply Assoc Resp */
		assocSendReAssocRespFrame(prAdapter, prStaRec);

	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aaaFsmRunEventRxAssoc() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle TxDone(Auth2/AssocReq) Event of AAA FSM.
*
* @param[in] prAdapter      Pointer to the Adapter structure.
* @param[in] prMsduInfo     Pointer to the MSDU_INFO_T.
* @param[in] rTxDoneStatus  Return TX status of the Auth1/Auth3/AssocReq frame.
*
* @retval WLAN_STATUS_SUCCESS
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS
aaaFsmRunEventTxDone(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN ENUM_TX_RESULT_CODE_T rTxDoneStatus)
{
	P_STA_RECORD_T prStaRec;
	P_BSS_INFO_T prBssInfo;

	ASSERT(prAdapter);
	ASSERT(prMsduInfo);

	prStaRec = cnmGetStaRecByIndex(prAdapter, prMsduInfo->ucStaRecIndex);

	if ((!prStaRec) || (!prStaRec->fgIsInUse))
		return WLAN_STATUS_SUCCESS;	/* For the case of replying ERROR STATUS CODE */

	ASSERT(prStaRec->ucBssIndex <= MAX_BSS_INDEX);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	/* Trigger statistics log if Auth/Assoc Tx failed */
	if (rTxDoneStatus != TX_RESULT_SUCCESS) {
		DBGLOG(AAA, INFO, "EVENT-TX DONE: Status[%d] SeqNo[%d] Current Time = %ld\n",
		       rTxDoneStatus, prMsduInfo->ucTxSeqNum, kalGetTimeTick());
		wlanTriggerStatsLog(prAdapter, prAdapter->rWifiVar.u4StatsLogDuration);
	}

	switch (prStaRec->eAuthAssocState) {
	case AAA_STATE_SEND_AUTH2:
		{
			/* Strictly check the outgoing frame is matched with current AA STATE */
			if (authCheckTxAuthFrame(prAdapter, prMsduInfo, AUTH_TRANSACTION_SEQ_2) != WLAN_STATUS_SUCCESS)
				break;

			if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
				if (rTxDoneStatus == TX_RESULT_SUCCESS) {

					/* NOTE(Kevin): Change to STATE_2 at TX Done */
					cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);
				} else {

					prStaRec->eAuthAssocState = AA_STATE_IDLE;

					/* NOTE(Kevin): Change to STATE_1 */
					cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

#if CFG_ENABLE_WIFI_DIRECT
					if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
						p2pRoleFsmRunEventAAATxFail(prAdapter, prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_ENABLE_BT_OVER_WIFI
					if (IS_STA_BOW_TYPE(prStaRec))
						bowRunEventAAATxFail(prAdapter, prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */
				}

			}
			/* NOTE(Kevin): Ignore the TX Done Event of Auth Frame with Error Status Code */

		}
		break;

	case AAA_STATE_SEND_ASSOC2:
		{
			/* Strictly check the outgoing frame is matched with current SAA STATE */
			if (assocCheckTxReAssocRespFrame(prAdapter, prMsduInfo) != WLAN_STATUS_SUCCESS)
				break;

			if (prStaRec->u2StatusCode == STATUS_CODE_SUCCESSFUL) {
				if (rTxDoneStatus == TX_RESULT_SUCCESS) {

					prStaRec->eAuthAssocState = AA_STATE_IDLE;

					/* NOTE(Kevin): Change to STATE_3 at TX Done */
#if CFG_ENABLE_WIFI_DIRECT
					if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
						p2pRoleFsmRunEventAAASuccess(prAdapter, prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI

					if (IS_STA_BOW_TYPE(prStaRec))
						bowRunEventAAAComplete(prAdapter, prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */

				} else {

					prStaRec->eAuthAssocState = AAA_STATE_SEND_AUTH2;

					/* NOTE(Kevin): Change to STATE_2 */
					cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_2);

#if CFG_ENABLE_WIFI_DIRECT
					if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
						p2pRoleFsmRunEventAAATxFail(prAdapter, prStaRec, prBssInfo);
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if CFG_ENABLE_BT_OVER_WIFI
					if (IS_STA_BOW_TYPE(prStaRec))
						bowRunEventAAATxFail(prAdapter, prStaRec);

#endif /* CFG_ENABLE_BT_OVER_WIFI */

				}
			}
			/* NOTE(Kevin): Ignore the TX Done Event of Auth Frame with Error Status Code */
		}
		break;

	case AA_STATE_IDLE:
		/* 2013-08-27 frog:  Do nothing.
		 * Somtimes we may send Assoc Resp twice. (Rx Assoc Req before the first Assoc TX Done)
		 * The AssocState is changed to IDLE after first TX done.
		 * Free station record when IDLE is seriously wrong.
		 */
		/* /cnmStaRecFree(prAdapter, prStaRec); */

	default:
		break;		/* Ignore other cases */
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aaaFsmRunEventTxDone() */
#endif /* CFG_SUPPORT_AAA */

