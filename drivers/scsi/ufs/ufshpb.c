// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017-2018 Samsung Electronics Co., Ltd.
 */

#include "ufshcd.h"
#include "ufshpb.h"
#include "ufsfeature.h"
#define UFSHCD_REQ_SENSE_SIZE	18

/*
 * define global constants
 */
static int sects_per_blk_shift;
static int bits_per_dword_shift;
static int bits_per_dword_mask;
static int bits_per_byte_shift;

static int ufshpb_create_sysfs(struct ufsf_feature *ufsf,
			       struct ufshpb_lu *hpb);
static int ufshpb_remove_sysfs(struct ufshpb_lu *hpb);

static inline bool ufshpb_is_triggered_by_fs(struct ufshpb_region *rgn)
{
	return atomic_read(&rgn->reason) == HPBUPDATE_RT_FROM_FS;
}
static inline void
ufshpb_get_pos_from_lpn(struct ufshpb_lu *hpb, unsigned long lpn, int *rgn_idx,
			int *srgn_idx, int *offset)
{
	int rgn_offset;

	*rgn_idx = lpn >> hpb->entries_per_rgn_shift;
	rgn_offset = lpn & hpb->entries_per_rgn_mask;
	*srgn_idx = rgn_offset >> hpb->entries_per_srgn_shift;
	*offset = rgn_offset & hpb->entries_per_srgn_mask;
}

inline int ufshpb_valid_srgn(struct ufshpb_region *rgn,
			     struct ufshpb_subregion *srgn)
{
	return rgn->rgn_state != HPBREGION_INACTIVE &&
		srgn->srgn_state == HPBSUBREGION_CLEAN;
}

/* Must be held hpb_lock  */
static bool ufshpb_ppn_dirty_check(struct ufshpb_lu *hpb, unsigned long lpn,
				   int transfer_len)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long cur_lpn = lpn;
	int rgn_idx, srgn_idx, srgn_offset, find_size;
	int scan_cnt = transfer_len;

	do {
		ufshpb_get_pos_from_lpn(hpb, cur_lpn, &rgn_idx, &srgn_idx,
					&srgn_offset);
		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!ufshpb_valid_srgn(rgn, srgn))
			return true;

		if (!srgn->mctx || !srgn->mctx->ppn_dirty)
			return true;

		if (hpb->entries_per_srgn < srgn_offset + scan_cnt) {
			int cnt = hpb->entries_per_srgn - srgn_offset;

			find_size = hpb->entries_per_srgn;
			scan_cnt -= cnt;
			cur_lpn += cnt;
		} else {
			find_size = srgn_offset + scan_cnt;
			scan_cnt = 0;
		}

		srgn_offset =
			find_next_bit((unsigned long *)srgn->mctx->ppn_dirty,
				      hpb->entries_per_srgn, srgn_offset);

		if (srgn_offset < hpb->entries_per_srgn)
			return srgn_offset < find_size;

	} while (scan_cnt);

	return false;
}

static void ufshpb_set_read16_cmd(struct ufshpb_lu *hpb,
				  struct ufshcd_lrb *lrbp,
				  unsigned long long ppn,
				  unsigned int transfer_len)
{
	unsigned char *cdb = lrbp->cmd->cmnd;

	cdb[0] = READ_16;
	cdb[2] = lrbp->cmd->cmnd[2];
	cdb[3] = lrbp->cmd->cmnd[3];
	cdb[4] = lrbp->cmd->cmnd[4];
	cdb[5] = lrbp->cmd->cmnd[5];
	cdb[6] = GET_BYTE_7(ppn);
	cdb[7] = GET_BYTE_6(ppn);
	cdb[8] = GET_BYTE_5(ppn);
	cdb[9] = GET_BYTE_4(ppn);
	cdb[10] = GET_BYTE_3(ppn);
	cdb[11] = GET_BYTE_2(ppn);
	cdb[12] = GET_BYTE_1(ppn);
	cdb[13] = GET_BYTE_0(ppn);

	if (lrbp->hpb_ctx_id < MAX_HPB_CONTEXT_ID)
		cdb[14] = (1 << 7) | lrbp->hpb_ctx_id;
	else
		cdb[14] = UFSHPB_GROUP_NUMBER;

	cdb[15] = transfer_len;

	lrbp->cmd->cmd_len = MAX_CDB_SIZE;
}

/* called with hpb_lock (irq) */
static inline void
ufshpb_set_dirty_bits(struct ufshpb_lu *hpb, struct ufshpb_region *rgn,
		      struct ufshpb_subregion *srgn, int dword, int offset,
		      unsigned int cnt)
{
	const unsigned long mask = ((1UL << cnt) - 1) & 0xffffffff;

	if (rgn->rgn_state == HPBREGION_INACTIVE)
		return;

	BUG_ON(!srgn->mctx);
	srgn->mctx->ppn_dirty[dword] |= (mask << offset);
}

static inline void ufshpb_get_bit_offset(struct ufshpb_lu *hpb, int srgn_offset,
					 int *dword, int *offset)
{
	*dword = srgn_offset >> bits_per_dword_shift;
	*offset = srgn_offset & bits_per_dword_mask;
}

static void ufshpb_set_dirty(struct ufshpb_lu *hpb, struct ufshcd_lrb *lrbp,
			     int rgn_idx, int srgn_idx, int srgn_offset)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	int cnt, bit_cnt, bit_dword, bit_offset;

	cnt = blk_rq_sectors(lrbp->cmd->request) >> sects_per_blk_shift;
	ufshpb_get_bit_offset(hpb, srgn_offset, &bit_dword, &bit_offset);

	do {
		bit_cnt = min(cnt, BITS_PER_DWORD - bit_offset);

		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		ufshpb_set_dirty_bits(hpb, rgn, srgn, bit_dword, bit_offset,
				      bit_cnt);

		bit_offset = 0;
		bit_dword++;

		if (bit_dword == hpb->dwords_per_srgn) {
			bit_dword = 0;
			srgn_idx++;

			if (srgn_idx == hpb->srgns_per_rgn) {
				srgn_idx = 0;
				rgn_idx++;
			}
		}
		cnt -= bit_cnt;
	} while (cnt);

	BUG_ON(cnt < 0);
}

static inline bool ufshpb_is_read_cmd(struct scsi_cmnd *cmd)
{
	if (cmd->cmnd[0] == READ_10 || cmd->cmnd[0] == READ_16)
		return true;

	return false;
}
static inline bool ufshpb_is_write_cmd(struct scsi_cmnd *cmd)
{
	return (cmd->cmnd[0] == WRITE_10 || cmd->cmnd[0] == WRITE_16);
}

static inline bool ufshpb_is_discard_cmd(struct scsi_cmnd *cmd)
{
	return cmd->cmnd[0] == UNMAP;
}

static inline bool ufshpb_is_write_discard_lrbp(struct ufshcd_lrb *lrbp)
{
	if (lrbp->cmd->cmnd[0] == WRITE_10 || lrbp->cmd->cmnd[0] == WRITE_16 ||
	    lrbp->cmd->cmnd[0] == UNMAP)
		return true;

	return false;
}


static inline bool ufshpb_is_host_control_mode(struct ufshpb_lu *hpb)
{
	if (hpb->ufsf->hpb_dev_info.hpb_control_mode == 0)
		return true;

	return false;
}

static unsigned long long ufshpb_get_ppn(struct ufshpb_map_ctx *mctx, int pos,
					 int *error)
{
	unsigned long long *ppn_table;
	struct page *page = NULL;
	int index, offset;

	index = pos / HPB_ENTREIS_PER_OS_PAGE;
	offset = pos % HPB_ENTREIS_PER_OS_PAGE;

	page = mctx->m_page[index];
	if (!page) {
		*error = -ENOMEM;
		ERR_MSG("mctx %p cannot get m_page", mctx);
		return 0;
	}

	ppn_table = page_address(page);
	if (!ppn_table) {
		*error = -ENOMEM;
		ERR_MSG("mctx %p cannot get ppn_table vm", mctx);
		return 0;
	}

	return ppn_table[offset];
}

static inline int ufshpb_lu_get(struct ufshpb_lu *hpb)
{
	if (!hpb || hpb->ufsf->ufshpb_state != HPB_PRESENT)
		return -ENODEV;

	kref_get(&hpb->ufsf->ufshpb_kref);
	return 0;
}

static inline void ufshpb_schedule_error_handler(struct kref *kref)
{
	struct ufsf_feature *ufsf;

	ufsf = container_of(kref, struct ufsf_feature, ufshpb_kref);
	schedule_work(&ufsf->ufshpb_eh_work);
}

static inline void ufshpb_lu_put(struct ufshpb_lu *hpb)
{
	kref_put(&hpb->ufsf->ufshpb_kref, ufshpb_schedule_error_handler);
}

static void ufshpb_failed(struct ufshpb_lu *hpb, const char *f)
{
	ERR_MSG("ufshpb_driver failed. function (%s)", f);
#if defined(CONFIG_HPB_RELEASE_CHECK)
	BUG();
#endif
	hpb->ufsf->ufshpb_state = HPB_FAILED;
	ufshpb_lu_put(hpb);
}

static inline void ufshpb_put_pre_req(struct ufshpb_lu *hpb,
				      struct ufshpb_req *pre_req)
{
	list_add_tail(&pre_req->list_req, &hpb->lh_pre_req_free);
	hpb->num_inflight_pre_req--;
}

static struct ufshpb_req *ufshpb_get_pre_req(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *pre_req;

	if (hpb->num_inflight_pre_req >= hpb->throttle_pre_req) {
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "pre_req throttle. inflight %d throttle %d",
			  hpb->num_inflight_pre_req, hpb->throttle_pre_req);
#endif
		return NULL;
	}

	pre_req = list_first_entry_or_null(&hpb->lh_pre_req_free,
					   struct ufshpb_req,
					   list_req);
	if (!pre_req) {
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "There is no pre_req");
#endif
		return NULL;
	}

	list_del_init(&pre_req->list_req);
	hpb->num_inflight_pre_req++;

	return pre_req;
}

static void ufshpb_pre_req_compl_fn(struct request *req, blk_status_t error)
{
	struct ufshpb_req *pre_req = (struct ufshpb_req *)req->end_io_data;
	struct ufshpb_lu *hpb = pre_req->hpb;
	unsigned long flags;
	struct scsi_sense_hdr sshdr;

	if (error) {
		ERR_MSG("error number %d", error);
		scsi_normalize_sense(pre_req->sense, SCSI_SENSE_BUFFERSIZE,
				     &sshdr);
		ERR_MSG("code %x sense_key %x asc %x ascq %x",
			sshdr.response_code,
			sshdr.sense_key, sshdr.asc, sshdr.ascq);
		ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x",
			sshdr.byte4, sshdr.byte5,
			sshdr.byte6, sshdr.additional_length);
	}

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_pre_req(pre_req->hpb, pre_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ufshpb_lu_put(pre_req->hpb);
}

static int ufshpb_prep_entry(struct ufshpb_req *pre_req,
			     struct page *page)
{
	struct ufshpb_lu *hpb = pre_req->hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long long *addr;
	unsigned long long entry_ppn = 0;
	unsigned long lpn = pre_req->wb.lpn;
	int rgn_idx, srgn_idx, srgn_offset;
	int i, error = 0;
	unsigned long flags;

	addr = page_address(page);

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	for (i = 0; i < pre_req->wb.len; i++, lpn++) {
		ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx,
					&srgn_offset);

		rgn = hpb->rgn_tbl + rgn_idx;
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!ufshpb_valid_srgn(rgn, srgn))
			goto mctx_error;

		if (!srgn->mctx)
			goto mctx_error;

		entry_ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
		if (error)
			goto mctx_error;

		addr[i] = entry_ppn;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return 0;
mctx_error:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return -ENOMEM;
}

static int ufshpb_pre_req_add_bio_page(struct request_queue *q,
				       struct ufshpb_req *pre_req)
{
	struct page *page = pre_req->wb.m_page;
	struct bio *bio = pre_req->bio;
	int ret;

	BUG_ON(!page);

	bio_reset(bio);

	ret = ufshpb_prep_entry(pre_req, page);
	if (ret)
		return ret;

	ret = bio_add_pc_page(q, bio, page, OS_PAGE_SIZE, 0);
	if (ret != OS_PAGE_SIZE) {
		ERR_MSG("bio_add_pc_page fail: %d", ret);
		return -ENOMEM;
	}

	return 0;
}

static void ufshpb_init_cmd_errh(struct scsi_cmnd *cmd)
{
	cmd->serial_number = 0;
	scsi_set_resid(cmd, 0);
	memset(cmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
	if (cmd->cmd_len == 0)
		cmd->cmd_len = scsi_command_size(cmd->cmnd);
}

static void ufshpb_pre_req_done(struct scsi_cmnd *cmd)
{
	blk_complete_request(cmd->request);
}

static inline unsigned long ufshpb_get_lpn(struct request *rq)
{
	return blk_rq_pos(rq) / SECTORS_PER_BLOCK;
}

static inline unsigned int ufshpb_get_len(struct request *rq)
{
	return blk_rq_sectors(rq) / SECTORS_PER_BLOCK;
}

static inline unsigned int ufshpb_is_unaligned(struct request *rq)
{
	return blk_rq_sectors(rq) % SECTORS_PER_BLOCK;
}

static inline int ufshpb_issue_ctx_id_ticket(struct ufshpb_lu *hpb)
{
	int hpb_ctx_id;

	hpb->ctx_id_ticket++;
	if (hpb->ctx_id_ticket >= MAX_HPB_CONTEXT_ID)
		hpb->ctx_id_ticket = 0;
	hpb_ctx_id = hpb->ctx_id_ticket;

	return hpb_ctx_id;
}

static inline void ufshpb_set_prefetch_cmd(unsigned char *cdb,
					   unsigned long lpn, unsigned int len,
					   int hpb_ctx_id)
{
	int len_byte = len * HPB_ENTRY_SIZE;

	cdb[0] = UFSHPB_WRITE_BUFFER;
	cdb[1] = UFSHPB_WB_ID_PREFETCH;
	cdb[2] = GET_BYTE_3(lpn);
	cdb[3] = GET_BYTE_2(lpn);
	cdb[4] = GET_BYTE_1(lpn);
	cdb[5] = GET_BYTE_0(lpn);
	cdb[6] = (1 << 7) | hpb_ctx_id;
	cdb[7] = GET_BYTE_1(len_byte);
	cdb[8] = GET_BYTE_0(len_byte);
	cdb[9] = 0x00;	/* Control = 0x00 */
}

static void ufshpb_mimic_scsi_release_buffers(struct scsi_cmnd *cmd)
{
	if (cmd->sdb.table.nents)
		sg_free_table_chained(&cmd->sdb.table, false);

	memset(&cmd->sdb, 0, sizeof(cmd->sdb));

	if (scsi_prot_sg_count(cmd))
		sg_free_table_chained(&cmd->prot_sdb->table, false);
}

static inline void ufshpb_mimic_scsi_dispatch_cmd(struct scsi_cmnd *cmd)
{
	atomic_inc(&cmd->device->iorequest_cnt);

	scsi_log_send(cmd);

	cmd->scsi_done = ufshpb_pre_req_done;
}

static int ufshpb_mimic_scsi_request_fn(struct ufshpb_lu *hpb,
					struct request *req)
{
	struct request_queue *q = req->q;
	struct scsi_device *sdev = q->queuedata;
	struct Scsi_Host *shost = sdev->host;
	struct scsi_cmnd *cmd;
	unsigned long flags;
	unsigned int busy;
	int ret = 0;

	spin_lock_irqsave(q->queue_lock, flags);
	req->rq_flags |= RQF_STARTED;

	ret = q->prep_rq_fn(q, req);
	if (unlikely(ret != BLKPREP_OK)) {
		ret = -EIO;
		goto prep_err;
	}
	cmd = req->special;
	if (unlikely(!cmd))
		BUG();

	busy = atomic_inc_return(&sdev->device_busy) - 1;
	if (busy >= sdev->queue_depth) {
		ret = -EIO;
		goto finish_cmd;
	}

	/* lh_pre_req_free list is dummy head for blk_dequeue_request() */
	list_add_tail(&req->queuelist, &hpb->lh_pre_req_dummy);
	ret = blk_queue_start_tag(q, req);
	if (ret) {
		list_del_init(&req->queuelist);
		ret = -EIO;
		goto finish_cmd;
	}
	spin_unlock_irqrestore(q->queue_lock, flags);

	/*
	 * UFS device has multi luns, so starget is not used.
	 * In case of UFS, starget->can_queue <= 0.
	 */
	if (unlikely(scsi_target(sdev)->can_queue > 0))
		atomic_inc(&scsi_target(sdev)->target_busy);
	atomic_inc(&shost->host_busy);

	ufshpb_init_cmd_errh(cmd);

	ufshpb_mimic_scsi_dispatch_cmd(cmd);

	return ret;
finish_cmd:
	ufshpb_mimic_scsi_release_buffers(cmd);
	scsi_put_command(cmd);
	put_device(&sdev->sdev_gendev);
	req->special = NULL;
	atomic_dec(&sdev->device_busy);
prep_err:
	spin_unlock_irqrestore(q->queue_lock, flags);
	return ret;
}

static int ufshpb_set_pre_req(struct ufshpb_lu *hpb, struct scsi_cmnd *cmd,
			      struct ufshpb_req *pre_req, int hpb_ctx_id)
{
	struct scsi_device *sdev = cmd->device;
	struct request_queue *q = sdev->request_queue;
	struct request *req;
	struct scsi_request *rq;
	struct scsi_cmnd *scmd;
	struct bio *bio = pre_req->bio;
	int ret = 0;

	pre_req->hpb = hpb;
	pre_req->wb.lpn = ufshpb_get_lpn(cmd->request);
	pre_req->wb.len = ufshpb_get_len(cmd->request);

	ret = ufshpb_pre_req_add_bio_page(q, pre_req);
	if (ret)
		return ret;

	req = pre_req->req;

	/*
	 * blk_init_rl() -> alloc_request_size().
	 * q->init_rq_fn = scsi_old_init_rq behavior.
	 */
	scmd = (struct scsi_cmnd *)(req + 1);
	memset(scmd, 0, sizeof(*scmd));
	scmd->sense_buffer = pre_req->sense;
	scmd->req.sense = scmd->sense_buffer;

	/* blk_get_request behavior */
	blk_rq_init(q, req);
	q->initialize_rq_fn(req);

	/* 1. request setup */
	blk_rq_append_bio(req, &bio);
	req->cmd_flags = REQ_OP_WRITE | REQ_SYNC | REQ_OP_SCSI_OUT;
	req->rq_flags = RQF_QUIET | RQF_PREEMPT;
	req->timeout = msecs_to_jiffies(30000);
	req->end_io_data = (void *)pre_req;
	req->end_io = ufshpb_pre_req_compl_fn;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	ufshpb_set_prefetch_cmd(rq->cmd, pre_req->wb.lpn, pre_req->wb.len,
				hpb_ctx_id);
	rq->cmd_len = scsi_command_size(rq->cmd);

	ret = ufshpb_mimic_scsi_request_fn(hpb, req);

	return ret;
}

static inline bool ufshpb_is_support_chunk(int transfer_len)
{
	return transfer_len <= HPB_MULTI_CHUNK_HIGH;
}

static int ufshpb_check_pre_req_cond(struct ufshpb_lu *hpb,
				     struct scsi_cmnd *cmd)
{
	struct request *rq = cmd->request;
	unsigned long flags;
	unsigned int transfer_len;
	unsigned int lpn;

	//no support Write buffer
	return -EINVAL;
	if (!ufshpb_is_read_cmd(cmd))
		return -EINVAL;

	if (ufshpb_is_unaligned(rq))
		return -EINVAL;

	transfer_len = ufshpb_get_len(rq);
	if (!transfer_len)
		return -EINVAL;

	if (!ufshpb_is_support_chunk(transfer_len))
		return -EINVAL;

	/*
	 * WRITE_BUFFER CMD support 36K (len=9) ~ 512K (len=128) default.
	 * it is possible to change range of transfer_len through sysfs.
	 */
	if (transfer_len < hpb->pre_req_min_tr_len ||
	    transfer_len > hpb->pre_req_max_tr_len)
		return -EINVAL;

	lpn = ufshpb_get_lpn(cmd->request);

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (ufshpb_ppn_dirty_check(hpb, lpn, transfer_len)) {
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	return 0;
}

void ufshpb_end_pre_req(struct ufsf_feature *ufsf, struct request *req)
{
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)(req + 1);

	set_host_byte(scmd, DID_OK);

	scmd->scsi_done(scmd);
}

int ufshpb_prepare_pre_req(struct ufsf_feature *ufsf, struct scsi_cmnd *cmd,
			   int lun)
{
	struct ufs_hba *hba = ufsf->hba;
	struct ufshpb_lu *hpb;
	struct ufshpb_req *pre_req;
	struct ufshcd_lrb *add_lrbp;
	struct ufshcd_lrb *orig_lrbp = &hba->lrb[cmd->request->tag];
	struct scsi_cmnd *pre_cmd;
	unsigned long flags;
	int add_tag, hpb_ctx_id;
	int ret = 0;

	/* WKLU could not be HPB-LU */
	if (!ufsf_is_valid_lun(lun))
		return -ENODEV;

	hpb = ufsf->ufshpb_lup[lun];
	ret = ufshpb_lu_get(hpb);
	if (ret)
		return ret;

	if (hpb->force_disable) {
		ret = -ENODEV;
		goto put_hpb;
	}

	ret = ufshpb_check_pre_req_cond(hpb, cmd);
	if (ret)
		goto put_hpb;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	pre_req = ufshpb_get_pre_req(hpb);
	if (!pre_req) {
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
		ret = -ENOMEM;
		goto put_hpb;
	}

	hpb_ctx_id = ufshpb_issue_ctx_id_ticket(hpb);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ret = ufshpb_set_pre_req(hpb, cmd, pre_req, hpb_ctx_id);
	if (ret)
		goto put_pre_req;

	add_tag = pre_req->req->tag;
	if (test_and_set_bit_lock(add_tag, &hba->lrb_in_use)) {
		ufshpb_end_pre_req(ufsf, pre_req->req);
		return -EIO;
	}

	add_lrbp = &hba->lrb[add_tag];
	WARN_ON(add_lrbp->cmd);

	pre_cmd = pre_req->req->special;
	add_lrbp->cmd = pre_cmd;
	add_lrbp->sense_bufflen = UFSHCD_REQ_SENSE_SIZE;
	add_lrbp->sense_buffer = pre_cmd->sense_buffer;
	add_lrbp->task_tag = add_tag;
	add_lrbp->lun = lun;
	add_lrbp->intr_cmd = !ufshcd_is_intr_aggr_allowed(hba) ? true : false;
	add_lrbp->req_abort_skip = false;

	/* MTK patch: reset crypto_enable */
	add_lrbp->crypto_enable = false;

	orig_lrbp->hpb_ctx_id = hpb_ctx_id;

	return add_tag;
put_pre_req:
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_pre_req(hpb, pre_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
put_hpb:
	ufshpb_lu_put(hpb);
	return ret;
}

int ufshpb_prepare_add_lrbp(struct ufsf_feature *ufsf, int add_tag)
{
	struct ufs_hba *hba = ufsf->hba;
	struct ufshcd_lrb *add_lrbp;
	struct scsi_cmnd *pre_cmd;
	int err = 0;

	add_lrbp = &hba->lrb[add_tag];


	err = ufshcd_hold(hba,false);
	if (err)
		goto hold_err;

	ufshcd_comp_scsi_upiu(hba, add_lrbp);

	err = ufshcd_map_sg(hba, add_lrbp);
	if (err)
		goto map_err;

	return 0;
map_err:
	ufshcd_release(hba);
hold_err:
	pre_cmd = add_lrbp->cmd;
	add_lrbp->cmd = NULL;
	clear_bit_unlock(add_tag, &hba->lrb_in_use);
	ufsf_hpb_end_pre_req(&hba->ufsf, pre_cmd->request);
	return -EIO;
}
static inline void ufshpb_update_lru_list(enum selection_type selection_type,
					  struct list_head *target_lh,
					  struct ufshpb_region *rgn)
{
	switch (selection_type) {
	case LRU:
		list_move_tail(&rgn->list_lru_rgn, target_lh);
		break;
	default:
		break;
	}
}

static void ufshpb_hit_lru_info(struct victim_select_info *lru_info,
				struct ufshpb_region *rgn)
{
	struct list_head *target_lh = NULL;

	if (rgn->rgn_state == HPBREGION_ACTIVE) {
		if (ufshpb_is_triggered_by_fs(rgn)) {
			rgn->rgn_state = HPBREGION_RT_PINNED;
			target_lh = &lru_info->lh_lru_rt_pinned_rgn;
		} else {
			target_lh = &lru_info->lh_lru_rgn;
		}
	} else if (rgn->rgn_state == HPBREGION_RT_PINNED) {
		target_lh = &lru_info->lh_lru_rt_pinned_rgn;
	}

	ufshpb_update_lru_list(lru_info->selection_type, target_lh, rgn);
}

static inline void ufshpb_act_rsp_list_add(struct ufshpb_lu *hpb,
					   struct ufshpb_subregion *srgn)
{
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_inc(&hpb->act_rsp_list_cnt);
#endif
	list_add_tail(&srgn->list_act_srgn, &hpb->lh_act_srgn);
}

static inline void ufshpb_act_rsp_list_del(struct ufshpb_lu *hpb,
					   struct ufshpb_subregion *srgn)
{
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_dec(&hpb->act_rsp_list_cnt);
#endif
	list_del_init(&srgn->list_act_srgn);
}

static inline void ufshpb_inact_rsp_list_add(struct ufshpb_lu *hpb,
					     struct ufshpb_region *rgn)
{
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_inc(&hpb->inact_rsp_list_cnt);
#endif
	list_add_tail(&rgn->list_inact_rgn, &hpb->lh_inact_rgn);
}

static inline void ufshpb_inact_rsp_list_del(struct ufshpb_lu *hpb,
					     struct ufshpb_region *rgn)
{
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_dec(&hpb->inact_rsp_list_cnt);
#endif
	list_del_init(&rgn->list_inact_rgn);
}

static void ufshpb_update_active_info(struct ufshpb_lu *hpb, int rgn_idx,
				      int srgn_idx, enum HPBUPDATE_INFO info)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;

	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;

	if (!list_empty(&rgn->list_inact_rgn))
		ufshpb_inact_rsp_list_del(hpb, rgn);

	if (list_empty(&srgn->list_act_srgn)) {
		ufshpb_act_rsp_list_add(hpb, srgn);
#if defined(CONFIG_HPB_DEBUG)
		if (info == HPBUPDATE_RT_FROM_FS) {
			HPB_DEBUG(hpb, "*** Noti [HOST]: RT SET %d - %d",
				  rgn_idx, srgn_idx);
			TMSG(hpb->ufsf, hpb->lun, "*** Noti [HOST]: RT SET %d - %d",
			     rgn_idx, srgn_idx);
		} else if (info == HPBUPDATE_FROM_DEV) {
			HPB_DEBUG(hpb, "*** Noti [DEV]: ACT %d - %d",
				  rgn_idx, srgn_idx);
			TMSG(hpb->ufsf, hpb->lun, "*** Noti [DEV]: ACT %d - %d",
			     rgn_idx, srgn_idx);
		}
#endif
	}
}

static void ufshpb_update_inactive_info(struct ufshpb_lu *hpb, int rgn_idx,
					enum HPBUPDATE_INFO info)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	rgn = hpb->rgn_tbl + rgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!list_empty(&srgn->list_act_srgn))
			ufshpb_act_rsp_list_del(hpb, srgn);
	}

	if (list_empty(&rgn->list_inact_rgn)) {
		ufshpb_inact_rsp_list_add(hpb, rgn);
#if defined(CONFIG_HPB_DEBUG)
		if (info == HPBUPDATE_RT_FROM_FS) {
			HPB_DEBUG(hpb, "*** Noti [HOST]: RT UNSET %d", rgn_idx);
			TMSG(hpb->ufsf, hpb->lun, "*** Noti [HOST]: RT UNSET %d",
			     rgn_idx);
		} else if (info == HPBUPDATE_FROM_DEV) {
			HPB_DEBUG(hpb, "*** Noti [DEV]: INACT %d", rgn_idx);
			TMSG(hpb->ufsf, hpb->lun, "*** Noti [DEV]: INACT %d",
			     rgn_idx);
		} else {
			HPB_DEBUG(hpb, "*** ERR : INACT %d", rgn_idx);
			TMSG(hpb->ufsf, hpb->lun, "*** ERR [DEV]: INACT %d",
			     rgn_idx);
		}
#endif
	}
}

static void ufshpb_update_active_info_by_read(struct ufshpb_lu *hpb,
					      struct scsi_cmnd *cmd)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct request *rq = cmd->request;
	int s_rgn_idx, e_rgn_idx, rgn_idx, srgn_idx;
	bool is_update = false;
	unsigned int start_lpn, end_lpn;
	unsigned long flags;
	struct victim_select_info *lru_info = &hpb->lru_info;
	//long long miss_after_region_full,hit_after_region_full,total_access_region_after_region_full;

	start_lpn = ufshpb_get_lpn(rq);
	end_lpn = start_lpn + ufshpb_get_len(rq) - 1;

	s_rgn_idx = start_lpn >> hpb->entries_per_rgn_shift;
	e_rgn_idx = end_lpn >> hpb->entries_per_rgn_shift;

	for (rgn_idx = s_rgn_idx; rgn_idx <= e_rgn_idx; rgn_idx++) {
		rgn = hpb->rgn_tbl + rgn_idx;
		for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
			is_update = false;
			srgn = rgn->srgn_tbl + srgn_idx;
			spin_lock_irqsave(&hpb->hpb_lock, flags);
			//&& srgn->srgn_state != HPBSUBREGION_CLEAN
		    if (rgn->rgn_state == HPBREGION_INACTIVE ||rgn->rgn_state == HPBREGION_ACTIVE) {
				rgn->active_threshold = UFSHPB_HOST_MAX_ACTIVE_THRESHOLD >> (atomic64_read(&rgn->last_hit_cnt)/atomic64_read(&rgn->last_miss_cnt));
				if (rgn->active_threshold < UFSHPB_HOST_MIN_ACTIVE_THRESHOLD) {
					rgn->active_threshold = UFSHPB_HOST_MIN_ACTIVE_THRESHOLD;
				}
				atomic64_inc(&srgn->read_cnt);
				if (atomic64_read(&srgn->read_cnt) > rgn->active_threshold) {
					is_update = true;
					atomic_set(&rgn->reason, HPBUPDATE_RT_FROM_FS);
				}
			}
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			if (is_update) {
#if defined(CONFIG_HPB_DEBUG)
				TMSG(hpb->ufsf, hpb->lun, "%llu + %u set rt_pin %d",
					 (unsigned long long) blk_rq_pos(rq),
					 (unsigned int) blk_rq_sectors(rq), rgn_idx);
#endif
				//printk("***active region :%d ****\n",rgn_idx);
				spin_lock_irqsave(&hpb->rsp_list_lock, flags);
				ufshpb_update_active_info(hpb, rgn_idx,srgn_idx,HPBUPDATE_RT_FROM_FS);
				spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
			}
		}
	}
}

static void ufshpb_update_inactive_info_by_write_discard(struct ufshpb_lu *hpb,
						struct scsi_cmnd *cmd)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct request *rq = cmd->request;
	int s_rgn_idx, s_srgn_idx, s_offset;
	int e_rgn_idx, e_srgn_idx, e_offset;
	int rgn_idx,srgn_idx;
	bool is_update = false;
	unsigned int start_lpn, end_lpn, length;
	unsigned long flags;
	struct victim_select_info *lru_info = &hpb->lru_info;

	start_lpn = ufshpb_get_lpn(rq);
	end_lpn = start_lpn + ufshpb_get_len(rq) - 1;
	length = ufshpb_get_len(rq);
	ufshpb_get_pos_from_lpn(hpb, start_lpn, &s_rgn_idx, &s_srgn_idx,
				&s_offset);
	ufshpb_get_pos_from_lpn(hpb, end_lpn, &e_rgn_idx, &e_srgn_idx,
				&e_offset);
	rgn= hpb->rgn_tbl + s_rgn_idx;
	 if ((ufshpb_is_discard_cmd(cmd) || (ufshpb_is_write_cmd(cmd))) && (rgn->rgn_state == HPBREGION_RT_PINNED)) {
		if (e_rgn_idx == s_rgn_idx) {
			is_update = false;
			rgn= hpb->rgn_tbl + s_rgn_idx;
			spin_lock_irqsave(&hpb->hpb_lock, flags);
			rgn->discard_write_size += length;
			if (atomic64_read(&rgn->last_hit_cnt) > UFSHPB_HOST_HIT_CNT_THRESHOLD2) {
				rgn->deactive_threshold = UFSHPB_HOST_MIN_DEACTIVE_THRESHOLD *(1<< (atomic64_read(&rgn->last_hit_cnt)/atomic64_read(&rgn->last_miss_cnt)));
				if (rgn->deactive_threshold > UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD) {
					rgn->deactive_threshold = UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD;
				}
			} else {
				rgn->deactive_threshold = UFSHPB_HOST_MID_DEACTIVE_THRESHOLD *(1<< (atomic64_read(&rgn->last_hit_cnt)/atomic64_read(&rgn->last_miss_cnt)));
				if (rgn->deactive_threshold > UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD) {
					rgn->deactive_threshold = UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD;
				}
			}
			if (rgn->discard_write_size > rgn->deactive_threshold) {
				is_update = true;
				atomic_set(&rgn->reason, HPBUPDATE_RT_FROM_FS);
			}
			if (ufshpb_is_discard_cmd(cmd)) {
				rgn->total_unmap_count += length;
			} else {
				rgn->total_write_count += length;
			}
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);

			if (is_update) {
#if defined(CONFIG_HPB_DEBUG)
				TMSG(hpb->ufsf, hpb->lun, "%llu + %u unset rt_pin %d",
					 (unsigned long long) blk_rq_pos(rq),
					 (unsigned int) blk_rq_sectors(rq), s_rgn_idx);
#endif
				printk("***de-active region :%d ****\n",s_rgn_idx);
				spin_lock_irqsave(&hpb->rsp_list_lock, flags);
				ufshpb_update_inactive_info(hpb, s_rgn_idx,
								HPBUPDATE_RT_FROM_FS);
				spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
			}
		} else {
			for (rgn_idx = s_rgn_idx; rgn_idx <= e_rgn_idx; rgn_idx++) {
				rgn = hpb->rgn_tbl + rgn_idx;
				is_update = false;
				spin_lock_irqsave(&hpb->hpb_lock, flags);
				if (rgn_idx == s_rgn_idx) {
					rgn->discard_write_size += ((hpb->srgn_unit_size / BLOCK) - s_offset);
					if (ufshpb_is_discard_cmd(cmd)) {
						rgn->total_unmap_count += ((hpb->srgn_unit_size / BLOCK) - s_offset);
					} else {
						rgn->total_write_count += ((hpb->srgn_unit_size / BLOCK) - s_offset);
					}
				} else if (rgn_idx == e_rgn_idx) {
					rgn->discard_write_size +=  e_offset;
					if (ufshpb_is_discard_cmd(cmd)) {
						rgn->total_unmap_count += e_offset;
					} else {
						rgn->total_write_count += e_offset;
					}
				} else {
					rgn->discard_write_size +=  (hpb->srgn_unit_size / BLOCK);
					if (ufshpb_is_discard_cmd(cmd)) {
						rgn->total_unmap_count += (hpb->srgn_unit_size / BLOCK);
					} else {
						rgn->total_write_count += (hpb->srgn_unit_size / BLOCK);
					}
				}
			if (atomic64_read(&rgn->last_hit_cnt) > UFSHPB_HOST_HIT_CNT_THRESHOLD2) {
				rgn->deactive_threshold = UFSHPB_HOST_MIN_DEACTIVE_THRESHOLD *(1<< (atomic64_read(&rgn->last_hit_cnt)/atomic64_read(&rgn->last_miss_cnt)));
				if (rgn->deactive_threshold > UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD) {
					rgn->deactive_threshold = UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD;
				}
			} else {
				rgn->deactive_threshold = UFSHPB_HOST_MID_DEACTIVE_THRESHOLD *(1<< (atomic64_read(&rgn->last_hit_cnt)/atomic64_read(&rgn->last_miss_cnt)));
				if (rgn->deactive_threshold > UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD) {
					rgn->deactive_threshold = UFSHPB_HOST_MAX_DEACTIVE_THRESHOLD;
				}
			}

			if (rgn->discard_write_size > rgn->deactive_threshold) {
				is_update = true;
				atomic_set(&rgn->reason, HPBUPDATE_RT_FROM_FS);
			}

			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			if (is_update) {
#if defined(CONFIG_HPB_DEBUG)
				TMSG(hpb->ufsf, hpb->lun, "%llu + %u unset rt_pin %d",
					 (unsigned long long) blk_rq_pos(rq),
					 (unsigned int) blk_rq_sectors(rq), rgn_idx);
#endif
				printk("***de-active region :%d ****\n",rgn_idx);
				spin_lock_irqsave(&hpb->rsp_list_lock, flags);
				ufshpb_update_inactive_info(hpb, rgn_idx,
								HPBUPDATE_RT_FROM_FS);
				spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
            }
        }
    }
  }
}

#if defined(CONFIG_HPB_DEBUG_SYSFS)
static void ufshpb_increase_hit_count(struct ufshpb_lu *hpb,
				      struct ufshpb_region *rgn,
				      int transfer_len)
{
	if (rgn->rgn_state == HPBREGION_PINNED ||
	    rgn->rgn_state == HPBREGION_RT_PINNED) {
		if (transfer_len < HPB_MULTI_CHUNK_LOW) {
			atomic64_inc(&hpb->pinned_low_hit);
			atomic64_inc(&rgn->rgn_pinned_low_hit);
		} else {
			atomic64_inc(&hpb->pinned_high_hit);
			atomic64_inc(&rgn->rgn_pinned_high_hit);
		}
	} else if (rgn->rgn_state == HPBREGION_ACTIVE) {
		if (transfer_len < HPB_MULTI_CHUNK_LOW) {
			atomic64_inc(&hpb->active_low_hit);
			atomic64_inc(&rgn->rgn_active_low_hit);
		} else {
			atomic64_inc(&hpb->active_high_hit);
			atomic64_inc(&rgn->rgn_active_high_hit);
		}
	}
}

static void ufshpb_increase_miss_count(struct ufshpb_lu *hpb,
				       struct ufshpb_region *rgn,
				       int transfer_len)
{
	if (rgn->rgn_state == HPBREGION_PINNED ||
	    rgn->rgn_state == HPBREGION_RT_PINNED) {
		if (transfer_len < HPB_MULTI_CHUNK_LOW) {
			atomic64_inc(&hpb->pinned_low_miss);
			atomic64_inc(&rgn->rgn_pinned_low_miss);
		} else {
			atomic64_inc(&hpb->pinned_high_miss);
			atomic64_inc(&rgn->rgn_pinned_high_miss);
		}
	} else if (rgn->rgn_state == HPBREGION_ACTIVE) {
		if (transfer_len < HPB_MULTI_CHUNK_LOW) {
			atomic64_inc(&hpb->active_low_miss);
			atomic64_inc(&rgn->rgn_active_low_miss);
		} else {
			atomic64_inc(&hpb->active_high_miss);
			atomic64_inc(&rgn->rgn_active_high_miss);
		}
	} else if (rgn->rgn_state == HPBREGION_INACTIVE) {
		if (transfer_len < HPB_MULTI_CHUNK_LOW) {
			atomic64_inc(&hpb->inactive_low_read);
			atomic64_inc(&rgn->rgn_inactive_low_read);
		} else {
			atomic64_inc(&hpb->inactive_high_read);
			atomic64_inc(&rgn->rgn_inactive_high_read);
		}
	}
}

static void ufshpb_increase_rb_count(struct ufshpb_lu *hpb,
				struct ufshpb_region *rgn)
{
	if (rgn->rgn_state == HPBREGION_PINNED ||
		rgn->rgn_state == HPBREGION_RT_PINNED) {
		atomic64_inc(&hpb->pinned_rb_cnt);
		atomic64_inc(&rgn->rgn_pinned_rb_cnt);
       } else if (rgn->rgn_state == HPBREGION_ACTIVE) {
		atomic64_inc(&hpb->active_rb_cnt);
		atomic64_inc(&rgn->rgn_active_rb_cnt);
       }
}

static void ufshpb_debug_sys_init(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn;
	int rgn_idx;

	atomic64_set(&hpb->pinned_low_hit, 0);
	atomic64_set(&hpb->pinned_high_hit, 0);
	atomic64_set(&hpb->pinned_low_miss, 0);
	atomic64_set(&hpb->pinned_high_miss, 0);
	atomic64_set(&hpb->active_low_hit, 0);
	atomic64_set(&hpb->active_high_hit, 0);
	atomic64_set(&hpb->active_low_miss, 0);
	atomic64_set(&hpb->active_high_miss, 0);
	atomic64_set(&hpb->inactive_low_read, 0);
	atomic64_set(&hpb->inactive_high_read, 0);
	atomic64_set(&hpb->pinned_rb_cnt, 0);
	atomic64_set(&hpb->active_rb_cnt, 0);

	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		rgn = hpb->rgn_tbl + rgn_idx;
		atomic64_set(&rgn->rgn_pinned_low_hit, 0);
		atomic64_set(&rgn->rgn_pinned_high_hit, 0);
		atomic64_set(&rgn->rgn_pinned_low_miss, 0);
		atomic64_set(&rgn->rgn_pinned_high_miss, 0);
		atomic64_set(&rgn->rgn_active_low_hit, 0);
		atomic64_set(&rgn->rgn_active_high_hit, 0);
		atomic64_set(&rgn->rgn_active_low_miss, 0);
		atomic64_set(&rgn->rgn_active_high_miss, 0);
		atomic64_set(&rgn->rgn_inactive_low_read, 0);
		atomic64_set(&rgn->rgn_inactive_high_read, 0);
		atomic64_set(&rgn->rgn_pinned_rb_cnt, 0);
		atomic64_set(&rgn->rgn_active_rb_cnt, 0);
	}
}
#endif

/* routine : READ10 -> HPB_READ  */
void ufshpb_prep_fn(struct ufsf_feature *ufsf, struct ufshcd_lrb *lrbp)
{
	struct ufshpb_lu *hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct request *rq;
	struct scsi_cmnd *cmd;
	unsigned long long ppn = 0;
	unsigned long lpn, flags;
	int transfer_len = TRANSFER_LEN;
	int rgn_idx, srgn_idx, srgn_offset, ret, error = 0;

	/* WKLU could not be HPB-LU */
	if (!lrbp || !ufsf_is_valid_lun(lrbp->lun))
		return;

	cmd = lrbp->cmd;
	if (!ufshpb_is_read_cmd(cmd) && !ufshpb_is_write_cmd(cmd) &&
	    !ufshpb_is_discard_cmd(cmd))
		return;

	rq = cmd->request;
	hpb = ufsf->ufshpb_lup[lrbp->lun];
	ret = ufshpb_lu_get(hpb);
	if (ret)
		return;

	if (hpb->force_disable) {
#if defined(CONFIG_HPB_DEBUG)
		if (ufshpb_is_read_cmd(cmd))
			TMSG(ufsf, hpb->lun, "%llu + %u READ_10",
			     (unsigned long long) blk_rq_pos(rq),
			     (unsigned int) blk_rq_sectors(rq));
#endif
		goto put_hpb;
	}

	lpn = ufshpb_get_lpn(rq);
	ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx, &srgn_offset);
	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;
	/*
	 * If cmd type is WRITE, bitmap set to dirty.
	 */
	if (ufshpb_is_write_cmd(cmd) || ufshpb_is_discard_cmd(cmd)) {
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		if (rgn->rgn_state != HPBREGION_INACTIVE) {
			ufshpb_set_dirty(hpb, lrbp, rgn_idx, srgn_idx,
					 srgn_offset);
		}
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	}

	if(ufshpb_is_host_control_mode(hpb))
	{
		if (ufshpb_is_read_cmd(cmd))
			ufshpb_update_active_info_by_read(hpb, cmd);
		else if(ufshpb_is_discard_cmd(cmd) || ufshpb_is_write_cmd(cmd))
			ufshpb_update_inactive_info_by_write_discard(hpb, cmd);
	}

	if (!ufshpb_is_read_cmd(cmd))
		goto put_hpb;

	if (ufshpb_is_unaligned(rq)) {
#if defined(CONFIG_HPB_DEBUG)
		TMSG_CMD(hpb, "READ_10 not aligned 4KB", rq, rgn_idx, srgn_idx);
#endif
		goto put_hpb;
	}

	transfer_len = ufshpb_get_len(rq);
	if (!transfer_len)
		goto put_hpb;

	if (!ufshpb_is_support_chunk(transfer_len)) {
#if defined(CONFIG_HPB_DEBUG)
		TMSG_CMD(hpb, "READ_10 doesn't support chunk size",
			 rq, rgn_idx, srgn_idx);
#endif
		goto put_hpb;
	}

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (ufshpb_ppn_dirty_check(hpb, lpn, transfer_len)) {
		atomic64_inc(&hpb->miss);
		if (rgn->rgn_state == HPBREGION_PINNED ||
	        rgn->rgn_state == HPBREGION_RT_PINNED ||
			rgn->rgn_state == HPBREGION_ACTIVE) {
			atomic64_inc(&rgn->last_miss_cnt);
			atomic64_inc(&rgn->total_miss_cnt);
		}

		/*if(atomic64_read(&hpb->lru_info.active_cnt) == hpb->lru_info.max_lru_active_cnt) {
			atomic64_inc(&hpb->miss_after_region_full);
		}*/
#if defined(CONFIG_HPB_DEBUG_SYSFS)
		ufshpb_increase_miss_count(hpb, rgn, transfer_len);
#endif
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
#if defined(CONFIG_HPB_DEBUG)
		TMSG_CMD(hpb, "READ_10 E_D", rq, rgn_idx, srgn_idx);
#endif
		goto put_hpb;
	}

	ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	if (error) {
		ERR_MSG("get_ppn failed.. err %d region %d subregion %d",
			error, rgn_idx, srgn_idx);
		ufshpb_lu_put(hpb);
		goto wakeup_ee_worker;
	}

	ufshpb_set_read16_cmd(hpb, lrbp, ppn, transfer_len);
#if defined(CONFIG_HPB_DEBUG)
	TMSG(ufsf, hpb->lun, "%llu + %u HPB_READ %d - %d context_id %d",
	     (unsigned long long) blk_rq_pos(rq),
	     (unsigned int) blk_rq_sectors(rq), rgn_idx,
	     srgn_idx, lrbp->hpb_ctx_id);
#endif

	atomic64_inc(&hpb->hit);
	atomic64_inc(&rgn->total_hit_cnt);
	atomic64_inc(&rgn->last_hit_cnt);
	/*if(atomic64_read(&hpb->lru_info.active_cnt) == hpb->lru_info.max_lru_active_cnt) {
			atomic64_inc(&hpb->hit_after_region_full);
	}*/
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	ufshpb_increase_hit_count(hpb, rgn, transfer_len);
#endif
put_hpb:
	ufshpb_lu_put(hpb);
	return;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
}

static inline void ufshpb_put_map_req(struct ufshpb_lu *hpb,
				      struct ufshpb_req *map_req)
{
	list_add_tail(&map_req->list_req, &hpb->lh_map_req_free);
	hpb->num_inflight_map_req--;
	hpb->num_inflight_rt_req--;
}

static struct ufshpb_req *ufshpb_get_map_req(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *map_req;

	if (hpb->num_inflight_map_req >= hpb->throttle_map_req ||
	    hpb->num_inflight_rt_req >= hpb->throttle_rt_req) {
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "map_req throttle. inflight %d throttle %d",
			  hpb->num_inflight_map_req, hpb->throttle_map_req);
		HPB_DEBUG(hpb, "rt_req throttle. rt inflight %d rt throttle %d",
			  hpb->num_inflight_rt_req, hpb->throttle_rt_req);
#endif
		return NULL;
	}

	map_req = list_first_entry_or_null(&hpb->lh_map_req_free,
					   struct ufshpb_req, list_req);
	if (!map_req) {
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "There is no map_req");
#endif
		return NULL;
	}

	list_del_init(&map_req->list_req);
	hpb->num_inflight_map_req++;
	hpb->num_inflight_rt_req++;

	return map_req;
}

static int ufshpb_clean_dirty_bitmap(struct ufshpb_lu *hpb,
				     struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPBREGION_INACTIVE) {
		ERR_MSG("region[%d] was activated. But it's state is changed",
			rgn->rgn_idx);
		return -EINVAL;
	}

	memset(srgn->mctx->ppn_dirty, 0x00,
	       hpb->entries_per_srgn >> bits_per_byte_shift);

	return 0;
}

static void ufshpb_clean_active_subregion(struct ufshpb_lu *hpb,
					  struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPBREGION_INACTIVE) {
		ERR_MSG("region[%d] was activated. But it's state is changed",
			rgn->rgn_idx);
		return;
	}
	srgn->srgn_state = HPBSUBREGION_CLEAN;
}

static void ufshpb_error_active_subregion(struct ufshpb_lu *hpb,
					  struct ufshpb_subregion *srgn)
{
	struct ufshpb_region *rgn;

	BUG_ON(!srgn->mctx);

	rgn = hpb->rgn_tbl + srgn->rgn_idx;

	if (rgn->rgn_state == HPBREGION_INACTIVE) {
		ERR_MSG("region[%d] was activated. But it's state is changed",
			rgn->rgn_idx);
		return;
	}
	srgn->srgn_state = HPBSUBREGION_DIRTY;
}

#if defined(CONFIG_HPB_DEBUG)
static void ufshpb_check_ppn(struct ufshpb_lu *hpb, int rgn_idx, int srgn_idx,
			     struct ufshpb_map_ctx *mctx, const char *str)
{
	struct ufshpb_region *rgn = hpb->rgn_tbl + rgn_idx;
	int error = 0;
	unsigned long long val[2];

	BUG_ON(!mctx);

	val[0] = ufshpb_get_ppn(mctx, 0, &error);
	if (!error)
		val[1] = ufshpb_get_ppn(mctx, hpb->entries_per_srgn - 1,
					&error);
	if (error)
		val[0] = val[1] = 0;

	HPB_DEBUG(hpb, "%s READ BUFFER %d - %d ( %llx ~ %llx ) rgn_state %d",
		  str, rgn_idx, srgn_idx, val[0], val[1], rgn->rgn_state);
}
#endif
static bool is_clear_reason(struct ufshpb_lu *hpb, struct ufshpb_region *rgn)
{
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	bool ret = true;
	int srgn_idx;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	if (!list_empty(&rgn->list_inact_rgn))
		ret = false;
	else {
		for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
			srgn = rgn->srgn_tbl + srgn_idx;

			if (!list_empty(&srgn->list_act_srgn)) {
				ret = false;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

	return ret;
}

static void ufshpb_map_compl_process(struct ufshpb_req *map_req)
{
	struct ufshpb_lu *hpb = map_req->hpb;
	struct ufshpb_region *rgn = hpb->rgn_tbl + map_req->rgn_idx;
	struct ufshpb_subregion *srgn = rgn->srgn_tbl + map_req->srgn_idx;
	unsigned long flags;
	int i = 0;

#if defined(CONFIG_HPB_DEBUG)
	if (hpb->debug)
		ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx, srgn->mctx,
				 "COMPL");

	TMSG(hpb->ufsf, hpb->lun, "Noti: C RB %d - %d", map_req->rgn_idx,
	     map_req->srgn_idx);
#endif
	printk("***compl active region :%d ****\n",map_req->rgn_idx);
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_clean_active_subregion(hpb, srgn);
	atomic64_set(&srgn->read_cnt, 0);
	atomic64_inc(&rgn->active_cnt);
	atomic64_set(&rgn->last_hit_cnt, 0);
	atomic64_set(&rgn->last_miss_cnt, 0);
	atomic64_set(&rgn->last_write_cnt, 0);
	rgn->discard_write_size = 0;
	/*if(atomic64_read(&hpb->lru_info.active_cnt) == hpb->lru_info.max_lru_active_cnt)
	{
			atomic64_set(&hpb->hit_after_region_full,0);
			atomic64_set(&hpb->miss_after_region_full,0);
			atomic64_set(&hpb->total_access_region_after_region_full,0);
			memset(&hpb->access_region, 0, 1024);// will replace by bitmap
	}*/
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	if (is_clear_reason(hpb, rgn))
		atomic_set(&rgn->reason, HPBUPDATE_NONE);
}

static int ufshpb_map_req_error(struct ufshpb_req *map_req)
{
	struct ufshpb_lu *hpb = map_req->hpb;
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	struct scsi_sense_hdr sshdr;
	unsigned long flags;

	rgn = hpb->rgn_tbl + map_req->rgn_idx;
	srgn = rgn->srgn_tbl + map_req->srgn_idx;

	scsi_normalize_sense(map_req->sense, SCSI_SENSE_BUFFERSIZE, &sshdr);

	ERR_MSG("code %x sense_key %x asc %x ascq %x", sshdr.response_code,
		sshdr.sense_key, sshdr.asc, sshdr.ascq);
	ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x", sshdr.byte4,
		sshdr.byte5, sshdr.byte6, sshdr.additional_length);

	if (sshdr.sense_key != ILLEGAL_REQUEST)
		return 0;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (rgn->rgn_state == HPBREGION_PINNED) {
		if (sshdr.asc == 0x06 && sshdr.ascq == 0x01) {
#if defined(CONFIG_HPB_DEBUG)
			HPB_DEBUG(hpb, "retry pinned rb %d - %d",
				  map_req->rgn_idx, map_req->srgn_idx);
#endif
			list_add_tail(&map_req->list_req,
				      &hpb->lh_map_req_retry);
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);

			schedule_delayed_work(&hpb->ufshpb_retry_work,
					      msecs_to_jiffies(RETRY_DELAY_MS));
			return -EAGAIN;
		}
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "pinned rb %d - %d(dirty)",
			  map_req->rgn_idx, map_req->srgn_idx);
#endif
		ufshpb_error_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	} else {
		ufshpb_error_active_subregion(hpb, srgn);

		spin_unlock_irqrestore(&hpb->hpb_lock, flags);

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
		ufshpb_update_inactive_info(hpb, map_req->rgn_idx,
					    HPBUPDATE_NONE);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "Non-pinned rb %d will be inactive",
			  map_req->rgn_idx);
#endif
	}

	return 0;
}

#ifdef CONFIG_PM
static inline void ufshpb_mimic_blk_pm_put_request(struct request *rq)
{
	if (rq->q->dev && !(rq->rq_flags & RQF_PM) && !--rq->q->nr_pending)
		pm_runtime_mark_last_busy(rq->q->dev);
}
#endif

/* routine : map_req compl */
static void ufshpb_map_req_compl_fn(struct request *req, blk_status_t error)
{
	struct ufshpb_req *map_req = (struct ufshpb_req *) req->end_io_data;
	struct ufshpb_lu *hpb = map_req->hpb;
	unsigned long flags;
	int ret;

#ifdef CONFIG_PM
	ufshpb_mimic_blk_pm_put_request(req);
#endif
	if (hpb->ufsf->ufshpb_state != HPB_PRESENT)
		goto free_map_req;

	if (error) {
		ERR_MSG("COMP_NOTI: RB number %d ( %d - %d )", error,
			map_req->rgn_idx, map_req->srgn_idx);

		ret = ufshpb_map_req_error(map_req);
		if (ret)
			goto retry_map_req;
	} else
		ufshpb_map_compl_process(map_req);

free_map_req:
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	ufshpb_put_map_req(map_req->hpb, map_req);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
retry_map_req:
	scsi_device_put(hpb->ufsf->sdev_ufs_lu[hpb->lun]);
	ufshpb_lu_put(hpb);
}

static inline int ufshpb_get_scsi_device(struct ufs_hba *hba,
					 struct scsi_device *sdev)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(hba->host->host_lock, flags);
	ret = scsi_device_get(sdev);
	if (!ret && !scsi_device_online(sdev)) {
		spin_unlock_irqrestore(hba->host->host_lock, flags);
		scsi_device_put(sdev);
		WARNING_MSG("scsi_device_get failed.. ret %d", ret);
		return -ENODEV;
	}
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	return 0;
}

static int ufshpb_execute_dev_ctx_req(struct ufshpb_lu *hpb, unsigned char *cdb,
				      void *buf, int len)
{
	struct scsi_sense_hdr sshdr;
	struct scsi_device *sdev;
	struct ufsf_feature *ufsf = hpb->ufsf;
	int ret = 0;

	sdev = ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		WARNING_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	ret = ufshpb_get_scsi_device(ufsf->hba, sdev);
	if (ret)
		return ret;

	ufsf->issue_ioctl = true;

	ret = scsi_execute(sdev, cdb, DMA_FROM_DEVICE, buf, len, NULL, &sshdr,
			   msecs_to_jiffies(30000), 3, 0, 0, NULL);

	ufsf->issue_ioctl = false;

	scsi_device_put(sdev);

	return ret;
}

static inline void ufshpb_set_read_dev_ctx(unsigned char *cdb, int lba, int len)
{
	cdb[0] = READ_10;
	cdb[1] = 0x02;
	cdb[2] = GET_BYTE_3(lba);
	cdb[3] = GET_BYTE_2(lba);
	cdb[4] = GET_BYTE_1(lba);
	cdb[5] = GET_BYTE_0(lba);
	cdb[6] = GET_BYTE_2(len);
	cdb[7] = GET_BYTE_1(len);
	cdb[8] = GET_BYTE_0(len);
}

int ufshpb_issue_req_dev_ctx(struct ufshpb_lu *hpb, unsigned char *buf,
			     int buf_len)
{
	unsigned char cdb[10] = { 0 };
	int cmd_len = buf_len >> OS_PAGE_SHIFT;
	int ret = 0;

	ufshpb_set_read_dev_ctx(cdb, READ10_DEBUG_LBA, cmd_len);

	ret = ufshpb_execute_dev_ctx_req(hpb, cdb, buf, buf_len);

	if (ret < 0)
		ERR_MSG("failed with err %d", ret);

	return ret;
}

static inline void ufshpb_set_read_buf_cmd(unsigned char *cdb, int rgn_idx,
					   int srgn_idx, int srgn_mem_size)
{
	cdb[0] = UFSHPB_READ_BUFFER;
	cdb[1] = UFSHPB_RB_ID_READ;
	cdb[2] = GET_BYTE_1(rgn_idx);
	cdb[3] = GET_BYTE_0(rgn_idx);
	cdb[4] = GET_BYTE_1(srgn_idx);
	cdb[5] = GET_BYTE_0(srgn_idx);
	cdb[6] = GET_BYTE_2(srgn_mem_size);
	cdb[7] = GET_BYTE_1(srgn_mem_size);
	cdb[8] = GET_BYTE_0(srgn_mem_size);
	cdb[9] = 0x00;
	printk("******rgn_idx:%d, srgn_idx:%d, mem size:%d\n",rgn_idx,srgn_idx,srgn_mem_size);
}

static inline void ufshpb_set_rt_cmd(unsigned char *cdb, int rgn_idx,
				     int srgn_idx, int srgn_mem_size)
{
	cdb[0] = UFSHPB_READ_BUFFER;
	cdb[1] = UFSHPB_RB_ID_READ;
	cdb[2] = GET_BYTE_1(rgn_idx);
	cdb[3] = GET_BYTE_0(rgn_idx);
	cdb[4] = GET_BYTE_1(srgn_idx);
	cdb[5] = GET_BYTE_0(srgn_idx);
	cdb[6] = GET_BYTE_2(srgn_mem_size);
	cdb[7] = GET_BYTE_1(srgn_mem_size);
	cdb[8] = GET_BYTE_0(srgn_mem_size);
	cdb[9] = 0x00;
}

static int ufshpb_map_req_add_bio_page(struct ufshpb_lu *hpb,
				       struct request_queue *q, struct bio *bio,
				       struct ufshpb_map_ctx *mctx)
{
	struct page *page = NULL;
	int i, ret = 0;

	bio_reset(bio);

	for (i = 0; i < hpb->mpages_per_srgn; i++) {
		/* virt_to_page(p + (OS_PAGE_SIZE * i)); */
		page = mctx->m_page[i];
		if (!page)
			return -ENOMEM;

		ret = bio_add_pc_page(q, bio, page, hpb->mpage_bytes, 0);

		if (ret != hpb->mpage_bytes) {
			ERR_MSG("bio_add_pc_page fail: %d", ret);
			return -ENOMEM;
		}
	}

	return 0;
}

static int ufshpb_execute_map_req(struct ufshpb_lu *hpb,
				  struct scsi_device *sdev,
				  struct ufshpb_req *map_req)
{
	struct request_queue *q = sdev->request_queue;
	struct request *req;
	struct scsi_request *rq;
	struct scsi_cmnd *scmd;
	struct bio *bio = map_req->bio;
	struct ufshpb_region *rgn = hpb->rgn_tbl + map_req->rgn_idx;
	int ret;

	ret = ufshpb_map_req_add_bio_page(hpb, q, bio, map_req->rb.mctx);
	if (ret)
		return ret;

	req = map_req->req;

	/*
	 * blk_init_rl() -> alloc_request_size().
	 * q->init_rq_fn = scsi_old_init_rq behavior.
	 */
	scmd = (struct scsi_cmnd *)(req + 1);
	memset(scmd, 0, sizeof(*scmd));
	scmd->sense_buffer = map_req->sense;
	scmd->req.sense = scmd->sense_buffer;

	/* blk_get_request behavior */
	blk_rq_init(q, req);
	q->initialize_rq_fn(req);

	/* 1. request setup */
	blk_rq_append_bio(req, &bio); /* req->__data_len is setted */
	req->cmd_flags = REQ_OP_READ | REQ_OP_SCSI_IN;
	req->rq_flags = RQF_QUIET | RQF_PREEMPT;
	req->timeout = msecs_to_jiffies(30000);
	req->end_io_data = (void *)map_req;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	if (ufshpb_is_triggered_by_fs(rgn))
		ufshpb_set_rt_cmd(rq->cmd, map_req->rgn_idx, map_req->srgn_idx,
				  hpb->srgn_mem_size);
	else
		ufshpb_set_read_buf_cmd(rq->cmd, map_req->rgn_idx,
					map_req->srgn_idx, hpb->srgn_mem_size);
	rq->cmd_len = scsi_command_size(rq->cmd);

#if defined(CONFIG_HPB_DEBUG)
	if (hpb->debug)
		ufshpb_check_ppn(hpb, map_req->rgn_idx, map_req->srgn_idx,
				 map_req->rb.mctx, "ISSUE");

	TMSG(hpb->ufsf, hpb->lun, "Noti: I RB %d - %d", map_req->rgn_idx,
	     map_req->srgn_idx);
#endif

	blk_execute_rq_nowait(q, NULL, req, 1, ufshpb_map_req_compl_fn);

	if (ufshpb_is_triggered_by_fs(rgn))
		atomic64_inc(&hpb->set_rt_req_cnt);
	else
		atomic64_inc(&hpb->map_req_cnt);
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	ufshpb_increase_rb_count(hpb, rgn);
#endif
#if defined(SEC_UFS_ERROR_COUNT)
	/* It's same as ufshpb_increase_rb_count() */
	SEC_ufs_hpb_rb_count(hpb->ufsf->hba, rgn);
#endif

	return 0;
}

static int ufshpb_issue_map_req(struct ufshpb_lu *hpb,
				struct ufshpb_req *map_req)
{
	struct scsi_device *sdev;
	struct ufsf_feature *ufsf = hpb->ufsf;
	int ret = 0;

	sdev = ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		WARNING_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	ret = ufshpb_get_scsi_device(ufsf->hba, sdev);
	if (ret)
		return ret;

	ret = ufshpb_execute_map_req(hpb, sdev, map_req);
	if (ret)
		scsi_device_put(sdev);

	return ret;
}

static inline void ufshpb_set_map_req(struct ufshpb_lu *hpb, int rgn_idx,
				      int srgn_idx, struct ufshpb_map_ctx *mctx,
				      struct ufshpb_req *map_req)
{
	map_req->hpb = hpb;
	map_req->rgn_idx = rgn_idx;
	map_req->srgn_idx = srgn_idx;
	map_req->rb.mctx = mctx;
	map_req->rb.lun = hpb->lun;
}

static struct ufshpb_map_ctx *ufshpb_get_map_ctx(struct ufshpb_lu *hpb,
						 int *err)
{
	struct ufshpb_map_ctx *mctx;

	mctx = list_first_entry_or_null(&hpb->lh_map_ctx_free,
					struct ufshpb_map_ctx, list_table);
	if (mctx) {
		list_del_init(&mctx->list_table);
		hpb->debug_free_table--;
		return mctx;
	}
	*err = -ENOMEM;
	return NULL;
}

static inline void ufshpb_add_lru_info(struct victim_select_info *lru_info,
				       struct ufshpb_region *rgn)
{
	if (ufshpb_is_triggered_by_fs(rgn)) {
		rgn->rgn_state = HPBREGION_RT_PINNED;
		list_add_tail(&rgn->list_lru_rgn,
			      &lru_info->lh_lru_rt_pinned_rgn);
	} else {
		rgn->rgn_state = HPBREGION_ACTIVE;
		list_add_tail(&rgn->list_lru_rgn, &lru_info->lh_lru_rgn);
	}
	atomic64_inc(&lru_info->active_cnt);
}

static inline int ufshpb_add_region(struct ufshpb_lu *hpb,
				    struct ufshpb_region *rgn)
{
	struct victim_select_info *lru_info;
	int srgn_idx;
	int err = 0;

	lru_info = &hpb->lru_info;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn;

		srgn = rgn->srgn_tbl + srgn_idx;

		srgn->mctx = ufshpb_get_map_ctx(hpb, &err);
		if (!srgn->mctx) {
			ERR_MSG("get mctx err %d srgn %d free_table %d",
				  err, srgn_idx, hpb->debug_free_table);
			goto out;
		}

		srgn->srgn_state = HPBSUBREGION_DIRTY;
	}
#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "\x1b[44m\x1b[32m E->active region: %d \x1b[0m",
		  rgn->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "Noti: ACT RG: %d", rgn->rgn_idx);
#endif

	ufshpb_add_lru_info(lru_info, rgn);
out:
	return err;
}

static inline void ufshpb_put_map_ctx(struct ufshpb_lu *hpb,
				      struct ufshpb_map_ctx *mctx)
{
	list_add(&mctx->list_table, &hpb->lh_map_ctx_free);
	hpb->debug_free_table++;
}

static inline void ufshpb_purge_active_subregion(struct ufshpb_lu *hpb,
						 struct ufshpb_subregion *srgn,
						 int state)
{
	if (state == HPBSUBREGION_UNUSED) {
		ufshpb_put_map_ctx(hpb, srgn->mctx);
		srgn->mctx = NULL;
	}

	srgn->srgn_state = state;
}

static inline void ufshpb_cleanup_lru_info(struct victim_select_info *lru_info,
					   struct ufshpb_region *rgn)
{
	list_del_init(&rgn->list_lru_rgn);
	rgn->rgn_state = HPBREGION_INACTIVE;
	atomic64_dec(&lru_info->active_cnt);
}

static void __ufshpb_evict_region(struct ufshpb_lu *hpb,
				  struct ufshpb_region *rgn)
{
	struct victim_select_info *lru_info;
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	lru_info = &hpb->lru_info;

#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "\x1b[41m\x1b[33m C->EVICT region: %d \x1b[0m",
		  rgn->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "Noti: EVIC RG: %d", rgn->rgn_idx);
#endif

	ufshpb_cleanup_lru_info(lru_info, rgn);

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		ufshpb_purge_active_subregion(hpb, srgn, HPBSUBREGION_UNUSED);
	}
}

/*
 *  Must be held hpb_lock before call this func.
 */
static int ufshpb_check_issue_state_srgns(struct ufshpb_lu *hpb,
					  struct ufshpb_region *rgn)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn  = rgn->srgn_tbl + srgn_idx;

		if (srgn->srgn_state == HPBSUBREGION_ISSUED)
			return -EPERM;
	}
	return 0;
}

static struct
ufshpb_region *ufshpb_victim_lru_info(struct ufshpb_lu *hpb,
				      struct ufshpb_region *load_rgn)
{
	struct victim_select_info *lru_info = &hpb->lru_info;
	struct ufshpb_region *rgn;
	struct ufshpb_region *victim_rgn = NULL;

	switch (lru_info->selection_type) {
	case LRU:
		list_for_each_entry(rgn, &lru_info->lh_lru_rgn, list_lru_rgn) {
			if (!rgn)
				break;

			if (ufshpb_check_issue_state_srgns(hpb, rgn))
				continue;

			victim_rgn = rgn;
			break;
		}

		if (victim_rgn || !ufshpb_is_triggered_by_fs(load_rgn))
			break;

		list_for_each_entry(rgn, &lru_info->lh_lru_rt_pinned_rgn,
				    list_lru_rgn) {
			if (!rgn)
				break;

			if (ufshpb_check_issue_state_srgns(hpb, rgn))
				continue;

			victim_rgn = rgn;
			break;

		}
		break;
	default:
		break;
	}

	return victim_rgn;
}

/* routine : unset_rt compl */
static void ufshpb_unset_rt_req_compl_fn(struct request *req,
					 blk_status_t error)
{
	struct ufshpb_req *pre_req = (struct ufshpb_req *) req->end_io_data;
	struct ufshpb_lu *hpb = pre_req->hpb;
	struct ufshpb_region *rgn = hpb->rgn_tbl + pre_req->rgn_idx;
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	int srgn_idx = 0;

#ifdef CONFIG_PM
	ufshpb_mimic_blk_pm_put_request(req);
#endif

	if (hpb->ufsf->ufshpb_state != HPB_PRESENT) {
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		goto free_pre_req;
	}

	if (error) {
		ERR_MSG("COMP UNSET_RT err %d rgn %d", error, pre_req->rgn_idx);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		goto free_pre_req;
	}

#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "COMPL UNSET_RT %d", pre_req->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "COMPL UNSET_RT %d", pre_req->rgn_idx);
#endif
	printk("***compl de-active region :%d ****\n",pre_req->rgn_idx);
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (rgn->rgn_state != HPBREGION_INACTIVE)
		rgn->rgn_state = HPBREGION_ACTIVE;
free_pre_req:
	ufshpb_put_pre_req(pre_req->hpb, pre_req);
	hpb->num_inflight_rt_req--;
	rgn->discard_write_size = 0;
	//atomic64_set(&rgn->last_miss_cnt, 0);
	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++){
			srgn = rgn->srgn_tbl + srgn_idx;
			atomic64_set(&srgn->read_cnt, 0);
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	if (is_clear_reason(hpb, rgn))
		atomic_set(&rgn->reason, HPBUPDATE_NONE);

	scsi_device_put(hpb->ufsf->sdev_ufs_lu[hpb->lun]);
	ufshpb_lu_put(hpb);
}

static inline void ufshpb_set_unset_rt_cmd(unsigned char *cdb, u16 rgn_idx)
{
	cdb[0] = UFSHPB_WRITE_BUFFER;
	cdb[1] = UFSHPB_WB_ID_UNSET_RT;
	cdb[2] = GET_BYTE_1(rgn_idx);
	cdb[3] = GET_BYTE_0(rgn_idx);
	cdb[9] = 0x00;	/* Control = 0x00 */
}

static int ufshpb_execute_unset_rt_req(struct ufshpb_lu *hpb,
				       struct scsi_device *sdev,
				       struct ufshpb_req *pre_req)
{
	struct request_queue *q = sdev->request_queue;
	struct request *req;
	struct scsi_request *rq;
	struct scsi_cmnd *scmd;

	req = pre_req->req;

	/*
	 * blk_init_rl() -> alloc_request_size().
	 * q->init_rq_fn = scsi_old_init_rq behavior.
	 */
	scmd = (struct scsi_cmnd *)(req + 1);
	memset(scmd, 0, sizeof(*scmd));
	scmd->sense_buffer = pre_req->sense;
	scmd->req.sense = scmd->sense_buffer;

	/* blk_get_request behavior */
	blk_rq_init(q, req);
	q->initialize_rq_fn(req);

	/* 1. request setup */
	req->cmd_flags = REQ_OP_WRITE | REQ_OP_SCSI_OUT;
	req->rq_flags = RQF_QUIET | RQF_PREEMPT;
	req->timeout = msecs_to_jiffies(30000);
	req->end_io_data = (void *)pre_req;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	ufshpb_set_unset_rt_cmd(rq->cmd, pre_req->rgn_idx);
	rq->cmd_len = scsi_command_size(rq->cmd);

#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "ISSUE UNSET_RT %d", pre_req->rgn_idx);
	TMSG(hpb->ufsf, hpb->lun, "ISSUE UNSET_RT %d", pre_req->rgn_idx);
#endif

	blk_execute_rq_nowait(q, NULL, req, 1, ufshpb_unset_rt_req_compl_fn);

	atomic64_inc(&hpb->unset_rt_req_cnt);

	return 0;
}

static int ufshpb_issue_unset_rt_req(struct ufshpb_lu *hpb,
				     struct ufshpb_req *pre_req)
{
	struct scsi_device *sdev;
	struct ufsf_feature *ufsf = hpb->ufsf;
	int ret = 0;

	sdev = ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		WARNING_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	ret = ufshpb_get_scsi_device(ufsf->hba, sdev);
	if (ret)
		return ret;

	ret = ufshpb_execute_unset_rt_req(hpb, sdev, pre_req);
	if (ret)
		scsi_device_put(sdev);

	return ret;
}

static int ufshpb_prepare_unset_rt_req(struct ufshpb_lu *hpb,
				       struct ufshpb_region *rgn)
{
	struct ufshpb_req *pre_req;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (hpb->num_inflight_rt_req >= hpb->throttle_rt_req) {
#if defined(CONFIG_HPB_DEBUG)
		HPB_DEBUG(hpb, "rt_req throttle. rt inflight %d rt throttle %d",
			  hpb->num_inflight_rt_req, hpb->throttle_rt_req);
#endif
		ret = -ENOMEM;
		goto unlock_out;
	}
	pre_req = ufshpb_get_pre_req(hpb);
	if (!pre_req) {
		ret = -ENOMEM;
		goto unlock_out;
	}
	hpb->num_inflight_rt_req++;
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	pre_req->hpb = hpb;
	pre_req->rgn_idx = rgn->rgn_idx;

	ret = ufshpb_lu_get(hpb);
	if (ret) {
		WARNING_MSG("warning: ufshpb_lu_get failed.. %d", ret);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_put_pre_req(hpb, pre_req);
		hpb->num_inflight_rt_req--;
		goto unlock_out;
	}

	ret = ufshpb_issue_unset_rt_req(hpb, pre_req);
	if (ret) {
		ERR_MSG("issue unset_rt_req failed. rgn %d err %d",
			rgn->rgn_idx, ret);
		ufshpb_lu_put(hpb);
		goto wakeup_ee_worker;
	}
	return ret;
unlock_out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return ret;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
	return ret;

}

static int ufshpb_evict_region(struct ufshpb_lu *hpb, struct ufshpb_region *rgn)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (rgn->rgn_state == HPBREGION_PINNED) {
		/*
		 * Pinned active-block should not drop-out.
		 * But if so, it would treat error as critical,
		 * and it will run ufshpb_eh_work
		 */
		WARNING_MSG("rgn[%d] pinned drop-out error",
			    rgn->rgn_idx);
		goto out;
	} else if (rgn->rgn_state == HPBREGION_RT_PINNED) {
		if (!ufshpb_is_triggered_by_fs(rgn)) {
			WARNING_MSG("rgn[%d] rt_pinned drop-out error",
				  rgn->rgn_idx);
			goto out;
		} else {
			if (ufshpb_check_issue_state_srgns(hpb, rgn))
				goto evict_fail;

			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			ret = ufshpb_prepare_unset_rt_req(hpb, rgn);
			spin_lock_irqsave(&hpb->hpb_lock, flags);
			if (ret)
				goto evict_fail;
		}
	}

	if (!list_empty(&rgn->list_lru_rgn)) {
		if (ufshpb_check_issue_state_srgns(hpb, rgn))
			goto evict_fail;

		__ufshpb_evict_region(hpb, rgn);
	}
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return 0;
evict_fail:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return -EPERM;
}

static inline struct
ufshpb_rsp_field *ufshpb_get_hpb_rsp(struct ufshcd_lrb *lrbp)
{
	return (struct ufshpb_rsp_field *)&lrbp->ucd_rsp_ptr->sr.sense_data_len;
}

static int ufshpb_prepare_map_req(struct ufshpb_lu *hpb,
				  struct ufshpb_subregion *srgn)
{
	struct ufshpb_req *map_req;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&hpb->hpb_lock, flags);

	if (srgn->srgn_state == HPBSUBREGION_ISSUED) {
		ret = -EAGAIN;
		goto unlock_out;
	}

	map_req = ufshpb_get_map_req(hpb);
	if (!map_req) {
		ret = -ENOMEM;
		goto unlock_out;
	}

	srgn->srgn_state = HPBSUBREGION_ISSUED;

	ret = ufshpb_clean_dirty_bitmap(hpb, srgn);
	if (ret) {
		ufshpb_put_map_req(hpb, map_req);
		goto unlock_out;
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	ufshpb_set_map_req(hpb, srgn->rgn_idx, srgn->srgn_idx,
			   srgn->mctx, map_req);

	ret = ufshpb_lu_get(hpb);
	if (ret) {
		WARNING_MSG("warning: ufshpb_lu_get failed.. %d", ret);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_put_map_req(hpb, map_req);
		goto unlock_out;
	}

	ret = ufshpb_issue_map_req(hpb, map_req);
	if (ret) {
		ERR_MSG("issue map_req failed. [%d-%d] err %d",
			srgn->rgn_idx, srgn->srgn_idx, ret);
		ufshpb_lu_put(hpb);
		goto wakeup_ee_worker;
	}
	return ret;
unlock_out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return ret;
wakeup_ee_worker:
	ufshpb_failed(hpb, __func__);
	return ret;
}

static int ufshpb_load_region(struct ufshpb_lu *hpb, struct ufshpb_region *rgn)
{
	struct ufshpb_region *victim_rgn;
	struct victim_select_info *lru_info = &hpb->lru_info;
	unsigned long flags;
	int ret = 0;

	/*
	 * if already region is added to lru_list,
	 * just initiate the information of lru.
	 * because the region already has the map ctx.
	 * (!list_empty(&rgn->list_region) == region->state=active...)
	 */
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	if (!list_empty(&rgn->list_lru_rgn)) {
		ufshpb_hit_lru_info(lru_info, rgn);
		goto out;
	}

	if (rgn->rgn_state == HPBREGION_INACTIVE) {
		if (atomic64_read(&lru_info->active_cnt)
		    == lru_info->max_lru_active_cnt) {
			victim_rgn = ufshpb_victim_lru_info(hpb, rgn);
			if (!victim_rgn) {
#if defined(CONFIG_HPB_DEBUG)
				HPB_DEBUG(hpb, "UFSHPB victim_rgn is NULL");
#endif
				if (list_empty(&lru_info->lh_lru_rgn) &&
				    !ufshpb_is_triggered_by_fs(rgn))
					ret = -EACCES;
				else
					ret = -ENOMEM;
				goto out;
			}

			if (victim_rgn->rgn_state == HPBREGION_RT_PINNED) {
				spin_unlock_irqrestore(&hpb->hpb_lock, flags);
				ret = ufshpb_prepare_unset_rt_req(hpb, victim_rgn);
				spin_lock_irqsave(&hpb->hpb_lock, flags);
				if (ret) {
					ERR_MSG("issue unset_rt failed. [%d] err %d",
						rgn->rgn_idx, ret);
					goto out;
				}
			}

#if defined(CONFIG_HPB_DEBUG)
			TMSG(hpb->ufsf, hpb->lun, "Noti: VT RG %d state %d",
			     victim_rgn->rgn_idx, victim_rgn->rgn_state);
			HPB_DEBUG(hpb,
				  "LRU MAX(=%lld). victim choose %d state %d",
				  (long long)atomic64_read(&lru_info->active_cnt),
				  victim_rgn->rgn_idx, victim_rgn->rgn_state);
#endif

			__ufshpb_evict_region(hpb, victim_rgn);
		}

		ret = ufshpb_add_region(hpb, rgn);
		if (ret) {
			ERR_MSG("UFSHPB memory allocation failed");
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			goto wake_up_ee_worker;
		}
	}
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return ret;
wake_up_ee_worker:
	ufshpb_failed(hpb, __func__);
	return ret;
}

static void ufshpb_rsp_req_region_update(struct ufshpb_lu *hpb,
					 struct ufshpb_rsp_field *rsp_field)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	int num, rgn_idx, srgn_idx;

	/*
	 * If active rgn = inactive rgn, choose inactive rgn.
	 * So, active process -> inactive process
	 */
	for (num = 0; num < rsp_field->active_rgn_cnt; num++) {
		rgn_idx = be16_to_cpu(rsp_field->hpb_active_field[num].active_rgn);
		srgn_idx = be16_to_cpu(rsp_field->hpb_active_field[num].active_srgn);

		if (unlikely(rgn_idx >= hpb->rgns_per_lu)) {
			ERR_MSG("rgn_idx %d is wrong. (Max %d\n",
				rgn_idx, hpb->rgns_per_lu);
			continue;
		}

		rgn = hpb->rgn_tbl + rgn_idx;

		if (unlikely(srgn_idx >= rgn->srgn_cnt)) {
			ERR_MSG("srgn_idx %d is wrong. (Max %d\n",
				srgn_idx, rgn->srgn_cnt);
			continue;
		}

		srgn = rgn->srgn_tbl + srgn_idx;

		if (ufshpb_is_triggered_by_fs(rgn) || ufshpb_is_host_control_mode(hpb))
		{
			spin_lock(&hpb->rsp_list_lock);
			ufshpb_update_active_info(hpb, rgn_idx,srgn_idx,
						  HPBUPDATE_RT_FROM_FS);
			spin_unlock(&hpb->rsp_list_lock);

			spin_lock(&hpb->hpb_lock);
			atomic_set(&rgn->reason, HPBUPDATE_RT_FROM_FS);
			if (srgn->srgn_state == HPBSUBREGION_CLEAN)
				srgn->srgn_state = HPBSUBREGION_DIRTY;
			spin_unlock(&hpb->hpb_lock);
		} else {
			//continue;
			spin_lock(&hpb->rsp_list_lock);
			ufshpb_update_active_info(hpb, rgn_idx, srgn_idx,
						  HPBUPDATE_FROM_DEV);
			spin_unlock(&hpb->rsp_list_lock);

			/* It is just blocking HPB_READ */
			spin_lock(&hpb->hpb_lock);
			if (srgn->srgn_state == HPBSUBREGION_CLEAN)
				srgn->srgn_state = HPBSUBREGION_DIRTY;
			spin_unlock(&hpb->hpb_lock);

			atomic64_inc(&hpb->rb_active_cnt);
		}
	}

	for (num = 0; num < rsp_field->inactive_rgn_cnt; num++) {
		rgn_idx = be16_to_cpu(rsp_field->hpb_inactive_field[num]);

		if (unlikely(rgn_idx >= hpb->rgns_per_lu)) {
			ERR_MSG("rgn_idx %d is wrong. (Max %d\n",
				rgn_idx, hpb->rgns_per_lu);
			continue;
		}

		rgn = hpb->rgn_tbl + rgn_idx;

		if (ufshpb_is_triggered_by_fs(rgn) || ufshpb_is_host_control_mode(hpb))
			continue;

		spin_lock(&hpb->rsp_list_lock);
		ufshpb_update_inactive_info(hpb, rgn_idx,
					    HPBUPDATE_FROM_DEV);
		spin_unlock(&hpb->rsp_list_lock);

		/*
		 * Doesn't clean up lru info. It will be executed in worker.
		 * It is just blocking HPB_READ.
		 */
		spin_lock(&hpb->hpb_lock);
		if (rgn->rgn_state == HPBREGION_ACTIVE)
			rgn->rgn_state = HPBREGION_INACTIVE;
		spin_unlock(&hpb->hpb_lock);

		atomic64_inc(&hpb->rb_inactive_cnt);
	}
}

static inline int ufshpb_may_field_valid(struct ufshcd_lrb *lrbp,
					 struct ufshpb_rsp_field *rsp_field)
{
	if (be16_to_cpu(rsp_field->sense_data_len) != DEV_SENSE_SEG_LEN ||
	    rsp_field->desc_type != DEV_DES_TYPE ||
	    rsp_field->additional_len != DEV_ADDITIONAL_LEN ||
	    rsp_field->hpb_type == HPB_RSP_NONE ||
	    rsp_field->active_rgn_cnt > MAX_ACTIVE_NUM ||
	    rsp_field->inactive_rgn_cnt > MAX_INACTIVE_NUM ||
	    (!rsp_field->active_rgn_cnt && !rsp_field->inactive_rgn_cnt))
		return -EINVAL;

	if (!ufsf_is_valid_lun(lrbp->lun)) {
		ERR_MSG("LU(%d) is not supported", lrbp->lun);
		return -EINVAL;
	}

	return 0;
}

static bool ufshpb_is_empty_rsp_lists(struct ufshpb_lu *hpb)
{
	bool ret = true;
	unsigned long flags;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	if (!list_empty(&hpb->lh_inact_rgn) || !list_empty(&hpb->lh_act_srgn))
		ret = false;
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

	return ret;
}

void ufshpb_wakeup_worker_on_idle(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	if (ufsf->hba->outstanding_reqs)
		return;

	seq_scan_lu(lun) {
		hpb = ufsf->ufshpb_lup[lun];
		if (hpb) {
			if (!ufshpb_is_empty_rsp_lists(hpb))
				schedule_work(&hpb->ufshpb_task_workq);
		}
	}
}
/* routine : isr (ufs) */
void ufshpb_rsp_upiu(struct ufsf_feature *ufsf, struct ufshcd_lrb *lrbp)
{
	struct ufshpb_lu *hpb;
	struct ufshpb_rsp_field *rsp_field;
	struct ufshpb_rsp_field sense_data;
	int data_seg_len, ret;

	data_seg_len = be32_to_cpu(lrbp->ucd_rsp_ptr->header.dword_2)
		& MASK_RSP_UPIU_DATA_SEG_LEN;
	if (!data_seg_len)
		return;

	memcpy(&sense_data, &lrbp->ucd_rsp_ptr->sr.sense_data_len,
		sizeof(struct ufshpb_rsp_field));
	rsp_field = &sense_data;

	if (ufshpb_may_field_valid(lrbp, rsp_field)) {
		WARN_ON(rsp_field->additional_len != DEV_ADDITIONAL_LEN);
		return;
	}

	hpb = ufsf->ufshpb_lup[lrbp->lun];
	ret = ufshpb_lu_get(hpb);
	if (ret) {
		WARNING_MSG("warning: ufshpb_lu_get failed %d..", ret);
		return;
	}

	if (hpb->force_map_req_disable)
		goto put_hpb;

	atomic64_inc(&hpb->rb_noti_cnt);

	switch (rsp_field->hpb_type) {
	case HPB_RSP_REQ_REGION_UPDATE:
		WARN_ON(data_seg_len != DEV_DATA_SEG_LEN);
		ufshpb_rsp_req_region_update(hpb, rsp_field);
		break;
	default:
		break;
	}

put_hpb:
	ufshpb_lu_put(hpb);
}

static int ufshpb_execute_map_req_wait(struct ufshpb_lu *hpb,
				       unsigned char *cmd,
				       struct ufshpb_subregion *srgn)
{
	struct ufsf_feature *ufsf = hpb->ufsf;
	struct scsi_device *sdev;
	struct request_queue *q;
	struct request *req;
	struct scsi_request *rq;
	struct bio *bio;
	struct scsi_sense_hdr sshdr = {0};
	unsigned long flags;
	int ret = 0;

	sdev = ufsf->sdev_ufs_lu[hpb->lun];
	if (!sdev) {
		WARNING_MSG("cannot find scsi_device");
		return -ENODEV;
	}

	q = sdev->request_queue;

	ret = ufshpb_get_scsi_device(ufsf->hba, sdev);
	if (ret)
		return ret;

	req = blk_get_request(q, REQ_OP_SCSI_IN, 0);
	if (IS_ERR(req)) {
		WARNING_MSG("cannot get request");
		ret = -EIO;
		goto sdev_put_out;
	}

	bio = bio_kmalloc(GFP_KERNEL, hpb->mpages_per_srgn);
	if (!bio) {
		ret = -ENOMEM;
		goto req_put_out;
	}

	ret = ufshpb_map_req_add_bio_page(hpb, q, bio, srgn->mctx);
	if (ret)
		goto mem_free_out;

	/* 1. request setup*/
	blk_rq_append_bio(req, &bio); /* req->__data_len */
	req->timeout = msecs_to_jiffies(30000);
	req->cmd_flags |= REQ_OP_READ;
	req->rq_flags |= RQF_QUIET | RQF_PREEMPT;

	/* 2. scsi_request setup */
	rq = scsi_req(req);
	rq->cmd_len = scsi_command_size(cmd);
	memcpy(rq->cmd, cmd, rq->cmd_len);

	blk_execute_rq(q, NULL, req, 1);
	if (rq->result) {
		ret = -EIO;
		scsi_normalize_sense(rq->sense, SCSI_SENSE_BUFFERSIZE, &sshdr);
		ERR_MSG("code %x sense_key %x asc %x ascq %x",
			sshdr.response_code, sshdr.sense_key, sshdr.asc,
			sshdr.ascq);
		ERR_MSG("byte4 %x byte5 %x byte6 %x additional_len %x",
			sshdr.byte4, sshdr.byte5, sshdr.byte6,
			sshdr.additional_length);
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_error_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	}
mem_free_out:
	bio_put(bio);
req_put_out:
	blk_put_request(req);
sdev_put_out:
	scsi_device_put(sdev);
	return ret;
}

static int ufshpb_issue_map_req_from_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);

	while ((srgn = list_first_entry_or_null(&hpb->lh_pinned_srgn,
						struct ufshpb_subregion,
						list_act_srgn))) {
		unsigned char cmd[10] = { 0 };

		list_del_init(&srgn->list_act_srgn);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		ufshpb_set_read_buf_cmd(cmd, srgn->rgn_idx, srgn->srgn_idx,
					hpb->srgn_mem_size);
#if defined(CONFIG_HPB_DEBUG)
		if (hpb->debug)
			ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx,
					 srgn->mctx, "ISSUE");

		TMSG(hpb->ufsf, hpb->lun, "Pinned: I RB %d - %d",
		     srgn->rgn_idx, srgn->srgn_idx);
#endif
		ret = ufshpb_execute_map_req_wait(hpb, cmd, srgn);
		if (ret < 0) {
			ERR_MSG("region %d sub %d failed with err %d",
				srgn->rgn_idx, srgn->srgn_idx, ret);
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			if (list_empty(&srgn->list_act_srgn))
				list_add(&srgn->list_act_srgn,
					 &hpb->lh_pinned_srgn);
			continue;
		}
#if defined(CONFIG_HPB_DEBUG)
		if (hpb->debug)
			ufshpb_check_ppn(hpb, srgn->rgn_idx, srgn->srgn_idx,
					 srgn->mctx, "COMPL");
		TMSG(hpb->ufsf, hpb->lun, "Pinend: C RB %d - %d",
		     srgn->rgn_idx, srgn->srgn_idx);
#endif
		spin_lock_irqsave(&hpb->hpb_lock, flags);
		ufshpb_clean_active_subregion(hpb, srgn);
		spin_unlock_irqrestore(&hpb->hpb_lock, flags);

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}

	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

	return 0;
}

static void ufshpb_work_handler(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	int ret;

	hpb = container_of(work, struct ufshpb_lu, ufshpb_work);
#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "worker start for pinned region");
#endif
	if (!list_empty(&hpb->lh_pinned_srgn)) {
		ret = ufshpb_issue_map_req_from_list(hpb);
		/*
		 * if its function failed at init time,
		 * ufshpb-device will request map-req,
		 * so it is not critical-error, and just finish work-handler
		 */
		if (ret)
			WARNING_MSG("failed map-issue. ret %d", ret);
	}
#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "worker end");
#endif
}

static int ufshpb_check_pm(struct ufshpb_lu *hpb)
{
	struct ufs_hba *hba = hpb->ufsf->hba;

	if (hba->pm_op_in_progress ||
	    hba->curr_dev_pwr_mode != UFS_ACTIVE_PWR_MODE) {
		INFO_MSG("hba current power state %d pm_progress %d",
			 hba->curr_dev_pwr_mode,
			 hba->pm_op_in_progress);
		return -ENODEV;
	}
	return 0;
}

static void ufshpb_retry_work_handler(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	struct delayed_work *dwork = to_delayed_work(work);
	struct ufshpb_req *map_req, *next;
	unsigned long flags;
	int ret = 0;

	LIST_HEAD(retry_list);

	hpb = container_of(dwork, struct ufshpb_lu, ufshpb_retry_work);

	if (ufshpb_check_pm(hpb))
		return;

	ret = ufshpb_lu_get(hpb);
	if (ret) {
		WARNING_MSG("warning: ufshpb_lu_get failed %d..", ret);
		return;
	}
#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "retry worker start");
#endif
	spin_lock_irqsave(&hpb->hpb_lock, flags);
	list_splice_init(&hpb->lh_map_req_retry, &retry_list);
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);

	list_for_each_entry_safe(map_req, next, &retry_list, list_req) {
		list_del_init(&map_req->list_req);

		ret = ufshpb_lu_get(hpb);
		if (ret) {
			WARNING_MSG("warning: ufshpb_lu_get failed %d..", ret);
			spin_lock_irqsave(&hpb->hpb_lock, flags);
			ufshpb_put_map_req(hpb, map_req);
			spin_unlock_irqrestore(&hpb->hpb_lock, flags);
			continue;
		}

		ret = ufshpb_issue_map_req(hpb, map_req);
		if (ret) {
			ERR_MSG("issue map_req failed. [%d-%d] err %d",
				map_req->rgn_idx, map_req->srgn_idx, ret);
			ufshpb_lu_put(hpb);
			goto wakeup_ee_worker;
		}
	}
#if defined(CONFIG_HPB_DEBUG)
	HPB_DEBUG(hpb, "worker end");
#endif
	ufshpb_lu_put(hpb);
	return;
wakeup_ee_worker:
	ufshpb_lu_put(hpb);
	ufshpb_failed(hpb, __func__);
}

static void ufshpb_add_starved_list(struct ufshpb_lu *hpb,
				    struct ufshpb_region *rgn,
				    struct list_head *starved_list)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx;

	if (!list_empty(&rgn->list_inact_rgn))
		return;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		if (!list_empty(&srgn->list_act_srgn))
			return;
	}

	/* It will be added to hpb->lh_inact_rgn */
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_inc(&hpb->inact_rsp_list_cnt);
#endif
	list_add_tail(&rgn->list_inact_rgn, starved_list);
}

static void ufshpb_run_inactive_region_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn;
	unsigned long flags;
	int ret;
	LIST_HEAD(starved_list);

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	while ((rgn = list_first_entry_or_null(&hpb->lh_inact_rgn,
					       struct ufshpb_region,
					       list_inact_rgn))) {
		if (hpb->ufsf->ufshpb_state == HPB_SUSPEND) {
			INFO_MSG("suspend state. Issuing WRITE_BUFFER the next turn");
			break;
		}

		ufshpb_inact_rsp_list_del(hpb, rgn);
		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		ret = ufshpb_evict_region(hpb, rgn);
		if (ret) {
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			ufshpb_add_starved_list(hpb, rgn, &starved_list);
			spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
		}

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}

	list_splice(&starved_list, &hpb->lh_inact_rgn);
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_add_active_list(struct ufshpb_lu *hpb,
				   struct ufshpb_region *rgn,
				   struct ufshpb_subregion *srgn)
{
	if (!list_empty(&rgn->list_inact_rgn))
		return;

	if (!list_empty(&srgn->list_act_srgn)) {
		list_move(&srgn->list_act_srgn, &hpb->lh_act_srgn);
		return;
	}

	ufshpb_act_rsp_list_add(hpb, srgn);
}

static void ufshpb_run_active_subregion_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	while ((srgn = list_first_entry_or_null(&hpb->lh_act_srgn,
						struct ufshpb_subregion,
						list_act_srgn))) {
		if (hpb->ufsf->ufshpb_state == HPB_SUSPEND) {
			INFO_MSG("suspend state. Issuing READ_BUFFER the next turn");
			break;
		}

		ufshpb_act_rsp_list_del(hpb, srgn);

		if (hpb->force_map_req_disable)
			continue;

		spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);

		rgn = hpb->rgn_tbl + srgn->rgn_idx;

		ret = ufshpb_load_region(hpb, rgn);
		if (ret) {
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			if (ret == -EACCES) {
				continue;
			} else {
				ufshpb_add_active_list(hpb, rgn, srgn);
				break;
			}
		}

		ret = ufshpb_prepare_map_req(hpb, srgn);

		if (ret) {
			spin_lock_irqsave(&hpb->rsp_list_lock, flags);
			ufshpb_add_active_list(hpb, rgn, srgn);
			break;
		}

		spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	}
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_task_workq_fn(struct work_struct *work)
{
	struct ufshpb_lu *hpb;
	int ret;

	hpb = container_of(work, struct ufshpb_lu, ufshpb_task_workq);
	ret = ufshpb_lu_get(hpb);
	if (ret) {
		if (hpb)
			WARNING_MSG("lu_get failed.. hpb_state %d",
				    hpb->ufsf->ufshpb_state);
		WARNING_MSG("warning: ufshpb_lu_get failed %d..", ret);
		return;
	}

	ufshpb_run_inactive_region_list(hpb);
	ufshpb_run_active_subregion_list(hpb);

	ufshpb_lu_put(hpb);
}

static void ufshpb_init_constant(void)
{
	sects_per_blk_shift = ffs(BLOCK) - ffs(SECTOR);
	INIT_INFO("sects_per_blk_shift: %u %u", sects_per_blk_shift,
		  ffs(SECTORS_PER_BLOCK) - 1);

	bits_per_dword_shift = ffs(BITS_PER_DWORD) - 1;
	bits_per_dword_mask = BITS_PER_DWORD - 1;
	INIT_INFO("bits_per_dword %u shift %u mask 0x%X", BITS_PER_DWORD,
		  bits_per_dword_shift, bits_per_dword_mask);

	bits_per_byte_shift = ffs(BITS_PER_BYTE) - 1;
	INIT_INFO("bits_per_byte %u shift %u", BITS_PER_BYTE,
		  bits_per_byte_shift);
}

static inline void ufshpb_map_req_mempool_remove(struct ufshpb_lu *hpb)
{
	int i;

	for (i = 0; i < hpb->qd; i++) {
		kfree(hpb->map_req[i].req);
		bio_put(hpb->map_req[i].bio);
	}

	kfree(hpb->map_req);
}

static inline void ufshpb_pre_req_mempool_remove(struct ufshpb_lu *hpb)
{
	int i;

	for (i = 0; i < hpb->qd; i++) {
		kfree(hpb->pre_req[i].req);
		bio_put(hpb->pre_req[i].bio);
		__free_page(hpb->pre_req[i].wb.m_page);
	}

	kfree(hpb->pre_req);
}

static void ufshpb_table_mempool_remove(struct ufshpb_lu *hpb)
{
	struct ufshpb_map_ctx *mctx, *next;
	int i;

	/*
	 * the mctx in the lh_map_ctx_free has been allocated completely.
	 */
	list_for_each_entry_safe(mctx, next, &hpb->lh_map_ctx_free,
				 list_table) {
		for (i = 0; i < hpb->mpages_per_srgn; i++)
			__free_page(mctx->m_page[i]);

		vfree(mctx->ppn_dirty);
		kfree(mctx->m_page);
		kfree(mctx);
		hpb->alloc_mctx--;
	}
}

/*
 * this function doesn't need to hold lock due to be called in init.
 * (hpb_lock, rsp_list_lock, etc..)
 */
static int ufshpb_init_pinned_active_region(struct ufshpb_lu *hpb,
					    struct ufshpb_region *rgn)
{
	struct ufshpb_subregion *srgn;
	int srgn_idx, j;
	int err = 0;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		srgn = rgn->srgn_tbl + srgn_idx;

		srgn->mctx = ufshpb_get_map_ctx(hpb, &err);
		if (err) {
			ERR_MSG("get mctx err %d srgn %d free_table %d",
				err, srgn_idx, hpb->debug_free_table);
			goto release;
		}

		srgn->srgn_state = HPBSUBREGION_ISSUED;
		ufshpb_clean_dirty_bitmap(hpb, srgn);
		list_add_tail(&srgn->list_act_srgn, &hpb->lh_pinned_srgn);
	}

	rgn->rgn_state = HPBREGION_PINNED;

	return 0;

release:
	for (j = 0; j < srgn_idx; j++) {
		srgn = rgn->srgn_tbl + j;
		ufshpb_put_map_ctx(hpb, srgn->mctx);
	}

	return err;
}

static inline bool ufshpb_is_pinned_region(struct ufshpb_lu *hpb, int rgn_idx)
{
	if (hpb->lu_pinned_end_offset != -1 &&
	    rgn_idx >= hpb->lu_pinned_rgn_startidx &&
	    rgn_idx <= hpb->lu_pinned_end_offset)
		return true;

	return false;
}

static inline void ufshpb_init_jobs(struct ufshpb_lu *hpb)
{
	INIT_WORK(&hpb->ufshpb_work, ufshpb_work_handler);
	INIT_DELAYED_WORK(&hpb->ufshpb_retry_work, ufshpb_retry_work_handler);
	INIT_WORK(&hpb->ufshpb_task_workq, ufshpb_task_workq_fn);
}

static inline void ufshpb_cancel_jobs(struct ufshpb_lu *hpb)
{
	cancel_work_sync(&hpb->ufshpb_work);
	cancel_delayed_work_sync(&hpb->ufshpb_retry_work);
	cancel_work_sync(&hpb->ufshpb_task_workq);
}

static void ufshpb_init_subregion_tbl(struct ufshpb_lu *hpb,
				      struct ufshpb_region *rgn)
{
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn = rgn->srgn_tbl + srgn_idx;

		INIT_LIST_HEAD(&srgn->list_act_srgn);

		srgn->rgn_idx = rgn->rgn_idx;
		srgn->srgn_idx = srgn_idx;
		srgn->srgn_state = HPBSUBREGION_UNUSED;
		atomic64_set(&srgn->read_cnt, 0);
	}
}

static inline int ufshpb_alloc_subregion_tbl(struct ufshpb_lu *hpb,
					     struct ufshpb_region *rgn,
					     int srgn_cnt)
{
	rgn->srgn_tbl =
		kzalloc(sizeof(struct ufshpb_subregion) * srgn_cnt, GFP_KERNEL);
	if (!rgn->srgn_tbl)
		return -ENOMEM;

	rgn->srgn_cnt = srgn_cnt;
	return 0;
}

static int ufshpb_table_mempool_init(struct ufshpb_lu *hpb)
{
	struct ufshpb_map_ctx *mctx = NULL;
	int i, j, k;

	INIT_LIST_HEAD(&hpb->lh_map_ctx_free);

	hpb->alloc_mctx = hpb->lu_max_active_rgns * hpb->srgns_per_rgn;

	for (i = 0; i < hpb->alloc_mctx; i++) {
		mctx = kmalloc(sizeof(struct ufshpb_map_ctx), GFP_KERNEL);
		if (!mctx)
			goto release_mem;

		mctx->m_page =
			kzalloc(sizeof(struct page *) * hpb->mpages_per_srgn,
				GFP_KERNEL);
		if (!mctx->m_page)
			goto release_mem;

		mctx->ppn_dirty =
			vzalloc(hpb->entries_per_srgn >> bits_per_byte_shift);
		if (!mctx->ppn_dirty)
			goto release_mem;

		for (j = 0; j < hpb->mpages_per_srgn; j++) {
			mctx->m_page[j] = alloc_page(GFP_KERNEL | __GFP_ZERO);
			if (!mctx->m_page[j]) {
				for (k = 0; k < j; k++)
					kfree(mctx->m_page[k]);
				goto release_mem;
			}
		}

		INIT_LIST_HEAD(&mctx->list_table);
		list_add(&mctx->list_table, &hpb->lh_map_ctx_free);

		hpb->debug_free_table++;
	}

	INIT_INFO("The number of mctx = %d. debug_free_table %d",
		  hpb->alloc_mctx, hpb->debug_free_table);
	return 0;
release_mem:
	/*
	 * mctxs already added in lh_map_ctx_free will be removed
	 * in the caller function.
	 */
	if (mctx) {
		kfree(mctx->m_page);
		vfree(mctx->ppn_dirty);
		kfree(mctx);
	}
	return -ENOMEM;
}

static int
ufshpb_map_req_mempool_init(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct request_queue *q;
	struct ufshpb_req *map_req = NULL;
	int qd = hpb->qd;
	int i, j;

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	q = sdev->request_queue;

	INIT_LIST_HEAD(&hpb->lh_map_req_free);
	INIT_LIST_HEAD(&hpb->lh_map_req_retry);

	hpb->map_req = kzalloc(sizeof(struct ufshpb_req) * qd, GFP_KERNEL);
	if (!hpb->map_req)
		return -ENOMEM;

	/*
	 * q->cmd_size: sizeof(struct scsi_cmd) + shost->hostt->cmd_size
	 */
	for (i = 0; i < qd; i++) {
		map_req = hpb->map_req + i;
		INIT_LIST_HEAD(&map_req->list_req);
		map_req->req = kzalloc(sizeof(struct request) + q->cmd_size,
				       GFP_KERNEL);
		if (!map_req->req) {
			for (j = 0; j < i; j++)
				kfree(hpb->map_req[j].req);
			goto release_mem;
		}

		map_req->bio = bio_kmalloc(GFP_KERNEL, hpb->mpages_per_srgn);
		if (!map_req->bio) {
			kfree(hpb->map_req[i].req);
			for (j = 0; j < i; j++) {
				kfree(hpb->map_req[j].req);
				bio_put(hpb->map_req[j].bio);
			}
			goto release_mem;
		}
		list_add_tail(&map_req->list_req, &hpb->lh_map_req_free);
	}

	return 0;
release_mem:
	kfree(hpb->map_req);
	return -ENOMEM;
}

static int
ufshpb_pre_req_mempool_init(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct request_queue *q;
	struct ufshpb_req *pre_req = NULL;
	int qd = hpb->qd;
	int i, j;

	INIT_LIST_HEAD(&hpb->lh_pre_req_free);
	INIT_LIST_HEAD(&hpb->lh_pre_req_dummy);

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	q = sdev->request_queue;

	hpb->pre_req = kzalloc(sizeof(struct ufshpb_req) * qd, GFP_KERNEL);
	if (!hpb->pre_req)
		return -ENOMEM;

	/*
	 * q->cmd_size: sizeof(struct scsi_cmd) + shost->hostt->cmd_size
	 */
	for (i = 0; i < qd; i++) {
		pre_req = hpb->pre_req + i;
		INIT_LIST_HEAD(&pre_req->list_req);
		pre_req->req = kzalloc(sizeof(struct request) + q->cmd_size,
				       GFP_KERNEL);
		if (!pre_req->req) {
			for (j = 0; j < i; j++)
				kfree(hpb->pre_req[j].req);
			goto release_mem;
		}

		pre_req->bio = bio_kmalloc(GFP_KERNEL, 1);
		if (!pre_req->bio) {
			kfree(hpb->pre_req[i].req);
			for (j = 0; j < i; j++) {
				kfree(hpb->pre_req[j].req);
				bio_put(hpb->pre_req[j].bio);
			}
			goto release_mem;
		}

		pre_req->wb.m_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
		if (!pre_req->wb.m_page) {
			kfree(hpb->pre_req[i].req);
			bio_put(hpb->pre_req[i].bio);
			for (j = 0; j < i; j++) {
				kfree(hpb->pre_req[j].req);
				bio_put(hpb->pre_req[j].bio);
				__free_page(hpb->pre_req[j].wb.m_page);
			}
			goto release_mem;
		}
		list_add_tail(&pre_req->list_req, &hpb->lh_pre_req_free);
	}

	return 0;
release_mem:
	kfree(hpb->pre_req);
	return -ENOMEM;
}

static void ufshpb_find_lu_qd(struct ufshpb_lu *hpb)
{
	struct scsi_device *sdev;
	struct ufs_hba *hba;

	sdev = hpb->ufsf->sdev_ufs_lu[hpb->lun];
	hba = hpb->ufsf->hba;

	/*
	 * ufshcd_slave_alloc(sdev) -> ufshcd_set_queue_depth(sdev)
	 * a lu-queue-depth compared with lu_info and hba->nutrs
	 * is selected in ufshcd_set_queue_depth()
	 */
	hpb->qd = sdev->queue_depth;
	INIT_INFO("lu %d queue_depth %d", hpb->lun, hpb->qd);
	if (!hpb->qd) {
		hpb->qd = hba->nutrs;
		INIT_INFO("lu_queue_depth is 0. we use device's queue info.");
		INIT_INFO("hba->nutrs = %d", hba->nutrs);
	}

	hpb->throttle_map_req = hpb->qd / 8;
	hpb->throttle_pre_req = hpb->qd / 2;
	hpb->throttle_rt_req = hpb->qd / 8;
	hpb->num_inflight_map_req = 0;
	hpb->num_inflight_pre_req = 0;
	hpb->num_inflight_rt_req = 0;
}

static void ufshpb_init_lu_constant(struct ufshpb_dev_info *hpb_dev_info,
				    struct ufshpb_lu *hpb)
{
	unsigned long long rgn_unit_size, rgn_mem_size;
	int entries_per_rgn;

	hpb->debug = false;

	ufshpb_find_lu_qd(hpb);

	/* for pre_req */
	hpb->pre_req_min_tr_len = HPB_MULTI_CHUNK_LOW;
	hpb->pre_req_max_tr_len = HPB_MULTI_CHUNK_HIGH;
	hpb->ctx_id_ticket = 0;

	/*	From descriptors	*/
	rgn_unit_size = (unsigned long long)
		SECTOR * (0x01 << hpb_dev_info->hpb_rgn_size);
	rgn_mem_size = rgn_unit_size / BLOCK * HPB_ENTRY_SIZE;

	hpb->srgn_unit_size = (unsigned long long)
		SECTOR * (0x01 << hpb_dev_info->hpb_srgn_size);
	hpb->srgn_mem_size =
		hpb->srgn_unit_size / BLOCK * HPB_ENTRY_SIZE;

	hpb->hpb_ver = hpb_dev_info->hpb_ver;

	/* relation : lu <-> region <-> sub region <-> entry */
	entries_per_rgn = rgn_mem_size / HPB_ENTRY_SIZE;
	hpb->entries_per_srgn = hpb->srgn_mem_size / HPB_ENTRY_SIZE;
#if BITS_PER_LONG == 32
	hpb->srgns_per_rgn = div_u64(rgn_mem_size, hpb->srgn_mem_size);

	/*
	 * regions_per_lu = (lu_num_blocks * 4096) / region_unit_size
	 *	          = (lu_num_blocks * HPB_ENTRY_SIZE) / region_mem_size
	 */
	hpb->rgns_per_lu =
		div_u64(((unsigned long long)hpb->lu_num_blocks
		 + (rgn_mem_size / HPB_ENTRY_SIZE) - 1),
		 (rgn_mem_size / HPB_ENTRY_SIZE));
	hpb->srgns_per_lu =
		div_u64(((unsigned long long)hpb->lu_num_blocks
		 + (hpb->srgn_mem_size / HPB_ENTRY_SIZE) - 1),
		(hpb->srgn_mem_size / HPB_ENTRY_SIZE));
#else
	hpb->srgns_per_rgn = rgn_mem_size / hpb->srgn_mem_size;

	/*
	 * regions_per_lu = (lu_num_blocks * 4096) / region_unit_size
	 *	          = (lu_num_blocks * HPB_ENTRY_SIZE) / region_mem_size
	 */
	hpb->rgns_per_lu =
		((unsigned long long)hpb->lu_num_blocks
		 + (rgn_mem_size / HPB_ENTRY_SIZE) - 1)
		/ (rgn_mem_size / HPB_ENTRY_SIZE);
	hpb->srgns_per_lu =
		((unsigned long long)hpb->lu_num_blocks
		 + (hpb->srgn_mem_size / HPB_ENTRY_SIZE) - 1)
		/ (hpb->srgn_mem_size / HPB_ENTRY_SIZE);
#endif

	/* mempool info */
	hpb->mpage_bytes = OS_PAGE_SIZE;
	hpb->mpages_per_srgn = hpb->srgn_mem_size / hpb->mpage_bytes;

	/* Bitmask Info. */
	hpb->dwords_per_srgn = hpb->entries_per_srgn / BITS_PER_DWORD;
	hpb->entries_per_rgn_shift = ffs(entries_per_rgn) - 1;
	hpb->entries_per_rgn_mask = entries_per_rgn - 1;
	hpb->entries_per_srgn_shift = ffs(hpb->entries_per_srgn) - 1;
	hpb->entries_per_srgn_mask = hpb->entries_per_srgn - 1;

	INIT_INFO("===== From Device Descriptor! =====");
	INIT_INFO("hpb_region_size = %d, hpb_subregion_size = %d",
		  hpb_dev_info->hpb_rgn_size,
		  hpb_dev_info->hpb_srgn_size);
	INIT_INFO("=====   Constant Values(LU)   =====");
	INIT_INFO("region_unit_size = %lld, region_mem_size %lld",
		  rgn_unit_size, rgn_mem_size);
	INIT_INFO("subregion_unit_size = %lld, subregion_mem_size %d",
		  hpb->srgn_unit_size, hpb->srgn_mem_size);

	INIT_INFO("lu_num_blocks = %d", hpb->lu_num_blocks);
	INIT_INFO("regions_per_lu = %d, subregions_per_lu = %d",
		  hpb->rgns_per_lu, hpb->srgns_per_lu);

	INIT_INFO("subregions_per_region = %d", hpb->srgns_per_rgn);
	INIT_INFO("entries_per_region %u shift %u mask 0x%X",
		  entries_per_rgn, hpb->entries_per_rgn_shift,
		  hpb->entries_per_rgn_mask);
	INIT_INFO("entries_per_subregion %u shift %u mask 0x%X",
		  hpb->entries_per_srgn, hpb->entries_per_srgn_shift,
		  hpb->entries_per_srgn_mask);
	INIT_INFO("mpages_per_subregion : %d", hpb->mpages_per_srgn);
	INIT_INFO("===================================\n");
}

static int ufshpb_lu_hpb_init(struct ufsf_feature *ufsf, int lun)
{
	struct ufshpb_lu *hpb = ufsf->ufshpb_lup[lun];
	struct ufshpb_region *rgn_table, *rgn;
	struct ufshpb_subregion *srgn;
	int rgn_idx, srgn_idx, total_srgn_cnt, srgn_cnt, i, ret = 0;
	bool do_work_handler = false;

	ufshpb_init_lu_constant(&ufsf->hpb_dev_info, hpb);

	/*if (hpb->hpb_ver >= 0x0220) {
		ret = ufshpb_issue_unset_rt_all_req(hpb);
		if (ret) {
			ERR_MSG("V2.2 spec cmd is not allowed.");
			return ret;
		}
	}*/

	rgn_table = vzalloc(sizeof(struct ufshpb_region) * hpb->rgns_per_lu);
	if (!rgn_table) {
		ret = -ENOMEM;
		goto out;
	}

	INIT_INFO("active_region_table bytes: %lu",
		  (sizeof(struct ufshpb_region) * hpb->rgns_per_lu));

	hpb->rgn_tbl = rgn_table;

	spin_lock_init(&hpb->hpb_lock);
	spin_lock_init(&hpb->rsp_list_lock);
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	atomic64_set(&hpb->act_rsp_list_cnt, 0);
	atomic64_set(&hpb->inact_rsp_list_cnt, 0);
#endif

	/* init lru information */
	INIT_LIST_HEAD(&hpb->lru_info.lh_lru_rgn);
	INIT_LIST_HEAD(&hpb->lru_info.lh_lru_rt_pinned_rgn);
	hpb->lru_info.selection_type = LRU;

	INIT_LIST_HEAD(&hpb->lh_pinned_srgn);
	INIT_LIST_HEAD(&hpb->lh_act_srgn);
	INIT_LIST_HEAD(&hpb->lh_inact_rgn);

	INIT_LIST_HEAD(&hpb->lh_map_ctx_free);

	ufshpb_init_jobs(hpb);

	ret = ufshpb_map_req_mempool_init(hpb);
	if (ret) {
		ERR_MSG("map_req_mempool init fail!");
		goto release_rgn_table;
	}

	ret = ufshpb_pre_req_mempool_init(hpb);
	if (ret) {
		ERR_MSG("pre_req_mempool init fail!");
		goto release_map_req_mempool;
	}

	ret = ufshpb_table_mempool_init(hpb);
	if (ret) {
		ERR_MSG("ppn table mempool init fail!");
		ufshpb_table_mempool_remove(hpb);
		goto release_pre_req_mempool;
	}

	total_srgn_cnt = hpb->srgns_per_lu;
	INIT_INFO("total_subregion_count: %d", total_srgn_cnt);
	for (rgn_idx = 0, srgn_cnt = 0; rgn_idx < hpb->rgns_per_lu;
        rgn_idx++, total_srgn_cnt -= srgn_cnt) {
        rgn = rgn_table + rgn_idx;
        rgn->rgn_idx = rgn_idx;
        rgn->total_write_count = 0;
        rgn->total_read_count  = 0;
        rgn->total_unmap_count = 0;

        rgn->active_threshold = 128;
        rgn->deactive_threshold = 16383; //64MB

        atomic64_set(&rgn->total_hit_cnt,0);
        atomic64_set(&rgn->total_miss_cnt,0);
        //atomic64_set(&rgn->chunksize_miss,0);
        //atomic64_set(&rgn->region_miss,0);
        atomic64_set(&rgn->active_cnt,0);
        //atomic64_set(&rgn->deactive_cnt,0);
        atomic64_set(&rgn->last_hit_cnt,0);
        atomic64_set(&rgn->last_miss_cnt, 0);
        atomic64_set(&rgn->last_write_cnt, 0);
        atomic_set(&rgn->reason, HPBUPDATE_NONE);

		INIT_LIST_HEAD(&rgn->list_inact_rgn);

		/* init lru region information*/
		INIT_LIST_HEAD(&rgn->list_lru_rgn);

		srgn_cnt = min(total_srgn_cnt, hpb->srgns_per_rgn);

		ret = ufshpb_alloc_subregion_tbl(hpb, rgn, srgn_cnt);
		if (ret)
			goto release_srgns;
		ufshpb_init_subregion_tbl(hpb, rgn);

		if (ufshpb_is_pinned_region(hpb, rgn_idx)) {
			ret = ufshpb_init_pinned_active_region(hpb, rgn);
			if (ret)
				goto release_srgns;

			do_work_handler = true;
		} else {
			rgn->rgn_state = HPBREGION_INACTIVE;
		}
	}

	if (total_srgn_cnt != 0) {
		ERR_MSG("error total_subregion_count: %d",
			total_srgn_cnt);
		goto release_srgns;
	}

	if (do_work_handler)
		schedule_work(&hpb->ufshpb_work);

	/*
	 * even if creating sysfs failed, ufshpb could run normally.
	 * so we don't deal with error handling
	 */
	ufshpb_create_sysfs(ufsf, hpb);

	return 0;
release_srgns:
	for (i = 0; i < rgn_idx; i++) {
		rgn = rgn_table + i;
		if (rgn->srgn_tbl) {
			for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt;
			     srgn_idx++) {
				srgn = rgn->srgn_tbl + srgn_idx;
				if (srgn->mctx)
					ufshpb_put_map_ctx(hpb, srgn->mctx);
			}
			kfree(rgn->srgn_tbl);
		}
	}

	ufshpb_table_mempool_remove(hpb);
release_pre_req_mempool:
	ufshpb_pre_req_mempool_remove(hpb);
release_map_req_mempool:
	ufshpb_map_req_mempool_remove(hpb);
release_rgn_table:
	vfree(rgn_table);
out:
	return ret;
}

static inline int ufshpb_version_check(struct ufshpb_dev_info *hpb_dev_info)
{
	if (hpb_dev_info->hpb_ver < UFSHPB_VER) {
		INIT_INFO("Driver = %.2x %.2x, Device = %.2x %.2x",
			  GET_BYTE_1(UFSHPB_VER), GET_BYTE_0(UFSHPB_VER),
			  GET_BYTE_1(hpb_dev_info->hpb_ver),
			  GET_BYTE_0(hpb_dev_info->hpb_ver));
		return -ENODEV;
	}
	return 0;
}

void ufshpb_get_dev_info(struct ufshpb_dev_info *hpb_dev_info, u8 *desc_buf)
{
	int ret;

	hpb_dev_info->hpb_device = false;

	if (desc_buf[DEVICE_DESC_PARAM_FEAT_SUP] & UFS_FEATURE_SUPPORT_HPB_BIT)
		INIT_INFO("bUFSFeaturesSupport: HPB is set");
	else {
		INIT_INFO("bUFSFeaturesSupport: HPB not support");
		return;
	}

	hpb_dev_info->hpb_ver = LI_EN_16(desc_buf + DEVICE_DESC_PARAM_HPB_VER);

	ret = ufshpb_version_check(hpb_dev_info);
	if (!ret)
		hpb_dev_info->hpb_device = true;
}

void ufshpb_get_geo_info(struct ufshpb_dev_info *hpb_dev_info, u8 *geo_buf)
{
	hpb_dev_info->hpb_number_lu = geo_buf[GEOMETRY_DESC_HPB_NUMBER_LU];
	if (hpb_dev_info->hpb_number_lu == 0) {
		ERR_MSG("Don't have a lu for hpb.");
		hpb_dev_info->hpb_device = false;
		return;
	}

	hpb_dev_info->hpb_rgn_size = geo_buf[GEOMETRY_DESC_HPB_REGION_SIZE];
	hpb_dev_info->hpb_srgn_size = geo_buf[GEOMETRY_DESC_HPB_SUBREGION_SIZE];
	hpb_dev_info->hpb_device_max_active_rgns =
		LI_EN_16(geo_buf + GEOMETRY_DESC_HPB_DEVICE_MAX_ACTIVE_REGIONS);

	INIT_INFO("[48] bHPBRegionSiz %u", hpb_dev_info->hpb_rgn_size);
	INIT_INFO("[49] bHPBNumberLU %u", hpb_dev_info->hpb_number_lu);
	INIT_INFO("[4A] bHPBSubRegionSize %u", hpb_dev_info->hpb_srgn_size);
	INIT_INFO("[4B:4C] wDeviceMaxActiveHPBRegions %u",
		  hpb_dev_info->hpb_device_max_active_rgns);

	ufshpb_init_constant();
}

int ufshpb_get_lu_info(struct ufsf_feature *ufsf, int lun, u8 *unit_buf)
{
	struct ufsf_lu_desc lu_desc;
	struct ufshpb_lu *hpb;

	lu_desc.lu_enable = unit_buf[UNIT_DESC_PARAM_LU_ENABLE];
	lu_desc.lu_queue_depth = unit_buf[UNIT_DESC_PARAM_LU_Q_DEPTH];
	lu_desc.lu_logblk_size = unit_buf[UNIT_DESC_PARAM_LOGICAL_BLK_SIZE];
	lu_desc.lu_logblk_cnt =
		LI_EN_64(unit_buf + UNIT_DESC_PARAM_LOGICAL_BLK_COUNT);
	lu_desc.lu_max_active_hpb_rgns =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_MAX_ACTIVE_REGIONS);
	lu_desc.lu_hpb_pinned_rgn_startidx =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_PIN_REGION_START_OFFSET);
	lu_desc.lu_num_hpb_pinned_rgns =
		LI_EN_16(unit_buf + UNIT_DESC_HPB_LU_NUM_PIN_REGIONS);

	if (lu_desc.lu_num_hpb_pinned_rgns > 0) {
		lu_desc.lu_hpb_pinned_end_offset =
			lu_desc.lu_hpb_pinned_rgn_startidx +
			lu_desc.lu_num_hpb_pinned_rgns - 1;
	} else
		lu_desc.lu_hpb_pinned_end_offset = -1;

	INIT_INFO("LUN(%d) [0A] bLogicalBlockSize %d",
		  lun, lu_desc.lu_logblk_size);
	INIT_INFO("LUN(%d) [0B] qLogicalBlockCount %llu",
		  lun, lu_desc.lu_logblk_cnt);
	INIT_INFO("LUN(%d) [03] bLuEnable %d", lun, lu_desc.lu_enable);
	INIT_INFO("LUN(%d) [06] bLuQueueDepth %d", lun, lu_desc.lu_queue_depth);
	INIT_INFO("LUN(%d) [23:24] wLUMaxActiveHPBRegions %d",
		  lun, lu_desc.lu_max_active_hpb_rgns);
	INIT_INFO("LUN(%d) [25:26] wHPBPinnedRegionStartIdx %d",
		  lun, lu_desc.lu_hpb_pinned_rgn_startidx);
	INIT_INFO("LUN(%d) [27:28] wNumHPBPinnedRegions %d",
		  lun, lu_desc.lu_num_hpb_pinned_rgns);
	INIT_INFO("LUN(%d) PINNED Start %d End %d",
		  lun, lu_desc.lu_hpb_pinned_rgn_startidx,
		  lu_desc.lu_hpb_pinned_end_offset);

	ufsf->ufshpb_lup[lun] = NULL;

	if (lu_desc.lu_enable == 0x02) {
		ufsf->ufshpb_lup[lun] = kzalloc(sizeof(struct ufshpb_lu),
						GFP_KERNEL);
		if (!ufsf->ufshpb_lup[lun])
			return -ENOMEM;

		hpb = ufsf->ufshpb_lup[lun];
		hpb->ufsf = ufsf;
		hpb->lun = lun;
		hpb->lu_num_blocks = lu_desc.lu_logblk_cnt;
		hpb->lu_max_active_rgns = lu_desc.lu_max_active_hpb_rgns;
		hpb->lru_info.max_lru_active_cnt =
			lu_desc.lu_max_active_hpb_rgns -
			lu_desc.lu_num_hpb_pinned_rgns;
		atomic64_set(&hpb->lru_info.active_cnt, 0);
		hpb->lru_info.max_update_lba_cnt = 0;
		hpb->lru_info.average_update_lba_cnt = 0;
		hpb->lu_pinned_rgn_startidx =
			lu_desc.lu_hpb_pinned_rgn_startidx;
		hpb->lu_pinned_end_offset = lu_desc.lu_hpb_pinned_end_offset;
	} else {
		INIT_INFO("===== LU %d is hpb-disabled.", lun);
		return -ENODEV;
	}

	return 0;
}

static void ufshpb_error_handler(struct work_struct *work)
{
	struct ufsf_feature *ufsf;

	ufsf = container_of(work, struct ufsf_feature, ufshpb_eh_work);

	WARNING_MSG("driver has failed. but UFSHCD can run without UFSHPB");
	WARNING_MSG("UFSHPB will be removed from the kernel");

	ufshpb_release(ufsf, HPB_FAILED);
}

static int ufshpb_init(struct ufsf_feature *ufsf)
{
	int lun, ret;
	int hpb_enabled_lun = 0;

	seq_scan_lu(lun) {
		if (!ufsf->ufshpb_lup[lun])
			continue;

		/*
		 * HPB need info about request queue in order to issue
		 * RB-CMD for pinned region.
		 */
		if (!ufsf->sdev_ufs_lu[lun]) {
			WARNING_MSG("warn: lun %d don't have scsi_device", lun);
			continue;
		}

		ret = ufshpb_lu_hpb_init(ufsf, lun);
		if (ret) {
			if (ret == -ENODEV)
				continue;
			else
				goto out_free_mem;
		}
		hpb_enabled_lun++;
	}

	if (hpb_enabled_lun == 0) {
		ERR_MSG("No UFSHPB LU to init");
		ret = -ENODEV;
		goto out_free_mem;
	}

	INIT_WORK(&ufsf->ufshpb_reset_work, ufshpb_reset_handler);
	INIT_WORK(&ufsf->ufshpb_eh_work, ufshpb_error_handler);

	kref_init(&ufsf->ufshpb_kref);
	ufsf->ufshpb_state = HPB_PRESENT;
	ufsf->issue_ioctl = false;

	seq_scan_lu(lun)
		if (ufsf->ufshpb_lup[lun])
			INFO_MSG("UFSHPB LU %d working", lun);

	return 0;
out_free_mem:
	seq_scan_lu(lun) {
		kfree(ufsf->ufshpb_lup[lun]);
		ufsf->ufshpb_lup[lun] = NULL;
}

	ufsf->ufshpb_state = HPB_FAILED;
	return ret;
}

static void ufshpb_drop_retry_list(struct ufshpb_lu *hpb)
{
	struct ufshpb_req *map_req, *next;
	unsigned long flags;

	if (list_empty(&hpb->lh_map_req_retry))
		return;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	list_for_each_entry_safe(map_req, next, &hpb->lh_map_req_retry,
				 list_req) {
		INFO_MSG("drop map_req %p ( %d - %d )", map_req,
			 map_req->rgn_idx, map_req->srgn_idx);

		list_del_init(&map_req->list_req);

		ufshpb_put_map_req(hpb, map_req);
	}
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
}

static void ufshpb_drop_rsp_lists(struct ufshpb_lu *hpb)
{
	struct ufshpb_region *rgn, *next_rgn;
	struct ufshpb_subregion *srgn, *next_srgn;
	unsigned long flags;

	spin_lock_irqsave(&hpb->rsp_list_lock, flags);
	list_for_each_entry_safe(rgn, next_rgn, &hpb->lh_inact_rgn,
				 list_inact_rgn) {
		list_del_init(&rgn->list_inact_rgn);
	}

	list_for_each_entry_safe(srgn, next_srgn, &hpb->lh_act_srgn,
				 list_act_srgn) {
		list_del_init(&srgn->list_act_srgn);
	}
	spin_unlock_irqrestore(&hpb->rsp_list_lock, flags);
}

static void ufshpb_destroy_subregion_tbl(struct ufshpb_lu *hpb,
					 struct ufshpb_region *rgn)
{
	int srgn_idx;

	for (srgn_idx = 0; srgn_idx < rgn->srgn_cnt; srgn_idx++) {
		struct ufshpb_subregion *srgn;

		srgn = rgn->srgn_tbl + srgn_idx;
		srgn->srgn_state = HPBSUBREGION_UNUSED;

		ufshpb_put_map_ctx(hpb, srgn->mctx);
	}

	kfree(rgn->srgn_tbl);
}

static void ufshpb_destroy_region_tbl(struct ufshpb_lu *hpb)
{
	int rgn_idx;

	RELEASE_INFO("Start");

	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		struct ufshpb_region *rgn;

		rgn = hpb->rgn_tbl + rgn_idx;
		if (rgn->rgn_state == HPBREGION_PINNED ||
		    rgn->rgn_state == HPBREGION_ACTIVE) {
			rgn->rgn_state = HPBREGION_INACTIVE;

			ufshpb_destroy_subregion_tbl(hpb, rgn);
		}
	}

	ufshpb_table_mempool_remove(hpb);
	vfree(hpb->rgn_tbl);

	RELEASE_INFO("End");
}

void ufshpb_release(struct ufsf_feature *ufsf, int state)
{
	struct ufshpb_lu *hpb;
	int lun;

	RELEASE_INFO("start release");
	ufsf->ufshpb_state = HPB_FAILED;

	RELEASE_INFO("kref count %d",
		     atomic_read(&ufsf->ufshpb_kref.refcount.refs));

	seq_scan_lu(lun) {
		hpb = ufsf->ufshpb_lup[lun];

		RELEASE_INFO("lun %d %p", lun, hpb);

		ufsf->ufshpb_lup[lun] = NULL;

		if (!hpb)
			continue;

		ufshpb_cancel_jobs(hpb);

		ufshpb_destroy_region_tbl(hpb);
		if (hpb->alloc_mctx != 0)
			WARNING_MSG("warning: alloc_mctx %d", hpb->alloc_mctx);

		ufshpb_map_req_mempool_remove(hpb);

		ufshpb_pre_req_mempool_remove(hpb);

		ufshpb_remove_sysfs(hpb);

		kfree(hpb);
	}

	ufsf->ufshpb_state = state;

	RELEASE_INFO("end release");
}

static void ufshpb_reset(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	seq_scan_lu(lun) {
		hpb = ufsf->ufshpb_lup[lun];
		if (hpb) {
			INFO_MSG("UFSHPB lun %d reset", lun);
			ufshpb_cancel_jobs(hpb);
			ufshpb_drop_retry_list(hpb);
			ufshpb_drop_rsp_lists(hpb);
			//ufshpb_purge_active_region(hpb);
		}
	}

	ufsf->ufshpb_state = HPB_PRESENT;
}

static inline int ufshpb_wait_kref_init_value(struct ufsf_feature *ufsf)
{
	return (atomic_read(&ufsf->ufshpb_kref.refcount.refs) == 1);
}

void ufshpb_reset_handler(struct work_struct *work)
{
	struct ufsf_feature *ufsf;
	int ret;

	ufsf = container_of(work, struct ufsf_feature, ufshpb_reset_work);

	init_waitqueue_head(&ufsf->wait_hpb);

	ret = wait_event_timeout(ufsf->wait_hpb,
				 ufshpb_wait_kref_init_value(ufsf),
				 msecs_to_jiffies(15000));
	if (ret == 0)
		ERR_MSG("UFSHPB kref is not init_value(=1). kref count = %d",
			atomic_read(&ufsf->ufshpb_kref.refcount.refs));

	ufshpb_reset(ufsf);
}

static inline int ufshpb_probe_lun_done(struct ufsf_feature *ufsf)
{
	return (ufsf->num_lu == ufsf->slave_conf_cnt);
}

void ufshpb_init_handler(struct work_struct *work)
{
	struct ufsf_feature *ufsf;
	int ret;

	ufsf = container_of(work, struct ufsf_feature, ufshpb_init_work);

	init_waitqueue_head(&ufsf->wait_hpb);

	ret = wait_event_timeout(ufsf->wait_hpb,
				 ufshpb_probe_lun_done(ufsf),
				 msecs_to_jiffies(10000));
	if (ret == 0)
		ERR_MSG("Probing LU is not fully complete.");

	INIT_INFO("HPB_INIT_START");

	ret = ufshpb_init(ufsf);
	if (ret)
		ERR_MSG("UFSHPB driver init failed. err %d", ret);
}

void ufshpb_suspend(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	seq_scan_lu(lun) {
		hpb = ufsf->ufshpb_lup[lun];
		if (hpb) {
			INFO_MSG("ufshpb_lu %d changes suspend state", lun);
			hpb->ufsf->ufshpb_state = HPB_SUSPEND;
			ufshpb_cancel_jobs(hpb);
		}
	}
}

void ufshpb_resume(struct ufsf_feature *ufsf)
{
	struct ufshpb_lu *hpb;
	int lun;

	seq_scan_lu(lun) {
		hpb = ufsf->ufshpb_lup[lun];
		if (hpb) {
			bool do_workq = false;
			bool do_retry_work = false;
			hpb->ufsf->ufshpb_state = HPB_PRESENT;

			do_workq = !ufshpb_is_empty_rsp_lists(hpb);
			do_retry_work =
				!list_empty_careful(&hpb->lh_map_req_retry);

			INFO_MSG("ufshpb_lu %d resume. do_workq %d retry %d",
				 lun, do_workq, do_retry_work);

			if (do_workq)
				schedule_work(&hpb->ufshpb_task_workq);
			if (do_retry_work)
				schedule_delayed_work(&hpb->ufshpb_retry_work,
						      msecs_to_jiffies(100));
		}
	}
}

static void ufshpb_stat_init(struct ufshpb_lu *hpb)
{
	atomic64_set(&hpb->hit, 0);
	atomic64_set(&hpb->miss, 0);
	atomic64_set(&hpb->rb_noti_cnt, 0);
	atomic64_set(&hpb->rb_active_cnt, 0);
	atomic64_set(&hpb->rb_inactive_cnt, 0);
	atomic64_set(&hpb->map_req_cnt, 0);
	atomic64_set(&hpb->pre_req_cnt, 0);
	atomic64_set(&hpb->set_rt_req_cnt, 0);
	atomic64_set(&hpb->unset_rt_req_cnt, 0);
	//atomic64_set(&hpb->miss_after_region_full, 0);
	//atomic64_set(&hpb->hit_after_region_full, 0);
	//atomic64_set(&hpb->total_access_region_after_region_full,0);
	//memset(&hpb->access_region, 0, 1024);// will replace by bitmap
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	ufshpb_debug_sys_init(hpb);
#endif
}

/* SYSFS functions */
static ssize_t ufshpb_sysfs_prep_disable_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "force_hpb_read_disable %d\n",
		       hpb->force_disable);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_prep_disable_store(struct ufshpb_lu *hpb,
					       const char *buf, size_t cnt)
{
	unsigned long value = 0;

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	if (value > 1)
		return -EINVAL;

	if (value == 1)
		hpb->force_disable = true;
	else if (value == 0)
		hpb->force_disable = false;

	SYSFS_INFO("force_hpb_read_disable %d", hpb->force_disable);

	return cnt;
}

static ssize_t ufshpb_sysfs_map_disable_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "force_map_req_disable %d\n",
		       hpb->force_map_req_disable);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_map_disable_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long value = 0;

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	if (value > 1)
		return -EINVAL;

	if (value == 1)
		hpb->force_map_req_disable = true;
	else if (value == 0)
		hpb->force_map_req_disable = false;

	SYSFS_INFO("force_map_req_disable %d", hpb->force_map_req_disable);

	return cnt;
}

static ssize_t ufshpb_sysfs_throttle_map_req_show(struct ufshpb_lu *hpb,
						  char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "throttle_map_req %d\n",
		       hpb->throttle_map_req);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_throttle_map_req_store(struct ufshpb_lu *hpb,
						   const char *buf, size_t cnt)
{
	unsigned long throttle_map_req = 0;

	if (kstrtoul(buf, 0, &throttle_map_req))
		return -EINVAL;

	hpb->throttle_map_req = (int)throttle_map_req;

	SYSFS_INFO("throttle_map_req %d", hpb->throttle_map_req);

	return cnt;
}

static ssize_t ufshpb_sysfs_throttle_pre_req_show(struct ufshpb_lu *hpb,
						  char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "throttle_pre_req %d\n",
		       hpb->throttle_pre_req);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_throttle_pre_req_store(struct ufshpb_lu *hpb,
						   const char *buf, size_t cnt)
{
	unsigned long throttle_pre_req = 0;

	if (kstrtoul(buf, 0, &throttle_pre_req))
		return -EINVAL;

	hpb->throttle_pre_req = (int)throttle_pre_req;

	SYSFS_INFO("throttle_pre_req %d", hpb->throttle_pre_req);

	return cnt;
}

static ssize_t ufshpb_sysfs_throttle_rt_req_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "throttle_rt_req %d\n", hpb->throttle_rt_req);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_throttle_rt_req_store(struct ufshpb_lu *hpb,
						  const char *buf, size_t cnt)
{
	unsigned long throttle_rt_req;

	if (kstrtoul(buf, 0, &throttle_rt_req))
		return -EINVAL;

	if (throttle_rt_req > 0 && throttle_rt_req <= 32)
		hpb->throttle_rt_req = (int)throttle_rt_req;
	else
		SYSFS_INFO("value is wrong. throttle_rt_req must be in [1~32] ");

	SYSFS_INFO("throttle_rt_req %d", hpb->throttle_rt_req);

	return cnt;
}

static ssize_t ufshpb_sysfs_pre_req_min_tr_len_show(struct ufshpb_lu *hpb,
						    char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d", hpb->pre_req_min_tr_len);
	SYSFS_INFO("pre_req min transfer len %d", hpb->pre_req_min_tr_len);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_min_tr_len_store(struct ufshpb_lu *hpb,
						     const char *buf,
						     size_t count)
{
	unsigned long val = 0;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0)
		val = 0;

	if (hpb->pre_req_max_tr_len < val || val < HPB_MULTI_CHUNK_LOW)
		SYSFS_INFO("value is wrong. pre_req transfer len %d ~ %d\n",
			   HPB_MULTI_CHUNK_LOW, hpb->pre_req_max_tr_len);
	else
		hpb->pre_req_min_tr_len = val;

	SYSFS_INFO("pre_req min transfer len %d", hpb->pre_req_min_tr_len);

	return count;
}

static ssize_t ufshpb_sysfs_pre_req_max_tr_len_show(struct ufshpb_lu *hpb,
						    char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "%d", hpb->pre_req_max_tr_len);
	SYSFS_INFO("pre_req max transfer len %d", hpb->pre_req_max_tr_len);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_max_tr_len_store(struct ufshpb_lu *hpb,
						     const char *buf,
						     size_t count)
{
	unsigned long val = 0;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (hpb->pre_req_min_tr_len > val || val > HPB_MULTI_CHUNK_HIGH)
		SYSFS_INFO("value is wrong. pre_req transfer len %d ~ %d\n",
			   hpb->pre_req_min_tr_len, HPB_MULTI_CHUNK_HIGH);
	else
		hpb->pre_req_max_tr_len = val;

	SYSFS_INFO("pre_req max transfer len %d", hpb->pre_req_max_tr_len);

	return count;
}

#if defined(CONFIG_HPB_DEBUG)
static ssize_t ufshpb_sysfs_debug_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "debug %d\n", hpb->debug);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_debug_store(struct ufshpb_lu *hpb,
					const char *buf, size_t cnt)
{
	unsigned long debug = 0;

	if (kstrtoul(buf, 0, &debug))
		return -EINVAL;

	if (debug >= 1)
		hpb->debug = 1;
	else
		hpb->debug = 0;

	SYSFS_INFO("debug %d", hpb->debug);

	return cnt;
}
#endif

#if defined(CONFIG_HPB_DEBUG_SYSFS)
static ssize_t ufshpb_sysfs_lba_info_show(struct ufshpb_lu *hpb, char *buf)
{
       struct ufshpb_region *rgn;
       struct ufshpb_subregion *srgn;
       enum HPBREGION_STATE rgn_state;
       enum HPBSUBREGION_STATE srgn_state;
       unsigned long lpn, flags;
       int rgn_idx, srgn_idx, srgn_offset;
       int ppn_clean_offset, ppn_clean_pinned_cnt = 0, ppn_clean_active_cnt = 0;
       unsigned long long pinned_valid_size = 0, active_valid_size = 0;
       int ret;

       lpn = hpb->lba_rgns_info / SECTORS_PER_BLOCK;

       ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx, &srgn_offset);

       rgn = hpb->rgn_tbl + rgn_idx;

       spin_lock_irqsave(&hpb->hpb_lock, flags);
       rgn_state = rgn->rgn_state;
       for (srgn_idx = 0; srgn_idx < hpb->srgns_per_rgn; srgn_idx++) {
	       srgn = rgn->srgn_tbl + srgn_idx;
	       srgn_state = srgn->srgn_state;

	       if (!ufshpb_valid_srgn(rgn, srgn))
		       continue;

	       if (!srgn->mctx || !srgn->mctx->ppn_dirty)
		       continue;

	       ppn_clean_offset = 0;

	       while (ppn_clean_offset < hpb->entries_per_srgn) {
		       ppn_clean_offset =
			       find_next_zero_bit((unsigned long *)srgn->mctx->ppn_dirty,
						  hpb->entries_per_srgn,
						  ppn_clean_offset);

		       if (ppn_clean_offset >= hpb->entries_per_srgn)
			       break;

		       if (rgn_state == HPBREGION_PINNED ||
			   rgn_state == HPBREGION_RT_PINNED)
			       ppn_clean_pinned_cnt++;
		       else if (rgn_state == HPBREGION_ACTIVE)
			       ppn_clean_active_cnt++;

		       ppn_clean_offset++;
	       }
       }

       pinned_valid_size = ppn_clean_pinned_cnt * BLOCK_KB;
       active_valid_size = ppn_clean_active_cnt * BLOCK_KB;

       ret = snprintf(buf, PAGE_SIZE,
		      "[LBA] %d [Region State] %d (0-INACTIVE, 1-ACTIVE, 2-PINNED, 3-RT_PINNED)\n"
		      "[Pinned] hit_count(low) %lld hit_count(high) %lld miss_count(low) %lld miss_count(high) %lld\n"
		      "[Active] hit_count(low) %lld hit_count(high) %lld miss_count(low) %lld miss_count(high) %lld\n"
		      "[Inactive] read_count(low) %lld read_count(high) %lld\n"
		      "[Pinned_rb_cnt] %lld [Active_rb_cnt] %lld\n"
		      "[Current_pinned_valid_size] %llu [Current_active_valid_size] %llu\n",
		      hpb->lba_rgns_info, rgn_state,
		      (long long)atomic64_read(&rgn->rgn_pinned_low_hit),
		      (long long)atomic64_read(&rgn->rgn_pinned_high_hit),
		      (long long)atomic64_read(&rgn->rgn_pinned_low_miss),
		      (long long)atomic64_read(&rgn->rgn_pinned_high_miss),
		      (long long)atomic64_read(&rgn->rgn_active_low_hit),
		      (long long)atomic64_read(&rgn->rgn_active_high_hit),
		      (long long)atomic64_read(&rgn->rgn_active_low_miss),
		      (long long)atomic64_read(&rgn->rgn_active_high_miss),
		      (long long)atomic64_read(&rgn->rgn_inactive_low_read),
		      (long long)atomic64_read(&rgn->rgn_inactive_high_read),
		      (long long)atomic64_read(&rgn->rgn_pinned_rb_cnt),
		      (long long)atomic64_read(&rgn->rgn_active_rb_cnt),
		      pinned_valid_size, active_valid_size);

       SYSFS_INFO("%s", buf);
       spin_unlock_irqrestore(&hpb->hpb_lock, flags);
       return ret;
}

static ssize_t ufshpb_sysfs_lba_info_store(struct ufshpb_lu *hpb,
					   const char *buf, size_t cnt)
{
       unsigned long value;

       if (kstrtoul(buf, 0, &value)) {
	       ERR_MSG("kstrtoul error");
	       return -EINVAL;
       }

       if (value > hpb->lu_num_blocks * SECTORS_PER_BLOCK) {
	       ERR_MSG("value %lu > lu_num_blocks %d error",
		       value, hpb->lu_num_blocks);
	       return -EINVAL;
       }

       hpb->lba_rgns_info = (int)value;

       SYSFS_INFO("lba_info %d", hpb->lba_rgns_info);

       return cnt;
}

static ssize_t ufshpb_sysfs_active_size_show(struct ufshpb_lu *hpb, char *buf)
{
       struct ufshpb_region *rgn;
       struct ufshpb_subregion *srgn;
       enum HPBREGION_STATE rgn_state;
       enum HPBSUBREGION_STATE srgn_state;
       int ret, rgn_idx, srgn_idx;
       int ppn_clean_offset, ppn_clean_pinned_cnt = 0, ppn_clean_active_cnt = 0;
       unsigned long flags;
       unsigned long long pinned_valid_size = 0, active_valid_size = 0;

       spin_lock_irqsave(&hpb->hpb_lock, flags);
       for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
	       rgn = hpb->rgn_tbl + rgn_idx;
	       rgn_state = rgn->rgn_state;

	       for (srgn_idx = 0; srgn_idx < hpb->srgns_per_rgn; srgn_idx++) {
		       srgn = rgn->srgn_tbl + srgn_idx;
		       srgn_state = srgn->srgn_state;

		       if (!ufshpb_valid_srgn(rgn, srgn))
			       continue;

		       if (!srgn->mctx || !srgn->mctx->ppn_dirty)
			       continue;

		       ppn_clean_offset = 0;

		       while (ppn_clean_offset < hpb->entries_per_srgn) {
			       ppn_clean_offset =
				       find_next_zero_bit((unsigned long *)srgn->mctx->ppn_dirty,
							  hpb->entries_per_srgn,
							  ppn_clean_offset);

			       if (ppn_clean_offset >= hpb->entries_per_srgn)
				       break;

			       if (rgn_state == HPBREGION_PINNED ||
				   rgn_state == HPBREGION_RT_PINNED)
				       ppn_clean_pinned_cnt++;
			       else if (rgn_state == HPBREGION_ACTIVE)
				       ppn_clean_active_cnt++;

			       ppn_clean_offset++;
		       }
	       }
       }
       spin_unlock_irqrestore(&hpb->hpb_lock, flags);
       pinned_valid_size = ppn_clean_pinned_cnt * BLOCK_KB;
       active_valid_size = ppn_clean_active_cnt * BLOCK_KB;

       ret = snprintf(buf, PAGE_SIZE,
		      "[Pinned] current_valid_size %llu [Active] current_valid_size %llu\n",
		      pinned_valid_size, active_valid_size);

       SYSFS_INFO("%s", buf);

       return ret;
}

static ssize_t ufshpb_sysfs_hpb_normal_read_count_show(struct ufshpb_lu *hpb,
						       char *buf)
{
	long long pinned_low_hit_cnt, pinned_high_hit_cnt;
	long long pinned_low_miss_cnt, pinned_high_miss_cnt;
	long long active_low_hit_cnt, active_high_hit_cnt;
	long long active_low_miss_cnt, active_high_miss_cnt;
	long long inactive_low_read_cnt, inactive_high_read_cnt;
	int ret;

	pinned_low_hit_cnt = atomic64_read(&hpb->pinned_low_hit);
	pinned_high_hit_cnt = atomic64_read(&hpb->pinned_high_hit);
	pinned_low_miss_cnt = atomic64_read(&hpb->pinned_low_miss);
	pinned_high_miss_cnt = atomic64_read(&hpb->pinned_high_miss);
	active_low_hit_cnt = atomic64_read(&hpb->active_low_hit);
	active_high_hit_cnt = atomic64_read(&hpb->active_high_hit);
	active_low_miss_cnt = atomic64_read(&hpb->active_low_miss);
	active_high_miss_cnt = atomic64_read(&hpb->active_high_miss);
	inactive_low_read_cnt = atomic64_read(&hpb->inactive_low_read);
	inactive_high_read_cnt = atomic64_read(&hpb->inactive_high_read);

	ret = snprintf(buf, PAGE_SIZE,
		       "[Pinned] hit_count(low) %lld hit_count(high) %lld miss_count(low) %lld miss_count(high) %lld\n"
		       "[Active] hit_count(low) %lld hit_count(high) %lld miss_count(low) %lld miss_count(high) %lld\n"
		       "[Inactive] read_count(low) %lld read_count(high) %lld\n",
		       pinned_low_hit_cnt, pinned_high_hit_cnt, pinned_low_miss_cnt, pinned_high_miss_cnt,
		       active_low_hit_cnt, active_high_hit_cnt, active_low_miss_cnt, active_high_miss_cnt,
		       inactive_low_read_cnt, inactive_high_read_cnt);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_rb_count_show(struct ufshpb_lu *hpb, char *buf)
{
       long long pinned_rb_cnt, active_rb_cnt;
       int ret;

       pinned_rb_cnt = atomic64_read(&hpb->pinned_rb_cnt);
       active_rb_cnt = atomic64_read(&hpb->active_rb_cnt);

       ret = snprintf(buf, PAGE_SIZE,
		      "[Pinned] rb_count %lld [Active] rb_count %lld\n",
		      pinned_rb_cnt, active_rb_cnt);

       SYSFS_INFO("%s", buf);

       return ret;
}

static ssize_t ufshpb_sysfs_rsp_list_cnt_show(struct ufshpb_lu *hpb, char *buf)
{
	long long act_cnt, inact_cnt;
	int ret;

	act_cnt = atomic64_read(&hpb->act_rsp_list_cnt);
	inact_cnt = atomic64_read(&hpb->inact_rsp_list_cnt);

	ret = snprintf(buf, PAGE_SIZE,
		       "act_rsp_list_cnt %lld inact_rsp_list_cnt %lld\n",
		       act_cnt, inact_cnt);

	SYSFS_INFO("%s", buf);

	return ret;
}
#endif

static ssize_t ufshpb_sysfs_version_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "HPB version %.4X D/D version %.6X%s\n",
		       hpb->hpb_ver, UFSHPB_DD_VER, UFSHPB_DD_VER_POST);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_hit_show(struct ufshpb_lu *hpb, char *buf)
{
	long long hit_cnt;
	int ret;
	int rgn_idx = 0;
	struct ufshpb_region *rgn;
	//printk(KERN_ERR "*****ufshpb_sysfs_hit_show,read latency:%d,write latency:%d,unmap latency:%d, load L2P latency:%d************\n",
	//total_read_latency,total_write_latency,total_unmap_latency,total_load_L2P_latency);
	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		rgn = hpb->rgn_tbl + rgn_idx;
		printk(KERN_ERR "[region] %d, [Region_State] %d,[Pinned]_hit_cnt(low) %lld, hit_cnt(high) %lld, miss_cnt(low) %lld, miss_cnt(high) %lld,[Active]_hit_cnt(low) %lld, hit_cnt(high) %lld, miss_cnt(low) %lld, miss_cnt(high) %lld,[Inactive]_read_cnt(low) %lld, read_cnt(high) %lld,[Pinned_rb_cnt] %lld, [Active_rb_cnt] %lld,[deactive_threshold] %d,[active_threshold] %d,[last_hit_cnt] %lld,[average_hit_cnt] %d\n",
			rgn_idx, rgn->rgn_state,
			(long long)atomic64_read(&rgn->rgn_pinned_low_hit),
			(long long)atomic64_read(&rgn->rgn_pinned_high_hit),
			(long long)atomic64_read(&rgn->rgn_pinned_low_miss),
			(long long)atomic64_read(&rgn->rgn_pinned_high_miss),
			(long long)atomic64_read(&rgn->rgn_active_low_hit),
			(long long)atomic64_read(&rgn->rgn_active_high_hit),
			(long long)atomic64_read(&rgn->rgn_active_low_miss),
			(long long)atomic64_read(&rgn->rgn_active_high_miss),
			(long long)atomic64_read(&rgn->rgn_inactive_low_read),
			(long long)atomic64_read(&rgn->rgn_inactive_high_read),
			(long long)atomic64_read(&rgn->rgn_pinned_rb_cnt),
			(long long)atomic64_read(&rgn->rgn_active_rb_cnt),
			rgn->deactive_threshold,rgn->active_threshold,
			(long long)atomic64_read(&rgn->last_hit_cnt),
			(long long)atomic64_read(&rgn->total_hit_cnt)/(long long)atomic64_read(&rgn->active_cnt));
	}

	hit_cnt = atomic64_read(&hpb->hit);

	ret = snprintf(buf, PAGE_SIZE, "hit_count %lld\n", hit_cnt);

	if (ret < 0)
		return ret;

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_miss_show(struct ufshpb_lu *hpb, char *buf)
{
	long long miss_cnt;
	int ret;

	miss_cnt = atomic64_read(&hpb->miss);

	ret = snprintf(buf, PAGE_SIZE, "miss_count %lld\n", miss_cnt);

	if (ret < 0)
		return ret;

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_map_req_show(struct ufshpb_lu *hpb, char *buf)
{
	long long rb_noti_cnt, rb_active_cnt, rb_inactive_cnt, map_req_cnt;
	int ret;

	rb_noti_cnt = atomic64_read(&hpb->rb_noti_cnt);
	rb_active_cnt = atomic64_read(&hpb->rb_active_cnt);
	rb_inactive_cnt = atomic64_read(&hpb->rb_inactive_cnt);
	map_req_cnt = atomic64_read(&hpb->map_req_cnt);

	ret = snprintf(buf, PAGE_SIZE,
		       "rb_noti %lld ACT %lld INACT %lld map_req_count %lld\n",
		       rb_noti_cnt, rb_active_cnt, rb_inactive_cnt,
		       map_req_cnt);

	if (ret < 0)
		return ret;

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_pre_req_show(struct ufshpb_lu *hpb, char *buf)
{
	long long pre_req_cnt;
	int ret;

	pre_req_cnt = atomic64_read(&hpb->pre_req_cnt);

	ret = snprintf(buf, PAGE_SIZE, "pre_req_count %lld\n", pre_req_cnt);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_rt_req_show(struct ufshpb_lu *hpb, char *buf)
{
	long long set_rt_cnt, unset_rt_cnt;
	int ret;

	set_rt_cnt = atomic64_read(&hpb->set_rt_req_cnt);
	unset_rt_cnt = atomic64_read(&hpb->unset_rt_req_cnt);

	ret = snprintf(buf, PAGE_SIZE,
		       "set_rt_count %lld unset_rt_count %lld\n",
		       set_rt_cnt, unset_rt_cnt);

	SYSFS_INFO("%s", buf);

	return ret;
}

static ssize_t ufshpb_sysfs_region_stat_show(struct ufshpb_lu *hpb, char *buf)
{
	int ret, pin_cnt = 0, rt_pin_cnt = 0, act_cnt = 0, inact_cnt = 0;
	int rgn_idx;
	enum HPBREGION_STATE state;

	for (rgn_idx = 0; rgn_idx < hpb->rgns_per_lu; rgn_idx++) {
		state = hpb->rgn_tbl[rgn_idx].rgn_state;
		if (state == HPBREGION_PINNED)
			pin_cnt++;
		else if (state == HPBREGION_RT_PINNED)
			rt_pin_cnt++;
		else if (state == HPBREGION_ACTIVE)
			act_cnt++;
		else if (state == HPBREGION_INACTIVE)
			inact_cnt++;
	}

	ret = snprintf(buf, PAGE_SIZE,
		       "Total %d pinned %d rt_pinned %d active %d inactive %d\n",
		       hpb->rgns_per_lu, pin_cnt, rt_pin_cnt, act_cnt,
		       inact_cnt);

	return ret;
}

static ssize_t ufshpb_sysfs_count_reset_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long debug;

	if (kstrtoul(buf, 0, &debug))
		return -EINVAL;

	SYSFS_INFO("Stat Init");

	ufshpb_stat_init(hpb);

	return cnt;
}

static ssize_t ufshpb_sysfs_info_lba_store(struct ufshpb_lu *hpb,
					   const char *buf, size_t cnt)
{
	struct ufshpb_region *rgn;
	struct ufshpb_subregion *srgn;
	unsigned long long ppn = 0;
	unsigned long value, lpn, flags;
	int rgn_idx, srgn_idx, srgn_offset, error = 0;

	if (kstrtoul(buf, 0, &value)) {
		ERR_MSG("kstrtoul error");
		return -EINVAL;
	}

	if (value > hpb->lu_num_blocks * SECTORS_PER_BLOCK) {
		ERR_MSG("value %lu > lu_num_blocks %d error",
			value, hpb->lu_num_blocks);
		return -EINVAL;
	}

	lpn = value / SECTORS_PER_BLOCK;

	ufshpb_get_pos_from_lpn(hpb, lpn, &rgn_idx, &srgn_idx, &srgn_offset);

	rgn = hpb->rgn_tbl + rgn_idx;
	srgn = rgn->srgn_tbl + srgn_idx;

	spin_lock_irqsave(&hpb->hpb_lock, flags);
	SYSFS_INFO("lba %lu lpn %lu region %d state %d subregion %d state %d",
		   value, lpn, rgn_idx, rgn->rgn_state, srgn_idx,
		   srgn->srgn_state);

	if (!ufshpb_valid_srgn(rgn, srgn)) {
		SYSFS_INFO("[region %d subregion %d] has not valid hpb info.",
			   rgn_idx, srgn_idx);
		goto out;
	}

	if (!srgn->mctx) {
		SYSFS_INFO("mctx is NULL");
		goto out;
	}

	ppn = ufshpb_get_ppn(srgn->mctx, srgn_offset, &error);
	if (error) {
		SYSFS_INFO("getting ppn is fail from a page.");
		goto out;
	}

	SYSFS_INFO("ppn %llx is_dirty %d", ppn,
		   ufshpb_ppn_dirty_check(hpb, lpn, 1));
out:
	spin_unlock_irqrestore(&hpb->hpb_lock, flags);
	return cnt;
}

static ssize_t ufshpb_sysfs_info_region_store(struct ufshpb_lu *hpb,
					      const char *buf, size_t cnt)
{
	unsigned long rgn_idx;
	int srgn_idx;

	if (kstrtoul(buf, 0, &rgn_idx))
		return -EINVAL;

	if (rgn_idx >= hpb->rgns_per_lu)
		ERR_MSG("error region %ld max %d", rgn_idx, hpb->rgns_per_lu);
	else {
		SYSFS_INFO("(region state : PINNED=%d ACTIVE=%d INACTIVE=%d)",
			   HPBREGION_PINNED, HPBREGION_ACTIVE,
			   HPBREGION_INACTIVE);

		SYSFS_INFO("region %ld state %d", rgn_idx,
			   hpb->rgn_tbl[rgn_idx].rgn_state);

		for (srgn_idx = 0; srgn_idx < hpb->rgn_tbl[rgn_idx].srgn_cnt;
		     srgn_idx++) {
			SYSFS_INFO("--- subregion %d state %d", srgn_idx,
				   hpb->rgn_tbl[rgn_idx].srgn_tbl[srgn_idx].srgn_state);
		}
	}

	return cnt;
}

static ssize_t ufshpb_sysfs_ufshpb_release_store(struct ufshpb_lu *hpb,
						 const char *buf, size_t cnt)
{
	unsigned long value = 0;

	SYSFS_INFO("start release function");

	if (kstrtoul(buf, 0, &value)) {
		ERR_MSG("kstrtoul error");
		return -EINVAL;
	}

	if (value == 0xab) {
		SYSFS_INFO("magic number %lu release start", value);
		goto err_out;
	} else
		SYSFS_INFO("wrong magic number %lu", value);

	return cnt;
err_out:
	SYSFS_INFO("ref_cnt %d",
		   atomic_read(&hpb->ufsf->ufshpb_kref.refcount.refs));
	ufshpb_failed(hpb, __func__);

	return cnt;
}

static struct ufshpb_sysfs_entry ufshpb_sysfs_entries[] = {
	__ATTR(hpb_read_disable, 0644,
	       ufshpb_sysfs_prep_disable_show, ufshpb_sysfs_prep_disable_store),
	__ATTR(map_cmd_disable, 0644,
	       ufshpb_sysfs_map_disable_show, ufshpb_sysfs_map_disable_store),
	__ATTR(throttle_map_req, 0644,
	       ufshpb_sysfs_throttle_map_req_show,
	       ufshpb_sysfs_throttle_map_req_store),
	__ATTR(throttle_pre_req, 0644,
	       ufshpb_sysfs_throttle_pre_req_show,
	       ufshpb_sysfs_throttle_pre_req_store),
	__ATTR(throttle_rt_req, 0644,
	       ufshpb_sysfs_throttle_rt_req_show,
	       ufshpb_sysfs_throttle_rt_req_store),
	__ATTR(pre_req_min_tr_len, 0644,
	       ufshpb_sysfs_pre_req_min_tr_len_show,
	       ufshpb_sysfs_pre_req_min_tr_len_store),
	__ATTR(pre_req_max_tr_len, 0644,
	       ufshpb_sysfs_pre_req_max_tr_len_show,
	       ufshpb_sysfs_pre_req_max_tr_len_store),
#if defined(CONFIG_HPB_DEBUG)
	__ATTR(debug, 0644,
	       ufshpb_sysfs_debug_show, ufshpb_sysfs_debug_store),
#endif
#if defined(CONFIG_HPB_DEBUG_SYSFS)
	__ATTR(lba_info, 0644,
	       ufshpb_sysfs_lba_info_show, ufshpb_sysfs_lba_info_store),
	__ATTR(active_size, 0444, ufshpb_sysfs_active_size_show, NULL),
	__ATTR(read_count, 0444, ufshpb_sysfs_hpb_normal_read_count_show, NULL),
	__ATTR(rb_count, 0444, ufshpb_sysfs_rb_count_show, NULL),
	__ATTR(rsp_list_count, 0444, ufshpb_sysfs_rsp_list_cnt_show, NULL),
#endif
	__ATTR(hpb_version, 0444, ufshpb_sysfs_version_show, NULL),
	__ATTR(hit_count, 0444, ufshpb_sysfs_hit_show, NULL),
	__ATTR(miss_count, 0444, ufshpb_sysfs_miss_show, NULL),
	__ATTR(map_req_count, 0444, ufshpb_sysfs_map_req_show, NULL),
	__ATTR(pre_req_count, 0444, ufshpb_sysfs_pre_req_show, NULL),
	__ATTR(rt_req_count, 0444, ufshpb_sysfs_rt_req_show, NULL),
	__ATTR(region_stat_count, 0444, ufshpb_sysfs_region_stat_show, NULL),
	__ATTR(count_reset, 0200, NULL, ufshpb_sysfs_count_reset_store),
	__ATTR(get_info_from_lba, 0200, NULL, ufshpb_sysfs_info_lba_store),
	__ATTR(get_info_from_region, 0200, NULL,
	       ufshpb_sysfs_info_region_store),
	__ATTR(release, 0200, NULL, ufshpb_sysfs_ufshpb_release_store),
	__ATTR_NULL
};

static ssize_t ufshpb_attr_show(struct kobject *kobj, struct attribute *attr,
				char *page)
{
	struct ufshpb_sysfs_entry *entry;
	struct ufshpb_lu *hpb;
	ssize_t error;

	entry = container_of(attr, struct ufshpb_sysfs_entry, attr);
	hpb = container_of(kobj, struct ufshpb_lu, kobj);

	if (!entry->show)
		return -EIO;

	mutex_lock(&hpb->sysfs_lock);
	error = entry->show(hpb, page);
	mutex_unlock(&hpb->sysfs_lock);
	return error;
}

static ssize_t ufshpb_attr_store(struct kobject *kobj, struct attribute *attr,
				 const char *page, size_t len)
{
	struct ufshpb_sysfs_entry *entry;
	struct ufshpb_lu *hpb;
	ssize_t error;

	entry = container_of(attr, struct ufshpb_sysfs_entry, attr);
	hpb = container_of(kobj, struct ufshpb_lu, kobj);

	if (!entry->store)
		return -EIO;

	mutex_lock(&hpb->sysfs_lock);
	error = entry->store(hpb, page, len);
	mutex_unlock(&hpb->sysfs_lock);
	return error;
}

static const struct sysfs_ops ufshpb_sysfs_ops = {
	.show = ufshpb_attr_show,
	.store = ufshpb_attr_store,
};

static struct kobj_type ufshpb_ktype = {
	.sysfs_ops = &ufshpb_sysfs_ops,
	.release = NULL,
};

static int ufshpb_create_sysfs(struct ufsf_feature *ufsf, struct ufshpb_lu *hpb)
{
	struct device *dev = ufsf->hba->dev;
	struct ufshpb_sysfs_entry *entry;
	int err;

	hpb->sysfs_entries = ufshpb_sysfs_entries;

	ufshpb_stat_init(hpb);

	kobject_init(&hpb->kobj, &ufshpb_ktype);
	mutex_init(&hpb->sysfs_lock);

	INIT_INFO("ufshpb creates sysfs lu %d %p dev->kobj %p", hpb->lun,
		  &hpb->kobj, &dev->kobj);

	err = kobject_add(&hpb->kobj, kobject_get(&dev->kobj),
			  "ufshpb_lu%d", hpb->lun);
	if (!err) {
		for (entry = hpb->sysfs_entries; entry->attr.name != NULL;
		     entry++) {
			INIT_INFO("ufshpb_lu%d sysfs attr creates: %s",
				  hpb->lun, entry->attr.name);
			if (sysfs_create_file(&hpb->kobj, &entry->attr))
				break;
		}
		INIT_INFO("ufshpb_lu%d sysfs adds uevent", hpb->lun);
		kobject_uevent(&hpb->kobj, KOBJ_ADD);
	}

	return err;
}

static int ufshpb_remove_sysfs(struct ufshpb_lu *hpb)
{
	struct ufshpb_sysfs_entry *entry;

	for (entry = hpb->sysfs_entries; entry->attr.name != NULL;
	     entry++) {
		INIT_INFO("ufshpb_lu%d sysfs attr removes: %s",
			  hpb->lun, entry->attr.name);
		sysfs_remove_file(&hpb->kobj, &entry->attr);
	}
	kobject_uevent(&hpb->kobj, KOBJ_REMOVE);

	INIT_INFO("ufshpb removes sysfs lu %d %p ", hpb->lun, &hpb->kobj);
	kobject_del(&hpb->kobj);

	return 0;
}
