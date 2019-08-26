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

#ifndef __MP_COMMON_H__
#define __MP_COMMON_H__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include "../ilitek_drv_common.h"

#define MP_DEBUG

#ifdef MP_DEBUG
#define mp_err(fmt, arg...) pr_err("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg);
#define mp_info(fmt, arg...) pr_info("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg);
#else
#define mp_err(fmt, arg...) pr_err("ILITEK: (%s, %d): " fmt, __func__, __LINE__, ##arg);
#define mp_info(fmt, arg...)
#endif

#define ERR_ALLOC_MEM(X)	((IS_ERR(X) || X == NULL) ? 1 : 0)
#define ADDR(a,b) (( a*mp_test_data->sensorInfo.numDrv)+(b))
#define _ABS(x,y) ((x-y) > 0 ? (x-y) : (y-x))

#define	TEST_ITEM_NUM 8
#define FILENAME_MAX 4096
#define MAX_MUTUAL_NUM 1904
#define MAX_CHANNEL_DRV  40
#define MAX_CHANNEL_SEN  40
#define	MAX_CHANNEL_NUM_28XX	60
#define MAX_CHANNEL_NUM_30XX    49
#define MAX_SUBFRAMES_30XX      28
#define MAX_AFENUMs_30XX 2
#define NULL_DATA -3240
#define ENABLE_BY_PASS_CHANNEL
#define NORMAL_JUDGE  0
#define NO_RATIO_JUDGE 1
#define NO_JUDGE 2
#define NOTCH_JUDGE 3
#define PIN_NO_ERROR 0xFFFF
#define IIR_MAX 32600
#define PIN_UN_USE 0xABCD
#define MP_RETRY_TEST_TIME 3

//MPTest Result
#define ITO_NO_TEST                   0
#define ITO_TEST_OK                   1
#define ITO_TEST_FAIL                 -1
#define ITO_TEST_GET_TP_TYPE_ERROR    3
#define ITO_TEST_UNDEFINED_ERROR      4
#define ITO_TEST_PROCESSING		      5

#define INI_PATH "/sdcard/autoSettings.ini"

#define RUN_OPEN_TEST	0
#define RUN_SHORT_TEST	1
#define RUN_UNIFORMITY_TEST	2

typedef enum
{
    ONE_DAC_ENABLE = 0,
    TWO_DAC_ENABLE = 1,
} ItoTestDACStatus;

typedef enum
{
    SINGLE = 0,
    MULTI = 1,
} ItoTestScreenType;

typedef enum
{
    eDAC_0,
    eDAC_1,
} ItoTestDAC;

typedef enum
{
	_Msg30xx50p,
	_Msg30xx45p,
	_Msg30xx35p,
	_Msg30xx30p,
	_Msg30xx25p
} ItoTestMsg30xxPin;

typedef struct
{
    u16 X;
    u16 Y;
} MutualMapping_t;

typedef struct
{
    char * sSupportIC;
    int bmDKVerify;
    int bCurrentTest;
    int bChipVerify;
    int bFWUpdate;
    int bFWTest;
    int bOpenTest;
    int bShortTest;
    int bUniformityTest;
    int bWpTest;
    int bFunctionTest;
    int bAutoStart;
    int bAutoMation;
    int bTriggerMode;
    int bTSMode;
    int bTSEnable;
    int bPhaseKTest;
} MutualUIConfig_t;

typedef struct
{
    u16 persentDC_VA_Range;
    u16 persentDC_VA_Ratio;

    u16 persentDC_Border_Ratio;
    u16 persentDC_VA_Range_up;
    u16 persentDC_VA_Ratio_up;

    u16 persentDG_Range;
    u16 persentDG_Ratio;
    u16 persentDG_Range_up;
    u16 persentDG_Ratio_up;
    u16 persentWater_DG_Range;
} MutualToast_t;

typedef struct
{
    u16 numKey;
    u16 numKeyLine;
    u16 numDummy;
    u16 numTriangle;
    u16 KeyDrv;
    u16 KEY_CH;
    u16 KeyDrv_o;
    char *key_type; 
    int thrsShort;
    int thrsICpin;
    int thrsOpen;
    int thrsWater;

    u16 numSen;
    u16 numDrv;
    u16 numGr;
    MutualMapping_t *mapping;
} MutualSensor_t;

typedef struct
{
    u32  bd_top;
    u32  bd_bottom;
    u32  bd_l_top;
    u32  bd_r_top;
    u32  bd_l_bottom;
    u32  bd_r_bottom;
    u32  va_top;
    u32  va_bottom;
    u32  notch;
}MpUniformityPart;

typedef struct
{
	MutualUIConfig_t UIConfig;
    int logResult;
    int logFWResult;

    int Enable;

    char * ana_version;
    char * project_name;
    char * binname;
    char * versionFW;
    u16 slaveI2cID;
    char * stationNow;
    char * inipassword;
    u16 Mutual_Key;
    u16 Pattern_type;
    u16 Pattern_model;

    int Crc_check;

    MutualSensor_t sensorInfo;
    MutualToast_t ToastInfo;
    int FPC_threshold;
    int KeyDC_threshold;
    int KEY_Timer;
    int Open_test_csub;
    int Open_test_cfb;
    int Open_mode;
    int Open_fixed_carrier;
    int Open_fixed_carrier1;
    int Open_test_chargepump;
    int Open_test_mode;
    int Open_Swt;
    int Open_Cbg2deltac_ratio;
    int Open_Csig_Vari_Up;
    int Open_Csig_Vari_Low;
    int Open_Csig_Vari_Notch_Up;
    int Open_Csig_Vari_Notch_Low;
    int Short_test_mode;
    int inverter_mode;
    int Current_threshold;
    int CurrentThreshold_Powerdown;

    int OPEN_Charge;
    int OPEN_Dump;
    int SHORT_Postidle;
    int SHORT_Charge;
    int SHORT_Dump1;
    int SHORT_Dump2;
    int SHORT_Hopping;
    int SHORT_Fout_max_1;
    int SHORT_Fout_max_2;
    int SHORT_Capture_count;
    int SHORT_AVG_count;
    int SHORT_sensor_pin_number;
    int SHORT_sample_number;
    int Water_Charge;
    int Water_Dump;

    int * KeySen;

    int * Goldensample_CH_0_Max_Avg;///[ana26xxM.MAX_MUTUAL_NUM];
    int * Goldensample_CH_0_Max;    ///[ana26xxM.MAX_MUTUAL_NUM];
    int * Goldensample_CH_0;        ///[ana26xxM.MAX_MUTUAL_NUM];
    int * Goldensample_CH_0_Min;    ///[ana26xxM.MAX_MUTUAL_NUM];
    int * Goldensample_CH_0_Min_Avg;///[ana26xxM.MAX_MUTUAL_NUM];

    int * PhaseGolden_Max;    ///[ana26xxM.MAX_MUTUAL_NUM];
    int * PhaseGolden;        ///[ana26xxM.MAX_MUTUAL_NUM];
    int * PhaseGolden_Min;    ///[ana26xxM.MAX_MUTUAL_NUM];

    int * PhaseWaterGolden_Max;
    int * PhaseWaterGolden;
    int * PhaseWaterGolden_Min;

    u16 * PAD2Sense;
    u16 * PAD2Drive;
    u16 * PAD2GR;

    u16 * phase_freq;
    u16 freq_num;
    u16 phase_time;
    u16 band_num;
    u16 * pgd;
    u16 * water_pgd;
    u16 * water_sense;
    u16 * water_drive;
    u16 charge_pump;
    u16 raw_type;
    u16 noise_thd;
    u16 sub_frame;
    u16 afe_num;
    u16 phase_sen_num;
    u16 * phase_sense;
    u16 water_sen_num;
    u16 water_drv_num;
    int update_bin;
    int force_phaseK;
    int update_info;
    int log_phasek;
    int border_drive_phase;
    int sw_calibration;
    u8 phase_version;
    u8 mapping_version;
    int deep_standby;
    int deep_standby_timeout;
    int Open_KeySettingByFW;
    u8 post_idle;
    u8 self_sample_hi;
    u8 self_sample_lo;
    MpUniformityPart uniformity_ratio;
    MpUniformityPart bd_va_ratio_max;
    MpUniformityPart bd_va_ratio_min;
    int get_deltac_flag;
} MutualMpTest_t;

typedef struct
{
	int nOpenResult;
	int nShortResult;
        int nUniformityResult;
	int nWaterProofResult;

	int nRatioAvg_max;
	int nRatioAvg_min;
	int nBorder_RatioAvg_max;
	int nBorder_RatioAvg_min;

    int * pOpenResultData;
    int * pOpenFailChannel;
    int * pOpenRatioFailChannel;
    int * pShortResultData;
    int * pShortRData;
    int * pICPinShortResultData;
    int * pICPinShortRData;
    int * pICPinChannel;
    int * pShortFailChannel;
    int * pICPinShortFailChannel;
    int * pWaterProofResultData;
    int * pWaterProofFailChannel;

    int *pCheck_Fail;
    int *pResult_DeltaC;
    int *pGolden_CH_Max_Avg;
    int *pGolden_CH_Min_Avg;
    int *pGolden_CH_Max;
    int *pGolden_CH_Min;
    int *pGolden_CH;
    char *mapTbl_sec;
    int *puniformity_deltac;
    int *pdeltac_buffer;
    int *pdelta_LR_buf;
    int *pdelta_UD_buf;
    int *pborder_ratio_buf;
    int  uniformity_check_fail[3];
    MpUniformityPart uniformity_judge;

} MutualMpTestResult_t;

enum new_flow_test_type
{
    TYPE_RESET = 0x0,
    TYPE_OPEN = 0x1,
    TYPE_SHORT = 0x2,
    TYPE_BootPalm = 0x3,
    TYPE_KPhase = 0x4,
    TYPE_SELF = 0x5,
    TYPE_DQPhase = 0x6,
    TYPE_SHORT_HOPPING = 0x7,
};

struct mp_main_func
{
    int chip_type;
    int fout_data_addr;
    int max_channel_num;
    int sense_len;
    int drive_len;
    int gr_len;
    int (*check_mp_switch)(void);
    int (*enter_mp_mode)(void);
    int (*open_judge)(int *deltac_data, int size);
    int (*short_judge)(int *deltac_data);
    int (*uniformity_judge)(int *deltac_data, int size);    
};

extern MutualMpTest_t *mp_test_data;
extern MutualMpTestResult_t *mp_test_result; 
extern struct mp_main_func *p_mp_func;

extern int mp_new_flow_main(int item);
extern int startMPTest(int nChipType, char *pFilePath);

extern int mp_parse(char *path);
extern int ms_get_ini_data(char *section, char *ItemName, char *returnValue);
extern int ms_ini_split_u8_array(char * key, u8 * pBuf);
extern int ms_ini_split_int_array(char * key, int * pBuf);
extern int ms_ini_split_golden(int *pBuf, int line);
extern int ms_ini_split_u16_array(char * key, u16 * pBuf);
extern int ms_atoi(char *nptr);

extern int msg28xx_check_mp_switch(void);
extern int msg28xx_enter_mp_mode(void);
extern int msg28xx_short_judge(int *deltac_data);
extern int msg28xx_open_judge(int *deltac_data, int deltac_size);
extern int msg28xx_uniformity_judge(int *deltac_data, int deltac_size);

extern int msg30xx_check_mp_switch(void);
extern int msg30xx_enter_mp_mode(void);
extern int msg30xx_short_judge(int *deltac_data);
extern int msg30xx_open_judge(int *deltac_data, int deltac_size);
extern int msg30xx_uniformity_judge(int *deltac_data, int deltac_size);

extern void Msg30xxDBBusReadDQMemStart(void);
extern void Msg30xxDBBusReadDQMemEnd(void);

extern void StopMCU(void);
extern void StartMCU(void);
extern void EnterDBBus(void);
extern void ExitDBBus(void);

extern void mp_save_result(int result);

static inline void mp_kfree(void **mem) {
	if(*mem != NULL) {
		kfree(*mem);
		*mem = NULL;
	}
}

static inline u8 check_thrs(s32 nValue, s32 nMax, s32 nMin) {  	
    return ((nValue <= nMax && nValue >= nMin) ? 1 : 0);
}

#endif