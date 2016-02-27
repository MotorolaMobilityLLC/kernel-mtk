#ifndef __LCM_GPIO_H__
#define __LCM_GPIO_H__

#include "lcm_drv.h"
#include "lcm_common.h"


#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
#define LCM_GPIO_DEVICE	"lcm_mode"


LCM_STATUS lcm_gpio_set_data(char type, const LCM_DATA_T1 *t1);
#endif

#endif

