/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

/*****************************************************************************
*
* Filename:
* ---------
*   bq25898.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   bq25898 header file
*
* Author:
* -------
*
****************************************************************************/
#ifndef _bq25898_SW_H_
#define _bq25898_SW_H_

#define bq25898_CON0      0x00
#define bq25898_CON1      0x01
#define bq25898_CON2      0x02
#define bq25898_CON3      0x03
#define bq25898_CON4      0x04
#define bq25898_CON5      0x05
#define bq25898_CON6      0x06
#define bq25898_CON7     0x07
#define bq25898_CON8     0x08
#define bq25898_CON9     0x09
#define bq25898_CONA     0x0A
#define bq25898_CONB     0x0B
#define bq25898_CONC     0x0C
#define bq25898_COND     0x0D
#define bq25898_CONE     0x0E
#define bq25898_CON11    0X11
#define bq25898_CON12     0x12
#define bq25898_CON13     0x13




#define bq25898_REG_NUM 21
#define CON1_RESET_MASK   0x1
#define CON1_RESET_SHIFT  7






/* CON0 */
#define CON0_EN_HIZ_MASK   0x1
#define CON0_EN_HIZ_SHIFT  7

#define CON0_EN_ILIM_MASK   0x1
#define CON0_EN_ILIM_SHIFT  6

#define CON0_IINLIM_MASK   0x3F
#define CON0_IINLIM_SHIFT  0


/* CON1 */
#define CON1_VINDPM_OS_MASK   0x1F
#define CON1_VINDPM_OS_SHIFT  0


/* CON2 */
#define CON2_CONV_START_MASK   0x1
#define CON2_CONV_START_SHIFT  7

#define CON2_CONV_RATE_MASK   0x1
#define CON2_CONV_RATE_SHIFT  6

#define CON2_BOOST_FREQ_MASK   0x1
#define CON2_BOOST_FREQ_SHIFT  5

#define CON2_ICO_EN_MASK   0x1
#define CON2_ICO_EN_RATE_SHIFT  4

#define CON2_HVDCP_EN_MASK   0x1
#define CON2_HVDCP_EN_SHIFT  3

#define CON2_MAX_EN_MASK   0x1
#define CON2_MAX_EN_SHIFT  2

#define CON2_FORCE_DPDM_MASK   0x1
#define CON2_FORCE_DPDM_SHIFT  1

#define CON2_AUTO_DPDM_EN_MASK   0x1
#define CON2_AUTO_DPDM_EN_SHIFT  0




/* CON3 */
#define CON3_BAT_LOADEN_MASK   0x1
#define CON3_BAT_LOADEN_SHIFT  7

#define CON3_WD_MASK   0x1
#define CON3_WD_SHIFT  6

#define CON3_OTG_CONFIG_MASK   0x1
#define CON3_OTG_CONFIG_SHIFT  5

#define CON3_CHG_CONFIG_MASK   0x1
#define CON3_CHG_CONFIG_SHIFT  4

#define CON3_SYS_V_LIMIT_MASK   0x7
#define CON3_SYS_V_LIMIT_SHIFT  1



/* CON4 */
#define CON4_EN_PUMPX_MASK   0x1
#define CON4_EN_PUMPX_SHIFT  7

#define CON4_ICHG_MASK   0x7F
#define CON4_ICHG_SHIFT  0

/* CON5 */
#define CON5_IPRECHG_MASK   0xF
#define CON5_IPRECHG_SHIFT  4

#define CON5_ITERM_MASK   0xF
#define CON5_ITERM_SHIFT  0



/* CON6 */
#define CON6_2XTMR_EN_MASK   0x3F
#define CON6_2XTMR_EN_SHIFT  2

#define CON6_BATLOWV_MASK   0x1
#define CON6_BATLOWV_SHIFT  1

#define CON6_VRECHG_MASK   0x1
#define CON6_VRECHG_SHIFT  0

/* CON7 */

#define CON7_EN_TERM_CHG_MASK   0x1
#define CON7_EN_TERM_CHG_SHIFT  7

#define CON7_STAT_DIS_MASK   0x1
#define CON7_STAT_DIS_SHIFT  6

#define CON7_WTG_TIM_SET_MASK   0x3
#define CON7_WTG_TIM_SET_SHIFT  4

#define CON7_EN_TIMER_MASK   0x1
#define CON7_EN_TIMER_SHIFT  3

#define CON7_SET_CHG_TIM_MASK   0x3
#define CON7_SET_CHG_TIM_SHIFT  1

#define CON7_JEITA_ISET_MASK   0x1
#define CON7_JEITA_ISET_SHIFT  0
/* CON8 */
#define CON8_TREG_MASK 0x3
#define CON8_TREG_SHIFT 0

#define CON8_VCLAMP_MASK 0x7
#define CON8_VCLAMP_SHIFT 2

#define CON8_BAT_COMP_MASK 0x7
#define CON8_BAT_COMP_SHIFT 5
/* CON9 */

#define CON9_PUMPX_UP   0x1
#define CON9_PUMPX_UP_SHIFT  1

#define CON9_PUMPX_DN   0x1
#define CON9_PUMPX_DN_SHIFT  0

/* CONA */
#define CONA_BOOST_VLIM_MASK 0xF
#define CONA_BOOST_VLIM_SHIFT 4

#define CONA_BOOST_ILIM_MASK 0x07
#define CONA_BOOST_ILIM_SHIFT 0


/* CONB */

#define CONB_VBUS_STAT_MASK   0x7
#define CONB_VBUS_STAT_SHIFT  5

#define CONB_CHRG_STAT_MASK   0x3
#define CONB_CHRG_STAT_SHIFT  3

#define CONB_PG_STAT_MASK   0x1
#define CONB_PG_STAT_SHIFT  2

#define CONB_SDP_STAT_MASK   0x1
#define CONB_SDP_STAT_SHIFT  1

#define CONB_VSYS_STAT_MASK   0x1
#define CONB_VSYS_STAT_SHIFT  0


/* CONC */

#define CONB_WATG_STAT_MASK   0x1
#define CONB_WATG_STAT_SHIFT  7

#define CONB_BOOST_STAT_MASK   0x1
#define CONB_BOOST_STAT_SHIFT  6

/*#define CONB_CHRG_FAULT_MASK   0x3
#define CONB_CHRG_FAULT_SHIFT  4 //already define at line 175 */

#define CONB_BAT_STAT_MASK   0x1
#define CONB_BAT_STAT_SHIFT  3

/* COND */
#define COND_FORCE_VINDPM_MASK 0x01
#define COND_FORCE_VINDPM_SHIFT 7

#define COND_VINDPM_MASK   0x7F
#define COND_VINDPM_SHIFT  0

/* CONE */
#define CONE_VBAT_MASK 0x7F
#define CONE_VBAT_SHIFT 0

/* CON11 */
#define CON11_VBUS_MASK 0x7F
#define CON11_VBUS_SHIFT 0
/* CON12 */

#define CONB_ICHG_STAT_MASK   0x7F
#define CONB_ICHG_STAT_SHIFT  0

/* CON13 */
#define CON13_IDPM_STAT_MASK   0x1
#define CON13_IDPM_STAT_SHIFT  6

#define CON13_VDPM_STAT_MASK   0x1
#define CON13_VDPM_STAT_SHIFT  7


/**********************************************************
  *
  *   [Extern Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */
extern void bq25898_set_en_hiz(unsigned int val);
extern void bq25898_set_en_ilim(unsigned int val);
void bq25898_set_iinlim(unsigned int val);

/* CON1---------------------------------------------------- */
/* willcai */
extern void bq25898_set_vindpm(unsigned int val);
/*  */

/* CON2---------------------------------------------------- */
extern void bq25898_ADC_start(unsigned int val);
extern void bq25898_set_ADC_rate(unsigned int val);
extern void bq25898_set_ico_en_start(unsigned int val);
/*  */


/* CON3---------------------------------------------------- */
/* willcai */
extern void bq25898_wd_reset(unsigned int val);
extern void bq25898_otg_en(unsigned int val);
extern void bq25898_chg_en(unsigned int val);
extern void bq25898_set_sys_min(unsigned int val);
/*  */


/* CON4---------------------------------------------------- */

/* willcai */
extern void bq25898_en_pumpx(unsigned int val);
extern void bq25898_set_ichg(unsigned int val);

/*  */

/* CON5---------------------------------------------------- */

/* willcai */
extern void bq25898_set_iprechg(unsigned int val);
extern void bq25898_set_iterml(unsigned int val);
/*  */


/* CON6---------------------------------------------------- */
/* willcai */
extern void bq25898_set_vreg(unsigned int val);
extern void bq25898_set_batlowv(unsigned int val);
extern void bq25898_set_vrechg(unsigned int val);


/* con7 */

extern void bq25898_en_term_chg(unsigned int val);
extern void bq25898_en_state_dis(unsigned int val);
extern void bq25898_set_wd_timer(unsigned int val);
extern void bq25898_en_chg_timer(unsigned int val);

extern void bq25898_set_chg_timer(unsigned int val);


/* con8 */
extern void bq25898_set_thermal_regulation(unsigned int val);
extern void bq25898_set_VBAT_clamp(unsigned int val);
extern void bq25898_set_VBAT_IR_compensation(unsigned int val);
/* con9 */
void bq25898_pumpx_up(unsigned int val);




/* --------------------------------------------------------- */
extern void bq25898_dump_register(void);
extern unsigned int bq25898_reg_config_interface(unsigned char RegNum, unsigned char val);

extern unsigned int bq25898_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
					 unsigned char SHIFT);
extern unsigned int bq25898_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
					   unsigned char SHIFT);
/*
*static void hw_bc11_dump_register(void);
*static void hw_bc11_init(void);
*static U32 hw_bc11_DCD(void);
*static U32 hw_bc11_stepA1(void);
*static U32 hw_bc11_stepB1(void);
*static U32 hw_bc11_stepC1(void);
*static U32 hw_bc11_stepA2(void);
*static U32 hw_bc11_stepB2(void);
*static void hw_bc11_done(void);
*/
/* CONA */
extern void bq25898_set_boost_ilim(unsigned int val);
extern void bq25898_set_boost_vlim(unsigned int val);

/* CONB */
unsigned int bq25898_get_vbus_state(void);
unsigned int bq25898_get_chrg_state(void);
unsigned int bq25898_get_pg_state(void);
unsigned int bq25898_get_sdp_state(void);
unsigned int bq25898_get_vsys_state(void);
unsigned int bq25898_get_wdt_state(void);
unsigned int bq25898_get_boost_state(void);
unsigned int bq25898_get_chrg_fault_state(void);
unsigned int bq25898_get_bat_state(void);
unsigned int bq25898_get_ichg(void);

/* CON0D */
extern void bq25898_set_FORCE_VINDPM(unsigned int val);
extern void bq25898_set_VINDPM(unsigned int val);

/* CON11 */
extern unsigned int bq25898_get_vbus(void);

/* aggregated APIs */
extern void bq25898_hw_init(void);
extern void bq25898_charging_enable(unsigned int bEnable);
extern void bq25898_dump_register(void);
extern unsigned int bq25898_get_chrg_stat(void);


/*CON13*/
unsigned int bq25898_get_idpm_state(void);
unsigned int bq25898_get_vdpm_state(void);

/*Added for debuging to check power off caller*/
extern void dump_stack(void);
#endif
