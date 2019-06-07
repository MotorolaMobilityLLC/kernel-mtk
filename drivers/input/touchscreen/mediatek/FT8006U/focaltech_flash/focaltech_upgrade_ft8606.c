/*
 *
 * FocalTech fts TouchScreen driver.
 *
 * Copyright (c) 2010-2017, Focaltech Ltd. All rights reserved.
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
* File Name: focaltech_upgrade_ft8606.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-15
*
* Abstract:
*
* Reference:
*
*****************************************************************************/
/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "../focaltech_core.h"
#include "../focaltech_flash.h"

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
u8 pb_file_ft8606[] = {
#include "../include/pramboot/FT8606_Pramboot_V0.7_20150507.i"
};

/************************************************************************
 * fts_ft8606_upgrade_mode
 * Return: return 0 if success, otherwise return error code
 ***********************************************************************/
static int fts_ft8606_upgrade_mode(
    struct i2c_client *client,
    enum FW_FLASH_MODE mode,
    u8 *buf,
    u32 len
)
{
    int ret = 0;
    u32 start_addr = 0;
    u8 cmd[4] = { 0 };
    u32 delay = 0;
    int ecc_in_host = 0;
    int ecc_in_tp = 0;

    if ((NULL == buf) || (len < FTS_MIN_LEN)) {
        FTS_ERROR("buffer/len is invalid");
        return -EINVAL;
    }

    /* enter into upgrade environment */
    ret = fts_fwupg_enter_into_boot(client);
    if (ret < 0) {
        FTS_ERROR("enter into pramboot/bootloader fail,ret=%d", ret);
        goto fw_reset;
    }

    cmd[0] = FTS_CMD_FLASH_MODE;
    cmd[1] = FLASH_MODE_UPGRADE_VALUE;
    start_addr = upgrade_func_ft8606.appoff;
    FTS_INFO("flash mode:0x%02x, start addr=0x%x", cmd[1], start_addr);
    ret = fts_i2c_write(client, cmd, 2);
    if (ret < 0) {
        FTS_ERROR("upgrade mode(09) cmd write fail");
        goto fw_reset;
    }

    delay = 60 * (len / FTS_MAX_LEN_SECTOR);
    ret = fts_fwupg_erase(client, delay);
    if (ret < 0) {
        FTS_ERROR("erase cmd write fail");
        goto fw_reset;
    }

    /* write app */
    ecc_in_host = fts_flash_write_buf(client, start_addr, buf, len, 1);
    if (ecc_in_host < 0) {
        FTS_ERROR("lcd initial code write fail");
        goto fw_reset;
    }

    /* ecc */
    ecc_in_tp = fts_fwupg_ecc_cal(client, start_addr, len);
    if (ecc_in_tp < 0 ) {
        FTS_ERROR("ecc read fail");
        goto fw_reset;
    }

    FTS_INFO("ecc in tp:%x, host:%x", ecc_in_tp, ecc_in_host);
    if (ecc_in_tp != ecc_in_host) {
        FTS_ERROR("ecc check fail");
        goto fw_reset;
    }

    FTS_INFO("upgrade success, reset to normal boot");
    ret = fts_fwupg_reset_in_boot(client);
    if (ret < 0) {
        FTS_ERROR("reset to normal boot fail");
    }
    msleep(400);
    return 0;

fw_reset:
    return -EIO;
}


/************************************************************************
* Name: fts_ft8606_upgrade
* Brief:
* Input:
* Output:
* Return: return 0 if success, otherwise return error code
***********************************************************************/
static int fts_ft8606_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;

    FTS_FUNC_ENTER();

    if (NULL == buf) {
        FTS_ERROR("fw buf is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_APP)) {
        FTS_ERROR("fw buffer len(%x) fail", len);
        return -EINVAL;
    }

    ret = fts_ft8606_upgrade_mode(client, FLASH_MODE_APP, buf, len);
    if (ret < 0) {
        FTS_INFO("fw upgrade fail,reset to normal boot");
        if (fts_fwupg_reset_in_boot(client) < 0) {
            FTS_ERROR("reset to normal boot fail");
        }
        return ret;
    }

    return 0;
}

struct upgrade_func upgrade_func_ft8606 = {
    .ctype = {0x08},
    .fwveroff = 0x010A,
    .fwcfgoff = 0x0780,
    .appoff = 0x1000,
    .licoff = 0x0000,
    .paramcfgoff = 0x000,
    .pramboot_supported = true,
    .pramboot = pb_file_ft8606,
    .pb_length = sizeof(pb_file_ft8606),
    .hid_supported = false,
    .upgrade = fts_ft8606_upgrade,
    .get_hlic_ver = NULL,
    .lic_upgrade = NULL,
};
