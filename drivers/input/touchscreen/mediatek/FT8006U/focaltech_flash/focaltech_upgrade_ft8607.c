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
* File Name: focaltech_upgrade_ft8607.c
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
u8 pb_file_ft8607[] = {
#include "../include/pramboot/FT8607_Pramboot_V0.6_20170526.i"
};

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define MAX_BANK_DATA       0x80
#define MAX_GAMMA_LEN       0x180
/* calculate lcd init code checksum */
static unsigned short cal_lcdinitcode_checksum(u8 *ptr, int initcode_length)
{
    u32 checksum = 0;
    u8 *p_block_addr;
    u16 block_len;
    u16 param_num;
    int i;
    int index = 2;

    while (index < initcode_length - 6) {
        p_block_addr = ptr + index;
        block_len = ((p_block_addr[0] << 8) + p_block_addr[1]) * 2;
        param_num = block_len - 4;

        checksum += ( param_num * ( param_num - 1 ) / 2 );

        checksum += p_block_addr[3];

        for ( i = 0; i < param_num; ++i ) {
            checksum += p_block_addr[4 + i];
        }

        checksum &= 0xffff;

        index += block_len;
    }

    return checksum;
}

static int print_data(u8 *buf, u32 len)
{
    int i = 0;
    int n = 0;
    u8 *p = NULL;

    p = kmalloc(len * 4, GFP_KERNEL);
    memset(p, 0, len * 4);

    for (i = 0; i < len; i++) {
        n += sprintf(p + n, "%02x ", buf[i]);
    }

    FTS_DEBUG("%s", p);

    kfree(p);
    return 0;
}

static int read_3gamma(struct i2c_client *client, u8 **gamma, u16 *len)
{
    int ret = 0;
    int i = 0;
    int packet_num = 0;
    int packet_len = 0;
    int remainder = 0;
    u8 cmd[4] = { 0 };
    u32 addr = 0x01D000;
    u8 gamma_header[0x20] = { 0 };
    u16 gamma_len = 0;
    u16 gamma_len_n = 0;
    u8 *pgamma = NULL;
    int j = 0;
    u8 gamma_ecc = 0;

    cmd[0] = 0x05;
    cmd[1] = 0x80;
    cmd[2] = 0x01;
    ret = fts_i2c_write(client, cmd, 3);
    if (ret < 0) {
        FTS_ERROR("get flash type and clock fail !");
        return ret;
    }

    cmd[0] = 0x03;
    cmd[1] = (u8)(addr >> 16);
    cmd[2] = (u8)(addr >> 8);
    cmd[3] = (u8)addr;
    ret = fts_i2c_write(client, cmd, 4);
    msleep(10);

    ret = fts_i2c_read(client, NULL, 0, gamma_header, 0x20);
    if (ret < 0) {
        FTS_ERROR("read 3-gamma header fail");
        return ret;
    }

    gamma_len = (u16)((u16)gamma_header[0] << 8) + gamma_header[1];
    gamma_len_n = (u16)((u16)gamma_header[2] << 8) + gamma_header[3];

    if ((gamma_len + gamma_len_n) != 0xFFFF) {
        FTS_INFO("gamma length check fail:%x %x", gamma_len, gamma_len);
        return -EIO;
    }

    if ((gamma_header[4] + gamma_header[5]) != 0xFF) {
        FTS_INFO("gamma ecc check fail:%x %x", gamma_header[4], gamma_header[5]);
        return -EIO;
    }

    if (gamma_len > MAX_GAMMA_LEN) {
        FTS_ERROR("gamma data len(%d) is too long", gamma_len);
        return -EINVAL;
    }

    *gamma = kmalloc(MAX_GAMMA_LEN, GFP_KERNEL);
    if (NULL == *gamma) {
        FTS_ERROR("malloc gamma memory fail");
        return -ENOMEM;
    }
    pgamma = *gamma;

    packet_num = gamma_len / 256;
    packet_len = 256;
    remainder = gamma_len % 256;
    if (remainder) packet_num++;
    FTS_INFO("3-gamma len:%d", gamma_len);
    cmd[0] = 0x03;
    addr += 0x20;
    for (i = 0; i < packet_num; i++) {
        addr += i * 256;
        cmd[1] = (u8)(addr >> 16);
        cmd[2] = (u8)(addr >> 8);
        cmd[3] = (u8)addr;
        if ((i == packet_num - 1) && remainder)
            packet_len = remainder;
        ret = fts_i2c_write(client, cmd, 4);
        msleep(10);

        ret = fts_i2c_read(client, NULL, 0, pgamma + i * 256, packet_len);
        if (ret < 0) {
            FTS_ERROR("read 3-gamma data fail");
            return ret;
        }
    }


    // ecc
    for (j = 0; j < gamma_len; j++) {
        gamma_ecc ^= pgamma[j];
    }
    FTS_INFO("back_3gamma_ecc: 0x%x, 0x%x", gamma_ecc, gamma_header[0x04]);
    if (gamma_ecc != gamma_header[0x04]) {
        FTS_ERROR("back gamma ecc check fail:%x %x", gamma_ecc, gamma_header[0x04]);
        return -EIO;
    }

    *len = gamma_len;

    FTS_DEBUG("read 3-gamma data:");
    print_data(*gamma, gamma_len);

    return 0;
}

static int replace_3gamma(u8 *initcode, u8 *gamma, u16 gamma_len)
{
    u16 gamma_pos = 0;
    int i;
    int gamma_bank[6][6] = {
        { 0x0468, 0x00, 0x20, 0x82, 0xD1, 0x3C },
        { 0x04a8, 0x00, 0x20, 0x82, 0xD2, 0x3C },
        { 0x04e8, 0x00, 0x20, 0x82, 0xD3, 0x3C },
        { 0x0528, 0x00, 0x20, 0x82, 0xD4, 0x3C },
        { 0x0568, 0x00, 0x20, 0x82, 0xD5, 0x3C },
        { 0x05a8, 0x00, 0x20, 0x82, 0xD6, 0x3C },
    };
    FTS_FUNC_ENTER();

    /* bank Gamma */
    for (i = 0; i < 6; i++) {
        if ((initcode[gamma_bank[i][0]] == gamma[gamma_pos])
            && (initcode[gamma_bank[i][0] + 3] == gamma[gamma_pos + 3])) {
            memcpy(initcode + gamma_bank[i][0] + 4, gamma + gamma_pos + 4, gamma_bank[i][5]);
            gamma_pos += gamma_bank[i][5] + 4;
        } else
            goto find_gamma_bank_err;
    }

    FTS_DEBUG("replace 3-gamma data:");
    print_data(initcode + 0x468, gamma_len);

    FTS_FUNC_EXIT();
    return 0;

find_gamma_bank_err:
    FTS_INFO("3-gamma bank(%02x %02x) not find",
             gamma[gamma_pos], gamma[gamma_pos + 1]);
    return -ENODATA;
}

/*
 * read_replace_3gamma - read and replace 3-gamma data
 */
static int read_replace_3gamma(struct i2c_client *client, u8 *buf)
{
    int ret = 0;
    u16 initcode_checksum = 0;
    u8 *gamma = NULL;
    u16 gamma_len = 0;
    u16 hlic_len = 0;

    FTS_FUNC_ENTER();

    ret = read_3gamma(client, &gamma, &gamma_len);
    if (ret < 0) {
        FTS_INFO("no vaid 3-gamma data, not replace");
        if (gamma)
            kfree(gamma);
        return 0;
    }

    ret = replace_3gamma(buf, gamma, gamma_len);
    if (ret < 0) {
        FTS_ERROR("replace 3-gamma fail");
        kfree(gamma);
        return ret;
    }

    hlic_len = ((u16)(((u16)buf[0]) << 8) + buf[1]) * 2;
    initcode_checksum = cal_lcdinitcode_checksum(buf, hlic_len);
    FTS_INFO("lcd init code calc checksum:%04x", initcode_checksum);
    buf[hlic_len - 2] = (u8)(initcode_checksum >> 8);
    buf[hlic_len - 1] = (u8)(initcode_checksum);

    FTS_FUNC_EXIT();

    kfree(gamma);
    return 0;
}

/************************************************************************
* Name: check_initial_code_valid
* Brief:
* Input:
* Output:
* Return: return 0 if valid, otherwise return error code
***********************************************************************/
static int check_initial_code_valid(struct i2c_client *client, u8 *buf)
{
    u16 initcode_checksum = 0;
    u16 buf_checksum = 0;
    u16 hlic_len = 0;

    hlic_len = ((u16)(((u16)buf[0]) << 8) + buf[1]) * 2;
    initcode_checksum = cal_lcdinitcode_checksum(buf, hlic_len);
    buf_checksum = ((u16)((u16)buf[hlic_len - 2] << 8) + buf[hlic_len - 1]);
    FTS_INFO("lcd init code calc checksum:%04x, %04x", initcode_checksum, buf_checksum);
    if (initcode_checksum != buf_checksum) {
        FTS_ERROR("Initial Code checksum fail");
        return -EINVAL;
    }
    return 0;
}

/************************************************************************
 * fts_ft8607_upgrade_mode -
 * Return: return 0 if success, otherwise return error code
 ***********************************************************************/
static int fts_ft8607_upgrade_mode(
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
        FTS_ERROR("buffer/len is invalid");
        return -EINVAL;
    }

    /* enter into upgrade environment */
    ret = fts_fwupg_enter_into_boot(client);
    if (ret < 0) {
        FTS_ERROR("enter into pramboot/bootloader fail,ret=%d", ret);
        goto fw_reset;
    }

    if (FLASH_MODE_LIC == mode) {
        /* lcd initial code upgrade */
        ret = read_replace_3gamma(client, buf);
        if (ret < 0) {
            FTS_ERROR("replace 3-gamma fail, not upgrade lcd init code");
            goto fw_reset;
        }

        cmd[0] = FTS_CMD_FLASH_MODE;
        cmd[1] = 0x80;
        cmd[2] = 0x01;
        start_addr = upgrade_func_ft8607.licoff;
        ret = fts_i2c_write(client, cmd, 3);
    } else {
        cmd[0] = FTS_CMD_FLASH_MODE;
        cmd[1] = FLASH_MODE_UPGRADE_VALUE;
        start_addr = upgrade_func_ft8607.appoff;
        ret = fts_i2c_write(client, cmd, 2);
    }
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
 * fts_get_hlic_ver - read host lcd init code version
 *
 * return 0 if host lcd init code is valid, otherwise return error code
 */
static int fts_ft8607_get_hlic_ver(u8 *initcode)
{
    u8 *hlic_buf = initcode;
    u16 hlic_len = 0;
    u8 hlic_ver[2] = { 0 };

    hlic_len = ( (u16)(((u16)hlic_buf[0]) << 8) + hlic_buf[1]) * 2;
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
* Name: fts_ft8607_upgrade
* Brief:
* Input:
* Output:
* Return: return 0 if success, otherwise return error code
***********************************************************************/
static int fts_ft8607_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;
    u8 *tmpbuf = NULL;
    u32 app_len = 0;
    u32 off = 0;

    FTS_FUNC_ENTER();

    if (NULL == buf) {
        FTS_ERROR("fw buf is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_FILE)) {
        FTS_ERROR("fw buffer len(%x) fail", len);
        return -EINVAL;
    }

    off = upgrade_func_ft8607.appoff + FTS_APPINFO_OFF + FTS_APPINFO_APPLEN_OFF;
    app_len = (((u32)buf[off] << 8) + buf[off + 1]);
    if ((app_len < FTS_MIN_LEN) || (app_len > FTS_MAX_LEN_APP)) {
        FTS_ERROR("app len(%x) fail", len);
        return -EINVAL;
    }

    tmpbuf = buf + upgrade_func_ft8607.appoff;
    ret = fts_ft8607_upgrade_mode(client, FLASH_MODE_APP, tmpbuf, app_len);
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
* Name: fts_ft8607_lic_upgrade
* Brief:
* Input:
* Output:
* Return: return 0 if success, otherwise return error code
***********************************************************************/
static int fts_ft8607_lic_upgrade(struct i2c_client *client, u8 *buf, u32 len)
{
    int ret = 0;
    u8 *tmpbuf = NULL;
    u32 lic_len = 0;

    FTS_FUNC_ENTER();

    if (NULL == buf) {
        FTS_ERROR("lcd initial code buffer is null");
        return -EINVAL;
    }

    if ((len < FTS_MIN_LEN) || (len > FTS_MAX_LEN_FILE)) {
        FTS_ERROR("lcd initial code buffer len(%x) fail", len);
        return -EINVAL;
    }

    ret = check_initial_code_valid(client, buf);
    if (ret < 0) {
        FTS_ERROR("initial code invalid, not upgrade lcd init code");
        return -EINVAL;
    }

    lic_len = FTS_MAX_LEN_SECTOR;
    /* remalloc memory for initcode, need change content of initcode afterwise */
    tmpbuf = kmalloc(lic_len, GFP_KERNEL);
    if (NULL == tmpbuf) {
        FTS_INFO("initial code buf malloc fail");
        return -EINVAL;
    }
    memcpy(tmpbuf, buf, lic_len);

    ret = fts_ft8607_upgrade_mode(client, FLASH_MODE_LIC, tmpbuf, lic_len);
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

struct upgrade_func upgrade_func_ft8607 = {
    .ctype = {0x09},
    .fwveroff = 0x110E,
    .fwcfgoff = 0x0780,
    .appoff = 0x1000,
    .licoff = 0x0000,
    .paramcfgoff = 0x0000,
    .pramboot_supported = true,
    .pramboot = pb_file_ft8607,
    .pb_length = sizeof(pb_file_ft8607),
    .hid_supported = false,
    .upgrade = fts_ft8607_upgrade,
    .get_hlic_ver = fts_ft8607_get_hlic_ver,
    .lic_upgrade = fts_ft8607_lic_upgrade,
};
