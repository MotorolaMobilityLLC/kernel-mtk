/* SPDX-License-Identifier: GPL-2.0-only */
// sgm4154x Charger Driver
// Copyright (C) 2021 Texas Instruments Incorporated - http://www.sg-micro.com

#ifndef _SY697X_CHARGER_H
#define _SY697X_CHARGER_H

#include <linux/i2c.h>

#define SY697X_MANUFACTURER	"Silergy Corp"

/*define register*/
#define SY697X_CHRG_CTRL_0	0x00
#define SY697X_CHRG_CTRL_1	0x01
#define SY697X_CHRG_CTRL_2	0x02
#define SY697X_CHRG_CTRL_3	0x03
#define SY697X_CHRG_CTRL_4	0x04
#define SY697X_CHRG_CTRL_5	0x05
#define SY697X_CHRG_CTRL_6	0x06
#define SY697X_CHRG_CTRL_7	0x07
#define SY697X_CHRG_CTRL_8	0x08
#define SY697X_CHRG_CTRL_9	0x09
#define SY697X_CHRG_CTRL_a	0x0a
#define SY697X_CHRG_CTRL_b	0x0b
#define SY697X_CHRG_CTRL_c	0x0c
#define SY697X_CHRG_CTRL_d	0x0d
#define SY697X_CHRG_CTRL_e   	0x0e
#define SY697X_CHRG_CTRL_f	0x0f
#define SY697X_CHRG_CTRL_10	0x10
#define SY697X_CHRG_CTRL_11	0x11
#define SY697X_CHRG_CTRL_12	0x12
#define SY697X_CHRG_CTRL_13	0x13
#define SY697X_CHRG_CTRL_14     0x14

/* Charging Termination Enable flags  */
#define SY6970X_EN_TERM_MASK        0x80
#define SY6970X_EN_TERM_SHIFT       7
#define SY6970X_TERM_ENABLE         1
#define SY6970X_TERM_DISABLE        0

/* charge enable bc1.2 */
#define SY697X_FORCE_DPDM_EN    BIT(1)
#define SY697X_CHG_DPDM_MASK    0x02
#define SY697X_CHG_DPDM_SHIFT   1

/* charge status flags  */
#define SY697X_CHRG_EN	BIT(4)
#define SY697X_HIZ_EN   BIT(7)
#define SY697X_HIZ_MASK   GENMASK(7, 7)
#define SY697X_TERM_EN	BIT(7)
//#define SGM4154x_VAC_OVP_MASK	GENMASK(7, 6)
//#define SGM4154x_DPDM_ONGOING   BIT(7)
#define SY697X_VBUS_GOOD        BIT(7)

/*otg*/
#define SY697X_BOOST_VOL_MASK         GENMASK(7, 4)
#define SY697X_BOOST_CUR_MASK         GENMASK(2, 0)
#define SY697X_OTG_EN			BIT(5)

/* Part ID  */
#define SY697X_PN_MASK	    GENMASK(5, 3)

/* WDT TIMER SET  */
#define SY697X_WDT_TIMER_MASK        GENMASK(5, 4)
#define SY697X_WDT_TIMER_DISABLE     0
#define SY697X_WDT_TIMER_40S         BIT(4)
#define SY697X_WDT_TIMER_80S         BIT(5)
#define SY697X_WDT_TIMER_160S        (BIT(4)| BIT(5))

#define SY697X_WDT_RST_MASK          BIT(6)

/* SAFETY TIMER SET  */
#define SY697X_SAFETY_TIMER_MASK     GENMASK(3, 3)
#define SY697X_SAFETY_TIMER_DISABLE     0
#define SY697X_SAFETY_TIMER_EN       BIT(3)
#define SY697X_SAFETY_TIMER_5H         0
#define SY697X_SAFETY_TIMER_10H      BIT(2)

/* recharge voltage  */
#define SY697X_VRECHARGE              	BIT(0)
#define SY697X_VRECHARGE_MAX_mV         200
#define SY697X_VRECHRG_STEP_mV		100
#define SY697X_VRECHRG_OFFSET_mV	100

/* charge status  */
#define SY697X_VSYS_STAT	BIT(0)
#define SY697X_THERM_STAT	BIT(1)
#define SY697X_PG_STAT		BIT(2)
#define SY697X_CHG_STAT_MASK	GENMASK(4, 3)
#define SY697X_PRECHRG		BIT(3)
#define SY697X_FAST_CHRG	BIT(4)
#define SY697X_TERM_CHRG	(BIT(3)| BIT(4))

/* charge type  */
#define SY697X_VBUS_STAT_MASK	GENMASK(7, 5)
#define SY697X_NOT_CHRGING	0
#define SY697X_USB_SDP		BIT(5)
#define SY697X_USB_CDP		BIT(6)
#define SY697X_USB_DCP		(BIT(5) | BIT(6))
#define SY697X_UNKNOWN	    	(BIT(7) | BIT(5))
#define SY697X_NON_STANDARD	(BIT(7) | BIT(6))
#define SY697X_OTG_MODE	    	(BIT(7) | BIT(6) | BIT(5))

/* TEMP Status  */
#define SY697X_TEMP_MASK	    GENMASK(2, 0)
#define SY697X_TEMP_NORMAL	BIT(0)
#define SY697X_TEMP_WARM	    BIT(1)
#define SY697X_TEMP_COOL	    (BIT(0) | BIT(1))
#define SY697X_TEMP_COLD	    (BIT(0) | BIT(3))
#define SY697X_TEMP_HOT	    (BIT(2) | BIT(3))

/* precharge current  */
#define SY697X_PRECHRG_CUR_MASK		GENMASK(7, 4)
#define SY697X_PRECHRG_CURRENT_STEP_uA		64000
#define SY697X_PRECHRG_I_MIN_uA		64000
#define SY697X_PRECHRG_I_MAX_uA		1024000
#define SY697X_PRECHRG_I_DEF_uA		128000

/* termination current  */
#define SY697X_TERMCHRG_CUR_MASK		GENMASK(3, 0)
#define SY697X_TERMCHRG_CURRENT_STEP_uA		64000
#define SY697X_TERMCHRG_I_MIN_uA		64000
#define SY697X_TERMCHRG_I_MAX_uA		1024000
#define SY697X_TERMCHRG_I_DEF_uA		243000

/* charge current  */
#define SY697X_ICHRG_I_MASK		GENMASK(6, 0)
#define SY697X_ICHRG_I_MIN_uA		0
#define SY697X_ICHRG_I_STEP_uA		64000
#define SY697X_ICHRG_I_MAX_uA		5056000
#define SY697X_ICHRG_I_DEF_uA		2048000

/* charge voltage  */
#define SY697X_VREG_V_MASK	    GENMASK(7, 2)
#define SY697X_VREG_V_MAX_uV	    4608000
#define SY697X_VREG_V_MIN_uV	    3840000
#define SY697X_VREG_V_DEF_uV	    4450000
#define SY697X_VREG_V_STEP_uV	    16000

/* charger iindpm */
#define SY697x_APPLE_1A             1000000
#define SY697x_APPLE_2A             2000000
#define SY697x_APPLE_2P1A           2100000
#define SY697x_APPLE_2P4A           2400000

/* iindpm current  */
#define SY697X_IINDPM_I_MASK	  GENMASK(5, 0)
#define SY697X_IINDPM_I_MIN_uA    100000
#define SY697X_IINDPM_I_MAX_uA    3250000
#define SY697X_IINDPM_STEP_uA	  50000
#define SY697X_IINDPM_DEF_uA	  2400000

/* vindpm voltage  */
#define SY697X_VINDPM_V_MASK      GENMASK(6, 0)
#define SY697X_VINDPM_V_MIN_uV    3900000
#define SY697X_VINDPM_V_MAX_uV    15300000
#define SY697X_VINDPM_STEP_uV     100000
#define SY697X_VINDPM_DEF_uV	  4400000
#define SY697X_VINDPM_OS_MASK     BIT(7)

/* PUMPX SET  */
#define SY697X_EN_PUMPX           BIT(7)
#define SY697X_PUMPX_UP           BIT(1)
#define SY697X_PUMPX_DN           BIT(0)

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

#define CHARGERING_33W             33
struct sy697x_init_data {
	u32 ichg;	/* charge current		*/
	u32 ilim;	/* input current		*/
	u32 vreg;	/* regulation voltage		*/
	u32 iterm;	/* termination current		*/
	u32 iprechg;	/* precharge current		*/
	u32 vlim;	/* minimum system voltage limit */
	u32 max_ichg;
	u32 max_vreg;
};

struct sy697x_state {
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
	bool charge_enabled;
	u8 ntc_fault;
};

struct sy697x_jeita {
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

struct sy697x_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;	
	struct power_supply *usb;
	struct power_supply *ac;
	enum power_supply_usb_type psy_usb_type;
	struct mutex lock;
	struct mutex i2c_rw_lock;

	struct charger_consumer *chg_consumer;
	struct usb_phy *usb2_phy;
	struct usb_phy *usb3_phy;
	struct notifier_block usb_nb;
	struct work_struct usb_work;
	unsigned long usb_event;
	struct regmap *regmap;

	char model_name[I2C_NAME_SIZE];
	int device_id;
	int advanced;

	struct sy697x_init_data init_data;
	struct sy697x_state state;
	u32 watchdog_timer;
	#if 1//defined(CONFIG_MTK_GAUGE_VERSION) && (CONFIG_MTK_GAUGE_VERSION == 30)
	struct charger_device *chg_dev;
	#endif
	struct charger_properties sy697x_chg_props;
	struct regulator_dev *otg_rdev;

	struct delayed_work charge_detect_delayed_work;
	struct delayed_work charge_monitor_work;
	struct delayed_work apsd_work;
	struct notifier_block pm_nb;
	bool sy697x_suspend_flag;

	struct delayed_work psy_work;
	struct wakeup_source *charger_wakelock;
	bool enable_sw_jeita;
	struct sy697x_jeita data;
};

#endif /* _SGM4154x_CHARGER_H */

