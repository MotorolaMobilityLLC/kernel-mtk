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
/* Firmware data with static array */
#include "ilitek_fw.h"

#define UPDATE_PASS		0
#define UPDATE_FAIL		-1
#define TIMEOUT_SECTOR		500
#define TIMEOUT_PAGE		3500
#define TIMEOUT_PROGRAM		10

static struct touch_fw_data {
	u8 block_number;
	u32 start_addr;
	u32 end_addr;
	u32 new_fw_cb;
	int delay_after_upgrade;
	bool isCRC;
	bool isboot;
	int hex_tag;
} tfd;

static struct flash_block_info {
	char *name;
	u32 start;
	u32 end;
	u32 len;
	u32 mem_start;
	u32 fix_mem_start;
	u8 mode;
} fbi[FW_BLOCK_INFO_NUM];

u8 *pfw;

static u32 HexToDec(char *phex, s32 len)
{
	u32 ret = 0, temp = 0, i;
	s32 shift = (len - 1) * 4;

	for (i = 0; i < len; shift -= 4, i++) {
		if ((phex[i] >= '0') && (phex[i] <= '9'))
			temp = phex[i] - '0';
		else if ((phex[i] >= 'a') && (phex[i] <= 'f'))
			temp = (phex[i] - 'a') + 10;
		else if ((phex[i] >= 'A') && (phex[i] <= 'F'))
			temp = (phex[i] - 'A') + 10;
		else
			return -1;

		ret |= (temp << shift);
	}
	return ret;
}

static int CalculateCRC32(u32 start_addr, u32 len, u8 *pfw)
{
	int i = 0, j = 0;
	int crc_poly = 0x04C11DB7;
	int tmp_crc = 0xFFFFFFFF;

	for (i = start_addr; i < start_addr + len; i++) {
		tmp_crc ^= (pfw[i] << 24);

		for (j = 0; j < 8; j++) {
			if ((tmp_crc & 0x80000000) != 0)
				tmp_crc = tmp_crc << 1 ^ crc_poly;
			else
				tmp_crc = tmp_crc << 1;
		}
	}
	return tmp_crc;
}

void ilitek_fw_dump_iram_data(u32 start, u32 end, bool save)
{
	return;
}

static int ilitek_tddi_fw_check_hex_hw_crc(u8 *pfw)
{
	u32 i = 0, len = 0;
	u32 hex_crc = 0, hw_crc;

	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		if (fbi[i].end == 0)
			continue;

		len = fbi[i].end - fbi[i].start + 1 - 4;

		hex_crc = CalculateCRC32(fbi[i].start, len, pfw);
		hw_crc = ilitek_tddi_fw_read_hw_crc(fbi[i].start, len);

		ipio_debug("Block = %d, Hex CRC = %x, HW CRC = %x\n", i, hex_crc, hw_crc);

		if (hex_crc != hw_crc) {
			ipio_err("Hex and HW CRC NO matched !!!\n");
			return UPDATE_FAIL;
		}
	}

	ipio_info("Hex and HW CRC match!\n");
	return UPDATE_PASS;
}

static int ilitek_tddi_flash_poll_busy(int timer)
{
	int ret = UPDATE_PASS, retry = timer;
	u8 cmd = 0x5;
	u32 temp = 0;

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Pull cs low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, cmd, 1) < 0)
		ipio_err("Write 0x5 cmd failed\n");

	do {
		if (ilitek_ice_mode_write(FLASH2_ADDR, 0xFF, 1) < 0)
			ipio_err("Write dummy failed\n");

		mdelay(1);

		if (ilitek_ice_mode_read(FLASH4_ADDR, &temp, sizeof(u8)) < 0)
			ipio_err("Read flash busy error\n");

		if ((temp & 0x3) == 0)
			break;
	} while (--retry >= 0);

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Pull cs high failed\n");

	if (retry <= 0) {
		ipio_err("Flash polling busy timeout ! tmp = %x\n", temp);
		ret = UPDATE_FAIL;
	}

	return ret;
}

void ilitek_tddi_flash_clear_dma(void)
{
	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_bit_mask_write(FLASH0_ADDR, FLASH0_reg_preclk_sel, (2 << 16)) < 0)
		ipio_err("Write %lu at %x failed\n", FLASH0_reg_preclk_sel, FLASH0_ADDR);

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x01, 1) < 0)
		ipio_err("Pull cs high failed\n");

	if (ilitek_ice_mode_bit_mask_write(FLASH4_ADDR, FLASH4_reg_flash_dma_trigger_en, (0 << 24)) < 0)
		ipio_err("Write %lu at %x failed\n", FLASH4_reg_flash_dma_trigger_en, FLASH4_ADDR);

	if (ilitek_ice_mode_bit_mask_write(FLASH0_ADDR, FLASH0_reg_rx_dual, (0 << 24)) < 0)
		ipio_err("Write %lu at %x failed\n", FLASH0_reg_rx_dual, FLASH0_ADDR);

	if (ilitek_ice_mode_write(FLASH3_reg_rcv_cnt, 0x00, 1) < 0)
		ipio_err("Write 0x0 at %x failed\n", FLASH3_reg_rcv_cnt);

	if (ilitek_ice_mode_write(FLASH4_reg_rcv_data, 0xFF, 1) < 0)
		ipio_err("Write 0xFF at %x failed\n", FLASH4_reg_rcv_data);
}

int ilitek_tddi_flash_read_int_flag(void)
{
	int retry = 500;
	u32 data = 0;

	do {
		if (ilitek_ice_mode_read(INTR1_ADDR & BIT(25), &data, sizeof(u32)) < 0)
			ipio_err("Read flash int flag error\n");

		ipio_debug("int flag = %x\n", data);
		if (data)
			break;
	} while (--retry >= 0);

	if (retry <= 0) {
		ipio_err("Read Flash INT flag timeout !, flag = 0x%x\n", data);
		return -1;
	}
	return 0;
}

void ilitek_tddi_flash_dma_write(u32 start, u32 end, u32 len)
{
	if (ilitek_ice_mode_bit_mask_write(FLASH0_ADDR, FLASH0_reg_preclk_sel, 1 << 16) < 0)
		ipio_err("Write %lu at %x failed\n", FLASH0_reg_preclk_sel, FLASH0_ADDR);

	if (ilitek_ice_mode_write(FLASH0_reg_flash_csb, 0x00, 1) < 0)
		ipio_err("Pull cs low failed\n");

	if (ilitek_ice_mode_write(FLASH1_reg_flash_key1, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_reg_tx_data, 0x0b, 1) < 0)
		ipio_err("Write 0x0b at %x failed\n", FLASH2_reg_tx_data);

	if (ilitek_tddi_flash_read_int_flag() < 0) {
		ipio_err("Write 0xb timeout \n");
		return;
	}

	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_write(FLASH2_reg_tx_data, (start & 0xFF0000) >> 16, 1) < 0)
		ipio_err("Write %x at %x failed\n", (start & 0xFF0000), FLASH2_reg_tx_data);

	if (ilitek_tddi_flash_read_int_flag() < 0) {
		ipio_err("Write addr1 timeout\n");
		return;
	}

	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_write(FLASH2_reg_tx_data, (start & 0x00FF00) >> 8, 1) < 0)
		ipio_err("Write %x at %x failed\n", (start & 0x00FF00) >> 8, FLASH2_reg_tx_data);

	if (ilitek_tddi_flash_read_int_flag() < 0) {
		ipio_err("Write addr2 timeout\n");
		return;
	}

	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_write(FLASH2_reg_tx_data, (start & 0x0000FF), 1) < 0)
		ipio_err("Write %x at %x failed\n", (start & 0x0000FF), FLASH2_reg_tx_data);

	if (ilitek_tddi_flash_read_int_flag() < 0) {
		ipio_err("Write addr3 timeout\n");
		return;
	}

	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_bit_mask_write(FLASH0_ADDR, FLASH0_reg_rx_dual, 0 << 24) < 0)
		ipio_err("Write %lu at %x failed\n", FLASH0_reg_rx_dual, FLASH0_ADDR);

	if (ilitek_ice_mode_write(FLASH2_reg_tx_data, 0x00, 1) < 0)
		ipio_err("Write dummy failed\n");

	if (ilitek_tddi_flash_read_int_flag() < 0) {
		ipio_err("Write dummy timeout\n");
		return;
	}

	if (ilitek_ice_mode_bit_mask_write(INTR1_ADDR, INTR1_reg_flash_int_flag, (1 << 25)) < 0)
		ipio_err("Write %lu at %x failed\n", INTR1_reg_flash_int_flag, INTR1_ADDR);

	if (ilitek_ice_mode_write(FLASH3_reg_rcv_cnt, len, 4) < 0)
		ipio_err("Write length failed\n");
}

static void ilitek_tddi_flash_write_enable(void)
{
	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Pull CS low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0x6, 1) < 0)
		ipio_err("Write 0x6 failed\n");

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Pull CS high failed\n");
}

u32 ilitek_tddi_fw_read_hw_crc(u32 start, u32 end)
{
	int retry = 500;
	u32 busy = 0;
	u32 write_len = end;
	u32 flash_crc = 0;

	if (write_len > idev->chip->max_count) {
		ipio_err("The length (%x) written into firmware is greater than max count (%x)\n",
			write_len, idev->chip->max_count);
		return -1;
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Pull CS low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0x3b, 1) < 0)
		ipio_err("Write 0x3b failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0xFF0000) >> 16, 1) < 0)
		ipio_err("Write address failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0x00FF00) >> 8, 1) < 0)
		ipio_err("Write address failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0x0000FF), 1) < 0)
		ipio_err("Write address failed\n");

	if (ilitek_ice_mode_write(0x041003, 0x01, 1) < 0)
		ipio_err("Write enable Dio_Rx_dual failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0xFF, 1) < 0)
		ipio_err("Write dummy failed\n");

	if (ilitek_ice_mode_write(0x04100C, write_len, 3) < 0)
		ipio_err("Write Set Receive count failed\n");

	if (ilitek_ice_mode_write(0x048007, 0x02, 1) < 0)
		ipio_err("Write clearing int flag failed\n");

	if (ilitek_ice_mode_write(0x041016, 0x00, 1) < 0)
		ipio_err("Write 0x0 at 0x041016 failed\n");

	if (ilitek_ice_mode_write(0x041016, 0x01, 1) < 0)
		ipio_err("Write Checksum_En failed\n");

	if (ilitek_ice_mode_write(FLASH4_ADDR, 0xFF, 1) < 0)
		ipio_err("Write start to receive failed\n");

	do {
		if (ilitek_ice_mode_read(0x048007, &busy, sizeof(u8)) < 0)
			ipio_err("Read busy error\n");

		ipio_debug("busy = %x\n", busy);
		if (((busy >> 1) & 0x01) == 0x01)
			break;
	} while (--retry >= 0);

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Write CS high failed\n");

	if (retry <= 0) {
		ipio_err("Read HW CRC timeout !, busy = 0x%x\n", busy);
		return -1;
	}

	if (ilitek_ice_mode_write(0x041003, 0x0, 1) < 0)
		ipio_err("Write disable dio_Rx_dual failed\n");

	if (ilitek_ice_mode_read(0x04101C, &flash_crc, sizeof(u32)) < 0) {
		ipio_err("Read hw crc error\n");
		return -1;
	}

	return flash_crc;
}

int ilitek_tddi_fw_read_flash_data(u32 start, u32 end, u8 *data, int len)
{
	u32 i, index = 0, precent;
	u32 tmp;

	if (end - start > len) {
		ipio_err("the length (%d) reading crc is over than len(%d)\n", end - start, len);
		return -1;
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Write cs low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0x03, 1) < 0)
		ipio_err("Write 0x3 failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0xFF0000) >> 16, 1) < 0)
		ipio_err("Write address failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0x00FF00) >> 8, 1) < 0)
		ipio_err("Write address failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, (start & 0x0000FF), 1) < 0)
		ipio_err("Write address failed\n");

	for (i = start; i <= end; i++) {
		if (ilitek_ice_mode_write(FLASH2_ADDR, 0xFF, 1) < 0)
			ipio_err("Write dummy failed\n");

		if (ilitek_ice_mode_read(FLASH4_ADDR, &tmp, sizeof(u8)) < 0)
			ipio_err("Read flash data error!\n");

		data[index] = tmp;
		index++;
		precent = (i * 100) / end;
		ipio_debug("Reading flash data .... %d%c", precent, '%');
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Write cs high failed\n");

	return 0;
}

int ilitek_tddi_fw_dump_flash_data(u32 start, u32 end, bool user)
{
	struct file *f = NULL;
	u8 *buf = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	u32 start_addr, end_addr;
	int ret, length;

	f = filp_open(DUMP_FLASH_PATH, O_WRONLY | O_CREAT | O_TRUNC, 644);
	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open the file at %ld.\n", PTR_ERR(f));
		return -1;
	}

	ret = ilitek_ice_mode_ctrl(ENABLE, OFF);
	if (ret < 0)
		return ret;

	if (user) {
		start_addr = 0x0;
		end_addr = 0x1FFFF;
	} else {
		start_addr = start;
		end_addr = end;
	}

	length = end_addr - start_addr + 1;
	ipio_info("len = %d\n", length);

	buf = vmalloc(length * sizeof(u8));
	if (ERR_ALLOC_MEM(buf)) {
		ipio_err("Failed to allocate buf memory, %ld\n", PTR_ERR(buf));
		filp_close(f, NULL);
		ilitek_ice_mode_ctrl(DISABLE, OFF);
		return -1;
	}

	ilitek_tddi_fw_read_flash_data(start_addr, end_addr, buf, length);

	old_fs = get_fs();
	set_fs(get_ds());
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, buf, length, &pos);
	set_fs(old_fs);
	filp_close(f, NULL);
	ipio_vfree((void **)&buf);
	ilitek_ice_mode_ctrl(DISABLE, OFF);
	ipio_info("dump flash success\n");
	return 0;
}

static void ilitek_tddi_flash_protect(bool enable)
{
	ipio_info("%s flash protection\n", enable ? "Enable" : "Disable");

	ilitek_tddi_flash_write_enable();

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Write cs low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0x1, 1) < 0)
		ipio_err("Write 0x1 failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, 0x0, 1) < 0)
		ipio_err("Write 0x0 failed\n");

	switch (idev->flash_mid) {
	case 0xEF:
		if (idev->flash_devid == 0x6012 || idev->flash_devid == 0x6011) {
			if (enable) {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0x7E, 1) < 0)
					ipio_err("Write 0x7E at %x failed\n", FLASH2_ADDR);
			} else {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0x0, 1) < 0)
					ipio_err("Write 0x0 at %x failed\n", FLASH2_ADDR);
			}
		}
		break;
	case 0xC8:
		if (idev->flash_devid == 0x6012 || idev->flash_devid == 0x6013) {
			if (enable) {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0x7A, 1) < 0)
					ipio_err("Write 0x7A at %x failed\n", FLASH2_ADDR);
			} else {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0x0, 1) < 0)
					ipio_err("Write 0x0 at %x failed\n", FLASH2_ADDR);
			}

		}
		break;
	default:
		ipio_err("Can't find flash id(0x%x), ignore protection\n", idev->flash_mid);
		break;
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Write cs high failed\n");
}

static int ilitek_tddi_fw_check_ver(u8 *pfw)
{
	int i, len = 0, crc_byte_len = 4;
	u8 flash_crc[4] = {0};
	u32 start_addr = 0, end_addr = 0;
	u32 hw_crc[FW_BLOCK_INFO_NUM];
	u32 flash_crc_cb = 0, hex_crc = 0;

	/* Check Flash and HW CRC */
	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		start_addr = fbi[i].start;
		end_addr = fbi[i].end;

		/* Invaild end address */
		if (end_addr == 0)
			continue;

		if (ilitek_tddi_fw_read_flash_data(end_addr - crc_byte_len + 1, end_addr,
					flash_crc, sizeof(flash_crc)) < 0) {
			ipio_err("Read Flash failed\n");
			return UPDATE_FAIL;
		}

		flash_crc_cb = flash_crc[0] << 24 | flash_crc[1] << 16 | flash_crc[2] << 8 | flash_crc[3];

		hw_crc[i] = ilitek_tddi_fw_read_hw_crc(start_addr, end_addr - start_addr - crc_byte_len + 1);

		ipio_info("Block = %d, HW CRC = 0x%06x, Flash CRC = 0x%06x\n", i, hw_crc[i], flash_crc_cb);

		/* Compare Flash CRC with HW CRC */
		if (flash_crc_cb != hw_crc[i]) {
			ipio_info("HW and Flash CRc not matched, do upgrade\n");
			return UPDATE_FAIL;
		}
		memset(flash_crc, 0, sizeof(flash_crc));
	}

	/* Check FW version */
	ipio_info("New FW ver = 0x%x, Current FW ver = 0x%x\n", tfd.new_fw_cb, idev->chip->fw_ver);
	if (tfd.new_fw_cb != idev->chip->fw_ver) {
		ipio_info("FW version not matched, do upgrade\n");
		return UPDATE_FAIL;
	} else {
		/* Check Hex and HW CRC */
		for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
			if (fbi[i].end == 0)
				continue;

			len = fbi[i].end - fbi[i].start + 1 - 4;
			hex_crc = CalculateCRC32(fbi[i].start, len, pfw);
			ipio_info("Block = %d, HW CRC = 0x%06x, Hex CRC = 0x%06x\n", i, hw_crc[i], hex_crc);
			if (hex_crc != hw_crc[i]) {
				ipio_err("Hex and HW CRC not matched, do upgrade\n");
				return UPDATE_FAIL;
			}
		}
	}
	ipio_info("Firmware is the newest version, upgrade abort\n");
	return UPDATE_PASS;
}

static int ilitek_tddi_fw_flash_program(u8 *pfw)
{
	u8 buf[512] = {0};
	u32 i = 0, addr = 0, k = 0, recv_addr = 0;
	bool skip = true;

	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		if (fbi[i].end == 0)
			continue;

		if (fbi[i].start >= RESERVE_BLOCK_START_ADDR &&
			fbi[i].end <= RESERVE_BLOCK_END_ADDR)
			continue;

		ipio_info("Block[%d]: Programing from (0x%x) to (0x%x)\n", i, fbi[i].start, fbi[i].end);

		for (addr = fbi[i].start; addr < fbi[i].end; addr += idev->program_page) {
			buf[0] = 0x25;
			buf[3] = 0x04;
			buf[2] = 0x10;
			buf[1] = 0x08;

			for (k = 0; k < idev->program_page; k++) {
				if (addr + k <= tfd.end_addr)
					buf[4 + k] = pfw[addr + k];
				else
					buf[4 + k] = 0xFF;

				if (buf[4 + k] != 0xFF)
					skip = false;
			}

			if (skip) {
				if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
					ipio_err("Write cs high failed\n");
				return UPDATE_FAIL;
			}

			ilitek_tddi_flash_write_enable();

			if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
				ipio_err("Write cs low failed\n");

			if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
				ipio_err("Write key failed\n");

			if (ilitek_ice_mode_write(FLASH2_ADDR, 0x2, 1) < 0)
				ipio_err("Write 0x2 failed\n");

			recv_addr = ((addr & 0xFF0000) >> 16) | (addr & 0x00FF00) | ((addr & 0x0000FF) << 16);
			if (ilitek_ice_mode_write(FLASH2_ADDR, recv_addr, 3) < 0)
				ipio_err("Write address failed\n");

			if (idev->write(buf, idev->program_page + 4) < 0) {
				ipio_err("Failed to program data at start_addr = 0x%X, k = 0x%X, addr = 0x%x\n",
				addr, k, addr + k);
				if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
					ipio_err("Write cs high failed\n");
				return UPDATE_FAIL;
			}

			if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
				ipio_err("Write cs high failed\n");

			if (idev->flash_mid == 0xEF) {
				mdelay(1);
			} else {
				if (ilitek_tddi_flash_poll_busy(TIMEOUT_PROGRAM) < 0)
					return UPDATE_FAIL;
			}

			/* holding the status until finish this upgrade. */
			idev->fw_update_stat = (addr * 101) / tfd.end_addr;
			if (idev->fw_update_stat > 90)
				idev->fw_update_stat = 90;
		}
	}
	return UPDATE_PASS;
}

static int ilitek_tddi_fw_flash_erase(void)
{
	int ret = 0;
	u32 i = 0, addr = 0, recv_addr = 0;

	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		if (fbi[i].end == 0)
			continue;

		if (fbi[i].start >= RESERVE_BLOCK_START_ADDR &&
			fbi[i].end <= RESERVE_BLOCK_END_ADDR)
			continue;

		ipio_info("Block[%d]: Erasing from (0x%x) to (0x%x) \n", i, fbi[i].start, fbi[i].end);

		for (addr = fbi[i].start; addr <= fbi[i].end; addr += idev->flash_sector) {
			ilitek_tddi_flash_write_enable();

			if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
				ipio_err("Write cs low failed\n");

			if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
				ipio_err("Write key failed\n");

			if (addr == fbi[AP].start) {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0xD8, 1) < 0)
					ipio_err("Write 0xB at %x failed\n", FLASH2_ADDR);
			} else {
				if (ilitek_ice_mode_write(FLASH2_ADDR, 0x20, 1) < 0)
					ipio_err("Write 0x20 at %x failed\n", FLASH2_ADDR);
			}

			recv_addr = ((addr & 0xFF0000) >> 16) | (addr & 0x00FF00) | ((addr & 0x0000FF) << 16);
			if (ilitek_ice_mode_write(FLASH2_ADDR, recv_addr, 3) < 0)
				ipio_err("Write address failed\n");

			if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
				ipio_err("Write cs high failed\n");

			/* Waitint for flash setting ready */
			mdelay(1);

			if (addr == fbi[AP].start)
				ret = ilitek_tddi_flash_poll_busy(TIMEOUT_PAGE);
			else
				ret = ilitek_tddi_flash_poll_busy(TIMEOUT_SECTOR);

			if (ret < 0)
				return UPDATE_FAIL;

			if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
				ipio_err("Write cs high failed\n");

			if (fbi[i].start == fbi[AP].start)
				break;
		}
	}
	return UPDATE_PASS;
}

static int ilitek_tddi_fw_flash_upgrade(u8 *pfw)
{
	int ret = UPDATE_PASS;

	if (ilitek_tddi_reset_ctrl(idev->reset) < 0)
		ipio_err("TP reset failed during flash progam\n");

	/* Get current fw version before comparing. */
	idev->info_from_hex = DISABLE;
	if (ilitek_tddi_ic_get_fw_ver() < 0)
		ipio_err("Get firmware ver failed before upgrade\n");
	idev->info_from_hex = ENABLE;

	ret = ilitek_ice_mode_ctrl(ENABLE, OFF);
	if (ret < 0)
		return UPDATE_FAIL;

	ret = ilitek_tddi_ic_watch_dog_ctrl(ILI_WRITE, DISABLE);
	if (ret < 0)
		return ret;

	ret = ilitek_tddi_fw_check_ver(pfw);
	if (ret == UPDATE_PASS) {
		if (ilitek_ice_mode_ctrl(DISABLE, OFF) < 0) {
			ipio_err("Disable ice mode failed, call reset instead\n");
			if (ilitek_tddi_reset_ctrl(idev->reset) < 0) {
				ipio_err("TP reset failed during flash progam\n");
				return UPDATE_FAIL;
			}
			return UPDATE_PASS;
		}
		return UPDATE_PASS;
	}

	ret = ilitek_tddi_fw_flash_erase();
	if (ret == UPDATE_FAIL)
		return UPDATE_FAIL;

	ret = ilitek_tddi_fw_flash_program(pfw);
	if (ret == UPDATE_FAIL)
		return UPDATE_FAIL;

	ret = ilitek_tddi_fw_check_hex_hw_crc(pfw);
	if (ret == UPDATE_FAIL)
		return UPDATE_FAIL;

	/* We do have to reset chip in order to move new code from flash to iram. */
	if (ilitek_tddi_reset_ctrl(idev->reset) < 0)
		ipio_err("TP reset failed after flash progam\n");

	return ret;
}

static int ilitek_fw_calc_file_crc(u8 *pfw)
{
	int i;
	u32 ex_addr, data_crc, file_crc;

	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		if (fbi[i].end == 0)
			continue;
		ex_addr = fbi[i].end;
		data_crc = CalculateCRC32(fbi[i].start, fbi[i].len - 4, pfw);
		file_crc = pfw[ex_addr - 3] << 24 | pfw[ex_addr - 2] << 16 | pfw[ex_addr - 1] << 8 | pfw[ex_addr];
		ipio_info("data crc = %x, file crc = %x\n", data_crc, file_crc);
		if (data_crc != file_crc) {
			ipio_err("Content of fw file is broken. (%d, %x, %x)\n",
				i, data_crc, file_crc);
			return -1;
		}
	}

	ipio_info("Content of fw file is correct\n");
	return 0;
}

static void ilitek_tddi_fw_update_block_info(u8 *pfw)
{
	u32 fw_info_addr = 0;

	fbi[AP].name = "AP";
	fbi[DATA].name = "DATA";
	fbi[TUNING].name = "TUNING";
	fbi[MP].name = "MP";
	fbi[GESTURE].name = "GESTURE";

	/* upgrade mode define */
	fbi[DATA].mode = fbi[AP].mode = fbi[TUNING].mode = AP;
	fbi[MP].mode = MP;
	fbi[GESTURE].mode = GESTURE;

	/* copy fw info */
	fw_info_addr = fbi[AP].end - INFO_HEX_ST_ADDR;
	ipio_info("Parsing hex info start addr = 0x%x\n", fw_info_addr);
	ipio_memcpy(idev->fw_info, pfw + fw_info_addr, sizeof(idev->fw_info), sizeof(idev->fw_info));

	/* Get hex fw vers */
	tfd.new_fw_cb = (idev->fw_info[44] << 24) | (idev->fw_info[45] << 16) |
			(idev->fw_info[46] << 8) | idev->fw_info[47];

	/* Calculate update address */
	ipio_info("New FW ver = 0x%x\n", tfd.new_fw_cb);
	ipio_info("star_addr = 0x%06X, end_addr = 0x%06X, Block Num = %d\n", tfd.start_addr, tfd.end_addr, tfd.block_number);
}

static int ilitek_tddi_fw_ili_convert(u8 *pfw)
{
	int i = 0, block_enable = 0, num = 0;
	u8 block;
	u32 Addr;

	if (sizeof(CTPM_FW) < ILI_FILE_HEADER) {
		ipio_err("size of ILI file is invalid\n");
		return -EINVAL;
	}

	ipio_info("Start to parse ILI file, type = %d, block_count = %d\n", CTPM_FW[32], CTPM_FW[33]);

	memset(fbi, 0x0, sizeof(fbi));

	tfd.start_addr = 0;
	tfd.end_addr = 0;
	tfd.hex_tag = 0;

	block_enable = CTPM_FW[32];

	if (block_enable == 0) {
		tfd.hex_tag = BLOCK_TAG_AE;
		goto out;
	}

	tfd.hex_tag = BLOCK_TAG_AF;
	for (i = 0; i < FW_BLOCK_INFO_NUM; i++) {
		if (((block_enable >> i) & 0x01) == 0x01) {
			num = i + 1;

			if (num > (FW_BLOCK_INFO_NUM - 1)) {
				ipio_err("ERROR! block num is larger than its define (%d, %d)\n",
						num, FW_BLOCK_INFO_NUM - 1);
				return -EINVAL;
			}

			if ((num) == 6) {
				fbi[num].start = (CTPM_FW[0] << 16) + (CTPM_FW[1] << 8) + (CTPM_FW[2]);
				fbi[num].end = (CTPM_FW[3] << 16) + (CTPM_FW[4] << 8) + (CTPM_FW[5]);
				fbi[num].fix_mem_start = INT_MAX;
			} else {
				fbi[num].start = (CTPM_FW[34 + i * 6] << 16) + (CTPM_FW[35 + i * 6] << 8) + (CTPM_FW[36 + i * 6]);
				fbi[num].end = (CTPM_FW[37 + i * 6] << 16) + (CTPM_FW[38 + i * 6] << 8) + (CTPM_FW[39 + i * 6]);
				fbi[num].fix_mem_start = INT_MAX;
			}

			if (fbi[num].start == fbi[num].end)
				continue;

			fbi[num].len = fbi[num].end - fbi[num].start + 1;
			ipio_info("Block[%d]: start_addr = %x, end = %x\n", num, fbi[num].start, fbi[num].end);

			if (num == GESTURE)
				idev->gesture_load_code = true;
		}
	}

	if ((block_enable & 0x80) == 0x80) {
		for (i = 0; i < 3; i++) {
			Addr = (CTPM_FW[6 + i * 4] << 16) + (CTPM_FW[7 + i * 4] << 8) + (CTPM_FW[8 + i * 4]);
			block = CTPM_FW[9 + i * 4];

			if ((block != 0) && (Addr != 0x000000)) {
				fbi[block].fix_mem_start = Addr;
				ipio_info("Tag 0xB0: change Block[%d] to addr = 0x%x\n", block, fbi[block].fix_mem_start);
			}
		}
	}

out:
	memcpy(pfw, CTPM_FW + ILI_FILE_HEADER, (sizeof(CTPM_FW) - ILI_FILE_HEADER));

	if (ilitek_fw_calc_file_crc(pfw) < 0)
		return -1;

	tfd.block_number = CTPM_FW[33];
	tfd.end_addr = (sizeof(CTPM_FW) - ILI_FILE_HEADER);
	return 0;
}

static int ilitek_tddi_fw_hex_convert(u8 *phex, int size, u8 *pfw)
{
	int block = 0;
	u32 i = 0, j = 0, k = 0, num = 0;
	u32 len = 0, addr = 0, type = 0;
	u32 start_addr = 0x0, end_addr = 0x0, ex_addr = 0;
	u32 offset;

	memset(fbi, 0x0, sizeof(fbi));

	/* Parsing HEX file */
	for (; i < size;) {
		len = HexToDec(&phex[i + 1], 2);
		addr = HexToDec(&phex[i + 3], 4);
		type = HexToDec(&phex[i + 7], 2);

		if (type == 0x04) {
			ex_addr = HexToDec(&phex[i + 9], 4);
		} else if (type == 0x02) {
			ex_addr = HexToDec(&phex[i + 9], 4);
			ex_addr = ex_addr >> 12;
		} else if (type == BLOCK_TAG_AE || type == BLOCK_TAG_AF) {
			/* insert block info extracted from hex */
			tfd.hex_tag = type;
			if (tfd.hex_tag == BLOCK_TAG_AF)
				num = HexToDec(&phex[i + 9 + 6 + 6], 2);
			else
				num = block;

			if (num > (FW_BLOCK_INFO_NUM - 1)) {
				ipio_err("ERROR! block num is larger than its define (%d, %d)\n",
						num, FW_BLOCK_INFO_NUM - 1);
				return -EINVAL;
			}

			fbi[num].start = HexToDec(&phex[i + 9], 6);
			fbi[num].end = HexToDec(&phex[i + 9 + 6], 6);
			fbi[num].fix_mem_start = INT_MAX;
			fbi[num].len = fbi[num].end - fbi[num].start + 1;
			ipio_info("Block[%d]: start_addr = %x, end = %x", num, fbi[num].start, fbi[num].end);

			if (num == GESTURE)
				idev->gesture_load_code = true;

			block++;
		} else if (type == BLOCK_TAG_B0 && tfd.hex_tag == BLOCK_TAG_AF) {
			num = HexToDec(&phex[i + 9 + 6], 2);

			if (num > (FW_BLOCK_INFO_NUM - 1)) {
				ipio_err("ERROR! block num is larger than its define (%d, %d)\n",
						num, FW_BLOCK_INFO_NUM - 1);
				return -EINVAL;
			}

			fbi[num].fix_mem_start = HexToDec(&phex[i + 9], 6);
			ipio_info("Tag 0xB0: change Block[%d] to addr = 0x%x\n", num, fbi[num].fix_mem_start);
		}

		addr = addr + (ex_addr << 16);

		if (phex[i + 1 + 2 + 4 + 2 + (len * 2) + 2] == 0x0D)
			offset = 2;
		else
			offset = 1;

		if (addr > MAX_HEX_FILE_SIZE) {
			ipio_err("Invalid hex format %d\n", addr);
			return -1;
		}

		if (type == 0x00) {
			end_addr = addr + len;
			if (addr < start_addr)
				start_addr = addr;
			/* fill data */
			for (j = 0, k = 0; j < (len * 2); j += 2, k++)
				pfw[addr + k] = HexToDec(&phex[i + 9 + j], 2);
		}
		i += 1 + 2 + 4 + 2 + (len * 2) + 2 + offset;
	}

	if (ilitek_fw_calc_file_crc(pfw) < 0)
		return -1;

	tfd.start_addr = start_addr;
	tfd.end_addr = end_addr;
	tfd.block_number = block;
	return 0;
}

static int ilitek_tdd_fw_hex_open(u8 op, u8 *pfw)
{
	int fsize = 0;
	const struct firmware *fw = NULL;
	struct file *f = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;

	ipio_info("Open file method = %s, path = %s\n",
		op ? "FILP_OPEN" : "REQUEST_FIRMWARE",
		op ? UPDATE_FW_FILP_PATH : UPDATE_FW_REQUEST_PATH);

	switch (op) {
	case REQUEST_FIRMWARE:
		if (request_firmware(&fw, UPDATE_FW_REQUEST_PATH, idev->dev) < 0) {
			ipio_err("Request firmware failed\n");
			goto convert_hex;
		}

		fsize = fw->size;
		ipio_info("fsize = %d\n", fsize);
		if (fsize <= 0) {
			ipio_err("The size of file is zero\n");
			release_firmware(fw);
			goto convert_hex;
		}

		ipio_vfree((void **)&(idev->tp_fw.data));
		idev->tp_fw.size = 0;
		idev->tp_fw.data = vmalloc(fsize);
		if (!idev->tp_fw.data) {
			ipio_err("Failed to allocate tp_fw by vmalloc, try again\n");
			idev->tp_fw.data = vmalloc(fsize);
			if (!idev->tp_fw.data) {
				ipio_err("Failed to allocate tp_fw after retry\n");
				return -ENOMEM;
			}
		}

		/* Copy fw data got from request_firmware to global */
		ipio_memcpy((u8 *)idev->tp_fw.data, fw->data, fsize * sizeof(*fw->data), fsize);
		idev->tp_fw.size = fsize;
		release_firmware(fw);
		break;
	case FILP_OPEN:
		f = filp_open(UPDATE_FW_FILP_PATH, O_RDONLY, 0644);
		if (ERR_ALLOC_MEM(f)) {
			ipio_err("Failed to open the file at %ld, try to load it from tp_fw\n", PTR_ERR(f));
			goto convert_hex;
		}

		fsize = f->f_inode->i_size;
		ipio_info("fsize = %d\n", fsize);
		if (fsize <= 0) {
			ipio_err("The size of file is invaild, try to load it from tp_fw\n");
			filp_close(f, NULL);
			goto convert_hex;
		}

		ipio_vfree((void **)&(idev->tp_fw.data));
		idev->tp_fw.size = 0;
		idev->tp_fw.data = vmalloc(fsize);
		if (idev->tp_fw.data == NULL) {
			ipio_err("Failed to allocate tp_fw by vmalloc, try again\n");
			idev->tp_fw.data = vmalloc(fsize);
			if (idev->tp_fw.data == NULL) {
				ipio_err("Failed to allocate tp_fw after retry\n");
				filp_close(f, NULL);
				return -ENOMEM;
			}
		}

		/* ready to map user's memory to obtain data by reading files */
		old_fs = get_fs();
		set_fs(get_ds());
		set_fs(KERNEL_DS);
		pos = 0;
		vfs_read(f, (u8 *)idev->tp_fw.data, fsize, &pos);
		set_fs(old_fs);
		filp_close(f, NULL);
		idev->tp_fw.size = fsize;
		break;
	default:
		ipio_err("Unknown open file method, %d\n", op);
		break;
	}

convert_hex:
	/* Convert hex and copy data from tp_fw.data to pfw */
	if (!ERR_ALLOC_MEM(idev->tp_fw.data) && idev->tp_fw.size > 0) {
		if (ilitek_tddi_fw_hex_convert((u8 *)idev->tp_fw.data, idev->tp_fw.size, pfw) < 0) {
			ipio_err("Convert hex file failed\n");
			return -1;
		}
	} else {
		ipio_err("tp_fw is NULL or fsize(%d) is zero\n", fsize);
		return -1;
	}
	return 0;
}

int ilitek_tddi_fw_upgrade(int op)
{
	int ret = 0, retry = 3;

	if (!idev->boot || idev->force_fw_update) {
		ipio_vfree((void **)&pfw);

		pfw = vmalloc(MAX_HEX_FILE_SIZE * sizeof(u8));
		if (ERR_ALLOC_MEM(pfw)) {
			ipio_err("Failed to allocate pfw memory, %ld\n", PTR_ERR(pfw));
			ret = -ENOMEM;
			goto out;
		}

		memset(pfw, 0xFF, MAX_HEX_FILE_SIZE * sizeof(u8));

		if (ilitek_tdd_fw_hex_open(op, pfw) < 0) {
			ipio_err("Open hex file fail, try upgrade from ILI file\n");
			if (ilitek_tddi_fw_ili_convert(pfw) < 0) {
				ipio_err("Convert ILI file error\n");
				ret = UPDATE_FAIL;
				goto out;
			}
		}
		ilitek_tddi_fw_update_block_info(pfw);
	}

	do {
		ret = ilitek_tddi_fw_flash_upgrade(pfw);
		if (ret == UPDATE_PASS)
			break;

		ipio_err("Upgrade failed, do retry!\n");
	} while (--retry > 0);

	if (ret != UPDATE_PASS) {
		ipio_err("Failed to upgrade fw %d times, erasing flash\n", retry);
		if (ilitek_ice_mode_ctrl(ENABLE, OFF) < 0)
			ipio_err("Enable ice mode failed while erasing flash\n");
		if (ilitek_tddi_fw_flash_erase() < 0)
			ipio_err("Failed to erase flash\n");
		if (ilitek_ice_mode_ctrl(DISABLE, OFF) < 0)
			ipio_err("Disable ice mode failed after erase flash\n");
		if (ilitek_tddi_reset_ctrl(idev->reset) < 0)
			ipio_err("TP reset failed after erase flash\n");
	}

out:
	if (ret < 0)
		idev->info_from_hex = DISABLE;

	ilitek_tddi_ic_get_core_ver();
	ilitek_tddi_ic_get_protocl_ver();
	ilitek_tddi_ic_get_fw_ver();
	ilitek_tddi_ic_get_tp_info();
	ilitek_tddi_ic_get_panel_info();

	if (!idev->info_from_hex)
		idev->info_from_hex = ENABLE;

	return ret;
}

struct flash_table {
	u16 mid;
	u16 dev_id;
	int mem_size;
	int program_page;
	int sector;
} flashtab[] = {
	[0] = {0x00, 0x0000, (256 * K), 256, (4 * K)}, /* Default */
	[1] = {0xEF, 0x6011, (128 * K), 256, (4 * K)}, /* W25Q10EW	*/
	[2] = {0xEF, 0x6012, (256 * K), 256, (4 * K)}, /* W25Q20EW	*/
	[3] = {0xC8, 0x6012, (256 * K), 256, (4 * K)}, /* GD25LQ20B */
	[4] = {0xC8, 0x6013, (512 * K), 256, (4 * K)}, /* GD25LQ40 */
	[5] = {0x85, 0x6013, (4 * M), 256, (4 * K)},
	[6] = {0xC2, 0x2812, (256 * K), 256, (4 * K)},
	[7] = {0x1C, 0x3812, (256 * K), 256, (4 * K)},
};

void ilitek_tddi_fw_read_flash_info(void)
{
	int i = 0, size;
	u8 buf[4] = {0};
	u8 cmd = 0x9F;
	u32 tmp = 0;
	u16 flash_id = 0, flash_mid = 0;
	bool ice = atomic_read(&idev->ice_stat);

	if (!ice) {
		if (ilitek_ice_mode_ctrl(ENABLE, OFF) < 0)
			ipio_err("Enable ice mode failed while reading flash info\n");
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x0, 1) < 0)
		ipio_err("Write cs low failed\n");

	if (ilitek_ice_mode_write(FLASH1_ADDR, 0x66aa55, 3) < 0)
		ipio_err("Write key failed\n");

	if (ilitek_ice_mode_write(FLASH2_ADDR, cmd, 1) < 0)
		ipio_err("Write 0x9F failed\n");
	size = ARRAY_SIZE(buf);
	for (i = 0; i < size; i++) {
		if (ilitek_ice_mode_write(FLASH2_ADDR, 0xFF, 1) < 0)
			ipio_err("Write dummy failed\n");

		if (ilitek_ice_mode_read(FLASH4_ADDR, &tmp, sizeof(u8)) < 0)
			ipio_err("Read flash info error\n");

		buf[i] = tmp;
	}

	if (ilitek_ice_mode_write(FLASH_BASED_ADDR, 0x1, 1) < 0)
		ipio_err("Write cs high failed\n"); /* CS high */

	flash_mid = buf[0];
	flash_id = buf[1] << 8 | buf[2];

	size = ARRAY_SIZE(flashtab);
	for (i = 0; i < size; i++) {
		if (flash_mid == flashtab[i].mid && flash_id == flashtab[i].dev_id) {
			idev->flash_mid = flashtab[i].mid;
			idev->flash_devid = flashtab[i].dev_id;
			idev->program_page = flashtab[i].program_page;
			idev->flash_sector = flashtab[i].sector;
			break;
		}
	}

	if (i >= ARRAY_SIZE(flashtab)) {
		ipio_info("Not found flash id in tab, use default\n");
		idev->flash_mid = flashtab[0].mid;
		idev->flash_devid = flashtab[0].dev_id;
		idev->program_page = flashtab[0].program_page;
		idev->flash_sector = flashtab[0].sector;
	}

	ipio_info("Flash MID = %x, Flash DEV_ID = %x\n", idev->flash_mid, idev->flash_devid);
	ipio_info("Flash program page = %d\n", idev->program_page);
	ipio_info("Flash sector = %d\n", idev->flash_sector);

	ilitek_tddi_flash_protect(DISABLE);

	if (!ice) {
		if (ilitek_ice_mode_ctrl(DISABLE, OFF) < 0)
			ipio_err("Disable ice mode failed while reading flash info\n");
	}
}