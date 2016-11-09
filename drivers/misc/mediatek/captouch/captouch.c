
#include "inc/captouch.h"
//++ Modified for close Captouch by shentaotao 2016.07.02
#if defined(CONFIG_MTK_SX9311)
#include <linux/fb.h>
static struct notifier_block captouch_fb_notifier;
extern void SX9311_suspend(void);
extern void SX9311_resume(void);
#endif

/*LCT cly add cap sensor */
#define CAPTOUCH_MISC_DEV_NAME          "m_capsensor_misc"
#define CAPTOUCH_INPUTDEV_NAME          "m_capsensor_input"

//-- Modified for close Captouch by shentaotao 2016.07.02
struct captouch_context *captouch_context_obj = NULL;
static struct platform_device *pltfm_dev;

static struct captouch_init_info* captouch_init_list[MAX_CHOOSE_CAPTOUCH_NUM]= {0}; //modified

static bool captouch_misc_dev_init;

int captouch_data_report(struct input_dev *dev, int value,int status)
{
	CAPTOUCH_LOG("+captouch_data_report! %d, %d\n",value,status);
	input_report_rel(dev, EVENT_TYPE_CAPTOUCH_VALUE, (value+1));
	input_report_rel(dev, EVENT_TYPE_CAPTOUCH_STATUS, status);
	input_sync(dev); 
	return 0;
}

static struct captouch_context *captouch_context_alloc_object(void)
{
	
	struct captouch_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL); 
    	CAPTOUCH_LOG("captouch_context_alloc_object++++\n");
	if(!obj)
	{
		CAPTOUCH_ERR("Alloc captouch object error!\n");
		return NULL;
	}	
	
	CAPTOUCH_LOG("captouch_context_alloc_object----\n");
	return obj;
}


static ssize_t cap_show_active(struct device* dev, struct device_attribute *attr, char *buf) 
{
	return 1;
}

static int cap_touch_remove(struct platform_device *pdev)
{
	CAPTOUCH_LOG("cap_touch_remove\n");
	return 0;
}

static int cap_touch_probe(struct platform_device *pdev) 
{
	CAPTOUCH_LOG("cap_touch_probe\n");
	pltfm_dev = pdev;
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cap_touch_of_match[] = {
	{ .compatible = "mediatek,cap_touch", },
	{},
};
#endif

static struct platform_driver cap_touch_driver = {
	.probe      = cap_touch_probe,
	.remove     = cap_touch_remove,    
	.driver     = 
	{
		.name  = "cap_touch",
        #ifdef CONFIG_OF
		.of_match_table = cap_touch_of_match,
		#endif
	}
};

static int captouch_real_driver_init(void) 
{
	int i = 0;
	int err = 0;

	CAPTOUCH_LOG(" captouch_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_CAPTOUCH_NUM; i++) {
		CAPTOUCH_LOG("captouch_real_driver_init i=%d\n", i);
		if (0 != captouch_init_list[i]) {
			CAPTOUCH_LOG(" captouch try to init driver %s\n", captouch_init_list[i]->name);
			err = captouch_init_list[i]->init();
			if (0 == err) {
				CAPTOUCH_LOG(" captouch real driver %s probe ok\n", captouch_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_CAPTOUCH_NUM) {
		CAPTOUCH_LOG(" captouch_real_driver_init fail\n");
		err =  -1;
	}

	return err;
}

  int captouch_driver_add(struct captouch_init_info* obj) 
{
	int err = 0;
	int i = 0;

	CAPTOUCH_FUN();
	if (!obj) {
		CAPTOUCH_ERR("CAPTOUCH driver add fail, captouch_init_info is NULL\n");
		return -1;
	}

	for (i = 0; i < MAX_CHOOSE_CAPTOUCH_NUM; i++) {
		if ((i == 0) && (NULL == captouch_init_list[0])) {
			CAPTOUCH_LOG("register captouch driver for the first time\n");
			if (platform_driver_register(&cap_touch_driver))
				CAPTOUCH_ERR("failed to register captouch driver already exist\n");
		}

		if (NULL == captouch_init_list[i]) {
			obj->platform_diver_addr = &cap_touch_driver;
			captouch_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_CAPTOUCH_NUM) {
		CAPTOUCH_ERR("CAPTOUCH driver add err\n");
		err =  -1;
	}

	return err;
}
EXPORT_SYMBOL_GPL(captouch_driver_add);
struct platform_device *get_captouch_platformdev(void)
{
	return pltfm_dev;
}

int captouch_report_interrupt_data(int value) 
{
	struct captouch_context *cxt = NULL;
	//int err =0;
	cxt = captouch_context_obj;	

	captouch_data_report(cxt->idev,value,3);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(captouch_report_interrupt_data);

static int captouch_misc_init(struct captouch_context *cxt)
{

    int err=0;
    cxt->mdev.minor = MISC_DYNAMIC_MINOR;
	cxt->mdev.name  = CAPTOUCH_MISC_DEV_NAME;
	if((err = misc_register(&cxt->mdev)))
	{
		CAPTOUCH_ERR("unable to register captouch misc device!!\n");
	}
	return err;
}

static int captouch_input_init(struct captouch_context *cxt)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (NULL == dev)
		return -ENOMEM;

	dev->name = CAPTOUCH_INPUTDEV_NAME;

	set_bit(EV_REL, dev->evbit);
	set_bit(EV_SYN, dev->evbit);
	input_set_capability(dev, EV_REL, EVENT_TYPE_CAPTOUCH_VALUE);
	input_set_capability(dev, EV_REL, EVENT_TYPE_CAPTOUCH_STATUS);
	
	input_set_drvdata(dev, cxt);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	cxt->idev= dev;

	return 0;
}

DEVICE_ATTR(capactive,     S_IWUSR | S_IRUGO, cap_show_active, NULL);


static struct attribute *captouch_attributes[] = {
	&dev_attr_capactive.attr,
	NULL
};

static struct attribute_group captouch_attribute_group = {
	.attrs = captouch_attributes
};

int captouch_register_control_path(struct captouch_control_path *ctl)
{
	struct captouch_context *cxt = NULL;
	int err = 0;

	cxt = captouch_context_obj;
	cxt->captouch_ctl.set_delay = ctl->set_delay;
	cxt->captouch_ctl.open_report_data = ctl->open_report_data;
	cxt->captouch_ctl.enable_nodata = ctl->enable_nodata;
	cxt->captouch_ctl.is_support_batch = ctl->is_support_batch;
	cxt->captouch_ctl.is_report_input_direct = ctl->is_report_input_direct;
	cxt->captouch_ctl.cap_calibration = ctl->cap_calibration;
	cxt->captouch_ctl.cap_threshold_setting = ctl->cap_threshold_setting;
	cxt->captouch_ctl.is_use_common_factory = ctl->is_use_common_factory;
	cxt->captouch_ctl.is_polling_mode = ctl->is_polling_mode;

	if (!captouch_misc_dev_init) {
		/* add misc dev for sensor hal control cmd */
		err = captouch_misc_init(captouch_context_obj);
		if (err) {
			CAPTOUCH_ERR("unable to register captouch misc device!!\n");
			return -2;
		}
		err = sysfs_create_group(&captouch_context_obj->mdev.this_device->kobj,
				&captouch_attribute_group);
		if (err < 0) {
			CAPTOUCH_ERR("unable to create captouch attribute file\n");
			return -3;
		}
		kobject_uevent(&captouch_context_obj->mdev.this_device->kobj, KOBJ_ADD);
		captouch_misc_dev_init = true;
	}
	return 0;
}

//++ Modified for close Captouch by shentaotao 2016.07.02
#if defined(CONFIG_MTK_SX9311)
static int captouch_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = NULL;
	int blank;

	CAPTOUCH_LOG("captouch_fb_notifier_callback\n");

	evdata = data;
	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	switch (blank) {
	case FB_BLANK_UNBLANK:
		CAPTOUCH_LOG("LCD ON Notify\n");
		SX9311_resume();
		break;
	case FB_BLANK_POWERDOWN:
		CAPTOUCH_LOG("LCD OFF Notify\n");
		SX9311_suspend();
		break;
	default:
		break;
	}
	return 0;
}
#endif
//-- Modified for close Captouch by shentaotao 2016.07.02

static int captouch_probe(void) 
{
	int err;

	CAPTOUCH_LOG("+++++++++++++captouch_probe!!\n");

	captouch_context_obj = captouch_context_alloc_object();
	if (!captouch_context_obj)
	{
		err = -ENOMEM;
		CAPTOUCH_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}

	//init input dev
	err = captouch_input_init(captouch_context_obj);
	if(err)
	{
		CAPTOUCH_ERR("unable to register captouch input device!\n");
		goto exit_alloc_input_dev_failed;
	}

	//init real alspseleration driver
	err = captouch_real_driver_init();
	if(err)
	{
		CAPTOUCH_ERR("captouch real driver init fail\n");
		goto real_driver_init_fail;
	}
  
	//++ Modified for close Captouch by shentaotao 2016.07.02
	#if defined(CONFIG_MTK_SX9311)
	captouch_fb_notifier.notifier_call = captouch_fb_notifier_callback;
	if (fb_register_client(&captouch_fb_notifier))
		CAPTOUCH_ERR("register captouch_fb_notifier fail!\n");
	#endif
	//-- Modified for close Captouch by shentaotao 2016.07.02

	CAPTOUCH_LOG("----captouch_probe OK !!\n");
	return 0;

real_driver_init_fail:
exit_alloc_input_dev_failed:
	kfree(captouch_context_obj);
	captouch_context_obj = NULL;
exit_alloc_data_failed:
	CAPTOUCH_ERR("----captouch_probe fail !!!\n");
	return err;
}

static int captouch_remove(void)
{
	int err=0;
	CAPTOUCH_FUN(f);
	input_unregister_device(captouch_context_obj->idev);        
	
	if((err = misc_deregister(&captouch_context_obj->mdev)))
	{
		CAPTOUCH_ERR("misc_deregister fail: %d\n", err);
	}
	kfree(captouch_context_obj);

	return 0;
}

static int __init captouch_init(void) 
{
	CAPTOUCH_FUN();

	if (captouch_probe()) {
		CAPTOUCH_ERR("failed to register captouch driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit captouch_exit(void)
{
	captouch_remove();
	platform_driver_unregister(&cap_touch_driver);

}
late_initcall(captouch_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CAPTOUCH device driver");
MODULE_AUTHOR("Mediatek");

