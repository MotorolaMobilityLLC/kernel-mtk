/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>
#include <queue.h>
#include "mt_sd.h"
#include <mt-plat/sd_misc.h>

#ifndef FPGA_PLATFORM
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#endif

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <core.h>


#define PARTITION_NAME_LENGTH    (64)
#define DRV_NAME_MISC            "misc-sd"

#define DEBUG_MMC_IOCTL   0
#define DEBUG_MSDC_SSC    1
/*
 * For simple_sd_ioctl
 */
#define FORCE_IN_DMA (0x11)
#define FORCE_IN_PIO (0x10)
#define FORCE_NOTHING (0x0)

static int dma_force[HOST_MAX_NUM] =	/* used for sd ioctrol */
{
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING,
};

#define dma_is_forced(host_id)     (dma_force[host_id] & 0x10)
#define get_forced_transfer_mode(host_id)  (dma_force[host_id] & 0x01)


static u32 *sg_msdc_multi_buffer;

static int simple_sd_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sd_ioctl_reinit(void)
{
	struct msdc_host *host = msdc_get_host(MSDC_SD, 0, 0);

	if (NULL != host)
		return msdc_reinit(host);
	else
		return -EINVAL;
}

static int sd_ioctl_cd_pin_en(void)
{
	struct msdc_host *host = msdc_get_host(MSDC_SD, 0, 0);

	if (NULL != host) {
		if (host->hw->host_function == MSDC_SD)
			return ((host->hw->flags & MSDC_CD_PIN_EN) == MSDC_CD_PIN_EN);
		else
			return -EINVAL;
	} else
		return -EINVAL;
}

static void simple_sd_get_driving(struct msdc_host *host,
	struct msdc_ioctl *msdc_ctl)
{
	switch (host->id) {
	case 0:
		sdr_get_field(MSDC0_GPIO_DRV0_G5_ADDR,
			MSDC0_DRV_CMD_MASK, msdc_ctl->cmd_pu_driving);
		sdr_get_field(MSDC0_GPIO_DRV0_G5_ADDR,
			MSDC0_DRV_DSL_MASK, msdc_ctl->ds_pu_driving);
		sdr_get_field(MSDC0_GPIO_DRV0_G5_ADDR,
			MSDC0_DRV_CLK_MASK, msdc_ctl->clk_pu_driving);
		sdr_get_field(MSDC0_GPIO_DRV0_G5_ADDR,
			MSDC0_DRV_DAT_MASK, msdc_ctl->dat_pu_driving);
		sdr_get_field(MSDC0_GPIO_DRV0_G5_ADDR,
			MSDC0_DRV_RSTB_MASK, msdc_ctl->rst_pu_driving);
		break;
	case 1:
		sdr_get_field(MSDC1_GPIO_DRV0_G4_ADDR,
			MSDC1_DRV_CMD_MASK, msdc_ctl->cmd_pu_driving);
		sdr_get_field(MSDC1_GPIO_DRV0_G4_ADDR,
			MSDC1_DRV_CLK_MASK, msdc_ctl->clk_pu_driving);
		sdr_get_field(MSDC1_GPIO_DRV0_G4_ADDR,
			MSDC1_DRV_DAT_MASK, msdc_ctl->dat_pu_driving);
		msdc_ctl->rst_pu_driving = 0;
		msdc_ctl->ds_pu_driving = 0;
		break;
#ifdef CFG_DEV_MSDC2 /* FIXME: For 6630 */
	case 2:
		sdr_get_field(MSDC2_GPIO_DRV0_G0_ADDR,
			MSDC2_DRV_CMD_MASK, msdc_ctl->cmd_pu_driving);
		sdr_get_field(MSDC2_GPIO_DRV0_G0_ADDR,
			MSDC2_DRV_CLK_MASK, msdc_ctl->clk_pu_driving);
		sdr_get_field(MSDC2_GPIO_DRV0_G0_ADDR,
			MSDC2_DRV_DAT_MASK, msdc_ctl->dat_pu_driving);
		msdc_ctl->rst_pu_driving = 0;
		msdc_ctl->ds_pu_driving = 0;
		break;
#endif
	default:
		pr_err("error...[%s] host->id out of range!!!\n", __func__);
		break;
	}
}

static int simple_sd_ioctl_multi_rw(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_sbc;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;
	int ret = 0;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;
	if (msdc_ctl->host_num != 0)
		return -EINVAL;
	if (msdc_ctl->total_size <= 0)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

	msdc_ctl->result = 0;
	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_sbc, 0, sizeof(struct mmc_command));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
	memset(&msdc_stop, 0, sizeof(struct mmc_command));

	/*
	 * workaround here, if sd.c has remove baisc DMA 64KB limit
	 * sg_msdc_multi_buffer should alloc size :max_reg_size,not 64KB
	 * at simple_sd_init function.
	 */
	if (msdc_ctl->total_size > (64 * 1024)) {
		pr_err("%s [%d]: read or write size excced 64KB\n", __func__, __LINE__);
		ret = -EFAULT;
		goto out;
	}

	if (msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer,
				msdc_ctl->total_size)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
				goto out;
			}
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, msdc_ctl->total_size);
		}
	}

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want access %d partition\n", msdc_ctl->partition);
#endif

	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_debug("mmc_send_ext_csd error, multi rw\n");
		goto release;
	}
#ifdef CONFIG_MTK_EMMC_SUPPORT
	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		if (0x1 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 1 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x1;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	case EMMC_PART_BOOT2:
		if (0x2 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 2 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x2;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	default:
		/* make sure access partition is user data area */
		if (0 != (l_buf[179] & 0x7)) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	}
#endif

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if (msdc_ctl->trans_type)
		dma_force[host_ctl->id] = FORCE_IN_DMA;
	else
		dma_force[host_ctl->id] = FORCE_IN_PIO;

	if (msdc_ctl->iswrite) {
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		memset(sg_msdc_multi_buffer, 0, msdc_ctl->total_size);
	}

#ifdef MTK_MSDC_USE_CMD23
	if ((mmc_card_mmc(host_ctl->mmc->card)
	     || (mmc_card_sd(host_ctl->mmc->card)
		 && host_ctl->mmc->card->scr.cmds & SD_SCR_CMD23_SUPPORT))
	    && !(host_ctl->mmc->card->quirks & MMC_QUIRK_BLK_NO_CMD23)) {
		msdc_mrq.sbc = &msdc_sbc;
		msdc_mrq.sbc->opcode = MMC_SET_BLOCK_COUNT;
#ifdef MTK_MSDC_USE_CACHE
		/* if ioctl access cacheable partition data, there is on flush mechanism in msdc driver
		 * so do reliable write .*/
		if (mmc_card_mmc(host_ctl->mmc->card)
		    && (host_ctl->mmc->card->ext_csd.cache_ctrl & 0x1)
		    && (msdc_cmd.opcode == MMC_WRITE_MULTIPLE_BLOCK))
			msdc_mrq.sbc->arg = msdc_data.blocks | (1 << 31);
		else
			msdc_mrq.sbc->arg = msdc_data.blocks;
#else
		msdc_mrq.sbc->arg = msdc_data.blocks;
#endif
		msdc_mrq.sbc->flags = MMC_RSP_R1 | MMC_CMD_AC;
	}
#endif

	msdc_cmd.arg = msdc_ctl->address;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("this device use byte address!!\n");
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	msdc_data.stop = &msdc_stop;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	pr_debug("total size is %d\n", msdc_ctl->total_size);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);
	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (msdc_ctl->partition) {
		ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
		if (ret) {
			pr_debug("mmc_send_ext_csd error, multi rw2\n");
			goto release;
		}

		if (l_buf[179] & 0x7) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
	}

release:
	mmc_release_host(host_ctl->mmc);
	if (!ret && !msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_to_user(msdc_ctl->buffer, sg_msdc_multi_buffer,
				msdc_ctl->total_size)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
			}
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer, msdc_ctl->total_size);
		}
	}
	/* clear the global buffer of R/W IOCTL */
	memset(sg_msdc_multi_buffer, 0 , msdc_ctl->total_size);

out:
	if (ret)
		msdc_ctl->result = ret;

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result = msdc_data.error;

	dma_force[host_ctl->id] = FORCE_NOTHING;
	return msdc_ctl->result;

}

static int simple_sd_ioctl_single_rw(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
	int ret = 0;

	if (!msdc_ctl)
		return -EINVAL;
	if (msdc_ctl->host_num != 0)
		return -EINVAL;
	if (msdc_ctl->total_size <= 0)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

	msdc_ctl->result = 0;
	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
#ifdef MTK_MSDC_USE_CACHE
	if (msdc_ctl->iswrite && mmc_card_mmc(host_ctl->mmc->card)
	    && (host_ctl->mmc->card->ext_csd.cache_ctrl & 0x1))
		return simple_sd_ioctl_multi_rw(msdc_ctl);
#endif
	if (msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer, 512)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
				goto out;
			}
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, 512);
		}
	} else {
		memset(sg_msdc_multi_buffer, 0 , 512);
	}

	if (msdc_ctl->total_size > 512) {
		ret = -EFAULT;
		goto out;
	}

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want access %d partition\n", msdc_ctl->partition);
#endif

	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_err("%s:%d:get ext_csd error,ret=%d\n", __func__, __LINE__, ret);
		goto release;
	}
#ifdef CONFIG_MTK_EMMC_SUPPORT
	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		if (0x1 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 1 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x1;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	case EMMC_PART_BOOT2:
		if (0x2 != (l_buf[179] & 0x7)) {
			/* change to access boot partition 2 */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x2;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	default:
		/* make sure access partition is user data area */
		if (0 != (l_buf[179] & 0x7)) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
		break;
	}
#endif

#if DEBUG_MMC_IOCTL
	pr_debug("start MSDC_SINGLE_READ_WRITE !!\n");
#endif

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if (msdc_ctl->trans_type)
		dma_force[host_ctl->id] = FORCE_IN_DMA;
	else
		dma_force[host_ctl->id] = FORCE_IN_PIO;

	if (msdc_ctl->iswrite) {
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;

		memset(sg_msdc_multi_buffer, 0, 512);
	}

	msdc_cmd.arg = msdc_ctl->address;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	pr_debug("single block: ueser buf address is 0x%p!\n", msdc_ctl->buffer);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (msdc_ctl->partition) {
		ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
		if (ret) {
			pr_err("%s:%d:get ext_csd error,ret=%d\n", __func__, __LINE__, ret);
			goto release;
		}

		if (l_buf[179] & 0x7) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
				179, l_buf[179], 1000);
		}
	}

release:
	mmc_release_host(host_ctl->mmc);
	if (!ret && !msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_to_user(msdc_ctl->buffer, sg_msdc_multi_buffer, 512)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
			}
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer, 512);
		}
	}

	/* clear the global buffer of R/W IOCTL */
	memset(sg_msdc_multi_buffer, 0, 512);
out:
	if (ret)
		msdc_ctl->result = ret;

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result = msdc_data.error;

	dma_force[host_ctl->id] = FORCE_NOTHING;
	return msdc_ctl->result;
}

int simple_sd_ioctl_rw(struct msdc_ioctl *msdc_ctl)
{
	if ((msdc_ctl->total_size > 512) && (msdc_ctl->total_size <= MAX_REQ_SZ))
		return simple_sd_ioctl_multi_rw(msdc_ctl);
	else
		return simple_sd_ioctl_single_rw(msdc_ctl);
}

static int simple_sd_ioctl_get_cid(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

#if DEBUG_MMC_IOCTL
	pr_debug("user want the cid in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_cid, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("cid:0x%x,0x%x,0x%x,0x%x\n",
		 host_ctl->mmc->card->raw_cid[0], host_ctl->mmc->card->raw_cid[1],
		 host_ctl->mmc->card->raw_cid[2], host_ctl->mmc->card->raw_cid[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_csd(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

#if DEBUG_MMC_IOCTL
	pr_debug("user want the csd in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_csd, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("csd:0x%x,0x%x,0x%x,0x%x\n",
		 host_ctl->mmc->card->raw_csd[0], host_ctl->mmc->card->raw_csd[1],
		 host_ctl->mmc->card->raw_csd[2], host_ctl->mmc->card->raw_csd[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_excsd(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct msdc_host *host_ctl;
	int ret = 0;
#if DEBUG_MMC_IOCTL
	int i;
#endif

	if (!msdc_ctl)
		goto out;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	if (IS_ERR_OR_NULL(host_ctl))
		goto out;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		goto out;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		goto out;

	mmc_claim_host(host_ctl->mmc);
	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	mmc_release_host(host_ctl->mmc);
	if (ret)
		goto out;
	if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
		goto out;

#if DEBUG_MMC_IOCTL
	for (i = 0; i < 512; i++) {
		pr_debug("%x", l_buf[i]);
		if (0 == ((i + 1) % 16))
			pr_debug("\n");
	}
#endif
	return 0;
out:
	return -EINVAL;
}


static int simple_sd_ioctl_set_driving(struct msdc_ioctl *msdc_ctl)
{
	void __iomem *base;
	struct msdc_host *host;
#if DEBUG_MMC_IOCTL
	unsigned int l_value;
#endif

	if (!msdc_ctl)
		return -EINVAL;

	if ((msdc_ctl->host_num < 0) || (msdc_ctl->host_num >= HOST_MAX_NUM)) {
		pr_err("invalid host num: %d\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	if (mtk_msdc_host[msdc_ctl->host_num])
		host = mtk_msdc_host[msdc_ctl->host_num];
	else {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	base = host->base;
#ifndef FPGA_PLATFORM
#if DEBUG_MMC_IOCTL
	pr_debug("set: clk driving is 0x%x\n", msdc_ctl->clk_pu_driving);
	pr_debug("set: cmd driving is 0x%x\n", msdc_ctl->cmd_pu_driving);
	pr_debug("set: dat driving is 0x%x\n", msdc_ctl->dat_pu_driving);
	pr_debug("set: rst driving is 0x%x\n", msdc_ctl->rst_pu_driving);
	pr_debug("set: ds driving is 0x%x\n", msdc_ctl->ds_pu_driving);
#endif
	host->hw->clk_drv = msdc_ctl->clk_pu_driving;
	host->hw->cmd_drv = msdc_ctl->cmd_pu_driving;
	host->hw->dat_drv = msdc_ctl->dat_pu_driving;
	host->hw->rst_drv = msdc_ctl->rst_pu_driving;
	host->hw->ds_drv = msdc_ctl->ds_pu_driving;
	host->hw->clk_drv_sd_18 = msdc_ctl->clk_pu_driving;
	host->hw->cmd_drv_sd_18 = msdc_ctl->cmd_pu_driving;
	host->hw->dat_drv_sd_18 = msdc_ctl->dat_pu_driving;
	msdc_set_driving(host, host->hw, 0);
#endif

	return 0;
}

static int simple_sd_ioctl_get_driving(struct msdc_ioctl *msdc_ctl)
{
	void __iomem *base;
/* unsigned int l_value; */
	struct msdc_host *host;

	if (!msdc_ctl)
		return -EINVAL;

	if ((msdc_ctl->host_num < 0) || (msdc_ctl->host_num >= HOST_MAX_NUM)) {
		pr_err("invalid host num: %d\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	if (mtk_msdc_host[msdc_ctl->host_num])
		host = mtk_msdc_host[msdc_ctl->host_num];
	else {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	base = host->base;
#ifndef FPGA_PLATFORM
	simple_sd_get_driving(mtk_msdc_host[msdc_ctl->host_num], msdc_ctl);
#if DEBUG_MMC_IOCTL
	pr_debug("read: clk driving is 0x%x\n", msdc_ctl->clk_pu_driving);
	pr_debug("read: cmd driving is 0x%x\n", msdc_ctl->cmd_pu_driving);
	pr_debug("read: dat driving is 0x%x\n", msdc_ctl->dat_pu_driving);
	pr_debug("read: rst driving is 0x%x\n", msdc_ctl->rst_pu_driving);
	pr_debug("read: ds driving is 0x%x\n", msdc_ctl->ds_pu_driving);
#endif
#endif

	return 0;
}

static int simple_sd_ioctl_sd30_mode_switch(struct msdc_ioctl *msdc_ctl)
{
/* u32 base; */
/* struct msdc_hw hw; */
	int id;
#if DEBUG_MMC_IOCTL
	unsigned int l_value;
#endif

	if (!msdc_ctl)
		return -EINVAL;
	id = msdc_ctl->host_num;

	if ((msdc_ctl->host_num < 0) || (msdc_ctl->host_num >= HOST_MAX_NUM)) {
		pr_err("invalid host num: %d\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	if (!mtk_msdc_host[msdc_ctl->host_num]) {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	switch (msdc_ctl->sd30_mode) {
	case SDHC_HIGHSPEED:
		msdc_host_mode[id] |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
		msdc_host_mode[id] &= (~MMC_CAP_UHS_SDR12) & (~MMC_CAP_UHS_SDR25) &
		    (~MMC_CAP_UHS_SDR50) & (~MMC_CAP_UHS_DDR50) &
		    (~MMC_CAP_1_8V_DDR) & (~MMC_CAP_UHS_SDR104);
		msdc_host_mode2[id] &= (~MMC_CAP2_HS200_1_8V_SDR) & (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support Highspeed\n");
		break;
	case UHS_SDR12:
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12;
		msdc_host_mode[id] &= (~MMC_CAP_UHS_SDR25) & (~MMC_CAP_UHS_SDR50) &
		    (~MMC_CAP_UHS_DDR50) & (~MMC_CAP_1_8V_DDR) & (~MMC_CAP_UHS_SDR104);
		msdc_host_mode2[id] &= (~MMC_CAP2_HS200_1_8V_SDR) & (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support UHS-SDR12\n");
		break;
	case UHS_SDR25:
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25;
		msdc_host_mode[id] &= (~MMC_CAP_UHS_SDR50) & (~MMC_CAP_UHS_DDR50) &
		    (~MMC_CAP_1_8V_DDR) & (~MMC_CAP_UHS_SDR104);
		msdc_host_mode2[id] &= (~MMC_CAP2_HS200_1_8V_SDR) & (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support UHS-SDR25\n");
		break;
	case UHS_SDR50:
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50;
		msdc_host_mode[id] &= (~MMC_CAP_UHS_DDR50) & (~MMC_CAP_1_8V_DDR) &
		    (~MMC_CAP_UHS_SDR104);
		msdc_host_mode2[id] &= (~MMC_CAP2_HS200_1_8V_SDR) & (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support UHS-SDR50\n");
		break;
	case UHS_SDR104:
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR | MMC_CAP_UHS_SDR104;
		msdc_host_mode2[id] |= MMC_CAP2_HS200_1_8V_SDR;
		msdc_host_mode2[id] &= (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support UHS-SDR104\n");
		break;
	case UHS_DDR50:
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		    MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR;
		msdc_host_mode[id] &= (~MMC_CAP_UHS_SDR50) & (~MMC_CAP_UHS_SDR104);
		msdc_host_mode2[id] &= (~MMC_CAP2_HS200_1_8V_SDR) & (~MMC_CAP2_HS400_1_8V);
		pr_debug("[****SD_Debug****]host will support UHS-DDR50\n");
		break;
	case 6:		/* EMMC_HS400: */
		msdc_host_mode[id] |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR | MMC_CAP_UHS_SDR104;
		msdc_host_mode2[id] |= MMC_CAP2_HS200_1_8V_SDR | MMC_CAP2_HS400_1_8V;
		pr_debug("[****SD_Debug****]host will support EMMC_HS400\n");
		break;
	default:
		pr_debug("[****SD_Debug****]invalid sd30_mode:%d\n", msdc_ctl->sd30_mode);
		break;
	}

	pr_debug("\n [%s]: msdc%d, line:%d, msdc_host_mode2=%d\n",
		 __func__, id, __LINE__, msdc_host_mode2[id]);

	return 0;
}

static int simple_sd_ioctl_get_bootpart(struct msdc_ioctl *msdc_ctl)
{
	char *l_buf;
	struct msdc_host *host_ctl;
	int ret = 0;
	int bootpart = 0;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

	if (msdc_ctl->buffer == NULL)
		return -EINVAL;

	l_buf = kzalloc((512), GFP_KERNEL);
	if (!l_buf)
		return -ENOMEM;

#if DEBUG_MMC_IOCTL
	pr_debug("user want get boot partition info in msdc slot%d\n",
		msdc_ctl->host_num);
#endif
	mmc_claim_host(host_ctl->mmc);
	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	mmc_release_host(host_ctl->mmc);
	if (ret) {
		pr_debug("mmc_send_ext_csd error, get boot part\n");
		goto end;
	}
	bootpart = (l_buf[EXT_CSD_PART_CFG] & 0x38) >> 3;

#if DEBUG_MMC_IOCTL
	pr_debug("bootpart Byte[EXT_CSD_PART_CFG] =%x, booten=%x\n",
		l_buf[EXT_CSD_PART_CFG], bootpart);
#endif

	if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
		if (copy_to_user(msdc_ctl->buffer, &bootpart, 1)) {
			ret = -EFAULT;
			goto end;
		}
	} else {
		/* called from other kernel module */
		memcpy(msdc_ctl->buffer, &bootpart, 1);
	}

end:
	msdc_ctl->result = ret;

	kfree(l_buf);

	return ret;
}

static int simple_sd_ioctl_set_bootpart(struct msdc_ioctl *msdc_ctl)
{
	char *l_buf;
	struct msdc_host *host_ctl;
	int ret = 0;
	int bootpart = 0;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

	if (msdc_ctl->buffer == NULL)
		return -EINVAL;

	if (copy_from_user(&bootpart, msdc_ctl->buffer, 1))
		return -EINVAL;

	l_buf = kzalloc((512), GFP_KERNEL);
	if (!l_buf)
		return -ENOMEM;

#if DEBUG_MMC_IOCTL
	pr_debug("user want set boot partition in msdc slot%d\n",
		msdc_ctl->host_num);
#endif
	mmc_claim_host(host_ctl->mmc);
	ret = mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (ret) {
		pr_debug("mmc_send_ext_csd error, set boot partition\n");
		goto end;
	}

	if ((bootpart != EMMC_BOOT1_EN)
	 && (bootpart != EMMC_BOOT2_EN)
	 && (bootpart != EMMC_BOOT_USER)) {
		pr_debug("set boot partition error, not support %d\n",
			bootpart);
		ret = -EFAULT;
		goto end;
	}

	if (((l_buf[EXT_CSD_PART_CFG] & 0x38) >> 3) != bootpart) {
		/* active boot partition */
		l_buf[EXT_CSD_PART_CFG] &= ~0x38;
		l_buf[EXT_CSD_PART_CFG] |= (bootpart << 3);
		pr_debug("mmc_switch set %x\n", l_buf[EXT_CSD_PART_CFG]);
		ret = mmc_switch(host_ctl->mmc->card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_PART_CFG, l_buf[EXT_CSD_PART_CFG], 1000);
		if (ret) {
			pr_debug("mmc_switch error, set boot partition\n");
		} else {
			host_ctl->mmc->card->ext_csd.part_config =
			l_buf[EXT_CSD_PART_CFG];
		}
	}

end:
	msdc_ctl->result = ret;

	mmc_release_host(host_ctl->mmc);

	kfree(l_buf);
	return ret;
}
static u64 msdc_get_user_capacity(struct msdc_host *host)
{
	u64 capacity = 0;
	u32 legacy_capacity = 0;
	struct mmc_card *card;

	if ((host != NULL) && (host->mmc != NULL) && (host->mmc->card != NULL))
		card = host->mmc->card;
	else
		return 0;

	card = host->mmc->card;
	if (mmc_card_mmc(card)) {
		if (card->csd.read_blkbits) {
			legacy_capacity =
				(2 << (card->csd.read_blkbits - 1))
				* card->csd.capacity;
		} else {
			legacy_capacity = card->csd.capacity;
			ERR_MSG("XXX read_blkbits = 0 XXX");
		}
		capacity =
			(u64)(card->ext_csd.sectors) * 512 > legacy_capacity ?
			(u64)(card->ext_csd.sectors) * 512 : legacy_capacity;
	} else if (mmc_card_sd(card)) {
		capacity = (u64) (card->csd.capacity)
			<< (card->csd.read_blkbits);
	}
	return capacity;
}

static unsigned int msdc_get_other_capacity(struct msdc_host *host, char *name)
{
	unsigned int device_other_capacity = 0;
	int i;
	struct mmc_card *card;

	if ((host != NULL) && (host->mmc != NULL) && (host->mmc->card != NULL))
		card = host->mmc->card;
	else
		return 0;

	for (i = 0; i < card->nr_parts; i++) {
		if (!name) {
			device_other_capacity += card->part[i].size;
		} else if (strcmp(name, card->part[i].name) == 0) {
			device_other_capacity = card->part[i].size;
			break;
		}
	}

	return device_other_capacity;
}

static int simple_sd_ioctl_get_partition_size(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;
	unsigned long long partitionsize = 0;
	struct msdc_host *host;
	int ret = 0;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	if (IS_ERR_OR_NULL(host_ctl))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc))
		return -EINVAL;
	if (IS_ERR_OR_NULL(host_ctl->mmc->card))
		return -EINVAL;

	host = mmc_priv(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want get size of partition=%d\n", msdc_ctl->partition);
#endif

	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		partitionsize = msdc_get_other_capacity(host, "boot0");
		break;
	case EMMC_PART_BOOT2:
		partitionsize = msdc_get_other_capacity(host, "boot1");
		break;
	case EMMC_PART_RPMB:
		partitionsize = msdc_get_other_capacity(host, "rpmb");
		break;
	case EMMC_PART_USER:
		partitionsize = msdc_get_user_capacity(host);
		break;
	default:
		pr_debug("not support partition =%d\n", msdc_ctl->partition);
		partitionsize = 0;
		break;
	}
#if DEBUG_MMC_IOCTL
	pr_debug("bootpart partitionsize =%llx\n", partitionsize);
#endif
	if (copy_to_user(msdc_ctl->buffer, &partitionsize, 8))
		ret = -EFAULT;

	msdc_ctl->result = ret;

	return ret;
}
static long simple_sd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct msdc_ioctl msdc_ctl;
	int ret = 0, size = sizeof(struct msdc_ioctl);

	if ((struct msdc_ioctl *)arg == NULL) {
		switch (cmd) {
		case MSDC_REINIT_SDCARD:
			ret = sd_ioctl_reinit();
			break;
		case MSDC_CD_PIN_EN_SDCARD:
			ret = sd_ioctl_cd_pin_en();
			break;
		default:
			pr_err("mt_sd_ioctl:this opcode value is illegal!!\n");
			return -EINVAL;
		}
	} else {
		if (copy_from_user(&msdc_ctl, (void *)arg, size))
			return -EFAULT;
		if (msdc_ctl.host_num >= HOST_MAX_NUM || msdc_ctl.host_num < 0) {
			pr_err("%s: invalid host id :%d.\n", __func__, msdc_ctl.host_num);
			return -EINVAL;
		}

		switch (msdc_ctl.opcode) {
		case MSDC_SINGLE_READ_WRITE:
			ret = simple_sd_ioctl_single_rw(&msdc_ctl);
			break;
		case MSDC_MULTIPLE_READ_WRITE:
			ret = simple_sd_ioctl_multi_rw(&msdc_ctl);
			break;
		case MSDC_GET_CID:
			ret = simple_sd_ioctl_get_cid(&msdc_ctl);
			break;
		case MSDC_GET_CSD:
			ret = simple_sd_ioctl_get_csd(&msdc_ctl);
			break;
		case MSDC_GET_EXCSD:
			ret = simple_sd_ioctl_get_excsd(&msdc_ctl);
			break;
		case MSDC_DRIVING_SETTING:
			pr_debug("in ioctl to change driving\n");
			if (msdc_ctl.iswrite)
				ret = simple_sd_ioctl_set_driving(&msdc_ctl);
			else
				ret = simple_sd_ioctl_get_driving(&msdc_ctl);
			break;
		case MSDC_SD30_MODE_SWITCH:
			ret = simple_sd_ioctl_sd30_mode_switch(&msdc_ctl);
			break;
		case MSDC_GET_BOOTPART:
			ret = simple_sd_ioctl_get_bootpart(&msdc_ctl);
			break;
		case MSDC_SET_BOOTPART:
			ret = simple_sd_ioctl_set_bootpart(&msdc_ctl);
			break;
		case MSDC_GET_PARTSIZE:
			ret = simple_sd_ioctl_get_partition_size(&msdc_ctl);
			break;
		default:
			pr_err("%s: illegal opcode\n", __func__);
			return -EINVAL;
		}
		msdc_ctl.result = ret;
		if (copy_to_user((void *)arg, &msdc_ctl, size))
			return -EFAULT;
	}
	return ret;
}

static const struct file_operations simple_msdc_em_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = simple_sd_ioctl,
	.open = simple_sd_open,
};

static struct miscdevice simple_msdc_em_dev = {
	 .minor = MISC_DYNAMIC_MINOR,
	 .name = "misc-sd",
	 .fops = &simple_msdc_em_fops,
};

static int __init simple_sd_init(void)
{
	int ret = 0;

	sg_msdc_multi_buffer = kmalloc(64 * 1024, GFP_KERNEL);
	if (sg_msdc_multi_buffer == NULL) {
		pr_err("%s: alloc memory fail\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	ret = misc_register(&simple_msdc_em_dev);
	if (ret)
		pr_err("%s: register misc-sd fail, ret = %d\n", __func__, ret);
out:
	return ret;
}

static void __exit simple_sd_exit(void)
{
	if (sg_msdc_multi_buffer != NULL) {
		kfree(sg_msdc_multi_buffer);
		sg_msdc_multi_buffer = NULL;
	}

	misc_deregister(&simple_msdc_em_dev);
}

module_init(simple_sd_init);
module_exit(simple_sd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("simple MediaTek SD/MMC Card Driver");
