/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: focaltech_test_ft8716.c
*
* Author: Software Development
*
* Created: 2016-08-01
*
* Abstract: test item for FT8736
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include "../focaltech_test.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#if (FTS_CHIP_TEST_TYPE ==FT8736_TEST)

/////////////////////////////////////////////////Reg FT8736
#define DEVIDE_MODE_ADDR    0x00
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define FT8736_LEFT_KEY_REG    0X1E
#define FT8736_RIGHT_KEY_REG   0X1F
#define REG_CbAddrH         0x18
#define REG_CbAddrL         0x19
#define REG_OrderAddrH      0x1A
#define REG_OrderAddrL      0x1B
#define REG_RawBuf0         0x6A
#define REG_RawBuf1         0x6B
#define REG_OrderBuf0       0x6C
#define REG_CbBuf0          0x6E
#define REG_K1Delay         0x31
#define REG_K2Delay         0x32
#define REG_SCChannelCf     0x34
#define REG_8736_LCD_NOISE_FRAME        0x12
#define REG_8736_LCD_NOISE_START        0x11
#define REG_8736_LCD_NOISE_NUMBER       0x13
#define REG_8736_LCD_NOISE_DATA_READY   0x00
#define REG_CLB                        0x04

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
struct ft8736_test_item {
    bool rawdata_test;
    bool cb_test;
    bool short_test;
    bool lcd_noise_test;
    bool open_test;
};
struct ft8736_basic_threshold {
    char project_code[32];
    bool rawdata_test_va_check;
    int rawdata_test_min;
    int rawdata_test_max;
    bool rawdata_test_vk_check;
    int rawdata_test_min_vk;
    int rawdata_test_max_vk;

    bool cb_test_va_check;
    int cb_test_min;
    int cb_test_max;
    bool cb_test_vk_check;
    int cb_test_min_vk;
    int cb_test_max_vk;

    int short_res_min;
    //int ShortTest_K2Value;
    int lcd_noise_test_frame_num;
    int lcd_noise_test_coefficient;
    int lcd_noise_test_coefficient_key;
    u8 lcd_noise_test_noise_mode;
    int open_test_cb_min;
};

enum test_item_ft8736 {
    CODE_FT8736_ENTER_FACTORY_MODE = 0,//All IC are required to test items
    CODE_FT8736_RAWDATA_TEST = 7,
    CODE_FT8736_CB_TEST = 12,
    CODE_FT8736_SHORT_CIRCUIT_TEST = 14,
    CODE_FT8736_OPEN_TEST = 15,
    CODE_FT8736_LCD_NOISE_TEST = 19,
};

/*******************************************************************************
* Static variables
*******************************************************************************/
static int raw_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static int cb_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static u8 tmp_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0}; //One-dimensional
static int tmp_rawdata[TX_NUM_MAX *RX_NUM_MAX] = {0};
static int adc_data[TX_NUM_MAX *RX_NUM_MAX] =  {0};
static int short_res[TX_NUM_MAX][RX_NUM_MAX] = { {0} };
static u8 read_buffer[80 * 80 * 2] = {0};
static int lcd_noise[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static u8 screen_noise_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
struct ft8736_test_item ft8736_item;
struct ft8736_basic_threshold ft8736_basic_thr;
/*******************************************************************************
* Static function prototypes
*******************************************************************************/
static u32 sqrt_new(u32 n);
static int start_scan(void);
static int read_raw_data(u8 freq, u8 line_num, int byte_num, int *rev_buffer);
static int get_tx_rx_cb(unsigned short start_node, unsigned short read_num, u8 *read_buffer);
static int get_raw_data(void);
static int read_bytes_by_key( u8 key, int byte_num, u8 *byte_data );
static int ft8736_get_key_num(void);
static void ft8736_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static bool ft8736_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static bool ft8736_compare_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min, int data_max, int vk_data_min, int vk_data_max, bool include_key);
static int weakshort_get_adc_data( int all_adcdata_len, int *rev_buffer  );
static int chip_clb(u8 *clb_result);
static bool wait_state_update(void);
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, u8 row, u8 col, u8 item_count);
static int ft8736_enter_factory_mode(void);
static int ft8736_rawdata_test(bool *test_result);
static int ft8736_cb_test(bool *test_result);
static int ft8736_lcdnoise_test(bool *test_result);
static int ft8736_open_test(bool *test_result);
static int ft8736_short_test(bool *test_result);  //This test item requires LCD driver coordination

/************************************************************************
* Name: start_test_ft8736
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
bool start_test_ft8736(void)
{
    bool test_result = true, tmp_result = 1;
    int ret;
    int item_count = 0;

    FTS_TEST_FUNC_ENTER();

    //--------------1. Init part
    if (init_test() < 0) {
        FTS_TEST_SAVE_INFO("\n[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == test_data.test_num)
        test_result = false;

    ////////Testing process, the order of the test_data.test_item structure of the test items
    for (item_count = 0; item_count < test_data.test_num; item_count++) {
        test_data.test_item_code = test_data.test_item[item_count].itemcode;

        ///////////////////////////////////////////////////////FT8736_ENTER_FACTORY_MODE
        if (CODE_FT8736_ENTER_FACTORY_MODE == test_data.test_item[item_count].itemcode) {
            ret = ft8736_enter_factory_mode();
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_RAWDATA_TEST
        if (CODE_FT8736_RAWDATA_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8736_rawdata_test(&tmp_result);
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_CB_TEST
        if (CODE_FT8736_CB_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8736_cb_test(&tmp_result); //
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_SHORT_TEST
        if (CODE_FT8736_SHORT_CIRCUIT_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8736_short_test(&tmp_result);
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_Open_TEST
        if (CODE_FT8736_OPEN_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8736_open_test(&tmp_result);
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_LCDNoise_TEST
        if (CODE_FT8736_LCD_NOISE_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8736_lcdnoise_test(&tmp_result);
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

    }

    //--------------3. End Part
    finish_test();

    //--------------4. return result
    return test_result;

}

/************************************************************************
* Name: ft8736_enter_factory_mode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_enter_factory_mode(void)
{
    int ret = 0;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("enter factory mode fail, can't get tx/rx num");
        return ret;
    }
    ret = ft8736_get_key_num();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("get key num fail");
        return ret;
    }

    return ret;
}

/************************************************************************
* Name: ft8736_rawdata_test
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_rawdata_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    int i = 0;
    bool include_key = ft8736_basic_thr.rawdata_test_vk_check;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: -------- Raw Data Test\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        return ret;
    }
    //----------------------------------------------------------Read RawData
    for (i = 0 ; i < 3; i++) //Lost 3 Frames, In order to obtain stable data
        ret = get_raw_data();
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to get Raw Data!! Error Code: %d\n", ret);
        return ret;
    }
    //----------------------------------------------------------Show RawData
    ft8736_show_data(raw_data, include_key);

    //----------------------------------------------------------To Determine RawData if in Range or not
    tmp_result = ft8736_compare_detailthreshold_data(raw_data, test_data.incell_detail_thr.RawDataTest_Min, test_data.incell_detail_thr.RawDataTest_Max, include_key);
    //////////////////////////////Save Test Data
    save_test_data(raw_data, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);


    //----------------------------------------------------------Return Result
    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========RawData Test is OK!\n");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========RawData Test is NG!\n");
    }
    return ret;
}

/************************************************************************
* Name: ft8736_cb_test
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_cb_test(bool *test_result)
{
    bool tmp_result = true;
    int ret = ERROR_CODE_OK;
    int row = 0;
    int col = 0;
    int i = 0;
    bool include_key = false;

    u8 clb_result = 0;
    u8 uc_bits = 0;
    int read_key_len = test_data.screen_param.key_num_total;

    include_key = ft8736_basic_thr.cb_test_vk_check;

    FTS_TEST_SAVE_INFO("\n\n\n==============================Test Item: --------  CB Test\n\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
    }

    for ( i = 0; i < 10; i++) {
        FTS_TEST_SAVE_INFO("\n start chipclb times:%d. ", i);
        //auto clb
        ret = chip_clb( &clb_result );
        sys_delay(50);
        if ( 0 != clb_result) {
            break;
        }
    }

    if ( false == clb_result) {
        FTS_TEST_SAVE_INFO("\n ReCalib Failed.");
        tmp_result = false;
    }

    ret = read_reg(0x0B, &uc_bits);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n Read Reg 0x0B Failed!");
    }

    read_key_len = test_data.screen_param.key_num_total;
    if (uc_bits != 0) {
        read_key_len = test_data.screen_param.key_num_total * 2;
    }

    ret = get_tx_rx_cb( 0, (short)(test_data.screen_param.tx_num * test_data.screen_param.rx_num  + read_key_len), tmp_data );
    if ( ERROR_CODE_OK != ret ) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\nFailed to get CB value...\n");
        goto TEST_ERR;
    }

    memset(cb_data, 0, sizeof(cb_data));
    ///VA area
    for ( row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            cb_data[row][col] = tmp_data[ row * test_data.screen_param.rx_num + col ];
        }
    }

    for ( col = 0; col < read_key_len/*test_data.screen_param.key_num_total*/; ++col ) {
        if (uc_bits != 0) {
            cb_data[test_data.screen_param.tx_num][col / 2] = (short)((tmp_data[ test_data.screen_param.tx_num * test_data.screen_param.rx_num + col ] & 0x01 ) << 8) + tmp_data[ test_data.screen_param.tx_num *
                    test_data.screen_param.rx_num + col + 1 ];
            col++;
        } else {
            cb_data[test_data.screen_param.tx_num][col] = tmp_data[ test_data.screen_param.tx_num * test_data.screen_param.rx_num + col ];
        }
    }

    //------------------------------------------------Show CbData
    ft8736_show_data(cb_data, include_key);

    //----------------------------------------------------------To Determine Cb if in Range or not
    tmp_result = ft8736_compare_detailthreshold_data(cb_data, test_data.incell_detail_thr.CBTest_Min, test_data.incell_detail_thr.CBTest_Max, include_key);

    //////////////////////////////Save Test Data
    save_test_data(cb_data, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);
    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========CB Test is OK!\n");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========CB Test is NG!\n");
    }

    return ret;

TEST_ERR:

    * test_result = false;
    FTS_TEST_SAVE_INFO("\n==========CB Test is NG!\n");
    return ret;
}

/************************************************************************
* Name: ft8736_short_test
* Brief:  Get short circuit test mode data, judge whether there is a short circuit
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_short_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    int res_min = 0;
    u8 tx_num = 0, rx_num = 0, channel_num = 0;
    int all_adc_data_num = 0;
    int row = 0;
    int col = 0;
    int tmp_adc = 0;
    int value_min = 0;
    int value_max = 0;

    FTS_TEST_SAVE_INFO("\n==============================Test Item: -------- Short Circuit Test \r\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        goto TEST_END;
    }

    /*****************************************************
    Befor ShortCircuitTest need LCD driver to control power off
    ******************************************************/
    ret = read_reg(0x02, &tx_num);
    ret = read_reg(0x03, &rx_num);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n// Failed to read reg. Error Code: %d", ret);
        goto TEST_END;
    }

    FTS_TEST_SAVE_INFO("\n tx_num:%d.  rx_num:%d.", tx_num, rx_num);
    channel_num = tx_num + rx_num;
    all_adc_data_num = tx_num * rx_num + test_data.screen_param.key_num_total;

    memset(adc_data, 0, sizeof(adc_data));
    memset(short_res, 0, sizeof(short_res));

    ret = weakshort_get_adc_data(all_adc_data_num * 2, adc_data);
    sys_delay(50);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n // Failed to get AdcData. Error Code: %d", ret);
        goto TEST_END;
    }

    //show ADCData
#if 0
    FTS_TEST_SAVE_INFO("\nADCData:\n");
    for (i = 0; i < all_adc_data_num; i++) {
        FTS_TEST_SAVE_INFO("%-4d  ", adc_data[i]);
        if (0 == (i + 1) % rx_num) {
            FTS_TEST_SAVE_INFO("\n\n");
        }
    }
    FTS_TEST_SAVE_INFO("\n\n");
#endif

    //  FTS_TEST_DBG("shortRes data:\n");
    for ( row = 0; row < test_data.screen_param.tx_num + 1; ++row ) {
        for ( col = 0; col <  test_data.screen_param.rx_num; ++col ) {
            tmp_adc = adc_data[row * rx_num + col];
            if (2007 <= tmp_adc)  tmp_adc = 2007;
            short_res[row][col] = (tmp_adc * 200) / (2047 - tmp_adc);
        }
    }

    //////////////////////// analyze
    res_min = ft8736_basic_thr.short_res_min;
    value_min = res_min;
    value_max = 100000000;
    FTS_TEST_SAVE_INFO("\n Short Circuit test , Set_Range=(%d, %d). \n", \
                       value_min, value_max);

    tmp_result = ft8736_compare_data(short_res, value_min, value_max, value_min, value_max, true);

    /*****************************************************
    After ShortCircuitTest need LCD driver to control power on
    ******************************************************/

TEST_END:
    if (tmp_result) {
        FTS_TEST_SAVE_INFO("\n==========Short Circuit Test is OK!");
        * test_result = true;
    } else {
        FTS_TEST_SAVE_INFO("\n==========Short Circuit Test is NG!");
        * test_result = false;
    }

    return ret;
}

/************************************************************************
* Name: ft8736_open_test
* Brief:  Check if channel is open
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_open_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    u8 cb_value = 0xff;
    u8 clb_result = 0;
    int min = 0;
    int max = 0;
    int row = 0;
    int col = 0;

    FTS_TEST_SAVE_INFO("\n\r\n\r\n==============================Test Item: --------  Open Test");

    ret = enter_factory_mode();
    sys_delay(50);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n\r\n =========  Enter Factory Failed!");
        goto TEST_ERR;
    }

    // set GIP to VGHO2/VGLO2 in factory mode (0x22 register write 0x80)
    ret = read_reg(0x22, &cb_value);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n// =========  Read Reg Failed!");
        goto TEST_ERR;
    }

    ret = write_reg(0x22, 0x80);
    sys_delay(50);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n// =========  Write Reg Failed!");
        goto TEST_ERR;
    }

    if (!wait_state_update()) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n//=========Wait State Update Failed!");
        goto TEST_ERR;
    }

    //auto clb
    ret = chip_clb(&clb_result);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto TEST_ERR;
    }

    ret = get_tx_rx_cb( 0, (short)(test_data.screen_param.tx_num * test_data.screen_param.rx_num + test_data.screen_param.key_num_total), tmp_data );
    if ( ERROR_CODE_OK != ret ) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n\r\n//=========get CB Failed!!");
        goto TEST_ERR;
    }

    memset(cb_data, 0, sizeof(cb_data));
    for (  row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            cb_data[row][col] = tmp_data[ row * test_data.screen_param.rx_num + col ];
        }
    }

    // show open data
    ft8736_show_data(cb_data, false);

    //  analyze
    min = ft8736_basic_thr.open_test_cb_min;
    max = 200;
    tmp_result = ft8736_compare_data(cb_data, min, max, min, max, false);

    // save data
    save_test_data(cb_data, 0, test_data.screen_param.tx_num, test_data.screen_param.rx_num, 1);

    // restore the register value of 0x22
    ret = write_reg(0x22, cb_value);
    sys_delay(50);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n//=========  Write Reg Failed!");
        goto TEST_ERR;
    }

    ret = chip_clb( &clb_result );
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto TEST_ERR;
    }

    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========Open Test is OK!\n");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========Open Test is NG!\n");
    }
    return ret;

TEST_ERR:
    * test_result = false;
    FTS_TEST_SAVE_INFO("\n==========Open Test is NG!\n");


    return ret;
}

/************************************************************************
* Name: ft8736_lcdnoise_test
* Brief:   obtain is differ mode  the data and calculate the corresponding type of noise value.
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8736_lcdnoise_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool result_flag = true;
    int frame_num = 0;
    int i = 0;
    int row = 0;
    int col = 0;
    int value_min = 0;
    int value_max = 0;
    int value_max_vk = 0;
    int bytes_num  = 0;
    int map_index = 0;
    u8 reg_data = 0, old_mode = 0, ch_new_mode = 0, data_ready = 0;
    u8 ch_noise_value_va = 0xff, ch_noise_value_key = 0xff;

    FTS_TEST_SAVE_INFO("\n==============================Test Item: -------- LCD Noise Test \r\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        goto TEST_ERR;
    }

    ret =  read_reg( 0x06, &old_mode);
    ret =  write_reg( 0x06, 0x01 );
    sys_delay(10);

    ret = read_reg( 0x06, &ch_new_mode );
    if ( ret != ERROR_CODE_OK || ch_new_mode != 1 ) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\r\nSwitch Mode Failed!\r\n");
        goto TEST_ERR;
    }

    frame_num = ft8736_basic_thr.lcd_noise_test_frame_num / 4;
    ret = write_reg( REG_8736_LCD_NOISE_FRAME, frame_num );
    if (ret != ERROR_CODE_OK) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\r\n//=========  Write Reg Failed!");
        goto TEST_ERR;
    }

    ret = write_reg( REG_8736_LCD_NOISE_START, 0x01 );
    for ( i = 0; i < 100; i++) {
        ret = read_reg( REG_8736_LCD_NOISE_DATA_READY, &data_ready );
        sys_delay(200);

        if ( 0x00 == (data_ready >> 7) ) {
            break;
        } else {
            sys_delay( 100 );
        }

        if ( 99 == i ) {
            ret = write_reg( REG_8736_LCD_NOISE_START, 0x00 );
            if (ret != ERROR_CODE_OK) {
                result_flag = false;
                FTS_TEST_SAVE_INFO("\r\nRestore Failed!");
                goto TEST_ERR;
            }

            result_flag = false;
            FTS_TEST_SAVE_INFO("\r\nTime Over!");
            goto TEST_ERR;
        }
    }

    bytes_num = 2 * ( test_data.screen_param.tx_num *  test_data.screen_param.rx_num +  test_data.screen_param.key_num_total);

    ret = write_reg(0x01/*REG_LINE_NUM*/, 0xAD);
    ret = read_bytes_by_key( 0x6a, bytes_num, screen_noise_data );

    for ( row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for (  col = 0; col < test_data.screen_param.rx_num; ++col ) {
            map_index = row * test_data.screen_param.rx_num + col;
            raw_data[row][col] = (short)(screen_noise_data[(map_index << 1)] << 8) + screen_noise_data[(map_index << 1) + 1];
        }
    }
    for (  col = 0; col < test_data.screen_param.key_num_total; ++col ) {
        map_index = test_data.screen_param.tx_num * test_data.screen_param.rx_num + col;
        raw_data[test_data.screen_param.tx_num][col] = (short)(screen_noise_data[(map_index << 1)] << 8) + screen_noise_data[(map_index << 1) + 1];
    }
    ret = write_reg( REG_8736_LCD_NOISE_START, 0x00 );
    if (ret != ERROR_CODE_OK) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\r\nRestore Failed!");
        goto TEST_ERR;
    }

    ret = read_reg( REG_8736_LCD_NOISE_NUMBER, &reg_data );
    if ( reg_data <= 0 ) {
        reg_data = 1;
    }

    ret = write_reg( 0x06, old_mode );
    if (ret != ERROR_CODE_OK) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\r\nWrite Reg Failed!");
        goto TEST_ERR;
    }

    if (0 == ft8736_basic_thr.lcd_noise_test_noise_mode) {
        for (  row = 0; row <= test_data.screen_param.tx_num; ++row ) {
            for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
                lcd_noise[row][col] = raw_data[row][col];
            }
        }
    }

    if (1 == ft8736_basic_thr.lcd_noise_test_noise_mode) {
        for ( row = 0; row <= test_data.screen_param.tx_num; ++row ) {
            for (  col = 0; col < test_data.screen_param.rx_num; ++col ) {
                lcd_noise[row][col] = sqrt_new( raw_data[row][col] / reg_data);
            }
        }
    }

    ret = enter_work_mode();
    sys_delay(100);
    ret = read_reg( 0x80, &ch_noise_value_va );
    ret = read_reg( 0x82, &ch_noise_value_key );
    ret = enter_factory_mode();
    sys_delay(500);
    if (ret != ERROR_CODE_OK) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\r\n enter factory mode failed.");
    }

#if 1
    //show data of lcd_noise
    ft8736_show_data(lcd_noise, false);
#endif

    //////////////////////// analyze
    value_min = 0;
    value_max = ft8736_basic_thr.lcd_noise_test_coefficient * ch_noise_value_va * 32 / 100;
    value_max_vk = ft8736_basic_thr.lcd_noise_test_coefficient_key * ch_noise_value_key * 32 / 100;
    result_flag = ft8736_compare_data(lcd_noise, value_min, value_max, value_min, value_max_vk, true);

    //  Save Test Data
    save_test_data(lcd_noise, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1 );

    if (result_flag) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========LCD Noise Test is OK!");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========LCD Noise Test is NG!");
    }
    return ret;

TEST_ERR:
    * test_result = false;
    FTS_TEST_SAVE_INFO(" LCD Noise Test is NG. ");

    return ret;
}

/************************************************************************
* Name: sqrt_new
* Brief:  calculate sqrt of input.
* Input: unsigned int n
* Output: none
* Return: sqrt of n.
***********************************************************************/
static u32 sqrt_new(u32 n)
{
    unsigned int  val = 0, last = 0;
    unsigned char i = 0;;

    if (n < 6) {
        if (n < 2) {
            return n;
        }
        return n / 2;
    }
    val = n;
    i = 0;
    while (val > 1) {
        val >>= 1;
        i++;
    }
    val <<= (i >> 1);
    val = (val + val + val) >> 1;
    do {
        last = val;
        val = ((val + n / val) >> 1);
    } while (focal_abs(val - last) > 1);
    return val;
}

static bool wait_state_update(void)
{
    int wait_timeout = 500;
    u8 state_value = 0xff;
    while (wait_timeout--) {
        //Wait register status update.
        state_value = 0xff;
        read_reg(0x0e, &state_value);
        if (0x00 != state_value)
            sys_delay(5);
        else
            break;
    }

    if (0 >= wait_timeout) {
        FTS_TEST_SAVE_INFO("\r\n//=========Wait State Update Failed!");
        return false;
    }
    return true;
}

static int chip_clb(u8 *clb_result)
{
    u8 reg_data = 0;
    u8 timeout_times = 50;        //5s
    int ret = ERROR_CODE_OK;

    ret = write_reg(REG_CLB, 4);  //start auto clb

    if (ret == ERROR_CODE_OK) {
        while (timeout_times--) {
            sys_delay(100);  //delay 500ms
            ret = write_reg(DEVIDE_MODE_ADDR, 0x04 << 4);
            ret = read_reg(0x04, &reg_data);
            if (ret == ERROR_CODE_OK) {
                if (reg_data == 0x02) {
                    *clb_result = 1;
                    break;
                }
            } else {
                break;
            }
        }

        if (timeout_times == 0) {
            *clb_result = 0;
        }
    }
    return ret;
}

/************************************************************************
* Name: save_test_data
* Brief:  Storage format of test data
* Input: int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, u8 row, u8 col, u8 item_count
* Output: none
* Return: none
***********************************************************************/
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, u8 row, u8 col, u8 item_count)
{
    int len = 0;
    int i = 0, j = 0;

    //Save  Msg (itemcode is enough, ItemName is not necessary, so set it to "NA".)
    len = sprintf(test_data.tmp_buffer, "NA, %d, %d, %d, %d, %d, ", \
                  test_data.test_item_code, row, col, test_data.start_line, item_count);
    memcpy(test_data.msg_area_line2 + test_data.len_msg_area_line2, test_data.tmp_buffer, len);
    test_data.len_msg_area_line2 += len;
    test_data.start_line += row;
    test_data.test_data_count++;

    //Save Data
    for (i = 0 + array_index; (i < row + array_index) && (i < TX_NUM_MAX); i++) {
        for (j = 0; (j < col) && (j < RX_NUM_MAX); j++) {
            if (j == (col - 1)) //The Last Data of the row, add "\n"
                len = sprintf(test_data.tmp_buffer, "%d, \n",  data[i][j]);
            else
                len = sprintf(test_data.tmp_buffer, "%d, ", data[i][j]);

            memcpy(test_data.store_data_area + test_data.len_store_data_area, test_data.tmp_buffer, len);
            test_data.len_store_data_area += len;
        }
    }
}

////////////////////////////////////////////////////////////
/************************************************************************
* Name: start_scan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int start_scan(void)
{
    u8 reg_val = 0x00;
    u8 times = 0;
    const u8 max_times = 20;  //The longest wait 160ms
    int ret = ERROR_CODE_COMM_ERROR;

    //if(hDevice == NULL)       return ERROR_CODE_NO_DEVICE;

    ret = read_reg(DEVIDE_MODE_ADDR, &reg_val);
    if (ret == ERROR_CODE_OK) {
        reg_val |= 0x80;     //Top bit position 1, start scan
        ret = write_reg(DEVIDE_MODE_ADDR, reg_val);
        if (ret == ERROR_CODE_OK) {
            while (times++ < max_times) {    //Wait for the scan to complete
                sys_delay(8);    //8ms
                ret = read_reg(DEVIDE_MODE_ADDR, &reg_val);
                if (ret == ERROR_CODE_OK) {
                    if ((reg_val >> 7) == 0)    break;
                } else {
                    break;
                }
            }
            if (times < max_times)    ret = ERROR_CODE_OK;
            else ret = ERROR_CODE_COMM_ERROR;
        }
    }
    return ret;

}
/************************************************************************
* Name: read_raw_data(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: freq(No longer used, reserved), line_num, byte_num
* Output: rev_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int read_raw_data(u8 freq, u8 line_num, int byte_num, int *rev_buffer)
{
    int ret = ERROR_CODE_COMM_ERROR;
    u8 i2c_w_buffer[3] = {0};
    u8 read_data[byte_num];
    int i, read_num;
    unsigned short bytes_num_in_test_mode1 = 0;

    read_num = byte_num / BYTES_PER_TIME;

    if (0 != (byte_num % BYTES_PER_TIME)) read_num++;

    if (byte_num <= BYTES_PER_TIME) {
        bytes_num_in_test_mode1 = byte_num;
    } else {
        bytes_num_in_test_mode1 = BYTES_PER_TIME;
    }

    ret = write_reg(REG_LINE_NUM, line_num);//Set row addr;

    //***********************************************************Read raw data in test mode1
    i2c_w_buffer[0] = REG_RawBuf0;   //set begin address
    if (ret == ERROR_CODE_OK) {
        sys_delay(10);
        ret = fts_i2c_read_write(i2c_w_buffer, 1, read_data, bytes_num_in_test_mode1);
    }

    for (i = 1; i < read_num; i++) {
        if (ret != ERROR_CODE_OK) break;

        if (i == read_num - 1) { //last packet
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, byte_num - BYTES_PER_TIME * i);
        } else {
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, BYTES_PER_TIME);
        }
    }

    if (ret == ERROR_CODE_OK) {
        for (i = 0; i < (byte_num >> 1); i++) {
            rev_buffer[i] = (read_data[i << 1] << 8) + read_data[(i << 1) + 1];
        }
    }

    return ret;
}
/************************************************************************
* Name: get_tx_rx_cb(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx/Rx
* Input: start_node, read_num
* Output: read_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int get_tx_rx_cb(unsigned short start_node, unsigned short read_num, u8 *read_buffer)
{
    int ret = ERROR_CODE_OK;
    unsigned short return_num = 0;//Number to return in every time
    unsigned short total_return_num = 0;//Total return number
    u8 w_buffer[4];
    int i, bytes_num;

    bytes_num = read_num / BYTES_PER_TIME;

    if (0 != (read_num % BYTES_PER_TIME)) bytes_num++;

    w_buffer[0] = REG_CbBuf0;

    total_return_num = 0;

    for (i = 1; i <= bytes_num; i++) {
        if (i * BYTES_PER_TIME > read_num)
            return_num = read_num - (i - 1) * BYTES_PER_TIME;
        else
            return_num = BYTES_PER_TIME;

        w_buffer[1] = (start_node + total_return_num) >> 8; //Address offset high 8 bit
        w_buffer[2] = (start_node + total_return_num) & 0xff; //Address offset low 8 bit

        ret = write_reg(REG_CbAddrH, w_buffer[1]);
        ret = write_reg(REG_CbAddrL, w_buffer[2]);
        ret = fts_i2c_read_write(w_buffer, 1, read_buffer + total_return_num, return_num);

        total_return_num += return_num;

        if (ret != ERROR_CODE_OK)return ret;

        //if(ret < 0) return ret;
    }
    return ret;
}

/************************************************************************
* Name: get_raw_data
* Brief:  Get Raw Data of VA area and Key area
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int get_raw_data(void)
{
    int ret = ERROR_CODE_OK;
    int row, col;

    //--------------------------------------------Start Scanning
    ret = start_scan();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("Failed to Scan \n");
        return ret;
    }

    //--------------------------------------------Read RawData for Channel Area
    //FTS_TEST_SAVE_INFO("Read RawData...");
    memset(raw_data, 0, sizeof(raw_data));
    memset(tmp_rawdata, 0, sizeof(tmp_rawdata));
    ret = read_raw_data(0, 0xAD, test_data.screen_param.tx_num * test_data.screen_param.rx_num * 2, tmp_rawdata);
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("Failed to Get RawData \n");
        return ret;
    }

    for (row = 0; row < test_data.screen_param.tx_num; ++row) {
        for (col = 0; col < test_data.screen_param.rx_num; ++col) {
            raw_data[row][col] = tmp_rawdata[row * test_data.screen_param.rx_num + col];
        }
    }

    //--------------------------------------------Read RawData for Key Area
    memset(tmp_rawdata, 0, sizeof(tmp_rawdata));
    ret = read_raw_data( 0, 0xAE, test_data.screen_param.key_num_total * 2, tmp_rawdata );
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("Failed to Get RawData \n");
        return ret;
    }

    for (col = 0; col < test_data.screen_param.key_num_total; ++col) {
        raw_data[test_data.screen_param.tx_num][col] = tmp_rawdata[col];
    }

    return ret;

}

static int read_bytes_by_key( u8 key, int byte_num, u8 *byte_data )
{

    int ret = ERROR_CODE_OK;
    u8 i2c_w_buffer[3] = {0};
    unsigned short bytes_num_in_test_mode1 = 0;
    int i = 0;
    int read_num = 0;

    FTS_TEST_FUNC_ENTER();

    read_num =  byte_num / BYTES_PER_TIME;

    if (0 != (byte_num % BYTES_PER_TIME)) read_num++;

    if (byte_num <= BYTES_PER_TIME) {
        bytes_num_in_test_mode1 = byte_num;
    } else {
        bytes_num_in_test_mode1 = BYTES_PER_TIME;
    }


    i2c_w_buffer[0] = key;   //set begin address
    if (ret == ERROR_CODE_OK) {
        ret = fts_i2c_read_write( i2c_w_buffer, 1, byte_data, bytes_num_in_test_mode1);
    }

    for ( i = 1; i < read_num; i++) {
        if (ret != ERROR_CODE_OK) break;

        if (i == read_num - 1) { //last packet
            ret = fts_i2c_read_write( NULL, 0, byte_data + BYTES_PER_TIME * i, byte_num - BYTES_PER_TIME * i);
        } else
            ret = fts_i2c_read_write( NULL, 0, byte_data + BYTES_PER_TIME * i, BYTES_PER_TIME);

    }

    FTS_TEST_FUNC_EXIT();

    return ret;

}

/************************************************************************
* Name: weakshort_get_adc_data
* Brief:  Get Adc Data
* Input: all_adcdata_len
* Output: rev_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int weakshort_get_adc_data( int all_adcdata_len, int *rev_buffer  )
{
    int ret = ERROR_CODE_OK;
    u8 reg_mark = 0;
    int index = 0;
    int i = 0;
    int return_num = 0;
    u8 w_buffer[2] = {0};

    int read_num = all_adcdata_len / BYTES_PER_TIME;

    FTS_TEST_FUNC_ENTER();

    memset( w_buffer, 0, sizeof(w_buffer) );
    w_buffer[0] = 0x89;

    if ((all_adcdata_len % BYTES_PER_TIME) > 0) ++read_num;

    ret = write_reg( 0x0F, 1 );  //Start ADC sample

    for ( index = 0; index < 200; ++index ) {
        sys_delay(100 );
        ret = read_reg( 0x10, &reg_mark );  //Polling sampling end mark
        if ( ERROR_CODE_OK == ret && 0 == reg_mark )
            break;
    }
    if ( index >= 200) {
        FTS_TEST_SAVE_INFO("read_reg failed, ADC data not OK.");
        return 6;
    }

    {
        return_num = BYTES_PER_TIME;
        if (ret == ERROR_CODE_OK) {
            ret = fts_i2c_read_write(w_buffer, 1, read_buffer, return_num);
        }

        for ( i = 1; i < read_num; i++) {
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("fts_i2c_read_write  error.   !!!");
                break;
            }

            if (i == read_num - 1) { //last packet
                return_num = all_adcdata_len - BYTES_PER_TIME * i;
                ret = fts_i2c_read_write(NULL, 0, read_buffer + BYTES_PER_TIME * i, return_num);
            } else {
                return_num = BYTES_PER_TIME;
                ret = fts_i2c_read_write(NULL, 0, read_buffer + BYTES_PER_TIME * i, return_num);
            }
        }
    }

    for ( index = 0; index < all_adcdata_len / 2; ++index ) {
        rev_buffer[index] = (read_buffer[index * 2] << 8) + read_buffer[index * 2 + 1];
    }

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8736_get_key_num(void)
{
    int ret = 0;
    int i = 0;
    u8 keyval = 0;

    test_data.screen_param.key_num = 0;
    for (i = 0; i < 3; i++) {
        ret = read_reg( FT8736_LEFT_KEY_REG, &keyval );
        if (0 == ret) {
            if (((keyval >> 0) & 0x01)) {
                test_data.screen_param.left_key1 = true;
                ++test_data.screen_param.key_num;
            }
            if (((keyval >> 1) & 0x01)) {
                test_data.screen_param.left_key2 = true;
                ++test_data.screen_param.key_num;
            }
            if (((keyval >> 2) & 0x01)) {
                test_data.screen_param.left_key3 = true;
                ++test_data.screen_param.key_num;
            }
            break;
        } else {
            sys_delay(150);
            continue;
        }
    }

    if (i >= 3) {
        FTS_TEST_SAVE_INFO("can't get left key num");
        return ret;
    }

    for (i = 0; i < 3; i++) {
        ret = read_reg( FT8736_RIGHT_KEY_REG, &keyval );
        if (0 == ret) {
            if (((keyval >> 0) & 0x01)) {
                test_data.screen_param.right_key1 = true;
                ++test_data.screen_param.key_num;
            }
            if (((keyval >> 1) & 0x01)) {
                test_data.screen_param.right_key2 = true;
                ++test_data.screen_param.key_num;
            }
            if (((keyval >> 2) & 0x01)) {
                test_data.screen_param.right_key3 = true;
                ++test_data.screen_param.key_num;
            }
            break;
        } else {
            sys_delay(150);
            continue;
        }
    }

    if (i >= 3) {
        FTS_TEST_SAVE_INFO("can't get right key num");
        return ret;
    }

    return 0;
}

/************************************************************************
* Name: ft8736_show_data
* Brief:  Show ft8736_show_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ft8736_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
{
    int row, col;

    FTS_TEST_SAVE_INFO("\nVA Channels: ");
    for (row = 0; row < test_data.screen_param.tx_num; row++) {
        FTS_TEST_SAVE_INFO("\nCh_%02d:  ", row + 1);
        for (col = 0; col < test_data.screen_param.rx_num; col++) {
            FTS_TEST_SAVE_INFO("%5d, ", data[row][col]);
        }
    }
    if (include_key) {
        FTS_TEST_SAVE_INFO("\nKeys:  ");
        for ( col = 0; col < test_data.screen_param.key_num_total; col++ ) {
            FTS_TEST_SAVE_INFO("%5d, ",  data[test_data.screen_param.tx_num][col]);
        }
        FTS_TEST_SAVE_INFO("\n");
    }
}

/************************************************************************
* Name: ft8736_compare_detailthreshold_data
* Brief:  ft8736_compare_detailthreshold_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool ft8736_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
{
    int row, col;
    int value;
    bool tmp_result = true;
    int tmp_min, tmp_max;
    // VA
    for (row = 0; row < test_data.screen_param.tx_num; row++) {
        for (col = 0; col < test_data.screen_param.rx_num; col++) {
            if (test_data.incell_detail_thr.InvalidNode[row][col] == 0)continue; //Invalid Node
            tmp_min = data_min[row][col];
            tmp_max = data_max[row][col];
            value = data[row][col];
            if (value < tmp_min || value > tmp_max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                                   row + 1, col + 1, value, tmp_min, tmp_max);
            }
        }
    }
    // Key
    if (include_key) {
        row = test_data.screen_param.tx_num;
        for ( col = 0; col < test_data.screen_param.key_num_total; col++ ) {
            if (test_data.incell_detail_thr.InvalidNode[row][col] == 0)continue; //Invalid Node
            tmp_min = data_min[row][col];
            tmp_max = data_max[row][col];
            value = data[row][col];
            if (value < tmp_min || value > tmp_max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                                   row + 1, col + 1, value, tmp_min, tmp_max);
            }
        }
    }

    return tmp_result;
}

/************************************************************************
* Name: ft8736_compare_data
* Brief:  ft8736_compare_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool ft8736_compare_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min, int data_max, int vk_data_min, int vk_data_max, bool include_key)
{
    int row, col;
    int value;
    bool tmp_result = true;

    // VA
    for (row = 0; row < test_data.screen_param.tx_num; row++) {
        for (col = 0; col < test_data.screen_param.rx_num; col++) {
            if (test_data.incell_detail_thr.InvalidNode[row][col] == 0)continue; //Invalid Node
            value = data[row][col];
            if (value < data_min || value > data_max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                                   row + 1, col + 1, value, data_min, data_max);
            }
        }
    }
    // Key
    if (include_key) {
        row = test_data.screen_param.tx_num;
        for ( col = 0; col < test_data.screen_param.key_num_total; col++ ) {
            if (test_data.incell_detail_thr.InvalidNode[row][col] == 0)continue; //Invalid Node
            value = data[row][col];
            if (value < vk_data_min || value > vk_data_max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                                   row + 1, col + 1, value, vk_data_min, vk_data_max);
            }
        }
    }

    return tmp_result;
}

void init_testitem_ft8736(char *strIniFile)
{
    char str[512];
    FTS_TEST_FUNC_ENTER();

    /////////////////////////////////// RAWDATA_TEST
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    ft8736_item.rawdata_test = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    ft8736_item.cb_test = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "SHORT_CIRCUIT_TEST", "1", str, strIniFile);
    ft8736_item.short_test = fts_atoi(str);

    /////////////////////////////////// OPEN_TEST
    GetPrivateProfileString("TestItem", "OPEN_TEST", "0", str, strIniFile);
    ft8736_item.open_test = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem", "lcd_noise_test", "0", str, strIniFile);
    ft8736_item.lcd_noise_test = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_basicthreshold_ft8736(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VA_Check", "1", str, strIniFile);
    ft8736_basic_thr.rawdata_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "5000", str, strIniFile);
    ft8736_basic_thr.rawdata_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "11000", str, strIniFile);
    ft8736_basic_thr.rawdata_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VKey_Check", "1", str, strIniFile);
    ft8736_basic_thr.rawdata_test_vk_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min_VKey", "5000", str, strIniFile);
    ft8736_basic_thr.rawdata_test_min_vk = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max_VKey", "11000", str, strIniFile);
    ft8736_basic_thr.rawdata_test_max_vk = fts_atoi(str);

    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold", "CBTest_VA_Check", "1", str, strIniFile);
    ft8736_basic_thr.cb_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min", "3", str, strIniFile);
    ft8736_basic_thr.cb_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max", "100", str, strIniFile);
    ft8736_basic_thr.cb_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_VKey_Check", "1", str, strIniFile);
    ft8736_basic_thr.cb_test_vk_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min_Vkey", "3", str, strIniFile);
    ft8736_basic_thr.cb_test_min_vk = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max_Vkey", "100", str, strIniFile);
    ft8736_basic_thr.cb_test_max_vk = fts_atoi(str);

    //////////////////////////////////////////////////////////// Short Circuit Test
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_ResMin", "1200", str, strIniFile);
    ft8736_basic_thr.short_res_min = fts_atoi(str);

    ////////////////////////////////////////////////////////////LCDNoiseTest
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_FrameNum", "200", str, strIniFile);
    ft8736_basic_thr.lcd_noise_test_frame_num = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_Coefficient", "60", str, strIniFile);
    ft8736_basic_thr.lcd_noise_test_coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_NoiseMode", "0", str, strIniFile);
    ft8736_basic_thr.lcd_noise_test_noise_mode = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_Coefficient_Key", "60", str, strIniFile);
    ft8736_basic_thr.lcd_noise_test_coefficient_key = fts_atoi(str);

    ////////////////////////////////////////////////////////////open test
    GetPrivateProfileString("Basic_Threshold", "OpenTest_CBMin", "100", str, strIniFile);
    ft8736_basic_thr.open_test_cb_min = fts_atoi(str);
    FTS_TEST_FUNC_EXIT();
}

void init_detailthreshold_ft8736(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_CBTest(ini);
    OnThreshold_VkAndVaRawDataSeparateTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft8736(void)
{
    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();
    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(CODE_FT8736_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////Short Test
    if ( ft8736_item.short_test == 1) {
        fts_set_testitem(CODE_FT8736_SHORT_CIRCUIT_TEST);
    }

    //////////////////////////////////////////////////cb_test
    if ( ft8736_item.cb_test == 1) {
        fts_set_testitem(CODE_FT8736_CB_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( ft8736_item.rawdata_test == 1) {
        fts_set_testitem(CODE_FT8736_RAWDATA_TEST);
    }


    //////////////////////////////////////////////////lcd_noise_test
    if ( ft8736_item.lcd_noise_test == 1) {
        fts_set_testitem(CODE_FT8736_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////open_test
    if ( ft8736_item.open_test == 1) {
        fts_set_testitem(CODE_FT8736_OPEN_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft8736,
    .init_basicthreshold = init_basicthreshold_ft8736,
    .init_detailthreshold = init_detailthreshold_ft8736,
    .set_testitem_sequence  = set_testitem_sequence_ft8736,
    .start_test = start_test_ft8736,
};
#endif

