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
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/fd.h>
#include <linux/file.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/firmware.h>

#include "../common.h"
#include "../platform.h"
#include "config.h"
#include "i2c.h"
#include "firmware.h"
#include "flash.h"
#include "protocol.h"
#include "finger_report.h"
#include "gesture.h"
#include "mp_test.h"

#define CHECK_FW_FAIL	-1
#define UPDATE_FAIL		-1
#define NEED_UPDATE		 1
#define NO_NEED_UPDATE 	 0
#define UPDATE_OK		 0
#define FW_VER_ADDR	   0xFFE0
#define TIMEOUT_SECTOR	 500
#define TIMEOUT_PAGE	 3500
#define TIMEOUT_PROGRAM	 10
#define CRC_ONESET(X, Y)	({Y = (*(X+0) << 24) | (*(X+1) << 16) | (*(X+2) << 8) | (*(X+3)); })

struct flash_sector *g_flash_sector;
struct flash_block_info fbi[FW_BLOCK_INFO_NUM];
struct core_firmware_data *core_firmware;
u8 gestrue_fw[(10 * K)];
bool first_int = false;
extern unsigned char g_user_buf[PAGE_SIZE];
extern unsigned char fw_name_buf[PAGE_SIZE];
extern bool use_g_user_buf;
extern is_force_upgrade;
extern bool is_lcd_resume;
int ilitek_fw_fsize = 0;
uint8_t *ilitek_hex_buffer = NULL;
static int convert_hex_file(u8 *phex, uint32_t nSize, u8 *pfw);

static uint32_t HexToDec(char *pHex, int32_t nLength)
{
	uint32_t nRetVal = 0, nTemp = 0, i;
	int32_t nShift = (nLength - 1) * 4;

	for (i = 0; i < nLength; nShift -= 4, i++) {
		if ((pHex[i] >= '0') && (pHex[i] <= '9')) {
			nTemp = pHex[i] - '0';
		} else if ((pHex[i] >= 'a') && (pHex[i] <= 'f')) {
			nTemp = (pHex[i] - 'a') + 10;
		} else if ((pHex[i] >= 'A') && (pHex[i] <= 'F')) {
			nTemp = (pHex[i] - 'A') + 10;
		} else {
			return -1;
		}

		nRetVal |= (nTemp << nShift);
	}

	return nRetVal;
}

static int write_download(uint32_t start, uint32_t size, uint8_t *w_buf, uint32_t w_len)
{
	int addr = 0, i = 0, update_status = 0, j = 0;
	uint32_t end = start + size;
	uint8_t *buf = NULL;

    buf = (uint8_t *)vmalloc(sizeof(uint8_t) * w_len + 4);

	if (ERR_ALLOC_MEM(buf)) {
		ipio_err("Failed to allocate a buffer to be read, %ld\n", PTR_ERR(buf));
		return -ENOMEM;
	}

	memset(buf, 0xFF, (int)sizeof(uint8_t) * w_len + 4);

	for (addr = start, i = 0; addr < end; addr += w_len, i += w_len) {
		if ((addr + w_len) > end)
			w_len = end % w_len;

		buf[0] = 0x25;
		buf[3] = (char)((addr & 0x00FF0000) >> 16);
		buf[2] = (char)((addr & 0x0000FF00) >> 8);
		buf[1] = (char)((addr & 0x000000FF));

		for (j = 0; j < w_len; j++)
			buf[4 + j] = w_buf[i + j];

		if (core_write(core_config->slave_i2c_addr, buf, w_len + 4)) {
			ipio_err("Failed to write data via SPI in host download (%x)\n", w_len);
			ipio_vfree((void **)&buf);
			return -EIO;
		}

		update_status = (i * 101) / end;
	}

	ipio_vfree((void **)&buf);
	return 0;
}

static void fw_upgrade_info_setting(u8 *pfw, u8 type)
{
	uint32_t  ges_info_addr, ges_fw_start, ges_fw_end;

	if (type == UPGRADE_IRAM) {
		if (core_firmware->hex_tag == BLOCK_TAG_AF) {
			fbi[AP].mem_start = (fbi[AP].fix_mem_start != INT_MAX) ? fbi[AP].fix_mem_start : 0;
			fbi[DATA].mem_start = (fbi[DATA].fix_mem_start != INT_MAX) ? fbi[DATA].fix_mem_start : DLM_START_ADDRESS;
			fbi[TUNING].mem_start = (fbi[TUNING].fix_mem_start != INT_MAX) ? fbi[TUNING].fix_mem_start :  fbi[DATA].mem_start + fbi[DATA].len;
			fbi[MP].mem_start = (fbi[MP].fix_mem_start != INT_MAX) ? fbi[MP].fix_mem_start :  0;
			fbi[GESTURE].mem_start = (fbi[GESTURE].fix_mem_start != INT_MAX) ? fbi[GESTURE].fix_mem_start :  0;

			/* Parsing gesture info form AP code */
			ges_info_addr = (fbi[AP].end + 1 - 60);
			core_gesture->area_section = (pfw[ges_info_addr + 3] << 24) + (pfw[ges_info_addr + 2] << 16) + (pfw[ges_info_addr + 1] << 8) + pfw[ges_info_addr];
			fbi[GESTURE].mem_start = (pfw[ges_info_addr + 7] << 24) + (pfw[ges_info_addr + 6] << 16) + (pfw[ges_info_addr + 5] << 8) + pfw[ges_info_addr + 4];
			fbi[GESTURE].len = MAX_GESTURE_FIRMWARE_SIZE;
			ges_fw_start = (pfw[ges_info_addr + 15] << 24) + (pfw[ges_info_addr + 14] << 16) + (pfw[ges_info_addr + 13] << 8) + pfw[ges_info_addr + 12];
			ges_fw_end = fbi[GESTURE].end;
			fbi[GESTURE].start = 0;
		} else {
			fbi[AP].start = 0;
			fbi[AP].mem_start = 0;
			fbi[AP].len = MAX_AP_FIRMWARE_SIZE;

			fbi[DATA].start = DLM_HEX_ADDRESS;
			fbi[DATA].mem_start = DLM_START_ADDRESS;
			fbi[DATA].len = MAX_DLM_FIRMWARE_SIZE;

			fbi[MP].start = MP_HEX_ADDRESS;
			fbi[MP].mem_start = 0;
			fbi[MP].len = MAX_MP_FIRMWARE_SIZE;

			core_gesture->area_section = (pfw[0xFFCF] << 24) + (pfw[0xFFCE] << 16) + (pfw[0xFFCD] << 8) + pfw[0xFFCC];;
			fbi[GESTURE].mem_start = (pfw[0xFFD3] << 24) + (pfw[0xFFD2] << 16) + (pfw[0xFFD1] << 8) + pfw[0xFFD0];;
			fbi[GESTURE].len = MAX_GESTURE_FIRMWARE_SIZE;
			ges_fw_start = (pfw[0xFFDB] << 24) + (pfw[0xFFDA] << 16) + (pfw[0xFFD9] << 8) + pfw[0xFFD8];
			ges_fw_end = (pfw[0xFFDB] << 24) + (pfw[0xFFDA] << 16) + (pfw[0xFFD9] << 8) + pfw[0xFFD8];
			fbi[GESTURE].start = 0;
		}

		memset(gestrue_fw, 0xff, sizeof(gestrue_fw));

#ifdef GESTURE_ENABLE
		/* Parsing gesture info and code */
		if (fbi[GESTURE].mem_start != 0xffffffff && ges_fw_start != 0xffffffff && fbi[GESTURE].mem_start != 0 && ges_fw_start != 0)
			ipio_memcpy(gestrue_fw, (pfw + ges_fw_start), fbi[GESTURE].len, sizeof(gestrue_fw));
		else
			ipio_err("There is no gesture data inside fw\n");

		core_gesture->ap_length = MAX_GESTURE_FIRMWARE_SIZE;

		ipio_info("GESTURE memory start = 0x%x, upgrade lenth = 0x%x",
					fbi[GESTURE].mem_start, MAX_GESTURE_FIRMWARE_SIZE);
		ipio_info("hex area = %d, ap_start_addr = 0x%x, ap_end_addr = 0x%x\n",
					core_gesture->area_section, ges_fw_start, ges_fw_end);
#endif
		fbi[AP].name = "AP";
		fbi[DATA].name = "DATA";
		fbi[TUNING].name = "TUNING";
		fbi[MP].name = "MP";
		fbi[GESTURE].name = "GESTURE";

		/* upgrade mode define */
		fbi[DATA].mode = fbi[AP].mode = fbi[TUNING].mode = AP;
		fbi[MP].mode = MP;
		fbi[GESTURE].mode = GESTURE;
	}

	/* Get hex fw vers */
	core_firmware->new_fw_cb = (pfw[FW_VER_ADDR] << 24) | (pfw[FW_VER_ADDR + 1] << 16) |
			(pfw[FW_VER_ADDR + 2] << 8) | (pfw[FW_VER_ADDR + 3]);

	/* Calculate update adress  */
	ipio_info("Hex Tag = 0x%x\n", core_firmware->hex_tag);
	ipio_info("new_fw_cb = 0x%x\n", core_firmware->new_fw_cb);
	ipio_info("nStartAddr = 0x%06X, nEndAddr = 0x%06X, block_number(AF) = %d\n", core_firmware->start_addr, core_firmware->end_addr, core_firmware->block_number);
}

static int convert_hex_file(u8 *phex, uint32_t nSize, u8 *pfw)
{
	int block = 0;
	uint32_t i = 0, j = 0, k = 0, num = 0;
	uint32_t nLength = 0, nAddr = 0, nType = 0;
	uint32_t nStartAddr = 0x0, nEndAddr = 0x0, nExAddr = 0;
	uint32_t nOffset;

	memset(fbi, 0x0, sizeof(fbi));

	/* Parsing HEX file */
	for (; i < nSize;) {
		nLength = HexToDec(&phex[i + 1], 2);
		nAddr = HexToDec(&phex[i + 3], 4);
		nType = HexToDec(&phex[i + 7], 2);

		if (nType == 0x04) {
			nExAddr = HexToDec(&phex[i + 9], 4);
		} else if (nType == 0x02) {
			nExAddr = HexToDec(&phex[i + 9], 4);
			nExAddr = nExAddr >> 12;
		} else if (nType == BLOCK_TAG_AE || nType == BLOCK_TAG_AF) {
			/* insert block info extracted from hex */
			core_firmware->hex_tag = nType;
			if (core_firmware->hex_tag == BLOCK_TAG_AF)
				num = HexToDec(&phex[i + 9 + 6 + 6], 2);
			else
				num = block;

			fbi[num].start = HexToDec(&phex[i + 9], 6);
			fbi[num].end = HexToDec(&phex[i + 9 + 6], 6);
			fbi[num].fix_mem_start = INT_MAX;
			fbi[num].len = fbi[num].end - fbi[num].start + 1;
			ipio_info("Block[%d]: start_addr = %x, end = %x", num, fbi[num].start, fbi[num].end);

			block++;
		} else if (nType == BLOCK_TAG_B0 && core_firmware->hex_tag == BLOCK_TAG_AF) {
			num = HexToDec(&phex[i + 9 + 6], 2);
			fbi[num].fix_mem_start = HexToDec(&phex[i + 9], 6);
			ipio_info("Tag 0xB0: change Block[%d] to addr = 0x%x\n", num, fbi[num].fix_mem_start);
		}

		nAddr = nAddr + (nExAddr << 16);

		if (phex[i + 1 + 2 + 4 + 2 + (nLength * 2) + 2] == 0x0D)
			nOffset = 2;
		else
			nOffset = 1;

		if (nAddr > MAX_HEX_FILE_SIZE) {
			ipio_err("Invalid hex format %d\n", nAddr);
			return -1;
		}

		if (nType == 0x00) {
			nEndAddr = nAddr + nLength;
			if (nAddr < nStartAddr)
				nStartAddr = nAddr;
			/* fill data */
			for (j = 0, k = 0; j < (nLength * 2); j += 2, k++)
				pfw[nAddr + k] = HexToDec(&phex[i + 9 + j], 2);
		}
		i += 1 + 2 + 4 + 2 + (nLength * 2) + 2 + nOffset;
	}

	core_firmware->start_addr = nStartAddr;
	core_firmware->end_addr = nEndAddr;
	core_firmware->block_number = block;

	return 0;
}

static int hex_file_open_convert(u8 open_file_method, u8 *pfw)
{
	int ret = 0;
	const struct firmware *fw = NULL;
	struct file *f = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;

	ipio_info("Open file method = %s\n", open_file_method ? "FILP_OPEN" : "REQUEST_FIRMWARE");

	switch (open_file_method) {
	case REQUEST_FIRMWARE:
		if ((ilitek_fw_fsize > 0) && (ilitek_hex_buffer != NULL) && (is_force_upgrade == false)) {
			ipio_info("FW is already request,no need request again\n");

		} else {
			ipio_info("Request_firmware_file, name = %s \n", fw_name_buf);
			ret = request_firmware(&fw, fw_name_buf, ipd->dev);
			if (ret < 0) {
				ipio_err("Failed to open the file Name %s,try to open ili file\n", fw_name_buf);
				return -ENOMEM;
			}

			ilitek_fw_fsize = fw->size;
			ipio_info("ilitek_fw_fsize = %d\n", ilitek_fw_fsize);
			if (ilitek_fw_fsize <= 0) {
				ipio_err("The size of file is zero\n");
				release_firmware(fw);
				return -ENOMEM;
			}

			ilitek_hex_buffer = vmalloc(ilitek_fw_fsize * sizeof(uint8_t));
			if (ERR_ALLOC_MEM(ilitek_hex_buffer)) {
				ipio_err("Failed to allocate hex_buffer memory, %ld\n", PTR_ERR(ilitek_hex_buffer));
				release_firmware(fw);
				return -ENOMEM;
			}

			ipio_memcpy(ilitek_hex_buffer, fw->data, ilitek_fw_fsize * sizeof(*fw->data), ilitek_fw_fsize);
			release_firmware(fw);
		}
		break;
	case FILP_OPEN:
		ipio_err("filp_open path %s \n", UPDATE_FW_PATH);
		if(use_g_user_buf)
			f = filp_open(g_user_buf, O_RDONLY, 0644);
		else
			f = filp_open(UPDATE_FW_PATH, O_RDONLY, 0644);
		if (ERR_ALLOC_MEM(f)) {
			ipio_err("Failed to open the file at %ld.\n", PTR_ERR(f));
			return -ENOMEM;
		}

		ilitek_fw_fsize = f->f_inode->i_size;
		ipio_info("ilitek_fw_fsize = %d\n", ilitek_fw_fsize);
		if (ilitek_fw_fsize <= 0) {
			ipio_err("The size of file is invaild\n");
			filp_close(f, NULL);
			return -ENOMEM;
		}
		/* TODO: evaluate if switch to reserve memory method */
		ilitek_hex_buffer = vmalloc(ilitek_fw_fsize * sizeof(uint8_t));

		if (ERR_ALLOC_MEM(ilitek_hex_buffer)) {
			ipio_err("Failed to allocate hex_buffer memory, %ld\n", PTR_ERR(ilitek_hex_buffer));
			filp_close(f, NULL);
			return -ENOMEM;
		}

		/* ready to map user's memory to obtain data by reading files */
		old_fs = get_fs();
		set_fs(get_ds());
		set_fs(KERNEL_DS);
		pos = 0;
		vfs_read(f, ilitek_hex_buffer, ilitek_fw_fsize, &pos);
		set_fs(old_fs);
		filp_close(f, NULL);
		break;
	default:
		ipio_err("Unknown open file method, %d\n", open_file_method);
		break;
	}

	/* Convert hex and copy data from hex_buffer to pfw */
	convert_hex_file(ilitek_hex_buffer, ilitek_fw_fsize, pfw);

	return ret;
}

static int fw_upgrade_file_convert(int target, u8 *pfw, int open_file_method)
{
	int ret = 0;

	ipio_info("Convert fw data from %s\n", (target == ILI_FILE ? "ILI_FILE" : "HEX_FILE"));

	switch (target) {
	case HEX_FILE:
		/* Feed ili file if can't find hex file from filesystem. */
		if (hex_file_open_convert(open_file_method, pfw) < 0) {
			ipio_err("Open hex file fail, try ili file upgrade");
			ret = UPDATE_FAIL;
			break;
		}
		break;
	default:
		ipio_err("Unknown convert file, %d\n", target);
		ret = UPDATE_FAIL;
		break;
	}
	return ret;
}

static int host_download_dma_check(uint32_t start_addr, uint32_t block_size)
{
	int count = 50;
	uint32_t busy = 0;

	/* dma1 src1 adress */
	core_config_ice_mode_write(0x072104, start_addr, 4);
	/* dma1 src1 format */
	core_config_ice_mode_write(0x072108, 0x80000001, 4);
	/* dma1 dest address */
	core_config_ice_mode_write(0x072114, 0x00030000, 4);
	/* dma1 dest format */
	core_config_ice_mode_write(0x072118, 0x80000000, 4);
	/* Block size*/
	core_config_ice_mode_write(0x07211C, block_size, 4);

	if (core_config->chip_id == CHIP_TYPE_ILI7807) {
		/* crc off */
		core_config_ice_mode_write(0x041016, 0x00, 1);
		/* dma crc */
		 core_config_ice_mode_write(0x041017, 0x03, 1);
	} else if (core_config->chip_id == CHIP_TYPE_ILI9881) {
		/* crc off */
		core_config_ice_mode_write(0x041014, 0x00000000, 4);
		/* dma crc */
		core_config_ice_mode_write(0x041048, 0x00000001, 4);
	}

	/* crc on */
	core_config_ice_mode_write(0x041016, 0x01, 1);
	/* Dma1 stop */
	core_config_ice_mode_write(0x072100, 0x00000000, 4);
	/* clr int */
	core_config_ice_mode_write(0x048006, 0x1, 1);
	/* Dma1 start */
	core_config_ice_mode_write(0x072100, 0x01000000, 4);

	/* Polling BIT0 */
	while (count > 0) {
		mdelay(1);
		busy = core_config_read_write_onebyte(0x048006);

		if ((busy & 0x01) == 1)
			break;

		count--;
	}

	if (count <= 0) {
		ipio_err("BIT0 is busy\n");
		return -1;
	}

	return core_config_ice_mode_read(0x04101C);
}

static uint32_t tddi_check_data(uint32_t start_addr, uint32_t end_addr)
{
	int timer = 500;
	uint32_t busy = 0;
	uint32_t write_len = 0;
	uint32_t iram_check = 0;
	uint32_t id = core_config->chip_id;
	uint32_t type = core_config->chip_type;

	write_len = end_addr;

	ipio_debug(DEBUG_FIRMWARE, "start = 0x%x , write_len = 0x%x, max_count = %x\n",
	    start_addr, end_addr, core_firmware->max_count);

	if (write_len > core_firmware->max_count) {
		ipio_err("The length (%x) written to firmware is greater than max count (%x)\n",
			write_len, core_firmware->max_count);
		goto out;
	}

	core_config_ice_mode_write(0x041000, 0x0, 1);	/* CS low */
	core_config_ice_mode_write(0x041004, 0x66aa55, 3);	/* Key */

	core_config_ice_mode_write(0x041008, 0x3b, 1);
	core_config_ice_mode_write(0x041008, (start_addr & 0xFF0000) >> 16, 1);
	core_config_ice_mode_write(0x041008, (start_addr & 0x00FF00) >> 8, 1);
	core_config_ice_mode_write(0x041008, (start_addr & 0x0000FF), 1);

	core_config_ice_mode_write(0x041003, 0x01, 1);	/* Enable Dio_Rx_dual */
	core_config_ice_mode_write(0x041008, 0xFF, 1);	/* Dummy */

	/* Set Receive count */
	if (core_firmware->max_count == 0xFFFF)
		core_config_ice_mode_write(0x04100C, write_len, 2);
	else if (core_firmware->max_count == 0x1FFFF)
		core_config_ice_mode_write(0x04100C, write_len, 3);

	if (id == CHIP_TYPE_ILI9881) {
		if (type == TYPE_F) {
			/* Checksum_En */
			core_config_ice_mode_write(0x041014, 0x10000, 3);
		} else {
			/* Clear Int Flag */
			core_config_ice_mode_write(0x048007, 0x02, 1);

			/* Checksum_En */
			core_config_ice_mode_write(0x041016, 0x00, 1);
			core_config_ice_mode_write(0x041016, 0x01, 1);
		}
	} else if (id == CHIP_TYPE_ILI7807) {
			/* Clear Int Flag */
			core_config_ice_mode_write(0x048007, 0x02, 1);

			/* Checksum_En */
			core_config_ice_mode_write(0x041016, 0x00, 1);
			core_config_ice_mode_write(0x041016, 0x01, 1);
	} else {
		ipio_err("Unknown CHIP\n");
		return -ENODEV;
	}

	/* Start to receive */
	core_config_ice_mode_write(0x041010, 0xFF, 1);

	while (timer > 0) {

		mdelay(1);

		if (id == CHIP_TYPE_ILI9881) {
			if (type == TYPE_F) {
				busy = core_config_read_write_onebyte(0x041014);
			} else {
				busy = core_config_read_write_onebyte(0x048007);
				busy = busy >> 1;
			}
		} else if (id == CHIP_TYPE_ILI7807) {
			busy = core_config_read_write_onebyte(0x048007);
			busy = busy >> 1;
		} else {
			ipio_err("Unknow chip type\n");
			break;
		}

		if ((busy & 0x01) == 0x01)
			break;

		timer--;
	}

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */

	if (timer >= 0) {
		/* Disable dio_Rx_dual */
		core_config_ice_mode_write(0x041003, 0x0, 1);
		iram_check =  core_firmware->isCRC ? core_config_ice_mode_read(0x4101C) : core_config_ice_mode_read(0x041018);
	} else {
		ipio_err("TIME OUT\n");
		goto out;
	}

	return iram_check;

out:
	ipio_err("Failed to read Checksum/CRC from IC\n");
	return -1;

}

static int tddi_read_flash(uint32_t start, uint32_t end, uint8_t *data, int dlen)
{
	uint32_t i, cont = 0;

	if (data == NULL) {
		ipio_err("data is null, read failed\n");
		return -1;
	}

	if (end - start > dlen) {
		ipio_err("the length (%d) reading crc is over than dlen(%d)\n", end - start, dlen);
		return -1;
	}

	core_config_ice_mode_write(0x041000, 0x0, 1);	/* CS low */
	core_config_ice_mode_write(0x041004, 0x66aa55, 3);	/* Key */
	core_config_ice_mode_write(0x041008, 0x03, 1);

	core_config_ice_mode_write(0x041008, (start & 0xFF0000) >> 16, 1);
	core_config_ice_mode_write(0x041008, (start & 0x00FF00) >> 8, 1);
	core_config_ice_mode_write(0x041008, (start & 0x0000FF), 1);

	for (i = start; i <= end; i++) {
		core_config_ice_mode_write(0x041008, 0xFF, 1);	/* Dummy */

		data[cont] = core_config_read_write_onebyte(0x41010);
		ipio_info("data[%d] = %x\n", cont, data[cont]);
		cont++;
	}

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */
	return 0;
}

int core_dump_flash(void)
{
	struct file *f = NULL;
	uint8_t *hex_buffer = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	u32 start_addr = 0x0, end_addr = 0x1FFFF;
	int length;

	core_config_ice_mode_enable(STOP_MCU);

	f = filp_open(DUMP_FLASH_PATH, O_WRONLY | O_CREAT | O_TRUNC, 644);
	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open the file at %ld.\n", PTR_ERR(f));
		goto out;
	}

	length = end_addr - start_addr + 1;

	hex_buffer = vmalloc(length * sizeof(uint8_t));
	if (ERR_ALLOC_MEM(hex_buffer)) {
		ipio_err("Failed to allocate hex_buffer memory, %ld\n", PTR_ERR(hex_buffer));
		filp_close(f, NULL);
		goto out;
	}

	tddi_read_flash(start_addr, end_addr, hex_buffer, length);

	old_fs = get_fs();
	set_fs(get_ds());
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, hex_buffer, length, &pos);
	set_fs(old_fs);
	filp_close(f, NULL);

	ipio_info("dump flash success\n");

	return 0;

out:
	core_config_ice_mode_disable();
	return -1;
}

static int check_fw_upgrade(u8 *pfw)
{
	int ret = NO_NEED_UPDATE;
	int i, crc_byte_len = 4;
	uint8_t flash_crc[4] = {0};
	uint32_t start_addr = 0, end_addr = 0, flash_crc_cb;
	uint32_t hex_crc, block_crc;

	if (pfw == NULL) {
		ipio_err("pfw data is null, ignore upgrade\n");
		return CHECK_FW_FAIL;
	}

	/* No need to check fw ver/crc if we upgrade it by manual */
	if (!core_firmware->isboot) {
		ipio_info("Upgrad FW forcly by manual\n");
		return NEED_UPDATE;
	}

	/* Store current fw version */
	if (protocol->major >= 0x5 && protocol->mid >= 0x3) {
		core_firmware->current_fw_cb = (core_config->firmware_ver[1] << 24) |
			(core_config->firmware_ver[2] << 16) | (core_config->firmware_ver[3] << 8) | core_config->firmware_ver[4];
	} else {
		core_firmware->current_fw_cb = (core_config->firmware_ver[1] << 16) |
			(core_config->firmware_ver[2] << 8) | core_config->firmware_ver[3];
	}

	/* Check FW version */
	ipio_info("New FW ver = 0x%x, Current FW ver = 0x%x\n", core_firmware->new_fw_cb, core_firmware->current_fw_cb);
	if (core_firmware->new_fw_cb > core_firmware->current_fw_cb) {
		ipio_info("New FW ver is greater than current, need to upgrade\n");
		return NEED_UPDATE;
	} else if (core_firmware->new_fw_cb == core_firmware->current_fw_cb) {
		ipio_info("New FW ver is equial to current, need to check hw crc if it's correct\n");
		goto check_hw_crc;
	} else {
		ipio_info("New FW ver is smaller than current, need to check flash crc if it's correct\n");
		goto check_flash_crc;
	}

check_hw_crc:
	/* Check HW and Hex CRC */
	for (i = 0; i < ARRAY_SIZE(fbi); i++) {
		start_addr = fbi[i].start;
		end_addr = fbi[i].end;

		/* Invaild end address */
		if (end_addr == 0)
			continue;

		hex_crc = (pfw[end_addr - 3] << 24) + (pfw[end_addr - 2] << 16) + (pfw[end_addr - 1] << 8) + pfw[end_addr];

		/* Get HW CRC for each block */
		block_crc = tddi_check_data(start_addr, end_addr - start_addr - crc_byte_len + 1);

		ipio_info("HW CRC = 0x%06x, HEX CRC = 0x%06x\n", hex_crc, block_crc);

		/* Compare HW CRC with HEX CRC */
		if (hex_crc != block_crc) {
			ipio_info("HW CRC with HEX CRC is diffirent ,have to update \n");
			ret = NEED_UPDATE;
		} else {
			ipio_info("HW CRC same as HEX CRC ,no need to update \n");
		}
	}

	return ret;

check_flash_crc:
	/* Check Flash CRC and HW CRC */
	for (i = 0; i < ARRAY_SIZE(fbi); i++) {
		start_addr = fbi[i].start;
		end_addr = fbi[i].end;

		/* Invaild end address */
		if (end_addr == 0)
			continue;

		ret = tddi_read_flash(end_addr - crc_byte_len + 1, end_addr, flash_crc, sizeof(flash_crc));
		if (ret < 0) {
			ipio_err("Read Flash failed\n");
			return CHECK_FW_FAIL;
		}

		block_crc = tddi_check_data(start_addr, end_addr - start_addr - crc_byte_len + 1);

		CRC_ONESET(flash_crc, flash_crc_cb);

		ipio_info("HW CRC = 0x%06x, Flash CRC = 0x%06x\n", block_crc, flash_crc_cb);

		/* Compare Flash CRC with HW CRC */
		if (flash_crc_cb != block_crc) {
			ipio_info("Flash CRC with HW CRC is diffirent ,have to update \n");
			ret = NEED_UPDATE;
		} else {
			ipio_info("Flash CRC same as HW CRC ,no need to update \n");
		}
		memset(flash_crc, 0, sizeof(flash_crc));
	}

	return ret;
}

static int do_erase_flash(uint32_t start_addr)
{
	int ret = 0;
	uint32_t rev_addr;

	ret = core_flash_write_enable();
	if (ret < 0) {
		ipio_err("Failed to config write enable\n");
		goto out;
	}

	core_config_ice_mode_write(0x041000, 0x0, 1);	/* CS low */
	core_config_ice_mode_write(0x041004, 0x66aa55, 3);	/* Key */

	if (start_addr == fbi[AP].start)
		core_config_ice_mode_write(0x041008, 0xD8, 1);
	else
		core_config_ice_mode_write(0x041008, 0x20, 1);

	rev_addr = ((start_addr & 0xFF0000) >> 16) | (start_addr & 0x00FF00) | ((start_addr & 0x0000FF) << 16);
	core_config_ice_mode_write(0x041008, rev_addr, 3);

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */

	mdelay(1);

	if (start_addr == fbi[AP].start)
		ret = core_flash_poll_busy(TIMEOUT_PAGE);
	else
		ret = core_flash_poll_busy(TIMEOUT_SECTOR);

	if (ret < 0)
		goto out;

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */

	ipio_debug(DEBUG_FIRMWARE, "Earsing data at start addr: %x\n", start_addr);

out:
	return ret;
}

static int flash_erase(void)
{
	uint32_t i, j, ret = 0;

	for (i = 0; i < ARRAY_SIZE(fbi); i++) {
		if (fbi[i].end == 0)
			continue;
		ipio_debug(DEBUG_FIRMWARE, "Block[%d] earsing start 0x%x to end 0x%x \n", i, fbi[i].start, fbi[i].end);
		for (j = fbi[i].start; j <= fbi[i].end; j += flashtab->sector) {
			ret = do_erase_flash(j);
			if (ret < 0)
				goto out;
			if (fbi[i].start == fbi[AP].start)
				break;
		}
	}

out:
	return ret;
}

static int do_program_flash(uint32_t start_addr, u8 *pfw)
{
	int ret = 0;
	uint32_t k, rev_addr;
	uint8_t buf[512] = { 0 };
	bool skip = true;

	buf[0] = 0x25;
	buf[3] = 0x04;
	buf[2] = 0x10;
	buf[1] = 0x08;

	for (k = 0; k < flashtab->program_page; k++) {
		if (start_addr + k <= core_firmware->end_addr)
			buf[4 + k] = pfw[start_addr + k];
		else
			buf[4 + k] = 0xFF;

		if (buf[4 + k] != 0xFF)
			skip = false;
	}

	if (skip) {
		core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */
		goto out;
	}

	ret = core_flash_write_enable();
	if (ret < 0)
		goto out;

	core_config_ice_mode_write(0x041000, 0x0, 1);	/* CS low */
	core_config_ice_mode_write(0x041004, 0x66aa55, 3);	/* Key */

	core_config_ice_mode_write(0x041008, 0x02, 1);
	rev_addr = ((start_addr & 0xFF0000) >> 16) | (start_addr & 0x00FF00) | ((start_addr & 0x0000FF) << 16);
	core_config_ice_mode_write(0x041008, rev_addr, 3);

	if (core_write(core_config->slave_i2c_addr, buf, flashtab->program_page + 4) < 0) {
		ipio_err("Failed to write data at start_addr = 0x%X, k = 0x%X, addr = 0x%x\n",
			start_addr, k, start_addr + k);
		ret = -EIO;
		goto out;
	}

	core_config_ice_mode_write(0x041000, 0x1, 1);	/* CS high */

	if (flashtab->mid == 0xEF) {
		mdelay(1);
	} else {
		ret = core_flash_poll_busy(TIMEOUT_PROGRAM);
		if (ret < 0)
			goto out;
	}

	core_firmware->update_status = (start_addr * 101) / core_firmware->end_addr;

	/* holding the status until finish this upgrade. */
	if (core_firmware->update_status > 90)
		core_firmware->update_status = 90;

	/* Don't use ipio_info to print log because it needs to be kpet in the same line */
	//printk(KERN_CONT "%c ILITEK: Upgrading firmware ... start_addr = 0x%x, %02d%c \n", 0x0D,
			//start_addr, core_firmware->update_status,'%');

out:
	return ret;
}

static int flash_program(u8 *pfw)
{
	uint32_t i, j, ret = 0, times, len;

	for (i = 0; i < ARRAY_SIZE(fbi); i++) {

		if (fbi[i].end == 0)
			continue;

		len = (fbi[i].end - fbi[i].start) + 1;
		times = (len - 1 + flashtab->program_page) / flashtab->program_page;
		ipio_debug(DEBUG_FIRMWARE, "Block[%d] program start 0x%x to end 0x%x times%d\n", i, fbi[i].start, fbi[i].end, times);
		j = fbi[i].start;
		do {
			ret = do_program_flash(j, pfw);
			j += flashtab->program_page;
			if (ret < 0)
				goto out;
		} while (--times > 0);

	}

out:
	return ret;
}

static uint32_t calc_crc32(uint32_t start_addr, uint32_t len, uint8_t *data)
{
	uint32_t i, j;
	uint32_t CRC_POLY = 0x04C11DB7;
	uint32_t ReturnCRC = 0xFFFFFFFF;

	for (i = start_addr; i < start_addr + len; i++) {
		ReturnCRC ^= (data[i] << 24);

		for (j = 0; j < 8; j++) {
			if ((ReturnCRC & 0x80000000) != 0) {
				ReturnCRC = ReturnCRC << 1 ^ CRC_POLY;
			} else {
				ReturnCRC = ReturnCRC << 1;
			}
		}
	}

	return ReturnCRC;
}

static void calc_verify_data(uint32_t sa, uint32_t len, uint32_t *check, u8 *pfw)
{
	uint32_t i = 0;
	uint32_t tmp_ck = 0, tmp_crc = 0;

	if (core_firmware->isCRC) {
		tmp_crc = calc_crc32(sa, len, pfw);
		*check = tmp_crc;
	} else {
		for (i = sa; i < (sa + len); i++)
			tmp_ck = tmp_ck + pfw[i];

		*check = tmp_ck;
	}
}

static int do_check(uint32_t start, uint32_t len, u8 *pfw)
{
	int ret = 0;
	uint32_t vd = 0, lc = 0;

	calc_verify_data(start, len, &lc, pfw);
	vd = tddi_check_data(start, len);
	ret = CHECK_EQUAL(vd, lc);

	ipio_info("%s (%x) : (%x)\n", (ret < 0 ? "Invalid !" : "Correct !"), vd, lc);

	return ret;
}

static int verify_flash_data(u8 *pfw)
{
	uint32_t i, ret = 0, len;

	/* Check crc in each block */
	for (i = 0; i < ARRAY_SIZE(fbi); i++) {

		if (fbi[i].end == 0)
			continue;

		len = fbi[i].end - fbi[i].start + 1 - 4;

		ret = do_check(fbi[i].start, len, pfw);
		if (ret < 0)
			goto out;
	}
out:
	return ret;
}

static int fw_upgrade_flash(u8 *pfw)
{
	int ret = UPDATE_OK;

	ilitek_platform_reset_ctrl(true, SW_RST);

	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enable ICE mode\n");
		goto out;
	}

	/* Check if need to upgrade fw */
	ret = check_fw_upgrade(pfw);
	if (ret != NEED_UPDATE)
		goto out;

	/* Disable flash protection from being written */
	core_flash_enable_protect(false);

	ret = flash_erase();
	if (ret < 0) {
		ipio_err("Failed to erase flash\n");
		goto out;
	}

	mdelay(1);

	ret = flash_program(pfw);
	if (ret < 0) {
		ipio_err("Failed to program flash\n");
		goto out;
	}

	/* We do have to reset chip in order to move new code from flash to iram. */
	ilitek_platform_reset_ctrl(true, SW_RST);

	/* the delay time moving code depends on what the touch IC you're using. */
	mdelay(core_firmware->delay_after_upgrade);

	/* ensure that the chip has been updated */
	ipio_info("Enter to ICE Mode again\n");
	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enable ICE mode\n");
		goto out;
	}

	/* check the data that we've just written into the iram. */
	ret = verify_flash_data(pfw);
	if (ret == 0)
		ipio_info("Data Correct !\n");

out:
	core_config_ice_mode_disable();
	return ret;
}
static int tddi_iram_read(u8 *buf, u32 start, u32 end)
{
	int i;
	int addr = 0, r_len = SPI_UPGRADE_LEN;
	u8 cmd[4] = {0};

	if (!buf) {
		ipio_err("buf in null\n");
		return -ENOMEM;
	}

	for (addr = start, i = 0; addr < end; i += r_len, addr += r_len) {
		if ((addr + r_len) > (end + 1))
			r_len = end % r_len;

		cmd[0] = 0x25;
		cmd[3] = (char)((addr & 0x00FF0000) >> 16);
		cmd[2] = (char)((addr & 0x0000FF00) >> 8);
		cmd[1] = (char)((addr & 0x000000FF));

		if (core_write(core_config->slave_i2c_addr, cmd, 4)) {
			ipio_err("Failed to write iram data\n");
			return -ENODEV;
		}

		if (core_read(core_config->slave_i2c_addr, buf + i, r_len)) {
			ipio_err("Failed to Read iram data\n");
			return -ENODEV;
		}
	}
	return 0;
}

int core_dump_iram(u32 start, u32 end)
{
	struct file *f = NULL;
	u8 *buf = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	int ret, wdt, i;
	int len;

	f = filp_open(DUMP_IRAM_PATH, O_WRONLY | O_CREAT | O_TRUNC, 666);
	if (ERR_ALLOC_MEM(f)) {
		ipio_err("Failed to open the file at %ld.\n", PTR_ERR(f));
		return -1;
	}

	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		filp_close(f, NULL);
		return ret;
	}
	wdt = core_config_ice_mode_read(core_config->wdt_addr);
	ipio_info("Read WDT: %s\n", (ret ? "ON" : "OFF"));
	if(wdt)
	{
		if (core_config_set_watch_dog(false) < 0) {
			ipio_err("Failed to disable watch dog\n");
			filp_close(f, NULL);
			ret = -EINVAL;
			return ret;
		}
	}
	len = end - start + 1;

	buf = vmalloc(len * sizeof(u8));
	if (ERR_ALLOC_MEM(buf)) {
		ipio_err("Failed to allocate buf memory, %ld\n", PTR_ERR(buf));
		filp_close(f, NULL);
		ret = ENOMEM;
		goto out;
	}

	for (i = 0; i < len; i++)
		buf[i] = 0xFF;
	ipio_info("len= 0x%x\n",len);
	if (tddi_iram_read(buf, start, end) < 0)
		ipio_err("Read IRAM data failed\n");

	old_fs = get_fs();
	set_fs(get_ds());
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(f, buf, len, &pos);
	set_fs(old_fs);

out:
	if (wdt) {
		core_config_set_watch_dog(true);
	}
	core_config_ice_mode_disable();
	filp_close(f, NULL);
	ipio_vfree((void **)&buf);
	ipio_info("dump iram data success\n");
	return 0;
}
static void tddi_fw_print_iram_data(void)
{
	int i, len;
	int tmp = ipio_debug_level;
	u8 *buf = NULL;
	u32 start = 0, end = 0xFFFF;

	len = end - start + 1;

	buf = vmalloc(len * sizeof(u8));
	if (ERR_ALLOC_MEM(buf)) {
		ipio_err("Failed to allocate buf memory, %ld\n", PTR_ERR(buf));
		return;
	}

	for (i = 0; i < len; i++)
		buf[i] = 0xFF;

	if (tddi_iram_read(buf, start, end) < 0)
		ipio_err("Read IRAM data failed\n");

	ipio_debug_level = DEBUG_ALL;
	dump_data(buf, 8, len, 0, "IRAM");
	ipio_debug_level = tmp;
	ipio_vfree((void **)&buf);
}
static int fw_upgrade_iram(u8 *pfw)
{
	int ret = UPDATE_OK, i;
	int timeout = 0;
	uint32_t mode, crc, dma;
	u8 *fw_ptr = NULL;

	/* Reset before load AP and MP code*/
	if (!is_lcd_resume) {
		if (!core_gesture->entry) {
			if (RST_METHODS == HW_RST_HOST_DOWNLOAD)
				ilitek_platform_tp_hw_reset(true);
			else
				ret = core_config_ic_reset();
		}
		ret = core_config_ice_mode_enable(STOP_MCU);
		if (ret < 0) {
			ipio_err("Failed to enter ICE mode, ret = %d\n", ret);
			return ret;
		}
	}
	is_lcd_resume =false;
	if (core_config_set_watch_dog(false) < 0) {
		ipio_err("Failed to disable watch dog\n");
		ret = -EINVAL;
		return ret;
	}

	fw_ptr = pfw;
	if (core_fr->actual_fw_mode == protocol->test_mode) {
		mode = MP;
	} else if (core_gesture->entry) {
		mode = GESTURE;
		fw_ptr = gestrue_fw;
	} else {
		mode = AP;
	}

	/* Program data to iram acorrding to each block */
	for (i = 0; i < ARRAY_SIZE(fbi); i++) {
		if (fbi[i].mode == mode && fbi[i].len != 0) {
			ipio_info("Download %s code from hex 0x%x to IRAN 0x%x len = 0x%x\n", fbi[i].name, fbi[i].start, fbi[i].mem_start, fbi[i].len);
			write_download(fbi[i].mem_start, fbi[i].len, (fw_ptr + fbi[i].start), SPI_UPGRADE_LEN);

			crc = calc_crc32(fbi[i].start, fbi[i].len - 4, fw_ptr);
			dma = host_download_dma_check(fbi[i].mem_start, fbi[i].len - 4);

			ipio_info("%s CRC is %s (%x) : (%x)\n", fbi[i].name, (crc != dma ? "Invalid !" : "Correct !"), crc, dma);

			if (CHECK_EQUAL(crc, dma) == UPDATE_FAIL) {
				tddi_fw_print_iram_data();
				return UPDATE_FAIL;
			}
		}
	}
	if (ipd->sys_boot_fw) {
		ipio_info("wait for lcm init code download\n");
		timeout= wait_event_interruptible_timeout(ipd->wait_for_lcm, ipd->lcm_finish> 0,msecs_to_jiffies(500));
		if (!timeout)
			ipio_info("initcode not in\n");
		ipd->lcm_finish = 0;
	}
	if (!core_gesture->entry) {
		/* ice mode code reset */
		ipio_info("Doing code reset ...\n");
		core_config_ice_mode_write(0x40040, 0xAE, 1);
		first_int = true;
	}

	core_config_ice_mode_disable();
	mdelay(60);
	return ret;
}

static int flash_erase_mode(uint32_t mode)
{
	uint32_t i, ret = 0;

	ilitek_platform_reset_ctrl(true, SW_RST);

	ret = core_config_ice_mode_enable(STOP_MCU);
	if (ret < 0) {
		ipio_err("Failed to enable ICE mode\n");
		goto out;
	}
	core_flash_enable_protect(false);

	if (fbi[mode].end == 0)
		goto out;

	ipio_debug(DEBUG_FIRMWARE, "Block[%d] earsing start 0x%x to end 0x%x \n", mode, fbi[mode].start, fbi[mode].end);
	for (i = fbi[mode].start; i <= fbi[mode].end ; i += flashtab->sector) {
		ret = do_erase_flash(i);
		if (ret < 0)
			goto out;
	}

out:
	core_config_ice_mode_disable();
	return ret;
}

int core_firmware_upgrade(int upgrade_type, int file_type, int open_file_method)
{
	u8 *pfw = NULL;
	int ret  = UPDATE_OK, retry, rel = 0;
	bool power = false, esd = false;

	retry = core_firmware->retry_times;

	core_firmware->isUpgrading = true;
	core_firmware->update_status = 0;

	power = ipd->isEnablePollCheckPower;
	if (power) {
		ipd->isEnablePollCheckPower = false;
		cancel_delayed_work_sync(&ipd->check_power_status_work);
	}

	esd = ipd->isEnablePollCheckEsd;
	if (esd) {
		ipd->isEnablePollCheckEsd = false;
		cancel_delayed_work_sync(&ipd->check_esd_status_work);
	}

	pfw = vmalloc(UPGRADE_BUFFER_SIZE * sizeof(uint8_t));
	if (ERR_ALLOC_MEM(pfw)) {
		ipio_err("Failed to allocate pfw memory, %ld\n", PTR_ERR(pfw));
		ret = -ENOMEM;
		goto out;
	}
	memset(pfw, 0xFF, UPGRADE_BUFFER_SIZE * sizeof(uint8_t));

	if (!core_gesture->entry) {
		/* Parse ili/hex file */
		if (fw_upgrade_file_convert(file_type, pfw, open_file_method) < 0)
			goto out;

		fw_upgrade_info_setting(pfw, upgrade_type);
	}

	do {
		switch (upgrade_type) {
		case UPGRADE_FLASH:
			ret = fw_upgrade_flash(pfw);
			break;
		case UPGRADE_IRAM:
			ret = fw_upgrade_iram(pfw);
			break;
		default:
			ipio_err("Unknown upgrade type, %d\n", upgrade_type);
			ret = UPDATE_FAIL;
			break;
		}
		if (ret >= 0)
			break;

		ipio_info("Upgrade firmware retry Fail %d times !\n", (core_firmware->retry_times - retry + 1));
	} while (--retry > 0);

	if (ret < 0) {
		ipio_info("Upgrade firmware failed, erase %s \n", ((ret == UPGRADE_FLASH) ? "FLASH" : "IRAM"));
		if (upgrade_type == UPGRADE_FLASH) {
			rel = flash_erase_mode(AP);
		} else {
			if (RST_METHODS == HW_RST_HOST_DOWNLOAD)
				ilitek_platform_reset_ctrl(true, HW_RST);
			else
				ilitek_platform_reset_ctrl(true, SW_RST);
		}

		if (rel < 0)
			ipio_err("Failed to erase %s\n", ((upgrade_type == UPGRADE_FLASH) ? "FLASH" : "IRAM"));

		goto out;
	}

	core_config_get_fw_ver();
	core_config_get_protocol_ver();
	core_config_get_core_ver();
	core_config_get_tp_info();
	core_config_get_key_info();

out:
	if (power) {
		ipd->isEnablePollCheckPower = true;
		queue_delayed_work(ipd->check_power_status_queue,
			&ipd->check_power_status_work, ipd->work_delay);
	}
	if (esd) {
		ipd->isEnablePollCheckEsd = true;
		queue_delayed_work(ipd->check_esd_status_queue,
			&ipd->check_esd_status_work, ipd->esd_check_time);
	}

	core_firmware->isUpgrading = false;
	ipio_vfree((void **)&pfw);

	ipio_info("Upgrade firmware %s !\n", ((ret < 0) ? "failed" : "succed"));
	return ret;
}

int core_firmware_init(void)
{
	int i = 0, j = 0;

	core_firmware = devm_kzalloc(ipd->dev, sizeof(struct core_firmware_data), GFP_KERNEL);
	if (ERR_ALLOC_MEM(core_firmware)) {
		ipio_err("Failed to allocate core_firmware mem, %ld\n", PTR_ERR(core_firmware));
		return -ENOMEM;
	}

	core_firmware->hex_tag = 0;
	core_firmware->isboot = false;

	for (j = 0; j < 4; j++)
		core_firmware->new_fw_ver[i] = 0x0;

	core_firmware->max_count = 0x1FFFF;
	core_firmware->isCRC = true;
	core_firmware->retry_times = 3;
	core_firmware->delay_after_upgrade = 200;

	return 0;
}