/*
* Copyright (C) 2016 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/printk.h>

#include "wmt_exp.h"
#include "stp_exp.h"

MODULE_LICENSE("Dual BSD/GPL");

#define BT_DRIVER_NAME "mtk_stp_bt_chrdev"
#define BT_DEV_MAJOR 192

#define PFX                         "[MTK-BT] "
#define BT_LOG_DBG                  3
#define BT_LOG_INFO                 2
#define BT_LOG_WARN                 1
#define BT_LOG_ERR                  0

static UINT32 gDbgLevel = BT_LOG_INFO;

#define BT_DBG_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_DBG) pr_debug(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_INFO_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_INFO) pr_debug(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_WARN_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_WARN) pr_warn(PFX "%s: " fmt, __func__, ##arg); } while (0)
#define BT_ERR_FUNC(fmt, arg...)	\
	do { if (gDbgLevel >= BT_LOG_ERR) pr_err(PFX "%s: " fmt, __func__, ##arg); } while (0)

#define VERSION "2.0"

#define COMBO_IOC_MAGIC             0xb0
#define COMBO_IOCTL_FW_ASSERT       _IOW(COMBO_IOC_MAGIC, 0, int)
#define COMBO_IOCTL_BT_SET_PSM      _IOW(COMBO_IOC_MAGIC, 1, bool)
#define COMBO_IOCTL_BT_IC_HW_VER    _IOR(COMBO_IOC_MAGIC, 2, void*)
#define COMBO_IOCTL_BT_IC_FW_VER    _IOR(COMBO_IOC_MAGIC, 3, void*)

static INT32 BT_devs = 1;
static INT32 BT_major = BT_DEV_MAJOR;
module_param(BT_major, uint, 0);
static struct cdev BT_cdev;
#if CREATE_NODE_DYNAMIC
static struct class *stpbt_class;
static struct device *stpbt_dev;
#endif

#define BT_BUFFER_SIZE              2048
static UINT8 i_buf[BT_BUFFER_SIZE]; /* Input buffer for read */
static UINT8 o_buf[BT_BUFFER_SIZE]; /* Output buffer for write */

static struct semaphore wr_mtx, rd_mtx;
static struct wake_lock bt_wakelock;
/* Wait queue for poll and read */
static wait_queue_head_t inq;
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static INT32 flag;
/*
 * Reset flag for whole chip reset scenario, to indicate reset status:
 *   0 - normal, no whole chip reset occurs
 *   1 - reset start
 *   2 - reset end, have not sent Hardware Error event yet
 *   3 - reset end, already sent Hardware Error event
 */
static volatile UINT32 rstflag;
static UINT8 HCI_EVT_HW_ERROR[] = {0x04, 0x10, 0x01, 0x00};

/*******************************************************************
 * WHOLE CHIP RESET message handler
 *******************************************************************
*/
static VOID bt_cdev_rst_cb(ENUM_WMTDRV_TYPE_T src,
			   ENUM_WMTDRV_TYPE_T dst, ENUM_WMTMSG_TYPE_T type, PVOID buf, UINT32 sz)
{
	ENUM_WMTRSTMSG_TYPE_T rst_msg;

	if (sz > sizeof(ENUM_WMTRSTMSG_TYPE_T)) {
		BT_WARN_FUNC("Invalid message format!\n");
		return;
	}

	memcpy((PINT8)&rst_msg, (PINT8)buf, sz);
	BT_DBG_FUNC("src = %d, dst = %d, type = %d, buf = 0x%x sz = %d, max = %d\n",
		    src, dst, type, rst_msg, sz, WMTRSTMSG_RESET_MAX);
	if ((src == WMTDRV_TYPE_WMT) && (dst == WMTDRV_TYPE_BT) && (type == WMTMSG_TYPE_RESET)) {
		switch (rst_msg) {
		case WMTRSTMSG_RESET_START:
			BT_INFO_FUNC("Whole chip reset start!\n");
			rstflag = 1;
			break;

		case WMTRSTMSG_RESET_END:
		case WMTRSTMSG_RESET_END_FAIL:
			if (rst_msg == WMTRSTMSG_RESET_END)
				BT_INFO_FUNC("Whole chip reset end!\n");
			else
				BT_INFO_FUNC("Whole chip reset fail!\n");
			rstflag = 2;
			flag = 1;
			wake_up_interruptible(&inq);
			wake_up(&BT_wq);
			break;

		default:
			break;
		}
	}
}

VOID BT_event_cb(VOID)
{
	BT_DBG_FUNC("BT_event_cb\n");

	/*
	 * Hold wakelock for 100ms to avoid system enter suspend in such case:
	 *   FW has sent data to host, STP driver received the data and put it
	 *   into BT rx queue, then send sleep command and release wakelock as
	 *   quick sleep mechanism for low power, BT driver will wake up stack
	 *   hci thread stuck in poll or read.
	 *   But before hci thread comes to read data, system enter suspend,
	 *   hci command timeout timer keeps counting during suspend period till
	 *   expires, then the RTC interrupt wakes up system, command timeout
	 *   handler is executed and meanwhile the event is received.
	 *   This will false trigger FW assert and should never happen.
	 */
	wake_lock_timeout(&bt_wakelock, 100);

	/*
	 * Finally, wake up any reader blocked in poll or read
	 */
	flag = 1;
	wake_up_interruptible(&inq);
	wake_up(&BT_wq);
}

unsigned int BT_poll(struct file *filp, poll_table *wait)
{
	UINT32 mask = 0;

	if ((mtk_wcn_stp_is_rxqueue_empty(BT_TASK_INDX) && rstflag == 0) ||
	    (rstflag == 1) || (rstflag == 3)) {
		/*
		 * BT rx queue is empty, or whole chip reset start, or already sent Hardware Error event
		 * for whole chip reset end, add to wait queue.
		 */
		poll_wait(filp, &inq, wait);
		/*
		 * Check if condition changes before poll_wait return, in case of
		 * wake_up_interruptible is called before add_wait_queue, otherwise,
		 * do_poll will get into sleep and never be waken up until timeout.
		 */
		if (!((mtk_wcn_stp_is_rxqueue_empty(BT_TASK_INDX) && rstflag == 0) ||
		      (rstflag == 1) || (rstflag == 3)))
			mask |= POLLIN | POLLRDNORM;	/* Readable */
	} else {
		/* BT rx queue has valid data, or whole chip reset end, have not sent Hardware Error event yet */
		mask |= POLLIN | POLLRDNORM;	/* Readable */
	}

	/* Do we need condition here? */
	mask |= POLLOUT | POLLWRNORM;	/* Writable */

	return mask;
}

ssize_t BT_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 retval = 0;

	down(&wr_mtx);

	BT_DBG_FUNC("count %zd pos %lld\n", count, *f_pos);

	if (rstflag) {
		BT_ERR_FUNC("whole chip reset occurs! rstflag=%d\n", rstflag);
		retval = -EIO;
		goto OUT;
	}

	if (count > 0) {
		if (count > BT_BUFFER_SIZE) {
			count = BT_BUFFER_SIZE;
			BT_WARN_FUNC("Shorten write count from %zd to %d\n", count, BT_BUFFER_SIZE);
		}

		if (copy_from_user(o_buf, buf, count)) {
			retval = -EFAULT;
			goto OUT;
		}

		retval = mtk_wcn_stp_send_data(o_buf, count, BT_TASK_INDX);
		if (retval < 0)
			BT_ERR_FUNC("mtk_wcn_stp_send_data fail, retval %d\n", retval);
		else if (retval == 0) {
			/* Device cannot process data in time, STP queue is full and no space is available for write,
			 * native program should not call BT_write with no delay.
			 */
			BT_ERR_FUNC("Packet length %zd, sent bytes %d, no space is available!\n", count, retval);
			retval = -EAGAIN;
		} else
			BT_DBG_FUNC("Packet length %zd, sent bytes %d\n", count, retval);

	} else {
		BT_ERR_FUNC("Packet length %zd is not allowed\n", count);
		retval = -EINVAL;
	}

OUT:
	up(&wr_mtx);
	return retval;
}

ssize_t BT_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	INT32 retval = 0;

	down(&rd_mtx);

	BT_DBG_FUNC("count %zd pos %lld\n", count, *f_pos);

	if (rstflag) {
		while (rstflag != 2) {
			/*
			 * If nonblocking mode, return directly.
			 * O_NONBLOCK is specified during open()
			 */
			if (filp->f_flags & O_NONBLOCK) {
				BT_ERR_FUNC("Non-blocking read, whole chip reset occurs! rstflag=%d\n", rstflag);
				retval = -EIO;
				goto OUT;
			}

			wait_event(BT_wq, flag != 0);
			flag = 0;
		}
		/*
		 * Reset end, send Hardware Error event to stack only once.
		 * To avoid high frequency read from stack before process is killed, set rstflag to 3
		 * to block poll and read after Hardware Error event is sent.
		 */
		BT_INFO_FUNC("Send Hardware Error event to stack to restart Bluetooth\n");
		memcpy(i_buf, HCI_EVT_HW_ERROR, sizeof(HCI_EVT_HW_ERROR));
		retval = sizeof(HCI_EVT_HW_ERROR);
		rstflag = 3;

		if (copy_to_user(buf, i_buf, retval)) {
			retval = -EFAULT;
			rstflag = 2;
		}

		goto OUT;
	}

	if (count > BT_BUFFER_SIZE) {
		count = BT_BUFFER_SIZE;
		BT_WARN_FUNC("Shorten read count from %zd to %d\n", count, BT_BUFFER_SIZE);
	}

	do {
		retval = mtk_wcn_stp_receive_data(i_buf, count, BT_TASK_INDX);
		if (retval < 0) {
			BT_ERR_FUNC("mtk_wcn_stp_receive_data fail, retval %d\n", retval);
			goto OUT;
		} else if (retval == 0) {	/* Got nothing, wait for STP's signal */
			/*
			 * If nonblocking mode, return directly.
			 * O_NONBLOCK is specified during open()
			 */
			if (filp->f_flags & O_NONBLOCK) {
				BT_ERR_FUNC("Non-blocking read, no data is available!\n");
				retval = -EAGAIN;
				goto OUT;
			}

			wait_event(BT_wq, flag != 0);
			flag = 0;
		} else {	/* Got something from STP driver */
			BT_DBG_FUNC("Read bytes %d\n", retval);
			break;
		}
	} while (!mtk_wcn_stp_is_rxqueue_empty(BT_TASK_INDX) && rstflag == 0);

	if (retval == 0) {
		if (rstflag != 2) {	/* Should never happen */
			WARN(1, "Blocking read is waken up with no data but rstflag=%d\n", rstflag);
			retval = -EIO;
			goto OUT;
		} else {	/* Reset end, send Hardware Error event only once */
			BT_INFO_FUNC("Send Hardware Error event to stack to restart Bluetooth\n");
			memcpy(i_buf, HCI_EVT_HW_ERROR, sizeof(HCI_EVT_HW_ERROR));
			retval = sizeof(HCI_EVT_HW_ERROR);
			rstflag = 3;
		}
	}

	if (copy_to_user(buf, i_buf, retval)) {
		retval = -EFAULT;
		if (rstflag == 3)
			rstflag = 2;
		goto OUT;
	}

OUT:
	up(&rd_mtx);
	return retval;
}

/* int BT_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) */
long BT_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	INT32 retval = 0;
	UINT32 reason;
	UINT32 ver = 0;

	BT_DBG_FUNC("cmd: 0x%08x\n", cmd);

	switch (cmd) {
	case COMBO_IOCTL_FW_ASSERT:
		/* Trigger FW assert for debug */
		reason = (UINT32)arg & 0xFFFF;
		BT_INFO_FUNC("Host trigger FW assert......, reason:%d\n", reason);
		if (reason == 31) /* HCI command timeout */
			BT_INFO_FUNC("HCI command timeout OpCode 0x%04x\n", ((UINT32)arg >> 16) & 0xFFFF);

		if (mtk_wcn_wmt_assert(WMTDRV_TYPE_BT, reason) == MTK_WCN_BOOL_TRUE) {
			BT_INFO_FUNC("Host trigger FW assert succeed\n");
			retval = 0;
		} else {
			BT_ERR_FUNC("Host trigger FW assert failed\n");
			retval = -EBUSY;
		}
		break;
	case COMBO_IOCTL_BT_SET_PSM:
		/* BT stack may need to dynamically enable/disable Power Saving Mode
		 * in some scenarios for performance, e.g. A2DP chopping.
		 */
		BT_INFO_FUNC("BT stack change PSM setting: %lu\n", arg);
		retval = mtk_wcn_wmt_psm_ctrl((MTK_WCN_BOOL)arg);
		break;
	case COMBO_IOCTL_BT_IC_HW_VER:
		ver = mtk_wcn_wmt_ic_info_get(WMTCHIN_HWVER);
		BT_INFO_FUNC("HW ver: 0x%x\n", ver);
		if (copy_to_user((UINT32 __user *)arg, &ver, sizeof(ver)))
			retval = -EFAULT;
		break;
	case COMBO_IOCTL_BT_IC_FW_VER:
		ver = mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER);
		BT_INFO_FUNC("FW ver: 0x%x\n", ver);
		if (copy_to_user((UINT32 __user *)arg, &ver, sizeof(ver)))
			retval = -EFAULT;
		break;
	default:
		BT_ERR_FUNC("Unknown cmd: 0x%08x\n", cmd);
		retval = -EOPNOTSUPP;
		break;
	}

	return retval;
}

long BT_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return BT_unlocked_ioctl(filp, cmd, arg);
}

static int BT_open(struct inode *inode, struct file *file)
{
	BT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	/* Turn on BT */
	if (mtk_wcn_wmt_func_on(WMTDRV_TYPE_BT) == MTK_WCN_BOOL_FALSE) {
		BT_WARN_FUNC("WMT turn on BT fail!\n");
		return -EIO;
	}

	BT_INFO_FUNC("WMT turn on BT OK!\n");

	if (mtk_wcn_stp_is_ready() == MTK_WCN_BOOL_FALSE) {

		BT_ERR_FUNC("STP is not ready!\n");
		mtk_wcn_wmt_func_off(WMTDRV_TYPE_BT);
		return -EIO;
	}

	mtk_wcn_stp_set_bluez(0);

	BT_INFO_FUNC("Now it's in MTK Bluetooth Mode\n");
	BT_INFO_FUNC("STP is ready!\n");

	BT_DBG_FUNC("Register BT event callback!\n");
	mtk_wcn_stp_register_event_cb(BT_TASK_INDX, BT_event_cb);

	BT_DBG_FUNC("Register BT reset callback!\n");
	mtk_wcn_wmt_msgcb_reg(WMTDRV_TYPE_BT, bt_cdev_rst_cb);
	rstflag = 0;

	sema_init(&wr_mtx, 1);
	sema_init(&rd_mtx, 1);

	return 0;
}

static int BT_close(struct inode *inode, struct file *file)
{
	BT_INFO_FUNC("major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);
	rstflag = 0;
	mtk_wcn_wmt_msgcb_unreg(WMTDRV_TYPE_BT);
	mtk_wcn_stp_register_event_cb(BT_TASK_INDX, NULL);

	if (mtk_wcn_wmt_func_off(WMTDRV_TYPE_BT) == MTK_WCN_BOOL_FALSE) {
		BT_ERR_FUNC("WMT turn off BT fail!\n");
		return -EIO;	/* Mostly, native program will not check this return value. */
	}

	BT_INFO_FUNC("WMT turn off BT OK!\n");
	return 0;
}

const struct file_operations BT_fops = {
	.open = BT_open,
	.release = BT_close,
	.read = BT_read,
	.write = BT_write,
	/* .ioctl = BT_ioctl, */
	.unlocked_ioctl = BT_unlocked_ioctl,
	.compat_ioctl = BT_compat_ioctl,
	.poll = BT_poll
};

static int BT_init(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	INT32 alloc_ret = 0;
	INT32 cdev_err = 0;

	/* Allocate char device */
	alloc_ret = register_chrdev_region(dev, BT_devs, BT_DRIVER_NAME);
	if (alloc_ret) {
		BT_ERR_FUNC("Failed to register device numbers\n");
		return alloc_ret;
	}

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;

	cdev_err = cdev_add(&BT_cdev, dev, BT_devs);
	if (cdev_err)
		goto error;

#if CREATE_NODE_DYNAMIC /* mknod replace */
	stpbt_class = class_create(THIS_MODULE, "stpbt");
	if (IS_ERR(stpbt_class))
		goto error;
	stpbt_dev = device_create(stpbt_class, NULL, dev, NULL, "stpbt");
	if (IS_ERR(stpbt_dev))
		goto error;
#endif

	BT_INFO_FUNC("%s driver(major %d) installed\n", BT_DRIVER_NAME, BT_major);

	/* Initialize wait queue */
	init_waitqueue_head(&(inq));
	/* Initialize wake lock */
	wake_lock_init(&bt_wakelock, WAKE_LOCK_SUSPEND, "bt_drv");

	return 0;

error:
#if CREATE_NODE_DYNAMIC
	if (stpbt_dev && !IS_ERR(stpbt_dev)) {
		device_destroy(stpbt_class, dev);
		stpbt_dev = NULL;
	}
	if (stpbt_class && !IS_ERR(stpbt_class)) {
		class_destroy(stpbt_class);
		stpbt_class = NULL;
	}
#endif
	if (cdev_err == 0)
		cdev_del(&BT_cdev);

	if (alloc_ret == 0)
		unregister_chrdev_region(dev, BT_devs);

	return -1;
}

static void BT_exit(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	/* Destroy wake lock*/
	wake_lock_destroy(&bt_wakelock);

#if CREATE_NODE_DYNAMIC
	if (stpbt_dev && !IS_ERR(stpbt_dev)) {
		device_destroy(stpbt_class, dev);
		stpbt_dev = NULL;
	}
	if (stpbt_class && !IS_ERR(stpbt_class)) {
		class_destroy(stpbt_class);
		stpbt_class = NULL;
	}
#endif

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, BT_devs);

	BT_INFO_FUNC("%s driver removed\n", BT_DRIVER_NAME);
}

#ifdef MTK_WCN_REMOVE_KERNEL_MODULE

int mtk_wcn_stpbt_drv_init(void)
{
	return BT_init();
}
EXPORT_SYMBOL(mtk_wcn_stpbt_drv_init);

void mtk_wcn_stpbt_drv_exit(void)
{
	return BT_exit();
}
EXPORT_SYMBOL(mtk_wcn_stpbt_drv_exit);

#else

module_init(BT_init);
module_exit(BT_exit);

#endif
