/************************************************************************
* Copyright (C) 2010-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Focaltech_test_ft3D47.c
*
* Author: Software Development Team, AE
*
* Created: 2016-11-16
*
* Abstract: test item for FT3D47
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include "../focaltech_test.h"

#if (FTS_CHIP_TEST_TYPE ==FT3D47_TEST)

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/

/////////////////////////////////////////////////Reg
#define     DEVIDE_MODE_ADDR            0x00
#define     REG_LINE_NUM                0x01
#define     REG_TX_NUM                  0x02
#define     REG_RX_NUM                  0x03
#define     REG_PATTERN_3C47            0x53
#define     REG_ScCbBuf0                0x4E
#define     REG_ScWorkMode              0x44
#define     REG_ScCbAddrR               0x45
#define     REG_RawBuf0                 0x36
#define     REG_WATER_CHANNEL_SELECT    0x09
#define     REG_TX_NOMAPPING_NUM        0x55
#define     REG_RX_NOMAPPING_NUM        0x56
#define     REG_PATTERN_5422            0X53
#define     REG_MAPPING_SWITCH          0X54
#define     REG_VA_MUL                  0X22
#define     REG_KEY_NUL                 0X21
#define     REG_SHIFT                   0X23
#define     REG_TX_CLK_NUM              0X33
#define     REG_SAMPLE_MOD              0X35
#define     REG_SCAN_VOL                0X20
#define     REG_NORMALIZE_TYPE          0X16
#define     REG_FREQUENCY               0x0A
#define     REG_FIR                     0XFB
#define     REG_RELEASECODEID_H         0xAE
#define     REG_RELEASECODEID_L         0xAF
#define     REG_FW_PROCESS              0x1A
#define     REG_REAL_TX_NUM             0XEB
#define     REG_REAL_RX_NUM             0XEC

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
enum WaterproofType {
    WT_NeedProofOnTest,
    WT_NeedProofOffTest,
    WT_NeedTxOnVal,
    WT_NeedRxOnVal,
    WT_NeedTxOffVal,
    WT_NeedRxOffVal,
};


struct stCfg_FT3D47_TestItem {
    bool FW_VERSION_TEST;
    bool FACTORY_ID_TEST;
    bool PROJECT_CODE_TEST;
    bool IC_VERSION_TEST;
    bool RAWDATA_TEST;
    bool ADC_DETECT_TEST;
    bool SCAP_CB_TEST;
    bool SCAP_RAWDATA_TEST;
    bool CHANNEL_NUM_TEST;
    bool INT_PIN_TEST;
    bool RESET_PIN_TEST;
    bool NOISE_TEST;
    bool WEAK_SHORT_CIRCUIT_TEST;
    bool UNIFORMITY_TEST;
    bool CM_TEST;

    bool RAWDATA_MARGIN_TEST;
    bool PANEL_DIFFER_TEST;
    bool PANEL_DIFFER_UNIFORMITY_TEST;

    bool LCM_ID_TEST;

    bool TE_TEST;
    bool SITO_RAWDATA_UNIFORMITY_TEST;
    bool PATTERN_TEST;

    bool FORCETOUCH_CHANNEL_NUM_TEST;
    bool FORCETOUCH_RAWDATA_TEST;
    bool FORCETOUCH_CB_TEST;
    bool FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST;
    bool FORCETOUCH_FLATNESS_TEST;
};

struct stCfg_FT3D47_BasicThreshold {
    u8 FW_VER_VALUE;
    u8 Factory_ID_Number;
    char Project_Code[32];
    u8 IC_Version;
    u8 LCM_ID;
    int RawDataTest_low_Min;
    int RawDataTest_Low_Max;
    int RawDataTest_high_Min;
    int RawDataTest_high_Max;
    u8 RawDataTest_SetLowFreq;
    u8 RawDataTest_SetHighFreq;
    int AdcDetect_Max;
    int SCapCbTest_OFF_Min;
    int SCapCbTest_OFF_Max;
    int SCapCbTest_ON_Min;
    int SCapCbTest_ON_Max;
    bool SCapCbTest_LetTx_Disable;
    u8 SCapCbTest_SetWaterproof_OFF;
    u8 SCapCbTest_SetWaterproof_ON;
    int SCapRawDataTest_OFF_Min;
    int SCapRawDataTest_OFF_Max;
    int SCapRawDataTest_ON_Min;
    int SCapRawDataTest_ON_Max;
    bool SCapRawDataTest_LetTx_Disable;
    u8 SCapRawDataTest_SetWaterproof_OFF;
    u8 SCapRawDataTest_SetWaterproof_ON;
    bool bChannelTestMapping;
    bool bChannelTestNoMapping;
    u8 ChannelNumTest_TxNum;
    u8 ChannelNumTest_RxNum;
    u8 ChannelNumTest_TxNpNum;
    u8 ChannelNumTest_RxNpNum;
    u8 ResetPinTest_RegAddr;
    u8 IntPinTest_RegAddr;
    u8 IntPinTest_TestNum;
    int NoiseTest_Max;
    int GloveNoiseTest_Coefficient;
    int NoiseTest_Frames;
    int NoiseTest_Time;
    u8 NoiseTest_SampeMode;
    u8 NoiseTest_NoiseMode;
    u8 NoiseTest_ShowTip;
    bool bNoiseTest_GloveMode;
    int NoiseTest_RawdataMin;
    unsigned char Set_Frequency;
    bool bNoiseThreshold_Choose;
    int NoiseTest_Threshold;
    int NoiseTest_MinNgFrame;
    int WeakShortTest_CG;
    int WeakShortTest_CC;
    int WeakShortTest_CC_Rsen;
    bool WeakShortTest_CapShortTest;
    bool Uniformity_CheckTx;
    bool Uniformity_CheckRx;
    bool Uniformity_CheckMinMax;
    int  Uniformity_Tx_Hole;
    int  Uniformity_Rx_Hole;
    int  Uniformity_MinMax_Hole;
    bool CMTest_CheckMin;
    bool CMTest_CheckMax;
    int  CMTest_MinHole;
    int  CMTest_MaxHole;

    int RawdataMarginTest_Min;
    int RawdataMarginTest_Max;

    int PanelDifferTest_Min;
    int PanelDifferTest_Max;

    bool PanelDiffer_UniformityTest_Check_Tx;
    bool PanelDiffer_UniformityTest_Check_Rx;
    bool PanelDiffer_UniformityTest_Check_MinMax;
    int  PanelDiffer_UniformityTest_Tx_Hole;
    int  PanelDiffer_UniformityTest_Rx_Hole;
    int  PanelDiffer_UniformityTest_MinMax_Hole;

    bool SITO_RawdtaUniformityTest_Check_Tx;
    bool SITO_RawdtaUniformityTest_Check_Rx;
    int  SITO_RawdtaUniformityTest_Tx_Hole;
    int  SITO_RawdtaUniformityTest_Rx_Hole;

    bool bPattern00;
    bool bPatternFF;
    bool bPattern55;
    bool bPatternAA;
    bool bPatternBin;

    int ForceTouch_ChannelNumTest_ChannelNum;

    int ForceTouch_SCapRawDataTest_OFF_Min;
    int ForceTouch_SCapRawDataTest_OFF_Max;
    int ForceTouch_SCapRawDataTest_ON_Min;
    int ForceTouch_SCapRawDataTest_ON_Max;
    u8 ForceTouch_SCapRawDataTest_SetWaterproof_OFF;
    u8 ForceTouch_SCapRawDataTest_SetWaterproof_ON;

    int ForceTouch_SCapCbTest_OFF_Min;
    int ForceTouch_SCapCbTest_OFF_Max;
    int ForceTouch_SCapCbTest_ON_Min;
    int ForceTouch_SCapCbTest_ON_Max;
    u8 ForceTouch_SCapCBTest_SetWaterproof_OFF;
    u8 ForceTouch_SCapCBTest_SetWaterproof_ON;

    int  ForceTouch_WeakShortTest_CG;
    int  ForceTouch_WeakShortTest_CC;
    bool bForceTouch_WeakShortTest_CapShortTest;

    int ForceTouch_FlatnessTest_Differ_Threshold;
    int ForceTouch_FlatnessTest_Differ_Coefficient;
    bool ForceTouch_FlatnessTest_Differ_Threshold_Check;
    bool ForceTouch_FlatnessTest_Differ_Coefficient_Check;

};
enum enumTestItem_FT3D47 {
    Code_FT3D47_ENTER_FACTORY_MODE,//All IC are required to test items
    Code_FT3D47_DOWNLOAD,//All IC are required to test items
    Code_FT3D47_UPGRADE,//All IC are required to test items
    Code_FT3D47_FACTORY_ID_TEST,
    Code_FT3D47_PROJECT_CODE_TEST,
    Code_FT3D47_FW_VERSION_TEST,
    Code_FT3D47_IC_VERSION_TEST,
    Code_FT3D47_RAWDATA_TEST,
    Code_FT3D47_ADCDETECT_TEST,
    Code_FT3D47_SCAP_CB_TEST,
    Code_FT3D47_SCAP_RAWDATA_TEST,
    Code_FT3D47_CHANNEL_NUM_TEST,
    Code_FT3D47_INT_PIN_TEST,
    Code_FT3D47_RESET_PIN_TEST,
    Code_FT3D47_NOISE_TEST,
    Code_FT3D47_WEAK_SHORT_CIRCUIT_TEST,
    Code_FT3D47_UNIFORMITY_TEST,
    Code_FT3D47_CM_TEST,
    Code_FT3D47_RAWDATA_MARGIN_TEST,
    Code_FT3D47_WRITE_CONFIG,//All IC are required to test items
    Code_FT3D47_PANELDIFFER_TEST,
    Code_FT3D47_PANELDIFFER_UNIFORMITY_TEST,
    Code_FT3D47_LCM_ID_TEST,
    Code_FT3D47_JUDEG_NORMALIZE_TYPE,
    Code_FT3D47_TE_TEST,
    Code_FT3D47_SITO_RAWDATA_UNIFORMITY_TEST,
    Code_FT3D47_PATTERN_TEST,

    Code_FT3D47_GPIO_TEST,
    Code_FT3D47_LCD_NOISE_TEST,
    Code_FT3D47_FORCE_TOUCH_CHANNEL_NUM_TEST,
    Code_FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST,
    Code_FT3D47_FORCE_TOUCH_SCAP_CB_TEST,
    Code_FT3D47_FORCE_TOUCH_WEAK_SHORT_CIRCUIT_TEST,
    Code_FT3D47_FORCE_TOUCH_FLATNESS_TEST,
};


/*******************************************************************************
* Static variables
*******************************************************************************/


static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
/* Force RawData */
static int m_Force_RawData_Water[2][TX_NUM_MAX] = {{0, 0}};
static int m_Force_CBData_Water[2][TX_NUM_MAX] = {{0, 0}};
static int m_Force_RawData_Normal[2][TX_NUM_MAX] = {{0, 0}};
static int m_Force_CBData_Normal[2][TX_NUM_MAX] = {{0, 0}};
static int m_iTempRawData[TX_NUM_MAX *RX_NUM_MAX] = {0};
static unsigned char m_ucTempData[TX_NUM_MAX *RX_NUM_MAX * 2] = {0};
static bool m_bV3TP = false;
static unsigned char m_iAllTx = 0;
static unsigned char m_iAllRx = 0;
static int m_iForceTouchTx = 0;
static int m_iForceTouchRx = 0;

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

struct stCfg_FT3D47_TestItem g_stCfg_FT3D47_TestItem;
struct stCfg_FT3D47_BasicThreshold g_stCfg_FT3D47_BasicThreshold;

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
//////////////////////////////////////////////Communication function
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue);
//////////////////////////////////////////////Common function
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
//////////////////////////////////////////////about Test
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void ShowRawData(void);
static bool GetTestCondition(int iTestType, unsigned char ucChannelValue);
bool start_test_ft3d47(void);
unsigned char FT3D47_TestItem_EnterFactoryMode(void);
unsigned char FT3D47_TestItem_RawDataTest(bool *bTestResult);
unsigned char FT3D47_TestItem_SCapRawDataTest(bool *bTestResult);
unsigned char FT3D47_TestItem_SCapCbTest(bool *bTestResult);
unsigned char FT3D47_TestItem_ForceTouch_SCapRawDataTest(bool *bTestResult);
unsigned char FT3D47_TestItem_ForceTouch_SCapCbTest(bool *bTestResult);


/************************************************************************
* Name: start_test_ft3d47
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
bool start_test_ft3d47(void)
{

    bool bTestResult = true;
    bool bTempResult = 1;
    unsigned char ReCode = 0;
    int iItemCount = 0;

    FTS_TEST_DBG("start_test_ft3d47");

    //--------------1. Init part
    if (init_test() < 0) {
        FTS_TEST_ERROR("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == test_data.test_num)
        bTestResult = false;

    for (iItemCount = 0; iItemCount < test_data.test_num; iItemCount++) {
        test_data.test_item_code = test_data.test_item[iItemCount].itemcode;

        ///////////////////////////////////////////////////////FT3D47_ENTER_FACTORY_MODE
        if (Code_FT3D47_ENTER_FACTORY_MODE == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }



        ///////////////////////////////////////////////////////FT3D47_RAWDATA_TEST
        if (Code_FT3D47_RAWDATA_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT3D47_SCAP_CB_TEST
        if (Code_FT3D47_SCAP_CB_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_SCAP_RAWDATA_TEST
        if (Code_FT3D47_SCAP_RAWDATA_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_FORCE_TOUCH_SCAP_CB_TEST
        if (Code_FT3D47_FORCE_TOUCH_SCAP_CB_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_ForceTouch_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST
        if (Code_FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT3D47_TestItem_ForceTouch_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }


    }

    //--------------3. End Part
    finish_test();

    //--------------4. return result
    return bTestResult;


}

/************************************************************************
* Name: FT3D47_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_EnterFactoryMode(void)
{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;  //If failed, repeat 5 times.
    int i ;
    unsigned char chPattern = 0;

    sys_delay(150);
    for (i = 1; i <= iRedo; i++) {
        ReCode = enter_factory_mode();
        if (ERROR_CODE_OK != ReCode) {
            FTS_TEST_ERROR("Failed to Enter factory mode...");
            if (i < iRedo) {
                sys_delay(50);
                continue;
            }
        } else {
            break;
        }

    }
    sys_delay(300);


    if (ReCode != ERROR_CODE_OK) {
        return ReCode;
    }

    //After the success of the factory model, read the number of channels
    ReCode = GetChannelNum();

    ////////////set FIR£¬0£ºclose£¬1£ºopen
    //theDevice.m_cHidDev[m_NumDevice]->write_reg(0xFB, 0);

    // to determine whether the V3 screen body
    ReCode = read_reg( REG_PATTERN_3C47, &chPattern );
    if (chPattern == 1) {
        m_bV3TP = true;
    } else {
        m_bV3TP = false;
    }

    return ReCode;
}
/************************************************************************
* Name: FT3D47_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_RawDataTest(bool *bTestResult)
{
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int RawDataMin;
    int RawDataMax;
    unsigned char ucFre; /* bakeup the current frequency */

    unsigned char OriginValue = 0xff;
    int index = 0;
    int iRow, iCol;
    int iValue = 0;

    FTS_TEST_DBG("Test for ft3d47");


    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data  Test \n");
    ReCode = enter_factory_mode();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }


    /* Read Current Frequency */
    ReCode = read_reg( REG_FREQUENCY, &ucFre );

    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n Read  REG_FREQUENCY error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    /*
      1:auto normalize rawdata
      0:all normalize rawdata
    */
    ReCode = read_reg( REG_NORMALIZE_TYPE, &OriginValue );// read the original value
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-2");

        FTS_TEST_ERROR("\n Read  REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }


    /* auto normalize rawdata */
    if (test_data.screen_param.normalize == AUTO_NORMALIZE) {

        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-3");

        /* if original value is not the value needed,write the register to change */
        if (OriginValue != 1) {
            ReCode = write_reg( REG_NORMALIZE_TYPE, 0x01 );

            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("\n write  REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }
        }

        //Set Frequecy High
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-4");


        FTS_TEST_DBG( "\n=========Set Frequecy High\n" );
        ReCode = write_reg( REG_FREQUENCY, 0x81 );
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("\n Set Frequecy High error. Error Code: %d",  ReCode);
            goto TEST_ERR;
        }

        //change register value before,need to lose 3 frame data
        for (index = 0; index < 3; ++index ) {
            ReCode = GetRawData();
        }

        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
            goto TEST_ERR;
        }

        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-5");

        /* Debug Info */
        ShowRawData();

        ////////////////////////////////To Determine RawData if in Range or not
        for (iRow = 0; iRow < test_data.screen_param.tx_num; iRow++) {
            for (iCol = 0; iCol < test_data.screen_param.rx_num; iCol++) {
                if (test_data.mcap_detail_thr.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = test_data.mcap_detail_thr.RawDataTest_High_Min[iRow][iCol];
                RawDataMax = test_data.mcap_detail_thr.RawDataTest_High_Max[iRow][iCol];
                iValue = m_RawData[iRow][iCol];
                if (iValue < RawDataMin || iValue > RawDataMax) {
                    btmpresult = false;
                    FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow + 1, iCol + 1, iValue, RawDataMin, RawDataMax);
                }
            }
        }

        //////////////////////////////Save Test Data
        Save_Test_Data(m_RawData, 0, test_data.screen_param.tx_num, test_data.screen_param.rx_num, 1);
    } else {
        FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-6");


        if (OriginValue != 0) { //if original value is not the value needed,write the register to change
            ReCode = write_reg( REG_NORMALIZE_TYPE, 0x00 );

            FTS_TEST_INFO("write_reg( REG_NORMALIZE_TYPE, 0x00 )");

            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("\n write REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }
        }

        ReCode =  read_reg( REG_FREQUENCY, &ucFre );

        FTS_TEST_INFO("read_reg( REG_FREQUENCY, &ucFre )");

        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("\n Read frequency error. Error Code: %d",  ReCode);
            goto TEST_ERR;
        }


        //Set Frequecy Low
        if (g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetLowFreq) {
            FTS_TEST_DBG("\n=========Set Frequecy Low");
            ReCode = write_reg( REG_FREQUENCY, 0x80 );
            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("\n write frequency error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }

            sys_delay(100);

            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index ) {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK ) {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                goto TEST_ERR;
            }

            FTS_TEST_DBG("FT3D47_TestItem_RawDataTest-7");

            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow < test_data.screen_param.tx_num; iRow++) {

                for (iCol = 0; iCol < test_data.screen_param.rx_num; iCol++) {
                    if (test_data.mcap_detail_thr.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = test_data.mcap_detail_thr.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = test_data.mcap_detail_thr.RawDataTest_High_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];
                    if (iValue < RawDataMin || iValue > RawDataMax) {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow + 1, iCol + 1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, test_data.screen_param.tx_num, test_data.screen_param.rx_num, 1);
        }


        //Set Frequecy High
        if ( g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetHighFreq ) {

            FTS_TEST_DBG( "\n=========Set Frequecy High");
            ReCode = write_reg( REG_FREQUENCY, 0x81 );
            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("\n Set Frequecy High error. Error Code: %d",  ReCode);
                goto TEST_ERR;
            }

            sys_delay(100);

            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index ) {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK ) {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);

                if ( ReCode != ERROR_CODE_OK ) {
                    goto TEST_ERR;
                }
            }

            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow < test_data.screen_param.tx_num; iRow++) {

                for (iCol = 0; iCol < test_data.screen_param.rx_num; iCol++) {
                    if (test_data.mcap_detail_thr.InvalidNode[iRow][iCol] == 0) {
                        continue;//Invalid Node
                    }

                    RawDataMin = test_data.mcap_detail_thr.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = test_data.mcap_detail_thr.RawDataTest_High_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];

                    if (iValue < RawDataMin || iValue > RawDataMax) {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow + 1, iCol + 1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, test_data.screen_param.tx_num, test_data.screen_param.rx_num, 2);
        }

    }


    FTS_TEST_INFO("write_reg( REG_NORMALIZE_TYPE, OriginValue )");

    ReCode = write_reg( REG_NORMALIZE_TYPE, OriginValue ); /* recovery REG_NORMALIZE_TYPE reg value */
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n Write REG_NORMALIZE_TYPE error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    FTS_TEST_INFO("write_reg( REG_FREQUENCY, ucFre )");

    ReCode = write_reg( REG_FREQUENCY, ucFre ); /* recovery REG_FREQUENCY reg value */
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n Write REG_FREQUENCY error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    //-------------------------Result
    if ( btmpresult ) {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!");
    } else {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_DBG("\n\n//RawData Test is NG!");
    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "RawData Test is NG. \n\n");
    return ReCode;

}
/************************************************************************
* Name: FT3D47_TestItem_SCapRawDataTest
* Brief:  TestItem: SCapRawDataTest. Check if SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_SCapRawDataTest(bool *bTestResult)
{
    int i = 0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    bool bFlag = true;
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int iMax = 0;
    int iMin = 0;
    int iAvg = 0;
    int ByteNum = 0;
    unsigned char wc_value = 0;//waterproof channel value

    int iCount = 0;
    int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = enter_factory_mode();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = read_reg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to read REG_WATER_CHANNEL_SELECT. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++) {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //Waterproof rawdata
        ByteNum = (m_iAllTx + m_iAllRx) * 2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("Failed to ReadRawData water! ");
            goto TEST_ERR;
        }

        memcpy( m_RawData[0 + test_data.screen_param.tx_num], m_iTempRawData, sizeof(int)*test_data.screen_param.rx_num );
        memcpy( m_RawData[1 + test_data.screen_param.tx_num], m_iTempRawData + m_iAllRx, sizeof(int)*test_data.screen_param.tx_num );

        // No waterproof rawdata
        ByteNum = (m_iAllTx + m_iAllRx) * 2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("Failed to ReadRawData no water! ");
            goto TEST_ERR;
        }
        memcpy( m_RawData[2 + test_data.screen_param.tx_num], m_iTempRawData, sizeof(int)*test_data.screen_param.rx_num );
        memcpy( m_RawData[3 + test_data.screen_param.tx_num], m_iTempRawData + m_iAllRx, sizeof(int)*test_data.screen_param.tx_num );
    }


    //-----3. Judge

    //Waterproof ON
    bFlag = GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_ON && bFlag ) {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Max;
        iMax = -m_RawData[0 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[0 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;


        bFlag = GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < test_data.screen_param.rx_num; i++ ) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.SCapRawDataTest_ON_Min[0][i];
            RawDataMax = test_data.mcap_detail_thr.SCapRawDataTest_ON_Max[0][i];
            Value = m_RawData[0 + test_data.screen_param.tx_num][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < test_data.screen_param.tx_num; i++) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.SCapRawDataTest_ON_Min[1][i];
            RawDataMax = test_data.mcap_detail_thr.SCapRawDataTest_ON_Max[1][i];
            Value = m_RawData[1 + test_data.screen_param.tx_num][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = test_data.screen_param.tx_num > test_data.screen_param.rx_num ? test_data.screen_param.tx_num : test_data.screen_param.rx_num;
        Save_Test_Data(m_RawData, test_data.screen_param.tx_num + 0, 2, ibiggerValue, 1);
    }

    //Waterproof OFF
    bFlag = GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_OFF && bFlag) {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Max;
        iMax = -m_RawData[2 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[2 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;

        bFlag = GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < test_data.screen_param.rx_num; i++) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = test_data.mcap_detail_thr.SCapRawDataTest_OFF_Max[0][i];
            Value = m_RawData[2 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag = GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < test_data.screen_param.tx_num; i++) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_RawData[3 + test_data.screen_param.tx_num][i];
            RawDataMin = test_data.mcap_detail_thr.SCapRawDataTest_OFF_Min[1][i];
            RawDataMax = test_data.mcap_detail_thr.SCapRawDataTest_OFF_Max[1][i];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = test_data.screen_param.tx_num > test_data.screen_param.rx_num ? test_data.screen_param.tx_num : test_data.screen_param.rx_num;
        Save_Test_Data(m_RawData, test_data.screen_param.tx_num + 2, 2, ibiggerValue, 2);
    }

    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "SCap RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    //-----5. Test Result
    if ( btmpresult ) {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap RawData Test is OK!");
    } else {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "SCap RawData Test is NG. \n\n");
    return ReCode;
}

/************************************************************************
* Name: FT3D47_TestItem_SCapCbTest
* Brief:  TestItem: SCapCbTest. Check if SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_SCapCbTest(bool *bTestResult)
{
    int i,/* j, iOutNum,*/index, Value, CBMin, CBMax;
    bool bFlag = true;
    unsigned char ReCode;
    bool btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;

    int iCount = 0;
    int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -----  Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = enter_factory_mode();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = read_reg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n Read REG_WATER_CHANNEL_SELECT error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("Failed to Scan SCap RawData!ReCode = %d. ",  ReCode);
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++) {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //Waterproof CB
        ReCode = write_reg( REG_ScWorkMode, 1 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = write_reg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        for ( index = 0; index < test_data.screen_param.rx_num; ++index ) {
            m_RawData[0 + test_data.screen_param.tx_num][index] = m_ucTempData[index];
        }
        for ( index = 0; index < test_data.screen_param.tx_num; ++index ) {
            m_RawData[1 + test_data.screen_param.tx_num][index] = m_ucTempData[index + m_iAllRx];
        }

        // Non Waterproof rawdata
        ReCode = write_reg( REG_ScWorkMode, 0 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = write_reg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }
        for ( index = 0; index < test_data.screen_param.rx_num; ++index ) {
            m_RawData[2 + test_data.screen_param.tx_num][index] = m_ucTempData[index];
        }
        for ( index = 0; index < test_data.screen_param.tx_num; ++index ) {
            m_RawData[3 + test_data.screen_param.tx_num][index] = m_ucTempData[index + m_iAllRx];
        }

        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag = GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_ON && bFlag) {
        FTS_TEST_DBG("SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[0 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag = GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < test_data.screen_param.rx_num; i++ ) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.SCapCbTest_ON_Min[0][i];
            CBMax = test_data.mcap_detail_thr.SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < test_data.screen_param.tx_num; i++) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.SCapCbTest_ON_Min[1][i];
            CBMax = test_data.mcap_detail_thr.SCapCbTest_ON_Max[1][i];
            Value = m_RawData[1 + test_data.screen_param.tx_num][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = test_data.screen_param.tx_num > test_data.screen_param.rx_num ? test_data.screen_param.tx_num : test_data.screen_param.rx_num;
        Save_Test_Data(m_RawData, test_data.screen_param.tx_num + 0, 2, ibiggerValue, 1);
    }

    bFlag = GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_OFF && bFlag) {
        FTS_TEST_DBG("SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[2 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag = GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < test_data.screen_param.rx_num; i++) {
            if ( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.SCapCbTest_OFF_Min[0][i];
            CBMax = test_data.mcap_detail_thr.SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag && i < test_data.screen_param.tx_num; i++) {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            if ( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.SCapCbTest_OFF_Min[1][i];
            CBMax = test_data.mcap_detail_thr.SCapCbTest_OFF_Max[1][i];
            Value = m_RawData[3 + test_data.screen_param.tx_num][i];

            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        ibiggerValue = test_data.screen_param.tx_num > test_data.screen_param.rx_num ? test_data.screen_param.tx_num : test_data.screen_param.rx_num;
        Save_Test_Data(m_RawData, test_data.screen_param.tx_num + 2, 2, ibiggerValue, 2);
    }

    //-----5. Test Result

    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "SCap CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult ) {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is OK!");
    } else {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "SCap CB Test is NG. \n\n");
    return ReCode;
}



/************************************************************************
* Name: FT3D47_TestItem_ForceTouch_SCapRawDataTest
* Brief:  TestItem: ForceTouch_SCapRawDataTest. Check if ForceTouch_SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_ForceTouch_SCapRawDataTest(bool *bTestResult)
{
    int m = 0;
    int i = 0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    bool bFlag = true;
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int iMax = 0;
    int iMin = 0;
    int iAvg = 0;
    int ByteNum = 0;
    unsigned char wc_value = 0;//waterproof channel value
    int iCount = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Force Touch Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = enter_factory_mode();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = read_reg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to read REG_WATER_CHANNEL_SELECT. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++) {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //Waterproof rawdata
        ByteNum = (test_data.screen_param.rx_num + test_data.screen_param.tx_num + m_iForceTouchRx + m_iForceTouchTx) * 2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("Failed to ReadRawData water! ");
            goto TEST_ERR;
        }

        for (m = 0; m < m_iForceTouchTx; m++) {
            m_Force_RawData_Water[0][m] = m_iTempRawData[test_data.screen_param.rx_num + test_data.screen_param.tx_num + m];
        }

        FTS_TEST_INFO("test_data.screen_param.tx_num[%d]", test_data.screen_param.tx_num);
        FTS_TEST_INFO("test_data.screen_param.rx_num[%d]", test_data.screen_param.rx_num);
        FTS_TEST_INFO("m_iForceTouchRx[%d]", m_iForceTouchRx);
        FTS_TEST_INFO("m_iForceTouchTx[%d]", m_iForceTouchTx);

        //No Waterproof rawdata
        //ByteNum = (test_data.screen_param.tx_num + test_data.screen_param.rx_num)*2;
        ByteNum = (test_data.screen_param.rx_num + test_data.screen_param.tx_num + m_iForceTouchRx + m_iForceTouchTx) * 2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("Failed to ReadRawData no water! ");
            goto TEST_ERR;
        }

        for (m = 0; m < m_iForceTouchTx; m++) {
            m_Force_RawData_Normal[0][m] = m_iTempRawData[test_data.screen_param.rx_num + test_data.screen_param.tx_num + m];
        }

        FTS_TEST_INFO("test_data.screen_param.tx_num[%d]", test_data.screen_param.tx_num);
        FTS_TEST_INFO("test_data.screen_param.rx_num[%d]", test_data.screen_param.rx_num);
        FTS_TEST_INFO("m_iForceTouchRx[%d]", m_iForceTouchRx);
        FTS_TEST_INFO("m_iForceTouchTx[%d]", m_iForceTouchTx);
    }


    //-----3. Judge

    //Waterproof ON
    bFlag = GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_ON && bFlag ) {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Max;
        iMax = -m_Force_RawData_Water[0][0];
        iMin = 2 * m_Force_RawData_Water[0][0];
        iAvg = 0;
        Value = 0;


        bFlag = GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < m_iForceTouchRx; i++ ) {
            //if( test_data.mcap_detail_thr.ForceTouch_InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_ON_Min[0][i];
            RawDataMax = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_ON_Max[0][i];
            Value = m_Force_RawData_Water[0][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_ON_Min[0][i + m_iForceTouchRx];
            RawDataMax = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_ON_Max[0][i + m_iForceTouchRx];
            Value = m_Force_RawData_Water[0][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("Force Touch SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
    }

    //Waterproof OFF
    bFlag = GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_OFF && bFlag) {
        iCount = 0;
        RawDataMin = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Max;
        iMax = -m_Force_RawData_Normal[0][0];
        iMin = 2 * m_Force_RawData_Normal[0][0];
        iAvg = 0;
        Value = 0;

        bFlag = GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < m_iForceTouchRx; i++) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_OFF_Max[0][i];
            Value = m_Force_RawData_Normal[0][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag = GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_Force_RawData_Normal[0][i];
            RawDataMin = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_OFF_Min[0][i + m_iForceTouchRx];
            RawDataMax = test_data.mcap_detail_thr.ForceTouch_SCapRawDataTest_OFF_Max[0][i + m_iForceTouchRx];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("Force Touch SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
    }

    Save_Test_Data(m_Force_RawData_Water, 0, 1, m_iForceTouchTx, 1);
    Save_Test_Data(m_Force_RawData_Normal, 0, 1, m_iForceTouchTx, 1);

    //-----5. Test Result

    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "Force Touch SCap RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult ) {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is OK!");
    } else {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//Force Touch SCap RawData Test is NG!");
    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "Force Touch SCap RawData Test is NG. \n\n");
    return ReCode;
}


/************************************************************************
* Name: FT3D47_TestItem_ForceTouch_SCapCbTest
* Brief:  TestItem: ForceTouch_SCapCbTest. Check if ForceTouch_SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT3D47_TestItem_ForceTouch_SCapCbTest(bool *bTestResult)
{
    int i,/* j, iOutNum,*/index, Value, CBMin, CBMax;
    bool bFlag = true;
    unsigned char ReCode;
    bool btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;
    int iCount = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -----  Force Touch Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = enter_factory_mode();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = read_reg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("\n Read REG_WATER_CHANNEL_SELECT error. Error Code: %d",  ReCode);
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("Failed to Scan SCap RawData!ReCode = %d. ",  ReCode);
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++) {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //Waterproof CB
        ReCode = write_reg( REG_ScWorkMode, 1 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = write_reg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        FTS_TEST_INFO("m_iAllTx[%d], m_iAllRx[%d]", m_iAllTx, m_iAllRx);
        index = test_data.screen_param.rx_num + test_data.screen_param.tx_num;

        for (i = 0; i < m_iForceTouchTx; i++) {

            m_Force_CBData_Water[0][i] = m_ucTempData[index];
            FTS_TEST_DBG("i[%d], CbData[%d]", i , m_Force_CBData_Water[0][i]);
            index++;
        }

        //No Waterproof rawdata
        ReCode = write_reg( REG_ScWorkMode, 0 );//ScWorkMode:  1: waterproof 0:Non-waterproof
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Get REG_ScWorkMode Failed!");
            goto TEST_ERR;
        }

        ReCode = StartScan();
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("StartScan Failed!");
            goto TEST_ERR;
        }

        ReCode = write_reg( REG_ScCbAddrR, 0 );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Write REG_ScCbAddrR Failed!");
            goto TEST_ERR;
        }

        ReCode = GetTxSC_CB( m_iAllTx + m_iAllRx + 128, m_ucTempData );
        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("GetTxSC_CB Failed!");
            goto TEST_ERR;
        }

        index = test_data.screen_param.rx_num + test_data.screen_param.tx_num;

        for (i = 0; i < m_iForceTouchTx; i++) {

            m_Force_CBData_Normal[0][i] = m_ucTempData[index];
            FTS_TEST_DBG("i[%d], CbData[%d]", i , m_RawData[0 + test_data.screen_param.tx_num][m_iForceTouchRx + i]);
            index++;
        }

        if ( ReCode != ERROR_CODE_OK ) {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag = GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_ON && bFlag) {
        FTS_TEST_DBG("Force Touch SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[0 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag = GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < m_iForceTouchRx; i++ ) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_ON_Min[0][i];
            CBMax = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < m_iForceTouchTx; i++) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_ON_Min[0][i + m_iForceTouchRx];
            CBMax = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_ON_Max[0][i + m_iForceTouchRx];
            Value = m_RawData[1 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            FTS_TEST_DBG("Value = %d.", Value);
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("Force Touch SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = m_iForceTouchTx>m_iForceTouchRx?m_iForceTouchTx:m_iForceTouchRx;
        //Save_Test_Data(m_RawData, test_data.screen_param.tx_num+0, 1, m_iForceTouchRx + m_iForceTouchTx, 1);
    }

    bFlag = GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_OFF && bFlag) {
        FTS_TEST_DBG("Force Touch SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2 + test_data.screen_param.tx_num][0];
        iMin = 2 * m_RawData[2 + test_data.screen_param.tx_num][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;

        bFlag = GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < m_iForceTouchRx; i++) {
            //if( test_data.mcap_detail_thr.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_OFF_Min[0][i];
            CBMax = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2 + test_data.screen_param.tx_num][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag = GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Force Touch SCap CB_Tx:  ");
        for (i = 0; bFlag && i < m_iForceTouchTx; i++) {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            //if( test_data.mcap_detail_thr.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_OFF_Min[0][i + m_iForceTouchRx];
            CBMax = test_data.mcap_detail_thr.ForceTouch_SCapCbTest_OFF_Max[0][i + m_iForceTouchRx];
            Value = m_RawData[3 + test_data.screen_param.tx_num][i];

            FTS_TEST_DBG("Value = %d. ", Value);


            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin) {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i + 1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount) {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        } else
            iAvg = iAvg / iCount;

        FTS_TEST_DBG("Force Touch SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //      ibiggerValue = m_iForceTouchTx>m_iForceTouchRx?m_iForceTouchTx:m_iForceTouchRx;
        //Save_Test_Data(m_RawData, test_data.screen_param.tx_num+2, 1, m_iForceTouchRx + m_iForceTouchTx, 2);
    }

    Save_Test_Data(m_Force_CBData_Water, 0, 1, m_iForceTouchTx, 2);
    Save_Test_Data(m_Force_CBData_Normal, 0, 1, m_iForceTouchTx, 2);

    //-----5. Test Result

    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "Force Touch SCap CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult ) {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is OK!");
    } else {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//Force Touch SCap CB Test is NG!");
    test_data.testresult_len += sprintf(test_data.testresult + test_data.testresult_len, "Force Touch SCap CB Test is NG. \n\n");
    return ReCode;
}
/************************************************************************
* Name: GetPanelRows(Same function name as FT_MultipleTest)
* Brief:  Get row of TP
* Input: none
* Output: pPanelRows
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return read_reg(REG_TX_NUM, pPanelRows);
}

/************************************************************************
* Name: GetPanelCols(Same function name as FT_MultipleTest)
* Brief:  get column of TP
* Input: none
* Output: pPanelCols
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return read_reg(REG_RX_NUM, pPanelCols);
}
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0;
    unsigned char times = 0;
    const unsigned char MaxTimes = 250;
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    ReCode = read_reg(DEVIDE_MODE_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK) {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = write_reg(DEVIDE_MODE_ADDR, RegVal);
        if (ReCode == ERROR_CODE_OK) {
            while (times++ < MaxTimes) {    //Wait for the scan to complete
                sys_delay(8);    //8ms
                ReCode = read_reg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK) {
                    if ((RegVal >> 7) == 0)    break;
                } else {
                    FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.");
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        } else
            FTS_TEST_ERROR("StartScan write DEVIDE_MODE_ADDR error.");
    } else
        FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.");
    return ReCode;

}
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3];
    int i, iReadNum;
    unsigned short BytesNumInTestMode1 = 0;

    iReadNum = ByteNum / BYTES_PER_TIME;

    if (0 != (ByteNum % BYTES_PER_TIME)) iReadNum++;

    if (ByteNum <= BYTES_PER_TIME) {
        BytesNumInTestMode1 = ByteNum;
    } else {
        BytesNumInTestMode1 = BYTES_PER_TIME;
    }

    ReCode = write_reg(REG_LINE_NUM, LineNum);//Set row addr;

    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_ERROR("Failed to write REG_LINE_NUM! ");
        goto READ_ERR;
    }

    //***********************************************************Read raw data
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK) {
        sys_delay(10);
        ReCode = fts_i2c_read_write(I2C_wBuffer, 1, m_ucTempData, BytesNumInTestMode1);
        if (ReCode != ERROR_CODE_OK) {
            FTS_TEST_ERROR("read rawdata fts_i2c_read_write Failed!1 ");
            goto READ_ERR;
        }
    }

    for (i = 1; i < iReadNum; i++) {
        if (ReCode != ERROR_CODE_OK) break;

        if (i == iReadNum - 1) { //last packet
            sys_delay(10);
            ReCode = fts_i2c_read_write(NULL, 0, m_ucTempData + BYTES_PER_TIME * i, ByteNum - BYTES_PER_TIME * i);
            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("read rawdata fts_i2c_read_write Failed!2 ");
                goto READ_ERR;
            }
        } else {
            sys_delay(10);
            ReCode = fts_i2c_read_write(NULL, 0, m_ucTempData + BYTES_PER_TIME * i, BYTES_PER_TIME);

            if (ReCode != ERROR_CODE_OK) {
                FTS_TEST_ERROR("read rawdata fts_i2c_read_write Failed!3 ");
                goto READ_ERR;
            }
        }

    }

    if (ReCode == ERROR_CODE_OK) {
        for (i = 0; i < (ByteNum >> 1); i++) {
            pRevBuffer[i] = (m_ucTempData[i << 1] << 8) + m_ucTempData[(i << 1) + 1];
            //if(pRevBuffer[i] & 0x8000)//have sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
        }
    }

READ_ERR:
    return ReCode;

}
/************************************************************************
* Name: GetTxSC_CB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx SCap
* Input: index
* Output: pcbValue
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char wBuffer[4];

    if (index < 128) { //single read
        *pcbValue = 0;
        write_reg(REG_ScCbAddrR, index);
        ReCode = read_reg(REG_ScCbBuf0, pcbValue);
    } else { //Sequential Read length index-128
        write_reg(REG_ScCbAddrR, 0);
        wBuffer[0] = REG_ScCbBuf0;
        ReCode = fts_i2c_read_write(wBuffer, 1, pcbValue, index - 128);

    }

    return ReCode;
}


/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
    int iLen = 0;
    int i = 0, j = 0;

    //Save  Msg (itemcode is enough, ItemName is not necessary, so set it to "NA".)
    iLen = sprintf(test_data.tmp_buffer, "NA, %d, %d, %d, %d, %d, ", \
                   test_data.test_item_code, Row, Col, test_data.start_line, ItemCount);
    memcpy(test_data.msg_area_line2 + test_data.len_msg_area_line2, test_data.tmp_buffer, iLen);
    test_data.len_msg_area_line2 += iLen;

    test_data.start_line += Row;
    test_data.test_data_count++;

    //Save Data
    for (i = 0 + iArrayIndex; i < Row + iArrayIndex; i++) {
        for (j = 0; j < Col; j++) {
            if (j == (Col - 1)) //The Last Data of the Row, add "\n"
                iLen = sprintf(test_data.tmp_buffer, "%d, \n ",  iData[i][j]);
            else
                iLen = sprintf(test_data.tmp_buffer, "%d, ", iData[i][j]);

            memcpy(test_data.store_data_area + test_data.len_store_data_area, test_data.tmp_buffer, iLen);
            test_data.len_store_data_area += iLen;
        }
    }

}

/************************************************************************
* Name: GetChannelNum
* Brief:  Get Channel Num(Tx and Rx)
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode = 0;
    unsigned char rBuffer[1] = {0}; //= new unsigned char;

    unsigned char TxNum = 0;
    unsigned char RxNum = 0;

    //m_strCurrentTestMsg = "Get Tx Num...";
    ReCode = GetPanelRows(rBuffer);
    if (ReCode == ERROR_CODE_OK) {
        test_data.screen_param.tx_num = rBuffer[0];
        if (test_data.screen_param.tx_num > test_data.screen_param.used_max_tx_num) {

            FTS_TEST_ERROR("Failed to get Tx number, Get num = %d, UsedMaxNum = %d",
                           test_data.screen_param.tx_num, test_data.screen_param.used_max_tx_num);
            test_data.screen_param.tx_num = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
    } else {
        FTS_TEST_ERROR("Failed to get Tx number");
    }

    ///////////////m_strCurrentTestMsg = "Get Rx Num...";

    ReCode = GetPanelCols(rBuffer);
    if (ReCode == ERROR_CODE_OK) {
        test_data.screen_param.rx_num = rBuffer[0];
        if (test_data.screen_param.rx_num > test_data.screen_param.used_max_rx_num) {

            FTS_TEST_ERROR("Failed to get Rx number, Get num = %d, UsedMaxNum = %d",
                           test_data.screen_param.rx_num, test_data.screen_param.used_max_rx_num);
            test_data.screen_param.rx_num = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
    } else {
        FTS_TEST_ERROR("Failed to get Rx number");
    }


    //get force touch channel num
    //m_strCurrentTestMsg = "Get real Rx Num...";
    ReCode = read_reg(REG_REAL_RX_NUM, &RxNum);
    if (ReCode == ERROR_CODE_OK) {
        m_iForceTouchRx = RxNum - test_data.screen_param.rx_num;
    } else {
        FTS_TEST_DBG("Failed to get real Rx number\r");
    }

    //m_strCurrentTestMsg = "Get real Tx Num...";
    ReCode = read_reg(REG_REAL_TX_NUM, &TxNum);
    if (ReCode == ERROR_CODE_OK) {
        m_iForceTouchTx = TxNum - test_data.screen_param.tx_num;
    } else {
        FTS_TEST_ERROR("Failed to get real Tx number\r");
    }

    m_iAllTx = TxNum;
    m_iAllRx = RxNum;

    FTS_TEST_DBG("m_iForceTouchRx = %d, m_iForceTouchTx = %d", m_iForceTouchRx, m_iForceTouchTx);
    return ReCode;

}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of MCAP
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;

    //--------------------------------------------Enter Factory Mode
    ReCode = enter_factory_mode();
    if ( ERROR_CODE_OK != ReCode ) {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (test_data.screen_param.tx_num + test_data.screen_param.rx_num)) {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode ) {
            FTS_TEST_ERROR("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode) {
        FTS_TEST_ERROR("Failed to Scan ...");
        return ReCode;
    }

    //--------------------------------------------Read RawData, Only MCAP
    memset(m_RawData, 0, sizeof(m_RawData));
    ReCode = ReadRawData( 1, 0xAA, ( test_data.screen_param.tx_num * test_data.screen_param.rx_num ) * 2, m_iTempRawData );
    for (iRow = 0; iRow < test_data.screen_param.tx_num; iRow++) {
        for (iCol = 0; iCol < test_data.screen_param.rx_num; iCol++) {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow * test_data.screen_param.rx_num + iCol];
        }
    }
    return ReCode;
}
/************************************************************************
* Name: ShowRawData
* Brief:  Show RawData
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ShowRawData(void)
{
    int iRow, iCol;
    //----------------------------------------------------------Show RawData
    for (iRow = 0; iRow < test_data.screen_param.tx_num; iRow++) {
        FTS_TEST_DBG("Tx%2d:  ", iRow + 1);
        for (iCol = 0; iCol < test_data.screen_param.rx_num; iCol++) {
            FTS_TEST_DBG("%5d    ", m_RawData[iRow][iCol]);
        }
        FTS_TEST_DBG("\n ");
    }
}

/************************************************************************
* Name: GetTestCondition
* Brief:  Check whether Rx or TX need to test, in Waterproof ON/OFF Mode.
* Input: none
* Output: none
* Return: true: need to test; false: Not tested.
***********************************************************************/
static bool GetTestCondition(int iTestType, unsigned char ucChannelValue)
{
    bool bIsNeeded = false;
    switch (iTestType) {
    case WT_NeedProofOnTest://Bit5:  0:test waterProof mode ;  1 not test waterProof mode
        bIsNeeded = !( ucChannelValue & 0x20 );
        break;
    case WT_NeedProofOffTest://Bit7: 0: test normal mode  1:not test normal mode
        bIsNeeded = !( ucChannelValue & 0x80 );
        break;
    case WT_NeedTxOnVal:
        //Bit6:  0 : test waterProof Rx+Tx  1:test waterProof single channel
        //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
        bIsNeeded = !( ucChannelValue & 0x40 ) || !( ucChannelValue & 0x04 );
        break;
    case WT_NeedRxOnVal:
        //Bit6:  0 : test waterProof Rx+Tx  1 test waterProof single channel
        //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
        bIsNeeded = !( ucChannelValue & 0x40 ) || ( ucChannelValue & 0x04 );
        break;
    case WT_NeedTxOffVal://Bit1,Bit0:  00:test normal Tx; 10: test normal Rx+Tx
        bIsNeeded = (0x00 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
        break;
    case WT_NeedRxOffVal://Bit1,Bit0:  01: test normal Rx;    10: test normal Rx+Tx
        bIsNeeded = (0x01 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
        break;
    default:
        break;
    }
    return bIsNeeded;
}



void init_testitem_ft3d47(char *strIniFile)
{

    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString("TestItem", "FW_VERSION_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FW_VERSION_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("TestItem", "FACTORY_ID_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FACTORY_ID_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("TestItem", "PROJECT_CODE_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.PROJECT_CODE_TEST = fts_atoi(str);

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    g_stCfg_FT3D47_TestItem.RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// SCAP_CB_TEST
    GetPrivateProfileString("TestItem", "SCAP_CB_TEST", "1", str, strIniFile);
    g_stCfg_FT3D47_TestItem.SCAP_CB_TEST = fts_atoi(str);

    /////////////////////////////////// SCAP_RAWDATA_TEST
    GetPrivateProfileString("TestItem", "SCAP_RAWDATA_TEST", "1", str, strIniFile);
    g_stCfg_FT3D47_TestItem.SCAP_RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem", "CHANNEL_NUM_TEST", "1", str, strIniFile);
    g_stCfg_FT3D47_TestItem.CHANNEL_NUM_TEST = fts_atoi(str);

    /////////////////////////////////// NOISE_TEST
    GetPrivateProfileString("TestItem", "NOISE_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// WEAK_SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "WEAK_SHORT_CIRCUIT_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.WEAK_SHORT_CIRCUIT_TEST = fts_atoi(str);

    /////////////////////////////////// UNIFORMITY_TEST
    GetPrivateProfileString("TestItem", "UNIFORMITY_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.UNIFORMITY_TEST = fts_atoi(str);

    /////////////////////////////////// panel differ_TEST
    GetPrivateProfileString("TestItem", "PANEL_DIFFER_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.PANEL_DIFFER_TEST = fts_atoi(str);

    /////////////////////////////////// panel differ uniformity_TEST
    GetPrivateProfileString("TestItem", "PANEL_DIFFER_UNIFORMITY_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.PANEL_DIFFER_UNIFORMITY_TEST = fts_atoi(str);

    ///////////////////////////////////SITO_RAWDATA_UNIFORMITY_TEST
    GetPrivateProfileString("TestItem", "SITO_RAWDATA_UNIFORMITY_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.SITO_RAWDATA_UNIFORMITY_TEST = fts_atoi(str);

    ///////////////////////////////////PATTERN_TEST
    GetPrivateProfileString("TestItem", "PATTERN_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.PATTERN_TEST = fts_atoi(str);

    //force touch
    ///////////////////////////////////FORCETOUCH_CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem", "FORCETOUCH_CHANNEL_NUM_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FORCETOUCH_CHANNEL_NUM_TEST = fts_atoi(str);

    ///////////////////////////////////FORCETOUCH_RAWDATA_TEST
    GetPrivateProfileString("TestItem", "FORCETOUCH_RAWDATA_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FORCETOUCH_RAWDATA_TEST = fts_atoi(str);

    ///////////////////////////////////FORCETOUCH_CB_TEST
    GetPrivateProfileString("TestItem", "FORCETOUCH_CB_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FORCETOUCH_CB_TEST = fts_atoi(str);

    ///////////////////////////////////FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST", "0", str, strIniFile);
    g_stCfg_FT3D47_TestItem.FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();

}
void init_basicthreshold_ft3d47(char *strIniFile)
{

    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString( "Basic_Threshold", "FW_VER_VALUE", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.FW_VER_VALUE = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("Basic_Threshold", "Factory_ID_Number", "255", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Factory_ID_Number = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("Basic_Threshold", "Project_Code", " ", str, strIniFile);
    //g_stCfg_FT3C47_BasicThreshold.Project_Code.Format("%s", str);
    sprintf(g_stCfg_FT3D47_BasicThreshold.Project_Code, "%s", str);

    //////////////////////////////////////////////////////////// RawDataTest
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Low_Min", "3000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_low_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Low_Max", "15000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_Low_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "RawDataTest_High_Min", "3000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_high_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_High_Max", "15000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_high_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "RawDataTest_LowFreq", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetLowFreq  = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_HighFreq", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.RawDataTest_SetHighFreq = fts_atoi(str);

    //////////////////////////////////////////////////////////// SCapCbTest
    GetPrivateProfileString("Basic_Threshold", "SCapCbTest_OFF_Min", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_OFF_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SCapCbTest_OFF_Max", "240", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_OFF_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapCbTest_ON_Min", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_ON_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SCapCbTest_ON_Max", "240", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_ON_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ScapCBTest_SetWaterproof_OFF", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_OFF = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ScapCBTest_SetWaterproof_ON", "240", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_SetWaterproof_ON = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapCBTest_LetTx_Disable", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapCbTest_LetTx_Disable = fts_atoi(str);

    //////////////////////////////////////////////////////////// SCapRawDataTest
    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_OFF_Min", "5000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_OFF_Max", "8500", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_OFF_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_ON_Min", "5000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_ON_Max", "8500", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_ON_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_SetWaterproof_OFF", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_OFF = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_SetWaterproof_ON", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_SetWaterproof_ON = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SCapRawDataTest_LetTx_Disable", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SCapRawDataTest_LetTx_Disable = fts_atoi(str);

    //////////////////////////////////////////////////////////// ChannelNumTest
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_Mapping", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.bChannelTestMapping = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_NoMapping", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.bChannelTestNoMapping = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_TxNum", "13", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ChannelNumTest_TxNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_RxNum", "24", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ChannelNumTest_RxNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_Tx_NP_Num", "13", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ChannelNumTest_TxNpNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_Rx_NP_Num", "24", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ChannelNumTest_RxNpNum = fts_atoi(str);

    //////////////////////////////////////////////////////////// NoiseTest
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Max", "20", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Frames", "32", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_Frames = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Time", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_Time = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_SampeMode", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_SampeMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_NoiseMode", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_ShowTip", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_ShowTip = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_GloveMode", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.bNoiseTest_GloveMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_RawdataMin", "5000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_RawdataMin = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "GloveNoiseTest_Coefficient", "100", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.GloveNoiseTest_Coefficient = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "Set_Frequency", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Set_Frequency = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseThreshold_Choose", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.bNoiseThreshold_Choose = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Threshold", "50", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_Threshold = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_MinNGFrame", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.NoiseTest_MinNgFrame = fts_atoi(str);

    //////////////////////////////////////////////////////////// WeakShortTest
    GetPrivateProfileString("Basic_Threshold", "WeakShortTest_CG", "2000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.WeakShortTest_CG = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "WeakShortTest_CC", "2000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.WeakShortTest_CC = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "WeakShortTest_CC_Rsen", "57", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.WeakShortTest_CC_Rsen = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "WeakShortTest_CapShortTest", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.WeakShortTest_CapShortTest = fts_atoi(str);

    //////////////////////////////////////////////////////////// UniformityTest
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_Check_Tx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_CheckTx = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_Check_Rx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_CheckRx = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_Check_MinMax", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_CheckMinMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_Tx_Hole", "20", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_Tx_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_Rx_Hole", "20", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_Rx_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "UniformityTest_MinMax_Hole", "70", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.Uniformity_MinMax_Hole = fts_atoi(str);

    //////////////////////////////////////////////////////////// PanelDifferTest
    GetPrivateProfileString("Basic_Threshold", "PanelDifferTest_Min", "150", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDifferTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDifferTest_Max", "1000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDifferTest_Max = fts_atoi(str);

    //////////////////////////////////////////////////////////// PanelDiffer_UniformityTest
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_Check_Tx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_Check_Tx = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_Check_Rx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_Check_Rx = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_Check_MinMax", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_Check_MinMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_Tx_Hole", "20", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_Tx_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_Rx_Hole", "20", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_Rx_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "PanelDiffer_UniformityTest_MinMax_Hole", "70", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.PanelDiffer_UniformityTest_MinMax_Hole = fts_atoi(str);

    //////////////////////////////////////////////////////////// SITO_RawdataUniformityTest
    GetPrivateProfileString("Basic_Threshold", "SITO_RawdataUniformityTest_Check_Tx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SITO_RawdtaUniformityTest_Check_Tx = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SITO_RawdataUniformityTest_Check_Rx", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SITO_RawdtaUniformityTest_Check_Rx = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "SITO_RawdataUniformityTest_Tx_Hole", "10", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SITO_RawdtaUniformityTest_Tx_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "SITO_RawdataUniformityTest_Rx_Hole", "10", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.SITO_RawdtaUniformityTest_Rx_Hole = fts_atoi(str);

    //force touch
    //////////////////////////////////////////////////////////// ChannelNumTest
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_ChannelNumTest_ChannelNum", "10", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_ChannelNumTest_ChannelNum = fts_atoi(str);

    //////////////////////////////////////////////////////////// ForceTouch_SCapRawDataTest
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_OFF_Min", "3000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_OFF_Max", "15000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_OFF_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_ON_Min", "3000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_ON_Max", "15000", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_ON_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_SetWaterproof_OFF", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_OFF = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapRawDataTest_SetWaterproof_ON", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapRawDataTest_SetWaterproof_ON = fts_atoi(str);

    //////////////////////////////////////////////////////////// ForceTouch_SCapCbTest
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapCbTest_OFF_Min", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCbTest_OFF_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapCbTest_OFF_Max", "240", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCbTest_OFF_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapCbTest_ON_Min", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCbTest_ON_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_SCapCbTest_ON_Max", "240", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCbTest_ON_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ForceTouch_ScapCBTest_SetWaterproof_OFF", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_OFF = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_ScapCBTest_SetWaterproof_ON", "1", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_SCapCBTest_SetWaterproof_ON = fts_atoi(str);

    //////////////////////////////////////////////////////////// ForceTouch_WeakShortTest
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_WeakShortTest_CG", "1200", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_WeakShortTest_CG = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_WeakShortTest_CC", "1200", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.ForceTouch_WeakShortTest_CC = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ForceTouch_WeakShortTest_CapShortTest", "0", str, strIniFile);
    g_stCfg_FT3D47_BasicThreshold.bForceTouch_WeakShortTest_CapShortTest = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();

}

void init_detailthreshold_ft3d47(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_RawDataTest(ini);
    OnInit_DThreshold_SCapRawDataTest(ini);
    OnInit_DThreshold_SCapCbTest(ini);

    OnInit_DThreshold_ForceTouch_SCapRawDataTest(ini);
    OnInit_DThreshold_ForceTouch_SCapCbTest(ini);

    OnInit_DThreshold_RxLinearityTest(ini);
    OnInit_DThreshold_TxLinearityTest(ini);

    OnInit_DThreshold_PanelDifferTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft3d47(void)
{

    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////FACTORY_ID_TEST
    if ( g_stCfg_FT3D47_TestItem.FACTORY_ID_TEST == 1) {
        fts_set_testitem(Code_FT3D47_FACTORY_ID_TEST);
    }

    //////////////////////////////////////////////////Project Code Test
    if ( g_stCfg_FT3D47_TestItem.PROJECT_CODE_TEST == 1) {

        fts_set_testitem(Code_FT3D47_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////FW Version Test
    if ( g_stCfg_FT3D47_TestItem.FW_VERSION_TEST == 1) {

        fts_set_testitem(Code_FT3D47_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(Code_FT3D47_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CHANNEL_NUM_TEST
    if ( g_stCfg_FT3D47_TestItem.CHANNEL_NUM_TEST == 1) {

        fts_set_testitem(Code_FT3D47_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////NOISE_TEST
    if ( g_stCfg_FT3D47_TestItem.NOISE_TEST == 1) {

        fts_set_testitem(Code_FT3D47_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( g_stCfg_FT3D47_TestItem.RAWDATA_TEST == 1) {

        fts_set_testitem(Code_FT3D47_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////Rawdata Uniformity Test
    if ( g_stCfg_FT3D47_TestItem.UNIFORMITY_TEST == 1) {

        fts_set_testitem(Code_FT3D47_UNIFORMITY_TEST);
    }

    //////////////////////////////////////////////////SITO_Rawdata Uniformity Test
    if ( g_stCfg_FT3D47_TestItem.SITO_RAWDATA_UNIFORMITY_TEST == 1) {

        fts_set_testitem(Code_FT3D47_SITO_RAWDATA_UNIFORMITY_TEST);
    }

    //////////////////////////////////////////////////SCAP_CB_TEST
    if ( g_stCfg_FT3D47_TestItem.SCAP_CB_TEST == 1) {

        fts_set_testitem(Code_FT3D47_SCAP_CB_TEST);
    }

    //////////////////////////////////////////////////SCAP_RAWDATA_TEST
    if ( g_stCfg_FT3D47_TestItem.SCAP_RAWDATA_TEST == 1) {

        fts_set_testitem(Code_FT3D47_SCAP_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////WEAK_SHORT_CIRCUIT_TEST
    if ( g_stCfg_FT3D47_TestItem.WEAK_SHORT_CIRCUIT_TEST == 1) {

        fts_set_testitem(Code_FT3D47_WEAK_SHORT_CIRCUIT_TEST);
    }

    //////////////////////////////////////////////////panel differ_TEST
    if ( g_stCfg_FT3D47_TestItem.PANEL_DIFFER_TEST == 1) {

        fts_set_testitem(Code_FT3D47_PANELDIFFER_TEST);
    }
    //////////////////////////////////////////////////panel differ uniformity
    if ( g_stCfg_FT3D47_TestItem.PANEL_DIFFER_UNIFORMITY_TEST == 1) {

        fts_set_testitem(Code_FT3D47_PANELDIFFER_UNIFORMITY_TEST);
    }

    //force touch
    //////////////////////////////////////////////////FORCETOUCH_CHANNEL_NUM_TEST
    if ( g_stCfg_FT3D47_TestItem.FORCETOUCH_CHANNEL_NUM_TEST == 1) {

        fts_set_testitem(Code_FT3D47_FORCE_TOUCH_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////FORCETOUCH_RAWDATA_TEST
    if ( g_stCfg_FT3D47_TestItem.FORCETOUCH_RAWDATA_TEST == 1) {

        fts_set_testitem(Code_FT3D47_FORCE_TOUCH_SCAP_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////FORCETOUCH_CB_TEST
    if ( g_stCfg_FT3D47_TestItem.FORCETOUCH_CB_TEST == 1) {

        fts_set_testitem(Code_FT3D47_FORCE_TOUCH_SCAP_CB_TEST);
    }

    //////////////////////////////////////////////////FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST
    if ( g_stCfg_FT3D47_TestItem.FORCETOUCH_WEAK_SHORT_CIRCUIT_TEST == 1) {

        fts_set_testitem(Code_FT3D47_FORCE_TOUCH_WEAK_SHORT_CIRCUIT_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft3d47,
    .init_basicthreshold = init_basicthreshold_ft3d47,
    .init_detailthreshold = init_detailthreshold_ft3d47,
    .set_testitem_sequence  = set_testitem_sequence_ft3d47,
    .start_test = start_test_ft3d47,
};
#endif
