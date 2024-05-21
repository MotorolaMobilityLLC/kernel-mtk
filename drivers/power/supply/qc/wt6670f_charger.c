/*opyright (C) 2020 XiaoMi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "[wt6670f] %s: " fmt, __func__
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>
#include <linux/firmware.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/v1/charger_class.h>
#include <mt-plat/v1/mtk_charger.h>
#include <mt-plat/v1/charger_type.h>

#include "../sgm415xx/sgm415xx.h"
#include <linux/stat.h>
#include <linux/ctype.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/vmalloc.h>
#include <linux/preempt.h>
#include "wt6670f_firmware.h"

#define FIRMWARE_FILE_LENGTH            15000
#define UPDATE_SUCCESS                  0
#define ERROR_REQUESET_FIRMWARE         -1
#define ERROR_CHECK_FIREWARE_FORMAT     -2
#define ERROR_GET_CHIPID_FAILED         -3
#define ERROR_ERASE_FAILED              -4
#define ERROR_FINISH_CMD_FAILED         -5
#define ERROR_FILE_CRC_FAILED           -6
#define ERROR_HIGHADD_CMD_FAILED        -7
#define ERROR_PROGRAM_CMD_FAILED        -8
#define ERROR_CALLBACK_FAILED           -9

/*Z350*/
#define Z350_REG_VERSION            0x14
#define Z350_REG_QC30_PM            0x73
#define Z350_REG_QC35_PM            0x83

#define Z350_REG_SOFT_RESET         0X04
#define Z350_REG_QC_MODE            0x02
#define Z350_REG_HVDCP              0x05
#define Z350_REG_BC1P2              0x06
#define Z350_REG_CHG_TYPE           0x11
#define Z350_REG_VIN                0x12
#define Z350_REG_VID                0x13


/*WT6670F*/
#define WT6670F_REG_RUN_APSD     0xB0
#define WT6670F_REG_QC_MODE      0xB1
#define WT6670F_REG_SOFT_RESET   0xB3
#define WT6670F_REG_BC12_START   0xB6
#define WT6670F_REG_QC30_PM      0xBA
#define WT6670F_REG_QC35_PM      0xBB
#define WT6670F_REG_FIRM_VER     0xBF
#define WT6670F_REG_CHG_TYPE     0XBD

#define QC3P_WT6670F   0x01 /*100021221*/
#define QC3P_Z350    0x00
#define QC35_SDP   0x04
u16 g_qc3p_id = 0;
#define QC35_UNKNOW  0x02
#define POWER_SUPPLY_PROP_PD_VERIFY_IN_PROCESS 0x03
#define WT6670F_FIRMWARE_VERSION 0x0200
#define QC3_DP_DM_PR_FCC 2000000

#define ENABLE      0x01
#define DISABLE     0x00

#define QC35_DPDM   0x01
#define QC30_DPDM   0x00

#define WT6670F_ADDR    0x34
#define SOFT_RESET_VAL  0xff

extern void hvdcp3_chgstat_notify(void);
bool g_qc3_type_set = false;

static  int m_chg_type = 0;
static int g_change_num = 0;

enum usbsw_state {
    USBSW_CHG = 0,
    USBSW_USB,
};

enum qc35_error_state {
    QC35_ERROR_NO,
    QC35_ERROR_QC30_DET,
    QC35_ERROR_TA_DET,
    QC35_ERROR_TA_CAP,
    QC35_QC3_PLUS_DET_OK,
};

enum thermal_status_levels {
    TEMP_SHUT_DOWN = 0,
    TEMP_SHUT_DOWN_SMB,
    TEMP_ALERT_LEVEL,
    TEMP_ABOVE_RANGE,
    TEMP_WITHIN_RANGE,
    TEMP_BELOW_RANGE,
};

struct wt6670f_desc {
    bool en_bc12;
    bool en_hvdcp;
    bool en_intb;
    bool en_sleep;
    const char *chg_dev_name;
    const char *alias_name;
};

static struct wt6670f_desc wt6670f_default_desc = {
    .en_bc12 = true,
    .en_hvdcp = true,
    .en_intb = true,
    .en_sleep = true,
    .chg_dev_name = "secondary_chg",
    .alias_name = "z350_wt6670f",
};

bool qc3p_z350_init_ok = false;
struct mt_charger {
    struct power_supply_desc usb_desc;
};

struct wt6670f_charger {
    struct i2c_client           *client;
    struct device               *dev;

    struct wt6670f_desc     *desc;//100021221
    struct charger_device       *chg_dev;
    struct charger_device       *chg_dev_p;
    struct charger_properties   chg_props;

    struct charger_consumer     *chg_consumer;
    struct mt_charger           *mt_chg;

    bool                        tcpc_attach;
    enum charger_type           chg_type;

    int                         rst_gpio;
    int                         int_gpio;
    int                         reset_pin;
    bool                        otg_enable;
    struct mutex                chgdet_lock;
    bool                        hvdcp_dpdm_status;
    unsigned int                connect_therm_gpio;
    int                         connector_temp;
    struct mutex                irq_complete;

    int                         qc35_chg_type;
    int                         qc35_err_sta;
    int                         pulse_cnt;
    bool                        vbus_disable;
    enum thermal_status_levels  thermal_status;
    int                         entry_time;
    int                         count;//100021221
    int                         rerun_apsd_count;

    bool                        chg_ready;
    bool                        bc12_unsupported;
    bool                        hvdcp_unsupported;
    bool                        intb_unsupported;
    bool                        sleep_unsupported;
    struct delayed_work         conn_therm_work;
    struct delayed_work         charger_type_det_work;
    struct delayed_work         chip_reset_wt6670f;
    struct delayed_work         chip_update_work;
    struct delayed_work         hvdcp_det_retry_work;

    /* psy */
    struct power_supply         *usb_psy;
    struct power_supply         *bms_psy;
    struct power_supply         *main_psy;
    struct power_supply         *charger_psy;
    struct power_supply         *charger_identify_psy;
    struct power_supply_desc    charger_identify_psy_d;

    bool                        irq_waiting;
    bool                        resume_completed;
    int                         dpdm_mode;
    bool                        hvdcp_en;
    bool                        irq_data_process_enable;
    struct pinctrl              *wt6670f_pinctrl;
    struct pinctrl_state        *pinctrl_state_normal;
    struct pinctrl_state        *pinctrl_state_isp;
    struct pinctrl_state        *pinctrl_scl_normal;
    struct pinctrl_state        *pinctrl_scl_isp;
    struct pinctrl_state        *pinctrl_sda_normal;
    struct pinctrl_state        *pinctrl_sda_isp;
    int                         wt6670f_sda_gpio;
    int                         wt6670f_scl_gpio;
    struct mutex                isp_sequence_lock;

    int                         hvdcp_timer;
    int                         hvdcp_retry_timer;
    int                         ocp_timer;
};

static struct wt6670f_charger *_chip = NULL;

static int __wt6670f_write_byte_only(struct wt6670f_charger *chip, u8 reg)
{
    s32 ret;

    ret = i2c_smbus_write_byte(chip->client, reg);
    if (ret < 0) {
        pr_err("%s: reg[0x%02X] write failed.\n",
                __func__, reg);
        return ret;
    }
    return 0;
}

static int __wt6670f_write_byte(struct wt6670f_charger *chip, u8 reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(chip->client, reg, val);
    if (ret < 0) {
        pr_err("%s: reg[0x%02X] write failed.\n",
                __func__, reg);
        return ret;
    }
    return 0;
}

static int __wt6670f_read_word(struct wt6670f_charger *chip, u8 reg, u16 *data)
{
    s32 ret;

    ret = i2c_smbus_read_word_data(chip->client, reg);
    if (ret < 0) {
        pr_err("%s: reg[0x%02X] read failed.\n",
                __func__, reg);
        return ret;
    }

    *data = (u16) ret;

    return 0;
}


static int __wt6670f_write_word(struct wt6670f_charger *chip, u8 reg, u16 val)
{
    s32 ret;

    ret = i2c_smbus_write_word_data(chip->client, reg, val);
    if (ret < 0) {
        pr_err("%s: reg[0x%02X] write failed.\n",
                __func__, reg);
        return ret;
    }
    return 0;
}

/*
static int __wt6670f_read_block(struct wt6670f_charger *chip, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_i2c_block_data(chip->client, reg, 4, data);
    if (ret < 0) {
        pr_err("%s: reg[0x%02X] read failed.\n",
                __func__, reg);
        return ret;
    }

    return 0;
}
*/
static int wt6670f_write_byte_only(struct wt6670f_charger *chip, u8 reg)
{
    return __wt6670f_write_byte_only(chip, reg);
}

static int wt6670f_write_byte(struct wt6670f_charger *chip, u8 reg, u8 data)
{
    return __wt6670f_write_byte(chip, reg, data);
}

static int wt6670f_read_word(struct wt6670f_charger *chip, u8 reg, u16 *data)
{
    return __wt6670f_read_word(chip, reg, data);
}

static int wt6670f_write_word(struct wt6670f_charger *chip, u8 reg, u16 data)
{
    return __wt6670f_write_word(chip, reg, data);
}
/*
static int wt6670f_read_block(struct wt6670f_charger *chip, u8 reg, u8 *data)
{
    return __wt6670f_read_block(chip, reg, data);
}
*/

static int wt6670f_set_usbsw_state(struct wt6670f_charger *chip, int state)
{
    pr_info("%s: state = %s\n", __func__, state ? "usb" : "chg");

    if (state == USBSW_CHG) {
        Charger_Detect_Init();
    } else {
        Charger_Detect_Release();
    }
    return 0;
}

int wt6670f_set_volt_count(int count)
{
    int ret = 0;
    u16 step = abs(count);
    struct wt6670f_charger *chip =  _chip;
    pr_info("Set qc3 vbus with %d pulse!\n",count);

    if(count  < 0) {
        chip->count -= step;
        step &= 0x7FFF;
        step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
        if(chip->count < 0)
            chip->count = 0;
    } else if(count > 0) {
        chip->count += step;
        step |= 0x8000;
        step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
    } else {
        pr_err("%s: return witch count == 0\n", __func__);
        return 0;
    }

    pr_info("---->chip->count = %d  step = %04x\n", chip->count, step);

    if (QC3P_WT6670F == g_qc3p_id) {
        ret = wt6670f_write_word(chip, WT6670F_REG_QC35_PM, step);
    } else if(QC3P_Z350 == g_qc3p_id) {
        ret = wt6670f_write_word(chip, Z350_REG_QC35_PM, step);
    } else {
    }
    return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_set_volt_count);


int wt6670f_set_qc3_volt_count(int count)
{
    int ret = 0;
    u16 step = abs(count);
    struct wt6670f_charger *chip =  _chip;
    pr_info("Set vbus with %d pulse!\n",count);

    if(count  < 0) {
        chip->count -= step;
        step &= 0x7FFF;
        step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
        if(chip->count < 0)
            chip->count = 0;
    } else if(count > 0) {
        chip->count += step;
        step |= 0x8000;
        step = ((step & 0xff) << 8) | ((step >> 8) & 0xff);
    } else {
        pr_err("%s: return witch count == 0\n", __func__);
        return 0;
    }

    pr_info("---->chip->count = %d  step = %04x\n", chip->count, step);

    if (QC3P_WT6670F == g_qc3p_id) {
        ret = wt6670f_write_word(chip, WT6670F_REG_QC30_PM, step);
    } else if(QC3P_Z350 == g_qc3p_id) {
        ret = wt6670f_write_word(chip, Z350_REG_QC30_PM, step);
    } else {
        /* Nothing */
    }

    return ret;
}
EXPORT_SYMBOL_GPL(wt6670f_set_qc3_volt_count);

static u16 wt6670f_get_firmware_version(struct wt6670f_charger *chip, u16 *val)
{
    int ret;
    u16 tmp = 0;

    ret = wt6670f_read_word(chip, WT6670F_REG_FIRM_VER, &tmp);
    if (ret) {
        pr_err("%s: get version failed!\n", __func__);
    } else {
        *val = (u16)(((tmp & 0xFF) << 8) | ((tmp >> 8) & 0xFF));
        pr_info("%s: get version success, 0x%04x!\n", __func__, *val);
    }

    return ret;
}

static void wt6670f_do_reset(struct wt6670f_charger *chip)
{
    if (chip == NULL) {
        pr_err("%s chip is NULL do_reset\n", __func__);
        return;
    }

    gpio_set_value(chip->rst_gpio, 1);
    mdelay(15);
    gpio_set_value(chip->rst_gpio, 0);
    mdelay(40); 
}

void wt6670f_charger_reset_reg(void)
{
    if (_chip == NULL) {
        pr_err("%s _chip is NULL\n", __func__);
        return;
    }
    struct wt6670f_charger *chip =  _chip;
    pr_info("%s: reset z350\n", __func__);
    gpio_set_value(chip->rst_gpio, 1);
}

void wt6670f_reset_chg_type(void)//(struct wt6670f_charger *chip)
{
    if (_chip == NULL) {
        pr_err("%s _chip is NULL\n", __func__);
        return;
    }
    struct wt6670f_charger *chip =  _chip;
    chip->chg_type = 0;
    chip->chg_ready = false;
}
EXPORT_SYMBOL_GPL(wt6670f_reset_chg_type);

void wt6670f_do_init(struct wt6670f_charger *chip)
{
    //bool qc3p_z350_init_ok = false;
    u16 is_already_probe_ok = 0;
    u16 firmware_version = 0;
    wt6670f_do_reset(chip);
    u16 val = 0;

    gpio_direction_output(chip->rst_gpio, 1);
    if(g_qc3p_id == QC3P_Z350)
        qc3p_z350_init_ok = true;
    is_already_probe_ok = true;

    firmware_version = wt6670f_get_firmware_version(chip, &val);
    wt6670f_reset_chg_type();
    pr_info("[%s] firmware version = %d, chr_type=0x%x\n", __func__, val, chip->chg_type);
}   

static u16 wt6670f_get_id(struct wt6670f_charger *chip, u8 reg)
{
    int ret = 0;
    u16 id;

    ret = wt6670f_read_word(chip, reg, &id);
    if (ret){
        pr_err("%s: get vendor id failed!\n", __func__);
        return 0;
    }
    else {
        pr_info("%s: get vendor id : %d\n", __func__, id);
        return id;
    }
}

void wt6670f_do_reinit(struct wt6670f_charger *chip)
{
    //bool qc3p_z350_init_ok = false;
    u16 is_ready_probe_ok = 0;

    wt6670f_do_reset(chip);
    if(QC3P_Z350 == g_qc3p_id) {
        qc3p_z350_init_ok = true;
        gpio_direction_output(chip->reset_pin, 1);
        pr_info("[%s] z350 reinit ok\n", __func__);
        is_ready_probe_ok = 1;
    }
}

static void wt6670f_en_hvdcp(struct wt6670f_charger *chip, bool enable) 
{
    if(enable) {
        wt6670f_write_byte(chip, Z350_REG_HVDCP, 0x01);
    } else {
        wt6670f_write_byte(chip, Z350_REG_HVDCP, 0x00);
    }
    pr_info("[%s] z350 hvdcp enable : %d\n", __func__, enable);
}

static u32 wt6670f_start_detection(struct wt6670f_charger *chip)
{
    int ret = 0;

    ret = wt6670f_write_byte_only(chip, WT6670F_REG_RUN_APSD);
    pr_info("[%s] wt6670 apsd : %d\n", __func__, ret);
    return ret;
}

static void wt6670f_force_qc2_5V(struct wt6670f_charger *chip)
{
    if (QC3P_Z350 == g_qc3p_id) {
        wt6670f_write_byte(chip, Z350_REG_QC_MODE, 0x01);
    } else {
        wt6670f_write_byte(chip, WT6670F_REG_QC_MODE, 0x01);
    }
    pr_info("[%s]: QC2 MODE 5V\n", __func__);
}

static void wt6670f_force_qc3_5V(struct wt6670f_charger *chip)
{
    if (QC3P_Z350 == g_qc3p_id) {
        wt6670f_write_byte(chip, Z350_REG_QC_MODE, 0x04);
    } else {
        wt6670f_write_byte(chip, WT6670F_REG_QC_MODE, 0x04);
    }
    pr_info("[%s]: QC3 MODE 5V\n", __func__);
}

static int wt6670f_charger_type(struct wt6670f_charger *chip)
{
     int ret;
     u16 data;
     u8 data1, data2;
     if (QC3P_WT6670F == g_qc3p_id) {
         ret = wt6670f_read_word(chip, WT6670F_REG_CHG_TYPE, &data);
         pr_info("wt6670f get protocol %x\n", data);
     } else if (QC3P_Z350 == g_qc3p_id) {
         ret = wt6670f_read_word(chip, Z350_REG_CHG_TYPE, &data);
         pr_info("z350 get protocol %x\n",data);
     } else {
         ret = -1;
     }

     if (ret < 0) {
        pr_err("wt6670f get protocol fail\n");
        return ret;
     }

     // Get data2 part
     data1 = data & 0xFF;
     data2 = data >> 8;
     if ((QC3P_Z350 == g_qc3p_id) && (data1 <= 7))
             data1--;

     pr_info("Get charger type, rowdata = 0X%04x, data1= 0X%02x, data2=0X%02x \n", data, data1, data2);
     if ((data2 == 0x03) && ((data1 > 0x9) || (data1 == 0x7))) {
        pr_err("fail to get charger type, error happens!\n");
        return -EINVAL;
     }

     if (data2 == 0x04) {
         pr_info("detected QC3+ charger:0X%02x!\n", data1);
     }

     if ((data1 > 0x00 && data1 < 0x07) ||
        (data1 > 0x07 && data1 < 0x0a) ||(QC3P_Z350 == g_qc3p_id && data1 == 0x10)) {
            ret = data1;
     } else {
            ret = 0x00;
     }

    chip->chg_type = ret;
//        return data & 0xff;
    return ret;
}

bool wt6670f_is_charger_ready(void)
{
    bool m_chg_ready = true;
    return m_chg_ready;
}

int wt6670f_get_charger_type(void)
{
    return m_chg_type;
}
EXPORT_SYMBOL_GPL(wt6670f_get_charger_type);

int wt6670f_reset_charger_type(void)
{
    m_chg_type = 0;
    g_change_num = 0;
    return m_chg_type;
}
EXPORT_SYMBOL_GPL(wt6670f_reset_charger_type);

void wt6670f_get_charger_type_func_work(struct work_struct *work)
{

    if (_chip == NULL) {
        pr_err("__chip is NULL\n");
        return;
    }
    int early_chg_type = 0;
    int wait_count = 0;
    int count = 0;
    struct wt6670f_charger *chip = _chip; 
    struct delayed_work *psy_work = NULL;
    union power_supply_propval  propval = {0};
    bool m_chg_ready = false;
    bool early_notified = false;
    bool should_notify = false;
    bool need_retry = false;
    bool wt6670f_is_detect = true;

    psy_work = container_of(work, struct delayed_work, work);
    if(psy_work == NULL) {
        pr_err("cannot get charger_monitor_work\n");
        return;
    }
    wt6670f_do_reset(chip);
    /*Set cur is 500ma before do BC1.2 & QC*/
    charger_manager_set_qc_input_current_limit(chip->chg_consumer, 0, 500000);
    charger_manager_set_qc_charging_current_limit(chip->chg_consumer, 0, 500000);

    Charger_Detect_Init();
    if(QC3P_WT6670F == g_qc3p_id) {
        do {
            m_chg_ready = false;
            m_chg_type = 0;
            early_notified = false;
            should_notify = false;
            need_retry = false;
            early_chg_type = 0;
            /* APSD BC1.2 need 800ms */
            wt6670f_start_detection(chip);
            msleep(800);

            while((!m_chg_ready) && (count< 13)) {
                msleep(200);
                count++;
                //m_chg_ready = wt6670f_is_charger_ready();
                if(!early_notified) {
                    early_chg_type = wt6670f_charger_type(chip);
                }

                if(early_chg_type == 0x08 || early_chg_type == 0x09) {
                    m_chg_type = early_chg_type;
                    pr_info("[%s] WT6670F early type is QC3+, skip detecting\n", __func__);
                    break;
                }
                pr_info("wt6670f waiting early type:0x%x, detect ready: 0x%x",early_chg_type, m_chg_ready);
                m_chg_type = wt6670f_charger_type(chip);

                if(m_chg_type == 0x7 && !need_retry) {
                    need_retry = true;
                } else {
                    need_retry = false;
                }
            }
        }while(need_retry);
    } // wt6670f

    if (QC3P_Z350 == g_qc3p_id) { //z350
        if(qc3p_z350_init_ok) {
            qc3p_z350_init_ok = false;
            wt6670f_do_reset(chip);
        }
        m_chg_type = 0;
        wait_count = 0;
        /* 800ms for BC12 */
        while ((!m_chg_type) && (wait_count < 8)) {
            msleep(100);
            wait_count++;
            pr_info("z350 early waiting dcp type:%x,%d\n",m_chg_type,wait_count);
        }

        m_chg_type = wt6670f_charger_type(chip);

        if ((m_chg_type != 0x02) && (m_chg_type != 0x03)) {
            if (!chip->charger_psy) 
                chip->charger_psy = power_supply_get_by_name("sgm4154x-charger");

            wait_count = 0;
            while ((!m_chg_type) && (wait_count < 8)) {
                msleep(100);
                wait_count++;
                pr_info("z350 waiting dcp type:%x,%d\n", m_chg_type,wait_count);
            }
            m_chg_type = wt6670f_charger_type(chip);
        }
        /* about 1500ms for HVDCP after DCP */
        if (m_chg_type == 0x04) {
            pr_info("z350 == 01 retry type");
            wait_count = 0;
            while (wait_count < 15) {
                msleep(100);
                wait_count++;
                m_chg_type = wt6670f_charger_type(chip);
                if (m_chg_type == 0x10) {
                    pr_info("z350 early waiting HVDCP type:%x,%d\n", m_chg_type, wait_count);
                    break;
                }
            }
            pr_info("z350 == 04 retry type:%x, %d\n", m_chg_type, wait_count);
        }

        if (m_chg_type == 0x10) {
            wt6670f_en_hvdcp(chip, true);
            wait_count = 0;
            /* about 1000ms doing QC3+ */
            while((m_chg_type != 0xff) && (wait_count < 10)) {
                msleep(100);
                wait_count++;
            }
            m_chg_type = wt6670f_charger_type(chip);
        }
        pr_err(" [%s] z350 charger type is 0x%x\n", __func__, m_chg_type);
    }

    wt6670f_is_detect = false;
    Charger_Detect_Release();

    if (m_chg_type == 0x05) {
        wt6670f_force_qc2_5V(chip);
        pr_err("Force set qc2 5V\n");
    } else if (m_chg_type == 0x06) {
        wt6670f_force_qc3_5V(chip);
        hvdcp3_chgstat_notify();
        msleep(100);
        pr_err("Force set qc3 5V\n");
        /* set 5 plues for 18W*/
        wt6670f_set_qc3_volt_count(5);
        g_qc3_type_set = true;
    } else {
        //do nothing
    }
    /* set curr default after BC1.2*/
    charger_manager_set_qc_input_current_limit(chip->chg_consumer, 0, -1);
    charger_manager_set_qc_charging_current_limit(chip->chg_consumer, 0, -1);

    if (QC3P_Z350 == g_qc3p_id)
        power_supply_changed(chip->charger_psy);
    else if(QC3P_WT6670F == g_qc3p_id) {
        if(m_chg_type == 0x08 && m_chg_type == 0x09) {
            g_change_num++;
            if(g_change_num == 2) { //detect need second
                g_change_num = 0;
                power_supply_changed(chip->charger_psy);
            }
        }
    }
}

static irqreturn_t wt6670f_interrupt(int irq, void *dev_id)
{
    struct wt6670f_charger *chip = dev_id;
    int type;

    pr_info("%s: INT OCCURED\n", __func__);

    type = wt6670f_charger_type(chip);
    pr_info("%s: qc logic charger type is [%d]!\n", __func__, type);

    return IRQ_HANDLED;
}

int wt6670f_i2c_read_cmd(const struct i2c_client *client, char read_addr, const char *buf)
{
    int ret;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[2];
    char send_buf[2] = {0x61, 0x00};

    send_buf[1] = read_addr;

    msg[0].addr = WT6670F_ADDR;
    msg[0].len = 2;
    msg[0].buf = send_buf;

    msg[1].addr = WT6670F_ADDR;
    msg[1].flags = I2C_M_STOP & I2C_M_NOSTART;
    msg[1].flags |= I2C_M_RD;
    msg[1].len = 64;
    msg[1].buf = (char *)buf;

    ret = i2c_transfer(adap, &msg[0], 2);

    return ret;
}

int wt6670f_i2c_get_chip_id(const struct i2c_client *client, const char *buf)
{
    int ret;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[2];
    char send_buf[2] = {0x80, 0x00};

    msg[0].addr = WT6670F_ADDR;
    msg[0].len = 2;
    msg[0].buf = send_buf;

    msg[1].addr = WT6670F_ADDR;
    msg[1].flags = I2C_M_STOP & I2C_M_NOSTART;
    msg[1].flags |= I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = (char *)buf;

    ret = i2c_transfer(adap, &msg[0], 2);

    return ret;
}

int wt6670f_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
    int ret;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg;

    msg.addr = WT6670F_ADDR;
    msg.flags = 0x0000;
    msg.flags = msg.flags | I2C_M_STOP;
    msg.len = count;
    msg.buf = (char *)buf;

    ret = i2c_transfer(adap, &msg, 1);

    return (ret == 1) ? count : ret;
}

static int wt6670f_i2c_sequence_send(const struct i2c_client *client, const char *buf, int count)
{
    int ret;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[2];

    msg[0].addr = 0x2B;//WT6670F_ADDR;;
    msg[0].flags = 0x0000;
    msg[0].flags = msg[0].flags | I2C_M_NO_RD_ACK | I2C_M_IGNORE_NAK | I2C_M_NOSTART;
    msg[0].len = 1;
    msg[0].buf = (char *)buf;

    msg[1].addr = 0x48;//WT6670F_ADDR;;
    msg[1].flags = 0x0000;
    msg[1].flags = msg[1].flags | I2C_M_NO_RD_ACK | I2C_M_IGNORE_NAK | I2C_M_NOSTART;
    msg[1].len = 1;
    msg[1].buf = (char *)buf;

    ret = i2c_transfer(adap, &msg[0], 2);

    return (ret == 2) ? count : ret;

}

#define FIRWARE_SIZE                        0x40
#define ENABLE_ISP_CMD_LEN                  7
#define I2C_MASTER_SEND_LEN                 3
#define ISP_CHIP_ID                         0x70

void wt6670f_isp_flow(struct wt6670f_charger *chip)
{
    struct device *dev = chip->dev;
    unsigned int pos = 0;
    char chip_id = 0x00;
    int rc = 0;
    char i2c_buf[1] = {0};
    char enable_isp_cmd[7] = {0x57, 0x54, 0x36, 0x36, 0x37, 0x30, 0x46};
    char enable_isp_flash_mode_cmd[3] = {0x10, 0x02, 0x08};
    char chip_erase_cmd[3] = {0x20, 0x00, 0x00};
    char finish_cmd[3] = {0x00, 0x00, 0x00};
    char set_addr_high_byte_cmd[3] = {0x10, 0x01, 0x00};
    u8 program_cmd[66] = {0x00};
    char high_addr = 0;
    char low_addr = 0;
    char length = 0;
    int i = 0;
    int j = 0;
    char mem_data[64] = {0x00};
    unsigned int pos_file = 0;
    u8 *code = NULL;
    char flash_addr = 0;
    u8 file_version;
    int len = 0;
    unsigned int size = sizeof(wt6670f_qc3p_bin);

    code = kzalloc(FIRWARE_SIZE, GFP_KERNEL);
    if(code ==NULL){
        dev_err(chip->dev, "%s: code is null\n", __func__);
        return ;
    }

    rc = wt6670f_i2c_master_send(chip->client, enable_isp_cmd, ENABLE_ISP_CMD_LEN);
    if (rc != ENABLE_ISP_CMD_LEN)
        dev_err(chip->dev, "%s: enable_isp_cmd is failed, rc=%d\n", __func__, rc);

    rc = wt6670f_i2c_get_chip_id(chip->client, &chip_id);
    dev_err(chip->dev, "%s: rc=%d, chip_id=%02x\n", __func__, rc, chip_id);

    if (chip_id == ISP_CHIP_ID) {
        rc = wt6670f_i2c_master_send(chip->client, enable_isp_flash_mode_cmd, I2C_MASTER_SEND_LEN);
        dev_err(chip->dev, "%s: enable_isp_flash_mode_cmd, rc=%d\n", __func__, rc);

        rc = wt6670f_i2c_master_send(chip->client, chip_erase_cmd, I2C_MASTER_SEND_LEN);
        if (rc != I2C_MASTER_SEND_LEN) {
            dev_err(chip->dev, "%s: chip_erase_cmd is failed, rc=%d\n", __func__, rc);
            rc = ERROR_ERASE_FAILED;
            goto update_failed;
        }
        mdelay(20);

        rc = wt6670f_i2c_master_send(chip->client, finish_cmd, I2C_MASTER_SEND_LEN);
        if (rc != I2C_MASTER_SEND_LEN) {
            dev_err(chip->dev, "%s: finish_cmd is failed, rc=%d\n", __func__, rc);
            rc = ERROR_FINISH_CMD_FAILED;
            goto update_failed;
        }
        /* flash program 64 byte 256 count */
        while (pos < size) {
            high_addr = (pos >> 8) & 0x0f ;
            low_addr = pos % 256;
            length = FIRWARE_SIZE;
            dev_err(chip->dev, "%s: high_addr = %02x, low_addr = %02x, length=%d\n",
                    __func__, high_addr, low_addr, length);

             memset(code, 0, FIRWARE_SIZE);
             set_addr_high_byte_cmd[2] = high_addr;
             rc = wt6670f_i2c_master_send(chip->client, set_addr_high_byte_cmd, 3);
                if (rc != 3) {
                    dev_err(chip->dev, "%s: set_addr_high_byte_cmd is failed, rc=%d\n", __func__, rc);
                    rc = ERROR_HIGHADD_CMD_FAILED;
                    goto update_failed;
                }

                if((pos +FIRWARE_SIZE) > size)
                {
                    len = size % FIRWARE_SIZE;
                } else {
                    len = FIRWARE_SIZE;
                }

                for (i = 0; i < len; i++){
                    code[i] = wt6670f_qc3p_bin[pos+i];
                }

                if(len != FIRWARE_SIZE){
                    for (i = len; i < FIRWARE_SIZE; i++){
                        code[i] = 0xff;
                    }
                }
                /* high addr is 0x41 */
                program_cmd[0] = 0x41;
                program_cmd[1] = low_addr;
                memcpy(program_cmd + 2, code, FIRWARE_SIZE);
                rc = wt6670f_i2c_master_send(chip->client, program_cmd, FIRWARE_SIZE+2);
                if (rc != FIRWARE_SIZE+2) {
                    dev_err(chip->dev, "%s: program_cmd is failed, rc=%d\n", __func__, rc);
                    rc = ERROR_PROGRAM_CMD_FAILED;
                    goto update_failed;
                }

                rc = wt6670f_i2c_master_send(chip->client, finish_cmd, 3);
                if (rc != 3) {
                    dev_err(chip->dev, "%s: finish_cmd is failed, rc=%d\n", __func__, rc);
                    rc = ERROR_FINISH_CMD_FAILED;
                    goto update_failed;
                }
            pos = pos + length;
            dev_err(chip->dev, "%s: pos=%d\n", __func__, pos);
        }

        for (i = 0; i < 16; i++) {
            set_addr_high_byte_cmd[2] = i;
            rc = wt6670f_i2c_master_send(chip->client, set_addr_high_byte_cmd, 3);
            if (rc != 3) {
                dev_err(chip->dev, "%s: set_addr_high_byte_cmd is failed, rc=%d\n", __func__, rc);
                rc = ERROR_HIGHADD_CMD_FAILED;
                goto update_failed;
            }
            /* flash addr is 0x00, 0x40, 0x80, 0xc0 */
            flash_addr = 0x00;
            wt6670f_i2c_read_cmd(chip->client, flash_addr, mem_data);
            for (j = 0; j < 64; j++) {
                dev_err(chip->dev, "%s: mem_data[%d]=%02x\n", __func__, j + flash_addr + i*256, mem_data[j]);
                dev_err(chip->dev, "%s: wt6670f_qc3p_bin[%d]=%02x\n", __func__, j + flash_addr + i*256, wt6670f_qc3p_bin[j + flash_addr + i*256]);
                if (wt6670f_qc3p_bin[j + flash_addr + i*256] != mem_data[j]) {
                    dev_err(chip->dev, "%s: flash data is wrong.\n", __func__);
                   rc = ERROR_CALLBACK_FAILED;
                   goto update_failed;
                }
            }

            flash_addr = 0x40;
            wt6670f_i2c_read_cmd(chip->client, flash_addr, mem_data);
            for (j = 0; j < 64; j++) {
                dev_dbg(chip->dev, "%s: mem_data[%d]=%02x\n", __func__, j + flash_addr + i*256, mem_data[j]);
                dev_dbg(chip->dev, "%s: wt6670f_qc3p_bin[%d]=%02x\n", __func__, j + flash_addr + i*256, wt6670f_qc3p_bin[j + flash_addr + i*256]);
                if (wt6670f_qc3p_bin[j + flash_addr + i*256] != mem_data[j]) {
                    dev_err(chip->dev, "%s: flash data is wrong.\n", __func__);
                    rc = ERROR_CALLBACK_FAILED;
                    goto update_failed;
                }
            }

            flash_addr = 0x80;
            wt6670f_i2c_read_cmd(chip->client, flash_addr, mem_data);
            for (j = 0; j < 64; j++) {
                dev_dbg(chip->dev, "%s: mem_data[%d]=%02x\n", __func__, j + flash_addr + i*256, mem_data[j]);
                dev_dbg(chip->dev, "%s: wt6670f_qc3p_bin[%d]=%02x\n", __func__, j + flash_addr + i*256, wt6670f_qc3p_bin[j + flash_addr + i*256]);
                if (wt6670f_qc3p_bin[j + flash_addr + i*256] != mem_data[j]) {
                    dev_err(chip->dev, "%s: flash data is wrong.\n", __func__);
                    rc = ERROR_CALLBACK_FAILED;
                    goto update_failed;
                }
            }

            flash_addr = 0xC0;
            wt6670f_i2c_read_cmd(chip->client, flash_addr, mem_data);
            for (j = 0; j < 64; j++) {
                dev_dbg(chip->dev, "%s: mem_data[%d]=%02x\n", __func__, j + flash_addr + i*256, mem_data[j]);
                dev_dbg(chip->dev, "%s: wt6670f_qc3p_bin[%d]=%02x\n", __func__, j + flash_addr + i*256, wt6670f_qc3p_bin[j + flash_addr + i*256]);
                if (wt6670f_qc3p_bin[j + flash_addr + i*256] != mem_data[j]) {
                    dev_err(chip->dev, "%s: flash data is wrong.\n", __func__);
                    rc = ERROR_CALLBACK_FAILED;
                    goto update_failed;
                }
            }
        }

        rc = UPDATE_SUCCESS;
    } else {
        dev_err(chip->dev, "%s: chip id is not right, and end update.\n", __func__);
        rc = ERROR_GET_CHIPID_FAILED;
        goto update_failed;
    }

update_failed:
    dev_err(chip->dev, "%s: chip id is %d ,end update. 0--success\n", __func__, rc);
    wt6670f_do_reset(chip);
    kfree(code);
    return ;
}

static int wt6670f_parse_dt(struct wt6670f_charger *chip)
{
    struct wt6670f_desc *desc = NULL;
    struct device_node *np = chip->dev->of_node;

    if (!np) {
        dev_err(chip->dev, "%s: device tree info missing.\n", __func__);
        return -EINVAL;
    }

    chip->rst_gpio = of_get_named_gpio(np,
            "wt,wt6670f_rst_gpio", 0);
    if ((!gpio_is_valid(chip->rst_gpio))) {
        dev_err(chip->dev, "%s: no wt6670f_rst_gpio\n", __func__);
        return -EINVAL;
    }
    dev_dbg(chip->dev,"rst_gpio: %d\n", chip->rst_gpio);

    chip->int_gpio = of_get_named_gpio(np,
            "wt,wt6670f_int_gpio", 0);
    if ((!gpio_is_valid(chip->int_gpio))) {
        dev_err(chip->dev, "%s: no wt6670f_int_gpio\n", __func__);
        return -EINVAL;
    }
    dev_dbg(chip->dev,"int_gpio: %d\n", chip->int_gpio);

    chip->wt6670f_sda_gpio = of_get_named_gpio(np,
            "wt,wt6670f_sda_gpio", 0);
    if ((!gpio_is_valid(chip->wt6670f_sda_gpio))) {
        dev_err(chip->dev, "%s: no wt6670f_sda_gpio\n", __func__);
    }
    dev_dbg(chip->dev,"wt6670f_sda_gpio: %d\n", chip->wt6670f_sda_gpio);

    chip->wt6670f_scl_gpio = of_get_named_gpio(np,
            "wt,wt6670f_scl_gpio", 0);
    if ((!gpio_is_valid(chip->wt6670f_scl_gpio))) {
        dev_err(chip->dev, "%s: no wt6670f_scl_gpio\n", __func__);
    }
    dev_dbg(chip->dev,"wt6670f_scl_gpio: %d\n", chip->wt6670f_scl_gpio);

    chip->connect_therm_gpio = of_get_named_gpio(np, "mi,connect_therm", 0);
    if ((!gpio_is_valid(chip->connect_therm_gpio))) {
        dev_err(chip->dev, "%s: no connect_therm_gpio\n", __func__);
  //      return -EINVAL;
    }
    dev_dbg(chip->dev,"connect_therm_gpio: %d\n", chip->connect_therm_gpio);

    chip->bc12_unsupported = of_property_read_bool(np,
            "wt,bc12_unsupported");
    chip->hvdcp_unsupported = of_property_read_bool(np,
            "wt,hvdcp_unsupported");
    chip->intb_unsupported = of_property_read_bool(np,
            "wt,intb_unsupported");
    chip->sleep_unsupported = of_property_read_bool(np,
            "wt,sleep_unsupported");

    chip->desc = &wt6670f_default_desc;

    desc = devm_kzalloc(chip->dev, sizeof(struct wt6670f_desc), GFP_KERNEL);
    if (!desc)
        return -ENOMEM;

    memcpy(desc, &wt6670f_default_desc, sizeof(struct wt6670f_desc));

    if (of_property_read_string(np, "charger_name",
                &desc->chg_dev_name) < 0)
        dev_err(chip->dev, "%s: dts no charger name\n", __func__);

    if (of_property_read_string(np, "alias_name", &desc->alias_name) < 0)
        dev_err(chip->dev, "%s: no alias name\n", __func__);

    desc->en_bc12 = of_property_read_bool(np, "en_bc12");
    desc->en_hvdcp = of_property_read_bool(np, "en_hvdcp");
    desc->en_intb = of_property_read_bool(np, "en_intb");
    desc->en_sleep = of_property_read_bool(np, "en_sleep");

    chip->desc = desc;
    chip->chg_props.alias_name = chip->desc->alias_name;
    dev_err(chip->dev, "%s: chg_name:%s alias:%s\n", __func__,
            chip->desc->chg_dev_name, chip->chg_props.alias_name);

    return 0;
}

static int isp_pinctrl_enable(struct wt6670f_charger *chip)
{
    char chip_id = 0x00;
    int rc = 0;
    u16 version = 0;
    char i2c_buf_enable_cmd[7] = {0x57, 0x54, 0x36, 0x36, 0x37, 0x30, 0x46};

    rc = wt6670f_get_firmware_version(chip, &version);
    if ((rc) || (version != WT6670F_FIRMWARE_VERSION)) {
        pr_info("get version failed and will update firmware, rc=%d.\n", rc);

        dev_err(chip->dev, "%s: step 1.1: select pinstate\n", __func__);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_scl_isp);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_sda_isp);
        if (rc < 0) {
            dev_err(chip->dev,
                "%s: Failed to select isp pinstate %d\n",
                __func__, rc);
            goto error;
        }

        dev_err(chip->dev, "%s: step 1.2: request 2 gpio\n", __func__);
        rc = devm_gpio_request(chip->dev,
                    chip->wt6670f_sda_gpio, "wt6670f_sda_gpio");
        if (rc) {
            pr_err("request wt6670f_sda_gpio failed, rc=%d\n",
                    rc);
            goto error;
        }
        rc = devm_gpio_request(chip->dev,
                    chip->wt6670f_scl_gpio, "wt6670f_scl_gpio");
        if (rc) {
            pr_err("request wt6670f_scl_gpio failed, rc=%d\n",
                    rc);
            goto error;
        }

        mutex_lock(&chip->isp_sequence_lock);
        dev_err(chip->dev, "%s: step 2\n", __func__);
        gpio_set_value(chip->rst_gpio, 1);
        mdelay(11);
        gpio_set_value(chip->rst_gpio, 0);
        mdelay(3);

        gpio_direction_output(chip->wt6670f_sda_gpio, 0);
        gpio_direction_output(chip->wt6670f_scl_gpio, 0);

        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:0
        udelay(5);

        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 1);// sda:[1]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        gpio_set_value(chip->wt6670f_sda_gpio, 0);// sda:[0]
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 1);// scl:1
        udelay(5);
        gpio_set_value(chip->wt6670f_scl_gpio, 0);// scl:0

        mdelay(10);

        devm_gpio_free(chip->dev,
                    chip->wt6670f_sda_gpio);
        devm_gpio_free(chip->dev,
                    chip->wt6670f_scl_gpio);
        dev_err(chip->dev, "%s: step 2.1: select pinstate normal to i2c.\n", __func__);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_scl_normal);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_sda_normal);
        if (rc < 0) {
            dev_err(chip->dev,
                "%s: Failed to select normal pinstate %d\n",
                __func__, rc);
            goto error;
        }
        mutex_unlock(&chip->isp_sequence_lock);
        dev_err(chip->dev, "%s: step 3\n", __func__);
        rc = wt6670f_i2c_master_send(chip->client, i2c_buf_enable_cmd, 7);
        dev_err(chip->dev, "%s: rc=%d, step3 chip_id=%02x\n", __func__, rc, chip_id);

        dev_err(chip->dev, "%s: step 4\n", __func__);
        rc = wt6670f_i2c_get_chip_id(chip->client, &chip_id);
        dev_err(chip->dev, "%s: rc=%d, step4 chip_id=%02x\n", __func__, rc, chip_id);
    }
    return 0;
error:
    pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_sda_normal);
    pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_scl_normal);
    devm_gpio_free(chip->dev,
                chip->wt6670f_sda_gpio);
    devm_gpio_free(chip->dev,
                chip->wt6670f_scl_gpio);
    return rc;
}

static int wt6670f_pinctrl_init(struct wt6670f_charger *chip)
{
    int ret;

    chip->wt6670f_pinctrl = devm_pinctrl_get(chip->dev);
    if (IS_ERR_OR_NULL(chip->wt6670f_pinctrl)) {
        ret = PTR_ERR(chip->wt6670f_pinctrl);
        dev_err(chip->dev,
            "Target does not use pinctrl %d\n", ret);
        goto err_pinctrl_get;
    }
    chip->pinctrl_state_normal
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "rst_normal");
    if (IS_ERR_OR_NULL(chip->pinctrl_state_normal)) {
        ret = PTR_ERR(chip->pinctrl_state_normal);
        dev_err(chip->dev,
            "Can not lookup wt6670f_normal pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    chip->pinctrl_state_isp
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "rst_isp");
    if (IS_ERR_OR_NULL(chip->pinctrl_state_isp)) {
        ret = PTR_ERR(chip->pinctrl_state_isp);
        dev_err(chip->dev,
            "Can not lookup wt6670f_isp  pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    chip->pinctrl_scl_normal
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "scl_normal");
    if (IS_ERR_OR_NULL(chip->pinctrl_scl_normal)) {
        ret = PTR_ERR(chip->pinctrl_scl_normal);
        dev_err(chip->dev,
            "Can not lookup scl_normal pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    chip->pinctrl_scl_isp
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "scl_isp");
    if (IS_ERR_OR_NULL(chip->pinctrl_scl_isp)) {
        ret = PTR_ERR(chip->pinctrl_scl_isp);
        dev_err(chip->dev,
            "Can not lookup scl_isp  pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    chip->pinctrl_sda_normal
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "sda_normal");
    if (IS_ERR_OR_NULL(chip->pinctrl_sda_normal)) {
        ret = PTR_ERR(chip->pinctrl_sda_normal);
        dev_err(chip->dev,
            "Can not lookup sda_normal pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    chip->pinctrl_sda_isp
        = pinctrl_lookup_state(chip->wt6670f_pinctrl,
                    "sda_isp");
    if (IS_ERR_OR_NULL(chip->pinctrl_sda_isp)) {
        ret = PTR_ERR(chip->pinctrl_sda_isp);
        dev_err(chip->dev,
            "Can not lookup sda_isp  pinstate %d\n",
            ret);
        goto err_pinctrl_lookup;
    }

    return 0;

err_pinctrl_get:
    devm_pinctrl_put(chip->wt6670f_pinctrl);
err_pinctrl_lookup:
    chip->wt6670f_pinctrl = NULL;

    return ret;
}

static int wt6670f_charger_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int rc;
    u16 firmware_version = 0;
    int wt6670f_id = 0;
    //bool qc3p_z350_init_ok = false;//100021221
    u16 is_already_probe_ok = 0;
    struct wt6670f_charger *chip = NULL;
    int irqn = 0;
    bool penang_unsupport = false;
    struct device_node *np = NULL;

    pr_info("%s: probe start!\n", __func__);
    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip) {
        pr_err("Couldn't allocate memory\n");
        return -ENOMEM;
    }

    _chip = chip;

    chip->client = client;
    chip->dev = &client->dev;
    mutex_init(&chip->irq_complete);
    mutex_init(&chip->isp_sequence_lock);

    chip->thermal_status = TEMP_BELOW_RANGE;
    chip->rerun_apsd_count = 0;
    chip->tcpc_attach = false;
    chip->irq_data_process_enable = false;
    chip->irq_waiting = false;
    chip->dpdm_mode = QC30_DPDM;
    chip->resume_completed = true;
    chip->hvdcp_en = true;
    np = chip->dev->of_node; 
    penang_unsupport = of_property_read_bool(np, "wt,penang_unsupport");
    if (penang_unsupport) {
        pr_err("wt6670 not support\n");
        return 0;
    }
    i2c_set_clientdata(client, chip);

    wt6670f_set_usbsw_state(chip, USBSW_CHG);
    rc = wt6670f_parse_dt(chip);
    if (rc) {
        pr_err("Couldn't parse DT nodes rc=%d\n", rc);
        goto err_mutex_init;
    }

    rc = devm_gpio_request(&client->dev,
            chip->rst_gpio, "wt6670f reset gpio");
    if (rc) {
        pr_err("request wt6670f reset gpio failed, rc=%d\n",
                rc);
        goto err_mutex_init;
    }
    gpio_direction_output(chip->rst_gpio, 1);
    gpio_set_value(chip->rst_gpio, 0);

    rc = devm_gpio_request(&client->dev,
            chip->int_gpio, "wt6670f int gpio");
    if (rc) {
        pr_err("request wt6670f int gpio failed, rc=%d\n",
                rc);
        goto err_mutex_init;
    }

    gpio_direction_input(chip->int_gpio);
    irqn = gpio_to_irq(chip->int_gpio);
    if (irqn < 0) {
        dev_err(chip->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
        return irqn;
    }
    chip->client->irq = irqn;
    pr_info("request wt6670f int gpio irqn=%d\n",irqn);

    chip->bms_psy = power_supply_get_by_name("battery");
    if (!chip->bms_psy) {
        pr_err("%s: get battery power supply failed\n", __func__);
        rc = -EPROBE_DEFER;
        return rc;
    }

    chip->usb_psy = power_supply_get_by_name("usb");
    if (!chip->bms_psy) {
        pr_err("%s: get usb power supply failed\n", __func__);
        rc = -EPROBE_DEFER;
        return rc;
    }

    chip->charger_psy = power_supply_get_by_name("sgm4154x-charger");
    if(!chip->charger_psy) {
        pr_err("%s: get charger power supply failed\n", __func__);
        rc = -EPROBE_DEFER;
        return rc;
    }
    //charger_dev_set_charging_current(chip->chg_dev_p, QC3_DP_DM_PR_FCC);

    chip->chg_dev = get_charger_by_name("primary_chg");
    if (!chip->chg_dev) {
        pr_err("%s: get chrg device failed\n", __func__);
        rc = -EPROBE_DEFER;
        return rc;
    }

    chip->chg_consumer = charger_manager_get_by_name(chip->dev, "primary_chg");
    if(!chip->chg_consumer) {
        pr_err("%s: get chg_consumer failed\n", __func__);
    }

    wt6670f_do_reset(chip);
    wt6670f_id = wt6670f_get_id(chip, 0x13);
    if(0x3349 == wt6670f_id) {
        g_qc3p_id = QC3P_Z350;
        qc3p_z350_init_ok = true;
        gpio_direction_output(chip->reset_pin, 1);
        pr_info("[%s] is z350\n", __func__);
        is_already_probe_ok = 1;
        goto probe_out;
    } 

    rc = wt6670f_pinctrl_init(chip);
    if (!rc && chip->wt6670f_pinctrl) {
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_state_isp);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_scl_normal);
        rc = pinctrl_select_state(chip->wt6670f_pinctrl,
                    chip->pinctrl_sda_normal);
        if (rc < 0)
            dev_err(&client->dev,
                "%s: Failed to select active pinstate %d\n",
                __func__, rc);
    }
    gpio_direction_output(chip->rst_gpio, 0);
    gpio_set_value(chip->rst_gpio, 0);
    mdelay(15);

    rc = wt6670f_get_firmware_version(chip, &firmware_version);
    pinctrl_select_state(chip->wt6670f_pinctrl,
                chip->pinctrl_scl_isp);
    pinctrl_select_state(chip->wt6670f_pinctrl,
                chip->pinctrl_sda_isp);
    mdelay(15);

    wt6670f_reset_chg_type();
    pr_info("[%s] firmware_version= 0x%x, chg_type = %d\n", __func__, firmware_version, chip->chg_type);
    if(firmware_version != WT6670F_FIRMWARE_VERSION) {
        pr_info("[%s]: firmware need upgrade run wt6670f_isp again!\n", __func__);
        wt6670f_do_reset(chip);
        isp_pinctrl_enable(chip);
        wt6670f_isp_flow(chip);
        //wt6670f_do_reset(chip);
        pr_info("[%s]: firmware run wt6670f_isp end!\n", __func__);
    }
    pinctrl_select_state(chip->wt6670f_pinctrl,
                chip->pinctrl_scl_normal);
    pinctrl_select_state(chip->wt6670f_pinctrl,
                chip->pinctrl_sda_normal);
    mdelay(5);

    rc = wt6670f_get_firmware_version(chip, &firmware_version);
    pr_info("[%s] firmware_version= 0x%x, rc = %d\n", __func__, firmware_version, rc);
    if (firmware_version == WT6670F_FIRMWARE_VERSION) {
        g_qc3p_id = QC3P_WT6670F;
        is_already_probe_ok = 1;
        pr_info("[%s] is wt6670f, wt6670f_id= %x\n", __func__, g_qc3p_id);
    }

    pr_info("wt6670f successfully probed.\n");
    return 0;

probe_out:
    return 0;
err_mutex_init:
    mutex_destroy(&chip->irq_complete);
    mutex_destroy(&chip->isp_sequence_lock);
    return rc;
}

static int wt6670f_charger_remove(struct i2c_client *client)
{
    struct wt6670f_charger *chip = i2c_get_clientdata(client);

    pr_info("%s: remove\n", __func__);
    mutex_destroy(&chip->irq_complete);
    mutex_destroy(&chip->isp_sequence_lock);
    chip->hvdcp_en = true;
#if 0
    cancel_delayed_work_sync(&chip->charger_type_det_work);
    cancel_delayed_work_sync(&chip->chip_update_work);
    cancel_delayed_work_sync(&chip->hvdcp_det_retry_work);
#endif
    return 0;
}

static int wt6670f_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct wt6670f_charger *chip = i2c_get_clientdata(client);

//    cancel_delayed_work_sync(&chip->conn_therm_work);

    mutex_lock(&chip->irq_complete);
    chip->resume_completed = false;
    mutex_unlock(&chip->irq_complete);

    return 0;
}

static int wt6670f_suspend_noirq(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct wt6670f_charger *chip = i2c_get_clientdata(client);

    if (chip->irq_waiting) {
        pr_err_ratelimited("Aborting suspend, an interrupt was detected while suspending\n");
        return -EBUSY;
    }
    return 0;
}

static int wt6670f_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct wt6670f_charger *chip = i2c_get_clientdata(client);
//  union power_supply_propval val;
//  int usb_present = 0;

    mutex_lock(&chip->irq_complete);
    chip->resume_completed = true;
    if (chip->irq_waiting) {
        mutex_unlock(&chip->irq_complete);
        enable_irq(client->irq);
    } else {
        mutex_unlock(&chip->irq_complete);
    }
    return 0;
}

static const struct dev_pm_ops wt6670f_pm_ops = {
    .suspend    = wt6670f_suspend,
    .suspend_noirq  = wt6670f_suspend_noirq,
    .resume     = wt6670f_resume,
};

static void wt6670f_charger_shutdown(struct i2c_client *client)
{
    struct wt6670f_charger *chip = i2c_get_clientdata(client);

    pr_info("%s: shutdown\n", __func__);

    if(g_qc3p_id == QC3P_Z350){
        wt6670f_write_byte(chip, Z350_REG_SOFT_RESET, SOFT_RESET_VAL);
        wt6670f_write_byte(chip, Z350_REG_BC1P2, 0x00);
    } else {
        wt6670f_write_byte(chip, WT6670F_REG_SOFT_RESET, SOFT_RESET_VAL);
        pr_info("[%s] wt6670 soft reset\n", __func__);
    }
    chip->hvdcp_en = true;
#if 0
    cancel_delayed_work_sync(&chip->charger_type_det_work);
    cancel_delayed_work_sync(&chip->chip_update_work);
    cancel_delayed_work_sync(&chip->hvdcp_det_retry_work);
#endif
    mutex_destroy(&chip->irq_complete);
    mutex_destroy(&chip->isp_sequence_lock);
}

static const struct of_device_id wt6670f_match_table[] = {
    { .compatible = "wt,wt6670f_charger",},
    { },
};

static const struct i2c_device_id wt6670f_charger_id[] = {
    {"wt6670f_charger", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, wt6670f_charger_id);

static struct i2c_driver wt6670f_charger_driver = {
    .driver     = {
        .name       = "wt6670f_charger",
        .owner      = THIS_MODULE,
        .of_match_table = wt6670f_match_table,
        .pm     = &wt6670f_pm_ops,
    },
    .probe      = wt6670f_charger_probe,
    .remove     = wt6670f_charger_remove,
    .id_table   = wt6670f_charger_id,
    .shutdown   = wt6670f_charger_shutdown,
};

module_i2c_driver(wt6670f_charger_driver);

MODULE_AUTHOR("bsp@hq.com");
MODULE_DESCRIPTION("wt6670f Charger");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("i2c:wt6670f-charger");

