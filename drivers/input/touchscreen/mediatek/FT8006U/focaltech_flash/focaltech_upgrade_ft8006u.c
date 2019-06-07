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
* File Name: focaltech_upgrade_ft8006u.c
*
* Author: Focaltech Driver Team
*
* Created: 2017-07-22
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
u8 pb_file_ft8006u[] = {
#include "../include/pramboot/FT8006U_Pramboot_V0.3_20170915.i"
};

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
/************************************************************************
 * fts_ft8006u_upgrade_mode -
 * Return: return 0 if success, otherwise return error code
 ***********************************************************************/
static int fts_ft8006u_upgrade_mode(
    struct i2c_client *client,
    enum FW_FLASH_MODE mode,
    u8 *buf,
    u32 len)
{
    int ret = 0;
    u32 start_addr = 0;
    u8 cmd[4] = { 0 };
    u32 delay = 0;
    int ecc_in_host = 0;
    int ecc_in_tp = 0;

    if ((NULL == buf) || (len < FTS_MIN_LEN)) {
        FTS_ERROR("buffer/len(%x) is invalid", len);
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
    start_addr = upgrade_func_ft8006u.appoff;
    if (FLASH_MODE_LIC == mode) {
        /* lcd initial code upgrade */
        /* not finish 3-gamma yet   */
        cmd[1] = FLASH_MODE_LIC_VALUE;
        start_addr = upgrade_func_ft8006u.licoff;
    } else if (FLASH_MODE_PARAM == mode) {
        cmd[1] = FLASH_MODE_PARAM_VALUE;
        start_addr = upgrade_func_ft8006u.paramcfgoff;
    }
    FTS_INFO("flash mode:0x%02x, start addr=0x%04x", cmd[1], start_addr);

    ret = fts_i2c_write(client, cmd, 2);
    if (ret < 0) {
        FTS_ERROR("upgrade mode(09) cmd write fail");
        goto fw_reset;
    }

    delay = FTS_ERASE_SECTOR_DELAY * (len / FTS_MAX_LEN_SECTOR);
    ret = fts_fwupg_erase(client, delay);
    if (ret < 0) {
        FTS_ERROR("erase cmd write fail");
        goto fw_reset;
    }

    /* write app */
    ecc_in_host = fts_flash_write_buf(client, start_addr, buf, len, 1);
    if (ecc_in_host < 0 ) {
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

/*
 * fts_ft8006u_get_hlic_ver - read host lcd init code version
 *
 * return 0 if host lcd init code is valid, otherwise return error code
 */
static int fts_ft8006u_get_hlic_ver(u8 *initcode)
{
    u8 *hlic_buf = initcode;
    u16 hlic_len = 0;
    u8 hlic_ver[2] = { 0 };

    hlic_len = (u16)(((u16)hlic_buf[3]) << 8) + hlic_buf[2];
    FTS_INFO("host lcd init code len:%x", hlic_len);
    if (hlic_len >= FTS_MAX_LEN_SECTOR) {
        FTS_ERROR("host lcd init code len(%x) is too large", hlic_len);
        return -EINVAL;
    }

    hlic_ver[0] = hlic_buf[hlic_len];
    hlic_ver[1] = hlic_buf[hlic_len + 1];

    FTS_INFO("host lcd init code ver:%x %x", hlic_ver[0], hlic_ver[1]);
    if (0xFF != (hlic_ver[0] + hlic_ver[1])) {
        FTS_ERROR("host lcd init code version check fail");
        return -EINVAL;
    }

    return hlic_ver[0];
}

/************************************************************************
* Name: fts_ft8006u_upgrade
* Brief:
* Input:
* Output:
* Return: return 0 if success, otherwise return error code
***********************************************************************/
static int fts_ft8006u_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;
    u8 *tmpbuf = NULL;
    u32 app_len = 0;

    FTS_INFO("fw app upgrade...");
    if (NULL == buf) {
        FTS_ERROR("fw buf is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_FILE)) {
        FTS_ERROR("fw buffer len(%x) fail", len);
        return -EINVAL;
    }

    app_len = len - upgrade_func_ft8006u.appoff;
    tmpbuf = buf + upgrade_func_ft8006u.appoff;
    ret = fts_ft8006u_upgrade_mode(client, FLASH_MODE_APP, tmpbuf, app_len);
    if (ret < 0) {
        FTS_INFO("fw upgrade fail,reset to normal boot");
        if (fts_fwupg_reset_in_boot(client) < 0) {
            FTS_ERROR("reset to normal boot fail");
        }
        return ret;
    }

    return 0;
}

/************************************************************************
* Name: fts_ft8006u_lic_upgrade
* Brief:
* Input:
* Output:
* Return: return 0 if success, otherwise return error code
***********************************************************************/
static int fts_ft8006u_lic_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;
    u8 *tmpbuf = NULL;
    u32 lic_len = 0;

    FTS_INFO("lcd initial code upgrade...");
    if (NULL == buf) {
        FTS_ERROR("lcd initial code buffer is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_FILE)) {
        FTS_ERROR("lcd initial code buffer len(%x) fail", len);
        return -EINVAL;
    }

    /* remalloc memory for initcode, need change content of initcode afterwise */
    lic_len = FTS_MAX_LEN_SECTOR;
    tmpbuf = kmalloc(lic_len, GFP_KERNEL);
    if (NULL == tmpbuf) {
        FTS_INFO("initial code buf malloc fail");
        return -EINVAL;
    }
    memcpy(tmpbuf, buf, lic_len);

    ret = fts_ft8006u_upgrade_mode(client, FLASH_MODE_LIC, tmpbuf, lic_len);
    if (ret < 0) {
        FTS_INFO("lcd initial code upgrade fail,reset to normal boot");
        if (fts_fwupg_reset_in_boot(client) < 0) {
            FTS_ERROR("reset to normal boot fail");
        }
        if (tmpbuf) {
            kfree(tmpbuf);
            tmpbuf = NULL;
        }
        return ret;
    }

    if (tmpbuf) {
        kfree(tmpbuf);
        tmpbuf = NULL;
    }
    return 0;
}

/************************************************************************
 * Name: fts_ft8006_param_upgrade
 * Brief:
 * Input: buf - all.bin
 *        len - len of all.bin
 * Output:
 * Return: return 0 if success, otherwise return error code
 ***********************************************************************/
static int fts_ft8006u_param_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;
    u8 *tmpbuf = NULL;
    u32 param_length = 0;

    FTS_INFO("parameter configure upgrade...");
    if (NULL == buf) {
        FTS_ERROR("fw file buffer is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_FILE)) {
        FTS_ERROR("fw file buffer len(%x) fail", len);
        return -EINVAL;
    }

    tmpbuf = buf + upgrade_func_ft8006u.paramcfgoff;
    param_length = len - upgrade_func_ft8006u.paramcfgoff;
    ret = fts_ft8006u_upgrade_mode(client, FLASH_MODE_PARAM, tmpbuf, param_length);
    if (ret < 0) {
        FTS_INFO("fw upgrade fail,reset to normal boot");
        if (fts_fwupg_reset_in_boot(client) < 0) {
            FTS_ERROR("reset to normal boot fail");
        }
        return ret;
    }

    return 0;
}

struct upgrade_func upgrade_func_ft8006u = {
    .ctype = {0x0B},
    .fwveroff = 0x210E,
    .fwcfgoff = 0x1F80,
    .appoff = 0x2000,
    .licoff = 0x0000,
    .paramcfgoff = 0x12000,
    .paramcfgveroff = 0x12005,
    .pramboot_supported = true,
    .pramboot = pb_file_ft8006u,
    .pb_length = sizeof(pb_file_ft8006u),
    .hid_supported = false,
    .upgrade = fts_ft8006u_upgrade,
    .get_hlic_ver = fts_ft8006u_get_hlic_ver,
    .lic_upgrade = fts_ft8006u_lic_upgrade,
    .param_upgrade = fts_ft8006u_param_upgrade,
};
