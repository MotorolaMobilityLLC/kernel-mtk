/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: focaltech_test_ft8716.c
*
* Author: LiuWeiGuang
*
* Created: 2016-08-01
*
* Abstract: test item for FT8716
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "../focaltech_test.h"

#if (FTS_CHIP_TEST_TYPE == FT8716_TEST)
/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct ft8716_test_item {
    bool rawdata_test;
    bool cb_test;
    bool short_circuit_test;
    bool open_test;
    bool lcd_noise_test;
    bool key_short_test;
};

struct ft8716_basic_threshold {
    bool rawdata_test_va_check;
    int rawdata_test_min;
    int rawdata_test_max;
    bool rawdata_test_vkey_check;
    int rawdata_test_min_vkey;
    int rawdata_test_max_vkey;
    bool cb_test_va_check;
    int cb_test_min;
    int cb_test_max;
    bool cb_test_vkey_check;
    int cb_test_min_vkey;
    int cb_test_max_vkey;
    bool cb_test_vkey_dcheck_check;
    bool short_va_check;
    bool short_vkey_check;
    int short_va_resistor_min;
    int short_vkey_resistor_min;
    int open_test_cb_min;
    bool open_test_k1_check;
    int open_test_k1_threshold;
    bool open_test_k2_check;
    int open_test_k2_threshold;
    int lcd_noise_test_frame;
    int lcd_noise_test_coefficient;
    int lcd_noise_test_coefficient_key;
    u8 lcd_noise_test_mode;
    int keyshort_k1;
    int keyshort_cb_max;
};

enum enumTestItem_FT8716 {
    CODE_FT8716_ENTER_FACTORY_MODE = 0,
    CODE_FT8716_RAWDATA_TEST = 7,
    CODE_FT8716_CB_TEST = 12,
    CODE_FT8716_SHORT_CIRCUIT_TEST = 14,
    CODE_FT8716_OPEN_TEST = 15,
    CODE_FT8716_LCD_NOISE_TEST = 19,
    CODE_FT8716U_KEY_SHORT_TEST = 26,
};

u8 localbitwise;
static void set_keybit_val(u8 val)
{
    localbitwise = val;
}

static bool is_key_autofit(void)
{
    return ((localbitwise & 0x0F) == 1);
}
/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
struct ft8716_basic_threshold ft8716_basicthreshold;
struct ft8716_test_item ft8716_testitem;
/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static void show_data(int *data, bool key_support)
{
    int chx = 0;
    int chy = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    int tmptx = 0;
    u8 key_actual_num = 0;

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;

    FTS_TEST_SAVE_INFO("\nVA Channels: ");
    for (chx = 0; chx < tx_num; chx++) {
        FTS_TEST_SAVE_INFO("\nCh_%02d:  ", chx + 1);
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            FTS_TEST_SAVE_INFO("%5d, ", data[tmptx + chy]);
        }
    }

    if (key_support) {
        FTS_TEST_SAVE_INFO("\nKeys:\n ");
        key_actual_num = ((is_key_autofit()) ? test_data.screen_param.key_num : test_data.screen_param.key_num_total);
        tmptx = tx_num * rx_num;
        for (chy = 0; chy < key_actual_num; chy++) {
            FTS_TEST_SAVE_INFO("%5d, ", data[tmptx + chy]);
        }
    }
}

/************************************************************************
 * Name: save_test_data
 * Brief:  Storage format of test data
 * Input:
 * Output: none
 * Return: none
 ***********************************************************************/
static void save_test_data(int *data, u8 tx_num, u8 rx_num, u8 item_count)
{
    int len = 0;
    int i = 0;
    int j = 0;
    int tmptx = 0;

    //Save  Msg (itemcode is enough, ItemName is not necessary, so set it to "NA".)
    len = sprintf(test_data.tmp_buffer, "NA, %d, %d, %d, %d, %d, ", \
                  test_data.test_item_code, tx_num, rx_num, test_data.start_line, item_count);
    memcpy(test_data.msg_area_line2 + test_data.len_msg_area_line2, test_data.tmp_buffer, len);
    test_data.len_msg_area_line2 += len;

    test_data.start_line += tx_num;
    test_data.test_data_count++;

    //Save Data
    for (i = 0; i < tx_num; i++) {
        tmptx = i * rx_num;
        for (j = 0; j < rx_num; j++) {
            if (j == (rx_num - 1)) //The Last Data of the Row, add "\n"
                len = sprintf(test_data.tmp_buffer, "%d, \n",  data[tmptx + j]);
            else
                len = sprintf(test_data.tmp_buffer, "%d, ", data[tmptx + j]);

            memcpy(test_data.store_data_area + test_data.len_store_data_area, test_data.tmp_buffer, len);
            test_data.len_store_data_area += len;
        }
    }
}

static bool compare_data(int *data, int min, int max, int vk_min, int vk_max, bool key_support)
{
    int chx = 0;
    int chy = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    u8 key_actual_num = 0;
    int tmp_chx = 0;
    int value = 0;
    bool tmp_result = true;

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;

    // VA
    for (chx = 0; chx < tx_num; chx++) {
        tmp_chx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue;

            value = data[tmp_chx + chy];
            if (value < min || value > max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,%d), Get_value=%d, Set_Range=(%d,%d)\n", \
                                   chx + 1, chy + 1, value, min, max);
            }
        }
    }

    // Key
    if (key_support) {
        if (is_key_autofit())
            key_actual_num = test_data.screen_param.key_num;
        else
            key_actual_num = test_data.screen_param.key_num_total;
        chx = tx_num;
        tmp_chx = chx * rx_num;
        for ( chy = 0; chy < key_actual_num; chy++ ) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue;

            value = data[tmp_chx + chy];
            if (value < vk_min || value > vk_max) {
                tmp_result = false;
                FTS_TEST_SAVE_INFO("test failure. Node=(%d,%d), Get_value=%d, Set_Range=(%d,%d)\n", \
                                   chx + 1, chy + 1, value, vk_min, vk_max);
            }
        }
    }

    return tmp_result;
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

int get_key_num(void)
{
    int ret = 0;
    int i = 0;
    u8 keyval = 0;

    test_data.screen_param.key_num = 0;
    for (i = 0; i < 3; i++) {
        ret = read_reg( FACTORY_REG_LEFT_KEY, &keyval );
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
        ret = read_reg( FACTORY_REG_RIGHT_KEY, &keyval );
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
* Name: wait_state_update
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static int wait_state_update(void)
{
    int ret = 0;
    int times = 0;
    int max_times = 100;
    u8 state = 0x00;

    while (times++) {
        sys_delay(16);
        //Wait register status update.
        ret = read_reg(FACTORY_REG_PARAM_UPDATE_STATE, &state);
        if ((0 == ret) && (0x00 == state))
            break;
    }

    if (times >= max_times) {
        FTS_TEST_SAVE_INFO("\r\n//=========Wait State Update Failed!");
        return -EIO;
    }

    return 0;
}

/************************************************************************
* Name: chip_clb
* Brief:
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
static int chip_clb(void)
{
    int ret = 0;
    u8 reg_data = 0;
    int times = 0;
    int max_times = 20;

    /* start clb */
    ret = write_reg(FACTORY_REG_CLB, 0x04);
    if (ret) {
        FTS_TEST_SAVE_INFO("write start clb fail");
        return ret;
    }

    sys_delay(100);
    while (times++ < max_times) {
        ret = read_reg(0x04, &reg_data);
        if (0 == ret) {
            if (0x02 == reg_data) {
                /* clb ok */
                break;
            }
        }

        sys_delay(16);
    }

    if (times >= max_times) {
        FTS_TEST_SAVE_INFO("chip clb timeout");
        return -EIO;
    }

    return 0;
}

/************************************************************************
* Name: start_scan
* Brief:
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
static int start_scan(void)
{
    int ret = 0;
    u8 val = 0x00;
    int times = 0;

    ret = read_reg(DEVIDE_MODE_ADDR, &val);
    if (ret) {
        FTS_TEST_SAVE_INFO("read device mode fail");
        return ret;
    }

    /* Top bit position 1, start scan */
    val |= 0x80;
    ret = write_reg(DEVIDE_MODE_ADDR, val);
    if (ret) {
        FTS_TEST_SAVE_INFO("write device mode fail");
        return ret;
    }

    while (times++ < START_SCAN_RETRIES_INCELL) {
        /* Wait for the scan to complete */
        sys_delay(START_SCAN_RETRIES_DELAY_INCELL);

        ret = read_reg(DEVIDE_MODE_ADDR, &val);
        if (0 == ret) {
            if ((val >> 7) == 0)
                break;
        }
    }

    if (times >= START_SCAN_RETRIES_INCELL) {
        FTS_TEST_SAVE_INFO("start scan timeout");
        return -EIO;
    }

    return 0;
}

static int read_rawdata(u8 line_addr, int byte_num, int *rawbuf)
{
    int ret = 0;
    int i = 0;
    u8 reg_addr = 0;
    u8 *data = NULL;
    int read_num = 0;
    int packet_num = 0;
    int packet_remainder = 0;
    int offset = 0;

    data = (u8 *)fts_malloc(byte_num * sizeof(u8));
    if (NULL == data) {
        FTS_TEST_SAVE_INFO("rawdata buffer malloc fail");
        return -ENOMEM;
    }

    /* set rawdata line addr */
    ret = write_reg(FACTORY_REG_LINE_ADDR, line_addr);
    if (ret) {
        FTS_TEST_SAVE_INFO("write rawdata line addr fail");
        goto READ_RAWDATA_ERR;
    }
    sys_delay(10);

    /* read rawdata buffer */
    FTS_TEST_INFO("rawdata len:%d", byte_num);
    packet_num = byte_num / BYTES_PER_TIME;
    packet_remainder = byte_num % BYTES_PER_TIME;
    if (packet_remainder)
        packet_num++;

    if (byte_num < BYTES_PER_TIME) {
        read_num = byte_num;
    } else {
        read_num = BYTES_PER_TIME;
    }
    FTS_TEST_INFO("packet num:%d, remainder:%d", packet_num, packet_remainder);


    reg_addr = FACTORY_REG_RAWDATA_ADDR;
    ret = fts_test_i2c_read(&reg_addr, 1, data, read_num);
    if (ret) {
        FTS_TEST_SAVE_INFO("read rawdata fail");
        goto READ_RAWDATA_ERR;
    }

    for (i = 1; i < packet_num; i++) {
        offset = read_num * i;
        if ((i == (packet_num - 1)) && packet_remainder) {
            read_num = packet_remainder;
        }

        ret = fts_test_i2c_read(NULL, 0, data + offset, read_num);
        if (ret) {
            FTS_TEST_SAVE_INFO("read much rawdata fail");
            goto READ_RAWDATA_ERR;
        }
    }

    for (i = 0; i < byte_num; i = i + 2) {
        rawbuf[i >> 1] = (int)(((int)(data[i]) << 8) + data[i + 1]);
    }

    if (data) {
        fts_free(data);
    }
    return 0;

READ_RAWDATA_ERR:
    if (data) {
        fts_free(data);
    }
    return ret;
}

/************************************************************************
* Name: get_rawdata
* Brief:  Get Raw Data of VA area and Key area
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
static int get_rawdata(int *rawbuf)
{
    int ret = 0;
    int tx_num = 0;
    int rx_num = 0;
    int key_num = 0;

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    key_num = test_data.screen_param.key_num_total;
    FTS_TEST_INFO("tx num:%d, rx num%d, key num%d\n", tx_num, rx_num, key_num);
    if (((tx_num + 1) > TX_NUM_MAX) || (rx_num > RX_NUM_MAX)) {
        FTS_TEST_SAVE_INFO("tx/rx/key num fail");
        return -EINVAL;
    }

    /* Enter Factory Mode */
    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Enter Factory Mode\n");
        return ret;
    }

    /* write data select */
    ret = write_reg(FACTORY_REG_DATA_SELECT, 0x00);
    if (ret) {
        FTS_TEST_SAVE_INFO("write data select fail");
        return ret;
    }

    /* Start Scanning */
    ret = start_scan();
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Scan\n");
        return ret;
    }

    memset(rawbuf, 0, (tx_num + 1) * rx_num);
    /* Read RawData for Channel Area */
    ret = read_rawdata(0xAD, tx_num * rx_num * 2, rawbuf);
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Get VA RawData\n");
        return ret;
    }

    //--------------------------------------------Read RawData for Key Area
    ret = read_rawdata(0xAE, key_num * 2, rawbuf + tx_num * rx_num);
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Get KEY RawData\n");
        return ret;
    }

    return 0;
}

/************************************************************************
* Name: get_cb
* Brief:  get CB of Tx/Rx
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
static int get_cb(u16 saddr, int byte_num, u8 *cb_buf)
{
    int ret = 0;
    int i = 0;
    u8 cb_addr = 0;
    u8 addr_h = 0;
    u8 addr_l = 0;
    int read_num = 0;
    int packet_num = 0;
    int packet_remainder = 0;
    int offset = 0;
    int addr = 0;

    packet_num = byte_num / BYTES_PER_TIME;
    packet_remainder = byte_num % BYTES_PER_TIME;
    if (packet_remainder)
        packet_num++;
    read_num = BYTES_PER_TIME;

    cb_addr = FACTORY_REG_CB_ADDR;
    for (i = 0; i < packet_num; i++) {
        offset = read_num * i;
        addr = saddr + offset;
        addr_h = (addr >> 8) & 0x0F;
        addr_l = addr & 0x0F;
        if ((i == (packet_num - 1)) && packet_remainder) {
            read_num = packet_remainder;
        }

        ret = write_reg(FACTORY_REG_CB_ADDR_H, addr_h);
        if (ret) {
            FTS_TEST_SAVE_INFO("write cb addr high fail");
            return ret;
        }
        ret = write_reg(FACTORY_REG_CB_ADDR_L, addr_l);
        if (ret) {
            FTS_TEST_SAVE_INFO("write cb addr low fail");
            return ret;
        }

        ret = fts_test_i2c_read(&cb_addr, 1, cb_buf + offset, read_num);
        if (ret) {
            FTS_TEST_SAVE_INFO("read cb fail");
            return ret;
        }
    }

    return 0;
}

/************************************************************************
* Name: weakshort_get_adc_data
* Brief:  Get Adc Data
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
static int weakshort_get_adc_data(u8 ch_num, int byte_num, int *adc_buf)
{
    int ret = 0;
    u8 short_state = 0;
    int times = 0;
    int max_times = 50;
    int i = 0;
    int read_num = 0;
    int packet_num = 0;
    int packet_remainder = 0;
    int offset = 0;
    u8 short_addr = 0;
    u8 *tmpbuf;

    FTS_TEST_FUNC_ENTER();

    tmpbuf = fts_malloc(byte_num * sizeof(u8));
    if (NULL == tmpbuf) {
        FTS_TEST_SAVE_INFO("temp buffer malloc fail");
        return -ENOMEM;
    }
    memset(tmpbuf, 0, byte_num);

    /* Start ADC sample */
    ret = write_reg(FACTORY_REG_SHORT_TEST_EN, 0x01);
    if (ret) {
        FTS_TEST_SAVE_INFO("start short test fail");
        goto ADC_ERROR;
    }
    sys_delay(ch_num * 16);

    for (times = 0; times < max_times; times++) {
        ret = read_reg(FACTORY_REG_SHORT_TEST_STATE, &short_state);
        if ((0 == ret) && (0x00 == short_state))
            break;

        sys_delay(100);
    }
    if (times >= max_times) {
        FTS_TEST_SAVE_INFO("short test timeout, ADC data not OK.");
        ret = -EIO;
        goto ADC_ERROR;
    }

    /* read adc data */
    packet_num = byte_num / BYTES_PER_TIME;
    packet_remainder = byte_num % BYTES_PER_TIME;
    if (packet_remainder)
        packet_num++;

    if (byte_num <= BYTES_PER_TIME) {
        read_num = byte_num;
    } else {
        read_num = BYTES_PER_TIME;
    }

    /* read rawdata buffer */
    short_addr = FACTORY_REG_SHORT_ADDR;
    ret = fts_test_i2c_read(&short_addr, 1, tmpbuf, read_num);
    if (ret) {
        FTS_TEST_SAVE_INFO("read short adc data fail");
        return ret;
    }
    for (i = 1; i < packet_num; i++) {
        offset = read_num * i;
        if ((i == (packet_num - 1)) && packet_remainder) {
            read_num = packet_remainder;
        }

        ret = fts_test_i2c_read(NULL, 0, tmpbuf + offset, read_num);
        if (ret) {
            FTS_TEST_SAVE_INFO("read much short adc data fail");
            goto ADC_ERROR;
        }
    }

    for (i = 0; i < byte_num; i = i + 2) {
        adc_buf[i >> 1] = (int)(((int)tmpbuf[i] << 8) + tmpbuf[i + 1]);
    }

    if (tmpbuf) {
        fts_free(tmpbuf);
    }
    return 0;

ADC_ERROR:
    if (tmpbuf) {
        fts_free(tmpbuf);
    }
    FTS_TEST_FUNC_EXIT();
    return ret;
}


/************************************************************************
* Name: ft8716_enter_factory_mode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: return 0 if succuss
***********************************************************************/
int ft8716_enter_factory_mode(void)
{
    int ret = 0;
    u8 keyFit = 0;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("enter factory mode fail, can't get tx/rx num");
        return ret;
    }

    ret = get_key_num();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("get key num fail");
        return ret;
    }

    ret = read_reg(0xFC, &keyFit );
    if (0 == ret)
        set_keybit_val(keyFit);

    return ret;
}

/************************************************************************
* Name: ft8716_rawdata_test
* Brief:
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
int ft8716_rawdata_test(bool *test_result)
{
    int ret = 0;
    bool temp_result = true;
    int rawdata_min;
    int rawdata_max;
    int value = 0;
    int i = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    int chx = 0;
    int chy = 0;
    int tmptx = 0;
    bool key_support = false;
    u8 key_actual_num = 0;
    int *rawbuf = NULL;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: -------- Raw Data Test\n");

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    key_support = ft8716_basicthreshold.rawdata_test_vkey_check;

    memset(test_data.buffer, 0, ((test_data.screen_param.tx_num + 1) * test_data.screen_param.rx_num) * sizeof(int));
    rawbuf = test_data.buffer;

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Enter Factory Mode\n");
        goto RAWDATA_TEST_ERR;
    }
    //----------------------------------------------------------Read RawData
    /* get 3rd Frames, In order to obtain stable data */
    for (i = 0 ; i < 3; i++) {
        ret = get_rawdata(rawbuf);
    }
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to get Raw Data,ret=%d", ret);
        goto RAWDATA_TEST_ERR;
    }

    //----------------------------------------------------------Show RawData
    show_data(rawbuf, key_support);

    //----------------------------------------------------------To Determine RawData if in Range or not
    //   VA
    for (chx  = 0; chx < tx_num; chx++) {
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue; //Invalid Node
            rawdata_min = test_data.incell_detail_thr.RawDataTest_Min[chx][chy];
            rawdata_max = test_data.incell_detail_thr.RawDataTest_Max[chx][chy];
            value = rawbuf[tmptx + chy];
            if (value < rawdata_min || value > rawdata_max) {
                temp_result = false;
                FTS_TEST_SAVE_INFO("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  \
                                   chx + 1, chy + 1, value, rawdata_min, rawdata_max);
            }
        }
    }

    // Key
    if (key_support) {
        tmptx = tx_num * rx_num;
        for (chy = 0; chy < key_actual_num; chy++) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue; //Invalid Node
            rawdata_min = test_data.incell_detail_thr.RawDataTest_Min[chx][chy];
            rawdata_max = test_data.incell_detail_thr.RawDataTest_Max[chx][chy];
            value = rawbuf[tmptx + chy];
            if (value < rawdata_min || value > rawdata_max) {
                temp_result = false;
                FTS_TEST_SAVE_INFO("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  \
                                   chx + 1, chy + 1, value, rawdata_min, rawdata_max);
            }
        }
    }

    //////////////////////////////Save Test Data
    save_test_data(rawbuf, tx_num + 1, rx_num, 1);
    //----------------------------------------------------------Return Result
    FTS_TEST_SAVE_INFO("\n==========RawData Test is %s. \n\n", (temp_result ? "OK" : "NG"));

    if (temp_result) {
        *test_result = true;
    } else {
        *test_result = false;
    }

    return 0;

RAWDATA_TEST_ERR:
    *test_result = false;
    FTS_TEST_SAVE_INFO("\n\n/==========RAWDATA Test is NG!");
    return ret;
}

/************************************************************************
* Name: ft8716_cb_test
* Brief:
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
int ft8716_cb_test(bool *test_result)
{
    bool temp_result = true;
    unsigned char ret = ERROR_CODE_OK;
    int chx = 0;
    int chy = 0;
    int tmptx = 0;
    int cb_max = 0;
    int cb_min = 0;
    int cb_value = 0;
    bool key_support = false;
    unsigned char key_actual_num = 0;
    u8 key_cb_width = 0;
    int key_num = 0;
    int cb_byte_num = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    int *cb_buf = NULL;
    u8 *tmpbuf = NULL;

    FTS_TEST_SAVE_INFO("\n\n==============================Test Item: --------  CB Test\n");

    memset(test_data.buffer, 0, ((test_data.screen_param.tx_num + 1) * test_data.screen_param.rx_num) * sizeof(int));
    cb_buf = test_data.buffer;

    key_support = ft8716_basicthreshold.cb_test_vkey_check;

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Enter Factory Mode\n");
        return ret;
    }

    ret = chip_clb();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto CB_TEST_ERR;
    }

    ret = read_reg(FACTORY_REG_KEY_CBWIDTH, &key_cb_width);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========  Read Reg 0x0B Failed!");
        goto CB_TEST_ERR;
    }

    if (0 == key_cb_width) {
        key_num = test_data.screen_param.key_num_total;
    } else {
        key_num = test_data.screen_param.key_num_total * 2;
    }

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    cb_byte_num = (int)(tx_num * rx_num  + key_num);

    tmpbuf = fts_malloc(cb_byte_num * sizeof(u8));
    if (NULL == tmpbuf) {
        FTS_TEST_SAVE_INFO("cb memory malloc fail");
        goto CB_TEST_ERR;
    }
    memset(tmpbuf, 0, cb_byte_num);
    memset(cb_buf, 0, cb_byte_num * sizeof(int));
    ret = get_cb(0, cb_byte_num, tmpbuf);
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to get CB value\n");
        goto CB_TEST_ERR;
    }

    ///VA area
    for (chx = 0; chx < tx_num; chx++) {
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            cb_buf[tmptx + chy] = tmpbuf[tmptx + chy];
        }
    }

    tmptx = tx_num * rx_num;
    for (chy = 0; chy < key_num; chy++) {
        if (key_cb_width != 0) {
            cb_buf[tmptx + chy / 2] = (short)((tmpbuf[tmptx + chy] & 0x01 ) << 8) + tmpbuf[tmptx + chy + 1];
            chy++;
        } else {
            cb_buf[tmptx + chy] = tmpbuf[tmptx + chy];
        }
    }
    //------------------------------------------------Show CbData
    show_data(cb_buf, key_support);

    // VA
    for (chx = 0; chx < tx_num; chx++) {
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue;
            cb_min = test_data.incell_detail_thr.CBTest_Min[chx][chy];
            cb_max = test_data.incell_detail_thr.CBTest_Max[chx][chy];
            cb_value = cb_buf[tmptx + chy];
            if ( cb_value < cb_min || cb_value > cb_max) {
                temp_result = false;
                FTS_TEST_SAVE_INFO("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  \
                                   chx + 1, chy + 1, cb_value, cb_min, cb_max);
            }
        }
    }

    // Key
    if (key_support) {
        tmptx = tx_num * rx_num;
        for ( chy = 0; chy < key_actual_num; chy++) {
            if (0 == test_data.incell_detail_thr.InvalidNode[chx][chy])
                continue;
            cb_min = test_data.incell_detail_thr.CBTest_Min[chx][chy];
            cb_max = test_data.incell_detail_thr.CBTest_Max[chx][chy];
            cb_value = cb_buf[tmptx + chy];
            if (cb_value < cb_min || cb_value > cb_max) {
                temp_result = false;
                FTS_TEST_SAVE_INFO("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n ",  \
                                   chx + 1, chy + 1, cb_value, cb_min, cb_max);
            }
        }
    }

    //////////////////////////////Save Test Data
    save_test_data(cb_buf, tx_num + 1, rx_num, 1);
    FTS_TEST_SAVE_INFO("\n========== CB Test is %s. \n\n", (temp_result ? "OK" : "NG"));

    *test_result = temp_result;
    if (tmpbuf) {
        fts_free(tmpbuf);
    }
    return 0;

CB_TEST_ERR:
    FTS_TEST_SAVE_INFO("\n\n/==========CB Test is NG!");
    *test_result = false;
    if (tmpbuf) {
        fts_free(tmpbuf);
    }
    return ret;
}

/************************************************************************
* Name: ft8716_short_test
* Brief:
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
int ft8716_short_test(bool *test_result)
{
    int ret = 0;
    bool temp_result = true;
    int adc_data_num = 0;
    int chx = 0;
    int chy = 0;
    int adc_tmp = 0;
    int min = 0;
    int min_vk = 0;
    int max = 0;
    //    int short_value = 0;
    bool key_support = false;
    u8 tx_num = 0;
    u8 rx_num = 0;
    u8 channel_num = 0;
    int *adc_data = NULL;
    int tmptx = 0;

    FTS_TEST_SAVE_INFO("\r\n\r\n==============================Test Item: -------- Short Circuit Test \r\n");

    memset(test_data.buffer, 0, ((test_data.screen_param.tx_num + 1) * test_data.screen_param.rx_num) * sizeof(int));
    adc_data = test_data.buffer;

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n\r\n// Failed to Enter factory mode.");
        goto SHORT_TEST_ERR;
    }

    /* get adc & short resistor data */
    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    channel_num = tx_num + rx_num;

    if (test_data.screen_param.selected_ic == IC_FT8716) {
        adc_data_num = tx_num * rx_num + test_data.screen_param.key_num_total;
    } else if (test_data.screen_param.selected_ic == IC_FT8716U || test_data.screen_param.selected_ic == IC_FT8613) {
        adc_data_num = tx_num * rx_num ;
    }

    memset(adc_data, 0, adc_data_num * sizeof(int));
    ret = weakshort_get_adc_data(channel_num, adc_data_num * 2, adc_data);
    if (ret) {
        FTS_TEST_SAVE_INFO("// Failed to get AdcData. Error Code: %d", ret);
        goto SHORT_TEST_ERR;
    }

    /* revise and calculate resistor */
    for (chx = 0; chx < tx_num + 1; chx++) {
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++) {
            adc_tmp = adc_data[tmptx + chy];
            if (adc_tmp > 2007)
                adc_tmp = 2007;
            adc_data[tmptx + chy] = (adc_tmp * 100) / (2047 - adc_tmp);
        }
    }

    /* check short threshold */
    key_support =  ft8716_basicthreshold.short_vkey_check;
    min = ft8716_basicthreshold.short_va_resistor_min;
    min_vk = ft8716_basicthreshold.short_vkey_resistor_min;
    max = 100000000;
    temp_result = compare_data(adc_data, min, max, min_vk, max, key_support);

    FTS_TEST_SAVE_INFO(" ShortCircuit Test is %s. \n\n", (temp_result  ? "OK" : "NG"));

    *test_result = temp_result;
    return 0;

SHORT_TEST_ERR:
    *test_result = false;
    FTS_TEST_SAVE_INFO(" ShortCircuit Test is NG. \n\n");

    return ret;
}

/************************************************************************
* Name: ft8716u_key_short_test
* Brief:  Get Key short circuit test mode data, judge whether there is a short circuit
* Input: none
* Output: none
* Return: return 0 if success
***********************************************************************/
int ft8716u_key_short_test(bool *test_result)
{
    int ret = 0;
    bool temp_result = true;
    u8 key_num = 0;
    int i = 0;
    u8 test_finish = 0xFF;
    short tmpval;
    u8 k1_value = 0;
    u8 key_cb[KEY_NUM_MAX * 2] = { 0 };

    FTS_TEST_SAVE_INFO("\r\n\r\n==============================Test Item: -------- KEY Short Test \r\n");

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n\r\n// Failed to Enter factory mode.");
        goto KEYSHORT_TEST_ERR;
    }

    ret = read_reg(FACTORY_REG_K1, &k1_value);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========  Read K1 Reg Failed!");
        goto KEYSHORT_TEST_ERR;
    }

    ret = write_reg(FACTORY_REG_K1, ft8716_basicthreshold.keyshort_k1);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========  Write K1 Reg Failed!");
        goto KEYSHORT_TEST_ERR;
    }

    ret = wait_state_update();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========Wait State Update Failed!");
        goto KEYSHORT_TEST_ERR;
    }

    //Start KEY short Test
    ret = write_reg(0x2E, 0x01);
    if (ret) {
        FTS_TEST_SAVE_INFO("start key short test fail");
        goto KEYSHORT_TEST_ERR;
    }

    //check test is finished or not
    for ( i = 0; i < 20; ++i) {
        ret =  read_reg(0x2F, &test_finish);
        if ((0 == ret) && (0 == test_finish))
            break;

        sys_delay(50);
    }
    if (i >= 20) {
        FTS_TEST_SAVE_INFO("\r\n Test is not finished.\r\n");
        goto KEYSHORT_TEST_ERR;
    }

    key_num = (is_key_autofit() ? test_data.screen_param.key_num : test_data.screen_param.key_num_total) * 2;
    ret = get_cb(0, key_num, key_cb);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n\r\n//=========  CB test Failed!");
        goto KEYSHORT_TEST_ERR;
    }

    for ( i = 0; i < key_num; i += 2) {
        tmpval = (short)((key_cb[i] & 0x01 ) << 8) + key_cb[i + 1];
        if (tmpval  > ft8716_basicthreshold.keyshort_cb_max) {
            temp_result = false;
            FTS_TEST_SAVE_INFO("Point( 0, %-2d): %-9d  ", i / 2, tmpval);
        }
    }

    //Restore K1 Value, start CB calibration
    ret = write_reg(FACTORY_REG_K1, k1_value);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= restore k1 value fail");
        goto KEYSHORT_TEST_ERR;
    }

    ret = chip_clb();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto KEYSHORT_TEST_ERR;
    }

    FTS_TEST_SAVE_INFO(" KEY Short Test is %s. \n\n", (temp_result  ? "OK" : "NG"));

    *test_result = temp_result;
    return 0;

KEYSHORT_TEST_ERR:
    *test_result = false;
    FTS_TEST_SAVE_INFO("\r\n\r\n==========//KEY Short Test is NG!");

    return ret;
}

/************************************************************************
* Name: ft8716_open_test
* Brief:  Check if channel is open
* Input:
* Output:
* Return: return 0 if success
***********************************************************************/
int ft8716_open_test(bool *test_result)
{
    int ret = 0;
    bool temp_result = true;
    int cb_min = 0;
    int cb_max = 0;
    int i = 0;
    u8 gip_mode = 0xFF;
    u8 source_mode = 0xFF;
    u8 k1_value = 0;
    u8 k2_value = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    int cb_byte_num = 0;
    int *cb_buf = NULL;
    u8 *tmpbuf = NULL;

    FTS_TEST_SAVE_INFO("\r\n\r\n==============================Test Item: --------  Open Test");

    memset(test_data.buffer, 0, ((test_data.screen_param.tx_num + 1) * test_data.screen_param.rx_num) * sizeof(int));
    cb_buf = test_data.buffer;

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========  Enter Factory Failed!");
        goto OPEN_TEST_ERR;
    }

    /* set driver mode */
    if (test_data.screen_param.selected_ic == IC_FT8716U || test_data.screen_param.selected_ic == IC_FT8613) {
        ret = read_reg(FACTORY_REG_SOURCE_DRIVER_MODE, &source_mode);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//========= read source driver mode fail");
            goto OPEN_TEST_ERR;
        }
    }

    ret = read_reg(FACTORY_REG_GIP_DRIVER_MODE, &gip_mode);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= read gip driver mode fail");
        goto OPEN_TEST_ERR;
    }

    if (test_data.screen_param.selected_ic == IC_FT8716U || test_data.screen_param.selected_ic == IC_FT8613) {
        ret = write_reg(FACTORY_REG_SOURCE_DRIVER_MODE, 0x03);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//========= write source driver mode fail");
            goto OPEN_TEST_ERR;
        }
    }
    ret = write_reg(FACTORY_REG_GIP_DRIVER_MODE, 0x02);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= write gip driver mode fail");
        goto OPEN_TEST_ERR;
    }
    sys_delay(50);

    if (wait_state_update()) {
        FTS_TEST_SAVE_INFO("\r\n//=========Wait State Update Failed!");
        goto OPEN_TEST_ERR;
    }

    /* set k1/k2 */
    if (ft8716_basicthreshold.open_test_k1_check) {
        ret = read_reg(FACTORY_REG_K1, &k1_value);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  Read Reg Failed!");
            goto OPEN_TEST_ERR;
        }
        ret = write_reg(FACTORY_REG_K1, ft8716_basicthreshold.open_test_k1_threshold);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  Write Reg Failed!");
            goto OPEN_TEST_ERR;
        }
    }

    if (ft8716_basicthreshold.open_test_k2_check) {
        ret = read_reg(FACTORY_REG_K2, &k2_value);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  Read Reg Failed!");
            goto OPEN_TEST_ERR;
        }
        ret = write_reg(FACTORY_REG_K2, ft8716_basicthreshold.open_test_k2_threshold);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  Write Reg Failed!");
            goto OPEN_TEST_ERR;
        }
    }

    /* get cb data */
    ret = chip_clb();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto OPEN_TEST_ERR;
    }

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    cb_byte_num = (int)(tx_num * rx_num );

    tmpbuf = fts_malloc(cb_byte_num * sizeof(u8));
    if (NULL == tmpbuf) {
        FTS_TEST_SAVE_INFO("cb memory malloc fail");
        goto OPEN_TEST_ERR;
    }
    memset(tmpbuf, 0, cb_byte_num);
    memset(cb_buf, 0, cb_byte_num * sizeof(int));
    ret = get_cb(0, cb_byte_num, tmpbuf);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n\r\n//=========get CB Failed!");
        goto OPEN_TEST_ERR;
    }

    for (i = 0; i < cb_byte_num; i++) {
        cb_buf[i] = tmpbuf[i];
    }

    show_data(cb_buf, false);

    cb_min = ft8716_basicthreshold.open_test_cb_min;
    cb_max = 256;
    temp_result = compare_data(cb_buf, cb_min, cb_max, 0, 0, false);

    save_test_data(cb_buf, tx_num, rx_num, 1);

    /* restore register */
    if (ft8716_basicthreshold.open_test_k1_check) {
        ret = write_reg(FACTORY_REG_K1, k1_value);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  restore K1 fail");
            goto OPEN_TEST_ERR;
        }
    }

    if (ft8716_basicthreshold.open_test_k2_check) {
        ret = write_reg(FACTORY_REG_K2, k2_value);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  restore K2 fail");
            goto OPEN_TEST_ERR;
        }
    }

    ret = write_reg(FACTORY_REG_GIP_DRIVER_MODE, gip_mode);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//=========  restore GIP DRV MODE fail");
        goto OPEN_TEST_ERR;
    }

    if (test_data.screen_param.selected_ic == IC_FT8716U || test_data.screen_param.selected_ic == IC_FT8613) {
        ret = write_reg(FACTORY_REG_SOURCE_DRIVER_MODE, source_mode);
        if (ret) {
            FTS_TEST_SAVE_INFO("\r\n//=========  restore SOURCE DRV MODE fail");
            goto OPEN_TEST_ERR;
        }
    }
    sys_delay(50);

    ret = chip_clb();
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n//========= auto clb Failed!");
        goto OPEN_TEST_ERR;
    }
    FTS_TEST_SAVE_INFO("\n\r==========Open Test is %s. \n\n", (temp_result  ? "OK" : "NG"));

    if (tmpbuf)
        fts_free(tmpbuf);
    *test_result = temp_result;
    return 0;

OPEN_TEST_ERR:
    if (tmpbuf)
        fts_free(tmpbuf);
    *test_result = false;
    FTS_TEST_SAVE_INFO("\r\nOpen Test is NG. \n\n");

    return ret;
}

/************************************************************************
 * Name: ft8716_lcd_noise_test
 * Brief: obtain is differ data and calculate the corresponding type
 *        of noise value.
 * Input:
 * Output:
 * Return: return 0 if success
 ***********************************************************************/
int ft8716_lcd_noise_test(bool *test_result)
{
    int ret = 0;
    bool temp_result = true;
    int frame_num = 0;
    int i = 0;
    int chx = 0;
    int chy = 0;
    u8 tx_num = 0;
    u8 rx_num = 0;
    int tmptx = 0;
    int va_num = 0;
    u8 key_num = 0;
    int max = 0;
    int max_vk = 0;
    u8 act_lcdnoise_num = 0;
    u8 reg_value = 0;
    int *diff_buf = NULL;

    FTS_TEST_SAVE_INFO("\r\n\r\n==============================Test Item: -------- LCD Noise Test \r\n");

    memset(test_data.buffer, 0, ((test_data.screen_param.tx_num + 1) * test_data.screen_param.rx_num) * sizeof(int));
    diff_buf = test_data.buffer;

    ret = enter_factory_mode();
    if (ret) {
        FTS_TEST_SAVE_INFO("Enter Factory Mode fail\n");
        goto LCDNOISE_TEST_ERR;
    }

    /* write data select */
    ret = write_reg(FACTORY_REG_DATA_SELECT, 0x01);
    if (ret) {
        FTS_TEST_SAVE_INFO("write data select fail");
        return ret;
    }

    frame_num = ft8716_basicthreshold.lcd_noise_test_frame;
    ret = write_reg(FACTORY_REG_LCD_NOISE_FRAME, frame_num / 4);
    if (ret) {
        FTS_TEST_SAVE_INFO("\r\n write lcd noise frame fail");
        goto LCDNOISE_TEST_ERR;
    }

    ret = write_reg(FACTORY_REG_LCD_NOISE_START, 0x01);
    if (ret) {
        FTS_TEST_SAVE_INFO("start lcd noise test fail");
        goto LCDNOISE_TEST_ERR;
    }

    sys_delay(frame_num * 8);
    for (i = 0; i < frame_num; i++) {
        ret = read_reg(FACTORY_REG_LCD_NOISE_START, &reg_value );
        if ((0 == ret) && (0x00 == reg_value)) {
            sys_delay(16);
            ret = read_reg(DEVIDE_MODE_ADDR, &reg_value );
            if ((0 == ret) && (0x00 == (reg_value >> 7)))
                break;
        }

        sys_delay(50);
    }
    if (i >= frame_num) {
        ret = write_reg(FACTORY_REG_LCD_NOISE_START, 0x00);
        if (ret) {
            FTS_TEST_SAVE_INFO("write 0x00 to lcd noise start reg fail");
        } else
            ret = -EIO;

        FTS_TEST_SAVE_INFO( "\r\nLCD NOISE Time Over" );
        goto LCDNOISE_TEST_ERR;
    }

    tx_num = test_data.screen_param.tx_num;
    rx_num = test_data.screen_param.rx_num;
    key_num = test_data.screen_param.key_num_total;
    va_num = tx_num * rx_num;
    memset(diff_buf, 0, (tx_num + 1) * rx_num * sizeof(int));
    //--------------------------------------------Read RawData
    // VA
    ret = read_rawdata(0xAD, va_num * 2, diff_buf);
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Get VA RawData\n");
        goto LCDNOISE_TEST_ERR;
    }
    ret = read_rawdata(0xAE, key_num * 2, diff_buf + va_num);
    if (ret) {
        FTS_TEST_SAVE_INFO("Failed to Get KEY RawData\n");
        goto LCDNOISE_TEST_ERR;
    }

    ret = write_reg(FACTORY_REG_DATA_SELECT, 0x00);
    ret = write_reg(FACTORY_REG_LCD_NOISE_START, 0x00);
    if (ret) {
        FTS_TEST_SAVE_INFO( "\r\nRestore Failed" );
        goto LCDNOISE_TEST_ERR;
    }

    ret = read_reg(FACTORY_REG_LCD_NOISE_NUMBER, &act_lcdnoise_num);
    if (0 == act_lcdnoise_num) {
        act_lcdnoise_num = 1;
    }

    for (chx = 0; chx < tx_num + 1; chx++) {
        tmptx = chx * rx_num;
        for (chy = 0; chy < rx_num; chy++ ) {
            if (1 == ft8716_basicthreshold.lcd_noise_test_mode)
                diff_buf[tmptx + chy] = sqrt_new(diff_buf[tmptx + chy] / act_lcdnoise_num);
        }
    }

    // show data
    show_data(diff_buf, true);

    max = ft8716_basicthreshold.lcd_noise_test_coefficient * test_data.va_touch_thr * 32 / 100;
    max_vk = ft8716_basicthreshold.lcd_noise_test_coefficient_key * test_data.key_touch_thr * 32 / 100;
    temp_result = compare_data(diff_buf, 0, max, 0, max_vk, true);

    save_test_data(diff_buf, tx_num + 1, rx_num, 1 );

    FTS_TEST_SAVE_INFO(" \n ==========LCD Noise Test is %s. \n\n", (temp_result  ? "OK" : "NG"));

    *test_result = temp_result;
    return 0;

LCDNOISE_TEST_ERR:
    *test_result = false;
    FTS_TEST_SAVE_INFO("LCD Noise Test is NG. \n\n");

    return ret;
}

/************************************************************************
* Name: start_test_ft8716
* Brief: test entry
* Input:
* Output:
* Return: Test Result, PASS or FAIL
***********************************************************************/
bool start_test_ft8716(void)
{
    int ret = 0;
    bool test_result = true;
    bool temp_result = true;
    int item_count = 0;
    u8 item_code = 0;

    if (0 == test_data.test_num)
        test_result = false;

    ret = init_test();
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("Failed to init test. \n");
        return false;
    }

    for (item_count = 0; item_count < test_data.test_num; item_count++) {
        test_data.test_item_code = test_data.test_item[item_count].itemcode;
        item_code = test_data.test_item[item_count].itemcode;

        /* FT8716_ENTER_FACTORY_MODE */
        if (CODE_FT8716_ENTER_FACTORY_MODE == item_code) {
            ret = ft8716_enter_factory_mode();
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
                break; /* if this item FAIL, no longer test */
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716_RAWDATA_TEST */
        if (CODE_FT8716_RAWDATA_TEST == item_code) {
            ret = ft8716_rawdata_test(&temp_result);
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716_CB_TEST */
        if (CODE_FT8716_CB_TEST == item_code) {
            ret = ft8716_cb_test(&temp_result); //
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716_ShortCircuit_TEST */
        if (CODE_FT8716_SHORT_CIRCUIT_TEST == item_code) {
            ret = ft8716_short_test(&temp_result);
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716_Open_TEST */
        if (CODE_FT8716_OPEN_TEST == item_code) {
            ret = ft8716_open_test(&temp_result);
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716_LCDNoise_TEST */
        if (CODE_FT8716_LCD_NOISE_TEST == item_code) {
            ret = ft8716_lcd_noise_test(&temp_result);
            if (ret || (!temp_result)) {
                test_result = false;
                test_data.test_item[item_count].testresult = RESULT_NG;
            } else
                test_data.test_item[item_count].testresult = RESULT_PASS;
        }

        /* FT8716U_KEY_SHORT_TEST */
        if (CODE_FT8716U_KEY_SHORT_TEST == item_code) {
            ret = ft8716u_key_short_test(&temp_result);
            if (ret || (!temp_result)) {
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

void init_testitem_ft8716(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem", "RAWDATA_TEST", "1", str, strIniFile);
    ft8716_testitem.rawdata_test = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem", "CB_TEST", "1", str, strIniFile);
    ft8716_testitem.cb_test = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem", "SHORT_CIRCUIT_TEST", "1", str, strIniFile);
    ft8716_testitem.short_circuit_test = fts_atoi(str);

    /////////////////////////////////// OPEN_TEST
    GetPrivateProfileString("TestItem", "OPEN_TEST", "0", str, strIniFile);
    ft8716_testitem.open_test = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem", "LCD_NOISE_TEST", "0", str, strIniFile);
    ft8716_testitem.lcd_noise_test = fts_atoi(str);
    //////////////////////////////////////////////////////////// KEY_SHORT_TEST
    GetPrivateProfileString("TestItem", "KEY_SHORT_TEST", "0", str, strIniFile);
    ft8716_testitem.key_short_test = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_basicthreshold_ft8716(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VA_Check", "1", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min", "5000", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max", "11000", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_VKey_Check", "1", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_vkey_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Min_VKey", "5000", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_min_vkey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "RawDataTest_Max_VKey", "11000", str, strIniFile);
    ft8716_basicthreshold.rawdata_test_max_vkey = fts_atoi(str);

    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold", "CBTest_VA_Check", "1", str, strIniFile);
    ft8716_basicthreshold.cb_test_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min", "3", str, strIniFile);
    ft8716_basicthreshold.cb_test_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max", "100", str, strIniFile);
    ft8716_basicthreshold.cb_test_max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_VKey_Check", "1", str, strIniFile);
    ft8716_basicthreshold.cb_test_vkey_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Min_Vkey", "3", str, strIniFile);
    ft8716_basicthreshold.cb_test_min_vkey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "CBTest_Max_Vkey", "100", str, strIniFile);
    ft8716_basicthreshold.cb_test_max_vkey = fts_atoi(str);

    //////////////////////////////////////////////////////////// Short Circuit Test  VA,Key
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_VA_Check", "1", str, strIniFile);
    ft8716_basicthreshold.short_va_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_VKey_Check", "1", str, strIniFile);
    ft8716_basicthreshold.short_vkey_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_VA_ResMin", "200", str, strIniFile);
    ft8716_basicthreshold.short_va_resistor_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "ShortCircuit_Key_ResMin", "200", str, strIniFile);
    ft8716_basicthreshold.short_vkey_resistor_min = fts_atoi(str);


    ////////////////////////////////////////////////////////////open test
    GetPrivateProfileString("Basic_Threshold", "OpenTest_CBMin", "100", str, strIniFile);
    ft8716_basicthreshold.open_test_cb_min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "OpenTest_Check_K1", "0", str, strIniFile);
    ft8716_basicthreshold.open_test_k1_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "OpenTest_K1Threshold", "30", str, strIniFile);
    ft8716_basicthreshold.open_test_k1_threshold = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "OpenTest_Check_K2", "0", str, strIniFile);
    ft8716_basicthreshold.open_test_k2_check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "OpenTest_K2Threshold", "5", str, strIniFile);
    ft8716_basicthreshold.open_test_k2_threshold = fts_atoi(str);


    ////////////////////////////////////////////////////////////LCDNoiseTest
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_FrameNum", "200", str, strIniFile);
    ft8716_basicthreshold.lcd_noise_test_frame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_Coefficient", "60", str, strIniFile);
    ft8716_basicthreshold.lcd_noise_test_coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_Coefficient_Key", "60", str, strIniFile);
    ft8716_basicthreshold.lcd_noise_test_coefficient_key = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "LCDNoiseTest_NoiseMode", "0", str, strIniFile);
    ft8716_basicthreshold.lcd_noise_test_mode = fts_atoi(str);

    ////////////////////////////////////////////////////////////KEYShrotTest
    GetPrivateProfileString("Basic_Threshold", "KEY_Short_Test_K1_Value", "54", str, strIniFile);
    ft8716_basicthreshold.keyshort_k1 = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold", "KEY_Short_Test_CB_Max", "300", str, strIniFile);
    ft8716_basicthreshold.keyshort_cb_max = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();
}

void init_detailthreshold_ft8716(char *ini)
{
    FTS_TEST_FUNC_ENTER();

    OnInit_InvalidNode(ini);
    OnInit_DThreshold_CBTest(ini);
    OnThreshold_VkAndVaRawDataSeparateTest(ini);

    FTS_TEST_FUNC_EXIT();
}

void set_testitem_sequence_ft8716(void)
{
    test_data.test_num = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////Enter Factory Mode
    fts_set_testitem(CODE_FT8716_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////OPEN_TEST
    if ( ft8716_testitem.open_test == 1) {
        fts_set_testitem(CODE_FT8716_OPEN_TEST);
    }

    //////////////////////////////////////////////////SHORT_CIRCUIT_TEST
    if ( ft8716_testitem.short_circuit_test == 1) {
        fts_set_testitem(CODE_FT8716_SHORT_CIRCUIT_TEST) ;
    }

    //////////////////////////////////////////////////CB_TEST
    if ( ft8716_testitem.cb_test == 1) {
        fts_set_testitem(CODE_FT8716_CB_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( ft8716_testitem.lcd_noise_test == 1) {
        fts_set_testitem(CODE_FT8716_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( ft8716_testitem.rawdata_test == 1) {
        fts_set_testitem(CODE_FT8716_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////KEY_SHORT_TEST  for 8716U
    if ( ft8716_testitem.key_short_test == 1) {
        fts_set_testitem(CODE_FT8716U_KEY_SHORT_TEST) ;
    }
    FTS_TEST_FUNC_EXIT();

}

struct test_funcs test_func = {
    .init_testitem = init_testitem_ft8716,
    .init_basicthreshold = init_basicthreshold_ft8716,
    .init_detailthreshold = init_detailthreshold_ft8716,
    .set_testitem_sequence  = set_testitem_sequence_ft8716,
    .start_test = start_test_ft8716,
};
#endif
