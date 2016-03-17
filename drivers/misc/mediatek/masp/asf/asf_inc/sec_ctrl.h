/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef SECLIB_CTRL_H
#define SECLIB_CTRL_H

/**************************************************************************
 * [SEC-CTRL ID]
 **************************************************************************/
#define ROM_INFO_SEC_CTRL_ID            "AND_SECCTRL_v"
#define ROM_INFO_SEC_CTRL_VER           0x1

/**************************************************************************
 * [SEC-CTRL FORMAT]
 **************************************************************************/
#define AND_SEC_CTRL_SIZE               (52)

typedef struct {
	unsigned char m_id[16];
	unsigned int m_sec_cfg_ver;
	unsigned int m_sec_usb_dl;
	unsigned int m_sec_boot;
	unsigned int m_sec_modem_auth;
	unsigned int m_sec_sds_en;
	unsigned char m_seccfg_ac_en;
	unsigned char m_sec_aes_legacy;
	unsigned char m_secro_ac_en;
	unsigned char m_sml_aes_key_ac_en;
	unsigned int reserve[3];

} AND_SECCTRL_T;

extern void sec_ctrl_init(void);

#endif				/* SECLIB_CTRL_H */
