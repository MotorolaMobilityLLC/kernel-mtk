/*
 * Copyright (C) 2017 MediaTek Inc.
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
#ifndef __MTK_NAND_DEVICE_FEATURE_H
#define __MTK_NAND_DEVICE_FEATURE_H

#include "mtk_nand_util.h"
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>

#define SUPPORT_TOSHIBA_DEVICE 1
#ifndef FALSE
  #define FALSE (0)
#endif

#ifndef TRUE
  #define TRUE  (1)
#endif

#ifdef SUPPORT_MICRON_DEVICE
#define MICRON_DEVICE 2
#else
#define MICRON_DEVICE 0
#endif
#ifdef SUPPORT_SANDISK_DEVICE
#define SANDISK_DEVICE 4
#else
#define SANDISK_DEVICE 0
#endif
#ifdef SUPPORT_HYNIX_DEVICE
#define HYNIX_DEVICE 4
#else
#define HYNIX_DEVICE 0
#endif
#ifdef SUPPORT_TOSHIBA_DEVICE
#define TOSHIBA_DEVICE 4
#else
#define TOSHIBA_DEVICE 0
#endif

#define TOTAL_SUPPORT_DEVICE (MICRON_DEVICE + SANDISK_DEVICE + HYNIX_DEVICE + TOSHIBA_DEVICE)

#define MAX_FLASH 20

#define NAND_MAX_ID		6
#define CHIP_CNT		21
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

#define NAND_FLASH_SLC	(0x0000)
#define NAND_FLASH_MLC	(0x0001)
#define NAND_FLASH_TLC	(0x0002)
#define NAND_FLASH_MLC_HYBER	(0x0003)
#define NAND_FLASH_MASK	(0x00FF)

#define flashdev_info struct flashdev_info_t

typedef void(*rrtryFunctionType) (struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
				 bool defValue);
typedef u32 (*GetLowPageNumber)(u32 pageNo);
typedef u32 (*TransferPageNumber)(u32 pageNo, bool high_to_low);

enum readRetryType {
#ifdef SUPPORT_MICRON_DEVICE
	RTYPE_MICRON,
#endif
#ifdef SUPPORT_SANDISK_DEVICE
	RTYPE_SANDISK,
	RTYPE_SANDISK_19NM,
	RTYPE_SANDISK_TLC_1YNM,
#endif
#ifdef SUPPORT_HYNIX_DEVICE
	RTYPE_HYNIX,
	RTYPE_HYNIX_16NM,
#endif
#if SUPPORT_TOSHIBA_DEVICE
	RTYPE_TOSHIBA,
	RTYPE_TOSHIBA_TLC
#endif
};

enum NAND_TYPE_MASK {
	TYPE_ASYNC = 0x0,
	TYPE_TOGGLE = 0x1,
	TYPE_SYNC = 0x2,
	TYPE_RESERVED = 0x3,
	TYPE_MLC = 0x4,
	TYPE_SLC = 0x4,
};

enum pptbl {
#ifdef SUPPORT_MICRON_DEVICE
	MICRON_8K,
#endif
#ifdef SUPPORT_HYNIX_DEVICE
	HYNIX_8K,
#endif
#ifdef SUPPORT_SANDISK_DEVICE
	SANDISK_16K,
#endif
#ifdef SUPPORT_TOSHIBA_DEVICE
	TOSHIBA_16K,
#endif


	PPTBL_NONE,
};

enum flashdev_IOWidth {
	IO_8BIT = 8,
	IO_16BIT = 16,
	IO_TOGGLEDDR = 9,
	IO_TOGGLESDR = 10,
	IO_ONFI = 12,
};
struct tag_nand_number {
	u32 number;
};

struct gFeature {
	u32 address;
	u32 feature;
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

struct MLC_feature_set {
	enum pptbl ptbl_idx;
	struct gFeatureSet	FeatureSet;
	struct gRandConfig	randConfig;
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

struct flashdev_info_t {
	u8 id[NAND_MAX_ID];
	u8 id_length;
	u8 addr_cycle;
	enum flashdev_IOWidth iowidth;
	u32 totalsize;
	u16 blocksize;
	u16 pagesize;
	u16 sparesize;
	u32 timmingsetting;
	u32 dqs_delay_ctrl;
	u32 s_acccon;
	u32 s_acccon1;
	u32 freq;
	enum flashdev_vendor vendor;
	u16 sectorsize;
	u8 devciename[30];
	u32 advancedmode;
	struct MLC_feature_set feature_set;
	u16 NAND_FLASH_TYPE;
	struct NFI_TLC_CTRL tlcControl;
	bool two_phyplane;
	struct NAND_LIFE_PARA lifepara;
};

extern u32 TOSHIBA_TRANSFER(u32 pageNo);
extern u32 MICRON_TRANSFER(u32 pageNo);
extern u32 HYNIX_TRANSFER(u32 pageNo);
extern u32 SANDISK_TRANSFER(u32 pageNo);

extern u32 micron_pairpage_mapping(u32 page, bool high_to_low);
extern u32 hynix_pairpage_mapping(u32 page, bool high_to_low);
extern u32 sandisk_pairpage_mapping(u32 page, bool high_to_low);
extern u32 toshiba_pairpage_mapping(u32 page, bool high_to_low);

extern u16 randomizer_seed[128];

#ifdef SUPPORT_MICRON_DEVICE
extern void mtk_nand_micron_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
#endif
#ifdef SUPPORT_SANDISK_DEVICE
extern void mtk_nand_sandisk_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
extern void mtk_nand_sandisk_19nm_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
extern void mtk_nand_sandisk_tlc_1ynm_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
#endif
#ifdef SUPPORT_HYNIX_DEVICE
extern void mtk_nand_hynix_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
extern void mtk_nand_hynix_16nm_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
#endif
#if SUPPORT_TOSHIBA_DEVICE
extern void mtk_nand_toshiba_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
extern void mtk_nand_toshiba_tlc_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue);
#endif

extern const struct flashdev_info_t gen_FlashTable[];

extern u32 mtk_nand_rrtry_setting(flashdev_info deviceinfo, enum readRetryType type, u32 retryStart,
				  u32 loopNo);
extern void mtk_nand_rrtry_func(struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
								  bool defValue);

extern rrtryFunctionType rtyFuncArray[];
extern TransferPageNumber fsFuncArray[];
extern GetLowPageNumber functArray[];

#endif
