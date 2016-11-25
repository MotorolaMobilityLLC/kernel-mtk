/*
**
*/

/*! \file   "nic_rate.c"
    \brief  This file contains the transmission rate handling routines.

    This file contains the transmission rate handling routines for setting up
    ACK/CTS Rate, Highest Tx Rate, Lowest Tx Rate, Initial Tx Rate and do
    conversion between Rate Set and Data Rates.
*/

/*
** Log: rate.c
**
** 07 12 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** 1. Update VHT IE composing function
** 2. disable bow
** 3. Exchange bss/sta rec update sequence for temp solution
**
** 11 06 2012 eason.tsai
** [BORA00002255] [MT6630 Wi-Fi][Driver] develop
** .
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 06 08 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * add rate.c.
 *
 * 03 16 2010 kevin.huang
 * [BORA00000663][WIFISYS][New Feature] AdHoc Mode Support
 * Add AdHoc Mode
 *
 * 12 18 2009 cm.chang
 * [BORA00000018]Integrate WIFI part into BORA for the 1st time
 * .
 *
 * Nov 23 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Update comments
 *
 * Nov 16 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 * Fix DBGLOG
 *
 * Nov 5 2009 mtk01461
 * [BORA00000018] Integrate WIFI part into BORA for the 1st time
 *
**  \main\maintrunk.MT5921\12 2008-12-19 17:19:32 GMT mtk01461
**  Fix the problem that do not ASSERT the length of Supported Rate IE == 8
**  \main\maintrunk.MT5921\11 2008-12-01 18:17:42 GMT mtk01088
**  fixed the lint "possible using null pointer" warning
**  \main\maintrunk.MT5921\10 2008-08-20 00:16:36 GMT mtk01461
**  Update for Driver Review
**  \main\maintrunk.MT5921\9 2008-04-13 21:17:13 GMT mtk01461
**  Revise GEN Link Speed OID
**  \main\maintrunk.MT5921\8 2008-03-28 10:40:13 GMT mtk01461
**  Add rateGetRateSetFromDataRates() for set desired rate OID
**  \main\maintrunk.MT5921\7 2008-03-26 09:16:20 GMT mtk01461
**  Add adopt operational rate as ACK rate if BasicRateSet was not found
**  Add comments
**  \main\maintrunk.MT5921\6 2008-02-21 15:01:39 GMT mtk01461
**  Add initial rate according rx signal quality support
**  \main\maintrunk.MT5921\5 2008-01-07 15:06:44 GMT mtk01461
**  Fix typo of rate adaptation of CtrlResp Frame
**  \main\maintrunk.MT5921\4 2007-10-25 18:05:12 GMT mtk01461
**  Add VOIP SCAN Support  & Refine Roaming
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

const UINT_16 au2RateCCKLong[CCK_RATE_NUM] = {
	RATE_CCK_1M_LONG,	/* RATE_1M_INDEX = 0 */
	RATE_CCK_2M_LONG,	/* RATE_2M_INDEX */
	RATE_CCK_5_5M_LONG,	/* RATE_5_5M_INDEX */
	RATE_CCK_11M_LONG	/* RATE_11M_INDEX */
};

const UINT_16 au2RateCCKShort[CCK_RATE_NUM] = {
	RATE_CCK_1M_LONG,	/* RATE_1M_INDEX = 0 */
	RATE_CCK_2M_SHORT,	/* RATE_2M_INDEX */
	RATE_CCK_5_5M_SHORT,	/* RATE_5_5M_INDEX */
	RATE_CCK_11M_SHORT	/* RATE_11M_INDEX */
};

const UINT_16 au2RateOFDM[OFDM_RATE_NUM] = {
	RATE_OFDM_6M,		/* RATE_6M_INDEX */
	RATE_OFDM_9M,		/* RATE_9M_INDEX */
	RATE_OFDM_12M,		/* RATE_12M_INDEX */
	RATE_OFDM_18M,		/* RATE_18M_INDEX */
	RATE_OFDM_24M,		/* RATE_24M_INDEX */
	RATE_OFDM_36M,		/* RATE_36M_INDEX */
	RATE_OFDM_48M,		/* RATE_48M_INDEX */
	RATE_OFDM_54M		/* RATE_54M_INDEX */
};

const UINT_16 au2RateHTMixed[HT_RATE_NUM] = {
	RATE_MM_MCS_32,		/* RATE_MCS32_INDEX, */
	RATE_MM_MCS_0,		/* RATE_MCS0_INDEX, */
	RATE_MM_MCS_1,		/* RATE_MCS1_INDEX, */
	RATE_MM_MCS_2,		/* RATE_MCS2_INDEX, */
	RATE_MM_MCS_3,		/* RATE_MCS3_INDEX, */
	RATE_MM_MCS_4,		/* RATE_MCS4_INDEX, */
	RATE_MM_MCS_5,		/* RATE_MCS5_INDEX, */
	RATE_MM_MCS_6,		/* RATE_MCS6_INDEX, */
	RATE_MM_MCS_7		/* RATE_MCS7_INDEX, */
};

const UINT_16 au2RateHTGreenField[HT_RATE_NUM] = {
	RATE_GF_MCS_32,		/* RATE_MCS32_INDEX, */
	RATE_GF_MCS_0,		/* RATE_MCS0_INDEX, */
	RATE_GF_MCS_1,		/* RATE_MCS1_INDEX, */
	RATE_GF_MCS_2,		/* RATE_MCS2_INDEX, */
	RATE_GF_MCS_3,		/* RATE_MCS3_INDEX, */
	RATE_GF_MCS_4,		/* RATE_MCS4_INDEX, */
	RATE_GF_MCS_5,		/* RATE_MCS5_INDEX, */
	RATE_GF_MCS_6,		/* RATE_MCS6_INDEX, */
	RATE_GF_MCS_7,		/* RATE_MCS7_INDEX, */
};

const UINT_16 au2RateVHT[VHT_RATE_NUM] = {
	RATE_VHT_MCS_0,		/* RATE_MCS0_INDEX, */
	RATE_VHT_MCS_1,		/* RATE_MCS1_INDEX, */
	RATE_VHT_MCS_2,		/* RATE_MCS2_INDEX, */
	RATE_VHT_MCS_3,		/* RATE_MCS3_INDEX, */
	RATE_VHT_MCS_4,		/* RATE_MCS4_INDEX, */
	RATE_VHT_MCS_5,		/* RATE_MCS5_INDEX, */
	RATE_VHT_MCS_6,		/* RATE_MCS6_INDEX, */
	RATE_VHT_MCS_7,		/* RATE_MCS7_INDEX, */
	RATE_VHT_MCS_8,		/* RATE_MCS8_INDEX, */
	RATE_VHT_MCS_9		/* RATE_MCS9_INDEX, */
};

/* in unit of 100kb/s */
const EMU_MAC_RATE_INFO_T arMcsRate2PhyRate[] = {
	/* Phy Rate Code,           BW20,  BW20 SGI, BW40, BW40 SGI, BW80, BW80 SGI, BW160, BW160 SGI */
	RATE_INFO(PHY_RATE_MCS0, 65, 72, 135, 150, 293, 325, 585, 650),
	RATE_INFO(PHY_RATE_MCS1, 130, 144, 270, 300, 585, 650, 1170, 1300),
	RATE_INFO(PHY_RATE_MCS2, 195, 217, 405, 450, 878, 975, 1755, 1950),
	RATE_INFO(PHY_RATE_MCS3, 260, 289, 540, 600, 1170, 1300, 2340, 2600),
	RATE_INFO(PHY_RATE_MCS4, 390, 433, 810, 900, 1755, 1950, 3510, 3900),
	RATE_INFO(PHY_RATE_MCS5, 520, 578, 1080, 1200, 2340, 2600, 4680, 5200),
	RATE_INFO(PHY_RATE_MCS6, 585, 650, 1215, 1350, 2633, 2925, 5265, 5850),
	RATE_INFO(PHY_RATE_MCS7, 650, 722, 1350, 1500, 2925, 3250, 5850, 6500),
	RATE_INFO(PHY_RATE_MCS8, 780, 867, 1620, 1800, 3510, 3900, 7020, 7800),
	RATE_INFO(PHY_RATE_MCS9, 0, 0, 1800, 2000, 3900, 4333, 7800, 8667),
	RATE_INFO(PHY_RATE_MCS32, 0, 0, 60, 67, 0, 0, 0, 0)
};

/* in uint of 500kb/s */
const UINT_8 aucHwRate2PhyRate[] = {
	RATE_1M,		/*1M long */
	RATE_2M,		/*2M long */
	RATE_5_5M,		/*5.5M long */
	RATE_11M,		/*11M long */
	RATE_1M,		/*1M short invalid */
	RATE_2M,		/*2M short */
	RATE_5_5M,		/*5.5M short */
	RATE_11M,		/*11M short */
	RATE_48M,		/*48M */
	RATE_24M,		/*24M */
	RATE_12M,		/*12M */
	RATE_6M,		/*6M */
	RATE_54M,		/*54M */
	RATE_36M,		/*36M */
	RATE_18M,		/*18M */
	RATE_9M			/*9M */
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

UINT_32
nicGetPhyRateByMcsRate(
	IN UINT_8 ucIdx,
	IN UINT_8 ucBw,
	IN UINT_8 ucGI)
{
	return	arMcsRate2PhyRate[ucIdx].u4PhyRate[ucBw][ucGI];
}

UINT_32
nicGetHwRateByPhyRate(
	IN UINT_8 ucIdx)
{
	return	aucHwRate2PhyRate[ucIdx]; /* uint : 500 kbps */
}

WLAN_STATUS
nicSwIndex2RateIndex(
	IN UINT_8 ucRateSwIndex,
	OUT PUINT_8 pucRateIndex,
	OUT PUINT_8 pucPreambleOption
	)
{
	ASSERT(pucRateIndex);
	ASSERT(pucPreambleOption);

	if (ucRateSwIndex >= RATE_6M_SW_INDEX) {
		*pucRateIndex = ucRateSwIndex - RATE_6M_SW_INDEX;
		*pucPreambleOption = PREAMBLE_OFDM_MODE;
	} else {
		*pucRateIndex = ucRateSwIndex;
		*pucPreambleOption = PREAMBLE_DEFAULT_LONG_NONE;
	}
	return WLAN_STATUS_SUCCESS;
}

WLAN_STATUS nicRateIndex2RateCode(IN UINT_8 ucPreambleOption, IN UINT_8 ucRateIndex, OUT PUINT_16 pu2RateCode)
{
	switch (ucPreambleOption) {
	case PREAMBLE_DEFAULT_LONG_NONE:
		if (ucRateIndex >= CCK_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateCCKLong[ucRateIndex];
		break;

	case PREAMBLE_OPTION_SHORT:
		if (ucRateIndex >= CCK_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateCCKShort[ucRateIndex];
		break;

	case PREAMBLE_OFDM_MODE:
		if (ucRateIndex >= OFDM_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateOFDM[ucRateIndex];
		break;

	case PREAMBLE_HT_MIXED_MODE:
		if (ucRateIndex >= HT_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateHTMixed[ucRateIndex];
		break;

	case PREAMBLE_HT_GREEN_FIELD:
		if (ucRateIndex >= HT_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateHTGreenField[ucRateIndex];
		break;

	case PREAMBLE_VHT_FIELD:
		if (ucRateIndex >= VHT_RATE_NUM)
			return WLAN_STATUS_INVALID_DATA;
		*pu2RateCode = au2RateVHT[ucRateIndex];
		break;

	default:
		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
}

UINT_32
nicRateCode2PhyRate(
	IN UINT_16  u2RateCode,
	IN UINT_8   ucBandwidth,
	IN UINT_8   ucGI,
	IN UINT_8   ucRateNss)
{
	UINT_8 ucPhyRate;
	UINT_16 u2TxMode;
	UINT_32 u4PhyRateBy1SS, u4PhyRateIn100Kbps = 0;

	ucPhyRate = RATE_CODE_GET_PHY_RATE(u2RateCode);
	u2TxMode = u2RateCode & RATE_TX_MODE_MASK;
	ucRateNss = ucRateNss + AR_SS_1; /* change to be base=1 */

	if ((u2TxMode == TX_MODE_HT_GF) || (u2TxMode == TX_MODE_HT_MM)) {

		if (ucPhyRate > PHY_RATE_MCS7)
			u2RateCode = u2RateCode - HT_RATE_MCS7_INDEX;
		else
			ucRateNss = AR_SS_1;

	} else if ((u2TxMode == TX_MODE_OFDM) || (u2TxMode == TX_MODE_CCK)) {
		ucRateNss = AR_SS_1;
	}

	u4PhyRateBy1SS = nicRateCode2DataRate(u2RateCode, ucBandwidth, ucGI);
	u4PhyRateIn100Kbps = u4PhyRateBy1SS * ucRateNss;

	DBGLOG(NIC, TRACE, "Coex:nicRateCode2PhyRate,RC:%x,B:%d,I:%d,1ss R:%d,PHY R:%d\n",
			u2RateCode, ucBandwidth, ucGI, u4PhyRateBy1SS, u4PhyRateIn100Kbps);

	return u4PhyRateIn100Kbps;
}

UINT_32
nicRateCode2DataRate(
	IN UINT_16  u2RateCode,
	IN UINT_8   ucBandwidth,
	IN UINT_8   ucGI)
{
	UINT_8 ucPhyRate, ucIdx, ucBw = 0;
	UINT_32 u4PhyRateIn100Kbps = 0;
	UINT_16 u2TxMode;

	if ((ucBandwidth == FIX_BW_NO_FIXED) || (ucBandwidth == FIX_BW_20))
		ucBw = MAC_BW_20;
	else if (ucBandwidth == FIX_BW_40)
		ucBw = MAC_BW_40;
	else if (ucBandwidth == FIX_BW_80)
		ucBw = MAC_BW_80;
	else if (ucBandwidth == FIX_BW_160)
		ucBw = MAC_BW_160;

	ucPhyRate = RATE_CODE_GET_PHY_RATE(u2RateCode);
	u2TxMode = u2RateCode & RATE_TX_MODE_MASK;
	/* Set MMSS parameter if HT/VHT rate */
	if ((u2TxMode == TX_MODE_HT_GF) ||
		(u2TxMode == TX_MODE_HT_MM) ||
		(u2TxMode == TX_MODE_VHT)) {
		/* No SGI Greenfield for 1T */
		/* Refer to section 20.3.11.11.6 of IEEE802.11-2012 */
		if (u2TxMode == TX_MODE_HT_GF)
			ucGI = MAC_GI_NORMAL;

		ucIdx = ucPhyRate;

		if (ucIdx == PHY_RATE_MCS32)
			ucIdx = 10;

		u4PhyRateIn100Kbps = nicGetPhyRateByMcsRate(ucIdx, ucBw, ucGI);
	} else if ((u2TxMode == TX_MODE_OFDM) ||
		(u2TxMode == TX_MODE_CCK)) {
		u4PhyRateIn100Kbps = (nicGetHwRateByPhyRate(ucPhyRate & BITS(0, 3)))*5;
	} else {
		ASSERT(FALSE);
	}
	return u4PhyRateIn100Kbps;
}

BOOLEAN
nicGetRateIndexFromRateSetWithLimit(
	IN UINT_16 u2RateSet,
	IN UINT_32 u4PhyRateLimit,
	IN BOOLEAN fgGetLowest,
	OUT PUINT_8 pucRateSwIndex)
{
	UINT_32 i;
	UINT_32 u4CurPhyRate, u4TarPhyRate, u4HighestPhyRate, u4LowestPhyRate;
	UINT_8 ucRateIndex, ucRatePreamble, ucTarRateSwIndex, ucHighestPhyRateSwIdx, ucLowestPhyRateSwIdx;
	UINT_16 u2CurRateCode;
	UINT_32 u4Status;

	/* Set init value */
	if (fgGetLowest) {
		u4TarPhyRate = 0xFFFFFFFF;
		u4HighestPhyRate = 0;
		ucHighestPhyRateSwIdx = RATE_NUM_SW;
	} else {
		u4TarPhyRate = 0;
		u4LowestPhyRate = 0xFFFFFFFF;
		ucLowestPhyRateSwIdx = RATE_NUM_SW;
	}

	ucTarRateSwIndex = RATE_NUM_SW;

	/* Find SW rate index by limitation */
	for (i = RATE_1M_SW_INDEX; i <= RATE_54M_SW_INDEX; i++) {
		if (u2RateSet & BIT(i)) {

			/* Convert SW rate index to phy rate in 100kbps */
			nicSwIndex2RateIndex(i, &ucRateIndex, &ucRatePreamble);
			u4Status = nicRateIndex2RateCode(ucRatePreamble, ucRateIndex, &u2CurRateCode);

			if (u4Status != WLAN_STATUS_SUCCESS)
				continue;

			u4CurPhyRate =
				nicRateCode2DataRate(u2CurRateCode, MAC_BW_20, MAC_GI_NORMAL);

			if (u4HighestPhyRate < u4CurPhyRate) {
				u4HighestPhyRate = u4CurPhyRate;
				ucHighestPhyRateSwIdx = i;
			}
			if (u4LowestPhyRate > u4CurPhyRate) {
				u4LowestPhyRate = u4CurPhyRate;
				ucLowestPhyRateSwIdx = i;
			}

			/* Compare */
			if (fgGetLowest) {
				if ((u4CurPhyRate >= u4PhyRateLimit) && (u4CurPhyRate <= u4TarPhyRate)) {
					u4TarPhyRate = u4CurPhyRate;
					ucTarRateSwIndex = i;
				}
			} else {
				if ((u4CurPhyRate <= u4PhyRateLimit) && (u4CurPhyRate >= u4TarPhyRate)) {
					u4TarPhyRate = u4CurPhyRate;
					ucTarRateSwIndex = i;
				}
			}
		}
	}

	/* Return target SW rate index */
	if (ucTarRateSwIndex < RATE_NUM_SW) {
		*pucRateSwIndex = ucTarRateSwIndex;
	} else {
		if (fgGetLowest)
			*pucRateSwIndex = ucHighestPhyRateSwIdx;
		else
			*pucRateSwIndex = ucLowestPhyRateSwIdx;
	}
	return TRUE;
}
