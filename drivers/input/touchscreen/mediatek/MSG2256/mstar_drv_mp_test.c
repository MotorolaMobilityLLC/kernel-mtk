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
 * @file    mstar_drv_mp_test.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
#include "mstar_drv_mp_test.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_fw_control.h"
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_ic_fw_porting_layer.h"

#ifdef CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
/* The below .h file are included for MSG21xxA */
// Modify.
#include "msg21xxa_open_test_ANA1_X.h"
#include "msg21xxa_open_test_ANA2_X.h"
#include "msg21xxa_open_test_ANA1_B_X.h"
#include "msg21xxa_open_test_ANA2_B_X.h"
#include "msg21xxa_open_test_ANA3_X.h"

#include "msg21xxa_open_test_ANA1_Y.h"
#include "msg21xxa_open_test_ANA2_Y.h"
#include "msg21xxa_open_test_ANA1_B_Y.h"
#include "msg21xxa_open_test_ANA2_B_Y.h"
#include "msg21xxa_open_test_ANA3_Y.h"

// Modify.
#include "msg21xxa_short_test_ANA1_X.h"
#include "msg21xxa_short_test_ANA2_X.h"
#include "msg21xxa_short_test_ANA3_X.h"
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE  
#include "msg21xxa_short_test_ANA4_X.h"
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

#include "msg21xxa_short_test_ANA1_Y.h"
#include "msg21xxa_short_test_ANA2_Y.h"
#include "msg21xxa_short_test_ANA3_Y.h"
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE  
#include "msg21xxa_short_test_ANA4_Y.h"
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
/* The below .h file are included for MSG22xx */
// Modify.
#include "open_test_RIU1_HLT.h"
#include "open_test_RIU2_HLT.h"
#include "open_test_RIU3_HLT.h"

//#include "msg22xx_open_test_RIU1_Y.h"
//#include "msg22xx_open_test_RIU2_Y.h"
//#include "msg22xx_open_test_RIU3_Y.h"

// Modify.
#include "short_test_RIU1_HLT.h"
#include "short_test_RIU2_HLT.h"
//#include "short_test_RIU3_HLT.h"
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE  
#include "short_test_RIU4_HLT.h"
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

//#include "msg22xx_short_test_RIU1_Y.h"
//#include "msg22xx_short_test_RIU2_Y.h"
//#include "msg22xx_short_test_RIU3_Y.h"
//#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE  
//#include "msg22xx_short_test_RIU4_Y.h"
//#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
/* The below .h file are included for MSG26xxM */
#include "msg26xxm_open_test_X.h"
#include "msg26xxm_short_test_X.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
/* The below .h file are included for MSG28xx */
#include "msg28xx_mp_test_X.h"
#include "msg28xx_mp_test_Y.h"
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

/*=============================================================*/
// PREPROCESSOR CONSTANT DEFINITION
/*=============================================================*/
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
#define NUM_OPEN_1_SENSOR_HLT_UP 20
#define NUM_OPEN_2_SENSOR_HLT_DOWN 20
s16 Golden_0[NUM_OPEN_1_SENSOR_HLT_UP] = {1166,1950,1978,2007,2035,2052,2086,2056,2036,1263,657,1492,1468,1446,1463,1475,1485,1505,1533,723};
s16 Golden_1[NUM_OPEN_2_SENSOR_HLT_DOWN] = {1169,1945,1972,2010,2035,2060,2086,2052,2037,1257,660,1488,1476,1451,1461,1471,1485,1505,1534,725};
//-bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
// Modify.
#define TP_TYPE_HLT    (1) //(2)
//#define TP_TYPE_Y    (4)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern u32 SLAVE_I2C_ID_DBBUS;
extern u32 SLAVE_I2C_ID_DWI2C;

extern u8 g_ChipType;

extern struct i2c_client *g_I2cClient;
extern struct mutex g_Mutex;

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

u32 g_IsInMpTest = 0;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static u32 _gTestRetryCount = CTP_MP_TEST_RETRY_COUNT;
static ItoTestMode_e _gItoTestMode = 0;

static s32 _gCtpMpTestStatus = ITO_TEST_UNDER_TESTING;

static u32 _gTestFailChannelCount = 0;

static struct work_struct _gCtpItoTestWork;
static struct workqueue_struct *_gCtpMpTestWorkQueue = NULL;

ItoTestResult_e _gItoTestResult = ITO_TEST_OK;

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
/* The below variable are defined for MSG21xxA/MSG22xx */
static u8 _gSelfICTestFailChannel[SELF_IC_MAX_CHANNEL_NUM] = {0};

static s16 _gSelfICRawData1[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s16 _gSelfICRawData2[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s16 _gSelfICRawData3[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s16 _gSelfICRawData4[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s8 _gSelfICDataFlag1[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s8 _gSelfICDataFlag2[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s8 _gSelfICDataFlag3[SELF_IC_MAX_CHANNEL_NUM] = {0};
static s8 _gSelfICDataFlag4[SELF_IC_MAX_CHANNEL_NUM] = {0};

static s16 _gRawDataLog[SELF_IC_MAX_CHANNEL_NUM*2+1] = {0};

static u8 _gSelfICItoTestKeyNum = 0;
static u8 _gSelfICItoTestDummyNum = 0;
static u8 _gSelfICItoTestTriangleNum = 0;
static u8 _gSelfICIsEnable2R = 0;

static s16 *_gSelfIC_MAP1 = NULL;
static s16 *_gSelfIC_MAP2 = NULL;
static s16 *_gSelfIC_MAP3 = NULL;
static u8 *_gSelfIC_MAP40_1 = NULL;
static u8 *_gSelfIC_MAP40_2 = NULL;
static u8 *_gSelfIC_MAP41_1 = NULL;
static u8 *_gSelfIC_MAP41_2 = NULL;

static u8 *_gSelfIC_SHORT_MAP1 = NULL;
static u8 *_gSelfIC_SHORT_MAP2 = NULL;
static u8 *_gSelfIC_SHORT_MAP3 = NULL;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
static u8 *_gSelfIC_MAP40_3 = NULL;
static u8 *_gSelfIC_MAP40_4 = NULL;
static u8 *_gSelfIC_MAP41_3 = NULL;
static u8 *_gSelfIC_MAP41_4 = NULL;

static u8 *_gSelfIC_SHORT_MAP4 = NULL;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
static u8 _gMsg21xxaLTP = 1;	

// _gMsg21xxaOpen1~_gMsg21xxaOpen3 are for MSG21XXA
static u16 *_gMsg21xxaOpen1 = NULL;
static u16 *_gMsg21xxaOpen1B = NULL;
static u16 *_gMsg21xxaOpen2 = NULL;
static u16 *_gMsg21xxaOpen2B = NULL;
static u16 *_gMsg21xxaOpen3 = NULL;

// _gMsg21xxaShort_1~_gMsg21xxaShort_4 are for MSG21XXA
static u16 *_gMsg21xxaShort_1 = NULL;
static u16 *_gMsg21xxaShort_2 = NULL;
static u16 *_gMsg21xxaShort_3 = NULL;

// _gMsg21xxaShort_1_GPO~_gMsg21xxaShort_4_GPO are for MSG21XXA
static u16 *_gMsg21xxaShort_1_GPO = NULL;
static u16 *_gMsg21xxaShort_2_GPO = NULL;
static u16 *_gMsg21xxaShort_3_GPO = NULL;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
static u16 *_gMsg21xxaShort_4 = NULL;
static u16 *_gMsg21xxaShort_4_GPO = NULL;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
static u8 _gIsOldFirmwareVersion = 0;

// _gMsg22xxOpenRIU1~_gMsg22xxOpenRIU3 are for MSG22XX
static u32 *_gMsg22xxOpenRIU1 = NULL;
static u32 *_gMsg22xxOpenRIU2 = NULL;
static u32 *_gMsg22xxOpenRIU3 = NULL;

// _gMsg22xxShort_RIU1~_gMsg22xxShort_RIU4 are for MSG22XX
static u32 *_gMsg22xxShort_RIU1 = NULL;
static u32 *_gMsg22xxShort_RIU2 = NULL;
static u32 *_gMsg22xxShort_RIU3 = NULL;

// _gMsg22xxOpenSubFrameNum1~_gMsg22xxShortSubFrameNum4 are for MSG22XX
static u8 _gMsg22xxOpenSubFrameNum1 = 0;
static u8 _gMsg22xxOpenSubFrameNum2 = 0;
static u8 _gMsg22xxOpenSubFrameNum3 = 0;
static u8 _gMsg22xxShortSubFrameNum1 = 0;
static u8 _gMsg22xxShortSubFrameNum2 = 0;
static u8 _gMsg22xxShortSubFrameNum3 = 0;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
static u32 *_gMsg22xxShort_RIU4 = NULL;
static u8 _gMsg22xxShortSubFrameNum4 = 0;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX


#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
/* The below variable are defined for MSG26xxM/MSG28xx */
TestScopeInfo_t g_TestScopeInfo = {0}; // Used for MSG26xxM/MSG28xx

static u16 _gMutualICSenseLineNum = 0;
static u16 _gMutualICDriveLineNum = 0;
static u16 _gMutualICWaterProofNum = 0;

static u8 _gMutualICTestFailChannel[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};

static s32 _gMutualICDeltaC[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICResult[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICResultWater[12] = {0};
//static u8 _gMutualICMode[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICSenseR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gMutualICDriveR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
static u8 _gMutualICTestAutoSwitchFlag = 1;
static u8 _gMutualICTestSwitchMode = 0;

static s32 _gMutualICTempDeltaC[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};

static u8 _gMsg26xxmShortTestChannel[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
u16 _gMsg28xxMuxMem_20_3E_0_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_1_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_2_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_3_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_4_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_5_Settings[16] = {0};
u16 _gMsg28xxMuxMem_20_3E_6_Settings[16] = {0};

static s32 _gMutualICOnCellOpenTestResult[2] = {0};
static s32 _gMutualICOnCellOpenTestResultData[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestResultRatioData[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestGoldenChannel[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestGoldenChannelMax[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestGoldenChannelMin[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static u16 _gNormalTestFailCheckDeltac[MUTUAL_IC_MAX_MUTUAL_NUM];
static u16 _gNormalTestFailCheckRatio1000[MUTUAL_IC_MAX_MUTUAL_NUM];
static s32 _gMutualICOnCellOpenTestRatio1000[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestRatioBorder1000[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestRatioMove1000[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICOnCellOpenTestRatioBorderMove1000[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static u16 _gNormalTestFailCheckDeltac[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static u16 _gNormalTestFailCheckRatio[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gMutualICDeltaC2[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};
static s32 _gItoShortRData[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gItoShortResultData[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gItoShortFailChannel[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gIcPinShortResultData[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static u16 _gIcPinShortSenceR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gIcPinShortFailChannel[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gMutualICDeltaCWater[12] = {0};
static s32 _gMutualICGRR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
static s32 _gKeyArray[3] = {0};

// Used for MSG28xx TP type definition
static u16 _gMsg28xx_SENSE_NUM = 0;
static u16 _gMsg28xx_DRIVE_NUM = 0;
static u16 _gMsg28xx_KEY_NUM = 0;
static u16 _gMsg28xx_KEY_LINE = 0;
static u16 _gMsg28xx_GR_NUM = 0;
static u16 _gMsg28xx_CSUB_REF = 0;
static u16 _gMsg28xx_CFB_REF = 0;
static u16 _gMsg28xx_SENSE_MUTUAL_SCAN_NUM = 0;
static u16 _gMsg28xx_MUTUAL_KEY = 0;
static u16 _gMsg28xx_PATTERN_TYPE = 0;
static u16 _gMsg28xx_DC_RANGE = 0;
static u16 _gMsg28xx_DC_RANGE_UP = 0;
static u16 _gMsg28xx_DC_Ratio_1000 = 0;
static u16 _gMsg28xx_DC_Ratio_UP_1000 = 0;
static u16 _gMsg28xx_DC_Border_Ratio_1000 = 0;
static u16 _gMsg28xx_DC_Border_Ratio_UP_1000 = 0;
static u16 _gMsg28xx_PATTERN_MODEL = 0;
static u16 _gMsg28xx_SENSOR_KEY_CH = 0;
static s16 _gIcPinShortCheckFail = 0;
static s16 _gItoShortCheckFail = 0;

static u16 _gMsg28xx_SHORT_N1_TEST_NUMBER = 0;
static u16 _gMsg28xx_SHORT_N2_TEST_NUMBER = 0;
static u16 _gMsg28xx_SHORT_S1_TEST_NUMBER = 0;
static u16 _gMsg28xx_SHORT_S2_TEST_NUMBER = 0;
static u16 _gMsg28xx_SHORT_TEST_5_TYPE = 0;
static u16 _gMsg28xx_SHORT_X_TEST_NUMBER = 0;
                                                                                                                                    
static u16 * _gMsg28xx_SHORT_N1_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_N1_MUX_MEM_20_3E = NULL;
static u16 * _gMsg28xx_SHORT_N2_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_N2_MUX_MEM_20_3E = NULL;
static u16 * _gMsg28xx_SHORT_S1_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_S1_MUX_MEM_20_3E = NULL;
static u16 * _gMsg28xx_SHORT_S2_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_S2_MUX_MEM_20_3E = NULL;
static u16 * _gMsg28xx_SHORT_X_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_X_MUX_MEM_20_3E = NULL;                                                           
static u16 * _gMsg28xx_SHORT_GR_TEST_PIN = NULL;
static u16 * _gMsg28xx_SHORT_GR_MUX_MEM_20_3E = NULL;

static u16 * _gMsg28xx_SHORT_IcPIN_MUX_MEM_1_Settings = NULL;
static u16 * _gMsg28xx_SHORT_IcPIN_MUX_MEM_2_Settings = NULL;
static u16 * _gMsg28xx_SHORT_IcPIN_MUX_MEM_3_Settings = NULL;
static u16 * _gMsg28xx_SHORT_IcPIN_MUX_MEM_4_Settings = NULL;
static u16 * _gMsg28xx_SHORT_IcPIN_MUX_MEM_5_Settings = NULL;

static u16 * _gMsg28xx_PAD_TABLE_DRIVE = NULL;
static u16 * _gMsg28xx_PAD_TABLE_SENSE = NULL;
static u16 * _gMsg28xx_PAD_TABLE_GR = NULL;
static u16 * _gMsg28xx_SENSE_PAD_PIN_MAPPING = NULL;
static u8 * _gMsg28xx_KEYSEN = NULL;
static u8 * _gMsg28xx_KEYDRV = NULL;
                                                            
//static u8 ** g_Msg28xxMapVaMutual;
static u16 _gMsg28xxTpType = 0;
static s8 _gMsg28xxDeepStandBy = 0;
static u16 _gMsg28xxDeepStandByTimeOut = 0;
static u16 _gMsg28xx_SHORT_VALUE = 0;
static u16 _gMsg28xx_ICPIN_SHORT = 0;
static u16 _gMsg28xx_SupportIC = 0;
static u16 _gMsg28xx_OPEN_MODE = 0;
static u16 _gMsg28xx_CHARGE_PUMP = 0;
static u16 _gMsg28xx_KEYDRV_O = 0;
static u16 _gMsg28xx_SUB_FRAME = 0;
static s32 _gMsg28xx_FIXED_CARRIER = 0;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
 
/*=============================================================*/
// EXTERN FUNCTION DECLARATION
/*=============================================================*/


/*=============================================================*/
// LOCAL FUNCTION DECLARATION
/*=============================================================*/

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
static u16 _DrvMpTestItoTestSelfICGetTpType(void);
static u16 _DrvMpTestItoTestSelfICChooseTpType(void);
static void _DrvMpTestItoTestSelfICAnaSwReset(void);
static u8 _DrvMpTestSelfCheckValueDoubleRange(int nValue, int nMax, int nMin);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
static u8 _DrvMpTestMutualICCheckValueInRange(s32 nValue, s32 nMax, s32 nMin);
static void _DrvMpTestMutualICDebugShowArray(void *pBuf, u16 nLen, int nDataType, int nCarry, int nChangeLine);
//static void _DrvMpTestMutualICDebugShowS32Array(s32 *pBuf, u16 nRow, u16 nCol);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
static u16 _DrvMpTestItoTestMsg21xxaGetNum(void)
{
    u32 i;
    u16 nSensorNum = 0;
    u16 nRegVal1, nRegVal2;
 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nRegVal1 = RegGet16BitValue(0x114A); //bank:ana, addr:h0025  
    DBG(&g_I2cClient->dev, "nRegValue1 = %d\n", nRegVal1);
    
    if ((nRegVal1 & BIT1) == BIT1)
    {
        nRegVal1 = RegGet16BitValue(0x120A); //bank:ana2, addr:h0005  			
        nRegVal1 = nRegVal1 & 0x0F;
    	
        nRegVal2 = RegGet16BitValue(0x1216); //bank:ana2, addr:h000b    		
        nRegVal2 = ((nRegVal2 >> 1) & 0x0F) + 1;
    	
        nSensorNum = nRegVal1 * nRegVal2;
    }
    else
    {
        for (i = 0; i < 4; i ++)
        {
            nSensorNum += (RegGet16BitValue(0x120A)>>(4*i))&0x0F; //bank:ana2, addr:h0005  
        }
    }
    DBG(&g_I2cClient->dev, "nSensorNum = %d\n", nSensorNum);

    return nSensorNum;        
}

static void _DrvMpTestItoTestMsg21xxaDisableFilterNoiseDetect(void)
{
    u16 nRegValue;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
     
    // Disable DIG/ANA drop
    nRegValue = RegGet16BitValue(0x1302); 
      
    RegSet16BitValue(0x1302, nRegValue & (~(BIT2 | BIT1 | BIT0)));      
}

static void _DrvMpTestItoTestMsg21xxaPolling(void)
{
    u16 nRegInt = 0x0000;
    u16 nRegVal;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x130C, BIT15); //bank:fir, addr:h0006         
    RegSet16BitValue(0x1214, (RegGet16BitValue(0x1214) | BIT0)); //bank:ana2, addr:h000a        
            
    DBG(&g_I2cClient->dev, "polling start\n");

    do
    {
        nRegInt = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    } while((nRegInt & SELF_IC_FIQ_E_FRAME_READY_MASK) == 0x0000);

    DBG(&g_I2cClient->dev, "polling end\n");
    
    nRegVal = RegGet16BitValue(0x3D18); 
    RegSet16BitValue(0x3D18, nRegVal & (~SELF_IC_FIQ_E_FRAME_READY_MASK));      
}

static void _DrvMpTestItoOpenTestMsg21xxaSetV(u8 nEnable, u8 nPrs)	
{
    u16 nRegVal;        
    
    DBG(&g_I2cClient->dev, "*** %s() nEnable = %d, nPrs = %d ***\n", __func__, nEnable, nPrs);
    
    nRegVal = RegGet16BitValue(0x1208); //bank:ana2, addr:h0004
    nRegVal = nRegVal & 0xF1; 							
    
    if (nPrs == 0)
    {
        RegSet16BitValue(0x1208, nRegVal|0x0C); 		
    }
    else if (nPrs == 1)
    {
        RegSet16BitValue(0x1208, nRegVal|0x0E); 		     	
    }
    else
    {
        RegSet16BitValue(0x1208, nRegVal|0x02); 			
    }    
    
    if (nEnable)
    {
        nRegVal = RegGet16BitValue(0x1106); //bank:ana, addr:h0003  
        RegSet16BitValue(0x1106, nRegVal|0x03);   	
    }
    else
    {
        nRegVal = RegGet16BitValue(0x1106);    
        nRegVal = nRegVal & 0xFC;					
        RegSet16BitValue(0x1106, nRegVal);         
    }
}

static u16 _DrvMpTestItoTestMsg21xxaGetDataOut(s16 *pRawData)
{
    u32 i;
    u16 szRawData[SELF_IC_MAX_CHANNEL_NUM] = {0};
    u16 nSensorNum;
    u16 nRegInt;
    u8  szDbBusTxData[8] = {0};
    u8  szDbBusRxData[SELF_IC_MAX_CHANNEL_NUM*2] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nSensorNum = _DrvMpTestItoTestMsg21xxaGetNum();
    
    if ((nSensorNum*2) > (SELF_IC_MAX_CHANNEL_NUM*2))
    {
        DBG(&g_I2cClient->dev, "Danger. nSensorNum = %d\n", nSensorNum);
        return nSensorNum;
    }

    nRegInt = RegGet16BitValue((0x3d<<8) | (MSG21XXA_REG_INTR_FIQ_MASK<<1)); 
    RegSet16BitValue((0x3d<<8) | (MSG21XXA_REG_INTR_FIQ_MASK<<1), (nRegInt & (u16)(~SELF_IC_FIQ_E_FRAME_READY_MASK))); 
    
    _DrvMpTestItoTestMsg21xxaPolling();
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x13; //bank:fir, addr:h0020 
    szDbBusTxData[2] = 0x40;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    mdelay(20);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szDbBusRxData[0], (nSensorNum * 2));
    mdelay(100);
    
    for (i = 0; i < nSensorNum * 2; i ++)
    {
        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = %d\n", i, szDbBusRxData[i]); // add for debug
    }
 
    nRegInt = RegGet16BitValue((0x3d<<8) | (MSG21XXA_REG_INTR_FIQ_MASK<<1)); 
    RegSet16BitValue((0x3d<<8) | (MSG21XXA_REG_INTR_FIQ_MASK<<1), (nRegInt | (u16)SELF_IC_FIQ_E_FRAME_READY_MASK)); 

    for (i = 0; i < nSensorNum; i ++)
    {
        szRawData[i] = (szDbBusRxData[2 * i + 1] << 8 ) | (szDbBusRxData[2 * i]);
        pRawData[i] = (s16)szRawData[i];
    }
    
    return nSensorNum;
}

static void _DrvMpTestItoTestMsg21xxaSendDataIn(u8 nStep)
{
    u32	i;
    u16 *pType = NULL;        
    u8 	szDbBusTxData[512] = {0};

    DBG(&g_I2cClient->dev, "*** %s() nStep = %d ***\n", __func__, nStep);

    if (nStep == 1) //39-1
    {
        pType = &_gMsg21xxaShort_1[0];      	
    }
    else if (nStep == 2) //39-2
    {
        pType = &_gMsg21xxaShort_2[0];      	
    }
    else if (nStep == 3) //39-3
    {
        pType = &_gMsg21xxaShort_3[0];        
    }
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nStep == 0) //39-4 (2R)
    {
        pType = &_gMsg21xxaShort_4[0];
    }
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nStep == 4)
    {
        pType = &_gMsg21xxaOpen1[0];        
    }
    else if (nStep == 5)
    {
        pType = &_gMsg21xxaOpen2[0];      	
    }
    else if (nStep == 6)
    {
        pType = &_gMsg21xxaOpen3[0];      	
    }
    else if (nStep == 9)
    {
        pType = &_gMsg21xxaOpen1B[0];        
    }
    else if (nStep == 10)
    {
        pType = &_gMsg21xxaOpen2B[0];      	
    } 
     
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x11; //bank:ana, addr:h0000
    szDbBusTxData[2] = 0x00;    
    for (i = 0; i <= 0x3E ; i ++)
    {
        szDbBusTxData[3+2*i] = pType[i] & 0xFF;
        szDbBusTxData[4+2*i] = (pType[i] >> 8) & 0xFF;    	
    }
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+0x3F*2);

    szDbBusTxData[2] = 0x7A * 2; //bank:ana, addr:h007a
    for (i = 0x7A; i <= 0x7D ; i ++)
    {
        szDbBusTxData[3+2*(i-0x7A)] = 0;
        szDbBusTxData[4+2*(i-0x7A)] = 0;    	    	
    }
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+8);
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x12; //bank:ana2, addr:h0005
      
    szDbBusTxData[2] = 0x05 * 2; 
    szDbBusTxData[3] = pType[128+0x05] & 0xFF;
    szDbBusTxData[4] = (pType[128+0x05] >> 8) & 0xFF;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 5);
    
    szDbBusTxData[2] = 0x0B * 2; //bank:ana2, addr:h000b
    szDbBusTxData[3] = pType[128+0x0B] & 0xFF;
    szDbBusTxData[4] = (pType[128+0x0B] >> 8) & 0xFF;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 5);
    
    szDbBusTxData[2] = 0x12 * 2; //bank:ana2, addr:h0012
    szDbBusTxData[3] = pType[128+0x12] & 0xFF;
    szDbBusTxData[4] = (pType[128+0x12] >> 8) & 0xFF;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 5);
    
    szDbBusTxData[2] = 0x15 * 2; //bank:ana2, addr:h0015
    szDbBusTxData[3] = pType[128+0x15] & 0xFF;
    szDbBusTxData[4] = (pType[128+0x15] >> 8) & 0xFF;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 5);

    if (_gItoTestMode == ITO_TEST_MODE_OPEN_TEST)
    {    	
//#if 1 //for AC mod
        szDbBusTxData[1] = 0x13;
        szDbBusTxData[2] = 0x12 * 2;
        szDbBusTxData[3] = 0x30;
        szDbBusTxData[4] = 0x30;
        IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 5);        
        
        szDbBusTxData[2] = 0x14 * 2;
        szDbBusTxData[3] = 0X30;
        szDbBusTxData[4] = 0X30;
        IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 5);     
        
        szDbBusTxData[1] = 0x12;
        for (i = 0x0D; i <= 0x10; i ++) //for AC noise(++)
        {
            szDbBusTxData[2] = i * 2;
            szDbBusTxData[3] = pType[128+i] & 0xFF;
            szDbBusTxData[4] = (pType[128+i] >> 8) & 0xFF;
            IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 5);  
        }

        for (i = 0x16; i <= 0x18; i ++) //for AC noise
        {
	          szDbBusTxData[2] = i * 2;
	          szDbBusTxData[3] = pType[128+i] & 0xFF;
	          szDbBusTxData[4] = (pType[128+i] >> 8) & 0xFF;
	          IicWriteData(SLAVE_I2C_ID_DBBUS, szDbBusTxData, 5);  
        }
//#endif
    }
}

static void _DrvMpTestItoOpenTestMsg21xxaSetC(u8 nCSubStep)
{
    u32 i;
    u8 szDbBusTxData[SELF_IC_MAX_CHANNEL_NUM+3];
    u8 nHighLevelCSub = 0;
    u8 nCSubNew;
     
    DBG(&g_I2cClient->dev, "*** %s() nCSubStep = %d ***\n", __func__, nCSubStep);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x11; //bank:ana, addr:h0042       
    szDbBusTxData[2] = 0x84;        
    
    for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
    {
        nCSubNew = nCSubStep;        
        nHighLevelCSub = 0;   
        
        if (nCSubNew > 0x1F)
        {
            nCSubNew = nCSubNew - 0x14;
            nHighLevelCSub = 1;
        }
           
        szDbBusTxData[3+i] = nCSubNew & 0x1F;        
        if (nHighLevelCSub == 1)
        {
            szDbBusTxData[3+i] |= BIT5;
        }
    }
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], SELF_IC_MAX_CHANNEL_NUM+3);

    szDbBusTxData[2] = 0xB4; //bank:ana, addr:h005a        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], SELF_IC_MAX_CHANNEL_NUM+3);
}

static void _DrvMpTestItoOpenTestMsg21xxaFirst(u8 nItemId, s16 *pRawData, s8 *pDataFlag)		
{
    u32 i, j;
    s16 szTmpRawData[SELF_IC_MAX_CHANNEL_NUM] = {0};
    u16	nRegVal;
    u8  nLoop;
    u8  nSensorNum1 = 0, nSensorNum2 = 0, nTotalSensor = 0;
    s16 *pMapping = NULL;
    
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);
	
    // Stop cpu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

    RegSet16BitValue(0x1E24, 0x0500); //bank:chip, addr:h0012
    RegSet16BitValue(0x1E2A, 0x0000); //bank:chip, addr:h0015
    RegSet16BitValue(0x1EE6, 0x6E00); //bank:chip, addr:h0073
    RegSet16BitValue(0x1EE8, 0x0071); //bank:chip, addr:h0074
	    
    if (nItemId == 40)    			
    {
        pMapping = _gSelfIC_MAP1;
        if (_gSelfICIsEnable2R)
        {
            nTotalSensor = _gSelfICItoTestTriangleNum/2; 
        }
        else
        {
            nTotalSensor = _gSelfICItoTestTriangleNum/2 + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum;
        }
    }
    else if (nItemId == 41)    		
    {
        pMapping = _gSelfIC_MAP2;
        if (_gSelfICIsEnable2R)
        {
            nTotalSensor = _gSelfICItoTestTriangleNum/2; 
        }
        else
        {
            nTotalSensor = _gSelfICItoTestTriangleNum/2 + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum;
        }
    }
    else if (nItemId == 42)    		
    {
        pMapping = _gSelfIC_MAP3;      
        nTotalSensor = _gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum; 
    }
        	    
    nLoop = 1;
    if (nItemId != 42)
    {
        if (nTotalSensor > 11)
        {
            nLoop = 2;
        }
    }	
    
    DBG(&g_I2cClient->dev, "nLoop = %d\n", nLoop);
	
    for (i = 0; i < nLoop; i ++)
    {
        if (i == 0)
        {
            _DrvMpTestItoTestMsg21xxaSendDataIn(nItemId - 36);
        }
        else
        { 
            if (nItemId == 40)
            { 
                _DrvMpTestItoTestMsg21xxaSendDataIn(9);
            }
            else
            { 		
                _DrvMpTestItoTestMsg21xxaSendDataIn(10);
            }
        }
	
        _DrvMpTestItoTestMsg21xxaDisableFilterNoiseDetect();
	
        _DrvMpTestItoOpenTestMsg21xxaSetV(1, 0);    
        nRegVal = RegGet16BitValue(0x110E); //bank:ana, addr:h0007   			
        RegSet16BitValue(0x110E, nRegVal | BIT11);				 		
	
        if (_gMsg21xxaLTP == 1)
        {
            _DrvMpTestItoOpenTestMsg21xxaSetC(32);
        }	    	
        else
        {	    	
	    	    _DrvMpTestItoOpenTestMsg21xxaSetC(0);
        }
        
        _DrvMpTestItoTestSelfICAnaSwReset();
		
        if (i == 0)	 
        {      
            nSensorNum1 = _DrvMpTestItoTestMsg21xxaGetDataOut(szTmpRawData);
            DBG(&g_I2cClient->dev, "nSensorNum1 = %d\n", nSensorNum1);
        }
        else	
        {      
            nSensorNum2 = _DrvMpTestItoTestMsg21xxaGetDataOut(&szTmpRawData[nSensorNum1]);
            DBG(&g_I2cClient->dev, "nSensorNum1 = %d, nSensorNum2 = %d\n", nSensorNum1, nSensorNum2);
        }
    }
    
    for (j = 0; j < nTotalSensor; j ++)
    {
        if (_gMsg21xxaLTP == 1)
        {
            pRawData[pMapping[j]] = szTmpRawData[j] + 4096;
            pDataFlag[pMapping[j]] = 1;
        }
        else
        {
            pRawData[pMapping[j]] = szTmpRawData[j];	
            pDataFlag[pMapping[j]] = 1;
        }
    }	
}

static void _DrvMpTestItoShortTestMsg21xxaChangeGPOSetting(u8 nItemId)
{
    u8 szDbBusTxData[3+MSG21XXA_GPO_SETTING_SIZE*2] = {0};
    u16 szGPOSetting[3] = {0};
    u32 i;
    
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);
    
    if (nItemId == 1) // 39-1
    {
        szGPOSetting[0] = _gMsg21xxaShort_1_GPO[0];		
        szGPOSetting[1] = _gMsg21xxaShort_1_GPO[1];		
        szGPOSetting[2] = _gMsg21xxaShort_1_GPO[2];		
        szGPOSetting[2] |= (1 << (int)(MSG21XXA_PIN_GUARD_RING % 16));
    }
    else if (nItemId == 2) // 39-2
    {
        szGPOSetting[0] = _gMsg21xxaShort_2_GPO[0];		
        szGPOSetting[1] = _gMsg21xxaShort_2_GPO[1];		
        szGPOSetting[2] = _gMsg21xxaShort_2_GPO[2];		
        szGPOSetting[2] |= (1 << (int)(MSG21XXA_PIN_GUARD_RING % 16));
    }
    else if (nItemId == 3) // 39-3
    {
        szGPOSetting[0] = _gMsg21xxaShort_3_GPO[0];		
        szGPOSetting[1] = _gMsg21xxaShort_3_GPO[1];		
        szGPOSetting[2] = _gMsg21xxaShort_3_GPO[2];		
        szGPOSetting[2] |= (1 << (int)(MSG21XXA_PIN_GUARD_RING % 16));
    }
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nItemId == 0) // 39-4 (2R)
    {
        szGPOSetting[0] = _gMsg21xxaShort_4_GPO[0];		
        szGPOSetting[1] = _gMsg21xxaShort_4_GPO[1];		
        szGPOSetting[2] = _gMsg21xxaShort_4_GPO[2];		
        szGPOSetting[2] |= (1 << (int)(MSG21XXA_PIN_GUARD_RING % 16));
    }
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else
    {
        DBG(&g_I2cClient->dev, "Invalid item id for changing GPIO setting of short test.\n");

        return;
    }

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x12;
    szDbBusTxData[2] = 0x48;

    for (i = 0; i < MSG21XXA_GPO_SETTING_SIZE; i ++)
    {
        szDbBusTxData[3+2*i] = szGPOSetting[i] & 0xFF;
        szDbBusTxData[4+2*i] = (szGPOSetting[i] >> 8) & 0xFF;    	
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+MSG21XXA_GPO_SETTING_SIZE*2);    
}

static void _DrvMpTestItoShortTestMsg21xxaChangeRmodeSetting(u8 nMode)
{
    u8 szDbBusTxData[6] = {0};

    DBG(&g_I2cClient->dev, "*** %s() nMode = %d ***\n", __func__, nMode);

    // AFE R-mode enable(Bit-12)
    RegSetLByteValue(0x1103, 0x10);

    // drv_mux_OV (Bit-8 1:enable)
    RegSetLByteValue(0x1107, 0x55);
    
    if (nMode == 1) // P_CODE: 0V
    {
        RegSet16BitValue(0x110E, 0x073A);
    }
    else if (nMode == 0) // N_CODE: 2.4V
    {
        RegSet16BitValue(0x110E, 0x073B);
    }

    // SW2 rising & SW3 rising return to 0
    RegSetLByteValue(0x1227, 0x01);
    // turn off the chopping
    RegSetLByteValue(0x1208, 0x0C);
    // idle driver ov
    RegSetLByteValue(0x1241, 0xC0);
	  
	  // AFE ov
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x12;
    szDbBusTxData[2] = 0x44;
    szDbBusTxData[3] = 0xFF;
    szDbBusTxData[4] = 0xFF;
    szDbBusTxData[5] = 0xFF;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 6);        
}	

static void _DrvMpTestItoShortTestMsg21xxaFirst(u8 nItemId, s16 *pRawData, s8 *pDataFlag)		
{
    u32 i;
    s16 szTmpRawData[SELF_IC_MAX_CHANNEL_NUM] = {0};
    s16 szTmpRawData2[SELF_IC_MAX_CHANNEL_NUM] = {0};
    u8  nSensorNum, nSensorNum2, nNumOfSensorMapping1, nNumOfSensorMapping2, nSensorCount = 0;
    u8 	*pMapping = NULL;
    
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

    // Stop cpu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
    // chip top op0
    RegSet16BitValue(0x1E24, 0x0500); //bank:chip, addr:h0012
    RegSet16BitValue(0x1E2A, 0x0000); //bank:chip, addr:h0015
    RegSet16BitValue(0x1EE6, 0x6E00); //bank:chip, addr:h0073
    RegSet16BitValue(0x1EE8, 0x0071); //bank:chip, addr:h0074
	    
    if ((_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) % 2 != 0)
    {
        nNumOfSensorMapping1 = (_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) / 2 + 1;
        nNumOfSensorMapping2 = nNumOfSensorMapping1;
    }
    else
    {
        nNumOfSensorMapping1 = (_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) / 2;
        nNumOfSensorMapping2 = nNumOfSensorMapping1;
        if (nNumOfSensorMapping2 % 2 != 0)
        {	
            nNumOfSensorMapping2 ++;
        }
    }        

    if (nItemId == 1) // 39-1    			    		
    {
        pMapping = &_gSelfIC_SHORT_MAP1[0];
        nSensorCount = nNumOfSensorMapping1; 
    }
    else if (nItemId == 2) // 39-2   		
    {
        pMapping = &_gSelfIC_SHORT_MAP2[0];      
        nSensorCount = nNumOfSensorMapping2; 
    }
    else if (nItemId == 3) // 39-3    		
    {
        pMapping = &_gSelfIC_SHORT_MAP3[0];      
        nSensorCount = _gSelfICItoTestTriangleNum; 
    }
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nItemId == 0) // 39-4 (2R)    			
    {
        pMapping = &_gSelfIC_SHORT_MAP4[0];
        nSensorCount = _gSelfICItoTestTriangleNum/2; 
    }
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

    DBG(&g_I2cClient->dev, "nSensorCount = %d\n", nSensorCount);
        	    
    _DrvMpTestItoTestMsg21xxaSendDataIn(nItemId);
    
    _DrvMpTestItoTestMsg21xxaDisableFilterNoiseDetect();

    _DrvMpTestItoShortTestMsg21xxaChangeRmodeSetting(1);
    _DrvMpTestItoShortTestMsg21xxaChangeGPOSetting(nItemId);
    _DrvMpTestItoTestSelfICAnaSwReset();
    nSensorNum = _DrvMpTestItoTestMsg21xxaGetDataOut(szTmpRawData);
    DBG(&g_I2cClient->dev, "nSensorNum = %d\n", nSensorNum);

    _DrvMpTestItoShortTestMsg21xxaChangeRmodeSetting(0);
    _DrvMpTestItoShortTestMsg21xxaChangeGPOSetting(nItemId);
    _DrvMpTestItoTestSelfICAnaSwReset();
    nSensorNum2 = _DrvMpTestItoTestMsg21xxaGetDataOut(szTmpRawData2);
    DBG(&g_I2cClient->dev, "nSensorNum2 = %d\n", nSensorNum2);
    
    for (i = 0; i < nSensorCount; i ++)
    {
        pRawData[pMapping[i]] = szTmpRawData[i] - szTmpRawData2[i];	
        pDataFlag[pMapping[i]] = 1;
    }	
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
static u8 _DrvMpTestSelfCheckValueDoubleRange(int nValue, int nMax, int nMin)
{
   	if (nValue <= nMax && nValue >= nMin)
   	{
   	   	return 1;
   	}
   	else
   	{
   	   	return 0;
   	}
}
//- bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
static u16 _DrvMpTestItoTestMsg22xxGetDataOut(s16 *pRawData, u16 nSubFrameNum)
{
    u32 i;
    u16 szRawData[SELF_IC_MAX_CHANNEL_NUM*2] = {0};
    u16 nRegInt = 0x0000;
    u16 nSize = nSubFrameNum * 4;  // 1SF 4AFE
    u8  szDbBusTxData[8] = {0};
    u8  szDbBusRxData[SELF_IC_MAX_CHANNEL_NUM*4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValueOff(0x120A, BIT1); //one-shot mode
    RegSet16BitValueOff(0x3D08, BIT8); //SELF_IC_FIQ_E_FRAME_READY_MASK
    //RegSet16BitValue(0x130C, BIT15); //MCU read done
    RegSet16BitValueOn(0x120A, BIT0); //trigger one-shot 

    DBG(&g_I2cClient->dev, "polling start\n");

    //Polling frame-ready interrupt status
    do 
    {
        nRegInt = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    } while((nRegInt & SELF_IC_FIQ_E_FRAME_READY_MASK) == 0x0000);

    DBG(&g_I2cClient->dev, "polling end\n");

    RegSet16BitValueOff(0x3D18, BIT8); //Clear frame-ready interrupt status

    //ReadRegBurst start   
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x15; //bank:fout, addr:h0000 
    szDbBusTxData[2] = 0x00;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    mdelay(20);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szDbBusRxData[0], (nSubFrameNum * 4 * 2));
    mdelay(100);

    for (i = 0; i < (nSubFrameNum * 4 * 2); i ++)
    {
        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = %d\n", i, szDbBusRxData[i]); // add for debug
    }
    //ReadRegBurst stop
 
    RegSet16BitValueOn(0x3D08, BIT8); //SELF_IC_FIQ_E_FRAME_READY_MASK

    for (i = 0; i < nSize; i ++)
    {
        szRawData[i] = (szDbBusRxData[2 * i + 1] << 8 ) | (szDbBusRxData[2 * i]);
        pRawData[i] = (s16)szRawData[i];
    }
    
    return nSize;
}

static void _DrvMpTestItoTestMsg22xxSendDataIn(u8 nStep, u16 nRiuWriteLength)
{
    u32	i;
    u32 *pType = NULL;

    DBG(&g_I2cClient->dev, "*** %s() nStep = %d, nRiuWriteLength = %d ***\n", __func__, nStep, nRiuWriteLength);

    if (nStep == 1) //39-1
    {
        pType = &_gMsg22xxShort_RIU1[0];      	
    }
    else if (nStep == 2) //39-2
    {
        pType = &_gMsg22xxShort_RIU2[0];      	
    }
/*
    else if (nStep == 3) //39-3
    {
        pType = &_gMsg22xxShort_RIU3[0];        
    }
*/
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nStep == 0) //39-4 (2R)
    {
        pType = &_gMsg22xxShort_RIU4[0];
    }
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nStep == 4)
    {
        pType = &_gMsg22xxOpenRIU1[0];        
    }
    else if (nStep == 5)
    {
        pType = &_gMsg22xxOpenRIU2[0];      	
    }
    else if (nStep == 6)
    {
        pType = &_gMsg22xxOpenRIU3[0];      	
    }

    RegSet16BitValueOn(0x1192, BIT4); // force on enable sensor mux and csub sel sram clock
    RegSet16BitValueOff(0x1192, BIT5); // mem clk sel 
    RegSet16BitValueOff(0x1100, BIT3); // tgen soft rst
    RegSet16BitValue(0x1180, MSG22XX_RIU_BASE_ADDR); // sensor mux sram read/write base address
    RegSet16BitValue(0x1182, nRiuWriteLength); // sensor mux sram write length
    RegSet16BitValueOn(0x1188, BIT0); // reg_mem0_w_start

    for (i = MSG22XX_RIU_BASE_ADDR; i < (MSG22XX_RIU_BASE_ADDR + nRiuWriteLength); i ++)
    {
        RegSet16BitValue(0x118A, (u16)(pType[i]));
        RegSet16BitValue(0x118C, (u16)(pType[i] >> 16));
    }
}

static void _DrvMpTestItoTestMsg22xxSetC(u8 nCSubStep)
{
    u32 i;
    u16 nRegVal;
    u32 nCSubNew; 
     
    DBG(&g_I2cClient->dev, "*** %s() nCSubStep = %d ***\n", __func__, nCSubStep);
    
    nCSubNew = (nCSubStep > MSG22XX_CSUB_REF_MAX) ? MSG22XX_CSUB_REF_MAX : nCSubStep; // 6 bits
    nCSubNew = (nCSubNew | (nCSubNew << 8) | (nCSubNew << 16) | (nCSubNew << 24));

    nRegVal = RegGet16BitValue(0x11C8); // csub sel overwrite enable, will referance value of 11C0

    if (nRegVal == 0x000F)
    {
        RegSet16BitValue(0x11C0, nCSubNew);         // prs 0
        RegSet16BitValue(0x11C2, (nCSubNew >> 16)); // prs 0
        RegSet16BitValue(0x11C4, nCSubNew);         // prs 1
        RegSet16BitValue(0x11C6, (nCSubNew >> 16)); // prs 1
    }
    else
    {
        RegSet16BitValueOn(0x1192, BIT4);   // force on enable sensor mux  and csub sel sram clock
        RegSet16BitValueOff(0x1192, BIT5);   // mem clk sel 
        RegSet16BitValueOff(0x1100, BIT3);   // tgen soft rst
        RegSet16BitValue(0x1184, 0);          // nAddr
        RegSet16BitValue(0x1186, SELF_IC_MAX_CHANNEL_NUM);         // nLen
        RegSet16BitValueOn(0x1188, BIT2);   // reg_mem0_w_start

        for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
        {
            RegSet16BitValue(0x118E, nCSubNew);
            RegSet16BitValue(0x1190, (nCSubNew >> 16));
        }
    }
}

static void _DrvMpTestItoTestMsg22xxRegisterReset(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValueOn(0x1102, (BIT3 | BIT4 | BIT5 | BIT6 | BIT7));
    RegSet16BitValueOff(0x1130, (BIT0 | BIT1 | BIT2 | BIT3 | BIT8));

    RegSet16BitValueOn(0x1312, BIT15);
    RegSet16BitValueOff(0x1312, BIT13);

    RegMask16BitValue(0x1250, 0x007F, ((5 << 0) & 0x007F), ADDRESS_MODE_8BIT); 

    RegMask16BitValue(0x1250, 0x7F00, ((1 << 8) & 0x7F00), ADDRESS_MODE_8BIT); 

    RegSet16BitValueOff(0x1250, 0x8080);

    RegSet16BitValueOff(0x1312, (BIT12 | BIT14));

    RegSet16BitValueOn(0x1300, BIT15);
    RegSet16BitValueOff(0x1300, (BIT10 | BIT11 | BIT12 | BIT13 | BIT14));

    RegSet16BitValueOn(0x1130, BIT9);

    RegSet16BitValueOn(0x1318, (BIT12 | BIT13));

    RegSet16BitValue(0x121A, ((8 - 1) & 0x01FF));       // sampling number group A
    RegSet16BitValue(0x121C, ((8 - 1) & 0x01FF));       // sampling number group B
    
    RegMask16BitValue(0x131A, 0x3FFF, 0x2000, ADDRESS_MODE_8BIT); 
    
    RegMask16BitValue(0x131C, (BIT8 | BIT9 | BIT10), (2 << 8), ADDRESS_MODE_8BIT); 

    RegSet16BitValueOff(0x1174, 0x0F00);

    RegSet16BitValueOff(0x1240, 0xFFFF);       // mutual cap mode selection for total 24 subframes, 0: normal sense, 1: mutual cap sense
    RegSet16BitValueOff(0x1242, 0x00FF);       // mutual cap mode selection for total 24 subframes, 0: normal sense, 1: mutual cap sense

    RegSet16BitValueOn(0x1212, 0xFFFF);       // timing group A/B selection for total 24 subframes, 0: group A, 1: group B
    RegSet16BitValueOn(0x1214, 0x00FF);       // timing group A/B selection for total 24 subframes, 0: group A, 1: group B

    RegSet16BitValueOn(0x121E, 0xFFFF);   //sample number group A/B selection for total 24 subframes, 0: group A, 1: group B       
    RegSet16BitValueOn(0x1220, 0x00FF);   //sample number group A/B selection for total 24 subframes, 0: group A, 1: group B    

    RegSet16BitValueOff(0x120E, 0xFFFF);  // noise sense mode selection for total 24 subframes
    RegSet16BitValueOff(0x1210, 0x00FF);  // noise sense mode selection for total 24 subframes

    RegSet16BitValue(0x128C, 0x0F);  // ADC afe gain correction bypass
    RegSet16BitValueOff(0x1104, BIT12); 
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
	RegMask16BitValue( 0x1104, 0x0FFF, 0x0770, ADDRESS_MODE_16BIT);
	RegMask16BitValue( 0x1105, 0x0FFF, 0x0770, ADDRESS_MODE_16BIT);

	RegSet16BitValue(0x115C, 0x0); //clear sensor ov enable
 	RegSet16BitValue(0x115D, 0x0); //clear sensor ov enable
	RegSet16BitValue( 0x115E, 0x0); //clear sensor and gr ov enable 
 //-bug 282400 ,zwk.wt,20170802,add new tp openshort test mode    
}

static void _DrvMpTestItoTestMsg22xxRegisterResetPatch(void)
{
    DBG(&g_I2cClient->dev, "*** %s() *** _gIsOldFirmwareVersion = %d\n", __func__, _gIsOldFirmwareVersion);
    
    if (_gIsOldFirmwareVersion == 0) // Only need to patch register reset for MSG22XX firmware older than V01.005.02;
    {
        return;		
    }

    RegMask16BitValue(0x1101, 0xFFFF, 0x501E, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1102, 0x7FFF, 0x06FF, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1104, 0x0FFF, 0x0772, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1105, 0x0FFF, 0x0770, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1106, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1107, 0x0003, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1108, 0x0073, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1109, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x110A, 0x7FFF, 0x1087, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x110E, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x110F, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1117, 0xFF0F, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1118, 0xFFFF, 0x0200, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1119, 0x00FF, 0x000E, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x111E, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x111F, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1133, 0x000F, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x113A, 0x0F37, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x113B, 0x0077, 0x0077, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x113C, 0xFF00, 0xA000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x113D, 0x0077, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x113E, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 

    RegMask16BitValue(0x1204, 0x0006, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1205, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1207, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1208, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1209, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x120A, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x120B, 0x003F, 0x002E, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x120D, 0x001F, 0x0005, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x120E, 0x001F, 0x0005, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x120F, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1210, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1211, 0x0FFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1212, 0x1F87, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1213, 0x0F7F, 0x0014, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1214, 0xFF9F, 0x090A, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1215, 0x0F7F, 0x0F0C, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1216, 0x0FFF, 0x0700, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1217, 0xFF1F, 0x5C0A, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1218, 0x1F7F, 0x0A14, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1219, 0xFFFF, 0x218C, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x121E, 0x1F1F, 0x0712, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x121F, 0x3F3F, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1220, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1221, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1223, 0x3F3F, 0x0002, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1224, 0x003F, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1228, 0xFFFF, 0x8183, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x122D, 0x0001, 0x0001, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1250, 0xFFFF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1251, 0x00FF, 0x0000, ADDRESS_MODE_16BIT); 
    RegMask16BitValue(0x1270, 0x0003, 0x0003, ADDRESS_MODE_16BIT); 

    RegSet16BitValueByAddressMode(0x115C, 0x0000, ADDRESS_MODE_16BIT); //sensor ov enable
    RegSet16BitValueByAddressMode(0x115D, 0x0000, ADDRESS_MODE_16BIT); //sensor ov enable
    RegSet16BitValueByAddressMode(0x115E, 0x0000, ADDRESS_MODE_16BIT); //sensor and gr ov enable

    RegSet16BitValueByAddressMode(0x124E, 0x000F, ADDRESS_MODE_16BIT); //bypass mode
}

static void _DrvMpTestItoTestMsg22xxGetChargeDumpTime(u16 nMode, u16 *pChargeTime, u16 *pDumpTime)
{
    u16 nChargeTime = 0, nDumpTime = 0;
    u16 nMinChargeTime = 0xFFFF, nMinDumpTime = 0xFFFF, nMaxChargeTime = 0x0000, nMaxDumpTime = 0x0000;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nChargeTime = RegGet16BitValue(0x1226);
    nDumpTime = RegGet16BitValue(0x122A);

    if (nMinChargeTime > nChargeTime)
    {
        nMinChargeTime = nChargeTime;
    }

    if (nMaxChargeTime < nChargeTime)
    {
        nMaxChargeTime = nChargeTime;
    }

    if (nMinDumpTime > nDumpTime)
    {
        nMinDumpTime = nDumpTime;
    }

    if (nMaxDumpTime < nDumpTime)
    {
        nMaxDumpTime = nDumpTime;
    }

    DBG(&g_I2cClient->dev, "nChargeTime = %d, nDumpTime = %d\n", nChargeTime, nDumpTime);
    
    if (nMode == 1)
    {
        *pChargeTime = nMaxChargeTime;
        *pDumpTime = nMaxDumpTime;
    }
    else
    {
        *pChargeTime = nMinChargeTime;
        *pDumpTime = nMinDumpTime;
    }
}

static void _DrvMpTestItoOpenTestMsg22xxFirst(u8 nItemId, s16 *pRawData, s8 *pDataFlag)		
{
    u32 i;
    s16 szTmpRawData[SELF_IC_MAX_CHANNEL_NUM*2] = {0};
    u16 nSubFrameNum = 0;
    u16 nChargeTime, nDumpTime;
    s16 	*pMapping = NULL;
    
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);
	
    if (nItemId == 40)
    {
        // Stop cpu
        RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

        _DrvMpTestItoTestMsg22xxRegisterResetPatch();
        _DrvMpTestItoTestMsg22xxRegisterReset();
    }

    switch (nItemId)
    {
        case 40:
            pMapping = _gSelfIC_MAP1;
            nSubFrameNum = _gMsg22xxOpenSubFrameNum1;
            break;
        case 41:
            pMapping = _gSelfIC_MAP2;
            nSubFrameNum = _gMsg22xxOpenSubFrameNum2;
            break;
        case 42:
            pMapping = _gSelfIC_MAP3;
            nSubFrameNum = _gMsg22xxOpenSubFrameNum3;
            break;
    }

    if (nSubFrameNum > 24) // SELF_IC_MAX_CHANNEL_NUM/2
    {
        nSubFrameNum = 24;
    }

    _DrvMpTestItoTestMsg22xxSendDataIn(nItemId - 36, nSubFrameNum * 6);
    RegSet16BitValue(0x1216, (nSubFrameNum - 1) << 1); // subframe numbers, 0:1subframe, 1:2subframe  

    if (nItemId == 40)
    {
        if (1)
        {
            RegSet16BitValue(0x1110, 0x0060); //2.4V -> 1.2V    
        }
        else
        {
            RegSet16BitValue(0x1110, 0x0020); //3.0V -> 0.6V
        }
    
        RegSet16BitValue(0x11C8, 0x000F); //csub sel overwrite enable, will referance value of 11C0
        RegSet16BitValue(0x1174, 0x0F06); // 1 : sel idel driver for sensor pad that connected to AFE
        RegSet16BitValue(0x1208, 0x0006); //PRS1
        RegSet16BitValue(0x1240, 0xFFFF); //mutual cap mode selection for total 24 subframes, 0: normal sense, 1: mutual cap sense
        RegSet16BitValue(0x1242, 0x00FF); //mutual cap mode selection for total 24 subframes, 0: normal sense, 1: mutual cap sense

        _DrvMpTestItoTestMsg22xxGetChargeDumpTime(1, &nChargeTime, &nDumpTime);

        RegSet16BitValue(0x1226, nChargeTime);
        RegSet16BitValue(0x122A, nDumpTime);

        _DrvMpTestItoTestMsg22xxSetC(MSG22XX_CSUB_REF);
        _DrvMpTestItoTestSelfICAnaSwReset();
    }

    _DrvMpTestItoTestMsg22xxGetDataOut(szTmpRawData, nSubFrameNum);

    for (i = 0; i < nSubFrameNum; i ++)
    {
//        DBG(&g_I2cClient->dev, "szTmpRawData[%d * 4] >> 3 = %d\n", i, szTmpRawData[i * 4] >> 3); // add for debug
//        DBG(&g_I2cClient->dev, "pMapping[%d] = %d\n", i, pMapping[i]); // add for debug

        pRawData[pMapping[i]] = (szTmpRawData[i * 4] >> 3);  // Filter to ADC 
        pDataFlag[pMapping[i]] = 1;
        
        DBG(&g_I2cClient->dev, "pRawData[%d] = %d\n", pMapping[i], pRawData[pMapping[i]]);
    }
}

static void _DrvMpTestItoShortTestMsg22xxFirst(u8 nItemId, s16 *pRawData, s8 *pDataFlag)		
{
    u32 i, j;
    s16 szIIR1[MSG22XX_MAX_SUBFRAME_NUM*MSG22XX_MAX_AFE_NUM] = {32767};
    s16 szIIR2[MSG22XX_MAX_SUBFRAME_NUM*MSG22XX_MAX_AFE_NUM] = {32767};
    s16 szIIRTmp[MSG22XX_MAX_SUBFRAME_NUM*MSG22XX_MAX_AFE_NUM] = {32767};
    u16 nSensorNum = 0, nSubFrameNum = 0;
    u8 	*pMapping = NULL;
    
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

    if ((nItemId == 0 && _gSelfICIsEnable2R == 1) || (nItemId == 1 && _gSelfICIsEnable2R == 0))
    {
        // Stop cpu
        RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
        
        _DrvMpTestItoTestMsg22xxRegisterResetPatch();
        _DrvMpTestItoTestMsg22xxRegisterReset();
    }

    switch(nItemId)
    {
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        case 0: // 39-4 (2R)
            pMapping = &_gSelfIC_SHORT_MAP4[0];
            nSensorNum = _gMsg22xxShortSubFrameNum4;
            break;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        case 1: // 39-1
            pMapping = &_gSelfIC_SHORT_MAP1[0];
            nSensorNum = _gMsg22xxShortSubFrameNum1;
            break;
        case 2: // 39-2
            pMapping = &_gSelfIC_SHORT_MAP2[0];
            nSensorNum = _gMsg22xxShortSubFrameNum2;
            break;
        case 3: // 39-3
            pMapping = &_gSelfIC_SHORT_MAP3[0];
            nSensorNum = _gMsg22xxShortSubFrameNum3;
            break;            
    }
    
    if (nSensorNum > 24) // SELF_IC_MAX_CHANNEL_NUM/2
    {
        nSubFrameNum = 24;
    }
    else
    {
        nSubFrameNum = nSensorNum;
    }

    _DrvMpTestItoTestMsg22xxSendDataIn(nItemId, nSubFrameNum * 6);
    RegSet16BitValue(0x1216, (nSubFrameNum - 1) << 1); // subframe numbers, 0:1subframe, 1:2subframe

    if ((nItemId == 0 && _gSelfICIsEnable2R == 1) || (nItemId == 1 && _gSelfICIsEnable2R == 0))
    {
        RegSet16BitValue(0x1110, 0x0030); // [6:4}  011 : 2.1V -> 1.5V
//        RegSet16BitValue(0x1110, 0x0060); // 2.4V -> 1.2V
        RegSet16BitValue(0x11C8, 0x000F); // csub sel overwrite enable, will referance value of 11C0
        RegSet16BitValue(0x1174, 0x0000); // [11:8] 000 : sel active driver for sensor pad that connected to AFE
        RegSet16BitValue(0x1208, 0x0006); // PRS 1
        RegSet16BitValueOn(0x1104, BIT14); // R mode 
        RegSet16BitValue(0x1176, 0x0000); // CFB 10p
        
        _DrvMpTestItoTestMsg22xxSetC(MSG22XX_CSUB_REF);

        RegMask16BitValue(0x1213, 0x007F, 0x003B, ADDRESS_MODE_16BIT); // Charge 20ns (group A)
        RegMask16BitValue(0x1218, 0x007F, 0x003B, ADDRESS_MODE_16BIT); // Charge 20ns (group B)
    }    

    RegMask16BitValue(0x1215, 0x007F, 0x000B, ADDRESS_MODE_16BIT); // Dump 4ns (group A)
    RegMask16BitValue(0x1219, 0x007F, 0x000B, ADDRESS_MODE_16BIT); // Dump 4ns (group B)

    _DrvMpTestItoTestSelfICAnaSwReset();
    _DrvMpTestItoTestMsg22xxGetDataOut(szIIRTmp, nSubFrameNum);
    
    if (nSensorNum > MSG22XX_MAX_SUBFRAME_NUM)
    {
        j = 0;

        for (i = 0; i < MSG22XX_MAX_SUBFRAME_NUM; i ++)
        {
            szIIR1[j] = (szIIRTmp[i * 4] >> 3);
            j ++;
            
            if ((nSensorNum - MSG22XX_MAX_SUBFRAME_NUM) > i)
            {
                szIIR1[j] = (szIIRTmp[i * 4 + 1] >> 3);
                j ++;
            }    
        }
    }
    else
    {
        for (i = 0; i < nSensorNum; i ++)
        {
            szIIR1[i] = (szIIRTmp[i * 4 + 1] >> 3);
        }
    }

    RegMask16BitValue(0x1215, 0x007F, 0x0017, ADDRESS_MODE_16BIT); // Dump 8ns (group A)
    RegMask16BitValue(0x1219, 0x007F, 0x0017, ADDRESS_MODE_16BIT); // Dump 8ns (group B)

    _DrvMpTestItoTestSelfICAnaSwReset();
    _DrvMpTestItoTestMsg22xxGetDataOut(szIIRTmp, nSubFrameNum);

    if (nSensorNum > MSG22XX_MAX_SUBFRAME_NUM)
    {
        j = 0;

        for (i = 0; i < MSG22XX_MAX_SUBFRAME_NUM; i ++)
        {
            szIIR2[j] = (szIIRTmp[i * 4] >> 3);
            j ++;
            
            if ((nSensorNum - MSG22XX_MAX_SUBFRAME_NUM) > i)
            {
                szIIR2[j] = (szIIRTmp[i * 4 + 1] >> 3);
                j ++;
            }    
        }
    }
    else
    {
        for (i = 0; i < nSensorNum; i ++)
        {
            szIIR2[i] = (szIIRTmp[i * 4 + 1] >> 3);
        }
    }

    for (i = 0; i < nSensorNum; i ++)
    {
        if ((abs(szIIR1[i]) > 4000) || (abs(szIIR2[i]) > 4000))
        {
            pRawData[pMapping[i]] = 8192;
        }
        else
        {
            pRawData[pMapping[i]] = ((szIIR2[i] - szIIR1[i]) * 4);
        }
        pDataFlag[pMapping[i]] = 1;

//        DBG(&g_I2cClient->dev, "szIIR1[%d] = %d, szIIR2[%d] = %d\n", i, szIIR1[i], i, szIIR2[i]); // add for debug
//        DBG(&g_I2cClient->dev, "pRawData[%d] = %d\n", pMapping[i], pRawData[pMapping[i]]); // add for debug
//        DBG(&g_I2cClient->dev, "pMapping[%d] = %d\n", i, pMapping[i]); // add for debug
    }
}

static u8 _DrvMpTestMsg22xxCheckFirmwareVersion(void) // Only MSG22XX support platform firmware version
{
    u32 i;
    s32 nDiff;
    u16 nRegData1, nRegData2;
    u8 szDbBusRxData[12] = {0};
    u8 szCompareFwVersion[10] = {0x56, 0x30, 0x31, 0x2E, 0x30, 0x30, 0x35, 0x2E, 0x30, 0x32}; //{'V', '0', '1', '.', '0', '0', '5', '.', '0', '2'}
    u8 nIsOldFirmwareVersion = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    mutex_lock(&g_Mutex);

    DrvPlatformLyrTouchDeviceResetHw();
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); 

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
        
        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+0, szDbBusRxData[i*4+0], szDbBusRxData[i*4+0]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+1, szDbBusRxData[i*4+1], szDbBusRxData[i*4+1]); // add for debug
        
        szDbBusRxData[i*4+2] = (nRegData2 & 0xFF);
        szDbBusRxData[i*4+3] = ((nRegData2 >> 8 ) & 0xFF);

        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+2, szDbBusRxData[i*4+2], szDbBusRxData[i*4+2]); // add for debug
        DBG(&g_I2cClient->dev, "szDbBusRxData[%d] = 0x%x , %c \n", i*4+3, szDbBusRxData[i*4+3], szDbBusRxData[i*4+3]); // add for debug
    }

    // Clear burst mode
    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 
    
    for (i = 0; i < 10; i ++)
    {
        nDiff = szDbBusRxData[2+i] - szCompareFwVersion[i];
        
        DBG(&g_I2cClient->dev, "i = %d, nDiff = %d\n", i, nDiff);
        
        if (nDiff < 0)
        {
            nIsOldFirmwareVersion = 1;
            break;
        }
        else if (nDiff > 0)
        {
            nIsOldFirmwareVersion = 0;
            break;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();

    mutex_unlock(&g_Mutex);

    return nIsOldFirmwareVersion;
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
static u16 _DrvMpTestItoTestSelfICGetTpType(void)
{
    u16 nMajor = 0, nMinor = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
        
    if (g_ChipType == CHIP_TYPE_MSG21XXA)
    {
        u8 szDbBusTxData[3] = {0};
        u8 szDbBusRxData[4] = {0};

        szDbBusTxData[0] = 0x53;
        szDbBusTxData[1] = 0x00;
        szDbBusTxData[2] = 0x2A;

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

        nMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
        nMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

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

        nMajor = (((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF);
        nMinor = (((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF);

        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);
    }

    DBG(&g_I2cClient->dev, "*** major = %d ***\n", nMajor);
    DBG(&g_I2cClient->dev, "*** minor = %d ***\n", nMinor);
    
    return nMajor;
}

static u16 _DrvMpTestItoTestSelfICChooseTpType(void)
{
    u16 nTpType = 0;
    u32 i = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    if (g_ChipType == CHIP_TYPE_MSG21XXA)   
    {
        // _gMsg21xxaOpen1~_gMsg21xxaOpen3 are for MSG21XXA
        _gMsg21xxaOpen1 = NULL;
        _gMsg21xxaOpen1B = NULL;
        _gMsg21xxaOpen2 = NULL;
        _gMsg21xxaOpen2B = NULL;
        _gMsg21xxaOpen3 = NULL;

        // _gMsg21xxaShort_1~_gMsg21xxaShort_4 are for MSG21XXA
        _gMsg21xxaShort_1 = NULL;
        _gMsg21xxaShort_2 = NULL;
        _gMsg21xxaShort_3 = NULL;

        _gMsg21xxaShort_1_GPO = NULL;
        _gMsg21xxaShort_2_GPO = NULL;
        _gMsg21xxaShort_3_GPO = NULL;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        _gMsg21xxaShort_4 = NULL;
        _gMsg21xxaShort_4_GPO = NULL;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    if (g_ChipType == CHIP_TYPE_MSG22XX)   
    {
        // _gMsg22xxOpenRIU1~_gMsg22xxOpenRIU3 are for MSG22XX
        _gMsg22xxOpenRIU1 = NULL;
        _gMsg22xxOpenRIU2 = NULL;
        _gMsg22xxOpenRIU3 = NULL;

        // _gMsg22xxShort_RIU1~_gMsg22xxShort_RIU4 are for MSG22XX
        _gMsg22xxShort_RIU1 = NULL;
        _gMsg22xxShort_RIU2 = NULL;
        _gMsg22xxShort_RIU3 = NULL;

        _gMsg22xxOpenSubFrameNum1 = 0;
        _gMsg22xxOpenSubFrameNum2 = 0;
        _gMsg22xxOpenSubFrameNum3 = 0;
        _gMsg22xxShortSubFrameNum1 = 0;
        _gMsg22xxShortSubFrameNum2 = 0;
        _gMsg22xxShortSubFrameNum3 = 0;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        _gMsg22xxShort_RIU4 = NULL;
        _gMsg22xxShortSubFrameNum4 = 0;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

    _gSelfIC_MAP1 = NULL;
    _gSelfIC_MAP2 = NULL;
    _gSelfIC_MAP3 = NULL;
    _gSelfIC_MAP40_1 = NULL;
    _gSelfIC_MAP40_2 = NULL;
    _gSelfIC_MAP41_1 = NULL;
    _gSelfIC_MAP41_2 = NULL;
    
    _gSelfIC_SHORT_MAP1 = NULL;
    _gSelfIC_SHORT_MAP2 = NULL;
    _gSelfIC_SHORT_MAP3 = NULL;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    _gSelfIC_MAP40_3 = NULL;
    _gSelfIC_MAP40_4 = NULL;
    _gSelfIC_MAP41_3 = NULL;
    _gSelfIC_MAP41_4 = NULL;

    _gSelfIC_SHORT_MAP4 = NULL;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    
    _gSelfICItoTestKeyNum = 0;
    _gSelfICItoTestDummyNum = 0;
    _gSelfICItoTestTriangleNum = 0;
    _gSelfICIsEnable2R = 0;

    for (i = 0; i < 10; i ++)
    {
        nTpType = _DrvMpTestItoTestSelfICGetTpType();
        DBG(&g_I2cClient->dev, "nTpType = %d, i = %d\n", nTpType, i);

        if (TP_TYPE_HLT== nTpType )//|| TP_TYPE_Y == nTpType) // Modify.
        {
            break;
        }
        else if (i < 5)
        {
            mdelay(100);  
        }
        else
        {
            DrvPlatformLyrTouchDeviceResetHw();
        }
    }
    
    if (TP_TYPE_HLT == nTpType) // Modify. 
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)   
        {
            _gMsg21xxaOpen1 = MSG21XXA_open_1_X;
            _gMsg21xxaOpen1B = MSG21XXA_open_1B_X;
            _gMsg21xxaOpen2 = MSG21XXA_open_2_X;
            _gMsg21xxaOpen2B = MSG21XXA_open_2B_X;
            _gMsg21xxaOpen3 = MSG21XXA_open_3_X;

            _gMsg21xxaShort_1 = MSG21XXA_short_1_X;
            _gMsg21xxaShort_2 = MSG21XXA_short_2_X;
            _gMsg21xxaShort_3 = MSG21XXA_short_3_X;

            _gMsg21xxaShort_1_GPO = MSG21XXA_short_1_X_GPO;
            _gMsg21xxaShort_2_GPO = MSG21XXA_short_2_X_GPO;
            _gMsg21xxaShort_3_GPO = MSG21XXA_short_3_X_GPO;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
            _gMsg21xxaShort_4 = MSG21XXA_short_4_X;
            _gMsg21xxaShort_4_GPO = MSG21XXA_short_4_X_GPO;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

            _gSelfIC_MAP1 = MSG21XXA_MAP1_X;
            _gSelfIC_MAP2 = MSG21XXA_MAP2_X;
            _gSelfIC_MAP3 = MSG21XXA_MAP3_X;
            _gSelfIC_MAP40_1 = MSG21XXA_MAP40_1_X;
            _gSelfIC_MAP40_2 = MSG21XXA_MAP40_2_X;
            _gSelfIC_MAP41_1 = MSG21XXA_MAP41_1_X;
            _gSelfIC_MAP41_2 = MSG21XXA_MAP41_2_X;

            _gSelfIC_SHORT_MAP1 = MSG21XXA_SHORT_MAP1_X;
            _gSelfIC_SHORT_MAP2 = MSG21XXA_SHORT_MAP2_X;
            _gSelfIC_SHORT_MAP3 = MSG21XXA_SHORT_MAP3_X;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
            _gSelfIC_MAP40_3 = MSG21XXA_MAP40_3_X;
            _gSelfIC_MAP40_4 = MSG21XXA_MAP40_4_X;
            _gSelfIC_MAP41_3 = MSG21XXA_MAP41_3_X;
            _gSelfIC_MAP41_4 = MSG21XXA_MAP41_4_X;

            _gSelfIC_SHORT_MAP4 = MSG21XXA_SHORT_MAP4_X;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
 
            _gSelfICItoTestKeyNum = MSG21XXA_NUM_KEY_X;
            _gSelfICItoTestDummyNum = MSG21XXA_NUM_DUMMY_X;
            _gSelfICItoTestTriangleNum = MSG21XXA_NUM_SENSOR_X;
            _gSelfICIsEnable2R = MSG21XXA_ENABLE_2R_X;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)   
        {
            _gMsg22xxOpenRIU1 = open_1_HLT;
            _gMsg22xxOpenRIU2 = open_2_HLT;
            _gMsg22xxOpenRIU3 = open_3_HLT;

            _gMsg22xxShort_RIU1 = short_1_HLT;
            _gMsg22xxShort_RIU2 = short_2_HLT;
           // _gMsg22xxShort_RIU3 = short_3_HLT;

            _gMsg22xxOpenSubFrameNum1 = NUM_OPEN_1_SENSOR_HLT;
            _gMsg22xxOpenSubFrameNum2 = NUM_OPEN_2_SENSOR_HLT;
            _gMsg22xxOpenSubFrameNum3 = NUM_OPEN_3_SENSOR_HLT;
            _gMsg22xxShortSubFrameNum1 = NUM_SHORT_1_SENSOR_HLT;
            _gMsg22xxShortSubFrameNum2 = NUM_SHORT_2_SENSOR_HLT;
           // _gMsg22xxShortSubFrameNum3 = NUM_SHORT_3_SENSOR_HLT;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
            _gMsg22xxShort_RIU4 = short_4_HLT;
            _gMsg22xxShortSubFrameNum4 = NUM_SHORT_4_SENSOR_HLT;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

            _gSelfIC_MAP1 = MAP1_HLT;
            _gSelfIC_MAP2 = MAP2_HLT;
            _gSelfIC_MAP3 = MAP3_HLT;
            _gSelfIC_MAP40_1 = MAP40_1_HLT;
            _gSelfIC_MAP40_2 = MAP40_2_HLT;
            _gSelfIC_MAP41_1 = MAP41_1_HLT;
            _gSelfIC_MAP41_2 = MAP41_2_HLT;

            _gSelfIC_SHORT_MAP1 = SHORT_MAP1_HLT;
            _gSelfIC_SHORT_MAP2 = SHORT_MAP2_HLT;
            //_gSelfIC_SHORT_MAP3 = SHORT_MAP3_HLT;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
            _gSelfIC_MAP40_3 = MAP40_3_HLT;
            _gSelfIC_MAP40_4 = MAP40_4_HLT;
            _gSelfIC_MAP41_3 = MAP41_3_HLT;
            _gSelfIC_MAP41_4 = MAP41_4_HLT;

            _gSelfIC_SHORT_MAP4 =SHORT_MAP4_HLT;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
 
            _gSelfICItoTestKeyNum = NUM_KEY_HLT;
            _gSelfICItoTestDummyNum = NUM_DUMMY_HLT;
            _gSelfICItoTestTriangleNum = NUM_SENSOR_HLT;
            _gSelfICIsEnable2R = ENABLE_2R_HLT;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    }
#if 0
    else if (TP_TYPE_Y == nTpType) // Modify. 
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)   
        {
            _gMsg21xxaOpen1 = MSG21XXA_open_1_Y;
            _gMsg21xxaOpen1B = MSG21XXA_open_1B_Y;
            _gMsg21xxaOpen2 = MSG21XXA_open_2_Y;
            _gMsg21xxaOpen2B = MSG21XXA_open_2B_Y;
            _gMsg21xxaOpen3 = MSG21XXA_open_3_Y;

            _gMsg21xxaShort_1 = MSG21XXA_short_1_Y;
            _gMsg21xxaShort_2 = MSG21XXA_short_2_Y;
            _gMsg21xxaShort_3 = MSG21XXA_short_3_Y;

            _gMsg21xxaShort_1_GPO = MSG21XXA_short_1_Y_GPO;
            _gMsg21xxaShort_2_GPO = MSG21XXA_short_2_Y_GPO;
            _gMsg21xxaShort_3_GPO = MSG21XXA_short_3_Y_GPO;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
            _gMsg21xxaShort_4 = MSG21XXA_short_4_Y;
            _gMsg21xxaShort_4_GPO = MSG21XXA_short_4_Y_GPO;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

             _gSelfIC_MAP1 = MSG21XXA_MAP1_Y;
             _gSelfIC_MAP2 = MSG21XXA_MAP2_Y;
             _gSelfIC_MAP3 = MSG21XXA_MAP3_Y;
             _gSelfIC_MAP40_1 = MSG21XXA_MAP40_1_Y;
             _gSelfIC_MAP40_2 = MSG21XXA_MAP40_2_Y;
             _gSelfIC_MAP41_1 = MSG21XXA_MAP41_1_Y;
             _gSelfIC_MAP41_2 = MSG21XXA_MAP41_2_Y;

             _gSelfIC_SHORT_MAP1 = MSG21XXA_SHORT_MAP1_Y;
             _gSelfIC_SHORT_MAP2 = MSG21XXA_SHORT_MAP2_Y;
             _gSelfIC_SHORT_MAP3 = MSG21XXA_SHORT_MAP3_Y;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
             _gSelfIC_MAP40_3 = MSG21XXA_MAP40_3_Y;
             _gSelfIC_MAP40_4 = MSG21XXA_MAP40_4_Y;
             _gSelfIC_MAP41_3 = MSG21XXA_MAP41_3_Y;
             _gSelfIC_MAP41_4 = MSG21XXA_MAP41_4_Y;
        
             _gSelfIC_SHORT_MAP4 = MSG21XXA_SHORT_MAP4_Y;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

             _gSelfICItoTestKeyNum = MSG21XXA_NUM_KEY_Y;
             _gSelfICItoTestDummyNum = MSG21XXA_NUM_DUMMY_Y;
             _gSelfICItoTestTriangleNum = MSG21XXA_NUM_SENSOR_Y;
             _gSelfICIsEnable2R = MSG21XXA_ENABLE_2R_Y;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)   
        {
             _gMsg22xxOpenRIU1 = MSG22XX_open_1_Y;
             _gMsg22xxOpenRIU2 = MSG22XX_open_2_Y;
             _gMsg22xxOpenRIU3 = MSG22XX_open_3_Y;

             _gMsg22xxShort_RIU1 = MSG22XX_short_1_Y;
             _gMsg22xxShort_RIU2 = MSG22XX_short_2_Y;
             _gMsg22xxShort_RIU3 = MSG22XX_short_3_Y;

             _gMsg22xxOpenSubFrameNum1 = MSG22XX_NUM_OPEN_1_SENSOR_Y;
             _gMsg22xxOpenSubFrameNum2 = MSG22XX_NUM_OPEN_2_SENSOR_Y;
             _gMsg22xxOpenSubFrameNum3 = MSG22XX_NUM_OPEN_3_SENSOR_Y;
             _gMsg22xxShortSubFrameNum1 = MSG22XX_NUM_SHORT_1_SENSOR_Y;
             _gMsg22xxShortSubFrameNum2 = MSG22XX_NUM_SHORT_2_SENSOR_Y;
             _gMsg22xxShortSubFrameNum3 = MSG22XX_NUM_SHORT_3_SENSOR_Y;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
             _gMsg22xxShort_RIU4 = MSG22XX_short_4_Y;
             _gMsg22xxShortSubFrameNum4 = MSG22XX_NUM_SHORT_4_SENSOR_Y;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

             _gSelfIC_MAP1 = MSG22XX_MAP1_Y;
             _gSelfIC_MAP2 = MSG22XX_MAP2_Y;
             _gSelfIC_MAP3 = MSG22XX_MAP3_Y;
             _gSelfIC_MAP40_1 = MSG22XX_MAP40_1_Y;
             _gSelfIC_MAP40_2 = MSG22XX_MAP40_2_Y;
             _gSelfIC_MAP41_1 = MSG22XX_MAP41_1_Y;
             _gSelfIC_MAP41_2 = MSG22XX_MAP41_2_Y;

             _gSelfIC_SHORT_MAP1 = MSG22XX_SHORT_MAP1_Y;
             _gSelfIC_SHORT_MAP2 = MSG22XX_SHORT_MAP2_Y;
             _gSelfIC_SHORT_MAP3 = MSG22XX_SHORT_MAP3_Y;

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE       
             _gSelfIC_MAP40_3 = MSG22XX_MAP40_3_Y;
             _gSelfIC_MAP40_4 = MSG22XX_MAP40_4_Y;
             _gSelfIC_MAP41_3 = MSG22XX_MAP41_3_Y;
             _gSelfIC_MAP41_4 = MSG22XX_MAP41_4_Y;
        
             _gSelfIC_SHORT_MAP4 = MSG22XX_SHORT_MAP4_Y;
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

             _gSelfICItoTestKeyNum = MSG22XX_NUM_KEY_Y;
             _gSelfICItoTestDummyNum = MSG22XX_NUM_DUMMY_Y;
             _gSelfICItoTestTriangleNum = MSG22XX_NUM_SENSOR_Y;
             _gSelfICIsEnable2R = MSG22XX_ENABLE_2R_Y;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    }
#endif
    else
    {
        nTpType = 0;
    }
    
    return nTpType;
}

static void _DrvMpTestItoTestSelfICAnaSwReset(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x1100, 0xFFFF); // reset ANA - analog

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        RegSet16BitValueOn(0x130C, BIT1); // reset Filter - digital 
    }
    
    RegSet16BitValue(0x1100, 0x0000);
    mdelay(15);
}

static ItoTestResult_e _DrvMpTestItoOpenTestSelfICSecond(u8 nItemId)
{
    ItoTestResult_e nRetVal = ITO_TEST_OK;
    u32 i;
    s32 nTmpRawDataJg1 = 0;
    s32 nTmpRawDataJg2 = 0;
    s32 nTmpJgAvgThMax1 = 0;
    s32 nTmpJgAvgThMin1 = 0;
    s32 nTmpJgAvgThMax2 = 0;
    s32 nTmpJgAvgThMin2 = 0;

    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

    if (nItemId == 40)    			
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/2)-2; i ++)
        {
            nTmpRawDataJg1 += _gSelfICRawData1[*(_gSelfIC_MAP40_1+i)];
        }

        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg2 += _gSelfICRawData1[*(_gSelfIC_MAP40_2+i)];
        }
    }
    else if (nItemId == 41)    		
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/2)-2; i ++)
        {
            nTmpRawDataJg1 += _gSelfICRawData2[*(_gSelfIC_MAP41_1+i)];
        }
		
        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg2 += _gSelfICRawData2[*(_gSelfIC_MAP41_2+i)];
        }
    }

    nTmpJgAvgThMax1 = (nTmpRawDataJg1 / ((_gSelfICItoTestTriangleNum/2)-2)) * ( 100 + SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin1 = (nTmpRawDataJg1 / ((_gSelfICItoTestTriangleNum/2)-2)) * ( 100 - SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMax2 = (nTmpRawDataJg2 / 2) * ( 100 + SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin2 = (nTmpRawDataJg2 / 2 ) * ( 100 - SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
	
    DBG(&g_I2cClient->dev, "nItemId = %d, nTmpRawDataJg1 = %d, nTmpJgAvgThMax1 = %d, nTmpJgAvgThMin1 = %d, nTmpRawDataJg2 = %d, nTmpJgAvgThMax2 = %d, nTmpJgAvgThMin2 = %d\n", nItemId, nTmpRawDataJg1, nTmpJgAvgThMax1, nTmpJgAvgThMin1, nTmpRawDataJg2, nTmpJgAvgThMax2, nTmpJgAvgThMin2);

    if (nItemId == 40) 
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/2)-2; i ++)
        {
            if (_gSelfICRawData1[*(_gSelfIC_MAP40_1+i)] > nTmpJgAvgThMax1 || _gSelfICRawData1[*(_gSelfIC_MAP40_1+i)] < nTmpJgAvgThMin1)
            { 
                _gSelfICTestFailChannel[_gTestFailChannelCount] = *(_gSelfIC_MAP40_1+i);
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData1[*(_gSelfIC_MAP40_2+i)] > nTmpJgAvgThMax2 || _gSelfICRawData1[*(_gSelfIC_MAP40_2+i)] < nTmpJgAvgThMin2)
            { 
                _gSelfICTestFailChannel[_gTestFailChannelCount] = *(_gSelfIC_MAP40_2+i);
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        } 
    }
    else if (nItemId == 41) 
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/2)-2; i ++)
        {
            if (_gSelfICRawData2[*(_gSelfIC_MAP41_1+i)] > nTmpJgAvgThMax1 || _gSelfICRawData2[*(_gSelfIC_MAP41_1+i)] < nTmpJgAvgThMin1) 
            { 
                _gSelfICTestFailChannel[_gTestFailChannelCount] = *(_gSelfIC_MAP41_1+i);
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
        
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData2[*(_gSelfIC_MAP41_2+i)] > nTmpJgAvgThMax2 || _gSelfICRawData2[*(_gSelfIC_MAP41_2+i)] < nTmpJgAvgThMin2) 
            { 
                _gSelfICTestFailChannel[_gTestFailChannelCount] = *(_gSelfIC_MAP41_2+i);
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
    }

    return nRetVal;
}

#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
static ItoTestResult_e _DrvMpTestItoOpenTestSelfICSecond2r(u8 nItemId)
{
    ItoTestResult_e nRetVal = ITO_TEST_OK;
    u32 i;
    s32 nTmpRawDataJg1 = 0;
    s32 nTmpRawDataJg2 = 0;
    s32 nTmpRawDataJg3 = 0;
    s32 nTmpRawDataJg4 = 0;
    s32 nTmpJgAvgThMax1 = 0;
    s32 nTmpJgAvgThMin1 = 0;
    s32 nTmpJgAvgThMax2 = 0;
    s32 nTmpJgAvgThMin2 = 0;
    s32 nTmpJgAvgThMax3 = 0;
    s32 nTmpJgAvgThMin3 = 0;
    s32 nTmpJgAvgThMax4 = 0;
    s32 nTmpJgAvgThMin4 = 0;

    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

    if (nItemId == 40)    			
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            nTmpRawDataJg1 += _gSelfICRawData1[_gSelfIC_MAP40_1[i]];  //first region: non-border 
        }
		
        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg2 += _gSelfICRawData1[_gSelfIC_MAP40_2[i]];  //first region: border
        }

        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            nTmpRawDataJg3 += _gSelfICRawData1[_gSelfIC_MAP40_3[i]];  //second region: non-border
        }
		
        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg4 += _gSelfICRawData1[_gSelfIC_MAP40_4[i]];  //second region: border
        }
    }
    else if (nItemId == 41)    		
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            nTmpRawDataJg1 += _gSelfICRawData2[_gSelfIC_MAP41_1[i]];  //first region: non-border
        }
		
        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg2 += _gSelfICRawData2[_gSelfIC_MAP41_2[i]];  //first region: border
        }
		
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            nTmpRawDataJg3 += _gSelfICRawData2[_gSelfIC_MAP41_3[i]];  //second region: non-border
        }
		
        for (i = 0; i < 2; i ++)
        {
            nTmpRawDataJg4 += _gSelfICRawData2[_gSelfIC_MAP41_4[i]];  //second region: border
        }
    }

    nTmpJgAvgThMax1 = (nTmpRawDataJg1 / ((_gSelfICItoTestTriangleNum/4)-2)) * ( 100 + SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin1 = (nTmpRawDataJg1 / ((_gSelfICItoTestTriangleNum/4)-2)) * ( 100 - SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMax2 = (nTmpRawDataJg2 / 2) * ( 100 + SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin2 = (nTmpRawDataJg2 / 2) * ( 100 - SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMax3 = (nTmpRawDataJg3 / ((_gSelfICItoTestTriangleNum/4)-2)) * ( 100 + SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin3 = (nTmpRawDataJg3 / ((_gSelfICItoTestTriangleNum/4)-2)) * ( 100 - SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMax4 = (nTmpRawDataJg4 / 2) * ( 100 + SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
    nTmpJgAvgThMin4 = (nTmpRawDataJg4 / 2) * ( 100 - SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD) / 100;
		
    DBG(&g_I2cClient->dev, "nItemId = %d, nTmpRawDataJg1 = %d, nTmpJgAvgThMax1 = %d, nTmpJgAvgThMin1 = %d, nTmpRawDataJg2 = %d, nTmpJgAvgThMax2 = %d, nTmpJgAvgThMin2 = %d\n", nItemId, nTmpRawDataJg1, nTmpJgAvgThMax1, nTmpJgAvgThMin1, nTmpRawDataJg2, nTmpJgAvgThMax2, nTmpJgAvgThMin2);
    DBG(&g_I2cClient->dev, "nTmpRawDataJg3 = %d, nTmpJgAvgThMax3 = %d, nTmpJgAvgThMin3 = %d, nTmpRawDataJg4 = %d, nTmpJgAvgThMax4 = %d, nTmpJgAvgThMin4 = %d\n", nTmpRawDataJg3, nTmpJgAvgThMax3, nTmpJgAvgThMin3, nTmpRawDataJg4, nTmpJgAvgThMax4, nTmpJgAvgThMin4);

    if (nItemId == 40) 
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            if (_gSelfICRawData1[_gSelfIC_MAP40_1[i]] > nTmpJgAvgThMax1 || _gSelfICRawData1[_gSelfIC_MAP40_1[i]] < nTmpJgAvgThMin1) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP40_1[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData1[_gSelfIC_MAP40_2[i]] > nTmpJgAvgThMax2 || _gSelfICRawData1[_gSelfIC_MAP40_2[i]] < nTmpJgAvgThMin2) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP40_2[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        } 
		
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            if (_gSelfICRawData1[_gSelfIC_MAP40_3[i]] > nTmpJgAvgThMax3 || _gSelfICRawData1[_gSelfIC_MAP40_3[i]] < nTmpJgAvgThMin3) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP40_3[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData1[_gSelfIC_MAP40_4[i]] > nTmpJgAvgThMax4 || _gSelfICRawData1[_gSelfIC_MAP40_4[i]] < nTmpJgAvgThMin4) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP40_4[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        } 
    }
    else if (nItemId == 41) 
    {
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            if (_gSelfICRawData2[_gSelfIC_MAP41_1[i]] > nTmpJgAvgThMax1 || _gSelfICRawData2[_gSelfIC_MAP41_1[i]] < nTmpJgAvgThMin1) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP41_1[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData2[_gSelfIC_MAP41_2[i]] > nTmpJgAvgThMax2 || _gSelfICRawData2[_gSelfIC_MAP41_2[i]] < nTmpJgAvgThMin2) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP41_2[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < (_gSelfICItoTestTriangleNum/4)-2; i ++)
        {
            if (_gSelfICRawData2[_gSelfIC_MAP41_3[i]] > nTmpJgAvgThMax3 || _gSelfICRawData2[_gSelfIC_MAP41_3[i]] < nTmpJgAvgThMin3) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP41_3[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
		
        for (i = 0; i < 2; i ++)
        {
            if (_gSelfICRawData2[_gSelfIC_MAP41_4[i]] > nTmpJgAvgThMax4 || _gSelfICRawData2[_gSelfIC_MAP41_4[i]] < nTmpJgAvgThMin4) 
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_MAP41_4[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        } 
    }

    return nRetVal;
}
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
 void sort_mp(s16 *b ,  int num)
{

	 int i;
	 int n=num;
	 s16 temp,*p;
   	 for(i=0;i<n-1;i++)
        for(p=b;p<b+n-i-1;p++)
            if(*p>*(p+1))
            {
                temp=*p;
                *p=*(p+1);
                *(p+1)=temp;
            }
}

static ItoTestResult_e _DrvMpTestItoOpenTestSelfICSecond2r_Golden(u8 nItemId)
{
    ItoTestResult_e nRetVal = ITO_TEST_OK;     

       s16 temp_DeltaC[SELF_IC_MAX_CHANNEL_NUM] = {0};
	s16 Golden_DeltaC[SELF_IC_MAX_CHANNEL_NUM] = {0};
	int Ratio[SELF_IC_MAX_CHANNEL_NUM]={0};
	int Ratio_Off[SELF_IC_MAX_CHANNEL_NUM]={0};
	int Max_Ratio, Min_Ratio, Max_Ratio_border, Min_Ratio_border;
	int Sum1_Ratio=0 , Sum2_Ratio=0, Sum3_Ratio=0 ,Sum4_Ratio=0;  // 1-4 : \CF\C2border / \CF\C2va /\C9\CFborder / \C9\CFVA 
	int Avg1_Ratio=0 , Avg2_Ratio=0, Avg3_Ratio=0 ,Avg4_Ratio=0; 
	s16 *pMap= NULL;
	s16 map_data[20]={0}; 
	u8   i;

    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

	switch(nItemId)
	{
		case 40:
			 pMap = _gSelfIC_MAP1;
		       for(i=0;i< 20;i++)
			 map_data[i] = *(pMap+i);
		       for(i=0;i< 20;i++)
			 Golden_DeltaC[i]=Golden_0[i];
		    	for(i=0;i< 48;i++)
			 temp_DeltaC[i]=_gSelfICRawData1[i];
			 
			break;
		case 41:
			 pMap = _gSelfIC_MAP2;
		        for(i=0;i< 20;i++)
			 map_data[i] = *(pMap+i);
			for(i=0;i< 20;i++)
			 Golden_DeltaC[i]=Golden_1[i];
			for(i=0;i< 48;i++)
			  {
			  	temp_DeltaC[i]=_gSelfICRawData2[i];
			  }
			
			break;			
	}

	if(Golden_DeltaC[0]==0)
	{
		nRetVal=ITO_TEST_FAIL;
		return nRetVal;
	}

	sort_mp(map_data, 20);
	for(i=0;i<20;i++)
	{
		Ratio[i] = (100*temp_DeltaC[map_data[i]]) /Golden_DeltaC[i];

		if(nItemId==40)
		  {
			_gRawDataLog[i]=temp_DeltaC[map_data[i]];
		   }
		else{
			if(nItemId==41)
			_gRawDataLog[i+SELF_IC_MAX_CHANNEL_NUM]=temp_DeltaC[map_data[i]];
		   }

		
		if(!_DrvMpTestSelfCheckValueDoubleRange(Ratio[i], (100+DC_Range), (100-DC_Range)))
			 nRetVal=ITO_TEST_FAIL;		
	}

	for(i=0;i<20;i++)
	{
		if((i==0)||(i==20/2-1))
			Sum1_Ratio+=Ratio[i];
		if(i>0&&i<(20/2-1))
			Sum2_Ratio+=Ratio[i];
		if(i==20/2||i==20-1)
			Sum3_Ratio+=Ratio[i];
		if(i>20/2&&i<20-1)
			Sum4_Ratio+=Ratio[i];		
	}

	Avg1_Ratio= Sum1_Ratio /2 ;
	Avg2_Ratio= Sum2_Ratio /(20/2-2);
	Avg3_Ratio= Sum3_Ratio /2 ;
	Avg4_Ratio= Sum4_Ratio /(20/2-2);

	Max_Ratio =  (100 + DC_VA_Ratio ) ;
	Min_Ratio =  (100 - DC_VA_Ratio ) ;
	Max_Ratio_border =  (100 + DC_Border_Ratio ) ;
	Min_Ratio_border =  (100 - DC_Border_Ratio ) ;

	for(i=0;i<20;i++)
	{
		if(i==0||i==(20/2-1))
			Ratio_Off[i]=Ratio[i]+100-Avg1_Ratio;
		if(i>0 && i<(20/2-1))
			Ratio_Off[i]=Ratio[i]+100-Avg2_Ratio;
		if(i==20/2||i==20-1)
			Ratio_Off[i]=Ratio[i]+100-Avg3_Ratio;
		if(i>20/2&& i<20-1)
			Ratio_Off[i]=Ratio[i]+100-Avg4_Ratio;	
	}

	for(i=0;i<20;i++)
	{
		if(i==0||i==(20/2-1)||i==20/2||i==20-1)
			{
				if(!_DrvMpTestSelfCheckValueDoubleRange(Ratio_Off[i],Max_Ratio_border,Min_Ratio_border))
				 	nRetVal=ITO_TEST_FAIL;
			}
		else
			{ 
			if(!_DrvMpTestSelfCheckValueDoubleRange(Ratio_Off[i],Max_Ratio,Min_Ratio))
				 	nRetVal=ITO_TEST_FAIL;
			}
	}   

	for(i=0;i<20;i++)
	{
		 DBG(&g_I2cClient->dev, "Ratio[%d] = %d\n", i, Ratio[i]);
		 DBG(&g_I2cClient->dev, "Ratio_Off[%d] = %d\n", i, Ratio_Off[i]);

		 if(nItemId==40)
			_gRawDataLog[i+SELF_IC_MAX_CHANNEL_NUM/2]=Ratio_Off[i];
		else
			if(nItemId==41)
			_gRawDataLog[i+(3*SELF_IC_MAX_CHANNEL_NUM)/2]=Ratio_Off[i];
		 
	}
	
    return nRetVal;
}
//- bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

static ItoTestResult_e _DrvMpTestItoShortTestSelfICSecond(u8 nItemId)
{
    ItoTestResult_e nRetVal = ITO_TEST_OK;
    u32 i;
    u8 nSensorCount = 0;
    u8 nNumOfSensorMapping1 = 0, nNumOfSensorMapping2 = 0;
	
    DBG(&g_I2cClient->dev, "*** %s() nItemId = %d ***\n", __func__, nItemId);

    if (g_ChipType == CHIP_TYPE_MSG21XXA)
    {
        if ((_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) % 2 != 0)
        {
            nNumOfSensorMapping1 = (_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) / 2 + 1;
            nNumOfSensorMapping2 = nNumOfSensorMapping1;
        }
        else
        {
            nNumOfSensorMapping1 = (_gSelfICItoTestTriangleNum + _gSelfICItoTestKeyNum + _gSelfICItoTestDummyNum) / 2;
            nNumOfSensorMapping2 = nNumOfSensorMapping1;
            if (nNumOfSensorMapping2 % 2 != 0)
            {	
                nNumOfSensorMapping2 ++;
            }
        }        
    }

    if (nItemId == 1) // 39-1
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nSensorCount = nNumOfSensorMapping1;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nSensorCount = _gMsg22xxShortSubFrameNum1;
        }        
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        
        for (i = 0; i < nSensorCount; i ++)
        {
            if (_gSelfICRawData1[_gSelfIC_SHORT_MAP1[i]] > SELF_IC_SHORT_TEST_THRESHOLD)
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_SHORT_MAP1[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
    }
    else if (nItemId == 2) // 39-2
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nSensorCount = nNumOfSensorMapping2;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nSensorCount = _gMsg22xxShortSubFrameNum2;
        }        
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        
        for (i = 0; i < nSensorCount; i ++)
        {
            if (_gSelfICRawData2[_gSelfIC_SHORT_MAP2[i]] > SELF_IC_SHORT_TEST_THRESHOLD)
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_SHORT_MAP2[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
    }
    else if (nItemId == 3) // 39-3
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nSensorCount = _gSelfICItoTestTriangleNum;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nSensorCount = _gMsg22xxShortSubFrameNum3;
        }      
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
      
        for (i = 0; i < nSensorCount; i ++)
        {
            if (_gSelfICRawData3[_gSelfIC_SHORT_MAP3[i]] > SELF_IC_SHORT_TEST_THRESHOLD)
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_SHORT_MAP3[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
    }
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    else if (nItemId == 0) // 39-4 (2R)   
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
        if (g_ChipType == CHIP_TYPE_MSG21XXA)
        {
            nSensorCount = _gSelfICItoTestTriangleNum/2;
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nSensorCount = _gMsg22xxShortSubFrameNum4;
        }        
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        
        for (i = 0; i < nSensorCount; i ++)
        {
            if (_gSelfICRawData4[_gSelfIC_SHORT_MAP4[i]] > SELF_IC_SHORT_TEST_THRESHOLD)
            {
                _gSelfICTestFailChannel[_gTestFailChannelCount] = _gSelfIC_SHORT_MAP4[i];
                _gTestFailChannelCount ++; 
                nRetVal = ITO_TEST_FAIL;
            }
        }
    }
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
 
    DBG(&g_I2cClient->dev, "nSensorCount = %d\n", nSensorCount);

    return nRetVal;
}

static ItoTestResult_e _DrvMpTestSelfICItoOpenTestEntry(void) 
{
    ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK;
    u32 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG(&g_I2cClient->dev, "open test start\n");

    DrvPlatformLyrSetIicDataRate(g_I2cClient, 50000); //50 KHz
  
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();
    
    if (!_DrvMpTestItoTestSelfICChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        nRetVal1 = ITO_TEST_GET_TP_TYPE_ERROR;
        goto ITO_TEST_END;
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop cpu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    mdelay(50);
    
    for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gSelfICRawData1[i] = 0;
        _gSelfICRawData2[i] = 0;
        _gSelfICRawData3[i] = 0;
        _gSelfICDataFlag1[i] = 0;
        _gSelfICDataFlag2[i] = 0;
        _gSelfICDataFlag3[i] = 0;
        _gSelfICTestFailChannel[i] = 0;
    }	
	
    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    if (g_ChipType == CHIP_TYPE_MSG21XXA)
    {
        _DrvMpTestItoOpenTestMsg21xxaFirst(40, _gSelfICRawData1, _gSelfICDataFlag1);

        if (_gSelfICIsEnable2R)
        {
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
            nRetVal2 = _DrvMpTestItoOpenTestSelfICSecond2r(40);
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        }
        else
        {
            nRetVal2 = _DrvMpTestItoOpenTestSelfICSecond(40);
        }
    
        _DrvMpTestItoOpenTestMsg21xxaFirst(41, _gSelfICRawData2, _gSelfICDataFlag2);

        if (_gSelfICIsEnable2R)
        {
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
            nRetVal3 = _DrvMpTestItoOpenTestSelfICSecond2r(41);
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        }
        else
        {
            nRetVal3 = _DrvMpTestItoOpenTestSelfICSecond(41);
        }
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        _DrvMpTestItoOpenTestMsg22xxFirst(40, _gSelfICRawData1, _gSelfICDataFlag1);

        if (_gSelfICIsEnable2R)
        {
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
	if(1)
	{
		nRetVal2 = _DrvMpTestItoOpenTestSelfICSecond2r_Golden(40);
	}else
	{
            nRetVal2 = _DrvMpTestItoOpenTestSelfICSecond2r(40);
	}
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        }
        else
        {
            nRetVal2 = _DrvMpTestItoOpenTestSelfICSecond(40);
        }
    
        _DrvMpTestItoOpenTestMsg22xxFirst(41, _gSelfICRawData2, _gSelfICDataFlag2);

        if (_gSelfICIsEnable2R)
        {
#ifdef CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
    if(1)
	{
		nRetVal3 = _DrvMpTestItoOpenTestSelfICSecond2r_Golden(41);
	}else
	{
            nRetVal3 = _DrvMpTestItoOpenTestSelfICSecond2r(41);
	}
#endif //CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
        }
        else
        {
            nRetVal3 = _DrvMpTestItoOpenTestSelfICSecond(41);
        }
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

ITO_TEST_END:

    DrvPlatformLyrSetIicDataRate(g_I2cClient, 100000); //100 KHz
    
    DrvPlatformLyrTouchDeviceResetHw();
    DrvPlatformLyrEnableFingerTouchReport();

    DBG(&g_I2cClient->dev, "open test end\n");

    if ((nRetVal1 != ITO_TEST_OK) && (nRetVal2 == ITO_TEST_OK) && (nRetVal3 == ITO_TEST_OK))
    {
        return ITO_TEST_GET_TP_TYPE_ERROR;		
    }
    else if ((nRetVal1 == ITO_TEST_OK) && ((nRetVal2 != ITO_TEST_OK) || (nRetVal3 != ITO_TEST_OK)))
    {
        return ITO_TEST_FAIL;	
    }
    else
    {
        return ITO_TEST_OK;	
    }
}

static ItoTestResult_e _DrvMpTestSelfICItoShortTestEntry(void)
{
    ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK, nRetVal4 = ITO_TEST_OK, nRetVal5 = ITO_TEST_OK;
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG(&g_I2cClient->dev, "short test start\n");

    DrvPlatformLyrSetIicDataRate(g_I2cClient, 50000); //50 KHz
  
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw(); 
    
    if (!_DrvMpTestItoTestSelfICChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        nRetVal1 = ITO_TEST_GET_TP_TYPE_ERROR;
        goto ITO_TEST_END;
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop cpu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

    // Stop watchdog
    RegSet16BitValue(0x3C60, 0xAA55); //bank:reg_PIU_MISC_0, addr:h0030

    mdelay(50);
    
    for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gSelfICRawData1[i] = 0;
        _gSelfICRawData2[i] = 0;
        _gSelfICRawData3[i] = 0;
        _gSelfICRawData4[i] = 0;
        _gSelfICDataFlag1[i] = 0;
        _gSelfICDataFlag2[i] = 0;
        _gSelfICDataFlag3[i] = 0;
        _gSelfICDataFlag4[i] = 0;
        _gSelfICTestFailChannel[i] = 0;
    }	
	
    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start
	
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
    if (g_ChipType == CHIP_TYPE_MSG21XXA)
    {
        if (_gSelfICIsEnable2R)
        {
            _DrvMpTestItoShortTestMsg21xxaFirst(0, _gSelfICRawData4, _gSelfICDataFlag4);
            nRetVal2 = _DrvMpTestItoShortTestSelfICSecond(0);
        }

        _DrvMpTestItoShortTestMsg21xxaFirst(1, _gSelfICRawData1, _gSelfICDataFlag1);
        nRetVal3 = _DrvMpTestItoShortTestSelfICSecond(1);

        _DrvMpTestItoShortTestMsg21xxaFirst(2, _gSelfICRawData2, _gSelfICDataFlag2);
        nRetVal4 = _DrvMpTestItoShortTestSelfICSecond(2);

        _DrvMpTestItoShortTestMsg21xxaFirst(3, _gSelfICRawData3, _gSelfICDataFlag3);
        nRetVal5 = _DrvMpTestItoShortTestSelfICSecond(3);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        if (_gSelfICIsEnable2R)
        {
            _DrvMpTestItoShortTestMsg22xxFirst(0, _gSelfICRawData4, _gSelfICDataFlag4);
            nRetVal2 = _DrvMpTestItoShortTestSelfICSecond(0);
        }

        _DrvMpTestItoShortTestMsg22xxFirst(1, _gSelfICRawData1, _gSelfICDataFlag1);
        nRetVal3 = _DrvMpTestItoShortTestSelfICSecond(1);

        _DrvMpTestItoShortTestMsg22xxFirst(2, _gSelfICRawData2, _gSelfICDataFlag2);
        nRetVal4 = _DrvMpTestItoShortTestSelfICSecond(2);

        _DrvMpTestItoShortTestMsg22xxFirst(3, _gSelfICRawData3, _gSelfICDataFlag3);
        nRetVal5 = _DrvMpTestItoShortTestSelfICSecond(3);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

ITO_TEST_END:

    DrvPlatformLyrSetIicDataRate(g_I2cClient, 100000); //100 KHz
    
    DrvPlatformLyrTouchDeviceResetHw();
    DrvPlatformLyrEnableFingerTouchReport();

    DBG(&g_I2cClient->dev, "short test end\n");
    
    if ((nRetVal1 != ITO_TEST_OK) && (nRetVal2 == ITO_TEST_OK) && (nRetVal3 == ITO_TEST_OK) && (nRetVal4 == ITO_TEST_OK) && (nRetVal5 == ITO_TEST_OK))
    {
        return ITO_TEST_GET_TP_TYPE_ERROR;		
    }
    else if ((nRetVal1 == ITO_TEST_OK) && ((nRetVal2 != ITO_TEST_OK) || (nRetVal3 != ITO_TEST_OK) || (nRetVal4 != ITO_TEST_OK) || (nRetVal5 != ITO_TEST_OK)))
    {
        return ITO_TEST_FAIL;
    }
    else
    {
        return ITO_TEST_OK;	
    }
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
static void _DrvMpTestItoTestMsg26xxmSetToNormalMode(void)
{
    u16 nRegData = 0;
    u16 nTmpAddr = 0, nAddr = 0;
    u16 nDriveNumGeg = 0, nSenseNumGeg = 0;
    u16 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001); 

    nRegData = RegGet16BitValue(0x110E); 
    
    if (nRegData == 0x1D08)
    {
        DBG(&g_I2cClient->dev, "Wrong mode 0\n");
    }
    
    nRegData &= 0x0800;
    
    if (nRegData > 1)
    {
        DBG(&g_I2cClient->dev, "Wrong mode\n");
    }
    
    RegSet16BitValueOff(0x110E, 0x0800); 
    RegSet16BitValueOn(0x1116, 0x0005); 
    RegSet16BitValueOn(0x114A, 0x0001); 

    for (i = 0; i < 7; i ++)
    {
        nTmpAddr = 0x3C + i;	
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x11 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_open_ANA1_N_X[i]); 
    }
    
    RegSet16BitValue(0x1E66, 0x0000); 
    RegSet16BitValue(0x1E67, 0x0000); 
    RegSet16BitValue(0x1E68, 0x0000); 
    RegSet16BitValue(0x1E69, 0x0000); 
    RegSet16BitValue(0x1E6A, 0x0000); 
    RegSet16BitValue(0x1E6B, 0x0000); 
    
    for (i = 0; i < 21; i ++)
    {
        nTmpAddr = 3 + i;	
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_open_ANA3_N_X[i]); 
    }
    
    nDriveNumGeg = ((MSG26XXM_DRIVE_NUM - 1) << 8 & 0xFF00);
    nSenseNumGeg = (MSG26XXM_SENSE_NUM & 0x00FF);
    
    RegSet16BitValue(0x1216, nDriveNumGeg); 
    RegSet16BitValue(0x102E, nSenseNumGeg); 

    RegSet16BitValue(0x0FE6, 0x0001); 

    DBG(&g_I2cClient->dev, "Wrong mode correction\n");
}

/*
static void _DrvMpTestItoTestMsg26xxmSetToWaterProofMode(void)
{
    u16 nTmpAddr = 0, nAddr = 0;
    u16 i = 0;
    u16 nCsubWPdata = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nCsubWPdata = (WATERPROOF_CSUB_REF << 8) | WATERPROOF_CSUB_REF;

    for (i = 0; i < 7; i ++)
    {
        nTmpAddr = 0x3C + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x11 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_WATERPROOF_ANA1_N_X[i]);
    }

    for (i = 0; i < 21; i ++)
    {
        nTmpAddr = 3 + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_WATERPROOF_ANA3_N_X[i]);
    }

    // ana2 write
    RegSet16BitValue(0x1216, 0x0000);

    for (i = 0; i < 6; i ++)
    {
        nTmpAddr = 0x20 + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, nCsubWPdata);
    }

    RegSet16BitValueOn(0x1116, 0x0005);
    RegSet16BitValueOn(0x114A, 0x0001);
    RegSet16BitValue(0x1208, 0x0002);

    DBG(&g_I2cClient->dev, "WaterProof : Wrong mode correction\n");
}
*/

static void _DrvMpTestItoTestMsg26xxmMcuStop(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
}

static void _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual(void)
{
    u16 nTemp = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x114A); //bank:ana, addr:h0025
    nTemp |= BIT0;
    RegSet16BitValue(0x114A, nTemp);
    nTemp = RegGet16BitValue(0x1116); //bank:ana, addr:h000b
    nTemp |= (BIT2 | BIT0);
    RegSet16BitValue(0x1116, nTemp);
}

static u16 _DrvMpTestItoTestMsg26xxmAnaGetMutualChannelNum(void)
{
    u16 nSenseLineNum = 0;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nRegData = RegGet16BitValue(0x102E); //bank:ana3, addr:h0017
    nSenseLineNum = nRegData & 0x000F;

    DBG(&g_I2cClient->dev, "nSenseLineNum = %d\n", nSenseLineNum);

    return nSenseLineNum;
}

static u16 _DrvMpTestItoTestMsg26xxmAnaGetMutualSubFrameNum(void)
{
    u16 nDriveLineNum = 0;
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nRegData = RegGet16BitValue(0x1216); //bank:ana2, addr:h000b
    nDriveLineNum = ((nRegData & 0xFF00) >> 8) + 1; //Since we only retrieve 8th~12th bit of reg_m_sf_num, 0xFF00 shall be changed to 0x1F00. 

    DBG(&g_I2cClient->dev, "nDriveLineNum = %d\n", nDriveLineNum);

    return nDriveLineNum;
}

/*
static void _DrvMpTestItoOpenTestMsg26xxmAnaGetMutualCSub(u8 *pMode)
{
    u16 i, j;
    u16 nSenseLineNum;
    u16 nDriveLineNum;
    u16 nTotalNum;
    u8 szDataAna4[3];    
    u8 szDataAna3[3];    
    u8 szDataAna41[MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER]; //200 = 392 - 192  
    u8 szDataAna31[MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER]; //192 = 14 * 13 + 10   
    u8 szModeTemp[MUTUAL_IC_MAX_MUTUAL_NUM];

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTotalNum = MUTUAL_IC_MAX_MUTUAL_NUM;

    nSenseLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualChannelNum();
    nDriveLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualSubFrameNum();

    if (MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER > 0)
    {
        mdelay(100);
        for (i = 0; i < MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER; i ++)
        {	
            szDataAna41[i] = 0;
        }

        szDataAna4[0] = 0x10;
        szDataAna4[1] = 0x15; //bank:ana4, addr:h0000
        szDataAna4[2] = 0x00;
        
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDataAna4[0], 3);
        IicReadData(SLAVE_I2C_ID_DBBUS, &szDataAna41[0], MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER); //200

        nTotalNum -= (u16)MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER;
    }

    for (i = 0; i < nTotalNum; i ++)
    {
        szDataAna31[i] = 0;
    }

    mdelay(100);

    szDataAna3[0] = 0x10;
    szDataAna3[1] = 0x10; //bank:ana3, addr:h0020
    szDataAna3[2] = 0x40;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDataAna3[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szDataAna31[0], MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER); //192

    for (i = 0; i < MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER; i ++)
    {
        szModeTemp[i] = szDataAna31[i];
    }

    for (i = MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER; i < (MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER + MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER); i ++)
    {
        szModeTemp[i] = szDataAna41[i - MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER];
    }
    
    for (i = 0; i < nDriveLineNum; i ++)
    {
        for (j = 0; j < nSenseLineNum; j ++)
        {
            _gMutualICMode[j * nDriveLineNum + i] = szModeTemp[i * MUTUAL_IC_MAX_CHANNEL_SEN + j];

//            DBG(&g_I2cClient->dev, "_gMutualICMode[%d] = %d\n", j * nDriveLineNum + i, _gMutualICMode[j * nDriveLineNum + i]);
        }
    }
}
*/

static void _DrvMpTestItoOpenTestMsg26xxmAnaSetMutualCSub(u16 nCSub)
{
    u16 i = 0;
    u8 szDbBusTxData[256] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x15; //bank:ana4, addr:h0000
    szDbBusTxData[2] = 0x00;

    for (i = 3; i < (3+MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER); i ++)
    {
        szDbBusTxData[i] = (u8)nCSub;             
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x10; //bank:ana3, addr:h0020
    szDbBusTxData[2] = 0x40;

    for (i = 3; i < (3+MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER); i ++)
    {
        szDbBusTxData[i] = (u8)nCSub;             
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER);	
}

static void _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect(void)
{
    u16 nTemp = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x1302); //bank:fir, addr:h0001
    nTemp &= (~(BIT2 | BIT1 | BIT0));
    RegSet16BitValue(0x1302, nTemp);
}

static void _DrvMpTestItoTestMsg26xxmAnaSwReset(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x1100, 0xFFFF); //bank:ana, addr:h0000
    RegSet16BitValue(0x1100, 0x0000);
    mdelay(100);
}

static void _DrvMpTestItoTestMsg26xxmEnableAdcOneShot(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x130C, BIT15); //bank:fir, addr:h0006
    RegSet16BitValue(0x1214, 0x0031);
}

static void _DrvMpTestItoTestMsg26xxmGetMutualOneShotRawIir(u16 wszResultData[][MUTUAL_IC_MAX_CHANNEL_DRV], u16 nDriveLineNum, u16 nSenseLineNum)
{
    u16 nRegData;
    u16 i, j;
    u16 nTemp;
    u16 nReadSize;
    u8 szDbBusTxData[3];
    u8 szShotData1[MSG26XXM_FILTER1_MUTUAL_DELTA_C_NUMBER]; //190 = (6 * 14 + 11) * 2
    u8 szShotData2[MSG26XXM_FILTER2_MUTUAL_DELTA_C_NUMBER]; //594 = (MUTUAL_IC_MAX_MUTUAL_NUM - (6 * 14 + 11)) * 2

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp &= (~(BIT8 | BIT4));
    RegSet16BitValue(0x3D08, nTemp);

    _DrvMpTestItoTestMsg26xxmEnableAdcOneShot();
    
    nRegData = 0;
    while (0x0000 == (nRegData & BIT8))
    {
        nRegData = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    }

    for (i = 0; i < MSG26XXM_FILTER1_MUTUAL_DELTA_C_NUMBER; i ++)
    {
        szShotData1[i] = 0;
    }
    
    for (i = 0; i < MSG26XXM_FILTER2_MUTUAL_DELTA_C_NUMBER; i ++)
    {
        szShotData2[i] = 0;
    }

    mdelay(100);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x13; //bank:fir, addr:h0021
    szDbBusTxData[2] = 0x42;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData1[0], MSG26XXM_FILTER1_MUTUAL_DELTA_C_NUMBER); //190

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    mdelay(100);
    nReadSize = IicSegmentReadDataByDbBus(0x20, 0x00, &szShotData2[0], MSG26XXM_FILTER2_MUTUAL_DELTA_C_NUMBER, MAX_I2C_TRANSACTION_LENGTH_LIMIT); //594
    DBG(&g_I2cClient->dev, "*** nReadSize = %d ***\n", nReadSize); // add for debug
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    mdelay(100);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x20; //bank:fir2, addr:h0000
    szDbBusTxData[2] = 0x00;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData2[0], MSG26XXM_FILTER2_MUTUAL_DELTA_C_NUMBER); //594
#endif

    for (j = 0; j < nDriveLineNum; j ++)
    {
        for (i = 0; i < nSenseLineNum; i ++)
        {
            // FILTER1 : SF0~SF5, AFE0~AFE13; SF6, AFE0~AFE10
            if ((j <= 5) || ((j == 6) && (i <= 10)))
            {
                nRegData = (u16)(szShotData1[(j * 14 + i) * 2] | szShotData1[(j * 14 + i) * 2 + 1] << 8);
                wszResultData[i][ j] = (short)nRegData;
            }
            else
            {
                // FILTER2 : SF6, AFE11~AFE13
                if ((j == 6) && (i > 10))
                {
                    nRegData = (u16)(szShotData2[((j - 6) * 14 + (i - 11)) * 2] | szShotData2[((j - 6) * 14 + (i - 11)) * 2 + 1] << 8);
                    wszResultData[i][j] = (short)nRegData;
                }
                else
                {
                    nRegData = (u16)(szShotData2[6 + ((j - 7) * 14 + i) * 2] | szShotData2[6 + ((j - 7) * 14 + i) * 2 + 1] << 8);
                    wszResultData[i][j] = (short)nRegData;
                }
            }
        }
    }

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp |= (BIT8 | BIT4);
    RegSet16BitValue(0x3D08, nTemp);
}

static void _DrvMpTestItoTestMsg26xxmGetDeltaC(s32 *pTarget)
{
    s16 nTemp;
    u16 wszRawData[MUTUAL_IC_MAX_CHANNEL_SEN][MUTUAL_IC_MAX_CHANNEL_DRV];
    u16 i, j;
    u16 nDriveLineNum = 0, nSenseLineNum = 0, nShift = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nSenseLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualChannelNum();
    nDriveLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualSubFrameNum();

    _DrvMpTestItoTestMsg26xxmGetMutualOneShotRawIir(wszRawData, nDriveLineNum, nSenseLineNum);

    for (i = 0; i < nSenseLineNum; i ++)
    {
        for (j = 0; j < nDriveLineNum; j ++)
        {
            nShift = (u16)(i * nDriveLineNum + j);
            nTemp = (s16)wszRawData[i][j];
            pTarget[nShift] = nTemp;

//            DBG(&g_I2cClient->dev, "wszRawData[%d][%d] = %d\n", i, j, nTemp);
        }
    }
}

static s32 _DrvMpTestItoTestMsg26xxmReadTrunkFwVersion(u32* pVersion)
{
    u16 nMajor = 0;
    u16 nMinor = 0;
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x53;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x24;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    DBG(&g_I2cClient->dev, "szDbBusRxData[0] = 0x%x\n", szDbBusRxData[0]); // add for debug
    DBG(&g_I2cClient->dev, "szDbBusRxData[1] = 0x%x\n", szDbBusRxData[1]); // add for debug
    DBG(&g_I2cClient->dev, "szDbBusRxData[2] = 0x%x\n", szDbBusRxData[2]); // add for debug
    DBG(&g_I2cClient->dev, "szDbBusRxData[3] = 0x%x\n", szDbBusRxData[3]); // add for debug

    nMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
    nMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];

    DBG(&g_I2cClient->dev, "*** major = %x ***\n", nMajor);
    DBG(&g_I2cClient->dev, "*** minor = %x ***\n", nMinor);

    *pVersion = (nMajor << 16) + nMinor;

    return 0;
}

static s32 _DrvMpTestItoTestMsg26xxmGetSwitchFlag(void)
{
    u32 nFwVersion = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_DrvMpTestItoTestMsg26xxmReadTrunkFwVersion(&nFwVersion) < 0)
    {
        _gMutualICTestSwitchMode = 0;
        return 0;
    }

    if (nFwVersion >= 0x10030000)
    {
        _gMutualICTestSwitchMode = 1;
    }
    else
    {
        _gMutualICTestSwitchMode = 0;
    }

    return 0;
}

static s32 _DrvMpTestItoTestMsg26xxmCheckSwitchStatus(void)
{
    u32 nRegData = 0;
    int nTimeOut = 100;
    int nCount = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
        mdelay(20);
        nCount ++;
        if (nCount > nTimeOut)
        {
            return -1;
        }
        DBG(&g_I2cClient->dev, "*** %s() nRegData:%x***\n", __func__ , nRegData);

    } while (nRegData != 0x7447);

    return 0;
}

static s32 _DrvMpTestItoTestMsg26xxmSwitchFwMode(u8 nFWMode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001);    //MCU_stop
    mdelay(150);

    RegSet16BitValue(0X3C60, 0xAA55);    // disable watch dog

    RegSet16BitValue(0X3D08, 0xFFFF);
    RegSet16BitValue(0X3D18, 0xFFFF);

    RegSet16BitValue(0x3CE4, 0x7474);

    //RegSet16BitValue(0x1E04, 0x7D60);
    RegSet16BitValue(0x1E04, 0x829F);
    RegSet16BitValue(0x0FE6, 0x0000);
    mdelay(150);

    if (_DrvMpTestItoTestMsg26xxmCheckSwitchStatus() < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg26xx MP Test# CheckSwitchStatus failed! ***\n");
        return -1;
    }

    switch (nFWMode)
    {
        case MUTUAL:
            RegSet16BitValue(0x3CE4, 0x5705);
            break;

        case SELF:
            RegSet16BitValue(0x3CE4, 0x6278);
            break;

        case WATERPROOF:
            RegSet16BitValue(0x3CE4, 0x7992);
            DBG(&g_I2cClient->dev, "*** Msg26xx MP Test# WATERPROOF mode***\n");
            break;

        default:
            return -1;
    }
    if (_DrvMpTestItoTestMsg26xxmCheckSwitchStatus() < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg26xx MP Test# CheckSwitchStatus failed! ***\n");
        return -1;
    }

    RegSet16BitValue(0x0FE6, 0x0001);// stop mcu
    RegSet16BitValue(0x3D08, 0xFEFF);//open timer

    return 0;
}

static s32 _DrvMpTestItoTestMsg26xxmSwitchMode(u8 nSwitchMode, u8 nFMode)
{
    if (_gMutualICTestSwitchMode != 0)
    {
        if (_DrvMpTestItoTestMsg26xxmSwitchFwMode(nFMode) < 0)
        {
            if (nFMode == MUTUAL)
            {
                _DrvMpTestItoTestMsg26xxmSetToNormalMode();
            }
            else
            {
                //_DrvMpTestItoTestMsg26xxmSetToWaterProofMode();
                DBG(&g_I2cClient->dev, "*** Msg26xx MP Test# _DrvMpTestItoTestMsg26xxmSwitchMode failed! ***\n");
                return -1;
            }
        }
    }
    else
    {
        if (nFMode == MUTUAL)
        {
            _DrvMpTestItoTestMsg26xxmSetToNormalMode();
        }
        else
        {
            //_DrvMpTestItoTestMsg26xxmSetToWaterProofMode();
            DBG(&g_I2cClient->dev, "*** Msg26xx MP Test# _DrvMpTestItoTestMsg26xxmSwitchMode failed! ***\n");
            return -1;
        }
    }
    
    return 0;
}

static s32 _DrvMpTestMsg26xxmItoOpenTestEntry(void)
{
    s32 nRetVal = 0;
    s32 nPrev = 0, nDelta = 0;
    u16 i = 0, j = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();

    DrvPlatformLyrTouchDeviceResetHw();

    //auto detetc fw version to decide switch fw mode
    if (_gMutualICTestAutoSwitchFlag != 0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    _DrvMpTestItoTestMsg26xxmSwitchMode(_gMutualICTestSwitchMode, MUTUAL);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    mdelay(10);

    for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i ++)
    {
        _gMutualICTestFailChannel[i] = 0;
    }	

    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start

    _gMutualICSenseLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualChannelNum();
    _gMutualICDriveLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualSubFrameNum();

    _DrvMpTestItoOpenTestMsg26xxmAnaSetMutualCSub(MSG26XXM_OPEN_CSUB_REF_X);

    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    
    // Set charge&dump time 
    RegSet16BitValue(0x1224, 0xFFC0);
    RegSet16BitValue(0x122A, 0x0C0A);

    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gMutualICDeltaC); 
    
    for (i = 0; i < _gMutualICSenseLineNum; i ++)
    {
        DBG(&g_I2cClient->dev, "\nSense[%02d]\t", i);
        
        for (j = 0; j < _gMutualICDriveLineNum; j ++)
        {
            _gMutualICResult[i * _gMutualICDriveLineNum + j] = (4464*MSG26XXM_OPEN_CSUB_REF_X - _gMutualICDeltaC[i * _gMutualICDriveLineNum + j]);
            DBG(&g_I2cClient->dev, "%d  %d  %d\t", _gMutualICResult[i * _gMutualICDriveLineNum + j], 4464*MSG26XXM_OPEN_CSUB_REF_X, _gMutualICDeltaC[i * _gMutualICDriveLineNum + j]);
        }
    }
    
    DBG(&g_I2cClient->dev, "\n\n\n");

//    for (j = 0; j < _gMutualICDriveLineNum; j ++)
    for (j = 0; j < (_gMutualICDriveLineNum-1); j ++)
    {
        for (i = 0; i < _gMutualICSenseLineNum; i ++)
        {
            if (_gMutualICResult[i * _gMutualICDriveLineNum + j] < MSG26XXM_FIR_THRESHOLD)
            {
                _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j] = 1;
                _gTestFailChannelCount ++; 
                nRetVal = -1;
                DBG(&g_I2cClient->dev, "\nSense%d, Drive%d, MIN_Threshold = %d\t", i, j, _gMutualICResult[i * _gMutualICDriveLineNum + j]);
            }

            if (i > 0)
            {
                nDelta = _gMutualICResult[i * _gMutualICDriveLineNum + j] > nPrev ? (_gMutualICResult[i * _gMutualICDriveLineNum + j] - nPrev) : (nPrev - _gMutualICResult[i * _gMutualICDriveLineNum + j]);
                if (nDelta > nPrev*MUTUAL_IC_FIR_RATIO/100)
                {
                    if (0 == _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j]) // for avoid _gTestFailChannelCount to be added twice
                    {
                        _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j] = 1;
                        _gTestFailChannelCount ++; 
                    }
                    nRetVal = -1;
                    DBG(&g_I2cClient->dev, "\nSense%d, Drive%d, MAX_Ratio = %d,%d\t", i, j, nDelta, nPrev);
                }
            }
            nPrev = _gMutualICResult[i * _gMutualICDriveLineNum + j];
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

static void _DrvMpTestItoTestMsg26xxmSendDataIn(u16 nAddr, u16 nLength, u16 *data)
{
    u8 szDbBusTxData[256] = {0};
    int i = 0;

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = (nAddr >> 8) & 0xFF;
    szDbBusTxData[2] = (nAddr & 0xFF);

    for (i = 0; i <= nLength ; i ++)
    {
        szDbBusTxData[3+2*i] = (data[i] & 0xFF);
        szDbBusTxData[4+2*i] = (data[i] >> 8) & 0xFF;
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+nLength*2);
}

static void _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(u8 nItemID)
{
    u16 gpioSetting[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
    u16 gpioEnabling[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
    u16 gpioZero[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
    u8 	gpioNum = 0;
    u16 *gpioPIN = NULL;
    int i = 0;
    int j = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nItemID == 1)
    {
        gpioNum = SHORT_N1_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(u16) * gpioNum, GFP_KERNEL);

        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_N1_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 2)
    {
        gpioNum = SHORT_N2_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(u16) * gpioNum, GFP_KERNEL);
    
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_N2_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 3)
    {
        gpioNum = SHORT_S1_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(u16) * gpioNum, GFP_KERNEL);
		
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_S1_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 4)
    {
        gpioNum = SHORT_S2_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(u16) * gpioNum, GFP_KERNEL);
		
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_S2_GPO_PIN_X[i];
        }
    }
    DBG(&g_I2cClient->dev, "ItemID %d, gpioNum %d",nItemID, gpioNum);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        gpioEnabling[i] = 0xFFFF;
    }

    for (i = 0; i < gpioNum; i++)
    {
        gpioSetting[gpioPIN[i] / 16] |= (u16)(1 << (gpioPIN[i] % 16));
        gpioEnabling[gpioPIN[i] / 16] &= (u16)(~(1 << (gpioPIN[i] % 16)));
    }

    ///cts sw overwrite
    {
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1E66, gpioNum, &gpioSetting[0]);   ///who -> will be controlled
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1E6C, gpioNum, &gpioEnabling[0]);   ///who -> enable sw overwrite
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1E72, gpioNum, &gpioZero[0]);       ///who -> set2GPO
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1E78, gpioNum, &gpioZero[0]);       ///who -> GPO2GND
    }

    for (j = 0; j < gpioNum; j++)
    {
        if (MSG26XXM_PIN_GUARD_RING == gpioPIN[j])
        {
            u16 u16RegData;
            u16RegData = RegGet16BitValue(0x1E12);
            u16RegData = ((u16RegData & 0xFFF9) | BIT0);
            RegSet16BitValue(0x1E12, u16RegData);
        }
    }

    kfree(gpioPIN);
}

static void _DrvMpTestItoShortTestMsg26xxmChangeANASetting(u8 nItemID)
{
    u16 SHORT_MAP_ANA1[7] = {0};
    u16 SHORT_MAP_ANA2[1] = {0};
    u16 SHORT_MAP_ANA3[21] = {0};
    int i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nItemID == 1)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_N1_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_N1_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_N1_X[0];
    }
    else if (nItemID == 2)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_N2_X[i];
        }
		
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_N2_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_N2_X[0];
    }
    else if (nItemID == 3)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_S1_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_S1_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_S1_X[0];
    }
    else if (nItemID == 4)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_S2_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_S2_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_S2_X[0];
    }

    ///change ANA setting
    {
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1178, 7, &SHORT_MAP_ANA1[0]);		///ANA1_3C_42
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1216, 1, &SHORT_MAP_ANA2[0]);   	///ANA2_0B
        _DrvMpTestItoTestMsg26xxmSendDataIn(0x1006, 21, &SHORT_MAP_ANA3[0]);    ///ANA3_03_17
    }
}

static void _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(u16 nOption)
{
    u16 nTemp = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x1208); //bank:ana2, addr:h000a
    nTemp &= 0x00F1;
    nTemp |= (u16)((nOption << 1) & 0x000E);
    RegSet16BitValue(0x1208, nTemp);
}

static void _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(u8 nEnable)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nEnable)
    {
        RegSet16BitValueOn(0x110E, BIT11);
        RegSet16BitValueOff(0x1116, BIT2);
    }
    else
    {
        RegSet16BitValueOff(0x110E, BIT11);
    }
}

static void _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(u16 nTime1, u16 nTime2)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x1224, nTime1);
    RegSet16BitValue(0x122A, nTime2);
}

static void _DrvMpTestMsg26xxmItoShortTest(u8 nItemID)
{
    int i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(nItemID);
    _DrvMpTestItoShortTestMsg26xxmChangeANASetting(nItemID);
    _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual();
    _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(7);
    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(1);
    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(SHORT_Charge_X, SHORT_Dump1_X);
    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gMutualICTempDeltaC);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(nItemID);
    _DrvMpTestItoShortTestMsg26xxmChangeANASetting(nItemID);
    _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual();
    _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(7);
    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(1);
    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(SHORT_Charge_X, SHORT_Dump2_X);
    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gMutualICDeltaC);

    for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i ++)
    {
        if ((_gMutualICDeltaC[i] <= -(MUTUAL_IC_IIR_MAX)) || (_gMutualICTempDeltaC[i] <= -(MUTUAL_IC_IIR_MAX)) || (_gMutualICDeltaC[i] >= (MUTUAL_IC_IIR_MAX)) || (_gMutualICTempDeltaC[i] >= (MUTUAL_IC_IIR_MAX)))
        {
            _gMutualICDeltaC[i] = 0x7FFF;
        }
        else
        {
            _gMutualICDeltaC[i] = abs(_gMutualICDeltaC[i] - _gMutualICTempDeltaC[i]);
        }
        //DBG(&g_I2cClient->dev, "ItemID%d, MUTUAL_NUM %d, _gMutualICDeltaC = %d\t", nItemID, i, _gMutualICDeltaC[i]);
    }
    DBG(&g_I2cClient->dev, "\n");
}

static s32 _DrvMpTestItoShortTestMsg26xxmCovertRValue(s32 nValue)
{
   	if (nValue == 0)
   	{
   	   	nValue = 1;
   	}   	   	   	

   	if (nValue >= MUTUAL_IC_IIR_MAX)
   	{
   	   	return 0;
   	}

   	return ((500*11398) / (nValue));
}

static ItoTestResult_e _DrvMpTestItoShortTestMsg26xxmJudge(u8 nItemID)
{
   	ItoTestResult_e nRetVal = ITO_TEST_OK;
   	u8 nTestPinLength = 0;
   	u16 i = 0;
   	u8 nGpioNum = 0;
   	u8* pTestGpio = NULL;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

   	if (nItemID == 1)
   	{
   	   	nGpioNum = SHORT_N1_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(u8) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_N1_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 2)
   	{
   	   	nGpioNum = SHORT_N2_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(u8) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_N2_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 3)
   	{
   	   	nGpioNum = SHORT_S1_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(u8) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_S1_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 4)
   	{
   	   	nGpioNum = SHORT_S2_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(u8) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_S2_TEST_PIN_X[i];
   	   	}
   	}

   	nTestPinLength = nGpioNum;

   	for (i = 0;i < nTestPinLength; i++)
   	{
   	   	_gMsg26xxmShortTestChannel[i] = pTestGpio[i];

   	   	if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICDeltaC[i], MSG26XXM_SHORT_VALUE, -MSG26XXM_SHORT_VALUE))
   	   	{
   	   	   	nRetVal = ITO_TEST_FAIL;
   	   	   	_gTestFailChannelCount++;
   	   	   	DBG(&g_I2cClient->dev, "_gMsg26xxmShortTestChannel i = %d, _gMutualICDeltaC = %d\t", i, _gMutualICDeltaC[i]);
   	   	}
   	}
   	
   	kfree(pTestGpio);

   	return nRetVal;
}

static ItoTestResult_e _DrvMpTestMsg26xxmItoShortTestEntry(void)
{
    ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK, nRetVal4 = ITO_TEST_OK, nRetVal5 = ITO_TEST_OK;
    u32 i = 0;
    u32 j = 0;
    u16 nTestPinCount = 0;
    s32 nShortThreshold = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();
    
    DrvPlatformLyrTouchDeviceResetHw();

	  //auto detetc fw version to decide switch fw mode
    if (_gMutualICTestAutoSwitchFlag != 0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

  	_DrvMpTestItoTestMsg26xxmSwitchMode(_gMutualICTestSwitchMode, MUTUAL);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0;
    }

    for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i ++)
    {
        _gMutualICDeltaC[i] = 0;
    }

    _gMutualICSenseLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualChannelNum();
    _gMutualICDriveLineNum = _DrvMpTestItoTestMsg26xxmAnaGetMutualSubFrameNum();

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0xff;
    }

    _gTestFailChannelCount = 0;

    nTestPinCount = 0; // Reset nTestPinCount to 0 before test start

    //N1_ShortTest

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(1); 
    }

    nRetVal2 = _DrvMpTestItoShortTestMsg26xxmJudge(1);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        if (_gMsg26xxmShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gMutualICSenseLineNum; j++)
        {
            if (_gMsg26xxmShortTestChannel[i] == SENSE_X[j])
            {
                _gMutualICSenseR[j] = _DrvMpTestItoShortTestMsg26xxmCovertRValue(_gMutualICDeltaC[i]);

                DBG(&g_I2cClient->dev, "_gMutualICSenseR[%d] = %d\t", j , _gMutualICSenseR[j]);
            }
        }
    }
    DBG(&g_I2cClient->dev, "\n");

    //clear
    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0xff;
    }

    nTestPinCount = 0;

    //N2_ShortTest

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(2);
    }

    nRetVal3 = _DrvMpTestItoShortTestMsg26xxmJudge(2);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        if (_gMsg26xxmShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }            
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gMutualICSenseLineNum; j++)
        {
            if (_gMsg26xxmShortTestChannel[i] == SENSE_X[j])
            {
                _gMutualICSenseR[j] = _DrvMpTestItoShortTestMsg26xxmCovertRValue(_gMutualICDeltaC[i]);

                DBG(&g_I2cClient->dev, "_gMutualICSenseR[%d] = %d\t", j , _gMutualICSenseR[j]);
            }
        }
    }
    DBG(&g_I2cClient->dev, "\n");

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(3);
    }
    
    nRetVal4 = _DrvMpTestItoShortTestMsg26xxmJudge(3);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        if (_gMsg26xxmShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gMutualICDriveLineNum; j++)
        {
            if (_gMsg26xxmShortTestChannel[i] == DRIVE_X[j])
            {
                _gMutualICDriveR[j] = _DrvMpTestItoShortTestMsg26xxmCovertRValue(_gMutualICDeltaC[i]);
                DBG(&g_I2cClient->dev, "_gMutualICDriveR[%d] = %d\t", j , _gMutualICDriveR[j]);
            }
        }
    }
    DBG(&g_I2cClient->dev, "\n");
    
    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(4);
    }
    
    nRetVal4 = _DrvMpTestItoShortTestMsg26xxmJudge(4);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        if (_gMsg26xxmShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gMutualICDriveLineNum ; j++)
        {
            if (_gMsg26xxmShortTestChannel[i] == DRIVE_X[j])
            {
                _gMutualICDriveR[j] = _DrvMpTestItoShortTestMsg26xxmCovertRValue(_gMutualICDeltaC[i]);
                DBG(&g_I2cClient->dev, "_gMutualICDriveR[%d] = %d\t", j , _gMutualICDriveR[j]);
            }
        }
    }
    DBG(&g_I2cClient->dev, "\n");
    
    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i ++)
    {
        _gMsg26xxmShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;
    nShortThreshold = _DrvMpTestItoShortTestMsg26xxmCovertRValue(MSG26XXM_SHORT_VALUE);

    for (i = 0; i < _gMutualICSenseLineNum; i++)
    {
        _gMutualICResult[i] = _gMutualICSenseR[i];
    }

    for (i = 0; i < _gMutualICDriveLineNum; i++)
    {
        _gMutualICResult[i + _gMutualICSenseLineNum] = _gMutualICDriveR[i];
    }

    for (i = 0; i < (_gMutualICSenseLineNum + _gMutualICDriveLineNum); i++)
    {
        if (_gMutualICResult[i] < nShortThreshold)
        {
            _gMutualICTestFailChannel[i] = 1;
        }
        else
        {
            _gMutualICTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    DBG(&g_I2cClient->dev, "short test end\n");

    DBG(&g_I2cClient->dev, "nRetVal1 = %d, nRetVal2 = %d, nRetVal3 = %d, nRetVal4 = %d, nRetVal5 = %d\n",nRetVal1,nRetVal2,nRetVal3,nRetVal4,nRetVal5);

    if ((nRetVal1 != ITO_TEST_OK) && (nRetVal2 == ITO_TEST_OK) && (nRetVal3 == ITO_TEST_OK) && (nRetVal4 == ITO_TEST_OK) && (nRetVal5 == ITO_TEST_OK))
    {
        return ITO_TEST_GET_TP_TYPE_ERROR;
    }
    else if ((nRetVal1 == ITO_TEST_OK) && ((nRetVal2 != ITO_TEST_OK) || (nRetVal3 != ITO_TEST_OK) || (nRetVal4 != ITO_TEST_OK) || (nRetVal5 != ITO_TEST_OK)))
    {
        return -1;
    }
    else
    {
        return ITO_TEST_OK;
    }
}

static s32 _DrvMpTestItoTestMsg26xxmGetWaterProofOneShotRawIir(u16 wszResultData[])
{
    u16 nRegData;
    u16 i;
    u16 nTemp;
    u8 szDbBusTxData[3];
    u32 nGetdataNum = 12;
    u8 szShotData[24] = {0}; //Get 12 FIR data

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp &= (~(BIT8));
    RegSet16BitValue(0x3D08, nTemp);
    //RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK

    _DrvMpTestItoTestMsg26xxmEnableAdcOneShot();

    nRegData = 0;
    while (0x0000 == (nRegData & BIT8))
    {
        nRegData = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    }

    for (i = 0; i < nGetdataNum * 2; i ++)
    {
        szShotData[i] = 0;
    }

    mdelay(200);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x13; //bank:fir, addr:h0021
    szDbBusTxData[2] = 0x42;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData[0], 24); //12

    for (i = 0; i < nGetdataNum; i ++)
    {
        nRegData = (u16)(szShotData[i * 2] | szShotData[i * 2 + 1] << 8);
        wszResultData[i] = (short)nRegData;

		//DBG(&g_I2cClient->dev, "wszResultData[%d] = %x\t", i  , wszResultData[i]);
    }

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp |= (BIT8 | BIT4);
    RegSet16BitValue(0x3D08, nTemp);

    return 0;
}

static s32 _DrvMpTestItoTestMsg26xxmGetWaterProofDeltaC(s32 *pTarget)
{
    u16 wszRawData[12];
    u16 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_DrvMpTestItoTestMsg26xxmGetWaterProofOneShotRawIir(wszRawData) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg26xxm WaterProof Test# GetMutualOneShotRawIIR failed! ***\n");
        return -1;
    }

    for (i = 0; i < 12; i ++)
    {
        pTarget[i] = (s16)wszRawData[i];
    }

    return 0;
}

static s32 _DrvMpTestMsg26xxmItoWaterProofTest(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    //RegSet16BitValue(0x0FE6, 0x0001);

    _DrvMpTestItoTestMsg26xxmAnaSwReset();

    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(WATERPROOF_Charge_X, WATERPROOF_Dump_X);

    if(_DrvMpTestItoTestMsg26xxmGetWaterProofDeltaC(_gMutualICDeltaC) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg26xxm WaterProof Test# GetWaterDeltaC failed! ***\n");
        return -1;
    }

    return 0;
}

static s32 _DrvMpTestMsg26xxmItoWaterProofTestJudge(void)
{
    u16 i;
    u32 nGetdataNum = 12;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < nGetdataNum; i ++)
    {
        _gMutualICResultWater[i] = 0;
    }

    for (i = 0; i < nGetdataNum; i++)
    {
   		_gMutualICResultWater[i] = _gMutualICDeltaC[i];
    }

    return 0;
}

s32 _DrvMpTestMsg26xxmItoWaterProofTestEntry(void)
{
    s32 nRetVal = 0;
    u32 nRegData = 0;
    u16 i = 0;
    s32 aResultTemp[12] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gMutualICWaterProofNum = 12;

    DrvPlatformLyrDisableFingerTouchReport();

    DrvPlatformLyrTouchDeviceResetHw();

	  //auto detetc fw version to decide switch fw mode
    if (_gMutualICTestAutoSwitchFlag != 0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if (_DrvMpTestItoTestMsg26xxmSwitchMode(_gMutualICTestSwitchMode, WATERPROOF) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg26xxm WaterProof Test# Switch FW mode failed! ***\n");
        return -1;
    }

    _DrvMpTestItoTestMsg26xxmMcuStop();

    nRegData = RegGet16BitValue(0x3CE4);

    if (nRegData == 0x8bbd)//// if polling 0X8BBD, means the FW is NOT supporting WP.
    {
        DBG(&g_I2cClient->dev, "*** Msg26xxm WaterProof Test# No supporting this function! ***\n");
        return -1;
    }

    mdelay(10);

    for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i ++)
    {
        _gMutualICTestFailChannel[i] = 0;
    }

    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start

  	if(_DrvMpTestMsg26xxmItoWaterProofTest() < 0)
  	{
        DBG(&g_I2cClient->dev, "*** Msg26xxm WaterProof Test# Get DeltaC failed! ***\n");
        return -1;
  	}

    _DrvMpTestMsg26xxmItoWaterProofTestJudge();

    for (i = 0; i < 12; i++)
    {
        aResultTemp[i] = _gMutualICResultWater[i];
    }

    _DrvMpTestMutualICDebugShowArray(_gMutualICResultWater, 12, -32, 10, 8);
    for (i = 0; i < 12; i++)
    {
        if (aResultTemp[i] < WATERPROOFVALUE)    //change comparison way because float computing in driver is prohibited
        {
            _gMutualICTestFailChannel[i] = 1;
            _gTestFailChannelCount++;
            nRetVal = -1;
        }
        else
        {
            _gMutualICTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static void _DrvMpTestItoOpenTestMsg28xxSetMutualCsubViaDBbus(s16 nCSub)
{   
    u8 nBaseLen = 6;
    u16 nFilter = 0x3F;
    u16 nLastFilter = 0xFFF;
    u8 nBasePattern = nCSub & nFilter; 
    u8 nPattern;
    u16 n16BitsPattern;
    u16 nCSub16Bits[5] = {0};
    int i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for(i=0; i<5; i++)
    {
        if(i == 0)
        {
            nPattern = nBasePattern;    //Patn => Pattern
        }
         
        n16BitsPattern = ((nPattern & 0xF) << nBaseLen*2) | (nPattern << nBaseLen) | nPattern;

        if(i == 4)
        {
            nCSub16Bits[i] = n16BitsPattern & nLastFilter;
        }
        else
        {
            nCSub16Bits[i] = n16BitsPattern;
        }
        nPattern = (u8)((n16BitsPattern >> 4) & nFilter);
    }    
        
    RegSet16BitValue(0x215C, 0x1FFF);

    for (i = 0; i < 5; i++)
    {
        RegSet16BitValue(0x2148 + 2 * i, nCSub16Bits[i]);
        RegSet16BitValue(0x2152 + 2 * i, nCSub16Bits[i]);
    }     
}

/*
static void _DrvMpTestItoOpenTestMsg28xxAFEGainOne(void)
{
    u8 nRegData = 0;
    u16 nAFECoef = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // AFE gain = 1X
    RegSet16BitValue(0x1318, 0x4440);
    RegSet16BitValue(0x131A, 0x4444);
    RegSet16BitValue(0x13D6, 0x2000);
    
    RegSet16BitValue(0x2160, 0x0040);
    RegSet16BitValue(0x2162, 0x0040);
    RegSet16BitValue(0x2164, 0x0040);
    RegSet16BitValue(0x2166, 0x0040);
    RegSet16BitValue(0x2168, 0x0040);
    RegSet16BitValue(0x216A, 0x0040);
    RegSet16BitValue(0x216C, 0x0040);
    RegSet16BitValue(0x216E, 0x0040);
    RegSet16BitValue(0x2170, 0x0040);
    RegSet16BitValue(0x2172, 0x0040);
    RegSet16BitValue(0x2174, 0x0040);
    RegSet16BitValue(0x2176, 0x0040);
    RegSet16BitValue(0x2178, 0x0040);
    RegSet16BitValue(0x217A, 0x1FFF);
    RegSet16BitValue(0x217C, 0x1FFF);
    
    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);
    
    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)
    
    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);

    //AFE coef
    nRegData = RegGetLByteValue(0x101A);
    nAFECoef = (u16)(0x10000/nRegData);
    RegSet16BitValue(0x13D6, nAFECoef);        
}
*/

static void _DrvMpTestItoOpenTestMsg28xxAFEGainOne(void)
{
    // AFE gain = 1X
    u16 nAfeGain = 0;
    u16 nDriOpening = 0;
    u8 nRegData = 0;
    u16 nAfeCoef = 0;
    u16 i = 0;

    //proto.MstarReadReg(loopDevice, (uint)0x1312, ref regdata); //get dri num
    nRegData = RegGetLByteValue(0x1312);    
    nDriOpening = nRegData;

    ///filter unit gain
    if (nDriOpening == 11 || nDriOpening == 15)
    {
        RegSet16BitValue(0x1318, 0x4470);
    }
    else if (nDriOpening == 7)
    {
        RegSet16BitValue(0x1318, 0x4460);
    }

    //proto.MstarWriteReg_16(loopDevice, 0x131A, 0x4444);
    RegSet16BitValue(0x131A, 0x4444);

    ///AFE coef
    //proto.MstarReadReg(loopDevice, (uint)0x101A, ref regdata);
    nRegData = RegGetLByteValue(0x101A); 
    nAfeCoef = 0x10000 / nRegData;
    //proto.MstarWriteReg_16(loopDevice, (uint)0x13D6, AFE_coef);
    RegSet16BitValue(0x13D6, nAfeCoef);

    ///AFE gain
    if (nDriOpening == 7 || nDriOpening == 15)
    {
        nAfeGain = 0x0040;
    }    
    else if (nDriOpening == 11)
    {
        nAfeGain = 0x0055;
    }
    
    for (i = 0; i < 13; i++)
    {
        RegSet16BitValue(0x2160 + 2 * i, nAfeGain);
    }

    ///AFE gain: over write enable
    RegSet16BitValue(0x217A, 0x1FFF);
    RegSet16BitValue(0x217C, 0x1FFF);

    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    if ((_gMsg28xx_PATTERN_TYPE == 5) && (_gMsg28xx_PATTERN_MODEL == 1))
    {
        switch (_gMsg28xx_CFB_REF)
        {
             case 0:
             case 2:
                 SetCfb(_20p);
                 break;
             case 1:
                 SetCfb(_10p);
                 break;
             case 3:
                 SetCfb(_30p);
                 break;
             case 4:
                 SetCfb(_40p);
                 break;
             case 5:
                 SetCfb(_50p);
                 break;
        }

        DBG(&g_I2cClient->dev,"SetCfb : %d", _gMsg28xx_CFB_REF);
    }
    else
    {
        RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)
    }

    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);

    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);
}

static void _DrvMpTestItoOpenTestMsg28xxCalibrateMutualCsub(s16 nCSub)
{   
    u8 nChipVer;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nChipVer = RegGetLByteValue(0x1ECE);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Chip ID = %d ***\n", nChipVer);    
    
    if (nChipVer != 0)
        RegSet16BitValue(0x10F0, 0x0004);//bit2
    
    _DrvMpTestItoOpenTestMsg28xxSetMutualCsubViaDBbus(nCSub);
    _DrvMpTestItoOpenTestMsg28xxAFEGainOne();    
}

static void _DrvMpTestItoTestDBBusReadDQMemStart(void)
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

/*
static void _DrvMpTestItoTestDBBusReadDQMemStartAddr24(void)
{
    u8 nParCmdSelUseCfg = 0x7F;
//    u8 nParCmdAdByteEn0 = 0x50;
//    u8 nParCmdAdByteEn1 = 0x51;
    u8 nParCmdAdByteEn2 = 0x52;    
//    u8 nParCmdDaByteEn0 = 0x54;
    u8 nParCmdUSetSelB0 = 0x80;
    u8 nParCmdUSetSelB1 = 0x82;
    u8 nParCmdSetSelB2  = 0x85;
    u8 nParCmdIicUse    = 0x35;
    //u8 nParCmdWr        = 0x10;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn2, 1);    
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1); 
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2,  1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse,    1);        
}
*/

static void _DrvMpTestItoTestDBBusReadDQMemEnd(void)
{
    u8 nParCmdNSelUseCfg = 0x7E;    
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1);
}

/*
static void _DrvMpTestItoTestDBBusReadDQMemEndAddr24(void)
{
    u8 nParCmdSelUseCfg  = 0x7F;
    u8 nParCmdAdByteEn1  = 0x51;
    u8 nParCmdSetSelB0   = 0x81;
    u8 nParCmdSetSelB1   = 0x83;    
    u8 nParCmdNSetSelB2  = 0x84;
    u8 nParCmdIicUse     = 0x35;
    u8 nParCmdNSelUseCfg = 0x7E;
    u8 nParCmdNIicUse    = 0x34;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    RegSetLByteValue(0, 0);       

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB1, 1);    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSetSelB2, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse, 1);        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1); 
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNIicUse,  1);
}
*/

static void _DrvMpTestItoTestMsg28xxEnableAdcOneShot(void)
{
    RegSet16BitValueOn(0x100a, BIT0);

    return;
}

static s32 _DrvMpTestItoTestMsg28xxTriggerMutualOneShot(s16 * pResultData, u16 * pSenNum, u16 * pDrvNum)
{    
    u16 nAddr = 0x5000, nAddrNextSF = 0x1A4;
    u16 nSF = 0, nAfeOpening = 0, nDriOpening = 0;
    u16 nMaxDataNumOfOneSF = 0;
    u16 nDriMode = 0;
    int nDataShift = -1;
    u16 i, j, k;    
    u8 nRegData = 0;
    u8 aShotData[392] = {0};//13*15*2
    u16 nRegDataU16 = 0;
    s16 *pShotDataAll = NULL;
    u8 nParCmdIicUse = 0x35;
    u16 nRegData2 = 0, nSwcap = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse, 1);        
    nRegData = RegGetLByteValue(0x130A);
    nSF = nRegData>>4;
    nAfeOpening = nRegData & 0x0f;
    
    if (nSF == 0)
    {
        return -1;
    }
    
    nRegData = RegGetLByteValue(0x100B);
    nDriMode = nRegData;
    
    nRegData = RegGetLByteValue(0x1312);
    nDriOpening = nRegData;

    DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# TriggerMutualOneShot nSF=%d, nAfeOpening=%d, nDriMode=%d, nDriOpening=%d. ***\n", nSF, nAfeOpening, nDriMode, nDriOpening);
    
    nMaxDataNumOfOneSF = nAfeOpening * nDriOpening;
    
    pShotDataAll = kzalloc(sizeof(s16) * nSF * nMaxDataNumOfOneSF, GFP_KERNEL);

    RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        if (nDriMode == 2)  // for short test
        {
            _DrvMpTestItoTestMsg28xxEnableAdcOneShot();
        }
        else
        {
            nRegData2 = RegGet16BitValueByAddressMode(0x1301, ADDRESS_MODE_16BIT);
            nSwcap = (nRegData2 & 0x0800);
            if (nSwcap)
            {
                _DrvMpTestItoTestMsg28xxEnableAdcOneShot();
            }
            else
            { // sine mode
                RegMask16BitValue(0x1E2A, BIT0, 0x0000, ADDRESS_MODE_16BIT);
                RegMask16BitValue(0x1110, BIT1, BIT1, ADDRESS_MODE_16BIT);
                RegMask16BitValue(0x1E2A, BIT0, BIT0, ADDRESS_MODE_16BIT);
            }
        }
    }
    else
    {
        _DrvMpTestItoTestMsg28xxEnableAdcOneShot();
    }

    //polling frame-ready interrupt status
    while (0x0000 == (nRegDataU16 & BIT8))
    {
        nRegDataU16 = RegGet16BitValue(0x3D18);
    }
    
    if (nDriMode == 2) // for short test
    {
        if (nAfeOpening % 2 == 0)
            nDataShift = -1;
        else
            nDataShift = 0;    //special case    
        //s16 nShortResultData[nSF][nAfeOpening];
        
        /// get ALL raw data
        for (i = 0; i < nSF; i++)
        {
            memset(aShotData, 0, sizeof(aShotData));
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, aShotData, 28, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();

            //_DrvMpTestMutualICDebugShowArray(aShotData, 26, 8, 16, 16);   
            if (_gMsg28xx_PATTERN_TYPE == 5)
            {
                for (j = 0; j < nAfeOpening; j ++)
                {
                    pResultData[i*nAfeOpening+j] = (s16)(aShotData[2 * j] | aShotData[2 * j + 1] << 8);

                    if (nDataShift == 0 && (j == nAfeOpening-1))
                    {
                        pResultData[i*nAfeOpening+j] = (s16)(aShotData[2 * (j + 1)] | aShotData[2 * (j + 1) + 1] << 8);
                    }
                }
            }
            else
            {
                for (j = 0; j < nAfeOpening; j ++)
                {
                    pResultData[i*MUTUAL_IC_MAX_CHANNEL_DRV+j] = (s16)(aShotData[2 * j] | aShotData[2 * j + 1] << 8);

                    if (nDataShift == 0 && (j == nAfeOpening-1))
                    {
                        pResultData[i*MUTUAL_IC_MAX_CHANNEL_DRV+j] = (s16)(aShotData[2 * (j + 1)] | aShotData[2 * (j + 1) + 1] << 8);                
                    }
                }
            }
        }

        *pSenNum = nSF;
        *pDrvNum = nAfeOpening;         
    }
    else // for open test
    {
        //s16 nOpenResultData[nSF * nAfeOpening][nDriOpening];
    
        if (nAfeOpening % 2 == 0 || nDriOpening % 2 == 0)
            nDataShift = -1;
        else
            nDataShift = 0;    //special case

        /// get ALL raw data, combine and handle datashift.
        for (i = 0; i < nSF; i ++)
        {        
            memset(aShotData, 0, sizeof(aShotData));
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, aShotData, 392, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();
          
            //_DrvMpTestMutualICDebugShowArray(aShotData, 390, 8, 10, 16);  
            for (j = 0; j < nMaxDataNumOfOneSF; j ++)
            {
                DBG(&g_I2cClient->dev, "*** i : %d, j : %d ***\n", i, j);
                pShotDataAll[i*nMaxDataNumOfOneSF+j] = (s16)(aShotData[2 * j] | aShotData[2 * j + 1] << 8);
                DBG(&g_I2cClient->dev, "*** _DrvMpTestItoTestMsg28xxTriggerMutualOneShot_1 : %02X, %02X, %04X ***\n", aShotData[2 * j + 1], aShotData[2 * j], pShotDataAll[i*nMaxDataNumOfOneSF+j]);

                if (nDataShift == 0 && j == (nMaxDataNumOfOneSF - 1))
                {
                    pShotDataAll[i*nMaxDataNumOfOneSF+j] = (s16)(aShotData[2 * (j + 1)] | aShotData[2 * (j + 1) + 1] << 8);
                    DBG(&g_I2cClient->dev, "*** _DrvMpTestItoTestMsg28xxTriggerMutualOneShot_2 : %02X, %02X, %04X ***\n", aShotData[2 * (j + 1) + 1], aShotData[2 * (j + 1)], pShotDataAll[i*nMaxDataNumOfOneSF+j]);
                }
            }
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# aShotData ***\n");
            _DrvMpTestMutualICDebugShowArray(aShotData, _gMutualICSenseLineNum * _gMutualICDriveLineNum, 8, 16, _gMutualICDriveLineNum);
        }

        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# pShotDataAll ***\n");
        _DrvMpTestMutualICDebugShowArray(pShotDataAll, _gMutualICSenseLineNum * _gMutualICDriveLineNum, -16, 16, _gMutualICDriveLineNum);

        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# pShotDataAll ***\n");
        _DrvMpTestMutualICDebugShowArray(pShotDataAll, _gMutualICSenseLineNum * _gMutualICDriveLineNum, -16, 10, _gMutualICDriveLineNum);

        //problem here
        for (k = 0; k < nSF; k++)
        {
            for (i = k * nAfeOpening; i < nAfeOpening * (k + 1); i++) //Sen
            {
                for (j = 0; j < nDriOpening; j ++) //Dri
                {
                    pResultData[i*MUTUAL_IC_MAX_CHANNEL_DRV+j] = pShotDataAll[k*nMaxDataNumOfOneSF + (j + (i - nAfeOpening * k) * nDriOpening)]; //resultData[Sen, Dri]
                }
            }
        }

        *pSenNum = nSF * nAfeOpening;
        *pDrvNum = nDriOpening;        
    }
    RegSet16BitValueOn(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK
    RegSet16BitValueOn(0x3D08, BIT4);      ///FIQ_E_TIMER0_MASK

    kfree(pShotDataAll);

    return 0;
}

static s32 _DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(s16 * nResultData, u16 * pSenNum, u16 * pDrvNum)
{
    return _DrvMpTestItoTestMsg28xxTriggerMutualOneShot(nResultData, pSenNum, pDrvNum); 
}

static s32 _DrvMpTestItoTestMsg28xxGetDeltaC(s32 *pDeltaC)
{        
    s16 * pRawData = NULL;
    s16 aRawDataOverlapDone[_gMsg28xx_SENSE_NUM][_gMsg28xx_DRIVE_NUM];
    u16 nDrvPos = 0, nSenPos = 0, nShift = 0;    
    u16 nSenNumBak = 0;
    u16 nDrvNumBak = 0;    
    s16 i, j;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    pRawData = kzalloc(sizeof(s16) * MUTUAL_IC_MAX_CHANNEL_SEN*2 * MUTUAL_IC_MAX_CHANNEL_DRV, GFP_KERNEL);

    if(_DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(pRawData, &nSenNumBak, &nDrvNumBak) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetMutualOneShotRawIIR failed! ***\n");      
        return -1;
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# nSenNumBak=%d nDrvNumBak=%d ***\n", nSenNumBak, nDrvNumBak); 

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# pRawData ***\n");
    _DrvMpTestMutualICDebugShowArray(pRawData, _gMutualICSenseLineNum * _gMutualICDriveLineNum, -16, 10, _gMutualICSenseLineNum);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        for (i = 0; i < _gMsg28xx_SENSE_NUM; i ++)
        {
            for (j = 0; j < _gMsg28xx_DRIVE_NUM; j ++)
            {
                aRawDataOverlapDone[i][j] = MSG28XX_UN_USE_SENSOR;
            }
        }
    }

    for (i = 0; i < nSenNumBak; i++)
    {
        for (j = 0; j < nDrvNumBak; j++)
        {
            nShift = (u16)(i * nDrvNumBak + j);
            
            if (_gMsg28xxTpType == TP_TYPE_HLT)
            {
                nDrvPos = g_MapVaMutual_X[nShift][1];            
                nSenPos = g_MapVaMutual_X[nShift][0];            
            }
	  /*
            else if (_gMsg28xxTpType == TP_TYPE_Y)
            {
                nDrvPos = g_MapVaMutual_Y[nShift][1];            
                nSenPos = g_MapVaMutual_Y[nShift][0];            
            }
		*/
            if (nDrvPos >= _gMutualICDriveLineNum || nSenPos >= _gMutualICSenseLineNum) continue;

//	            DBG(&g_I2cClient->dev, "*** nDrvPos=%d nSenPos=%d ***\n", nDrvPos, nSenPos);
            if (nDrvPos != 0xFF && nSenPos != 0xFF)
            {
                aRawDataOverlapDone[nSenPos][nDrvPos] = pRawData[i*MUTUAL_IC_MAX_CHANNEL_DRV+j];
            }
        }
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# aRawDataOverlapDone ***\n");
    _DrvMpTestMutualICDebugShowArray(aRawDataOverlapDone, _gMutualICSenseLineNum * _gMutualICDriveLineNum, -16, 10, _gMutualICSenseLineNum);

    for (i = 0; i < _gMutualICSenseLineNum; i++)
    {
        for (j = 0; j < _gMutualICDriveLineNum; j++)
        {
            nShift = (u16)(i * _gMutualICDriveLineNum + j);
            pDeltaC[nShift] = (s32)aRawDataOverlapDone[i][j];
        }
    }
  
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# gDeltaC ***\n");
    _DrvMpTestMutualICDebugShowArray(pDeltaC, _gMutualICSenseLineNum * _gMutualICDriveLineNum, -32, 10, _gMutualICSenseLineNum);  

    kfree(pRawData);

    return 0;
}

static void _DrvMpTestItoTestMsg28xxAnaSwReset(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    /// reset ANA
    RegSet16BitValueOn(0x1002, (BIT0 | BIT1 | BIT2 | BIT3));     ///reg_tgen_soft_rst: 1 to reset
    RegSet16BitValueOff(0x1002, (BIT0 | BIT1 | BIT2 | BIT3));
    
    /// delay
    mdelay(20);
}

static void _DrvMpTestItoTestMsg28xxAnaChangeCDtime(u16 nTime1, u16 nTime2)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x1026, nTime1);
    RegSet16BitValue(0x1030, nTime2);
}

static void _DrvMpTestItoOpenTestMsg28xxOnCellAnaEnableChargePump(u8 enable_charge_pump)
{
    u16 val = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (!enable_charge_pump)
    {
        RegSet16BitValueOff(0x1560, BIT0 | BIT1);	//high v buf en, close
        val = RegGet16BitValue(0x1566); //set
        val |= BIT12;
        RegSet16BitValue(0x1566, val);        //LV on
        RegSet16BitValueOff(0x1566, BIT8 | BIT9 | BIT10 | BIT11);
        RegSet16BitValueOff(0x1570, BIT8 | BIT10);
        RegSet16BitValueOff(0x1580, BIT0 | BIT4);

        val = RegGet16BitValue(0x1464); //set
        val |= BIT2 | BIT3;
        RegSet16BitValue(0x1464, val);
    }
    else
    {
        val = RegGet16BitValue(0x1560); //set
        val |= 0x03;
        RegSet16BitValue(0x1560, val); //high v buf en, close
        RegSet16BitValueOff(0x1566, BIT12);        //LV on
        RegSet16BitValueOff(0x1566, BIT8 | BIT9 | BIT10 | BIT11);        //LV on

        val = RegGet16BitValue(0x1566); //set
        val |= BIT8 | BIT10;
        RegSet16BitValue(0x1566, val);

        val = RegGet16BitValue(0x1570); //cset
        val |= BIT10;
        RegSet16BitValue(0x1570, val);

        val = RegGet16BitValue(0x1580); //set
        val |= BIT0 | BIT1;
        RegSet16BitValue(0x1580, val);
        RegSet16BitValueOff(0x1464, BIT2 | BIT3);
    }
}

static void _DrvMpTestItoOpenTestMsg28xxOnCellUpdateAnaChargeDumpSetting(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValueByAddressMode(0x1018, 0x001F, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1019, 0x003f, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x101a, 0x0028, ADDRESS_MODE_16BIT);  //post idle time in sub0
    RegSet16BitValueByAddressMode(0x101b, 0x0028, ADDRESS_MODE_16BIT);  //post idle time in sub1
    RegSet16BitValueByAddressMode(0x101c, 0x0028, ADDRESS_MODE_16BIT);  //post idle time in sub2
    RegSet16BitValueByAddressMode(0x101d, 0x0028, ADDRESS_MODE_16BIT);  //post idle time in sub3
    RegSet16BitValueByAddressMode(0x101e, 0x0028, ADDRESS_MODE_16BIT);  //post idle time in sub4
    RegSet16BitValueByAddressMode(0x101f, 0x0000, ADDRESS_MODE_16BIT);

    RegSet16BitValueByAddressMode(0x100d, 0x0020, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1103, 0x0020, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1104, 0x0020, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1302, 0x0020, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x136b, 0x10000 / 0x20, ADDRESS_MODE_16BIT);    // AFE_coef, set value by 0x10000 dividing register 0x100d value
    RegSet16BitValueByAddressMode(0x1b30, 0x0020, ADDRESS_MODE_16BIT);
}

static void _DrvMpTestItoOpenTestMsg28xxOpenSineModeSetting(void)
{
    _DrvMpTestItoOpenTestMsg28xxOnCellAnaEnableChargePump(_gMsg28xx_CHARGE_PUMP);

    switch (_gMsg28xx_CFB_REF) {
        case 0:
        case 2:
        	SetCfb(_20p);
            break;
        case 1:
            SetCfb(_10p);
            break;
        case 3:
            SetCfb(_30p);
            break;
        case 4:
            SetCfb(_40p);
            break;
        case 5:
            SetCfb(_50p);
            break;
    }
    DBG(&g_I2cClient->dev, "SetCfb : %d", _gMsg28xx_CFB_REF);
}

static s32 _DrvMpTestItoOpenTestMsg28xxObtainOpenValueKeys(s32 *pkeyarray)
{
    u16 k;
    u32 nShift = 0;

    SetCfb(_50p);
    _DrvMpTestItoOpenTestMsg28xxOnCellAnaEnableChargePump(DISABLE);
    if ( _DrvMpTestItoTestMsg28xxGetDeltaC(_gMutualICDeltaC) < 0 )
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetDeltaC failed! ***\n");
        return -1;
    }

    for (k = 0; k < _gMsg28xx_KEY_NUM; k++)
    {
        nShift = (_gMsg28xx_KEYSEN[k] - 1) * _gMsg28xx_KEYDRV_O + _gMsg28xx_KEYDRV_O - 1;
        pkeyarray[k] = _gMutualICDeltaC[nShift];
    }

    return 0;
}

static void _DrvMpTestItoOpenTestMsg28xxDBBusReEnter(void)
{
    u8 nParCmdSelUseCfg = 0x7F;
    u8 nParCmdAdByteEn1 = 0x51;
    u8 nPar_N_CmdUSetSelB0 = 0x80;
    u8 nPar_N_CmdUSetSelB1 = 0x82;
    u8 nPar_N_CmdSetSelB2  = 0x84;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nPar_N_CmdUSetSelB0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nPar_N_CmdUSetSelB1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nPar_N_CmdSetSelB2, 1);
}

static s32 _DrvMpTestItoTestMsg28xxCheckSwitchStatus(void)
{
    u32 nRegData = 0;
    int nTimeOut = 280;
    int nT = 0;

    do
    {
        nRegData = RegGet16BitValue(0x1402);
        mdelay(20);
        nT++;
        if (nT > nTimeOut)
        {
            return -1;
        }

    } while (nRegData != 0x7447);

    return 0;
}

static s32 _DrvMpTestItoOpenTestMsg28xxReEnterMutualMode(u8 nFMode)
{
    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    _DrvMpTestItoOpenTestMsg28xxDBBusReEnter();

    mdelay(50);

    switch (nFMode)
    {
        case MUTUAL_MODE:
            RegSet16BitValue(0x1402, 0x5705);
            break;
        case MUTUAL_SINE:
            RegSet16BitValue(0x1402, 0x5706);
            break;
    }

    if (_DrvMpTestItoTestMsg28xxCheckSwitchStatus()<0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# CheckSwitchStatus failed! ***\n");
        return -1;
    }

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);
    RegSet16BitValue(0x3D08, 0xFEFF);//open timer

    return 0;
}

void _DrvMpTestItoOpenTestMsg28xxOpenSwcapModeSetting(void)
{
    u16 nChargeT = 0x0C, nDumpT = 0x06;

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    _DrvMpTestItoOpenTestMsg28xxCalibrateMutualCsub(_gMsg28xx_CSUB_REF);
    _DrvMpTestItoTestMsg28xxAnaChangeCDtime(nChargeT, nDumpT);
    _DrvMpTestItoOpenTestMsg28xxOnCellAnaEnableChargePump(DISABLE);
    RegSet16BitValue(0x156A, 0x000A); ///DAC com voltage
    _DrvMpTestItoOpenTestMsg28xxOnCellUpdateAnaChargeDumpSetting();
    _DrvMpTestItoTestMsg28xxAnaSwReset();
}

void _DrvMpTestItoTestMsg28xxSramEnterAccessMode(void)
{
    // change to R2 mode
    RegMask16BitValue(0x2149, BIT5, BIT5, ADDRESS_MODE_16BIT);
    // SRAM using MCU clock
    RegMask16BitValue(0x1E11, BIT13, BIT13, ADDRESS_MODE_16BIT);
}

void _DrvMpTestItoTestMsg28xxSramExitAccessMode(void)
{
    // change to R2 mode
    RegMask16BitValue(0x2149, BIT5, 0, ADDRESS_MODE_16BIT);
    // SRAM using MCU clock
    RegMask16BitValue(0x1E11, BIT13, 0, ADDRESS_MODE_16BIT);
}

void _DrvMpTestItoTestMsg28xxRegGet16BitByteValueBuf(u16 nAddr, u8 *pBuf, u16 nLen)
{
    u16 i;
    u8 aTxData[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    u8 *pRxData = (u8 *)kzalloc(nLen, sizeof(u8));

    IicWriteData(SLAVE_I2C_ID_DBBUS, &aTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, pRxData, nLen);

    for (i = 0; i < nLen; i++)
    {
        pBuf[i] = pRxData[i];
    }
    
    kfree(pRxData);
}

void _DrvMpTestItoTestMsg28xxWriteDQMem_8bit(u16 addr, u8 data)
{
    u8 aReadBuf[4] = {0};
    u16 nHigh16, nLow16 = 0;
    u16 read_addr = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    read_addr = addr - (addr % 4);

    _DrvMpTestItoTestMsg28xxRegGet16BitByteValueBuf(read_addr, aReadBuf, sizeof(aReadBuf) / sizeof(aReadBuf[0]));

    aReadBuf[addr % 4] = data;
    DBG(&g_I2cClient->dev, "read_buf:0x%02x 0x%02x 0x%02x  0x%02x", aReadBuf[0], aReadBuf[1], aReadBuf[2], aReadBuf[3]);
    nLow16 = (((u16)aReadBuf[1] << 8) | aReadBuf[0]);
    nHigh16 = (((u16)aReadBuf[3] << 8) | aReadBuf[2]);

    RegSet16BitValue(read_addr + 2, nLow16);
    RegSet16BitValue(read_addr, nHigh16);
}

s32 _DrvMpTestItoOpenTestMsg28xxOnCellOpenVaValue(void)
{
    int i, nIsf = 0;
    u8 aShotData[16];
    u16 nAddr = 0x6410;
    u16 nKeySen = _gMsg28xx_PAD_TABLE_DRIVE[_gMsg28xx_KEYDRV_O - 1];

    _DrvMpTestItoTestMsg28xxSramEnterAccessMode();
    for (nIsf = 0; nIsf < _gMsg28xx_SUB_FRAME; nIsf++)
    {
        memset(aShotData, 0, sizeof(aShotData) / sizeof(aShotData[0]));
        _DrvMpTestItoTestDBBusReadDQMemStart();
        //			RegGetXByteValue(u8ShotData, addr + 0x20 * isf, sizeof(u8ShotData) / sizeof(u8ShotData[0]));
        RegGetXBitValue(nAddr + 0x20 * nIsf, aShotData, sizeof(aShotData) / sizeof(aShotData[0]), MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        DBG(&g_I2cClient->dev, "*** %s() _ Before***\n", __func__);
        _DrvMpTestMutualICDebugShowArray(aShotData, sizeof(aShotData) / sizeof(aShotData[0]), 8, 16, 10);
        // Find key index, then replace key sensor by assign sensor 56(0x38).
        for (i = 0; i < (sizeof(aShotData) / sizeof(u8)); i++)
        {
            if (aShotData[i] == nKeySen)
            {
                _DrvMpTestItoTestMsg28xxWriteDQMem_8bit(nAddr + nIsf * 0x20 + i, 0x38);
            }
        }
        //			RegGetXByteValue(u8ShotData, addr + 0x20 * isf, sizeof(u8ShotData) / sizeof(u8ShotData[0]));
        RegGetXBitValue(nAddr + 0x20 * nIsf, aShotData, sizeof(aShotData) / sizeof(aShotData[0]), MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        DBG(&g_I2cClient->dev, "*** %s() _ After***\n", __func__);
        _DrvMpTestMutualICDebugShowArray(aShotData, sizeof(aShotData) / sizeof(aShotData[0]), 8, 16, 10);
        _DrvMpTestItoTestDBBusReadDQMemEnd();
    }
    _DrvMpTestItoTestMsg28xxSramExitAccessMode();
    _DrvMpTestItoTestMsg28xxAnaSwReset();
    if (_DrvMpTestItoTestMsg28xxGetDeltaC(_gMutualICDeltaC) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetDeltaC failed! ***\n");
        return -1;
    }
	return 0;
}

static s32 _DrvMpTestMsg28xxItoOpenTest(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    _DrvMpTestItoOpenTestMsg28xxCalibrateMutualCsub(_gMsg28xx_CSUB_REF);
    RegSet16BitValue(0x156A, 0x000A); ///DAC com voltage
    _DrvMpTestItoTestMsg28xxAnaSwReset();

    if (_DrvMpTestItoTestMsg28xxGetDeltaC(_gMutualICDeltaC) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetDeltaC failed! ***\n");
        return -1;
    }

    return 0;
}

static s32 _DrvMpTestMsg28xxOnCellItoOpenTest(u8 nFMode, u8 nVaMode)
{
    s32 k;
    u16 nShift = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    if (nFMode == MUTUAL_SINE)
    {
        _DrvMpTestItoOpenTestMsg28xxOpenSineModeSetting();
    }
    else
    {
        _DrvMpTestItoOpenTestMsg28xxOpenSwcapModeSetting();
    }

    if ((_gMsg28xx_PATTERN_TYPE == 5) && (_gMsg28xx_PATTERN_MODEL == 1) && (_gMsg28xx_KEY_NUM))
    {

        if (nVaMode == 0)
        {
            if (_DrvMpTestItoOpenTestMsg28xxObtainOpenValueKeys(_gKeyArray) < 0)
            {
                DBG(&g_I2cClient->dev, "*** Msg28xxObtainOpenValue_Keys failed ***\n");
                return -1;
            }
        }
        else if (nVaMode == 1)
        {
        	if (_DrvMpTestItoOpenTestMsg28xxOnCellOpenVaValue() < 0)
            {
                DBG(&g_I2cClient->dev, "*** ObtainOpenValue_VA failed ***\n");
                return -1;
            }

            for (k = 0; k < _gMsg28xx_KEY_NUM; k++)
            {
                nShift = (_gMsg28xx_KEYSEN[k] - 1) * _gMsg28xx_KEYDRV_O + _gMsg28xx_KEYDRV_O - 1;
                _gMutualICDeltaC[nShift] = _gKeyArray[k];
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** va_mode is ERROR!! ***\n");
            return -1;

        }
    }
    else
    {
    	if (_DrvMpTestItoTestMsg28xxGetDeltaC(_gMutualICDeltaC) < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetDeltaC failed! ***\n");
            return -1;
    	}
    }

    return 0;
}

static s32 _DrvMpTestItoOpenTestMsg28xxOpenJudge(u16 nItemID, s8 pNormalTestResult[][2], u16 pNormalTestResultCheck[][13]/*, u16 nDriOpening*/)
{
    s32 nRetVal = 0;
    u16 nCSub = _gMsg28xx_CSUB_REF;
    u16 nRowNum = 0, nColumnNum = 0;
    u32 nSum=0, nAvg=0, nDelta=0, nPrev=0;
    u16 i, j, k;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    for (i = 0; i < _gMutualICSenseLineNum * _gMutualICDriveLineNum; i++)
    {
        //deltaC_result[i] = deltaC[i];
        //_gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i] * 2 / (nDriOpening + 1);
        if (_gMutualICDeltaC[i] > 31000)
        {
            return -1; 
        }
        
        _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
    
        // For mutual key, last column if not be used, show number "one".
        if ((_gMsg28xx_MUTUAL_KEY == 1 || _gMsg28xx_MUTUAL_KEY == 2) && (_gMsg28xx_KEY_NUM != 0))
        {
            if ((_gMutualICSenseLineNum < _gMutualICDriveLineNum) && ((i + 1) % _gMutualICDriveLineNum == 0))
            {
                _gMutualICResult[i] = -32000;    
                for (k = 0; k < _gMsg28xx_KEY_NUM; k++)
                {
                    if ((i + 1) / _gMutualICDriveLineNum == _gMsg28xx_KEYSEN[k])
                    {
                        //_gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i] * 2 / (nDriOpening + 1);
                        _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                    }    
                }
            }
    
            if ((_gMutualICSenseLineNum > _gMutualICDriveLineNum) && (i > (_gMutualICSenseLineNum - 1) * _gMutualICDriveLineNum - 1))
            {
                _gMutualICResult[i] = -32000;        
                for (k = 0; k < _gMsg28xx_KEY_NUM; k++)
                {
                    if (((i + 1) - (_gMutualICSenseLineNum - 1) * _gMutualICDriveLineNum) == _gMsg28xx_KEYSEN[k])
                    {
                        //_gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i] * 2 / (nDriOpening + 1);
                        _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                    }    
                }
            }
        }
    }

    if (_gMsg28xx_KEY_NUM > 0)
    {
        if(_gMutualICDriveLineNum >= _gMutualICSenseLineNum)
        {
            nRowNum = _gMutualICDriveLineNum-1;
            nColumnNum = _gMutualICSenseLineNum;
        }
        else
        {
            nRowNum = _gMutualICDriveLineNum;
            nColumnNum = _gMutualICSenseLineNum-1;
        }
    }
    else
    {
        nRowNum = _gMutualICDriveLineNum;
        nColumnNum = _gMutualICSenseLineNum;
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICResult ***\n"); 
    //_DrvMpTestMutualICDebugShowArray(_gMutualICResult, nRowNum*nColumnNum, -32, 10, nColumnNum);
    for (j = 0; j < _gMutualICDriveLineNum; j ++)
    {
        for (i = 0; i < _gMutualICSenseLineNum; i ++)
        {
            DBG(&g_I2cClient->dev, "%d  ", _gMutualICResult[i * _gMutualICDriveLineNum + j]);
        }
        DBG(&g_I2cClient->dev, "\n");                
    } 

    for (j = 0; j < nRowNum; j ++)
    {
        nSum = 0;
        for (i = 0; i < nColumnNum; i++)
        {
             nSum = nSum + _gMutualICResult[i * _gMutualICDriveLineNum + j];                               
        } 
        
        nAvg = nSum / nColumnNum;             
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# OpenJudge average=%d ***\n", nAvg);        
        for (i = 0; i < nColumnNum; i ++)
        {
   	   	    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i * _gMutualICDriveLineNum + j], (s32)(nAvg + nAvg * MSG28XX_DC_RANGE/100), (s32)(nAvg - nAvg * MSG28XX_DC_RANGE/100)))
   	   	    {
                _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j] = 1;
                _gTestFailChannelCount ++; 
                nRetVal = -1;
            }
    
            if (i > 0)
            {
                nDelta = _gMutualICResult[i * _gMutualICDriveLineNum + j] > nPrev ? (_gMutualICResult[i * _gMutualICDriveLineNum + j] - nPrev) : (nPrev - _gMutualICResult[i * _gMutualICDriveLineNum + j]);
                if (nDelta > nPrev*MUTUAL_IC_FIR_RATIO/100)
                {
                    if (0 == _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j]) // for avoid _gTestFailChannelCount to be added twice
                    {
                        _gMutualICTestFailChannel[i * _gMutualICDriveLineNum + j] = 1;
                        _gTestFailChannelCount ++; 
                    }
                    nRetVal = -1;
                    DBG(&g_I2cClient->dev, "\nSense%d, Drive%d, MAX_Ratio = %d,%d\t", i, j, nDelta, nPrev);
                }
            }
            nPrev = _gMutualICResult[i * _gMutualICDriveLineNum + j];
        }
    }  

    //DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# OpenJudge _gTestFailChannelCount=%d ***\n", _gTestFailChannelCount);    
    return nRetVal;
}

static s32 _DrvMpTestItoOpenTestMsg28xxOnCellOpenJudge(u16 nItemID, s8 pNormalTestResult[], u16 pNormalTestResultCheck[]/*, u16 nDriOpening*/)
{
    s32 nRetVal = 0;
    u16 nCSub = _gMsg28xx_CSUB_REF;
    u16 nRowNum = 0, nColumnNum = 0;
    u16 i, j, k;
    s32 bg_per_csub = 0;
    u16 nCfb = 0;
    s32 ratioAvg1000 = 0, ratioAvgMax1000 = 0, ratioAvgMin1000 = 0, passCount = 0;
    s32 ratioAvgBorder1000 = 0, ratioAvgBorderMax1000 = 0, ratioAvgBorderMin1000 = 0, passCount1 = 0;
    s32 ratioAvgMove1000 = 0, ratioAvgBorderMove1000 = 0;

    memset(_gNormalTestFailCheckDeltac, 0xFFFF, MUTUAL_IC_MAX_MUTUAL_NUM);
    memset(_gNormalTestFailCheckRatio1000, 0xFFFF, MUTUAL_IC_MAX_MUTUAL_NUM);
    memset( _gMutualICOnCellOpenTestRatio1000,0,sizeof( _gMutualICOnCellOpenTestRatio1000));
    memset(_gMutualICOnCellOpenTestRatioBorder1000,0,sizeof(_gMutualICOnCellOpenTestRatioBorder1000));
    memset(_gMutualICOnCellOpenTestRatioMove1000,0,sizeof(_gMutualICOnCellOpenTestRatioMove1000));
    memset(_gMutualICOnCellOpenTestRatioBorderMove1000,0,sizeof(_gMutualICOnCellOpenTestRatioBorderMove1000));

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_gMsg28xx_OPEN_MODE == 1) // if open mode = 1 (sine mode), Csub must be zero.
    {
        nCSub = 0;
    }

    if (!_gMsg28xx_CFB_REF)
    {
        nCfb = 2;
    }
    else
    {
	    nCfb = _gMsg28xx_CFB_REF;
    }

    bg_per_csub = (int)8365 / nCfb;
    //bg_per_csub = (int)(2.4 * 1.17 * 32768 / (11 * nCfb));

    for (i = 0; i < _gMutualICSenseLineNum * _gMutualICDriveLineNum; i ++)
    {
        if (_gMutualICDeltaC[i] > 31000)
        {
            return -1;
        }

        if (_gMutualICDeltaC[i] != MSG28XX_UN_USE_SENSOR)
        {
            if ((_gMsg28xx_PATTERN_TYPE == 5) && (_gMsg28xx_PATTERN_MODEL == 1))
            {
                _gMutualICResult[i] = bg_per_csub * nCSub - _gMutualICDeltaC[i];
            }
            else
            {
                _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
            }
        }
        else
        {
            _gMutualICResult[i] = MSG28XX_NULL_DATA;
        }
        // For mutual key, last column if not be used, show number "one".
        if ((_gMsg28xx_MUTUAL_KEY == 1 || _gMsg28xx_MUTUAL_KEY == 2) && (_gMsg28xx_KEY_NUM != 0))
        {
		    if (_gMsg28xx_PATTERN_TYPE == 5)
            {
                if (_gMsg28xx_SENSOR_KEY_CH != _gMsg28xx_KEY_NUM)
                {
                    if (!((i + 1) % _gMutualICDriveLineNum))
                    {
                        _gMutualICResult[i] = MSG28XX_NULL_DATA;

                        for (k = 0; k < _gMsg28xx_KEY_NUM; k ++)
                        {
                            if ((i + 1) / _gMutualICDriveLineNum == _gMsg28xx_KEYSEN[k])
                            {
                                if (_gMsg28xx_PATTERN_MODEL == 1)
                                {
                                    _gMutualICResult[i] = bg_per_csub * nCSub - _gMutualICDeltaC[i];
                                }
                                else
                                {
                                    _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (i > ((_gMsg28xx_SENSE_NUM - 1) * _gMsg28xx_DRIVE_NUM - 1))
                    {
                        _gMutualICResult[i] = MSG28XX_NULL_DATA;

                        for (k = 0; k < _gMsg28xx_KEY_NUM; k ++)
                        {
                            if (((i + 1) - (_gMutualICSenseLineNum - 1) * _gMutualICDriveLineNum) == _gMsg28xx_KEYSEN[k])
                            {
                                if (_gMsg28xx_PATTERN_MODEL == 1)
                                {
                                    _gMutualICResult[i] = bg_per_csub * nCSub - _gMutualICDeltaC[i];
                                }
                                else
                                {
                                    _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if ((_gMutualICSenseLineNum < _gMutualICDriveLineNum) && ((i + 1) % _gMutualICDriveLineNum == 0))
                {
                    _gMutualICResult[i] = MSG28XX_NULL_DATA;
                    for (k = 0; k < _gMsg28xx_KEY_NUM; k ++)
                    {
                        if ((i + 1) / _gMutualICDriveLineNum == _gMsg28xx_KEYSEN[k])
                        {
                            _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                        }
                    }
                }

                if ((_gMutualICSenseLineNum > _gMutualICDriveLineNum) && (i > (_gMutualICSenseLineNum - 1) * _gMutualICDriveLineNum - 1))
                {
                    _gMutualICResult[i] = MSG28XX_NULL_DATA;
                    for (k = 0; k < _gMsg28xx_KEY_NUM; k ++)
                    {
                        if (((i + 1) - (_gMutualICSenseLineNum - 1) * _gMutualICDriveLineNum) == _gMsg28xx_KEYSEN[k])
                        {
                            _gMutualICResult[i] = 1673 * nCSub - _gMutualICDeltaC[i];
                        }
                    }
                }
            }
        }
    }

//    nRowNum = _gMutualICDriveLineNum;
//    nColumnNum = _gMutualICSenseLineNum;
    nRowNum = _gMutualICSenseLineNum;
    nColumnNum = _gMutualICDriveLineNum;

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICResult ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICResult, nRowNum*nColumnNum, -32, 10, nColumnNum);
    memcpy(_gMutualICOnCellOpenTestResultData,_gMutualICResult,sizeof( _gMutualICOnCellOpenTestResultData));
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICOnCellOpenTestGoldenChannel ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICOnCellOpenTestGoldenChannel, nRowNum*nColumnNum, -32, 10, nColumnNum);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICOnCellOpenTestGoldenChannelMax ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICOnCellOpenTestGoldenChannelMax, nRowNum*nColumnNum, -32, 10, nColumnNum);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICOnCellOpenTestGoldenChannelMin ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICOnCellOpenTestGoldenChannelMin, nRowNum*nColumnNum, -32, 10, nColumnNum);

   for (k = 0; k < (sizeof(_gMutualICDeltaC) / sizeof(_gMutualICDeltaC[0])); k++)
   {
        if (0 == _gMutualICOnCellOpenTestGoldenChannel[k])
        {
		    if (k == 0)
            {
			    pNormalTestResult[0] = 1;	// no golden sample
            }
		    break;
	    }

        if (_gMutualICResult[k] != MSG28XX_NULL_DATA)
	    {
             _gMutualICOnCellOpenTestRatio1000[k] = _gMutualICResult[k] * 1000 / _gMutualICOnCellOpenTestGoldenChannel[k];

		    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[k], _gMutualICOnCellOpenTestGoldenChannelMax[k], _gMutualICOnCellOpenTestGoldenChannelMin[k]))
		    {
		        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Golden FAIL: %d ***\n", k);
                DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# _gMutualICResult: %d ***\n", _gMutualICResult[k]);
                DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# _gMutualICOnCellOpenTestGoldenChannelMax: %d ***\n", _gMutualICOnCellOpenTestGoldenChannelMax[k]);
                DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# _gMutualICOnCellOpenTestGoldenChannelMin: %d ***\n", _gMutualICOnCellOpenTestGoldenChannelMin[k]);
                pNormalTestResult[0] = 1;
			    pNormalTestResultCheck[k] = (u16)(((k / _gMutualICDriveLineNum) + 1) * 100 + ((k % _gMutualICDriveLineNum) + 1));
            }
            else
            {
			    pNormalTestResultCheck[k] = MSG28XX_PIN_NO_ERROR;

                if ((_gMsg28xx_PATTERN_TYPE == 3) && (_gMsg28xx_KEY_NUM == 0) && ((k % _gMutualICDriveLineNum == 0 )||((k + 1) % _gMutualICDriveLineNum == 0)))
                {
                    ratioAvgBorder1000 +=  _gMutualICOnCellOpenTestRatio1000[k];
                    passCount1 += 1;
                }
                else if ((_gMsg28xx_PATTERN_TYPE == 3) && (_gMsg28xx_KEY_NUM != 0) && ((k % _gMutualICDriveLineNum == 0) || ((k + 2) % _gMutualICDriveLineNum == 0)))
                {
                    ratioAvgBorder1000 +=  _gMutualICOnCellOpenTestRatio1000[k];
                    passCount1 += 1;
                }
                else
                {
                    ratioAvg1000 +=  _gMutualICOnCellOpenTestRatio1000[k];
                    passCount += 1;
                }
            }
		}
        else
        {
            pNormalTestResultCheck[k] = MSG28XX_PIN_NO_ERROR;
        }

        _gNormalTestFailCheckDeltac[k] = pNormalTestResultCheck[k];

    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show  _gMutualICOnCellOpenTestRatio1000 ***\n");
    _DrvMpTestMutualICDebugShowArray( _gMutualICOnCellOpenTestRatio1000, nRowNum*nColumnNum, -32, 10, nColumnNum);
    ratioAvgMax1000 = (s32)(100000 + _gMsg28xx_DC_Ratio_1000 + _gMsg28xx_DC_Ratio_UP_1000) / 100;
    ratioAvgMin1000 = (s32)(100000 - _gMsg28xx_DC_Ratio_1000) / 100;
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# _gMsg28xx_DC_Ratio_1000: %d ***\n", _gMsg28xx_DC_Ratio_1000);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# ratioAvgMax1000: %d ***\n", ratioAvgMax1000);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# ratioAvgMin1000: %d ***\n", ratioAvgMin1000);
    ratioAvgBorderMax1000 = (s32)(100000 + _gMsg28xx_DC_Border_Ratio_1000 + _gMsg28xx_DC_Border_Ratio_UP_1000) / 100;
    ratioAvgBorderMin1000 = (s32)(100000 - _gMsg28xx_DC_Border_Ratio_1000) / 100;
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# _gMsg28xx_DC_Border_Ratio_1000: %d ***\n", _gMsg28xx_DC_Border_Ratio_1000);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# ratioAvgBorderMax1000: %d ***\n", ratioAvgBorderMax1000);
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# ratioAvgBorderMin1000: %d ***\n", ratioAvgBorderMin1000);

    if (passCount != 0)
    {
        if (passCount1 != 0)
        {
            ratioAvgBorderMove1000 = ratioAvgBorder1000 / passCount1;
            ratioAvgMove1000 = ratioAvg1000 / passCount;

            for (i = 0; i < sizeof( _gMutualICOnCellOpenTestRatio1000) / sizeof( _gMutualICOnCellOpenTestRatio1000[0]); i++)
            {
                if ((_gMsg28xx_KEY_NUM == 0) && ((i % _gMutualICDriveLineNum == 0) || ((i + 1) % _gMutualICDriveLineNum == 0)))
                {
                    _gMutualICOnCellOpenTestRatioMove1000[i] =  _gMutualICOnCellOpenTestRatio1000[i] - ratioAvgBorderMove1000 + 1;
                }
                else if ((_gMsg28xx_KEY_NUM != 0) && ((i % _gMutualICDriveLineNum == 0) || ((i + 2) % _gMutualICDriveLineNum == 0)))
                {
                    _gMutualICOnCellOpenTestRatioMove1000[i] =  _gMutualICOnCellOpenTestRatio1000[i] - ratioAvgBorderMove1000 + 1;
                }
                else
                {
                    _gMutualICOnCellOpenTestRatioMove1000[i] =  _gMutualICOnCellOpenTestRatio1000[i] - ratioAvgMove1000 + 1;
                }
            }
        }
        else
        {
            ratioAvgMove1000 = ratioAvg1000 / passCount;
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# ratioAvgMove1000: %d ***\n", ratioAvgMove1000);

            for (i = 0; i < sizeof( _gMutualICOnCellOpenTestRatio1000) / sizeof( _gMutualICOnCellOpenTestRatio1000[0]); i++)
            {
               _gMutualICOnCellOpenTestRatioMove1000[i] =  _gMutualICOnCellOpenTestRatio1000[i] - ratioAvgMove1000 + 1000;
            }
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Show _gMutualICOnCellOpenTestRatioMove1000 ***\n");
            _DrvMpTestMutualICDebugShowArray(_gMutualICOnCellOpenTestRatioMove1000, nRowNum*nColumnNum, -32, 10, nColumnNum);
            memcpy(_gMutualICOnCellOpenTestResultRatioData,_gMutualICOnCellOpenTestRatioMove1000,sizeof( _gMutualICOnCellOpenTestResultRatioData));
        }
    }
    else
    {
        memcpy( _gMutualICOnCellOpenTestRatio1000,_gMutualICOnCellOpenTestRatioMove1000,sizeof( _gMutualICOnCellOpenTestRatio1000));
    }

    for (j = 0; j < (sizeof(_gMutualICDeltaC) / sizeof(_gMutualICDeltaC[0])); j++)
    {
        if (0 == _gMutualICOnCellOpenTestGoldenChannel[j])
        {
            if (j == 0)
            {
                pNormalTestResult[1] = 1;	// no golden sample
            }
            break;
        }

        if (MSG28XX_PIN_NO_ERROR == pNormalTestResultCheck[j])
        {

            if (_gMutualICResult[j] != MSG28XX_NULL_DATA)
            {
                if ((_gMsg28xx_PATTERN_TYPE == 3) && (_gMsg28xx_KEY_NUM == 0) && ((j % _gMutualICDriveLineNum == 0) || ((j + 1) % _gMutualICDriveLineNum == 0)))
                {
                    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICOnCellOpenTestRatioMove1000[j], ratioAvgBorderMax1000, ratioAvgBorderMin1000))
                    {
                        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# FAIL: %d ***\n", j);
                        pNormalTestResult[1] = 1;
                        pNormalTestResultCheck[j] = (u16)(((j / _gMutualICDriveLineNum) + 1) * 100 + ((j % _gMutualICDriveLineNum) + 1));
                    }
                    else
                    {
                        pNormalTestResultCheck[j] = MSG28XX_PIN_NO_ERROR;
                    }
                }
                else if ((_gMsg28xx_PATTERN_TYPE == 3) && (_gMsg28xx_KEY_NUM != 0) && ((j % _gMutualICDriveLineNum == 0) || ((j + 2) % _gMutualICDriveLineNum == 0)))
                {
                    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICOnCellOpenTestRatioMove1000[j], ratioAvgBorderMax1000, ratioAvgBorderMin1000))
                    {
                        pNormalTestResult[1] = 1;
                        pNormalTestResultCheck[j] = (u16)(((j / _gMutualICDriveLineNum) + 1) * 100 + ((j % _gMutualICDriveLineNum) + 1));
                    }
                    else
                    {
                        pNormalTestResultCheck[j] = MSG28XX_PIN_NO_ERROR;
                    }
                }
                else
                {
                    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICOnCellOpenTestRatioMove1000[j], ratioAvgMax1000, ratioAvgMin1000))
                    {
                        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Ratio FAIL: %d ***\n", j);
                        pNormalTestResult[1] = 1;
                        pNormalTestResultCheck[j] = (u16)(((j / _gMutualICDriveLineNum) + 1) * 100 + ((j % _gMutualICDriveLineNum) + 1));
                        DBG(&g_I2cClient->dev, "*** _gMutualICDriveLineNum: %d ***\n", _gMutualICDriveLineNum);
                    }
                    else
                    {
                        pNormalTestResultCheck[j] = MSG28XX_PIN_NO_ERROR;
                    }
                }
            }
            else
            {
                _gNormalTestFailCheckRatio1000[j] = MSG28XX_PIN_NO_ERROR;
            }
        }
        else
        {
            _gNormalTestFailCheckRatio[j] = pNormalTestResultCheck[j];
            continue;
        }

        _gNormalTestFailCheckRatio[j] = pNormalTestResultCheck[j];
    }

    for (k = 0; k < MUTUAL_IC_MAX_MUTUAL_NUM; k++)
    {
        if (0 == _gMutualICOnCellOpenTestGoldenChannel[k])
        {
            pNormalTestResultCheck[k] = MSG28XX_PIN_NO_ERROR;
            _gNormalTestFailCheckDeltac[k] = MSG28XX_PIN_NO_ERROR;
            _gNormalTestFailCheckRatio[k] = MSG28XX_PIN_NO_ERROR;
        }
        else
        {
            continue;
        }
    }

    if ((pNormalTestResult[0] != 0) || (pNormalTestResult[1] != 0))
        nRetVal = -1;

    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# pNormalTestResult[0]: %d ***\n", pNormalTestResult[0]);
    _gMutualICOnCellOpenTestResult[0] = pNormalTestResult[0];
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# pNormalTestResult[1]: %d ***\n", pNormalTestResult[1]);
    _gMutualICOnCellOpenTestResult[1] = pNormalTestResult[1];

    return nRetVal;
}

static s32 _DrvMpTestMsg28xxItoTestSwitchFwMode(u8 *pFMode)
{
    u8 nFreq = _gMsg28xx_FIXED_CARRIER, nRegData;
    u8 aCmd[3] = {0x0B, 0x01, 0x00};
    u32 nFreqData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001);    //MCU_stop
    mdelay(100);
    RegSet16BitValue(0X3C60, 0xAA55);    // disable watch dog

    RegSet16BitValue(0X3D08, 0xFFFF);
    RegSet16BitValue(0X3D18, 0xFFFF);

    RegSet16BitValue(0x1402, 0x7474);

    RegSet16BitValue(0x1E06, 0x0000);
    RegSet16BitValue(0x1E06, 0x0001);
    //RegSet16BitValue((uint)0x1E04, (uint)0x7D60);
    //RegSet16BitValue(0x1E04, 0x829F);
    RegSet16BitValue(0x0FE6, 0x0000);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        mdelay(300);
    }
    else
    {
        mdelay(150);
    }

    if (_DrvMpTestItoTestMsg28xxCheckSwitchStatus() < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# CheckSwitchStatus failed! Enter MP mode failed ***\n");
        return -1;
    }

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        if (_gMsg28xxDeepStandBy == 1)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# enter deep standby! ***\n");

            RegSet16BitValue(0x1402, 0x6179);
            mdelay(_gMsg28xxDeepStandByTimeOut);

            DbBusEnterSerialDebugMode();
    //	        DbBusWaitMCU();
            DbBusStopMCU();
            DbBusIICUseBus();
            DbBusIICReshape();

            if (_DrvMpTestItoTestMsg28xxCheckSwitchStatus() < 0)
            {
                _gMsg28xxDeepStandBy = 0;
                DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# Deep standby fail, fw not support DEEP STANDBY ***\n");
                return -1;
            }
        }
    }
    else
    {
        if (_gMsg28xxDeepStandBy == 0)
        {
            //deep satndby mode
            RegSet16BitValue(0x1402, 0x6179);
            mdelay(600);

            DbBusEnterSerialDebugMode();
            DbBusWaitMCU();
            DbBusIICUseBus();
            DbBusIICReshape();

            if (_DrvMpTestItoTestMsg28xxCheckSwitchStatus() < 0)
            {
                _gMsg28xxDeepStandBy = -1;
                DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# Deep standby fail, fw not support DEEP STANDBY ***\n");
                return -1;
            }
        }
    }

    if (*pFMode == MUTUAL_SINE)
    {
    	nFreq = (_gMsg28xx_FIXED_CARRIER > 30) ? _gMsg28xx_FIXED_CARRIER : 30;
    	nFreq = (_gMsg28xx_FIXED_CARRIER < 140) ? _gMsg28xx_FIXED_CARRIER : 140;
    	aCmd[2] = nFreq;

        DBG(&g_I2cClient->dev, "*** nFreq = %d  ***\n", nFreq);
    	IicWriteData(SLAVE_I2C_ID_DWI2C, &aCmd[0], sizeof(aCmd) / sizeof(u8));
    }

    DBG(&g_I2cClient->dev, "*** nFMode = %d  ***\n", *pFMode);

    switch (*pFMode)
    {
        case MUTUAL_MODE:
            RegSet16BitValue(0x1402, 0x5705);
            break;

        case MUTUAL_SINE:
            RegSet16BitValue(0x1402, 0x5706);
            break;

        case SELF:
            RegSet16BitValue(0x1402, 0x6278);
            break;

        case WATERPROOF:
            RegSet16BitValue(0x1402, 0x7992);
            break;

        case MUTUAL_SINGLE_DRIVE:
            RegSet16BitValue(0x1402, 0x0158);
            break;

        default:
            return -1;
    }

    if (_DrvMpTestItoTestMsg28xxCheckSwitchStatus() < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx MP Test# CheckSwitchStatus failed! Enter FW mode failed  ***\n");
        return -1;
    }

    if (*pFMode == MUTUAL_SINE)
    {
        nRegData = RegGet16BitValueByAddressMode(0x2003, ADDRESS_MODE_16BIT);
        nFreqData = nRegData * (13000000 / 16384) / 1000; //khz

        if (abs(nFreqData - _gMsg28xx_FIXED_CARRIER) >= 2)
        {
            DBG(&g_I2cClient->dev, "Fixed carrier failed, current frequency = %d khz, need fixed frequency = %d khz", nFreqData, _gMsg28xx_FIXED_CARRIER);
            return -1;
        }
    }

    RegSet16BitValue(0x0FE6, 0x0001);// stop mcu
    RegSet16BitValue(0x3D08, 0xFEFF);//open timer

    return 0;
}

u16 _DrvMpTestMsg28xxItoTestGetTpType(void)
{
    u16 nMajor = 0, nMinor = 0;
    u8 szDbBusTxData[3] = {0};
    u8 szDbBusRxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x03;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);
    
    nMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
    nMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];

    DBG(&g_I2cClient->dev, "*** major = %d ***\n", nMajor);
    DBG(&g_I2cClient->dev, "*** minor = %d ***\n", nMinor);

    return nMajor;
}

static void _DrvMpTestItoOpenTestMsg28xxOnCellCalGoldenRange(void)
{
    u32 i = 0, value = 0, value_up = 0;

    for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i++)
    {
        value = (s32)_gMsg28xx_DC_RANGE * abs(_gMutualICOnCellOpenTestGoldenChannel[i]) / 100;
        value_up = (s32)_gMsg28xx_DC_RANGE_UP * abs(_gMutualICOnCellOpenTestGoldenChannel[i]) / 100;
        _gMutualICOnCellOpenTestGoldenChannelMax[i] = _gMutualICOnCellOpenTestGoldenChannel[i] + value + value_up;
        _gMutualICOnCellOpenTestGoldenChannelMin[i] = _gMutualICOnCellOpenTestGoldenChannel[i] - value;
    }
}

static u16 _DrvMpTestMsg28xxItoTestChooseTpType(void)
{
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gMsg28xx_SENSE_NUM = 0;
    _gMsg28xx_DRIVE_NUM = 0;
    _gMsg28xx_KEY_NUM = 0;
    _gMsg28xx_KEY_LINE = 0;
    _gMsg28xx_GR_NUM = 0;
    _gMsg28xx_CSUB_REF = 0;
    _gMsg28xx_CFB_REF = 0;
    _gMsg28xx_SENSE_MUTUAL_SCAN_NUM = 0;
    _gMsg28xx_MUTUAL_KEY = 0;
    _gMsg28xx_DC_RANGE = 0;
    _gMsg28xx_DC_RANGE_UP = 0;
    _gMsg28xx_DC_Ratio_1000 = 0;
    _gMsg28xx_DC_Ratio_UP_1000 = 0;
    _gMsg28xx_DC_Border_Ratio_1000 = 0;
    _gMsg28xx_DC_Border_Ratio_UP_1000 = 0;
    _gMsg28xx_PATTERN_MODEL = 0;
    _gMsg28xx_SENSOR_KEY_CH = 0;
    _gMsg28xxDeepStandBy = 0;
    _gMsg28xxDeepStandByTimeOut = 0;
    _gMsg28xx_SHORT_VALUE=0;
    _gMsg28xx_ICPIN_SHORT=0;
    _gMsg28xx_SupportIC=0;
    _gMsg28xx_OPEN_MODE = 0;
    _gMsg28xx_CHARGE_PUMP = 0;
    _gMsg28xx_KEYDRV_O = 0;
    _gMsg28xx_SUB_FRAME = 0;
    _gMsg28xx_FIXED_CARRIER = 0;

    _gMsg28xx_SHORT_N1_TEST_NUMBER = 0;
    _gMsg28xx_SHORT_N2_TEST_NUMBER = 0;
    _gMsg28xx_SHORT_S1_TEST_NUMBER = 0;
    _gMsg28xx_SHORT_S2_TEST_NUMBER = 0;
    _gMsg28xx_SHORT_TEST_5_TYPE = 0;
    _gMsg28xx_SHORT_X_TEST_NUMBER = 0;
                                                                                                                                        
    _gMsg28xx_SHORT_N1_TEST_PIN = NULL;
    _gMsg28xx_SHORT_N1_MUX_MEM_20_3E = NULL;
    _gMsg28xx_SHORT_N2_TEST_PIN = NULL;
    _gMsg28xx_SHORT_N2_MUX_MEM_20_3E = NULL;
    _gMsg28xx_SHORT_S1_TEST_PIN = NULL;
    _gMsg28xx_SHORT_S1_MUX_MEM_20_3E = NULL;
    _gMsg28xx_SHORT_S2_TEST_PIN = NULL;
    _gMsg28xx_SHORT_S2_MUX_MEM_20_3E = NULL;
    _gMsg28xx_SHORT_X_TEST_PIN = NULL;
    _gMsg28xx_SHORT_X_MUX_MEM_20_3E = NULL;                                                           
    _gMsg28xx_SHORT_GR_TEST_PIN = NULL;
    _gMsg28xx_SHORT_GR_MUX_MEM_20_3E = NULL;
    _gMsg28xx_PAD_TABLE_DRIVE = NULL;
    _gMsg28xx_PAD_TABLE_SENSE = NULL;
    _gMsg28xx_PAD_TABLE_GR = NULL;
                                   
    _gMsg28xx_KEYSEN = NULL;
    _gMsg28xx_KEYDRV = NULL;
                                                                
    //g_Msg28xxMapVaMutual = NULL;

    for (i = 0; i < 10; i ++)
    {
        _gMsg28xxTpType = _DrvMpTestMsg28xxItoTestGetTpType();
        DBG(&g_I2cClient->dev, "TP Type = %d, i = %d\n", _gMsg28xxTpType, i);

        if (TP_TYPE_HLT == _gMsg28xxTpType)// || TP_TYPE_Y == _gMsg28xxTpType) // Modify.
        {
            break;
        }
        else if (i < 5)
        {
            mdelay(100);  
        }
        else
        {
            DrvPlatformLyrTouchDeviceResetHw();
        }
    }
    
    if (TP_TYPE_HLT== _gMsg28xxTpType) // Modify. 
    {
        DBG(&g_I2cClient->dev, "*** Choose Tp Type X ***\n");        
        
        _gMsg28xx_SENSE_NUM = SENSE_NUM_X;
        _gMsg28xx_DRIVE_NUM = DRIVE_NUM_X;
        _gMsg28xx_KEY_NUM = KEY_NUM_X;
        _gMsg28xx_KEY_LINE = KEY_LINE_X;
        _gMsg28xx_GR_NUM = GR_NUM_X;
        _gMsg28xx_CSUB_REF = CSUB_REF_X;
        _gMsg28xx_SENSE_MUTUAL_SCAN_NUM = SENSE_MUTUAL_SCAN_NUM_X;
        _gMsg28xx_MUTUAL_KEY = MUTUAL_KEY_X;
        _gMsg28xx_PATTERN_TYPE = PATTERN_TYPE_X;
    
        _gMsg28xx_SHORT_N1_TEST_NUMBER = SHORT_N1_TEST_NUMBER_X;
        _gMsg28xx_SHORT_N2_TEST_NUMBER = SHORT_N2_TEST_NUMBER_X;
        _gMsg28xx_SHORT_S1_TEST_NUMBER = SHORT_S1_TEST_NUMBER_X;
        _gMsg28xx_SHORT_S2_TEST_NUMBER = SHORT_S2_TEST_NUMBER_X;
        _gMsg28xx_SHORT_TEST_5_TYPE = SHORT_TEST_5_TYPE_X;
        _gMsg28xx_SHORT_X_TEST_NUMBER = SHORT_X_TEST_NUMBER_X;
                                                                                                                                            
        _gMsg28xx_SHORT_N1_TEST_PIN = MSG28XX_SHORT_N1_TEST_PIN_X;
        _gMsg28xx_SHORT_N1_MUX_MEM_20_3E = SHORT_N1_MUX_MEM_20_3E_X;
        _gMsg28xx_SHORT_N2_TEST_PIN = MSG28XX_SHORT_N2_TEST_PIN_X;
        _gMsg28xx_SHORT_N2_MUX_MEM_20_3E = SHORT_N2_MUX_MEM_20_3E_X;
        _gMsg28xx_SHORT_S1_TEST_PIN = MSG28XX_SHORT_S1_TEST_PIN_X;
        _gMsg28xx_SHORT_S1_MUX_MEM_20_3E = SHORT_S1_MUX_MEM_20_3E_X;
        _gMsg28xx_SHORT_S2_TEST_PIN = MSG28XX_SHORT_S2_TEST_PIN_X;
        _gMsg28xx_SHORT_S2_MUX_MEM_20_3E = SHORT_S2_MUX_MEM_20_3E_X;
        _gMsg28xx_SHORT_X_TEST_PIN = MSG28XX_SHORT_X_TEST_PIN_X;
        _gMsg28xx_SHORT_X_MUX_MEM_20_3E = SHORT_X_MUX_MEM_20_3E_X;                                                           
                                       
        _gMsg28xx_PAD_TABLE_DRIVE = PAD_TABLE_DRIVE_X;
        _gMsg28xx_PAD_TABLE_SENSE = PAD_TABLE_SENSE_X;
        _gMsg28xx_PAD_TABLE_GR = PAD_TABLE_GR_X;
                                       
        _gMsg28xx_KEYSEN = KEYSEN_X;
        _gMsg28xx_KEYDRV = KEYDRV_X;
                                                                    
        if (_gMsg28xx_PATTERN_TYPE == 5)
        {
            _gMsg28xx_CFB_REF = CFB_REF_X;
            _gMsg28xx_DC_RANGE = DC_RANGE_X;
            _gMsg28xx_DC_RANGE_UP = DC_Range_UP_X;
            _gMsg28xx_DC_Ratio_1000 = DC_Ratio_X * 1000;
            _gMsg28xx_DC_Ratio_UP_1000 = DC_Ratio_UP_X * 1000;
            _gMsg28xx_DC_Border_Ratio_1000 = DC_Border_Ratio_X * 1000;
            _gMsg28xx_DC_Border_Ratio_UP_1000 = DC_Border_Ratio_UP_X * 1000;
            _gMsg28xx_PATTERN_MODEL = TP_1T2R_MODEL_X;
            _gMsg28xx_SENSOR_KEY_CH = SENSOR_KEY_CH_X;
            _gMsg28xxDeepStandBy = DEEP_STANDBY_X;
            _gMsg28xxDeepStandByTimeOut = DEEP_STANDBY_TIMEOUT_X;
            _gMsg28xx_SHORT_VALUE=SHORT_VALUE_X;
            _gMsg28xx_ICPIN_SHORT=ICPIN_SHORT_X;
            _gMsg28xx_SupportIC=SupportIC_X;
            _gMsg28xx_OPEN_MODE = OPEN_MODE_X;
            _gMsg28xx_CHARGE_PUMP = CHARGE_PUMP_X;
            _gMsg28xx_KEYDRV_O = KEYDRV_O_X;
            _gMsg28xx_SUB_FRAME = SUB_FRAME_X;
            _gMsg28xx_FIXED_CARRIER = FIXED_CARRIER_X;

            _gMsg28xx_SHORT_X_TEST_NUMBER = SHORT_GR_TEST_NUMBER_X;
            _gMsg28xx_SHORT_GR_TEST_PIN = MSG28XX_SHORT_GR_TEST_PIN_X;
            _gMsg28xx_SHORT_GR_MUX_MEM_20_3E = SHORT_GR_MUX_MEM_20_3E_X;

            _gMsg28xx_SHORT_IcPIN_MUX_MEM_1_Settings = ICPIN_MUX_MEM_1_Settings_X;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_2_Settings = ICPIN_MUX_MEM_2_Settings_X;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_3_Settings = ICPIN_MUX_MEM_3_Settings_X;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_4_Settings = ICPIN_MUX_MEM_4_Settings_X;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_5_Settings = ICPIN_MUX_MEM_5_Settings_X;

            _gMsg28xx_SENSE_PAD_PIN_MAPPING = SENSE_PAD_PIN_MAPPING_X;

            memcpy(_gMutualICOnCellOpenTestGoldenChannel, g_GoldenChannel_X, sizeof(_gMutualICOnCellOpenTestGoldenChannel));

            _DrvMpTestItoOpenTestMsg28xxOnCellCalGoldenRange();
        }
        //g_Msg28xxMapVaMutual = g_MapVaMutual_X;
    }
#if 0
    else if (TP_TYPE_Y == _gMsg28xxTpType) // Modify. 
    {
        DBG(&g_I2cClient->dev, "*** Choose Tp Type Y ***\n");        
        
        _gMsg28xx_SENSE_NUM = SENSE_NUM_Y;
        _gMsg28xx_DRIVE_NUM = DRIVE_NUM_Y;
        _gMsg28xx_KEY_NUM = KEY_NUM_Y;
        _gMsg28xx_KEY_LINE = KEY_LINE_Y;
        _gMsg28xx_GR_NUM = GR_NUM_Y;
        _gMsg28xx_CSUB_REF = CSUB_REF_Y;
        _gMsg28xx_SENSE_MUTUAL_SCAN_NUM = SENSE_MUTUAL_SCAN_NUM_Y;
        _gMsg28xx_MUTUAL_KEY = MUTUAL_KEY_Y;
        _gMsg28xx_PATTERN_TYPE = PATTERN_TYPE_Y;
    
        _gMsg28xx_SHORT_N1_TEST_NUMBER = SHORT_N1_TEST_NUMBER_Y;
        _gMsg28xx_SHORT_N2_TEST_NUMBER = SHORT_N2_TEST_NUMBER_Y;
        _gMsg28xx_SHORT_S1_TEST_NUMBER = SHORT_S1_TEST_NUMBER_Y;
        _gMsg28xx_SHORT_S2_TEST_NUMBER = SHORT_S2_TEST_NUMBER_Y;
        _gMsg28xx_SHORT_TEST_5_TYPE = SHORT_TEST_5_TYPE_Y;
        _gMsg28xx_SHORT_X_TEST_NUMBER = SHORT_X_TEST_NUMBER_Y;
                                                                                                                                            
        _gMsg28xx_SHORT_N1_TEST_PIN = MSG28XX_SHORT_N1_TEST_PIN_Y;
        _gMsg28xx_SHORT_N1_MUX_MEM_20_3E = SHORT_N1_MUX_MEM_20_3E_Y;
        _gMsg28xx_SHORT_N2_TEST_PIN = MSG28XX_SHORT_N2_TEST_PIN_Y;
        _gMsg28xx_SHORT_N2_MUX_MEM_20_3E = SHORT_N2_MUX_MEM_20_3E_Y;
        _gMsg28xx_SHORT_S1_TEST_PIN = MSG28XX_SHORT_S1_TEST_PIN_Y;
        _gMsg28xx_SHORT_S1_MUX_MEM_20_3E = SHORT_S1_MUX_MEM_20_3E_Y;
        _gMsg28xx_SHORT_S2_TEST_PIN = MSG28XX_SHORT_S2_TEST_PIN_Y;
        _gMsg28xx_SHORT_S2_MUX_MEM_20_3E = SHORT_S2_MUX_MEM_20_3E_Y;
        _gMsg28xx_SHORT_X_TEST_PIN = MSG28XX_SHORT_X_TEST_PIN_Y;
        _gMsg28xx_SHORT_X_MUX_MEM_20_3E = SHORT_X_MUX_MEM_20_3E_Y;                                                           
                                       
        _gMsg28xx_PAD_TABLE_DRIVE = PAD_TABLE_DRIVE_Y;
        _gMsg28xx_PAD_TABLE_SENSE = PAD_TABLE_SENSE_Y;
        _gMsg28xx_PAD_TABLE_GR = PAD_TABLE_GR_Y;
                                       
        _gMsg28xx_KEYSEN = KEYSEN_Y;
        _gMsg28xx_KEYDRV = KEYDRV_Y;
                                                                    
        if ((_gMsg28xx_PATTERN_TYPE == 5))
        {
            _gMsg28xx_CFB_REF = CFB_REF_Y;
            _gMsg28xx_DC_RANGE = DC_RANGE_Y;
            _gMsg28xx_DC_RANGE_UP = DC_Range_UP_X;
            _gMsg28xx_DC_Ratio_1000 = DC_Ratio_Y * 1000;
            _gMsg28xx_DC_Ratio_UP_1000 = DC_Ratio_UP_Y * 1000;
            _gMsg28xx_DC_Border_Ratio_1000 = DC_Border_Ratio_Y * 1000;
            _gMsg28xx_DC_Border_Ratio_UP_1000 = DC_Border_Ratio_UP_Y * 1000;
            _gMsg28xx_PATTERN_MODEL = TP_1T2R_MODEL_Y;
            _gMsg28xx_SENSOR_KEY_CH = SENSOR_KEY_CH_Y;
            _gMsg28xxDeepStandBy = DEEP_STANDBY_Y;
            _gMsg28xxDeepStandByTimeOut = DEEP_STANDBY_TIMEOUT_Y;
            _gMsg28xx_SHORT_VALUE=SHORT_VALUE_Y;
            _gMsg28xx_ICPIN_SHORT=ICPIN_SHORT_Y;
            _gMsg28xx_SupportIC=SupportIC_Y;
            _gMsg28xx_OPEN_MODE = OPEN_MODE_Y;
            _gMsg28xx_CHARGE_PUMP = CHARGE_PUMP_Y;
            _gMsg28xx_KEYDRV_O = KEYDRV_O_Y;
            _gMsg28xx_SUB_FRAME = SUB_FRAME_Y;
             _gMsg28xx_FIXED_CARRIER = FIXED_CARRIER_Y;

            _gMsg28xx_SHORT_X_TEST_NUMBER = SHORT_GR_TEST_NUMBER_Y;
            _gMsg28xx_SHORT_GR_TEST_PIN = MSG28XX_SHORT_GR_TEST_PIN_Y;
            _gMsg28xx_SHORT_GR_MUX_MEM_20_3E = SHORT_GR_MUX_MEM_20_3E_Y;

            _gMsg28xx_SHORT_IcPIN_MUX_MEM_1_Settings = ICPIN_MUX_MEM_1_Settings_Y;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_2_Settings = ICPIN_MUX_MEM_2_Settings_Y;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_3_Settings = ICPIN_MUX_MEM_3_Settings_Y;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_4_Settings = ICPIN_MUX_MEM_4_Settings_Y;
            _gMsg28xx_SHORT_IcPIN_MUX_MEM_5_Settings = ICPIN_MUX_MEM_5_Settings_Y;

            _gMsg28xx_SENSE_PAD_PIN_MAPPING = SENSE_PAD_PIN_MAPPING_Y;

            memcpy(_gMutualICOnCellOpenTestGoldenChannel, g_GoldenChannel_Y, sizeof(_gMutualICOnCellOpenTestGoldenChannel));

            _DrvMpTestItoOpenTestMsg28xxOnCellCalGoldenRange();
        }
        //g_Msg28xxMapVaMutual = g_MapVaMutual_Y;
    }
#endif
    else
    {
        _gMsg28xxTpType = 0;
    }
    
    return _gMsg28xxTpType;
}

s32 _DrvMpTestMsg28xxItoOpenTestEntry(void)
{
    u8 nFwMode = MUTUAL_MODE;
    s32 nRetVal = 0;
//    u8 nDrvOpening = 0;
    //u16 nCheckState = 0;
    u16 nTime = 0;    
    s8 aNormalTestResult[8][2] = {{0}};    //0:golden    1:ratio
    u16 aNormalTestResultCheck[6][13] = {{0}};        //6:max subframe    13:max afe
    s8 aOnCellNormalTestResult[2] = {0};    //0:golden    1:ratio
    u16 aOnCellNormalTestResultCheck[MUTUAL_IC_MAX_MUTUAL_NUM] = {0};        //6:max subframe    13:max afe
    u8 nVaMode = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    if (_gMsg28xx_PATTERN_TYPE != 5)
    {
        _gMsg28xxDeepStandBy = 0;
    }
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        nRetVal = -2;
        goto ITO_TEST_END;
    }

    _gMutualICSenseLineNum = _gMsg28xx_SENSE_NUM;        
    _gMutualICDriveLineNum = _gMsg28xx_DRIVE_NUM;

_RETRY_OPEN:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    //DbBusWaitMCU();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);    

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        switch (_gMsg28xx_OPEN_MODE)
        {
            case 0:
                nFwMode = MUTUAL_MODE;
                break;
            case 1:
                nFwMode = MUTUAL_SINE;
                break;
        }
    }

    if (_DrvMpTestMsg28xxItoTestSwitchFwMode(&nFwMode) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _RETRY_OPEN;
        }    
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# Switch Fw Mode failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;
    }

    //nDrvOpening = RegGetLByteValue(0x1312);
    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        if (_DrvMpTestMsg28xxOnCellItoOpenTest(nFwMode, nVaMode) < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# OpenTest failed! ***\n");
            nRetVal = -1;
            goto ITO_TEST_END;
        }

        if ((!nVaMode) && (nFwMode == MUTUAL_SINE))
        {
            nVaMode = 1;
            goto _RETRY_OPEN;
        }
    }
    else
    {
        if(_DrvMpTestMsg28xxItoOpenTest() < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# OpenTest failed! ***\n");
            nRetVal = -1;
            goto ITO_TEST_END;
        }
    }
    
    mdelay(10);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        nRetVal = _DrvMpTestItoOpenTestMsg28xxOnCellOpenJudge(0, aOnCellNormalTestResult, aOnCellNormalTestResultCheck/*, nDrvOpening*/);
    }
    else
    {
        nRetVal = _DrvMpTestItoOpenTestMsg28xxOpenJudge(0, aNormalTestResult, aNormalTestResultCheck/*, nDrvOpening*/);
    }
    DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# OpenTestOpenJudge return value = %d ***\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
ITO_TEST_END:    

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);
    
    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

static void _DrvMpTestItoShortTestMsg28xxSetNoiseSensorMode(u8 nEnable)
{
    s16 j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);    

    if (nEnable)
    {
        RegSet16BitValueOn(0x1546, BIT4);
        for (j = 0; j < 10; j++)
        {
            RegSet16BitValue(0x2148 + 2 * j, 0x0000);
        }
        RegSet16BitValue(0x215C, 0x1FFF);
    }
}

static void _DrvMpTestItoShortTestMsg28xxAnaFixPrs(u16 nOption)
{
    u16 nRegData = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001);

    nRegData = RegGet16BitValue(0x1008);
    nRegData &= 0x00F1;
    nRegData |= (u16)((nOption << 1) & 0x000E);
    RegSet16BitValue(0x1008, nRegData);
}

static void _DrvMpTestItoShortTestMsg28xxChangeANASetting(void)
{
    int i, nMappingItem; 
    u8 nChipVer;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
    
    nChipVer = RegGetLByteValue(0x1ECE);
    //uint chip_ver = Convert.ToUInt16((uint)regdata[0]); //for U01 (SF shift).
    
    if (nChipVer != 0)
        RegSetLByteValue(0x131E, 0x01);
    
    for (nMappingItem = 0; nMappingItem < 6; nMappingItem++)
    {
        /// sensor mux sram read/write base address / write length
        RegSetLByteValue(0x2192, 0x00);
        RegSetLByteValue(0x2102, 0x01);
        RegSetLByteValue(0x2102, 0x00);
        RegSetLByteValue(0x2182, 0x08);
        RegSetLByteValue(0x2180, 0x08 * nMappingItem);
        RegSetLByteValue(0x2188, 0x01);
    
        for (i = 0; i < 8; i++)
        {
            if (nMappingItem == 0 && nChipVer == 0x0)
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 1 && nChipVer == 0x0) || (nMappingItem == 0 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 2 && nChipVer == 0x0) || (nMappingItem == 1 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 3 && nChipVer == 0x0) || (nMappingItem == 2 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 4 && nChipVer == 0x0) || (nMappingItem == 3 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 5 && nChipVer == 0x0) || (nMappingItem == 4 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i + 1]);
            }
            if (nMappingItem == 5 && nChipVer != 0x0)
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i + 1]);
            }
        }
    }
}

static void _DrvMpTestMsg28xxItoReadSetting(u16 * pPad2Sense, u16 * pPad2Drive, u16 * pPad2GR)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    memcpy(_gMsg28xxMuxMem_20_3E_1_Settings, _gMsg28xx_SHORT_N1_MUX_MEM_20_3E, sizeof(u16) * 16);
    memcpy(_gMsg28xxMuxMem_20_3E_2_Settings, _gMsg28xx_SHORT_N2_MUX_MEM_20_3E, sizeof(u16) * 16);
    memcpy(_gMsg28xxMuxMem_20_3E_3_Settings, _gMsg28xx_SHORT_S1_MUX_MEM_20_3E, sizeof(u16) * 16);
    memcpy(_gMsg28xxMuxMem_20_3E_4_Settings, _gMsg28xx_SHORT_S2_MUX_MEM_20_3E, sizeof(u16) * 16);

    if (_gMsg28xx_SHORT_TEST_5_TYPE != 0)
    {
        if (_gMsg28xx_PATTERN_TYPE == 5)
        {
            memcpy(_gMsg28xxMuxMem_20_3E_5_Settings, _gMsg28xx_SHORT_GR_MUX_MEM_20_3E, sizeof(u16) * 16);
        }
        else
        {
            memcpy(_gMsg28xxMuxMem_20_3E_5_Settings, _gMsg28xx_SHORT_X_MUX_MEM_20_3E, sizeof(u16) * 16);
        }
    }

    memcpy(pPad2Sense, _gMsg28xx_PAD_TABLE_SENSE, sizeof(u16) * _gMutualICSenseLineNum);
    memcpy(pPad2Drive, _gMsg28xx_PAD_TABLE_DRIVE, sizeof(u16) * _gMutualICDriveLineNum);

    if (_gMsg28xx_GR_NUM != 0)
    {
        memcpy(pPad2GR, _gMsg28xx_PAD_TABLE_GR, sizeof(u16) * _gMsg28xx_GR_NUM);
    }
}

static s32 _DrvMpTestItoShortTestMsg28xxGetValueR(s32 * pTarget)
{    
    s16 * pRawData = NULL;
    u16 nSenNumBak = 0;
    u16 nDrvNumBak = 0;     
    u16 nShift = 0;
    s16 i, j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        pRawData = kzalloc(sizeof(s16) * 1170, GFP_KERNEL);
    }
    else
    {
        pRawData = kzalloc(sizeof(s16) * MUTUAL_IC_MAX_CHANNEL_SEN*2 * MUTUAL_IC_MAX_CHANNEL_DRV, GFP_KERNEL);
    }

    if (_DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(pRawData, &nSenNumBak, &nDrvNumBak) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# GetMutualOneShotRawIIR failed! ***\n");                    
        return -1;
    }
    
    for (i = 0; i < 5; i ++)
    {
        for (j = 0; j < 13; j ++)
        {
            nShift = (u16)(j + 13 * i);
            if (_gMsg28xx_PATTERN_TYPE == 5)
            {
                pTarget[nShift] = pRawData[i*13+j];
            }
            else
            {
                pTarget[nShift] = pRawData[i*MUTUAL_IC_MAX_CHANNEL_DRV+j];
            }
        }
    }

    kfree(pRawData);

    return 0;
}

static void _DrvMpTestItoShortTestMsg28xxOnCellSetSensorPADState(u16 state)
{
	u16 value = 0, i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < 8; i ++)
    {
        value |= (u16)(state << (i * 2));
    }

    for (i = 0; i < 8; i ++)
    {
        RegSet16BitValueByAddressMode(0x1514 + i, value, ADDRESS_MODE_16BIT);
    }

    for (i = 0; i < 4; i ++)
    {
        RegSet16BitValueByAddressMode(0x1510 + i, 0xFFFF, ADDRESS_MODE_16BIT); //After DAC overwrite, output DC
    }
}

static void _DrvMpTestItoShortTestMsg28xxOnCellpatchFWAnaSetting_for_shorttest(void)
{
	u16 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // overwrite sensor PAD , restore to default state
    for (i = 0; i < 8; i ++)
    {
        RegSet16BitValueByAddressMode(0x1e33 + i, 0x0000, ADDRESS_MODE_16BIT);
    }

    // overwrite PAD gpio , restore to default state
    RegSet16BitValueByAddressMode(0x1e30, 0x000f, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1e31, 0x0000, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1e32, 0xffff, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1e3b, 0xffff, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1e3c, 0xffff, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x1e3d, 0x003f, ADDRESS_MODE_16BIT);

    for (i = 0; i < 20; i ++)
    {
        RegSet16BitValueByAddressMode(0x2110 + i, 0x0000, ADDRESS_MODE_16BIT);
    }

    for (i = 0; i < 16; i ++)
    {
        RegSet16BitValueByAddressMode(0x2160 + i, 0x0000, ADDRESS_MODE_16BIT);
    }

    //post idle for
    RegSet16BitValueByAddressMode(0x101a, 0x0028, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x101b, 0x0028, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x101c, 0x0028, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x101d, 0x0028, ADDRESS_MODE_16BIT);
    RegSet16BitValueByAddressMode(0x101e, 0x0028, ADDRESS_MODE_16BIT);
}

static s32 _DrvMpTestMsg28xxItoShortTest(u8 nItemID)
{
    s16 i;
    u8 nRegData = 0;
    u16 nAfeCoef = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);    

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

    ///set Subframe = 6 ; Sensor = 13
    RegSetLByteValue(0x130A, 0x6D);
    RegSetLByteValue(0x1103, 0x06);
    RegSetLByteValue(0x1016, 0x0C);

    RegSetLByteValue(0x1104, 0x0C);
    RegSetLByteValue(0x100C, 0x0C);
    RegSetLByteValue(0x1B10, 0x0C);

    /// adc analog+digital pipe delay, 60= 13 AFE.
    RegSetLByteValue(0x102F, 0x60);

    ///trim: Fout 52M &  1.2V
    RegSet16BitValue(0x1420, 0xA55A);//password
    RegSet16BitValue(0x1428, 0xA55A);//password
    RegSet16BitValue(0x1422, 0xFC4C);//go

    _DrvMpTestItoShortTestMsg28xxSetNoiseSensorMode(1);    
    _DrvMpTestItoShortTestMsg28xxAnaFixPrs(3);    
    _DrvMpTestItoTestMsg28xxAnaChangeCDtime(0x007E, 0x0006);

    if (_gMsg28xx_PATTERN_TYPE != 5)
    {
        ///DAC overwrite
        RegSet16BitValue(0x150C, 0x80A2); //bit15 //AE:3.5v for test
        RegSet16BitValue(0x1520, 0xFFFF);//After DAC overwrite, output DC
        RegSet16BitValue(0x1522, 0xFFFF);
        RegSet16BitValue(0x1524, 0xFFFF);
        RegSet16BitValue(0x1526, 0xFFFF);
    }

    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)

    /// reg_afe_icmp disenable
    RegSet16BitValue(0x1552, 0x0000);

    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);

    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);

    ///reg_sel_ros disenable
    RegSet16BitValue(0x156A, 0x0000);

    ///reg_adc_desp_invert disenable
    RegSetLByteValue(0x1221, 0x00);

    if (_gMsg28xx_PATTERN_TYPE != 5)
    {
        ///AFE coef
        nRegData = RegGetLByteValue(0x101A);
        nAfeCoef = 0x10000 / nRegData;
        RegSet16BitValue(0x13D6, nAfeCoef);
    }
    
    /// AFE gain = 1X
    //RegSet16BitValue(0x1318, 0x4440);
    //RegSet16BitValue(0x131A, 0x4444);
    //RegSet16BitValue(0x13D6, 0x2000);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        //AFE gain = 1X
        RegSet16BitValue(0x1318, 0x4440);
        RegSet16BitValue(0x131A, 0x4444);

        RegSet16BitValueByAddressMode(0x1012, 0x0680, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1022, 0x0000, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x110A, 0x0104, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1310, 0x04F1, ADDRESS_MODE_16BIT);

        RegSet16BitValueByAddressMode(0x1317, 0x04F1, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1432, 0x0000, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1435, 0x0C00, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1538, 0x0400, ADDRESS_MODE_16BIT);

        RegSet16BitValueByAddressMode(0x1540, 0x0012, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1530, 0x0133, ADDRESS_MODE_16BIT);//HI v buf enable
        //	    	RegSet16BitValueByAddressMode(0x1533, 0x0522, ADDRESS_MODE_16BIT);//low v buf gain
        RegSet16BitValueByAddressMode(0x1533, 0x0000, ADDRESS_MODE_16BIT);//low v buf gain
        RegSet16BitValueByAddressMode(0x1E11, 0x8000, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x2003, 0x007E, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x2006, 0x137F, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x213E, 0x1FFF, ADDRESS_MODE_16BIT);

        // re-set sample and coefficient
        RegSet16BitValueByAddressMode(0x100D, 0x0020, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1103, 0x0020, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1104, 0x0020, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1302, 0x0020, ADDRESS_MODE_16BIT);
        RegSet16BitValueByAddressMode(0x1B30, 0x0020, ADDRESS_MODE_16BIT);

        // coefficient
        RegSet16BitValueByAddressMode(0x136B, 0x10000 / 0x0020, ADDRESS_MODE_16BIT);// 65536/ sample

        _DrvMpTestItoShortTestMsg28xxOnCellSetSensorPADState(POS_PULSE);

        //DAC overwrite
        RegSet16BitValue(0x150C, 0x8040); //bit15 //AFE:1.3v for test
    }

    _DrvMpTestItoShortTestMsg28xxChangeANASetting();
    _DrvMpTestItoTestMsg28xxAnaSwReset();

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        _DrvMpTestItoShortTestMsg28xxOnCellpatchFWAnaSetting_for_shorttest();
        memset(_gMutualICDeltaC, 0, sizeof(_gMutualICDeltaC));
    }
    if (_DrvMpTestItoShortTestMsg28xxGetValueR(_gMutualICDeltaC)<0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# GetValueR failed! ***\n");      
        return -1;
    }
    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Ito Short Test# GetValueR 1.3v! ***\n");
        _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC, 65, -32, 10, 13);

        memset(_gMutualICDeltaC2, 0, sizeof(_gMutualICDeltaC2));
        RegSet16BitValue(0x150C, 0x8083); //bit15 //AFE:3.72v for test
        if (_DrvMpTestItoShortTestMsg28xxGetValueR(_gMutualICDeltaC2)<0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# GetValueR failed! ***\n");
            return -1;
        }
        DBG(&g_I2cClient->dev, "*** Msg28xx Ito Short Test# GetValueR 3.72v! ***\n");
        _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC2, 65, -32, 10, 13);
    }
    else
    {
        _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC, 128, -32, 10, 8);
    }

    for (i = 0; i < 65; i ++) // 13 AFE * 5 subframe
    {
        if (_gMsg28xx_PATTERN_TYPE == 5)
        {
            if ((abs(_gMutualICDeltaC2[i]) < MSG28XX_IIR_MAX) && (abs(_gMutualICDeltaC[i]) < MSG28XX_IIR_MAX))
            {
                _gMutualICDeltaC[i] = _gMutualICDeltaC2[i] - _gMutualICDeltaC[i];
            }
            else
            {
                _gMutualICDeltaC[i] = _gMutualICDeltaC2[i];
            }

//	            if (_gMutualICDeltaC[i] <= -1000 || _gMutualICDeltaC[i] >= (MSG28XX_IIR_MAX))
            if (_gMutualICDeltaC[i] >= (MSG28XX_IIR_MAX))
            {
                _gMutualICDeltaC[i] = 0x7FFF;
            }
            else
            {
                _gMutualICDeltaC[i] = abs(_gMutualICDeltaC[i]);
            }
        }
        else
        {
            if (_gMutualICDeltaC[i] <= -1000 || _gMutualICDeltaC[i] >= (MUTUAL_IC_IIR_MAX))
            {
                _gMutualICDeltaC[i] = 0x7FFF;
            }
            else
            {
                _gMutualICDeltaC[i] = abs(_gMutualICDeltaC[i]);
            }
        }
    }

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Ito Short Test# GetValueR 3.72v - 1.3v ! ***\n");
        _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC, 65, -32, 10, 13);
    }

    return 0;
}

static s32 _DrvMpTestItoShortTestMsg28xxReadTestPins(u8 nItemID, u16 * pTestPins)
{    
    u16 nCount = 0;
    s16 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    switch (nItemID)
    {
        case 1:
        case 11:
            nCount = _gMsg28xx_SHORT_N1_TEST_NUMBER;            
            memcpy(pTestPins, _gMsg28xx_SHORT_N1_TEST_PIN, sizeof(u16) * nCount);
            break;
        case 2:
        case 12:
            nCount = _gMsg28xx_SHORT_N2_TEST_NUMBER;            
            memcpy(pTestPins, _gMsg28xx_SHORT_N2_TEST_PIN, sizeof(u16) * nCount);
            break;
        case 3:
        case 13:
            nCount = _gMsg28xx_SHORT_S1_TEST_NUMBER;            
            memcpy(pTestPins, _gMsg28xx_SHORT_S1_TEST_PIN, sizeof(u16) * nCount);
            break;
        case 4:
        case 14:
            nCount = _gMsg28xx_SHORT_S2_TEST_NUMBER;            
            memcpy(pTestPins, _gMsg28xx_SHORT_S2_TEST_PIN, sizeof(u16) * nCount);
            break;
    
        case 5:
        case 15:
            if(_gMsg28xx_SHORT_TEST_5_TYPE != 0)
            {
                nCount = _gMsg28xx_SHORT_X_TEST_NUMBER;            
                if (_gMsg28xx_PATTERN_TYPE == 5)
                {
                    memcpy(pTestPins, _gMsg28xx_SHORT_GR_TEST_PIN, sizeof(u16) * nCount);
                }
                else
                {
                    memcpy(pTestPins, _gMsg28xx_SHORT_X_TEST_PIN, sizeof(u16) * nCount);
                }
            }    
            break;

        case 0:
        default:
            return 0;
    }

    for (i = nCount; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        pTestPins[i] = MSG28XX_PIN_NO_ERROR;    //PIN_NO_ERROR
    }
    
    return nCount;
}

static s32 _DrvMpTestItoShortTestMsg28xxJudge(u8 nItemID, /*s8 pNormalTestResult[][2],*/ u16 pTestPinMap[][13], u16 * pTestPinCount)
{
    s32 nRetVal = 0;
    u16 nTestPins[MUTUAL_IC_MAX_CHANNEL_NUM];
    s16 i; 

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    *pTestPinCount = _DrvMpTestItoShortTestMsg28xxReadTestPins(nItemID, nTestPins);
    //_DrvMpTestMutualICDebugShowArray(nTestPins, *pTestPinCount, 16, 10, 8);    
    if (*pTestPinCount == 0)
    {
        if (nItemID == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 0)
        {

        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# TestPinCount = 0 ***\n");       
            return -1;
        }
    }

  /*  
    u16 nCountTestPin = 0;    
    for (i = 0; i < testPins.Length; i++)
    {
        if (pTestPins[i] != 0xFFFF)
            nCountTestPin++;
    }
   */ 
    
    for (i = (nItemID - 1) * 13; i < (13 * nItemID); i++)
    {
        _gMutualICResult[i] = _gMutualICDeltaC[i];
    }
    
    for (i = 0; i < *pTestPinCount; i++)
    {
        pTestPinMap[nItemID][i] = nTestPins[i];    
   	   	if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nItemID - 1) * 13], MSG28XX_SHORT_VALUE, -1000))    //0: false   1: true
        {
            //pNormalTestResult[nItemID][0] = -1;    //-1: failed   0: success 
            //                         //0: golden   1: ratio
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# ShortTestMsg28xxJudge failed! ***\n");             
            nRetVal = -1;
        }
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# nItemID = %d ***\n", nItemID);        
    //_DrvMpTestMutualICDebugShowArray(pTestPinMap[nItemID], *pTestPinCount, 16, 10, 8);
    
    return nRetVal;
}

static s32 _DrvMpTestItoShortTestMsg28xxOnCellJudge(u8 nItemID, /*s8 pNormalTestResult[][2],*/ u16 pTestPinMap[][13], s8 *TestFail, u16*pTestPinCount)
{
    s32 nRetVal = 0;
    u16 nTestPins[MUTUAL_IC_MAX_CHANNEL_NUM];
    s16 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    *pTestPinCount = _DrvMpTestItoShortTestMsg28xxReadTestPins(nItemID, nTestPins);
    //_DrvMpTestMutualICDebugShowArray(nTestPins, *pTestPinCount, 16, 10, 8);
    if (*pTestPinCount == 0)
    {
        if (nItemID == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 0)
        {

        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# TestPinCount = 0 ***\n");
            return -1;
        }
    }

    for (i = (nItemID - 1) * 13; i < (13 * nItemID); i ++)
    {
        _gMutualICResult[i] = _gMutualICDeltaC[i];
    }

    for (i = 0; i < *pTestPinCount; i++)
    {
        pTestPinMap[nItemID][i] = nTestPins[i];

//	   	   	if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nItemID - 1) * 13], MSG28XX_SHORT_VALUE, -1000))    //0: false   1: true
//	        if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nItemID - 1) * 13], _gMsg28xx_SHORT_VALUE, -1000))    //0: false   1: true
        if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nItemID - 1) * 13], _gMsg28xx_SHORT_VALUE, -_gMsg28xx_SHORT_VALUE))    //0: false   1: true
        {
            //pNormalTestResult[nItemID][0] = -1;    //-1: failed   0: success
            //                         //0: golden   1: ratio
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# ShortTestMsg28xxJudge failed! ***\n");
            TestFail[nItemID] = 1;
            nRetVal = -1;
        }
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# nItemID = %d ***\n", nItemID);
    //_DrvMpTestMutualICDebugShowArray(pTestPinMap[nItemID], *pTestPinCount, 16, 10, 8);

    return nRetVal;
}

static s32 _DrvMpTestItoShortTestMsg28xxCovertRValue(s32 nValue)
{
   	if (nValue >= MUTUAL_IC_IIR_MAX)
   	{
   	   	return 0;
   	}

    //return ((3.53 - 1.3) * 10 / (50 * (((float)nValue - 0 ) / 32768 * 1.1)));
    return 223 * 32768 / (nValue * 550);
}

static s32 _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(s32 deltaR)
{
	DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

	if (deltaR >= MUTUAL_IC_IIR_MAX)
    {
		return 0;
    }

	if (deltaR == 0)
    {
	    deltaR = 1;
    }

//	   	return ((3.72 - 1.3) * 2.15 * 32768 * 1.1 / (50 * (((double)deltaR - 0))));
    return 187541 / (50 * deltaR);
}

static void _DrvMpTestIcPinShortTestMsg28xxOnCellPrepareAna(void)
{
	DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

	// Stop mcu
    RegSet16BitValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

	//set Subframe = 6 ; Sensor = 13
	RegSetLByteValue(0x130A, 0x6D);
	RegSetLByteValue(0x1103, 0x06);
	RegSetLByteValue(0x1016, 0x0C);

	RegSetLByteValue(0x1104, 0x0C);
	RegSetLByteValue(0x100C, 0x0C);
	RegSetLByteValue(0x1B10, 0x0C);

	// adc analog+digital pipe delay, 60= 13 AFE.
	RegSetLByteValue(0x102F, 0x60);

	//trim: Fout 52M &  1.2V
	RegSet16BitValue(0x1420, 0xA55A);//password
	RegSet16BitValue(0x1428, 0xA55A);//password
	RegSet16BitValue(0x1422, 0xFC4C);//go

    _DrvMpTestItoShortTestMsg28xxSetNoiseSensorMode(1);
    _DrvMpTestItoShortTestMsg28xxAnaFixPrs(3);
    _DrvMpTestItoTestMsg28xxAnaChangeCDtime(0x007E, 0x0006);

	// all AFE Cfb use defalt (50p)
	RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
	RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)

	// reg_afe_icmp disenable
	RegSet16BitValue(0x1552, 0x0000);

	// reg_hvbuf_sel_gain
	RegSet16BitValue(0x1564, 0x0077);

	//ADC: AFE Gain bypass
	RegSet16BitValue(0x1260, 0x1FFF);

	//reg_sel_ros disenable
	RegSet16BitValue(0x156A, 0x0000);

	//reg_adc_desp_invert disenable
	RegSetLByteValue(0x1221, 0x00);

	//AFE gain = 1X
	RegSet16BitValue(0x1318, 0x4440);
	RegSet16BitValue(0x131A, 0x4444);

	RegSet16BitValueByAddressMode(0x1012, 0x0680, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1022, 0x0000, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x110A, 0x0104, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1310, 0x04F1, ADDRESS_MODE_16BIT);

	RegSet16BitValueByAddressMode(0x1317, 0x04F1, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1432, 0x0000, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1435, 0x0C00, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1538, 0x0400, ADDRESS_MODE_16BIT);

	RegSet16BitValueByAddressMode(0x1540, 0x0012, ADDRESS_MODE_16BIT);//
	RegSet16BitValueByAddressMode(0x1530, 0x0133, ADDRESS_MODE_16BIT);//HI v buf enable
//		RegSet16BitValueByAddressMode(0x1533, 0x0522, ADDRESS_MODE_16BIT);//low v buf gain
    RegSet16BitValueByAddressMode(0x1533, 0x0000, ADDRESS_MODE_16BIT);//low v buf gain
	RegSet16BitValueByAddressMode(0x1E11, 0x8000, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x2003, 0x007E, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x2006, 0x137F, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x213E, 0x1FFF, ADDRESS_MODE_16BIT);

	// re-set sample and coefficient
	RegSet16BitValueByAddressMode(0x100D, 0x0020, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1103, 0x0020, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1104, 0x0020, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1302, 0x0020, ADDRESS_MODE_16BIT);
	RegSet16BitValueByAddressMode(0x1B30, 0x0020, ADDRESS_MODE_16BIT);

	// coefficient
	RegSet16BitValueByAddressMode(0x136B, 0x10000 / 0x0020, ADDRESS_MODE_16BIT);// 65536/ sample
}

static void _DrvMpTestIcPinShortTestMsg28xxOnCellChangeANASetting(void)
{
    int i, nMappingItem;
    u8 nChipVer;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x01);

    nChipVer = RegGetLByteValue(0x1ECE);

    if (nChipVer != 0)
    {
        RegSetLByteValue(0x131E, 0x01);
    }

    memcpy(_gMsg28xxMuxMem_20_3E_1_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_1_Settings, sizeof(_gMsg28xxMuxMem_20_3E_1_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_2_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_2_Settings, sizeof(_gMsg28xxMuxMem_20_3E_2_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_3_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_3_Settings, sizeof(_gMsg28xxMuxMem_20_3E_3_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_4_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_4_Settings, sizeof(_gMsg28xxMuxMem_20_3E_4_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_5_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_5_Settings, sizeof(_gMsg28xxMuxMem_20_3E_5_Settings));

    for (nMappingItem = 0; nMappingItem < 6; nMappingItem ++)
    {
        /// sensor mux sram read/write base address / write length
        RegSetLByteValue(0x2192, 0x00);
        RegSetLByteValue(0x2102, 0x01);
        RegSetLByteValue(0x2102, 0x00);
        RegSetLByteValue(0x2182, 0x08);
        RegSetLByteValue(0x2180, 0x08 * nMappingItem);
        RegSetLByteValue(0x2188, 0x01);

        for (i = 0; i < 8; i ++)
        {
            if (nMappingItem == 0 && nChipVer == 0x0)
            {
                memset(_gMsg28xxMuxMem_20_3E_0_Settings, 0, sizeof(_gMsg28xxMuxMem_20_3E_0_Settings));
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_0_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_0_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_0_Settings[2 * i + 1]);
            }

            if ((nMappingItem == 1 && nChipVer == 0x0) || (nMappingItem == 0 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_1_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_1_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_1_Settings[2 * i + 1]);
            }

            if ((nMappingItem == 2 && nChipVer == 0x0) || (nMappingItem == 1 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_2_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_2_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_2_Settings[2 * i + 1]);
            }

            if ((nMappingItem == 3 && nChipVer == 0x0) || (nMappingItem == 2 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_3_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_3_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_3_Settings[2 * i + 1]);
            }

            if ((nMappingItem == 4 && nChipVer == 0x0) || (nMappingItem == 3 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_4_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_4_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_4_Settings[2 * i + 1]);
            }

            if ((nMappingItem == 5 && nChipVer == 0x0) || (nMappingItem == 4 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_5_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_5_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_5_Settings[2 * i + 1]);
            }

            if (nMappingItem == 5 && nChipVer != 0x0)
            {
                memset(_gMsg28xxMuxMem_20_3E_6_Settings, 0, sizeof(_gMsg28xxMuxMem_20_3E_6_Settings));
                RegSet16BitValue(0x218A, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i + 1]);
                DBG(&g_I2cClient->dev, "_gMsg28xxMuxMem_20_3E_6_Settings[%d] = %x, _gMsg28xxMuxMem_20_3E_6_Settings[%d] = %x\n", 2*i, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i], 2*i+1, _gMsg28xxMuxMem_20_3E_6_Settings[2 * i + 1]);
            }
        }
    }
}

static void _DrvMpTestIcPinShortTestMsg28xxOnCellAnaLoadSetting(void)
{
    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x01);
	_DrvMpTestIcPinShortTestMsg28xxOnCellChangeANASetting();
}

static s32 _DrvMpTestIcPinShortTestMsg28xxOnCellIcPinShort(void)
{
    s16 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _DrvMpTestIcPinShortTestMsg28xxOnCellPrepareAna();
    _DrvMpTestItoShortTestMsg28xxOnCellSetSensorPADState(POS_PULSE);

    //DAC overwrite
    RegSet16BitValue(0x150C, 0x8040); //bit15 //AFE:1.3v for test

    _DrvMpTestIcPinShortTestMsg28xxOnCellAnaLoadSetting();
    _DrvMpTestItoTestMsg28xxAnaSwReset();
    _DrvMpTestItoShortTestMsg28xxOnCellpatchFWAnaSetting_for_shorttest();

    memset(_gMutualICDeltaC, 0, sizeof(_gMutualICDeltaC));

    if (_DrvMpTestItoShortTestMsg28xxGetValueR(_gMutualICDeltaC)<0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx IC Pin Short Test# GetValueR failed! ***\n");
        return -1;
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx IC Pin Short Test# GetValueR 1.3v! ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC, 65, -32, 10, 13);

    memset(_gMutualICDeltaC2, 0, sizeof(_gMutualICDeltaC2));

    ///DAC overwrite
    RegSet16BitValue(0x150C, 0x8083); //bit15 //AFE:3.72v for test

    if (_DrvMpTestItoShortTestMsg28xxGetValueR(_gMutualICDeltaC2)<0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# GetValueR failed! ***\n");
        return -1;
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx IC Pin Short Test# GetValueR 3.72v! ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC2, 65, -32, 10, 13);

    for (i = 0; i < 65; i ++) // 13 AFE * 5 subframe
    {
        if ((abs(_gMutualICDeltaC2[i]) < MSG28XX_IIR_MAX) && (abs(_gMutualICDeltaC[i]) < MSG28XX_IIR_MAX))
        {
            _gMutualICDeltaC[i] = _gMutualICDeltaC2[i] - _gMutualICDeltaC[i];
        }
        else
        {
            _gMutualICDeltaC[i] = _gMutualICDeltaC2[i];
        }

//	        if (_gMutualICDeltaC[i] <= -1000 || _gMutualICDeltaC[i] >= (MSG28XX_IIR_MAX)) // -1000 is fout bias issue
        if (_gMutualICDeltaC[i] >= (MSG28XX_IIR_MAX))
        {
            _gMutualICDeltaC[i] = 0x7FFF;
        }
        else
        {
            _gMutualICDeltaC[i] = abs(_gMutualICDeltaC[i]);
        }
    }

    DBG(&g_I2cClient->dev, "*** Msg28xx IC Pin Short Test# GetValueR 3.72v - 1.3v ! ***\n");
    _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaC, 65, -32, 10, 13);

    return 0;
}

static s32 _DrvMpTestIcPinShortTestMsg28xxOnCellReadMapping(u16 testPin_data[][13])
{
    int i, j, nMappingItem;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    memcpy(_gMsg28xxMuxMem_20_3E_1_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_1_Settings, sizeof(_gMsg28xxMuxMem_20_3E_1_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_2_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_2_Settings, sizeof(_gMsg28xxMuxMem_20_3E_2_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_3_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_3_Settings, sizeof(_gMsg28xxMuxMem_20_3E_3_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_4_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_4_Settings, sizeof(_gMsg28xxMuxMem_20_3E_4_Settings));
	memcpy(_gMsg28xxMuxMem_20_3E_5_Settings, _gMsg28xx_SHORT_IcPIN_MUX_MEM_5_Settings, sizeof(_gMsg28xxMuxMem_20_3E_5_Settings));

	for (nMappingItem = 1; nMappingItem < 6; nMappingItem ++)
    {
		u16 testpin = 1;
        u16 index = 0;

        for (i = 0; i < sizeof(_gMsg28xxMuxMem_20_3E_1_Settings) / sizeof(_gMsg28xxMuxMem_20_3E_1_Settings[0]); i++)
        {
            for (j = 0; j < 4; j ++)
            {
                if (nMappingItem == 1)
                {
                    if (((_gMsg28xxMuxMem_20_3E_1_Settings[i] >> (4 * j)) & 0x0F) != 0)
                    {
                        testPin_data[nMappingItem][index] = testpin;
                        index++;
                    }
                }
                else if (nMappingItem == 2)
                {
                    if (((_gMsg28xxMuxMem_20_3E_2_Settings[i] >> (4 * j)) & 0x0F) != 0)
                    {
                        testPin_data[nMappingItem][index] = testpin;
                        index++;
                    }
                }
                else if (nMappingItem == 3)
                {
                    if (((_gMsg28xxMuxMem_20_3E_3_Settings[i] >> (4 * j)) & 0x0F) != 0)
                    {
                        testPin_data[nMappingItem][index] = testpin;
                        index++;
                    }
                }
                else if (nMappingItem == 4)
                {
                    if (((_gMsg28xxMuxMem_20_3E_4_Settings[i] >> (4 * j)) & 0x0F) != 0)
                    {
                        testPin_data[nMappingItem][index] = testpin;
                        index++;
                    }
                }
                else if (nMappingItem == 5)
                {
                    if (((_gMsg28xxMuxMem_20_3E_5_Settings[i] >> (4 * j)) & 0x0F) != 0)
                    {
                        testPin_data[nMappingItem][index] = testpin;
                        index++;
                    }
                }
                testpin++;
            } // for (j = 0; j < 4; j++)
        } // for (i = 0; i < MUX_MEM_1_Settings.Length; i++)
    } // for (nMappingItem = 1; nMappingItem < 6; nMappingItem++)

	return 1;
}

static s32 _DrvMpTestIcPinShortTestMsg28xxOnCellShortTestJudge(u16 nItemID, u16 pTestPinMap[][13], s8 *TestFail)
{
	int nRet = 1, i, count_test_pin = 0, j;
	u16 GRPins[13] = {0}, GR_Id[13] = {0};
	int found_gr = 0, count = 0, BypassGR = 0;

	DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

	if (!_DrvMpTestIcPinShortTestMsg28xxOnCellReadMapping(pTestPinMap))
    {
	    return 0;
    }

	if (_gMsg28xx_GR_NUM == 0)
    {
		BypassGR = 1;
    }

	for (i = 0; i < sizeof(GRPins) / sizeof(u16); i++)
    {
		GRPins[i] = 0xFFFF;
	}

    for (i = 0; i < _gMsg28xx_SHORT_X_TEST_NUMBER; i++)
    {
		GRPins[i] = _gMsg28xx_SHORT_GR_TEST_PIN[i];
	}
    DBG(&g_I2cClient->dev, "GRPins[0]= %d\n", GRPins[0]);

	if (_gMsg28xx_SHORT_X_TEST_NUMBER)
    {
        DBG(&g_I2cClient->dev, "*** %s()_1 ***\n", __func__);
		for (j = 0; j < sizeof(GRPins) / sizeof(u16); j++)
        {
            DBG(&g_I2cClient->dev, "*** %s()_2 ***\n", __func__);

			if (GRPins[j] == 0xFFFF)
            {
				continue;
            }

			for (i = 0; i < 13; i++)
            {
                DBG(&g_I2cClient->dev, "pTestPinMap[%d][%d] = %d\n", nItemID, i, pTestPinMap[nItemID][i]);

				if ((pTestPinMap[nItemID][i] != MSG28XX_UN_USE_PIN) && (pTestPinMap[nItemID][i] != 0))
                {
                    DBG(&g_I2cClient->dev, "pTestPinMap[%d][%d] = %d\n", nItemID, i, pTestPinMap[nItemID][i]);
					if (pTestPinMap[nItemID][i] == (GRPins[j] + 1))
                    {
						DBG(&g_I2cClient->dev, "pTestPinMap[%d][%d] = %d\n", nItemID, i, pTestPinMap[nItemID][i]);
						GR_Id[count] = i;
				        found_gr = 1;

				        if (count < sizeof(GR_Id) / sizeof(u16))
                        {
                            count++;
                        }
				    }
				}
				else
                {
					break;
                }
			}
		}
	}

	count = 0;

	for (i = (nItemID - 1) * 13; i < (13 * nItemID); i++)
    {
		if ((found_gr == 1) && (i == GR_Id[count] + ((nItemID - 1) * 13)) && (BypassGR == 1))
        {
			_gMutualICDeltaC[i] = 1;

			if (count < sizeof(GR_Id) / sizeof(u16))
            {
				count++;
            }
		}
	}

	for (i = 0; i < sizeof(pTestPinMap) / sizeof(pTestPinMap[0]); i++)
    {
		if (pTestPinMap[nItemID][i] != MSG28XX_PIN_NO_ERROR)
        {
			count_test_pin++;
        }
	}

	for (i = 0; i < count_test_pin; i++)
    {
//			if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICDeltaC[i + (nItemID - 1) * 13], _gMsg28xx_ICPIN_SHORT, -1000))
		if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICDeltaC[i + (nItemID - 1) * 13], _gMsg28xx_ICPIN_SHORT, -_gMsg28xx_ICPIN_SHORT))
        {
            TestFail[nItemID] = 1;
        }
	}

	return nRet;
}

static s32 _DrvMpTestIcPinShortTestMsg28xxOnCellResultPrepare(s32 thrs, u16 *senseR)
{
	u16 count = 0, i, nRet = 0;

	DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
        _gIcPinShortFailChannel[i] = 0;

        if (_gMsg28xx_SENSE_PAD_PIN_MAPPING[i] != 0)
        {
            DBG(&g_I2cClient->dev, "pICPinChannel[%d] = %d, IC Pin P[%d] = %dM, deltaC = %d\n", i, _gMsg28xx_SENSE_PAD_PIN_MAPPING[i], _gMsg28xx_SENSE_PAD_PIN_MAPPING[i], senseR[i], _gIcPinShortResultData[i]);

            if (senseR[i] < thrs)
            {
                _gIcPinShortFailChannel[i] = _gMsg28xx_SENSE_PAD_PIN_MAPPING[i];
                DBG(&g_I2cClient->dev, "IC Pin Fail P%d = %dM", _gMsg28xx_SENSE_PAD_PIN_MAPPING[i], senseR[i]);
                count++;
                nRet = -1;
            }
        }
    } // for (int i = 0; i < senseR.Length; i++)

    return nRet;
}

static s32 _DrvMpTestIcPinShortTestMsg28xxOnCellIcPinShortTestEntry(void)
{
    u8 nFwMode = MUTUAL_SINGLE_DRIVE;
    s16 i = 0, j = 0, count_test_pin = 0;
    u16 *pPad2Drive = NULL;
    u16 *pPad2Sense = NULL;
    u16 *pDriveR = NULL;
    u16 *pSenseR = NULL;
    u16 *pGRR = NULL;
    u16 nTime = 0;
    u16 nTestItemLoop = 6;
    u16 nTestItem = 0;
    u16 aTestPinMap[6][13] = {{0}};        //6:max subframe    13:max afe
    u32 nRetVal = 0;
    s32 thrs = 0;
    s8 aNormalTestFail[MSG28XX_TEST_ITEM_NUM] = {0};    //0:golden    1:ratio

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        DrvPlatformLyrTouchDeviceResetHw();
        DrvPlatformLyrEnableFingerTouchReport();
        return -2;
    }

    pPad2Drive = kzalloc(sizeof(s16) * _gMsg28xx_DRIVE_NUM, GFP_KERNEL);
    pPad2Sense = kzalloc(sizeof(s16) * _gMsg28xx_SENSE_NUM, GFP_KERNEL);
    pDriveR = kzalloc(sizeof(s16) * _gMsg28xx_DRIVE_NUM, GFP_KERNEL);
    pSenseR = kzalloc(sizeof(s16) * _gMsg28xx_SENSE_NUM, GFP_KERNEL);
    pGRR = kzalloc(sizeof(s16) * _gMsg28xx_GR_NUM, GFP_KERNEL);

    _gMutualICSenseLineNum = _gMsg28xx_SENSE_NUM;
    _gMutualICDriveLineNum = _gMsg28xx_DRIVE_NUM;

_RETRY_SHORT:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset only
//	    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
//	    DbBusWaitMCU();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00);

    if (_DrvMpTestMsg28xxItoTestSwitchFwMode(&nFwMode) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _RETRY_SHORT;
        }
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Switch Fw Mode failed! ***\n");
        _gIcPinShortCheckFail = 1;
        nRetVal = -1;

        goto ITO_TEST_END;
    }

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073

    for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
    {
		 pSenseR[i] = MSG28XX_PIN_NO_ERROR;
	}

    thrs = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMsg28xx_ICPIN_SHORT);

    for (i = 1; i < 6; i++)
    {//max 6 subframe
        for (j = 0; j < 13; j++)
        {// max 13 AFE
            if (((i-1) * 13 + j) < MUTUAL_IC_MAX_CHANNEL_NUM)	// prevent heap corruption detected
            {
                _gIcPinShortFailChannel[(i-1) * 13 + j] = MSG28XX_UN_USE_PIN;
            }

            aTestPinMap[i][j] = MSG28XX_UN_USE_PIN;
        }
    }

    if (_DrvMpTestIcPinShortTestMsg28xxOnCellIcPinShort() < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Get DeltaC failed! ***\n");
        _gIcPinShortCheckFail = 1;
        nRetVal = -1;
        goto ITO_TEST_END;
    }

    for (nTestItem = 1; nTestItem < nTestItemLoop; nTestItem++)
    {
        DBG(&g_I2cClient->dev, "*** Short test item %d ***\n", nTestItem);
        if (_DrvMpTestIcPinShortTestMsg28xxOnCellShortTestJudge(nTestItem, /*aNormalTestResult,*/ aTestPinMap, aNormalTestFail) < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Item %d is failed! ***\n", nTestItem);
            _gIcPinShortCheckFail = 1;
            nRetVal = -1;
            goto ITO_TEST_END;
        }

        count_test_pin = 0;

	    for (i = 0; i < 13; i++)
        {
            if (aTestPinMap[nTestItem][i] != MSG28XX_UN_USE_PIN)
            {
                //DEBUG("normalTestFail_check[%d, %d] = %d", nTestItem, i, testPin_data[nTestItem][i]);
                count_test_pin++;
            }
	    }

        if ((nTestItem > 0) && (nTestItem < 6))
	    {
            for (i = 0; i < count_test_pin; i++)
            {
                if ((aTestPinMap[nTestItem][i] > 0) && (aTestPinMap[nTestItem][i] <= MUTUAL_IC_MAX_CHANNEL_NUM))
                {
                    pSenseR[aTestPinMap[nTestItem][i] - 1] = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMutualICDeltaC[i + (nTestItem - 1) * 13]);
                    _gIcPinShortResultData[aTestPinMap[nTestItem][i] - 1] = _gMutualICDeltaC[i + (nTestItem - 1) * 13];
	                DBG(&g_I2cClient->dev, "pSenseR[%d]={%d}, _gDeltaC[{%d}]={%d}", aTestPinMap[nTestItem][i] - 1, pSenseR[aTestPinMap[nTestItem][i] - 1], i + (nTestItem - 1) * 13, _gMutualICDeltaC[i + (nTestItem - 1) * 13]);
                }
	            else
                {
                    nRetVal = -1;
                    _gIcPinShortCheckFail = 1;	// ic pin short fail
                    goto ITO_TEST_END;
	            }
            }
	    } // if ((nTestItem > 0) && (nTestItem < 6))
    }

    if (_DrvMpTestIcPinShortTestMsg28xxOnCellResultPrepare(thrs, pSenseR) < 0)
    {
        _gIcPinShortCheckFail  = 1;	// ic pin short fail
        nRetVal = -1;
    }

	memcpy(_gIcPinShortSenceR, pSenseR, sizeof(_gIcPinShortSenceR));

    DBG(&g_I2cClient->dev, "*** %s()_gIcPinShortFailChannel:  ***\n", __func__);
    _DrvMpTestMutualICDebugShowArray(_gIcPinShortFailChannel, 60, -32, 10, 10);
    DBG(&g_I2cClient->dev, "*** %s()_gIcPinShortResultData:  ***\n", __func__);
    _DrvMpTestMutualICDebugShowArray(_gIcPinShortResultData, 60, -32, 10, 10);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();
    kfree(pPad2Sense);
    kfree(pPad2Drive);

    return nRetVal;
}

static s32 _DrvMpTestItoShortTestMsg28xxOnCellItoShortTestEntry(void)
{
    u8 nFwMode = MUTUAL_SINGLE_DRIVE;
    s16 i = 0, j = 0;
    u16 *pPad2Drive = NULL;
    u16 *pPad2Sense = NULL;
    u16 *pDriveR = NULL;
    u16 *pSenseR = NULL;
    u16 *pGRR = NULL;
    u16 nTime = 0;
    u16 aPad2GR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};

    u16 nTestItemLoop = 6;
    u16 nTestItem = 0;
    u16 aTestPinMap[6][13] = {{0}};        //6:max subframe    13:max afe
    u16 nTestPinNum = 0;
    u32 nRetVal = 0;
    s32 thrs = 0;
    s8 aNormalTestFail[MSG28XX_TEST_ITEM_NUM] = {0};    //0:golden    1:ratio

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        DrvPlatformLyrTouchDeviceResetHw();
        DrvPlatformLyrEnableFingerTouchReport();
        return -2;
    }

    pPad2Drive = kzalloc(sizeof(s16) * _gMsg28xx_DRIVE_NUM, GFP_KERNEL);
    pPad2Sense = kzalloc(sizeof(s16) * _gMsg28xx_SENSE_NUM, GFP_KERNEL);
    pDriveR = kzalloc(sizeof(s16) * _gMsg28xx_DRIVE_NUM, GFP_KERNEL);
    pSenseR = kzalloc(sizeof(s16) * _gMsg28xx_SENSE_NUM, GFP_KERNEL);
    pGRR = kzalloc(sizeof(s16) * _gMsg28xx_GR_NUM, GFP_KERNEL);

    _gMutualICSenseLineNum = _gMsg28xx_SENSE_NUM;
    _gMutualICDriveLineNum = _gMsg28xx_DRIVE_NUM;

_RETRY_SHORT:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset only
//	    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
//	    DbBusWaitMCU();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    // Start mcu
    RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073

    if (_DrvMpTestMsg28xxItoTestSwitchFwMode(&nFwMode) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _RETRY_SHORT;
        }
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Switch Fw Mode failed! ***\n");
        _gItoShortCheckFail = 1;
        nRetVal = -1;

        goto ITO_TEST_END;
    }

    DbBusStopMCU();

    _DrvMpTestMsg28xxItoReadSetting(pPad2Sense, pPad2Drive, aPad2GR);

    thrs = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMsg28xx_SHORT_VALUE);

	for (i = 0; i < _gMutualICSenseLineNum; i++)
    {
        pSenseR[i] = thrs;
    }
	for (i = 0; i < _gMutualICDriveLineNum; i++)
    {
        pDriveR[i] = thrs;
    }
	for (i = 0; i < _gMsg28xx_GR_NUM; i++)
    {
        pGRR[i] = thrs;
    }

    for (i = 1; i < 6; i++)
    {//max 6 subframe
		 for (j = 0; j < 13; j++)
         {// max 13 AFE
            if (((i-1) * 13 + j) < MUTUAL_IC_MAX_CHANNEL_NUM)
            {
                _gItoShortFailChannel[(i-1) * 13 + j] = (u32)MSG28XX_UN_USE_PIN;
            }
            aTestPinMap[i][j] = MSG28XX_UN_USE_PIN;
		 }
	 }

	//N1_ShortTest
    if (_DrvMpTestMsg28xxItoShortTest(1) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Get DeltaC failed! ***\n");
        _gItoShortCheckFail = 1;
        nRetVal = -1;
        goto ITO_TEST_END;
    }

    for (nTestItem = 1; nTestItem < nTestItemLoop; nTestItem++)
    {
        DBG(&g_I2cClient->dev, "*** Short test item %d ***\n", nTestItem);
        if (_DrvMpTestItoShortTestMsg28xxOnCellJudge(nTestItem, /*aNormalTestResult,*/ aTestPinMap, aNormalTestFail, &nTestPinNum) < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Item %d is failed! ***\n", nTestItem);
            nRetVal = -1;
            goto ITO_TEST_END;
        }

        if (nTestItem == 1 || nTestItem == 2 || (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 1))
        {
            DBG(&g_I2cClient->dev, "SHORT_TEST_N3");
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMutualICSenseLineNum; j++)
                {
                    if (aTestPinMap[nTestItem][i] == pPad2Sense[j])
                    {
                        //_gMutualICSenseR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
//	                        _gMutualICSenseR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited

                        pSenseR[j] = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        DBG(&g_I2cClient->dev, "senseR[%d] = %d, _gDeltaC[%d] = %d\n", j, pSenseR[j], i + (nTestItem - 1) * 13, _gMutualICResult[i + (nTestItem - 1) * 13]);

                        _gItoShortRData[j] = pSenseR[j];
                        _gItoShortResultData[j] = _gMutualICResult[i + (nTestItem - 1) * 13];

                        if (pSenseR[j] >= 10)
                        {
                            _gItoShortRData[j] = 10;
                        }

//		        			if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -1000))
                        if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -_gMsg28xx_SHORT_VALUE))
                        {
                            _gItoShortFailChannel[j] = (u32)aTestPinMap[nTestItem][i];
                            //DEBUG("Ito Short senseR, count_test_pin = %d, normalTestFail_check[%d][%d] = %d, pShortFailChannel[%d] = %d, _gDeltaC[%d] = %d", count_test_pin, nTestItem, i, normalTestFail_check[nTestItem][i], j, ptMutualMpTestResult->pShortFailChannel[j], i + (nTestItem - 1) * 13, _gDeltaC[i + (nTestItem - 1) * 13]);
                        }
                    }
                }
            }
        }

        if (nTestItem == 3 || nTestItem == 4 || (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 2))
        {
            DBG(&g_I2cClient->dev, "SHORT_TEST_S3");
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMutualICDriveLineNum; j++)
                {
                    if (aTestPinMap[nTestItem][i] == pPad2Drive[j])
                    {
                        //_gMutualICDriveR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
//	                        _gMutualICDriveR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited

                        pDriveR[j] = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        DBG(&g_I2cClient->dev, "driveR[%d] = %d, _gDeltaC[%d] = %d\n", j, pDriveR[j], i + (nTestItem - 1) * 13, _gMutualICResult[i + (nTestItem - 1) * 13]);
                        _gItoShortRData[_gMutualICSenseLineNum + j] = pDriveR[j];
                        _gItoShortResultData[_gMutualICSenseLineNum + j] = _gMutualICResult[i + (nTestItem - 1) * 13];

                        if (pDriveR[j] >= 10)
                        {
                            _gItoShortRData[_gMutualICSenseLineNum + j] = 10;
                        }

//		        			if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -1000))
                        if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -_gMsg28xx_SHORT_VALUE))
                        {
                            _gItoShortFailChannel[_gMutualICSenseLineNum + j] = (u32)aTestPinMap[nTestItem][i];

                            //DEBUG("Ito Short driveR, count_test_pin = %d, normalTestFail_check[%d][%d] = %d, pShortFailChannel[%d] = %d, _gDeltaC[%d] = %d", count_test_pin, nTestItem, i, normalTestFail_check[nTestItem][i], _gSenseNum+j, ptMutualMpTestResult->pShortFailChannel[_gSenseNum+j], i + (nTestItem - 1) * 13, _gDeltaC[i + (nTestItem - 1) * 13]);
                        }
                    }
                }
            }
        }

        if (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 3)
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMsg28xx_GR_NUM; j++)
                {
                    if (aTestPinMap[nTestItem][i] == aPad2GR[j])
                    {
                        //_gMutualICGRR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
//	                        _gMutualICGRR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited

                        pGRR[j] = _DrvMpTestItoShortTestMsg28xxOnCellCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        _gItoShortRData[_gMutualICSenseLineNum + _gMutualICDriveLineNum + j] = pGRR[j];
                        _gItoShortResultData[_gMutualICSenseLineNum + _gMutualICDriveLineNum + j] = _gMutualICResult[i + (nTestItem - 1) * 13];

                        if (pGRR[j] >= 10)
                        {
                            _gItoShortRData[_gMutualICSenseLineNum + _gMutualICDriveLineNum + j] = 10.0;
                        }

//		                    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -1000))
	                    if (0 == _DrvMpTestMutualICCheckValueInRange(_gMutualICResult[i + (nTestItem - 1) * 13], _gMsg28xx_SHORT_VALUE, -_gMsg28xx_SHORT_VALUE))
	                    {
                            _gItoShortFailChannel[_gMutualICSenseLineNum + _gMutualICDriveLineNum + j] = (u32)aTestPinMap[nTestItem][i];
                            //DEBUG("Ito Short GRR, count_test_pin = %d, normalTestFail_check[%d][%d] = %d, pShortFailChannel[%d] = %d, _gDeltaC[%d] = %d", count_test_pin, nTestItem, i, normalTestFail_check[nTestItem][i], _gSenseNum+j, ptMutualMpTestResult->pShortFailChannel[_gSenseNum+j], i + (nTestItem - 1) * 13, _gDeltaC[i + (nTestItem - 1) * 13]);
	                    }
                    }
                }
            }
        }

	    if (aNormalTestFail[nTestItem])
        {
            _gItoShortCheckFail = aNormalTestFail[nTestItem];	// ito short fail
            nRetVal = -1;
        }
    }

    DBG(&g_I2cClient->dev, "*** %s()_gItoShortFailChannel:  ***\n", __func__);
    _DrvMpTestMutualICDebugShowArray(_gItoShortFailChannel, 60, -32, 10, 10);
    DBG(&g_I2cClient->dev, "*** %s()_gItoShortResultData:  ***\n", __func__);
    _DrvMpTestMutualICDebugShowArray(_gItoShortResultData, 60, -32, 10, 10);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();
    kfree(pPad2Sense);
    kfree(pPad2Drive);

    return nRetVal;
}

static s32 _DrvMpTestMsg28xxItoShortTestEntry(void)
{
    u8 nFwMode = MUTUAL_SINGLE_DRIVE;
    //ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK, nRetVal4 = ITO_TEST_OK, nRetVal5 = ITO_TEST_OK;
    s16 i = 0, j = 0;
    //u16 nTestPinCount = 0;
    //s32 nShortThreshold = 0;
    u16 *pPad2Drive = NULL;
    u16 *pPad2Sense = NULL;
    u16 nTime = 0;    
    u16 aPad2GR[MUTUAL_IC_MAX_CHANNEL_NUM] = {0};
    s32 aResultTemp[(MUTUAL_IC_MAX_CHANNEL_SEN+MUTUAL_IC_MAX_CHANNEL_DRV)*2] = {0};

    ///short test1 to 5.
    //u16 nTestPinCount = 0;
    u16 nTestItemLoop = 6;
    u16 nTestItem = 0; 
    //s8 aNormalTestResult[8][2] = {0};    //0:golden    1:ratio
    u16 aTestPinMap[6][13] = {{0}};        //6:max subframe    13:max afe
    u16 nTestPinNum = 0;
//    s32 nThrs = 0;
    u32 nRetVal = 0;
        
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gMsg28xxDeepStandBy = 0;
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
        DrvPlatformLyrTouchDeviceResetHw();    
        DrvPlatformLyrEnableFingerTouchReport();         
        return -2;
    }

    pPad2Drive = kzalloc(sizeof(s16) * _gMsg28xx_DRIVE_NUM, GFP_KERNEL);
    pPad2Sense = kzalloc(sizeof(s16) * _gMsg28xx_SENSE_NUM, GFP_KERNEL);
    _gMutualICSenseLineNum = _gMsg28xx_SENSE_NUM;
    _gMutualICDriveLineNum = _gMsg28xx_DRIVE_NUM;

_RETRY_SHORT:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusWaitMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if (_DrvMpTestMsg28xxItoTestSwitchFwMode(&nFwMode) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _RETRY_SHORT;
        }    
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Switch Fw Mode failed! ***\n");
        nRetVal = -1;

        goto ITO_TEST_END;
    }

    _DrvMpTestMsg28xxItoReadSetting(pPad2Sense, pPad2Drive, aPad2GR);


	//N1_ShortTest    
    if(_DrvMpTestMsg28xxItoShortTest(1) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Get DeltaC failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;      
    }

    for (nTestItem = 1; nTestItem < nTestItemLoop; nTestItem++)
    {
        DBG(&g_I2cClient->dev, "*** Short test item %d ***\n", nTestItem);
        if (_DrvMpTestItoShortTestMsg28xxJudge(nTestItem, /*aNormalTestResult,*/ aTestPinMap, &nTestPinNum) < 0)
        {
            DBG(&g_I2cClient->dev, "*** Msg28xx Short Test# Item %d is failed! ***\n", nTestItem);
            nRetVal = -1;
            goto ITO_TEST_END;  
        }

        if (nTestItem == 1 || nTestItem == 2 || (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 1))
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMutualICSenseLineNum; j++)
                {
                    if (aTestPinMap[nTestItem][i] == pPad2Sense[j])
                    {
                        //_gMutualICSenseR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        _gMutualICSenseR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }

        if (nTestItem == 3 || nTestItem == 4 || (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 2))
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMutualICDriveLineNum; j++)
                {
                    if (aTestPinMap[nTestItem][i] == pPad2Drive[j])
                    {
                        //_gMutualICDriveR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        _gMutualICDriveR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }

        if (nTestItem == 5 && _gMsg28xx_SHORT_TEST_5_TYPE == 3)
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gMsg28xx_GR_NUM; j++)
                {
                    if (aTestPinMap[nTestItem][i] == aPad2GR[j])
                    {
                        //_gMutualICGRR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gMutualICResult[i + (nTestItem - 1) * 13]);
                        _gMutualICGRR[j] = _gMutualICResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }
    }

    for (i = 0; i < _gMutualICSenseLineNum; i++)
    {
        aResultTemp[i] = _gMutualICSenseR[i];
    }

    //for (i = 0; i < _gMutualICDriveLineNum - 1; i++)
    for (i = 0; i < _gMutualICDriveLineNum; i++)
    {
        aResultTemp[i + _gMutualICSenseLineNum] = _gMutualICDriveR[i];
    }

    //nThrs = _DrvMpTestItoShortTestMsg28xxCovertRValue(MSG28XX_SHORT_VALUE);     
    for (i = 0; i < _gMutualICSenseLineNum + _gMutualICDriveLineNum; i++)
    {
        if (aResultTemp[i] == 0)
        {        
            _gMutualICResult[i] = _DrvMpTestItoShortTestMsg28xxCovertRValue(1);
        }
        else
        {
            _gMutualICResult[i] = _DrvMpTestItoShortTestMsg28xxCovertRValue(aResultTemp[i]);
        }
    }

    _DrvMpTestMutualICDebugShowArray(_gMutualICResult, _gMutualICSenseLineNum + _gMutualICDriveLineNum, -32, 10, 8);
    //for (i = 0; i < (_gMutualICSenseLineNum + _gMutualICDriveLineNum - 1); i++)
    for (i = 0; i < (_gMutualICSenseLineNum + _gMutualICDriveLineNum); i++)        
    {
        if (aResultTemp[i] > MSG28XX_SHORT_VALUE)    //change comparison way because float computing in driver is prohibited
        {
            _gMutualICTestFailChannel[i] = 1;
            _gTestFailChannelCount++;
            nRetVal = -1;
        }
        else
        {
            _gMutualICTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();
    kfree(pPad2Sense);
    kfree(pPad2Drive);    

    return nRetVal;
}

static s32 _DrvMpTestItoWaterProofTestMsg28xxTriggerWaterProofOneShot(s16 * pResultData, u32 nDelay)
{
    u16 nAddr = 0x5000, nAddrNextSF = 0x1A4;
    u16 nSF = 0, nAfeOpening = 0, nDriOpening = 0;
    u16 nMaxDataNumOfOneSF = 0;
    u16 nDriMode = 0;
    u16 i;
    u8 nRegData = 0;
    u8 aShotData[390] = {0};//13*15*2
    u16 nRegDataU16 = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nRegData = RegGetLByteValue(0x130A);
    nSF = nRegData>>4;
    nAfeOpening = nRegData & 0x0f;

    if (nSF == 0)
    {
        return -1;
    }

    nRegData = RegGetLByteValue(0x100B);
    nDriMode = nRegData;

    nRegData = RegGetLByteValue(0x1312);
    nDriOpening = nRegData;

    DBG(&g_I2cClient->dev, "*** Msg28xx WaterProof Test# TriggerWaterProofOneShot nSF=%d, nAfeOpening=%d, nDriMode=%d, nDriOpening=%d. ***\n", nSF, nAfeOpening, nDriMode, nDriOpening);

    nMaxDataNumOfOneSF = nAfeOpening * nDriOpening;

    RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK

    ///polling frame-ready interrupt status
    _DrvMpTestItoTestMsg28xxEnableAdcOneShot();

    while (0x0000 == (nRegDataU16 & BIT8))
    {
        nRegDataU16 = RegGet16BitValue(0x3D18);
    }

    RegSet16BitValueOn(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK
    RegSet16BitValueOn(0x3D08, BIT4);      ///FIQ_E_TIMER0_MASK

    if (_gMsg28xx_PATTERN_TYPE == 1) // for short test
    {
        //s16 nShortResultData[nSF][nAfeOpening];

        /// get ALL raw data
        _DrvMpTestItoTestDBBusReadDQMemStart();
        RegGetXBitValue(nAddr, aShotData, 16, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        _DrvMpTestItoTestDBBusReadDQMemEnd();

        //_DrvMpTestMutualICDebugShowArray(aShotData, 26, 8, 16, 16);
        for (i = 0; i < 8; i++)
        {
            pResultData[i] = (s16)(aShotData[2 * i] | aShotData[2 * i + 1] << 8);
        }
    }
    else if(_gMsg28xx_PATTERN_TYPE == 3 || _gMsg28xx_PATTERN_TYPE == 4)// for open test
    {
        //s16 nOpenResultData[nSF * nAfeOpening][nDriOpening];

        if (nSF >4)
        {    
            nSF = 4;
        }

        /// get ALL raw data, combine and handle datashift.
        for (i = 0; i < nSF; i++)
        {
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, aShotData, 16, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();

            //_DrvMpTestMutualICDebugShowArray(aShotData, 390, 8, 10, 16);
            pResultData[2 * i] = (s16)(aShotData[4 * i] | aShotData[4 * i + 1] << 8);
            pResultData[2 * i + 1] = (s16)(aShotData[4 * i + 2] | aShotData[4 * i + 3] << 8);
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

static s32 _DrvMpTestItoWaterProofTesMsg28xxtGetWaterProofOneShotRawIIR(s16 * pRawDataWP, u32 nDelay)
{
    return _DrvMpTestItoWaterProofTestMsg28xxTriggerWaterProofOneShot(pRawDataWP, nDelay);
}

static s32 _DrvMpTestItoWaterProofTestMsg28xxGetDeltaCWP(s32 *pTarget, s8 nSwap, u32 nDelay)
{
    s16 nRawDataWP[12] = {0};
    s16 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if(_DrvMpTestItoWaterProofTesMsg28xxtGetWaterProofOneShotRawIIR(nRawDataWP, nDelay) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx Open Test# GetMutualOneShotRawIIR failed! ***\n");
        return -1;
    }

    for (i = 0; i < _gMutualICWaterProofNum; i++)
    {
        pTarget[i] = nRawDataWP[i];
    }

    return 0;
}

static s32 _DrvMpTestMsg28xxItoWaterProofTest(u32 nDelay)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
    _DrvMpTestItoTestMsg28xxAnaSwReset();

    if (_DrvMpTestItoWaterProofTestMsg28xxGetDeltaCWP(_gMutualICDeltaCWater, -1, nDelay)<0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx WaterProof Test# GetDeltaCWP failed! ***\n");
        return -1;
    }

    _DrvMpTestMutualICDebugShowArray(_gMutualICDeltaCWater, 12, -32, 10, 16);

    return 0;
}

static void _DrvMpTestMsg28xxItoWaterProofTestMsgJudge(void)
{
    int i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < _gMutualICWaterProofNum; i++)
    {
        _gMutualICResultWater[i] =  abs(_gMutualICDeltaCWater[i]);
    }
}

static s32 _DrvMpTestMsg28xxItoWaterProofTestEntry(void)
{
    u8 nFwMode = WATERPROOF;
    s16 i = 0;
    u32 nRetVal = 0;
    u16 nRegDataWP = 0;
    u32 nDelay = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gMutualICWaterProofNum = 12;

    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    //reset only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusWaitMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if (_DrvMpTestMsg28xxItoTestSwitchFwMode(&nFwMode) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx WaterProof Test# Switch FW mode failed! ***\n");
        nRetVal =  -1;
        goto ITO_TEST_END;
    }

    nRegDataWP = RegGet16BitValue(0x1402);
    if(nRegDataWP == 0x8BBD)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx WaterProof Test# FW don't support waterproof! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;         
    }

    if(_DrvMpTestMsg28xxItoWaterProofTest(nDelay) < 0)
    {
        DBG(&g_I2cClient->dev, "*** Msg28xx WaterProof Test# Get DeltaC failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;        
    }

    _DrvMpTestMsg28xxItoWaterProofTestMsgJudge();

    for (i = 0; i < _gMutualICWaterProofNum; i++)
    {
        if (_gMutualICResultWater[i] > MSG28XX_WATER_VALUE)    //change comparison way because float computing in driver is prohibited
        {
            _gMutualICTestFailChannel[i] = 1;
            _gTestFailChannelCount++;
            nRetVal = -1;
        }
        else
        {
            _gMutualICTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

//------------------------------------------------------------------------------//

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
static u8 _DrvMpTestMutualICCheckValueInRange(s32 nValue, s32 nMax, s32 nMin)
{
   	if (nValue <= nMax && nValue >= nMin)
   	{
   	   	return 1;
   	}
   	else
   	{
   	   	return 0;
   	}
}

static void _DrvMpTestMutualICDebugShowArray(void *pBuf, u16 nLen, int nDataType, int nCarry, int nChangeLine)
{
    u8 * pU8Buf = NULL;
    s8 * pS8Buf = NULL;
    u16 * pU16Buf = NULL;
    s16 * pS16Buf = NULL;
    u32 * pU32Buf = NULL;
    s32 * pS32Buf = NULL;
    int i;

    if(nDataType == 8)
        pU8Buf = (u8 *)pBuf;    
    else if(nDataType == -8)
        pS8Buf = (s8 *)pBuf;    
    else if(nDataType == 16)
        pU16Buf = (u16 *)pBuf;    
    else if(nDataType == -16)
        pS16Buf = (s16 *)pBuf;    
    else if(nDataType == 32)
        pU32Buf = (u32 *)pBuf;    
    else if(nDataType == -32)
        pS32Buf = (s32 *)pBuf;    

    for(i=0; i < nLen; i++)
    {
        if(nCarry == 16)    
        {
            if(nDataType == 8)        
                DBG(&g_I2cClient->dev, "%02X ", pU8Buf[i]);
            else if(nDataType == -8)        
                DBG(&g_I2cClient->dev, "%02X ", pS8Buf[i]);
            else if(nDataType == 16)        
                DBG(&g_I2cClient->dev, "%04X ", pU16Buf[i]);
            else if(nDataType == -16)        
                DBG(&g_I2cClient->dev, "%04X ", pS16Buf[i]);
            else if(nDataType == 32)        
                DBG(&g_I2cClient->dev, "%08X ", pU32Buf[i]);            
            else if(nDataType == -32)        
                DBG(&g_I2cClient->dev, "%08X ", pS32Buf[i]);            
        }    
        else if(nCarry == 10)
        {
            if(nDataType == 8)
                DBG(&g_I2cClient->dev, "%6d ", pU8Buf[i]);
            else if(nDataType == -8)
                DBG(&g_I2cClient->dev, "%6d ", pS8Buf[i]);
            else if(nDataType == 16)
                DBG(&g_I2cClient->dev, "%6d ", pU16Buf[i]);
            else if(nDataType == -16)
                DBG(&g_I2cClient->dev, "%6d ", pS16Buf[i]);
            else if(nDataType == 32)
                DBG(&g_I2cClient->dev, "%6d ", pU32Buf[i]);
            else if(nDataType == -32)
                DBG(&g_I2cClient->dev, "%6d ", pS32Buf[i]);
        }
 
        if(i%nChangeLine == nChangeLine-1)
        {  
            DBG(&g_I2cClient->dev, "\n");
        }
    }
    DBG(&g_I2cClient->dev, "\n");    
}

/*
static void _DrvMpTestMutualICDebugShowS32Array(s32 *pBuf, u16 nRow, u16 nCol)
{
    int i, j;

    for(j=0; j < nRow; j++)
    {
        for(i=0; i < nCol; i++)    
        {
            DBG(&g_I2cClient->dev, "%4d ", pBuf[i * nRow + j]);       
        }
        DBG(&g_I2cClient->dev, "\n");
    }
    DBG(&g_I2cClient->dev, "\n");    
}
*/

static s32 _DrvMpTestItoWaterProofTest(void)
{
    s32 nRetVal = -1;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        return _DrvMpTestMsg26xxmItoWaterProofTestEntry();
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        return _DrvMpTestMsg28xxItoWaterProofTestEntry();
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

    return nRetVal;
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

//------------------------------------------------------------------------------//
//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
static int  Mstar_SaveTestData(char *file_name, char *data_buf, int iLen)
{
	struct file *pfile = NULL;
	
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s",  MSTAR_TEST_DATA, file_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_CREAT|O_RDWR, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(pfile, data_buf, iLen, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}
//- bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
 s32 _DrvMpTestItoOpenTest(void)
{
    s32 nRetVal = -1;  
    char *_gTestData= NULL;
    int i=0;
    int len =0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    _gTestData = kmalloc(400, GFP_ATOMIC);
    memset(_gTestData,0,400);
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nRetVal = _DrvMpTestSelfICItoOpenTestEntry();
		//+ bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
		if(nRetVal==0)
			_gRawDataLog[SELF_IC_MAX_CHANNEL_NUM*2]= 1;
		else
			_gRawDataLog[SELF_IC_MAX_CHANNEL_NUM*2]= 2;
		
		for(i=0 ;i <SELF_IC_MAX_CHANNEL_NUM*2+1;i++)
		   {
			len+=sprintf(_gTestData+len ,"%d,",_gRawDataLog[i]);
			if(((2*(i+1))%SELF_IC_MAX_CHANNEL_NUM==0)&&( i !=0))
			    _gTestData[len-1]='\n';
		   }
	
		Mstar_SaveTestData("SelfRawdata.csv", _gTestData, len);
		//- bug 282400 ,zwk.wt,20170802,add new tp openshort test mode 
    }    
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        nRetVal = _DrvMpTestMsg26xxmItoOpenTestEntry();
    }    
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        nRetVal = _DrvMpTestMsg28xxItoOpenTestEntry();
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
 if(NULL != _gTestData) kfree(_gTestData);
    return nRetVal;
}

 s32 _DrvMpTestItoShortTest(void)
{
    s32 nRetVal = -1;    

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nRetVal = _DrvMpTestSelfICItoShortTestEntry();
    }    
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        nRetVal = _DrvMpTestMsg26xxmItoShortTestEntry();
    }    
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        if (!_DrvMpTestMsg28xxItoTestChooseTpType())
        {
            DBG(&g_I2cClient->dev, "Choose Tp Type failed\n");
            nRetVal = -1;
        }

        if (_gMsg28xx_PATTERN_TYPE == 5)
        {
            s32 nRetVal1 = -1, nRetVal2 = -1;

            nRetVal1 = _DrvMpTestItoShortTestMsg28xxOnCellItoShortTestEntry();
            nRetVal2 = _DrvMpTestIcPinShortTestMsg28xxOnCellIcPinShortTestEntry();

            if ((nRetVal1 == -1) || (nRetVal2 == -1))
            {
                nRetVal = -1;
            }
            else
            {
                nRetVal = 0;
            }
        }
        else
        {
            nRetVal = _DrvMpTestMsg28xxItoShortTestEntry();
        }
    }    
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

    return nRetVal; 
}

static void _DrvMpTestItoTestDoWork(struct work_struct *pWork)
{
    s32 nRetVal = ITO_TEST_OK;
    
    DBG(&g_I2cClient->dev, "*** %s() g_IsInMpTest = %d, _gTestRetryCount = %d ***\n", __func__, g_IsInMpTest, _gTestRetryCount);

    if (_gItoTestMode == ITO_TEST_MODE_OPEN_TEST)
    {
        nRetVal = _DrvMpTestItoOpenTest();
    }
    else if (_gItoTestMode == ITO_TEST_MODE_SHORT_TEST)
    {
        nRetVal = _DrvMpTestItoShortTest();
    }
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
    else if (_gItoTestMode == ITO_TEST_MODE_WATERPROOF_TEST)
    {
        nRetVal = _DrvMpTestItoWaterProofTest();
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    else
    {
        DBG(&g_I2cClient->dev, "*** Undefined Mp Test Mode = %d ***\n", _gItoTestMode);
        return;
    }

    DBG(&g_I2cClient->dev, "*** ctp mp test result = %d ***\n", nRetVal);
    
    if (nRetVal == ITO_TEST_OK) //nRetVal == 0
    {
        _gCtpMpTestStatus = ITO_TEST_OK; //PASS
        mutex_lock(&g_Mutex);
        g_IsInMpTest = 0;
        mutex_unlock(&g_Mutex);
	memset(_gRawDataLog,0,sizeof(_gRawDataLog));
        DBG(&g_I2cClient->dev, "mp test success\n");
    }
    else
    {
        _gTestRetryCount --;
        if (_gTestRetryCount > 0)
        {
            DBG(&g_I2cClient->dev, "_gTestRetryCount = %d\n", _gTestRetryCount);
            queue_work(_gCtpMpTestWorkQueue, &_gCtpItoTestWork);
        }
        else
        {
            if (((g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX) && (nRetVal == ITO_TEST_FAIL)) 
            	|| ((g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) && (nRetVal == -1)))
            {
                _gCtpMpTestStatus = ITO_TEST_FAIL;
            }
            else if (((g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX) && (nRetVal == ITO_TEST_GET_TP_TYPE_ERROR)) 
            	|| ((g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) && (nRetVal == -2)))
            {
                _gCtpMpTestStatus = ITO_TEST_GET_TP_TYPE_ERROR;
            }
            else
            {
                _gCtpMpTestStatus = ITO_TEST_UNDEFINED_ERROR;
            }
              
            mutex_lock(&g_Mutex);
            g_IsInMpTest = 0;
            mutex_unlock(&g_Mutex);
            DBG(&g_I2cClient->dev, "mp test failed\n");
        }
	memset(_gRawDataLog,0,sizeof(_gRawDataLog));
    }
}

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
void DrvMpTestGetTestScope(TestScopeInfo_t *pInfo)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) 
    {
        pInfo->nMy = _gMutualICDriveLineNum;
        pInfo->nMx = _gMutualICSenseLineNum;
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        pInfo->nKeyNum = _gMsg28xx_KEY_NUM;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

        DBG(&g_I2cClient->dev, "*** My = %d ***\n", pInfo->nMy);
        DBG(&g_I2cClient->dev, "*** Mx = %d ***\n", pInfo->nMx);
        DBG(&g_I2cClient->dev, "*** KeyNum = %d ***\n", pInfo->nKeyNum);    
    }
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

s32 DrvMpTestGetTestResult(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "_gCtpMpTestStatus = %d\n", _gCtpMpTestStatus);

    return _gCtpMpTestStatus;
}

void DrvMpTestGetTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount)
{
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
    u32 i;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX || CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "_gTestFailChannelCount = %d\n", _gTestFailChannelCount);
    
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX) 
    {
        for (i = 0; i < _gTestFailChannelCount; i ++)
        {
    	      pFailChannel[i] = _gSelfICTestFailChannel[i];
        }

        *pFailChannelCount = _gTestFailChannelCount;
    }
    else
    {
        DBG(&g_I2cClient->dev, "g_ChipType = 0x%x is an undefined chip type.\n", g_ChipType);
        *pFailChannelCount = 0;
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) 
    {
        for (i = 0; i < MUTUAL_IC_MAX_MUTUAL_NUM; i ++)
        {
    	      pFailChannel[i] = _gMutualICTestFailChannel[i];
        }
    
        *pFailChannelCount = MUTUAL_IC_MAX_MUTUAL_NUM; // Return the test result of all channels, APK will filter out the fail channels.
    }
    else
    {
        DBG(&g_I2cClient->dev, "g_ChipType = 0x%x is an undefined chip type.\n", g_ChipType);
        *pFailChannelCount = 0;
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
}

void DrvMpTestGetTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
    if (g_ChipType == CHIP_TYPE_MSG21XXA || g_ChipType == CHIP_TYPE_MSG22XX) 
    {
        u32 i;
        u8 nHighByte, nLowByte;
    
        if (eItoTestMode == ITO_TEST_MODE_OPEN_TEST)
        {
            for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
            {
                nHighByte = (_gSelfICRawData1[i] >> 8) & 0xFF;
                nLowByte = (_gSelfICRawData1[i]) & 0xFF;
    	  
                if (_gSelfICDataFlag1[i] == 1)
                {
                    pDataLog[i*4] = 1; // indicate it is a on-use channel number
                }
                else
                {
                    pDataLog[i*4] = 0; // indicate it is a non-use channel number
                }
            
                if (_gSelfICRawData1[i] >= 0)
                {
                    pDataLog[i*4+1] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[i*4+1] = 1; // - : a negative number
                }

                pDataLog[i*4+2] = nHighByte;
                pDataLog[i*4+3] = nLowByte;
            }

            for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
            {
                nHighByte = (_gSelfICRawData2[i] >> 8) & 0xFF;
                nLowByte = (_gSelfICRawData2[i]) & 0xFF;
        
                if (_gSelfICDataFlag2[i] == 1)
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*4] = 1; // indicate it is a on-use channel number
                }
                else
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*4] = 0; // indicate it is a non-use channel number
                }

                if (_gSelfICRawData2[i] >= 0)
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*4] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*4] = 1; // - : a negative number
                }

                pDataLog[(i*4+2)+SELF_IC_MAX_CHANNEL_NUM*4] = nHighByte;
                pDataLog[(i*4+3)+SELF_IC_MAX_CHANNEL_NUM*4] = nLowByte;
            }

            *pLength = SELF_IC_MAX_CHANNEL_NUM*8;
        }
        else if (eItoTestMode == ITO_TEST_MODE_SHORT_TEST)
        {
            for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
            {
                nHighByte = (_gSelfICRawData1[i] >> 8) & 0xFF;
                nLowByte = (_gSelfICRawData1[i]) & 0xFF;

                if (_gSelfICDataFlag1[i] == 1)
                {
                    pDataLog[i*4] = 1; // indicate it is a on-use channel number
                }
                else
                {
                    pDataLog[i*4] = 0; // indicate it is a non-use channel number
                }

                if (_gSelfICRawData1[i] >= 0)
                {
                    pDataLog[i*4+1] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[i*4+1] = 1; // - : a negative number
                }

                pDataLog[i*4+2] = nHighByte;
                pDataLog[i*4+3] = nLowByte;
            }

            for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
            {
                nHighByte = (_gSelfICRawData2[i] >> 8) & 0xFF;
                nLowByte = (_gSelfICRawData2[i]) & 0xFF;
        
                if (_gSelfICDataFlag2[i] == 1)
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*4] = 1; // indicate it is a on-use channel number
                }
                else
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*4] = 0; // indicate it is a non-use channel number
                }

                if (_gSelfICRawData2[i] >= 0)
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*4] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*4] = 1; // - : a negative number
                }

                pDataLog[i*4+2+SELF_IC_MAX_CHANNEL_NUM*4] = nHighByte;
                pDataLog[i*4+3+SELF_IC_MAX_CHANNEL_NUM*4] = nLowByte;
            }

            for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
            {
                nHighByte = (_gSelfICRawData3[i] >> 8) & 0xFF;
                nLowByte = (_gSelfICRawData3[i]) & 0xFF;
        
                if (_gSelfICDataFlag3[i] == 1)
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*8] = 1; // indicate it is a on-use channel number
                }
                else
                {
                    pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*8] = 0; // indicate it is a non-use channel number
                }

                if (_gSelfICRawData3[i] >= 0)
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*8] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*8] = 1; // - : a negative number
                }

                pDataLog[(i*4+2)+SELF_IC_MAX_CHANNEL_NUM*8] = nHighByte;
                pDataLog[(i*4+3)+SELF_IC_MAX_CHANNEL_NUM*8] = nLowByte;
            }

            if (_gSelfICIsEnable2R)
            {
                for (i = 0; i < SELF_IC_MAX_CHANNEL_NUM; i ++)
                {
                    nHighByte = (_gSelfICRawData4[i] >> 8) & 0xFF;
                    nLowByte = (_gSelfICRawData4[i]) & 0xFF;
        
                    if (_gSelfICDataFlag4[i] == 1)
                    {
                        pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*12] = 1; // indicate it is a on-use channel number
                    }
                    else
                    {
                        pDataLog[i*4+SELF_IC_MAX_CHANNEL_NUM*12] = 0; // indicate it is a non-use channel number
                    }

                    if (_gSelfICRawData4[i] >= 0)
                    {
                        pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*12] = 0; // + : a positive number
                    }
                    else
                    {
                        pDataLog[(i*4+1)+SELF_IC_MAX_CHANNEL_NUM*12] = 1; // - : a negative number
                    }

                    pDataLog[(i*4+2)+SELF_IC_MAX_CHANNEL_NUM*12] = nHighByte;
                    pDataLog[(i*4+3)+SELF_IC_MAX_CHANNEL_NUM*12] = nLowByte;
                }
            }
        
            *pLength = SELF_IC_MAX_CHANNEL_NUM*16;
        }
        else 
        {
            DBG(&g_I2cClient->dev, "*** Undefined MP Test Mode ***\n");
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "g_ChipType = 0x%x is an undefined chip type.\n", g_ChipType);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
    if (g_ChipType == CHIP_TYPE_MSG26XXM || g_ChipType == CHIP_TYPE_MSG28XX) 
    {
        u32 i, j, k;
    
        if (eItoTestMode == ITO_TEST_MODE_OPEN_TEST)
        {
            k = 0;
        
            for (j = 0; j < _gMutualICDriveLineNum; j ++)
//            for (j = 0; j < (_gMutualICDriveLineNum-1); j ++)
            {
                for (i = 0; i < _gMutualICSenseLineNum; i ++)
                {
//                    DBG(&g_I2cClient->dev, "\nDrive%d, Sense%d, Value = %d\t", j, i, _gMutualICResult[i * _gMutualICDriveLineNum + j]); // add for debug

                    if (_gMutualICResult[i * _gMutualICDriveLineNum + j] >= 0)
                    {
                        pDataLog[k*5] = 0; // + : a positive number
                    }
                    else
                    {
                        pDataLog[k*5] = 1; // - : a negative number
                    }

                    pDataLog[k*5+1] = (_gMutualICResult[i * _gMutualICDriveLineNum + j] >> 24) & 0xFF;
                    pDataLog[k*5+2] = (_gMutualICResult[i * _gMutualICDriveLineNum + j] >> 16) & 0xFF;
                    pDataLog[k*5+3] = (_gMutualICResult[i * _gMutualICDriveLineNum + j] >> 8) & 0xFF;
                    pDataLog[k*5+4] = (_gMutualICResult[i * _gMutualICDriveLineNum + j]) & 0xFF;
                
                    k ++;
                }
            }

            DBG(&g_I2cClient->dev, "\nk = %d\n", k);

            *pLength = k*5;
        }
        else if (eItoTestMode == ITO_TEST_MODE_SHORT_TEST)
        {
            k = 0;
        
            for (i = 0; i < (_gMutualICDriveLineNum-1 + _gMutualICSenseLineNum); i++)
            {
                if (_gMutualICResult[i] >= 0)
                {
                    pDataLog[k*5] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[k*5] = 1; // - : a negative number
                }

                pDataLog[k*5+1] = (_gMutualICResult[i] >> 24) & 0xFF;
                pDataLog[k*5+2] = (_gMutualICResult[i] >> 16) & 0xFF;
                pDataLog[k*5+3] = (_gMutualICResult[i] >> 8) & 0xFF;
                pDataLog[k*5+4] = (_gMutualICResult[i]) & 0xFF;
                k ++;
            }

            DBG(&g_I2cClient->dev, "\nk = %d\n", k);

            *pLength = k*5;
        }
        else if (eItoTestMode == ITO_TEST_MODE_WATERPROOF_TEST)
        {
            k = 0;

            for (i = 0; i < _gMutualICWaterProofNum; i++)
            {
                if (_gMutualICResultWater[i] >= 0)
                {
                    pDataLog[k*5] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[k*5] = 1; // - : a negative number
                }

                pDataLog[k*5+1] = (_gMutualICResultWater[i] >> 24) & 0xFF;
                pDataLog[k*5+2] = (_gMutualICResultWater[i] >> 16) & 0xFF;
                pDataLog[k*5+3] = (_gMutualICResultWater[i] >> 8) & 0xFF;
                pDataLog[k*5+4] = (_gMutualICResultWater[i]) & 0xFF;
                k ++;
            }

            DBG(&g_I2cClient->dev, "\nk = %d\n", k);

            *pLength = k*5;
        }
        else 
        {
            DBG(&g_I2cClient->dev, "*** Undefined MP Test Mode ***\n");
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "g_ChipType = 0x%x is an undefined chip type.\n", g_ChipType);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX
}

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
void DrvMpTestGetTestLogAll(u8 *pDataLog, u32 *pLength)
{
    u16 i = 0;
    u8 *pRowStr = "D", *pColStr = "S", *pPinStr = "P", *pItem = NULL;
    u8 *pVersion = NULL;
    u16 nMajor = 0, nMinor = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_gMsg28xx_PATTERN_TYPE == 5)
    {
        DBG(&g_I2cClient->dev, "_gMsg28xx_PATTERN_TYPE = 5\n");

        if (g_IsInMpTest != 0)
        {
            *pLength = *pLength + sprintf(pDataLog + *pLength, "Test is still running!!!\n");
        }
        else
        {
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "Device Driver Version : %s\n", DEVICE_DRIVER_RELEASE_VERSION);
            DrvIcFwLyrGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "Main Block FW Version : %d.%03d\n", nMajor, nMinor);
            DrvIcFwLyrGetCustomerFirmwareVersionByDbBus(EMEM_MAIN, &nMajor, &nMinor, &pVersion);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "Main Block FW Version : %d.%03d\n", nMajor, nMinor);
            DrvIcFwLyrGetCustomerFirmwareVersionByDbBus(EMEM_INFO, &nMajor, &nMinor, &pVersion);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "Info Block FW Version : %d.%03d\n", nMajor, nMinor);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "SupportIC : %d\n",_gMsg28xx_SupportIC);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "DC_Range=%d\n",_gMsg28xx_DC_RANGE);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "DC_Ratio_1000=%d\n",_gMsg28xx_DC_Ratio_1000);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "DC_Border_Ratio_1000=%d\n",_gMsg28xx_DC_Border_Ratio_1000);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "Golden";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < _gMsg28xx_DRIVE_NUM; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d",pRowStr, i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
            {
                if((i % _gMsg28xx_DRIVE_NUM) == 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d",pColStr, (i/_gMsg28xx_DRIVE_NUM) + 1);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannel[i]);
                }
                else if((i % _gMsg28xx_DRIVE_NUM) == (_gMsg28xx_DRIVE_NUM - 1))
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d\n", _gMutualICOnCellOpenTestGoldenChannel[i]);
                }
                else
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannel[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "Golden_Max";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < _gMsg28xx_DRIVE_NUM; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d",pRowStr, i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
            {
                if((i % _gMsg28xx_DRIVE_NUM) == 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d",pColStr, (i/_gMsg28xx_DRIVE_NUM) + 1);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannelMax[i]);
                }
                else if((i % _gMsg28xx_DRIVE_NUM) == (_gMsg28xx_DRIVE_NUM - 1))
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d\n", _gMutualICOnCellOpenTestGoldenChannelMax[i]);
                }
                else
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannelMax[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "Golden_Min";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < _gMsg28xx_DRIVE_NUM; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d",pRowStr, i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
            {
                if((i % _gMsg28xx_DRIVE_NUM) == 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d",pColStr, (i/_gMsg28xx_DRIVE_NUM) + 1);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannelMin[i]);
                }
                else if((i % _gMsg28xx_DRIVE_NUM) == (_gMsg28xx_DRIVE_NUM - 1))
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d\n", _gMutualICOnCellOpenTestGoldenChannelMin[i]);
                }
                else
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestGoldenChannelMin[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "DeltaC";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < _gMsg28xx_DRIVE_NUM; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d",pRowStr, i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
            {
                if((i % _gMsg28xx_DRIVE_NUM) == 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d",pColStr, (i/_gMsg28xx_DRIVE_NUM) + 1);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestResultData[i]);
                }
                else if((i % _gMsg28xx_DRIVE_NUM) == (_gMsg28xx_DRIVE_NUM - 1))
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d\n", _gMutualICOnCellOpenTestResultData[i]);
                }
                else
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestResultData[i]);
                }
            }
            if (_gMutualICOnCellOpenTestResult[0] == 0)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "DeltaC_Result:PASS\n");
            }
            else
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "DeltaC_Result:FAIL\n");
                pItem = "Fail Channel:  ";
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%s",pItem);
                for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
                {
                    if (_gNormalTestFailCheckDeltac[i] == MSG28XX_PIN_NO_ERROR)
                    {
                        continue;
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "D%d.S%d", _gNormalTestFailCheckDeltac[i] % 100, _gNormalTestFailCheckDeltac[i] / 100);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "    ");
                }
                *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "Ratio";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < _gMsg28xx_DRIVE_NUM; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d",pRowStr, i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
            {
                if((i % _gMsg28xx_DRIVE_NUM) == 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d",pColStr, (i/_gMsg28xx_DRIVE_NUM) + 1);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestResultRatioData[i]);
                }
                else if((i % _gMsg28xx_DRIVE_NUM) == (_gMsg28xx_DRIVE_NUM - 1))
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d\n", _gMutualICOnCellOpenTestResultRatioData[i]);
                }
                else
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gMutualICOnCellOpenTestResultRatioData[i]);
                }
            }
            if (_gMutualICOnCellOpenTestResult[1] == 0)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "Ratio_Result:PASS\n");
            }
            else
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "Ratio_Result:FAIL\n");
                pItem = "Fail Channel:  ";
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%s",pItem);
                for (i = 0; i < _gMsg28xx_SENSE_NUM * _gMsg28xx_DRIVE_NUM; i++)
                {
                    if (_gNormalTestFailCheckRatio[i] == MSG28XX_PIN_NO_ERROR)
                    {
                        continue;
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "D%d.S%d", _gNormalTestFailCheckRatio[i] % 100, _gNormalTestFailCheckRatio[i] / 100);
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "    ");
                }
                *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            *pLength = *pLength + sprintf(pDataLog + *pLength, "ShortValue=%d\n",_gMsg28xx_SHORT_VALUE);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "ICPinShort=%d\n",_gMsg28xx_ICPIN_SHORT);
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "Pin Number";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
            {
                if (_gMsg28xx_SENSE_PAD_PIN_MAPPING[i] != 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%5s%2d", pPinStr, _gMsg28xx_SENSE_PAD_PIN_MAPPING[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "deltaR";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
            {
                if (_gMsg28xx_SENSE_PAD_PIN_MAPPING[i] != 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%6dM",_gIcPinShortSenceR[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "resultData";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
            {
                if (_gMsg28xx_SENSE_PAD_PIN_MAPPING[i] != 0)
                {
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d",_gIcPinShortResultData[i]);
                }
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            if (_gIcPinShortCheckFail == 0)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "ICPin Short Test:PASS\n");
            }
            else
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "ICPin Short Test:FAIL\n");
                pItem = "Fail Channel:  ";
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%s",pItem);
                for (i = 0; i < MUTUAL_IC_MAX_CHANNEL_NUM; i++)
                {
                    if (_gIcPinShortFailChannel[i] != 0)
                    {
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "P%d", _gIcPinShortFailChannel[i]);
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "    ");
                    }
                }
                *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "deltaR";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < 10; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM;i++)
            {
                if ((i % 10) == 0)
                {
                    if (i != 0)
                    {
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d", pColStr, i);
                }

                *pLength = *pLength + sprintf(pDataLog + *pLength, "%6dM", _gItoShortRData[i]);
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            for (i = 0; i < _gMsg28xx_DRIVE_NUM;i++)
            {
                if ((i % 10) == 0)
                {
                    if (i != 0)
                    {
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d", pRowStr, i);
                }

                *pLength = *pLength + sprintf(pDataLog + *pLength, "%6dM", _gItoShortRData[i+_gMsg28xx_SENSE_NUM]);
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            pItem = "resultData";
            *pLength = *pLength + sprintf(pDataLog + *pLength, "%10s",pItem);
            for (i = 0; i < 10; i++)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", i + 1);
            }

            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            for (i = 0; i < _gMsg28xx_SENSE_NUM;i++)
            {
                if ((i % 10) == 0)
                {
                    if (i != 0)
                    {
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d", pColStr, i);
                }

                *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gItoShortResultData[i]);
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
            for (i = 0; i < _gMsg28xx_DRIVE_NUM;i++)
            {
                if ((i % 10) == 0)
                {
                    if (i != 0)
                    {
                        *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");
                    }
                    *pLength = *pLength + sprintf(pDataLog + *pLength, "%8s%2d", pRowStr, i);
                }

                *pLength = *pLength + sprintf(pDataLog + *pLength, "%7d", _gItoShortResultData[i+_gMsg28xx_SENSE_NUM]);
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            if (_gItoShortCheckFail == 0)
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "ITO Short Test:PASS\n");
            }
            else
            {
                *pLength = *pLength + sprintf(pDataLog + *pLength, "ITO Short Test:FAIL\n");
            }
            *pLength = *pLength + sprintf(pDataLog + *pLength, "\n");

            DBG(&g_I2cClient->dev, "*** pDataLog: %d ***\n", pDataLog[0]);
            DBG(&g_I2cClient->dev, "*** *pLength: %d ***\n", *pLength);
        }
    }
    else
    {
        *pLength = *pLength + sprintf(pDataLog + *pLength, "No Logs!!!\n");
    }
}
#endif//CONFIG_ENABLE_CHIP_TYPE_MSG28XX

void DrvMpTestScheduleMpTestWork(ItoTestMode_e eItoTestMode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "*** g_IsInMpTest = %d ***\n", g_IsInMpTest);

    if (g_IsInMpTest == 0)
    {
        DBG(&g_I2cClient->dev, "ctp mp test start\n");

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            _gIsOldFirmwareVersion = _DrvMpTestMsg22xxCheckFirmwareVersion();
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

        _gItoTestMode = eItoTestMode;

        mutex_lock(&g_Mutex);
        g_IsInMpTest = 1;
        mutex_unlock(&g_Mutex);

        _gTestRetryCount = CTP_MP_TEST_RETRY_COUNT;
        _gCtpMpTestStatus = ITO_TEST_UNDER_TESTING;

        queue_work(_gCtpMpTestWorkQueue, &_gCtpItoTestWork);
    }
}

void DrvMpTestCreateMpTestWorkQueue(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gCtpMpTestWorkQueue = create_singlethread_workqueue("ctp_mp_test");
    INIT_WORK(&_gCtpItoTestWork, _DrvMpTestItoTestDoWork);
}

#endif //CONFIG_ENABLE_ITO_MP_TEST
