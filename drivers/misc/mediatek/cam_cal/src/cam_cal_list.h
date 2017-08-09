/*****************************************************************************
 *
 * Filename:
 * ---------
 *   cam_cal_list.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of CAM_CAL driver
 *
 *
 * Author:
 * -------
 *   LukeHu (MTK10439)
 *
 *============================================================================*/
#ifndef __CAM_CAL_LIST_H
#define __CAM_CAL_LIST_H
#include <linux/i2c.h>

/*LukeHu++150910=For Common Sendcommand Founction*/

typedef unsigned int (*checkFunc)(struct i2c_client *, unsigned int);
#define cam_cal_check_func checkFunc
typedef struct {
	unsigned int sensorID;
	unsigned int slaveID;
	unsigned int cmdType;
	cam_cal_check_func checkFunc;
} stCAM_CAL_LIST_STRUCT, *stPCAM_CAL_LIST_STRUCT;

typedef unsigned int (*camCalCMDFunc)(struct i2c_client *client, unsigned int,
				      unsigned char *, unsigned int);

#define cam_cal_cmd_func camCalCMDFunc

typedef struct {
	unsigned char cmdType;
	cam_cal_cmd_func readCamCalData;
} stCAM_CAL_FUNC_STRUCT, *stPCAM_CAL_FUNC_STRUCT;

unsigned int cam_cal_get_sensor_list(stCAM_CAL_LIST_STRUCT **ppCamcalList);
unsigned int cam_cal_get_func_list(stCAM_CAL_FUNC_STRUCT **ppCamcalFuncList);
unsigned int cam_cal_check_mtk_cid(struct i2c_client *client, unsigned int cmdIndx);

#endif /* __CAM_CAL_LIST_H */

