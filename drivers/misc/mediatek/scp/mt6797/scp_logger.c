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

/*
 * TODO: fix index address
 * TODO: add a periodic timer for log read ?
 * TODO: irq mask when get log
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
#include <linux/kfifo.h>
#include <asm/spinlock.h>

#include <mt-plat/sync_write.h>
#include "scp_ipi.h"
#include "scp_helper.h"

#define LOG_TO_AP_UART_LINE 64
#define SCP_FIFO_SIZE 2048

/* This structre need to sync with SCP-side */
typedef struct {
	unsigned int scp_log_buf_addr;
	unsigned int scp_log_start_idx_addr;
	unsigned int scp_log_end_idx_addr;
	unsigned int scp_log_lock_addr;
	unsigned int enable_scp_mobile_log_addr;
	unsigned int scp_log_buf_maxlen;
} SCP_LOG_INFO;

struct scp_log_buffer {
	unsigned int len;
	unsigned int maxlen;
	unsigned char buffer[];
};

static unsigned int scp_log_buf_addr;
static unsigned int scp_log_start_idx_addr;
static unsigned int scp_log_end_idx_addr;
static unsigned int scp_log_lock_addr;
static unsigned int scp_log_buf_maxlen;
static unsigned int enable_scp_mobile_log_addr;
static unsigned int scp_mobile_log_ready;
static unsigned int scp_mobile_log_enable;

static struct scp_log_buffer *scp_log_buf;
static struct kfifo scp_buf_fifo;
static DEFINE_SPINLOCK(scp_log_buf_spinlock);
static struct mutex scp_log_write_mutex;
static struct mutex scp_log_read_mutex;
static wait_queue_head_t logwait;
static struct scp_work_struct scp_logger_work, scp_buf_full_work;
static char *last_log;


/*
 * get log from scp to buf
 * @param buf:  the pointer of data destination
 * @param len:  data length
 * @return:     length of log
 */
static size_t scp_get_log_buf(unsigned char *buf, size_t b_len)
{
	size_t ret = 0;
	unsigned int log_start_idx;
	unsigned int log_end_idx;
	unsigned char *__log_buf = (unsigned char *)(SCP_DTCM + scp_log_buf_addr);

	/*pr_debug("[SCP] %s\n", __func__);*/

	if (scp_mobile_log_ready == 0) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return 0;
	}

	/* Lock the log buffer */
	mt_reg_sync_writel(0x1, (SCP_DTCM + scp_log_lock_addr));

	log_start_idx = readl((void __iomem *)(SCP_DTCM + scp_log_start_idx_addr));
	log_end_idx = readl((void __iomem *)(SCP_DTCM + scp_log_end_idx_addr));

	/*
	pr_debug("[SCP] log_start_idx = %d\n", log_start_idx);
	pr_debug("[SCP] log_end_idx = %d\n", log_end_idx);
	pr_debug("[SCP] scp_log_buf_maxlen = %d\n", scp_log_buf_maxlen);
	*/
	if (b_len > scp_log_buf_maxlen) {
		pr_debug("[SCP] b_len %zu > scp_log_buf_maxlen %d\n", b_len, scp_log_buf_maxlen);
		b_len = scp_log_buf_maxlen;
	}

#define LOG_BUF_MASK (scp_log_buf_maxlen - 1)
#define LOG_BUF(idx) (__log_buf[(idx) & LOG_BUF_MASK])

/* Read SCP log */
	ret = 0;
	if (buf) {
		while ((log_start_idx != log_end_idx) && ret < b_len) {
			buf[ret] = LOG_BUF(log_start_idx);
			log_start_idx++;
			ret++;
		}
	} else {
		/* no buffer, just skip logs*/
		log_start_idx = log_end_idx;
	}
	mt_reg_sync_writel(log_start_idx, (SCP_DTCM + scp_log_start_idx_addr));

	/* Unlock the log buffer */
	mt_reg_sync_writel(0x0, (SCP_DTCM + scp_log_lock_addr));

	return ret;
}

/*
 * API for user application, open log dev
 * @param file:
 * @param data:
 * @param len:
 * @param ppos:
 * @return:
 */
static int scp_log_open(struct inode *inode, struct file *file)
{
	pr_debug("[SCP] scp_log_open\n");
	return nonseekable_open(inode, file);
}

/*
 * API for user application, get log from log dev
 * get log from fifo
 * @param file:
 * @param data:
 * @param len:
 * @param ppos:
 * @return:     length of data
 */
static ssize_t scp_log_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	int ret = 0;
	unsigned int copy_size;

	/* pr_debug("[SCP] Enter scp_log_read, len = %d\n", len); */

#ifndef LOG_TO_AP_UART
	/* keep one consumer a time for fifo */
	mutex_lock(&scp_log_read_mutex);
	ret = kfifo_to_user(&scp_buf_fifo, data, len, &copy_size);
	mutex_unlock(&scp_log_read_mutex);

	if (ret)
		return -EFAULT;
#else
	copy_size = 0;
#endif

	/* pr_debug("[SCP] Exit scp_log_read, ret_len = %d\n", ret_len); */

	return copy_size;
}

/*
 * API for user application, poll log dev
 * @param file:
 * @param wait:
 * @return:     status
 */
static unsigned int scp_poll(struct file *file, poll_table *wait)
{
	unsigned int ret = 0;

	/* pr_debug("[SCP] Enter scp_poll\n"); */

	if (!(file->f_mode & FMODE_READ))
		return ret;

	poll_wait(file, &logwait, wait);

	if (kfifo_len(&scp_buf_fifo) > 0)
		ret |= (POLLIN | POLLRDNORM);

	return ret;
}

const struct file_operations scp_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_log_read,
	.open = scp_log_open,
	.poll = scp_poll,
};

/*
 * IPI for log config
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_logger_ipi_handler(int id, void *data, unsigned int len)
{
	SCP_LOG_INFO *log_info = (SCP_LOG_INFO *)data;
	unsigned long flags;

	pr_debug("[SCP] %s\n", __func__);
	spin_lock_irqsave(&scp_log_buf_spinlock, flags);

	scp_log_buf_addr = log_info->scp_log_buf_addr;
	scp_log_start_idx_addr = log_info->scp_log_start_idx_addr;
	scp_log_end_idx_addr = log_info->scp_log_end_idx_addr;
	scp_log_lock_addr = log_info->scp_log_lock_addr;
	enable_scp_mobile_log_addr = log_info->enable_scp_mobile_log_addr;
	scp_log_buf_maxlen = log_info->scp_log_buf_maxlen;

	scp_mobile_log_ready = 1;
	/* for system recovery */
	mt_reg_sync_writel(scp_mobile_log_enable, (SCP_DTCM + enable_scp_mobile_log_addr));

	spin_unlock_irqrestore(&scp_log_buf_spinlock, flags);

	scp_schedule_work(&scp_logger_work);
}

/*
 * allocate a buffer for IPI handler
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_logger_ws(struct work_struct *ws)
{
	struct scp_log_buffer *buf, *prev_buf;
	unsigned long flags;

	/* we don't expect scp_log_buf have old logs */
	buf = kmalloc(sizeof(*buf) + scp_log_buf_maxlen + 1, GFP_KERNEL);
	if (!buf)
		return;
	buf->maxlen = scp_log_buf_maxlen;
	buf->len = 0;

	spin_lock_irqsave(&scp_log_buf_spinlock, flags);

	prev_buf = scp_log_buf;
	scp_log_buf = buf;

	spin_unlock_irqrestore(&scp_log_buf_spinlock, flags);

	kfree(prev_buf);

	pr_debug("[SCP] scp_log_buf_addr = 0x%x\n", scp_log_buf_addr);
	pr_debug("[SCP] scp_log_start_idx_addr = 0x%x\n", scp_log_start_idx_addr);
	pr_debug("[SCP] scp_log_end_idx_addr = 0x%x\n", scp_log_end_idx_addr);
	pr_debug("[SCP] scp_log_lock_addr = 0x%x\n", scp_log_lock_addr);
	pr_debug("[SCP] enable_scp_mobile_log_addr = 0x%x\n", enable_scp_mobile_log_addr);
	pr_debug("[SCP] scp_log_buf_maxlen = 0x%x\n", scp_log_buf_maxlen);
}

/*
 * allocate a new buffer replace the old buffer for IPI handler
 * insert the old buffer into a fifo for read from user, or just print it to UART
 * NOTE: this function may be blocked and should not be called in interrupt context
 */
static void scp_buf_save(void)
{

	struct scp_log_buffer *new_buf, *prev_buf;
	unsigned int avail, i;
	unsigned long flags;

	/*pr_debug("[SCP] %s\n", __func__);*/

	if (scp_mobile_log_ready == 0) {
		pr_err("[SCP] %s(): logger has not been init\n", __func__);
		return;
	}

	avail = scp_log_buf_maxlen;

	prev_buf = NULL;
	new_buf = kmalloc(sizeof(*new_buf) + avail + 1, GFP_KERNEL);

	if (!new_buf)
		return;

	new_buf->maxlen = avail;
	new_buf->len = 0;

	spin_lock_irqsave(&scp_log_buf_spinlock, flags);

	if (scp_log_buf == NULL || scp_log_buf->len > 0) {
		prev_buf = scp_log_buf;
		scp_log_buf = new_buf;
		new_buf = NULL;
	}

	spin_unlock_irqrestore(&scp_log_buf_spinlock, flags);

	if (prev_buf && prev_buf->len > 0) {
		mutex_lock(&scp_log_write_mutex);
#ifndef LOG_TO_AP_UART
		avail = kfifo_avail(&scp_buf_fifo);

		if (avail < prev_buf->len) {
			for (i = 0; i < (prev_buf->len - avail); i++)
				kfifo_skip(&scp_buf_fifo);
		}

		kfifo_in(&scp_buf_fifo, prev_buf->buffer, prev_buf->len);
		memcpy(last_log, prev_buf->buffer, prev_buf->len);
		last_log[prev_buf->len] = '\0';
		/* notify poll */
		wake_up(&logwait);
#else
		/* prepare a buffer to print on AP's uart */
		size_t base, print_len;
		unsigned char *line_buf;

		line_buf = kmalloc(LOG_TO_AP_UART_LINE + 1, GFP_KERNEL);

		base = 0;
		len = prev_buf->len;
		while (len > 0) {
			print_len = (len > LOG_TO_AP_UART_LINE) ? LOG_TO_AP_UART_LINE : len;
			memcpy(line_buf, prev_buf->buffer + base, print_len);
			line_buf[print_len] = '\0';
			pr_debug("[SCP] LOG: %s\n", line_buf);
			base += print_len;
			len -= print_len;
		}

		kfree(line_buf);
#endif
		mutex_unlock(&scp_log_write_mutex);
		kfree(prev_buf);
	}
	kfree(new_buf);
}

/*
 * handler for received log in work queue
 * @param ws:   work struct
 */
static void scp_buf_full_ws(struct work_struct *ws)
{
	scp_buf_save();
}

char *scp_get_last_log(void)
{
	return last_log;
}
/*
 * get log from scp and optionally save it to fifo
 * NOTE: this function may be blocked and should not be called in interrupt context when save = 1
 * @param save: 0: just get log, 1: get log and save to fifo
 */
void scp_get_log(int save)
{
	unsigned long flags;

	/*pr_debug("[SCP] %s\n", __func__);*/

	if (scp_log_buf) {
		spin_lock_irqsave(&scp_log_buf_spinlock, flags);
		scp_log_buf->len = scp_get_log_buf(scp_log_buf->buffer, scp_log_buf->maxlen);
		spin_unlock_irqrestore(&scp_log_buf_spinlock, flags);
	}

	if (save)
		scp_buf_save();
}

/*
 * get log from scp when received a buf full notify
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_buf_full_ipi_handler(int id, void *data, unsigned int len)
{
	/* pr_debug("[SCP] Enter scp_buf_full_ipi_handler\n"); */

	scp_get_log(0);

	scp_schedule_work(&scp_buf_full_work);
	/* pr_debug("[SCP] Exit scp_buf_full_ipi_handler\n"); */
}

static inline ssize_t scp_log_len_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int log_length = 0;

	if (scp_mobile_log_ready)
		log_length = scp_log_buf_maxlen;

	return sprintf(buf, "%08x\n", log_length);
}

static ssize_t scp_log_len_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	/* do nothing */
	return n;
}

DEVICE_ATTR(scp_log_len, 0644, scp_log_len_show, scp_log_len_store);

static inline ssize_t scp_mobile_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	if (scp_mobile_log_enable == 0x0)
		return sprintf(buf, "SCP mobile log is disabled\n");
	else if (scp_mobile_log_enable == 0x1)
		return sprintf(buf, "SCP mobile log is enabled\n");
	else
		return sprintf(buf, "SCP mobile log is in unknown state...\n");
}

static ssize_t scp_mobile_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	scp_mobile_log_enable = enable;

	if (scp_mobile_log_ready)
		mt_reg_sync_writel(scp_mobile_log_enable, (SCP_DTCM + enable_scp_mobile_log_addr));

	return n;
}

DEVICE_ATTR(scp_mobile_log, 0644, scp_mobile_log_show, scp_mobile_log_store);

static ssize_t scp_log_flush_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	scp_get_log(1);
	return n;
}

DEVICE_ATTR(scp_log_flush, 0200, NULL, scp_log_flush_store);

/*
 * init scp logger
 * @return:     0: success, otherwise: fail
 */
int scp_logger_init(void)
{
	mutex_init(&scp_log_write_mutex);
	mutex_init(&scp_log_read_mutex);
	init_waitqueue_head(&logwait);

	if (kfifo_alloc(&scp_buf_fifo, SCP_FIFO_SIZE, GFP_KERNEL) != 0) {
		pr_err("[SCP] allocate log fifo fail\n");
		return -1;
	}

	/* static value init. */
	scp_log_buf = NULL;
	scp_mobile_log_ready = 0;
	scp_mobile_log_enable = 0;

	INIT_WORK(&scp_logger_work.work, scp_logger_ws);
	INIT_WORK(&scp_buf_full_work.work, scp_buf_full_ws);
	/* register logger IPI */
	scp_ipi_registration(IPI_LOGGER, scp_logger_ipi_handler, "logger");
	/* register log buf full IPI */
	scp_ipi_registration(IPI_BUF_FULL, scp_buf_full_ipi_handler, "buf_full");
	/* register memory for storing last log */
	last_log = vmalloc(SCP_FIFO_SIZE + 1);

	return 0;
}

/*
 * mark scp as not ready
 * this will cause kernel not intent to get any log from scp
 */
void scp_logger_stop(void)
{
	scp_mobile_log_ready = 0;
}

/*
 * cleanup scp logger
 */
void scp_logger_cleanup(void)
{
	kfifo_free(&scp_buf_fifo);
	kfree(scp_log_buf);
	vfree(last_log);
}
