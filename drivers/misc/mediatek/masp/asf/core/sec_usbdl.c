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

#include "sec_hal.h"
#include "sec_typedef.h"
#include "sec_rom_info.h"
#include "sec_usbdl.h"
#include "sec_log.h"
#include "sec_boot_lib.h"

/******************************************************************************
 *  DEFINITIONS
 ******************************************************************************/
#define MOD                         "ASF"

/******************************************************************************
 * CHECK IF SECURE USBDL IS ENABLED
 ******************************************************************************/
int sec_usbdl_enabled(void)
{
	switch (rom_info.m_SEC_CTRL.m_sec_usb_dl) {
	case ATTR_SUSBDL_ENABLE:
		SMSG(bMsg, "[%s] SUSBDL is enabled\n", MOD);
		SMSG(bMsg, "0x%x, SD-FORCE\n", ATTR_SUSBDL_ENABLE);
		return 1;

		/* SUSBDL can't be disabled on security chip */
	case ATTR_SUSBDL_DISABLE:
	case ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP:
		SMSG(bMsg, "[%s] SUSBDL is only enabled on S-CHIP\n", MOD);
		if (TRUE == masp_hal_sbc_enabled()) {
			SMSG(true, "0x%x, SD-SC\n", ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP);
			return 1;
		}
		SMSG(true, "0x%x,SD-NSC\n", ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP);
		return 0;

	default:
		SMSG(true, "[%s] invalid susbdl config (SD-0x%x)\n", MOD,
		     rom_info.m_SEC_CTRL.m_sec_usb_dl);
		SEC_ASSERT(0);
		return 1;
	}
}
