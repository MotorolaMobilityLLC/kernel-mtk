/* Sim check driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2016 Longcheer
 * All Rights Reserved
 *
 * VERSION: V0.1
 * HISTORY:
 * V0.0 --- Driver creation
 * V0.1 --- Modify for EVT2 & decrease init function 
 */

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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <linux/seq_file.h>
#include <mt-plat/mt_gpio.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <linux/of_irq.h>

#include "lct_print.h"

#ifdef LCT_LOG_MOD
#undef LCT_LOG_MOD
#define LCT_LOG_MOD "[SIM_CHECK]"
#endif

//define from hardware design
#define LCT_BOARD_ID 0
#define LCT_SKU_ID_1 59
#define LCT_SKU_ID_2 60

static int sku_id_value = 0;
static struct pinctrl *sim_check_pinctrl;
static struct pinctrl_state *sim_check_pin_default;

static char *gpio_str[]={
	"default", "sim_check_gpio", "sku_id_gpio1", "sku_id_gpio2","sku_id_gpio3", "sku_id_gpio4"
};

//define gpio number
//The sequency is Board_ID, HW_ID1, HW_ID2, HW_ID3, HW_ID4
static int gpio_hw[]={0,86,78,59,60};

/// FUNCTION_NAME:dual_sim_check_show 
///
/// @Param: dev
/// @Param: attr
/// @Param: buf
///
/// @Returns: 
static ssize_t dual_sim_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	static int board_id_value = 0;

	board_id_value  = sku_id_value & 0x01;

	lct_pr_debug("%s: board_id_value = %d\n",__FUNCTION__, board_id_value);
	return snprintf(buf, PAGE_SIZE, "%d\n", board_id_value);
}

/// FUNCTION_NAME:sku_check_show 
///
/// @Param: dev
/// @Param: attr
/// @Param: buf
///
/// @Returns: 
static ssize_t sku_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/************************************************************
	 *
	 * ID4  ID3   ID2   ID1  ID0  SUM  PRUODUCT      SKU
	 ************************************************************
	 *  0     0    0     0    1    1     EVT2     A01(DUAL SIM)
	 *  0     0    0     1    1    3     EVT2     B01(DUAL SIM)
	 *  0     0    0     1    0    2     EVT2     C01(SIG SIM)
	 *  0     0    1     0    1    5     EVT2     D01(DUAL SIM)
	 *  0     0    1     0    0    4     EVT2     E01(SIG SIM)
	 ************************************************************
	 *  
	 *  0     1    0     0    1    9     DVT1     A01(DUAL SIM)
	 *  0     1    0     1    1    B     DVT1     B01(DUAL SIM)
	 *  0     1    0     1    0    A     DVT1     C01(SIG SIM)
	 *  0     1    1     0    1    D     DVT1     D01(DUAL SIM)
	 *  0     1    1     0    0    C     DVT1     E01(SIG SIM)
	 ************************************************************
	 *  
	 *  1     0    0     0    1    11     DVT2     A01(DUAL SIM)
	 *  1     0    0     1    1    13     DVT2     B01(DUAL SIM)
	 *  1     0    0     1    0    12     DVT2     C01(SIG SIM)
	 *  1     0    1     0    1    15     DVT2     D01(DUAL SIM)
	 *  1     0    1     0    0    14     DVT2     E01(SIG SIM)
	 ************************************************************
	 *  
	 *  1     1    0     0    1    19     PVT     A01(DUAL SIM)
	 *  1     1    0     1    1    1B     PVT     B01(DUAL SIM)
	 *  1     1    0     1    0    1A     PVT     C01(SIG SIM)
	 *  1     1    1     0    1    1D     PVT     D01(DUAL SIM)
	 *  1     1    1     0    0    1C     PVT     E01(SIG SIM)
	 *  
	 *************************************************************/
	return snprintf(buf, PAGE_SIZE, "%d\n", sku_id_value);
}

DEVICE_ATTR(sim_check, S_IRUGO, dual_sim_check_show, NULL);
DEVICE_ATTR(sku_check, S_IRUGO, sku_check_show, NULL);

static struct device_attribute *sim_sku_attr_list[] = {
	&dev_attr_sim_check,
	&dev_attr_sku_check,
};

/// FUNCTION_NAME:sim_sku_create_attr 
///
/// @Param: dev
///
/// @Returns: 
static int sim_sku_create_attr(struct device *dev)
{
	static int idx, err = 0;
	static int num = (int)(sizeof(sim_sku_attr_list) / sizeof(sim_sku_attr_list[0]));

	if (!dev)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = device_create_file(dev, sim_sku_attr_list[idx]);
		if (err) {
			lct_pr_err("device_create_file (%s) = %d\n", sim_sku_attr_list[idx]->attr.name,
				err);
			break;
		}
	}

	return err;
}

/// FUNCTION_NAME:sim_check_gpio_set
///
/// @Returns: 
int sim_check_gpio_set(void)
{
	int gpio_num = 0;
	int i = 0;

	gpio_num = sizeof(gpio_str)/sizeof(gpio_str[0]);

	for(i = 0; i < gpio_num; i++)
	{
		sim_check_pin_default = pinctrl_lookup_state(sim_check_pinctrl, gpio_str[i]);
		if (IS_ERR(sim_check_pin_default)) {
			lct_pr_err("sim_check cannot find pinctrl %s\n",gpio_str[i]);
			continue;
		}
		pinctrl_select_state(sim_check_pinctrl, sim_check_pin_default);
	}

	return 0;
}

/// FUNCTION_NAME:sim_check_gpio_init 
///
/// @Returns: 
static int sim_check_gpio_init(void)
{
	static struct device_node *node;
	static struct platform_device *sim_check_dev=NULL;

	node = of_find_compatible_node(NULL, NULL, "longcheer,simcheck");

	sim_check_dev = of_find_device_by_node(node);

	if (node) {
		sim_check_pinctrl = devm_pinctrl_get(&(sim_check_dev->dev));
		if (IS_ERR(sim_check_pinctrl)) {
			lct_pr_debug("sim_check cannot find pinctrl\n");
			return -1;
		}
	}
	else
	{
		lct_pr_err("[sim_check]%s can't find compatible node\n", __func__);
		return -1;
	}

	return 0;
}

/// FUNCTION_NAME:sim_check_gpio_probe 
///
/// @Param: pdev
///
/// @Returns: 
static int sim_check_gpio_probe(struct platform_device *pdev)
{
	int i = 0;
	int value = 0;
	int gpio_hw_length = 0;

	if(sim_check_gpio_init())
	{
		lct_pr_err("sim_check gpio init error\n");
		return -1;
	}

	if(sim_check_gpio_set())
	{
		lct_pr_err("sim_check set gpio error\n");
		return -2;
	}

	gpio_hw_length = sizeof(gpio_hw)/sizeof(gpio_hw[0]);
	for(i = 0; i < gpio_hw_length; i++)
	{
		value = gpio_get_value(gpio_hw[i]);
		lct_pr_debug("%s: gpio[%d]=%d\n",__FUNCTION__,gpio_hw[i],value);
		sku_id_value += (value<<i);
	}
	lct_pr_debug("%s: sku_id_value=%d\n",__FUNCTION__,sku_id_value);
	
	sim_sku_create_attr(&(pdev->dev));

	lct_pr_debug("%s ok\n",__FUNCTION__);

	return 0;
}

/// FUNCTION_NAME:sim_check_gpio_remove 
///
/// @Param: pdev
///
/// @Returns: 
static int sim_check_gpio_remove(struct platform_device *pdev)
{
	lct_pr_err("prboe fail, remove this driver\n");
	return 0;
}


static const struct of_device_id sim_check_gpio_of_match[] = {
	{ .compatible = "longcheer,simcheck", },
	{ }
};
MODULE_DEVICE_TABLE(of, sim_check_gpio_of_match);

static struct platform_driver sim_check_gpio_driver = {
	.probe = sim_check_gpio_probe,
	.remove = sim_check_gpio_remove,
	.driver = {
		.name = "sim_check-gpio",
		.owner = THIS_MODULE,
		.of_match_table = sim_check_gpio_of_match,
	},
};
module_platform_driver(sim_check_gpio_driver);

/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("LONGCHEER SIM CHECK DRIVER");
MODULE_AUTHOR("zhaofei1@longcheer.com");

