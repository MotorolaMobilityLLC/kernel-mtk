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


#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/device.h>       /* needed by device_* */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/vmalloc.h>      /* needed by vmalloc */
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <mt-plat/sync_write.h>
#include "scp_ipi.h"
#include "scp_helper.h"

#define DRAM_BUF_LEN				(1 * 1024 * 1024)
#define SCP_TIMER_TIMEOUT	        (1 * HZ) /* 1 seconds*/
#define ROUNDUP(a, b)		        (((a) + ((b)-1)) & ~((b)-1))
#define PLT_LOG_ENABLE              0x504C5402 /*magic*/
#define SCP_IPI_RETRY_TIMES         (5000)


#define SCP_LOGGER_UT (1)

struct log_ctrl_s {
	unsigned int base;
	unsigned int size;
	unsigned int enable;
	unsigned int info_ofs;
	unsigned int buff_ofs;
	unsigned int buff_size;
};

struct buffer_info_s {
	unsigned int r_pos;
	unsigned int w_pos;
};

typedef struct {
	unsigned int scp_log_dram_addr;
	unsigned int scp_log_buf_addr;
	unsigned int scp_log_start_addr;
	unsigned int scp_log_end_addr;
	unsigned int scp_log_buf_maxlen;
} SCP_LOG_INFO;


static unsigned int scp_A_logger_inited;
static unsigned int scp_B_logger_inited;
static unsigned int scp_A_logger_wakeup_ap;
static unsigned int scp_B_logger_wakeup_ap;
static struct log_ctrl_s *SCP_A_log_ctl;
static struct log_ctrl_s *SCP_B_log_ctl;
static struct buffer_info_s *SCP_A_buf_info;
static struct buffer_info_s *SCP_B_buf_info;
/*static struct timer_list scp_log_timer;*/
static DEFINE_MUTEX(scp_A_log_mutex);
static DEFINE_MUTEX(scp_B_log_mutex);
static DEFINE_SPINLOCK(scp_A_log_buf_spinlock);
static DEFINE_SPINLOCK(scp_B_log_buf_spinlock);
static struct scp_work_struct scp_logger_notify_work[SCP_CORE_TOTAL];

/*scp last log info*/
static unsigned int scp_A_log_dram_addr_last;
static unsigned int scp_A_log_buf_addr_last;
static unsigned int scp_A_log_start_addr_last;
static unsigned int scp_A_log_end_addr_last;
static unsigned int scp_A_log_buf_maxlen_last;
static char *scp_A_last_log;
static unsigned int scp_B_log_dram_addr_last;
static unsigned int scp_B_log_buf_addr_last;
static unsigned int scp_B_log_start_addr_last;
static unsigned int scp_B_log_end_addr_last;
static unsigned int scp_B_log_buf_maxlen_last;
static char *scp_B_last_log;
static wait_queue_head_t scp_A_logwait;
static wait_queue_head_t scp_B_logwait;

static DEFINE_MUTEX(scp_logger_mutex);
static char *scp_last_logger;
static unsigned int scp_log_buf_addr_last[SCP_CORE_TOTAL];
static unsigned int scp_log_start_addr_last[SCP_CORE_TOTAL];
static unsigned int scp_log_end_addr_last[SCP_CORE_TOTAL];
static unsigned int scp_log_buf_maxlen_last[SCP_CORE_TOTAL];
/*global value*/
unsigned int r_pos_debug;
unsigned int log_ctl_debug;

/*
 * get log from scp when received a buf full notify
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_logger_wakeup_handler(int id, void *data, unsigned int len)
{
	pr_debug("[SCP]wakeup by SCP logger\n");
}

/*
 * get log from scp to last_log_buf
 * @param len:  data length
 * @return:     length of log
 */
static size_t scp_A_get_last_log(size_t b_len)
{
	size_t ret = 0;
	unsigned int log_start_idx;
	unsigned int log_end_idx;
	unsigned int update_start_idx;
	char *pre_scp_last_log_buf;
	unsigned char *scp_last_log_buf = (unsigned char *)(SCP_TCM + scp_A_log_buf_addr_last);

	/*pr_debug("[SCP] %s\n", __func__);*/

	if (!scp_A_logger_inited) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return 0;
	}

	/*SCP keep awake */
	scp_awake_lock(SCP_A_ID);

	log_start_idx = readl((void __iomem *)(SCP_TCM + scp_A_log_start_addr_last));
	log_end_idx = readl((void __iomem *)(SCP_TCM + scp_A_log_end_addr_last));

	if (b_len > scp_A_log_buf_maxlen_last) {
		pr_debug("[SCP] b_len %zu > scp_log_buf_maxlen %d\n", b_len, scp_A_log_buf_maxlen_last);
		b_len = scp_A_log_buf_maxlen_last;
	}

	if (log_end_idx >= b_len)
		update_start_idx = log_end_idx - b_len;
	else
		update_start_idx = scp_A_log_buf_maxlen_last - (b_len - log_end_idx) + 1;

	pre_scp_last_log_buf = scp_A_last_log;
	scp_A_last_log = vmalloc(scp_A_log_buf_maxlen_last + 1);

	/* read log from scp buffer */
	ret = 0;
	if (scp_A_last_log) {
		while ((update_start_idx != log_end_idx) && ret < b_len) {
			scp_A_last_log[ret] = scp_last_log_buf[update_start_idx];
			update_start_idx++;
			ret++;
			if (update_start_idx >= scp_A_log_buf_maxlen_last)
				update_start_idx = update_start_idx - scp_A_log_buf_maxlen_last;
		}
	} else {
		/* no buffer, just skip logs*/
		update_start_idx = log_end_idx;
	}
	scp_A_last_log[ret] = '\0';

	/*SCP release awake */
	scp_awake_unlock(SCP_A_ID);
	vfree(pre_scp_last_log_buf);
	return ret;
}

ssize_t scp_A_log_read(char __user *data, size_t len)
{
	unsigned int w_pos, r_pos, datalen;
	char *buf;

	if (!scp_A_logger_inited)
		return 0;

	datalen = 0;

	mutex_lock(&scp_A_log_mutex);

	r_pos = SCP_A_buf_info->r_pos;
	w_pos = SCP_A_buf_info->w_pos;

	if (r_pos == w_pos)
		goto error;

	if (r_pos > w_pos)
		datalen = DRAM_BUF_LEN - r_pos; /* not wrap */
	else
		datalen = w_pos - r_pos;

	if (datalen > len)
		datalen = len;

	/*debug for logger pos fail*/
	r_pos_debug = r_pos;
	log_ctl_debug = SCP_A_log_ctl->buff_ofs;
	if (r_pos >= DRAM_BUF_LEN) {
		pr_err("[SCP] %s(): r_pos >= DRAM_BUF_LEN,%x,%x\n", __func__, r_pos_debug, log_ctl_debug);
		return 0;
	}

	buf = ((char *) SCP_A_log_ctl) + SCP_A_log_ctl->buff_ofs + r_pos;

	len = datalen;
	/*memory copy from log buf*/
	memcpy_fromio(data, buf, len);

	r_pos += datalen;
	if (r_pos >= DRAM_BUF_LEN)
		r_pos -= DRAM_BUF_LEN;

	SCP_A_buf_info->r_pos = r_pos;

error:
	mutex_unlock(&scp_A_log_mutex);

	return datalen;
}

unsigned int scp_A_log_poll(void)
{
	if (!scp_A_logger_inited)
		return 0;

	if (SCP_A_buf_info->r_pos != SCP_A_buf_info->w_pos)
		return POLLIN | POLLRDNORM;

	/*scp_log_timer_add();*/

	return 0;
}


static ssize_t scp_A_log_if_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	ssize_t ret;

	/*pr_debug("[SCP A] scp_A_log_if_read\n");*/

	ret = 0;

	/*if (access_ok(VERIFY_WRITE, data, len))*/
		ret = scp_A_log_read(data, len);

	return ret;
}

static int scp_A_log_if_open(struct inode *inode, struct file *file)
{
	/*pr_debug("[SCP A] scp_A_log_if_open\n");*/
	return nonseekable_open(inode, file);
}

static unsigned int scp_A_log_if_poll(struct file *file, poll_table *wait)
{
	unsigned int ret = 0;

	/* pr_debug("[SCP A] scp_A_log_if_poll\n"); */

	if (!(file->f_mode & FMODE_READ))
		return ret;

	poll_wait(file, &scp_A_logwait, wait);

	ret = scp_A_log_poll();

	return ret;
}
/*
 * ipi send to enable scp logger flag
 */
static unsigned int scp_A_log_enable_set(unsigned int enable)
{
	int ret;
	unsigned int retrytimes;

	if (scp_A_logger_inited) {
		/*
		 *send ipi to invoke scp logger
		 */
		ret = 0;
		enable = (enable) ? 1 : 0;
		retrytimes = SCP_IPI_RETRY_TIMES;
		do {
			ret = scp_ipi_send(IPI_LOGGER_ENABLE, &enable, sizeof(enable), 0, SCP_A_ID);
			if (ret == DONE)
				break;
			retrytimes--;
			udelay(100);
		} while (retrytimes > 0);
		/*
		 *disable/enable logger flag
		 */
		if ((ret == DONE) && (enable == 1))
			SCP_A_log_ctl->enable = 1;
		else if ((ret == DONE) && (enable == 0))
			SCP_A_log_ctl->enable = 0;

		if (ret != DONE) {
			pr_err("[SCP] scp_A_log_enable_set fail ret=%d\n", ret);
			goto error;
		}

	}

error:
	return 0;
}

/*
 *ipi send enable scp logger wake up flag
 */
static unsigned int scp_A_log_wakeup_set(unsigned int enable)
{
	int ret;
	unsigned int retrytimes;

	if (scp_A_logger_inited) {
		/*
		 *send ipi to invoke scp logger
		 */
		ret = 0;
		enable = (enable) ? 1 : 0;
		retrytimes = SCP_IPI_RETRY_TIMES;
		do {
			ret = scp_ipi_send(IPI_LOGGER_WAKEUP, &enable, sizeof(enable), 0, SCP_A_ID);
			if (ret == DONE)
				break;
			retrytimes--;
			udelay(100);
		} while (retrytimes > 0);
		/*
		 *disable/enable logger flag
		 */
		if ((ret == DONE) && (enable == 1))
			scp_A_logger_wakeup_ap = 1;
		else if ((ret == DONE) && (enable == 0))
			scp_A_logger_wakeup_ap = 0;

		if (ret != DONE) {
			pr_err("[SCP] scp_B_log_wakeup_set fail ret=%d\n", ret);
			goto error;
		}

	}

error:
	return 0;
}

/*
 * create device sysfs, scp logger status
 */
static ssize_t scp_A_mobile_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int stat;

	stat = (scp_A_logger_inited && SCP_A_log_ctl->enable) ? 1 : 0;

	return sprintf(buf, "[SCP A] mobile log is %s\n", (stat == 0x1) ? "enabled" : "disabled");
}

static ssize_t scp_A_mobile_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_A_log_mutex);
	scp_A_log_enable_set(enable);
	mutex_unlock(&scp_A_log_mutex);

	return n;
}

DEVICE_ATTR(scp_mobile_log, S_IWUSR | S_IRUGO, scp_A_mobile_log_show, scp_A_mobile_log_store);


/*
 * create device sysfs, scp ADB cmd to set SCP wakeup AP flag
 */
static ssize_t scp_A_wakeup_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int stat;

	stat = (scp_A_logger_inited && scp_A_logger_wakeup_ap) ? 1 : 0;

	return sprintf(buf, "[SCP A] logger wakeup AP is %s\n", (stat == 0x1) ? "enabled" : "disabled");
}

static ssize_t scp_A_wakeup_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_A_log_mutex);
	scp_A_log_wakeup_set(enable);
	mutex_unlock(&scp_A_log_mutex);

	return n;
}

DEVICE_ATTR(scp_A_logger_wakeup_AP, S_IWUSR | S_IRUGO, scp_A_wakeup_show, scp_A_wakeup_store);

/*
 * create device sysfs, scp last log show
 */
static ssize_t scp_A_last_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	scp_A_get_last_log(scp_A_log_buf_maxlen_last);
	return sprintf(buf, "scp_A_log_buf_maxlen=%u, log=%s\n", scp_A_log_buf_maxlen_last, scp_A_last_log);
}
static ssize_t scp_A_last_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	scp_A_get_last_log(scp_A_log_buf_maxlen_last);
	return n;
}

DEVICE_ATTR(scp_A_get_last_log, S_IWUSR | S_IRUGO, scp_A_last_log_show, scp_A_last_log_store);

/*
 * logger UT test
 *
 */
#if SCP_LOGGER_UT
static ssize_t scp_A_mobile_log_UT_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int w_pos, r_pos, datalen;
	char *logger_buf;
	size_t len = 1024;

	if (!scp_A_logger_inited)
		return 0;

	datalen = 0;

	mutex_lock(&scp_A_log_mutex);

	r_pos = SCP_A_buf_info->r_pos;
	w_pos = SCP_A_buf_info->w_pos;

	if (r_pos == w_pos)
		goto error;

	if (r_pos > w_pos)
		datalen = DRAM_BUF_LEN - r_pos; /* not wrap */
	else
		datalen = w_pos - r_pos;

	if (datalen > len)
		datalen = len;

	logger_buf = ((char *) SCP_A_log_ctl) + SCP_A_log_ctl->buff_ofs + r_pos;

	len = datalen;
	/*memory copy from log buf*/
	memcpy_fromio(buf, logger_buf, len);

	r_pos += datalen;
	if (r_pos >= DRAM_BUF_LEN)
		r_pos -= DRAM_BUF_LEN;

	SCP_A_buf_info->r_pos = r_pos;

error:
	mutex_unlock(&scp_A_log_mutex);


	return len;
}

static ssize_t scp_A_mobile_log_UT_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_A_log_mutex);
	scp_A_log_enable_set(enable);
	mutex_unlock(&scp_A_log_mutex);

	return n;
}

DEVICE_ATTR(scp_A_mobile_log_UT, S_IWUSR | S_IRUGO, scp_A_mobile_log_UT_show, scp_A_mobile_log_UT_store);
#endif

/*
 * IPI for logger init
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_A_logger_init_handler(int id, void *data, unsigned int len)
{
	unsigned long flags;
	SCP_LOG_INFO *log_info = (SCP_LOG_INFO *)data;

	pr_debug("[SCP]scp_get_reserve_mem_phys=%llx\n", scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID));
	spin_lock_irqsave(&scp_A_log_buf_spinlock, flags);
	/* sync scp last log information*/
	scp_A_log_dram_addr_last = log_info->scp_log_dram_addr;
	scp_A_log_buf_addr_last = log_info->scp_log_buf_addr;
	scp_A_log_start_addr_last = log_info->scp_log_start_addr;
	scp_A_log_end_addr_last = log_info->scp_log_end_addr;
	scp_A_log_buf_maxlen_last = log_info->scp_log_buf_maxlen;
	scp_log_buf_addr_last[SCP_A_ID] = log_info->scp_log_buf_addr;
	scp_log_buf_maxlen_last[SCP_A_ID] = log_info->scp_log_buf_maxlen;
	scp_log_start_addr_last[SCP_A_ID] = log_info->scp_log_start_addr;
	scp_log_end_addr_last[SCP_A_ID] = log_info->scp_log_end_addr;

	/* setting dram ctrl config to scp*/
	/* scp side get wakelock, AP to write info to scp sram*/
	mt_reg_sync_writel(scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID), (SCP_TCM + scp_A_log_dram_addr_last));

	spin_unlock_irqrestore(&scp_A_log_buf_spinlock, flags);

	/*set a wq to enable scp logger*/
	scp_logger_notify_work[SCP_A_ID].id = SCP_A_ID;
	scp_schedule_work(&scp_logger_notify_work[SCP_A_ID]);
}

/*
 *
 *  SCP B
 *
 */


/*
 * get log from scp to last_log_buf
 * @param len:  data length
 * @return:     length of log
 */
static size_t scp_B_get_last_log(size_t b_len)
{
	size_t ret = 0;
	unsigned int log_start_idx;
	unsigned int log_end_idx;
	unsigned int update_start_idx;
	char *pre_scp_last_log_buf;
	unsigned char *scp_last_log_buf = (unsigned char *)(SCP_TCM + scp_B_log_buf_addr_last);

	/*pr_debug("[SCP] %s\n", __func__);*/

	if (!scp_B_logger_inited) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return 0;
	}

	/*SCP keep awake */
	scp_awake_lock(SCP_B_ID);

	log_start_idx = readl((void __iomem *)(SCP_TCM + scp_B_log_start_addr_last));
	log_end_idx = readl((void __iomem *)(SCP_TCM + scp_B_log_end_addr_last));

	if (b_len > scp_B_log_buf_maxlen_last) {
		pr_debug("[SCP] b_len %zu > scp_log_buf_maxlen %d\n", b_len, scp_B_log_buf_maxlen_last);
		b_len = scp_B_log_buf_maxlen_last;
	}

	if (log_end_idx >= b_len)
		update_start_idx = log_end_idx - b_len;
	else
		update_start_idx = scp_B_log_buf_maxlen_last - (b_len - log_end_idx) + 1;

	pre_scp_last_log_buf = scp_B_last_log;
	scp_B_last_log = vmalloc(scp_B_log_buf_maxlen_last + 1);

	/* read log from scp buffer */
	ret = 0;
	if (scp_B_last_log) {
		while ((update_start_idx != log_end_idx) && ret < b_len) {
			scp_B_last_log[ret] = scp_last_log_buf[update_start_idx];
			update_start_idx++;
			ret++;
			if (update_start_idx >= scp_B_log_buf_maxlen_last)
				update_start_idx = update_start_idx - scp_B_log_buf_maxlen_last;
		}
	} else {
		/* no buffer, just skip logs*/
		update_start_idx = log_end_idx;
	}
	scp_B_last_log[ret] = '\0';

	/*SCP release awake */
	scp_awake_unlock(SCP_B_ID);
	vfree(pre_scp_last_log_buf);
	return ret;
}

ssize_t scp_B_log_read(char __user *data, size_t len)
{
	unsigned int w_pos, r_pos, datalen;
	char *buf;

	if (!scp_B_logger_inited)
		return 0;

	datalen = 0;

	mutex_lock(&scp_B_log_mutex);

	r_pos = SCP_B_buf_info->r_pos;
	w_pos = SCP_B_buf_info->w_pos;

	if (r_pos == w_pos)
		goto error;

	if (r_pos > w_pos)
		datalen = DRAM_BUF_LEN - r_pos; /* not wrap */
	else
		datalen = w_pos - r_pos;

	if (datalen > len)
		datalen = len;

	/*debug for logger pos fail*/
	r_pos_debug = r_pos;
	log_ctl_debug = SCP_B_log_ctl->buff_ofs;
	if (r_pos >= DRAM_BUF_LEN) {
		pr_err("[SCP] %s(): r_pos >= DRAM_BUF_LEN,%x,%x\n", __func__, r_pos_debug, log_ctl_debug);
		return 0;
	}

	buf = ((char *) SCP_B_log_ctl) + SCP_B_log_ctl->buff_ofs + r_pos;

	len = datalen;
	/*memory copy from log buf*/
	memcpy_fromio(data, buf, len);

	r_pos += datalen;
	if (r_pos >= DRAM_BUF_LEN)
		r_pos -= DRAM_BUF_LEN;

	SCP_B_buf_info->r_pos = r_pos;

error:
	mutex_unlock(&scp_B_log_mutex);

	return datalen;
}

unsigned int scp_B_log_poll(void)
{
	if (!scp_B_logger_inited)
		return 0;

	if (SCP_B_buf_info->r_pos != SCP_B_buf_info->w_pos)
		return POLLIN | POLLRDNORM;

	/*scp_log_timer_add();*/

	return 0;
}


static ssize_t scp_B_log_if_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	ssize_t ret;

	/*pr_debug("[SCP B] scp_B_log_if_read\n");*/

	ret = 0;

	/*if (access_ok(VERIFY_WRITE, data, len))*/
		ret = scp_B_log_read(data, len);

	return ret;
}

static int scp_B_log_if_open(struct inode *inode, struct file *file)
{
	/*pr_debug("[SCP B] scp_B_log_if_open\n");*/
	return nonseekable_open(inode, file);
}

static unsigned int scp_B_log_if_poll(struct file *file, poll_table *wait)
{
	unsigned int ret = 0;

	/* pr_debug("[SCP B] scp_B_log_if_poll\n"); */

	if (!(file->f_mode & FMODE_READ))
		return ret;

	poll_wait(file, &scp_B_logwait, wait);

	ret = scp_B_log_poll();

	return ret;
}

/*
 *ipi send enable scp logger flag
 */
static unsigned int scp_B_log_enable_set(unsigned int enable)
{
	int ret;
	unsigned int retrytimes;

	if (scp_B_logger_inited) {
		/*
		 *send ipi to invoke scp logger
		 */
		ret = 0;
		enable = (enable) ? 1 : 0;
		retrytimes = SCP_IPI_RETRY_TIMES;
		do {
			ret = scp_ipi_send(IPI_LOGGER_ENABLE, &enable, sizeof(enable), 0, SCP_B_ID);
			if (ret == DONE)
				break;
			retrytimes--;
			udelay(100);
		} while (retrytimes > 0);
		/*
		 *disable/enable logger flag
		 */
		if ((ret == DONE) && (enable == 1))
			SCP_B_log_ctl->enable = 1;
		else if ((ret == DONE) && (enable == 0))
			SCP_B_log_ctl->enable = 0;

		if (ret != DONE) {
			pr_err("[SCP] scp_B_log_enable_set fail ret=%d\n", ret);
			goto error;
		}

	}

error:
	return 0;
}

/*
 *ipi send enable scp logger wake up flag
 */
static unsigned int scp_B_log_wakeup_set(unsigned int enable)
{
	int ret;
	unsigned int retrytimes;

	if (scp_B_logger_inited) {
		/*
		 *send ipi to invoke scp logger
		 */
		ret = 0;
		enable = (enable) ? 1 : 0;
		retrytimes = SCP_IPI_RETRY_TIMES;
		do {
			ret = scp_ipi_send(IPI_LOGGER_WAKEUP, &enable, sizeof(enable), 0, SCP_B_ID);
			if (ret == DONE)
				break;
			retrytimes--;
			udelay(100);
		} while (retrytimes > 0);
		/*
		 *disable/enable logger flag
		 */
		if ((ret == DONE) && (enable == 1))
			scp_B_logger_wakeup_ap = 1;
		else if ((ret == DONE) && (enable == 0))
			scp_B_logger_wakeup_ap = 0;

		if (ret != DONE) {
			pr_err("[SCP] scp_B_log_wakeup_set fail ret=%d\n", ret);
			goto error;
		}

	}

error:
	return 0;
}

/*
 * create device sysfs, scp logger status
 */
static ssize_t scp_B_mobile_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int stat;

	stat = (scp_B_logger_inited && SCP_B_log_ctl->enable) ? 1 : 0;

	return sprintf(buf, "[SCP B] mobile log is %s\n", (stat == 0x1) ? "enabled" : "disabled");
}

static ssize_t scp_B_mobile_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_B_log_mutex);
	scp_B_log_enable_set(enable);
	mutex_unlock(&scp_B_log_mutex);

	return n;
}

DEVICE_ATTR(scp_B_mobile_log, S_IWUSR | S_IRUGO, scp_B_mobile_log_show, scp_B_mobile_log_store);


/*
 * create device sysfs, scp ADB cmd to set SCP wakeup AP flag
 */
static ssize_t scp_B_wakeup_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int stat;

	stat = (scp_B_logger_inited && scp_B_logger_wakeup_ap) ? 1 : 0;

	return sprintf(buf, "[SCP B] logger wakeup AP is %s\n", (stat == 0x1) ? "enabled" : "disabled");
}

static ssize_t scp_B_wakeup_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_B_log_mutex);
	scp_B_log_wakeup_set(enable);
	mutex_unlock(&scp_B_log_mutex);

	return n;
}

DEVICE_ATTR(scp_B_logger_wakeup_AP, S_IWUSR | S_IRUGO, scp_B_wakeup_show, scp_B_wakeup_store);

/*
 * create device sysfs, scp last log show
 */
static ssize_t scp_B_last_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	scp_B_get_last_log(scp_B_log_buf_maxlen_last);
	return sprintf(buf, "scp_B_log_buf_maxlen=%u, log=%s\n", scp_B_log_buf_maxlen_last, scp_B_last_log);
}
static ssize_t scp_B_last_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	scp_B_get_last_log(scp_B_log_buf_maxlen_last);
	return n;
}

DEVICE_ATTR(scp_B_get_last_log, S_IWUSR | S_IRUGO, scp_B_last_log_show, scp_B_last_log_store);

#if SCP_LOGGER_UT
static ssize_t scp_B_mobile_log_UT_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int w_pos, r_pos, datalen;
	char *logger_buf;
	size_t len = 1024;

	if (!scp_B_logger_inited)
		return 0;

	datalen = 0;

	mutex_lock(&scp_B_log_mutex);

	r_pos = SCP_B_buf_info->r_pos;
	w_pos = SCP_B_buf_info->w_pos;

	if (r_pos == w_pos)
		goto error;

	if (r_pos > w_pos)
		datalen = DRAM_BUF_LEN - r_pos; /* not wrap */
	else
		datalen = w_pos - r_pos;

	if (datalen > len)
		datalen = len;

	logger_buf = ((char *) SCP_B_log_ctl) + SCP_B_log_ctl->buff_ofs + r_pos;

	len = datalen;
	/*memory copy from log buf*/
	memcpy_fromio(buf, logger_buf, len);

	r_pos += datalen;
	if (r_pos >= DRAM_BUF_LEN)
		r_pos -= DRAM_BUF_LEN;

	SCP_B_buf_info->r_pos = r_pos;

error:
	mutex_unlock(&scp_B_log_mutex);


	return len;
}

static ssize_t scp_B_mobile_log_UT_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_B_log_mutex);
	scp_B_log_enable_set(enable);
	mutex_unlock(&scp_B_log_mutex);

	return n;
}

DEVICE_ATTR(scp_B_mobile_log_UT, S_IWUSR | S_IRUGO, scp_B_mobile_log_UT_show, scp_B_mobile_log_UT_store);
#endif

/*
 * IPI handler for logger init
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_B_logger_init_handler(int id, void *data, unsigned int len)
{
	unsigned long flags;
	SCP_LOG_INFO *log_info = (SCP_LOG_INFO *)data;

	pr_debug("[SCP]scp_get_reserve_mem_phys=%llx\n", scp_get_reserve_mem_phys(SCP_B_LOGGER_MEM_ID));
	spin_lock_irqsave(&scp_B_log_buf_spinlock, flags);

	/* sync scp last log information*/
	scp_B_log_dram_addr_last = log_info->scp_log_dram_addr;
	scp_B_log_buf_addr_last = log_info->scp_log_buf_addr;
	scp_B_log_start_addr_last = log_info->scp_log_start_addr;
	scp_B_log_end_addr_last = log_info->scp_log_end_addr;
	scp_B_log_buf_maxlen_last = log_info->scp_log_buf_maxlen;
	scp_log_buf_addr_last[SCP_B_ID] = log_info->scp_log_buf_addr;
	scp_log_buf_maxlen_last[SCP_B_ID] = log_info->scp_log_buf_maxlen;
	scp_log_start_addr_last[SCP_B_ID] = log_info->scp_log_start_addr;
	scp_log_end_addr_last[SCP_B_ID] = log_info->scp_log_end_addr;

	/* setting dram ctrl config to scp*/
	/* scp side get wakelock, AP to write info to scp sram*/
	mt_reg_sync_writel(scp_get_reserve_mem_phys(SCP_B_LOGGER_MEM_ID), (SCP_TCM + scp_B_log_dram_addr_last));

	spin_unlock_irqrestore(&scp_B_log_buf_spinlock, flags);

	/*set a wq to enable scp logger*/
	scp_logger_notify_work[SCP_B_ID].id = SCP_B_ID;
	scp_schedule_work(&scp_logger_notify_work[SCP_B_ID]);

}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_logger_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int magic = 0x5A5A5A5A;
	unsigned int retrytimes;
	unsigned int scp_core_id = sws->id;
	ipi_status ret;
	ipi_id scp_ipi_id;

	if (scp_core_id == SCP_A_ID)
		scp_ipi_id = IPI_LOGGER_INIT_A;
	else
		scp_ipi_id = IPI_LOGGER_INIT_B;

	pr_err("[SCP]scp_logger_notify_ws id=%u\n", scp_ipi_id);
	/*
	 *send ipi to invoke scp logger
	 */
	retrytimes = SCP_IPI_RETRY_TIMES;
	do {
		ret = scp_ipi_send(scp_ipi_id, &magic, sizeof(magic), 0, scp_core_id);
		pr_debug("[SCP]scp_logger_notify_ws ipi ret=%u\n", ret);
		if (ret == DONE)
			break;
		retrytimes--;
		udelay(2000);
	} while (retrytimes > 0);

	/*enable logger flag*/
	if (ret == DONE) {
		if (scp_core_id == SCP_A_ID)
			SCP_A_log_ctl->enable = 1;
		else
			SCP_B_log_ctl->enable = 1;
	} else {
		pr_err("[SCP]logger initial fail, ipi ret=%d\n", ret);
	}

}


/*
 * init scp logger dram ctrl structure
 * @return:     0: success, otherwise: fail
 */
int scp_B_logger_init(phys_addr_t start, phys_addr_t limit)
{
	int last_ofs;

	/*init wait queue*/
	init_waitqueue_head(&scp_B_logwait);
	scp_B_logger_wakeup_ap = 0;

	/*init work queue*/
	INIT_WORK(&scp_logger_notify_work[SCP_B_ID].work, scp_logger_notify_ws);

	/*init dram ctrl table*/
	last_ofs = 0;

	SCP_B_log_ctl = (struct log_ctrl_s *) start;
	SCP_B_log_ctl->base = PLT_LOG_ENABLE; /* magic */
	SCP_B_log_ctl->enable = 0;
	SCP_B_log_ctl->size = sizeof(*SCP_B_log_ctl);

	last_ofs += SCP_B_log_ctl->size;
	SCP_B_log_ctl->info_ofs = last_ofs;

	last_ofs += sizeof(*SCP_B_buf_info);
	last_ofs = ROUNDUP(last_ofs, 4);
	SCP_B_log_ctl->buff_ofs = last_ofs;
	SCP_B_log_ctl->buff_size = DRAM_BUF_LEN;

	SCP_B_buf_info = (struct buffer_info_s *) (((unsigned char *) SCP_B_log_ctl) + SCP_B_log_ctl->info_ofs);
	SCP_B_buf_info->r_pos = 0;
	SCP_B_buf_info->w_pos = 0;

	last_ofs += SCP_B_log_ctl->buff_size;

	if (last_ofs >= limit) {
		pr_err("[SCP]:%s() initial fail, last_ofs=%u, limit=%u\n", __func__, last_ofs, (unsigned int) limit);
		goto error;
	}

	/* init last log buffer*/
	scp_B_last_log = NULL;
	/* register logger ini IPI */
	scp_ipi_registration(IPI_LOGGER_INIT_B, scp_B_logger_init_handler, "loggerB");
	/* register log wakeup IPI */
	scp_ipi_registration(IPI_LOGGER_WAKEUP, scp_logger_wakeup_handler, "log wakeup");

	scp_B_logger_inited = 1;
	return last_ofs;

error:
	scp_B_logger_inited = 0;
	SCP_B_log_ctl = NULL;
	SCP_B_buf_info = NULL;
	return -1;

}



/*
 * init scp logger dram ctrl structure
 * @return:     0: success, otherwise: fail
 */
int scp_logger_init(phys_addr_t start, phys_addr_t limit)
{
	int last_ofs;

	/*init wait queue*/
	init_waitqueue_head(&scp_A_logwait);
	scp_A_logger_wakeup_ap = 0;

	/*init work queue*/
	INIT_WORK(&scp_logger_notify_work[SCP_A_ID].work, scp_logger_notify_ws);

	/*init dram ctrl table*/
	last_ofs = 0;

	SCP_A_log_ctl = (struct log_ctrl_s *) start;
	SCP_A_log_ctl->base = PLT_LOG_ENABLE; /* magic */
	SCP_A_log_ctl->enable = 0;
	SCP_A_log_ctl->size = sizeof(*SCP_A_log_ctl);

	last_ofs += SCP_A_log_ctl->size;
	SCP_A_log_ctl->info_ofs = last_ofs;

	last_ofs += sizeof(*SCP_A_buf_info);
	last_ofs = ROUNDUP(last_ofs, 4);
	SCP_A_log_ctl->buff_ofs = last_ofs;
	SCP_A_log_ctl->buff_size = DRAM_BUF_LEN;

	SCP_A_buf_info = (struct buffer_info_s *) (((unsigned char *) SCP_A_log_ctl) + SCP_A_log_ctl->info_ofs);
	SCP_A_buf_info->r_pos = 0;
	SCP_A_buf_info->w_pos = 0;

	last_ofs += SCP_A_log_ctl->buff_size;

	if (last_ofs >= limit) {
		pr_err("[SCP]:%s() initial fail, last_ofs=%u, limit=%u\n", __func__, last_ofs, (unsigned int) limit);
		goto error;
	}

	/* init last log buffer*/
	scp_A_last_log = NULL;
	/* register logger ini IPI */
	scp_ipi_registration(IPI_LOGGER_INIT_A, scp_A_logger_init_handler, "loggerA");
	/* register log wakeup IPI */
	scp_ipi_registration(IPI_LOGGER_WAKEUP, scp_logger_wakeup_handler, "log wakeup");

	scp_A_logger_inited = 1;
	return last_ofs;

error:
	scp_A_logger_inited = 0;
	SCP_A_log_ctl = NULL;
	SCP_A_buf_info = NULL;
	return -1;

}

const struct file_operations scp_A_log_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_A_log_if_read,
	.open = scp_A_log_if_open,
	.poll = scp_A_log_if_poll,
};

const struct file_operations scp_B_log_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_B_log_if_read,
	.open = scp_B_log_if_open,
	.poll = scp_B_log_if_poll,
};



/*
 * move scp last log from sram to dram
 * NOTE: this function may be blocked
 * @param scp_core_id:  fill scp id to get last log
 */
void scp_crash_log_move_to_buf(scp_core_id scp_id)
{
	int pos;
	unsigned int ret;
	unsigned int length;
	unsigned int log_start_idx;  /*SCP log start pointer */
	unsigned int log_end_idx;    /*SCP log end pointer */
	unsigned int w_pos;          /*buf write pointer*/
	char *pre_scp_logger_buf;
	char *dram_logger_buf;       /*dram buffer*/

	char *crash_message = "****SCP EE LOG DUMP****\n";
	unsigned char *scp_logger_buf = (unsigned char *)(SCP_TCM + scp_log_buf_addr_last[scp_id]);

	if (!scp_A_logger_inited && scp_id == SCP_A_ID) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return;
	}

	if (!scp_B_logger_inited && scp_id == SCP_B_ID) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return;
	}

	/*SCP keep awake */
	mutex_lock(&scp_logger_mutex);
	scp_awake_lock(scp_id);

	log_start_idx = readl((void __iomem *)(SCP_TCM + scp_log_start_addr_last[scp_id]));
	log_end_idx = readl((void __iomem *)(SCP_TCM + scp_log_end_addr_last[scp_id]));

	if (log_end_idx >= log_start_idx)
		length = log_end_idx - log_start_idx;
	else
		length = scp_log_buf_maxlen_last[scp_id] - (log_start_idx - log_end_idx);

	if (length >= scp_log_buf_maxlen_last[scp_id]) {
		pr_err("scp_crash_log_move_to_buf: length >= max\n");
		length = scp_log_buf_maxlen_last[scp_id];
	}

	pre_scp_logger_buf = scp_last_logger;
	scp_last_logger = vmalloc(length + strlen(crash_message) + 1);
	/* read log from scp buffer */
	ret = 0;
	if (scp_last_logger) {
		ret += snprintf(scp_last_logger, strlen(crash_message), crash_message);
		ret--;
		while ((log_start_idx != log_end_idx) && ret <= (length + strlen(crash_message))) {
			scp_last_logger[ret] = scp_logger_buf[log_start_idx];
			log_start_idx++;
			ret++;
			if (log_start_idx >= scp_log_buf_maxlen_last[scp_id])
				log_start_idx = log_start_idx - scp_log_buf_maxlen_last[scp_id];
		}
	} else {
		/* no buffer, just skip logs*/
		log_start_idx = log_end_idx;
	}
	scp_last_logger[ret] = '\0';

	if (ret != 0) {
		/*get buffer w pos*/
		if (scp_id == SCP_A_ID)
			w_pos = SCP_A_buf_info->w_pos;
		else
			w_pos = SCP_B_buf_info->w_pos;

		if (w_pos >= DRAM_BUF_LEN) {
			pr_err("[SCP] %s(): w_pos >= DRAM_BUF_LEN, w_pos=%u", __func__, w_pos);
			return;
		}

		/*copy to dram buffer*/
		if (scp_id == SCP_A_ID)
			dram_logger_buf = ((char *) SCP_A_log_ctl) + SCP_A_log_ctl->buff_ofs + w_pos;
		else
			dram_logger_buf = ((char *) SCP_B_log_ctl) + SCP_B_log_ctl->buff_ofs + w_pos;

		/*memory copy from log buf*/
		pos = 0;
		while ((pos != ret) && pos <= ret) {
			*dram_logger_buf = scp_last_logger[pos];
			pos++;
			w_pos++;
			dram_logger_buf++;
			if (w_pos >= DRAM_BUF_LEN) {
				/*warp*/
				pr_err("scp_crash_log_move_to_buf: dram warp\n");
				w_pos = 0;
				if (scp_id == SCP_A_ID)
					dram_logger_buf = ((char *) SCP_A_log_ctl) + SCP_A_log_ctl->buff_ofs;
				else
					dram_logger_buf = ((char *) SCP_B_log_ctl) + SCP_B_log_ctl->buff_ofs;
			}
		}
		/*update write pointer*/
		if (scp_id == SCP_A_ID)
			SCP_A_buf_info->w_pos = w_pos;
		else
			SCP_B_buf_info->w_pos = w_pos;

	}

	/*SCP release awake */
	scp_awake_unlock(scp_id);
	mutex_unlock(&scp_logger_mutex);
	vfree(pre_scp_logger_buf);
}



/*
 * get log from scp and optionally save it
 * NOTE: this function may be blocked
 * @param scp_core_id:  fill scp id to get last log
 */
void scp_get_log(scp_core_id scp_id)
{
	pr_debug("[SCP] %s\n", __func__);
#if SCP_LOGGER_ENABLE
	/*move last log to dram*/
	scp_crash_log_move_to_buf(scp_id);
#endif
}

/*
 * return scp last log
 */
char *scp_get_last_log(scp_core_id id)
{
	char *last_log;

	if (id == SCP_A_ID)
		last_log = scp_A_last_log;
	else
		last_log = scp_B_last_log;

	return last_log;
}
