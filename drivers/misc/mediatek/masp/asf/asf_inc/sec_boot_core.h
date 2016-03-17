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

#ifndef SEC_BOOT_CORE_H
#define SEC_BOOT_CORE_H

/**************************************************************************
 * EXPORT FUNCTION
 **************************************************************************/
extern int masp_boot_init(void);

/* get platform configuration */
extern int sec_boot_enabled(void);
extern int sec_modem_auth_enabled(void);
extern int sec_sds_enabled(void);
extern int sec_schip_enabled(void);

/* sec boot check */
extern bool sec_boot_check_part_enabled(char *part_name);

/* HACC HW init */
extern unsigned int sec_boot_hacc_init(void);

#endif				/* SEC_BOOT_CORE_H */
