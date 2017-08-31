/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <mach/emi_mpu.h>
#include <mach/fliper.h>
#include <mt-plat/mtk_meminfo.h>
#include "mtkdcs_drv.h"
#include <mtk_spm_vcore_dvfs.h>
#include <mtk_vcorefs_manager.h>
#include <mtkdcs_drv.h>
#include <smi_public.h>
#include <internal.h>

static enum dcs_status sys_dcs_status = DCS_NORMAL;
static bool dcs_core_initialized;
static bool dcs_full_initialized;
static int normal_channel_num;
static int lowpower_channel_num;
static struct rw_semaphore dcs_rwsem;
static DEFINE_MUTEX(dcs_kicker_lock);
static unsigned long dcs_kicker;
static struct task_struct *dcs_thread;
static u64 lbw_start = ULONG_MAX, lbw_end;
static phys_addr_t mpu_start, mpu_end;
static DEFINE_MUTEX(dcs_lbw_lock);
static struct wake_lock dcs_wake_lock;
static unsigned int nr_swap;
#define DCS_PROFILE

#ifdef DCS_PROFILE
struct perf {
	/*
	 * latest_async_time is not very accurate
	 * sinece the entry_perf allows multiple callers
	 * at the same time. It should be used in a
	 * single user environment.
	 */
	unsigned long long latest_async_time; /* not accurate */
	unsigned long long latest_time;
	unsigned long long max_time;
};
static struct perf perf;
static unsigned long long async_start;
#endif

static char * const __dcs_status_name[DCS_NR_STATUS] = {
	"normal",
	"low power",
	"busy",
};

enum dcs_sysfs_mode {
	DCS_SYSFS_MODE_START,
	DCS_SYSFS_ALWAYS_NORMAL = DCS_SYSFS_MODE_START,
	DCS_SYSFS_ALWAYS_LOWPOWER,
	DCS_SYSFS_FREERUN,
	DCS_SYSFS_FREERUN_NORMAL,
	DCS_SYSFS_FREERUN_LOWPOWER,
	DCS_SYSFS_FREERUN_ASYNC_NORMAL,
	DCS_SYSFS_FREERUN_ASYNC_EXIT_NORMAL,
	DCS_SYSFS_WFD_KICKER,
	DCS_SYSFS_EXIT_WFD_KICKER,
	DCS_SYSFS_VENC_KICKER,
	DCS_SYSFS_EXIT_VENC_KICKER,
	DCS_SYSFS_CAMERA_KICKER,
	DCS_SYSFS_EXIT_CAMERA_KICKER,
	DCS_SYSFS_NR_MODE,
};

enum dcs_sysfs_mode dcs_sysfs_mode = DCS_SYSFS_ALWAYS_NORMAL;

static char * const dcs_sysfs_mode_name[DCS_SYSFS_NR_MODE] = {
	"always normal",
	"always lowpower",
	"freerun only",
	"freerun normal",
	"freerun lowpower",
	"freerun async normal",
	"freerun async exit normal",
	"wfd", /* DCS_SYSFS_WFD_KICKER */
	"exit wfd", /* DCS_SYSFS_EXIT_WFD_KICKER */
	"venc", /* DCS_SYSFS_VENC_KICKER */
	"exit venc", /* DCS_SYSFS_EXIT_VENC_KICKER */
	"camera", /* DCS_SYSFS_CAMERA_KICKER */
	"exit camera", /* DCS_SYSFS_EXIT_CAMERA_KICKER */
};

/*
 * dcs_status_name
 * return the status name for the given dcs status
 * @status: the dcs status
 *
 * return the pointer of the name or NULL for invalid status
 */
char * const dcs_status_name(enum dcs_status status)
{
	if (status < DCS_NR_STATUS)
		return __dcs_status_name[status];
	else
		return NULL;
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include "sspm_ipi.h"
static unsigned int dcs_recv_data[4];

static int dcs_get_status_ipi(enum dcs_status *dcs_status)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[6];

	ipi_buf[0] = IPI_DCS_GET_MODE;

	err = sspm_ipi_send_sync_new(IPI_ID_DCS, IPI_OPT_POLLING, (void *)ipi_buf, 6,
			&ipi_data_ret, 1);

	if (err) {
		pr_err("[%s:%d]ipi_write error: %d\n", __func__, __LINE__, err);
		return -EBUSY;
	}

	if (ipi_data_ret == -1)
		return -EBUSY;

	*dcs_status = (ipi_data_ret) ? DCS_LOWPOWER : DCS_NORMAL;

	return 0;
}

static int dcs_migration_ipi(enum migrate_dir dir)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[6];

	ipi_buf[0] = IPI_DCS_MIGRATION;
	ipi_buf[1] = dir;

	err = sspm_ipi_send_sync_new(IPI_ID_DCS, IPI_OPT_POLLING, (void *)ipi_buf, 6,
			&ipi_data_ret, 1);

	if (err) {
		pr_err("[%d]ipi_write error: %d\n", __LINE__, err);
		return -EBUSY;
	}

	return 0;
}

static int dcs_set_dummy_write_ipi(void)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[6];

	ipi_buf[0] = IPI_DCS_SET_DUMMY_WRITE;
	ipi_buf[1] = 0;
	ipi_buf[2] = 0x200000;
	ipi_buf[3] = 0x000000;
	ipi_buf[4] = 0x300000;
	ipi_buf[5] = 0x000000;

	err = sspm_ipi_send_sync_new(IPI_ID_DCS, IPI_OPT_POLLING, (void *)ipi_buf, 6,
			&ipi_data_ret, 1);

	if (err) {
		pr_err("[%d]ipi_write error: %d\n", __LINE__, err);
		return -EBUSY;
	}

	return 0;
}

static int dcs_dump_reg_ipi(void)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[6];

	ipi_buf[0] = IPI_DCS_DUMP_REG;

	err = sspm_ipi_send_sync_new(IPI_ID_DCS, IPI_OPT_POLLING, (void *)ipi_buf, 6,
			&ipi_data_ret, 1);

	if (err) {
		pr_err("[%d]ipi_write error: %d\n", __LINE__, err);
		return -EBUSY;
	}

	return 0;
}

static int dcs_ipi_register(void)
{
	int ret;
	int retry = 0;
	struct ipi_action dcs_isr;

	dcs_isr.data = (void *)dcs_recv_data;

	do {
		ret = sspm_ipi_recv_registration(IPI_ID_DCS, &dcs_isr);
	} while ((ret != IPI_REG_OK) && (retry++ < 10));

	if (ret != IPI_REG_OK) {
		pr_err("dcs_ipi_register fail\n");
		return -EBUSY;
	}

	return 0;
}

/*
 * __dcs_dram_channel_switch
 *
 * Do the channel switch operation
 * Callers must hold write semaphore: dcs_rwsem
 *
 * return 0 on success or error code
 */
static int __dcs_dram_channel_switch(enum dcs_status status)
{
	int err;
	int retry = 100;
#ifdef DCS_PROFILE
	unsigned long long start, t, now;
#endif

	if ((sys_dcs_status < DCS_BUSY) &&
		(status < DCS_BUSY) &&
		(sys_dcs_status != status)) {
		/* speed up lpdma, use max DRAM frequency */
		vcorefs_request_dvfs_opp(KIR_DCS, OPP_0);
		/* enable SMI outstanding */
		smi_bus_enable(SMI_LARB_MMSYS0, "DCS");
		smi_common_ostd_setting(1);

#ifdef DCS_PROFILE
		start = sched_clock();
#endif
		pr_info("dcs migration start\n");
		err = dcs_migration_ipi(status == DCS_NORMAL ? NORMAL : LOWPWR);
		if (err) {
			pr_err("[%d]ipi_write error: %d\n",
					__LINE__, err);
			sys_dcs_status = DCS_BUSY;
			BUG(); /* fatal error */
			return -EBUSY;
		}
		/* poll status */
		mdelay(700);
		while (retry > 0) {
			err = dcs_get_status_ipi(&sys_dcs_status);
			if (!err)
				break;
			pr_warn("dcs get status (%d)\n", retry);
			mdelay(100);
			retry--;
		}
		if (retry == 0)
			BUG();
#ifdef DCS_PROFILE
		now = sched_clock();
		t = now - start;
		if (t > perf.max_time)
			perf.max_time = t;
		perf.latest_time = t;
		if (status == DCS_NORMAL) {
			/* we only care about the switch to normal time */
			t = now - async_start;
			perf.latest_async_time = t;
		} else
			perf.latest_async_time = 0;
#endif
		if (sys_dcs_status != status)
			pr_warn("status not changed. (%s should be %s\n",
					dcs_status_name(sys_dcs_status),
					dcs_status_name(status));
		pr_info("dcs migration end\n");
		pr_info("sys_dcs_status=%s\n", dcs_status_name(sys_dcs_status));
		nr_swap++;

		/* notify bwm */
		notify_bwm_dcs(status == DCS_NORMAL ?
				normal_channel_num : lowpower_channel_num);
		/* disable SMI outstanding */
		smi_common_ostd_setting(0);
		smi_bus_disable(SMI_LARB_MMSYS0, "DCS");
		/* release DRAM frequency */
		vcorefs_request_dvfs_opp(KIR_DCS, OPP_UNREQ);
		/* update DVFSRC setting */
		spm_dvfsrc_set_channel_bw(status == DCS_NORMAL ?
				DVFSRC_CHANNEL_4 : DVFSRC_CHANNEL_2);
	} else {
		pr_info("sys_dcs_status not changed\n");
	}

	return 0;
}
#else /* !CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static int dcs_get_status_ipi(enum dcs_status *sys_dcs_status)
{
	*sys_dcs_status = DCS_NORMAL;
	return 0;
}
static int dcs_set_dummy_write_ipi(void) { return 0; }
static int dcs_dump_reg_ipi(void) { return 0; }
static int dcs_ipi_register(void) { return 0; }
static int dcs_force_acc_low_ipi(int enable) { return 0; }
static int __dcs_dram_channel_switch(enum dcs_status status) { return 0; }
#endif /* end of CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

/*
 * dcs_dram_channel_switch
 *
 * Send a IPI call to SSPM to perform dynamic channel switch.
 * The dynamic channel switch only performed in stable status.
 * i.e., DCS_NORMAL or DCS_LOWPOWER.
 * @status: channel mode
 *
 * return 0 on success or error code
 */
static int dcs_dram_channel_switch(enum dcs_status status)
{
	int ret = 0;

	if (!dcs_core_initialized || !dcs_full_initialized)
		return -ENODEV;

	wake_lock(&dcs_wake_lock);
	down_write(&dcs_rwsem);

	if (dcs_sysfs_mode == DCS_SYSFS_FREERUN)
		ret = __dcs_dram_channel_switch(status);

	up_write(&dcs_rwsem);
	wake_unlock(&dcs_wake_lock);

	return ret;
}

/*
 * __dcs_enter_perf
 * mark the kicker status and switch to full bandwidth mode
 * callers must hold dcs_kicker_lock
 * @kicker: the kicker who enters performance mode
 *
 * return 0 on success or error code
 */
int __dcs_enter_perf(enum dcs_kicker kicker)
{
	dcs_kicker |= (1 << kicker);

	pr_info("[%d]dcs_kicker=%08lx\n", __LINE__, dcs_kicker);

	/* wakeup thread */
	pr_info("wakeup dcs_thread\n");
#ifdef DCS_PROFILE
	async_start = sched_clock();
#endif
	if (!wake_up_process(dcs_thread))
		pr_info("dcs_thread is already running\n");

	return 0;
}

/*
 * dcs_enter_perf
 * mark the kicker status and switch to full bandwidth mode
 * @kicker: the kicker who enters performance mode
 *
 * return 0 on success or error code
 */
int dcs_enter_perf(enum dcs_kicker kicker)
{
	if (!dcs_core_initialized)
		return -ENODEV;
	if (kicker >= DCS_NR_KICKER)
		return -EINVAL;

	mutex_lock(&dcs_kicker_lock);
	__dcs_enter_perf(kicker);
	mutex_unlock(&dcs_kicker_lock);

	return 0;
}

/*
 * __dcs_exit_perf
 * unset the kicker status
 * callers must hold dcs_kicker_lock
 * @kicker: the kicker who exits performance mode
 *
 * return 0 on success or error code
 */
int __dcs_exit_perf(enum dcs_kicker kicker)
{
	dcs_kicker &= ~(1 << kicker);

	pr_info("[%d]dcs_kicker=%08lx\n", __LINE__, dcs_kicker);

	return 0;
}

/*
 * dcs_exit_perf
 * unset the kicker status
 * @kicker: the kicker who exits performance mode
 *
 * return 0 on success or error code
 */
int dcs_exit_perf(enum dcs_kicker kicker)
{
	if (!dcs_core_initialized)
		return -ENODEV;
	if (kicker >= DCS_NR_KICKER)
		return -EINVAL;

	mutex_lock(&dcs_kicker_lock);
	__dcs_exit_perf(kicker);
	mutex_unlock(&dcs_kicker_lock);

	return 0;
}


/*
 * dcs_dram_channel_switch_by_sysfs_mode
 *
 * Update dcs_sysfs_mode and send a IPI call to SSPM to perform
 * dynamic channel switch by the sysfs mode.
 * The dynamic channel switch only performed in stable status.
 * i.e., DCS_NORMAL or DCS_LOWPOWER.
 * @mode: sysfs mode
 *
 * return 0 on success or error code
 */
static int dcs_dram_channel_switch_by_sysfs_mode(enum dcs_sysfs_mode mode)
{
	int ret = 0;

	if (!dcs_core_initialized || !dcs_full_initialized)
		return -ENODEV;

	mutex_lock(&dcs_kicker_lock);
	wake_lock(&dcs_wake_lock);
	ret = down_write_trylock(&dcs_rwsem);
	if (!ret) {
		ret = -EBUSY;
		goto out_busy;
	}

	/* only 'always' commands can overwrite 'always' commands */
	if ((dcs_sysfs_mode <= DCS_SYSFS_ALWAYS_LOWPOWER) &&
			mode > DCS_SYSFS_ALWAYS_LOWPOWER)
		goto out;

	dcs_sysfs_mode = mode;
	switch (mode) {
	case DCS_SYSFS_FREERUN_NORMAL:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		/* fallthrough */
	case DCS_SYSFS_ALWAYS_NORMAL:
		ret = __dcs_dram_channel_switch(DCS_NORMAL);
		break;
	case DCS_SYSFS_FREERUN_LOWPOWER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		/* fallthrough */
	case DCS_SYSFS_ALWAYS_LOWPOWER:
		ret = __dcs_dram_channel_switch(DCS_LOWPOWER);
		break;
	case DCS_SYSFS_FREERUN_ASYNC_NORMAL:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_enter_perf(DCS_KICKER_PERF);
		break;
	case DCS_SYSFS_FREERUN_ASYNC_EXIT_NORMAL:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_exit_perf(DCS_KICKER_PERF);
		break;
	case DCS_SYSFS_WFD_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_enter_perf(DCS_KICKER_WFD);
		break;
	case DCS_SYSFS_EXIT_WFD_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_exit_perf(DCS_KICKER_WFD);
		break;
	case DCS_SYSFS_VENC_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_enter_perf(DCS_KICKER_VENC);
		break;
	case DCS_SYSFS_EXIT_VENC_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_exit_perf(DCS_KICKER_VENC);
		break;
	case DCS_SYSFS_CAMERA_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_enter_perf(DCS_KICKER_CAMERA);
		break;
	case DCS_SYSFS_EXIT_CAMERA_KICKER:
		dcs_sysfs_mode = DCS_SYSFS_FREERUN;
		ret = __dcs_exit_perf(DCS_KICKER_CAMERA);
		break;
	default:
		pr_alert("unknown sysfs mode: %d\n", mode);
		break;
	}

out:
	up_write(&dcs_rwsem);
out_busy:
	wake_unlock(&dcs_wake_lock);
	mutex_unlock(&dcs_kicker_lock);

	return ret;
}

/*
 * dcs_set_lbw_region
 *
 * set low bandwidth region for DCS
 * Some performance-critical memory areas need high
 * bandwidth. DCS can use low bandwidth in the areas in
 * interesting.
 *
 * Ths setting must finish before DCS driver's initialization
 *
 * H/W only support one area so the API may be called multiple
 * times and calculate one area to cover all areas.
 * @start: 64-bit start physical address
 * @end: 64-bit end physical address
 *
 * return 0 on success or error code
 */
int dcs_set_lbw_region(u64 start, u64 end)
{
	/* do not change lbw setting after initialization */
	if (dcs_core_initialized)
		return 0;

	start -= PHYS_OFFSET;
	end -= PHYS_OFFSET;

	/* sanity check */
	if ((start > lbw_start) && (end < lbw_end))
		goto out;

	mutex_lock(&dcs_lbw_lock);
	/* update lbw_[start|end] */
	if (start < lbw_start)
		lbw_start = start;
	if (end > lbw_end)
		lbw_end = end;
	mutex_unlock(&dcs_lbw_lock);

out:
	return 0;
}

/*
 * dcs_switch_to_lowpower
 * try to switch to lowpower mode, every kicker must be unset
 *
 * return 0 on success or error code
 */
int dcs_switch_to_lowpower(void)
{
	int err;

	if (!dcs_core_initialized)
		return -ENODEV;

	mutex_lock(&dcs_kicker_lock);
	pr_info("[%d]dcs_kicker=%08lx\n", __LINE__, dcs_kicker);

	if (!dcs_kicker) {
		err = dcs_dram_channel_switch(DCS_LOWPOWER);
		if (err) {
			pr_err("[%d]fail: %d\n", __LINE__, err);
			mutex_unlock(&dcs_kicker_lock);
			return err;
		}
		pr_info("entering lowpower mode\n");
	} else
		pr_info("not entering lowpower mode\n");

	mutex_unlock(&dcs_kicker_lock);

	return 0;
}

/*
 * dcs_thread_entry
 * A thread performs the channel switch.
 *
 * return 0 on success or error code
 */
static int dcs_thread_entry(void *p)
{
	int err;

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		set_current_state(TASK_RUNNING);
		/* perform channel swtich */
		pr_info("%s waken\n", __func__);
		err = dcs_dram_channel_switch(DCS_NORMAL);
		if (err) {
			pr_err("[%d] fail: %d\n", __LINE__, err);
		}
	} while (1);

	return 0;
}
/*
 * __dcs_get_dcs_status
 * return the number of DRAM channels and status
 * Callers need to hold dcs_resem
 * DO _NOT_ USE THIS API UNLESS YOU KNOW HOW TO USE THIS API
 * @ch: address storing the number of DRAM channels.
 * @dcs_status: address storing the system dcs status
 *
 * return 0 on success or error code
 */
int __dcs_get_dcs_status(int *ch, enum dcs_status *dcs_status)
{
	*dcs_status = sys_dcs_status;

	switch (sys_dcs_status) {
	case DCS_NORMAL:
		*ch = normal_channel_num;
		break;
	case DCS_LOWPOWER:
		*ch = lowpower_channel_num;
		break;
	default:
		pr_err("[%d], incorrect DCS status=%s\n",
				__LINE__,
				dcs_status_name(sys_dcs_status));
		BUG(); /* fatal error */
	}
	return 0;
}

/*
 * dcs_get_dcs_status_lock
 * return the number of DRAM channels and status and get the dcs lock
 * @ch: address storing the number of DRAM channels.
 * @dcs_status: address storing the system dcs status
 *
 * return 0 on success or error code
 */
int dcs_get_dcs_status_lock(int *ch, enum dcs_status *dcs_status)
{
	if (!dcs_core_initialized)
		return -ENODEV;

	down_read(&dcs_rwsem);

	return __dcs_get_dcs_status(ch, dcs_status);
}

/*
 * dcs_get_dcs_status_trylock
 * return the number of DRAM channels and status and trylock dcs lock
 * @ch: address storing the number of DRAM channels, -1 if lock failed
 * @dcs_status: address storing the system dcs status, DCS_BUSY if lock failed
 *
 * return 0 on success or error code
 */
int dcs_get_dcs_status_trylock(int *ch, enum dcs_status *dcs_status)
{
	if (!dcs_core_initialized)
		return -ENODEV;

	if (!down_read_trylock(&dcs_rwsem)) {
		/* lock failed */
		*dcs_status = DCS_BUSY;
		goto BUSY;
	}

	return __dcs_get_dcs_status(ch, dcs_status);

BUSY:
	*ch = -1;
	return -EBUSY;
}

/*
 * dcs_get_dcs_status_unlock
 * unlock the dcs lock
 */
void dcs_get_dcs_status_unlock(void)
{
	if (!dcs_core_initialized)
		return;

	up_read(&dcs_rwsem);
}

/*
 * dcs_initialied
 *
 * return true if dcs is initialized
 */
bool dcs_initialied(void)
{
	return dcs_core_initialized && dcs_full_initialized;
}

/*
 * dcs_full_init
 *
 * Set dcs_full_initialized flag. The last module that DCS driver
 * depends on should call this function.
 * Do the first time of dcs_switch_to_lowpower().
 *
 * return 0 on success or error code
 */
int dcs_full_init(void)
{
	dcs_full_initialized = true;
	dcs_switch_to_lowpower();

	return 0;
}

static int __dcs_mpu_protection_enable(void)
{
	emi_mpu_set_region_protection((unsigned long long)mpu_start,
			(unsigned long long)mpu_end - 1, DCS_MPU_REGION,
			MPU_ACCESS_PERMISSON_FORBIDDEN);

	pr_debug("enable MPU\n");

	/* wait for EMI to consume all transactions in the proection range */
	mdelay(1);

	return 0;
}

static int __dcs_mpu_protection_disable(void)
{
	emi_mpu_set_region_protection((unsigned long long)mpu_start,
			(unsigned long long)mpu_end - 1, DCS_MPU_REGION,
			MPU_ACCESS_PERMISSON_NO_PROTECTION);

	pr_debug("disable MPU\n");

	/* wait for EMI to consume all transactions in the proection range */
	mdelay(1);

	return 0;
}

/*
 * dcs_mpu_protection
 *
 * enable/disable DCS MPU
 * @enable: disable DCS MPU if enable is 0, otherwise enable MPU
 *
 * return 0 on success or error code
 */
int dcs_mpu_protection(int enable)
{
	if (enable)
		__dcs_mpu_protection_enable();
	else
		__dcs_mpu_protection_disable();

	return 0;
}

static ssize_t mtkdcs_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	enum dcs_status dcs_status;
	int n = 0, ch, ret;

	mutex_lock(&dcs_kicker_lock);
	ret = dcs_get_dcs_status_lock(&ch, &dcs_status);

	if (!ret) {
		/*
		 * we're holding the rw_sem, so it's safe to use
		 * dcs_sysfs_mode
		 */
		n += snprintf(buf + n, PAGE_SIZE - n, "dcs_status=%s, channel=%d, dcs_sysfs_mode=%s\n",
				dcs_status_name(dcs_status),
				ch,
				dcs_sysfs_mode_name[dcs_sysfs_mode]);
		n += snprintf(buf + n, PAGE_SIZE - n, "dcs lbw_start=%llx, lbw_end=%llx\n",
				lbw_start, lbw_end);
		n += snprintf(buf + n, PAGE_SIZE - n, "dcs mpu_start=%llx, mpu_end=%llx\n",
				mpu_start, mpu_end);
		n += snprintf(buf + n, PAGE_SIZE - n, "nr_swap=%u\n", nr_swap);
		n += snprintf(buf + n, PAGE_SIZE - n, "kicker=0x%lx\n", dcs_kicker);
		dcs_get_dcs_status_unlock();
	}

	mutex_unlock(&dcs_kicker_lock);

	/* call debug ipi */
	dcs_set_dummy_write_ipi();
	dcs_dump_reg_ipi();

	return n;
}

/*
 * mtkdcs_debug_show
 * debug entry for debug system, do not wait for any lock here
 */
static ssize_t mtkdcs_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	enum dcs_status dcs_status;
	int n = 0, ch;
	int ret_kicker_lock, ret_status_lock;

	ret_kicker_lock = mutex_trylock(&dcs_kicker_lock);
	ret_status_lock = dcs_get_dcs_status_trylock(&ch, &dcs_status);

	/*
	 * We're getting debug information, do not care lock
	 */
	n += snprintf(buf + n, PAGE_SIZE - n, "dcs_kicker_lock %s, dcs_rwsem %s\n",
			ret_kicker_lock ? "ok" : "fail",
			!ret_status_lock ? "ok" : "fail");
	n += snprintf(buf + n, PAGE_SIZE - n, "dcs_status=%s, channel=%d, dcs_sysfs_mode=%s\n",
			dcs_status_name(dcs_status),
			ch,
			dcs_sysfs_mode_name[dcs_sysfs_mode]);
	n += snprintf(buf + n, PAGE_SIZE - n, "dcs lbw_start=%llx, lbw_end=%llx\n",
			lbw_start, lbw_end);
	n += snprintf(buf + n, PAGE_SIZE - n, "dcs mpu_start=%llx, mpu_end=%llx\n",
			mpu_start, mpu_end);
	n += snprintf(buf + n, PAGE_SIZE - n, "nr_swap=%u\n", nr_swap);
	n += snprintf(buf + n, PAGE_SIZE - n, "kicker=0x%lx\n", dcs_kicker);

	if (ret_kicker_lock)
		mutex_unlock(&dcs_kicker_lock);
	if (!ret_status_lock)
		dcs_get_dcs_status_unlock();

	return n;
}
static ssize_t mtkdcs_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	enum dcs_sysfs_mode mode;
	int n = 0;

	n += snprintf(buf + n, PAGE_SIZE - n, "available modes:\n");
	for (mode = DCS_SYSFS_MODE_START; mode < DCS_SYSFS_NR_MODE; mode++)
		n += snprintf(buf + n, PAGE_SIZE - n, "%s\n", dcs_sysfs_mode_name[mode]);

	return n;
}

static ssize_t mtkdcs_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	enum dcs_sysfs_mode mode;
	char *name;

	for (mode = DCS_SYSFS_MODE_START; mode < DCS_SYSFS_NR_MODE; mode++) {
		name = dcs_sysfs_mode_name[mode];
		if (!strncmp(buf, name, strlen(name)))
			goto apply_mode;
	}

	pr_alert("[%d] unknown command: %s\n", __LINE__, buf);
	return n;

apply_mode:

	pr_info("mtkdcs_mode_store cmd=%s", buf);
	dcs_dram_channel_switch_by_sysfs_mode(mode);
	return n;
}

#ifdef DCS_PROFILE
static ssize_t mtkdcs_perf_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int n = 0;

	n += snprintf(buf + n, PAGE_SIZE - n, "latest_ipi=%lluns, max_ipi=%lluns, latest_async=%lluns\n",
			perf.latest_time, perf.max_time,
			perf.latest_async_time);

	return n;
}
#endif

static DEVICE_ATTR(status, S_IRUGO, mtkdcs_status_show, NULL);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, mtkdcs_mode_show, mtkdcs_mode_store);
#ifdef DCS_PROFILE
static DEVICE_ATTR(perf, S_IRUGO, mtkdcs_perf_show, NULL);
#endif
static DEVICE_ATTR(debug, S_IRUGO, mtkdcs_debug_show, NULL);

static struct attribute *mtkdcs_attrs[] = {
	&dev_attr_status.attr,
	&dev_attr_mode.attr,
#ifdef DCS_PROFILE
	&dev_attr_perf.attr,
#endif
	&dev_attr_debug.attr,
	NULL,
};

struct attribute_group mtkdcs_attr_group = {
	.attrs = mtkdcs_attrs,
	.name = "mtkdcs",
};

static int __init mtkdcs_init(void)
{
	int ret;

	/* init rwsem */
	init_rwsem(&dcs_rwsem);

	/* init wakelock */
	wake_lock_init(&dcs_wake_lock, WAKE_LOCK_SUSPEND, "dcs_wakelock");

	/* register IPI */
	ret = dcs_ipi_register();
	if (ret)
		return -EBUSY;

	/* read system dcs status */
	ret = dcs_get_status_ipi(&sys_dcs_status);
	if (!ret)
		pr_info("get init dcs status: %s\n",
			dcs_status_name(sys_dcs_status));
	else
		return ret;

	/* Create SYSFS interface */
	ret = sysfs_create_group(power_kobj, &mtkdcs_attr_group);
	if (ret)
		return ret;

	/* read number of dram channels */
	normal_channel_num = get_emi_channel_number();

	/* the channel number must be multiple of 2 */
	if (normal_channel_num % 2) {
		pr_err("%s fail, incorrect normal channel num=%d\n",
				__func__, normal_channel_num);
		return -EINVAL;
	}

	lowpower_channel_num = (normal_channel_num / 2);

	/* sanity check */
	BUILD_BUG_ON(DCS_NR_KICKER > BITS_PER_LONG);

	/* get mpu region (high address) */
	mpu_start = get_max_DRAM_size() / 2 + PHYS_OFFSET;
	mpu_end = get_max_DRAM_size() + PHYS_OFFSET;

	/* Start a kernel thread */
	dcs_thread = kthread_run(dcs_thread_entry, NULL, "dcs_thread");
	if (IS_ERR(dcs_thread)) {
		pr_err("Failed to start dcs_thread!\n");
		return -ENODEV;
	}

	dcs_core_initialized = true;

#ifdef DCS_SCREENOFF_ONLY_MODE
	ret = dcs_enter_perf(DCS_KICKER_DEBUG);
	if (ret) {
		pr_err("enter perf failed, kick=%d\n", DCS_KICKER_DEBUG);
		return ret;
	}
#endif

	return 0;
}

static void __exit mtkdcs_exit(void) { }

module_init(mtkdcs_init);
module_exit(mtkdcs_exit);
