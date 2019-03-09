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

#ifndef __FIRMWARE_H
#define __FIRMWARE_H

struct core_firmware_data {
	uint8_t new_fw_ver[4];
	uint8_t  block_number;

	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t current_fw_cb;
	uint32_t new_fw_cb;

	uint32_t update_status;
	uint32_t max_count;
	uint8_t  retry_times;

	int delay_after_upgrade;

	bool isUpgrading;
	bool isCRC;
	bool isboot;
	int hex_tag;
};

struct flash_block_info {
	char *name;
	uint32_t start;
	uint32_t end;
	uint32_t len;
	uint32_t mem_start;
	uint32_t fix_mem_start;
	uint8_t mode;
};

enum upgrade_target {
	ILI_FILE = 0,
	HEX_FILE
};

enum upgrade_type {
	UPGRADE_FLASH = 0,
	UPGRADE_IRAM
};

/* FW block number */
enum block_num {
	AP = 1,
	DATA = 2,
	TUNING = 3,
	GESTURE = 4,
	MP = 5,
	DDI = 6
};

/* FW block info tag */
enum {
	BLOCK_TAG_AE = 0xAE,
	BLOCK_TAG_AF = 0xAF,
	BLOCK_TAG_B0 = 0xB0
};

/* The addr of block reserved for customers */
#define RESERVE_BLOCK_START_ADDR 0x1D000
#define RESERVE_BLOCK_END_ADDR 0x1DFFF

#define UPGRADE_BUFFER_SIZE	   MAX_HEX_FILE_SIZE

extern struct core_firmware_data *core_firmware;
extern int core_firmware_upgrade(int type, int file_type, int open_file_method);
extern int core_firmware_init(void);
extern int core_dump_flash(void);

#endif /* __FIRMWARE_H */
