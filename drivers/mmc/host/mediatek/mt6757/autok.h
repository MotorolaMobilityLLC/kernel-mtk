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

#ifndef _AUTOK_H_
#define _AUTOK_H_

#include <linux/completion.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/scatterlist.h>
#include <mt-plat/mtk_chip.h>

#define AUTOK_VERSION (0x18012613)
struct msdc_host;

#define E_RESULT_PASS (0)
#define E_RESULT_CMD_TMO (1 << 0)
#define E_RESULT_RSP_CRC (1 << 1)
#define E_RESULT_DAT_CRC (1 << 2)
#define E_RESULT_DAT_TMO (1 << 3)
#define E_RESULT_W_CRC (1 << 4)
#define E_RESULT_ERR (1 << 5)
#define E_RESULT_START (1 << 6)
#define E_RESULT_PW_SMALL (1 << 7)
#define E_RESULT_KEEP_OLD (1 << 8)
#define E_RESULT_CMP_ERR (1 << 9)
#define E_RESULT_FATAL_ERR (1 << 10)

#define E_RESULT_MAX

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE (0 == 0)
#endif
#ifndef FALSE
#define FALSE (0 != 0)
#endif

#define AUTOK_DBG_OFF 0
#define AUTOK_DBG_ERROR 1
#define AUTOK_DBG_RES 2
#define AUTOK_DBG_WARN 3
#define AUTOK_DBG_TRACE 4
#define AUTOK_DBG_LOUD 5

extern unsigned int autok_debug_level;

#define AUTOK_DBGPRINT(_level, _fmt...)                                        \
	({                                                                     \
		if (autok_debug_level >= _level) {                             \
			pr_notice(_fmt);                                       \
		}                                                              \
	})

#define AUTOK_RAWPRINT(_fmt...) ({ pr_notice(_fmt); })

enum ERROR_TYPE {
	CMD_ERROR = 0,
	DATA_ERROR,
	CRC_STATUS_ERROR,
};

enum AUTOK_PARAM {
	/* command response sample selection (MSDC_SMPL_RISING,
	   MSDC_SMPL_FALLING) */
	CMD_EDGE,

	/* cmd response async fifo out edge select */
	CMD_FIFO_EDGE,

	/* read data sample selection (MSDC_SMPL_RISING, MSDC_SMPL_FALLING) */
	RDATA_EDGE,

	/* read data async fifo out edge select */
	RD_FIFO_EDGE,

	/* write data crc status async fifo out edge select */
	WD_FIFO_EDGE,

	/* [Data Tune]CMD Pad RX Delay Line1 Control. This register is used to
	 * fine-tune CMD pad macro respose latch timing. Total 32 stages[Data
	 * Tune]
	 */
	CMD_RD_D_DLY1,

	/* [Data Tune]CMD Pad RX Delay Line1 Sel-> delay cell1 enable */
	CMD_RD_D_DLY1_SEL,

	/* [Data Tune]CMD Pad RX Delay Line2 Control. This register is used to
	 * fine-tune CMD pad macro respose latch timing. Total 32 stages[Data
	 * Tune]
	 */
	CMD_RD_D_DLY2,

	/* [Data Tune]CMD Pad RX Delay Line1 Sel-> delay cell2 enable */
	CMD_RD_D_DLY2_SEL,

	/* [Data Tune]DAT Pad RX Delay Line1 Control (for MSDC RD), Total 32
	   stages [Data Tune] */
	DAT_RD_D_DLY1,

	/* [Data Tune]DAT Pad RX Delay Line1 Sel-> delay cell1 enable */
	DAT_RD_D_DLY1_SEL,

	/* [Data Tune]DAT Pad RX Delay Line2 Control (for MSDC RD), Total 32
	   stages [Data Tune] */
	DAT_RD_D_DLY2,

	/* [Data Tune]DAT Pad RX Delay Line2 Sel-> delay cell2 enable */
	DAT_RD_D_DLY2_SEL,

	/* Internal MSDC clock phase selection. Total 8 stages, each stage can
	   delay 1 clock period of msdc_src_ck */
	INT_DAT_LATCH_CK,

	/* DS Pad Z clk delay count, range: 0~63, Z dly1(0~31)+Z dly2(0~31) */
	EMMC50_DS_Z_DLY1,

	/* DS Pad Z clk del sel: [dly2_sel:dly1_sel] -> [0,1]:
	 * dly1 enable [1,2]:dl2 & dly1 enable ,else :no dly enable
	 */
	EMMC50_DS_Z_DLY1_SEL,

	/* DS Pad Z clk delay count, range: 0~63, Z dly1(0~31)+Z dly2(0~31) */
	EMMC50_DS_Z_DLY2,

	/* DS Pad Z clk del sel: [dly2_sel:dly1_sel] -> [0,1]:
	 * dly1 enable [1,2]:dl2 & dly1 enable ,else :no dly enable,
	 */
	EMMC50_DS_Z_DLY2_SEL,

	/* DS Pad Z_DLY clk delay count, range: 0~31 */
	EMMC50_DS_ZDLY_DLY,

	/* eMMC50 CMD tx DLY */
	EMMC50_CMD_TX_DLY,

	/* eMMC50 DAT tx DLY */
	EMMC50_DAT0_TX_DLY,
	EMMC50_DAT1_TX_DLY,
	EMMC50_DAT2_TX_DLY,
	EMMC50_DAT3_TX_DLY,
	EMMC50_DAT4_TX_DLY,
	EMMC50_DAT5_TX_DLY,
	EMMC50_DAT6_TX_DLY,
	EMMC50_DAT7_TX_DLY,
	TUNING_PARAM_COUNT,

	/* CMD scan result */
	CMD_SCAN_R0,
	CMD_SCAN_R1,
	CMD_SCAN_R2,
	CMD_SCAN_R3,
	CMD_SCAN_R4,
	CMD_SCAN_R5,
	CMD_SCAN_R6,
	CMD_SCAN_R7,

	CMD_SCAN_F0,
	CMD_SCAN_F1,
	CMD_SCAN_F2,
	CMD_SCAN_F3,
	CMD_SCAN_F4,
	CMD_SCAN_F5,
	CMD_SCAN_F6,
	CMD_SCAN_F7,

	/* DATA scan result */
	DAT_SCAN_R0,
	DAT_SCAN_R1,
	DAT_SCAN_R2,
	DAT_SCAN_R3,
	DAT_SCAN_R4,
	DAT_SCAN_R5,
	DAT_SCAN_R6,
	DAT_SCAN_R7,

	DAT_SCAN_F0,
	DAT_SCAN_F1,
	DAT_SCAN_F2,
	DAT_SCAN_F3,
	DAT_SCAN_F4,
	DAT_SCAN_F5,
	DAT_SCAN_F6,
	DAT_SCAN_F7,

	/* DS scan result */
	DS_SCAN_0,
	DS_SCAN_1,
	DS_SCAN_2,
	DS_SCAN_3,
	DS_SCAN_4,
	DS_SCAN_5,
	DS_SCAN_6,
	DS_SCAN_7,

	/* Host CMD TX result */
	H_CMD_SCAN_0,
	H_CMD_SCAN_1,
	H_CMD_SCAN_2,
	H_CMD_SCAN_3,

	/* Host DATA TX result */
	H_DATA_SCAN_0,
	H_DATA_SCAN_1,
	H_DATA_SCAN_2,
	H_DATA_SCAN_3,

	/* AUTOK version */
	AUTOK_VER0,
	AUTOK_VER1,
	AUTOK_VER2,
	AUTOK_VER3,

	TUNING_PARA_SCAN_COUNT,

	/* Data line rising/falling latch  fine tune selection in read
	 * transaction.
	 * 1'b0: All data line share one value indicated by MSDC_IOCON.R_D_SMPL.
	 * 1'b1: Each data line has its own  selection value indicated by Data
	 * line (x): MSDC_IOCON.R_D(x)_SMPL
	 */
	READ_DATA_SMPL_SEL,

	/* Data line rising/falling latch  fine tune selection in write
	 * transaction.
	 * 1'b0: All data line share one value indicated by MSDC_IOCON.W_D_SMPL.
	 * 1'b1: Each data line has its own selection value indicated by Data
	 * line (x): MSDC_IOCON.W_D(x)_SMPL
	 */
	WRITE_DATA_SMPL_SEL,

	/* Data line delay line fine tune selection. 1'b0: All data line share
	 * one delay
	 * selection value indicated by PAD_TUNE.PAD_DAT_RD_RXDLY. 1'b1: Each
	 * data line has its
	 * own delay selection value indicated by Data line (x):
	 * DAT_RD_DLY(x).DAT0_RD_DLY
	 */
	DATA_DLYLINE_SEL,

	/* [Data Tune]CMD & DATA Pin tune Data Selection[Data Tune Sel] */
	MSDC_DAT_TUNE_SEL,

	/* [Async_FIFO Mode Sel For Write Path] */
	MSDC_WCRC_ASYNC_FIFO_SEL,

	/* [Async_FIFO Mode Sel For CMD Path] */
	MSDC_RESP_ASYNC_FIFO_SEL,

	/* Write Path Mux for emmc50 function & emmc45 function , Only emmc50
	   design valid,[1-eMMC50, 0-eMMC45] */
	EMMC50_WDATA_MUX_EN,

	/* CMD Path Mux for emmc50 function & emmc45 function , Only emmc50
	   design valid,[1-eMMC50, 0-eMMC45] */
	EMMC50_CMD_MUX_EN,

	/* write data crc status async fifo output edge select */
	EMMC50_WDATA_EDGE,

	/* CKBUF in CKGEN Delay Selection. Total 32 stages */
	CKGEN_MSDC_DLY_SEL,

	/* CMD response turn around period. The turn around cycle =
	 * CMD_RSP_TA_CNTR + 2,
	 * Only for USH104 mode, this register should be set to 0 in non-UHS104
	 * mode
	 */
	CMD_RSP_TA_CNTR,

	/* Write data and CRC status turn around period. The turn around cycle =
	 * WRDAT_CRCS_TA_CNTR + 2,
	 * Only for USH104 mode,  this register should be set to 0 in non-UHS104
	 * mode
	 */
	WRDAT_CRCS_TA_CNTR,

	/* CLK Pad TX Delay Control. This register is used to add delay to CLK
	   phase. Total 32 stages */
	PAD_CLK_TXDLY,
	TOTAL_PARAM_COUNT
};

struct AUTOK_PLAT_PARA_TX {
	unsigned int chip_hw_ver;

	u8 msdc0_hs400_clktx;
	u8 msdc0_hs400_cmdtx;
	u8 msdc0_hs400_dat0tx;
	u8 msdc0_hs400_dat1tx;
	u8 msdc0_hs400_dat2tx;
	u8 msdc0_hs400_dat3tx;
	u8 msdc0_hs400_dat4tx;
	u8 msdc0_hs400_dat5tx;
	u8 msdc0_hs400_dat6tx;
	u8 msdc0_hs400_dat7tx;
	u8 msdc0_hs400_txskew;
	u8 msdc0_ddr_ckd;
	u8 msdc_ddr_ckd;

	u8 msdc0_clktx;
	u8 msdc0_cmdtx;
	u8 msdc0_dat0tx;
	u8 msdc0_dat1tx;
	u8 msdc0_dat2tx;
	u8 msdc0_dat3tx;
	u8 msdc0_dat4tx;
	u8 msdc0_dat5tx;
	u8 msdc0_dat6tx;
	u8 msdc0_dat7tx;
	u8 msdc0_txskew;

	u8 msdc1_clktx;
	u8 msdc1_sdr104_clktx;

	u8 msdc2_clktx;
};

struct AUTOK_PLAT_PARA_RX {
	unsigned int chip_hw_ver;

	u8 ckgen_val;
	u8 latch_en_cmd_hs400;
	u8 latch_en_crc_hs400;
	u8 latch_en_cmd_hs200;
	u8 latch_en_crc_hs200;
	u8 latch_en_cmd_sdr104;
	u8 latch_en_crc_sdr104;
	u8 latch_en_cmd_hs;
	u8 latch_en_crc_hs;
	u8 cmd_ta_val;
	u8 crc_ta_val;
	u8 busy_ma_val;

	u8 new_water_hs400;
	u8 new_water_hs200;
	u8 new_water_sdr104;
	u8 new_water_hs;

	u8 new_stop_hs400;
	u8 new_stop_hs200;
	u8 new_stop_sdr104;
	u8 new_stop_hs;

	u8 old_water_hs400;
	u8 old_water_hs200;
	u8 old_water_sdr104;
	u8 old_water_hs;

	u8 old_stop_hs400;
	u8 old_stop_hs200;
	u8 old_stop_sdr104;
	u8 old_stop_hs;
};

struct AUTOK_PLAT_FUNC {
	unsigned int chip_hw_ver;

	u8 new_path_hs400;
	u8 new_path_hs200;
	u8 new_path_ddr208;
	u8 new_path_sdr104;
	u8 new_path_hs;
};

#define get_platform_para_tx(autok_para_tx)                                    \
	do {                                                                   \
		autok_para_tx.chip_hw_ver = mt_get_chip_hw_ver();              \
		autok_para_tx.msdc0_hs400_clktx = 0;                           \
		autok_para_tx.msdc0_hs400_cmdtx = 9;                           \
		autok_para_tx.msdc0_hs400_dat0tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat1tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat2tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat3tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat4tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat5tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat6tx = 0;                          \
		autok_para_tx.msdc0_hs400_dat7tx = 0;                          \
		autok_para_tx.msdc0_hs400_txskew = 0;                          \
		autok_para_tx.msdc0_ddr_ckd = 1;                               \
		autok_para_tx.msdc_ddr_ckd = 0;                                \
		autok_para_tx.msdc0_clktx = 0;                                 \
		autok_para_tx.msdc0_cmdtx = 0;                                 \
		autok_para_tx.msdc0_dat0tx = 0;                                \
		autok_para_tx.msdc0_dat1tx = 0;                                \
		autok_para_tx.msdc0_dat2tx = 0;                                \
		autok_para_tx.msdc0_dat3tx = 0;                                \
		autok_para_tx.msdc0_dat4tx = 0;                                \
		autok_para_tx.msdc0_dat5tx = 0;                                \
		autok_para_tx.msdc0_dat6tx = 0;                                \
		autok_para_tx.msdc0_dat7tx = 0;                                \
		autok_para_tx.msdc0_txskew = 0;                                \
		autok_para_tx.msdc1_clktx = 0;                                 \
		autok_para_tx.msdc1_sdr104_clktx = 0;                          \
		autok_para_tx.msdc2_clktx = 0;                                 \
		if (autok_para_tx.chip_hw_ver == 0xcb00) {                     \
			autok_para_tx.msdc1_sdr104_clktx = 8;                  \
			autok_para_tx.msdc2_clktx = 8;                         \
		}                                                              \
	} while (0)

#define get_platform_para_rx(autok_para_rx)                                    \
	do {                                                                   \
		autok_para_rx.chip_hw_ver = mt_get_chip_hw_ver();              \
		autok_para_rx.ckgen_val = 0;                                   \
		autok_para_rx.latch_en_cmd_hs400 = 3;                          \
		autok_para_rx.latch_en_crc_hs400 = 3;                          \
		autok_para_rx.latch_en_cmd_hs200 = 2;                          \
		autok_para_rx.latch_en_crc_hs200 = 2;                          \
		autok_para_rx.latch_en_cmd_sdr104 = 2;                         \
		autok_para_rx.latch_en_crc_sdr104 = 2;                         \
		autok_para_rx.latch_en_cmd_hs = 1;                             \
		autok_para_rx.latch_en_crc_hs = 1;                             \
		autok_para_rx.cmd_ta_val = 0;                                  \
		autok_para_rx.crc_ta_val = 0;                                  \
		autok_para_rx.busy_ma_val = 1;                                 \
		autok_para_rx.new_water_hs400 = 8;                             \
		autok_para_rx.new_stop_hs400 = 3;                              \
		autok_para_rx.new_water_hs200 = 0;                             \
		autok_para_rx.new_stop_hs200 = 6;                              \
		autok_para_rx.new_water_sdr104 = 0;                            \
		autok_para_rx.new_stop_sdr104 = 6;                             \
		autok_para_rx.new_water_hs = 8;                                \
		autok_para_rx.new_stop_hs = 3;                                 \
		autok_para_rx.old_water_hs400 = 8;                             \
		autok_para_rx.old_stop_hs400 = 3;                              \
		autok_para_rx.old_water_hs200 = 8;                             \
		autok_para_rx.old_stop_hs200 = 3;                              \
		autok_para_rx.old_water_sdr104 = 8;                            \
		autok_para_rx.old_stop_sdr104 = 3;                             \
		autok_para_rx.old_water_hs = 8;                                \
		autok_para_rx.old_stop_hs = 3;                                 \
		if (autok_para_rx.chip_hw_ver == 0xcb00) {                     \
			autok_para_rx.latch_en_cmd_hs200 = 1;                  \
			autok_para_rx.latch_en_crc_hs200 = 1;                  \
			autok_para_rx.latch_en_cmd_sdr104 = 1;                 \
			autok_para_rx.latch_en_crc_sdr104 = 1;                 \
		}                                                              \
	} while (0)

#define get_platform_func(autok_para_func)                                     \
	do {                                                                   \
		autok_para_func.chip_hw_ver = mt_get_chip_hw_ver();            \
		autok_para_func.new_path_hs400 = 1;                            \
		autok_para_func.new_path_hs200 = 1;                            \
		autok_para_func.new_path_ddr208 = 1;                           \
		autok_para_func.new_path_sdr104 = 1;                           \
		autok_para_func.new_path_hs = 1;                               \
		if (autok_para_func.chip_hw_ver == 0xcb00) {                   \
			autok_para_func.new_path_hs200 = 0;                    \
		}                                                              \
	} while (0)

/**********************************************************
* Feature  Control Defination                             *
**********************************************************/
#define AUTOK_OFFLINE_TUNE_TX_ENABLE 0
#define AUTOK_OFFLINE_TUNE_ENABLE 0
#define HS400_OFFLINE_TUNE_ENABLE 0
#define HS200_OFFLINE_TUNE_ENABLE 0
#define HS400_DSCLK_NEED_TUNING 0
#define AUTOK_PARAM_DUMP_ENABLE 0
#define SINGLE_EDGE_ONLINE_TUNE 0
/* #define CHIP_DENALI_3_DAT_TUNE */
/* #define SDIO_TUNE_WRITE_PATH */

/**********************************************************
* Function Declaration                                    *
**********************************************************/
extern int autok_path_sel(struct msdc_host *host);
extern int autok_init_sdr104(struct msdc_host *host);
extern int autok_init_hs200(struct msdc_host *host);
extern int autok_init_hs400(struct msdc_host *host);
extern struct AUTOK_TX_PARA autok_offline_tuning_TX(struct msdc_host *host,
						    unsigned int value);
extern void autok_msdc_tx_setting(struct msdc_host *host, struct mmc_ios *ios);
extern void autok_low_speed_switch_edge(struct msdc_host *host,
					struct mmc_ios *ios,
					enum ERROR_TYPE error_type);
extern void autok_tuning_parameter_init(struct msdc_host *host, u8 *res);
extern int autok_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs200_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs200_execute_tuning_cmd(struct msdc_host *host, u8 *res);
extern int hs400_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs400_execute_tuning_cmd(struct msdc_host *host, u8 *res);

#endif /* _AUTOK_H_ */
