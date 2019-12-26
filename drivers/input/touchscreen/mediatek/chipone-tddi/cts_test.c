#define LOG_TAG         "Test"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"

#define CTS_FIRMWARE_WORK_MODE_NORMAL   (0x00)
#define CTS_FIRMWARE_WORK_MODE_FACTORY  (0x01)
#define CTS_FIRMWARE_WORK_MODE_CONFIG   (0x02)
#define CTS_FIRMWARE_WORK_MODE_TEST     (0x03)

#define CTS_TEST_SHORT                  (0x01)
#define CTS_TEST_OPEN                   (0x02)

#define CTS_SHORT_TEST_UNDEFINED        (0x00)
#define CTS_SHORT_TEST_BETWEEN_COLS     (0x01)
#define CTS_SHORT_TEST_BETWEEN_ROWS     (0x02)
#define CTS_SHORT_TEST_BETWEEN_GND      (0x03)

#define TEST_RESULT_BUFFER_SIZE(cts_dev) \
    (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

#define RAWDATA_BUFFER_SIZE(cts_dev) \
		(cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

static int disable_fw_esd_protection(struct cts_device *cts_dev)
{
    return cts_fw_reg_writeb(cts_dev, 0x8000 + 342, 1);
}

static int disable_fw_monitor_mode(struct cts_device *cts_dev)
{
    int ret;
    u8 value;

    ret = cts_fw_reg_readb(cts_dev, 0x8000 + 344, &value);
    if (ret) {
        return ret;
    }

    if (value & BIT(0)) {
        return cts_fw_reg_writeb(cts_dev, 0x8000 + 344, value & (~BIT(0)));
    }

    return 0;
}

static int disable_fw_auto_compensate(struct cts_device *cts_dev)
{
    return cts_fw_reg_writeb(cts_dev, 0x8000 + 276, 1);
}

static int set_fw_work_mode(struct cts_device *cts_dev, u8 mode)
{
    int ret, retries;
    u8  pwr_mode;

	cts_info("Set firmware work mode to %u", mode);

    ret = cts_fw_reg_writeb(cts_dev, CTS_DEVICE_FW_REG_WORK_MODE, mode);
    if (ret) {
        cts_err("Write firmware work mode register failed %d", ret);
        return ret;
    }

    ret = cts_fw_reg_readb(cts_dev, 0x05, &pwr_mode);
    if (ret) {
        cts_err("Read firmware power mode register failed %d", ret);
        return ret;
    }

    if (pwr_mode == 1) {
        ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
        if (ret) {
            cts_err("Send cmd QUIT_GESTURE_MONITOR failed %d", ret);
            return ret;
        }

        msleep(50);
    }

    retries = 0;
    do {
        u8 sys_busy, curr_mode;

        msleep(10);

        ret = cts_fw_reg_readb(cts_dev, 0x01, &sys_busy);
        if (ret) {
            cts_err("Read firmware system busy register failed %d", ret);
            //return ret;
			continue;
        }
        if (sys_busy)
            continue;

        ret = cts_fw_reg_readb(cts_dev, 0x3F, &curr_mode);
        if (ret) {
            cts_err("Read firmware current work mode failed %d", ret);
            //return ret;
			continue;
        }

        if (curr_mode == mode/* || curr_mode == 0xFF*/) {
            break;
        }
    } while (retries++ < 1000);

    return (retries >= 1000 ? -ETIMEDOUT : 0);
}

static int set_display_state(struct cts_device *cts_dev, bool active)
{
    int ret;
    u8  access_flag;

	cts_info("Set display state to %s", active ? "ACTIVE" : "SLEEP");
	
    ret = cts_hw_reg_readb(cts_dev, 0x3002C, &access_flag);
    if (ret) {
        cts_err("Read display access flag failed %d", ret);
        return ret;
    }

    ret = cts_hw_reg_writeb(cts_dev, 0x3002C, access_flag | 0x01);
    if (ret) {
        cts_err("Write display access flag %02x failed %d", access_flag, ret);
        return ret;
    }

    if (active) {
        ret = cts_hw_reg_writeb(cts_dev, 0x3C044, 0x55);
        if (ret) {
            cts_err("Write DCS-CMD11 fail");
            return ret;
        }

        msleep(100);

        ret = cts_hw_reg_writeb(cts_dev, 0x3C0A4, 0x55);
        if (ret) {
            cts_err("Write DCS-CMD29 fail");
            return ret;
        }

        msleep(100);
    } else {
        ret = cts_hw_reg_writeb(cts_dev, 0x3C0A0, 0x55);
        if (ret) {
            cts_err("Write DCS-CMD28 fail");
            return ret;
        }

        msleep(100);

        ret = cts_hw_reg_writeb(cts_dev, 0x3C040, 0x55);
        if (ret) {
            cts_err("Write DCS-CMD10 fail");
            return ret;
        }

        msleep(100);
    }

    ret = cts_hw_reg_writeb(cts_dev, 0x3002C, access_flag);
    if (ret) {
        cts_err("Restore display access flag %02x failed %d", access_flag, ret);
        return ret;
    }

    return 0;
}

static int wait_test_complete(struct cts_device *cts_dev, int skip_frames)
{
    int ret, i, j;

	cts_info("Wait test complete skip %d frames", skip_frames);

    for(i = 0; i < (skip_frames + 1); i++) {
        u8 ready;

        for (j = 0; j < 1000; j++) {
            mdelay(1);

            ready = 0;
            ret = cts_get_data_ready_flag(cts_dev, &ready);
            if (ret) {
                cts_err("Get data ready flag failed %d", ret);
                return ret;
            }

            if (ready) {
                break;
            }
        }

        if (ready == 0) {
            return -ETIMEDOUT;
        }
        if (i < skip_frames) {
	        ret = cts_clr_data_ready_flag(cts_dev);
	        if (ret) {
	            cts_err("Clear data ready flag failed %d", ret);
	            return ret;
	        }
        }
    }

    return 0;
}

static int get_test_result(struct cts_device *cts_dev, u16 *result)
{
    int ret;

    ret = cts_fw_reg_readsb(cts_dev, CTS_DEVICE_FW_REG_RAW_DATA, result,
            TEST_RESULT_BUFFER_SIZE(cts_dev));
    if (ret) {
        cts_err("Get test result data failed %d", ret);
        return ret;
    }

    ret = cts_clr_data_ready_flag(cts_dev);
    if (ret) {
        cts_err("Clear data ready flag failed %d", ret);
        return ret;
    }
    
    return 0;
}

static int set_fw_test_type(struct cts_device *cts_dev, u8 type)
{
    int ret, retries = 0;
    u8  sys_busy;
    u8  type_readback;

	cts_info("Set test type %d", type);

    ret = cts_fw_reg_writeb(cts_dev, 0x34, type);
    if (ret) {
        cts_err("Write test type register to failed %d", ret);
        return ret;
    }

    do {
        msleep(1);

        ret = cts_fw_reg_readb(cts_dev, 0x01, &sys_busy);
        if (ret) {
            cts_err("Read system busy register failed %d", ret);
            return ret;
        }
    } while (sys_busy && retries++ < 1000);

    if (retries >= 1000) {
        cts_err("Wait system ready timeout");
        return -ETIMEDOUT;
    }

    ret = cts_fw_reg_readb(cts_dev, 0x34, &type_readback);
    if (ret) {
        cts_err("Read test type register failed %d", ret);
        return ret;
    }

    if (type != type_readback) {
        cts_err("Set test type %u != readback %u", type, type_readback);
        return -EFAULT;
    }

    return 0;
}

static bool set_short_test_type(struct cts_device *cts_dev, u8 type)
{
    static struct fw_short_test_param {
        u8  type;
        u32 col_pattern[2];
        u32 row_pattern[2];
    } param = {
        .type = CTS_SHORT_TEST_BETWEEN_COLS,
        .col_pattern = {0, 0},
        .row_pattern = {0, 0}
    };
    int ret;

	cts_info("Set short test type to %u", type);

	param.type = type;
    ret = cts_fw_reg_writesb(cts_dev, 0x5000, &param, sizeof(param));
    if (ret) {
        cts_err("Set short test type to %u failed %d", type, ret);
        return ret;
    }

    return 0;
}

static void dump_test_data(struct cts_device *cts_dev, 
    const char *desc, const u16 *data)
{
#define SPLIT_LINE_STR \
        "----------------------------------------------------------------------------------------------------------------"
#define ROW_NUM_FORMAT_STR  "%2d | "
#define COL_NUM_FORMAT_STR  "%-5u "
#define DATA_FORMAT_STR     "%-5u "

    int r, c;
    u32 max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    char linebuf [128];
    int count;

    max = min = data[0];
    sum = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            u16 val = data[r * cts_dev->fwdata.cols + c];

            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

    cts_info(SPLIT_LINE_STR);
    count = 0;
    count += snprintf(linebuf + count, sizeof(linebuf) - count,
           " %s test data MIN: [%u][%u]=%u, MAX: [%u][%u]=%u, AVG=%u",
           desc, min_r, min_c, min, max_r, max_c, max, average);
    cts_info("%s", linebuf);
    cts_info(SPLIT_LINE_STR);

    count = 0;
    count += snprintf(linebuf + count, sizeof(linebuf) - count, "   | ");
    for (c = 0; c < cts_dev->fwdata.cols; c++) {
        count += snprintf(linebuf + count, sizeof(linebuf) - count,
			COL_NUM_FORMAT_STR, c);
    }
    cts_info("%s", linebuf);
    cts_info(SPLIT_LINE_STR);

    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        count = 0;
        count += snprintf(linebuf + count, sizeof(linebuf) - count,
			ROW_NUM_FORMAT_STR, r);
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            count += snprintf(linebuf + count, sizeof(linebuf) - count,
				DATA_FORMAT_STR, data[r * cts_dev->fwdata.cols + c]);
        }
        cts_info("%s", linebuf);
    }
    cts_info(SPLIT_LINE_STR);

#undef SPLIT_LINE_STR
#undef ROW_NUM_FORMAT_STR
#undef COL_NUM_FORMAT_STR
#undef DATA_FORMAT_STR
}

static void dump_comp_cap(struct cts_device *cts_dev, const u8 *data)
{
#define SPLIT_LINE_STR \
        "----------------------------------------------------------------------------"
#define ROW_NUM_FORMAT_STR  "%2d | "
#define COL_NUM_FORMAT_STR  "%-3u "
#define DATA_FORMAT_STR     "%-3u "

    int r, c;
    u32 max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    char linebuf [128];
    int count;

    max = min = data[0];
    sum = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            u8 val = data[r * cts_dev->fwdata.cols + c];

            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

    cts_info(SPLIT_LINE_STR);
    count = 0;
    count += snprintf(linebuf + count, sizeof(linebuf) - count,
           " Compensate Cap MIN: [%u][%u]=%u, MAX: [%u][%u]=%u, AVG=%u",
           min_r, min_c, min, max_r, max_c, max, average);
    cts_info("%s", linebuf);
    cts_info(SPLIT_LINE_STR);

    count = 0;
    count += snprintf(linebuf + count, sizeof(linebuf) - count, "   | ");
    for (c = 0; c < cts_dev->fwdata.cols; c++) {
        count += snprintf(linebuf + count, sizeof(linebuf) - count,
			COL_NUM_FORMAT_STR, c);
    }
    cts_info("%s", linebuf);
    cts_info(SPLIT_LINE_STR);

    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        count = 0;
        count += snprintf(linebuf + count, sizeof(linebuf) - count,
			ROW_NUM_FORMAT_STR, r);
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            count += snprintf(linebuf + count, sizeof(linebuf) - count,
				DATA_FORMAT_STR, data[r * cts_dev->fwdata.cols + c]);
        }
        cts_info("%s", linebuf);
    }
    cts_info(SPLIT_LINE_STR);

#undef SPLIT_LINE_STR
#undef ROW_NUM_FORMAT_STR
#undef COL_NUM_FORMAT_STR
#undef DATA_FORMAT_STR
}
/* Return number of failed nodes */
static int validate_test_result(struct cts_device *cts_dev,
        const char *desc, u16 *test_result, u16 threshold)
{
#define SPLIT_LINE_STR \
    "------------------------------\n"

    int r, c;
    int failed_cnt = 0;

    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            if (test_result[r * cts_dev->fwdata.cols + c] < threshold) {
                if (failed_cnt == 0) {
                    printk("%s failed nodes:\n"
                           SPLIT_LINE_STR, desc);
                }
                failed_cnt++;

                printk("  %3d: [%-2d][%-2d] = %u\n", failed_cnt, r, c,
                    test_result[r * cts_dev->fwdata.cols + c]);
            }
        }
    }

    printk("%s"
           "%s test %d node total failed\n",
           failed_cnt ? SPLIT_LINE_STR : "", desc, failed_cnt);

    return failed_cnt;

#undef SPLIT_LINE_STR
}

static int test_short_to_gnd(struct cts_device *cts_dev,
        u8 feature_ver, u16 *test_result, u16 threshold)
{
    int ret;

    cts_info("Test short to GND");

    ret = set_short_test_type(cts_dev, CTS_SHORT_TEST_UNDEFINED);
    if (ret) {
        cts_err("Set short test type to UNDEFINED failed %d", ret);
        return ret;
    }

    ret = set_fw_test_type(cts_dev, CTS_TEST_SHORT);
    if (ret) {
        cts_err("Set test type to SHORT failed %d", ret);
        return ret;
    }

    ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_TEST);
    if (ret) {
        cts_err("Set firmware work mode to WORK_MODE_TEST failed %d", ret);
        return ret;
    }

    if (feature_ver <= 3) {
        u8 val;
        ret = cts_hw_reg_readb(cts_dev, 0x350E2, &val);
        if (ret) {
            cts_err("Read 0x350E2 failed %d", ret);
            return ret;
        }
        ret = cts_hw_reg_writeb(cts_dev, 0x350E2, val & 0xDB);
        if (ret) {
            cts_err("Write 0x350E2 failed %d", ret);
            return ret;
        }
    }

    ret = set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_GND);
    if (ret) {
        cts_err("Set short test type to SHORT_TO_GND failed %d", ret);
        return ret;
    }

    ret = wait_test_complete(cts_dev, 0);
    if (ret) {
        cts_err("Wait test complete failed %d", ret);
        return ret;
    }

    ret = get_test_result(cts_dev, test_result);
    if (ret) {
        cts_err("Read test result failed %d", ret);
        return ret;
    }

    dump_test_data(cts_dev, "GND-short", test_result);

    return validate_test_result(cts_dev, "GND-short", test_result, threshold);
}

static int test_short_to_gnd_legacy(struct cts_device *cts_dev,
        u16 *test_result, u16 threshold)
{
    int ret;

    cts_info("Test short to GND");

    ret = cts_send_command(cts_dev, CTS_CMD_RECOVERY_TX_VOL);
    if (ret) {
        cts_err("Send command RECOVERY_TX_VOL failed %d", ret);
        return ret;
    }

    ret = wait_test_complete(cts_dev, 2);
    if (ret) {
        cts_err("Wait test complete failed %d", ret);
        return ret;
    }

    ret = get_test_result(cts_dev, test_result);
    if (ret) {
        cts_err("Read test result failed %d", ret);
        return ret;
    }

    dump_test_data(cts_dev, "GND-short", test_result);

    return validate_test_result(cts_dev, "GND-short", test_result, threshold);
}

static int test_short_between_cols(struct cts_device *cts_dev,
    u16 *test_result, u16 threshold, bool skip_first_frame)
{
    int ret;

    cts_info("Test short between columns");

    ret = set_fw_test_type(cts_dev, CTS_TEST_SHORT);
    if (ret) {
        cts_err("Set test type to SHORT failed %d", ret);
        return ret;
    }

    ret = set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_COLS);
    if (ret) {
        cts_err("Set short test type to BETWEEN_COLS failed %d", ret);
        return ret;
    }

    ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_TEST);
    if (ret) {
        cts_err("Set firmware work mode to WORK_MODE_TEST failed %d", ret);
        return ret;
    }

    if (skip_first_frame) {
        cts_info("Skip first frame data");

        ret = wait_test_complete(cts_dev, 0);
        if (ret) {
            cts_err("Wait test complete failed %d", ret);
            return ret;
        }

        ret = get_test_result(cts_dev, test_result);
        if (ret) {
            cts_err("Read skip test result failed %d", ret);
            return ret;
        }

        ret = set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_COLS);
        if (ret) {
            cts_err("Set short test type to BETWEEN_COLS failed %d", ret);
            return ret;
        }
    }

    ret = wait_test_complete(cts_dev, 0);
    if (ret) {
        cts_err("Wait test complete failed %d", ret);
        return ret;
    }

    ret = get_test_result(cts_dev, test_result);
    if (ret) {
        cts_err("Read test result failed %d", ret);
        return ret;
    }

    dump_test_data(cts_dev, "Col-short", test_result);

    return validate_test_result(cts_dev, "Col-short", test_result, threshold);
}

static int test_short_between_rows(struct cts_device *cts_dev,
        u16 *test_result, u16 threshold)
{
    int ret;
    int loopcnt;

    cts_info("Test short between rows");

    ret = set_short_test_type(cts_dev, CTS_SHORT_TEST_BETWEEN_ROWS);
    if (ret) {
        cts_err("Set short test type to BETWEEN_ROWS failed %d", ret);
        return ret;
    }

    loopcnt = cts_dev->hwdata->num_row;
    while (loopcnt > 1) {
        ret = wait_test_complete(cts_dev, 0);
        if (ret) {
            cts_err("Wait test complete failed %d", ret);
            return ret;
        }

        ret = get_test_result(cts_dev, test_result);
        if (ret) {
            cts_err("Read test result failed %d", ret);
            return ret;
        }

        dump_test_data(cts_dev, "Row-short", test_result);

        if ((ret = validate_test_result(cts_dev,
                    "Row-short", test_result,threshold)) != 0) {
            break;
        }

        loopcnt += loopcnt % 2;
        loopcnt = loopcnt >> 1;
    }

    return ret;
}

static int prepare_test(struct cts_device *cts_dev)
{
    int ret;

    cts_info("Prepare test");

    cts_plat_reset_device(cts_dev->pdata);

    ret = disable_fw_esd_protection(cts_dev);
    if (ret) {
        cts_err("Disable firmware ESD protection failed %d", ret);
        return ret;
    }

    ret = disable_fw_monitor_mode(cts_dev);
    if (ret) {
        cts_err("Disable firmware monitor mode failed %d", ret);
        return ret;
    }

    ret = disable_fw_auto_compensate(cts_dev);
    if (ret) {
        cts_err("Disable firmware auto compensate failed %d", ret);
        return ret;
    }

    ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_FACTORY);
    if (ret) {
        cts_err("Set firmware work mode to WORK_MODE_FACTORY failed %d", ret);
        return ret;
    }

    cts_dev->rtdata.testing = true;

    return 0;
}

static void post_test(struct cts_device *cts_dev)
{
    int ret;

    cts_info("Post test");

    cts_plat_reset_device(cts_dev->pdata);

    ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_NORMAL);
    if (ret) {
        cts_err("Set firmware work mode to WORK_MODE_NORMAL failed %d", ret);
    }

    cts_dev->rtdata.testing = false;
}

/* Return 0 success 
          negative value while error occurs
          positive value means how many nodes fail */
int cts_short_test(struct cts_device *cts_dev, u16 threshold)
{
    int  ret;
    u16 *test_result = NULL;
    bool recovery_display_state = false;
    u8   need_display_on;
    u8   feature_ver;

    cts_info("Start short test, threshold = %u", threshold);

    test_result = (u16 *)kmalloc(TEST_RESULT_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (test_result == NULL) {
        cts_err("Allocate test result buffer failed");
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);

    ret = prepare_test(cts_dev);
    if (ret) {
        cts_err("Prepare test failed %d", ret);
        goto err_free_test_result;
    }

    ret = cts_sram_readb(cts_dev, 0xE8, &feature_ver);
    if (ret) {
        cts_err("Read firmware feature version failed %d", ret);
        goto err_free_test_result;
    }
    cts_info("Feature version: %u", feature_ver);

    if (feature_ver > 0) {
        ret = test_short_to_gnd(cts_dev, feature_ver, test_result, threshold);
    } else {
        ret = test_short_to_gnd_legacy(cts_dev, test_result, threshold);
    }
    if (ret) {
        cts_err("Short to GND test failed %d", ret);
        goto err_free_test_result;
    }

    ret = cts_fw_reg_readb(cts_dev, 0x8000 + 163, &need_display_on);
    if (ret) {
        cts_err("Read need display on register failed %d", ret);
        goto err_free_test_result;
    }

    if (need_display_on == 0) {
        ret = set_display_state(cts_dev, false);
        if (ret) {
            cts_err("Set display state to SLEEP failed %d", ret);
            goto err_free_test_result;
        }
        recovery_display_state = true;
    }

    ret = test_short_between_cols(cts_dev,
            test_result, threshold, need_display_on == 0);
    if (ret) {
        cts_err("Short between columns test failed %d", ret);
        goto err_recovery_display_state;
    }

    ret = test_short_between_rows(cts_dev, test_result, threshold);
    if (ret) {
        cts_err("Short between columns test failed %d", ret);
        goto err_recovery_display_state;
    }

err_recovery_display_state:
    if (recovery_display_state) {
        int r = set_display_state(cts_dev, true);
        if (r) {
            cts_err("Set display state to ACTIVE failed %d", r);
        }
    }
err_free_test_result:
    kfree(test_result);
    post_test(cts_dev);

    cts_unlock_device(cts_dev);

    cts_info("Short test %s", ret ? "FAILED" : "PASSED");

    return ret;
}

/* Return 0 success 
          negative value while error occurs
          positive value means how many nodes fail */
int cts_open_test(struct cts_device *cts_dev, u16 threshold)
{
    int  ret;
    u16 *test_result = NULL;
    bool recovery_display_state = false;
    u8   need_display_on;

    cts_info("Start open test, threshold = %u", threshold);

    test_result = (u16 *)kmalloc(TEST_RESULT_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (test_result == NULL) {
        cts_err("Allocate memory for test result faild");
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = prepare_test(cts_dev);
    if (ret) {
        cts_err("Prepare test failed %d", ret);
        goto err_free_test_result;
    }

    ret = cts_fw_reg_readb(cts_dev, 0x8000 + 163, &need_display_on);
    if (ret) {
        cts_err("Read need display on register failed %d", ret);
        goto err_free_test_result;
    }

    if (need_display_on == 0) {
        ret = set_display_state(cts_dev, false);
        if (ret) {
            cts_err("Set display state to SLEEP failed %d", ret);
            goto err_free_test_result;
        }
        recovery_display_state = true;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_RECOVERY_TX_VOL);
    if (ret) {
        cts_err("Recovery tx voltage failed %d", ret);
        goto err_recovery_display_state;
    }

    ret = set_fw_test_type(cts_dev, CTS_TEST_OPEN);
    if (ret) {
        cts_err("Set test type to OPEN_TEST failed %d", ret);
        goto err_recovery_display_state;
    }

    ret = set_fw_work_mode(cts_dev, CTS_FIRMWARE_WORK_MODE_TEST);
    if (ret) {
        cts_err("Set firmware work mode to WORK_MODE_TEST failed %d", ret);
        goto err_recovery_display_state;
    }

    ret = wait_test_complete(cts_dev, 2);
    if (ret) {
        cts_err("Wait test complete failed %d", ret);
        goto err_recovery_display_state;
    }

    ret = get_test_result(cts_dev, test_result);
    if (ret) {
        cts_err("Read test result failed %d", ret);
        goto err_recovery_display_state;
    }

    dump_test_data(cts_dev, "Open-circuit", test_result);

    ret = validate_test_result(cts_dev, "Open-circuit", test_result, threshold);

err_recovery_display_state:
    if (recovery_display_state) {
        int r = set_display_state(cts_dev, true);
        if (r) {
            cts_err("Set display state to ACTIVE failed %d", r);
        }
    }
err_free_test_result:
    kfree(test_result);
    post_test(cts_dev);

    cts_unlock_device(cts_dev);
    cts_info("Open test %s", ret ? "FAILED" : "PASSED");

    return ret;
}

#ifdef CFG_CTS_HAS_RESET_PIN
int cts_reset_test(struct cts_device *cts_dev)
{
    int ret;
    int val = 0;

    cts_lock_device(cts_dev);    
    ret = cts_stop_device(cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
    }    
    cts_plat_set_reset(cts_dev->pdata, 0);
    mdelay(50);
#ifdef CONFIG_CTS_I2C_HOST
    /* Check whether device is in normal mode */
    if (!cts_plat_is_i2c_online(cts_dev->pdata,
        CTS_DEV_NORMAL_MODE_I2CADDR)) {
#else
	if (!cts_plat_is_normal_mode(cts_dev->pdata)) {
#endif /* CONFIG_CTS_I2C_HOST */
		val++;
	}
    cts_plat_set_reset(cts_dev->pdata, 1);
    mdelay(50);
#ifdef CONFIG_CTS_I2C_HOST
    /* Check whether device is in normal mode */
    if (cts_plat_is_i2c_online(cts_dev->pdata,
        CTS_DEV_NORMAL_MODE_I2CADDR)) {
#else
	if (cts_plat_is_normal_mode(cts_dev->pdata)) {
#endif /* CONFIG_CTS_I2C_HOST */
		val ++;
	}
    ret = cts_start_device(cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
    }    
#ifdef CONFIG_CTS_CHARGER_DETECT
    if (cts_is_charger_exist(cts_dev)) {
        cts_charger_plugin(cts_dev);
    }
#endif /* CONFIG_CTS_CHARGER_DETECT */

#ifdef CONFIG_CTS_GLOVE
    if (cts_is_glove_enabled(cts_dev)) {
        cts_enter_glove_mode(cts_dev);
    }    
#endif

#ifdef CFG_CTS_FW_LOG_REDIRECT
    if (cts_is_fw_log_redirect(cts_dev)) {
        cts_enable_fw_log_redirect(cts_dev);    
    }    	
#endif

    cts_unlock_device(cts_dev);    
    if (val == 2) {
        if (!cts_dev->rtdata.program_mode) {
            cts_set_normal_addr(cts_dev);
        }
        return 0;
    }        
    return -1;
} 
#endif        	

int cts_compensate_cap_test(struct cts_device *cts_dev, u8 min_thres, u8 max_thres)
{
    u8 *cap = NULL;
    int ret = 0;
    bool data_valid = false;
    u8 r, c;

    cts_info("Compensate Cap test, threshold [%u, %u]",
		min_thres, max_thres);

    cap = kzalloc(cts_dev->hwdata->num_row * cts_dev->hwdata->num_col, GFP_KERNEL);
    if (cap == NULL) {
        cts_err("Allocate mem for cap failed %d", ret);
		return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_compensate_cap(cts_dev);
    if (ret) {
        cts_err("Enable get compensate cap failed %d", ret);
        goto unlock_device;
    }

    ret = cts_get_compensate_cap(cts_dev, cap);
    if (ret) {
        cts_err("Get compensate cap failed %d", ret);
        goto unlock_device;
    }

    data_valid = true;

    {
	int r = cts_disable_get_compensate_cap(cts_dev);
	if (r) {
	    cts_err("Disable get compensate cap failed %d", r);
	}
    }
unlock_device:
    cts_unlock_device(cts_dev);

    if (data_valid) {
	dump_comp_cap(cts_dev, cap);
        ret = 0;
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                if (cap[r * cts_dev->fwdata.cols + c] > max_thres ||
                     cap[r * cts_dev->fwdata.cols + c] < min_thres)
                {
                    ret++;
                }        
            }
        }
    }
	kfree(cap);
    return ret;
} 

int cts_rawdata_test(struct cts_device *cts_dev, u16 min_thres, u16 max_thres)
{
	int ret;
	bool data_valid = false;
	u8 r, c;
	u16 *rawdata;
	int i;
	
	cts_info("cts_rawdata_test");
    rawdata = (u16 *)kmalloc(RAWDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata == NULL) {
        cts_err("Allocate memory for rawdata failed");
		return -ENOMEM;
    }

	cts_lock_device(cts_dev);
	ret = cts_enable_get_rawdata(cts_dev);
	if (ret) {
		cts_err("Enable get rawdata failed %d", ret);
		goto unlock_device;
	}

	ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
	if (ret) {
		cts_err("send quit gesture monitor failed %d", ret);
	}

	msleep(50);
	for (i = 0; i < 3; i++) {
		ret = cts_get_rawdata(cts_dev, rawdata);
		if (ret) {
			cts_err("get rawdata failed %d", ret);
		} else {
			break;
		}	
		mdelay(30);
	}
	ret = cts_disable_get_rawdata(cts_dev);
	if (ret) {
		cts_err("Disable get rawdata failed %d", ret);
		goto unlock_device;
	}
	if (i < 3) {
		data_valid = true;
	}	
unlock_device:
	cts_unlock_device(cts_dev);

	if (data_valid) {
		dump_test_data(cts_dev, "Rawdata", rawdata);
		ret = 0;
		for (r = 0; r < cts_dev->fwdata.rows; r++) {
			for (c = 0; c < cts_dev->fwdata.cols; c++) {
				if (rawdata[r * cts_dev->fwdata.cols + c] > max_thres 
					|| rawdata[r * cts_dev->fwdata.cols + c] < min_thres)
				{
					ret++;
					break;	  
				}		 
			}
		}
	}
	kfree(rawdata);
	return ret;
}

