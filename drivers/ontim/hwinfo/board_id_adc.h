/*************************************************************************
    > File Name: board_id_adc.h
    > Author: jiangfuxiong
    > Mail: fuxiong.jiang@ontim.cn
    > Created Time: 2018年09月18日 星期二 15时33分33秒
 ************************************************************************/

#ifndef _LINUX_FOCLATECH_CONFIG_H_
#define _LINUX_FOCLATECH_CONFIG_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/qpnp/qpnp-adc.h>

//#define ADC_ERROR(fmt, args...) printk(KERN_ERR "[ADC_BOARD_ID][Error]"fmt"\n", ##args)
//struct qpnp_vadc_result adc_result;
struct adc_board_id {
	int vadc_mux;
	int voltage;
	struct qpnp_vadc_chip *vadc_dev;
};

extern struct adc_board_id *chip_adc;

#endif
