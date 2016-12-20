#ifndef __ALSPS_FACTORY_H__
#define __ALSPS_FACTORY_H__

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/atomic.h>

#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <batch.h>

/*#include <mach/mt_typedefs.h>*/
/*#include <mach/mt_gpio.h>*/
/*#include <mach/mt_pm_ldo.h>*/

#include "alsps.h"
#include "cust_alsps.h"

extern struct alsps_context *alsps_context_obj;

#define SETCALI 1
#define CLRCALI 2
#define GETCALI 3

#define GET_TH_HIGH	1
#define GET_TH_LOW		2
#define SET_TH			3
#define GET_TH_RESULT	4

int alsps_factory_device_init(void);

#endif

