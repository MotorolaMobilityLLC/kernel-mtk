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
 * @file    mstar_drv_mp_test.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_MP_TEST_H__
#define __MSTAR_DRV_MP_TEST_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

#ifdef CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define CTP_MP_TEST_RETRY_COUNT (3)

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG22XX)
/*--------- Constant defined for MSG21xxA/MSG22xx ---------*/
#define SELF_IC_OPEN_TEST_NON_BORDER_AREA_THRESHOLD (35) // range : 25~60
#define SELF_IC_OPEN_TEST_BORDER_AREA_THRESHOLD     (40) // range : 25~60

#define	SELF_IC_SHORT_TEST_THRESHOLD                (3500)
#define SELF_IC_MAX_CHANNEL_NUM   (48)
#define SELF_IC_FIQ_E_FRAME_READY_MASK      (1 << 8)
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA || CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
// Constant defined for MSG21xxA
#define MSG21XXA_PIN_GUARD_RING    (46) 
#define MSG21XXA_GPO_SETTING_SIZE  (3)  
#define MSG21XXA_REG_INTR_FIQ_MASK (0x04)          
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG21XXA

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
// Constant defined for MSG22xx
#define MSG22XX_RIU_BASE_ADDR       (0)   
#define MSG22XX_RIU_WRITE_LENGTH    (144)  
#define MSG22XX_CSUB_REF            (0) //(18)   
#define MSG22XX_CSUB_REF_MAX        (0x3F) 

#define MSG22XX_MAX_SUBFRAME_NUM    (24)
#define MSG22XX_MAX_AFE_NUM         (4)
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
/*--------- Constant defined for MSG26xxM/MSG28xx ---------*/
#define MUTUAL_IC_MAX_CHANNEL_NUM  60 //38
#define MUTUAL_IC_MAX_MUTUAL_NUM  (MUTUAL_IC_MAX_CHANNEL_DRV * MUTUAL_IC_MAX_CHANNEL_SEN)

#define MUTUAL_IC_FIR_RATIO    50 //25
#define MUTUAL_IC_IIR_MAX		32600
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
// Constant defined for MSG26xxM
#define MUTUAL_IC_MAX_CHANNEL_DRV  28
#define MUTUAL_IC_MAX_CHANNEL_SEN  14

#define MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER (192) //192 = 14 * 13 + 10
#define MSG26XXM_ANA4_MUTUAL_CSUB_NUMBER (MUTUAL_IC_MAX_MUTUAL_NUM - MSG26XXM_ANA3_MUTUAL_CSUB_NUMBER) //200 = 392 - 192
#define MSG26XXM_FILTER1_MUTUAL_DELTA_C_NUMBER (190) //190 = (6 * 14 + 11) * 2
#define MSG26XXM_FILTER2_MUTUAL_DELTA_C_NUMBER (594) //594 = (MUTUAL_IC_MAX_MUTUAL_NUM - (6 * 14 + 11)) * 2
#define MSG26XXM_FIR_THRESHOLD    6553
#define MSG26XXM_SHORT_VALUE	2000
#define MSG26XXM_PIN_GUARD_RING    (38)
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
// Constant defined for MSG28xx
#define MUTUAL_IC_MAX_CHANNEL_DRV  30
#define MUTUAL_IC_MAX_CHANNEL_SEN  20

#define MSG28XX_SHORT_VALUE  3000
#define MSG28XX_WATER_VALUE  20000
#define MSG28XX_DC_RANGE  30
#define MSG28XX_DC_RATIO  10
#define MSG28XX_UN_USE_SENSOR (0x5AA5)
#define MSG28XX_UN_USE_PIN (0xABCD)
#define MSG28XX_NULL_DATA (-3240)
#define MSG28XX_PIN_NO_ERROR (0xFFFF)
#define MSG28XX_IIR_MAX 32600
#define MSG28XX_TEST_ITEM_NUM 8
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR MACRO DEFINITION                                            */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
typedef struct
{
    u8 nMy;
    u8 nMx;
    u8 nKeyNum;
} TestScopeInfo_t;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

/*--------------------------------------------------------------------------*/
/* GLOBAL VARIABLE DEFINITION                                               */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG26XXM) || defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
extern void DrvMpTestGetTestScope(TestScopeInfo_t *pInfo);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG26XXM || CONFIG_ENABLE_CHIP_TYPE_MSG28XX

#if defined(CONFIG_ENABLE_CHIP_TYPE_MSG28XX)
extern void DrvMpTestGetTestLogAll(u8 *pDataLog, u32 *pLength);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
extern void DrvMpTestCreateMpTestWorkQueue(void);
extern void DrvMpTestGetTestDataLog(ItoTestMode_e eItoTestMode, u8 *pDataLog, u32 *pLength);
extern void DrvMpTestGetTestFailChannel(ItoTestMode_e eItoTestMode, u8 *pFailChannel, u32 *pFailChannelCount);
extern s32 DrvMpTestGetTestResult(void);
extern void DrvMpTestScheduleMpTestWork(ItoTestMode_e eItoTestMode);

#endif //CONFIG_ENABLE_ITO_MP_TEST

#endif  /* __MSTAR_DRV_MP_TEST_H__ */