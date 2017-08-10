#ifndef _SENSOR_ATTR_H
#define _SENSOR_ATTR_H
#include <linux/major.h>
#include <linux/types.h>

struct sensor_attr_t  {
	unsigned char minor;
	const char *name;
	const struct file_operations *fops;
	struct list_head list;
	struct device *parent;
	struct device *this_device;
};
struct sensor_attr_dev {
	struct device *dev;
};
extern int sensor_attr_register(struct sensor_attr_t *misc);
extern int sensor_attr_deregister(struct sensor_attr_t *misc);

#endif
