/* Copyright (C) 2015 MediaTek Inc.
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

#ifndef _AUTOK_CUST_H_
#define _AUTOK_CUST_H_

#define AUTOK_VERSION                   (0x16101119)
#define AUTOK_LATCH_CK_EMMC_TUNE_TIMES  (10) /* 5.0IP eMMC 1KB fifo ZIZE */
#define AUTOK_LATCH_CK_SDIO_TUNE_TIMES  (20) /* 4.5IP 1KB fifo CMD19 need send 20 times  */
#define AUTOK_LATCH_CK_SD_TUNE_TIMES    (20) /* 4.5IP 1KB fifo CMD19 need send 20 times  */

/* autok platform specific setting */
#define AUTOK_CKGEN_VALUE                       (0)
#define AUTOK_CMD_LATCH_EN_HS400_PORT0_VALUE    (3)
#define AUTOK_CRC_LATCH_EN_HS400_PORT0_VALUE    (3)
#define AUTOK_CMD_LATCH_EN_DDR208_PORT3_VALUE   (3)
#define AUTOK_CRC_LATCH_EN_DDR208_PORT3_VALUE   (3)
#define AUTOK_CMD_LATCH_EN_HS200_PORT0_VALUE    (2)
#define AUTOK_CRC_LATCH_EN_HS200_PORT0_VALUE    (2)
#define AUTOK_CMD_LATCH_EN_SDR104_PORT1_VALUE   (2)
#define AUTOK_CRC_LATCH_EN_SDR104_PORT1_VALUE   (2)
#define AUTOK_CMD_LATCH_EN_HS_VALUE             (1)
#define AUTOK_CRC_LATCH_EN_HS_VALUE             (1)
#define AUTOK_CMD_TA_VALUE                      (0)
#define AUTOK_CRC_TA_VALUE                      (0)
#define AUTOK_BUSY_MA_VALUE                     (1)

/* autok msdc TX init setting */
#define AUTOK_MSDC0_HS400_CLKTXDLY            0
#define AUTOK_MSDC0_HS400_CMDTXDLY            0
#define AUTOK_MSDC0_HS400_DAT0TXDLY           12
#define AUTOK_MSDC0_HS400_DAT1TXDLY           0
#define AUTOK_MSDC0_HS400_DAT2TXDLY           0
#define AUTOK_MSDC0_HS400_DAT3TXDLY           0
#define AUTOK_MSDC0_HS400_DAT4TXDLY           3
#define AUTOK_MSDC0_HS400_DAT5TXDLY           6
#define AUTOK_MSDC0_HS400_DAT6TXDLY           0
#define AUTOK_MSDC0_HS400_DAT7TXDLY           0
#define AUTOK_MSDC0_HS400_TXSKEW              0

#define AUTOK_MSDC0_DDR50_DDRCKD              1
#define AUTOK_MSDC_DDRCKD                     0

#define AUTOK_MSDC0_CLKTXDLY                  0
#define AUTOK_MSDC0_CMDTXDLY                  0
#define AUTOK_MSDC0_DAT0TXDLY                 0
#define AUTOK_MSDC0_DAT1TXDLY                 0
#define AUTOK_MSDC0_DAT2TXDLY                 0
#define AUTOK_MSDC0_DAT3TXDLY                 0
#define AUTOK_MSDC0_DAT4TXDLY                 0
#define AUTOK_MSDC0_DAT5TXDLY                 0
#define AUTOK_MSDC0_DAT6TXDLY                 0
#define AUTOK_MSDC0_DAT7TXDLY                 0

#define AUTOK_MSDC0_TXSKEW                    0

#define AUTOK_MSDC1_CLK_TX_VALUE              0
#define AUTOK_MSDC1_CLK_SDR104_TX_VALUE       8

#define AUTOK_MSDC2_CLK_TX_VALUE              0

#define AUTOK_MSDC3_SDIO_PLUS_CLKTXDLY        0
#define AUTOK_MSDC3_SDIO_PLUS_CMDTXDLY        0
#define AUTOK_MSDC3_SDIO_PLUS_DAT0TXDLY       0
#define AUTOK_MSDC3_SDIO_PLUS_DAT1TXDLY       0
#define AUTOK_MSDC3_SDIO_PLUS_DAT2TXDLY       0
#define AUTOK_MSDC3_SDIO_PLUS_DAT3TXDLY       0

#define PORT0_PB0_RD_DAT_SEL_VALID
#define PORT1_PB0_RD_DAT_SEL_VALID
#define PORT3_PB0_RD_DAT_SEL_VALID

/**********************************************************
* Feature  Control Defination                             *
**********************************************************/
#define AUTOK_OFFLINE_TUNE_TX_ENABLE 0
#define AUTOK_OFFLINE_TUNE_DEVICE_RX_ENABLE 0
#define AUTOK_PARAM_DUMP_ENABLE   0
#define SINGLE_EDGE_ONLINE_TUNE   0
#define SDIO_PLUS_CMD_TUNE        1
#define STOP_CLK_NEW_PATH         0
/* #define CHIP_DENALI_3_DAT_TUNE */
/* #define SDIO_TUNE_WRITE_PATH */


#endif /* _AUTOK_CUST_H_ */

