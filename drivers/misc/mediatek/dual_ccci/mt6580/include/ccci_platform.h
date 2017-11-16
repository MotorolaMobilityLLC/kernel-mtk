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

#ifndef __CCCI_PLATFORM_H__
#define __CCCI_PLATFORM_H__
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <mach/irqs.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/sync_write.h>
#include <linux/memory.h>
#include <linux/string.h>
#include "ccci_platform_cfg.h"

/* ******************** macro definition**************************/
#define CCCI_DEV_MAJOR			184
#define MD_IMG_RESRVED_SIZE		(0x700000)
#define MD_RW_MEM_RESERVED_SIZE	(0xF00000)
#if defined(CCCI_STATIC_SHARED_MEM)
/*  2MB shared memory because MD side proctection is 2MB alignment */
#define CCCI_SHARED_MEM_SIZE	UL(0x200000)
#endif
/*modem&dsp name define*/
#define MD_INDEX	0
#define DSP_INDEX	1
#define IMG_CNT		1
/*  For MD1 */
#define MOEDM_IMAGE_NAME	"modem.img"
#define DSP_IMAGE_NAME		"DSP_ROM"
#define MOEDM_IMAGE_E1_NAME	"modem_E1.img"
#define DSP_IMAGE_E1_NAME	"DSP_ROM_E1"
#define MOEDM_IMAGE_E2_NAME	"modem_E2.img"
#define DSP_IMAGE_E2_NAME	"DSP_ROM_E2"
/*  For MD2 */
#define MOEDM_SYS2_IMAGE_NAME		"modem_sys2.img"
#define MOEDM_SYS2_IMAGE_E1_NAME	"modem_sys2_E1.img"
#define MOEDM_SYS2_IMAGE_E2_NAME	"modem_sys2_E2.img"
#ifndef CONFIG_MODEM_FIRMWARE_PATH
#define CONFIG_MODEM_FIRMWARE_PATH	"/etc/firmware/"
#define CONFIG_MODEM_FIRMWARE_CIP_PATH	"/custom/etc/firmware/"
#define MOEDM_IMAGE_PATH				"/etc/firmware/modem.img"
#define DSP_IMAGE_PATH					"/etc/firmware/DSP_ROM"
#define MOEDM_IMAGE_E1_PATH				"/etc/firmware/modem_E1.img"
#define DSP_IMAGE_E1_PATH				"/etc/firmware/DSP_ROM_E1"
#define MOEDM_IMAGE_E2_PATH				"/etc/firmware/modem_E2.img"
#define DSP_IMAGE_E2_PATH				"/etc/firmware/DSP_ROM_E2"
#endif
/*modem&dsp check header macro define*/
#define DSP_VER_3G  0x0
#define DSP_VER_2G  0x1
#define DSP_VER_INVALID  0x2
#define VER_3G_STR  "3G"
#define VER_2G_STR  "2G"
#define VER_WG_STR   "WG"
#define VER_TG_STR   "TG"
#define VER_INVALID_STR  "INVALID"
#define DEBUG_STR   "Debug"
#define RELEASE_STR  "Release"
#define INVALID_STR  "INVALID"
#define DSP_ROM_TYPE 0x0104
#define DSP_BL_TYPE  0x0003
#define DSP_ROM_STR  "DSP_ROM"
#define DSP_BL_STR   "DSP_BL"
#define MD_HEADER_MAGIC_NO "CHECK_HEADER"
/* #define MD_HEADER_VER_NO 0x2     */
#define GFH_HEADER_MAGIC_NO 0x4D4D4D
/* #define GFH_HEADER_VER_NO 0x1 */
#define GFH_FILE_INFO_TYPE 0x0
#define GFH_CHECK_HEADER_TYPE 0x104
#define DSP_2G_BIT 16
#define DSP_DEBUG_BIT 17
/* *********************HW register definitions*********************/
/* MD & DSP WDT Default value and KEY */
/*-- Modem --*/
#define WDT_MD_MODE_DEFAULT		(0x3)
#define WDT_MD_MODE_KEY			(0x22<<8)
#define WDT_MD_LENGTH_DEFAULT		(0x7FF<<5)
#define WDT_MD_LENGTH_KEY		(0x8)
#define WDT_MD_RESTART_KEY		(0x1971)
/*-- DSP --*/
#define WDT_DSP_MODE_DEFAULT		(0x3)
#define WDT_DSP_MODE_KEY		(0x22<<8)
#define WDT_DSP_LENGTH_DEFAULT		(0x7FF<<5)
#define WDT_DSP_LENGTH_KEY		(0x8)
#define WDT_DSP_RESTART_KEY		(0x1971)
/* Modem WDT */
#define WDT_MD_MODE(base)		((base) + 0x00)
#define WDT_MD_LENGTH(base)		((base) + 0x04)
#define WDT_MD_RESTART(base)		((base) + 0x08)
#define WDT_MD_STA(base)		((base) + 0x0C)
#define WDT_MD_SWRST(base)		((base) + 0x1C)
/*  Modem side: 0x810D0000 */
#define MD_INFRA_BASE			(0xD10D0000)
/*  Modem side: 0x810C0000 */
#define MD_RGU_BASE			(0xD10C0000)
#define CLK_SW_CON0			(0x00000910)
#define CLK_SW_CON1			(0x00000914)
#define CLK_SW_CON2			(0x00000918)
#define BOOT_JUMP_ADDR			(0x00000980)
/******Physical address remapping register between AP and MD memory***********/
#define AP_BANK4_MAP0(base)			((volatile unsigned int *)(base+0x200))
#define AP_BANK4_MAP1(base)			((volatile unsigned int *)(base+0x204))
/* - MD side, using infra config base */
/* -- MD1 Bank 0 */
#define MD1_BANK0_MAP0(base)			((volatile unsigned int *)(base+0x300))
#define MD1_BANK0_MAP1(base)			((volatile unsigned int *)(base+0x304))
/* -- MD1 Bank 4 */
#define MD1_BANK4_MAP0(base)			((volatile unsigned int *)(base+0x308))
#define MD1_BANK4_MAP1(base)			((volatile unsigned int *)(base+0x30C))
/* -- Using infra config base too */
#define TOPAXI_AXI_ASLICE_CRL(base)		((volatile unsigned int *)(base+0x22C))
/* *****************structure definition*****************************/
/*  DSP check image header */
struct GFH_HEADER {
	/* bit23-bit0="MMM", not31-bit24:header version number=1 */
	unsigned int m_magic_ver;
	/* the size of GFH structure */
	unsigned short m_size;
	/* m_type=0, struct GFH_FILE_INFO_v1; m_type=0x104, struct GFH_CHECK_CFG_v1 */
	unsigned short m_type;
};

struct GFH_FILE_INFO_v1 {
	struct GFH_HEADER m_gfh_hdr;	/* m_type=0 */
	unsigned char m_identifier[12];	/* "FILE_INFO" */
	unsigned int m_file_ver;		/* bit16=0:3G, bit16=1:2G;  bit17=0:release, bit17=1:debug */
	unsigned short m_file_type;	/* DSP_ROM:0x0104; DSP_BL:0x0003 */
	unsigned char md_id;		/* MD_SYS1: 1, MD_SYS2: 2 */
	unsigned char dummy2;
	unsigned int dummy3[7];
};

struct GFH_CHECK_CFG_v1 {
	struct GFH_HEADER m_gfh_hdr;	/* m_type=0x104, m_size=0xc8 */
	unsigned int m_product_ver;	/* 0x0:invalid; 0x1:debug version; 0x2:release version */
	unsigned int m_image_type;	/* 0x0:invalid; 0x1:2G modem; 0x2: 3G modem */
	unsigned char m_platform_id[16];	/* chip version, ex:MT6573_S01 */
	unsigned char m_project_id[64];	/* build version, ex: MAUI.11A_MD.W11.31 */
	unsigned char m_build_time[64];	/* build time, ex: 2011/8/4 04:19:30 */
	unsigned char reserved[64];
};

struct MD_CHECK_HEADER {
	unsigned char check_header[12];	/* magic number is "CHECK_HEADER" */
	unsigned int header_verno;	/* header structure version number */
	unsigned int product_ver;	/* 0x0:invalid; 0x1:debug version; 0x2:release version */
	unsigned int image_type;		/* 0x0:invalid; 0x1:2G modem; 0x2: 3G modem */
	unsigned char platform[16];	/* MT6573_S01 or MT6573_S02 */
	unsigned char build_time[64];	/* build time string */
	unsigned char build_ver[64];	/* project version, ex:11A_MD.W11.28 */
	unsigned char bind_sys_id;		/* bind to md sys id, MD SYS1: 1, MD SYS2: 2 */
	unsigned char ext_attr;		/* no shrink: 0, shrink: 1 */
	unsigned char reserved[2];		/* for reserved */
	unsigned int mem_size;		/* md ROM/RAM image size requested by md */
	unsigned int md_img_size;	/* md image size, exclude head size */
	unsigned int reserved_info;	/* for reserved */
	unsigned int size;		/* the size of this structure */
};

struct CCCI_REGION_LAYOUT {
	unsigned long modem_region_base;
	unsigned long modem_region_len;
	unsigned long dsp_region_base;
	unsigned long dsp_region_len;
	unsigned long mdif_region_base;
	unsigned long mdif_region_len;
	unsigned long ccif_region_base;
	unsigned long ccif_region_len;
	unsigned long mdcfg_region_base;
	unsigned long mdcfg_region_len;
};

#define  NAME_LEN 100
#define  AP_PLATFORM_LEN 16
struct image_info {
	/*type=0,modem image; type=1, dsp image */
	int type;
	char file_name[NAME_LEN];
	unsigned long address;
	ssize_t size;
	loff_t offset;
	unsigned int tail_length;
	char *ap_platform;
	struct IMG_CHECK_INFO img_info;
	struct IMG_CHECK_INFO ap_info;
	int (*load_firmware)(int, struct image_info *info);
	unsigned int flags;
};

/*modem image load way define*/
enum LOAD_FLAG {
	LOAD_NONE = 0,
	LOAD_MD_ONLY = (1 << MD_INDEX),
	LOAD_DSP_ONLY = (1 << DSP_INDEX),
	LOAD_MD_DSP = (1 << MD_INDEX | 1 << DSP_INDEX),
};

/* *******************external Function definition*****************/
extern struct ccci_mem_layout_t md_mem_layout_tab[];
extern void dump_firmware_info(void);
#ifdef ENABLE_DRAM_API
extern unsigned int get_max_DRAM_size(void);
extern unsigned int get_phys_offset(void);
extern unsigned int get_max_phys_addr(void);
#endif
extern int set_ap_smem_remap(int md_id, unsigned int src, unsigned int des);
extern int set_md_smem_remap(int md_id, unsigned int src, unsigned int des);
extern int set_md_rom_rw_mem_remap(int md_id, unsigned int src,
				   unsigned int des);

#endif	/* __CCCI_PLATFORM_H__ */
