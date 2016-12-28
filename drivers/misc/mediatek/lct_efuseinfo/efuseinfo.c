#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/types.h>	/* size_t */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/proc_fs.h>  /*proc*/
#include <linux/genhd.h>    //special
#include <linux/cdev.h>
#include <asm/uaccess.h>   /*set_fs get_fs mm_segment_t*/


static struct class  *efusectrl_class;
static struct device *efusectrl_dev;

static unsigned int efuse_status = 0xff;

static int get_efuse_status(void)
{
	char *ptr;
	ptr = strstr(saved_command_line,"efuse_status=");
	if(ptr == NULL)
	{
	  return -1;
	}
	ptr += strlen("efuse_status=");
	efuse_status= simple_strtol(ptr,NULL,10);
	
    printk("[DEVINFO_EMCP][%s]: Get boot ram list num form comandline.[%d]\n", __func__,efuse_status);
	return efuse_status;
}

static ssize_t efusestatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int efuse_status = 0;
	efuse_status = get_efuse_status();
	return snprintf(buf, PAGE_SIZE, "%d\n",efuse_status );
	
}

static DEVICE_ATTR(efusestatus, S_IRUGO, efusestatus_show, 	NULL);
static struct device_attribute *dev_attr_efuse[] = {
	&dev_attr_efusestatus,
	NULL
};

static int __init efuseinfo_localinit(void)
{
	int err = 0;
    efusectrl_class = class_create(THIS_MODULE, "efusectrl-cls");

    if(IS_ERR(efusectrl_class))
    {
        printk(KERN_ERR "Failed to create class(efusectrl)!\n");
		return -1;
    }

    efusectrl_dev = device_create(efusectrl_class, NULL, 0, NULL, "efusectrl-dev");

    if(IS_ERR(efusectrl_dev))
    {
        printk(KERN_ERR "Failed to create device(efusectrl_dev)!\n");
		class_destroy(efusectrl_class);
		return -1;
    }

   err = device_create_file(efusectrl_dev, dev_attr_efuse[0]);
     if (err)  
    {
      //  printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_efuse.attr.name);
		device_destroy(efusectrl_class, 0);
		class_destroy(efusectrl_class);
		return -1;
    }


    return 0;  
}

static int __init efuseinfo_init(void)
{

	efuseinfo_localinit();
    return 0;
}

static void __exit efuseinfo_exit(void)
{

}

module_init(efuseinfo_init);
//late_initcall(efuseinfo_init);
module_exit(efuseinfo_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Efuseinfo Management Driver");
MODULE_AUTHOR("wangxiqiang <wangxiqiang@longcheer.net>");
