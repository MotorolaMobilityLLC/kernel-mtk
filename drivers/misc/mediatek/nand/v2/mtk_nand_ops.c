/******************************************************************************
* mtk_nand_chip.c - MTK NAND Flash Device Driver for FTL partition
 *
* Copyright 2016 MediaTek Co.,Ltd.
 *
* DESCRIPTION:
*	This file provid the other drivers nand relative functions
*	1) GPL Related Function;
*	2) BMT Realtaed Function;
*	3) MTK Nand Driver Related Functnions
*	4) API for nand Wrapper
*
* modification history
* ----------------------------------------
* v1.0, 2 Aug 2016, mtk
* ----------------------------------------
******************************************************************************/

#include "mtk_nand_ops.h"
#include "bmt.h"

static struct mtk_nand_data_info *data_info;



/*********	MTK Nand Driver Related Functnions *********/

/* mtk_isbad_block
 * Mark specific block as bad block,and update bad block list and bad block table.
 * @block: block address to markbad
 */
bool mtk_isbad_block(unsigned int block)
{
	struct mtk_nand_chip_bbt_info *chip_bbt;
	unsigned int i;

	chip_bbt = &data_info->chip_bbt;
	for (i = 0; i < chip_bbt->bad_block_num; i++) {
		if (block == chip_bbt->bad_block_table[i]) {
			nand_debug("Check block:0x%x is bad", block);
			return TRUE;
		}
	}

	/* nand_debug("Check block:0x%x is Good", block); */
	return FALSE;
}

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
void mtk_nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	/* Release the controller and the chip */
	spin_lock(&chip->controller->lock);
	chip->controller->active = NULL;
	if (chip->state != FL_READY && chip->state != FL_PM_SUSPENDED)
		nand_disable_clock();

	chip->state = FL_READY;
	wake_up(&chip->controller->wq);
	spin_unlock(&chip->controller->lock);
}


static int mtk_nand_read_pages(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page,
		unsigned int offset, unsigned int size)
{
	struct mtd_info *mtd = data_info->mtd;
	struct nand_chip *chip = mtd->priv;
	struct mtd_ecc_stats stats;
	unsigned char *fdm_buf;
#ifdef MTK_FORCE_READ_FULL_PAGE
	unsigned char *tmp_buf;
#endif
	unsigned int page_addr, page_size, oob_size;
	unsigned int sect_num, start_sect;
	unsigned int col_addr, sect_read;
	int ret = 0;


	nand_debug("mtk_nand_chip_read_page, block:0x%x page:0x%x offset:0x%x size:0x%x",
		block, page, offset, size);

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_READING);

	stats = mtd->ecc_stats;
	chip->select_chip(mtd, 0);

	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		&& (block >= info->data_block_num))
		page_addr = (block+data_info->bmt.start_block)*info->data_page_num+page*MTK_TLC_DIV;

	else
		page_addr = (block+data_info->bmt.start_block)*info->data_page_num+page;

	/* For log area access */
	if (block < info->data_block_num) {
		devinfo.tlcControl.slcopmodeEn = FALSE;
		page_size = info->data_page_size;
		oob_size = info->log_oob_size;
	} else {
		/* nand_debug("Read SLC mode"); */
		devinfo.tlcControl.slcopmodeEn = TRUE;
		page_size = info->log_page_size;
		oob_size = info->log_oob_size;
	}

	fdm_buf = kmalloc(1024, GFP_KERNEL);
	if (fdm_buf == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	if (size < page_size) {

		/* Sector read case */
		sect_num = page_size/(1<<info->sector_size_shift);
		start_sect  = offset/(1<<info->sector_size_shift);
		col_addr = start_sect*((1<<info->sector_size_shift)+mtd->oobsize/sect_num);
		sect_read = size/(1<<info->sector_size_shift);
		/* nand_debug("Sector read col_addr:0x%x sect_read:%d, sect_num:%d, start_sect:%d", */
			/* col_addr, sect_read, sect_num, start_sect); */
#ifdef MTK_FORCE_READ_FULL_PAGE
		tmp_buf = kmalloc(page_size, GFP_KERNEL);
		if (tmp_buf == NULL) {
			ret = -ENOMEM;
			goto exit;
		}
		ret = mtk_nand_exec_read_page(mtd, page_addr, page_size, tmp_buf, fdm_buf);
		memcpy(data_buffer, tmp_buf+offset, size);
		memcpy(oob_buffer, fdm_buf+start_sect*host->hw->nand_fdm_size,
			sect_read*host->hw->nand_fdm_size);
		kfree(tmp_buf);
#else
		ret = mtk_nand_exec_read_sector_single(mtd, page_addr, col_addr, page_size,
				data_buffer, fdm_buf, sect_read);
		memcpy(oob_buffer, fdm_buf, sect_read*host->hw->nand_fdm_size);
#endif
	} else {
		ret = mtk_nand_exec_read_page(mtd, page_addr, page_size, data_buffer, fdm_buf);
		memcpy(oob_buffer, fdm_buf, oob_size);
	}

	if (ret != 1)
		ret = -ENANDREAD;
	else
		ret = 0;
#if 0
	else
		ret =  mtd->ecc_stats.corrected - stats.corrected ? -ENANDFLIPS : 0;
#endif

exit:
	kfree(fdm_buf);
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);

	return ret;

}
/**
 * mtk_nand_write_pages -  NAND write with ECC
 * @ops: mtk_nand_chip_operation structure
 * @lecountn: number of pages to write
 *
 * NAND write with ECC.
 */
static int mtk_nand_write_pages(struct mtk_nand_chip_operation *ops0,
		struct mtk_nand_chip_operation *ops1)
{
	struct mtd_info *mtd = data_info->mtd;
	struct mtk_nand_chip_info *info = &data_info->chip_info;
	unsigned char *fdm_buf = NULL;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	unsigned char *temp_page_buf = NULL;
	unsigned char *temp_fdm_buf = NULL;
#endif
	unsigned int page_addr0, page_addr1;
	unsigned int page_size, oob_size, sect_num;
	int ret = 0;

	if (ops0 == NULL) {
		nand_err("ops0 is NULL");
		return -EIO;
	}

	if (ops1 != NULL) {
		/* Check both in data or log area */
		if (((ops0->block < info->data_block_num)
				&& (ops1->block >= info->data_block_num))
			|| ((ops0->block >= info->data_block_num)
				&& (ops1->block < info->data_block_num))) {
			nand_err("do not in same area ops0->block:0x%x ops1->block:0x%x ",
				ops0->block, ops1->block);
			return -EIO;
		}
		if (mtk_isbad_block(ops1->block))
			page_addr1 = 0;
		else {
			if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
				&& (ops1->block >= info->data_block_num)) {
				page_addr1 = (ops1->block+data_info->bmt.start_block)*info->data_page_num
						+ ops1->page*MTK_TLC_DIV;
			} else {
				page_addr1 = (ops1->block+data_info->bmt.start_block)*info->data_page_num
					+ ops1->page;
			}
		}
	} else {
		page_addr1 = 0;
	}

	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		&& (ops0->block >= info->data_block_num))
		page_addr0 = (ops0->block+data_info->bmt.start_block)*info->data_page_num
				+ ops0->page*MTK_TLC_DIV;
	else
		page_addr0 = (ops0->block+data_info->bmt.start_block)*info->data_page_num
				+ ops0->page;

	nand_debug("page_addr0= 0x%x page_addr1=0x%x",
		page_addr0, page_addr1);
#if 0
	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_WRITING);
	/* Select the NAND device */
	chip->select_chip(mtd, 0);
#endif
	/* For log area access */
	if (ops0->block < info->data_block_num) {
		page_size = info->data_page_size;
		devinfo.tlcControl.slcopmodeEn = FALSE;
		oob_size = info->data_oob_size;
	} else {
		nand_debug("write SLC mode");
		page_size = info->log_page_size;
		devinfo.tlcControl.slcopmodeEn = TRUE;
		oob_size = info->log_oob_size;
	}

	sect_num = page_size/(1<<info->sector_size_shift);

	fdm_buf = kmalloc(1024, GFP_KERNEL);
	if (fdm_buf == NULL) {
		ret = -ENOMEM;
		goto exit;
	}
	memset(fdm_buf, 0xff, 1024);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		if ((devinfo.tlcControl.normaltlc) && (devinfo.tlcControl.pPlaneEn)) {
			page_size /= 2;
			sect_num /= 2;
			tlc_snd_phyplane = FALSE;
			tlc_lg_left_plane = TRUE;
			memcpy(fdm_buf, ops0->oob_buffer, oob_size);
			temp_page_buf = ops0->data_buffer;
			temp_fdm_buf = fdm_buf;
			ret = mtk_nand_exec_write_page_hw(mtd, page_addr0,
						page_size, temp_page_buf, fdm_buf);
			if (ret != 0) {
				nand_err("write failed for page_addr0:0x%x", page_addr0);
				goto exit;
			}

			tlc_lg_left_plane = FALSE;
			temp_page_buf += page_size;
			temp_fdm_buf += (sect_num * host->hw->nand_fdm_size);

			ret = mtk_nand_exec_write_page_hw(mtd, page_addr0,
						page_size, temp_page_buf, fdm_buf);
			if (ret != 0) {
				nand_err("write failed for page_addr0:0x%x", page_addr0);
				goto exit;
			}

			if ((devinfo.advancedmode & MULTI_PLANE)  && page_addr1) {
				nand_debug("write Multi_plane mode page_addr0:0x%x page_addr1:0x%x",
					page_addr0, page_addr1);
				tlc_snd_phyplane = TRUE;
				tlc_lg_left_plane = TRUE;
				memcpy(fdm_buf, ops1->oob_buffer, oob_size);

				temp_page_buf = ops1->data_buffer;
				temp_fdm_buf = fdm_buf;

				ret = mtk_nand_exec_write_page_hw(mtd, page_addr1, page_size,
						temp_page_buf, temp_fdm_buf);
				if (ret != 0) {
					nand_err("write failed for page_addr1:0x%x", page_addr1);
					goto exit;
				}

				tlc_lg_left_plane = FALSE;
				temp_page_buf += page_size;
				temp_fdm_buf += (sect_num * host->hw->nand_fdm_size);

				ret  = mtk_nand_exec_write_page_hw(mtd, page_addr1, page_size,
						temp_page_buf, temp_fdm_buf);
				if (ret != 0) {
					nand_err("write failed for page_addr1:0x%x", page_addr1);
					goto exit;
				}
				tlc_snd_phyplane = FALSE;
			}
		} else {
			tlc_snd_phyplane = FALSE;
			memcpy(fdm_buf, ops0->oob_buffer, oob_size);
			temp_page_buf = ops0->data_buffer;
			temp_fdm_buf = fdm_buf;
			ret = mtk_nand_exec_write_page_hw(mtd, page_addr0,
					page_size, temp_page_buf, fdm_buf);
			if (ret != 0) {
				nand_err("write failed for page_addr0:0x%x", page_addr0);
				goto exit;
			}

			if ((devinfo.tlcControl.normaltlc)
					&& ((devinfo.advancedmode & MULTI_PLANE) && page_addr1)) {
				nand_debug("write Multi_plane mode page_addr0:0x%x page_addr1:0x%x",
					page_addr0, page_addr1);
				tlc_snd_phyplane = TRUE;
				memcpy(fdm_buf, ops1->oob_buffer, oob_size);

				temp_page_buf = ops1->data_buffer;
				temp_fdm_buf = fdm_buf;

				ret = mtk_nand_exec_write_page_hw(mtd, page_addr1, page_size,
						temp_page_buf, temp_fdm_buf);
				if (ret != 0) {
					nand_err("write failed for page_addr1:0x%x", page_addr1);
					goto exit;
				}
				tlc_snd_phyplane = FALSE;
			}
		}
	} else
#endif
	{
		tlc_snd_phyplane = FALSE;
		memcpy(fdm_buf, ops0->oob_buffer, oob_size);

		ret = mtk_nand_exec_write_page_hw(mtd, page_addr0,
				page_size, ops0->data_buffer, fdm_buf);
		if (ret != 0) {
			nand_err("write failed for page_addr0:0x%x", page_addr0);
			goto exit;
		}

		if ((devinfo.advancedmode & MULTI_PLANE)  && page_addr1) {
			nand_debug("write Multi_plane mode page_addr0:0x%x page_addr1:0x%x",
				page_addr0, page_addr1);
			tlc_snd_phyplane = TRUE;
			memcpy(fdm_buf, ops1->oob_buffer, oob_size);
			ret = mtk_nand_exec_write_page_hw(mtd, page_addr1,
						page_size, ops1->data_buffer, fdm_buf);
			tlc_snd_phyplane = FALSE;
			if (ret != 0) {
				nand_err("write failed for page_addr1:0x%x", page_addr1);
				goto exit;
			}
		}

	}

exit:
	kfree(fdm_buf);

#if 0
	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);
#endif
	/* Return more or less happy */
	return ret;
}

/**
 * nand_erase_blocks - [INTERN] erase block(s)
 * @ops: erase instruction
 * @count: block count to be ease
 *
 * Erase one ore more blocks.
 */
static int mtk_nand_erase_blocks(struct mtk_nand_chip_operation *ops0,
			struct mtk_nand_chip_operation *ops1)
{
	struct mtd_info *mtd = data_info->mtd;
	struct nand_chip *chip = mtd->priv;
	struct mtk_nand_chip_info *info = &data_info->chip_info;
	unsigned int page_addr0, page_addr1;
	int status;
	int ret = 0;

	nand_debug("block0= 0x%x", ops0->block);

	if (ops0 == NULL) {
		nand_err("ops0 is NULL");
		return -EIO;
	}

	if (ops1 != NULL) {
		/* Check both in data or log area */
		if (((ops0->block < info->data_block_num)
				&& (ops1->block >= info->data_block_num))
			|| ((ops0->block >= info->data_block_num)
				&& (ops1->block < info->data_block_num))) {
			nand_err("do not in same area ops0->block:0x%x ops1->block:0x%x ",
				ops0->block, ops1->block);
			return -EIO;
		}
		if (mtk_isbad_block(ops1->block))
			page_addr1 = 0;
		else
			page_addr1 = (ops1->block+data_info->bmt.start_block)*info->data_page_num;
	} else {
		page_addr1 = 0;
	}

	page_addr0 = (ops0->block+data_info->bmt.start_block)*info->data_page_num;

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_ERASING);

	if (ops0->block < info->data_block_num) {
		devinfo.tlcControl.slcopmodeEn = FALSE;
	} else {
		nand_debug("erase SLC mode");
		devinfo.tlcControl.slcopmodeEn = TRUE;
	}

	chip->select_chip(mtd, 0);

	status = mtk_chip_erase_blocks(mtd, page_addr0, page_addr1);

	/* See if block erase succeeded */
	if (status & NAND_STATUS_FAIL) {
		nand_debug("%s: failed erase, page_addr0 0x%x\n",
			__func__, page_addr0);
		ret = MTD_ERASE_FAILED;
	}

	nand_debug("%s: done start page_addr0= 0x%x page_addr1= 0x%x",
			__func__, page_addr0, page_addr1);

	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);

	/* Return more or less happy */
	return ret;
}



static inline int get_list_item_index(
	struct list_node *head, struct list_node *cur)
{
	int index = 0;
	struct list_node *item;

	item = head->next;
	while ((item != cur) && (item != NULL)) {
		item = item->next;
		index++;
	}

	if (item != cur) {
		nand_err("check no node cur:%p", cur);
		return 0;
	}

	return index;
}

static inline struct list_node *get_list_item_prev(
	struct list_node *head, struct list_node *cur)
{
	struct list_node *item;

	item = head;
	while ((item->next != cur) && (item->next != NULL))
		item = item->next;

	if (item->next != cur) {
		nand_err("check no node cur:%p", cur);
		return NULL;
	}

	return item;
}

static inline struct list_node *get_list_tail(
	struct list_node *head)
{
	struct list_node *item;

	item = head;
	while (item->next != NULL)
		item = item->next;

	return item;
}

static inline struct nand_work *get_list_work(
	struct list_node *node)
{
	return containerof(node, struct nand_work, list);
}

static struct list_node *seek_list_item(
	struct list_node *cur, unsigned int offset)
{
	struct list_node *item;
	int i;

	i = 0;
	item = cur;
	while (i < offset) {
		item = item->next;
		i++;
	}
	return item;
}

static inline void lock_list(
	struct worklist_ctrl *list_ctrl)
{
	spin_lock(&list_ctrl->list_lock);
}

static inline void unlock_list(
	struct worklist_ctrl *list_ctrl)
{
	spin_unlock(&list_ctrl->list_lock);
}

static inline void add_list_node(
	struct worklist_ctrl *list_ctrl, struct list_node *node)
{
	struct list_node *tail;

	lock_list(list_ctrl);
	tail = get_list_tail(&list_ctrl->head);
	tail->next = node;
	list_ctrl->total_num++;
	nand_debug("add node:%p", node);
	unlock_list(list_ctrl);
}

static inline int get_list_work_cnt(
	struct worklist_ctrl *list_ctrl)
{
	int cnt = 0;

	lock_list(list_ctrl);
	cnt = list_ctrl->total_num;
	unlock_list(list_ctrl);

	return cnt;
}

static bool is_in_slc_block_range(
	struct mtk_nand_chip_info *info,
	struct list_node *cur)
{
	struct nand_work *work;

	if (cur == NULL) {
		nand_err("cur is NULL");
		return false;
	}
	work = get_list_work(cur);
	return (work->ops.block >= info->data_block_num);
}

static bool is_valid_mlc_multi_work(
	struct list_node *cur)
{
	struct nand_work *work1, *work2;
	struct mtk_nand_chip_operation *ops1, *ops2;

	if (cur->next == NULL) {
		nand_err("item2 NULL!!\n");
		return false;
	}

	work1 = get_list_work(cur);
	work2 = get_list_work(cur->next);
	ops1 = &work1->ops;
	ops2 = &work2->ops;

	if (((ops1->block%2) ^ (ops2->block%2)) == 0) {
		nand_err("continues write's block on same plan\n"
				"block1:%d, block2:%d",
				ops1->block, ops2->block);
		return false;
	}

	return true;
}

static void call_multi_work_callback(
	struct list_node *cur, int count, int status)
{
	int i;
	struct list_node *item;
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops;

	item = cur;
	for (i = 0; i < count; i++)  {
		if (item == NULL) {
			nand_err("NULL item");
			return;
		}
		work = get_list_work(item);
		ops = &work->ops;
		nand_debug("callback i:%d, block:%d page:%d",
			i, ops->block, ops->page);
		ops->callback(ops->info, ops->data_buffer,
			ops->oob_buffer, ops->block,
			ops->page, status, ops->userdata);
		if (i >= count)
			break;
		item = item->next;
	}
}

static void call_tlc_page_group_callback(
	struct list_node *base_node, int start, int end,
	bool multi_plane, int status)
{
	struct list_node *item;
	int i;
	int count = multi_plane ? 2 : 1;

	item = seek_list_item(base_node, start * count);
	for (i = 0; i < end - start + 1; i++) {
		call_multi_work_callback(
			item, count, status);

		if (i >= end - start)
			break;
		if (count == 1)
			item = item->next;
		else
			item = item->next->next;
	}
}

static struct list_node *free_multi_work(
	struct worklist_ctrl *list_ctrl,
	struct list_node *head,
	struct list_node *start_node, int count)
{
	int i;
	struct list_node *item, *prev, *next;
	struct nand_work *work;

	item = start_node;

	lock_list(list_ctrl);
	prev = get_list_item_prev(head, start_node);
	if (prev == NULL) {
		nand_err("prev is NULL!!");
		return NULL;
	}
	for (i = 0; i < count; i++)  {
		if (item == NULL) {
			nand_err("NULL item");
			return NULL;
		}
		work = get_list_work(item);

		nand_debug("i:%d, page:%d",
			i, work->ops.page);

		next = item->next;
		list_ctrl->total_num--;
		kfree(work);
		item = next;
	}
	prev->next = item;
	unlock_list(list_ctrl);
	return item;
}

static struct list_node *free_multi_write_work(
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node, int count)
{
	struct list_node *head, *next;

	head = &list_ctrl->head;
	next = free_multi_work(list_ctrl,
			head, start_node, count);
	return next;
}

static struct list_node *free_multi_erase_work(
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node, int count)
{
	struct list_node *head, *next;

	head = &list_ctrl->head;
	next = free_multi_work(list_ctrl,
			head, start_node, count);
	return next;
}
static struct list_node *free_tlc_page_group_work(
	struct worklist_ctrl *list_ctrl,
	struct list_node *base_node, int start,
	int end, bool multi_plane)
{
	struct list_node *start_node;
	int i;
	int count = multi_plane ? 2 : 1;

	start_node = seek_list_item(base_node, start * count);
	for (i = 0; i < end - start + 1; i++) {
		start_node = free_multi_write_work(
			list_ctrl, start_node, count);
	}
	return start_node;
}


static int do_multi_work_erase(
	struct list_node *cur, int count)
{
	int i, status = 0;
	struct list_node *item;
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops0, *ops1;

	ops0 = NULL;
	ops1 = NULL;

	item = cur;
	for (i = 0; i < count; i++)  {
		if (item == NULL) {
			nand_err("i:%d, NULL item\n", i);
			return -ENANDWRITE;
		}
		work = get_list_work(item);
		if (i == 0)
			ops0 = &work->ops;
		else if (i == 1)
			ops1 = &work->ops;

		if (i+1 >= count)
			break;
		item = item->next;
	}

	status = mtk_nand_erase_blocks(ops0, ops1);

	return status;
}

static int do_multi_work_write(
	struct list_node *cur, int count)
{
	int i, status = 0;
	struct list_node *item;
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops0, *ops1;

	ops0 = NULL;
	ops1 = NULL;
	item = cur;
	for (i = 0; i < count; i++)  {
		if (item == NULL) {
			nand_err("NULL item");
			return -ENANDWRITE;
		}
		work = get_list_work(item);
		if (i == 0)
			ops0 = &work->ops;
		else if (i == 1)
			ops1 = &work->ops;

		if (i+1 >= count)
			break;
		item = item->next;
	}

	status = mtk_nand_write_pages(ops0, ops1);

	return status;
}

static struct list_node *do_single_page_write(
	struct worklist_ctrl *list_ctrl,
	struct list_node *cur)
{
	int status;
	struct mtd_info *mtd = data_info->mtd;
	struct nand_chip *chip = mtd->priv;

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_WRITING);
	/* Select the NAND device */
	chip->select_chip(mtd, 0);

	status = do_multi_work_write(cur, 1);

	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);

	call_multi_work_callback(cur, 1, status);

	return free_multi_write_work(
			list_ctrl, cur, 1);
}

static struct list_node *do_mlc_multi_plane_write(
	struct mtk_nand_chip_info *info,
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node,
	int count)
{
	struct list_node *item;
	int status = 0;
	struct mtd_info *mtd = data_info->mtd;
	struct nand_chip *chip = mtd->priv;

	nand_debug("do_mlc_multi_plane_write enter");
	if (count != info->max_keep_pages) {
		nand_err("error:count:%d max:%d",
			count, info->max_keep_pages);
		return start_node;
	}

	if (start_node == NULL) {
		nand_err("error:start node is NULL!");
		return NULL;
	}

	item = start_node;

	if (!is_valid_mlc_multi_work(item))
		return item;

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_WRITING);
	/* Select the NAND device */
	chip->select_chip(mtd, 0);

	status = do_multi_work_write(item, count);
	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);

	call_multi_work_callback(item,
			count, status);

	return free_multi_write_work(
			list_ctrl, item, count);
}

static int do_tlc_page_group_write(
	struct list_node *cur, int start, int end,
	bool multi_plane, enum NFI_TLC_PG_CYCLE program_cycle)
{
	struct list_node *item;
	int status = 0, i;
	int count = multi_plane ? 2 : 1;
	struct mtd_info *mtd = data_info->mtd;
	struct nand_chip *chip = mtd->priv;


	item = seek_list_item(cur, start * count);

	nand_debug("start:%d, end:%d, item:%p",
		start, end, item);

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_WRITING);
	/* Select the NAND device */
	chip->select_chip(mtd, 0);

	devinfo.tlcControl.slcopmodeEn = FALSE;
	tlc_program_cycle = program_cycle;

	for (i = 0; i < end - start + 1; i++) {
		status = do_multi_work_write(item, count);
		if (i >= end - start)
			break;
		if (count == 1)
			item = item->next;
		else
			item = item->next->next;
	}

	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	mtk_nand_release_device(mtd);

	return status;
}

static struct list_node *do_tlc_write(
	struct mtk_nand_chip_info *info,
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node,
	int count)
{
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops;
	struct list_node *item;
	int status;
	bool multi_plan = false;

	if (count != info->max_keep_pages) {
		nand_err("error:count:%d max:%d",
			count, info->max_keep_pages);
		return start_node;
	}

	if (start_node == NULL) {
		nand_err("error:start node is NULL!");
		return NULL;
	}

	item = start_node;

	multi_plan = (info->option & MTK_NAND_PLANE_MODE_SUPPORT);

	work = get_list_work(item);
	ops = &work->ops;

	nand_debug("page num:%d", ops->page);

	if ((ops->page % MTK_TLC_DIV) != 0) {
		nand_err("Not TLC(3 pages) alsigned ops->page:0x%x",
			ops->page);
		return NULL;
	}


	if (ops->page == 0) {
		status = do_tlc_page_group_write(
				item, 0, 2, multi_plan, PROGRAM_1ST_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
		status = do_tlc_page_group_write(
				item, 3, 5, multi_plan, PROGRAM_1ST_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
		status = do_tlc_page_group_write(
				item, 0, 2, multi_plan, PROGRAM_2ND_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
	}


	if ((ops->page/3 + 2) < (info->data_page_num/3)) {
		status = do_tlc_page_group_write(
				item, 6, 8, multi_plan, PROGRAM_1ST_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
	}

	if ((ops->page/3 + 1) < (info->data_page_num/3)) {
		status = do_tlc_page_group_write(
				item, 3, 5, multi_plan, PROGRAM_2ND_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
	}

	status = do_tlc_page_group_write(
			item, 0, 2, multi_plan, PROGRAM_3RD_CYCLE);
	if (status)
		nand_err("Write failed ops->page:0x%x", ops->page);

	if (ops->page == (info->data_page_num - 9)) {
		status = do_tlc_page_group_write(
				item, 6, 8, multi_plan, PROGRAM_2ND_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}

		status = do_tlc_page_group_write(
				item, 3, 5, multi_plan, PROGRAM_3RD_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}

		status = do_tlc_page_group_write(
				item, 6, 8, multi_plan, PROGRAM_3RD_CYCLE);
		if (status) {
			nand_err("Write failed ops->page:0x%x", ops->page);
			goto write_failed;
		}
	}

write_failed:

	call_tlc_page_group_callback(
		item, 0, 2, multi_plan, status);

	if (multi_plan)
		work = get_list_work(seek_list_item(item, 8 * 2));
	else
		work = get_list_work(seek_list_item(item, 8));

	item = free_tlc_page_group_work(list_ctrl, item,
						0, 2, multi_plan);

	ops = &work->ops;

	if (ops->page == (info->data_page_num - 1)) {
		/*3~5*/
		call_tlc_page_group_callback(item, 0, 2,
						multi_plan, status);
		/*6~8*/
		call_tlc_page_group_callback(item, 3, 5,
						multi_plan, status);
		/*3~5*/
		item = free_tlc_page_group_work(list_ctrl, item,
						0, 2, multi_plan);
		/*6~8*/
		item = free_tlc_page_group_work(list_ctrl, item,
						0, 2, multi_plan);
	}

	return item;
}

static struct list_node *complete_erase_count(
	struct mtk_nand_chip_info *info,  struct list_node *head,
	struct list_node *cur, unsigned int *count)
{
	struct nand_work *work;
	unsigned int total;

	if (cur == NULL) {
		nand_err("cur is NULL!!!");
		return NULL;
	}

	work = get_list_work(cur);
	total = get_list_item_index(head, cur) + 1;
	/*process plane_num one time*/
	if (total == info->plane_num) {
		*count = info->plane_num;
		return head->next;
	}

	/*not enough plane_num, process one by one*/
	if (!work->ops.more) {
		nand_err("more=0 but not enough for plane_num!!!");
		*count = 1;
		return head->next;
	}

	*count = 0;
	return NULL;
}

static struct list_node *complete_write_count(
	struct mtk_nand_chip_info *info, struct list_node *head,
	struct list_node *cur, unsigned int *count)
{
	struct nand_work *work;
	unsigned int total;

	if (info == NULL)
		nand_err("info is NULL!!!");
	if (head == NULL)
		nand_err("head is NULL!!!");
	if (cur == NULL)
		nand_err("cur is NULL!!!");
	if (count == NULL)
		nand_err("count is NULL!!!");

	if (info->types == NAND_FLASH_SLC ||
		is_in_slc_block_range(info, cur)) {
		*count = 1;
		return cur;
	}

	work = get_list_work(cur);
	total = get_list_item_index(head, cur) + 1;

	/*process max_keep_pages one time*/
	if (total >= info->max_keep_pages) {
		*count = info->max_keep_pages;
		nand_debug("total==max:%d", total);
		return head->next;
	}

	/*not enough max keep pages, process one by one*/
	if (!work->ops.more &&
			info->types == NAND_FLASH_MLC) {
		nand_debug("more=0 but not enough for max_keep_pages!!!");
		*count = 1;
		return head->next;
	}

	*count = 0;
	return NULL;
}

static int init_list_ctrl(struct worklist_ctrl *list_ctrl,
			is_data_ready is_ready_func,
			process_list_data process_data_func)
{
	mutex_init(&list_ctrl->sync_lock);
	spin_lock_init(&list_ctrl->list_lock);
	list_ctrl->total_num = 0;
	list_ctrl->head.next = NULL;
	list_ctrl->is_ready_func = is_ready_func;
	list_ctrl->process_data_func = process_data_func;

	return 0;
}

static inline bool block_num_is_valid(
	struct mtk_nand_chip_info *info,
	unsigned int block)
{
	return (block >= 0 && block <
		(info->data_block_num + info->log_block_num));
}

static inline bool block_page_num_is_valid(
	struct mtk_nand_chip_info *info,
	unsigned int block, unsigned int page)
{
	if (!block_num_is_valid(info, block))
		return false;

	if (page < 0)
		return false;

	if ((block < info->data_block_num &&
		page < info->data_page_num) ||
		(block >= info->data_block_num &&
		page < info->log_page_num))
		return true;
	else
		return false;
}


static struct list_node *mtk_nand_do_erase(
	struct mtk_nand_chip_info *info,
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node,
	int count)
{
	struct list_node *node;
	int i, status, op_cnt;

	i = 0;
	node = start_node;
	if (node == NULL) {
		nand_err("head next null");
		return NULL;
	}
	while ((i < count) && node) {
		op_cnt = ((count-i) > info->plane_num) ? info->plane_num : (count-i);
		nand_debug(" i:%d, op_cnt:%d", i, op_cnt);
		status = do_multi_work_erase(node, op_cnt);
		call_multi_work_callback(node, op_cnt, status);
		node = free_multi_erase_work(list_ctrl, node, op_cnt);
		i += op_cnt;
	}

	nand_debug("mtk_nand_do_erase done\n");

	return node;
}

static struct list_node *mtk_nand_do_write(
	struct mtk_nand_chip_info *info,
	struct worklist_ctrl *list_ctrl,
	struct list_node *start_node,
	int count)
{
	if (count == 1)
		return do_single_page_write(list_ctrl, start_node);

	if (info->types == NAND_FLASH_TLC)
		return do_tlc_write(info, list_ctrl, start_node, count);
	else
		return do_mlc_multi_plane_write(info,
				list_ctrl, start_node, count);
}

static int mtk_nand_process_list(
	struct mtk_nand_chip_info *info,
	struct worklist_ctrl *list_ctrl,
	int sync_num)
{
	struct list_node *head, *item, *next;
	struct list_node *start_node = NULL;
	unsigned int count;
	unsigned process_cnt = 0;
	struct nand_work *work = NULL;

	mutex_lock(&list_ctrl->sync_lock);

	count = get_list_work_cnt(list_ctrl);

	if (((sync_num > 0) &&
			(count < sync_num))
		|| (count == 0))
		goto OUT;

	if (sync_num && count)
		nand_debug("sync_num:%d, count:%d",
			sync_num, count);

	lock_list(list_ctrl);
	head = &list_ctrl->head;
	item = head->next;
	unlock_list(list_ctrl);

	while (item != NULL) {
		start_node = list_ctrl->is_ready_func(info, head, item, &count);
		if (count) {
			nand_debug("get write count:%d, start_node:%p, sync:%d",
				count, start_node, sync_num);
			next = list_ctrl->process_data_func(info,
					list_ctrl, start_node, count);
			item = next;
			process_cnt += count;
		} else {
			lock_list(list_ctrl);
			item = item->next;
			unlock_list(list_ctrl);
		}
	}
	if (sync_num == -1) {
		lock_list(list_ctrl);
		head = &list_ctrl->head;
		item = head->next;
		unlock_list(list_ctrl);

		while (item != NULL && sync_num != process_cnt) {
			work = get_list_work(item);
			if (info->types == MTK_NAND_FLASH_TLC &&
					work->ops.types == MTK_NAND_OP_WRITE &&
					work->ops.block < info->data_block_num) {
				nand_err("Error!TLC write page not ready!block:%d, page:%d\n",
						work->ops.block, work->ops.page);
				break;
			}
			next = list_ctrl->process_data_func(info, list_ctrl, item, 1);
			item = next;
			process_cnt += 1;
		}
	}

OUT:
	mutex_unlock(&list_ctrl->sync_lock);
	return process_cnt;
}


static int mtk_nand_work_thread(void *u)
{
	struct mtk_nand_chip_info *info = &data_info->chip_info;
	struct worklist_ctrl *elist_ctrl = &data_info->elist_ctrl;
	struct worklist_ctrl *wlist_ctrl = &data_info->wlist_ctrl;
	unsigned int ret = 0;

	nand_debug("mtk_nand_async_thread thread started, PID %d",
		task_pid_nr(current));

	set_freezable();

	for (;;) {
		if (kthread_should_stop())
			break;
		if (try_to_freeze())
			continue;

#ifdef MTK_NAND_OPS_BG_CTRL
		wait_for_completion(&data_info->ops_ctrl);
#endif
		ret = mtk_nand_process_list(info, elist_ctrl, 0);
		ret  |= mtk_nand_process_list(info, wlist_ctrl, 0);

		if (ret == 0) {
			schedule();
			continue;
		} else {
			cond_resched();
		}

	}

	nand_debug("mtk_nand_async_thread thread killed, PID %d",
		task_pid_nr(current));

	return 0;
}

static void mtk_nand_dump_bbt_info(struct mtk_nand_chip_bbt_info *chip_bbt)
{
#if 1
	unsigned int i;

	nand_info("mtk_nand_dump_bbt_info bad_block_num:0x%x",
			chip_bbt->bad_block_num);

	for (i = 0; i < chip_bbt->bad_block_num; i++) {
		nand_info("bad_index:0x%x",
			chip_bbt->bad_block_table[i]);
	}
#endif
}

int mtk_chip_bbt_init(struct data_bmt_struct *data_bmt)
{
	struct mtk_nand_chip_bbt_info *chip_bbt = &data_info->chip_bbt;
	unsigned int i;

	chip_bbt->bad_block_num = data_bmt->bad_count;
	for (i = 0; i < 1024; i++) {
		if (i < data_bmt->bad_count)
			chip_bbt->bad_block_table[i] =
				data_bmt->entry[i].bad_index - data_bmt->start_block;
		else
			chip_bbt->bad_block_table[i] = 0xffff;
	}

	mtk_nand_dump_bbt_info(chip_bbt);

	return 0;
}

static int mtk_chip_info_init(struct mtk_nand_chip_info *chip_info)
{
	unsigned int page_per_block;

	page_per_block = devinfo.blocksize*1024/devinfo.pagesize;

	chip_info->data_block_num = data_info->partition_info.total_block
		*(100 - data_info->partition_info.slc_ratio)/100;

	chip_info->data_page_num = page_per_block;
	chip_info->data_page_size = devinfo.pagesize;
	chip_info->data_block_size = devinfo.blocksize*1024;
	chip_info->log_page_size = devinfo.pagesize;

	if (devinfo.NAND_FLASH_TYPE == MTK_NAND_FLASH_TLC) {
		chip_info->log_page_num =
			page_per_block/MTK_TLC_DIV;
		chip_info->log_block_size =
			chip_info->data_block_size/MTK_TLC_DIV;
	} else {

		chip_info->log_page_num =
			page_per_block/MTK_MLC_DIV;
		chip_info->log_block_size =
			chip_info->data_block_size/MTK_MLC_DIV;
	}

	if (devinfo.advancedmode & MULTI_PLANE) {
		chip_info->data_block_num >>= 1;
		chip_info->data_block_num <<= 1;
	}

	chip_info->log_block_num =
		data_info->partition_info.total_block - chip_info->data_block_num;

	if (devinfo.advancedmode & MULTI_PLANE) {
		chip_info->log_block_num >>= 1;
		chip_info->log_block_num <<= 1;
	}

	chip_info->data_oob_size = (devinfo.pagesize >> host->hw->nand_sec_shift)
					* host->hw->nand_fdm_size;
	chip_info->log_oob_size = chip_info->data_oob_size;

	chip_info->slc_ratio = data_info->partition_info.slc_ratio;
	chip_info->start_block = data_info->partition_info.start_block;

	chip_info->sector_size_shift = 10;

	if (devinfo.NAND_FLASH_TYPE == MTK_NAND_FLASH_TLC)
		chip_info->max_keep_pages = (devinfo.advancedmode & MULTI_PLANE)?18:9;
	else
		chip_info->max_keep_pages = (devinfo.advancedmode & MULTI_PLANE)?2:1;

	if (devinfo.NAND_FLASH_TYPE == MTK_NAND_FLASH_TLC)
		chip_info->types = MTK_NAND_FLASH_TLC;
	else
		chip_info->types = MTK_NAND_FLASH_MLC;

	chip_info->plane_mask = 0x1;
	chip_info->plane_num = (devinfo.advancedmode & MULTI_PLANE)?2:1;
	chip_info->chip_num = 1;

	if (devinfo.advancedmode & MULTI_PLANE)
		chip_info->option = MTK_NAND_PLANE_MODE_SUPPORT;

	return 0;
}

static void mtk_nand_dump_chip_info(struct mtk_nand_chip_info *chip_info)
{
	nand_debug("mtk_nand_chip_info dump info here");
	nand_debug("data_block_num:0x%x", chip_info->data_block_num);
	nand_debug("data_page_num:0x%x", chip_info->data_page_num);
	nand_debug("data_page_size:0x%x", chip_info->data_page_size);
	nand_debug("data_oob_size:0x%x", chip_info->data_oob_size);
	nand_debug("data_block_size:0x%x", chip_info->data_block_size);
	nand_debug("log_block_num:0x%x", chip_info->log_block_num);
	nand_debug("log_page_num:0x%x", chip_info->log_page_num);
	nand_debug("log_page_size:0x%x", chip_info->log_page_size);
	nand_debug("log_block_size:0x%x", chip_info->log_block_size);
	nand_debug("log_oob_size:0x%x", chip_info->log_oob_size);
	nand_debug("slc_ratio:0x%x", chip_info->slc_ratio);
	nand_debug("start_block:0x%x", chip_info->start_block);
	nand_debug("sector_size_shift:0x%x", chip_info->sector_size_shift);
	nand_debug("max_keep_pages:0x%x", chip_info->max_keep_pages);
	nand_debug("types:0x%x", chip_info->types);
	nand_debug("plane_mask:0x%x", chip_info->plane_mask);
	nand_debug("plane_num:0x%x", chip_info->plane_num);
	nand_debug("chip_num:0x%x", chip_info->chip_num);
	nand_debug("option:0x%x", chip_info->option);
}

static void mtk_nand_dump_partition_info(
		struct nand_ftl_partition_info *partition_info)
{
	nand_debug("nand_ftl_partition_info dump info here");
	nand_debug("start_block:0x%x", partition_info->start_block);
	nand_debug("total_block:0x%x", partition_info->total_block);
	nand_debug("slc_ratio:%d", partition_info->slc_ratio);
	nand_debug("slc_block:%d", partition_info->slc_block);
}

static void mtk_nand_dump_bmt_info(struct data_bmt_struct *data_bmt)
{
	unsigned int i;

	nand_debug("nand_ftl_partition_info dump info here");
	nand_debug("bad_count:0x%x", data_bmt->bad_count);
	nand_debug("start_block:0x%x", data_bmt->start_block);
	nand_debug("end_block:0x%x", data_bmt->end_block);
	nand_debug("version:%d", data_bmt->version);

	for (i = 0; i < data_bmt->bad_count; i++) {
		nand_debug("bad_index:0x%x, flag:%d",
			data_bmt->entry[i].bad_index,
			data_bmt->entry[i].flag);
	}
}



/*********	API for nand Wrapper *********/

/* struct mtk_nand_chip_info *mtk_nand_chip_init(void)
 * Init mntl_chip_info to nand wrapper, after nand driver init.
 * Return: On success, return mtk_nand_chip_info. On error, return error num.
 */
struct mtk_nand_chip_info *mtk_nand_chip_init(void)
{

	if (&data_info->chip_info != NULL)
		return &data_info->chip_info;
	else
		return 0;
}
EXPORT_SYMBOL(mtk_nand_chip_init);

/* mtk_nand_chip_read_page
 * Only one page data and FDM data read, support partial read.
 *
 * @info: NAND handler
 * @data_buffer/oob_buffer: Null for no need data/oob, must contiguous address space.
 * @block/page: The block/page to read data.
 * @offset: data offset to start read, and must be aligned to sector size.
 * @size: data size to read. size <= pagesize, less than page size will partial read,
 *    and OOB is only related sectors, uncompleted refer to whole page.
 * return : 0 on success, On error, return error num.
 */
int mtk_nand_chip_read_page(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page,
		unsigned int offset, unsigned int size)
{
	int ret = 0;

	if (oob_buffer == NULL) {
		nand_err("data_buffer is null");
		return -EINVAL;
	}

	if (oob_buffer == NULL) {
		nand_err("oob_buffer is null");
		return -EINVAL;
	}

	if ((offset % (1 << info->sector_size_shift) != 0)
		|| (size % (1 << info->sector_size_shift) != 0)) {
		nand_err("offset or size is invalid:offset:%d, size:%d",
			offset, size);
		return -EINVAL;
	}

	if (!block_page_num_is_valid(info, block, page)) {
		nand_err("block or page num is invalid:block:%d, page:%d",
			block, page);
		return -EINVAL;
	}

	if (mtk_isbad_block(block))
		return -ENANDBAD;

	ret = mtk_nand_read_pages(info, data_buffer,
		oob_buffer, block, page, offset, size);

	return ret;
}
EXPORT_SYMBOL(mtk_nand_chip_read_page);


/* mtk_nand_chip_write_page
 * write page API. Only one page data write, async mode.
 * Just return 0 and add to write worklist as below:
 *  a) For TLC WL write, NAND handler call nand driver WL write function.
 *  b) For Multi-plane program, if more_page is TRUE,
 *  wait for the next pages write and do multi-plane write on time.
 *  c) For cache  program, driver will depend on more_page flag for TLC program,
 *  can not used for SLC/MLC program.
 * after Nand driver erase/write operation, callback function will be done.
 *
 * @info: NAND handler
 * @data_buffer/oob_buffer: must contiguous address space.
 * @block/page: The block/page to write data.
 * @more_page: for TLC WL write and multi-plane program operation.
 *    if more_page is true, driver will wait complete operation and call driver function.
 * @*callback: callback for wrapper, called after driver finish the operation.
 * @*userdata : for callback function
 * return : 0 on success, On error, return error num casted by ERR_PTR
 */
int mtk_nand_chip_write_page(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page, bool more_page,
		mtk_nand_callback_func callback, void *userdata)
{
	/* Add to worklist here*/
	struct worklist_ctrl *list_ctrl;
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops;
	int total_num;

	nand_debug("%s, block:0x%x page:0x%x more_page:0x%x",
		__func__, block, page, more_page);

	if (oob_buffer == NULL) {
		nand_err("data_buffer is null");
		return -EINVAL;
	}

	if (oob_buffer == NULL) {
		nand_err("oob_buffer is null");
		return -EINVAL;
	}

	if (!block_page_num_is_valid(info, block, page)) {
		nand_err("block or page num is invalid:block:%d, page:%d",
			block, page);
		return -EINVAL;
	}

	list_ctrl = &data_info->wlist_ctrl;
	total_num = get_list_work_cnt(list_ctrl);

	while (total_num >= info->max_keep_pages) {
		nand_debug("total_num:%d", total_num);
		mtk_nand_process_list(info,
				list_ctrl, info->max_keep_pages);
		list_ctrl = &data_info->wlist_ctrl;
		total_num = get_list_work_cnt(list_ctrl);
	};

	work = kmalloc(sizeof(struct nand_work), GFP_KERNEL);
	if (work == NULL)
		return -ENOMEM;

	work->list.next = NULL;
	ops = &work->ops;

	ops->info = info;
	ops->types = MTK_NAND_OP_WRITE;
	ops->data_buffer = data_buffer;
	ops->oob_buffer = oob_buffer;
	ops->block = block;
	ops->page = page;
	ops->more = more_page;
	ops->callback = callback;
	ops->userdata = userdata;

	nand_debug("block:%d, page:%d, add(%p)",
		block, page, &work->list);

	list_ctrl = &data_info->wlist_ctrl;

	add_list_node(list_ctrl, &work->list);

#ifdef MTK_NAND_OPS_BG_CTRL
	complete(&data_info->ops_ctrl);
#else
	wake_up_process(data_info->nand_bgt);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_nand_chip_write_page);

/*
 * mtk_nand_chip_erase_block
 * Erase API for nand wrapper, async mode for erase, just return success,
 * put erase operation into write worklist.
 * After Nand driver erase/write operation, callback function will be done.
 * @block: The block to erase
 * @*callback: Callback for wrapper, called after driver finish the operation.
 * @* userdata: for callback function
 * return : 0 on success, On error, return error num casted by ERR_PTR
 */
int mtk_nand_chip_erase_block(struct mtk_nand_chip_info *info,
		unsigned int block, unsigned int more_block,
		mtk_nand_callback_func callback, void *userdata)
{
	struct worklist_ctrl *list_ctrl;
	struct nand_work *work;
	struct mtk_nand_chip_operation *ops;
	int total_num;

	nand_debug("%s, block:0x%x more_page:0x%x",
		__func__, block, more_block);

	if (!block_num_is_valid(info, block)) {
		nand_err("block num is invalid:block:%d", block);
		return -EINVAL;
	}

	list_ctrl = &data_info->elist_ctrl;
	total_num = get_list_work_cnt(list_ctrl);

	while (total_num >= info->max_keep_pages) {
		nand_debug("total_num:%d", total_num);
		mtk_nand_process_list(info,
				list_ctrl, info->max_keep_pages);
		list_ctrl = &data_info->elist_ctrl;
		total_num = get_list_work_cnt(list_ctrl);
	};

	work = kmalloc(sizeof(struct nand_work), GFP_KERNEL);
	if (work == NULL)
		return -ENOMEM;

	work->list.next = NULL;
	ops = &work->ops;

	ops->info = info;
	ops->types = MTK_NAND_OP_ERASE;
	ops->block = block;
	ops->page = 0;
	ops->more = more_block;
	ops->data_buffer = NULL;
	ops->oob_buffer = NULL;
	ops->callback = callback;
	ops->userdata = userdata;

	list_ctrl = &data_info->elist_ctrl;

	nand_debug("block:%d, add(%p)",
		block, &work->list);

	add_list_node(list_ctrl, &work->list);

#ifdef MTK_NAND_OPS_BG_CTRL
	complete(&data_info->ops_ctrl);
#else
	wake_up_process(data_info->nand_bgt);
#endif

	return 0;
}
EXPORT_SYMBOL(mtk_nand_chip_erase_block);

/*
 * mtk_nand_chip_sync
 * flush all async worklist to nand driver.
 * return : On success, return 0. On error, return error num
 */
int mtk_nand_chip_sync(struct mtk_nand_chip_info *info)
{
	struct worklist_ctrl *elist_ctrl, *wlist_ctrl;
	int ret = 0;

	nand_debug("%s enter", __func__);

	elist_ctrl = &data_info->elist_ctrl;
	wlist_ctrl = &data_info->wlist_ctrl;

	ret = mtk_nand_process_list(info, elist_ctrl, -1);
	nand_debug(" sync erase done");

	ret |= mtk_nand_process_list(info, wlist_ctrl, -1);

	nand_debug("sync done, ret:%d", ret);
	return ret;
}
EXPORT_SYMBOL(mtk_nand_chip_sync);

/* mtk_nand_chip_bmt, bad block table maintained by driver, and read only for wrapper
 * @info: NAND handler
 * Return FTL partition's bad block table for nand wrapper.
 */
const struct mtk_nand_chip_bbt_info *mtk_nand_chip_bmt(struct mtk_nand_chip_info *info)
{

	if (&data_info->chip_bbt != NULL)
		return &data_info->chip_bbt;
	else
		return 0;
}
EXPORT_SYMBOL(mtk_nand_chip_bmt);

/* mtk_chip_mark_bad_block
 * Mark specific block as bad block,and update bad block list and bad block table.
 * @block: block address to markbad
 */
void mtk_chip_mark_bad_block(unsigned int block)
{
	struct mtk_nand_chip_bbt_info *chip_bbt = &data_info->chip_bbt;
	unsigned int i;

	nand_debug("markbad block:0x%x", block);
	for (i = 0; i < chip_bbt->bad_block_num; i++) {
		if (block == chip_bbt->bad_block_table[i])
			return;
	}

	chip_bbt->bad_block_table[chip_bbt->bad_block_num++] = block;

	update_bmt(block + data_info->bmt.start_block, 0, NULL, NULL);
}
EXPORT_SYMBOL(mtk_chip_mark_bad_block);

/************** UNIT TEST ***************/
#ifdef MTK_NAND_CHIP_TEST
int mtk_nand_debug_callback(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page, int status, void *userdata)
{
#if 0
	nand_debug("info:0x%x", info);
	nand_debug("data_buffer:0x%lx", data_buffer);
	nand_debug("oob_buffer:0x%lx", oob_buffer);
#endif
	nand_debug("mtk_nand_debug_callback block:0x%x", block);
	nand_debug("mtk_nand_debug_callback page:0x%x", page);
	nand_debug("mtk_nand_debug_callback status:0x%x", status);
#if 0

	nand_debug("userdata:0x%x", (unsigned int)userdata);
#endif
#if 0
	if (data_buffer != NULL)
		kfree(data_buffer);
	if (oob_buffer != NULL)
		kfree(oob_buffer);
#endif
	return 0;
}

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
static void dump_buf_data(unsigned char *buf, unsigned size)
{
	unsigned int i;

	for (i = 0; i < size; i += 16) {
		pr_info("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       buf[i], buf[i+1], buf[i+2], buf[i+3],
		       buf[i+4], buf[i+5], buf[i+6], buf[i+7],
		       buf[i+8], buf[i+9], buf[i+10], buf[i+11],
		       buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
	}
}
#endif

#define MAGIC_PATTERN_S (0x4E414542)
static int ticks = 1;
static int randseed = 12345;
static int total_count = 10;

static int rand(void)
{
	return (randseed = randseed * 12345 + 17);
}

static void seed(int seed)
{
	randseed = seed;
}


static unsigned short SS_RANDOM_SEED[128] = {
	/* for page 0~127 */
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

static void prandom_bytes(char *buf, unsigned int size)
{
	int i = 0;
	int temp;

	for (i = 0; i < size; i++) {
		temp = rand() % 256;
		temp = (temp < 128) ? temp : temp-256;
		buf[i] = (char)temp;
	}
	seed((int)SS_RANDOM_SEED[total_count++]+ticks);
	if (total_count > 127)
		total_count = 0;
	ticks++;
}

static void psequence_bytes(char *buf, unsigned int size)
{
	int i = 0;
	unsigned short *buf_tmp;

	buf_tmp = (unsigned short *)buf;
	for (i = 0; i < size/2; i++)
		buf_tmp[i] = i;
}

static int check_data_empty(void *data, unsigned size)
{
	unsigned i;
	u32 *tp = (u32 *) data;

	for (i = 0; i < size / 4; i++) {
		if (*(tp + i) != 0xffffffff)
			return 0;
	}

	return 1;
}

#ifdef MTK_NAND_READ_COMPARE
static int compare_data(unsigned char *sbuf, unsigned char *dbuf, unsigned size)
{
	unsigned i, ret = 0;

	for (i = 0; i < size; i++) {
		if (sbuf[i] != dbuf[i]) {
			nand_err("compare data failed sbuf[%d]:%x dbuf[%d]:%x",
				i, sbuf[i], i, dbuf[i]);
			ret = 1;
		}
	}

	if (ret) {
#if 0
		for (i = 0; i < 16; i++) {
			nand_err("Nand_ErrNUM[%d]:%d",
				i, Nand_ErrNUM[i]);
			for (j = 0; j < ((Nand_ErrNUM[i] + 1) >> 1); ++j)
				nand_err("Nand_ErrBitLoc:0x%x",
					Nand_ErrBitLoc[i][j]);
		}
#endif
		dump_nfi();
	}
	return 0;
}
#endif

#ifdef MTK_NAND_PERFORMANCE_TEST
static suseconds_t time_diff(struct timeval *end_time, struct timeval *start_time)
{
	struct timeval difference;

	difference.tv_sec = end_time->tv_sec - start_time->tv_sec;
	difference.tv_usec = end_time->tv_usec - start_time->tv_usec;

	/* Using while instead of if below makes the code slightly more robust. */

	while (difference.tv_usec < 0) {
		difference.tv_usec += 1000000;
		difference.tv_sec -= 1;
	}

	return 1000000LL * difference.tv_sec + difference.tv_usec;
}
#endif

void mtk_chip_unit_test(void)
{
	struct mtk_nand_chip_info *chip_info = &data_info->chip_info;
#ifdef MTK_NAND_PERFORMANCE_TEST
	struct timeval stimer, etimer;
	suseconds_t total_time;
	unsigned long block_size;
#endif
	unsigned char *data_buffer, *temp_buf;
	unsigned char *oob_buffer, *oob_buf;
	unsigned int i, j, data_flag, read_cnt;
	unsigned int pages_per_blk;
	unsigned int userdata;
	int ret = 0;

	data_buffer =  kmalloc(chip_info->data_page_size, GFP_KERNEL);
	oob_buffer =  kmalloc(1024, GFP_KERNEL);

	temp_buf =  kmalloc(chip_info->data_page_size, GFP_KERNEL);
	oob_buf =  kmalloc(1024, GFP_KERNEL);

	/*** BMT Related ***/

	/* dump bbt info */
	mtk_nand_dump_bbt_info(&data_info->chip_bbt);
#if 0
	/* markbad */
	mtk_chip_mark_bad_block(0x8);
	mtk_chip_mark_bad_block(0x50);
	mtk_chip_mark_bad_block(0x78);
	mtk_chip_mark_bad_block(0x9);

	/* dump bbt info*/
	nand_info("dump bbt info");
	for (i = 0; i < pt_info->total_block; i++)
		mtk_isbad_block(i);
#endif

	/*** block Erase/Write/Read ***/
	nand_info("chip_info->data_block_num:0x%x",
		chip_info->data_block_num);

	data_flag = 0;

#if 0
			if (data_flag == 1) {
				prandom_bytes(data_buffer, chip_info->data_page_size);
				prandom_bytes(oob_buffer, chip_info->data_oob_size);
			} else if (data_flag == 0) {
				psequence_bytes(data_buffer, chip_info->data_page_size);
				prandom_bytes(oob_buffer, chip_info->data_oob_size);
			}
			memcpy(temp_buf, data_buffer, chip_info->data_page_size);
			memcpy(oob_buf, oob_buffer, chip_info->data_oob_size);
#endif

	/* for (i = 0; i < 1; i += 2) { */
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
	/* for (i = 0; i < chip_info->data_block_num + chip_info->log_block_num; i += 2) { */
	for (i = chip_info->data_block_num - 8; i < chip_info->data_block_num + 8; i += 2) {
#else
	for (i = 0; i < chip_info->data_block_num + chip_info->log_block_num; i++) {
#endif
		nand_info("Begin to test block:0x%x", i);
		/* block test*/
		if (i < chip_info->data_block_num)
			pages_per_blk = chip_info->data_page_num;
		else
			pages_per_blk = chip_info->log_page_num;

		if (mtk_isbad_block(i)) {
			nand_err("Skip bad blk:0x%x", i);
			continue;
		}

		userdata = (i*chip_info->data_page_num) | (1<<31);
#if 1
#ifdef MTK_NAND_PERFORMANCE_TEST
			do_gettimeofday(&stimer);
#endif
		ret = mtk_nand_chip_erase_block(chip_info, i, 1,
			&mtk_nand_debug_callback, &userdata);
		if (ret) {
			nand_err("bad block here block:0x%x", i);
			continue;
		}
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		if (mtk_isbad_block(i+1)) {
			nand_err("Skip bad blk:0x%x", i);
			continue;
		}

		ret = mtk_nand_chip_erase_block(chip_info, i+1, 1,
			&mtk_nand_debug_callback, &userdata);
		if (ret) {
			nand_err("bad block here block:0x%x", i);
			continue;
		}
#endif
		mtk_nand_chip_sync(chip_info);


#ifdef MTK_NAND_PERFORMANCE_TEST
		do_gettimeofday(&etimer);
		total_time = time_diff(&etimer, &stimer);
		nand_info("Erase total_time:%ld mS", total_time/1000);
		total_time  = 0;
#endif

#if 1
		if (data_flag++ & 1) {
			prandom_bytes(data_buffer, chip_info->data_page_size);
			psequence_bytes(oob_buffer, chip_info->data_oob_size);
		} else {
			psequence_bytes(data_buffer, chip_info->data_page_size);
			prandom_bytes(oob_buffer, chip_info->data_oob_size);
		}
		memcpy(temp_buf, data_buffer, chip_info->data_page_size);
		memcpy(oob_buf, oob_buffer, chip_info->data_oob_size);
#endif

#ifdef MTK_NAND_PERFORMANCE_TEST
			do_gettimeofday(&stimer);
#endif
		for (j = 0; j < pages_per_blk; j++) {
#if 0
			if (data_flag == 1) {
				prandom_bytes(data_buffer, chip_info->data_page_size);
				prandom_bytes(oob_buffer, chip_info->data_oob_size);
			} else if (data_flag == 0) {
				psequence_bytes(data_buffer, chip_info->data_page_size);
				psequence_bytes(oob_buffer, chip_info->data_oob_size);
			}
			memcpy(temp_buf, data_buffer, chip_info->data_page_size);
			memcpy(oob_buf, oob_buffer, chip_info->data_oob_size);
#endif

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
			nand_info("write blk:0x%x page:0x%x data dump", i, j);
			dump_buf_data(data_buffer, chip_info->data_oob_size);
			nand_info("oob dump here");
			dump_buf_data(oob_buffer, chip_info->data_oob_size);
#endif
			userdata = (i*chip_info->data_page_num + j) | (1<<30);

			ret = mtk_nand_chip_write_page(chip_info, data_buffer, oob_buffer,
				i, j, 1, &mtk_nand_debug_callback, &userdata);
			if (ret) {
				nand_err("bad block here block:0x%x", i);
				break;
			}

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
			ret = mtk_nand_chip_write_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 1, &mtk_nand_debug_callback, &userdata);
			if (ret) {
				nand_err("bad block here block:0x%x", i);
				break;
			}
#endif

#if 0
			if (i >= chip_info->data_block_num) {
				mtk_nand_chip_sync(chip_info);

				ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
					i, j, 0, chip_info->data_page_size);
				if ((ret) && (ret != -ENANDFLIPS)) {
					nand_err("read block failed i:0x%x j:0x%x", i, j);
					break;
				}

				if (check_data_empty(data_buffer, chip_info->data_page_size)) {
					nand_debug("Check empty page here blk:0x%x page:0x%x ", i, j);
					break;
				}

#ifdef MTK_NAND_READ_COMPARE

				if (compare_data(data_buffer, temp_buf, chip_info->data_page_size))
					nand_err("check data failed i:0x%x j:0x%x", i, j);

				if (compare_data(oob_buffer, oob_buf, chip_info->data_oob_size))
					nand_err("check data failed i:0x%x j:0x%x", i, j);
#endif

			}
#endif
		}

		mtk_nand_chip_sync(chip_info);

#ifdef MTK_NAND_PERFORMANCE_TEST
		do_gettimeofday(&etimer);
		total_time = time_diff(&etimer, &stimer);
		block_size = (pages_per_blk*chip_info->data_page_size);
		block_size *= 1000;
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		block_size *= 2;
#endif
		nand_info("Write Speed: %ldKB/S, total_time:%ld mS, block_size:%ld",
			block_size/total_time, total_time/1000, block_size/1000);
		total_time  = 0;
#endif

#endif

#if 1
		read_cnt = 1;
		prandom_bytes(data_buffer, chip_info->data_page_size);
		psequence_bytes(oob_buffer, chip_info->data_oob_size);
READ_DISTUB:
		nand_info("block:0x%x, read_cnt:0x%x", i, read_cnt);

		for (j = 0; j < pages_per_blk; j++) {
			memset(data_buffer, 0, chip_info->data_page_size);
			memset(oob_buffer, 0, chip_info->data_oob_size);
#ifdef MTK_NAND_PERFORMANCE_TEST
			do_gettimeofday(&stimer);
#endif
#if 1
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i, j, 0, chip_info->data_page_size);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
#else
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i, j, 0, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i, j, 4096, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i, j, 8192, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i, j, 12288, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
#endif
#ifdef MTK_NAND_PERFORMANCE_TEST
		do_gettimeofday(&etimer);
		total_time += time_diff(&etimer, &stimer);
#endif
			if (check_data_empty(data_buffer, chip_info->data_page_size)) {
				nand_err("*****Check empty page here blk:0x%x page:0x%x ", i, j);
				break;
			}

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
			nand_info("read blk:0x%x page:0x%x data first 64 bytes dump", i, j);
			dump_buf_data(data_buffer, chip_info->data_oob_size);
			nand_info("data last 64 bytes dump");
			dump_buf_data((data_buffer+chip_info->data_page_size
					-chip_info->data_oob_size),
					chip_info->data_oob_size);

			nand_info("oob dump here");
			dump_buf_data(oob_buffer, chip_info->data_oob_size);
#endif

#ifdef MTK_NAND_READ_COMPARE
			if (compare_data(data_buffer, temp_buf, chip_info->data_page_size))
				nand_err("$$$$$$check data failed i:0x%x j:0x%x", i, j);

			if (compare_data(oob_buffer, oob_buf, chip_info->data_oob_size))
				nand_err("######check fdm failed i:0x%x j:0x%x", i, j);
#endif

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
#ifdef MTK_NAND_PERFORMANCE_TEST
			do_gettimeofday(&stimer);
#endif
#if 1
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 0, chip_info->data_page_size);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
#else
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 0, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 4096, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 8192, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
			ret = mtk_nand_chip_read_page(chip_info, data_buffer, oob_buffer,
				i+1, j, 12288, 4096);
			if (ret && (ret != -ENANDFLIPS)) {
				nand_err("read block failed i:0x%x j:0x%x", i, j);
				break;
			}
#endif
#ifdef MTK_NAND_PERFORMANCE_TEST
		do_gettimeofday(&etimer);
		total_time += time_diff(&etimer, &stimer);
#endif
			if (check_data_empty(data_buffer, chip_info->data_page_size)) {
				nand_err("*****Check empty page here blk:0x%x page:0x%x ", i+1, j);
				break;
			}

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
			nand_info("read blk:0x%x page:0x%x data first 64 bytes dump", i+1, j);
			dump_buf_data(data_buffer, chip_info->data_oob_size);
			nand_info("data last 64 bytes dump");
			dump_buf_data((data_buffer+chip_info->data_page_size
					-chip_info->data_oob_size),
					chip_info->data_oob_size);

			nand_info("oob dump here");
			dump_buf_data(oob_buffer, chip_info->data_oob_size);
#endif

#ifdef MTK_NAND_READ_COMPARE
			if (compare_data(data_buffer, temp_buf, chip_info->data_page_size))
				nand_err("$$$$$$check data failed i:0x%x j:0x%x", i+1, j);

			if (compare_data(oob_buffer, oob_buf, chip_info->data_oob_size))
				nand_err("######check fdm failed i:0x%x j:0x%x", i+1, j);
#endif
#endif

		}

#ifdef MTK_NAND_PERFORMANCE_TEST

		block_size = (pages_per_blk*chip_info->data_page_size);
		block_size *= 1000;
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		block_size *= 2;
#endif
		nand_info("Read Speed: %ldKB/S, total_time:%ld mS, block_size:%ld",
			block_size/total_time, total_time/1000, block_size/1000);
#endif
		if (read_cnt++ < 1)
			goto READ_DISTUB;
#endif

	}

	kfree(data_buffer);
	kfree(oob_buffer);

	nand_info("^^^^^^^TEST END^^^^^^^^");

	/* while(1); */

}
#endif

int mtk_nand_data_info_init(void)
{
	data_info = kmalloc(sizeof(struct mtk_nand_data_info), GFP_KERNEL);
	if (data_info == NULL) {
		nand_err("Malloc datainfo size: 0x%lx failed\n",
			sizeof(struct mtk_nand_data_info));
		return -ENOMEM;
	}
	memset(data_info, 0, sizeof(struct mtk_nand_data_info));

	return 0;
}

int mtk_nand_ops_init(struct mtd_info *mtd, struct nand_chip *chip)
{
	int ret = 0;

	data_info->mtd = mtd;

	ret = get_data_partition_info(&data_info->partition_info);
	if (ret) {
		nand_err("Get FTL partition info failed\n");
		goto err_out;
	}
	mtk_nand_dump_partition_info(&data_info->partition_info);

	ret = get_data_bmt(&data_info->bmt);
	if (ret) {
		pr_err("Get FTL bmt info failed\n");
		goto err_out;
	}
	mtk_nand_dump_bmt_info(&data_info->bmt);

	ret = mtk_chip_info_init(&data_info->chip_info);
	if (ret) {
		pr_err("Get chip info failed\n");
		goto err_out;
	}
	mtk_nand_dump_chip_info(&data_info->chip_info);

	mtk_chip_bbt_init(&data_info->bmt);
	if (ret) {
		pr_err("Get chip bbt info failed\n");
		goto err_out;
	}

	init_list_ctrl(&data_info->elist_ctrl,
		complete_erase_count,
		mtk_nand_do_erase);

	init_list_ctrl(&data_info->wlist_ctrl,
		complete_write_count,
		mtk_nand_do_write);

#ifdef MTK_NAND_OPS_BG_CTRL
	init_completion(&data_info->ops_ctrl);
#endif

	data_info->nand_bgt = kthread_create(mtk_nand_work_thread,
				data_info, "nand_bgt");
	if (IS_ERR(data_info->nand_bgt)) {
		ret = PTR_ERR(data_info->nand_bgt);
		data_info->nand_bgt = NULL;
		nand_err("kthread_create failed error %d", ret);
		goto err_out;
	}

	/* wake_up_process(data_info->nand_bgt); */
#ifdef MTK_NAND_CHIP_TEST
	mtk_chip_unit_test();
#endif
	return 0;

err_out:
	return ret;
}
