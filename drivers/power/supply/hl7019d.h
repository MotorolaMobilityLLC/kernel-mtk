
/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/types.h>
#ifndef _HL7019D_SW_H_
#define _HL7019D_SW_H_

#define HL7019D_CON0		0x00
#define HL7019D_CON1		0x01
#define HL7019D_CON2		0x02
#define HL7019D_CON3		0x03
#define HL7019D_CON4		0x04
#define HL7019D_CON5		0x05
#define HL7019D_CON6		0x06
#define HL7019D_CON7		0x07
#define HL7019D_CON8		0x08
#define HL7019D_CON9		0x09
#define HL7019D_CONA		0x0A
#define HL7019D_CONB		0x0B
#define HL7019D_CONC		0x0C
#define HL7019D_COND		0x0D
#define HL7019D_REG_NUM		14

/* CON0 */
#define CON0_EN_HIZ_MASK	0x01
#define CON0_EN_HIZ_SHIFT	7
#define CON0_VINDPM_MASK	0x0F
#define CON0_VINDPM_SHIFT	3
#define CON0_IINLIM_MASK	0x07
#define CON0_IINLIM_SHIFT	0

/* CON1 */
#define CON1_REG_RESET_MASK			0x01
#define CON1_REG_RESET_SHIFT		7
#define CON1_I2C_WDT_RESET_MASK		0x01
#define CON1_I2C_WDT_RESET_SHIFT	6
#define CON1_CHG_CONFIG_MASK		0x03
#define CON1_CHG_CONFIG_SHIFT		4
#define CON1_SYS_MIN_MASK			0x07
#define CON1_SYS_MIN_SHIFT			1
#define CON1_BOOST_LIM_MASK			0x01
#define CON1_BOOST_LIM_SHIFT		0

/* CON2 */
#define CON2_ICHG_MASK			0x3F
#define CON2_ICHG_SHIFT			2
#define CON2_BCLOD_MASK			0x01
#define CON2_BCLOD_SHIFT		1
#define CON2_FORCE_20PCT_MASK	0x01
#define CON2_FORCE_20PCT_SHIFT	0

/* CON3 */
#define CON3_IPRECHG_MASK	0x0F
#define CON3_IPRECHG_SHIFT	4
#define CON3_ITERM_MASK		0x0F
#define CON3_ITERM_SHIFT	0

/* CON4 */
#define CON4_VREG_MASK		0x3F
#define CON4_VREG_SHIFT		2
#define CON4_BATLOWV_MASK	0x01
#define CON4_BATLOWV_SHIFT	1
#define CON4_VRECHG_MASK	0x01
#define CON4_VRECHG_SHIFT	0

/* CON5 */
#define CON5_EN_TERM_MASK			0x01
#define CON5_EN_TERM_SHIFT			7
#define CON5_TERM_STAT_MASK			0x01
#define CON5_TERM_STAT_SHIFT		6
#define CON5_WATCHDOG_MASK			0x03
#define CON5_WATCHDOG_SHIFT			4
#define CON5_EN_SAFE_TIMER_MASK		0x01
#define CON5_EN_SAFE_TIMER_SHIFT	3
#define CON5_CHG_TIMER_MASK			0x03
#define CON5_CHG_TIMER_SHIFT		1
#define CON5_Reserved_MASK			0x01
#define CON5_Reserved_SHIFT			0

/* CON6 */
#define CON6_BOOSTV_MASK	0x0F
#define CON6_BOOSTV_SHIFT	4
#define CON6_BHOT_MASK		0x03
#define CON6_BHOT_SHIFT		2
#define CON6_TREG_MASK		0x03
#define CON6_TREG_SHIFT		0

/* CON7 */
#define CON7_DPDM_EN_MASK				0x01
#define CON7_DPDM_EN_SHIFT				7
#define CON7_TMR2X_EN_MASK				0x01
#define CON7_TMR2X_EN_SHIFT				6
#define CON7_PPFET_DISABLE_MASK			0x01
#define CON7_PPFET_DISABLE_SHIFT		5
//three bits were reserved
#define CON7_CHRG_FAULT_INT_MASK_MASK	0x01
#define CON7_CHRG_FAULT_INT_MASK_SHIFT	1
#define CON7_BAT_FAULT_INT_MASK_MASK	0x01
#define CON7_BAT_FAULT_INT_MASK_SHIFT	0

/* CON8 */
#define CON8_VIN_STAT_MASK		0x03
#define CON8_VIN_STAT_SHIFT		6
#define CON8_CHRG_STAT_MASK		0x03
#define CON8_CHRG_STAT_SHIFT	4
#define CON8_DPM_STAT_MASK		0x01
#define CON8_DPM_STAT_SHIFT		3
#define CON8_PG_STAT_MASK		0x01
#define CON8_PG_STAT_SHIFT		2
#define CON8_THERM_STAT_MASK	0x01
#define CON8_THERM_STAT_SHIFT	1
#define CON8_VSYS_STAT_MASK		0x01
#define CON8_VSYS_STAT_SHIFT	0

/* CON9 */
#define CON9_WATCHDOG_FAULT_MASK	0x01
#define CON9_WATCHDOG_FAULT_SHIFT	7
#define CON9_OTG_FAULT_MASK			0x01
#define CON9_OTG_FAULT_SHIFT		6
#define CON9_CHRG_FAULT_MASK		0x03
#define CON9_CHRG_FAULT_SHIFT		4
#define CON9_BAT_FAULT_MASK			0x01
#define CON9_BAT_FAULT_SHIFT		3
// one bit was reserved
#define CON9_NTC_FAULT_MASK			0x07
#define CON9_NTC_FAULT_SHIFT		0

/* CONA */
// vender info register

/* CONB */
#define CONB_TSR_MASK				0x03
#define CONB_TSR_SHIFT				6
#define CONB_TRSP_MASK				0x01
#define CONB_TRSP_SHIFT				5
#define CONB_DIS_RECONNECT_MASK		0x01
#define CONB_DIS_RECONNECT_SHIFT	4
#define CONB_DIS_SR_INCHG_MASK		0x01
#define CONB_DIS_SR_INCHG_SHIFT		3
#define CONB_TSHIP_MASK				0x07
#define CONB_TSHIP_SHIFT			0

/* CONC */
#define CONC_BAT_COMP_MASK			0x07
#define CONC_BAT_COMP_SHIFT			5
#define CONC_BAT_VCLAMP_MASK		0x07
#define CONC_BAT_VCLAMP_SHIFT		2
// one bit was reserved
#define CONC_BOOST_9V_EN_MASK		0x01
#define CONC_BOOST_9V_EN_SHIFT		0

/* COND */
// one bit was reserved
#define COND_DISABLE_TS_MASK		0x01
#define COND_DISABLE_TS_SHIFT		6
#define COND_VINDPM_OFFSET_MASK		0x01
#define COND_VINDPM_OFFSET_SHIFT	5
// five bits were reserved


#endif
