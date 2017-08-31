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

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#define CONFIG_MTKDCS_DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <mach/emi_mpu.h>
#include <mt-plat/mtk_meminfo.h>
#include "mtkdcs_drv.h"

/* Memory lowpower private header file */
#include "../internal.h"

static enum dcs_status sys_dcs_status = DCS_NORMAL;
static int dcs_initialized;
static int normal_channel_num;
static int lowpower_channel_num;
static DEFINE_MUTEX(dcs_mutex);

static char * const dcs_status_name[DCS_NR_STATUS] = {
	"normal",
	"low power",
	"busy",
};

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include "sspm_ipi.h"
static unsigned int dcs_recv_data[4];

static int dcs_get_status_ipi(enum dcs_status *sys_dcs_status)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[32];

	ipi_buf[0] = IPI_DCS_GET_MODE;

	err = sspm_ipi_send_sync(IPI_ID_DCS, 1, (void *)ipi_buf, 0, &ipi_data_ret);

	if (err) {
		pr_err("[%s:%d]ipi_write error: %d\n", __func__, __LINE__, err);
		return -EBUSY;
	}

	*sys_dcs_status = (ipi_data_ret) ? DCS_LOWPOWER : DCS_LOWPOWER;

	return 0;
}

static int dcs_migration_ipi(enum migrate_dir dir)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[32];

	ipi_buf[0] = IPI_DCS_MIGRATION;
	ipi_buf[1] = dir;

	err = sspm_ipi_send_sync(IPI_ID_DCS, 1, (void *)ipi_buf, 0, &ipi_data_ret);

	if (err) {
		pr_err("[%s:%d]ipi_write error: %d\n", __func__, __LINE__, err);
		return -EBUSY;
	}

	return 0;
}

static int dcs_set_dummy_write_ipi(void)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[32];

	ipi_buf[0] = IPI_DCS_SET_DUMMY_WRITE;
	ipi_buf[1] = 0;
	ipi_buf[2] = 0x200000;
	ipi_buf[3] = 0x000000;
	ipi_buf[4] = 0x300000;
	ipi_buf[5] = 0x000000;

	err = sspm_ipi_send_sync(IPI_ID_DCS, 1, (void *)ipi_buf, 0, &ipi_data_ret);

	if (err) {
		pr_err("[%s:%d]ipi_write error: %d\n", __func__, __LINE__, err);
		return -EBUSY;
	}

	return 0;
}

static int dcs_dump_reg_ipi(void)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[32];

	ipi_buf[0] = IPI_DCS_DUMP_REG;

	err = sspm_ipi_send_sync(IPI_ID_DCS, 1, (void *)ipi_buf, 0, &ipi_data_ret);

	if (err) {
		pr_err("[%s:%d]ipi_write error: %d\n", __func__, __LINE__, err);
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
#else /* !CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static int dcs_get_status_ipi(enum dcs_status *sys_dcs_status) { return 0; }
static int dcs_ipi_register(void) { return 0; }
static int dcs_migration_ipi(enum migrate_dir dir) { return 0; }
static int dcs_set_dummy_write_ipi(void) { return 0; }
static int dcs_dump_reg_ipi(void) { return 0; }
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

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
int dcs_dram_channel_switch(enum dcs_status status)
{
	if (!mutex_trylock(&dcs_mutex))
		return -EBUSY;

	if ((sys_dcs_status < DCS_BUSY) &&
		(status < DCS_BUSY) &&
		(sys_dcs_status != status)) {
		dcs_migration_ipi(status == DCS_NORMAL ? NORMAL : LOWPWR);
		sys_dcs_status = DCS_BUSY;
		mutex_unlock(&dcs_mutex);
	} else {
		pr_info("sys_dcs_status not changed\n");
		mutex_unlock(&dcs_mutex);
		return 0;
	}
	/*
	 * assume we success doing channel switch, the sys_dcs_status
	 * should be updated in the ISR
	 */
	mutex_lock(&dcs_mutex);
	sys_dcs_status = status;
	pr_info("sys_dcs_status=%s\n", dcs_status_name[sys_dcs_status]);
	mutex_unlock(&dcs_mutex);
	return 0;
}

/*
 * dcs_get_channel_num_trylock
 * return the number of DRAM channels and get the dcs lock
 * @num: address storing the number of DRAM channels.
 *
 * return 0 on success or error code
 */
int dcs_get_channel_num_trylock(int *num)
{
	if (!mutex_trylock(&dcs_mutex))
		goto BUSY;

	switch (sys_dcs_status) {
	case DCS_NORMAL:
		*num = normal_channel_num;
		break;
	case DCS_LOWPOWER:
		*num = lowpower_channel_num;
		break;
	default:
		mutex_unlock(&dcs_mutex);
		goto BUSY;
	}
	return 0;
BUSY:
	*num = -1;
	return -EBUSY;
}

/*
 * dcs_get_channel_num_unlock
 * unlock the dcs lock
 */
void dcs_get_channel_num_unlock(void)
{
	mutex_unlock(&dcs_mutex);
}

/*
 * dcs_initialied
 *
 * return true if dcs is initialized
 */
bool dcs_initialied(void)
{
	return dcs_initialized;
}

static int __init mtkdcs_init(void)
{
	int ret;

	/* read system dcs status */
	ret = dcs_get_status_ipi(&sys_dcs_status);
	if (!ret)
		pr_info("get init dcs status: %s\n",
			dcs_status_name[sys_dcs_status]);
	else
		return ret;

	/* read number of dram channels */
	normal_channel_num = MAX_CHANNELS;

	/* the channel number must be multiple of 2 */
	if (normal_channel_num % 2) {
		pr_err("%s fail, incorrect normal channel num=%d\n",
				__func__, normal_channel_num);
		dcs_initialized = false;
		return -EINVAL;
	}

	lowpower_channel_num = (normal_channel_num / 2);

	/* register IPI */
	ret = dcs_ipi_register();
	if (ret)
		return -EBUSY;

	dcs_initialized = true;

	return 0;
}

static void __exit mtkdcs_exit(void) { }

device_initcall(mtkdcs_init);
module_exit(mtkdcs_exit);

#ifdef CONFIG_MTKDCS_DEBUG
static int mtkdcs_show(struct seq_file *m, void *v)
{
	int num = 0, ret = -1;

	ret = dcs_get_channel_num_trylock(&num);
	if (!ret) {
		pr_info("dcs currnet channel number=%d\n", num);
		dcs_get_channel_num_unlock();
	} else {
		pr_info("dcs_get_channel_num_trylock busy\n");
	}

	/* call debug ipi */
	dcs_set_dummy_write_ipi();
	dcs_dump_reg_ipi();

	return 0;
}

static int mtkdcs_open(struct inode *inode, struct file *file)
{
	return single_open(file, &mtkdcs_show, NULL);
}

static ssize_t mtkdcs_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	static char state;

	if (count > 0) {
		if (get_user(state, buffer))
			return -EFAULT;
		state -= '0';

		if (state) {
			/* low power to normal */
			pr_alert("state=%d\n", state);
			dcs_dram_channel_switch(DCS_NORMAL);
		} else {
			/* normal to low power */
			pr_alert("state=%d\n", state);
			dcs_dram_channel_switch(DCS_LOWPOWER);
		}
	}

	return count;
}

static const struct file_operations mtkdcs_fops = {
	.open		= mtkdcs_open,
	.write		= mtkdcs_write,
	.read		= seq_read,
	.release	= single_release,
};

static int __init mtkdcs_debug_init(void)
{
	struct dentry *dentry;

	if (!dcs_initialied()) {
		pr_err("mtkdcs is not inited\n");
		return 1;
	}

	dentry = debugfs_create_file("mtkdcs", S_IRUGO, NULL, NULL,
			&mtkdcs_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs mtkdcs file\n");

	return 0;
}

late_initcall(mtkdcs_debug_init);
#endif /* CONFIG_MTKDCS_DEBUG */
