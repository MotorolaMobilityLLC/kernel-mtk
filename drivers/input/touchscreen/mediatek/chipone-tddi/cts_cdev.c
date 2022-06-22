#define LOG_TAG         "CDev"

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_ioctl.h"
#include "cts_strerror.h"

#ifdef CONFIG_CTS_CDEV
struct cts_cdev_data {
    dev_t devid;
    struct class *class;
    struct device *device;
    struct cdev cdev;
    bool cdev_added;
    struct chipone_ts_data *cts_data;
};

static int cts_dev_open(struct inode *inode, struct file *file)
{
    struct cts_cdev_data *cdev_data = NULL;

    cdev_data = container_of(inode->i_cdev, struct cts_cdev_data, cdev);
    file->private_data = cdev_data->cts_data;
    return 0;
}

static int cts_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static const struct file_operations cts_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = no_llseek,
    .unlocked_ioctl = cts_unlocked_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = cts_compat_ioctl,
#endif /* CONFIG_COMPAT */
    .open       = cts_dev_open,
    .release    = cts_dev_release,
};

int cts_init_cdev(struct chipone_ts_data *cts_data)
{
    struct cts_cdev_data *cdev_data;
    int ret;

    if (cts_data == NULL) {
        cts_err("Init detect with cts_data = NULL");
        return -EFAULT;
    }

    cdev_data = kzalloc(sizeof(*cdev_data), GFP_KERNEL);
    if (cdev_data == NULL) {
        cts_err("Alloc cts_cdev_data failed");
        return -ENOMEM;
    }

    cts_info("Init");

    ret = alloc_chrdev_region(&cdev_data->devid, 0, 1, CFG_CTS_DEVICE_NAME);
    if (ret) {
        cdev_data->devid = 0;
        cts_err("Alloc char dev region failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

    cts_info("Allocated cdev major: %u, minor: %u",
        MAJOR(cdev_data->devid), MINOR(cdev_data->devid));

    cdev_init(&cdev_data->cdev, &cts_dev_fops);
    ret = cdev_add(&cdev_data->cdev, cdev_data->devid, 1);
    if (ret) {
        cts_err("Add char dev failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_unregister_chrdev_region;
    }
    cdev_data->cdev_added = true;

    cdev_data->class = class_create(THIS_MODULE, CFG_CTS_DRIVER_NAME);
    if (IS_ERR(cdev_data->class)) {
        ret = PTR_ERR(cdev_data->class);
        cdev_data->class = NULL;
        cts_err("Create class '"CFG_CTS_DRIVER_NAME"'failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_del_cdev;
    }

    cdev_data->device = device_create(cdev_data->class,
        NULL, cdev_data->devid, cts_data->device, "chipone_touch");
    if (IS_ERR(cdev_data->device)) {
        ret = PTR_ERR(cdev_data->device);
        cdev_data->device = NULL;
        cts_err("Create device 'chipone_touch' failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_destroy_class;
    }

    cts_data->cdev_data = cdev_data;
    cdev_data->cts_data = cts_data;

    return 0;

err_destroy_class:
    class_destroy(cdev_data->class);
    cdev_data->class = NULL;
err_del_cdev:
    cdev_del(&cdev_data->cdev);
    cdev_data->cdev_added = false;
err_unregister_chrdev_region:
    unregister_chrdev_region(cdev_data->devid, 1);
    cdev_data->devid = 0;

    return ret;
}

int cts_deinit_cdev(struct chipone_ts_data *cts_data)
{
    struct cts_cdev_data *cdev_data;

    if (cts_data == NULL) {
        cts_err("Deinit with cts_data = NULL");
        return -EFAULT;
    }

    cdev_data = cts_data->cdev_data;
    if (cdev_data == NULL) {
        cts_warn("Deinit with cdev_data = NULL");
        return 0;
    }

    cts_info("Deinit");

    if (cdev_data->cdev_added) {
        cdev_del(&cdev_data->cdev);
        cdev_data->cdev_added = false;
    }

    if (cdev_data->device && cdev_data->devid) {
        device_destroy(cdev_data->class, cdev_data->devid);
        cdev_data->device = NULL;
    }

    if (cdev_data->class) {
        class_destroy(cdev_data->class);
        cdev_data->class = NULL;
    }

    if (cdev_data->devid) {
        unregister_chrdev_region(cdev_data->devid, 1);
        cdev_data->devid = 0;
    }

    kfree(cdev_data);
    cts_data->cdev_data = NULL;

    return 0;
}
#endif /* CONFIG_CTS_CDEV */

