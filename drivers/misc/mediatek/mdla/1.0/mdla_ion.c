/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>
#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#include "mdla_ioctl.h"
#include "mdla_ion.h"
#include "mdla_debug.h"

static struct ion_client *ion_client;

void mdla_ion_init(void)
{
	ion_client = ion_client_create(g_ion_device, "mdla");

	// TODO: remove debug
	mdla_debug("%s: ion_client_create(): %p\n", __func__, ion_client);
}

void mdla_ion_exit(void)
{
	if (ion_client) {
		// TODO: remove debug
		mdla_debug("%s: %p\n", __func__, ion_client);
		ion_client_destroy(ion_client);
		ion_client = NULL;
	}
}

int mdla_ion_kmap(unsigned long arg)
{
	struct ioctl_ion ion_data;
	struct ion_handle *hndl;
	ion_phys_addr_t addr;
	void *kva;
	int ret;

	if (!ion_client)
		return -ENOENT;

	if (copy_from_user(&ion_data, (void * __user) arg, sizeof(ion_data)))
		return -EFAULT;

	// TODO: remove debug
	mdla_debug("%s: share fd: %d\n", __func__, ion_data.fd);

	hndl = ion_import_dma_buf_fd(ion_client, ion_data.fd);

	if (IS_ERR_OR_NULL(hndl)) {
		// TODO: remove debug
		mdla_debug("%s: ion_import_dma_buf_fd(): failed: %d\n",
			__func__, PTR_ERR(hndl));
		return -EINVAL;
	}

	// TODO: remove debug
	mdla_debug("%s: ion_import_dma_buf_fd(): %p\n", __func__, hndl);

	/*  map to to kernel virtual address */
	kva = ion_map_kernel(ion_client, hndl);
	if (IS_ERR_OR_NULL(kva)) {
		mdla_debug("%s: ion_map_kernel(%p, %p): %d\n",
			__func__, ion_client, hndl, ret); // TODO: remove debug
		return -EINVAL;
	}

	/*  Get the phyiscal address (mva) to the buffer,
	 *  mdla_run_command_async() will translate mva to kva again,
	 *  via phys_to_virt()
	 */
	ret = ion_phys(ion_client, hndl, &addr, &ion_data.len);
	if (ret < 0) {
		// TODO: remove debug
		mdla_debug("%s: ion_phys(%p, %p): %d\n",
			__func__, ion_client, hndl, ret);

		return -EINVAL;
	}

	ion_data.kva = (__u64) kva;
	ion_data.mva = addr;
	ion_data.khandle = (__u64) hndl;

	if (copy_to_user((void * __user) arg, &ion_data, sizeof(ion_data)))
		return -EFAULT;

	// TODO: remove debug
	mdla_debug("%s: mva=%llxh, kva=%llxh, kernel_handle=%llxh\n",
		__func__, ion_data.mva, ion_data.kva, ion_data.khandle);


	return 0;
}

int mdla_ion_kunmap(unsigned long arg)
{
	struct ioctl_ion ion_data;
	struct ion_handle *hndl;

	if (!ion_client)
		return -ENOENT;

	if (copy_from_user(&ion_data, (void * __user) arg, sizeof(ion_data)))
		return -EFAULT;

	// TODO: remove debug
	mdla_debug("%s: mva=%llxh, kva=%llxh, kernel_handle=%llxh\n",
		__func__, ion_data.mva, ion_data.kva, ion_data.khandle);

	hndl = (struct ion_handle *)ion_data.khandle;

	ion_unmap_kernel(ion_client, hndl);
	ion_free(ion_client, hndl);

	return 0;
}

void mdla_ion_sync(u64 hndl)
{
	struct ion_sys_data sys_data;
	int ret;

	sys_data.sys_cmd = ION_SYS_CACHE_SYNC;
	sys_data.cache_sync_param.kernel_handle =
		(struct ion_handle *)hndl;
	sys_data.cache_sync_param.sync_type = ION_CACHE_FLUSH_BY_RANGE;

	ret = ion_kernel_ioctl(ion_client, ION_CMD_SYSTEM,
		(unsigned long)&sys_data);

	// TODO: remove debug
	mdla_debug("%s: ion_kernel_ioctl kernel_handle=%llx\n",
		__func__, (unsigned long long)hndl);

	if (ret) {
		// TODO: remove debug
		mdla_debug("%s: ion_kernel_ioctl(hndl=%llx): %d failed\n",
			__func__, (unsigned long long)hndl, ret);
	}
}

