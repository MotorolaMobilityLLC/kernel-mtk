#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <video/mmp_disp.h>
#include <linux/delay.h>


static struct kobject *bootinfo_kobj = NULL;
int gesture_dubbleclick_en =0;

static ssize_t gesture_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	return (s - buf);
}

static ssize_t gesture_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)

{
    int enable=0,ret;
    ret = kstrtouint(buf, 10, &enable);
	if(ret != 0)
		return 0;
    gesture_dubbleclick_en =enable;

	return n;
}
static struct kobj_attribute gesture_enable_attr = {
	.attr = {
		.name = "gesture_enable",
		.mode = 0644,
	},
	.show =&gesture_enable_show,
	.store= &gesture_enable_store,
};


static ssize_t i2c_devices_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	return (s - buf);
}
static struct kobj_attribute i2c_devices_info_attr = {
	.attr = {
		.name = "i2c_devices_probe_info",
		.mode = 0444,
	},
	.show =&i2c_devices_info_show,
};

static struct attribute * g[] = {
	&i2c_devices_info_attr.attr,//+add by liuwei
	&gesture_enable_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static int __init bootinfo_init(void)
{
	int ret = -ENOMEM;

	//printk("%s,line=%d\n",__func__,__LINE__);

	bootinfo_kobj = kobject_create_and_add("ontim_bootinfo", NULL);

	if (bootinfo_kobj == NULL) {
		printk("bootinfo_init: kobject_create_and_add failed\n");
		goto fail;
	}

	ret = sysfs_create_group(bootinfo_kobj, &attr_group);
	if (ret) {
		printk("bootinfo_init: sysfs_create_group failed\n");
		goto sys_fail;
	}

	return ret;
sys_fail:
	kobject_del(bootinfo_kobj);
fail:
	return ret;

}

static void __exit bootinfo_exit(void)
{

	if (bootinfo_kobj) {
		sysfs_remove_group(bootinfo_kobj, &attr_group);
		kobject_del(bootinfo_kobj);
	}
}

arch_initcall(bootinfo_init);
module_exit(bootinfo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Boot information collector");
