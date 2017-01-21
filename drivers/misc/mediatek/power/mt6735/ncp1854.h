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

/*****************************************************************************
*
* Filename:
* ---------
*   ncp1854.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   ncp1854 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _NCP1854_SW_H_
#define _NCP1854_SW_H_

#define NCP1854_CON0      0x00
#define NCP1854_CON1      0x01
#define NCP1854_CON2      0x02
#define NCP1854_CON3      0x03
#define NCP1854_CON4      0x04
#define NCP1854_CON5      0x05
#define NCP1854_CON6      0x06
#define NCP1854_CON7      0x07
#define NCP1854_CON8      0x08
#define NCP1854_CON9      0x09
#define NCP1854_CON10    0x0A
#define NCP1854_CON11    0x0B
#define NCP1854_CON12    0x0C
#define NCP1854_CON13    0x0D
#define NCP1854_CON14    0x0E
#define NCP1854_CON15    0x0F
#define NCP1854_CON16    0x10
#define NCP1854_CON17    0x11
#define NCP1854_CON18    0x12
#define ncp1854_REG_NUM 18

/**********************************************************
  *
  *   [MASK/SHIFT]
  *
  *********************************************************/
/* CON0 */
#define CON0_STATE_MASK			0x0F
#define CON0_STATE_SHIFT			4

#define CON0_BATFET_MASK			0x01
#define CON0_BATFET_SHIFT			3

#define CON0_STATINT_MASK			0x01
#define CON0_STATINT_SHIFT		1

#define CON0_FAULTINT_MASK		0x01
#define CON0_FAULTINT_SHIFT		0

/* CON1 */
#define CON1_REG_RST_MASK			0x01
#define CON1_REG_RST_SHIFT		7

#define CON1_CHG_EN_MASK			0x01
#define CON1_CHG_EN_SHIFT			6

#define CON1_OTG_EN_MASK			0x01
#define CON1_OTG_EN_SHIFT			5

#define CON1_FCTRY_MOD_MASK		0x01
#define CON1_FCTRY_MOD_SHIFT		4

#define CON1_TJ_WARN_OPT_MASK	0x01
#define CON1_TJ_WARN_OPT_SHIFT	3

#define CON1_JEITA_OPT_MASK		0x01
#define CON1_JEITA_OPT_SHIFT		2

#define CON1_TCHG_RST_MASK		0x01
#define CON1_TCHG_RST_SHIFT		1

#define CON1_INT_MASK_MASK		0x01
#define CON1_INT_MASK_SHIFT		0

/* CON2 */
#define CON2_WDTO_DIS_MASK		0x01
#define CON2_WDTO_DIS_SHIFT		7

#define CON2_CHGTO_DIS_MASK		0x01
#define CON2_CHGTO_DIS_SHIFT		6

#define CON2_PWR_PATH_MASK		0x01
#define CON2_PWR_PATH_SHIFT		5

#define CON2_TRANS_EN_MASK		0x01
#define CON2_TRANS_EN_SHIFT		4

#define CON2_IINSET_PIN_EN_MASK	0x01
#define CON2_IINSET_PIN_EN_SHIFT	2

#define CON2_IINLIM_EN_MASK		0x01
#define CON2_IINLIM_EN_SHIFT		1

#define CON2_AICL_EN_MASK			0x01
#define CON2_AICL_EN_SHIFT			0

/* CON3 */
/* Status flag */

/* CON4 */
/* Status flag */

/* CON5 */
/* Status flag */

/* CON6 */
/* Boost mode interrupt */
#define CON6_VOBSTOL2_MASK		0x01
#define CON6_VOBSTOL2_SHIFT		3

#define CON6_VOBSTOL1_MASK		0x01
#define CON6_VOBSTOL1_SHIFT		2

#define CON6_VBUSOV_MASK		0x01
#define CON6_VBUSOV_SHIFT		1

#define CON6_VBAT_NOK_MASK		0x01
#define CON6_VBAT_NOK_SHIFT		0

/* CON7 */
/* Status flag */

/* CON8 */
#define CON8_NTC_RMV_MASK		0x01
#define CON8_NTC_RMV_SHIFT		7

#define CON8_VBAT_OV_MASK			0x01
#define CON8_VBAT_OV_SHIFT			6

#define CON8_VRECHG_OK_MASK		0x01
#define CON8_VRECHG_OK_SHIFT		5

#define CON8_VFET_OK_MASK			0x01
#define CON8_VFET_OK_SHIFT		4

#define CON8_VPRE_OK_MASK			0x01
#define CON8_VPRE_OK_SHIFT		3

#define CON8_VSAFE_OK_MASK		0x01
#define CON8_VSAFE_OK_SHIFT		2

#define CON8_IEOC_OK_MASK			0x01
#define CON8_IEOC_OK_SHIFT			1

/* CON9 */
#define CON9_TSD_MASK				0x01
#define CON9_TSD_SHIFT				3

#define CON9_TM2_MASK				0x01
#define CON9_TM2_SHIFT				2

#define CON9_TM1_MASK				0x01
#define CON9_TM1_SHIFT				1

#define CON9_TWARN_MASK			0x01
#define CON9_TWARN_SHIFT			0

/* CON10 */
/* Interrupt mask */

/* CON11 */
/* Interrupt mask */

/* CON12 */
/* Interrupt mask */

/* CON13 */
/* Interrupt mask */

/* CON14 */
#define CON14_CTRL_VBAT_MASK		0x3F
#define CON14_CTRL_VBAT_SHIFT		0

/* CON15 */
#define CON15_ICHG_HIGH_MASK			0x01
#define CON15_ICHG_HIGH_SHIFT			7

#define CON15_IEOC_MASK			0x07
#define CON15_IEOC_SHIFT			4

#define CON15_ICHG_MASK			0x0F
#define CON15_ICHG_SHIFT			0

/* CON16 */
#define CON16_IWEAK_MASK			0x03
#define CON16_IWEAK_SHIFT			5

#define CON16_CTRL_VFET_MASK		0x07
#define CON16_CTRL_VFET_SHIFT		2

#define CON16_IINLIM_MASK			0x3
#define CON16_IINLIM_SHIFT			0

/* CON17 */
#define CON17_IINLIM_TA_MASK			0x0F
#define CON17_IINLIM_TA_SHIFT			4

/* CON18 */
#define CON18_BATCOLD_MASK		0x07
#define CON18_BATCOLD_SHIFT		5

#define CON18_BATHOT_MASK		0x07
#define CON18_BATHOT_SHIFT		2

#define CON18_BATCHIL_MASK		0x01
#define CON18_BATCHIL_SHIFT		1

#define CON18_BATWARM_MASK		0x01
#define CON18_BATWARM_SHIFT		0

/**********************************************************
  *
  *   [Extern Function]
  *
  *********************************************************/
#define NCP1854_REG_NUM 18
extern unsigned char ncp1854_reg[NCP1854_REG_NUM];

/* CON0 */
extern unsigned int ncp1854_get_chip_status(void);

extern unsigned int ncp1854_get_batfet(void);

extern unsigned int ncp1854_get_statint(void);

extern unsigned int ncp1854_get_faultint(void);


/* CON1 */
extern void ncp1854_set_reset(unsigned int val);

extern void ncp1854_set_chg_en(unsigned int val);

extern void ncp1854_set_otg_en(unsigned int val);

extern unsigned int ncp1854_get_otg_en(void);

extern void ncp1854_set_fctry_mode(unsigned int val);


extern void ncp1854_set_tj_warn_opt(unsigned int val);

extern unsigned int ncp1854_get_usb_cfg(void);

extern void ncp1854_set_tchg_rst(unsigned int val);

extern void ncp1854_set_int_mask(unsigned int val);

/* CON2 */
extern void ncp1854_set_wdto_dis(unsigned int val);

extern void ncp1854_set_chgto_dis(unsigned int val);

extern void ncp1854_set_pwr_path(unsigned int val);

extern void ncp1854_set_trans_en(unsigned int val);

extern void ncp1854_set_chg_halt(unsigned int val);

extern void ncp1854_set_iinset_pin_en(unsigned int val);

extern void ncp1854_set_iinlim_en(unsigned int val);

extern void ncp1854_set_aicl_en(unsigned int val);

/* CON8 */
extern unsigned int ncp1854_get_vfet_ok(void);

/* CON14 */
extern void ncp1854_set_ctrl_vbat(unsigned int val);

/* CON15 */
extern void ncp1854_set_ieoc(unsigned int val);

extern void ncp1854_set_ichg_high(unsigned int val);

extern void ncp1854_set_ichg(unsigned int val);

extern unsigned int ncp1854_get_ichg(void);

/* CON16 */
extern void ncp1854_set_iweak(unsigned int val);

extern void ncp1854_set_ctrl_vfet(unsigned int val);

extern void ncp1854_set_iinlim(unsigned int val);

/* CON17 */
extern void ncp1854_set_iinlim_ta(unsigned int val);

extern void ncp1854_dump_register(void);

extern void ncp1854_read_register(int i);

#endif				/* _NCP1854_SW_H_ */
