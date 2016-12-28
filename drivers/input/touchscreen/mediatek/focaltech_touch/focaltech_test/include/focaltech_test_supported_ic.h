/************************************************************************
* Copyright (C) 2012-2016, Focaltech Systems (R) All Rights Reserved.
*
* File Name: focaltech_test_supported_ic.h
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: test entry for all IC
*
************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>

#include "../focaltech_test_config.h"
#include "focaltech_test_main.h"

#if (FT8716_TEST)

struct stCfg_FT8716_TestItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool CB_TEST;
	bool SHORT_CIRCUIT_TEST;
	bool OPEN_TEST;
	bool CB_UNIFORMITY_TEST;
	bool DIFFER_UNIFORMITY_TEST;
	bool DIFFER2_UNIFORMITY_TEST;
	bool LCD_NOISE_TEST;

};
struct stCfg_FT8716_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	bool bRawDataTest_VA_Check;
	int RawDataTest_Min;
	int RawDataTest_Max;
	bool bRawDataTest_VKey_Check;
	int RawDataTest_Min_VKey;
	int RawDataTest_Max_VKey;
	BYTE ChannelNumTest_ChannelXNum;
	BYTE ChannelNumTest_ChannelYNum;
	BYTE ChannelNumTest_KeyNum;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	int NoiseTest_Coefficient;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	bool bCBTest_VA_Check;
	int CbTest_Min;
	int CbTest_Max;
	bool bCBTest_VKey_Check;
	int CbTest_Min_Vkey;
	int CbTest_Max_Vkey;

	bool bCBTest_VKey_DCheck_Check;
	int CbTest_Min_DCheck_Vkey;
	int CbTest_Max_DCheck_Vkey;

	bool bLcdBusyAdjust;
	int ShortCircuit_ResMin;
	/* int ShortTest_K2Value; */
	int OpenTest_CBMin;
	bool OpenTest_Check_K1;
	int OpenTest_K1Threshold;
	bool OpenTest_Check_K2;
	int OpenTest_K2Threshold;

	bool CBUniformityTest_Check_CHX;
	bool CBUniformityTest_Check_CHY;
	bool CBUniformityTest_Check_MinMax;
	int CBUniformityTest_CHX_Hole;
	int CBUniformityTest_CHY_Hole;
	int CBUniformityTest_MinMax_Hole;

	bool DifferUniformityTest_Check_CHX;
	bool DifferUniformityTest_Check_CHY;
	bool DifferUniformityTest_Check_MinMax;
	int DifferUniformityTest_CHX_Hole;
	int DifferUniformityTest_CHY_Hole;
	int DifferUniformityTest_MinMax_Hole;
	int DeltaVol;

	bool Differ2UniformityTest_Check_CHX;
	bool Differ2UniformityTest_Check_CHY;
	int Differ2UniformityTest_CHX_Hole;
	int Differ2UniformityTest_CHY_Hole;
	int Differ2UniformityTest_Differ_Min;
	int Differ2UniformityTest_Differ_Max;

	int LCDNoiseTest_FrameNum;
	int LCDNoiseTest_Coefficient;
	int LCDNoiseTest_Coefficient_Key;
	BYTE LCDNoiseTest_NoiseMode;
	int LCDNoiseTest_SequenceFrame;
	int LCDNoiseTest_MaxFrame;

};
enum enumTestItem_FT8716 {
	Code_FT8716_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT8716_DOWNLOAD,	/* All IC are required to test items */
	Code_FT8716_UPGRADE,	/* All IC are required to test items */
	Code_FT8716_FACTORY_ID_TEST,
	Code_FT8716_PROJECT_CODE_TEST,
	Code_FT8716_FW_VERSION_TEST,
	Code_FT8716_IC_VERSION_TEST,
	Code_FT8716_RAWDATA_TEST,
	Code_FT8716_CHANNEL_NUM_TEST,
	/* Code_FT8716_CHANNEL_SHORT_TEST, */
	Code_FT8716_INT_PIN_TEST,
	Code_FT8716_RESET_PIN_TEST,
	Code_FT8716_NOISE_TEST,
	Code_FT8716_CB_TEST,
	/* Code_FT8716_DELTA_CB_TEST, */
	/* Code_FT8716_CHANNELS_DEVIATION_TEST, */
	/* Code_FT8716_TWO_SIDES_DEVIATION_TEST, */
	/* Code_FT8716_FPC_SHORT_TEST, */
	/* Code_FT8716_FPC_OPEN_TEST, */
	/* Code_FT8716_SREF_OPEN_TEST, */
	/* Code_FT8716_TE_TEST, */
	/* Code_FT8716_CB_DEVIATION_TEST, */
	Code_FT8716_WRITE_CONFIG,	/* All IC are required to test items */
	/* Code_FT8716_DIFFER_TEST, */
	Code_FT8716_SHORT_CIRCUIT_TEST,
	Code_FT8716_OPEN_TEST,
	Code_FT8716_CB_UNIFORMITY_TEST,
	Code_FT8716_DIFFER_UNIFORMITY_TEST,
	Code_FT8716_DIFFER2_UNIFORMITY_TEST,
	Code_FT8716_LCD_NOISE_TEST,
};

#elif (FT8607_TEST)

struct stCfg_FT8607_TESTItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool CB_TEST;
	bool SHORT_CIRCUIT_TEST;
	bool LCD_NOISE_TEST;
	bool OSC60MHZ_TEST;
	bool OSCTRM_TEST;
	bool SNR_TEST;
	bool LPWG_RAWDATA_TEST;
	bool LPWG_CB_TEST;
	bool LPWG_NOISE_TEST;
	bool DIFFER_TEST;
	bool DIFFER_UNIFORMITY_TEST;
	bool OPEN_TEST;
};

struct stCfg_FT8607_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	int RawDataTest_Min;
	int RawDataTest_Max;
	BYTE ChannelNumTest_ChannelXNum;
	BYTE ChannelNumTest_ChannelYNum;
	BYTE ChannelNumTest_KeyNum;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	int NoiseTest_Coefficient;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	BYTE IsDifferMode;
	bool bCBTest_VA_Check;
	int CbTest_Min;
	int CbTest_Max;
	bool bCBTest_VKey_Check;
	int CbTest_Min_Vkey;
	int CbTest_Max_Vkey;
	int ShortCircuit_ResMin;
	/*int ShortTest_Max;
	   int ShortTest_K2Value;
	   bool ShortTest_Tip; */
	int iLCDNoiseTestFrame;
	int iLCDNoiseTestMax;

	int iLCDNoiseCoefficient;
	int OSC60MHZTest_OSCMin;
	int OSC60MHZTest_OSCMax;

	int OSCTRMTest_OSCMin;
	int OSCTRMTest_OSCMax;
	int OSCTRMTest_OSCDetMin;
	int OSCTRMTest_OSCDetMax;
	int SNRTest_FrameNum;
	int SNRTest_Min;

	int DIFFERTest_FrameNum;
	int DIFFERTest_DifferMax;
	int DIFFERTest_DifferMin;

	bool DifferUniformityTest_Check_CHX;
	bool DifferUniformityTest_Check_CHY;
	bool DifferUniformityTest_Check_MinMax;
	int DifferUniformityTest_CHX_Hole;
	int DifferUniformityTest_CHY_Hole;
	int DifferUniformityTest_MinMax_Hole;

	int LPWG_RawDataTest_Min;
	int LPWG_RawDataTest_Max;

	bool bLPWG_CBTest_VA_Check;
	int LPWG_CbTest_Min;
	int LPWG_CbTest_Max;
	bool bLPWG_CBTest_VKey_Check;
	int LPWG_CbTest_Min_Vkey;
	int LPWG_CbTest_Max_Vkey;

	int LPWG_NoiseTest_Coefficient;
	int LPWG_NoiseTest_Frames;
	int LPWG_NoiseTest_Time;
	BYTE LPWG_NoiseTest_SampeMode;
	BYTE LPWG_NoiseTest_NoiseMode;
	BYTE LPWG_NoiseTest_ShowTip;
	BYTE LPWG_IsDifferMode;

	int Open_Test_CBMin;
};

enum enumTestItem_FT8607 {
	Code_FT8607_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT8607_DOWNLOAD,	/* All IC are required to test items */
	Code_FT8607_UPGRADE,	/* All IC are required to test items */
	Code_FT8607_FACTORY_ID_TEST,
	Code_FT8607_PROJECT_CODE_TEST,
	Code_FT8607_FW_VERSION_TEST,
	Code_FT8607_IC_VERSION_TEST,
	Code_FT8607_RAWDATA_TEST,
	Code_FT8607_CHANNEL_NUM_TEST,
	/* Code_FT8607_CHANNEL_SHORT_TEST, */
	Code_FT8607_INT_PIN_TEST,
	Code_FT8607_RESET_PIN_TEST,
	Code_FT8607_NOISE_TEST,
	Code_FT8607_CB_TEST,
	/* Code_FT8607_DELTA_CB_TEST, */
	/* Code_FT8607_CHANNELS_DEVIATION_TEST, */
	/* Code_FT8607_TWO_SIDES_DEVIATION_TEST, */
	/* Code_FT8607_FPC_SHORT_TEST, */
	/* Code_FT8607_FPC_OPEN_TEST, */
	/* Code_FT8607_SREF_OPEN_TEST, */
	/* Code_FT8607_TE_TEST, */
	/* Code_FT8607_CB_DEVIATION_TEST, */
	Code_FT8607_WRITE_CONFIG,	/* All IC are required to test items */
	/* Code_FT8607_DIFFER_TEST, */
	Code_FT8607_SHORT_CIRCUIT_TEST,
	Code_FT8607_LCD_NOISE_TEST,

	Code_FT8607_OSC60MHZ_TEST,
	Code_FT8607_OSCTRM_TEST,
	Code_FT8607_SNR_TEST,
	Code_FT8607_DIFFER_TEST,
	Code_FT8607_DIFFER_UNIFORMITY_TEST,

	Code_FT8607_LPWG_RAWDATA_TEST,
	Code_FT8607_LPWG_CB_TEST,
	Code_FT8607_LPWG_NOISE_TEST,

	Code_FT8607_OPEN_TEST,
};

#elif (FT5X46_TEST)
/*-----------------------------------------------
FT5X46 and FT5X22 is the same series of chips
FT5422\FT5X22  is the chip code for internally develope
FT5X46 is the chip code for market
------------------------------------------------*/
struct stCfg_FT5X46_TestItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool ADC_DETECT_TEST;
	bool SCAP_CB_TEST;
	bool SCAP_RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool WEAK_SHORT_CIRCUIT_TEST;
	bool UNIFORMITY_TEST;
	bool CM_TEST;

	bool RAWDATA_MARGIN_TEST;
	bool PANEL_DIFFER_TEST;
	bool PANEL_DIFFER_UNIFORMITY_TEST;

	bool LCM_ID_TEST;
	bool PANEL_ID_TEST;

	bool TE_TEST;
	bool SITO_RAWDATA_UNIFORMITY_TEST;
	bool PATTERN_TEST;
	bool GPIO_TEST;
	bool LCD_NOISE_TEST;
};
struct stCfg_FT5X46_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	BYTE LCM_ID;
	int RawDataTest_low_Min;
	int RawDataTest_Low_Max;
	int RawDataTest_high_Min;
	int RawDataTest_high_Max;
	BYTE RawDataTest_SetLowFreq;
	BYTE RawDataTest_SetHighFreq;
	int AdcDetect_Max;
	/* int RxShortTest_Min; */
	/* int RxShortTest_Max; */
	/* int TxShortTest_Min; */
	/* int TxShortTest_Max; */
	int SCapCbTest_OFF_Min;
	int SCapCbTest_OFF_Max;
	int SCapCbTest_ON_Min;
	int SCapCbTest_ON_Max;
	bool SCapCbTest_LetTx_Disable;
	BYTE SCapCbTest_SetWaterproof_OFF;
	BYTE SCapCbTest_SetWaterproof_ON;
	/* int SCapDifferTest_Min; */
	/* int SCapDifferTest_Max; */
	/* BYTE SCapDifferTest_CbLevel; */
	int SCapRawDataTest_OFF_Min;
	int SCapRawDataTest_OFF_Max;
	int SCapRawDataTest_ON_Min;
	int SCapRawDataTest_ON_Max;
	bool SCapRawDataTest_LetTx_Disable;
	BYTE SCapRawDataTest_SetWaterproof_OFF;
	BYTE SCapRawDataTest_SetWaterproof_ON;
	bool bChannelTestMapping;
	bool bChannelTestNoMapping;
	BYTE ChannelNumTest_TxNum;
	BYTE ChannelNumTest_RxNum;
	BYTE ChannelNumTest_TxNpNum;
	BYTE ChannelNumTest_RxNpNum;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	BYTE IntPinTest_TestNum;
	int NoiseTest_Max;
	int GloveNoiseTest_Coefficient;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	bool bNoiseTest_GloveMode;
	int NoiseTest_RawdataMin;
	unsigned char Set_Frequency;
	bool bNoiseThreshold_Choose;
	int NoiseTest_Threshold;
	int NoiseTest_MinNgFrame;
	/* BYTE SCapClbTest_Frame; */
	/* int SCapClbTest_Max; */
	/* int RxCrosstalkTest_Min; */
	/* int RxCrosstalkTest_Max; */
	/* int RawDataRxDeviationTest_Max; */
	/* int RawDataUniformityTest_Percent; */
	/* int RxLinearityTest_Max; */
	/* int TxLinearityTest_Max; */
	/* int DifferDataUniformityTest_Percent; */
	int WeakShortTest_CG;
	int WeakShortTest_CC;
	int WeakShortTest_CC_Rsen;
	bool WeakShortTest_CapShortTest;

	/* int WeakShortTest_ChannelNum; */
	bool Uniformity_CheckTx;
	bool Uniformity_CheckRx;
	bool Uniformity_CheckMinMax;
	int Uniformity_Tx_Hole;
	int Uniformity_Rx_Hole;
	int Uniformity_MinMax_Hole;
	bool CMTest_CheckMin;
	bool CMTest_CheckMax;
	int CMTest_MinHole;
	int CMTest_MaxHole;

	int RawdataMarginTest_Min;
	int RawdataMarginTest_Max;

	int PanelDifferTest_Min;
	int PanelDifferTest_Max;

	bool PanelDiffer_UniformityTest_Check_Tx;
	bool PanelDiffer_UniformityTest_Check_Rx;
	bool PanelDiffer_UniformityTest_Check_MinMax;
	int PanelDiffer_UniformityTest_Tx_Hole;
	int PanelDiffer_UniformityTest_Rx_Hole;
	int PanelDiffer_UniformityTest_MinMax_Hole;

	bool SITO_RawdtaUniformityTest_Check_Tx;
	bool SITO_RawdtaUniformityTest_Check_Rx;
	int SITO_RawdtaUniformityTest_Tx_Hole;
	int SITO_RawdtaUniformityTest_Rx_Hole;

	bool bPattern00;
	bool bPatternFF;
	bool bPattern55;
	bool bPatternAA;
	bool bPatternBin;

	int Lcd_Noise_MaxFrame;
	int Lcd_Noise_Conficient;
	int Lcd_Noise_Noise_Mode;
	int Lcd_Noise_MaxNgPoint;
	int Lcd_Noise_FrameNum;

	bool Lcd_Noise_NoiseThresholdMode;
	int Lcd_Noise_NoiseCoefficient;
	int Lcd_Noise_NoiseMax;
	int Lcd_Noise_SetFrequency;
};
enum enumTestItem_FT5X46 {
	Code_FT5X46_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT5X46_DOWNLOAD,	/* All IC are required to test items */
	Code_FT5X46_UPGRADE,	/* All IC are required to test items */
	Code_FT5X46_FACTORY_ID_TEST,
	Code_FT5X46_PROJECT_CODE_TEST,
	Code_FT5X46_FW_VERSION_TEST,
	Code_FT5X46_IC_VERSION_TEST,
	Code_FT5X46_RAWDATA_TEST,
	Code_FT5X46_ADCDETECT_TEST,
	Code_FT5X46_SCAP_CB_TEST,
	Code_FT5X46_SCAP_RAWDATA_TEST,
	Code_FT5X46_CHANNEL_NUM_TEST,
	Code_FT5X46_INT_PIN_TEST,
	Code_FT5X46_RESET_PIN_TEST,
	Code_FT5X46_NOISE_TEST,
	Code_FT5X46_WEAK_SHORT_CIRCUIT_TEST,
	Code_FT5X46_UNIFORMITY_TEST,
	Code_FT5X46_CM_TEST,
	Code_FT5X46_RAWDATA_MARGIN_TEST,
	Code_FT5X46_WRITE_CONFIG,	/* All IC are required to test items */
	Code_FT5X46_PANELDIFFER_TEST,
	Code_FT5X46_PANELDIFFER_UNIFORMITY_TEST,
	Code_FT5X46_LCM_ID_TEST,
	Code_FT5X46_JUDEG_NORMALIZE_TYPE,
	Code_FT5X46_TE_TEST,
	Code_FT5X46_SITO_RAWDATA_UNIFORMITY_TEST,
	Code_FT5X46_PATTERN_TEST,
	Code_FT5X46_GPIO_TEST,
	Code_FT5X46_LCD_NOISE_TEST,
};

#elif (FT6X36_TEST)

struct stCfg_FT6X36_TestItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool CHANNEL_SHORT_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool CB_TEST;
	bool DELTA_CB_TEST;
	bool CHANNELS_DEVIATION_TEST;
	bool TWO_SIDES_DEVIATION_TEST;
	bool FPC_SHORT_TEST;
	bool FPC_OPEN_TEST;
	bool SREF_OPEN_TEST;
	bool TE_TEST;
	bool CB_DEVIATION_TEST;
	bool DIFFER_TEST;
	bool WEAK_SHORT_TEST;
	bool DIFFER_TEST2;
	bool K1_DIFFER_TEST;
};

struct stCfg_FT6X36_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	int RawDataTest_Min;
	int RawDataTest_Max;
	BYTE ChannelNumTest_ChannelNum;
	BYTE ChannelNumTest_KeyNum;
	int ChannelShortTest_K1;
	int ChannelShortTest_K2;
	int ChannelShortTest_CB;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	int WeakShortThreshold;
	int NoiseTest_Max;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	int FPCShort_CB_Min;
	int FPCShort_CB_Max;
	int FPCShort_RawData_Min;
	int FPCShort_RawData_Max;
	int FPCOpen_CB_Min;
	int FPCOpen_CB_Max;
	int FPCOpen_RawData_Min;
	int FPCOpen_RawData_Max;
	int SREFOpen_Hole_Base1;
	int SREFOpen_Hole_Base2;
	int SREFOpen_Hole;
	int CBDeviationTest_Hole;
	int Differ_Ave_Hole;
	int Differ_Max_Hole;
	int CbTest_Min;
	int CbTest_Max;
	int DeltaCbTest_Base;
	int DeltaCbTest_Differ_Max;
	bool DeltaCbTest_Include_Key_Test;
	int DeltaCbTest_Key_Differ_Max;
	int DeltaCbTest_Deviation_S1;
	int DeltaCbTest_Deviation_S2;
	int DeltaCbTest_Deviation_S3;
	int DeltaCbTest_Deviation_S4;
	int DeltaCbTest_Deviation_S5;
	int DeltaCbTest_Deviation_S6;
	bool DeltaCbTest_Set_Critical;
	int DeltaCbTest_Critical_S1;
	int DeltaCbTest_Critical_S2;
	int DeltaCbTest_Critical_S3;
	int DeltaCbTest_Critical_S4;
	int DeltaCbTest_Critical_S5;
	int DeltaCbTest_Critical_S6;

	int ChannelsDeviationTest_Deviation_S1;
	int ChannelsDeviationTest_Deviation_S2;
	int ChannelsDeviationTest_Deviation_S3;
	int ChannelsDeviationTest_Deviation_S4;
	int ChannelsDeviationTest_Deviation_S5;
	int ChannelsDeviationTest_Deviation_S6;
	bool ChannelsDeviationTest_Set_Critical;
	int ChannelsDeviationTest_Critical_S1;
	int ChannelsDeviationTest_Critical_S2;
	int ChannelsDeviationTest_Critical_S3;
	int ChannelsDeviationTest_Critical_S4;
	int ChannelsDeviationTest_Critical_S5;
	int ChannelsDeviationTest_Critical_S6;

	int TwoSidesDeviationTest_Deviation_S1;
	int TwoSidesDeviationTest_Deviation_S2;
	int TwoSidesDeviationTest_Deviation_S3;
	int TwoSidesDeviationTest_Deviation_S4;
	int TwoSidesDeviationTest_Deviation_S5;
	int TwoSidesDeviationTest_Deviation_S6;
	bool TwoSidesDeviationTest_Set_Critical;
	int TwoSidesDeviationTest_Critical_S1;
	int TwoSidesDeviationTest_Critical_S2;
	int TwoSidesDeviationTest_Critical_S3;
	int TwoSidesDeviationTest_Critical_S4;
	int TwoSidesDeviationTest_Critical_S5;
	int TwoSidesDeviationTest_Critical_S6;

	int DifferTest2_Data_H_Min;
	int DifferTest2_Data_H_Max;
	int DifferTest2_Data_M_Min;
	int DifferTest2_Data_M_Max;
	int DifferTest2_Data_L_Min;
	int DifferTest2_Data_L_Max;
	bool bDifferTest2_Data_H;
	bool bDifferTest2_Data_M;
	bool bDifferTest2_Data_L;
	int K1DifferTest_StartK1;
	int K1DifferTest_EndK1;
	int K1DifferTest_MinHold2;
	int K1DifferTest_MaxHold2;
	int K1DifferTest_MinHold4;
	int K1DifferTest_MaxHold4;
	int K1DifferTest_Deviation2;
	int K1DifferTest_Deviation4;
};

enum enumTestItem_FT6X36 {
	Code_FT6X36_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT6X36_DOWNLOAD,	/* All IC are required to test items */
	Code_FT6X36_UPGRADE,	/* All IC are required to test items */
	Code_FT6X36_FACTORY_ID_TEST,
	Code_FT6X36_PROJECT_CODE_TEST,
	Code_FT6X36_FW_VERSION_TEST,
	Code_FT6X36_IC_VERSION_TEST,
	Code_FT6X36_RAWDATA_TEST,
	Code_FT6X36_CHANNEL_NUM_TEST,
	Code_FT6X36_CHANNEL_SHORT_TEST,
	Code_FT6X36_INT_PIN_TEST,
	Code_FT6X36_RESET_PIN_TEST,
	Code_FT6X36_NOISE_TEST,
	Code_FT6X36_CB_TEST,
	Code_FT6X36_DELTA_CB_TEST,
	Code_FT6X36_CHANNELS_DEVIATION_TEST,
	Code_FT6X36_TWO_SIDES_DEVIATION_TEST,
	Code_FT6X36_FPC_SHORT_TEST,
	Code_FT6X36_FPC_OPEN_TEST,
	Code_FT6X36_SREF_OPEN_TEST,
	Code_FT6X36_TE_TEST,
	Code_FT6X36_CB_DEVIATION_TEST,
	Code_FT6X36_WRITE_CONFIG,	/* All IC are required to test items */
	Code_FT6X36_DIFFER_TEST,
	Code_FT6X36_WEAK_SHORT_TEST,
	Code_FT6X36_DIFFER_TEST2,
	Code_FT6X36_K1_DIFFER_TEST,
};

#elif (FT5822_TEST)

struct stCfg_FT5822_TestItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool ADC_DETECT_TEST;
	bool SCAP_CB_TEST;
	bool SCAP_RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool WEAK_SHORT_CIRCUIT_TEST;
	bool UNIFORMITY_TEST;
	bool CM_TEST;

	bool RAWDATA_MARGIN_TEST;
	bool PANEL_DIFFER_TEST;
	bool PANEL_DIFFER_UNIFORMITY_TEST;

	bool LCM_ID_TEST;

	bool TE_TEST;
	bool SITO_RAWDATA_UNIFORMITY_TEST;
	bool PATTERN_TEST;
};
struct stCfg_FT5822_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	BYTE LCM_ID;
	int RawDataTest_low_Min;
	int RawDataTest_Low_Max;
	int RawDataTest_high_Min;
	int RawDataTest_high_Max;
	BYTE RawDataTest_SetLowFreq;
	BYTE RawDataTest_SetHighFreq;
	int AdcDetect_Max;
	/* int RxShortTest_Min; */
	/* int RxShortTest_Max; */
	/* int TxShortTest_Min; */
	/* int TxShortTest_Max; */
	int SCapCbTest_OFF_Min;
	int SCapCbTest_OFF_Max;
	int SCapCbTest_ON_Min;
	int SCapCbTest_ON_Max;
	bool SCapCbTest_LetTx_Disable;
	BYTE SCapCbTest_SetWaterproof_OFF;
	BYTE SCapCbTest_SetWaterproof_ON;
	/* int SCapDifferTest_Min; */
	/* int SCapDifferTest_Max; */
	/* BYTE SCapDifferTest_CbLevel; */
	int SCapRawDataTest_OFF_Min;
	int SCapRawDataTest_OFF_Max;
	int SCapRawDataTest_ON_Min;
	int SCapRawDataTest_ON_Max;
	bool SCapRawDataTest_LetTx_Disable;
	BYTE SCapRawDataTest_SetWaterproof_OFF;
	BYTE SCapRawDataTest_SetWaterproof_ON;
	bool bChannelTestMapping;
	bool bChannelTestNoMapping;
	BYTE ChannelNumTest_TxNum;
	BYTE ChannelNumTest_RxNum;
	BYTE ChannelNumTest_TxNpNum;
	BYTE ChannelNumTest_RxNpNum;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	BYTE IntPinTest_TestNum;
	int NoiseTest_Max;
	int GloveNoiseTest_Coefficient;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	bool bNoiseTest_GloveMode;
	int NoiseTest_RawdataMin;
	unsigned char Set_Frequency;
	bool bNoiseThreshold_Choose;
	int NoiseTest_Threshold;
	int NoiseTest_MinNgFrame;
	/* BYTE SCapClbTest_Frame; */
	/* int SCapClbTest_Max; */
	/* int RxCrosstalkTest_Min; */
	/* int RxCrosstalkTest_Max; */
	/* int RawDataRxDeviationTest_Max; */
	/* int RawDataUniformityTest_Percent; */
	/* int RxLinearityTest_Max; */
	/* int TxLinearityTest_Max; */
	/* int DifferDataUniformityTest_Percent; */
	int WeakShortTest_CG;
	int WeakShortTest_CC;
	/* int WeakShortTest_ChannelNum; */
	bool Uniformity_CheckTx;
	bool Uniformity_CheckRx;
	bool Uniformity_CheckMinMax;
	int Uniformity_Tx_Hole;
	int Uniformity_Rx_Hole;
	int Uniformity_MinMax_Hole;
	bool CMTest_CheckMin;
	bool CMTest_CheckMax;
	int CMTest_MinHole;
	int CMTest_MaxHole;

	int RawdataMarginTest_Min;
	int RawdataMarginTest_Max;

	int PanelDifferTest_Min;
	int PanelDifferTest_Max;

	bool PanelDiffer_UniformityTest_Check_Tx;
	bool PanelDiffer_UniformityTest_Check_Rx;
	bool PanelDiffer_UniformityTest_Check_MinMax;
	int PanelDiffer_UniformityTest_Tx_Hole;
	int PanelDiffer_UniformityTest_Rx_Hole;
	int PanelDiffer_UniformityTest_MinMax_Hole;

	bool SITO_RawdtaUniformityTest_Check_Tx;
	bool SITO_RawdtaUniformityTest_Check_Rx;
	int SITO_RawdtaUniformityTest_Tx_Hole;
	int SITO_RawdtaUniformityTest_Rx_Hole;

	bool bPattern00;
	bool bPatternFF;
	bool bPattern55;
	bool bPatternAA;
	bool bPatternBin;
};
enum enumTestItem_FT5822 {
	Code_FT5822_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT5822_DOWNLOAD,	/* All IC are required to test items */
	Code_FT5822_UPGRADE,	/* All IC are required to test items */
	Code_FT5822_FACTORY_ID_TEST,
	Code_FT5822_PROJECT_CODE_TEST,
	Code_FT5822_FW_VERSION_TEST,
	Code_FT5822_IC_VERSION_TEST,
	Code_FT5822_RAWDATA_TEST,
	Code_FT5822_ADCDETECT_TEST,
	Code_FT5822_SCAP_CB_TEST,
	Code_FT5822_SCAP_RAWDATA_TEST,
	Code_FT5822_CHANNEL_NUM_TEST,
	Code_FT5822_INT_PIN_TEST,
	Code_FT5822_RESET_PIN_TEST,
	Code_FT5822_NOISE_TEST,
	Code_FT5822_WEAK_SHORT_CIRCUIT_TEST,
	Code_FT5822_UNIFORMITY_TEST,
	Code_FT5822_CM_TEST,
	Code_FT5822_RAWDATA_MARGIN_TEST,
	Code_FT5822_WRITE_CONFIG,	/* All IC are required to test items */
	Code_FT5822_PANELDIFFER_TEST,
	Code_FT5822_PANELDIFFER_UNIFORMITY_TEST,
	Code_FT5822_LCM_ID_TEST,
	Code_FT5822_JUDEG_NORMALIZE_TYPE,
	Code_FT5822_TE_TEST,
	Code_FT5822_SITO_RAWDATA_UNIFORMITY_TEST,
	Code_FT5822_PATTERN_TEST,
};

#elif (FT8006_TEST)

struct stCfg_FT8006_TestItem {
	bool FW_VERSION_TEST;
	bool FACTORY_ID_TEST;
	bool PROJECT_CODE_TEST;
	bool IC_VERSION_TEST;
	bool RAWDATA_TEST;
	bool CHANNEL_NUM_TEST;
	bool INT_PIN_TEST;
	bool RESET_PIN_TEST;
	bool NOISE_TEST;
	bool CB_TEST;
	bool SHORT_CIRCUIT_TEST;
	bool OPEN_TEST;
	bool CB_UNIFORMITY_TEST;
	bool DIFFER_UNIFORMITY_TEST;
	bool DIFFER2_UNIFORMITY_TEST;
	bool LCD_NOISE_TEST;

};
struct stCfg_FT8006_BasicThreshold {
	BYTE FW_VER_VALUE;
	BYTE Factory_ID_Number;
	char Project_Code[32];
	BYTE IC_Version;
	bool bRawDataTest_VA_Check;
	int RawDataTest_Min;
	int RawDataTest_Max;
	bool bRawDataTest_VKey_Check;
	int RawDataTest_Min_VKey;
	int RawDataTest_Max_VKey;
	BYTE ChannelNumTest_ChannelXNum;
	BYTE ChannelNumTest_ChannelYNum;
	BYTE ChannelNumTest_KeyNum;
	BYTE ResetPinTest_RegAddr;
	BYTE IntPinTest_RegAddr;
	int NoiseTest_Coefficient;
	int NoiseTest_Frames;
	int NoiseTest_Time;
	BYTE NoiseTest_SampeMode;
	BYTE NoiseTest_NoiseMode;
	BYTE NoiseTest_ShowTip;
	bool bCBTest_VA_Check;
	int CbTest_Min;
	int CbTest_Max;
	bool bCBTest_VKey_Check;
	int CbTest_Min_Vkey;
	int CbTest_Max_Vkey;

	bool bCBTest_VKey_DCheck_Check;
	int CbTest_Min_DCheck_Vkey;
	int CbTest_Max_DCheck_Vkey;

	bool bLcdBusyAdjust;
	int ShortCircuit_ResMin;
	/* int ShortTest_K2Value; */
	int OpenTest_CBMin;
	bool OpenTest_Check_K1;
	int OpenTest_K1Threshold;
	bool OpenTest_Check_K2;
	int OpenTest_K2Threshold;

	bool CBUniformityTest_Check_CHX;
	bool CBUniformityTest_Check_CHY;
	bool CBUniformityTest_Check_MinMax;
	int CBUniformityTest_CHX_Hole;
	int CBUniformityTest_CHY_Hole;
	int CBUniformityTest_MinMax_Hole;

	bool DifferUniformityTest_Check_CHX;
	bool DifferUniformityTest_Check_CHY;
	bool DifferUniformityTest_Check_MinMax;
	int DifferUniformityTest_CHX_Hole;
	int DifferUniformityTest_CHY_Hole;
	int DifferUniformityTest_MinMax_Hole;
	int DeltaVol;

	bool Differ2UniformityTest_Check_CHX;
	bool Differ2UniformityTest_Check_CHY;
	int Differ2UniformityTest_CHX_Hole;
	int Differ2UniformityTest_CHY_Hole;
	int Differ2UniformityTest_Differ_Min;
	int Differ2UniformityTest_Differ_Max;

	int LCDNoiseTest_FrameNum;
	int LCDNoiseTest_Coefficient;
	int LCDNoiseTest_Coefficient_Key;
	BYTE LCDNoiseTest_NoiseMode;
	int LCDNoiseTest_SequenceFrame;
	int LCDNoiseTest_MaxFrame;

};
enum enumTestItem_FT8006 {
	Code_FT8006_ENTER_FACTORY_MODE,	/* All IC are required to test items */
	Code_FT8006_DOWNLOAD,	/* All IC are required to test items */
	Code_FT8006_UPGRADE,	/* All IC are required to test items */
	Code_FT8006_FACTORY_ID_TEST,
	Code_FT8006_PROJECT_CODE_TEST,
	Code_FT8006_FW_VERSION_TEST,
	Code_FT8006_IC_VERSION_TEST,
	Code_FT8006_RAWDATA_TEST,
	Code_FT8006_CHANNEL_NUM_TEST,
	/* Code_FT8006_CHANNEL_SHORT_TEST, */
	Code_FT8006_INT_PIN_TEST,
	Code_FT8006_RESET_PIN_TEST,
	Code_FT8006_NOISE_TEST,
	Code_FT8006_CB_TEST,
	/* Code_FT8006_DELTA_CB_TEST, */
	/* Code_FT8006_CHANNELS_DEVIATION_TEST, */
	/* Code_FT8006_TWO_SIDES_DEVIATION_TEST, */
	/* Code_FT8006_FPC_SHORT_TEST, */
	/* Code_FT8006_FPC_OPEN_TEST, */
	/* Code_FT8006_SREF_OPEN_TEST, */
	/* Code_FT8006_TE_TEST, */
	/* Code_FT8006_CB_DEVIATION_TEST, */
	Code_FT8006_WRITE_CONFIG,	/* All IC are required to test items */
	/* Code_FT8006_DIFFER_TEST, */
	Code_FT8006_SHORT_CIRCUIT_TEST,
	Code_FT8006_OPEN_TEST,
	Code_FT8006_CB_UNIFORMITY_TEST,
	Code_FT8006_DIFFER_UNIFORMITY_TEST,
	Code_FT8006_DIFFER2_UNIFORMITY_TEST,
	Code_FT8006_LCD_NOISE_TEST,
};

#endif
