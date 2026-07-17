/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Wrapfd UAPI
 *
 * Copyright (C) 2025 Google, Inc.
 */

#ifndef _UAPI_WRAPFD_H
#define _UAPI_WRAPFD_H

#include <linux/types.h>

struct wrapfd_wrap {
	__u32 fd;		/* [in] file to wrap */
	__u32 prot;		/* [in] protection bits */
	__u64 reserved;
};

#define WRAPFD_CONTENT_EMPTY	0
#define WRAPFD_CONTENT_RDONLY	1
#define WRAPFD_CONTENT_RDWR	2

struct wrapfd_get_state {
	__u32 state;		/* [out] wrapfd content state */
	__u32 pad;
	__u64 reserved;
};

struct wrapfd_load {
	__u32 fd;		/* [in] file to load */
	__u32 pad;
	__u64 file_offs;	/* [in] file offset */
	__u64 buf_offs;		/* [in] buffer offset */
	__u64 len;		/* [in] number of bytes to load */
	__u64 reserved;
};

struct wrapfd_rewrap {
	__u32 prot;		/* [in] protection bits */
	__u32 pad;
	__u64 reserved;
};

/* ioctls for /dev/wrapfd */
#define WRAPFD_DEV_IOC 0xBC
#define WRAPFD_DEV_IOC_WRAP	_IOW(WRAPFD_DEV_IOC, 0, struct wrapfd_wrap)

/* ioctl for wrapfd */
#define WRAPFD_DEV_IOC_GET_STATE	_IOWR(WRAPFD_DEV_IOC, 1, \
					      struct wrapfd_get_state)
#define WRAPFD_DEV_IOC_ACQUIRE_OWNERSHIP	_IO(WRAPFD_DEV_IOC, 2)
#define WRAPFD_DEV_IOC_RELEASE_OWNERSHIP	_IO(WRAPFD_DEV_IOC, 3)
#define WRAPFD_DEV_IOC_LOAD	_IOW(WRAPFD_DEV_IOC, 4, struct wrapfd_load)
#define WRAPFD_DEV_IOC_REWRAP	_IOW(WRAPFD_DEV_IOC, 5, struct wrapfd_rewrap)
#define WRAPFD_DEV_IOC_EMPTY	_IO(WRAPFD_DEV_IOC, 6)
#define WRAPFD_DEV_IOC_ALLOW_GUESTS	_IO(WRAPFD_DEV_IOC, 7)
#define WRAPFD_DEV_IOC_PROHIBIT_GUESTS	_IO(WRAPFD_DEV_IOC, 8)

#endif /* _UAPI_WRAPFD_H */
