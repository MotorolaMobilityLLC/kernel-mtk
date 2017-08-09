#ifndef __MT6627_FM_H__
#define __MT6627_FM_H__

#include "fm_typedef.h"

/* #define FM_PowerOn_with_ShortAntenna */
#define MT6627_RSSI_TH_LONG    0xFF01	/* FM radio long antenna RSSI threshold(11.375dBuV) */
#define MT6627_RSSI_TH_SHORT   0xFEE0	/* FM radio short antenna RSSI threshold(-1dBuV) */
#define MT6627_CQI_TH          0x00E9	/* FM radio Channel quality indicator threshold(0x0000~0x00FF) */
#define MT6627_SEEK_SPACE      1	/* FM radio seek space,1:100KHZ; 2:200KHZ */
#define MT6627_SCAN_CH_SIZE    40	/* FM radio scan max channel size */
/* FM radio band, 1:87.5MHz~108.0MHz; 2:76.0MHz~90.0MHz; 3:76.0MHz~108.0MHz; 4:special */
#define MT6627_BAND            1
#define MT6627_BAND_FREQ_L     875	/* FM radio special band low freq(Default 87.5MHz) */
#define MT6627_BAND_FREQ_H     1080	/* FM radio special band high freq(Default 108.0MHz) */
#define MT6627_DEEMPHASIS_50us TRUE

#define MT6627_SLAVE_ADDR    0xE0	/* 0x70 7-bit address */
#define MT6627_MAX_COUNT     100

#ifdef CONFIG_MTK_FM_50KHZ_SUPPORT
#define MT6627_SCANTBL_SIZE  26	/* 16*uinit16_t */
#else
#define MT6627_SCANTBL_SIZE  16	/* 16*uinit16_t */
#endif

#define AFC_ON  0x01
#if AFC_ON
#define FM_MAIN_CTRL_INIT  0x480
#else
#define FM_MAIN_CTRL_INIT  0x080
#endif

#define ext_clk			/* if define ext_clk use external reference clock or mask will use internal */
#define MT6627_DEV			"MT6627"

#endif /* end of #ifndef __MT6627_FM_H__ */
