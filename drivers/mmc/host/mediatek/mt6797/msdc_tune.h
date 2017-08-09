#ifndef _MSDC_TUNE_H_
#define _MSDC_TUNE_H_

/* Declared in msdc_tune.c */
/* FIX ME: move it to another file */
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

void msdc_init_tune_setting(struct msdc_host *host, int re_init);
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios);
void msdc_sdio_set_long_timing_delay_by_freq(struct msdc_host *host, u32 clock);

void msdc_reset_pwr_cycle_counter(struct msdc_host *host);
void msdc_reset_tmo_tune_counter(struct msdc_host *host,
	unsigned int index);
void msdc_reset_crc_tune_counter(struct msdc_host *host,
	unsigned int index);
u32 msdc_power_tuning(struct msdc_host *host);
int msdc_tune_cmdrsp(struct msdc_host *host);
int hs400_restore_pb1(int restore);
int hs400_restore_ddly0(int restore);
int hs400_restore_ddly1(int restore);
int hs400_restore_cmd_tune(int restore);
int hs400_restore_dat01_tune(int restore);
int hs400_restore_dat23_tune(int restore);
int hs400_restore_dat45_tune(int restore);
int hs400_restore_dat67_tune(int restore);
int hs400_restore_ds_tune(int restore);
void emmc_hs400_backup(void);
void emmc_hs400_restore(void);
int emmc_hs400_tune_rw(struct msdc_host *host);
int msdc_tune_read(struct msdc_host *host);
int msdc_tune_write(struct msdc_host *host);
int msdc_crc_tune(struct msdc_host *host, struct mmc_command *cmd,
	struct mmc_data *data, struct mmc_command *stop,
	struct mmc_command *sbc);
int msdc_cmd_timeout_tune(struct msdc_host *host, struct mmc_command *cmd);
int msdc_data_timeout_tune(struct msdc_host *host, struct mmc_data *data);

void emmc_clear_timing(void);
void msdc_save_timing_as_0(struct msdc_host *host);
void msdc_save_timing_setting(struct msdc_host *host, u32 init_hw,
	u32 emmc_suspend, u32 sdio_suspend, u32 power_tuning, u32 power_off);
void msdc_set_timing_setting(struct msdc_host *host,
	int emmc_restore, int sdio_restore);
void msdc_restore_info(struct msdc_host *host);

enum EMMC_CHIP_TAG {
	SAMSUNG_EMMC_CHIP = 0x15,
	SANDISK_EMMC_CHIP = 0x45,
	HYNIX_EMMC_CHIP = 0x90,
};

#define MSDC0_ETT_COUNTS 20

/*#define MSDC_SUPPORT_SANDISK_COMBO_ETT*/
/*#define MSDC_SUPPORT_SAMSUNG_COMBO_ETT*/

#include "board.h"
extern struct msdc_ett_settings msdc0_ett_settings[MSDC0_ETT_COUNTS];

#ifdef MSDC_SUPPORT_SANDISK_COMBO_ETT
extern struct msdc_ett_settings
	msdc0_ett_settings_for_sandisk[MSDC0_ETT_COUNTS];
#endif

#ifdef MSDC_SUPPORT_SAMSUNG_COMBO_ETT
extern struct msdc_ett_settings
	msdc0_ett_settings_for_samsung[MSDC0_ETT_COUNTS];
#endif

int msdc_setting_parameter(struct msdc_hw *hw, unsigned int *para);

#endif /* end of _MSDC_TUNE_H_ */
