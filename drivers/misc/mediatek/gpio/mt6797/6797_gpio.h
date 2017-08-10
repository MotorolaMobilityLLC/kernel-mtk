#ifndef _6797_GPIO_H
#define _6797_GPIO_H

#include "mt_gpio_base.h"
#include <linux/slab.h>
#include <linux/device.h>

extern void gpio_dump_regs(void);
extern ssize_t mt_gpio_show_pin(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t mt_gpio_store_pin(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
struct device_node *get_gpio_np(void);
extern long gpio_pull_select_unsupport[MAX_GPIO_PIN];
extern long gpio_pullen_unsupport[MAX_GPIO_PIN];
extern long gpio_smt_unsupport[MAX_GPIO_PIN];
void mt_get_md_gpio_debug(char *str);
#endif   /*_6797_GPIO_H*/
