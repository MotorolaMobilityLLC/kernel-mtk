//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_regs.h

	Description : header of FC8180 register map

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#ifndef __FC8180_REGS_H__
#define __FC8180_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif
extern u32 bbm_xtal_freq;

#define FEATURE_MTK
#define BBM_SPI_IF
/* #define BBM_I2C_SPI */
/* #define BBM_I2C_TSIF */
/* #define BBM_TSIF_LOW_ACTIVE */
#define BBM_INT_LOW_ACTIVE
/* #define BBM_AUX_INT */
/* #define BBM_FAIL_FRAME */
/* #define BBM_TS_204 */
/* #define BBM_SPI_PHA_1 */ /* SPI*/
#define BBM_I2C_SPI_POL 0   /* I2C+SPI */
#define BBM_I2C_SPI_PHA 0   /* I2C+SPI */
/* #define BBM_EXT_LNA */
/* #define BBM_EXT_TUN */ /* deprecated */

/* #define BBM_XTAL_FREQ               16000 */
/* #define BBM_XTAL_FREQ               16384 */
/* #define BBM_XTAL_FREQ               18000 */
/* #define BBM_XTAL_FREQ               19200 */
/* #define BBM_XTAL_FREQ               24000 */
/* #define BBM_XTAL_FREQ               26000 */
/* #define BBM_XTAL_FREQ               27000 */
/* #define BBM_XTAL_FREQ               27120 */
/* #define BBM_XTAL_FREQ               24576 */
/* #define BBM_XTAL_FREQ               32000 */
/* #define BBM_XTAL_FREQ               37200 */
/* #define BBM_XTAL_FREQ               37400 */
/* #define BBM_XTAL_FREQ               38400 */
// bbm frequency should be same as dtv input clock, otherwise tuner frequency can not be locked
#define DEFAULT_BBM_XTAL_FREQ          26000
#define BBM_XTAL_FREQ				   bbm_xtal_freq

#define BBM_BAND_WIDTH              6
/* #define BBM_BAND_WIDTH              7 */
/* #define BBM_BAND_WIDTH              8 */

	/* Host register */
#define BBM_ADDRESS_REG             0x00
#define BBM_COMMAND_REG             0x01
#define BBM_DATA_REG                0x02

	/* Common */
#define BBM_AP2APB_LT               0x0000
#define BBM_SW_RESET                0x0001
#define BBM_INT_STATUS              0x0002
#define BBM_INT_MASK                0x0003
#define BBM_INT_STS_EN              0x0006
#define BBM_AC_DATA                 0x0007
#define BBM_TS_DATA                 0x0008
#define BBM_TS_CLK_DIV              0x0010
#define BBM_TS_CTRL                 0x0011
#define BBM_MD_MISO                 0x0012
#define BBM_TS_SEL                  0x0013
#define BBM_TS_PAUSE                0x0014
#define BBM_RF_DEVID                0x0015
#define BBM_INT_AUTO_CLEAR          0x0017
#define BBM_INT_PERIOD              0x0018
#define BBM_NON_AUTO_INT_PERIOD     0x0019
#define BBM_STATUS_AUTO_CLEAR_EN    0x001a
#define BBM_BUF_SPIOUT              0x001c
#define BBM_INT_POLAR_SEL           0x0020
#define BBM_PATTERN_MODE            0x0021
#define BBM_CHIP_ID_L               0x0026
#define BBM_CHIP_VERSION            0x0028
#define BBM_FUNCTION_SEL            0x0070
#define BBM_TS_PAT_L                0x00a0
#define BBM_AC_PAT_L                0x00a2
#define BBM_VERIFY_TEST             0x00a4

	/* I2C */
#define BBM_I2C_PR_L                0x0030
#define BBM_I2C_PR_H                0x0031
#define BBM_I2C_CTR                 0x0032
#define BBM_I2C_RXR                 0x0033
#define BBM_I2C_SR                  0x0034
#define BBM_I2C_TXR                 0x0035
#define BBM_I2C_CR                  0x0036

	/* RSSI */
#define BBM_RSSI                    0x0100

	/* SYNC */
#define BBM_IFOECFG_HSR             0x2060

	/* CE */
#define BBM_WSCN_MSQ                0x4063

	/* FEC */
#define BBM_REQ_BER                 0x5000
#define BBM_A_BER_RXD_RSPS          0x5020
#define BBM_A_BER_ERR_RSPS          0x5022
#define BBM_A_BER_ERR_BITS          0x5024
#define BBM_BER_RXD_RSPS            0x5030
#define BBM_BER_ERR_RSPS            0x5032
#define BBM_BER_ERR_BITS            0x5034
#define BBM_DMP_BER_RXD_BITS        0x5040
#define BBM_DMP_BER_ERR_BITS        0x5044

	/* Buffer */
#define BBM_BUF_STATUS              0x8000
#define BBM_BUF_OVERRUN             0x8001
#define BBM_BUF_ENABLE              0x8002
#define BBM_BUF_INT                 0x8003
#define BBM_RS_FAIL_TX              0x8004

#define BBM_SYNC_RELATED_INT_STATUS 0x8006
#define BBM_SYNC_RELATED_INT_ENABLE 0x8007
#define BBM_HANGING_TS              0x800A
#define BBM_HANGING_AC              0x800B
#define BBM_HANGING_ENABLE          0x800C

#define BBM_BUF_TS_START            0x8010
#define BBM_BUF_AC_START            0x8012
#define BBM_BUF_TS_END              0x8020
#define BBM_BUF_AC_END              0x8022
#define BBM_BUF_TS_THR              0x8030
#define BBM_BUF_AC_THR              0x8032

	/* DM */
#define BBM_DM_DATA                 0xf000

	/*  Buffer Configuration */
#define TS_BUF_SIZE     (188 * 32 * 2)
#define TS_BUF_START    (0)
#define TS_BUF_END      (TS_BUF_START + TS_BUF_SIZE - 1)
#define TS_BUF_THR      ((TS_BUF_SIZE >> 1) - 1)

#define AC_BUF_SIZE     (204 * 2)
#define AC_BUF_START    (TS_BUF_START + TS_BUF_SIZE)
#define AC_BUF_END      (AC_BUF_START + AC_BUF_SIZE - 1)
#define AC_BUF_THR      ((AC_BUF_SIZE >> 1) - 1)

#ifdef __cplusplus
}
#endif

#endif /* __FC8180_REGS_H__ */
//add dtv ---shenyong@wind-mobi.com ---20161119 end 