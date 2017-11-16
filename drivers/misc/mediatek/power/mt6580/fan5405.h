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

#include <linux/types.h>
#include <mt-plat/upmu_common.h>
#ifndef _fan5405_SW_H_
#define _fan5405_SW_H_

/* #define HIGH_BATTERY_VOLTAGE_SUPPORT */

#define fan5405_CON0      0x00
#define fan5405_CON1      0x01
#define fan5405_CON2      0x02
#define fan5405_CON3      0x03
#define fan5405_CON4      0x04
#define fan5405_CON5      0x05
#define fan5405_CON6      0x06
#define fan5405_REG_NUM 7

/* CON0 */
#define CON0_TMR_RST_MASK   0x01
#define CON0_TMR_RST_SHIFT  7

#define CON0_OTG_MASK       0x01
#define CON0_OTG_SHIFT      7

#define CON0_EN_STAT_MASK   0x01
#define CON0_EN_STAT_SHIFT  6

#define CON0_STAT_MASK      0x03
#define CON0_STAT_SHIFT     4

#define CON0_BOOST_MASK     0x01
#define CON0_BOOST_SHIFT    3

#define CON0_FAULT_MASK     0x07
#define CON0_FAULT_SHIFT    0

/* CON1 */
#define CON1_LIN_LIMIT_MASK     0x03
#define CON1_LIN_LIMIT_SHIFT    6

#define CON1_LOW_V_MASK     0x03
#define CON1_LOW_V_SHIFT    4

#define CON1_TE_MASK        0x01
#define CON1_TE_SHIFT       3

#define CON1_CE_MASK        0x01
#define CON1_CE_SHIFT       2

#define CON1_HZ_MODE_MASK   0x01
#define CON1_HZ_MODE_SHIFT  1

#define CON1_OPA_MODE_MASK  0x01
#define CON1_OPA_MODE_SHIFT 0

/* CON2 */
#define CON2_OREG_MASK    0x3F
#define CON2_OREG_SHIFT   2

#define CON2_OTG_PL_MASK    0x01
#define CON2_OTG_PL_SHIFT   1

#define CON2_OTG_EN_MASK    0x01
#define CON2_OTG_EN_SHIFT   0

/* CON3 */
#define CON3_VENDER_CODE_MASK   0x07
#define CON3_VENDER_CODE_SHIFT  5

#define CON3_PIN_MASK           0x03
#define CON3_PIN_SHIFT          3

#define CON3_REVISION_MASK      0x07
#define CON3_REVISION_SHIFT     0

/* CON4 */
#define CON4_RESET_MASK     0x01
#define CON4_RESET_SHIFT    7

#define CON4_I_CHR_MASK     0x07
#define CON4_I_CHR_SHIFT    4

#define CON4_I_TERM_MASK    0x07
#define CON4_I_TERM_SHIFT   0

/* CON5 */
#define CON5_DIS_VREG_MASK      0x01
#define CON5_DIS_VREG_SHIFT     6

#define CON5_IO_LEVEL_MASK      0x01
#define CON5_IO_LEVEL_SHIFT     5

#define CON5_SP_STATUS_MASK     0x01
#define CON5_SP_STATUS_SHIFT    4

#define CON5_EN_LEVEL_MASK      0x01
#define CON5_EN_LEVEL_SHIFT     3

#define CON5_VSP_MASK           0x07
#define CON5_VSP_SHIFT          0

/* CON6 */
#define CON6_ISAFE_MASK     0x07
#define CON6_ISAFE_SHIFT    4

#define CON6_VSAFE_MASK     0x0F
#define CON6_VSAFE_SHIFT    0

/* CON0---------------------------------------------------- */
extern void fan5405_set_tmr_rst(u32 val);
extern u32 fan5405_get_otg_status(void);
extern void fan5405_set_en_stat(u32 val);
extern u32 fan5405_get_chip_status(void);
extern u32 fan5405_get_boost_status(void);
extern u32 fan5405_get_fault_status(void);
/* CON1---------------------------------------------------- */
extern void fan5405_set_input_charging_current(u32 val);
extern void fan5405_set_v_low(u32 val);
extern void fan5405_set_te(u32 val);
extern void fan5405_set_ce(u32 val);
extern void fan5405_set_hz_mode(u32 val);
extern void fan5405_set_opa_mode(u32 val);
/* CON2---------------------------------------------------- */
extern void fan5405_set_oreg(u32 val);
extern void fan5405_set_otg_pl(u32 val);
extern void fan5405_set_otg_en(u32 val);
/* CON3---------------------------------------------------- */
extern u32 fan5405_get_vender_code(void);
extern u32 fan5405_get_pn(void);
extern u32 fan5405_get_revision(void);
/* CON4---------------------------------------------------- */
extern void fan5405_set_reset(u32 val);
extern void fan5405_set_iocharge(u32 val);
extern void fan5405_set_iterm(u32 val);
/* CON5---------------------------------------------------- */
extern void fan5405_set_dis_vreg(u32 val);
extern void fan5405_set_io_level(u32 val);
extern u32 fan5405_get_sp_status(void);
extern u32 fan5405_get_en_level(void);
extern void fan5405_set_vsp(u32 val);
/* CON6---------------------------------------------------- */
extern void fan5405_set_i_safe(u32 val);
extern void fan5405_set_v_safe(u32 val);
/* --------------------------------------------------------- */
extern void fan5405_dump_register(void);
extern u32 fan5405_reg_config_interface(u8 RegNum, u8 val);

extern u32 fan5405_read_interface(u8 RegNum, u8 *val, u8 MASK, u8 SHIFT);
extern u32 fan5405_config_interface(u8 RegNum, u8 val, u8 MASK, u8 SHIFT);

extern u16 pmic_get_register_value(PMU_FLAGS_LIST_ENUM flagname);
extern int hw_charging_get_charger_type(void);
extern bool mt_usb_is_device(void);
extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
extern void mt_power_off(void);

#endif				/* _fan5405_SW_H_ */
