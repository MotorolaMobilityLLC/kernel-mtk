#ifndef __LCM_I2C_H__
#define __LCM_I2C_H__

#include "lcm_drv.h"
#include "lcm_common.h"


#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
LCM_STATUS lcm_i2c_set_data(char type, const LCM_DATA_T2 *t2);
#endif

#endif

