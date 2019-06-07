/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Focaltech_test_ft8606.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: test item for FT8606
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include "../focaltech_test.h"

#if (FTS_CHIP_TEST_TYPE ==FT8606_TEST)

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
/////////////////////////////////////////////////Reg 8606
#define DEVIDE_MODE_ADDR    0x00
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define FT_8606_LEFT_KEY_REG    0X1E
#define FT_8606_RIGHT_KEY_REG   0X1F
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
#define REG_CLB             0x04


/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
struct ft8606_test_item {
    bool rawdata_test;
    bool cb_test;
    bool short_test;
    bool lcd_noise_test;
};
struct ft8606_basic_threshold {
    int rawdata_test_min;
    int rawdata_test_max;
    int cb_test_min;
    int cb_test_max;
    int short_test_max;
    int short_test_k2_value;
    bool short_test_tip;
    int lcd_noise_test_frame;
    int lcd_noise_test_max_screen;
    int lcd_noise_test_max_frame;
    int lcd_noise_coefficient;
};
enum test_item_ft8606 {
    CODE_FT8606_ENTER_FACTORY_MODE = 0,//All IC are required to test items
    CODE_FT8606_RAWDATA_TEST = 7,
    CODE_FT8606_CB_TEST = 12,
    CODE_FT8606_SHORT_CIRCUIT_TEST = 14,
    CODE_FT8606_LCD_NOISE_TEST = 15,

};

/*******************************************************************************
* Static variables
*******************************************************************************/

static int raw_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static int cb_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static u8 temp_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0}; //One-dimensional
static int temp_rawdata[TX_NUM_MAX *RX_NUM_MAX] = {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
struct ft8606_test_item ft8606_item;
struct ft8606_basic_threshold ft8606_basic_thr;

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
static int start_scan(void);
static unsigned char read_raw_data(unsigned char freq, unsigned char line_num, int byte_num, int *rev_buffer);
static unsigned char get_tx_rx_cb(unsigned short start_node, unsigned short read_num, unsigned char *read_buffer);
static unsigned char get_raw_data(void);
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, unsigned char row, unsigned char col, unsigned char item_count);
static unsigned char chip_clb(unsigned char *clb_result);
static int ft8606_get_key_num(void);
static void ft8606_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static bool ft8606_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static int ft8606_enter_factory_mode(void);
static int ft8606_rawdata_test(bool *test_result);
static int ft8606_cb_test(bool *test_result);

/************************************************************************
* Name: start_test_ft8606
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
bool start_test_ft8606(void)
{

    bool test_result = true, temp_result = 1;
    int ret;
    int item_count = 0;


    //--------------1. Init part
    if (init_test() < 0) {
        FTS_TEST_SAVE_INFO("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == test_data.test_num)
        test_result = false;

    ////////Testing process, the order of the test_data.test_item structure of the test items
    for (item_count = 0; item_count < test_data.test_num; item_count++) {
        test_data.test_item_code = test_data.test_item[item_count].itemcode;

        ///////////////////////////////////////////////////////FT8606_ENTER_FACTORY_MODE
        if (CODE_FT8606_ENTER_FACTORY_MODE == test_data.test_item[item_count].itemcode
           ) {
            ret = ft8606_enter_factory_mode();
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8606_RAWDATA_TEST

        if (CODE_FT8606_RAWDATA_TEST == test_data.test_item[item_count].itemcode
           ) {
            ret = ft8606_rawdata_test(&temp_result);
            if (ERROR_CODE_OK != ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8606_CB_TEST

        if (CODE_FT8606_CB_TEST == test_data.test_item[item_count].itemcode
           ) {
            ret = ft8606_cb_test(&temp_result);
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
* Name: ft8606_enter_factory_mode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8606_enter_factory_mode(void)
{
    int ret = 0;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("enter factory mode fail, can't get tx/rx num");
        return ret;
    }
    ret = ft8606_get_key_num();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("get key num fail");
        return ret;
    }

    return ret;
}

/************************************************************************
* Name: ft8606_rawdata_test
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8606_rawdata_test(bool *test_result)
{
    int ret;
    bool tmp_result = true;
    int i = 0;


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
        FTS_TEST_SAVE_INFO("Failed to get Raw Data!! Error Code: %d",  ret);
        return ret;
    }
    //----------------------------------------------------------Show RawData
    ft8606_show_data(raw_data, true);


    //----------------------------------------------------------To Determine RawData if in Range or not
    tmp_result = ft8606_compare_detailthreshold_data(raw_data, test_data.incell_detail_thr.RawDataTest_Min, test_data.incell_detail_thr.RawDataTest_Max, true);

    //////////////////////////////Save Test Data
    save_test_data(raw_data, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);
    //----------------------------------------------------------Return Result

    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========RawData Test is OK!");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========RawData Test is NG!");
    }
    return ret;
}

/************************************************************************
* Name: ft8606_cb_test
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int ft8606_cb_test(bool *test_result)
{
    bool tmp_result = true;
    int ret = ERROR_CODE_OK;
    int row = 0;
    int col = 0;
    int i = 0;
    unsigned char clb_result = 0;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: --------  CB Test\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        tmp_result = false;
        goto TEST_ERR;
    }

    for ( i = 0; i < 10; i++) {
        ret = chip_clb( &clb_result );
        sys_delay(50);
        if ( 0 != clb_result) {
            break;
        }
    }
    if ( false == clb_result) {
        FTS_TEST_SAVE_INFO( "\r\nReCalib Failed\r\n" );
        tmp_result = false;
        goto TEST_ERR;
    }


    ret = get_tx_rx_cb( 0, (short)(test_data.screen_param.tx_num * test_data.screen_param.rx_num + test_data.screen_param.key_num_total), temp_data );
    if ( ERROR_CODE_OK != ret ) {
        tmp_result = false;
        FTS_TEST_SAVE_INFO("Failed to get CB value...");
        goto TEST_ERR;
    }

    memset(cb_data, 0, sizeof(cb_data));
    ///VA area
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

    ft8606_show_data(cb_data, true);


    //------------------------------------------------Analysis
    tmp_result = ft8606_compare_detailthreshold_data(cb_data, test_data.incell_detail_thr.CBTest_Min, test_data.incell_detail_thr.CBTest_Max, true);


    //////////////////////////////Save Test Data
    save_test_data(cb_data, 0, test_data.screen_param.tx_num + 1, test_data.screen_param.rx_num, 1);

    if (tmp_result) {
        * test_result = true;
        FTS_TEST_SAVE_INFO("\n==========CB Test is OK!");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_INFO("\n==========CB Test is NG!");
    }

    return ret;

TEST_ERR:

    * test_result = false;
    FTS_TEST_SAVE_INFO("\n==========CB Test is NG!");
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
    unsigned char reg_val = 0x00;
    unsigned char times = 0;
    const unsigned char max_times = 250;
    int ret = ERROR_CODE_COMM_ERROR;


    ret = read_reg(DEVIDE_MODE_ADDR, &reg_val);
    if (ret == ERROR_CODE_OK) {
        reg_val |= 0x80;     //Top bit position 1, start scan
        ret = write_reg(DEVIDE_MODE_ADDR, reg_val);
        if (ret == ERROR_CODE_OK) {
            while (times++ < max_times) {     //Wait for the scan to complete
                sys_delay(16);   //16ms
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
static unsigned char read_raw_data(unsigned char freq, unsigned char line_num, int byte_num, int *rev_buffer)
{
    int ret = ERROR_CODE_COMM_ERROR;
    unsigned char i2c_w_buffer[3] = {0};
    unsigned char read_data[byte_num];
    //unsigned char pReadDataTmp[byte_num*2];
    int i, read_num;
    unsigned short bytes_per_time = 0;

    read_num = byte_num / BYTES_PER_TIME;

    if (0 != (byte_num % BYTES_PER_TIME)) read_num++;

    if (byte_num <= BYTES_PER_TIME) {
        bytes_per_time = byte_num;
    } else {
        bytes_per_time = BYTES_PER_TIME;
    }

    ret = write_reg(REG_LINE_NUM, line_num);//Set row addr;


    //***********************************************************Read raw data in test mode1
    i2c_w_buffer[0] = REG_RawBuf0;   //set begin address
    if (ret == ERROR_CODE_OK) {
        sys_delay(10);
        ret = fts_i2c_read_write(i2c_w_buffer, 1, read_data, bytes_per_time);
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
static unsigned char get_tx_rx_cb(unsigned short start_node, unsigned short read_num, unsigned char *read_buffer)
{
    int ret = ERROR_CODE_OK;
    unsigned short return_num = 0;//Number to return in every time
    unsigned short total_return_num = 0;//Total return number
    unsigned char w_buffer[4];
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
    int row, col;

    //--------------------------------------------Start Scanning
    ret = start_scan();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("Failed to Scan ...");
        return ret;
    }

    //--------------------------------------------Read RawData for Channel Area
    memset(raw_data, 0, sizeof(raw_data));
    memset(temp_rawdata, 0, sizeof(temp_rawdata));
    ret = read_raw_data(0, 0xAD, test_data.screen_param.tx_num * test_data.screen_param.rx_num * 2, temp_rawdata);
    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("Failed to Get RawData");
        return ret;
    }

    for (row = 0; row < test_data.screen_param.tx_num; ++row) {
        for (col = 0; col < test_data.screen_param.rx_num; ++col) {
            raw_data[row][col] = temp_rawdata[row * test_data.screen_param.rx_num + col];
        }
    }

    //--------------------------------------------Read RawData for Key Area
    memset(temp_rawdata, 0, sizeof(temp_rawdata));
    ret = read_raw_data( 0, 0xAE, test_data.screen_param.key_num_total * 2, temp_rawdata );
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("Failed to Get RawData");
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

static int ft8606_get_key_num(void)
{
    int ret = 0;
    int i = 0;
    u8 keyval = 0;

    test_data.screen_param.key_num = 0;
    for (i = 0; i < 3; i++) {
        ret = read_reg( FT_8606_LEFT_KEY_REG, &keyval );
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
        ret = read_reg( FT_8606_RIGHT_KEY_REG, &keyval );
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
* Name: ft8606_show_data
* Brief:  Show ft8606_show_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ft8606_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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
* Name: ft8606_compare_detailthreshold_data
* Brief:  ft8606_compare_detailthreshold_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool ft8606_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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

void init_basicthreshold_ft8606(char *strIniFile)
{

    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// RawdataTest
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "5000", str, strIniFile);
    ft8606_basic_thr.rawdata_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "11000", str, strIniFile);
    ft8606_basic_thr.rawdata_test_max = fts_atoi(str);

    //////////////////////////////////////////////////////////// CBTest
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min", "3", str, strIniFile);
    ft8606_basic_thr.cb_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max", "100", str, strIniFile);
    ft8606_basic_thr.cb_test_max = fts_atoi(str);

    //////////////////////////////////////////////////////////// ShortCircuit
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_CBMax", "120", str, strIniFile);
    ft8606_basic_thr.short_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_K2Value", "150", str, strIniFile);
    ft8606_basic_thr.short_test_k2_value = fts_atoi(str);

    //////////////////////////////////////////////////////////// lcd_noise
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Frame", "50", str, strIniFile);
    ft8606_basic_thr.lcd_noise_test_frame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Max_Screen", "32", str, strIniFile);
    ft8606_basic_thr.lcd_noise_test_max_screen = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Max_Frame", "32", str, strIniFile);
    ft8606_basic_thr.lcd_noise_test_max_frame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCD_NoiseTest_Coefficient", "50", str, strIniFile);
    ft8606_basic_thr.lcd_noise_coefficient = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();

}

void init_testitem_ft8606(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();


    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    ft8606_item.rawdata_test = fts_atoi(str);


    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    ft8606_item.cb_test = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "SHORT_CIRCUIT_TEST", "1", str, strIniFile);
    ft8606_item.short_test = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem", "LCD_NOISE_TEST", "0", str, strIniFile);
    ft8606_item.lcd_noise_test = fts_atoi(str);


    FTS_TEST_FUNC_EXIT();

}

void init_detailthreshold_ft8606(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_RawDataTest(ini);
    OnInit_DThreshold_AllButtonCBTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft8606(void)
{

    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();


    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(CODE_FT8606_ENTER_FACTORY_MODE);


    //////////////////////////////////////////////////Short Test
    if ( ft8606_item.short_test == 1) {

        fts_set_testitem(CODE_FT8606_SHORT_CIRCUIT_TEST);
    }

    //////////////////////////////////////////////////CB_TEST
    if ( ft8606_item.cb_test == 1) {

        fts_set_testitem(CODE_FT8606_CB_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( ft8606_item.lcd_noise_test == 1) {

        fts_set_testitem(CODE_FT8606_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( ft8606_item.rawdata_test == 1) {

        fts_set_testitem(CODE_FT8606_RAWDATA_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft8606,
    .init_basicthreshold = init_basicthreshold_ft8606,
    .init_detailthreshold = init_detailthreshold_ft8606,
    .set_testitem_sequence  = set_testitem_sequence_ft8606,
    .start_test = start_test_ft8606,
};

#endif
