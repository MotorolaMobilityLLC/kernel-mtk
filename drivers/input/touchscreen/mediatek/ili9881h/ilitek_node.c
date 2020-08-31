/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ilitek.h"

#define USER_STR_BUFF		PAGE_SIZE
#define IOCTL_I2C_BUFF		PAGE_SIZE
#define ILITEK_IOCTL_MAGIC	100
#define ILITEK_IOCTL_MAXNR	23

#define ILITEK_IOCTL_I2C_WRITE_DATA		_IOWR(ILITEK_IOCTL_MAGIC, 0, u8*)
#define ILITEK_IOCTL_I2C_SET_WRITE_LENGTH	_IOWR(ILITEK_IOCTL_MAGIC, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA		_IOWR(ILITEK_IOCTL_MAGIC, 2, u8*)
#define ILITEK_IOCTL_I2C_SET_READ_LENGTH	_IOWR(ILITEK_IOCTL_MAGIC, 3, int)

#define ILITEK_IOCTL_TP_HW_RESET		_IOWR(ILITEK_IOCTL_MAGIC, 4, int)
#define ILITEK_IOCTL_TP_POWER_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 5, int)
#define ILITEK_IOCTL_TP_REPORT_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 6, int)
#define ILITEK_IOCTL_TP_IRQ_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 7, int)

#define ILITEK_IOCTL_TP_DEBUG_LEVEL		_IOWR(ILITEK_IOCTL_MAGIC, 8, int)
#define ILITEK_IOCTL_TP_FUNC_MODE		_IOWR(ILITEK_IOCTL_MAGIC, 9, int)

#define ILITEK_IOCTL_TP_FW_VER			_IOWR(ILITEK_IOCTL_MAGIC, 10, u8*)
#define ILITEK_IOCTL_TP_PL_VER			_IOWR(ILITEK_IOCTL_MAGIC, 11, u8*)
#define ILITEK_IOCTL_TP_CORE_VER		_IOWR(ILITEK_IOCTL_MAGIC, 12, u8*)
#define ILITEK_IOCTL_TP_DRV_VER			_IOWR(ILITEK_IOCTL_MAGIC, 13, u8*)
#define ILITEK_IOCTL_TP_CHIP_ID			_IOWR(ILITEK_IOCTL_MAGIC, 14, u32*)

#define ILITEK_IOCTL_TP_NETLINK_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 15, int*)
#define ILITEK_IOCTL_TP_NETLINK_STATUS		_IOWR(ILITEK_IOCTL_MAGIC, 16, int*)

#define ILITEK_IOCTL_TP_MODE_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 17, u8*)
#define ILITEK_IOCTL_TP_MODE_STATUS		_IOWR(ILITEK_IOCTL_MAGIC, 18, int*)
#define ILITEK_IOCTL_ICE_MODE_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 19, int)

#define ILITEK_IOCTL_TP_INTERFACE_TYPE		_IOWR(ILITEK_IOCTL_MAGIC, 20, u8*)
#define ILITEK_IOCTL_TP_DUMP_FLASH		_IOWR(ILITEK_IOCTL_MAGIC, 21, int)
#define ILITEK_IOCTL_TP_FW_UART_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 22, u8*)

#ifdef CONFIG_COMPAT
#define ILITEK_COMPAT_IOCTL_I2C_WRITE_DATA		_IOWR(ILITEK_IOCTL_MAGIC, 0, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_I2C_SET_WRITE_LENGTH	_IOWR(ILITEK_IOCTL_MAGIC, 1, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_I2C_READ_DATA		_IOWR(ILITEK_IOCTL_MAGIC, 2, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_I2C_SET_READ_LENGTH		_IOWR(ILITEK_IOCTL_MAGIC, 3, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_HW_RESET			_IOWR(ILITEK_IOCTL_MAGIC, 4, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_POWER_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 5, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_REPORT_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 6, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_IRQ_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 7, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_DEBUG_LEVEL		_IOWR(ILITEK_IOCTL_MAGIC, 8, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_FUNC_MODE		_IOWR(ILITEK_IOCTL_MAGIC, 9, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_FW_VER			_IOWR(ILITEK_IOCTL_MAGIC, 10, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_PL_VER			_IOWR(ILITEK_IOCTL_MAGIC, 11, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_CORE_VER			_IOWR(ILITEK_IOCTL_MAGIC, 12, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_DRV_VER			_IOWR(ILITEK_IOCTL_MAGIC, 13, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_CHIP_ID			_IOWR(ILITEK_IOCTL_MAGIC, 14, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_NETLINK_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 15, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_NETLINK_STATUS		_IOWR(ILITEK_IOCTL_MAGIC, 16, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_MODE_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 17, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_MODE_STATUS		_IOWR(ILITEK_IOCTL_MAGIC, 18, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_ICE_MODE_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 19, compat_uptr_t)

#define ILITEK_COMPAT_IOCTL_TP_INTERFACE_TYPE		_IOWR(ILITEK_IOCTL_MAGIC, 20, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_DUMP_FLASH		_IOWR(ILITEK_IOCTL_MAGIC, 21, compat_uptr_t)
#define ILITEK_COMPAT_IOCTL_TP_FW_UART_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 22, compat_uptr_t)
#endif

struct record_state {
	u8 touch_palm_state_e : 2;
	u8 app_an_statu_e : 3;
	u8 app_sys_check_bg_abnormal : 1;
	u8 g_b_wrong_bg : 1;
};

unsigned char g_user_buf[USER_STR_BUFF] = {0};

int str2hex(char *str)
{
	int strlen, result, intermed, intermedtop;
	char *s = str;

	while (*s != 0x0) {
		s++;
	}

	strlen = (int)(s - str);
	s = str;
	if (*s != 0x30) {
		return -1;
	}

	s++;

	if (*s != 0x78 && *s != 0x58) {
		return -1;
	}
	s++;

	strlen = strlen - 3;
	result = 0;
	while (*s != 0x0) {
		intermed = *s & 0x0f;
		intermedtop = *s & 0xf0;
		if (intermedtop == 0x60 || intermedtop == 0x40) {
			intermed += 0x09;
		}
		intermed = intermed << (strlen << 2);
		result = result | intermed;
		strlen -= 1;
		s++;
	}
	return result;
}

int katoi(char *str)
{
	int result = 0;
	unsigned int digit;
	int sign;

	if (*str == '-') {
		sign = 1;
		str += 1;
	} else {
		sign = 0;
		if (*str == '+') {
			str += 1;
		}
	}

	for (;; str += 1) {
		digit = *str - '0';
		if (digit > 9)
			break;
		result = (10 * result) + digit;
	}

	if (sign) {
		return -result;
	}
	return result;
}

struct file_buffer {
	char *ptr;
	char fname[128];
	int32_t wlen;
	int32_t flen;
	int32_t wtemp_zise;
};

static int file_write(struct file_buffer *file, bool new_open)
{
	struct file *f = NULL;
	mm_segment_t fs;
	loff_t pos;

	if (file->ptr == NULL) {
		ipio_err("str is invaild\n");
		return -1;
	}

	if (file->fname == NULL) {
		ipio_err("file name is invaild\n");
		return -1;
	}

	if (file->wlen >= file->wtemp_zise) {
		ipio_err("Saved to file length is too long !, %d\n", file->wlen);
		return -1;
	}

	if (new_open)
		f = filp_open(file->fname, O_WRONLY | O_CREAT | O_TRUNC, 644);
	else
		f = filp_open(file->fname, O_WRONLY | O_CREAT | O_APPEND, 644);

	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open %s file\n", file->fname);
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, file->ptr, file->wlen, &pos);
	set_fs(fs);
	filp_close(f, NULL);
	file->flen += file->wlen;
	return 0;
}

void ilitek_debug_node_buff_control(void)
{
	int i;
	bool free_flag = false;

	idev->debug_node_open = !idev->debug_node_open;

	if (idev->debug_node_open == true) {
		idev->debug_data_frame = 0;
		idev->out_data_index = 0;

		if (idev->debug_buf == NULL) {
			idev->debug_buf = (struct debug_buf_list *)kzalloc(TR_BUF_LIST_SIZE * sizeof(*idev->debug_buf), GFP_KERNEL);
			if (ERR_ALLOC_MEM(idev->debug_buf)) {
				ipio_err("Failed to allocate idev->debug_buf mem, %ld\n", PTR_ERR(idev->debug_buf));
				free_flag = true;
			}
		}

		if (idev->debug_buf  != NULL) {
			for (i = 0; i < TR_BUF_LIST_SIZE; i++) {
				idev->debug_buf[i].mark = false;
				if (idev->debug_buf[i].data == NULL) {
					idev->debug_buf[i].data = (unsigned char *)kzalloc(TR_BUF_SIZE * sizeof(unsigned char), GFP_KERNEL);
					if (ERR_ALLOC_MEM(idev->debug_buf[i].data)) {
						ipio_err("Failed to allocate debug_buf[%d] mem, %ld\n", i, PTR_ERR(idev->debug_buf[i].data));
						free_flag = true;
						break;
					}
				}
			}
		}

	}

	/* Note that it might be freed by next touch event */
	if (free_flag || (!idev->debug_node_open)) {
		if (idev->debug_buf != NULL) {
			for (i = 0; i < TR_BUF_LIST_SIZE; i++) {
				idev->debug_buf[i].mark = false;
				if (idev->debug_buf[i].data != NULL) {
					kfree(idev->debug_buf[i].data);
					idev->debug_buf[i].data = NULL;
				}
			}
			kfree(idev->debug_buf);
			idev->debug_buf = NULL;
		}
		idev->debug_node_open = false;
	}
}

static int debug_mode_get_data(struct file_buffer *file, u8 type, u32 frame_count)
{
	int ret;
	u8 cmd[2] = { 0 }, row, col;
	s16 temp;
	unsigned char *ptr;
	int j;
	u16 write_index = 0;

	if (idev->debug_node_open)
		ilitek_debug_node_buff_control();
	idev->debug_data_frame = 0;
	row = idev->ych_num;
	col = idev->xch_num;

	mutex_lock(&idev->touch_mutex);
	cmd[0] = 0xFA;
	cmd[1] = type;
	ret = idev->write(cmd, 2);
	ilitek_debug_node_buff_control();
	mutex_unlock(&idev->touch_mutex);
	if (!idev->debug_node_open){
		ipio_err("open debug fail\n");
		return ret;
	}
	if (ret < 0) {
		ipio_err("Write 0xFA,0x%x failed\n", type);
		return ret;
	}

	while (write_index < frame_count) {
		file->wlen = 0;
		ipio_info("frame = %d,index = %d,count = %d\n", write_index,idev->out_data_index, idev->debug_data_frame);
		if (!wait_event_interruptible_timeout(idev->inq, idev->debug_buf[idev->out_data_index].mark, msecs_to_jiffies(3000))) {
			ipio_err("debug mode get data timeout!\n");
			goto out;
		}

		mutex_lock(&idev->touch_mutex);

		memset(file->ptr, 0, file->wtemp_zise);
		file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "\n\nFrame%d,", write_index);
		for (j = 0; j < col; j++)
			file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "[X%d] ,", j);
		ptr = &idev->debug_buf[idev->out_data_index].data[35];
		for (j = 0; j < row * col; j++, ptr += 2) {
			temp = (*ptr << 8) + *(ptr + 1);
			if (j % col == 0)
				file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "\n[Y%d] ,", (j / col));
			file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "%d, ", temp);
		}
		file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "\n[X] ,");
		for (j = 0; j < row + col; j++, ptr += 2) {
			temp = (*ptr << 8) + *(ptr + 1);
			if (j == col)
				file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "\n[Y] ,");
			file->wlen += snprintf(file->ptr + file->wlen, (file->wtemp_zise - file->wlen), "%d, ", temp);
		}
		file_write(file, false);
		write_index++;

		mutex_unlock(&idev->touch_mutex);

		idev->debug_buf[idev->out_data_index].mark = false;
		idev->out_data_index = ((idev->out_data_index + 1) % TR_BUF_LIST_SIZE);
	}
out:
	ilitek_debug_node_buff_control();
	return 0;
}

static int dev_mkdir(char *name, umode_t mode)
{
	int err;
	mm_segment_t fs;

	ipio_info("mkdir: %s\n", name);
	fs = get_fs();
	set_fs(KERNEL_DS);
	err = sys_mkdir(name, mode);
	set_fs(fs);

	return err;
}

static ssize_t ilitek_proc_get_delta_data_read(struct file *pFile, char __user *buf, size_t size, loff_t *pos)
{
	s16 *delta = NULL;
	int row = 0, col = 0,  index = 0;
	int ret, i, x, y;
	int read_length = 0;
	u8 cmd[2] = {0};
	u8 *data = NULL;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);
	mutex_lock(&idev->touch_mutex);

	row = idev->ych_num;
	col = idev->xch_num;
	read_length = 4 + 2 * row * col + 1 ;

	ipio_info("read length = %d\n", read_length);

	data = kcalloc(read_length + 1, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(data)) {
		ipio_err("Failed to allocate data mem\n");
		return 0;
	}

	delta = kcalloc(P5_X_DEBUG_MODE_PACKET_LENGTH, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(delta)) {
		ipio_err("Failed to allocate delta mem\n");
		return 0;
	}

	cmd[0] = 0xB7;
	cmd[1] = 0x1; //get delta
	ret = idev->write(cmd, sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x1 command, %d\n", ret);
		goto out;
	}

	msleep(120);

	/* read debug packet header */
	ret = idev->read(data, read_length);
	if (ret < 0) {
		ipio_err("Read debug packet header failed, %d\n", ret);
		goto out;
	}

	cmd[1] = 0x03; //switch to normal mode
	ret = idev->write(cmd, sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x3 command, %d\n", ret);
		goto out;
	}

	for (i = 4, index = 0; index < row * col * 2; i += 2, index++)
		delta[index] = (data[i] << 8) + data[i + 1];

	size = snprintf(g_user_buf + size, PAGE_SIZE - size, "======== Deltadata ========\n");
	ipio_info("======== Deltadata ========\n");

	size += snprintf(g_user_buf + size, PAGE_SIZE - size,
		"Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2] << 8) | data[3]);
	ipio_info("Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2] << 8) | data[3]);

	// print delta data
	for (y = 0; y < row; y++) {
		size += snprintf(g_user_buf + size, PAGE_SIZE - size, "[%2d] ", (y+1));
		ipio_info("[%2d] ", (y+1));

		for (x = 0; x < col; x++) {
			int shift = y * col + x;
			size += snprintf(g_user_buf + size, PAGE_SIZE - size, "%5d", delta[shift]);
			printk(KERN_CONT "%5d", delta[shift]);
		}
		size += snprintf(g_user_buf + size, PAGE_SIZE - size, "\n");
		printk(KERN_CONT "\n");
	}

	if (copy_to_user(buf, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	*pos += size;

out:
	mutex_unlock(&idev->touch_mutex);
	ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);
	ipio_kfree((void **)&data);
	ipio_kfree((void **)&delta);
	return 0;
}

static ssize_t ilitek_proc_fw_get_raw_data_read(struct file *pFile, char __user *buf, size_t size, loff_t *pos)
{
	s16 *rawdata = NULL;
	int row = 0, col = 0,  index = 0;
	int ret, i, x, y, len;
	int read_length = 0;
	u8 cmd[2] = {0};
	u8 *data = NULL;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);
	mutex_lock(&idev->touch_mutex);

	row = idev->ych_num;
	col = idev->xch_num;
	read_length = 4 + 2 * row * col + 1 ;

	ipio_info("read length = %d\n", read_length);

	data = kcalloc(read_length + 1, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(data)) {
			ipio_err("Failed to allocate data mem\n");
			return 0;
	}

	rawdata = kcalloc(P5_X_DEBUG_MODE_PACKET_LENGTH, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(rawdata)) {
			ipio_err("Failed to allocate rawdata mem\n");
			return 0;
	}

	cmd[0] = 0xB7;
	cmd[1] = 0x2; //get rawdata
	ret = idev->write(cmd, sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x2 command, %d\n", ret);
		goto out;
	}

	//msleep(20);
	msleep(120);

	/* read debug packet header */
	ret = idev->read(data, read_length);
	if (ret < 0) {
		ipio_err("Read debug packet header failed, %d\n", ret);
		goto out;
	}

	cmd[1] = 0x03; //switch to normal mode
	ret = idev->write(cmd, sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x3 command, %d\n", ret);
		goto out;
	}

	len = row * col * 2;
	for (i = 4, index = 0; index < len; i += 2, index++)
		rawdata[index] = (data[i] << 8) + data[i + 1];

	size = snprintf(g_user_buf, PAGE_SIZE, "======== RawData ========\n");
	ipio_info("======== RawData ========\n");

	size += snprintf(g_user_buf + size, PAGE_SIZE - size,
			"Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2] << 8) | data[3]);
	ipio_info("Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2] << 8) | data[3]);

	// print raw data
	for (y = 0; y < row; y++) {
		size += snprintf(g_user_buf + size, PAGE_SIZE - size, "[%2d] ", (y+1));
		ipio_info("[%2d] ", (y+1));

		for (x = 0; x < col; x++) {
			int shift = y * col + x;
			size += snprintf(g_user_buf + size, PAGE_SIZE - size, "%5d", rawdata[shift]);
			printk(KERN_CONT "%5d", rawdata[shift]);
		}
		size += snprintf(g_user_buf + size, PAGE_SIZE - size, "\n");
		printk(KERN_CONT "\n");
	}

	if (copy_to_user(buf, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	*pos += size;

out:
	mutex_unlock(&idev->touch_mutex);
	ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);
	ipio_kfree((void **)&data);
	ipio_kfree((void **)&rawdata);
	return 0;
}

static ssize_t ilitek_proc_fw_pc_counter_read(struct file *pFile, char __user *buf, size_t size, loff_t *pos)
{
	u32 pc;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	pc = ilitek_tddi_ic_get_pc_counter();
	size = snprintf(g_user_buf, PAGE_SIZE, "pc counter = 0x%x\n", pc);

	if (copy_to_user(buf, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	*pos += size;
	return size;
}

u32 rw_reg[5] = {0};
static ssize_t ilitek_proc_rw_tp_reg_read(struct file *pFile, char __user *buf, size_t size, loff_t *pos)
{
	int ret = 0;
	bool mcu_on = 0, read = 0;
	u32 type, addr, read_data, write_data, write_len, stop_mcu;
	bool esd_en = idev->wq_esd_ctrl, bat_en = idev->wq_bat_ctrl;

	if (*pos != 0)
		return 0;

	stop_mcu = rw_reg[0];
	type = rw_reg[1];
	addr = rw_reg[2];
	write_data = rw_reg[3];
	write_len = rw_reg[4];

	ipio_info("stop_mcu = %d\n", rw_reg[0]);

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);

	mutex_lock(&idev->touch_mutex);

	if (stop_mcu == mcu_on) {
		ret = ilitek_ice_mode_ctrl(ENABLE, ON);
		if (ret < 0) {
			ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
			return -1;
		}
	} else {
		ret = ilitek_ice_mode_ctrl(ENABLE, OFF);
		if (ret < 0) {
			ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
			return -1;
		}
	}

	if (type == read) {
		if (ilitek_ice_mode_read(addr, &read_data, sizeof(u32)) < 0)
			ipio_err("Read data error\n");
		ipio_info("READ:addr = 0x%06x, read = 0x%08x\n", addr, read_data);
		size = snprintf(g_user_buf, PAGE_SIZE, "READ:addr = 0x%06x, read = 0x%08x\n", addr, read_data);
	} else {
		if (ilitek_ice_mode_write(addr, write_data, write_len) < 0)
			ipio_err("Write data error\n");
		ipio_info("WRITE:addr = 0x%06x, write = 0x%08x, len =%d byte\n", addr, write_data, write_len);
		size = snprintf(g_user_buf, PAGE_SIZE, "WRITE:addr = 0x%06x, write = 0x%08x, len =%d byte\n", addr, write_data, write_len);
	}

	if (stop_mcu == mcu_on) {
		ret = ilitek_ice_mode_ctrl(DISABLE, ON);
		if (ret < 0) {
			ipio_err("Failed to disable ICE mode, ret = %d\n", ret);
			return -1;
		}
	} else {
		ret = ilitek_ice_mode_ctrl(DISABLE, OFF);
		if (ret < 0) {
			ipio_err("Failed to disable ICE mode, ret = %d\n", ret);
			return -1;
		}
	}

	if (copy_to_user(buf, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	*pos += size;
	mutex_unlock(&idev->touch_mutex);

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);

	return size;
}

static ssize_t ilitek_proc_rw_tp_reg_write(struct file *filp, const char *buff, size_t size, loff_t *pos)
{
	char *token = NULL, *cur = NULL;
	char cmd[256] = { 0 };
	u32 count = 0;

	if ((size - 1) > sizeof(cmd)) {
		ipio_err("ERROR! input length is larger than local buffer\n");
		return -1;
	}

	if (buff != NULL) {
		if (copy_from_user(cmd, buff, size - 1)) {
			ipio_info("Failed to copy data from user space\n");
			return -1;
		}
	}
	token = cur = cmd;
	while ((token = strsep(&cur, ",")) != NULL) {
		rw_reg[count] = str2hex(token);
		ipio_info("rw_reg[%d] = 0x%x\n", count, rw_reg[count]);
		count++;
	}
	return size;
}

static ssize_t ilitek_proc_debug_switch_read(struct file *pFile, char __user *buff, size_t size, loff_t *pos)
{

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));


	mutex_lock(&idev->debug_mutex);

	ilitek_debug_node_buff_control();

	size = snprintf(g_user_buf, USER_STR_BUFF * sizeof(unsigned char), "debug_node_open : %s\n", idev->debug_node_open ? "Enabled" : "Disabled");
	*pos = size;

	ipio_info(" %s debug_flag message = %x\n", idev->debug_node_open ? "Enabled" : "Disabled", idev->debug_node_open);
	if (copy_to_user(buff, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	mutex_unlock(&idev->debug_mutex);

	return size;
}

static ssize_t ilitek_proc_debug_message_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	unsigned long p = *pos;
	int i = 0;
	int send_data_len = 0;
	int ret = 0;
	int data_count = 0;
	int one_data_bytes = 0;
	int need_read_data_len = 0;
	int type = 0;
	unsigned char *tmpbuf = NULL;
	unsigned char tmpbufback[128] = {0};

	if (filp->f_flags & O_NONBLOCK) {
		return -EAGAIN;
	}

	mutex_lock(&idev->debug_read_mutex);
	ipio_debug("f_count= %d, index = %d, mark = %d\n", idev->debug_data_frame, idev->out_data_index, idev->debug_buf[idev->out_data_index].mark);
	if (!wait_event_interruptible_timeout(idev->inq, idev->debug_buf[idev->out_data_index].mark, msecs_to_jiffies(3000))) {
		ipio_err("Error! get debug data fail\n");
		*pos = send_data_len;
		mutex_unlock(&idev->debug_read_mutex);
		return send_data_len;
	}
	mutex_lock(&idev->debug_mutex);

	tmpbuf = vmalloc(4096);	/* buf size if even */
	if (ERR_ALLOC_MEM(tmpbuf)) {
		ipio_err("buffer vmalloc error\n");
		send_data_len += snprintf(tmpbufback + send_data_len, sizeof(tmpbufback), "buffer vmalloc error\n");
		ret = copy_to_user(buff, tmpbufback, send_data_len); /*idev->debug_buf[0] */
		goto out;
	}

	if (idev->debug_buf[idev->out_data_index].mark) {
		if (idev->debug_buf[idev->out_data_index].data[0] == P5_X_DEMO_PACKET_ID) {
			need_read_data_len = 43;
		} else if (idev->debug_buf[idev->out_data_index].data[0] == P5_X_I2CUART_PACKET_ID) {
			type = idev->debug_buf[idev->out_data_index].data[3] & 0x0F;

			data_count = idev->debug_buf[idev->out_data_index].data[1] * idev->debug_buf[idev->out_data_index].data[2];

			if (type == 0 || type == 1 || type == 6) {
				one_data_bytes = 1;
			} else if (type == 2 || type == 3) {
				one_data_bytes = 2;
			} else if (type == 4 || type == 5) {
				one_data_bytes = 4;
			}
			need_read_data_len = data_count * one_data_bytes + 1 + 5;
		} else if (idev->debug_buf[idev->out_data_index].data[0] == P5_X_DEBUG_PACKET_ID) {
			send_data_len = 0;	/* idev->debug_buf[0][1] - 2; */
			need_read_data_len = TR_BUF_SIZE - 8;
		}

		for (i = 0; i < need_read_data_len; i++) {
			send_data_len += snprintf(tmpbuf + send_data_len, sizeof(tmpbufback), "%02X", idev->debug_buf[idev->out_data_index].data[i]);
			if (send_data_len >= 4096) {
				ipio_err("send_data_len = %d set 4096 i = %d\n", send_data_len, i);
				send_data_len = 4096;
				break;
			}
		}

		send_data_len += snprintf(tmpbuf + send_data_len, sizeof(tmpbufback), "\n\n");

		if (p == 5 || size == 4096 || size == 2048) {
			idev->debug_buf[idev->out_data_index].mark = false;
			idev->out_data_index = ((idev->out_data_index + 1) % TR_BUF_LIST_SIZE);
		}

	}

	/* Preparing to send debug data to user */
	if (size == 4096)
		ret = copy_to_user(buff, tmpbuf, send_data_len);
	else
		ret = copy_to_user(buff, tmpbuf + p, send_data_len - p);

	/* ipio_err("send_data_len = %d\n", send_data_len); */
	if (send_data_len <= 0 || send_data_len > 4096) {
		ipio_err("send_data_len = %d set 4096\n", send_data_len);
		send_data_len = 4096;
	}

	if (ret) {
		ipio_err("copy_to_user err\n");
		ret = -EFAULT;
	} else {
		*pos += send_data_len;
		ret = send_data_len;
		ipio_debug("Read %d bytes(s) from %ld\n", send_data_len, p);
	}

out:
	mutex_unlock(&idev->debug_mutex);
	mutex_unlock(&idev->debug_read_mutex);
	ipio_vfree((void **)&tmpbuf);
	return send_data_len;
}

static ssize_t ilitek_proc_get_debug_mode_data_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	int ret;
	struct file_buffer csv;

	if (*pos != 0)
		return 0;

	/* initialize file */
	memset(csv.fname, 0, sizeof(csv.fname));
	snprintf(csv.fname, sizeof(csv.fname), "%s", DEBUG_DATA_FILE_PATH);
	csv.flen = 0;
	csv.wlen = 0;
	csv.wtemp_zise = DEBUG_DATA_FILE_SIZE;
	csv.ptr = vmalloc(csv.wtemp_zise);

	if (ERR_ALLOC_MEM(csv.ptr)) {
		ipio_err("Failed to allocate CSV mem\n");
		goto out;
	}

	/* save data to csv */
	ipio_info("Get Raw data %d frame\n", idev->raw_count);
	ipio_info("Get Delta data %d frame\n", idev->delta_count);
	csv.wlen += snprintf(csv.ptr + csv.wlen, (csv.wtemp_zise - csv.wlen), "Get Raw data %d frame\n", idev->raw_count);
	csv.wlen += snprintf(csv.ptr + csv.wlen, (csv.wtemp_zise - csv.wlen), "Get Delta data %d frame\n", idev->delta_count);

	file_write(&csv, true);

	/* change to debug mode */
	if (ilitek_set_tp_data_len(DATA_FORMAT_DEBUG) < 0) {
		ipio_err("Failed to set tp data length\n");
		goto out;
	}

	/* get raw data */
	csv.wlen = 0;
	memset(csv.ptr, 0, csv.wtemp_zise);
	csv.wlen += snprintf(csv.ptr + csv.wlen, (csv.wtemp_zise - csv.wlen), "\n\n=======Raw data=======");
	file_write(&csv, false);

	ret = debug_mode_get_data(&csv, P5_X_FW_RAW_DATA_MODE, idev->raw_count);
	if (ret < 0)
		goto out;

	/* get delta data */
	csv.wlen = 0;
	memset(csv.ptr, 0, csv.wtemp_zise);
	csv.wlen += snprintf(csv.ptr + csv.wlen, (csv.wtemp_zise - csv.wlen), "\n\n=======Delta data=======");
	file_write(&csv, false);

	ret = debug_mode_get_data(&csv, P5_X_FW_DELTA_DATA_MODE, idev->delta_count);
	if (ret < 0)
		goto out;

	/* change to demo mode */
	if (ilitek_set_tp_data_len(DATA_FORMAT_DEMO) < 0)
		ipio_err("Failed to set tp data length\n");

out:
	ipio_vfree((void **)&csv.ptr);
	return 0;
}

static ssize_t ilitek_proc_get_debug_mode_data_write(struct file *filp, const char *buff, size_t size, loff_t *pos)
{
	char *token = NULL, *cur = NULL;
	char cmd[256] = {0};
	u8 temp[256] = {0}, count = 0;

	if ((size - 1) > sizeof(cmd)) {
		ipio_err("ERROR! input length is larger than local buffer\n");
		return -1;
	}

	if (buff != NULL) {
		if (copy_from_user(cmd, buff, size - 1)) {
			ipio_info("Failed to copy data from user space\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);
	token = cur = cmd;
	while ((token = strsep(&cur, ",")) != NULL) {
		temp[count] = str2hex(token);
		ipio_info("temp[%d] = %d\n", count, temp[count]);
		count++;
	}

	idev->raw_count = ((temp[0] << 8) | temp[1]);
	idev->delta_count = ((temp[2] << 8) | temp[3]);
	idev->bg_count = ((temp[4] << 8) | temp[5]);

	ipio_info("Raw_count = %d, Delta_count = %d, BG_count = %d\n", idev->raw_count, idev->delta_count, idev->bg_count);
	return size;
}

static ssize_t ilitek_node_mp_lcm_on_test_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	int ret = 0;
	char apk_ret[100] = {0};
	bool esd_en = idev->wq_esd_ctrl, bat_en = idev->wq_bat_ctrl;

	ipio_info("Run MP test with LCM on\n");

	if (*pos != 0)
		return 0;

	mutex_lock(&idev->touch_mutex);

	/* Create the directory for mp_test result */
	if ((dev_mkdir(CSV_LCM_ON_PATH, S_IRUGO | S_IWUSR)) != 0)
		ipio_err("Failed to create directory for mp_test\n");

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);

	ret = ilitek_tddi_mp_test_handler(apk_ret, ON, NULL);
	ipio_info("MP TEST %s, Error code = %d\n", (ret < 0) ? "FAIL" : "PASS", ret);
	//apk_ret[sizeof(apk_ret) - 1] = ret;
	memset(apk_ret, 0, sizeof(apk_ret));
	if(ret <0){
		size = sprintf(apk_ret, "%s\n", "fail");
               ipio_info("fail wzx \n");
	}
	else{
	       size = sprintf(apk_ret, "%s\n", "pass");
		   ipio_info("pass wzx \n");
		}

	ret = copy_to_user((char *)buff, apk_ret, size);
		//ipio_err("Failed to copy data to user space\n");

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);

	mutex_unlock(&idev->touch_mutex);
	 *pos += size;
	return size;
}

static ssize_t ilitek_node_mp_lcm_off_test_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	int ret = 0;
	char apk_ret[100] = {0};
	bool esd_en = idev->wq_esd_ctrl, bat_en = idev->wq_bat_ctrl;

	ipio_info("Run MP test with LCM off\n");

	if (*pos != 0)
		return 0;

	mutex_lock(&idev->touch_mutex);

	/* Create the directory for mp_test result */
	ret = dev_mkdir(CSV_LCM_OFF_PATH, S_IRUGO | S_IWUSR);
	if (ret != 0)
		ipio_err("Failed to create directory for mp_test\n");

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);

	ret = ilitek_tddi_mp_test_handler(apk_ret, OFF, NULL);
	ipio_info("MP TEST %s, Error code = %d\n", (ret < 0) ? "FAIL" : "PASS", ret);
	apk_ret[sizeof(apk_ret) - 1] = ret;

	if (copy_to_user((char *)buff, apk_ret, sizeof(apk_ret)))
		ipio_err("Failed to copy data to user space\n");

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);

	mutex_unlock(&idev->touch_mutex);
	return ret;
}

static ssize_t ilitek_proc_fw_process_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	u32 len = 0;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	len = snprintf(g_user_buf, USER_STR_BUFF * sizeof(unsigned char), "%02d\n", idev->fw_update_stat);

	ipio_info("update status = %d\n", idev->fw_update_stat);

	if (copy_to_user((char *)buff, &idev->fw_update_stat, len))
		ipio_err("Failed to copy data to user space\n");

	*pos = len;
	return len;
}

static ssize_t ilitek_node_fw_upgrade_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	int ret = 0;
	u32 len = 0;
	bool esd_en = idev->wq_esd_ctrl, bat_en = idev->wq_bat_ctrl;

	ipio_info("Preparing to upgarde firmware\n");

	if (*pos != 0)
		return 0;

	mutex_lock(&idev->touch_mutex);

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);

	idev->force_fw_update = ENABLE;

	ret = ilitek_tddi_fw_upgrade_handler(NULL);
	len = snprintf(g_user_buf, USER_STR_BUFF * sizeof(unsigned char), "upgrade firwmare %s\n", (ret != 0) ? "failed" : "succeed");

	idev->force_fw_update = DISABLE;

	if (copy_to_user((u32 *) buff, g_user_buf, len))
		ipio_err("Failed to copy data to user space\n");

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);

	mutex_unlock(&idev->touch_mutex);

	return ret;
}

static ssize_t ilitek_proc_debug_level_read(struct file *filp, char __user *buff, size_t size, loff_t *pos)
{
	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ipio_debug_level = !ipio_debug_level;

	ipio_info(" %s debug level = %x\n", ipio_debug_level ? "Enable" : "Disable", ipio_debug_level);

	size = snprintf(g_user_buf, USER_STR_BUFF * sizeof(unsigned char), "debug level : %s\n", ipio_debug_level ? "Enable" : "Disable");

	*pos += size;

	if (copy_to_user((u32 *) buff, g_user_buf, size))
		ipio_err("Failed to copy data to user space\n");

	return size;
}

int get_tp_recore_ctrl(int data)
{
	int ret = 0;

	switch ((int)data) {

	case ENABLE_RECORD:
		ipio_info("recore enable");
		ret = ilitek_tddi_ic_func_ctrl("tp_recore", 1);
		mdelay(200);
		break;
	case DATA_RECORD:
		mdelay(50);
		ipio_info("Get data");
		ret = ilitek_tddi_ic_func_ctrl("tp_recore", 2);
		if (ret < 0) {
			ipio_err("cmd fail\n");
			goto out;
		}

		ret = get_tp_recore_data();
		if (ret < 0)
			ipio_err("get data fail\n");

		ipio_info("recore reset");
		ret = ilitek_tddi_ic_func_ctrl("tp_recore", 3);
		if (ret < 0) {
			ipio_err("cmd fail\n");
			goto out;
		}
		break;
	case DISABLE_RECORD:
		ipio_info("recore disable");
		ret = ilitek_tddi_ic_func_ctrl("tp_recore", 0);
		break;

	}
out:
	return ret;

}

int get_tp_recore_data(void)
{
	u8 buf[8] = {0}, record_case = 0;
	s8 index;
	u16 *raw = NULL, *raw_ptr = NULL, frame_len = 0;
	u32 base_addr = 0x20000, addr, len, *ptr, i, fcnt;
	struct record_state record_stat;
	bool ice = atomic_read(&idev->ice_stat);

	if (idev->read(buf, sizeof(buf)) < 0) {
		ipio_err("Get info fail\n");
		return -1;
	}

	addr = ((buf[0] << 8) | buf[1]) + base_addr;
	len = ((buf[2] << 8) | buf[3]);
	index = buf[4];
	fcnt = buf[5];
	record_case = buf[6];
	ipio_memcpy(&record_stat, &buf[7], 1, 1);
	ipio_info("addr = 0x%x, len = %d, lndex = 0x%x, fram num = %d, record_case = 0x%x\n",
		addr, len, index, fcnt, record_case);
	ilitek_dump_data(buf, 8, sizeof(buf), 0, "all record bytes");

	raw = kcalloc(len, sizeof(u8), GFP_ATOMIC);
	if (ERR_ALLOC_MEM(raw)) {
		ipio_err("Failed to allocate packet memory, %ld\n", PTR_ERR(raw));
		return -1;
	}
	ptr = (u32 *)raw;

	if (!ice)
		ilitek_ice_mode_ctrl(ENABLE, ON);

	buf[0] = 0x25;
	buf[3] = (char)((addr & 0x00FF0000) >> 16);
	buf[2] = (char)((addr & 0x0000FF00) >> 8);
	buf[1] = (char)((addr & 0x000000FF));

	if (idev->write(buf, 4)) {
		ipio_err("Failed to write iram data\n");
		return -ENODEV;
	}

	if (idev->read((u8 *)raw, len)) {
		ipio_err("Failed to Read iram data\n");
		return -ENODEV;
	}

	frame_len = (len / (fcnt * 2));
	for (i = 0; i < fcnt; i++) {
		raw_ptr = raw + (index * frame_len);

		ilitek_dump_data(raw_ptr, 16, frame_len, idev->xch_num, "recore_data");
		index--;
		if (index < 0)
			index = fcnt - 1;
	}

	if (!ice)
		ilitek_ice_mode_ctrl(DISABLE, ON);

	if (record_case == 2) {
		ipio_info("tp_palm_stat = %d\n", record_stat.touch_palm_state_e);
		ipio_info("app_an_stat = %d\n", record_stat.app_an_statu_e);
		ipio_info("app_check_abnor = %d\n", record_stat.app_sys_check_bg_abnormal);
		ipio_info("wrong_bg = %d\n", record_stat.g_b_wrong_bg);
	}

	ipio_kfree((void **)&raw);

	return 0;
}

void gesture_fail_reason(bool enable)
{

	u8 cmd[24] = {0};

	/* set symbol */
	if (ilitek_tddi_ic_func_ctrl("knock_en", 0x8) < 0)
		ipio_err("set symbol failed");

	/* enable gesture fail reason */
	cmd[0] = 0x01;
	cmd[1] = 0x0A;
	cmd[2] = 0x10;
	if (enable)
		cmd[3] = 0x01;
	else
		cmd[3] = 0x00;
	cmd[4] = 0xFF;
	cmd[5] = 0xFF;
	if ((idev->write(cmd, 6)) < 0)
		ipio_err("enable gesture fail reason failed");

	/* set gesture parameters */
	cmd[0] = 0x01;
	cmd[1] = 0x0A;
	cmd[2] = 0x12;
	cmd[3] = 0x01;
	memset(cmd + 4, 0xFF, 20);
	if ((idev->write(cmd, 24)) < 0)
		ipio_err("set gesture parameters failed");

	/* get gesture parameters */
	cmd[0] = 0x01;
	cmd[1] = 0x0A;
	cmd[2] = 0x11;
	cmd[3] = 0x01;
	if ((idev->write(cmd, 4)) < 0)
		ipio_err("get gesture parameters failed");

}

static ssize_t ilitek_node_ioctl_write(struct file *filp, const char *buff, size_t size, loff_t *pos)
{
	int i, count = 0;
	char cmd[512] = {0};
	char *token = NULL, *cur = NULL;
	u8 temp[256] = {0};
	u32 *data = NULL;

	if ((size - 1) > sizeof(cmd)) {
		ipio_err("ERROR! input length is larger than local buffer\n");
		return -1;
	}

	mutex_lock(&idev->touch_mutex);

	if (buff != NULL) {
		if (copy_from_user(cmd, buff, size - 1)) {
			ipio_info("Failed to copy data from user space\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);

	token = cur = cmd;

	data = kcalloc(512, sizeof(u32), GFP_KERNEL);

	while ((token = strsep(&cur, ",")) != NULL) {
		data[count] = str2hex(token);
		ipio_info("data[%d] = %x\n", count, data[count]);
		count++;
	}

	ipio_info("cmd = %s\n", cmd);

	if (strncmp(cmd, "hwreset", strlen(cmd)) == 0) {
		ilitek_tddi_reset_ctrl(TP_HW_RST_ONLY);
	} else if (strcmp(cmd, "rawdatarecore") == 0) {
		if (data[1] == 0)
			get_tp_recore_ctrl(ENABLE_RECORD);
		else if (data[1] == 1)
			get_tp_recore_ctrl(DATA_RECORD);
		else if (data[1] == 2)
			get_tp_recore_ctrl(DISABLE_RECORD);
	} else if (strcmp(cmd, "switchdemodebuginfomode") == 0) {
		ilitek_set_tp_data_len(DATA_FORMAT_DEMO_DEBUG_INFO);
	} else if (strcmp(cmd, "gesturedemoen") == 0) {
		if (data[1] == 0)
			idev->gesture_demo_ctrl = DISABLE;
		else
			idev->gesture_demo_ctrl = ENABLE;
		ilitek_set_tp_data_len(DATA_FORMAT_GESTURE_DEMO);
	} else if (strcmp(cmd, "gesturefailrsn") == 0) {
		if (data[1] == 0)
			gesture_fail_reason(DISABLE);
		else
			gesture_fail_reason(ENABLE);
		ipio_info("%s gesture fail reason\n", data[1] ? "ENABLE" : "DISABLE");
	} else if (strncmp(cmd, "icwholereset", strlen(cmd)) == 0) {
		ilitek_ice_mode_ctrl(ENABLE, OFF);
		ilitek_tddi_reset_ctrl(TP_IC_WHOLE_RST);
	} else if (strncmp(cmd, "iccodereset", strlen(cmd)) == 0) {
		ilitek_ice_mode_ctrl(ENABLE, OFF);
		ilitek_tddi_reset_ctrl(TP_IC_CODE_RST);
		ilitek_ice_mode_ctrl(DISABLE, OFF);
	} else if (strncmp(cmd, "getinfo", strlen(cmd)) == 0) {
		ilitek_ice_mode_ctrl(ENABLE, OFF);
		ilitek_tddi_ic_get_info();
		ilitek_ice_mode_ctrl(DISABLE, OFF);
		ilitek_tddi_ic_get_protocl_ver();
		ilitek_tddi_ic_get_fw_ver();
		ilitek_tddi_ic_get_core_ver();
		ilitek_tddi_ic_get_tp_info();
		ilitek_tddi_ic_get_panel_info();
		ipio_info("Driver version = %s\n", DRIVER_VERSION);
	} else if (strncmp(cmd, "enableicemode", strlen(cmd)) == 0) {
		if (data[1] == ON)
			ilitek_ice_mode_ctrl(ENABLE, ON);
		else
			ilitek_ice_mode_ctrl(ENABLE, OFF);
	} else if (strncmp(cmd, "wqctrl", strlen(cmd)) == 0) {
		idev->wq_ctrl = !idev->wq_ctrl;
		ipio_info("wq_ctrl flag= %d\n", idev->wq_ctrl);
	} else if (strncmp(cmd, "disableicemode", strlen(cmd)) == 0) {
		ilitek_ice_mode_ctrl(DISABLE, OFF);
	} else if (strncmp(cmd, "enablewqesd", strlen(cmd)) == 0) {
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	} else if (strncmp(cmd, "enablewqbat", strlen(cmd)) == 0) {
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);
	} else if (strncmp(cmd, "disablewqesd", strlen(cmd)) == 0) {
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	} else if (strncmp(cmd, "disablewqbat", strlen(cmd)) == 0) {
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);
	} else if (strncmp(cmd, "gesture", strlen(cmd)) == 0) {
		idev->gesture = !idev->gesture;
		ipio_info("gesture = %d\n", idev->gesture);
	} else if (strncmp(cmd, "esdgesture", strlen(cmd)) == 0) {
		if (ilitek_tddi_gesture_recovery() < 0) {
			ipio_err("gesture recovery failed\n");
			size = -1;
		}
	} else if (strncmp(cmd, "esdspi", strlen(cmd)) == 0) {
		ilitek_tddi_spi_recovery();
	} else if (strncmp(cmd, "sleepin", strlen(cmd)) == 0) {
		ilitek_tddi_ic_func_ctrl("sleep", SLEEP_IN);
	} else if (strncmp(cmd, "deepsleepin", strlen(cmd)) == 0) {
		ilitek_tddi_ic_func_ctrl("sleep", DEEP_SLEEP_IN);
	} else if (strncmp(cmd, "iceflag", strlen(cmd)) == 0) {
		if (data[1] == ENABLE)
			atomic_set(&idev->ice_stat, ENABLE);
		else
			atomic_set(&idev->ice_stat, DISABLE);
		ipio_info("ice mode flag = %d\n", atomic_read(&idev->ice_stat));
	} else if (strncmp(cmd, "gesturenormal", strlen(cmd)) == 0) {
		idev->gesture_mode = DATA_FORMAT_GESTURE_NORMAL;
		ipio_info("gesture mode = %d\n", idev->gesture_mode);
	} else if (strncmp(cmd, "gestureinfo", strlen(cmd)) == 0) {
		idev->gesture_mode = DATA_FORMAT_GESTURE_INFO;
		ipio_info("gesture mode = %d\n", idev->gesture_mode);
	} else if (strncmp(cmd, "netlink", strlen(cmd)) == 0) {
		idev->netlink = !idev->netlink;
		ipio_info("netlink flag= %d\n", idev->netlink);
	} else if (strncmp(cmd, "switchtestmode", strlen(cmd)) == 0) {
		ilitek_tddi_switch_tp_mode(P5_X_FW_TEST_MODE);
	} else if (strncmp(cmd, "switchdebugmode", strlen(cmd)) == 0) {
		ilitek_set_tp_data_len(DATA_FORMAT_DEBUG);
	} else if (strncmp(cmd, "switchdemomode", strlen(cmd)) == 0) {
		ilitek_set_tp_data_len(DATA_FORMAT_DEMO);
	} else if (strncmp(cmd, "dbgflag", strlen(cmd)) == 0) {
		idev->debug_node_open = !idev->debug_node_open;
		ipio_info("debug flag message = %d\n", idev->debug_node_open);
	} else if (strncmp(cmd, "iow", strlen(cmd)) == 0) {
		int w_len = 0;
		w_len = data[1];
		ipio_info("w_len = %d\n", w_len);

		for (i = 0; i < w_len; i++) {
			temp[i] = data[2 + i];
			ipio_info("write[%d] = %x\n", i, temp[i]);
		}

		idev->write(temp, w_len);
	} else if (strncmp(cmd, "ior", strlen(cmd)) == 0) {
		int r_len = 0;
		r_len = data[1];
		ipio_info("r_len = %d\n", r_len);
		idev->read(temp, r_len);
		for (i = 0; i < r_len; i++)
			ipio_info("read[%d] = %x\n", i, temp[i]);
	} else if (strncmp(cmd, "iowr", strlen(cmd)) == 0) {
		int delay = 0;
		int w_len = 0, r_len = 0;
		w_len = data[1];
		r_len = data[2];
		delay = data[3];
		ipio_info("w_len = %d, r_len = %d, delay = %d\n", w_len, r_len, delay);

		for (i = 0; i < w_len; i++) {
			temp[i] = data[4 + i];
			ipio_info("write[%d] = %x\n", i, temp[i]);
		}
		idev->write(temp, w_len);
		memset(temp, 0, sizeof(temp));
		mdelay(delay);
		idev->read(temp, r_len);

		for (i = 0; i < r_len; i++)
			ipio_info("read[%d] = %x\n", i, temp[i]);
	} else if (strncmp(cmd, "getddiregdata", strlen(cmd)) == 0) {
		ipio_info("Get ddi reg one page: page = %x, reg = %x\n", data[1], data[2]);
		ilitek_tddi_ic_get_ddi_reg_onepage(data[1], data[2]);
	} else if (strncmp(cmd, "setddiregdata", strlen(cmd)) == 0) {
		ipio_info("Set ddi reg one page: page = %x, reg = %x, data = %x\n", data[1], data[2], data[3]);
		ilitek_tddi_ic_set_ddi_reg_onepage(data[1], data[2], data[3]);
	} else if (strncmp(cmd, "dumpflashdata", strlen(cmd)) == 0) {
		ipio_info("Start = 0x%x, End = 0x%x, Dump Hex path = %s\n", data[1], data[2], DUMP_FLASH_PATH);
		ilitek_tddi_fw_dump_flash_data(data[1], data[2], false);
	} else if (strncmp(cmd, "dumpiramdata", strlen(cmd)) == 0) {
		ipio_info("Start = 0x%x, End = 0x%x, Dump IRAM path = %s\n", data[1], data[2], DUMP_IRAM_PATH);
		ilitek_fw_dump_iram_data(data[1], data[2], true);
	} else if (strncmp(cmd, "edge_palm_ctrl", strlen(cmd)) == 0) {
		ilitek_tddi_ic_func_ctrl("edge_palm", data[1]);
	} else if (strncmp(cmd, "uart_mode_ctrl", strlen(cmd)) == 0) {
		ilitek_tddi_fw_uart_ctrl(data[1]);
	} else if (strncmp(cmd, "flashesdgesture", strlen(cmd)) == 0) {
		ilitek_tddi_touch_esd_gesture_flash();
	} else if (strncmp(cmd, "spiw", strlen(cmd)) == 0) {
		int wlen;
		wlen = data[1];
		temp[0] = 0x82;
		for (i = 0; i < wlen; i++) {
			temp[i] = data[2 + i];
			ipio_info("write[%d] = %x\n", i, temp[i]);
		}
		idev->spi_write_then_read(idev->spi, temp, wlen, NULL, 0);
	} else if (strncmp(cmd, "spir", strlen(cmd)) == 0) {
		int rlen;
		u8 *rbuf = NULL;
		rlen = data[1];
		rbuf = kzalloc(rlen, GFP_KERNEL | GFP_DMA);
		if (ERR_ALLOC_MEM(rbuf)) {
			ipio_err("Failed to allocate dma_rxbuf, %ld\n", PTR_ERR(rbuf));
			kfree(rbuf);
			goto out;
		}
		temp[0] = 0x83;
		idev->spi_write_then_read(idev->spi, temp, 1, rbuf, rlen);
		for (i = 0; i < rlen; i++)
			ipio_info("read[%d] = %x\n", i, rbuf[i]);
		kfree(rbuf);
	} else if (strncmp(cmd, "spirw", strlen(cmd)) == 0) {
		int wlen, rlen;
		u8 *rbuf = NULL;
		wlen = data[1];
		rlen = data[2];
		for (i = 0; i < wlen; i++) {
			temp[i] = data[3 + i];
			ipio_info("write[%d] = %x\n", i, temp[i]);
		}
		if (rlen != 0) {
			rbuf = kzalloc(rlen, GFP_KERNEL | GFP_DMA);
			if (ERR_ALLOC_MEM(rbuf)) {
				ipio_err("Failed to allocate dma_rxbuf, %ld\n", PTR_ERR(rbuf));
				kfree(rbuf);
				goto out;
			}
		}
		idev->spi_write_then_read(idev->spi, temp, wlen, rbuf, rlen);
		if (rlen != 0) {
			for (i = 0; i < rlen; i++)
				ipio_info("read[%d] = %x\n", i, rbuf[i]);
		}
		kfree(rbuf);
	} else {
		ipio_err("Unknown command\n");
		size = -1;
	}
out:
	ipio_kfree((void **)&data);
	mutex_unlock(&idev->touch_mutex);
	return size;
}

#ifdef CONFIG_COMPAT
static long ilitek_node_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		ipio_err("There's no unlocked_ioctl defined in file\n");
		return -ENOTTY;
	}

	ipio_info("cmd = %d\n", _IOC_NR(cmd));

	switch (cmd) {
	case ILITEK_COMPAT_IOCTL_I2C_WRITE_DATA:
		ipio_info("compat_ioctl: convert i2c/spi write\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_I2C_WRITE_DATA, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_I2C_READ_DATA:
		ipio_info("compat_ioctl: convert i2c/spi read\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_I2C_READ_DATA, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_I2C_SET_WRITE_LENGTH:
		ipio_info("compat_ioctl: convert set write length\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_I2C_SET_WRITE_LENGTH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_I2C_SET_READ_LENGTH:
		ipio_info("compat_ioctl: convert set read length\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_I2C_SET_READ_LENGTH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_HW_RESET:
		ipio_info("compat_ioctl: convert hw reset\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_HW_RESET, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_POWER_SWITCH:
		ipio_info("compat_ioctl: convert power switch\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_POWER_SWITCH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_REPORT_SWITCH:
		ipio_info("compat_ioctl: convert report switch\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_REPORT_SWITCH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_IRQ_SWITCH:
		ipio_info("compat_ioctl: convert irq switch\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_IRQ_SWITCH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_DEBUG_LEVEL:
		ipio_info("compat_ioctl: convert debug level\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_DEBUG_LEVEL, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_FUNC_MODE:
		ipio_info("compat_ioctl: convert format mode\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_FUNC_MODE, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_FW_VER:
		ipio_info("compat_ioctl: convert set read length\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_FW_VER, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_PL_VER:
		ipio_info("compat_ioctl: convert fw version\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_PL_VER, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_CORE_VER:
		ipio_info("compat_ioctl: convert core version\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_CORE_VER, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_DRV_VER:
		ipio_info("compat_ioctl: convert driver version\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_DRV_VER, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_CHIP_ID:
		ipio_info("compat_ioctl: convert chip id\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_CHIP_ID, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_NETLINK_CTRL:
		ipio_info("compat_ioctl: convert netlink ctrl\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_NETLINK_CTRL, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_NETLINK_STATUS:
		ipio_info("compat_ioctl: convert netlink status\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_NETLINK_STATUS, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_MODE_CTRL:
		ipio_info("compat_ioctl: convert tp mode ctrl\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_MODE_CTRL, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_MODE_STATUS:
		ipio_info("compat_ioctl: convert tp mode status\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_MODE_STATUS, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_ICE_MODE_SWITCH:
		ipio_info("compat_ioctl: convert tp mode switch\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_ICE_MODE_SWITCH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_INTERFACE_TYPE:
		ipio_info("compat_ioctl: convert interface type\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_INTERFACE_TYPE, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_DUMP_FLASH:
		ipio_info("compat_ioctl: convert dump flash\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_DUMP_FLASH, (unsigned long)compat_ptr(arg));
		return ret;
	case ILITEK_COMPAT_IOCTL_TP_FW_UART_CTRL:
		ipio_info("compat_ioctl: convert fw uart\n");
		ret = filp->f_op->unlocked_ioctl(filp, ILITEK_IOCTL_TP_FW_UART_CTRL, (unsigned long)compat_ptr(arg));
		return ret;
	default:
		ipio_err("no ioctl cmd, return ilitek_node_ioctl\n");
		return -ENOIOCTLCMD;
	}
}
#endif

static long ilitek_node_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0, length = 0;
	u8 *szBuf = NULL, if_to_user = 0;
	static u16 i2c_rw_length;
	u32 id_to_user[3] = {0};
	bool esd_en = idev->wq_esd_ctrl, bat_en = idev->wq_bat_ctrl;

	if (_IOC_TYPE(cmd) != ILITEK_IOCTL_MAGIC) {
		ipio_err("The Magic number doesn't match\n");
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > ILITEK_IOCTL_MAXNR) {
		ipio_err("The number of ioctl doesn't match\n");
		return -ENOTTY;
	}

	ipio_debug("cmd = %d\n", _IOC_NR(cmd));

	mutex_lock(&idev->touch_mutex);

	szBuf = kcalloc(IOCTL_I2C_BUFF, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(szBuf)) {
		ipio_err("Failed to allocate mem\n");
		ret = -ENOMEM;
		goto out;
	}

	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, DISABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, DISABLE);

	switch (cmd) {
	case ILITEK_IOCTL_I2C_WRITE_DATA:
		ipio_debug("ioctl: write len = %d\n", i2c_rw_length);

		if (i2c_rw_length > IOCTL_I2C_BUFF) {
			ipio_err("ERROR! write len is largn than ioctl buf (%d, %ld)\n",
					i2c_rw_length, IOCTL_I2C_BUFF);
			ret = -ENOTTY;
			break;
		}

		if (copy_from_user(szBuf, (u8 *) arg, i2c_rw_length)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}

		ret = idev->write(&szBuf[0], i2c_rw_length);
		if (ret < 0)
			ipio_err("Failed to write data\n");
		break;
	case ILITEK_IOCTL_I2C_READ_DATA:
		ipio_debug("ioctl: read len = %d\n", i2c_rw_length);

		if (i2c_rw_length > IOCTL_I2C_BUFF) {
			ipio_err("ERROR! read len is largn than ioctl buf (%d, %ld)\n",
					i2c_rw_length, IOCTL_I2C_BUFF);
			ret = -ENOTTY;
			break;
		}

		ret = idev->read(szBuf, i2c_rw_length);
		if (ret < 0) {
			ipio_err("Failed to read data\n");
			break;
		}

		if (copy_to_user((u8 *) arg, szBuf, i2c_rw_length)) {
			ipio_err("Failed to copy data to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_I2C_SET_WRITE_LENGTH:
	case ILITEK_IOCTL_I2C_SET_READ_LENGTH:
		i2c_rw_length = arg;
		break;
	case ILITEK_IOCTL_TP_HW_RESET:
		ipio_debug("ioctl: hw reset\n");
		ilitek_tddi_reset_ctrl(idev->reset);
		break;
	case ILITEK_IOCTL_TP_POWER_SWITCH:
		ipio_debug("Not implemented yet\n");
		break;
	case ILITEK_IOCTL_TP_REPORT_SWITCH:
		if (copy_from_user(szBuf, (u8 *) arg, 1)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}

		ipio_debug("ioctl: report switch = %d\n", szBuf[0]);
		if (szBuf[0]) {
			idev->report = ENABLE;
			ipio_debug("report is enabled\n");
		} else {
			idev->report = DISABLE;
			ipio_debug("report is disabled\n");
		}
		break;
	case ILITEK_IOCTL_TP_IRQ_SWITCH:
		if (copy_from_user(szBuf, (u8 *) arg, 1)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}
		ipio_debug("ioctl: irq switch = %d\n", szBuf[0]);
		if (szBuf[0])
			ilitek_plat_irq_enable();
		else
			ilitek_plat_irq_disable();
		break;
	case ILITEK_IOCTL_TP_DEBUG_LEVEL:
		if (copy_from_user(szBuf, (u8 *) arg, sizeof(u32))) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}

		ipio_debug_level = !ipio_debug_level;
		ipio_info("ipio_debug_level = %d", ipio_debug_level);
		break;
	case ILITEK_IOCTL_TP_FUNC_MODE:
		if (copy_from_user(szBuf, (u8 *) arg, 3)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}

		ipio_debug("ioctl: set func mode = %x,%x,%x\n", szBuf[0], szBuf[1], szBuf[2]);
		idev->write(&szBuf[0], 3);
		break;
	case ILITEK_IOCTL_TP_FW_VER:
		ipio_debug("ioctl: get fw version\n");
		ret = ilitek_tddi_ic_get_fw_ver();
		if (ret < 0) {
			ipio_err("Failed to get firmware version\n");
			break;
		}

		szBuf[3] = idev->chip->fw_ver & 0xFF;
		szBuf[2] = (idev->chip->fw_ver >> 8) & 0xFF;
		szBuf[1] = (idev->chip->fw_ver >> 16) & 0xFF;
		szBuf[0] = idev->chip->fw_ver >> 24;
		ipio_debug("Firmware version = %d.%d.%d.%d\n", szBuf[0], szBuf[1], szBuf[2], szBuf[3]);

		if (copy_to_user((u8 *) arg, szBuf, 4)) {
			ipio_err("Failed to copy data to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_PL_VER:
		ipio_debug("ioctl: get protocl version\n");
		ret = ilitek_tddi_ic_get_protocl_ver();
		if (ret < 0) {
			ipio_err("Failed to get protocol version\n");
			break;
		}

		szBuf[2] = idev->protocol->ver & 0xFF;
		szBuf[1] = (idev->protocol->ver >> 8) & 0xFF;
		szBuf[0] = idev->protocol->ver >> 16;
		ipio_debug("Protocol version = %d.%d.%d\n", szBuf[0], szBuf[1], szBuf[2]);

		if (copy_to_user((u8 *) arg, szBuf, 3)) {
			ipio_err("Failed to copy data to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_CORE_VER:
		ipio_debug("ioctl: get core version\n");
		ret = ilitek_tddi_ic_get_core_ver();
		if (ret < 0) {
			ipio_err("Failed to get core version\n");
			break;
		}

		szBuf[3] = idev->chip->core_ver & 0xFF;
		szBuf[2] = (idev->chip->core_ver >> 8) & 0xFF;
		szBuf[1] = (idev->chip->core_ver >> 16) & 0xFF;
		szBuf[0] = idev->chip->core_ver >> 24;
		ipio_debug("Core version = %d.%d.%d.%d\n", szBuf[0], szBuf[1], szBuf[2], szBuf[3]);

		if (copy_to_user((u8 *) arg, szBuf, 4)) {
			ipio_err("Failed to copy data to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_DRV_VER:
		ipio_debug("ioctl: get driver version\n");
		length = snprintf(szBuf, USER_STR_BUFF * sizeof(unsigned char), "%s", DRIVER_VERSION);

		if (copy_to_user((u8 *) arg, szBuf, length)) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_CHIP_ID:
		ipio_debug("ioctl: get chip id\n");
		ilitek_ice_mode_ctrl(ENABLE, OFF);
		ret = ilitek_tddi_ic_get_info();
		if (ret < 0) {
			ipio_err("Failed to get chip id\n");
			break;
		}

		id_to_user[0] = idev->chip->pid;
		id_to_user[1] = idev->chip->otp_id;
		id_to_user[2] = idev->chip->ana_id;

		if (copy_to_user((u32 *) arg, id_to_user, sizeof(id_to_user))) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}

		ilitek_ice_mode_ctrl(DISABLE, OFF);
		break;
	case ILITEK_IOCTL_TP_NETLINK_CTRL:
		if (copy_from_user(szBuf, (u8 *) arg, 1)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}
		ipio_debug("ioctl: netlink ctrl = %d\n", szBuf[0]);
		if (szBuf[0]) {
			idev->netlink = ENABLE;
			ipio_debug("ioctl: Netlink is enabled\n");
		} else {
			idev->netlink = DISABLE;
			ipio_debug("ioctl: Netlink is disabled\n");
		}
		break;
	case ILITEK_IOCTL_TP_NETLINK_STATUS:
		ipio_debug("ioctl: get netlink stat = %d\n", idev->netlink);
		if (copy_to_user((int *)arg, &idev->netlink, sizeof(int))) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_MODE_CTRL:
		if (copy_from_user(szBuf, (u8 *) arg, 4)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}
		ipio_debug("ioctl: switch fw format = %d\n", szBuf[0]);
		if (szBuf[0] == 0) {
			if (idev->actual_tp_mode == P5_X_FW_AP_MODE) {
				if (ilitek_set_tp_data_len(DATA_FORMAT_DEMO) < 0) {
					ipio_err("Failed to set tp data length\n");
					ret = -ENOTTY;
				}
			} else {
				if (ilitek_tddi_switch_tp_mode(P5_X_FW_AP_MODE) < 0) {
					ipio_err("Failed to set tp data length\n");
					ret = -ENOTTY;
				}
			}
		} else if (szBuf[0] == 1) {
			if (ilitek_tddi_switch_tp_mode(P5_X_FW_TEST_MODE) < 0) {
				ipio_err("Failed to set tp data length\n");
				ret = -ENOTTY;
			}
		} else if (szBuf[0] == 2) {
			if (idev->actual_tp_mode != P5_X_FW_AP_MODE) {
				if (ilitek_tddi_switch_tp_mode(P5_X_FW_AP_MODE) < 0) {
					ipio_err("Failed to set tp data length\n");
					ret = -ENOTTY;
					break;
				}
			}
			if (ilitek_set_tp_data_len(DATA_FORMAT_DEBUG) < 0) {
				ipio_err("Failed to set tp data length\n");
				ret = -ENOTTY;
			}
		} else {
			ipio_err("Unknown TP mode ctrl\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_MODE_STATUS:
		ipio_debug("ioctl: current firmware mode = %d", idev->actual_tp_mode);
		if (copy_to_user((int *)arg, &idev->actual_tp_mode, sizeof(int))) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}
		break;
	/* It works for host downloado only */
	case ILITEK_IOCTL_ICE_MODE_SWITCH:
		if (copy_from_user(szBuf, (u8 *) arg, 1)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}
		ipio_debug("ioctl: switch ice mode = %d", szBuf[0]);
		if (szBuf[0]) {
			atomic_set(&idev->ice_stat, ENABLE);
			ipio_debug("ioctl: set ice mode enabled\n");
		} else {
			atomic_set(&idev->ice_stat, DISABLE);
			ipio_debug("ioctl: set ice mode disabled\n");
		}
		break;
	case ILITEK_IOCTL_TP_INTERFACE_TYPE:
		if_to_user = idev->hwif->bus_type;
		if (copy_to_user((u8 *) arg, &if_to_user, sizeof(if_to_user))) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}
		break;
	case ILITEK_IOCTL_TP_DUMP_FLASH:
		ipio_debug("ioctl: dump flash data\n");
		ret = ilitek_tddi_fw_dump_flash_data(0, 0, true);
		if (ret < 0) {
			ipio_err("ioctl: Failed to dump flash data\n");
		}
		break;
	case ILITEK_IOCTL_TP_FW_UART_CTRL:
		if (copy_from_user(szBuf, (u8 *) arg, 1)) {
			ipio_err("Failed to copy data from user space\n");
			ret = -ENOTTY;
			break;
		}
		ipio_debug("ioctl: fw UART  = %d\n", szBuf[0]);

		ilitek_tddi_fw_uart_ctrl(szBuf[0]);

		if_to_user = idev->fw_uart_en;

		if (copy_to_user((u8 *) arg, &if_to_user, sizeof(if_to_user))) {
			ipio_err("Failed to copy driver ver to user space\n");
			ret = -ENOTTY;
		}
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	ipio_kfree((void **)&szBuf);

out:
	if (esd_en)
		ilitek_tddi_wq_ctrl(WQ_ESD, ENABLE);
	if (bat_en)
		ilitek_tddi_wq_ctrl(WQ_BAT, ENABLE);

	mutex_unlock(&idev->touch_mutex);
	return ret;
}

struct proc_dir_entry *proc_dir_ilitek;

typedef struct {
	char *name;
	struct proc_dir_entry *node;
	struct file_operations *fops;
	bool isCreated;
} proc_node_t;

struct file_operations proc_mp_lcm_on_test_fops = {
	.read = ilitek_node_mp_lcm_on_test_read,
};

struct file_operations proc_mp_lcm_off_test_fops = {
	.read = ilitek_node_mp_lcm_off_test_read,
};

struct file_operations proc_debug_message_fops = {
	.read = ilitek_proc_debug_message_read,
};

struct file_operations proc_debug_message_switch_fops = {
	.read = ilitek_proc_debug_switch_read,
};

struct file_operations proc_ioctl_fops = {
	.unlocked_ioctl = ilitek_node_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ilitek_node_compat_ioctl,
#endif
	.write = ilitek_node_ioctl_write,
};

struct file_operations proc_fw_upgrade_fops = {
	.read = ilitek_node_fw_upgrade_read,
};

struct file_operations proc_fw_process_fops = {
	.read = ilitek_proc_fw_process_read,
};

struct file_operations proc_get_delta_data_fops = {
	.read = ilitek_proc_get_delta_data_read,
};

struct file_operations proc_get_raw_data_fops = {
	.read = ilitek_proc_fw_get_raw_data_read,
};

struct file_operations proc_rw_tp_reg_fops = {
	.read = ilitek_proc_rw_tp_reg_read,
	.write = ilitek_proc_rw_tp_reg_write,
};

struct file_operations proc_fw_pc_counter_fops = {
	.read = ilitek_proc_fw_pc_counter_read,
};

struct file_operations proc_get_debug_mode_data_fops = {
	.read = ilitek_proc_get_debug_mode_data_read,
	.write = ilitek_proc_get_debug_mode_data_write,
};

struct file_operations proc_debug_level_fops = {
	.read = ilitek_proc_debug_level_read,
};

proc_node_t proc_table[] = {
	{"ioctl", NULL, &proc_ioctl_fops, false},
	{"fw_process", NULL, &proc_fw_process_fops, false},
	{"fw_upgrade", NULL, &proc_fw_upgrade_fops, false},
	{"debug_level", NULL, &proc_debug_level_fops, false},
	{"mp_lcm_on_test", NULL, &proc_mp_lcm_on_test_fops, false},
	{"mp_lcm_off_test", NULL, &proc_mp_lcm_off_test_fops, false},
	{"debug_message", NULL, &proc_debug_message_fops, false},
	{"debug_message_switch", NULL, &proc_debug_message_switch_fops, false},
	{"fw_pc_counter", NULL, &proc_fw_pc_counter_fops, false},
	{"show_delta_data", NULL, &proc_get_delta_data_fops, false},
	{"show_raw_data", NULL, &proc_get_raw_data_fops, false},
	{"get_debug_mode_data", NULL, &proc_get_debug_mode_data_fops, false},
	{"rw_tp_reg", NULL, &proc_rw_tp_reg_fops, false},
};

#define NETLINK_USER 21
struct sock *netlink_skb;
struct nlmsghdr *netlink_head;
struct sk_buff *skb_out;
int netlink_pid;

void netlink_reply_msg(void *raw, int size)
{
	int ret;
	int msg_size = size;
	u8 *data = (u8 *) raw;

	ipio_info("The size of data being sent to user = %d\n", msg_size);
	ipio_info("pid = %d\n", netlink_pid);
	ipio_info("Netlink is enable = %d\n", idev->netlink);

	if (idev->netlink) {
		skb_out = nlmsg_new(msg_size, 0);

		if (!skb_out) {
			ipio_err("Failed to allocate new skb\n");
			return;
		}

		netlink_head = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
		NETLINK_CB(skb_out).dst_group = 0;	/* not in mcast group */

		/* strncpy(NLMSG_DATA(netlink_head), data, msg_size); */
		ipio_memcpy(nlmsg_data(netlink_head), data, msg_size, size);

		ret = nlmsg_unicast(netlink_skb, skb_out, netlink_pid);
		if (ret < 0)
			ipio_err("Failed to send data back to user\n");
	}
}

static void netlink_recv_msg(struct sk_buff *skb)
{
	netlink_pid = 0;

	ipio_info("Netlink = %d\n", idev->netlink);

	netlink_head = (struct nlmsghdr *)skb->data;

	ipio_info("Received a request from client: %s, %d\n",
		(char *)NLMSG_DATA(netlink_head), (int)strlen((char *)NLMSG_DATA(netlink_head)));

	/* pid of sending process */
	netlink_pid = netlink_head->nlmsg_pid;

	ipio_info("the pid of sending process = %d\n", netlink_pid);

	/* TODO: may do something if there's not receiving msg from user. */
	if (netlink_pid != 0) {
		ipio_err("The channel of Netlink has been established successfully !\n");
		idev->netlink = ENABLE;
	} else {
		ipio_err("Failed to establish the channel between kernel and user space\n");
		idev->netlink = DISABLE;
	}
}

static int netlink_init(void)
{
	int ret = 0;

#if KERNEL_VERSION(3, 4, 0) > LINUX_VERSION_CODE
	netlink_skb = netlink_kernel_create(&init_net, NETLINK_USER, netlink_recv_msg, NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = {
		.input = netlink_recv_msg,
	};

	netlink_skb = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
#endif

	ipio_info("Initialise Netlink and create its socket\n");

	if (!netlink_skb) {
		ipio_err("Failed to create nelink socket\n");
		ret = -EFAULT;
	}
	return ret;
}

void ilitek_tddi_node_init(void)
{
	int i = 0, ret = 0, size;

	proc_dir_ilitek = proc_mkdir("ilitek", NULL);

	size = ARRAY_SIZE(proc_table);
	for (; i < size; i++) {
		proc_table[i].node = proc_create(proc_table[i].name, 0644, proc_dir_ilitek, proc_table[i].fops);

		if (proc_table[i].node == NULL) {
			proc_table[i].isCreated = false;
			ipio_err("Failed to create %s under /proc\n", proc_table[i].name);
			ret = -ENODEV;
		} else {
			proc_table[i].isCreated = true;
			ipio_info("Succeed to create %s under /proc\n", proc_table[i].name);
		}
	}
	netlink_init();
}
