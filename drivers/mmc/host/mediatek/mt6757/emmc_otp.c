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
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "mtk_sd.h"
#include "dbg.h"

#define OTP_MAGIC_NUM           0x4E3AF28B

struct emmc_otp_config {
	u32 (*read)(u32 blkno, void *BufferPtr);
	u32 (*write)(u32 blkno, void *BufferPtr);
	u32 (*query_length)(u32 *Length);
};

struct otp_ctl {
	unsigned int QLength;
	unsigned int Offset;
	unsigned int Length;
	char *BufferPtr;
	unsigned int status;
};

#ifdef CONFIG_COMPAT
struct compat_otp_ctl {
	compat_uint_t QLength;
	compat_uint_t Offset;
	compat_uint_t Length;
	compat_uptr_t BufferPtr;
	unsigned int status;
};
#endif

#define EMMC_OTP_START_ADDRESS  (0xc0000000)    /* just for debug */

#define EMMC_HOST_NUM           0
#define EMMC_OTP_MAGIC          'k'

/* EMMC OTP IO control number */
#define EMMC_OTP_GET_LENGTH     _IOW(EMMC_OTP_MAGIC, 1, int)
#define EMMC_OTP_READ           _IOW(EMMC_OTP_MAGIC, 2, int)
#define EMMC_OTP_WRITE          _IOW(EMMC_OTP_MAGIC, 3, int)

#ifdef CONFIG_COMPAT
#define COMPAT_EMMC_OTP_GET_LENGTH _IOW(EMMC_OTP_MAGIC, 1, compat_int_t)
#define COMPAT_EMMC_OTP_READ    _IOW(EMMC_OTP_MAGIC, 2, compat_int_t)
#define COMPAT_EMMC_OTP_WRITE   _IOW(EMMC_OTP_MAGIC, 3, compat_int_t)
#endif

#define FS_EMMC_OTP_READ        0
#define FS_EMMC_OTP_WRITE       1

/* EMMC OTP Error codes */
#define OTP_SUCCESS             0
#define OTP_ERROR_OVERSCOPE     -1
#define OTP_ERROR_TIMEOUT       -2
#define OTP_ERROR_BUSY          -3
#define OTP_ERROR_NOMEM         -4
#define OTP_ERROR_RESET         -5

#include <mt-plat/mtk_partition.h>
#define DRV_NAME_MISC           "otp"
#define PROCNAME                "driver/otp"

#define EMMC_OTP_DEBUG          1

/* static spinlock_t g_emmc_otp_lock; */
static struct emmc_otp_config g_emmc_otp_func;
static unsigned int sg_wp_size; /* byte unit */
static unsigned int g_otp_user_ccci = 2 * 1024 * 1024;

struct msdc_host *emmc_otp_get_host(void)
{
	return mtk_msdc_host[EMMC_HOST_NUM];
}


/******************************************************************************
 * EMMC OTP operations
 * ***************************************************************************/
unsigned int emmc_get_wp_size(void)
{
	u8 *l_ext_csd;
	struct msdc_host *host_ctl;
	int ret = 0;
	u32 *csd = NULL;
	u32 write_prot_grpsz = 0;

	if (sg_wp_size == 0) {
		/* not to change ERASE_GRP_DEF after card initialized */
		host_ctl = emmc_otp_get_host();

		WARN_ON(!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card);

		if (!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card) {
			pr_err("host_ctl or mmc or card is NULL\n");
			return -1;
		}

		pr_debug("[%s:%d]mmc_host is : 0x%p\n",
			__func__, __LINE__, host_ctl->mmc);
#if 0
		pr_debug("[%s:%d]claim host done! claim_status = %d, claim_cnt = %d, claimer = 0x%x, current = 0x%x\n",
			__func__, __LINE__, host_ctl->mmc->claimed,
			host_ctl->mmc->claim_cnt, host_ctl->mmc->claimer,
			current);
#endif
		mmc_claim_host(host_ctl->mmc);

#if 0
		pr_debug("[%s:%d]claim host done! claim_status = %d, claim_cnt = %d, claimer = 0x%x, current = 0x%x\n",
			__func__, __LINE__, host_ctl->mmc->claimed,
			host_ctl->mmc->claim_cnt, host_ctl->mmc->claimer,
			current);
#endif
		/* As the ext_csd is large and mostly unused, raw ext_csd is
		 * not stored in mmc_card, so get it again.
		 */
		ret = mmc_get_ext_csd(host_ctl->mmc->card, &l_ext_csd);
		mmc_release_host(host_ctl->mmc);
		if (ret) {
			pr_err("mmc_get_ext_csd err\n");
			return -1;
		}
#if EMMC_OTP_DEBUG
		{
			int i;

			for (i = 0; i < 512; i++) {
				pr_debug("%x", l_ext_csd[i]);
				if (0 == ((i + 1) % 16))
					pr_debug("\n");
			}
		}
#endif

		csd = host_ctl->mmc->card->raw_csd;
		write_prot_grpsz = UNSTUFF_BITS(csd, 32, 5);
		pr_err("otp: write_prot_grpsz %d\n", write_prot_grpsz);
		/* otp length equal to one write protect group size */
		if (l_ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x1) {
			/* use high-capacity erase uint size, hc erase timeout,
			 * hc wp size, store in EXT_CSD
			 */
			sg_wp_size = (512 * 1024 *
				l_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] *
				l_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]);
			pr_err("otp: hc unit sg_wp_size %d\n", sg_wp_size);
		} else {
			/* use old erase group size and
			 * write protect group size, store in CSD
			 */
			sg_wp_size = (512 * host_ctl->mmc->card->erase_size) *
				(write_prot_grpsz + 1);
			pr_err("otp: non-hc unit sg_wp_size %d\n", sg_wp_size);
		}
	}
	kfree(l_ext_csd);

	return sg_wp_size;
}

/* need subtract emmc reserved region for combo feature */
unsigned int emmc_otp_start(void)
{
	unsigned int l_addr;
	struct msdc_host *host_ctl;
	struct hd_struct *lp_hd_struct = NULL;

	if (sg_wp_size == 0)
		emmc_get_wp_size();

	host_ctl = emmc_otp_get_host();

	WARN_ON(!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card);
	if (!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card) {
		pr_err("host_ctl or mmc or card is NULL\n");
		return -EINVAL;
	}

	lp_hd_struct = get_part("otp");
	if (lp_hd_struct == NULL) {
		pr_debug("not find otp in scatter file error!\n");
		put_part(lp_hd_struct);
		return -EFAULT;
	}

	l_addr = lp_hd_struct->start_sect;        /* block unit */

	/* find wp group start address in 43MB reserved region */
	if (l_addr % (sg_wp_size >> 9))
		l_addr += ((sg_wp_size >> 9) - l_addr % (sg_wp_size >> 9));

	if (lp_hd_struct != NULL)
		put_part(lp_hd_struct);

	pr_debug("find OTP start address is 0x%x\n", l_addr);

	return l_addr;
}

unsigned int emmc_otp_query_length(unsigned int *QLength)
{
	/* otp length equal to one write protect group size */
	*QLength = emmc_get_wp_size() - g_otp_user_ccci;

	return 0;
}

unsigned int emmc_otp_read(unsigned int blk_offset, void *BufferPtr)
{
	unsigned char *l_rcv_buf = (unsigned char *)BufferPtr;
	unsigned int l_addr;
	unsigned int l_otp_size;
	unsigned int l_ret;
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	int is_cmdq_en = false;
	int ret;
#endif

	/* check parameter */
	l_addr = emmc_otp_start();
	l_otp_size = emmc_get_wp_size();
	if (blk_offset > (l_otp_size >> 9))
		return OTP_ERROR_OVERSCOPE;

	l_addr += blk_offset;

	host_ctl = emmc_otp_get_host();

	WARN_ON(!host_ctl || !host_ctl->mmc);
	if (!host_ctl || !host_ctl->mmc) {
		pr_err("host_ctl or mmc is NULL\n");
		return -1;
	}

	pr_debug("[%s:%d]mmc_host is : 0x%p\n", __func__, __LINE__,
		host_ctl->mmc);
	mmc_claim_host(host_ctl->mmc);

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	is_cmdq_en = false;
	if (host_ctl->mmc->card->ext_csd.cmdq_mode_en) {
		/* cmdq enabled, turn it off first */
		pr_debug("EMMC_OTP: cmdq enabled, turn it off\n");
		is_cmdq_en = true;
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 0);
		if (ret) {
			pr_debug("EMMC_OTP turn off cmdq en failed\n");
			mmc_release_host(host_ctl->mmc);
			return ret;
		}
	}
#endif

	/* make sure access user data area */
	msdc_switch_part(host_ctl, 0);

#if EMMC_OTP_DEBUG
	pr_debug("EMMC_OTP: start MSDC_SINGLE_READ_WRITE!\n");
#endif

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	msdc_data.flags = MMC_DATA_READ;
	msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
	msdc_data.blocks = 1;   /* one block Request */

	memset(l_rcv_buf, 0, 512);

	msdc_cmd.arg = l_addr;

	WARN_ON(!host_ctl->mmc->card);
	if (!host_ctl->mmc->card) {
		pr_err("card is NULL\n");
		return -1;
	}

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, l_rcv_buf, 512);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	if (is_cmdq_en) {
		pr_debug("EMMC_OTP turn on cmdq\n");
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 1);
		if (ret) {
			pr_debug("EMMC_OTP turn on cmdq en failed\n");
			mmc_release_host(host_ctl->mmc);
			return ret;
		}
	}
#endif

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		l_ret = msdc_cmd.error;

	if (msdc_data.error)
		l_ret = msdc_data.error;
	else
		l_ret = OTP_SUCCESS;

	return l_ret;
}

unsigned int emmc_otp_write(unsigned int blk_offset, void *BufferPtr)
{
	unsigned char *l_send_buf = (unsigned char *)BufferPtr;
	unsigned int l_ret;
	unsigned int l_addr;
	unsigned int l_otp_size;
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
#ifdef MTK_MSDC_USE_CACHE
	struct mmc_command msdc_sbc;
#endif
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	int is_cmdq_en;
	int ret;
#endif

	/* check parameter */
	l_addr = emmc_otp_start();
	l_otp_size = emmc_get_wp_size();
	if (blk_offset > (l_otp_size >> 9))
		return OTP_ERROR_OVERSCOPE;

	l_addr += blk_offset;

	host_ctl = emmc_otp_get_host();

	WARN_ON(!host_ctl || !host_ctl->mmc);
	if (!host_ctl || !host_ctl->mmc) {
		pr_err("host_ctl or mmc is NULL\n");
		return -1;
	}

	mmc_claim_host(host_ctl->mmc);

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	is_cmdq_en = false;
	if (host_ctl->mmc->card->ext_csd.cmdq_mode_en) {
		/* cmdq enabled, turn it off first */
		pr_debug("EMMC_OTP: cmdq enabled, turn it off\n");
		is_cmdq_en = true;
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 0);
		if (ret) {
			pr_debug("EMMC_OTP: turn off cmdq en failed\n");
			mmc_release_host(host_ctl->mmc);
			return ret;
		}
	}
#endif

	/* make sure access user data area */
	msdc_switch_part(host_ctl, 0);

#if EMMC_OTP_DEBUG
	pr_debug("EMMC_OTP: start MSDC_SINGLE_READ_WRITE!\n");
#endif

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
#ifdef MTK_MSDC_USE_CACHE
	memset(&msdc_sbc, 0, sizeof(struct mmc_command));
#endif

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	msdc_data.flags = MMC_DATA_WRITE;
#ifdef MTK_MSDC_USE_CACHE
	if (mmc_card_mmc(host_ctl->mmc->card)
		&& (host_ctl->mmc->card->ext_csd.cache_ctrl & 0x1)) {
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_mrq.sbc = &msdc_sbc;
		msdc_mrq.sbc->opcode = MMC_SET_BLOCK_COUNT;
		msdc_mrq.sbc->arg = msdc_data.blocks | (1 << 31);
		msdc_mrq.sbc->flags = MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		msdc_cmd.opcode = MMC_WRITE_BLOCK;
	}
#else
	msdc_cmd.opcode = MMC_WRITE_BLOCK;
#endif
	msdc_data.blocks = 1;

	msdc_cmd.arg = l_addr;

	WARN_ON(!host_ctl->mmc->card);
	if (!host_ctl->mmc->card) {
		pr_err("card is NULL\n");
		return -1;
	}

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, l_send_buf, 512);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	if (is_cmdq_en) {
		pr_debug("EMMC_OTP turn on cmdq\n");
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 1);
		if (ret) {
			pr_debug("EMMC_OTP turn on cmdq en failed\n");
			mmc_release_host(host_ctl->mmc);
			return ret;
		}
	}
#endif

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		l_ret = msdc_cmd.error;

	if (msdc_data.error)
		l_ret = msdc_data.error;
	else
		l_ret = OTP_SUCCESS;

	return l_ret;
}

static int mt_otp_open(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n",
		 __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int *)OTP_MAGIC_NUM;
	return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n",
		 __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset,
	void *buff_ptr, unsigned int length, unsigned int *status)
{
	unsigned int ret = 0;
	char *BufAddr = (char *)buff_ptr;
	unsigned int blkno, AccessLength = 0;
	unsigned int l_block_size = 512;
	int Status = 0;

	char *p_D_Buff;
	/* char S_Buff[64]; */

	p_D_Buff = kmalloc(l_block_size, GFP_KERNEL);
	if (!p_D_Buff) {
		ret = -ENOMEM;
		*status = OTP_ERROR_NOMEM;
		goto exit;
	}

	pr_debug("[%s]: %s (0x%x) length:(%d bytes) !\n",
		 __func__, access_type ? "WRITE" : "READ", offset, length);

	do {
		blkno = offset / l_block_size;
		if (access_type == FS_EMMC_OTP_READ) {
			memset(p_D_Buff, 0xff, l_block_size);

			pr_debug("[%s]: Read Access of page (%d)\n", __func__,
				blkno);

			Status = g_emmc_otp_func.read(blkno, p_D_Buff);
			*status = Status;

			if (Status != OTP_SUCCESS) {
				pr_debug("[%s]: Read status (%d)\n", __func__,
					Status);
				break;
			}

			AccessLength = l_block_size - (offset % l_block_size);

			if (length >= AccessLength) {
				memcpy(BufAddr,
					(p_D_Buff + (offset % l_block_size)),
					AccessLength);
			} else {
				/* last time */
				memcpy(BufAddr,
					(p_D_Buff + (offset % l_block_size)),
					length);
			}
		} else if (access_type == FS_EMMC_OTP_WRITE) {
			AccessLength = l_block_size - (offset % l_block_size);
			memset(p_D_Buff, 0x0, l_block_size);

			Status = g_emmc_otp_func.read(blkno, p_D_Buff);
			*status = Status;

			if (Status != OTP_SUCCESS) {
				pr_debug("[%s]: read before write, Read status (%d) blkno (0x%x)\n",
					__func__, Status, blkno);
				break;
			}

			if (length >= AccessLength) {
				memcpy((p_D_Buff + (offset % l_block_size)),
					BufAddr, AccessLength);
			} else {
				/* last time */
				memcpy((p_D_Buff + (offset % l_block_size)),
					BufAddr, length);
			}

			Status = g_emmc_otp_func.write(blkno, p_D_Buff);
			*status = Status;

			if (Status != OTP_SUCCESS) {
				pr_debug("[%s]: Write status (%d)\n", __func__,
					Status);
				break;
			}
		} else {
			pr_err("[%s]: Error, neither read nor write operations!\n",
				__func__);
			break;
		}

		offset += AccessLength;
		BufAddr += AccessLength;
		if (length <= AccessLength) {
			length = 0;
			break;
		}
		length -= AccessLength;
		pr_debug("[%s]: Remaining %s (%d) !\n",
			 __func__, access_type ? "WRITE" : "READ", length);

	} while (1);


	kfree(p_D_Buff);
 exit:
	return ret;
}

static long mt_otp_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	char *pbuf;

	void __user *uarg = (void __user *)arg;
	struct otp_ctl otpctl;

	/* Lock */
	/* spin_lock(&g_emmc_otp_lock); */

	if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl))) {
		ret = -EFAULT;
		goto exit;
	}

#if 0
	if (g_bInitDone == false) {
		pr_err("ERROR: EMMC Flash Not initialized !!\n");
		ret = -EFAULT;
		goto exit;
	}
#endif

	pbuf = kmalloc_array(otpctl.Length, sizeof(char), GFP_KERNEL);
	if (!pbuf) {
		ret = -ENOMEM;
		goto exit;
	}

	switch (cmd) {
	case EMMC_OTP_GET_LENGTH:
		pr_debug("OTP IOCTL: EMMC_OTP_GET_LENGTH\n");
		if (('c' == (otpctl.status & 0xff))
			&& ('c' == ((otpctl.status >> 8) & 0xff))
			&& ('c' == ((otpctl.status >> 16) & 0xff))
			&& ('i' == ((otpctl.status >> 24) & 0xff))) {
			otpctl.QLength = g_otp_user_ccci;
		} else {
			g_emmc_otp_func.query_length(&otpctl.QLength);
		}
		otpctl.status = OTP_SUCCESS;
		pr_debug("OTP IOCTL: The Length is %d\n", otpctl.QLength);
		break;
	case EMMC_OTP_READ:
		pr_debug("OTP IOCTL: EMMC_OTP_READ Offset(0x%x), Length(0x%x)\n",
			otpctl.Offset, otpctl.Length);
		memset(pbuf, 0xff, sizeof(char) * otpctl.Length);

		mt_otp_access(FS_EMMC_OTP_READ, otpctl.Offset, pbuf,
			otpctl.Length, &otpctl.status);

		if (copy_to_user(otpctl.BufferPtr, pbuf,
			(sizeof(char) * otpctl.Length))) {
			pr_err("EMMC_OTP IOCTL: Copy to user buffer Error!\n");
			goto error;
		}
		break;
	case EMMC_OTP_WRITE:
		pr_debug("OTP IOCTL: EMMC_OTP_WRITE Offset(0x%x), Length(0x%x)\n",
			otpctl.Offset, otpctl.Length);
		if (copy_from_user(pbuf, otpctl.BufferPtr,
			(sizeof(char) * otpctl.Length))) {
			pr_err("EMMC_OTP IOCTL: Copy from user buffer Error!\n");
			goto error;
		}
		mt_otp_access(FS_EMMC_OTP_WRITE, otpctl.Offset, pbuf,
			otpctl.Length, &otpctl.status);
		break;
	default:
		ret = -EINVAL;
	}

	ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

 error:
	kfree(pbuf);
 exit:
	/* spin_unlock(&g_emmc_otp_lock); */
	return ret;
}

#ifdef CONFIG_COMPAT
static int compat_get_otp_ctl_allocation(struct compat_otp_ctl __user *arg32,
	struct otp_ctl __user *arg64)
{
	compat_uint_t u;
	compat_uptr_t p;
	int err;

	err = get_user(u, &(arg32->QLength));
	err |= put_user(u, &(arg64->QLength));
	err |= get_user(u, &(arg32->Offset));
	err |= put_user(u, &(arg64->Offset));
	err |= get_user(u, &(arg32->Length));
	err |= put_user(u, &(arg64->Length));
	err |= get_user(p, &(arg32->BufferPtr));
	err |= put_user(compat_ptr(p), &(arg64->BufferPtr));
	err |= get_user(u, &(arg32->status));
	err |= put_user(u, &(arg64->status));

	return err;
}

static int compat_put_otp_ctl_allocation(struct compat_otp_ctl __user *arg32,
	struct otp_ctl __user *arg64)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(arg64->QLength));
	err |= put_user(u, &(arg32->QLength));
	err |= get_user(u, &(arg64->Offset));
	err |= put_user(u, &(arg32->Offset));
	err |= get_user(u, &(arg64->Length));
	err |= put_user(u, &(arg32->Length));
	err |= get_user(u, &(arg64->status));
	err |= put_user(u, &(arg32->status));

	return err;
}

static long mt_otp_ioctl_compat(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	int err = 0;
	struct compat_otp_ctl *arg32;
	struct otp_ctl *arg64;

	/*void __user *uarg = compat_ptr(arg);*/

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	arg32 = compat_ptr(arg);
	arg64 = compat_alloc_user_space(sizeof(*arg64));
	if (arg64 == NULL)
		return -EFAULT;

	ret = compat_get_otp_ctl_allocation(arg32, arg64);
	if (ret)
		return ret;
	/* Lock */
	/* spin_lock(&g_emmc_otp_lock); */

	switch (cmd) {
	case COMPAT_EMMC_OTP_GET_LENGTH:
		ret = mt_otp_ioctl(file, EMMC_OTP_GET_LENGTH,
			(unsigned long)arg64);
		/*pr_err("OTP IOCTL: COMPAT_EMMC_OTP_GET_LENGTH\n");*/
		if (ret)
			pr_err("EMMC_OTP_GET_LENGTH unlocked_ioctl failed.");

		break;
	case COMPAT_EMMC_OTP_READ:
		/*pr_err("OTP IOCTL: COMPAT_EMMC_OTP_READ\n");*/
		ret = mt_otp_ioctl(file, EMMC_OTP_READ, (unsigned long)arg64);

		if (ret)
			pr_err("EMMC_OTP_READ unlocked_ioctl failed.");

		break;
	case COMPAT_EMMC_OTP_WRITE:
		/*pr_err("OTP IOCTL: COMPAT_EMMC_OTP_WRITE\n");*/
		ret = mt_otp_ioctl(file, EMMC_OTP_WRITE, (unsigned long)arg64);

		if (ret)
			pr_err("EMMC_OTP_WRITE unlocked_ioctl failed.");

		break;
	default:
		ret = -EINVAL;
	}
	err = compat_put_otp_ctl_allocation(arg32, arg64);
	return ret ? ret : err;
}
#endif

static const struct file_operations emmc_otp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mt_otp_ioctl,
	.open = mt_otp_open,
	.release = mt_otp_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mt_otp_ioctl_compat,
#endif
};

static struct miscdevice emmc_otp_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "otp",
	.fops = &emmc_otp_fops,
};


static int emmc_otp_probe(struct platform_device *pdev)
{
	pr_debug("in emmc otp probe function\n");

	return 0;
}

static int emmc_otp_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver emmc_otp_driver = {
	.probe = emmc_otp_probe,
	.remove = emmc_otp_remove,

	.driver = {
		.name = DRV_NAME_MISC,
		.owner = THIS_MODULE,
	},
};

static int __init emmc_otp_init(void)
{
	int err = 0;

	pr_debug("MediaTek EMMC OTP misc driver init\n");

	/*spin_lock_init(&g_emmc_otp_lock);*/

	g_emmc_otp_func.query_length = emmc_otp_query_length;
	g_emmc_otp_func.read         = emmc_otp_read;
	g_emmc_otp_func.write        = emmc_otp_write;
#if 0
#ifndef USER_BUILD_KERNEL
	entry = create_proc_entry(PROCNAME, 0660, NULL);
#else
	entry = create_proc_entry(PROCNAME, 0440, NULL);
#endif

	if (entry == NULL) {
		pr_err("emmc OTP: unable to create /proc entry\n");
		return -ENOMEM;
	}

	entry->read_proc = emmc_otp_proc_read;
	entry->write_proc = emmc_otp_proc_write;
	/* entry->owner = THIS_MODULE; */
#endif
	platform_driver_register(&emmc_otp_driver);

	pr_debug("OTP: register EMMC OTP device ...\n");
	err = misc_register(&emmc_otp_dev);
	if (unlikely(err)) {
		pr_err("OTP: failed to register EMMC OTP device!\n");
		return err;
	}

	return 0;
}

static void __exit emmc_otp_exit(void)
{
	pr_debug("MediaTek EMMC OTP misc driver exit\n");

	misc_deregister(&emmc_otp_dev);

	g_emmc_otp_func.query_length = NULL;
	g_emmc_otp_func.read         = NULL;
	g_emmc_otp_func.write        = NULL;

	platform_driver_unregister(&emmc_otp_driver);
	remove_proc_entry(PROCNAME, NULL);
}


module_init(emmc_otp_init);
module_exit(emmc_otp_exit);
