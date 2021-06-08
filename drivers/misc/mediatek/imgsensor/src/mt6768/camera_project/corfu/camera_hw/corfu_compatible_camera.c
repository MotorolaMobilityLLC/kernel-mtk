#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "imgsensor_cfg_table.h"

static int Corf_Compatible_Camera_probe(struct platform_device *pdev)
{
	struct device_node *np;
	const char *camera_board;
	np = (pdev->dev).of_node;
	if (of_property_read_string(np, "compatible_camera", &camera_board) < 0) {
		printk("%s: no camera board name find\n");
		return -1;
	}
	if(!strncmp(camera_board,"evt",3))
	{
		(((imgsensor_custom_config[0]).pwr_info)[3]).id = IMGSENSOR_HW_ID_REGULATOR;
		(((imgsensor_custom_config[0]).pwr_info)[3]).pin = IMGSENSOR_HW_PIN_AVDD;

		(((sensor_power_sequence[0]).pwr_info)[3]).pin = AVDD;
		(((sensor_power_sequence[0]).pwr_info)[3]).pin_state_on = Vol_2800;

		(((imgsensor_custom_config[1]).pwr_info)[6]).id = IMGSENSOR_HW_ID_GPIO;
		(((imgsensor_custom_config[1]).pwr_info)[6]).pin = IMGSENSOR_HW_PIN_AFVDD;
		(((imgsensor_custom_config[1]).pwr_info)[7]).id = IMGSENSOR_HW_ID_NONE;
		(((imgsensor_custom_config[1]).pwr_info)[7]).pin = IMGSENSOR_HW_PIN_NONE;

		(((sensor_power_sequence[1]).pwr_info)[4]).pin = AFVDD;
		(((sensor_power_sequence[1]).pwr_info)[4]).pin_state_on = Vol_High;
		(((sensor_power_sequence[1]).pwr_info)[5]).pin = DVDD;
		(((sensor_power_sequence[1]).pwr_info)[5]).pin_state_on = Vol_1100;
		(((sensor_power_sequence[1]).pwr_info)[5]).pin_on_delay = 1;
		(((sensor_power_sequence[1]).pwr_info)[6]).pin = RST;
		(((sensor_power_sequence[1]).pwr_info)[6]).pin_state_on = Vol_High;
		(((sensor_power_sequence[1]).pwr_info)[6]).pin_on_delay = 5;
	}
	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id compatible_of_device_id[] = {
	{ .compatible = "mediatek,compatible_camera", },
	{}
};
#endif

static struct platform_driver Corf_Compatible_Camera_driver = {
	.probe      = Corf_Compatible_Camera_probe,
	.driver     = {
		.name   = "Compatible_Camera",
		.owner  = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = compatible_of_device_id,
#endif
	}
};

static int __init Corf_Compatible_Camera_init(void)
{
	if (platform_driver_register(&Corf_Compatible_Camera_driver)) {
		printk("Failed to register Corf_Compatible_Camera driver\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit Corf_Compatible_Camera_exit(void)
{
	platform_driver_unregister(&Corf_Compatible_Camera_driver);
}

fs_initcall(Corf_Compatible_Camera_init);
module_exit(Corf_Compatible_Camera_exit);

MODULE_DESCRIPTION("Corf Compatible Camera");
MODULE_AUTHOR("lugang@longcheer.com>");
MODULE_LICENSE("GPL");

