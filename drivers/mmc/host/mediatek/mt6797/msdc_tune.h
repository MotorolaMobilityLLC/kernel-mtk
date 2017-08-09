#ifndef _MSDC_TUNE_H_
#define _MSDC_TUNE_H_

#define MSDC_CLKTXDLY                   0
#define MSDC0_HS400_CLKTXDLY            0
#define MSDC0_HS400_CMDTXDLY            0xA
#define MSDC0_HS400_DAT0TXDLY           0
#define MSDC0_HS400_DAT1TXDLY           0
#define MSDC0_HS400_DAT2TXDLY           0
#define MSDC0_HS400_DAT3TXDLY           0
#define MSDC0_HS400_DAT4TXDLY           0
#define MSDC0_HS400_DAT5TXDLY           0
#define MSDC0_HS400_DAT6TXDLY           0
#define MSDC0_HS400_DAT7TXDLY           0
#define MSDC0_HS400_TXSKEW              1

#define MSDC0_DDR50_DDRCKD              1

#define MSDC0_CLKTXDLY                  0
#define MSDC0_CMDTXDLY                  0
#define MSDC0_DAT0TXDLY                 0
#define MSDC0_DAT1TXDLY                 0
#define MSDC0_DAT2TXDLY                 0
#define MSDC0_DAT3TXDLY                 0
#define MSDC0_DAT4TXDLY                 0
#define MSDC0_DAT5TXDLY                 0
#define MSDC0_DAT6TXDLY                 0
#define MSDC0_DAT7TXDLY                 0
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

void msdc_init_tune_setting(struct msdc_host *host);
void msdc_ios_tune_setting(struct msdc_host *host, struct mmc_ios *ios);
void msdc_sdio_set_long_timing_delay_by_freq(struct msdc_host *host, u32 clock);
void msdc_init_tune_path(struct msdc_host *host, unsigned char timing);

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
void msdc_restore_timing_setting(struct msdc_host *host);

unsigned int msdc_tuning_smpl(struct msdc_host *host);
void msdc_restore_info(struct msdc_host *host);
int msdc_setting_parameter(struct msdc_hw *hw, unsigned int *para);

#endif /* end of _MSDC_TUNE_H_ */
