#ifndef __OIS_EXT_CMD_H__
#define __OIS_EXT_CMD_H__

#define OIS_HEA_MAX_TEST_POINTS 40
#define OIS_HEA_CHANNEL_NUM     2
#define OIS_EXT_DATA_MAX_LEN    (OIS_HEA_MAX_TEST_POINTS*OIS_HEA_CHANNEL_NUM*6*sizeof(u16)+4*sizeof(u16))

typedef enum {
	OIS_EXT_INVALID_CMD,
	//Action
	OIS_SART_FW_DL,
	OIS_START_HEA_TEST,
	OIS_START_GYRO_OFFSET_CALI,
	OIS_ACTION_MAX = OIS_START_GYRO_OFFSET_CALI,
	//Result
	OIS_QUERY_FW_INFO,
	OIS_QUERY_HEA_RESULT,
	OIS_QUERY_GYRO_OFFSET_RESULT,
	OIS_EXT_INTF_MAX
} motOISExtInfType;

typedef struct {
	u16 version;
	u16 date;
	u16 valid;
	u16 gOffset_valid;
} motOISFwInfo;

typedef struct {
	u16 radius;
	u16 accuracy;
	u16 steps_in_degree;
	u16 wait0;
	u16 wait1;
	u16 wait2;
	u16 ref_stroke;
} motOISHeaParam;

typedef struct {
	u16 points;
	u16 ng_points;
	u16 g_targetAdc[OIS_HEA_CHANNEL_NUM*2][OIS_HEA_MAX_TEST_POINTS];
	u16 g_diffs[OIS_HEA_CHANNEL_NUM][OIS_HEA_MAX_TEST_POINTS];
} motOISHeaResult;

typedef struct {
	u16 is_success;
	u16 x_offset;
	u16 y_offset;
} motOISGOffsetResult;

typedef union {
	motOISFwInfo fw_info;
	motOISHeaParam hea_param;
	motOISHeaResult hea_result;
	motOISGOffsetResult gyro_offset_result;
	u32 data[OIS_EXT_DATA_MAX_LEN/sizeof(u32)];
} motOISExtData;

typedef struct {
	motOISExtInfType cmd;
	motOISExtData    data;
} motOISExtIntf;
#endif
