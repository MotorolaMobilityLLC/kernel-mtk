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

#define CMD_TUNE_UHS_MAX_TIME           (2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME            (2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME  (2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME          (2*32*32)
#define READ_TUNE_HS_MAX_TIME           (2*32)

#define WRITE_TUNE_HS_MAX_TIME          (2*32)
#define WRITE_TUNE_UHS_MAX_TIME         (2*32*8)

#define MSDC_LOWER_FREQ
#define MSDC_MAX_FREQ_DIV               (2)
#define MSDC_MAX_TIMEOUT_RETRY          (1)
#define MSDC_MAX_TIMEOUT_RETRY_EMMC     (2)
#define MSDC_MAX_W_TIMEOUT_TUNE         (5)
#define MSDC_MAX_W_TIMEOUT_TUNE_EMMC    (64)
#define MSDC_MAX_R_TIMEOUT_TUNE         (3)
#define MSDC_MAX_POWER_CYCLE            (5)

#define MAX_HS400_TUNE_COUNT            (576) /*(32*18)*/

#define CMD_SET_FOR_MMC_TUNE_CASE1      (0x00000000FB260140ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE2      (0x0000000000000080ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE3      (0x0000000000001000ULL)
#define CMD_SET_FOR_MMC_TUNE_CASE4      (0x0000000000000020ULL)
/*#define CMD_SET_FOR_MMC_TUNE_CASE5      (0x0000000000084000ULL)*/

#define CMD_SET_FOR_SD_TUNE_CASE1       (0x000000007B060040ULL)
#define CMD_SET_FOR_APP_TUNE_CASE1      (0x0008000000402000ULL)

#define IS_IN_CMD_SET(cmd_num, set)     ((0x1ULL << cmd_num) & (set))

#define MSDC_VERIFY_NEED_TUNE           (0)
#define MSDC_VERIFY_ERROR               (1)
#define MSDC_VERIFY_NEED_NOT_TUNE       (2)


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

#ifdef MSDC_SUPPORT_SANDISK_COMBO_ETT
struct msdc_ett_settings msdc0_ett_settings_for_sandisk[] = {
	{ MSDC_HS200_MODE, 0xb0,  (0x7 << 7), 0 },
		/*PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL] */
	{ MSDC_HS200_MODE, 0xb0,  (0x1f << 10), 0 },
		/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL] */
	{ MSDC_HS400_MODE, 0xb0,  (0x7 << 7), 0 },
		/*PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL] */
	{ MSDC_HS400_MODE, 0xb0,  (0x1f << 10), 0 },
		/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL] */
	{ MSDC_HS400_MODE, 0x188, (0x1f << 2), 2 /*0x0*/ },
		/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY1] */
	{ MSDC_HS400_MODE, 0x188, (0x1f << 12), 18 /*0x13*/},
		/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY3] */

	/* command & resp ett settings */
	{ MSDC_HS200_MODE, 0xb4,  (0x7 << 3), 1 },
		/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 1), 1 },
		/*MSDC_IOCON[MSDC_IOCON_RSPL] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 16), 0 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 22), 6 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY] */

	{ MSDC_HS400_MODE, 0xb4,  (0x7 << 3), 1 },
		/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR] */
	{ MSDC_HS400_MODE, 0x4,   (0x1 << 1), 1 },
		/*MSDC_IOCON[MSDC_IOCON_RSPL] */
	{ MSDC_HS400_MODE, 0xf0,  (0x1f << 16), 0 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY] */
	{ MSDC_HS400_MODE, 0xf0,  (0x1f << 22), 11 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY] */

	/* write ett settings */
	{ MSDC_HS200_MODE, 0xb4,  (0x7 << 0), 1 },
		/*PATCH_BIT1[MSDC_PB1_WRDAT_CRCS_TA_CNTR] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 0), 15 },
		/*PAD_TUNE[MSDC_PAD_TUNE_DATWRDLY] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 10), 1 },
		/*MSDC_IOCON[MSDC_IOCON_W_D0SPL] */
	{ MSDC_HS200_MODE, 0xf8,  (0x1f << 24), 5 },
		/*DAT_RD_DLY0[MSDC_DAT_RDDLY0_D0] */

	/* read ett settings */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 8), 18},
		/*PAD_TUNE[MSDC_PAD_TUNE_DATRRDLY] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 2), 3 },
		/*MSDC_IOCON[MSDC_IOCON_R_D_SMPL]     */
};
#endif /* mt6735m MSDC_SUPPORT_SANDISK_COMBO_ETT */

#ifdef MSDC_SUPPORT_SAMSUNG_COMBO_ETT
struct msdc_ett_settings msdc0_ett_settings_for_samsung[] = {
	{ MSDC_HS200_MODE, 0xb0,  (0x7 << 7), 0 },
		/*PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL] */
	{ MSDC_HS200_MODE, 0xb0,  (0x1f << 10), 0 },
		/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL] */
	{ MSDC_HS400_MODE, 0xb0,  (0x7 << 7), 0 },
		/*PATCH_BIT0[MSDC_PB0_INT_DAT_LATCH_CK_SEL] */
	{ MSDC_HS400_MODE, 0xb0,  (0x1f << 10), 0 },
		/*PATCH_BIT0[MSDC_PB0_CKGEN_MSDC_DLY_SEL] */
	{ MSDC_HS400_MODE, 0x188, (0x1f << 2), 2 },
		/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY1] */
	{ MSDC_HS400_MODE, 0x188, (0x1f << 12), 18 },
		/*EMMC50_PAD_DS_TUNE[MSDC_EMMC50_PAD_DS_TUNE_DLY3] */

	/* command & resp ett settings */
	{ MSDC_HS200_MODE, 0xb4,  (0x7 << 3), 1 },
		/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 1), 1 },
		/*MSDC_IOCON[MSDC_IOCON_RSPL] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 16), 0 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 22), 6 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY] */

	{ MSDC_HS400_MODE, 0xb4,  (0x7 << 3), 1 },
		/*PATCH_BIT1[MSDC_PB1_CMD_RSP_TA_CNTR] */
	{ MSDC_HS400_MODE, 0x4,   (0x1 << 1), 1 },
		/*MSDC_IOCON[MSDC_IOCON_RSPL] */
	{ MSDC_HS400_MODE, 0xf0,  (0x1f << 16), 0 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRDLY] */
	{ MSDC_HS400_MODE, 0xf0,  (0x1f << 22), 11 },
		/*PAD_TUNE[MSDC_PAD_TUNE_CMDRRDLY] */

	/* write ett settings */
	{ MSDC_HS200_MODE, 0xb4,  (0x7 << 0), 1 },
		/*PATCH_BIT1[MSDC_PB1_WRDAT_CRCS_TA_CNTR] */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 0), 15 },
		/*PAD_TUNE[MSDC_PAD_TUNE_DATWRDLY] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 10), 1 },
		/*MSDC_IOCON[MSDC_IOCON_W_D0SPL] */
	{ MSDC_HS200_MODE, 0xf8,  (0x1f << 24), 5 },
		/*DAT_RD_DLY0[MSDC_DAT_RDDLY0_D0] */

	/* read ett settings */
	{ MSDC_HS200_MODE, 0xf0,  (0x1f << 8), 18},
		/*PAD_TUNE[MSDC_PAD_TUNE_DATRRDLY] */
	{ MSDC_HS200_MODE, 0x4,   (0x1 << 2), 4 },
		/*MSDC_IOCON[MSDC_IOCON_R_D_SMPL] */
};
#endif /* mt6735m MSDC_SUPPORT_SAMSUNG_COMBO_ETT */

int msdc_setting_parameter(struct msdc_hw *hw, unsigned int *para)
{
	struct tag_msdc_hw_para *msdc_para_hw_datap =
		(struct tag_msdc_hw_para *) para;

	if ((NULL == hw) || (msdc_para_hw_datap == NULL))
		return -1;

	if ((msdc_para_hw_datap->host_function != MSDC_SD)
	 && (msdc_para_hw_datap->host_function == MSDC_EMMC))
		return -1;

	if ((msdc_para_hw_datap->version == 0x5A01)
	 && (msdc_para_hw_datap->end_flag == 0x5a5a5a5a)) {
		memcpy(hw, msdc_para_hw_datap,
			sizeof(struct tag_msdc_hw_para));
		return 0;
	}
	return -1;
}

void msdc_reset_pwr_cycle_counter(struct msdc_host *host)
{
	host->power_cycle = 0;
	host->power_cycle_enable = 1;
}

void msdc_reset_tmo_tune_counter(struct msdc_host *host,
	unsigned int index)
{
	switch (index) {
	case all_counter:
	case cmd_counter:
		if (host->rwcmd_time_tune != 0)
			ERR_MSG("TMO TUNE CMD Times(%d)",
				host->rwcmd_time_tune);
		host->rwcmd_time_tune = 0;
		if (index == cmd_counter)
			break;
	case read_counter:
		if (host->read_time_tune != 0)
			ERR_MSG("TMO TUNE READ Times(%d)",
				host->read_time_tune);
		host->read_time_tune = 0;
		if (index == read_counter)
			break;
	case write_counter:
		if (host->write_time_tune != 0)
			ERR_MSG("TMO TUNE WRITE Times(%d)",
				host->write_time_tune);
		host->write_time_tune = 0;
		break;
	default:
		ERR_MSG("msdc%d ==> reset tmo counter index(%d) error!\n",
			host->id, index);
		break;
	}
}

void msdc_reset_crc_tune_counter(struct msdc_host *host,
	unsigned int index)
{
	void __iomem *base = host->base;
	struct tune_counter *t_counter = &host->t_counter;

	switch (index) {
	case all_counter:
		if (t_counter->time_hs400 != 0) {
			if (g_reset_tune) {
				MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
					MSDC_EMMC50_PAD_DS_TUNE_DLY1, 0x1c);
				MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
					MSDC_EMMC50_PAD_DS_TUNE_DLY3, 0xe);
			}
			ERR_MSG("TUNE HS400 Times(%d)", t_counter->time_hs400);
			if (g_ett_tune)
				g_ett_hs400_tune = t_counter->time_hs400;
		}
		t_counter->time_hs400 = 0;
		break;
	case cmd_counter:
		if (t_counter->time_cmd != 0) {
			ERR_MSG("CRC TUNE CMD Times(%d)",
				t_counter->time_cmd);
			if (g_ett_tune)
				g_ett_cmd_tune = t_counter->time_cmd;
		}
		t_counter->time_cmd = 0;
		if (index == cmd_counter)
			break;
		break;
	case read_counter:
		if (t_counter->time_read != 0) {
			ERR_MSG("CRC TUNE READ Times(%d)",
				t_counter->time_read);
			if (g_ett_tune)
				g_ett_read_tune = t_counter->time_read;
		}
		t_counter->time_read = 0;
		if (index == read_counter)
			break;
	case write_counter:
		if (t_counter->time_write != 0) {
			ERR_MSG("CRC TUNE WRITE Times(%d)",
				t_counter->time_write);
			if (g_ett_tune)
				g_ett_write_tune = t_counter->time_write;
		}
		t_counter->time_write = 0;
		break;
	default:
		ERR_MSG("msdc%d ==> reset crc counter index(%d) error!\n",
			host->id, index);
		break;
	}
}

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
	struct msdc_saved_para *saved_para = &(host->saved_para);
	void __iomem *base = host->base;
	u32 clkmode;

	if (emmc_suspend || sdio_suspend) {
		host->saved_para.hz = host->mclk;
		saved_para->sdc_cfg = MSDC_READ32(SDC_CFG);
	}

	if (emmc_suspend || power_tuning || power_off) {
		MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, hw->ddlsel);
		MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);
		MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
		MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);
	}

	if (init_hw || emmc_suspend || power_tuning || power_off) {
		saved_para->ddly1 = MSDC_READ32(MSDC_DAT_RDDLY1);
		MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA,
			saved_para->write_busy_margin);
		MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA,
			saved_para->write_crc_margin);
	}

	if (emmc_suspend || power_off || (init_hw && host->id == 0)) {
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			saved_para->ds_dly1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			saved_para->ds_dly3);
		saved_para->emmc50_pad_cmd_tune =
			MSDC_READ32(EMMC50_PAD_CMD_TUNE);
	}

	if (emmc_suspend) {
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, clkmode);
		saved_para->timing = host->timing;
	}

	if (emmc_suspend || init_hw) {
		MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,
			saved_para->cfg_crcsts_path);
		MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,
			saved_para->cfg_cmdrsp_path);
		MSDC_GET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT,
			host->saved_para.resp_wait_cnt);
	}

	if (sdio_suspend) {
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, saved_para->mode);
		MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, saved_para->div);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			saved_para->int_dat_latch_ck_sel);
		MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			saved_para->ckgen_msdc_dly_sel);
		MSDC_GET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			saved_para->inten_sdio_irq);
		host->saved_para.msdc_cfg = MSDC_READ32(MSDC_CFG);

		saved_para->iocon = MSDC_READ32(MSDC_IOCON);

		saved_para->timing = host->timing;
	}

	saved_para->pad_tune0 = MSDC_READ32(MSDC_PAD_TUNE0);
	saved_para->ddly0 = MSDC_READ32(MSDC_DAT_RDDLY0);

	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
		saved_para->cmd_resp_ta_cntr);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
		saved_para->wrdat_crc_ta_cntr);
}

/* 0 means pass */
u32 msdc_power_tuning(struct msdc_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct mmc_card *card;
	struct mmc_request *mrq;
	u32 power_cycle = 0;
	int read_timeout_tune = 0;
	int write_timeout_tune = 0;
	u32 rwcmd_timeout_tune = 0;
	u32 read_timeout_tune_uhs104 = 0;
	u32 write_timeout_tune_uhs104 = 0;
	u32 sw_timeout = 0;
	u32 ret = 1;
	u32 host_err = 0;

	if (!mmc)
		return 1;

	card = mmc->card;
	if (card == NULL) {
		ERR_MSG("mmc->card is NULL");
		return 1;
	}
	/* eMMC first */
#ifdef CONFIG_MTK_EMMC_SUPPORT
	if (mmc_card_mmc(card) && (host->hw->host_function == MSDC_EMMC))
		return 1;
#endif

	if (!host->error_tune_enable)
		return 1;

	if ((host->sd_30_busy > 0)
	 && (host->sd_30_busy <= MSDC_MAX_POWER_CYCLE)) {
		host->power_cycle_enable = 1;
	}

	if (!mmc_card_sd(card) || (host->hw->host_function != MSDC_SD))
		return ret;

	if (host->power_cycle == MSDC_MAX_POWER_CYCLE) {
		if (host->error_tune_enable) {
			ERR_MSG("do disable error tune flow of bad SD card");
			host->error_tune_enable = 0;
		}
		return ret;
	}

	if ((host->power_cycle > MSDC_MAX_POWER_CYCLE) ||
	    (!host->power_cycle_enable))
		return ret;

	/* power cycle */
	ERR_MSG("the %d time, Power cycle start", host->power_cycle);
	spin_unlock(&host->lock);

	if (host->power_control)
		host->power_control(host, 0);

	mdelay(10);

	if (host->power_control)
		host->power_control(host, 1);

	spin_lock(&host->lock);
	msdc_save_timing_setting(host, 0, 0, 0, 1, 0);
	if ((host->sclk > 100000000) && (host->power_cycle >= 1))
		mmc->caps &= ~MMC_CAP_UHS_SDR104;
	if (((host->sclk <= 100000000) &&
	   ((host->sclk > 50000000) || (host->timing == MMC_TIMING_UHS_DDR50)))
	   && (host->power_cycle >= 1)) {
		mmc->caps &= ~(MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104
			| MMC_CAP_UHS_DDR50);
	}

	msdc_host_mode[host->id] = mmc->caps;
	msdc_host_mode2[host->id] = mmc->caps2;

	/* clock should set to 260K*/
	mmc->ios.clock = HOST_MIN_MCLK;
	mmc->ios.bus_width = MMC_BUS_WIDTH_1;
	mmc->ios.timing = MMC_TIMING_LEGACY;
	msdc_set_mclk(host, MMC_TIMING_LEGACY, HOST_MIN_MCLK);

	/* re-init the card!*/
	mrq = host->mrq;
	host->mrq = NULL;
	power_cycle = host->power_cycle;
	host->power_cycle = MSDC_MAX_POWER_CYCLE;
	read_timeout_tune = host->read_time_tune;
	write_timeout_tune = host->write_time_tune;
	rwcmd_timeout_tune = host->rwcmd_time_tune;
	read_timeout_tune_uhs104 = host->read_timeout_uhs104;
	write_timeout_tune_uhs104 = host->write_timeout_uhs104;
	sw_timeout = host->sw_timeout;
	host_err = host->error;
	spin_unlock(&host->lock);
	ret = mmc_sd_power_cycle(mmc, card->ocr, card);
	spin_lock(&host->lock);
	host->mrq = mrq;
	host->power_cycle = power_cycle;
	host->read_time_tune = read_timeout_tune;
	host->write_time_tune = write_timeout_tune;
	host->rwcmd_time_tune = rwcmd_timeout_tune;
	if (host->sclk > 100000000) {
		host->write_timeout_uhs104 = write_timeout_tune_uhs104;
	} else {
		host->read_timeout_uhs104 = 0;
		host->write_timeout_uhs104 = 0;
	}
	host->sw_timeout = sw_timeout;
	host->error = host_err;
	if (!ret)
		host->power_cycle_enable = 0;
	ERR_MSG("the %d time, Power cycle Done, host->error(0x%x), ret(%d)",
		host->power_cycle, host->error, ret);
	(host->power_cycle)++;

	return ret;
}

static int msdc_lower_freq(struct msdc_host *host)
{
	u32 div, mode, hs400_src;
	void __iomem *base = host->base;
	u32 *hclks;

	ERR_MSG("need to lower freq");
	msdc_reset_crc_tune_counter(host, all_counter);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, mode);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKDIV, div);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD_HS400, hs400_src);

	hclks = msdc_get_hclks(host->id);

	if (div >= MSDC_MAX_FREQ_DIV) {
		ERR_MSG("div<%d> too large, change to power tuning", div);
		return msdc_power_tuning(host);
	} else if ((mode == 3) && (host->id == 0)) {
		/* when HS400 low freq, you cannot change to mode 2 (DDR mode),
		   else read data will be latched by clk instead of ds pin*/
		#if 0 /* HW not support 800MHz source */
		if (hs400_src == 1) {
			/*change from 400Mhz to 800Mhz,
			   because CCKDIV is invalid when 400Mhz clk src.*/
			hs400_src = 0;
			msdc_clock_src[host->id] = MSDC50_CLKSRC_400MHZ;
			host->hw->clk_src = msdc_clock_src[host->id];
			msdc_select_clksrc(host, host->hw->clk_src);
		}
		msdc_clk_stable(host, mode, div, hs400_src);
		host->sclk = hclks[host->hw->clk_src] / (2 * 4 * (div + 1));
		#else
		mode = 2; /* change to DDR */
		msdc_clock_src[host->id] = MSDC50_CLKSRC_200MHZ;
		host->hw->clk_src = msdc_clock_src[host->id];
		msdc_select_clksrc(host, host->hw->clk_src);
		div = 0;
		msdc_clk_stable(host, mode, div, hs400_src);
		host->sclk = hclks[host->hw->clk_src] / 2;
		#endif
	} else if (mode == 1) {
		mode = 0;
		msdc_clk_stable(host, mode, div, hs400_src);
		host->sclk = (div == 0) ?
			hclks[host->hw->clk_src]/2 :
			hclks[host->hw->clk_src]/(4*div);
	} else {
		msdc_clk_stable(host, mode, div + 1, hs400_src);
		host->sclk = (mode == 2) ?
			hclks[host->hw->clk_src]/(2*4*(div+1)) :
			hclks[host->hw->clk_src]/(4*(div+1));
	}

	ERR_MSG("new div<%d>, mode<%d> new freq.<%dKHz>",
		(mode == 1) ? div : div + 1, mode, host->sclk / 1000);
	return 0;
}

int hs400_restore_pad_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		mtk_msdc_host[0]->saved_para.pad_tune0 =
			MSDC_READ32(MSDC_PAD_TUNE0);
	}
	return 0;
}
int hs400_restore_pb1(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (!restore)
			MSDC_SET_FIELD(MSDC_PATCH_BIT1,
				MSDC_PB1_WRDAT_CRCS_TA_CNTR, 0x2);
		MSDC_GET_FIELD((MSDC_PATCH_BIT1), MSDC_PB1_WRDAT_CRCS_TA_CNTR,
			mtk_msdc_host[0]->saved_para.wrdat_crc_ta_cntr);
	}
	return 0;
}

int hs400_restore_ddly0(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		mtk_msdc_host[0]->saved_para.ddly0 =
			MSDC_READ32(MSDC_DAT_RDDLY0);
	}
	return 0;
}

int hs400_restore_ddly1(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		mtk_msdc_host[0]->saved_para.ddly1 =
			MSDC_READ32(MSDC_DAT_RDDLY1);
	}
	return 0;
}

int hs400_restore_cmd_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (!restore) {
			MSDC_SET_FIELD(EMMC50_PAD_CMD_TUNE,
				MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, 0x8);
		}
	}
	return 0;
}

int hs400_restore_dat01_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (0) {        /* (!restore) { */
			MSDC_SET_FIELD(EMMC50_PAD_DAT01_TUNE,
				MSDC_EMMC50_PAD_DAT0_TXDLY, 0x4);
			MSDC_SET_FIELD(EMMC50_PAD_DAT01_TUNE,
				MSDC_EMMC50_PAD_DAT1_TXDLY, 0x4);
		}
	}
	return 0;
}

int hs400_restore_dat23_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (0) {        /* (!restore) { */
			MSDC_SET_FIELD(EMMC50_PAD_DAT23_TUNE,
				MSDC_EMMC50_PAD_DAT2_TXDLY, 0x4);
			MSDC_SET_FIELD(EMMC50_PAD_DAT23_TUNE,
				MSDC_EMMC50_PAD_DAT3_TXDLY, 0x4);
		}
	}
	return 0;
}

int hs400_restore_dat45_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (0) {        /* (!restore) { */
			MSDC_SET_FIELD(EMMC50_PAD_DAT45_TUNE,
				MSDC_EMMC50_PAD_DAT4_TXDLY, 0x4);
			MSDC_SET_FIELD(EMMC50_PAD_DAT45_TUNE,
				MSDC_EMMC50_PAD_DAT5_TXDLY, 0x4);
		}
	}
	return 0;
}

int hs400_restore_dat67_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (0) {        /* (!restore) { */
			MSDC_SET_FIELD(EMMC50_PAD_DAT67_TUNE,
				MSDC_EMMC50_PAD_DAT6_TXDLY, 0x4);
			MSDC_SET_FIELD(EMMC50_PAD_DAT67_TUNE,
				MSDC_EMMC50_PAD_DAT7_TXDLY, 0x4);
		}
	}
	return 0;
}

int hs400_restore_ds_tune(int restore)
{
	void __iomem *base = 0;

	if (mtk_msdc_host[0]) {
		base = mtk_msdc_host[0]->base;
		if (0) {        /* (!restore) { */
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY1, 0x7);
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY3, 0x18);
		}
	}
	return 0;
}

/*
 *  2013-12-09
 *  HS400 error tune flow of read/write data error
 *  HS400 error tune flow of cmd error is same as eMMC4.5 backward speed mode.
 */
int emmc_hs400_tune_rw(struct msdc_host *host)
{
	void __iomem *base = host->base;
	int cur_ds_dly1 = 0, cur_ds_dly3 = 0;
	int orig_ds_dly1 = 0, orig_ds_dly3 = 0;
	int err = 0;

	if ((host->id != 0) || (host->timing != MMC_TIMING_MMC_HS400))
		return err;

	if (!host->error_tune_enable)
		return 1;

	MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
		orig_ds_dly1);
	MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
		orig_ds_dly3);

	if (g_ett_tune) {
		cur_ds_dly3 = orig_ds_dly3 + 1;
		cur_ds_dly1 = orig_ds_dly1;
		if (cur_ds_dly3 >= 32) {
			cur_ds_dly3 = 0;
			cur_ds_dly1 = orig_ds_dly1 + 1;
			if (cur_ds_dly1 >= 32)
				cur_ds_dly1 = 0;
		}
	} else {
		cur_ds_dly1 = orig_ds_dly1 - 1;
		cur_ds_dly3 = orig_ds_dly3;
		if (cur_ds_dly1 < 0) {
			cur_ds_dly1 = 17;
			cur_ds_dly3 = orig_ds_dly3 + 1;
			if (cur_ds_dly3 >= 32)
				cur_ds_dly3 = 0;
		}
	}

	if (++host->t_counter.time_hs400 ==
		(g_ett_tune ? (32 * 32) : MAX_HS400_TUNE_COUNT)) {
		ERR_MSG("Failed to update EMMC50_PAD_DS_TUNE_DLY, cur_ds_dly3=0x%x, cur_ds_dly1=0x%x",
			cur_ds_dly3, cur_ds_dly1);
#ifdef MSDC_LOWER_FREQ
		err = msdc_lower_freq(host);
#else
		err = 1;
#endif
		goto out;
	} else {
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
			MSDC_EMMC50_PAD_DS_TUNE_DLY1, cur_ds_dly1);
		if (cur_ds_dly3 != orig_ds_dly3) {
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY3, cur_ds_dly3);
		}
		INIT_MSG("HS400_TUNE: orig_ds_dly1<0x%x>, orig_ds_dly3<0x%x>, cur_ds_dly1<0x%x>, cur_ds_dly3<0x%x>",
			orig_ds_dly1, orig_ds_dly3, cur_ds_dly1, cur_ds_dly3);
	}
 out:
	/*Light*/
	pr_err("\nhs400_tune_rw iocon=%x\n", MSDC_READ32(MSDC_IOCON));
	return err;
}

/*
 *  2013-12-09
 *  different register settings between eMMC 4.5 backward speed mode
 *  and HS400 speed mode
 */
#define HS400_BACKUP_REG_NUM (12)
struct msdc_reg_control hs400_backup_reg_list[HS400_BACKUP_REG_NUM] = {
	/*   addr
		mask
		value   default
		restore_func */
	{(OFFSET_MSDC_PATCH_BIT0),
		(MSDC_PB0_INT_DAT_LATCH_CK_SEL),
		0x0,    0x0,
		NULL},
	{(OFFSET_MSDC_PATCH_BIT1),
		(MSDC_PB1_WRDAT_CRCS_TA_CNTR),
		0x0,    0x0,
		hs400_restore_pb1},
			/* defalut init value is 1, but HS400 shall be 0x0*/
	{(OFFSET_MSDC_IOCON),
		(MSDC_IOCON_R_D_SMPL | MSDC_IOCON_DDLSEL |
		 MSDC_IOCON_R_D_SMPL_SEL | MSDC_IOCON_R_D0SPL |
		 MSDC_IOCON_W_D_SMPL_SEL | MSDC_IOCON_W_D_SMPL),
		0x0,    0x0,
		NULL},
	{(OFFSET_MSDC_PAD_TUNE0),
		(MSDC_PAD_TUNE0_DATWRDLY | MSDC_PAD_TUNE0_DATRRDLY),
		0x0,    0x0,
		hs400_restore_pad_tune},
	{(OFFSET_MSDC_DAT_RDDLY0),
		(MSDC_DAT_RDDLY0_D3 | MSDC_DAT_RDDLY0_D2 |
		 MSDC_DAT_RDDLY0_D1 | MSDC_DAT_RDDLY0_D0),
		0x0,    0x0,
		hs400_restore_ddly0},
	{(OFFSET_MSDC_DAT_RDDLY1),
		(MSDC_DAT_RDDLY1_D7 | MSDC_DAT_RDDLY1_D6 |
		 MSDC_DAT_RDDLY1_D5 | MSDC_DAT_RDDLY1_D4),
		0x0,    0x0,
		hs400_restore_ddly1},
	{(OFFSET_EMMC50_PAD_CMD_TUNE),
		(MSDC_EMMC50_PAD_CMD_TUNE_TXDLY),
		0x0,    0x00000200,
		hs400_restore_cmd_tune},
	{(OFFSET_EMMC50_PAD_DS_TUNE),
		(MSDC_EMMC50_PAD_DS_TUNE_DLY1 | MSDC_EMMC50_PAD_DS_TUNE_DLY3),
		0x0,    0x0,
		hs400_restore_ds_tune},
	{(OFFSET_EMMC50_PAD_DAT01_TUNE),
		(MSDC_EMMC50_PAD_DAT0_TXDLY | MSDC_EMMC50_PAD_DAT1_TXDLY),
		0x0,    0x01000100,
		hs400_restore_dat01_tune},
	{(OFFSET_EMMC50_PAD_DAT23_TUNE),
		(MSDC_EMMC50_PAD_DAT2_TXDLY | MSDC_EMMC50_PAD_DAT3_TXDLY),
		0x0,    0x01000100,
		hs400_restore_dat23_tune},
	{(OFFSET_EMMC50_PAD_DAT45_TUNE),
		(MSDC_EMMC50_PAD_DAT4_TXDLY | MSDC_EMMC50_PAD_DAT5_TXDLY),
		0x0,    0x01000100,
		hs400_restore_dat45_tune},
	{(OFFSET_EMMC50_PAD_DAT67_TUNE),
		(MSDC_EMMC50_PAD_DAT6_TXDLY | MSDC_EMMC50_PAD_DAT7_TXDLY),
		0x0,    0x01000100,
		hs400_restore_dat67_tune},
};

/*
 *  2013-12-09
 *  when switch from eMMC 4.5 backward speed mode to HS400 speed mode
 *   back up eMMC 4.5 backward speed mode tunning result,
 *   and init them with defalut value for HS400 speed mode
 */
void emmc_hs400_backup(void)
{
	int i = 0, err = 0;
	void __iomem *addr;

	for (i = 0; i < HS400_BACKUP_REG_NUM; i++) {
		addr = hs400_backup_reg_list[i].addr + mtk_msdc_host[0]->base;
		MSDC_GET_FIELD(addr,
			hs400_backup_reg_list[i].mask,
			hs400_backup_reg_list[i].value);
		if (hs400_backup_reg_list[i].restore_func) {
			err = hs400_backup_reg_list[i].restore_func(0);
			if (err) {
				pr_err("[%s]: failed to restore reg[%p][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
					__func__, addr,
					hs400_backup_reg_list[i].mask,
					hs400_backup_reg_list[i].default_value,
					MSDC_READ32(addr), err);
			}
		}
	}
}
/*
 *  2013-12-09
 *  when switch from HS400 speed mode to eMMC 4.5 backward speed mode
 *  do restore the eMMC 4.5 backward speed mode tunning result
 */
void emmc_hs400_restore(void)
{
	int i = 0, err = 0;
	void __iomem *addr;

	if (!mtk_msdc_host[0]) {
		pr_err("[%s] msdc%d is not exist\n", __func__, 0);
		return;
	}

	for (i = 0; i < HS400_BACKUP_REG_NUM; i++) {
		addr = mtk_msdc_host[0]->base+hs400_backup_reg_list[i].addr;
		MSDC_SET_FIELD(addr,
			hs400_backup_reg_list[i].mask,
			hs400_backup_reg_list[i].value);
		if (hs400_backup_reg_list[i].restore_func) {
			err = hs400_backup_reg_list[i].restore_func(1);
			if (err) {
				pr_err("[%s]:failed to restore reg[%p][0x%x], expected value[0x%x], actual value[0x%x] err=0x%x",
					__func__, addr,
					hs400_backup_reg_list[i].mask,
					hs400_backup_reg_list[i].value,
					MSDC_READ32(addr), err);
			}
		}
		pr_debug("[%s]:i:%d, reg=%p, value=0x%x\n", __func__, i, addr,
			MSDC_READ32(addr));
	}
}

/*
   register as callback function of WIFI(combo_sdio_register_pm) .
   can called by msdc_drv_suspend/resume too.
*/
void msdc_set_timing_setting(struct msdc_host *host,
	int emmc_restore, int sdio_restore)
{
	void __iomem *base = host->base;
	struct msdc_saved_para *saved_para = &(host->saved_para);

	/*Light*/
	pr_err("msdc_set_timing_setting %d %d\n", emmc_restore, sdio_restore);
	if (emmc_restore)
		msdc_set_mclk(host, saved_para->timing, host->mclk);

	MSDC_WRITE32(SDC_CFG, saved_para->sdc_cfg);

	if (emmc_restore)
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, host->hw->ddlsel);
	else if (sdio_restore)
		MSDC_WRITE32(MSDC_IOCON, saved_para->iocon);

	if (emmc_restore)
		msdc_set_smpl_all(host,
			(saved_para->timing == MMC_TIMING_MMC_HS400));

	MSDC_WRITE32(MSDC_PAD_TUNE0, saved_para->pad_tune0);
	MSDC_WRITE32(MSDC_DAT_RDDLY0, saved_para->ddly0);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, saved_para->ddly1);
	MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
		saved_para->wrdat_crc_ta_cntr);
	MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
		saved_para->cmd_resp_ta_cntr);

	if (sdio_restore) {
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			saved_para->int_dat_latch_ck_sel);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL,
			saved_para->ckgen_msdc_dly_sel);
		MSDC_SET_FIELD(MSDC_INTEN, MSDC_INT_SDIOIRQ,
			saved_para->inten_sdio_irq);
	}

	if (emmc_restore) {
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			saved_para->ds_dly1);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			saved_para->ds_dly3);
		MSDC_WRITE32(EMMC50_PAD_CMD_TUNE,
			saved_para->emmc50_pad_cmd_tune);

		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA,
			host->saved_para.write_busy_margin);
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA,
			host->saved_para.write_crc_margin);

		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,
			host->saved_para.cfg_crcsts_path);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,
			host->saved_para.cfg_cmdrsp_path);
		MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT,
			host->saved_para.resp_wait_cnt);
	}
}

void msdc_restore_info(struct msdc_host *host)
{
	/* Currently for SDIO only*/
	void __iomem *base = host->base;
	int retry = 3;

	msdc_reset_hw(host->id);
	/* force bit5(BV18SDT) to 0 */

	host->saved_para.msdc_cfg = host->saved_para.msdc_cfg & 0xFFFFFFDF;
	MSDC_WRITE32(MSDC_CFG, host->saved_para.msdc_cfg);

	while (retry--) {
		msdc_set_mclk(host, host->saved_para.timing,
			host->saved_para.hz);
		if ((MSDC_READ32(MSDC_CFG) & 0xFFFFFF9F)
			!= (host->saved_para.msdc_cfg & 0xFFFFFF9F)) {
			ERR_MSG("msdc set_mclk is unstable (cur_cfg=%x, save_cfg=%x, cur_hz=%d, save_hz=%d).",
				MSDC_READ32(MSDC_CFG),
				host->saved_para.msdc_cfg, host->mclk,
				host->saved_para.hz);
		} else {
			break;
		}
	}
	msdc_set_timing_setting(host, 0, 1);
}

/*
 *  2015-01-09
 *  Runtime reducing to legacy speed or slower clock, clear eMMC ett timing
 */
void emmc_clear_timing(void)
{
	int i = 0;

	if (!mtk_msdc_host[0]) {
		pr_err("[%s] msdc%d is not exist\n", __func__, 0);
		return;
	}

	pr_err("emmc_clear_timing msdc0\n");
	for (i = 0; i < HS400_BACKUP_REG_NUM; i++) {
		MSDC_SET_FIELD(
			(hs400_backup_reg_list[i].addr+mtk_msdc_host[0]->base),
			hs400_backup_reg_list[i].mask,
			hs400_backup_reg_list[i].default_value);
	}
}
void msdc_init_tune_setting(struct msdc_host *host, int re_init)
{
	struct msdc_hw *hw = host->hw;
	void __iomem *base = host->base;
	u32 cur_rxdly0, cur_rxdly1;

#ifdef CFG_DEV_MSDC2
	if (host->id == 2)
		MSDC_WRITE32(MSDC_PAD_TUNE0,   0x00000000);
	else
#endif
		MSDC_WRITE32(MSDC_PAD_TUNE0,   0x00008000);

	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY,
		hw->datwrddly);
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY,
		hw->cmdrrddly);
	MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY,
		hw->cmdrddly);

	pr_err("msdc_init_tune_setting\n"); /*Light*/
	MSDC_WRITE32(MSDC_IOCON, 0x00000000);

	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
	cur_rxdly0 = ((hw->dat0rddly & 0x1F) << 24) |
		     ((hw->dat1rddly & 0x1F) << 16) |
		     ((hw->dat2rddly & 0x1F) << 8) |
		     ((hw->dat3rddly & 0x1F) << 0);
	cur_rxdly1 = ((hw->dat4rddly & 0x1F) << 24) |
		     ((hw->dat5rddly & 0x1F) << 16) |
		     ((hw->dat6rddly & 0x1F) << 8) |
		     ((hw->dat7rddly & 0x1F) << 0);

	MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);
	MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1);

	if ((hw->host_function == MSDC_EMMC) ||
	    (hw->host_function == MSDC_SD)) {
		MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFE00C9);
	} else {
		MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFE0009);
	}

	/* disable async fifo use interl delay*/
	MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
	MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP);

	/* 64T + 48T cmd <-> resp */
	MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_RESPWAITCNT, 3);

	if (!(is_card_sdio(host) || (hw->flags & MSDC_SDIO_IRQ))) {
		/* internal clock: latch read data, not apply to sdio */
		hw->cmd_edge  = 0;
		hw->rdata_edge = 0;
		hw->wdata_edge = 0;
	}

	if (!re_init) {
		host->saved_para.ddly0 = cur_rxdly0;
		host->saved_para.ddly1 = cur_rxdly1;

		if (is_card_sdio(host))
			msdc_sdio_set_long_timing_delay_by_freq(host,
				50*1000*1000);
	}

	msdc_save_timing_setting(host, 1, 0, 0, 0, 0);
}

static void msdc_apply_ett_settings(struct msdc_host *host, int mode)
{
	unsigned int i = 0;
	void __iomem *base = host->base;
	struct msdc_ett_settings *ett = NULL, *ett_item = NULL;
	unsigned int ett_count = 0;

	switch (g_emmc_id) {
#ifdef MSDC_SUPPORT_SANDISK_COMBO_ETT
	case SANDISK_EMMC_CHIP:
		pr_err("--- apply sandisk emmc ett settings\n");
		host->hw->ett_hs200_settings =
			msdc0_ett_hs200_settings_for_sandisk;
		host->hw->ett_hs400_settings =
			msdc0_ett_hs400_settings_for_sandisk;
		break;
#endif
#ifdef MSDC_SUPPORT_SAMSUNG_COMBO_ETT
	case SAMSUNG_EMMC_CHIP:
		pr_err("--- apply samsung emmc ett settings\n");
		host->hw->ett_hs200_settings =
			msdc0_ett_hs200_settings_for_samsung;
		host->hw->ett_hs400_settings =
			msdc0_ett_hs400_settings_for_samsung;
		break;
#endif
	default:
		pr_err("--- apply default emmc ett settings\n");
		break;
	}

	if (MSDC_HS200_MODE == mode) {
		ett_count = host->hw->ett_hs200_count;
		ett = host->hw->ett_hs200_settings;
		pr_err("[MSDC, %s] hs200 ett, ett_count=%d\n", __func__,
				host->hw->ett_hs200_count);
	} else if (MSDC_HS400_MODE == mode) {
		/* clear hs200 setting */
		ett_count = host->hw->ett_hs200_count;
		ett = host->hw->ett_hs200_settings;
		for (i = 0; i < ett_count; i++) {
			ett_item = (struct msdc_ett_settings *)(ett + i);
			MSDC_SET_FIELD((base + ett_item->reg_addr), ett_item->reg_offset, 0);
		}
		ett_count = host->hw->ett_hs400_count;
		ett = host->hw->ett_hs400_settings;
		pr_err("[MSDC, %s] hs400 ett, ett_count=%d\n", __func__,
				host->hw->ett_hs400_count);
	}
	for (i = 0; i < ett_count; i++) {
		ett_item = (struct msdc_ett_settings *)(ett + i);
		MSDC_SET_FIELD((base + ett_item->reg_addr),
				ett_item->reg_offset, ett_item->value);
		pr_err("%s:msdc%d,reg[0x%x],offset[0x%x],val[0x%x],readback[0x%x]\n",
				__func__, host->id,
				ett_item->reg_addr,
				ett_item->reg_offset,
				ett_item->value,
				MSDC_READ32(base + ett_item->reg_addr));
	}
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
		host->saved_para.cmd_resp_ta_cntr);
	host->saved_para.pad_tune0 = MSDC_READ32(MSDC_PAD_TUNE0);
	host->saved_para.ddly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
	host->saved_para.ddly1 = MSDC_READ32(MSDC_DAT_RDDLY1);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
		host->saved_para.wrdat_crc_ta_cntr);
	if ((host->id == 0) && (mode == MSDC_HS400_MODE)) {
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1,
			host->saved_para.ds_dly1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3,
			host->saved_para.ds_dly3);
	}
}

int msdc_tune_cmdrsp(struct msdc_host *host)
{
	int result = 0;
	void __iomem *base = host->base;
	u32 sel = 0;
	u32 cur_rsmpl = 0, orig_rsmpl;
	u32 cur_rrdly = 0, orig_rrdly;
	u32 cur_cntr = 0, orig_cmdrtc;
	u32 cur_dl_cksel = 0, orig_dl_cksel;
	u32 clkmode;
	int hs400 = 0;

	if (!host->error_tune_enable)
		return 1;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, orig_rrdly);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,
		orig_cmdrtc);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
		orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, clkmode);
	hs400 = (clkmode == 3) ? 1 : 0;

#if 1
	if (host->mclk >= 100000000) {
		sel = 1;
	} else {
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, 1);
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
			0);
	}

	cur_rsmpl = (orig_rsmpl + 1);
	msdc_set_smpl(host, hs400, cur_rsmpl % 2, TYPE_CMD_RESP_EDGE, NULL);
	if (host->mclk <= 400000) {
		msdc_set_smpl(host, hs400, 0, TYPE_CMD_RESP_EDGE, NULL);
		cur_rsmpl = 2;
	}
	if (cur_rsmpl >= 2) {
		cur_rrdly = (orig_rrdly + 1);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY,
			cur_rrdly & 0x1F);
	}
	if (cur_rrdly >= 32) {
		if (sel) {
			cur_cntr = (orig_cmdrtc + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT1,
				MSDC_PB1_CMD_RSP_TA_CNTR, cur_cntr & 0x7);
		}
	}
	if (cur_cntr >= 8) {
		if (sel) {
			cur_dl_cksel = (orig_dl_cksel + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0,
				MSDC_PB0_INT_DAT_LATCH_CK_SEL,
				cur_dl_cksel & 0x7);
		}
	}
	++(host->t_counter.time_cmd);
	if ((sel &&
	     host->t_counter.time_cmd == CMD_TUNE_UHS_MAX_TIME)
	 || (sel == 0 &&
	     host->t_counter.time_cmd == CMD_TUNE_HS_MAX_TIME)) {
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_cmd = 0;
	}
#else
	if (orig_rsmpl == 0) {
		cur_rsmpl = 1;
		msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE, NULL);
	} else {
		cur_rsmpl = 0;
			/* need second layer */
		msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE, NULL);
		cur_rrdly = (orig_rrdly + 1);
		if (cur_rrdly >= 32) {
			ERR_MSG("failed to update rrdly<%d>", cur_rrdly);
			MSDC_SET_FIELD(MSDC_PAD_TUNE0,
				MSDC_PAD_TUNE0_CMDRDLY, 0);
#ifdef MSDC_LOWER_FREQ
			return msdc_lower_freq(host);
#else
			return 1;
#endif
		}
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY,
			cur_rrdly);
	}

#endif
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, orig_rrdly);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, orig_cmdrtc);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
		orig_dl_cksel);
	INIT_MSG("TUNE_CMD: rsmpl<%d> rrdly<%d> cmdrtc<%d> dl_cksel<%d> sfreq.<%d>", orig_rsmpl,
		orig_rrdly, orig_cmdrtc, orig_dl_cksel, host->sclk);

	return result;
}

int msdc_tune_read(struct msdc_host *host)
{
	void __iomem *base = host->base;
	u32 sel = 0;
	u32 ddr = 0, hs400 = 0;
	u32 dcrc;
	u32 clkmode = 0;
	u32 cur_rxdly0, cur_rxdly1;
	u32 cur_dsmpl = 0, orig_dsmpl;
	u32 cur_dsel = 0, orig_dsel;
	u32 cur_dl_cksel = 0, orig_dl_cksel;
	u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0;
	u32 cur_dat4 = 0, cur_dat5 = 0, cur_dat6 = 0, cur_dat7 = 0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	u32 orig_dat4, orig_dat5, orig_dat6, orig_dat7;
	int result = 0;

	if (!host->error_tune_enable)
		return 1;

#if 1
	if (host->mclk >= 100000000)
		sel = 1;
	else
		MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, 0);

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, clkmode);
	ddr = (clkmode == 2) ? 1 : 0;
	hs400 = (clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
		orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);

	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
	cur_dsmpl = (orig_dsmpl + 1);
	msdc_set_smpl(host, hs400, cur_dsmpl % 2, TYPE_READ_DATA_EDGE, NULL);

	if (cur_dsmpl >= 2) {
		MSDC_GET_FIELD(SDC_DCRC_STS,
			SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);

		if (!ddr)
			dcrc &= ~SDC_DCRC_STS_NEG;

		cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
		cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);

		orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
		orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
		orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
		orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;
		orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
		orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
		orig_dat6 = (cur_rxdly1 >> 8) & 0x1F;
		orig_dat7 = (cur_rxdly1 >> 0) & 0x1F;

		if (ddr) {
			/* Combine pos edge and neg edge error into
			   bits 7~0 of dcrc */
			dcrc = dcrc | (dcrc>>8);
		}

		cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
		cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
		cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
		cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
		cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
		cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
		cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
		cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;

		cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) |
			(cur_dat2 << 8) | (cur_dat3 << 0);
		cur_rxdly1 = (cur_dat4 << 24) | (cur_dat5 << 16) |
			(cur_dat6 << 8) | (cur_dat7 << 0);


		MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0 & 0x1F1F1F1F);
		MSDC_WRITE32(MSDC_DAT_RDDLY1, cur_rxdly1 & 0x1F1F1F1F);

	}
	if ((cur_dat0 >= 32) || (cur_dat1 >= 32) || (cur_dat2 >= 32) ||
	     (cur_dat3 >= 32) || (cur_dat4 >= 32) || (cur_dat5 >= 32) ||
	     (cur_dat6 >= 32) || (cur_dat7 >= 32)) {
		if (sel) {
			MSDC_WRITE32(MSDC_DAT_RDDLY0, 0);
			MSDC_WRITE32(MSDC_DAT_RDDLY1, 0);
			cur_dsel = (orig_dsel + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0,
				MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_dsel % 32);
		}
	}
	if (cur_dsel >= 32) {
		if (clkmode == 1 && sel) {
			cur_dl_cksel = (orig_dl_cksel + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT0,
				MSDC_PB0_INT_DAT_LATCH_CK_SEL,
				cur_dl_cksel % 8);
		}
	}
	++(host->t_counter.time_read);
	if ((sel == 1 && clkmode == 1 &&
	     host->t_counter.time_read == READ_TUNE_UHS_CLKMOD1_MAX_TIME)
	 || (sel == 1 && (clkmode == 0 || clkmode == 2) &&
	     host->t_counter.time_read == READ_TUNE_UHS_MAX_TIME)
	 || (sel == 0 && (clkmode == 0 || clkmode == 2) &&
	     host->t_counter.time_read == READ_TUNE_HS_MAX_TIME)) {
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_read = 0;
	}
#endif

	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_dsel);
	MSDC_GET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,
		orig_dl_cksel);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
	cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
	cur_rxdly1 = MSDC_READ32(MSDC_DAT_RDDLY1);
	INIT_MSG("TUNE_READ: dsmpl<%d> rxdly0<0x%x> rxdly1<0x%x> dsel<%d> dl_cksel<%d> sfreq.<%d>",
		orig_dsmpl, cur_rxdly0, cur_rxdly1, orig_dsel, orig_dl_cksel,
		host->sclk);

	return result;
}

int msdc_tune_write(struct msdc_host *host)
{
	void __iomem *base = host->base;
	/* u32 cur_wrrdly = 0, orig_wrrdly; */
	u32 cur_dsmpl = 0, orig_dsmpl;
	u32 cur_rxdly0 = 0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0;
	u32 cur_d_cntr = 0, orig_d_cntr;
	int result = 0;

	int sel = 0;
	int clkmode = 0;
	int hs400 = 0;

	if (!host->error_tune_enable)
		return 1;
#if 1
	if (host->mclk >= 100000000)
		sel = 1;
	else
		MSDC_SET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, 1);

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, clkmode);
	/*MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY,
		orig_wrrdly);*/
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
		orig_d_cntr);
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	cur_dsmpl = (orig_dsmpl + 1);
	hs400 = (clkmode == 3) ? 1 : 0;
	msdc_set_smpl(host, hs400, cur_dsmpl % 2, TYPE_WRITE_CRC_EDGE, NULL);
#if 0
	if (cur_dsmpl >= 2) {
		cur_wrrdly = (orig_wrrdly + 1);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY,
			cur_wrrdly & 0x1F);
	}
#endif
	if (cur_dsmpl >= 2) {
		cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);

		orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
		orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
		orig_dat2 = (cur_rxdly0 >> 8) & 0x1F;
		orig_dat3 = (cur_rxdly0 >> 0) & 0x1F;

		cur_dat0 = (orig_dat0 + 1);     /* only adjust bit-1 for crc */
		cur_dat1 = orig_dat1;
		cur_dat2 = orig_dat2;
		cur_dat3 = orig_dat3;

		cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) |
			(cur_dat2 << 8) | (cur_dat3 << 0);

		MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0 & 0x1F1F1F1F);
	}
	if (cur_dat0 >= 32) {
		if (sel) {
			cur_d_cntr = (orig_d_cntr + 1);
			MSDC_SET_FIELD(MSDC_PATCH_BIT1,
				MSDC_PB1_WRDAT_CRCS_TA_CNTR, cur_d_cntr & 0x7);
		}
	}
	++(host->t_counter.time_write);
	if ((sel == 0 &&
	     host->t_counter.time_write == WRITE_TUNE_HS_MAX_TIME)
	 || (sel &&
	     host->t_counter.time_write == WRITE_TUNE_UHS_MAX_TIME)) {
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_write = 0;
	}
#endif
	/*
	MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, orig_wrrdly);
	*/
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
	MSDC_GET_FIELD(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,
		orig_d_cntr);
	cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
	INIT_MSG("TUNE_WRITE: dsmpl<%d> rxdly0<0x%x> d_cntr<%d> sfreq.<%d>",
		orig_dsmpl, cur_rxdly0, orig_d_cntr, host->sclk);

	return result;
}

void msdc_save_timing_as_0(struct msdc_host *host)
{
	struct msdc_saved_para *saved_para = &(host->saved_para);

	/* FIX ME: check if memset all 0 is OK */
	saved_para->suspend_flag = 0;
	saved_para->msdc_cfg = 0;
	saved_para->mode = 0;
	saved_para->div = 0;
	saved_para->sdc_cfg = 0;
	saved_para->iocon = 0;
	saved_para->timing = 0;
	saved_para->hz = 0;
	saved_para->cmd_resp_ta_cntr = 0;
	saved_para->wrdat_crc_ta_cntr = 0;
	saved_para->int_dat_latch_ck_sel = 0;
	saved_para->ckgen_msdc_dly_sel = 0;
	saved_para->inten_sdio_irq = 0;    /* default disable */
	saved_para->cfg_cmdrsp_path = 0;
	saved_para->cfg_crcsts_path = 0;
}

/*For msdc_ios_set_ios()*/
void msdc_ios_tune_setting(struct msdc_host *host,
	struct mmc_ios *ios)
{

	if (host->id == 0) {
		if (ios->timing == MMC_TIMING_MMC_HS200) {
			msdc_apply_ett_settings(host, MSDC_HS200_MODE);
		} else if (ios->timing == MMC_TIMING_MMC_HS400) {
			/* switch from eMMC 4.5 backward speed mode to HS400 */
			emmc_hs400_backup();
			msdc_apply_ett_settings(host, MSDC_HS400_MODE);
		}
		/* switch from HS400 to eMMC 4.5 backward speed mode */
		if (host->timing == MMC_TIMING_MMC_HS400)
			emmc_hs400_restore();
	}
}

static u32 msdc_status_verify_case1(struct msdc_host *host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;
	unsigned long tmo = jiffies + POLLING_BUSY;
	void __iomem *base = host->base;

	while (state != 4) { /* until status to "tran" */
		msdc_reset_hw(host->id);
		while ((err = msdc_get_card_status(mmc, host, &status))) {
			ERR_MSG("CMD13 ERR<%d>", err);
			if (err != (unsigned int)-EIO) {
				return msdc_power_tuning(host);
			} else if (msdc_tune_cmdrsp(host)) {
				ERR_MSG("update cmd para failed");
				return MSDC_VERIFY_ERROR;
			}
		}

		state = R1_CURRENT_STATE(status);
		ERR_MSG("check card state<%d>", state);
		if (state == 5 || state == 6) {
			ERR_MSG("state<%d> need cmd12 to stop", state);
			msdc_send_stop(host); /* don't tuning */
		} else if (state == 7) { /* busy in programing */
			ERR_MSG("state<%d> card is busy", state);
			spin_unlock(&host->lock);
			msleep(100);
			spin_lock(&host->lock);
		} else if (state != 4) {
			ERR_MSG("state<%d> ??? ", state);
			return msdc_power_tuning(host);
		}

		if (time_after(jiffies, tmo)) {
			ERR_MSG("abort timeout. Do power cycle");
			if ((host->hw->host_function == MSDC_SD)
				&& (host->sclk >= 100000000
				|| (host->timing == MMC_TIMING_UHS_DDR50)))
				host->sd_30_busy++;
			return msdc_power_tuning(host);
		}
	}

	msdc_reset_hw(host->id);
	return MSDC_VERIFY_NEED_TUNE;
}

static u32 msdc_status_verify_case2(struct msdc_host *host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;/*0: can tune normaly; 1: err hapen; 2: tune pass;*/
	struct mmc_card *card = host->mmc->card;
	void __iomem *base = host->base;

	while (1) {
		msdc_reset_hw(host->id);
		err = msdc_get_card_status(mmc, host, &status);
		if (!err) {
			break;
		} else if (err != (unsigned int)-EIO) {
			ERR_MSG("CMD13 ERR<%d>", err);
			return msdc_power_tuning(host);
		} else if (msdc_tune_cmdrsp(host)) {
			ERR_MSG("update cmd para failed");
			return MSDC_VERIFY_ERROR;
		}
		ERR_MSG("CMD13 ERR<%d>", err);
	}
	state = R1_CURRENT_STATE(status);

	/*wether is right RCA*/
	if (cmd->arg == card->rca << 16) {
		return (3 == state || 8 == state) ? MSDC_VERIFY_NEED_TUNE :
			MSDC_VERIFY_NEED_NOT_TUNE;
	} else {
		return (4 == state || 5 == state || 6 == state || 7 == state)
			? MSDC_VERIFY_NEED_TUNE : MSDC_VERIFY_NEED_NOT_TUNE;
	}
}

static u32 msdc_status_verify_case3(struct msdc_host *host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;/*0: can tune normaly; 1: tune pass;*/
	void __iomem *base = host->base;

	while (1) {
		msdc_reset_hw(host->id);
		err = msdc_get_card_status(mmc, host, &status);
		if (!err) {
			break;
		} else if (err != (unsigned int)-EIO) {
			ERR_MSG("CMD13 ERR<%d>", err);
			return msdc_power_tuning(host);
		} else if (msdc_tune_cmdrsp(host)) {
			ERR_MSG("update cmd para failed");
			return MSDC_VERIFY_ERROR;
		}
		ERR_MSG("CMD13 ERR<%d>", err);
	}
	state = R1_CURRENT_STATE(status);
	return (5 == state || 6 == state) ? MSDC_VERIFY_NEED_TUNE :
		MSDC_VERIFY_NEED_NOT_TUNE;
}

static u32 msdc_status_verify_case4(struct msdc_host *host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;/*0: can tune normaly; 1: tune pass;*/
	void __iomem *base = host->base;

	if (cmd->arg && (0x1UL << 15))
		return MSDC_VERIFY_NEED_NOT_TUNE;
	while (1) {
		msdc_reset_hw(host->id);
		err = msdc_get_card_status(mmc, host, &status);
		if (!err) {
			break;
		} else if (err != (unsigned int)-EIO) {
			ERR_MSG("CMD13 ERR<%d>", err);
			break;
		} else if (msdc_tune_cmdrsp(host)) {
			ERR_MSG("update cmd para failed");
			return MSDC_VERIFY_ERROR;
		}
		ERR_MSG("CMD13 ERR<%d>", err);
	}
	state = R1_CURRENT_STATE(status);
	return 3 == state ? MSDC_VERIFY_NEED_NOT_TUNE :
		MSDC_VERIFY_NEED_TUNE;
}

#if 0
static u32 msdc_status_verify_case5(struct msdc_host *host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;/*0: can tune normaly; 1: tune pass;*/
	struct mmc_card *card = host->mmc->card;
	struct mmc_command cmd_bus_test = { 0 };
	struct mmc_request mrq_bus_sest = { 0 };

	while ((err = msdc_get_card_status(mmc, host, &status))) {
		ERR_MSG("CMD13 ERR<%d>", err);
		if (err != (unsigned int)-EIO) {
			return msdc_power_tuning(host);
		} else if (msdc_tune_cmdrsp(host)) {
			ERR_MSG("update cmd para failed");
			return MSDC_VERIFY_ERROR;
		}
	}
	state = R1_CURRENT_STATE(status);

	if (MMC_SEND_TUNING_BLOCK == cmd->opcode) {
		if (state == 9) {
			/*send cmd14*/
			/*u32 err = -1;*/
			cmd_bus_test.opcode = MMC_BUS_TEST_R;
			cmd_bus_test.arg = 0;
			cmd_bus_test.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

			mrq_bus_sest.cmd = &cmd_bus_test;
			cmd_bus_test.mrq = &mrq_bus_sest;
			cmd_bus_test.data = NULL;
			msdc_do_command(host, &cmd_bus_test, 0, CMD_TIMEOUT);
		}
		return MSDC_VERIFY_NEED_TUNE;
	}
	if (state == 4) {
		/*send cmd19*/
		/*u32 err = -1;*/
		cmd_bus_test.opcode = MMC_BUS_TEST_W;
		cmd_bus_test.arg = 0;
		cmd_bus_test.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

		mrq_bus_sest.cmd = &cmd_bus_test;
		cmd_bus_test.mrq = &mrq_bus_sest;
		cmd_bus_test.data = NULL;
		msdc_do_command(host, &cmd_bus_test, 0, CMD_TIMEOUT);
	}
	return MSDC_VERIFY_NEED_TUNE;
}
#endif

static u32 msdc_status_verify(struct msdc_host *host, struct mmc_command *cmd)
{
	if (!host->mmc || !host->mmc->card || !host->mmc->card->rca) {
		/*card is not identify*/
		return MSDC_VERIFY_NEED_TUNE;
	}

	if (((host->hw->host_function == MSDC_EMMC) &&
	      IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_MMC_TUNE_CASE1))
	 || ((host->hw->host_function == MSDC_SD) &&
	      IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_SD_TUNE_CASE1))
	 || (host->app_cmd &&
	      IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_APP_TUNE_CASE1))) {
		return msdc_status_verify_case1(host, cmd);
	} else if (IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_MMC_TUNE_CASE2)) {
		return msdc_status_verify_case2(host, cmd);
	} else if (IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_MMC_TUNE_CASE3)) {
		return msdc_status_verify_case3(host, cmd);
	} else if ((host->hw->host_function == MSDC_EMMC)
		&& IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_MMC_TUNE_CASE4)) {
		return msdc_status_verify_case4(host, cmd);
#if 0
	} else if ((host->hw->host_function == MSDC_EMMC)
		&& IS_IN_CMD_SET(cmd->opcode, CMD_SET_FOR_MMC_TUNE_CASE5)) {
		return msdc_status_verify_case5(host, cmd);
#endif
	} else {
		return MSDC_VERIFY_NEED_TUNE;
	}

}

int msdc_crc_tune(struct msdc_host *host, struct mmc_command *cmd,
	struct mmc_data *data, struct mmc_command *stop,
	struct mmc_command *sbc)
{
	u32 status_verify = 0;

#ifdef MTK_MSDC_USE_CMD23
	/* cmd->error also set when autocmd23 crc error */
	if ((cmd->error == (unsigned int)-EIO)
	 || (stop && (stop->error == (unsigned int)-EIO))
	 || (sbc && (sbc->error == (unsigned int)-EIO))) {
#else
	if ((cmd->error == (unsigned int)-EIO)
	 || (stop && (stop->error == (unsigned int)-EIO))) {
#endif
		if (msdc_tune_cmdrsp(host)) {
			ERR_MSG("failed to updata cmd para");
			return 1;
		}
	}

	if (data && (data->error == (unsigned int)-EIO)) {
		if ((host->id == 0) &&
		    (host->timing == MMC_TIMING_MMC_HS400)) {
			if (emmc_hs400_tune_rw(host)) {
				ERR_MSG("failed to updata write para");
				return 1;
			}
		} else if (data->flags & MMC_DATA_READ) {      /* read */
			if (msdc_tune_read(host)) {
				ERR_MSG("failed to updata read para");
				return 1;
			}
		} else {
			if (msdc_tune_write(host)) {
				ERR_MSG("failed to updata write para");
				return 1;
			}
		}
	}

	status_verify = msdc_status_verify(host, cmd);
	if (MSDC_VERIFY_ERROR == status_verify) {
		ERR_MSG("status verify failed");
		/*data_abort = 1;*/
		if (host->hw->host_function == MSDC_SD) {
			if (host->error_tune_enable) {
				ERR_MSG("disable error tune flow of bad SD card");
				host->error_tune_enable = 0;
			}
			return 1;
		}
	} else if (MSDC_VERIFY_NEED_NOT_TUNE == status_verify) {
		/* clear the error condition. */
		ERR_MSG("need not error tune");
		cmd->error = 0;
		return 1;
	}

	return 0;
}

int msdc_cmd_timeout_tune(struct msdc_host *host, struct mmc_command *cmd)
{
	/* CMD TO -> not tuning.
	   cmd->error also set when autocmd23 TO error */
	if (cmd->error != (unsigned int)-ETIMEDOUT)
		return 0;

	if (check_mmc_cmd1718(cmd->opcode) ||
	    check_mmc_cmd2425(cmd->opcode)) {
		if ((host->sw_timeout) ||
		    (++(host->rwcmd_time_tune) > MSDC_MAX_TIMEOUT_RETRY)) {
			ERR_MSG("msdc%d exceed max r/w cmd timeout tune times(%d) or SW timeout(%d),Power cycle",
				host->id, host->rwcmd_time_tune,
				host->sw_timeout);
			if (!(host->sd_30_busy) && msdc_power_tuning(host))
				return 1;
		}
	} else {
		return 1;
	}

	return 0;
}

int msdc_data_timeout_tune(struct msdc_host *host, struct mmc_data *data)
{
	if (!data || (data->error != (unsigned int)-ETIMEDOUT))
		return 0;

	/* [ALPS114710] Patch for data timeout issue. */
	if (data->flags & MMC_DATA_READ) {
		if (!(host->sw_timeout) &&
		    (host->hw->host_function == MSDC_SD) &&
		    (host->sclk > 100000000) &&
		    (host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE)) {
			if (host->t_counter.time_read)
				host->t_counter.time_read--;
			host->read_timeout_uhs104++;
			msdc_tune_read(host);
		} else if ((host->sw_timeout) ||
			   (host->read_timeout_uhs104
				>= MSDC_MAX_R_TIMEOUT_TUNE) ||
			   (++(host->read_time_tune)
				> MSDC_MAX_TIMEOUT_RETRY)) {
			ERR_MSG("msdc%d exceed max read timeout retry times(%d)",
				host->id, host->read_time_tune);
			ERR_MSG("SW timeout(%d) or read timeout tuning times(%d), Power cycle",
				host->sw_timeout, host->read_timeout_uhs104);
			if (msdc_power_tuning(host))
				return 1;
		}
	} else if (data->flags & MMC_DATA_WRITE) {
		if (!(host->sw_timeout)
		 && (host->hw->host_function == MSDC_SD)
		 && (host->sclk > 100000000)
		 && (host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE)) {
			if (host->t_counter.time_write)
				host->t_counter.time_write--;
			host->write_timeout_uhs104++;
			msdc_tune_write(host);
		} else if (!(host->sw_timeout) &&
			   (host->hw->host_function == MSDC_EMMC) &&
			   (host->write_timeout_emmc
				< MSDC_MAX_W_TIMEOUT_TUNE_EMMC)) {
			if (host->t_counter.time_write)
				host->t_counter.time_write--;
			host->write_timeout_emmc++;

			if ((host->id == 0) &&
			    (host->timing == MMC_TIMING_MMC_HS400))
				emmc_hs400_tune_rw(host);
			else
				msdc_tune_write(host);
		} else if ((host->hw->host_function == MSDC_SD)
			&& ((host->sw_timeout) ||
			    (host->write_timeout_uhs104
				>= MSDC_MAX_W_TIMEOUT_TUNE) ||
			    (++(host->write_time_tune)
				> MSDC_MAX_TIMEOUT_RETRY))) {
			ERR_MSG("msdc%d exceed max read timeout retry times(%d)",
				host->id, host->read_time_tune);
			ERR_MSG("SW timeout(%d) or read timeout tuning times(%d), Power cycle",
				host->sw_timeout, host->write_timeout_uhs104);
			if (!(host->sd_30_busy) && msdc_power_tuning(host))
				return 1;
		} else if ((host->hw->host_function == MSDC_EMMC)
			&& ((host->sw_timeout) ||
			    (++(host->write_time_tune)
				> MSDC_MAX_TIMEOUT_RETRY_EMMC))) {
			ERR_MSG("msdc%d exceed max read timeout retry times(%d)",
				host->id, host->read_time_tune);
			ERR_MSG("SW timeout(%d) or read timeout tuning times(%d), Power cycle",
				host->sw_timeout, host->write_timeout_emmc);
			host->write_timeout_emmc = 0;
			return 1;
		}
	}

	return 0;
}
