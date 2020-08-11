#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

static u32 usbcable_eint_num = 0;
static u32 usbcable_gpio_num = 0,usbcable_gpio_deb = 0;
static int last_eint_level = 0;
static struct input_dev *cable_input_dev;

static int usbcable_probe(struct platform_device *pdev);

static const struct of_device_id usbcable_of_match[] = {
	{ .compatible = "mediatek,usb_cable_eint", },
	{},
};

MODULE_DEVICE_TABLE(of, usbirq_dt_match);

static struct platform_driver usbcable_driver = {
	.probe = usbcable_probe,
	.driver = {
		.name = "Usbcable_driver",
		.owner	= THIS_MODULE,
		.of_match_table = usbcable_of_match,
	},
};
//module_platform_driver(usbcable_driver);/*USB CABLE GPIO EINT19*/

unsigned int get_rf_gpio_value(void)
{
	return last_eint_level;
}
EXPORT_SYMBOL(get_rf_gpio_value);

 static irqreturn_t usbcable_irq_handler(int irq, void *dev)
 {
	int level = 0;
	//printk("[USBCABLE]%s enter\n",__func__);

	level = __gpio_get_value(usbcable_gpio_num);
	//printk("[USBCABLE] gpio_value=%d,last_value=%d\n",level,last_eint_level);
	if(level!=last_eint_level){
		printk("[USBCABLE]report key [%d]\n",level);
		input_report_key(cable_input_dev, KEY_TWEN, level);
 		input_sync(cable_input_dev);
		last_eint_level = level;
 	}
	//printk("[USBCABLE]%s finished\n",__func__);
	return IRQ_HANDLED;
}

static int usbcable_probe(struct platform_device *pdev){
	int ret = 0;
	struct device_node *node = NULL;
	struct pinctrl_state *pins_default = NULL;
	static struct pinctrl *usbcable_pinctrl;
	printk("[USBCABLE]usbcable_gpio_probe Enter\n");

	usbcable_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(usbcable_pinctrl)) {
		ret = PTR_ERR(usbcable_pinctrl);
		dev_notice(&pdev->dev, "get usbcable_pinctrl fail.\n");
		return ret;
	}

	pins_default = pinctrl_lookup_state(usbcable_pinctrl, "default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		dev_notice(&pdev->dev, "lookup default pinctrl fail\n");
		return ret;
	}

	pinctrl_select_state(usbcable_pinctrl, pins_default);

	node = of_find_matching_node(NULL, usbcable_of_match);
	if (!node) {
		printk("%s can't find compatible node\n", __func__);
		return -1;
	}

	usbcable_gpio_num = of_get_named_gpio(node, "usbcable-gpio", 0);
	ret = of_property_read_u32(node, "debounce", &usbcable_gpio_deb);
	if (ret < 0) {
		pr_notice("[USBCABLE] %s gpiodebounce not found,ret:%d\n",__func__, ret);
		return ret;
	}
	gpio_set_debounce(usbcable_gpio_num, usbcable_gpio_deb);
	printk("[USBCABLE]usbcable_gpio_num<%d>debounce<%d>,\n", usbcable_gpio_num,usbcable_gpio_deb);

	cable_input_dev = input_allocate_device();
	if(!cable_input_dev)
		return -1;
	cable_input_dev->name = "USBCABLE";
	cable_input_dev->id.bustype = BUS_HOST;

	__set_bit(EV_KEY, cable_input_dev->evbit);
	__set_bit(KEY_TWEN, cable_input_dev->keybit);

	ret = input_register_device(cable_input_dev);
	if (ret){
		input_free_device(cable_input_dev);
		printk("%s input_register_device fail, ret:%d.\n", __func__,ret);
		return -1;
	}

	usbcable_eint_num = irq_of_parse_and_map(node, 0);
	printk("[USBCABLE]usbcable_eint_num<%d>\n",usbcable_eint_num);
	ret = request_irq(usbcable_eint_num, usbcable_irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING , "USB_CABLE", NULL);
	if (ret) {
		printk("%s request_irq fail, ret:%d.\n", __func__,ret);
		return ret;
	}
	//enable_irq_wake(usbcable_eint_num);

	ret = __gpio_get_value(usbcable_gpio_num);
	if(ret!=last_eint_level){
		printk("[USBCABLE]probe report key [%d]\n",ret);
		input_report_key(cable_input_dev, KEY_TWEN, ret);
		input_sync(cable_input_dev);
		last_eint_level = ret;
	}

	return 0;
}

static int __init usbcable_mod_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&usbcable_driver);
 	if (ret)
 		printk("usbcable platform_driver_register error:(%d)\n", ret);
 	printk("%s() done!3\n", __func__);
 	return ret;
}

static void __exit usbcable_mod_exit(void)
{
 	printk("%s()\n", __func__);
	platform_driver_unregister(&usbcable_driver);
}

module_init(usbcable_mod_init);
module_exit(usbcable_mod_exit);

MODULE_AUTHOR("Sha Bei<bei.sha@ontim.cn>");
MODULE_DESCRIPTION("usb cable gpio irq driver");
MODULE_LICENSE("GPL");

