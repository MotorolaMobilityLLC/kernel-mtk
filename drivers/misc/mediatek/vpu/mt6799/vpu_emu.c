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

#include "vpu_emu.h"
#include "vpu_drv.h"
#include "vpu_cmn.h"
#include "vpu_reg.h"
#include "vpu_hw.h"
#include <m4u.h>

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

static uint64_t vpu_base;
static uint64_t bin_base;
static struct task_struct *emulator_task;
static struct workqueue_struct *busy_workqueue;
static struct work_struct *busy_work;
static irq_handler_t irq_handler;
static uint32_t irq_num;

static void vpu_emulator_resotre_ready(struct work_struct *w)
{
	mdelay(10 * 1000);
	vpu_write_field(FLD_XTENSA_INFO0, VPU_STATE_READY);
}

static void vpu_emulator_be_busy(void)
{
	vpu_write_field(FLD_XTENSA_INFO0, VPU_STATE_BUSY);
	queue_work(busy_workqueue, busy_work);
	schedule_work(busy_work);
}

static int vpu_emulator_do_loader(void)
{
	LOG_DBG("emulater: do loader\n");
	return 0;
}

struct emu_setting {
	int param1;
	int param2;
	int param3;
	int param4;
	int param5;
};

#define INS_PROP(id, name, type, count, access) \
	{ id, VPU_PROP_TYPE_ ## type, VPU_PROP_ACCESS_ ## access, 0, count, name }

static struct vpu_prop_desc info_prop_descs[4] = {
	INS_PROP(1, "version", INT32, 1, RDONLY),
	INS_PROP(2, "wrk_buf_size", INT32, 1, RDONLY),
	INS_PROP(3, "support_format", INT32, 10, RDONLY),
	INS_PROP(4, "reserved", INT32, 64, RDONLY),
};
static struct vpu_prop_desc sett_prop_descs[5] = {
	INS_PROP(1, "param1", INT32, 1, RDONLY),
	INS_PROP(2, "param2", INT32, 1, RDONLY),
	INS_PROP(3, "param3", INT32, 1, RDONLY),
	INS_PROP(4, "param4", INT32, 1, RDONLY),
	INS_PROP(5, "param5", INT32, 1, RDWR),
};
#undef INS_PROP

static int vpu_emulator_do_d2d(void)
{
	unsigned int mva_buffers, num_buffers;
	unsigned int tmp_size;
	unsigned long va_buffers;
	struct vpu_buffer *buf;

	num_buffers = vpu_read_field(FLD_XTENSA_INFO12);
	mva_buffers = vpu_read_field(FLD_XTENSA_INFO13);

	LOG_DBG("emulater[D2D]: num_buffers=%d, mva_buffers=0x%x\n", num_buffers, mva_buffers);

	/* print buffer info */
	if (m4u_mva_map_kernel(mva_buffers,
						   (unsigned int) (sizeof(struct vpu_buffer) * num_buffers),
						   &va_buffers, &tmp_size)) {
		LOG_ERR("m4u_mva_map_kernel failed: mva_buffers");
	}

	buf = (struct vpu_buffer *) va_buffers;
	LOG_DBG("emulater[D2D]: width=%d, height=%d, port_id=%d\n",
			buf->width, buf->height, buf->port_id);

	m4u_mva_unmap_kernel(mva_buffers,
						 (unsigned int) (sizeof(struct vpu_buffer) * num_buffers), va_buffers);

#if 0
	{
		struct emu_setting *emu_sett;
		unsigned int  mva_param, size_param;
		unsigned long va_param;

		mva_param = vpu_read_field(FLD_XTENSA_INFO14);
		size_param = vpu_read_field(FLD_XTENSA_INFO15);

		/* print parameters */
		if (m4u_mva_map_kernel(mva_param, size_param, &va_param, &tmp_size))
			LOG_ERR("m4u_mva_map_kernel failed: mva_param");

		emu_sett = (struct emu_setting *) va_param;
		emu_sett->param5 = emu_sett->param1 + emu_sett->param2 + emu_sett->param3 + emu_sett->param4;

		LOG_DBG("emulater[D2D]: param1=%d, param2=%d, param3=%d, param4=%d, param5=%d\n",
				emu_sett->param1, emu_sett->param2, emu_sett->param3,
				emu_sett->param4, emu_sett->param5);

		m4u_mva_unmap_kernel(mva_param, size_param, va_param);
	}
#endif

	msleep(100);
	return 0;
}

static int vpu_emulator_get_algo(void)
{
	int i;
	unsigned int mva_ports, mva_info_data, mva_info_descs, mva_sett_descs;
	unsigned int size_ports, size_info_data, size_info_descs, size_sett_descs;
	unsigned long va_ports, va_info_data, va_info_descs, va_sett_descs;
	unsigned int tmp_size;

	struct vpu_port *port;

	mva_ports = vpu_read_field(FLD_XTENSA_INFO6);
	size_ports = sizeof(struct vpu_port) * VPU_MAX_NUM_PORTS;
	mva_info_data = vpu_read_field(FLD_XTENSA_INFO7);
	size_info_data = vpu_read_field(FLD_XTENSA_INFO8);
	mva_info_descs = vpu_read_field(FLD_XTENSA_INFO10);
	size_info_descs = sizeof(struct vpu_prop_desc) * VPU_MAX_NUM_PROPS;
	mva_sett_descs = vpu_read_field(FLD_XTENSA_INFO12);
	size_sett_descs = sizeof(struct vpu_prop_desc) * VPU_MAX_NUM_PROPS;

	if (m4u_mva_map_kernel(mva_ports, size_ports, &va_ports, &tmp_size))
		LOG_ERR("m4u_mva_map_kernel failed: mva_ports");

	if (m4u_mva_map_kernel(mva_info_data, size_info_data, &va_info_data, &tmp_size))
		LOG_ERR("m4u_mva_map_kernel failed: mva_info_data");

	if (m4u_mva_map_kernel(mva_info_descs, size_info_descs, &va_info_descs, &tmp_size))
		LOG_ERR("m4u_mva_map_kernel failed: mva_info_desc");

	if (m4u_mva_map_kernel(mva_sett_descs, size_sett_descs, &va_sett_descs, &tmp_size))
		LOG_ERR("m4u_mva_map_kernel failed: mva_sett_desc");

	/* ports */
	port = (struct vpu_port *) va_ports;
	for (i = 0; i < 3; i++) {
		port[i].id = i + 1;
		sprintf(port[i].name, "port-emu-%d", i + 1);
		port[i].dir = VPU_PORT_DIR_IN_OUT;
		port[i].usage = VPU_PORT_USAGE_IMAGE;
	}
	vpu_write_field(FLD_XTENSA_INFO5, 3);

	/* property description */
	memcpy((void *) va_info_descs, info_prop_descs, sizeof(struct vpu_prop_desc) * 4);
	memcpy((void *) va_sett_descs, sett_prop_descs, sizeof(struct vpu_prop_desc) * 5);
	vpu_write_field(FLD_XTENSA_INFO9, 4);
	vpu_write_field(FLD_XTENSA_INFO11, 5);

	/* property data */
	*((unsigned int *) va_info_data + 0) = 0x0100;
	*((unsigned int *) va_info_data + 1) = 0x1234;
	*((unsigned int *) va_info_data + 2) = 0x000f;
	for (i = 0; i < 100; i++)
		*((unsigned char *) (va_info_data + 48) + i) = i;

	m4u_mva_unmap_kernel(mva_ports, size_ports, va_ports);
	m4u_mva_unmap_kernel(mva_info_data, size_info_data, va_info_data);
	m4u_mva_unmap_kernel(mva_info_descs, size_info_descs, va_info_descs);
	m4u_mva_unmap_kernel(mva_sett_descs, size_sett_descs, va_sett_descs);
	return 0;
}

static int vpu_emulator_loop(void *arg)
{
	uint32_t timeout, cmd;

	for (; ;) {
		if (kthread_should_stop())
			break;
		do {
			set_current_state(TASK_INTERRUPTIBLE);
			timeout = schedule_timeout(msecs_to_jiffies(100));

			if (vpu_read_field(FLD_INT_CTL_XTENSA00) == 1) {
				cmd = vpu_read_field(FLD_XTENSA_INFO1);
				LOG_DBG("received interrupt from arm, cmd:%d\n", cmd);

				switch (cmd) {
				case VPU_CMD_DO_LOADER:
					vpu_emulator_do_loader();
					break;
				case VPU_CMD_DO_D2D:
					vpu_emulator_do_d2d();
					break;
				case VPU_CMD_GET_ALGO:
					vpu_emulator_get_algo();
					break;
				case VPU_CMD_EXT_BUSY:
					vpu_emulator_be_busy();
					break;
				}

				vpu_write_field(FLD_INT_CTL_XTENSA00, 0);
				LOG_DBG("finish the isr of emulator\n");
				if (irq_handler != NULL)
					irq_handler(irq_num, NULL);
			}
		} while (timeout);
	}

	return 0;
}

static int vpu_emulate_binary_data(uint64_t addr)
{
	size_t i, j;
	struct vpu_algo_info *algo_info;
	struct vpu_image_header *header = (void *) (addr + (VPU_OFFSET_IMAGE_HEADERS));

	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++, header++) {
		header->algo_info_count = i + 1;
		for (j = 0; j < header->algo_info_count; j++) {
			algo_info = &header->algo_infos[j];
			algo_info->offset = VPU_OFFSET_ALGO_AREA + (i + j) * 0x1000;
			algo_info->length = 0x1000;
			sprintf(algo_info->name, "emulator-algo-%02d", (int) (1 + i + j));
		}
	}

	return 0;
}


int vpu_init_emulator(struct vpu_device *device)
{
	int ret;

	/* Allocate the register of VPU emulator */
	vpu_base = (uint64_t) kzalloc(0xF0 + 4, GFP_KERNEL);
	if (vpu_base == 0) {
		LOG_ERR("fail to alloc register area!\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Emulate the memory of IPU binary data, which allocated by Little Kernel */
	bin_base = (uint64_t) vmalloc(VPU_SIZE_BINARY_CODE);
	if (bin_base == 0) {
		LOG_ERR("fail to alloc binary area!\n");
		ret = -ENOMEM;
		goto out;
	}

	device->vpu_base = vpu_base;
	device->bin_base = bin_base;
	vpu_emulate_binary_data(bin_base);


	busy_work = kzalloc(sizeof(struct work_struct), GFP_KERNEL);
	if (busy_work == 0) {
		LOG_ERR("fail to alloc work_struct!\n");
		ret = -ENOMEM;
		goto out;
	}

	INIT_WORK(busy_work, vpu_emulator_resotre_ready);
	busy_workqueue = create_singlethread_workqueue("busy_work");

	/* init register module before call */
	vpu_init_reg(device);
	/* VPU Interrupt bit: default 1 */
	vpu_write_field(FLD_INT_XTENSA, 1);
	/* Change status to ready. */
	vpu_write_field(FLD_XTENSA_INFO0, VPU_STATE_READY);

	emulator_task = kthread_create(vpu_emulator_loop, NULL, "vpu-emu");
	if (IS_ERR(emulator_task)) {
		ret = PTR_ERR(emulator_task);
		emulator_task = NULL;
		goto out;
	}
	wake_up_process(emulator_task);

	return 0;

out:
	vpu_uninit_emulator();

	return ret;
}

int vpu_request_emulator_irq(uint32_t irq, irq_handler_t handler)
{
	irq_num = irq;
	irq_handler = handler;
	return 0;
}

int vpu_uninit_emulator(void)
{
	if (emulator_task != NULL)
		kthread_stop(emulator_task);

	if (busy_workqueue != NULL)
		destroy_workqueue(busy_workqueue);

	if (busy_work != NULL)
		kfree(busy_work);

	if (vpu_base != 0) {
		kfree((void *) vpu_base);
		vpu_base = 0;
	}

	if (bin_base != 0) {
		vfree((void *) bin_base);
		bin_base = 0;
	}
	return 0;
}
