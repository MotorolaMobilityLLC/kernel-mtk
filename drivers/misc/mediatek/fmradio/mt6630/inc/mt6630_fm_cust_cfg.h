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

#ifndef __MT6630_CUST_CFG_H__
#define __MT6630_CUST_CFG_H__

/* scan sort algorithm */
enum {
	FM_SCAN_SORT_NON = 0,
	FM_SCAN_SORT_UP,
	FM_SCAN_SORT_DOWN,
	FM_SCAN_SORT_MAX
};
/*typedef struct  {
    fm_s32 short_ana_rssi_th;
    fm_s32 long_ana_rssi_th;
    fm_s32 desene_rssi_th;
    fm_s32 pamd_th;
    fm_s32 mr_th;
    fm_s32 atdc_th;
    fm_u32 prx_th;
    fm_u32 atdev_th;
    fm_u16 smg_th;
    fm_u16 deemphasis;
    fm_u16 osc_freq;
}mt6628_fm_rx_cust_cfg;

typedef struct{
    mt6628_fm_rx_cust_cfg rx_cfg;
}mt6628_fm_cust_cfg;
*/
/* ***************************************************************************************** */
/* ***********************************FM config for customer: start****************************** */
/* ***************************************************************************************** */
/* RX */
#define FM_RX_RSSI_TH_LONG_MT6630    -296	/* FM radio long antenna RSSI threshold(-4dBuV) */
#define FM_RX_RSSI_TH_SHORT_MT6630   -296	/* FM radio short antenna RSSI threshold(-4dBuV) */
#define FM_RX_DESENSE_RSSI_MT6630   -258
#define FM_RX_PAMD_TH_MT6630          -12
#define FM_RX_MR_TH_MT6630           -67
#define FM_RX_ATDC_TH_MT6630           3496
#define FM_RX_PRX_TH_MT6630           64
#define FM_RX_SMG_TH_MT6630          16421	/* FM soft-mute gain threshold */
#define FM_RX_DEEMPHASIS_MT6630       0	/* 0-50us, China Mainland; 1-75us China Taiwan */
#define FM_RX_OSC_FREQ_MT6630         0	/* 0-26MHz; 1-19MHz; 2-24MHz; 3-38.4MHz; 4-40MHz; 5-52MHz */

/* TX */
/* #define FM_TX_PWR_LEVEL_MAX_MT6630  120 */
/* #define FM_TX_SCAN_HOLE_LOW_MT6630  923         //92.3MHz~95.4MHz should not show to user */
/* #define FM_TX_SCAN_HOLE_HIGH_MT6630 954         //92.3MHz~95.4MHz should not show to user */
#define FM_TX_PAMD_TH_MT6630		-23
#define FM_TX_MR_TH_MT6630			60
#define FM_TX_SMG_TH_MT6630			8231

/* ***************************************************************************************** */
/* ***********************************FM config for customer:end ******************************* */
/* ***************************************************************************************** */

/* #define FM_SEEK_SPACE_MT6630 FM_RX_SEEK_SPACE_MT6630 */
/* max scan chl num */
/* #define FM_MAX_CHL_SIZ_MT6630E FM_RX_SCAN_CH_SIZE_MT6630 */
/* auto HiLo */
#define FM_AUTO_HILO_OFF_MT6630    0
#define FM_AUTO_HILO_ON_MT6630     1

/* seek threshold */
#define FM_SEEKTH_LEVEL_DEFAULT_MT6630 4

extern fm_cust_cfg mt6630_fm_config;

#endif /* __MT6630_CUST_CFG_H__ */
