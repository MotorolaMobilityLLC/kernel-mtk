/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Focaltech_test_ft8607.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: test item for FT8607
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/

#include "../focaltech_test.h"

#if (FTS_CHIP_TEST_TYPE ==FT8607_TEST)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define DEVIDE_MODE_ADDR                0x00
#define REG_LINE_NUM                    0x01
#define REG_TX_NUM                      0x02
#define REG_RX_NUM                      0x03
#define REG_RawBuf0                     0x6A
#define REG_RawBuf1                     0x6B
#define REG_OrderBuf0                   0x6C
#define REG_CbBuf0                      0x6E
#define REG_CbAddrH                     0x18
#define REG_CbAddrL                     0x19
#define REG_OrderAddrH                  0x1A
#define REG_OrderAddrL                  0x1B
#define FT_8607_LEFT_KEY_REG             0X1E
#define FT_8607_RIGHT_KEY_REG            0X1F
#define REG_8607_LCD_NOISE_FRAME         0X12
#define REG_8607_LCD_NOISE_START         0X11
#define REG_8607_LCD_NOISE_NUMBER        0X13
#define REG_CLB                          0x04
#define REG_FWVERSION                    0xA6
#define REG_FACTORYID                    0xA8
#define REG_FWCNT                        0x17


/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct ft8607_test_item {
    bool rawdata_test;
    bool cb_test;
    bool short_test;
    bool lcd_noise_test;
    bool open_test;

};

struct ft8607_basic_threshold {
    int rawdata_test_min;
    int rawdata_test_max;
    bool cb_test_va_check;
    int cb_test_min;
    int cb_test_max;
    bool cb_test_vk_check;
    int cb_test_min_vk;
    int cb_test_max_vk;
    int short_res_min;
    int lcd_noise_test_frame;
    int lcd_noise_test_max;
    int lcd_noise_coefficient;
    int lcd_noise_coefficient_key;
    int open_test_cb_min;
};


enum enumTestItem_FT8607 {
    CODE_FT8607_ENTER_FACTORY_MODE = 0,//All IC are required to test items
    CODE_FT8607_RAWDATA_TEST = 7,
    CODE_FT8607_CB_TEST = 12,
    CODE_FT8607_SHORT_CIRCUIT_TEST = 14,
    CODE_FT8607_LCD_NOISE_TEST = 15,
    CODE_FT8607_OPEN_TEST = 24,
};

/*****************************************************************************
* Static variables
*****************************************************************************/

static int raw_data[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int cb_data[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static u8 temp_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0}; //One-dimensional
static int temp_rawdata[TX_NUM_MAX *RX_NUM_MAX] = {0};
static unsigned char read_buffer[80 * 80 * 2] = {0};
static int adc_data[TX_NUM_MAX *RX_NUM_MAX] =  {0};
static int short_res[TX_NUM_MAX][RX_NUM_MAX] = { {0} };
static int lcd_noise[TX_NUM_MAX][RX_NUM_MAX] = { {0} };
static int screen_noise_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0};

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

struct ft8607_test_item ft8607_item;
struct ft8607_basic_threshold ft8607_basic_thr;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
/////////////////////////////////////////////////////////////
static int start_scan(void);
static unsigned char read_raw_data(unsigned char freq, unsigned char line_num, int byte_num, int *rev_buffer);
static unsigned char get_tx_rx_cb(unsigned short start_node, unsigned short read_num, unsigned char *read_buffer);
static unsigned char get_raw_data(void);
static unsigned char read_lcd_noise_data(int byte_num, int *rev_buffer);
static unsigned char weak_short_get_adc_data( int all_adc_data_len, int *rev_buffer );
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, unsigned char row, unsigned char col, unsigned char item_count);
static unsigned char chip_clb(unsigned char *clb_result);
static int ft8607_get_key_num(void);
static void ft8607_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static bool ft8607_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static bool ft8607_compare_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min, int data_max, int vk_data_min, int vk_data_max, bool include_key);
static int ft8607_enter_factory_mode(void);
static int ft8607_rawdata_test(bool *test_result);
static int ft8607_cb_test(bool *test_result);
static int ft8607_short_test(bool *test_result);
static int ft8607_open_test(bool *test_result);
static int ft8607_lcdnoise_test(bool *test_result);


/************************************************************************
* Name: start_test_ft8607
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/

bool start_test_ft8607(void)
{
    bool test_result = true, temp_result = 1;
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

        ///////////////////////////////////////////////////////FT8607_ENTER_FACTORY_MODE
        if (CODE_FT8607_ENTER_FACTORY_MODE == test_data.test_item[item_count].itemcode) {
            ret = ft8607_enter_factory_mode();
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8607_RAWDATA_TEST
        if (CODE_FT8607_RAWDATA_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8607_rawdata_test(&temp_result);
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT8607_CB_TEST
        if (CODE_FT8607_CB_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8607_cb_test(&temp_result); //
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8607_SHORT_TEST
        if (CODE_FT8607_SHORT_CIRCUIT_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8607_short_test(&temp_result);
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8607_Open_TEST
        if (CODE_FT8607_OPEN_TEST == test_data.test_item[item_count].itemcode) {
            ret = ft8607_open_test(&temp_result);
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8607_LCD_NOISE_TEST
        if (CODE_FT8607_LCD_NOISE_TEST == test_data.test_item[item_count].itemcode) {
            ret =  ft8607_lcdnoise_test(&temp_result);
            if (ERROR_CODE_OK != ret || (!temp_result)) {
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
* Name: ft8607_enter_factory_mode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8607_enter_factory_mode(void)
{
    int ret = 0;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("enter factory mode fail, can't get tx/rx num");
        return ret;
    }
    ret = ft8607_get_key_num();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("get key num fail");
        return ret;
    }

    return ret;

}
static int ft8607_rawdata_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    int i = 0;

    FTS_TEST_SAVE_INFO("\n\n\n==============================Test Item: -------- Raw Data Test\n\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        return ret;
    }

    for (i = 0 ; i < 3; i++) //Lost 3 Frames, In order to obtain stable data
        ret = get_raw_data();
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to get Raw Data!! Error Code: %d\n", ret);
        return ret;
    }
    //----------------------------------------------------------Show RawData
    ft8607_show_data(raw_data, true);

    //----------------------------------------------------------To Determine RawData if in Range or not
    tmp_result = ft8607_compare_detailthreshold_data(raw_data, test_data.incell_detail_thr.RawDataTest_Min, test_data.incell_detail_thr.RawDataTest_Max, true);


    //////////////////////////////Save Test Data
    save_test_data(raw_data, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);

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
* Name: ft8607_cb_test
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8607_cb_test(bool *test_result)
{
    bool tmp_result = true;
    int ret = ERROR_CODE_OK;
    int row = 0;
    int col = 0;
    unsigned char clb_result = 0;
    int i = 0;
    bool include_key = false;

    include_key = ft8607_basic_thr.cb_test_vk_check;

    FTS_TEST_SAVE_INFO("\n\n\n==============================Test Item: --------  CB Test\n\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        goto TEST_ERR;
    }

    for ( i = 0; i < 10; i++) {
        FTS_TEST_SAVE_INFO("\n start chipclb times:%d. ", i);

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

    ret = get_tx_rx_cb( 0, (short)(test_data.screen_param.tx_num * test_data.screen_param.rx_num + test_data.screen_param.key_num_total), temp_data );

    if ( ERROR_CODE_OK != ret ) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\nFailed to get CB value...\n");
        goto TEST_ERR;
    }

    memset(cb_data, 0, sizeof(cb_data));

    for ( row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            cb_data[row][col] = temp_data[ row * test_data.screen_param.rx_num + col ];
        }
    }
    ///key
    for ( col = 0; col < test_data.screen_param.key_num_total; ++col ) {
        cb_data[test_data.screen_param.tx_num][col] = temp_data[ test_data.screen_param.tx_num * test_data.screen_param.rx_num + col ];
    }

    //------------------------------------------------Show CbData
    ft8607_show_data(cb_data, include_key);

    //----------------------------------------------------------To Determine Cb if in Range or not
    tmp_result = ft8607_compare_detailthreshold_data(cb_data, test_data.incell_detail_thr.CBTest_Min, test_data.incell_detail_thr.CBTest_Max, include_key);

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
* Name: ft8607_short_test
* Brief:  Get short circuit test mode data, judge whether there is a short circuit
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8607_short_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool temp_result = true;
    int res_min = 0;
    int all_adc_data_num = 0;
    unsigned char tx_num = 0, rx_num = 0, channel_num = 0;
    int row = 0;
    int col = 0;
    int i = 0;
    int tmp_adc = 0;
    int value_min = 0;
    int value_max = 0;

    FTS_TEST_SAVE_INFO("\n==============================Test Item: -------- Short Circuit Test \r\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        temp_result = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        goto TEST_END;
    }

    ret = read_reg(0x02, &tx_num);
    ret = read_reg(0x03, &rx_num);
    if (ERROR_CODE_OK != ret) {
        temp_result = false;
        FTS_TEST_SAVE_INFO("\n// Failed to read reg. Error Code: %d", ret);
        goto TEST_END;
    }

    FTS_TEST_SAVE_INFO("\n tx_num:%d.  rx_num:%d.", tx_num, rx_num);

    channel_num = tx_num + rx_num;
    all_adc_data_num = tx_num * rx_num + test_data.screen_param.key_num_total;
    memset(adc_data, 0, sizeof(adc_data));

    for (i = 0; i < 1; i++) {
        ret = weak_short_get_adc_data(all_adc_data_num * 2, adc_data);
        sys_delay(50);
        if (ERROR_CODE_OK != ret) {
            temp_result = false;
            FTS_TEST_SAVE_INFO("\n // Failed to get AdcData. Error Code: %d", ret);
            goto TEST_END;
        }
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

    for ( row = 0; row < test_data.screen_param.tx_num + 1; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            tmp_adc = adc_data[row * rx_num + col];
            if (tmp_adc > 2007) tmp_adc = 2007;
            short_res[row][col] = (tmp_adc * 100) / (2047 - tmp_adc);
        }
    }

    //////////////////////// analyze
    res_min = ft8607_basic_thr.short_res_min;
    value_min = res_min;
    value_max = 100000000;
    FTS_TEST_SAVE_INFO("\n Short Circuit test , Set_Range=(%d, %d). \n", \
                       value_min, value_max);

    temp_result = ft8607_compare_data(short_res, value_min, value_max, value_min, value_max, true);

TEST_END:

    if (temp_result) {
        FTS_TEST_SAVE_INFO("\n==========Short Circuit Test is OK!");
        * test_result = true;
    } else {
        FTS_TEST_SAVE_INFO("\n==========Short Circuit Test is NG!");
        * test_result = false;
    }

    return ret;
}

/************************************************************************
* Name: ft8607_open_test
* Brief:  Check if channel is opens
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8607_open_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    int index = 0;
    unsigned char ch_value = 0xff;
    unsigned char clb_result = 0;
    int row = 0;
    int col = 0;
    int min = 0;
    int max = 0;
    unsigned char reg_value = 0;

    FTS_TEST_SAVE_INFO("\n\r\n\r\n==============================Test Item: --------  Open Test");
    ret = enter_factory_mode();
    sys_delay(50);
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n\r\n//=========  Enter Factory Failed!");
        goto TEST_ERR;
    }

    if (test_data.screen_param.selected_ic == IC_FT8607) {
        ret = write_reg(0xf8, 0x01);
        for ( index = 0; index < 40; ++index ) {
            ret = read_reg( 0xf8, &ch_value );
            if ( ret != ERROR_CODE_OK ) {
                tmp_result = false;
                break;
            }
            if ( ch_value == 0xaa )  break;
            sys_delay( 50 );
        }

        if ( ch_value != 0xaa ) {
            tmp_result = false;
            FTS_TEST_SAVE_INFO( "\r\n//Open Test Timeout!" );
            goto TEST_ERR;
        }
    }

    if (test_data.screen_param.selected_ic == IC_FT8607U) {
        ret = read_reg(0x21, &reg_value);
        ret = write_reg(0x21, 2);
        if (ERROR_CODE_OK != ret) {
            tmp_result = false;
            FTS_TEST_SAVE_INFO("\n\r\n//=========  write reg failed !");
            goto TEST_ERR;
        }

        ////auto clb
        ret = chip_clb( &clb_result );
        if (ERROR_CODE_OK != ret) {
            tmp_result = false;
            FTS_TEST_SAVE_INFO("\n\r\n//========= auto clb Failed  !");
            goto TEST_ERR;
        }
    }

    memset(cb_data, 0, sizeof(cb_data));
    memset(temp_data, 0, sizeof(temp_data));
    //get cb data
    ret = get_tx_rx_cb( 0, (short)(test_data.screen_param.tx_num * test_data.screen_param.rx_num + test_data.screen_param.key_num_total), temp_data );
    if ( ERROR_CODE_OK != ret ) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n\r\n\r\n//=========get CB Failed!");

    }
    if (test_data.screen_param.selected_ic == IC_FT8607U) {
        ret = write_reg( 0x21, reg_value );
    }
    ////auto clb
    ret = chip_clb( &clb_result );
    if (ERROR_CODE_OK != ret) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("\n\r\n//========= auto clb Failed!");
        goto TEST_ERR;
    }

    for ( row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            cb_data[row][col] = temp_data[ row * test_data.screen_param.rx_num + col ];
        }
    }

    // show open data
    ft8607_show_data(cb_data, false);

    //  analyze
    min = ft8607_basic_thr.open_test_cb_min;
    max = 200;
    tmp_result = ft8607_compare_data(cb_data, min, max, min, max, false);

    // save data
    save_test_data(cb_data, 0, test_data.screen_param.tx_num, test_data.screen_param.rx_num, 1);
    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========Open Test is OK!\n");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========Open Test is NG!\n");
    }

    return ret;
TEST_ERR:

    chip_clb( &clb_result );
    * test_result = false;
    FTS_TEST_SAVE_INFO("\n==========Open Test is NG!\n");
    return ret;
}

/************************************************************************
* Name: ft8607_lcdnoise_test
* Brief:  TestItem: LCD NoiseTest. Check if Noise is within the range.
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8607_lcdnoise_test(bool *test_result)
{
    int ret = ERROR_CODE_COMM_ERROR;
    bool result_flag = true;
    unsigned char old_mode = 0;
    int row = 0;
    int col = 0;
    int value_min = 0;
    int value_max = 0;
    int value_max_vk = 0;
    int retry = 0;
    unsigned char status = 0xff;
    unsigned char ch_noise_value_va = 0xff, ch_noise_value_key = 0xff;
    int index  = 0;

    FTS_TEST_SAVE_INFO("\n==============================Test Item: -------- LCD Noise Test \r\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        goto TEST_END;
    }

    //switch is differ mode
    read_reg( 0x06, &old_mode );
    write_reg( 0x06, 1 );

    //clear  FW counter
    ret = write_reg( 0x01, 0xAD );


    //set scan number
    FTS_TEST_SAVE_INFO("\n lcd_noise_test_frame:%d. ",  ft8607_basic_thr.lcd_noise_test_frame );
    ret = write_reg( REG_8607_LCD_NOISE_FRAME,  (unsigned char)(ft8607_basic_thr.lcd_noise_test_frame & 0xFF) );
    ret = write_reg( REG_8607_LCD_NOISE_FRAME + 1, (unsigned char)( (ft8607_basic_thr.lcd_noise_test_frame >> 8) & 0xFF) );

    //  start test.
    ret = write_reg( REG_8607_LCD_NOISE_START, 0x01 );

    //check status
    for ( retry = 0; retry < 50; ++retry ) {
        status = 0xff;
        ret = read_reg( REG_8607_LCD_NOISE_START, &status );

        if ( ERROR_CODE_OK == ret && status == 0x00  ) {
            FTS_TEST_SAVE_INFO("\n read register 0x%x, get value is:%d. \n ", REG_8607_LCD_NOISE_START, status);
            break;
        }

        sys_delay( 500 );
    }
    if ( retry == 50 ) {
        result_flag = false;
        FTS_TEST_SAVE_INFO("\nScan Noise Time Out!");
        goto TEST_END;
    }


    memset(raw_data, 0, sizeof(raw_data));
    memset(screen_noise_data, 0, sizeof(screen_noise_data));
    ret = read_lcd_noise_data(test_data.screen_param.tx_num * test_data.screen_param.rx_num * 2 + test_data.screen_param.key_num_total * 2, screen_noise_data );
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to Read Data.\n");
        result_flag = false;
        goto TEST_END;
    }

    for ( row = 0; row < test_data.screen_param.tx_num; ++row ) {
        for (  col = 0; col < test_data.screen_param.rx_num; ++col ) {
            index = row * test_data.screen_param.rx_num + col;
            raw_data[row][col] = screen_noise_data[index];
        }
    }
    for (  col = 0; col < test_data.screen_param.key_num_total; ++col ) {
        index = test_data.screen_param.tx_num * test_data.screen_param.rx_num + col;
        raw_data[test_data.screen_param.tx_num][col] = (int)(short)( screen_noise_data[index]);
    }

    ret = enter_work_mode();
    sys_delay(100);
    ret = read_reg( 0x80, &ch_noise_value_va );
    ret = read_reg( 0x82, &ch_noise_value_key );
    FTS_TEST_SAVE_INFO("\n ch_noise_value_va: %d, ch_noise_value_key: %d. \n", ch_noise_value_va, ch_noise_value_key);
    ret = enter_factory_mode();
    sys_delay(100);

    for ( row = 0; row < test_data.screen_param.tx_num + 1; ++row ) {
        for ( col = 0; col < test_data.screen_param.rx_num; ++col ) {
            lcd_noise[row][col] = focal_abs(raw_data[row][col]);
        }
    }

    //show data of lcd_noise
    ft8607_show_data(lcd_noise, true);

    //////////////////////// analyze
    if (test_data.screen_param.selected_ic == IC_FT8607U) {
        value_min = 0;
        value_max = ft8607_basic_thr.lcd_noise_coefficient * ch_noise_value_va * 32 / 100;
        FTS_TEST_SAVE_INFO("\nVA_Set_Range=(%d, %d). ", value_min, value_max);
        result_flag = ft8607_compare_data(lcd_noise, value_min, value_max, value_min, value_max, true);
    }

    else if (test_data.screen_param.selected_ic == IC_FT8607) {

        value_min = 0;
        value_max = ft8607_basic_thr.lcd_noise_coefficient * ch_noise_value_va * 32 / 100;
        value_max_vk = ft8607_basic_thr.lcd_noise_coefficient_key * ch_noise_value_key * 32 / 100;
        FTS_TEST_SAVE_INFO("\nVA_Set_Range=(%d, %d). Key_Set_Range=(%d, %d). ", value_min, value_max, value_min, value_max_vk);
        result_flag = ft8607_compare_data(lcd_noise, value_min, value_max, value_min, value_max_vk, true);
    }

    //  Save Test Data
    save_test_data(lcd_noise, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);

TEST_END:

    write_reg( 0x06, old_mode );
    sys_delay( 20 );

    if (result_flag) {
        FTS_TEST_SAVE_INFO("\n==========LCD Noise Test is OK!");
        * test_result = true;
    } else {
        FTS_TEST_SAVE_INFO("\n==========LCD Noise Test is NG!");
        * test_result = false;
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
static unsigned char read_raw_data(unsigned char freq, unsigned char line_num, int byte_num, int *rev_buffer)
{
    int ret = ERROR_CODE_COMM_ERROR;
    unsigned char i2c_w_buffer[3] = {0};
    unsigned char read_data[byte_num];
    int i, read_num;
    unsigned short bytes_per_time = 0;

    FTS_TEST_FUNC_ENTER();

    memset(read_data, 0, sizeof(read_data));
    read_num = byte_num / BYTES_PER_TIME;

    if (0 != (byte_num % BYTES_PER_TIME)) read_num++;

    if (byte_num <= BYTES_PER_TIME) {
        bytes_per_time = byte_num;
    } else {
        bytes_per_time = BYTES_PER_TIME;
    }

    ret = write_reg(REG_LINE_NUM, line_num);//Set row addr;    //  set  0x01 register's value to 0xAD to clear 0 of FM

    if (ret != ERROR_CODE_OK) {
        FTS_TEST_SAVE_INFO("\nFailed to write REG_LINE_NUM! ");
        goto READ_ERR;
    }

    //***********************************************************Read raw data in test mode1
    i2c_w_buffer[0] = REG_RawBuf0;   //set begin address
    if (ret == ERROR_CODE_OK) {
        sys_delay(10);
        ret = fts_i2c_read_write(i2c_w_buffer, 1, read_data, bytes_per_time);
        if (ret != ERROR_CODE_OK) {
            FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!1 ");
            goto READ_ERR;
        }
    }

    for (i = 1; i < read_num; i++) {
        if (ret != ERROR_CODE_OK) break;

        if (i == read_num - 1) { //last packet
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, byte_num - BYTES_PER_TIME * i);
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!2 ");
                goto READ_ERR;
            }
        } else {
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, BYTES_PER_TIME);
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!3 ");
                goto READ_ERR;
            }
        }

    }

    if (ret == ERROR_CODE_OK) {
        for (i = 0; i < (byte_num >> 1); i++) {
            rev_buffer[i] = (read_data[i << 1] << 8) + read_data[(i << 1) + 1];
        }
    }
    FTS_TEST_FUNC_EXIT();

READ_ERR:
    return ret;

}

/************************************************************************
* Name: save_test_data
* Brief:  Storage format of test data
* Input: int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, unsigned char row, unsigned char col, unsigned char item_count
* Output: none
* Return: none
***********************************************************************/
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, unsigned char row, unsigned char col, unsigned char item_count)
{
    int len = 0;
    int i = 0, j = 0;

    FTS_TEST_FUNC_ENTER();

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
                len = sprintf(test_data.tmp_buffer, "%d, \n", data[i][j]);
            else
                len = sprintf(test_data.tmp_buffer, "%d, ", data[i][j]);

            memcpy(test_data.store_data_area + test_data.len_store_data_area, test_data.tmp_buffer, len);
            test_data.len_store_data_area += len;
        }
    }
}
/************************************************************************
* Name: start_scan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int start_scan(void)
{
    unsigned char reg_val = 0;
    unsigned char times = 0;
    const unsigned char max_times = 250;
    int ret = ERROR_CODE_COMM_ERROR;

    FTS_TEST_FUNC_ENTER();

    ret = read_reg(DEVIDE_MODE_ADDR, &reg_val);
    if (ret == ERROR_CODE_OK) {
        reg_val |= 0x80;     //Top bit position 1, start scan
        ret = write_reg(DEVIDE_MODE_ADDR, reg_val);
        if (ret == ERROR_CODE_OK) {
            while (times++ < max_times) {    //Wait for the scan to complete
                sys_delay(16);
                ret = read_reg(DEVIDE_MODE_ADDR, &reg_val);
                if (ret == ERROR_CODE_OK) {
                    if ((reg_val >> 7) == 0)    break;
                } else {
                    FTS_TEST_SAVE_INFO("\nStartScan read DEVIDE_MODE_ADDR error.\n");
                    break;
                }
            }
            if (times < max_times)    ret = ERROR_CODE_OK;
            else ret = ERROR_CODE_COMM_ERROR;
        } else
            FTS_TEST_SAVE_INFO("\nStartScan write DEVIDE_MODE_ADDR error.\n");
    } else
        FTS_TEST_SAVE_INFO("\nStartScan read DEVIDE_MODE_ADDR error.\n");
    return ret;

}

/************************************************************************
* Name: get_tx_rx_cb(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx/Rx
* Input: start_node, read_num
* Output: read_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char get_tx_rx_cb(unsigned short start_node, unsigned short read_num, unsigned char *read_buffer)
{
    int ret = ERROR_CODE_OK;
    unsigned short return_num = 0;//Number to return in every time
    unsigned short total_return_num = 0;//Total return number
    unsigned char w_buffer[4] = {0};
    int i = 0, bytes_num = 0;

    FTS_TEST_FUNC_ENTER();

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
        //ret = fts_i2c_read_test(w_buffer, 1, read_buffer+total_return_num, return_num);
        ret = fts_i2c_read_write(w_buffer, 1, read_buffer + total_return_num, return_num);

        total_return_num += return_num;

        if (ret != ERROR_CODE_OK)return ret;

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
static unsigned char get_raw_data(void)
{
    int ret = ERROR_CODE_OK;
    int row = 0, col = 0;


    //--------------------------------------------Start Scanning
    ret = start_scan();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\nFailed to Scan ...\n");
        return ret;
    }

    //--------------------------------------------Read RawData for Channel Area
    memset(raw_data, 0, sizeof(raw_data));
    memset(temp_rawdata, 0, sizeof(temp_rawdata));
    ret = read_raw_data(0, 0xAD, test_data.screen_param.tx_num * test_data.screen_param.rx_num * 2, temp_rawdata);
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to Get RawData of channel.\n");
        return ret;
    }
    for (row = 0; row < test_data.screen_param.tx_num; ++row) {
        for (col = 0; col < test_data.screen_param.rx_num; ++col) {
            raw_data[row][col] = temp_rawdata[row * test_data.screen_param.rx_num + col];
        }
    }

    memset(temp_rawdata, 0, sizeof(temp_rawdata));
    ret = read_raw_data( 0, 0xAE, test_data.screen_param.key_num_total * 2, temp_rawdata );
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to Get RawData of keys.\n");
        return ret;
    }

    for (col = 0; col < test_data.screen_param.key_num_total; ++col) {
        raw_data[test_data.screen_param.tx_num][col] = temp_rawdata[col];
    }

    return ret;
}

//Auto clb
static unsigned char chip_clb(unsigned char *clb_result)
{
    unsigned char reg_data = 0;
    unsigned char timeout_times = 50;        //5s
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
* Name: read_lcd_noise_data
* Brief:  Read Lcd Noise Data
* Input: byte_num
* Output: rev_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char read_lcd_noise_data(int byte_num, int *rev_buffer)
{
    int ret = ERROR_CODE_OK;
    unsigned char i2c_w_buffer[3] = {0};
    unsigned char read_data[byte_num];
    int i, read_num;
    unsigned short bytes_per_time = 0;

    FTS_TEST_FUNC_ENTER();

    memset(read_data, 0, sizeof(read_data));
    read_num = byte_num / BYTES_PER_TIME;

    if (0 != (byte_num % BYTES_PER_TIME)) read_num++;

    if (byte_num <= BYTES_PER_TIME) {
        bytes_per_time = byte_num;
    } else {
        bytes_per_time = BYTES_PER_TIME;
    }

    //***********************************************************Read raw data in test mode1
    i2c_w_buffer[0] = REG_RawBuf0;   //set begin address
    if (true) {
        sys_delay(10);
        ret = fts_i2c_read_write(i2c_w_buffer, 1, read_data, bytes_per_time);
        if (ret != ERROR_CODE_OK) {
            FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!1 ");
            goto READ_ERR;
        }
    }

    for (i = 1; i < read_num; i++) {
        if (ret != ERROR_CODE_OK) break;

        if (i == read_num - 1) { //last packet
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, byte_num - BYTES_PER_TIME * i);
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!2 ");
                goto READ_ERR;
            }
        } else {
            sys_delay(10);
            ret = fts_i2c_read_write(NULL, 0, read_data + BYTES_PER_TIME * i, BYTES_PER_TIME);
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("\nread rawdata fts_i2c_read_write Failed!3 ");
                goto READ_ERR;
            }
        }

    }

    if (ret == ERROR_CODE_OK) {
        for (i = 0; i < (byte_num >> 1); i++) {
            rev_buffer[i] = (read_data[i << 1] << 8) + read_data[(i << 1) + 1];

        }
    }

    FTS_TEST_FUNC_EXIT();

READ_ERR:
    return ret;

}

/************************************************************************
* Name: weak_short_get_adc_data
* Brief:  Get Adc Data
* Input: all_adc_data_len
* Output: rev_buffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char weak_short_get_adc_data( int all_adc_data_len, int *rev_buffer )

{
    int ret = ERROR_CODE_OK;
    unsigned char reg_mark = 0;
    int index = 0;
    int i = 0;
    int return_num = 0;
    unsigned char w_buffer[2] = {0};

    int read_num = all_adc_data_len / BYTES_PER_TIME;

    FTS_TEST_FUNC_ENTER();

    memset( w_buffer, 0, sizeof(w_buffer) );
    w_buffer[0] = 0x89;

    if ((all_adc_data_len % BYTES_PER_TIME) > 0) ++read_num;

    ret = write_reg( 0x0F, 1 );  //Start ADC sample

    for ( index = 0; index < 50; ++index ) {
        sys_delay( 50 );
        ret = read_reg( 0x10, &reg_mark );  //Polling sampling end mark
        if ( ERROR_CODE_OK == ret && 0 == reg_mark )
            break;
    }
    if ( index >= 50) {
        FTS_TEST_SAVE_INFO("\nReadReg failed, ADC data not OK.");
        return ERROR_CODE_WAIT_RESPONSE_TIMEOUT;
    }

    {
        return_num = BYTES_PER_TIME;
        if (ret == ERROR_CODE_OK) {
            ret = fts_i2c_read_write(w_buffer, 1, read_buffer, return_num);
        }

        for ( i = 1; i < read_num; i++) {
            if (ret != ERROR_CODE_OK) {
                FTS_TEST_SAVE_INFO("\nfts_i2c_read_write  error.   !!!");
                break;
            }

            if (i == read_num - 1) { //last packet
                return_num = all_adc_data_len - BYTES_PER_TIME * i;
                ret = fts_i2c_read_write(NULL, 0, read_buffer + BYTES_PER_TIME * i, return_num);
            } else {
                return_num = BYTES_PER_TIME;
                ret = fts_i2c_read_write(NULL, 0, read_buffer + BYTES_PER_TIME * i, return_num);
            }
        }
    }

    for ( index = 0; index < all_adc_data_len / 2; ++index ) {
        rev_buffer[index] = (read_buffer[index * 2] << 8) + read_buffer[index * 2 + 1];
    }

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8607_get_key_num(void)
{
    int ret = 0;
    int i = 0;
    u8 keyval = 0;

    test_data.screen_param.key_num = 0;
    for (i = 0; i < 3; i++) {
        ret = read_reg( FT_8607_LEFT_KEY_REG, &keyval );
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
        ret = read_reg( FT_8607_RIGHT_KEY_REG, &keyval );
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
* Name: ft8607_show_data
* Brief:  Show ft8607_show_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ft8607_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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
* Name: ft8607_compare_detailthreshold_data
* Brief:  ft8607_compare_detailthreshold_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool ft8607_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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
* Name: ft8607_compare_data
* Brief:  ft8607_compare_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool ft8607_compare_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min, int data_max, int vk_data_min, int vk_data_max, bool include_key)
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

void init_basicthreshold_ft8607(char *strIniFile)
{
    char str[512] = {0};

    FTS_TEST_FUNC_ENTER();

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "5000", str, strIniFile);
    ft8607_basic_thr.rawdata_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "11000", str, strIniFile);
    ft8607_basic_thr.rawdata_test_max = fts_atoi(str);

    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold", "CBTest_VA_Check", "1", str, strIniFile);
    ft8607_basic_thr.cb_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min", "3", str, strIniFile);
    ft8607_basic_thr.cb_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max", "60", str, strIniFile);
    ft8607_basic_thr.cb_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_VKey_Check", "1", str, strIniFile);
    ft8607_basic_thr.cb_test_vk_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min_Vkey", "3", str, strIniFile);
    ft8607_basic_thr.cb_test_min_vk = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max_Vkey", "100", str, strIniFile);
    ft8607_basic_thr.cb_test_max_vk = fts_atoi(str);

    //////////////////////////////////////////////////////////// Short Circuit Test
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_ResMin", "200", str, strIniFile);
    ft8607_basic_thr.short_res_min = fts_atoi(str);

    //////////////////////////////////////////////////////////// LCD Noise Test
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Frame", "50", str, strIniFile);
    ft8607_basic_thr.lcd_noise_test_frame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Max", "30", str, strIniFile);
    ft8607_basic_thr.lcd_noise_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Coefficient", "50", str, strIniFile);
    ft8607_basic_thr.lcd_noise_coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Coefficient_Key", "50", str, strIniFile);
    ft8607_basic_thr.lcd_noise_coefficient_key = fts_atoi(str);

    //////////////////////////////////////////////////////////// Open Test
    GetPrivateProfileString("Basic_Threshold", "OpenTest_CBMin", "64", str, strIniFile);
    ft8607_basic_thr.open_test_cb_min = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_testitem_ft8607(char  *strIniFile)
{
    char str[512] = {0};

    FTS_TEST_FUNC_ENTER();

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    ft8607_item.rawdata_test = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    ft8607_item.cb_test = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "SHORT_CIRCUIT_TEST", "1", str, strIniFile);
    ft8607_item.short_test = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem", "LCD_NOISE_TEST", "0", str, strIniFile);
    ft8607_item.lcd_noise_test = fts_atoi(str);

    /////////////////////////////////// OPEN_TEST
    GetPrivateProfileString("TestItem", "OPEN_TEST", "0", str, strIniFile);
    ft8607_item.open_test = fts_atoi(str);
    FTS_TEST_FUNC_EXIT();
}

void init_detailthreshold_ft8607(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_RawDataTest(ini);
    OnInit_DThreshold_CBTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft8607(void)
{

    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(CODE_FT8607_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////Short Test
    if ( ft8607_item.short_test == 1) {
        fts_set_testitem(CODE_FT8607_SHORT_CIRCUIT_TEST);
    }

    //////////////////////////////////////////////////Open Test
    if ( ft8607_item.open_test == 1) {
        fts_set_testitem(CODE_FT8607_OPEN_TEST);
    }
    //////////////////////////////////////////////////CB_TEST
    if ( ft8607_item.cb_test == 1) {
        fts_set_testitem(CODE_FT8607_CB_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( ft8607_item.lcd_noise_test == 1) {
        fts_set_testitem(CODE_FT8607_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( ft8607_item.rawdata_test == 1) {
        fts_set_testitem(CODE_FT8607_RAWDATA_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft8607,
    .init_basicthreshold = init_basicthreshold_ft8607,
    .init_detailthreshold = init_detailthreshold_ft8607,
    .set_testitem_sequence  = set_testitem_sequence_ft8607,
    .start_test = start_test_ft8607,
};
#endif
