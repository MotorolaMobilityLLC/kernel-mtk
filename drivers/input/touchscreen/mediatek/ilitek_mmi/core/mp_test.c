/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/fd.h>
#include <linux/file.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include "../common.h"
#include "../platform.h"
#include "i2c.h"
#include "config.h"
#include "mp_test.h"
#include "protocol.h"
#include "parser.h"
#include "finger_report.h"
#include "flash.h"

#define RETRY_COUNT 3

#define INT_CHECK 0
#define POLL_CHECK 1
#define DELAY_CHECK 2
#define CHECK_BUSY_TYPE POLL_CHECK

#define NORMAL_CSV_PASS_NAME		"mp_pass"
#define NORMAL_CSV_FAIL_NAME		"mp_fail"

#define CSV_FILE_SIZE   (1024 * 1024)

#define Mathabs(x) ({						\
		long ret;							\
		if (sizeof(x) == sizeof(long)) {	\
		long __x = (x);						\
		ret = (__x < 0) ? -__x : __x;		\
		} else {							\
		int __x = (x);						\
		ret = (__x < 0) ? -__x : __x;		\
		}									\
		ret;								\
	})

#define DUMP(level, fmt, arg...)		\
	do {								\
		if (level & ipio_debug_level)	\
		printk(KERN_CONT fmt, ##arg);	\
	} while (0)


enum open_test_node_type {
	NO_COMPARE = 0x00,  /*Not A Area, No Compare  */
	AA_Area = 0x01,	    /*AA Area, Compare using Charge_AA  */
	Border_Area = 0x02, /*Border Area, Compare using Charge_Border  */
	Notch = 0x04,       /*Notch Area, Compare using Charge_Notch  */
	Round_Corner = 0x08,/* Round Corner, No Compare */
	Skip_Micro = 0x10   /* Skip_Micro, No Compare */
};

struct open_test_c_spec {
	int tvch;
	int tvcl;
	int gain;
} open_c_spec;

struct mp_move_code_data {
	uint32_t overlay_start_addr;
	uint32_t overlay_end_addr;
	uint32_t mp_flash_addr;
	uint32_t mp_size;
	uint8_t dma_trigger_enable;
} mp_move;

/* You must declare a new test in this struct before running a new process of mp test */
struct mp_test_items tItems[] = {
	{.id = 0, .name = "mutual_dac", .desp = "calibration data(dac)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 1, .name = "mutual_bg", .desp = "baseline data(bg)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 2, .name = "mutual_signal", .desp = "untouch signal data(bg-raw-4096) - mutual", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 3, .name = "mutual_no_bk", .desp = "raw data(no bk)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 4, .name = "mutual_has_bk", .desp = "raw data(have bk)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 5, .name = "mutual_bk_dac", .desp = "manual bk data(mutual)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 6, .name = "self_dac", .desp = "calibration data(dac) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 7, .name = "self_bg", .desp = "baselin data(bg,self_tx,self_r)", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 8, .name = "self_signal", .desp = "untouch signal data(bg-raw-4096) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 9, .name = "self_no_bk", .desp = "raw data(no bk) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 10, .name = "self_has_bk", .desp = "raw data(have bk) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 11, .name = "self_bk_dac", .desp = "manual bk dac data(self_tx,self_rx)", .result = "FAIL", .catalog = SELF_TEST},
	{.id = 12, .name = "key_dac", .desp = "calibration data(dac/icon)", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 13, .name = "key_bg", .desp = "key baseline data", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 14, .name = "key_no_bk", .desp = "key raw data", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 15, .name = "key_has_bk", .desp = "key raw bk dac", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 16, .name = "key_open", .desp = "key raw open test", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 17, .name = "key_short", .desp = "key raw short test", .result = "FAIL", .catalog = KEY_TEST},
	{.id = 18, .name = "st_dac", .desp = "st calibration data(dac)", .result = "FAIL", .catalog = ST_TEST},
	{.id = 19, .name = "st_bg", .desp = "st baseline data(bg)", .result = "FAIL", .catalog = ST_TEST},
	{.id = 20, .name = "st_no_bk", .desp = "st raw data(no bk)", .result = "FAIL", .catalog = ST_TEST},
	{.id = 21, .name = "st_has_bk", .desp = "st raw(have bk)", .result = "FAIL", .catalog = ST_TEST},
	{.id = 22, .name = "st_open", .desp = "st open data", .result = "FAIL", .catalog = ST_TEST},
	{.id = 23, .name = "tx_short", .desp = "tx short test", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 24, .name = "rx_short", .desp = "short test -ili9881", .result = "FAIL", .catalog = SHORT_TEST},
	{.id = 25, .name = "rx_open", .desp = "rx open", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 26, .name = "cm_data", .desp = "untouch cm data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 27, .name = "cs_data", .desp = "untouch cs data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 28, .name = "tx_rx_delta", .desp = "tx/rx delta", .result = "FAIL", .catalog = TX_RX_DELTA},
	{.id = 29, .name = "p2p", .desp = "untouch peak to peak", .result = "FAIL", .catalog = UNTOUCH_P2P},
	{.id = 30, .name = "pixel_no_bk", .desp = "pixel raw (no bk)", .result = "FAIL", .catalog = PIXEL},
	{.id = 31, .name = "pixel_has_bk", .desp = "pixel raw (have bk)", .result = "FAIL", .catalog = PIXEL},
	{.id = 32, .name = "open_integration", .desp = "open test(integration)", .result = "FAIL", .catalog = OPEN_TEST},
	{.id = 33, .name = "open_cap", .desp = "open test(cap)", .result = "FAIL", .catalog = OPEN_TEST},

	/* New test items for protocol 5.4.0 as below */
	{.id = 34, .name = "noise_peak_to_peak_ic", .desp = "noise peak to peak(ic only)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 35, .name = "noise_peak_to_peak_panel", .desp = "noise peak to peak(with panel)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 36, .name = "noise_peak_to_peak_ic_lcm_off", .desp = "noise peak to peak(ic only) (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 37, .name = "noise_peak_to_peak_panel_lcm_off", .desp = "noise peak to peak(with panel) (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 38, .name = "mutual_no_bk_lcm_off", .desp = "raw data(no bk) (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 39, .name = "mutual_has_bk_lcm_off", .desp = "raw data(have bk) (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 40, .name = "open_integration_sp", .desp = "open test(integration)_sp", .result = "FAIL", .catalog = OPEN_TEST},
	{.id = 41, .name = "doze_raw", .desp = "doze raw data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 42, .name = "doze_p2p", .desp = "doze peak to peak", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 43, .name = "doze_raw_td_lcm_off", .desp = "raw data_td (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.id = 44, .name = "doze_p2p_td_lcm_off", .desp = "peak to peak_td (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.id = 45, .name = "rx_short", .desp = "short test", .result = "FAIL", .catalog = SHORT_TEST},
	{.id = 46, .name = "open test_c", .desp = "open test_c", .result = "FAIL", .catalog = OPEN_TEST},
	{.id = 47, .name = "touch deltac", .desp = "touch deltac", .result = "FAIL", .catalog = MUTUAL_TEST},
};
int32_t *frame_buf;
int32_t *key_buf;
/* For printing open_test_sp data */
int32_t *frame1_cbk700 = NULL, *frame1_cbk250 = NULL, *frame1_cbk200 = NULL;
int32_t *cap_dac = NULL, *cap_raw = NULL;
struct core_mp_test_data *core_mp;

void dump_data(void *data, int type, int len, int row_len, const char *name)
{
	int i, row = 31;
	uint8_t *p8 = NULL;
	int32_t *p32 = NULL;

	if (row_len > 0)
		row = row_len;

	if (ipio_debug_level & DEBUG_MP_TEST) {
		if (data == NULL) {
			ipio_err("The data going to dump is NULL\n");
			return;
		}

		printk(KERN_CONT "\n\n");
		printk(KERN_CONT "ILITEK: Dump %s data\n", name);
		printk(KERN_CONT "ILITEK: ");

		if (type == 8)
			p8 = (uint8_t *) data;
		if (type == 32 || type == 10)
			p32 = (int32_t *) data;

		for (i = 0; i < len; i++) {
			if (type == 8)
				printk(KERN_CONT " %4x ", p8[i]);
			else if (type == 32)
				printk(KERN_CONT " %4x ", p32[i]);
			else if (type == 10)
				printk(KERN_CONT " %4d ", p32[i]);
			if ((i % row) == row - 1) {
				printk(KERN_CONT "\n");
				printk(KERN_CONT "ILITEK: ");
			}
		}
		printk(KERN_CONT "\n\n");
	}
}
EXPORT_SYMBOL(dump_data);

static char *get_date_time_str(void)
{
	struct timespec now_time;
	struct rtc_time rtc_now_time;
	static char time_data_buf[128] = { 0 };

	getnstimeofday(&now_time);
	rtc_time_to_tm(now_time.tv_sec, &rtc_now_time);
	sprintf(time_data_buf, "%04d%02d%02d-%02d%02d%02d",
		(rtc_now_time.tm_year + 1900), rtc_now_time.tm_mon + 1,
		rtc_now_time.tm_mday, rtc_now_time.tm_hour, rtc_now_time.tm_min,
		rtc_now_time.tm_sec);

	return time_data_buf;
}

static void mp_print_csv_header(char *csv, int *csv_len, int *csv_line)
{
	int i, tmp_len = *csv_len, tmp_line = *csv_line;

	/* header must has 19 line*/
	tmp_len += sprintf(csv + tmp_len, "==============================================================================\n");
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "ILITek C-TP Utility V%s  %x : Driver Sensor Test\n", DRIVER_VERSION, core_config->chip_pid);
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "Confidentiality Notice:\n");
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "Any information of this tool is confidential and privileged.\n");
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "@ ILI TECHNOLOGY CORP. All Rights Reserved.\n");
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "==============================================================================\n");
	tmp_line++;

	if (protocol->mid >= 0x3) {
		/*line7*/
		tmp_len += sprintf(csv + tmp_len, "Firmware Version ,V%d.%d.%d.%d\n", core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3], core_config->firmware_ver[4]);
	} else {
		tmp_len += sprintf(csv + tmp_len, "Firmware Version ,V%d.%d.%d\n", core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3]);
	}

	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "Panel information ,XCH=%d, YCH=%d\n", core_mp->xch_len, core_mp->ych_len);
	tmp_line++;
	tmp_len += sprintf(csv + tmp_len, "Test Item:\n");
	tmp_line++;

	for (i = 0; i < ARRAY_SIZE(tItems); i++) {
		if (tItems[i].run == 1) {
			tmp_len += sprintf(csv + tmp_len, "	  ---%s\n", tItems[i].desp);
			tmp_line++;
		}
	}

	while (tmp_line < 19) {
		tmp_len += sprintf(csv + tmp_len, "\n");
		tmp_line++;
	}

	tmp_len += sprintf(csv + tmp_len, "==============================================================================\n");

	*csv_len = tmp_len;
	*csv_line = tmp_line;
}

static void mp_print_csv_tail(char *csv, int *csv_len)
{
	int i, tmp_len = *csv_len;;

	tmp_len += sprintf(csv + tmp_len, "==============================================================================\n");
	tmp_len += sprintf(csv + tmp_len, "Result_Summary           \n");

	for (i = 0; i < ARRAY_SIZE(tItems); i++) {
		if (tItems[i].run) {
			if (tItems[i].item_result == MP_PASS)
				tmp_len += sprintf(csv + tmp_len, "   {%s}     ,OK\n", tItems[i].desp);
			else
				tmp_len += sprintf(csv + tmp_len, "   {%s}     ,NG\n", tItems[i].desp);
		}
	}

	*csv_len = tmp_len;
}

static void mp_print_csv_cdc_cmd(char *csv, int *csv_len, int index)
{
	int i, slen = 0, tmp_len = *csv_len;
	char str[128] = {0};
	char *open_sp_cmd[] = {"open dac", "open raw1", "open raw2", "open raw3"};
	char *open_c_cmd[] = {"open cap1 dac", "open cap1 raw", "open cap2 dac", "open cap2 raw"};

	if (strcmp(tItems[index].desp, "open test(integration)_sp") == 0) {
		for (i = 0; i < ARRAY_SIZE(open_sp_cmd); i++) {
			slen = core_parser_get_int_data("pv5_4 command", open_sp_cmd[i], str);
			if (slen < 0) {
				ipio_err("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			} else {
				tmp_len += sprintf(csv + tmp_len, "%s = ,%s\n", open_sp_cmd[i], str);
			}
		}
	} else if (strcmp(tItems[index].desp, "open test_c") == 0) {
		for (i = 0; i < ARRAY_SIZE(open_c_cmd); i++) {
			slen = core_parser_get_int_data("pv5_4 command", open_c_cmd[i], str);
			if (slen < 0) {
				ipio_err("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			} else {
				tmp_len += sprintf(csv + tmp_len, "%s = ,%s\n", open_c_cmd[i], str);
			}
		}
	} else {
		slen = core_parser_get_int_data("pv5_4 command", tItems[index].desp, str);
		if (slen < 0) {
			ipio_err("Failed to get CDC command %s from ini\n", tItems[index].desp);
		} else {
			tmp_len += sprintf(csv + tmp_len, "CDC command = ,%s\n", str);
		}
	}

	*csv_len = tmp_len;

}

static void dump_benchmark_data(int32_t *max_ptr, int32_t *min_ptr)
{
	int i;

	if (ipio_debug_level & DEBUG_MP_TEST) {
		ipio_info("Dump Benchmark Max\n");

		for (i = 0; i < core_mp->frame_len; i++) {
			printk(KERN_CONT "%d, ", max_ptr[i]);
			if (i % core_mp->xch_len == core_mp->xch_len - 1)
				printk(KERN_CONT "\n");
		}

		ipio_info(KERN_CONT "Dump Denchmark Min\n");

		for (i = 0; i < core_mp->frame_len; i++) {
			printk(KERN_CONT "%d, ", min_ptr[i]);
			if (i % core_mp->xch_len == core_mp->xch_len - 1)
				printk(KERN_CONT "\n");
		}
	}
}

void dump_node_type_buffer(int32_t *node_ptr, uint8_t *name)
{
	int i;

	if (ipio_debug_level & DEBUG_MP_TEST) {
		ipio_info("Dump NodeType\n");
		for (i = 0; i < core_mp->frame_len; i++) {
			printk(KERN_CONT "%d, ", node_ptr[i]);
			if (i % core_mp->xch_len == core_mp->xch_len-1)
				printk(KERN_CONT "\n");
		}
	}
}

static void mp_compare_cdc_result(int index, int32_t *tmp, int32_t *max_ts, int32_t *min_ts, int *result)
{
	int i;

	if (ERR_ALLOC_MEM(tmp)) {
		ipio_err("The data of test item is null (%p)\n", tmp);
		*result = MP_FAIL;
		return;
	}

	if (tItems[index].catalog == SHORT_TEST) {
		for (i = 0; i < core_mp->frame_len; i++) {
			if (tmp[i] < min_ts[i]) {
				*result = MP_FAIL;
				return;
			}
		}
	} else {
		for (i = 0; i < core_mp->frame_len; i++) {
			if (tmp[i] > max_ts[i] || tmp[i] < min_ts[i]) {
				*result = MP_FAIL;
				return;
			}
		}
	}
}

static void mp_compare_cdc_show_result(int index, int32_t *tmp, char *csv, int *csv_len,
		int type, int32_t *max_ts, int32_t *min_ts, const char *desp)
{
	int x, y, tmp_len = *csv_len;
	int mp_result = MP_PASS;

	if (ERR_ALLOC_MEM(tmp)) {
		ipio_err("The data of test item is null (%p)\n", tmp);
		mp_result = MP_FAIL;
		goto out;
	}

	/* print X raw only */
	for (x = 0; x < core_mp->xch_len; x++) {
		if (x == 0) {
			DUMP(DEBUG_MP_TEST, "\n %s ", desp);
			tmp_len += sprintf(csv + tmp_len, "\n      %s ,", desp);
		}

		DUMP(DEBUG_MP_TEST, "  X_%d  ,", (x+1));
		tmp_len += sprintf(csv + tmp_len, "  X_%d  ,", (x+1));
	}

	DUMP(DEBUG_MP_TEST, "\n");
	tmp_len += sprintf(csv + tmp_len, "\n");

	for (y = 0; y < core_mp->ych_len; y++) {
		DUMP(DEBUG_MP_TEST, "  Y_%d  ,", (y+1));
		tmp_len += sprintf(csv + tmp_len, "  Y_%d  ,", (y+1));

		for (x = 0; x < core_mp->xch_len; x++) {
			int shift = y * core_mp->xch_len + x;

			/* In Short teset, we only identify if its value is low than min threshold. */
			if (tItems[index].catalog == SHORT_TEST) {
				if (tmp[shift] < min_ts[shift]) {
					DUMP(DEBUG_MP_TEST, " #%7d ", tmp[shift]);
					tmp_len += sprintf(csv + tmp_len, "#%7d,", tmp[shift]);
					mp_result = MP_FAIL;
				} else {
					DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
					tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
				}
				continue;
			}

			if ((tmp[shift] <= max_ts[shift] && tmp[shift] >= min_ts[shift]) || (type != TYPE_JUGE)) {

				if ((tmp[shift] == INT_MAX || tmp[shift] == INT_MIN) && (type == TYPE_BENCHMARK)) {
					DUMP(DEBUG_MP_TEST, "%s", "BYPASS,");
					tmp_len += sprintf(csv + tmp_len, "BYPASS,");
				} else {
					DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
					tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
				}
			} else {
				if (tmp[shift] > max_ts[shift]) {
					DUMP(DEBUG_MP_TEST, " *%7d ", tmp[shift]);
					tmp_len += sprintf(csv + tmp_len, "*%7d,", tmp[shift]);
				} else {
					DUMP(DEBUG_MP_TEST, " #%7d ", tmp[shift]);
					tmp_len += sprintf(csv + tmp_len, "#%7d,", tmp[shift]);
				}
				mp_result = MP_FAIL;
			}
		}

		DUMP(DEBUG_MP_TEST, "\n");
		tmp_len += sprintf(csv + tmp_len, "\n");
	}

out:
	if (type == TYPE_JUGE) {
		if (mp_result == MP_PASS) {
			pr_info("\n Result : PASS\n");
			tmp_len += sprintf(csv + tmp_len, "Result : PASS\n");
		} else {
			pr_info("\n Result : FAIL\n");
			tmp_len += sprintf(csv + tmp_len, "Result : FAIL\n");
		}
	}

	*csv_len = tmp_len;
}

static int create_mp_test_frame_buffer(int index, int frame_count)
{
	ipio_debug(DEBUG_MP_TEST, "Create MP frame buffers (index = %d), count = %d\n"
			, index, frame_count);

	if (tItems[index].catalog == TX_RX_DELTA) {
		if (core_mp->tx_delta_buf == NULL) {
			core_mp->tx_delta_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->tx_delta_buf)) {
				ipio_err("Failed to allocate tx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp->tx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp->rx_delta_buf == NULL) {
			core_mp->rx_delta_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->rx_delta_buf)) {
				ipio_err("Failed to allocate rx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp->rx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp->tx_max_buf == NULL) {
			core_mp->tx_max_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->tx_max_buf)) {
				ipio_err("Failed to allocate tx_max_buf mem\n");
				ipio_kfree((void **)&core_mp->tx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp->tx_min_buf == NULL) {
			core_mp->tx_min_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->tx_min_buf)) {
				ipio_err("Failed to allocate tx_min_buf mem\n");
				ipio_kfree((void **)&core_mp->tx_min_buf);
				return -ENOMEM;
			}
		}

		if (core_mp->rx_max_buf == NULL) {
			core_mp->rx_max_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->rx_max_buf)) {
				ipio_err("Failed to allocate rx_max_buf mem\n");
				ipio_kfree((void **)&core_mp->rx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp->rx_min_buf == NULL) {
			core_mp->rx_min_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp->rx_min_buf)) {
				ipio_err("Failed to allocate rx_min_buf mem\n");
				ipio_kfree((void **)&core_mp->rx_min_buf);
				return -ENOMEM;
			}
		}
	} else {
		if (tItems[index].buf == NULL) {
			tItems[index].buf = vmalloc(frame_count * core_mp->frame_len * sizeof(int32_t));
			if (ERR_ALLOC_MEM(tItems[index].buf)) {
				ipio_err("Failed to allocate buf mem\n");
				ipio_kfree((void **)&tItems[index].buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].result_buf == NULL) {
			tItems[index].result_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].result_buf)) {
				ipio_err("Failed to allocate result_buf mem\n");
				ipio_kfree((void **)&tItems[index].result_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].max_buf == NULL) {
			tItems[index].max_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].max_buf)) {
				ipio_err("Failed to allocate max_buf mem\n");
				ipio_kfree((void **)&tItems[index].max_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].min_buf == NULL) {
			tItems[index].min_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].min_buf)) {
				ipio_err("Failed to allocate min_buf mem\n");
				ipio_kfree((void **)&tItems[index].min_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].spec_option == BENCHMARK) {
			if (tItems[index].bench_mark_max == NULL) {
				tItems[index].bench_mark_max = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bench_mark_max)) {
					ipio_err("Failed to allocate bench_mark_max mem\n");
					ipio_kfree((void **)&tItems[index].bench_mark_max);
					return -ENOMEM;
				}
			}
			if (tItems[index].bench_mark_min == NULL) {
				tItems[index].bench_mark_min = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bench_mark_min)) {
					ipio_err("Failed to allocate bench_mark_min mem\n");
					ipio_kfree((void **)&tItems[index].bench_mark_min);
					return -ENOMEM;
				}
			}
		}
	}
	return 0;
}

static int allnode_key_cdc_data(int index)
{
	int i, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	uint8_t cmd[3] = { 0 };
	uint8_t *ori = NULL;

	len = core_mp->key_len * 2;

	ipio_debug(DEBUG_MP_TEST, "Read key's length = %d\n", len);
	ipio_debug(DEBUG_MP_TEST, "core_mp->key_len = %d\n", core_mp->key_len);

	if (len <= 0) {
		ipio_err("Length is invalid\n");
		ret = -1;
		goto out;
	}

	/* CDC init */
	cmd[0] = protocol->cmd_cdc;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	ret = core_write(core_config->slave_i2c_addr, cmd, 3);
	if (ret < 0) {
		ipio_err("I2C Write Error while initialising cdc\n");
		goto out;
	}

	mdelay(1);

	/* Check busy */
	if (core_config_check_cdc_busy(50, 50) < 0) {
		ipio_err("Check busy is timout !\n");
		ret = -1;
		goto out;
	}

	mdelay(1);

	/* Prepare to get cdc data */
	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_cdc;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
		goto out;
	}

	mdelay(1);

	/* Get original frame(cdc) data */
	ret = core_read(core_config->slave_i2c_addr, ori, len);
	if (ret < 0) {
		ipio_err("I2C Read Error while getting original cdc data\n");
		goto out;
	}

	dump_data(ori, 8, len, 0, "Key CDC original");

	if (key_buf == NULL) {
		key_buf = kcalloc(core_mp->key_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(key_buf)) {
			ipio_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(key_buf));
			goto out;
		}
	} else {
		memset(key_buf, 0x0, core_mp->key_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp->frame_len; i++) {
		if (tItems[index].cmd == protocol->key_dac) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			key_buf[i] = (inDACp + inDACn) / 2;
		}
	}

	dump_data(key_buf, 32, core_mp->frame_len, core_mp->xch_len, "Key CDC combined data");

out:
	ipio_kfree((void **)&ori);
	return ret;
}

static int mp_get_timing_info(void)
{
	int slen = 0;
	char str[256] = {0};
	uint8_t info[64] = {0};
	char *key = "timing_info_raw";

	core_mp->isLongV = 0;

	slen = core_parser_get_int_data("pv5_4 command", key, str);
	if (slen < 0)
		return -1;

	if (core_parser_get_u8_array(str, info, 16, slen) < 0)
		return -1;

	core_mp->isLongV = info[6];

	ipio_info("DDI Mode = %s\n", (core_mp->isLongV ? "Long V" : "Long H"));

	return 0;
}

static int mp_cdc_get_pv5_4_command(uint8_t *cmd, int len, int index)
{
	int slen = 0;
	char str[128] = {0};
	char *key = tItems[index].desp;

	ipio_info("Get cdc command for %s\n", key);

	slen = core_parser_get_int_data("pv5_4 command", key, str);
	if (slen < 0)
		return -1;

	if (core_parser_get_u8_array(str, cmd, 16, len) < 0)
		return -1;

	return 0;
}

static int mp_cdc_init_cmd_common(uint8_t *cmd, int len, int index)
{
	int ret = 0;

	if (protocol->major >= 5 && protocol->mid >= 4) {
		ipio_info("Get CDC command with protocol v5.4\n");
		protocol->cdc_len = 15;
		return mp_cdc_get_pv5_4_command(cmd, len, index);
	}

	cmd[0] = protocol->cmd_cdc;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	protocol->cdc_len = 3;

	if (strcmp(tItems[index].name, "open_integration") == 0)
		cmd[2] = 0x2;
	if (strcmp(tItems[index].name, "open_cap") == 0)
		cmd[2] = 0x3;

	if (tItems[index].catalog == PEAK_TO_PEAK_TEST) {
		cmd[2] = ((tItems[index].frame_count & 0xff00) >> 8);
		cmd[3] = tItems[index].frame_count & 0xff;
		cmd[4] = 0;

		protocol->cdc_len = 5;

		if (strcmp(tItems[index].name, "noise_peak_to_peak_cut") == 0)
			cmd[4] = 0x1;

		ipio_debug(DEBUG_MP_TEST, "P2P CMD: %d,%d,%d,%d,%d\n",
				cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	}

	return ret;
}

static int allnode_mutual_cdc_data(int index)
{
	static int i, ret, len;
	int inDACp = 0, inDACn = 0;
	uint8_t cmd[15] = {0};
	uint8_t *ori = NULL;

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (core_mp->xch_len * core_mp->ych_len * 2) + 2;

	ipio_debug(DEBUG_MP_TEST, "Read X/Y Channel length = %d\n", len);
	ipio_debug(DEBUG_MP_TEST, "core_mp->frame_len = %d\n", core_mp->frame_len);

	if (len <= 2) {
		ipio_err("Length is invalid\n");
		ret = -1;
		goto out;
	}

	memset(cmd, 0xFF, sizeof(cmd));

	/* CDC init */
	ret = mp_cdc_init_cmd_common(cmd, sizeof(cmd), index);
	if (ret < 0) {
		ipio_err("Failed to get cdc command\n");
		goto out;
	}

	dump_data(cmd, 8, protocol->cdc_len, 0, "Mutual CDC command");

	ret = core_write(core_config->slave_i2c_addr, cmd, protocol->cdc_len);
	if (ret < 0) {
		ipio_err("I2C Write Error while initialising cdc\n");
		goto out;
	}

	/* Check busy */
	ipio_info("Check busy method = %d\n", core_mp->busy_cdc);
	if (core_mp->busy_cdc == POLL_CHECK) {
		ret = core_config_check_cdc_busy(50, 50);
	} else if (core_mp->busy_cdc == INT_CHECK) {
		ret = core_config_check_int_status(true);
	} else if (core_mp->busy_cdc == DELAY_CHECK) {
		mdelay(600);
	}

	if (ret < 0) {
		ipio_err("Check busy timeout !\n");
		ret = -1;
		goto out;
	}

	/* Prepare to get cdc data */
	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_cdc;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	mdelay(1);

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
		goto out;
	}

	/* Get original frame(cdc) data */
	ret = core_read(core_config->slave_i2c_addr, ori, len);
	if (ret < 0) {
		ipio_err("I2C Read Error while getting original cdc data\n");
		goto out;
	}

	dump_data(ori, 8, len, 0, "Mutual CDC original");

	if (frame_buf == NULL) {
		frame_buf = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame_buf)) {
			ipio_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(frame_buf));
			goto out;
		}
	} else {
		memset(frame_buf, 0x0, core_mp->frame_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp->frame_len; i++) {
		if (tItems[index].cmd == protocol->mutual_dac) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			frame_buf[i] = (inDACp + inDACn) / 2;
		} else {
			/* H byte + L byte */
			int32_t tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];

			if ((tmp & 0x8000) == 0x8000)
				frame_buf[i] = tmp - 65536;
			else
				frame_buf[i] = tmp;

			if (strncmp(tItems[index].name, "mutual_no_bk", strlen("mutual_no_bk")) == 0 ||
				strncmp(tItems[index].name, "mutual_no_bk_lcm_off", strlen("mutual_no_bk_lcm_off")) == 0) {
				if ((core_config->chip_id == CHIP_TYPE_ILI9881 && core_config->chip_type == TYPE_H) ||
				(core_config->chip_id == CHIP_TYPE_ILI7807 && core_config->chip_type == TYPE_G)) {
					frame_buf[i] -= RAWDATA_NO_BK_DATA_SHIFT_9881H;
				} else {
					frame_buf[i] -= RAWDATA_NO_BK_DATA_SHIFT_9881F;
				}
			}
		}
	}

	dump_data(frame_buf, 32, core_mp->frame_len,  core_mp->xch_len, "Mutual CDC combined");

out:
	ipio_kfree((void **)&ori);
	return ret;
}

static void run_pixel_test(int index)
{
	int i, x, y;
	int32_t *p_comb = frame_buf;

	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
			int tmp[4] = { 0 }, max = 0;
			int shift = y * core_mp->xch_len;
			int centre = p_comb[shift + x];

			/* if its position is in corner, the number of point
			   we have to minus is around 2 to 3.  */
			if (y == 0 && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == (core_mp->ych_len - 1) && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == 0 && x == (core_mp->xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
			} else if (y == (core_mp->ych_len - 1) && x == (core_mp->xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
			} else if (y == 0 && x != 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y != 0 && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
				tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */

			} else if (y == (core_mp->ych_len - 1) && x != 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y != 0 && x == (core_mp->xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
			} else {
				/* middle minus four directions */
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[2] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[3] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			}

			max = tmp[0];

			for (i = 0; i < 4; i++) {
				if (tmp[i] > max)
					max = tmp[i];
			}

			tItems[index].buf[shift + x] = max;
		}
	}
}

static int run_open_test(int index)
{
	int i, x, y, k, ret = 0;
	int border_x[] = {-1, 0, 1, 1, 1, 0, -1, -1};
	int border_y[] = {-1, -1, -1, 0, 1, 1, 1, 0};
	int32_t *p_comb = frame_buf;

	if (strcmp(tItems[index].name, "open_integration") == 0) {
		for (i = 0; i < core_mp->frame_len; i++)
			tItems[index].buf[i] = p_comb[i];
	} else if (strcmp(tItems[index].name, "open_cap") == 0) {
		/* Each result is getting from a 3 by 3 grid depending on where the centre location is.
		   So if the centre is at corner, the number of node grabbed from a grid will be different. */
		for (y = 0; y < core_mp->ych_len; y++) {
			for (x = 0; x < core_mp->xch_len; x++) {
				int sum = 0, avg = 0, count = 0;
				int shift = y * core_mp->xch_len;
				int centre = p_comb[shift + x];

				for (k = 0; k < 8; k++) {
					if (((y + border_y[k] >= 0) && (y + border_y[k] < core_mp->ych_len)) &&
								((x + border_x[k] >= 0) && (x + border_x[k] < core_mp->xch_len))) {
						count++;
						sum += p_comb[(y + border_y[k]) * core_mp->xch_len + (x + border_x[k])];
					}
				}

				avg = (sum + centre) / (count + 1);	/* plus 1 because of centre */
				tItems[index].buf[shift + x] = (centre * 100) / avg;
			}
		}
	}
	return ret;
}

static void run_tx_rx_delta_test(int index)
{
	int x, y;
	int32_t *p_comb = frame_buf;

	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
			int shift = y * core_mp->xch_len;

			/* Tx Delta */
			if (y != (core_mp->ych_len - 1)) {
				core_mp->tx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[(shift + 1) + x]);
			}

			/* Rx Delta */
			if (x != (core_mp->xch_len - 1)) {
				core_mp->rx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[shift + (x + 1)]);
			}
		}
	}
}

static void run_untouch_p2p_test(int index)
{
	int x, y;
	int32_t *p_comb = frame_buf;

	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
			int shift = y * core_mp->xch_len;

			if (p_comb[shift + x] > tItems[index].max_buf[shift + x]) {
				tItems[index].max_buf[shift + x] = p_comb[shift + x];
			}

			if (p_comb[shift + x] < tItems[index].min_buf[shift + x]) {
				tItems[index].min_buf[shift + x] = p_comb[shift + x];
			}

			tItems[index].buf[shift + x] =
			    tItems[index].max_buf[shift + x] - tItems[index].min_buf[shift + x];
		}
	}
}

static void compare_MaxMin_result(int index, int32_t *data)
{
	int x, y;

	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
			int shift = y * core_mp->xch_len;

			if (tItems[index].catalog == UNTOUCH_P2P)
				return;
			else if (tItems[index].catalog == TX_RX_DELTA) {
				/* Tx max/min comparison */
				if (core_mp->tx_delta_buf[shift + x] < data[shift + x]) {
					core_mp->tx_max_buf[shift + x] = data[shift + x];
				}

				if (core_mp->tx_delta_buf[shift + x] > data[shift + x]) {
					core_mp->tx_min_buf[shift + x] = data[shift + x];
				}

				/* Rx max/min comparison */
				if (core_mp->rx_delta_buf[shift + x] < data[shift + x]) {
					core_mp->rx_max_buf[shift + x] = data[shift + x];
				}

				if (core_mp->rx_delta_buf[shift + x] > data[shift + x]) {
					core_mp->rx_min_buf[shift + x] = data[shift + x];
				}
			} else {
				if (tItems[index].max_buf[shift + x] < data[shift + x]) {
					tItems[index].max_buf[shift + x] = data[shift + x];
				}

				if (tItems[index].min_buf[shift + x] > data[shift + x]) {
					tItems[index].min_buf[shift + x] = data[shift + x];
				}
			}
		}
	}
}


#define ABS(a, b) ((a > b) ? (a - b) : (b - a))
#define ADDR(x, y) ((y * core_mp->xch_len) + (x))

int full_open_rate_compare(int32_t *full_open, int32_t *cbk, int x, int y, int32_t inNodeType, int full_open_rate)
{
	int ret = true;

	if ((inNodeType == NO_COMPARE) || ((inNodeType & Round_Corner) == Round_Corner)) {
		return true;
	}

	if (full_open[ADDR(x, y)] < (cbk[ADDR(x, y)] * full_open_rate / 100))
		ret = false;

	return ret;
}


int compare_charge(int32_t *charge_rate, int x, int y, int32_t *inNodeType, int Charge_AA, int Charge_Border, int Charge_Notch)
{
	int OpenThreadhold, tempY, tempX, ret, k;
	int sx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	int sy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

	ret = charge_rate[ADDR(x, y)];

	/*Setting Threadhold from node type  */

	if (charge_rate[ADDR(x, y)] == 0)
		return ret;
	else if ((inNodeType[ADDR(x, y)] & AA_Area) == AA_Area)
		OpenThreadhold = Charge_AA;
	else if ((inNodeType[ADDR(x, y)] & Border_Area) == Border_Area)
		OpenThreadhold = Charge_Border;
	else if ((inNodeType[ADDR(x, y)] & Notch) == Notch)
		OpenThreadhold = Charge_Notch;
	else
		return ret;

	/* compare carge rate with 3*3 node */
	/* by pass => 1.no compare 2.corner 3.Skip_Micro 4.full open fail node */
	for (k = 0; k < 8; k++) {
		tempX = x + sx[k];
		tempY = y + sy[k];

		if ((tempX < 0) || (tempX >= core_mp->xch_len) || (tempY < 0) || (tempY >= core_mp->ych_len)) /*out of range */
			continue;

		if ((inNodeType[ADDR(tempX, tempY)] == NO_COMPARE) || ((inNodeType[ADDR(tempX, tempY)] & Round_Corner) == Round_Corner) ||
		((inNodeType[ADDR(tempX, tempY)] & Skip_Micro) == Skip_Micro) || charge_rate[ADDR(tempX, tempY)] == 0)
			continue;

		if ((charge_rate[ADDR(tempX, tempY)]-charge_rate[ADDR(x, y)]) > OpenThreadhold)
			return OpenThreadhold;
	}
	return ret;
}
void allnode_open_cdc_result(int index, int *buf, int *dac, int *raw)
{
	int i;

	if (strcmp(tItems[index].name, "open_integration_sp") == 0) {
		for (i = 0; i < core_mp->frame_len; i++) {
			if (core_config->chip_id == CHIP_TYPE_ILI9881) {
				buf[i] = (int)((int)(dac[i] * 2 * 10000 * 161 / 100) - (int)(16384 / 2 - (int)raw[i]) * 20000 * 7 / 16384 * 36 / 10) / 31 / 2;
			} else if (core_config->chip_id == CHIP_TYPE_ILI7807) {
				buf[i] = (int)((int)(dac[i] * 2 * 10000 * 131 / 100) - (int)(16384 / 2 - (int)raw[i]) * 20000 * 7 / 16384 * 36 / 10) / 31 / 2;
			}
		}
	} else if (strcmp(tItems[index].name, "open test_c") == 0) {
		for (i = 0; i < core_mp->frame_len; i++) {
			buf[i] = (int)((int)(dac[i] * 414 * 39 / 2) + (int)(((int)raw[i] - 8192) * 36 * (7 * 100 - 22) * 10 / 16384)) /
						(open_c_spec.tvch - open_c_spec.tvcl) / 100 / open_c_spec.gain;
		}
	}

}

/* This will be merged to allnode_mutual_cdc_data in next version */
int allnode_open_cdc_data(int mode, int *buf)
{
	int i = 0, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	uint8_t cmd[15] = {0};
	uint8_t *ori = NULL;
	char str[128] = {0};
	char tmp[128] = {0};
	char *key[] = {"open dac", "open raw1", "open raw2", "open raw3",
					"open cap1 dac", "open cap1 raw"};

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (core_mp->xch_len * core_mp->ych_len * 2) + 2;

	ipio_debug(DEBUG_MP_TEST, "Read X/Y Channel length = %d\n", len);
	ipio_debug(DEBUG_MP_TEST, "core_mp->frame_len = %d, mode= %d\n", core_mp->frame_len, mode);

	if (len <= 2) {
		ipio_err("Length is invalid\n");
		ret = -1;
		goto out;
	}

	/* CDC init. Read command from ini file */
	ret = core_parser_get_int_data("pv5_4 command", key[mode], str);
	if (ret < 0) {
		ipio_err("Failed to parse PV54 command, ret = %d\n", ret);
		goto out;
	}

	strncpy(tmp, str, ret);
	core_parser_get_u8_array(tmp, cmd, 16, sizeof(cmd));

	dump_data(cmd, 8, sizeof(cmd), 0, "Open SP command");

	ret = core_write(core_config->slave_i2c_addr, cmd, protocol->cdc_len);
	if (ret < 0) {
		ipio_err("I2C Write Error while initialising cdc\n");
		goto out;
	}

	/* Check busy */
	ipio_info("Check busy method = %d\n", core_mp->busy_cdc);
	if (core_mp->busy_cdc == POLL_CHECK) {
		ret = core_config_check_cdc_busy(50, 50);
	} else if (core_mp->busy_cdc == INT_CHECK) {
		ret = core_config_check_int_status(true);
	} else if (core_mp->busy_cdc == DELAY_CHECK) {
		mdelay(600);
	}

	if (ret < 0) {
		ipio_err("Check busy timeout !\n");
		ret = -1;
		goto out;
	}

	/* Prepare to get cdc data */
	cmd[0] = protocol->cmd_read_ctrl;
	cmd[1] = protocol->cmd_get_cdc;

	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	mdelay(1);

	ret = core_write(core_config->slave_i2c_addr, &cmd[1], 1);
	if (ret < 0) {
		ipio_err("I2C Write Error\n");
		goto out;
	}

	mdelay(1);

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
		goto out;
	}

	/* Get original frame(cdc) data */
	ret = core_read(core_config->slave_i2c_addr, ori, len);
	if (ret < 0) {
		ipio_err("I2C Read Error while getting original cdc data\n");
		goto out;
	}

	dump_data(ori, 8, len, 0, "Open SP CDC original");

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp->frame_len; i++) {
		if ((mode == 0) || (mode == 4)) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			if (mode == 0)
				buf[i] = (inDACp + inDACn) / 2;
			else
				buf[i] = inDACp + inDACn;

		} else {
			/* H byte + L byte */
			int32_t tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];
			if ((tmp & 0x8000) == 0x8000)
				buf[i] = tmp - 65536;
			else
				buf[i] = tmp;

		}
	}
	dump_data(buf, 10, core_mp->frame_len,  core_mp->xch_len, "Open SP CDC combined");
out:
	ipio_kfree((void **)&ori);

	return ret;
}

static int open_test_cap(int index)
{
	struct mp_test_open_c open[tItems[index].frame_count];
	int i = 0, x = 0, y = 0, ret = 0, addr = 0;
	char str[512] = { 0 };

	if (tItems[index].frame_count <= 0) {
		ipio_err("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0)
		goto out;

	if (cap_dac == NULL) {
		cap_dac = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(cap_dac)) {
			ipio_err("Failed to allocate cap_dac buffer\n");
			return -ENOMEM;
		}
	} else {
		memset(cap_dac, 0x0, core_mp->frame_len);
	}

	if (cap_raw == NULL) {
		cap_raw = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(cap_raw)) {
			ipio_err("Failed to allocate cap_raw buffer\n");
			ipio_kfree((void **)&cap_dac);
			return -ENOMEM;
		}
	} else {
		memset(cap_raw, 0x0, core_mp->frame_len);
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
				tItems[index].max_buf[y * core_mp->xch_len + x] = INT_MIN;
				tItems[index].min_buf[y * core_mp->xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		core_parser_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
							tItems[index].type_option, tItems[index].desp, core_mp->frame_len);
		if (ipio_debug_level && DEBUG_PARSER > 0)
			dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	ret = core_parser_get_int_data(tItems[index].desp, "gain", str);
	if (ret || ret == 0)
		open_c_spec.gain = katoi(str);

	ret = core_parser_get_int_data(tItems[index].desp, "tvch", str);
	if (ret || ret == 0)
		open_c_spec.tvch = katoi(str);

	ret = core_parser_get_int_data(tItems[index].desp, "tvcl", str);
	if (ret || ret == 0)
		open_c_spec.tvcl = katoi(str);

	if (ret < 0) {
		ipio_err("Failed to get parameters from ini file\n");
		goto out;
	}

	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].cap_dac = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].cap_raw = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].dcl_cap = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	}

	for (i = 0; i < tItems[index].frame_count; i++) {
		ret = allnode_open_cdc_data(4, open[i].cap_dac);
		if (ret < 0) {
			ipio_err("Failed to get Open CAP DAC data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(5, open[i].cap_raw);
		if (ret < 0) {
			ipio_err("Failed to get Open CAP RAW data, %d\n", ret);
			goto out;
		}

		allnode_open_cdc_result(index, open[i].dcl_cap, open[i].cap_dac, open[i].cap_raw);

		/* record fist frame for debug */
		if (i == 0) {
			ipio_memcpy(cap_dac, open[i].cap_dac, core_mp->frame_len * sizeof(int32_t), core_mp->frame_len * sizeof(int32_t));
			ipio_memcpy(cap_raw, open[i].cap_raw, core_mp->frame_len * sizeof(int32_t), core_mp->frame_len * sizeof(int32_t));
		}

		dump_data(open[i].dcl_cap, 10, core_mp->frame_len, core_mp->xch_len, "DCL_Cap");

		addr = 0;
		for (y = 0; y < core_mp->ych_len; y++) {
			for (x = 0; x < core_mp->xch_len; x++) {
				tItems[index].buf[(i * core_mp->frame_len) + addr] = open[i].dcl_cap[addr];
				addr++;
			}
		}
		compare_MaxMin_result(index, &tItems[index].buf[(i * core_mp->frame_len)]);
	}

out:

	for (i = 0; i < tItems[index].frame_count; i++) {
		ipio_kfree((void **)&open[i].cap_dac);
		ipio_kfree((void **)&open[i].cap_raw);
		ipio_kfree((void **)&open[i].dcl_cap);
	}

	return ret;
}

static int open_test_sp(int index)
{
	struct mp_test_P540_open open[tItems[index].frame_count];
	int i = 0, x = 0, y = 0, ret = 0, addr = 0; /*get_frame_cont = tItems[index].frame_count*/
	int Charge_AA = 0, Charge_Border = 0, Charge_Notch = 0, full_open_rate = 0;
	char str[512] = { 0 };

	ipio_debug(DEBUG_MP_TEST, "index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
	    index, tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

	/*
	 * We assume that users who are calling the test forget to config frame count
	 * as 1, so we just help them to set it up.
	 */
	if (tItems[index].frame_count <= 0) {
		ipio_err("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0)
		goto out;

	if (frame1_cbk700 == NULL) {
		frame1_cbk700 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk700)) {
			ipio_err("Failed to allocate frame1_cbk700 buffer\n");
			return -ENOMEM;
		}
	} else {
		memset(frame1_cbk700, 0x0, core_mp->frame_len);
	}

	if (frame1_cbk250 == NULL) {
		frame1_cbk250 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk250)) {
			ipio_err("Failed to allocate frame1_cbk250 buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			return -ENOMEM;
		}
	} else {
		memset(frame1_cbk250, 0x0, core_mp->frame_len);
	}

	if (frame1_cbk200 == NULL) {
		frame1_cbk200 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk200)) {
			ipio_err("Failed to allocate cbk buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			ipio_kfree((void **)&frame1_cbk250);
			return -ENOMEM;
		}
	} else {
		memset(frame1_cbk200, 0x0, core_mp->frame_len);
	}

	tItems[index].node_type = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(tItems[index].node_type)) {
		ipio_err("Failed to allocate node_type FRAME buffer\n");
		return -ENOMEM;
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
				tItems[index].max_buf[y * core_mp->xch_len + x] = INT_MIN;
				tItems[index].min_buf[y * core_mp->xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		core_parser_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
							tItems[index].type_option, tItems[index].desp, core_mp->frame_len);
		if (ipio_debug_level && DEBUG_PARSER > 0)
			dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	core_parser_nodetype(tItems[index].node_type, NODE_TYPE_KEY_NAME, core_mp->frame_len);
	if (ipio_debug_level && DEBUG_PARSER > 0)
		dump_node_type_buffer(tItems[index].node_type, "node type");

	ret = core_parser_get_int_data(tItems[index].desp, "charge_aa", str);
	if (ret || ret == 0)
		Charge_AA = katoi(str);

	ret = core_parser_get_int_data(tItems[index].desp, "charge_border", str);
	if (ret || ret == 0)
		Charge_Border = katoi(str);

	ret = core_parser_get_int_data(tItems[index].desp, "charge_notch", str);
	if (ret || ret == 0)
		Charge_Notch = katoi(str);

	ret = core_parser_get_int_data(tItems[index].desp, "full open", str);
	if (ret || ret == 0)
		full_open_rate = katoi(str);

	if (ret < 0) {
		ipio_err("Failed to get parameters from ini file\n");
		goto out;
	}

	ipio_debug(DEBUG_MP_TEST, "pen test frame_cont %d, AA %d, Border %d, Notch %d, full_open_rate %d \n",
			tItems[index].frame_count, Charge_AA, Charge_Border, Charge_Notch, full_open_rate);

	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].tdf_700 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].tdf_250 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].tdf_200 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].cbk_700 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].cbk_250 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].cbk_200 = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].charg_rate = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].full_Open = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
		open[i].dac = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	}

	for (i = 0; i < tItems[index].frame_count; i++) {
		ret = allnode_open_cdc_data(0, open[i].dac);
		if (ret < 0) {
			ipio_err("Failed to get Open SP DAC data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(1, open[i].tdf_700);
		if (ret < 0) {
			ipio_err("Failed to get Open SP Raw1 data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(2, open[i].tdf_250);
		if (ret < 0) {
			ipio_err("Failed to get Open SP Raw2 data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(3, open[i].tdf_200);
		if (ret < 0) {
			ipio_err("Failed to get Open SP Raw3 data, %d\n", ret);
			goto out;
		}

		allnode_open_cdc_result(index, open[i].cbk_700, open[i].dac, open[i].tdf_700);
		allnode_open_cdc_result(index, open[i].cbk_250, open[i].dac, open[i].tdf_250);
		allnode_open_cdc_result(index, open[i].cbk_200, open[i].dac, open[i].tdf_200);

		addr = 0;

		/* record fist frame for debug */
		if (i == 0) {
			ipio_memcpy(frame1_cbk700, open[i].cbk_700, core_mp->frame_len * sizeof(int32_t), core_mp->frame_len * sizeof(int32_t));
			ipio_memcpy(frame1_cbk250, open[i].cbk_250, core_mp->frame_len * sizeof(int32_t), core_mp->frame_len * sizeof(int32_t));
			ipio_memcpy(frame1_cbk200, open[i].cbk_200, core_mp->frame_len * sizeof(int32_t), core_mp->frame_len * sizeof(int32_t));
		}

		dump_data(open[i].cbk_700, 10, core_mp->frame_len, core_mp->xch_len, "cbk 700");
		dump_data(open[i].cbk_250, 10, core_mp->frame_len, core_mp->xch_len, "cbk 250");
		dump_data(open[i].cbk_200, 10, core_mp->frame_len, core_mp->xch_len, "cbk 200");

		for (y = 0; y < core_mp->ych_len; y++) {
			for (x = 0; x < core_mp->xch_len; x++) {
				open[i].charg_rate[addr] = open[i].cbk_250[addr] * 100 / open[i].cbk_700[addr];
				open[i].full_Open[addr] = open[i].cbk_700[addr] - open[i].cbk_200[addr];
				addr++;
			}
		}

		if (ipio_debug_level & DEBUG_MP_TEST) {
			dump_data(open[i].charg_rate, 10, core_mp->frame_len, core_mp->xch_len, "origin charge rate");
			dump_data(open[i].full_Open, 10, core_mp->frame_len, core_mp->xch_len, "origin full open");
		}

		addr = 0;
		for (y = 0; y < core_mp->ych_len; y++) {
			for (x = 0; x < core_mp->xch_len; x++) {
						if (full_open_rate_compare(open[i].full_Open, open[i].cbk_700, x, y, tItems[index].node_type[addr], full_open_rate) == false) {
							tItems[index].buf[(i * core_mp->frame_len) + addr] = 0;
							open[i].charg_rate[addr] = 0;
						}
				addr++;
			}
		}

		if (ipio_debug_level & DEBUG_MP_TEST)
			dump_data(open[i].charg_rate, 10, core_mp->frame_len, core_mp->xch_len, "charg_rate data after full_open_rate_compare");

		addr = 0;
		for (y = 0; y < core_mp->ych_len; y++) {
			for (x = 0; x < core_mp->xch_len; x++) {
						tItems[index].buf[(i * core_mp->frame_len) + addr] = compare_charge(open[i].charg_rate, x, y, tItems[index].node_type, Charge_AA, Charge_Border, Charge_Notch);
						addr++;
			}
		}

		if (ipio_debug_level & DEBUG_MP_TEST)
			dump_data(&tItems[index].buf[(i * core_mp->frame_len)], 10, core_mp->frame_len, core_mp->xch_len, "after compare charge rate");

		compare_MaxMin_result(index, &tItems[index].buf[(i * core_mp->frame_len)]);
	}

out:
	ipio_kfree((void **)&tItems[index].node_type);

	for (i = 0; i < tItems[index].frame_count; i++) {
		ipio_kfree((void **)&open[i].tdf_700);
		ipio_kfree((void **)&open[i].tdf_250);
		ipio_kfree((void **)&open[i].tdf_200);
		ipio_kfree((void **)&open[i].cbk_700);
		ipio_kfree((void **)&open[i].cbk_250);
		ipio_kfree((void **)&open[i].cbk_200);
		ipio_kfree((void **)&open[i].charg_rate);
		ipio_kfree((void **)&open[i].full_Open);
		ipio_kfree((void **)&open[i].dac);
	}

	return ret;
}

static int codeToOhm(int32_t Code, uint16_t *v_tdf, uint16_t *h_tdf)
{
	int douTDF1 = 0;
	int douTDF2 = 0;
	int douTVCH = 24;
	int douTVCL = 8;
	int douCint = 7;
	int douVariation = 64;
	int douRinternal = 930;
	int32_t temp = 0;

	if (core_mp->isLongV) {
		douTDF1 = *v_tdf;
		douTDF2 = *(v_tdf + 1);
	} else {
		douTDF1 = *h_tdf;
		douTDF2 = *(h_tdf + 1);
	}

	if (Code == 0) {
		ipio_debug(DEBUG_MP_TEST, "code is invalid\n");
	} else {
		temp = ((douTVCH - douTVCL) * douVariation * (douTDF1 - douTDF2) * (1<<12) / (9 * Code * douCint)) * 100;
		temp = (temp - douRinternal) / 1000;
	}
	/* Unit = M Ohm */
	return temp;
}

static int short_test(int index, int frame_index)
{
	int j = 0, ret = 0;
	uint16_t v_tdf[2] = {0};
	uint16_t h_tdf[2] = {0};

	v_tdf[0] = tItems[index].v_tdf_1;
	v_tdf[1] = tItems[index].v_tdf_2;
	h_tdf[0] = tItems[index].h_tdf_1;
	h_tdf[1] = tItems[index].h_tdf_2;

	if (protocol->major >= 5 && protocol->mid >= 4) {
		/* Calculate code to ohm and save to tItems[index].buf */
		for (j = 0; j < core_mp->frame_len; j++)
			tItems[index].buf[frame_index * core_mp->frame_len + j] = codeToOhm(frame_buf[j], v_tdf, h_tdf);
	} else {
		for (j = 0; j < core_mp->frame_len; j++)
			tItems[index].buf[frame_index * core_mp->frame_len + j] = frame_buf[j];
	}

	return ret;
}

static int mutual_test(int index)
{
	int i = 0, j = 0, x = 0, y = 0, ret = 0, get_frame_cont = 1;

	ipio_debug(DEBUG_MP_TEST, "index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
	    index, tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

	/*
	 * We assume that users who are calling the test forget to config frame count
	 * as 1, so we just help them to set it up.
	 */
	if (tItems[index].frame_count <= 0) {
		ipio_err("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0)
		goto out;

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp->ych_len; y++) {
		for (x = 0; x < core_mp->xch_len; x++) {
			if (tItems[i].catalog == TX_RX_DELTA) {
				core_mp->tx_max_buf[y * core_mp->xch_len + x] = INT_MIN;
				core_mp->rx_max_buf[y * core_mp->xch_len + x] = INT_MIN;
				core_mp->tx_min_buf[y * core_mp->xch_len + x] = INT_MAX;
				core_mp->rx_min_buf[y * core_mp->xch_len + x] = INT_MAX;
			} else {
				tItems[index].max_buf[y * core_mp->xch_len + x] = INT_MIN;
				tItems[index].min_buf[y * core_mp->xch_len + x] = INT_MAX;
			}
		}
	}

	if (tItems[index].catalog != PEAK_TO_PEAK_TEST)
		get_frame_cont = tItems[index].frame_count;

	if (tItems[index].spec_option == BENCHMARK) {
		core_parser_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
								tItems[index].type_option, tItems[index].desp, core_mp->frame_len);
		if (ipio_debug_level && DEBUG_PARSER > 0)
			dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	for (i = 0; i < get_frame_cont; i++) {
		ret = allnode_mutual_cdc_data(index);
		if (ret < 0) {
			ipio_err("Failed to initialise CDC data, %d\n", ret);
			goto out;
		}
		switch (tItems[index].catalog) {
		case PIXEL:
			run_pixel_test(index);
			break;
		case UNTOUCH_P2P:
			run_untouch_p2p_test(index);
			break;
		case OPEN_TEST:
			run_open_test(index);
			break;
		case TX_RX_DELTA:
			run_tx_rx_delta_test(index);
			break;
		case SHORT_TEST:
			short_test(index, i);
			break;
		default:
			for (j = 0; j < core_mp->frame_len; j++)
				tItems[index].buf[i * core_mp->frame_len + j] = frame_buf[j];
			break;
		}
		compare_MaxMin_result(index, &tItems[index].buf[i * core_mp->frame_len]);
	}

out:
	return ret;
}

static int key_test(int index)
{
	int i, j = 0, ret = 0;

	ipio_debug(DEBUG_MP_TEST, "Item = %s, CMD = 0x%x, Frame Count = %d\n",
	    tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

	if (tItems[index].frame_count == 0) {
		ipio_err("Frame count is zero, which at least sets as 1\n");
		ret = -EINVAL;
		goto out;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0)
		goto out;

	for (i = 0; i < tItems[index].frame_count; i++) {
		ret = allnode_key_cdc_data(index);
		if (ret < 0) {
			ipio_err("Failed to initialise CDC data, %d\n", ret);
			goto out;
		}

		for (j = 0; j < core_mp->key_len; j++)
			tItems[index].buf[j] = key_buf[j];
	}

	compare_MaxMin_result(index, tItems[index].buf);

out:
	return ret;
}

static int self_test(int index)
{
	ipio_err("TDDI has no self to be tested currently\n");
	return -1;
}

static int st_test(int index)
{
	ipio_err("ST Test is not supported by the driver\n");
	return -1;
}

int mp_test_data_sort_average(int32_t *oringin_data, int index, int32_t *avg_result)
{
	int i, j, k, x, y, len = 5;
	int32_t u32temp;
	int u32up_frame, u32down_frame;
	int32_t *u32sum_raw_data;
	int32_t *u32data_buff;

	if (tItems[index].frame_count <= 1)
		return 0;


	if (ERR_ALLOC_MEM(oringin_data)) {
		ipio_err("Input wrong adress\n");
			return -ENOMEM;
	}

	u32data_buff = kcalloc(core_mp->frame_len * tItems[index].frame_count, sizeof(int32_t), GFP_KERNEL);
	u32sum_raw_data = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(u32sum_raw_data) || (ERR_ALLOC_MEM(u32data_buff))) {
		ipio_err("Failed to allocate u32sum_raw_data FRAME buffer\n");
		return -ENOMEM;
	}

	for (i = 0; i < core_mp->frame_len * tItems[index].frame_count; i++) {
		u32data_buff[i] = oringin_data[i];
	}

	u32up_frame = tItems[index].frame_count * tItems[index].highest_percentage / 100;
	u32down_frame = tItems[index].frame_count * tItems[index].lowest_percentage / 100;
	ipio_debug(DEBUG_MP_TEST, "Up=%d,Down=%d -%s\n", u32up_frame, u32down_frame, tItems[index].desp);

	if (ipio_debug_level & DEBUG_MP_TEST) {
		printk(KERN_CONT "\n[Show Original frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp->frame_len; i++) {
			for (j = 0 ; j < tItems[index].frame_count ; j++) {
				if ((i < len) || (i >= (core_mp->frame_len-len)))
					printk(KERN_CONT "%d,", u32data_buff[j * core_mp->frame_len + i]);
			}
			if ((i < len) || (i >= (core_mp->frame_len-len)))
				printk(KERN_CONT "\n");
		}
	}

	for (i = 0; i < core_mp->frame_len; i++) {
		for (j = 0; j < tItems[index].frame_count-1; j++) {
			for (k = 0; k < (tItems[index].frame_count-1-j); k++) {
				x = i+k*core_mp->frame_len;
				y = i+(k+1)*core_mp->frame_len;
				if (*(u32data_buff+x) > *(u32data_buff+y)) {
					u32temp = *(u32data_buff+x);
					*(u32data_buff+x) = *(u32data_buff+y);
					*(u32data_buff+y) = u32temp;
				}
			}
		}
	}

	if (ipio_debug_level & DEBUG_MP_TEST) {
		printk(KERN_CONT "\n[After sorting frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp->frame_len; i++) {
			for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++) {
				if ((i < len) || (i >= (core_mp->frame_len - len)))
					printk(KERN_CONT "%d,", u32data_buff[i + j * core_mp->frame_len]);
			}
			if ((i < len) || (i >= (core_mp->frame_len-len)))
				printk(KERN_CONT "\n");
		}
	}

	for (i = 0 ; i < core_mp->frame_len ; i++) {
		u32sum_raw_data[i] = 0;
		for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++)
			u32sum_raw_data[i] += u32data_buff[i + j * core_mp->frame_len];

		avg_result[i] = u32sum_raw_data[i] / (tItems[index].frame_count - u32down_frame - u32up_frame);
	}

	if (ipio_debug_level & DEBUG_MP_TEST) {
		printk(KERN_CONT "\n[Average result frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp->frame_len; i++) {
			if ((i < len) || (i >= (core_mp->frame_len-len)))
				printk(KERN_CONT "%d,", avg_result[i]);
		}
		if ((i < len) || (i >= (core_mp->frame_len-len)))
			printk(KERN_CONT "\n");
	}

	ipio_kfree((void **)&u32data_buff);
	ipio_kfree((void **)&u32sum_raw_data);
	return 0;
}

static int mp_comp_result_before_retry(int index)
{
	int i, test_result = MP_PASS;
	int32_t *max_threshold = NULL, *min_threshold = NULL;

	max_threshold = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		ipio_kfree((void **)&max_threshold);
		test_result = MP_FAIL;
		goto fail_alloc;
	}

	min_threshold = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(min_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		ipio_kfree((void **)&min_threshold);
		test_result = MP_FAIL;
		goto fail_alloc;
	}

	/* Show test result as below */
	if (tItems[index].catalog == TX_RX_DELTA) {
		if (ERR_ALLOC_MEM(core_mp->rx_delta_buf) || ERR_ALLOC_MEM(core_mp->tx_delta_buf)) {
			ipio_err("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_FAIL;
			goto out;
		}

		for (i = 0; i < core_mp->frame_len; i++) {
			max_threshold[i] = core_mp->TxDeltaMax;
			min_threshold[i] = core_mp->TxDeltaMin;
		}
		mp_compare_cdc_result(index, core_mp->tx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp->tx_min_buf, max_threshold, min_threshold, &test_result);

		for (i = 0; i < core_mp->frame_len; i++) {
			max_threshold[i] = core_mp->RxDeltaMax;
			min_threshold[i] = core_mp->RxDeltaMin;
		}

		mp_compare_cdc_result(index, core_mp->rx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp->rx_min_buf, max_threshold, min_threshold, &test_result);
	} else {
		if (ERR_ALLOC_MEM(tItems[index].buf) || ERR_ALLOC_MEM(tItems[index].max_buf) ||
				ERR_ALLOC_MEM(tItems[index].min_buf) || ERR_ALLOC_MEM(tItems[index].result_buf)) {
			ipio_err("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_FAIL;
			goto out;
		}

		if (tItems[index].spec_option == BENCHMARK) {
			for (i = 0; i < core_mp->frame_len; i++) {
				max_threshold[i] = tItems[index].bench_mark_max[i];
				min_threshold[i] = tItems[index].bench_mark_min[i];
			}
		} else {
			for (i = 0; i < core_mp->frame_len; i++) {
				max_threshold[i] = tItems[index].max;
				min_threshold[i] = tItems[index].min;
			}
		}

		/* general result */
		if (tItems[index].trimmed_mean && tItems[index].catalog != PEAK_TO_PEAK_TEST) {
			mp_test_data_sort_average(tItems[index].buf, index, tItems[index].result_buf);
			mp_compare_cdc_result(index, tItems[index].result_buf, max_threshold, min_threshold, &test_result);
		} else {
			mp_compare_cdc_result(index, tItems[index].max_buf, max_threshold, min_threshold, &test_result);
			mp_compare_cdc_result(index, tItems[index].min_buf, max_threshold, min_threshold, &test_result);
		}
	}

out:
	ipio_kfree((void **)&max_threshold);
	ipio_kfree((void **)&min_threshold);

fail_alloc:
	tItems[index].item_result = test_result;
	return test_result;
}

static void mp_do_retry(int index, int count)
{
	if (count == 0) {
		ipio_info("Finish retry action\n");
		return;
	}

	ipio_info("retry = %d, item = %s\n", count, tItems[index].desp);

	tItems[index].do_test(index);

	if (mp_comp_result_before_retry(index) == MP_FAIL)
		return mp_do_retry(index, count - 1);
}

static void mp_print_ap_parm(char *csv, int *csv_len)
{
	u8 cmd[15] = {0xF1, 0x3A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	u8 *mp_thr = NULL;
	u16 gain_int, gain_dec, label_thr, td_thr, src_state, gip_state, tdf;
	int i, ret, slen = 0, len = 24, tmp_len = *csv_len;
	char str[8] = {0};
	bool enable;
	char *state[] = {"LFD", "LFD_TRANS", "DC", "HIZ", "INIT", "DC_HIGH", "DC_LOW"};

	slen = core_parser_get_int_data("get mp threshold", "enable", str);
	if (slen < 0) {
		ipio_err("Failed to parse mp threshold item from ini\n");
		goto out;
	}

	enable = katoi(str);
	if (enable) {
		ipio_info("writing ap parameters into MP log\n");
	} else {
		goto out;
	}

	ret = core_write(core_config->slave_i2c_addr, cmd, protocol->cdc_len);
	if (ret < 0) {
		ipio_err("Write CDC command failed\n");
		goto out;
	}

	mp_thr = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(mp_thr)) {
		ipio_err("Failed to allocate mp_thr, (%ld)\n", PTR_ERR(mp_thr));
		goto out;
	}

	ret = core_read(core_config->slave_i2c_addr, mp_thr, len);
	if (ret < 0) {
		ipio_err("Read cdc data error\n");
		goto out;
	}

	dump_data(mp_thr, 8, len, 0, "MP threshold");

	tmp_len += sprintf(csv + tmp_len, "\n[MP Threshold]\n");

	for (i = 0; i < len; i++) {
		tmp_len += sprintf(csv + tmp_len, "0x%02x  ,", mp_thr[i]);
	}

	tmp_len += sprintf(csv + tmp_len, "\n");

	gain_int = ((mp_thr[2] << 8) | mp_thr[3]) / 10;
	gain_dec = ((mp_thr[2] << 8) | mp_thr[3]) % 10;
	tmp_len += sprintf(csv + tmp_len, "DeltaC Gain ,%d.%d%%\n", gain_int, gain_dec);

	label_thr = (mp_thr[4] << 8) | mp_thr[5];
	tmp_len += sprintf(csv + tmp_len, "Label Threshold ,%d\n", label_thr);
	td_thr = (mp_thr[6] << 8) | mp_thr[7];
	tmp_len += sprintf(csv + tmp_len, "TD Threshold ,%d\n", td_thr);

	src_state = (mp_thr[8] << 8) | mp_thr[9];
	tmp_len += sprintf(csv + tmp_len, "SRC State ,%d(%s)\n", src_state, state[src_state]);
	gip_state = (mp_thr[10] << 8) | mp_thr[11];
	tmp_len += sprintf(csv + tmp_len, "GIP State ,%d(%s)\n", gip_state, state[gip_state]);

	tdf = (mp_thr[12] << 8) | mp_thr[13];
	tmp_len += sprintf(csv + tmp_len, "TDF ,%d\n", tdf);

	tmp_len += sprintf(csv + tmp_len, "NODP ,%d\n", mp_thr[14]);

	*csv_len = tmp_len;

out:
	ipio_kfree((void **)&mp_thr);
}

static void mp_show_result(const char *csv_path)
{
	int i, x, y, j, csv_len = 0, pass_item_count = 0, line_count = 0, get_frame_cont = 1;
	int32_t *max_threshold = NULL, *min_threshold = NULL;
	char *csv = NULL;
	char csv_name[128] = { 0 };
	char *ret_pass_name = NULL, *ret_fail_name = NULL;
	struct file *f = NULL;
	mm_segment_t fs;
	loff_t pos;

	csv = vmalloc(CSV_FILE_SIZE);
	if (ERR_ALLOC_MEM(csv)) {
		ipio_err("Failed to allocate CSV mem\n");
		goto fail_open;
	}

	max_threshold = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	min_threshold = kcalloc(core_mp->frame_len, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold) || ERR_ALLOC_MEM(min_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		goto fail_open;
	}

	mp_print_csv_header(csv, &csv_len, &line_count);

	for (i = 0; i < ARRAY_SIZE(tItems); i++) {

		get_frame_cont = 1;

		if (tItems[i].run != 1)
			continue;

		if (tItems[i].item_result == MP_PASS) {
			pr_info("\n\n[%s],OK \n", tItems[i].desp);
			csv_len += sprintf(csv + csv_len, "\n\n[%s],OK\n", tItems[i].desp);
		} else {
			pr_info("\n\n[%s],NG \n", tItems[i].desp);
			csv_len += sprintf(csv + csv_len, "\n\n[%s],NG\n", tItems[i].desp);
		}

		mp_print_csv_cdc_cmd(csv, &csv_len, i);

		pr_info("Frame count = %d\n", tItems[i].frame_count);
		csv_len += sprintf(csv + csv_len, "Frame count = %d\n", tItems[i].frame_count);

		if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
			pr_info("lowest percentage = %d\n", tItems[i].lowest_percentage);
			csv_len += sprintf(csv + csv_len, "lowest percentage = %d\n", tItems[i].lowest_percentage);

			pr_info("highest percentage = %d\n", tItems[i].highest_percentage);
			csv_len += sprintf(csv + csv_len, "highest percentage = %d\n", tItems[i].highest_percentage);
		}

		/* Show result of benchmark max and min */
		if (tItems[i].spec_option == BENCHMARK) {
			for (j = 0; j < core_mp->frame_len; j++) {
				max_threshold[j] = tItems[i].bench_mark_max[j];
				min_threshold[j] = tItems[i].bench_mark_min[j];
			}

			mp_compare_cdc_show_result(i, tItems[i].bench_mark_max, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Max_Bench");
			mp_compare_cdc_show_result(i, tItems[i].bench_mark_min, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Min_Bench");
		} else {

			for (j = 0; j < core_mp->frame_len; j++) {
				max_threshold[j] = tItems[i].max;
				min_threshold[j] = tItems[i].min;
			}

			pr_info("Max = %d\n", tItems[i].max);
			csv_len += sprintf(csv + csv_len, "Max = %d\n", tItems[i].max);

			pr_info("Min = %d\n", tItems[i].min);
			csv_len += sprintf(csv + csv_len, "Min = %d\n", tItems[i].min);
		}

		if (strcmp(tItems[i].name, "open_integration_sp") == 0) {
			mp_compare_cdc_show_result(i, frame1_cbk700, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk700");
			mp_compare_cdc_show_result(i, frame1_cbk250, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk250");
			mp_compare_cdc_show_result(i, frame1_cbk200, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk200");
		}

		if (strcmp(tItems[i].name, "open test_c") == 0) {
			mp_compare_cdc_show_result(i, cap_dac, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_DAC");
			mp_compare_cdc_show_result(i, cap_raw, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_RAW");
		}

		if (tItems[i].catalog == TX_RX_DELTA) {
			if (ERR_ALLOC_MEM(core_mp->rx_delta_buf) || ERR_ALLOC_MEM(core_mp->tx_delta_buf)) {
				ipio_err("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
				continue;
			}
		} else {
			if (ERR_ALLOC_MEM(tItems[i].buf) || ERR_ALLOC_MEM(tItems[i].max_buf) ||
					ERR_ALLOC_MEM(tItems[i].min_buf)) {
				ipio_err("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
				continue;
			}
		}

		/* Show test result as below */
		if (tItems[i].catalog == KEY_TEST) {
			for (x = 0; x < core_mp->key_len; x++) {
				DUMP(DEBUG_MP_TEST, "KEY_%02d ", x);
				csv_len += sprintf(csv + csv_len, "KEY_%02d,", x);
			}

			DUMP(DEBUG_MP_TEST, "\n");
			csv_len += sprintf(csv + csv_len, "\n");

			for (y = 0; y < core_mp->key_len; y++) {
				DUMP(DEBUG_MP_TEST, " %3d   ", tItems[i].buf[y]);
				csv_len += sprintf(csv + csv_len, " %3d, ", tItems[i].buf[y]);
			}

			DUMP(DEBUG_MP_TEST, "\n");
			csv_len += sprintf(csv + csv_len, "\n");
		} else if (tItems[i].catalog == TX_RX_DELTA) {

			for (j = 0; j < core_mp->frame_len; j++) {
				max_threshold[j] = core_mp->TxDeltaMax;
				min_threshold[j] = core_mp->TxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp->tx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Max Hold");
			mp_compare_cdc_show_result(i, core_mp->tx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Min Hold");

			for (j = 0; j < core_mp->frame_len; j++) {
				max_threshold[j] = core_mp->RxDeltaMax;
				min_threshold[j] = core_mp->RxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp->rx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Max Hold");
			mp_compare_cdc_show_result(i, core_mp->rx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Min Hold");
		} else {
			/* general result */
			if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
				mp_compare_cdc_show_result(i, tItems[i].result_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Mean result");
			} else {
				mp_compare_cdc_show_result(i, tItems[i].max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Max Hold");
				mp_compare_cdc_show_result(i, tItems[i].min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Min Hold");
			}
			if (tItems[i].catalog != PEAK_TO_PEAK_TEST)
				get_frame_cont = tItems[i].frame_count;

			/* result of each frame */
			for (j = 0; j < get_frame_cont; j++) {
				char frame_name[128] = { 0 };
				sprintf(frame_name, "Frame %d", (j+1));
				mp_compare_cdc_show_result(i, &tItems[i].buf[(j*core_mp->frame_len)], csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, frame_name);
			}
		}
	}

	memset(csv_name, 0, 128 * sizeof(char));

	mp_print_csv_tail(csv, &csv_len);

	mp_print_ap_parm(csv, &csv_len);

	for (i = 0; i < ARRAY_SIZE(tItems); i++) {
		if (tItems[i].run) {
			if (tItems[i].item_result == MP_FAIL) {
				pass_item_count = 0;
				break;
			}
			pass_item_count++;
		}
	}

	/* define csv file name */
	ret_pass_name = NORMAL_CSV_PASS_NAME;
	ret_fail_name = NORMAL_CSV_FAIL_NAME;

	if (pass_item_count == 0) {
		core_mp->final_result = MP_FAIL;
		sprintf(csv_name, "%s/%s_%s.csv", csv_path, get_date_time_str(), ret_fail_name);
	} else {
		core_mp->final_result = MP_PASS;
		sprintf(csv_name, "%s/%s_%s.csv", csv_path, get_date_time_str(), ret_pass_name);
	}

	ipio_info("Open CSV : %s\n", csv_name);

	if (f == NULL)
		f = filp_open(csv_name, O_WRONLY | O_CREAT | O_TRUNC, 644);

	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open CSV file");
		goto fail_open;
	}

	ipio_info("Open CSV succeed, its length = %d\n ", csv_len);

	if (csv_len >= CSV_FILE_SIZE) {
		ipio_err("The length saved to CSV is too long !\n");
		goto fail_open;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, csv, csv_len, &pos);
	set_fs(fs);
	filp_close(f, NULL);

	ipio_info("Writing Data into CSV succeed\n");

fail_open:
	if (csv != NULL)
		vfree(csv);

	ipio_kfree((void **)&max_threshold);
	ipio_kfree((void **)&min_threshold);
}

/* The method to copy results to user depends on what APK needs */
void core_mp_copy_reseult(char *buf, size_t size)
{
	int i, run = 0;

	if (size < core_mp->mp_items) {
		ipio_err("The size given from caller is less than item\n");
		return;
	}

	for (i = 0; i < core_mp->mp_items; i++) {
		buf[i] = 2;
		if (tItems[i].run) {
			if (tItems[i].item_result == MP_FAIL)
				buf[i] = 1;
			else
				buf[i] = 0;

			run++;
		}
	}
}
EXPORT_SYMBOL(core_mp_copy_reseult);

static void mp_run_test(char *item, int id)
{
	char str[512] = { 0 };

	if (ERR_ALLOC_MEM(core_mp)) {
		ipio_err("core_mp is null, do nothing\n");
		return;
	}

	/* update X/Y channel length if they're changed */
	core_mp->xch_len = core_config->tp_info->nXChannelNum;
	core_mp->ych_len = core_config->tp_info->nYChannelNum;

	/* update key's length if they're changed */
	core_mp->key_len = core_config->tp_info->nKeyCount;

	/* compute the total length in one frame */
	core_mp->frame_len = core_mp->xch_len * core_mp->ych_len;

	if (item == NULL || strncmp(item, " ", strlen(item)) == 0 || core_mp->frame_len == 0) {
		tItems[id].result = "FAIL";
		core_mp->final_result = MP_FAIL;
		ipio_err("Invaild string (%s) or frame length (%d)\n", item, core_mp->frame_len);
		return;
	}

	ipio_debug(DEBUG_MP_TEST, "id = %d, item = %s, core type = %d\n", id, item, core_config->core_type);

	/* Get parameters from ini */
	core_parser_get_int_data(item, "enable", str);
	tItems[id].run = katoi(str);
	core_parser_get_int_data(item, "spec option", str);
	tItems[id].spec_option = katoi(str);
	core_parser_get_int_data(item, "type option", str);
	tItems[id].type_option = katoi(str);
	core_parser_get_int_data(item, "frame count", str);
	tItems[id].frame_count = katoi(str);
	core_parser_get_int_data(item, "trimmed mean", str);
	tItems[id].trimmed_mean = katoi(str);
	core_parser_get_int_data(item, "lowest percentage", str);
	tItems[id].lowest_percentage = katoi(str);
	core_parser_get_int_data(item, "highest percentage", str);
	tItems[id].highest_percentage = katoi(str);

	/* Get TDF value from ini */
	if (tItems[id].catalog == SHORT_TEST) {
		core_parser_get_int_data(item, "v_tdf_1", str);
		tItems[id].v_tdf_1 = core_parser_get_tdf_value(str, tItems[id].catalog);
		core_parser_get_int_data(item, "v_tdf_2", str);
		tItems[id].v_tdf_2 = core_parser_get_tdf_value(str, tItems[id].catalog);
		core_parser_get_int_data(item, "h_tdf_1", str);
		tItems[id].h_tdf_1 = core_parser_get_tdf_value(str, tItems[id].catalog);
		core_parser_get_int_data(item, "h_tdf_2", str);
		tItems[id].h_tdf_2 = core_parser_get_tdf_value(str, tItems[id].catalog);
	} else {
		core_parser_get_int_data(item, "v_tdf", str);
		tItems[id].v_tdf_1 = core_parser_get_tdf_value(str, tItems[id].catalog);
		core_parser_get_int_data(item, "h_tdf", str);
		tItems[id].h_tdf_1 = core_parser_get_tdf_value(str, tItems[id].catalog);
	}

	/* Get threshold from ini structure in parser */
	if (strcmp(item, "tx/rx delta") == 0) {
		core_parser_get_int_data(item, "tx max", str);
		core_mp->TxDeltaMax = katoi(str);
		core_parser_get_int_data(item, "tx min", str);
		core_mp->TxDeltaMin = katoi(str);
		core_parser_get_int_data(item, "rx max", str);
		core_mp->RxDeltaMax = katoi(str);
		core_parser_get_int_data(item, "rx min", str);
		core_mp->RxDeltaMin = katoi(str);
		ipio_debug(DEBUG_MP_TEST, "%s: Tx Max = %d, Tx Min = %d, Rx Max = %d,  Rx Min = %d\n",
				tItems[id].desp, core_mp->TxDeltaMax, core_mp->TxDeltaMin,
				core_mp->RxDeltaMax, core_mp->RxDeltaMin);
	} else {
		core_parser_get_int_data(item, "max", str);
		tItems[id].max = katoi(str);
		core_parser_get_int_data(item, "min", str);
		tItems[id].min = katoi(str);
	}

	core_parser_get_int_data(item, "frame count", str);
	tItems[id].frame_count = katoi(str);

	ipio_debug(DEBUG_MP_TEST, "%s: run = %d, max = %d, min = %d, frame_count = %d\n", tItems[id].desp,
			tItems[id].run, tItems[id].max, tItems[id].min, tItems[id].frame_count);

	ipio_debug(DEBUG_MP_TEST, "v_tdf_1 = %d, v_tdf_2 = %d, h_tdf_1 = %d, h_tdf_2 = %d\n", tItems[id].v_tdf_1,
			tItems[id].v_tdf_2, tItems[id].h_tdf_1, tItems[id].h_tdf_2);

	if (tItems[id].run) {
		ipio_info("Running Test Item : %s\n", tItems[id].desp);
		tItems[id].do_test(id);

		/* Check result before do retry (if enabled)  */
		if (mp_comp_result_before_retry(id) == MP_FAIL) {
			if (core_mp->retry) {
				ipio_info("MP failed, doing retry\n");
				mp_do_retry(id, RETRY_COUNT);
			}
		}
	}
}

#ifndef HOST_DOWNLOAD
static void dma_clear_register_setting(void)
{
	ipio_info("interrupt t0/t1 enable flag\n");
	core_config_ice_mode_bit_mask(INTR32_ADDR, INTR32_reg_t0_int_en, (0 << 24));
	core_config_ice_mode_bit_mask(INTR32_ADDR, INTR32_reg_t1_int_en, (0 << 25));

	ipio_info("clear tdi_err_int_flag\n");
	core_config_ice_mode_bit_mask(INTR2_ADDR, INTR2_tdi_err_int_flag_clear, (1 << 18));

	ipio_info("clear dma channel 0 src1 info\n");
	core_config_ice_mode_write(DMA49_reg_dma_ch0_src1_addr, 0x00000000, 4);
	core_config_ice_mode_write(DMA50_reg_dma_ch0_src1_step_inc, 0x00, 1);
	core_config_ice_mode_bit_mask(DMA50_ADDR, DMA50_reg_dma_ch0_src1_format, (0 << 24));
	core_config_ice_mode_bit_mask(DMA50_ADDR, DMA50_reg_dma_ch0_src1_en, (1 << 31));

	ipio_info("clear dma channel 0 src2 info\n");
	core_config_ice_mode_bit_mask(DMA52_ADDR, DMA52_reg_dma_ch0_src2_en, (0 << 31));

	ipio_info("clear dma channel 0 trafer info\n");
	core_config_ice_mode_write(DMA55_reg_dma_ch0_trafer_counts, 0x00000000, 4);
	core_config_ice_mode_bit_mask(DMA55_ADDR, DMA55_reg_dma_ch0_trafer_mode, (0 << 24));

	ipio_info("clear dma channel 0 trigger select\n");
	core_config_ice_mode_bit_mask(DMA48_ADDR, DMA48_reg_dma_ch0_trigger_sel, (0 << 16));

	core_config_ice_mode_bit_mask(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25));

	ipio_info("clear dma flash setting\n");
	core_flash_dma_clear();
}

static void dma_trigger_register_setting(uint32_t nRegDestAddr, uint32_t nFlashStartAddr, uint32_t nCopySize)
{
	ipio_info("set dma channel 0 clear\n");
	core_config_ice_mode_bit_mask(DMA48_ADDR, DMA48_reg_dma_ch0_start_clear, (1 << 25));

	ipio_info("set dma channel 0 src1 info\n");
	core_config_ice_mode_write(DMA49_reg_dma_ch0_src1_addr, 0x00041010, 4);
	core_config_ice_mode_write(DMA50_reg_dma_ch0_src1_step_inc, 0x00, 1);
	core_config_ice_mode_bit_mask(DMA50_ADDR, DMA50_reg_dma_ch0_src1_format, (0 << 24));
	core_config_ice_mode_bit_mask(DMA50_ADDR, DMA50_reg_dma_ch0_src1_en, (1 << 31));

	ipio_info("set dma channel 0 src2 info\n");
	core_config_ice_mode_bit_mask(DMA52_ADDR, DMA52_reg_dma_ch0_src2_en, (0 << 31));

	ipio_info("set dma channel 0 dest info\n");
	core_config_ice_mode_write(DMA53_reg_dma_ch0_dest_addr, nRegDestAddr, 3);
	core_config_ice_mode_write(DMA54_reg_dma_ch0_dest_step_inc, 0x01, 1);
	core_config_ice_mode_bit_mask(DMA54_ADDR, DMA54_reg_dma_ch0_dest_format, (0 << 24));
	core_config_ice_mode_bit_mask(DMA54_ADDR, DMA54_reg_dma_ch0_dest_en, (1 << 31));

	ipio_info("set dma channel 0 trafer info\n");
	core_config_ice_mode_write(DMA55_reg_dma_ch0_trafer_counts, nCopySize, 4);
	core_config_ice_mode_bit_mask(DMA55_ADDR, DMA55_reg_dma_ch0_trafer_mode, (0 << 24));

	ipio_info("set dma channel 0 int info\n");
	core_config_ice_mode_bit_mask(INTR33_ADDR, INTR33_reg_dma_ch0_int_en, (1 << 17));

	ipio_info("set dma channel 0 trigger select\n");
	core_config_ice_mode_bit_mask(DMA48_ADDR, DMA48_reg_dma_ch0_trigger_sel, (1 << 16));

	ipio_info("set dma flash setting, FlashAddr = 0x%x\n", nFlashStartAddr);
	core_flash_dma_write(nFlashStartAddr, (nFlashStartAddr+nCopySize), nCopySize);

	ipio_info("clear flash and dma ch0 int flag\n");
	core_config_ice_mode_bit_mask(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25));
	core_config_ice_mode_bit_mask(INTR1_ADDR, INTR1_reg_dma_ch0_int_flag, (1 << 17));
	core_config_ice_mode_bit_mask(0x041013, BIT(0), 1); //patch

	/* DMA Trigger */
	core_config_ice_mode_write(FLASH4_reg_rcv_data, 0xFF, 1);
	mdelay(30);

	/* CS High */
	core_config_ice_mode_write(FLASH0_reg_flash_csb, 0x1, 1);
	mdelay(60);
}

void get_dma_overlay_info(void)
{
	int ret = 0;
	uint8_t szOutBuf[16] = {0};

	szOutBuf[0] = protocol->cmd_get_mp_info;

	ipio_info("Get overlay info, cmd = 0x%x\n", szOutBuf[0]);

	ret = core_write(core_config->slave_i2c_addr, szOutBuf, 1);
	if (ret < 0) {
		ipio_err("Failed to get overlay command\n");
		return;
	}

	memset(szOutBuf, 0, sizeof(szOutBuf));

	ret = core_read(core_config->slave_i2c_addr, szOutBuf, protocol->mp_info_len);
	if (ret < 0) {
		ipio_err("Failed to get overlay command\n");
		return;
	}

	mp_move.dma_trigger_enable = 0;

	mp_move.mp_flash_addr = szOutBuf[3] + (szOutBuf[2] << 8) + (szOutBuf[1] << 16);
	mp_move.mp_size = szOutBuf[6] + (szOutBuf[5] << 8) + (szOutBuf[4] << 16);
	mp_move.overlay_start_addr = szOutBuf[9] + (szOutBuf[8] << 8) + (szOutBuf[7] << 16);
	mp_move.overlay_end_addr = szOutBuf[12] + (szOutBuf[11] << 8) + (szOutBuf[10] << 16);

	if (mp_move.overlay_start_addr != 0x0 && mp_move.overlay_end_addr != 0x0 && szOutBuf[0] == protocol->cmd_get_mp_info)
		mp_move.dma_trigger_enable = 1;

	ipio_info("Overlay addr = 0x%x ~ 0x%x , flash addr = 0x%x , mp size = 0x%x\n",
		mp_move.overlay_start_addr, mp_move.overlay_end_addr, mp_move.mp_flash_addr, mp_move.mp_size);
}
#endif

int core_mp_move_code(void)
{
	int ret = 0;
#ifndef HOST_DOWNLOAD
	uint32_t mp_text_size = 0, mp_andes_init_size = 0;
#endif

	ipio_info("Start moving MP code\n");

#ifdef HOST_DOWNLOAD
	if (ilitek_platform_reset_ctrl(true, RST_METHODS) < 0) {
		goto out;
	}
#else
	/* Get Dma overlay info command only be used in I2C mode */
	get_dma_overlay_info();

	if (core_config_ice_mode_enable(STOP_MCU) < 0) {
		ipio_err("Failed to enter ICE mode\n");
		ret = -1;
		goto out;
	}

	ipio_info("DMA trigger = %d\n", mp_move.dma_trigger_enable);

	if (mp_move.dma_trigger_enable) {
		mp_andes_init_size = mp_move.overlay_start_addr;
		mp_text_size = (mp_move.mp_size - mp_move.overlay_end_addr) + 1;
		ipio_info("Mp andes init size = %d , Mp text size = %d\n", mp_andes_init_size, mp_text_size);

		ipio_info("[clear register setting]\n");
		dma_clear_register_setting();

		ipio_info("[Move ANDES.INIT to DRAM]\n");
		dma_trigger_register_setting(0, mp_move.mp_flash_addr, mp_andes_init_size);   /* DMA ANDES.INIT */

		ipio_info("[Clear register setting]\n");
		dma_clear_register_setting();

		ipio_info("[Move MP.TEXT to DRAM]\n");
		//dma_trigger_register_setting(NUM_OF_4_MULTIPLE(mp_move.overlay_end_addr), (mp_move.mp_flash_addr + NUM_OF_4_MULTIPLE(mp_move.overlay_end_addr)), mp_text_size);  /* DMA MP.TEXT */
		dma_trigger_register_setting(mp_move.overlay_end_addr, (mp_move.mp_flash_addr + mp_move.overlay_start_addr), mp_text_size);
	} else {
		/* DMA Trigger */
		core_config_ice_mode_write(FLASH4_reg_rcv_data, 0xFF, 1);
		mdelay(30);

		/* CS High */
		core_config_ice_mode_write(FLASH0_reg_flash_csb, 0x1, 1);
		mdelay(60);
	}

	/* Code reset */
	core_config_ice_mode_write(0x40040, 0xAE, 1);

	core_config_ice_mode_disable();

	if (core_config_check_cdc_busy(300, 50) < 0) {
		ipio_err("Check busy is timout ! moving MP code failed\n");
		ret = -1;
		goto out;
	}
#endif

out:
	return ret;
}
EXPORT_SYMBOL(core_mp_move_code);

void core_mp_test_free(void)
{
	int i;

	ipio_info("Free all allocated mem\n");

	core_mp->final_result = MP_FAIL;

	for (i = 0; i < ARRAY_SIZE(tItems); i++) {
		tItems[i].run = false;
		tItems[i].max_res = MP_FAIL;
		tItems[i].min_res = MP_FAIL;
		tItems[i].item_result = MP_PASS;
		sprintf(tItems[i].result, "%s", "FAIL");

		if (tItems[i].catalog == TX_RX_DELTA) {
			if (core_mp != NULL) {
				ipio_kfree((void **)&core_mp->rx_delta_buf);
				ipio_kfree((void **)&core_mp->tx_delta_buf);
				ipio_kfree((void **)&core_mp->tx_max_buf);
				ipio_kfree((void **)&core_mp->tx_min_buf);
				ipio_kfree((void **)&core_mp->rx_max_buf);
				ipio_kfree((void **)&core_mp->rx_min_buf);
			}
		} else {
			if (tItems[i].spec_option == BENCHMARK) {
				ipio_kfree((void **)&tItems[i].bench_mark_max);
				ipio_kfree((void **)&tItems[i].bench_mark_min);
			}

			ipio_kfree((void **)&tItems[i].result_buf);
			ipio_kfree((void **)&tItems[i].max_buf);
			ipio_kfree((void **)&tItems[i].min_buf);
			vfree(tItems[i].buf);
			tItems[i].buf = NULL;
		}
	}

	ipio_kfree((void **)&frame1_cbk700);
	ipio_kfree((void **)&frame1_cbk250);
	ipio_kfree((void **)&frame1_cbk200);
	ipio_kfree((void **)&frame_buf);
	ipio_kfree((void **)&key_buf);
	ipio_kfree((void **)&core_mp);
}
EXPORT_SYMBOL(core_mp_test_free);

static void mp_test_init_item(void)
{
	int i;

	core_mp->mp_items = ARRAY_SIZE(tItems);

	/* assign test functions run on MP flow according to their catalog */
	for (i = 0; i < ARRAY_SIZE(tItems); i++) {

		tItems[i].spec_option = 0;
		tItems[i].type_option = 0;
		tItems[i].run = false;
		tItems[i].max = 0;
		tItems[i].max_res = MP_FAIL;
		tItems[i].item_result = MP_PASS;
		tItems[i].min = 0;
		tItems[i].min_res = MP_FAIL;
		tItems[i].frame_count = 0;
		tItems[i].trimmed_mean = 0;
		tItems[i].lowest_percentage = 0;
		tItems[i].highest_percentage = 0;
		tItems[i].v_tdf_1 = 0;
		tItems[i].v_tdf_2 = 0;
		tItems[i].h_tdf_1 = 0;
		tItems[i].h_tdf_2 = 0;
		tItems[i].result_buf = NULL;
		tItems[i].buf = NULL;
		tItems[i].max_buf = NULL;
		tItems[i].min_buf = NULL;
		tItems[i].bench_mark_max = NULL;
		tItems[i].bench_mark_min = NULL;
		tItems[i].node_type = NULL;

		if (tItems[i].catalog == MUTUAL_TEST) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == TX_RX_DELTA) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == UNTOUCH_P2P) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == PIXEL) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == OPEN_TEST) {
			if (strcmp(tItems[i].name, "open_integration_sp") == 0)
				tItems[i].do_test = open_test_sp;
			else if (strcmp(tItems[i].name, "open test_c") == 0)
				tItems[i].do_test = open_test_cap;
			else
				tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == KEY_TEST) {
			tItems[i].do_test = key_test;
		} else if (tItems[i].catalog == SELF_TEST) {
			tItems[i].do_test = self_test;
		} else if (tItems[i].catalog == ST_TEST) {
			tItems[i].do_test = st_test;
		} else if (tItems[i].catalog == PEAK_TO_PEAK_TEST) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == SHORT_TEST) {
			tItems[i].do_test = mutual_test;
		}

		tItems[i].result = kmalloc(16, GFP_KERNEL);
		sprintf(tItems[i].result, "%s", "FAIL");
	}

	/*
	 * assign protocol command written into firmware via I2C,
	 * which might be differnet if the version of protocol was changed.
	 */
	tItems[0].cmd = protocol->mutual_dac;
	tItems[1].cmd = protocol->mutual_bg;
	tItems[2].cmd = protocol->mutual_signal;
	tItems[3].cmd = protocol->mutual_no_bk;
	tItems[4].cmd = protocol->mutual_has_bk;
	tItems[5].cmd = protocol->mutual_bk_dac;
	tItems[6].cmd = protocol->self_dac;
	tItems[7].cmd = protocol->self_bg;
	tItems[8].cmd = protocol->self_signal;
	tItems[9].cmd = protocol->self_no_bk;
	tItems[10].cmd = protocol->self_has_bk;
	tItems[11].cmd = protocol->self_bk_dac;
	tItems[12].cmd = protocol->key_dac;
	tItems[13].cmd = protocol->key_bg;
	tItems[14].cmd = protocol->key_no_bk;
	tItems[15].cmd = protocol->key_has_bk;
	tItems[16].cmd = protocol->key_open;
	tItems[17].cmd = protocol->key_short;
	tItems[18].cmd = protocol->st_dac;
	tItems[19].cmd = protocol->st_bg;
	tItems[20].cmd = protocol->st_no_bk;
	tItems[21].cmd = protocol->st_has_bk;
	tItems[22].cmd = protocol->st_open;
	tItems[23].cmd = protocol->tx_short;
	tItems[24].cmd = protocol->rx_short;
	tItems[25].cmd = protocol->rx_open;
	tItems[26].cmd = protocol->cm_data;
	tItems[27].cmd = protocol->cs_data;
	tItems[28].cmd = protocol->tx_rx_delta;
	tItems[29].cmd = protocol->mutual_signal;
	tItems[30].cmd = protocol->mutual_no_bk;
	tItems[31].cmd = protocol->mutual_has_bk;
	tItems[32].cmd = protocol->rx_open;
	tItems[33].cmd = protocol->rx_open;
	tItems[34].cmd = protocol->peak_to_peak;
}

static int mp_data_init(void)
{
	int ret = 0;

	if (!ERR_ALLOC_MEM(core_config->tp_info)) {
		if (core_mp == NULL) {
			core_mp = kzalloc(sizeof(*core_mp), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp)) {
				ipio_err("Failed to init core_mp, %ld\n", PTR_ERR(core_mp));
				ret = -ENOMEM;
				goto out;
			}

			core_mp->xch_len = core_config->tp_info->nXChannelNum;
			core_mp->ych_len = core_config->tp_info->nYChannelNum;

			core_mp->stx_len = core_config->tp_info->self_tx_channel_num;
			core_mp->srx_len = core_config->tp_info->self_rx_channel_num;

			core_mp->key_len = core_config->tp_info->nKeyCount;
			core_mp->st_len = core_config->tp_info->side_touch_type;

			core_mp->tdf = 240;
			core_mp->busy_cdc = CHECK_BUSY_TYPE;

			core_mp->retry = false;
			core_mp->final_result = MP_FAIL;

			mp_test_init_item();
		}
	} else {
		ipio_err("Failed to allocate core_mp mem as did not find TP info\n");
		ret = -ENOMEM;
	}

out:
	return ret;
}

int core_mp_start_test(bool lcm_on)
{
	int ret = 0;
	const char *csv_path = NULL;

	ilitek_platform_disable_irq();
	mutex_lock(&ipd->plat_mutex);
	mutex_lock(&ipd->touch_mutex);

	/* Init MP structure */
	ret = mp_data_init();
	if (ret < 0) {
		ipio_err("Failed to init mp\n");
		goto out;
	}

	ret = core_parser_path(INI_NAME_PATH);
	if (ret < 0) {
		ipio_err("Failed to parsing INI file\n");
		goto out;
	}

	if (core_fr->actual_fw_mode != protocol->test_mode) {
		/* Switch to Test mode nad move mp code */
		ret = core_config_switch_fw_mode(&protocol->test_mode);
		if (ret < 0) {
			ipio_err("Switch to test mode failed\n");
			goto out;
		}
	}

	/* Read timing info from ini file */
	if (protocol->major >= 5 && protocol->mid >= 4) {
		ret = mp_get_timing_info();
		if (ret < 0) {
			ipio_err("Failed to get timing info from ini\n");
			goto out;
		}
	}

	/* Do not chang the sequence of test */
	if (protocol->major >= 5 && protocol->mid >= 4) {
		if (lcm_on) {
			csv_path = CSV_LCM_ON_PATH;
			mp_run_test("noise peak to peak(with panel)", 35);
			mp_run_test("noise peak to peak(ic only)", 34);
			mp_run_test("short test -ili9881", 24); //compatible with old ini version.
			mp_run_test("short test", 45);
			mp_run_test("open test(integration)_sp", 40);
			mp_run_test("raw data(no bk)", 3);
			mp_run_test("calibration data(dac)", 0);
			mp_run_test("doze raw data", 41);
			mp_run_test("doze peak to peak", 42);
			mp_run_test("open test_c", 46);
			mp_run_test("touch deltac", 47);
		} else {
			csv_path = CSV_LCM_OFF_PATH;
			mp_run_test("raw data(have bk) (lcm off)", 39);
			mp_run_test("raw data(no bk) (lcm off)", 38);
			mp_run_test("noise peak to peak(with panel) (lcm off)", 37);
			mp_run_test("noise peak to peak(ic only) (lcm off)", 36);
			mp_run_test("raw data_td (lcm off)", 43);
			mp_run_test("peak to peak_td (lcm off)", 44);
		}
	} else {
		mp_run_test("untouch peak to peak", 29);
		mp_run_test("open test(integration)", 32);
		mp_run_test("open test(cap)", 33);
		// mp_run_test("Short Test (Rx)");
		// mp_run_test("Untouch Calibration Data(DAC) - Mutual");
		// mp_run_test("Untouch Raw Data(Have BK) - Mutual");
		// mp_run_test("Untouch Raw Data(No BK) - Mutual");
		mp_run_test("untouch cm data", 26);
		mp_run_test("pixel raw (no bk)", 30);
		mp_run_test("pixel raw (have bk)", 31);
	}

	mp_show_result(csv_path);

out:
	core_config_switch_fw_mode(&protocol->demo_mode);
	mutex_unlock(&ipd->plat_mutex);
	mutex_unlock(&ipd->touch_mutex);
	ilitek_platform_enable_irq();
	return ret;
}
EXPORT_SYMBOL(core_mp_start_test);
