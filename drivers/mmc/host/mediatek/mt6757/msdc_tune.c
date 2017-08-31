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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/gpio.h>
#include <linux/delay.h>

#include "mtk_sd.h"
#include <mmc/core/core.h>
#include "dbg.h"
#include "autok.h"
#include "autok_dvfs.h"

void msdc_sdio_restore_after_resume(struct msdc_host *host)
{
	void __iomem *base = host->base;

	if (host->saved_para.hz) {
		if ((host->saved_para.suspend_flag) ||
				((host->saved_para.msdc_cfg != 0) &&
				((host->saved_para.msdc_cfg&0xFFFFFF9F) !=
				(MSDC_READ32(MSDC_CFG)&0xFFFFFF9F)))) {
			ERR_MSG("msdc resume[ns] cur_cfg=%x, save_cfg=%x\n",
				MSDC_READ32(MSDC_CFG),
				host->saved_para.msdc_cfg);
			ERR_MSG("cur_hz=%d, save_hz=%d\n",
				host->mclk, host->saved_para.hz);

			host->saved_para.suspend_flag = 0;
			msdc_restore_timing_setting(host);
		}
	}
}

void msdc_save_timing_setting(struct msdc_host *host, int save_mode)
{
	struct msdc_hw *hw = host->hw;
	void __iomem *base = host->base;
	/* save_mode: 1 emmc_suspend
	 *	      2 sdio_suspend
	 *	      3 power_tuning
	 *	      4 power_off
	 */

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);

	if ((save_mode == 1) || (save_mode == 2)) {
		host->saved_para.hz = host->mclk;
		host->saved_para.sdc_cfg = MSDC_READ32(SDC_CFG);
	}

	if (((save_mode == 1) || (save_mode == 4))
	 && (hw->host_function == MSDC_EMMC)) {
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			host->saved_para.ds_dly1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			host->saved_para.ds_dly3);
		host->saved_para.emmc50_pad_cmd_tune =
			MSDC_READ32(EMMC50_PAD_CMD_TUNE);
		host->saved_para.emmc50_dat01 =
			MSDC_READ32(EMMC50_PAD_DAT01_TUNE);
		host->saved_para.emmc50_dat23 =
			MSDC_READ32(EMMC50_PAD_DAT23_TUNE);
		host->saved_para.emmc50_dat45 =
			MSDC_READ32(EMMC50_PAD_DAT45_TUNE);
		host->saved_para.emmc50_dat67 =
			MSDC_READ32(EMMC50_PAD_DAT67_TUNE);
	}

	if (save_mode == 1) {
		host->saved_para.timing = host->timing;
		host->saved_para.msdc_cfg = MSDC_READ32(MSDC_CFG);
		host->saved_para.iocon = MSDC_READ32(MSDC_IOCON);
	}

	if (save_mode == 2) {
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, host->saved_para.mode);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, host->saved_para.div);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->saved_para.int_dat_latch_ck_sel);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			host->saved_para.ckgen_msdc_dly_sel);
		MSDC_GET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			host->saved_para.inten_sdio_irq);
		host->saved_para.msdc_cfg = MSDC_READ32(MSDC_CFG);

		host->saved_para.iocon = MSDC_READ32(MSDC_IOCON);

		host->saved_para.timing = host->timing;
	}

	host->saved_para.pad_tune0 = MSDC_READ32(MSDC_PAD_TUNE0);
	host->saved_para.pad_tune1 = MSDC_READ32(MSDC_PAD_TUNE1);

	host->saved_para.ddly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
	host->saved_para.ddly1 = MSDC_READ32(MSDC_DAT_RDDLY1);

	host->saved_para.pb1 = MSDC_READ32(MSDC_PATCH_BIT1);
	host->saved_para.pb2 = MSDC_READ32(MSDC_PATCH_BIT2);
	host->saved_para.sdc_fifo_cfg = MSDC_READ32(SDC_FIFO_CFG);

	/*msdc_dump_register(host);*/
}

void msdc_set_bad_card_and_remove(struct msdc_host *host)
{
	unsigned long flags;

	if (host == NULL) {
		pr_err("WARN: host is NULL");
		return;
	}
	host->card_inserted = 0;

	if ((host->mmc == NULL) || (host->mmc->card == NULL)) {
		ERR_MSG("WARN: mmc or card is NULL");
		return;
	}

	if (host->mmc->card) {
		spin_lock_irqsave(&host->remove_bad_card, flags);
		host->block_bad_card = 1;

		mmc_card_set_removed(host->mmc->card);
		spin_unlock_irqrestore(&host->remove_bad_card, flags);

#ifndef CONFIG_GPIOLIB
		ERR_MSG("Cannot get gpio %d level", cd_gpio);
#else
		if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE)
		 && (host->hw->cd_level == __gpio_get_value(cd_gpio))) {
			/* do nothing */
		} else
#endif
		{
			mmc_remove_card(host->mmc->card);
			host->mmc->card = NULL;
			mmc_detach_bus(host->mmc);
			mmc_power_off(host->mmc);
		}

		ERR_MSG("Remove the bad card, block_bad_card=%d, card_inserted=%d",
			host->block_bad_card, host->card_inserted);
	}
}

/*  HS400 can not lower frequence
 *  1. Change to HS200 mode, reinit
 *  2. Lower frequence
 */
int emmc_reinit_tuning(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	void __iomem *base = host->base;
	u32 div = 0;
	u32 mode = 0;
	unsigned int caps_hw_reset = 0;

	if (!mmc->card) {
		pr_err("mmc card = NULL, skip reset tuning\n");
		return -1;
	}

	if (mmc->card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400) {
		mmc->card->mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS400;
		pr_err("msdc%d: witch to HS200 mode, reinit card\n", host->id);
		if (mmc->caps & MMC_CAP_HW_RESET) {
			caps_hw_reset = 1;
		} else {
			caps_hw_reset = 0;
			mmc->caps |= MMC_CAP_HW_RESET;
		}
		mmc->ios.timing = MMC_TIMING_LEGACY;
		mmc->ios.clock = 260000;
		msdc_ops_set_ios(mmc, &mmc->ios);
		if (mmc_hw_reset(mmc))
			pr_err("msdc%d switch HS200 failed\n", host->id);
		/* recovery MMC_CAP_HW_RESET */
		if (!caps_hw_reset)
			mmc->caps &= ~MMC_CAP_HW_RESET;
		goto done;
	}

	/* lower frequence */
	if (mmc->card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200) {
		mmc->card->mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS200;
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, div);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, mode);
		div += 1;
		if (div > EMMC_MAX_FREQ_DIV) {
			pr_err("msdc%d: max lower freq: %d\n", host->id, div);
			return 1;
		}
		msdc_clk_stable(host, mode, div, 1);
		host->sclk =
			(div == 0) ? host->hclk / 4 : host->hclk / (4 * div);
		pr_err("msdc%d: HS200 mode lower frequence to %dMhz\n",
			host->id, host->sclk / 1000000);
	}

done:
	return 0;

}
/* SDcard will change speed mode and power reset
 * UHS_SDR104 --> UHS_DDR50 --> UHS_SDR50 --> UHS_SDR25
 */
int sdcard_reset_tuning(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;

	if (!mmc->card) {
		pr_err("mmc card = NULL, skip reset tuning\n");
		return -1;
	}

	if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR104) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR104;
		pr_err("msdc%d remove UHS_SDR104 mode\n", host->id);
	} else if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_DDR50) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_DDR50;
		pr_err("msdc%d remove UHS_DDR50 mode\n", host->id);
	} else if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR50) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR50;
		pr_err("msdc%d remove UHS_SDR50 mode\n", host->id);
	} else if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR25) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR25;
		pr_err("msdc%d remove UHS_SDR25 mode\n", host->id);
	}

	pr_err("msdc%d reinit card\n", host->id);
	mmc->ios.timing = MMC_TIMING_LEGACY;
	mmc->ios.clock = 260000;
	msdc_ops_set_ios(mmc, &mmc->ios);
	/* power reset sdcard */
	ret = mmc_hw_reset(mmc);
	if (ret)
		pr_err("msdc%d power reset failed\n", host->id);
	return ret;
}
/*
 * register as callback function of WIFI(combo_sdio_register_pm) .
 * can called by msdc_drv_suspend/resume too.
 */
void msdc_restore_timing_setting(struct msdc_host *host)
{
	void __iomem *base = host->base;
	int retry = 3;
	int emmc = (host->hw->host_function == MSDC_EMMC) ? 1 : 0;
	int sdio = (host->hw->host_function == MSDC_SDIO) ? 1 : 0;

	if (sdio) {
		msdc_reset_hw(host->id); /* force bit5(BV18SDT) to 0 */
		host->saved_para.msdc_cfg =
			host->saved_para.msdc_cfg & 0xFFFFFFDF;
		MSDC_WRITE32(MSDC_CFG, host->saved_para.msdc_cfg);
	}

	do {
		msdc_set_mclk(host, host->saved_para.timing,
			host->saved_para.hz);
		if ((MSDC_READ32(MSDC_CFG) & 0xFFFFFF9F) ==
		    (host->saved_para.msdc_cfg & 0xFFFFFF9F))
			break;
		ERR_MSG("msdc set_mclk is unstable (cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d)",
			MSDC_READ32(MSDC_CFG),
			host->saved_para.msdc_cfg, host->mclk,
			host->saved_para.hz);
	} while (retry--);

	MSDC_WRITE32(SDC_CFG, host->saved_para.sdc_cfg);

	MSDC_WRITE32(MSDC_IOCON, host->saved_para.iocon);

	MSDC_WRITE32(MSDC_PAD_TUNE0, host->saved_para.pad_tune0);
	MSDC_WRITE32(MSDC_PAD_TUNE1, host->saved_para.pad_tune1);

	MSDC_WRITE32(MSDC_DAT_RDDLY0, host->saved_para.ddly0);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, host->saved_para.ddly1);

	MSDC_WRITE32(MSDC_PATCH_BIT1, host->saved_para.pb1);
	MSDC_WRITE32(MSDC_PATCH_BIT2, host->saved_para.pb2);
	MSDC_WRITE32(SDC_FIFO_CFG, host->saved_para.sdc_fifo_cfg);

	if (sdio) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->saved_para.int_dat_latch_ck_sel);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			host->saved_para.ckgen_msdc_dly_sel);
		MSDC_SET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			host->saved_para.inten_sdio_irq);

		autok_init_sdr104(host);
		if (vcorefs_get_hw_opp() == OPPI_PERF)
			autok_tuning_parameter_init(host,
				host->autok_res[AUTOK_VCORE_HIGH]);
		else
			autok_tuning_parameter_init(host,
				host->autok_res[AUTOK_VCORE_LOW]);

		sdio_dvfs_reg_restore(host);

		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
		host->mmc->rescan_entered = 0;
	}

	if (emmc) {
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			host->saved_para.ds_dly1);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			host->saved_para.ds_dly3);
		MSDC_WRITE32(EMMC50_PAD_CMD_TUNE,
			host->saved_para.emmc50_pad_cmd_tune);
		MSDC_WRITE32(EMMC50_PAD_DAT01_TUNE,
			host->saved_para.emmc50_dat01);
		MSDC_WRITE32(EMMC50_PAD_DAT23_TUNE,
			host->saved_para.emmc50_dat23);
		MSDC_WRITE32(EMMC50_PAD_DAT45_TUNE,
			host->saved_para.emmc50_dat45);
		MSDC_WRITE32(EMMC50_PAD_DAT67_TUNE,
			host->saved_para.emmc50_dat67);
	}

	/*msdc_dump_register(host);*/
}
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing)
{
	void __iomem *base = host->base;

	MSDC_WRITE32(MSDC_PAD_TUNE0,   0x00000000);

	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_DDLSEL);
	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL);
	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_R_D_SMPL);
	if (timing == MMC_TIMING_MMC_HS400) {
		MSDC_CLR_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
	} else {
		MSDC_SET_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL);
		MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL);
	}

	if (timing == MMC_TIMING_MMC_HS400)
		MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
	else
		MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);

	MSDC_CLR_BIT32(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);

	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);
	MSDC_SET_BIT32(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL);
	MSDC_CLR_BIT32(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL);

	MSDC_CLR_BIT32(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL);

	autok_path_sel(host);
}

void msdc_init_tune_setting(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* Fix HS400 mode */
	MSDC_CLR_BIT32(EMMC50_CFG0, MSDC_EMMC50_CFG_TXSKEW_SEL);
	MSDC_SET_BIT32(MSDC_PATCH_BIT1, MSDC_PB1_DDR_CMD_FIX_SEL);

	/* DDR50 mode */
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_DDR50_SEL);

	/* 64T + 48T cmd <-> resp */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT,
		MSDC_PB2_DEFAULT_RESPWAITCNT);

	autok_path_sel(host);
}

void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios)
{
	autok_msdc_tx_setting(host, ios);
}
