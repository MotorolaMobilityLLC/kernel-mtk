
#ifndef _UAPI_LINUX_FPSGO_H
#define _UAPI_LINUX_FPSGO_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define	FPSGO_QUEUE						_IOW('g', 1, __u32)
#define	FPSGO_DEQUEUE					_IOW('g', 3, __u32)
#define	FPSGO_VSYNC						_IOW('g', 5, __u32)
#define	FPSGO_QUEUE_LENGTH		_IOW('g', 6, __u32)
#define	FPSGO_ACQUIRE_BUFFER	_IOW('g', 7, __u32)

#define IOCTL_WRITE_AS _IOW('g', 8, int)
#define IOCTL_WRITE_GM _IOW('g', 9, int)
#define IOCTL_WRITE_TH _IOW('g', 10, int)
#define IOCTL_WRITE_FC _IOW('g', 11, int)
#define IOCTL_WRITE_IV _IOW('g', 12, int)
#define IOCTL_WRITE_NR _IOW('g', 13, int)
#define IOCTL_WRITE_SB _IOW('g', 14, int)

#endif
