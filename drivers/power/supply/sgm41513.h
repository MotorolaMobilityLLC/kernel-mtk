#ifndef SGM41513_CHARGER_H
#define SGM41513_CHARGER_H

#include <linux/bits.h>
#include <linux/thermal.h>
#include <linux/iio/consumer.h>

#define R_VBUS_CHARGER_1   330
#define R_VBUS_CHARGER_2   39

#define SGM41513_TEMP_INVALID   1000

#define SGM41513_CHG_DISABLED     0
#define SGM41513_PRECHARGE        1
#define SGM41513_FAST_CHARGE      2
#define SGM41513_TERM_CHARGE      3

#define SGM41513_CHG_DISABLE      1
#define SGM41513_STATE_COLD       2
#define SGM41513_STATE_COOL       3
#define SGM41513_STATE_NORMAL     4
#define SGM41513_STATE_WARM       5

/*define register*/
#define SGM41513_CHRG_CTRL_0    0x00
#define SGM41513_CHRG_CTRL_1    0x01
#define SGM41513_CHRG_CTRL_2    0x02
#define SGM41513_CHRG_CTRL_3    0x03
#define SGM41513_CHRG_CTRL_4    0x04
#define SGM41513_CHRG_CTRL_5    0x05
#define SGM41513_CHRG_CTRL_6    0x06
#define SGM41513_CHRG_CTRL_7    0x07
#define SGM41513_CHRG_CTRL_8    0x08
#define SGM41513_CHRG_CTRL_9    0x09
#define SGM41513_CHRG_CTRL_A    0x0a
#define SGM41513_CHRG_CTRL_B    0x0b
#define SGM41513_CHRG_CTRL_C    0x0c
#define SGM41513_CHRG_CTRL_D    0x0d
#define SGM41513_CHRG_CTRL_E    0x0e
#define SGM41513_CHRG_CTRL_F    0x0f

/* charge status flags  */
#define SGM41513_CHRG_EN         BIT(4)
#define SGM41513_HIZ_EN          BIT(7)
#define SGM41513_TERM_EN         BIT(7)
#define SGM41513_VAC_OVP_MASK    GENMASK(7, 6)
#define SGM41513_DPDM_ONGOING    BIT(7)
#define SGM41513_VBUS_GOOD       BIT(7)

#define SGM41513_OTG_EN          BIT(5)

/* Part ID  */
#define SGM41513_PN_MASK         GENMASK(6, 3)
#define SGM41513_PN_ID           0
#define SGM41513AD_PN_ID         BIT(3)
#define SGM41543_PN_ID           (BIT(6)| BIT(3))

/* register reset*/
#define SGM41513_REG_RESET       BIT(7)

/* WDT TIMER SET  */
#define SGM41513_WDT_TIMER_MASK       GENMASK(5, 4)
#define SGM41513_WDT_TIMER_DISABLE    0
#define SGM41513_WDT_TIMER_40S        BIT(4)
#define SGM41513_WDT_TIMER_80S        BIT(5)
#define SGM41513_WDT_TIMER_160S       (BIT(5)| BIT(4))

#define SGM41513_WDT_RST_MASK         BIT(6)

/* boost current set */
#define SGM41513_BOOST_CUR_0A5        0
#define SGM41513_BOOST_CUR_1A2        BIT(7)
#define SGM41513_BOOST_MASK           BIT(7)

/* safety timer set  */
#define SGM41513_SAFETY_TIMER_MASK       GENMASK(3, 3)
#define SGM41513_SAFETY_TIMER_DISABLE    0
#define SGM41513_SAFETY_TIMER_EN         BIT(3)
#define SGM41513_SAFETY_TIMER_5H         0
#define SGM41513_SAFETY_TIMER_10H        BIT(2)

/* recharge voltage  */
#define SGM41513_VRECHARGE            BIT(0)
#define SGM41513_VRECHRG_STEP_mV      100
#define SGM41513_VRECHRG_OFFSET_mV    100

/* charge status  */
#define SGM41513_VSYS_STAT        BIT(0)
#define SGM41513_THERM_STAT       BIT(1)
#define SGM41513_PG_STAT          BIT(2)
#define SGM41513_VBUS_STAT_OTG    GENMASK(7, 5)

/* charge status*/
#define SGM41513_VBUS_STAT_MASK    GENMASK(7, 5)
#define SGM41513_CHG_STAT_MASK     GENMASK(4, 3)
#define SGM41513_PG_GOOD_MASK      BIT(2)

/* termination current  */
#define SGM41513_TERMCHRG_CUR_MASK           GENMASK(3, 0)
#define SGM41513_TERMCHRG_CURRENT_STEP_MA    60
#define SGM41513_TERMCHRG_I_MIN_MA           60
#define SGM41513_TERMCHRG_I_MAX_MA           960
#define SGM41513_TERMCHRG_I_DEF_MA           180

/* precharge current  */
#define SGM41513_PRECHG_CUR_MASK           GENMASK(7, 4)
#define SGM41513_PRECHG_CURRENT_STEP_MA    60
#define SGM41513_PRECHG_I_MIN_MA           60

/* charge current  */
#define SGM41513_ICHRG_CUR_MASK           GENMASK(5, 0)
#define SGM41513_ICHRG_CURRENT_STEP_MA    60
#define SGM41513_ICHRG_I_MIN_MA           0
#define SGM41513_ICHRG_I_MAX_MA           3780
#define SGM41513_ICHRG_I_DEF_MA           2040
#define SGM41513_ICHRG_MAX_CFG            2400

/* charge voltage  */
#define SGM41513_VREG_V_MASK       GENMASK(7, 3)
#define SGM41513_VREG_V_MAX_MV     4624
#define SGM41513_VREG_V_MIN_MV     3856
#define SGM41513_VREG_V_DEF_MV     4208
#define SGM41513_VREG_V_STEP_MV    32

/* iindpm current  */
#define SGM41513_IINDPM_I_MASK      GENMASK(4, 0)
#define SGM41513_IINDPM_I_MIN_MA    100
#define SGM41513_IINDPM_I_MAX_MA    3800
#define SGM41513_IINDPM_STEP_MA     100
#define SGM41513_IINDPM_DEF_MA      2400

/* vindpm voltage  */
#define SGM41513_VINDPM_V_MASK      GENMASK(3, 0)
#define SGM41513_VINDPM_V_MIN_MV    3900
#define SGM41513_VINDPM_V_MAX_MV    12000
#define SGM41513_VINDPM_STEP_MV     100
#define SGM41513_VINDPM_DEF_MV      3600
#define SGM41513_VINDPM_OS_MASK     GENMASK(1, 0)

/* vreg fine tune */
#define SGM41513_VREG_FINE_TUNE_MASK         GENMASK(7, 6)
#define SGM41513_VREG_FINE_TUNE_DISABLE      0
#define SGM41513_VREG_FINE_TUNE_PLUS_8MV     BIT(6)
#define SGM41513_VREG_FINE_TUNE_MINUS_8MV    BIT(7)
#define SGM41513_VREG_FINE_TUNE_MINUS_16MV   (BIT(7) | BIT(6))

/* recharge voltage */
#define SGM41513_RECHARGE_VOLTAGE_MASK    BIT(0)
#define SGM41513_RECHARGE_VOLTAGE_100MV   0
#define SGM41513_RECHARGE_VOLTAGE_200MV   BIT(0)

/* vindpm track set voltage */
#define SGM41513_VINDPM_TRACK_SET_MASK      GENMASK(1, 0)
#define SGM41513_VINDPM_TRACK_SET_DISABLE   0
#define SGM41513_VINDPM_TRACK_SET_200MV     BIT(0)
#define SGM41513_VINDPM_TRACK_SET_250MV     BIT(1)
#define SGM41513_VINDPM_TRACK_SET_300MV     (BIT(1) | BIT(0))

#define SGM41513_DPM_INT_MASK      GENMASK(1, 0)
#define SGM41513_IINDPM_INT_MASK   BIT(0)
#define SGM41513_VINDPM_INT_MASK   BIT(1)

struct sgm41513_state {
    u32 vbus_stat;
    u32 chrg_stat;
    u32 pg_stat;
    u32 therm_stat;
    u32 vsys_stat;
    u32 wdt_fault;
    u32 boost_fault;
    u32 chrg_fault;
    u32 bat_fault;
    u32 ntc_fault;
};

struct sgm4154x_init_data {
    u32 ichg;    /* charge current        */
    u32 ilim;    /* input current        */
    u32 vreg;    /* regulation voltage        */
    u32 iterm;    /* termination current        */
    u32 iprechg;    /* precharge current        */
    u32 vlim;    /* minimum system voltage limit */
    u32 max_ichg;
    u32 max_vreg;
};

struct sgm41513_device {
    int temp;
    int ibat;
    int en_gpio;
    u32 reg_addr;
    int irq_gpio;
    int temp_state;
    int otg_mode;
    int chg_en;
    int vtemp;
    int voltage_max;
    int current_max;
    struct device *dev;
    struct iio_channel *vbus;
    struct i2c_client *client;
    struct sgm4154x_init_data init_data;
    struct sgm41513_state state;
    struct delayed_work irq_work;
    struct delayed_work psy_work;
    struct delayed_work charge_work;
    struct power_supply *ac_psy;
    struct power_supply *aca_psy;
    struct power_supply *usb_psy;
    struct power_supply *charger_psy;
    struct power_supply *battery_psy;
    struct power_supply_desc *chg_desc;
    struct regulator_dev *otg_rdev;
    struct charger_device *chg_dev;
    struct thermal_zone_device *tz_ap;
    struct thermal_zone_device *tz_rf;
    struct thermal_zone_device *tz_chg;
	struct thermal_zone_device *tz_batt;
};

#endif
