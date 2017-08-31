/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <m4u.h>

#include "vpu_dbg.h"
#include "vpu_drv.h"
#include "vpu_cmn.h"

#define VPU_USING_M4U      (0)
#define ALGO_OF_MAX_POWER  (3)

/* global variables */
int g_vpu_log_level = 1;

/* #ifdef CONFIG_ION */
#if 0

#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>

#include "../test/datapp.h"
#include "../test/datasrc.h"
#include "../test/datadst.h"

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>

#if 0
static void vpu_save_to_file(char *filename, char *buf, int size)
{
	struct file *f = NULL;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(KERNEL_DS);

	if (f == NULL)
		f = filp_open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);

	if (IS_ERR(f))
		LOG_DBG("fail to open %s\n", filename);

	f->f_op->write(f, buf, size, &f->f_pos);

	set_fs(fs);
	filp_close(f, NULL);
}
#else
#define vpu_save_to_file(...)
#endif

static void vpu_test_ion(void)
{
	const int width = 1920;
	const int height = 1080;
	const int data_len = 4172;
	const int sett_len = 1024;
	const int img_size = width * height;
	const int buf_size = img_size * 2 + data_len + sett_len;

	struct vpu_user *user;
	struct vpu_request *req;
	struct vpu_buffer *buf;
	struct vpu_algo *algo;

	struct ion_client *ion_client;
	struct ion_handle *buf_handle;
	struct ion_mm_data mm_data;
	struct ion_sys_data sys_data;

	void *buf_va;
	unsigned int buf_pa;

#if VPU_USING_M4U
	m4u_client_t *m4u_client;
	struct sg_table *buf_sgtbl;
#endif

	ion_client = ion_client_create(g_ion_device, "vpu");
	buf_handle = ion_alloc(ion_client, buf_size, 0, ION_HEAP_MULTIMEDIA_MASK, 0);

#if VPU_USING_M4U
	buf_sgtbl = ion_sg_table(ion_client, buf_handle);
	buf_pa = 0x70000000;
	m4u_client = m4u_create_client();
	m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU, 0, buf_sgtbl, buf_size,
				  M4U_PROT_READ | M4U_PROT_WRITE,
				  M4U_FLAGS_FIX_MVA, &buf_pa);
#else
	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER_EXT;
	mm_data.config_buffer_param.kernel_handle = buf_handle;
	mm_data.config_buffer_param.module_id = VPU_PORT_OF_IOMMU;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 1;
	mm_data.config_buffer_param.reserve_iova_start = 0x50000000;
	mm_data.config_buffer_param.reserve_iova_end = 0x7FFFFFFF;
	if (ion_kernel_ioctl(ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&mm_data) < 0) {
		LOG_ERR("vpu test: config ion buffer failed.\n");
		return;
	}

	sys_data.sys_cmd = ION_SYS_GET_PHYS;
	sys_data.get_phys_param.kernel_handle = buf_handle;
	if (ion_kernel_ioctl(ion_client, ION_CMD_SYSTEM, (unsigned long)&sys_data)) {
		LOG_ERR("vpu test: get phys failed.\n");
		return;
	}

	buf_pa = sys_data.get_phys_param.phy_addr;
	LOG_DBG("vpu test: physical address=0x%x, len=%ld\n",
		 buf_pa, sys_data.get_phys_param.len);
#endif

	if (vpu_find_algo_by_name("ipu_flo_d2d_k3", &algo) != 0) {
		LOG_ERR("algo[ipu_flo_d2d_k3] is not existed!\n");
		return;
	}

	vpu_create_user(&user);

	vpu_alloc_request(&req);
	buf = &req->buffers[0];
	req->algo_id = algo->id;
	req->buffer_count = 3;
	/* buffer 0 */
	buf->format = VPU_BUF_FORMAT_IMG_Y8;
	buf->width = width;
	buf->height = height;
	buf->plane_count = 1;
	buf->planes[0].stride = width;
	buf->planes[0].ptr = buf_pa;
	buf->planes[0].length = width * height;
	buf->port_id = 0;

	/* buffer 1 */
	buf = &req->buffers[1];
	buf->format = VPU_BUF_FORMAT_IMG_Y8;
	buf->width = width;
	buf->height = height;
	buf->plane_count = 1;
	buf->planes[0].stride = width;
	buf->planes[0].ptr = buf_pa + width * height;
	buf->planes[0].length = width * height;
	buf->port_id = 1;

	/* buffer 2 */
	buf = &req->buffers[2];
	buf->format = VPU_BUF_FORMAT_DATA;
	buf->plane_count = 1;
	buf->planes[0].ptr = buf_pa + 2 * width * height;
	buf->planes[0].length = data_len;
	buf->port_id = 2;

	buf_va = ion_map_kernel(ion_client, buf_handle);

	LOG_INF("ion alloced: src_va=0x%p\n", buf_va);

	memcpy(buf_va, g_datasrc_1920x1080_wpp, img_size);
	memset((char *)buf_va + img_size, 0x19, img_size);
	memcpy((char *)buf_va + img_size * 2, g_procParam, data_len);

	/* update request's setting */
	{
		struct emu_setting {
			int param1;
			int param2;
			int param3;
			int param4;
			int param5;
		};
		struct emu_setting *sett;

		sett = (struct emu_setting *)((char *)buf_va + (img_size * 2 + data_len));
		sett->param1 = 1;
		sett->param2 = 2;
		sett->param3 = 3;
		sett->param4 = 4;
		req->sett_ptr = buf_pa + (img_size * 2 + data_len);
		req->sett_length = 1024;
	}

	vpu_push_request_to_queue(user, req);
	vpu_pop_request_from_queue(user, &req);

	/* compare with golden */
	{
		int ret_cmp;

		ret_cmp = memcmp((char *)buf_va + img_size, g_datadst_1920x1080_golden_wpp, img_size);
		LOG_INF("comparison result:%d", ret_cmp);
		vpu_save_to_file("/data/dst.raw", (char *)buf_va + img_size, img_size);
	}
	vpu_free_request(req);
	vpu_delete_user(user);

#if VPU_USING_M4U
	m4u_dealloc_mva(m4u_client, VPU_PORT_OF_IOMMU, buf_pa);
	m4u_destroy_client(m4u_client);
#endif

	ion_free(ion_client, buf_handle);
	ion_client_destroy(ion_client);
}


static void vpu_test_be_true(void)
{
	struct vpu_user *user;
	struct vpu_request *req;
	struct vpu_buffer *buf;
	struct vpu_algo *algo;
	int width, height, size;

	m4u_client_t *m4u_client;
	struct ion_client *ion_client;
	struct ion_handle *buf_handle;

	unsigned char *buf_va;
	unsigned int buf_pa;

	struct sg_table *buf_sgtbl;

	width = height = 64;
	size = width * height * 2;

	m4u_client = m4u_create_client();
	ion_client = ion_client_create(g_ion_device, "vpu");

	buf_handle = ion_alloc(ion_client, size, 0, ION_HEAP_MULTIMEDIA_MASK, 0);
	buf_sgtbl = ion_sg_table(ion_client, buf_handle);
	buf_pa = 0x70000000;
	m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU, 0, buf_sgtbl, size,
				  M4U_PROT_READ | M4U_PROT_WRITE,
				  M4U_FLAGS_FIX_MVA, &buf_pa);

	if (vpu_find_algo_by_name("ipu_flo_d2d_k5", &algo) != 0) {
		LOG_ERR("algo[ipu1_flo_d2d_k5] is not existed in pool\n");
		return;
	}

	vpu_create_user(&user);

	vpu_alloc_request(&req);
	buf = &req->buffers[0];
	req->algo_id = algo->id;
	req->buffer_count = 0;

	buf_va = (unsigned char *) ion_map_kernel(ion_client, buf_handle);
	LOG_INF("ion alloced: src_va=0x%p\n", buf_va);

	memset(buf_va, 0x1, width * height);

	vpu_push_request_to_queue(user, req);
	vpu_pop_request_from_queue(user, &req);

	{
		int ret_cmp;

		memset((char *)buf_va, 0x2, width * height);
		ret_cmp = memcmp(buf_va, buf_va + width * height, width * height);
		LOG_INF("comparison result:%d", ret_cmp);
	}
	vpu_free_request(req);
	vpu_delete_user(user);

	m4u_dealloc_mva(m4u_client, VPU_PORT_OF_IOMMU, buf_pa);
	m4u_destroy_client(m4u_client);

	ion_free(ion_client, buf_handle);
	ion_client_destroy(ion_client);
}

#else
#define vpu_test_ion(...)
#define vpu_test_be_true(...)
#endif

#ifdef MTK_VPU_DVT
static u64 test_value;
static struct vpu_user *test_user[10];


static int vpu_user_test_case1(void *arg)
{
	struct vpu_user *user;
	struct vpu_request *req;
	int i;

	vpu_create_user(&user);

	for (i = 0; i < 100; i++) {
		vpu_alloc_request(&req);
		req->algo_id = ALGO_OF_MAX_POWER;
		req->buffer_count = 0;
		req->priv = i;
		vpu_push_request_to_queue(user, req);
	}

	for (i = 0; i < 100;) {
		if (vpu_pop_request_from_queue(user, &req)) {
			LOG_ERR("deque failed. i=%d\n", i);
		} else {
			if (req->priv != i) {
				LOG_ERR("out of order of request deque. i=%d, priv=%d\n",
						i, (int) req->priv);
			}
			i++;
			vpu_free_request(req);
		}
		msleep(30);
	}

	vpu_delete_user(user);
	return 0;
}


static int vpu_user_test_case2(void *arg)
{
	struct vpu_user *user;
	int i;

	vpu_create_user(&user);

	for (i = 0; i < 100; i++) {
		struct vpu_request *req;

		if (vpu_alloc_request(&req)) {
			LOG_ERR("allocate request failed. i=%d\n", i);
			return 0;
		}

		req->algo_id = ALGO_OF_MAX_POWER;
		req->buffer_count = 0;
		vpu_push_request_to_queue(user, req);

		if (vpu_pop_request_from_queue(user, &req)) {
			LOG_ERR("deque request failed. i=%d\n", i);
		} else {
			vpu_free_request(req);
			break;
		}
	}

	vpu_delete_user(user);
	return 0;
}


static int vpu_user_test_case3(void *arg)
{
	struct vpu_user *user;
	struct vpu_request *req;
	int i;

	vpu_create_user(&user);

	for (i = 0; i < 100; i++) {
		vpu_alloc_request(&req);
		req->algo_id = ALGO_OF_MAX_POWER;
		req->buffer_count = 0;
		req->priv = i;
		vpu_push_request_to_queue(user, req);
	}

	for (i = 0; i < 100; i++) {
		if (i == 50 && vpu_flush_requests_from_queue(user))
			LOG_ERR("flush request failed!\n");

		if (vpu_pop_request_from_queue(user, &req)) {
			LOG_ERR("deque request failed. i=%d\n", i);
			break;
		}

		if (req->priv != i) {
			LOG_ERR("out of order of request deque. i=%d, priv=%d\n",
					i, (int) req->priv);
		}
		vpu_free_request(req);
	}

	vpu_delete_user(user);
	return 0;
}

static int vpu_test_lock(void)
{
	struct vpu_user *user;

	vpu_create_user(&user);
	vpu_hw_lock(user);
	msleep(10 * 1000);
	vpu_hw_unlock(user);
	vpu_delete_user(user);

	return 0;
}

/*
 * 1: boot up
 * 1X: use algo id to load algo
 * 4X: create user X
 * 5X: push a request to user X
 * 6X: pop a request to user X
 * 7X: flush requests of user X
 * 8X: delete user X
 * 9X: delete user X
 * 100: let emulator busy
 * 101: run test case1
 * 102: run test case2
 * 103: run test case3
 */
static int vpu_test_set(void *data, u64 val)
{
	struct vpu_algo *algo;
	struct vpu_request *req;

	LOG_INF("vpu_test_set:val=%llu\n", val);

	switch (val) {
	case 0:
		/* do nothing */
		break;
	case 1:
		vpu_boot_up();
		break;
	case 10 ... 19: /* use algo's id to load algo */
	{
		vpu_id_t id = (int) val - 10;

		if (vpu_find_algo_by_id(id, &algo))
			LOG_DBG("algo[%d] is not existed\n", id);

		break;
	}
	case 40 ... 49: /* create user X */
	{
		int index = val - 40;

		if (test_user[index] == NULL)
			vpu_create_user(&test_user[index]);
		break;
	}
	case 50 ... 59: /* push a request to user X */
	{
		int index = val - 50;

		vpu_alloc_request(&req);
		req->algo_id = ALGO_OF_MAX_POWER;
		req->buffer_count = 0;
		vpu_push_request_to_queue(test_user[index], req);
		break;
	}
	case 60 ... 69: /* pop a request to user N */
	{
		int index = val - 60;

		if (vpu_pop_request_from_queue(test_user[index], &req))
			LOG_ERR("deque failed! user: %d\n", index);
		else
			vpu_free_request(req);

		break;
	}
	case 70 ... 79: /* flush requests of user X */
	{
		int index = val - 70;

		vpu_flush_requests_from_queue(test_user[index]);
		break;
	}
	case 80 ... 89: /* delete user X */
	{
		int index = val - 80;

		vpu_delete_user(test_user[index]);
		test_user[index] = NULL;
		break;
	}
	case 90:
		vpu_test_ion();
		break;
	case 91:
		vpu_test_be_true();
		break;
	case 92:
		vpu_test_lock();
		break;
	case 100:
		vpu_ext_be_busy();
		break;
	case 101:
	{
		struct task_struct *task;

		task = kthread_create(vpu_user_test_case1, NULL, "vpu-test1-thread");
		wake_up_process(task);
		break;
	}
	case 102:
	{
		struct task_struct *task;

		task = kthread_create(vpu_user_test_case2, NULL, "vpu-test2-thread");
		wake_up_process(task);
		break;
	}
	case 103:
	{
		struct task_struct *task;

		task = kthread_create(vpu_user_test_case3, NULL, "vpu-test3-thread");
		wake_up_process(task);
		break;
	}
	default:
		LOG_INF("vpu_test_set error,val=%llu\n", val);
	}

	test_value = val;
	return 0;
}

static int vpu_test_get(void *data, u64 *val)
{
	*val = test_value;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vpu_test_fops, vpu_test_get, vpu_test_set, "%llu\n");
#endif

static int vpu_log_level_set(void *data, u64 val)
{
	g_vpu_log_level = val & 0xf;
	LOG_INF("g_vpu_log_level: %d\n", g_vpu_log_level);

	return 0;
}

static int vpu_log_level_get(void *data, u64 *val)
{
	*val = g_vpu_log_level;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vpu_log_level_fops, vpu_log_level_get, vpu_log_level_set, "%llu\n");

int vpu_debug_algo_show(struct seq_file *s, void *unused)
{
	vpu_dump_algo(s);
	return 0;
}

int vpu_debug_algo_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_algo_show, inode->i_private);
}

const struct file_operations vpu_debug_algo_fops = {
	.open = vpu_debug_algo_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};


int vpu_debug_reg_show(struct seq_file *s, void *unused)
{
	vpu_dump_register(s);
	return 0;
}

int vpu_debug_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_reg_show, inode->i_private);
}

const struct file_operations vpu_debug_reg_fops = {
	.open = vpu_debug_reg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};


int vpu_debug_user_show(struct seq_file *s, void *unused)
{
	vpu_dump_user(s);
	return 0;
}

int vpu_debug_user_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_user_show, inode->i_private);
}

const struct file_operations vpu_debug_user_fops = {
	.open = vpu_debug_user_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int vpu_debug_image_file_show(struct seq_file *s, void *unused)
{
	vpu_dump_image_file(s);
	return 0;
}

int vpu_debug_image_file_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_image_file_show, inode->i_private);
}

const struct file_operations vpu_debug_image_file_fops = {
	.open = vpu_debug_image_file_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int vpu_debug_mesg_show(struct seq_file *s, void *unused)
{
	vpu_dump_mesg(s);
	return 0;
}

int vpu_debug_mesg_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_mesg_show, inode->i_private);
}

const struct file_operations vpu_debug_mesg_fops = {
	.open = vpu_debug_mesg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int vpu_init_debug(struct vpu_device *vpu_dev)
{
	struct dentry *debug_file;

	vpu_dev->debug_root = debugfs_create_dir("vpu", NULL);

	if (IS_ERR_OR_NULL(vpu_dev->debug_root))
		LOG_INF("vpu: failed to create debug dir.\n");

	debug_file = debugfs_create_file("algo", 0644, vpu_dev->debug_root, NULL, &vpu_debug_algo_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 1.\n");

	debug_file = debugfs_create_file("log_level", 0644, vpu_dev->debug_root, NULL, &vpu_log_level_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 2.\n");

	debug_file = debugfs_create_file("reg", 0644, vpu_dev->debug_root, NULL, &vpu_debug_reg_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 3.\n");

	debug_file = debugfs_create_file("user", 0644, vpu_dev->debug_root, NULL, &vpu_debug_user_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 4.\n");

	debug_file = debugfs_create_file("image_file", 0644, vpu_dev->debug_root, NULL, &vpu_debug_image_file_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 5.\n");

	debug_file = debugfs_create_file("mesg", 0644, vpu_dev->debug_root, NULL, &vpu_debug_mesg_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 6.\n");

#ifdef MTK_VPU_DVT
	debug_file = debugfs_create_file("test", 0644, vpu_dev->debug_root, NULL, &vpu_test_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_INF("vpu: failed to create debug files 7.\n");
#endif

	return 0;
}
