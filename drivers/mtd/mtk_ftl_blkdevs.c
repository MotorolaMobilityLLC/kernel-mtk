/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Isaac.Lee <isaac.lee@mediatek.com>
 *	  Bean.Li <bean.li@mediatek.com>
 *	  Moon.Li <moon.li@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Read-Write block devices on top of UBI volumes
 *
 * A robust implementation to allow a block device to be layered on top of a
 * UBI volume. The implementation is provided by creating a dynamic page
 * mapping between the block device and the UBI volume.
 *
 * The addressed byte is obtained from the addressed block sector, which is
 * mapped table into the corresponding LEB & PAGE:
 *
 * This feature is compiled in the UBI core, and adds a 'block' parameter
 * to allow early creation of block devices on top of UBI volumes. Runtime
 * block creation/removal for UBI volumes is provided through two UBI ioctls:
 * UBI_IOCVOLCRBLK and UBI_IOCVOLRMBLK.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/ubi.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/reboot.h>
#include <asm/div64.h>

#include "ubi/ubi-media.h"
#include "ubi/ubi.h"
#include "mtk_ftl.h"

#include <trace/events/mtk_ftl.h>

/* Maximum number of supported devices */
#define MT_FTL_BLK_MAX_DEVICES 32

/* Maximum length of the 'block=' parameter */
#define MT_FTL_BLK_PARAM_LEN 63

/* Maximum number of comma-separated items in the 'block=' parameter */
#define MT_FTL_BLK_PARAM_COUNT 2

struct mt_ftl_blk_param {
	int ubi_num;
	int vol_id;
	char name[MT_FTL_BLK_PARAM_LEN+1];
};

/* Numbers of elements set in the @mt_ftl_blk_param array */
static int mt_ftl_blk_devs __initdata;

/* MTD devices specification parameters */
static struct mt_ftl_blk_param mt_ftl_blk_param[MT_FTL_BLK_MAX_DEVICES] __initdata;

/* Linked list of all mt_ftl_blk instances */
static LIST_HEAD(mt_ftl_blk_devices);
static DEFINE_MUTEX(devices_mutex);
static int mt_ftl_blk_major;
static int mt_ftl_dev_count;
static struct mt_ftl_blk *mt_ftl_dev_ptr[MT_FTL_BLK_MAX_DEVICES];

static int __init mt_ftl_blk_set_param(const char *val,
				     const struct kernel_param *kp)
{
	int i, ret;
	size_t len;
	struct mt_ftl_blk_param *param;
	char buf[MT_FTL_BLK_PARAM_LEN];
	char *pbuf = &buf[0];
	char *tokens[MT_FTL_BLK_PARAM_COUNT];

	if (!val)
		return -EINVAL;

	len = strnlen(val, MT_FTL_BLK_PARAM_LEN);
	if (len == 0)
		return 0;

	if (len == MT_FTL_BLK_PARAM_LEN)
		return -EINVAL;

	strcpy(buf, val);

	/* Get rid of the final newline */
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	for (i = 0; i < MT_FTL_BLK_PARAM_COUNT; i++)
		tokens[i] = strsep(&pbuf, ",");

	param = &mt_ftl_blk_param[mt_ftl_blk_devs];
	if (tokens[1]) {
		/* Two parameters: can be 'ubi, vol_id' or 'ubi, vol_name' */
		ret = kstrtoint(tokens[0], 10, &param->ubi_num);
		if (ret < 0)
			return -EINVAL;

		/* Second param can be a number or a name */
		ret = kstrtoint(tokens[1], 10, &param->vol_id);
		if (ret < 0) {
			param->vol_id = -1;
			strcpy(param->name, tokens[1]);
		}

	} else {
		/* One parameter: must be device path */
		strcpy(param->name, tokens[0]);
		param->ubi_num = -1;
		param->vol_id = -1;
	}

	mt_ftl_blk_devs++;

	return 0;
}

static struct kernel_param_ops mt_ftl_blk_param_ops = {
	.set    = mt_ftl_blk_set_param,
};
module_param_cb(block, &mt_ftl_blk_param_ops, NULL, 0);
MODULE_PARM_DESC(block, "Attach block devices to UBI volumes. Parameter format: block=<path|dev,num|dev,name>.\n"
			"Multiple \"block\" parameters may be specified.\n"
			"UBI volumes may be specified by their number, name, or path to the device node.\n"
			"Examples\n"
			"Using the UBI volume path:\n"
			"ubi.block=/dev/ubi0_0\n"
			"Using the UBI device, and the volume name:\n"
			"ubi.block=0,rootfs\n"
			"Using both UBI device number and UBI volume number:\n"
			"ubi.block=0,0\n");

static struct mt_ftl_blk *find_dev_nolock(int ubi_num, int vol_id)
{
	struct mt_ftl_blk *dev;

	list_for_each_entry(dev, &mt_ftl_blk_devices, list)
		if (dev->ubi_num == ubi_num && dev->vol_id == vol_id)
			return dev;
	return NULL;
}

static int mt_ftl_blk_sync_data(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;

	ret = mt_ftl_write_page(dev);
#ifdef MT_FTL_SUPPORT_WBUF
	mt_ftl_wbuf_forced_flush(dev);
#endif

	return ret;
}

static int mt_ftl_blk_flush(struct mt_ftl_blk *dev, bool sync)
{
	int ret = 0;

	/* Don't put mt_ftl_commit here */
	/* Sync ubi device when device is released and on block flush ioctl */
	if (sync) {
		mt_ftl_commit(dev);
		ret = ubi_sync(dev->ubi_num);
	}
	return ret;
}

static int do_mt_ftl_blk_request(struct mt_ftl_blk *dev, struct request *req)
{
	int ret = MT_FTL_SUCCESS;
	sector_t sec;
	unsigned nsects, i;
	unsigned max_discard_sectors, granularity;
	struct bio_vec bvec;
	struct req_iterator iter;
	char *data = NULL;

	if (req->cmd_type != REQ_TYPE_FS) {
		mt_ftl_err(dev, "cmd_type != REQ_TYPE_FS %d", req->cmd_type);
		blk_dump_rq_flags(req, "[Bean]");
		return -EIO;
	}

	sec = blk_rq_pos(req);
	nsects = blk_rq_sectors(req);

	if (req->cmd_flags & REQ_FLUSH) {
		if (mt_ftl_blk_sync_data(dev))
			return -EIO;
	}

	/* Ignore empty request */
	if (nsects == 0)
		return 0;

	if (sec + nsects > get_capacity(req->rq_disk)) {
		mt_ftl_err(dev, "0x%lx 0x%x", (unsigned long)sec, nsects);
		if ((req->cmd_flags & REQ_SECURE) && (sec == 0)) {
			mt_ftl_err(dev, "discard secure");
			dev->secure_discard += nsects;
			ret = mt_ftl_param_default(dev);
			dev->secure_discard = 0;
			return ret ? -EIO : 0;
		}
		mt_ftl_err(dev, "request(0x%lx) exceed the capacity(0x%lx)",
			(unsigned long)(sec + nsects), (unsigned long)get_capacity(req->rq_disk));
		return -EIO;
	}

	if (req->cmd_flags & REQ_SECURE) {
		mt_ftl_err(dev, "discard secure");
		granularity = max(dev->rq->limits.discard_granularity >> 9, 1U);
		max_discard_sectors = min(dev->rq->limits.max_discard_sectors, UINT_MAX >> 9);
		max_discard_sectors -= max_discard_sectors % granularity;
		if (nsects >= max_discard_sectors) {
			dev->secure_discard += nsects;
			ret = mt_ftl_param_default(dev);
			return ret ? -EIO : 0;
		} else if (dev->secure_discard > 0 && dev->secure_discard < get_capacity(req->rq_disk)) {
			dev->secure_discard += nsects;
			mt_ftl_err(dev, "%lx %lx",
				(unsigned long)dev->secure_discard, (unsigned long)get_capacity(req->rq_disk));
			return 0;
		}
		dev->secure_discard = 0;
		return -EIO;
	}
	dev->secure_discard = 0;

	if (req->cmd_flags & REQ_DISCARD) {
		mt_ftl_debug(dev, "discard %lx %x", (unsigned long)sec, nsects);
		trace_mt_ftl_discard(dev, sec, nsects);
		for (i = 0; i < nsects; i += SECS_OF_FS_PAGE) {
			/* write zero for discard */
			ret = mt_ftl_write(dev, data, sec + i, 0);
			if (ret)
				mt_ftl_err(dev, "%lx %x", (unsigned long)sec, nsects);
		}
		return ret ? -EIO : 0;
	}

	switch (rq_data_dir(req)) {
	case READ:
		rq_for_each_segment(bvec, req, iter) {
			data = page_address(bvec.bv_page) + bvec.bv_offset;
			ret = mt_ftl_read(dev, data, iter.iter.bi_sector, bvec.bv_len);
			flush_dcache_page(bvec.bv_page);
			if (ret)
				return -EIO;
		}
		break;
	case WRITE:
		rq_for_each_segment(bvec, req, iter) {
			data = page_address(bvec.bv_page) + bvec.bv_offset;
			flush_dcache_page(bvec.bv_page);
			ret = mt_ftl_write(dev, data, iter.iter.bi_sector, bvec.bv_len);
			if (ret)
				return -EIO;
		}
		break;
	default:
		mt_ftl_err(dev, "unknown request\n");
		ret = -EIO;
	}
	return ret;
}

#ifdef MT_FTL_SUPPORT_MQ
int mt_ftl_blk_thread(void *info)
{
	int err;
	unsigned long flags;
	struct mt_ftl_blk *dev = info;
	struct mt_ftl_blk_rq *rq = NULL;
	struct request *req = NULL;

	mt_ftl_err(dev, "%s thread started, PID %d", dev->blk_name, current->pid);
	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		if (list_empty_careful(&dev->ftl_rq_list)) {
			if (kthread_should_stop()) {
				__set_current_state(TASK_RUNNING);
				break;
			}
			schedule();
			continue;
		} else
			__set_current_state(TASK_RUNNING);

		rq = list_first_entry(&dev->ftl_rq_list, struct mt_ftl_blk_rq, list);
		spin_lock_irqsave(&dev->ftl_rq_lock, flags);
		list_del_init(&rq->list);
		spin_unlock_irqrestore(&dev->ftl_rq_lock, flags);
		req = rq->req;
		mutex_lock(&dev->dev_mutex);
		err = do_mt_ftl_blk_request(dev, req);
		mutex_unlock(&dev->dev_mutex);
		if (err) {
			mt_ftl_err(dev, "[Bean]request error %d\n", err);
			dump_stack();
			blk_mq_end_request(req, err);
			break;
		}
		req->errors = err;
		blk_mq_end_request(req, req->errors);
		cond_resched();
	}
	mt_ftl_err(dev, "%s thread stops, err = %d", dev->blk_name, err);
	return 0;
}

static int mt_ftl_blk_mq_init(struct mt_ftl_blk *dev)
{
	int ret;

	spin_lock_init(&dev->ftl_rq_lock);
	INIT_LIST_HEAD(&dev->ftl_rq_list);

	sprintf(dev->blk_name, "mt_ftl%d/%d", dev->ubi_num, dev->vol_id);
	dev->blk_bgt = kthread_create(mt_ftl_blk_thread, dev, "%s", dev->blk_name);
	if (IS_ERR(dev->blk_bgt)) {
		ret = PTR_ERR(dev->blk_bgt);
		dev->blk_bgt = NULL;
		mt_ftl_err(dev, "cannot spawn \"%s\", error %d", dev->blk_name, ret);
		return ret;
	}
	wake_up_process(dev->blk_bgt);
	return MT_FTL_SUCCESS;
}

static int mt_ftl_queue_rq(struct blk_mq_hw_ctx *hctx, struct request *req, bool last)
{
	unsigned long flags;
	struct mt_ftl_blk *dev = hctx->queue->queuedata;
	struct mt_ftl_blk_rq *ftl_rq = NULL;
	struct mt_ftl_blk_pdu *blk_pdu = NULL;

	if (dev->request_type == FTL_RT_MQ_THREAD) {
		ftl_rq = blk_mq_rq_to_pdu(req);
		blk_mq_start_request(req);
		ftl_rq->req = req;
		spin_lock_irqsave(&dev->ftl_rq_lock, flags);
		list_add_tail(&ftl_rq->list, &dev->ftl_rq_list);
		spin_unlock_irqrestore(&dev->ftl_rq_lock, flags);

		if (dev->blk_bgt)
			wake_up_process(dev->blk_bgt);
	} else {
		blk_pdu = blk_mq_rq_to_pdu(req);
		queue_work(dev->wq, &blk_pdu->work);
	}

	return BLK_MQ_RQ_QUEUE_OK;
}


static void mt_ftl_do_mq_work(struct work_struct *work)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_blk_pdu *blk_pdu = container_of(work, struct mt_ftl_blk_pdu, work);
	struct request *req = blk_mq_rq_from_pdu(blk_pdu);

	blk_mq_start_request(req);

	mutex_lock(&blk_pdu->dev->dev_mutex);
	ret = do_mt_ftl_blk_request(blk_pdu->dev, req);
	if (ret) {
		mt_ftl_err(blk_pdu->dev, "[Bean]request error %d\n", ret);
		dump_stack();
	}
	mutex_unlock(&blk_pdu->dev->dev_mutex);
	req->errors = ret;
	blk_mq_end_request(req, ret);
}

static int mt_ftl_init_request(void *data, struct request *req,
				 unsigned int hctx_idx,
				 unsigned int request_idx,
				 unsigned int numa_node)
{
	struct mt_ftl_blk_pdu *blk_pdu = NULL;
	struct mt_ftl_blk *dev = data;

	if (dev->request_type == FTL_RT_MQ) {
		blk_pdu = blk_mq_rq_to_pdu(req);
		blk_pdu->dev = data;

		INIT_WORK(&blk_pdu->work, mt_ftl_do_mq_work);
	}

	return 0;
}

static struct blk_mq_ops mt_ftl_mq_ops = {
	.queue_rq	= mt_ftl_queue_rq,
	.map_queue	= blk_mq_map_queue,
	.init_request	= mt_ftl_init_request,
};

static void mt_ftl_make_request(struct request_queue *q, struct bio *bio)
{

}
#endif /* MT_FTL_SUPPORT_MQ */

static void mt_ftl_blk_do_work(struct work_struct *work)
{
	struct mt_ftl_blk *dev =
		container_of(work, struct mt_ftl_blk, work);
	struct request_queue *rq = dev->rq;
	struct request *req = NULL;
	int res;

	while (1) {
		mutex_lock(&dev->dev_mutex);
		spin_lock_irq(rq->queue_lock);
		req = blk_fetch_request(rq);
		spin_unlock_irq(rq->queue_lock);
		if (!req)
			break;
		res = do_mt_ftl_blk_request(dev, req);
		if (res) {
			mt_ftl_err(dev, "[Bean]request error %d\n", res);
			dump_stack();
			blk_end_request_all(req, res);
			break;
		}
		blk_end_request_all(req, res);
		mutex_unlock(&dev->dev_mutex);
		cond_resched();
	}
	mutex_unlock(&dev->dev_mutex);
}

static void mt_ftl_blk_request(struct request_queue *rq)
{
	struct mt_ftl_blk *dev;
	struct request *req;

	dev = rq->queuedata;

	if (!dev)
		while ((req = blk_fetch_request(rq)) != NULL)
			__blk_end_request_all(req, -ENODEV);
	else
		queue_work(dev->wq, &dev->work);
}

static int mt_ftl_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mt_ftl_blk *dev = bdev->bd_disk->private_data;
	int ret;

	mutex_lock(&dev->dev_mutex);
	if (dev->refcnt > 0) {
		/*
		 * The volume is already open, just increase the reference
		 * counter.
		 */
		goto out_done;
	}

	/*
	 * We want users to be aware they should only mount us as read-only.
	 * It's just a paranoid check, as write requests will get rejected
	 * in any case.
	 */
	if (mode & FMODE_WRITE) {
		if (dev->readonly) {
			set_disk_ro(dev->gd, 0);
			dev->commit_node.u4BlockDeviceModeFlag = 0;
		}
		dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
	} else
		dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READONLY);

	if (IS_ERR(dev->desc)) {
		mt_ftl_err(dev, "%s failed to open ubi volume %d_%d",
			dev->gd->disk_name, dev->ubi_num, dev->vol_id);
		ret = PTR_ERR(dev->desc);
		dev->desc = NULL;
		goto out_unlock;
	}
out_done:
	dev->refcnt++;
	mutex_unlock(&dev->dev_mutex);
	return 0;

out_unlock:
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static void mt_ftl_blk_release(struct gendisk *gd, fmode_t mode)
{
	bool sync;
	struct mt_ftl_blk *dev = gd->private_data;

	mutex_lock(&dev->dev_mutex);
	dev->refcnt--;
	sync = !!(mode & FMODE_WRITE);
	if (sync)
		mt_ftl_blk_sync_data(dev);

	if (dev->refcnt == 0) {
		mt_ftl_blk_flush(dev, sync);
		ubi_close_volume(dev->desc);
		dev->desc = NULL;
	}
	mutex_unlock(&dev->dev_mutex);
}

static int mt_ftl_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	/* Some tools might require this information */
	geo->heads = 1;
	geo->cylinders = 1;
	geo->sectors = get_capacity(bdev->bd_disk);
	geo->start = 0;
	return 0;
}

static int mt_ftl_blk_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct mt_ftl_blk *dev = bdev->bd_disk->private_data;
	int ret = -ENXIO, data;

	if (!dev)
		return ret;

	mutex_lock(&dev->dev_mutex);
	switch (cmd) {
	case BLKROSET:
		/*need to do only for adb remount*/
		if (copy_from_user(&data, (void __user *)arg, sizeof(int))) {
			ret = -EFAULT;
			break;
		}
		if (data == 0) {
			mt_ftl_debug(dev, "[Bean]set rw %d %d", dev->ubi_num, dev->vol_id);
			set_disk_ro(dev->gd, 0);
			ubi_close_volume(dev->desc);
			dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
			dev->commit_node.u4BlockDeviceModeFlag = 0;
			mt_ftl_commit(dev);
		}
		ret = 0;
		break;
	case BLKFLSBUF:
		mt_ftl_err(dev, "[Bean]BLKFLSBUF");
		ret = mt_ftl_blk_flush(dev, true);
		break;
	default:
		ret = -ENOTTY;
	}
	mt_ftl_debug(dev, "CMD:%d data:%d ret:%d", cmd, data, ret);
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

#ifdef CONFIG_COMPAT
static int mt_ftl_blk_compat_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	return mt_ftl_blk_ioctl(bdev, mode, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int mt_ftl_blk_rw_page(struct block_device *bdev, sector_t sector,
		       struct page *page, int rw)
{
	int ret;
	struct mt_ftl_blk *dev = bdev->bd_disk->private_data;
	char *data = NULL;

	ubi_assert(FS_PAGE_SIZE <= PAGE_CACHE_SIZE);
	mt_ftl_debug(dev, "[Bean]%lx %d", (unsigned long)sector, rw);
	mutex_lock(&dev->dev_mutex);
	data = page_address(page);
	if (rw == READ) {
		ret = mt_ftl_read(dev, data, sector, FS_PAGE_SIZE);
		flush_dcache_page(page);
		if (ret)
			ret = -EIO;
	} else {
		mt_ftl_err(dev, "[Bean]rw_write %lx %d %d",
			(unsigned long)sector, dev->commit_node.u4BlockDeviceModeFlag, rw);
		if (dev->commit_node.u4BlockDeviceModeFlag) {
			mt_ftl_err(dev, "Read Only Block Device %s", dev->gd->disk_name);
			ret = -EPERM;
		}
		/*Caution: Remove write operation here!!!!*/
		#if 0
		flush_dcache_page(page);
		ret = mt_ftl_write(dev, data, sector, FS_PAGE_SIZE);
		if (ret)
			ret = -EIO;
		#else
		ret = -EPERM;
		#endif
	}
	page_endio(page, rw & WRITE, ret);
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static const struct block_device_operations mt_ftl_blk_ops = {
	.owner = THIS_MODULE,
	.open = mt_ftl_blk_open,
	.release = mt_ftl_blk_release,
	.getgeo	= mt_ftl_blk_getgeo,
	.ioctl = mt_ftl_blk_ioctl,
	.rw_page = mt_ftl_blk_rw_page,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mt_ftl_blk_compat_ioctl,
#endif

};

static void mt_ftl_blk_info(struct mt_ftl_blk *dev)
{
	mt_ftl_err(dev, "%s info:", dev->gd->disk_name);
	mt_ftl_err(dev, "\t\t ubi_num: %d", dev->ubi_num);
	mt_ftl_err(dev, "\t\t vol_id: %d", dev->vol_id);
	mt_ftl_err(dev, "\t\t leb_size: %d", dev->leb_size);
	mt_ftl_err(dev, "\t\t min_io_size: %d", dev->min_io_size);
	mt_ftl_err(dev, "\t\t pm_per_io: %d", dev->pm_per_io);
	mt_ftl_err(dev, "\t\t pmt_blk_num: %d", dev->pmt_blk_num);
	mt_ftl_err(dev, "\t\t data_start_blk: %d", dev->data_start_blk);
	mt_ftl_err(dev, "\t\t max_replay_blks: %d", dev->max_replay_blks);
	mt_ftl_err(dev, "\t\t max_pmt_replay_blk: %d", dev->max_pmt_replay_blk);
}

int mt_ftl_blk_create(struct ubi_volume_desc *desc)
{
	struct mt_ftl_blk *dev;
	struct gendisk *gd;
	u64 disk_capacity;
	int ret, max_pages;
	struct ubi_volume_info vi;

	ubi_get_volume_info(desc, &vi);
	/* Check that the volume isn't already handled */
	mutex_lock(&devices_mutex);
	if (find_dev_nolock(vi.ubi_num, vi.vol_id)) {
		mutex_unlock(&devices_mutex);
		return -EEXIST;
	}
	mutex_unlock(&devices_mutex);

	dev = kzalloc(sizeof(struct mt_ftl_blk), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mutex_init(&dev->dev_mutex);

	/********************start init device info********************/
	dev->desc = desc;
	dev->ubi_num = vi.ubi_num;
	dev->vol_id = vi.vol_id;
	dev->leb_size = vi.usable_leb_size;
	dev->min_io_size = desc->vol->ubi->min_io_size;
	/* Page Mapping per nand page (must be power of 2), so SHIFT value */
	dev->pm_per_io = ffs(dev->min_io_size >> 2) - 1;
	mt_ftl_err(dev, "pm_per_io=%d", dev->pm_per_io);
	dev->readonly = 0;
	dev->max_replay_blks = dev->leb_size / dev->min_io_size * REPLAY_BLOCK_NUM;
	dev->commit_node.dev_blocks = dev->desc->vol->reserved_pebs - NAND_RESERVED_PEB;
	dev->commit_node.dev_clusters = (vi.used_bytes >> (dev->pm_per_io + SECS_OF_SHIFT + 9)) + 1;
	/********************end init device info********************/

	/********************start init gendisk*********************/
	/* Initialize the gendisk of this mt_ftl_blk device */
	gd = alloc_disk(1);
	if (!gd) {
		mt_ftl_err(dev, "block: alloc_disk failed");
		ret = -ENODEV;
		goto out_free_dev;
	}
	gd->fops = &mt_ftl_blk_ops;
	gd->major = mt_ftl_blk_major;
	gd->first_minor = dev->ubi_num * UBI_MAX_VOLUMES + dev->vol_id;
	gd->private_data = dev;
	sprintf(gd->disk_name, "mt_ftl_blk%d_%d", dev->ubi_num, dev->vol_id);
	/* To do readonly in attach ftl */
	max_pages = dev->commit_node.dev_blocks * (dev->leb_size / dev->min_io_size);
	if (strstr(vi.name, "system")) {
		dev->pmt_blk_num = PMT_BLOCK_NUM >> 1;
		dev->data_start_blk = PMT_START_BLOCK + dev->pmt_blk_num;
		disk_capacity =
			((vi.used_bytes - (dev->data_start_blk + NAND_RESERVED_PEB) * vi.usable_leb_size) >> 9);
		dev->readonly = 1;
	} else {
		dev->pmt_blk_num = PMT_BLOCK_NUM;
		dev->data_start_blk = PMT_START_BLOCK + dev->pmt_blk_num;
		disk_capacity = ((vi.used_bytes -
			NAND_RESERVED_FTL((int)(dev->commit_node.dev_blocks)) * vi.usable_leb_size) >> 9);
		dev->readonly = 0;
	}
	dev->max_pmt_replay_blk = dev->pmt_blk_num << 1;
	set_capacity(gd, disk_capacity);
	dev->gd = gd;
	/********************end init gendisk*********************/

	/********************start init request queue***************/
	spin_lock_init(&dev->queue_lock);
	dev->request_type = FTL_RT_FULL;
	switch (dev->request_type) {
	case FTL_RT_FULL:
		dev->rq = blk_init_queue(mt_ftl_blk_request, &dev->queue_lock);
		break;
#ifdef MT_FTL_SUPPORT_MQ
	case FTL_RT_MAKE_QUEUE:
		dev->rq = blk_alloc_queue(GFP_KERNEL);
		blk_queue_make_request(dev->rq, mt_ftl_make_request);
		break;
	case FTL_RT_MQ_THREAD:
		ret = mt_ftl_blk_mq_init(dev);
		if (ret)
			goto out_put_disk;
	case FTL_RT_MQ:
		memset(&dev->tag_set, 0, sizeof(dev->tag_set));
		dev->tag_set.ops = &mt_ftl_mq_ops;
		dev->tag_set.queue_depth = MT_FTL_MAX_SLOTS;
		dev->tag_set.numa_node = NUMA_NO_NODE;
		dev->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
		if (dev->request_type == FTL_RT_MQ_THREAD)
			dev->tag_set.cmd_size = sizeof(struct mt_ftl_blk_rq);
		else
			dev->tag_set.cmd_size = sizeof(struct mt_ftl_blk_pdu);

		dev->tag_set.driver_data = dev;
		/*because we have ubi device support nr_cpu_ids, else default 1*/
		dev->tag_set.nr_hw_queues = 1;
		dev->tag_set.reserved_tags = 1;

		ret = blk_mq_alloc_tag_set(&dev->tag_set);
		if (ret)
			goto out_put_disk;

		dev->rq = blk_mq_init_queue(&dev->tag_set);
		break;
#endif /* MT_FTL_SUPPORT_MQ */
	default:
		mt_ftl_err(dev, "request type %d, useing full request", dev->request_type);
		dev->request_type = FTL_RT_FULL;
		dev->rq = blk_init_queue(mt_ftl_blk_request, &dev->queue_lock);
	}

	if (IS_ERR(dev->rq)) {
		mt_ftl_err(dev, "block: blk_init_queue failed");
		ret = -ENODEV;
		goto out_put_disk;
	}
	dev->rq->queuedata = dev;
	dev->gd->queue = dev->rq;

	/* ret = elevator_change(dev->rq, "noop");
	 * if (ret)
	 * goto out_put_disk;
	 */

	blk_queue_flush(dev->rq, REQ_FLUSH);
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, dev->rq);
	queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, dev->rq);
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, dev->rq);
	queue_flag_set_unlocked(QUEUE_FLAG_SECDISCARD, dev->rq);
	dev->rq->limits.discard_zeroes_data = 1;
	dev->rq->limits.discard_granularity = FS_PAGE_SIZE;
	/* please not to over UINT_MAX >> 9 */
	dev->rq->limits.max_discard_sectors = UINT_MAX >> 9;

	blk_queue_max_segments(dev->rq, MT_FTL_MAX_SG);
	blk_queue_bounce_limit(dev->rq, BLK_BOUNCE_HIGH);
	blk_queue_max_hw_sectors(dev->rq, -1U);
	blk_queue_max_segment_size(dev->rq, MT_FTL_MAX_SG_SZ);
	blk_queue_physical_block_size(dev->rq, FS_PAGE_SIZE);
	blk_queue_logical_block_size(dev->rq, FS_PAGE_SIZE);
	blk_queue_io_min(dev->rq, FS_PAGE_SIZE);

	if (dev->request_type != FTL_RT_MQ_THREAD) {
		/* if use create_workqueue, please open mutex_lock on mt_ftl_do_mq_work */
		dev->wq = alloc_ordered_workqueue("%s", WQ_MEM_RECLAIM|WQ_HIGHPRI, gd->disk_name);
		if (!dev->wq) {
			ret = -ENOMEM;
			goto out_free_queue;
		}
	}
	if (dev->request_type == FTL_RT_FULL)
		INIT_WORK(&dev->work, mt_ftl_blk_do_work);
	/********************end init request queue***************/

	mutex_lock(&devices_mutex);
	list_add_tail(&dev->list, &mt_ftl_blk_devices);
	mutex_unlock(&devices_mutex);

	/* Must be the last step: anyone can call file ops from now on */
	add_disk(dev->gd);
	if (!dev->readonly)
		mt_ftl_dev_ptr[mt_ftl_dev_count++] = dev;
	mt_ftl_blk_info(dev);

	/********************start create mt_ftl***************/
	dev->param = kzalloc(sizeof(struct mt_ftl_param), GFP_KERNEL);
	if (!dev->param) {
		ret = -ENOMEM;
		goto out_free_queue;
	}
	ret = mt_ftl_create(dev);
	if (ret) {
		ret = -EFAULT;
		goto out_free_param;
	}
	/********************end create mt_ftl***************/

	return 0;

out_free_param:
	kfree(dev->param);
	dev->param = NULL;
out_free_queue:
	blk_cleanup_queue(dev->rq);
out_put_disk:
	put_disk(dev->gd);
out_free_dev:
	kfree(dev);
	mutex_destroy(&dev->dev_mutex);

	return ret;
}

static void mt_ftl_blk_cleanup(struct mt_ftl_blk *dev)
{
	del_gendisk(dev->gd);
	destroy_workqueue(dev->wq);
	blk_cleanup_queue(dev->rq);

#ifdef MT_FTL_SUPPORT_MQ
	if (dev->request_type == FTL_RT_MQ || dev->request_type == FTL_RT_MQ_THREAD) {
		blk_mq_free_tag_set(&dev->tag_set);
		if (dev->blk_bgt)
			kthread_stop(dev->blk_bgt);
	}
#endif

	mt_ftl_cache_node_free(dev);
	pr_notice("UBI: %s released", dev->gd->disk_name);
	put_disk(dev->gd);
}

int mt_ftl_blk_remove(struct ubi_volume_info *vi)
{
	struct mt_ftl_blk *dev;
	int ret;

	mutex_lock(&devices_mutex);
	dev = find_dev_nolock(vi->ubi_num, vi->vol_id);
	if (!dev) {
		mutex_unlock(&devices_mutex);
		return -ENODEV;
	}

	ret = mt_ftl_remove(dev);
	if (ret)
		return -EFAULT;

	/* Found a device, let's lock it so we can check if it's busy */
	mutex_lock(&dev->dev_mutex);
	if (dev->refcnt > 0) {
		mutex_unlock(&dev->dev_mutex);
		mutex_unlock(&devices_mutex);
		return -EBUSY;
	}

	/* Remove from device list */
	list_del(&dev->list);
	mutex_unlock(&devices_mutex);

	mt_ftl_blk_cleanup(dev);
	mutex_unlock(&dev->dev_mutex);
	kfree(dev);
	return 0;
}

static int mt_ftl_blk_resize(struct ubi_volume_info *vi)
{
	struct mt_ftl_blk *dev;
	u64 disk_capacity = 0;
	int max_pages;

	/*
	 * Need to lock the device list until we stop using the device,
	 * otherwise the device struct might get released in
	 * 'mt_ftl_blk_remove()'.
	 */
	mutex_lock(&devices_mutex);
	dev = find_dev_nolock(vi->ubi_num, vi->vol_id);
	if (!dev) {
		mutex_unlock(&devices_mutex);
		return -ENODEV;
	}
	if ((sector_t)disk_capacity != disk_capacity) {
		mutex_unlock(&devices_mutex);
		mt_ftl_err(dev, "%s: the volume is too big (%d LEBs), cannot resize", dev->gd->disk_name, vi->size);
		return -EFBIG;
	}

	mutex_lock(&dev->dev_mutex);
	dev->commit_node.dev_blocks = dev->desc->vol->reserved_pebs - NAND_RESERVED_PEB;
	/* To do readonly in attach ftl, setting the default value */
	max_pages = dev->commit_node.dev_blocks * (dev->leb_size / dev->min_io_size);
	if (strstr(vi->name, "system")) {
		disk_capacity =
			((vi->used_bytes - (dev->data_start_blk + NAND_RESERVED_PEB) * vi->usable_leb_size) >> 9);
		dev->readonly = 1;
	} else {
		disk_capacity = ((vi->used_bytes -
			NAND_RESERVED_FTL((int)(dev->commit_node.dev_blocks)) * vi->usable_leb_size) >> 9);
		dev->readonly = 0;
	}
	dev->max_pmt_replay_blk = dev->pmt_blk_num << 1;
	if (get_capacity(dev->gd) != disk_capacity) {
		set_capacity(dev->gd, disk_capacity);
		pr_notice("UBI[%d]: %s resized to %lld bytes", vi->ubi_num, dev->gd->disk_name,
			vi->used_bytes);
	}
	mutex_unlock(&dev->dev_mutex);
	mutex_unlock(&devices_mutex);
	return 0;
}

static int mt_ftl_blk_notify(struct notifier_block *nb,
			 unsigned long notification_type, void *ns_ptr)
{
	struct ubi_notification *nt = ns_ptr;

	switch (notification_type) {
	case UBI_VOLUME_ADDED:
		/*
		 * We want to enforce explicit block device creation for
		 * volumes, so when a volume is added we do nothing.
		 */
		break;
	case UBI_VOLUME_REMOVED:
		mt_ftl_blk_remove(&nt->vi);
		break;
	case UBI_VOLUME_RESIZED:
		mt_ftl_blk_resize(&nt->vi);
		break;
	case UBI_VOLUME_UPDATED:
		/*
		 * If the volume is static, a content update might mean the
		 * size (i.e. used_bytes) was also changed.
		 */
		if (nt->vi.vol_type == UBI_STATIC_VOLUME)
			mt_ftl_blk_resize(&nt->vi);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mt_ftl_blk_notifier = {
	.notifier_call = mt_ftl_blk_notify,
};

static int mt_ftl_blk_reboot(struct notifier_block *notifier, unsigned long val, void *v)
{
	int i, closed = 0;
	struct mt_ftl_blk *dev;

	for (i = 0; i < mt_ftl_dev_count; i++) {
		pr_err("[Bean]mt_ftl_blk_reboot: commit %d!\n", i);
		dev = mt_ftl_dev_ptr[i];
		mutex_lock(&dev->dev_mutex);
		if (!dev->desc) {
			closed = 1;
			dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
		}
		mt_ftl_commit(dev);
		ubi_flush(dev->ubi_num, dev->vol_id, UBI_ALL);
		if (closed) {
			ubi_close_volume(dev->desc);
			dev->desc = NULL;
			closed = 0;
		}
		pr_err("reboot end");
		mutex_unlock(&dev->dev_mutex);
	}
	return NOTIFY_DONE;
}

static struct notifier_block mt_ftl_blk_reboot_notifier = {
	.notifier_call = mt_ftl_blk_reboot,
};

static struct ubi_volume_desc * __init
open_volume_desc(const char *name, int ubi_num, int vol_id)
{
	if (ubi_num == -1)
		/* No ubi num, name must be a vol device path */
		return ubi_open_volume_path(name, UBI_READWRITE);
	else if (vol_id == -1)
		/* No vol_id, must be vol_name */
		return ubi_open_volume_nm(ubi_num, name, UBI_READWRITE);
	else
		return ubi_open_volume(ubi_num, vol_id, UBI_READWRITE);
}

static int __init mt_ftl_blk_create_from_param(void)
{
	int i, ret = 0;
	struct mt_ftl_blk_param *p;
	struct ubi_volume_desc *desc;
	struct ubi_volume_info vi;

	for (i = 0; i < mt_ftl_blk_devs; i++) {
		p = &mt_ftl_blk_param[i];
		desc = open_volume_desc(p->name, p->ubi_num, p->vol_id);
		if (IS_ERR(desc)) {
			ret = PTR_ERR(desc);
			break;
		}

		ubi_get_volume_info(desc, &vi);
		ret = mt_ftl_blk_create(desc);
		if (ret)
			break;
		ubi_close_volume(desc);
	}
	return ret;
}

static void mt_ftl_blk_remove_all(void)
{
	struct mt_ftl_blk *next;
	struct mt_ftl_blk *dev;

	list_for_each_entry_safe(dev, next, &mt_ftl_blk_devices, list) {
		/* Flush pending work and stop workqueue */
		destroy_workqueue(dev->wq);
		/* The module is being forcefully removed */
		WARN_ON(dev->desc);
		/* Remove from device list */
		list_del(&dev->list);
		mt_ftl_blk_cleanup(dev);
		kfree(dev);
	}
}

int __init mt_ftl_blk_init(void)
{
	int ret;

	mt_ftl_dev_count = 0;
	mt_ftl_blk_major = register_blkdev(0, "mt_ftl_blk");
	if (mt_ftl_blk_major < 0)
		return mt_ftl_blk_major;

	/* Attach block devices from 'block=' module param */
	ret = mt_ftl_blk_create_from_param();
	if (ret)
		goto err_remove;

	register_reboot_notifier(&mt_ftl_blk_reboot_notifier);
	/*
	 * Block devices are only created upon user requests, so we ignore
	 * existing volumes.
	 */
	ret = ubi_register_volume_notifier(&mt_ftl_blk_notifier, 1);
	if (ret)
		goto err_unreg;
	return 0;

err_unreg:
	unregister_reboot_notifier(&mt_ftl_blk_reboot_notifier);
	unregister_blkdev(mt_ftl_blk_major, "mt_ftl_blk");
err_remove:
	mt_ftl_blk_remove_all();
	return ret;
}

void __exit mt_ftl_blk_exit(void)
{
	mt_ftl_dev_count = 0;
	unregister_reboot_notifier(&mt_ftl_blk_reboot_notifier);
	ubi_unregister_volume_notifier(&mt_ftl_blk_notifier);
	mt_ftl_blk_remove_all();
	unregister_blkdev(mt_ftl_blk_major, "mt_ftl_blk");
}

module_init(mt_ftl_blk_init);
module_exit(mt_ftl_blk_exit);
