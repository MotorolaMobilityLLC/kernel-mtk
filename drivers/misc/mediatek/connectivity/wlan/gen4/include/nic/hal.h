/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/include/nic/hal.h#1
*/

/*! \file   "hal.h"
 *   \brief  The declaration of hal functions
 *
 *   N/A
*/

/*
** Log: hal.h
**
** 03 19 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** restore to max RX length = 16 because RTL has been configured to 16 instead of 64 in data sheet definition
**
** 03 18 2013 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** use RX default maximum length to 16 (max. 64)
**
** 01 22 2013 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Remove compiling warning about print argument of long format
**
** 10 25 2012 cp.wu
** [BORA00002227] [MT6630 Wi-Fi][Driver] Update for Makefile and HIFSYS modifications
** sync with MT6630 HIFSYS update.
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
 *
 * 04 01 2011 tsaiyuan.hsu
 * [WCXRP00000615] [MT 6620 Wi-Fi][Driver] Fix klocwork issues
 * fix the klocwork issues, 57500, 57501, 57502 and 57503.
 *
 * 03 21 2011 cp.wu
 * [WCXRP00000540] [MT5931][Driver] Add eHPI8/eHPI16 support to Linux Glue Layer
 * portability improvement
 *
 * 03 07 2011 terry.wu
 * [WCXRP00000521] [MT6620 Wi-Fi][Driver] Remove non-standard debug message
 * Toggle non-standard debug messages to comments.
 *
 * 11 08 2010 cp.wu
 * [WCXRP00000166] [MT6620 Wi-Fi][Driver] use SDIO CMD52 for enabling/disabling interrupt to reduce transaction period
 * change to use CMD52 for enabling/disabling interrupt to reduce SDIO transaction time
 *
 * 09 01 2010 cp.wu
 * NULL
 * move HIF CR initialization from where after sdioSetupCardFeature() to wlanAdapterStart()
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 06 15 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * change zero-padding for TX port access to HAL.
 *
 * 06 06 2010 kevin.huang
 * [WPD00003832][MT6620 5931] Create driver base
 * [MT6620 5931] Create driver base
 *
 * 04 06 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * eliminate direct access for prGlueInfo->fgIsCardRemoved in non-glue layer
 *
 * 01 27 2010 cp.wu
 * [WPD00001943]Create WiFi test driver framework on WinXP
 * 1. eliminate improper variable in rHifInfo
 *  *  *  * 2. block TX/ordinary OID when RF test mode is engaged
 *  *  *  * 3. wait until firmware finish operation when entering into and leaving from RF test mode
 *  *  *  * 4. correct some HAL implementation
**  \main\maintrunk.MT6620WiFiDriver_Prj\17 2009-12-16 18:02:26 GMT mtk02752
**  include precomp.h
**  \main\maintrunk.MT6620WiFiDriver_Prj\16 2009-12-10 16:43:16 GMT mtk02752
**  code clean
**  \main\maintrunk.MT6620WiFiDriver_Prj\15 2009-11-13 13:54:15 GMT mtk01084
**  \main\maintrunk.MT6620WiFiDriver_Prj\14 2009-11-11 10:36:01 GMT mtk01084
**  modify HAL functions
**  \main\maintrunk.MT6620WiFiDriver_Prj\13 2009-11-09 22:56:28 GMT mtk01084
**  modify HW access routines
**  \main\maintrunk.MT6620WiFiDriver_Prj\12 2009-10-29 19:50:09 GMT mtk01084
**  add new macro HAL_TX_PORT_WR
**  \main\maintrunk.MT6620WiFiDriver_Prj\11 2009-10-23 16:08:10 GMT mtk01084
**  \main\maintrunk.MT6620WiFiDriver_Prj\10 2009-10-13 21:58:50 GMT mtk01084
**  update for new HW architecture design
**  \main\maintrunk.MT6620WiFiDriver_Prj\9 2009-05-18 14:28:10 GMT mtk01084
**  fix issue in HAL_DRIVER_OWN_BY_SDIO_CMD52()
**  \main\maintrunk.MT6620WiFiDriver_Prj\8 2009-05-11 17:26:33 GMT mtk01084
**  modify the bit definition to check driver own status
**  \main\maintrunk.MT6620WiFiDriver_Prj\7 2009-04-28 10:30:22 GMT mtk01461
**  Fix typo
**  \main\maintrunk.MT6620WiFiDriver_Prj\6 2009-04-01 10:50:34 GMT mtk01461
**  Redefine HAL_PORT_RD/WR macro for SW pre test
**  \main\maintrunk.MT6620WiFiDriver_Prj\5 2009-03-24 09:46:49 GMT mtk01084
**  fix LINT error
**  \main\maintrunk.MT6620WiFiDriver_Prj\4 2009-03-23 16:53:38 GMT mtk01084
**  add HAL_DRIVER_OWN_BY_SDIO_CMD52()
**  \main\maintrunk.MT6620WiFiDriver_Prj\3 2009-03-18 20:53:13 GMT mtk01426
**  Fixed lint warn
**  \main\maintrunk.MT6620WiFiDriver_Prj\2 2009-03-10 20:16:20 GMT mtk01426
**  Init for develop
**
*/

#ifndef _HAL_H
#define _HAL_H

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

/* Macros for flag operations for the Adapter structure */
#define HAL_SET_FLAG(_M, _F)             ((_M)->u4HwFlags |= (_F))
#define HAL_CLEAR_FLAG(_M, _F)           ((_M)->u4HwFlags &= ~(_F))
#define HAL_TEST_FLAG(_M, _F)            ((_M)->u4HwFlags & (_F))
#define HAL_TEST_FLAGS(_M, _F)           (((_M)->u4HwFlags & (_F)) == (_F))

#if defined(_HIF_SDIO)
#define HAL_MCR_RD(_prAdapter, _u4Offset, _pu4Value) \
do { \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
			ASSERT(0); \
		} \
		if (_u4Offset > 0xFFFF) { \
			if (kalDevRegRead_mac(_prAdapter->prGlueInfo, _u4Offset, _pu4Value) == FALSE) {\
				HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
				fgIsBusAccessFailed = TRUE; \
				DBGLOG(HAL, ERROR, "HAL_MCR_RD (MAC) access fail! 0x%lx: 0x%lx\n", \
					(UINT_32) (_u4Offset), *((PUINT_32) (_pu4Value))); \
			} \
		} else { \
			if (kalDevRegRead(_prAdapter->prGlueInfo, _u4Offset, _pu4Value) == FALSE) {\
				HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
				fgIsBusAccessFailed = TRUE; \
				DBGLOG(HAL, ERROR, "HAL_MCR_RD (SDIO) access fail! 0x%lx: 0x%lx\n", \
					(UINT_32) (_u4Offset), *((PUINT_32) (_pu4Value))); \
			} \
		} \
	} else { \
		DBGLOG(HAL, WARN, "ignore HAL_MCR_RD access! 0x%lx\n", \
			(UINT_32) (_u4Offset)); \
	} \
} while (0)

#define HAL_MCR_WR(_prAdapter, _u4Offset, _u4Value) \
do { \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
			ASSERT(0); \
		} \
		if (_u4Offset > 0xFFFF) { \
			if (kalDevRegWrite_mac(_prAdapter->prGlueInfo, _u4Offset, _u4Value) == FALSE) {\
				HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
				fgIsBusAccessFailed = TRUE; \
				DBGLOG(HAL, ERROR, "HAL_MCR_WR (MAC) access fail! 0x%lx: 0x%lx\n", \
					(UINT_32) (_u4Offset), (UINT_32) (_u4Value)); \
			} \
		} else { \
			if (kalDevRegWrite(_prAdapter->prGlueInfo, _u4Offset, _u4Value) == FALSE) {\
				HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
				fgIsBusAccessFailed = TRUE; \
				DBGLOG(HAL, ERROR, "HAL_MCR_WR (SDIO) access fail! 0x%lx: 0x%lx\n", \
					(UINT_32) (_u4Offset), (UINT_32) (_u4Value)); \
			} \
		} \
	} else { \
		DBGLOG(HAL, WARN, "ignore HAL_MCR_WR access! 0x%lx: 0x%lx\n", \
			(UINT_32) (_u4Offset), (UINT_32) (_u4Value)); \
	} \
} while (0)

#define HAL_PORT_RD(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	/*fgResult = FALSE; */\
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (kalDevPortRead(_prAdapter->prGlueInfo, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) == FALSE) {\
			HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
			fgIsBusAccessFailed = TRUE; \
			DBGLOG(HAL, ERROR, "HAL_PORT_RD access fail! 0x%lx\n", \
				(UINT_32) (_u4Port)); \
		} \
		else { \
			/*fgResult = TRUE;*/ } \
	} else { \
		DBGLOG(HAL, WARN, "ignore HAL_PORT_RD access! 0x%lx\n", \
			(UINT_32) (_u4Port)); \
	} \
}

#define HAL_PORT_WR(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	/*fgResult = FALSE; */\
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (kalDevPortWrite(_prAdapter->prGlueInfo, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) == FALSE) {\
			HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
			fgIsBusAccessFailed = TRUE; \
			DBGLOG(HAL, ERROR, "HAL_PORT_WR access fail! 0x%lx\n", \
				(UINT_32) (_u4Port)); \
		} \
		else \
			; /*fgResult = TRUE;*/ \
	} else { \
		DBGLOG(HAL, WARN, "ignore HAL_PORT_WR access! 0x%lx\n", \
			(UINT_32) (_u4Port)); \
	} \
}

#define HAL_BYTE_WR(_prAdapter, _u4Port, _ucBuf) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (kalDevWriteWithSdioCmd52(_prAdapter->prGlueInfo, _u4Port, _ucBuf) == FALSE) {\
			HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
			fgIsBusAccessFailed = TRUE; \
			DBGLOG(HAL, ERROR, "HAL_BYTE_WR access fail! 0x%lx\n", \
				(UINT_32)(_u4Port)); \
		} \
		else { \
			/* Todo:: Nothing*/ \
		} \
	} \
	else { \
		DBGLOG(HAL, WARN, "ignore HAL_BYTE_WR access! 0x%lx\n", \
			(UINT_32) (_u4Port)); \
	} \
}

#define HAL_DRIVER_OWN_BY_SDIO_CMD52(_prAdapter, _pfgDriverIsOwnReady) \
{ \
	UINT_8 ucBuf = BIT(1); \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	if (HAL_TEST_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR) == FALSE) { \
		if (kalDevReadAfterWriteWithSdioCmd52(_prAdapter->prGlueInfo, MCR_WHLPCR_BYTE1, &ucBuf, 1) == FALSE) {\
			HAL_SET_FLAG(_prAdapter, ADAPTER_FLAG_HW_ERR); \
			fgIsBusAccessFailed = TRUE; \
			DBGLOG(HAL, ERROR, "kalDevReadAfterWriteWithSdioCmd52 access fail!\n"); \
		} \
		else { \
			*_pfgDriverIsOwnReady = (ucBuf & BIT(0)) ? TRUE : FALSE; \
		} \
	} else { \
		DBGLOG(HAL, WARN, "ignore HAL_DRIVER_OWN_BY_SDIO_CMD52 access!\n"); \
	} \
}

#else /* #if defined(_HIF_SDIO) */
#define HAL_MCR_RD(_prAdapter, _u4Offset, _pu4Value) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevRegRead(_prAdapter->prGlueInfo, _u4Offset, _pu4Value); \
}

#define HAL_MCR_WR(_prAdapter, _u4Offset, _u4Value) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevRegWrite(_prAdapter->prGlueInfo, _u4Offset, _u4Value); \
}

#define HAL_PORT_RD(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevPortRead(_prAdapter->prGlueInfo, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize); \
}

#define HAL_PORT_WR(_prAdapter, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevPortWrite(_prAdapter->prGlueInfo, _u4Port, _u4Len, _pucBuf, _u4ValidBufSize); \
}

#define HAL_BYTE_WR(_prAdapter, _u4Port, _ucBuf) \
{ \
	HAL_MCR_WR(_prAdapter, _u4Port, (UINT_32)_ucBuf); \
}

#endif /* #if defined(_HIF_SDIO) */

#define HAL_WRITE_TX_DATA(_prAdapter, _prMsduInfo) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevWriteData(_prAdapter->prGlueInfo, _prMsduInfo); \
}

#define HAL_KICK_TX_DATA(_prAdapter) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevKickData(_prAdapter->prGlueInfo); \
}

#define HAL_WRITE_TX_CMD(_prAdapter, _prCmdInfo, _ucTC) \
{ \
	if (_prAdapter->rAcpiState == ACPI_STATE_D3) { \
		ASSERT(0); \
	} \
	kalDevWriteCmd(_prAdapter->prGlueInfo, _prCmdInfo, _ucTC); \
}

#if defined(_HIF_PCIE)
#define HAL_READ_RX_PORT(prAdapter, u4PortId, u4Len, pvBuf, _u4ValidBufSize) \
{ \
	ASSERT(u4PortId < 2); \
	HAL_PORT_RD(prAdapter, \
		u4PortId, \
		u4Len, \
		pvBuf, \
		_u4ValidBufSize/*temp!!*//*4Kbyte*/) \
}

#define HAL_WRITE_TX_PORT(_prAdapter, _u4PortId, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	HAL_PORT_WR(_prAdapter, \
		_u4PortId, \
		_u4Len, \
		_pucBuf, \
		_u4ValidBufSize/*temp!!*//*4KByte*/) \
}

#define HAL_MCR_RD_AND_WAIT(_pAdapter, _offset, _pReadValue, _waitCondition, _waitDelay, _waitCount, _status)

#define HAL_MCR_WR_AND_WAIT(_pAdapter, _offset, _writeValue, _busyMask, _waitDelay, _waitCount, _status)

#define HAL_GET_CHIP_ID_VER(_prAdapter, pu2ChipId, pu2Version) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(_prAdapter, HIF_SYS_REV, &u4Value); \
	*pu2ChipId = ((u4Value & PCIE_HIF_SYS_PROJ) >> 16); \
	*pu2Version = (u4Value & PCIE_HIF_SYS_REV); \
}

#define HAL_WIFI_FUNC_READY_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	UINT_32 u4Value; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	*_pfgResult = FALSE; \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &u4Value); \
	if ((u4Value & (_checkItem << prChipInfo->sw_ready_bit_offset)) \
	     == (_checkItem << prChipInfo->sw_ready_bit_offset)) \
		*_pfgResult = TRUE; \
} while (0)

#define HAL_WIFI_FUNC_OFF_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	UINT_32 u4Value; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	*_pfgResult = FALSE; \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &u4Value); \
	if ((u4Value & (_checkItem << prChipInfo->sw_ready_bit_offset)) == 0) \
		*_pfgResult = TRUE; \
} while (0)

#define HAL_WIFI_FUNC_GET_STATUS(_prAdapter, _u4Result) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &_u4Result); \
} while (0)

#define HAL_INTR_DISABLE(_prAdapter)

#define HAL_INTR_ENABLE(_prAdapter)

#define HAL_INTR_ENABLE_AND_LP_OWN_SET(_prAdapter)

/* Polling interrupt status bit due to CFG_PCIE_LPCR_HOST status bit not work when chip sleep */
#define HAL_LP_OWN_RD(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	HAL_MCR_RD(prAdapter, WPDMA_INT_STA, &u4RegValue); \
	*_pfgResult = HAL_IS_FW_OWNBACK_INTR(u4RegValue);\
}

#define HAL_LP_OWN_SET(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	HAL_MCR_WR(_prAdapter, CFG_PCIE_LPCR_HOST, PCIE_LPCR_HOST_SET_OWN); \
	HAL_MCR_RD(_prAdapter, CFG_PCIE_LPCR_HOST, &u4RegValue); \
	if (u4RegValue == 0) { \
		*_pfgResult = TRUE; \
	} \
}

#define HAL_LP_OWN_CLR(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	/* Software get LP ownership */ \
	HAL_MCR_WR(_prAdapter, \
			CFG_PCIE_LPCR_HOST, \
			PCIE_LPCR_HOST_CLR_OWN); \
	HAL_MCR_RD(_prAdapter, CFG_PCIE_LPCR_HOST, &u4RegValue); \
	if (u4RegValue == 0) { \
		*_pfgResult = TRUE; \
	} \
}

#define HAL_GET_ABNORMAL_INTERRUPT_REASON_CODE(_prAdapter, pu4AbnormalReason)

#define HAL_DISABLE_RX_ENHANCE_MODE(_prAdapter)

#define HAL_ENABLE_RX_ENHANCE_MODE(_prAdapter)

#define HAL_CFG_MAX_HIF_RX_LEN_NUM(_prAdapter, _ucNumOfRxLen)

#define HAL_SET_INTR_STATUS_READ_CLEAR(prAdapter)

#define HAL_SET_INTR_STATUS_WRITE_1_CLEAR(prAdapter)

#define HAL_READ_INTR_STATUS(prAdapter, length, pvBuf)

#define HAL_READ_TX_RELEASED_COUNT(_prAdapter, au2TxReleaseCount)

#define HAL_READ_RX_LENGTH(prAdapter, pu2Rx0Len, pu2Rx1Len)

#define HAL_GET_INTR_STATUS_FROM_ENHANCE_MODE_STRUCT(pvBuf, u2Len, pu4Status)

#define HAL_GET_TX_STATUS_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4BufOut, u4LenBufOut)

#define HAL_GET_RX_LENGTH_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu2Rx0Num, au2Rx0Len, pu2Rx1Num, au2Rx1Len)

#define HAL_GET_MAILBOX_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4Mailbox0, pu4Mailbox1)

#define HAL_IS_TX_DONE_INTR(u4IntrStatus) \
	((u4IntrStatus & (WPDMA_TX_DONE_INT0 | WPDMA_TX_DONE_INT1 | WPDMA_TX_DONE_INT2 | WPDMA_TX_DONE_INT3)) \
	? TRUE : FALSE)

#define HAL_IS_RX_DONE_INTR(u4IntrStatus) \
	((u4IntrStatus & (WPDMA_RX_DONE_INT0 | WPDMA_RX_DONE_INT1)) ? TRUE : FALSE)

#define HAL_IS_ABNORMAL_INTR(u4IntrStatus)

#define HAL_IS_FW_OWNBACK_INTR(u4IntrStatus) \
	((u4IntrStatus & WPDMA_FW_CLR_OWN_INT) ? TRUE : FALSE)

#define HAL_PUT_MAILBOX(prAdapter, u4MboxId, u4Data)

#define HAL_GET_MAILBOX(prAdapter, u4MboxId, pu4Data)

#define HAL_SET_MAILBOX_READ_CLEAR(prAdapter, fgEnableReadClear) \
{ \
	prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear = fgEnableReadClear;\
}

#define HAL_GET_MAILBOX_READ_CLEAR(prAdapter) (prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear)

#define HAL_READ_INT_STATUS(prAdapter, _pu4IntStatus) \
{ \
	kalDevReadIntStatus(prAdapter, _pu4IntStatus);\
}

#define HAL_HIF_INIT(prAdapter)

#define HAL_ENABLE_FWDL(_prAdapter, _fgEnable)

#define HAL_WAKE_UP_WIFI(_prAdapter) \
{ \
	halWakeUpWiFi(_prAdapter);\
}

#define HAL_CLEAR_DUMMY_REQ(_prAdapter) \
{ \
	struct mt66xx_chip_info *prChipInfo; \
	UINT_32 u4Value; \
	if (!_prAdapter->chip_info) {\
		ASSERT(0); \
	} else {\
		prChipInfo = _prAdapter->chip_info; \
		HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &u4Value); \
		u4Value &= ~(WIFI_FUNC_DUMMY_REQ << prChipInfo->sw_ready_bit_offset);\
		HAL_MCR_WR(prAdapter, prChipInfo->sw_sync0, u4Value);\
	} \
}

#endif

#if defined(_HIF_SDIO)
#define HIF_H2D_SW_INT_SHFT                 (16)
#define SDIO_MAILBOX_FUNC_READ_REG_IDX      (BIT(0) << HIF_H2D_SW_INT_SHFT) /* bit16 */
#define SDIO_MAILBOX_FUNC_WRITE_REG_IDX     (BIT(1) << HIF_H2D_SW_INT_SHFT) /* bit17 */
#define SDIO_MAILBOX_FUNC_CHECKSUN16_IDX    (BIT(2) << HIF_H2D_SW_INT_SHFT) /* bit18 */

#define HAL_READ_RX_PORT(prAdapter, u4PortId, u4Len, pvBuf, _u4ValidBufSize) \
{ \
	ASSERT(u4PortId < 2); \
	HAL_PORT_RD(prAdapter, \
		((u4PortId == 0) ? MCR_WRDR0 : MCR_WRDR1), \
		u4Len, \
		pvBuf, \
		_u4ValidBufSize/*temp!!*//*4Kbyte*/) \
}

#define HAL_WRITE_TX_PORT(_prAdapter, _u4PortId, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	if ((_u4ValidBufSize - _u4Len) >= sizeof(UINT_32)) { \
		/* fill with single dword of zero as TX-aggregation termination */ \
		*(PUINT_32) (&((_pucBuf)[ALIGN_4(_u4Len)])) = 0; \
	} \
	HAL_PORT_WR(_prAdapter, \
		MCR_WTDR1, \
		_u4Len, \
		_pucBuf, \
		_u4ValidBufSize/*temp!!*//*4KByte*/) \
}

/* The macro to read the given MCR several times to check if the wait
 *  condition come true.
 */
#define HAL_MCR_RD_AND_WAIT(_pAdapter, _offset, _pReadValue, _waitCondition, _waitDelay, _waitCount, _status) \
	{ \
		UINT_32 count; \
		(_status) = FALSE; \
		for (count = 0; count < (_waitCount); count++) { \
			HAL_MCR_RD((_pAdapter), (_offset), (_pReadValue)); \
			if ((_waitCondition)) { \
				(_status) = TRUE; \
				break; \
			} \
			kalUdelay((_waitDelay)); \
		} \
	}

/* The macro to write 1 to a R/S bit and read it several times to check if the
 *  command is done
 */
#define HAL_MCR_WR_AND_WAIT(_pAdapter, _offset, _writeValue, _busyMask, _waitDelay, _waitCount, _status) \
	{ \
		UINT_32 u4Temp; \
		UINT_32 u4Count = _waitCount; \
		(_status) = FALSE; \
		HAL_MCR_WR((_pAdapter), (_offset), (_writeValue)); \
		do { \
			kalUdelay((_waitDelay)); \
			HAL_MCR_RD((_pAdapter), (_offset), &u4Temp); \
			if (!(u4Temp & (_busyMask))) { \
				(_status) = TRUE; \
				break; \
			} \
			u4Count--; \
		} while (u4Count); \
	}

#define HAL_GET_CHIP_ID_VER(_prAdapter, pu2ChipId, pu2Version) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WCIR, \
		&u4Value); \
	*pu2ChipId = (UINT_16)(u4Value & WCIR_CHIP_ID); \
	*pu2Version = (UINT_16)(u4Value & WCIR_REVISION_ID) >> 16; \
}

#define HAL_WIFI_FUNC_READY_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	UINT_32 u4Value; \
	*_pfgResult = FALSE; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WCIR, \
		&u4Value); \
	if (u4Value & WCIR_WLAN_READY) { \
		*_pfgResult = TRUE; \
	} \
} while (0)

#define HAL_WIFI_FUNC_OFF_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	UINT_32 u4Value, u4MailBoxStatus0, u4MailBoxStatus1; \
	*_pfgResult = FALSE; \
	HAL_MCR_RD(_prAdapter, MCR_WCIR, &u4Value); \
	if ((u4Value & WCIR_WLAN_READY) == 0) { \
		*_pfgResult = TRUE; \
	} \
	halGetMailbox(_prAdapter, 0, &u4MailBoxStatus0); \
	halGetMailbox(_prAdapter, 1, &u4MailBoxStatus1); \
	DBGLOG(HAL, INFO, "MailBox Status = 0x%08X, 0x%08X\n", u4MailBoxStatus0, u4MailBoxStatus1); \
} while (0)

#define HAL_WIFI_FUNC_GET_STATUS(_prAdapter, _u4Result) \
	halGetMailbox(_prAdapter, 0, &_u4Result)

#define HAL_INTR_DISABLE(_prAdapter) \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHLPCR, \
		WHLPCR_INT_EN_CLR)

#define HAL_INTR_ENABLE(_prAdapter) \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHLPCR, \
		WHLPCR_INT_EN_SET)

#define HAL_INTR_ENABLE_AND_LP_OWN_SET(_prAdapter) \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHLPCR, \
		(WHLPCR_INT_EN_SET | WHLPCR_FW_OWN_REQ_SET))

#define HAL_LP_OWN_RD(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue); \
	if (u4RegValue & WHLPCR_IS_DRIVER_OWN) { \
		*_pfgResult = TRUE; \
	} \
}

#define HAL_LP_OWN_SET(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHLPCR, \
		WHLPCR_FW_OWN_REQ_SET); \
	HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue); \
	if (u4RegValue & WHLPCR_IS_DRIVER_OWN) { \
		*_pfgResult = TRUE; \
	} \
}

#define HAL_LP_OWN_CLR(_prAdapter, _pfgResult) \
{ \
	UINT_32 u4RegValue; \
	*_pfgResult = FALSE; \
	/* Software get LP ownership */ \
	HAL_MCR_WR(_prAdapter, \
			MCR_WHLPCR, \
			WHLPCR_FW_OWN_REQ_CLR); \
	HAL_MCR_RD(_prAdapter, MCR_WHLPCR, &u4RegValue); \
	if (u4RegValue & WHLPCR_IS_DRIVER_OWN) { \
		*_pfgResult = TRUE; \
	} \
}

#define HAL_GET_ABNORMAL_INTERRUPT_REASON_CODE(_prAdapter, pu4AbnormalReason) \
{ \
	HAL_MCR_RD(_prAdapter, \
		MCR_WASR, \
		pu4AbnormalReason); \
}

#define HAL_DISABLE_RX_ENHANCE_MODE(_prAdapter) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WHCR, \
		&u4Value); \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHCR, \
		u4Value & ~WHCR_RX_ENHANCE_MODE_EN); \
}

#define HAL_ENABLE_RX_ENHANCE_MODE(_prAdapter) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WHCR, \
		&u4Value); \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHCR, \
		u4Value | WHCR_RX_ENHANCE_MODE_EN); \
}

#define HAL_CFG_MAX_HIF_RX_LEN_NUM(_prAdapter, _ucNumOfRxLen) \
{ \
	UINT_32 u4Value, ucNum; \
	ucNum = ((_ucNumOfRxLen >= 16) ? 0 : _ucNumOfRxLen); \
	u4Value = 0; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WHCR, \
		&u4Value); \
	u4Value &= ~WHCR_MAX_HIF_RX_LEN_NUM; \
	u4Value |= ((((UINT_32)ucNum) << WHCR_OFFSET_MAX_HIF_RX_LEN_NUM) & WHCR_MAX_HIF_RX_LEN_NUM); \
	HAL_MCR_WR(_prAdapter, \
		MCR_WHCR, \
		u4Value); \
}

#define HAL_SET_INTR_STATUS_READ_CLEAR(prAdapter) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(prAdapter, \
		MCR_WHCR, \
		&u4Value); \
	HAL_MCR_WR(prAdapter, \
		MCR_WHCR, \
		u4Value & ~WHCR_W_INT_CLR_CTRL); \
	prAdapter->prGlueInfo->rHifInfo.fgIntReadClear = TRUE;\
}

#define HAL_SET_INTR_STATUS_WRITE_1_CLEAR(prAdapter) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(prAdapter, \
		MCR_WHCR, \
		&u4Value); \
	HAL_MCR_WR(prAdapter, \
		MCR_WHCR, \
		u4Value | WHCR_W_INT_CLR_CTRL); \
	prAdapter->prGlueInfo->rHifInfo.fgIntReadClear = FALSE;\
}

/* Note: enhance mode structure may also carried inside the buffer,
*	 if the length of the buffer is long enough
*/
#define HAL_READ_INTR_STATUS(prAdapter, length, pvBuf) \
	HAL_PORT_RD(prAdapter, \
		MCR_WHISR, \
		length, \
		pvBuf, \
		length)

#define HAL_READ_TX_RELEASED_COUNT(_prAdapter, au2TxReleaseCount) \
{ \
	PUINT_32 pu4Value = (PUINT_32)au2TxReleaseCount; \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR0, \
		&pu4Value[0]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR1, \
		&pu4Value[1]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR2, \
		&pu4Value[2]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR3, \
		&pu4Value[3]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR4, \
		&pu4Value[4]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR5, \
		&pu4Value[5]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR6, \
		&pu4Value[6]); \
	HAL_MCR_RD(_prAdapter, \
		MCR_WTQCR7, \
		&pu4Value[7]); \
}

#define HAL_READ_RX_LENGTH(prAdapter, pu2Rx0Len, pu2Rx1Len) \
{ \
	UINT_32 u4Value; \
	u4Value = 0; \
	HAL_MCR_RD(prAdapter, \
		MCR_WRPLR, \
		&u4Value); \
	*pu2Rx0Len = (UINT_16)u4Value; \
	*pu2Rx1Len = (UINT_16)(u4Value >> 16); \
}

#define HAL_GET_INTR_STATUS_FROM_ENHANCE_MODE_STRUCT(pvBuf, u2Len, pu4Status) \
{ \
	PUINT_32 pu4Buf = (PUINT_32)pvBuf; \
	*pu4Status = pu4Buf[0]; \
}

#define HAL_GET_TX_STATUS_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4BufOut, u4LenBufOut) \
{ \
	PUINT_32 pu4Buf = (PUINT_32)pvInBuf; \
	ASSERT(u4LenBufOut >= 8); \
	pu4BufOut[0] = pu4Buf[1]; \
	pu4BufOut[1] = pu4Buf[2]; \
}

#define HAL_GET_RX_LENGTH_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu2Rx0Num, au2Rx0Len, pu2Rx1Num, au2Rx1Len) \
{ \
	PUINT_32 pu4Buf = (PUINT_32)pvInBuf; \
	ASSERT((sizeof(au2Rx0Len) / sizeof(UINT_16)) >= 16); \
	ASSERT((sizeof(au2Rx1Len) / sizeof(UINT_16)) >= 16); \
	*pu2Rx0Num = (UINT_16)pu4Buf[3]; \
	*pu2Rx1Num = (UINT_16)(pu4Buf[3] >> 16); \
	kalMemCopy(au2Rx0Len, &pu4Buf[4], 8); \
	kalMemCopy(au2Rx1Len, &pu4Buf[12], 8); \
}

#define HAL_GET_MAILBOX_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4Mailbox0, pu4Mailbox1) \
{ \
	PUINT_32 pu4Buf = (PUINT_32)pvInBuf; \
	*pu4Mailbox0 = (UINT_16)pu4Buf[21]; \
	*pu4Mailbox1 = (UINT_16)pu4Buf[22]; \
}

#define HAL_IS_TX_DONE_INTR(u4IntrStatus) \
	((u4IntrStatus & WHISR_TX_DONE_INT) ? TRUE : FALSE)

#define HAL_IS_RX_DONE_INTR(u4IntrStatus) \
	((u4IntrStatus & (WHISR_RX0_DONE_INT | WHISR_RX1_DONE_INT)) ? TRUE : FALSE)

#define HAL_IS_ABNORMAL_INTR(u4IntrStatus) \
	((u4IntrStatus & WHISR_ABNORMAL_INT) ? TRUE : FALSE)

#define HAL_IS_FW_OWNBACK_INTR(u4IntrStatus) \
	((u4IntrStatus & WHISR_FW_OWN_BACK_INT) ? TRUE : FALSE)

#define HAL_PUT_MAILBOX(prAdapter, u4MboxId, u4Data) \
{ \
	ASSERT(u4MboxId < 2); \
	HAL_MCR_WR(prAdapter, \
		((u4MboxId == 0) ? MCR_H2DSM0R : MCR_H2DSM1R), \
		u4Data); \
}

#define HAL_GET_MAILBOX(prAdapter, u4MboxId, pu4Data) \
{ \
	ASSERT(u4MboxId < 2); \
	HAL_MCR_RD(prAdapter, \
		((u4MboxId == 0) ? MCR_D2HRM0R : MCR_D2HRM1R), \
		pu4Data); \
}

#define HAL_SET_MAILBOX_READ_CLEAR(prAdapter, fgEnableReadClear) \
{ \
	UINT_32 u4Value; \
	HAL_MCR_RD(prAdapter, MCR_WHCR, &u4Value);\
	HAL_MCR_WR(prAdapter, MCR_WHCR, \
		    (fgEnableReadClear) ? \
			(u4Value | WHCR_RECV_MAILBOX_RD_CLR_EN) : \
			(u4Value & ~WHCR_RECV_MAILBOX_RD_CLR_EN)); \
	prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear = fgEnableReadClear;\
}

#define HAL_GET_MAILBOX_READ_CLEAR(prAdapter) (prAdapter->prGlueInfo->rHifInfo.fgMbxReadClear)

#define HAL_READ_INT_STATUS(prAdapter, _pu4IntStatus) \
{ \
	kalDevReadIntStatus(prAdapter, _pu4IntStatus);\
}

#define HAL_HIF_INIT(_prAdapter) \
{ \
	halDevInit(_prAdapter);\
}

#define HAL_ENABLE_FWDL(_prAdapter, _fgEnable)

#define HAL_WAKE_UP_WIFI(_prAdapter) \
{ \
	halWakeUpWiFi(_prAdapter);\
}

#endif

#if defined(_HIF_USB)
#define HAL_GET_ABNORMAL_INTERRUPT_REASON_CODE(_prAdapter, pu4AbnormalReason)

#define HAL_DISABLE_RX_ENHANCE_MODE(_prAdapter)

#define HAL_ENABLE_RX_ENHANCE_MODE(_prAdapter)

#define HAL_CFG_MAX_HIF_RX_LEN_NUM(_prAdapter, _ucNumOfRxLen)

#define HAL_SET_INTR_STATUS_READ_CLEAR(prAdapter)

#define HAL_SET_INTR_STATUS_WRITE_1_CLEAR(prAdapter)

#define HAL_READ_INTR_STATUS(prAdapter, length, pvBuf)

#define HAL_READ_TX_RELEASED_COUNT(_prAdapter, au2TxReleaseCount)

#define HAL_READ_RX_LENGTH(prAdapter, pu2Rx0Len, pu2Rx1Len)

#define HAL_GET_INTR_STATUS_FROM_ENHANCE_MODE_STRUCT(pvBuf, u2Len, pu4Status)

#define HAL_GET_TX_STATUS_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4BufOut, u4LenBufOut)

#define HAL_GET_RX_LENGTH_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu2Rx0Num, au2Rx0Len, pu2Rx1Num, au2Rx1Len)

#define HAL_GET_MAILBOX_FROM_ENHANCE_MODE_STRUCT(pvInBuf, pu4Mailbox0, pu4Mailbox1)

#define HAL_READ_TX_RELEASED_COUNT(_prAdapter, au2TxReleaseCount)

#define HAL_IS_TX_DONE_INTR(u4IntrStatus) FALSE

#define HAL_IS_RX_DONE_INTR(u4IntrStatus)

#define HAL_IS_ABNORMAL_INTR(u4IntrStatus)

#define HAL_IS_FW_OWNBACK_INTR(u4IntrStatus)

#define HAL_PUT_MAILBOX(prAdapter, u4MboxId, u4Data)

#define HAL_GET_MAILBOX(prAdapter, u4MboxId, pu4Data) TRUE

#define HAL_SET_MAILBOX_READ_CLEAR(prAdapter, fgEnableReadClear)

#define HAL_GET_MAILBOX_READ_CLEAR(prAdapter) TRUE

#define HAL_WIFI_FUNC_POWER_ON(_prAdapter) \
	mtk_usb_vendor_request(_prAdapter->prGlueInfo, 0, DEVICE_VENDOR_REQUEST_OUT, \
			       VND_REQ_POWER_ON_WIFI, 0, 1, NULL, 0)

#define HAL_WIFI_FUNC_READY_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	UINT_32 u4Value; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	*_pfgResult = FALSE; \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &u4Value); \
	if ((u4Value & (_checkItem << prChipInfo->sw_ready_bit_offset)) \
	     == (_checkItem << prChipInfo->sw_ready_bit_offset)) \
		*_pfgResult = TRUE; \
} while (0)

#define HAL_WIFI_FUNC_OFF_CHECK(_prAdapter, _checkItem, _pfgResult) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	UINT_32 u4Value; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	*_pfgResult = FALSE; \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &u4Value); \
	if ((u4Value & (_checkItem << prChipInfo->sw_ready_bit_offset)) == 0) \
		*_pfgResult = TRUE; \
} while (0)

#define HAL_WIFI_FUNC_GET_STATUS(_prAdapter, _u4Result) \
do { \
	struct mt66xx_chip_info *prChipInfo; \
	if (!_prAdapter->chip_info) \
		ASSERT(0); \
	prChipInfo = _prAdapter->chip_info; \
	HAL_MCR_RD(_prAdapter, prChipInfo->sw_sync0, &_u4Result); \
} while (0)

#define HAL_INTR_DISABLE(_prAdapter)

#define HAL_INTR_ENABLE(_prAdapter)

#define HAL_INTR_ENABLE_AND_LP_OWN_SET(_prAdapter)

#define HAL_READ_INT_STATUS(prAdapter, _pu4IntStatus) \
{ \
	halGetCompleteStatus(prAdapter, _pu4IntStatus); \
}

#define HAL_HIF_INIT(_prAdapter) \
{ \
	halDevInit(_prAdapter);\
}

#define HAL_ENABLE_FWDL(_prAdapter, _fgEnable) \
{ \
	halEnableFWDownload(_prAdapter, _fgEnable);\
}

#define HAL_WAKE_UP_WIFI(_prAdapter) \
{ \
	halWakeUpWiFi(_prAdapter);\
}

#define HAL_LP_OWN_RD(_prAdapter, _pfgResult)

#define HAL_LP_OWN_SET(_prAdapter, _pfgResult)

#define HAL_LP_OWN_CLR(_prAdapter, _pfgResult)

#define HAL_WRITE_TX_PORT(_prAdapter, _u4PortId, _u4Len, _pucBuf, _u4ValidBufSize) \
{ \
	HAL_PORT_WR(_prAdapter, \
		_u4PortId, \
		_u4Len, \
		_pucBuf, \
		_u4ValidBufSize/*temp!!*//*4KByte*/) \
}

#endif

#define INVALID_VERSION 0xFFFF /* used by HW/FW version */
/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

BOOL halVerifyChipID(IN P_ADAPTER_T prAdapter);
UINT_32 halGetChipHwVer(IN P_ADAPTER_T prAdapter);
UINT_32 halGetChipSwVer(IN P_ADAPTER_T prAdapter);

WLAN_STATUS halRxWaitResponse(IN P_ADAPTER_T prAdapter, IN UINT_8 ucPortIdx, OUT PUINT_8 pucRspBuffer,
	IN UINT_32 u4MaxRespBufferLen, OUT PUINT_32 pu4Length);

VOID halEnableInterrupt(IN P_ADAPTER_T prAdapter);
VOID halDisableInterrupt(IN P_ADAPTER_T prAdapter);

BOOLEAN halSetDriverOwn(IN P_ADAPTER_T prAdapter);
VOID halSetFWOwn(IN P_ADAPTER_T prAdapter, IN BOOLEAN fgEnableGlobalInt);

VOID halDevInit(IN P_ADAPTER_T prAdapter);
VOID halEnableFWDownload(IN P_ADAPTER_T prAdapter, IN BOOL fgEnable);
VOID halWakeUpWiFi(IN P_ADAPTER_T prAdapter);
VOID halTxCancelSendingCmd(IN P_ADAPTER_T prAdapter, IN P_CMD_INFO_T prCmdInfo);
BOOLEAN halTxIsDataBufEnough(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo);
VOID halProcessTxInterrupt(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halTxPollingResource(IN P_ADAPTER_T prAdapter, IN UINT_8 ucTC);

VOID halProcessRxInterrupt(IN P_ADAPTER_T prAdapter);
VOID halProcessSoftwareInterrupt(IN P_ADAPTER_T prAdapter);
/* Hif power off wifi */
WLAN_STATUS halHifPowerOffWifi(IN P_ADAPTER_T prAdapter);


VOID halHifSwInfoInit(IN P_ADAPTER_T prAdapter);
VOID halRxProcessMsduReport(IN P_ADAPTER_T prAdapter, IN OUT P_SW_RFB_T prSwRfb);
UINT_32 halTxGetPageCount(IN UINT_32 u4FrameLength, IN BOOLEAN fgIncludeDesc);
UINT_32 halDumpHifStatus(IN P_ADAPTER_T prAdapter, IN PUINT_8 pucBuf, IN UINT_32 u4Max);
BOOLEAN halIsPendingRx(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halAllocateIOBuffer(IN P_ADAPTER_T prAdapter);
WLAN_STATUS halReleaseIOBuffer(IN P_ADAPTER_T prAdapter);
VOID halDeAggRxPktWorker(struct work_struct *work);
#endif /* _HAL_H */
