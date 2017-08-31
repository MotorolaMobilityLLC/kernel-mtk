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

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/ctype.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <m4u.h>
#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>
#include <mtk_gpio.h>
#include <mach/gpio_const.h>
#include <mt-plat/mtk_chip.h>

#ifndef MTK_VPU_FPGA_PORTING
#include <mmdvfs_mgr.h>
#include <mtk_smi.h>
#include <mtk_pmic_info.h>
#endif

#ifdef MTK_VPU_DVT
#define VPU_TRACE_ENABLED
#endif

#include "vpu_hw.h"
#include "vpu_reg.h"
#include "vpu_cmn.h"
#include "vpu_algo.h"
#include "vpu_dbg.h"

#define CMD_WAIT_TIME_MS    (10 * 1000)
#define PWR_KEEP_TIME_MS    (500)
#define IOMMU_VA_START      (0x60000000)
#define IOMMU_VA_END        (0x7FFFFFFF)

/* working buffer */
static struct vpu_shared_memory *work_buf;
static uint64_t work_buf_va;
static uint32_t work_buf_pa;

/* ion & m4u */
static m4u_client_t *m4u_client;
static struct ion_client *ion_client;
static struct sg_table sg_reset_vector;
static struct sg_table sg_main_program;

static uint64_t vpu_base;
static uint64_t bin_base;
static struct vpu_device *vpu_dev;
static struct task_struct *enque_task;
static struct mutex cmd_mutex;
static wait_queue_head_t cmd_wait;
static bool is_cmd_done;
static bool is_boot_up;
static vpu_id_t algo_loaded;

#ifndef MTK_VPU_FPGA_PORTING
/* regulator */
static struct regulator *reg_vimvo;

/* clock */
static struct clk *clk_top_dsp_sel;
static struct clk *clk_top_ipu_if_sel;
static struct clk *clk_ipu;
static struct clk *clk_ipu_isp;
static struct clk *clk_ipu_jtag;
static struct clk *clk_ipu_axi;
static struct clk *clk_ipu_ahb;
static struct clk *clk_ipu_mm_axi;
static struct clk *clk_ipu_cam_axi;
static struct clk *clk_ipu_img_axi;
static struct clk *clk_top_syspll1_d2;
static struct clk *clk_top_syspll_d3;
static struct clk *clk_top_mmpll_d5;
static struct clk *clk_top_vcodecpll_d6;
static struct clk *clk_top_syspll_d2;

/* mtcmos */
static struct clk *mtcmos_mm0;
static struct clk *mtcmos_ipu;

/* smi */
static struct clk *clk_mm_ipu;
static struct clk *clk_mm_gals_m0_2x;
static struct clk *clk_mm_gals_m1_2x;
static struct clk *clk_mm_upsz0;
static struct clk *clk_mm_upsz1;
static struct clk *clk_mm_fifo0;
static struct clk *clk_mm_fifo1;
static struct clk *clk_mm_smi_common;
static struct clk *clk_mm_smi_common_2x;
#endif

/* power */
static struct mutex power_mutex;
static bool is_power_dynamic = true;

/* mmdvfs */
static struct vpu_dvfs_steps step_dsp;
static struct vpu_dvfs_steps step_ipu_if;
static struct vpu_dvfs_steps step_vimvo;

/* jtag */
static bool is_jtag_enabled;

/* direct link */
static bool is_locked;
static struct mutex lock_mutex;
static wait_queue_head_t lock_wait;

/* sw version */
static uint32_t sw_ver;

static inline void lock_command(void)
{
	mutex_lock(&cmd_mutex);
	is_cmd_done = false;
}

static inline int wait_command(void)
{
	return (wait_event_interruptible_timeout(
				cmd_wait, is_cmd_done, msecs_to_jiffies(CMD_WAIT_TIME_MS)) > 0)
			? 0 : -ETIMEDOUT;
}

static inline void unlock_command(void)
{
	mutex_unlock(&cmd_mutex);
}

#ifdef MTK_VPU_FPGA_PORTING
#define vpu_prepare_regulator_and_clock(...)   0
#define vpu_enable_regulator_and_clock(...)    0
#define vpu_disable_regulator_and_clock(...)   0
#define vpu_unprepare_regulator_and_clock(...)
#else
static int vpu_prepare_regulator_and_clock(struct device *pdev)
{
	int ret = 0;

	reg_vimvo = regulator_get(pdev, "vimvo");
	ret = IS_ERR(reg_vimvo);
	CHECK_RET("can not find regulator: vimvo\n");

#define PREPARE_VPU_MTCMOS(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find mtcmos: %s\n", #clk); \
		} \
	}

	PREPARE_VPU_MTCMOS(mtcmos_mm0);
	PREPARE_VPU_MTCMOS(mtcmos_ipu);
#undef PREPARE_VPU_MTCMOS

#define PREPARE_VPU_CLK(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find clock: %s\n", #clk); \
		} else if (clk_prepare(clk)) { \
			ret = -EBADE; \
			LOG_ERR("fail to prepare clock: %s\n", #clk); \
		} \
	}

	PREPARE_VPU_CLK(clk_mm_ipu);
	PREPARE_VPU_CLK(clk_mm_gals_m0_2x);
	PREPARE_VPU_CLK(clk_mm_gals_m1_2x);
	PREPARE_VPU_CLK(clk_mm_upsz0);
	PREPARE_VPU_CLK(clk_mm_upsz1);
	PREPARE_VPU_CLK(clk_mm_fifo0);
	PREPARE_VPU_CLK(clk_mm_fifo1);
	PREPARE_VPU_CLK(clk_mm_smi_common);
	PREPARE_VPU_CLK(clk_mm_smi_common_2x);
	PREPARE_VPU_CLK(clk_ipu);
	PREPARE_VPU_CLK(clk_ipu_isp);
	PREPARE_VPU_CLK(clk_ipu_jtag);
	PREPARE_VPU_CLK(clk_ipu_axi);
	PREPARE_VPU_CLK(clk_ipu_ahb);
	PREPARE_VPU_CLK(clk_ipu_mm_axi);
	PREPARE_VPU_CLK(clk_ipu_cam_axi);
	PREPARE_VPU_CLK(clk_ipu_img_axi);
	PREPARE_VPU_CLK(clk_top_dsp_sel);
	PREPARE_VPU_CLK(clk_top_ipu_if_sel);
	if (sw_ver == CHIP_SW_VER_02) {
		PREPARE_VPU_CLK(clk_top_syspll1_d2);
		PREPARE_VPU_CLK(clk_top_syspll_d3);
		PREPARE_VPU_CLK(clk_top_vcodecpll_d6);
		PREPARE_VPU_CLK(clk_top_syspll_d2);
	} else {
		PREPARE_VPU_CLK(clk_top_syspll_d3);
		PREPARE_VPU_CLK(clk_top_mmpll_d5);
	}
#undef PREPARE_VPU_CLK

out:
	return ret;
}

static int vpu_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	if (sw_ver == CHIP_SW_VER_02) {
		/* set dsp frequency - 0:546MHZ, 1:504HMz, 2:364MHz, 3:274MHz */
		switch (step) {
		case 0:
			clk_src = clk_top_syspll_d2;
			break;
		case 1:
			clk_src = clk_top_vcodecpll_d6;
			break;
		case 2:
			clk_src = clk_top_syspll_d3;
			break;
		case 3:
			clk_src = clk_top_syspll1_d2;
			break;
		default:
			return -EINVAL;
		}
	} else {
		/* set dsp frequency - 0:480MHz, 1:364MHz */
		switch (step) {
		case 0:
			clk_src = clk_top_mmpll_d5;
			break;
		case 1:
			clk_src = clk_top_syspll_d3;
			break;
		default:
			return -EINVAL;
		}
	}

	return clk_set_parent(clk, clk_src);
}

static int vpu_enable_regulator_and_clock(void)
{
	int ret;

	vpu_trace_begin("vpu_enable_regulator_and_clock");

	/* set VSRAM_CORE to 1.0V */
	vpu_trace_begin("vsram_vcore_tracking:disable");
	ret = enable_vsram_vcore_hw_tracking(0);
	vpu_trace_end();
	CHECK_RET("fail to disable vsram_core tracking!\n");
	ndelay(40);

	vpu_trace_begin("vimvo:enable_lp_mode");
	ret = enable_vimvo_lp_mode(0);
	vpu_trace_end();
	CHECK_RET("fail to switch vimvo to normal mode!\n");

	ret = (reg_vimvo == NULL) ? -1 : 0;
	CHECK_RET("regulator not existed: vimvo\n");

	vpu_trace_begin("vimvo:enable");
	ret = regulator_enable(reg_vimvo);
	vpu_trace_end();
	CHECK_RET("fail to enable regulator: vimvo\n");

	ret = (step_vimvo.curr >= step_vimvo.max) ? -EINVAL : 0;
	CHECK_RET("vimvo has wrong voltage, step=%d\n", step_vimvo.curr);

	vpu_trace_begin("vimvo:set_voltage");
	ret = regulator_set_voltage(reg_vimvo,
		step_vimvo.values[step_vimvo.curr],
		step_vimvo.values[step_vimvo.curr]);
	vpu_trace_end();
	CHECK_RET("fail to set vimvo, step=%d, ret=%d\n", step_vimvo.curr, ret);
	ndelay(70);

#define ENABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_prepare_enable(clk)) \
				LOG_ERR("fail to prepare and enable mtcmos: %s\n", #clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}

	vpu_trace_begin("mtcmos:enable");
	ENABLE_VPU_MTCMOS(mtcmos_mm0);
	ENABLE_VPU_MTCMOS(mtcmos_ipu);
	vpu_trace_end();
#undef ENABLE_VPU_MTCMOS

#define ENABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_enable(clk)) \
				LOG_ERR("fail to enable clock: %s\n", #clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}

	vpu_trace_begin("clock:enable");
	ENABLE_VPU_CLK(clk_mm_ipu);
	ENABLE_VPU_CLK(clk_mm_gals_m0_2x);
	ENABLE_VPU_CLK(clk_mm_gals_m1_2x);
	ENABLE_VPU_CLK(clk_mm_upsz0);
	ENABLE_VPU_CLK(clk_mm_upsz1);
	ENABLE_VPU_CLK(clk_mm_fifo0);
	ENABLE_VPU_CLK(clk_mm_fifo1);
	ENABLE_VPU_CLK(clk_mm_smi_common);
	ENABLE_VPU_CLK(clk_mm_smi_common_2x);
	ENABLE_VPU_CLK(clk_ipu);
	ENABLE_VPU_CLK(clk_ipu_isp);
	ENABLE_VPU_CLK(clk_ipu_jtag);
	ENABLE_VPU_CLK(clk_ipu_axi);
	ENABLE_VPU_CLK(clk_ipu_ahb);
	ENABLE_VPU_CLK(clk_ipu_mm_axi);
	ENABLE_VPU_CLK(clk_ipu_cam_axi);
	ENABLE_VPU_CLK(clk_ipu_img_axi);
	ENABLE_VPU_CLK(clk_top_dsp_sel);
	ENABLE_VPU_CLK(clk_top_ipu_if_sel);
	vpu_trace_end();
#undef ENABLE_VPU_CLK

	vpu_set_clock_source(clk_top_dsp_sel, step_dsp.curr);
	CHECK_RET("fail to set dsp freq, step=%d, ret=%d\n", step_dsp.curr, ret);

	vpu_set_clock_source(clk_top_ipu_if_sel, step_ipu_if.curr);
	CHECK_RET("fail to set ipu_if freq, step=%d, ret=%d\n", step_ipu_if.curr, ret);

out:
	vpu_trace_end();
	return ret;
}



static int vpu_disable_regulator_and_clock(void)
{
	int ret;

#define DISABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable(clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}

	DISABLE_VPU_CLK(clk_top_dsp_sel);
	DISABLE_VPU_CLK(clk_top_ipu_if_sel);
	DISABLE_VPU_CLK(clk_ipu);
	DISABLE_VPU_CLK(clk_ipu_isp);
	DISABLE_VPU_CLK(clk_ipu_jtag);
	DISABLE_VPU_CLK(clk_ipu_axi);
	DISABLE_VPU_CLK(clk_ipu_ahb);
	DISABLE_VPU_CLK(clk_ipu_mm_axi);
	DISABLE_VPU_CLK(clk_ipu_cam_axi);
	DISABLE_VPU_CLK(clk_ipu_img_axi);
	DISABLE_VPU_CLK(clk_mm_ipu);
	DISABLE_VPU_CLK(clk_mm_gals_m0_2x);
	DISABLE_VPU_CLK(clk_mm_gals_m1_2x);
	DISABLE_VPU_CLK(clk_mm_upsz0);
	DISABLE_VPU_CLK(clk_mm_upsz1);
	DISABLE_VPU_CLK(clk_mm_fifo0);
	DISABLE_VPU_CLK(clk_mm_fifo1);
	DISABLE_VPU_CLK(clk_mm_smi_common);
	DISABLE_VPU_CLK(clk_mm_smi_common_2x);
#undef DISABLE_VPU_CLK

#define DISABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable_unprepare(clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}

	DISABLE_VPU_MTCMOS(mtcmos_ipu);
	DISABLE_VPU_MTCMOS(mtcmos_mm0);
#undef DISABLE_VPU_MTCMOS

	ret = enable_vimvo_lp_mode(1);
	CHECK_RET("fail to switch vimvo to sleep mode!\n");

	ret = enable_vsram_vcore_hw_tracking(1);
	CHECK_RET("fail to enable vsram_core tracking!\n");

out:
	return ret;
}

static void vpu_unprepare_regulator_and_clock(void)
{

#define UNPREPARE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_unprepare(clk); \
			clk = NULL; \
		} \
	}

	UNPREPARE_VPU_CLK(clk_top_dsp_sel);
	UNPREPARE_VPU_CLK(clk_top_ipu_if_sel);
	if (sw_ver == CHIP_SW_VER_02) {
		UNPREPARE_VPU_CLK(clk_top_syspll1_d2);
		UNPREPARE_VPU_CLK(clk_top_syspll_d3);
		UNPREPARE_VPU_CLK(clk_top_vcodecpll_d6);
		UNPREPARE_VPU_CLK(clk_top_syspll_d2);
	} else {
		UNPREPARE_VPU_CLK(clk_top_syspll_d3);
		UNPREPARE_VPU_CLK(clk_top_mmpll_d5);
	}
	UNPREPARE_VPU_CLK(clk_ipu);
	UNPREPARE_VPU_CLK(clk_ipu_isp);
	UNPREPARE_VPU_CLK(clk_ipu_jtag);
	UNPREPARE_VPU_CLK(clk_ipu_axi);
	UNPREPARE_VPU_CLK(clk_ipu_ahb);
	UNPREPARE_VPU_CLK(clk_ipu_mm_axi);
	UNPREPARE_VPU_CLK(clk_ipu_cam_axi);
	UNPREPARE_VPU_CLK(clk_ipu_img_axi);
	UNPREPARE_VPU_CLK(clk_mm_ipu);
	UNPREPARE_VPU_CLK(clk_mm_gals_m0_2x);
	UNPREPARE_VPU_CLK(clk_mm_gals_m1_2x);
	UNPREPARE_VPU_CLK(clk_mm_upsz0);
	UNPREPARE_VPU_CLK(clk_mm_upsz1);
	UNPREPARE_VPU_CLK(clk_mm_fifo0);
	UNPREPARE_VPU_CLK(clk_mm_fifo1);
	UNPREPARE_VPU_CLK(clk_mm_smi_common);
	UNPREPARE_VPU_CLK(clk_mm_smi_common_2x);
#undef UNPREPARE_VPU_CLK

	if (reg_vimvo != NULL)
		regulator_put(reg_vimvo);

	if (enable_vimvo_lp_mode(1))
		LOG_ERR("fail to switch vimvo to sleep mode!\n");

	ndelay(95);
}
#endif

irqreturn_t vpu_isr_handler(int irq, void *dev_id)
{
	LOG_DBG("received interrupt\n");
	is_cmd_done = true;
	wake_up_interruptible(&cmd_wait);
	vpu_write_field(FLD_INT_XTENSA, 1);                   /* clear int */

	return IRQ_HANDLED;
}

static bool users_queue_are_empty(void)
{
	struct list_head *head;
	struct vpu_user *user;
	bool is_empty = true;

	mutex_lock(&vpu_dev->user_mutex);
	list_for_each(head, &vpu_dev->user_list)
	{
		user = vlist_node_of(head, struct vpu_user);
		if (!list_empty(&user->enque_list)) {
			is_empty = false;
			break;
		}
	}
	mutex_unlock(&vpu_dev->user_mutex);

	return is_empty;
}

static int vpu_enque_routine_loop(void *arg)
{
	struct list_head *head;
	struct vpu_user *user;
	struct vpu_request *req;
	struct vpu_algo *algo;

	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	for (; !kthread_should_stop();) {
		/* wait for requests if there is no one in user's queue */
		add_wait_queue(&vpu_dev->req_wait, &wait);
		while (1) {
			if (!users_queue_are_empty())
				break;

			if (is_power_dynamic && is_boot_up) {
				/* return 0 if timeout */
				if (!wait_woken(&wait, TASK_INTERRUPTIBLE, msecs_to_jiffies(PWR_KEEP_TIME_MS)))
					vpu_shut_down();
			} else
				wait_woken(&wait, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
		}
		remove_wait_queue(&vpu_dev->req_wait, &wait);

		/* this thread will be stopped if start direct link */
		wait_event_interruptible(lock_wait, !is_locked);

		/* consume the user's queue */
		mutex_lock(&vpu_dev->user_mutex);
		list_for_each(head, &vpu_dev->user_list)
		{
			user = vlist_node_of(head, struct vpu_user);
			mutex_lock(&user->data_mutex);
			/* flush thread will handle the remaining queue if flush*/
			if (user->flush || list_empty(&user->enque_list)) {
				mutex_unlock(&user->data_mutex);
				continue;
			}

			/* get first node from enque list */
			req = vlist_node_of(user->enque_list.next, struct vpu_request);
			list_del_init(vlist_link(req, struct vpu_request));

			user->running = true;
			mutex_unlock(&user->data_mutex);
			/* unlock for avoiding long time locking */
			mutex_unlock(&vpu_dev->user_mutex);

			if (req->algo_id != algo_loaded) {
				if (vpu_find_algo_by_id(req->algo_id, &algo)) {
					req->status = VPU_REQ_STATUS_INVALID;
					LOG_ERR("can not find the specific algo, id=%d\n", req->algo_id);
					goto out;
				}
				if (vpu_hw_load_algo(algo)) {
					LOG_ERR("load algo failed, while enque\n");
					req->status = VPU_REQ_STATUS_FAILURE;
					goto out;
				}
			}

			vpu_hw_enque_request(req);

out:
			mutex_lock(&vpu_dev->user_mutex);
			mutex_lock(&user->data_mutex);
			list_add_tail(vlist_link(req, struct vpu_request), &user->deque_list);
			user->running = false;
			mutex_unlock(&user->data_mutex);

			wake_up_interruptible_all(&user->deque_wait);
			/* leave loop of round-robin */
			if (is_locked)
				break;
		}
		mutex_unlock(&vpu_dev->user_mutex);
		/* release cpu for another operations */
		usleep_range(1, 10);
	}
	return 0;
}

#ifndef MTK_VPU_EMULATOR

static int vpu_map_mva_of_bin(uint64_t bin_pa)
{
	int ret;
	uint32_t mva_reset_vector = VPU_MVA_RESET_VECTOR;
	uint32_t mva_main_program = VPU_MVA_MAIN_PROGRAM;
	struct sg_table *sg;
	const uint64_t size_main_program =
			VPU_SIZE_MAIN_PROGRAM +
			VPU_SIZE_RESERVED_INSTRUCT +
			VPU_SIZE_ALGO_AREA;

	/* 1. map reset vector */
	sg = &sg_reset_vector;
	ret = sg_alloc_table(sg, 1, GFP_KERNEL);
	CHECK_RET("fail to allocate sg table[reset]!\n");

	sg_dma_address(sg->sgl) = bin_pa;
	sg_dma_len(sg->sgl) = VPU_SIZE_RESET_VECTOR;
	ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
			0, sg,
			VPU_SIZE_RESET_VECTOR,
			M4U_PROT_READ | M4U_PROT_WRITE,
			M4U_FLAGS_FIX_MVA, &mva_reset_vector);
	CHECK_RET("fail to allocate mva of reset vecter!\n");

	/* 2. map main program */
	sg = &sg_main_program;
	ret = sg_alloc_table(sg, 1, GFP_KERNEL);
	CHECK_RET("fail to allocate sg table[main]!\n");

	sg_dma_address(sg->sgl) = bin_pa + VPU_OFFSET_MAIN_PROGRAM;
	sg_dma_len(sg->sgl) = size_main_program;
	ret = m4u_alloc_mva(m4u_client, VPU_PORT_OF_IOMMU,
			0, sg,
			size_main_program,
			M4U_PROT_READ | M4U_PROT_WRITE,
			M4U_FLAGS_FIX_MVA, &mva_main_program);
	if (ret) {
		LOG_ERR("fail to allocate mva of main program!\n");
		m4u_dealloc_mva(m4u_client, VPU_PORT_OF_IOMMU, mva_reset_vector);
		goto out;
	}

out:

	return ret;
}

#endif

static int vpu_dump_power(struct seq_file *s, void *unused)
{
	vpu_print_seq(s, "dynamic(rw): %d\n",
			is_power_dynamic);

	vpu_print_seq(s, "dvfs_debug(rw): %d %d %d\n",
			step_vimvo.curr,
			step_dsp.curr,
			step_ipu_if.curr);

	vpu_print_seq(s, "jtag(rw): %d\n", is_jtag_enabled);
	vpu_print_seq(s, "boot_up(r): %d\n", is_boot_up);

	return 0;
}

static int vpu_set_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_dump_power, inode->i_private);
}

static ssize_t vpu_set_power_write(struct file *flip, const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	char *tmp, *token, *cursor;
	int ret, i, cmd, argc;
	unsigned int args[3];

	enum {
		cmd_dynamic,
		cmd_dvfs_debug,
		cmd_jtag_enabled,
	};

	tmp = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = copy_from_user(tmp, buffer, count);
	CHECK_RET("copy_from_user failed, ret=%d\n", ret);

	cursor = tmp;

	/* parse a command */
	token = strsep(&cursor, " ");
	if (strcmp(token, "dynamic") == 0) {
		cmd = cmd_dynamic;
		argc = 1;
	} else if (strcmp(token, "dvfs_debug") == 0) {
		cmd = cmd_dvfs_debug;
		argc = 3;
	} else if (strcmp(token, "jtag") == 0) {
		cmd = cmd_jtag_enabled;
		argc = 1;
	} else {
		ret = -EINVAL;
		LOG_ERR("no power command[%s]!\n", token);
		goto out;
	}

	/* parse arguments */
	for (i = 0; i < argc && (token = strsep(&cursor, " ")); i++) {
		ret = kstrtouint(token, 10, &args[i]);
		CHECK_RET("fail to parse args[%d]\n", i);
	}

	ret = (i != argc) ? -EINVAL : 0;
	CHECK_RET("wrong num of args");

	switch (cmd) {
	case cmd_dynamic:
		vpu_set_power_dynamic(args[0] != 0);
		break;

	case cmd_dvfs_debug:
		/* step of vimvo regulator */
		ret = args[0] >= step_vimvo.max;
		CHECK_RET("the step of vimvo is out-of-bound, max:%d", step_vimvo.max);

		/* step of dsp clock */
		ret = args[1] >= step_dsp.max;
		CHECK_RET("the step of dsp is out-of-bound, max:%d", step_dsp.max);

		/* step of ipu if */
		ret = args[2] >= step_ipu_if.max;
		CHECK_RET("the step of ipu_if is out-of-bound, max:%d", step_ipu_if.max);

		step_vimvo.curr = args[0];
		step_dsp.curr = args[1];
		step_ipu_if.curr = args[2];

		if (is_power_dynamic)
			break;

		vpu_shut_down();
		vpu_boot_up();

		break;

	case cmd_jtag_enabled:
		is_jtag_enabled = args[0];
		ret = vpu_hw_enable_jtag(is_jtag_enabled);

		break;

	}
	ret = count;
out:

	kfree(tmp);
	return ret;
}

const struct file_operations vpu_set_power_fops = {
	.open = vpu_set_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = vpu_set_power_write,
};

int vpu_create_hw_debugfs(void)
{
	struct dentry *debug_file;

	if (IS_ERR_OR_NULL(vpu_dev->debug_root)) {
		LOG_ERR("have no root of debug dir.\n");
		return -ENOENT;
	}

	debug_file = debugfs_create_file("power", 0644, vpu_dev->debug_root, NULL, &vpu_set_power_fops);
	if (IS_ERR_OR_NULL(debug_file))
		LOG_ERR("fail to create debug file[set_power].\n");

	return 0;
}

#ifndef MTK_VPU_FPGA_PORTING
static int vpu_mmdvfs_receive_event(struct mmdvfs_state_change_event *e)
{
	int ret;

	/* Filter the scenario, which is not relative to VPU */
	switch (e->scenario) {
	case SMI_BWC_SCEN_VR:
	case SMI_BWC_SCEN_VR_SLOW:
	case SMI_BWC_SCEN_ICFP:
	case SMI_BWC_SCEN_VSS:
	case SMI_BWC_SCEN_CAM_PV:
	case SMI_BWC_SCEN_CAM_CP:
		break;
	default:
		return 0;
	}

	LOG_INF("receive mmdvfs event, vore=%d, vimvo=%d, vpu=%d, vpu_if=%d",
			e->vcore_vol_step,
			e->vimvo_vol_step,
			e->vpu_clk_step,
			e->vpu_if_clk_step);

	ret = e->vimvo_vol_step >= step_vimvo.max;
	CHECK_RET("the step of vimvo is out-of-bound, max:%d", step_vimvo.max);

	ret = e->vpu_clk_step >= step_dsp.max;
	CHECK_RET("the step of dsp is out-of-bound, max:%d", step_dsp.max);

	ret = e->vpu_if_clk_step >= step_ipu_if.max;
	CHECK_RET("the step of ipu_if is out-of-bound, max:%d", step_ipu_if.max);

	step_vimvo.curr = e->vimvo_vol_step;
	step_dsp.curr = e->vpu_clk_step;
	step_ipu_if.curr = e->vpu_if_clk_step;

out:
	return ret;
}
#endif

int vpu_init_hw(struct vpu_device *device)
{
	int ret;

	m4u_client = m4u_create_client();
	ion_client = ion_client_create(g_ion_device, "vpu");

	mutex_init(&cmd_mutex);
	init_waitqueue_head(&cmd_wait);

	mutex_init(&lock_mutex);
	init_waitqueue_head(&lock_wait);

	mutex_init(&power_mutex);

	vpu_base = device->vpu_base;
	bin_base = device->bin_base;

	vpu_dev = device;

	ret = vpu_prepare_regulator_and_clock(vpu_dev->dev);
	CHECK_RET("fail to prepare regulator or clock!\n");

#ifdef MTK_VPU_EMULATOR
	vpu_request_emulator_irq(device->irq_num, vpu_isr_handler);
#else
	ret = vpu_map_mva_of_bin(device->bin_pa);
	CHECK_RET("fail to map binary data!\n");

	ret = request_irq(device->irq_num, vpu_isr_handler, IRQF_TRIGGER_NONE, "vpu", NULL);
	CHECK_RET("fail to request vpu irq!\n");
#endif

	ret = vpu_create_hw_debugfs();
	CHECK_RET("fail to create hw debugfs!\n");

	enque_task = kthread_create(vpu_enque_routine_loop, NULL, "vpu");
	if (IS_ERR(enque_task)) {
		ret = PTR_ERR(enque_task);
		enque_task = NULL;
		goto out;
	}
	wake_up_process(enque_task);

#ifndef MTK_VPU_FPGA_PORTING
	ret = register_mmdvfs_state_change_cb(MMDVFS_SCEN_VPU, vpu_mmdvfs_receive_event);
	CHECK_RET("fail to register mmdvfs callback!\n");

	/* [WE1] force to low power mode, while boot up */
	ret = enable_vimvo_lp_mode(1);
	CHECK_RET("fail to switch vimvo to low power mode!\n");
#endif

	ret = vpu_alloc_shared_memory(&work_buf, VPU_SIZE_WORK_BUF);
	CHECK_RET("fail to allocate working buffer!\n");
	work_buf_pa = work_buf->pa;
	work_buf_va = work_buf->va;

	/* get sw version */
	sw_ver = mt_get_chip_sw_ver();
#define DECLARE_VPU_STEP(s, m, v0, v1, v2, v3) \
	{ \
		s.curr = m - 1; \
		s.min = 0; \
		s.max = m; \
		s.values[0] = v0; \
		s.values[1] = v1; \
		s.values[2] = v2; \
		s.values[3] = v3; \
	}

	if (sw_ver == CHIP_SW_VER_01) {
		DECLARE_VPU_STEP(step_dsp, 2, 480000, 364000, 0, 0);
		DECLARE_VPU_STEP(step_ipu_if, 2, 480000, 364000, 0, 0);
		DECLARE_VPU_STEP(step_vimvo, 2, 900000, 800000, 0, 0);
	} else if (sw_ver == CHIP_SW_VER_02) {
		DECLARE_VPU_STEP(step_dsp, 4, 546000, 504000, 364000, 274000);
		DECLARE_VPU_STEP(step_ipu_if, 4, 546000, 504000, 364000, 274000);
		DECLARE_VPU_STEP(step_vimvo, 4, 800000, 750000, 700000, 650000);
	} else {
		ret = -EINVAL;
		CHECK_RET("have a wrong software version:%d!\n", sw_ver);
	}
#undef DECLARE_VPU_STEP

	return 0;

out:
	if (work_buf)
		vpu_free_shared_memory(work_buf);

	return ret;
}

int vpu_uninit_hw(void)
{
	if (enque_task) {
		kthread_stop(enque_task);
		enque_task = NULL;
	}

	vpu_unprepare_regulator_and_clock();

	if (work_buf) {
		vpu_free_shared_memory(work_buf);
		work_buf = NULL;
		work_buf_pa = 0;
		work_buf_va = 0;
	}

	if (m4u_client != NULL) {
		m4u_destroy_client(m4u_client);
		m4u_client = NULL;
	}

	if (ion_client != NULL) {
		ion_client_destroy(ion_client);
		ion_client = NULL;
	}
	return 0;
}


static int vpu_check_precond(void)
{
	uint32_t status;
	size_t i;

	/* wait 1 seconds, if not ready or busy */
	for (i = 0; i < 50; i++) {
		status = vpu_read_field(FLD_XTENSA_INFO0);
		switch (status) {
		case VPU_STATE_READY:
		case VPU_STATE_IDLE:
		case VPU_STATE_ERROR:
			return 0;
		case VPU_STATE_NOT_READY:
		case VPU_STATE_BUSY:
			msleep(20);
			break;
		case VPU_STATE_TERMINATED:
			return -EBADFD;
		}
	}
	LOG_ERR("still busy after wait 1 second.\n");
	return -EBUSY;
}

static int vpu_check_postcond(void)
{
	uint32_t status = vpu_read_field(FLD_XTENSA_INFO0);

	switch (status) {
	case VPU_STATE_READY:
	case VPU_STATE_IDLE:
		return 0;
	case VPU_STATE_NOT_READY:
	case VPU_STATE_BUSY:
		return -EIO;
	default:
		return -EINVAL;
	}
}

int vpu_hw_enable_jtag(bool enabled)
{
	int ret;

	ret = mt_set_gpio_mode(GPIO161 | 0x80000000, enabled ? GPIO_MODE_03 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO162 | 0x80000000, enabled ? GPIO_MODE_03 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO163 | 0x80000000, enabled ? GPIO_MODE_03 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO164 | 0x80000000, enabled ? GPIO_MODE_03 : GPIO_MODE_01);
	ret |= mt_set_gpio_mode(GPIO165 | 0x80000000, enabled ? GPIO_MODE_03 : GPIO_MODE_01);

	CHECK_RET("fail to config gpio-jtag2!\n");

	vpu_write_field(FLD_SPNIDEN, enabled);
	vpu_write_field(FLD_SPIDEN, enabled);
	vpu_write_field(FLD_NIDEN, enabled);
	vpu_write_field(FLD_DBG_EN, enabled);

out:
	return ret;
}

int vpu_hw_boot_sequence(void)
{
	int ret;
	uint64_t ptr_ctrl;
	uint64_t ptr_reset;
	uint64_t ptr_axi;

	vpu_trace_begin("vpu_hw_boot_sequence");

	lock_command();

	ptr_ctrl = vpu_base + g_vpu_reg_descs[REG_CTRL].offset;
	ptr_reset = vpu_base + g_vpu_reg_descs[REG_RESET].offset;
	ptr_axi = vpu_base + g_vpu_reg_descs[REG_AXI_DEFAULT0].offset;

	/* 1. write register */
	VPU_SET_BIT(ptr_ctrl, 31);      /* csr_p_debug_enable */
	VPU_SET_BIT(ptr_ctrl, 26);      /* debug interface cock gated enable */

#ifdef VPU_INTERNAL_BOOT
	VPU_SET_BIT(ptr_ctrl, 19);      /* internal_boot_enable */
#endif

	VPU_SET_BIT(ptr_ctrl, 23);      /* RUN_STALL pull up */
	VPU_SET_BIT(ptr_ctrl, 17);      /* pif gated enable */
	VPU_SET_BIT(ptr_reset, 31);     /* SHARE_SRAM_CONFIG: ICache 0-way, IMEM 64 kbyte */
	VPU_SET_BIT(ptr_reset, 30);
	VPU_SET_BIT(ptr_reset, 12);     /* OCD_HALT_ON_RST pull up */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_reset, 12);     /* OCD_HALT_ON_RST pull down */
	VPU_SET_BIT(ptr_reset, 4);      /* B_RST pull up */
	VPU_SET_BIT(ptr_reset, 8);      /* D_RST pull up */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_reset, 4);      /* B_RST pull down */
	VPU_CLR_BIT(ptr_reset, 8);      /* D_RST pull down */
	ndelay(27);                     /* wait for 27ns */

	VPU_CLR_BIT(ptr_ctrl, 17);      /* pif gated disable, to prevent unknown propagate to BUS */

	VPU_SET_BIT(ptr_axi, 12);       /* AXI Request via M4U */
	VPU_SET_BIT(ptr_axi, 17);
	VPU_SET_BIT(ptr_axi, 24);
	VPU_SET_BIT(ptr_axi, 29);

	/* 2. trigger to run */
	vpu_trace_begin("dsp:running");
	VPU_CLR_BIT(ptr_ctrl, 23);      /* RUN_STALL pull down */

	/* 3. wait until done */
	ret = wait_command();
	vpu_trace_end();
	if (ret) {
		vpu_aee("VPU Timeout", "timeout to external boot\n");
		goto out;
	}

	/* 4. check the result of boot sequence */
	ret = vpu_check_postcond();
	CHECK_RET("fail to boot vpu!\n");

out:
	unlock_command();
	vpu_trace_end();
	return ret;
}

int vpu_hw_set_debug(void)
{
	int ret;

	vpu_trace_begin("vpu_hw_set_debug");

	lock_command();

	/* 1. set debug */
	vpu_write_field(FLD_XTENSA_INFO1, VPU_CMD_SET_DEBUG);
	vpu_write_field(FLD_XTENSA_INFO21, work_buf_pa + VPU_OFFSET_LOG);
	vpu_write_field(FLD_XTENSA_INFO22, VPU_SIZE_LOG_BUF);
	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	vpu_write_field(FLD_INT_CTL_XTENSA00, 1);

	/* 3. wait until done */
	ret = wait_command();
	vpu_trace_end();
	CHECK_RET("timeout of set debug\n");

	/* 4. check the result */
	ret = vpu_check_postcond();
	CHECK_RET("fail to set debug!\n");

out:
	unlock_command();
	vpu_trace_end();
	return ret;
}

int vpu_get_name_of_algo(int id, char **name)
{
	int i;
	int tmp = id;
	struct vpu_image_header *header;

	header = (struct vpu_image_header *) (bin_base + (VPU_OFFSET_IMAGE_HEADERS));
	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		if (tmp > header[i].algo_info_count) {
			tmp -= header[i].algo_info_count;
			continue;
		}

		*name = header[i].algo_infos[tmp - 1].name;
		return 0;
	}

	*name = NULL;
	LOG_ERR("algo is not existed, id=%d\n", id);
	return -ENOENT;
}

int vpu_get_entry_of_algo(char *name, int *id, int *mva, int *length)
{
	int i, j;
	int s = 1;
	struct vpu_algo_info *algo_info;
	struct vpu_image_header *header;

	header = (struct vpu_image_header *) (bin_base + (VPU_OFFSET_IMAGE_HEADERS));
	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		for (j = 0; j < header[i].algo_info_count; j++) {
			algo_info = &header[i].algo_infos[j];
			if (strcmp(name, algo_info->name) == 0) {
				*mva = algo_info->offset - VPU_OFFSET_MAIN_PROGRAM + VPU_MVA_MAIN_PROGRAM;
				*length = algo_info->length;
				*id = s;
				return 0;
			}
			s++;
		}
	}

	*id = 0;
	LOG_ERR("algo is not existed, name=%s\n", name);
	return -ENOENT;
};


int vpu_ext_be_busy(void)
{
	int ret;

	lock_command();

	/* 1. write register */
	vpu_write_field(FLD_XTENSA_INFO1, VPU_CMD_EXT_BUSY);            /* command: be busy */
	/* 2. trigger interrupt */
	vpu_write_field(FLD_INT_CTL_XTENSA00, 1);

	/* 3. wait until done */
	ret = wait_command();

	unlock_command();
	return ret;
}

int vpu_boot_up(void)
{
	int ret;

	mutex_lock(&power_mutex);
	if (is_boot_up) {
		mutex_unlock(&power_mutex);
		return 0;
	}

	vpu_trace_begin("vpu_boot_up");

	ret = vpu_enable_regulator_and_clock();
	CHECK_RET("fail to enable regulator or clock\n");

	ret = vpu_hw_boot_sequence();
	CHECK_RET("fail to do boot sequence\n");

	ret = vpu_hw_set_debug();
	CHECK_RET("fail to set debug\n");

	is_boot_up = true;

out:
	vpu_trace_end();
	mutex_unlock(&power_mutex);
	return ret;
}

int vpu_shut_down(void)
{
	int ret;

	mutex_lock(&power_mutex);
	if (!is_boot_up) {
		mutex_unlock(&power_mutex);
		return 0;
	}

	vpu_trace_begin("vpu_shut_down");

	ret = vpu_disable_regulator_and_clock();
	CHECK_RET("fail to disable regulator and clock\n");

	algo_loaded = 0;
	is_boot_up = false;

out:
	vpu_trace_end();
	mutex_unlock(&power_mutex);
	return ret;
}

int vpu_set_power_dynamic(bool enabled)
{
	is_power_dynamic = enabled;

	/* power on immediately if not dynamic, and vice versa */
	if (!is_power_dynamic)
		vpu_boot_up();
	else if (users_queue_are_empty())
		vpu_shut_down();

	return 0;
}

int vpu_hw_load_algo(struct vpu_algo *algo)
{
	int ret;

	/* this is first entry of vpu hw */
	vpu_boot_up();

	/* no need to reload algo if have same loaded algo*/
	if (algo_loaded == algo->id)
		return 0;

	vpu_trace_begin("vpu_hw_load_algo(%d)", algo->id);

	lock_command();
	LOG_DBG("start to load algo\n");

	ret = vpu_check_precond();
	CHECK_RET("have wrong status before do loader!\n");

	/* 1. write register */
	vpu_write_field(FLD_XTENSA_INFO1, VPU_CMD_DO_LOADER);           /* command: d2d */
	vpu_write_field(FLD_XTENSA_INFO12, algo->bin_ptr);              /* binary data's address */
	vpu_write_field(FLD_XTENSA_INFO13, algo->bin_length);           /* binary data's length */
	vpu_write_field(FLD_XTENSA_INFO15, step_dsp.values[step_dsp.curr]);
	vpu_write_field(FLD_XTENSA_INFO16, step_ipu_if.values[step_ipu_if.curr]);

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	vpu_write_field(FLD_INT_CTL_XTENSA00, 1);

	/* 3. wait until done */
	ret = wait_command();
	vpu_trace_end();
	if (ret) {
		vpu_dump_mesg(NULL);
		vpu_aee("VPU Timeout", "timeout to do loader, algo_id=%d\n", algo_loaded);
		goto out;
	}

	/* 4. update the id of loaded algo */
	algo_loaded = algo->id;

out:
	unlock_command();
	vpu_trace_end();
	return ret;
}

int vpu_hw_enque_request(struct vpu_request *request)
{
	int ret, i, j;
	unsigned int arg_bufs = 0;

	vpu_trace_begin("vpu_hw_enque_request(%d)", request->algo_id);

	lock_command();
	LOG_DBG("start to enque request\n");

	ret = vpu_check_precond();
	if (ret) {
		request->status = VPU_REQ_STATUS_BUSY;
		LOG_ERR("error state before enque request!\n");
		goto out;
	}

	arg_bufs = work_buf_pa;
	memcpy((void *) work_buf_va, request->buffers,
			sizeof(struct vpu_buffer) * request->buffer_count);

	if (g_vpu_log_level > 4) {
		struct vpu_buffer *buf;
		struct vpu_plane *plane;

		LOG_DBG("dump request - setting: 0x%x, length: %d\n",
				(uint32_t) request->sett_ptr, request->sett_length);

		for (i = 0; i < request->buffer_count; i++) {
			buf = &request->buffers[i];
			LOG_DBG("  buffer[%d] - port: %d, size: %dx%d, format: %d\n",
					i, buf->port_id, buf->width, buf->height, buf->format);

			for (j = 0; j < buf->plane_count; j++) {
				plane = &buf->planes[j];
				LOG_DBG("    plane[%d] - ptr: 0x%x, length: %d, stride: %d\n",
						j, (uint32_t) plane->ptr, plane->length, plane->stride);
			}
		}
	}

	/* 1. write register */
	vpu_write_field(FLD_XTENSA_INFO1, VPU_CMD_DO_D2D);              /* command: d2d */
	vpu_write_field(FLD_XTENSA_INFO12, request->buffer_count);      /* buffer count */
	vpu_write_field(FLD_XTENSA_INFO13, arg_bufs);                   /* pointer to array of struct vpu_buffer */
	vpu_write_field(FLD_XTENSA_INFO14, request->sett_ptr);          /* pointer to property buffer */
	vpu_write_field(FLD_XTENSA_INFO15, request->sett_length);       /* size of property buffer */

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	vpu_write_field(FLD_INT_CTL_XTENSA00, 1);

	/* 3. wait until done */
	ret = wait_command();
	vpu_trace_end();
	if (ret) {
		request->status = VPU_REQ_STATUS_TIMEOUT;
		vpu_dump_mesg(NULL);
		vpu_aee("VPU Timeout", "timeout to do d2d, algo_id=%d\n", algo_loaded);
		goto out;
	}

	request->status = (vpu_check_postcond()) ? VPU_REQ_STATUS_FAILURE : VPU_REQ_STATUS_SUCCESS;

out:
	unlock_command();
	vpu_trace_end();
	return ret;

}

int vpu_hw_get_algo_info(struct vpu_algo *algo)
{
	int ret = 0;
	int port_count = 0;
	int info_desc_count = 0;
	int sett_desc_count = 0;
	unsigned int ofs_ports, ofs_info, ofs_info_descs, ofs_sett_descs;

	vpu_trace_begin("vpu_hw_get_algo_info(%d)", algo->id);

	lock_command();
	LOG_DBG("start to get algo, algo_id=%d\n", algo->id);

	ret = vpu_check_precond();
	CHECK_RET("have wrong status before get algo!\n");

	ofs_ports = 0;
	ofs_info = sizeof(((struct vpu_algo *)0)->ports);
	ofs_info_descs = ofs_info + algo->info_length;
	ofs_sett_descs = ofs_info_descs + sizeof(((struct vpu_algo *)0)->info_descs);


	/* 1. write register */
	vpu_write_field(FLD_XTENSA_INFO1, VPU_CMD_GET_ALGO);   /* command: get algo */
	vpu_write_field(FLD_XTENSA_INFO6, work_buf_pa + ofs_ports);
	vpu_write_field(FLD_XTENSA_INFO7, work_buf_pa + ofs_info);
	vpu_write_field(FLD_XTENSA_INFO8, algo->info_length);
	vpu_write_field(FLD_XTENSA_INFO10, work_buf_pa + ofs_info_descs);
	vpu_write_field(FLD_XTENSA_INFO12, work_buf_pa + ofs_sett_descs);

	/* 2. trigger interrupt */
	vpu_trace_begin("dsp:running");
	vpu_write_field(FLD_INT_CTL_XTENSA00, 1);

	/* 3. wait until done */
	ret = wait_command();
	vpu_trace_end();
	if (ret) {
		vpu_dump_mesg(NULL);
		vpu_aee("VPU Timeout", "timeout to get algo, algo_id=%d\n", algo_loaded);
		goto out;
	}

	/* 4. get the return value */
	port_count = vpu_read_field(FLD_XTENSA_INFO5);
	info_desc_count = vpu_read_field(FLD_XTENSA_INFO9);
	sett_desc_count = vpu_read_field(FLD_XTENSA_INFO11);
	algo->port_count = port_count;
	algo->info_desc_count = info_desc_count;
	algo->sett_desc_count = sett_desc_count;

	LOG_DBG("end of get algo, port_count=%d\n", port_count);

	/* 5. write back data from working buffer */
	memcpy((void *) algo->ports, (void *) (work_buf_va + ofs_ports),
			sizeof(struct vpu_port) * port_count);
	memcpy((void *) algo->info_ptr, (void *) (work_buf_va + ofs_info),
			algo->info_length);
	memcpy((void *) algo->info_descs, (void *) (work_buf_va + ofs_info_descs),
			sizeof(struct vpu_prop_desc) * info_desc_count);
	memcpy((void *) algo->sett_descs, (void *) (work_buf_va + ofs_sett_descs),
			sizeof(struct vpu_prop_desc) * sett_desc_count);

out:
	unlock_command();
	vpu_trace_end();
	return ret;
}

void vpu_hw_lock(struct vpu_user *user)
{
	if (user->locked)
		LOG_ERR("double locking bug, pid=%d, tid=%d\n",
				user->open_pid, user->open_tgid);
	else {
		mutex_lock(&lock_mutex);
		is_locked = true;
		user->locked = true;
	}
}

void vpu_hw_unlock(struct vpu_user *user)
{
	if (user->locked) {
		is_locked = false;
		user->locked = false;
		wake_up_interruptible(&lock_wait);
		mutex_unlock(&lock_mutex);
	} else
		LOG_ERR("should not unlock while unlocked, pid=%d, tid=%d\n",
				user->open_pid, user->open_tgid);
}

int vpu_alloc_shared_memory(struct vpu_shared_memory **shmem, int size)
{
	int ret;
	struct ion_mm_data mm_data;
	struct ion_sys_data sys_data;
	struct ion_handle *handle = NULL;

	*shmem = kzalloc(sizeof(struct vpu_shared_memory), GFP_KERNEL);
	ret = (*shmem == NULL);
	CHECK_RET("fail to kzalloc 'struct memory'!\n");

	handle = ion_alloc(ion_client, size, 0, ION_HEAP_MULTIMEDIA_MASK, 0);
	ret = (handle == NULL) ? -ENOMEM : 0;
	CHECK_RET("fail to alloc ion buffer, ret=%d\n", ret);
	(*shmem)->handle = (void *) handle;

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER_EXT;
	mm_data.config_buffer_param.kernel_handle = handle;
	mm_data.config_buffer_param.module_id = VPU_PORT_OF_IOMMU;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 1;
	mm_data.config_buffer_param.reserve_iova_start = IOMMU_VA_START;
	mm_data.config_buffer_param.reserve_iova_end = IOMMU_VA_END;
	ret = ion_kernel_ioctl(ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&mm_data);
	CHECK_RET("fail to config ion buffer, ret=%d\n", ret);

	sys_data.sys_cmd = ION_SYS_GET_PHYS;
	sys_data.get_phys_param.kernel_handle = handle;
	sys_data.get_phys_param.phy_addr = VPU_PORT_OF_IOMMU << 24 | ION_FLAG_GET_FIXED_PHYS;
	sys_data.get_phys_param.len = ION_FLAG_GET_FIXED_PHYS;
	ret = ion_kernel_ioctl(ion_client, ION_CMD_SYSTEM, (unsigned long)&sys_data);
	CHECK_RET("fail to get ion phys, ret=%d\n", ret);
	(*shmem)->pa = sys_data.get_phys_param.phy_addr;
	(*shmem)->length = sys_data.get_phys_param.len;

	(*shmem)->va = (uint64_t) ion_map_kernel(ion_client, handle);
	ret = ((*shmem)->va) ? 0 : -ENOMEM;
	CHECK_RET("fail to map va of buffer!\n");

	return 0;

out:
	if (handle)
		ion_free(ion_client, handle);

	if (*shmem) {
		kfree(*shmem);
		*shmem = NULL;
	}

	return ret;
}

void vpu_free_shared_memory(struct vpu_shared_memory *shmem)
{
	struct ion_handle *handle;

	if (shmem == NULL)
		return;

	handle = (struct ion_handle *) shmem->handle;
	if (handle) {
		ion_unmap_kernel(ion_client, handle);
		ion_free(ion_client, handle);
	}

	kfree(shmem);
}

int vpu_dump_register(struct seq_file *s)
{
	int i, j;
	bool first_row_of_field;
	struct vpu_reg_desc *reg;
	struct vpu_reg_field_desc *field;

#define LINE_BAR "  +---------------+------+---+---+-------------------------+----------+\n"
	vpu_print_seq(s, "Base Address: 0x%llx\n", vpu_base);
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-15s|%-6s|%-3s|%-3s|%-25s|%-10s|\n",
				  "Register", "Offset", "MSB", "LSB", "Field", "Value");
	vpu_print_seq(s, LINE_BAR);


	for (i = 0; i < VPU_NUM_REGS; i++) {
		reg = &g_vpu_reg_descs[i];
		first_row_of_field = true;
		for (j = 0; j < VPU_NUM_REG_FIELDS; j++) {
			field = &g_vpu_reg_field_descs[j];
			if (reg->reg != field->reg)
				continue;

			if (first_row_of_field) {
				first_row_of_field = false;
				vpu_print_seq(s, "  |%-15s|0x%-4.4x|%-3d|%-3d|%-25s|0x%-8.8x|\n",
							  reg->name,
							  reg->offset,
							  field->msb,
							  field->lsb,
							  field->name,
							  vpu_read_field((enum vpu_reg_field) j));
			} else {
				vpu_print_seq(s, "  |%-15s|%-6s|%-3d|%-3d|%-25s|0x%-8.8x|\n",
							  "", "",
							  field->msb,
							  field->lsb,
							  field->name,
							  vpu_read_field((enum vpu_reg_field) j));
			}
		}
		vpu_print_seq(s, LINE_BAR);
	}
#undef LINE_BAR

	return 0;
}

int vpu_dump_image_file(struct seq_file *s)
{
	int i, j, id = 1;
	struct vpu_algo_info *algo_info;
	struct vpu_image_header *header;

#define LINE_BAR "  +------+-----+--------------------------------+-----------+----------+\n"
	vpu_print_seq(s, "Base Address[Bin]: 0x%llx\n", bin_base);
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-6s|%-5s|%-32s|%-11s|%-10s|\n",
				  "Header", "Id", "Name", "MVA", "Length");
	vpu_print_seq(s, LINE_BAR);

	for (i = 0; i < VPU_NUMS_IMAGE_HEADER; i++) {
		header = (struct vpu_image_header *) (bin_base + (VPU_OFFSET_IMAGE_HEADERS) +
										 sizeof(struct vpu_image_header) * i);
		for (j = 0; j < header->algo_info_count; j++) {
			algo_info = &header->algo_infos[j];

			vpu_print_seq(s, "  |%-6d|%-5d|%-32s|0x%-9x|0x%-8x|\n",
						  (i + 1),
						  id,
						  algo_info->name,
						  algo_info->offset - VPU_OFFSET_MAIN_PROGRAM + VPU_MVA_MAIN_PROGRAM,
						  algo_info->length);
			id++;
		}
	}

	vpu_print_seq(s, LINE_BAR);
#undef LINE_BAR

#ifdef MTK_VPU_DUMP_BINARY
	{
		uint32_t dump_1k_size = (0x00000400);
		unsigned char *ptr = NULL;

		vpu_print_seq(s, "Reset Vector Data:\n");
		ptr = (unsigned char *) bin_base + VPU_OFFSET_RESET_VECTOR;
		for (i = 0; i < dump_1k_size / 2; i++, ptr++) {
			if (i % 16 == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			vpu_print_seq(s, "%02X ", *ptr);
		}
		vpu_print_seq(s, "\n");
		vpu_print_seq(s, "\n");
		vpu_print_seq(s, "Main Program Data:\n");
		ptr = (unsigned char *) bin_base + VPU_OFFSET_MAIN_PROGRAM;
		for (i = 0; i < dump_1k_size; i++, ptr++) {
			if (i % 16 == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			vpu_print_seq(s, "%02X ", *ptr);
		}
		vpu_print_seq(s, "\n");
	}
#endif

	return 0;
}

int vpu_dump_mesg(struct seq_file *s)
{
	char *ptr = NULL;
	char *log_head;
	char *log_buf = (char *) (work_buf_va + VPU_OFFSET_LOG);


	if (g_vpu_log_level > 8) {
		int i;
		int line_pos = 0;
		char line_buffer[16 + 1] = {0};

		ptr = log_buf;
		vpu_print_seq(s, "Log Buffer:\n");
		for (i = 0; i < VPU_SIZE_LOG_BUF; i++, ptr++) {
			line_pos = i % 16;
			if (line_pos == 0)
				vpu_print_seq(s, "\n%07X0h: ", i / 16);

			line_buffer[line_pos] = isascii(*ptr) && isprint(*ptr) ? *ptr : '.';
			vpu_print_seq(s, "%02X ", *ptr);
			if (line_pos == 15)
				vpu_print_seq(s, " %s", line_buffer);

		}
		vpu_print_seq(s, "\n\n");
	}

	ptr = log_buf;

	/* set the last byte to '\0' */
	*(ptr + VPU_SIZE_LOG_BUF - 1) = '\0';

	/* skip the header part */
	ptr += VPU_SIZE_LOG_HEADER;
	log_head = strchr(ptr, '\0') + 1;

	vpu_print_seq(s, "vpu: print dsp log\n%s%s", log_head, ptr);

	return 0;
}
