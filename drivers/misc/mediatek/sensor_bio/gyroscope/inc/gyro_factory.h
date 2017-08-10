#ifndef __GYRO_FACTORY_H__
#define __GYRO_FACTORY_H__

#include "gyroscope.h"
#include "cust_gyro.h"

extern struct gyro_context *gyro_context_obj;

#define SETCALI 1
#define CLRCALI 2
#define GETCALI 3

int gyro_factory_device_init(void);

#endif

