
#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
//#include <mt-plat/mtk_boot.h>
//#include <mt-plat/upmu_common.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "wt6670f.h"

static DEFINE_MUTEX(wt6670f_i2c_access);
//static DEFINE_MUTEX(wt6670f_access_lock);
//static struct i2c_client *new_client;
//static int wt6670f_reset_pin = -1;
static int wt6670f_int_pin = -1;

struct pinctrl *i2c6_pinctrl;
struct pinctrl_state *i2c6_i2c;
struct pinctrl_state *i2c6_scl_low;
struct pinctrl_state *i2c6_scl_high;
struct pinctrl_state *i2c6_sda_low;
struct pinctrl_state *i2c6_sda_high;

struct wt6670f *_wt = NULL;

static int __wt6670f_write_word(struct wt6670f *wt, u8 reg, u16 data)
{
	s32 ret;

	ret = i2c_smbus_write_word_data(wt->client, reg, data);
	if (ret < 0) {
		pr_err("i2c write fail: can't write from reg 0x%02X\n", reg);
		return ret;
	}

	return 0;
}


static int __wt6670f_read_word(struct wt6670f *wt, u8 reg, u16 *data)
{
	s32 ret;

	ret = i2c_smbus_read_word_data(wt->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u16) ret;

	return 0;
}

static int wt6670f_read_word(struct wt6670f *wt, u8 reg, u16 *data)
{
	int ret;

	mutex_lock(&wt->i2c_rw_lock);
	ret = __wt6670f_read_word(wt, reg, data);
	mutex_unlock(&wt->i2c_rw_lock);

	return ret;
}

static int wt6670f_write_word(struct wt6670f *wt, u8 reg, u16 data)
{
	int ret;

	mutex_lock(&wt->i2c_rw_lock);
	ret = __wt6670f_write_word(wt, reg, data);
	mutex_unlock(&wt->i2c_rw_lock);

	return ret;
}

static u16 wt6670f_get_vbus_voltage(void)
{
	int ret;
	u8 data[2];
	u16 tmp;

	ret = wt6670f_read_word(_wt, 0xBE, (u16 *)data);
	if(ret < 0)
	{
		pr_err("wt6670f get vbus voltage fail\n");
		return ret;
	}

	tmp = ((data[0] << 8) | data[1]) & 0x3ff;

	pr_err(">>>>>>wt6670f get vbus voltage = %04x  %02x  %02x\n", tmp,
			data[0], data[1]);
	return (u16)(tmp * 18.98);
}

/*
static u16 wt6670f_get_id(void)
{
	int ret;
	u16 data;

	ret = wt6670f_read_word(_wt, 0xBC, &data);
	if(ret < 0)
	{
		pr_err("wt6670f get id fail\n");
		return ret;
	}

	pr_err(">>>>>>wt6670f get id = %x\n", data);
	return data;
}
*/

int wt6670f_start_detection(void)
{
	int ret;
	u16 data = 0x01;

	ret = wt6670f_write_word(_wt, 0xB6, data);
	if (ret < 0)
	{
		pr_info("wt6670f start detection fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_start_detection);

int wt6670f_re_run_apsd(void)
{
	int ret;
	u16 data = 0x01;

	ret = wt6670f_write_word(_wt, 0xB6, data);

	if (ret < 0)
	{
		pr_info("wt6670f re run apsd fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_re_run_apsd);

int wt6670f_get_protocol(void)
{
	int ret;
	u16 data;
        u8 data1, data2;

	ret = wt6670f_read_word(_wt, 0xBD, &data);
	if (ret < 0)
	{
		pr_err("wt6670f get protocol fail\n");
		return ret;
	}

        // Get data2 part
        data1 = data & 0xFF;
        data2 = data >> 8;

        pr_err("Get charger type, rowdata = 0X%04x, data1= 0X%02x, data2=0X%02x \n", data, data1, data2);
        if((data2 == 0x03) && ((data1 > 0x9) || (data1 == 0x7)))
        {
                pr_err("fail to get charger type, error happens!\n");
                return -EINVAL;
        }

        if(data2 == 0x04)
        {
                pr_err("detected QC3+ charger:0X%02x!\n", data1);
        }

	if((data1 > 0x00 && data1 < 0x07) ||
           (data1 > 0x07 && data1 < 0x0a)){
		ret = data1;
	}
	else {
		ret = 0x00;
	}

	_wt->chg_type = ret;
//	return data & 0xff;
	return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_get_protocol);

int wt6670f_get_charger_type(void)
{
        return _wt->chg_type;
}
EXPORT_SYMBOL_GPL(wt6670f_get_charger_type);

bool wt6670f_is_charger_ready(void)
{
	return _wt->chg_ready;
}
EXPORT_SYMBOL_GPL(wt6670f_is_charger_ready);

int wt6670f_get_firmware_version(void)
{
	int ret;
	u16 data;

	ret = wt6670f_read_word(_wt, 0xBF, &data);
	if (ret < 0)
	{
		pr_err("wt6670f get firmware fail\n");
		return ret;
	}

	return data & 0xff;
}
EXPORT_SYMBOL_GPL(wt6670f_get_firmware_version);


int wt6670f_set_voltage(u16 voltage)
{
	int ret;
	u16 step;
	u16 voltage_now;

	voltage_now = wt6670f_get_vbus_voltage();

/*  if(voltage == 5000)
  {
    step = _wt->count;
    step &= 0x7FFF;
	  step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
    _wt->count = 0;
  }
	else*/ if(voltage - voltage_now < 0)
	{
		step = (u16)((voltage_now - voltage) / 20);
    _wt->count -= step;
		step &= 0x7FFF;
		step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);

    if(_wt->count < 0)
        _wt->count = 0;
	}
	else
	{
		step = (u16)((voltage - voltage_now) / 20);
    _wt->count += step;
    step |= 0x8000;
    step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
	}
  pr_err("---->southchip count = %d   %04x\n", _wt->count, step);
	ret = wt6670f_write_word(_wt, 0xBB, step);

	return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_set_voltage);



static ssize_t wt6670f_show_test(struct device *dev,
						struct device_attribute *attr, char *buf)
{
//	struct wt6670f *wt = dev_get_drvdata(dev);

	int idx = 0;
	u8 data;

	data = wt6670f_get_protocol();
	idx = snprintf(buf, PAGE_SIZE, ">>> protocol = %02x\n", data);
	return idx;
}

static ssize_t wt6670f_store_test(struct device *dev,
								struct device_attribute *attr, const char *buf, size_t count)
{
//	struct wt6670f *wt = dev_get_drvdata(dev);

	u16 val;
	int ret;

	ret = sscanf(buf, "%d", &val);

	ret = wt6670f_set_voltage(val);

	return count;

}


static ssize_t wt6670f_show_registers(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	struct wt6670f *wt = dev_get_drvdata(dev);

	int idx = 0;
	int ret ;
	u16 data;

	ret = wt6670f_read_word(wt, 0xBD, &data);
	idx = snprintf(buf, PAGE_SIZE, ">>> reg[0xBD] = %04x\n", data);
	pr_err(">>>>>>>>>>test southchip  0xBD = %04x\n", data);
	return idx;
}

static ssize_t wt6670f_store_registers(struct device *dev,
								struct device_attribute *attr, const char *buf, size_t count)
{
	struct wt6670f *wt = dev_get_drvdata(dev);

	u16 val;
	int ret;

	ret = sscanf(buf, "%x", &val);

	ret = wt6670f_write_word(wt, 0xBB, val);

	return count;

}


static DEVICE_ATTR(test, 0660, wt6670f_show_test, wt6670f_store_test);
static DEVICE_ATTR(registers, 0660, wt6670f_show_registers, wt6670f_store_registers);

static void wt6670f_create_device_node(struct device *dev)
{
	device_create_file(dev, &dev_attr_registers);
  device_create_file(dev, &dev_attr_test);
}

int wt6670f_do_reset(void)
{
	gpio_direction_output(_wt->reset_pin,1);
	msleep(1);
	gpio_direction_output(_wt->reset_pin,0);
	msleep(10);

	return 0;
}
EXPORT_SYMBOL_GPL(wt6670f_do_reset);

void wt6670f_reset_chg_type(void)
{
        _wt->chg_type = 0;
	_wt->chg_ready = false;
}
EXPORT_SYMBOL_GPL(wt6670f_reset_chg_type);

static irqreturn_t wt6670f_intr_handler(int irq, void *data)
{
	pr_info("%s: read charger type!\n", __func__);
	_wt->chg_ready = true;

	return IRQ_HANDLED;
}

static int wt6670f_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

	pr_info("%s\n", __func__);
	if (!np) {
		pr_info("%s: no of node\n", __func__);
		return -ENODEV;
	}
#if 0
	wt6670f_reset_pin = of_get_named_gpio(np,"wt6670f-reset-gpio",0);
	if(wt6670f_reset_pin < 0){
	pr_info("%s: no reset\n", __func__);
	return -ENODATA;
	}
	else
	pr_info("%s: wt6670f-reset-gpio = %d\n", __func__,wt6670f_reset_pin);
	gpio_request(wt6670f_reset_pin,"wt6670f_reset");
	gpio_direction_output(wt6670f_reset_pin,0);
#endif

	wt6670f_int_pin = of_get_named_gpio(np,"wt6670f-int-gpio",0);
	if(wt6670f_int_pin < 0){
	pr_info("%s: no int\n", __func__);
	return -ENODATA;
	}
	else
	pr_info("%s: wt6670f-int-gpio = %d\n", __func__,wt6670f_int_pin);
	gpio_request(wt6670f_int_pin,"wt6670f_int");

	ret = request_irq(gpio_to_irq(wt6670f_int_pin), wt6670f_intr_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "wt6670f int", dev);
	enable_irq_wake(gpio_to_irq(wt6670f_int_pin));
	return 0;
}

extern int wt6670f_isp_flow(struct wt6670f *chip);

static int wt6670f_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	int ret = 0;
	u16 firmware_version = 0;
	struct wt6670f *wt;

	pr_info("[%s]\n", __func__);

	wt = devm_kzalloc(&client->dev, sizeof(struct wt6670f), GFP_KERNEL);
	if (!wt)
		return -ENOMEM;

	wt->dev = &client->dev;
	wt->client = client;
	i2c_set_clientdata(client, wt);
	mutex_init(&wt->i2c_rw_lock);

	wt->reset_pin = of_get_named_gpio(wt->dev->of_node, "wt6670f-reset-gpio", 0);
	if (wt->reset_pin < 0)
		pr_info("[%s] of get wt6670 gpio failed, reset_pin:%d\n",
			__func__, wt->reset_pin);

	if (gpio_request(wt->reset_pin, "wt6670_reset_pin"))
		pr_info("[%s] gpio_request reset failed", __func__);

	if (gpio_direction_output(wt->reset_pin, 0))
		pr_info("[%s] gpio_direction_output failed", __func__);

	i2c6_pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(i2c6_pinctrl)) {
		ret = PTR_ERR(i2c6_pinctrl);
		pr_info("fwq Cannot find i2c6_pinctrl!\n");
		return ret;
	}

	i2c6_scl_low = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_scl_low");
	if (IS_ERR(i2c6_scl_low)) {
		ret = PTR_ERR(i2c6_scl_low);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_scl_high = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_scl_high");
	if (IS_ERR(i2c6_scl_high)) {
		ret = PTR_ERR(i2c6_scl_high);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_sda_low = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_sda_low");
	if (IS_ERR(i2c6_sda_low)) {
		ret = PTR_ERR(i2c6_sda_low);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_sda_high = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c_sda_high");
	if (IS_ERR(i2c6_sda_high)) {
		ret = PTR_ERR(i2c6_sda_high);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	i2c6_i2c = pinctrl_lookup_state(i2c6_pinctrl, "wt6670_i2c");
	if (IS_ERR(i2c6_i2c)) {
		ret = PTR_ERR(i2c6_i2c);
		pr_info("Cannot find pinctrl state_i2c!\n");
		return ret;
	}

	_wt = wt;

	ret = wt6670f_parse_dt(&client->dev);
	if (ret < 0)
		pr_info("%s: parse dt error\n", __func__);

	wt6670f_create_device_node(&(client->dev));

	wt6670f_do_reset();
	firmware_version = wt6670f_get_firmware_version();
	wt6670f_reset_chg_type();
	pr_info("[%s] firmware_version = %d, chg_type = 0x%x\n", __func__,firmware_version, _wt->chg_type);

	if(firmware_version != WT6670_FIRMWARE_VERSION){
            pr_info("[%s]: firmware need upgrade, run wt6670_isp!", __func__);
            wt6670f_isp_flow(wt);
        }

	return 0;
}


//#define WT6670F_PM_OPS	(NULL)

static const struct i2c_device_id wt6670f_id_table[] = {
	{"wt6670f", 0},
	{},
};

static const struct of_device_id wt_match_table[] = {
	{.compatible = "mediatek,wt6670f_qc3p",},
	{},
};

static struct i2c_driver wt6670f_driver = {
	.driver = {
		.name = "wt6670f_qc3p",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = wt_match_table,
#endif
		//.pm = WT6670F_PM_OPS,
	},
	.probe = wt6670f_i2c_probe,
	//.remove = wt6670f_i2c_remove,
	//.shutdown = wt6670f_shutdown,
	.id_table = wt6670f_id_table,
};

static int __init wt6670f_init(void)
{
	pr_info("[%s] init start with i2c DTS", __func__);
	if (i2c_add_driver(&wt6670f_driver) != 0) {
		pr_info(
			"[%s] failed to register wt6670f i2c driver.\n",__func__);
	} else {
		pr_info(
			"[%s] Success to register wt6670f i2c driver.\n",__func__);
	}
	
	return 0;
	//return i2c_add_driver(&wt6670f_driver);
}

static void __exit wt6670f_exit(void)
{
	i2c_del_driver(&wt6670f_driver);
}
module_init(wt6670f_init);
module_exit(wt6670f_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lai Du <dulai@longcheer.com>");
MODULE_DESCRIPTION("WT6670F QC3P Driver");


