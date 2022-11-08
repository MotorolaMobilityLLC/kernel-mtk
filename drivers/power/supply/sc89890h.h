#ifndef SC89890H_CHARGER_H
#define SC89890H_CHARGER_H

#include <linux/bits.h>
#include <linux/thermal.h>
#include <linux/iio/consumer.h>

#define R_VBUS_CHARGER_1   330
#define R_VBUS_CHARGER_2   39

#define SC89890H_TEMP_INVALID   1000

#define SC89890H_CHG_DISABLED     0
#define SC89890H_PRECHARGE        1
#define SC89890H_FAST_CHARGE      2
#define SC89890H_TERM_CHARGE      3

#define QC_TYPE_AUTO_CHECK   0
#define QC_TYPE_FORCE_1P0    1
#define QC_TYPE_FORCE_2P0    2
#define QC_TYPE_FORCE_3P0    3
#define QUICK_CHARGE_1P0     4
#define QUICK_CHARGE_2P0     5
#define QUICK_CHARGE_3P0     6

#define SC89890H_USB_NON        0
#define SC89890H_USB_SDP        1
#define SC89890H_USB_CDP        2
#define SC89890H_USB_DCP        3
#define SC89890H_USB_HVDCP      4
#define SC89890H_USB_UNKNOWN    5
#define SC89890H_USB_NSTANDA    6
#define SC89890H_USB_OTG        7

#define SC89890H_CHG_DISABLE      1
#define SC89890H_STATE_COLD       2
#define SC89890H_STATE_COOL       3
#define SC89890H_STATE_NORMAL     4
#define SC89890H_STATE_WARM       5

/*define register*/
#define SC89890H_CHRG_CTRL_0    0x00
#define SC89890H_CHRG_CTRL_1    0x01
#define SC89890H_CHRG_CTRL_2    0x02
#define SC89890H_CHRG_CTRL_3    0x03
#define SC89890H_CHRG_CTRL_4    0x04
#define SC89890H_CHRG_CTRL_5    0x05
#define SC89890H_CHRG_CTRL_6    0x06
#define SC89890H_CHRG_CTRL_7    0x07
#define SC89890H_CHRG_CTRL_8    0x08
#define SC89890H_CHRG_CTRL_9    0x09
#define SC89890H_CHRG_CTRL_A    0x0a
#define SC89890H_CHRG_CTRL_B    0x0b
#define SC89890H_CHRG_CTRL_C    0x0c
#define SC89890H_CHRG_CTRL_D    0x0d
#define SC89890H_CHRG_CTRL_E    0x0e
#define SC89890H_CHRG_CTRL_F    0x0f
#define SC89890H_CHRG_CTRL_10   0x10
#define SC89890H_CHRG_CTRL_11   0x11
#define SC89890H_CHRG_CTRL_12   0x12
#define SC89890H_CHRG_CTRL_13   0x13
#define SC89890H_CHRG_CTRL_14   0x14

#define SC89890H_PN_MASK         GENMASK(5, 3)
#define SC89890H_PN_ID           BIT(5)

#define SC89890H_CHRG_EN           BIT(4)
#define SC89890H_HIZ_EN            BIT(7)
#define SC89890H_TERM_EN           BIT(7)
#define SC89890H_VAC_OVP_MASK      GENMASK(7, 6)
#define SC89890H_DPDM_ONGOING      BIT(7)
#define SC89890H_VBUS_GOOD         BIT(7)
#define SC89890H_OTG_CHG_MASK      GENMASK(5, 4)
#define SC89890H_OTG_ENABLE        BIT(5)
#define SC89890H_ILI_PIN_EN_MASK   BIT(6)
#define SC89890H_ILI_PIN_DISABLE   0
#define SC89890H_HVDCP_EN_MASK     BIT(3)
#define SC89890H_HVDCP_DISABLE     0

#define SC89890H_WDT_TIMER_MASK       GENMASK(5, 4)
#define SC89890H_WDT_TIMER_DISABLE    0
#define SC89890H_WDT_TIMER_40S        BIT(4)
#define SC89890H_WDT_TIMER_80S        BIT(5)
#define SC89890H_WDT_TIMER_160S       (BIT(5)| BIT(4))

#define SC89890H_BOOST_CUR_MASK     GENMASK(2, 0)
#define SC89890H_BOOST_CUR_500MA    0
#define SC89890H_BOOST_CUR_750MA    1
#define SC89890H_BOOST_CUR_1200MA   2
#define SC89890H_BOOST_CUR_1400MA   3
#define SC89890H_BOOST_CUR_1650MA   4
#define SC89890H_BOOST_CUR_1875MA   5
#define SC89890H_BOOST_CUR_2150MA   6
#define SC89890H_BOOST_CUR_2450MA   7

#define SC89890H_VSYS_STAT        BIT(0)
#define SC89890H_THERM_STAT       BIT(1)
#define SC89890H_PG_STAT          BIT(2)
#define SC89890H_VBUS_STAT_OTG    GENMASK(7, 5)

#define SC89890H_VBUS_STAT_MASK    GENMASK(7, 5)
#define SC89890H_CHG_STAT_MASK     GENMASK(4, 3)
#define SC89890H_VBUS_GOOD_MASK    BIT(7)

#define SC89890H_TERMCHRG_CUR_MASK           GENMASK(3, 0)
#define SC89890H_TERMCHRG_CURRENT_STEP_MA    60
#define SC89890H_TERMCHRG_I_MIN_MA           30

#define SC89890H_PRECHG_CUR_MASK           GENMASK(7, 4)
#define SC89890H_PRECHG_CURRENT_STEP_MA    60
#define SC89890H_PRECHG_I_MIN_MA           60

#define SC89890H_ICHRG_CUR_MASK           GENMASK(6, 0)
#define SC89890H_ICHRG_CURRENT_STEP_MA    60

#define SC89890H_VBAT_VREG_MASK       GENMASK(7, 2)
#define SC89890H_VBAT_VREG_MIN_MV     3840
#define SC89890H_VBAT_VREG_STEP_MV    16

#define SC89890H_IINDPM_I_MASK      GENMASK(5, 0)
#define SC89890H_IINDPM_I_MIN_MA    100
#define SC89890H_IINDPM_STEP_MA     50

#define SC89890H_DPM_FORCE_DET   BIT(1)

#define SC89890H_DP_VSEL_MASK    GENMASK(7, 5)
#define SC89890H_DM_VSEL_MASK    GENMASK(4, 2)
#define SC89890H_DPM_VSEL_MASK   GENMASK(7, 2)
#define SC89890H_DP_HIZ          0
#define SC89890H_DP_0V6          BIT(6)
#define SC89890H_DP_3V3          (BIT(7) | BIT(6))
#define SC89890H_DM_HIZ          0
#define SC89890H_DM_0V6          BIT(3)
#define SC89890H_DM_3V3          (BIT(4) | BIT(3))

#define SC89890H_RECHARGE_VOLTAGE_MASK    BIT(0)
#define SC89890H_RECHARGE_VOLTAGE_100MV   0
#define SC89890H_RECHARGE_VOLTAGE_200MV   BIT(0)

#define SC89890H_DPM_INT_MASK      GENMASK(1, 0)
#define SC89890H_IINDPM_INT_MASK   BIT(0)
#define SC89890H_VINDPM_INT_MASK   BIT(1)

struct sc89890h_state {
    u32 vbus_stat;
    u32 chrg_stat;
    u32 vbus_gd;
    u32 therm_stat;
    u32 vsys_stat;
    u32 wdt_fault;
    u32 boost_fault;
    u32 chrg_fault;
    u32 bat_fault;
    u32 ntc_fault;
};

struct sc89890h_init_data {
    u32 ichg;
    u32 ilim;
    u32 vreg;
    u32 iterm;
    u32 iprechg;
    u32 vlim;
    u32 max_ichg;
    u32 max_vreg;
};

struct sc89890h_device {
    int temp;
    int ibat;
    int chg_en_gpio;
    int otg_en_gpio;
    int qc_type;
    int qc_force;
    u32 reg_addr;
    int irq_gpio;
    int temp_state;
    int chg_volt;
    int chg_curr;
    int otg_mode;
    int chg_en;
    int vtemp;
    int voltage_max;
    int current_max;
    struct device *dev;
    struct iio_channel *vbus;
    struct i2c_client *client;
    struct sc89890h_init_data init_data;
    struct sc89890h_state state;
    struct delayed_work irq_work;
    struct delayed_work psy_work;
    struct delayed_work hvdcp_work;
    struct delayed_work charge_work;
    struct power_supply *ac_psy;
    struct power_supply *usb_psy;
    struct power_supply *charger_psy;
    struct power_supply *battery_psy;
    struct regulator_dev *otg_rdev;
    struct charger_device *chg_dev;
    struct power_supply_desc *chg_desc;
    struct thermal_zone_device *tz_ap;
    struct thermal_zone_device *tz_rf;
    struct thermal_zone_device *tz_chg;
    struct thermal_zone_device *tz_batt;
};

#endif
