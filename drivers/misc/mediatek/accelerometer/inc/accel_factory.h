#ifndef __ACC_FACTORY_H__
#define __ACC_FACTORY_H__

#include <linux/uaccess.h>
#include <sensors_io.h>
#include "cust_acc.h"
#include "accel.h"

extern struct acc_context *acc_context_obj;

#define SETCALI 1
#define CLRCALI 2
#define GETCALI 3

int acc_factory_device_init(void);

#endif
