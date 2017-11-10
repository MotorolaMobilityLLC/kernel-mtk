/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: DetailThreshold.h
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: Set Detail Threshold for all IC
*
************************************************************************/

#ifndef _DETAIL_THRESHOLD_H
#define _DETAIL_THRESHOLD_H

#define TX_NUM_MAX			50
#define RX_NUM_MAX			50
#define MAX_PATH			256
//#define BUFFER_LENGTH		80*80*8
#define BUFFER_LENGTH		512
#define MAX_TEST_ITEM		100
#define MAX_GRAPH_ITEM       20
#define MAX_CHANNEL_NUM	144

struct stCfg_MCap_DetailThreshold
{
	unsigned char InvalidNode[TX_NUM_MAX][RX_NUM_MAX];
	unsigned char InvalidNode_SC[TX_NUM_MAX][RX_NUM_MAX];

	int RawDataTest_Min[TX_NUM_MAX][RX_NUM_MAX];
	int RawDataTest_Max[TX_NUM_MAX][RX_NUM_MAX];
	int RawDataTest_Low_Min[TX_NUM_MAX][RX_NUM_MAX];
	int RawDataTest_Low_Max[TX_NUM_MAX][RX_NUM_MAX];
	int RawDataTest_High_Min[TX_NUM_MAX][RX_NUM_MAX];
	int RawDataTest_High_Max[TX_NUM_MAX][RX_NUM_MAX];

	int RxLinearityTest_Max[TX_NUM_MAX][RX_NUM_MAX];
	int TxLinearityTest_Max[TX_NUM_MAX][RX_NUM_MAX];


	int SCapRawDataTest_ON_Max[TX_NUM_MAX][RX_NUM_MAX];
	int SCapRawDataTest_ON_Min[TX_NUM_MAX][RX_NUM_MAX];
	int SCapRawDataTest_OFF_Max[TX_NUM_MAX][RX_NUM_MAX];
	int SCapRawDataTest_OFF_Min[TX_NUM_MAX][RX_NUM_MAX];
	short SCapCbTest_ON_Max[TX_NUM_MAX][RX_NUM_MAX];
	short SCapCbTest_ON_Min[TX_NUM_MAX][RX_NUM_MAX];
	short SCapCbTest_OFF_Max[TX_NUM_MAX][RX_NUM_MAX];
	short SCapCbTest_OFF_Min[TX_NUM_MAX][RX_NUM_MAX];



	int NoistTest_Coefficient[TX_NUM_MAX][RX_NUM_MAX];


};

struct stCfg_SCap_DetailThreshold
{
	int TempData[MAX_CHANNEL_NUM];
	int RawDataTest_Max[MAX_CHANNEL_NUM];
	int RawDataTest_Min[MAX_CHANNEL_NUM];
	int CiTest_Max[MAX_CHANNEL_NUM];
	int CiTest_Min[MAX_CHANNEL_NUM];
	int DeltaCiTest_Base[MAX_CHANNEL_NUM];
	int DeltaCiTest_AnotherBase1[MAX_CHANNEL_NUM];
	int DeltaCiTest_AnotherBase2[MAX_CHANNEL_NUM];
	int CiDeviationTest_Base[MAX_CHANNEL_NUM];
	
	int NoiseTest_Max[MAX_CHANNEL_NUM];
	int DeltaCxTest_Sort[MAX_CHANNEL_NUM];         
	int DeltaCxTest_Area[MAX_CHANNEL_NUM];         

	int CbTest_Max[MAX_CHANNEL_NUM];
	int CbTest_Min[MAX_CHANNEL_NUM];
	int DeltaCbTest_Base[MAX_CHANNEL_NUM];
	int DifferTest_Base[MAX_CHANNEL_NUM];
	int CBDeviationTest_Base[MAX_CHANNEL_NUM];
	int K1DifferTest_Base[MAX_CHANNEL_NUM];

	/*
	stCfg_SCap_DetailThreshold()
	{
		memset(TempData, 0, sizeof(TempData));

		memset(RawDataTest_Max, 0, sizeof(RawDataTest_Max));
		memset(RawDataTest_Min, 1, sizeof(RawDataTest_Min));

		memset(CiTest_Max, 0, sizeof(RawDataTest_Max));
		memset(CiTest_Min, 1, sizeof(RawDataTest_Min));

		memset(DeltaCiTest_Base, 0, sizeof(DeltaCiTest_Base));
		memset(DeltaCiTest_AnotherBase1, 0, sizeof(DeltaCiTest_AnotherBase1));
		memset(DeltaCiTest_AnotherBase2, 0, sizeof(DeltaCiTest_AnotherBase2));
	
		memset(NoiseTest_Max, 0, sizeof(NoiseTest_Max));
		memset(CiDeviationTest_Base, 0, sizeof(CiDeviationTest_Base));
		
		memset(DeltaCxTest_Sort, 1, sizeof(DeltaCxTest_Sort));

		memset(CbTest_Max, 0, sizeof(CbTest_Max));
		memset(CbTest_Min, 1, sizeof(CbTest_Min));
		memset(DeltaCbTest_Base, 0, sizeof(DeltaCbTest_Base));
        	memset(DifferTest_Base, 0, sizeof(DifferTest_Base));
		memset(CBDeviationTest_Base, 0, sizeof(CBDeviationTest_Base));
	}
	*/

};

//struct stCfg_MCap_DetailThreshold g_stCfg_MCap_DetailThreshold;
//struct stCfg_SCap_DetailThreshold g_stCfg_SCap_DetailThreshold;

void OnInit_MCap_DetailThreshold(char *strIniFile);
void OnInit_SCap_DetailThreshold(char *strIniFile);

void OnInit_InvalidNode(char *strIniFile);
void OnGetTestItemParam(char *strItemName, char *strIniFile, int iDefautValue);
void OnInit_DThreshold_RawDataTest(char *strIniFile);
void OnInit_DThreshold_SCapRawDataTest(char *strIniFile);
void OnInit_DThreshold_SCapCbTest(char *strIniFile);

void OnInit_DThreshold_RxLinearityTest(char *strIniFile);//For FT5822
void OnInit_DThreshold_TxLinearityTest(char *strIniFile);//For FT5822

void set_max_channel_num(void);

#endif
