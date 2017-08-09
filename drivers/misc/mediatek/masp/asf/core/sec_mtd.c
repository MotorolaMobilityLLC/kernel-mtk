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

#include "sec_osal.h"
#include "sec_osal_light.h"
#include "sec_boot_lib.h"
#include "sec_rom_info.h"
#include "sec_secroimg.h"
#include "sec_boot.h"
#include "sec_error.h"
#include "alg_sha1.h"
#include "sec_mtd.h"
#include "sec_typedef.h"
#include "sec_log.h"

/**************************************************************************
 *  MACRO
 **************************************************************************/
#define MOD                         "ASF"

/**************************************************************************
 *  GET MTD PARTITION OFFSET
 **************************************************************************/
unsigned int sec_mtd_get_off(char *part_name)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_MTD_PARTITIONS; i++) {
		if (0 == mcmp(mtd_part_map[i].name, part_name, strlen(mtd_part_map[i].name)))
			return mtd_part_map[i].off;
	}

	SEC_ASSERT(0);
	return 0;
}

/**************************************************************************
 *  READ IMAGE
 **************************************************************************/
unsigned int sec_mtd_read_image(char *part_name, char *buf, unsigned int off, unsigned int size)
{
	ASF_FILE fp;
	unsigned int ret = SEC_OK;
	unsigned int i = 0;
	char mtd_name[32];
	unsigned int part_index = 0;

	/* find which partition should be updated in mtd */
	for (i = 0; i < MAX_MTD_PARTITIONS; i++) {
		if (0 == mcmp(mtd_part_map[i].name, part_name, strlen(part_name))) {
			part_index = i;
			break;
		}
	}

	if (MAX_MTD_PARTITIONS == i) {
		ret = ERR_SBOOT_UPDATE_IMG_NOT_FOUND_IN_MTD;
		goto _end;
	}


	/* indicate which partition */
	sprintf(mtd_name, "/dev/mtd/mtd%d", part_index);

	fp = ASF_OPEN(mtd_name);
	if (ASF_IS_ERR(fp)) {
		SMSG(true, "[%s] open fail\n", MOD);
		ret = ERR_SBOOT_UPDATE_IMG_OPEN_FAIL;
		goto _open_fail;
	}

	/* configure file system type */
	osal_set_kernel_fs();

	/* adjust read off */
	ASF_SEEK_SET(fp, off);

	/* read image to input buf */
	if (0 >= ASF_READ(fp, buf, size)) {
		ret = ERR_SBOOT_UPDATE_IMG_READ_FAIL;
		goto _read_fail;
	}

_read_fail:
	ASF_CLOSE(fp);
	osal_restore_fs();
_open_fail:
_end:
	return ret;
}
