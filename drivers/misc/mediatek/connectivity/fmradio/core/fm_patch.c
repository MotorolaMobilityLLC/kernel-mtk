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

#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"

/*
 * fm_file_read - read FM DSP patch/coeff/hwcoeff/rom binary file
 * @filename - source file name
 * @dst - target buffer
 * @len - desired read length
 * @position - the read position
 * If success, return read length in bytes, else error code
 */
signed int fm_file_read(const signed char *filename, unsigned char *dst, signed int len, signed int position)
{
	const struct firmware *fw = NULL;
	signed int ret = 0;

	ret = request_firmware(&fw, filename, NULL);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "Failed to load firmware \"%s\"\n", filename);
		return -FM_EPATCH;
	}
	WCN_DBG(FM_NTC | CHIP, "load firmware \"%s\" ok\n", filename);

	if (len >= fw->size) {
		memcpy(dst, fw->data, fw->size);
		WCN_DBG(FM_NTC | CHIP, "Copy file data(%p) size(%zu)\n", fw->data, fw->size);
		ret = fw->size;
	} else {
		WCN_DBG(FM_NTC | CHIP, "Copy file data failed fw->size(%zu) > bufsize(%d)\n", fw->size, len);
		ret = -FM_EPATCH;
	}
	release_firmware(fw);
	return ret;
}

signed int fm_file_write(const signed char *filename, unsigned char *dst, signed int len, signed int *ppos)
{
	signed int ret = 0;
	return ret;
}
