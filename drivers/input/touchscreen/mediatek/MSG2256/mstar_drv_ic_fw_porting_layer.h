////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_ic_fw_porting_layer.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_IC_FW_PORTING_LAYER_H__
#define __MSTAR_DRV_IC_FW_PORTING_LAYER_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"
#include "mstar_drv_fw_control.h"
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "mstar_drv_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvIcFwLyrOpenGestureWakeup(u32 *pWakeupMode);
extern void DrvIcFwLyrCloseGestureWakeup(void);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern void DrvIcFwLyrOpenGestureDebugMode(u8 nGestureFlag);
extern void DrvIcFwLyrCloseGestureDebugMode(void);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

extern u32 DrvIcFwLyrReadDQMemValue(u16 nAddr);
extern void DrvIcFwLyrWriteDQMemValue(u16 nAddr, u32 nData);

extern u16 DrvIcFwLyrChangeFirmwareMode(u16 nMode);
extern void DrvIcFwLyrSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo);
extern void DrvIcFwLyrMutualGetFirmwareInfo(MutualFirmwareInfo_t *pInfo);
extern u16 DrvIcFwLyrGetFirmwareMode(void); // used for MSG26xxM only
extern void DrvIcFwLyrRestoreFirmwareModeToLogDataMode(void);

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvIcFwLyrCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern void DrvIcFwLyrVariableInitialize(void);
extern void DrvIcFwLyrOptimizeCurrentConsumption(void);
extern u8 DrvIcFwLyrGetChipType(void);
extern void DrvIcFwLyrGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvIcFwLyrGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvIcFwLyrGetPlatformFirmwareVersion(u8 **ppVersion);
extern void DrvIcFwLyrHandleFingerTouch(u8 *pPacket, u16 nLength);
extern u32 DrvIcFwLyrIsRegisterFingerTouchInterruptHandler(void);
extern s32 DrvIcFwLyrUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);
extern s32 DrvIcFwLyrUpdateFirmwareBySdCard(const char *pFilePath);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
extern void DrvIcFwLyrGetMpTestScope(TestScopeInfo_t *pInfo); // for MSG26xxM/MSG28xx
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

extern void DrvIcFwLyrCreateMpTestWorkQueue(void);
extern void DrvIcFwLyrScheduleMpTestWork(ItoTestMode_e eItoTestMode);
extern void DrvIcFwLyrGetMpTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength);
extern void DrvIcFwLyrGetMpTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount);
extern s32 DrvIcFwLyrGetMpTestResult(void);

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
extern void DrvIcFwLyrGetMpTestLogAll(u8 *pDataLog, u32 *pLength);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST
        
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
extern void DrvIcFwLyrGetTouchPacketAddress(u16 *pDataAddress, u16 *pFlagAddress); // for MSG26xxM/MSG28xx
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern s32 DrvIcFwLyrEnableProximity(void);
extern s32 DrvIcFwLyrDisableProximity(void);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GLOVE_MODE
extern void DrvIcFwLyrOpenGloveMode(void);
extern void DrvIcFwLyrCloseGloveMode(void);
extern void DrvIcFwLyrGetGloveInfo(u8 *pGloveMode);
#endif //CONFIG_ENABLE_GLOVE_MODE

#ifdef CONFIG_ENABLE_LEATHER_SHEATH_MODE
extern void DrvIcFwLyrOpenLeatherSheathMode(void);
extern void DrvIcFwLyrCloseLeatherSheathMode(void);
extern void DrvIcFwLyrGetLeatherSheathInfo(u8 *pLeatherSheathMode);
#endif //CONFIG_ENABLE_LEATHER_SHEATH_MODE

#endif  /* __MSTAR_DRV_IC_FW_PORTING_LAYER_H__ */
