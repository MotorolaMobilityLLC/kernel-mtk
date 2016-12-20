/*
 *
 * FocalTech fts TouchScreen driver.
 *
 * Copyright (c) 2010-2016, Focaltech Ltd. All rights reserved.
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
* File Name: focaltech_upgrade_ft5x46.c
*
* Author:    fupeipei
*
* Created:    2016-08-15
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
#include "focaltech_upgrade_common.h"

/*****************************************************************************
* Static variables
*****************************************************************************/
#define APP_FILE_MAX_SIZE       (60 * 1024)
#define APP_FILE_MIN_SIZE       (8)

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
static int fts_ctpm_get_app_i_file_ver(void);
static int fts_ctpm_get_app_bin_file_ver(char *firmware_name);
static int fts_ctpm_fw_upgrade_with_app_i_file(struct i2c_client *client);
static int fts_ctpm_fw_upgrade_with_app_bin_file(struct i2c_client *client,
                                                 char *firmware_name);
static unsigned char fts_ctpm_fw_upgrade_get_vendorid_from_boot(struct
                                                                i2c_client
                                                                *client);

struct fts_Upgrade_Fun fts_updatefun = {
    .chip_idh = 0x54,
    .chip_idl = 0x22,
    .get_app_bin_file_ver = fts_ctpm_get_app_bin_file_ver,
    .get_app_i_file_ver = fts_ctpm_get_app_i_file_ver,
    .upgrade_with_app_i_file = fts_ctpm_fw_upgrade_with_app_i_file,
    .upgrade_with_app_bin_file = fts_ctpm_fw_upgrade_with_app_bin_file,
    .upgrade_with_lcd_cfg_i_file = NULL,
    .upgrade_with_lcd_cfg_bin_file = NULL,
    .get_vendorid_from_boot = fts_ctpm_fw_upgrade_get_vendorid_from_boot,
};

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

/************************************************************************
* Name: fts_ctpm_get_app_bin_file_ver
* Brief:  get .i file version
* Input: no
* Output: no
* Return: fw version
***********************************************************************/
static int fts_ctpm_get_app_bin_file_ver(char *firmware_name)
{
    u8 *pbt_buf = NULL;
    int fwsize = fts_GetFirmwareSize(firmware_name);

    FTS_FUNC_ENTER();

    FTS_INFO("start upgrade with app.bin!!");

    if (fwsize <= 0) {
        FTS_ERROR("[UPGRADE]: Get firmware size failed!!");
        return -EIO;
    }

    if (fwsize < APP_FILE_MIN_SIZE || fwsize > APP_FILE_MAX_SIZE) {
        FTS_ERROR("[UPGRADE]: FW length error, upgrade fail!!");
        return -EIO;
    }

    pbt_buf = (unsigned char *)kmalloc(fwsize + 1, GFP_ATOMIC);
    if (fts_ReadFirmware(firmware_name, pbt_buf)) {
        FTS_ERROR("[UPGRADE]: request_firmware failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    if (fwsize > 2) {
        return pbt_buf[fwsize - 2];
    }

    FTS_FUNC_EXIT();

    return 0x00;
}

/************************************************************************
* Name: fts_ctpm_get_app_i_file_ver
* Brief:  get .i file version
* Input: no
* Output: no
* Return: fw version
***********************************************************************/
static int fts_ctpm_get_app_i_file_ver(void)
{
    u16 ui_sz;
    ui_sz = fts_getsize(FW_SIZE);
    if (ui_sz > 2) {
        return CTPM_FW[ui_sz - 2];
    }

    return 0x00;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_use_buf
* Brief: fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_ctpm_fw_upgrade_use_buf(struct i2c_client *client, u8 * pbt_buf,
                                       u32 dw_lenth)
{
    u8 reg_val[4] = { 0 };
    u32 i = 0;
    u32 packet_number;
    u32 j = 0;
    u32 temp;
    u32 lenght;
    u8 packet_buf[FTS_PACKET_LENGTH + 6];
    u8 auc_i2c_write_buf[10];
    u8 upgrade_ecc;
    int i_ret;

    fts_ctpm_i2c_hid2std(client);

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*write 0xaa to register FTS_RST_CMD_REG1 */
        fts_i2c_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        /*write 0x55 to register FTS_RST_CMD_REG1 */
        fts_i2c_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_55);
        msleep(200);

        /*Enter upgrade mode */
        fts_ctpm_i2c_hid2std(client);

        msleep(10);
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
        i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
        if (i_ret < 0) {
            FTS_ERROR("[UPGRADE]: failed writing  0x55 and 0xaa!!");
            continue;
        }

        /*check run in bootloader or not */
        msleep(1);
        auc_i2c_write_buf[0] = FTS_READ_ID_REG;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
            0x00;
        reg_val[0] = reg_val[1] = 0x00;
        fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

        if (reg_val[0] == chip_types.bootloader_idh
            && reg_val[1] == chip_types.bootloader_idl) {
            FTS_DEBUG
                ("[UPGRADE]: read bootload id ok!! ID1 = 0x%x,ID2 = 0x%x!!",
                 reg_val[0], reg_val[1]);
            break;
        }
        else {
            FTS_ERROR
                ("[UPGRADE]: read bootload id fail!! ID1 = 0x%x,ID2 = 0x%x!!",
                 reg_val[0], reg_val[1]);
            continue;
        }
    }

    if (i >= FTS_UPGRADE_LOOP)
        return -EIO;

    /*Step 4:erase app and panel paramenter area */
    FTS_DEBUG("[UPGRADE]: erase app and panel paramenter area!!");
    auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
    fts_i2c_write(client, auc_i2c_write_buf, 1);
    msleep(1350);
    for (i = 0; i < 15; i++) {
        auc_i2c_write_buf[0] = 0x6a;
        reg_val[0] = reg_val[1] = 0x00;
        fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
        if (0xF0 == reg_val[0] && 0xAA == reg_val[1]) {
            break;
        }
        msleep(50);
    }
    FTS_DEBUG("[UPGRADE]: erase app area reg_val[0] = %x reg_val[1] = %x!!",
              reg_val[0], reg_val[1]);
    auc_i2c_write_buf[0] = 0xB0;
    auc_i2c_write_buf[1] = (u8) ((dw_lenth >> 16) & 0xFF);
    auc_i2c_write_buf[2] = (u8) ((dw_lenth >> 8) & 0xFF);
    auc_i2c_write_buf[3] = (u8) (dw_lenth & 0xFF);
    fts_i2c_write(client, auc_i2c_write_buf, 4);
    /*********Step 5:write firmware(FW) to ctpm flash*********/
    upgrade_ecc = 0;
    FTS_DEBUG("[UPGRADE]: write FW to ctpm flash!!");
    temp = 0;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = FTS_FW_WRITE_CMD;
    packet_buf[1] = 0x00;

    for (j = 0; j < packet_number; j++) {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (lenght >> 8);
        packet_buf[5] = (u8) lenght;
        for (i = 0; i < FTS_PACKET_LENGTH; i++) {
            packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
            upgrade_ecc ^= packet_buf[6 + i];
        }
        fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
        msleep(20);

        for (i = 0; i < 30; i++) {
            auc_i2c_write_buf[0] = 0x6a;
            reg_val[0] = reg_val[1] = 0x00;
            fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

            if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1])) {
                break;
            }
            FTS_DEBUG("[UPGRADE]: reg_val[0] = %x reg_val[1] = %x!!",
                      reg_val[0], reg_val[1]);
            //msleep(1);
            fts_ctpm_upgrade_delay(1000);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (u8) (temp >> 8);
        packet_buf[3] = (u8) temp;
        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (u8) (temp >> 8);
        packet_buf[5] = (u8) temp;
        for (i = 0; i < temp; i++) {
            packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
            upgrade_ecc ^= packet_buf[6 + i];
        }
        fts_i2c_write(client, packet_buf, temp + 6);
        msleep(20);

        for (i = 0; i < 30; i++) {
            auc_i2c_write_buf[0] = 0x6a;
            reg_val[0] = reg_val[1] = 0x00;
            fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

            if ((0x1000 +
                 ((packet_number * FTS_PACKET_LENGTH) /
                  ((dw_lenth) % FTS_PACKET_LENGTH))) ==
                (((reg_val[0]) << 8) | reg_val[1])) {
                break;
            }
            FTS_DEBUG
                ("[UPGRADE]: reg_val[0] = %x reg_val[1] = %x  reg_val[2] = 0x%x!!",
                 reg_val[0], reg_val[1],
                 (((packet_number * FTS_PACKET_LENGTH) /
                   ((dw_lenth) % FTS_PACKET_LENGTH)) + 0x1000));
            //msleep(1);
            fts_ctpm_upgrade_delay(1000);
        }
    }

    msleep(50);

    /*********Step 6: read out checksum***********************/
    /*send the opration head */
    FTS_DEBUG("[UPGRADE]: read out checksum!!");
    auc_i2c_write_buf[0] = 0x64;
    fts_i2c_write(client, auc_i2c_write_buf, 1);
    msleep(300);

    temp = 0;
    auc_i2c_write_buf[0] = 0x65;
    auc_i2c_write_buf[1] = (u8) (temp >> 16);
    auc_i2c_write_buf[2] = (u8) (temp >> 8);
    auc_i2c_write_buf[3] = (u8) (temp);
    temp = dw_lenth;
    auc_i2c_write_buf[4] = (u8) (temp >> 8);
    auc_i2c_write_buf[5] = (u8) (temp);
    i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6);
    msleep(dw_lenth / 256);

    for (i = 0; i < 100; i++) {
        auc_i2c_write_buf[0] = 0x6a;
        reg_val[0] = reg_val[1] = 0x00;
        fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

        if (0xF0 == reg_val[0] && 0x55 == reg_val[1]) {
            FTS_DEBUG("[UPGRADE]: reg_val[0]=%02x reg_val[0]=%02x!!",
                      reg_val[0], reg_val[1]);
            break;
        }
        msleep(1);

    }
    auc_i2c_write_buf[0] = 0x66;
    fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
    if (reg_val[0] != upgrade_ecc) {
        FTS_ERROR("[UPGRADE]: ecc error! FW=%02x upgrade_ecc=%02x!!",
                  reg_val[0], upgrade_ecc);
        return -EIO;
    }
    FTS_DEBUG("[UPGRADE]: checksum %x %x!!", reg_val[0], upgrade_ecc);

    FTS_DEBUG("[UPGRADE]: reset the new FW!!");
    auc_i2c_write_buf[0] = FTS_REG_RESET_FW;
    fts_i2c_write(client, auc_i2c_write_buf, 1);
    msleep(200);

    fts_ctpm_i2c_hid2std(client);

    return 0;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_i_file
* Brief:  upgrade with *.i file
* Input: i2c info
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_ctpm_fw_upgrade_with_app_i_file(struct i2c_client *client)
{
    u8 *pbt_buf = NULL;
    int i_ret = 0;
    int fw_len;

    FTS_FUNC_ENTER();

    fw_len = fts_getsize(FW_SIZE);
    if (fw_len < APP_FILE_MIN_SIZE || fw_len > APP_FILE_MAX_SIZE) {
        pr_err("FW length error\n");
        return -EIO;
    }
    FTS_DEBUG("[UPGRADE]: firmware size ok: fw_len == %x!!", fw_len);
    /*FW upgrade */
    pbt_buf = CTPM_FW;
    i_ret = fts_ctpm_fw_upgrade_use_buf(client, pbt_buf, fw_len);
    if (i_ret != 0) {
        dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
    }
    else {
#if FTS_AUTO_CLB_EN
        fts_ctpm_auto_clb(client);  /*start auto CLB */
#endif
    }
    FTS_FUNC_EXIT();

    return i_ret;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_app_file
* Brief:  upgrade with *.bin file
* Input: i2c info, file name
* Output: no
* Return: success =0
***********************************************************************/
static int fts_ctpm_fw_upgrade_with_app_bin_file(struct i2c_client *client,
                                                 char *firmware_name)
{
    u8 *pbt_buf = NULL;
    int i_ret = 0;
    bool ecc_ok = false;
    int fwsize = fts_GetFirmwareSize(firmware_name);

    FTS_INFO("[UPGRADE]: start upgrade with app.bin!!");

    if (fwsize <= 0) {
        FTS_ERROR("[UPGRADE]: Get firmware size failed!!");
        return -EIO;
    }
    if (fwsize < APP_FILE_MIN_SIZE || fwsize > APP_FILE_MAX_SIZE) {
        FTS_ERROR("[UPGRADE]: FW length error!!");
        return -EIO;
    }

    pbt_buf = (unsigned char *)kmalloc(fwsize + 1, GFP_ATOMIC);
    if (fts_ReadFirmware(firmware_name, pbt_buf)) {
        FTS_ERROR("[UPGRADE]: request_firmware failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    if ((pbt_buf[fwsize - 1] != FTS_VENDOR_1_ID)
        && (pbt_buf[fwsize - 1] != FTS_VENDOR_2_ID)) {
        FTS_ERROR("[UPGRADE]: vendor id is error, app.bin upgrade failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    /*check the app.bin invalid or not */
    ecc_ok = fts_check_app_bin(pbt_buf, fwsize);

    if (ecc_ok) {
        FTS_INFO("[UPGRADE]: app.bin ecc ok!!");
        i_ret = fts_ctpm_fw_upgrade_use_buf(client, pbt_buf, fwsize);
        if (i_ret != 0) {
            FTS_ERROR("[UPGRADE]: app.bin upgrade failed!!");
            kfree(pbt_buf);
            return -EIO;
        }
        else if (fts_updateinfo_curr.AUTO_CLB == AUTO_CLB_NEED) {
            fts_ctpm_auto_clb(client);
        }
    }
    else {
        FTS_ERROR("[UPGRADE]: app.bin ecc failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    kfree(pbt_buf);

    return i_ret;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_get_vendorid_from_boot
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static unsigned char fts_ctpm_fw_upgrade_get_vendorid_from_boot(struct
                                                                i2c_client
                                                                *client)
{
    unsigned char auc_i2c_write_buf[10];
    unsigned char reg_val[4] = { 0 };
    unsigned char i = 0;
    unsigned char boot_vendor_id = 0xFF;
    int i_ret;

    fts_ctpm_i2c_hid2std(client);

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register FTS_RST_CMD_REG1 */
        fts_i2c_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_AA);
        msleep(fts_updateinfo_curr.delay_aa);

        //write 0x55 to register FTS_RST_CMD_REG1
        fts_i2c_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_55);
        msleep(200);
        /*********Step 2:Enter upgrade mode *****/
        i_ret = fts_ctpm_i2c_hid2std(client);

        if (i_ret == 0) {
            FTS_DEBUG("[UPGRADE]: hid change to i2c fail!!");
        }
        msleep(10);
        auc_i2c_write_buf[0] = FTS_UPGRADE_55;
        auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
        i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
        if (i_ret < 0) {
            FTS_ERROR("[UPGRADE]: failed writing  0x55 and 0xaa!!");
            continue;
        }
        /*********Step 3:check READ-ID***********************/
        msleep(1);
        auc_i2c_write_buf[0] = FTS_READ_ID_REG;
        auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
            0x00;
        reg_val[0] = reg_val[1] = 0x00;
        fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

        if (reg_val[0] == chip_types.pramboot_idh
            && reg_val[1] == chip_types.pramboot_idl) {
            FTS_DEBUG
                ("[UPGRADE]: read bootload id ok!! ID1 = 0x%x,ID2 = 0x%x!!",
                 reg_val[0], reg_val[1]);
            break;
        }
        else {
            FTS_ERROR
                ("[UPGRADE]: read bootload id fail!! ID1 = 0x%x,ID2 = 0x%x!!",
                 reg_val[0], reg_val[1]);
            continue;
        }
    }

    if (i >= FTS_UPGRADE_LOOP)
        return -EIO;

    FTS_DEBUG("[UPGRADE]: FTS_UPGRADE_LOOP is  i = %d \n", i);

    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
        auc_i2c_write_buf[0] = 0x03;
        auc_i2c_write_buf[1] = 0x00;
        auc_i2c_write_buf[2] = 0xd7;
        auc_i2c_write_buf[3] = 0x83;
        i_ret = fts_i2c_write(client, auc_i2c_write_buf, 4);
        if (i_ret < 0) {
            FTS_DEBUG
                ("[UPGRADE]: read vendor id from flash error when i2c write, i_ret = %d!!",
                 i_ret);
            continue;
        }

        i_ret = fts_i2c_read(client, auc_i2c_write_buf, 0, reg_val, 4);
        if (i_ret < 0) {
            FTS_DEBUG
                ("[UPGRADE]: read vendor id from flash error when i2c write, i_ret = %d!!",
                 i_ret);
            continue;
        }

        boot_vendor_id = reg_val[1];
        FTS_DEBUG("[UPGRADE]: boot_vendor_id = 0x%x!!", reg_val[1]);
        break;
    }
    FTS_DEBUG("[UPGRADE]: reset the new FW\n");
    auc_i2c_write_buf[0] = 0x07;
    fts_i2c_write(client, auc_i2c_write_buf, 1);
    msleep(300);                /*make sure CTP startup normally */
    return boot_vendor_id;
}
