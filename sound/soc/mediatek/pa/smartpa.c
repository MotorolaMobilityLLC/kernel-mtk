#include "smartpa.h"

static long
get_smartpa_pin_info(struct device *dev, struct smartpa_hw *hw)
{
	long ret = 0;

	if (NULL == dev || NULL == hw)
		goto arg_err;

	hw->pinctrl = devm_pinctrl_get(dev);

	if (IS_ERR(hw->pinctrl)) {
		printk("Cannot find smartpa pinctrl!\n");
		ret = PTR_ERR(hw->pinctrl);
		return ret;
	} else {
		hw->pin_cfg.rst_low = pinctrl_lookup_state(hw->pinctrl, "smartpa_rst_low");
		if (IS_ERR(hw->pin_cfg.rst_low)) {
			ret = PTR_ERR(hw->pin_cfg.rst_low);
			printk("Cannot find smartpa pinctrl smartpa_rst_low!\n");
			return ret;
		}

		hw->pin_cfg.rst_high = pinctrl_lookup_state(hw->pinctrl, "smartpa_rst_high");
		if (IS_ERR(hw->pin_cfg.rst_high)) {
			ret = PTR_ERR(hw->pin_cfg.rst_high);
			printk("Cannot find smartpa pinctrl smartpa_rst_high!\n");
			return ret;
		}
	}

	return ret;

arg_err:
	return -1;
}
#if 0
static int
register_smartpa_i2c_client(const char *name, struct smartpa_hw *hw)
{
	int ret = 0;
	u32 i2c_num = 0;
	u32 i2c_addr = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_board_info info;
	struct device_node *node = NULL;

	printk("Device Tree get %s i2c info!\n", name);

	if (NULL == name || NULL == hw)
		goto arg_err;


	/* 1: get arg */
	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", &i2c_num, 1);
		if (ret == 0) {
			hw->i2c_num	= i2c_num;
		} else {
			printk("cant not found %s i2c_num\n", name);
			goto arg_err;
		}

		ret = of_property_read_u32_array(node , "i2c_addr", &i2c_addr, 1);
		if (ret == 0) {
			hw->i2c_addr = i2c_addr;
		} else {
			printk("cant not found %s i2c_addr\n", name);
			goto arg_err;
		}
	} else {
		printk("Device Tree: can not find %s node!. Go to use old cust info\n", name);
		goto arg_err;
	}

	printk("i2c num=%d, i2c_addr=0x%x\n", hw->i2c_num, hw->i2c_addr);

	/* register i2c_client */
	memset(&info, 0, sizeof(struct i2c_board_info));
	strcpy(info.type, name);
	info.addr = i2c_addr;
	adapter = i2c_get_adapter(hw->i2c_num);
	i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);

	return 0;

arg_err:
	return -1;
}
#endif
int
smartpa_rst_ctl(struct smartpa_hw *hw, u8 enable)
{
	if (NULL == hw)
		goto err;

	if (1 == enable) {
		if (IS_ERR(hw->pin_cfg.rst_high)) {
			printk("Can not find smartpa_rst_high, set fail!!!");
			goto err;
		} else
			pinctrl_select_state(hw->pinctrl, hw->pin_cfg.rst_high);
	} else {
		if (IS_ERR(hw->pin_cfg.rst_low)) {
			printk("Can not find smartpa_rst_low, set fail!!!");
			goto err;
		} else
			pinctrl_select_state(hw->pinctrl, hw->pin_cfg.rst_low);
	}

	return 0;
err:
	return -1;
}

int
smartpa_init(struct platform_device *pdev, struct smartpa_hw *hw)
{
	if (get_smartpa_pin_info(&pdev->dev, hw)) {
		printk("get_smartpa_pin_info failed\n");
		goto err;
	}

	return 0;

err:
	return -1;
}

