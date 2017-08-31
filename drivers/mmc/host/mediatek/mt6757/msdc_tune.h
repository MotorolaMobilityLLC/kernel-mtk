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
/* FIX ME: move it to another file */
extern int g_reset_tune;

/* FIX ME: check if it can be removed since it is set
 * but referenced
 */
extern u32 sdio_tune_flag;

/* sdio function */
void msdc_init_tune_setting(struct msdc_host *host);
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios);
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing);
void emmc_clear_timing(void);
void msdc_sdio_restore_after_resume(struct msdc_host *host);
void msdc_save_timing_setting(struct msdc_host *host, int save_mode);
void msdc_restore_timing_setting(struct msdc_host *host);

void msdc_set_bad_card_and_remove(struct msdc_host *host);

#endif /* end of _MSDC_TUNE_H_ */
