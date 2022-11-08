#define LOG_TAG         "IOCTL"

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <linux/slab.h>
#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_ioctl.h"
#include "cts_strerror.h"

#define CTS_DRIVER_VERSION_CODE \
    ((CFG_CTS_DRIVER_MAJOR_VERSION << 16) | \
     (CFG_CTS_DRIVER_MINOR_VERSION << 8) | \
     (CFG_CTS_DRIVER_PATCH_VERSION << 0))

long cts_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct chipone_ts_data *cts_data;

    cts_info("ioctl, cmd=0x%08x, arg=0x%08lx", cmd, arg);

    cts_data = file->private_data;
    if (cts_data == NULL) {
        cts_err("IOCTL with private data = NULL");
        return -EFAULT;
    }

    switch (cmd) {
    case CTS_IOC_GET_DRIVER_VERSION:
        return put_user(CTS_DRIVER_VERSION_CODE,
                (unsigned int __user *)arg);
    default:
        cts_err("Unsupported ioctl cmd=0x%08x, arg=0x%08lx", cmd, arg);
        break;
    }

    return -ENOTSUPP;
}

#ifdef CONFIG_COMPAT
long cts_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct chipone_ts_data *cts_data;

    cts_info("ioctl, cmd=0x%08x, arg=0x%08lx", cmd, arg);

    cts_data = file->private_data;
    if (cts_data == NULL) {
        cts_err("IOCTL with private data = NULL");
        return -EFAULT;
    }

    switch (cmd) {
    case CTS_IOC_GET_DRIVER_VERSION:
        return put_user(CTS_DRIVER_VERSION_CODE,
                (unsigned int __user *)arg);
    default:
        cts_err("Unsupported ioctl cmd=0x%08x, arg=0x%08lx", cmd, arg);
        break;
    }

    return -ENOTSUPP;

}
#endif /* CONFIG_COMPAT */

