#ifndef _SMARTPA_H_
#define _SMARTPA_H_

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/slab.h>

struct smartpa_pin_cfg {
	struct pinctrl_state *rst_low;
	struct pinctrl_state *rst_high;
};

struct smartpa_hw {
	struct pinctrl *pinctrl;
	struct smartpa_pin_cfg pin_cfg;
};

enum {
	LOW,
	HIGH
};

int smartpa_rst_ctl(struct smartpa_hw *hw, u8 enable);
int smartpa_init(struct platform_device *pdev, struct smartpa_hw *hw);

#endif
