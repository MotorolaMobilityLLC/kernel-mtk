#ifndef __MAG_FACTORY_H__
#define __MAG_FACTORY_H__

#include "mag.h"
#include "cust_mag.h"

extern struct mag_context *mag_context_obj;

#define SETCALI 1
#define CLRCALI 2
#define GETCALI 3

int mag_factory_device_init(void);

#endif

