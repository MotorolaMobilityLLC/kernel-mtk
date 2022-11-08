#ifndef ETA6963_CHARGER_H
#define ETA6963_CHARGER_H

#include <linux/bits.h>
#include <linux/thermal.h>
#include <linux/iio/consumer.h>

#define R_VBUS_CHARGER_1   330
#define R_VBUS_CHARGER_2   39

#define VBUS_MINUM_LEVEL       4000
#define ETA6963_TEMP_INVALID   0xffff

#define ETA6963_NOINPUT          0
#define ETA6963_VBUS_SDP         1
#define ETA6963_VBUS_CDP         2
#define ETA6963_VBUS_DCP         3
#define ETA6963_VBUS_UNKNOWN     5
#define ETA6963_VBUS_NOST        6
#define ETA6963_VBUS_OTG         7

#define ETA6963_CHG_DISABLED     0
#define ETA6963_PRECHARGE        1
#define ETA6963_FAST_CHARGE      2
#define ETA6963_TERM_CHARGE      3

#define ETA6963_CHG_DISABLE      1
#define ETA6963_STATE_COLD       2
#define ETA6963_STATE_COOL       3
#define ETA6963_STATE_NORMAL     4
#define ETA6963_STATE_WARM       5

/*define register*/
#define ETA6963_CHRG_CTRL_0    0x00
#define ETA6963_CHRG_CTRL_1    0x01
#define ETA6963_CHRG_CTRL_2    0x02
#define ETA6963_CHRG_CTRL_3    0x03
#define ETA6963_CHRG_CTRL_4    0x04
#define ETA6963_CHRG_CTRL_5    0x05
#define ETA6963_CHRG_CTRL_6    0x06
#define ETA6963_CHRG_CTRL_7    0x07
#define ETA6963_CHRG_CTRL_8    0x08
#define ETA6963_CHRG_CTRL_9    0x09
#define ETA6963_CHRG_CTRL_A    0x0a
#define ETA6963_CHRG_CTRL_B    0x0b

/* charge status flags  */
#define ETA6963_CHRG_EN         BIT(4)
#define ETA6963_HIZ_EN          BIT(7)
#define ETA6963_TERM_EN         BIT(7)
#define ETA6963_VAC_OVP_MASK    GENMASK(7, 6)
#define ETA6963_DPDM_ONGOING    BIT(7)
#define ETA6963_VBUS_GOOD       BIT(7)

#define ETA6963_OTG_EN          BIT(5)

/* Part ID  */
#define ETA6963_PN_MASK         GENMASK(6, 3)
#define ETA6963AD_PN_ID         BIT(3)
#define ETA6963_PN_ID           (BIT(5)| BIT(4)| BIT(3))

/* register reset*/
#define ETA6963_REG_RESET       BIT(7)

/* WDT TIMER SET  */
#define ETA6963_WDT_TIMER_MASK       GENMASK(5, 4)
#define ETA6963_WDT_TIMER_DISABLE    0
#define ETA6963_WDT_TIMER_40S        BIT(4)
#define ETA6963_WDT_TIMER_80S        BIT(5)
#define ETA6963_WDT_TIMER_160S       (BIT(5)| BIT(4))

#define ETA6963_WDT_RST_MASK         BIT(6)

/* boost current set */
#define ETA6963_BOOST_CUR_0A5        0
#define ETA6963_BOOST_CUR_1A2        BIT(7)
#define ETA6963_BOOST_MASK           BIT(7)

/* safety timer set  */
#define ETA6963_SAFETY_TIMER_MASK       GENMASK(3, 3)
#define ETA6963_SAFETY_TIMER_DISABLE    0
#define ETA6963_SAFETY_TIMER_EN         BIT(3)
#define ETA6963_SAFETY_TIMER_5H         0
#define ETA6963_SAFETY_TIMER_10H        BIT(2)

/* recharge voltage  */
#define ETA6963_VRECHARGE            BIT(0)
#define ETA6963_VRECHRG_STEP_mV      100
#define ETA6963_VRECHRG_OFFSET_mV    100

/* charge status  */
#define ETA6963_VSYS_STAT        BIT(0)
#define ETA6963_THERM_STAT       BIT(1)
#define ETA6963_PG_STAT          BIT(2)
#define ETA6963_VBUS_STAT_OTG    GENMASK(7, 5)

/* charge status*/
#define ETA6963_VBUS_STAT_MASK    GENMASK(7, 5)
#define ETA6963_CHG_STAT_MASK     GENMASK(4, 3)
#define ETA6963_PG_GOOD_MASK      BIT(2)

/* termination current  */
#define ETA6963_TERMCHRG_CUR_MASK           GENMASK(3, 0)
#define ETA6963_TERMCHRG_CURRENT_STEP_MA    60
#define ETA6963_TERMCHRG_I_MIN_MA           60
#define ETA6963_TERMCHRG_I_MAX_MA           960
#define ETA6963_TERMCHRG_I_DEF_MA           180

/* precharge current  */
#define ETA6963_PRECHG_CUR_MASK           GENMASK(7, 4)
#define ETA6963_PRECHG_CURRENT_STEP_MA    60
#define ETA6963_PRECHG_I_MIN_MA           60

/* charge current  */
#define ETA6963_ICHRG_CUR_MASK           GENMASK(5, 0)
#define ETA6963_ICHRG_CURRENT_STEP_MA    60
#define ETA6963_ICHRG_I_MIN_MA           0
#define ETA6963_ICHRG_I_MAX_MA           3780
#define ETA6963_ICHRG_I_DEF_MA           2040
#define ETA6963_ICHRG_MAX_CFG            2400

/* charge voltage  */
#define ETA6963_VREG_V_MASK       GENMASK(7, 3)
#define ETA6963_VREG_V_MAX_MV     4624
#define ETA6963_VREG_V_MIN_MV     3848
#define ETA6963_VREG_V_DEF_MV     4208
#define ETA6963_VREG_V_STEP_MV    32

/* iindpm current  */
#define ETA6963_IINDPM_I_MASK      GENMASK(4, 0)
#define ETA6963_IINDPM_I_MIN_MA    100
#define ETA6963_IINDPM_I_MAX_MA    3800
#define ETA6963_IINDPM_STEP_MA     100
#define ETA6963_IINDPM_DEF_MA      2400

/* vindpm voltage  */
#define ETA6963_VINDPM_V_MASK      GENMASK(3, 0)
#define ETA6963_VINDPM_V_MIN_MV    3900
#define ETA6963_VINDPM_V_MAX_MV    12000
#define ETA6963_VINDPM_STEP_MV     100
#define ETA6963_VINDPM_DEF_MV      3600
#define ETA6963_VINDPM_OS_MASK     GENMASK(1, 0)

/* recharge voltage */
#define ETA6963_RECHARGE_VOLTAGE_MASK    BIT(0)
#define ETA6963_RECHARGE_VOLTAGE_120MV   0
#define ETA6963_RECHARGE_VOLTAGE_240MV   BIT(0)

/* vindpm track set voltage */
#define ETA6963_VINDPM_TRACK_SET_MASK      GENMASK(1, 0)
#define ETA6963_VINDPM_TRACK_SET_DISABLE   0
#define ETA6963_VINDPM_TRACK_SET_200MV     BIT(0)
#define ETA6963_VINDPM_TRACK_SET_250MV     BIT(1)
#define ETA6963_VINDPM_TRACK_SET_300MV     (BIT(1) | BIT(0))

#define ETA6963_DPM_INT_MASK      GENMASK(1, 0)
#define ETA6963_IINDPM_INT_MASK   BIT(0)
#define ETA6963_VINDPM_INT_MASK   BIT(1)

struct eta6963_state {
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

struct eta6963_init_data {
    u32 ichg;    /* charge current        */
    u32 ilim;    /* input current        */
    u32 vreg;    /* regulation voltage        */
    u32 iterm;    /* termination current        */
    u32 iprechg;    /* precharge current        */
    u32 vlim;    /* minimum system voltage limit */
    u32 max_ichg;
    u32 max_vreg;
};

struct eta6963_device {
    int temp;
    int ibat;
    int en_gpio;
	int id_gpio;
    u32 reg_addr;
    int irq_gpio;
    int temp_state;
    int otg_mode;
    int chg_en;
    int vtemp;
    int voltage_max;
    int current_max;
    int vbus_voltage;
    int vbus_online;
    struct device *dev;
    struct iio_channel *vbus;
    struct i2c_client *client;
    struct eta6963_init_data init_data;
    struct eta6963_state state;
    struct delayed_work irq_work;
    struct delayed_work psy_work;
    struct delayed_work charge_work;
    struct delayed_work curr_work;
	struct delayed_work poff_work;
    struct power_supply *ac_psy;
    struct power_supply *aca_psy;
    struct power_supply *usb_psy;
    struct power_supply *battery_psy;
    struct power_supply *charger_psy;
    struct regulator_dev *otg_rdev;
};

#endif
