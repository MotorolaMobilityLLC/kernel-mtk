/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/nic/nic_rate.h#1
*/

/*! \file  nic_rate.h
    \brief This file contains the rate utility function of
	   IEEE 802.11 family for MediaTek 802.11 Wireless LAN Adapters.
*/

/*
** Log: nic_rate.h
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
 *
 * 09 03 2010 kevin.huang
 * NULL
 * Refine #include sequence and solve recursive/nested #include issue
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 06 10 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * add buildable & linkable ais_fsm.c
 *
 * related reference are still waiting to be resolved
 *
*/

#ifndef _NIC_RATE_H
#define _NIC_RATE_H

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

/*******************************************************************************
*                         D A T A   T Y P E S
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
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
UINT_32
nicGetPhyRateByMcsRate(
	IN UINT_8 ucIdx,
	IN UINT_8 ucBw,
	IN UINT_8 ucGI
	);

UINT_32
nicGetHwRateByPhyRate(
	IN UINT_8 ucIdx
	);

WLAN_STATUS
nicSwIndex2RateIndex(
	IN UINT_8 ucRateSwIndex,
	OUT PUINT_8 pucRateIndex,
	OUT PUINT_8 pucPreambleOption
	);

WLAN_STATUS
nicRateIndex2RateCode(
	IN UINT_8 ucPreambleOption,
	IN UINT_8 ucRateIndex,
	OUT PUINT_16 pu2RateCode
	);

UINT_32
nicRateCode2PhyRate(
	IN UINT_16 u2RateCode,
	IN UINT_8 ucBandwidth,
	IN UINT_8 ucGI,
	IN UINT_8 ucRateNss
	);

UINT_32
nicRateCode2DataRate(
	IN UINT_16 u2RateCode,
	IN UINT_8 ucBandwidth,
	IN UINT_8 ucGI
	);

BOOLEAN
nicGetRateIndexFromRateSetWithLimit(
	IN UINT_16 u2RateSet,
	IN UINT_32 u4PhyRateLimit,
	IN BOOLEAN fgGetLowest,
	OUT PUINT_8 pucRateSwIndex
	);

#endif /* _NIC_RATE_H */
