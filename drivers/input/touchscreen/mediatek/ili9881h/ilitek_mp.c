/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
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
 */

#include "ilitek.h"

#define VALUE			0
#define RETRY_COUNT		3
#define INT_CHECK		0
#define POLL_CHECK		1

#define BENCHMARK		1
#define NODETYPE		1

#define TYPE_BENCHMARK		0
#define TYPE_NO_JUGE		1
#define TYPE_JUGE		2

#define NORMAL_CSV_PASS_NAME	"mp_pass"
#define NORMAL_CSV_FAIL_NAME	"mp_fail"
#define NORMAL_CSV_WARNING_NAME	"mp_warning"

#define CSV_FILE_SIZE		(1 * M)

#define PARSER_MAX_CFG_BUF		(512 * 3)
#define PARSER_MAX_KEY_NUM		(600 * 3)
#define PARSER_MAX_KEY_NAME_LEN		100
#define PARSER_MAX_KEY_VALUE_LEN	2000
#define BENCHMARK_KEY_NAME		"benchmark_data"
#define NODE_TYPE_KEY_NAME		"node type"
#define INI_ERR_OUT_OF_LINE		-1

#define CMD_MUTUAL_DAC			0x1
#define CMD_MUTUAL_BG			0x2
#define CMD_MUTUAL_SIGNAL		0x3
#define CMD_MUTUAL_NO_BK		0x5
#define CMD_MUTUAL_HAVE_BK		0x8
#define CMD_MUTUAL_BK_DAC		0x10
#define CMD_SELF_DAC			0xC
#define CMD_SELF_BG			0xF
#define CMD_SELF_SIGNAL			0xD
#define CMD_SELF_NO_BK			0xE
#define CMD_SELF_HAVE_BK		0xB
#define CMD_SELF_BK_DAC			0x11
#define CMD_KEY_DAC			0x14
#define CMD_KEY_BG			0x16
#define CMD_KEY_NO_BK			0x7
#define CMD_KEY_HAVE_BK			0x15
#define CMD_KEY_OPEN			0x12
#define CMD_KEY_SHORT			0x13
#define CMD_ST_DAC			0x1A
#define CMD_ST_BG			0x1C
#define CMD_ST_NO_BK			0x17
#define CMD_ST_HAVE_BK			0x1B
#define CMD_ST_OPEN			0x18
#define CMD_TX_SHORT			0x19
#define CMD_RX_SHORT			0x4
#define CMD_RX_OPEN			0x6
#define CMD_TX_RX_DELTA			0x1E
#define CMD_CM_DATA			0x9
#define CMD_CS_DATA			0xA
#define CMD_TRCRQ_PIN			0x20
#define CMD_RESX2_PIN			0x21
#define CMD_MUTUAL_INTEGRA_TIME		0x22
#define CMD_SELF_INTEGRA_TIME		0x23
#define CMD_KEY_INTERGRA_TIME		0x24
#define CMD_ST_INTERGRA_TIME		0x25
#define CMD_PEAK_TO_PEAK		0x1D
#define CMD_GET_TIMING_INFO		0x30
#define CMD_DOZE_P2P			0x32
#define CMD_DOZE_RAW			0x33
#define CMD_PIN_TEST			0x61

#define MP_DATA_PASS			0
#define MP_DATA_FAIL			-1

/* Exception error codes */
#define EMP_CMD				100
#define EMP_PROTOCOL			101
#define EMP_FILE			102
#define EMP_INI				103
#define EMP_TIMING_INFO			104
#define EMP_INVAL			105
#define EMP_PARSE			106
#define EMP_NOMEM			107
#define EMP_GET_CDC			108
#define EMP_INT				109
#define EMP_CHECK_BUY			110

#define Mathabs(x) ({					\
		long ret;				\
		if (sizeof(x) == sizeof(long)) {	\
		long __x = (x);				\
		ret = (__x < 0) ? -__x : __x;		\
		} else {				\
		int __x = (x);				\
		ret = (__x < 0) ? -__x : __x;		\
		}					\
		ret;					\
	})

#define DUMP(fmt, arg...)		\
	do {				\
		if (ipio_debug_level)	\
		pr_cont(fmt, ##arg);	\
	} while (0)

struct ini_file_data {
	char pSectionName[PARSER_MAX_KEY_NAME_LEN];
	char pKeyName[PARSER_MAX_KEY_NAME_LEN];
	char pKeyValue[PARSER_MAX_KEY_VALUE_LEN];
	int iSectionNameLen;
	int iKeyNameLen;
	int iKeyValueLen;
} *ilitek_ini_file_data;

enum open_test_node_type {
	NO_COMPARE = 0x00,	/* Not A Area, No Compare */
	AA_Area = 0x01,		/* AA Area, Compare using Charge_AA */
	Border_Area = 0x02, 	/* Border Area, Compare using Charge_Border */
	Notch = 0x04,		/* Notch Area, Compare using Charge_Notch */
	Round_Corner = 0x08,	/* Round Corner, No Compare */
	Skip_Micro = 0x10	/* Skip_Micro, No Compare */
};

enum mp_test_catalog {
	MUTUAL_TEST = 0,
	SELF_TEST = 1,
	KEY_TEST = 2,
	ST_TEST = 3,
	TX_RX_DELTA = 4,
	UNTOUCH_P2P = 5,
	PIXEL = 6,
	OPEN_TEST = 7,
	PEAK_TO_PEAK_TEST = 8,
	SHORT_TEST = 9,
	PIN_TEST = 10,
};

struct mp_test_P540_open {
	s32 *tdf_700;
	s32 *tdf_250;
	s32 *tdf_200;
	s32 *cbk_700;
	s32 *cbk_250;
	s32 *cbk_200;
	s32 *charg_rate;
	s32 *full_Open;
	s32 *dac;
};

struct mp_test_open_c {
	s32 *cap_dac;
	s32 *cap_raw;
	s32 *dcl_cap;
};

struct open_test_spec {
	int tvch;
	int tvcl;
	int gain;
} open_spec;

struct core_mp_test_data {
	u32 chip_pid;
	u32 fw_ver;
	u32 protocol_ver;
	u32 core_ver;
	int no_bk_shift;
	bool retry;
	bool m_signal;
	bool m_dac;
	bool s_signal;
	bool s_dac;
	bool key_dac;
	bool st_dac;
	bool p_no_bk;
	bool p_has_bk;
	bool open_integ;
	bool open_cap;
	bool isLongV;

	int cdc_len;
	int xch_len;
	int ych_len;
	int stx_len;
	int srx_len;
	int key_len;
	int st_len;
	int frame_len;
	int mp_items;
	int final_result;

	u32 overlay_start_addr;
	u32 overlay_end_addr;
	u32 mp_flash_addr;
	u32 mp_size;
	u8 dma_trigger_enable;

	/* Tx/Rx threshold & buffer */
	int TxDeltaMax;
	int TxDeltaMin;
	int RxDeltaMax;
	int RxDeltaMin;
	s32 *tx_delta_buf;
	s32 *rx_delta_buf;
	s32 *tx_max_buf;
	s32 *tx_min_buf;
	s32 *rx_max_buf;
	s32 *rx_min_buf;

	int tdf;
	int busy_cdc;
	bool ctrl_lcm;
	bool lost_benchmark;
} core_mp = {0};

struct mp_test_items {
	char *name;
	/* The description must be the same as ini's section name */
	char *desp;
	char *result;
	int catalog;
	u8 cmd;
	u8 spec_option;
	u8 type_option;
	bool run;
	int max;
	int max_res;
	int item_result;
	int min;
	int min_res;
	int frame_count;
	int trimmed_mean;
	int lowest_percentage;
	int highest_percentage;
	int v_tdf_1;
	int v_tdf_2;
	int h_tdf_1;
	int h_tdf_2;
	int goldenmode;
	u8  delay_time;
	u8  test_int_pin;
	u8  int_pulse_test;
	s32 *result_buf;
	s32 *buf;
	s32 *max_buf;
	s32 *min_buf;
	s32 *bench_mark_max;
	s32 *bench_mark_min;
	s32 *node_type;
	int (*do_test)(int index);
};

#define DEF_TEST_LCM_ON	13
static char run_lcm_on[DEF_TEST_LCM_ON][64] = {
	"pin test ( int and rst )",
	"noise peak to peak(with panel)",
	"noise peak to peak(ic only)",
	"open test(integration)_sp",
	"raw data(no bk)",
	"raw data(have bk)",
	"calibration data(dac)",
	"short test -ili9881",
	"short test",
	"doze raw data",
	"doze peak to peak",
	"open test_c",
	"touch deltac"
};

#define DEF_TEST_LCM_OFF	6
static char run_lcm_off[DEF_TEST_LCM_OFF][64] = {
	"raw data(have bk) (lcm off)",
	"raw data(no bk) (lcm off)",
	"noise peak to peak(with panel) (lcm off)",
	"noise peak to peak(ic only) (lcm off)",
	"raw data_td (lcm off)",
	"peak to peak_td (lcm off)"
};

#define DEF_OLD_TEST	6
static char run_old_test[DEF_OLD_TEST][64] = {
	"untouch peak to peak",
	"open test(integration)",
	"open test(cap)",
	"untouch cm data",
	"pixel raw (no bk)",
	"pixel raw (have bk)"
};

#define MP_TEST_ITEM	49
static struct mp_test_items tItems[MP_TEST_ITEM] = {
	{.name = "mutual_dac", .desp = "calibration data(dac)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_bg", .desp = "baseline data(bg)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_signal", .desp = "untouch signal data(bg-raw-4096) - mutual", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_no_bk", .desp = "raw data(no bk)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_has_bk", .desp = "raw data(have bk)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_bk_dac", .desp = "manual bk data(mutual)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "self_dac", .desp = "calibration data(dac) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "self_bg", .desp = "baselin data(bg,self_tx,self_r)", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "self_signal", .desp = "untouch signal data(bg-raw-4096) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "self_no_bk", .desp = "raw data(no bk) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "self_has_bk", .desp = "raw data(have bk) - self", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "self_bk_dac", .desp = "manual bk dac data(self_tx,self_rx)", .result = "FAIL", .catalog = SELF_TEST},
	{.name = "key_dac", .desp = "calibration data(dac/icon)", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "key_bg", .desp = "key baseline data", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "key_no_bk", .desp = "key raw data", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "key_has_bk", .desp = "key raw bk dac", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "key_open", .desp = "key raw open test", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "key_short", .desp = "key raw short test", .result = "FAIL", .catalog = KEY_TEST},
	{.name = "st_dac", .desp = "st calibration data(dac)", .result = "FAIL", .catalog = ST_TEST},
	{.name = "st_bg", .desp = "st baseline data(bg)", .result = "FAIL", .catalog = ST_TEST},
	{.name = "st_no_bk", .desp = "st raw data(no bk)", .result = "FAIL", .catalog = ST_TEST},
	{.name = "st_has_bk", .desp = "st raw(have bk)", .result = "FAIL", .catalog = ST_TEST},
	{.name = "st_open", .desp = "st open data", .result = "FAIL", .catalog = ST_TEST},
	{.name = "tx_short", .desp = "tx short test", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "rx_short", .desp = "short test -ili9881", .result = "FAIL", .catalog = SHORT_TEST},
	{.name = "rx_open", .desp = "rx open", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "cm_data", .desp = "untouch cm data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "cs_data", .desp = "untouch cs data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "tx_rx_delta", .desp = "tx/rx delta", .result = "FAIL", .catalog = TX_RX_DELTA},
	{.name = "p2p", .desp = "untouch peak to peak", .result = "FAIL", .catalog = UNTOUCH_P2P},
	{.name = "pixel_no_bk", .desp = "pixel raw (no bk)", .result = "FAIL", .catalog = PIXEL},
	{.name = "pixel_has_bk", .desp = "pixel raw (have bk)", .result = "FAIL", .catalog = PIXEL},
	{.name = "open_integration", .desp = "open test(integration)", .result = "FAIL", .catalog = OPEN_TEST},
	{.name = "open_cap", .desp = "open test(cap)", .result = "FAIL", .catalog = OPEN_TEST},
	/* New test items for protocol 5.4.0 as below */
	{.name = "noise_peak_to_peak_ic", .desp = "noise peak to peak(ic only)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "noise_peak_to_peak_panel", .desp = "noise peak to peak(with panel)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "noise_peak_to_peak_ic_lcm_off", .desp = "noise peak to peak(ic only) (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "noise_peak_to_peak_panel_lcm_off", .desp = "noise peak to peak(with panel) (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "mutual_no_bk_lcm_off", .desp = "raw data(no bk) (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "mutual_has_bk_lcm_off", .desp = "raw data(have bk) (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "open_integration_sp", .desp = "open test(integration)_sp", .result = "FAIL", .catalog = OPEN_TEST},
	{.name = "doze_raw", .desp = "doze raw data", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "doze_p2p", .desp = "doze peak to peak", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "doze_raw_td_lcm_off", .desp = "raw data_td (lcm off)", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "doze_p2p_td_lcm_off", .desp = "peak to peak_td (lcm off)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
	{.name = "rx_short", .desp = "short test", .result = "FAIL", .catalog = SHORT_TEST},
	{.name = "open test_c", .desp = "open test_c", .result = "FAIL", .catalog = OPEN_TEST},
	{.name = "touch deltac", .desp = "touch deltac", .result = "FAIL", .catalog = MUTUAL_TEST},
	{.name = "pin test", .desp = "pin test ( int and rst )", .result = "FAIL", .catalog = PIN_TEST},
};

s32 *frame_buf;
s32 *key_buf;
s32 *frame1_cbk700, *frame1_cbk250, *frame1_cbk200;
s32 *cap_dac, *cap_raw;
int g_ini_items;

static int isspace_t(int x)
{
	if (x == ' ' || x == '\t' || x == '\n' ||
			x == '\f' || x == '\b' || x == '\r')
		return 1;
	else
		return 0;
}

static void dump_benchmark_data(s32 *max_ptr, s32 *min_ptr)
{
	int i;

	if (ipio_debug_level) {
		ipio_info("Dump Benchmark Max\n");

		for (i = 0; i < core_mp.frame_len; i++) {
			pr_cont("%d, ", max_ptr[i]);
			if (i % core_mp.xch_len == core_mp.xch_len - 1)
				pr_cont("\n");
		}

		pr_cont("Dump Denchmark Min\n");

		for (i = 0; i < core_mp.frame_len; i++) {
			pr_cont("%d, ", min_ptr[i]);
			if (i % core_mp.xch_len == core_mp.xch_len - 1)
				pr_cont("\n");
		}
	}
}

void dump_node_type_buffer(s32 *node_ptr, u8 *name)
{
	int i;

	if (ipio_debug_level) {
		ipio_info("Dump NodeType\n");
		for (i = 0; i < core_mp.frame_len; i++) {
			pr_cont("%d, ", node_ptr[i]);
			if (i % core_mp.xch_len == core_mp.xch_len-1)
				pr_cont("\n");
		}
	}
}

static int parser_get_ini_key_value(char *section, char *key, char *value)
{
	int i = 0;
	int ret = -2;
	int len = 0;

	len = strlen(key);

	for (i = 0; i < g_ini_items; i++) {
		if (strncmp(section, ilitek_ini_file_data[i].pSectionName, strlen(section)) != 0)
			continue;

		if (strncmp(key, ilitek_ini_file_data[i].pKeyName, strlen(key)) == 0) {
			ipio_memcpy(value, ilitek_ini_file_data[i].pKeyValue, ilitek_ini_file_data[i].iKeyValueLen, PARSER_MAX_KEY_VALUE_LEN);
			ipio_debug(" value:%s , pKeyValue: %s\n", value, ilitek_ini_file_data[i].pKeyValue);
			ret = 0;
			break;
		}
	}
	return ret;
}

void parser_ini_nodetype(s32 *type_ptr, char *desp, int frame_len)
{
	int i = 0, j = 0, index1 = 0, temp, count = 0;
	char str[512] = {0}, record = ',';

	for (i = 0; i < g_ini_items; i++) {
		if ((strstr(ilitek_ini_file_data[i].pSectionName, desp) <= 0) ||
			strncmp(ilitek_ini_file_data[i].pKeyName, NODE_TYPE_KEY_NAME, strlen(ilitek_ini_file_data[i].pKeyName)) != 0) {
			continue;
		}

		record = ',';
		for (j = 0, index1 = 0; j <= ilitek_ini_file_data[i].iKeyValueLen; j++) {
			if (ilitek_ini_file_data[i].pKeyValue[j] == ';' || j == ilitek_ini_file_data[i].iKeyValueLen) {
				if (record != '.') {
					memset(str, 0, sizeof(str));
					ipio_memcpy(str, &ilitek_ini_file_data[i].pKeyValue[index1], (j - index1), sizeof(str));
					temp = katoi(str);

					/* Over boundary, end to calculate. */
					if (count >= frame_len) {
						ipio_err("count(%d) is larger than frame length, break\n", count);
						break;
					}
					type_ptr[count] = temp;
					count++;
				}
				record = ilitek_ini_file_data[i].pKeyValue[j];
				index1 = j + 1;
			}
		}
	}
}

void parser_ini_benchmark(s32 *max_ptr, s32 *min_ptr, int8_t type, char *desp, int frame_len)
{
	int i = 0, j = 0, index1 = 0, temp, count = 0;
	char str[512] = {0}, record = ',';
	s32 data[4];
	char benchmark_str[256] = {0};

	/* format complete string from the name of section "_Benchmark_Data". */
	snprintf(benchmark_str, sizeof(benchmark_str), "%s%s%s", desp, "_", BENCHMARK_KEY_NAME);

	for (i = 0; i < g_ini_items; i++) {
		if ((strncmp(ilitek_ini_file_data[i].pSectionName, benchmark_str, strlen(ilitek_ini_file_data[i].pSectionName)) != 0) ||
			strncmp(ilitek_ini_file_data[i].pKeyName, BENCHMARK_KEY_NAME, strlen(ilitek_ini_file_data[i].pSectionName)) != 0)
			continue;
		record = ',';
		for (j = 0, index1 = 0; j <= ilitek_ini_file_data[i].iKeyValueLen; j++) {
			if (ilitek_ini_file_data[i].pKeyValue[j] == ',' || ilitek_ini_file_data[i].pKeyValue[j] == ';' ||
				ilitek_ini_file_data[i].pKeyValue[j] == '.' || j == ilitek_ini_file_data[i].iKeyValueLen) {

				if (record != '.') {
					memset(str, 0, sizeof(str));
					ipio_memcpy(str, &ilitek_ini_file_data[i].pKeyValue[index1], (j - index1), sizeof(str));
					temp = katoi(str);
					data[(count % 4)] = temp;

					/* Over boundary, end to calculate. */
					if ((count / 4) >= frame_len) {
						ipio_err("count (%d) is larger than frame length, break\n", (count / 4));
						break;
					}

					if ((count % 4) == 3) {
						if (data[0] == 1) {
							if (type == VALUE) {
								max_ptr[count/4] = data[1] + data[2];
								min_ptr[count/4] = data[1] - data[3];
							} else {
								max_ptr[count/4] = data[1] + (data[1] * data[2]) / 100;
								min_ptr[count/4] = data[1] - (data[1] * data[3]) / 100;
							}
						} else {
							max_ptr[count/4] = INT_MAX;
							min_ptr[count/4] = INT_MIN;
						}
					}
					count++;
				}
				record = ilitek_ini_file_data[i].pKeyValue[j];
				index1 = j + 1;
			}
		}
	}
}

static int parser_get_tdf_value(char *str, int catalog)
{
	u32 i, ans, index = 0, flag = 0, count = 0, size;
	char s[10] = {0};

	if (!str) {
		ipio_err("String is null\n");
		return -1;
	}

	size = strlen(str);
	for (i = 0, count = 0; i < size; i++) {
		if (str[i] == '.') {
			flag = 1;
			continue;
		}
		s[index++] = str[i];
		if (flag)
			count++;
	}
	ans = katoi(s);

	/* Multiply by 100 to shift out of decimal point */
	if (catalog == SHORT_TEST) {
		if (count == 0)
			ans = ans * 100;
		else if (count == 1)
			ans = ans * 10;
	}

	return ans;
}

static int parser_get_u8_array(char *key, u8 *buf, u16 base, int len)
{
	char *s = key;
	char *pToken;
	int ret, conut = 0;
	long s_to_long = 0;

	if (strlen(s) == 0 || len <= 0) {
		ipio_err("Can't find any characters inside buffer\n");
		return -1;
	}

	/*
	 *	@base: The number base to use. The maximum supported base is 16. If base is
	 *	given as 0, then the base of the string is automatically detected with the
	 *	conventional semantics - If it begins with 0x the number will be parsed as a
	 *	hexadecimal (case insensitive), if it otherwise begins with 0, it will be
	 *	parsed as an octal number. Otherwise it will be parsed as a decimal.
	 */
	if (isspace_t((int)(unsigned char)*s) == 0) {
		while ((pToken = strsep(&s, ",")) != NULL) {
			ret = kstrtol(pToken, base, &s_to_long);
			if (ret == 0)
				buf[conut] = s_to_long;
			else
				ipio_info("convert string too long, ret = %d\n", ret);
			conut++;

			if (conut >= len)
				break;
		}
	}

	return conut;
}

static int parser_get_int_data(char *section, char *keyname, char *rv, int rv_len)
{
	int len = 0;
	char value[512] = { 0 };

	if (rv == NULL || section == NULL || keyname == NULL) {
		ipio_err("Parameters are invalid\n");
		return -EINVAL;
	}

	/* return a white-space string if get nothing */
	if (parser_get_ini_key_value(section, keyname, value) < 0) {
		snprintf(rv, rv_len, "%s", value);
		return 0;
	}

	len = snprintf(rv, rv_len, "%s", value);
	return len;
}

/* Count the number of each line and assign the content to tmp buffer */
static int parser_get_ini_phy_line(char *data, char *buffer, int maxlen)
{
	int i = 0;
	int j = 0;
	int iRetNum = -1;
	char ch1 = '\0';

	for (i = 0, j = 0; i < maxlen; j++) {
		ch1 = data[j];
		iRetNum = j + 1;
		if (ch1 == '\n' || ch1 == '\r') {	/* line end */
			ch1 = data[j + 1];
			if (ch1 == '\n' || ch1 == '\r')
				iRetNum++;
			break;
		} else if (ch1 == 0x00) {
			//iRetNum = -1;
			break;	/* file end */
		}

		buffer[i++] = ch1;
	}

	buffer[i] = '\0';
	return iRetNum;
}

static char *parser_ini_str_trim_r(char *buf)
{
	int len, i;
	char tmp[512] = { 0 };

	len = strlen(buf);

	for (i = 0; i < len; i++) {
		if (buf[i] != ' ')
			break;
	}

	if (i < len)
		strncpy(tmp, (buf + i), (len - i));

	strncpy(buf, tmp, len);
	return buf;
}

static int parser_get_ini_phy_data(char *data, int fsize)
{
	int i, n = 0, ret = 0, banchmark_flag = 0, empty_section, nodetype_flag = 0;
	int offset = 0, isEqualSign = 0;
	char *ini_buf = NULL, *tmpSectionName = NULL;
	char M_CFG_SSL = '[';
	char M_CFG_SSR = ']';
/* char M_CFG_NIS = ':'; */
	char M_CFG_NTS = '#';
	char M_CFG_EQS = '=';

	if (data == NULL) {
		ipio_err("INI data is NULL\n");
		ret = -EINVAL;
		goto out;
	}

	ini_buf = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ini_buf)) {
		ipio_err("Failed to allocate ini_buf memory, %ld\n", PTR_ERR(ini_buf));
		ret = -ENOMEM;
		goto out;
	}

	tmpSectionName = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
	if (ERR_ALLOC_MEM(tmpSectionName)) {
		ipio_err("Failed to allocate tmpSectionName memory, %ld\n", PTR_ERR(tmpSectionName));
		ret = -ENOMEM;
		goto out;
	}

	while (true) {
		banchmark_flag = 0;
		empty_section = 0;
		nodetype_flag = 0;
		if (g_ini_items > PARSER_MAX_KEY_NUM) {
			ipio_err("MAX_KEY_NUM: Out of length\n");
			goto out;
		}

		if (offset >= fsize)
			goto out;/*over size*/

		n = parser_get_ini_phy_line(data + offset, ini_buf, PARSER_MAX_CFG_BUF);
		if (n < 0) {
			ipio_err("End of Line\n");
			goto out;
		}

		offset += n;

		n = strlen(parser_ini_str_trim_r(ini_buf));

		if (n == 0 || ini_buf[0] == M_CFG_NTS)
			continue;

		/* Get section names */
		if (n > 2 && ((ini_buf[0] == M_CFG_SSL && ini_buf[n - 1] != M_CFG_SSR))) {
			ipio_err("Bad Section: %s\n", ini_buf);
			ret = -EINVAL;
			goto out;
		} else {
			if (ini_buf[0] == M_CFG_SSL) {
				ilitek_ini_file_data[g_ini_items].iSectionNameLen = n - 2;
				if (ilitek_ini_file_data[g_ini_items].iSectionNameLen > PARSER_MAX_KEY_NAME_LEN) {
					ipio_err("MAX_KEY_NAME_LEN: Out Of Length\n");
					ret = INI_ERR_OUT_OF_LINE;
					goto out;
				}

				ini_buf[n - 1] = 0x00;
				strncpy((char *)tmpSectionName, ini_buf + 1, (PARSER_MAX_CFG_BUF + 1) * sizeof(char));
				banchmark_flag = 0;
				nodetype_flag = 0;
				ipio_debug("Section Name: %s, Len: %d, offset = %d\n", tmpSectionName, n - 2, offset);
				continue;
			}
		}

		/* copy section's name without square brackets to its real buffer */
		strncpy(ilitek_ini_file_data[g_ini_items].pSectionName, tmpSectionName, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
		ilitek_ini_file_data[g_ini_items].iSectionNameLen = strlen(tmpSectionName);

		isEqualSign = 0;
		for (i = 0; i < n; i++) {
			if (ini_buf[i] == M_CFG_EQS) {
				isEqualSign = i;
				break;
			}
			if (ini_buf[i] == M_CFG_SSL || ini_buf[i] == M_CFG_SSR) {
				empty_section = 1;
				break;
			}
		}

		if (isEqualSign == 0) {
			if (empty_section)
				continue;

			if (strstr(ilitek_ini_file_data[g_ini_items].pSectionName, BENCHMARK_KEY_NAME) > 0) {
				banchmark_flag = 1;
				isEqualSign = -1;
			} else if (strstr(ilitek_ini_file_data[g_ini_items].pSectionName, NODE_TYPE_KEY_NAME) > 0) {
				nodetype_flag = 1;
				isEqualSign = -1;
			} else {
				continue;
			}
		}

		if (banchmark_flag) {
			ilitek_ini_file_data[g_ini_items].iKeyNameLen = strlen(BENCHMARK_KEY_NAME);
			strncpy(ilitek_ini_file_data[g_ini_items].pKeyName, BENCHMARK_KEY_NAME, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
			ilitek_ini_file_data[g_ini_items].iKeyValueLen = n;
		} else if (nodetype_flag) {
			ilitek_ini_file_data[g_ini_items].iKeyNameLen = strlen(NODE_TYPE_KEY_NAME);
			strncpy(ilitek_ini_file_data[g_ini_items].pKeyName, NODE_TYPE_KEY_NAME, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
			ilitek_ini_file_data[g_ini_items].iKeyValueLen = n;
		} else{
			ilitek_ini_file_data[g_ini_items].iKeyNameLen = isEqualSign;
			if (ilitek_ini_file_data[g_ini_items].iKeyNameLen > PARSER_MAX_KEY_NAME_LEN) {
				/* ret = CFG_ERR_OUT_OF_LEN; */
				ipio_err("MAX_KEY_NAME_LEN: Out Of Length\n");
				ret = INI_ERR_OUT_OF_LINE;
				goto out;
			}

			ipio_memcpy(ilitek_ini_file_data[g_ini_items].pKeyName, ini_buf,
						ilitek_ini_file_data[g_ini_items].iKeyNameLen, PARSER_MAX_KEY_NAME_LEN);
			ilitek_ini_file_data[g_ini_items].iKeyValueLen = n - isEqualSign - 1;
		}

		if (ilitek_ini_file_data[g_ini_items].iKeyValueLen > PARSER_MAX_KEY_VALUE_LEN) {
			ipio_err("MAX_KEY_VALUE_LEN: Out Of Length\n");
			ret = INI_ERR_OUT_OF_LINE;
			goto out;
		}

		ipio_memcpy(ilitek_ini_file_data[g_ini_items].pKeyValue,
			   ini_buf + isEqualSign + 1, ilitek_ini_file_data[g_ini_items].iKeyValueLen, PARSER_MAX_KEY_VALUE_LEN);

		ipio_debug("%s = %s\n", ilitek_ini_file_data[g_ini_items].pKeyName,
			ilitek_ini_file_data[g_ini_items].pKeyValue);

		g_ini_items++;
	}
out:
	ipio_kfree((void **)&ini_buf);
	ipio_kfree((void **)&tmpSectionName);
	return ret;
}

static int ilitek_tddi_mp_ini_parser(const char *path)
{
	int i, ret = 0, fsize = 0;
	char *tmp = NULL;
	struct file *f = NULL;
	struct inode *inode;
	mm_segment_t old_fs;
	loff_t pos = 0;

	ipio_info("ini file path = %s\n", path);

	f = filp_open(path, O_RDONLY, 644);
	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open ini file at %ld.\n", PTR_ERR(f));
		return -ENOENT;
	}

#if KERNEL_VERSION(3, 18, 0) >= LINUX_VERSION_CODE
	inode = f->f_dentry->d_inode;
#else
	inode = f->f_path.dentry->d_inode;
#endif

	fsize = inode->i_size;
	ipio_info("ini file size = %d\n", fsize);
	if (fsize <= 0) {
		ipio_err("The size of file is invaild\n");
		ret = -EINVAL;
		goto out;
	}

	tmp = vmalloc(fsize+1);
	if (ERR_ALLOC_MEM(tmp)) {
		ipio_err("Failed to allocate tmp memory, %ld\n", PTR_ERR(tmp));
		ret = -ENOMEM;
		goto out;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(f, tmp, fsize, &pos);
	set_fs(old_fs);
	tmp[fsize] = 0x00;

	g_ini_items = 0;

	/* Initialise ini strcture */
	for (i = 0; i < PARSER_MAX_KEY_NUM; i++) {
		memset(ilitek_ini_file_data[i].pSectionName, 0, PARSER_MAX_KEY_NAME_LEN);
		memset(ilitek_ini_file_data[i].pKeyName, 0, PARSER_MAX_KEY_NAME_LEN);
		memset(ilitek_ini_file_data[i].pKeyValue, 0, PARSER_MAX_KEY_VALUE_LEN);
		ilitek_ini_file_data[i].iSectionNameLen = 0;
		ilitek_ini_file_data[i].iKeyNameLen = 0;
		ilitek_ini_file_data[i].iKeyValueLen = 0;
	}

	/* change all characters to lower case */
	for (i = 0; i < fsize; i++)
		tmp[i] = tolower(tmp[i]);

	ret = parser_get_ini_phy_data(tmp, fsize);
	if (ret < 0) {
		ipio_err("Failed to get physical ini data, ret = %d\n", ret);
		goto out;
	}

	ipio_info("Parsed ini file done\n");
out:
	ipio_vfree((void **)&tmp);
	filp_close(f, NULL);
	return ret;
}

static void run_pixel_test(int index)
{
	int i, x, y;
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int tmp[4] = { 0 }, max = 0;
			int shift = y * core_mp.xch_len;
			int centre = p_comb[shift + x];

			/*
			 * if its position is in corner, the number of point
			 * we have to minus is around 2 to 3.
			 */
			if (y == 0 && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == (core_mp.ych_len - 1) && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == 0 && x == (core_mp.xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
			} else if (y == (core_mp.ych_len - 1) && x == (core_mp.xch_len - 1)) {
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

			} else if (y == (core_mp.ych_len - 1) && x != 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y != 0 && x == (core_mp.xch_len - 1)) {
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

static void run_untouch_p2p_test(int index)
{
	int x, y;
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len;

			if (p_comb[shift + x] > tItems[index].max_buf[shift + x])
				tItems[index].max_buf[shift + x] = p_comb[shift + x];

			if (p_comb[shift + x] < tItems[index].min_buf[shift + x])
				tItems[index].min_buf[shift + x] = p_comb[shift + x];

			tItems[index].buf[shift + x] =
				tItems[index].max_buf[shift + x] - tItems[index].min_buf[shift + x];
		}
	}
}

static int run_open_test(int index)
{
	int i, x, y, k, ret = 0;
	int border_x[] = {-1, 0, 1, 1, 1, 0, -1, -1};
	int border_y[] = {-1, -1, -1, 0, 1, 1, 1, 0};
	s32 *p_comb = frame_buf;

	if (strncmp(tItems[index].name, "open_integration", strlen(tItems[index].name)) == 0) {
		for (i = 0; i < core_mp.frame_len; i++)
			tItems[index].buf[i] = p_comb[i];
	} else if (strncmp(tItems[index].name, "open_cap", strlen(tItems[index].name)) == 0) {
		/*
		 * Each result is getting from a 3 by 3 grid depending on where the centre location is.
		 * So if the centre is at corner, the number of node grabbed from a grid will be different.
		 */
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				int sum = 0, avg = 0, count = 0;
				int shift = y * core_mp.xch_len;
				int centre = p_comb[shift + x];

				for (k = 0; k < 8; k++) {
					if (((y + border_y[k] >= 0) && (y + border_y[k] < core_mp.ych_len)) &&
								((x + border_x[k] >= 0) && (x + border_x[k] < core_mp.xch_len))) {
						count++;
						sum += p_comb[(y + border_y[k]) * core_mp.xch_len + (x + border_x[k])];
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
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len;

			/* Tx Delta */
			if (y != (core_mp.ych_len - 1))
				core_mp.tx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[(shift + 1) + x]);

			/* Rx Delta */
			if (x != (core_mp.xch_len - 1))
				core_mp.rx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[shift + (x + 1)]);
		}
	}
}

static char *get_date_time_str(void)
{
	struct timespec now_time;
	struct rtc_time rtc_now_time;
	static char time_data_buf[128] = { 0 };

	getnstimeofday(&now_time);
	rtc_time_to_tm(now_time.tv_sec, &rtc_now_time);
	snprintf(time_data_buf, sizeof(time_data_buf), "%04d%02d%02d-%02d%02d%02d",
		(rtc_now_time.tm_year + 1900), rtc_now_time.tm_mon + 1,
		rtc_now_time.tm_mday, rtc_now_time.tm_hour, rtc_now_time.tm_min,
		rtc_now_time.tm_sec);

	return time_data_buf;
}

static void mp_print_csv_header(char *csv, int *csv_len, int *csv_line, int file_size)
{
	int i, tmp_len = *csv_len, tmp_line = *csv_line, size;

	/* header must has 19 line*/
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "ILITek C-TP Utility V%s	%x : Driver Sensor Test\n", DRIVER_VERSION, core_mp.chip_pid);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Confidentiality Notice:\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Any information of this tool is confidential and privileged.\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "@ ILI TECHNOLOGY CORP. All Rights Reserved.\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Firmware Version ,0x%x\n", core_mp.fw_ver);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Panel information ,XCH=%d, YCH=%d\n", core_mp.xch_len, core_mp.ych_len);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Test Item:\n");
	tmp_line++;

	size = ARRAY_SIZE(tItems);
	for (i = 0; i < size; i++) {
		if (tItems[i].run == 1) {
			tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  ---%s\n", tItems[i].desp);
			tmp_line++;
		}
	}

	while (tmp_line < 19) {
		tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "\n");
		tmp_line++;
	}

	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");

	*csv_len = tmp_len;
	*csv_line = tmp_line;
}

static void mp_print_csv_tail(char *csv, int *csv_len, int file_size)
{
	int i, tmp_len = *csv_len, size;

	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Result_Summary			\n");

	size = ARRAY_SIZE(tItems);
	for (i = 0; i < size; i++) {
		if (tItems[i].run) {
			if (tItems[i].item_result == MP_DATA_PASS)
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  {%s}	   ,OK\n", tItems[i].desp);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  {%s}	   ,NG\n", tItems[i].desp);
		}
	}
	*csv_len = tmp_len;
}

static void mp_print_csv_cdc_cmd(char *csv, int *csv_len, int index, int file_size)
{
	int i, slen = 0, tmp_len = *csv_len, size;
	char str[128] = {0};
	char *open_sp_cmd[] = {"open dac", "open raw1", "open raw2", "open raw3"};
	char *open_c_cmd[] = {"open cap1 dac", "open cap1 raw"};
	char *name = tItems[index].desp;

	if (strncmp(name, "open test(integration)_sp", strlen(name)) == 0) {
		size = ARRAY_SIZE(open_sp_cmd);
		for (i = 0; i < size; i++) {
			slen = parser_get_int_data("pv5_4 command", open_sp_cmd[i], str, sizeof(str));
			if (slen < 0)
				ipio_err("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "%s = ,%s\n", open_sp_cmd[i], str);
		}
	} else if (strncmp(name, "open test_c", strlen(name)) == 0) {
		size = ARRAY_SIZE(open_c_cmd);
		for (i = 0; i < size; i++) {
			slen = parser_get_int_data("pv5_4 command", open_c_cmd[i], str, sizeof(str));
			if (slen < 0)
				ipio_err("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "%s = ,%s\n", open_c_cmd[i], str);
		}
	} else {
		slen = parser_get_int_data("pv5_4 command", name, str, sizeof(str));
		if (slen < 0)
			ipio_err("Failed to get CDC command %s from ini\n", name);
		else
			tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "CDC command = ,%s\n", str);
	}
	*csv_len = tmp_len;
}

static void mp_compare_cdc_show_result(int index, s32 *tmp, char *csv,
				int *csv_len, int type, s32 *max_ts,
				s32 *min_ts, const char *desp, int file_zise)
{
	int x, y, tmp_len = *csv_len;
	int mp_result = MP_DATA_PASS;

	if (ERR_ALLOC_MEM(tmp)) {
		ipio_err("The data of test item is null (%p)\n", tmp);
		mp_result = -EMP_INVAL;
		goto out;
	}

	/* print X raw only */
	for (x = 0; x < core_mp.xch_len; x++) {
		if (x == 0) {
			DUMP("\n %s ", desp);
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n	   %s ,", desp);
		}
		DUMP("  X_%d	,", (x+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 X_%d  ,", (x+1));
	}

	DUMP("\n");
	tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");

	for (y = 0; y < core_mp.ych_len; y++) {
		DUMP("  Y_%d	,", (y+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 Y_%d  ,", (y+1));

		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len + x;

			/* In Short teset, we only identify if its value is low than min threshold. */
			if (tItems[index].catalog == SHORT_TEST) {
				if (tmp[shift] < min_ts[shift]) {
					DUMP(" #%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "#%7d,", tmp[shift]);
					mp_result = MP_DATA_FAIL;
				} else {
					DUMP(" %7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), " %7d, ", tmp[shift]);
				}
				continue;
			}

			if ((tmp[shift] <= max_ts[shift] && tmp[shift] >= min_ts[shift]) || (type != TYPE_JUGE)) {
				if ((tmp[shift] == INT_MAX || tmp[shift] == INT_MIN) && (type == TYPE_BENCHMARK)) {
					DUMP("%s", "BYPASS,");
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "BYPASS,");
				} else {
					DUMP(" %7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), " %7d, ", tmp[shift]);
				}
			} else {
				if (tmp[shift] > max_ts[shift]) {
					DUMP(" *%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "*%7d,", tmp[shift]);
				} else {
					DUMP(" #%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "#%7d,", tmp[shift]);
				}
				mp_result = MP_DATA_FAIL;
			}
		}
		DUMP("\n");
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");
	}

out:
	if (type == TYPE_JUGE) {
		if (mp_result == MP_DATA_PASS) {
			pr_info("\n Result : PASS\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : PASS\n");
		} else {
			pr_info("\n Result : FAIL\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : FAIL\n");
		}
	}
	*csv_len = tmp_len;
}

#define ABS(a, b) ((a > b) ? (a - b) : (b - a))
#define ADDR(x, y) ((y * core_mp.xch_len) + (x))

int compare_charge(s32 *charge_rate, int x, int y, s32 *inNodeType,
		int Charge_AA, int Charge_Border, int Charge_Notch)
{
	int OpenThreadhold, tempY, tempX, ret, k;
	int sx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
	int sy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

	ret = charge_rate[ADDR(x, y)];

	/*Setting Threadhold from node type	 */
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

		/*out of range */
		if ((tempX < 0) || (tempX >= core_mp.xch_len) || (tempY < 0) || (tempY >= core_mp.ych_len))
			continue;

		if ((inNodeType[ADDR(tempX, tempY)] == NO_COMPARE) || ((inNodeType[ADDR(tempX, tempY)] & Round_Corner) == Round_Corner) ||
		((inNodeType[ADDR(tempX, tempY)] & Skip_Micro) == Skip_Micro) || charge_rate[ADDR(tempX, tempY)] == 0)
			continue;

		if ((charge_rate[ADDR(tempX, tempY)] - charge_rate[ADDR(x, y)]) > OpenThreadhold)
			return OpenThreadhold;
	}
	return ret;
}

int full_open_rate_compare(s32 *full_open, s32 *cbk, int x, int y, s32 inNodeType, int full_open_rate)
{
	int ret = true;

	if ((inNodeType == NO_COMPARE) || ((inNodeType & Round_Corner) == Round_Corner))
		return true;

	if (full_open[ADDR(x, y)] < (cbk[ADDR(x, y)] * full_open_rate / 100))
		ret = false;

	return ret;
}

void allnode_open_cdc_result(int index, int *buf, int *dac, int *raw)
{
	int i;
	char *name = tItems[index].name;

	if (strncmp(name, "open_integration_sp", strlen(name)) == 0) {
		for (i = 0; i < core_mp.frame_len; i++)
			buf[i] = idev->chip->open_sp_formula(dac[i], raw[i], open_spec.tvch, open_spec.tvcl);
	} else if (strncmp(name, "open test_c", strlen(name)) == 0) {
		for (i = 0; i < core_mp.frame_len; i++)
			buf[i] = idev->chip->open_c_formula(dac[i], raw[i], open_spec.tvch - open_spec.tvcl, open_spec.gain);
	}
}

static int codeToOhm(s32 Code, u16 *v_tdf, u16 *h_tdf)
{
	int douTDF1 = 0;
	int douTDF2 = 0;
	int douTVCH = 24;
	int douTVCL = 8;
	int douCint = 7;
	int douVariation = 64;
	int douRinternal = 930;
	s32 temp = 0;

	if (core_mp.isLongV) {
		douTDF1 = *v_tdf;
		douTDF2 = *(v_tdf + 1);
	} else {
		douTDF1 = *h_tdf;
		douTDF2 = *(h_tdf + 1);
	}

	if (Code == 0) {
		ipio_err("code is invalid\n");
	} else {
		temp = ((douTVCH - douTVCL) * douVariation * (douTDF1 - douTDF2) * (1 << 12) / (9 * Code * douCint)) * 100;
		temp = (temp - douRinternal) / 1000;
	}
	/* Unit = M Ohm */
	return temp;
}

static int short_test(int index, int frame_index)
{
	int j = 0, ret = 0;
	u16 v_tdf[2] = {0};
	u16 h_tdf[2] = {0};

	v_tdf[0] = tItems[index].v_tdf_1;
	v_tdf[1] = tItems[index].v_tdf_2;
	h_tdf[0] = tItems[index].h_tdf_1;
	h_tdf[1] = tItems[index].h_tdf_2;

	if (core_mp.protocol_ver >= PROTOCOL_VER_540) {
		/* Calculate code to ohm and save to tItems[index].buf */
		for (j = 0; j < core_mp.frame_len; j++)
			tItems[index].buf[frame_index * core_mp.frame_len + j] = codeToOhm(frame_buf[j], v_tdf, h_tdf);
	} else {
		for (j = 0; j < core_mp.frame_len; j++)
			tItems[index].buf[frame_index * core_mp.frame_len + j] = frame_buf[j];
	}

	return ret;
}

static int allnode_key_cdc_data(int index)
{
	int i, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[3] = {0};
	u8 *ori = NULL;

	len = core_mp.key_len * 2;

	ipio_debug("Read key's length = %d\n", len);
	ipio_debug("core_mp.key_len = %d\n", core_mp.key_len);

	if (len <= 0) {
		ipio_err("Length is invalid\n");
		ret = -1;
		goto out;
	}

	/* CDC init */
	cmd[0] = P5_X_SET_CDC_INIT;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	atomic_set(&idev->mp_int_check, ENABLE);

	ret = idev->write(cmd, 3);
	if (ret < 0) {
		ipio_err("Write CDC command failed\n");
		goto out;
	}

	/* Check busy */
	if (core_mp.busy_cdc == POLL_CHECK)
		ret = ilitek_tddi_ic_check_busy(50, 50);
	else
		ret = ilitek_tddi_ic_check_int_stat();

	if (ret < 0)
		goto out;

	/* Prepare to get cdc data */
	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_GET_CDC_DATA;

	ret = idev->write(cmd, 2);
	if (ret < 0) {
		ipio_err("Write (0x%x, 0x%x) error\n", cmd[0], cmd[1]);
		goto out;
	}

	/* Waiting for FW to prepare cdc data */
	mdelay(1);

	ret = idev->write(&cmd[1], 1);
	if (ret < 0) {
		ipio_err("Write (0x%x) error\n", cmd[1]);
		goto out;
	}

	/* Waiting for FW to prepare cdc data */
	mdelay(1);

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
		goto out;
	}

	/* Get original frame(cdc) data */
	ret = idev->read(ori, len);
	if (ret < 0) {
		ipio_err("Read cdc data error, len = %d\n", len);
		goto out;
	}

	ilitek_dump_data(ori, 8, len, 0, "Key CDC original");

	if (key_buf == NULL) {
		key_buf = kcalloc(core_mp.key_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(key_buf)) {
			ipio_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(key_buf));
			goto out;
		}
	} else {
		memset(key_buf, 0x0, core_mp.key_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp.frame_len; i++) {
		if (tItems[index].cmd == CMD_KEY_DAC) {
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
	ilitek_dump_data(key_buf, 32, core_mp.frame_len, core_mp.xch_len, "Key CDC combined data");

out:
	atomic_set(&idev->mp_int_check, DISABLE);
	ipio_kfree((void **)&ori);
	return ret;
}

static int mp_cdc_get_pv5_4_command(u8 *cmd, int len, int index)
{
	int slen = 0;
	char str[128] = {0};
	char *key = tItems[index].desp;

	slen = parser_get_int_data("pv5_4 command", key, str, sizeof(str));
	if (slen < 0)
		return -1;

	if (parser_get_u8_array(str, cmd, 16, len) < 0)
		return -1;

	return 0;
}

static int mp_cdc_init_cmd_common(u8 *cmd, int len, int index)
{
	int ret = 0;

	if (core_mp.protocol_ver >= PROTOCOL_VER_540) {
		core_mp.cdc_len = 15;
		return mp_cdc_get_pv5_4_command(cmd, len, index);
	}

	cmd[0] = P5_X_SET_CDC_INIT;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	core_mp.cdc_len = 3;

	if (strncmp(tItems[index].name, "open_integration", strlen(tItems[index].name)) == 0)
		cmd[2] = 0x2;
	if (strncmp(tItems[index].name, "open_cap", strlen(tItems[index].name)) == 0)
		cmd[2] = 0x3;

	if (tItems[index].catalog == PEAK_TO_PEAK_TEST) {
		cmd[2] = ((tItems[index].frame_count & 0xff00) >> 8);
		cmd[3] = tItems[index].frame_count & 0xff;
		cmd[4] = 0;

		core_mp.cdc_len = 5;

		if (strncmp(tItems[index].name, "noise_peak_to_peak_cut", strlen(tItems[index].name)) == 0)
			cmd[4] = 0x1;

		ipio_debug("P2P CMD: %d,%d,%d,%d,%d\n",
				cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	}

	return ret;
}

static int allnode_open_cdc_data(int mode, int *buf)
{
	int i = 0, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[15] = {0};
	u8 *ori = NULL;
	char str[128] = {0};
	char *key[] = {"open dac", "open raw1", "open raw2", "open raw3",
			"open cap1 dac", "open cap1 raw"};

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (core_mp.xch_len * core_mp.ych_len * 2) + 2;

	ipio_debug("Read X/Y Channel length = %d, mode = %d\n", len, mode);

	if (len <= 2) {
		ipio_err("Length is invalid\n");
		ret = -EMP_INVAL;
		goto out;
	}

	/* CDC init. Read command from ini file */
	if (parser_get_int_data("pv5_4 command", key[mode], str, sizeof(str)) < 0) {
		ipio_err("Failed to parse PV54 command, ret = %d\n", ret);
		ret = -EMP_PARSE;
		goto out;
	}

	parser_get_u8_array(str, cmd, 16, sizeof(cmd));

	ilitek_dump_data(cmd, 8, sizeof(cmd), 0, "Open SP command");

	atomic_set(&idev->mp_int_check, ENABLE);

	if (idev->write(cmd, core_mp.cdc_len) < 0) {
		ipio_err("Write CDC command failed\n");
		ret = -EMP_CMD;
		goto out;
	}

	/* Check busy */
	if (core_mp.busy_cdc == POLL_CHECK)
		ret = ilitek_tddi_ic_check_busy(50, 50);
	else
		ret = ilitek_tddi_ic_check_int_stat();

	if (ret < 0) {
		ret = -EMP_CHECK_BUY;
		goto out;
	}

	/* Prepare to get cdc data */
	cmd[0] = P5_X_READ_DATA_CTRL;
	cmd[1] = P5_X_GET_CDC_DATA;

	if (idev->write(cmd, 2) < 0) {
		ipio_err("Write (0x%x, 0x%x) error\n", cmd[0], cmd[1]);
		ret = -EMP_CMD;
		goto out;
	}

	/* Waiting for FW to prepare cdc data */
	mdelay(1);

	if (idev->write(&cmd[1], 1) < 0) {
		ipio_err("Write (0x%x) error\n", cmd[1]);
		ret = -EMP_GET_CDC;
		goto out;
	}

	/* Waiting for FW to prepare cdc data */
	mdelay(1);

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori, (%ld)\n", PTR_ERR(ori));
		ret = -EMP_NOMEM;
		goto out;
	}

	/* Get original frame(cdc) data */
	if (idev->read(ori, len) < 0) {
		ipio_err("Read cdc data error, len = %d\n", len);
		ret = -EMP_GET_CDC;
		goto out;
	}

	ilitek_dump_data(ori, 8, len, 0, "Open SP CDC original");

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp.frame_len; i++) {
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

			buf[i] = inDACp + inDACn;
		} else {
			/* H byte + L byte */
			s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];
			if ((tmp & 0x8000) == 0x8000)
				buf[i] = tmp - 65536;
			else
				buf[i] = tmp;

		}
	}
	ilitek_dump_data(buf, 10, core_mp.frame_len,  core_mp.xch_len, "Open SP CDC combined");
out:
	atomic_set(&idev->mp_int_check, DISABLE);
	ipio_kfree((void **)&ori);
	return ret;
}

static int allnode_mutual_cdc_data(int index)
{
	int i, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[15] = {0};
	u8 *ori = NULL;

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (core_mp.xch_len * core_mp.ych_len * 2) + 2;

	ipio_debug("Read X/Y Channel length = %d\n", len);

	if (len <= 2) {
		ipio_err("Length is invalid\n");
		ret = -EMP_INVAL;
		goto out;
	}

	memset(cmd, 0xFF, sizeof(cmd));

	/* CDC init */
	if (mp_cdc_init_cmd_common(cmd, sizeof(cmd), index) < 0) {
		ipio_err("Failed to get cdc command\n");
		ret = -EMP_CMD;
		goto out;
	}

	ilitek_dump_data(cmd, 8, core_mp.cdc_len, 0, "Mutual CDC command");

	atomic_set(&idev->mp_int_check, ENABLE);

	if (idev->write(cmd, core_mp.cdc_len) < 0) {
		ipio_err("Write CDC command failed\n");
		ret = -EMP_CMD;
		goto out;
	}

	/* Check busy */
	if (core_mp.busy_cdc == POLL_CHECK)
		ret = ilitek_tddi_ic_check_busy(50, 50);
	else
		ret = ilitek_tddi_ic_check_int_stat();

	if (ret < 0) {
		ret = -EMP_CHECK_BUY;
		goto out;
	}

	if (core_mp.core_ver < CORE_VER_1430) {
		/* Prepare to get cdc data */
		cmd[0] = P5_X_READ_DATA_CTRL;
		cmd[1] = P5_X_GET_CDC_DATA;

		if (idev->write(cmd, 2) < 0) {
			ipio_err("Write (0x%x, 0x%x) error\n", cmd[0], cmd[1]);
			ret = -EMP_CMD;
			goto out;
		}

		/* Waiting for FW to prepare cdc data */
		mdelay(1);

		if (idev->write(&cmd[1], 1) < 0) {
			ipio_err("Write (0x%x) error\n", cmd[1]);
			ret = -EMP_CMD;
			goto out;
		}

		/* Waiting for FW to prepare cdc data */
		mdelay(1);
	}

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ipio_err("Failed to allocate ori, (%ld)\n", PTR_ERR(ori));
		ret = -EMP_NOMEM;
		goto out;
	}

	/* Get original frame(cdc) data */
	if (idev->read(ori, len) < 0) {
		ipio_err("Read cdc data error, len = %d\n", len);
		ret = -EMP_GET_CDC;
		goto out;
	}

	ilitek_dump_data(ori, 8, len, 0, "Mutual CDC original");

	if (frame_buf == NULL) {
		frame_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame_buf)) {
			ipio_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(frame_buf));
			ret = -EMP_NOMEM;
			goto out;
		}
	} else {
		memset(frame_buf, 0x0, core_mp.frame_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp.frame_len; i++) {
		if (tItems[index].cmd == CMD_MUTUAL_DAC) {
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
			s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];

			if ((tmp & 0x8000) == 0x8000)
				frame_buf[i] = tmp - 65536;
			else
				frame_buf[i] = tmp;

			if (strncmp(tItems[index].name, "mutual_no_bk", strlen("mutual_no_bk")) == 0 ||
				strncmp(tItems[index].name, "mutual_no_bk_lcm_off", strlen("mutual_no_bk_lcm_off")) == 0) {
					frame_buf[i] -= core_mp.no_bk_shift;
			}
		}
	}

	ilitek_dump_data(frame_buf, 32, core_mp.frame_len, core_mp.xch_len, "Mutual CDC combined");

out:
	atomic_set(&idev->mp_int_check, DISABLE);
	ipio_kfree((void **)&ori);
	return ret;
}

static void compare_MaxMin_result(int index, s32 *data)
{
	int x, y;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len;

			if (tItems[index].catalog == UNTOUCH_P2P)
				return;
			else if (tItems[index].catalog == TX_RX_DELTA) {
				/* Tx max/min comparison */
				if (core_mp.tx_delta_buf[shift + x] < data[shift + x])
					core_mp.tx_max_buf[shift + x] = data[shift + x];

				if (core_mp.tx_delta_buf[shift + x] > data[shift + x])
					core_mp.tx_min_buf[shift + x] = data[shift + x];

				/* Rx max/min comparison */
				if (core_mp.rx_delta_buf[shift + x] < data[shift + x])
					core_mp.rx_max_buf[shift + x] = data[shift + x];

				if (core_mp.rx_delta_buf[shift + x] > data[shift + x])
					core_mp.rx_min_buf[shift + x] = data[shift + x];
			} else {
				if (tItems[index].max_buf[shift + x] < data[shift + x])
					tItems[index].max_buf[shift + x] = data[shift + x];

				if (tItems[index].min_buf[shift + x] > data[shift + x])
					tItems[index].min_buf[shift + x] = data[shift + x];
			}
		}
	}
}

static int create_mp_test_frame_buffer(int index, int frame_count)
{
	ipio_debug("Create MP frame buffers (index = %d), count = %d\n",
			index, frame_count);

	if (tItems[index].catalog == TX_RX_DELTA) {
		if (core_mp.tx_delta_buf == NULL) {
			core_mp.tx_delta_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
				ipio_err("Failed to allocate tx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_delta_buf == NULL) {
			core_mp.rx_delta_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_delta_buf)) {
				ipio_err("Failed to allocate rx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.tx_max_buf == NULL) {
			core_mp.tx_max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_max_buf)) {
				ipio_err("Failed to allocate tx_max_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.tx_min_buf == NULL) {
			core_mp.tx_min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_min_buf)) {
				ipio_err("Failed to allocate tx_min_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_min_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_max_buf == NULL) {
			core_mp.rx_max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_max_buf)) {
				ipio_err("Failed to allocate rx_max_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_min_buf == NULL) {
			core_mp.rx_min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_min_buf)) {
				ipio_err("Failed to allocate rx_min_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_min_buf);
				return -ENOMEM;
			}
		}
	} else {
		if (tItems[index].buf == NULL) {
			tItems[index].buf = vmalloc(frame_count * core_mp.frame_len * sizeof(s32));
			if (ERR_ALLOC_MEM(tItems[index].buf)) {
				ipio_err("Failed to allocate buf mem\n");
				ipio_kfree((void **)&tItems[index].buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].result_buf == NULL) {
			tItems[index].result_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].result_buf)) {
				ipio_err("Failed to allocate result_buf mem\n");
				ipio_kfree((void **)&tItems[index].result_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].max_buf == NULL) {
			tItems[index].max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].max_buf)) {
				ipio_err("Failed to allocate max_buf mem\n");
				ipio_kfree((void **)&tItems[index].max_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].min_buf == NULL) {
			tItems[index].min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].min_buf)) {
				ipio_err("Failed to allocate min_buf mem\n");
				ipio_kfree((void **)&tItems[index].min_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].spec_option == BENCHMARK) {
			if (tItems[index].bench_mark_max == NULL) {
				tItems[index].bench_mark_max = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bench_mark_max)) {
					ipio_err("Failed to allocate bench_mark_max mem\n");
					ipio_kfree((void **)&tItems[index].bench_mark_max);
					return -ENOMEM;
				}
			}
			if (tItems[index].bench_mark_min == NULL) {
				tItems[index].bench_mark_min = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
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

int check_int_level(bool high)
{
	int timer = 500, ret = -1, level;

	/* From FW request, timeout should at least be 5 sec */
	while (timer) {
		level = gpio_get_value(IRQ_GPIO_NUM);

		ipio_info("int GPIO level = %d\n", level);
		if (high) {
			if (level) {
				ipio_info("check int high sucess \n");
				ret = 0;
				break;
			}
		} else {
			if (!level) {
				ipio_info("check int low sucess \n");
				ret = 0;
				break;
			}
		}
		timer--;
		udelay(2);
	}
	if (timer == 0)
		ipio_info("check int %s fail \n", (high == true) ? "High" : "Low");

	return ret;
}

static int pin_test(int index)
{
	int ret = 0;
	u8 cmd[5] = {0};

	ipio_info("PIN test start");
	ipio_info("test_int_pin = 0x%x\n", tItems[index].test_int_pin);
	ipio_info("int_pulse_test = 0x%x\n", tItems[index].int_pulse_test);
	ipio_info("delay_time = 0x%x\n", tItems[index].delay_time);

	if (tItems[index].test_int_pin == ENABLE) {
		/* test HIGH level*/
		cmd[0] = tItems[index].cmd;
		cmd[1] = 0x1;
		ret = idev->write(cmd, 2);
		if (ret < 0) {
			ipio_err("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = check_int_level(true);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		/* test LOW level*/
		cmd[1] = 0x0;
		ret = idev->write(cmd, 2);
		if (ret < 0) {
			ipio_err("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = check_int_level(false);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}
	}

	if (tItems[index].int_pulse_test == ENABLE) {

		ipio_info("MP IRQ Rising Trigger Test\n");
		cmd[1] = 0x2;
		cmd[2] = tItems[index].delay_time;

		ilitek_plat_irq_unregister();
		ret = ilitek_plat_irq_register(IRQF_TRIGGER_RISING);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		atomic_set(&idev->mp_int_check, ENABLE);
		ret = idev->write(cmd, 3);
		if (ret < 0) {
			ipio_err("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = ilitek_tddi_ic_check_int_stat();
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		ipio_info("MP IRQ Falling Trigger Test\n");
		ilitek_plat_irq_unregister();
		ret = ilitek_plat_irq_register(IRQF_TRIGGER_FALLING);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		atomic_set(&idev->mp_int_check, ENABLE);

		ret = idev->write(cmd, 3);
		if (ret < 0) {
			ipio_err("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = ilitek_tddi_ic_check_int_stat();
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}
	}

	tItems[index].item_result = MP_DATA_PASS;

out:
	if (ret < 0)
		tItems[index].item_result = MP_DATA_FAIL;

	ipio_info("Change to defualt IRQ trigger type\n");
	ilitek_plat_irq_unregister();
	ilitek_plat_irq_register(idev->irq_tirgger_type);

	return ret;
}

static int mutual_test(int index)
{
	int i = 0, j = 0, x = 0, y = 0, ret = 0, get_frame_cont = 1;

	ipio_debug("index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
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
	if (ret < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			if (tItems[i].catalog == TX_RX_DELTA) {
				core_mp.tx_max_buf[y * core_mp.xch_len + x] = INT_MIN;
				core_mp.rx_max_buf[y * core_mp.xch_len + x] = INT_MIN;
				core_mp.tx_min_buf[y * core_mp.xch_len + x] = INT_MAX;
				core_mp.rx_min_buf[y * core_mp.xch_len + x] = INT_MAX;
			} else {
				tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
				tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
			}
		}
	}

	if (tItems[index].catalog != PEAK_TO_PEAK_TEST)
		get_frame_cont = tItems[index].frame_count;

	if (tItems[index].spec_option == BENCHMARK) {
		parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
								tItems[index].type_option, tItems[index].desp, core_mp.frame_len);
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
			for (j = 0; j < core_mp.frame_len; j++)
				tItems[index].buf[i * core_mp.frame_len + j] = frame_buf[j];
			break;
		}
		compare_MaxMin_result(index, &tItems[index].buf[i * core_mp.frame_len]);
	}

out:
	return ret;
}

static int open_test_sp(int index)
{
	struct mp_test_P540_open open[tItems[index].frame_count];
	int i = 0, x = 0, y = 0, ret = 0, addr = 0;
	int Charge_AA = 0, Charge_Border = 0, Charge_Notch = 0, full_open_rate = 0;
	char str[512] = {0};

	ipio_debug("index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
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
	if (ret < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (frame1_cbk700 == NULL) {
		frame1_cbk700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk700)) {
			ipio_err("Failed to allocate frame1_cbk700 buffer\n");
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk700, 0x0, core_mp.frame_len);
	}

	if (frame1_cbk250 == NULL) {
		frame1_cbk250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk250)) {
			ipio_err("Failed to allocate frame1_cbk250 buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk250, 0x0, core_mp.frame_len);
	}

	if (frame1_cbk200 == NULL) {
		frame1_cbk200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk200)) {
			ipio_err("Failed to allocate cbk buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			ipio_kfree((void **)&frame1_cbk250);
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk200, 0x0, core_mp.frame_len);
	}

	tItems[index].node_type = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(tItems[index].node_type)) {
		ipio_err("Failed to allocate node_type FRAME buffer\n");
		return -EMP_NOMEM;
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
			tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
							tItems[index].type_option, tItems[index].desp, core_mp.frame_len);
		dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	parser_ini_nodetype(tItems[index].node_type, NODE_TYPE_KEY_NAME, core_mp.frame_len);
	dump_node_type_buffer(tItems[index].node_type, "node type");

	ret = parser_get_int_data(tItems[index].desp, "charge_aa", str, sizeof(str));
	if (ret || ret == 0)
		Charge_AA = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "charge_border", str, sizeof(str));
	if (ret || ret == 0)
		Charge_Border = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "charge_notch", str, sizeof(str));
	if (ret || ret == 0)
		Charge_Notch = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "full open", str, sizeof(str));
	if (ret || ret == 0)
		full_open_rate = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "tvch", str, sizeof(str));
	if (ret || ret == 0)
		open_spec.tvch = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "tvcl", str, sizeof(str));
	if (ret || ret == 0)
		open_spec.tvcl = katoi(str);

	if (ret < 0) {
		ipio_err("Failed to get parameters from ini file\n");
		ret = -EMP_PARSE;
		goto out;
	}

	ipio_debug("open_test_sp: frame_cont %d, AA %d, Border %d, Notch %d, full_open_rate %d\n",
			tItems[index].frame_count, Charge_AA, Charge_Border, Charge_Notch, full_open_rate);

	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].tdf_700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].tdf_250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].tdf_200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].charg_rate = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].full_Open = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
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
			ipio_memcpy(frame1_cbk700, open[i].cbk_700, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			ipio_memcpy(frame1_cbk250, open[i].cbk_250, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			ipio_memcpy(frame1_cbk200, open[i].cbk_200, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
		}

		ilitek_dump_data(open[i].cbk_700, 10, core_mp.frame_len, core_mp.xch_len, "cbk 700");
		ilitek_dump_data(open[i].cbk_250, 10, core_mp.frame_len, core_mp.xch_len, "cbk 250");
		ilitek_dump_data(open[i].cbk_200, 10, core_mp.frame_len, core_mp.xch_len, "cbk 200");

		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				open[i].charg_rate[addr] = open[i].cbk_250[addr] * 100 / open[i].cbk_700[addr];
				open[i].full_Open[addr] = open[i].cbk_700[addr] - open[i].cbk_200[addr];
				addr++;
			}
		}

		ilitek_dump_data(open[i].charg_rate, 10, core_mp.frame_len, core_mp.xch_len, "origin charge rate");
		ilitek_dump_data(open[i].full_Open, 10, core_mp.frame_len, core_mp.xch_len, "origin full open");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				if (full_open_rate_compare(open[i].full_Open, open[i].cbk_700, x, y, tItems[index].node_type[addr], full_open_rate) == false) {
					tItems[index].buf[(i * core_mp.frame_len) + addr] = 0;
					open[i].charg_rate[addr] = 0;
				}
				addr++;
			}
		}

		ilitek_dump_data(&tItems[index].buf[(i * core_mp.frame_len)], 10, core_mp.frame_len, core_mp.xch_len, "after full_open_rate_compare");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				tItems[index].buf[(i * core_mp.frame_len) + addr] = compare_charge(open[i].charg_rate, x, y, tItems[index].node_type, Charge_AA, Charge_Border, Charge_Notch);
				addr++;
			}
		}

		ilitek_dump_data(&tItems[index].buf[(i * core_mp.frame_len)], 10, core_mp.frame_len, core_mp.xch_len, "after compare charge rate");

		compare_MaxMin_result(index, &tItems[index].buf[(i * core_mp.frame_len)]);
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

static int open_test_cap(int index)
{
	struct mp_test_open_c open[tItems[index].frame_count];
	int i = 0, x = 0, y = 0, ret = 0, addr = 0;
	char str[512] = {0};

	if (tItems[index].frame_count <= 0) {
		ipio_err("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	if (create_mp_test_frame_buffer(index, tItems[index].frame_count) < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (cap_dac == NULL) {
		cap_dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(cap_dac)) {
			ipio_err("Failed to allocate cap_dac buffer\n");
			return -EMP_NOMEM;
		}
	} else {
		memset(cap_dac, 0x0, core_mp.frame_len);
	}

	if (cap_raw == NULL) {
		cap_raw = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(cap_raw)) {
			ipio_err("Failed to allocate cap_raw buffer\n");
			ipio_kfree((void **)&cap_dac);
			return -EMP_NOMEM;
		}
	} else {
		memset(cap_raw, 0x0, core_mp.frame_len);
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
				tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
				tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
							tItems[index].type_option, tItems[index].desp, core_mp.frame_len);
		dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	ret = parser_get_int_data(tItems[index].desp, "gain", str, sizeof(str));
	if (ret || ret == 0)
		open_spec.gain = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "tvch", str, sizeof(str));
	if (ret || ret == 0)
		open_spec.tvch = katoi(str);

	ret = parser_get_int_data(tItems[index].desp, "tvcl", str, sizeof(str));
	if (ret || ret == 0)
		open_spec.tvcl = katoi(str);

	if (ret < 0) {
		ipio_err("Failed to get parameters from ini file\n");
		ret = -EMP_PARSE;
		goto out;
	}

	ipio_debug("open_test_c: frame_cont = %d, gain = %d, tvch = %d, tvcl = %d\n",
			tItems[index].frame_count, open_spec.gain, open_spec.tvch, open_spec.tvcl);

	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].cap_dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cap_raw = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].dcl_cap = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
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
			ipio_memcpy(cap_dac, open[i].cap_dac, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			ipio_memcpy(cap_raw, open[i].cap_raw, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
		}

		ilitek_dump_data(open[i].dcl_cap, 10, core_mp.frame_len, core_mp.xch_len, "DCL_Cap");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				tItems[index].buf[(i * core_mp.frame_len) + addr] = open[i].dcl_cap[addr];
				addr++;
			}
		}
		compare_MaxMin_result(index, &tItems[index].buf[i * core_mp.frame_len]);
	}

out:

	for (i = 0; i < tItems[index].frame_count; i++) {
		ipio_kfree((void **)&open[i].cap_dac);
		ipio_kfree((void **)&open[i].cap_raw);
		ipio_kfree((void **)&open[i].dcl_cap);
	}
	return ret;
}

static int key_test(int index)
{
	int i, j = 0, ret = 0;

	ipio_debug("Item = %s, CMD = 0x%x, Frame Count = %d\n",
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

		for (j = 0; j < core_mp.key_len; j++)
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

static int mp_get_timing_info(void)
{
	int slen = 0;
	char str[256] = {0};
	u8 info[64] = {0};
	char *key = "timing_info_raw";

	core_mp.isLongV = 0;

	slen = parser_get_int_data("pv5_4 command", key, str, sizeof(str));
	if (slen < 0)
		return -1;

	if (parser_get_u8_array(str, info, 16, slen) < 0)
		return -1;

	core_mp.isLongV = info[6];

	ipio_info("DDI Mode = %s\n", (core_mp.isLongV ? "Long V" : "Long H"));

	return 0;
}

static int mp_test_data_sort_average(s32 *oringin_data, int index, s32 *avg_result)
{
	int i, j, k, x, y, len = 5, size;
	s32 u32temp;
	int u32up_frame, u32down_frame;
	s32 *u32sum_raw_data;
	s32 *u32data_buff;

	if (tItems[index].frame_count <= 1)
		return 0;


	if (ERR_ALLOC_MEM(oringin_data)) {
		ipio_err("Input wrong address\n");
		return -ENOMEM;
	}

	u32data_buff = kcalloc(core_mp.frame_len * tItems[index].frame_count, sizeof(s32), GFP_KERNEL);
	u32sum_raw_data = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(u32sum_raw_data) || (ERR_ALLOC_MEM(u32data_buff))) {
		ipio_err("Failed to allocate u32sum_raw_data FRAME buffer\n");
		return -ENOMEM;
	}

	size = core_mp.frame_len * tItems[index].frame_count;
	for (i = 0; i < size; i++) {
		u32data_buff[i] = oringin_data[i];
	}

	u32up_frame = tItems[index].frame_count * tItems[index].highest_percentage / 100;
	u32down_frame = tItems[index].frame_count * tItems[index].lowest_percentage / 100;
	ipio_debug("Up=%d, Down=%d -%s\n", u32up_frame, u32down_frame, tItems[index].desp);

	if (ipio_debug_level) {
		pr_cont("\n[Show Original frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			for (j = 0 ; j < tItems[index].frame_count ; j++) {
				if ((i < len) || (i >= (core_mp.frame_len-len)))
					pr_cont("%d,", u32data_buff[j * core_mp.frame_len + i]);
			}
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("\n");
		}
	}

	for (i = 0; i < core_mp.frame_len; i++) {
		for (j = 0; j < tItems[index].frame_count-1; j++) {
			for (k = 0; k < (tItems[index].frame_count-1-j); k++) {
				x = i+k*core_mp.frame_len;
				y = i+(k+1)*core_mp.frame_len;
				if (*(u32data_buff+x) > *(u32data_buff+y)) {
					u32temp = *(u32data_buff+x);
					*(u32data_buff+x) = *(u32data_buff+y);
					*(u32data_buff+y) = u32temp;
				}
			}
		}
	}

	if (ipio_debug_level) {
		pr_cont("\n[After sorting frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++) {
				if ((i < len) || (i >= (core_mp.frame_len - len)))
					pr_cont("%d,", u32data_buff[i + j * core_mp.frame_len]);
			}
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("\n");
		}
	}

	for (i = 0 ; i < core_mp.frame_len ; i++) {
		u32sum_raw_data[i] = 0;
		for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++)
			u32sum_raw_data[i] += u32data_buff[i + j * core_mp.frame_len];

		avg_result[i] = u32sum_raw_data[i] / (tItems[index].frame_count - u32down_frame - u32up_frame);
	}

	if (ipio_debug_level) {
		pr_cont("\n[Average result frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("%d,", avg_result[i]);
		}
		if ((i < len) || (i >= (core_mp.frame_len-len)))
			pr_cont("\n");
	}

	ipio_kfree((void **)&u32data_buff);
	ipio_kfree((void **)&u32sum_raw_data);
	return 0;
}

static void mp_compare_cdc_result(int index, s32 *tmp, s32 *max_ts, s32 *min_ts, int *result)
{
	int i;

	if (ERR_ALLOC_MEM(tmp)) {
		ipio_err("The data of test item is null (%p)\n", tmp);
		*result = MP_DATA_FAIL;
		return;
	}

	if (tItems[index].catalog == SHORT_TEST) {
		for (i = 0; i < core_mp.frame_len; i++) {
			if (tmp[i] < min_ts[i]) {
				*result = MP_DATA_FAIL;
				return;
			}
		}
	} else {
		for (i = 0; i < core_mp.frame_len; i++) {
			if (tmp[i] > max_ts[i] || tmp[i] < min_ts[i]) {
				*result = MP_DATA_FAIL;
				return;
			}
		}
	}
}

static int mp_comp_result_before_retry(int index)
{
	int i, ret = 0, test_result = MP_DATA_PASS;
	s32 *max_threshold = NULL, *min_threshold = NULL;

	if (tItems[index].catalog == PIN_TEST)
		return tItems[index].item_result;

	max_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		ipio_kfree((void **)&max_threshold);
		test_result = MP_DATA_FAIL;
		ret = -EMP_NOMEM;
		goto fail_alloc;
	}

	min_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(min_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		ipio_kfree((void **)&min_threshold);
		test_result = MP_DATA_FAIL;
		ret = -EMP_NOMEM;
		goto fail_alloc;
	}

	/* Show test result as below */
	if (tItems[index].catalog == TX_RX_DELTA) {
		if (ERR_ALLOC_MEM(core_mp.rx_delta_buf) || ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
			ipio_err("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_DATA_FAIL;
			ret = -EMP_NOMEM;
			goto out;
		}

		for (i = 0; i < core_mp.frame_len; i++) {
			max_threshold[i] = core_mp.TxDeltaMax;
			min_threshold[i] = core_mp.TxDeltaMin;
		}
		mp_compare_cdc_result(index, core_mp.tx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp.tx_min_buf, max_threshold, min_threshold, &test_result);

		for (i = 0; i < core_mp.frame_len; i++) {
			max_threshold[i] = core_mp.RxDeltaMax;
			min_threshold[i] = core_mp.RxDeltaMin;
		}

		mp_compare_cdc_result(index, core_mp.rx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp.rx_min_buf, max_threshold, min_threshold, &test_result);
	} else {
		if (ERR_ALLOC_MEM(tItems[index].buf) || ERR_ALLOC_MEM(tItems[index].max_buf) ||
				ERR_ALLOC_MEM(tItems[index].min_buf) || ERR_ALLOC_MEM(tItems[index].result_buf)) {
			ipio_err("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_DATA_FAIL;
			goto out;
		}

		if (tItems[index].spec_option == BENCHMARK) {
			for (i = 0; i < core_mp.frame_len; i++) {
				max_threshold[i] = tItems[index].bench_mark_max[i];
				min_threshold[i] = tItems[index].bench_mark_min[i];
			}
		} else {
			for (i = 0; i < core_mp.frame_len; i++) {
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
	return ret;
}

static void mp_do_retry(int index, int count)
{
	if (count == 0) {
		ipio_info("Finish retry action\n");
		return;
	}

	ipio_info("retry = %d, item = %s\n", count, tItems[index].desp);

	tItems[index].do_test(index);

	if (mp_comp_result_before_retry(index) < 0)
		return mp_do_retry(index, count - 1);
}

static int mp_show_result(bool lcm_on)
{
	int ret = MP_DATA_PASS, size;
	int i, x, y, j, csv_len = 0, pass_item_count = 0, line_count = 0, get_frame_cont = 1;
	s32 *max_threshold = NULL, *min_threshold = NULL;
	char *csv = NULL;
	char csv_name[128] = { 0 };
	char *ret_pass_name = NULL, *ret_fail_name = NULL, *ret_warning_name = NULL;
	struct file *f = NULL;
	mm_segment_t fs;
	loff_t pos;

	csv = vmalloc(CSV_FILE_SIZE);
	if (ERR_ALLOC_MEM(csv)) {
		ipio_err("Failed to allocate CSV mem\n");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	max_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	min_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold) || ERR_ALLOC_MEM(min_threshold)) {
		ipio_err("Failed to allocate threshold FRAME buffer\n");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	mp_print_csv_header(csv, &csv_len, &line_count, CSV_FILE_SIZE);

	size = ARRAY_SIZE(tItems);
	for (i = 0; i < size; i++) {

		get_frame_cont = 1;
		if (tItems[i].run != 1)
			continue;

		if (tItems[i].item_result == MP_DATA_PASS) {
			pr_info("\n[%s],OK \n", tItems[i].desp);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n[%s],OK\n", tItems[i].desp);
		} else {
			pr_info("\n[%s],NG \n", tItems[i].desp);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n[%s],NG\n", tItems[i].desp);
		}

		if (tItems[i].catalog == PIN_TEST) {
			pr_info("Test INT Pin = %d\n", tItems[i].test_int_pin);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Test INT Pin = %d\n", tItems[i].test_int_pin);
			pr_info("Pulse Test = %d\n", tItems[i].int_pulse_test);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Pulse Test = %d\n", tItems[i].int_pulse_test);
			pr_info("Delay Time = %d\n", tItems[i].delay_time);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Delay Time = %d\n", tItems[i].delay_time);
			continue;
		}

		mp_print_csv_cdc_cmd(csv, &csv_len, i, CSV_FILE_SIZE);

		pr_info("Frame count = %d\n", tItems[i].frame_count);
		csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Frame count = %d\n", tItems[i].frame_count);

		if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
			pr_info("lowest percentage = %d\n", tItems[i].lowest_percentage);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "lowest percentage = %d\n", tItems[i].lowest_percentage);

			pr_info("highest percentage = %d\n", tItems[i].highest_percentage);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "highest percentage = %d\n", tItems[i].highest_percentage);
		}

		/* Show result of benchmark max and min */
		if (tItems[i].spec_option == BENCHMARK) {
			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = tItems[i].bench_mark_max[j];
				min_threshold[j] = tItems[i].bench_mark_min[j];
			}
			mp_compare_cdc_show_result(i, tItems[i].bench_mark_max, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Max_Bench", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, tItems[i].bench_mark_min, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Min_Bench", CSV_FILE_SIZE);
		} else {
			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = tItems[i].max;
				min_threshold[j] = tItems[i].min;
			}

			pr_info("Max = %d\n", tItems[i].max);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Max = %d\n", tItems[i].max);

			pr_info("Min = %d\n", tItems[i].min);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Min = %d\n", tItems[i].min);
		}

		if (strncmp(tItems[i].name, "open_integration_sp", strlen(tItems[i].name)) == 0) {
			mp_compare_cdc_show_result(i, frame1_cbk700, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk700", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, frame1_cbk250, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk250", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, frame1_cbk200, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk200", CSV_FILE_SIZE);
		}

		if (strncmp(tItems[i].name, "open test_c", strlen(tItems[i].name)) == 0) {
			mp_compare_cdc_show_result(i, cap_dac, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_DAC", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, cap_raw, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_RAW", CSV_FILE_SIZE);
		}

		if (tItems[i].catalog == TX_RX_DELTA) {
			if (ERR_ALLOC_MEM(core_mp.rx_delta_buf) || ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
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
			for (x = 0; x < core_mp.key_len; x++) {
				DUMP("KEY_%02d ", x);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "KEY_%02d,", x);
			}

			DUMP("\n");
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n");

			for (y = 0; y < core_mp.key_len; y++) {
				DUMP(" %3d   ", tItems[i].buf[y]);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), " %3d, ", tItems[i].buf[y]);
			}

			DUMP("\n");
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n");
		} else if (tItems[i].catalog == TX_RX_DELTA) {
			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = core_mp.TxDeltaMax;
				min_threshold[j] = core_mp.TxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp.tx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Max Hold", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, core_mp.tx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Min Hold", CSV_FILE_SIZE);

			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = core_mp.RxDeltaMax;
				min_threshold[j] = core_mp.RxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp.rx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Max Hold", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, core_mp.rx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Min Hold", CSV_FILE_SIZE);
		} else {
			/* general result */
			if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
				mp_compare_cdc_show_result(i, tItems[i].result_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Mean result", CSV_FILE_SIZE);
			} else {
				mp_compare_cdc_show_result(i, tItems[i].max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Max Hold", CSV_FILE_SIZE);
				mp_compare_cdc_show_result(i, tItems[i].min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Min Hold", CSV_FILE_SIZE);
			}
			if (tItems[i].catalog != PEAK_TO_PEAK_TEST)
				get_frame_cont = tItems[i].frame_count;

			/* result of each frame */
			for (j = 0; j < get_frame_cont; j++) {
				char frame_name[128] = {0};
				snprintf(frame_name, (CSV_FILE_SIZE - csv_len), "Frame %d", (j+1));
				mp_compare_cdc_show_result(i, &tItems[i].buf[(j*core_mp.frame_len)], csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, frame_name, CSV_FILE_SIZE);
			}
		}
	}

	memset(csv_name, 0, 128 * sizeof(char));

	mp_print_csv_tail(csv, &csv_len, CSV_FILE_SIZE);

	size = ARRAY_SIZE(tItems);
	for (i = 0; i < size; i++) {
		if (tItems[i].run) {
			if (tItems[i].item_result < 0) {
				pass_item_count = 0;
				break;
			}
			pass_item_count++;
		}
	}

	/* define csv file name */
	ret_pass_name = NORMAL_CSV_PASS_NAME;
	ret_fail_name = NORMAL_CSV_FAIL_NAME;
	ret_warning_name = NORMAL_CSV_WARNING_NAME;

	if (core_mp.lost_benchmark) {
		core_mp.final_result = MP_DATA_FAIL;
		ret = MP_DATA_FAIL;
		if (lcm_on)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_ON_PATH, get_date_time_str(), ret_warning_name);
		else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_OFF_PATH, get_date_time_str(), ret_warning_name);
	} else if (pass_item_count == 0) {
		core_mp.final_result = MP_DATA_FAIL;
		ret = MP_DATA_FAIL;
		if (lcm_on)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_ON_PATH, get_date_time_str(), ret_fail_name);
		else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_OFF_PATH, get_date_time_str(), ret_fail_name);
	} else {
		core_mp.final_result = MP_DATA_PASS;
		ret = MP_DATA_PASS;
		if (lcm_on)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_ON_PATH, get_date_time_str(), ret_pass_name);
		else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_OFF_PATH, get_date_time_str(), ret_pass_name);
	}

	ipio_info("Open CSV : %s\n", csv_name);

	if (f == NULL)
		f = filp_open(csv_name, O_WRONLY | O_CREAT | O_TRUNC, 644);

	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open CSV file");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	ipio_info("Open CSV succeed, its length = %d\n ", csv_len);

	if (csv_len >= CSV_FILE_SIZE) {
		ipio_err("The length saved to CSV is too long !\n");
		ret = -EMP_INVAL;
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
	ipio_vfree((void **)&csv);
	ipio_kfree((void **)&max_threshold);
	ipio_kfree((void **)&min_threshold);
	return ret;
}

static void ilitek_tddi_mp_init_item(void)
{
	int i = 0;

	memset(&core_mp, 0, sizeof(core_mp));

	core_mp.chip_pid = idev->chip->pid;
	core_mp.fw_ver = idev->chip->fw_ver;
	core_mp.protocol_ver = idev->protocol->ver;
	core_mp.core_ver = idev->chip->core_ver;
	core_mp.cdc_len = idev->protocol->cdc_len;
	core_mp.no_bk_shift = idev->chip->no_bk_shift;
	core_mp.xch_len = idev->xch_num;
	core_mp.ych_len = idev->ych_num;
	core_mp.frame_len = core_mp.xch_len * core_mp.ych_len;
	core_mp.stx_len = 0;
	core_mp.srx_len = 0;
	core_mp.key_len = 0;
	core_mp.st_len = 0;
	core_mp.tdf = 240;
	core_mp.busy_cdc = INT_CHECK;
	core_mp.retry = false;
	core_mp.final_result = MP_DATA_FAIL;
	core_mp.lost_benchmark = false;

	ipio_info("============== TP & Panel info ================\n");
	ipio_info("CHIP = 0x%x\n", core_mp.chip_pid);
	ipio_info("Firmware version = %x\n", core_mp.fw_ver);
	ipio_info("Protocol version = %x\n", core_mp.protocol_ver);
	ipio_info("Core version = %x\n", core_mp.core_ver);
	ipio_info("Read CDC Length = %d\n", core_mp.cdc_len);
	ipio_info("X length = %d, Y length = %d\n", core_mp.xch_len, core_mp.ych_len);
	ipio_info("Frame length = %d\n", core_mp.frame_len);
	ipio_info("Check busy method = %d\n", core_mp.busy_cdc);
	ipio_info("===============================================\n");

	for (i = 0; i < MP_TEST_ITEM; i++) {
		tItems[i].spec_option = 0;
		tItems[i].type_option = 0;
		tItems[i].run = false;
		tItems[i].max = 0;
		tItems[i].max_res = MP_DATA_FAIL;
		tItems[i].item_result = MP_DATA_PASS;
		tItems[i].min = 0;
		tItems[i].min_res = MP_DATA_FAIL;
		tItems[i].frame_count = 0;
		tItems[i].trimmed_mean = 0;
		tItems[i].lowest_percentage = 0;
		tItems[i].highest_percentage = 0;
		tItems[i].v_tdf_1 = 0;
		tItems[i].v_tdf_2 = 0;
		tItems[i].h_tdf_1 = 0;
		tItems[i].h_tdf_2 = 0;
		tItems[i].goldenmode = 0;
		tItems[i].result_buf = NULL;
		tItems[i].buf = NULL;
		tItems[i].max_buf = NULL;
		tItems[i].min_buf = NULL;
		tItems[i].bench_mark_max = NULL;
		tItems[i].bench_mark_min = NULL;
		tItems[i].node_type = NULL;
		tItems[i].delay_time = 0;
		tItems[i].test_int_pin = 0;
		tItems[i].int_pulse_test = 0;

		if (tItems[i].catalog == MUTUAL_TEST) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == TX_RX_DELTA) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == UNTOUCH_P2P) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == PIXEL) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == OPEN_TEST) {
			if (strncmp(tItems[i].name, "open_integration_sp", strlen(tItems[i].name)) == 0)
				tItems[i].do_test = open_test_sp;
			else if (strncmp(tItems[i].name, "open test_c", strlen(tItems[i].name)) == 0)
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
		} else if (tItems[i].catalog == PIN_TEST) {
			tItems[i].do_test = pin_test;
		}

		tItems[i].result = kmalloc(16, GFP_KERNEL);
		snprintf(tItems[i].result, 16, "%s", "FAIL");
	}

	tItems[0].cmd = CMD_MUTUAL_DAC;
	tItems[1].cmd = CMD_MUTUAL_BG;
	tItems[2].cmd = CMD_MUTUAL_SIGNAL;
	tItems[3].cmd = CMD_MUTUAL_NO_BK;
	tItems[4].cmd = CMD_MUTUAL_HAVE_BK;
	tItems[5].cmd = CMD_MUTUAL_BK_DAC;
	tItems[6].cmd = CMD_SELF_DAC;
	tItems[7].cmd = CMD_SELF_BG;
	tItems[8].cmd = CMD_SELF_SIGNAL;
	tItems[9].cmd = CMD_SELF_NO_BK;
	tItems[10].cmd = CMD_SELF_HAVE_BK;
	tItems[11].cmd = CMD_SELF_BK_DAC;
	tItems[12].cmd = CMD_KEY_DAC;
	tItems[13].cmd = CMD_KEY_BG;
	tItems[14].cmd = CMD_KEY_NO_BK;
	tItems[15].cmd = CMD_KEY_HAVE_BK;
	tItems[16].cmd = CMD_KEY_OPEN;
	tItems[17].cmd = CMD_KEY_SHORT;
	tItems[18].cmd = CMD_ST_DAC;
	tItems[19].cmd = CMD_ST_BG;
	tItems[20].cmd = CMD_ST_NO_BK;
	tItems[21].cmd = CMD_ST_HAVE_BK;
	tItems[22].cmd = CMD_ST_OPEN;
	tItems[23].cmd = CMD_TX_SHORT;
	tItems[24].cmd = CMD_RX_SHORT;
	tItems[25].cmd = CMD_RX_OPEN;
	tItems[26].cmd = CMD_CM_DATA;
	tItems[27].cmd = CMD_CS_DATA;
	tItems[28].cmd = CMD_TX_RX_DELTA;
	tItems[29].cmd = CMD_MUTUAL_SIGNAL;
	tItems[30].cmd = CMD_MUTUAL_NO_BK;
	tItems[31].cmd = CMD_MUTUAL_HAVE_BK;
	tItems[32].cmd = CMD_RX_SHORT;
	tItems[33].cmd = CMD_RX_SHORT;
	tItems[34].cmd = CMD_PEAK_TO_PEAK;
	tItems[48].cmd = CMD_PIN_TEST;
}

static int mp_test_run(char *item)
{
	int i, ret = MP_DATA_PASS;
	char str[512] = {0};

	if (item == NULL || strncmp(item, " ", strlen(item)) == 0 || core_mp.frame_len == 0) {
		core_mp.final_result = MP_DATA_FAIL;
		ret = -EMP_INVAL;
		ipio_err("Invaild string (%s) or frame length (%d)\n", item, core_mp.frame_len);
		goto out;
	}

	ipio_debug("Test item = %s\n", item);

	for (i = 0; i < MP_TEST_ITEM; i++) {
		if (strncmp(item, tItems[i].desp, strlen(item)) == 0) {
			if (strlen(item) != strlen(tItems[i].desp))
				continue;

			/* Get parameters from ini */
			parser_get_int_data(item, "enable", str, sizeof(str));
			tItems[i].run = katoi(str);
			parser_get_int_data(item, "spec option", str, sizeof(str));
			tItems[i].spec_option = katoi(str);
			parser_get_int_data(item, "type option", str, sizeof(str));
			tItems[i].type_option = katoi(str);
			parser_get_int_data(item, "frame count", str, sizeof(str));
			tItems[i].frame_count = katoi(str);
			parser_get_int_data(item, "trimmed mean", str, sizeof(str));
			tItems[i].trimmed_mean = katoi(str);
			parser_get_int_data(item, "lowest percentage", str, sizeof(str));
			tItems[i].lowest_percentage = katoi(str);
			parser_get_int_data(item, "highest percentage", str, sizeof(str));
			tItems[i].highest_percentage = katoi(str);
			parser_get_int_data(item, "goldenmode", str, sizeof(str));
			tItems[i].goldenmode = katoi(str);

			if (tItems[i].goldenmode && (tItems[i].spec_option != tItems[i].goldenmode))
				core_mp.lost_benchmark = true;

			/* Get pin test delay time */
			if (tItems[i].catalog == PIN_TEST) {
				parser_get_int_data(item, "test int pin", str, sizeof(str));
				tItems[i].test_int_pin = katoi(str);
				parser_get_int_data(item, "int pulse test", str, sizeof(str));
				tItems[i].int_pulse_test = katoi(str);
				parser_get_int_data(item, "delay time", str, sizeof(str));
				tItems[i].delay_time = katoi(str);
			}

			/* Get TDF value from ini */
			if (tItems[i].catalog == SHORT_TEST) {
				parser_get_int_data(item, "v_tdf_1", str, sizeof(str));
				tItems[i].v_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
				parser_get_int_data(item, "v_tdf_2", str, sizeof(str));
				tItems[i].v_tdf_2 = parser_get_tdf_value(str, tItems[i].catalog);
				parser_get_int_data(item, "h_tdf_1", str, sizeof(str));
				tItems[i].h_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
				parser_get_int_data(item, "h_tdf_2", str, sizeof(str));
				tItems[i].h_tdf_2 = parser_get_tdf_value(str, tItems[i].catalog);
			} else {
				parser_get_int_data(item, "v_tdf", str, sizeof(str));
				tItems[i].v_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
				parser_get_int_data(item, "h_tdf", str, sizeof(str));
				tItems[i].h_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
			}

			/* Get threshold from ini structure in parser */
			if (strncmp(item, "tx/rx delta", strlen(item)) == 0) {
				parser_get_int_data(item, "tx max", str, sizeof(str));
				core_mp.TxDeltaMax = katoi(str);
				parser_get_int_data(item, "tx min", str, sizeof(str));
				core_mp.TxDeltaMin = katoi(str);
				parser_get_int_data(item, "rx max", str, sizeof(str));
				core_mp.RxDeltaMax = katoi(str);
				parser_get_int_data(item, "rx min", str, sizeof(str));
				core_mp.RxDeltaMin = katoi(str);
				ipio_debug("%s: Tx Max = %d, Tx Min = %d, Rx Max = %d,  Rx Min = %d\n",
						tItems[i].desp, core_mp.TxDeltaMax, core_mp.TxDeltaMin,
						core_mp.RxDeltaMax, core_mp.RxDeltaMin);
			} else {
				parser_get_int_data(item, "max", str, sizeof(str));
				tItems[i].max = katoi(str);
				parser_get_int_data(item, "min", str, sizeof(str));
				tItems[i].min = katoi(str);
			}

			ipio_debug("%s: run = %d, max = %d, min = %d, frame_count = %d\n", tItems[i].desp,
					tItems[i].run, tItems[i].max, tItems[i].min, tItems[i].frame_count);

			ipio_debug("v_tdf_1 = %d, v_tdf_2 = %d, h_tdf_1 = %d, h_tdf_2 = %d", tItems[i].v_tdf_1,
					tItems[i].v_tdf_2, tItems[i].h_tdf_1, tItems[i].h_tdf_2);

			if (!tItems[i].run)
				continue;

			ipio_info("Run MP Test Item : %s\n", tItems[i].desp);
			ret = tItems[i].do_test(i);

			/* Check result before do retry (if enabled)  */
			if (mp_comp_result_before_retry(i) < 0) {
				if (core_mp.retry) {
					ipio_info("MP failed, doing retry\n");
					mp_do_retry(i, RETRY_COUNT);
				}
			}
		}
	}

out:
	return ret;
}

static void mp_test_free(void)
{
	int i, size;

	ipio_info("Free all allocated mem for MP\n");

	core_mp.final_result = MP_DATA_FAIL;

	size = ARRAY_SIZE(tItems);
	for (i = 0; i < size; i++) {
		tItems[i].run = false;
		tItems[i].max_res = MP_DATA_FAIL;
		tItems[i].min_res = MP_DATA_FAIL;
		tItems[i].item_result = MP_DATA_PASS;
		snprintf(tItems[i].result, 16, "%s", "FAIL");

		if (tItems[i].catalog == TX_RX_DELTA) {
				ipio_kfree((void **)&core_mp.rx_delta_buf);
				ipio_kfree((void **)&core_mp.tx_delta_buf);
				ipio_kfree((void **)&core_mp.tx_max_buf);
				ipio_kfree((void **)&core_mp.tx_min_buf);
				ipio_kfree((void **)&core_mp.rx_max_buf);
				ipio_kfree((void **)&core_mp.rx_min_buf);
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
}

/* The method to copy results to user depends on what APK needs */
static void mp_copy_ret_to_apk(char *buf)
{
	int i, run = 0;

	if (!buf) {
		ipio_err("apk buffer is null\n");
		return;
	}

	for (i = 0; i < MP_TEST_ITEM; i++) {
		buf[i] = 2;
		if (tItems[i].run) {
			if (tItems[i].item_result == MP_DATA_FAIL) {
				ipio_err("Item [%s] data fail\n", tItems[i].desp);
				buf[i] = 1;
			} else {
				ipio_info("Item [%s] data pass\n", tItems[i].desp);
				buf[i] = 0;
			}
			run++;
		}
	}
}

int ilitek_tddi_mp_test_main(char *apk, bool lcm_on, char *single)
{
	int i, ret = 0;
	char str[128] = {0}, ver[128] = {0};

	ilitek_ini_file_data = (struct ini_file_data *)vmalloc(sizeof(struct ini_file_data) * PARSER_MAX_KEY_NUM);
	if (ERR_ALLOC_MEM(ilitek_ini_file_data)) {
		ipio_info("Failed to malloc ilitek_ini_file_data\n");
		ret = -EMP_NOMEM;
		goto out;
	}

	ilitek_tddi_mp_init_item();

	ret = ilitek_tddi_mp_ini_parser(INI_NAME_PATH);
	if (ret < 0) {
		ipio_err("Failed to parsing INI file\n");
		ret = -EMP_INI;
		goto out;
	}

	/* Compare both protocol version of ini and firmware */
	parser_get_ini_key_value("pv5_4 command", "protocol", str);
	snprintf(ver, sizeof(ver), "0x%s", str);
	if ((str2hex(ver)) != (core_mp.protocol_ver >> 8)) {
		ipio_err("ERROR! MP Protocol version is invaild, 0x%x\n", str2hex(ver));
		ret = -EMP_PROTOCOL;
		goto out;
	}

	/* Read timing info from ini file */
	if (mp_get_timing_info() < 0) {
		ipio_err("Failed to get timing info from ini\n");
		ret = -EMP_TIMING_INFO;
		goto out;
	}

	if (single != NULL) {
		ret = mp_test_run(single);
		if (ret < 0) {
			mp_show_result(lcm_on);
			goto mp_failed;
		}
	}

	/* Do not chang the sequence of test */
	if (core_mp.protocol_ver >= PROTOCOL_VER_540) {
		if (lcm_on) {
			for (i = 0; i < DEF_TEST_LCM_ON; i++) {
				ret = mp_test_run(run_lcm_on[i]);
				if (ret < 0) {
					mp_show_result(lcm_on);
					goto mp_failed;
				}
			}
		} else {
			for (i = 0; i < DEF_TEST_LCM_OFF; i++) {
				ret = mp_test_run(run_lcm_off[i]);
				if (ret < 0) {
					mp_show_result(lcm_on);
					goto mp_failed;
				}
			}
		}
	} else {
		for (i = 0; i < DEF_OLD_TEST; i++) {
			ret = mp_test_run(run_old_test[i]);
			if (ret < 0) {
				mp_show_result(true);
				goto mp_failed;
			}
		}
	}

	ret = mp_show_result(lcm_on);

mp_failed:
	mp_copy_ret_to_apk(apk);
	mp_test_free();

out:
	ipio_vfree((void **)&ilitek_ini_file_data);
	return ret;
};
