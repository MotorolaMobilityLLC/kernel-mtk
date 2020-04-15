/************************************************************************
* Copyright (C) 2012-2019, Focaltech Systems (R), All Rights Reserved.
*
* File Name: Focaltech_test_ft8006p.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-01
*
* Abstract:
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "../focaltech_test.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define CODE2_SHORT_TEST                14
#define CODE2_LCD_NOISE_TEST            15

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

/*******************************************************************************
* Static variables
*******************************************************************************/

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
static int ft8006p_short_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    int i = 0;
    bool tmp_result = false;
    int byte_num = 0;
    int ch_num = 0;
    int min = 0;
    int max = 0;
    int tmp_adc = 0;
    int *adcdata = NULL;
    struct incell_threshold *thr = &tdata->ic.incell.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: Short Circuit Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    adcdata = tdata->buffer;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    byte_num = tdata->node.node_num * 2;
    ch_num = tdata->node.rx_num;
    ret = short_get_adcdata_incell(TEST_RETVAL_AA, ch_num, byte_num, adcdata);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("get adc data fail\n");
        goto test_err;
    }

    /* calculate resistor */
    for (i = 0; i < tdata->node.node_num; i++) {
        tmp_adc = adcdata[i];
        if (tmp_adc <= 0) {
            adcdata[i] = -1;
            continue;
        } else if ((tmp_adc >= 1) && (tmp_adc <= 300)) {
            tmp_adc = 300;
        }

        adcdata[i] = (4200 * 4096 / (70 * tmp_adc - 16384)) - 20;
        if (adcdata[i] > 3000) {
            adcdata[i] = 3000;
        }
    }

    /* show data in txt */
    show_data(adcdata, false);

    /* compare */
    min = thr->basic.short_res_min;
    max = TEST_SHORT_RES_MAX;
    tmp_result = compare_data(adcdata, min, max, min, max, false);

    ret = 0;
test_err:
    ret = fts_test_write_reg(FACTORY_REG_SHORT_TEST_STATE, 0x03);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write 03 to reg10 fail\n");
    }

    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ Short Circuit Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ Short Circuit Test NG\n");
    }

    /* save data */
    fts_test_save_data("Short Circuit Test", CODE2_SHORT_TEST,
                       adcdata, 0, false, false, *test_result);

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8006p_open_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    int i = 0;
    bool tmp_result = false;
    u8 state = 0;
    int byte_num = 0;
    int min = 0;
    int max = 0;
    int *opendata = NULL;
    struct incell_threshold *thr = &tdata->ic.incell.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: Open Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    opendata = tdata->buffer;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_write_reg(FACTORY_REG_OPEN_START, 0x01);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("start open test fail\n");
        goto test_err;
    }

    /* check test status */
    for (i = 0; i < FACTORY_TEST_RETRY; i++) {
        sys_delay(FACTORY_TEST_RETRY_DELAY);
        ret = fts_test_read_reg(FACTORY_REG_OPEN_STATE, &state);
        if ((ret >= 0) && (TEST_RETVAL_AA == state)) {
            break;
        } else {
            FTS_TEST_DBG("reg%x=%x,retry:%d\n",
                         FACTORY_REG_OPEN_STATE, state, i);
        }
    }
    if (i >= FACTORY_TEST_RETRY) {
        FTS_TEST_SAVE_ERR("open test timeout\n");
        goto restore_reg;
    }

    /* get cb data */
    byte_num = tdata->node.node_num;
    ret = get_cb_incell(0, byte_num, opendata);
    if (ret) {
        FTS_TEST_SAVE_ERR("get CB fail\n");
        goto restore_reg;
    }

    /* show data in txt */
    show_data(opendata, false);

    /* compare */
    min = thr->basic.open_cb_min;
    max = TEST_OPEN_MAX_VALUE;
    tmp_result = compare_data(opendata, min, max, 0, 0, false);

restore_reg:
    /* auto clb */
    ret = chip_clb();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("auto clb fail\n");
    }

test_err:
    ret = fts_test_write_reg(FACTORY_REG_OPEN_STATE, 0x03);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write 03 to reg16 fail\n");
    }

    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ Open Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ Open Test NG\n");
    }

    /* save */
    fts_test_save_data("Open Test", CODE_OPEN_TEST,
                       opendata, 0, false, false, *test_result);

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8006p_cb_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool tmp_result = false;
    int byte_num = 0;
    int *cbdata = NULL;
    struct incell_threshold *thr = &tdata->ic.incell.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: CB Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    cbdata = tdata->buffer;

    if (!thr->cb_min || !thr->cb_max || !test_result) {
        FTS_TEST_SAVE_ERR("cb_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* cb test enable */
    ret = fts_test_write_reg(FACTORY_REG_CB_TEST_EN, 0x01);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("cb test enable fail\n");
        goto test_err;
    }

    /* auto clb */
    ret = chip_clb();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("auto clb fail\n");
        goto test_err;
    }

    byte_num = tdata->node.node_num;
    ret = get_cb_incell(0, byte_num, cbdata);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("get cb fail\n");
        goto test_err;
    }

    /* show data in txt */
    show_data(cbdata, false);

    /* compare */
    tmp_result = compare_array(cbdata, thr->cb_min, thr->cb_max, false);

test_err:
    /* cb test disable */
    ret = fts_test_write_reg(FACTORY_REG_CB_TEST_EN, 0x00);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("cb test disable fail\n");
    }

    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ CB Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ CB Test NG\n");
    }

    /* save */
    fts_test_save_data("CB Test", CODE_CB_TEST,
                       cbdata, 0, false, false, *test_result);

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8006p_rawdata_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool tmp_result = false;
    int *rawdata = NULL;
    struct incell_threshold *thr = &tdata->ic.incell.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: RawData Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    rawdata = tdata->buffer;

    if (!thr->rawdata_min || !thr->rawdata_max || !test_result) {
        FTS_TEST_SAVE_ERR("rawdata_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* rawdata test enable */
    ret = fts_test_write_reg(FACTORY_REG_RAWDATA_TEST_EN, 0x01);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("rawdata test enable fail\n");
        goto test_err;
    }

    /* read rawdata */
    ret = get_rawdata(rawdata);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("get RawData fail,ret=%d\n", ret);
        goto test_err;
    }

    /* show dat in txt */
    show_data(rawdata, false);

    /* compare */
    tmp_result = compare_array(rawdata,
                               thr->rawdata_min,
                               thr->rawdata_max,
                               false);
    ret = 0;
test_err:
    /* rawdata test disble */
    ret = fts_test_write_reg(FACTORY_REG_RAWDATA_TEST_EN, 0x00);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("rawdata test disable fail\n");
    }

    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ RawData Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ RawData Test NG\n");
    }

    /* save */
    fts_test_save_data("RawData Test", CODE_RAWDATA_TEST,
                       rawdata, 0, false, false, *test_result);

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft8006p_lcdnoise_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    int i = 0;
    bool tmp_result = false;
    u8 old_mode = 0;
    u8 status = 0;
    u8 touch_thr = 0;
    int byte_num = 0;
    int frame_num = 0;
    int max = 0;
    int *lcdnoise = NULL;
    struct incell_threshold *thr = &tdata->ic.incell.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: LCD Noise Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    lcdnoise = tdata->buffer;

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_LCD_NOISE_TTHR, &touch_thr);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read reg14 fail\n");
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_DATA_SELECT, &old_mode);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read reg06 fail\n");
        goto test_err;
    }

    ret =  fts_test_write_reg(FACTORY_REG_DATA_SELECT, 0x01);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write 1 to reg06 fail\n");
        goto restore_reg;
    }

    ret =  fts_test_write_reg(FACTORY_REG_LINE_ADDR, 0xAD);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write reg01 fail\n");
        goto restore_reg;
    }

    frame_num = thr->basic.lcdnoise_frame;
    ret = fts_test_write_reg(FACTORY_REG_LCD_NOISE_FRAME, frame_num / 4);
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("write frame num fail\n");
        goto restore_reg;
    }

    /* start test */
    ret = fts_test_write_reg(FACTORY_REG_LCD_NOISE_START, 0x01);
    if (ret < 0) {
        FTS_TEST_SAVE_INFO("start lcdnoise test fail\n");
        goto restore_reg;
    }

    /* check test status */
    sys_delay(frame_num * FACTORY_TEST_DELAY / 2);
    for (i = 0; i < FACTORY_TEST_RETRY; i++) {
        status = 0xFF;
        ret = fts_test_read_reg(FACTORY_REG_LCD_NOISE_STATE, &status);
        if ((ret >= 0) && (TEST_RETVAL_AA == status)) {
            break;
        } else {
            FTS_TEST_DBG("reg%x=%x,retry:%d\n",
                         FACTORY_REG_LCD_NOISE_STATE, status, i);
        }
        sys_delay(FACTORY_TEST_RETRY_DELAY);
    }
    if (i >= FACTORY_TEST_RETRY) {
        FTS_TEST_SAVE_ERR("lcdnoise test timeout\n");
        goto restore_reg;
    }
    /* read lcdnoise */
    byte_num = tdata->node.node_num * 2;
    ret = read_mass_data(FACTORY_REG_RAWDATA_ADDR, byte_num, lcdnoise);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read rawdata fail\n");
        goto restore_reg;
    }

    /* show data in txt */
    show_data(lcdnoise, false);

    /* compare */
    max = thr->basic.lcdnoise_coefficient * touch_thr * 32 / 100;
    FTS_TEST_DBG("touch thr:%d, max:%d", touch_thr, max);
    tmp_result = compare_data(lcdnoise, 0, max, 0, 0, false);

restore_reg:
    ret = fts_test_write_reg(FACTORY_REG_LCD_NOISE_START, 0x00);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write 0 to reg11 fail\n");
    }

    ret = fts_test_write_reg(FACTORY_REG_DATA_SELECT, old_mode);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore reg06 fail\n");
    }
test_err:
    ret = fts_test_write_reg(FACTORY_REG_LCD_NOISE_STATE, 0x03);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("write 03 to reg1E fail\n");
    }

    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ LCD Noise Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ LCD Noise Test NG\n");
    }

    /* save */
    fts_test_save_data("LCD Noise Test", CODE2_LCD_NOISE_TEST,
                       lcdnoise, 0, false, false, *test_result);

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int start_test_ft8006p(void)
{
    int ret = 0;
    struct fts_test *tdata = fts_ftest;
    struct incell_testitem *test_item = &tdata->ic.incell.u.item;
    bool temp_result = false;
    bool test_result = true;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_INFO("test item:0x%x", fts_ftest->ic.incell.u.tmp);

    if (!tdata || !tdata->buffer) {
        FTS_TEST_ERROR("tdata is null");
        return -EINVAL;
    }

    /* short test */
    if (true == test_item->short_test) {
        ret = ft8006p_short_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
        }
    }

    /* open test */
    if (true == test_item->open_test) {
        ret = ft8006p_open_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
        }
    }

    /* cb test */
    if (true == test_item->cb_test) {
        ret = ft8006p_cb_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
        }
    }

    /* rawdata test */
    if (true == test_item->rawdata_test) {
        ret = ft8006p_rawdata_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
        }
    }

    /* lcd noise test */
    if (true == test_item->lcdnoise_test) {
        ret = ft8006p_lcdnoise_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
        }
    }

    return test_result;
}

struct test_funcs test_func_ft8006p = {
    .ctype = {0x11},
    .hwtype = IC_HW_INCELL,
    .key_num_total = 0,
    .start_test = start_test_ft8006p,
};
