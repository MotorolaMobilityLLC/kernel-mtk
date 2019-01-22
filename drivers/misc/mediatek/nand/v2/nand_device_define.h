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

#ifndef __NAND_DEVICE_LIST_H__
#define __NAND_DEVICE_LIST_H__

/* #define NAND_ABTC_ATAG */
/* #define ATAG_FLASH_NUMBER_INFO       0x54430006 */
/* #define ATAG_FLASH_INFO       0x54430007 */

struct tag_nand_number {
	u32 number;
};

#define MAX_FLASH 20		/* modify this define if device list is more than 20 later .xiaolei */

#define NAND_MAX_ID		6
#define CHIP_CNT		10
#define P_SIZE		16384
#define P_PER_BLK		256
#define C_SIZE		8192
#define RAMDOM_READ		(1<<0)
#define CACHE_READ		(1<<1)
#define MULTI_PLANE		(1<<2)
#define RAND_TYPE_SAMSUNG 0
#define RAND_TYPE_TOSHIBA 1
#define RAND_TYPE_NONE 2

#define READ_RETRY_MAX 10
struct gFeature {
	u32 address;
	u32 feature;
};

enum readRetryType {
	RTYPE_MICRON,
	RTYPE_SANDISK,
	RTYPE_SANDISK_19NM,
	RTYPE_TOSHIBA,
	RTYPE_TOSHIBA_15NM,
	RTYPE_HYNIX,
	RTYPE_HYNIX_16NM,
	RTYPE_SANDISK_TLC_1YNM,
	RTYPE_TOSHIBA_TLC,
	RTYPE_SANDISK_TLC_1ZNM
};

struct gFeatureSet {
	u8 sfeatureCmd;
	u8 gfeatureCmd;
	u8 readRetryPreCmd;
	u8 readRetryCnt;
	u32 readRetryAddress;
	u32 readRetryDefault;
	u32 readRetryStart;
	enum readRetryType rtype;
	struct gFeature Interface;
	struct gFeature Async_timing;
};

struct gRandConfig {
	u8 type;
	u32 seed[6];
};

enum pptbl {
	MICRON_8K,
	HYNIX_8K,
	SANDISK_16K,
	PPTBL_NONE,
};

struct MLC_feature_set {
	enum pptbl ptbl_idx;
	struct gFeatureSet FeatureSet;
	struct gRandConfig randConfig;
};

enum flashdev_vendor {
	VEND_SAMSUNG,
	VEND_MICRON,
	VEND_TOSHIBA,
	VEND_HYNIX,
	VEND_SANDISK,
	VEND_BIWIN,
	VEND_NONE,
};

enum flashdev_IOWidth {
	IO_8BIT = 8,
	IO_16BIT = 16,
	IO_TOGGLEDDR = 9,
	IO_TOGGLESDR = 10,
	IO_ONFI = 12,
};

#define NAND_FLASH_SLC	(0x0000)
#define NAND_FLASH_MLC	(0x0001)
#define NAND_FLASH_TLC	(0x0002)
#define NAND_FLASH_MLC_HYBER	(0x0003)
#define NAND_FLASH_MASK	(0x00FF)

struct NFI_TLC_CTRL {
	bool		slcopmodeEn;				/*TRUE: slc mode	FALSE: tlc mode*/
	bool		pPlaneEn;					/*this chip has pseudo plane*/
	bool		needchangecolumn;			/*read page with change column address command*/
	bool		normaltlc;					/*whether need 09/0d 01/02/03*/
	u16		en_slc_mode_cmd;			/*enable slc mode cmd*/
	u16		dis_slc_mode_cmd;			/*disable slc mode cmd: 0xff is invalid*/
	bool		ecc_recalculate_en;		/*for nfi config*/
	u8		ecc_required;				/*required ecc bit*/
	u8		block_bit;					/*block address start bit;*/
	u8		pPlane_bit;				/*pesudo plane bit;*/
};

enum NFI_TLC_PG_CYCLE {
	PROGRAM_1ST_CYCLE = 1,
	PROGRAM_2ND_CYCLE = 2,
	PROGRAM_3RD_CYCLE = 3
};

enum NFI_TLC_WL_PRE {
	WL_LOW_PAGE = 0,
	WL_MID_PAGE = 1,
	WL_HIGH_PAGE = 2,
};

struct NFI_TLC_WL_INFO {
	u32 word_line_idx;
	enum NFI_TLC_WL_PRE wl_pre;
};

struct NAND_LIFE_PARA {
	u16 slc_pe;	/* by 1k */
	u16 data_pe;
	u16 slc_bitflip;
	u16 data_bitflip;
};

typedef struct {
	u8 id[NAND_MAX_ID];
	u8 id_length;
	u8 addr_cycle;
	u32 iowidth;
	u32 totalsize;
	u16 blocksize;
	u16 pagesize;
	u16 sparesize;
	u32 timmingsetting;
	u32 s_acccon;
	u32 s_acccon1;
	u32 freq;
	u16 vendor;
	u16 sectorsize;
	u8 devciename[30];
	u32 advancedmode;
	struct MLC_feature_set feature_set;
	u16 NAND_FLASH_TYPE;
	struct NFI_TLC_CTRL tlcControl;
	bool two_phyplane;
	struct NAND_LIFE_PARA lifepara;
} flashdev_info_t, *pflashdev_info;

#endif
