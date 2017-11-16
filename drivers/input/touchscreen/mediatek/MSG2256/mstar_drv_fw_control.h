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
 * @file    mstar_drv_fw_control.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_FW_CONTROL_H__
#define __MSTAR_DRV_FW_CONTROL_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"
#ifdef CONFIG_ENABLE_HOTKNOT
#include "mstar_drv_hotknot.h"
#endif //CONFIG_ENABLE_HOTKNOT

/*--------------------------------------------------------------------------*/
/* COMPILE OPTION DEFINITION                                                */
/*--------------------------------------------------------------------------*/

/* The below 3 define are used for MSG21xxA/MSG22xx */
//#define CONFIG_SWAP_X_Y

//#define CONFIG_REVERSE_X
//#define CONFIG_REVERSE_Y

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define MUTUAL_DEMO_MODE_PACKET_LENGTH    (43) // for MSG26xxM/MSG28xx
#define SELF_DEMO_MODE_PACKET_LENGTH    (8) // for MSG21xxA/MSG22xx

#define MUTUAL_MAX_TOUCH_NUM           (10) // for MSG26xxM/MSG28xx    
#define SELF_MAX_TOUCH_NUM           (2) // for MSG21xxA/MSG22xx     

#define MUTUAL_DEBUG_MODE_PACKET_LENGTH    (1280) // for MSG26xxM/MSG28xx. It is a predefined maximum packet length, not the actual packet length which queried from firmware.
#define SELF_DEBUG_MODE_PACKET_LENGTH    (128) //  for MSG21xxA/MSG22xx


#define MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE (32) //32K
#define MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE (1)  //1K
#define MSG21XXA_FIRMWARE_WHOLE_SIZE (MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE+MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE) //33K

#define MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE (48)  //48K
#define MSG22XX_FIRMWARE_INFO_BLOCK_SIZE (512) //512Byte

//#define MSG21XXA_FIRMWARE_MODE_UNKNOWN_MODE   (0xFF)
#define MSG21XXA_FIRMWARE_MODE_DEMO_MODE      (0x00)
#define MSG21XXA_FIRMWARE_MODE_DEBUG_MODE     (0x01)
#define MSG21XXA_FIRMWARE_MODE_RAW_DATA_MODE  (0x02)

//#define MSG22XX_FIRMWARE_MODE_UNKNOWN_MODE    (0xFF)
#define MSG22XX_FIRMWARE_MODE_DEMO_MODE       (0x00)
#define MSG22XX_FIRMWARE_MODE_DEBUG_MODE      (0x01)
#define MSG22XX_FIRMWARE_MODE_RAW_DATA_MODE   (0x02)

#define MSG22XX_MAX_ERASE_EFLASH_TIMES   (2) // for update firmware of MSG22xx 


#define MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE (32) //32K
#define MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE (8) //8K
#define MSG26XXM_FIRMWARE_WHOLE_SIZE (MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE+MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE) //40K

#define MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE (128) //128K
#define MSG28XX_FIRMWARE_INFO_BLOCK_SIZE (2) //2K
#define MSG28XX_FIRMWARE_WHOLE_SIZE (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+MSG28XX_FIRMWARE_INFO_BLOCK_SIZE) //130K

#define MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE  (128)
#define MSG28XX_EMEM_SIZE_BYTES_ONE_WORD  (4)

#define MSG28XX_EMEM_MAIN_MAX_ADDR  (0x3FFF) //0~0x3FFF = 0x4000 = 16384 = 65536/4
#define MSG28XX_EMEM_INFO_MAX_ADDR  (0x1FF) //0~0x1FF = 0x200 = 512 = 2048/4


#define MSG26XXM_FIRMWARE_MODE_UNKNOWN_MODE (0xFFFF)
#define MSG26XXM_FIRMWARE_MODE_DEMO_MODE    (0x0005)
#define MSG26XXM_FIRMWARE_MODE_DEBUG_MODE   (0x0105)

#define MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE (0xFF)
#define MSG28XX_FIRMWARE_MODE_DEMO_MODE    (0x00)
#define MSG28XX_FIRMWARE_MODE_DEBUG_MODE   (0x01)

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#define UPDATE_FIRMWARE_RETRY_COUNT (2)
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#define FIRMWARE_GESTURE_INFORMATION_MODE_A	(0x00)
#define FIRMWARE_GESTURE_INFORMATION_MODE_B	(0x01)
#define FIRMWARE_GESTURE_INFORMATION_MODE_C	(0x02)
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct
{
    u16 nX;
    u16 nY;
} SelfTouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    SelfTouchPoint_t tPoint[2];
} SelfTouchInfo_t;

typedef struct
{
    u8 nFirmwareMode;
    u8 nLogModePacketHeader;
    u16 nLogModePacketLength;
    u8 nIsCanChangeFirmwareMode;
} SelfFirmwareInfo_t;

typedef struct
{
    u16 nId;
    u16 nX;
    u16 nY;
    u16 nP;
} MutualTouchPoint_t;

/// max 80+1+1 = 82 bytes
typedef struct
{
    u8 nCount;
    u8 nKeyCode;
    MutualTouchPoint_t tPoint[10];
} MutualTouchInfo_t;

typedef struct
{
    u16 nFirmwareMode;
    u8 nType;
    u8 nLogModePacketHeader;
    u8 nMy;
    u8 nMx;
    u8 nSd;
    u8 nSs;
    u16 nLogModePacketLength;
} MutualFirmwareInfo_t;


#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * The following is sw id enum definition for MSG21XXA.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG21XXA_SW_ID_XXXX = 0,  
    MSG21XXA_SW_ID_YYYY,
    MSG21XXA_SW_ID_UNDEFINED
} Msg21xxaSwId_e;

/*
 * Note.
 * The following is sw id enum definition for MSG22XX.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG22XX_SW_ID_XXXX = 0x0001,
    //MSG22XX_SW_ID_YYYY = 0x0002,  
    MSG22XX_SW_ID_UNDEFINED = 0xFFFF
} Msg22xxSwId_e;

/*
 * Note.
 * The following is sw id enum definition for MSG26XXM.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG26XXM_SW_ID_XXXX = 0x0001,
    MSG26XXM_SW_ID_YYYY = 0x0002,
    MSG26XXM_SW_ID_UNDEFINED = 0xFFFF
} Msg26xxmSwId_e;

/*
 * Note.
 * The following is sw id enum definition for MSG28XX.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG28XX_SW_ID_XXXX = 0x0001,
    MSG28XX_SW_ID_YYYY = 0x0002,
    MSG28XX_SW_ID_UNDEFINED = 0xFFFF
} Msg28xxSwId_e;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvFwCtrlOpenGestureWakeup(u32 *pMode);
extern void DrvFwCtrlCloseGestureWakeup(void);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern void DrvFwCtrlOpenGestureDebugMode(u8 nGestureFlag);
extern void DrvFwCtrlCloseGestureDebugMode(void);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

extern u32 DrvFwCtrlReadDQMemValue(u16 nAddr);
extern void DrvFwCtrlWriteDQMemValue(u16 nAddr, u32 nData);

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvFwCtrlCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern u16 DrvFwCtrlChangeFirmwareMode(u16 nMode);
extern void DrvFwCtrlSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo);
extern void DrvFwCtrlMutualGetFirmwareInfo(MutualFirmwareInfo_t *pInfo);
extern u16 DrvFwCtrlGetFirmwareMode(void);
extern void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void);

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
extern void DrvFwCtrlGetTouchPacketAddress(u16 *pDataAddress, u16 *pFlagAddress);
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern s32 DrvFwCtrlEnableProximity(void);
extern s32 DrvFwCtrlDisableProximity(void);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

extern void DrvFwCtrlVariableInitialize(void);
extern void DrvFwCtrlOptimizeCurrentConsumption(void);
extern u8 DrvFwCtrlGetChipType(void);
extern void DrvFwCtrlGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvFwCtrlGetPlatformFirmwareVersion(u8 **ppVersion);
extern void DrvFwCtrlHandleFingerTouch(void);
extern s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);
extern s32 DrvFwCtrlUpdateFirmwareBySdCard(const char *pFilePath);

#ifdef CONFIG_ENABLE_HOTKNOT
extern void ReportHotKnotCmd(u8 *pPacket, u16 nLength);
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_GLOVE_MODE
extern void DrvFwCtrlOpenGloveMode(void);
extern void DrvFwCtrlCloseGloveMode(void);
extern void DrvFwCtrlGetGloveInfo(u8 *pGloveMode);
#endif //CONFIG_ENABLE_GLOVE_MODE

#ifdef CONFIG_ENABLE_LEATHER_SHEATH_MODE
extern void DrvFwCtrlOpenLeatherSheathMode(void);
extern void DrvFwCtrlCloseLeatherSheathMode(void);
extern void DrvFwCtrlGetLeatherSheathInfo(u8 *pLeatherSheathMode);
#endif //CONFIG_ENABLE_LEATHER_SHEATH_MODE

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
extern void DrvFwCtrlChargerDetection(u8 nChargerStatus);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

        
#endif  /* __MSTAR_DRV_FW_CONTROL_H__ */
