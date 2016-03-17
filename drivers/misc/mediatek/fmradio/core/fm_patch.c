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
#include <asm/uaccess.h>

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"

/*
 * fm_file_exist - test file
 * @filename - source file name
 * If exsit, return 0, else error code
 */
fm_s32 fm_file_exist(const fm_s8 *filename)
{
	fm_s32 ret = 0;
	mm_segment_t old_fs;
	struct file *fp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(filename, O_RDONLY, 0);

	if (IS_ERR(fp)) {
		WCN_DBG(FM_ERR | CHIP, "open \"%s\" failed\n", filename);
		set_fs(old_fs);
		return -FM_EPATCH;
	}

	WCN_DBG(FM_DBG | CHIP, "open \"%s\" ok\n", filename);

	if (fp)
		filp_close(fp, NULL);

	set_fs(old_fs);

	return ret;
}

/*
 * fm_file_read - read FM DSP patch/coeff/hwcoeff/rom binary file
 * @filename - source file name
 * @dst - target buffer
 * @len - desired read length
 * @position - the read position
 * If success, return read length in bytes, else error code
 */
fm_s32 fm_file_read(const fm_s8 *filename, fm_u8 *dst, fm_s32 len, fm_s32 position)
{
	fm_s32 ret = 0;
	loff_t pos = position;
	mm_segment_t old_fs;
	struct file *fp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(filename, O_RDONLY, 0);

	if ((!fp) || IS_ERR(fp)) {
		WCN_DBG(FM_ERR | CHIP, "open \"%s\" failed\n", filename);
		set_fs(old_fs);
		return -FM_EPATCH;
	}

	ret = vfs_read(fp, (char __user *)dst, len, &pos);

	if (ret < 0)
		WCN_DBG(FM_ERR | CHIP, "read \"%s\" failed\n", filename);
	else if (ret < len)
		WCN_DBG(FM_DBG | CHIP, "read \"%s\" part data\n", filename);
	else
		WCN_DBG(FM_DBG | CHIP, "read \"%s\" full data\n", filename);

	if (fp)
		filp_close(fp, NULL);

	set_fs(old_fs);

	return ret;
}

fm_s32 fm_file_write(const fm_s8 *filename, fm_u8 *dst, fm_s32 len, fm_s32 *ppos)
{
	fm_s32 ret = 0;
	loff_t pos = *ppos;
	mm_segment_t old_fs;
	struct file *fp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(filename, O_CREAT | O_RDWR, 0);

	if ((!fp) || IS_ERR(fp)) {
		WCN_DBG(FM_ERR | CHIP, "open \"%s\" failed\n", filename);
		set_fs(old_fs);
		return -FM_EPATCH;
	}

	WCN_DBG(FM_DBG | CHIP, "open \"%s\" ok\n", filename);
	WCN_DBG(FM_DBG | CHIP, "\"%s\" old pos %d\n", filename, (int)pos);
	ret = vfs_write(fp, (char __user *)dst, len, &pos);
	WCN_DBG(FM_DBG | CHIP, "\"%s\" new pos %d\n", filename, (int)pos);
	*ppos = pos;
	if (ret < 0)
		WCN_DBG(FM_ERR | CHIP, "write \"%s\" failed\n", filename);
	else if (ret < len)
		WCN_DBG(FM_DBG | CHIP, "write \"%s\" data\n", filename);

	if (fp)
		filp_close(fp, NULL);

	set_fs(old_fs);

	return ret;
}
