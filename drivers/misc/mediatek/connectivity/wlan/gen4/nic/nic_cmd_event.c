/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/nic/nic_cmd_event.c#3
*/

/*! \file   nic_cmd_event.c
*    \brief  Callback functions for Command packets.
*
*	Various Event packet handlers which will be setup in the callback function of
*    a command packet.
*/

/*
** Log: nic_cmd_event.c
**
** 06 12 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch]
**	update BLBIST dump burst mode
**
**	Review: http://mtksap20:8080/go?page=NewReview&reviewid=110351
**
** 04 08 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch]
** add for BLBIST dump index
**
** 01 15 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch][MT6630][Driver]MT6630 Wi-Fi Patch
** Merging
**
**	//ALPS_SW/DEV/ALPS.JB2.MT6630.DEV/alps/mediatek/kernel/drivers/combo/drv_wlan/mt6630/wlan/...
**
**	to //ALPS_SW/TRUNK/KK/alps/mediatek/kernel/drivers/combo/drv_wlan/mt6630/wlan/...
**
** 12 27 2013 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch][MT6630][Driver]MT6630 Wi-Fi Patch
** update code for ICAP & nvram
**
** 08 20 2013 eason.tsai
** [BORA00002255] [MT6630 Wi-Fi][Driver] develop
** Icap function
**
** 08 20 2013 eason.tsai
** [BORA00002255] [MT6630 Wi-Fi][Driver] develop
** ICAP part for win32
**
** 08 09 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** 1. integrate scheduled scan functionality
** 2. condition compilation for linux-3.4 & linux-3.8 compatibility
** 3. correct CMD queue access to reduce lock scope
**
** 06 19 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** update MAC address handling logic
**
** 06 18 2013 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Get MAC address by NIC_CAPABILITY command
**
** 06 18 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** Update for 1st connection
**
** 02 19 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** enable build for nic_rx.c & nic_cmd_event.c
**
** 01 22 2013 cp.wu
** [BORA00002253] [MT6630 Wi-Fi][Driver][Firmware] Add NLO and timeout mechanism to SCN module
** modification for ucBssIndex migration
**
** 10 25 2012 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** sync with MT6630 HIFSYS update.
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
**
** 09 04 2012 cp.wu
** [WCXRP00001269] [MT6620 Wi-Fi][Driver] cfg80211 porting merge back to DaVinci
** sync RSSI ignoring when BSS is disconnected
**
** 08 24 2012 cp.wu
** [WCXRP00001269] [MT6620 Wi-Fi][Driver] cfg80211 porting merge back to DaVinci
** .
**
** 08 24 2012 cp.wu
** [WCXRP00001269] [MT6620 Wi-Fi][Driver] cfg80211 porting merge back to DaVinci
** cfg80211 support merge back from ALPS.JB to DaVinci - MT6620 Driver v2.3 branch.
 *
 * 04 10 2012 yuche.tsai
 * NULL
 * Update address for wifi direct connection issue.
 *
 * 06 15 2011 cm.chang
 * [WCXRP00000785] [MT6620 Wi-Fi][Driver][FW] P2P/BOW MAC address is XOR with AIS MAC address
 * P2P/BOW mac address XOR with local bit instead of OR
 *
 * 03 05 2011 wh.su
 * [WCXRP00000506] [MT6620 Wi-Fi][Driver][FW] Add Security check related code
 * add the code to get the check rsponse and indicate to app.
 *
 * 03 02 2011 wh.su
 * [WCXRP00000506] [MT6620 Wi-Fi][Driver][FW] Add Security check related code
 * Add security check code.
 *
 * 02 24 2011 cp.wu
 * [WCXRP00000493] [MT6620 Wi-Fi][Driver] Do not indicate redundant
 * disconnection to host when entering into RF test mode
 * only indicate DISCONNECTION to host when entering RF test if necessary (connected -> disconnected cases)
 *
 * 01 20 2011 eddie.chen
 * [WCXRP00000374] [MT6620 Wi-Fi][DRV] SW debug control
 * Add Oid for sw control debug command
 *
 * 12 31 2010 cp.wu
 * [WCXRP00000335] [MT6620 Wi-Fi][Driver] change to use milliseconds sleep
 * instead of delay to avoid blocking to system scheduling
 * change to use msleep() and shorten waiting interval to reduce
 * blocking to other task while Wi-Fi driver is being loaded
 *
 * 12 01 2010 cp.wu
 * [WCXRP00000223] MT6620 Wi-Fi][Driver][FW] Adopt NVRAM parameters when enter/exit RF test mode
 * reload NVRAM settings before entering RF test mode and leaving from RF test mode.
 *
 * 11 01 2010 cp.wu
 * [WCXRP00000056] [MT6620 Wi-Fi][Driver] NVRAM implementation
 * with Version Check[WCXRP00000150] [MT6620 Wi-Fi][Driver] Add
 * implementation for querying current TX rate from firmware auto rate module
 * 1) Query link speed (TX rate) from firmware directly with buffering mechanism to reduce overhead
 * 2) Remove CNM CH-RECOVER event handling
 * 3) cfg read/write API renamed with kal prefix for unified naming rules.
 *
 * 10 20 2010 cp.wu
 * [WCXRP00000117] [MT6620 Wi-Fi][Driver] Add logic for suspending driver when MT6620 is not responding anymore
 * use OID_CUSTOM_TEST_MODE as indication for driver reset
 * by dropping pending TX packets
 *
 * 10 18 2010 cp.wu
 * [WCXRP00000056] [MT6620 Wi-Fi][Driver] NVRAM implementation
 * with Version Check[WCXRP00000086] [MT6620 Wi-Fi][Driver] The mac address is all zero at android
 * complete implementation of Android NVRAM access
 *
 * 09 21 2010 cp.wu
 * [WCXRP00000053] [MT6620 Wi-Fi][Driver] Reset incomplete
 * and might leads to BSOD when entering RF test with AIS associated
 * Do a complete reset with STA-REC null checking for RF test re-entry
 *
 * 09 15 2010 yuche.tsai
 * NULL
 * Start to test AT GO only when P2P state is not IDLE.
 *
 * 09 09 2010 yuche.tsai
 * NULL
 * Add AT GO Test mode after MAC address available.
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
 * Replace CFG_SUPPORT_BOW by CFG_ENABLE_BT_OVER_WIFI.
 * There is no CFG_SUPPORT_BOW in driver domain source.
 *
 * 08 12 2010 cp.wu
 * NULL
 * [AIS-FSM] honor registry setting for adhoc running mode. (A/B/G)
 *
 * 08 11 2010 yuche.tsai
 * NULL
 * Add support for P2P Device Address query from FW.
 *
 * 08 03 2010 cp.wu
 * NULL
 * Centralize mgmt/system service procedures into independent calls.
 *
 * 08 02 2010 cp.wu
 * NULL
 * reset FSMs before entering RF test mode.
 *
 * 07 22 2010 cp.wu
 *
 * 1) refine AIS-FSM indent.
 * 2) when entering RF Test mode, flush 802.1X frames as well
 * 3) when entering D3 state, flush 802.1X frames as well
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 07 05 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * 1) change fake BSS_DESC from channel 6 to channel 1 due to channel switching is not done yet.
 * 2) after MAC address is queried from firmware, all related variables in driver domain should be updated as well
 *
 * 06 21 2010 wh.su
 * [WPD00003840][MT6620 5931] Security migration
 * remove duplicate variable for migration.
 *
 * 06 06 2010 kevin.huang
 * [WPD00003832][MT6620 5931] Create driver base
 * [MT6620 5931] Create driver base
 *
 * 05 29 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * change upon request: indicate as disconnected in driver domain when leaving from RF test mode
 *
 * 05 24 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * do not clear scanning list array after disassociation
 *
 * 05 22 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1) disable NETWORK_LAYER_ADDRESSES handling temporally.
 * 2) finish statistics OIDs
 *
 * 05 22 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * change OID behavior to meet WHQL requirement.
 *
 * 05 20 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1) integrate OID_GEN_NETWORK_LAYER_ADDRESSES with CMD_ID_SET_IP_ADDRESS
 * 2) buffer statistics data for 2 seconds
 * 3) use default value for adhoc parameters instead of 0
 *
 * 05 19 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1) do not take timeout mechanism for power mode oids
 * 2) retrieve network type from connection status
 * 3) after disassciation, set radio state to off
 * 4) TCP option over IPv6 is supported
 *
 * 05 17 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * correct OID_802_11_DISASSOCIATE handling.
 *
 * 05 17 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * 1) add timeout handler mechanism for pending command packets
 * 2) add p2p add/removal key
 *
 * 04 16 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * treat BUS access failure as kind of card removal.
 *
 * 04 14 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * information buffer for query oid/ioctl is now buffered in prCmdInfo
 *  *  *  *  *  * instead of glue-layer variable to improve multiple oid/ioctl capability
 *
 * 04 13 2010 cp.wu
 * [WPD00003823][MT6620 Wi-Fi] Add Bluetooth-over-Wi-Fi support
 * add framework for BT-over-Wi-Fi support.
 *  *  *  *  *  *  *  *  *  *  *  *  *  * 1) prPendingCmdInfo is replaced by queue for multiple handler capability
 *  *  *  *  *  *  *  *  *  *  *  *  *  * 2) command sequence number is now increased atomically
 *  *  *  *  *  *  *  *  *  *  *  *  *  * 3) private data could be hold and taken use for other purpose
 *
 * 04 07 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * accessing to firmware load/start address, and access to OID handling information
 * are now handled in glue layer
 *
 * 04 07 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * rWlanInfo should be placed at adapter rather than glue due to most operations
 *  *  *  * are done in adapter layer.
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * add KAL API: kalFlushPendingTxPackets(), and take use of the API
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * eliminate direct access to prGlueInfo->rWlanInfo.eLinkAttr.ucMediaStreamMode from non-glue layer.
 *
 * 04 06 2010 jeffrey.chang
 * [WPD00003826]Initial import for Linux port
 * improve none-glude code portability
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * sync statistics data structure definition with firmware implementation
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * code refine: fgTestMode should be at adapter rather than glue due to the device/fw is also involved
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * eliminate direct access for prGlueInfo->fgIsCardRemoved in non-glue layer
 *
 * 03 30 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * statistics information OIDs are now handled by querying from firmware domain
 *
 * 03 26 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * indicate media stream mode after set is done
 *
 * 03 24 2010 jeffrey.chang
 * [WPD00003826]Initial import for Linux port
 * initial import for Linux port
 *
 * 03 03 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * implement custom OID: EEPROM read/write access
 *
 * 03 03 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * implement OID_802_3_MULTICAST_LIST oid handling
 *
 * 02 25 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * limit RSSI return value to microsoft defined range.
 *
 * 02 09 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1. Permanent and current MAC address are now retrieved by CMD/EVENT packets instead of hard-coded address
 *  *  *  *  *  *  * 2. follow MSDN defined behavior when associates to another AP
 *  *  *  *  *  *  * 3. for firmware download, packet size could be up to 2048 bytes
 *
 * 01 29 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * block until firmware finished RF test enter/leave then indicate completion to upper layer
 *
 * 01 29 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * when entering RF test mode and leaving from RF test mode,
 * wait for W_FUNC_RDY bit to be asserted forever until it is set or card is removed.
 *
 * 01 27 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1. eliminate improper variable in rHifInfo
 *  *  *  *  *  *  *  * 2. block TX/ordinary OID when RF test mode is engaged
 *  *  *  *  *  *  *  * 3. wait until firmware finish operation when entering into and leaving from RF test mode
 *  *  *  *  *  *  *  * 4. correct some HAL implementation
 *
 * 01 26 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * Under WinXP with SDIO, use prGlueInfo->rHifInfo.pvInformationBuffer instead of prGlueInfo->pvInformationBuffer
 *
 * 01 22 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * implement following 802.11 OIDs:
 *  *  *  *  * OID_802_11_RSSI,
 *  *  *  *  * OID_802_11_RSSI_TRIGGER,
 *  *  *  *  * OID_802_11_STATISTICS,
 *  *  *  *  * OID_802_11_DISASSOCIATE,
 *  *  *  *  * OID_802_11_POWER_MODE
 *
 * 01 21 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * implement OID_802_11_MEDIA_STREAM_MODE
 *
 * 12 30 2009 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1) According to CMD/EVENT documentation v0.8,
 *  *  *  *  *  *  *  * OID_CUSTOM_TEST_RX_STATUS & OID_CUSTOM_TEST_TX_STATUS is no longer used,
 *  *  *  *  *  *  *  * and result is retrieved by get ATInfo instead
 *  *  *  *  *  *  *  * 2) add 4 counter for recording aggregation statistics
**  \main\maintrunk.MT6620WiFiDriver_Prj\10 2009-12-10 16:47:47 GMT mtk02752
**  only handle MCR read when accessing FW domain register
**  \main\maintrunk.MT6620WiFiDriver_Prj\9 2009-12-08 17:37:28 GMT mtk02752
**  * refine nicCmdEventQueryMcrRead
**  + add TxStatus/RxStatus for RF test QueryInformation OIDs
**  \main\maintrunk.MT6620WiFiDriver_Prj\8 2009-12-02 22:05:45 GMT mtk02752
**  kalOidComplete() will decrease i4OidPendingCount
**  \main\maintrunk.MT6620WiFiDriver_Prj\7 2009-12-01 23:02:57 GMT mtk02752
**  remove unnecessary spin locks
**  \main\maintrunk.MT6620WiFiDriver_Prj\6 2009-12-01 22:51:18 GMT mtk02752
**  maintein i4OidPendingCount
**  \main\maintrunk.MT6620WiFiDriver_Prj\5 2009-11-30 10:55:03 GMT mtk02752
**  modify for compatibility
**  \main\maintrunk.MT6620WiFiDriver_Prj\4 2009-11-23 14:46:32 GMT mtk02752
**  add another version of command-done handler upon new event structure
**  \main\maintrunk.MT6620WiFiDriver_Prj\3 2009-04-29 15:42:33 GMT mtk01461
**  Add comment
**  \main\maintrunk.MT6620WiFiDriver_Prj\2 2009-04-21 19:32:42 GMT mtk01461
**  Add nicCmdEventSetCommon() for general set OID
**  \main\maintrunk.MT6620WiFiDriver_Prj\1 2009-04-21 01:40:35 GMT mtk01461
**  Command Done Handler
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
#include "gl_ate_agent.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
const NIC_CAPABILITY_V2_REF_TABLE_T gNicCapabilityV2InfoTable[] = {
	{TAG_CAP_TX_RESOURCE, nicCmdEventQueryNicTxResource},
	{TAG_CAP_TX_EFUSEADDRESS, nicCmdEventQueryNicEfuseAddr},
	{TAG_CAP_COEX_FEATURE, nicCmdEventQueryNicCoexFeature}
};

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                            F U N C T I O N   D A T A
********************************************************************************
*/
VOID nicCmdEventQueryMcrRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_PARAM_CUSTOM_MCR_RW_STRUCT_T prMcrRdInfo;
	P_GLUE_INFO_T prGlueInfo;
	P_CMD_ACCESS_REG prCmdAccessReg;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdAccessReg = (P_CMD_ACCESS_REG) (pucEventBuf);

		u4QueryInfoLen = sizeof(PARAM_CUSTOM_MCR_RW_STRUCT_T);

		prMcrRdInfo = (P_PARAM_CUSTOM_MCR_RW_STRUCT_T) prCmdInfo->pvInformationBuffer;
		prMcrRdInfo->u4McrOffset = prCmdAccessReg->u4Address;
		prMcrRdInfo->u4McrData = prCmdAccessReg->u4Data;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	return;

}

#if CFG_SUPPORT_QA_TOOL
VOID nicCmdEventQueryRxStatistics(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_PARAM_CUSTOM_ACCESS_RX_STAT prRxStatistics;
	P_EVENT_ACCESS_RX_STAT prEventAccessRxStat;
	UINT_32 u4QueryInfoLen, i;
	P_GLUE_INFO_T prGlueInfo;
	PUINT_32 prElement;
	UINT_32 u4Temp;
	/* P_CMD_ACCESS_RX_STAT                  prCmdRxStat, prRxStat; */

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventAccessRxStat = (P_EVENT_ACCESS_RX_STAT) (pucEventBuf);

		prRxStatistics = (P_PARAM_CUSTOM_ACCESS_RX_STAT) prCmdInfo->pvInformationBuffer;
		prRxStatistics->u4SeqNum = prEventAccessRxStat->u4SeqNum;
		prRxStatistics->u4TotalNum = prEventAccessRxStat->u4TotalNum;

		u4QueryInfoLen = sizeof(CMD_ACCESS_RX_STAT);

		if (prRxStatistics->u4SeqNum == u4RxStatSeqNum) {
			prElement = &g_HqaRxStat.MAC_FCS_Err;
			for (i = 0; i < HQA_RX_STATISTIC_NUM; i++) {
				u4Temp = ntohl(prEventAccessRxStat->au4Buffer[i]);
				kalMemCopy(prElement, &u4Temp, 4);

				if (i < (HQA_RX_STATISTIC_NUM - 1))
					prElement++;
			}

			g_HqaRxStat.AllMacMdrdy0 = ntohl(prEventAccessRxStat->au4Buffer[i]);
			i++;
			g_HqaRxStat.AllMacMdrdy1 = ntohl(prEventAccessRxStat->au4Buffer[i]);
			/* i++; */
			/* g_HqaRxStat.AllFCSErr0 = ntohl(prEventAccessRxStat->au4Buffer[i]); */
			/* i++; */
			/* g_HqaRxStat.AllFCSErr1 = ntohl(prEventAccessRxStat->au4Buffer[i]); */
		}

		DBGLOG(INIT, ERROR,
			"MT6632 : RX Statistics Test SeqNum = %d, TotalNum = %d\n",
			 (unsigned int)prEventAccessRxStat->u4SeqNum, (unsigned int)prEventAccessRxStat->u4TotalNum);

		DBGLOG(INIT, ERROR, "MAC_FCS_ERR = %d, MAC_MDRDY = %d, MU_RX_CNT = %d, RX_FIFO_FULL = %d\n",
			 (unsigned int)prEventAccessRxStat->au4Buffer[0],
				(unsigned int)prEventAccessRxStat->au4Buffer[1],
				(unsigned int)prEventAccessRxStat->au4Buffer[65],
				(unsigned int)prEventAccessRxStat->au4Buffer[22]);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

#if CFG_SUPPORT_TX_BF
VOID nicCmdEventPfmuDataRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_PFMU_DATA prEventPfmuDataRead = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventPfmuDataRead = (P_PFMU_DATA) (pucEventBuf);

		u4QueryInfoLen = sizeof(PFMU_DATA);

		g_rPfmuData = *prEventPfmuDataRead;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	DBGLOG(INIT, INFO, "=========== Before ===========\n");
	if (prEventPfmuDataRead != NULL) {
		DBGLOG(INIT, INFO, "u2Phi11 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi11);
		DBGLOG(INIT, INFO, "ucPsi21 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi21);
		DBGLOG(INIT, INFO, "u2Phi21 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi21);
		DBGLOG(INIT, INFO, "ucPsi31 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi31);
		DBGLOG(INIT, INFO, "u2Phi31 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi31);
		DBGLOG(INIT, INFO, "ucPsi41 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi41);
		DBGLOG(INIT, INFO, "u2Phi22 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi22);
		DBGLOG(INIT, INFO, "ucPsi32 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi32);
		DBGLOG(INIT, INFO, "u2Phi32 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi32);
		DBGLOG(INIT, INFO, "ucPsi42 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi42);
		DBGLOG(INIT, INFO, "u2Phi33 = 0x%x\n", prEventPfmuDataRead->rField.u2Phi33);
		DBGLOG(INIT, INFO, "ucPsi43 = 0x%x\n", prEventPfmuDataRead->rField.ucPsi43);
		DBGLOG(INIT, INFO, "u2dSNR00 = 0x%x\n", prEventPfmuDataRead->rField.u2dSNR00);
		DBGLOG(INIT, INFO, "u2dSNR01 = 0x%x\n", prEventPfmuDataRead->rField.u2dSNR01);
		DBGLOG(INIT, INFO, "u2dSNR02 = 0x%x\n", prEventPfmuDataRead->rField.u2dSNR02);
		DBGLOG(INIT, INFO, "u2dSNR03 = 0x%x\n", prEventPfmuDataRead->rField.u2dSNR03);
	}
}

VOID nicCmdEventPfmuTagRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_PFMU_TAG_READ_T prEventPfmuTagRead = NULL;
	P_PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T prPfumTagRead = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventPfmuTagRead = (P_EVENT_PFMU_TAG_READ_T) (pucEventBuf);

		prPfumTagRead = (P_PARAM_CUSTOM_PFMU_TAG_READ_STRUCT_T) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prPfumTagRead, prEventPfmuTagRead, sizeof(EVENT_PFMU_TAG_READ_T));

		u4QueryInfoLen = sizeof(CMD_TXBF_ACTION_T);

		g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1;
		g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
	DBGLOG(INIT, INFO, "========================== (R)Tag1 info ==========================\n");

	DBGLOG(INIT, INFO, " Row data0 : %x, Row data1 : %x, Row data2 : %x, Row data3 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[0],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[1],
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[2], prEventPfmuTagRead->ru4TxBfPFMUTag1.au4RawData[3]);
	DBGLOG(INIT, INFO, "ProfileID = %d Invalid status = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucProfileID,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucInvalidProf);
	DBGLOG(INIT, INFO, "0:iBF / 1:eBF = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucTxBf);
	DBGLOG(INIT, INFO, "0:SU / 1:MU = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSU_MU);
	DBGLOG(INIT, INFO, "DBW(0/1/2/3 BW20/40/80/160NC) = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucDBW);
	DBGLOG(INIT, INFO, "RMSD = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucRMSD);
	DBGLOG(INIT, INFO, "Nrow = %d, Ncol = %d, Ng = %d, LM = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNrow,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNcol,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucNgroup, prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucLM);
	DBGLOG(INIT, INFO, "Mem1(%d, %d), Mem2(%d, %d), Mem3(%d, %d), Mem4(%d, %d)\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr1RowIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2ColIdx,
	       (prEventPfmuTagRead->ru4TxBfPFMUTag1.
		rField.ucMemAddr2RowIdx | (prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr2RowIdxMsb << 5)),
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr3RowIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4ColIdx,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucMemAddr4RowIdx);
	DBGLOG(INIT, INFO, "SNR STS0=0x%x, SNR STS1=0x%x, SNR STS2=0x%x, SNR STS3=0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS0,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS1,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS2,
	       prEventPfmuTagRead->ru4TxBfPFMUTag1.rField.ucSNR_STS3);
	DBGLOG(INIT, INFO, "===============================================================\n");

	DBGLOG(INIT, INFO, "========================== (R)Tag2 info ==========================\n");
	DBGLOG(INIT, INFO, " Row data0 : %x, Row data1 : %x, Row data2 : %x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[0],
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[1], prEventPfmuTagRead->ru4TxBfPFMUTag2.au4RawData[2]);
	DBGLOG(INIT, INFO, "Smart Ant Cfg = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.u2SmartAnt);
	DBGLOG(INIT, INFO, "SE index = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucSEIdx);
	DBGLOG(INIT, INFO, "RMSD Threshold = %d\n", prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucRMSDThd);
	DBGLOG(INIT, INFO, "MCS TH L1SS = %d, S1SS = %d, L2SS = %d, S2SS = %d\n"
	       "L3SS = %d, S3SS = %d\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL1SS,
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS1SS,
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL2SS,
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS2SS,
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThL3SS,
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.ucMCSThS3SS);
	DBGLOG(INIT, INFO, "iBF lifetime limit(unit:4ms) = 0x%x\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfTimeOut);
	DBGLOG(INIT, INFO, "iBF desired DBW = %d\n  0/1/2/3 : BW20/40/80/160NC\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfDBW);
	DBGLOG(INIT, INFO, "iBF desired Ncol = %d\n  0/1/2 : Ncol = 1 ~ 3\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNcol);
	DBGLOG(INIT, INFO, "iBF desired Nrow = %d\n  0/1/2/3 : Nrow = 1 ~ 4\n",
	       prEventPfmuTagRead->ru4TxBfPFMUTag2.rField.uciBfNrow);
	DBGLOG(INIT, INFO, "===============================================================\n");

}

#endif /* CFG_SUPPORT_TX_BF */
#if CFG_SUPPORT_MU_MIMO
VOID nicCmdEventGetQd(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_HQA_GET_QD prEventHqaGetQd;
	UINT_32 i;

	P_PARAM_CUSTOM_GET_QD_STRUCT_T prGetQd = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventHqaGetQd = (P_EVENT_HQA_GET_QD) (pucEventBuf);

		prGetQd = (P_PARAM_CUSTOM_GET_QD_STRUCT_T) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prGetQd, prEventHqaGetQd, sizeof(EVENT_HQA_GET_QD));

		u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

		/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
		/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	DBGLOG(INIT, INFO, " event id : %x\n", prGetQd->u4EventId);
	for (i = 0; i < 14; i++)
		DBGLOG(INIT, INFO, "au4RawData[%d]: %x\n", i, prGetQd->au4RawData[i]);

}

VOID nicCmdEventGetCalcLq(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_HQA_GET_MU_CALC_LQ prEventHqaGetMuCalcLq;
	UINT_32 i, j;

	P_PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T prGetMuCalcLq = NULL;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventHqaGetMuCalcLq = (P_EVENT_HQA_GET_MU_CALC_LQ) (pucEventBuf);

		prGetMuCalcLq = (P_PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT_T) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prGetMuCalcLq, prEventHqaGetMuCalcLq, sizeof(EVENT_HQA_GET_MU_CALC_LQ));

		u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

		/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
		/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	DBGLOG(INIT, INFO, " event id : %x\n", prGetMuCalcLq->u4EventId);
	for (i = 0; i < NUM_OF_USER; i++)
		for (j = 0; j < NUM_OF_MODUL; j++)
			DBGLOG(INIT, INFO, " lq_report[%d][%d]: %x\n", i, j, prGetMuCalcLq->rEntry.lq_report[i][j]);

}

VOID nicCmdEventGetCalcInitMcs(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_SHOW_GROUP_TBL_ENTRY prEventShowGroupTblEntry = NULL;

	P_PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T prShowGroupTbl;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventShowGroupTblEntry = (P_EVENT_SHOW_GROUP_TBL_ENTRY) (pucEventBuf);

		prShowGroupTbl = (P_PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT_T) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prShowGroupTbl, prEventShowGroupTblEntry, sizeof(EVENT_SHOW_GROUP_TBL_ENTRY));

		u4QueryInfoLen = sizeof(CMD_MUMIMO_ACTION_T);

		/* g_rPfmuTag1 = prPfumTagRead->ru4TxBfPFMUTag1; */
		/* g_rPfmuTag2 = prPfumTagRead->ru4TxBfPFMUTag2; */

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

	DBGLOG(INIT, INFO, "========================== (R)Group table info ==========================\n");
	DBGLOG(INIT, INFO, " event id : %x\n", prEventShowGroupTblEntry->u4EventId);
	DBGLOG(INIT, INFO, "index = %x numUser = %x\n", prEventShowGroupTblEntry->index,
	       prEventShowGroupTblEntry->numUser);
	DBGLOG(INIT, INFO, "BW = %x NS0/1/ = %x/%x\n", prEventShowGroupTblEntry->BW, prEventShowGroupTblEntry->NS0,
	       prEventShowGroupTblEntry->NS1);
	DBGLOG(INIT, INFO, "PFIDUser0/1 = %x/%x\n", prEventShowGroupTblEntry->PFIDUser0,
	       prEventShowGroupTblEntry->PFIDUser1);
	DBGLOG(INIT, INFO, "fgIsShortGI = %x, fgIsUsed = %x, fgIsDisable = %x\n", prEventShowGroupTblEntry->fgIsShortGI,
	       prEventShowGroupTblEntry->fgIsUsed, prEventShowGroupTblEntry->fgIsDisable);
	DBGLOG(INIT, INFO, "initMcsUser0/1 = %x/%x\n", prEventShowGroupTblEntry->initMcsUser0,
	       prEventShowGroupTblEntry->initMcsUser1);
	DBGLOG(INIT, INFO, "dMcsUser0: 0/1/ = %x/%x\n", prEventShowGroupTblEntry->dMcsUser0,
	       prEventShowGroupTblEntry->dMcsUser1);

}
#endif /* CFG_SUPPORT_MU_MIMO */
#endif /* CFG_SUPPORT_QA_TOOL */

VOID nicCmdEventQuerySwCtrlRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_PARAM_CUSTOM_SW_CTRL_STRUCT_T prSwCtrlInfo;
	P_GLUE_INFO_T prGlueInfo;
	P_CMD_SW_DBG_CTRL_T prCmdSwCtrl;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdSwCtrl = (P_CMD_SW_DBG_CTRL_T) (pucEventBuf);

		u4QueryInfoLen = sizeof(PARAM_CUSTOM_SW_CTRL_STRUCT_T);

		prSwCtrlInfo = (P_PARAM_CUSTOM_SW_CTRL_STRUCT_T) prCmdInfo->pvInformationBuffer;
		prSwCtrlInfo->u4Id = prCmdSwCtrl->u4Id;
		prSwCtrlInfo->u4Data = prCmdSwCtrl->u4Data;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventQueryChipConfig(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T prChipConfigInfo;
	P_GLUE_INFO_T prGlueInfo;
	P_CMD_CHIP_CONFIG_T prCmdChipConfig;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prCmdChipConfig = (P_CMD_CHIP_CONFIG_T) (pucEventBuf);

		u4QueryInfoLen = sizeof(PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T);

		if (prCmdInfo->u4InformationBufferLength < sizeof(PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T)) {
			DBGLOG(REQ, INFO,
			       "Chip config u4InformationBufferLength %u is not valid (event)\n",
			       prCmdInfo->u4InformationBufferLength);
		}
		prChipConfigInfo = (P_PARAM_CUSTOM_CHIP_CONFIG_STRUCT_T) prCmdInfo->pvInformationBuffer;
		prChipConfigInfo->ucRespType = prCmdChipConfig->ucRespType;
		prChipConfigInfo->u2MsgSize = prCmdChipConfig->u2MsgSize;
		DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__, prChipConfigInfo->ucRespType);
		DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__, prChipConfigInfo->u2MsgSize);

#if 0
		if (prChipConfigInfo->u2MsgSize > CHIP_CONFIG_RESP_SIZE) {
			DBGLOG(REQ, INFO,
			       "Chip config Msg Size %u is not valid (event)\n", prChipConfigInfo->u2MsgSize);
			prChipConfigInfo->u2MsgSize = CHIP_CONFIG_RESP_SIZE;
		}
#endif
		kalMemCopy(prChipConfigInfo->aucCmd, prCmdChipConfig->aucCmd, prChipConfigInfo->u2MsgSize);
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventSetCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
			       prCmdInfo->u4InformationBufferLength, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventSetDisassociate(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
	}

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);

#if !defined(LINUX)
	prAdapter->fgIsRadioOff = TRUE;
#endif

}

VOID nicCmdEventSetIpAddress(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4Count;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	u4Count = (prCmdInfo->u4SetInfoLen - OFFSET_OF(CMD_SET_NETWORK_ADDRESS_LIST, arNetAddress))
	    / sizeof(IPV4_NETWORK_ADDRESS);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery,
			       OFFSET_OF(PARAM_NETWORK_ADDRESS_LIST, arAddress) + u4Count *
			       (OFFSET_OF(PARAM_NETWORK_ADDRESS, aucAddress) +
				sizeof(PARAM_NETWORK_ADDRESS_IP)), WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventQueryRfTestATInfo(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_TEST_STATUS prTestStatus, prQueryBuffer;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prTestStatus = (P_EVENT_TEST_STATUS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prQueryBuffer = (P_EVENT_TEST_STATUS) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prQueryBuffer, prTestStatus, sizeof(EVENT_TEST_STATUS));

		u4QueryInfoLen = sizeof(EVENT_TEST_STATUS);

		/* Update Query Information Length */
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventQueryLinkQuality(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	PARAM_RSSI rRssi, *prRssi;
	P_EVENT_LINK_QUALITY prLinkQuality;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prLinkQuality = (P_EVENT_LINK_QUALITY) pucEventBuf;

	rRssi = (PARAM_RSSI) prLinkQuality->cRssi;	/* ranged from (-128 ~ 30) in unit of dBm */

	if (prAdapter->prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		if (rRssi > PARAM_WHQL_RSSI_MAX_DBM)
			rRssi = PARAM_WHQL_RSSI_MAX_DBM;
		else if (rRssi < PARAM_WHQL_RSSI_MIN_DBM)
			rRssi = PARAM_WHQL_RSSI_MIN_DBM;
	} else {
		rRssi = PARAM_WHQL_RSSI_MIN_DBM;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prRssi = (PARAM_RSSI *) prCmdInfo->pvInformationBuffer;

		kalMemCopy(prRssi, &rRssi, sizeof(PARAM_RSSI));
		u4QueryInfoLen = sizeof(PARAM_RSSI);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This routine is in response of OID_GEN_LINK_SPEED query request
*
* @param prAdapter      Pointer to the Adapter structure.
* @param prCmdInfo      Pointer to the pending command info
* @param pucEventBuf
*
* @retval none
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventQueryLinkSpeed(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_LINK_QUALITY prLinkQuality;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4LinkSpeed;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prLinkQuality = (P_EVENT_LINK_QUALITY) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		pu4LinkSpeed = (PUINT_32) (prCmdInfo->pvInformationBuffer);

		*pu4LinkSpeed = prLinkQuality->u2LinkSpeed * 5000;

		u4QueryInfoLen = sizeof(UINT_32);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryStatistics(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_PARAM_802_11_STATISTICS_STRUCT_T prStatistics;
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		u4QueryInfoLen = sizeof(PARAM_802_11_STATISTICS_STRUCT_T);
		prStatistics = (P_PARAM_802_11_STATISTICS_STRUCT_T) prCmdInfo->pvInformationBuffer;

		prStatistics->u4Length = sizeof(PARAM_802_11_STATISTICS_STRUCT_T);
		prStatistics->rTransmittedFragmentCount = prEventStatistics->rTransmittedFragmentCount;
		prStatistics->rMulticastTransmittedFrameCount = prEventStatistics->rMulticastTransmittedFrameCount;
		prStatistics->rFailedCount = prEventStatistics->rFailedCount;
		prStatistics->rRetryCount = prEventStatistics->rRetryCount;
		prStatistics->rMultipleRetryCount = prEventStatistics->rMultipleRetryCount;
		prStatistics->rRTSSuccessCount = prEventStatistics->rRTSSuccessCount;
		prStatistics->rRTSFailureCount = prEventStatistics->rRTSFailureCount;
		prStatistics->rACKFailureCount = prEventStatistics->rACKFailureCount;
		prStatistics->rFrameDuplicateCount = prEventStatistics->rFrameDuplicateCount;
		prStatistics->rReceivedFragmentCount = prEventStatistics->rReceivedFragmentCount;
		prStatistics->rMulticastReceivedFrameCount = prEventStatistics->rMulticastReceivedFrameCount;
		prStatistics->rFCSErrorCount = prEventStatistics->rFCSErrorCount;
		prStatistics->rTKIPLocalMICFailures.QuadPart = 0;
		prStatistics->rTKIPICVErrors.QuadPart = 0;
		prStatistics->rTKIPCounterMeasuresInvoked.QuadPart = 0;
		prStatistics->rTKIPReplays.QuadPart = 0;
		prStatistics->rCCMPFormatErrors.QuadPart = 0;
		prStatistics->rCCMPReplays.QuadPart = 0;
		prStatistics->rCCMPDecryptErrors.QuadPart = 0;
		prStatistics->rFourWayHandshakeFailures.QuadPart = 0;
		prStatistics->rWEPUndecryptableCount.QuadPart = 0;
		prStatistics->rWEPICVErrorCount.QuadPart = 0;
		prStatistics->rDecryptSuccessCount.QuadPart = 0;
		prStatistics->rDecryptFailureCount.QuadPart = 0;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventEnterRfTest(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	/* [driver-land] */
	/* prAdapter->fgTestMode = TRUE; */
	if (prAdapter->fgTestMode)
		prAdapter->fgTestMode = FALSE;
	else
		prAdapter->fgTestMode = TRUE;

	/* 0. always indicate disconnection */
	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) != PARAM_MEDIA_STATE_DISCONNECTED)
		kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);
	/* 1. Remove pending TX */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Security / Management Frames */
	kalClearSecurityFrames(prAdapter->prGlueInfo);
	kalClearMgmtFrames(prAdapter->prGlueInfo);

	/* 1.2 clear pending TX packet queued in glue layer */
	kalFlushPendingTxPackets(prAdapter->prGlueInfo);

	/* 2. Reset driver-domain FSMs */
	nicUninitMGMT(prAdapter);

	nicResetSystemService(prAdapter);
	nicInitMGMT(prAdapter, NULL);

#if defined(_HIF_SDIO) && 0
	/* 3. Disable Interrupt */
	HAL_INTR_DISABLE(prAdapter);

	/* 4. Block til firmware completed entering into RF test mode */
	kalMsleep(500);
	while (1) {
		UINT_32 u4Value;

		HAL_MCR_RD(prAdapter, MCR_WCIR, &u4Value);

		if (u4Value & WCIR_WLAN_READY) {
			break;
		} else if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE || fgIsBusAccessFailed == TRUE) {
			if (prCmdInfo->fgIsOid) {
				/* Update Set Information Length */
				kalOidComplete(prAdapter->prGlueInfo,
					       prCmdInfo->fgSetQuery,
					       prCmdInfo->u4SetInfoLen, WLAN_STATUS_NOT_SUPPORTED);

			}
			return;
		}
			kalMsleep(10);
	}

	/* 5. Clear Interrupt Status */
	{
		UINT_32 u4WHISR = 0;
		UINT_16 au2TxCount[16];

		HAL_READ_INTR_STATUS(prAdapter, 4, (PUINT_8)&u4WHISR);
		if (HAL_IS_TX_DONE_INTR(u4WHISR))
			HAL_READ_TX_RELEASED_COUNT(prAdapter, au2TxCount);
	}
	/* 6. Reset TX Counter */
	nicTxResetResource(prAdapter);

	/* 7. Re-enable Interrupt */
	HAL_INTR_ENABLE(prAdapter);
#endif

	/* 8. completion indication */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
	}
#if CFG_SUPPORT_NVRAM
	/* 9. load manufacture data */
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		wlanLoadManufactureData(prAdapter, kalGetConfiguration(prAdapter->prGlueInfo));
	else
		DBGLOG(REQ, WARN, "%s: load manufacture data fail\n", __func__);
#endif

}

VOID nicCmdEventLeaveRfTest(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
#if defined(_HIF_SDIO) && 0
	UINT_32 u4WHISR = 0;
	UINT_16 au2TxCount[16];
	UINT_32 u4Value;

	/* 1. Disable Interrupt */
	HAL_INTR_DISABLE(prAdapter);

	/* 2. Block until firmware completed leaving from RF test mode */
	kalMsleep(500);
	while (1) {
		HAL_MCR_RD(prAdapter, MCR_WCIR, &u4Value);

		if (u4Value & WCIR_WLAN_READY) {
			break;
		} else if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE || fgIsBusAccessFailed == TRUE) {
			if (prCmdInfo->fgIsOid) {
				/* Update Set Information Length */
				kalOidComplete(prAdapter->prGlueInfo,
					       prCmdInfo->fgSetQuery,
					       prCmdInfo->u4SetInfoLen, WLAN_STATUS_NOT_SUPPORTED);

			}
			return;
		}
			kalMsleep(10);
	}
	/* 3. Clear Interrupt Status */
	HAL_READ_INTR_STATUS(prAdapter, 4, (PUINT_8)&u4WHISR);
	if (HAL_IS_TX_DONE_INTR(u4WHISR))
		HAL_READ_TX_RELEASED_COUNT(prAdapter, au2TxCount);
	/* 4. Reset TX Counter */
	nicTxResetResource(prAdapter);

	/* 5. Re-enable Interrupt */
	HAL_INTR_ENABLE(prAdapter);
#endif

	/* 6. set driver-land variable */
	prAdapter->fgTestMode = FALSE;
	prAdapter->fgIcapMode = FALSE;

	/* 7. completion indication */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
	}

	/* 8. Indicate as disconnected */
	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo) != PARAM_MEDIA_STATE_DISCONNECTED) {

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_MEDIA_DISCONNECT, NULL, 0);

		prAdapter->rWlanInfo.u4SysTime = kalGetTimeTick();
	}
#if CFG_SUPPORT_NVRAM
	/* 9. load manufacture data */
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		wlanLoadManufactureData(prAdapter, kalGetConfiguration(prAdapter->prGlueInfo));
	else
		DBGLOG(REQ, WARN, "%s: load manufacture data fail\n", __func__);
#endif

	/* 10. Override network address */
	wlanUpdateNetworkAddress(prAdapter);

}

VOID nicCmdEventQueryMcastAddr(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_MAC_MCAST_ADDR prEventMacMcastAddr;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventMacMcastAddr = (P_EVENT_MAC_MCAST_ADDR) (pucEventBuf);

		u4QueryInfoLen = prEventMacMcastAddr->u4NumOfGroupAddr * MAC_ADDR_LEN;

		/* buffer length check */
		if (prCmdInfo->u4InformationBufferLength < u4QueryInfoLen) {
			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_BUFFER_TOO_SHORT);
		} else {
			kalMemCopy(prCmdInfo->pvInformationBuffer,
				   prEventMacMcastAddr->arAddress,
				   prEventMacMcastAddr->u4NumOfGroupAddr * MAC_ADDR_LEN);

			kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
		}
	}
}

VOID nicCmdEventQueryEepromRead(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_PARAM_CUSTOM_EEPROM_RW_STRUCT_T prEepromRdInfo;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_ACCESS_EEPROM prEventAccessEeprom;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventAccessEeprom = (P_EVENT_ACCESS_EEPROM) (pucEventBuf);

		u4QueryInfoLen = sizeof(PARAM_CUSTOM_EEPROM_RW_STRUCT_T);

		prEepromRdInfo = (P_PARAM_CUSTOM_EEPROM_RW_STRUCT_T) prCmdInfo->pvInformationBuffer;
		prEepromRdInfo->ucEepromIndex = (UINT_8) (prEventAccessEeprom->u2Offset);
		prEepromRdInfo->u2EepromData = prEventAccessEeprom->u2Data;

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventSetMediaStreamMode(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	PARAM_MEDIA_STREAMING_INDICATION rParamMediaStreamIndication;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4SetInfoLen, WLAN_STATUS_SUCCESS);
	}

	rParamMediaStreamIndication.rStatus.eStatusType = ENUM_STATUS_TYPE_MEDIA_STREAM_MODE;
	rParamMediaStreamIndication.eMediaStreamMode =
	    prAdapter->rWlanInfo.eLinkAttr.ucMediaStreamMode == 0 ? ENUM_MEDIA_STREAM_OFF : ENUM_MEDIA_STREAM_ON;

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
				     WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
				     (PVOID)&rParamMediaStreamIndication, sizeof(PARAM_MEDIA_STREAMING_INDICATION));
}

VOID nicCmdEventSetStopSchedScan(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	/*
	 *  DBGLOG(SCN, INFO, "--->nicCmdEventSetStopSchedScan\n" ));
	 */
	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	/*
	 *  DBGLOG(SCN, INFO, "<--kalSchedScanStopped\n" );
	 */
	if (prCmdInfo->fgIsOid) {
		/* Update Set Information Length */
		kalOidComplete(prAdapter->prGlueInfo,
			       prCmdInfo->fgSetQuery, prCmdInfo->u4InformationBufferLength, WLAN_STATUS_SUCCESS);
	}

	DBGLOG(SCN, INFO, "nicCmdEventSetStopSchedScan OID done, release lock and send event to uplayer\n");
	/*Due to dead lock issue, need to release the IO control before calling kernel APIs */
	kalSchedScanStopped(prAdapter->prGlueInfo);

}

/* Statistics responder */
VOID nicCmdEventQueryXmitOk(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rTransmittedFragmentCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = prEventStatistics->rTransmittedFragmentCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryRecvOk(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rReceivedFragmentCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = prEventStatistics->rReceivedFragmentCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryXmitError(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rFailedCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = (UINT_64) prEventStatistics->rFailedCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryRecvError(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rFCSErrorCount.QuadPart;
			/* @FIXME, RX_ERROR_DROP_COUNT/RX_FIFO_FULL_DROP_COUNT is not calculated */
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = prEventStatistics->rFCSErrorCount.QuadPart;
			/* @FIXME, RX_ERROR_DROP_COUNT/RX_FIFO_FULL_DROP_COUNT is not calculated */
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryRecvNoBuffer(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = 0;	/* @FIXME? */
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = 0;	/* @FIXME? */
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryRecvCrcError(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rFCSErrorCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = prEventStatistics->rFCSErrorCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryRecvErrorAlignment(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) 0;	/* @FIXME */
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = 0;	/* @FIXME */
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryXmitOneCollision(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data =
			    (UINT_32) (prEventStatistics->rMultipleRetryCount.QuadPart -
				       prEventStatistics->rRetryCount.QuadPart);
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data =
			    (UINT_64) (prEventStatistics->rMultipleRetryCount.QuadPart -
				       prEventStatistics->rRetryCount.QuadPart);
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryXmitMoreCollisions(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rMultipleRetryCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = (UINT_64) prEventStatistics->rMultipleRetryCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

VOID nicCmdEventQueryXmitMaxCollisions(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_EVENT_STATISTICS prEventStatistics;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;
	PUINT_32 pu4Data;
	PUINT_64 pu8Data;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventStatistics = (P_EVENT_STATISTICS) pucEventBuf;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		if (prCmdInfo->u4InformationBufferLength == sizeof(UINT_32)) {
			u4QueryInfoLen = sizeof(UINT_32);

			pu4Data = (PUINT_32) prCmdInfo->pvInformationBuffer;
			*pu4Data = (UINT_32) prEventStatistics->rFailedCount.QuadPart;
		} else {
			u4QueryInfoLen = sizeof(UINT_64);

			pu8Data = (PUINT_64) prCmdInfo->pvInformationBuffer;
			*pu8Data = (UINT_64) prEventStatistics->rFailedCount.QuadPart;
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when command by OID/ioctl has been timeout
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
*
* @return TRUE
*         FALSE
*/
/*----------------------------------------------------------------------------*/
VOID nicOidCmdTimeoutCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo)
{
	ASSERT(prAdapter);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_FAILURE);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is a generic command timeout handler
*
* @param pfnOidHandler      Pointer to the OID handler
*
* @return none
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdTimeoutCommon(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo)
{
	ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when command for entering RF test has
*        failed sending due to timeout (highly possibly by firmware crash)
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicOidCmdEnterRFTestTimeout(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo)
{
	ASSERT(prAdapter);

	/* 1. Remove pending TX frames */
	nicTxRelease(prAdapter, TRUE);

	/* 1.1 clear pending Security / Management Frames */
	kalClearSecurityFrames(prAdapter->prGlueInfo);
	kalClearMgmtFrames(prAdapter->prGlueInfo);

	/* 1.2 clear pending TX packet queued in glue layer */
	kalFlushPendingTxPackets(prAdapter->prGlueInfo);

	/* 2. indicate for OID failure */
	kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_FAILURE);
}

#if CFG_SUPPORT_QA_TOOL
/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when received dump memory event packet.
*        transfer the memory data to the IQ format data and write into file
*
* @param prIQAry     Pointer to the array store I or Q data.
*		 prDataLen   The return data length - bytes
*        u4IQ        0: get I data
*				 1 : get Q data
*
* @return -1: open file error
*
*/
/*----------------------------------------------------------------------------*/
INT_32 GetIQData(INT_32 **prIQAry, UINT_32 *prDataLen, UINT_32 u4IQ, UINT_32 u4GetWf1)
{
	UINT_8 aucPath[50];	/* the path for iq data dump out */
	UINT_8 aucData[50];	/* iq data in string format */
	UINT_32 i = 0, j = 0, count = 0;
	INT_32 ret = -1;
	INT_32 rv;
	struct file *file = NULL;

	*prIQAry = g_au4IQData;

	/* sprintf(aucPath, "/pattern.txt");             // CSD's Pattern */
	sprintf(aucPath, "/dump_out_%05ld_WF%d.txt", (g_u2DumpIndex - 1), u4GetWf1);

	file = kalFileOpen(aucPath, O_RDONLY, 0);

	if ((file != NULL) && !IS_ERR(file)) {
		/* read 1K data per time */
		for (i = 0; i < RTN_IQ_DATA_LEN / sizeof(UINT_32);
		     i++, g_au4Offset[u4GetWf1][u4IQ] += IQ_FILE_LINE_OFFSET) {
			if (kalFileRead(file, g_au4Offset[u4GetWf1][u4IQ], aucData, IQ_FILE_IQ_STR_LEN) == 0)
				break;

			count = 0;

			for (j = 0; j < 8; j++) {
				if (aucData[j] != ' ')
					aucData[count++] = aucData[j];
			}

		    aucData[count] = '\0';

			rv = kstrtoint(aucData, 0, &g_au4IQData[i]);	/* transfer data format (string to int) */
		}
		*prDataLen = i * sizeof(UINT_32);
		kalFileClose(file);
		ret = 0;
	}

	DBGLOG(INIT, INFO, "MT6632 : QA_AGENT GetIQData prDataLen = %d\n", *prDataLen);
	DBGLOG(INIT, INFO, "MT6632 : QA_AGENT GetIQData i = %d\n", i);

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when received dump memory event packet.
*        transfer the memory data to the IQ format data and write into file
*
* @param prEventDumpMem     Pointer to the event dump memory structure.
*
* @return 0: SUCCESS, -1: FAIL
*
*/
/*----------------------------------------------------------------------------*/

UINT_32 TsfRawData2IqFmt(P_EVENT_DUMP_MEM_T prEventDumpMem)
{
	static UINT_8 aucPathWF0[40];	/* the path for iq data dump out */
	static UINT_8 aucPathWF1[40];	/* the path for iq data dump out */
	static UINT_8 aucPathRAWWF0[40];	/* the path for iq data dump out */
	static UINT_8 aucPathRAWWF1[40];	/* the path for iq data dump out */
	PUINT_8 pucDataWF0;	/* the data write into file */
	PUINT_8 pucDataWF1;	/* the data write into file */
	PUINT_8 pucDataRAWWF0;	/* the data write into file */
	PUINT_8 pucDataRAWWF1;	/* the data write into file */
	UINT_32 u4SrcOffset;	/* record the buffer offset */
	UINT_32 u4FmtLen = 0;	/* bus format length */
	UINT_32 u4CpyLen = 0;
	UINT_32 u4RemainByte;
	BOOLEAN fgAppend;
	INT_32 u4Iqc160WF0Q0, u4Iqc160WF1I1;

	static UINT_8 ucDstOffset;	/* for alignment. bcs we send 2KB data per packet,*/
								/*the data will not align in 12 bytes case. */
	static UINT_32 u4CurTimeTick;

	static ICAP_BUS_FMT icapBusData;
	UINT_32 *ptr;

	pucDataWF0 = kmalloc(150, GFP_KERNEL);
	pucDataWF1 = kmalloc(150, GFP_KERNEL);
	pucDataRAWWF0 = kmalloc(150, GFP_KERNEL);
	pucDataRAWWF1 = kmalloc(150, GFP_KERNEL);

	fgAppend = TRUE;
	if (prEventDumpMem->ucFragNum == 1) {

		u4CurTimeTick = kalGetTimeTick();
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current  system tick>_<memory address>_<memory length>.hex
		 */
#if defined(LINUX)

		/*if blbist mkdir undre /data/blbist, the dump files wouls put on it */
		sprintf(aucPathWF0, "/dump_out_%05ld_WF0.txt", g_u2DumpIndex);
		sprintf(aucPathWF1, "/dump_out_%05ld_WF1.txt", g_u2DumpIndex);
		if (kalCheckPath(aucPathWF0) == -1) {
			kalMemSet(aucPathWF0, 0x00, 256);
			sprintf(aucPathWF0, "/data/dump_out_%05ld_WF0.txt", g_u2DumpIndex);
		}
		if (kalCheckPath(aucPathWF1) == -1) {
			kalMemSet(aucPathWF1, 0x00, 256);
			sprintf(aucPathWF1, "/data/dump_out_%05ld_WF1.txt", g_u2DumpIndex);
		}
		sprintf(aucPathRAWWF0, "/dump_RAW_%05ld_WF0.txt", g_u2DumpIndex);
		sprintf(aucPathRAWWF1, "/dump_RAW_%05ld_WF1.txt", g_u2DumpIndex);
#else
		kal_sprintf_ddk(aucPathWF0, sizeof(aucPathWF0),
				u4CurTimeTick,
				prEventDumpMem->u4Address, prEventDumpMem->u4Length + prEventDumpMem->u4RemainLength);
		kal_sprintf_ddk(aucPathWF1, sizeof(aucPathWF1),
				u4CurTimeTick,
				prEventDumpMem->u4Address, prEventDumpMem->u4Length + prEventDumpMem->u4RemainLength);
#endif
		/* fgAppend = FALSE; */
	}

	ptr = (PUINT_32)(&prEventDumpMem->aucBuffer[0]);
	DBGLOG(INIT, INFO, ": ==> (prEventDumpMem = %08x %08x %08x)\n", *(ptr), *(ptr + 4), *(ptr + 8));
	DBGLOG(INIT, INFO, ": ==> (prEventDumpMem->eIcapContent = %x)\n", prEventDumpMem->eIcapContent);

	for (u4SrcOffset = 0, u4RemainByte = prEventDumpMem->u4Length; u4RemainByte > 0;) {
		u4FmtLen =
(prEventDumpMem->eIcapContent == ICAP_CONTENT_SPECTRUM) ? sizeof(SPECTRUM_BUS_FMT_T) : sizeof(ICAP_BUS_FMT);
		/* 4 bytes : 12 bytes */
		u4CpyLen = (u4RemainByte - u4FmtLen >= 0) ? u4FmtLen : u4RemainByte;

		memcpy(&icapBusData + ucDstOffset, &prEventDumpMem->aucBuffer[0] + u4SrcOffset, u4CpyLen);
#if 0
		if (prEventDumpMem->eIcapContent == ICAP_CONTENT_ADC) {
			sprintf(aucDataWF0, "%8d,%8d\n", icapBusData.rAdcBusData.u4Dcoc0I,
				icapBusData.rAdcBusData.u4Dcoc0Q);
			sprintf(aucDataWF1, "%8d,%8d\n", icapBusData.rAdcBusData.u4Dcoc1I,
				icapBusData.rAdcBusData.u4Dcoc1Q);
		}
#endif
		if (prEventDumpMem->eIcapContent == ICAP_CONTENT_FIIQ ||
		    prEventDumpMem->eIcapContent == ICAP_CONTENT_FDIQ) {
			sprintf(pucDataWF0, "%8d,%8d\n", icapBusData.rIqcBusData.u4Iqc0I,
				icapBusData.rIqcBusData.u4Iqc0Q);
			sprintf(pucDataWF1, "%8d,%8d\n", icapBusData.rIqcBusData.u4Iqc1I,
				icapBusData.rIqcBusData.u4Iqc1Q);
		} else if (prEventDumpMem->eIcapContent - 1000 == ICAP_CONTENT_FIIQ
			   || prEventDumpMem->eIcapContent - 1000 == ICAP_CONTENT_FDIQ) {
			u4Iqc160WF0Q0 =
			    icapBusData.rIqc160BusData.u4Iqc0Q0P1 | (icapBusData.rIqc160BusData.u4Iqc0Q0P2 << 8);
			u4Iqc160WF1I1 =
			    icapBusData.rIqc160BusData.u4Iqc1I1P1 | (icapBusData.rIqc160BusData.u4Iqc1I1P2 << 4);

			sprintf(pucDataWF0, "%8d,%8d\n%8d,%8d\n", icapBusData.rIqc160BusData.u4Iqc0I0, u4Iqc160WF0Q0,
				icapBusData.rIqc160BusData.u4Iqc0I1, icapBusData.rIqc160BusData.u4Iqc0Q1);

			sprintf(pucDataWF1, "%8d,%8d\n%8d,%8d\n", icapBusData.rIqc160BusData.u4Iqc1I0,
				icapBusData.rIqc160BusData.u4Iqc1Q0, u4Iqc160WF1I1,
				icapBusData.rIqc160BusData.u4Iqc1Q1);

		} else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_SPECTRUM) {
			sprintf(pucDataWF0, "%8d,%8d\n", icapBusData.rSpectrumBusData.u4DcocI,
				icapBusData.rSpectrumBusData.u4DcocQ);
		} else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_ADC) {
			sprintf(pucDataWF0, "%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0, icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1, icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2, icapBusData.rPackedAdcBusData.u4AdcQ0T2,
				icapBusData.rPackedAdcBusData.u4AdcI0T3, icapBusData.rPackedAdcBusData.u4AdcQ0T3,
				icapBusData.rPackedAdcBusData.u4AdcI0T4, icapBusData.rPackedAdcBusData.u4AdcQ0T4,
				icapBusData.rPackedAdcBusData.u4AdcI0T5, icapBusData.rPackedAdcBusData.u4AdcQ0T5);

			sprintf(pucDataWF1, "%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0, icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1, icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2, icapBusData.rPackedAdcBusData.u4AdcQ1T2,
				icapBusData.rPackedAdcBusData.u4AdcI1T3, icapBusData.rPackedAdcBusData.u4AdcQ1T3,
				icapBusData.rPackedAdcBusData.u4AdcI1T4, icapBusData.rPackedAdcBusData.u4AdcQ1T4,
				icapBusData.rPackedAdcBusData.u4AdcI1T5, icapBusData.rPackedAdcBusData.u4AdcQ1T5);
		} else if (prEventDumpMem->eIcapContent - 2000 == ICAP_CONTENT_ADC) {
			sprintf(pucDataWF0, "%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI0T0, icapBusData.rPackedAdcBusData.u4AdcQ0T0,
				icapBusData.rPackedAdcBusData.u4AdcI0T1, icapBusData.rPackedAdcBusData.u4AdcQ0T1,
				icapBusData.rPackedAdcBusData.u4AdcI0T2, icapBusData.rPackedAdcBusData.u4AdcQ0T2);

			sprintf(pucDataWF1, "%8d,%8d\n%8d,%8d\n%8d,%8d\n",
				icapBusData.rPackedAdcBusData.u4AdcI1T0, icapBusData.rPackedAdcBusData.u4AdcQ1T0,
				icapBusData.rPackedAdcBusData.u4AdcI1T1, icapBusData.rPackedAdcBusData.u4AdcQ1T1,
				icapBusData.rPackedAdcBusData.u4AdcI1T2, icapBusData.rPackedAdcBusData.u4AdcQ1T2);
		} else if (prEventDumpMem->eIcapContent == ICAP_CONTENT_TOAE) {
			/* actually, this is DCOC. we take TOAE as DCOC */
			sprintf(pucDataWF0, "%8d,%8d\n", icapBusData.rAdcBusData.u4Dcoc0I,
				icapBusData.rAdcBusData.u4Dcoc0Q);
			sprintf(pucDataWF1, "%8d,%8d\n", icapBusData.rAdcBusData.u4Dcoc1I,
				icapBusData.rAdcBusData.u4Dcoc1Q);
		}
		if (u4CpyLen == u4FmtLen) {	/* the data format is complete */
			kalWriteToFile(aucPathWF0, fgAppend, pucDataWF0, strlen(pucDataWF0));
			kalWriteToFile(aucPathWF1, fgAppend, pucDataWF1, strlen(pucDataWF1));
		}
		ptr = (PUINT_32)(&prEventDumpMem->aucBuffer[0] + u4SrcOffset);
		sprintf(pucDataRAWWF0, "%08x%08x%08x\n", *(ptr + 2), *(ptr + 1), *ptr);
		kalWriteToFile(aucPathRAWWF0, fgAppend, pucDataRAWWF0, strlen(pucDataRAWWF0));
		kalWriteToFile(aucPathRAWWF1, fgAppend, pucDataRAWWF1, strlen(pucDataRAWWF1));

		u4RemainByte -= u4CpyLen;
		u4SrcOffset += u4CpyLen;	/* shift offset */
		ucDstOffset = 0;	/* only use ucDstOffset at first packet for align 2KB */
	}
	/* if this is a last packet, we can't transfer the remain data.
	 *  bcs we can't guarantee the data is complete align data format
	 */
	if (u4CpyLen != u4FmtLen) {	/* the data format is complete */
		ucDstOffset = u4CpyLen;	/* not align 2KB, keep the data and next packet data will append it */
	}

	kfree(pucDataWF0);
	kfree(pucDataWF1);
	kfree(pucDataRAWWF0);
	kfree(pucDataRAWWF1);

	if (u4RemainByte < 0) {
		ASSERT(-1);
		return -1;
	}

	return 0;
}
#endif /* CFG_SUPPORT_QA_TOOL */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called to handle dump burst event
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo         Pointer to the command information
* @param pucEventBuf       Pointer to event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/

VOID nicEventQueryMemDump(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	P_EVENT_DUMP_MEM_T prEventDumpMem;
	static UINT_8 aucPath[256];
	static UINT_8 aucPath_done[300];
	static UINT_32 u4CurTimeTick;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	sprintf(aucPath, "/dump_%05ld.hex", g_u2DumpIndex);

	prEventDumpMem = (P_EVENT_DUMP_MEM_T) (pucEventBuf);

	if (kalCheckPath(aucPath) == -1) {
		kalMemSet(aucPath, 0x00, 256);
		sprintf(aucPath, "/data/dump_%05ld.hex", g_u2DumpIndex);
	}

	if (prEventDumpMem->ucFragNum == 1) {
		/* Store memory dump into sdcard,
		 * path /sdcard/dump_<current  system tick>_<memory address>_<memory length>.hex
		 */
		u4CurTimeTick = kalGetTimeTick();
#if defined(LINUX)

		/*if blbist mkdir undre /data/blbist, the dump files wouls put on it */
		sprintf(aucPath, "/dump_%05ld.hex", g_u2DumpIndex);
		if (kalCheckPath(aucPath) == -1) {
			kalMemSet(aucPath, 0x00, 256);
			sprintf(aucPath, "/data/dump_%05ld.hex", g_u2DumpIndex);
		}
#else
		kal_sprintf_ddk(aucPath, sizeof(aucPath),
				u4CurTimeTick,
				prEventDumpMem->u4Address, prEventDumpMem->u4Length + prEventDumpMem->u4RemainLength);
#endif
		kalWriteToFile(aucPath, FALSE, &prEventDumpMem->aucBuffer[0], prEventDumpMem->u4Length);
	} else {
		/* Append current memory dump to the hex file */
		kalWriteToFile(aucPath, TRUE, &prEventDumpMem->aucBuffer[0], prEventDumpMem->u4Length);
	}
#if CFG_SUPPORT_QA_TOOL
	TsfRawData2IqFmt(prEventDumpMem);
#endif /* CFG_SUPPORT_QA_TOOL */
	DBGLOG(INIT, INFO,
	       ": ==> (u4RemainLength = %x, u4Address=%x )\n", prEventDumpMem->u4RemainLength,
	       prEventDumpMem->u4Address);

	if (prEventDumpMem->u4RemainLength == 0 || prEventDumpMem->u4Address == 0xFFFFFFFF) {

		/* The request is finished or firmware response a error */
		/* Reply time tick to iwpriv */

		g_bIcapEnable = FALSE;
		g_bCaptureDone = TRUE;

		sprintf(aucPath_done, "/file_dump_done.txt");
		if (kalCheckPath(aucPath_done) == -1) {
			kalMemSet(aucPath_done, 0x00, 256);
			sprintf(aucPath_done, "/data/file_dump_done.txt");
		}
		DBGLOG(INIT, INFO, ": ==> gen done_file\n");
		kalWriteToFile(aucPath_done, FALSE, aucPath_done, sizeof(aucPath_done));
#if CFG_SUPPORT_QA_TOOL
		g_au4Offset[0][0] = 0;
		g_au4Offset[0][1] = 9;
		g_au4Offset[1][0] = 0;
		g_au4Offset[1][1] = 9;
#endif /* CFG_SUPPORT_QA_TOOL */

		g_u2DumpIndex++;

	}

}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when command for memory dump has
*        replied a event.
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo         Pointer to the command information
* @param pucEventBuf       Pointer to event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventQueryMemDump(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_GLUE_INFO_T prGlueInfo;
	P_EVENT_DUMP_MEM_T prEventDumpMem;
	static UINT_8 aucPath[256];
/*	static UINT_8 aucPath_done[300]; */
	static UINT_32 u4CurTimeTick;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (1) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventDumpMem = (P_EVENT_DUMP_MEM_T) (pucEventBuf);

		u4QueryInfoLen = sizeof(P_PARAM_CUSTOM_MEM_DUMP_STRUCT_T);

#if 0
		do {
			UINT_32 i = 0;

			DBGLOG(REQ, TRACE, "Rx dump address 0x%X, Length %d, FragNum %d, remain %d\n",
			       prEventDumpMem->u4Address,
			       prEventDumpMem->u4Length, prEventDumpMem->ucFragNum, prEventDumpMem->u4RemainLength);
#if 0
			for (i = 0; i < prEventDumpMem->u4Length; i++) {
				DBGLOG(REQ, TRACE, "%02X ", prEventDumpMem->aucBuffer[i]);
				if (i % 32 == 31)
					DBGLOG(REQ, TRACE, "\n");
			}
#endif
		} while (FALSE);
#endif

		if (prEventDumpMem->ucFragNum == 1) {
			/* Store memory dump into sdcard,
			 * path /sdcard/dump_<current  system tick>_<memory address>_<memory length>.hex
			 */
			u4CurTimeTick = kalGetTimeTick();
#if defined(LINUX)
#if 0
			sprintf(aucPath, "/sdcard/dump_%ld_0x%08lX_%ld.hex",
				u4CurTimeTick,
				prEventDumpMem->u4Address, prEventDumpMem->u4Length + prEventDumpMem->u4RemainLength);
#else

			/*if blbist mkdir undre /data/blbist, the dump files wouls put on it */
			sprintf(aucPath, "/dump_%05ld.hex", g_u2DumpIndex);
			if (kalCheckPath(aucPath) == -1) {
				kalMemSet(aucPath, 0x00, 256);
				sprintf(aucPath, "/dump_%05ld.hex", g_u2DumpIndex);
			}
#endif
#else
			kal_sprintf_ddk(aucPath, sizeof(aucPath),
					u4CurTimeTick,
					prEventDumpMem->u4Address,
					prEventDumpMem->u4Length + prEventDumpMem->u4RemainLength);
			/* strcpy(aucPath, "dump.hex"); */
#endif
			kalWriteToFile(aucPath, FALSE, &prEventDumpMem->aucBuffer[0], prEventDumpMem->u4Length);
		} else {
			/* Append current memory dump to the hex file */
			kalWriteToFile(aucPath, TRUE, &prEventDumpMem->aucBuffer[0], prEventDumpMem->u4Length);
		}
#if CFG_SUPPORT_QA_TOOL
		TsfRawData2IqFmt(prEventDumpMem);
#endif /* CFG_SUPPORT_QA_TOOL */
		if (prEventDumpMem->u4RemainLength == 0 || prEventDumpMem->u4Address == 0xFFFFFFFF) {
			/* The request is finished or firmware response a error */
			/* Reply time tick to iwpriv */
			if (prCmdInfo->fgIsOid) {

				/* the oid would be complete only in oid-trigger  mode,
				 * that is no need to if the event-trigger
				 */
				if (g_bIcapEnable == FALSE) {
					*((PUINT_32) prCmdInfo->pvInformationBuffer) = u4CurTimeTick;
					kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery,
						       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
				}
			}
			g_bIcapEnable = FALSE;
			g_bCaptureDone = TRUE;
#if defined(LINUX)
#if 0
			sprintf(aucPath_done, "/data/blbist/file_dump_done.txt");
			if (kalCheckPath(aucPath_done) == -1) {
				kalMemSet(aucPath_done, 0x00, 256);
				sprintf(aucPath_done, "/data/file_dump_done.txt");
			}
			DBGLOG(INIT, INFO, ": ==> gen done_file\n");
			kalWriteToFile(aucPath_done, FALSE, aucPath_done, sizeof(aucPath_done));
#endif
			g_u2DumpIndex++;

#else
			kal_sprintf_done_ddk(aucPath_done, sizeof(aucPath_done));
			kalWriteToFile(aucPath_done, FALSE, aucPath_done, sizeof(aucPath_done));
#endif
		} else {
#if defined(LINUX)

#else /* 2013/05/26 fw would try to send the buffer successfully */
			/* The memory dump request is not finished, Send next command */
			wlanSendMemDumpCmd(prAdapter,
					   prCmdInfo->pvInformationBuffer, prCmdInfo->u4InformationBufferLength);
#endif
		}
	}

	return;

}

#if CFG_SUPPORT_BATCH_SCAN
/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when event for SUPPORT_BATCH_SCAN
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
* @param pucEventBuf        Pointer to the event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventBatchScanResult(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_EVENT_BATCH_RESULT_T prEventBatchResult;
	P_GLUE_INFO_T prGlueInfo;

	DBGLOG(SCN, TRACE, "nicCmdEventBatchScanResult");

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEventBatchResult = (P_EVENT_BATCH_RESULT_T) pucEventBuf;

		u4QueryInfoLen = sizeof(EVENT_BATCH_RESULT_T);
		kalMemCopy(prCmdInfo->pvInformationBuffer, prEventBatchResult, sizeof(EVENT_BATCH_RESULT_T));

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

VOID nicEventHifCtrl(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if defined(_HIF_USB)
	P_EVENT_HIF_CTRL_T prEventHifCtrl;

	prEventHifCtrl = (P_EVENT_HIF_CTRL_T) ((PUINT_8) prEvent + EVENT_HDR_SIZE);

	DBGLOG(HAL, INFO, "%s: EVENT_ID_HIF_CTRL\n", __func__);
	DBGLOG(HAL, INFO, "prEventHifCtrl->ucHifType = %hhu\n", prEventHifCtrl->ucHifType);
	DBGLOG(HAL, INFO, "prEventHifCtrl->ucHifTxTrafficStatus, prEventHifCtrl->ucHifRxTrafficStatus = %hhu, %hhu\n",
	       prEventHifCtrl->ucHifTxTrafficStatus, prEventHifCtrl->ucHifRxTrafficStatus);

	if (prEventHifCtrl->ucHifTxTrafficStatus == ENUM_HIF_TRAFFIC_IDLE &&
	    prEventHifCtrl->ucHifRxTrafficStatus == ENUM_HIF_TRAFFIC_IDLE) {	/* success */
		halUSBPreSuspendDone(prAdapter, NULL, prEvent->aucBuffer);
	} else if (prEventHifCtrl->ucHifTxTrafficStatus == ENUM_HIF_TRAFFIC_BUSY ||
		   prEventHifCtrl->ucHifRxTrafficStatus == ENUM_HIF_TRAFFIC_BUSY) {	/* busy */
		halUSBPreSuspendTimeout(prAdapter, NULL);
	} else if (prEventHifCtrl->ucHifTxTrafficStatus == ENUM_HIF_TRAFFIC_INVALID ||
		   prEventHifCtrl->ucHifRxTrafficStatus == ENUM_HIF_TRAFFIC_INVALID) {	/* invalid */
		halUSBPreSuspendTimeout(prAdapter, NULL);
	}
#endif
}

#if CFG_SUPPORT_BUILD_DATE_CODE
/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when event for build date code information
*        has been retrieved
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
* @param pucEventBuf        Pointer to the event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventBuildDateCode(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_EVENT_BUILD_DATE_CODE prEvent;
	P_GLUE_INFO_T prGlueInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	/* 4 <2> Update information of OID */
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEvent = (P_EVENT_BUILD_DATE_CODE) pucEventBuf;

		u4QueryInfoLen = sizeof(UINT_8) * 16;
		kalMemCopy(prCmdInfo->pvInformationBuffer, prEvent->aucDateCode, sizeof(UINT_8) * 16);

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when event for query STA link status
*        has been retrieved
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
* @param pucEventBuf        Pointer to the event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventQueryStaStatistics(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_EVENT_STA_STATISTICS_T prEvent;
	P_GLUE_INFO_T prGlueInfo;
	P_PARAM_GET_STA_STATISTICS prStaStatistics;
	ENUM_WMM_ACI_T eAci;
	P_STA_RECORD_T prStaRec;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	ASSERT(prCmdInfo->pvInformationBuffer);

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEvent = (P_EVENT_STA_STATISTICS_T) pucEventBuf;
		prStaStatistics = (P_PARAM_GET_STA_STATISTICS) prCmdInfo->pvInformationBuffer;

		u4QueryInfoLen = sizeof(PARAM_GET_STA_STA_STATISTICS);

		/* Statistics from FW is valid */
		if (prEvent->u4Flags & BIT(0)) {
			prStaStatistics->ucPer = prEvent->ucPer;
			prStaStatistics->ucRcpi = prEvent->ucRcpi;
			prStaStatistics->u4PhyMode = prEvent->u4PhyMode;
			prStaStatistics->u2LinkSpeed = prEvent->u2LinkSpeed;

			prStaStatistics->u4TxFailCount = prEvent->u4TxFailCount;
			prStaStatistics->u4TxLifeTimeoutCount = prEvent->u4TxLifeTimeoutCount;
			prStaStatistics->u4TransmitCount = prEvent->u4TransmitCount;
			prStaStatistics->u4TransmitFailCount = prEvent->u4TransmitFailCount;
			prStaStatistics->u4Rate1TxCnt = prEvent->u4Rate1TxCnt;
			prStaStatistics->u4Rate1FailCnt = prEvent->u4Rate1FailCnt;

			prStaStatistics->ucTemperature = prEvent->ucTemperature;
			prStaStatistics->ucSkipAr = prEvent->ucSkipAr;
			prStaStatistics->ucArTableIdx = prEvent->ucArTableIdx;
			prStaStatistics->ucRateEntryIdx = prEvent->ucRateEntryIdx;
			prStaStatistics->ucRateEntryIdxPrev = prEvent->ucRateEntryIdxPrev;
			prStaStatistics->ucTxSgiDetectPassCnt = prEvent->ucTxSgiDetectPassCnt;
			prStaStatistics->ucAvePer = prEvent->ucAvePer;
			kalMemCopy(prStaStatistics->aucArRatePer, prEvent->aucArRatePer,
				sizeof(prEvent->aucArRatePer));
			kalMemCopy(prStaStatistics->aucRateEntryIndex, prEvent->aucRateEntryIndex,
				sizeof(prEvent->aucRateEntryIndex));
			prStaStatistics->ucArStateCurr = prEvent->ucArStateCurr;
			prStaStatistics->ucArStatePrev = prEvent->ucArStatePrev;
			prStaStatistics->ucArActionType = prEvent->ucArActionType;
			prStaStatistics->ucHighestRateCnt = prEvent->ucHighestRateCnt;
			prStaStatistics->ucLowestRateCnt = prEvent->ucLowestRateCnt;
			prStaStatistics->u2TrainUp = prEvent->u2TrainUp;
			prStaStatistics->u2TrainDown = prEvent->u2TrainDown;
			kalMemCopy(&prStaStatistics->rTxVector, &prEvent->rTxVector,
				sizeof(prEvent->rTxVector));
			kalMemCopy(&prStaStatistics->rMibInfo, &prEvent->rMibInfo,
				sizeof(prEvent->rMibInfo));

			prStaRec = cnmGetStaRecByIndex(prAdapter, prEvent->ucStaRecIdx);

			if (prStaRec) {
				/*link layer statistics */
				for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
					prStaStatistics->arLinkStatistics[eAci].u4TxFailMsdu =
					    prEvent->arLinkStatistics[eAci].u4TxFailMsdu;
					prStaStatistics->arLinkStatistics[eAci].u4TxRetryMsdu =
					    prEvent->arLinkStatistics[eAci].u4TxRetryMsdu;

					/*for dump bss statistics */
					prStaRec->arLinkStatistics[eAci].u4TxFailMsdu =
					    prEvent->arLinkStatistics[eAci].u4TxFailMsdu;
					prStaRec->arLinkStatistics[eAci].u4TxRetryMsdu =
					    prEvent->arLinkStatistics[eAci].u4TxRetryMsdu;
				}
			}
			if (prEvent->u4TxCount) {
				UINT_32 u4TxDoneAirTimeMs = USEC_TO_MSEC(prEvent->u4TxDoneAirTime * 32);

				prStaStatistics->u4TxAverageAirTime = (u4TxDoneAirTimeMs / prEvent->u4TxCount);
			} else {
				prStaStatistics->u4TxAverageAirTime = 0;
			}
		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

#if CFG_AUTO_CHANNEL_SEL_SUPPORT

/* 4  Auto Channel Selection */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is called when event for query STA link status
*        has been retrieved
*
* @param prAdapter          Pointer to the Adapter structure.
* @param prCmdInfo          Pointer to the command information
* @param pucEventBuf        Pointer to the event buffer
*
* @return none
*
*/
/*----------------------------------------------------------------------------*/
VOID nicCmdEventQueryChannelLoad(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_EVENT_CHN_LOAD_T prEvent;
	P_GLUE_INFO_T prGlueInfo;
	P_PARAM_GET_CHN_LOAD prChnLoad;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	ASSERT(prCmdInfo->pvInformationBuffer);

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEvent = (P_EVENT_CHN_LOAD_T) pucEventBuf;	/* 4 The firmware responsed data */
		/* 4 Fill the firmware data in and send it back to host */
		prChnLoad = (P_PARAM_GET_CHN_LOAD) prCmdInfo->pvInformationBuffer;

		u4QueryInfoLen = sizeof(PARAM_GET_CHN_LOAD);

		/* Statistics from FW is valid */
		if (prEvent->u4Flags & BIT(0)) {
			prChnLoad->rEachChnLoad[0].ucChannel = prEvent->ucChannel;
			prChnLoad->rEachChnLoad[0].u2ChannelLoad = prEvent->u2ChannelLoad;
			DBGLOG(P2P, INFO, "CHN[%d]=%d\n", prEvent->ucChannel, prEvent->u2ChannelLoad);

		}

		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}

VOID nicCmdEventQueryLTESafeChn(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	UINT_32 u4QueryInfoLen;
	P_EVENT_LTE_MODE_T prEvent;
	P_GLUE_INFO_T prGlueInfo;
	P_PARAM_GET_CHN_LOAD prLteSafeChnInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);
	ASSERT(prCmdInfo->pvInformationBuffer);
	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prEvent = (P_EVENT_LTE_MODE_T) pucEventBuf;	/* 4 The firmware responsed data */

		prLteSafeChnInfo = (P_PARAM_GET_CHN_LOAD) prCmdInfo->pvInformationBuffer;

		u4QueryInfoLen = sizeof(PARAM_GET_CHN_LOAD);

		/* Statistics from FW is valid */
		if (prEvent->u4Flags & BIT(0)) {
			/* prLteSafeChnInfo->rLteSafeChnList.ucChannelHigh= prEvent->rLteSafeChn.ucChannelHigh; */
			/* prLteSafeChnInfo->rLteSafeChnList.ucChannelLow= prEvent->rLteSafeChn.ucChannelLow; */
			prLteSafeChnInfo->rLteSafeChnList.u4SafeChannelBitmask[0] =
			    prEvent->rLteSafeChn.u4SafeChannelBitmask[0];
			if (prEvent->ucVersion != 0) {
				prLteSafeChnInfo->rLteSafeChnList.u4SafeChannelBitmask[1] =
				    prEvent->rLteSafeChn.u4SafeChannelBitmask[1];
				prLteSafeChnInfo->rLteSafeChnList.u4SafeChannelBitmask[2] =
				    prEvent->rLteSafeChn.u4SafeChannelBitmask[2];
				prLteSafeChnInfo->rLteSafeChnList.u4SafeChannelBitmask[3] =
				    prEvent->rLteSafeChn.u4SafeChannelBitmask[3];
			}
			DBGLOG(P2P, INFO,
			       "[Query-info Auto Channel]LTE safe channels 0x%08x\n",
			       prLteSafeChnInfo->rLteSafeChnList.u4SafeChannelBitmask[0]);
		}
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

}
#endif

VOID nicEventRddPulseDump(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	UINT_16 u2Idx, u2PulseCnt;
	P_EVENT_WIFI_RDD_TEST_T prRddPulseEvent;

	ASSERT(prAdapter);
	ASSERT(pucEventBuf);

	prRddPulseEvent = (P_EVENT_WIFI_RDD_TEST_T) (pucEventBuf);

	u2PulseCnt = (prRddPulseEvent->u4FuncLength - RDD_EVENT_HDR_SIZE) / RDD_ONEPLUSE_SIZE;

	TOOL_PRINTLOG(INIT, INFO, "[RDD]0x%08x %08d[RDD%d]\n", prRddPulseEvent->u4Prefix
			, prRddPulseEvent->u4Count, prRddPulseEvent->ucRddIdx);

	for (u2Idx = 0; u2Idx < u2PulseCnt; u2Idx++) {
		TOOL_PRINTLOG(INIT, INFO, "[RDD]0x%02x%02x%02x%02x %02x%02x%02x%02x[RDD%d]\n"
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET3]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET2]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET1]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET0]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET7]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET6]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET5]
			, prRddPulseEvent->aucBuffer[RDD_ONEPLUSE_SIZE*u2Idx+RDD_PULSE_OFFSET4]
			, prRddPulseEvent->ucRddIdx
			);
	}

	TOOL_PRINTLOG(INIT, INFO, "[RDD]0x%08x %08x[RDD%d]\n", prRddPulseEvent->u4SubBandRssi0
			, prRddPulseEvent->u4SubBandRssi1, prRddPulseEvent->ucRddIdx);

}


#if CFG_SUPPORT_MSP
VOID nicCmdEventQueryWlanInfo(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_PARAM_HW_WLAN_INFO_T prWlanInfo;
	P_EVENT_WLAN_INFO prEventWlanInfo;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventWlanInfo = (P_EVENT_WLAN_INFO) pucEventBuf;

	DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryWlanInfo\n");

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		u4QueryInfoLen = sizeof(PARAM_HW_WLAN_INFO_T);
		prWlanInfo = (P_PARAM_HW_WLAN_INFO_T) prCmdInfo->pvInformationBuffer;

		/* prWlanInfo->u4Length = sizeof(PARAM_HW_WLAN_INFO_T); */
		if (prEventWlanInfo && prWlanInfo) {
			kalMemCopy(&prWlanInfo->rWtblTxConfig,
				   &prEventWlanInfo->rWtblTxConfig,
				   sizeof(PARAM_TX_CONFIG_T));
			kalMemCopy(&prWlanInfo->rWtblSecConfig,
				   &prEventWlanInfo->rWtblSecConfig,
				   sizeof(PARAM_SEC_CONFIG_T));
			kalMemCopy(&prWlanInfo->rWtblKeyConfig,
				   &prEventWlanInfo->rWtblKeyConfig,
				   sizeof(PARAM_KEY_CONFIG_T));
			kalMemCopy(&prWlanInfo->rWtblRateInfo,
				   &prEventWlanInfo->rWtblRateInfo,
				   sizeof(PARAM_PEER_RATE_INFO_T));
			kalMemCopy(&prWlanInfo->rWtblBaConfig,
				   &prEventWlanInfo->rWtblBaConfig,
				   sizeof(PARAM_PEER_BA_CONFIG_T));
			kalMemCopy(&prWlanInfo->rWtblPeerCap,
				   &prEventWlanInfo->rWtblPeerCap,
				   sizeof(PARAM_PEER_CAP_T));
			kalMemCopy(&prWlanInfo->rWtblRxCounter,
				   &prEventWlanInfo->rWtblRxCounter,
				   sizeof(PARAM_PEER_RX_COUNTER_ALL_T));
			kalMemCopy(&prWlanInfo->rWtblTxCounter,
					&prEventWlanInfo->rWtblTxCounter,
					sizeof(PARAM_PEER_TX_COUNTER_ALL_T));
		}
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}


VOID nicCmdEventQueryMibInfo(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo, IN PUINT_8 pucEventBuf)
{
	P_PARAM_HW_MIB_INFO_T prMibInfo;
	P_EVENT_MIB_INFO prEventMibInfo;
	P_GLUE_INFO_T prGlueInfo;
	UINT_32 u4QueryInfoLen;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prEventMibInfo = (P_EVENT_MIB_INFO) pucEventBuf;

	DBGLOG(RSN, INFO, "MT6632 : nicCmdEventQueryMibInfo\n");

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		u4QueryInfoLen = sizeof(PARAM_HW_MIB_INFO_T);
		prMibInfo = (P_PARAM_HW_MIB_INFO_T) prCmdInfo->pvInformationBuffer;
		if (prEventMibInfo && prMibInfo) {
			kalMemCopy(&prMibInfo->rHwMibCnt,
				   &prEventMibInfo->rHwMibCnt,
				   sizeof(HW_MIB_COUNTER_T));
			kalMemCopy(&prMibInfo->rHwMib2Cnt,
				   &prEventMibInfo->rHwMib2Cnt,
				   sizeof(HW_MIB2_COUNTER_T));
			kalMemCopy(&prMibInfo->rHwTxAmpduMts,
				   &prEventMibInfo->rHwTxAmpduMts,
				   sizeof(HW_TX_AMPDU_METRICS_T));
		}
		kalOidComplete(prGlueInfo, prCmdInfo->fgSetQuery, u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}
#endif

WLAN_STATUS nicCmdEventQueryNicCoexFeature(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	P_NIC_COEX_FEATURE_T prCoexFeature = (P_NIC_COEX_FEATURE_T)pucEventBuf;

	prAdapter->u4FddMode = prCoexFeature->u4FddMode;

	DBGLOG(INIT, INFO, "nicCmdEventQueryNicCoexFeature: u4FddMode = %x",
						prAdapter->u4FddMode);

	return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicCmdEventQueryNicEfuseAddr(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	P_NIC_EFUSE_ADDRESS_T prTxResource = (P_NIC_EFUSE_ADDRESS_T)pucEventBuf;

	prAdapter->u4EfuseStartAddress = prTxResource->u4EfuseStartAddress;
	prAdapter->u4EfuseEndAddress = prTxResource->u4EfuseEndAddress;

	DBGLOG(INIT, INFO, "nicCmdEventQueryNicEfuseAddr: u4EfuseStartAddress = %x",
						prAdapter->u4EfuseStartAddress);
	DBGLOG(INIT, INFO, "nicCmdEventQueryNicEfuseAddr: u4EfuseEndAddress = %x",
						prAdapter->u4EfuseEndAddress);

	return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicCmdEventQueryNicTxResource(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	P_NIC_TX_RESOURCE_T prTxResource = (P_NIC_TX_RESOURCE_T)pucEventBuf;

	prAdapter->fgIsNicTxReousrceValid = TRUE;
	prAdapter->nicTxReousrce.u4McuTotalResource = prTxResource->u4McuTotalResource;
	prAdapter->nicTxReousrce.u4McuResourceUnit = prTxResource->u4McuResourceUnit;
	prAdapter->nicTxReousrce.u4LmacTotalResource = prTxResource->u4LmacTotalResource;
	prAdapter->nicTxReousrce.u4LmacResourceUnit = prTxResource->u4LmacResourceUnit;

	DBGLOG(INIT, INFO, "nicCmdEventQueryNicTxResource: u4McuTotalResource = %x",
						prAdapter->nicTxReousrce.u4McuTotalResource);
	DBGLOG(INIT, INFO, "nicCmdEventQueryNicTxResource: u4McuResourceUnit = %x",
						prAdapter->nicTxReousrce.u4McuResourceUnit);
	DBGLOG(INIT, INFO, "nicCmdEventQueryNicTxResource: u4LmacTotalResource = %x",
						prAdapter->nicTxReousrce.u4LmacTotalResource);
	DBGLOG(INIT, INFO, "nicCmdEventQueryNicTxResource: u4LmacResourceUnit = %x",
						prAdapter->nicTxReousrce.u4LmacResourceUnit);

	return WLAN_STATUS_SUCCESS;
}

VOID nicCmdEventQueryNicCapabilityV2(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucEventBuf)
{
	P_EVENT_NIC_CAPABILITY_V2_T prEventNicV2 = (P_EVENT_NIC_CAPABILITY_V2_T)pucEventBuf;
	P_NIC_CAPABILITY_V2_ELEMENT prElement;
	UINT_32 tag_idx, table_idx, offset;

	offset = 0;

	/* process each element */
	for (tag_idx = 0; tag_idx < prEventNicV2->u2TotalElementNum; tag_idx++) {

		prElement = (P_NIC_CAPABILITY_V2_ELEMENT)(prEventNicV2->aucBuffer + offset);

		for (table_idx = 0;
			 table_idx < (sizeof(gNicCapabilityV2InfoTable)/sizeof(NIC_CAPABILITY_V2_REF_TABLE_T));
			 table_idx++) {

			/* find the corresponding tag's handler */
			if (gNicCapabilityV2InfoTable[table_idx].tag_type == prElement->tag_type) {
				gNicCapabilityV2InfoTable[table_idx].hdlr(prAdapter, prElement->aucbody);
				break;
			}
		}

		/* move to the next tag */
		offset += prElement->body_len + (UINT_16) OFFSET_OF(NIC_CAPABILITY_V2_ELEMENT, aucbody);
	}

}

VOID nicEventLinkQuality(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_CMD_INFO_T prCmdInfo;

#if CFG_ENABLE_WIFI_DIRECT && CFG_SUPPORT_P2P_RSSI_QUERY
	if (prEvent->u2PacketLen == EVENT_HDR_SIZE + sizeof(EVENT_LINK_QUALITY_EX)) {
		P_EVENT_LINK_QUALITY_EX prLqEx = (P_EVENT_LINK_QUALITY_EX) (prEvent->aucBuffer);

		if (prLqEx->ucIsLQ0Rdy)
			nicUpdateLinkQuality(prAdapter, 0, (P_EVENT_LINK_QUALITY) prLqEx);
		if (prLqEx->ucIsLQ1Rdy)
			nicUpdateLinkQuality(prAdapter, 1, (P_EVENT_LINK_QUALITY) prLqEx);
	} else {
		/* For old FW, P2P may invoke link quality query, and make driver flag becone TRUE. */
		DBGLOG(P2P, WARN, "Old FW version, not support P2P RSSI query.\n");

		/* Must not use NETWORK_TYPE_P2P_INDEX, cause the structure is mismatch. */
		nicUpdateLinkQuality(prAdapter, 0, (P_EVENT_LINK_QUALITY) (prEvent->aucBuffer));
	}
#else
	/*only support ais query */
	{
		UINT_8 ucBssIndex;
		P_BSS_INFO_T prBssInfo;

		for (ucBssIndex = 0; ucBssIndex < BSS_INFO_NUM; ucBssIndex++) {
			prBssInfo = prAdapter->aprBssInfo[ucBssIndex];

			if (prBssInfo->eNetworkType == NETWORK_TYPE_AIS && prBssInfo->fgIsInUse)
				break;
		}

		if (ucBssIndex >= BSS_INFO_NUM)
			ucBssIndex = 1;	/* No hit(bss1 for default ais network) */
		/* printk("=======> rssi with bss%d ,%d\n",ucBssIndex,
		 * ((P_EVENT_LINK_QUALITY_V2)(prEvent->aucBuffer))->rLq[ucBssIndex].cRssi);
		 */
		nicUpdateLinkQuality(prAdapter, ucBssIndex, (P_EVENT_LINK_QUALITY_V2) (prEvent->aucBuffer));
	}

#endif

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo, prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
#ifndef LINUX
	if (prAdapter->rWlanInfo.eRssiTriggerType == ENUM_RSSI_TRIGGER_GREATER &&
		prAdapter->rWlanInfo.rRssiTriggerValue >= (PARAM_RSSI) (prAdapter->rLinkQuality.cRssi)) {

		prAdapter->rWlanInfo.eRssiTriggerType = ENUM_RSSI_TRIGGER_TRIGGERED;

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
			(PVOID)&(prAdapter->rWlanInfo.rRssiTriggerValue), sizeof(PARAM_RSSI));
	} else if (prAdapter->rWlanInfo.eRssiTriggerType == ENUM_RSSI_TRIGGER_LESS &&
		prAdapter->rWlanInfo.rRssiTriggerValue <= (PARAM_RSSI) (prAdapter->rLinkQuality.cRssi)) {

		prAdapter->rWlanInfo.eRssiTriggerType = ENUM_RSSI_TRIGGER_TRIGGERED;

		kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
			(PVOID)&(prAdapter->rWlanInfo.rRssiTriggerValue), sizeof(PARAM_RSSI));
	}
#endif
}

VOID nicEventLayer0ExtMagic(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	UINT_32 u4QueryInfoLen;
	P_CMD_INFO_T prCmdInfo;
	P_EVENT_ACCESS_EFUSE prEventEfuseAccess;
	P_EVENT_EFUSE_FREE_BLOCK_T prEventGetFreeBlock;
	P_EVENT_GET_TX_POWER_T prEventGetTXPower;

	if ((prEvent->ucExtenEID) == EXT_EVENT_ID_CMD_RESULT) {

		u4QueryInfoLen = sizeof(PARAM_CUSTOM_EFUSE_BUFFER_MODE_T);

		prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

		if (prCmdInfo != NULL) {
			if ((prCmdInfo->fgIsOid) != 0) {
				kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
					u4QueryInfoLen, WLAN_STATUS_SUCCESS);
				/* return prCmdInfo */
				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			}
		}
	} else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_CMD_EFUSE_ACCESS) {
		u4QueryInfoLen = sizeof(PARAM_CUSTOM_ACCESS_EFUSE_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
		prEventEfuseAccess = (P_EVENT_ACCESS_EFUSE) (prEvent->aucBuffer);

		/* Efuse block size 16 */
		kalMemCopy(prAdapter->aucEepromVaule, prEventEfuseAccess->aucData, 16);

		if (prCmdInfo != NULL) {
			if ((prCmdInfo->fgIsOid) != 0) {
				kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
					u4QueryInfoLen, WLAN_STATUS_SUCCESS);
				/* return prCmdInfo */
				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

			}
		}
	} else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_EFUSE_FREE_BLOCK) {
		u4QueryInfoLen = sizeof(PARAM_CUSTOM_EFUSE_FREE_BLOCK_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
		prEventGetFreeBlock = (P_EVENT_EFUSE_FREE_BLOCK_T) (prEvent->aucBuffer);
		prAdapter->u4FreeBlockNum = prEventGetFreeBlock->u2FreeBlockNum;

		if (prCmdInfo != NULL) {
			if ((prCmdInfo->fgIsOid) != 0) {
				kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
					u4QueryInfoLen, WLAN_STATUS_SUCCESS);
				/* return prCmdInfo */
				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			}
		}
	} else if ((prEvent->ucExtenEID) == EXT_EVENT_ID_GET_TX_POWER) {
		u4QueryInfoLen = sizeof(PARAM_CUSTOM_GET_TX_POWER_T);
		prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);
		prEventGetTXPower = (P_EVENT_GET_TX_POWER_T) (prEvent->aucBuffer);

		prAdapter->u4GetTxPower = prEventGetTXPower->ucTx0TargetPower;

		if (prCmdInfo != NULL) {
			if ((prCmdInfo->fgIsOid) != 0) {
				kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery,
					u4QueryInfoLen, WLAN_STATUS_SUCCESS);
				/* return prCmdInfo */
				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

			}
		}
	}
}

VOID nicEventMicErrorInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_MIC_ERR_INFO prMicError;
	/* P_PARAM_AUTH_EVENT_T prAuthEvent; */
	P_STA_RECORD_T prStaRec;

	DBGLOG(RSN, EVENT, "EVENT_ID_MIC_ERR_INFO\n");

	prMicError = (P_EVENT_MIC_ERR_INFO) (prEvent->aucBuffer);
	prStaRec = cnmGetStaRecByAddress(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
					prAdapter->rWlanInfo.rCurrBssId.arMacAddress);
	ASSERT(prStaRec);

	if (prStaRec)
		rsnTkipHandleMICFailure(prAdapter, prStaRec, (BOOLEAN) prMicError->u4Flags);
	else
		DBGLOG(RSN, INFO, "No STA rec!!\n");
#if 0
	prAuthEvent = (P_PARAM_AUTH_EVENT_T) prAdapter->aucIndicationEventBuffer;

	/* Status type: Authentication Event */
	prAuthEvent->rStatus.eStatusType = ENUM_STATUS_TYPE_AUTHENTICATION;

	/* Authentication request */
	prAuthEvent->arRequest[0].u4Length = sizeof(PARAM_AUTH_REQUEST_T);
	kalMemCopy((PVOID) prAuthEvent->arRequest[0].arBssid,
			(PVOID) prAdapter->rWlanInfo.rCurrBssId.arMacAddress,	/* whsu:Todo? */
		   PARAM_MAC_ADDR_LEN);

	if (prMicError->u4Flags != 0)
		prAuthEvent->arRequest[0].u4Flags = PARAM_AUTH_REQUEST_GROUP_ERROR;
	else
		prAuthEvent->arRequest[0].u4Flags = PARAM_AUTH_REQUEST_PAIRWISE_ERROR;

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
				     WLAN_STATUS_MEDIA_SPECIFIC_INDICATION,
				     (PVOID) prAuthEvent,
				     sizeof(PARAM_STATUS_INDICATION_T) + sizeof(PARAM_AUTH_REQUEST_T));
#endif
}

VOID nicEventScanDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	scnEventScanDone(prAdapter, (P_EVENT_SCAN_DONE) (prEvent->aucBuffer), TRUE);
}

VOID nicEventNloDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	DBGLOG(INIT, INFO, "EVENT_ID_NLO_DONE\n");
	scnEventNloDone(prAdapter, (P_EVENT_NLO_DONE_T) (prEvent->aucBuffer));
#if CFG_SUPPORT_PNO
	prAdapter->prAisBssInfo->fgIsPNOEnable = FALSE;
	if (prAdapter->prAisBssInfo->fgIsNetRequestInActive && prAdapter->prAisBssInfo->fgIsPNOEnable) {
		UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
		DBGLOG(INIT, INFO, "INACTIVE  AIS from  ACTIVEto disable PNO\n");
		/* sync with firmware */
		nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	}
#endif
}

VOID nicEventSleepyNotify(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if !defined(_HIF_USB)
	P_EVENT_SLEEPY_INFO_T prEventSleepyNotify;

	prEventSleepyNotify = (P_EVENT_SLEEPY_INFO_T) (prEvent->aucBuffer);

	/* DBGLOG(RX, INFO, ("ucSleepyState = %d\n", prEventSleepyNotify->ucSleepyState)); */

	prAdapter->fgWiFiInSleepyState = (BOOLEAN) (prEventSleepyNotify->ucSleepyState);

#if CFG_SUPPORT_MULTITHREAD
	if (prEventSleepyNotify->ucSleepyState)
		kalSetFwOwnEvent2Hif(prAdapter->prGlueInfo);
#endif
#endif
}

VOID nicEventBtOverWifi(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if CFG_ENABLE_BT_OVER_WIFI
	UINT_8 aucTmp[sizeof(AMPC_EVENT) + sizeof(BOW_LINK_DISCONNECTED)];
	P_EVENT_BT_OVER_WIFI prEventBtOverWifi;
	P_AMPC_EVENT prBowEvent;
	P_BOW_LINK_CONNECTED prBowLinkConnected;
	P_BOW_LINK_DISCONNECTED prBowLinkDisconnected;

	prEventBtOverWifi = (P_EVENT_BT_OVER_WIFI) (prEvent->aucBuffer);

	/* construct event header */
	prBowEvent = (P_AMPC_EVENT) aucTmp;

	if (prEventBtOverWifi->ucLinkStatus == 0) {
		/* Connection */
		prBowEvent->rHeader.ucEventId = BOW_EVENT_ID_LINK_CONNECTED;
		prBowEvent->rHeader.ucSeqNumber = 0;
		prBowEvent->rHeader.u2PayloadLength = sizeof(BOW_LINK_CONNECTED);

		/* fill event body */
		prBowLinkConnected = (P_BOW_LINK_CONNECTED) (prBowEvent->aucPayload);
		prBowLinkConnected->rChannel.ucChannelNum = prEventBtOverWifi->ucSelectedChannel;
		kalMemZero(prBowLinkConnected->aucPeerAddress, MAC_ADDR_LEN);	/* @FIXME */

		kalIndicateBOWEvent(prAdapter->prGlueInfo, prBowEvent);
	} else {
		/* Disconnection */
		prBowEvent->rHeader.ucEventId = BOW_EVENT_ID_LINK_DISCONNECTED;
		prBowEvent->rHeader.ucSeqNumber = 0;
		prBowEvent->rHeader.u2PayloadLength = sizeof(BOW_LINK_DISCONNECTED);

		/* fill event body */
		prBowLinkDisconnected = (P_BOW_LINK_DISCONNECTED) (prBowEvent->aucPayload);
		prBowLinkDisconnected->ucReason = 0;	/* @FIXME */
		kalMemZero(prBowLinkDisconnected->aucPeerAddress, MAC_ADDR_LEN);	/* @FIXME */

		kalIndicateBOWEvent(prAdapter->prGlueInfo, prBowEvent);
	}
#endif
}

VOID nicEventStatistics(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_CMD_INFO_T prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rStatStruct, prEvent->aucBuffer, sizeof(EVENT_STATISTICS));

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo, prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}
VOID nicEventWlanInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_CMD_INFO_T prCmdInfo;

	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();
	kalMemCopy(&prAdapter->rEventWlanInfo, prEvent->aucBuffer, sizeof(EVENT_WLAN_INFO));

	DBGLOG(RSN, INFO, "EVENT_ID_WLAN_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo, prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}

VOID nicEventMibInfo(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_CMD_INFO_T prCmdInfo;


	/* buffer statistics for further query */
	prAdapter->fgIsStatValid = TRUE;
	prAdapter->rStatUpdateTime = kalGetTimeTick();

	DBGLOG(RSN, INFO, "EVENT_ID_MIB_INFO");
	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo, prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}

}

VOID nicEventBeaconTimeout(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	DBGLOG(NIC, INFO, "EVENT_ID_BSS_BEACON_TIMEOUT\n");

	if (prAdapter->fgDisBcnLostDetection == FALSE) {
		P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T) NULL;
		P_EVENT_BSS_BEACON_TIMEOUT_T prEventBssBeaconTimeout;

		prEventBssBeaconTimeout = (P_EVENT_BSS_BEACON_TIMEOUT_T) (prEvent->aucBuffer);

		if (prEventBssBeaconTimeout->ucBssIndex >= MAX_BSS_INDEX)
			return;

		DBGLOG(NIC, INFO, "Reason code: %d\n", prEventBssBeaconTimeout->ucReasonCode);

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prEventBssBeaconTimeout->ucBssIndex);

		if (prEventBssBeaconTimeout->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex)
			aisBssBeaconTimeout(prAdapter);
#if CFG_ENABLE_WIFI_DIRECT
		else if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P)
			p2pRoleFsmRunEventBeaconTimeout(prAdapter, prBssInfo);
#endif
#if CFG_ENABLE_BT_OVER_WIFI
		else if (GET_BSS_INFO_BY_INDEX(prAdapter, prEventBssBeaconTimeout->ucBssIndex)->eNetworkType ==
			 NETWORK_TYPE_BOW) {
			/* ToDo:: Nothing */
		}
#endif
		else {
			DBGLOG(RX, ERROR, "EVENT_ID_BSS_BEACON_TIMEOUT: (ucBssIndex = %d)\n",
				prEventBssBeaconTimeout->ucBssIndex);
		}
	}

}

VOID nicEventUpdateNoaParams(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered) {
		P_EVENT_UPDATE_NOA_PARAMS_T prEventUpdateNoaParam;

		prEventUpdateNoaParam = (P_EVENT_UPDATE_NOA_PARAMS_T) (prEvent->aucBuffer);

		if (GET_BSS_INFO_BY_INDEX(prAdapter, prEventUpdateNoaParam->ucBssIndex)->eNetworkType ==
			NETWORK_TYPE_P2P) {

			p2pProcessEvent_UpdateNOAParam(prAdapter, prEventUpdateNoaParam->ucBssIndex,
				prEventUpdateNoaParam);
		} else {
			ASSERT(0);
		}
	}
#else
	ASSERT(0);
#endif
}

VOID nicEventStaAgingTimeout(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	if (prAdapter->fgDisStaAgingTimeoutDetection == FALSE) {
		P_EVENT_STA_AGING_TIMEOUT_T prEventStaAgingTimeout;
		P_STA_RECORD_T prStaRec;
		P_BSS_INFO_T prBssInfo = (P_BSS_INFO_T) NULL;

		prEventStaAgingTimeout = (P_EVENT_STA_AGING_TIMEOUT_T) (prEvent->aucBuffer);
		prStaRec = cnmGetStaRecByIndex(prAdapter, prEventStaAgingTimeout->ucStaRecIdx);
		if (prStaRec == NULL)
			return;

		DBGLOG(NIC, INFO, "EVENT_ID_STA_AGING_TIMEOUT %u " MACSTR "\n",
			prEventStaAgingTimeout->ucStaRecIdx, MAC2STR(prStaRec->aucMacAddr));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

		bssRemoveClient(prAdapter, prBssInfo, prStaRec);

		/* Call False Auth */
		if (prAdapter->fgIsP2PRegistered) {
			p2pFuncDisconnect(prAdapter, prBssInfo, prStaRec, TRUE,
				REASON_CODE_DISASSOC_INACTIVITY);
		}

	}
	/* gDisStaAgingTimeoutDetection */
}

VOID nicEventApObssStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if CFG_ENABLE_WIFI_DIRECT
	if (prAdapter->fgIsP2PRegistered)
		rlmHandleObssStatusEventPkt(prAdapter, (P_EVENT_AP_OBSS_STATUS_T) prEvent->aucBuffer);
#endif
}

VOID nicEventRoamingStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if CFG_SUPPORT_ROAMING
	P_CMD_ROAMING_TRANSIT_T prTransit;

	prTransit = (P_CMD_ROAMING_TRANSIT_T) (prEvent->aucBuffer);

	/*[ tzufan 20160218]For roaming event compatibility, to remove afterward*/
	if (prTransit->u2Event == ROAMING_EVENT_DISCOVERY) {
		if ((prEvent->u2PacketLength - 12) < sizeof(CMD_ROAMING_TRANSIT_T))
			DBGLOG(ROAMING, WARN, "Old FW, only u2Event and u2Data(RCPI) in roaming event are valid\n");
		else if ((prEvent->u2PacketLength - 12) == sizeof(CMD_ROAMING_TRANSIT_T))
			/*[TODO]move to roamingFsmProcessEvent*/
			DBGLOG(ROAMING, INFO, "RX ROAMING_EVENT_DISCOVERY RCPI[%d] Thr[%d] Reason[%d] Time[%ld]\n",
				prTransit->u2Data,
				prTransit->u2RcpiLowThreshold,
				prTransit->eReason,
				prTransit->u4RoamingTriggerTime);
	}

	roamingFsmProcessEvent(prAdapter, prTransit);
#endif /* CFG_SUPPORT_ROAMING */
}

VOID nicEventSendDeauth(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	SW_RFB_T rSwRfb;

#if DBG
	P_WLAN_MAC_HEADER_T prWlanMacHeader;

	prWlanMacHeader = (P_WLAN_MAC_HEADER_T) &prEvent->aucBuffer[0];
	DBGLOG(RX, INFO, "nicRx: aucAddr1: " MACSTR "\n", MAC2STR(prWlanMacHeader->aucAddr1));
	DBGLOG(RX, INFO, "nicRx: aucAddr2: " MACSTR "\n", MAC2STR(prWlanMacHeader->aucAddr2));
#endif

	/* receive packets without StaRec */
	rSwRfb.pvHeader = (P_WLAN_MAC_HEADER_T) &prEvent->aucBuffer[0];
	if (authSendDeauthFrame(prAdapter, NULL, NULL, &rSwRfb, REASON_CODE_CLASS_3_ERR,
		(PFN_TX_DONE_HANDLER) NULL) == WLAN_STATUS_SUCCESS) {

		DBGLOG(RX, INFO, "Send Deauth Error\n");
	}
}

VOID nicEventUpdateRddStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
#if CFG_SUPPORT_RDD_TEST_MODE
	P_EVENT_RDD_STATUS_T prEventRddStatus;

	prEventRddStatus = (P_EVENT_RDD_STATUS_T) (prEvent->aucBuffer);

	prAdapter->ucRddStatus = prEventRddStatus->ucRddStatus;
#endif
}

VOID nicEventUpdateBwcsStatus(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_PTA_IPC_T prEventBwcsStatus;

	prEventBwcsStatus = (P_PTA_IPC_T) (prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
	DBGLOG(RSN, EVENT, "BCM BWCS Event: %02x%02x%02x%02x\n",
	       prEventBwcsStatus->u.aucBTPParams[0],
	       prEventBwcsStatus->u.aucBTPParams[1],
	       prEventBwcsStatus->u.aucBTPParams[2], prEventBwcsStatus->u.aucBTPParams[3]);
#endif

	kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_BWCS_UPDATE,
		(PVOID) prEventBwcsStatus, sizeof(PTA_IPC_T));
}

VOID nicEventUpdateBcmDebug(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_PTA_IPC_T prEventBwcsStatus;

	prEventBwcsStatus = (P_PTA_IPC_T) (prEvent->aucBuffer);

#if CFG_SUPPORT_BCM_BWCS_DEBUG
	DBGLOG(RSN, EVENT, "BCM FW status: %02x%02x%02x%02x\n",
	       prEventBwcsStatus->u.aucBTPParams[0],
	       prEventBwcsStatus->u.aucBTPParams[1],
	       prEventBwcsStatus->u.aucBTPParams[2], prEventBwcsStatus->u.aucBTPParams[3]);
#endif
}

VOID nicEventAddPkeyDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_ADD_KEY_DONE_INFO prAddKeyDone;
	P_STA_RECORD_T prStaRec;

	prAddKeyDone = (P_EVENT_ADD_KEY_DONE_INFO) (prEvent->aucBuffer);

	DBGLOG(RSN, EVENT, "EVENT_ID_ADD_PKEY_DONE BSSIDX=%d " MACSTR "\n",
		prAddKeyDone->ucBSSIndex, MAC2STR(prAddKeyDone->aucStaAddr));

	prStaRec = cnmGetStaRecByAddress(prAdapter, prAddKeyDone->ucBSSIndex, prAddKeyDone->aucStaAddr);

	if (prStaRec) {
		DBGLOG(RSN, EVENT, "STA " MACSTR " Add Key Done!!\n", MAC2STR(prStaRec->aucMacAddr));
		prStaRec->fgIsTxKeyReady = TRUE;
		qmUpdateStaRec(prAdapter, prStaRec);
	}
}

VOID nicEventIcapDone(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_ICAP_STATUS_T prEventIcapStatus;
	PARAM_CUSTOM_MEM_DUMP_STRUCT_T rMemDumpInfo;
	UINT_32 u4QueryInfo;

	prEventIcapStatus = (P_EVENT_ICAP_STATUS_T) (prEvent->aucBuffer);

	rMemDumpInfo.u4Address = prEventIcapStatus->u4StartAddress;
	rMemDumpInfo.u4Length = prEventIcapStatus->u4IcapSieze;
#if CFG_SUPPORT_QA_TOOL
	rMemDumpInfo.u4IcapContent = prEventIcapStatus->u4IcapContent;
#endif

	wlanoidQueryMemDump(prAdapter, &rMemDumpInfo, sizeof(rMemDumpInfo), &u4QueryInfo);
}

VOID nicEventDebugMsg(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_EVENT_DEBUG_MSG_T prEventDebugMsg;
	UINT_16 u2DebugMsgId;
	UINT_8 ucMsgType;
	UINT_8 ucFlags;
	UINT_32 u4Value;
	UINT_16 u2MsgSize;
	P_UINT_8 pucMsg;

	prEventDebugMsg = (P_EVENT_DEBUG_MSG_T) (prEvent->aucBuffer);

	u2DebugMsgId = prEventDebugMsg->u2DebugMsgId;
	ucMsgType = prEventDebugMsg->ucMsgType;
	ucFlags = prEventDebugMsg->ucFlags;
	u4Value = prEventDebugMsg->u4Value;
	u2MsgSize = prEventDebugMsg->u2MsgSize;
	pucMsg = prEventDebugMsg->aucMsg;

	DBGLOG(SW4, TRACE, "DEBUG_MSG Id %u Type %u Fg 0x%x Val 0x%x Size %u\n",
		u2DebugMsgId, ucMsgType, ucFlags, u4Value, u2MsgSize);

	if (u2MsgSize <= DEBUG_MSG_SIZE_MAX) {
		if (ucMsgType >= DEBUG_MSG_TYPE_END)
			ucMsgType = DEBUG_MSG_TYPE_MEM32;

		if (ucMsgType == DEBUG_MSG_TYPE_ASCII) {
			PUINT_8 pucChr;

			pucMsg[u2MsgSize] = '\0';

			/* skip newline */
			pucChr = kalStrChr(pucMsg, '\0');
			if (*(pucChr - 1) == '\n')
				*(pucChr - 1) = '\0';

			DBGLOG(SW4, INFO, "<FW>%s\n", pucMsg);
		} else if (ucMsgType == DEBUG_MSG_TYPE_MEM8) {
			DBGLOG(SW4, INFO, "<FW>Dump MEM8\n");
			DBGLOG_MEM8(SW4, INFO, pucMsg, u2MsgSize);
		} else {
			DBGLOG(SW4, INFO, "<FW>Dump MEM32\n");
			DBGLOG_MEM32(SW4, INFO, pucMsg, u2MsgSize);
		}
	} /* DEBUG_MSG_SIZE_MAX */
	else
		DBGLOG(SW4, INFO, "Debug msg size %u is too large.\n", u2MsgSize);
}

VOID nicEventTdls(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	TdlsexEventHandle(prAdapter->prGlueInfo, (PUINT_8)prEvent->aucBuffer,
		(UINT_32)(prEvent->u2PacketLength - 8));
}

VOID nicEventDumpMem(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	P_CMD_INFO_T prCmdInfo;

	DBGLOG(SW4, INFO, "%s: EVENT_ID_DUMP_MEM\n", __func__);

	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		DBGLOG(NIC, INFO, ": ==> 1\n");
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo, prEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo->fgSetQuery, 0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	} else {
		/* Burst mode */
		DBGLOG(NIC, INFO, ": ==> 2\n");
		nicEventQueryMemDump(prAdapter, prEvent->aucBuffer);
	}
}

VOID nicEventAssertDump(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	if (prEvent->ucS2DIndex == S2D_INDEX_EVENT_N2H) {
		if (!prAdapter->fgN9AssertDumpOngoing) {
			DBGLOG(NIC, INFO, "%s: EVENT_ID_ASSERT_DUMP\n", __func__);
			DBGLOG(NIC, INFO, "\n[DUMP_N9]====N9 ASSERT_DUMPSTART====\n");
			prAdapter->fgKeepPrintCoreDump = TRUE;
			if (kalOpenCorDumpFile(TRUE) != WLAN_STATUS_SUCCESS)
				DBGLOG(NIC, INFO, "kalOpenCorDumpFile fail\n");
			else
				prAdapter->fgN9CorDumpFileOpend = TRUE;

			prAdapter->fgN9AssertDumpOngoing = TRUE;
				}
		if (prAdapter->fgN9AssertDumpOngoing) {

			if (prAdapter->fgKeepPrintCoreDump)
				DBGLOG(NIC, INFO, "[DUMP_N9]%s:\n", prEvent->aucBuffer);
			if (!kalStrnCmp(prEvent->aucBuffer, ";more log added here", 5)
			    || !kalStrnCmp(prEvent->aucBuffer, ";[core dump start]", 5))
				prAdapter->fgKeepPrintCoreDump = FALSE;

			if (prAdapter->fgN9CorDumpFileOpend) {
				if (kalWriteCorDumpFile(prEvent->aucBuffer, prEvent->u2PacketLength, TRUE) !=
				    WLAN_STATUS_SUCCESS) {
					DBGLOG(NIC, INFO, "kalWriteN9CorDumpFile fail\n");
				}
			}
			wlanCorDumpTimerReset(prAdapter, TRUE);
		}
	} else {
		/* prEvent->ucS2DIndex == S2D_INDEX_EVENT_C2H */
		if (!prAdapter->fgCr4AssertDumpOngoing) {
			DBGLOG(NIC, INFO, "%s: EVENT_ID_ASSERT_DUMP\n", __func__);
			DBGLOG(NIC, INFO, "\n[DUMP_Cr4]====CR4 ASSERT_DUMPSTART====\n");
			prAdapter->fgKeepPrintCoreDump = TRUE;
			if (kalOpenCorDumpFile(FALSE) != WLAN_STATUS_SUCCESS)
				DBGLOG(NIC, INFO, "kalOpenCorDumpFile fail\n");
			else
				prAdapter->fgCr4CorDumpFileOpend = TRUE;

			prAdapter->fgCr4AssertDumpOngoing = TRUE;
		}
		if (prAdapter->fgCr4AssertDumpOngoing) {
			if (prAdapter->fgKeepPrintCoreDump)
				DBGLOG(NIC, INFO, "[DUMP_CR4]%s:\n", prEvent->aucBuffer);
			if (!kalStrnCmp(prEvent->aucBuffer, ";more log added here", 5))
				prAdapter->fgKeepPrintCoreDump = FALSE;

			if (prAdapter->fgCr4CorDumpFileOpend) {
				if (kalWriteCorDumpFile(prEvent->aucBuffer, prEvent->u2PacketLength, FALSE) !=
				    WLAN_STATUS_SUCCESS) {
					DBGLOG(NIC, INFO, "kalWriteN9CorDumpFile fail\n");
				}
			}
			wlanCorDumpTimerReset(prAdapter, FALSE);
		}
	}
}

VOID nicEventRddSendPulse(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	DBGLOG(RLM, INFO, "%s: EVENT_ID_RDD_SEND_PULSE\n", __func__);

	nicEventRddPulseDump(prAdapter, prEvent->aucBuffer);
}

VOID nicEventUpdateCoexPhyrate(IN P_ADAPTER_T prAdapter, IN P_WIFI_EVENT_T prEvent)
{
	UINT_8 i;
	P_EVENT_UPDATE_COEX_PHYRATE_T prEventUpdateCoexPhyrate;

	DBGLOG(NIC, LOUD, "%s\n", __func__);

	prEventUpdateCoexPhyrate = (P_EVENT_UPDATE_COEX_PHYRATE_T)(prEvent->aucBuffer);

	for (i = 0; i < (HW_BSSID_NUM+1); i++) {
		prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit = prEventUpdateCoexPhyrate->au4PhyRateLimit[i];
		DBGLOG(NIC, INFO, "Coex:BSS[%d]R:%d\n", i, prAdapter->aprBssInfo[i]->u4CoexPhyRateLimit);
	}
}

