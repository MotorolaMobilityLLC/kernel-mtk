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
 * @file    mstar_drv_fw_control.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_fw_control.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_platform_porting_layer.h"


/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern u32 SLAVE_I2C_ID_DBBUS;
extern u32 SLAVE_I2C_ID_DWI2C;

#ifdef CONFIG_TP_HAVE_KEY
extern int g_TpVirtualKey[];

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#endif //CONFIG_TP_HAVE_KEY

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern struct input_dev *g_ProximityInputDevice;
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

extern struct input_dev *g_InputDevice;
extern struct i2c_client *g_I2cClient;

extern u8 g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
extern u32 g_FwDataCount;

extern struct mutex g_Mutex;

extern u16 DEMO_MODE_PACKET_LENGTH; 
extern u16 DEBUG_MODE_PACKET_LENGTH;
extern u16 MAX_TOUCH_NUM; 

extern u16 FIRMWARE_MODE_UNKNOWN_MODE;
extern u16 FIRMWARE_MODE_DEMO_MODE;
extern u16 FIRMWARE_MODE_DEBUG_MODE;
extern u16 FIRMWARE_MODE_RAW_DATA_MODE;

extern struct kobject *g_TouchKObj;
extern u8 g_IsSwitchModeByAPK;

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;
extern u8 IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA // for MSG26xxM/MSG28xx
extern u16 g_FwPacketDataAddress;
extern u16 g_FwPacketFlagAddress;

extern u8 g_FwSupportSegment;
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern struct kobject *g_GestureKObj;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
extern u32 g_IsEnableReportRate;
extern u32 g_InterruptCount;
extern u32 g_ValidTouchCount;

extern struct timeval g_StartTime;
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

#ifdef CONFIG_ENABLE_HOTKNOT // for MSG26xxM/MSG28xx
extern u8 g_HotKnotState;
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_ITO_MP_TEST
extern u32 g_IsInMpTest;
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static u8 _gTpVendorCode[3] = {0}; // for MSG22xx/MSG28xx

static u8 _gOneDimenFwData[MSG28XX_FIRMWARE_WHOLE_SIZE*1024] = {0}; // for MSG22xx : array size = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG22XX_FIRMWARE_INFO_BLOCK_SIZE, for MSG28xx : array size = MSG28XX_FIRMWARE_WHOLE_SIZE*1024

static u8 _gFwDataBuf[MSG28XX_FIRMWARE_WHOLE_SIZE*1024] = {0}; // for update firmware from SD card

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * Please modify the name of the below .h depends on the vendor TP that you are using.
 */
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
#include "msg21xxa_xxxx_update_bin.h" // for MSG21xxA
#include "msg21xxa_yyyy_update_bin.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#include "msg22xx_xxxx_update_bin.h" // for MSG22xx msg22xx_xxxx_update_bin
//#include "msg22xx_yyyy_update_bin.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
#include "msg26xxm_xxxx_update_bin.h" // for MSG26xxM
#include "msg26xxm_yyyy_update_bin.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#include "msg28xx_xxxx_update_bin.h" // for MSG28xx
#include "msg28xx_yyyy_update_bin.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

static u32 _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
static u32 _gIsUpdateInfoBlockFirst = 0;
static struct work_struct _gUpdateFirmwareBySwIdWork;
static struct workqueue_struct *_gUpdateFirmwareBySwIdWorkQueue = NULL;

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
static u8 _gTempData[1024]; 
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#else

static u8 _gDwIicInfoData[1024]; // for MSG21xxA
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static u32 _gGestureWakeupValue[2] = {0};
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA // for MSG26xxM/MSG28xx
static u8 _gTouchPacketFlag[2] = {0};
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
static u8 _gChargerPlugIn = 0;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
static u8 _gCurrPress[SELF_MAX_TOUCH_NUM] = {0}; // for MSG21xxA/MSG22xx
static u8 _gPriorPress[SELF_MAX_TOUCH_NUM] = {0}; 
static u8 _gPrevTouchStatus = 0;

static u8 _gPreviousTouch[MUTUAL_MAX_TOUCH_NUM] = {0}; // for MSG26xxM/MSG28xx
static u8 _gCurrentTouch[MUTUAL_MAX_TOUCH_NUM] = {0};
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

static u8 _gIsDisableFinagerTouch = 0;

static u16 _gSFR_ADDR3_BYTE0_1_VALUE = 0x0000; // for MSG28xx
static u16 _gSFR_ADDR3_BYTE2_3_VALUE = 0x0000;

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

u8 g_ChipType = 0;
u8 g_DemoModePacket[MUTUAL_DEMO_MODE_PACKET_LENGTH] = {0}; // for MSG21xxA/MSG22xx : DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH, for MSG26xxM/MSG28xx : DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH

#ifdef CONFIG_ENABLE_HOTKNOT // for MSG28xx
u8 g_DemoModeHotKnotSndRetPacket[DEMO_HOTKNOT_SEND_RET_LEN] = {0};
u8 g_DebugModeHotKnotSndRetPacket[DEBUG_HOTKNOT_SEND_RET_LEN] = {0};
#endif //CONFIG_ENABLE_HOTKNOT

MutualFirmwareInfo_t g_MutualFirmwareInfo;
SelfFirmwareInfo_t g_SelfFirmwareInfo;
u8 g_LogModePacket[MUTUAL_DEBUG_MODE_PACKET_LENGTH] = {0}; // for MSG21xxA/MSG22xx : DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH, for MSG26xxM/MSG28xx : DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH
u16 g_FirmwareMode;

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_ENABLE_GESTURE_DEBUG_MODE)
u8 _gGestureWakeupPacket[GESTURE_DEBUG_MODE_PACKET_LENGTH] = {0};
#elif defined(CONFIG_ENABLE_GESTURE_INFORMATION_MODE)
u8 _gGestureWakeupPacket[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
#else
u8 _gGestureWakeupPacket[GESTURE_WAKEUP_PACKET_LENGTH] = {0}; // for MSG21xxA : packet length = DEMO_MODE_PACKET_LENGTH, for MSG22xx/MSG26xxM/MSG28xx : packet length = GESTURE_WAKEUP_PACKET_LENGTH
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
u8 g_GestureDebugFlag = 0x00;
u8 g_GestureDebugMode = 0x00;
u8 g_LogGestureDebug[GESTURE_DEBUG_MODE_PACKET_LENGTH] = {0};
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
u32 g_LogGestureInfor[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE // support at most 64 types of gesture wakeup mode
u32 g_GestureWakeupMode[2] = {0xFFFFFFFF, 0xFFFFFFFF};
#else                                              // support at most 16 types of gesture wakeup mode
u32 g_GestureWakeupMode[2] = {0x0000FFFF, 0x00000000};
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

u8 g_GestureWakeupFlag = 0;
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
u8 g_EnableTpProximity = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
u8 g_FaceClosingTp = 0; // for QCOM platform -> 1 : close to, 0 : far away 
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
u8 g_FaceClosingTp = 10; // for MTK platform -> 0 : close to, 10 : far away 
#endif
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
u8 g_ForceUpdate = 0;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
u8 g_IsHwResetByDriver = 0;
#endif //CONFIG_ENABLE_ESD_PROTECTION

u8 g_IsUpdateFirmware = 0x00;
u8 g_Msg22xxChipRevision = 0x00;

/*=============================================================*/
// LOCAL FUNCTION DECLARATION
/*=============================================================*/

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
static void _DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork);

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
static s32 _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(u8 szFwData[][1024], EmemType_e eEmemType);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static void _DrvFwCtrlCoordinate(u8 *pRawData, u32 *pTranX, u32 *pTranY);
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifndef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
static s32 _DrvFwCtrlUpdateFirmwareC33(u8 szFwData[][1024], EmemType_e eEmemType);
static s32 _DrvFwCtrlUpdateFirmwareC32(u8 szFwData[][1024], EmemType_e eEmemType);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

static s32 _DrvFwCtrlMsg22xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);
static void _DrvFwCtrlReadReadDQMemStart(void);
static void _DrvFwCtrlReadReadDQMemEnd(void);

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/
/*
static void _DrvFwCtrlEraseEmemC33(EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    // Clear pce
    RegSetLByteValue(0x1618, 0x80);

    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x1608, 0x10); //mark
    }

    RegSetLByteValue(0x1618, 0x40);
    mdelay(10);

    RegSetLByteValue(0x1618, 0x80);

    // erase trigger
    if (eEmemType == EMEM_MAIN)
    {
        RegSetLByteValue(0x160E, 0x04); //erase main
    }
    else
    {
        RegSetLByteValue(0x160E, 0x08); //erase all block
    }
}
*/
static void _DrvFwCtrlMsg22xxGetTpVendorCode(u8 *pTpVendorCode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xC1E9); // Set start address for tp vendor code on info block(Actually, start reading from 0xC1E8)
    
        // Enable burst mode
//        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        pTpVendorCode[0] = ((nRegData1 >> 8) & 0xFF);
        pTpVendorCode[1] = (nRegData2 & 0xFF);
        pTpVendorCode[2] = ((nRegData2 >> 8) & 0xFF);

        DBG(&g_I2cClient->dev, "pTpVendorCode[0] = 0x%x , %c \n", pTpVendorCode[0], pTpVendorCode[0]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[1] = 0x%x , %c \n", pTpVendorCode[1], pTpVendorCode[1]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[2] = 0x%x , %c \n", pTpVendorCode[2], pTpVendorCode[2]); 
        
        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
    }
}

static u16 _DrvFwCtrlMsg22xxGetTrimByte1(void)
{
    u16 nRegData = 0;
    u16 nTrimByte1 = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x161E, 0xBEAF); 
    RegSet16BitValue(0x1608, 0x0006); 
    RegSet16BitValue(0x160E, 0x0010); 
    RegSet16BitValue(0x1608, 0x1006); 
    RegSet16BitValue(0x1600, 0x0001); 
    RegSet16BitValue(0x160E, 0x0010);
    
    mdelay(10);
    
    RegSet16BitValue(0x1608, 0x1846);
    RegSet16BitValue(0x160E, 0x0010);
    
    mdelay(10);

    nRegData = RegGet16BitValue(0x1624);
    nRegData = nRegData & 0xFF;

    RegSet16BitValue(0x161E, 0x0000);

    nTrimByte1 = nRegData;

    DBG(&g_I2cClient->dev, "nTrimByte1 = 0x%X ***\n", nTrimByte1);
    
    return nTrimByte1;
}

static void _DrvFwCtrlMsg22xxChangeVoltage(void)
{
    u16 nTrimValue = 0;
    u16 nNewTrimValue = 0;
    u16 nTempValue = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x1840, 0xA55A);
	
    udelay(1000); // delay 1 ms

    nTrimValue = RegGet16BitValue(0x1820);

    udelay(1000); // delay 1 ms
    
    nTrimValue = nTrimValue & 0x1F;
    nTempValue = 0x1F & nTrimValue;
    nNewTrimValue = (nTempValue + 0x07);
    
    if (nNewTrimValue >= 0x20)
    {
        nNewTrimValue = nNewTrimValue - 0x20;
    }
    else
    {
        nNewTrimValue = nNewTrimValue;
    }
    
    if ((nTempValue & 0x10) != 0x10)
    {
        if (nNewTrimValue >= 0x0F && nNewTrimValue < 0x1F)
        {
            nNewTrimValue = 0x0F;
        }
        else
        {
            nNewTrimValue = nNewTrimValue;
        }
    }

    RegSet16BitValue(0x1842, nNewTrimValue);

    udelay(1000); // delay 1 ms

    RegSet16BitValueOn(0x1842, BIT5);

    udelay(1000); // delay 1 ms
}

static void _DrvFwCtrlMsg22xxRestoreVoltage(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValueOff(0x1842, BIT5);

    udelay(1000); // delay 1 ms

    RegSet16BitValue(0x1840, 0x0000);

    udelay(1000); // delay 1 ms
}

static u32 _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EmemType_e eEmemType) 
{
    u16 nCrcDown = 0;
    u32 nTimeOut = 0;
    u32 nRetVal = 0; 

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
    
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA);      

    // Set PCE high
    RegSetLByteValue(0x1618, 0x40);      

    if (eEmemType == EMEM_MAIN)
    {
        // Set start address and end address for main block
        RegSet16BitValue(0x1600, 0x0000);      
        RegSet16BitValue(0x1640, 0xBFF8);      
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Set start address and end address for info block
        RegSet16BitValue(0x1600, 0xC000);      
        RegSet16BitValue(0x1640, 0xC1F8);      
    }

    // CRC reset
    RegSet16BitValue(0x164E, 0x0001);      

    RegSet16BitValue(0x164E, 0x0000);   
    
    // Trigger CRC check
    RegSetLByteValue(0x160E, 0x20);   
    mdelay(10);
       
    while (1)
    {
        DBG(&g_I2cClient->dev, "Wait CRC down\n");

        nCrcDown = RegGet16BitValue(0x164E);
        if (nCrcDown == 2)
        {
            break;		
        }
        mdelay(10);

        if ((nTimeOut ++) > 30)
        {
            DBG(&g_I2cClient->dev, "Get CRC down failed. Timeout.\n");

            goto GetCRCEnd;
        }
    }
    
    nRetVal = RegGet16BitValue(0x1652);
    nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1650);

    GetCRCEnd:
	
    DBG(&g_I2cClient->dev, "Hardware CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static void _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(u8 szTwoDimenFwData[][1024], u8* pOneDimenFwData)
{
    u32 i, j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
    {
        if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
        {
            for (j = 0; j < 1024; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
        else // i == 48
        {
            for (j = 0; j < 512; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
    }
}

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL 
static u32 _DrvFwCtrlPointDistance(u16 nX, u16 nY, u16 nPrevX, u16 nPrevY)
{ 
    u32 nRetVal = 0;
	
    nRetVal = (((nX-nPrevX)*(nX-nPrevX))+((nY-nPrevY)*(nY-nPrevY)));
    
    return nRetVal;
}
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

static s32 _DrvFwCtrlSelfParsePacket(u8 *pPacket, u16 nLength, SelfTouchInfo_t *pInfo) // for MSG21xxA/MSG22xx
{
    u8 nCheckSum = 0;
    u32 nDeltaX = 0, nDeltaY = 0;
    u32 nX = 0;
    u32 nY = 0;
#ifdef CONFIG_SWAP_X_Y
    u32 nTempX;
    u32 nTempY;
#endif
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    static u8 nPrevTouchNum = 0; 
    static u16 szPrevX[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static u16 szPrevY[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static u8  szPrevPress[SELF_MAX_TOUCH_NUM] = {0};
    u32 i = 0;
    u16 szX[SELF_MAX_TOUCH_NUM] = {0};
    u16 szY[SELF_MAX_TOUCH_NUM] = {0};
    u16 nTemp = 0;
    u8  nChangePoints = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    u8 nCheckSumIndex = nLength-1; //Set default checksum index for demo mode

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    _gCurrPress[0] = 0;
    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (g_IsEnableReportRate == 1)
    {
        if (4294967295UL == g_InterruptCount )
        {
            g_InterruptCount = 0; // Reset count if overflow
            DBG(&g_I2cClient->dev, "g_InterruptCount reset to 0\n");
        }	

        if (g_InterruptCount == 0)
        {
            // Get start time
            do_gettimeofday(&g_StartTime);
    
            DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
        }
        
        g_InterruptCount ++;

        DBG(&g_I2cClient->dev, "g_InterruptCount = %d\n", g_InterruptCount);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            nCheckSumIndex = 7;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
        {
            nCheckSumIndex = 31;
        }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
        if (g_GestureWakeupFlag == 1)
        {
            nCheckSumIndex = nLength-1;
        }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    } //IS_FIRMWARE_DATA_LOG_ENABLED
    
    nCheckSum = DrvCommonCalculateCheckSum(&pPacket[0], nCheckSumIndex);
    DBG(&g_I2cClient->dev, "check sum : [%x] == [%x]? \n", pPacket[nCheckSumIndex], nCheckSum);

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x08 && pPacket[3] == PACKET_TYPE_ESD_CHECK_HW_RESET && pPacket[4] == 0xFF && pPacket[5] == 0xFF && pPacket[6] == 0xFF)
        {
            DBG(&g_I2cClient->dev, "ESD HwReset check : g_IsUpdateFirmware=%d, g_IsHwResetByDriver=%d\n", g_IsUpdateFirmware, g_IsHwResetByDriver);

            if (g_IsUpdateFirmware == 0
                && g_IsHwResetByDriver == 0
#ifdef CONFIG_ENABLE_ITO_MP_TEST
                && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST            	
            )
            {
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
                if (g_EnableTpProximity == 1)
                {
                    DrvFwCtrlEnableProximity();
				
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                    g_FaceClosingTp = 0; // far away for SPRD/QCOM platform
				           
                    if (g_ProximityInputDevice != NULL)
                    {
                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                        input_sync(g_ProximityInputDevice);
                    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
                    {
                        int nErr;
                        struct hwm_sensor_data tSensorData;
				
                        g_FaceClosingTp = 10; // far away for MTK platform
				
                        // map and store data to hwm_sensor_data
                        tSensorData.values[0] = DrvPlatformLyrGetTpPsData();
                        tSensorData.value_divide = 1;
                        tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        // let up layer to know
                        if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                        {
                            DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                        }
                    }
#endif               
                }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
/*
                DrvPlatformLyrFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
                input_sync(g_InputDevice);
*/
            }
            
            g_IsHwResetByDriver = 0; //Reset check flag to 0 after HwReset check

            return -1;
        }
    }
#endif //CONFIG_ENABLE_ESD_PROTECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u8 nWakeupMode = 0;
        u8 bIsCorrectFormat = 0;

        DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
        DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
            pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP)
        {
            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
        } 
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        else if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            u32 a = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
            
            for (a = 0; a < 0x80; a ++)
            {
                g_LogGestureDebug[a] = pPacket[a];
            }
            
            if (!(pPacket[5] >> 7))// LCM Light Flag = 0
            {
                nWakeupMode = 0xFE;
                DBG(&g_I2cClient->dev, "gesture debug mode LCM flag = 0\n");
            }
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        else if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_INFORMATION)
        {
            u32 a = 0;
            u32 nTmpCount = 0;
            u32 nWidth = 0;
            u32 nHeight = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;

            for (a = 0; a < 6; a ++)//header
            {
                g_LogGestureInfor[nTmpCount] = pPacket[a];
                nTmpCount++;
            }

            for (a = 6; a < 126; a = a+3)//parse packet to coordinate
            {
                u32 nTranX = 0;
                u32 nTranY = 0;
                
                _DrvFwCtrlCoordinate(&pPacket[a], &nTranX, &nTranY);
                g_LogGestureInfor[nTmpCount] = nTranX;
                nTmpCount++;
                g_LogGestureInfor[nTmpCount] = nTranY;
                nTmpCount++;
            }
            
            nWidth = (((pPacket[12] & 0xF0) << 4) | pPacket[13]); //parse width & height
            nHeight = (((pPacket[12] & 0x0F) << 8) | pPacket[14]);

            DBG(&g_I2cClient->dev, "Before convert [width,height]=[%d,%d]\n", nWidth, nHeight);

            if ((pPacket[12] == 0xFF) && (pPacket[13] == 0xFF) && (pPacket[14] == 0xFF))
            {
                nWidth = 0; 
                nHeight = 0; 
            }
            else
            {
                nWidth = (nWidth * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                nHeight = (nHeight * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "After convert [width,height]=[%d,%d]\n", nWidth, nHeight);
            }

            g_LogGestureInfor[10] = nWidth;
            g_LogGestureInfor[11] = nHeight;
            
            g_LogGestureInfor[nTmpCount] = pPacket[126]; //Dummy
            nTmpCount++;
            g_LogGestureInfor[nTmpCount] = pPacket[127]; //checksum
            nTmpCount++;
            DBG(&g_I2cClient->dev, "gesture information mode Count = %d\n", nTmpCount);
        }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        else if (g_ChipType == CHIP_TYPE_MSG21XXA && pPacket[0] == 0x52 && pPacket[1] == 0xFF && pPacket[2] == 0xFF && pPacket[3] == 0xFF && pPacket[4] == 0xFF && pPacket[6] == 0xFF)
        {
            nWakeupMode = pPacket[5];
            bIsCorrectFormat = 1;
        }
        
        if (bIsCorrectFormat) 
        {
            DBG(&g_I2cClient->dev, "nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x60:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
                    
                    DBG(&g_I2cClient->dev, "Light up screen by UP_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_UP, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_UP, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x61:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOWN_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_DOWN, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_DOWN, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x62:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by LEFT_DIRECT gesture wakeup.\n");

//                  input_report_key(g_InputDevice, KEY_LEFT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_LEFT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x63:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by RIGHT_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_RIGHT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_RIGHT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x64:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by m_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_M, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_M, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x65:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by W_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_W, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_W, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x66:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by C_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_C, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_C, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x67:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by e_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_E, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_E, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x68:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by V_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_V, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_V, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x69:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by O_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_O, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_O, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by S_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_S, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_S, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by Z_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_Z, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_Z, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE1_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE1_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER1, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER1, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE2_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE2_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER2, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER2, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE3_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE3_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER3, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER3, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
                case 0x6F:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE4_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE4_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER4, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER4, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x70:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE5_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE5_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER5, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER5, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x71:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE6_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE6_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER6, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER6, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x72:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE7_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE7_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER7, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER7, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x73:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE8_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE8_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER8, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER8, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x74:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE9_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE9_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER9, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER9, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x75:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE10_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE10_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER10, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER10, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x76:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE11_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE11_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER11, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER11, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x77:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE12_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE12_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER12, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER12, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x78:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE13_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE13_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER13, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER13, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x79:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE14_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE14_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER14, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER14, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE15_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE15_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER15, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER15, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE16_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE16_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER16, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER16, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE17_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE17_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER17, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER17, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE18_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE18_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER18, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER18, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE19_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE19_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER19, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER19, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE20_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE20_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER20, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER20, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x80:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE21_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE21_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER21, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER21, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x81:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE22_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE22_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER22, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER22, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x82:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE23_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE23_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER23, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER23, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x83:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE24_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE24_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER24, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER24, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x84:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE25_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE25_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER25, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER25, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x85:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE26_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE26_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER26, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER26, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x86:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE27_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE27_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER27, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER27, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x87:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE28_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE28_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER28, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER28, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x88:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE29_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE29_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER29, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER29, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x89:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE30_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE30_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER30, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER30, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE31_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE31_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER31, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER31, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE32_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE32_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER32, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER32, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE33_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE33_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER33, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER33, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE34_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE34_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER34, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER34, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE35_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE35_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER35, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER35, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE36_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE36_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER36, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER36, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x90:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE37_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE37_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER37, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER37, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x91:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE38_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE38_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER38, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER38, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x92:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE39_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE39_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER39, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER39, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x93:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE40_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE40_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER40, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER40, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x94:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE41_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE41_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER41, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER41, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x95:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE42_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE42_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER42, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER42, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x96:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE43_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE43_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER43, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER43, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x97:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE44_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE44_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER44, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER44, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x98:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE45_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE45_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER45, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER45, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x99:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE46_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE46_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER46, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER46, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE47_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE47_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER47, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER47, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE48_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE48_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER48, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER48, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE49_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE49_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER49, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER49, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE50_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE50_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER50, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER50, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE51_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE51_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
				case 0xFF://Gesture Fail
	            	_gGestureWakeupValue[1] = 0xFF;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_FAIL gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
                default:
                    _gGestureWakeupValue[0] = 0;
                    _gGestureWakeupValue[1] = 0;
                    DBG(&g_I2cClient->dev, "Un-supported gesture wakeup mode. Please check your device driver code.\n");
                    break;		
            }

            DBG(&g_I2cClient->dev, "_gGestureWakeupValue[0] = 0x%x\n", _gGestureWakeupValue[0]);
            DBG(&g_I2cClient->dev, "_gGestureWakeupValue[1] = 0x%x\n", _gGestureWakeupValue[1]);
        }
        else
        {
            DBG(&g_I2cClient->dev, "gesture wakeup packet format is incorrect.\n");
        }
        
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        // Notify android application to retrieve log data mode packet from device driver by sysfs.
        if (g_GestureKObj != NULL && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;

            pEnvp[0] = "STATUS=GET_GESTURE_DEBUG";
            pEnvp[1] = NULL;

            nRetVal = kobject_uevent_env(g_GestureKObj, KOBJ_CHANGE, pEnvp);
            DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        return -1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
    DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7]);

    if ((pPacket[nCheckSumIndex] == nCheckSum) && (pPacket[0] == 0x52))   // check the checksum of packet
    {
        nX = (((pPacket[1] & 0xF0) << 4) | pPacket[2]);         // parse the packet to coordinate
        nY = (((pPacket[1] & 0x0F) << 8) | pPacket[3]);

        nDeltaX = (((pPacket[4] & 0xF0) << 4) | pPacket[5]);
        nDeltaY = (((pPacket[4] & 0x0F) << 8) | pPacket[6]);

        DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
        DBG(&g_I2cClient->dev, "[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[3]:the first point abs, pPacket[4]~pPacket[6]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[4], pPacket[6] is 0xFF, keyevent, pPacket[5] to judge which key press.
         * pPacket[1]~pPacket[6] all are 0xFF, release touch
        */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[6] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[5] != 0x00) && (pPacket[5] != 0xFF)) /* pPacket[5] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d, pPacket[5] = 0x%x\n", g_EnableTpProximity, pPacket[5]);

                if (g_EnableTpProximity && ((pPacket[5] == 0x80) || (pPacket[5] == 0x40)))
                {
                    if (pPacket[5] == 0x80) // close to
                    {
                        g_FaceClosingTp = 1;

                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 0);
                        input_sync(g_ProximityInputDevice);
                    }
                    else if (pPacket[5] == 0x40) // far away
                    {
                        g_FaceClosingTp = 0;

                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                        input_sync(g_ProximityInputDevice);
                    }

                    DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);
                   
                    return -1;
                }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
                if (g_EnableTpProximity && ((pPacket[5] == 0x80) || (pPacket[5] == 0x40)))
                {
                    int nErr;
                    struct hwm_sensor_data tSensorData;

                    if (pPacket[5] == 0x80) // close to
                    {
                        g_FaceClosingTp = 0;
                    }
                    else if (pPacket[5] == 0x40) // far away
                    {
                        g_FaceClosingTp = 10;
                    }
                    
                    DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);

                    // map and store data to hwm_sensor_data
                    tSensorData.values[0] = DrvPlatformLyrGetTpPsData();
                    tSensorData.value_divide = 1;
                    tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                    // let up layer to know
                    if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                    {
                        DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                    }
                    
                    return -1;
                }
#endif               
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

                /* 0x00 is key up, 0xff is touch screen up */
                DBG(&g_I2cClient->dev, "touch key down pPacket[5]=%d\n", pPacket[5]);

                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[5];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (pPacket[5] == 4) // TOUCH_KEY_HOME
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[1].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[1].key_y;
                    }
                    else if (pPacket[5] == 2) // TOUCH_KEY_MENU
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[0].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[0].key_y;
                    }           
                    else if (pPacket[5] == 1) // TOUCH_KEY_BACK
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[2].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[2].key_y;
                    }           
                    else if (pPacket[5] == 8) // TOUCH_KEY_SEARCH 
                    {	
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[3].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[3].key_y;
                    }
                    else
                    {
                        DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                        return -1;
                    }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrPress[0] = 1;
                    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#else
                if (pPacket[5] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[5] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[5] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[5] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                    return -1;
                }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
            }
            else
            {   /* key up or touch up */
                DBG(&g_I2cClient->dev, "touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL 
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
            if (
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
                )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            if (_gPrevTouchStatus == 1)
            {
                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    szX[i] = pInfo->tPoint[i].nX;
                    szY[i] = pInfo->tPoint[i].nY;
                }
			
                if (/*(pInfo->nFingerNum == 1)&&*/(nPrevTouchNum == 2))
                {
                    if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0], szPrevY[0]) > _DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]))
                    {
                        nChangePoints = 1;
                    }
                }
                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 1))
                {
                    if (szPrevPress[0] == 1)
                    {
                        if(_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0] ,szPrevY[0]) > _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[0], szPrevY[0]))
                        {
                            nChangePoints = 1;
                        }
                    }
                    else
                    {
                        if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]) < _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[1], szPrevY[1]))
                        {
                            nChangePoints = 1;
                        }
                    }
                }
                else if ((pInfo->nFingerNum == 1) && (nPrevTouchNum == 1))
                {
                    if (_gCurrPress[0] != szPrevPress[0])
                    {
                        nChangePoints = 1;
                    }
                }
//                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 2))
//                {
//                }

                if (nChangePoints == 1)
                {
                    nTemp = _gCurrPress[0];
                    _gCurrPress[0] = _gCurrPress[1];
                    _gCurrPress[1] = nTemp;

                    nTemp = pInfo->tPoint[0].nX;
                    pInfo->tPoint[0].nX = pInfo->tPoint[1].nX;
                    pInfo->tPoint[1].nX = nTemp;

                    nTemp = pInfo->tPoint[0].nY;
                    pInfo->tPoint[0].nY = pInfo->tPoint[1].nY;
                    pInfo->tPoint[1].nY = nTemp;
                }
            }

            // Save current status
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                szPrevPress[i] = _gCurrPress[i];
                szPrevX[i] = pInfo->tPoint[i].nX;
                szPrevY[i] = pInfo->tPoint[i].nY;
            }
            nPrevTouchNum = pInfo->nFingerNum;

            _gPrevTouchStatus = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
        }
    }
    else if (pPacket[nCheckSumIndex] == nCheckSum && pPacket[0] == 0x62)
    {
        nX = ((pPacket[1] << 8) | pPacket[2]);  // Position_X
        nY = ((pPacket[3] << 8) | pPacket[4]);  // Position_Y

        nDeltaX = ((pPacket[13] << 8) | pPacket[14]); // Distance_X
        nDeltaY = ((pPacket[15] << 8) | pPacket[16]); // Distance_Y

        DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
        DBG(&g_I2cClient->dev, "[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[4]:the first point abs, pPacket[13]~pPacket[16]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[7] is 0xFF, keyevent, pPacket[8] to judge which key press.
         * pPacket[1]~pPacket[8] all are 0xFF, release touch
         */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[5] == 0xFF) && (pPacket[6] == 0xFF) && (pPacket[7] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[8] != 0x00) && (pPacket[8] != 0xFF)) /* pPacket[8] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
                DBG(&g_I2cClient->dev, "touch key down pPacket[8]=%d\n", pPacket[8]);
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[8];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (pPacket[8] == 4) // TOUCH_KEY_HOME
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[1].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[1].key_y;
                    }
                    else if (pPacket[8] == 2) // TOUCH_KEY_MENU
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[0].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[0].key_y;
                    }           
                    else if (pPacket[8] == 1) // TOUCH_KEY_BACK
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[2].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[2].key_y;
                    }           
                    else if (pPacket[8] == 8) // TOUCH_KEY_SEARCH 
                    {	
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[3].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[3].key_y;
                    }
                    else
                    {
                        DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                        return -1;
                    }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrPress[0] = 1;
                    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#else
                if (pPacket[8] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[8] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[8] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[8] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                    return -1;
                }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
            }
            else
            {   /* key up or touch up */
                DBG(&g_I2cClient->dev, "touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL 
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
            if (
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
                )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            if (_gPrevTouchStatus == 1)
            {
                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    szX[i] = pInfo->tPoint[i].nX;
                    szY[i] = pInfo->tPoint[i].nY;
                }
			
                if (/*(pInfo->nFingerNum == 1)&&*/(nPrevTouchNum == 2))
                {
                    if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0], szPrevY[0]) > _DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]))
                    {
                        nChangePoints = 1;
                    }
                }
                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 1))
                {
                    if (szPrevPress[0] == 1)
                    {
                        if(_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0] ,szPrevY[0]) > _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[0], szPrevY[0]))
                        {
                            nChangePoints = 1;
                        }
                    }
                    else
                    {
                        if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]) < _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[1], szPrevY[1]))
                        {
                            nChangePoints = 1;
                        }
                    }
                }
                else if ((pInfo->nFingerNum == 1) && (nPrevTouchNum == 1))
                {
                    if (_gCurrPress[0] != szPrevPress[0])
                    {
                        nChangePoints = 1;
                    }
                }
//                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 2))
//                {
//                }

                if (nChangePoints == 1)
                {
                    nTemp = _gCurrPress[0];
                    _gCurrPress[0] = _gCurrPress[1];
                    _gCurrPress[1] = nTemp;

                    nTemp = pInfo->tPoint[0].nX;
                    pInfo->tPoint[0].nX = pInfo->tPoint[1].nX;
                    pInfo->tPoint[1].nX = nTemp;

                    nTemp = pInfo->tPoint[0].nY;
                    pInfo->tPoint[0].nY = pInfo->tPoint[1].nY;
                    pInfo->tPoint[1].nY = nTemp;
                }
            }

            // Save current status
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                szPrevPress[i] = _gCurrPress[i];
                szPrevX[i] = pInfo->tPoint[i].nX;
                szPrevY[i] = pInfo->tPoint[i].nY;
            }
            nPrevTouchNum = pInfo->nFingerNum;

            _gPrevTouchStatus = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

            // Notify android application to retrieve log data mode packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char *pEnvp[2];
                s32 nRetVal = 0; 

                pEnvp[0] = "STATUS=GET_PACKET";  
                pEnvp[1] = NULL;  
    
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
            }
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "pPacket[0]=0x%x, pPacket[7]=0x%x, nCheckSum=0x%x\n", pPacket[0], pPacket[7], nCheckSum);

        if (pPacket[nCheckSumIndex] != nCheckSum)
        {
            DBG(&g_I2cClient->dev, "WRONG CHECKSUM\n");
            return -1;
        }

        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x52)
        {
            DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && pPacket[0] != 0x62)
        {
            DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && pPacket[0] != 0x62)
        {
            DBG(&g_I2cClient->dev, "WRONG RAW DATA MODE HEADER\n");
            return -1;
        }
    }

    return 0;
}

static s32 _DrvFwCtrlMutualParsePacket(u8 *pPacket, u16 nLength, MutualTouchInfo_t *pInfo) // for MSG26xxM/MSG28xx
{
    u32 i;
    u8 nCheckSum = 0;
    u32 nX = 0, nY = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (g_IsEnableReportRate == 1)
    {
        if (4294967295UL ==g_InterruptCount )
        {
            g_InterruptCount = 0; // Reset count if overflow
            DBG(&g_I2cClient->dev, "g_InterruptCount reset to 0\n");
        }	

        if (g_InterruptCount == 0)
        {
            // Get start time
            do_gettimeofday(&g_StartTime);
    
            DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
        }
        
        g_InterruptCount ++;

        DBG(&g_I2cClient->dev, "g_InterruptCount = %d\n", g_InterruptCount);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

    DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
    DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x pPacket[8]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7], pPacket[8]);

    nCheckSum = DrvCommonCalculateCheckSum(&pPacket[0], (nLength-1));
    DBG(&g_I2cClient->dev, "checksum : [%x] == [%x]? \n", pPacket[nLength-1], nCheckSum);

    if (pPacket[nLength-1] != nCheckSum)
    {
        DBG(&g_I2cClient->dev, "WRONG CHECKSUM\n");
        return -1;
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u8 nWakeupMode = 0;
        u8 bIsCorrectFormat = 0;

        DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
        DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP) 
        {
            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
        }
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        else if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            u32 a = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
            
            for (a = 0; a < 0x80; a ++)
            {
                g_LogGestureDebug[a] = pPacket[a];
            }

            if (!(pPacket[5] >> 7))// LCM Light Flag = 0
            {
                nWakeupMode = 0xFE;
                DBG(&g_I2cClient->dev, "gesture debug mode LCM flag = 0\n");
            }
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        else if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_INFORMATION)
        {
            u32 a = 0;
            u32 nTmpCount = 0;
            u32 nWidth = 0;
            u32 nHeight = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;

            for (a = 0; a < 6; a ++)//header
            {
                g_LogGestureInfor[nTmpCount] = pPacket[a];
                nTmpCount++;
            }

            for (a = 6; a < 126; a = a+3)//parse packet to coordinate
            {
                u32 nTranX = 0;
                u32 nTranY = 0;
				
                _DrvFwCtrlCoordinate(&pPacket[a], &nTranX, &nTranY);
                g_LogGestureInfor[nTmpCount] = nTranX;
                nTmpCount++;
                g_LogGestureInfor[nTmpCount] = nTranY;
                nTmpCount++;
            }
						
            nWidth = (((pPacket[12] & 0xF0) << 4) | pPacket[13]); //parse width & height
            nHeight = (((pPacket[12] & 0x0F) << 8) | pPacket[14]);

            DBG(&g_I2cClient->dev, "Before convert [width,height]=[%d,%d]\n", nWidth, nHeight);

            if ((pPacket[12] == 0xFF) && (pPacket[13] == 0xFF) && (pPacket[14] == 0xFF))
            {
                nWidth = 0; 
                nHeight = 0; 
            }
            else
            {
                nWidth = (nWidth * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                nHeight = (nHeight * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "After convert [width,height]=[%d,%d]\n", nWidth, nHeight);
            }

            g_LogGestureInfor[10] = nWidth;
            g_LogGestureInfor[11] = nHeight;
    
            g_LogGestureInfor[nTmpCount] = pPacket[126]; //Dummy
            nTmpCount++;
            g_LogGestureInfor[nTmpCount] = pPacket[127]; //checksum
            nTmpCount++;
            DBG(&g_I2cClient->dev, "gesture information mode Count = %d\n", nTmpCount);
        }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        if (bIsCorrectFormat)
        {
            DBG(&g_I2cClient->dev, "nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x60:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
                    
                    DBG(&g_I2cClient->dev, "Light up screen by UP_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_UP, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_UP, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x61:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOWN_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_DOWN, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_DOWN, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x62:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by LEFT_DIRECT gesture wakeup.\n");

//                  input_report_key(g_InputDevice, KEY_LEFT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_LEFT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x63:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by RIGHT_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_RIGHT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_RIGHT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x64:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by m_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_M, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_M, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x65:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by W_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_W, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_W, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x66:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by C_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_C, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_C, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x67:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by e_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_E, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_E, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x68:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by V_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_V, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_V, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x69:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by O_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_O, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_O, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by S_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_S, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_S, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by Z_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_Z, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_Z, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE1_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE1_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER1, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER1, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE2_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE2_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER2, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER2, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE3_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE3_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER3, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER3, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
                case 0x6F:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE4_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE4_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER4, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER4, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x70:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE5_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE5_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER5, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER5, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x71:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE6_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE6_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER6, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER6, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x72:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE7_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE7_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER7, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER7, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x73:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE8_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE8_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER8, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER8, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x74:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE9_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE9_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER9, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER9, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x75:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE10_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE10_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER10, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER10, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x76:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE11_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE11_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER11, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER11, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x77:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE12_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE12_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER12, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER12, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x78:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE13_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE13_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER13, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER13, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x79:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE14_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE14_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER14, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER14, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE15_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE15_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER15, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER15, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE16_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE16_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER16, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER16, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE17_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE17_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER17, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER17, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE18_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE18_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER18, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER18, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE19_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE19_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER19, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER19, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE20_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE20_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER20, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER20, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x80:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE21_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE21_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER21, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER21, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x81:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE22_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE22_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER22, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER22, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x82:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE23_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE23_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER23, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER23, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x83:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE24_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE24_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER24, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER24, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x84:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE25_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE25_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER25, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER25, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x85:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE26_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE26_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER26, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER26, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x86:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE27_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE27_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER27, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER27, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x87:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE28_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE28_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER28, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER28, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x88:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE29_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE29_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER29, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER29, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x89:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE30_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE30_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER30, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER30, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE31_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE31_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER31, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER31, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE32_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE32_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER32, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER32, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE33_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE33_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER33, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER33, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE34_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE34_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER34, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER34, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE35_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE35_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER35, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER35, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE36_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE36_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER36, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER36, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x90:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE37_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE37_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER37, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER37, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x91:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE38_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE38_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER38, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER38, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x92:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE39_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE39_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER39, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER39, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x93:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE40_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE40_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER40, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER40, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x94:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE41_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE41_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER41, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER41, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x95:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE42_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE42_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER42, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER42, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x96:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE43_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE43_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER43, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER43, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x97:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE44_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE44_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER44, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER44, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x98:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE45_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE45_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER45, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER45, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x99:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE46_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE46_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER46, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER46, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE47_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE47_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER47, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER47, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE48_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE48_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER48, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER48, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE49_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE49_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER49, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER49, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE50_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE50_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER50, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER50, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE51_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE51_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
                case 0xFF://Gesture Fail
                    _gGestureWakeupValue[1] = 0xFF;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_FAIL gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

                default:
                    _gGestureWakeupValue[0] = 0;
                    _gGestureWakeupValue[1] = 0;
                    DBG(&g_I2cClient->dev, "Un-supported gesture wakeup mode. Please check your device driver code.\n");
                    break;		
            }

            DBG(&g_I2cClient->dev, "_gGestureWakeupValue = 0x%x, 0x%x\n", _gGestureWakeupValue[0], _gGestureWakeupValue[1]);
        }
        else
        {
            DBG(&g_I2cClient->dev, "gesture wakeup packet format is incorrect.\n");
        }

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        // Notify android application to retrieve log data mode packet from device driver by sysfs.
        if (g_GestureKObj != NULL && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;

            pEnvp[0] = "STATUS=GET_GESTURE_DEBUG";
            pEnvp[1] = NULL;

            nRetVal = kobject_uevent_env(g_GestureKObj, KOBJ_CHANGE, pEnvp);
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_GESTURE_DEBUG, nRetVal = %d\n", nRetVal);
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        return -1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP


    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {    	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x5A)
        {
#ifdef CONFIG_ENABLE_HOTKNOT 
            if (pPacket[3] != HOTKNOT_PACKET_TYPE && pPacket[3] != HOTKNOT_RECEIVE_PACKET_TYPE)
#endif //CONFIG_ENABLE_HOTKNOT 
            {
                DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
                return -1;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && pPacket[0] != 0xA5 && pPacket[0] != 0xAB && (pPacket[0] != 0xA7 && pPacket[3] != PACKET_TYPE_TOOTH_PATTERN))
        {
            DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER\n");
            return -1;
        }
    }
    else
    {
        if (pPacket[0] != 0x5A)
        {
#ifdef CONFIG_ENABLE_HOTKNOT 
            if (pPacket[3] != HOTKNOT_PACKET_TYPE && pPacket[3] != HOTKNOT_RECEIVE_PACKET_TYPE)
#endif //CONFIG_ENABLE_HOTKNOT            
            {
                DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
                return -1;        
            }
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED

    // Process raw data...
    if (pPacket[0] == 0x5A)
    {
#ifdef CONFIG_ENABLE_HOTKNOT
        if (((DemoHotKnotCmdRet_t*)pPacket)->nIdentify == DEMO_PD_PACKET_IDENTIFY)
        {
            ReportHotKnotCmd(pPacket, nLength);				                       
            return -1;    //return 0 will run key procedure  
        }
        else  
#endif //CONFIG_ENABLE_HOTKNOT  
        {
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                if ((pPacket[(4*i)+1] == 0xFF) && (pPacket[(4*i)+2] == 0xFF) && (pPacket[(4*i)+3] == 0xFF))
                {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrentTouch[i] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                    
                    continue;
                }
		
                nX = (((pPacket[(4*i)+1] & 0xF0) << 4) | (pPacket[(4*i)+2]));
                nY = (((pPacket[(4*i)+1] & 0x0F) << 8) | (pPacket[(4*i)+3]));
                
                pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
                pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
                pInfo->tPoint[pInfo->nCount].nP = pPacket[4*(i+1)];
                pInfo->tPoint[pInfo->nCount].nId = i;
		
                DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
                DBG(&g_I2cClient->dev, "point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
                pInfo->nCount ++;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrentTouch[i] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

            // Notify android application to retrieve demo mode packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char *pEnvp[2];
                s32 nRetVal = 0;  

                pEnvp[0] = "STATUS=GET_DEMO_MODE_PACKET";  
                pEnvp[1] = NULL;  
            
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEMO_MODE_PACKET, nRetVal = %d\n", nRetVal);
            }
        }
    }
    else if (pPacket[0] == 0xA5 || pPacket[0] == 0xAB)
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(3*i)+4] == 0xFF) && (pPacket[(3*i)+5] == 0xFF) && (pPacket[(3*i)+6] == 0xFF))
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrentTouch[i] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                continue;
            }
		
            nX = (((pPacket[(3*i)+4] & 0xF0) << 4) | (pPacket[(3*i)+5]));
            nY = (((pPacket[(3*i)+4] & 0x0F) << 8) | (pPacket[(3*i)+6]));

            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
            DBG(&g_I2cClient->dev, "point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gCurrentTouch[i] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;  

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";  
            pEnvp[1] = NULL;  
            
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEBUG_MODE_PACKET, nRetVal = %d\n", nRetVal);
        }
    }
    else if (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN)
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(3*i)+5] == 0xFF) && (pPacket[(3*i)+6] == 0xFF) && (pPacket[(3*i)+7] == 0xFF))
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrentTouch[i] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                continue;
            }
		
            nX = (((pPacket[(3*i)+5] & 0xF0) << 4) | (pPacket[(3*i)+6]));
            nY = (((pPacket[(3*i)+5] & 0x0F) << 8) | (pPacket[(3*i)+7]));

            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
            DBG(&g_I2cClient->dev, "point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gCurrentTouch[i] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;  

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";  
            pEnvp[1] = NULL;  
            
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEBUG_MODE_PACKET, nRetVal = %d\n", nRetVal);
        }
    }
#ifdef CONFIG_ENABLE_HOTKNOT
    else if (pPacket[0] == 0xA7)
    {
        if (pPacket[3] == HOTKNOT_PACKET_TYPE || pPacket[3] == HOTKNOT_RECEIVE_PACKET_TYPE)
        {
            ReportHotKnotCmd(pPacket, nLength); 								   
            return -1;    //return 0 will run key procedure  
        }
    }
#endif //CONFIG_ENABLE_HOTKNOT


#ifdef CONFIG_TP_HAVE_KEY
    if (pPacket[0] == 0x5A)
    {
        u8 nButton = pPacket[nLength-2]; //Since the key value is stored in 0th~3th bit of variable "button", we can only retrieve 0th~3th bit of it. 

//        if (nButton)
        if (nButton != 0xFF)
        {
            DBG(&g_I2cClient->dev, "button = %x\n", nButton);

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
            DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d, pPacket[nLength-2] = 0x%x\n", g_EnableTpProximity, pPacket[nLength-2]);

            if (g_EnableTpProximity && ((pPacket[nLength-2] == 0x80) || (pPacket[nLength-2] == 0x40)))
            {
                if (pPacket[nLength-2] == 0x80) // close to
                {
                    g_FaceClosingTp = 1;

                    input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 0);
                    input_sync(g_ProximityInputDevice);
                }
                else if (pPacket[nLength-2] == 0x40) // far away
                {
                    g_FaceClosingTp = 0;

                    input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                    input_sync(g_ProximityInputDevice);
                }

                DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);
               
                return -1;
            }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
            if (g_EnableTpProximity && ((pPacket[nLength-2] == 0x80) || (pPacket[nLength-2] == 0x40)))
            {
                int nErr;
                struct hwm_sensor_data tSensorData;

                if (pPacket[nLength-2] == 0x80) // close to
                {
                    g_FaceClosingTp = 0;
                }
                else if (pPacket[nLength-2] == 0x40) // far away
                {
                    g_FaceClosingTp = 10;
                }
                
                DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);

                // map and store data to hwm_sensor_data
                tSensorData.values[0] = DrvPlatformLyrGetTpPsData();
                tSensorData.value_divide = 1;
                tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                // let up layer to know
                if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                {
                    DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                }
                
                return -1;
            }
#endif               
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

            for (i = 0; i < MAX_KEY_NUM; i ++)
            {
                if ((nButton & (1<<i)) == (1<<i))
                {
                    if (pInfo->nKeyCode == 0)
                    {
                        pInfo->nKeyCode = i;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                        DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                        if (tpd_dts_data.use_tpd_button)
                        {
                            DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, tpd_dts_data.tpd_key_local[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            pInfo->nKeyCode = 0xFF;
                            pInfo->nCount = 1;
                            pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[i].key_x;
                            pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[i].key_y;
                            pInfo->tPoint[0].nP = 1;
                            pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        }
#else
                        DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        pInfo->nKeyCode = 0xFF;
                        pInfo->nCount = 1;
                        pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[i][0];
                        pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[i][1];
                        pInfo->tPoint[0].nP = 1;
                        pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif 
                    }
                    else
                    {
                        /// if pressing multi-key => no report
                        pInfo->nKeyCode = 0xFF;
                    }
                }
            }
        }
        else
        {
            pInfo->nKeyCode = 0xFF;
        }
    }
    else if (pPacket[0] == 0xA5 || pPacket[0] == 0xAB || (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN))
    {
    		// TODO : waiting for firmware define the virtual key

        if (pPacket[0] == 0xA5)
        {
        	  // Do nothing	because of 0xA5 not define virtual key in the packet
        }
        else if (pPacket[0] == 0xAB || (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN))
        {
            u8 nButton = 0xFF;

            if (pPacket[0] == 0xAB)
            {
                nButton = pPacket[3]; // The pressed virtual key is stored in 4th byte for debug mode packet 0xAB.
            }
            else if (pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN)
            {
                nButton = pPacket[4]; // The pressed virtual key is stored in 5th byte for debug mode packet 0xA7.
            }

            if (nButton != 0xFF)
            {
                DBG(&g_I2cClient->dev, "button = %x\n", nButton);

                for (i = 0; i < MAX_KEY_NUM; i ++)
                {
                    if ((nButton & (1<<i)) == (1<<i))
                    {
                        if (pInfo->nKeyCode == 0)
                        {
                            pInfo->nKeyCode = i;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                            DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                            if (tpd_dts_data.use_tpd_button)
                            {
                                DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, tpd_dts_data.tpd_key_local[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                                pInfo->nKeyCode = 0xFF;
                                pInfo->nCount = 1;
                                pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[i].key_x;
                                pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[i].key_y;
                                pInfo->tPoint[0].nP = 1;
                                pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            }
#else
                            DBG(&g_I2cClient->dev, "key[%d]=%d ...\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            pInfo->nKeyCode = 0xFF;
                            pInfo->nCount = 1;
                            pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[i][0];
                            pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[i][1];
                            pInfo->tPoint[0].nP = 1;
                            pInfo->tPoint[0].nId = 0;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif 
                        }
                        else
                        {
                            /// if pressing multi-key => no report
                            pInfo->nKeyCode = 0xFF;
                        }
                    }
                }
            }
            else
            {
                pInfo->nKeyCode = 0xFF;
            }
        }
    }
#endif //CONFIG_TP_HAVE_KEY

    return 0;
}

static void _DrvFwCtrlStoreFirmwareData(u8 *pBuf, u32 nSize)
{
    u32 nCount = nSize / 1024;
    u32 nRemainder = nSize % 1024;
    u32 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nCount > 0) // nSize >= 1024
   	{
        for (i = 0; i < nCount; i ++)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), 1024);

            g_FwDataCount ++;
        }

        if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
        {
            DBG(&g_I2cClient->dev, "nRemainder = %d\n", nRemainder);

            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), nRemainder);

            g_FwDataCount ++;
        }
    }
    else // nSize < 1024
    {
        if (nSize > 0)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf, nSize);

            g_FwDataCount ++;
        }
    }

    DBG(&g_I2cClient->dev, "*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (pBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** buf[0] = %c ***\n", pBuf[0]);
    }
}

static u16 _DrvFwCtrlMsg21xxaGetSwId(EmemType_e eEmemType) 
{
    u16 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start MCU
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0x55;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        szDbBusTxData[1] = 0x83;
        szDbBusTxData[2] = 0x00;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    DBG(&g_I2cClient->dev, "szDbBusRxData[0,1,2,3] = 0x%x,0x%x,0x%x,0x%x\n", szDbBusRxData[0], szDbBusRxData[1], szDbBusRxData[2], szDbBusRxData[3]);

    if ((szDbBusRxData[0] >= 0x30 && szDbBusRxData[0] <= 0x39)
        &&(szDbBusRxData[1] >= 0x30 && szDbBusRxData[1] <= 0x39)
        &&(szDbBusRxData[2] >= 0x31 && szDbBusRxData[2] <= 0x39))  
    {
        nRetVal = (szDbBusRxData[0]-0x30)*100+(szDbBusRxData[1]-0x30)*10+(szDbBusRxData[2]-0x30);
    }
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}		

static u16 _DrvFwCtrlMsg22xxGetSwId(EmemType_e eEmemType) 
{
    u16 nRetVal = 0; 
    u16 nRegData1 = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
    
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#else
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA); 

    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for main block SW ID
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        RegSet16BitValue(0x1600, 0xC1EC); // Set start address for info block SW ID
    }

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0xBFF4
          Major high byte at address 0xBFF5
          
          SW ID in Info Block :
          Major low byte at address 0xC1EC
          Major high byte at address 0xC1ED
    */
    
    // Enable burst mode
//    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
//    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal = ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    // Clear burst mode
//    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static u16 _DrvFwCtrlMsg26xxmGetSwId(EmemType_e eEmemType)
{
    u16 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        szDbBusTxData[1] = 0x00;
        szDbBusTxData[2] = 0x2A;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        szDbBusTxData[1] = 0x80;
        szDbBusTxData[2] = 0x04;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0x002A
          Major high byte at address 0x002B
          
          SW ID in Info Block :
          Major low byte at address 0x8004
          Major high byte at address 0x8005
    */

    nRetVal = szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static s32 _DrvFwCtrlMsg26xxmUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    /////////////////////////
    // Erase
    /////////////////////////

    DBG(&g_I2cClient->dev, "erase 0\n");

    DrvPlatformLyrTouchDeviceResetHw(); 
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "erase 1\n");

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // Set PROGRAM password
    RegSet16BitValue(0x161A, 0xABBA); //bank:emem, addr:h000D

    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG(&g_I2cClient->dev, "erase 2\n");
    // Clear setting
    RegSetLByteValue(0x1618, 0x40); //bank:emem, addr:h000C
    
    mdelay(10);
    
    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG(&g_I2cClient->dev, "erase 3\n");
    // Trigger erase
    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x160E, 0x08); //all chip //bank:emem, addr:h0007
    }
    else
    {
        RegSetLByteValue(0x160E, 0x04); //sector //bank:emem, addr:h0007
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    mdelay(1000);
    DBG(&g_I2cClient->dev, "erase OK\n");

    /////////////////////////
    // Program
    /////////////////////////

    DBG(&g_I2cClient->dev, "program 0\n");

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "program 1\n");

    // Check_Loader_Ready: Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x1C70);

    DBG(&g_I2cClient->dev, "program 2\n");

    RegSet16BitValue(0x3CE4, 0xE38F);  //all chip
    mdelay(100);

    // Check_Loader_Ready2Program: Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    DBG(&g_I2cClient->dev, "program 3\n");

    // prepare CRC & send data
    DrvCommonCrcInitTable();

    for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++) // Main : 32KB + Info : 8KB
    {
        if (i > 31)
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
        }
        else if (i < 31)
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
            }
        }
        else ///if (i == 31)
        {
            szFwData[i][1014] = 0x5A;
            szFwData[i][1015] = 0xA5;

            for (j = 0; j < 1016; j ++)
            {
                nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
            }
        }

        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
        mdelay(100);

        // Check_Program_Done: Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        // Continue_Program
        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    DBG(&g_I2cClient->dev, "program 4\n");

    // Notify_Write_Done
    RegSet16BitValue(0x3CE4, 0x1380);
    mdelay(100);

    DBG(&g_I2cClient->dev, "program 5\n");

    // Check_CRC_Done: Polling 0x3CE4 is 0x9432
    do
    {
       nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    DBG(&g_I2cClient->dev, "program 6\n");
    DBG(&g_I2cClient->dev, "program OK\n");

    // check CRC
    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    // read CRC from TP
    nCrcMainTp = RegGet16BitValue(0x3C80);
    nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
    nCrcInfoTp = RegGet16BitValue(0x3CA0);
    nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);

   // DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n",
              // nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);
    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
    {
        DBG(&g_I2cClient->dev, "Update FAILED\n");

        return -1;
    }

    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

static void _DrvFwCtrlMsg28xxConvertFwDataTwoDimenToOneDimen(u8 szTwoDimenFwData[][1024], u8* pOneDimenFwData)
{
    u32 i, j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
    {
        for (j = 0; j < 1024; j ++)
        {
            pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
        }
    }
}

static u32 _DrvFwCtrlMsg28xxCalculateCrc(u8 *pFwData, u32 nOffset, u32 nSize)
{
    u32 i;
    u32 nData = 0, nCrc = 0;
    u32 nCrcRule = 0x0C470C06; // 0000 1100 0100 0111 0000 1100 0000 0110

    for (i = 0; i < nSize; i += 4)
    {
   	    nData = (pFwData[nOffset+i]) | (pFwData[nOffset+i+1] << 8) | (pFwData[nOffset+i+2] << 16) | (pFwData[nOffset+i+3] << 24);
   	    nCrc = (nCrc >> 1) ^ (nCrc << 1) ^ (nCrc & nCrcRule) ^ nData;
    }
    
    return nCrc;
}

static void _DrvFwCtrlMsg28xxAccessEFlashInit(void)
{
    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Disable cpu read flash
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1608, 0x20);

    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);
}

static void _DrvFwCtrlMsg28xxIspBurstWriteEFlashStart(u16 nStartAddr, u8 *pFirstData, u32 nBlockSize, u16 nPageNum, EmemType_e eEmemType)
{
    u16 nWriteAddr = nStartAddr/4;
    u8  szDbBusTxData[3] = {0};
    
    DBG(&g_I2cClient->dev, "*** %s() nStartAddr = 0x%x, nBlockSize = %d, nPageNum = %d, eEmemType = %d ***\n", __func__, nStartAddr, nBlockSize, nPageNum, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    
    // Set e-flash mode to page write mode
    RegSet16BitValue(0x1606, 0x0080);

    // Set data align
    RegSetLByteValue(0x1640, 0x01);
    
    if (eEmemType == EMEM_INFO) 
    {
        RegSetLByteValue(0x1607, 0x08);
    }
    
    // Set double buffer
    RegSetLByteValue(0x1604, 0x01);
        
    // Set page write number
    RegSet16BitValue(0x161A, nPageNum);

    // Set e-flash mode trigger(Trigger write mode)
    RegSetLByteValue(0x1606, 0x81);

    // Set init data
    RegSetLByteValue(0x1602, pFirstData[0]);
    RegSetLByteValue(0x1602, pFirstData[1]);
    RegSetLByteValue(0x1602, pFirstData[2]);
    RegSetLByteValue(0x1602, pFirstData[3]);

    // Set initial address(for latch SA, CA)
    RegSet16BitValue(0x1600, nWriteAddr);

    // Set initial address(for latch PA)
    RegSet16BitValue(0x1600, nWriteAddr);
    
    // Enable burst mode
    RegSetLByteValue(0x1608, 0x21);
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);

    szDbBusTxData[0] = 0x20;
//    szDbBusTxData[1] = 0x00;
//    szDbBusTxData[2] = 0x00;    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);
}	

static void _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(u8 *pBufferData, u32 nLength)
{
    u32 i;
    u8  szDbBusTxData[3+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = {0};

    DBG(&g_I2cClient->dev, "*** %s() nLength = %d ***\n", __func__, nLength);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;
        
    for (i = 0; i < nLength; i ++)
    {
        szDbBusTxData[3+i] = pBufferData[i];
    }
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+nLength);
}	

static void _DrvFwCtrlMsg28xxIspBurstWriteEFlashEnd(void)
{
    u8 szDbBusTxData[1] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    szDbBusTxData[0] = 0x21;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    szDbBusTxData[0] = 0x7E;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    // Clear burst mode
    RegSetLByteValue(0x1608, 0x20);
}

static void _DrvFwCtrlMsg28xxWriteEFlashStart(u16 nStartAddr, u8 *pFirstData, EmemType_e eEmemType)
{
    u16 nWriteAddr = nStartAddr/4;
    
    DBG(&g_I2cClient->dev, "*** %s() nStartAddr = 0x%x, eEmemType = %d ***\n", __func__, nStartAddr, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    
    // Set e-flash mode to write mode
    RegSet16BitValue(0x1606, 0x0040);

    // Set data align
    RegSetLByteValue(0x1640, 0x01);
    
    if (eEmemType == EMEM_INFO) 
    {
        RegSetLByteValue(0x1607, 0x08);
    }

    // Set double buffer
    RegSetLByteValue(0x1604, 0x01);
        
    // Set e-flash mode trigger(Trigger write mode)
    RegSetLByteValue(0x1606, 0x81);

    // Set init data
    RegSetLByteValue(0x1602, pFirstData[0]);
    RegSetLByteValue(0x1602, pFirstData[1]);
    RegSetLByteValue(0x1602, pFirstData[2]);
    RegSetLByteValue(0x1602, pFirstData[3]);

    // Set initial address(for latch SA, CA)
    RegSet16BitValue(0x1600, nWriteAddr);

    // Set initial address(for latch PA)
    RegSet16BitValue(0x1600, nWriteAddr);
}	

static void _DrvFwCtrlMsg28xxWriteEFlashDoWrite(u16 nStartAddr, u8 *pBufferData)
{
    u16 nWriteAddr = nStartAddr/4;

    DBG(&g_I2cClient->dev, "*** %s() nWriteAddr = %d ***\n", __func__, nWriteAddr);

    // Write data
    RegSetLByteValue(0x1602, pBufferData[0]);
    RegSetLByteValue(0x1602, pBufferData[1]);
    RegSetLByteValue(0x1602, pBufferData[2]);
    RegSetLByteValue(0x1602, pBufferData[3]);

    // Set address
    RegSet16BitValue(0x1600, nWriteAddr);
}	

static void _DrvFwCtrlMsg28xxWriteEFlashEnd(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Do nothing
}

static void _DrvFwCtrlMsg28xxReadEFlashStart(u16 nStartAddr, EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);

    RegSetLByteValue(0x1606, 0x02);

    RegSet16BitValue(0x1600, nStartAddr);
    
    if (eEmemType == EMEM_MAIN)
    {
        // Set main block
        RegSetLByteValue(0x1607, 0x00);

        // Set main double buffer
        RegSetLByteValue(0x1604, 0x01);

        // Set e-flash mode to read mode for main
        RegSet16BitValue(0x1606, 0x0001);
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Set info block 
        RegSetLByteValue(0x1607, 0x08);
        
        // Set info double buffer
        RegSetLByteValue(0x1604, 0x01);
        
        // Set e-flash mode to read mode for info
        RegSet16BitValue(0x1606, 0x0801);
    }
}

static void _DrvFwCtrlMsg28xxReadEFlashDoRead(u16 nReadAddr, u8 *pReadData)
{
    u16 nRegData1 = 0, nRegData2 = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() nReadAddr = 0x%x ***\n", __func__, nReadAddr);

    // Set read address
    RegSet16BitValue(0x1600, nReadAddr);

    // Read 16+16 bits
    nRegData1 = RegGet16BitValue(0x160A);
    nRegData2 = RegGet16BitValue(0x160C);

    pReadData[0] = nRegData1 & 0xFF;
    pReadData[1] = (nRegData1 >> 8) & 0xFF;
    pReadData[2] = nRegData2 & 0xFF;
    pReadData[3] = (nRegData2 >> 8) & 0xFF;
}	

static void _DrvFwCtrlMsg28xxReadEFlashEnd(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Set read done
    RegSetLByteValue(0x1606, 0x02);

    // Unset info flag
    RegSetLByteValue(0x1607, 0x00);

    // Clear address
    RegSet16BitValue(0x1600, 0x0000);
}

/*
static void _DrvFwCtrlMsg28xxGetTpVendorCode(u8 *pTpVendorCode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        u16 nReadAddr = 0;
        u8  szTmpData[4] = {0};

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        _DrvFwCtrlMsg28xxReadEFlashStart(0x81FA, EMEM_INFO);
        nReadAddr = 0x81FA;

        _DrvFwCtrlMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

        DBG(&g_I2cClient->dev, "szTmpData[0] = 0x%x\n", szTmpData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[1] = 0x%x\n", szTmpData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[2] = 0x%x\n", szTmpData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[3] = 0x%x\n", szTmpData[3]); // add for debug
   
        _DrvFwCtrlMsg28xxReadEFlashEnd();

        pTpVendorCode[0] = szTmpData[1];
        pTpVendorCode[1] = szTmpData[2];
        pTpVendorCode[2] = szTmpData[3];

        DBG(&g_I2cClient->dev, "pTpVendorCode[0] = 0x%x , %c \n", pTpVendorCode[0], pTpVendorCode[0]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[1] = 0x%x , %c \n", pTpVendorCode[1], pTpVendorCode[1]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[2] = 0x%x , %c \n", pTpVendorCode[2], pTpVendorCode[2]); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
}
*/

static void _DrvFwCtrlMsg28xxGetSfrAddr3Value(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);

    // Set e-flash mode to read mode
    RegSetLByteValue(0x1606, 0x01);
    RegSetLByteValue(0x1610, 0x01);
    RegSetLByteValue(0x1607, 0x20);

    // Set read address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);

    _gSFR_ADDR3_BYTE0_1_VALUE = RegGet16BitValue(0x160A);
    _gSFR_ADDR3_BYTE2_3_VALUE = RegGet16BitValue(0x160C);

    DBG(&g_I2cClient->dev, "_gSFR_ADDR3_BYTE0_1_VALUE = 0x%4X, _gSFR_ADDR3_BYTE2_3_VALUE = 0x%4X\n", _gSFR_ADDR3_BYTE0_1_VALUE, _gSFR_ADDR3_BYTE2_3_VALUE);
}

static void _DrvFwCtrlMsg28xxUnsetProtectBit(void)
{
    u8 nB0, nB1, nB2, nB3;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    _DrvFwCtrlMsg28xxGetSfrAddr3Value();
    
    nB0 = _gSFR_ADDR3_BYTE0_1_VALUE & 0xFF;
    nB1 = (_gSFR_ADDR3_BYTE0_1_VALUE & 0xFF00) >> 8;

    nB2 = _gSFR_ADDR3_BYTE2_3_VALUE & 0xFF;
    nB3 = (_gSFR_ADDR3_BYTE2_3_VALUE & 0xFF00) >> 8;

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    nB2 = nB2 & 0xBF; // 10111111
    nB3 = nB3 & 0xFC; // 11111100

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1610, 0x80);
    RegSetLByteValue(0x1607, 0x10);

    // Trigger SFR write
    RegSetLByteValue(0x1606, 0x01);

    // Set write data
    RegSetLByteValue(0x1602, nB0);
    RegSetLByteValue(0x1602, nB1);
    RegSetLByteValue(0x1602, nB2);
    RegSetLByteValue(0x1602, nB3);

    // Set write address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);

    // Set TM mode = 0
    RegSetLByteValue(0x1607, 0x00);

#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    RegSetLByteValue(0x1606, 0x01);
    RegSetLByteValue(0x1606, 0x20);
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
}

static void _DrvFwCtrlMsg28xxSetProtectBit(void)
{
    u8 nB0, nB1, nB2, nB3;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nB0 = _gSFR_ADDR3_BYTE0_1_VALUE & 0xFF;
    nB1 = (_gSFR_ADDR3_BYTE0_1_VALUE & 0xFF00) >> 8;

    nB2 = _gSFR_ADDR3_BYTE2_3_VALUE & 0xFF;
    nB3 = (_gSFR_ADDR3_BYTE2_3_VALUE & 0xFF00) >> 8;

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1610, 0x80);
    RegSetLByteValue(0x1607, 0x10);

    // Trigger SFR write
    RegSetLByteValue(0x1606, 0x01);

    // Set write data
    RegSetLByteValue(0x1602, nB0);
    RegSetLByteValue(0x1602, nB1);
    RegSetLByteValue(0x1602, nB2);
    RegSetLByteValue(0x1602, nB3);

    // Set write address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);
    RegSetLByteValue(0x1606, 0x02);
}

static void _DrvFwCtrlMsg28xxEraseEmem(EmemType_e eEmemType)
{
    u32 nInfoAddr = 0x20;
    u32 nTimeOut = 0;
    u8 nRegData = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "Erase start\n");

    _DrvFwCtrlMsg28xxAccessEFlashInit();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01);

    // Set PROGRAM erase password
    RegSet16BitValue(0x1618, 0x5AA5);
    
    _DrvFwCtrlMsg28xxUnsetProtectBit();

    if (eEmemType == EMEM_MAIN) // 128KB
    {
        DBG(&g_I2cClient->dev, "Erase main block\n");

        // Set main block
        RegSetLByteValue(0x1607, 0x00);

        // Set e-flash mode to erase mode
        RegSetLByteValue(0x1606, 0xC0);

        // Set page erase main
        RegSetLByteValue(0x1607, 0x03);

        // e-flash mode trigger
        RegSetLByteValue(0x1606, 0xC1);

        nTimeOut = 0;
        while (1) // Wait erase done
        {
            nRegData = RegGetLByteValue(0x160E);
            nRegData = (nRegData & BIT3);
            
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT3)
            {
                break;
            }

            mdelay(10);
            
            if ((nTimeOut ++) > 10)
            {
                DBG(&g_I2cClient->dev, "Erase main block failed. Timeout.\n");

                goto EraseEnd;
            }
        }
    }
    else if (eEmemType == EMEM_INFO) // 2KB
    {
        DBG(&g_I2cClient->dev, "Erase info block\n");
        
        // Set info block
        RegSetLByteValue(0x1607, 0x08);

        // Set info double buffer
        RegSetLByteValue(0x1604, 0x01);

        // Set e-flash mode to erase mode
        RegSetLByteValue(0x1606, 0xC0);

        // Set page erase info
        RegSetLByteValue(0x1607, 0x09);
        
        for (nInfoAddr = 0x20; nInfoAddr <= MSG28XX_EMEM_INFO_MAX_ADDR; nInfoAddr += 0x20)
        {
            DBG(&g_I2cClient->dev, "nInfoAddr = 0x%x\n", nInfoAddr); // add for debug

            // Set address
            RegSet16BitValue(0x1600, nInfoAddr);

            // e-flash mode trigger
            RegSetLByteValue(0x1606, 0xC1);

            nTimeOut = 0;
            while (1) // Wait erase done
            {
                nRegData = RegGetLByteValue(0x160E);
                nRegData = (nRegData & BIT3);

                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);
            
                if (nRegData == BIT3)
                {
                    break;
                }

                mdelay(10);
            
                if ((nTimeOut ++) > 10)
                {
                    DBG(&g_I2cClient->dev, "Erase info block failed. Timeout.\n");

                    // Set main block
                    RegSetLByteValue(0x1607, 0x00);

                    goto EraseEnd;
                }
            }        	
        }

        // Set main block
        RegSetLByteValue(0x1607, 0x00);
    }
    
    EraseEnd:
    
    _DrvFwCtrlMsg28xxSetProtectBit();

    RegSetLByteValue(0x1606, 0x00);
    RegSetLByteValue(0x1607, 0x00);
		
    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);

    DBG(&g_I2cClient->dev, "Erase end\n");
    
    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static void _DrvFwCtrlMsg28xxProgramEmem(EmemType_e eEmemType)
{
    u32 i, j;
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
    u32 k;
#endif
    u32 nPageNum = 0, nLength = 0, nIndex = 0, nWordNum = 0;
    u32 nRetryTime = 0;
    u8  nRegData = 0;
    u8  szFirstData[MSG28XX_EMEM_SIZE_BYTES_ONE_WORD] = {0};
    u8  szBufferData[MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = {0};
#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    u8  szWriteData[3+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*2] = {0};
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "Program start\n");

    _DrvFwCtrlMsg28xxAccessEFlashInit();
    
    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); 
    
    // Set PROGRAM erase password
    RegSet16BitValue(0x1618, 0x5AA5);

    _DrvFwCtrlMsg28xxUnsetProtectBit();
    
    if (eEmemType == EMEM_MAIN) // Program main block
    {
        DBG(&g_I2cClient->dev, "Program main block\n");

#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128*1024/128=1024

        // Set ISP mode
        RegSet16BitValueOn(0x1EBE, BIT15);

        RegSetLByteValue(0x1604, 0x01);
        
        RegSet16BitValue(0x161A, nPageNum);
        RegSet16BitValue(0x1600, 0x0000); // Set initial address
        RegSet16BitValueOn(0x3C00, BIT0); // Disable INT GPIO mode
        RegSet16BitValueOn(0x1EA0, BIT1); // Set ISP INT enable
        RegSet16BitValue(0x1E34, 0x0000); // Set DQMem start address

        _DrvFwCtrlReadReadDQMemStart();
        
        szWriteData[0] = 0x10;
        szWriteData[1] = 0x00;
        szWriteData[2] = 0x00;
        
        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*2; //128*2=256
        
        for (j = 0; j < nLength; j ++)
        {
            szWriteData[3+j] = g_FwData[0][j];
        }
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); // Write the first two pages(page 0 & page 1)
        
        _DrvFwCtrlReadReadDQMemEnd();

        RegSet16BitValueOn(0x1EBE, BIT15); // Set ISP mode
        RegSet16BitValueOn(0x1608, BIT0); // Set burst mode
        RegSet16BitValueOn(0x161A, BIT13); // Set ISP trig

        udelay(2000); // delay about 2ms

        _DrvFwCtrlReadReadDQMemStart();

        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        for (i = 2; i < nPageNum; i ++) 
        {
            if (i == 2)
            {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x00;

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); 
            }
            else if (i == (nPageNum - 1))
            {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x80;

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                szWriteData[3+128] = 0xFF;
                szWriteData[3+129] = 0xFF;
                szWriteData[3+130] = 0xFF;
                szWriteData[3+131] = 0xFF;

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength+4); 
            }           
            else
            {
//                szWriteData[0] = 0x10;
//                szWriteData[1] = 0x00;
                if (szWriteData[2] == 0x00)
                {
                    szWriteData[2] = 0x80;
                }
                else // szWriteData[2] == 0x80
                {
                		szWriteData[2] = 0x00;
                }

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); 
            }
        }

        _DrvFwCtrlReadReadDQMemEnd();

#else

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 8; // 128*1024/8=16384 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 32; // 128*1024/32=4096 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128*1024/128=1024
#endif 
     
        nIndex = 0;
        
        for (i = 0; i < nPageNum; i ++) 
        {
            if (i == 0)
            {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

                szFirstData[0] = g_FwData[0][0];
                szFirstData[1] = g_FwData[0][1];
                szFirstData[2] = g_FwData[0][2];
                szFirstData[3] = g_FwData[0][3];
            
                _DrvFwCtrlMsg28xxIspBurstWriteEFlashStart(nIndex, &szFirstData[0], MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024, nPageNum, EMEM_MAIN);

                nIndex += nLength;
            
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 4 = 8 - 4 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 28 = 32 - 4 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 124 = 128 - 4
#endif

                for (j = 0; j < nLength; j ++)
                {
                    szBufferData[j] = g_FwData[0][4+j];
                }
            }
            else
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8; 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32; 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128
#endif 

                for (j = 0; j < nLength; j ++)
                {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                    szBufferData[j] = g_FwData[i/128][8*(i-(128*(i/128)))+j]; 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                    szBufferData[j] = g_FwData[i/32][32*(i-(32*(i/32)))+j]; 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                    szBufferData[j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
#endif
                }
            }

            _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], nLength);
            
            udelay(2000); // delay about 2ms
        
            nIndex += nLength;
        }

        _DrvFwCtrlMsg28xxIspBurstWriteEFlashEnd();

        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);

        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "main block can't wait write to done.\n");

                goto ProgramEnd;
            }
        }
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    }
    else if (eEmemType == EMEM_INFO) // Program info block
    {
        DBG(&g_I2cClient->dev, "Program info block\n");

        nPageNum = (MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 2*1024/128=16

        nIndex = 0;
        nIndex += MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        // Skip firt page(page 0) & Update page 1~14 by isp burst write mode
        for (i = 1; i < (nPageNum - 1); i ++) // Skip the first 128 byte and the last 128 byte of info block
        {
            if (i == 1)
            {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;
            
                szFirstData[0] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+1];
                szFirstData[2] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+2];
                szFirstData[3] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+3];
            
                _DrvFwCtrlMsg28xxIspBurstWriteEFlashStart(nIndex, &szFirstData[0], MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024, nPageNum-1, EMEM_INFO);
            
                nIndex += nLength;
            
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 124 = 128 - 4

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                for (j = 0; j < (nLength/8); j ++) // 124/8 = 15
                {
                    for (k = 0; k < 8; k ++)
                    {
                        szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(8*j)+k];
                    }
                    
                    _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                    udelay(2000); // delay about 2ms 
                }

                for (k = 0; k < (nLength%8); k ++) // 124%8 = 4
                {
                    szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(8*j)+k];
                }

                _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], (nLength%8)); // 124%8 = 4

                udelay(2000); // delay about 2ms 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                for (j = 0; j < (nLength/32); j ++) // 124/8 = 3
                {
                    for (k = 0; k < 32; k ++)
                    {
                        szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(32*j)+k];
                    }
                    
                    _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                    udelay(2000); // delay about 2ms 
                }

                for (k = 0; k < (nLength%32); k ++) // 124%32 = 28
                {
                    szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(32*j)+k];
                }

                _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], (nLength%32)); // 124%8 = 28

                udelay(2000); // delay about 2ms 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                for (j = 0; j < nLength; j ++)
                {
                    szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+j];
                }
#endif
            }
            else
            {
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < (nLength/8); j ++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+(8*j)+k];
                        }
                    
                        _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                        udelay(2000); // delay about 2ms 
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < (nLength/8); j ++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+(8*j)+k];
                        }
                    
                        _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                        udelay(2000); // delay about 2ms 
                    }
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < (nLength/32); j ++) // 128/32 = 4
                    {
                        for (k = 0; k < 32; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+(32*j)+k];
                        }
                    
                        _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                        udelay(2000); // delay about 2ms 
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < (nLength/32); j ++) // 128/32 = 4
                    {
                        for (k = 0; k < 32; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+(32*j)+k];
                        }
                    
                        _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                        udelay(2000); // delay about 2ms 
                    }
                }
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < nLength; j ++)
                    {
                        szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+j];
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < nLength; j ++)
                    {
                        szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+j];
                    }
                }
#endif
            }
        
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
            // Do nothing here
#else
            _DrvFwCtrlMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], nLength);

            udelay(2000); // delay about 2ms 
#endif        
            nIndex += nLength;
        }

        _DrvFwCtrlMsg28xxIspBurstWriteEFlashEnd();
    
        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);
            
        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "Info block page 1~14 can't wait write to done.\n");

                goto ProgramEnd;
            }
        }

        RegSet16BitValueOff(0x1EBE, BIT15); 

        // Update page 15 by write mode
        nIndex = 15 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;
        nWordNum = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE / MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 128/4=32
        nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

        for (i = 0; i < nWordNum; i ++) 
        {
            if (i == 0)
            {
                szFirstData[0] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+1];
                szFirstData[2] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+2];
                szFirstData[3] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+3];

                _DrvFwCtrlMsg28xxWriteEFlashStart(nIndex, &szFirstData[0], EMEM_INFO);
            }
            else
            {
                for (j = 0; j < nLength; j ++)
                {
                    szFirstData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+(4*i)+j];
                }

                _DrvFwCtrlMsg28xxWriteEFlashDoWrite(nIndex, &szFirstData[0]);            			
            }

            udelay(2000); // delay about 2ms 

            nIndex += nLength;
        }
        
        _DrvFwCtrlMsg28xxWriteEFlashEnd();

        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);
            
        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "Info block page 15 can't wait write to done.\n");
                
                goto ProgramEnd;
            }
        }
    }

    ProgramEnd:
    
    _DrvFwCtrlMsg28xxSetProtectBit();

    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);

    DBG(&g_I2cClient->dev, "Program end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static u16 _DrvFwCtrlMsg28xxGetSwId(EmemType_e eEmemType) 
{
    u16 nRetVal = 0; 
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        _DrvFwCtrlMsg28xxReadEFlashStart(0x7FFD, EMEM_MAIN); 
        nReadAddr = 0x7FFD;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        _DrvFwCtrlMsg28xxReadEFlashStart(0x81FB, EMEM_INFO);
        nReadAddr = 0x81FB;
    }

    _DrvFwCtrlMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

    _DrvFwCtrlMsg28xxReadEFlashEnd();

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0x7FFD
          
          SW ID in Info Block :
          Major low byte at address 0x81FB
    */

    nRetVal = (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static u32 _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u32 nRetryTime = 0;
    u32 nCrcEndAddr = 0;
    u16 nCrcDown = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    _DrvFwCtrlMsg28xxAccessEFlashInit();

    if (eEmemType == EMEM_MAIN)
    {
        // Disable cpu read flash
        RegSetLByteValue(0x1608, 0x20);
        RegSetLByteValue(0x1606, 0x20);
        
        // Set read flag
        RegSet16BitValue(0x1610, 0x0001);
    		
        // Mode reset main block
        RegSet16BitValue(0x1606, 0x0000);

        // CRC reset
        RegSet16BitValue(0x1620, 0x0002);

        RegSet16BitValue(0x1620, 0x0000);
        
        // Set CRC e-flash block start address => Main Block : 0x0000 ~ 0x7FFE
        RegSet16BitValue(0x1600, 0x0000);
        
        nCrcEndAddr = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)/4-2;

        RegSet16BitValue(0x1622, nCrcEndAddr);
        
        // Trigger CRC check
        RegSet16BitValue(0x1620, 0x0001);
        
        nCrcDown = RegGet16BitValue(0x1620);
        
        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0)
        {
            mdelay(10);

            nCrcDown = RegGet16BitValue(0x1620);
            nRetryTime ++;
            
            if (nRetryTime > 30)
            {
                DBG(&g_I2cClient->dev, "Wait main block nCrcDown failed.\n");
                break;
            }
        }
        
        nRetVal = RegGet16BitValue(0x1626);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1624);
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Disable cpu read flash
        RegSetLByteValue(0x1608, 0x20);
        RegSetLByteValue(0x1606, 0x20);
        
        // Set read flag
        RegSet16BitValue(0x1610, 0x0001);
    		
        // Mode reset info block
        RegSet16BitValue(0x1606, 0x0800);

        RegSetLByteValue(0x1604, 0x01);

        // CRC reset
        RegSet16BitValue(0x1620, 0x0002);

        RegSet16BitValue(0x1620, 0x0000);
        
        // Set CRC e-flash block start address => Info Block : 0x0020 ~ 0x01FE
        RegSet16BitValue(0x1600, 0x0020);
        RegSet16BitValue(0x1622, 0x01FE);

        // Trigger CRC check
        RegSet16BitValue(0x1620, 0x0001);
        
        nCrcDown = RegGet16BitValue(0x1620);
        
        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0)
        {
            mdelay(10);

            nCrcDown = RegGet16BitValue(0x1620);
            nRetryTime ++;
            
            if (nRetryTime > 30)
            {
                DBG(&g_I2cClient->dev, "Wait info block nCrcDown failed.\n");
                break;
            }
        }
        
        nRetVal = RegGet16BitValue(0x1626);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1624);
    }

    DBG(&g_I2cClient->dev, "Hardware CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    if (eEmemType == EMEM_MAIN) // Read main block CRC(128KB-4) from main block
    {
        _DrvFwCtrlMsg28xxReadEFlashStart(0x7FFF, EMEM_MAIN);
        nReadAddr = 0x7FFF;
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(2KB-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-4) from info block
    {
        _DrvFwCtrlMsg28xxReadEFlashStart(0x81FF, EMEM_INFO);
        nReadAddr = 0x81FF;
    }

    _DrvFwCtrlMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

    DBG(&g_I2cClient->dev, "szTmpData[0] = 0x%x\n", szTmpData[0]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[1] = 0x%x\n", szTmpData[1]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[2] = 0x%x\n", szTmpData[2]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[3] = 0x%x\n", szTmpData[3]); // add for debug
   
    _DrvFwCtrlMsg28xxReadEFlashEnd();

    nRetVal = (szTmpData[3] << 24);
    nRetVal |= (szTmpData[2] << 16);
    nRetVal |= (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[][1024], EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) 
        {
            nRetVal = szTmpBuf[127][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1020];
        }
        else if (eEmemType == EMEM_INFO) 
        {
            nRetVal = szTmpBuf[129][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1020];
        }
    }

    return nRetVal;
}

static s32 _DrvFwCtrlMsg28xxCheckFirmwareBinIntegrity(u8 szFwData[][1024])
{
    u32 nCrcMain = 0, nCrcMainBin = 0;
    u32 nCrcInfo = 0, nCrcInfoBin = 0;
    u32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	
    _DrvFwCtrlMsg28xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);
    
    /* Calculate main block CRC & info block CRC by device driver itself */
    nCrcMain = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
    nCrcInfo = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

    /* Read main block CRC & info block CRC from firmware bin file */
    nCrcMainBin = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(szFwData, EMEM_MAIN);
    nCrcInfoBin = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(szFwData, EMEM_INFO);

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainBin=0x%x, nCrcInfoBin=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainBin, nCrcInfoBin);

    if ((nCrcMainBin != nCrcMain) || (nCrcInfoBin != nCrcInfo))
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");
      
        nRetVal = -1;
    } 
    else
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY SUCCESS. PROCEED UPDATE FIRMWARE.\n");

        nRetVal = 0;
    }

    return nRetVal;
}

static s32 _DrvFwCtrlMsg28xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 nCrcMain = 0, nCrcMainHardware = 0, nCrcMainEflash = 0;
    u32 nCrcInfo = 0, nCrcInfoHardware = 0, nCrcInfoEflash = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    if (_DrvFwCtrlMsg28xxCheckFirmwareBinIntegrity(szFwData) < 0)
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");

        g_FwDataCount = 0; // Reset g_FwDataCount to 0 

        DrvPlatformLyrTouchDeviceResetHw();

        return -1;	
    }

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing
    
    /////////////////////////
    // Erase
    /////////////////////////

    if (eEmemType == EMEM_ALL)
    {
        _DrvFwCtrlMsg28xxEraseEmem(EMEM_MAIN);
        _DrvFwCtrlMsg28xxEraseEmem(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        _DrvFwCtrlMsg28xxEraseEmem(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        _DrvFwCtrlMsg28xxEraseEmem(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "erase OK\n");

    /////////////////////////
    // Program
    /////////////////////////

    if (eEmemType == EMEM_ALL)
    {
        _DrvFwCtrlMsg28xxProgramEmem(EMEM_MAIN);
        _DrvFwCtrlMsg28xxProgramEmem(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        _DrvFwCtrlMsg28xxProgramEmem(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        _DrvFwCtrlMsg28xxProgramEmem(EMEM_INFO);
    }
    
    DBG(&g_I2cClient->dev, "program OK\n");

    /* Calculate main block CRC & info block CRC by device driver itself */
    _DrvFwCtrlMsg28xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);
    
    /* Read main block CRC & info block CRC from TP */
    if (eEmemType == EMEM_ALL)
    {
        nCrcMain = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfo = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

        nCrcMainHardware = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
        nCrcInfoHardware = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

        nCrcMainEflash = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
        nCrcInfoEflash = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        nCrcMain = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcMainHardware = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
        nCrcMainEflash = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        nCrcInfo = _DrvFwCtrlMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfoHardware = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);
        nCrcInfoEflash = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainHardware=0x%x, nCrcInfoHardware=0x%x, nCrcMainEflash=0x%x, nCrcInfoEflash=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainHardware, nCrcInfoHardware, nCrcMainEflash, nCrcInfoEflash);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if (eEmemType == EMEM_ALL)
    {
        if ((nCrcMainHardware != nCrcMain) || (nCrcInfoHardware != nCrcInfo) || (nCrcMainEflash != nCrcMain) || (nCrcInfoEflash != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_MAIN)
    {
        if ((nCrcMainHardware != nCrcMain) || (nCrcMainEflash != nCrcMain))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_INFO)
    {
        if ((nCrcInfoHardware != nCrcInfo) || (nCrcInfoEflash != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }

    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

static s32 _DrvFwCtrlUpdateFirmwareCash(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "g_ChipType = 0x%x\n", g_ChipType);
    
    if (g_ChipType == CHIP_TYPE_MSG21XXA) // (0x02)
    {
        u8 nChipVersion = 0;

        DrvPlatformLyrTouchDeviceResetHw();

        // Erase TP Flash first
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
    
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01);

        // Disable watchdog
        RegSet16BitValue(0x3C60, 0xAA55);
    
        /////////////////////////
        // Difference between C2 and C3
        /////////////////////////
        // c2:MSG2133(1) c32:MSG2133A(2) c33:MSG2138A(2)
            
        // check ic version
        nChipVersion = RegGet16BitValue(0x3CEA) & 0xFF;

        DBG(&g_I2cClient->dev, "chip version = 0x%x\n", nChipVersion);
        
        if (nChipVersion == 3)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
            return _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(szFwData, EMEM_MAIN);
#else
            g_FwDataCount = 0; // Reset g_FwDataCount to 0
            
            return -1;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA            
#else
            return _DrvFwCtrlUpdateFirmwareC33(szFwData, EMEM_MAIN);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID   
        }
        else
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
            return _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(szFwData, EMEM_MAIN);
#else
            g_FwDataCount = 0; // Reset g_FwDataCount to 0
            
            return -1;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA            
#else
            return _DrvFwCtrlUpdateFirmwareC32(szFwData, EMEM_ALL);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID        
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX) // (0x7A)
    {
        _DrvFwCtrlMsg22xxGetTpVendorCode(_gTpVendorCode);
        
        if (_gTpVendorCode[0] == 'C' && _gTpVendorCode[1] == 'N' && _gTpVendorCode[2] == 'T') // for specific TP vendor which store some important information in info block, only update firmware for main block, do not update firmware for info block.
        {
            return _DrvFwCtrlMsg22xxUpdateFirmware(szFwData, EMEM_MAIN);
        }
        else
        {
            return _DrvFwCtrlMsg22xxUpdateFirmware(szFwData, EMEM_ALL);
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM) // (0x03)
    {
        return _DrvFwCtrlMsg26xxmUpdateFirmware(szFwData, eEmemType);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX) // (0x85)
    {
        DBG(&g_I2cClient->dev, "IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);

        if (IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED) // Force to update firmware, do not check whether the vendor id of the update firmware bin file is equal to the vendor id on e-flash.
        {
            return _DrvFwCtrlMsg28xxUpdateFirmware(szFwData, EMEM_MAIN);
        }
        else
        {
            u16 eSwId = 0x0000;
            u16 eVendorId = 0x0000;

            eVendorId = szFwData[129][1005] << 8 | szFwData[129][1004]; // Retrieve major from info block
            eSwId = _DrvFwCtrlMsg28xxGetSwId(EMEM_INFO);

            DBG(&g_I2cClient->dev, "eVendorId = 0x%x, eSwId = 0x%x\n", eVendorId, eSwId);
    		
            // Check if the vendor id of the update firmware bin file is equal to the vendor id on e-flash. YES => allow update, NO => not allow update
            if (eSwId != eVendorId)
            {
                DrvPlatformLyrTouchDeviceResetHw(); // Reset HW here to avoid touch may be not worked after get sw id. 

                DBG(&g_I2cClient->dev, "The vendor id of the update firmware bin file is different from the vendor id on e-flash. Not allow to update.\n");
                g_FwDataCount = 0; // Reset g_FwDataCount to 0

                return -1;
            }
            else
            {
                return _DrvFwCtrlMsg28xxUpdateFirmware(szFwData, EMEM_MAIN);
            }
        }
    }
    else 
    {
        DBG(&g_I2cClient->dev, "Undefined chip type.\n");
        g_FwDataCount = 0; // Reset g_FwDataCount to 0

        return -1;
    }	
}

static s32 _DrvFwCtrlUpdateFirmwareBySdCard(const char *pFilePath)
{
    s32 nRetVal = 0;
    struct file *pfile = NULL;
    struct inode *inode;
    s32 fsize = 0;
    mm_segment_t old_fs;
    loff_t pos;
    u16 eSwId = 0x0000;
    u16 eVendorId = 0x0000;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    pfile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pfile))
    {
        DBG(&g_I2cClient->dev, "Error occurred while opening file %s.\n", pFilePath);
        return -1;
    }

    inode = pfile->f_dentry->d_inode;
    fsize = inode->i_size;

    DBG(&g_I2cClient->dev, "fsize = %d\n", fsize);

    if (fsize <= 0)
    {
        filp_close(pfile, NULL);
        return -1;
    }

    // read firmware
    memset(_gFwDataBuf, 0, MSG28XX_FIRMWARE_WHOLE_SIZE*1024);

    old_fs = get_fs();
    set_fs(KERNEL_DS);
  
    pos = 0;
    vfs_read(pfile, _gFwDataBuf, fsize, &pos);
  
    filp_close(pfile, NULL);
    set_fs(old_fs);

    _DrvFwCtrlStoreFirmwareData(_gFwDataBuf, fsize);

    DrvPlatformLyrDisableFingerTouchReport();
    
    if (g_ChipType == CHIP_TYPE_MSG21XXA)    
    {
        eVendorId = g_FwData[31][0x34F] <<8 | g_FwData[31][0x34E];
        eSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_MAIN);
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
        eVendorId = g_FwData[47][1013] <<8 | g_FwData[47][1012];
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)    
    {
        eVendorId = g_FwData[0][0x2B] <<8 | g_FwData[0][0x2A];
        eSwId = _DrvFwCtrlMsg26xxmGetSwId(EMEM_MAIN);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)    
    {
        eVendorId = g_FwData[129][1005] << 8 | g_FwData[129][1004]; // Retrieve major from info block
        eSwId = _DrvFwCtrlMsg28xxGetSwId(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "eVendorId = 0x%x, eSwId = 0x%x\n", eVendorId, eSwId);
    DBG(&g_I2cClient->dev, "IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);
    		
    if ((eSwId == eVendorId) || (IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED))
    {
        if ((g_ChipType == CHIP_TYPE_MSG21XXA && fsize == 33792/* 33KB */) || (g_ChipType == CHIP_TYPE_MSG22XX && fsize == 49664/* 48.5KB */) || (g_ChipType == CHIP_TYPE_MSG26XXM && fsize == 40960/* 40KB */))
        {
    	      nRetVal = _DrvFwCtrlUpdateFirmwareCash(g_FwData, EMEM_ALL);
        }
        else if (g_ChipType == CHIP_TYPE_MSG28XX && fsize == 133120/* 130KB */)
        {
    	      nRetVal = _DrvFwCtrlUpdateFirmwareCash(g_FwData, EMEM_MAIN); // For MSG28xx sine mode requirement, update main block only, do not update info block.
        }
        else
       	{
            DrvPlatformLyrTouchDeviceResetHw();

            DBG(&g_I2cClient->dev, "The file size of the update firmware bin file is not supported, fsize = %d\n", fsize);
            nRetVal = -1;
        }
    }
    else 
    {
        DrvPlatformLyrTouchDeviceResetHw();

        DBG(&g_I2cClient->dev, "The vendor id of the update firmware bin file is different from the vendor id on e-flash. Not allow to update.\n");
        nRetVal = -1;
    }
 
    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware
    
    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

void DrvFwCtrlOpenGestureWakeup(u32 *pMode)
{
    u8 szDbBusTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "wakeup mode 0 = 0x%x\n", pMode[0]);
    DBG(&g_I2cClient->dev, "wakeup mode 1 = 0x%x\n", pMode[1]);

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    szDbBusTxData[0] = 0x59;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = ((pMode[1] & 0xFF000000) >> 24);
    szDbBusTxData[3] = ((pMode[1] & 0x00FF0000) >> 16);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 0 success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable gesture wakeup index 0 failed\n");
    }

    szDbBusTxData[0] = 0x59;
    szDbBusTxData[1] = 0x01;
    szDbBusTxData[2] = ((pMode[1] & 0x0000FF00) >> 8);
    szDbBusTxData[3] = ((pMode[1] & 0x000000FF) >> 0);
	
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 1 success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable gesture wakeup index 1 failed\n");
    }

    szDbBusTxData[0] = 0x59;
    szDbBusTxData[1] = 0x02;
    szDbBusTxData[2] = ((pMode[0] & 0xFF000000) >> 24);
    szDbBusTxData[3] = ((pMode[0] & 0x00FF0000) >> 16);
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 2 success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable gesture wakeup index 2 failed\n");
    }

    szDbBusTxData[0] = 0x59;
    szDbBusTxData[1] = 0x03;
    szDbBusTxData[2] = ((pMode[0] & 0x0000FF00) >> 8);
    szDbBusTxData[3] = ((pMode[0] & 0x000000FF) >> 0);
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 3 success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable gesture wakeup index 3 failed\n");
    }

    g_GestureWakeupFlag = 1; // gesture wakeup is enabled

#else
	
    szDbBusTxData[0] = 0x58;
    szDbBusTxData[1] = ((pMode[0] & 0x0000FF00) >> 8);
    szDbBusTxData[2] = ((pMode[0] & 0x000000FF) >> 0);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable gesture wakeup failed\n");
    }

    g_GestureWakeupFlag = 1; // gesture wakeup is enabled
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
}

void DrvFwCtrlCloseGestureWakeup(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_GestureWakeupFlag = 0; // gesture wakeup is disabled
}

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
void DrvFwCtrlOpenGestureDebugMode(u8 nGestureFlag)
{
    u8 szDbBusTxData[3] = {0};
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "Gesture Flag = 0x%x\n", nGestureFlag);

    szDbBusTxData[0] = 0x30;
    szDbBusTxData[1] = 0x01;
    szDbBusTxData[2] = nGestureFlag;

    mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "Enable gesture debug mode failed\n");
    }
    else
    {
        g_GestureDebugMode = 1; // gesture debug mode is enabled

        DBG(&g_I2cClient->dev, "Enable gesture debug mode success\n");
    }
}

void DrvFwCtrlCloseGestureDebugMode(void)
{
    u8 szDbBusTxData[3] = {0};
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x30;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x00;

    mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "Disable gesture debug mode failed\n");
    }
    else
    {
        g_GestureDebugMode = 0; // gesture debug mode is disabled

        DBG(&g_I2cClient->dev, "Disable gesture debug mode success\n");
    }
}
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static void _DrvFwCtrlCoordinate(u8 *pRawData, u32 *pTranX, u32 *pTranY)
{
   	u32 nX;
   	u32 nY;
#ifdef CONFIG_SWAP_X_Y
   	u32 nTempX;
   	u32 nTempY;
#endif

   	nX = (((pRawData[0] & 0xF0) << 4) | pRawData[1]);         // parse the packet to coordinate
    nY = (((pRawData[0] & 0x0F) << 8) | pRawData[2]);

    DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);

#ifdef CONFIG_SWAP_X_Y
    nTempY = nX;
   	nTempX = nY;
    nX = nTempX;
    nY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
    nX = 2047 - nX;
#endif

#ifdef CONFIG_REVERSE_Y
    nY = 2047 - nY;
#endif

   	/*
   	 * pRawData[0]~pRawData[2] : the point abs,
   	 * pRawData[0]~pRawData[2] all are 0xFF, release touch
   	 */
    if ((pRawData[0] == 0xFF) && (pRawData[1] == 0xFF) && (pRawData[2] == 0xFF))
    {
   	    *pTranX = 0; // final X coordinate
        *pTranY = 0; // final Y coordinate
    }
    else
    {
     	  /* one touch point */
        *pTranX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
        *pTranY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
        DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
        DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, *pTranX, *pTranY);
    }
}
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

static void _DrvFwCtrlReadReadDQMemStart(void)
{
    u8 nParCmdSelUseCfg = 0x7F;
    u8 nParCmdAdByteEn0 = 0x50;
    u8 nParCmdAdByteEn1 = 0x51;
    u8 nParCmdDaByteEn0 = 0x54;
    u8 nParCmdUSetSelB0 = 0x80;
    u8 nParCmdUSetSelB1 = 0x82;
    u8 nParCmdSetSelB2  = 0x85;
    u8 nParCmdIicUse    = 0x35;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2,  1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse,    1);
}

static void _DrvFwCtrlReadReadDQMemEnd(void)
{
    u8 nParCmdNSelUseCfg = 0x7E;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1);
}

u32 DrvFwCtrlReadDQMemValue(u16 nAddr)
{
    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        u8 tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
        u8 rx_data[4] = {0};

        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        DBG(&g_I2cClient->dev, "DQMem Addr = 0x%x\n", nAddr);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073
        mdelay(100);

        _DrvFwCtrlReadReadDQMemStart();

        IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
        IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 4);

        _DrvFwCtrlReadReadDQMemEnd();

        // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        return (rx_data[3] <<24 | rx_data[2] <<16 | rx_data[1] << 8 | rx_data[0]);
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        // TODO : not support yet
    
        return 0;	
    }
}

void DrvFwCtrlWriteDQMemValue(u16 nAddr, u32 nData)
{
    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        u8 szDbBusTxData[7] = {0};

        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        DBG(&g_I2cClient->dev, "DQMem Addr = 0x%x\n", nAddr);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
    
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073
        mdelay(100);

        _DrvFwCtrlReadReadDQMemStart();
    
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = ((nAddr >> 8) & 0xff);
        szDbBusTxData[2] = (nAddr & 0xff);
        szDbBusTxData[3] = nData & 0x000000FF;
        szDbBusTxData[4] = ((nData & 0x0000FF00) >> 8);
        szDbBusTxData[5] = ((nData & 0x00FF0000) >> 16);
        szDbBusTxData[6] = ((nData & 0xFF000000) >> 24);
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 7);

        _DrvFwCtrlReadReadDQMemEnd();

    	  // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
        mdelay(100);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        // TODO : not support yet
    }
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//-------------------------Start of SW ID for MSG22XX----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
static void _DrvFwCtrlMsg22xxEraseEmem(EmemType_e eEmemType)
{
    u32 i = 0;
    u32 nEraseCount = 0;
    u32 nMaxEraseTimes = 0;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    u16 nTrimByte1 = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "Erase start\n");

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
    
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    nTrimByte1 = _DrvFwCtrlMsg22xxGetTrimByte1();
    
    _DrvFwCtrlMsg22xxChangeVoltage();

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (nTrimByte1 == 0xCA)
    {
        nMaxEraseTimes = MSG22XX_MAX_ERASE_EFLASH_TIMES;
    }
    else
    {
        nMaxEraseTimes = 1;	
    }

    for (nEraseCount = 0; nEraseCount < nMaxEraseTimes; nEraseCount ++)
    {
        if (eEmemType == EMEM_ALL) // 48KB + 512Byte
        {
            DBG(&g_I2cClient->dev, "Erase all block %d times\n", nEraseCount);

            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(100);

            // Chip erase
            RegSet16BitValue(0x160E, BIT3);

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
            
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }

                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase all block %d times failed. Timeout.\n", nEraseCount);

                    if (nEraseCount == (nMaxEraseTimes - 1))
                    {
                        goto EraseEnd;
                    }
                }
            }
        }
        else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
        {
            DBG(&g_I2cClient->dev, "Erase main block %d times\n", nEraseCount);

            for (i = 0; i < 3; i ++)
            {
                // Clear pce
                RegSetLByteValue(0x1618, 0x80);
                mdelay(10);
 
                if (i == 0)
                {
                    RegSet16BitValue(0x1600, 0x0000);
                }
                else if (i == 1)
                {
                    RegSet16BitValue(0x1600, 0x8000);
                }
                else if (i == 2)
                {
                    RegSet16BitValue(0x1600, 0xA000);
                }

                // Sector erase
                RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

                DBG(&g_I2cClient->dev, "Wait erase done flag\n");

                nRegData = 0;
                nTimeOut = 0;

                while (1) // Wait erase done flag
                {
                    nRegData = RegGet16BitValue(0x1610); // Memory status
                    nRegData = nRegData & BIT1;
            
                    DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                    if (nRegData == BIT1)
                    {
                        break;		
                    }
                    mdelay(50);

                    if ((nTimeOut ++) > 30)
                    {
                        DBG(&g_I2cClient->dev, "Erase main block %d times failed. Timeout.\n", nEraseCount);

                        if (nEraseCount == (nMaxEraseTimes - 1))
                        {
                            goto EraseEnd;
                        }
                    }
                }
            }   
        }
        else if (eEmemType == EMEM_INFO) // 512Byte
        {
            DBG(&g_I2cClient->dev, "Erase info block %d times\n", nEraseCount);

            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            RegSet16BitValue(0x1600, 0xC000);
        
            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
            
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }
                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase info block %d times failed. Timeout.\n", nEraseCount);

                    if (nEraseCount == (nMaxEraseTimes - 1))
                    {
                        goto EraseEnd;
                    }
                }
            }
        }
    }
    
    _DrvFwCtrlMsg22xxRestoreVoltage();

    EraseEnd:
    
    DBG(&g_I2cClient->dev, "Erase end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static void _DrvFwCtrlMsg22xxProgramEmem(EmemType_e eEmemType) 
{
    u32 i, j; 
    u32 nRemainSize = 0, nBlockSize = 0, nSize = 0, index = 0;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    s32 rc = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u8 szDbBusTxData[128] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else 
    u32 nSizePerWrite = 125;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u8 szDbBusTxData[1024] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else
    u32 nSizePerWrite = 1021;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#endif

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 41; // 123/3=41
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 340; // 1020/3=340
#endif
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 31; // 124/4=31
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 255; // 1020/4=255
#endif
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00);

    DBG(&g_I2cClient->dev, "Program start\n");

    RegSet16BitValue(0x161A, 0xABBA);
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

    if (eEmemType == EMEM_MAIN)
    {
        DBG(&g_I2cClient->dev, "Program main block\n");

        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
    }
    else if (eEmemType == EMEM_INFO)
    {
        DBG(&g_I2cClient->dev, "Program info block\n");

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024;
    }
    else
    {
        DBG(&g_I2cClient->dev, "eEmemType = %d is not supported for program e-memory.\n", eEmemType);
        return;
    }

    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
        RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
        RegSet16BitValueOn(0x160A, BIT0);
        RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Program start
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

    szDbBusTxData[0] = 0x20;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    i = 0;
    
    while (nRemainSize > 0)
    {
        if (nRemainSize > nSizePerWrite)
        {
            nBlockSize = nSizePerWrite;
        }
        else
        {
            nBlockSize = nRemainSize;
        }

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        nSize = 3;

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            u32 k = 0;

            for (j = 0; j < nBlockSize; j ++)
            {
                for (k = 0; k < 3; k ++)
                {
                    szDbBusTxData[3+(j*3)+k] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                }
                nSize = nSize + 3;
            }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            u32 k = 0;

            for (j = 0; j < nBlockSize; j ++)
            {
                for (k = 0; k < 4; k ++)
                {
                    szDbBusTxData[3+(j*4)+k] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                }
                nSize = nSize + 4;
            }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
            for (j = 0; j < nBlockSize; j ++)
            {
                szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
        else
        {
            for (j = 0; j < nBlockSize; j ++)
            {
                szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                nSize ++; 
            }
        }
#else
        for (j = 0; j < nBlockSize; j ++)
        {
            szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
            nSize ++; 
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        i ++;

        rc = IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "Write firmware data failed, rc = %d. Exit program procedure.\n", rc);

            goto ProgramEnd;
        }

        nRemainSize = nRemainSize - nBlockSize;
    }

    // Program end
    szDbBusTxData[0] = 0x21;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
        RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
        RegSet16BitValueOff(0x160A, BIT0);
        RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

    DBG(&g_I2cClient->dev, "Wait write done flag\n");

    while (1) // Wait write done flag
    {
        // Polling 0x1610 is 0x0002
        nRegData = RegGet16BitValue(0x1610); // Memory status
        nRegData = nRegData & BIT1;
    
        DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

        if (nRegData == BIT1)
        {
            break;		
        }
        mdelay(10);

        if ((nTimeOut ++) > 30)
        {
            DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

            goto ProgramEnd;
        }
    }

    ProgramEnd:

    DBG(&g_I2cClient->dev, "Program end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static u32 _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u16 nRegData1 = 0, nRegData2 = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
    
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#else
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55);

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA); 

    if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from main block
    {
        RegSet16BitValue(0x1600, 0xBFFC); // Set start address for main block CRC
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from info block
    {
        RegSet16BitValue(0x1600, 0xC1FC); // Set start address for info block CRC
    }
    
    // Enable burst mode
    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal  = ((nRegData2 >> 8) & 0xFF) << 24;
    nRetVal |= (nRegData2 & 0xFF) << 16;
    nRetVal |= ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    // Clear burst mode
    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[], EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from bin file
        {
            nRetVal  = szTmpBuf[0xBFFF] << 24;
            nRetVal |= szTmpBuf[0xBFFE] << 16;
            nRetVal |= szTmpBuf[0xBFFD] << 8;
            nRetVal |= szTmpBuf[0xBFFC];
        }
        else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from bin file
        {
            nRetVal  = szTmpBuf[0xC1FF] << 24;
            nRetVal |= szTmpBuf[0xC1FE] << 16;
            nRetVal |= szTmpBuf[0xC1FD] << 8;
            nRetVal |= szTmpBuf[0xC1FC];
        }
    }

    return nRetVal;
}

static s32 _DrvFwCtrlMsg22xxUpdateFirmwareBySwId(void) 
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    DBG(&g_I2cClient->dev, "_gIsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, g_IsUpdateFirmware);

    _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(g_FwData, _gOneDimenFwData);
    
    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);
 
            nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
                _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

                nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
                nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

                DBG(&g_I2cClient->dev, "nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
                _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

                nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
                nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
                
                DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

            nCrcInfoA = _DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}

void _DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId(void)
{
    u32 nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg22xxSwId_e eSwId = MSG22XX_SW_ID_UNDEFINED;
    
   // DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    //DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);
    TPD_DMESG("tp firmware update start\n");
    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    nCrcMainB = _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);

    nCrcInfoA = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
    nCrcInfoB = _DrvFwCtrlMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    
    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    //DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);
       TPD_DMESG("nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);        
    if (nCrcMainA == nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 1. Main Block:OK, Info Block:OK
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
    		
        if (eSwId == MSG22XX_SW_ID_XXXX)
        {
            nUpdateBinMajor = msg22xx_xxxx_update_bin[0xBFF5]<<8 | msg22xx_xxxx_update_bin[0xBFF4];
            nUpdateBinMinor = msg22xx_xxxx_update_bin[0xBFF7]<<8 | msg22xx_xxxx_update_bin[0xBFF6];
        }
	/*
        else if (eSwId == MSG22XX_SW_ID_YYYY)
        {
            nUpdateBinMajor = msg22xx_yyyy_update_bin[0xBFF5]<<8 | msg22xx_yyyy_update_bin[0xBFF4];
            nUpdateBinMinor = msg22xx_yyyy_update_bin[0xBFF7]<<8 | msg22xx_yyyy_update_bin[0xBFF6];
        }
     */
        else //eSwId == MSG22XX_SW_ID_UNDEFINED
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }
    		
        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

        DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (eSwId == MSG22XX_SW_ID_XXXX)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 512);
                    }
                }
            }
	/*
            else if (eSwId == MSG22XX_SW_ID_YYYY)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 1024);
                    }
                    else // i == 48
                    {
                        _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 512);
                    }
                }
            }
	*/
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG22XX_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA == nCrcMainB && nCrcInfoA != nCrcInfoB) // Case 2. Main Block:OK, Info Block:FAIL
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
    		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        if (eSwId == MSG22XX_SW_ID_XXXX)
        {
            for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
            {
                if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 1024);
                }
                else // i == 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 512);
                }
            }
        }
	/*
        else if (eSwId == MSG22XX_SW_ID_YYYY)
        {
            for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
            {
                if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 1024);
                }
                else // i == 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 512);
                }
            }
        }
	*/
        else
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA != nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 3. Main Block:FAIL, Info Block:OK
    {
        eSwId = _DrvFwCtrlMsg22xxGetSwId(EMEM_INFO);
		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        if (eSwId == MSG22XX_SW_ID_XXXX)
        {
            for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
            {
                if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 1024);
                }
                else // i == 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_xxxx_update_bin[i*1024]), 512);
                }
            }
        }
	/*
        else if (eSwId == MSG22XX_SW_ID_YYYY)
        {
            for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
            {
                if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 1024);
                }
                else // i == 48
                {
                    _DrvFwCtrlStoreFirmwareData(&(msg22xx_yyyy_update_bin[i*1024]), 512);
                }
            }
        }
	*/
        else
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else // Case 4. Main Block:FAIL, Info Block:FAIL
    {
        DBG(&g_I2cClient->dev, "Main block and Info block are broken.\n");
        DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
    }

    DrvPlatformLyrTouchDeviceResetHw();

    DrvPlatformLyrEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

//-------------------------End of SW ID for MSG22XX----------------------------//

//-------------------------Start of SW ID for MSG21XXA----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
static u32 _DrvFwCtrlMsg21xxaCalculateMainCrcFromEFlash(void) 
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop Watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xDF4C); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start MCU
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
//        DBG(&g_I2cClient->dev, "*** reg(0x3C, 0xE4) = 0x%x ***\n", nRegData); // add for debug

    } while (nRegData != 0x9432);

    // Read calculated main block CRC from register
    nRetVal = RegGet16BitValue(0x3C80);
    nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3C82);
        
    DBG(&g_I2cClient->dev, "Main Block CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg21xxaRetrieveMainCrcFromMainBlock(void) 
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start MCU
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

     // Read main block CRC from main block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x7F;
    szDbBusTxData[2] = 0xFC;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    nRetVal = szDbBusRxData[0];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[3];
   
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static s32 _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(u8 szFwData[][1024], EmemType_e eEmemType) 
{
    u32 i, j, nCalculateCrcSize;
    u32 nCrcMain = 0, nCrcMainTp = 0;
    u32 nCrcInfo = 0, nCrcInfoTp = 0;
    u32 nCrcTemp = 0;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // erase main
    _DrvFwCtrlEraseEmemC33(EMEM_MAIN);
    mdelay(1000);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x1C70);
    }

    switch (eEmemType)
    {
        case EMEM_ALL:
            RegSet16BitValue(0x3CE4, 0xE38F);  // for all blocks
            break;
        case EMEM_MAIN:
            RegSet16BitValue(0x3CE4, 0x7731);  // for main block
            break;
        case EMEM_INFO:
            RegSet16BitValue(0x3CE4, 0x7731);  // for info block

            RegSetLByteValue(0x0FE6, 0x01);

            RegSetLByteValue(0x3CE4, 0xC5); 
            RegSetLByteValue(0x3CE5, 0x78); 

            RegSetLByteValue(0x1E04, 0x9F);
            RegSetLByteValue(0x1E05, 0x82);

            RegSetLByteValue(0x0FE6, 0x00);
            mdelay(100);
            break;
    }

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    if (eEmemType == EMEM_ALL)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_WHOLE_SIZE;
    }
    else if (eEmemType == EMEM_MAIN)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE;
    }
    else if (eEmemType == EMEM_INFO)
    {
        nCalculateCrcSize = MSG21XXA_FIRMWARE_INFO_BLOCK_SIZE;
    }
    else
    {
        nCalculateCrcSize = 0;
    }
		
    for (i = 0; i < nCalculateCrcSize; i ++)
    {
        if (eEmemType == EMEM_INFO)
        {
            i = 32;
        }

        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG(&g_I2cClient->dev, "nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG(&g_I2cClient->dev, "((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG(&g_I2cClient->dev, "Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]);
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
            
            if (eEmemType == EMEM_MAIN)
            {
                break;
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // write file done and check crc
        RegSet16BitValue(0x3CE4, 0x1380);
    }
    mdelay(10);

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // Polling 0x3CE4 is 0x9432
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x9432);
    }

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // CRC Main from TP
        nCrcMainTp = RegGet16BitValue(0x3C80);
        nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
    }

    if (eEmemType == EMEM_ALL)
    {
        // CRC Info from TP
        nCrcInfoTp = RegGet16BitValue(0x3CA0);
        nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);
    }

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n", nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    
    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        if (nCrcMainTp != nCrcMain)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");

            return -1;
        }
    }

    if (eEmemType == EMEM_ALL)
    {
        if (nCrcInfoTp != nCrcInfo)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");

            return -1;
        }
    }

    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
} 

void _DrvFwCtrlMsg21xxaCheckFirmwareUpdateBySwId(void) 
{
    u32 nCrcMainA, nCrcMainB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 nIsCompareVersion = 0;
    u8 *pVersion = NULL; 
    Msg21xxaSwId_e eMainSwId = MSG21XXA_SW_ID_UNDEFINED, eInfoSwId = MSG21XXA_SW_ID_UNDEFINED, eSwId = MSG21XXA_SW_ID_UNDEFINED;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg21xxaCalculateMainCrcFromEFlash();
    nCrcMainB = _DrvFwCtrlMsg21xxaRetrieveMainCrcFromMainBlock();

    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
               
    if (nCrcMainA == nCrcMainB) 
    {
        eMainSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_MAIN);
        eInfoSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_INFO);
    		
        DBG(&g_I2cClient->dev, "Check firmware integrity success\n");
        DBG(&g_I2cClient->dev, "eMainSwId=0x%x, eInfoSwId=0x%x\n", eMainSwId, eInfoSwId);

        if (eMainSwId == eInfoSwId)
        {
        		eSwId = eMainSwId;
        		nIsCompareVersion = 1;
        }
        else
        {
        		eSwId = eInfoSwId;
        		nIsCompareVersion = 0;
        }
        
        if (eSwId == MSG21XXA_SW_ID_XXXX)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg21xxa_xxxx_update_bin[31][0x34F]<<8 | msg21xxa_xxxx_update_bin[31][0x34E];
            nUpdateBinMinor = msg21xxa_xxxx_update_bin[31][0x351]<<8 | msg21xxa_xxxx_update_bin[31][0x350];
#else // By one dimensional array
            nUpdateBinMajor = msg21xxa_xxxx_update_bin[0x7F4F]<<8 | msg21xxa_xxxx_update_bin[0x7F4E];
            nUpdateBinMinor = msg21xxa_xxxx_update_bin[0x7F51]<<8 | msg21xxa_xxxx_update_bin[0x7F50];
#endif
        }
        else if (eSwId == MSG21XXA_SW_ID_YYYY)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg21xxa_yyyy_update_bin[31][0x34F]<<8 | msg21xxa_yyyy_update_bin[31][0x34E];
            nUpdateBinMinor = msg21xxa_yyyy_update_bin[31][0x351]<<8 | msg21xxa_yyyy_update_bin[31][0x350];
#else // By one dimensional array
            nUpdateBinMajor = msg21xxa_yyyy_update_bin[0x7F4F]<<8 | msg21xxa_yyyy_update_bin[0x7F4E];
            nUpdateBinMinor = msg21xxa_yyyy_update_bin[0x7F51]<<8 | msg21xxa_yyyy_update_bin[0x7F50];
#endif
        }
        else //eSwId == MSG21XXA_SW_ID_UNDEFINED
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG21XXA_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }

        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);
    		        
        DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if ((nUpdateBinMinor > nMinor && nIsCompareVersion == 1) || (nIsCompareVersion == 0))
        {
            if (eSwId == MSG21XXA_SW_ID_XXXX)
            {
                for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg21xxa_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg21xxa_xxxx_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG21XXA_SW_ID_YYYY)
            {
                for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg21xxa_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg21xxa_yyyy_update_bin[i*1024]), 1024);
#endif
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG21XXA_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG21XXA_SW_ID_UNDEFINED && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else
    {
        eSwId = _DrvFwCtrlMsg21xxaGetSwId(EMEM_INFO);
    		
        DBG(&g_I2cClient->dev, "Check firmware integrity failed\n");
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        if (eSwId == MSG21XXA_SW_ID_XXXX)
        {
            for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg21xxa_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg21xxa_xxxx_update_bin[i*1024]), 1024);
#endif
            }
        }
        else if (eSwId == MSG21XXA_SW_ID_YYYY)
        {
            for (i = 0; i < MSG21XXA_FIRMWARE_MAIN_BLOCK_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg21xxa_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg21xxa_yyyy_update_bin[i*1024]), 1024);
#endif
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG21XXA_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG21XXA_SW_ID_UNDEFINED && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }

    DrvPlatformLyrTouchDeviceResetHw();
    
    DrvPlatformLyrEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

//-------------------------End of SW ID for MSG21XXA----------------------------//

//-------------------------Start of SW ID for MSG26XXM----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
static u32 _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EmemType_e eEmemType, u8 nIsNeedResetHW)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d, nIsNeedResetHW = %d ***\n", __func__, eEmemType, nIsNeedResetHW);

    if (1 == nIsNeedResetHW)
    {
        DrvPlatformLyrTouchDeviceResetHw();
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    RegSet16BitValue(0x3CE4, 0xDF4C); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x9432);

    if (eEmemType == EMEM_MAIN) // Read calculated main block CRC(32K-8) from register
    {
        nRetVal = RegGet16BitValue(0x3C80);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3C82);
        
        DBG(&g_I2cClient->dev, "Main Block CRC = 0x%x\n", nRetVal);
    }
    else if (eEmemType == EMEM_INFO) // Read calculated info block CRC(8K) from register
    {
        nRetVal = RegGet16BitValue(0x3CA0);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x3CA2);

        DBG(&g_I2cClient->dev, "Info Block CRC = 0x%x\n", nRetVal);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg26xxmRetrieveFirmwareCrcFromMainBlock(EmemType_e eEmemType)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    if (eEmemType == EMEM_MAIN) // Read main block CRC(32K-8) from main block
    {
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0xF8;
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(8K) from main block
    {
        szDbBusTxData[1] = 0x7F;
        szDbBusTxData[2] = 0xFC;
    }
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    /*
      The order of 4 bytes [ 0 : 1 : 2 : 3 ]
      Ex. CRC32 = 0x12345678
          0x7FF8 = 0x78, 0x7FF9 = 0x56,
          0x7FFA = 0x34, 0x7FFB = 0x12
    */

    nRetVal = szDbBusRxData[3];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvFwCtrlMsg26xxmRetrieveInfoCrcFromInfoBlock(void)
{
    u32 nRetVal = 0; 
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);


    // Read info CRC(8K-4) from info block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x80;
    szDbBusTxData[2] = 0x00;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x04;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    nRetVal = szDbBusRxData[3];
    nRetVal = (nRetVal << 8) | szDbBusRxData[2];
    nRetVal = (nRetVal << 8) | szDbBusRxData[1];
    nRetVal = (nRetVal << 8) | szDbBusRxData[0];
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    return nRetVal;	
}

static u32 _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[][1024], EmemType_e eEmemType)
{
    u32 nRetVal = 0; 
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) // Read main block CRC(32K-8) from bin file
        {
            nRetVal = szTmpBuf[31][1019];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1018];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1017];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1016];
        }
        else if (eEmemType == EMEM_INFO) // Read info block CRC(8K) from bin file
        {
            nRetVal = szTmpBuf[31][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[31][1020];
        }
    }

    return nRetVal;
}

static u32 _DrvFwCtrlMsg26xxmCalculateInfoCrcByDeviceDriver(void)
{
    u32 nRetVal = 0xffffffff; 
    u32 i, j;
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    DrvCommonCrcInitTable();

    // Read info data(8K) from info block
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[3] = 0x00; // read 128 bytes
    szDbBusTxData[4] = 0x80;

    for (i = 0; i < 8; i ++)
    {
        for (j = 0; j < 8; j ++)
        {
            szDbBusTxData[1] = 0x80 + (i*0x04) + (((j*128)&0xff00)>>8);
            szDbBusTxData[2] = (j*128)&0x00ff;

            IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

            // Receive info data
            IicReadData(SLAVE_I2C_ID_DWI2C, &_gTempData[j*128], 128); 
        }
     
        if (i == 0)
        {
            for (j = 4; j < 1024; j ++)
            {
                nRetVal = DrvCommonCrcGetValue(_gTempData[j], nRetVal);
            }
        }
        else
        {
            for (j = 0; j < 1024; j ++)
            {
                nRetVal = DrvCommonCrcGetValue(_gTempData[j], nRetVal);
            }
        }
    }

    nRetVal = nRetVal ^ 0xffffffff;

    DBG(&g_I2cClient->dev, "Info(8K-4) CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static s32 _DrvFwCtrlMsg26xxmCompare8BytesForCrc(u8 szTmpBuf[][1024])
{
    s32 nRetVal = -1;
    u16 nRegData = 0;
    u8 szDbBusTxData[5] = {0};
    u8 szDbBusRxData[8] = {0};
    u8 crc[8] = {0}; 
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Read 8 bytes from bin file
    if (szTmpBuf != NULL)
    {
        crc[0] = szTmpBuf[31][1016];
        crc[1] = szTmpBuf[31][1017];
        crc[2] = szTmpBuf[31][1018];
        crc[3] = szTmpBuf[31][1019];
        crc[4] = szTmpBuf[31][1020];
        crc[5] = szTmpBuf[31][1021];
        crc[6] = szTmpBuf[31][1022];
        crc[7] = szTmpBuf[31][1023];
    }

    // Read 8 bytes from the firmware on e-flash
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);

    // Polling 0x3CE4 is 0x5B58
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
    } while (nRegData != 0x5B58);

    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x7F;
    szDbBusTxData[2] = 0xF8;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x08;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    if (crc[0] == szDbBusRxData[0]
        && crc[1] == szDbBusRxData[1]
        && crc[2] == szDbBusRxData[2]
        && crc[3] == szDbBusRxData[3]
        && crc[4] == szDbBusRxData[4]
        && crc[5] == szDbBusRxData[5]
        && crc[6] == szDbBusRxData[6]
        && crc[7] == szDbBusRxData[7])
    {
        nRetVal = 0;		
    }
    else
    {
        nRetVal = -1;		
    }
    
    DBG(&g_I2cClient->dev, "compare 8bytes for CRC = %d\n", nRetVal);

    return nRetVal;
}

static void _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DBG(&g_I2cClient->dev, "erase 0\n");

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // cmd
    RegSet16BitValue(0x3CE4, 0x78C5); //bank:reg_PIU_MISC_0, addr:h0072

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
    RegSet16BitValue(0x1E04, 0x829F);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    mdelay(100);
        
    DBG(&g_I2cClient->dev, "erase 1\n");

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    // Set PROGRAM password
    RegSet16BitValue(0x161A, 0xABBA); //bank:emem, addr:h000D

    if (eEmemType == EMEM_INFO)
    {
        RegSet16BitValue(0x1600, 0x8000); //bank:emem, addr:h0000
    }

    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG(&g_I2cClient->dev, "erase 2\n");

    // Clear setting
    RegSetLByteValue(0x1618, 0x40); //bank:emem, addr:h000C
    
    mdelay(10);
    
    // Clear pce
    RegSetLByteValue(0x1618, 0x80); //bank:emem, addr:h000C

    DBG(&g_I2cClient->dev, "erase 3\n");

    // Trigger erase
    if (eEmemType == EMEM_ALL)
    {
        RegSetLByteValue(0x160E, 0x08); //all chip //bank:emem, addr:h0007
    }
    else if (eEmemType == EMEM_MAIN || eEmemType == EMEM_INFO)
    {
        RegSetLByteValue(0x160E, 0x04); //sector //bank:emem, addr:h0007
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    mdelay(200);	
    
    DBG(&g_I2cClient->dev, "erase OK\n");
}

static void _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EmemType_e eEmemType)
{
    u32 nStart = 0, nEnd = 0; 
    u32 i, j; 
    u16 nRegData = 0;
//    u16 nRegData2 = 0, nRegData3 = 0; // add for debug

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG(&g_I2cClient->dev, "program 0\n");

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    if (eEmemType == EMEM_INFO || eEmemType == EMEM_MAIN)
    {
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

        // cmd
        RegSet16BitValue(0x3CE4, 0x78C5); //bank:reg_PIU_MISC_0, addr:h0072

        // TP SW reset
        RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
        RegSet16BitValue(0x1E04, 0x829F);
        
        nRegData = RegGet16BitValue(0x1618);
//        DBG(&g_I2cClient->dev, "*** reg(0x16, 0x18)  = 0x%x ***\n", nRegData); // add for debug
        
        nRegData |= 0x40;
//        DBG(&g_I2cClient->dev, "*** nRegData  = 0x%x ***\n", nRegData); // add for debug
        
        RegSetLByteValue(0x1618, nRegData);

        // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
        mdelay(100);
    }

    DBG(&g_I2cClient->dev, "program 1\n");

    RegSet16BitValue(0x0F52, 0xDB00); // add for analysis

    // Check_Loader_Ready: Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
//        DBG(&g_I2cClient->dev, "*** reg(0x3C, 0xE4) = 0x%x ***\n", nRegData); // add for debug

//        nRegData2 = RegGet16BitValue(0x0F00); // add for debug
//        DBG(&g_I2cClient->dev, "*** reg(0x0F, 0x00) = 0x%x ***\n", nRegData2);

//        nRegData3 = RegGet16BitValue(0x1E04); // add for debug
//        DBG(&g_I2cClient->dev, "*** reg(0x1E, 0x04) = 0x%x ***\n", nRegData3);

    } while (nRegData != 0x1C70);

    DBG(&g_I2cClient->dev, "program 2\n");

    if (eEmemType == EMEM_ALL)
    {
        RegSet16BitValue(0x3CE4, 0xE38F);  //all chip

        nStart = 0;
        nEnd = MSG26XXM_FIRMWARE_WHOLE_SIZE; //32K + 8K
    }
    else if (eEmemType == EMEM_MAIN)
    {
        RegSet16BitValue(0x3CE4, 0x7731);  //main block

        nStart = 0;
        nEnd = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE; //32K
    }
    else if (eEmemType == EMEM_INFO)
    {
        RegSet16BitValue(0x3CE4, 0xB9D6);  //info block

        nStart = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE;
        nEnd = MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE + MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE;
    }

    // Check_Loader_Ready2Program: Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    DBG(&g_I2cClient->dev, "program 3\n");

    for (i = nStart; i < nEnd; i ++)
    {
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &g_FwData[i][j*128], 128);
        }

        mdelay(100);

        // Check_Program_Done: Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        // Continue_Program
        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    DBG(&g_I2cClient->dev, "program 4\n");

    // Notify_Write_Done
    RegSet16BitValue(0x3CE4, 0x1380);
    mdelay(100);

    DBG(&g_I2cClient->dev, "program 5\n");

    // Check_CRC_Done: Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    DBG(&g_I2cClient->dev, "program 6\n");

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    mdelay(300);

    DBG(&g_I2cClient->dev, "program OK\n");
}

static s32 _DrvFwCtrlMsg26xxmUpdateFirmwareBySwId(void)
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    DBG(&g_I2cClient->dev, "_gIsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, g_IsUpdateFirmware);
    
    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_INFO);
            _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_INFO);
 
            nCrcInfoA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);

            DBG(&g_I2cClient->dev, "nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_MAIN);
                _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_MAIN);

                nCrcMainA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
                nCrcMainB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

                DBG(&g_I2cClient->dev, "nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    nRetVal = _DrvFwCtrlMsg26xxmCompare8BytesForCrc(g_FwData);
                    
                    if (nRetVal == 0)
                    {
                        g_IsUpdateFirmware = 0x00;
                    }
                    else
                    {
                        g_IsUpdateFirmware = 0x11;
                    }
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_MAIN);
            _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                nRetVal = _DrvFwCtrlMsg26xxmCompare8BytesForCrc(g_FwData);

                if (nRetVal == 0)
                {
                    g_IsUpdateFirmware = 0x00;
                }
                else
                {
                    g_IsUpdateFirmware = 0x11;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_MAIN);
            _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 0);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_INFO);
                _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_INFO);

                nCrcInfoA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
                nCrcInfoB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);
                
                DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    nRetVal = _DrvFwCtrlMsg26xxmCompare8BytesForCrc(g_FwData);
                    
                    if (nRetVal == 0)
                    {
                        g_IsUpdateFirmware = 0x00;
                    }
                    else
                    {
                        g_IsUpdateFirmware = 0x11;
                    }
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg26xxmEraseFirmwareOnEFlash(EMEM_INFO);
            _DrvFwCtrlMsg26xxmProgramFirmwareOnEFlash(EMEM_INFO);

            nCrcInfoA = _DrvFwCtrlMsg26xxmRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_INFO, 0);

            DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                nRetVal = _DrvFwCtrlMsg26xxmCompare8BytesForCrc(g_FwData);
                
                if (nRetVal == 0)
                {
                    g_IsUpdateFirmware = 0x00;
                }
                else
                {
                    g_IsUpdateFirmware = 0x11;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}

void _DrvFwCtrlMsg26xxmCheckFirmwareUpdateBySwId(void)
{
    u32 nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg26xxmSwId_e eSwId = MSG26XXM_SW_ID_UNDEFINED;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_MAIN, 1);
    nCrcMainB = _DrvFwCtrlMsg26xxmRetrieveFirmwareCrcFromMainBlock(EMEM_MAIN);

    nCrcInfoA = _DrvFwCtrlMsg26xxmCalculateFirmwareCrcFromEFlash(EMEM_INFO, 1);
    nCrcInfoB = _DrvFwCtrlMsg26xxmRetrieveFirmwareCrcFromMainBlock(EMEM_INFO);
    
    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);
               
    if (nCrcMainA == nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 1. Main Block:OK, Info Block:OK
    {
        eSwId = _DrvFwCtrlMsg26xxmGetSwId(EMEM_MAIN);

        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);
    		
        if (eSwId == MSG26XXM_SW_ID_XXXX)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg26xxm_xxxx_update_bin[0][0x2B]<<8 | msg26xxm_xxxx_update_bin[0][0x2A];
            nUpdateBinMinor = msg26xxm_xxxx_update_bin[0][0x2D]<<8 | msg26xxm_xxxx_update_bin[0][0x2C];
#else // By one dimensional array
            nUpdateBinMajor = msg26xxm_xxxx_update_bin[0x002B]<<8 | msg26xxm_xxxx_update_bin[0x002A];
            nUpdateBinMinor = msg26xxm_xxxx_update_bin[0x002D]<<8 | msg26xxm_xxxx_update_bin[0x002C];
#endif
        }
        else if (eSwId == MSG26XXM_SW_ID_YYYY)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg26xxm_yyyy_update_bin[0][0x2B]<<8 | msg26xxm_yyyy_update_bin[0][0x2A];
            nUpdateBinMinor = msg26xxm_yyyy_update_bin[0][0x2D]<<8 | msg26xxm_yyyy_update_bin[0][0x2C];
#else // By one dimensional array
            nUpdateBinMajor = msg26xxm_yyyy_update_bin[0x002B]<<8 | msg26xxm_yyyy_update_bin[0x002A];
            nUpdateBinMinor = msg26xxm_yyyy_update_bin[0x002D]<<8 | msg26xxm_yyyy_update_bin[0x002C];
#endif
        }
        else //eSwId == MSG26XXM_SW_ID_UNDEFINED
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG26XXM_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }
    		
        DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

        DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (eSwId == MSG26XXM_SW_ID_XXXX)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg26xxm_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg26xxm_xxxx_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG26XXM_SW_ID_YYYY)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg26xxm_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg26xxm_yyyy_update_bin[i*1024]), 1024);
#endif
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG26XXM_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA == nCrcMainB && nCrcInfoA != nCrcInfoB) // Case 2. Main Block:OK, Info Block:FAIL
    {
        eSwId = _DrvFwCtrlMsg26xxmGetSwId(EMEM_MAIN);
    		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        if (eSwId == MSG26XXM_SW_ID_XXXX)
        {
            for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg26xxm_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg26xxm_xxxx_update_bin[i*1024]), 1024);
#endif
            }
        }
        else if (eSwId == MSG26XXM_SW_ID_YYYY)
        {
            for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg26xxm_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg26xxm_yyyy_update_bin[i*1024]), 1024);
#endif
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG26XXM_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else // Case 3. Main Block:FAIL, Info Block:FAIL/OK
    {
        nCrcInfoA = _DrvFwCtrlMsg26xxmRetrieveInfoCrcFromInfoBlock();
        nCrcInfoB = _DrvFwCtrlMsg26xxmCalculateInfoCrcByDeviceDriver();
        
        DBG(&g_I2cClient->dev, "8K-4 : nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

        if (nCrcInfoA == nCrcInfoB) // Check if info block is actually OK.
        {
            eSwId = _DrvFwCtrlMsg26xxmGetSwId(EMEM_INFO);

            DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

            if (eSwId == MSG26XXM_SW_ID_XXXX)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg26xxm_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg26xxm_xxxx_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG26XXM_SW_ID_YYYY)
            {
                for (i = 0; i < MSG26XXM_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg26xxm_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg26xxm_yyyy_update_bin[i*1024]), 1024);
#endif
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG26XXM_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG26XXM_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "Info block is broken.\n");
        }
    }

    DrvPlatformLyrTouchDeviceResetHw(); 

    DrvPlatformLyrEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

//-------------------------End of SW ID for MSG26XXM----------------------------//

//-------------------------Start of SW ID for MSG28XX----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
/*
static s32 _DrvFwCtrlMsg28xxUpdateFirmwareBySwId(void) 
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    DBG(&g_I2cClient->dev, "_gIsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, g_IsUpdateFirmware);

    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg28xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg28xxProgramEmem(EMEM_INFO);
 
            nCrcInfoA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvFwCtrlMsg28xxEraseEmem(EMEM_MAIN);
                _DrvFwCtrlMsg28xxProgramEmem(EMEM_MAIN);

                nCrcMainA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
                nCrcMainB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

                DBG(&g_I2cClient->dev, "nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg28xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg28xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvFwCtrlMsg28xxEraseEmem(EMEM_MAIN);
            _DrvFwCtrlMsg28xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvFwCtrlMsg28xxEraseEmem(EMEM_INFO);
                _DrvFwCtrlMsg28xxProgramEmem(EMEM_INFO);

                nCrcInfoA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
                nCrcInfoB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);
                
                DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvFwCtrlMsg28xxEraseEmem(EMEM_INFO);
            _DrvFwCtrlMsg28xxProgramEmem(EMEM_INFO);

            nCrcInfoA = _DrvFwCtrlMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}
*/

void _DrvFwCtrlMsg28xxCheckFirmwareUpdateBySwId(void) 
{
    u32 nCrcMainA, nCrcMainB;
    u32 i;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 *pVersion = NULL;
    Msg28xxSwId_e eMainSwId = MSG28XX_SW_ID_UNDEFINED, eSwId = MSG28XX_SW_ID_UNDEFINED;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvPlatformLyrDisableFingerTouchReport();

    nCrcMainA = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
    nCrcMainB = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);

    DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

#ifdef CONFIG_ENABLE_CODE_FOR_DEBUG  // TODO : add for debug 
    if (nCrcMainA != nCrcMainB) 
    {
        for (i = 0; i < 5; i ++)
        {
            nCrcMainA = _DrvFwCtrlMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
            nCrcMainB = _DrvFwCtrlMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
    
            DBG(&g_I2cClient->dev, "*** Retry[%d] : nCrcMainA=0x%x, nCrcMainB=0x%x ***\n", i, nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                break;            
            }

            mdelay(50);
        }
    }
#endif //CONFIG_ENABLE_CODE_FOR_DEBUG

    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvFwCtrlUpdateFirmwareBySwIdDoWork);

    if (nCrcMainA == nCrcMainB) 
    {
        eMainSwId = _DrvFwCtrlMsg28xxGetSwId(EMEM_MAIN);
    		
        DBG(&g_I2cClient->dev, "eMainSwId=0x%x\n", eMainSwId);

        eSwId = eMainSwId;

        if (eSwId == MSG28XX_SW_ID_XXXX)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg28xx_xxxx_update_bin[127][1013]<<8 | msg28xx_xxxx_update_bin[127][1012]; 
            nUpdateBinMinor = msg28xx_xxxx_update_bin[127][1015]<<8 | msg28xx_xxxx_update_bin[127][1014];
#else // By one dimensional array
            nUpdateBinMajor = msg28xx_xxxx_update_bin[0x1FFF5]<<8 | msg28xx_xxxx_update_bin[0x1FFF4];
            nUpdateBinMinor = msg28xx_xxxx_update_bin[0x1FFF7]<<8 | msg28xx_xxxx_update_bin[0x1FFF6];
#endif
        }
        else if (eSwId == MSG28XX_SW_ID_YYYY)
        {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
            nUpdateBinMajor = msg28xx_yyyy_update_bin[127][1013]<<8 | msg28xx_yyyy_update_bin[127][1012]; 
            nUpdateBinMinor = msg28xx_yyyy_update_bin[127][1015]<<8 | msg28xx_yyyy_update_bin[127][1014];
#else // By one dimensional array
            nUpdateBinMajor = msg28xx_yyyy_update_bin[0x1FFF5]<<8 | msg28xx_yyyy_update_bin[0x1FFF4];
            nUpdateBinMinor = msg28xx_yyyy_update_bin[0x1FFF7]<<8 | msg28xx_yyyy_update_bin[0x1FFF6];
#endif
        }
        else //eSwId == MSG28XX_SW_ID_UNDEFINED
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG28XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }

        DrvFwCtrlGetCustomerFirmwareVersionByDbBus(EMEM_MAIN, &nMajor, &nMinor, &pVersion);

        DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (eSwId == MSG28XX_SW_ID_XXXX)
            {
                for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg28xx_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg28xx_xxxx_update_bin[i*1024]), 1024);
#endif
                }
            }
            else if (eSwId == MSG28XX_SW_ID_YYYY)
            {
                for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvFwCtrlStoreFirmwareData(msg28xx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                    _DrvFwCtrlStoreFirmwareData(&(msg28xx_yyyy_update_bin[i*1024]), 1024);
#endif
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG28XX_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG28XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else 
    {
        eSwId = _DrvFwCtrlMsg28xxGetSwId(EMEM_INFO);
		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        if (eSwId == MSG28XX_SW_ID_XXXX)
        {
            for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg28xx_xxxx_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg28xx_xxxx_update_bin[i*1024]), 1024);
#endif
            }
        }
        else if (eSwId == MSG28XX_SW_ID_YYYY)
        {
            for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                _DrvFwCtrlStoreFirmwareData(msg28xx_yyyy_update_bin[i], 1024);
#else // By one dimensional array
                _DrvFwCtrlStoreFirmwareData(&(msg28xx_yyyy_update_bin[i*1024]), 1024);
#endif
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG28XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG28XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }

    DrvPlatformLyrTouchDeviceResetHw();

    DrvPlatformLyrEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

//-------------------------End of SW ID for MSG28XX----------------------------//

static void _DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork)
{
    s32 nRetVal = -1;
    
   // DBG(&g_I2cClient->dev, "*** %s() _gUpdateRetryCount = %d ***\n", __func__, _gUpdateRetryCount);
	TPD_DMESG("_gUpdateRetryCount = %d\n",_gUpdateRetryCount);
    if (g_ChipType == CHIP_TYPE_MSG21XXA)   
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        nRetVal = _DrvFwCtrlMsg21xxaUpdateFirmwareBySwId(g_FwData, EMEM_MAIN);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        _DrvFwCtrlMsg22xxGetTpVendorCode(_gTpVendorCode);
        
        if (_gTpVendorCode[0] == 'C' && _gTpVendorCode[1] == 'N' && _gTpVendorCode[2] == 'T') // for specific TP vendor which store some important information in info block, only update firmware for main block, do not update firmware for info block.
        {
            nRetVal = _DrvFwCtrlMsg22xxUpdateFirmware(g_FwData, EMEM_MAIN);
        }
        else
        {
            nRetVal = _DrvFwCtrlMsg22xxUpdateFirmwareBySwId();
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
        nRetVal = _DrvFwCtrlMsg26xxmUpdateFirmwareBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        nRetVal = _DrvFwCtrlMsg28xxUpdateFirmware(g_FwData, EMEM_MAIN);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sw id\n", g_ChipType);

        DrvPlatformLyrTouchDeviceResetHw(); 

        DrvPlatformLyrEnableFingerTouchReport();

        nRetVal = -1;
        return;
    }
    
   // DBG(&g_I2cClient->dev, "*** update firmware by sw id result = %d ***\n", nRetVal);
    TPD_DMESG("*** update firmware by sw id result = %d ***\n", nRetVal);
    if (nRetVal == 0)
    {
        //DBG(&g_I2cClient->dev, "update firmware by sw id success\n");
	TPD_DMESG("update firmware by sw id success\n");
        DrvPlatformLyrTouchDeviceResetHw();

        DrvPlatformLyrEnableFingerTouchReport();

        if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)    
        {
            _gIsUpdateInfoBlockFirst = 0;
            g_IsUpdateFirmware = 0x00;
        }
    }
    else //nRetVal == -1
    {
        _gUpdateRetryCount --;
        if (_gUpdateRetryCount > 0)
        {
            DBG(&g_I2cClient->dev, "_gUpdateRetryCount = %d\n", _gUpdateRetryCount);
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
        }
        else
        {
            //DBG(&g_I2cClient->dev, "update firmware by sw id failed\n");
	    TPD_DMESG("update firmware by sw id failed\n");
            DrvPlatformLyrTouchDeviceResetHw();

            DrvPlatformLyrEnableFingerTouchReport();

            if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)    
            {
                _gIsUpdateInfoBlockFirst = 0;
                g_IsUpdateFirmware = 0x00;
            }
        }
    }
}

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

#ifndef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
static void _DrvFwCtrlEraseEmemC32(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    /////////////////////////
    //Erase  all
    /////////////////////////
    
    // Enter gpio mode
    RegSet16BitValue(0x161E, 0xBEAF);

    // Before gpio mode, set the control pin as the orginal status
    RegSet16BitValue(0x1608, 0x0000);
    RegSetLByteValue(0x160E, 0x10);
    mdelay(10); 

    // ptrim = 1, h'04[2]
    RegSetLByteValue(0x1608, 0x04);
    RegSetLByteValue(0x160E, 0x10);
    mdelay(10); 

    // ptm = 6, h'04[12:14] = b'110
    RegSetLByteValue(0x1609, 0x60);
    RegSetLByteValue(0x160E, 0x10);

    // pmasi = 1, h'04[6]
    RegSetLByteValue(0x1608, 0x44);
    // pce = 1, h'04[11]
    RegSetLByteValue(0x1609, 0x68);
    // perase = 1, h'04[7]
    RegSetLByteValue(0x1608, 0xC4);
    // pnvstr = 1, h'04[5]
    RegSetLByteValue(0x1608, 0xE4);
    // pwe = 1, h'04[9]
    RegSetLByteValue(0x1609, 0x6A);
    // trigger gpio load
    RegSetLByteValue(0x160E, 0x10);
}

static void _DrvFwCtrlReadInfoC33(void)
{
    u8 szDbBusTxData[5] = {0};
    u16 nRegData = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u32 i;
#endif 

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    mdelay(300);

    // Stop Watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    RegSet16BitValue(0x3CE4, 0xA4AB);

    RegSet16BitValue(0x1E04, 0x7d60);

    // TP SW reset
    RegSet16BitValue(0x1E04, 0x829F);
    mdelay(1);
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x0F;
    szDbBusTxData[2] = 0xE6;
    szDbBusTxData[3] = 0x00;
    IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 4);    
    mdelay(100);

    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x5B58);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[3] = 0x00;
    szDbBusTxData[4] = 0x80; // read 128 bytes

    for (i = 0; i < 8; i ++)
    {
        szDbBusTxData[1] = 0x80 + (((i*128)&0xff00)>>8);
        szDbBusTxData[2] = (i*128)&0x00ff;

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

        mdelay(50);

        // Receive info data
        IicReadData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[i*128], 128);
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    szDbBusTxData[0] = 0x72;
    szDbBusTxData[1] = 0x80;
    szDbBusTxData[2] = 0x00;
    szDbBusTxData[3] = 0x04; // read 1024 bytes
    szDbBusTxData[4] = 0x00;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, szDbBusTxData, 5);

    mdelay(50);

    // Receive info data
    IicReadData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[0], 1024);
#endif
}

static s32 _DrvFwCtrlUpdateFirmwareC32(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u32 nCrcTemp;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    /////////////////////////
    // Erase  all
    /////////////////////////
    _DrvFwCtrlEraseEmemC32();
    mdelay(1000); 

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Reset watch dog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x1C70);

    RegSet16BitValue(0x3CE4, 0xE38F);  // for all-blocks

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    for (i = 0; i < 33; i ++) // total  33 KB : 2 byte per R/W
    {
        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG(&g_I2cClient->dev, "nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG(&g_I2cClient->dev, "((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG(&g_I2cClient->dev, "Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]);
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(szFwData[i][j], nCrcInfo);
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    // Write file done
    RegSet16BitValue(0x3CE4, 0x1380);

    mdelay(10); 
    // Polling 0x3CE4 is 0x9432
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x9432);

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    // CRC Main from TP
    nCrcMainTp = RegGet16BitValue(0x3C80);
    nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);
 
    // CRC Info from TP
    nCrcInfoTp = RegGet16BitValue(0x3CA0);
    nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
    {
        DBG(&g_I2cClient->dev, "Update FAILED\n");

        return -1;
    }

    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

static s32 _DrvFwCtrlUpdateFirmwareC33(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u8 szLifeCounter[2];
    u32 i, j;
    u32 nCrcMain, nCrcMainTp;
    u32 nCrcInfo, nCrcInfoTp;
    u32 nCrcTemp;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nCrcMain = 0xffffffff;
    nCrcInfo = 0xffffffff;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    _DrvFwCtrlReadInfoC33();

    if (_gDwIicInfoData[0] == 'M' && _gDwIicInfoData[1] == 'S' && _gDwIicInfoData[2] == 'T' && _gDwIicInfoData[3] == 'A' && _gDwIicInfoData[4] == 'R' && _gDwIicInfoData[5] == 'T' && _gDwIicInfoData[6] == 'P' && _gDwIicInfoData[7] == 'C')
    {
        _gDwIicInfoData[8] = szFwData[32][8];
        _gDwIicInfoData[9] = szFwData[32][9];
        _gDwIicInfoData[10] = szFwData[32][10];
        _gDwIicInfoData[11] = szFwData[32][11];
        // updata life counter
        szLifeCounter[1] = ((((_gDwIicInfoData[13] << 8) | _gDwIicInfoData[12]) + 1) >> 8) & 0xFF;
        szLifeCounter[0] = (((_gDwIicInfoData[13] << 8) | _gDwIicInfoData[12]) + 1) & 0xFF;
        _gDwIicInfoData[12] = szLifeCounter[0];
        _gDwIicInfoData[13] = szLifeCounter[1];
        
        RegSet16BitValue(0x3CE4, 0x78C5);
        RegSet16BitValue(0x1E04, 0x7d60);
        // TP SW reset
        RegSet16BitValue(0x1E04, 0x829F);

        mdelay(50);

        // Polling 0x3CE4 is 0x2F43
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x2F43);

        // Transmit lk info data
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDwIicInfoData[0], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);
    }

    // erase main
    _DrvFwCtrlEraseEmemC33(EMEM_MAIN);
    mdelay(1000);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    /////////////////////////
    // Program
    /////////////////////////

    // Polling 0x3CE4 is 0x1C70
    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x1C70);
    }

    switch (eEmemType)
    {
        case EMEM_ALL:
            RegSet16BitValue(0x3CE4, 0xE38F);  // for all blocks
            break;
        case EMEM_MAIN:
            RegSet16BitValue(0x3CE4, 0x7731);  // for main block
            break;
        case EMEM_INFO:
            RegSet16BitValue(0x3CE4, 0x7731);  // for info block

            RegSetLByteValue(0x0FE6, 0x01);

            RegSetLByteValue(0x3CE4, 0xC5); 
            RegSetLByteValue(0x3CE5, 0x78); 

            RegSetLByteValue(0x1E04, 0x9F);
            RegSetLByteValue(0x1E05, 0x82);

            RegSetLByteValue(0x0FE6, 0x00);
            mdelay(100);
            break;
    }

    // Polling 0x3CE4 is 0x2F43
    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
    } while (nRegData != 0x2F43);

    // Calculate CRC 32
    DrvCommonCrcInitTable();

    for (i = 0; i < 33; i ++) // total 33 KB : 2 byte per R/W
    {
        if (eEmemType == EMEM_INFO)
        {
            i = 32;
        }

        if (i < 32)   // emem_main
        {
            if (i == 31)
            {
                szFwData[i][1014] = 0x5A;
                szFwData[i][1015] = 0xA5;

                for (j = 0; j < 1016; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }

                nCrcTemp = nCrcMain;
                nCrcTemp = nCrcTemp ^ 0xffffffff;

                DBG(&g_I2cClient->dev, "nCrcTemp=%x\n", nCrcTemp); // add for debug

                for (j = 0; j < 4; j ++)
                {
                    szFwData[i][1023-j] = ((nCrcTemp>>(8*j)) & 0xFF);

                    DBG(&g_I2cClient->dev, "((nCrcTemp>>(8*%d)) & 0xFF)=%x\n", j, ((nCrcTemp>>(8*j)) & 0xFF)); // add for debug
                    DBG(&g_I2cClient->dev, "Update main clock crc32 into bin buffer szFwData[%d][%d]=%x\n", i, (1020+j), szFwData[i][1020+j]); // add for debug
                }
            }
            else
            {
                for (j = 0; j < 1024; j ++)
                {
                    nCrcMain = DrvCommonCrcGetValue(szFwData[i][j], nCrcMain);
                }
            }
        }
        else  // emem_info
        {
            for (j = 0; j < 1024; j ++)
            {
                nCrcInfo = DrvCommonCrcGetValue(_gDwIicInfoData[j], nCrcInfo);
            }
            
            if (eEmemType == EMEM_MAIN)
            {
                break;
            }
        }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        for (j = 0; j < 8; j ++)
        {
            IicWriteData(SLAVE_I2C_ID_DWI2C, &szFwData[i][j*128], 128);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        IicWriteData(SLAVE_I2C_ID_DWI2C, szFwData[i], 1024);
#endif

        // Polling 0x3CE4 is 0xD0BC
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0xD0BC);

        RegSet16BitValue(0x3CE4, 0x2F43);
    }

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // write file done and check crc
        RegSet16BitValue(0x3CE4, 0x1380);
    }
    mdelay(10);

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // Polling 0x3CE4 is 0x9432
        do
        {
            nRegData = RegGet16BitValue(0x3CE4);
        } while (nRegData != 0x9432);
    }

    nCrcMain = nCrcMain ^ 0xffffffff;
    nCrcInfo = nCrcInfo ^ 0xffffffff;

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        // CRC Main from TP
        nCrcMainTp = RegGet16BitValue(0x3C80);
        nCrcMainTp = (nCrcMainTp << 16) | RegGet16BitValue(0x3C82);

        // CRC Info from TP
        nCrcInfoTp = RegGet16BitValue(0x3CA0);
        nCrcInfoTp = (nCrcInfoTp << 16) | RegGet16BitValue(0x3CA2);
    }
    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainTp=0x%x, nCrcInfoTp=0x%x\n", nCrcMain, nCrcInfo, nCrcMainTp, nCrcInfoTp);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if ((eEmemType == EMEM_ALL) || (eEmemType == EMEM_MAIN))
    {
        if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");

            return -1;
        }
    }
    
    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

static s32 _DrvFwCtrlMsg22xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i = 0, index = 0;
    u32 nEraseCount = 0;
    u32 nMaxEraseTimes = 0;
    u32 nCrcMain = 0, nCrcMainTp = 0;
    u32 nCrcInfo = 0, nCrcInfoTp = 0;
    u32 nRemainSize, nBlockSize, nSize;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    u16 nTrimByte1 = 0;
    s32 rc = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u8 szDbBusTxData[128] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else 
    u32 nSizePerWrite = 125;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u8 szDbBusTxData[1024] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else
    u32 nSizePerWrite = 1021;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#endif

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 41; // 123/3=41
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 340; // 1020/3=340
#endif
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 31; //124/4=31
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 255; // 1020/4=255
#endif
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    _DrvFwCtrlMsg22xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);

    DrvPlatformLyrTouchDeviceResetHw();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    DBG(&g_I2cClient->dev, "Erase start\n");

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    nTrimByte1 = _DrvFwCtrlMsg22xxGetTrimByte1();
    
    _DrvFwCtrlMsg22xxChangeVoltage();

    // Disable watchdog
    RegSetLByteValue(0x3C60, 0x55);
    RegSetLByteValue(0x3C61, 0xAA);

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (nTrimByte1 == 0xCA)
    {
        nMaxEraseTimes = MSG22XX_MAX_ERASE_EFLASH_TIMES;
    }
    else
    {
        nMaxEraseTimes = 1;	
    }
    
    for (nEraseCount = 0; nEraseCount < nMaxEraseTimes; nEraseCount ++)
    {
        if (eEmemType == EMEM_ALL) // 48KB + 512Byte
        {
            DBG(&g_I2cClient->dev, "Erase all block %d times\n", nEraseCount);

            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(100);

            // Chip erase
            RegSet16BitValue(0x160E, BIT3);

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
            
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }

                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase all block %d times failed. Timeout.\n", nEraseCount);

                    if (nEraseCount == (nMaxEraseTimes - 1))
                    {
                        goto UpdateEnd;
                    }
                }
            }
        }
        else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
        {
            DBG(&g_I2cClient->dev, "Erase main block %d times\n", nEraseCount);

            for (i = 0; i < 3; i ++)
            {
                // Clear pce
                RegSetLByteValue(0x1618, 0x80);
                mdelay(10);
 
                if (i == 0)
                {
                    RegSet16BitValue(0x1600, 0x0000);
                }
                else if (i == 1)
                {
                    RegSet16BitValue(0x1600, 0x8000);
                }
                else if (i == 2)
                {
                    RegSet16BitValue(0x1600, 0xA000);
                }

                // Sector erase
                RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

                DBG(&g_I2cClient->dev, "Wait erase done flag\n");

                nRegData = 0;
                nTimeOut = 0;

                while (1) // Wait erase done flag
                {
                    nRegData = RegGet16BitValue(0x1610); // Memory status
                    nRegData = nRegData & BIT1;
            
                    DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                    if (nRegData == BIT1)
                    {
                        break;		
                    }
                    mdelay(50);

                    if ((nTimeOut ++) > 30)
                    {
                        DBG(&g_I2cClient->dev, "Erase main block %d times failed. Timeout.\n", nEraseCount);

                        if (nEraseCount == (nMaxEraseTimes - 1))
                        {
                            goto UpdateEnd;
                        }
                    }
                }
            }   
        }
        else if (eEmemType == EMEM_INFO) // 512Byte
        {
            DBG(&g_I2cClient->dev, "Erase info block %d times\n", nEraseCount);

            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            RegSet16BitValue(0x1600, 0xC000);
        
            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
            
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }
                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase info block %d times failed. Timeout.\n", nEraseCount);

                    if (nEraseCount == (nMaxEraseTimes - 1))
                    {
                        goto UpdateEnd;
                    }
                }
            }
        }
    }

    _DrvFwCtrlMsg22xxRestoreVoltage();
    
    DBG(&g_I2cClient->dev, "Erase end\n");
    
    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00); 

    /////////////////////////
    // Program
    /////////////////////////

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN) // 48KB
    {
        DBG(&g_I2cClient->dev, "Program main block start\n");
		
        // Program main block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));
		
        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOn(0x160A, BIT0);
            RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    
		
        szDbBusTxData[0] = 0x20;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
		    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }
		
            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;
		
            nSize = 3;
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
            if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 3; j ++)
                    {
                        szDbBusTxData[3+(i*3)+j] = _gOneDimenFwData[index*nSizePerWrite+i];
                    }
                    nSize = nSize + 3;
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 4; j ++)
                    {
                        szDbBusTxData[3+(i*4)+j] = _gOneDimenFwData[index*nSizePerWrite+i];
                    }
                    nSize = nSize + 4;
                }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                    nSize ++; 
                }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
            }
            else
            {
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                    nSize ++; 
                }
            }
#else
            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

            index ++;
		
            rc = IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "Write firmware data failed, rc = %d. Exit program procedure.\n", rc);

                goto UpdateEnd;
            }
		        
            nRemainSize = nRemainSize - nBlockSize;
        }
		
        // Program end
        szDbBusTxData[0] = 0x21;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOff(0x160A, BIT0);
            RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

        DBG(&g_I2cClient->dev, "Wait main block write done flag\n");
		
        nRegData = 0;
        nTimeOut = 0;

        while (1) // Wait write done flag
        {
            // Polling 0x1610 is 0x0002
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
    
            DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(10);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

                goto UpdateEnd;
            }
        }
    
        DBG(&g_I2cClient->dev, "Program main block end\n");
    }
    
    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO) // 512 Byte
    {
        DBG(&g_I2cClient->dev, "Program info block start\n");

        // Program info block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOn(0x160A, BIT0);
            RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

        szDbBusTxData[0] = 0x20;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = 0;
    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }

            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;

            nSize = 3;

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
            if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 3; j ++)
                    {
                        szDbBusTxData[3+(i*3)+j] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    }
                    nSize = nSize + 3;
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 4; j ++)
                    {
                        szDbBusTxData[3+(i*4)+j] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    }
                    nSize = nSize + 4;
                }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    nSize ++; 
                }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
            }
            else
            {
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    nSize ++; 
                }
            }
#else
            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

            index ++;

            IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
        
            nRemainSize = nRemainSize - nBlockSize;
        }

        // Program end
        szDbBusTxData[0] = 0x21;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOff(0x160A, BIT0);
            RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

        DBG(&g_I2cClient->dev, "Wait info block write done flag\n");

        nRegData = 0;
        nTimeOut = 0;

        while (1) // Wait write done flag
        {
            // Polling 0x1610 is 0x0002
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
    
            DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(10);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

                goto UpdateEnd;
            }
        }

        DBG(&g_I2cClient->dev, "Program info block end\n");
    }
    
    UpdateEnd:

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN)
    {
        // Get CRC 32 from updated firmware bin file
        nCrcMain  = _gOneDimenFwData[0xBFFF] << 24;
        nCrcMain |= _gOneDimenFwData[0xBFFE] << 16;
        nCrcMain |= _gOneDimenFwData[0xBFFD] << 8;
        nCrcMain |= _gOneDimenFwData[0xBFFC];

        // CRC Main from TP
        DBG(&g_I2cClient->dev, "Get Main CRC from TP\n");

        nCrcMainTp = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    
        DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcMainTp=0x%x\n", nCrcMain, nCrcMainTp);
    }

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO)
    {
        nCrcInfo  = _gOneDimenFwData[0xC1FF] << 24;
        nCrcInfo |= _gOneDimenFwData[0xC1FE] << 16;
        nCrcInfo |= _gOneDimenFwData[0xC1FD] << 8;
        nCrcInfo |= _gOneDimenFwData[0xC1FC];

        // CRC Info from TP
        DBG(&g_I2cClient->dev, "Get Info CRC from TP\n");

        nCrcInfoTp = _DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

        DBG(&g_I2cClient->dev, "nCrcInfo=0x%x, nCrcInfoTp=0x%x\n", nCrcInfo, nCrcInfoTp);
    }

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if (eEmemType == EMEM_ALL)
    {
        if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_MAIN)
    {
        if (nCrcMainTp != nCrcMain)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_INFO)
    {
        if (nCrcInfoTp != nCrcInfo)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    
    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

void DrvFwCtrlVariableInitialize(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG21XXA)
    {
//        FIRMWARE_MODE_UNKNOWN_MODE = MSG21XXA_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG21XXA_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG21XXA_FIRMWARE_MODE_DEBUG_MODE;
        FIRMWARE_MODE_RAW_DATA_MODE = MSG21XXA_FIRMWARE_MODE_RAW_DATA_MODE;

        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;
        
        DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = SELF_MAX_TOUCH_NUM;
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
//        FIRMWARE_MODE_UNKNOWN_MODE = MSG22XX_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG22XX_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG22XX_FIRMWARE_MODE_DEBUG_MODE;
        FIRMWARE_MODE_RAW_DATA_MODE = MSG22XX_FIRMWARE_MODE_RAW_DATA_MODE;

        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;

        DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = SELF_MAX_TOUCH_NUM;
    }	
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        FIRMWARE_MODE_UNKNOWN_MODE = MSG26XXM_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG26XXM_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG26XXM_FIRMWARE_MODE_DEBUG_MODE;
//        FIRMWARE_MODE_RAW_DATA_MODE = MSG26XXM_FIRMWARE_MODE_RAW_DATA_MODE;

        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;

        DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = MUTUAL_MAX_TOUCH_NUM;
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        FIRMWARE_MODE_UNKNOWN_MODE = MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG28XX_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG28XX_FIRMWARE_MODE_DEBUG_MODE;
//        FIRMWARE_MODE_RAW_DATA_MODE = MSG28XX_FIRMWARE_MODE_RAW_DATA_MODE;

        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;

        DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = MUTUAL_MAX_TOUCH_NUM;
    }	
}	

void DrvFwCtrlOptimizeCurrentConsumption(void)
{
    u32 i;
    u8 szDbBusTxData[35] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "g_ChipType = 0x%x\n", g_ChipType);
    
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        return;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        DrvPlatformLyrTouchDeviceResetHw(); 

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        // Enable burst mode
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x11;
        szDbBusTxData[2] = 0xA0; //bank:0x11, addr:h0050
    
        for (i = 0; i < 24; i ++)
        {
            szDbBusTxData[i+3] = 0x11;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+24);  // Write 0x11 for reg 0x1150~0x115B

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x11;
        szDbBusTxData[2] = 0xB8; //bank:0x11, addr:h005C
    
        for (i = 0; i < 6; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+6);   // Write 0xFF for reg 0x115C~0x115E 
    
        // Clear burst mode
        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); 
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        DrvPlatformLyrTouchDeviceResetHw(); 

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Enable burst mode
        RegSetLByteValue(0x1608, 0x21);

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x15;
        szDbBusTxData[2] = 0x20; //bank:0x15, addr:h0010
    
        for (i = 0; i < 8; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+8);  // Write 0xFF for reg 0x1510~0x1513

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x15;
        szDbBusTxData[2] = 0x28; //bank:0x15, addr:h0014
    
        for (i = 0; i < 16; i ++)
        {
            szDbBusTxData[i+3] = 0x00;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+16);   // Write 0x00 for reg 0x1514~0x151B
    
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x21;
        szDbBusTxData[2] = 0x40; //bank:0x21, addr:h0020
    
        for (i = 0; i < 8; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+8);  // Write 0xFF for reg 0x2120~0x2123

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x21;
        szDbBusTxData[2] = 0x20; //bank:0x21, addr:h0010
    
        for (i = 0; i < 32; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+32);  // Write 0xFF for reg 0x2110~0x211F

        // Clear burst mode
        RegSetLByteValue(0x1608, 0x20);
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        mutex_unlock(&g_Mutex);
    }
}

u8 DrvFwCtrlGetChipType(void)
{
    s32 rc =0;
    u8 nChipType = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Erase TP Flash first
    rc = DbBusEnterSerialDebugMode();
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "*** DbBusEnterSerialDebugMode() failed, rc = %d ***\n", rc);
        return nChipType;
    }
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01);

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K // for MSG22xx only
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Disable watchdog
    RegSet16BitValue(0x3C60, 0xAA55);
    
    nChipType = RegGet16BitValue(0x1ECC) & 0xFF;

    if (nChipType != CHIP_TYPE_MSG21XX &&   // (0x01) 
        nChipType != CHIP_TYPE_MSG21XXA &&  // (0x02) 
        nChipType != CHIP_TYPE_MSG26XXM &&  // (0x03) 
        nChipType != CHIP_TYPE_MSG22XX &&   // (0x7A)
        nChipType != CHIP_TYPE_MSG28XX)     // (0x85)
    {
        nChipType = 0;
    }
    
    if (nChipType == CHIP_TYPE_MSG22XX)  // (0x7A)
    {
        g_Msg22xxChipRevision = RegGetHByteValue(0x1ECE);
    		
        DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);
    }

    DBG(&g_I2cClient->dev, "*** Chip Type = 0x%x ***\n", nChipType);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nChipType;
}	

void DrvFwCtrlGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, u16 *pMajor, u16 *pMinor, u8 **ppVersion) // support MSG28xx only
{
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    if (g_ChipType == CHIP_TYPE_MSG28XX)   
    {
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        if (eEmemType == EMEM_MAIN) // Read SW ID from main block
        {
            _DrvFwCtrlMsg28xxReadEFlashStart(0x7FFD, EMEM_MAIN); 
            nReadAddr = 0x7FFD;
        }
        else if (eEmemType == EMEM_INFO) // Read SW ID from info block
        {
            _DrvFwCtrlMsg28xxReadEFlashStart(0x81FB, EMEM_INFO);
            nReadAddr = 0x81FB;
        }

        _DrvFwCtrlMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

        _DrvFwCtrlMsg28xxReadEFlashEnd();

        /*
          Ex. Major in Main Block :
              Major low byte at address 0x7FFD
          
              Major in Info Block :
              Major low byte at address 0x81FB
        */

        *pMajor = (szTmpData[1] << 8);
        *pMajor |= szTmpData[0];
        *pMinor = (szTmpData[3] << 8);
        *pMinor |= szTmpData[2];

        DBG(&g_I2cClient->dev, "*** Major = %d ***\n", *pMajor);
        DBG(&g_I2cClient->dev, "*** Minor = %d ***\n", *pMinor);
    
        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*11, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%05d.%05d", *pMajor, *pMinor);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
}	

void DrvFwCtrlGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG21XX)
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[4] = {0};

        szDbBusTxData[0] = 0x53;
        szDbBusTxData[1] = 0x00;

        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {    
            szDbBusTxData[2] = 0x2A;
        }
        else if (g_ChipType == CHIP_TYPE_MSG21XX)
        {
            szDbBusTxData[2] = 0x74;
        }
        else
        {
            szDbBusTxData[2] = 0x2A;
        }

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

        mutex_unlock(&g_Mutex);

        *pMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
        *pMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
    
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#else
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for customer firmware version on main block
    
        // Enable burst mode
//        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        *pMajor = (((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF);
        *pMinor = (((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF);

        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[4] = {0};

        szDbBusTxData[0] = 0x53;
        szDbBusTxData[1] = 0x00;
        szDbBusTxData[2] = 0x2A;
    
        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

        mutex_unlock(&g_Mutex);

        DBG(&g_I2cClient->dev, "szDbBusRxData[0] = 0x%x\n", szDbBusRxData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[1] = 0x%x\n", szDbBusRxData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[2] = 0x%x\n", szDbBusRxData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[3] = 0x%x\n", szDbBusRxData[3]); // add for debug

        *pMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
        *pMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];
    }    
    else if (g_ChipType == CHIP_TYPE_MSG28XX)   
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[4] = {0};

        szDbBusTxData[0] = 0x03;

        mutex_lock(&g_Mutex);
    
        DrvPlatformLyrTouchDeviceResetHw();

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_SPRD_PLATFORM);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

        mutex_unlock(&g_Mutex);

        DBG(&g_I2cClient->dev, "szDbBusRxData[0] = 0x%x\n", szDbBusRxData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[1] = 0x%x\n", szDbBusRxData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[2] = 0x%x\n", szDbBusRxData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[3] = 0x%x\n", szDbBusRxData[3]); // add for debug

        *pMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
        *pMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];
    }

    DBG(&g_I2cClient->dev, "*** Major = %d ***\n", *pMajor);
    DBG(&g_I2cClient->dev, "*** Minor = %d ***\n", *pMinor);

    if (*ppVersion == NULL)
    {
        *ppVersion = kzalloc(sizeof(u8)*11, GFP_KERNEL);
    }

    sprintf(*ppVersion, "%d.%d", *pMajor, *pMinor);
}

void DrvFwCtrlGetPlatformFirmwareVersion(u8 **ppVersion)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX) 
    {
        u32 i;
        u16 nRegData1, nRegData2;
        u8 szDbBusRxData[12] = {0};

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
    
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#else
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xC1F2); // Set start address for platform firmware version on info block(Actually, start reading from 0xC1F0)
    
        // Enable burst mode
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        for (i = 0; i < 3; i ++)
        {
            RegSetLByteValue(0x160E, 0x01); 

            nRegData1 = RegGet16BitValue(0x1604);
            nRegData2 = RegGet16BitValue(0x1606);

            szDbBusRxData[i*4+0] = (nRegData1 & 0xFF);
            szDbBusRxData[i*4+1] = ((nRegData1 >> 8 ) & 0xFF);
            
//            DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+0, szDbBusRxData[i*4+0], szDbBusRxData[i*4+0]); // add for debug
//            DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+1, szDbBusRxData[i*4+1], szDbBusRxData[i*4+1]); // add for debug
            
            szDbBusRxData[i*4+2] = (nRegData2 & 0xFF);
            szDbBusRxData[i*4+3] = ((nRegData2 >> 8 ) & 0xFF);

//            DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+2, szDbBusRxData[i*4+2], szDbBusRxData[i*4+2]); // add for debug
//            DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+3, szDbBusRxData[i*4+3], szDbBusRxData[i*4+3]); // add for debug
        }

        // Clear burst mode
        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%c%c%c%c%c%c%c%c%c%c", szDbBusRxData[2], szDbBusRxData[3], szDbBusRxData[4],
            szDbBusRxData[5], szDbBusRxData[6], szDbBusRxData[7], szDbBusRxData[8], szDbBusRxData[9], szDbBusRxData[10], szDbBusRxData[11]);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
        u32 i;
        u16 nRegData = 0;
        u8 szDbBusTxData[5] = {0};
        u8 szDbBusRxData[16] = {0};

        mutex_lock(&g_Mutex);

        DrvPlatformLyrTouchDeviceResetHw();

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

        RegSet16BitValue(0x3CE4, 0xA4AB); //bank:reg_PIU_MISC_0, addr:h0072

        // TP SW reset
        RegSet16BitValue(0x1E04, 0x7d60); //bank:chip, addr:h0002
        RegSet16BitValue(0x1E04, 0x829F);

        // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
        mdelay(100);

        // Polling 0x3CE4 is 0x5B58
        do
        {
            nRegData = RegGet16BitValue(0x3CE4); //bank:reg_PIU_MISC_0, addr:h0072
        } while (nRegData != 0x5B58);

        // Read platform firmware version from info block
        szDbBusTxData[0] = 0x72;
        szDbBusTxData[3] = 0x00;
        szDbBusTxData[4] = 0x08;

        for (i = 0; i < 2; i ++)
        {
            szDbBusTxData[1] = 0x80;
            szDbBusTxData[2] = 0x10 + ((i*8)&0x00ff);

            IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 5);

            mdelay(50);

            IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[i*8], 8);
        }

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*16, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%.16s", szDbBusRxData);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)   
    {
        u8 szDbBusTxData[1] = {0};
        u8 szDbBusRxData[10] = {0};
    
        szDbBusTxData[0] = 0x04;

        mutex_lock(&g_Mutex);
    
        DrvPlatformLyrTouchDeviceResetHw();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_SPRD_PLATFORM);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 10);

        mutex_unlock(&g_Mutex);
           
        DBG(&g_I2cClient->dev, "szDbBusRxData[0] = 0x%x , %c \n", szDbBusRxData[0], szDbBusRxData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[1] = 0x%x , %c \n", szDbBusRxData[1], szDbBusRxData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[2] = 0x%x , %c \n", szDbBusRxData[2], szDbBusRxData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[3] = 0x%x , %c \n", szDbBusRxData[3], szDbBusRxData[3]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[4] = 0x%x , %c \n", szDbBusRxData[4], szDbBusRxData[4]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[5] = 0x%x , %c \n", szDbBusRxData[5], szDbBusRxData[5]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[6] = 0x%x , %c \n", szDbBusRxData[6], szDbBusRxData[6]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[7] = 0x%x , %c \n", szDbBusRxData[7], szDbBusRxData[7]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[8] = 0x%x , %c \n", szDbBusRxData[8], szDbBusRxData[8]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[9] = 0x%x , %c \n", szDbBusRxData[9], szDbBusRxData[9]); // add for debug

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%.10s", szDbBusRxData);
    }
    else
    {
        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%s", "N/A");
    }

    DBG(&g_I2cClient->dev, "*** platform firmware version = %s ***\n", *ppVersion);
}

s32 DrvFwCtrlUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return _DrvFwCtrlUpdateFirmwareCash(szFwData, eEmemType);
}	

s32 DrvFwCtrlUpdateFirmwareBySdCard(const char *pFilePath)
{
    s32 nRetVal = -1;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)    
    {
        nRetVal = _DrvFwCtrlUpdateFirmwareBySdCard(pFilePath);
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sd card\n", g_ChipType);
    }
    
    return nRetVal;
}	

//------------------------------------------------------------------------------//

u16 DrvFwCtrlGetFirmwareMode(void) // use for MSG26XXM only
{
    u16 nMode = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        nMode = RegGet16BitValue(0x3CF4); //bank:reg_PIU_MISC0, addr:h007a

        DBG(&g_I2cClient->dev, "firmware mode = 0x%x\n", nMode);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    
    return nMode;
}

u16 DrvFwCtrlChangeFirmwareMode(u16 nMode)
{
    DBG(&g_I2cClient->dev, "*** %s() *** nMode = 0x%x\n", __func__, nMode);

    if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
        DrvPlatformLyrTouchDeviceResetHw(); 

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        RegSet16BitValue(0x3CF4, nMode); //bank:reg_PIU_MISC0, addr:h007a
        nMode = RegGet16BitValue(0x3CF4); 

        DBG(&g_I2cClient->dev, "firmware mode = 0x%x\n", nMode);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX)  
    {
        u8 szDbBusTxData[2] = {0};
        u32 i = 0;
        s32 rc;

        _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware. 

        szDbBusTxData[0] = 0x02;
        szDbBusTxData[1] = (u8)nMode;

        mutex_lock(&g_Mutex);
        DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 2);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Change firmware mode success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Change firmware mode failed, rc = %d\n", rc);
        }

        DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
        mutex_unlock(&g_Mutex);

        _gIsDisableFinagerTouch = 0;
    }

    return nMode;
}

void DrvFwCtrlSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo) // for MSG21xxA/MSG22xx
{
    u8 szDbBusTxData[1] = {0};
    u8 szDbBusRxData[8] = {0};
    u32 i = 0;
    s32 rc;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware. 

    szDbBusTxData[0] = 0x01;

    mutex_lock(&g_Mutex);
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
        }
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get firmware info IicReadData() success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get firmware info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);
    
    if ((szDbBusRxData[1] & 0x80) == 0x80)
    {
        pInfo->nIsCanChangeFirmwareMode = 0;	
    }
    else
    {
        pInfo->nIsCanChangeFirmwareMode = 1;	
    }
    
    pInfo->nFirmwareMode = szDbBusRxData[1] & 0x7F;
    pInfo->nLogModePacketHeader = szDbBusRxData[2];
    pInfo->nLogModePacketLength = (szDbBusRxData[3]<<8) + szDbBusRxData[4];

    DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode=0x%x, pInfo->nLogModePacketHeader=0x%x, pInfo->nLogModePacketLength=%d, pInfo->nIsCanChangeFirmwareMode=%d\n", pInfo->nFirmwareMode, pInfo->nLogModePacketHeader, pInfo->nLogModePacketLength, pInfo->nIsCanChangeFirmwareMode);

    _gIsDisableFinagerTouch = 0;
}

void DrvFwCtrlMutualGetFirmwareInfo(MutualFirmwareInfo_t *pInfo) // for MSG26xxM/MSG28xx
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[8] = {0};
        u32 i = 0;
        s32 rc;

        _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware. 

        szDbBusTxData[0] = 0x53;
        szDbBusTxData[1] = 0x00;
        szDbBusTxData[2] = 0x48;

        mutex_lock(&g_Mutex);
    
        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
            }

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 8);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicReadData() success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Get firmware info failed, rc = %d\n", rc);
        }

        mutex_unlock(&g_Mutex);

        if (szDbBusRxData[0] == 8 && szDbBusRxData[1] == 0 && szDbBusRxData[2] == 9 && szDbBusRxData[3] == 0)
        {
            DBG(&g_I2cClient->dev, "*** Debug Mode Packet Header is 0xA5 ***\n");
/*
            DbBusEnterSerialDebugMode();
            DbBusStopMCU();
            DbBusIICUseBus();
            DbBusIICReshape();
            mdelay(100);
*/
            pInfo->nLogModePacketHeader = 0xA5;
            pInfo->nMy = 0;
            pInfo->nMx = 0;
//            *pDriveLineNumber = AnaGetMutualSubframeNum();
//            *pSenseLineNumber = AnaGetMutualChannelNum();

/*        
            DbBusIICNotUseBus();
            DbBusNotStopMCU();
            DbBusExitSerialDebugMode();
            mdelay(100);
*/
        }
        else if (szDbBusRxData[0] == 0xAB)
        {
            DBG(&g_I2cClient->dev, "*** Debug Mode Packet Header is 0xAB ***\n");

            pInfo->nLogModePacketHeader = szDbBusRxData[0];
            pInfo->nMy = szDbBusRxData[1];
            pInfo->nMx = szDbBusRxData[2];
//            pInfo->nSd = szDbBusRxData[1];
//            pInfo->nSs = szDbBusRxData[2];
        }
        else if (szDbBusRxData[0] == 0xA7 && szDbBusRxData[3] == PACKET_TYPE_TOOTH_PATTERN) 
        {
            DBG(&g_I2cClient->dev, "*** Debug Packet Header is 0xA7 ***\n");

            pInfo->nLogModePacketHeader = szDbBusRxData[0];
            pInfo->nType = szDbBusRxData[3];
            pInfo->nMy = szDbBusRxData[4];
            pInfo->nMx = szDbBusRxData[5];
            pInfo->nSd = szDbBusRxData[6];
            pInfo->nSs = szDbBusRxData[7];
        }

        if (pInfo->nLogModePacketHeader == 0xA5)
        {
            if (pInfo->nMy != 0 && pInfo->nMx != 0)
            {
                // for parsing debug mode packet 0xA5 
                pInfo->nLogModePacketLength = 1+1+1+1+10*3+pInfo->nMx*pInfo->nMy*2+1;
            }
            else
            {
                DBG(&g_I2cClient->dev, "Failed to retrieve channel number or subframe number for debug mode packet 0xA5.\n");
            }
        }
        else if (pInfo->nLogModePacketHeader == 0xAB)
        {
            if (pInfo->nMy != 0 && pInfo->nMx != 0)
            {
                // for parsing debug mode packet 0xAB 
                pInfo->nLogModePacketLength = 1+1+1+1+10*3+pInfo->nMy*pInfo->nMx*2+pInfo->nMy*2+pInfo->nMx*2+2*2+8*2+1;
            }
            else
            {
                DBG(&g_I2cClient->dev, "Failed to retrieve channel number or subframe number for debug mode packet 0xAB.\n");
            }
        }
        else if (pInfo->nLogModePacketHeader == 0xA7 && pInfo->nType == PACKET_TYPE_TOOTH_PATTERN)
        {
            if (pInfo->nMy != 0 && pInfo->nMx != 0 && pInfo->nSd != 0 && pInfo->nSs != 0)
            {
                // for parsing debug mode packet 0xA7  
                pInfo->nLogModePacketLength = 1+1+1+1+1+10*3+pInfo->nMy*pInfo->nMx*2+pInfo->nSd*2+pInfo->nSs*2+10*2+1;
            }
            else
            {
                DBG(&g_I2cClient->dev, "Failed to retrieve channel number or subframe number for debug mode packet 0xA7.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "Undefined debug mode packet header = 0x%x\n", pInfo->nLogModePacketHeader);
        }
    
        DBG(&g_I2cClient->dev, "*** debug mode packet header = 0x%x ***\n", pInfo->nLogModePacketHeader);
        DBG(&g_I2cClient->dev, "*** debug mode packet length = %d ***\n", pInfo->nLogModePacketLength);
        DBG(&g_I2cClient->dev, "*** Type = 0x%x ***\n", pInfo->nType);
        DBG(&g_I2cClient->dev, "*** My = %d ***\n", pInfo->nMy);
        DBG(&g_I2cClient->dev, "*** Mx = %d ***\n", pInfo->nMx);
        DBG(&g_I2cClient->dev, "*** Sd = %d ***\n", pInfo->nSd);
        DBG(&g_I2cClient->dev, "*** Ss = %d ***\n", pInfo->nSs);

        _gIsDisableFinagerTouch = 0;
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)   
    {
        u8 szDbBusTxData[1] = {0};
        u8 szDbBusRxData[10] = {0};
        u32 i = 0;
        s32 rc;
    
        _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware. 
        
        szDbBusTxData[0] = 0x01;

        mutex_lock(&g_Mutex);
        DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug
    
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
            }
            
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 10);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicReadData() success\n");

                if (szDbBusRxData[1] == FIRMWARE_MODE_DEMO_MODE || szDbBusRxData[1] == FIRMWARE_MODE_DEBUG_MODE)
                {
                    break;
                }
                else
                {
                    i = 0;
                }
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Get firmware info failed, rc = %d\n", rc);
        }

        DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
        mutex_unlock(&g_Mutex);
    
        // Add protection for incorrect firmware info check
        if ((szDbBusRxData[1] == FIRMWARE_MODE_DEBUG_MODE && szDbBusRxData[2] == 0xA7 && szDbBusRxData[5] == PACKET_TYPE_TOOTH_PATTERN) || (szDbBusRxData[1] == FIRMWARE_MODE_DEMO_MODE && szDbBusRxData[2] == 0x5A))
        {
            pInfo->nFirmwareMode = szDbBusRxData[1];
            DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode = 0x%x\n", pInfo->nFirmwareMode);

            pInfo->nLogModePacketHeader = szDbBusRxData[2]; 
            pInfo->nLogModePacketLength = (szDbBusRxData[3]<<8) + szDbBusRxData[4]; 
            pInfo->nType = szDbBusRxData[5];
            pInfo->nMy = szDbBusRxData[6];
            pInfo->nMx = szDbBusRxData[7];
            pInfo->nSd = szDbBusRxData[8];
            pInfo->nSs = szDbBusRxData[9];

            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketHeader = 0x%x\n", pInfo->nLogModePacketHeader);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketLength = %d\n", pInfo->nLogModePacketLength);
            DBG(&g_I2cClient->dev, "pInfo->nType = 0x%x\n", pInfo->nType);
            DBG(&g_I2cClient->dev, "pInfo->nMy = %d\n", pInfo->nMy);
            DBG(&g_I2cClient->dev, "pInfo->nMx = %d\n", pInfo->nMx);
            DBG(&g_I2cClient->dev, "pInfo->nSd = %d\n", pInfo->nSd);
            DBG(&g_I2cClient->dev, "pInfo->nSs = %d\n", pInfo->nSs);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Firmware info before correcting :\n");
            
            DBG(&g_I2cClient->dev, "FirmwareMode = 0x%x\n", szDbBusRxData[1]);
            DBG(&g_I2cClient->dev, "LogModePacketHeader = 0x%x\n", szDbBusRxData[2]);
            DBG(&g_I2cClient->dev, "LogModePacketLength = %d\n", (szDbBusRxData[3]<<8) + szDbBusRxData[4]);
            DBG(&g_I2cClient->dev, "Type = 0x%x\n", szDbBusRxData[5]);
            DBG(&g_I2cClient->dev, "My = %d\n", szDbBusRxData[6]);
            DBG(&g_I2cClient->dev, "Mx = %d\n", szDbBusRxData[7]);
            DBG(&g_I2cClient->dev, "Sd = %d\n", szDbBusRxData[8]);
            DBG(&g_I2cClient->dev, "Ss = %d\n", szDbBusRxData[9]);

            // Set firmware mode to demo mode(default)
            pInfo->nFirmwareMode = FIRMWARE_MODE_DEMO_MODE;
            pInfo->nLogModePacketHeader = 0x5A; 
            pInfo->nLogModePacketLength = DEMO_MODE_PACKET_LENGTH; 
            pInfo->nType = 0;
            pInfo->nMy = 0;
            pInfo->nMx = 0;
            pInfo->nSd = 0;
            pInfo->nSs = 0;

            DBG(&g_I2cClient->dev, "Firmware info after correcting :\n");

            DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode = 0x%x\n", pInfo->nFirmwareMode);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketHeader = 0x%x\n", pInfo->nLogModePacketHeader);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketLength = %d\n", pInfo->nLogModePacketLength);
            DBG(&g_I2cClient->dev, "pInfo->nType = 0x%x\n", pInfo->nType);
            DBG(&g_I2cClient->dev, "pInfo->nMy = %d\n", pInfo->nMy);
            DBG(&g_I2cClient->dev, "pInfo->nMx = %d\n", pInfo->nMx);
            DBG(&g_I2cClient->dev, "pInfo->nSd = %d\n", pInfo->nSd);
            DBG(&g_I2cClient->dev, "pInfo->nSs = %d\n", pInfo->nSs);
        }
        
        _gIsDisableFinagerTouch = 0;
    }
}

void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void)
{
    DBG(&g_I2cClient->dev, "*** %s() g_IsSwitchModeByAPK = %d ***\n", __func__, g_IsSwitchModeByAPK);

    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        if (g_IsSwitchModeByAPK == 1)
        {
            SelfFirmwareInfo_t tInfo;
    
            memset(&tInfo, 0x0, sizeof(SelfFirmwareInfo_t));

            DrvFwCtrlSelfGetFirmwareInfo(&tInfo);

            DBG(&g_I2cClient->dev, "g_FirmwareMode = 0x%x, tInfo.nFirmwareMode = 0x%x\n", g_FirmwareMode, tInfo.nFirmwareMode);

            // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            }
            else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && FIRMWARE_MODE_RAW_DATA_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
            }
            else
            {
                DBG(&g_I2cClient->dev, "firmware mode is not restored\n");
            }
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        if (g_IsSwitchModeByAPK == 1)
        {
            DBG(&g_I2cClient->dev, "g_FirmwareMode = 0x%x\n", g_FirmwareMode);

            // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEMO_MODE == DrvFwCtrlGetFirmwareMode())
            {
                g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            }
            else
            {
                DBG(&g_I2cClient->dev, "firmware mode is not restored\n");
            }
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        if (g_IsSwitchModeByAPK == 1)
        {
            MutualFirmwareInfo_t tInfo;
    
            memset(&tInfo, 0x0, sizeof(MutualFirmwareInfo_t));

            DrvFwCtrlMutualGetFirmwareInfo(&tInfo);

            DBG(&g_I2cClient->dev, "g_FirmwareMode = 0x%x, tInfo.nFirmwareMode = 0x%x\n", g_FirmwareMode, tInfo.nFirmwareMode);

            // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = DrvFwCtrlChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            }
            else
            {
                DBG(&g_I2cClient->dev, "firmware mode is not restored\n");
            }
        }
    }
}	

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
void DrvFwCtrlCheckFirmwareUpdateBySwId(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (g_ChipType == CHIP_TYPE_MSG21XXA)   
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        _DrvFwCtrlMsg21xxaCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        _DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX        
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM)   
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
        _DrvFwCtrlMsg26xxmCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM        
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        _DrvFwCtrlMsg28xxCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sw id\n", g_ChipType);
    }
}	
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
void DrvFwCtrlGetTouchPacketAddress(u16* pDataAddress, u16* pFlagAddress)
{
    s32 rc = 0;
    u32 i = 0;
    u8 szDbBusTxData[1] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x05;

    mutex_lock(&g_Mutex);
    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get touch packet address IicWriteData() success\n");
        }

        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get touch packet address IicReadData() success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get touch packet address failed, rc = %d\n", rc);
    }

    if (rc < 0)
    {
        g_FwSupportSegment = 0;
    }
    else
    {
        *pDataAddress = (szDbBusRxData[0]<<8) + szDbBusRxData[1];
        *pFlagAddress = (szDbBusRxData[2]<<8) + szDbBusRxData[3];

        g_FwSupportSegment = 1;

        DBG(&g_I2cClient->dev, "*** *pDataAddress = 0x%2X ***\n", *pDataAddress); // add for debug
        DBG(&g_I2cClient->dev, "*** *pFlagAddress = 0x%2X ***\n", *pFlagAddress); // add for debug
    }

    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
}

static int _DrvFwCtrlCheckFingerTouchPacketFlagBit1(void)
{
    u8 szDbBusTxData[3] = {0};
    s32 nRetVal;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x53;
    szDbBusTxData[1] = (g_FwPacketFlagAddress >> 8) & 0xFF;
    szDbBusTxData[2] = g_FwPacketFlagAddress & 0xFF;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DWI2C, &_gTouchPacketFlag[0], 2);

    if ((_gTouchPacketFlag[0] & BIT1) == 0x00)
    {
        nRetVal = 0; // Bit1 is 0
    }
    else
    {
        nRetVal = 1; // Bit1 is 1
    }
    DBG(&g_I2cClient->dev, "Bit1 = %d\n", nRetVal);

    return nRetVal;
}

static void _DrvFwCtrlResetFingerTouchPacketFlagBit1(void)
{
    u8 szDbBusTxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x52;
    szDbBusTxData[1] = (g_FwPacketFlagAddress >> 8) & 0xFF;
    szDbBusTxData[2] = g_FwPacketFlagAddress & 0xFF;
    szDbBusTxData[3] = _gTouchPacketFlag[0] | BIT1;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
}
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION

s32 DrvFwCtrlEnableProximity(void)
{
    u8 szDbBusTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x52;
    szDbBusTxData[1] = 0x00;
    
    if (g_ChipType == CHIP_TYPE_MSG21XX)
    {
        szDbBusTxData[2] = 0x62; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        szDbBusTxData[2] = 0x4a; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        szDbBusTxData[2] = 0x47; 
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** Un-recognized chip type = 0x%x ***\n", g_ChipType);
        return -1;
    }
    
    szDbBusTxData[3] = 0xa0;

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            g_EnableTpProximity = 1;

            //DBG(&g_I2cClient->dev, "Enable proximity detection success\n");
	    TPD_DMESG("psensor enable success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        //DBG(&g_I2cClient->dev, "Enable proximity detection failed, rc = %d\n", rc);
	 TPD_DMESG("psensor enable no success=%d\n",rc);
    }
    	
    return rc;
}

s32 DrvFwCtrlDisableProximity(void)
{
    u8 szDbBusTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x52;
    szDbBusTxData[1] = 0x00;

    if (g_ChipType == CHIP_TYPE_MSG21XX)
    {
        szDbBusTxData[2] = 0x62; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        szDbBusTxData[2] = 0x4a; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        szDbBusTxData[2] = 0x47; 
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** Un-recognized chip type = 0x%x ***\n", g_ChipType);
        return -1;
    }

    szDbBusTxData[3] = 0xa1;
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 4);
        if (rc > 0)
        {
            g_EnableTpProximity = 0;

            //DBG(&g_I2cClient->dev, "Disable proximity detection success\n");
	    TPD_DMESG("psensor disenable success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        //DBG(&g_I2cClient->dev, "Disable proximity detection failed, rc = %d\n", rc);
	 TPD_DMESG("psensor disenable error=%d\n",rc);
    }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    g_FaceClosingTp = 0;
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

    return rc;
}

#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_GLOVE_MODE
void DrvFwCtrlOpenGloveMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szDbBusTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x01;
    szDbBusTxData[2] = 0x01;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
       mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
       rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
       if (rc > 0)
       {
           DBG(&g_I2cClient->dev, "Open glove mode success\n");
           break;
       }

       i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Open glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    _gIsDisableFinagerTouch = 0;
}

void DrvFwCtrlCloseGloveMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szDbBusTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x01;
    szDbBusTxData[2] = 0x00;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
       mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
       rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
       if (rc > 0)
       {
           DBG(&g_I2cClient->dev, "Close glove mode success\n");
           break;
       }

       i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Close glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    _gIsDisableFinagerTouch = 0;
}

void DrvFwCtrlGetGloveInfo(u8 *pGloveMode) // used for MSG28xx only
{
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[2] = {0};
    u32 i = 0;
    s32 rc;

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x01;
    szDbBusTxData[2] = 0x02;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get glove info IicWriteData() success\n");
        }
        
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get glove info IicReadData() success\n");

            if (szDbBusRxData[0] == 0x00 || szDbBusRxData[0] == 0x01)
            {
                break;
            }
            else
            {
                i = 0;
            }
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get glove info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    *pGloveMode = szDbBusRxData[0];
    
    DBG(&g_I2cClient->dev, "*pGloveMode = 0x%x\n", *pGloveMode);

    _gIsDisableFinagerTouch = 0;
}

#endif //CONFIG_ENABLE_GLOVE_MODE
//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_LEATHER_SHEATH_MODE
void DrvFwCtrlOpenLeatherSheathMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szDbBusTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x02;
    szDbBusTxData[2] = 0x01;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
       mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
       rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
       if (rc > 0)
       {
           DBG(&g_I2cClient->dev, "Open leather sheath mode success\n");
           break;
       }

       i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Open leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    _gIsDisableFinagerTouch = 0;
}

void DrvFwCtrlCloseLeatherSheathMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szDbBusTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x02;
    szDbBusTxData[2] = 0x00;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
       mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
       rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
       if (rc > 0)
       {
           DBG(&g_I2cClient->dev, "Close leather sheath mode success\n");
           break;
       }

       i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Close leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    _gIsDisableFinagerTouch = 0;
}

void DrvFwCtrlGetLeatherSheathInfo(u8 *pLeatherSheathMode) // used for MSG28xx only
{
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[2] = {0};
    u32 i = 0;
    s32 rc;

    _gIsDisableFinagerTouch = 1; // Disable finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware.

    szDbBusTxData[0] = 0x06;
    szDbBusTxData[1] = 0x02;
    szDbBusTxData[2] = 0x02;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get leather sheath info IicWriteData() success\n");
        }
        
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get leather sheath info IicReadData() success\n");

            if (szDbBusRxData[0] == 0x00 || szDbBusRxData[0] == 0x01)
            {
                break;
            }
            else
            {
                i = 0;
            }
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get leather sheath info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    *pLeatherSheathMode = szDbBusRxData[0];
    
    DBG(&g_I2cClient->dev, "*pLeatherSheathMode = 0x%x\n", *pLeatherSheathMode);

    _gIsDisableFinagerTouch = 0;
}

#endif //CONFIG_ENABLE_LEATHER_SHEATH_MODE
//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_CHARGER_DETECTION

void DrvFwCtrlChargerDetection(u8 nChargerStatus)
{
    u32 i = 0;
    u8 szDbBusTxData[2] = {0};
    s32 rc = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "_gChargerPlugIn = %d, nChargerStatus = %d, g_ForceUpdate = %d\n", _gChargerPlugIn, nChargerStatus, g_ForceUpdate);

    mutex_lock(&g_Mutex);
    
    szDbBusTxData[0] = 0x09;

    if (nChargerStatus) // charger plug in
    {
        if (_gChargerPlugIn == 0 || g_ForceUpdate == 1)
        {
          	szDbBusTxData[1] = 0xA5;
            
            while (i < 5)
            {
                mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
                rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 2);
                if (rc > 0)
                {
                    _gChargerPlugIn = 1;

                    DBG(&g_I2cClient->dev, "Update status for charger plug in success.\n");
                    break;
                }

                i ++;
            }
            if (i == 5)
            {
                DBG(&g_I2cClient->dev, "Update status for charger plug in failed, rc = %d\n", rc);
            }

            g_ForceUpdate = 0; // Clear flag after force update charger status
        }
    }
    else  // charger plug out
    {
        if (_gChargerPlugIn == 1 || g_ForceUpdate == 1)
        {
          	szDbBusTxData[1] = 0x5A;
            
            while (i < 5)
            {
                mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
                rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 2);
                if (rc > 0)
                {
                    _gChargerPlugIn = 0;

                    DBG(&g_I2cClient->dev, "Update status for charger plug out success.\n");
                    break;
                }
                
                i ++;
            }
            if (i == 5)
            {
                DBG(&g_I2cClient->dev, "Update status for charger plug out failed, rc = %d\n", rc);
            }

            g_ForceUpdate = 0; // Clear flag after force update charger status
        }
    }	

    mutex_unlock(&g_Mutex);
}	

#endif //CONFIG_ENABLE_CHARGER_DETECTION

//------------------------------------------------------------------------------//

static s32 _DrvFwCtrlReadFingerTouchData(u8 *pPacket, u16 nReportPacketLength)
{
    s32 rc;

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
                return -1;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            DBG(&g_I2cClient->dev, "*** g_FwPacketDataAddress = 0x%2X ***\n", g_FwPacketDataAddress); // add for debug
            DBG(&g_I2cClient->dev, "*** g_FwPacketFlagAddress = 0x%2X ***\n", g_FwPacketFlagAddress); // add for debug

            if (g_FwSupportSegment == 0)
            {
                DBG(&g_I2cClient->dev, "g_FwPacketDataAddress & g_FwPacketFlagAddress is un-initialized\n");
                return -1;
            }

            if (_gIsDisableFinagerTouch == 1)
            {
                DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
                return -1;
            }

            rc = IicSegmentReadDataBySmBus(g_FwPacketDataAddress, &pPacket[0], nReportPacketLength, MAX_I2C_TRANSACTION_LENGTH_LIMIT);

            _DrvFwCtrlCheckFingerTouchPacketFlagBit1();
            _DrvFwCtrlResetFingerTouchPacketFlagBit1();

            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read debug mode packet data failed, rc = %d\n", rc);
                return -1;
            }
#else

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
            mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_SPRD_PLATFORM);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
                return -1;
            }
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA    		
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            return -1;
        }
    }
    else
    {	
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            return -1;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED	

    return 0;
}

//------------------------------------------------------------------------------//

void _DrvFwCtrlSelfHandleFingerTouch(void) // for MSG21xxA/MSG22xx
{
    SelfTouchInfo_t tInfo;
    u32 i;
#ifdef CONFIG_TP_HAVE_KEY
    u8 nTouchKeyCode = 0;
#endif //CONFIG_TP_HAVE_KEY
    static u32 nLastKeyCode = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;
    s32 rc;

//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);  // add for debug
    
    if (_gIsDisableFinagerTouch == 1)
    {
        DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
        return;
    }
    
    mutex_lock(&g_Mutex);
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    memset(&tInfo, 0x0, sizeof(SelfTouchInfo_t));

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    { 	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = g_DemoModePacket;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEBUG_MODE\n");

            if (g_SelfFirmwareInfo.nLogModePacketHeader != 0x62)
            {
                DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER : 0x%x\n", g_SelfFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }

            nReportPacketLength = g_SelfFirmwareInfo.nLogModePacketLength;
            pPacket = g_LogModePacket;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_RAW_DATA_MODE\n");

            if (g_SelfFirmwareInfo.nLogModePacketHeader != 0x62)
            {
                DBG(&g_I2cClient->dev, "WRONG RAW DATA MODE HEADER : 0x%x\n", g_SelfFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }

            nReportPacketLength = g_SelfFirmwareInfo.nLogModePacketLength;
            pPacket = g_LogModePacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

        nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
        pPacket = g_DemoModePacket;
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureDebugMode == 1 && g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture debug mode packet length, g_ChipType=0x%x\n", g_ChipType);

        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nReportPacketLength = GESTURE_DEBUG_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture debug mode.\n");
            goto TouchHandleEnd;		
        }
    }
    else if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length, g_ChipType=0x%x\n", g_ChipType);
      
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
            nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            pPacket = _gGestureWakeupPacket;
        }
        else if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture wakeup.\n");
            goto TouchHandleEnd;
        }
    }

#else

    if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length, g_ChipType=0x%x\n", g_ChipType);
        
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
            nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

            pPacket = _gGestureWakeupPacket;
        } 
        else if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture wakeup.\n");
            goto TouchHandleEnd;		
        }
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u32 i = 0
        
        while (i < 5)
        {
            mdelay(50);

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            
            if (rc > 0)
            {
                break;
            }
            
            i ++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
#else
    rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (nReportPacketLength > 8)
    {
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    }
    else
    {
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    }

    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#endif
    
    if (0 == _DrvFwCtrlSelfParsePacket(pPacket, nReportPacketLength, &tInfo))
    {
        //report...
        if ((tInfo.nFingerNum) == 0)   //touch end
        {
            if (nLastKeyCode != 0)
            {
                DBG(&g_I2cClient->dev, "key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
                input_report_key(g_InputDevice, nLastKeyCode, 0);

                input_sync(g_InputDevice);
                    
                nLastKeyCode = 0; //clear key status..
            }
            else
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
                for (i = 0; i < MAX_TOUCH_NUM; i ++) 
                {
                    DrvPlatformLyrFingerTouchReleased(0, 0, i);
		    _gPriorPress[i] = _gCurrPress[i];
                }

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 0);
#else // TYPE A PROTOCOL
                DrvPlatformLyrFingerTouchReleased(0, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                input_sync(g_InputDevice);
            }
        }
        else //touch on screen
        {
            if (tInfo.nTouchKeyCode != 0)
            {
#ifdef CONFIG_TP_HAVE_KEY
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                {
                    nTouchKeyCode = g_TpVirtualKey[1];           
                }
                else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                {
                    nTouchKeyCode = g_TpVirtualKey[0];
                }           
                else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                {
                    nTouchKeyCode = g_TpVirtualKey[2];
                }           
                else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                {	
                    nTouchKeyCode = g_TpVirtualKey[3];           
                }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[1];           
                    }
                    else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[0];
                    }           
                    else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[2];
                    }           
                    else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                    {	
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[3];           
                    }
                }
#else
                if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                {
                    nTouchKeyCode = g_TpVirtualKey[1];           
                }
                else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                {
                    nTouchKeyCode = g_TpVirtualKey[0];
                }           
                else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                {
                    nTouchKeyCode = g_TpVirtualKey[2];
                }           
                else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                {	
                    nTouchKeyCode = g_TpVirtualKey[3];           
                }
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

                if (nLastKeyCode != nTouchKeyCode)
                {
                    DBG(&g_I2cClient->dev, "key touch pressed\n");
                    DBG(&g_I2cClient->dev, "nTouchKeyCode = %d, nLastKeyCode = %d\n", nTouchKeyCode, nLastKeyCode);
                    
                    nLastKeyCode = nTouchKeyCode;

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
                    input_report_key(g_InputDevice, nTouchKeyCode, 1);

                    input_sync(g_InputDevice);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL 
                    _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#endif //CONFIG_TP_HAVE_KEY
            }
            else
            {
                DBG(&g_I2cClient->dev, "tInfo->nFingerNum = %d...............\n", tInfo.nFingerNum);
                
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                for (i = 0; i < MAX_TOUCH_NUM; i ++) 
                {
                    if (tInfo.nFingerNum != 0)
                    {
                        if (_gCurrPress[i])
                        {
			    input_report_key(g_InputDevice, BTN_TOUCH, 1);
			    DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, 0, i);
                            input_report_key(g_InputDevice, BTN_TOOL_FINGER, 1);
                        }
                        else if (0 == _gCurrPress[i] && 1 == _gPriorPress[i])
                        {
                            DrvPlatformLyrFingerTouchReleased(0, 0, i);
                        }
                    }
			 _gPriorPress[i] = _gCurrPress[i];
                }
#else // TYPE A PROTOCOL
                for (i = 0; i < tInfo.nFingerNum; i ++) 
                {
                    DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, 0, 0);
                }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                input_sync(g_InputDevice);
            }
        }

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
        if (g_IsEnableReportRate == 1)
        {
            if (4294967295UL == g_ValidTouchCount)
            {
                g_ValidTouchCount = 0; // Reset count if overflow
                DBG(&g_I2cClient->dev, "g_ValidTouchCount reset to 0\n");
            } 	

            g_ValidTouchCount ++;

            DBG(&g_I2cClient->dev, "g_ValidTouchCount = %d\n", g_ValidTouchCount);
        }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE
    }

    TouchHandleEnd: 
    	
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
}

void _DrvFwCtrlMutualHandleFingerTouch(void) // for MSG26xxM/MSG28xx
{
    MutualTouchInfo_t tInfo;
    u32 i = 0;
#ifdef CONFIG_TP_HAVE_KEY
    static u32 nLastKeyCode = 0xFF;
#endif //CONFIG_TP_HAVE_KEY
    static u32 nLastCount = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;

//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);  // add for debug
    
    if (_gIsDisableFinagerTouch == 1)
    {
        DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
        return;
    }

    mutex_lock(&g_Mutex); 
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    memset(&tInfo, 0x0, sizeof(MutualTouchInfo_t));

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
#ifdef CONFIG_ENABLE_HOTKNOT    //demo mode
            if (g_HotKnotState == HOTKNOT_TRANS_STATE)             
            {
                nReportPacketLength = DEMO_HOTKNOT_SEND_RET_LEN;
                pPacket = g_DemoModeHotKnotSndRetPacket;
            }
            else
#endif //CONFIG_ENABLE_HOTKNOT		
            {
                DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

                nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
                pPacket = g_DemoModePacket;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEBUG_MODE\n");

            if (g_MutualFirmwareInfo.nLogModePacketHeader != 0xA5 && g_MutualFirmwareInfo.nLogModePacketHeader != 0xAB && g_MutualFirmwareInfo.nLogModePacketHeader != 0xA7)
            {
                DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER : 0x%x\n", g_MutualFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }
        
#ifdef CONFIG_ENABLE_HOTKNOT    //debug mode
            if (g_HotKnotState == HOTKNOT_TRANS_STATE)             
            {
                nReportPacketLength = DEBUG_HOTKNOT_SEND_RET_LEN;
                pPacket = g_DebugModeHotKnotSndRetPacket;        
            }
            else
#endif //CONFIG_ENABLE_HOTKNOT	            
            {
                nReportPacketLength = g_MutualFirmwareInfo.nLogModePacketLength;
                pPacket = g_LogModePacket;
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            goto TouchHandleEnd;		
        }
    }
    else
    {
#ifdef CONFIG_ENABLE_HOTKNOT    //demo mode
        if (g_HotKnotState == HOTKNOT_TRANS_STATE)			  
        {
            nReportPacketLength = DEMO_HOTKNOT_SEND_RET_LEN;
            pPacket = g_DemoModeHotKnotSndRetPacket;
        }
        else
#endif //CONFIG_ENABLE_HOTKNOT        
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = g_DemoModePacket;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureDebugMode == 1 && g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture debug mode packet length, g_ChipType=0x%x\n", g_ChipType);

        if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) 
        {
            nReportPacketLength = GESTURE_DEBUG_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture debug mode.\n");
            goto TouchHandleEnd;		
        }
    }
    else if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length\n");

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
        nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        pPacket = _gGestureWakeupPacket;
    }

#else

    if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length\n");

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
        nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        pPacket = _gGestureWakeupPacket;
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        s32 rc;
        
        while (i < 5)
        {
            mdelay(50);

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc > 0)
            {
                break;
            }
            
            i ++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        if (0 != _DrvFwCtrlReadFingerTouchData(&pPacket[0], nReportPacketLength))
        {
            goto TouchHandleEnd;		
        }
    }
#else
    if (0 != _DrvFwCtrlReadFingerTouchData(&pPacket[0], nReportPacketLength))
    {
         goto TouchHandleEnd;		
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    if (0 != _DrvFwCtrlReadFingerTouchData(&pPacket[0], nReportPacketLength))
    {
        goto TouchHandleEnd;		
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    {
        s32 nRetVal = 0;
        
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
        
        nRetVal = _DrvFwCtrlReadFingerTouchData(&pPacket[0], nReportPacketLength);
        if (0 != nRetVal)
        {
            goto TouchHandleEnd;		
        }
    }
#endif

    if (0 == _DrvFwCtrlMutualParsePacket(pPacket, nReportPacketLength, &tInfo))
    {
#ifdef CONFIG_TP_HAVE_KEY
        if (tInfo.nKeyCode != 0xFF)   //key touch pressed
        {
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
            DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, g_TpVirtualKey[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, g_TpVirtualKey[tInfo.nKeyCode]);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            if (tpd_dts_data.use_tpd_button)
            {
                DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, tpd_dts_data.tpd_key_local[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, tpd_dts_data.tpd_key_local[tInfo.nKeyCode]);
            }
#else
            DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, g_TpVirtualKey[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, g_TpVirtualKey[tInfo.nKeyCode]);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
         
            if (tInfo.nKeyCode < MAX_KEY_NUM)
            {
                if (tInfo.nKeyCode != nLastKeyCode)
                {
                    DBG(&g_I2cClient->dev, "key touch pressed\n");

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                    input_report_key(g_InputDevice, g_TpVirtualKey[tInfo.nKeyCode], 1);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                    if (tpd_dts_data.use_tpd_button)
                    {
                        input_report_key(g_InputDevice, tpd_dts_data.tpd_key_local[tInfo.nKeyCode], 1);
                    }
#else
                    input_report_key(g_InputDevice, g_TpVirtualKey[tInfo.nKeyCode], 1);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
                    input_sync(g_InputDevice);

                    nLastKeyCode = tInfo.nKeyCode;
                }
                else
                {
                    /// pass duplicate key-pressing
                    DBG(&g_I2cClient->dev, "REPEATED KEY\n");
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "WRONG KEY\n");
            }
        }
        else                        //key touch released
        {
            if (nLastKeyCode != 0xFF)
            {
                DBG(&g_I2cClient->dev, "key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                input_report_key(g_InputDevice, g_TpVirtualKey[nLastKeyCode], 0);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    input_report_key(g_InputDevice, tpd_dts_data.tpd_key_local[nLastKeyCode], 0);
                }
#else
                input_report_key(g_InputDevice, g_TpVirtualKey[nLastKeyCode], 0);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
                input_sync(g_InputDevice);
                
                nLastKeyCode = 0xFF;
            }
        }
#endif //CONFIG_TP_HAVE_KEY

        DBG(&g_I2cClient->dev, "tInfo.nCount = %d, nLastCount = %d\n", tInfo.nCount, nLastCount);

        if (tInfo.nCount > 0)          //point touch pressed
        {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            for (i = 0; i < tInfo.nCount; i ++)
            {
                DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, tInfo.tPoint[i].nP, tInfo.tPoint[i].nId);

                input_report_key(g_InputDevice, BTN_TOUCH, 1); 
                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 1); 	
            }

            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                DBG(&g_I2cClient->dev, "_gPreviousTouch[%d]=%d, _gCurrentTouch[%d]=%d\n", i, _gPreviousTouch[i], i, _gCurrentTouch[i]); // TODO : add for debug

                if (_gCurrentTouch[i] == 0 && _gPreviousTouch[i] == 1)
                {
                    DrvPlatformLyrFingerTouchReleased(0, 0, i);
                }
                _gPreviousTouch[i] = _gCurrentTouch[i];
            }
#else // TYPE A PROTOCOL
            for (i = 0; i < tInfo.nCount; i ++)
            {
                DrvPlatformLyrFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, tInfo.tPoint[i].nP, tInfo.tPoint[i].nId);
            }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

            input_sync(g_InputDevice);

            nLastCount = tInfo.nCount;
        }
        else                        //point touch released
        {
            if (nLastCount > 0)
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    DBG(&g_I2cClient->dev, "_gPreviousTouch[%d]=%d, _gCurrentTouch[%d]=%d\n", i, _gPreviousTouch[i], i, _gCurrentTouch[i]); // TODO : add for debug

                    if (_gCurrentTouch[i] == 0 && _gPreviousTouch[i] == 1)
                    {
                        DrvPlatformLyrFingerTouchReleased(0, 0, i);
                    }
                    _gPreviousTouch[i] = _gCurrentTouch[i];
                }

                input_report_key(g_InputDevice, BTN_TOUCH, 0);                      
                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 0); 	
#else // TYPE A PROTOCOL
                DrvPlatformLyrFingerTouchReleased(0, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    
                input_sync(g_InputDevice);

                nLastCount = 0;
            }
        }

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
        if (g_IsEnableReportRate == 1)
        {
            if ( 4294967295UL== g_ValidTouchCount)
            {
                g_ValidTouchCount = 0; // Reset count if overflow
                DBG(&g_I2cClient->dev, "g_ValidTouchCount reset to 0\n");
            } 	

            g_ValidTouchCount ++;

            DBG(&g_I2cClient->dev, "g_ValidTouchCount = %d\n", g_ValidTouchCount);
        }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE
    }

    TouchHandleEnd: 
    	
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
}

void DrvFwCtrlHandleFingerTouch(void)
{
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        _DrvFwCtrlSelfHandleFingerTouch();
    }
    else if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX)
    {
        _DrvFwCtrlMutualHandleFingerTouch();
    }
}
