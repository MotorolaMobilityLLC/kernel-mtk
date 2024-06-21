/* SPDX-License-Identifier: GPL-2.0-only */
// sgm4154x Charger Driver
// Copyright (C) 2021 Texas Instruments Incorporated - http://www.sg-micro.com

#ifndef _SGM4154x_CHARGER_H
#define _SGM4154x_CHARGER_H

#include <linux/i2c.h>

#define SGM4154x_MANUFACTURER	"Texas Instruments"
#define __SGM41541_CHIP_ID__
#define __SGM41542_CHIP_ID__
#define __SGM41513_CHIP_ID__
#define __SGM41513A_CHIP_ID__
#define __SGM41513D_CHIP_ID__
#define __SGM41516_CHIP_ID__
#define __SGM41516D_CHIP_ID__
#define __SGM41543_CHIP_ID__
#define __SGM41543D_CHIP_ID__

#ifdef __SGM41541_CHIP_ID__
#define SGM41541_NAME		"sgm41541"
#define SGM41541_PN_ID     (BIT(6)| BIT(5))
#endif

#ifdef __SGM41542_CHIP_ID__
#define SGM41542_NAME		"sgm41542"
#define SGM41542_PN_ID      (BIT(6)| BIT(5)| BIT(3))
#endif

#ifdef __SGM41513_CHIP_ID__
#define SGM41513_NAME		"sgm41513"
#define SGM41513_PN_ID      0
#endif

#ifdef __SGM41513A_CHIP_ID__
#define SGM41513A_NAME		"sgm41513A"
#define SGM41513A_PN_ID      BIT(3)
#endif

#ifdef __SGM41513D_CHIP_ID__
#define SGM4153D_NAME		"sgm41513D"
#define SGM4153D_PN_ID      BIT(3)
#endif

#ifdef __SGM41516_CHIP_ID__
#define SGM41516_NAME		"sgm41516"
#define SGM41516_PN_ID      (BIT(6)| BIT(5))
#endif

#ifdef __SGM41516D_CHIP_ID__
#define SGM41516D_NAME		"sgm41516D"
#define SGM41516D_PN_ID     (BIT(6)| BIT(5)| BIT(3))
#endif

#ifdef __SGM41543_CHIP_ID__
#define SGM41543_NAME		"sgm41543"
#define SGM41543_PN_ID      BIT(6)
#endif

#ifdef __SGM41543D_CHIP_ID__
#define SGM41543D_NAME		"sgm41543D"
#define SGM41543D_PN_ID      (BIT(6)| BIT(3))
#endif

#define SGM4154x_IINDPM_INT		BIT(0)
#define SGM4154x_VINDPM_INT		BIT(1)


enum attach_type {
	ATTACH_TYPE_NONE,
	ATTACH_TYPE_PWR_RDY,
	ATTACH_TYPE_TYPEC,
	ATTACH_TYPE_PD,
	ATTACH_TYPE_PD_SDP,
	ATTACH_TYPE_PD_DCP,
	ATTACH_TYPE_PD_NONSTD,
};
/*define register*/
#define SGM4154x_CHRG_CTRL_0	0x00
#define SGM4154x_CHRG_CTRL_1	0x01
#define SGM4154x_CHRG_CTRL_2	0x02
#define SGM4154x_CHRG_CTRL_3	0x03
#define SGM4154x_CHRG_CTRL_4	0x04
#define SGM4154x_CHRG_CTRL_5	0x05
#define SGM4154x_CHRG_CTRL_6	0x06
#define SGM4154x_CHRG_CTRL_7	0x07
#define SGM4154x_CHRG_STAT	    0x08
#define SGM4154x_CHRG_FAULT	    0x09
#define SGM4154x_CHRG_CTRL_a	0x0a
#define SGM4154x_CHRG_CTRL_b	0x0b
#define SGM4154x_CHRG_CTRL_c	0x0c
#define SGM4154x_CHRG_CTRL_d	0x0d
#define SGM4154x_INPUT_DET   	0x0e
#define SGM4154x_CHRG_CTRL_f	0x0f

/* Register 0x04*/
#define REG04_VREG_MASK		0xF8
#define REG04_VREG_SHIFT	3
#define REG04_VREG_BASE		3856
#define REG04_VREG_LSB		32

#define SGM4154x_REG_08		0x08
#define SGM4154x_REG_0A		0x0A
#define SGM4154x_REG_0F		0x0F
#define SGM41543_VREG_FT_MASK	0xc0
#define	REG0A_VINDPM_STAT_MASK	0x40
#define SGM4154x_VREG_LSB	32

/* charge status flags  */
#define SGM4154x_CHRG_EN		BIT(4)
#define SGM4154x_HIZ_EN		    BIT(7)
#define SGM4154x_TERM_EN		BIT(7)
#define SGM4154x_VAC_OVP_MASK	GENMASK(7, 6)
#define SGM4154x_DPDM_ONGOING   BIT(7)
#define SGM4154x_VBUS_GOOD      BIT(7)

/* control led */
#define	REG00_STAT_CTRL_MASK		0x60
#define REG00_STAT_CTRL_SHIFT		5
#define	REG00_STAT_CTRL_ICHG		1
#define	REG00_STAT_CTRL_IINDPM		2
#define	REG00_STAT_CTRL_DISABLE		3

/*sysmin*/
#define SGM4154x_SYS_MIN_MASK      GENMASK(3, 1)
#define SGM4154x_SYS_MIN_3200MV    (BIT(1)| BIT(2))
#define SGM4154x_SYS_MIN_3400MV    BIT(3)
#define SGM4154x_SYS_MIN_3500MV    (BIT(1)| BIT(3))
#define SGM4154x_SYS_MIN_3600MV    (BIT(2)| BIT(3))

/*otg*/
#define SGM4154x_BOOSTV		GENMASK(5, 4)
#define SGM4154x_BOOST_LIM 		BIT(7)
#define SGM4154x_OTG_EN		    BIT(5)

/* Part ID  */
#define SGM4154x_PN_MASK	    GENMASK(6, 3)

/* WDT TIMER SET  */
#define SGM4154x_WDT_TIMER_MASK        GENMASK(5, 4)
#define SGM4154x_WDT_TIMER_DISABLE     0
#define SGM4154x_WDT_TIMER_40S         BIT(4)
#define SGM4154x_WDT_TIMER_80S         BIT(5)
#define SGM4154x_WDT_TIMER_160S        (BIT(4)| BIT(5))

#define SGM4154x_WDT_RST_MASK          BIT(6)

/*En termniataion*/
#define SGM4154x_EN_TERM_MASK          GENMASK(7,7)
#define SGM4154x_EN_TERM_DISABLE          0
#define SGM4154x_EN_TERM_ENABLE          BIT(7)

/* SAFETY TIMER SET  */
#define SGM4154x_SAFETY_TIMER_MASK     GENMASK(3, 3)
#define SGM4154x_SAFETY_TIMER_DISABLE     0
#define SGM4154x_SAFETY_TIMER_EN       BIT(3)
#define SGM4154x_SAFETY_TIMER_5H         0
#define SGM4154x_SAFETY_TIMER_10H      BIT(2)

/* recharge voltage  */
#define SGM4154x_VRECHARGE              BIT(0)
#define SGM4154x_VRECHRG_STEP_mV		100
#define SGM4154x_VRECHRG_OFFSET_mV		100

/* charge status  */
#define SGM4154x_VSYS_STAT		BIT(0)
#define SGM4154x_THERM_STAT		BIT(1)
#define SGM4154x_PG_STAT		BIT(2)
#define SGM4154x_CHG_STAT_MASK	GENMASK(4, 3)
#define SGM4154x_PRECHRG		BIT(3)
#define SGM4154x_FAST_CHRG	    BIT(4)
#define SGM4154x_TERM_CHRG	    (BIT(3)| BIT(4))

/* charge type  */
#define SGM4154x_VBUS_STAT_MASK	GENMASK(7, 5)
#define SGM4154x_NOT_CHRGING	0
#define SGM4154x_USB_SDP		BIT(5)
#define SGM4154x_USB_CDP		BIT(6)
#define SGM4154x_USB_DCP		(BIT(5) | BIT(6))
#define SGM4154x_UNKNOWN	    (BIT(7) | BIT(5))
#define SGM4154x_NON_STANDARD	(BIT(7) | BIT(6))
#define SGM4154x_OTG_MODE	    (BIT(7) | BIT(6) | BIT(5))

/* TEMP Status  */
#define SGM4154x_TEMP_MASK	    GENMASK(2, 0)
#define SGM4154x_TEMP_NORMAL	BIT(0)
#define SGM4154x_TEMP_WARM	    BIT(1)
#define SGM4154x_TEMP_COOL	    (BIT(0) | BIT(1))
#define SGM4154x_TEMP_COLD	    (BIT(0) | BIT(3))
#define SGM4154x_TEMP_HOT	    (BIT(2) | BIT(3))

/* precharge current  */
#define SGM4154x_PRECHRG_CUR_MASK		GENMASK(7, 4)
#define SGM4154x_PRECHRG_CURRENT_STEP_uA		60000
#define SGM4154x_PRECHRG_I_MIN_uA		60000
#define SGM4154x_PRECHRG_I_MAX_uA		780000
#define SGM4154x_PRECHRG_I_DEF_uA		180000

/* termination current  */
#define SGM4154x_TERMCHRG_CUR_MASK		GENMASK(3, 0)
#define SGM4154x_TERMCHRG_CURRENT_STEP_uA	60000
#define SGM4154x_TERMCHRG_I_MIN_uA		60000
#define SGM4154x_TERMCHRG_I_MAX_uA		960000
#define SGM4154x_TERMCHRG_I_DEF_uA		240000

/* charge current  */
#define SGM4154x_ICHRG_I_MASK		GENMASK(5, 0)

#define SGM4154x_ICHRG_I_MIN_uA			0
#define SGM41513_ICHRG_I_MAX_uA			3000000
#define SGM41513_ICHRG_I_DEF_uA			1980000
#define SGM4154x_ICHRG_I_STEP_uA	    60000
#define SGM4154x_ICHRG_I_MAX_uA			3780000
#define SGM4154x_ICHRG_I_DEF_uA			2040000
/* charge voltage  */
#define SGM4154x_VREG_V_MASK		GENMASK(7, 3)
#define SGM4154x_VREG_V_MAX_uV	    4624000
#define SGM4154x_VREG_V_MIN_uV	    3856000
#define SGM4154x_VREG_V_DEF_uV	    4450000
#define SGM4154x_VREG_V_STEP_uV	    32000

/* VREG Fine Tuning  */
#define SGM4154x_VREG_FT_MASK	     GENMASK(7, 6)
#define SGM4154x_VREG_FT_UP_8mV	     BIT(6)
#define SGM4154x_VREG_FT_DN_8mV	     BIT(7)
#define SGM4154x_VREG_FT_DN_16mV	 (BIT(7) | BIT(6))

/*rerun apsd*/
#define SGM4154x_IINDET_EN_MASK		GENMASK(7, 7)
#define SGM4154x_IINDET_EN				BIT(7)

/* iindpm current  */
#define SGM4154x_IINDPM_I_MASK		GENMASK(4, 0)
#define SGM4154x_IINDPM_I_MIN_uA	100000
#define SGM41513_IINDPM_I_MAX_uA	3200000
#define SGM415xx_IINDPM_I_MAX_uA	3800000
#define SGM4154x_IINDPM_STEP_uA	    100000
#define SGM4154x_IINDPM_DEF_uA	    2400000

/* vindpm voltage  */
#define SGM4154x_VINDPM_V_MASK      GENMASK(3, 0)
#define SGM4154x_VINDPM_V_MIN_uV    3900000
#define SGM4154x_VINDPM_V_MAX_uV    12000000
#define SGM4154x_VINDPM_STEP_uV     100000
#define SGM4154x_VINDPM_DEF_uV	    4700000
#define SGM4154x_VINDPM_OS_MASK     GENMASK(1, 0)

/* DP DM SEL  */
#define SGM4154x_DP_VSEL_MASK       GENMASK(4, 3)
#define SGM4154x_DM_VSEL_MASK       GENMASK(2, 1)
/* PUMPX SET  */
#define SGM4154x_EN_PUMPX           BIT(7)
#define SGM4154x_PUMPX_UP           BIT(6)
#define SGM4154x_PUMPX_DN           BIT(5)

/* customer define jeita paramter */
/*
#define JEITA_TEMP_ABOVE_T4_CV	0
#define JEITA_TEMP_T3_TO_T4_CV	4100000
#define JEITA_TEMP_T2_TO_T3_CV	4350000
#define JEITA_TEMP_T1_TO_T2_CV	4350000
#define JEITA_TEMP_T0_TO_T1_CV	0
#define JEITA_TEMP_BELOW_T0_CV	0
*/

#define JEITA_TEMP_ABOVE_T4_CC_CURRENT	0
#define JEITA_TEMP_T3_TO_T4_CC_CURRENT	1000000
#define JEITA_TEMP_T2_TO_T3_CC_CURRENT	2400000
#define JEITA_TEMP_T1_TO_T2_CC_CURRENT	2000000
#define JEITA_TEMP_T0_TO_T1_CC_CURRENT	0
#define JEITA_TEMP_BELOW_T0_CC_CURRENT	0

#define FFC_ITERM_500MA 500
#define FV_COMP_0_MV 0
#define FV_COMP_16_MV 16
#define FV_COMP_24_MV 24
#define FV_COMP_32_MV 32

/*
#define TEMP_T4_THRES  50
#define TEMP_T4_THRES_MINUS_X_DEGREE 47
#define TEMP_T3_THRES  45
#define TEMP_T3_THRES_MINUS_X_DEGREE 39
#define TEMP_T2_THRES  20
#define TEMP_T2_THRES_PLUS_X_DEGREE 16
#define TEMP_T1_THRES  0
#define TEMP_T1_THRES_PLUS_X_DEGREE 6
#define TEMP_T0_THRES  0
#define TEMP_T0_THRES_PLUS_X_DEGREE  0
#define TEMP_NEG_10_THRES 0
*/
enum sgm415xx_dev_id{
	SGM41541_ID,
	SGM41542_ID,
	SGM41513_ID,
	SGM41513A_ID,
	SGM41513D_ID,
	SGM41516_ID,
	SGM41516D_ID,
	SGM41543_ID,
	SGM41543D_ID,
};

struct sgm4154x_init_data {
	u32 ichg;	/* charge current		*/
	u32 ilim;	/* input current		*/
	u32 vreg;	/* regulation voltage		*/
	u32 iterm;	/* termination current		*/
	u32 iprechg;	/* precharge current		*/
	u32 vlim;	/* minimum system voltage limit */
	u32 max_ichg;
	u32 max_vreg;
};

struct sgm4154x_state {
	bool vsys_stat;
	bool therm_stat;
	bool online;	
	u8 chrg_stat;
	u8 vbus_status;

	bool chrg_en;
	bool hiz_en;
	bool term_en;
	bool vbus_gd;
	u8 chrg_type;
	u8 health;
	u8 chrg_fault;
	u8 ntc_fault;
};

struct sgm4154x_jeita {
	int jeita_temp_above_t4_cv;
	int jeita_temp_t3_to_t4_cv;
	int jeita_temp_t2_to_t3_cv;
	int jeita_temp_t1_to_t2_cv;
	int jeita_temp_t0_to_t1_cv;
	int jeita_temp_below_t0_cv;
	int jeita_temp_above_t4_cc_current;
	int jeita_temp_t3_to_t4_cc_current;
	int jeita_temp_t2_to_t3_cc_current;
	int jeita_temp_t1_to_t2_cc_current;
	int jeita_temp_below_t0_cc_current;
	int temp_t4_thres;
	int temp_t4_thres_minus_x_degree;
	int temp_t3_thres;
	int temp_t3_thres_minus_x_degree;
	int temp_t2_thres;
	int temp_t2_thres_plus_x_degree;
	int temp_t1_thres;
	int temp_t1_thres_plus_x_degree;
	int temp_t0_thres;
	int temp_t0_thres_plus_x_degree;
	int temp_neg_10_thres;
};

enum {
	SGM_POWER_SUPPLY_DP_DM_UNKNOWN = 0,
	SGM_POWER_SUPPLY_DP_DM_DP_PULSE = 1,
	SGM_POWER_SUPPLY_DP_DM_DM_PULSE = 2,
};

#define SGM_SYSFS_FIELD_RW(_name, _prop)	\
{									 \
	.attr	= __ATTR(_name, 0644, sgm_sysfs_show, sgm_sysfs_store),\
	.prop	= _prop,	\
	.set	= _name##_set,						\
	.get	= _name##_get,						\
}

#define SGM_SYSFS_FIELD_RO(_name, _prop)	\
{			\
	.attr   = __ATTR(_name, 0444, sgm_sysfs_show, sgm_sysfs_store),\
	.prop   = _prop,				  \
	.get	= _name##_get,						\
}

#define SGM_SYSFS_FIELD_WO(_name, _prop)	\
{								   \
	.attr	= __ATTR(_name, 0200, sgm_sysfs_show, sgm_sysfs_store),\
	.prop	= _prop,	\
	.set	= _name##_set,						\
}

enum sgm_property {
	SGM_PROP_CHARGING_ENABLED,
};

extern bool wt6670f_is_detect;
void sgm41543_enable_statpin(bool en);

#endif /* _SGM4154x_CHARGER_H */
