/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
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
 *
 */

#include "common.h"
#include "platform.h"
#include "core/config.h"
#include "core/firmware.h"
#include "core/finger_report.h"
#include "core/flash.h"
#include "core/i2c.h"
#include "core/protocol.h"
#include "core/mp_test.h"
#include "core/parser.h"
#include "core/gesture.h"

#define USER_STR_BUFF	PAGE_SIZE
#define IOCTL_I2C_BUFF	PAGE_SIZE
#define ILITEK_IOCTL_MAGIC	100
#define ILITEK_IOCTL_MAXNR	21

#define ILITEK_IOCTL_I2C_WRITE_DATA			_IOWR(ILITEK_IOCTL_MAGIC, 0, uint8_t*)
#define ILITEK_IOCTL_I2C_SET_WRITE_LENGTH	_IOWR(ILITEK_IOCTL_MAGIC, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA			_IOWR(ILITEK_IOCTL_MAGIC, 2, uint8_t*)
#define ILITEK_IOCTL_I2C_SET_READ_LENGTH	_IOWR(ILITEK_IOCTL_MAGIC, 3, int)

#define ILITEK_IOCTL_TP_HW_RESET			_IOWR(ILITEK_IOCTL_MAGIC, 4, int)
#define ILITEK_IOCTL_TP_POWER_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 5, int)
#define ILITEK_IOCTL_TP_REPORT_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 6, int)
#define ILITEK_IOCTL_TP_IRQ_SWITCH			_IOWR(ILITEK_IOCTL_MAGIC, 7, int)

#define ILITEK_IOCTL_TP_DEBUG_LEVEL			_IOWR(ILITEK_IOCTL_MAGIC, 8, int)
#define ILITEK_IOCTL_TP_FUNC_MODE			_IOWR(ILITEK_IOCTL_MAGIC, 9, int)

#define ILITEK_IOCTL_TP_FW_VER				_IOWR(ILITEK_IOCTL_MAGIC, 10, uint8_t*)
#define ILITEK_IOCTL_TP_PL_VER				_IOWR(ILITEK_IOCTL_MAGIC, 11, uint8_t*)
#define ILITEK_IOCTL_TP_CORE_VER			_IOWR(ILITEK_IOCTL_MAGIC, 12, uint8_t*)
#define ILITEK_IOCTL_TP_DRV_VER				_IOWR(ILITEK_IOCTL_MAGIC, 13, uint8_t*)
#define ILITEK_IOCTL_TP_CHIP_ID				_IOWR(ILITEK_IOCTL_MAGIC, 14, uint32_t*)

#define ILITEK_IOCTL_TP_NETLINK_CTRL		_IOWR(ILITEK_IOCTL_MAGIC, 15, int*)
#define ILITEK_IOCTL_TP_NETLINK_STATUS		_IOWR(ILITEK_IOCTL_MAGIC, 16, int*)

#define ILITEK_IOCTL_TP_MODE_CTRL			_IOWR(ILITEK_IOCTL_MAGIC, 17, uint8_t*)
#define ILITEK_IOCTL_TP_MODE_STATUS			_IOWR(ILITEK_IOCTL_MAGIC, 18, int*)
#define ILITEK_IOCTL_ICE_MODE_SWITCH		_IOWR(ILITEK_IOCTL_MAGIC, 19, int)
#define ILITEK_IOCTL_TP_INTERFACE_TYPE		_IOWR(ILITEK_IOCTL_MAGIC, 20, uint8_t*)
#define ILITEK_IOCTL_TP_DUMP_FLASH			_IOWR(ILITEK_IOCTL_MAGIC, 21, int)

unsigned char g_user_buf[USER_STR_BUFF] = { 0 };
unsigned char fw_name_buf[USER_STR_BUFF] = { 0 };
#define DEBUG_DATA_FILE_SIZE	(10 * 1024)
#define DEBUG_DATA_FILE_PATH	"/sdcard/ILITEK_log.csv"
#define REGISTER_READ	0
#define REGISTER_WRITE	1
uint32_t temp[5] = {0};
extern char mode_chose;
static struct class *touchscreen_class;
static struct device *touchscreen_class_dev;
extern bool use_g_user_buf;
bool is_force_upgrade =false;
struct file_buffer {
	char *ptr;
	char file_name[128];
	int32_t file_len;
	int32_t file_max_zise;
};

int katoi(char *string)
{
	int result = 0;
	unsigned int digit;
	int sign;

	if (*string == '-') {
		sign = 1;
		string += 1;
	} else {
		sign = 0;
		if (*string == '+') {
			string += 1;
		}
	}

	for (;; string += 1) {
		digit = *string - '0';
		if (digit > 9)
			break;
		result = (10 * result) + digit;
	}

	if (sign) {
		return -result;
	}
	return result;
}
EXPORT_SYMBOL(katoi);

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
EXPORT_SYMBOL(str2hex);

static int dev_mkdir(char *name, umode_t mode)
{
    struct dentry *dentry;
    struct path path;
    int err;

    dentry = kern_path_create(AT_FDCWD, name, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

    err = vfs_mkdir(path.dentry->d_inode, dentry, mode);
    done_path_create(&path, dentry);
    return err;
}

static ssize_t ilitek_proc_get_delta_data_read(struct file *pFile, char __user *buf, size_t nCount, loff_t *pos)
{
	int16_t *delta = NULL;
	int row = 0, col = 0,  index = 0;
	int ret, i, x, y;
	int read_length = 0;
	uint8_t cmd[2] = {0};
	uint8_t *data = NULL;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	mutex_lock(&ipd->touch_mutex);

	row = core_config->tp_info->nYChannelNum;
	col = core_config->tp_info->nXChannelNum;
	read_length = 4 + 2 * row * col + 1 ;

	ipio_info("read length = %d\n", read_length);

	data = kcalloc(read_length + 1, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(data)) {
		ipio_err("Failed to allocate data mem\n");
		return 0;
	}

	delta = kcalloc(P5_0_DEBUG_MODE_PACKET_LENGTH, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(delta)) {
		ipio_err("Failed to allocate delta mem\n");
		return 0;
	}

	cmd[0] = 0xB7;
	cmd[1] = 0x1; //get delta

	ret = core_write(core_config->slave_i2c_addr, &cmd[0], sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x1 command, %d\n", ret);
		goto out;
	}

	msleep(20);

	/* read debug packet header */
	ret = core_read(core_config->slave_i2c_addr, data, read_length);

	cmd[1] = 0x03; //switch to normal mode
	ret = core_write(core_config->slave_i2c_addr, &cmd[0], sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x3 command, %d\n", ret);
		goto out;
	}

	for (i = 4, index = 0; index < row * col * 2; i += 2, index++) {
		delta[index] = (data[i] << 8) + data[i + 1];
	}

	nCount = snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "======== Deltadata ========\n");
	ipio_info("======== Deltadata ========\n");

	nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount,
		"Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2]<<8) | data[3]);
	ipio_info("Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2]<<8) | data[3]);

	// print delta data
	for (y = 0; y < row; y++) {
		nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "[%2d] ", (y+1));
		ipio_info("[%2d] ", (y+1));

		for (x = 0; x < col; x++) {
			int shift = y * col + x;
			nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "%5d", delta[shift]);
			printk(KERN_CONT "%5d", delta[shift]);
		}
		nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "\n");
		printk(KERN_CONT "\n");
	}

	ret = copy_to_user(buf, g_user_buf, nCount);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space");
	}

	*pos += nCount;

out:
	mutex_unlock(&ipd->touch_mutex);
	ipio_kfree((void **)&data);
	ipio_kfree((void **)&delta);
	return nCount;
}

static ssize_t ilitek_proc_fw_get_raw_data_read(struct file *pFile, char __user *buf, size_t nCount, loff_t *pos)
{
	int16_t *rawdata = NULL;
	int row = 0, col = 0,  index = 0;
	int ret, i, x, y;
	int read_length = 0;
	uint8_t cmd[2] = {0};
	uint8_t *data = NULL;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	mutex_lock(&ipd->touch_mutex);

	row = core_config->tp_info->nYChannelNum;
	col = core_config->tp_info->nXChannelNum;
	read_length = 4 + 2 * row * col + 1 ;

	ipio_info("read length = %d\n", read_length);

	data = kcalloc(read_length + 1, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(data)) {
			ipio_err("Failed to allocate data mem\n");
			return 0;
	}

	rawdata = kcalloc(P5_0_DEBUG_MODE_PACKET_LENGTH, sizeof(int32_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(rawdata)) {
			ipio_err("Failed to allocate rawdata mem\n");
			return 0;
	}

	cmd[0] = 0xB7;
	cmd[1] = 0x2; //get rawdata

	ret = core_write(core_config->slave_i2c_addr, &cmd[0], sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x2 command, %d\n", ret);
		goto out;
	}

	msleep(20);

	/* read debug packet header */
	ret = core_read(core_config->slave_i2c_addr, data, read_length);

	cmd[1] = 0x03; //switch to normal mode
	ret = core_write(core_config->slave_i2c_addr, &cmd[0], sizeof(cmd));
	if (ret < 0) {
		ipio_err("Failed to write 0xB7,0x3 command, %d\n", ret);
		goto out;
	}

	for (i = 4, index = 0; index < row * col * 2; i += 2, index++) {
		rawdata[index] = (data[i] << 8) + data[i + 1];
	}

	nCount = snprintf(g_user_buf, PAGE_SIZE, "======== RawData ========\n");
	ipio_info("======== RawData ========\n");

	nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount,
			"Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2]<<8) | data[3]);
	ipio_info("Header 0x%x ,Type %d, Length %d\n", data[0], data[1], (data[2]<<8) | data[3]);

	// print raw data
	for (y = 0; y < row; y++) {
		nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "[%2d] ", (y+1));
		ipio_info("[%2d] ", (y+1));

		for (x = 0; x < col; x++) {
			int shift = y * col + x;
			nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "%5d", rawdata[shift]);
			printk(KERN_CONT "%5d", rawdata[shift]);
		}
		nCount += snprintf(g_user_buf + nCount, PAGE_SIZE - nCount, "\n");
		printk(KERN_CONT "\n");
	}

	ret = copy_to_user(buf, g_user_buf, nCount);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space");
	}

	*pos += nCount;

out:
	mutex_unlock(&ipd->touch_mutex);
	ipio_kfree((void **)&data);
	ipio_kfree((void **)&rawdata);
	return nCount;
}

static ssize_t ilitek_proc_fw_pc_counter_read(struct file *pFile, char __user *buf, size_t nCount, loff_t *pos)
{
	uint32_t pc;

	if (*pos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	mutex_lock(&ipd->plat_mutex);

	pc = core_config_read_pc_counter();

	mutex_unlock(&ipd->plat_mutex);

	nCount = snprintf(g_user_buf, PAGE_SIZE, "pc counter = 0x%x\n", pc);

	pc = copy_to_user(buf, g_user_buf, nCount);
	if (pc < 0) {
		ipio_err("Failed to copy data to user space");
	}

	*pos += nCount;

	return nCount;
}

static ssize_t ilitek_proc_debug_switch_read(struct file *pFile, char __user *buff, size_t nCount, loff_t *pPos)
{
	int ret = 0;

	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ipd->debug_node_open = !ipd->debug_node_open;

	ipio_info(" %s debug_flag message = %x\n", ipd->debug_node_open ? "Enabled" : "Disabled", ipd->debug_node_open);

	nCount = sprintf(g_user_buf, "ipd->debug_node_open : %s\n", ipd->debug_node_open ? "Enabled" : "Disabled");

	*pPos += nCount;

	ret = copy_to_user(buff, g_user_buf, nCount);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space");
	}

	return nCount;
}

static ssize_t ilitek_proc_debug_message_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	unsigned char buffer[512] = { 0 };

	/* check the buffer size whether it exceeds the local buffer size or not */
	if (size > 512) {
		ipio_err("buffer exceed 512 bytes\n");
		size = 512;
	}

	ret = copy_from_user(buffer, buff, size - 1);
	if (ret < 0) {
		ipio_err("copy data from user space, failed");
		return -1;
	}

	if (strcmp(buffer, "dbg_flag") == 0) {
		ipd->debug_node_open = !ipd->debug_node_open;
		ipio_info(" %s debug_flag message(%X).\n", ipd->debug_node_open ? "Enabled" : "Disabled",
			 ipd->debug_node_open);
	}
	return size;
}

static ssize_t ilitek_proc_debug_message_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	unsigned long p = *pPos;
	unsigned int count = size;
	int i = 0;
	int send_data_len = 0;
	size_t ret = 0;
	int data_count = 0;
	int one_data_bytes = 0;
	int need_read_data_len = 0;
	int type = 0;
	unsigned char *tmpbuf = NULL;
	unsigned char tmpbufback[128] = { 0 };

	mutex_lock(&ipd->ilitek_debug_read_mutex);

	while (ipd->debug_data_frame <= 0) {
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}
		wait_event_interruptible(ipd->inq, ipd->debug_data_frame > 0);
	}

	mutex_lock(&ipd->ilitek_debug_mutex);

	tmpbuf = vmalloc(4096);	/* buf size if even */
	if (ERR_ALLOC_MEM(tmpbuf)) {
		ipio_err("buffer vmalloc error\n");
		send_data_len += sprintf(tmpbufback + send_data_len, "buffer vmalloc error\n");
		ret = copy_to_user(buff, tmpbufback, send_data_len); /*ipd->debug_buf[0] */
	} else {
		if (ipd->debug_data_frame > 0) {
			if (ipd->debug_buf[0][0] == 0x5A) {
				need_read_data_len = 43;
			} else if (ipd->debug_buf[0][0] == 0x7A) {
				type = ipd->debug_buf[0][3] & 0x0F;

				data_count = ipd->debug_buf[0][1] * ipd->debug_buf[0][2];

				if (type == 0 || type == 1 || type == 6) {
					one_data_bytes = 1;
				} else if (type == 2 || type == 3) {
					one_data_bytes = 2;
				} else if (type == 4 || type == 5) {
					one_data_bytes = 4;
				}
				need_read_data_len = data_count * one_data_bytes + 1 + 5;
			}

			send_data_len = 0;	/* ipd->debug_buf[0][1] - 2; */
			need_read_data_len = 2040;
			if (need_read_data_len <= 0) {
				ipio_err("parse data err data len = %d\n", need_read_data_len);
				send_data_len +=
				    sprintf(tmpbuf + send_data_len, "parse data err data len = %d\n",
					    need_read_data_len);
			} else {
				for (i = 0; i < need_read_data_len; i++) {
					send_data_len += sprintf(tmpbuf + send_data_len, "%02X", ipd->debug_buf[0][i]);
					if (send_data_len >= 4096) {
						ipio_err("send_data_len = %d set 4096 i = %d\n", send_data_len, i);
						send_data_len = 4096;
						break;
					}
				}
			}
			send_data_len += sprintf(tmpbuf + send_data_len, "\n\n");

			if (p == 5 || size == 4096 || size == 2048) {
				ipd->debug_data_frame--;
				if (ipd->debug_data_frame < 0) {
					ipd->debug_data_frame = 0;
				}

				for (i = 1; i <= ipd->debug_data_frame; i++) {
					memcpy(ipd->debug_buf[i - 1], ipd->debug_buf[i], 2048);
				}
			}
		} else {
			ipio_err("no data send\n");
			send_data_len += sprintf(tmpbuf + send_data_len, "no data send\n");
		}

		/* Preparing to send data to user */
		if (size == 4096)
			ret = copy_to_user(buff, tmpbuf, send_data_len);
		else
			ret = copy_to_user(buff, tmpbuf + p, send_data_len - p);

		if (ret) {
			ipio_err("copy_to_user err\n");
			ret = -EFAULT;
		} else {
			*pPos += count;
			ret = count;
			ipio_debug(DEBUG_FINGER_REPORT, "Read %d bytes(s) from %ld\n", count, p);
		}
	}
	/* ipio_err("send_data_len = %d\n", send_data_len); */
	if (send_data_len <= 0 || send_data_len > 4096) {
		ipio_err("send_data_len = %d set 2048\n", send_data_len);
		send_data_len = 4096;
	}
	if (tmpbuf != NULL) {
		vfree(tmpbuf);
		tmpbuf = NULL;
	}

	mutex_unlock(&ipd->ilitek_debug_mutex);
	mutex_unlock(&ipd->ilitek_debug_read_mutex);
	return send_data_len;
}

static ssize_t ilitek_proc_mp_lcm_on_test_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	char apk[100] = {0};
	int ret;
	bool lcm_on = true;

	if (*pPos != 0)
		return 0;

	if (core_firmware->isUpgrading) {
		ipio_err("FW upgrading, please wait to complete\n");
		return 0;
	}

	/* Create the directory for mp_test result */
	ret = dev_mkdir(CSV_LCM_ON_PATH, S_IRUGO | S_IWUSR);
	if (ret != 0)
		ipio_err("Failed to create directory for mp_test\n");

	/* Running MP Test */
	ret = core_mp_start_test(lcm_on);
	if (ret < 0)
		goto out;

	/* copy MP result to user */
	core_mp_copy_reseult(apk, ARRAY_SIZE(apk));
	ret = copy_to_user((char *)buff, apk, sizeof(apk));
	if (ret < 0)
		ipio_err("Failed to copy data to user space\n");

out:
	core_mp_test_free();
	return ret;
}

static ssize_t ilitek_proc_mp_lcm_off_test_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	char apk[100] = {0};
	int ret;
	bool lcm_off = false;

	if (*pPos != 0)
		return 0;

	if (core_firmware->isUpgrading) {
		ipio_err("FW upgrading, please wait to complete\n");
		return 0;
	}

	/* Create the directory for mp_test result */
	ret = dev_mkdir(CSV_LCM_OFF_PATH, S_IRUGO | S_IWUSR);
	if (ret != 0)
		ipio_err("Failed to create directory for mp_test\n");

	/* Running MP Test */
	ret = core_mp_start_test(lcm_off);
	if (ret < 0)
		goto out;

	/* copy MP result to user */
	core_mp_copy_reseult(apk, ARRAY_SIZE(apk));
	ret = copy_to_user((char *)buff, apk, sizeof(apk));
	if (ret < 0)
		ipio_err("Failed to copy data to user space\n");

out:
	core_mp_test_free();
	return ret;
}
static ssize_t ilitek_proc_debug_level_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	uint32_t len = 0;

	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ipio_info("Current DEBUG Level = %d\n", ipio_debug_level);
	ipio_info("You can set one of levels for debug as below:\n");
	ipio_info("DEBUG_NONE = %d\n", DEBUG_NONE);
	ipio_info("DEBUG_IRQ = %d\n", DEBUG_IRQ);
	ipio_info("DEBUG_FINGER_REPORT = %d\n", DEBUG_FINGER_REPORT);
	ipio_info("DEBUG_FIRMWARE = %d\n", DEBUG_FIRMWARE);
	ipio_info("DEBUG_CONFIG = %d\n", DEBUG_CONFIG);
	ipio_info("DEBUG_I2C = %d\n", DEBUG_I2C);
	ipio_info("DEBUG_BATTERY = %d\n", DEBUG_BATTERY);
	ipio_info("DEBUG_MP_TEST = %d\n", DEBUG_MP_TEST);
	ipio_info("DEBUG_IOCTL = %d\n", DEBUG_IOCTL);
	ipio_info("DEBUG_NETLINK = %d\n", DEBUG_NETLINK);
	ipio_info("DEBUG_PARSER = %d\n", DEBUG_PARSER);
	ipio_info("DEBUG_GESTURE = %d\n", DEBUG_GESTURE);
	ipio_info("DEBUG_SPI = %d\n", DEBUG_SPI);
	ipio_info("DEBUG_ALL = %d\n", DEBUG_ALL);

	len = snprintf(g_user_buf, PAGE_SIZE, "Current DEBUG Level = %d\n", ipio_debug_level);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "You can set one of levels for debug as below:\n");
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_NONE = %d\n", DEBUG_NONE);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_IRQ = %d\n", DEBUG_IRQ);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_FINGER_REPORT = %d\n", DEBUG_FINGER_REPORT);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_FIRMWARE = %d\n", DEBUG_FIRMWARE);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_CONFIG = %d\n", DEBUG_CONFIG);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_I2C = %d\n", DEBUG_I2C);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_BATTERY = %d\n", DEBUG_BATTERY);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_MP_TEST = %d\n", DEBUG_MP_TEST);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_IOCTL = %d\n", DEBUG_IOCTL);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_NETLINK = %d\n", DEBUG_NETLINK);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_PARSER = %d\n", DEBUG_PARSER);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_GESTURE = %d\n", DEBUG_GESTURE);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_SPI = %d\n", DEBUG_SPI);
	len += snprintf(g_user_buf + len, PAGE_SIZE - len, "DEBUG_ALL = %d\n", DEBUG_ALL);

	ret = copy_to_user((uint32_t *) buff, g_user_buf, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos += len;

	return len;
}

static int file_write(struct file_buffer *file, bool new_open)
{
	struct file *f = NULL;
	mm_segment_t fs;
	loff_t pos;

	if (file->ptr == NULL) {
		ipio_err("str is invaild\n");
		return -1;
	}

	if (file->file_name == NULL) {
		ipio_err("file name is invaild\n");
		return -1;
	}

	if (file->file_len >= file->file_max_zise) {
		ipio_err("The length saved to file is too long !\n");
		return -1;
	}

	if (new_open)
		f = filp_open(file->file_name, O_WRONLY | O_CREAT | O_TRUNC, 644);
	else
		f = filp_open(file->file_name, O_WRONLY | O_CREAT | O_APPEND, 644);

	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open %s file\n", file->file_name);
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, file->ptr, file->file_len, &pos);
	set_fs(fs);
	filp_close(f, NULL);

	return 0;
}

static int debug_mode_get_data(struct file_buffer *file, uint8_t type, uint32_t frame_count)
{
	int ret;
	int timeout = 50;
	uint8_t cmd[2] = { 0 }, row, col;
	int16_t temp;
	unsigned char *ptr;
	int j;
	uint16_t write_index = 0;

	ipd->debug_data_start_flag = false;
	ipd->debug_data_frame = 0;
	row = core_config->tp_info->nYChannelNum;
	col = core_config->tp_info->nXChannelNum;

	mutex_lock(&ipd->touch_mutex);
	cmd[0] = 0xFA;
	cmd[1] = type;
	ret = core_write(core_config->slave_i2c_addr, cmd, 2);
	ipd->debug_data_start_flag = true;
	mutex_unlock(&ipd->touch_mutex);
	if (ret < 0)
		return ret;

	while ((write_index < frame_count) && (timeout > 0)) {

		ipio_err("frame %d,index=%d,count=%d\n", write_index, write_index % 1024, ipd->debug_data_frame);
		if ((write_index % 1024) < ipd->debug_data_frame) {
			mutex_lock(&ipd->touch_mutex);
			file->file_len = 0;
			memset(file->ptr, 0, file->file_max_zise);
			file->file_len += sprintf(file->ptr + file->file_len, "\n\nFrame%d,", write_index);
			for (j = 0; j < col; j++)
				file->file_len += sprintf(file->ptr + file->file_len, "[X%d] ,", j);
			ptr = &ipd->debug_buf[write_index%1024][35];
			for (j = 0; j < row * col; j ++, ptr += 2) {
				temp = (*ptr << 8) + *(ptr + 1);
				if (j % col == 0)
					file->file_len += sprintf(file->ptr + file->file_len, "\n[Y%d] ,", (j / col));
				file->file_len += sprintf(file->ptr + file->file_len, "%d, ", temp);
			}
			file->file_len += sprintf(file->ptr + file->file_len, "\n[X] ,");
			for (j = 0; j < row + col; j ++, ptr += 2) {
				temp = (*ptr << 8) + *(ptr + 1);
				if (j == col)
					file->file_len += sprintf(file->ptr + file->file_len, "\n[Y] ,");
				file->file_len += sprintf(file->ptr + file->file_len, "%d, ", temp);
			}
			file_write(file, false);
			write_index++;
			mutex_unlock(&ipd->touch_mutex);
			timeout = 50;
		}

		if (write_index%1024 == 0 && ipd->debug_data_frame == 1024)
			ipd->debug_data_frame = 0;

		mdelay(100);/*get one frame data take around 130ms*/
		timeout--;
		if (timeout == 0)
			ipio_err("debug mode get data timeout!\n");
	}
	ipd->debug_data_start_flag = false;

	return 0;

}

static ssize_t ilitek_proc_get_debug_mode_data_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret;
	uint8_t cmd[5] = { 0 };
	struct file_buffer csv;

	if (*pPos != 0)
		return 0;

	/*initialize file*/
	memset(csv.file_name, 0, sizeof(csv.file_name));
	sprintf(csv.file_name, "%s", DEBUG_DATA_FILE_PATH);
	csv.file_len = 0;
	csv.file_max_zise = DEBUG_DATA_FILE_SIZE;
	csv.ptr = vmalloc(csv.file_max_zise);

	if (ERR_ALLOC_MEM(csv.ptr)) {
		ipio_err("Failed to allocate CSV mem\n");
		goto out;
	}

	/*save data to csv*/
	ipio_info("Get Raw data%d frame\n", ipd->raw_count);
	ipio_info("Get Delta data %d frame\n", ipd->delta_count);


	csv.file_len += sprintf(csv.ptr + csv.file_len, "Get Raw data%d frame\n", ipd->raw_count);
	csv.file_len += sprintf(csv.ptr + csv.file_len, "Get Delta data %d frame\n", ipd->delta_count);
	file_write(&csv, true);

	/*change to debug mode*/
	cmd[0] = protocol->debug_mode;
	ret = core_config_switch_fw_mode(cmd);
	if (ret < 0)
		goto out;

	/*get raw data*/
	csv.file_len = 0;
	memset(csv.ptr, 0, csv.file_max_zise);
	csv.file_len += sprintf(csv.ptr + csv.file_len, "\n\n=======Raw data=======");
	file_write(&csv, false);
	ret = debug_mode_get_data(&csv, protocol->raw_data, ipd->raw_count);
	if (ret < 0)
		goto out;

	/*get delta data*/
	csv.file_len = 0;
	memset(csv.ptr, 0, csv.file_max_zise);
	csv.file_len += sprintf(csv.ptr + csv.file_len, "\n\n=======Delta data=======");
	file_write(&csv, false);
	ret = debug_mode_get_data(&csv, protocol->delta_data, ipd->delta_count);
	if (ret < 0)
		goto out;

	/*change to demo mode*/
	cmd[0] = protocol->demo_mode;
	ret = core_config_switch_fw_mode(cmd);

out:
	ipio_vfree((void **)&csv.ptr);


	return 0;
}

static ssize_t ilitek_proc_read_write_register_read(struct file *pFile, char __user *buf, size_t nCount, loff_t *pos)
{
	int ret = 0;
	uint32_t type, addr, read_data, write_data, write_len, stop_mcu;

	if (*pos != 0)
		return 0;

	stop_mcu = temp[0];
	type = temp[1];
	addr = temp[2];
	write_data = temp[3];
	write_len = temp[4];

	mutex_lock(&ipd->plat_mutex);

	ipio_info("stop_mcu = %d\n", temp[0]);

	if (stop_mcu == NO_STOP_MCU) {
		ret = core_config_ice_mode_enable(NO_STOP_MCU);
		if (ret < 0) {
			ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
			return -1;
		}
	} else {
		ret = core_config_ice_mode_enable(STOP_MCU);
		if (ret < 0) {
			ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
			return -1;
		}
	}

	if (type == REGISTER_READ) {
		read_data = core_config_ice_mode_read(addr);
		ipio_info("READ:addr = 0x%06x, read = 0x%08x\n", addr, read_data);
		nCount = snprintf(g_user_buf, PAGE_SIZE, "READ:addr = 0x%06x, read = 0x%08x\n", addr, read_data);

	} else {
		core_config_ice_mode_write(addr, write_data, write_len);
		ipio_info("WRITE:addr = 0x%06x, write = 0x%08x, len =%d byte\n", addr, write_data, write_len);
		nCount = snprintf(g_user_buf, PAGE_SIZE, "WRITE:addr = 0x%06x, write = 0x%08x, len =%d byte\n", addr, write_data, write_len);
	}
	core_config_ice_mode_disable();

	mutex_unlock(&ipd->plat_mutex);

	ret = copy_to_user(buf, g_user_buf, nCount);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space");
	}

	*pos += nCount;

	return nCount;

}

static ssize_t ilitek_proc_read_write_register_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char *token = NULL, *cur = NULL;
	char cmd[256] = { 0 };
	uint32_t count = 0;

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	token = cur = cmd;

	while ((token = strsep(&cur, ",")) != NULL) {
		temp[count] = str2hex(token);
		ipio_info("data[%d] = 0x%x\n", count, temp[count]);
		count++;
	}

	return size;
}

static ssize_t ilitek_proc_get_debug_mode_data_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char *token = NULL, *cur = NULL;
	char cmd[256] = { 0 };
	uint8_t temp[256] = { 0 }, count = 0;

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
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

	ipd->raw_count = ((temp[0] << 8) | temp[1]);
	ipd->delta_count = ((temp[2] << 8) | temp[3]);
	ipd->bg_count = ((temp[4] << 8) | temp[5]);

	ipio_info("Raw_count = %d, Delta_count = %d, BG_count = %d\n", ipd->raw_count, ipd->delta_count, ipd->bg_count);

	return size;
}

static ssize_t ilitek_proc_debug_level_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char cmd[10] = { 0 };

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	ipio_debug_level = katoi(cmd);

	ipio_info("ipio_debug_level = %d\n", ipio_debug_level);

	return size;
}

static ssize_t ilitek_proc_gesture_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	uint32_t len = 0;

	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	len = sprintf(g_user_buf, "isEnableGesture = %d\n", core_config->isEnableGesture);

	ipio_info("isEnableGesture = %d\n", core_config->isEnableGesture);

	ret = copy_to_user((uint32_t *) buff, g_user_buf, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos = len;

	return len;
}

static ssize_t ilitek_proc_gesture_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char cmd[10] = { 0 };

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);

	if (strcmp(cmd, "on") == 0) {
		ipio_info("enable gesture mode\n");
		core_config->isEnableGesture = true;
	} else if (strcmp(cmd, "off") == 0) {
		ipio_info("disable gesture mode\n");
		core_config->isEnableGesture = false;
	} else if (strcmp(cmd, "info") == 0) {
		ipio_info("gesture info mode\n");
		core_gesture->mode = GESTURE_INFO_MODE;
	} else if (strcmp(cmd, "normal") == 0) {
		ipio_info("gesture normal mode\n");
		core_gesture->mode = GESTURE_NORMAL_MODE;
	} else
		ipio_err("Unknown command\n");

	return size;
}

static ssize_t ilitek_proc_check_battery_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	uint32_t len = 0;

	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	len = sprintf(g_user_buf, "isEnablePollCheckPower = %d\n", ipd->isEnablePollCheckPower);

	ipio_info("isEnablePollCheckPower = %d\n", ipd->isEnablePollCheckPower);

	ret = copy_to_user((uint32_t *) buff, g_user_buf, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos = len;

	return len;
}

static ssize_t ilitek_proc_check_battery_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char cmd[10] = { 0 };

	if (size > sizeof(cmd)) {
		ipio_err("Size is larger than the length of cmd\n");
		goto out;
	}

	if (ipd->check_power_status_queue == NULL) {
		ipio_err("work queue isn't created, do nothing\n");
		goto out;
	}

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);

	if (strcmp(cmd, "on") == 0) {
		ipio_info("Start the thread of check power status\n");
		queue_delayed_work(ipd->check_power_status_queue, &ipd->check_power_status_work, ipd->work_delay);
		ipd->isEnablePollCheckPower = true;
	} else if (strcmp(cmd, "off") == 0) {
		ipio_info("Cancel the thread of check power status\n");
		cancel_delayed_work_sync(&ipd->check_power_status_work);
		ipd->isEnablePollCheckPower = false;
	} else
		ipio_err("Unknown command\n");

out:
	return size;
}

static ssize_t ilitek_proc_check_esd_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	uint32_t len = 0;

	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	len = sprintf(g_user_buf, "isEnablePollCheckEsd = %d\n", ipd->isEnablePollCheckEsd);

	ipio_info("isEnablePollCheckEsd = %d\n", ipd->isEnablePollCheckEsd);

	ret = copy_to_user((uint32_t *) buff, g_user_buf, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos = len;

	return len;
}

static ssize_t ilitek_proc_check_esd_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	char cmd[10] = { 0 };

	if (size > sizeof(cmd)) {
		ipio_err("Size is larger than the length of cmd\n");
		goto out;
	}

	if (ipd->check_esd_status_queue == NULL) {
		ipio_err("work queue isn't created, do nothing\n");
		goto out;
	}

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);

	if (strcmp(cmd, "on") == 0) {
		ipio_info("Start the thread of check esd status\n");
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);
		ipd->isEnablePollCheckEsd = true;
	} else if (strcmp(cmd, "off") == 0) {
		ipio_info("Cancel the thread of check esd status\n");
		cancel_delayed_work_sync(&ipd->check_esd_status_work);
		ipd->isEnablePollCheckEsd = false;
	} else
		ipio_err("Unknown command\n");

out:
	return size;
}

static ssize_t ilitek_proc_fw_process_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0;
	uint32_t len = 0;

	/*
	 * If file position is non-zero,  we assume the string has been read
	 * and indicates that there is no more data to be read.
	 */
	if (*pPos != 0)
		return 0;

	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	len = sprintf(g_user_buf, "%02d", core_firmware->update_status);

	ipio_info("update status = %d\n", core_firmware->update_status);

	ret = copy_to_user((uint32_t *) buff, &core_firmware->update_status, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos = len;

	return len;
}


static ssize_t ilitek_proc_fw_upgrade_read(struct file *filp, char __user *buff, size_t size, loff_t *pPos)
{
	int ret = 0, retry = 0;
	uint32_t len = 0;

	retry = core_firmware->retry_times;

	ipio_info("Preparing to upgarde firmware\n");

	if (*pPos != 0)
		return 0;

	 memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));

	ilitek_platform_disable_irq();
	is_force_upgrade = true;
	mutex_lock(&ipd->touch_mutex);
	if (mode_chose == SPI_MODE)
		ret = ilitek_platform_reset_ctrl(true, RST_METHODS);
	else
		ret = core_firmware_upgrade(UPGRADE_FLASH, HEX_FILE, OPEN_FW_METHOD);
	if (ret < 0)
		ipio_err("firmware upgrade failed\n");
	mutex_unlock(&ipd->touch_mutex);
	is_force_upgrade = false;
	ilitek_platform_enable_irq();

	if (ret < 0) {
		core_firmware->update_status = ret;
		ipio_err("Failed to upgrade firwmare\n");
	} else {
		core_firmware->update_status = 100;
		ipio_info("Succeed to upgrade firmware\n");
	}

	len = sprintf(g_user_buf, "upgrade firwmare %s\n", (ret < 0) ? "failed" : "succeed");

	ret = copy_to_user((uint32_t *) buff, g_user_buf, len);
	if (ret < 0) {
		ipio_err("Failed to copy data to user space\n");
	}

	*pPos = len;

	return len;
}

/* for debug */
static ssize_t ilitek_proc_ioctl_write(struct file *filp, const char *buff, size_t size, loff_t *pPos)
{
	int ret = 0, count = 0, i;
	char cmd[512] = { 0 };
	char *token = NULL, *cur = NULL;
	uint8_t temp[256] = { 0 };
	uint8_t *data = NULL;

	if (buff != NULL) {
		ret = copy_from_user(cmd, buff, size - 1);
		if (ret < 0) {
			ipio_info("copy data from user space, failed\n");
			return -1;
		}
	}

	ipio_info("size = %d, cmd = %s\n", (int)size, cmd);

	token = cur = cmd;

	data = kmalloc(512 * sizeof(uint8_t), GFP_KERNEL);
	memset(data, 0, 512);

	while ((token = strsep(&cur, ",")) != NULL) {
		data[count] = str2hex(token);
		ipio_info("data[%d] = %x\n", count, data[count]);
		count++;
	}

	ipio_info("cmd = %s\n", cmd);

	if (strcmp(cmd, "reset") == 0) {
		ipio_info("hw reset\n");
		ret = ilitek_platform_reset_ctrl(true, RST_METHODS);
	} else if (strcmp(cmd, "softreset") == 0) {
		ipio_info("software Reset\n");
		core_config_ic_reset();
	} else if (strcmp(cmd, "disirq") == 0) {
		ipio_info("Disable IRQ\n");
		ilitek_platform_disable_irq();
	} else if (strcmp(cmd, "enairq") == 0) {
		ipio_info("Enable IRQ\n");
		ilitek_platform_enable_irq();
	} else if (strcmp(cmd, "irqstatus") == 0) {
		ipio_info("IRQ status\n");
		ipio_info(" %s ITQ = %x\n", ipd->isEnableIRQ ? "Enabled" : "Disabled", ipd->isEnableIRQ);
	} else if (strcmp(cmd, "reportstatus") == 0) {
		ipio_info("report status\n");
		ipio_info(" %s report = %x\n", core_fr->isEnableFR ? "Enabled" : "Disabled", core_fr->isEnableFR);
	} else if (strcmp(cmd, "getchip") == 0) {
		ipio_info("Get Chip id\n");
		core_config_get_chip_id();
	} else if (strcmp(cmd, "gettpinfo") == 0) {
		ipio_info("Get tp info\n");
		ilitek_platform_read_tp_info();
	} else if (strcmp(cmd, "dispcc") == 0) {
		ipio_info("disable phone cover\n");
		core_config_phone_cover_ctrl(false);
	} else if (strcmp(cmd, "enapcc") == 0) {
		ipio_info("enable phone cover\n");
		core_config_phone_cover_ctrl(true);
	} else if (strcmp(cmd, "disfsc") == 0) {
		ipio_info("disable finger sense\n");
		    core_config_finger_sense_ctrl(false);
	} else if (strcmp(cmd, "enafsc") == 0) {
		ipio_info("enable finger sense\n");
		core_config_finger_sense_ctrl(true);
	} else if (strcmp(cmd, "disprox") == 0) {
		ipio_info("disable proximity\n");
		core_config_proximity_ctrl(false);
	} else if (strcmp(cmd, "enaprox") == 0) {
		ipio_info("enable proximity\n");
		core_config_proximity_ctrl(true);
	} else if (strcmp(cmd, "disglove") == 0) {
		ipio_info("disable glove function\n");
		core_config_glove_ctrl(false, false);
	} else if (strcmp(cmd, "enaglove") == 0) {
		ipio_info("enable glove function\n");
		core_config_glove_ctrl(true, false);
	} else if (strcmp(cmd, "glovesl") == 0) {
		ipio_info("set glove as seamless\n");
		core_config_glove_ctrl(true, true);
	} else if (strcmp(cmd, "enastylus") == 0) {
		ipio_info("enable stylus\n");
		core_config_stylus_ctrl(true, false);
	} else if (strcmp(cmd, "disstylus") == 0) {
		ipio_info("disable stylus\n");
		core_config_stylus_ctrl(false, false);
	} else if (strcmp(cmd, "stylussl") == 0) {
		ipio_info("set stylus as seamless\n");
		core_config_stylus_ctrl(true, true);
	} else if (strcmp(cmd, "tpscan_ab") == 0) {
		ipio_info("set TP scan as mode AB\n");
		core_config_tp_scan_mode(true);
	} else if (strcmp(cmd, "tpscan_b") == 0) {
		ipio_info("set TP scan as mode B\n");
		core_config_tp_scan_mode(false);
	} else if (strcmp(cmd, "phone_cover") == 0) {
		ipio_info("set size of phone conver window\n");
		core_config_set_phone_cover(data);
	} else if (strcmp(cmd, "debugmode") == 0) {
		ipio_info("debug mode test enter\n");
		temp[0] = protocol->debug_mode;
		core_config_switch_fw_mode(temp);
	} else if (strcmp(cmd, "baseline") == 0) {
		ipio_info("test baseline raw\n");
		temp[0] = protocol->debug_mode;
		core_config_switch_fw_mode(temp);
		ilitek_platform_disable_irq();
		temp[0] = 0xFA;
		temp[1] = 0x08;
		core_write(core_config->slave_i2c_addr, temp, 2);
		ilitek_platform_enable_irq();
	} else if (strcmp(cmd, "delac_on") == 0) {
		ipio_info("test get delac\n");
		temp[0] = protocol->debug_mode;
		core_config_switch_fw_mode(temp);
		ilitek_platform_disable_irq();
		temp[0] = 0xFA;
		temp[1] = 0x03;
		core_write(core_config->slave_i2c_addr, temp, 2);
		ilitek_platform_enable_irq();
	} else if (strcmp(cmd, "delac_off") == 0) {
		ipio_info("test get delac\n");
		temp[0] = protocol->demo_mode;
		core_config_switch_fw_mode(temp);
	} else if (strcmp(cmd, "test") == 0) {
		ipio_info("test test_reset test 1\n");
		gpio_direction_output(ipd->reset_gpio, 1);
		mdelay(1);
		gpio_set_value(ipd->reset_gpio, 0);
		mdelay(1);
		gpio_set_value(ipd->reset_gpio, 1);
		mdelay(10);
	} else if (strcmp(cmd, "gt") == 0) {
		ipio_info("test Gesture test\n");
		if (mode_chose == SPI_MODE)
			core_gesture_load_code();
	} else if (strcmp(cmd, "suspend") == 0) {
		ipio_info("test suspend test\n");
		core_config_ic_suspend();
	} else if (strcmp(cmd, "resume") == 0) {
		ipio_info("test resume test\n");
		core_config_ic_resume();
	} else if (strcmp(cmd, "i2c_w") == 0) {
		int w_len = 0;
		w_len = data[1];
		ipio_info("w_len = %d\n", w_len);

		for (i = 0; i < w_len; i++) {
			temp[i] = data[2 + i];
			ipio_info("i2c[%d] = %x\n", i, temp[i]);
		}

		core_write(core_config->slave_i2c_addr, temp, w_len);
	} else if (strcmp(cmd, "i2c_r") == 0) {
		int r_len = 0;
		r_len = data[1];
		ipio_info("r_len = %d\n", r_len);

		core_read(core_config->slave_i2c_addr, &temp[0], r_len);

		for (i = 0; i < r_len; i++)
			ipio_info("temp[%d] = %x\n", i, temp[i]);
	} else if (strcmp(cmd, "i2c_w_r") == 0) {
		int delay = 0;
		int w_len = 0, r_len = 0;
		w_len = data[1];
		r_len = data[2];
		delay = data[3];
		ipio_info("w_len = %d, r_len = %d, delay = %d\n", w_len, r_len, delay);

		for (i = 0; i < w_len; i++) {
			temp[i] = data[4 + i];
			ipio_info("temp[%d] = %x\n", i, temp[i]);
		}

		core_write(core_config->slave_i2c_addr, temp, w_len);

		memset(temp, 0, sizeof(temp));
		mdelay(delay);

		core_read(core_config->slave_i2c_addr, &temp[0], r_len);

		for (i = 0; i < r_len; i++)
			ipio_info("temp[%d] = %x\n", i, temp[i]);
	} else if (strcmp(cmd, "gettpregdata") == 0) {
		ipio_info("test gettpregdata set reg is 0x%X\n",\
			(data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4]));
		mutex_lock(&ipd->plat_mutex);
		core_config_get_reg_data(data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4]);
		mutex_unlock(&ipd->plat_mutex);
	} else if (strcmp(cmd, "getoneddiregdata") == 0) {
		ipio_info("test getoneddiregdata\n");
		mutex_lock(&ipd->plat_mutex);
		ret = core_config_ice_mode_enable(NO_STOP_MCU);
		if (ret < 0) {
			ipio_info("Failed to enter ICE mode, res = %d\n", ret);
		}
		core_get_ddi_register_onlyone(data[1], data[2]);
		core_config_ice_mode_disable();
		mutex_unlock(&ipd->plat_mutex);
	} else if (strcmp(cmd, "setoneddiregdata") == 0) {
		ipio_info("test getoneddiregdata\n");
		mutex_lock(&ipd->plat_mutex);
		ret = core_config_ice_mode_enable(NO_STOP_MCU);
		if (ret < 0) {
			ipio_info("Failed to enter ICE mode, res = %d\n", ret);
		}
		core_set_ddi_register_onlyone(data[1], data[2], data[3]);
		core_config_ice_mode_disable();
		mutex_unlock(&ipd->plat_mutex);
	} else if (strcmp(cmd, "edge_plam_ctrl") == 0) {
		core_edge_plam_ctrl(data[1]);
	} else if (strcmp(cmd, "dumpiramdata") == 0) {
		ipio_info("Start = 0x%x, End = 0x%x, Dump IRAM path = %s\n", data[1], data[2], DUMP_IRAM_PATH);
		ret = core_dump_iram(data[1], data[2]);
		if (ret < 0)
			ipio_err("Failed to dump iram data\n");
	} else {
		ipio_err("Unknown command\n");
	}

	ipio_kfree((void **)&data);
	return size;
}

static long ilitek_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0, length = 0;
	uint8_t *szBuf = NULL, if_to_user = 0;
	static uint16_t i2c_rw_length;
	uint32_t id_to_user[3] = {0};
	char dbg[10] = { 0 };

	if (_IOC_TYPE(cmd) != ILITEK_IOCTL_MAGIC) {
		ipio_err("The Magic number doesn't match\n");
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > ILITEK_IOCTL_MAXNR) {
		ipio_err("The number of ioctl doesn't match\n");
		return -ENOTTY;
	}

	szBuf = kcalloc(IOCTL_I2C_BUFF, sizeof(uint8_t), GFP_KERNEL);
	if (ERR_ALLOC_MEM(szBuf)) {
		ipio_err("Failed to allocate mem\n");
		return -ENOMEM;
	}

	mutex_lock(&ipd->plat_mutex);
	switch (cmd) {
	case ILITEK_IOCTL_I2C_WRITE_DATA:
		ipio_info("ioctl: i2c write: len = %d\n", i2c_rw_length);
		ret = copy_from_user(szBuf, (uint8_t *) arg, i2c_rw_length);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ret = core_write(core_config->slave_i2c_addr, &szBuf[0], i2c_rw_length);
			if (ret < 0)
				ipio_err("Failed to write data via i2c\n");
		}
		break;

	case ILITEK_IOCTL_I2C_READ_DATA:
		ipio_info("ioctl: i2c read: len = %d\n", i2c_rw_length);
		ret = core_read(core_config->slave_i2c_addr, szBuf, i2c_rw_length);
		if (ret < 0) {
			ipio_err("Failed to read data via i2c\n");
		} else {
			ret = copy_to_user((uint8_t *) arg, szBuf, i2c_rw_length);
			if (ret < 0)
				ipio_err("Failed to copy data to user space\n");
		}
		break;

	case ILITEK_IOCTL_I2C_SET_WRITE_LENGTH:
	case ILITEK_IOCTL_I2C_SET_READ_LENGTH:
		i2c_rw_length = arg;
		break;

	case ILITEK_IOCTL_TP_HW_RESET:
		ipio_info("ioctl: hw reset\n");
		ret = ilitek_platform_reset_ctrl(true, RST_METHODS);
		break;

	case ILITEK_IOCTL_TP_POWER_SWITCH:
		ipio_info("Not implemented yet\n");
		break;

	case ILITEK_IOCTL_TP_REPORT_SWITCH:
		ret = copy_from_user(szBuf, (uint8_t *) arg, 1);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_info("ioctl: report switch = %d\n", szBuf[0]);
			if (szBuf[0]) {
				core_fr->isEnableFR = true;
				ipio_debug(DEBUG_IOCTL, "Function of finger report was enabled\n");
			} else {
				core_fr->isEnableFR = false;
				ipio_debug(DEBUG_IOCTL, "Function of finger report was disabled\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_IRQ_SWITCH:
		ipio_info("ioctl: irq switch = %d\n", szBuf[0]);
		ret = copy_from_user(szBuf, (uint8_t *) arg, 1);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			if (szBuf[0]) {
				ilitek_platform_enable_irq();
			} else {
				ilitek_platform_disable_irq();
			}
		}
		break;

	case ILITEK_IOCTL_TP_DEBUG_LEVEL:
		ret = copy_from_user(dbg, (uint32_t *) arg, sizeof(uint32_t));
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_debug_level = katoi(dbg);
			ipio_info("ipio_debug_level = %d", ipio_debug_level);
		}
		break;

	case ILITEK_IOCTL_TP_FUNC_MODE:
		ret = copy_from_user(szBuf, (uint8_t *) arg, 3);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_info("ioctl: set func mode = %x,%x,%x\n", szBuf[0], szBuf[1], szBuf[2]);
			core_write(core_config->slave_i2c_addr, &szBuf[0], 3);
		}

		break;

	case ILITEK_IOCTL_TP_FW_VER:
		ret = core_config_get_fw_ver();
		if (ret < 0) {
			ipio_err("Failed to get firmware version\n");
		} else {
			ipio_info("ioctl: get fw version\n");
			ret = copy_to_user((uint8_t *) arg, core_config->firmware_ver, protocol->fw_ver_len);
			if (ret < 0) {
				ipio_err("Failed to copy firmware version to user space\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_PL_VER:
		ret = core_config_get_protocol_ver();
		if (ret < 0) {
			ipio_err("Failed to get protocol version\n");
		} else {
			ipio_info("ioctl: get protocl version\n");
			ret = copy_to_user((uint8_t *) arg, core_config->protocol_ver, protocol->pro_ver_len);
			if (ret < 0) {
				ipio_err("Failed to copy protocol version to user space\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_CORE_VER:
		ret = core_config_get_core_ver();
		if (ret < 0) {
			ipio_err("Failed to get core version\n");
		} else {
			ipio_info("ioctl: get core version\n");
			ret = copy_to_user((uint8_t *) arg, core_config->core_ver, protocol->core_ver_len);
			if (ret < 0) {
				ipio_err("Failed to copy core version to user space\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_DRV_VER:
		length = sprintf(szBuf, "%s", DRIVER_VERSION);
		if (!length) {
			ipio_err("Failed to convert driver version from definiation\n");
		} else {
			ipio_info("ioctl: get driver version\n");
			ret = copy_to_user((uint8_t *) arg, szBuf, length);
			if (ret < 0) {
				ipio_err("Failed to copy driver ver to user space\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_CHIP_ID:
		ret = core_config_get_chip_id();
		if (ret < 0) {
			ipio_err("Failed to get chip id\n");
		} else {
			ipio_info("ioctl: get chip id\n");
			id_to_user[0] = core_config->chip_id << 16 | core_config->chip_type;
			id_to_user[1] = core_config->chip_otp_id;
			id_to_user[2] = core_config->chip_ana_id;

			ret = copy_to_user((uint32_t *) arg, id_to_user, sizeof(id_to_user));
			if (ret < 0) {
				ipio_err("Failed to copy chip id to user space\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_NETLINK_CTRL:
		ret = copy_from_user(szBuf, (uint8_t *) arg, 1);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_info("ioctl: netlink ctrl = %d\n", szBuf[0]);
			if (szBuf[0]) {
				core_fr->isEnableNetlink = true;
				ipio_debug(DEBUG_IOCTL, "Netlink has been enabled\n");
			} else {
				core_fr->isEnableNetlink = false;
				ipio_debug(DEBUG_IOCTL, "Netlink has been disabled\n");
			}
		}
		break;

	case ILITEK_IOCTL_TP_NETLINK_STATUS:
		ipio_info("ioctl: get netlink stat = %d\n", core_fr->isEnableNetlink);
		ret = copy_to_user((int *)arg, &core_fr->isEnableNetlink, sizeof(int));
		if (ret < 0) {
			ipio_err("Failed to copy chip id to user space\n");
		}
		break;

	case ILITEK_IOCTL_TP_MODE_CTRL:
		ret = copy_from_user(szBuf, (uint8_t *) arg, 4);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_info("ioctl: switch fw mode = %d\n", szBuf[0]);
			ret = core_config_switch_fw_mode(szBuf);
			if (ret < 0)
				ipio_err("ioctl: switch to fw mode (%d) failed\n", szBuf[0]);
		}
		break;

	case ILITEK_IOCTL_TP_MODE_STATUS:
		ipio_info("ioctl: current firmware mode = %d", core_fr->actual_fw_mode);
		ret = copy_to_user((int *)arg, &core_fr->actual_fw_mode, sizeof(int));
		if (ret < 0)
			ipio_err("Failed to copy chip id to user space\n");

		break;
	/* It works for host downloado only */
	case ILITEK_IOCTL_ICE_MODE_SWITCH:
		ret = copy_from_user(szBuf, (uint8_t *) arg, 1);
		if (ret < 0) {
			ipio_err("Failed to copy data from user space\n");
		} else {
			ipio_info("ioctl: switch ice mode = %d", szBuf[0]);
			if (szBuf[0])
				core_config->icemodeenable = true;
			else
				core_config->icemodeenable = false;
		}
		break;

	case ILITEK_IOCTL_TP_INTERFACE_TYPE:
		if_to_user = mode_chose;
		ret = copy_to_user((uint8_t *) arg, &if_to_user, sizeof(if_to_user));
		if (ret < 0) {
			ipio_err("ioctl: Failed to copy interface type to user space\n");
		}
		break;

	case ILITEK_IOCTL_TP_DUMP_FLASH:
		ipio_info("ioctl: dump flash data\n");
		ret = core_dump_flash();
		if (ret < 0) {
			ipio_err("ioctl: Failed to dump flash data\n");
		}
		break;

	default:
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&ipd->plat_mutex);

	ipio_kfree((void **)&szBuf);
	return ret;
}

struct proc_dir_entry *proc_dir_ilitek;
struct proc_dir_entry *proc_ioctl;
struct proc_dir_entry *proc_fw_process;
struct proc_dir_entry *proc_fw_upgrade;
struct proc_dir_entry *proc_gesture;
struct proc_dir_entry *proc_debug_level;
struct proc_dir_entry *proc_mp_test;
struct proc_dir_entry *proc_debug_message;
struct proc_dir_entry *proc_debug_message_switch;
struct proc_dir_entry *proc_fw_pc_counter;
struct proc_dir_entry *proc_get_debug_mode_data;

struct file_operations proc_ioctl_fops = {
	.unlocked_ioctl = ilitek_proc_ioctl,
	.write = ilitek_proc_ioctl_write,
};

struct file_operations proc_fw_process_fops = {
	.read = ilitek_proc_fw_process_read,
};

struct file_operations proc_fw_upgrade_fops = {
	.read = ilitek_proc_fw_upgrade_read,
};

struct file_operations proc_gesture_fops = {
	.write = ilitek_proc_gesture_write,
	.read = ilitek_proc_gesture_read,
};

struct file_operations proc_check_battery_fops = {
	.write = ilitek_proc_check_battery_write,
	.read = ilitek_proc_check_battery_read,
};

struct file_operations proc_check_esd_fops = {
	.write = ilitek_proc_check_esd_write,
	.read = ilitek_proc_check_esd_read,
};

struct file_operations proc_debug_level_fops = {
	.write = ilitek_proc_debug_level_write,
	.read = ilitek_proc_debug_level_read,
};

struct file_operations proc_mp_lcm_on_test_fops = {
	.read = ilitek_proc_mp_lcm_on_test_read,
};

struct file_operations proc_mp_lcm_off_test_fops = {
	.read = ilitek_proc_mp_lcm_off_test_read,
};

struct file_operations proc_debug_message_fops = {
	.write = ilitek_proc_debug_message_write,
	.read = ilitek_proc_debug_message_read,
};

struct file_operations proc_debug_message_switch_fops = {
	.read = ilitek_proc_debug_switch_read,
};

struct file_operations proc_fw_pc_counter_fops = {
	.read = ilitek_proc_fw_pc_counter_read,
};

struct file_operations proc_get_delta_data_fops = {
	.read = ilitek_proc_get_delta_data_read,
};

struct file_operations proc_get_raw_data_fops = {
	.read = ilitek_proc_fw_get_raw_data_read,
};

struct file_operations proc_get_debug_mode_data_fops = {
	.read = ilitek_proc_get_debug_mode_data_read,
	.write = ilitek_proc_get_debug_mode_data_write,
};

struct file_operations proc_read_write_register_fops = {
	.read = ilitek_proc_read_write_register_read,
	.write = ilitek_proc_read_write_register_write,
};
/**
 * This struct lists all file nodes will be created under /proc filesystem.
 *
 * Before creating a node that you want, declaring its file_operations structure
 * is necessary. After that, puts the structure into proc_table, defines its
 * node's name in the same row, and the init function lterates the table and
 * creates all nodes under /proc.
 *
 */
typedef struct {
	char *name;
	struct proc_dir_entry *node;
	struct file_operations *fops;
	bool isCreated;
} proc_node_t;

proc_node_t proc_table[] = {
	{"ioctl", NULL, &proc_ioctl_fops, false},
	{"fw_process", NULL, &proc_fw_process_fops, false},
	{"fw_upgrade", NULL, &proc_fw_upgrade_fops, false},
	{"gesture", NULL, &proc_gesture_fops, false},
	{"check_battery", NULL, &proc_check_battery_fops, false},
	{"check_esd", NULL, &proc_check_esd_fops, false},
	{"debug_level", NULL, &proc_debug_level_fops, false},
	{"mp_lcm_on_test", NULL, &proc_mp_lcm_on_test_fops, false},
	{"mp_lcm_off_test", NULL, &proc_mp_lcm_off_test_fops, false},
	{"debug_message", NULL, &proc_debug_message_fops, false},
	{"debug_message_switch", NULL, &proc_debug_message_switch_fops, false},
	{"fw_pc_counter", NULL, &proc_fw_pc_counter_fops, false},
	{"show_delta_data", NULL, &proc_get_delta_data_fops, false},
	{"show_raw_data", NULL, &proc_get_raw_data_fops, false},
	{"get_debug_mode_data", NULL, &proc_get_debug_mode_data_fops, false},
	{"read_write_register", NULL, &proc_read_write_register_fops, false},
};

#define NETLINK_USER 21
struct sock *_gNetLinkSkb;
struct nlmsghdr *_gNetLinkHead;
struct sk_buff *_gSkbOut;
int _gPID;

void netlink_reply_msg(void *raw, int size)
{
	int ret;
	int msg_size = size;
	uint8_t *data = (uint8_t *) raw;

	ipio_debug(DEBUG_NETLINK, "The size of data being sent to user = %d\n", msg_size);
	ipio_debug(DEBUG_NETLINK, "pid = %d\n", _gPID);
	ipio_debug(DEBUG_NETLINK, "Netlink is enable = %d\n", core_fr->isEnableNetlink);

	if (core_fr->isEnableNetlink) {
		_gSkbOut = nlmsg_new(msg_size, 0);

		if (!_gSkbOut) {
			ipio_err("Failed to allocate new skb\n");
			return;
		}

		_gNetLinkHead = nlmsg_put(_gSkbOut, 0, 0, NLMSG_DONE, msg_size, 0);
		NETLINK_CB(_gSkbOut).dst_group = 0;	/* not in mcast group */

		/* strncpy(NLMSG_DATA(_gNetLinkHead), data, msg_size); */
		ipio_memcpy(nlmsg_data(_gNetLinkHead), data, msg_size, size);

		ret = nlmsg_unicast(_gNetLinkSkb, _gSkbOut, _gPID);
		if (ret < 0)
			ipio_err("Failed to send data back to user\n");
	}
}
EXPORT_SYMBOL(netlink_reply_msg);

static void netlink_recv_msg(struct sk_buff *skb)
{
	_gPID = 0;

	ipio_debug(DEBUG_NETLINK, "Netlink is enable = %d\n", core_fr->isEnableNetlink);

	_gNetLinkHead = (struct nlmsghdr *)skb->data;

	ipio_debug(DEBUG_NETLINK, "Received a request from client: %s, %d\n",
	    (char *)NLMSG_DATA(_gNetLinkHead), (int)strlen((char *)NLMSG_DATA(_gNetLinkHead)));

	/* pid of sending process */
	_gPID = _gNetLinkHead->nlmsg_pid;

	ipio_debug(DEBUG_NETLINK, "the pid of sending process = %d\n", _gPID);

	/* TODO: may do something if there's not receiving msg from user. */
	if (_gPID != 0) {
		ipio_err("The channel of Netlink has been established successfully !\n");
		core_fr->isEnableNetlink = true;
	} else {
		ipio_err("Failed to establish the channel between kernel and user space\n");
		core_fr->isEnableNetlink = false;
	}
}

static int netlink_init(void)
{
	int ret = 0;

#if KERNEL_VERSION(3, 4, 0) > LINUX_VERSION_CODE
	_gNetLinkSkb = netlink_kernel_create(&init_net, NETLINK_USER, netlink_recv_msg, NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = {
		.input = netlink_recv_msg,
	};

	_gNetLinkSkb = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
#endif

	ipio_info("Initialise Netlink and create its socket\n");

	if (!_gNetLinkSkb) {
		ipio_err("Failed to create nelink socket\n");
		ret = -EFAULT;
	}

	return ret;
}

int ilitek_proc_init(void)
{
	int i = 0, ret = 0;

	proc_dir_ilitek = proc_mkdir("ilitek", NULL);

	for (; i < ARRAY_SIZE(proc_table); i++) {
		proc_table[i].node = proc_create(proc_table[i].name, 0664, proc_dir_ilitek, proc_table[i].fops);

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

	return ret;
}
EXPORT_SYMBOL(ilitek_proc_init);

void ilitek_proc_remove(void)
{
	int i = 0;

	for (; i < ARRAY_SIZE(proc_table); i++) {
		if (proc_table[i].isCreated == true) {
			ipio_info("Removed %s under /proc\n", proc_table[i].name);
			remove_proc_entry(proc_table[i].name, proc_dir_ilitek);
		}
	}

	remove_proc_entry("ilitek", NULL);
	netlink_kernel_release(_gNetLinkSkb);
}
EXPORT_SYMBOL(ilitek_proc_remove);

/*********** Add sys node for tcmd and fw upgrade  *********************/

static ssize_t drv_power_on_show(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
	ipio_info("*** %s() ipd->suspended = %d ***\n", __func__, ipd->suspended);

	if (ipd->suspended == true)
		return snprintf(pBuf, PAGE_SIZE, "0\n");
	else
		return snprintf(pBuf, PAGE_SIZE, "1\n");
}
static DEVICE_ATTR(poweron, 0444, drv_power_on_show, NULL);

static ssize_t drv_product_info_show(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
	ipio_info ("*** %s() productinfo = %s ***\n", __func__, ipd->TP_IC_TYPE);

	return scnprintf(pBuf, PAGE_SIZE, "%s\n", ipd->TP_IC_TYPE);
}
static DEVICE_ATTR(productinfo, 0444, drv_product_info_show, NULL);

static ssize_t drv_force_reflash_store(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
	int temp;
	ipio_info ("*** %s() ***\n", __func__);
	memset(g_user_buf, 0, USER_STR_BUFF * sizeof(unsigned char));
	memcpy(g_user_buf, pBuf, nSize - 1);

	ipio_info ("%s g_user_buf=%s\n", __func__, g_user_buf);
	temp = katoi(g_user_buf);
	ipio_info ("%s temp=%d\n", __func__, temp);
	if (temp == 1)
		core_firmware->force_upgrad = true;
	else
		core_firmware->force_upgrad = false;

	ipio_info("%s core_firmware->force_upgrad =%d\n", __func__, core_firmware->force_upgrad);
	return nSize;
}
static DEVICE_ATTR(forcereflash, 0220, NULL, drv_force_reflash_store);


static ssize_t drv_flash_prog_show(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
	ipio_info ("*** %s() core_firmware->isUpgrading = %d ***\n", __func__, core_firmware->isUpgrading);

	return scnprintf(pBuf, PAGE_SIZE, "%d\n", core_firmware->isUpgrading);
}
static DEVICE_ATTR(flashprog, 0444, drv_flash_prog_show, NULL);

static ssize_t do_reflash_store(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
	int ret;
	char prefix[128] = "ILITEK";
	const char *chip_name = ipd->TP_IC_TYPE;

	ipio_info ("*** %s() ***\n", __func__);
	memset(fw_name_buf, 0, USER_STR_BUFF * sizeof(unsigned char));
	memcpy(fw_name_buf, pBuf, nSize - 1);
	ipio_info ("%s g_user_buf=%s\n", __func__, g_user_buf);

	if (ipd->suspended == true) {
		ipio_err("%s: In suspend state, try again later\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if (core_firmware->isUpgrading == true) {
		ipio_err("%s: In FW flashing state, try again later\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if ((core_firmware->force_upgrad != 1) && (mode_chose == I2C_MODE)) {
		if (strnstr(fw_name_buf, prefix, strnlen(fw_name_buf, USER_STR_BUFF * sizeof(unsigned char))) <= 0) {
			ipio_err("%s: FW does not belong to %s\n", __func__, prefix);
			ret = -EINVAL;
			goto exit;
		}
		if (strnstr(fw_name_buf, chip_name, strnlen(fw_name_buf, USER_STR_BUFF * sizeof(unsigned char))) <= 0) {
			ipio_err("%s: FW does not belong to chip %s\n", __func__, chip_name);
			ret = -EINVAL;
			goto exit;
		}
		ipio_info("%s: FW belong to %s\n", __func__, prefix);
		ipio_err("%s: FW belong to chip %s\n", __func__, chip_name);
	}

	ilitek_platform_disable_irq();

	if (ipd->isEnablePollCheckPower) {
		ipd->isEnablePollCheckPower = false;
		cancel_delayed_work_sync(&ipd->check_power_status_work);
	}

	if (ipd->isEnablePollCheckEsd) {
		ipd->isEnablePollCheckEsd = false;
		cancel_delayed_work_sync(&ipd->check_esd_status_work);
	}

	/* ret = core_firmware_upgrade(g_user_buf, false); */
	use_g_user_buf = true;
	if (mode_chose == SPI_MODE) {
		mutex_lock(&ipd->touch_mutex);
		ret = core_config_switch_fw_mode(&protocol->demo_mode);
		mutex_unlock(&ipd->touch_mutex);
	}
	ipd->sys_boot_fw = true;
	ipd->boot_download_fw_done = 1;
	wake_up(&(ipd->boot_download_fw));

	ilitek_platform_enable_irq();

	if (ipd->isEnablePollCheckPower)
		queue_delayed_work(ipd->check_power_status_queue,
			&ipd->check_power_status_work, ipd->work_delay);
	if (ipd->isEnablePollCheckEsd)
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);

	if (ret < 0) {
		core_firmware->update_status = ret;
		ipio_err("Failed to upgrade firwmare\n");
	} else {
		core_firmware->update_status = 100;
		ipio_info("Succeed to upgrade firmware\n");
	}

	core_firmware->isUpgrading = false;
	core_fr->isEnableFR = true;
exit:
	mutex_lock(&ipd->touch_mutex);
	if (mode_chose == SPI_MODE) {
		if (core_config_get_chip_id() < 0)
			ipio_err("Failed to get chip id\n");
	}
	mutex_unlock(&ipd->touch_mutex);
	return nSize;

}

static DEVICE_ATTR(doreflash, 0220, NULL, do_reflash_store);

static ssize_t drv_build_id_show(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
	ipio_info ("*** %s() Fw Version = V%d.%d.%d.%d***\n", __func__, core_config->firmware_ver[1], core_config->firmware_ver[2], core_config->firmware_ver[3], core_config->firmware_ver[4]);

	return scnprintf(pBuf, PAGE_SIZE, "%02x-%02x\n", core_config->firmware_ver[2], core_config->firmware_ver[3]);
}
static DEVICE_ATTR(buildid, 0444, drv_build_id_show, NULL);



static const struct attribute *dev_attrs_list[] = {
	&dev_attr_poweron.attr,
	&dev_attr_productinfo.attr,
	&dev_attr_forcereflash.attr,
	&dev_attr_flashprog.attr,
	&dev_attr_doreflash.attr,
	&dev_attr_buildid.attr,
	NULL
};

static ssize_t path_show(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
	ssize_t blen;
	const char *path;

	if(mode_chose == SPI_MODE)
		path = kobject_get_path(&ipd->spi->dev.kobj, GFP_KERNEL);
	else
		path = kobject_get_path(&ipd->client->dev.kobj, GFP_KERNEL);
	blen = scnprintf(pBuf, PAGE_SIZE, "%s\n", path ? path : "na");
	kfree(path);

	return blen;
}

/* Attribute: vendor (RO) */
static ssize_t vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ipio_info("*** %s() vendor = %s ***\n", __func__, "ilitek");
	return scnprintf(buf, PAGE_SIZE, "ilitek");
}


static struct device_attribute touchscreen_attributes[] = {
	__ATTR_RO(path),
	__ATTR_RO(vendor),
	__ATTR_NULL
};

int ilitek_sys_init(void)
{
	int i;
	s32 ret = 0;
	dev_t devno;
	struct device_attribute *attrs = touchscreen_attributes;

	ret = alloc_chrdev_region(&devno, 0, 1, ipd->TP_IC_TYPE);
	if (ret) {
		ipio_err ("can't allocate chrdev\n");
		return ret;
	} else {

		/* set sysfs for firmware */
		touchscreen_class = class_create(THIS_MODULE, "touchscreen");
		if (IS_ERR(touchscreen_class)) {
			ret = PTR_ERR(touchscreen_class);
			touchscreen_class = NULL;
			ipio_err("Failed to create touchscreen class!\n");
			return ret;
		}

		touchscreen_class_dev = device_create(touchscreen_class, NULL, devno, NULL, ipd->TP_IC_TYPE);
		pr_info(" touchscreen_class_dev = %p \n", touchscreen_class_dev);
		if (IS_ERR(touchscreen_class_dev)) {
			ret = PTR_ERR(touchscreen_class_dev);
			touchscreen_class_dev = NULL;
			ipio_err("Failed to create device(touchscreen_class_dev)!\n");
			return ret;
		}
		ipio_info("Succeed to create device(touchscreen_class_dev)!\n");

		for (i = 0; attrs[i].attr.name != NULL; ++i) {
			ret = device_create_file(touchscreen_class_dev, &attrs[i]);
			if (ret < 0)
				goto device_destroy;
		}
		if(mode_chose == SPI_MODE)
			ret = sysfs_create_files(&ipd->spi->dev.kobj, dev_attrs_list);
		else
			ret = sysfs_create_files(&ipd->client->dev.kobj, dev_attrs_list);
		if (ret < 0) {
			ipio_info("Fail to create dev_attrs_list files)!\n");
			goto device_destroy;
		}

		ipio_info("Succeed to sysfs create files)!\n");
	}

	return ret;

device_destroy:
	for (--i; i >= 0; --i)
		device_remove_file(touchscreen_class_dev, &attrs[i]);

	touchscreen_class_dev = NULL;
	class_unregister(touchscreen_class);
	ipio_err("error creating touchscreen class\n");

	return -ENODEV;
}

EXPORT_SYMBOL(ilitek_sys_init);

void ilitek_sys_remove(void)
{
	int i;
	struct device_attribute *attrs = touchscreen_attributes;
	if(mode_chose == SPI_MODE)
		sysfs_remove_files(&ipd->spi->dev.kobj, dev_attrs_list);
	else
		sysfs_remove_files(&ipd->client->dev.kobj, dev_attrs_list);

	for (i = 0; attrs[i].attr.name != NULL; ++i)
		device_remove_file(touchscreen_class_dev, &attrs[i]);


	device_unregister(touchscreen_class_dev);

	class_unregister(touchscreen_class);
}

EXPORT_SYMBOL(ilitek_sys_remove);

/*********** Add sys node for tcmd and fw upgrade  *********************/

