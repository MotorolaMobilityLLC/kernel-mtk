#include <fde_aes.h>
#include <fde_aes_dbg.h>

struct mmc_queue_req *msdc_mrq_get_mqrq(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct mmc_command *cmd;
	struct mmc_blk_request *brq;
	struct mmc_queue_req *mq_rq = NULL;
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	struct mmc_async_req *areq;
#endif

	if (mrq->host != mmc)
		return NULL;

	cmd = mrq->cmd;

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	/* Hardware Command Queue of eMMC */
	if (mmc->card && mmc->card->ext_csd.cmdq_mode_en) {
		if (check_mmc_cmd4647(cmd->opcode)) {
			areq = mmc->areq_que[(cmd->arg >> 16) & 0x1f];
			mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
		}
	} else
#endif

	/* Read Write Command of eMMC */
	if (check_mmc_cmd1718(cmd->opcode) || check_mmc_cmd2425(cmd->opcode)) {
		brq = container_of(mrq, struct mmc_blk_request, mrq);
		mq_rq = container_of(brq, struct mmc_queue_req, brq);
	}

	return mq_rq;
}

int msdc_check_fde_err(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_queue_req *mq_rq;
	void __iomem *base = host->base;

	if (!mrq || !mmc->card)
		return 0;

	cmd = mrq->cmd;

	mq_rq = msdc_mrq_get_mqrq(mmc, mrq);
	if (mq_rq && mq_rq->req && mq_rq->req->bio && mq_rq->req->bio->bi_hw_fde) {
		pr_notice("MSDC%d HW FDE error handling\n", host->id);
		msdc_reset(host->id);
		msdc_dma_clear(host);
		msdc_clr_fifo(host->id);
		msdc_clr_int();

		if (host->dma.mode == MSDC_MODE_DMA_DESC)
			host->dma.gpd->bdp = 0;
		MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);
		msdc_dma_stop(host);
		MSDC_WRITE32(MSDC_AES_SEL, 0);
		if (host->dma.mode == MSDC_MODE_DMA_DESC)
			host->dma.gpd->bdp = 1;
		msdc_reset(host->id);
		msdc_clr_fifo(host->id);
		return 1;
	}

	return 0;
}

static int msdc_check_fde_enable(struct mmc_queue_req *mq_rq, int id, u32 opcode)
{
	if (fde_aes_check_cmd(FDE_AES_EN_FDE, 1, id)) {
		FDEERR("%s manual control FDE %d\n", __func__, fde_aes_get_fde());
		return fde_aes_get_fde();
	}

	if (fde_aes_check_cmd(FDE_AES_EN_RAW, fde_aes_get_raw(), id)) {
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
		if (check_mmc_cmd46(opcode))
			return 0;
#endif
		if (check_mmc_cmd1718(opcode))
			return 0;
	}

	if (!mq_rq || !mq_rq->req || !mq_rq->req->bio)
		return 0;

	return mq_rq->req->bio->bi_hw_fde;
}

static void msdc_check_fde(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_blk_request *brq;
	struct mmc_queue_req *mq_rq;
	void __iomem *base = host->base;
	u32 blk_addr, blk_addr_e;

	cmd = mrq->cmd;
	WARN_ON(MSDC_READ32(MSDC_AES_SEL));

	if (host->hw == NULL ||
	    host->hw->host_function == MSDC_SDIO)
		return;

	mq_rq = msdc_mrq_get_mqrq(mmc, mrq);

	if (mq_rq) {
		#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
		if (check_mmc_cmd4647(cmd->opcode)) {
			if (!mmc_card_blockaddr(mmc->card))
				blk_addr = mq_rq->brq.que.arg >> 9;
			else
				blk_addr = mq_rq->brq.que.arg;
		} else
		#endif
		{
			if (!mmc_card_blockaddr(mmc->card))
				blk_addr = cmd->arg >> 9;
			else
				blk_addr = cmd->arg;
		}

		if (fde_aes_check_cmd(FDE_AES_CK_RANGE, fde_aes_get_range(), host->id)) {
			blk_addr_e = blk_addr + (host->dma.xfersz >> 9) - 1;
			if (blk_addr < fde_aes_get_range_start())
				FDEERR("%s MSDC%d err S range 0x%x 0x%x\n",
					__func__, host->id, blk_addr, blk_addr_e);
			if (blk_addr_e > fde_aes_get_range_end())
				FDEERR("%s MSDC%d err E range 0x%x 0x%x\n",
					__func__, host->id, blk_addr, blk_addr_e);
		}

		if (fde_aes_check_cmd(FDE_AES_EN_MSG, fde_aes_get_log(), host->id))
			FDEERR("%s MSDC%d CMD%d block 0x%x+0x%x FDE %d\n",
				__func__, host->id, cmd->opcode,
				blk_addr, host->dma.xfersz >> 9,
				msdc_check_fde_enable(mq_rq, host->id, cmd->opcode));

		if (msdc_check_fde_enable(mq_rq, host->id, cmd->opcode)) { /* Encrypted */
			/* Check data size with 16bytes */
			WARN_ON(host->dma.xfersz & 0xf);
			/* Check data addressw with 16bytes alignment */
			WARN_ON((host->dma.gpd_addr & 0xf) || (host->dma.bd_addr & 0xf));
			brq = &mq_rq->brq;
			fde_aes_exec(host->id, blk_addr, cmd->opcode);
			MSDC_WRITE32(MSDC_AES_SEL, 0x120 | host->id);
			return;
		}
	}
}
