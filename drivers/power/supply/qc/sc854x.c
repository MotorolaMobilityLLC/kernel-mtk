// SPDX-License-Identifier: GPL-2.0
/*
* Copyright (c) 2022 Southchip Semiconductor Technology(Shanghai) Co., Ltd.
*/

#define pr_fmt(fmt) "[sc854x] %s: " fmt, __func__

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/debugfs.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/power_supply.h>
#include "sc854x.h"

enum {
    SC8545_ID,
    SC8546_ID,
    SC8547_ID,
    SC8549_ID,
};

static int id_data[] = {
    [SC8545_ID] = 0x66,
    [SC8546_ID] = 0x67,
    [SC8547_ID] = 0x66,
    [SC8549_ID] = 0x66,
};

enum {
    SC8545_STANDALONG = 0,
    SC8545_MASTER,
    SC8545_SLAVE,
    SC8546_STANDALONG,
    SC8546_MASTER,
    SC8546_SLAVE,
    SC8547_STANDALONG,
    SC8547_MASTER,
    SC8547_SLAVE,
    SC8549_STANDALONG,
    SC8549_MASTER,
    SC8549_SLAVE,
};

static int sc854x_mode_data[] = {
    [SC8545_STANDALONG] = SC8545_STANDALONG,
    [SC8545_MASTER]     = SC8545_MASTER,
    [SC8545_SLAVE]      = SC8545_SLAVE,
    [SC8546_STANDALONG] = SC8546_STANDALONG,
    [SC8546_MASTER]     = SC8546_MASTER,
    [SC8546_SLAVE]      = SC8546_SLAVE,
    [SC8547_STANDALONG] = SC8547_STANDALONG,
    [SC8547_MASTER]     = SC8547_MASTER,
    [SC8547_SLAVE]      = SC8547_SLAVE,
    [SC8549_STANDALONG] = SC8549_STANDALONG,
    [SC8549_MASTER]     = SC8549_MASTER,
    [SC8549_SLAVE]      = SC8549_SLAVE,
};

typedef enum{
    STANDALONE = 0,
    MASTER,
    SLAVE,
}WORK_MODE;

typedef enum {
    ADC_IBUS,
    ADC_VBUS,
    ADC_VAC,
    ADC_VOUT,
    ADC_VBAT,
    ADC_IBAT,
    ADC_TDIE,
    ADC_MAX_NUM,
}ADC_CH;

#define sc_err(fmt, ...)    \
do {                    \
    if (sc->mode == STANDALONE) \
        printk(KERN_ERR "[sc854x-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else if (sc->mode == MASTER) \
        printk(KERN_ERR "[sc854x-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else \
        printk(KERN_ERR "[sc854x-SLAVE]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define sc_info(fmt, ...)   \
do {                        \
    if (sc->mode == STANDALONE) \
        printk(KERN_INFO "[sc854x-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else if (sc->mode == MASTER) \
        printk(KERN_INFO "[sc854x-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else \
        printk(KERN_INFO "[sc854x-SLAVE]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);

#define sc_dbg(fmt, ...)    \
do {                    \
    if (sc->mode == STANDALONE) \
        printk(KERN_DEBUG "[sc854x-STANDALONE]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else if (sc->mode == MASTER) \
        printk(KERN_DEBUG "[sc854x-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);\
    else \
        printk(KERN_DEBUG "[sc854x-MASTER]:%s:" fmt, __func__, ##__VA_ARGS__);\
} while(0);


struct sc854x_cfg {
    bool bat_ovp_disable;
    bool bat_ocp_disable;
    bool vdrop_ovp_disable;

    int bat_ovp_th;
    int bat_ocp_th;

    bool bus_ovp_disable;
    bool bus_ocp_disable;
    bool bus_ucp_disable;

    int bus_ovp_th;
    int bus_ocp_th;

    int ac_ovp_th;

    int sense_r_mohm;
};

struct sc854x {
    struct notifier_block pm_nb;
    bool sc854x_suspend_flag;
    struct device *dev;
    struct i2c_client *client;
    
    WORK_MODE mode;

    int irq_gpio;
    int irq;

    struct mutex data_lock;
    struct mutex i2c_rw_lock;
    struct mutex irq_complete;

    bool irq_waiting;
    bool irq_disabled;
    bool resume_completed;

    bool batt_present;
    bool vbus_present;

    bool usb_present;
    bool charge_enabled;    /* Register bit status */

    int vbus_error;
    int charger_mode;
    int direct_charge;

    /* ADC reading */
    int vbat_volt;
    int vbus_volt;
    int vout_volt;
    int vac_volt;

    int ibat_curr;
    int ibus_curr;

    int die_temp;

    struct sc854x_cfg *cfg;

    int skip_writes;
    int skip_reads;

    struct sc854x_platform_data *platform_data;

    struct power_supply_desc psy_desc;
    struct power_supply_config psy_cfg;
    struct power_supply *fc2_psy;
};

extern void usbqc_suspend_notifier(bool suspend);
/************************************************************************/
static int __sc854x_read_word(struct sc854x *sc, u8 reg, u16 *data)
{
    s32 ret;

    ret = i2c_smbus_read_word_data(sc->client, reg);
    if (ret < 0) {
        sc_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u16) ret;

    return 0;
}

static int __sc854x_read_byte(struct sc854x *sc, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_byte_data(sc->client, reg);
    if (ret < 0) {
        sc_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u8) ret;

    return 0;
}

static int __sc854x_write_byte(struct sc854x *sc, int reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(sc->client, reg, val);
    if (ret < 0) {
        sc_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
            val, reg, ret);
        return ret;
    }
    return 0;
}

static int sc854x_read_byte(struct sc854x *sc, u8 reg, u8 *data)
{
    int ret;

    if (sc->skip_reads) {
        *data = 0;
        return 0;
    }

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc854x_read_byte(sc, reg, data);
    mutex_unlock(&sc->i2c_rw_lock);

    return ret;
}

static int sc854x_write_byte(struct sc854x *sc, u8 reg, u8 data)
{
    int ret;

    if (sc->skip_writes)
        return 0;

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc854x_write_byte(sc, reg, data);
    mutex_unlock(&sc->i2c_rw_lock);

    return ret;
}

static int sc854x_read_word(struct sc854x *sc, u8 reg, u16 *data)
{
    int ret;

    if (sc->skip_reads) {
        *data = 0;
        return 0;
    }

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc854x_read_word(sc, reg, data);
    mutex_unlock(&sc->i2c_rw_lock);

    return ret;
}

static int sc854x_update_bits(struct sc854x*sc, u8 reg,
                    u8 mask, u8 data)
{
    int ret;
    u8 tmp;

    if (sc->skip_reads || sc->skip_writes)
        return 0;

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc854x_read_byte(sc, reg, &tmp);
    if (ret) {
        sc_err("Failed: reg=%02X, ret=%d\n", reg, ret);
        goto out;
    }

    tmp &= ~mask;
    tmp |= data & mask;

    ret = __sc854x_write_byte(sc, reg, tmp);
    if (ret)
        sc_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
    mutex_unlock(&sc->i2c_rw_lock);
    return ret;
}

/*********************************************************************/
static int sc854x_enable_batovp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_BAT_OVP_ENABLE;
    else
        val = SC854X_BAT_OVP_DISABLE;

    val <<= SC854X_BAT_OVP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_00,
                SC854X_BAT_OVP_DIS_MASK, val);
}

static int sc854x_set_batovp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_BAT_OVP_BASE)
        threshold = SC854X_BAT_OVP_BASE;

    val = (threshold - SC854X_BAT_OVP_BASE) / SC854X_BAT_OVP_LSB;

    val <<= SC854X_BAT_OVP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_00,
                SC854X_BAT_OVP_MASK, val);
}

static int sc854x_enable_batocp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_BAT_OCP_ENABLE;
    else
        val = SC854X_BAT_OCP_DISABLE;

    val <<= SC854X_BAT_OCP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_01,
                SC854X_BAT_OCP_DIS_MASK, val);
}

static int sc854x_set_batocp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_BAT_OCP_BASE)
        threshold = SC854X_BAT_OCP_BASE;

    val = (threshold - SC854X_BAT_OCP_BASE) / SC854X_BAT_OCP_LSB;

    val <<= SC854X_BAT_OCP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_01,
                SC854X_BAT_OCP_MASK, val);
}

static int sc854x_set_acovp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_AC_OVP_BASE)
        threshold = SC854X_AC_OVP_BASE;

    if (threshold == SC854X_AC_OVP_6P5V)
        val = 0x07;
    else
        val = (threshold - SC854X_AC_OVP_BASE) /  SC854X_AC_OVP_LSB;

    val <<= SC854X_AC_OVP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_02,
                SC854X_AC_OVP_MASK, val);
}

static int sc854x_enable_dropovp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_VDROP_OVP_ENABLE;
    else
        val = SC854X_VDROP_OVP_DISABLE;

    val <<= SC854X_VDROP_OVP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_03,
                SC854X_VDROP_OVP_DIS_MASK, val);
}

static int sc854x_set_vdrop_ovp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold == 400)
        val = SC854X_VDROP_OVP_THRESHOLD_400MV;
    else
        val = SC854X_VDROP_OVP_THRESHOLD_300MV;
    
    val <<= SC854X_VDROP_OVP_THRESHOLD_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_03,
                SC854X_VDROP_OVP_THRESHOLD_MASK, val);
}

static int sc854x_set_vdrop_deglitch(struct sc854x *sc, int deglitch)
{
    u8 val;

    if (deglitch == 5000)
        val = SC854X_VDROP_DEGLITCH_SET_5MS;
    else
        val = SC854X_VDROP_DEGLITCH_SET_8US;
    
    val <<= SC854X_VDROP_DEGLITCH_SET_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_03,
                SC854X_VDROP_DEGLITCH_SET_MASK, val);
}

static int sc854x_enable_busovp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_VBUS_OVP_ENABLE;
    else
        val = SC854X_VBUS_OVP_DISABLE;

    val <<= SC854X_VBUS_OVP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_04,
                SC854X_VBUS_OVP_DIS_MASK, val);
}

static int sc854x_set_busovp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_VBUS_OVP_BASE)
        threshold = SC854X_VBUS_OVP_BASE;

    val = (threshold - SC854X_VBUS_OVP_BASE) / SC854X_VBUS_OVP_LSB;

    val <<= SC854X_VBUS_OVP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_04,
                SC854X_VBUS_OVP_MASK, val);
}

static int sc854x_enable_busucp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_IBUS_UCP_ENABLE;
    else
        val = SC854X_IBUS_UCP_DISABLE;

    val <<= SC854X_IBUS_UCP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_05,
                SC854X_IBUS_UCP_DIS_MASK, val);
}

static int sc854x_enable_busocp(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_IBUS_OCP_ENABLE;
    else
        val = SC854X_IBUS_OCP_DISABLE;

    val <<= SC854X_IBUS_OCP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_05,
                SC854X_IBUS_OCP_DIS_MASK, val);
}

static int sc854x_enable_deglitch(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_IBUS_UCP_FALL_DEG_SET_5MS;
    else
        val = SC854X_IBUS_UCP_FALL_DEG_SET_10US;

    val <<= SC854X_IBUS_UCP_FALL_DEG_SET_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_05,
                SC854X_IBUS_UCP_FALL_DEG_SET_MASK, val);
}

static int sc854x_set_busocp_th(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_IBUS_OCP_BASE)
        threshold = SC854X_IBUS_OCP_BASE;

    val = (threshold - SC854X_IBUS_OCP_BASE) / SC854X_IBUS_OCP_LSB;

    val <<= SC854X_IBUS_OCP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_05,
                SC854X_IBUS_OCP_MASK, val);
}

static int sc854x_check_vbus_error_status(struct sc854x *sc, int *result)
{
    u8 ret;
    u8 reg;
    ret = sc854x_read_byte(sc, SC854X_REG_06, &reg);

    *result = (int)reg;
    return ret;
}

static int sc854x_enable_charge(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_CHG_ENABLE;
    else
        val = SC854X_CHG_DISABLE;

    val <<= SC854X_CHG_EN_SHIFT;

    sc_err("sc854x charger %s\n", enable == false ? "disable" : "enable");
    return sc854x_update_bits(sc, SC854X_REG_07,
                SC854X_CHG_EN_MASK, val);
}

static int sc854x_check_charge_enabled(struct sc854x *sc, bool *enabled)
{
    int ret;
    u8 val;

    ret = sc854x_read_byte(sc, SC854X_REG_07, &val);
    if (!ret) {
        sc_info(">>>reg [0x07] = 0x%02x\n", val);
        if (val & SC854X_CHG_EN_MASK) {
            ret = sc854x_read_byte(sc, SC854X_REG_06, &val);
            if (!ret) {
                sc_info(">>>reg [0x06] = 0x%02x\n", val);
                if (val & SC854X_CP_SWITCHING_STAT_MASK) {
                    *enabled = true;
                    return ret;
                }
            }
        }
    }

    *enabled = false;
    return ret;
}

static int sc854x_reg_reset(struct sc854x *sc, bool enable)
{   
    u8 val;

    if (enable)
        val = SC854X_RESET_REG;
    else
        val = SC854X_NO_REG_RESET;

    val <<= SC854X_REG_RESET_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_07,
                SC854X_REG_RESET_MASK, val);
}

static int sc854x_set_ss_timeout(struct sc854x *sc, int timeout)
{
    u8 val;

    switch (timeout) {
    case 0:
        val = SC854X_SS_TIMEOUT_DISABLE;
        break;
    case 40:
        val = SC854X_SS_TIMEOUT_40MS;
        break;
    case 80:
        val = SC854X_SS_TIMEOUT_80MS;
        break;
    case 320:
        val = SC854X_SS_TIMEOUT_320MS;
        break;
    case 1280:
        val = SC854X_SS_TIMEOUT_1280MS;
        break;
    case 5120:
        val = SC854X_SS_TIMEOUT_5120MS;
        break;
    case 20480:
        val = SC854X_SS_TIMEOUT_20480MS;
        break;
    case 81920:
        val = SC854X_SS_TIMEOUT_81920MS;
        break;
    default:
        val = SC854X_SS_TIMEOUT_DISABLE;
        break;
    }

    val <<= SC854X_SS_TIMEOUT_SET_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_08,
                SC854X_SS_TIMEOUT_SET_MASK,
                val);
}

static int sc854x_reg_timeout_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_650MS_REG_TIMEOUT_ENABLE;
    else
        val = SC854X_650MS_REG_TIMEOUT_DISABLE;

    val <<= SC854X_REG_TIMEOUT_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_08,
                SC854X_REG_TIMEOUT_DIS_MASK, val);
}

static int sc854x_voutovp_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_VOUT_OVP_ENABLE;
    else
        val = SC854X_VOUT_OVP_DISABLE;

    val <<= SC854X_VOUT_OVP_DIS_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_08,
                SC854X_VOUT_OVP_DIS_MASK, val);
}

static int sc854x_set_sense_resistor(struct sc854x *sc, int r_mohm)
{
    u8 val;

    if (r_mohm == 2)
        val = SC854X_SET_IBAT_SNS_RES_2MHM;
    else if (r_mohm == 5)
        val = SC854X_SET_IBAT_SNS_RES_5MHM;
    else
        return -EINVAL;

    val <<= SC854X_SET_IBAT_SNS_RES_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_08,
                SC854X_SET_IBAT_SNS_RES_MASK,
                val);
}

static int sc854x_vbus_pd_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_VBUS_PD_ENABLE;
    else
        val = SC854X_VBUS_PD_DISABLE;

    val <<= SC854X_VBUS_PD_EN_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_08,
                SC854X_VBUS_PD_EN_MASK, val);
}

static int sc854x_set_charge_mode(struct sc854x *sc, u8 charge_mode)
{
    u8 val;

    if(charge_mode)
        val = SC854X_CHARGE_MODE_1_1;
    else
        val = SC854X_CHARGE_MODE_2_1;

    val <<= SC854X_CHARGE_MODE_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_09,
                SC854X_CHARGE_MODE_MASK, val);
}

static int sc854x_get_charge_mode(struct sc854x *sc, int *result)
{
    int ret;
    u8 val;

    ret = sc854x_read_byte(sc, SC854X_REG_09, &val);
    if(ret < 0) {
        return ret;
    }

    *result = (val >> SC854X_CHARGE_MODE_SHIFT) &
                            SC854X_CHARGE_MODE_MASK;

    return ret;
}

static int sc854x_set_wdt(struct sc854x *sc, int ms)
{
    u8 val;

    if (ms == 0)
        val = SC854X_WATCHDOG_DIS;
    else if (ms == 200)
        val = SC854X_WATCHDOG_200MS;
    else if (ms == 500)
        val = SC854X_WATCHDOG_500MS;
    else if (ms == 1000)
        val = SC854X_WATCHDOG_1S;
    else if (ms == 5000)
        val = SC854X_WATCHDOG_5S;
    else if (ms == 30000)
        val = SC854X_WATCHDOG_30S;
    else
        val = SC854X_WATCHDOG_DIS;

    val <<= SC854X_WATCHDOG_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_09,
                SC854X_WATCHDOG_MASK, val);
}

static int sc854x_vbat_regulation_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_VBAT_REG_ENABLE;
    else
        val = SC854X_VBAT_REG_DISABLE;

    val <<= SC854X_VBAT_REG_EN_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0A,
                SC854X_VBAT_REG_EN_MASK, val);
}

static int sc854x_set_vbat_regulation(struct sc854x *sc, int mv)
{
    u8 val;

    if (mv == 50)
        val = SC854X_SET_VBATREG_50MV;
    else if (mv == 100)
        val = SC854X_SET_VBATREG_100MV;
    else if (mv == 150)
        val = SC854X_SET_VBATREG_150MV;
    else if (mv == 200)
        val = SC854X_SET_VBATREG_200MV;
    else
        val = SC854X_SET_VBATREG_50MV;

    val <<= SC854X_SET_VBATREG_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0A,
                SC854X_SET_VBATREG_MASK, val);
}

static int sc854x_ibat_regulation_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_IBAT_REG_ENABLE;
    else
        val = SC854X_IBAT_REG_DISABLE;

    val <<= SC854X_IBAT_REG_EN_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0B,
                SC854X_IBAT_REG_EN_MASK, val);
}

static int sc854x_set_ibat_regulation(struct sc854x *sc, int ma)
{
    u8 val;

    if (ma == 200)
        val = SC854X_SET_IBATREG_200MA;
    else if (ma == 300)
        val = SC854X_SET_IBATREG_300MA;
    else if (ma == 450)
        val = SC854X_SET_IBATREG_400MA;
    else if (ma == 500)
        val = SC854X_SET_IBATREG_500MA;
    else
        val = SC854X_SET_IBATREG_200MA;

    val <<= SC854X_SET_VBATREG_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0B,
                SC854X_SET_VBATREG_MASK, val);
}

static int sc854x_ibus_regulation_enabled(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_IBUS_REG_ENABLE;
    else
        val = SC854X_IBUS_REG_DISABLE;

    val <<= SC854X_IBUS_REG_EN_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0C,
                SC854X_IBUS_REG_EN_MASK, val);
}

static int sc854x_set_ibus_regulation(struct sc854x *sc, int threshold)
{
    u8 val;

    if (threshold < SC854X_SET_IBUSREG_BASE)
        threshold = SC854X_SET_IBUSREG_BASE;

    val = (threshold - SC854X_SET_IBUSREG_BASE) / SC854X_SET_IBUSREG_LSB;

    val <<= SC854X_SET_IBUSREG_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0C,
                SC854X_SET_IBUSREG_MASK, val);
}

static int sc854x_set_pmid2out_uvp(struct sc854x *sc, int mv)
{
    u8 val;

    if (mv == -50)
        val = SC854X_PMID2OUT_UVP_U_50MV;
    else if (mv == -100)
        val = SC854X_PMID2OUT_UVP_U_100MV;
    else if (mv == -150)
        val = SC854X_PMID2OUT_UVP_U_150MV;
    else if (mv == -200)
        val = SC854X_PMID2OUT_UVP_U_200MV;
    else
        val = SC854X_PMID2OUT_UVP_U_50MV;

    val <<= SC854X_PMID2OUT_UVP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0D,
                SC854X_PMID2OUT_UVP_MASK, val);
}

static int sc854x_set_pmid2out_ovp(struct sc854x *sc, int mv)
{
    u8 val;

    if (mv == 200)
        val = SC854X_PMID2OUT_OVP_200MV;
    else if (mv == 300)
        val = SC854X_PMID2OUT_OVP_300MV;
    else if (mv == 400)
        val = SC854X_PMID2OUT_OVP_400MV;
    else if (mv == 500)
        val = SC854X_PMID2OUT_OVP_500MV;
    else
        val = SC854X_PMID2OUT_OVP_200MV;

    val <<= SC854X_PMID2OUT_OVP_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_0D,
                SC854X_PMID2OUT_OVP_MASK, val);
}

static int sc854x_enable_adc(struct sc854x *sc, bool enable)
{
    u8 val;

    if (enable)
        val = SC854X_ADC_ENABLE;
    else
        val = SC854X_ADC_DISABLE;

    val <<= SC854X_ADC_EN_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_11,
                SC854X_ADC_EN_MASK, val);
}

static int sc854x_set_adc_scanrate(struct sc854x *sc, bool oneshot)
{
    u8 val;

    if (oneshot)
        val = SC854X_ADC_RATE_ONESHOT;
    else
        val = SC854X_ADC_RATE_CONTINOUS;

    val <<= SC854X_ADC_RATE_SHIFT;

    return sc854x_update_bits(sc, SC854X_REG_11,
                SC854X_ADC_EN_MASK, val);
}

static int sc854x_set_adc_scan(struct sc854x *sc, int channel, bool enable)
{
    u8 mask;
    u8 shift;
    u8 val;

    if (channel > ADC_MAX_NUM)
        return -EINVAL;

    shift = SC854X_ADC_DIS_SHIFT - channel;
    mask = SC854X_ADC_DIS_MASK << shift;

    if (enable)
        val = SC854X_ADC_CHANNEL_ENABLE << shift;
    else
        val = SC854X_ADC_CHANNEL_DISABLE << shift;

    return sc854x_update_bits(sc, SC854X_REG_12, mask, val);
}

static int sc854x_get_adc_data(struct sc854x *sc, int channel,  int *result)
{
    int ret;
    u16 val;
    u16 t;

    if(channel >= ADC_MAX_NUM) return 0;

    ret = sc854x_read_word(sc, SC854X_REG_13 + (channel << 1), &val);
    if (ret < 0)
        return ret;

    pr_err("%s channel 0x%x value is %d\n", __func__, channel, val);
    t = val & 0xFF;
    t <<= 8;
    t |= (val >> 8) & 0xFF;
    val = t;

    switch (channel)
    {
    case ADC_IBUS:
        val = val * SC854X_IBUS_ADC_LSB;
        break;
    case ADC_VBUS:
        val = val * SC854X_VBUS_ADC_LSB;
        break;
    case ADC_VAC:
        val = val * SC854X_VAC_ADC_LSB;
        break;
    case ADC_VOUT:
        val = val * SC854X_VOUT_ADC_LSB;
        break;
    case ADC_VBAT:
        val = val * SC854X_VBAT_ADC_LSB;
        break;
    case ADC_IBAT:
        val = val * SC854X_IBAT_ADC_LSB;
        break;
    case ADC_TDIE:
        val = val * SC854X_TDIE_ADC_LSB;
        break;
    default:
        sc_err("Not find adc channel : %d\n", channel);
        break;
    }
    pr_err("%s channel 0x%x actual value is %d\n", __func__, channel, val);
    *result = val;

    return 0;
}

static int sc854x_disable_vbus_range(struct sc854x *sc)
{
    return sc854x_update_bits(sc, SC854X_REG_3C,
                SC854X_VBUS_IN_RANGE_DIS_MASK, 
                SC854X_VBUS_EN_RANGE_DISABLE << SC854X_VBUS_IN_RANGE_DIS_SHIFT);
}

static int sc854x_dump_register(struct sc854x *sc)
{
    int idx;
    u8 data;
    int ret;
    for(idx = 0; idx < 0x20; idx++) {
        ret = sc854x_read_byte(sc, idx, &data);
        pr_err("%s reg 0x%x value is %d\n", __func__, idx, data);
    }
    return 0;
}

//*********************************************************************

/*
* interrupt does nothing, just info event chagne, other module could get info
* through power supply interface
*/
static void sc854x_check_fault_status(struct sc854x *sc);
static irqreturn_t sc854x_charger_interrupt(int irq, void *dev_id)
{
    struct sc854x *sc = dev_id;

    sc_dbg("INT OCCURED\n");
#if 1
    mutex_lock(&sc->irq_complete);
    sc->irq_waiting = true;
    if (!sc->resume_completed) {
        dev_dbg(sc->dev, "IRQ triggered before device-resume\n");
        if (!sc->irq_disabled) {
            disable_irq_nosync(irq);
            sc->irq_disabled = true;
        }
        mutex_unlock(&sc->irq_complete);
        return IRQ_HANDLED;
    }
    sc->irq_waiting = false;
#if 1
    /* TODO */
    sc854x_check_fault_status(sc);
#endif

#if 1
    sc854x_dump_register(sc);
#endif
    mutex_unlock(&sc->irq_complete);
#endif
    power_supply_changed(sc->fc2_psy);

    return IRQ_HANDLED;
}

static int sc854x_set_work_mode(struct sc854x *sc, int mode)
{
    switch (mode)
    {
    case SC8545_STANDALONG:
    case SC8546_STANDALONG:
    case SC8547_STANDALONG:
    case SC8549_STANDALONG:
        sc->mode = STANDALONE;
        break;
    case SC8545_MASTER:
    case SC8546_MASTER:
    case SC8547_MASTER:
    case SC8549_MASTER:
        sc->mode = MASTER;
        break;
    case SC8545_SLAVE:
    case SC8546_SLAVE:
    case SC8547_SLAVE:
    case SC8549_SLAVE:
        sc->mode = SLAVE;
        break;
    default:
        sc_err("Not find work mode\n");
        return -ENODEV;
    }
    sc_err("[%s] work mode is %s\n", mode <= SC8545_SLAVE ?
        "SC8545" : (mode <= SC8546_SLAVE ? "SC8546" : (mode <= 
        SC8547_SLAVE ? "SC8547" : "SC8549")), sc->mode == STANDALONE 
        ? "standalone" : (sc->mode == MASTER ? "master" : "slave"));

    return 0;
}

static int sc854x_detect_device(struct sc854x *sc)
{
    int ret;
    u8 data;

    ret = sc854x_read_byte(sc, SC854X_REG_36, &data);
    if (ret == 0) {
        if(data != id_data[SC8545_ID] && data != id_data[SC8546_ID] 
            && data != id_data[SC8547_ID] && data != id_data[SC8549_ID]) {
            return -ENOMEM;
        }
    }

    return ret;
}

static int sc854x_parse_dt(struct sc854x *sc, struct device *dev)
{
    int ret;
    struct device_node *np = dev->of_node;

    sc->cfg = devm_kzalloc(dev, sizeof(struct sc854x_cfg),
                    GFP_KERNEL);

    if (!sc->cfg)
        return -ENOMEM;

    sc->cfg->bat_ovp_disable = of_property_read_bool(np,
            "sc,sc854x,bat-ovp-disable");
    sc->cfg->bat_ocp_disable = of_property_read_bool(np,
            "sc,sc854x,bat-ocp-disable");
    sc->cfg->vdrop_ovp_disable = of_property_read_bool(np,
            "sc,sc854x,vdrop-ovp-disable");
    sc->cfg->bus_ovp_disable = of_property_read_bool(np,
            "sc,sc854x,bus-ovp-disable");
    sc->cfg->bus_ucp_disable = of_property_read_bool(np,
            "sc,sc854x,bus-ucp-disable");
    sc->cfg->bus_ocp_disable = of_property_read_bool(np,
            "sc,sc854x,bus-ocp-disable");

    ret = of_property_read_u32(np, "sc,sc854x,bat-ovp-threshold",
            &sc->cfg->bat_ovp_th);
    if (ret) {
        sc_err("failed to read bat-ovp-threshold\n");
        return ret;
    }
    ret = of_property_read_u32(np, "sc,sc854x,bat-ocp-threshold",
            &sc->cfg->bat_ocp_th);
    if (ret) {
        sc_err("failed to read bat-ocp-threshold\n");
        return ret;
    }
    ret = of_property_read_u32(np, "sc,sc854x,ac-ovp-threshold",
            &sc->cfg->ac_ovp_th);
    if (ret) {
        sc_err("failed to read ac-ovp-threshold\n");
        return ret;
    }
    ret = of_property_read_u32(np, "sc,sc854x,bus-ovp-threshold",
            &sc->cfg->bus_ovp_th);
    if (ret) {
        sc_err("failed to read bus-ovp-threshold\n");
        return ret;
    }
    ret = of_property_read_u32(np, "sc,sc854x,bus-ocp-threshold",
            &sc->cfg->bus_ocp_th);
    if (ret) {
        sc_err("failed to read bus-ocp-threshold\n");
        return ret;
    }

    ret = of_property_read_u32(np, "sc,sc854x,sense-resistor-mohm",
            &sc->cfg->sense_r_mohm);
    if (ret) {
        sc_err("failed to read sense-resistor-mohm\n");
        return ret;
    }

    return 0;
}

static int sc854x_init_protection(struct sc854x *sc)
{
    int ret;

    ret = sc854x_enable_batovp(sc, !sc->cfg->bat_ovp_disable);
    sc_info("%s bat ovp %s\n",
        sc->cfg->bat_ovp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    ret = sc854x_enable_batocp(sc, !sc->cfg->bat_ocp_disable);
    sc_info("%s bat ocp %s\n",
        sc->cfg->bat_ocp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    ret = sc854x_enable_dropovp(sc, !sc->cfg->vdrop_ovp_disable);
    sc_info("%s drop ovp %s\n",
        sc->cfg->vdrop_ovp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    ret = sc854x_enable_busovp(sc, !sc->cfg->bus_ovp_disable);
    sc_info("%s bus ovp %s\n",
        sc->cfg->bus_ovp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    ret = sc854x_enable_busucp(sc, !sc->cfg->bus_ucp_disable);
    sc_info("%s bus ucp %s\n",
        sc->cfg->bus_ucp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    ret = sc854x_enable_busocp(sc, !sc->cfg->bus_ocp_disable);
    sc_info("%s bus ocp %s\n",
        sc->cfg->bus_ocp_disable ? "disable" : "enable",
        !ret ? "successfullly" : "failed");

    sc854x_voutovp_enabled(sc, true);

    ret = sc854x_set_batovp_th(sc, sc->cfg->bat_ovp_th);
    sc_info("set bat ovp th %d %s\n", sc->cfg->bat_ovp_th,
        !ret ? "successfully" : "failed");

    ret = sc854x_set_batocp_th(sc, sc->cfg->bat_ocp_th);
    sc_info("set bat ocp threshold %d %s\n", sc->cfg->bat_ocp_th,
        !ret ? "successfully" : "failed");

    ret = sc854x_set_acovp_th(sc, sc->cfg->ac_ovp_th);
    sc_info("set ac ovp threshold %d %s\n", sc->cfg->ac_ovp_th,
        !ret ? "successfully" : "failed");

    ret = sc854x_set_busovp_th(sc, sc->cfg->bus_ovp_th);
    sc_info("set bus ovp threshold %d %s\n", sc->cfg->bus_ovp_th,
        !ret ? "successfully" : "failed");

    ret = sc854x_set_busocp_th(sc, sc->cfg->bus_ocp_th);
    sc_info("set bus ocp threshold %d %s\n", sc->cfg->bus_ocp_th,
        !ret ? "successfully" : "failed");

    ret = sc854x_set_sense_resistor(sc, sc->cfg->sense_r_mohm);
    sc_info("set sense r mohm %d %s\n", sc->cfg->sense_r_mohm,
        !ret ? "successfully" : "failed");

    return 0;
}

static int sc854x_init_adc(struct sc854x *sc)
{

    sc854x_set_adc_scanrate(sc, false);
    sc854x_set_adc_scan(sc, ADC_IBUS, true);
    sc854x_set_adc_scan(sc, ADC_VBUS, true);
    sc854x_set_adc_scan(sc, ADC_VOUT, true);
    sc854x_set_adc_scan(sc, ADC_VBAT, true);
    sc854x_set_adc_scan(sc, ADC_IBAT, true);
    sc854x_set_adc_scan(sc, ADC_TDIE, true);
    sc854x_set_adc_scan(sc, ADC_VAC, true);

    sc854x_enable_adc(sc, true);

    return 0;
}

static int sc854x_init_regulation(struct sc854x *sc)
{
    sc854x_set_ibat_regulation(sc, 300);
    sc854x_set_vbat_regulation(sc, 50);
    sc854x_set_ibus_regulation(sc, 2400);

    sc854x_reg_timeout_enabled(sc, true);

    sc854x_set_vdrop_deglitch(sc, 5000);
    sc854x_set_vdrop_ovp_th(sc, 400);

    sc854x_ibat_regulation_enabled(sc, false);
    sc854x_vbat_regulation_enabled(sc, false);
    sc854x_ibus_regulation_enabled(sc, false);

    return 0;
}

static int sc854x_init_device(struct sc854x *sc)
{
    sc854x_reg_reset(sc, true);

    sc854x_set_ss_timeout(sc, 0);
    sc854x_set_sense_resistor(sc, sc->cfg->sense_r_mohm);

    sc854x_init_protection(sc);
    sc854x_init_adc(sc);
    sc854x_enable_deglitch(sc, true);

    sc854x_set_wdt(sc, 0);//disable WTD
    sc854x_vbus_pd_enabled(sc, false);

    sc854x_set_pmid2out_uvp(sc, -100);
    sc854x_set_pmid2out_ovp(sc, 500);

    sc854x_init_regulation(sc);

    if (sc->mode == SLAVE) {
        sc854x_disable_vbus_range(sc);
    }
    sc854x_dump_register(sc);
    return 0;
}


static int sc854x_set_present(struct sc854x *sc, bool present)
{
    sc->usb_present = present;

    if (present)
        sc854x_init_device(sc);
    return 0;
}

static ssize_t sc854x_show_registers(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    struct sc854x *sc = dev_get_drvdata(dev);
    u8 addr;
    u8 val;
    u8 tmpbuf[300];
    int len;
    int idx = 0;
    int ret;

    idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc854x");
    for (addr = 0x0; addr <= 0x3C; addr++) {
        if((addr < 0x24) || (addr > 0x2B && addr < 0x33)
            || addr == 0x36 || addr == 0x3C) {
            ret = sc854x_read_byte(sc, addr, &val);
            if (ret == 0) {
                len = snprintf(tmpbuf, PAGE_SIZE - idx,
                        "Reg[%.2X] = 0x%.2x\n", addr, val);
                memcpy(&buf[idx], tmpbuf, len);
                idx += len;
            }
        }
    }

    return idx;
}

static ssize_t sc854x_store_register(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct sc854x *sc = dev_get_drvdata(dev);
    int ret;
    unsigned int reg;
    unsigned int val;

    ret = sscanf(buf, "%x %x", &reg, &val);
    if (ret == 2 && reg <= 0x3C)
        sc854x_write_byte(sc, (unsigned char)reg, (unsigned char)val);

    return count;
}

static DEVICE_ATTR(registers, 0660, sc854x_show_registers, sc854x_store_register);

static void sc854x_create_device_node(struct device *dev)
{
    device_create_file(dev, &dev_attr_registers);
}

static enum power_supply_property sc854x_charger_props[] = {
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_CHARGING_ENABLED,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_SC_BATTERY_PRESENT,
    POWER_SUPPLY_PROP_SC_VBUS_PRESENT,
    POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE,
    POWER_SUPPLY_PROP_SC_BATTERY_CURRENT,
    POWER_SUPPLY_PROP_SC_BUS_VOLTAGE,
    POWER_SUPPLY_PROP_SC_BUS_CURRENT,
    POWER_SUPPLY_PROP_SC_DIE_TEMPERATURE,
    POWER_SUPPLY_PROP_SC_VBUS_ERROR_STATUS,
    POWER_SUPPLY_PROP_SC_CHARGE_MODE,
    POWER_SUPPLY_PROP_SC_DIRECT_CHARGE,
};


static int sc854x_charger_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
    struct sc854x *sc = power_supply_get_drvdata(psy);
    int result;
    bool charge_enabled;
    int ret;
    u8 reg_val;

    sc_dbg(">>>>>psp = %d\n", psp);

    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
        ret = sc854x_check_charge_enabled(sc, &charge_enabled);
        if(!ret) {
            sc->charge_enabled = charge_enabled;
            val->intval = charge_enabled;
        } else {
            val->intval = sc->charge_enabled;
        }
        break;
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = 0;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = sc->usb_present;
        break;
    case POWER_SUPPLY_PROP_SC_BATTERY_PRESENT:
        ret = sc854x_read_byte(sc, SC854X_REG_0E, &reg_val);
        if (!ret) {
            sc->batt_present  = !!(reg_val & SC854X_VBAT_INSERT_STAT_MASK);
            val->intval = sc->batt_present;
        } else {
            val->intval = sc->batt_present;
        }
        break;
    case POWER_SUPPLY_PROP_SC_VBUS_PRESENT:
        ret = sc854x_read_byte(sc, SC854X_REG_0E, &reg_val);
        if (!ret) {
            sc->vbus_present  = !!(reg_val & SC854X_ADAPTER_INSERT_STAT_MASK);
            val->intval = sc->vbus_present;
        } else {
            val->intval = sc->vbus_present;
        }
        break;
    case POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE:
        ret = sc854x_get_adc_data(sc, ADC_VBAT, &result);
        if (!ret) {
            sc->vbat_volt = result * 1000;
            val->intval = sc->vbat_volt;
        } else {
            val->intval = sc->vbat_volt;
        }
        break;
    case POWER_SUPPLY_PROP_SC_BATTERY_CURRENT:
        ret = sc854x_get_adc_data(sc, ADC_IBAT, &result);
        if (!ret) {
            sc->ibat_curr = result * 1000;
            val->intval = sc->ibat_curr;
        } else {
            val->intval = sc->ibat_curr;
        }
        break;
    case POWER_SUPPLY_PROP_SC_BUS_VOLTAGE:
        ret = sc854x_get_adc_data(sc, ADC_VBUS, &result);
        if (!ret) {
            sc->vbus_volt = result * 1000;
            val->intval = sc->vbus_volt;
        } else {
           val->intval = sc->vbus_volt;
        }
        break;
    case POWER_SUPPLY_PROP_SC_BUS_CURRENT:
        ret = sc854x_get_adc_data(sc, ADC_IBUS, &result);
        if (!ret) {
            sc->ibus_curr = result * 1000;
            val->intval = sc->ibus_curr;
        } else {
            val->intval = sc->ibus_curr;
        }
        break;
    case POWER_SUPPLY_PROP_SC_DIE_TEMPERATURE:
        ret = sc854x_get_adc_data(sc, ADC_TDIE, &result);
        if (!ret) {
            sc->die_temp = result;
            val->intval = sc->die_temp;
        } else {
            val->intval = sc->die_temp;
        }
        break;
    case POWER_SUPPLY_PROP_SC_VBUS_ERROR_STATUS:
        ret = sc854x_check_vbus_error_status(sc, &result);
        if (!ret) {
            sc->vbus_error = result;
            val->intval = sc->vbus_error;
        } else {
            val->intval = sc->vbus_error;
        }
        break;
    case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
        ret = sc854x_get_charge_mode(sc, &result);
        if (!ret) {
            sc->charger_mode = result;
            val->intval = sc->charger_mode;
        } else {
            val->intval = sc->charger_mode;
        }
        break;
    case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
        val->intval = sc->direct_charge;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}


static int sc854x_charger_set_property(struct power_supply *psy,
                    enum power_supply_property prop,
                    const union power_supply_propval *val)
{
    struct sc854x *sc = power_supply_get_drvdata(psy);
    
    sc_err("prop = %d\n",  prop);
    switch (prop) {
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
        sc854x_enable_charge(sc, val->intval);
        sc854x_check_charge_enabled(sc, &sc->charge_enabled);
        sc_info("POWER_SUPPLY_PROP_CHARGING_ENABLED: %s\n",
                val->intval ? "enable" : "disable");
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        sc854x_set_present(sc, val->intval);
        break;
    case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
        sc854x_set_charge_mode(sc, val->intval);
        break;
    case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
        sc->direct_charge = val->intval;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}


static int sc854x_charger_is_writeable(struct power_supply *psy,
                    enum power_supply_property prop)
{
    int ret;

    switch (prop) {
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
    case POWER_SUPPLY_PROP_PRESENT:
    case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
    case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
        ret = 1;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

static int sc854x_psy_register(struct sc854x *sc)
{
    sc->psy_cfg.drv_data = sc;
    sc->psy_cfg.of_node = sc->dev->of_node;

    if (sc->mode == STANDALONE) {
        sc->psy_desc.name = "sc854x-standalone";
    } else if (sc->mode == MASTER) {
        sc->psy_desc.name = "sc854x-master";
    } else {
        sc->psy_desc.name = "sc854x-slave";
    }
    
    sc->psy_desc.type = POWER_SUPPLY_TYPE_MAINS;
    sc->psy_desc.properties = sc854x_charger_props;
    sc->psy_desc.num_properties = ARRAY_SIZE(sc854x_charger_props);
    sc->psy_desc.get_property = sc854x_charger_get_property;
    sc->psy_desc.set_property = sc854x_charger_set_property;
    sc->psy_desc.property_is_writeable = sc854x_charger_is_writeable;


    sc->fc2_psy = devm_power_supply_register(sc->dev, 
            &sc->psy_desc, &sc->psy_cfg);
    if (IS_ERR(sc->fc2_psy)) {
        sc_err("failed to register fc2_psy\n");
        return PTR_ERR(sc->fc2_psy);
    }

    sc_info("%s power supply register successfully\n", sc->psy_desc.name);

    return 0;
}

static int sc854x_irq_register(struct sc854x *sc)
{
    int ret;
    struct device_node *node = sc->dev->of_node;

    if (!node) {
        sc_err("device tree node missing\n");
        return -EINVAL;
    }

    sc->irq_gpio = of_get_named_gpio(node, "sc,sc854x,irq-gpio", 0);
    if (!gpio_is_valid(sc->irq_gpio)) {
        sc_err("fail to valid gpio : %d\n", sc->irq_gpio);
        return -EINVAL;
    }

    if (gpio_is_valid(sc->irq_gpio)) {
        ret = gpio_request_one(sc->irq_gpio, GPIOF_DIR_IN,"sc854x_irq");
        if (ret) {
            sc_err("failed to request sc854x_irq\n");
            return -EINVAL;
        }
        sc->irq = gpio_to_irq(sc->irq_gpio);
        if (sc->irq < 0) {
            sc_err("failed to gpio_to_irq\n");
            return -EINVAL;
        }
    } else {
        sc_err("irq gpio not provided\n");
        return -EINVAL;
    }

    if (sc->irq) {
        if (sc->mode == STANDALONE) {
            ret = devm_request_threaded_irq(&sc->client->dev, sc->irq,
                    NULL, sc854x_charger_interrupt,
                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                    "sc854x standalone irq", sc);
        } else if (sc->mode == MASTER) {
            ret = devm_request_threaded_irq(&sc->client->dev, sc->irq,
                    NULL, sc854x_charger_interrupt,
                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                    "sc854x master irq", sc);
        } else {
            ret = devm_request_threaded_irq(&sc->client->dev, sc->irq,
                    NULL, sc854x_charger_interrupt,
                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                    "sc854x slave irq", sc);
        }
        if (ret < 0) {
            sc_err("request irq for irq=%d failed, ret =%d\n",
                            sc->irq, ret);
            return ret;
        }
        enable_irq_wake(sc->irq);
    }

    return ret;
}

static void sc854x_check_fault_status(struct sc854x *sc)
{
    int ret;
    u8 flag = 0;

    mutex_lock(&sc->data_lock);

    ret = sc854x_read_byte(sc, SC854X_REG_02, &flag);
    if (!ret && (flag & SC854X_AC_OVP_FLAG_MASK))
        sc_err("irq VAC OVP FLAG\n");

    ret = sc854x_read_byte(sc, SC854X_REG_03, &flag);
    if (!ret && (flag & SC854X_VDROP_OVP_FLAG_MASK))
        sc_err("irq CDROP OVP FLAG\n");

    ret = sc854x_read_byte(sc, SC854X_REG_06, &flag);
    if (!ret && flag) {
        if (flag & SC854X_TSHUT_FLAG_MASK)
            sc_err("irq TSHUT FLAG\n");
        if (flag & SC854X_SS_TIMEOUT_FLAG_MASK)
            sc_err("irq SS TIMEOUT FLAG\n");
        if (flag & SC854X_REG_TIMEOUT_FLAG_MASK)
            sc_err("irq REG TIMEOUT FLAG\n");
        if (flag & SC854X_PIN_DIAG_FALL_FLAG_MASK)
            sc_err("irq PIN DIAG FALL FLAG\n");
    }

    ret = sc854x_read_byte(sc, SC854X_REG_09, &flag);
    if (!ret && flag) {
        if (flag & SC854X_POR_FLAG_MASK) {
            sc_err("irq POR FLAG\n");
            sc854x_set_present(sc, true);
        }
        if (flag & SC854X_IBUS_UCP_RISE_FLAG_MASK)
            sc_err("irq IBUS UCP RIST FLAG");
        if (flag & SC854X_WD_TIMEOUT_FLAG_MASK)
            sc_err("irq WD TIMEOUT FLAG\n");
    }

    ret = sc854x_read_byte(sc, SC854X_REG_0A, &flag);
    if (!ret && (flag & SC854X_VBATREG_ACTIVE_FLAG_MASK))
        sc_err("irq VBATREG ACTIVE FLAG\n");

    ret = sc854x_read_byte(sc, SC854X_REG_0B, &flag);
    if (!ret && (flag & SC854X_IBATREG_ACTIVE_FLAG_MASK))
        sc_err("irq IBATREG ACTIVE FLAG\n");

    ret = sc854x_read_byte(sc, SC854X_REG_0C, &flag);
    if (!ret && (flag & SC854X_IBUSREG_ACTIVE_FLAG_MASK))
        sc_err("irq IBUSREG ACTIVE FLAG\n");

    ret = sc854x_read_byte(sc, SC854X_REG_0D, &flag);
    if (!ret && flag) {
        if (flag & SC854X_PMID2OUT_UVP_FLAG_MASK)
            sc_err("irq PMID2OUT UVP FLAG\n");
        if (flag & SC854X_PMID2OUT_OVP_FLAG_MASK)
            sc_err("irq PMID2OUT OVP FLAG\n");
    }
    
    ret = sc854x_read_byte(sc, SC854X_REG_0F, &flag);
    if (!ret && flag) {
        if (flag & SC854X_VOUT_OVP_FLAG_MASK)
            sc_err("irq VOUT OVP FLAG\n");
        if (flag & SC854X_VBAT_OVP_FLAG_MASK)
            sc_err("irq VBAT OVP FLAG\n");
        if (flag & SC854X_IBAT_OCP_FLAG_MASK)
            sc_err("irq IBAT OCP FLAG\n");
        if (flag & SC854X_VBUS_OVP_FLAG_MASK)
            sc_err("irq VBUS OVP FLAG\n");
        if (flag & SC854X_IBUS_OCP_FLAG_MASK)
            sc_err("irq IBAT OCP FLAG\n");
        if (flag & SC854X_IBUS_UCP_FALL_FLAG_MASK)
            sc_err("irq IBUS UCP FALL FLAG\n");
        if (flag & SC854X_ADAPTER_INSERT_FLAG_MASK)
            sc_err("irq ADAPTER INSERT FLAG\n");
        if (flag & SC854X_VBAT_INSERT_FLAG_MASK)
            sc_err("irq VBAT INSERT FLAG\n");
    }

    mutex_unlock(&sc->data_lock);
}

static void determine_initial_status(struct sc854x *sc)
{
    if (sc->client->irq)
        sc854x_charger_interrupt(sc->client->irq, sc);
}

static struct of_device_id sc854x_charger_match_table[] = {
    {   .compatible = "sc,sc8545-standalone", 
        .data = &sc854x_mode_data[SC8545_STANDALONG],},
    {   .compatible = "sc,sc8545-master",
        .data = &sc854x_mode_data[SC8545_MASTER],},
    {   .compatible = "sc,sc8545-slave",
        .data = &sc854x_mode_data[SC8545_SLAVE],},
    {   .compatible = "sc,sc8546-standalone",
        .data = &sc854x_mode_data[SC8546_STANDALONG],},
    {   .compatible = "sc,sc8546-master",
        .data = &sc854x_mode_data[SC8546_MASTER],},
    {   .compatible = "sc,sc8546-slave",
        .data = &sc854x_mode_data[SC8546_SLAVE],},
    {   .compatible = "sc,sc8547-standalone",
        .data = &sc854x_mode_data[SC8547_STANDALONG],},
    {   .compatible = "sc,sc8547-master",
        .data = &sc854x_mode_data[SC8547_MASTER],},
    {   .compatible = "sc,sc8547-slave",
        .data = &sc854x_mode_data[SC8547_SLAVE],},
    {   .compatible = "sc,sc8549-standalone",
        .data = &sc854x_mode_data[SC8549_STANDALONG],},
    {   .compatible = "sc,sc8549-master",
        .data = &sc854x_mode_data[SC8549_MASTER],},
    {   .compatible = "sc,sc8549-slave",
        .data = &sc854x_mode_data[SC8549_SLAVE],},
    {},
};

static int sc854x_suspend_notifier(struct notifier_block *nb,
                unsigned long event,
                void *dummy)
{
    struct sc854x *sc = container_of(nb, struct sc854x, pm_nb);

    switch (event) {

    case PM_SUSPEND_PREPARE:
        pr_err("SC854X PM_SUSPEND \n");
        usbqc_suspend_notifier(true);
        sc->sc854x_suspend_flag = 1;
        return NOTIFY_OK;

    case PM_POST_SUSPEND:
        pr_err("SC854X PM_RESUME \n");
        usbqc_suspend_notifier(false);
        sc->sc854x_suspend_flag = 0;
        return NOTIFY_OK;

    default:
        return NOTIFY_DONE;
    }
}

static int sc854x_charger_probe(struct i2c_client *client,
                    const struct i2c_device_id *id)
{
    struct sc854x *sc;
    const struct of_device_id *match;
    struct device_node *node = client->dev.of_node;
    int ret;

    sc = devm_kzalloc(&client->dev, sizeof(struct sc854x), GFP_KERNEL);
    if (!sc)
        return -ENOMEM;

    sc->dev = &client->dev;

    sc->client = client;
    
    mutex_init(&sc->i2c_rw_lock);
    mutex_init(&sc->data_lock);
    mutex_init(&sc->irq_complete);

    sc->resume_completed = true;
    sc->irq_waiting = false;

    ret = sc854x_detect_device(sc);
    if (ret) {
        sc_err("No sc854x device found!\n");
        goto err_1;
    }
    
    i2c_set_clientdata(client, sc);
    sc854x_create_device_node(&(client->dev));

    match = of_match_node(sc854x_charger_match_table, node);
    if (match == NULL) {
        sc_err("device tree match not found!\n");
        goto err_1;
    }
    
    sc854x_set_work_mode(sc, *(int *)match->data);
    if (ret) {
        goto err_1;
    }

    ret = sc854x_parse_dt(sc, &client->dev);
    if (ret) {
        goto err_1;
    }

    ret = sc854x_init_device(sc);
    if (ret) {
        sc_err("Failed to init device\n");
        goto err_1;
    }
    
    sc->pm_nb.notifier_call = sc854x_suspend_notifier;
    register_pm_notifier(&sc->pm_nb);

    ret = sc854x_psy_register(sc);
    if (ret) {
        goto err_2;
    }

    ret = sc854x_irq_register(sc);
    if (ret < 0) {
        goto err_2;
    }

    device_init_wakeup(sc->dev, 1);

    determine_initial_status(sc);

    sc_info("sc854x probe successfully!\n");

    return 0;

err_2:
    power_supply_unregister(sc->fc2_psy);
err_1:
    mutex_destroy(&sc->i2c_rw_lock);
    mutex_destroy(&sc->data_lock);
    mutex_destroy(&sc->irq_complete);
    sc_info("sc854x probe fail\n");
    devm_kfree(&client->dev, sc);
    return ret;
}

static inline bool is_device_suspended(struct sc854x *sc)
{
    return !sc->resume_completed;
}

static int sc854x_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sc854x *sc = i2c_get_clientdata(client);

    mutex_lock(&sc->irq_complete);
    sc->resume_completed = false;
    mutex_unlock(&sc->irq_complete);
    sc_err("Suspend successfully!");

    return 0;
}

static int sc854x_suspend_noirq(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sc854x *sc = i2c_get_clientdata(client);

    if (sc->irq_waiting) {
        pr_err_ratelimited("Aborting suspend, an interrupt was detected while suspending\n");
        return -EBUSY;
    }
    return 0;
}

static int sc854x_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sc854x *sc = i2c_get_clientdata(client);

    mutex_lock(&sc->irq_complete);
    sc->resume_completed = true;
    if (sc->irq_waiting) {
        sc->irq_disabled = false;
        enable_irq(client->irq);
        mutex_unlock(&sc->irq_complete);
        sc854x_charger_interrupt(client->irq, sc);
    } else {
        mutex_unlock(&sc->irq_complete);
    }

    power_supply_changed(sc->fc2_psy);
    sc_err("Resume successfully!");

    return 0;
}
static int sc854x_charger_remove(struct i2c_client *client)
{
    struct sc854x *sc = i2c_get_clientdata(client);

    power_supply_unregister(sc->fc2_psy);
    mutex_destroy(&sc->data_lock);
    mutex_destroy(&sc->i2c_rw_lock);
    mutex_destroy(&sc->irq_complete);

    return 0;
}

static void sc854x_charger_shutdown(struct i2c_client *client)
{
    struct sc854x *sc = i2c_get_clientdata(client);
    /* disable charger pump when shutdown */
    sc854x_enable_adc(sc, false);
    sc854x_enable_charge(sc, false);
    return ;
}

static const struct dev_pm_ops sc854x_pm_ops = {
    .resume         = sc854x_resume,
    .suspend_noirq  = sc854x_suspend_noirq,
    .suspend        = sc854x_suspend,
};

static struct i2c_driver sc854x_charger_driver = {
    .driver     = {
        .name   = "sc854x-charger",
        .owner  = THIS_MODULE,
        .of_match_table = sc854x_charger_match_table,
        .pm = &sc854x_pm_ops,
    },
    .probe      = sc854x_charger_probe,
    .remove     = sc854x_charger_remove,
    .shutdown   = sc854x_charger_shutdown,
};

module_i2c_driver(sc854x_charger_driver);

MODULE_DESCRIPTION("SC SC854X Charge Pump Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("South Chip <Aiden-yu@southchip.com>");

