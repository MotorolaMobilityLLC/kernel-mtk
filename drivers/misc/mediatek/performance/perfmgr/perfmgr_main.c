#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/platform_device.h>
#include "perfmgr.h"

/*--------------------prototype--------------------*/

/*--------------------function--------------------*/

static int perfmgr_probe(struct platform_device *dev)
{
	return 0;
}

struct platform_device perfmgr_device = {
	.name   = "perfmgr",
	.id        = -1,
};

int perfmgr_suspend(struct device *dev)
{
#ifdef MTK_TOUCH_BOOST
	perfmgr_touch_suspend();
#endif
	return 0;
}

int perfmgr_resume(struct device *dev)
{
	return 0;
}

static struct platform_driver perfmgr_driver = {
	.probe      = perfmgr_probe,
	.driver     = {
		.name = "perfmgr",
		.pm = &(const struct dev_pm_ops){
			.suspend = perfmgr_suspend,
			.resume = perfmgr_resume,
		},
	},
};


/*--------------------INIT------------------------*/

static int __init init_perfmgr(void)
{
	struct proc_dir_entry *hps_dir = NULL;
	int ret = 0;

	ret = platform_device_register(&perfmgr_device);
	if (ret)
		return ret;
	ret = platform_driver_register(&perfmgr_driver);
	if (ret)
		return ret;


	hps_dir = proc_mkdir("perfmgr", NULL);
#ifdef MTK_TOUCH_BOOST
	init_perfmgr_touch();
#endif

	return 0;
}
device_initcall(init_perfmgr);

/*MODULE_LICENSE("GPL");*/
/*MODULE_AUTHOR("MTK");*/
/*MODULE_DESCRIPTION("The fliper function");*/
