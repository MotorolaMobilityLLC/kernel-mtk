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
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>

#include <mt-plat/sync_write.h>
#include "scp_ipi.h"
#include "scp_helper.h"

#define DRAM_BUF_LEN				(1 * 1024 * 1024)
#define SCP_TIMER_TIMEOUT	(1 * HZ) /* 1 seconds*/
#define ROUNDUP(a, b)		(((a) + ((b)-1)) & ~((b)-1))
#define PLT_LOG_ENABLE      0x504C5402 /*magic*/

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

static unsigned int scp_logger_inited;
static struct log_ctrl_s *log_ctl;
static struct buffer_info_s *buf_info;
/*static struct timer_list scp_log_timer;*/
static DEFINE_MUTEX(scp_log_mutex);
static DEFINE_SPINLOCK(scp_log_buf_spinlock);

static wait_queue_head_t logwait;

ssize_t scp_log_read(char __user *data, size_t len)
{
	unsigned long w_pos, r_pos, datalen;
	char *buf;

	if (!scp_logger_inited)
		return 0;

	datalen = 0;

	mutex_lock(&scp_log_mutex);

	r_pos = buf_info->r_pos;
	w_pos = buf_info->w_pos;

	if (r_pos == w_pos)
		goto error;

	if (r_pos > w_pos)
		datalen = DRAM_BUF_LEN - r_pos; /* not wrap */
	else
		datalen = w_pos - r_pos;

	if (datalen > len)
		datalen = len;

	buf = ((char *) log_ctl) + log_ctl->buff_ofs + r_pos;

	len = datalen;
	/*memory copy from log buf*/
	memcpy_fromio(data, buf, len);

	r_pos += datalen;
	if (r_pos >= DRAM_BUF_LEN)
		r_pos -= DRAM_BUF_LEN;

	buf_info->r_pos = r_pos;

error:
	mutex_unlock(&scp_log_mutex);

	return datalen;
}

unsigned int scp_log_poll(void)
{
	if (!scp_logger_inited)
		return 0;

	if (buf_info->r_pos != buf_info->w_pos)
		return POLLIN | POLLRDNORM;

	/*scp_log_timer_add();*/

	return 0;
}



static ssize_t scp_log_if_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	ssize_t ret;

	pr_debug("[SCP] scp_log_if_read\n");

	ret = 0;

	/*if (access_ok(VERIFY_WRITE, data, len))*/
		ret = scp_log_read(data, len);

	return ret;
}

static int scp_log_if_open(struct inode *inode, struct file *file)
{
	pr_debug("[SCP] scp_log_if_open\n");
	return nonseekable_open(inode, file);
}

static unsigned int scp_log_if_poll(struct file *file, poll_table *wait)
{
	unsigned int ret = 0;

	/* pr_debug("[SCP] scp_log_if_poll\n"); */

	if (!(file->f_mode & FMODE_READ))
		return ret;

	poll_wait(file, &logwait, wait);

	ret = scp_log_poll();

	return ret;
}

/*ipi send enable scp logger*/
static unsigned int scp_log_enable_set(unsigned int enable)
{

	int ret, ackdata;
/*TODO send ipi to scp and enable logger flag*/
	ret = 0;
	ackdata = 0;
	if (scp_logger_inited) {

		if (ret != 0) {
			pr_err("[SCP] logger IPI fail ret=%d\n", ret);
			goto error;
		}

		if (enable != ackdata) {
			pr_err("[SCP] scp_log_enable_set fail ret=%d\n", ackdata);
			goto error;
		}

		log_ctl->enable = enable;
	}

error:
	return 0;
}

/*create device sysfs*/

static ssize_t scp_mobile_log_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	unsigned int stat;

	stat = (scp_logger_inited && log_ctl->enable) ? 1 : 0;

	return sprintf(buf, "[SCP] mobile log is %s\n", (stat == 0x1) ? "enabled" : "disabled");
}

static ssize_t scp_mobile_log_store(struct device *kobj, struct device_attribute *attr, const char *buf, size_t n)
{
	unsigned int enable;

	if (kstrtouint(buf, 0, &enable) != 0)
		return -EINVAL;

	mutex_lock(&scp_log_mutex);
	scp_log_enable_set(enable);
	mutex_unlock(&scp_log_mutex);

	return n;
}

DEVICE_ATTR(scp_mobile_log, S_IWUSR | S_IRUGO, scp_mobile_log_show, scp_mobile_log_store);


/*
 * IPI for logger init
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_logger_init_handler(int id, void *data, unsigned int len)
{
	unsigned long flags;
	unsigned int addr;
	unsigned int magic = 0x5A5A5A5A;

	addr = *(unsigned int *)data;
	pr_debug("[SCP]%s,%x\n", __func__, addr);
	pr_debug("[SCP]scp_get_reserve_mem_phys=%llx\n", scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID));
	spin_lock_irqsave(&scp_log_buf_spinlock, flags);

	/* setting dram ctrl config */
	mt_reg_sync_writel(scp_get_reserve_mem_phys(SCP_A_LOGGER_MEM_ID), (SCP_TCM + addr));

	spin_unlock_irqrestore(&scp_log_buf_spinlock, flags);
	/*To do : to support dual scp*/
	scp_ipi_send(IPI_LOGGER, &magic, sizeof(magic), 0, SCP_A_ID);


}

/*
 * init scp logger dram ctrl structure
 * @return:     0: success, otherwise: fail
 */
int scp_logger_init(phys_addr_t start, phys_addr_t limit)
{
	int last_ofs;

	/*init wait queue*/
	init_waitqueue_head(&logwait);
	/*init dram ctrl table*/
	last_ofs = 0;

	log_ctl = (struct log_ctrl_s *) start;
	log_ctl->base = PLT_LOG_ENABLE; /* magic */
	log_ctl->enable = 0;
	log_ctl->size = sizeof(*log_ctl);

	last_ofs += log_ctl->size;
	log_ctl->info_ofs = last_ofs;

	last_ofs += sizeof(*buf_info);
	last_ofs = ROUNDUP(last_ofs, 4);
	log_ctl->buff_ofs = last_ofs;
	log_ctl->buff_size = DRAM_BUF_LEN;

	buf_info = (struct buffer_info_s *) (((unsigned char *) log_ctl) + log_ctl->info_ofs);
	buf_info->r_pos = 0;
	buf_info->w_pos = 0;

	last_ofs += log_ctl->buff_size;

	if (last_ofs >= limit) {
		pr_err("[SCP]:%s() initial fail, last_ofs=%u, limit=%u\n", __func__, last_ofs, (unsigned int) limit);
		goto error;
	}


	/* register logger ini IPI */
	scp_ipi_registration(IPI_LOGGER, scp_logger_init_handler, "logger");
	/* register log buf full IPI */
	/*scp_ipi_registration(IPI_BUF_FULL, scp_buf_full_ipi_handler, "buf_full");*/

	scp_logger_inited = 1;
	return last_ofs;

error:
	scp_logger_inited = 0;
	log_ctl = NULL;
	buf_info = NULL;
	return -1;

}

const struct file_operations scp_log_file_ops = {
	.owner = THIS_MODULE,
	.read = scp_log_if_read,
	.open = scp_log_if_open,
	.poll = scp_log_if_poll,
};
