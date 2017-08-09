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

#ifndef _MSDC_TUNE_H_
#define _MSDC_TUNE_H_

#define EMMC_MAX_FREQ_DIV               4 /* lower frequence to 12.5M */
#define MSDC_CLKTXDLY                   0

#define MSDC0_DDR50_DDRCKD              1

/* Declared in msdc_tune.c */
/* FIXME: move it to another file */
extern int g_ett_tune;
extern int g_reset_tune;

/* Declared in msdc_tune.c */
/* FIX ME: move it to another file */
extern u32 sdio_enable_tune;
extern u32 sdio_iocon_dspl;
extern u32 sdio_iocon_w_dspl;
extern u32 sdio_iocon_rspl;
extern u32 sdio_pad_tune_rrdly;
extern u32 sdio_pad_tune_rdly;
extern u32 sdio_pad_tune_wrdly;
extern u32 sdio_dat_rd_dly0_0;
extern u32 sdio_dat_rd_dly0_1;
extern u32 sdio_dat_rd_dly0_2;
extern u32 sdio_dat_rd_dly0_3;
extern u32 sdio_dat_rd_dly1_0;
extern u32 sdio_dat_rd_dly1_1;
extern u32 sdio_dat_rd_dly1_2;
extern u32 sdio_dat_rd_dly1_3;
extern u32 sdio_clk_drv;
extern u32 sdio_cmd_drv;
extern u32 sdio_data_drv;
extern u32 sdio_tune_flag;

/* sdio function */
void msdc_sdio_set_long_timing_delay_by_freq(struct msdc_host *host, u32 clock);
void msdc_init_tune_setting(struct msdc_host *host);
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios);
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing);
void emmc_clear_timing(void);
void msdc_save_timing_setting(struct msdc_host *host, u32 init_hw,
	u32 emmc_suspend, u32 sdio_suspend, u32 power_tuning, u32 power_off);
void msdc_restore_timing_setting(struct msdc_host *host);

#endif /* end of _MSDC_TUNE_H_ */
