#ifndef CTS_IOCTL_H
#define CTS_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define CTS_IOC_GET_DRIVER_VERSION  _IOR('C', 0x00, u32)

long cts_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
long cts_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif /* CONFIG_COMPAT */

#endif /* CTS_IOCTL_H */

