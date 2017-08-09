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

#include "mt_sd.h"
#include "msdc_io.h"
#include "dbg.h"
#include <mt-plat/sd_misc.h>
#include "board-custom.h"
#include <queue.h>

#ifndef FPGA_PLATFORM
#include <mach/mt_clkmgr.h>
#endif

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define PARTITION_NAME_LENGTH   (64)
#define DRV_NAME_MISC           "misc-sd"

#define DEBUG_MMC_IOCTL         0
#define DEBUG_MSDC_SSC          1
/*
 * For simple_sd_ioctl
 */
#define FORCE_IN_DMA            (0x11)
#define FORCE_IN_PIO            (0x10)
#define FORCE_NOTHING           (0x0)

static int dma_force[HOST_MAX_NUM] =    /* used for sd ioctrol */
{
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING,
};

#define dma_is_forced(host_id)                  (dma_force[host_id] & 0x10)
#define get_forced_transfer_mode(host_id)       (dma_force[host_id] & 0x01)


static u32 *sg_msdc_multi_buffer;

static int simple_sd_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sd_ioctl_reinit(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host = msdc_get_host(MSDC_SD,	0, 0);

	if (NULL != host)
		return msdc_reinit(host);
	else
		return -EINVAL;
}

static int sd_ioctl_cd_pin_en(struct msdc_ioctl	*msdc_ctl)
{
	struct msdc_host *host = msdc_get_host(MSDC_SD,	0, 0);

	if (NULL != host)
		return (host->hw->flags & MSDC_CD_PIN_EN) == MSDC_CD_PIN_EN;
	else
		return -EINVAL;
}

int simple_sd_ioctl_rw(struct msdc_ioctl *msdc_ctl)
{
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;
	int ret = 0;
#ifdef CONFIG_MTK_EMMC_SUPPORT
	char part_id;
#endif
	int no_single_rw;

#ifdef MTK_MSDC_USE_CMD23
	struct mmc_command msdc_sbc;
#endif

	struct mmc_request  msdc_mrq;
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];
	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	if ((msdc_ctl->total_size <= 0) ||
	    (msdc_ctl->total_size > host_ctl->mmc->max_seg_size))
		return -EINVAL;

	if (msdc_ctl->total_size > 512)
		no_single_rw = 1;
	else
		no_single_rw = 0;

#ifdef MTK_MSDC_USE_CACHE
	if (msdc_ctl->iswrite && mmc_card_mmc(host_ctl->mmc->card)
		&& (host_ctl->mmc->card->ext_csd.cache_ctrl & 0x1))
		no_single_rw = 1;
#endif
	if (msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_from_user(sg_msdc_multi_buffer,
				msdc_ctl->buffer, msdc_ctl->total_size)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
				goto rw_end_without_release;
			}
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer,
				msdc_ctl->total_size);
		}
	} else {
		memset(sg_msdc_multi_buffer, 0, msdc_ctl->total_size);
	}
	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want access %d partition\n", msdc_ctl->partition);
#endif

#ifdef CONFIG_MTK_EMMC_SUPPORT
	switch (msdc_ctl->partition) {
	case EMMC_PART_BOOT1:
		part_id = 1;
		break;
	case EMMC_PART_BOOT2:
		part_id = 2;
		break;
	default:
		/* make sure access partition is user data area */
		part_id = 0;
		break;
	}

	if (msdc_switch_part(host_ctl, part_id))
		goto rw_end;

#endif

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	if (no_single_rw) {
		memset(&msdc_stop, 0, sizeof(struct mmc_command));

#ifdef MTK_MSDC_USE_CMD23
		memset(&msdc_sbc, 0, sizeof(struct mmc_command));
#endif
	}

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if (msdc_ctl->trans_type)
		dma_force[host_ctl->id] = FORCE_IN_DMA;
	else
		dma_force[host_ctl->id] = FORCE_IN_PIO;

	if (msdc_ctl->iswrite) {
		msdc_data.flags = MMC_DATA_WRITE;
		if (no_single_rw)
			msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		else
			msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
	} else {
		msdc_data.flags = MMC_DATA_READ;
		if (no_single_rw)
			msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		else
			msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		memset(sg_msdc_multi_buffer, 0, msdc_ctl->total_size);
	}

#ifdef MTK_MSDC_USE_CMD23
	if (no_single_rw == 0)
		goto skip_sbc_prepare;

	if ((mmc_card_mmc(host_ctl->mmc->card)
		|| (mmc_card_sd(host_ctl->mmc->card)
		&& host_ctl->mmc->card->scr.cmds & SD_SCR_CMD23_SUPPORT))
		&& !(host_ctl->mmc->card->quirks & MMC_QUIRK_BLK_NO_CMD23)) {
		msdc_mrq.sbc = &msdc_sbc;
		msdc_mrq.sbc->opcode = MMC_SET_BLOCK_COUNT;
#ifdef MTK_MSDC_USE_CACHE
		/* if ioctl access cacheable partition data,
		   there is on flush mechanism in msdc driver
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
skip_sbc_prepare:
#endif

	msdc_cmd.arg = msdc_ctl->address;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("this device use byte address!!\n");
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	if (no_single_rw) {
		msdc_stop.opcode = MMC_STOP_TRANSMISSION;
		msdc_stop.arg = 0;
		msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

		msdc_data.stop = &msdc_stop;
	} else {
		msdc_data.stop = NULL;
	}
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	pr_debug("total size is %d\n", msdc_ctl->total_size);
	pr_debug("ueser buf address is 0x%p!\n", msdc_ctl->buffer);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);
	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	/* clear the global buffer of R/W IOCTL */
	memset(sg_msdc_multi_buffer, 0 , msdc_ctl->total_size);

	if (msdc_ctl->partition)
		msdc_switch_part(host_ctl, 0);

	mmc_release_host(host_ctl->mmc);
	if (!msdc_ctl->iswrite) {
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
			if (copy_to_user(msdc_ctl->buffer, sg_msdc_multi_buffer,
				msdc_ctl->total_size)) {
				dma_force[host_ctl->id] = FORCE_NOTHING;
				ret = -EFAULT;
				goto rw_end_without_release;
			}
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer,
				msdc_ctl->total_size);
		}
	}

rw_end:
	mmc_release_host(host_ctl->mmc);
rw_end_without_release:
	if (ret)
		msdc_ctl->result = ret;

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result = msdc_data.error;
	else
		msdc_ctl->result = 0;

	dma_force[host_ctl->id] = FORCE_NOTHING;
	return msdc_ctl->result;

}

static int simple_sd_ioctl_get_cid(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the cid in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_cid, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("cid:0x%x,0x%x,0x%x,0x%x\n",
		host_ctl->mmc->card->raw_cid[0],
		host_ctl->mmc->card->raw_cid[1],
		host_ctl->mmc->card->raw_cid[2],
		host_ctl->mmc->card->raw_cid[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_csd(struct msdc_ioctl *msdc_ctl)
{
	struct msdc_host *host_ctl;

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the csd in msdc slot%d\n", msdc_ctl->host_num);
#endif

	if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_csd, 16))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	pr_debug("csd:0x%x,0x%x,0x%x,0x%x\n",
		 host_ctl->mmc->card->raw_csd[0],
		 host_ctl->mmc->card->raw_csd[1],
		 host_ctl->mmc->card->raw_csd[2],
		 host_ctl->mmc->card->raw_csd[3]);
#endif
	return 0;

}

static int simple_sd_ioctl_get_excsd(struct msdc_ioctl *msdc_ctl)
{
	char l_buf[512];
	struct msdc_host *host_ctl;

#if DEBUG_MMC_IOCTL
	int i;
#endif

	if (!msdc_ctl)
		return -EINVAL;

	host_ctl = mtk_msdc_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	pr_debug("user want the extend csd in msdc slot%d\n",
		msdc_ctl->host_num);
#endif
	mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
		return -EFAULT;

#if DEBUG_MMC_IOCTL
	for (i = 0; i < 512; i++) {
		pr_debug("%x", l_buf[i]);
		if (0 == ((i + 1) % 16))
			pr_debug("\n");
	}
#endif

	if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
		return -EFAULT;

	mmc_release_host(host_ctl->mmc);

	return 0;

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

	host = mtk_msdc_host[msdc_ctl->host_num];
	if (host == NULL) {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	base = host->base;

	msdc_clk_enable(host);

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
#if DEBUG_MMC_IOCTL
#if 0
	msdc_dump_padctl(host);
#endif
#endif

	return 0;
}

static int simple_sd_ioctl_get_driving(struct msdc_ioctl *msdc_ctl)
{
	void __iomem *base;
	struct msdc_host *host;

	if (!msdc_ctl)
		return -EINVAL;

	if ((msdc_ctl->host_num < 0) || (msdc_ctl->host_num >= HOST_MAX_NUM)) {
		pr_err("invalid host num: %d\n", msdc_ctl->host_num);
		return -EINVAL;
	}
	host = mtk_msdc_host[msdc_ctl->host_num];
	if (host == NULL) {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	base = host->base;

	msdc_clk_enable(host);

	msdc_get_driving(host, msdc_ctl);
#if DEBUG_MMC_IOCTL
	pr_debug("read: clk driving is 0x%x\n", msdc_ctl->clk_pu_driving);
	pr_debug("read: cmd driving is 0x%x\n", msdc_ctl->cmd_pu_driving);
	pr_debug("read: dat driving is 0x%x\n", msdc_ctl->dat_pu_driving);
	pr_debug("read: rst driving is 0x%x\n", msdc_ctl->rst_pu_driving);
	pr_debug("read: ds driving is 0x%x\n", msdc_ctl->ds_pu_driving);
#endif

	return 0;
}

static int simple_sd_ioctl_sd30_mode_switch(struct msdc_ioctl *msdc_ctl)
{
	return -EINVAL;
#if 0
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
	if (mtk_msdc_host[msdc_ctl->host_num] == NULL) {
		pr_err("host%d is not config\n", msdc_ctl->host_num);
		return -EINVAL;
	}

	msdc_set_host_mode_speed(, msdc_ctl->sd30_mode);
	pr_debug("\n [%s]: msdc%d, line:%d, msdc_host_mode2=%d\n",
		 __func__, id, __LINE__, msdc_host_mode2[id]);

	msdc_set_host_mode_driver_type(id, msdc_ctl->sd30_drive);

#ifdef CONFIG_MTK_EMMC_SUPPORT
	/* just for emmc slot */
	if (msdc_ctl->host_num == 0)
		g_emmc_mode_switch = 1;
	else
		g_emmc_mode_switch = 0;
#endif
	return 0;
#endif
}

/*  to ensure format operate is clean the emmc device fully(partition erase) */
struct mbr_part_info {
	unsigned int start_sector;
	unsigned int nr_sects;
	unsigned int part_no;
	unsigned char *part_name;
};

#define MBR_PART_NUM            6
#define __MMC_ERASE_ARG         0x00000000
#define __MMC_TRIM_ARG          0x00000001
#define __MMC_DISCARD_ARG       0x00000003

#if 0
struct __mmc_blk_data {
	spinlock_t lock;
	struct gendisk *disk;
	struct mmc_queue queue;

	unsigned int usage;
	unsigned int read_only;
};
#endif

int msdc_get_info(STORAGE_TPYE storage_type, GET_STORAGE_INFO info_type,
	struct storage_info *info)
{
	struct msdc_host *host = NULL;
	int host_function = 0;
	bool boot = 0;

	if (!info)
		return -EINVAL;

	switch (storage_type) {
	case EMMC_CARD_BOOT:
		host_function = MSDC_EMMC;
		boot = MSDC_BOOT_EN;
		break;
	case EMMC_CARD:
		host_function = MSDC_EMMC;
		break;
	case SD_CARD_BOOT:
		host_function = MSDC_SD;
		boot = MSDC_BOOT_EN;
		break;
	case SD_CARD:
		host_function = MSDC_SD;
		break;
	default:
		pr_err("No supported storage type!");
		return 0;
	}
	host = msdc_get_host(host_function, boot, 0);
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);
	switch (info_type) {
	case CARD_INFO:
		if (host->mmc && host->mmc->card)
			info->card = host->mmc->card;
		else {
			pr_err("CARD was not ready<get card>!");
			return 0;
		}
		break;
	case DISK_INFO:
		if (host->mmc && host->mmc->card)
			info->disk = mmc_get_disk(host->mmc->card);
		else {
			pr_err("CARD was not ready<get disk>!");
			return 0;
		}
		break;
	case EMMC_USER_CAPACITY:
		info->emmc_user_capacity = msdc_get_capacity(0);
		break;
	case EMMC_CAPACITY:
		info->emmc_capacity = msdc_get_capacity(1);
		break;
	case EMMC_RESERVE:
#ifdef CONFIG_MTK_EMMC_SUPPORT
		info->emmc_reserve = msdc_get_reserve();
#endif
		break;
	default:
		pr_err("Please check INFO_TYPE");
		return 0;
	}
	return 1;
}

#ifdef CONFIG_MTK_EMMC_SUPPORT
static int simple_mmc_get_disk_info(struct mbr_part_info *mpi,
	unsigned char *name)
{
	char *no_partition_name = "n/a";
	struct disk_part_iter piter;
	struct hd_struct *part;
	struct msdc_host *host;
	struct gendisk *disk;

	/* emmc always in slot0 */
	host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN, 0);

	disk = mmc_get_disk(host->mmc->card);

	/* Find partition info in this way to
	 * avoid addr translation in scatter file and 64bit address calculate */
	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter))) {
#if DEBUG_MMC_IOCTL
		pr_debug("part_name = %s name = %s\n", part->info->volname,
			name);
#endif
		if (!strncmp(part->info->volname, name,
			PARTITION_NAME_LENGTH)) {
			mpi->start_sector = part->start_sect;
			mpi->nr_sects = part->nr_sects;
			mpi->part_no = part->partno;
			if (part->info)
				mpi->part_name = part->info->volname;
			else
				mpi->part_name = no_partition_name;

			disk_part_iter_exit(&piter);
			return 0;
		}
	}
	disk_part_iter_exit(&piter);

	return 1;
}

/* call mmc block layer interface for userspace to do erase operate */
static int simple_mmc_erase_func(unsigned int start, unsigned int size)
{
	struct msdc_host *host;
	unsigned int arg;

	/* emmc always in slot0 */
	host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN, 0);
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);

	mmc_claim_host(host->mmc);

	if (mmc_can_discard(host->mmc->card)) {
		arg = __MMC_DISCARD_ARG;
	} else if (mmc_can_trim(host->mmc->card)) {
		arg = __MMC_TRIM_ARG;
	} else if (mmc_can_erase(host->mmc->card)) {
		arg = __MMC_ERASE_ARG;
	} else {
		pr_err("[%s]: emmc card can't support trim / discard / erase\n",
			__func__);
		goto end;
	}

	pr_debug("[%s]: start=0x%x, size=%d, arg=0x%x, can_trim=(0x%x), EXT_CSD_SEC_GB_CL_EN=0x%lx\n",
		__func__, start, size, arg,
		host->mmc->card->ext_csd.sec_feature_support,
		EXT_CSD_SEC_GB_CL_EN);
	mmc_erase(host->mmc->card, start, size, arg);

#if DEBUG_MMC_IOCTL
	pr_debug("[%s]: erase done....arg=0x%x\n", __func__, arg);
#endif
 end:
	mmc_release_host(host->mmc);

	return 0;
}
#endif

static int simple_mmc_erase_partition(unsigned char *name)
{
#ifdef CONFIG_MTK_EMMC_SUPPORT
	struct mbr_part_info mbr_part;
	int l_ret = -1;

	BUG_ON(!name);
	/* just support erase cache & data partition now */
	if (strncmp(name, "usrdata", 7) == 0 ||
	    strncmp(name, "cache", 5) == 0) {
		/* find erase start address and erase size,
		just support high capacity emmc card now */
		l_ret = simple_mmc_get_disk_info(&mbr_part, name);

		if (l_ret == 0) {
			/* do erase */
			pr_debug("erase %s start sector: 0x%x size: 0x%x\n",
				mbr_part.part_name,
				mbr_part.start_sector, mbr_part.nr_sects);
			simple_mmc_erase_func(mbr_part.start_sector,
				mbr_part.nr_sects);
		}
	}

	return 0;
#else
	return 0;
#endif

}

static int simple_mmc_erase_partition_wrap(struct msdc_ioctl *msdc_ctl)
{
	unsigned char name[PARTITION_NAME_LENGTH];

	if (!msdc_ctl)
		return -EINVAL;

	if (msdc_ctl->total_size > PARTITION_NAME_LENGTH)
		return -EFAULT;

	if (copy_from_user(name, (unsigned char *)msdc_ctl->buffer,
		msdc_ctl->total_size))
		return -EFAULT;

	return simple_mmc_erase_partition(name);
}

static long simple_sd_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	struct msdc_ioctl msdc_ctl;
	int ret;

	if ((struct msdc_ioctl *)arg == NULL) {
		switch (cmd) {
		case MSDC_REINIT_SDCARD:
			ret = sd_ioctl_reinit((struct msdc_ioctl *)arg);
			break;

		case MSDC_CD_PIN_EN_SDCARD:
			ret = sd_ioctl_cd_pin_en((struct msdc_ioctl *)arg);
			break;

		default:
			pr_err("mt_sd_ioctl:this opcode value is illegal!!\n");
			return -EINVAL;
		}
		return ret;
	}
	if (copy_from_user(&msdc_ctl, (struct msdc_ioctl *)arg,
				sizeof(struct msdc_ioctl))) {
		return -EFAULT;
	}

	switch (msdc_ctl.opcode) {
	case MSDC_SINGLE_READ_WRITE:
	case MSDC_MULTIPLE_READ_WRITE:
		msdc_ctl.result = simple_sd_ioctl_rw(&msdc_ctl);
		break;
	case MSDC_GET_CID:
		msdc_ctl.result = simple_sd_ioctl_get_cid(&msdc_ctl);
		break;
	case MSDC_GET_CSD:
		msdc_ctl.result = simple_sd_ioctl_get_csd(&msdc_ctl);
		break;
	case MSDC_GET_EXCSD:
		msdc_ctl.result = simple_sd_ioctl_get_excsd(&msdc_ctl);
		break;
	case MSDC_DRIVING_SETTING:
		pr_debug("in ioctl to change driving\n");
		if (1 == msdc_ctl.iswrite) {
			msdc_ctl.result =
				simple_sd_ioctl_set_driving(&msdc_ctl);
		} else {
			msdc_ctl.result =
				simple_sd_ioctl_get_driving(&msdc_ctl);
		}
		break;
	case MSDC_ERASE_PARTITION:
		msdc_ctl.result =
			simple_mmc_erase_partition_wrap(&msdc_ctl);
		break;
	case MSDC_SD30_MODE_SWITCH:
		msdc_ctl.result =
			simple_sd_ioctl_sd30_mode_switch(&msdc_ctl);
		break;
	default:
		pr_err("simple_sd_ioctl:invlalid opcode!!\n");
		return -EINVAL;
	}

	if (copy_to_user((struct msdc_ioctl *)arg, &msdc_ctl,
				sizeof(struct msdc_ioctl))) {
		return -EFAULT;
	}

	return msdc_ctl.result;

}

static const struct file_operations simple_msdc_em_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = simple_sd_ioctl,
	.open = simple_sd_open,
};

static struct miscdevice simple_msdc_em_dev[] = {
	{
	 .minor = MISC_DYNAMIC_MINOR,
	 .name = "misc-sd",
	 .fops = &simple_msdc_em_fops,
	 }
};

static int simple_sd_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("in misc_sd_probe function\n");

	return ret;
}

static int simple_sd_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver simple_sd_driver = {
	.probe = simple_sd_probe,
	.remove = simple_sd_remove,

	.driver = {
		.name = DRV_NAME_MISC,
		.owner = THIS_MODULE,
	},
};

static int __init simple_sd_init(void)
{
	int ret;

	sg_msdc_multi_buffer = kmalloc(64 * 1024, GFP_KERNEL);

	ret = platform_driver_register(&simple_sd_driver);
	if (ret) {
		pr_err(DRV_NAME_MISC ": Can't register driver\n");
		return ret;
	}
	pr_debug(DRV_NAME_MISC ": MediaTek simple SD/MMC Card Driver\n");

	/*msdc0 is for emmc only, just for emmc */
	/* ret = misc_register(&simple_msdc_em_dev[host->id]); */
	ret = misc_register(&simple_msdc_em_dev[0]);
	if (ret) {
		pr_err("register MSDC Slot[0] misc driver failed (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit simple_sd_exit(void)
{
	if (sg_msdc_multi_buffer != NULL) {
		kfree(sg_msdc_multi_buffer);
		sg_msdc_multi_buffer = NULL;
	}

	misc_deregister(&simple_msdc_em_dev[0]);

	platform_driver_unregister(&simple_sd_driver);
}

module_init(simple_sd_init);
module_exit(simple_sd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("simple MediaTek SD/MMC Card Driver");
MODULE_AUTHOR("feifei.wang <feifei.wang@mediatek.com>");
