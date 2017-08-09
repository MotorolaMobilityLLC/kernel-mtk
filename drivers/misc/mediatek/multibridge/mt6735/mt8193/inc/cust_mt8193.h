#ifndef CUST_MT8193_H
#define CUST_MT8193_H

#include "cust_gpio_usage.h"

#define MT8193_I2C_ID       1
#define USING_MT8193_DPI1   1
#define MT8193_BUS_SWITCH_PIN_GPIO_MODE     GPIO_MODE_00 /* gpio mode */
#define MT8193_BUS_SWITCH_PIN_DPI_MODE      GPIO_MODE_02 /* dpi1d0 mode */
#define GPIO_MT8193_BUS_SWITCH_PIN          (GPIO0 | 0x80000000)

#endif /* CUST_MT8193_H */

