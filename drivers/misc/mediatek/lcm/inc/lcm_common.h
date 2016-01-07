#ifndef __LCM_COMMON_H__
#define __LCM_COMMON_H__

#include "lcm_drv.h"

typedef enum {
	LCM_STATUS_OK = 0,
	LCM_STATUS_ERROR,
} LCM_STATUS;


void lcm_common_parse_dts(const LCM_DTS *DTS, unsigned char force_update);
void lcm_common_set_util_funcs(const LCM_UTIL_FUNCS *util);
void lcm_common_get_params(LCM_PARAMS *params);
void lcm_common_init(void);
void lcm_common_suspend(void);
void lcm_common_resume(void);
void lcm_common_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
void lcm_common_setbacklight(unsigned int level);
unsigned int lcm_common_compare_id(void);

#endif
