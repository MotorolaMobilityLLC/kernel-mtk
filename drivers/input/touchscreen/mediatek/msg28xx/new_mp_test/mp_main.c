/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
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
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Dicky Chiang
 * Maintain: Luca Hsu, Tigers Huang
 */

#include "../ilitek_drv_common.h"
#include "mp_common.h"

extern struct mutex g_Mutex;
extern u32 g_IsInMpTest;
MutualMpTest_t *mp_test_data;
MutualMpTestResult_t *mp_test_result;
struct mp_main_func *p_mp_func;

int ana_count = 0;
u8 ana_ver[100] = {0};
int mapping_barker_ini = 0;

int mp_test_data_avg(int *data, int len)
{
	int i = 0, data_sum = 0;
	for(i = 0; i < len; i++)
		data_sum += data[i];
	return data_sum/len;
}

static void mp_calc_golden_range_new(int *goldenTbl,int weight_up,int weight_low, int *maxTbl, int *minTbl,int length)
{
	int i = 0;
	for(i = 0; i < length; i++)
	{
		maxTbl[i] = (goldenTbl[i] * (10000 + weight_up)) / 10000;
		minTbl[i] = (goldenTbl[i] * (10000 + weight_low)) / 10000;
	}
}
extern void DebugShowArray2(void *pBuf, u16 nLen, int nDataType, int nCarry, int nChangeLine);
static void mp_calc_golden_range(int *goldenTbl, u16 weight, u16 golden_up, int *maxTbl, int *minTbl, int length)
{
    int i, j = 0,value = 0, value_up = 0, shift_value = 0, weight_up = golden_up, weight_low = 0, weight_notch_up = 0, weight_notch_low = 0;
	int allnode_number = mp_test_data->sensorInfo.numDrv * mp_test_data->sensorInfo.numSen;
	int golden_Cbg = 0;
	int notch_node[] = { 0, 6, 7, 8, 9, 15, 23, 24, 512, 527 };
	bool notch_flag = false;
	golden_Cbg = mp_test_data_avg(goldenTbl, allnode_number);
    mp_info("*** --------------golden_Cbg=%d, allnode_number=%d---------------- ***\n",golden_Cbg,allnode_number);
    DebugShowArray2(goldenTbl, allnode_number, -32, 10, 16);	
	if(golden_Cbg == 0)
	{
		shift_value = 0;
	}
	else
	{
		shift_value = (int)((((mp_test_data->Open_Swt * 10000) / golden_Cbg - 10000) * ( mp_test_data->Open_Cbg2deltac_ratio))/10);
	}
	if(mp_test_data->Open_Swt > golden_Cbg)
	{
		mp_info("*** --------------shift_value = %d---------------- ***\n", shift_value);
		weight_up = shift_value + mp_test_data->Open_Csig_Vari_Up * 100;
		weight_low = shift_value + mp_test_data->Open_Csig_Vari_Low * 100;
		weight_notch_up = shift_value + mp_test_data->Open_Csig_Vari_Notch_Up * 100;
		weight_notch_low = shift_value + mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	}
	else if(mp_test_data->Open_Swt < golden_Cbg)
	{
		mp_info("*** --------------shift_value = %d---------------- ***\n", shift_value);
		weight_up = mp_test_data->Open_Csig_Vari_Up * 100;
		weight_low = shift_value + mp_test_data->Open_Csig_Vari_Low * 100;
		weight_notch_up = mp_test_data->Open_Csig_Vari_Notch_Up * 100;
		weight_notch_low = shift_value + mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	}
	else
	{
		mp_info("*** --------------shift_value = %d---------------- ***\n", shift_value);
		weight_up = mp_test_data->Open_Csig_Vari_Up * 100;
		weight_low = mp_test_data->Open_Csig_Vari_Low * 100;
		weight_notch_up = mp_test_data->Open_Csig_Vari_Notch_Up * 100;
		weight_notch_low = mp_test_data->Open_Csig_Vari_Notch_Low * 100;
	}

	mp_calc_golden_range_new(goldenTbl, weight_up, weight_low, maxTbl, minTbl, length);
 	mp_info("*** golden_Cbg=%d ***\n", golden_Cbg);
	 mp_info("*** weight_up=%d ***\n", weight_up);
	 mp_info("*** weight_low=%d ***\n", weight_low);
	 mp_info("*** weight_notch_up=%d ***\n", weight_notch_up);
	 mp_info("*** weight_notch_low=%d ***\n", weight_notch_low);
    for (i = 0; i < length; i++) {
		for(j = 0; j < sizeof(notch_node); j++)
		{
			if(i == notch_node[j])
			{
				notch_flag = true;
				break;
			}
		}
		if(notch_flag)
		{
			maxTbl[i] = abs(goldenTbl[i]) * (10000 + weight_notch_up) / 10000;
			minTbl[i] = abs(goldenTbl[i]) * (10000 - weight_notch_low) / 10000;
		}
		else
		{
			value = (int)weight * abs(goldenTbl[i]) / 100;
			value_up = (int)weight_up * abs(goldenTbl[i]) / 10000;

			maxTbl[i] = goldenTbl[i] + value + value_up;
			minTbl[i] = goldenTbl[i] - value;
		}
		notch_flag = false;
    }
    DebugShowArray2(maxTbl, allnode_number, -32, 10, 16);
    DebugShowArray2(minTbl, allnode_number, -32, 10, 16);
}

static int mp_load_ini(char * pFilePath)
{
	int res = 0, nSize = 0;
	char *token = NULL, str[512]={0};
	long s_to_long = 0;

	mp_info("*** %s() ***\n", __func__);

	if(mp_parse(pFilePath) < 0) {
         mp_err("Failed to parse file = %s\n", pFilePath);
         return -1;
	}

    mp_info("Parsed %s successfully!\n", pFilePath);

    mp_test_data = kcalloc(1, sizeof(*mp_test_data), GFP_KERNEL);
    mp_test_result = kcalloc(1, sizeof(*mp_test_result), GFP_KERNEL);
	if(ERR_ALLOC_MEM(mp_test_result) || ERR_ALLOC_MEM(mp_test_data)) {
		pr_err("Failed to allocate mp_test mem \n");
		return -1;
	}

	token = kmalloc(100, GFP_KERNEL);
	ms_get_ini_data("INFOMATION", "MAPPING_BARKER_INI",str);
	mapping_barker_ini = ms_atoi(str);
	mp_info(" mapping_barker_ini = %d \n", mapping_barker_ini);

	ms_get_ini_data("UI_CONFIG","OpenTest",str);
	mp_test_data->UIConfig.bOpenTest = ms_atoi(str);
	ms_get_ini_data("UI_CONFIG","ShortTest",str);
	mp_test_data->UIConfig.bShortTest = ms_atoi(str);
	ms_get_ini_data("UI_CONFIG","UniformityTest",str);
    mp_test_data->UIConfig.bUniformityTest =  ms_atoi(str);
	ms_get_ini_data("UI_CONFIG","WpTest",str);
	mp_test_data->UIConfig.bWpTest = ms_atoi(str);

    mp_test_data->ana_version = kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);
	if(ERR_ALLOC_MEM(mp_test_data->ana_version)) {
		pr_err("Failed to allocate Ana mem \n");
		return -1;
	}

	ms_get_ini_data("UI_CONFIG","ANAGEN_VER", str);
	strcpy(mp_test_data->ana_version, str);
	ana_count = ms_ini_split_u8_array(mp_test_data->ana_version, ana_ver);
	mp_info("Ana count = %d , mem = %p\n", ana_count, mp_test_data->ana_version);

	ms_get_ini_data("SENSOR","DrvNum",str);
	mp_test_data->sensorInfo.numDrv = ms_atoi(str);
	ms_get_ini_data("SENSOR","SenNum",str);
	mp_test_data->sensorInfo.numSen = ms_atoi(str);
	ms_get_ini_data("SENSOR","KeyNum",str);
	mp_test_data->sensorInfo.numKey = ms_atoi(str);
	ms_get_ini_data("SENSOR","KeyLine",str);
	mp_test_data->sensorInfo.numKeyLine = ms_atoi(str);
	ms_get_ini_data("SENSOR","GrNum",str);
	mp_test_data->sensorInfo.numGr = ms_atoi(str);

	ms_get_ini_data("OPEN_TEST_N","CSUB_REF",str);
	mp_test_data->Open_test_csub = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","CFB_REF",str);
	mp_test_data->Open_test_cfb = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","OPEN_MODE",str);
	mp_test_data->Open_mode = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","FIXED_CARRIER",str);
	mp_test_data->Open_fixed_carrier = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","FIXED_CARRIER1",str);
	mp_test_data->Open_fixed_carrier1 = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","CHARGE_PUMP",str);
	mp_test_data->Open_test_chargepump = ms_atoi(str);

	ms_get_ini_data("OPEN_TEST_N","CBG_SWT",str);
	mp_test_data->Open_Swt = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","CBG2DELTAC_RATIO",str);
	mp_test_data->Open_Cbg2deltac_ratio = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_UP",str);
	mp_test_data->Open_Csig_Vari_Up = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_LOW",str);
	mp_test_data->Open_Csig_Vari_Low = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_NOTCH_UP",str);
	mp_test_data->Open_Csig_Vari_Notch_Up = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N","Csig_SUPPORT_VARI_NOTCH_LOW",str);
	mp_test_data->Open_Csig_Vari_Notch_Low = ms_atoi(str);

	ms_get_ini_data("INFOMATION","MutualKey",str);
	mp_test_data->Mutual_Key = ms_atoi(str);
	ms_get_ini_data("INFOMATION","Pattern_type",str);
	mp_test_data->Pattern_type = ms_atoi(str);
	ms_get_ini_data("INFOMATION","1T2R_MODEL",str);
	mp_test_data->Pattern_model = ms_atoi(str);

	ms_get_ini_data("RULES","DC_Range",str);
	mp_test_data->ToastInfo.persentDC_VA_Range = ms_atoi(str);
	ms_get_ini_data("RULES","DC_Range_UP",str);
	mp_test_data->ToastInfo.persentDC_VA_Range_up = ms_atoi(str);
	ms_get_ini_data("RULES","DC_Ratio",str);
	mp_test_data->ToastInfo.persentDC_VA_Ratio = ms_atoi(str);
	ms_get_ini_data("RULES","DC_Ratio_UP",str);
	mp_test_data->ToastInfo.persentDC_VA_Ratio_up = ms_atoi(str);
	ms_get_ini_data("RULES","DC_Border_Ratio",str);
	mp_test_data->ToastInfo.persentDC_Border_Ratio = ms_atoi(str);
	ms_get_ini_data("RULES","opentestmode",str);
	mp_test_data->Open_test_mode = ms_atoi(str);
	ms_get_ini_data("RULES","shorttestmode",str);
	mp_test_data->Short_test_mode = ms_atoi(str);

	ms_get_ini_data("BASIC","DEEP_STANDBY",str);
	mp_test_data->deep_standby = ms_atoi(str);

	ms_get_ini_data("BASIC","DEEP_STANDBY_TIMEOUT",str);
	mp_test_data->deep_standby_timeout = ms_atoi(str);

    if ((mp_test_data->Mutual_Key == 1) && (mp_test_data->Mutual_Key == 2)) {
		ms_get_ini_data("SENSOR","KEY_CH",str);
		mp_test_data->sensorInfo.KEY_CH = ms_atoi(str);
	}

	ms_get_ini_data("OPEN_TEST_N", "KEY_SETTING_BY_FW", str);
	mp_test_data->Open_KeySettingByFW = ms_atoi(str);
	ms_get_ini_data("OPEN_TEST_N", "INVERT_MODE", str);
	mp_test_data->inverter_mode = ms_atoi(str);

	ms_get_ini_data("CDTIME", "OPEN_CHARGE", str);
	mp_test_data->OPEN_Charge = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->OPEN_Charge = s_to_long;

	ms_get_ini_data("CDTIME", "OPEN_DUMP", str);
	mp_test_data->OPEN_Dump = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->OPEN_Dump = s_to_long;

	ms_get_ini_data("CDTIME", "SHORT_POSTIDLE", str);
	mp_test_data->SHORT_Postidle = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Postidle = s_to_long;

	ms_get_ini_data("CDTIME", "SHORT_Charge", str);
	mp_test_data->SHORT_Charge = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Charge = s_to_long;

	ms_get_ini_data("CDTIME", "SHORT_Dump1", str);
	mp_test_data->SHORT_Dump1 = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Dump1 = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_Dump2", str);
	mp_test_data->SHORT_Dump2 = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Dump2 = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_AVG_OK_COUNTS", str);
	mp_test_data->SHORT_AVG_count = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_AVG_count = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_CAPTURE_COUNTS", str);
	mp_test_data->SHORT_Capture_count = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Capture_count = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_FOUT_MAX_1", str);
	mp_test_data->SHORT_Fout_max_1 = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Fout_max_1 = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_FOUT_MAX_2", str);
	mp_test_data->SHORT_Fout_max_2 = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Fout_max_2 = s_to_long;
	ms_get_ini_data("CDTIME", "SHORT_HOPPING_EN", str);
	mp_test_data->SHORT_Hopping = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_Hopping = s_to_long;

	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_TOP_Ratio", str);
	mp_test_data->uniformity_ratio.bd_top= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_BOTTOM_Ratio", str);
	mp_test_data->uniformity_ratio.bd_bottom= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_UPPER_Ratio", str);
	mp_test_data->uniformity_ratio.bd_l_top= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_UPPER_Ratio", str);
	mp_test_data->uniformity_ratio.bd_r_top= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_L_LOWER_Ratio", str);
	mp_test_data->uniformity_ratio.bd_l_bottom= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_R_LOWER_Ratio", str);
	mp_test_data->uniformity_ratio.bd_r_bottom= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "BD_Notch_Ratio", str);
	mp_test_data->uniformity_ratio.notch= ms_atoi(str);
    	ms_get_ini_data("UNIFORMITY_TEST_N", "VA_UPPER_Ratio", str);
	mp_test_data->uniformity_ratio.va_top= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "VA_LOWER_Ratio", str);
	mp_test_data->uniformity_ratio.va_bottom= ms_atoi(str);


  	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Top_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_top= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Top_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_top= ms_atoi(str);
	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Bottom_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_bottom= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Bottom_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_bottom= ms_atoi(str);
	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_UPPER_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_l_top= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_UPPER_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_l_top= ms_atoi(str);
  	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_LOWER_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_l_bottom= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Left_LOWER_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_l_bottom= ms_atoi(str);
  	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_UPPER_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_r_top= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_UPPER_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_r_top= ms_atoi(str);
  	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_LOWER_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.bd_r_bottom= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Right_LOWER_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.bd_r_bottom= ms_atoi(str);
  	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Notch_Ratio_Max", str);
	mp_test_data->bd_va_ratio_max.notch= ms_atoi(str);
   	ms_get_ini_data("UNIFORMITY_TEST_N", "BDVA_Notch_Ratio_Min", str);
	mp_test_data->bd_va_ratio_min.notch= ms_atoi(str);

   	ms_get_ini_data("SHORT_TEST_N", "SHORT_SENSOR_PIN_NUM", str);
	mp_test_data->SHORT_sensor_pin_number= ms_atoi(str);
	ms_get_ini_data("SHORT_TEST_N", "SHORT_SAMPLE_NUM", str);
	mp_test_data->OPEN_Dump = ms_atoi(str);
	strcpy(token, str);
	res = kstrtol((const char *)token, 0, &s_to_long);
	if(res == 0)
	 	mp_test_data->SHORT_sample_number = s_to_long;
    mp_info("ANAGEN_VER:    [%s]\n", mp_test_data->ana_version);
    mp_info("OpenTest:      [%d]\n", mp_test_data->UIConfig.bOpenTest);
    mp_info("ShortTest:      [%d]\n", mp_test_data->UIConfig.bShortTest);
    mp_info("WPTest:      [%d]\n", mp_test_data->UIConfig.bWpTest);
    mp_info("DrvNum:      [%d]\n", mp_test_data->sensorInfo.numDrv);
    mp_info("SenNum:      [%d]\n", mp_test_data->sensorInfo.numSen);
    mp_info("KeyNum:      [%d]\n", mp_test_data->sensorInfo.numKey);
    mp_info("KeyLine:      [%d]\n", mp_test_data->sensorInfo.numKeyLine);
    mp_info("DEEP_STANDBY = [%d] \n", mp_test_data->deep_standby);
    mp_info("GrNum:      [%d]\n", mp_test_data->sensorInfo.numGr);
    mp_info("CSUB_REF:      [%d]\n", mp_test_data->Open_test_csub);
    mp_info("CFB_REF:      [%d]\n", mp_test_data->Open_test_cfb);
    mp_info("OPEN_MODE:      [%d]\n", mp_test_data->Open_mode);
    mp_info("CBG_SWT:      [%d]\n", mp_test_data->Open_Swt);
    mp_info("CBG2DELTAC_RATIO = [%d] \n", mp_test_data->Open_Cbg2deltac_ratio);
    mp_info("Csig_SUPPORT_VARI_UP:      [%d]\n", mp_test_data->Open_Csig_Vari_Up);
    mp_info("Csig_SUPPORT_VARI_LOW:      [%d]\n", mp_test_data->Open_Csig_Vari_Low);
    mp_info("Csig_SUPPORT_VARI_NOTCH_UP:      [%d]\n", mp_test_data->Open_Csig_Vari_Notch_Up);
    mp_info("Csig_SUPPORT_VARI_NOTCH_LOW:      [%d]\n", mp_test_data->Open_Csig_Vari_Notch_Low);
    mp_info("FIXED_CARRIER:      [%d]\n", mp_test_data->Open_fixed_carrier);
    mp_info("FIXED_CARRIER1:      [%d]\n", mp_test_data->Open_fixed_carrier1);
    mp_info("CHARGE_PUMP:      [%d]\n", mp_test_data->Open_test_chargepump);
    mp_info("MutualKey:      [%d]\n", mp_test_data->Mutual_Key);
    mp_info("KEY_CH:      [%d]\n", mp_test_data->sensorInfo.KEY_CH);
    mp_info("Pattern_type:      [%d]\n", mp_test_data->Pattern_type);
    mp_info("Pattern_model:      [%d]\n", mp_test_data->Pattern_model);
    mp_info("DC_Range:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Range);
    mp_info("DC_Ratio:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Ratio);
    mp_info("DC_Range_up:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Range_up);
    mp_info("DC_Ratio_up:      [%d]\n", mp_test_data->ToastInfo.persentDC_VA_Ratio_up);
    mp_info("KEY_SETTING_BY_FW:      [%d]\n", mp_test_data->Open_KeySettingByFW);
    mp_info("INVERT_MODE:      [%d]\n", mp_test_data->inverter_mode);
    mp_info("DEEP_STANDBY_TIMEOUT:      [%d]\n", mp_test_data->deep_standby_timeout);
    mp_info("OPEN_CHARGE:      [%d]\n", mp_test_data->OPEN_Charge);
    mp_info("OPEN_DUMP:      [%d]\n", mp_test_data->OPEN_Dump);

    mp_info("SHORT_SENSOR_PIN_NUM:      [%d]\n", mp_test_data->SHORT_sensor_pin_number);
    mp_info("SHORT_SAMPLE_NUM:      [%d]\n", mp_test_data->SHORT_sample_number);
    mp_info("SHORT_POSTIDLE:      [%d]\n", mp_test_data->SHORT_Postidle);
    mp_info("SHORT_Charge:      [%d]\n", mp_test_data->SHORT_Charge);
    mp_info("SHORT_Dump1:      [%d]\n", mp_test_data->SHORT_Dump1);
    mp_info("SHORT_FOUT_MAX_1:      [%d]\n", mp_test_data->SHORT_Fout_max_1);
    mp_info("SHORT_Dump2:      [%d]\n", mp_test_data->SHORT_Dump2);
    mp_info("SHORT_FOUT_MAX_2:      [%d]\n", mp_test_data->SHORT_Fout_max_2);
    mp_info("SHORT_HOPPING_EN:      [%d]\n", mp_test_data->SHORT_Hopping);
    mp_info("SHORT_CAPTURE_COUNTS:      [%d]\n", mp_test_data->SHORT_Capture_count);
    mp_info("SHORT_AVG_OK_COUNTS:      [%d]\n", mp_test_data->SHORT_AVG_count);

    mp_info("[UNIFORMITY_TEST_N]\n");
    mp_info("BD_TOP_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_top);
    mp_info("BD_BOTTOM_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_bottom);
    mp_info("BD_L_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_top);
    mp_info("BD_R_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_top);
    mp_info("BD_L_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_l_bottom);
    mp_info("BD_R_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.bd_r_bottom);
    mp_info("BD_Notch_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.notch);
    mp_info("VA_UPPER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_top);
    mp_info("VA_LOWER_Ratio:      [%d]\n", mp_test_data->uniformity_ratio.va_bottom);

    mp_info("BDVA_Top_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_top);
    mp_info("BDVA_Top_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_top);
    mp_info("BDVA_Bottom_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_bottom);
    mp_info("BDVA_Bottom_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_bottom);
    mp_info("BDVA_Left_UPPER_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_l_top);
    mp_info("BDVA_Left_UPPER_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_l_top);
    mp_info("BDVA_Left_LOWER_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_l_bottom);
    mp_info("BDVA_Left_LOWER_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_l_bottom);
    mp_info("BDVA_Right_UPPER_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_r_top);
    mp_info("BDVA_Right_UPPER_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_r_top);
    mp_info("BDVA_Right_LOWER_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.bd_r_bottom);
    mp_info("BDVA_Right_LOWER_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.bd_r_bottom);
    mp_info("BDVA_Notch_Ratio_Max:      [%d]\n", mp_test_data->bd_va_ratio_max.notch);
    mp_info("BDVA_Notch_Ratio_Min:      [%d]\n", mp_test_data->bd_va_ratio_min.notch);



    if(mp_test_data->sensorInfo.numKey > 0) {
		ms_get_ini_data("SENSOR","KeyDrv_o",str);
		mp_test_data->sensorInfo.KeyDrv_o = ms_atoi(str);

		ms_get_ini_data("SENSOR","KEYSEN",str);
		mp_test_data->KeySen = kcalloc(mp_test_data->sensorInfo.numKey, sizeof(int), GFP_KERNEL);
        if(ERR_ALLOC_MEM(mp_test_data->KeySen)) {
			mp_err("Failed to allocate mp_test_data->KeySen mem\n");
			return -1;
		}

		ms_ini_split_int_array(str, mp_test_data->KeySen);		

		ms_get_ini_data("SENSOR","KEY_TYPE",str);
		mp_test_data->sensorInfo.key_type = kmalloc(64 * sizeof(char), GFP_KERNEL);
		if(ERR_ALLOC_MEM(mp_test_data->sensorInfo.key_type)) {
			mp_err("Failed to allocate mp_test_data->sensorInfo.key_type mem\n");
			return -1;
		}

		strcpy(mp_test_data->sensorInfo.key_type, str);
    }

	mp_test_data->UIConfig.sSupportIC = kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);

	if(ERR_ALLOC_MEM(mp_test_data->UIConfig.sSupportIC)) {
		mp_err("Failed to allocate mp_test_data->UIConfig.sSupportIC mem\n");
		return -1;
	}

	memset(mp_test_data->UIConfig.sSupportIC, 0, FILENAME_MAX * sizeof(char));
	if(ms_get_ini_data("UI_CONFIG","SupportIC",str) != 0)
		strcpy(mp_test_data->UIConfig.sSupportIC, str);
	
	mp_info("SupportIC:      [%s]\n", mp_test_data->UIConfig.sSupportIC);

 	mp_test_data->project_name = (char *)kmalloc(FILENAME_MAX * sizeof(char), GFP_KERNEL);
	if(ERR_ALLOC_MEM(mp_test_data->UIConfig.sSupportIC))
	{
		mp_err("Failed to allocate mp_test_data->project_name mem\n");
		return -1;
	}

	memset(mp_test_data->project_name, 0, FILENAME_MAX * sizeof(char));
	if(ms_get_ini_data("INFOMATION", "PROJECT", str) != 0)
		strcpy(mp_test_data->project_name, str);

	mp_info("PROJECT:      [%s]\n", mp_test_data->project_name);

    mp_test_data->Goldensample_CH_0 = (int *)kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	if(ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0)) {
		mp_err("Failed to allocate mp_test_data->Goldensample_CH_0 mem\n");
		return -1;
	}

	nSize = ms_ini_split_golden(mp_test_data->Goldensample_CH_0, mp_test_data->sensorInfo.numSen);
	mp_info("The number of Golden line = %d\n",nSize);

	mp_test_data->Goldensample_CH_0_Max = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL); 
	mp_test_data->Goldensample_CH_0_Max_Avg = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_data->Goldensample_CH_0_Min = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_data->Goldensample_CH_0_Min_Avg = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	if(ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Max) || ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Max_Avg) ||
			 ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Min)|| ERR_ALLOC_MEM(mp_test_data->Goldensample_CH_0_Min_Avg))
	{
		mp_err("Failed to allocate Goldensample mem\n");
		return -1;
	}

	if (mp_test_data->sensorInfo.numDrv && mp_test_data->sensorInfo.numSen) {

		mp_test_data->PAD2Drive = kmalloc(mp_test_data->sensorInfo.numDrv * sizeof(u16), GFP_KERNEL);
		if(ERR_ALLOC_MEM(mp_test_data->PAD2Drive)) {
			mp_err("Failed to allocate PAD2Drive mem\n");
			return -1;
		}

		ms_get_ini_data("PAD_TABLE","DRIVE",str);
		mp_info("PAD_TABLE(DRIVE):      [%s]\n", str);
		p_mp_func->drive_len = ms_ini_split_u16_array(str, mp_test_data->PAD2Drive);

		mp_test_data->PAD2Sense = kmalloc(mp_test_data->sensorInfo.numSen * sizeof(u16), GFP_KERNEL);
		if(ERR_ALLOC_MEM(mp_test_data->PAD2Sense)) {
			mp_err("Failed to allocate PAD2Sense mem\n");
			return -1;
		}

		ms_get_ini_data("PAD_TABLE","SENSE",str);
		mp_info("PAD_TABLE(SENSE):      [%s]\n", str);
		p_mp_func->sense_len = ms_ini_split_u16_array(str, mp_test_data->PAD2Sense);
	}

	if (mp_test_data->sensorInfo.numGr) {
		mp_test_data->PAD2GR = kmalloc(mp_test_data->sensorInfo.numGr * sizeof(u16), GFP_KERNEL);
		if(ERR_ALLOC_MEM(mp_test_data->PAD2GR)) {
			mp_err("Failed to allocate PAD2GR mem\n");
			return -1;
		}

		ms_get_ini_data("PAD_TABLE","GR",str);
		printk("PAD_TABLE(GR):      [%s]\n", str);
		p_mp_func->gr_len = ms_ini_split_u16_array(str, mp_test_data->PAD2GR);
	}

	ms_get_ini_data("RULES","ICPINSHORT",str);
	mp_test_data->sensorInfo.thrsICpin = ms_atoi(str);
	mp_info("ICPINSHORT:      [%d]\n", mp_test_data->sensorInfo.thrsICpin);

	mp_kfree((void **)&token);
	mp_info("MEM free token\n");
	return 0;
}

static int mp_main_init_var(void)
{
	mp_calc_golden_range(mp_test_data->Goldensample_CH_0,
		mp_test_data->ToastInfo.persentDC_VA_Range, mp_test_data->ToastInfo.persentDC_VA_Range_up,
		mp_test_data->Goldensample_CH_0_Max, mp_test_data->Goldensample_CH_0_Min, MAX_MUTUAL_NUM);

	mp_test_result->nRatioAvg_max = (int) (100 + mp_test_data->ToastInfo.persentDC_VA_Ratio + mp_test_data->ToastInfo.persentDC_VA_Ratio_up) / 100;
	mp_test_result->nRatioAvg_min =(int) (100 - mp_test_data->ToastInfo.persentDC_VA_Ratio) / 100;
	mp_test_result->nBorder_RatioAvg_max = (int) (100 + mp_test_data->ToastInfo.persentDC_Border_Ratio + mp_test_data->ToastInfo.persentDC_VA_Ratio_up) / 100;
	mp_test_result->nBorder_RatioAvg_min = (int) (100 - mp_test_data->ToastInfo.persentDC_Border_Ratio) / 100;

	mp_test_result->pCheck_Fail =               kcalloc(TEST_ITEM_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pOpenResultData =           kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pOpenFailChannel =          kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pOpenRatioFailChannel =     kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

	mp_test_result->pGolden_CH =                kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pGolden_CH_Max =            kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pGolden_CH_Max_Avg =        kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pGolden_CH_Min =            kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pGolden_CH_Min_Avg =        kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

	mp_test_result->pShortFailChannel =         kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
	mp_test_result->pShortResultData =          kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
	mp_test_result->pICPinChannel =             kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
	mp_test_result->pICPinShortFailChannel =    kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
	mp_test_result->pICPinShortResultData =     kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);

	mp_test_result->pICPinShortRData =          kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);
	mp_test_result->pShortRData =               kcalloc(p_mp_func->max_channel_num, sizeof(int), GFP_KERNEL);

	mp_test_result->puniformity_deltac = 		kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pdelta_LR_buf = 		kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pdelta_UD_buf = 		kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
	mp_test_result->pborder_ratio_buf = 		kcalloc(7*mp_test_data->sensorInfo.numSen, sizeof(int), GFP_KERNEL);
	mp_test_result->pdeltac_buffer = 		kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);

    /* Check allocated memory status */
	if(ERR_ALLOC_MEM(mp_test_result->pCheck_Fail) || ERR_ALLOC_MEM(mp_test_result->pOpenResultData) ||
		ERR_ALLOC_MEM(mp_test_result->pOpenFailChannel) || ERR_ALLOC_MEM(mp_test_result->pOpenRatioFailChannel))
	{
		mp_err("Failed to allocate channels's mem\n");
		return -1;
	}

	if(ERR_ALLOC_MEM(mp_test_result->pGolden_CH) || ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Max) ||
	    ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Max_Avg)|| ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Min) ||
		ERR_ALLOC_MEM(mp_test_result->pGolden_CH_Min_Avg))
	{
		mp_err("Failed to allocate pGolden_CH's mem\n");
		return -1;
	}

	if(ERR_ALLOC_MEM(mp_test_result->pShortFailChannel) || ERR_ALLOC_MEM(mp_test_result->pShortResultData) ||
	    ERR_ALLOC_MEM(mp_test_result->pICPinChannel)|| ERR_ALLOC_MEM(mp_test_result->pICPinShortFailChannel) ||
		ERR_ALLOC_MEM(mp_test_result->pICPinShortResultData))
	{
		mp_err("Failed to allocate pShortFailChannel's mem\n");
		return -1;
	}

	if(ERR_ALLOC_MEM(mp_test_result->pICPinShortRData) || ERR_ALLOC_MEM(mp_test_result->pShortRData))
	{
		mp_err("Failed to allocate pICPinShortRData's mem\n");
		return -1;
	}

	if(ERR_ALLOC_MEM(mp_test_result->puniformity_deltac) || ERR_ALLOC_MEM(mp_test_result->pdelta_LR_buf) ||
		ERR_ALLOC_MEM(mp_test_result->pdelta_UD_buf) || ERR_ALLOC_MEM(mp_test_result->pborder_ratio_buf) ||
		ERR_ALLOC_MEM(mp_test_result->pdeltac_buffer))
	{
		mp_err("Failed to allocate Uniformaity's mem\n");
		return -1;
	}

	return 0;
}

static int mp_start_test(void)
{
    int i, res = 0, retry = 0;

	mp_info("*** %s() ***\n", __func__);

    DrvDisableFingerTouchReport();
    DrvTouchDeviceHwReset();

    res = mp_main_init_var();
	if(res < 0)
		goto out;

    mp_test_data->get_deltac_flag = 0;
	/* Open Test */
    if(mp_test_data->UIConfig.bOpenTest == 1)
	{
        mp_info("*** Running Open Test ***\n");
		for(retry = 0; retry < MP_RETRY_TEST_TIME; retry++)
		{
			mp_test_result->nOpenResult = mp_new_flow_main(RUN_OPEN_TEST);
			if(mp_test_result->nOpenResult ==  ITO_TEST_OK)
					break;

			mp_info("MP OPEN TEST Fail, Retry %d\n", retry + 1);
		}
    	for (i = 0; i < MAX_MUTUAL_NUM; i++)
    	{
    		mp_test_result->pGolden_CH[i] = mp_test_data->Goldensample_CH_0[i];
    		mp_test_result->pGolden_CH_Max[i] = mp_test_data->Goldensample_CH_0_Max[i];
    		mp_test_result->pGolden_CH_Min[i] = mp_test_data->Goldensample_CH_0_Min[i];
    	}
    }
    else 
	{
    	mp_test_result->nOpenResult = ITO_NO_TEST;
    }

	/* Short Test */
    if(mp_test_data->UIConfig.bShortTest == 1)
	{
		mp_info("*** Running Short Test ***\n");
		for(retry = 0; retry < MP_RETRY_TEST_TIME; retry++)
		{
			mp_test_result->nShortResult = mp_new_flow_main(RUN_SHORT_TEST);
			if(mp_test_result->nShortResult ==  ITO_TEST_OK)
				break;

			mp_info("MP SHORT TEST Fail, Retry %d\n", retry + 1);
		}
		mp_info("*** End Short Test ***\n");
    }
    else 
	{
    	mp_test_result->nShortResult = ITO_NO_TEST;
    }

	/* Uniformity Test */
	if(mp_test_data->UIConfig.bUniformityTest == 1)
	{
		mp_info("*** Running Uniformity Test ***\n");
		mp_test_result->nUniformityResult = mp_new_flow_main(RUN_UNIFORMITY_TEST);
	}
	else
	{
		mp_test_result->nUniformityResult = ITO_NO_TEST;
	}

	/* Return final result */
	if(mp_test_result->nShortResult == ITO_TEST_FAIL || mp_test_result->nOpenResult == ITO_TEST_FAIL 
			|| mp_test_result->nUniformityResult == ITO_TEST_FAIL)
	{
		res = ITO_TEST_FAIL;
	}
	else
	{
		res = ITO_TEST_OK;
	}

    pr_info("Final Result(%d): Short = %d, Open = %d, Uniformity = %d \n",
			res, mp_test_result->nShortResult, mp_test_result->nOpenResult,  mp_test_result->nUniformityResult);

 out:
	DrvTouchDeviceHwReset();
	mdelay(100);
	DrvEnableFingerTouchReport();
	return res;
}

void mp_main_free(void)
{
	mp_info("*** %s *** \n",__func__);

	mp_kfree((void **)&mp_test_data->ana_version);
	if(mp_test_data->sensorInfo.numKey > 0)
	{
		mp_kfree((void **)&mp_test_data->KeySen);
		mp_kfree((void **)&mp_test_data->sensorInfo.key_type);
		mp_info("MEM free mp_test_data->KeySen\n");
		mp_info("MEM free mp_test_data->sensorInfo.key_type\n");
	}
	mp_kfree((void **)&mp_test_data->UIConfig.sSupportIC );
	mp_kfree((void **)&mp_test_data->project_name);
	mp_kfree((void **)&mp_test_data->Goldensample_CH_0 );
	mp_kfree((void **)&mp_test_data->Goldensample_CH_0_Max);
	mp_kfree((void **)&mp_test_data->Goldensample_CH_0_Max_Avg);
	mp_kfree((void **)&mp_test_data->Goldensample_CH_0_Min);
	mp_kfree((void **)&mp_test_data->Goldensample_CH_0_Min_Avg);
	mp_kfree((void **)&mp_test_data->PAD2Drive);
	mp_kfree((void **)&mp_test_data->PAD2Sense);
	if (mp_test_data->sensorInfo.numGr)
	{
		mp_kfree((void **)&mp_test_data->PAD2GR);
		mp_info("MEM free mp_test_data->PAD2GR\n");
	}

	mp_kfree((void **)&mp_test_result->pCheck_Fail);
	mp_kfree((void **)&mp_test_result->pOpenResultData);
	mp_kfree((void **)&mp_test_result->pOpenFailChannel);
	mp_kfree((void **)&mp_test_result->pOpenRatioFailChannel);

	mp_kfree((void **)&mp_test_result->pGolden_CH);
	mp_kfree((void **)&mp_test_result->pGolden_CH_Max);
	mp_kfree((void **)&mp_test_result->pGolden_CH_Max_Avg);
	mp_kfree((void **)&mp_test_result->pGolden_CH_Min);
	mp_kfree((void **)&mp_test_result->pGolden_CH_Min_Avg);

	mp_kfree((void **)&mp_test_result->pShortFailChannel);
	mp_kfree((void **)&mp_test_result->pShortResultData);
	mp_kfree((void **)&mp_test_result->pShortRData);

	mp_kfree((void **)&mp_test_result->pICPinChannel);
	mp_kfree((void **)&mp_test_result->pICPinShortFailChannel);
	mp_kfree((void **)&mp_test_result->pICPinShortResultData);
	mp_kfree((void **)&mp_test_result->pICPinShortRData);

	mp_kfree((void **)&mp_test_result->puniformity_deltac);
	mp_kfree((void **)&mp_test_result->pdelta_LR_buf);
	mp_kfree((void **)&mp_test_result->pdelta_UD_buf);
	mp_kfree((void **)&mp_test_result->pborder_ratio_buf);
	mp_kfree((void **)&mp_test_result->pdeltac_buffer);


	mp_kfree((void **)&mp_test_data);
	mp_kfree((void **)&mp_test_result);
	mp_kfree((void **)&p_mp_func);
}

int startMPTest(int nChipType, char *pFilePath)
{
	int res = 0;

	mp_info("*** nChipType = 0x%x *** \n",nChipType);
    mp_info("*** iniPath = %s *** \n", pFilePath);

	mutex_lock(&g_Mutex);
    g_IsInMpTest = 1;
    mutex_unlock(&g_Mutex);

	/* Init main structure members */
    p_mp_func = kmalloc(sizeof(struct mp_main_func), GFP_KERNEL);
    if(ERR_ALLOC_MEM(p_mp_func)) {
        mp_err("Failed to allocate mp_func mem\n");
        res = -ENOMEM;
        goto out;
    }

	p_mp_func->chip_type = nChipType;

    if (nChipType == CHIP_TYPE_MSG58XXA) {
        p_mp_func->check_mp_switch = msg30xx_check_mp_switch;
        p_mp_func->enter_mp_mode = msg30xx_enter_mp_mode;
		p_mp_func->open_judge = msg30xx_open_judge;
		p_mp_func->short_judge = msg30xx_short_judge;
        p_mp_func->uniformity_judge = msg30xx_uniformity_judge;
		p_mp_func->max_channel_num = MAX_CHANNEL_NUM_30XX;
		p_mp_func->fout_data_addr = 0x1361;
    } else if (nChipType == CHIP_TYPE_MSG28XX) {
        p_mp_func->check_mp_switch = msg28xx_check_mp_switch;
        p_mp_func->enter_mp_mode = msg28xx_enter_mp_mode;
		p_mp_func->open_judge = msg28xx_open_judge;
		p_mp_func->short_judge = msg28xx_short_judge;
        p_mp_func->uniformity_judge = msg28xx_uniformity_judge;        
		p_mp_func->max_channel_num = MAX_CHANNEL_NUM_28XX;
		p_mp_func->fout_data_addr = 0x136E;
    } else {
        mp_err("New MP Flow doesn't support this chip type\n");
        res = -1;
        goto out;
    }

	/* Parsing ini file and prepare to run MP test */
    res = mp_load_ini(pFilePath);
    if(res < 0) {
        mp_err("Failed to load ini\n");
        goto out;
    }

    res = mp_start_test();

	mp_save_result(res);

out:
	mp_main_free();
	mutex_lock(&g_Mutex);
    g_IsInMpTest = 0;
    mutex_unlock(&g_Mutex);
	return res;
}