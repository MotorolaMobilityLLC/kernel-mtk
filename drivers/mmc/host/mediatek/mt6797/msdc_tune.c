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
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include "mt_sd.h"
#include "dbg.h"
#include "autok.h"


int g_ett_tune = 0;	  /* enable or disable the ETT tune */
int g_ett_hs400_tune = 0; /* record the number of failed HS400 ETT settings */
int g_ett_cmd_tune = 0;   /* record the number of failed command ETT settings*/
int g_ett_read_tune = 0;  /* record the number of failed read ETT settings */
int g_ett_write_tune = 0; /* record the number of failed write ETT settings */
int g_reset_tune = 0;	  /* do not record the pass settigns, but try the worst
			     setting of each request. */

u32 sdio_enable_tune = 0;
u32 sdio_iocon_dspl = 0;
u32 sdio_iocon_w_dspl = 0;
u32 sdio_iocon_rspl = 0;
u32 sdio_pad_tune_rrdly = 0;
u32 sdio_pad_tune_rdly = 0;
u32 sdio_pad_tune_wrdly = 0;
u32 sdio_dat_rd_dly0_0 = 0;
u32 sdio_dat_rd_dly0_1 = 0;
u32 sdio_dat_rd_dly0_2 = 0;
u32 sdio_dat_rd_dly0_3 = 0;
u32 sdio_dat_rd_dly1_0 = 0;
u32 sdio_dat_rd_dly1_1 = 0;
u32 sdio_dat_rd_dly1_2 = 0;
u32 sdio_dat_rd_dly1_3 = 0;
u32 sdio_clk_drv = 0;
u32 sdio_cmd_drv = 0;
u32 sdio_data_drv = 0;
u32 sdio_tune_flag = 0;

void msdc_sdio_set_long_timing_delay_by_freq(struct msdc_host *host, u32 clock)
{
#ifdef CONFIG_SDIOAUTOK_SUPPORT
	void __iomem *base = host->base;
	struct msdc_hw *hw = host->hw;
	struct msdc_saved_para *saved_para = &(host->saved_para);

	if (clock >= 200000000) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
			hw->wdatcrctactr_sdr200);
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
			hw->cmdrtactr_sdr200);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			hw->intdatlatcksel_sdr200);
		saved_para->cmd_resp_ta_cntr = hw->cmdrtactr_sdr200;
		saved_para->wrdat_crc_ta_cntr = hw->wdatcrctactr_sdr200;
		saved_para->int_dat_latch_ck_sel = hw->intdatlatcksel_sdr200;
	} else {
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
			hw->wdatcrctactr_sdr50);
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
			hw->cmdrtactr_sdr50);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			hw->intdatlatcksel_sdr50);
		saved_para->cmd_resp_ta_cntr = hw->cmdrtactr_sdr50;
		saved_para->wrdat_crc_ta_cntr = hw->wdatcrctactr_sdr50;
		saved_para->int_dat_latch_ck_sel = hw->intdatlatcksel_sdr50;
	}
#else
	return;
#endif
}

void msdc_save_timing_setting(struct msdc_host *host, u32 init_hw,
	u32 emmc_suspend, u32 sdio_suspend, u32 power_tuning, u32 power_off)
{
	struct msdc_hw *hw = host->hw;
	void __iomem *base = host->base;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, hw->ddlsel);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);

	if (emmc_suspend || sdio_suspend) {
		host->saved_para.hz = host->mclk;
		host->saved_para.sdc_cfg = MSDC_READ32(SDC_CFG);
	}

	if ((emmc_suspend || power_off) && (hw->host_function == MSDC_EMMC)) {
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

	if (emmc_suspend) {
		host->saved_para.timing = host->timing;
		host->saved_para.msdc_cfg = MSDC_READ32(MSDC_CFG);
		host->saved_para.iocon = MSDC_READ32(MSDC_IOCON);
	}

	if (sdio_suspend) {
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
	/*msdc_dump_register(host);*/
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

	if (mmc->card == NULL) {
		pr_err("msdc%d emmc is under init, don't reset\n", host->id);
		return -EINVAL;
	}
	if (mmc->card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400) {
		mmc->card->mmc_avail_type &= ~EXT_CSD_CARD_TYPE_HS400;
		pr_err("msdc%d: witch to HS200 mode,reinit card\n", host->id);
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
			pr_err("msdc%d: max lower freq dev: %d\n", host->id, div);
			return -EINVAL;
		}
		msdc_clk_stable(host, mode, div, 1);
		host->sclk = (div == 0) ? host->hclk / 4 : host->hclk / (4 * div);
		pr_err("msdc%d: HS200 mode lower frequence to %d Mhz\n",
			host->id, host->sclk / 1000000);
	}
done:
	return 0;

}

int sdcard_hw_reset(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;

	/* power reset sdcard */
	mmc->ios.timing = MMC_TIMING_LEGACY;
	mmc->ios.clock = 260000;
	msdc_ops_set_ios(mmc, &mmc->ios);
	ret = mmc_hw_reset(mmc);
	if (ret)
		pr_notice("msdc%d power reset failed, block_bad_card = %d\n",
			host->id, host->block_bad_card);
	else
		pr_notice("msdc%d power reset success\n", host->id);
	return ret;
}

/* SDcard will change speed mode and power reset
 * UHS_SDR104 --> UHS_DDR50 --> UHS_SDR50 --> UHS_SDR25
 */
int sdcard_reset_tuning(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	int ret = 0;

	if (mmc->card == NULL) {
		pr_err("msdc%d sdcard is under init, don't reset\n", host->id);
		return -EINVAL;
	}

	if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR104) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR104;
		pr_err("msdc%d remove UHS_SDR104 mode\n", host->id);
		goto power_reinit;
	}
	if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_DDR50) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_DDR50;
		pr_err("msdc%d remove UHS_DDR50 mode\n", host->id);
		goto power_reinit;
	}
	if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR50) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR50;
		pr_err("msdc%d remove UHS_SDR50 mode\n", host->id);
		goto power_reinit;
	}
	if (mmc->card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR25) {
		mmc->card->sw_caps.sd3_bus_mode &= ~SD_MODE_UHS_SDR25;
		pr_err("msdc%d remove UHS_SDR25 mode\n", host->id);
		goto power_reinit;
	}

power_reinit:
	pr_err("msdc%d reinit card\n",
		host->id);
	ret = sdcard_hw_reset(mmc);
	return ret;
}
/*
   register as callback function of WIFI(combo_sdio_register_pm) .
   can called by msdc_drv_suspend/resume too.
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
		ERR_MSG("msdc set_mclk is unstable (cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d).",
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

	if (sdio) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			host->saved_para.int_dat_latch_ck_sel);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			host->saved_para.ckgen_msdc_dly_sel);
		MSDC_SET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			host->saved_para.inten_sdio_irq);
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
	/* tune path and related key hw fixed setting should be wrappered follow interface */
	autok_path_sel(host);
}

void msdc_init_tune_setting(struct msdc_host *host)
{
	struct msdc_hw *hw = host->hw;
	void __iomem *base = host->base;

	/* pr_err("msdc_init_tune_setting\n"); */
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY,
		MSDC_CLKTXDLY);
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY,
		hw->datwrddly);
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY,
		hw->cmdrrddly);
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY,
		hw->cmdrddly);

	MSDC_WRITE32(MSDC_IOCON, 0x00000000);

	MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);

	MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFE00C9);

	/* 64T + 48T cmd <-> resp */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT, 3);
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL, 0);
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, 0);

	/* low speed mode should be switch data tune path too even if not covered by autok */
	autok_path_sel(host);
}
void msdc_ios_tune_setting(struct msdc_host *host,
	struct mmc_ios *ios)
{
	autok_msdc_tx_setting(host, ios);
}
