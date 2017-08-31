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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/scatterlist.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>

struct msdc_host;

#define E_RESULT_PASS     (0)
#define E_RESULT_CMD_TMO  (1<<0)
#define E_RESULT_RSP_CRC  (1<<1)
#define E_RESULT_DAT_CRC  (1<<2)
#define E_RESULT_DAT_TMO  (1<<3)
#define E_RESULT_W_CRC    (1<<4)
#define E_RESULT_ERR      (1<<5)
#define E_RESULT_START    (1<<6)
#define E_RESULT_PW_SMALL (1<<7)
#define E_RESULT_KEEP_OLD (1<<8)
#define E_RESULT_CMP_ERR  (1<<9)
#define E_RESULT_FATAL_ERR  (1<<10)

#define E_RESULT_MAX
/**************************************************************/
/* TOP REG                                   */
/**************************************************************/

#define MSDC_TOP_BASE		0x11d60000

/* TOP REGISTER */
#define EMMC_TOP_CONTROL_ADDR           (MSDC_TOP_BASE + 0x00)
#define EMMC_TOP_CMD_ADDR               (MSDC_TOP_BASE + 0x04)
#define TOP_EMMC50_PAD_CTL0_ADDR        (MSDC_TOP_BASE + 0x08)
#define TOP_EMMC50_PAD_DS_TUNE_ADDR     (MSDC_TOP_BASE + 0x0c)
#define TOP_EMMC50_PAD_DAT0_TUNE_ADDR   (MSDC_TOP_BASE + 0x10)
#define TOP_EMMC50_PAD_DAT1_TUNE_ADDR   (MSDC_TOP_BASE + 0x14)
#define TOP_EMMC50_PAD_DAT2_TUNE_ADDR   (MSDC_TOP_BASE + 0x18)
#define TOP_EMMC50_PAD_DAT3_TUNE_ADDR   (MSDC_TOP_BASE + 0x1c)
#define TOP_EMMC50_PAD_DAT4_TUNE_ADDR   (MSDC_TOP_BASE + 0x20)
#define TOP_EMMC50_PAD_DAT5_TUNE_ADDR   (MSDC_TOP_BASE + 0x24)
#define TOP_EMMC50_PAD_DAT6_TUNE_ADDR   (MSDC_TOP_BASE + 0x28)
#define TOP_EMMC50_PAD_DAT7_TUNE_ADDR   (MSDC_TOP_BASE + 0x2c)

/* EMMC_TOP_CONTROL mask */
#define PAD_RXDLY_SEL           (0x1 << 0)      /* RW */
#define DELAY_EN                (0x1 << 1)      /* RW */
#define PAD_DAT_RD_RXDLY2       (0x1F << 2)     /* RW */
#define PAD_DAT_RD_RXDLY        (0x1F << 7)     /* RW */
#define PAD_DAT_RD_RXDLY2_SEL   (0x1 << 12)     /* RW */
#define PAD_DAT_RD_RXDLY_SEL    (0x1 << 13)     /* RW */
#define DATA_K_VALUE_SEL        (0x1 << 14)     /* RW */

/* EMMC_TOP_CMD mask */
#define PAD_CMD_RXDLY2          (0x1F << 0)     /* RW */
#define PAD_CMD_RXDLY           (0x1F << 5)     /* RW */
#define PAD_CMD_RD_RXDLY2_SEL   (0x1 << 10)     /* RW */
#define PAD_CMD_RD_RXDLY_SEL    (0x1 << 11)     /* RW */
#define PAD_CMD_TX_DLY          (0x1F << 12)    /* RW */

/* TOP_EMMC50_PAD_CTL0_ADDR mask */
#define HL_SEL          (0x1 << 0)      /* RW */
#define DCC_SEL         (0x1 << 1)      /* RW */
#define DLN1            (0x3 << 2)      /* RW */
#define DLN0            (0x3 << 4)      /* RW */
#define DLP1            (0x3 << 6)      /* RW */
#define DLP0            (0x3 << 8)      /* RW */
#define PAD_CLK_TXDLY   (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DS_TUNE mask */
#define PAD_DS_DLY3         (0x1F << 0)     /* RW */
#define PAD_DS_DLY2         (0x1F << 5)     /* RW */
#define PAD_DS_DLY1         (0x1F << 10)    /* RW */
#define PAD_DS_DLY2_SEL     (0x1 << 15)     /* RW */
#define PAD_DS_DLY_SEL      (0x1 << 16)     /* RW */

/* TOP_EMMC50_PAD_DAT0_TUNE mask */
#define DAT0_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT0_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT0_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT1_TUNE mask */
#define DAT1_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT1_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT1_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT2_TUNE mask */
#define DAT2_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT2_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT2_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT3_TUNE mask */
#define DAT3_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT3_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT3_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT4_TUNE mask */
#define DAT4_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT4_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT4_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT5_TUNE mask */
#define DAT5_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT5_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT5_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT6_TUNE mask */
#define DAT6_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT6_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT6_TX_DLY     (0x1F << 10)    /* RW */

/* TOP_EMMC50_PAD_DAT7_TUNE mask */
#define DAT7_RD_DLY2        (0x1F << 0)     /* RW */
#define DAT7_RD_DLY1        (0x1F << 5)     /* RW */
#define PAD_DAT7_TX_DLY     (0x1F << 10)    /* RW */


#ifndef NULL
#define NULL                0
#endif
#ifndef TRUE
#define TRUE                (0 == 0)
#endif
#ifndef FALSE
#define FALSE               (0 != 0)
#endif

#define AUTOK_DBG_OFF                             0
#define AUTOK_DBG_ERROR                           1
#define AUTOK_DBG_RES                             2
#define AUTOK_DBG_WARN                            3
#define AUTOK_DBG_TRACE                           4
#define AUTOK_DBG_LOUD                            5

extern unsigned int autok_debug_level;

#define AUTOK_DBGPRINT(_level, _fmt ...)   \
({                                         \
	if (autok_debug_level >= _level) { \
		pr_err(_fmt);              \
	}                                  \
})

#define AUTOK_RAWPRINT(_fmt ...)           \
({                                         \
	pr_err(_fmt);                      \
})

enum ERROR_TYPE {
	CMD_ERROR = 0,
	DATA_ERROR,
	CRC_STATUS_ERROR,
};

enum AUTOK_PARAM {
	/* command response sample selection (MSDC_SMPL_RISING, MSDC_SMPL_FALLING) */
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
	 * fine-tune CMD pad macro respose latch timing. Total 32 stages[Data Tune]
	 */
	CMD_RD_D_DLY1,

	/* [Data Tune]CMD Pad RX Delay Line1 Sel-> delay cell1 enable */
	CMD_RD_D_DLY1_SEL,

	/* [Data Tune]CMD Pad RX Delay Line2 Control. This register is used to
	 * fine-tune CMD pad macro respose latch timing. Total 32 stages[Data Tune]
	 */
	CMD_RD_D_DLY2,

	/* [Data Tune]CMD Pad RX Delay Line1 Sel-> delay cell2 enable */
	CMD_RD_D_DLY2_SEL,

	/* [Data Tune]DAT Pad RX Delay Line1 Control (for MSDC RD), Total 32 stages [Data Tune] */
	DAT_RD_D_DLY1,

	/* [Data Tune]DAT Pad RX Delay Line1 Sel-> delay cell1 enable */
	DAT_RD_D_DLY1_SEL,

	/* [Data Tune]DAT Pad RX Delay Line2 Control (for MSDC RD), Total 32 stages [Data Tune] */
	DAT_RD_D_DLY2,

	/* [Data Tune]DAT Pad RX Delay Line2 Sel-> delay cell2 enable */
	DAT_RD_D_DLY2_SEL,

	/* Internal MSDC clock phase selection. Total 8 stages, each stage can delay 1 clock period of msdc_src_ck */
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
	TUNING_PARAM_COUNT,

	/* Data line rising/falling latch  fine tune selection in read transaction.
	 * 1'b0: All data line share one value indicated by MSDC_IOCON.R_D_SMPL.
	 * 1'b1: Each data line has its own  selection value indicated by Data line (x): MSDC_IOCON.R_D(x)_SMPL
	 */
	READ_DATA_SMPL_SEL,

	/* Data line rising/falling latch  fine tune selection in write transaction.
	 * 1'b0: All data line share one value indicated by MSDC_IOCON.W_D_SMPL.
	 * 1'b1: Each data line has its own selection value indicated by Data line (x): MSDC_IOCON.W_D(x)_SMPL
	 */
	WRITE_DATA_SMPL_SEL,

	/* Data line delay line fine tune selection. 1'b0: All data line share one delay
	 * selection value indicated by PAD_TUNE.PAD_DAT_RD_RXDLY. 1'b1: Each data line has its
	 * own delay selection value indicated by Data line (x): DAT_RD_DLY(x).DAT0_RD_DLY
	 */
	DATA_DLYLINE_SEL,

	/* [Data Tune]CMD & DATA Pin tune Data Selection[Data Tune Sel] */
	MSDC_DAT_TUNE_SEL,

	/* [Async_FIFO Mode Sel For Write Path] */
	MSDC_WCRC_ASYNC_FIFO_SEL,

	/* [Async_FIFO Mode Sel For CMD Path] */
	MSDC_RESP_ASYNC_FIFO_SEL,

	/* Write Path Mux for emmc50 function & emmc45 function , Only emmc50 design valid,[1-eMMC50, 0-eMMC45] */
	EMMC50_WDATA_MUX_EN,

	/* CMD Path Mux for emmc50 function & emmc45 function , Only emmc50 design valid,[1-eMMC50, 0-eMMC45] */
	EMMC50_CMD_MUX_EN,

	/* CMD response DS latch or internal clk latch */
	EMMC50_CMD_RESP_LATCH,

	/* write data crc status async fifo output edge select */
	EMMC50_WDATA_EDGE,

	/* CKBUF in CKGEN Delay Selection. Total 32 stages */
	CKGEN_MSDC_DLY_SEL,

	/* CMD response turn around period. The turn around cycle = CMD_RSP_TA_CNTR + 2,
	 * Only for USH104 mode, this register should be set to 0 in non-UHS104 mode
	 */
	CMD_RSP_TA_CNTR,

	/* Write data and CRC status turn around period. The turn around cycle = WRDAT_CRCS_TA_CNTR + 2,
	 * Only for USH104 mode,  this register should be set to 0 in non-UHS104 mode
	 */
	WRDAT_CRCS_TA_CNTR,

	/* CLK Pad TX Delay Control. This register is used to add delay to CLK phase. Total 32 stages */
	PAD_CLK_TXDLY_AUTOK,
	TOTAL_PARAM_COUNT
};

/**********************************************************
* Feature  Control Defination                             *
**********************************************************/
#define AUTOK_OFFLINE_TUNE_TX_ENABLE 0
#define AUTOK_PARAM_DUMP_ENABLE   0
#define SINGLE_EDGE_ONLINE_TUNE   0
#define SDIO_PLUS_CMD_TUNE        1
/* #define CHIP_DENALI_3_DAT_TUNE */
/* #define SDIO_TUNE_WRITE_PATH */



/**********************************************************
* Function Declaration                                    *
**********************************************************/
extern int autok_path_sel(struct msdc_host *host);
extern int autok_init_ddr208(struct msdc_host *host);
extern int autok_init_sdr104(struct msdc_host *host);
extern int autok_init_hs200(struct msdc_host *host);
extern int autok_init_hs400(struct msdc_host *host);
extern int autok_offline_tuning_TX(struct msdc_host *host);
extern void autok_msdc_tx_setting(struct msdc_host *host, struct mmc_ios *ios);
extern void autok_low_speed_switch_edge(struct msdc_host *host, struct mmc_ios *ios, enum ERROR_TYPE error_type);
extern void autok_tuning_parameter_init(struct msdc_host *host, u8 *res);
extern int autok_sdio30_plus_tuning(struct msdc_host *host, u8 *res);
extern int autok_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs200_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs200_execute_tuning_cmd(struct msdc_host *host, u8 *res);
extern int hs400_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs400_execute_tuning_cmd(struct msdc_host *host, u8 *res);

#endif  /* _AUTOK_H_ */

