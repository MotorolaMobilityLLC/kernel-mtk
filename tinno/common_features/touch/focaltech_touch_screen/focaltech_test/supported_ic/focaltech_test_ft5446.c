/************************************************************************
* Copyright (C) 2012-2018, Focaltech Systems (R), All Rights Reserved.
*
* File Name: Focaltech_test_ft5422.c
*
* Author: Focaltech Driver Team
*
* Created: 2015-07-14
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

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/

/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
struct selftest_result fts_selftest_result[5] = 
{
	{"rawdata test: ", 		false, false},
	{"scap_cb test: ", 		false, false},
	{"scap_rawdata test: ", 	false, false},
	{"short test: ", 		false, false},
	{"panel differ test: ", 	false, false},
};

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static int short_ch_gnd_ft5422(
    struct fts_test *tdata,
    int *adc,
    int *resistor,
    bool *result)
{
    int i = 0;
    bool tmp_result = true;
    u8 ic_version = 0;
    int dsen = 0;
    int drefn = 0;
    int fkcal = 1;
    int tmp1 = 0;
    int tmp2 = 0;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;
    int tx = tdata->sc_node.tx_num;

    FTS_TEST_FUNC_ENTER();
    if (!adc || !resistor || !result) {
        FTS_TEST_SAVE_ERR("adc/resistor/result is null\n");
        return -EINVAL;
    }

    FTS_TEST_DBG("short cg adc:\n");
    short_print_mc(adc, tdata->sc_node.channel_num + 1);

    show_data_mc_sc(&adc[1]);

    /* read 0xb1, <=0x05||==0xff:version E     =0x06 || others:version F */
    ic_version = (u8)(tdata->ic_ver & 0x000000FF);

    drefn = adc[0];
    for (i = 0; i < tdata->sc_node.channel_num; i++) {
        dsen = adc[i + 1];
        if (((2047 + tdata->offset) - dsen) <= 0) {
            /* <= 0: pass */
            continue;
        }

        if ((ic_version <= 0x05) || (ic_version == 0xFF)) {
            tmp1 = (dsen - tdata->offset + 410) * 25 * fkcal;
            tmp2 = (2047 + tdata->offset - dsen);
            resistor[i] = tmp1 / tmp2 - 3;
        } else {
            if ((drefn - dsen) <= 0) {
                /* <= 0: pass */
                resistor[i] = thr->basic.short_cg;
                continue;
            }
            tmp1 = (dsen - tdata->offset + 384) * 57;
            tmp2 = (drefn - dsen);
            resistor[i] = tmp1 / tmp2 - 1;
        }

        if (resistor[i] < 0)
            resistor[i] = 0;

        if ((resistor[i] < thr->basic.short_cg) || (dsen - tdata->offset) < 0) {
            tmp_result = false;
            if (i < tdata->sc_node.tx_num) {
                FTS_TEST_SAVE_ERR("TX%d:%d ", i + 1, resistor[i]);
            } else {
                FTS_TEST_SAVE_ERR("RX%d:%d ", i + 1 - tx, resistor[i]);
            }
        }
    }

    FTS_TEST_DBG("short cg resistor:\n");
    short_print_mc(adc, tdata->sc_node.channel_num);

    *result = tmp_result;
    FTS_TEST_FUNC_EXIT();
    return 0;
}

static int short_ch_ch_ft5422(
    struct fts_test *tdata,
    int *adc,
    int *resistor,
    bool *result)
{
    int i = 0;
    bool tmp_result = true;
    u8 ic_version = 0;
    int dsen = 0;
    int drefn = 0;
    int fkcal = 1;
    int dcal = 0;
    int ma = 0;
    int rsen = 57;
    int tmp1 = 0;
    int tmp2 = 0;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;
    int tx = tdata->sc_node.tx_num;

    FTS_TEST_FUNC_ENTER();
    if (!adc || !resistor || !result) {
        FTS_TEST_SAVE_ERR("adc/resistor/result is null\n");
        return -EINVAL;
    }

    FTS_TEST_DBG("short cc adc:\n");
    short_print_mc(adc, tdata->sc_node.channel_num + 1);
    show_data_mc_sc(&adc[1]);
    FTS_TEST_SAVE_INFO("offset:%d, code1:%d,code2:%d", tdata->offset, tdata->code1, tdata->code2);

    /* read 0xb1, <=0x05||==0xff:version E     =0x06 || others:version F */
    ic_version = (u8)(tdata->ic_ver & 0x000000FF);

    drefn = adc[0];
    dcal = 116 + tdata->offset;
    if (dcal < drefn)
        dcal = drefn;

    for (i = 0; i < tdata->sc_node.channel_num; i++) {
        dsen = adc[i + 1];
        if ((ic_version <= 0x05) || (ic_version == 0xFF)) {
            if ((dsen - drefn) < 0)
                continue;

            ma = dsen - dcal;
            ma = ma ? ma : 1;
            tmp1 = (2047 + tdata->offset - dcal) * 24;
            tmp2 = ma;
            resistor[i] = (tmp1 / ma - 27) * fkcal - 6;
        } else {
            if ((drefn - dsen) <= 0) {
                /* <= 0: pass */
                resistor[i] = thr->basic.short_cc;
                continue;
            }

            tmp1 = (dsen - tdata->offset - 123) * rsen * fkcal;
            tmp2 = (drefn - dsen);
            resistor[i] = tmp1 / tmp2;
        }

        if ((resistor[i] < 0) && (resistor[i] >= -240))
            resistor[i] = 0;
        else if (resistor[i] < -240)
            continue;

        if ((resistor[i] < thr->basic.short_cg) || (dsen - tdata->offset) < 0) {
            tmp_result = false;
            if (i < tdata->sc_node.tx_num) {
                FTS_TEST_SAVE_ERR("TX%d:%d ", i + 1, resistor[i]);
            } else {
                FTS_TEST_SAVE_ERR("RX%d:%d ", i + 1 - tx, resistor[i]);
            }
        }
    }

    FTS_TEST_DBG("short cc resistor:\n");
    short_print_mc(adc, tdata->sc_node.channel_num);

    *result = tmp_result;
    FTS_TEST_FUNC_EXIT();
    return 0;
}

static int ft5422_rawdata_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool result = false;
    bool result2 = false;
    u8 fre = 0;
    u8 fir = 0;
    u8 normalize = 0;
    int *rawdata = NULL;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: rawdata test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    rawdata = tdata->buffer;

    if (!thr->rawdata_h_min || !thr->rawdata_h_max || !test_result) {
        FTS_TEST_SAVE_ERR("rawdata_h_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    if (!thr->rawdata_l_min || !thr->rawdata_l_max) {
        FTS_TEST_SAVE_ERR("rawdata_l_min/max is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("failed to enter factory mode,ret=%d\n", ret);
        goto test_err;
    }

    /* rawdata test in mapping mode */
    ret = mapping_switch(MAPPING);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("switch mapping fail,ret=%d\n", ret);
        goto test_err;
    }

    /* save origin value */
    ret = fts_test_read_reg(FACTORY_REG_NORMALIZE, &normalize);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read normalize fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_FRE_LIST, &fre);
    if (ret) {
        FTS_TEST_SAVE_ERR("read 0x0A fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_FIR, &fir);
    if (ret) {
        FTS_TEST_SAVE_ERR("read 0xFB error,ret=%d\n", ret);
        goto test_err;
    }

    /* set to auto normalize */
    if (tdata->normalize != normalize) {
        ret = fts_test_write_reg(FACTORY_REG_NORMALIZE, tdata->normalize);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("write normalize fail,ret=%d\n", ret);
            goto restore_reg;
        }
    }

    result = true;
    result2 = true;
    if (NORMALIZE_AUTO == tdata->normalize) {
        FTS_TEST_SAVE_INFO( "NORMALIZE_AUTO:\n");
        ret = get_rawdata_mc(0x81, 0x01, rawdata);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("get rawdata fail,ret=%d\n", ret);
            goto restore_reg;
        }

        show_data(rawdata, false);
        save_data_csv(rawdata, "Rawdata Test", \
                      CODE_M_RAWDATA_TEST, false, false);

        /* compare */
        result = compare_array(rawdata,
                               thr->rawdata_h_min,
                               thr->rawdata_h_max,
                               false);
    } else if (NORMALIZE_OVERALL == tdata->normalize) {
        if (thr->basic.rawdata_set_lfreq) {
            FTS_TEST_SAVE_INFO( "NORMALIZE_OVERALL + SET_LOW_FREQ:\n");
            ret = get_rawdata_mc(0x80, 0x00, rawdata);
            if (ret < 0) {
                FTS_TEST_SAVE_ERR("get rawdata fail,ret=%d\n", ret);
                goto restore_reg;
            }

            show_data(rawdata, false);
            save_data_csv(rawdata, "Rawdata Test", \
                          CODE_M_RAWDATA_TEST, false, false);

            /* compare */
            result = compare_array(rawdata,
                                   thr->rawdata_l_min,
                                   thr->rawdata_l_max,
                                   false);
        }

        if (thr->basic.rawdata_set_hfreq) {
            FTS_TEST_SAVE_INFO( "NORMALIZE_OVERALL + SET_HIGH_FREQ:\n");
            ret = get_rawdata_mc(0x81, 0x00, rawdata);
            if (ret < 0) {
                FTS_TEST_SAVE_ERR("get rawdata fail,ret=%d\n", ret);
                goto restore_reg;
            }

            show_data(rawdata, false);
            save_data_csv(rawdata, "Rawdata Test", \
                          CODE_M_RAWDATA_TEST, false, false);

            /* compare */
            result2 = compare_array(rawdata,
                                    thr->rawdata_h_min,
                                    thr->rawdata_h_max,
                                    false);
        }
    }

restore_reg:
    /* set the origin value */
    ret = fts_test_write_reg(FACTORY_REG_NORMALIZE, normalize);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore normalize fail,ret=%d\n", ret);
    }

    ret = fts_test_write_reg(FACTORY_REG_FRE_LIST, fre);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore 0x0A fail,ret=%d\n", ret);
    }

    ret = fts_test_write_reg(FACTORY_REG_FIR, fir);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore 0xFB fail,ret=%d\n", ret);
    }

test_err:
    if (result && result2) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ Rawdata Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ Rawdata Test NG\n");
    }

    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft5422_scap_cb_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool tmp_result = false;
    bool tmp2_result = false;
    u8 wc_sel = 0;
    u8 sc_mode = 0;
    int byte_num = 0;
    bool fw_wp_check = false;
    bool tx_check = false;
    bool rx_check = false;
    int *scap_cb = NULL;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: Scap CB Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    scap_cb = tdata->buffer;
    byte_num = tdata->sc_node.node_num;

    if (!thr->scap_cb_on_min || !thr->scap_cb_on_max
        || !thr->scap_cb_off_min || !thr->scap_cb_off_max || !test_result) {
        FTS_TEST_SAVE_ERR("scap_cb_on/off_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* SCAP CB is in no-mapping mode */
    ret = mapping_switch(NO_MAPPING);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("switch no-mapping fail,ret=%d\n", ret);
        goto test_err;
    }

    /* get waterproof channel select */
    ret = fts_test_read_reg(FACTORY_REG_WC_SEL, &wc_sel);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read water_channel_sel fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_MC_SC_MODE, &sc_mode);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read sc_mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* water proof on check */
    fw_wp_check = get_fw_wp(wc_sel, WATER_PROOF_ON);
    if (thr->basic.scap_cb_wp_on_check && fw_wp_check) {
        /* 1:waterproof 0:non-waterproof */
        ret = get_cb_mc_sc(WATER_PROOF_ON, byte_num, scap_cb, DATA_ONE_BYTE);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("read sc_cb fail,ret=%d\n", ret);
            goto restore_reg;
        }

        /* show & save Scap CB */
        FTS_TEST_SAVE_INFO("scap_cb in waterproof on mode:\n");
        show_data_mc_sc(scap_cb);
        save_data_csv(scap_cb, "SCAP CB(ON) Test", \
                      CODE_M_SCAP_CB_TEST, true, false);

        /* compare */
        tx_check = get_fw_wp(wc_sel, WATER_PROOF_ON_TX);
        rx_check = get_fw_wp(wc_sel, WATER_PROOF_ON_RX);
        tmp_result = compare_mc_sc(tx_check, rx_check, scap_cb,
                                   thr->scap_cb_on_min,
                                   thr->scap_cb_on_max);
    } else {
        tmp_result = true;
    }

    /* water proof off check */
    fw_wp_check = get_fw_wp(wc_sel, WATER_PROOF_OFF);
    if (thr->basic.scap_cb_wp_on_check && fw_wp_check) {
        /* 1:waterproof 0:non-waterproof */
        ret = get_cb_mc_sc(WATER_PROOF_OFF, byte_num, scap_cb, DATA_ONE_BYTE);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("read sc_cb fail,ret=%d\n", ret);
            goto restore_reg;
        }

        /* show & save Scap CB */
        FTS_TEST_SAVE_INFO("scap_cb in waterproof off mode:\n");
        show_data_mc_sc(scap_cb);
        save_data_csv(scap_cb, "SCAP CB(OFF) Test", \
                      CODE_M_SCAP_CB_TEST, true, false);

        /* compare */
        tx_check = get_fw_wp(wc_sel, WATER_PROOF_OFF_TX);
        rx_check = get_fw_wp(wc_sel, WATER_PROOF_OFF_RX);
        tmp2_result = compare_mc_sc(tx_check, rx_check, scap_cb,
                                    thr->scap_cb_off_min,
                                    thr->scap_cb_off_max);

    } else {
        tmp2_result = true;
    }

restore_reg:
    ret = fts_test_write_reg(FACTORY_REG_MC_SC_MODE, sc_mode);/* set the origin value */
    if (ret) {
        FTS_TEST_SAVE_ERR("write sc mode fail,ret=%d\n", ret);
    }
test_err:
    if (tmp_result && tmp2_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ SCAP CB Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_ERR("------ SCAP CB Test NG\n");
    }
    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft5422_scap_rawdata_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool tmp_result = false;
    bool tmp2_result = false;
    u8 wc_sel = 0;
    bool fw_wp_check = false;
    bool tx_check = false;
    bool rx_check = false;
    int *scap_rawdata = NULL;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: Scap Rawdata Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    scap_rawdata = tdata->buffer;

    if (!thr->scap_rawdata_on_min || !thr->scap_rawdata_on_max
        || !thr->scap_rawdata_off_min || !thr->scap_rawdata_off_max
        || !test_result) {
        FTS_TEST_SAVE_ERR("scap_rawdata_on/off_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* SCAP CB is in no-mapping mode */
    ret = mapping_switch(NO_MAPPING);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("switch no-mapping fail,ret=%d\n", ret);
        goto test_err;
    }

    /* get waterproof channel select */
    ret = fts_test_read_reg(FACTORY_REG_WC_SEL, &wc_sel);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read water_channel_sel fail,ret=%d\n", ret);
        goto test_err;
    }

    /* scan rawdata */
    ret = start_scan();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("scan scap rawdata fail\n");
        goto test_err;
    }

    /* water proof on check */
    fw_wp_check = get_fw_wp(wc_sel, WATER_PROOF_ON);
    if (thr->basic.scap_rawdata_wp_on_check && fw_wp_check) {
        ret = get_rawdata_mc_sc(WATER_PROOF_ON, scap_rawdata);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("get scap(WP_ON) rawdata fail\n");
            goto test_err;
        }

        FTS_TEST_SAVE_INFO("scap_rawdata in waterproof on mode:\n");
        show_data_mc_sc(scap_rawdata);
        save_data_csv(scap_rawdata, "SCAP Rawdata(ON) Test", \
                      CODE_M_SCAP_RAWDATA_TEST, true, false);

        /* compare */
        tx_check = get_fw_wp(wc_sel, WATER_PROOF_ON_TX);
        rx_check = get_fw_wp(wc_sel, WATER_PROOF_ON_RX);
        tmp_result = compare_mc_sc(tx_check, rx_check, scap_rawdata,
                                   thr->scap_rawdata_on_min,
                                   thr->scap_rawdata_on_max);

    } else {
        tmp_result = true;
    }

    /* water proof off check */
    fw_wp_check = get_fw_wp(wc_sel, WATER_PROOF_OFF);
    if (thr->basic.scap_rawdata_wp_on_check && fw_wp_check) {
        ret = get_rawdata_mc_sc(WATER_PROOF_OFF, scap_rawdata);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("get scap(WP_OFF) rawdata fail\n");
            goto test_err;
        }

        FTS_TEST_SAVE_INFO("scap_rawdata in waterproof off mode:\n");
        show_data_mc_sc(scap_rawdata);
        save_data_csv(scap_rawdata, "SCAP Rawdata(OFF) Test", \
                      CODE_M_SCAP_RAWDATA_TEST, true, false);

        /* compare */
        tx_check = get_fw_wp(wc_sel, WATER_PROOF_OFF_TX);
        rx_check = get_fw_wp(wc_sel, WATER_PROOF_OFF_RX);
        tmp2_result = compare_mc_sc(tx_check, rx_check, scap_rawdata,
                                    thr->scap_rawdata_off_min,
                                    thr->scap_rawdata_off_max);

    } else {
        tmp2_result = true;
    }

test_err:
    if (tmp_result && tmp2_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ SCAP Rawdata Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_INFO("------ SCAP Rawdata Test NG\n");
    }
    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft5422_short_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    int i = 0;
    bool tmp_result = false;
    bool tmp2_result = false;
    int adc_num = 0;
    int *adc = NULL;
    int *resistor = NULL;

    FTS_TEST_SAVE_INFO("\n============ Test Item: Short Test\n");

    FTS_TEST_FUNC_ENTER();
    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("enter factory mode fail,ret=%d\n", ret);
        goto test_err;
    }

    /* SCAP CB is in no-mapping mode */
    ret = mapping_switch(NO_MAPPING);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("switch no-mapping fail,ret=%d\n", ret);
        goto test_err;
    }

    for (i = 0; i < 3; i++) {
        get_rawdata(tdata->buffer);
    }

    memset(tdata->buffer, 0, tdata->buffer_length);
    adc = tdata->buffer;

    /* The total channel number + calibration data + channel calibration data + Offset */
    adc_num = (1 + (1 + tdata->sc_node.channel_num) * 2);
    if (adc_num > tdata->buffer_length) {
        FTS_TEST_SAVE_ERR("adc num(%d) > buffer len(%d)", \
                          adc_num, tdata->buffer_length);
        ret = -EIO;
        goto test_err;
    }

    /* get adc data */
    for (i = 0; i < 5; i++) {
        ret = short_get_adc_data_mc(TEST_RETVAL_00, adc_num * 2, adc, \
                                    FACTROY_REG_SHORT_CA);
        if ((ret >= 0) && ((adc[0] != 0) && (adc[0] != 0xFFFF) \
                           && (adc[1] != 0) && (adc[1] != 0xFFFF)))
            break;
        else
            FTS_TEST_DBG("adc:%x %x,retry:%d", adc[0], adc[1], i);
    }
    if (i >= 5) {
        FTS_TEST_SAVE_ERR("get adc data timeout\n");
        ret = -EIO;
        goto test_err;
    }

    tdata->offset = adc[0] - 1024;
    tdata->code1 = adc[1];
    tdata->code2 = adc[2 + tdata->sc_node.channel_num];
    FTS_TEST_DBG("short offset:%d, code1:%d,code2:%d", tdata->offset, tdata->code1, tdata->code2);

    resistor = (int *)fts_malloc(tdata->sc_node.channel_num * sizeof(int));
    if (NULL == resistor) {
        FTS_TEST_SAVE_ERR("malloc for resistor fail\n");
        ret = -ENOMEM;
        goto test_err;
    }

    /* channel to gnd */
    ret = short_ch_gnd_ft5422(tdata, &adc[1], resistor, &tmp_result);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("channel to gnd fail\n");
    }

    /* channel to channel */
    ret = short_ch_ch_ft5422(tdata, &adc[1 + 1 + tdata->sc_node.channel_num], \
                             resistor, &tmp2_result);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("channel to channel fail\n");
    }

test_err:
    fts_free(resistor);

    if (tmp_result && tmp2_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ Short Test PASS\n");
    } else {
        *test_result = false;
        FTS_TEST_SAVE_ERR("code1:%d,offset:%d\n", tdata->code1, tdata->offset);
        FTS_TEST_SAVE_ERR("------ Short Test NG\n");
    }
    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int ft5422_panel_differ_test(struct fts_test *tdata, bool *test_result)
{
    int ret = 0;
    bool tmp_result = false;
    int i = 0;
    u8 fre = 0;
    u8 fir = 0;
    u8 normalize = 0;
    int *panel_differ = NULL;
    struct mc_sc_threshold *thr = &tdata->ic.mc_sc.thr;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_SAVE_INFO("\n============ Test Item: Panel Differ Test\n");
    memset(tdata->buffer, 0, tdata->buffer_length);
    panel_differ = tdata->buffer;

    if (!thr->panel_differ_min || !thr->panel_differ_max || !test_result) {
        FTS_TEST_SAVE_ERR("panel_differ_h_min/max test_result is null\n");
        ret = -EINVAL;
        goto test_err;
    }

    ret = enter_factory_mode();
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("failed to enter factory mode,ret=%d\n", ret);
        goto test_err;
    }

    /* panel differ test in mapping mode */
    ret = mapping_switch(MAPPING);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("switch mapping fail,ret=%d\n", ret);
        goto test_err;
    }

    /* save origin value */
    ret = fts_test_read_reg(FACTORY_REG_NORMALIZE, &normalize);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("read normalize fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_FRE_LIST, &fre);
    if (ret) {
        FTS_TEST_SAVE_ERR("read 0x0A fail,ret=%d\n", ret);
        goto test_err;
    }

    ret = fts_test_read_reg(FACTORY_REG_FIR, &fir);
    if (ret) {
        FTS_TEST_SAVE_ERR("read 0xFB fail,ret=%d\n", ret);
        goto test_err;
    }

    /* set to overall normalize */
    if (normalize != NORMALIZE_OVERALL) {
        ret = fts_test_write_reg(FACTORY_REG_NORMALIZE, NORMALIZE_OVERALL);
        if (ret < 0) {
            FTS_TEST_SAVE_ERR("write normalize fail,ret=%d\n", ret);
            goto restore_reg;
        }
    }

    ret = get_rawdata_mc(0x81, 0x00, panel_differ);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("get differ fail,ret=%d\n", ret);
        goto restore_reg;
    }

    for (i = 0; i < tdata->node.node_num; i++) {
        panel_differ[i] = panel_differ[i] / 10;
    }

    /* show & save test data */
    show_data(panel_differ, false);
    save_data_csv(panel_differ, "Panel Diff Test", \
                  CODE_M_PANELDIFFER_TEST, false, false);

    /* compare */
    tmp_result = compare_array(panel_differ,
                               thr->panel_differ_min,
                               thr->panel_differ_max,
                               false);

restore_reg:
    /* set the origin value */
    ret = fts_test_write_reg(FACTORY_REG_NORMALIZE, normalize);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore normalize fail,ret=%d\n", ret);
    }

    ret = fts_test_write_reg(FACTORY_REG_FRE_LIST, fre);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore 0x0A fail,ret=%d\n", ret);
    }

    ret = fts_test_write_reg(FACTORY_REG_FIR, fir);
    if (ret < 0) {
        FTS_TEST_SAVE_ERR("restore 0xFB fail,ret=%d\n", ret);
    }
test_err:
    /* result */
    if (tmp_result) {
        *test_result = true;
        FTS_TEST_SAVE_INFO("------ Panel Diff Test PASS\n");
    } else {
        * test_result = false;
        FTS_TEST_SAVE_ERR("------ Panel Diff Test NG\n");
    }
    FTS_TEST_FUNC_EXIT();
    return ret;
}

static int start_test_ft5422(void)
{
    int ret = 0;
    struct fts_test *tdata = fts_ftest;
    struct mc_sc_testitem *test_item = &tdata->ic.mc_sc.u.item;
    bool temp_result = false;
    bool test_result = true;

    FTS_TEST_FUNC_ENTER();
    FTS_TEST_INFO("test item:0x%x", fts_ftest->ic.mc_sc.u.tmp);

    /* rawdata test */
    if (true == test_item->rawdata_test) {
	fts_selftest_result[0].supported = true;
        ret = ft5422_rawdata_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
            fts_selftest_result[0].result = false;
        } else
            fts_selftest_result[0].result = true;
    } else
	fts_selftest_result[0].supported = false;

    /* scap_cb test */
    if (true == test_item->scap_cb_test) {
	fts_selftest_result[1].supported = true;
        ret = ft5422_scap_cb_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
            fts_selftest_result[1].result = false;
        } else
            fts_selftest_result[1].result = true;
    } else
	fts_selftest_result[1].supported = false;

    /* scap_rawdata test */
    if (true == test_item->scap_rawdata_test) {
	fts_selftest_result[2].supported = true;
        ret = ft5422_scap_rawdata_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
            fts_selftest_result[2].result = false;
        } else
            fts_selftest_result[2].result = true;
    } else
	fts_selftest_result[2].supported = false;

    /* short test */
    if (true == test_item->short_test) {
	fts_selftest_result[3].supported = true;
        ret = ft5422_short_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
            fts_selftest_result[3].result = false;
        } else
            fts_selftest_result[3].result = true;
    } else
	fts_selftest_result[3].supported = false;

    /* panel differ test */
    if (true == test_item->panel_differ_test) {
	fts_selftest_result[4].supported = true;
        ret = ft5422_panel_differ_test(tdata, &temp_result);
        if ((ret < 0) || (false == temp_result)) {
            test_result = false;
            fts_selftest_result[4].result = false;
        } else
            fts_selftest_result[4].result = true;
    } else
	fts_selftest_result[4].supported = false;

    /* restore mapping state */
    fts_test_write_reg(FACTORY_REG_NOMAPPING, tdata->mapping);
    return test_result;
}

struct test_funcs test_func_ft5422 = {
    .ctype = {0x02, 0x82},
    .hwtype = IC_HW_MC_SC,
    .key_num_total = 0,
    .rawdata2_support = true,
    .start_test = start_test_ft5422,
};
