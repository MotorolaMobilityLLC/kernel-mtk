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
* File Name: focaltech_sensor.c
*
*    Author: Focaltech Driver Team
*
*   Created: 2016-08-06
*
*  Abstract:
*
*   Version: v1.0
*
*****************************************************************************/

#include "focaltech_core.h"
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif

#define TPD_I2C_NUMBER                          0

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT

enum TOUCH_IPI_CMD_T {
    /* SCP->AP */
    IPI_COMMAND_SA_GESTURE_TYPE,
    /* AP->SCP */
    IPI_COMMAND_AS_CUST_PARAMETER,
    IPI_COMMAND_AS_ENTER_DOZEMODE,
    IPI_COMMAND_AS_ENABLE_GESTURE,
    IPI_COMMAND_AS_GESTURE_SWITCH,
};

struct Touch_Cust_Setting {
    u32 i2c_num;
    u32 int_num;
    u32 io_int;
    u32 io_rst;
};

struct Touch_IPI_Packet {
    u32 cmd;
    union {
        u32 data;
        struct Touch_Cust_Setting tcs;
    } param;
};

static bool tpd_scp_doze_en = TRUE;

static void tpd_scp_wakeup_enable(bool en)
{
    tpd_scp_doze_en = en;
}

ssize_t show_scp_ctrl(struct device *dev, struct device_attribute *attr,
                      char *buf)
{
    return 0;
}

ssize_t store_scp_ctrl(struct device * dev, struct device_attribute * attr,
                       const char *buf, size_t size)
{
    u32 cmd;
    Touch_IPI_Packet ipi_pkt;

    if (kstrtoul(buf, 10, &cmd)) {
        FTS_DEBUG("[SCP_CTRL]: Invalid values\n");
        return -EINVAL;
    }

    FTS_DEBUG("SCP_CTRL: Command=%d", cmd);
    switch (cmd) {
    case 1:
        /* make touch in doze mode */
        tpd_scp_wakeup_enable(TRUE);
        tpd_suspend(NULL);
        break;
    case 2:
        tpd_resume(NULL);
        break;
        /*case 3:
           // emulate in-pocket on
           ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
           ipi_pkt.param.data = 1;
           md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
           break;
           case 4:
           // emulate in-pocket off
           ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
           ipi_pkt.param.data = 0;
           md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
           break; */
    case 5:
        {
            Touch_IPI_Packet ipi_pkt;

            ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
            ipi_pkt.param.tcs.i2c_num = TPD_I2C_NUMBER;
            ipi_pkt.param.tcs.int_num = CUST_EINT_TOUCH_PANEL_NUM;
            ipi_pkt.param.tcs.io_int = TPD_RS;
            ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;
            if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0) < 0)
                FTS_DEBUG("[TOUCH] IPI cmd failed (%d)\n", ipi_pkt.cmd);

            break;
        }
    default:
        FTS_DEBUG("[SCP_CTRL] Unknown command");
        break;
    }

    return size;
}

int fts_sensor_init(void)
{
    int ret;

    ret = get_md32_semaphore(SEMAPHORE_TOUCH);
    if (ret < 0)
        pr_err("[TOUCH] HW semaphore reqiure timeout\n");

    return 0;
}

static s8 fts_sensor_enter_doze(struct i2c_client *client)
{
    s8 ret = -1;
    s8 retry = 0;
    char gestrue_on = 0x01;
    char gestrue_data;
    int i;

    FTS_DEBUG("Entering doze mode...");

    /* Enter gestrue recognition mode */
    ret = fts_i2c_write_reg(i2c_client, FTS_REG_GESTURE_EN, gestrue_on);
    if (ret < 0) {
        FTS_ERROR("Failed to enter Doze %d", retry);
        return ret;
    }
    msleep(30);

    for (i = 0; i < 10; i++) {
        fts_i2c_read_reg(i2c_client, FTS_REG_GESTURE_EN, &gestrue_data);
        if (gestrue_data == 0x01) {
            FTS_DEBUG("FTP has been working in doze mode!");
            break;
        }
        msleep(20);
        fts_i2c_write_reg(i2c_client, FTS_REG_GESTURE_EN, gestrue_on);

    }

    return ret;
}

void fts_sensor_enable(struct i2c_client *client)
{
    int ret;
    int data;

    if (tpd_scp_doze_en) {
        ret = get_md32_semaphore(SEMAPHORE_TOUCH);
        if (ret < 0) {
            FTS_DEBUG("[TOUCH] HW semaphore reqiure timeout\n");
        }
        else {
            Touch_IPI_Packet ipi_pkt = {
                .cmd = IPI_COMMAND_AS_ENABLE_GESTURE,
                .param.data = 0
            };

            md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
        }
    }

    data = 0x00;
    fts_i2c_write_reg(client, FTS_REG_GESTURE_EN, data);
}

void fts_sensor_suspend(struct i2c_client *i2c_client)
{
    int sem_ret;

    int ret;
    char gestrue_data;
    static int scp_init_flag;

    sem_ret = release_md32_semaphore(SEMAPHORE_TOUCH);

    if (scp_init_flag == 0) {
        Touch_IPI_Packet ipi_pkt;

        ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
        ipi_pkt.param.tcs.i2c_num = TPD_I2C_NUMBER;
        ipi_pkt.param.tcs.int_num = CUST_EINT_TOUCH_PANEL_NUM;
        ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
        ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;

        FTS_DEBUG("[TOUCH]SEND CUST command :%d ",
                  IPI_COMMAND_AS_CUST_PARAMETER);

        ret = md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
        if (ret < 0)
            FTS_DEBUG(" IPI cmd failed (%d)\n", ipi_pkt.cmd);

        msleep(20);             /* delay added between continuous command */
        /* Workaround if suffer MD32 reset */
        /* scp_init_flag = 1; */
    }

    if (tpd_scp_doze_en) {
        FTS_DEBUG("[TOUCH]SEND ENABLE GES command :%d ",
                  IPI_COMMAND_AS_ENABLE_GESTURE);
        ret = fts_sensor_enter_doze(i2c_client);
        if (ret < 0) {
            FTS_DEBUG("FTP Enter Doze mode failed\n");
        }
        else {
            int retry = 5;
            {
                /* check doze mode */
                fts_i2c_read_reg(i2c_client, FTS_REG_GESTURE_EN, &gestrue_data);
                FTS_DEBUG("========================>0x%x", gestrue_data);
            }

            msleep(20);
            Touch_IPI_Packet ipi_pkt = {.cmd =
                    IPI_COMMAND_AS_ENABLE_GESTURE,.param.data = 1 };

            do {
                if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 1) ==
                    DONE)
                    break;
                msleep(20);
                FTS_DEBUG("==>retry=%d", retry);
            }
            while (retry--);

            if (retry <= 0)
                FTS_DEBUG("############# md32_ipi_send failed retry=%d", retry);

        }
    }

}

#endif
