/*
** Id: mgmt/privacy.c#1
*/

/*! \file   "privacy.c"
*    \brief  This file including the protocol layer privacy function.
*
*    This file provided the macros and functions library support for the
*    protocol layer security setting from rsn.c and nic_privacy.c
*
*/

/*
** Log: privacy.c
**
** 07 16 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch]
** .
**
** 07 16 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch]
** fix ap assert after 32 times connect-disconnct
**
** 08 19 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** .Adjust some debug message, refine the wlan table assign for bss
**
** 07 30 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Add a function, for rx, input the rx wlan index to get the bss index
**
** 07 26 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** 1. Reduce extra Tx frame header parsing
** 2. Add TX port control
** 3. Add net interface to BSS binding
**
** 07 25 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** fix at 11w config, mgmt and data privacy setting issue
**
** 07 23 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** 1. build success for win32 port
** 2. add SDIO test read/write pattern for HQA tests (default off)
**
** 07 23 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Sync the latest jb2.mp 11w code as draft version
** Not the CM bit for avoid wapi 1x drop at re-key
**
** 07 19 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** wapi 1x frame don't need encrypt
**
** 07 19 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** fix wrong pointer usage.
**
** 07 18 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** At nicTxComposeDesc (Mgmt and Data) function, use security setting
** to decide frame protect or not.
**
** 07 17 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** fix and modify some security code
**
** 07 15 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** .
**
** 07 05 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Fix to let the wpa-psk ok
**
** 07 04 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Add a <<temp function>> to decide data frame need encrypted or not
**
** 07 04 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Add the function to got the STA index via the wlan index
** report at Rx status
**
** 07 03 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Refine some normal security code
**
** 07 02 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Refine some secutity code
**
** 07 02 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Refine security BMC wlan index assign
** Fix some compiling warning
**
** 07 01 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Add some debug code, fixed some compiling warning
**
** 06 25 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** Update for 1st connection
**
** 06 18 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** Update for 1st connection
**
** 03 29 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** 1. remove unused HIF definitions
** 2. enable NDIS 5.1 build success
**
** 03 29 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Do more sta record free mechanism check
** remove non-used code
**
** 03 27 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** add default ket handler
**
** 03 20 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Add the security code for wlan table assign operation
**
** 03 18 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** fixed compiling error
**
** 03 15 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** Modify some security part code
**
** 03 14 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** .modify some code define and flow
**
** 03 13 2013 wh.su
** [BORA00002446] [MT6630] [Wi-Fi] [Driver] Update the security function code
** .remove non-used code
**
** 03 12 2013 tsaiyuan.hsu
** [BORA00002222] MT6630 unified MAC RXM
** add rx data and management processing.
**
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

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to initialize the privacy-related
*        parameters.
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucNetTypeIdx  Pointer to netowrk type index
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID secInit(IN P_ADAPTER_T prAdapter, IN UINT_8 ucBssIndex)
{
	UINT_8 i;
	P_CONNECTION_SETTINGS_T prConnSettings;
	P_BSS_INFO_T prBssInfo;
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

	DEBUGFUNC("secInit");

	ASSERT(prAdapter);

	prConnSettings = &prAdapter->rWifiVar.rConnSettings;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;

	prBssInfo->u4RsnSelectedGroupCipher = 0;
	prBssInfo->u4RsnSelectedPairwiseCipher = 0;
	prBssInfo->u4RsnSelectedAKMSuite = 0;

#if 0				/* CFG_ENABLE_WIFI_DIRECT */
	prBssInfo = &prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_P2P];

	prBssInfo->u4RsnSelectedGroupCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_PSK;
#endif

#if 0				/* CFG_ENABLE_BT_OVER_WIFI */
	prBssInfo = &prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_BOW];

	prBssInfo->u4RsnSelectedGroupCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prBssInfo->u4RsnSelectedAKMSuite = RSN_AKM_SUITE_PSK;
#endif

	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[0].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP40;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[1].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_TKIP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[2].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_CCMP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[3].dot11RSNAConfigPairwiseCipher = WPA_CIPHER_SUITE_WEP104;

	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[4].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP40;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[5].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_TKIP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[6].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_CCMP;
	prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[7].dot11RSNAConfigPairwiseCipher = RSN_CIPHER_SUITE_WEP104;

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prAdapter->rMib.dot11RSNAConfigPairwiseCiphersTable[i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[0].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_NONE;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[1].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_802_1X;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[2].dot11RSNAConfigAuthenticationSuite =
	    WPA_AKM_SUITE_PSK;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[3].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_NONE;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[4].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_802_1X;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[5].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_PSK;

#if CFG_SUPPORT_802_11W
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[6].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_802_1X_SHA256;
	prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[7].dot11RSNAConfigAuthenticationSuite =
	    RSN_AKM_SUITE_PSK_SHA256;
#endif

	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++) {
		prAdapter->rMib.dot11RSNAConfigAuthenticationSuitesTable[i].dot11RSNAConfigAuthenticationSuiteEnabled =
		    FALSE;
	}

	secClearPmkid(prAdapter);

	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rPreauthenticationTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) rsnIndicatePmkidCand, (ULONG) NULL);

#if CFG_SUPPORT_802_11W
	cnmTimerInitTimer(prAdapter,
			  &prAisSpecBssInfo->rSaQueryTimer, (PFN_MGMT_TIMEOUT_FUNC) rsnStartSaQueryTimer, (ULONG) NULL);
#endif

	prAisSpecBssInfo->fgCounterMeasure = FALSE;
	prAdapter->prAisBssInfo->ucBcDefaultKeyIdx = 0xff;
	prAdapter->prAisBssInfo->fgBcDefaultKeyExist = FALSE;

#if 0
	for (i = 0; i < WTBL_SIZE; i++) {
		g_prWifiVar->arWtbl[i].ucUsed = FALSE;
		g_prWifiVar->arWtbl[i].prSta = NULL;
		g_prWifiVar->arWtbl[i].ucNetTypeIdx = NETWORK_TYPE_INDEX_NUM;

	}
	nicPrivacyInitialize((UINT_8) NETWORK_TYPE_INDEX_NUM);
#endif

}				/* secInit */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will indicate an Event of "Rx Class Error" to SEC_FSM for
*        JOIN Module.
*
* \param[in] prAdapter     Pointer to the Adapter structure
* \param[in] prSwRfb       Pointer to the SW RFB.
*
* \return FALSE                Class Error
*/
/*----------------------------------------------------------------------------*/
BOOL secCheckClassError(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSwRfb, IN P_STA_RECORD_T prStaRec)
{
	P_HW_MAC_RX_DESC_T prRxStatus;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);

	prRxStatus = prSwRfb->prRxStatus;

#if 1
	if (!prStaRec || (prRxStatus->u2StatusFlag & RXS_DW2_RX_CLASSERR_BITMAP) == RXS_DW2_RX_CLASSERR_VALUE) {

		DBGLOG(RSN, TRACE,
		       "prStaRec=%x RX Status = %x RX_CLASSERR check!\n", prStaRec, prRxStatus->u2StatusFlag);

		/* if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) { */
		authSendDeauthFrame(prAdapter,
				    NULL, NULL, prSwRfb, REASON_CODE_CLASS_3_ERR, (PFN_TX_DONE_HANDLER) NULL);
		return FALSE;
		/* } */
	}
#else
	if ((prStaRec) && 1 /* RXM_IS_DATA_FRAME(prSwRfb) */) {
		UINT_8 ucBssIndex = prStaRec->ucBssIndex;

		if (IS_NET_ACTIVE(prAdapter, ucBssIndex)) {
			/* P_BSS_INFO_T prBssInfo; */
			/* prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex); */

			if ((prRxStatus->u2StatusFlag & RXS_DW2_RX_CLASSERR_BITMAP) == RXS_DW2_RX_CLASSERR_VALUE) {
				/*
				*if ((prStaRec->ucStaState != STA_STATE_3) &&
				*IS_BSS_ACTIVE(prBssInfo) &&
				*prBssInfo->fgIsNetAbsent == FALSE) {
				* (IS_AP_STA(prStaRec) || IS_CLIENT_STA(prStaRec))) {
				*/
				if (authSendDeauthFrame(prAdapter,
						       prStaRec,
						       NULL,
						       REASON_CODE_CLASS_3_ERR,
						       (PFN_TX_DONE_HANDLER)
						       NULL) == WLAN_STATUS_SUCCESS) {
					DBGLOG(RSN, TRACE,
					       ("Send Deauth to MAC:[" MACSTR
						"] for Rx Class 3 Error.\n", MAC2STR(prStaRec->aucMacAddr)));
				}

				return FALSE;
			}

			return secRxPortControlCheck(prAdapter, prSwRfb);
		}
	}
#endif
	return TRUE;

}				/* end of secCheckClassError() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to setting the sta port status.
*
* \param[in]  prAdapter Pointer to the Adapter structure
* \param[in]  prSta Pointer to the sta
* \param[in]  fgPortBlock The port status
*
* \retval none
*
*/
/*----------------------------------------------------------------------------*/
VOID secSetPortBlocked(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, IN BOOLEAN fgPortBlock)
{
#if 0				/* Marked for MT6630 */
	if (prSta == NULL)
		return;

	prSta->fgPortBlock = fgPortBlock;

	DBGLOG(RSN, TRACE,
	       "The STA " MACSTR " port %s\n", MAC2STR(prSta->aucMacAddr), fgPortBlock == TRUE ? "BLOCK" : " OPEN");
#endif
}

#if 0				/* Marked for MT6630 */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to report the sta port status.
*
* \param[in]  prAdapter Pointer to the Adapter structure
* \param[in]  prSta Pointer to the sta
* \param[out]  fgPortBlock The port status
*
* \return TRUE sta exist, FALSE sta not exist
*
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secGetPortStatus(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta, OUT PBOOLEAN pfgPortStatus)
{
	if (prSta == NULL)
		return FALSE;

	*pfgPortStatus = prSta->fgPortBlock;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to handle Peer device Tx Security process MSDU.
*
* \param[in] prMsduInfo pointer to the packet info pointer
*
* \retval TRUE Accept the packet
* \retval FALSE Refuse the MSDU packet due port blocked
*
*/
/*----------------------------------------------------------------------------*/
BOOL				/* ENUM_PORT_CONTROL_RESULT */
secTxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN P_STA_RECORD_T prStaRec)
{
	ASSERT(prAdapter);
	ASSERT(prMsduInfo);
	ASSERT(prStaRec);

	if (prStaRec) {

		/* Todo:: */
		if (prMsduInfo->fgIs802_1x)
			return TRUE;

		if (prStaRec->fgPortBlock == TRUE) {
			DBGLOG(INIT, TRACE, "Drop Tx packet due Port Control!\n");
			return FALSE;
		}
#if CFG_SUPPORT_WAPI
		if (prAdapter->rWifiVar.rConnSettings.fgWapiMode)
			return TRUE;
#endif
		if (IS_STA_IN_AIS(prStaRec)) {
			if (!prAdapter->rWifiVar.rAisSpecificBssInfo.fgTransmitKeyExist &&
			    (prAdapter->rWifiVar.rConnSettings.eEncStatus == ENUM_ENCRYPTION1_ENABLED)) {
				DBGLOG(INIT, TRACE, "Drop Tx packet due the key is removed!!!\n");
				return FALSE;
			}
		}
	}

	return TRUE;
}
#endif /* Marked for MT6630 */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to handle The Rx Security process MSDU.
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] prSWRfb SW rfb pinter
*
* \retval TRUE Accept the packet
* \retval FALSE Refuse the MSDU packet due port control
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secRxPortControlCheck(IN P_ADAPTER_T prAdapter, IN P_SW_RFB_T prSWRfb)
{
	ASSERT(prSWRfb);

#if 0
	/* whsu:Todo: Process MGMT and DATA */
	if (prSWRfb->prStaRec) {
		if (prSWRfb->prStaRec->fgPortBlock == TRUE) {
			if (1 /* prSWRfb->fgIsDataFrame and not 1x */  &&
			    (g_prWifiVar->rConnSettings.eAuthMode >= AUTH_MODE_WPA)) {
				/* DBGLOG(SEC, WARN, ("Drop Rx data due port control !\r\n")); */
				return TRUE;	/* Todo: whsu FALSE; */
			}
			/* if (!RX_STATUS_IS_PROTECT(prSWRfb->prRxStatus)) { */
			/* DBGLOG(RSN, WARN, ("Drop rcv non-encrypted data frame!\n")); */
			/* return FALSE; */
			/* } */
		}
	} else {
	}
#endif
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine will enable/disable the cipher suite
*
* \param[in] prAdapter Pointer to the adapter object data area.
* \param[in] u4CipherSuitesFlags flag for cipher suite
*
* \retval none
*/
/*----------------------------------------------------------------------------*/
VOID secSetCipherSuite(IN P_ADAPTER_T prAdapter, IN UINT_32 u4CipherSuitesFlags)
{

	UINT_32 i;
	P_DOT11_RSNA_CONFIG_PAIRWISE_CIPHERS_ENTRY prEntry;
	P_IEEE_802_11_MIB_T prMib;

	ASSERT(prAdapter);

	prMib = &prAdapter->rMib;

	ASSERT(prMib);

	if (u4CipherSuitesFlags == CIPHER_FLAG_NONE) {
		/* Disable all the pairwise cipher suites. */
		for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
			prMib->dot11RSNAConfigPairwiseCiphersTable[i].dot11RSNAConfigPairwiseCipherEnabled = FALSE;

		/* Update the group cipher suite. */
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

		return;
	}

	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++) {
		prEntry = &prMib->dot11RSNAConfigPairwiseCiphersTable[i];

		switch (prEntry->dot11RSNAConfigPairwiseCipher) {
		case WPA_CIPHER_SUITE_WEP40:
		case RSN_CIPHER_SUITE_WEP40:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP40)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_TKIP:
		case RSN_CIPHER_SUITE_TKIP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_TKIP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_CCMP:
		case RSN_CIPHER_SUITE_CCMP:
			if (u4CipherSuitesFlags & CIPHER_FLAG_CCMP)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;

		case WPA_CIPHER_SUITE_WEP104:
		case RSN_CIPHER_SUITE_WEP104:
			if (u4CipherSuitesFlags & CIPHER_FLAG_WEP104)
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = TRUE;
			else
				prEntry->dot11RSNAConfigPairwiseCipherEnabled = FALSE;
			break;
		default:
			break;
		}
	}

	/* Update the group cipher suite. */
	if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_CCMP, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_CCMP;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_TKIP, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_TKIP;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP104, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP104;
	else if (rsnSearchSupportedCipher(prAdapter, WPA_CIPHER_SUITE_WEP40, &i))
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_WEP40;
	else
		prMib->dot11RSNAConfigGroupCipher = WPA_CIPHER_SUITE_NONE;

}				/* secSetCipherSuite */

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to initialize the pmkid parameters.
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \retval NONE
*/
/*----------------------------------------------------------------------------*/
VOID secClearPmkid(IN P_ADAPTER_T prAdapter)
{
	P_AIS_SPECIFIC_BSS_INFO_T prAisSpecBssInfo;

	DEBUGFUNC("secClearPmkid");

	prAisSpecBssInfo = &prAdapter->rWifiVar.rAisSpecificBssInfo;
	DBGLOG(RSN, TRACE, "secClearPmkid\n");
	prAisSpecBssInfo->u4PmkidCandicateCount = 0;
	prAisSpecBssInfo->u4PmkidCacheCount = 0;
	kalMemZero((PVOID) prAisSpecBssInfo->arPmkidCandicate, sizeof(PMKID_CANDICATE_T) * CFG_MAX_PMKID_CACHE);
	kalMemZero((PVOID) prAisSpecBssInfo->arPmkidCache, sizeof(PMKID_ENTRY_T) * CFG_MAX_PMKID_CACHE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Whether 802.11 privacy is enabled.
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \retval BOOLEAN
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secEnabledInAis(IN P_ADAPTER_T prAdapter)
{
	DEBUGFUNC("secEnabledInAis");

	ASSERT(prAdapter->rWifiVar.rConnSettings.eEncStatus < ENUM_ENCRYPTION3_KEY_ABSENT);

	switch (prAdapter->rWifiVar.rConnSettings.eEncStatus) {
	case ENUM_ENCRYPTION_DISABLED:
		return FALSE;
	case ENUM_ENCRYPTION1_ENABLED:
	case ENUM_ENCRYPTION2_ENABLED:
	case ENUM_ENCRYPTION3_ENABLED:
		return TRUE;
	default:
		DBGLOG(RSN, TRACE, "Unknown encryption setting %d\n", prAdapter->rWifiVar.rConnSettings.eEncStatus);
		break;
	}
	return FALSE;

}				/* secEnabledInAis */

BOOLEAN secIsProtected1xFrame(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	P_BSS_INFO_T prBssInfo;

	ASSERT(prAdapter);

	if (prStaRec) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
		if (prBssInfo && prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
			if (wlanQueryWapiMode(prAdapter))
				return FALSE;
#endif
		}

		return prStaRec->fgTransmitKeyExist;
	}
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is called to set the privacy bit at mac header for TxM
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] prMsdu the msdu for known the sta record
*
* \return TRUE the privacy need to set
*            FALSE the privacy no need to set
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secIsProtectedFrame(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsdu, IN P_STA_RECORD_T prStaRec)
{
	/* P_BSS_INFO_T prBssInfo; */

	ASSERT(prAdapter);
	ASSERT(prMsdu);
	/* ASSERT(prStaRec); */

#if CFG_SUPPORT_802_11W
	if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT) {
		BOOL fgRobustActionWithProtect = FALSE;
#if 0				/* Decide by Compose module */
		P_BSS_INFO_T prBssInfo;

		if (prStaRec) {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
			ASSERT(prBssInfo);
			if ((prBssInfo->eNetworkType == NETWORK_TYPE_AIS) &&
			    prAdapter->rWifiVar.rAisSpecificBssInfo.fgMgmtProtection /* Use MFP */) {

				fgRobustActionWithProtect = TRUE;
			}
		}
#endif
		if (prStaRec && fgRobustActionWithProtect /* AIS & Robust action frame */)
			return TRUE;
		else
			return FALSE;
	}
#else
	if (prMsdu->ucPacketType == TX_PACKET_TYPE_MGMT)
		return FALSE;
#endif

	return secIsProtectedBss(prAdapter, GET_BSS_INFO_BY_INDEX(prAdapter, prMsdu->ucBssIndex));
}

BOOLEAN secIsProtectedBss(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
	ASSERT(prBssInfo);

	if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS) {
#if CFG_SUPPORT_WAPI
		if (wlanQueryWapiMode(prAdapter))
			return TRUE;
#endif
		return secEnabledInAis(prAdapter);
	}
#if CFG_ENABLE_WIFI_DIRECT
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
		return kalP2PGetCipher(prAdapter->prGlueInfo, (UINT_8) prBssInfo->u4PrivateData);
#endif
	else if (prBssInfo->eNetworkType == NETWORK_TYPE_BOW)
		return TRUE;

	ASSERT(FALSE);
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used before add/update a WLAN entry.
*        Info the WLAN Table has available entry for this request
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in]  prSta the P_STA_RECORD_T for store
*
* \return TRUE Free Wlan table is reserved for this request
*            FALSE No free entry for this request
*
* \note
*/
/*----------------------------------------------------------------------------*/
BOOL secPrivacySeekForEntry(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prSta)
{
	P_BSS_INFO_T prP2pBssInfo;
	UINT_8 ucEntry = WTBL_RESERVED_ENTRY;
	UINT_8 i;
	UINT_8 ucStartIDX = 0, ucMaxIDX = 0;
	P_WLAN_TABLE_T prWtbl;
	UINT_8 ucRoleIdx = 0;

	ASSERT(prSta);

	if (!prSta->fgIsInUse)
		ASSERT(FALSE);

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
	ucRoleIdx = prP2pBssInfo->u4PrivateData;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	ucStartIDX = 0;
	ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

	DBGLOG(RSN, INFO, "secPrivacySeekForEntry\n");

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {
		if (prWtbl[i].ucUsed && EQUAL_MAC_ADDR(prSta->aucMacAddr, prWtbl[i].aucMacAddr)
		    && prWtbl[i].ucPairwise /* This function for ucPairwise only */) {
			ucEntry = i;
			DBGLOG(RSN, INFO, "[Wlan index]: Reuse entry #%d\n", i);
			break;
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
			if (prWtbl[i].ucUsed == FALSE) {
				ucEntry = i;
				DBGLOG(RSN, INFO, "[Wlan index]: Assign entry #%d\n", i);
				break;
			}
		}
	}

	/* Save to the driver maintain table */
	if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {

		prWtbl[ucEntry].ucUsed = TRUE;
		prWtbl[ucEntry].ucBssIndex = prSta->ucBssIndex;
		prWtbl[ucEntry].ucKeyId = 0xFF;
		prWtbl[ucEntry].ucPairwise = 1;
		COPY_MAC_ADDR(prWtbl[ucEntry].aucMacAddr, prSta->aucMacAddr);
		prWtbl[ucEntry].ucStaIndex = prSta->ucIndex;

		prSta->ucWlanIndex = ucEntry;

		{
			P_BSS_INFO_T prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prSta->ucBssIndex);
			/* for AP mode , if wep key exist, peer sta should also fgTransmitKeyExist */
			if (IS_BSS_P2P(prBssInfo) && kalP2PGetRole(prAdapter->prGlueInfo, ucRoleIdx) == 2) {
				if (prBssInfo->fgBcDefaultKeyExist &&
				    !(kalP2PGetCcmpCipher(prAdapter->prGlueInfo, ucRoleIdx) ||
				      kalP2PGetTkipCipher(prAdapter->prGlueInfo, ucRoleIdx))) {
					prSta->fgTransmitKeyExist = TRUE;
					prWtbl[ucEntry].ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
					DBGLOG(RSN, INFO, "peer sta set fgTransmitKeyExist\n");
				}
			}
		}

		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d " MACSTR
		       " staType=%x\n", prSta->ucBssIndex, 0, prWtbl[ucEntry].ucPairwise, ucEntry,
		       prSta->ucIndex, MAC2STR(prSta->aucMacAddr), prSta->eStaType);
#if 1				/* DBG */
		secCheckWTBLAssign(prAdapter);
#endif
		return TRUE;
	}
#if DBG
	secCheckWTBLAssign(prAdapter);
#endif
	DBGLOG(RSN, WARN, "[Wlan index] No more wlan table entry available!!!!\n");
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used free a WLAN entry.
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in]  ucEntry the wlan table index to free
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID secPrivacyFreeForEntry(IN P_ADAPTER_T prAdapter, IN UINT_8 ucEntry)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucEntry > WTBL_SIZE)
		return;

	DBGLOG(RSN, INFO, "secPrivacyFreeForEntry %d", ucEntry);

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucEntry].ucUsed) {
		prWtbl[ucEntry].ucUsed = FALSE;
		prWtbl[ucEntry].ucKeyId = 0xff;
		prWtbl[ucEntry].ucBssIndex = MAX_BSS_INDEX + 1;
		prWtbl[ucEntry].ucPairwise = 0;
		kalMemZero(prWtbl[ucEntry].aucMacAddr, MAC_ADDR_LEN);
		prWtbl[ucEntry].ucStaIndex = STA_REC_INDEX_NOT_FOUND;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used free a STA WLAN entry.
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in]  prStaRec the sta which want to free
*
* \return none
*/
/*----------------------------------------------------------------------------*/
VOID secPrivacyFreeSta(IN P_ADAPTER_T prAdapter, IN P_STA_RECORD_T prStaRec)
{
	UINT_32 entry;
	P_WLAN_TABLE_T prWtbl;

	if (!prStaRec)
		return;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (entry = 0; entry < WTBL_SIZE; entry++) {
		/* Consider GTK case !! */
		if (prWtbl[entry].ucUsed &&
		    EQUAL_MAC_ADDR(prStaRec->aucMacAddr, prWtbl[entry].aucMacAddr) && prWtbl[entry].ucPairwise) {
#if 1				/* DBG */
			DBGLOG(RSN, INFO, "Free STA entry (%lu)!\n", entry);
#endif
			secPrivacyFreeForEntry(prAdapter, entry);
			prStaRec->ucWlanIndex = WTBL_RESERVED_ENTRY;
			/* prStaRec->ucBMCWlanIndex = WTBL_RESERVED_ENTRY; */
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used for remove the BC entry of the BSS
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucBssIndex The BSS index
*
* \note
*/
/*----------------------------------------------------------------------------*/
VOID secRemoveBssBcEntry(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo, IN BOOL fgRoam)
{
	int i;
	P_CONNECTION_SETTINGS_T prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	if (!prBssInfo)
		return;

	DBGLOG(RSN, INFO, "remove all the key related with BSS!");

	if (fgRoam) {
		if (IS_BSS_AIS(prBssInfo) &&
		    prBssInfo->prStaRecOfAP && (prConnSettings->eAuthMode >= AUTH_MODE_WPA &&
						prConnSettings->eAuthMode != AUTH_MODE_WPA_NONE)) {

			for (i = 0; i < MAX_KEY_NUM; i++) {
				if (prBssInfo->ucBMCWlanIndexSUsed[i])
					secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndexS[i]);
				prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
				prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
			}
			prBssInfo->fgBcDefaultKeyExist = FALSE;
			prBssInfo->ucBcDefaultKeyIdx = 0xff;
		}
	} else {
		prBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;
		secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndex);

		for (i = 0; i < MAX_KEY_NUM; i++) {
			if (prBssInfo->ucBMCWlanIndexSUsed[i])
				secPrivacyFreeForEntry(prAdapter, prBssInfo->ucBMCWlanIndexS[i]);
			prBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
			prBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
		}
		for (i = 0; i < MAX_KEY_NUM; i++) {
			if (prBssInfo->wepkeyUsed[i])
				secPrivacyFreeForEntry(prAdapter, prBssInfo->wepkeyWlanIdx);
			prBssInfo->wepkeyUsed[i] = FALSE;
		}
		prBssInfo->wepkeyWlanIdx = WTBL_RESERVED_ENTRY;
		prBssInfo->fgBcDefaultKeyExist = FALSE;
		prBssInfo->ucBcDefaultKeyIdx = 0xff;
	}

}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used for adding the broadcast key used, to assign a wlan table entry
*         for reserved the specific entry for these key for
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucBssIndex The BSS index
* \param[in] ucNetTypeIdx The Network index
* \param[in] ucAlg the entry assign related with algorithm
* \param[in] ucKeyId The key id
* \param[in] ucTxRx The Type of the key
*
* \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation fail
*
* \note
*/
/*----------------------------------------------------------------------------*/
UINT_8
secPrivacySeekForBcEntry(IN P_ADAPTER_T prAdapter,
			 IN UINT_8 ucBssIndex,
			 IN PUINT_8 pucAddr, IN UINT_8 ucStaIdx, IN UINT_8 ucAlg, IN UINT_8 ucKeyId)
{
	UINT_8 ucEntry = WTBL_ALLOC_FAIL;
	UINT_8 ucStartIDX = 0, ucMaxIDX = 0;
	UINT_8 i;
	BOOLEAN fgCheckKeyId = TRUE;
	P_WLAN_TABLE_T prWtbl;
	/* P_BSS_INFO_T            prBSSInfo = GET_BSS_INFO_BY_INDEX(prAdapter,ucBssIndex); */

	prWtbl = prAdapter->rWifiVar.arWtbl;
	ASSERT(prAdapter);
	ASSERT(pucAddr);

	if (ucAlg == CIPHER_SUITE_WPI ||	/* CIPHER_SUITE_GCM_WPI || */
	    ucAlg == CIPHER_SUITE_WEP40 ||
	    ucAlg == CIPHER_SUITE_WEP104 || ucAlg == CIPHER_SUITE_WEP128 || ucAlg == CIPHER_SUITE_NONE)
		fgCheckKeyId = FALSE;

	if (ucKeyId == 0xFF || ucAlg == CIPHER_SUITE_BIP)
		fgCheckKeyId = FALSE;

	ucStartIDX = 0;
	ucMaxIDX = NIC_TX_DEFAULT_WLAN_INDEX - 1;

	DBGLOG(INIT, INFO, "secPrivacySeekForBcEntry\n");

	for (i = ucStartIDX; i <= ucMaxIDX; i++) {

		if (prWtbl[i].ucUsed && !prWtbl[i].ucPairwise && prWtbl[i].ucBssIndex == ucBssIndex) {

			if (!fgCheckKeyId) {
				ucEntry = i;
				DBGLOG(RSN, INFO, "[Wlan index]: Reuse entry #%d for open/wep/wpi\n", i);
				break;
			}

			if (fgCheckKeyId && (prWtbl[i].ucKeyId == ucKeyId || prWtbl[i].ucKeyId == 0xFF)) {
				ucEntry = i;
				DBGLOG(RSN, INFO, "[Wlan index]: Reuse entry #%d\n", i);
				break;
			}
		}
	}

	if (i == (ucMaxIDX + 1)) {
		for (i = ucStartIDX; i <= ucMaxIDX; i++) {
			if (prWtbl[i].ucUsed == FALSE) {
				ucEntry = i;
				DBGLOG(RSN, INFO, "[Wlan index]: Assign entry #%d\n", i);
				break;
			}
		}
	}

	if (ucEntry < NIC_TX_DEFAULT_WLAN_INDEX) {
		if (ucAlg != CIPHER_SUITE_BIP) {
			prWtbl[ucEntry].ucUsed = TRUE;
			prWtbl[ucEntry].ucKeyId = ucKeyId;
			prWtbl[ucEntry].ucBssIndex = ucBssIndex;
			prWtbl[ucEntry].ucPairwise = 0;
			kalMemCopy(prWtbl[ucEntry].aucMacAddr, pucAddr, MAC_ADDR_LEN);
			prWtbl[ucEntry].ucStaIndex = ucStaIdx;
		}
		DBGLOG(RSN, INFO,
		       "[Wlan index] BSS#%d keyid#%d P=%d use WlanIndex#%d STAIdx=%d " MACSTR
		       "\n", ucBssIndex, ucKeyId, prWtbl[ucEntry].ucPairwise, ucEntry, ucStaIdx, MAC2STR(pucAddr));

#if 1				/* DBG */
		secCheckWTBLAssign(prAdapter);
#endif
	} else {
		secCheckWTBLAssign(prAdapter);
		DBGLOG(RSN, INFO, "[Wlan index] No more wlan entry available!!!!\n");
	}

	return ucEntry;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \return ucEntryIndex The entry to be used, WTBL_ALLOC_FAIL for allocation fail
*
* \note
*/
/*----------------------------------------------------------------------------*/
BOOLEAN secCheckWTBLAssign(IN P_ADAPTER_T prAdapter)
{

	BOOLEAN fgCheckFail = FALSE;

	secPrivacyDumpWTBL(prAdapter);

	/* AIS STA should just has max 2 entry */
	/* Max STA check */
	if (fgCheckFail)
		ASSERT(FALSE);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief  Got the STA record index by wlan index
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucWlanIdx The Rx wlan index
*
* \return The STA record index, 0xff for invalid sta index
*/
/*----------------------------------------------------------------------------*/
UINT_8 secGetStaIdxByWlanIdx(P_ADAPTER_T prAdapter, UINT_8 ucWlanIdx)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return STA_REC_INDEX_NOT_FOUND;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	/* DBGLOG(RSN, TRACE, ("secGetStaIdxByWlanIdx=%d "MACSTR" used=%d\n", ucWlanIdx,
	*			MAC2STR(prWtbl[ucWlanIdx].aucMacAddr), prWtbl[ucWlanIdx].ucUsed));
	*/

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucStaIndex;
	else
		return STA_REC_INDEX_NOT_FOUND;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief  At Sw wlan table, got the BSS index by wlan index
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] ucWlanIdx The Rx wlan index
*
* \return The BSS index, 0xff for invalid bss index
*/
/*----------------------------------------------------------------------------*/
UINT_8 secGetBssIdxByWlanIdx(P_ADAPTER_T prAdapter, UINT_8 ucWlanIdx)
{
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);

	if (ucWlanIdx >= WTBL_SIZE)
		return WTBL_RESERVED_ENTRY;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (prWtbl[ucWlanIdx].ucUsed)
		return prWtbl[ucWlanIdx].ucBssIndex;
	else
		return WTBL_RESERVED_ENTRY;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief  Got the STA record index by mac addr
*
* \param[in] prAdapter Pointer to the Adapter structure
* \param[in] pucMacAddress MAC Addr
*
* \return The STA record index, 0xff for invalid sta index
*/
/*----------------------------------------------------------------------------*/
UINT_8 secLookupStaRecIndexFromTA(P_ADAPTER_T prAdapter, PUINT_8 pucMacAddress)
{
	int i;
	P_WLAN_TABLE_T prWtbl;

	ASSERT(prAdapter);
	prWtbl = prAdapter->rWifiVar.arWtbl;

	for (i = 0; i < WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed) {
			if (EQUAL_MAC_ADDR(pucMacAddress, prWtbl[i].aucMacAddr) && prWtbl[i].ucPairwise)
				return prWtbl[i].ucStaIndex;
		}
	}

	return STA_REC_INDEX_NOT_FOUND;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \note
*/
/*----------------------------------------------------------------------------*/
void secPrivacyDumpWTBL(IN P_ADAPTER_T prAdapter)
{
	P_WLAN_TABLE_T prWtbl;
	UINT_8 i;

	prWtbl = prAdapter->rWifiVar.arWtbl;

	DBGLOG(RSN, INFO, "The Wlan index\n");

	for (i = 0; i <= WTBL_SIZE; i++) {
		if (prWtbl[i].ucUsed)
			DBGLOG(RSN, INFO,
				"#%d Used=%d  BSSIdx=%d keyid=%d P=%d STA=%d Addr=" MACSTR "\n", i,
				prWtbl[i].ucUsed, prWtbl[i].ucBssIndex, prWtbl[i].ucKeyId,
				prWtbl[i].ucPairwise, prWtbl[i].ucStaIndex, MAC2STR(prWtbl[i].aucMacAddr));
	}
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Assin the wlan table with the join AP info
*
* \param[in] prAdapter Pointer to the Adapter structure
*
* \note
*/
/*----------------------------------------------------------------------------*/
void secPostUpdateAddr(IN P_ADAPTER_T prAdapter, IN P_BSS_INFO_T prBssInfo)
{
	P_CONNECTION_SETTINGS_T prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	P_WLAN_TABLE_T prWtbl;

	if (IS_BSS_AIS(prBssInfo) && prBssInfo->prStaRecOfAP) {

		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED) {

			if (prBssInfo->fgBcDefaultKeyExist) {

				prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->wepkeyWlanIdx];

				kalMemCopy(prWtbl->aucMacAddr, prBssInfo->prStaRecOfAP->aucMacAddr, MAC_ADDR_LEN);
				prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
				DBGLOG(RSN, INFO, "secPostUpdateAddr at [%d] " MACSTR "= STA Index=%d\n",
				       prBssInfo->wepkeyWlanIdx, MAC2STR(prWtbl->aucMacAddr),
				       prBssInfo->prStaRecOfAP->ucIndex);

				/* Update the wlan table of the prStaRecOfAP */
				prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->prStaRecOfAP->ucWlanIndex];
				prWtbl->ucKeyId = prBssInfo->ucBcDefaultKeyIdx;
				prBssInfo->prStaRecOfAP->fgTransmitKeyExist = TRUE;
			}
		}
		if (prConnSettings->eEncStatus == ENUM_ENCRYPTION_DISABLED) {
			prWtbl = &prAdapter->rWifiVar.arWtbl[prBssInfo->ucBMCWlanIndex];

			kalMemCopy(prWtbl->aucMacAddr, prBssInfo->prStaRecOfAP->aucMacAddr, MAC_ADDR_LEN);
			prWtbl->ucStaIndex = prBssInfo->prStaRecOfAP->ucIndex;
			DBGLOG(RSN, INFO, "secPostUpdateAddr at [%d] " MACSTR "= STA Index=%d\n",
			       prBssInfo->ucBMCWlanIndex, MAC2STR(prWtbl->aucMacAddr),
			       prBssInfo->prStaRecOfAP->ucIndex);
		}
	}
}
