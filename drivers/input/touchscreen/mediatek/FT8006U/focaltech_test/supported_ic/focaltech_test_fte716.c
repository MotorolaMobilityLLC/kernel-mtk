/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: focaltech_test_ftE716.c
*
* Author: Software Development
*
* Created: 2016-08-01
*
* Abstract: test item for FTE716
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include "../focaltech_test.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#if (FTS_CHIP_TEST_TYPE ==FTE716_TEST)

/////////////////////////////////////////////////Reg FTE716
#define DEVIDE_MODE_ADDR                0x00
#define REG_LINE_NUM                    0x01
#define REG_TX_NUM                      0x02
#define REG_RX_NUM                      0x03
#define FTE716_LEFT_KEY_REG             0X1E
#define FTE716_RIGHT_KEY_REG            0X1F
#define REG_CbAddrH                     0x18
#define REG_CbAddrL                     0x19
#define REG_OrderAddrH                  0x1A
#define REG_OrderAddrL                  0x1B
#define REG_RawBuf0                     0x6A
#define REG_RawBuf1                     0x6B
#define REG_OrderBuf0                   0x6C
#define REG_CbBuf0                      0x6E
#define REG_K1Delay                     0x31
#define REG_K2Delay                     0x32
#define REG_SCChannelCf                 0x34
#define REG_CLB                          0x04

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

struct fte716_test_item {
    bool rawdata_test;
    bool cb_test;
    bool short_test;
    bool lcd_noise_test;
    bool open_test;
};
struct fte716_basic_threshold {
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
    u8 lcd_noise_test_noise_mode;
    int open_test_cb_min;
};

enum test_item_fte716 {
    CODE_FTE716_ENTER_FACTORY_MODE = 0,//All IC are required to test items
    CODE_FTE716_RAWDATA_TEST = 7,
    CODE_FTE716_CB_TEST = 12,
    CODE_FTE716_SHORT_CIRCUIT_TEST = 14,
    CODE_FTE716_OPEN_TEST = 15,
    CODE_FTE716_LCD_NOISE_TEST = 19,
};

/*******************************************************************************
* Static variables
*******************************************************************************/
static int raw_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static int cb_data[TX_NUM_MAX][RX_NUM_MAX] = {{0, 0}};
static u8 tmp_data[TX_NUM_MAX *RX_NUM_MAX * 2] = {0}; //One-dimensional
static int tmp_rawdata[TX_NUM_MAX *RX_NUM_MAX] = {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
struct fte716_basic_threshold fte716_basic_thr;
struct fte716_test_item fte716_item;
/*******************************************************************************
* Static function prototypes
*******************************************************************************/

/////////////////////////////////////////////////////////////
static int start_scan(void);
static int read_raw_data(u8 freq, u8 line_num, int byte_num, int *rev_buffer);
static int get_tx_rx_cb(unsigned short start_node, unsigned short read_num, u8 *read_buffer);
static int get_raw_data(void);
static int chip_clb(u8 *clb_result);
static int fte716_get_key_num(void);
static bool fte716_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static void fte716_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key);
static void save_test_data(int data[TX_NUM_MAX][RX_NUM_MAX], int array_index, u8 row, u8 col, u8 item_count);
static int fte716_enter_factory_mode(void);
static int fte716_raw_data_test(bool *test_result);
static int fte716_cb_test(bool *test_result);

/************************************************************************
* Name: start_test_fte716
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/

bool start_test_fte716(void)
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

    ////////Testing process, the order of the test_item structure of the test items
    for (item_count = 0; item_count < test_data.test_num; item_count++) {
        test_data.test_item_code = test_data.test_item[item_count].itemcode;

        ///////////////////////////////////////////////////////FTE716_ENTER_FACTORY_MODE
        if (CODE_FTE716_ENTER_FACTORY_MODE == test_data.test_item[item_count].itemcode) {
            ret = fte716_enter_factory_mode();
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FTE716_RAWDATA_TEST
        if (CODE_FTE716_RAWDATA_TEST == test_data.test_item[item_count].itemcode) {
            ret = fte716_raw_data_test(&tmp_result);
            if (ERROR_CODE_OK != ret || (!tmp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FTE716_CB_TEST
        if (CODE_FTE716_CB_TEST == test_data.test_item[item_count].itemcode) {
            ret = fte716_cb_test(&tmp_result);
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
* Name: fte716_enter_factory_mode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int fte716_enter_factory_mode(void)
{

    int ret = 0;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("enter factory mode fail, can't get tx/rx num");
        return ret;
    }
    ret = fte716_get_key_num();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("get key num fail");
        return ret;
    }
    return ret;
}

/************************************************************************
* Name: fte716_raw_data_test
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: test_result
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int fte716_raw_data_test(bool *test_result)
{
    int ret = ERROR_CODE_OK;
    bool tmp_result = true;
    int i = 0;
    bool include_key = false;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: -------- Raw Data Test\n");

    include_key = fte716_basic_thr.rawdata_test_vk_check;

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        return ret;
    }

    //----------------------------------------------------------Read RawData
    for (i = 0 ; i < 3; i++) { //Lost 3 Frames, In order to obtain stable data
        ret = get_raw_data();
    }

    if ( ERROR_CODE_OK != ret ) {
        FTS_TEST_SAVE_INFO("\nFailed to get Raw Data!! Error Code: %d\n", ret);
        return ret;
    }
    //----------------------------------------------------------Show RawData
    fte716_show_data(raw_data, include_key);

    //----------------------------------------------------------To Determine RawData if in Range or not
    tmp_result = fte716_compare_detailthreshold_data(raw_data, test_data.incell_detail_thr.RawDataTest_Min, test_data.incell_detail_thr.RawDataTest_Max, include_key);
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
* Name: fte716_cb_test
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: test_result, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int fte716_cb_test(bool *test_result)
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

    include_key = fte716_basic_thr.cb_test_vk_check;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: --------  CB Test\n");

    ret = enter_factory_mode();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("\n Failed to Enter factory mode. Error Code: %d", ret);
        tmp_result = false;
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
        FTS_TEST_SAVE_INFO("\r\n//=========  Read Reg 0x0B Failed!");
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
    fte716_show_data(cb_data, include_key);

    //----------------------------------------------------------To Determine Cb if in Range or not
    tmp_result = fte716_compare_detailthreshold_data(cb_data, test_data.incell_detail_thr.CBTest_Min, test_data.incell_detail_thr.CBTest_Max, include_key);

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
    FTS_TEST_SAVE_INFO("\n\n/==========CB Test is NG!");

    return ret;
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
* Name: read_rawdata(Same function name as FT_MultipleTest)
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
    //u8 read_dataTmp[byte_num*2];
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
            //if(rev_buffer[i] & 0x8000)//The sign bit
            //{
            //  rev_buffer[i] -= 0xffff + 1;
            //}
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
    }
    return ret;
}


/************************************************************************
* Name: get_rawdata
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
    //FTS_TEST_DBG("Start Scan ...");
    ret = start_scan();
    if (ERROR_CODE_OK != ret) {
        FTS_TEST_SAVE_INFO("Failed to Scan \n");
        return ret;
    }


    //--------------------------------------------Read RawData for Channel Area
    //FTS_TEST_DBG("Read RawData...");
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

static int fte716_get_key_num(void)
{
    int ret = 0;
    int i = 0;
    u8 keyval = 0;

    test_data.screen_param.key_num = 0;
    for (i = 0; i < 3; i++) {
        ret = read_reg( FTE716_LEFT_KEY_REG, &keyval );
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
        ret = read_reg( FTE716_RIGHT_KEY_REG, &keyval );
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
* Name: fte716_show_data
* Brief:  Show ft8607_show_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void fte716_show_data(int data[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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
* Name: fte716_compare_detailthreshold_data
* Brief:  fte716_compare_detailthreshold_data
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static bool fte716_compare_detailthreshold_data(int data[TX_NUM_MAX][RX_NUM_MAX], int data_min[TX_NUM_MAX][RX_NUM_MAX], int data_max[TX_NUM_MAX][RX_NUM_MAX], bool include_key)
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

void init_testitem_fte716(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    /////////////////////////////////// RAWDATA_TEST
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    fte716_item.rawdata_test = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    fte716_item.cb_test = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "SHORT_CIRCUIT_TEST", "1", str, strIniFile);
    fte716_item.short_test = fts_atoi(str);

    /////////////////////////////////// OPEN_TEST
    GetPrivateProfileString("TestItem", "OPEN_TEST", "0", str, strIniFile);
    fte716_item.open_test = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem", "lcd_noise_test", "0", str, strIniFile);
    fte716_item.lcd_noise_test = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_basicthreshold_fte716(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();
    //////////////////////////////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VA_Check", "1", str, strIniFile);
    fte716_basic_thr.rawdata_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "5000", str, strIniFile);
    fte716_basic_thr.rawdata_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "11000", str, strIniFile);
    fte716_basic_thr.rawdata_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VKey_Check", "1", str, strIniFile);
    fte716_basic_thr.rawdata_test_vk_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min_VKey", "5000", str, strIniFile);
    fte716_basic_thr.rawdata_test_min_vk = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max_VKey", "11000", str, strIniFile);
    fte716_basic_thr.rawdata_test_max_vk = fts_atoi(str);
    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold", "CBTest_VA_Check", "1", str, strIniFile);
    fte716_basic_thr.cb_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min", "3", str, strIniFile);
    fte716_basic_thr.cb_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max", "100", str, strIniFile);
    fte716_basic_thr.cb_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_VKey_Check", "1", str, strIniFile);
    fte716_basic_thr.cb_test_vk_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min_Vkey", "3", str, strIniFile);
    fte716_basic_thr.cb_test_min_vk = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max_Vkey", "100", str, strIniFile);
    fte716_basic_thr.cb_test_max_vk = fts_atoi(str);


    //////////////////////////////////////////////////////////// Short Circuit Test
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_ResMin", "1200", str, strIniFile);
    fte716_basic_thr.short_res_min = fts_atoi(str);


    ////////////////////////////////////////////////////////////LCDNoiseTest
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_FrameNum", "200", str, strIniFile);
    fte716_basic_thr.lcd_noise_test_frame_num = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_Coefficient", "60", str, strIniFile);
    fte716_basic_thr.lcd_noise_test_coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_NoiseMode", "0", str, strIniFile);
    fte716_basic_thr.lcd_noise_test_noise_mode = fts_atoi(str);


    ////////////////////////////////////////////////////////////open test
    GetPrivateProfileString("Basic_Threshold", "OpenTest_CBMin", "100", str, strIniFile);
    fte716_basic_thr.open_test_cb_min = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();

}

void init_detailthreshold_fte716(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_CBTest(ini);
    OnThreshold_VkAndVaRawDataSeparateTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_fte716(void)
{
    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(CODE_FTE716_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CB_TEST
    if ( fte716_item.cb_test == 1) {
        fts_set_testitem(CODE_FTE716_CB_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( fte716_item.rawdata_test == 1) {
        fts_set_testitem(CODE_FTE716_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////OPEN_TEST
    if ( fte716_item.open_test == 1) {
        fts_set_testitem(CODE_FTE716_OPEN_TEST);
    }

    //////////////////////////////////////////////////SHORT_CIRCUIT_TEST
    if ( fte716_item.short_test == 1) {
        fts_set_testitem(CODE_FTE716_SHORT_CIRCUIT_TEST) ;
    }

    FTS_TEST_FUNC_EXIT();

}

struct test_funcs test_func = {
    .init_testitem = init_testitem_fte716,
    .init_basicthreshold = init_basicthreshold_fte716,
    .init_detailthreshold = init_detailthreshold_fte716,
    .set_testitem_sequence  = set_testitem_sequence_fte716,
    .start_test = start_test_fte716,
};
#endif
