/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/semaphore.h>
#include <linux/atomic.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#undef pr_debug
#define pr_debug printk

#include "sspm_ipi.h"
static atomic_t myipi;
static atomic_t myipi_err;

static struct task_struct *Ripi_plf_task;
static struct task_struct *Ripi_cpu_dvfs_task;
static struct task_struct *Ripi_gpu_dvfs_task;

#if 1
static struct task_struct *Sipi_plf_task;
static struct task_struct *Sipi_cpu_dvfs_task;
static struct task_struct *Sipi_gpu_dvfs_task;
static struct task_struct *Sipi_fhctl_task;
static struct task_struct *Sipi_pmic_task;
static struct task_struct *Sipi_mcdi_task;
static struct task_struct *Sipi_spms_task;
#endif

static const int pin_size[] = { 3, 4, 3, 9, 5, 2, 8 };
static const int pin_mode[] = { 1, 1, 0, 1, 1, 1, 1 };

#define SEND_NUM    (sizeof(pin_size)/sizeof(int))
static atomic_t pass_cnt[SEND_NUM];

static int chkdata(const char *name, struct ipi_action *ipi, uint32_t *adata)
{
	int id, i;
	uint32_t *pwdata;

	id = ipi->id;
	pwdata = (uint32_t *) ipi->data;
	for (i = 0; i < pin_size[id]; i++) {
		if (pwdata[i] != (id + 1 + i))
			break;
	}
	if (i >= pin_size[id]) {
		if (adata != NULL)
			*adata = (((id + 1) << 24) | ((id + 1) << 16) | ((id + 1) << 8) | (id + 1));

		pr_debug("PASS: %s received pin=%d data check OK\n", name, id);
		return 0;

	} else {
		pr_debug("FAIL: %s received pin=%d data check error\n", name, id);
		pr_debug("len=%d data =>\n", pin_size[id]);
		for (i = 0; i < pin_size[id]; i++)
			pr_debug("%08X\n", pwdata[i]);
	}

	if (adata != NULL)
		*adata = 0;

	return -1;
}

int ipi_single_test(int cnt)
{
	int i, j, ret, pass, fail, opts;
	uint32_t retdata, retchk;
	uint32_t ipi_buf[14];
	uint32_t retdata2[2];

	pass = fail = 0;
	pr_debug("Info: IPI (SEND_NUM=%d) single test (cnt=%d)\n", (int)(SEND_NUM), cnt);

	for (i = 0; i < SEND_NUM; i++) {

		if (i == IPI_ID_PMIC)
			continue;

		for (j = 0; j < pin_size[i]; j++)
			ipi_buf[j] = i + 1 + j;

		pr_debug("IPI send: ID=%d (cnt=%d)\n", i, cnt);
		retdata = 0;
		retchk = (((i + 1) << 24) | ((i + 1) << 16) | ((i + 1) << 8) | (i + 1));
		if (pin_mode[i])
			opts = IPI_OPT_POLLING;
		else
			opts = IPI_OPT_WAIT;

		if (pin_size[i])
			ret = sspm_ipi_send_sync(i, opts, (void *)ipi_buf, pin_size[i], &retdata, 1);
		else
			ret = sspm_ipi_send_sync(i, opts, (void *)ipi_buf, 0, NULL, 0);

		if (ret != IPI_DONE) {
			pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
					 i, ret, cnt);
			fail++;
			break;

		} else {
			if (pin_size[i] == 0) {
				pr_debug("PASS: IPI send ID=%d, no retdata (cnt=%d)\n", i, cnt);
				pass++;
			} else if (retdata == retchk) {
				pr_debug("PASS: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
						 retdata, cnt);
				pass++;
			} else {
				pr_debug("FAIL: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
						 retdata, cnt);
				fail++;
				break;
			}
		}
	}
	/* test for IPI_ID_PMIC */
	i = IPI_ID_PMIC;
	for (j = 0; j < pin_size[i]; j++)
		ipi_buf[j] = i + 1 + j;
	ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf, 5, retdata2, 2);
	if (ret != IPI_DONE) {
		pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n", i, ret,
				 cnt);
		fail++;
	} else {
		if ((retdata2[0] == 0x55555555) && (retdata2[1] == 0xAAAAAAAA)) {
			pr_debug("PASS: IPI sendex ID=%d, retdata[0]=%X retdata[1]=%X (cnt=%d)\n",
					 i, retdata2[0], retdata2[1], cnt);
			pass++;
		} else {
			pr_debug("FAIL: IPI sendex ID=%d, retdata[0]=%X retdata[1]=%X (cnt=%d)\n",
					 i, retdata2[0], retdata2[1], cnt);
			fail++;
		}
	}

	if (pass == SEND_NUM) {
		pr_debug("Info: IPI send test ALL PASS (pass=%d, cnt=%d)\n", pass, cnt);
	} else {
		pr_debug("Error: IPI send test pass=%d fail=%d (cnt=%d)\n", pass, fail, cnt);
		atomic_set(&myipi_err, i + 1);
	}

	return 0;
}


int Sipi_plf_thread(void *data)
{
	int i, j, ret;
	uint32_t retdata, retchk;
	uint32_t ipi_buf[3];

	pr_debug("Info: IPI Platform thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_PLATFORM;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			retdata = 0;
			retchk = (((i + 1) << 24) | ((i + 1) << 16) | ((i + 1) << 8) | (i + 1));
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], &retdata, 1);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				if (retdata == retchk) {
					pr_debug("PASS: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_inc(&pass_cnt[i]);
				} else {
					pr_debug("FAIL: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_set(&myipi_err, i + 1);
				}
			}
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int Sipi_cpu_dvfs_thread(void *data)
{
	int i, j, ret;
	uint32_t retdata, retchk;
	uint32_t ipi_buf[4];

	pr_debug("Info: IPI CPU DVFS thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_CPU_DVFS;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			retdata = 0;
			retchk = (((i + 1) << 24) | ((i + 1) << 16) | ((i + 1) << 8) | (i + 1));
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], &retdata, 1);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				if (retdata == retchk) {
					pr_debug("PASS: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_inc(&pass_cnt[i]);
				} else {
					pr_debug("FAIL: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_set(&myipi_err, i + 1);
				}
			}
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}

int Sipi_gpu_dvfs_thread(void *data)
{
	int i, j, ret;
	uint32_t retdata, retchk;
	uint32_t ipi_buf[3];

	pr_debug("Info: IPI GPU DVFS thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_GPU_DVFS;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			retdata = 0;
			retchk = (((i + 1) << 24) | ((i + 1) << 16) | ((i + 1) << 8) | (i + 1));
			ret = sspm_ipi_send_sync(i, IPI_OPT_WAIT, (void *)ipi_buf,
									 pin_size[i], &retdata, 1);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				if (retdata == retchk) {
					pr_debug("PASS: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_inc(&pass_cnt[i]);
				} else {
					pr_debug("FAIL: IPI send ID=%d, retdata=%08X (cnt=%d)\n", i,
							 retdata, atomic_read(&pass_cnt[i]));
					atomic_set(&myipi_err, i + 1);
				}
			}
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int Sipi_fhctl_thread(void *data)
{
	int i, j, ret;
	uint32_t ipi_buf[9];

	pr_debug("Info: IPI FHCTL thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_FHCTL;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], NULL, 0);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				pr_debug("PASS: IPI send ID=%d  (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			}
			atomic_inc(&pass_cnt[i]);
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int Sipi_pmic_thread(void *data)
{
	int i, j, ret;
	uint32_t ipi_buf[5];

	pr_debug("Info: IPI PMIC thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_PMIC;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], NULL, 0);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				pr_debug("PASS: IPI send ID=%d  (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			}
			atomic_inc(&pass_cnt[i]);
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int Sipi_mcdi_thread(void *data)
{
	int i, j, ret;
	unsigned long flags;
	uint32_t ipi_buf[2];

	pr_debug("Info: IPI MCDI thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_MCDI;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			local_irq_save(flags);
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], NULL, 0);
			local_irq_restore(flags);
			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
						 i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				pr_debug("PASS: IPI send ID=%d  (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			}
			atomic_inc(&pass_cnt[i]);
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int Sipi_spms_thread(void *data)
{
	int i, j, ret;
	unsigned long flags;
	uint32_t ipi_buf[8];

	pr_debug("Info: IPI SPM SUSPEND thread...\n");
	do {
		if (atomic_read(&myipi_err) == 0) {
			i = IPI_ID_SPM_SUSPEND;
			for (j = 0; j < pin_size[i]; j++)
				ipi_buf[j] = i + 1 + j;

			pr_debug("IPI send: ID=%d (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			local_irq_save(flags);
			ret = sspm_ipi_send_sync(i, IPI_OPT_POLLING, (void *)ipi_buf,
									 pin_size[i], NULL, 0);
			local_irq_restore(flags);

			if (ret != IPI_DONE) {
				pr_debug("Error: sspm_ipi_send_sync id=%d failed ret=%d (cnt=%d)\n",
							i, ret, atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, i + 1);
			} else {
				pr_debug("PASS: IPI send ID=%d  (cnt=%d)\n", i, atomic_read(&pass_cnt[i]));
			}
				atomic_inc(&pass_cnt[i]);
		} else {	/* error happened! wait for stop */
			msleep(1000);
		}
	} while (!kthread_should_stop());

	return 0;
}


int ipi_test_thread(void *data)
{
	int i = 0;
	int pass_chk[SEND_NUM];

	atomic_set(&myipi_err, 0);
	pr_debug("Info: IPI test thread begin...(myipi=%d)\n", atomic_read(&myipi));

	if (atomic_read(&myipi) == 1) {
		ipi_single_test(0);
	} else if (atomic_read(&myipi) == 2) {
		do {
			ipi_single_test(i++);
			msleep(1000);
		} while ((atomic_read(&myipi_err) == 0) && !kthread_should_stop());
	} else if (atomic_read(&myipi) == 3) {
		for (i = 0; i < SEND_NUM; i++)
			atomic_set(&pass_cnt[i], 0);

		Sipi_plf_task = kthread_run(Sipi_plf_thread, data, "Sipi_plf_thread");
		Sipi_cpu_dvfs_task = kthread_run(Sipi_cpu_dvfs_thread, data, "Sipi_cpu_dvfs_thread");
		Sipi_gpu_dvfs_task = kthread_run(Sipi_gpu_dvfs_thread, data, "Sipi_gpu_dvfs_thread");
		Sipi_fhctl_task = kthread_run(Sipi_fhctl_thread, data, "Sipi_fhctl_thread");
		Sipi_pmic_task = kthread_run(Sipi_pmic_thread, data, "Sipi_pmic_thread");

		Sipi_mcdi_task = kthread_run(Sipi_mcdi_thread, data, "Sipi_mcdi_thread");
		Sipi_spms_task = kthread_run(Sipi_spms_thread, data, "Sipi_spms_thread");

		do {
			msleep(10000);
			for (i = 0; i < SEND_NUM; i++) {
				if (pass_chk[i] == atomic_read(&pass_cnt[i]))
					break;

				pass_chk[i] = atomic_read(&pass_cnt[i]);
			}
			if (i < SEND_NUM) {
				pr_debug("Error: ID=%d (SEND_NUM=%d) stop test? (pass_chk=%d, pass_cnt=%d)\n",
						 i, (int)(SEND_NUM), pass_chk[i], atomic_read(&pass_cnt[i]));
				atomic_set(&myipi_err, 100);
			}
		} while ((atomic_read(&myipi_err) == 0) && (!kthread_should_stop()));

		kthread_stop(Sipi_plf_task);
		kthread_stop(Sipi_cpu_dvfs_task);
		kthread_stop(Sipi_gpu_dvfs_task);
		kthread_stop(Sipi_fhctl_task);
		kthread_stop(Sipi_pmic_task);

		kthread_stop(Sipi_mcdi_task);
		kthread_stop(Sipi_spms_task);
	}

	pr_debug("Info: IPI test thread end...(myipi_err=%d)\n", atomic_read(&myipi_err));
	atomic_set(&myipi, 0);
	return 0;
}

static struct ipi_action plf_act, cpudvfs_act, gpudvfs_act;
static uint32_t plf_buf[3], cpudvfs_buf[4], gpudvfs_buf[3];

int Ripi_plf_thread(void *data)
{
	int i, ret;
	uint32_t adata;

	pr_debug("Platform received thread\n");

	plf_act.data = (void *)plf_buf;
	ret = sspm_ipi_recv_registration(IPI_ID_PLATFORM, &plf_act);
	if (ret != 0) {
		pr_debug("Error: ipi_recv_registration Platform error: %d\n", ret);
		do {
			msleep(1000);
		} while (!kthread_should_stop());
		return (-1);
	}

	/* an endless loop in which we are doing our work */
	i = 0;
	do {
		i++;
		sspm_ipi_recv_wait(IPI_ID_PLATFORM);
		pr_debug("Info: Platform thread received ID=%d, i=%d\n", plf_act.id, i);
		chkdata("PLF", &plf_act, &adata);
		pr_debug("Info: Platform Task send back ack data=%08X\n", adata);
		ret = sspm_ipi_send_ack(IPI_ID_PLATFORM, &adata);
		if (ret != 0)
			pr_debug("Error: ipi_send_ack Platform error: %d\n", ret);

	} while (!kthread_should_stop());
	return 0;
}


int Ripi_cpu_dvfs_test_thread(void *data)
{
	spinlock_t cpudvfs_lock;
	unsigned long flags;
	int j, ret;
	int chk;
	uint32_t rdata[4];

	pr_debug("CPU DVFS received thread\n");

	spin_lock_init(&cpudvfs_lock);
	cpudvfs_act.data = (void *)cpudvfs_buf;
	ret = sspm_ipi_recv_registration_ex(IPI_ID_CPU_DVFS, &cpudvfs_lock, &cpudvfs_act);
	if (ret != 0) {
		pr_debug("Error: ipi_recv_registration CPU DVFS error: %d\n", ret);
		do {
			msleep(1000);
		} while (!kthread_should_stop());
		return (-1);
	}

	/* an endless loop in which we are doing our work */
	j = 0;
	do {
		j++;
		sspm_ipi_recv_wait(IPI_ID_CPU_DVFS);
		spin_lock_irqsave(&cpudvfs_lock, flags);
		memcpy(rdata, cpudvfs_buf, sizeof(rdata));
		spin_unlock_irqrestore(&cpudvfs_lock, flags);
		pr_debug("Info: CPU DVFS thread received ID=%d, j=%d\n", cpudvfs_act.id, j);
		for (chk = 1; chk < 4; chk++) {
			if (rdata[chk] != rdata[0])
				break;
		}
		if (chk < 4) {
			pr_debug("FAIL: CPU DVFS rdata=%d:%d:%d:%d\n", rdata[0], rdata[1], rdata[2], rdata[3]);
			atomic_set(&myipi_err, 101);
		} else {
			pr_debug("PASS: CPU DVFS rdata=%d:%d:%d:%d\n", rdata[0], rdata[1], rdata[2], rdata[3]);
		}

		if (rdata[0] >= 10)
			j = 0;
	} while (!kthread_should_stop());
	return 0;
}

int Ripi_gpu_dvfs_test_thread(void *data)
{
	int i, ret;
	uint32_t adata;

	pr_debug("GPU DVFS received thread\n");

	gpudvfs_act.data = (void *)gpudvfs_buf;
	ret = sspm_ipi_recv_registration(IPI_ID_GPU_DVFS, &gpudvfs_act);
	if (ret != 0) {
		pr_debug("Error: ipi_recv_registration GPU DVFS error: %d\n", ret);
		do {
			msleep(1000);
		} while (!kthread_should_stop());
		return (-1);
	}

	/* an endless loop in which we are doing our work */
	i = 0;
	do {
		i++;
		sspm_ipi_recv_wait(IPI_ID_GPU_DVFS);
		pr_debug("Info: GPU DVFS thread received ID=%d, i=%d\n", gpudvfs_act.id, i);
		chkdata("GPU DVFS", &gpudvfs_act, &adata);
		pr_debug("Info: GPU DVFS Task send back ack data=%08X\n", adata);
		ret = sspm_ipi_send_ack(IPI_ID_GPU_DVFS, &adata);
		if (ret != 0)
			pr_debug("Error: ipi_send_ack GPU DVFS error: %d\n", ret);

	} while (!kthread_should_stop());
	return 0;
}

#if 1
static ssize_t ver_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR(ver, 0444, ver_show, NULL);

#define IPI_VERSION	"1.0.2"
static ssize_t ver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%s\n", IPI_VERSION);
	mutex_unlock(&dev->mutex);
	return i;
}
#endif

static ssize_t ml_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ml_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(myipi, 0664, ml_show, ml_store);

static ssize_t ml_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	mutex_lock(&dev->mutex);
	i = snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&myipi));
	mutex_unlock(&dev->mutex);
	return i;
}

static struct task_struct *ipi_test_task;
static ssize_t ml_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value && atomic_read(&myipi)) {
		pr_debug("Error: IPI is in testing, myipi=%d\n", atomic_read(&myipi));
		pr_debug("     : Please write 0 into myipi to stop testing\n");
		return -EINVAL;
	}
	mutex_lock(&dev->mutex);

	switch (value) {
	case 0:
		if (atomic_read(&myipi) != 0) {
			pr_debug("Info: IPI is in testing, myipi=%d\n", atomic_read(&myipi));
			pr_debug("Info: stop test thread...\n");
			kthread_stop(ipi_test_task);
			pr_debug("Info: stop test thread...done\n");
		} else {
			pr_debug("Error: IPI isn't in testing\n");
		}
		break;
	case 1: /* single loop test for all pins  */
	case 2: /* multi loop test for all pins   */
	case 3: /* multi thread test for all pins */
		atomic_set(&myipi, value);
		ipi_test_task = kthread_run(ipi_test_thread, dev, "ipi_test_thread");
		break;
	default:
			pr_debug("Error: write unhandled value:%d\n", value);
	}
	mutex_unlock(&dev->mutex);
	return count;
}

static const struct file_operations ipi_file_ops = {
	.owner = THIS_MODULE
};

struct miscdevice ipi_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ipi",
	.mode = 0664,
	.fops = &ipi_file_ops
};

/* load the module */
static int __init ipi_test_init_module(void)
{
	int ret = 0;

	ret = misc_register(&ipi_device);
	if (ret != 0) {
		pr_err("misc register failed\n");
		return ret;
	}

	ret = device_create_file(ipi_device.this_device, &dev_attr_ver);
	if (ret != 0) {
		pr_err("can not create device file: ver\n");
		return ret;
	}

	ret = device_create_file(ipi_device.this_device, &dev_attr_myipi);
	if (ret != 0) {
		pr_err("can not create device file: myipi\n");
		return ret;
	}

	Ripi_plf_task = kthread_run(Ripi_plf_thread, NULL, "ipi_plf_rtask");
	Ripi_cpu_dvfs_task = kthread_run(Ripi_cpu_dvfs_test_thread, NULL, "ipi_cpu_dvfs_rtask");
	Ripi_gpu_dvfs_task = kthread_run(Ripi_gpu_dvfs_test_thread, NULL, "ipi_gpu_dvfs_rtask");
	return 0;
}

/* remove the module */
static void __exit ipi_test_cleanup_module(void)
{

	device_remove_file(ipi_device.this_device, &dev_attr_myipi);

	misc_deregister(&ipi_device);

	/* terminate the kernel threads */
	if (Ripi_plf_task) {
		kthread_stop(Ripi_plf_task);
		Ripi_plf_task = NULL;
	}
	if (Ripi_cpu_dvfs_task) {
		kthread_stop(Ripi_cpu_dvfs_task);
		Ripi_cpu_dvfs_task = NULL;
	}
	if (Ripi_gpu_dvfs_task) {
		kthread_stop(Ripi_gpu_dvfs_task);
		Ripi_gpu_dvfs_task = NULL;
	}
}

module_init(ipi_test_init_module);
module_exit(ipi_test_cleanup_module);
