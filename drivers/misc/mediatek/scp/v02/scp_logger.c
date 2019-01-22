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
#define SCP_IPI_RETRY_TIMES         (50)


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

	/*TODO: SCP keep awake */

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

	/*TODO: SCP release awake */
	vfree(pre_scp_last_log_buf);
	return ret;
}

ssize_t scp_A_log_read(char __user *data, size_t len)
{
	unsigned long w_pos, r_pos, datalen;
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

	pr_debug("[SCP A] scp_A_log_if_read\n");

	ret = 0;

	/*if (access_ok(VERIFY_WRITE, data, len))*/
		ret = scp_A_log_read(data, len);

	return ret;
}

static int scp_A_log_if_open(struct inode *inode, struct file *file)
{
	pr_debug("[SCP A] scp_A_log_if_open\n");
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
			udelay(1);
		} while ((ret == BUSY) && retrytimes > 0);
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
			udelay(1);
		} while ((ret == BUSY) && retrytimes > 0);
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

DEVICE_ATTR(scp_A_mobile_log, S_IWUSR | S_IRUGO, scp_A_mobile_log_show, scp_A_mobile_log_store);


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
	unsigned long w_pos, r_pos, datalen;
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
	unsigned int retrytimes;
	unsigned int magic = 0x5A5A5A5A;
	SCP_LOG_INFO *log_info = (SCP_LOG_INFO *)data;
	ipi_status ret;


	pr_debug("[SCP]scp_get_reserve_mem_phys=%llx\n", scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID));
	spin_lock_irqsave(&scp_A_log_buf_spinlock, flags);
	/* sync scp last log information*/
	scp_A_log_dram_addr_last = log_info->scp_log_dram_addr;
	scp_A_log_buf_addr_last = log_info->scp_log_buf_addr;
	scp_A_log_start_addr_last = log_info->scp_log_start_addr;
	scp_A_log_end_addr_last = log_info->scp_log_end_addr;
	scp_A_log_buf_maxlen_last = log_info->scp_log_buf_maxlen;

	/* setting dram ctrl config to scp*/
	/*TODO : get scp waklock to write scp sram*/
	mt_reg_sync_writel(scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID), (SCP_TCM + scp_A_log_dram_addr_last));
	spin_unlock_irqrestore(&scp_A_log_buf_spinlock, flags);
	/*
	 *send ipi to invoke scp logger
	 */
	retrytimes = SCP_IPI_RETRY_TIMES;
	do {
		ret = scp_ipi_send(IPI_LOGGER_INIT_A, &magic, sizeof(magic), 0, SCP_A_ID);
		if (ret == DONE)
			break;
		retrytimes--;
		udelay(10);
	} while ((ret == BUSY) && retrytimes > 0);

	/*enable logger flag*/
	if (ret == DONE)
		SCP_A_log_ctl->enable = 1;
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

	/*TODO: SCP keep awake */

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

	/*TODO: SCP release awake */
	vfree(pre_scp_last_log_buf);
	return ret;
}

ssize_t scp_B_log_read(char __user *data, size_t len)
{
	unsigned long w_pos, r_pos, datalen;
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

	pr_debug("[SCP B] scp_B_log_if_read\n");

	ret = 0;

	/*if (access_ok(VERIFY_WRITE, data, len))*/
		ret = scp_B_log_read(data, len);

	return ret;
}

static int scp_B_log_if_open(struct inode *inode, struct file *file)
{
	pr_debug("[SCP B] scp_B_log_if_open\n");
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
			udelay(1);
		} while ((ret == BUSY) && retrytimes > 0);
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
			udelay(1);
		} while ((ret == BUSY) && retrytimes > 0);
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
	unsigned long w_pos, r_pos, datalen;
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
	unsigned int retrytimes;
	unsigned int magic = 0x5A5A5A5A;
	SCP_LOG_INFO *log_info = (SCP_LOG_INFO *)data;
	ipi_status ret;

	pr_debug("[SCP]scp_get_reserve_mem_phys=%llx\n", scp_get_reserve_mem_phys(SCP_B_LOGGER_MEM_ID));
	spin_lock_irqsave(&scp_B_log_buf_spinlock, flags);

	/* sync scp last log information*/
	scp_B_log_dram_addr_last = log_info->scp_log_dram_addr;
	scp_B_log_buf_addr_last = log_info->scp_log_buf_addr;
	scp_B_log_start_addr_last = log_info->scp_log_start_addr;
	scp_B_log_end_addr_last = log_info->scp_log_end_addr;
	scp_B_log_buf_maxlen_last = log_info->scp_log_buf_maxlen;

	/* setting dram ctrl config to scp*/
	/*TODO : get scp waklock to write scp sram*/
	mt_reg_sync_writel(scp_get_reserve_mem_phys(SCP_B_LOGGER_MEM_ID), (SCP_TCM + scp_B_log_dram_addr_last));
	spin_unlock_irqrestore(&scp_B_log_buf_spinlock, flags);
	/*
	 *send ipi to invoke scp logger
	 */
	retrytimes = SCP_IPI_RETRY_TIMES;
	do {
		ret = scp_ipi_send(IPI_LOGGER_INIT_B, &magic, sizeof(magic), 0, SCP_B_ID);
		if (ret == DONE)
			break;
		retrytimes--;
		udelay(10);
	} while ((ret == BUSY) && retrytimes > 0);

	/*enable logger flag*/
	if (ret == DONE)
		SCP_B_log_ctl->enable = 1;
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
