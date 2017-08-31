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

#include "mtk_ftl.h"
#include "ubi/ubi.h"
#include <linux/crypto.h>

#define CREATE_TRACE_POINTS
#include <trace/events/mtk_ftl.h>

#ifdef MT_FTL_PROFILE
#include <linux/time.h>

unsigned long profile_time[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long start_time[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long end_time;
unsigned long start_time_all[MT_FTL_PROFILE_TOTAL_PROFILE_NUM];
unsigned long end_time_all;

unsigned long getnstimenow(void)
{
	struct timespec tv;

	getnstimeofday(&tv);
	return tv.tv_sec * 1000000000 + tv.tv_nsec;
}

#define MT_FTL_PROFILE_START(x)		(start_time[x] = getnstimenow())

#define MT_FTL_PROFILE_START_ALL(x)	(start_time_all[x] = getnstimenow())

#define MT_FTL_PROFILE_END(x)	\
	do { \
		end_time = getnstimenow(); \
		if (end_time >= start_time[x]) \
			profile_time[x] += (end_time - start_time[x]) / 1000; \
		else { \
			mt_ftl_err(dev, "end_time = %lu, start_time = %lu", end_time, start_time[x]);\
			profile_time[x] += (end_time + 0xFFFFFFFF - start_time[x]) / 1000; \
		} \
	} while (0)
#define MT_FTL_PROFILE_END_ALL(x)	\
	do { \
		end_time_all = getnstimenow(); \
		if (end_time_all >= start_time_all[x]) \
			profile_time[x] += (end_time_all - start_time_all[x]) / 1000; \
		else { \
			mt_ftl_err(dev, "end_time = %lu, start_time = %lu", end_time_all, start_time_all[x]); \
			profile_time[x] += (end_time_all + 0xFFFFFFFF - start_time_all[x]) / 1000; \
		} \
	} while (0)

char *mtk_ftl_profile_message[MT_FTL_PROFILE_TOTAL_PROFILE_NUM] = {
	"MT_FTL_PROFILE_WRITE_ALL",
	"MT_FTL_PROFILE_WRITE_COPYTOCACHE",
	"MT_FTL_PROFILE_WRITE_UPDATEPMT",
	"MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT",
	"MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT",
	"MT_FTL_PROFILE_COMMIT_PMT",
	"MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT",
	"MT_FTL_PROFILE_WRITE_COMPRESS",
	"MT_FTL_PROFILE_WRITE_WRITEPAGE",
	"MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB",
	"MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK",
	"MT_FTL_PROFILE_GETFREEBLOCK_PMT_GETLEB",
	"MT_FTL_PROFILE_GETFREEBLOCK_GETLEB",
	"MT_FTL_PROFILE_GC_FINDBLK",
	"MT_FTL_PROFILE_GC_CPVALID",
	"MT_FTL_PROFILE_GC_DATA_READOOB",
	"MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT",
	"MT_FTL_PROFILE_GC_DATA_WRITEOOB",
	"MT_FTL_PROFILE_GC_REMAP",
	"MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT",
	"MT_FTL_PROFILE_COMMIT",
	"MT_FTL_PROFILE_WRITE_PAGE_RESET",
	"MT_FTL_PROFILE_READ_ALL",
	"MT_FTL_PROFILE_READ_GETPMT",
	"MT_FTL_PROFILE_READ_DATATOCACHE",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST1",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST2",
	"MT_FTL_PROFILE_READ_DATATOCACHE_TEST3",
	"MT_FTL_PROFILE_READ_ADDRNOMATCH",
	"MT_FTL_PROFILE_READ_DECOMP",
	"MT_FTL_PROFILE_READ_COPYTOBUFF",
};
#else
#define MT_FTL_PROFILE_START(x)
#define MT_FTL_PROFILE_START_ALL(x)
#define MT_FTL_PROFILE_END(x)
#define MT_FTL_PROFILE_END_ALL(x)
#endif

#ifdef MT_FTL_TRACE_DEBUG
int ftl_debug;
#endif

static void mt_ftl_dump_data(const int *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		pr_cont("%08X ", buf[i]);
		if ((i+1) % 8 == 0)
			pr_cont("\n");
	}
}

static void mt_ftl_dump_info(struct mt_ftl_blk *dev)
{
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	mt_ftl_err(dev, "\nDump u4PMTIndicator:");
	mt_ftl_dump_data(param->u4PMTIndicator, commit_node->dev_clusters);
	mt_ftl_err(dev, "\nDump u4PMTCache:");
	mt_ftl_dump_data(param->u4PMTCache, dev->min_io_size >> 2);
	mt_ftl_err(dev, "\nDump u4MetaPMTCache:");
	mt_ftl_dump_data(param->u4MetaPMTCache, dev->min_io_size >> 2);
	mt_ftl_err(dev, "\nDump u4ReadPMTCache:");
	mt_ftl_dump_data(param->u4ReadPMTCache, dev->min_io_size >> 2);
	mt_ftl_err(dev, "\nDump u4ReadMetaPMTCache:");
	mt_ftl_dump_data(param->u4ReadMetaPMTCache, dev->min_io_size >> 2);
	mt_ftl_err(dev, "\nDump u4BIT:");
	mt_ftl_dump_data(param->u4BIT, commit_node->dev_blocks);
}

static int mt_ftl_compress(struct mt_ftl_blk *dev, void *in_buf, int in_len, int *out_len)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (in_len == 0) {
		*out_len = in_len;
		return ret;
	}
	if (!param->cc) {
		*out_len = in_len;
		param->cmpr_page_buffer = in_buf;
		return ret;
	}
	ubi_assert(in_len <= FS_PAGE_SIZE);
	memset(param->cmpr_page_buffer, 0xFF, dev->min_io_size * sizeof(unsigned char));
	ret = crypto_comp_compress(param->cc, in_buf, in_len, param->cmpr_page_buffer, out_len);
	if (ret) {
		mt_ftl_err(dev, "ret=%d, out_len=%d, in_len=0x%x", ret, *out_len, in_len);
		mt_ftl_err(dev, "cc=0x%lx, buffer=0x%lx", (unsigned long int)param->cc, (unsigned long int)in_buf);
		mt_ftl_err(dev, "cmpr_page_buffer=0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}
	if (*out_len >= FS_PAGE_SIZE) {
		/* mt_ftl_err(dev, "compress out_len(%d) large than in_len(%d)FS_PAGE_SIZE", *out_len, in_len); */
		memcpy(param->cmpr_page_buffer, in_buf, in_len);
		*out_len = in_len;
	}
	return ret;
}

static int mt_ftl_decompress(struct mt_ftl_blk *dev, void *in_buf, int in_len, int *out_len)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (!param->cc) {
		*out_len = in_len;
		param->cmpr_page_buffer = in_buf;
		return ret;
	}
	ubi_assert(in_len <= FS_PAGE_SIZE);
	memset(param->cmpr_page_buffer, 0xFF, dev->min_io_size * sizeof(unsigned char));
	if (in_len == FS_PAGE_SIZE) {
		/* mt_ftl_err(dev, "decompress out_len(%d) equal (FS_PAGE_SIZE)", in_len); */
		memcpy(param->cmpr_page_buffer, in_buf, in_len);
		*out_len = in_len;
	} else {
		ret = crypto_comp_decompress(param->cc, in_buf, in_len, param->cmpr_page_buffer, out_len);
		if (ret) {
			mt_ftl_err(dev, "ret=%d, out_len=%d, in_len=0x%x", ret, *out_len, in_len);
			mt_ftl_err(dev, "cmpr_page_buffer=0x%lx", (unsigned long int)param->cmpr_page_buffer);
			return MT_FTL_FAIL;
		}
	}
	return ret;
}

#ifdef MT_FTL_SUPPORT_WBUF

#define MT_FTL_WBUF_NOSYNC(dev)    ((dev)->need_sync = 0)

static int mt_ftl_wbuf_init(struct mt_ftl_blk *dev)
{
	int i, num;
	struct mt_ftl_wbuf_entry *ftl_wbuf;

	/* mutex_init(&dev->wbuf_mutex); */
	init_waitqueue_head(&dev->wbuf_wait);
	init_waitqueue_head(&dev->sync_wait);
	INIT_LIST_HEAD(&dev->wbuf_list);
	INIT_LIST_HEAD(&dev->pool_free_list);
	spin_lock_init(&dev->wbuf_lock);
	spin_lock_init(&dev->pool_lock);
	dev->wbuf_pool_num = 0;
	dev->wbuf_count = 0;
	memset(dev->pool_bitmap, 'E', MT_FTL_MAX_WBUF_NUM);
	dev->wbuf_pool[0] = vmalloc(dev->leb_size);
	if (!dev->wbuf_pool[0]) {
		mt_ftl_err(dev, "alloc wbuf_pool[%d] FAIL", dev->wbuf_pool_num);
		return -ENOMEM;
	}
	num = dev->leb_size / dev->min_io_size;
	for (i = 0; i < num; i++) {
		ftl_wbuf = kzalloc(sizeof(struct mt_ftl_wbuf_entry), GFP_KERNEL);
		if (!ftl_wbuf) {
			mt_ftl_err(dev, "alloc wbuf_entry FAIL");
			return -ENOMEM;
		}
		ftl_wbuf->pool_num = 0;
		ftl_wbuf->lnum = -1;
		ftl_wbuf->offset = -1;
		ftl_wbuf->len = -1;
		ftl_wbuf->wbuf = dev->wbuf_pool[0] + dev->min_io_size * i;
		ftl_wbuf->dev = NULL;
		spin_lock(&dev->pool_lock);
		list_add_tail(&ftl_wbuf->list, &dev->pool_free_list);
		dev->wbuf_count++;
		spin_unlock(&dev->pool_lock);
	}
	spin_lock(&dev->pool_lock);
	dev->pool_bitmap[0] = 'B';
	dev->wbuf_pool_num++;
	spin_unlock(&dev->pool_lock);
	return 0;
}

static int mt_ftl_wbuf_get_pool(struct mt_ftl_blk *dev)
{
	int i;

	for (i = 0; i < MT_FTL_MAX_WBUF_NUM; i++) {
		if (dev->pool_bitmap[i] == 'E')
			return i;
	}
	return -1;
}

static int mt_ftl_wbuf_expand(struct mt_ftl_blk *dev)
{
	int i, num, pool_num;
	struct mt_ftl_wbuf_entry *ftl_wbuf;

	if (dev->wbuf_pool_num >= MT_FTL_MAX_WBUF_NUM)
		return MT_FTL_FAIL;
	pool_num = mt_ftl_wbuf_get_pool(dev);
	dev->wbuf_pool[pool_num] = vmalloc(dev->leb_size);
	if (!dev->wbuf_pool[pool_num]) {
		mt_ftl_err(dev, "alloc wbuf_pool[%d] FAIL", pool_num);
		return -ENOMEM;
	}
	num = dev->leb_size / dev->min_io_size;
	for (i = 0; i < num; i++) {
		ftl_wbuf = kzalloc(sizeof(struct mt_ftl_wbuf_entry), GFP_KERNEL);
		if (!ftl_wbuf) {
			mt_ftl_err(dev, "alloc wbuf_entry FAIL");
			return -ENOMEM;
		}
		ftl_wbuf->pool_num = pool_num;
		ftl_wbuf->lnum = -1;
		ftl_wbuf->offset = -1;
		ftl_wbuf->len = -1;
		ftl_wbuf->wbuf = dev->wbuf_pool[pool_num] + dev->min_io_size * i;
		ftl_wbuf->dev = NULL;
		spin_lock(&dev->pool_lock);
		list_add_tail(&ftl_wbuf->list, &dev->pool_free_list);
		dev->wbuf_count++;
		spin_unlock(&dev->pool_lock);
	}
	spin_lock(&dev->pool_lock);
	dev->pool_bitmap[pool_num] = 'B';
	dev->wbuf_pool_num++;
	spin_unlock(&dev->pool_lock);
	mt_ftl_err(dev, "expand up to %d %d", pool_num, dev->wbuf_pool_num);
	return 0;
}

static void mt_ftl_wbuf_shrink(struct mt_ftl_blk *dev)
{
	int i, num, pool_num[MT_FTL_MAX_WBUF_NUM];
	struct mt_ftl_wbuf_entry *rr, *n;

	if (dev->wbuf_pool_num == 1)
		return;

	num = dev->leb_size / dev->min_io_size;
	if (dev->wbuf_count < num)
		return;
	memset(pool_num, 0, sizeof(int) * MT_FTL_MAX_WBUF_NUM);
	for (i = 0; i < dev->wbuf_pool_num; i++) {
		spin_lock(&dev->pool_lock);
		list_for_each_entry_safe(rr, n, &dev->pool_free_list, list) {
			if (i == rr->pool_num && dev->pool_bitmap[i] == 'A')
				pool_num[i]++;
		}
		if (pool_num[i] == num) {
			mt_ftl_err(dev, "found %d, need to shrink %d:%d", i, dev->wbuf_pool_num, dev->wbuf_count);
			list_for_each_entry_safe(rr, n, &dev->pool_free_list, list) {
				if (i == rr->pool_num) {
					list_del_init(&rr->list);
					kfree(rr);
					dev->wbuf_count--;
					rr = NULL;
				}
			}
			dev->pool_bitmap[i] = 'E';
			dev->wbuf_pool_num--;
			spin_unlock(&dev->pool_lock);
			vfree(dev->wbuf_pool[i]);
			/* wait write buffer */
			if (waitqueue_active(&dev->wbuf_wait)) {
				mt_ftl_err(dev, "wakeup wbuf1");
				wake_up(&dev->wbuf_wait);
			}
			schedule();
			continue;
		}
		spin_unlock(&dev->pool_lock);
	}
}

static int mt_ftl_wbuf_flush(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_wbuf_entry *ww;

	if (list_empty(&dev->wbuf_list)) {
		dev->need_bgt = 0;
		mt_ftl_wbuf_shrink(dev);
		/* wait write buffer */
		if (waitqueue_active(&dev->wbuf_wait))
			wake_up(&dev->wbuf_wait);
		/* wait sync */
		if (waitqueue_active(&dev->sync_wait))
			wake_up(&dev->sync_wait);
		cond_resched();
		return ret;
	}
	spin_lock(&dev->wbuf_lock);
	ww = list_first_entry(&dev->wbuf_list, struct mt_ftl_wbuf_entry, list);
	list_del_init(&ww->list);
	spin_unlock(&dev->wbuf_lock);
	/* mt_ftl_err(dev, "w%d %d %d(%lx:%lx:%lx)", ww->lnum, ww->offset, ww->len,
	 * (unsigned long)ww->wbuf, (unsigned long)desc, (unsigned long)dev->desc);
	 */
	ret = ubi_leb_write(ww->dev->desc, ww->lnum, ww->wbuf, ww->offset, ww->len);
	if (ret) {
		mt_ftl_err(dev, "Write failed:leb=%u,offset=%d,len=%d ret=%d",
			ww->lnum, ww->offset, ww->len, ret);
		ubi_assert(false);
		return ret;
	}
	spin_lock(&dev->pool_lock);
	list_add_tail(&ww->list, &dev->pool_free_list);
	dev->wbuf_count++;
	dev->pool_bitmap[ww->pool_num] = 'A';
	spin_unlock(&dev->pool_lock);
	if (dev->wbuf_pool_num > MT_FTL_WBUF_THRESHOLD) {
		mt_ftl_wbuf_shrink(dev);
		/* wait write buffer */
		if (waitqueue_active(&dev->wbuf_wait)) {
			mt_ftl_err(dev, "wakeup wbuf2");
			wake_up(&dev->wbuf_wait);
		}
	}
	return ret;
}

static void mt_ftl_wakeup_wbuf_thread(struct mt_ftl_blk *dev)
{
	if (dev->bgt && !dev->need_bgt && !dev->readonly) {
		dev->need_bgt = 1;
		wake_up_process(dev->bgt);
	}
}

int mt_ftl_wbuf_forced_flush(struct mt_ftl_blk *dev)
{
	/* wait sync */
	if (!waitqueue_active(&dev->sync_wait)) {
		mt_ftl_wakeup_wbuf_thread(dev);
		wait_event(dev->sync_wait, list_empty(&dev->wbuf_list));
	}

	return 0;
}

int mt_ftl_wbuf_thread(void *info)
{
	int err;
	struct mt_ftl_blk *dev = info;

	mt_ftl_err(dev, "FTL wbuf thread started, PID %d", current->pid);
	set_freezable();

	while (1) {
		if (kthread_should_stop())
			break;

		if (try_to_freeze())
			continue;

		set_current_state(TASK_INTERRUPTIBLE);
		/* Check if there is something to do */
		if (!dev->need_bgt) {
			if (kthread_should_stop())
				break;
			schedule();
			continue;
		} else
			__set_current_state(TASK_RUNNING);

		mutex_lock(&dev->dev_mutex);
		err = mt_ftl_wbuf_flush(dev);
		mutex_unlock(&dev->dev_mutex);
		if (err)
			return err;

		cond_resched();
	}

	mt_ftl_err(dev, "FTL wbuf thread stops");
	return 0;
}

static int mt_ftl_wbuf_alloc(struct mt_ftl_blk *dev)
{
	int err;

	dev->need_sync = 1;
	dev->need_bgt = 0;
	dev->bgt = NULL;
	if (!dev->readonly) {
		err = mt_ftl_wbuf_init(dev);
		if (err < 0)
			return err;
		/* Create background thread */
		sprintf(dev->bgt_name, "ftl_wbuf%d_%d", dev->ubi_num, dev->vol_id);
		dev->bgt = kthread_create(mt_ftl_wbuf_thread, dev, "%s", dev->bgt_name);
		if (IS_ERR(dev->bgt)) {
			err = PTR_ERR(dev->bgt);
			dev->bgt = NULL;
			mt_ftl_err(dev, "cannot spawn \"%s\", error %d", dev->bgt_name, err);
			return MT_FTL_FAIL;
		}
		wake_up_process(dev->bgt);
	}
	return MT_FTL_SUCCESS;
}

static void mt_ftl_wbuf_free(struct mt_ftl_blk *dev)
{
	int i;
	struct mt_ftl_wbuf_entry *dd, *n;

	dev->need_sync = 1;
	dev->need_bgt = 0;
	if (!dev->readonly) {
		if (dev->bgt)
			kthread_stop(dev->bgt);
		list_for_each_entry_safe(dd, n, &dev->wbuf_list, list) {
			list_del_init(&dd->list);
			kfree(dd);
		}
		list_for_each_entry_safe(dd, n, &dev->pool_free_list, list) {
			list_del_init(&dd->list);
			kfree(dd);
		}
		for (i = 0; i < MT_FTL_MAX_WBUF_NUM; i++) {
			if (!dev->wbuf_pool[i])
				vfree(dev->wbuf_pool);
		}
		/* mutex_destroy(&dev->wbuf_mutex); */
	}
}
#else
#define MT_FTL_WBUF_NOSYNC(dev)
#define mt_ftl_wbuf_alloc(dev) (MT_FTL_SUCCESS)
#define mt_ftl_wbuf_free(dev)
#endif


static void mt_ftl_leb_remap(struct mt_ftl_blk *dev, int lnum)
{
	int ret;

	mt_ftl_debug(dev, "eba_tbl[%d]=%d to unmap", lnum, dev->desc->vol->eba_tbl[lnum]);
	ret = ubi_is_mapped(dev->desc, lnum);
	if (ret) {
		ubi_leb_unmap(dev->desc, lnum);
		ubi_leb_map(dev->desc, lnum);
	}
	mt_ftl_debug(dev, "eba_tbl[%d]=%d has mapped", lnum, dev->desc->vol->eba_tbl[lnum]);
}

#ifdef MT_FTL_SUPPORT_WBUF
static int mt_ftl_leb_read(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	int copy_offset = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_wbuf_entry *rr, *n;

	if (!dev->readonly && lnum >= dev->data_start_blk) {
		spin_lock(&dev->wbuf_lock);
		list_for_each_entry_safe(rr, n, &dev->wbuf_list, list) {
			if ((rr->lnum == lnum) && (rr->offset <= offset) &&
				((offset + len) <= (rr->offset + rr->len))) {
				copy_offset = (offset == rr->offset) ? 0 : (offset % dev->min_io_size);
				mt_ftl_err(dev, "rbuf read %d %d %d %d", lnum, offset, copy_offset, len);
				memcpy(buf, rr->wbuf + copy_offset, len);
				spin_unlock(&dev->wbuf_lock);
				return MT_FTL_SUCCESS;
			}
		}
		spin_unlock(&dev->wbuf_lock);
	}
	ret = ubi_leb_read(desc, lnum, (char *)buf, offset, len, 0);
	if (ret) {
		mt_ftl_err(dev, "Read failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
		ubi_assert(false);
		return MT_FTL_FAIL;
	}
	cond_resched();
	return ret;
}

static int mt_ftl_leb_write(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_wbuf_entry *ftl_wbuf;
	/* unsigned long flags; */

	if (dev->readonly)
		dev->need_sync = 1;

	if (dev->need_sync || lnum < dev->data_start_blk) {
retry:
		ret = ubi_leb_write(desc, lnum, buf, offset, len);
		if (ret) {
			if (ret == -EROFS) {
				mt_ftl_err(dev, "RO Volume, switch to RW");
				if (!dev->readonly)
					dump_stack();
				ubi_close_volume(dev->desc);
				dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
				desc = dev->desc;
				goto retry;
			}
			mt_ftl_err(dev, "Write failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
			ubi_assert(false);
			return MT_FTL_FAIL;
		}
		cond_resched();
		return ret;
	}
	ubi_assert(len <= dev->min_io_size);
	/* if (list_empty(&dev->pool_free_list)) {
	 * mt_ftl_wakeup_wbuf_thread(dev);
	 * wait_event_timeout(dev->wbuf_wait, !list_empty(&dev->pool_free_list),
	 * MT_FTL_TIMEOUT_MSECS * HZ / 1000);
	 * }
	 */
	if (list_empty(&dev->pool_free_list)) {
		ret = mt_ftl_wbuf_expand(dev);
		if (ret) {
			ret = MT_FTL_SUCCESS;
			mt_ftl_err(dev, "sleep wbuf");
			mt_ftl_wakeup_wbuf_thread(dev);
			wait_event_timeout(dev->wbuf_wait, !list_empty(&dev->pool_free_list),
				MT_FTL_TIMEOUT_MSECS * HZ / 1000);
		}
	}
	spin_lock(&dev->pool_lock);
	ftl_wbuf = list_first_entry(&dev->pool_free_list, struct mt_ftl_wbuf_entry, list);
	list_del_init(&ftl_wbuf->list);
	dev->wbuf_count--;
	spin_unlock(&dev->pool_lock);
	ftl_wbuf->lnum = lnum;
	ftl_wbuf->offset = offset;
	ftl_wbuf->len = len;
	ftl_wbuf->dev = dev;
	memset(ftl_wbuf->wbuf, 0xFF, dev->min_io_size);
	memcpy(ftl_wbuf->wbuf, buf, len);
	/* mt_ftl_err(dev, "ww%d %d %d(%lx)", lnum, offset, len, (unsigned long)ftl_wbuf->wbuf); */
	spin_lock(&dev->wbuf_lock);
	list_add_tail(&ftl_wbuf->list, &dev->wbuf_list);
	spin_unlock(&dev->wbuf_lock);
	dev->need_sync = 1;
	mt_ftl_wakeup_wbuf_thread(dev);
	return ret;
}
#else
static int mt_ftl_leb_read(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;

	trace_mt_ftl_leb_read(dev, lnum, offset, len);
	ret = ubi_leb_read(desc, lnum, (char *)buf, offset, len, 0);
	if (ret) {
		mt_ftl_err(dev, "Read failed:leb=%u,peb=%u,offset=%d ret=%d", lnum, desc->vol->eba_tbl[lnum],
			offset, ret);
		mt_ftl_show_commit_node(dev);
		ret = MT_FTL_FAIL;
		ubi_assert(false);
	}
	return ret;
}
static int mt_ftl_leb_write(struct mt_ftl_blk *dev, int lnum, void *buf, int offset, int len)
{
	int ret = MT_FTL_SUCCESS;
	struct ubi_volume_desc *desc = dev->desc;

retry:
	trace_mt_ftl_leb_write(dev, lnum, offset, len);
	ret = ubi_leb_write(desc, lnum, buf, offset, len);
	if (ret) {
		if (ret == -EROFS) {
			mt_ftl_err(dev, "RO Volume, switch to RW");
			if (!dev->readonly)
				dump_stack();
			ubi_close_volume(dev->desc);
			dev->desc = ubi_open_volume(dev->ubi_num, dev->vol_id, UBI_READWRITE);
			desc = dev->desc;
			goto retry;
		}
		mt_ftl_err(dev, "Write failed:leb=%u,offset=%d ret=%d", lnum, offset, ret);
		mt_ftl_show_commit_node(dev);
		ret = MT_FTL_FAIL;
		ubi_assert(false);
	}
	return ret;
}
#endif
static int mt_ftl_leb_recover(struct mt_ftl_blk *dev, int lnum, int size, int check)
{
	int ret = MT_FTL_FAIL, offset = size;
	unsigned char *tmpbuf = NULL;
	struct mt_ftl_param *param = dev->param;

	if (size == 0)  {
		mt_ftl_err(dev, "size(%d)==0", size);
		ubi_leb_unmap(dev->desc, lnum);
		return MT_FTL_SUCCESS;
	}
	if (size >= dev->leb_size) {
		mt_ftl_err(dev, "size(%d)>=leb_size(%d)", size, dev->leb_size);
		return MT_FTL_SUCCESS;
	}
	if (check) {
		ret = mt_ftl_leb_read(dev, lnum, param->replay_page_buffer, offset, dev->min_io_size);
		if (ret == MT_FTL_SUCCESS) {
			offset += dev->min_io_size;
			if (offset >= dev->leb_size) {
				mt_ftl_err(dev, "offset(%d) over leb_size(%d)", offset, dev->leb_size);
				return ret;
			}
			mt_ftl_err(dev, "lnum(%d) read offset(%d) OK, read next(%d) for check", lnum, size, offset);
			ret = mt_ftl_leb_read(dev, lnum, param->replay_page_buffer, offset, dev->min_io_size);
		}
	}
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover leb(%d) size(%d)", lnum, size);
		tmpbuf = vmalloc(dev->leb_size);
		if (!tmpbuf) {
			ubi_assert(false);
			ret = MT_FTL_FAIL;
			goto out;
		}
		/* read if ecc error or not */
		ret = mt_ftl_leb_read(dev, lnum, tmpbuf, 0, size);
		ubi_leb_unmap(dev->desc, lnum);
		ret = mt_ftl_leb_write(dev, lnum, tmpbuf, 0, size);
		if (ret)
			goto out;
		ret = 1; /* recover ok */
	}
out:
	if (tmpbuf) {
		vfree(tmpbuf);
		tmpbuf = NULL;
	}
	return ret;
}

static int mt_ftl_get_last_config_offset(struct mt_ftl_blk *dev, int leb)
{
	int ret = MT_FTL_SUCCESS;
	int offset = 0;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	while (offset <= max_offset_per_block) {
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, offset, dev->min_io_size);
		if (param->tmp_page_buffer[0] != MT_MAGIC_NUMBER) {
			/* mt_ftl_err(dev, "[INFO] Get last page in leb:%d, page:%d", leb, offset / NAND_PAGE_SIZE); */
			break;
		}
		offset += dev->min_io_size;
	}
	return offset;
}


/* des sort */
static void mt_ftl_bit_sort(struct mt_ftl_blk *dev, int *arr, int start, int len)
{
	int i, j;
	int temp = 0;
	struct mt_ftl_param *param = dev->param;

	for (i = 0; i < len; i++)
		arr[i] = start + i;
	for (i = 1; i < len; i++) {
		temp = arr[i];
		for (j = i - 1; j >= 0 && (param->u4BIT[arr[j]] < param->u4BIT[temp]); j--)
			arr[j + 1] = arr[j];
		arr[j + 1] = temp;
	}
}

static int mt_ftl_get_move_num(struct mt_ftl_blk *dev, int *mv_arr, int start, int len)
{
	int i = 0, j = 0, ret = 0;
	int *arr = NULL;
	int valid = 0;
	struct mt_ftl_param *param = dev->param;
	int threshold = dev->leb_size / MT_CHARGE_THRESHOLD;

	arr = param->tmp_page_buffer;
	mt_ftl_bit_sort(dev, arr, start, len);
	if (arr[0] == param->gc_pmt_charge_blk) {
		threshold <<= 1;
		if (param->u4BIT[arr[1]] >= threshold)
			goto check_move;
		return 0;
	}
	valid = dev->leb_size - param->u4BIT[arr[0]];
	mv_arr[j++] = arr[0];
check_move:
	for (i = 1; i < PMT_CHARGE_NUM; i++) {
		if (arr[i] == param->gc_pmt_charge_blk)
			continue;
		valid += (dev->leb_size - param->u4BIT[arr[i]]);
		mv_arr[j++] = arr[i];
		if (valid >= dev->leb_size) {
			ret = i + 1;
			if (param->u4BIT[arr[i]] < threshold) {
				mt_ftl_err(dev, "bit balance arr[%d]=%d, blk=%d",
					i, arr[i], param->gc_pmt_charge_blk);
				ret = 0;
			}
			break;
		}
		threshold >>= 1;
	}
	return ret;
}

static int mt_ftl_gc_find_block(struct mt_ftl_blk *dev, int start_leb, int end_leb, bool ispmt, bool *invalid)
{
	int i = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	int return_leb = MT_INVALID_BLOCKPAGE;
	int max_invalid = 0, skip_leb = commit_node->u4GCReserveLeb;
	int total_invalid = dev->leb_size - (dev->leb_size / dev->min_io_size * 4);

	if (start_leb > commit_node->dev_blocks || end_leb > commit_node->dev_blocks) {
		mt_ftl_err(dev, "u4StartLeb(%d)u4EndLeb(%d) is larger than NAND_TOTAL_BLOCK_NUM(%d)",
			start_leb, end_leb, commit_node->dev_blocks);
		return MT_FTL_FAIL;
	}
	if (ispmt) {
		total_invalid = dev->leb_size;
		skip_leb = commit_node->u4PMTChargeLebIndicator;
	}
	for (i = start_leb; i < end_leb; i++) {
		if (i == skip_leb)
			continue;
		if (param->u4BIT[i] >= total_invalid) {
			return_leb = i;
			*invalid = true;
			mt_ftl_debug(dev, "u4BIT[%d] = %d", return_leb, param->u4BIT[return_leb]);
			return return_leb;
		}
		if (param->u4BIT[i] > max_invalid) {
			max_invalid = param->u4BIT[i];
			return_leb = i;
			mt_ftl_debug(dev, "BIT[%d] = %d(%d:%d)", return_leb, param->u4BIT[return_leb],
				start_leb, end_leb);
		}
	}

	*invalid = false;
	return return_leb;
}

static int mt_ftl_check_replay_commit(struct mt_ftl_blk *dev, int leb, bool ispmt, bool last)
{
	int i, commit = 0, data_leb = 0;
	unsigned int *rec = NULL, index = 0;
	int rec_leb = -1;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	index = ispmt ? param->replay_pmt_blk_index : param->replay_blk_index;
	rec = ispmt ? param->replay_pmt_blk_rec : param->replay_blk_rec;
	for (i = 0; i < index; i++) {
		if (ispmt)
			rec_leb = GET_PMT_REC_BLK(rec[i]);
		else
			rec_leb = rec[i];
		if (leb != rec_leb)
			continue;
		mt_ftl_err(dev, "u4SrcInvalidLeb(%d)==replay_blk_rec[%d](0x%x)", leb, i, rec[i]);
		if (!ispmt && last) {
			data_leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
			PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb,
				dev->leb_size / dev->min_io_size);
		}
		commit = 1;
		break;
	}
	return commit;
}

/* Replace  mt_ftl_bit_update with BIT_UPDATE */
#if 0
static inline void mt_ftl_bit_update(struct mt_ftl_blk *dev, int leb, int size, bool replay)
{
	int commit;
	struct mt_ftl_param *param = dev->param;
	int total_invalid = dev->leb_size - (dev->leb_size / dev->min_io_size * 4);

	param->u4BIT[leb] += size;
	return;
	/* skip replay */
	if (replay)
		return;
	/* skip non-data-blk */
	if (leb < dev->data_start_blk)
		return;
	if (total_invalid == param->u4BIT[leb]) {
		/* skip in replay block */
		commit = mt_ftl_check_replay_commit(dev, leb, false, false);
		if (commit)
			return;
		/* if BIT full to unmap */
		ubi_leb_unmap(dev->desc, leb);
	}
	if (total_invalid < param->u4BIT[leb])
		mt_ftl_err(dev, "[Bean BIT]error bit update %d %d", leb, param->u4BIT[leb]);
}
#endif

/* The process for PMT should traverse PMT indicator
 * if block num of items in PMT indicator is equal to u4SrcInvalidLeb, copy the
 * corresponding page to new block, and update corresponding PMT indicator
 */
static int mt_ftl_gc_pmt(struct mt_ftl_blk *dev, int sleb, int dleb, int *offset_dst, bool replay)
{
	int ret = MT_FTL_SUCCESS, i, j;
	int offset_src = 0, dirty = 0;
	int page = 0, cache_num = 0;
	int count = 0, move_num = 0;
	int tmp_offset = *offset_dst;
	int mv_arr[PMT_CHARGE_NUM] = {0};
	int commit = 0, pmt_num = 2;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;

	if (replay && param->replay_pmt_last_cmit) {
		for (j = 0; j < page_num_per_block; j++) {
			/* consider discard, check every page to judge the block which need replay or not */
			ret = mt_ftl_leb_read(dev, sleb, param->gc_pmt_buffer, j * dev->min_io_size, dev->min_io_size);
			for (i = 0; i < (dev->min_io_size >> 2); i++) {
				if (param->gc_pmt_buffer[i] != NAND_DEFAULT_VALUE) {
					mt_ftl_err(dev, "replay the last gc_pmt %d %d %d", sleb, dleb, j);
					replay = false;
					break;
				}
			}
			if (replay == false)
				break;
		}
	}
	if ((replay == true) && (param->replay_pmt_last_cmit))
		mt_ftl_err(dev, "gc_pmt:sleb is empty");
	if (!replay)
		mt_ftl_leb_remap(dev, dleb);
move_pmt:
	/* use tmp_page_buffer replace of gc_page_buffer, because gc data would update PMT & gc pmt */
	for (i = 0; i < commit_node->dev_clusters; i++) {
		if (PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[i]) != sleb)
			continue;
		offset_src = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[i]) * dev->min_io_size;
		if (!replay) {
			mt_ftl_debug(dev, "Copy PMT&META PMT %d %d %d %d", sleb, offset_src, dleb, tmp_offset);
			for (j = 0; j < pmt_num; j++) {
				/* Copy PMT & META PMT */
				ret = mt_ftl_leb_read(dev, sleb, param->gc_pmt_buffer,
					offset_src + j * dev->min_io_size, dev->min_io_size);
				if (ret)
					return ret;
				ret = mt_ftl_leb_write(dev, dleb, param->gc_pmt_buffer,
					tmp_offset + j * dev->min_io_size, dev->min_io_size);
				if (ret)
					return ret;
			}
		}
		BIT_UPDATE(param->u4BIT[sleb], pmt_num * dev->min_io_size);
		dirty = PMT_INDICATOR_IS_DIRTY(param->u4PMTIndicator[i]);
		cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[i]);
		page = tmp_offset / dev->min_io_size;
		PMT_INDICATOR_SET_BLOCKPAGE(param->u4PMTIndicator[i], dleb, page, dirty, cache_num);
		mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x, dirty = %d, cache_num = %d, page = %d %d %d\n",
			i, param->u4PMTIndicator[i], dirty, cache_num, page, ret, pmt_num);	/* Temporary */
		tmp_offset += pmt_num * dev->min_io_size;
		if (tmp_offset >= dev->leb_size) {
			mt_ftl_debug(dev, "tmp_offset(%d) >= dev->leb_size", tmp_offset);
			break;
		}
	}

	if (count == 0) {
		param->u4BIT[sleb] = 0;
		*offset_dst = tmp_offset;
		return ret;
		move_num = mt_ftl_get_move_num(dev, mv_arr, PMT_START_BLOCK, dev->pmt_blk_num);
		if (move_num == 0) {
			mt_ftl_err(dev, "NO_MOVE");
			return ret;
		}
		tmp_offset = 0;
		commit = mt_ftl_check_replay_commit(dev, commit_node->u4PMTChargeLebIndicator, true, true);
		if (commit) {
			param->get_pmt_blk_flag = 1;
			return ret;
		}
		if (!replay)
			mt_ftl_leb_remap(dev, commit_node->u4PMTChargeLebIndicator);

		mt_ftl_debug(dev, "retry=0, offset_dst = %d, sleb = %d", *offset_dst, sleb);
	}
	if (count < PMT_CHARGE_NUM) {
		sleb = mv_arr[count++];
		dleb = commit_node->u4PMTChargeLebIndicator;
		mt_ftl_debug(dev, "sleb=%d, dleb=%d", sleb, dleb);
		if (sleb) {
			mt_ftl_err(dev, "move begin, tmp_offset=%d, count=%d", tmp_offset, count);
			goto move_pmt;
		}
	}
	param->gc_pmt_charge_blk = commit_node->u4PMTChargeLebIndicator;
	commit_node->u4PMTChargeLebIndicator = mv_arr[0];
	param->u4BIT[commit_node->u4PMTChargeLebIndicator] = 0;
	mt_ftl_debug(dev, "u4PMTChargeLebIndicator = %d, count=%d, gc_pmt_charge_blk = %d",
		commit_node->u4PMTChargeLebIndicator, count, param->gc_pmt_charge_blk);
	mt_ftl_debug(dev, "BIT[3]=%d, BIT[4]=%d, BIT[5]=%d, BIT[6]=%d, BIT[7]=%d, BIT[8]=%d", param->u4BIT[3],
		param->u4BIT[4], param->u4BIT[5], param->u4BIT[6], param->u4BIT[7], param->u4BIT[8]);
	return ret;
}

static int mt_ftl_check_header(struct mt_ftl_blk *dev, struct mt_ftl_data_header *header, int num)
{
	int i;
	sector_t sec = NAND_DEFAULT_SECTOR_VALUE;

	for (i = 0; i < num; i++)
		sec &= header[i].sector;

	if (sec == NAND_DEFAULT_SECTOR_VALUE) {
		mt_ftl_debug(dev, "all sectors are 0xFFFFFFFF");
		return 0;
	}
	return 1;
}

static int mt_ftl_get_pmt(struct mt_ftl_blk *dev, sector_t sector, u32 *pmt, u32 *meta_pmt)
{
	int ret = MT_FTL_SUCCESS;
	u32 cluster = 0, sec_offset = 0;
	int cache_num = 0;
	int pm_per_page = dev->pm_per_io;
	int page_size = dev->min_io_size;
	int pmt_block = 0, pmt_page = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	/* Calculate clusters and sec_offsets */
	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> pm_per_page);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << pm_per_page) - 1));
	ubi_assert(cluster <= commit_node->dev_clusters);
	/* Download PMT to read PMT cache */
	/* Don't use mt_ftl_updatePMT, that will cause PMT indicator mixed in replay */
	if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
		cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
		ubi_assert(cache_num < PMT_CACHE_NUM);
		*pmt = param->u4PMTCache[(cache_num << pm_per_page) + sec_offset];
		*meta_pmt = param->u4MetaPMTCache[(cache_num << pm_per_page) + sec_offset];
	} else if (cluster == param->i4CurrentReadPMTClusterInCache) {
		/* mt_ftl_err(dev,  "cluster == i4CurrentReadPMTClusterInCache (%d)",
		 * param->i4CurrentReadPMTClusterInCache);
		 */
		*pmt = param->u4ReadPMTCache[sec_offset];
		*meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
	} else {
		pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
		pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[cluster]);

		if (unlikely(pmt_block == 0) ||
			unlikely(pmt_block == PMT_INDICATOR_GET_BLOCK(NAND_DEFAULT_VALUE))) {
			mt_ftl_debug(dev, "pmt_block == 0");
			memset(param->u4ReadPMTCache, 0xFF, sizeof(unsigned int) << pm_per_page);
			memset(param->u4ReadMetaPMTCache, 0xFF, sizeof(unsigned int) << pm_per_page);
			param->i4CurrentReadPMTClusterInCache = NAND_DEFAULT_VALUE;
		} else {
			mt_ftl_debug(dev, "Get PMT of cluster (%d)", cluster);
			ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadPMTCache,
				pmt_page * page_size, page_size);
			if (ret)
				goto out;
			ret = mt_ftl_leb_read(dev, pmt_block, param->u4ReadMetaPMTCache,
					(pmt_page + 1) * page_size, page_size);
			if (ret)
				goto out;
			param->i4CurrentReadPMTClusterInCache = cluster;
		}
		*pmt = param->u4ReadPMTCache[sec_offset];
		*meta_pmt = param->u4ReadMetaPMTCache[sec_offset];
	}

	if (*pmt == MT_INVALID_BLOCKPAGE) {
		mt_ftl_err(dev, "PMT of sector(0x%lx) is invalid", (unsigned long int)sector);
		ret = MT_FTL_FAIL;
	}
out:
	return ret;
}

static int mt_ftl_gc_check_data(struct mt_ftl_blk *dev, struct mt_ftl_data_header *header_buffer,
	struct mt_ftl_valid_data *valid_data, int data_num, int *last_data_len, bool replay)
{
	int i = 0, ret = MT_FTL_SUCCESS;
	int last_data_len_src = 0;
	u32 pmt = 0, meta_pmt = 0;
	sector_t sector = 0;
	u32 total_consumed_size = 0;
	u32 offset_in_page = 0, data_len = 0;
	int page_size = dev->min_io_size;
	struct mt_ftl_data_header valid_header;
	u32 check_num = 0;
	struct mt_ftl_param *param = dev->param;
	/* using replay_page_buffer as temp buffer here */
	char *last_data_buffer = (char *)param->replay_page_buffer;

	memset(last_data_buffer, 0xFF, page_size);
	for (i = 0; i < data_num; i++) {
		/* Get sector in the page */
		sector = header_buffer[data_num - i - 1].sector;
		if ((sector & NAND_DEFAULT_SECTOR_VALUE) == NAND_DEFAULT_SECTOR_VALUE)
			continue;
		if (replay) {
			valid_data->valid_data_size += (FDATA_LEN(header_buffer[data_num - i - 1].offset_len)
				+ sizeof(struct mt_ftl_data_header));
			mt_ftl_updatePMT(dev, sector, valid_data->info.dst_leb,
					 valid_data->info.page_dst * page_size, i,
					 FDATA_LEN(header_buffer[data_num - i - 1].offset_len), replay, false);
			continue;
		}
		if (FDATA_LEN(header_buffer[data_num - i - 1].offset_len) == 0)
			continue;
		ret = mt_ftl_get_pmt(dev, sector, &pmt, &meta_pmt);
		if (ret)
			goto out;
		if ((!replay) &&
			(valid_data->info.src_leb == PMT_GET_BLOCK(pmt)) &&
			(valid_data->info.page_src == PMT_GET_PAGE(pmt)) &&
			(i == PMT_GET_PART(meta_pmt))) {
			offset_in_page = FDATA_OFFSET(header_buffer[data_num - i - 1].offset_len);
			data_len = FDATA_LEN(header_buffer[data_num - i - 1].offset_len);
			valid_data->valid_data_size += (data_len + sizeof(struct mt_ftl_data_header));
			ubi_assert((data_len <= FS_PAGE_SIZE) && (offset_in_page < page_size));
			last_data_len_src = 0;
			total_consumed_size = valid_data->valid_data_offset + data_len +
				(valid_data->valid_data_num + 1) * sizeof(struct mt_ftl_data_header) + 4;
			if (total_consumed_size > page_size) {
				/* dump_valid_data(dev, valid_data); */
				if ((offset_in_page + data_len +
					data_num * sizeof(struct mt_ftl_data_header) + 4) > page_size) {
					last_data_len_src = page_size -
						data_num * sizeof(struct mt_ftl_data_header) - offset_in_page - 4;
					memcpy(last_data_buffer,
						(char *)param->gc_page_buffer + offset_in_page, last_data_len_src);
					ret = mt_ftl_leb_read(dev, valid_data->info.src_leb,
						param->tmp_page_buffer, valid_data->info.offset_src + page_size,
						page_size);
					if (ret)
						goto out;
					memcpy((char *)last_data_buffer + last_data_len_src,
						(char *)param->tmp_page_buffer, (data_len - last_data_len_src));
					check_num =
						PAGE_GET_DATA_NUM(param->tmp_page_buffer[(page_size >> 2) - 1]);
					if (check_num == LAST_DATA_TAG) {
						valid_data->info.page_src += 1;
						valid_data->info.offset_src += page_size;
					}
					/* offset_in_page = 0; */
				}
				*last_data_len = page_size - (valid_data->valid_data_num + 1) *
					sizeof(struct mt_ftl_data_header) - valid_data->valid_data_offset - 4;
				if (*last_data_len > 0) {
					if (last_data_len_src)
						memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
							last_data_buffer, *last_data_len);
					else
						memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
							(char *)param->gc_page_buffer + offset_in_page,
							*last_data_len);
					valid_header.offset_len =
						(valid_data->valid_data_offset << FDATA_SHIFT) | data_len;
					valid_header.sector = sector;
					valid_data->valid_header_offset -= sizeof(struct mt_ftl_data_header);
					mt_ftl_debug(dev, "valid_header_offset: %d", valid_data->valid_header_offset);
					memcpy(&valid_data->valid_buffer[valid_data->valid_header_offset],
						&valid_header, sizeof(struct mt_ftl_data_header));
					valid_data->valid_data_num++;
					memcpy(&valid_data->valid_buffer[page_size - 4],
						&valid_data->valid_data_num, 4);
					mt_ftl_updatePMT(dev, sector, valid_data->info.dst_leb,
						valid_data->info.page_dst * page_size,
						valid_data->valid_data_num - 1, data_len, replay, false);
				}
				mt_ftl_debug(dev, "leb:%doffset:%d", valid_data->info.dst_leb,
					valid_data->info.offset_dst);
				PAGE_SET_READ(*(u32 *)(valid_data->valid_buffer + page_size - 4));
				ret = mt_ftl_leb_write(dev, valid_data->info.dst_leb, valid_data->valid_buffer,
					valid_data->info.offset_dst, page_size);
				if (ret)
					goto out;
				if (*last_data_len <= 0) {
					BIT_UPDATE(param->u4BIT[valid_data->info.dst_leb],
						(*last_data_len + sizeof(struct mt_ftl_data_header)));
					*last_data_len = 0;
				}
				valid_data->valid_data_offset = 0;
				valid_data->valid_data_num = 0;
				memset(valid_data->valid_buffer, 0xFF, page_size * sizeof(char));
				valid_data->info.offset_dst += page_size;
				valid_data->info.page_dst += 1;
			}
			if (*last_data_len > 0) {
				valid_data->valid_data_offset = data_len - *last_data_len;
				if (last_data_len_src)
					memcpy(valid_data->valid_buffer,
						(char *)last_data_buffer + *last_data_len,
						valid_data->valid_data_offset);
				else
					memcpy(valid_data->valid_buffer, (char *)param->gc_page_buffer +
						offset_in_page + *last_data_len, valid_data->valid_data_offset);
				*last_data_len = 0;
			} else {
				mt_ftl_updatePMT(dev, sector, valid_data->info.dst_leb,
						 valid_data->info.page_dst * page_size,
						 valid_data->valid_data_num, data_len, replay, false);
				valid_header.offset_len = (valid_data->valid_data_offset << FDATA_SHIFT) | data_len;
				valid_header.sector = sector;
				total_consumed_size = offset_in_page + data_len +
					data_num  * sizeof(struct mt_ftl_data_header) + 4;
				if (total_consumed_size > page_size) {
					last_data_len_src = page_size - offset_in_page -
						(data_num * sizeof(struct mt_ftl_data_header) + 4);
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
						(char *)param->gc_page_buffer + offset_in_page, last_data_len_src);
					ret = mt_ftl_leb_read(dev, valid_data->info.src_leb,
						param->tmp_page_buffer,
						valid_data->info.offset_src + page_size, page_size);
					if (ret)
						goto out;
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset +
						last_data_len_src], (char *)param->tmp_page_buffer,
						data_len - last_data_len_src);
					check_num =
						PAGE_GET_DATA_NUM(param->tmp_page_buffer[(page_size >> 2) - 1]);
					if (check_num == LAST_DATA_TAG) {
						valid_data->info.page_src += 1;
						valid_data->info.offset_src += page_size;
					}
				} else
					memcpy(&valid_data->valid_buffer[valid_data->valid_data_offset],
						(char *)param->gc_page_buffer + offset_in_page,
						data_len);
				valid_data->valid_data_offset += data_len;
				valid_data->valid_header_offset = page_size -
					(valid_data->valid_data_num + 1) * sizeof(struct mt_ftl_data_header) - 4;
				memcpy(&valid_data->valid_buffer[valid_data->valid_header_offset],
						&valid_header, sizeof(struct mt_ftl_data_header));
				valid_data->valid_data_num += 1;
				memcpy(&valid_data->valid_buffer[page_size - 4], &valid_data->valid_data_num, 4);
			}
		}
	}
	if (replay) {
		data_len = FDATA_LEN(header_buffer[0].offset_len);
		offset_in_page = FDATA_OFFSET(header_buffer[0].offset_len);
		total_consumed_size = data_len + offset_in_page + data_num * sizeof(struct mt_ftl_data_header) + 4;
		if (total_consumed_size > page_size)
			*last_data_len = total_consumed_size - page_size;
		else if (total_consumed_size < page_size)
			BIT_UPDATE(param->u4BIT[valid_data->info.dst_leb], (page_size - total_consumed_size));
	}
out:
	return ret;
}

/* The process for data is to get all the sectors in the source block
 * and compare to PMT, if the corresponding block/page/part in PMT is
 * the same as source block/page/part, then current page should be copied
 * destination block, and then update PMT
 */
static int mt_ftl_gc_data(struct mt_ftl_blk *dev, int sleb, int dleb, int *offset_dst, bool replay)
{
	int ret = MT_FTL_SUCCESS;
	u32 data_num = 0, retry = 0;
	u32 data_hdr_offset = 0;
	u32 page_been_read = 0;
	struct mt_ftl_data_header *header_buffer = NULL;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_valid_data valid_data;
	int head_data = 0, last_data_len, src_leb;
	u32 total_consumed_size = 0;
	int page_size = dev->min_io_size;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;
	const int max_valid_data_size = dev->leb_size - param->u4BIT[sleb] - (dev->leb_size / dev->min_io_size * 4);

	valid_data.info.offset_src = 0;
	valid_data.info.offset_dst = 0;
	valid_data.info.src_leb = sleb;
	valid_data.info.dst_leb = dleb;
	valid_data.info.page_src = 0;
	valid_data.info.page_dst = 0;
	valid_data.valid_data_num = 0;
	valid_data.valid_data_offset = 0;
	valid_data.valid_data_size = 0;
	valid_data.valid_header_offset = 0;
	valid_data.valid_buffer = (char *)param->general_page_buffer;

	memset(valid_data.valid_buffer, 0xFF, page_size);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READOOB);
	if (!replay)
		mt_ftl_leb_remap(dev, dleb);
	if (!replay)
		src_leb = sleb;
	else
		src_leb = dleb;
	ret = mt_ftl_leb_read(dev, src_leb, param->gc_page_buffer, valid_data.info.offset_src, page_size);
	if (ret)
		goto out_free;

	data_num = PAGE_GET_DATA_NUM(param->gc_page_buffer[(page_size >> 2) - 1]);
	if ((data_num == PAGE_READ_TAG) || (data_num == LAST_DATA_TAG)) {
		mt_ftl_debug(dev, "End GC Data, offset_src=%d", valid_data.info.offset_src);
		goto out_free;
	}
	if (replay) {
		page_been_read = PAGE_BEEN_READ(param->gc_page_buffer[(page_size >> 2) - 1]);
		if (page_been_read == 0) {
			mt_ftl_debug(dev, "End of replay GC Data, offset_src = %d", valid_data.info.offset_src);
			goto out_free;
		}
	}
	data_hdr_offset = page_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
	header_buffer = (struct mt_ftl_data_header *)(&param->gc_page_buffer[data_hdr_offset >> 2]);

	head_data = mt_ftl_check_header(dev, header_buffer, data_num);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READOOB);

	while (head_data && (valid_data.info.offset_src <= max_offset_per_block)) {
		retry = 1;
		last_data_len = 0;
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		ret = mt_ftl_gc_check_data(dev, header_buffer, &valid_data, data_num, &last_data_len, replay);
		if (ret)
			goto out_free;
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READOOB);
retry_once:
		if (replay) {
			valid_data.info.offset_dst += page_size;
			valid_data.info.page_dst += 1;
		}

		mt_ftl_debug(dev, "%d %d %d %d", valid_data.info.offset_src, valid_data.info.page_src,
			valid_data.valid_data_size, max_valid_data_size);
		valid_data.info.offset_src += page_size;
		valid_data.info.page_src += 1;
		if (valid_data.valid_data_size >= max_valid_data_size)
			break;
		if (valid_data.info.offset_src > max_offset_per_block)
			break;
		ret = mt_ftl_leb_read(dev, src_leb, param->gc_page_buffer, valid_data.info.offset_src, page_size);
		if (ret)
			goto out_free;
		data_num = PAGE_GET_DATA_NUM(param->gc_page_buffer[(page_size >> 2) - 1]);
		if (data_num == LAST_DATA_TAG) {
			if (replay && last_data_len && retry) {
				BIT_UPDATE(param->u4BIT[dleb], (page_size - last_data_len - 4));
				last_data_len = 0;
				retry = 0;
				mt_ftl_debug(dev, "replay: data_num = 0x%x", data_num);
				goto retry_once;
			} else if (retry) {
				mt_ftl_debug(dev, "gc_data: data_num = 0x%x", data_num);
				retry = 0;
				goto retry_once;
			}
			mt_ftl_debug(dev, "break: data_num = 0x%x", data_num);
			break;
		}
		if (data_num == PAGE_READ_TAG) {
			mt_ftl_debug(dev, "break: data_num = 0x%x", data_num);
			break;
		}
		data_hdr_offset = page_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
		header_buffer = (struct mt_ftl_data_header *)(&param->gc_page_buffer[data_hdr_offset >> 2]);

		head_data = mt_ftl_check_header(dev, header_buffer, data_num);
		if (replay) {
			page_been_read = PAGE_BEEN_READ(param->gc_page_buffer[(page_size >> 2) - 1]);
			if (page_been_read == 0) {
				mt_ftl_debug(dev, "End of replay GC Data, offset_src = %d", valid_data.info.offset_src);
				goto out_free;
			}
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READOOB);
	}
	if ((valid_data.valid_data_num || valid_data.valid_data_offset) && (!replay)) {
		if (valid_data.valid_data_num) {
			total_consumed_size = valid_data.valid_data_offset +
				valid_data.valid_data_num * sizeof(struct mt_ftl_data_header) + 4;
			PAGE_SET_READ(*(u32 *)(valid_data.valid_buffer + page_size - 4));
		} else {
			total_consumed_size = valid_data.valid_data_offset + 4;
			PAGE_SET_LAST_DATA(*(u32 *)(valid_data.valid_buffer + page_size - 4));
		}
		ret = mt_ftl_leb_write(dev, dleb, valid_data.valid_buffer, valid_data.info.offset_dst, page_size);
		if (ret)
			goto out_free;
		BIT_UPDATE(param->u4BIT[dleb], (page_size - total_consumed_size));
		valid_data.info.offset_dst += page_size;
		/* param->u4NextPageOffsetIndicator = 0; */
		/* param->u4DataNum = 0; */
		mt_ftl_debug(dev, "gc finished, valid_data_num=%d", valid_data.valid_data_num);
	}
out_free:
	*offset_dst = valid_data.info.offset_dst;
	mt_ftl_debug(dev, "u4BIT[%d] = %d", dleb, param->u4BIT[dleb]);

	return ret;
}

static int mt_ftl_write_replay_blk(struct mt_ftl_blk *dev, int leb, int *page, bool replay)
{
	int ret = MT_FTL_SUCCESS, i;
	int replay_blk, replay_offset;
	struct mt_ftl_param *param = dev->param;
	const int max_replay_offset = dev->max_replay_blks * dev->min_io_size;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	if (!replay) {
		if (commit_node->u4NextReplayOffsetIndicator == 0) {
			for (i = 0; i < REPLAY_BLOCK_NUM; i++)
				mt_ftl_leb_remap(dev, REPLAY_BLOCK + i);
		}
		replay_blk = commit_node->u4NextReplayOffsetIndicator / dev->leb_size + REPLAY_BLOCK;
		replay_offset = commit_node->u4NextReplayOffsetIndicator % dev->leb_size;
		memset(param->replay_page_buffer, 0xFF, dev->min_io_size);
		param->replay_page_buffer[0] = MT_MAGIC_REPLAY;
		param->replay_page_buffer[1] = leb;
		mt_ftl_err(dev, "u4NextReplayOffsetIndicator=%d(%d) %d",
			commit_node->u4NextReplayOffsetIndicator, leb, param->replay_blk_index);
		ret = mt_ftl_leb_write(dev, replay_blk, param->replay_page_buffer,
			replay_offset, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		commit_node->u4NextReplayOffsetIndicator += dev->min_io_size;
		param->replay_blk_rec[param->replay_blk_index] = leb;
		param->replay_blk_index++;
		if (commit_node->u4NextReplayOffsetIndicator == max_replay_offset) {
			commit_node->u4NextReplayOffsetIndicator = 0;
			PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, *page);
			mt_ftl_commit(dev);
			*page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
		}
	} else {
		commit_node->u4NextReplayOffsetIndicator += dev->min_io_size;
		param->replay_blk_rec[param->replay_blk_index] = leb;
		param->replay_blk_index++;
		if (commit_node->u4NextReplayOffsetIndicator == max_replay_offset) {
			commit_node->u4NextReplayOffsetIndicator = 0;
			memset(param->replay_blk_rec, 0xFF, dev->max_replay_blks * sizeof(unsigned int));
			param->replay_blk_index = 0;
		}
	}
	return ret;
}

static int mt_ftl_gc(struct mt_ftl_blk *dev, int *updated_page, bool ispmt, bool replay, int *data_commit)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int src_leb = MT_INVALID_BLOCKPAGE;
	int return_leb = 0, offset_dst = 0;
	bool invalid = false;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	int start_leb = dev->data_start_blk, end_leb = commit_node->dev_blocks;
	struct mt_ftl_param *param = dev->param;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	int total_invalid = dev->leb_size - (dev->leb_size / dev->min_io_size * 4);

	trace_mt_ftl_gc(dev, ispmt, replay);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_FINDBLK);
	/* Get the most invalid block */
	if (ispmt) {
		start_leb = PMT_START_BLOCK;
		end_leb = dev->data_start_blk;
	}
	src_leb = mt_ftl_gc_find_block(dev, start_leb, end_leb, ispmt, &invalid);
	if (!replay) {
		ret = mt_ftl_check_replay_commit(dev, src_leb, ispmt, true);
		if (ret) {
			mt_ftl_err(dev, "commit src_leb(%d) == replay_blk_rec", src_leb);
			if (ispmt)
				param->get_pmt_blk_flag = 1;
			else {
				if (param->pmt_updated_flag == PMT_FREE_BLK)
					*data_commit = PMT_COMMIT;
				else
					mt_ftl_commit(dev);
			}
		}
	}
	if (invalid) {
		if (replay)
			goto gc_end;
		/* check the bit info for unmap */
		if (!ispmt) {
			for (start_leb = src_leb; start_leb < end_leb; start_leb++) {
				if (param->u4BIT[start_leb] < total_invalid)
					continue;
				if (!ubi_is_mapped(dev->desc, start_leb))
					continue;
				ret = mt_ftl_check_replay_commit(dev, start_leb, ispmt, false);
				if (!ret) {
					if (param->u4BIT[start_leb] > total_invalid)
						mt_ftl_err(dev, "param->u4BIT[%d]=%d", start_leb,
							param->u4BIT[start_leb]);
					mt_ftl_err(dev, "ubi unmap %d,peb=%d",
						start_leb, dev->desc->vol->eba_tbl[start_leb]);
					ubi_leb_unmap(dev->desc, start_leb);
				}
			}
			mt_ftl_leb_remap(dev, commit_node->u4GCReserveLeb);
		} else
			mt_ftl_leb_remap(dev, commit_node->u4GCReservePMTLeb);
		goto gc_end;
	}

	if (((u32)src_leb) == MT_INVALID_BLOCKPAGE) {
		mt_ftl_err(dev, "cannot find block for GC, isPMT = %d", ispmt);
		trace_mt_ftl_gc_error(dev, -1);
		return MT_FTL_FAIL;
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_FINDBLK);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_REMAP);

	/* Call sub function for pmt/data */
	if (ispmt)
		ret = mt_ftl_gc_pmt(dev, src_leb, commit_node->u4GCReservePMTLeb, &offset_dst, replay);
	else {
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
		param->gc_dat_cmit_flag = 1;
		ret = mt_ftl_gc_data(dev, src_leb, commit_node->u4GCReserveLeb, &offset_dst, replay);
		param->gc_dat_cmit_flag = 0;
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_DATA_READ_UPDATE_PMT);
	}
	/* PMT not in PMTBlock but dirty, ret == 1 is u4SrcInvalidLeb mapped*/
	if (ispmt && (ret == 1))
		goto gc_end;
	else if (ret) {
		mt_ftl_err(dev, "GC sub function failed, u4SrcInvalidLeb=%d, offset_dst=%d, ret=%d",
			   src_leb, offset_dst, ret);
		trace_mt_ftl_gc_error(dev, -2);
		return MT_FTL_FAIL;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_CPVALID);
gc_end:
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GC_REMAP);

	/* TODO: Use this information for replay, instead of check MT_PAGE_HAD_BEEN_READ */
	*updated_page = offset_dst / dev->min_io_size;
	if (*updated_page == page_num_per_block) {
		mt_ftl_err(dev, "There is no more free pages in the gathered block");
		mt_ftl_err(dev, "desc->vol->ubi->volumes[%d]->reserved_pebs = %d",
			   dev->vol_id, commit_node->dev_blocks);
		for (i = PMT_START_BLOCK; i < commit_node->dev_blocks; i += 8) {
			mt_ftl_err(dev, "%d\t %d\t %d\t %d\t %d\t %d\t %d\t %d", param->u4BIT[i],
				   param->u4BIT[i + 1], param->u4BIT[i + 2], param->u4BIT[i + 3],
				   param->u4BIT[i + 4], param->u4BIT[i + 5], param->u4BIT[i + 6],
				   param->u4BIT[i + 7]);
		}
		trace_mt_ftl_gc_error(dev, -3);
		return MT_FTL_FAIL;
	}

	if (ispmt) {
		return_leb = commit_node->u4GCReservePMTLeb;
		commit_node->u4GCReservePMTLeb = src_leb;
		mt_ftl_debug(dev, "u4GCReservePMTLeb = %d, *updated_page = %d, u4GCReserveLeb = %d",
				commit_node->u4GCReservePMTLeb, *updated_page, commit_node->u4GCReserveLeb);
		param->u4BIT[commit_node->u4GCReservePMTLeb] = 0;
		mt_ftl_debug(dev, "u4BIT[%d] = %d",
			commit_node->u4GCReservePMTLeb, param->u4BIT[commit_node->u4GCReservePMTLeb]);
	} else {
		return_leb = commit_node->u4GCReserveLeb;
		commit_node->u4GCReserveLeb = src_leb;
		mt_ftl_debug(dev, "u4GCReserveLeb=%d, *updated_page=%d", commit_node->u4GCReserveLeb, *updated_page);
		param->u4BIT[commit_node->u4GCReserveLeb] = 0;
		mt_ftl_debug(dev, "u4BIT[%d]=%d",
			commit_node->u4GCReserveLeb, param->u4BIT[commit_node->u4GCReserveLeb]);
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GC_REMAP);
	trace_mt_ftl_gc_done(dev, return_leb, *updated_page, src_leb);
	return return_leb;
}



/* TODO: Add new block to replay block, but have to consider the impact of original valid page to replay */
static int mt_ftl_get_free_block(struct mt_ftl_blk *dev, int *updated_page, bool ispmt, bool replay)
{
	int ret = MT_FTL_SUCCESS;
	int free_leb = 0;
	int data_commit = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	if (ispmt) {
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_PMT_GETLEB);
		if (commit_node->u4NextFreePMTLebIndicator != MT_INVALID_BLOCKPAGE) {
			free_leb = commit_node->u4NextFreePMTLebIndicator;
			/* if (!replay)
			 * mt_ftl_leb_map(dev, free_leb);
			 */
			*updated_page = 0;
			commit_node->u4NextFreePMTLebIndicator++;
			mt_ftl_debug(dev, "u4NextFreePMTLebIndicator=%d", commit_node->u4NextFreePMTLebIndicator);
			/* The PMT_BLOCK_NUM + 2 block is reserved to param->u4GCReserveLeb */
			if (commit_node->u4NextFreePMTLebIndicator >= dev->data_start_blk - 2) {
				/* mt_ftl_err(dev,  "[INFO] u4NextFreePMTLebIndicator is in the end"); */
				commit_node->u4NextFreePMTLebIndicator = MT_INVALID_BLOCKPAGE;
				mt_ftl_debug(dev, "u4NextFreePMTLebIndicator=%d",
					commit_node->u4NextFreePMTLebIndicator);
			}
		} else
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay, &data_commit);
		if (!replay) {
			param->replay_pmt_blk_rec[param->replay_pmt_blk_index] = SET_FREE_PMT_BLK(free_leb);
			param->replay_pmt_blk_index++;
		}
		if (replay && param->replay_pmt_last_cmit)
			param->record_last_pmt_blk = free_leb;
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
		return free_leb;
	}
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
	if (commit_node->u4NextFreeLebIndicator != MT_INVALID_BLOCKPAGE) {
		free_leb = commit_node->u4NextFreeLebIndicator;
		*updated_page = 0;
		/* if (!replay)
		 * mt_ftl_leb_map(dev, free_leb);
		 */
		commit_node->u4NextFreeLebIndicator++;
		/* param->u4NextPageOffsetIndicator = 0; */
		mt_ftl_debug(dev, "u4NextFreeLebIndicator=%d", commit_node->u4NextFreeLebIndicator);
		/* The last block is reserved to param->u4GCReserveLeb */
		/* [Bean] Please reserved block for UBI WL 2 PEB*/
		if (commit_node->u4NextFreeLebIndicator >= commit_node->dev_blocks - 1) {
			commit_node->u4NextFreeLebIndicator = MT_INVALID_BLOCKPAGE;
			mt_ftl_debug(dev, "u4NextFreeLebIndicator=%d", commit_node->u4NextFreeLebIndicator);
		}
	} else {
		if (param->pmt_updated_flag == PMT_FREE_BLK) {
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay, &data_commit);
			if (data_commit) {
				param->pmt_updated_flag = data_commit;
				mt_ftl_err(dev, "pmt_updated_flag = %d", param->pmt_updated_flag);
			}
		} else
			free_leb = mt_ftl_gc(dev, updated_page, ispmt, replay, &data_commit);
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_GETLEB);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT);

	/* if (!replay && (param->pmt_updated_flag == PMT_FREE_BLK)) {
	 * param->pmt_updated_flag = PMT_COMMIT;
	 * mt_ftl_debug(dev, "pmt_updated_flag = PMT_COMMIT");
	 * }
	 */
	ret = mt_ftl_write_replay_blk(dev, free_leb, updated_page, replay);
	if (ret)
		return MT_FTL_FAIL;

	trace_mt_ftl_get_free_block(dev, free_leb, *updated_page);
	mt_ftl_err(dev, "u4FreeLeb = %d, u4NextFreeLebIndicator = %d, isPMT = %d, updated_page = %d",
		   free_leb, commit_node->u4NextFreeLebIndicator, ispmt, *updated_page);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_GETFREEBLOCK_PUTREPLAY_COMMIT);
	return free_leb;
}



int mt_ftl_write_page(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i;
	int leb = 0, page = 0, cache_num = 0;
	u32 cluster = 0, sec_offset = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	u32 data_hdr_offset = 0;
	int data_offset = 0, data_len = 0;
	int header_buf_offset, data_buf_offset, num;
	struct mt_ftl_data_header *header_buffer;

	trace_mt_ftl_write_page(dev);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB);
	/* check the first cache has no data */
	if (!param->u4DataNum[0] && !param->data_cache_num && !param->u4NextPageOffsetIndicator) {
		mt_ftl_debug(dev, "%d", param->data_cache_num);
		return MT_FTL_SUCCESS;
	}
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
	mt_ftl_debug(dev, "u4NextLebPageIndicator = 0x%x, leb = %d, page = %d",
		commit_node->u4NextLebPageIndicator, leb, page);
	/* write every cache here */
	for (num = 0; num <= param->data_cache_num; num++) {
		data_buf_offset = num * dev->min_io_size;
		/* last data in cache*/
		if (param->u4DataNum[num] == 0 && param->u4NextPageOffsetIndicator) {
			data_buf_offset += (dev->min_io_size - 4);
			PAGE_SET_LAST_DATA(*(u32 *)(param->u1DataCache + data_buf_offset));
			BIT_UPDATE(param->u4BIT[leb], (dev->min_io_size - param->u4NextPageOffsetIndicator - 4));
		} else {
			data_hdr_offset = dev->min_io_size -
				(param->u4DataNum[num] * sizeof(struct mt_ftl_data_header) + 4);
			header_buf_offset = data_buf_offset + data_hdr_offset;
			header_buffer = (struct mt_ftl_data_header *)&param->u1DataCache[header_buf_offset];
			data_offset = FDATA_OFFSET(header_buffer->offset_len);
			data_len = FDATA_LEN(header_buffer->offset_len);
			if ((data_offset + data_len) < data_hdr_offset)
				BIT_UPDATE(param->u4BIT[leb], (data_hdr_offset - data_offset - data_len));
			data_buf_offset += (dev->min_io_size - 4);
			memcpy(&param->u1DataCache[data_buf_offset], &param->u4DataNum[num], 4);
			for (i = param->u4DataNum[num] - 1; i >= 0; i--) {
				cluster = (u32)(FS_PAGE_OF_SECTOR(header_buffer[i].sector) >> dev->pm_per_io);
				sec_offset =
					(u32)(FS_PAGE_OF_SECTOR(header_buffer[i].sector) & ((1 << dev->pm_per_io) - 1));
				ubi_assert(cluster < commit_node->dev_clusters);
				if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
					cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
					ubi_assert(cache_num < PMT_CACHE_NUM);
					PMT_RESET_DATA_INCACHE(param->u4MetaPMTCache[(cache_num << dev->pm_per_io)
						+ sec_offset]);
				}
			}
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_WRITEOOB);
	}
	MT_FTL_WBUF_NOSYNC(dev);
	ret = mt_ftl_leb_write(dev, leb, param->u1DataCache, page * dev->min_io_size, num * dev->min_io_size);
	if (ret)
		return ret;
	page += num;
	memset(param->u4DataNum, 0, MAX_DATA_CACHE_NUM * sizeof(unsigned int));
	param->u4NextPageOffsetIndicator = 0;
	param->data_cache_num = 0;
	if (page == page_num_per_block) {
		if (param->pmt_updated_flag == PMT_START) {
			param->pmt_updated_flag = PMT_FREE_BLK;
			mt_ftl_debug(dev, "pmt_updated_flag = PMT_FREE_BLK");
		}
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK);
		leb = mt_ftl_get_free_block(dev, &page, false, false);
		if (((u32)leb) == MT_INVALID_BLOCKPAGE) {
			mt_ftl_err(dev, "mt_ftl_getfreeblock failed");
			return MT_FTL_FAIL;
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_GETFREEBLK);
	}

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_PAGE_RESET);
	memset(param->u1DataCache, 0xFF, MAX_DATA_CACHE_NUM * dev->min_io_size * sizeof(unsigned char));

	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, page);
	/*mt_ftl_err(dev,  "u4NextLebPageIndicator = 0x%x", param->u4NextLebPageIndicator);*/

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_PAGE_RESET);

	return ret;
}

int mt_ftl_commitPMT(struct mt_ftl_blk *dev, bool replay, bool commit, int *touch)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int pmt_block = 0, pmt_page = 0;
	struct mt_ftl_param *param = dev->param;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	trace_mt_ftl_commitPMT(dev, commit_node->u4CurrentPMTLebPageIndicator, replay, commit);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_COMMIT_PMT);
	if ((!replay && commit) || param->u4NextPageOffsetIndicator) {
		mt_ftl_debug(dev, "commit PMT write page, u4NextPageOffsetIndicator=%d,data_num=%d",
			param->u4NextPageOffsetIndicator, param->u4DataNum[0]);
		ret = mt_ftl_write_page(dev);
	}

	/* do the toucher commit */
	if (*touch != -1) {
		i = dev->cache_tail->cache_num;
		goto touch_commit;
	}

	for (i = 0; i < PMT_CACHE_NUM; i++) {
		if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
			continue;
		ubi_assert(param->i4CurrentPMTClusterInCache[i] < commit_node->dev_clusters);
		if (!PMT_INDICATOR_IS_INCACHE
		    (param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
			mt_ftl_err(dev, "i4CurrentPMTClusterInCache(%d) is not in cache",
				   param->i4CurrentPMTClusterInCache[i]);
			return MT_FTL_FAIL;
		}
		if (!PMT_INDICATOR_IS_DIRTY
		    (param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
			mt_ftl_err(dev, "u4PMTIndicator[%d]=0x%x is not dirty",
				param->i4CurrentPMTClusterInCache[i],
				param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			/* clear i4CurrentPMTClusterInCache */
			PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator
				[param->i4CurrentPMTClusterInCache[i]]);
			param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
			continue;
		}
touch_commit:
		/* Update param->u4BIT of the block that is originally in param->u4PMTCache */
		pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		if ((((u32)pmt_block) != MT_INVALID_BLOCKPAGE) && (pmt_block != 0)) {
			BIT_UPDATE(param->u4BIT[pmt_block], (dev->min_io_size * 2));
			mt_ftl_debug(dev, "u4BIT[%d] = %d", pmt_block, param->u4BIT[pmt_block]);
		}

		/* Calculate new block/page from param->u4CurrentPMTLebPageIndicator
		 * Update param->u4CurrentPMTLebPageIndicator
		 * and check the correctness of param->u4CurrentPMTLebPageIndicator
		 * and if param->u4CurrentPMTLebPageIndicator is full
		 * need to call get free block/page function
		 * Write old param->u4PMTCache back to new block/page
		 * Update param->u4PMTIndicator of the block/page that is originally in param->u4PMTCache
		 */
		pmt_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4CurrentPMTLebPageIndicator);
		pmt_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4CurrentPMTLebPageIndicator);
		/* mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator = 0x%x", param->u4CurrentPMTLebPageIndicator); */

		/* replay the pmt block for power loss */
		if (replay && param->replay_pmt_last_cmit && (param->record_last_pmt_blk == -1)) {
			mt_ftl_err(dev, "recover last pmt leb %d %d", pmt_block, pmt_page);
			ret = mt_ftl_leb_recover(dev, pmt_block, pmt_page * dev->min_io_size, 0);
			if (ret == 1)
				param->record_last_pmt_blk = pmt_block;
			else if (ret == MT_FTL_FAIL)
				return ret;
		}
		if (!replay || param->record_last_pmt_blk == pmt_block) {
			mt_ftl_debug(dev, "write pmt %d %d %d %d",
				pmt_block, pmt_page, i, param->i4CurrentPMTClusterInCache[i]);
			ret = mt_ftl_leb_write(dev, pmt_block, &param->u4PMTCache[i << dev->pm_per_io],
					 pmt_page * dev->min_io_size, dev->min_io_size);
			ret = mt_ftl_leb_write(dev, pmt_block, &param->u4MetaPMTCache[i << dev->pm_per_io],
					 (pmt_page + 1) * dev->min_io_size, dev->min_io_size);
		}
		PMT_INDICATOR_SET_BLOCKPAGE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]],
					pmt_block, pmt_page, 0, i);
		/* clear i4CurrentPMTClusterInCache */
		if (param->i4CurrentReadPMTClusterInCache == param->i4CurrentPMTClusterInCache[i]) {
			mt_ftl_debug(dev, "[Bean]clear read PMTCluster cache");
			param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
		}
		PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", param->i4CurrentPMTClusterInCache[i],
			   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
		pmt_page += 2;
		if (pmt_page >= page_num_per_block) {
			pmt_block = mt_ftl_get_free_block(dev, &pmt_page, true, replay);
			if (((u32)pmt_block) == MT_INVALID_BLOCKPAGE) {
				mt_ftl_err(dev, "mt_ftl_getfreeblock failed");
				return MT_FTL_FAIL;
			}
		}
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4CurrentPMTLebPageIndicator, pmt_block, pmt_page);
		/* out the touch commit */
		if (*touch != -1) {
			*touch = i;
			break;
		}
	}
	mt_ftl_debug(dev, "commit pmt end 0x%x %d", commit_node->u4CurrentPMTLebPageIndicator, *touch);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_COMMIT_PMT);
	return ret;
}

int mt_ftl_commit_indicators(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, index, size, dst_size;
	unsigned int *buffer;
	struct mt_ftl_cache_node *pnode = NULL;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	/* write commit node */
	memset(param->commit_page_buffer, 0, dev->min_io_size);
	memcpy(param->commit_page_buffer, commit_node, sizeof(struct mt_ftl_commit_node));

	index = sizeof(struct mt_ftl_commit_node) / sizeof(unsigned int);
	memcpy(&param->commit_page_buffer[index], param->u4PMTIndicator,
		commit_node->dev_clusters * sizeof(unsigned int));
	index += commit_node->dev_clusters;
	if (dev->commit_page_num > 1) {
		size = dev->min_io_size - index * sizeof(unsigned int);
		memcpy(&param->commit_page_buffer[index], param->u4BIT, size);
		memset(param->tmp_page_buffer, 0, dev->min_io_size);
		param->tmp_page_buffer[0] = MT_MAGIC_NUMBER;
		dst_size = commit_node->dev_blocks * sizeof(unsigned int) - size;
		memcpy(&param->tmp_page_buffer[1], &param->u4BIT[size >> 2], dst_size);
		index = ((dst_size >> 2) + 1);
		buffer = param->tmp_page_buffer;
	} else {
		memcpy(&param->commit_page_buffer[index], param->u4BIT, commit_node->dev_blocks * sizeof(unsigned int));
		index += commit_node->dev_blocks;
		buffer = param->commit_page_buffer;
	}
	if (dev->cache_len == PMT_CACHE_NUM) {
		buffer[index++] = MT_MAGIC_CACHE_NODE;
		for (pnode = dev->cache_head; pnode; pnode = pnode->next)
			buffer[index++] = pnode->cache_num;
	}
	/*mt_ftl_show_commit_node(dev);*/

	param->i4CommitOffsetIndicator0 += dev->commit_page_num * dev->min_io_size;
	param->i4CommitOffsetIndicator1 += dev->commit_page_num * dev->min_io_size;
	for (index = 0; index < dev->commit_page_num; index++) {
		if (index > 0)
			buffer = param->tmp_page_buffer;
		else
			buffer = param->commit_page_buffer;
		size = index * dev->min_io_size;
		/* write config block*/
		if (param->i4CommitOffsetIndicator0 > max_offset_per_block) {
			ubi_assert(index == 0);
			ubi_leb_unmap(dev->desc, CONFIG_START_BLOCK);
			param->i4CommitOffsetIndicator0 = 0;
		}
		ret = mt_ftl_leb_write(dev, CONFIG_START_BLOCK, buffer,
			param->i4CommitOffsetIndicator0 + size, dev->min_io_size);

		/* write config backup block*/
		if (param->i4CommitOffsetIndicator1 > max_offset_per_block) {
			ubi_assert(index == 0);
			ubi_leb_unmap(dev->desc, CONFIG_START_BLOCK + 1);
			param->i4CommitOffsetIndicator1 = 0;
		}
		ret = mt_ftl_leb_write(dev, CONFIG_START_BLOCK + 1, buffer,
			param->i4CommitOffsetIndicator1 + size, dev->min_io_size);
	}

	memset(param->replay_blk_rec, 0xFF, dev->max_replay_blks * sizeof(unsigned int));
	param->replay_blk_index = 0;
	memset(param->replay_pmt_blk_rec, 0xFF, dev->max_pmt_replay_blk * sizeof(unsigned int));
	param->replay_pmt_blk_index = 0;
	param->replay_pmt_blk_rec[param->replay_pmt_blk_index]
		= PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4CurrentPMTLebPageIndicator);
	param->replay_pmt_blk_index++;
	param->replay_blk_rec[param->replay_blk_index] =
		PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
	param->replay_blk_index++;

	return ret;
}

int mt_ftl_commit(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int touch = -1; /* do not touch commit */
	struct mt_ftl_param *param = dev->param;

	trace_mt_ftl_commit(dev);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_COMMIT);
	mt_ftl_commitPMT(dev, false, true, &touch);

	for (i = 0; i < PMT_CACHE_NUM; i++) {
		if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
			continue;
		PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		mt_ftl_debug(dev, "u4PMTIndicator[%d] = %d", param->i4CurrentPMTClusterInCache[i],
			   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
		param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
	}
	/*mt_ftl_err(dev, "commit_page_num=%d", dev->commit_page_num);*/
	ret = mt_ftl_commit_indicators(dev);
	trace_mt_ftl_commit_done(dev, ret);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_COMMIT);
	return ret;
}

static int mt_ftl_downloadPMT(struct mt_ftl_blk *dev, u32 cluster, int *cache_num, bool replay)
{
	int ret = MT_FTL_SUCCESS;
	int pmt_block = 0, pmt_page = 0;
	int i = 0, rec_leb = -1;

	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;

	/* consider cluser if in cache */
	if (PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) {
		mt_ftl_err(dev, "After commit PMT: cluster(%d) is in cache", cluster);
		*cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
		return ret;
	}
	pmt_block = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
	pmt_page = PMT_INDICATOR_GET_PAGE(param->u4PMTIndicator[cluster]);
	if (!replay) {
		for (i = 0; i < param->replay_pmt_blk_index; i++) {
			rec_leb = GET_PMT_REC_BLK(param->replay_pmt_blk_rec[i]);
			if (pmt_block == rec_leb) {
				mt_ftl_debug(dev, "pmt_block[%d] == rec_leb[%d]:[%d]", pmt_block, i, rec_leb);
				break;
			}
		}
	}
	if ((!replay) && (i == param->replay_pmt_blk_index)) {
		param->replay_pmt_blk_rec[param->replay_pmt_blk_index] = SET_DL_PMT_BLK(pmt_block);
		param->replay_pmt_blk_index++;
	}
	/* mt_ftl_err(dev, "pmt[leb %d --> %d peb], pmt_page=%d", pmt_block,
	 * dev->desc->vol->eba_tbl[pmt_block], pmt_page);
	 */
	ubi_assert(*cache_num < PMT_CACHE_NUM);

	if (unlikely(pmt_block == 0) || unlikely(ubi_is_mapped(desc, pmt_block) == 0)) {
		mt_ftl_debug(dev, "unlikely pmt_block %d", ubi_is_mapped(desc, pmt_block));
		memset(&param->u4PMTCache[*cache_num << dev->pm_per_io], 0xFF,
			sizeof(unsigned int) << dev->pm_per_io);
		memset(&param->u4MetaPMTCache[*cache_num << dev->pm_per_io], 0xFF,
			sizeof(unsigned int) << dev->pm_per_io);
	} else {
		ret = mt_ftl_leb_read(dev, pmt_block, &param->u4PMTCache[*cache_num << dev->pm_per_io],
				pmt_page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		ret = mt_ftl_leb_read(dev, pmt_block, &param->u4MetaPMTCache[*cache_num << dev->pm_per_io],
			(pmt_page + 1) * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
	}
	param->i4CurrentPMTClusterInCache[*cache_num] = cluster;
	mt_ftl_debug(dev, "i4CurrentPMTClusterInCache[%d]=%d",
		*cache_num, param->i4CurrentPMTClusterInCache[*cache_num]);
	PMT_INDICATOR_SET_CACHE_BUF_NUM(param->u4PMTIndicator[cluster], *cache_num);
	mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);
	return ret;
}

int mt_ftl_cache_node_init(struct mt_ftl_blk *dev, u32 *buffer)
{
	int i;
	struct mt_ftl_cache_node *pnode = NULL;
	u32 bufsize;

	dev->cache_len = 0;
	dev->cache_head = dev->cache_tail = NULL;
	dev->cache_slab = NULL;
	dev->cache_table = NULL;

	dev->cache_slab = kmem_cache_create("cache_node_slab",
					sizeof(struct mt_ftl_cache_node),
					0,
					SLAB_HWCACHE_ALIGN | SLAB_PANIC,
					NULL);
	if (!dev->cache_slab)
		return -ENOMEM;
	bufsize = PMT_CACHE_NUM * sizeof(struct mt_ftl_cache_node *);
	dev->cache_table = kzalloc(bufsize, GFP_KERNEL);
	if (!dev->cache_table) {
		kmem_cache_destroy(dev->cache_slab);
		return -ENOMEM;
	}
	if (buffer && buffer[0] == MT_MAGIC_CACHE_NODE) {
		for (i = 0; i < PMT_CACHE_NUM; i++) {
			mt_ftl_debug(dev, "init here %u", buffer[i+1]);
			if (buffer[i+1] >= PMT_CACHE_NUM) {
				mt_ftl_err(dev, "cache node buffer error %d %u", i, buffer[i+1]);
				return MT_FTL_FAIL;
			}
			pnode = kmem_cache_alloc(dev->cache_slab, GFP_KERNEL);
			if (!pnode) {
				dump_stack();
				return -ENOMEM;
			}
			pnode->cache_num = buffer[i+1];
			pnode->prev = pnode->next = NULL;
			if (dev->cache_head == NULL)
				dev->cache_head = dev->cache_tail = pnode;
			else {
				dev->cache_tail->next = pnode;
				pnode->prev = dev->cache_tail;
				dev->cache_tail = pnode;
			}
			dev->cache_table[pnode->cache_num] = pnode;
			dev->cache_len++;
		}
	}
	return 0;
}

void mt_ftl_cache_node_free(struct mt_ftl_blk *dev)
{
	int i;

	if (!dev->cache_table)
		return;
	for (i = 0; i < PMT_CACHE_NUM; i++)
		kmem_cache_free(dev->cache_slab, dev->cache_table[i]);
	kmem_cache_destroy(dev->cache_slab);
	kfree(dev->cache_table);
}

static void push_cache_node_front(struct mt_ftl_blk *dev, struct mt_ftl_cache_node *node)
{
	if (dev->cache_len == 1)
		return;
	if (dev->cache_head == node)
		return;
	if (dev->cache_tail == node)
		dev->cache_tail = node->prev;
	/* doubly linked list */
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	node->next = dev->cache_head;
	node->prev = NULL;
	dev->cache_head->prev = node;
	dev->cache_head = node;
}

/* only support small cache table for touching, else using hash table instead of it  */
static int mt_ftl_touch_cache(struct mt_ftl_blk *dev, unsigned cache_num)
{
	struct mt_ftl_cache_node *pnode = NULL;

	if (dev->cache_len < PMT_CACHE_NUM && !dev->cache_table[cache_num]) {
		pnode = kmem_cache_alloc(dev->cache_slab, GFP_KERNEL);
		if (!pnode) {
			dump_stack();
			return -ENOMEM;
		}
		pnode->cache_num = cache_num;
		pnode->prev = pnode->next = NULL;
		if (dev->cache_head == NULL)
			dev->cache_head = dev->cache_tail = pnode;
		dev->cache_table[cache_num] = pnode;
		dev->cache_len++;
	} else
		pnode = dev->cache_table[cache_num];
	push_cache_node_front(dev, pnode);
	return 0;
}

int mt_ftl_updatePMT(struct mt_ftl_blk *dev, sector_t sector, int leb, int offset,
		     int part, u32 cmpr_data_size, bool replay, bool commit)
{
	int ret = MT_FTL_SUCCESS;
	int i = 0, touch = 0;
	u32 *pmt = NULL;
	u32 *meta_pmt = NULL;
	u32 cluster = 0, sec_offset = 0;
	int old_leb = 0, old_data_size = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));

	ubi_assert(cluster < commit_node->dev_clusters);
	if (!PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[cluster])) { /* cluster is not in cache */
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT);
		for (i = 0; i < PMT_CACHE_NUM; i++) {
			if (param->i4CurrentPMTClusterInCache[i] == 0xFFFFFFFF)
				break;
			if (!PMT_INDICATOR_IS_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]])) {
				/* Cluster download PMT CLUSTER cache, but i4CurrentPMTClusterInCache not to update */
				mt_ftl_err(dev, "i4CurrentPMTClusterInCache (%d) is not in cache",
					   param->i4CurrentPMTClusterInCache[i]);
				dump_stack();
				return MT_FTL_FAIL;
			}
			if (!PMT_INDICATOR_IS_DIRTY(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]))
				break;
		}
		if (i == PMT_CACHE_NUM) {
			mt_ftl_debug(dev, "All PMT cache are dirty, start commit PMT");
			/* Just for downloading corresponding PMT in cache */
			param->pmt_updated_flag = PMT_START;
			mt_ftl_commitPMT(dev, replay, commit, &touch);
			i = touch;
		}
		if (param->i4CurrentPMTClusterInCache[i] != 0xFFFFFFFF) {
			PMT_INDICATOR_RESET_INCACHE(param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			mt_ftl_debug(dev, "Reset (%d) u4PMTIndicator[%d]=0x%x", i, param->i4CurrentPMTClusterInCache[i],
				   param->u4PMTIndicator[param->i4CurrentPMTClusterInCache[i]]);
			param->i4CurrentPMTClusterInCache[i] = 0xFFFFFFFF;
		}
		MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_FINDCACHE_COMMITPMT);
		MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT);

		/* Download PMT from the block/page in param->u4PMTIndicator[cluster] */
		ret = mt_ftl_downloadPMT(dev, cluster, &i, replay);
		if (ret)
			return ret;

		MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_DOWNLOADPMT);
	} else
		i = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT);
	ubi_assert(i < PMT_CACHE_NUM);
	/* pmt touch counter */
	ret = mt_ftl_touch_cache(dev, i);
	if (ret) {
		mt_ftl_err(dev, "touch cache fail %d", i);
		return ret;
	}

	pmt = &param->u4PMTCache[(i << dev->pm_per_io) + sec_offset];
	meta_pmt = &param->u4MetaPMTCache[(i << dev->pm_per_io) + sec_offset];
	mt_ftl_debug(dev, "%lx %d %d %d", (unsigned long)sector, leb, offset / dev->min_io_size, part);
	/* Update param->u4BIT */
	if (*pmt != NAND_DEFAULT_VALUE) {
		old_leb = PMT_GET_BLOCK(*pmt);
		old_data_size = PMT_GET_DATASIZE(*meta_pmt);
		ubi_assert(old_leb < commit_node->dev_blocks);
		BIT_UPDATE(param->u4BIT[old_leb], (old_data_size + sizeof(struct mt_ftl_data_header)));
		if (param->u4BIT[old_leb] > (dev->leb_size - (dev->leb_size / dev->min_io_size * 4))) {
			mt_ftl_err(dev, "replay:%d, param->u4BIT[%d]=%d, sector[0x%x], cluster[%d]",
				replay, old_leb, param->u4BIT[old_leb], (u32)sector, cluster);
			dump_stack();
		}
	}
	if (cmpr_data_size == 0) {
		*pmt = NAND_DEFAULT_VALUE;
		*meta_pmt = NAND_DEFAULT_VALUE;
	} else {
		PMT_SET_BLOCKPAGE(*pmt, leb, offset / dev->min_io_size);
		META_PMT_SET_DATA(*meta_pmt, cmpr_data_size, part, -1);
	}
	/* PMT_SET_BLOCKPAGE(*pmt, leb, offset / dev->min_io_size);
	 * META_PMT_SET_DATA(*meta_pmt, cmpr_data_size, part, -1);
	 */
	/* Data not in cache */
	PMT_INDICATOR_SET_DIRTY(param->u4PMTIndicator[cluster]);
	mt_ftl_debug(dev, "u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_UPDATE_PMT_MODIFYPMT);
	if (!param->gc_dat_cmit_flag) {
		if ((param->pmt_updated_flag == PMT_COMMIT) || (param->get_pmt_blk_flag == 1)) {
			mt_ftl_err(dev, "pmt_updated_flag(%d),get_pmt_blk=%d",
				param->pmt_updated_flag, param->get_pmt_blk_flag);
			mt_ftl_commit(dev);
		}
		param->get_pmt_blk_flag = 0;
	}
	param->pmt_updated_flag = PMT_END;
	return ret;
}

int mt_ftl_flush(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;

	if (param->u4DataNum[0] || param->data_cache_num || param->u4NextPageOffsetIndicator) {
		mt_ftl_err(dev, "FTL Flush!");
		ret = mt_ftl_commit(dev);
	}
#ifdef MT_FTL_SUPPORT_WBUF
	mt_ftl_wakeup_wbuf_thread(dev);
#endif

	return ret;
}

static int mt_ftl_write_cache(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret;
	int offset, buf_offset;
	int data_len, part;
	u32 pmt, meta_pmt;
	int data_cache_num, last_data_len;
	struct mt_ftl_data_header *header_buffer;
	struct mt_ftl_param *param = dev->param;

	ret = mt_ftl_get_pmt(dev, sector, &pmt, &meta_pmt);
	if (ret || pmt == NAND_DEFAULT_VALUE)
		return MT_FTL_FAIL;
	if (PMT_IS_DATA_INCACHE(meta_pmt)) {
		data_cache_num = PMT_GET_DATACACHENUM(meta_pmt);
		part = PMT_GET_PART(meta_pmt);
		buf_offset = (data_cache_num + 1) * dev->min_io_size -
			(part + 1) * sizeof(struct mt_ftl_data_header) - 4;
		header_buffer = (struct mt_ftl_data_header *)&param->u1DataCache[buf_offset];
		offset = FDATA_OFFSET(header_buffer->offset_len);
		data_len = FDATA_LEN(header_buffer->offset_len);
		if (data_len != len || header_buffer->sector != sector) {
			mt_ftl_err(dev, "write len %d %d %lx %lx", data_len, len,
				(unsigned long)header_buffer->sector, (unsigned long)sector);
			return MT_FTL_FAIL;
		}
		mt_ftl_debug(dev, "write into cache sector=0x%x", (u32)sector);
		buf_offset = data_cache_num * dev->min_io_size + offset;
		last_data_len = dev->min_io_size - offset - ((part + 1) * sizeof(struct mt_ftl_data_header) + 4);
		/* copy the last data into cache */
		mt_ftl_debug(dev, "write cache %d %d %d %d", data_cache_num, part, offset, last_data_len);
		if (last_data_len < data_len && data_cache_num < param->data_cache_num) {
			memcpy(&param->u1DataCache[buf_offset], buffer, last_data_len);
			buf_offset = (data_cache_num + 1) * dev->min_io_size;
			memcpy(&param->u1DataCache[buf_offset], buffer + last_data_len, data_len - last_data_len);
		} else
			memcpy(&param->u1DataCache[buf_offset], buffer, len);
		return MT_FTL_SUCCESS;
	}

	return MT_FTL_FAIL;
}

/* Suppose FS_PAGE_SIZE for each write */
int mt_ftl_write(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret = MT_FTL_SUCCESS;
	int leb = 0, page = 0;
	u32 cluster = 0, sec_offset = 0;
	int cache_num = 0, header_buf_offset, data_buf_offset;
	int *meta_pmt = NULL;
	u32 cmpr_len = 0, data_offset = 0, total_consumed_size = 0;
	int commit_data = 0, data_num;
	int last_data_len = 0, data_cache_num;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	struct mt_ftl_data_header *header_buffer;

	trace_mt_ftl_write(dev, sector, len);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_START_ALL(MT_FTL_PROFILE_WRITE_ALL);

	ret = mt_ftl_compress(dev, buffer, len, &cmpr_len);
	if (ret) {
		mt_ftl_err(dev, "ret = %d, cmpr_len = %d, len = 0x%x", ret, cmpr_len, len);
		mt_ftl_err(dev, "cc = 0x%lx, buffer = 0x%lx", (unsigned long int)param->cc, (unsigned long int)buffer);
		mt_ftl_err(dev, "cmpr_page_buffer = 0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}

	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_WRITEPAGE);

	/* write data cache */
	if (param->data_cache_num || param->u4DataNum[0]) {
		ret = mt_ftl_write_cache(dev, param->cmpr_page_buffer, sector, cmpr_len);
		if (ret == MT_FTL_SUCCESS) {
			mt_ftl_err(dev, "write into cahce");
			return MT_FTL_SUCCESS;
		}
	}

	cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> dev->pm_per_io);
	sec_offset = (u32)(FS_PAGE_OF_SECTOR(sector) & ((1 << dev->pm_per_io) - 1));
	if (param->u4DataNum[param->data_cache_num] > 0) {
		data_buf_offset = param->data_cache_num * dev->min_io_size;
		header_buf_offset = data_buf_offset + dev->min_io_size -
			param->u4DataNum[param->data_cache_num] * sizeof(struct mt_ftl_data_header) - 4;
		header_buffer = (struct mt_ftl_data_header *)(&param->u1DataCache[header_buf_offset]);
		data_offset = FDATA_OFFSET(header_buffer->offset_len) + FDATA_LEN(header_buffer->offset_len);
		param->u4NextPageOffsetIndicator = 0;
	} else
		data_offset = param->u4NextPageOffsetIndicator;
	total_consumed_size = data_offset + cmpr_len +
		(param->u4DataNum[param->data_cache_num] + 1) * sizeof(struct mt_ftl_data_header) + 4;
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
	page += param->data_cache_num;
	if (total_consumed_size > dev->min_io_size) {
		last_data_len = dev->min_io_size - data_offset -
			((param->u4DataNum[param->data_cache_num] + 1) * sizeof(struct mt_ftl_data_header) + 4);
		/* support the block full or cache full write */
		if ((page + 1) == page_num_per_block) {
			mt_ftl_debug(dev, "write page %d %d %d", param->data_cache_num, page, last_data_len);
			commit_data = 1;
		} else if (last_data_len <= 0) {
			mt_ftl_debug(dev, "write_page=(%d:%d),data_offset=%d,last_data_len=%d",
				page, leb, data_offset, last_data_len);
			if ((param->data_cache_num + 1) == MAX_DATA_CACHE_NUM) {
				mt_ftl_debug(dev, "cache full %d %d %d", param->data_cache_num, page, last_data_len);
				commit_data = 1;
			} else {
				data_offset = 0;
				last_data_len = 0;
				param->data_cache_num++;
				page++;
			}
		}
		if (commit_data) {
			ret = mt_ftl_write_page(dev);
			data_offset = 0;
			last_data_len = 0;
			leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
			page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
		}
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_COPYTOCACHE);

	param->u4DataNum[param->data_cache_num]++;
	data_buf_offset = param->data_cache_num * dev->min_io_size;
	header_buf_offset = data_buf_offset + dev->min_io_size -
		param->u4DataNum[param->data_cache_num] * sizeof(struct mt_ftl_data_header) - 4;
	header_buffer = (struct mt_ftl_data_header *)(&param->u1DataCache[header_buf_offset]);
	header_buffer->sector = FS_PAGE_OF_SECTOR(sector) << SECS_OF_SHIFT;
	header_buffer->offset_len = (data_offset << FDATA_SHIFT) | cmpr_len;
	data_buf_offset += data_offset;
	if (last_data_len) {
		ubi_assert(data_offset + last_data_len < dev->min_io_size);
		memcpy(&param->u1DataCache[data_buf_offset], param->cmpr_page_buffer, last_data_len);
	} else {
		ubi_assert(data_offset + cmpr_len < dev->min_io_size);
		memcpy(&param->u1DataCache[data_buf_offset], param->cmpr_page_buffer, cmpr_len);
	}
	if (cmpr_len == 0)
		BIT_UPDATE(param->u4BIT[leb], sizeof(struct mt_ftl_data_header));
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_COPYTOCACHE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_UPDATEPMT);
	/* record the data cache num & data num for update PMT*/
	data_cache_num = param->data_cache_num;
	data_num = param->u4DataNum[data_cache_num];
	if (last_data_len) {
		if (param->u4DataNum[0]) {
			if ((param->data_cache_num + 1) == MAX_DATA_CACHE_NUM)
				ret = mt_ftl_write_page(dev);
			else
				param->data_cache_num++;
		}
		data_offset = param->data_cache_num * dev->min_io_size;
		param->u4NextPageOffsetIndicator = cmpr_len - last_data_len;
		memcpy(&param->u1DataCache[data_offset], &param->cmpr_page_buffer[last_data_len],
			param->u4NextPageOffsetIndicator);
		mt_ftl_debug(dev, "last_data:%u %d %d %d", param->u4NextPageOffsetIndicator,
			param->data_cache_num, data_offset, page);
	}
	mt_ftl_debug(dev, "pmt: %lx %d %d %d %d",
		(unsigned long)sector, leb, page, param->u4DataNum[data_cache_num], data_cache_num);
	ret = mt_ftl_updatePMT(dev, sector, leb, page * dev->min_io_size, data_num - 1, cmpr_len, false, true);
	if (ret < 0) {
		mt_ftl_err(dev, "mt_ftl_updatePMT sector(0x%lx) leb(%d) page(%d) fail\n",
			(unsigned long)sector, leb, page);
		return ret;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_WRITE_UPDATEPMT);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	ubi_assert(cluster < commit_node->dev_clusters);
	cache_num = PMT_INDICATOR_CACHE_BUF_NUM(param->u4PMTIndicator[cluster]);
	ubi_assert(cache_num < PMT_CACHE_NUM);
	meta_pmt = &param->u4MetaPMTCache[(cache_num << dev->pm_per_io) + sec_offset];
	if (param->u4DataNum[data_cache_num] != 0)
		PMT_SET_DATACACHE_BUF_NUM(*meta_pmt, data_cache_num);
	PMT_INDICATOR_SET_DIRTY(param->u4PMTIndicator[cluster]);	/* Set corresponding PMT cache to dirty */

	MT_FTL_PROFILE_START(MT_FTL_PROFILE_WRITE_WRITEPAGE);
	MT_FTL_PROFILE_END_ALL(MT_FTL_PROFILE_WRITE_ALL);
	return ret;
}

/* Suppose FS_PAGE_SIZE for each read */
int mt_ftl_read(struct mt_ftl_blk *dev, char *buffer, sector_t sector, int len)
{
	int ret = MT_FTL_SUCCESS;
	int leb = 0, page = 0, part = 0;
	u32 pmt = 0, meta_pmt = 0;
	u32 offset = 0, data_len = 0;
	int data_cache_num = 0, buf_offset;
	u32 decmpr_len = 0;
	u32 data_num = 0, cluster = 0;
	int next_block = 0, next_page = 0;
	int last_data_len = 0, pmt_leb = 0;
	int i = 0;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	u32 data_hdr_offset = 0;
	unsigned char *page_buffer = NULL;
	struct mt_ftl_data_header *header_buffer = NULL;

	trace_mt_ftl_read(dev, sector, len);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_GETPMT);
	MT_FTL_PROFILE_START_ALL(MT_FTL_PROFILE_READ_ALL);

	ret = mt_ftl_get_pmt(dev, sector, &pmt, &meta_pmt);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_GETPMT);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_DATATOCACHE);
	/* Look up PMT in cache and get data from NAND flash */
	if (pmt == NAND_DEFAULT_VALUE) {
		if (sector <= 24)
			mt_ftl_err(dev, "PMT of sector(0x%lx) is invalid", (unsigned long int)sector);
		memset((void *)buffer, 0x0, len);
		return MT_FTL_SUCCESS;
	} else if (ret)
		return MT_FTL_FAIL;
	leb = PMT_GET_BLOCK(pmt);
	page = PMT_GET_PAGE(pmt);
	part = PMT_GET_PART(meta_pmt);

	/* mt_ftl_debug(dev, "%lx %d %d %d", (unsigned long)sector, leb, page, part); */
	/* mt_ftl_err(dev, "Copy to cache"); */
	/** check the leb/page/part info **/
	if (leb >= commit_node->dev_blocks ||
		page >= (dev->leb_size / dev->min_io_size)) {
		mt_ftl_err(dev, "pmt:%08X %08X", pmt, meta_pmt);
		mt_ftl_err(dev, "leb:%d page:%d part:%d", leb, page, part);
		mt_ftl_dump_info(dev);
		memset((void *)buffer, 0x0, len);
		return MT_FTL_SUCCESS;
	}
	if (PMT_IS_DATA_INCACHE(meta_pmt)) {
		data_cache_num = PMT_GET_DATACACHENUM(meta_pmt);
		buf_offset = (data_cache_num + 1) * dev->min_io_size -
			(part + 1) * sizeof(struct mt_ftl_data_header) - 4;
		header_buffer = (struct mt_ftl_data_header *)&param->u1DataCache[buf_offset];
		if (header_buffer->sector != sector) {
			mt_ftl_err(dev, "inconformity sector: %lx %lx",
				(unsigned long)header_buffer->sector, (unsigned long)sector);
			return MT_FTL_FAIL;
		}
		offset = FDATA_OFFSET(header_buffer->offset_len);
		data_len = FDATA_LEN(header_buffer->offset_len);
		if (data_len == 0) {
			memset((void *)buffer, 0x0, len);
			mt_ftl_debug(dev, "sector is discard[0] %lx %lx",
				(unsigned long)sector, (unsigned long)header_buffer->offset_len);
			return MT_FTL_SUCCESS;
		}
		buf_offset = data_cache_num * dev->min_io_size + offset;
		last_data_len = dev->min_io_size - offset - ((part + 1) * sizeof(struct mt_ftl_data_header) + 4);
		/* copy the last data in cache */
		if (last_data_len < data_len && data_cache_num < param->data_cache_num) {
			memcpy((char *)param->general_page_buffer, &param->u1DataCache[buf_offset], last_data_len);
			buf_offset = (data_cache_num + 1) * dev->min_io_size;
			memcpy((char *)param->general_page_buffer + last_data_len, &param->u1DataCache[buf_offset],
					data_len - last_data_len);
			page_buffer = (unsigned char *)param->general_page_buffer;
		} else
			page_buffer = &param->u1DataCache[buf_offset];
	} else {
		data_hdr_offset = dev->min_io_size - (part + 1) * sizeof(struct mt_ftl_data_header) - 4;
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		header_buffer = (struct mt_ftl_data_header *)((char *)param->tmp_page_buffer + data_hdr_offset);
		offset = FDATA_OFFSET(header_buffer->offset_len);
		data_len = FDATA_LEN(header_buffer->offset_len);
		if (data_len == 0) {
			memset((void *)buffer, 0x0, len);
			mt_ftl_err(dev, "confuse:sector is discard[1] %lx %lx",
				(unsigned long)sector, (unsigned long)header_buffer->offset_len);
			return MT_FTL_SUCCESS;
		}

		last_data_len = dev->min_io_size - offset - ((part + 1) * sizeof(struct mt_ftl_data_header) + 4);
		mt_ftl_debug(dev, "last_data_len %d %d %d", last_data_len, offset, data_len);
		if (last_data_len < data_len) {
			memcpy(param->general_page_buffer, (char *)param->tmp_page_buffer + offset, last_data_len);
			next_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
			next_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
			if (leb == next_block && (page + 1) == next_page) {
				mt_ftl_err(dev, "read from %d %d", next_block, next_page);
				memcpy((char *)param->general_page_buffer + last_data_len, param->u1DataCache,
					data_len - last_data_len);
			} else {
				ret = mt_ftl_leb_read(dev, leb, (char *)param->general_page_buffer +
					last_data_len, (page + 1) * dev->min_io_size, data_len - last_data_len);
				if (ret)
					return MT_FTL_FAIL;
			}
		} else
			memcpy(param->general_page_buffer, (char *)param->tmp_page_buffer + offset, data_len);
		page_buffer = (unsigned char *)param->general_page_buffer;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_DATATOCACHE);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_ADDRNOMATCH);

	/* mt_ftl_err(dev,  "Check sector"); */
	if (header_buffer->sector != (FS_PAGE_OF_SECTOR(sector) << SECS_OF_SHIFT)) {
		for (i = 0; i < PMT_BLOCK_NUM; i++) {
			mt_ftl_err(dev, "pmt_leb[%d]-->peb[%d]",
				(i + PMT_START_BLOCK), dev->desc->vol->eba_tbl[i + PMT_START_BLOCK]);
		}
		if ((header_buffer->sector == NAND_DEFAULT_SECTOR_VALUE) && (page_buffer[0] == 0xFF)) {
			mt_ftl_err(dev, "sector(0x%lx) hasn't been written", (unsigned long int)sector);
			mt_ftl_err(dev, "leb = %d, page = %d, part = %d", leb, page, part);
			memset((void *)buffer, 0xFF, len);
			return MT_FTL_SUCCESS;
		}
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		cluster = (u32)(FS_PAGE_OF_SECTOR(sector) >> (dev->pm_per_io));
		data_num = PAGE_GET_DATA_NUM(param->tmp_page_buffer[(dev->min_io_size >> 2) - 1]);
		mt_ftl_err(dev, "param->u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);
		mt_ftl_err(dev, "for sector (0x%lx), pmt = 0x%x, meta_pmt = 0x%x, leb = %d, page = %d, part = %d",
			   (unsigned long int)sector, pmt, meta_pmt, leb, page, part);
		pmt_leb = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
		mt_ftl_err(dev, "pmt_peb=%d, leb_peb=%d", dev->desc->vol->eba_tbl[pmt_leb],
			dev->desc->vol->eba_tbl[leb]);
		mt_ftl_err(dev, "data_offset=%d,data_hdr_offset=%d data_num=%d", offset, data_hdr_offset, data_num);
		mt_ftl_err(dev, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 1],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 2],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 3],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 4],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 5],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 6],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 7],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 8],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) - 9]);
		ret = mt_ftl_get_pmt(dev, header_buffer->sector, &pmt, &meta_pmt);
		cluster = (u32)(FS_PAGE_OF_SECTOR(header_buffer->sector) >> (dev->pm_per_io));
		leb = PMT_GET_BLOCK(pmt);
		page = PMT_GET_PAGE(pmt);
		part = PMT_GET_PART(meta_pmt);
		pmt_leb = PMT_INDICATOR_GET_BLOCK(param->u4PMTIndicator[cluster]);
		mt_ftl_err(dev, "header_buffer[%d].sector(0x%x)!=sector(0x%x),header_buffer[%d].offset_len=0x%lx",
			   part, (u32)header_buffer->sector, (u32)sector, part,
			   (unsigned long)header_buffer->offset_len);
		mt_ftl_err(dev, "pmt->peb=%d, leb->peb=%d", dev->desc->vol->eba_tbl[pmt_leb],
			dev->desc->vol->eba_tbl[leb]);
		mt_ftl_err(dev, "param->u4PMTIndicator[%d] = 0x%x", cluster, param->u4PMTIndicator[cluster]);
		mt_ftl_err(dev, "for header_buffer->sector (0x%lx), pmt = 0x%x, meta_pmt = 0x%x, leb = %d, page = %d, part = %d",
			   (unsigned long int)header_buffer->sector, pmt, meta_pmt, leb, page, part);
		/*====================================================*/
		/* not to report error, but set 0 for fs*/
		memset((void *)buffer, 0, len);
		return MT_FTL_SUCCESS;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_ADDRNOMATCH);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_DECOMP);

	decmpr_len = dev->min_io_size;
	ret = mt_ftl_decompress(dev, page_buffer, FDATA_LEN(header_buffer->offset_len), &decmpr_len);
	if (ret) {
		ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, page * dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		mt_ftl_err(dev, "part = %d", part);
		mt_ftl_err(dev, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1)],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 1],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 2],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 3],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 4],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 5],
			   param->tmp_page_buffer[(dev->min_io_size >> 2) -
						  ((part + 2) * sizeof(struct mt_ftl_data_header) + 1) + 6]);
		mt_ftl_err(dev, "ret = %d, decmpr_len = %d, header_buffer->offset_len = 0x%lx",
			   ret, decmpr_len, (unsigned long)header_buffer->offset_len);
		mt_ftl_err(dev, "cc = 0x%lx, page_buffer = 0x%lx",
			   (unsigned long int)param->cc, (unsigned long int)page_buffer);
		mt_ftl_err(dev, "cmpr_page_buffer = 0x%lx", (unsigned long int)param->cmpr_page_buffer);
		return MT_FTL_FAIL;
	}
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_DECOMP);
	MT_FTL_PROFILE_START(MT_FTL_PROFILE_READ_COPYTOBUFF);

	offset = (u32)((sector & (SECS_OF_FS_PAGE - 1)) << 9);
	if ((offset + len) > decmpr_len) {
		mt_ftl_err(dev, "offset(%d)+len(%d)>decmpr_len (%d)", offset, len, decmpr_len);
		return MT_FTL_FAIL;
	}
	/* mt_ftl_err(dev,  "Copy to buffer"); */
	ubi_assert(offset + len <= dev->min_io_size);
	memcpy((void *)buffer, &param->cmpr_page_buffer[offset], len);
	MT_FTL_PROFILE_END(MT_FTL_PROFILE_READ_COPYTOBUFF);
	MT_FTL_PROFILE_END_ALL(MT_FTL_PROFILE_READ_ALL);
	return ret;
}

static int mt_ftl_replay_single_block(struct mt_ftl_blk *dev, int leb, int *page)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int offset = 0;
	sector_t sector = 0;
	u32 data_num = 0, data_hdr_offset = 0;
	int retry = 0, last_data_len = 0;
	struct mt_ftl_data_header *header_buffer = NULL;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	ret = ubi_is_mapped(dev->desc, leb);
	if (ret == 0) {
		mt_ftl_debug(dev, "leb%d is unmapped", leb);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, *page);
		return MT_FTL_SUCCESS;
	}
	offset = (*page) * dev->min_io_size;
	if (offset == dev->leb_size) {
		mt_ftl_debug(dev, "replay offset = leb size");
		return MT_FTL_SUCCESS;
	}
first_page:
	ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, offset, dev->min_io_size);
	if (ret)
		return MT_FTL_FAIL;

	data_num = PAGE_GET_DATA_NUM(param->general_page_buffer[(dev->min_io_size >> 2) - 1]);
	if (data_num == LAST_DATA_TAG) {
		mt_ftl_debug(dev, "NextLebPage:data_num = 0x3FFFFFFF,read next page, page(%d)", *page);
		offset += dev->min_io_size;
		*page += 1;
		goto first_page;
	}
	if (data_num == PAGE_READ_TAG) {
		if (offset >= dev->min_io_size) {
			ret = mt_ftl_leb_read(dev, leb, param->tmp_page_buffer, (offset - dev->min_io_size),
				dev->min_io_size);
			data_num = PAGE_GET_DATA_NUM(param->tmp_page_buffer[(dev->min_io_size >> 2) - 1]);
			if (data_num == LAST_DATA_TAG)
				mt_ftl_err(dev, "last page data_num=LAST_DATA_TAG, break");
			else {
				mt_ftl_err(dev, "data_num(%d) != LAST_DATA_TAG", data_num);
				data_hdr_offset = dev->min_io_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
				header_buffer =
					(struct mt_ftl_data_header *)(&param->tmp_page_buffer[data_hdr_offset >> 2]);
				last_data_len = dev->min_io_size - (FDATA_OFFSET(header_buffer->offset_len)
						+ FDATA_LEN(header_buffer->offset_len)
						+ (data_num * sizeof(struct mt_ftl_data_header) + 4));
				if (last_data_len < 0) {
					sector = header_buffer->sector;
					mt_ftl_err(dev, "power off: data of last page isn't write");
					mt_ftl_updatePMT(dev, sector, leb, (offset - dev->min_io_size), data_num - 1,
								0, true, false);
					BIT_UPDATE(param->u4BIT[leb], last_data_len);
				}
			}
		}
		mt_ftl_err(dev, "End of block,page=%d", *page);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, *page);
		return MT_FTL_SUCCESS;
	}
	data_hdr_offset = dev->min_io_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);

	header_buffer = (struct mt_ftl_data_header *)(&param->general_page_buffer[data_hdr_offset >> 2]);

	mt_ftl_debug(dev, "leb = %d, page = %d, data_num = %d", leb, *page, data_num);
	while (data_num && (offset <= max_offset_per_block)) {
		/* Update param->u4NextLebPageIndicator
		 * check the correctness of param->u4NextLebPageIndicator &
		 * if param->u4NextLebPageIndicator is full, need to call get free block & page function
		 */

		/* Update param->u4PMTCache and param->u4PMTIndicator and param->u4BIT */
		/* If the page is copied in GC, that means the page should not be replayed */
		retry = 1;
		last_data_len = dev->min_io_size - (FDATA_OFFSET(header_buffer[0].offset_len) +
			FDATA_LEN(header_buffer[0].offset_len) + (data_num * sizeof(struct mt_ftl_data_header) + 4));
		if (last_data_len > 0)
			BIT_UPDATE(param->u4BIT[leb], last_data_len);

		if (PAGE_BEEN_READ(param->general_page_buffer[(dev->min_io_size >> 2) - 1]) == 0) {
			for (i = 0; i < data_num; i++) {
				/* Get sector in the page */
				sector = header_buffer[data_num - i - 1].sector;
				if (FDATA_LEN(header_buffer[data_num - i - 1].offset_len) == 0)
					BIT_UPDATE(param->u4BIT[leb], sizeof(struct mt_ftl_data_header));
				if ((sector & NAND_DEFAULT_SECTOR_VALUE) == NAND_DEFAULT_SECTOR_VALUE) {
					mt_ftl_err(dev, "header_buffer[%d].sector == 0xFFFFFFFF, leb = %d, page = %d",
						   i, leb, offset / dev->min_io_size);
					continue;
				}
				mt_ftl_updatePMT(dev, sector, leb, offset, i,
						 FDATA_LEN(header_buffer[data_num - i - 1].offset_len), true, true);
			}
		}
retry_once:
		offset += dev->min_io_size;
		*page = offset / dev->min_io_size;
		if (offset > max_offset_per_block)
			break;
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, *page);
		ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, offset, dev->min_io_size);
		if (ret == MT_FTL_FAIL) {
			if (last_data_len < 0) {
				mt_ftl_err(dev, "power off:last page isn't write completely");
				mt_ftl_updatePMT(dev, sector, leb, offset, data_num - 1, 0, true, true);
				BIT_UPDATE(param->u4BIT[leb], last_data_len);
			}
			/* write page power loss recover */
			ret = mt_ftl_leb_recover(dev, leb, offset - dev->min_io_size, 0);
			if (ret == 1)
				ret = MT_FTL_SUCCESS;
			break;
		}

		data_num = PAGE_GET_DATA_NUM(param->general_page_buffer[(dev->min_io_size >> 2) - 1]);
		mt_ftl_debug(dev, "%d, %d, %u, %d", retry, last_data_len, data_num, *page);
		if ((data_num == LAST_DATA_TAG) && retry && last_data_len < 0) {
			mt_ftl_debug(dev, "replay: read retry, last_data_len=%d, page=%d", last_data_len, *page);
			retry = 0;
			BIT_UPDATE(param->u4BIT[leb], dev->min_io_size + last_data_len - 4);
			goto retry_once;
		}
		if (data_num == PAGE_READ_TAG) {
			if (last_data_len < 0) {
				mt_ftl_err(dev, "power off: data of last page isn't write");
				mt_ftl_updatePMT(dev, sector, leb, offset, data_num - 1, 0, true, true);
				BIT_UPDATE(param->u4BIT[leb], last_data_len);
			}
			break;
		}
		data_hdr_offset = dev->min_io_size - (data_num * sizeof(struct mt_ftl_data_header) + 4);
		header_buffer = (struct mt_ftl_data_header *)(&param->general_page_buffer[data_hdr_offset >> 2]);
	}

	mt_ftl_err(dev, "offset = %d at the end, max_offset_per_block = %d, retry=%d, last_data_len=%d, data_num=%u, page=%d",
		offset, max_offset_per_block, retry, last_data_len, data_num, *page);

	return ret;
}
static int mt_ftl_check_replay_blk(struct mt_ftl_blk *dev, int offset, int *leb)
{
	int ret = 0, magic = 0;
	int replay_blk, replay_offset;
	struct mt_ftl_param *param = dev->param;

	*leb = MT_INVALID_BLOCKPAGE;
	replay_blk = offset / dev->leb_size + REPLAY_BLOCK;
	replay_offset = offset % dev->leb_size;
	mt_ftl_debug(dev, "replay_blk:%d replay_offset:%d", replay_blk, replay_offset);
	/* Get the successive lebs to replay */
	ret = ubi_is_mapped(dev->desc, replay_blk);
	if (ret == 0) {
		mt_ftl_err(dev, "leb%d is unmapped", replay_blk);
		return 0;
	}
	if (offset >= (dev->max_replay_blks * dev->min_io_size))
		return 0;
	ret = mt_ftl_leb_read(dev, replay_blk, param->replay_page_buffer,
		replay_offset, sizeof(unsigned int) * 2);
	if (ret) {
		mt_ftl_leb_recover(dev, replay_blk, replay_offset, 0);
		return 0;
	}
	magic = param->replay_page_buffer[0];
	if (magic == MT_MAGIC_REPLAY) {
		*leb = param->replay_page_buffer[1];
		return 1; /* case 1 for gc done */
	}
	return 0;
}

static int mt_ftl_replay(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i;
	int leb = 0, page = 0, offset = 0, full = 1, first = 0;
	int nextleb_in_replay = MT_INVALID_BLOCKPAGE;
	int pmt_block = 0, pmt_page = 0, tmp_leb;
	struct mt_ftl_param *param = dev->param;
	const int max_replay_offset = dev->leb_size * dev->max_replay_blks;
	const int page_num_per_block = dev->leb_size / dev->min_io_size;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	if (commit_node->u4BlockDeviceModeFlag) {
		mt_ftl_err(dev, "[Bean]Read Only, no need replay");
		param->replay_pmt_blk_rec[param->replay_pmt_blk_index] =
			PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4CurrentPMTLebPageIndicator);
		param->replay_pmt_blk_index++;
		param->replay_blk_rec[param->replay_blk_index] =
			PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
		param->replay_blk_index++;
		return ret;
	}
	param->gc_pmt_charge_blk = commit_node->u4PMTChargeLebIndicator >> PMT_CHARGE_LEB_SHIFT;
	commit_node->u4PMTChargeLebIndicator &= ((1 << PMT_CHARGE_LEB_SHIFT) - 1);
	offset = commit_node->u4NextReplayOffsetIndicator;
	param->replay_blk_index = commit_node->u4NextReplayOffsetIndicator / dev->min_io_size % dev->max_replay_blks;
	mt_ftl_debug(dev, "replay offset: %d", offset);
	/* check the last replay block */
	ret = mt_ftl_check_replay_blk(dev, offset, &tmp_leb);
	if (!ret)
		param->replay_pmt_last_cmit = 1;
	/* Replay leb/page of param->u4NextLebPageIndicator */
	leb = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4NextLebPageIndicator);
	page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4NextLebPageIndicator);
	ret = mt_ftl_replay_single_block(dev, leb, &page);
	if (ret)
		return ret;
	if (page == page_num_per_block) {
		mt_ftl_debug(dev, "replay getfreeblock");
		first = 1;
	} else {
		mt_ftl_debug(dev, "replay nextlebpage only");
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, page);
		goto out;
	}
	/* replay replay_blk recorded block */
	while (offset < max_replay_offset) {
		ret = mt_ftl_check_replay_blk(dev, offset, &nextleb_in_replay);
		if (!ret)
			break;
		/* check the last replay block */
		ret = mt_ftl_check_replay_blk(dev, offset + dev->min_io_size, &tmp_leb);
		if (!ret)
			param->replay_pmt_last_cmit = 1;
		if (page != page_num_per_block) {
			mt_ftl_err(dev, "last replay error");
			ubi_assert(false);
			break;
		}
		/* check the first get free block */
		if (full) {
			if (first) {
				/* get the first free leb */
				leb = mt_ftl_get_free_block(dev, &page, false, true);
				first = 0;
			}
			mt_ftl_debug(dev, "[Bean]nextlebpage replay done %d %d", leb, nextleb_in_replay);
			if (leb == nextleb_in_replay) {
				mt_ftl_debug(dev, "replay next leb in replay block");
				full = 0;
			} else {
				offset += dev->min_io_size;
				mt_ftl_debug(dev, "replay replayblock mapped");
				continue;
			}
		} else {
			leb = mt_ftl_get_free_block(dev, &page, false, true);
			if (leb != nextleb_in_replay) {
				mt_ftl_err(dev, "leb(%d)!=nextleb_in_replay(%d)", leb, nextleb_in_replay);
				return MT_FTL_FAIL;
			}
		}
		offset += dev->min_io_size;
		ret = mt_ftl_replay_single_block(dev, leb, &page);
		if (ret)
			return ret;
	}
out:
	/* recover power cut data block */
	ret = mt_ftl_leb_recover(dev, leb, page * dev->min_io_size, 0);
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover data blk fail %d %d", leb, page);
		return ret;
	}
	/* recover power cut pmt block */
	pmt_block = PMT_LEB_PAGE_INDICATOR_GET_BLOCK(commit_node->u4CurrentPMTLebPageIndicator);
	pmt_page = PMT_LEB_PAGE_INDICATOR_GET_PAGE(commit_node->u4CurrentPMTLebPageIndicator);
	ret = mt_ftl_leb_recover(dev, pmt_block, pmt_page * dev->min_io_size, 0);
	if (ret == MT_FTL_FAIL) {
		mt_ftl_err(dev, "recover pmt blk fail %d %d", pmt_block, pmt_page);
		return ret;
	}
	ret = MT_FTL_SUCCESS;
	/* recover get free block */
	if (page == page_num_per_block) {
		mt_ftl_err(dev, "replay_blk_index:%d", param->replay_blk_index);
		leb = mt_ftl_get_free_block(dev, &page, false, false);
		PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, leb, page);
	}
	/* reset Block device flag to read only*/
	if (dev->readonly)
		commit_node->u4BlockDeviceModeFlag = 1;
	/* reset the replay block */
	for (i = 0; i < REPLAY_BLOCK_NUM; i++)
		mt_ftl_leb_remap(dev, REPLAY_BLOCK + i);
	commit_node->u4NextReplayOffsetIndicator = 0;
	/* reset replay flags*/
	param->replay_pmt_last_cmit = 0;
	param->record_last_pmt_blk = -1;
	/* forced commit after replay */
	mt_ftl_commit(dev);

	return ret;
}

static int mt_ftl_alloc_single_buffer(unsigned int **buf, int size, char *str)
{
	if (*buf == NULL) {
		*buf = kmalloc(size, GFP_KERNEL);
		if (!*buf)
			return -ENOMEM;
	}
	memset(*buf, 0xFF, size);

	return 0;
}

static int mt_ftl_alloc_buffers(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	ret = mt_ftl_alloc_single_buffer(&param->u4PMTIndicator,
					 commit_node->dev_clusters * sizeof(unsigned int),
					 "param->u4PMTIndicator");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4PMTCache,
					 PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4PMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4MetaPMTCache,
					 PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4MetaPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->i4CurrentPMTClusterInCache,
					 PMT_CACHE_NUM * sizeof(unsigned int),
					 "param->i4CurrentPMTClusterInCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4ReadPMTCache,
					 sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4ReadPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4ReadMetaPMTCache,
					 sizeof(unsigned int) << dev->pm_per_io,
					 "param->u4ReadMetaPMTCache");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->u4BIT,
					 commit_node->dev_blocks * sizeof(unsigned int),
					 "param->u4BIT");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->u1DataCache,
					 MAX_DATA_CACHE_NUM * dev->min_io_size * sizeof(unsigned char),
					 "param->u1DataCache");
	if (ret)
		return ret;


	ret = mt_ftl_alloc_single_buffer(&param->replay_blk_rec,
					 dev->max_replay_blks * sizeof(unsigned int),
					 "param->replay_blk_rec");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->replay_pmt_blk_rec,
					dev->max_pmt_replay_blk * sizeof(unsigned int),
					"param->replay_pmt_blk_rec");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->general_page_buffer,
					 dev->min_io_size,
					 "param->general_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->replay_page_buffer,
					 dev->min_io_size,
					 "param->replay_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->commit_page_buffer,
					 dev->min_io_size,
					 "param->commit_page_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->gc_page_buffer,
					 dev->min_io_size,
					 "param->gc_page_buffer");
	if (ret)
		return ret;
	ret = mt_ftl_alloc_single_buffer(&param->gc_pmt_buffer,
					 dev->min_io_size,
					 "param->gc_pmt_buffer");
	if (ret)
		return ret;

	ret = mt_ftl_alloc_single_buffer(&param->tmp_page_buffer,
					 dev->min_io_size,
					 "param->tmp_page_buffer");
	if (ret)
		return ret;

	return ret;
}


/* DOWNLOAD image as 1 page for commit indicators */
static int mt_ftl_reload_config_blk(struct mt_ftl_blk *dev, unsigned int *buf)
{
	unsigned int index, dest_index;
	unsigned int dev_blocks, dev_clusters;
	unsigned int min_dev_blocks, min_dev_clusters;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	/* check reloaded or not */
	if (PAGE_BEEN_READ(buf[1]) == 0) {
		mt_ftl_err(dev, "No need reloaded image! 0x%08X", buf[1]);
		return 0;
	}
	/* write to backup block */
	ubi_leb_unmap(dev->desc, CONFIG_START_BLOCK + 1);
	mt_ftl_leb_write(dev, CONFIG_START_BLOCK + 1, buf, 0, dev->min_io_size);
	/* reload parameters */
	buf[1] = MT_FTL_VERSION;
	dev_blocks = buf[2];
	buf[2] = commit_node->dev_blocks;
	dev_clusters = buf[3];
	buf[3] = commit_node->dev_clusters;
	/* calc min size */
	min_dev_clusters = min(dev_clusters, commit_node->dev_clusters);
	min_dev_blocks = min(dev_blocks, commit_node->dev_blocks);
	mt_ftl_err(dev, "%d %d %d %d", dev_clusters, commit_node->dev_clusters, dev_blocks, commit_node->dev_blocks);
	memset(param->tmp_page_buffer, 0, dev->min_io_size);
	/* copy commit node */
	dest_index = index = sizeof(struct mt_ftl_commit_node) / sizeof(unsigned int);
	memcpy(param->tmp_page_buffer, buf, sizeof(struct mt_ftl_commit_node));
	/* copy u4PMTIndicator, as no larger than one page */
	memcpy(&param->tmp_page_buffer[dest_index], &buf[index], min_dev_clusters * sizeof(unsigned int));
	dest_index += commit_node->dev_clusters;
	index += dev_clusters;
	/* copy BIT buffer */
	memcpy(&param->tmp_page_buffer[dest_index], &buf[index], min_dev_blocks * sizeof(unsigned int));
	ubi_leb_unmap(dev->desc, CONFIG_START_BLOCK);
	mt_ftl_leb_write(dev, CONFIG_START_BLOCK, param->tmp_page_buffer, 0, dev->min_io_size);
	/* ToDo: only support 2 page to commit */
	if (dev->commit_page_num > 1) {
		memset(param->tmp_page_buffer, 0, dev->min_io_size);
		param->tmp_page_buffer[0] = MT_MAGIC_NUMBER;
		mt_ftl_leb_write(dev, CONFIG_START_BLOCK, param->tmp_page_buffer, dev->min_io_size, dev->min_io_size);
	}
	ubi_leb_unmap(dev->desc, CONFIG_START_BLOCK + 1);
	return 1;
}

static int mt_ftl_page_all_00(struct mt_ftl_blk *dev, unsigned int *buf)
{
	unsigned int i;

	for (i = 0; i < (dev->min_io_size / 4); i++) {
		if (buf[i] != 0x00000000)
			return 0;
	}

	return 1;
}

static int mt_ftl_page_all_FF(struct mt_ftl_blk *dev, unsigned int *buf)
{
	unsigned int i;

	for (i = 0; i < (dev->min_io_size / 4); i++) {
		if (buf[i] != 0xFFFFFFFF)
			return 0;
	}

	return 1;
}

static int mt_ftl_recover_blk(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	int offset = 0;
	struct mt_ftl_param *param = dev->param;
	const int max_offset_per_block = dev->leb_size - dev->min_io_size;

	/* Recover Replay Blocks */
	for (i = 0; i < REPLAY_BLOCK_NUM; i++)
		ubi_leb_unmap(dev->desc, REPLAY_BLOCK + i);

	/* Recover PMT Blocks, default setting 1 block for recover */
	/* You can modify the i < 1 to 3 for large, but no needed base on 8GB*/
	for (i = 0; i < 1; i++) {
		mt_ftl_leb_remap(dev, PMT_START_BLOCK + i + 3);
		for (offset = 0; offset <= max_offset_per_block; offset += dev->min_io_size) {
			ret = mt_ftl_leb_read(dev, PMT_START_BLOCK + i, param->tmp_page_buffer,
				offset, dev->min_io_size);
			if (ret)
				return MT_FTL_FAIL;
			if (mt_ftl_page_all_00(dev, param->tmp_page_buffer)) {
				mt_ftl_err(dev, "[1]offset = %d, page = %d", offset, offset / dev->min_io_size);
				break;
			}
			ret = mt_ftl_leb_write(dev, PMT_START_BLOCK + i + 3, param->tmp_page_buffer,
				offset, dev->min_io_size);
			if (ret)
				return MT_FTL_FAIL;
		}
		mt_ftl_leb_remap(dev, PMT_START_BLOCK + i);
		for (offset = 0; offset <= max_offset_per_block; offset += dev->min_io_size) {
			ret = mt_ftl_leb_read(dev, PMT_START_BLOCK + i + 3, param->tmp_page_buffer,
				offset, dev->min_io_size);
			if (ret)
				return MT_FTL_FAIL;
			if (mt_ftl_page_all_FF(dev, param->tmp_page_buffer)) {
				mt_ftl_err(dev, "[2]offset = %d, page = %d", offset, offset / dev->min_io_size);
				goto out;
			}
			ret = mt_ftl_leb_write(dev, PMT_START_BLOCK + i, param->tmp_page_buffer,
				offset, dev->min_io_size);
			if (ret)
				return MT_FTL_FAIL;
		}
	}
out:
	if (i < 3)
		i += 1;
	for (; i < dev->pmt_blk_num; i++)
		ubi_leb_unmap(dev->desc, PMT_START_BLOCK + i);
	return ret;
}

int mt_ftl_show_commit_node(struct mt_ftl_blk *dev)
{
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	mt_ftl_err(dev, "magic: %08X", commit_node->magic);
	mt_ftl_err(dev, "version: %08X", commit_node->version);
	mt_ftl_err(dev, "dev_blocks: %08X", commit_node->dev_blocks);
	mt_ftl_err(dev, "dev_clusters: %08X", commit_node->dev_clusters);
	mt_ftl_err(dev, "u4ComprType: %08X", commit_node->u4ComprType);
	mt_ftl_err(dev, "u4BlockDeviceModeFlag: %08X", commit_node->u4BlockDeviceModeFlag);
	mt_ftl_err(dev, "u4NextReplayOffsetIndicator: %08X", commit_node->u4NextReplayOffsetIndicator);
	mt_ftl_err(dev, "u4NextLebPageIndicator: %08X", commit_node->u4NextLebPageIndicator);
	mt_ftl_err(dev, "u4CurrentPMTLebPageIndicator: %08X", commit_node->u4CurrentPMTLebPageIndicator);
	mt_ftl_err(dev, "u4NextFreeLebIndicator: %08X", commit_node->u4NextFreeLebIndicator);
	mt_ftl_err(dev, "u4NextFreePMTLebIndicator: %08X", commit_node->u4NextFreePMTLebIndicator);
	mt_ftl_err(dev, "u4GCReserveLeb: %08X", commit_node->u4GCReserveLeb);
	mt_ftl_err(dev, "u4GCReservePMTLeb: %08X", commit_node->u4GCReservePMTLeb);
	mt_ftl_err(dev, "u4PMTChargeLebIndicator: %08X", commit_node->u4PMTChargeLebIndicator);

	return MT_FTL_SUCCESS;
}

static void mt_ftl_flag_init(struct mt_ftl_blk *dev)
{
	struct mt_ftl_param *param = dev->param;

	/* recorded the last data's pmt for pmt recover*/
	param->pmt_updated_flag = PMT_END;
	/* recorded pmt get free blk for commit*/
	param->get_pmt_blk_flag = 0;
	/* recorded gc data updatePMT for commit*/
	param->gc_dat_cmit_flag = 0;
	/* recorded pmt charge blk for gc pmt */
	param->gc_pmt_charge_blk = -1;
	/* recorded pmt replay for last commit */
	param->replay_pmt_last_cmit = 0;
	/* recorded the last pmt block for recovery */
	param->record_last_pmt_blk = -1;
}

static int mt_ftl_commit_node_init(struct mt_ftl_blk *dev)
{
	int size;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	commit_node->magic = MT_MAGIC_NUMBER;
	commit_node->version = MT_FTL_VERSION;
	/*
	 * commit_node->dev_blocks; calc in mt_ftl_blk_create
	 * commit_node->dev_clusters; calc in mt_ftl_blk_create
	 * commit_node->u4ComprType = FTL_COMPR_NONE;  No need to default
	 */
	commit_node->u4BlockDeviceModeFlag = dev->readonly;
	commit_node->u4NextReplayOffsetIndicator = 0;
	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4NextLebPageIndicator, dev->data_start_blk, 0);
	PMT_LEB_PAGE_INDICATOR_SET_BLOCKPAGE(commit_node->u4CurrentPMTLebPageIndicator, PMT_START_BLOCK, 0);
	commit_node->u4NextFreeLebIndicator = dev->data_start_blk + 1;
	commit_node->u4NextFreePMTLebIndicator = PMT_START_BLOCK + 1;
	commit_node->u4GCReserveLeb = commit_node->dev_blocks - 1;
	commit_node->u4GCReservePMTLeb = dev->data_start_blk - 2;
	commit_node->u4PMTChargeLebIndicator =  dev->data_start_blk - 1;

	size = sizeof(struct mt_ftl_commit_node);
	size += commit_node->dev_clusters * sizeof(unsigned int);
	size += commit_node->dev_blocks * sizeof(unsigned int);
	size += (PMT_CACHE_NUM + 1) * sizeof(unsigned int);
	dev->commit_page_num = roundup(size, dev->min_io_size) / dev->min_io_size;
	mt_ftl_err(dev, "commit_page_num=%d %d", dev->commit_page_num, size);
	/* max commit page is 2 */
	if (dev->commit_page_num > 2)
		return MT_FTL_FAIL;
	return MT_FTL_SUCCESS;
}


int mt_ftl_param_default(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS, i = 0;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;

	ret = mt_ftl_commit_node_init(dev);
	if (ret)
		return ret;

	/* parameters reset */
	mt_ftl_flag_init(dev);
	memset(param->u4DataNum, 0, MAX_DATA_CACHE_NUM * sizeof(unsigned int));
	param->data_cache_num = 0;
	param->replay_blk_index = 0;
	param->replay_pmt_blk_index = 0;
	param->i4CommitOffsetIndicator0 = 0 - dev->commit_page_num * dev->min_io_size;
	param->i4CommitOffsetIndicator1 = 0 - dev->commit_page_num * dev->min_io_size;
	param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
	param->u4NextPageOffsetIndicator = 0;
	memset(param->u4PMTIndicator, 0, commit_node->dev_clusters * sizeof(unsigned int));
	memset(param->u4BIT, 0, commit_node->dev_blocks * sizeof(unsigned int));
	memset(param->i4CurrentPMTClusterInCache, 0xFF, PMT_CACHE_NUM * sizeof(unsigned int));
	memset(param->u4ReadMetaPMTCache, 0xFF, sizeof(unsigned int) << dev->pm_per_io);
	memset(param->u4ReadPMTCache, 0xFF, sizeof(unsigned int) << dev->pm_per_io);
	memset(param->u4MetaPMTCache, 0xFF, PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io);
	memset(param->u4PMTCache, 0xFF, PMT_CACHE_NUM * sizeof(unsigned int) << dev->pm_per_io);

	switch (commit_node->u4ComprType) {
	case FTL_COMPR_LZO:
		param->cc = crypto_alloc_comp("lzo", 0, 0);
		mt_ftl_err(dev, "Using LZO");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZO fail");
			return MT_FTL_FAIL;
		}
		break;
	case FTL_COMPR_LZ4K:
		param->cc = crypto_alloc_comp("lz4k", 0, 0);
		mt_ftl_err(dev, "Using LZ4K");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZ4K fail");
			return MT_FTL_FAIL;
		}
		break;
	default:
		param->cc = NULL;
		mt_ftl_err(dev, "NO COMPR TYPE");
	}
	if (param->cc) {
		ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->cmpr_page_buffer,
						 dev->min_io_size * sizeof(unsigned char),
						 "param->cmpr_page_buffer");
		if (ret)
			return ret;
	}

	/* sync erase all reserved pebs */
	for (i = 0; i < desc->vol->reserved_pebs; i++)
		ubi_leb_erase(desc, i);

	if (!dev->secure_discard) {
		ret = mt_ftl_cache_node_init(dev, NULL);
		if (ret)
			return ret;
	}

	ret = mt_ftl_commit_indicators(dev);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_commit_indicators fail, ret = %d", ret);
		return ret;
	}

	return ret;
}

static int mt_ftl_param_init(struct mt_ftl_blk *dev, int leb, int offset)
{
	int ret = MT_FTL_SUCCESS;
	int i, index, size, dst_size, dst_leb, dst_offset;
	struct mt_ftl_param *param = dev->param;
	struct mt_ftl_commit_node *commit_node = &dev->commit_node;
	unsigned int *buffer = param->general_page_buffer;

	memset(param->u4DataNum, 0, MAX_DATA_CACHE_NUM * sizeof(unsigned int));
	param->data_cache_num = 0;
	param->replay_blk_index = 0;
	param->replay_pmt_blk_index = 0;
	param->i4CurrentReadPMTClusterInCache = 0xFFFFFFFF;
	param->u4NextPageOffsetIndicator = 0;

	/* read first commit page */
	ret = mt_ftl_leb_read(dev, leb, buffer, offset, dev->min_io_size);
	if (ret)
		return MT_FTL_FAIL;
	/* check the version, only check the MSB version */
	if (MT_FTL_MSB_VER(buffer[1]) != MT_FTL_MSB_VER(MT_FTL_VERSION)) {
		mt_ftl_err(dev, "version(0x%x)!=(0x%x)", buffer[1], MT_FTL_VERSION);
		return MT_FTL_FAIL;
	}
	mt_ftl_err(dev, "Version:%d.%d", MT_FTL_MSB_VER(buffer[1]), MT_FTL_LSB_VER(buffer[1]));

	memcpy(commit_node, buffer, sizeof(struct mt_ftl_commit_node));

	switch (commit_node->u4ComprType) {
	case FTL_COMPR_LZO:
		param->cc = crypto_alloc_comp("lzo", 0, 0);
		mt_ftl_err(dev, "Using LZO");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZO fail");
			return MT_FTL_FAIL;
		}
		break;
	case FTL_COMPR_LZ4K:
		param->cc = crypto_alloc_comp("lz4k", 0, 0);
		mt_ftl_err(dev, "Using LZ4K");
		if (IS_ERR(param->cc)) {
			mt_ftl_err(dev, "init LZ4K fail");
			return MT_FTL_FAIL;
		}
		break;
	default:
		param->cc = NULL;
		mt_ftl_err(dev, "NO COMPR TYPE");
	}

	if (param->cc) {
		ret = mt_ftl_alloc_single_buffer((unsigned int **)&param->cmpr_page_buffer,
						 dev->min_io_size * sizeof(unsigned char),
						 "param->cmpr_page_buffer");
		if (ret)
			return ret;
	}

	if (commit_node->u4GCReserveLeb == NAND_DEFAULT_VALUE)
		commit_node->u4GCReserveLeb = commit_node->dev_blocks - 1;
	if (commit_node->u4GCReservePMTLeb == NAND_DEFAULT_VALUE)
		commit_node->u4GCReservePMTLeb = dev->data_start_blk - 2;
	if (commit_node->u4PMTChargeLebIndicator == NAND_DEFAULT_VALUE)
		commit_node->u4PMTChargeLebIndicator = dev->data_start_blk - 1;

	index = sizeof(struct mt_ftl_commit_node) / sizeof(unsigned int);
	memcpy(param->u4PMTIndicator, &buffer[index], commit_node->dev_clusters * sizeof(unsigned int));
	index += commit_node->dev_clusters;
	if (dev->commit_page_num > 1) {
		size = dev->min_io_size - index * sizeof(unsigned int);
		memcpy(param->u4BIT, &buffer[index], size);
		/* read second page */
		ret = mt_ftl_leb_read(dev, leb, buffer, offset + dev->min_io_size, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		/* skip magic number */
		dst_size = commit_node->dev_blocks * sizeof(unsigned int) - size;
		memcpy(&param->u4BIT[size >> 2], &buffer[1], dst_size);
		index = (dst_size >> 2) + 1;
	} else {
		memcpy(param->u4BIT, &buffer[index], commit_node->dev_blocks * sizeof(unsigned int));
		index += commit_node->dev_blocks;
	}
	ret = mt_ftl_cache_node_init(dev, &buffer[index]);
	if (ret)
		return ret;

	if (leb == CONFIG_START_BLOCK) {
		dst_leb = CONFIG_START_BLOCK + 1;
		param->i4CommitOffsetIndicator0 = offset;
		dst_offset = param->i4CommitOffsetIndicator1 = mt_ftl_get_last_config_offset(dev, dst_leb);
		if (dst_offset >= dev->leb_size) {
			mt_ftl_err(dev, "unmap CONFIG_START_BLOCK + 1");
			ubi_leb_unmap(dev->desc, dst_leb);
			dst_offset = param->i4CommitOffsetIndicator1 = 0;
		}
		mt_ftl_err(dev, "[0]Sync to backup CONFIG block %d %d", offset, dst_offset);
	} else {
		dst_leb = CONFIG_START_BLOCK;
		param->i4CommitOffsetIndicator1 = offset;
		dst_offset = param->i4CommitOffsetIndicator0 = mt_ftl_get_last_config_offset(dev, dst_leb);
		if (dst_offset >= dev->leb_size) {
			mt_ftl_err(dev, "unmap CONFIG_START_BLOCK");
			ubi_leb_unmap(dev->desc, dst_leb);
			dst_offset = param->i4CommitOffsetIndicator0 = 0;
		}
		mt_ftl_err(dev, "[1]Sync to backup CONFIG block %d %d", offset, dst_offset);
	}
	for (i = 0; i < dev->commit_page_num; i++) {
		size = i * dev->min_io_size;
		mt_ftl_leb_read(dev, leb, buffer, offset + size, dev->min_io_size);
		mt_ftl_leb_write(dev, dst_leb, buffer, dst_offset + size, dev->min_io_size);
	}

	mt_ftl_show_commit_node(dev);

	/* Replay */
	ret = mt_ftl_replay(dev);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_replay fail, ret = %d", ret);
		return ret;
	}
	if (dev->readonly)
		commit_node->u4BlockDeviceModeFlag = 1; /* reset Block device flag to read only*/

	mt_ftl_show_commit_node(dev);

	return ret;
}

int mt_ftl_create(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
	int leb = 0, offset = 0, tmp;
	struct ubi_volume_desc *desc = dev->desc;
	struct mt_ftl_param *param = dev->param;

#ifdef MT_FTL_PROFILE
	memset(profile_time, 0, sizeof(profile_time));
#endif
	mt_ftl_flag_init(dev);

	/* init commit node, must be here */
	ret = mt_ftl_commit_node_init(dev);
	if (ret)
		return ret;

	/* Allocate buffers for FTL usage */
	ret = mt_ftl_alloc_buffers(dev);
	if (ret)
		return ret;
	/* Allocate wbuf for FTL */
	ret = mt_ftl_wbuf_alloc(dev);
	if (ret)
		return ret;

	/* Check the mapping of CONFIG block */
	ret = ubi_is_mapped(desc, CONFIG_START_BLOCK);
	if (ret == 0) {
		ret = ubi_is_mapped(desc, CONFIG_START_BLOCK + 1);
		if (ret)
			leb = CONFIG_START_BLOCK + 1;
		else {
			mt_ftl_err(dev, "leb%d/leb%d are both unmapped", CONFIG_START_BLOCK,
				   CONFIG_START_BLOCK + 1);
			leb = CONFIG_START_BLOCK;
		}
	} else
		leb = CONFIG_START_BLOCK;

	/* Get lastest config page */
	offset = mt_ftl_get_last_config_offset(dev, leb);
	if (offset == 0) {
		mt_ftl_err(dev, "Config blocks are empty");
		ret = mt_ftl_param_default(dev);
		if (ret)
			mt_ftl_err(dev, "mt_ftl_param_default fail, ret = %d", ret);
		return ret;
	}

	offset -= dev->commit_page_num * dev->min_io_size;
	if (offset <= 0 && leb == CONFIG_START_BLOCK) {
		ret = mt_ftl_leb_read(dev, leb, param->general_page_buffer, 0, dev->min_io_size);
		if (ret)
			return MT_FTL_FAIL;
		/* Check if system.img/usrdata.img is just reloaded */
		ret = mt_ftl_reload_config_blk(dev, param->general_page_buffer);
		if (ret) {
			mt_ftl_err(dev, "image reloaded start!");
			ret = mt_ftl_recover_blk(dev);
			if (ret) {
				mt_ftl_err(dev, "recover block fail");
				ret = mt_ftl_param_default(dev);
				return ret;
			}
			offset = 0;
		}
	}
	/* check config block */
	if (dev->commit_page_num > 1) {
		tmp = offset / dev->min_io_size;
		tmp %= dev->commit_page_num;
		if (tmp) {
			mt_ftl_err(dev, "power cut at commit indicators(leb:%d offset:%d)", leb, offset);
			offset -= tmp * dev->min_io_size;
			tmp = offset + dev->commit_page_num * dev->min_io_size;
			mt_ftl_leb_recover(dev, leb, tmp, 0);
		}
	}
	/* Init param */
	ret = mt_ftl_param_init(dev, leb, offset);
	if (ret) {
		mt_ftl_err(dev, "mt_ftl_param_init fail, ret = %d", ret);
		return ret;
	}

	return ret;
}

/* TODO: Tracking remove process to make sure mt_ftl_remove can be called during shut down */
int mt_ftl_remove(struct mt_ftl_blk *dev)
{
	int ret = MT_FTL_SUCCESS;
#ifdef MT_FTL_PROFILE
	int i = 0;

	for (i = 0; i < MT_FTL_PROFILE_TOTAL_PROFILE_NUM; i++)
		mt_ftl_err(dev, "%s = %lu ms", mtk_ftl_profile_message[i], profile_time[i] / 1000);
#endif				/* PROFILE */
	mt_ftl_err(dev, "Enter");
	mt_ftl_commit(dev);

	if (dev->param->cc)
		crypto_free_comp(dev->param->cc);

	mt_ftl_wbuf_free(dev);
	mt_ftl_err(dev, "mt_ftl_commit done");

	return ret;
}
