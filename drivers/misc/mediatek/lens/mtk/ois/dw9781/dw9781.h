#ifndef __DW9781_H__
#define __DW9781_H__

#include "ois_ext_cmd.h"

#define EOK 0

/* gyro offset calibration */
#define GYRO_OFFSET_CAL_OK 0x00
#define GYRO_CAL_TIME_OVER 0xFF
#define X_GYRO_OFFSET_SPEC_OVER_NG 0x0001
#define X_GYRO_RAW_DATA_CHECK 0x0010
#define Y_GYRO_OFFSET_SPEC_OVER_NG 0x0002
#define Y_GYRO_RAW_DATA_CHECK 0x0020
#define GYRO_OFST_CAL_OVERCNT 50

int gyro_offset_calibrtion(void);
motOISGOffsetResult *dw9781_get_gyro_offset_result(void);
void calibration_save(void);
void check_calibration_data(void);
int Cross_Motion_Test(unsigned short RADIUS, unsigned short ACCURACY,
                      unsigned short DEG_STEP, unsigned short WAIT_TIME1,
                      unsigned short WAIT_TIME2, motOISExtData *pResult);
#endif
