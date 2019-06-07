/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Focaltech_test_ft6x36.c
*
* Author: Software Development Team, AE
*
* Created: 2015-10-08
*
* Abstract: test item for FT6X36/FT3X07/FT6416/FT6426
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "../focaltech_test.h"

#if (FTS_CHIP_TEST_TYPE ==FT6X36_TEST)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/

/////////////////////////////////////////////////Reg
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define REG_PATTERN_5422        0x53
#define REG_MAPPING_SWITCH      0x54
#define REG_TX_NOMAPPING_NUM        0x55
#define REG_RX_NOMAPPING_NUM      0x56
#define REG_NORMALIZE_TYPE      0x16
#define REG_ScCbBuf0    0x4E
#define REG_ScWorkMode  0x44
#define REG_ScCbAddrR   0x45
#define REG_RawBuf0 0x36
#define REG_WATER_CHANNEL_SELECT 0x09
#define C6208_SCAN_ADDR 0x08
#define C6X36_CHANNEL_NUM   0x0A        //  1 Byte read and write (RW)  TP_Channel_Num  The maximum number of channels in VA area is 63
#define C6X36_KEY_NUM       0x0B        //  1 Byte read and write(RW)           TP_Key_Num      The maximum number of virtual keys independent channel is 63
#define C6X36_CB_ADDR_W 0x32            //    1 Byte    read and write(RW)  CB_addr     Normal mode CB-- address    
#define C6X36_CB_ADDR_R 0x33            //  1 Byte  read and write(RW)  CB_addr     Normal mode CB-- address
#define C6X36_CB_BUF  0x39                  //                   //Byte only read (RO)  CB_buf      Ò»The channel is 2 bytes, and the length is 2*N                 
#define C6X36_RAWDATA_ADDR  0x34    //  //1 Byte    read and write(RW)  RawData_addr        Rawdata--address                            
#define C6X36_RAWDATA_BUF   0x35
#define C6206_FACTORY_TEST_MODE         0xAE    //0: normal factory model, 1: production test factory mode 1 (using a single  +0 level of water scanning), 2: production test factory mode 2 (using a single ended + non waterproof scanning)
#define C6206_FACTORY_TEST_STATUS       0xAD
#define MAX_SCAP_CHANNEL_NUM        144//Single Chip 72; Double Chips 144
#define MAX_SCAP_KEY_NUM            8

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
enum WaterproofType {
    WT_NeedProofOnTest,
    WT_NeedProofOffTest,
    WT_NeedTxOnVal,
    WT_NeedRxOnVal,
    WT_NeedTxOffVal,
    WT_NeedRxOffVal,
};

struct stCfg_FT6X36_TestItem {
    bool FW_VERSION_TEST;
    bool FACTORY_ID_TEST;
    bool PROJECT_CODE_TEST;
    bool IC_VERSION_TEST;
    bool RAWDATA_TEST;
    bool CHANNEL_NUM_TEST;
    bool CHANNEL_SHORT_TEST;
    bool NOISE_TEST;
    bool CB_TEST;
    bool DELTA_CB_TEST;
    bool CHANNELS_DEVIATION_TEST;
    bool TWO_SIDES_DEVIATION_TEST;
    bool CB_DEVIATION_TEST;
    bool DIFFER_TEST;
    bool WEAK_SHORT_TEST;
    bool DIFFER_TEST2;
};

struct stCfg_FT6X36_BasicThreshold {
    u8 FW_VER_VALUE;
    u8 Factory_ID_Number;
    char Project_Code[32];
    int RawDataTest_Min;
    int RawDataTest_Max;
    u8 ChannelNumTest_ChannelNum;
    u8 ChannelNumTest_KeyNum;
    int ChannelShortTest_K1;
    int ChannelShortTest_K2;
    int ChannelShortTest_CB;
    int WeakShortThreshold;
    int NoiseTest_Max;
    int NoiseTest_Frames;
    int NoiseTest_Time;
    u8 NoiseTest_SampeMode;
    u8 NoiseTest_NoiseMode;
    u8 NoiseTest_ShowTip;
    int CBDeviationTest_Hole;
    int Differ_Ave_Hole;
    int Differ_Max_Hole;
    int CbTest_Min;
    int CbTest_Max;
    int DeltaCbTest_Base;
    int DeltaCbTest_Differ_Max;
    bool DeltaCbTest_Include_Key_Test;
    int DeltaCbTest_Key_Differ_Max;
    int DeltaCbTest_Deviation_S1;
    int DeltaCbTest_Deviation_S2;
    int DeltaCbTest_Deviation_S3;
    int DeltaCbTest_Deviation_S4;
    int DeltaCbTest_Deviation_S5;
    int DeltaCbTest_Deviation_S6;
    bool DeltaCbTest_Set_Critical;
    int DeltaCbTest_Critical_S1;
    int DeltaCbTest_Critical_S2;
    int DeltaCbTest_Critical_S3;
    int DeltaCbTest_Critical_S4;
    int DeltaCbTest_Critical_S5;
    int DeltaCbTest_Critical_S6;

    int ChannelsDeviationTest_Deviation_S1;
    int ChannelsDeviationTest_Deviation_S2;
    int ChannelsDeviationTest_Deviation_S3;
    int ChannelsDeviationTest_Deviation_S4;
    int ChannelsDeviationTest_Deviation_S5;
    int ChannelsDeviationTest_Deviation_S6;
    bool ChannelsDeviationTest_Set_Critical;
    int ChannelsDeviationTest_Critical_S1;
    int ChannelsDeviationTest_Critical_S2;
    int ChannelsDeviationTest_Critical_S3;
    int ChannelsDeviationTest_Critical_S4;
    int ChannelsDeviationTest_Critical_S5;
    int ChannelsDeviationTest_Critical_S6;

    int TwoSidesDeviationTest_Deviation_S1;
    int TwoSidesDeviationTest_Deviation_S2;
    int TwoSidesDeviationTest_Deviation_S3;
    int TwoSidesDeviationTest_Deviation_S4;
    int TwoSidesDeviationTest_Deviation_S5;
    int TwoSidesDeviationTest_Deviation_S6;
    bool TwoSidesDeviationTest_Set_Critical;
    int TwoSidesDeviationTest_Critical_S1;
    int TwoSidesDeviationTest_Critical_S2;
    int TwoSidesDeviationTest_Critical_S3;
    int TwoSidesDeviationTest_Critical_S4;
    int TwoSidesDeviationTest_Critical_S5;
    int TwoSidesDeviationTest_Critical_S6;

};

enum enumTestItem_FT6X36 {
    Code_FT6X36_ENTER_FACTORY_MODE,//All IC are required to test items
    Code_FT6X36_DOWNLOAD,//All IC are required to test items
    Code_FT6X36_UPGRADE,//All IC are required to test items
    Code_FT6X36_FACTORY_ID_TEST,
    Code_FT6X36_PROJECT_CODE_TEST,
    Code_FT6X36_FW_VERSION_TEST,
    Code_FT6X36_IC_VERSION_TEST,
    Code_FT6X36_RAWDATA_TEST,
    Code_FT6X36_CHANNEL_NUM_TEST,
    Code_FT6X36_CHANNEL_SHORT_TEST,
    Code_FT6X36_INT_PIN_TEST,
    Code_FT6X36_RESET_PIN_TEST,
    Code_FT6X36_NOISE_TEST,
    Code_FT6X36_CB_TEST,
    Code_FT6X36_DELTA_CB_TEST,
    Code_FT6X36_CHANNELS_DEVIATION_TEST,
    Code_FT6X36_TWO_SIDES_DEVIATION_TEST,
    Code_FT6X36_FPC_SHORT_TEST,
    Code_FT6X36_FPC_OPEN_TEST,
    Code_FT6X36_SREF_OPEN_TEST,
    Code_FT6X36_TE_TEST,
    Code_FT6X36_CB_DEVIATION_TEST,
    Code_FT6X36_WRITE_CONFIG,//All IC are required to test items
    Code_FT6X36_DIFFER_TEST,
    Code_FT6X36_WEAK_SHORT_TEST,
    Code_FT6X36_DIFFER_TEST2,
    Code_FT6X36_K1_DIFFER_TEST,
};

enum PROOF_TYPE {
    Proof_Normal,               //mode 0
    Proof_Level0,                   //mode 1
    Proof_NoWaterProof,       //mode 2
};

/*****************************************************************************
* Static variables
*****************************************************************************/
static int m_RawData[MAX_SCAP_CHANNEL_NUM] = {0};
static unsigned char m_ucTempData[MAX_SCAP_CHANNEL_NUM * 2] = {0};
static int m_CbData[MAX_SCAP_CHANNEL_NUM] = {0};
static int m_TempCbData[MAX_SCAP_CHANNEL_NUM] = {0};
static int m_DeltaCbData[MAX_SCAP_CHANNEL_NUM] = {0};
static int m_DeltaCb_DifferData[MAX_SCAP_CHANNEL_NUM] = {0};
/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
struct stCfg_FT6X36_TestItem g_stCfg_FT6X36_TestItem;
struct stCfg_FT6X36_BasicThreshold g_stCfg_FT6X36_BasicThreshold;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
//////////////////////////////////////////////Communication function
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelChannels(unsigned char *pPanelRows);
static unsigned char GetPanelKeys(unsigned char *pPanelCols);
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
static void Save_Test_Data(int iData[MAX_SCAP_CHANNEL_NUM], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void ShowRawData(void);
unsigned char FT6X36_TestItem_EnterFactoryMode(void);
unsigned char FT6X36_TestItem_RawDataTest(bool *bTestResult);
unsigned char FT6X36_TestItem_CbTest(bool *bTestResult);
unsigned char FT6X36_TestItem_DeltaCbTest(bool *bTestResult);

/************************************************************************
* Name: start_test_ft6x36
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
bool start_test_ft6x36(void)
{
    bool bTestResult = true;
    bool bTempResult = 1;
    unsigned char ReCode = 0;
    int iItemCount = 0;

    //--------------1. Init part
    if (init_test() < 0) {
        FTS_TEST_SAVE_INFO("\n [focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == test_data.test_num)
        bTestResult = false;

    for (iItemCount = 0; iItemCount < test_data.test_num; iItemCount++) {
        test_data.test_item_code = test_data.test_item[iItemCount].itemcode;

        ///////////////////////////////////////////////////////FT6X36_ENTER_FACTORY_MODE
        if (Code_FT6X36_ENTER_FACTORY_MODE == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT6X36_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT6X36_RAWDATA_TEST
        if (Code_FT6X36_RAWDATA_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT6X36_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT6X36_CB_TEST
        if (Code_FT6X36_CB_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT6X36_TestItem_CbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult)) {
                bTestResult = false;
                test_data.test_item[iItemCount].testresult = RESULT_NG;
            } else
                test_data.test_item[iItemCount].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////Code_FT6X36_DELTA_CB_TEST
        if (Code_FT6X36_DELTA_CB_TEST == test_data.test_item[iItemCount].itemcode
           ) {
            ReCode = FT6X36_TestItem_DeltaCbTest(&bTempResult);
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
* Name: FT6X36_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT6X36_TestItem_EnterFactoryMode(void)
{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;  //If failed, repeat 5 times.
    int i ;

    sys_delay(150);
    for (i = 1; i <= iRedo; i++) {
        ReCode = enter_factory_mode();
        if (ERROR_CODE_OK != ReCode) {
            FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode...");
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

    return ReCode;
}


/************************************************************************
* Name: FT6X36_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT6X36_TestItem_RawDataTest(bool *bTestResult)
{
    int i = 0;
    //bool bFlag = true;
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    int RawDataMin = 0, RawDataMax = 0;

    int iNgNum = 0;
    int iMax = 0, iMin = 0, iAvg = 0;

    RawDataMin = g_stCfg_FT6X36_BasicThreshold.RawDataTest_Min;
    RawDataMax = g_stCfg_FT6X36_BasicThreshold.RawDataTest_Max;

    FTS_TEST_SAVE_INFO("\n \r\n\r\n==============================Test Item: -------- RawData Test \r");

    for (i = 0; i < 3; i++) {
        ReCode = write_reg(C6206_FACTORY_TEST_MODE, Proof_Normal);
        if (ERROR_CODE_OK == ReCode)
            ReCode = StartScan();
        if (ERROR_CODE_OK == ReCode)break;
    }

    ReCode = GetRawData();

    if (ReCode == ERROR_CODE_OK) { //Read the RawData and calculate the Differ value
        FTS_TEST_SAVE_INFO("\n \r\n//======= Test Data:  ");
        ShowRawData();
    } else {
        FTS_TEST_SAVE_INFO("\n \r\nRawData Test is Error. Failed to get Raw Data!!");
        btmpresult = false;//Can not get RawData, is also considered  NG
        goto TEST_END;
    }

    //----------------------------------------------------------Judge over the range of rawData
    iNgNum = 0;
    iMax = m_RawData[0];
    iMin = m_RawData[0];
    iAvg = 0;

    for (i = 0; i < test_data.screen_param.channels_num + test_data.screen_param.key_num; i++) {
        RawDataMin = test_data.scap_detail_thr.RawDataTest_Min[i];//Fetch detail threshold
        RawDataMax = test_data.scap_detail_thr.RawDataTest_Max[i];//Fetch detail threshold
        if (m_RawData[i] < RawDataMin || m_RawData[i] > RawDataMax) {
            btmpresult = false;

            if (iNgNum == 0) FTS_TEST_SAVE_INFO("\n \r\n//======= NG Data: \r");

            if (i < test_data.screen_param.channels_num)
                FTS_TEST_SAVE_INFO("\n Ch_%02d: %d Set_Range=(%d, %d) ,	", i + 1, m_RawData[i], RawDataMin, RawDataMax);
            else
                FTS_TEST_SAVE_INFO("\n Key_%d: %d Set_Range=(%d, %d) ,	", i + 1 - test_data.screen_param.channels_num, m_RawData[i], RawDataMin, RawDataMax);
            if (iNgNum % 6 == 0) {
                FTS_TEST_SAVE_INFO("\n \r\n" );
            }

            iNgNum++;
        }

        //Just calculate the maximum minimum average value
        iAvg += m_RawData[i];
        if (iMax < m_RawData[i])iMax = m_RawData[i];
        if (iMin > m_RawData[i])iMin = m_RawData[i];

    }

    iAvg /= test_data.screen_param.channels_num + test_data.screen_param.key_num;
    FTS_TEST_SAVE_INFO("\n \r\n\r\n// Max Raw Value: %d, Min Raw Value: %d, Deviation Value: %d, Average Value: %d", iMax, iMin, iMax - iMin, iAvg);

    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Collect test data and save CSV file.

    Save_Test_Data(m_RawData, 0, 1, test_data.screen_param.channels_num + test_data.screen_param.key_num, 1);

TEST_END:
    if (btmpresult) {
        * bTestResult = true;
        FTS_TEST_SAVE_INFO("\n\n\n/==========RawData Test is OK!\r");
    } else {
        * bTestResult = false;
        FTS_TEST_SAVE_INFO("\n\n\n/==========RawData Test is NG!\r");
    }
    return ReCode;
}

/************************************************************************
* Name: FT6X36_TestItem_CbTest
* Brief:  TestItem: CB Test. Check if SCAP CB is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT6X36_TestItem_CbTest(bool *bTestResult)
{
    int readlen = 0;

    bool btmpresult = true;
    int iCbMin = 0, iCbMax = 0;
    unsigned char chOldMode = 0;

    //unsigned char WaterProofResult=0;
    unsigned char ReCode = ERROR_CODE_OK;
    u8 pReadData[300] = {0};
    unsigned char I2C_wBuffer[1];

    int iNgNum = 0;
    int iMax = 0, iMin = 0, iAvg = 0;

    int i = 0;

    memset(m_CbData, 0, sizeof(m_CbData));
    readlen = test_data.screen_param.channels_num + test_data.screen_param.key_num;

    FTS_TEST_SAVE_INFO("\n \r\n\r\n==============================Test Item: --------  CB Test");

    iCbMin = g_stCfg_FT6X36_BasicThreshold.CbTest_Min;
    iCbMax = g_stCfg_FT6X36_BasicThreshold.CbTest_Max;

    ReCode = read_reg( C6206_FACTORY_TEST_MODE, &chOldMode );

    for (i = 0; i < 3; i++) {
        ReCode = write_reg(C6206_FACTORY_TEST_MODE, Proof_NoWaterProof);
        if (ERROR_CODE_OK == ReCode)
            ReCode = StartScan();
        if (ERROR_CODE_OK == ReCode)break;
    }

    if ((ERROR_CODE_OK != ReCode)/* || (1 != WaterProofResult)*/) {
        btmpresult = false;
        FTS_TEST_SAVE_INFO("\n \r\n\r\n//=========  CB test Failed!");
    } else {
        FTS_TEST_SAVE_INFO("\n \r\n\r\nGet Proof_NoWaterProof CB Data...");


        //Waterproof CB
        I2C_wBuffer[0] = 0x39;
        ReCode = write_reg( 0x33, 0 );
        ReCode = fts_i2c_read_write(I2C_wBuffer, 1, pReadData, readlen * 2 );

        for (i = 0; i < readlen; i++) {
            m_TempCbData[i] = (unsigned short)(pReadData[i * 2] << 8 | pReadData[i * 2 + 1]);
            if (i == 0) { //Half
                FTS_TEST_SAVE_INFO("\n \r\n\r\n//======= CB Data: ");
                FTS_TEST_SAVE_INFO("\n \r\nLeft Channel:	");
            } else if ( i * 2 == test_data.screen_param.channels_num) {
                FTS_TEST_SAVE_INFO("\n \r\nRight Channel:	");
            } else if ( i ==  test_data.screen_param.channels_num) {
                FTS_TEST_SAVE_INFO("\n \r\nKey: ");
            }
            FTS_TEST_SAVE_INFO("%3d,  ", m_TempCbData[i]);

        }
    }
    FTS_TEST_SAVE_INFO("\n\n");

    ////////////////////////////Single waterproof
    FTS_TEST_SAVE_INFO("\n Proof_Level0 CB Test...\r");
    for (i = 0; i < 3; i++) {
        ReCode = write_reg(C6206_FACTORY_TEST_MODE, Proof_Level0);
        if (ERROR_CODE_OK == ReCode)
            ReCode = StartScan();
        if (ERROR_CODE_OK == ReCode)break;
    }
    if ((ERROR_CODE_OK != ReCode)/* || (1 != WaterProofResult)*/) {
        btmpresult = false;
        FTS_TEST_SAVE_INFO("\n \r\n\r\n//========= CB test Failed!");
    } else {
        FTS_TEST_SAVE_INFO("\n \r\n\r\nGet Proof_Level0 CB Data...");

        //u8 pReadData[300] = {0};
        //unsigned char I2C_wBuffer[1];
        //Waterproof CB
        I2C_wBuffer[0] = 0x39;
        ReCode = write_reg( 0x33, 0 );
        ReCode = fts_i2c_read_write(I2C_wBuffer, 1, pReadData, readlen * 2 );

        for (i = 0; i < readlen; i++) {
            m_CbData[i] = (unsigned short)(pReadData[i * 2] << 8 | pReadData[i * 2 + 1]);

        }

        ReCode = write_reg( C6206_FACTORY_TEST_MODE, chOldMode );

        //----------------------------------------------------------Judge whether or not to exceed the threshold
        iNgNum = 0;
        iMax = m_TempCbData[0];
        iMin = m_TempCbData[0];
        iAvg = 0;

        for (i = 0; i < test_data.screen_param.channels_num + test_data.screen_param.key_num; i++) {

            iCbMin = test_data.scap_detail_thr.CbTest_Min[i];//Fetch detail threshold
            iCbMax = test_data.scap_detail_thr.CbTest_Max[i];//Fetch detail threshold

            if (m_TempCbData[i] < iCbMin || m_TempCbData[i] > iCbMax) {
                if (iNgNum == 0) {
                    FTS_TEST_SAVE_INFO("\n \r\n//======= NG Data: \r");
                }
                btmpresult = false;
                if (i < test_data.screen_param.channels_num)
                    FTS_TEST_SAVE_INFO("\n Ch_%02d: %d Set_Range=(%d, %d) ,	", i + 1, m_TempCbData[i], iCbMin, iCbMax);
                else
                    FTS_TEST_SAVE_INFO("\n Key_%d: %d Set_Range=(%d, %d),	", i + 1 - test_data.screen_param.channels_num, m_TempCbData[i], iCbMin, iCbMax);
                if (iNgNum % 6 == 0) {
                    FTS_TEST_SAVE_INFO("\n \r");
                }

                iNgNum++;
            }

            // calculate the maximum minimum average value
            iAvg += m_TempCbData[i];
            if (iMax < m_TempCbData[i])iMax = m_TempCbData[i];
            if (iMin > m_TempCbData[i])iMin = m_TempCbData[i];

        }

        iAvg /= test_data.screen_param.channels_num + test_data.screen_param.key_num;
        FTS_TEST_SAVE_INFO("\n \r\n\r\n// Max CB Value: %d, Min CB Value: %d, Deviation Value: %d, Average Value: %d", iMax, iMin, iMax - iMin, iAvg);

        if (btmpresult) {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n//CB Test is OK!\r");
            * bTestResult = 1;
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n//CB Test is NG!\r");
            * bTestResult = 0;
        }
    }
    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Collect test data and save CSV file.

    Save_Test_Data(m_TempCbData, 0, 1, test_data.screen_param.channels_num + test_data.screen_param.key_num, 1);
    return ReCode;

}

/************************************************************************
* Name: FT6X36_TestItem_DeltaCbTest
* Brief:  TestItem: Delta CB Test. Check if SCAP Delta CB is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT6X36_TestItem_DeltaCbTest(bool *bTestResult)
{
    bool btmpresult = true;
    int readlen = test_data.screen_param.channels_num + test_data.screen_param.key_num;
    int i = 0;

    ////////////The maximum Delta_Ci and minimum Delta_Ci difference is less than the preset value
    int Delta_Ci_Differ = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S1;
    int Delta_Ci_Differ_S2 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S2;
    int Delta_Ci_Differ_S3 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S3;
    int Delta_Ci_Differ_S4 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S4;
    int Delta_Ci_Differ_S5 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S5;
    int Delta_Ci_Differ_S6 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S6;

    int Delta_Min = 0, Delta_Max = 0;

    int Critical_Delta_S1 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S1;
    int Critical_Delta_S2 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S2;
    int Critical_Delta_S3 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S3;
    int Critical_Delta_S4 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S4;
    int Critical_Delta_S5 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S5;
    int Critical_Delta_S6 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S6;

    bool bUseCriticalValue = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Set_Critical;

    bool bCriticalResult = false;


    /////////////////////////////new test
    int Sort1Min, Sort2Min, Sort3Min, Sort4Min, Sort5Min, Sort6Min;
    int Sort1Max, Sort2Max, Sort3Max, Sort4Max, Sort5Max, Sort6Max;
    int Sort1Min_ChNum, Sort2Min_ChNum, Sort3Min_ChNum, Sort4Min_ChNum, Sort5Min_ChNum, Sort6Min_ChNum;
    int Sort1Max_ChNum, Sort2Max_ChNum, Sort3Max_ChNum, Sort4Max_ChNum, Sort5Max_ChNum, Sort6Max_ChNum;
    bool bUseSort1 = false;
    bool bUseSort2 = false;
    bool bUseSort3 = false;
    bool bUseSort4 = false;
    bool bUseSort5 = false;
    bool bUseSort6 = false;

    int Num = 0;

    int Key_Delta_Max = 0;
    int SetKeyMax = 0;
    int set_Delta_Cb_Max = 0;

    FTS_TEST_SAVE_INFO("\n \r\n\r\n==============================Test Item: -------- Delta CB Test ");

    for (i = 0; i < readlen; i++) {
        m_DeltaCbData[i] = m_TempCbData[i] - m_CbData[i];
        if (i == 0) { //Half
            FTS_TEST_SAVE_INFO("\n \r\n\r\n//======= Delta CB Data: ");
            FTS_TEST_SAVE_INFO("\n \r\nLeft Channel:	");
        } else if ( i * 2 == test_data.screen_param.channels_num) {
            FTS_TEST_SAVE_INFO("\n \r\nRight Channel:	");
        } else if ( i ==  test_data.screen_param.channels_num) {
            FTS_TEST_SAVE_INFO("\n \r\nKey: ");
        }
        FTS_TEST_SAVE_INFO(" %3d  ", m_DeltaCbData[i]);

    }
    FTS_TEST_SAVE_INFO("\n\n");

    /////////////////////////Delta CB Differ
    for (i = 0; i < readlen; i++) {
        m_DeltaCb_DifferData[i] = m_DeltaCbData[i] - test_data.scap_detail_thr.DeltaCbTest_Base[i];

        if (i == 0) { //Half
            FTS_TEST_SAVE_INFO("\n \r\n\r\n//======= Differ Data of Delta CB: ");
            FTS_TEST_SAVE_INFO("\n \r\nLeft Channel:	");
        } else if ( i * 2 == test_data.screen_param.channels_num) {
            FTS_TEST_SAVE_INFO("\n \r\nRight Channel:	");
        } else if ( i ==  test_data.screen_param.channels_num) {
            FTS_TEST_SAVE_INFO("\n \r\nKey:  ");
        }
        FTS_TEST_SAVE_INFO("%3d  ", m_DeltaCb_DifferData[i]);
    }
    FTS_TEST_SAVE_INFO("\n \r\n\r");

    //The maximum Delta_Ci and minimum Delta_Ci difference is less than the preset value
    Delta_Ci_Differ = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S1;
    Delta_Ci_Differ_S2 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S2;
    Delta_Ci_Differ_S3 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S3;
    Delta_Ci_Differ_S4 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S4;
    Delta_Ci_Differ_S5 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S5;
    Delta_Ci_Differ_S6 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S6;

    Delta_Min = 0;
    Delta_Max = 0;

    Critical_Delta_S1 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S1;
    Critical_Delta_S2 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S2;
    Critical_Delta_S3 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S3;
    Critical_Delta_S4 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S4;
    Critical_Delta_S5 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S5;
    Critical_Delta_S6 = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S6;

    bUseCriticalValue = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Set_Critical;

    bCriticalResult = false;


    /////////////////////////////new test
    bUseSort1 = false;
    bUseSort2 = false;
    bUseSort3 = false;
    bUseSort4 = false;
    bUseSort5 = false;
    bUseSort6 = false;

    Num = 0;

    Sort1Min_ChNum = Sort2Min_ChNum = Sort3Min_ChNum = Sort4Min_ChNum = Sort5Min_ChNum = Sort6Min_ChNum = 0;
    Sort1Max_ChNum = Sort2Max_ChNum = Sort3Max_ChNum = Sort4Max_ChNum = Sort5Max_ChNum = Sort6Max_ChNum = 0;

    Sort1Min = Sort2Min = Sort3Min = Sort4Min = Sort5Min  = Sort6Min = 1000;
    Sort1Max = Sort2Max = Sort3Max = Sort4Max = Sort5Max = Sort6Max = -1000;

    for (i = 0; i < test_data.screen_param.channels_num/*readlen*/; i++) {
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 1) {
            bUseSort1 = true;
            if (m_DeltaCb_DifferData[i] < Sort1Min) {
                Sort1Min = m_DeltaCb_DifferData[i];
                Sort1Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort1Max) {
                Sort1Max = m_DeltaCb_DifferData[i];
                Sort1Max_ChNum = i;
            }
        }
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 2) {
            bUseSort2 = true;
            if (m_DeltaCb_DifferData[i] < Sort2Min) {
                Sort2Min = m_DeltaCb_DifferData[i];
                Sort2Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort2Max) {
                Sort2Max = m_DeltaCb_DifferData[i];
                Sort2Max_ChNum = i;
            }
        }
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 3) {
            bUseSort3 = true;
            if (m_DeltaCb_DifferData[i] < Sort3Min) {
                Sort3Min = m_DeltaCb_DifferData[i];
                Sort3Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort3Max) {
                Sort3Max = m_DeltaCb_DifferData[i];
                Sort3Max_ChNum = i;
            }
        }
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 4) {
            bUseSort4 = true;
            if (m_DeltaCb_DifferData[i] < Sort4Min) {
                Sort4Min = m_DeltaCb_DifferData[i];
                Sort4Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort4Max) {
                Sort4Max = m_DeltaCb_DifferData[i];
                Sort4Max_ChNum = i;
            }
        }
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 5) {
            bUseSort5 = true;
            if (m_DeltaCb_DifferData[i] < Sort5Min) {
                Sort5Min = m_DeltaCb_DifferData[i];
                Sort5Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort5Max) {
                Sort5Max = m_DeltaCb_DifferData[i];
                Sort5Max_ChNum = i;
            }
        }
        if (test_data.scap_detail_thr.DeltaCxTest_Sort[i] == 6) {
            bUseSort6 = true;
            if (m_DeltaCb_DifferData[i] < Sort6Min) {
                Sort6Min = m_DeltaCb_DifferData[i];
                Sort6Min_ChNum = i;
            }
            if (m_DeltaCb_DifferData[i] > Sort6Max) {
                Sort6Max = m_DeltaCb_DifferData[i];
                Sort6Max_ChNum = i;
            }
        }


    }
    if (bUseSort1) {
        if (Delta_Ci_Differ <= Sort1Max - Sort1Min) {
            if (bUseCriticalValue) {
                if (Sort1Max - Sort1Min >= Critical_Delta_S1) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort1Max) > focal_abs(Sort1Min))
                        Num = Sort1Max_ChNum;
                    else
                        Num = Sort1Min_ChNum;
                    //SetCriticalMsg(Num);

                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort1: %d",
                                   Sort1Max, Sort1Min, Sort1Max - Sort1Min, Critical_Delta_S1);
            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort1: %d",
                                   Sort1Max, Sort1Min, Sort1Max - Sort1Min, Delta_Ci_Differ);
            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort1: %d",
                               Sort1Max, Sort1Min, Sort1Max - Sort1Min, Delta_Ci_Differ);
        }

        FTS_TEST_SAVE_INFO("\n \r\nMax Deviation,Sort1: %d, ", Sort1Max - Sort1Min);
    }
    if (bUseSort2) {

        if (Delta_Ci_Differ_S2 <= Sort2Max - Sort2Min) {
            if (bUseCriticalValue) {
                if (Sort2Max - Sort2Min >= Critical_Delta_S2) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort2Max) > focal_abs(Sort2Min))
                        Num = Sort2Max_ChNum;
                    else
                        Num = Sort2Min_ChNum;
                    //SetCriticalMsg(Num);
                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort2: %d",
                                   Sort2Max, Sort2Min, Sort2Max - Sort2Min, Critical_Delta_S2);
            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort2: %d",
                                   Sort2Max, Sort2Min, Sort2Max - Sort2Min, Delta_Ci_Differ_S2);
            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort2: %d",
                               Sort2Max, Sort2Min, Sort2Max - Sort2Min, Delta_Ci_Differ_S2);
        }

        FTS_TEST_SAVE_INFO("\n \r\nSort2: %d, ", Sort2Max - Sort2Min);
    }
    if (bUseSort3) {

        if (Delta_Ci_Differ_S3 <= Sort3Max - Sort3Min) {
            if (bUseCriticalValue) {
                if (Sort3Max - Sort3Min >= Critical_Delta_S3) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort3Max) > focal_abs(Sort3Min))
                        Num = Sort3Max_ChNum;
                    else
                        Num = Sort3Min_ChNum;
                    //SetCriticalMsg(Num);
                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort3: %d",
                                   Sort3Max, Sort3Min, Sort3Max - Sort3Min, Critical_Delta_S3);
            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort3: %d",
                                   Sort3Max, Sort3Min, Sort3Max - Sort3Min, Delta_Ci_Differ_S3);

            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort3: %d",
                               Sort3Max, Sort3Min, Sort3Max - Sort3Min, Delta_Ci_Differ_S3);

        }
        FTS_TEST_SAVE_INFO("\n \r\nSort3: %d, ", Sort3Max - Sort3Min);
    }
    if (bUseSort4) {
        if (Delta_Ci_Differ_S4 <= Sort4Max - Sort4Min) {
            if (bUseCriticalValue) {
                if (Sort4Max - Sort4Min >= Critical_Delta_S4) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort4Max) > focal_abs(Sort4Min))
                        Num = Sort4Max_ChNum;
                    else
                        Num = Sort4Min_ChNum;
                    //SetCriticalMsg(Num);
                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort4: %d",
                                   Sort4Max, Sort4Min, Sort4Max - Sort4Min, Critical_Delta_S4);

            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort4: %d",
                                   Sort4Max, Sort4Min, Sort4Max - Sort4Min, Delta_Ci_Differ_S4);

            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort4: %d",
                               Sort4Max, Sort4Min, Sort4Max - Sort4Min, Delta_Ci_Differ_S4);

        }
        FTS_TEST_SAVE_INFO("\n \r\nSort4: %d, ", Sort4Max - Sort4Min);
    }
    if (bUseSort5) {
        if (Delta_Ci_Differ_S5 <= Sort5Max - Sort5Min) {
            if (bUseCriticalValue) {
                if (Sort5Max - Sort5Min >= Critical_Delta_S5) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort5Max) > focal_abs(Sort5Min))
                        Num = Sort5Max_ChNum;
                    else
                        Num = Sort5Min_ChNum;
                    //SetCriticalMsg(Num);
                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort5: %d",
                                   Sort5Max, Sort5Min, Sort5Max - Sort5Min, Critical_Delta_S5);

            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort5: %d",
                                   Sort5Max, Sort5Min, Sort5Max - Sort5Min, Delta_Ci_Differ_S5);

            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort5: %d",
                               Sort5Max, Sort5Min, Sort5Max - Sort5Min, Delta_Ci_Differ_S5);

        }
        FTS_TEST_SAVE_INFO("\n \r\nSort5: %d, ", Sort5Max - Sort5Min);
    }
    if (bUseSort6) {
        if (Delta_Ci_Differ_S6 <= Sort6Max - Sort6Min) {
            if (bUseCriticalValue) {
                if (Sort6Max - Sort6Min >= Critical_Delta_S6) {
                    btmpresult = false;
                } else {
                    if (focal_abs(Sort6Max) > focal_abs(Sort6Min))
                        Num = Sort6Max_ChNum;
                    else
                        Num = Sort6Min_ChNum;
                    //SetCriticalMsg(Num);
                    bCriticalResult = true;
                }
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Critical Deviation of Sort6: %d",
                                   Sort6Max, Sort6Min, Sort6Max - Sort6Min, Critical_Delta_S6);
            } else {
                btmpresult = false;
                FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort6: %d",
                                   Sort6Max, Sort6Min, Sort6Max - Sort6Min, Delta_Ci_Differ_S6);

            }
        } else {
            FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition1: Max of Delta CB_Differ: %d, Min of Delta CB_Differ: %d, Get Deviation: %d, Set Deviation of Sort6: %d",
                               Sort6Max, Sort6Min, Sort6Max - Sort6Min, Delta_Ci_Differ_S6);

        }
        FTS_TEST_SAVE_INFO("\n \r\nSort6: %d, ", Sort6Max - Sort6Min);
    }

    /////////////////////Max Delta_Ci cannot exceed default values

    Delta_Min = Delta_Max = focal_abs(m_DeltaCb_DifferData[0]);
    for (i = 1; i < test_data.screen_param.channels_num/*readlen*/; i++) {
        if (focal_abs(m_DeltaCb_DifferData[i]) < Delta_Min) {
            Delta_Min = focal_abs(m_DeltaCb_DifferData[i]);
        }
        if (focal_abs(m_DeltaCb_DifferData[i]) > Delta_Max) {
            Delta_Max = focal_abs(m_DeltaCb_DifferData[i]);
        }
    }

    set_Delta_Cb_Max = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Differ_Max;
    if (set_Delta_Cb_Max < focal_abs(Delta_Max)) {
        btmpresult = false;
    }
    FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition2: Get Max Differ Data of Delta_CB(abs): %d, Set Max Differ Data of Delta_CB(abs): %d", Delta_Max, set_Delta_Cb_Max);


    if (g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Include_Key_Test) {

        SetKeyMax = g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Key_Differ_Max;

        Key_Delta_Max = focal_abs(m_DeltaCb_DifferData[test_data.screen_param.channels_num]);
        for (i = test_data.screen_param.channels_num; i < test_data.screen_param.channels_num + test_data.screen_param.key_num; i++) {
            if (focal_abs(m_DeltaCb_DifferData[i]) > Key_Delta_Max) {
                Key_Delta_Max = focal_abs(m_DeltaCb_DifferData[i]);
            }
        }
        if (SetKeyMax <= Key_Delta_Max ) {
            btmpresult = false;
        }
        FTS_TEST_SAVE_INFO("\n \r\n\r\n// Condition3: Include Key Test, Get Max Key Data: %d, Set Max Key Data: %d", Key_Delta_Max, SetKeyMax);
    }

    FTS_TEST_SAVE_INFO("\n \r\nMax Differ Data of Delta_CB(abs): %d ", Delta_Max);

    if (bCriticalResult && btmpresult) {
        FTS_TEST_SAVE_INFO("\n \r\n\r\nDelta CB Test has Critical Result(TBD)!");
    }
    ///////////////////////////////////////////////////////Delta Ci End
    if (btmpresult) {
        FTS_TEST_SAVE_INFO("\n\n\n/==========Delta CB Test is OK!\r");

        if (bCriticalResult)
            * bTestResult = 2;
        else
            * bTestResult = 1;
    } else {
        FTS_TEST_SAVE_INFO("\n\n\n/==========Delta CB Test is NG!\r");
        * bTestResult = 0;
    }
    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Collect test data and save CSV file.

    Save_Test_Data(m_DeltaCbData, 0, 1, test_data.screen_param.channels_num + test_data.screen_param.key_num, 1);

    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Collect test data and save CSV file.

    Save_Test_Data(m_DeltaCb_DifferData, 0, 1, test_data.screen_param.channels_num + test_data.screen_param.key_num, 2);

    //GetDeltaCiDataMsg();//Collection of Ci Data Delta, into the CSV file
    return 0;
}
/************************************************************************
* Name: GetPanelChannels(Same function name as FT_MultipleTest GetChannelNum)
* Brief:  Get row of TP
* Input: none
* Output: pPanelChannels
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelChannels(unsigned char *pPanelChannels)
{
    return read_reg(C6X36_CHANNEL_NUM, pPanelChannels);
}

/************************************************************************
* Name: GetPanelKeys(Same function name as FT_MultipleTest GetKeyNum)
* Brief:  get column of TP
* Input: none
* Output: pPanelKeys
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelKeys(unsigned char *pPanelKeys)
{
    return read_reg(C6X36_KEY_NUM, pPanelKeys);
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
    unsigned char RegVal = 0x01;
    unsigned int times = 0;
    const unsigned int MaxTimes = 500/*20*/;     // The longest wait 160ms
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    ReCode = read_reg(C6208_SCAN_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK) {
        RegVal = 0x01;      //Top bit position 1, start scan
        ReCode = write_reg(C6208_SCAN_ADDR, RegVal);
        if (ReCode == ERROR_CODE_OK) {
            while (times++ < MaxTimes) {    //Wait for the scan to complete
                sys_delay(8);    //8ms
                ReCode = read_reg(C6208_SCAN_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK) {
                    if (RegVal == 0) {
                        //ReCode == write_reg(0x01, 0x00);
                        break;
                    }
                } else {
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
    }

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
    unsigned short BytesNumInTestMode1 = 0;

    int i = 0;

    BytesNumInTestMode1 = ByteNum;

    //***********************************************************Read raw data in test mode1
    I2C_wBuffer[0] = C6X36_RAWDATA_ADDR;//Rawdata start addr register;
    I2C_wBuffer[1] = 0;//start index
    ReCode = fts_i2c_read_write(I2C_wBuffer, 2, NULL, 0);//   set rawdata start addr

    if ((ReCode == ERROR_CODE_OK)) {
        if (ReCode == ERROR_CODE_OK) {
            I2C_wBuffer[0] = C6X36_RAWDATA_BUF; //rawdata buffer addr register;

            ReCode = fts_i2c_read_write(I2C_wBuffer, 1, m_ucTempData, BytesNumInTestMode1);

        }
    }


    if (ReCode == ERROR_CODE_OK) {
        for (i = 0; i < (ByteNum >> 1); i++) {
            pRevBuffer[i] = (m_ucTempData[i << 1] << 8) + m_ucTempData[(i << 1) + 1];
        }
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
static void Save_Test_Data(int iData[MAX_SCAP_CHANNEL_NUM], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
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
                iLen = sprintf(test_data.tmp_buffer, "%d, \n",  iData[j]);
            else
                iLen = sprintf(test_data.tmp_buffer, "%d, ", iData[j]);

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
    unsigned char ReCode;
    unsigned char rBuffer[1]; //= new unsigned char;

    //m_strCurrentTestMsg = "Get Tx Num...";
    ReCode = GetPanelChannels(rBuffer);
    if (ReCode == ERROR_CODE_OK) {
        test_data.screen_param.channels_num = rBuffer[0];
    } else {
        FTS_TEST_SAVE_INFO("\n Failed to get channel number");
        test_data.screen_param.channels_num = 0;
    }

    ///////////////m_strCurrentTestMsg = "Get Rx Num...";

    ReCode = GetPanelKeys(rBuffer);
    if (ReCode == ERROR_CODE_OK) {
        test_data.screen_param.key_num = rBuffer[0];
    } else {
        test_data.screen_param.key_num = 0;
        FTS_TEST_SAVE_INFO("\n Failed to get Rx number");
    }

    return ReCode;

}

/************************************************************************
* Name: GetRawData
* Brief:  get panel rawdata by read rawdata function
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char GetRawData(void)
{
    //int LineNum=0;
    //int i=0;
    int readlen = 0;
    unsigned char ReCode = ERROR_CODE_OK;

    //--------------------------------------------Enter Factory Mode
    ReCode = enter_factory_mode();
    if ( ERROR_CODE_OK != ReCode ) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter Factory Mode...");
        return ReCode;
    }

    //--------------------------------------------Check Num of Channel
    if (0 == (test_data.screen_param.channels_num + test_data.screen_param.key_num)) {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode ) {
            FTS_TEST_SAVE_INFO("\n Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    FTS_TEST_SAVE_INFO("\n Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode) {
        FTS_TEST_SAVE_INFO("\n Failed to Scan ...");
        return ReCode;
    }

    memset(m_RawData, 0, sizeof(m_RawData));

    //--------------------------------------------Read RawData
    readlen = test_data.screen_param.channels_num + test_data.screen_param.key_num;
    if (readlen <= 0 || readlen >= MAX_SCAP_CHANNEL_NUM) return ERROR_CODE_INVALID_PARAM;


    ReCode = ReadRawData(3, 0, readlen * 2, m_RawData);
    if (ReCode != ERROR_CODE_OK) {
        FTS_TEST_SAVE_INFO("\n Failed to Read RawData...");
        return ReCode;
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
    int channels_num = 0, key_num = 0;

    //----------------------------------------------------------Show RawData
    FTS_TEST_SAVE_INFO("\n \nChannels:  ");
    for (channels_num = 0; channels_num < test_data.screen_param.channels_num; channels_num++) {
        FTS_TEST_SAVE_INFO("%5d    ", m_RawData[channels_num]);
    }

    FTS_TEST_SAVE_INFO("\n \nKeys:  ");
    for (key_num = 0; key_num < test_data.screen_param.key_num; key_num++) {
        FTS_TEST_SAVE_INFO("%5d    ", m_RawData[test_data.screen_param.channels_num + key_num]);
    }

    FTS_TEST_SAVE_INFO("\n\n");
}



/********************************************************************
 * test parameter parse from ini function
 *******************************************************************/
void init_testitem_ft6x36(char *strIniFile)
{
    char str[512] = {0};

    FTS_TEST_FUNC_ENTER();
    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString("TestItem", "FW_VERSION_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.FW_VERSION_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("TestItem", "FACTORY_ID_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.FACTORY_ID_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("TestItem", "PROJECT_CODE_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.PROJECT_CODE_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem", "CHANNEL_NUM_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.CHANNEL_NUM_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_SHORT_TEST
    GetPrivateProfileString("TestItem", "CHANNEL_SHORT_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.CHANNEL_SHORT_TEST = fts_atoi(str);

    /////////////////////////////////// NOISE_TEST
    GetPrivateProfileString("TestItem", "NOISE_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.CB_TEST = fts_atoi(str);

    /////////////////////////////////// DELTA_CB_TEST
    GetPrivateProfileString("TestItem", "DELTA_CB_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.DELTA_CB_TEST = fts_atoi(str);

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNELS_DEVIATION_TEST
    GetPrivateProfileString("TestItem", "CHANNELS_DEVIATION_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.CHANNELS_DEVIATION_TEST = fts_atoi(str);

    /////////////////////////////////// TWO_SIDES_DEVIATION_TEST
    GetPrivateProfileString("TestItem", "TWO_SIDES_DEVIATION_TEST", "1", str, strIniFile);
    g_stCfg_FT6X36_TestItem.TWO_SIDES_DEVIATION_TEST = fts_atoi(str);

    /////////////////////////////////// CB_DEVIATION_TEST
    GetPrivateProfileString("TestItem", "CB_DEVIATION_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.CB_DEVIATION_TEST = fts_atoi(str);

    /////////////////////////////////// DIFFER_TEST
    GetPrivateProfileString("TestItem", "DIFFER_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.DIFFER_TEST = fts_atoi(str);

    /////////////////////////////////// WEAK_SHORT_TEST
    GetPrivateProfileString("TestItem", "WEAK_SHORT_TEST", "0", str, strIniFile);
    g_stCfg_FT6X36_TestItem.WEAK_SHORT_TEST = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_basicthreshold_ft6x36(char *strIniFile)
{
    char str[512] = {0};

    FTS_TEST_FUNC_ENTER();
    //////////////////////////////////////////////////////////// FW Version

    GetPrivateProfileString( "Basic_Threshold", "FW_VER_VALUE", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.FW_VER_VALUE = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("Basic_Threshold", "Factory_ID_Number", "255", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.Factory_ID_Number = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("Basic_Threshold", "Project_Code", " ", str, strIniFile);
    sprintf(g_stCfg_FT6X36_BasicThreshold.Project_Code, "%s", str);

    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "13000", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.RawDataTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "17000", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.RawDataTest_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_ChannelNum", "22", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelNumTest_ChannelNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelNumTest_KeyNum", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelNumTest_KeyNum = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelShortTest_K1", "255", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelShortTest_K1 = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelShortTest_K2", "255", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelShortTest_K2 = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ChannelShortTest_CB", "255", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelShortTest_CB = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Max", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Frames", "32", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_Frames = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_Time", "1", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_Time = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "NoiseTest_SampeMode", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_SampeMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_NoiseMode", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "NoiseTest_ShowTip", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.NoiseTest_ShowTip = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "CbTest_Min", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.CbTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CbTest_Max", "250", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.CbTest_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Base", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Base = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Differ_Max", "50", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Differ_Max = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Include_Key_Test", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Include_Key_Test = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Key_Differ_Max", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Key_Differ_Max = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S1", "15", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S2", "15", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S3", "12", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S4", "12", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S5", "12", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Deviation_S6", "12", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Deviation_S6 = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Set_Critical", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Set_Critical = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S1", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S2", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S3", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S4", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S5", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "DeltaCbTest_Critical_S6", "20", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.DeltaCbTest_Critical_S6 = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S1", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S2", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S3", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S4", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S5", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Deviation_S6", "8", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Deviation_S6 = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Set_Critical", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Set_Critical = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S1", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S2", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S3", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S4", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S5", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "ChannelsDeviationTest_Critical_S6", "13", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.ChannelsDeviationTest_Critical_S6 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S1", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S2", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S3", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S4", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S5", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Deviation_S6", "5", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Deviation_S6 = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Set_Critical", "0", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Set_Critical = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S1", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S1 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S2", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S2 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S3", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S3 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S4", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S4 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S5", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S5 = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold", "TwoSidesDeviationTest_Critical_S6", "10", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.TwoSidesDeviationTest_Critical_S6 = fts_atoi(str);

    //CB_DEVIATION_TEST
    GetPrivateProfileString("Basic_Threshold", "CBDeviationTest_Hole", "50", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.CBDeviationTest_Hole = fts_atoi(str);

    //DIFFER_TEST
    GetPrivateProfileString("Basic_Threshold", "DifferTest_Ave_Hole", "500", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.Differ_Ave_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "DifferTest_Max_Hole", "500", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.Differ_Max_Hole = fts_atoi(str);

    //WEAK_SHORT_TEST
    GetPrivateProfileString("Basic_Threshold", "Weak_Short_Hole", "500", str, strIniFile);
    g_stCfg_FT6X36_BasicThreshold.WeakShortThreshold = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void OnGetTestItemParam(char *strItemName, char *strIniFile, int iDefautValue)
{
    char strValue[800];
    char str_tmp[128];
    int iValue = 0;
    int dividerPos = 0;
    int index = 0;
    int i = 0, j = 0, k = 0;
    memset(test_data.scap_detail_thr.TempData, 0, sizeof(test_data.scap_detail_thr.TempData));
    sprintf(str_tmp, "%d", iDefautValue);
    GetPrivateProfileString( "Basic_Threshold", strItemName, str_tmp, strValue, strIniFile);
    iValue = fts_atoi(strValue);
    for (i = 0; i < MAX_CHANNEL_NUM; i++) {
        test_data.scap_detail_thr.TempData[i] = iValue;
    }

    dividerPos = GetPrivateProfileString( "SpecialSet", strItemName, "", strValue, strIniFile);
    if (dividerPos > 0) {
        index = 0;
        k = 0;
        memset(str_tmp, 0x00, sizeof(str_tmp));
        for (j = 0; j < dividerPos; j++) {
            if (',' == strValue[j]) {
                test_data.scap_detail_thr.TempData[k] = (short)(fts_atoi(str_tmp));
                index = 0;
                memset(str_tmp, 0x00, sizeof(str_tmp));
                k++;
            } else {
                if (' ' == strValue[j])
                    continue;
                str_tmp[index] = strValue[j];
                index++;
            }
        }
    }
}

void init_detailthreshold_ft6x36(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnGetTestItemParam("RawDataTest_Max", ini, 12500);
    memcpy(test_data.scap_detail_thr.RawDataTest_Max, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("RawDataTest_Min", ini, 16500);
    memcpy(test_data.scap_detail_thr.RawDataTest_Min, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("CiTest_Max", ini, 5);
    memcpy(test_data.scap_detail_thr.CiTest_Max, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("CiTest_Min", ini, 250);
    memcpy(test_data.scap_detail_thr.CiTest_Min, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCiTest_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.DeltaCiTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCiTest_AnotherBase1", ini, 0);
    memcpy(test_data.scap_detail_thr.DeltaCiTest_AnotherBase1, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCiTest_AnotherBase2", ini, 0);
    memcpy(test_data.scap_detail_thr.DeltaCiTest_AnotherBase2, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("NoiseTest_Max", ini, 20);
    memcpy(test_data.scap_detail_thr.NoiseTest_Max, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("CiDeviation_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.CiDeviationTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCxTest_Sort", ini, 1);
    memcpy(test_data.scap_detail_thr.DeltaCxTest_Sort, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCxTest_Area", ini, 1);
    memcpy(test_data.scap_detail_thr.DeltaCxTest_Area, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    //6x36
    OnGetTestItemParam("CbTest_Max", ini, 0);
    memcpy(test_data.scap_detail_thr.CbTest_Max, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("CbTest_Min", ini, 0);
    memcpy(test_data.scap_detail_thr.CbTest_Min, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DeltaCbTest_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.DeltaCbTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("DifferTest_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.DifferTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("CBDeviation_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.CBDeviationTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    OnGetTestItemParam("K1DifferTest_Base", ini, 0);
    memcpy(test_data.scap_detail_thr.K1DifferTest_Base, test_data.scap_detail_thr.TempData, MAX_CHANNEL_NUM * sizeof(int));

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft6x36(void)
{

    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();
    //////////////////////////////////////////////////FACTORY_ID_TEST
    if ( g_stCfg_FT6X36_TestItem.FACTORY_ID_TEST == 1) {

        fts_set_testitem(Code_FT6X36_FACTORY_ID_TEST);
    }

    //////////////////////////////////////////////////Project Code Test
    if ( g_stCfg_FT6X36_TestItem.PROJECT_CODE_TEST == 1) {

        fts_set_testitem(Code_FT6X36_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////FW Version Test
    if ( g_stCfg_FT6X36_TestItem.FW_VERSION_TEST == 1) {

        fts_set_testitem(Code_FT6X36_FW_VERSION_TEST);
    }

    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(Code_FT6X36_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CHANNEL_NUM_TEST
    if ( g_stCfg_FT6X36_TestItem.CHANNEL_NUM_TEST == 1) {

        fts_set_testitem(Code_FT6X36_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////Differ Test
    if ( g_stCfg_FT6X36_TestItem.DIFFER_TEST == 1) {

        fts_set_testitem(Code_FT6X36_DIFFER_TEST);
    }

    //////////////////////////////////////////////////CB Deviation Test
    if ( g_stCfg_FT6X36_TestItem.CB_DEVIATION_TEST == 1) {

        fts_set_testitem(Code_FT6X36_CB_DEVIATION_TEST);
    }

    //////////////////////////////////////////////////CB_TEST
    if ( g_stCfg_FT6X36_TestItem.CB_TEST == 1) {

        fts_set_testitem(Code_FT6X36_CB_TEST);
    }

    //////////////////////////////////////////////////DELTA_CB_TEST
    if ( g_stCfg_FT6X36_TestItem.DELTA_CB_TEST == 1) {

        fts_set_testitem(Code_FT6X36_DELTA_CB_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( g_stCfg_FT6X36_TestItem.RAWDATA_TEST == 1) {

        fts_set_testitem(Code_FT6X36_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////CHANNELS_DEVIATION_TEST
    if ( g_stCfg_FT6X36_TestItem.CHANNELS_DEVIATION_TEST == 1) {

        fts_set_testitem(Code_FT6X36_CHANNELS_DEVIATION_TEST);
    }

    //////////////////////////////////////////////////TWO_SIDES_DEVIATION_TEST
    if ( g_stCfg_FT6X36_TestItem.TWO_SIDES_DEVIATION_TEST == 1) {

        fts_set_testitem(Code_FT6X36_TWO_SIDES_DEVIATION_TEST);
    }

    //////////////////////////////////////////////////CHANNEL_SHORT_TEST
    if ( g_stCfg_FT6X36_TestItem.CHANNEL_SHORT_TEST == 1) {

        fts_set_testitem(Code_FT6X36_CHANNEL_SHORT_TEST);
    }

    //////////////////////////////////////////////////NOISE_TEST
    if ( g_stCfg_FT6X36_TestItem.NOISE_TEST == 1) {

        fts_set_testitem(Code_FT6X36_NOISE_TEST);
    }

    //////////////////////////////////////////////////Weak Short Test
    if ( g_stCfg_FT6X36_TestItem.WEAK_SHORT_TEST == 1) {

        fts_set_testitem(Code_FT6X36_WEAK_SHORT_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft6x36,
    .init_basicthreshold = init_basicthreshold_ft6x36,
    .init_detailthreshold = init_detailthreshold_ft6x36,
    .set_testitem_sequence  = set_testitem_sequence_ft6x36,
    .start_test = start_test_ft6x36,
};
#endif
