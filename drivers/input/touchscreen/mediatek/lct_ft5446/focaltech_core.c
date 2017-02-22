/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*****************************************************************************
*
* File Name: focaltech_core.c

* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract: entrance for focaltech ts driver
*
* Version: V1.0
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
//#include <linux/rtpm_prio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include "focaltech_core.h"
#include "tpd.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "fts_ts"
#define INTERVAL_READ_REG                   20  //interval time per read reg unit:ms
#define TIMEOUT_READ_REG                    300 //timeout of read reg unit:ms
#define FTS_I2C_SLAVE_ADDR                  0x38
#define FTS_READ_TOUCH_BUFFER_DIVIDED       1
#define MTK_CTP_NODE						1	//add by hxl tp_infp node
/*****************************************************************************
* Static variables
*****************************************************************************/
struct i2c_client *fts_i2c_client;
struct input_dev *fts_input_dev;
struct task_struct *thread_tpd;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag;

#if FTS_DEBUG_EN
int g_show_log = 1;
#else
int g_show_log = 0;
#endif

unsigned int tpd_rst_gpio_number = 0;
unsigned int tpd_int_gpio_number = 1;
unsigned int ft_touch_irq = 0;

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
static int tpd_def_calmat_local_normal[8] =
    TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] =
    TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client,
                          struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);
static void fts_release_all_finger(void);

//add by hxl dev_info
u8 ver;
static u8 ctp_fw_version;
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define SLT_DEVINFO_CTP_DEBUG
#include  <dev_info.h>
u8 ver_id;
u8 ver_module;
//static int devinfo_first=0;
static char* temp_ver;
static struct devinfo_struct s_DEVINFO_ctp;   //suppose 10 max lcm device 
//The followd code is for GTP style
static void devinfo_ctp_regchar(char *module,char * vendor,char *version,char *used)
{
	//s_DEVINFO_ctp =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_ctp.device_type="CTP";
	s_DEVINFO_ctp.device_module=module;
	s_DEVINFO_ctp.device_vendor=vendor;
	s_DEVINFO_ctp.device_ic="FT5446";
	s_DEVINFO_ctp.device_info=DEVINFO_NULL;
	s_DEVINFO_ctp.device_version=version;
	s_DEVINFO_ctp.device_used=used;
#ifdef SLT_DEVINFO_CTP_DEBUG
	printk("[DEVINFO CTP]registe CTP device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s> \n",
		s_DEVINFO_ctp.device_type,s_DEVINFO_ctp.device_module,s_DEVINFO_ctp.device_vendor,
		s_DEVINFO_ctp.device_ic,s_DEVINFO_ctp.device_version,s_DEVINFO_ctp.device_info,
                s_DEVINFO_ctp.device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_ctp.device_type,s_DEVINFO_ctp.device_module,s_DEVINFO_ctp.device_vendor,
                                    s_DEVINFO_ctp.device_ic,s_DEVINFO_ctp.device_version,s_DEVINFO_ctp.device_info,
                                    s_DEVINFO_ctp.device_used);
}
#endif

/*****************************************************************************
* Focaltech ts i2c driver configuration
*****************************************************************************/
static const struct i2c_device_id fts_tpd_id[] = { {FTS_DRIVER_NAME, 0}, {} };

static const struct of_device_id fts_dt_match[] = {
    {.compatible = "mediatek,ft_cap_touch"},
    {},
};

MODULE_DEVICE_TABLE(of, fts_dt_match);

static struct i2c_driver tpd_i2c_driver = {
    .driver = {
               .name = FTS_DRIVER_NAME,
               .of_match_table = of_match_ptr(fts_dt_match),
               },
    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = fts_tpd_id,
    .detect = tpd_i2c_detect,
};

/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief:   Read chip id until TP FW become valid, need call when reset/power on/resume...
*           1. Read Chip ID per INTERVAL_READ_REG(20ms)
*           2. Timeout: TIMEOUT_READ_REG(400ms)
*  Input:
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_wait_tp_to_valid(struct i2c_client *client)
{
    int ret1 = 0;
	int ret2 = 0;
    int cnt = 0;
    u8 reg_value = 0;

    do {
        ret1 = fts_i2c_read_reg(client, FTS_REG_CHIP_ID, &reg_value);
        if ((ret1 < 0) || (reg_value != chip_types.chip_idh)) {
            FTS_INFO("TP Not Ready, ReadData = 0x%x", reg_value);
    		fts_reset_proc(300);
        }
        else if (reg_value == chip_types.chip_idh) {
            FTS_INFO("TP Ready, Device ID = 0x%x", reg_value);
            return 0;
        }
        cnt++;
        msleep(INTERVAL_READ_REG);
    }
    while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);
   ret2 = fts_ctpm_fw_upgrade_ReadBootloadorID(client);
   if(!ret2)
   {
      FTS_INFO("TP Need Upgrade, ret2 = 0x%x", ret2);
      return 0;
   }
    /* error: not get correct reg data */
    return -1;
}

/*****************************************************************************
*  Name: fts_recover_state
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct i2c_client *client)
{
    /* wait tp stable */
    fts_wait_tp_to_valid(client);
    /* recover TP charger state 0x8B */
    /* recover TP glove state 0xC0 */
    /* recover TP cover state 0xC1 */
    fts_ex_mode_recovery(client);

#if FTS_PSENSOR_EN
    fts_proximity_recovery(client);
#endif

    /* recover TP gesture state 0xD0 */
#if FTS_GESTURE_EN
    fts_gesture_recovery(client);
#endif

    fts_release_all_finger();
}

/*****************************************************************************
*  Name: fts_reset_proc
*  Brief: Execute reset operation
*  Input: hdelayms - delay time unit:ms
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_reset_proc(int hdelayms)
{
	int retval;

    retval = regulator_disable(tpd->reg);
    if (retval != 0) {
        FTS_ERROR("[POWER]Fail to disable regulator when init,ret=%d!", retval);
        return retval;
    }

    tpd_gpio_output(tpd_rst_gpio_number, 0);
    msleep(2);

	retval = regulator_enable(tpd->reg);
    if (retval != 0) {
        FTS_ERROR("[POWER]Fail to enable regulator when init,ret=%d!", retval);
        return retval;
    }

    msleep(5);
    tpd_gpio_output(tpd_rst_gpio_number, 1);
    msleep(hdelayms);

    return 0;
}

#if FTS_POWER_SOURCE_CUST_EN
/*****************************************************************************
* Power Control
*****************************************************************************/
int fts_power_init(void)
{
    int retval;
    /*set TP volt */
	if( tpd->reg == NULL)
	{
		tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
		if (IS_ERR(tpd->reg))
			FTS_ERROR("regulator_get() failed!\n");
		retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
		if (retval != 0) {
			FTS_ERROR("[POWER]Failed to set voltage of regulator,ret=%d!", retval);
			return retval;
		}

//		retval = regulator_enable(tpd->reg);
//		if (retval != 0) {
//			FTS_ERROR("[POWER]Fail to enable regulator when init,ret=%d!", retval);
//			return retval;
//		}
	}
    return 0;
}

void fts_power_suspend(void)
{
    int retval;

    retval = regulator_disable(tpd->reg);
    if (retval != 0)
        FTS_ERROR("[POWER]Failed to disable regulator when suspend ret=%d!",
                  retval);
}

//int fts_power_resume(void)
//{
//    int retval = 0;
//
//    retval = regulator_enable(tpd->reg);
//    if (retval != 0)
//        FTS_ERROR("[POWER]Failed to enable regulator when resume,ret=%d!",
//                  retval);
//
//    return retval;
//}
#endif

/*****************************************************************************
*  Reprot related
*****************************************************************************/
/*****************************************************************************
*  Name: fts_release_all_finger
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_release_all_finger(void)
{
#if FTS_MT_PROTOCOL_B_EN
    unsigned int finger_count = 0;
#endif

    FTS_FUNC_ENTER();
#if (!FTS_MT_PROTOCOL_B_EN)
    input_mt_sync(tpd->dev);
#else
    for (finger_count = 0; finger_count < FTS_MAX_POINTS; finger_count++) {
        input_mt_slot(tpd->dev, finger_count);
        input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
    }
    input_report_key(tpd->dev, BTN_TOUCH, 0);
#endif
    input_sync(tpd->dev);
    FTS_FUNC_EXIT();
}

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
char g_sz_debug[1024] = { 0 };
#endif

#if (!FTS_MT_PROTOCOL_B_EN)
static void tpd_down(int x, int y, int p, int id)
{
    int pressure;

    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
    input_report_key(tpd->dev, BTN_TOUCH, 1);

    if (FTS_REPORT_PRESSURE_EN) {
        if (FTS_FORCE_TOUCH_EN) {
            if (p > 0) {
                pressure = p;
            }
            else {
                FTS_ERROR("[A]Illegal pressure: %d", p);
                pressure = 1;
            }
        }
        else {
            pressure = 0x3f;
        }
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, pressure);
    }

    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);

    if (FTS_REPORT_PRESSURE_EN) {
        printk("[A]P%d(%4d,%4d)[p:%d] DOWN!", id, x, y, pressure);
    }
    else {
        printk("[A]P%d(%4d,%4d) DOWN!", id, x, y);
    }
}

static void tpd_up(int x, int y)
{
    FTS_DEBUG("[A]All Up!");
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_mt_sync(tpd->dev);
}

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
    int i = 0;
    int ret;
    u8 data[POINT_READ_BUF] = { 0 };
    u16 high_byte, low_byte;
    char writebuf[10] = { 0 };

#if FTS_READ_TOUCH_BUFFER_DIVIDED
    u8 pointnum;
    memset(data, 0xff, POINT_READ_BUF);
    ret = fts_i2c_read(fts_i2c_client, writebuf, 1, data, 3);
    if (ret < 0) {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        return ret;
    }

    pointnum = data[2] & 0x0f;
    if (pointnum > 0) {
        writebuf[0] = 0x03;
        ret =
            fts_i2c_read(fts_i2c_client, writebuf, 1, data + 3,
                         pointnum * FTS_TOUCH_STEP);
        if (ret < 0) {
            FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
            return ret;
        }
    }
#else
    ret = fts_i2c_read(fts_i2c_client, writebuf, 1, data, POINT_READ_BUF);
    if (ret < 0) {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        FTS_FUNC_EXIT();
        return ret;
    }
#endif

    if ((data[0] & 0x70) != 0)
        return false;

    memcpy(pinfo, cinfo, sizeof(struct touch_info));
    memset(cinfo, 0, sizeof(struct touch_info));
    for (i = 0; i < FTS_MAX_POINTS; i++)
        cinfo->p[i] = 1;        /* Put up */

    /*get the number of the touch points */
    cinfo->count = data[2] & 0x0f;
    FTS_DEBUG("Number of touch points = %d", cinfo->count);

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    {
        int len = cinfo->count * FTS_TOUCH_STEP;
        int count = 0;
        memset(g_sz_debug, 0, 1024);
        if (len > (POINT_READ_BUF - 3)) {
            len = POINT_READ_BUF - 3;
        }
        else if (len == 0) {
            len += FTS_TOUCH_STEP;
        }
        count +=
            sprintf(g_sz_debug, "%02X,%02X,%02X", data[0], data[1], data[2]);
        for (i = 0; i < len; i++) {
            count += sprintf(g_sz_debug + count, ",%02X", data[i + 3]);
        }
        FTS_DEBUG("buffer: %s", g_sz_debug);
    }
#endif
    for (i = 0; i < cinfo->count; i++) {
        cinfo->p[i] = (data[3 + 6 * i] >> 6) & 0x0003;  /* event flag */
        cinfo->id[i] = data[3 + 6 * i + 2] >> 4;    // touch id

        /*get the X coordinate, 2 bytes */
        high_byte = data[3 + 6 * i];
        high_byte <<= 8;
        high_byte &= 0x0F00;

        low_byte = data[3 + 6 * i + 1];
        low_byte &= 0x00FF;
        cinfo->x[i] = high_byte | low_byte;

        /*get the Y coordinate, 2 bytes */
        high_byte = data[3 + 6 * i + 2];
        high_byte <<= 8;
        high_byte &= 0x0F00;

        low_byte = data[3 + 6 * i + 3];
        low_byte &= 0x00FF;
        cinfo->y[i] = high_byte | low_byte;

        FTS_DEBUG(" cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d\n",
                  i, cinfo->x[i], i, cinfo->y[i], i, cinfo->p[i]);
    }

    return true;
};
#else
/************************************************************************
* Name: fts_read_touchdata
* Brief: report the point information
* Input: event info
* Output: get touch data in pinfo
* Return: success is zero
***********************************************************************/
static int fts_read_touchdata(struct ts_event *data)
{
    u8 buf[POINT_READ_BUF] = { 0 };
    int ret = -1;
    int i = 0;
    u8 pointid = FTS_MAX_ID;

    FTS_FUNC_ENTER();
#if FTS_READ_TOUCH_BUFFER_DIVIDED
    memset(buf, 0xff, POINT_READ_BUF);
    buf[0] = 0x0;
    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 3);
    if (ret < 0) {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        return ret;
    }
    ret = data->touchs;
    memset(data, 0, sizeof(struct ts_event));
    data->touchs = ret;
    data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
    buf[3] = 0x03;
    if (data->touch_point_num > 0) {
        ret =
            fts_i2c_read(fts_i2c_client, buf + 3, 1, buf + 3,
                         data->touch_point_num * FTS_TOUCH_STEP);
        if (ret < 0) {
            FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
            return ret;
        }
    }
#else
    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, POINT_READ_BUF);
    if (ret < 0) {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        FTS_FUNC_EXIT();
        return ret;
    }
    memset(data, 0, sizeof(struct ts_event));
    data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
#endif
    data->touch_point = 0;

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    {
        int len = data->touch_point_num * FTS_TOUCH_STEP;
        int count = 0;
        memset(g_sz_debug, 0, 1024);
        if (len > (POINT_READ_BUF - 3)) {
            len = POINT_READ_BUF - 3;
        }
        else if (len == 0) {
            len += FTS_TOUCH_STEP;
        }
        count += sprintf(g_sz_debug, "%02X,%02X,%02X", buf[0], buf[1], buf[2]);
        for (i = 0; i < len; i++) {
            count += sprintf(g_sz_debug + count, ",%02X", buf[i + 3]);
        }
        FTS_DEBUG("buffer: %s", g_sz_debug);
    }
#endif

    for (i = 0; i < tpd_dts_data.touch_max_num; i++) {
        pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
        if (pointid >= FTS_MAX_ID)
            break;
        else
            data->touch_point++;
        data->au16_x[i] =
            (s16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
        data->au16_y[i] =
            (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
        data->au8_touch_event[i] =
            buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
        data->au8_finger_id[i] =
            (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;

        data->pressure[i] = (buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);   //cannot constant value
        data->area[i] = (buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
        if ((data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2)
            && (data->touch_point_num == 0))
            break;

    }
    FTS_FUNC_EXIT();
    return 0;
}

/************************************************************************
* Name: fts_report_key
* Brief: report key event
* Input: event info
* Output: no
* Return: 0: is key event, -1: isn't key event
***********************************************************************/
static int fts_report_key(struct ts_event *data)
{
    int i = 0;

    if (1 != data->touch_point)
        return -1;

    for (i = 0; i < FTS_MAX_POINTS; i++) {{
        if (data->au16_y[i] <= TPD_RES_Y) {
            return -1;
        }
    }
    if (data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) {
        tpd_button(data->au16_x[0], data->au16_y[0], 1);
        FTS_DEBUG("[B]Key(%d, %d) DOWN!", data->au16_x[0], data->au16_y[0]);
    }
    else {
        tpd_button(data->au16_x[0], data->au16_y[0], 0);
        FTS_DEBUG("[B]Key(%d, %d) UP!", data->au16_x[0], data->au16_y[0]);
    }
	}//add by hxl
    input_sync(tpd->dev);

    return 0;
}

/************************************************************************
* Name: fts_report_value
* Brief: report the point information
* Input: event info
* Output: no
* Return: success is zero
***********************************************************************/
static int fts_report_value(struct ts_event *data)
{
    int i = 0;
    int up_point = 0;
    int touchs = 0;
    int pressure = 0;

    for (i = 0; i < data->touch_point; i++) {
        input_mt_slot(tpd->dev, data->au8_finger_id[i]);

        if (data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) {
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, true);
            input_report_key(tpd->dev, BTN_TOUCH, 1);
            if (FTS_REPORT_PRESSURE_EN) {
                if (FTS_FORCE_TOUCH_EN) {
                    if (data->pressure[i] > 0) {
                        pressure = data->pressure[i];
                    }
                    else {
                        FTS_ERROR("[B]Illegal pressure: %d", data->pressure[i]);
                        pressure = 1;
                    }
                }
                else {
                    pressure = 0x3f;
                }
                input_report_abs(tpd->dev, ABS_MT_PRESSURE, pressure);
            }
            if (data->area[i] > 0) {
                input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, data->area[i]);  //0x05
            }
            else {
                FTS_ERROR("[B]Illegal touch-major: %d", data->area[i]);
                input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);  //0x05
            }
            input_report_abs(tpd->dev, ABS_MT_POSITION_X, data->au16_x[i]);
            input_report_abs(tpd->dev, ABS_MT_POSITION_Y, data->au16_y[i]);
            touchs |= BIT(data->au8_finger_id[i]);
            data->touchs |= BIT(data->au8_finger_id[i]);

            if (FTS_REPORT_PRESSURE_EN) {
                FTS_DEBUG("[B]P%d(%4d,%4d)[p:%d,tm:%d] DOWN!",
                          data->au8_finger_id[i], data->au16_x[i],
                          data->au16_y[i], pressure, data->area[i]);
            }
            else {
                FTS_DEBUG("[B]P%d(%4d,%4d)[tm:%d] DOWN!",
                          data->au8_finger_id[i], data->au16_x[i],
                          data->au16_y[i], data->area[i]);
            }
        }
        else {
            up_point++;
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
            if (FTS_REPORT_PRESSURE_EN) {
                input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
            }
            data->touchs &= ~BIT(data->au8_finger_id[i]);
            FTS_DEBUG("[B]P%d UP!", data->au8_finger_id[i]);
        }

    }
    for (i = 0; i < tpd_dts_data.touch_max_num; i++) {
        if (BIT(i) & (data->touchs ^ touchs)) {
            FTS_DEBUG("[B]P%d UP!", i);
            data->touchs &= ~BIT(i);
            input_mt_slot(tpd->dev, i);
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
            if (FTS_REPORT_PRESSURE_EN) {
                input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
            }
        }
    }
    data->touchs = touchs;

    if ((data->touch_point == up_point) || !data->touch_point_num) {
        FTS_DEBUG("[B]Points All UP!");
        input_report_key(tpd->dev, BTN_TOUCH, 0);
    }
    else {
        input_report_key(tpd->dev, BTN_TOUCH, 1);
    }

    input_sync(tpd->dev);
    return 0;
}
#endif

/*****************************************************************************
*  Name: tpd_eint_interrupt_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
    return IRQ_HANDLED;
}

/*****************************************************************************
*  Name: tpd_irq_registration
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_irq_registration(void)
{
    struct device_node *node = NULL;
    int ret = 0;
    u32 ints[2] = { 0, 0 };

    FTS_FUNC_ENTER();
    node = of_find_matching_node(node, touch_of_match);
    if (node) {
        of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
        gpio_set_debounce(ints[0], ints[1]);

        ft_touch_irq = irq_of_parse_and_map(node, 0);
        ret = request_irq(ft_touch_irq, tpd_eint_interrupt_handler,
                          IRQF_TRIGGER_FALLING, "TOUCH_PANEL-eint", NULL);
        if (ret > 0)
            FTS_ERROR("tpd request_irq IRQ LINE NOT AVAILABLE!.");
        else
            FTS_INFO("IRQ request succussfully, irq=%d", ft_touch_irq);
    }
    else {
        FTS_ERROR("Can not find touch eint device node!");
    }
    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: touch_event_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int touch_event_handler(void *unused)
{
    int i = 0;
    int ret;

#if FTS_GESTURE_EN
    u8 state = 0;
#endif
#if FTS_MT_PROTOCOL_B_EN
    struct ts_event pevent;
#else
    struct touch_info cinfo, pinfo;
#endif

    struct touch_info finfo;
    //struct sched_param param = {.sched_priority = RTPM_PRIO_TPD };
	printk("touch event handler begain\n");
    if (tpd_dts_data.use_tpd_button) {
        memset(&finfo, 0, sizeof(struct touch_info));
        for (i = 0; i < FTS_MAX_POINTS; i++)
            finfo.p[i] = 1;
    }
#if !FTS_MT_PROTOCOL_B_EN
    memset(&cinfo, 0, sizeof(struct touch_info));
    memset(&pinfo, 0, sizeof(struct touch_info));
#endif
    //sched_setscheduler(current, SCHED_RR, &param);

    do {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag != 0);

        tpd_flag = 0;

        set_current_state(TASK_RUNNING);

#if FTS_GESTURE_EN
        ret = fts_i2c_read_reg(fts_i2c_client, FTS_REG_GESTURE_EN, &state);
        if (ret < 0) {
            FTS_ERROR("[Focal][Touch] read value fail");
        }
        if (state == 1) {
            fts_gesture_readdata(fts_i2c_client);
            continue;
        }
#endif

#if FTS_PSENSOR_EN
        if (fts_proximity_readdata(fts_i2c_client) == 0)
            continue;
#endif

        FTS_DEBUG("touch_event_handler start");
#if FTS_ESDCHECK_EN
        fts_esdcheck_set_intr(1);
#endif
#if FTS_MT_PROTOCOL_B_EN
        {
            ret = fts_read_touchdata(&pevent);
            if (ret == 0) {
                if (tpd_dts_data.use_tpd_button) {
                    ret = !fts_report_key(&pevent);
                }
                if (ret == 0) {
                    fts_report_value(&pevent);
                }
            }
        }
#else //FTS_MT_PROTOCOL_A_EN
        {
            ret = tpd_touchinfo(&cinfo, &pinfo);
            if (ret) {
                if (tpd_dts_data.use_tpd_button) {
                    if (cinfo.p[0] == 0)
                        memcpy(&finfo, &cinfo, sizeof(struct touch_info));
                }

                if ((cinfo.y[0] >= TPD_RES_Y) && (pinfo.y[0] < TPD_RES_Y)
                    && ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
                    FTS_DEBUG("All up");
                    tpd_up(pinfo.x[0], pinfo.y[0]);
                    input_sync(tpd->dev);
                    continue;
                }

                if (tpd_dts_data.use_tpd_button) {
                    if ((cinfo.y[0] <= TPD_RES_Y && cinfo.y[0] != 0)
                        && (pinfo.y[0] > TPD_RES_Y)
                        && ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
                        FTS_DEBUG("All key up");
                        tpd_button(pinfo.x[0], pinfo.y[0], 0);
                        input_sync(tpd->dev);
                        continue;
                    }

                    if ((cinfo.y[0] > TPD_RES_Y) || (pinfo.y[0] > TPD_RES_Y)) {
                        if (finfo.y[0] > TPD_RES_Y) {
                            if ((cinfo.p[0] == 0) || (cinfo.p[0] == 2)) {
                                FTS_DEBUG("Key(%d,%d) Down", pinfo.x[0],
                                          pinfo.y[0]);
                                tpd_button(pinfo.x[0], pinfo.y[0], 1);
                            }
                            else if ((cinfo.p[0] == 1) &&
                                     ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
                                FTS_DEBUG("Key(%d,%d) Up!", pinfo.x[0],
                                          pinfo.y[0]);
                                tpd_button(pinfo.x[0], pinfo.y[0], 0);
                            }
                            input_sync(tpd->dev);
                        }
                        continue;
                    }
                }
                if (cinfo.count > 0) {
                    for (i = 0; i < cinfo.count; i++)
                        tpd_down(cinfo.x[i], cinfo.y[i], i + 1, cinfo.id[i]);
                }
                else {
                    tpd_up(cinfo.x[0], cinfo.y[0]);
                }
                input_sync(tpd->dev);

            }
        }
#endif
#if FTS_ESDCHECK_EN
        fts_esdcheck_set_intr(0);
#endif
    }
    while (!kthread_should_stop());

    return 0;
}

/************************************************************************
* Name: tpd_i2c_detect
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static int tpd_i2c_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);

    return 0;
}

//add by hxl /proc/tp_info
#ifdef MTK_CTP_NODE 
#define CTP_PROC_FILE "tp_info"

static int ctp_proc_read_show (struct seq_file* m, void* data)
{
	char temp[40] = {0};
	sprintf(temp, "[Vendor]wistron,[Fw]0x%02x,[IC]FT5446\n",ctp_fw_version); 
	seq_printf(m, "%s\n", temp);
	return 0;
}

static int ctp_proc_open (struct inode* inode, struct file* file) 
{
    return single_open(file, ctp_proc_read_show, inode->i_private);
}

static const struct file_operations g_ctp_proc = 
{
    .open = ctp_proc_open,
    .read = seq_read,
};
#endif

/************************************************************************
* Name: fts_probe
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int retval = 0;

    FTS_FUNC_ENTER();

    fts_i2c_client = client;
    fts_input_dev = tpd->dev;
	printk("ft5446 probe\n");
    if (fts_i2c_client->addr != FTS_I2C_SLAVE_ADDR) {
        printk("[TPD]Change i2c addr 0x%02x to 0x38", fts_i2c_client->addr);
        //fts_i2c_client->addr = FTS_I2C_SLAVE_ADDR;
        printk("[TPD]fts_i2c_client->addr=0x%x\n", fts_i2c_client->addr);
    }

    /* Init I2C */
    fts_i2c_init();
    fts_reset_proc(300);
    retval = fts_wait_tp_to_valid(client);
	if(retval)
		goto err_read_device_id;

    /* Configure gpio to irq and request irq */
	tpd_gpio_as_int(tpd_int_gpio_number);
    tpd_irq_registration();

#if FTS_GESTURE_EN
    fts_gesture_init(tpd->dev, client);
#endif

#if FTS_PSENSOR_EN
    fts_proximity_init(client);
#endif

#if FTS_MT_PROTOCOL_B_EN
    input_mt_init_slots(tpd->dev, FTS_MAX_POINTS, INPUT_MT_DIRECT);
#endif

    fts_ctpm_get_upgrade_array();

#if FTS_SYSFS_NODE_EN
    fts_create_sysfs(client);
#endif

    fts_ex_mode_init(client);

#if FTS_APK_NODE_EN
    fts_create_apk_debug_channel(client);
#endif

    thread_tpd = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if (IS_ERR(thread_tpd)) {
        retval = PTR_ERR(thread_tpd);
        FTS_ERROR("[TPD]Failed to create kernel thread_tpd,ret=%d!", retval);
    }

    printk("[TPD]Touch Panel Device Probe %s!",
              (retval < 0) ? "FAIL" : "PASS");

#ifdef MTK_CTP_NODE
	if((proc_create(CTP_PROC_FILE,0444,NULL,&g_ctp_proc)) == NULL){
		printk("proc_create tp version node error\n");
	}
#endif

//add by hxl dev_info
	fts_i2c_read_reg(client, 0xA6, &ver);
	ctp_fw_version = ver;
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
   	ver_id = ctp_fw_version;
	ver_module = ver;
	temp_ver=(char*) kmalloc(8, GFP_KERNEL);	
	sprintf(temp_ver,"0x%x",ver_id);
	devinfo_ctp_regchar("unknown","wistron",temp_ver,DEVINFO_USED);
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_init();
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_init();
#endif

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_init();
#endif

#if FTS_TEST_EN
    fts_test_init(client);
#endif

	tpd_load_status = 1;
    FTS_FUNC_EXIT();
	
    return 0;

err_read_device_id:
	FTS_ERROR("[TPD][ft5446_probe]Failed to read Device ID,retval=%d!",retval);
	fts_i2c_exit();
	tpd_load_status = 0;
	return -1;
}

/*****************************************************************************
*  Name: tpd_remove
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_remove(struct i2c_client *client)
{
    FTS_FUNC_ENTER();

#if FTS_TEST_EN
    fts_test_exit(client);
#endif

#if FTS_SYSFS_NODE_EN
    fts_remove_sysfs(client);
#endif

    fts_ex_mode_exit(client);

#if FTS_PSENSOR_EN
    fts_proximity_exit(client);
#endif

#if FTS_APK_NODE_EN
    fts_release_apk_debug_channel();
#endif

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_exit();
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_exit();
#endif

#if FTS_GESTURE_EN
    fts_gesture_exit(client);
#endif

    fts_i2c_exit();

    FTS_FUNC_EXIT();

    return 0;
}

/*****************************************************************************
*  Name: tpd_local_init
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_local_init(void)
{
    FTS_FUNC_ENTER();
#if FTS_POWER_SOURCE_CUST_EN
    if (fts_power_init() != 0)
        return -1;
#endif

    if (i2c_add_driver(&tpd_i2c_driver) != 0) {
        FTS_ERROR("[TPD]: Unable to add fts i2c driver!!");
        FTS_FUNC_EXIT();
        return -1;
    }

    if (tpd_dts_data.use_tpd_button) {
        tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
                           tpd_dts_data.tpd_key_dim_local);
    }

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local_factory, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_factory, 8 * 4);

    memcpy(tpd_calmat, tpd_def_calmat_local_normal, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_normal, 8 * 4);
#endif

    tpd_type_cap = 1;

    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: tpd_suspend
*  Brief: When suspend, will call this function
*           1. Set gesture if EN
*           2. Disable ESD if EN
*           3. Process MTK sensor hub if configure, default n, if n, execute 4/5/6
*           4. disable irq
*           5. Set TP to sleep mode
*           6. Disable power(regulator) if EN
*           7. fts_release_all_finger
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void tpd_suspend(struct device *h)
{
    int retval = 0;

    FTS_FUNC_ENTER();

#if FTS_PSENSOR_EN
    if (fts_proximity_suspend() == 0)
        return;
#endif

    fts_release_all_finger();

#if FTS_GESTURE_EN
    retval = fts_gesture_suspend(fts_i2c_client);
    if (retval == 0) {
        /* Enter into gesture mode(suspend) */
        return;
    }
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_suspend();
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_suspend(fst_i2c_client);
#else
    disable_irq(ft_touch_irq);
    /* TP enter sleep mode */
    retval =
        fts_i2c_write_reg(fts_i2c_client, FTS_REG_POWER_MODE,
                          FTS_REG_POWER_MODE_SLEEP_VALUE);
    if (retval < 0) {
        FTS_ERROR("Set TP to sleep mode fail, ret=%d!", retval);
    }

#if FTS_POWER_SOURCE_CUST_EN
    fts_power_suspend();
#endif

#endif

    FTS_FUNC_EXIT();
}

/*****************************************************************************
*  Name: tpd_resume
*  Brief: When suspend, will call this function
*           1. Clear gesture if EN
*           2. Enable power(regulator) if EN
*           3. Execute reset if no IDC to wake up
*           4. Confirm TP in valid app by read chip ip register:0xA3
*           5. fts_release_all_finger
*           6. Enable ESD if EN
*           7. tpd_usb_plugin if EN
*           8. Process MTK sensor hub if configure, default n, if n, execute 9
*           9. disable irq
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void tpd_resume(struct device *h)
{
	int retval;

    FTS_FUNC_ENTER();
#if FTS_PSENSOR_EN
    if (fts_proximity_resume() == 0)
        return;
#endif

#if FTS_GESTURE_EN
    if (fts_gesture_resume(fts_i2c_client) == 0) {
        FTS_FUNC_EXIT();
        return;
    }
#endif

#if FTS_POWER_SOURCE_CUST_EN
    //fts_power_resume();
#endif

#if (!FTS_CHIP_IDC)
//    fts_reset_proc(300);
    tpd_gpio_output(tpd_rst_gpio_number, 0);
    msleep(2);
	retval = regulator_enable(tpd->reg);
    if (retval != 0) {
        FTS_ERROR("[POWER]Fail to enable regulator when init,ret=%d!", retval);
        return;
    }
    msleep(5);
    tpd_gpio_output(tpd_rst_gpio_number, 1);
    msleep(300);

#endif

    fts_tp_state_recovery(fts_i2c_client);

#if FTS_ESDCHECK_EN
    fts_esdcheck_resume();
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_enable(fts_i2c_client);
#else
    enable_irq(ft_touch_irq);
#endif

}

/*****************************************************************************
*  TPD Device Driver
*****************************************************************************/
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static DEVICE_ATTR(tpd_scp_ctrl, 0664, show_scp_ctrl, store_scp_ctrl);
#endif

struct device_attribute *fts_attrs[] = {
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    &dev_attr_tpd_scp_ctrl,
#endif
};

static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = FTS_DRIVER_NAME,
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
    .attrs = {
              .attr = fts_attrs,
              .num = ARRAY_SIZE(fts_attrs),
              },
};

/*****************************************************************************
*  Name: tpd_driver_init
*  Brief: 1. Get dts information
*         2. call tpd_driver_add to add tpd_device_driver
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int __init tpd_driver_init(void)
{
    FTS_FUNC_ENTER();
    FTS_INFO("Driver version: %s", FTS_DRIVER_VERSION);
    tpd_get_dts_info();
    if (tpd_driver_add(&tpd_device_driver) < 0) {
        FTS_ERROR("[TPD]: Add FTS Touch driver failed!!");
    }

    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: tpd_driver_exit
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void __exit tpd_driver_exit(void)
{
    FTS_FUNC_ENTER();
    tpd_driver_remove(&tpd_device_driver);
    FTS_FUNC_EXIT();
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);

MODULE_AUTHOR("FocalTech Driver Team");
MODULE_DESCRIPTION("FocalTech Touchscreen Driver for Mediatek");
MODULE_LICENSE("GPL v2");
