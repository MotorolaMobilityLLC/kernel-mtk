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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/memblock.h>
#include <asm/memblock.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#endif
#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#endif

#include <asm/setup.h>
#include <linux/atomic.h>
#include <mt-plat/mt_boot_common.h>
#include <mt-plat/mt_ccci_common.h>

#include <mt-plat/mtk_memcfg.h>
#include "ccci_util_log.h"
/**************************************************************************
**** Local debug option for this file only ********************************
**************************************************************************/
/* #define LK_LOAD_MD_INFO_DEBUG_EN */

#define CCCI_MEM_ALIGN      (SZ_32M)
#define CCCI_SMEM_ALIGN_MD1 (0x200000)	/*2M */
#define CCCI_SMEM_ALIGN_MD2 (0x200000)	/*2M */
/*====================================================== */
/* DFO support section */
/*====================================================== */
typedef struct fos_item {	/* Feature Option Setting */
	char *name;
	volatile int value;
} fos_item_t;
/* DFO table */
/* TODO : the following macro can be removed sometime */
/* MD1 */
#ifdef CONFIG_MTK_ENABLE_MD1
#define MTK_MD1_EN	(1)
#else
#define MTK_MD1_EN	(0)
#endif

#ifdef CONFIG_MTK_MD1_SUPPORT
#define MTK_MD1_SUPPORT	(CONFIG_MTK_MD1_SUPPORT)
#else
#define MTK_MD1_SUPPORT	(5)
#endif

/* MD2 */
#ifdef CONFIG_MTK_ENABLE_MD2
#define MTK_MD2_EN	(1)
#else
#define MTK_MD2_EN	(0)
#endif

#ifdef CONFIG_MTK_MD2_SUPPORT
#define MTK_MD2_SUPPORT	(CONFIG_MTK_MD2_SUPPORT)
#else
#define MTK_MD2_SUPPORT	(1)
#endif

/* MD3 */
#ifdef CONFIG_MTK_ECCCI_C2K
#define MTK_MD3_EN	(1)
#else
#define MTK_MD3_EN	(0)
#endif

#ifdef CONFIG_MTK_MD3_SUPPORT
#define MTK_MD3_SUPPORT	(CONFIG_MTK_MD3_SUPPORT)
#else
#define MTK_MD3_SUPPORT	(2)
#endif

/* MD5 */
#ifdef CONFIG_MTK_ENABLE_MD5
#define MTK_MD5_EN	(1)
#else
#define MTK_MD5_EN	(0)
#endif
#ifdef CONFIG_MTK_MD5_SUPPORT
#define MTK_MD5_SUPPORT	(CONFIG_MTK_MD5_SUPPORT)
#else
#define MTK_MD5_SUPPORT	(3)
#endif

/*#define FEATURE_DFO_EN */
static fos_item_t ccci_fos_default_setting[] = {
	{"MTK_ENABLE_MD1", MTK_MD1_EN},
	{"MTK_MD1_SUPPORT", MTK_MD1_SUPPORT},
	{"MTK_ENABLE_MD2", MTK_MD2_EN},
	{"MTK_MD2_SUPPORT", MTK_MD2_SUPPORT},
	{"MTK_ENABLE_MD3", MTK_MD3_EN},
	{"MTK_MD3_SUPPORT", MTK_MD3_SUPPORT},
	{"MTK_ENABLE_MD5", MTK_MD5_EN},
	{"MTK_MD5_SUPPORT", MTK_MD5_SUPPORT},
};

/* Tag value from LK */
static unsigned char md_info_tag_val[4];
static unsigned int md_support[MAX_MD_NUM];
static unsigned int meta_md_support[MAX_MD_NUM];

/*--- MD setting collect */
/* modem index is not continuous, so there may be gap in this arrays */
static unsigned int md_usage_case;

static unsigned int md_resv_mem_size[MAX_MD_NUM];	/* MD ROM+RAM */
static unsigned int md_resv_smem_size[MAX_MD_NUM];	/* share memory */
static unsigned int md_resv_size_list[MAX_MD_NUM];
static unsigned int resv_smem_size;
static unsigned int md1md3_resv_smem_size;
static unsigned int md_env_rdy_flag;

static phys_addr_t md_resv_mem_list[MAX_MD_NUM];
static phys_addr_t md_resv_mem_addr[MAX_MD_NUM];
static phys_addr_t md_resv_smem_addr[MAX_MD_NUM];
static phys_addr_t resv_smem_addr;
static phys_addr_t md1md3_resv_smem_addr;

static char *md1_check_hdr_info;
static char *md3_check_hdr_info;
static int md1_check_hdr_info_size;
static int md3_check_hdr_info_size;

static int md1_raw_img_size;
static int md3_raw_img_size;

static int curr_ccci_fo_version;
#define CCCI_FO_VER_02			(2) /* For ubin */

int ccci_get_fo_setting(char item[], unsigned int *val)
{
	char *ccci_name;
	int ccci_value;
	int i;

	for (i = 0; i < ARRAY_SIZE(ccci_fos_default_setting); i++) {
		ccci_name = ccci_fos_default_setting[i].name;
		ccci_value = ccci_fos_default_setting[i].value;
		if (!strcmp(ccci_name, item)) {
			CCCI_UTIL_ERR_MSG("FO:%s -> %08x\n", item, ccci_value);
			*val = (unsigned int)ccci_value;
			return 0;
		}
	}
	CCCI_UTIL_ERR_MSG("FO:%s not found\n", item);
	return -CCCI_ERR_INVALID_PARAM;
}

/*--- LK tag and device tree ----- */
typedef struct _modem_info {
	unsigned long long base_addr;
	unsigned int size;
	char md_id;
	char errno;
	char md_type;
	char ver;
	unsigned int reserved[2];
} modem_info_t;
static unsigned long dt_chosen_node;
static unsigned int lk_load_img_status;
static int lk_load_img_err_no[MAX_MD_NUM];
static unsigned int md_type_at_lk[MAX_MD_NUM];

#define LK_LOAD_MD_EN			(1<<0)
#define LK_LOAD_MD_ERR_INVALID_MD_ID	(1<<1)
#define LK_LOAD_MD_ERR_NO_MD_LOAD	(1<<2)
#define LK_LOAD_MD_ERR_LK_INFO_FAIL	(1<<3)
#define LK_KERNEL_SETTING_MIS_SYNC	(1<<4)

/* functions will be called by external */
int get_lk_load_md_info(char buf[], int size)
{
	int has_write;

	if (lk_load_img_status & LK_LOAD_MD_EN)
		has_write = snprintf(buf, size, "LK Load MD:[Enabled](0x%08x)\n", lk_load_img_status);
	else
		has_write = snprintf(buf, size, "LK Load MD:[Disabled](0x%08x)\n", lk_load_img_status);

	if ((lk_load_img_status & (~0x1)) == 0) {
		has_write += snprintf(&buf[has_write], size - has_write, "LK load MD success!\n");
		return has_write;
	}

	has_write += snprintf(&buf[has_write], size - has_write, "LK load MD has error:\n");
	has_write += snprintf(&buf[has_write], size - has_write, "---- More details ----------------\n");
	if (lk_load_img_status & LK_LOAD_MD_ERR_INVALID_MD_ID)
		has_write += snprintf(&buf[has_write], size - has_write, "Err: Got invalid md id\n");
	if (lk_load_img_status & LK_LOAD_MD_ERR_NO_MD_LOAD)
		has_write += snprintf(&buf[has_write], size - has_write, "Err: No valid md image\n");
	if (lk_load_img_status & LK_LOAD_MD_ERR_LK_INFO_FAIL)
		has_write += snprintf(&buf[has_write], size - has_write, "Err: Got lk info fail\n");
	if (lk_load_img_status & LK_KERNEL_SETTING_MIS_SYNC)
		has_write += snprintf(&buf[has_write], size - has_write, "Err: lk kernel setting mis sync\n");
	has_write += snprintf(&buf[has_write], size - has_write, "ERR> 1:[%d] 2:[%d] 3:[%d] 4:[%d] 5:[%d]\n",
			lk_load_img_err_no[0], lk_load_img_err_no[1], lk_load_img_err_no[2], lk_load_img_err_no[3],
			lk_load_img_err_no[4]);

	return has_write;
}

/* function will be called by exteranl */
int get_md_type_from_lk(int md_id)
{
	if (md_id < MAX_MD_NUM)
		return md_type_at_lk[md_id];
	return 0;
}

static int __init early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	dt_chosen_node = node;
	return 1;
}

#define MAX_LK_INFO_SIZE	(0x10000)
#define CCCI_TAG_NAME_LEN	(16)
typedef struct _ccci_lk_info {
	unsigned long long lk_info_base_addr;
	unsigned int       lk_info_size;
	unsigned int       lk_info_tag_num;
} ccci_lk_info_t;

typedef struct _ccci_tag {
	char tag_name[CCCI_TAG_NAME_LEN];
	unsigned int data_offset;
	unsigned int data_size;
	unsigned int next_tag_offset;
} ccci_tag_t;

typedef struct _smem_layout {
	unsigned long long base_addr;
	unsigned int ap_md1_smem_offset;
	unsigned int ap_md1_smem_size;
	unsigned int ap_md3_smem_offset;
	unsigned int ap_md3_smem_size;
	unsigned int md1_md3_smem_offset;
	unsigned int md1_md3_smem_size;
	unsigned int total_smem_size;
} smem_layout_t;

static int find_ccci_tag_inf(void __iomem *lk_inf_base, unsigned int tag_cnt, char *name, char *buf, unsigned int size)
{
	unsigned int i;
	ccci_tag_t *tag;
	unsigned int cpy_size;
	char tag_name[64]; /* 1. For strcmp/strncmp should not be used on device memory, so prepare a temp buffer. */

	if (buf == NULL)
		return -1;

	tag = (ccci_tag_t *)lk_inf_base;
	CCCI_UTIL_INF_MSG("------curr tags:%s----------\n", name);
	for (i = 0; i < tag_cnt; i++) {
		#ifdef LK_LOAD_MD_INFO_DEBUG_EN
		CCCI_UTIL_INF_MSG("tag->name:%s\n", tag->tag_name);
		CCCI_UTIL_INF_MSG("tag->data_offset:%d\n", tag->data_offset);
		CCCI_UTIL_INF_MSG("tag->data_size:%d\n", tag->data_size);
		CCCI_UTIL_INF_MSG("tag->next_tag_offset:%d\n", tag->next_tag_offset);
		CCCI_UTIL_INF_MSG("tag value:%d\n", *(unsigned int *)(lk_inf_base+tag->data_offset));
		#endif
		cpy_size = strlen(tag->tag_name) + 1;
		/* 2. copy string from device memory for use strcmp. */
		memcpy_fromio(tag_name, tag->tag_name, cpy_size);
		if (strcmp(tag_name, name) != 0) {
			tag = (ccci_tag_t *)(lk_inf_base + tag->next_tag_offset);
			continue;
		}
		/* found it */
		cpy_size = size > tag->data_size?tag->data_size:size;
		memcpy_fromio(buf, (void *)(lk_inf_base + tag->data_offset), cpy_size);

		return cpy_size;
	}
	return -1;
}

/*===== META arguments parse =================== */
#define ATAG_MDINFO_DATA 0x41000806
struct lk_tag_header {
	u32 size;
	u32 tag;
};
static void parse_md_info_tag_val(unsigned int *raw_ptr)
{
	unsigned char *p;
	int i;
	int active_id = -1;
	unsigned char md_info_tag_array[4];

	/*--- md info tag item ---------------------------------- */
	/* unsigned int tag_size = lk_tag_header(2) + uchar[4](1) */
	/* unsigned int tag_key_value                             */
	/* uchar[0]kuchar[0],uchar[0],uchar[0],uchar[0]           */
	if (*raw_ptr != ((sizeof(struct lk_tag_header) + sizeof(md_info_tag_array))>>2)) {
		CCCI_UTIL_ERR_MSG("md info tag size mis-sync.(%d)\n", *raw_ptr);
		return;
	}
	raw_ptr++;
	if (*raw_ptr != ATAG_MDINFO_DATA) {
		CCCI_UTIL_ERR_MSG("md info tag key mis-sync.\n");
		return;
	}
	raw_ptr++;
	p = (unsigned char *)raw_ptr;
	for (i = 0; i < 4; i++)
		md_info_tag_array[i] = p[i];

	if (md_info_tag_array[1] & MD1_EN)
		active_id = MD_SYS1;
	else if (md_info_tag_array[1] & MD2_EN)
		active_id = MD_SYS2;
	else if (md_info_tag_array[1] & MD3_EN)
		active_id = MD_SYS3;
	else if (md_info_tag_array[1] & MD5_EN)
		active_id = MD_SYS5;
	else {
		CCCI_UTIL_ERR_MSG("META MD setting not found [%d][%d]\n",
			md_info_tag_array[0], md_info_tag_array[1]);
		return;
	}

	CCCI_UTIL_ERR_MSG("md info tag val: [0x%x][0x%x][0x%x][0x%x]\n",
				md_info_tag_array[0], md_info_tag_array[1],
				md_info_tag_array[2], md_info_tag_array[3]);

	if (active_id == MD_SYS1) {
		if (md_capability(MD_SYS1, md_info_tag_array[0], md_type_at_lk[MD_SYS1]))
			meta_md_support[active_id] = md_info_tag_array[0];
		else
			CCCI_UTIL_ERR_MSG("md_type:%d] not support wm_id %d\n",
				md_type_at_lk[MD_SYS1], md_info_tag_array[0]);
	}
}

static mpu_cfg_t md_mpu_cfg_list[10];
mpu_cfg_t *get_mpu_region_cfg_info(int region_id)
{
	int i;

	for (i = 0; i < 10; i++)
		if (md_mpu_cfg_list[i].region == region_id)
			return &md_mpu_cfg_list[i];

	return NULL;
}

static void lk_dt_info_collect(void)
{
	/* Device tree method */
	int ret;
	modem_info_t md_inf[4];
	modem_info_t *curr;
	int md_num;
	ccci_lk_info_t lk_inf;
	void __iomem *lk_inf_base;
	unsigned int tag_cnt;
	unsigned int *raw_ptr;
	smem_layout_t smem_layout;
	int md_id;

	/* This function will initialize dt_chosen_node */
	ret = of_scan_flat_dt(early_init_dt_get_chosen, NULL);
	if (ret == 0) {
		CCCI_UTIL_INF_MSG("device node no chosen node\n");
		return;
	}

	raw_ptr = (unsigned int *)of_get_flat_dt_prop(dt_chosen_node, "ccci,modem_info", NULL);
	if (raw_ptr == NULL) {
		CCCI_UTIL_INF_MSG("ccci,modem_info not found\n");
		return;
	}

	lk_load_img_status |= LK_LOAD_MD_EN;
	curr_ccci_fo_version = CCCI_FO_VER_02;

	memcpy((void *)&lk_inf, raw_ptr, sizeof(ccci_lk_info_t));
	if (lk_inf.lk_info_base_addr == 0LL) {
		CCCI_UTIL_ERR_MSG("no image load success\n");
		lk_load_img_status |= LK_LOAD_MD_ERR_NO_MD_LOAD;
		goto _Load_fail;
	}

	#ifdef LK_LOAD_MD_INFO_DEBUG_EN
	CCCI_UTIL_INF_MSG("lk info.lk_info_base_addr: 0x%llX\n", lk_inf.lk_info_base_addr);
	CCCI_UTIL_INF_MSG("lk info.lk_info_size:      0x%x\n", lk_inf.lk_info_size);
	CCCI_UTIL_INF_MSG("lk info.lk_info_tag_num:   0x%x\n", lk_inf.lk_info_tag_num);
	#endif

	lk_inf_base = ioremap_nocache((phys_addr_t)lk_inf.lk_info_base_addr, MAX_LK_INFO_SIZE);
	if (lk_inf_base == NULL) {
		CCCI_UTIL_ERR_MSG("ioremap lk info buf fail\n");
		lk_load_img_status |= LK_LOAD_MD_ERR_NO_MD_LOAD;
		goto _Load_fail;
	}
	tag_cnt = lk_inf.lk_info_tag_num;

	if (find_ccci_tag_inf(lk_inf_base, tag_cnt, "hdr_count", (char *)&md_num, sizeof(int)) != sizeof(int)) {
		CCCI_UTIL_ERR_MSG("get hdr_count fail\n");
		lk_load_img_status |= LK_LOAD_MD_ERR_NO_MD_LOAD;
		goto _Exit;
	}

	find_ccci_tag_inf(lk_inf_base, tag_cnt, "hdr_tbl_inf", (char *)md_inf, sizeof(md_inf));
	CCCI_UTIL_INF_MSG("tag_cnt:%d md_num:%d\n", tag_cnt, md_num);
	curr = md_inf;

	/* MD ROM and RW part */
	while (md_num--) {
		#ifdef LK_LOAD_MD_INFO_DEBUG_EN
		CCCI_UTIL_INF_MSG("===== Dump modem memory info (%d)=====\n", (int)sizeof(modem_info_t));
		CCCI_UTIL_INF_MSG("base address : 0x%llX\n", curr->base_addr);
		CCCI_UTIL_INF_MSG("memory size  : 0x%08X\n", curr->size);
		CCCI_UTIL_INF_MSG("md id        : %d\n", (int)curr->md_id);
		CCCI_UTIL_INF_MSG("ver          : %d\n", (int)curr->ver);
		CCCI_UTIL_INF_MSG("type         : %d\n", (int)curr->md_type);
		CCCI_UTIL_INF_MSG("errno        : %d\n", (int)curr->errno);
		CCCI_UTIL_INF_MSG("=============================\n");
		#endif
		md_id = (int)curr->md_id;
		if (curr->size) {
			MTK_MEMCFG_LOG_AND_PRINTK("[PHY layout]ccci_md%d at LK  :  0x%llx - 0x%llx  (0x%llx)\n",
				md_id, curr->base_addr,
				curr->base_addr + (unsigned long long)curr->size - 1LL,
				(unsigned long long)curr->size);
		}

		if ((md_id < MAX_MD_NUM) && (md_resv_mem_size[md_id] == 0)) {
			md_resv_mem_size[md_id] = curr->size;
			md_resv_mem_addr[md_id] = (phys_addr_t)curr->base_addr;
			if (curr->errno & 0x80)
				lk_load_img_err_no[md_id] = ((int)curr->errno) | 0xFFFFFF00; /*signed extension */
			else
				lk_load_img_err_no[md_id] = (int)curr->errno;

			CCCI_UTIL_INF_MSG("md%d lk_load_img_err_no:\n", md_id+1, lk_load_img_err_no[md_id]);

			if (lk_load_img_err_no[md_id] == 0)
				md_env_rdy_flag |= 1<<md_id;
			md_type_at_lk[md_id] = (int)curr->md_type;
			CCCI_UTIL_INF_MSG("md%d MemStart: 0x%016x, MemSize:0x%08X\n", md_id+1,
					(unsigned long long)md_resv_mem_addr[md_id], md_resv_mem_size[md_id]);
		} else {
			CCCI_UTIL_ERR_MSG("Invalid dt para, id(%d)\n", md_id);
			lk_load_img_status |= LK_LOAD_MD_ERR_INVALID_MD_ID;
		}
		curr++;
	}

	/* Get share memory layout */
	if (find_ccci_tag_inf(lk_inf_base, tag_cnt, "smem_layout", (char *)&smem_layout, sizeof(smem_layout_t))
			!= sizeof(smem_layout_t)) {
		CCCI_UTIL_ERR_MSG("Invalid dt para, id(%d)\n", (int)curr->md_id);
		lk_load_img_status |= LK_LOAD_MD_ERR_LK_INFO_FAIL;
		md_env_rdy_flag = 0; /* Reset to zero if get share memory info fail */
		goto _Exit;
	}
	MTK_MEMCFG_LOG_AND_PRINTK("[PHY layout]ccci_share_mem at LK  :  0x%llx - 0x%llx  (0x%llx)\n",
				smem_layout.base_addr,
				smem_layout.base_addr + (unsigned long long)smem_layout.total_smem_size - 1LL,
				(unsigned long long)smem_layout.total_smem_size);

	/* MD*_SMEM_SIZE */
	md_resv_smem_size[MD_SYS1] = smem_layout.ap_md1_smem_size;
	md_resv_smem_size[MD_SYS3] = smem_layout.ap_md3_smem_size;

	/* MD1MD3_SMEM_SIZE*/
	md1md3_resv_smem_size = smem_layout.md1_md3_smem_size;

	/* MD Share memory layout */
	/*   AP    <-->   MD1     */
	/*   MD1   <-->   MD3     */
	/*   AP    <-->   MD3     */
	md_resv_smem_addr[MD_SYS1] = (phys_addr_t)(smem_layout.base_addr +
					(unsigned long long)smem_layout.ap_md1_smem_offset);
	md1md3_resv_smem_addr = (phys_addr_t)(smem_layout.base_addr +
					(unsigned long long)smem_layout.md1_md3_smem_offset);
	md_resv_smem_addr[MD_SYS3] = (phys_addr_t)(smem_layout.base_addr +
					(unsigned long long)smem_layout.ap_md3_smem_offset);
	CCCI_UTIL_INF_MSG("AP  <--> MD1 SMEM(0x%08X):%016x~%016x\n", md_resv_smem_size[MD_SYS1],
			(unsigned long long)md_resv_smem_addr[MD_SYS1],
			(unsigned long long)(md_resv_smem_addr[MD_SYS1]+md_resv_smem_size[MD_SYS1]-1));
	CCCI_UTIL_INF_MSG("MD1 <--> MD3 SMEM(0x%08X):%016x~%016x\n", md1md3_resv_smem_size,
			(unsigned long long)md1md3_resv_smem_addr,
			(unsigned long long)(md1md3_resv_smem_addr+md1md3_resv_smem_size-1));
	CCCI_UTIL_INF_MSG("AP  <--> MD3 SMEM(0x%08X):%016x~%016x\n", md_resv_smem_size[MD_SYS3],
			(unsigned long long)md_resv_smem_addr[MD_SYS3],
			(unsigned long long)(md_resv_smem_addr[MD_SYS3]+md_resv_smem_size[MD_SYS3]-1));

	if (md_usage_case & (1<<MD_SYS1)) {
		/* The allocated memory will be free after md structure initialized */
		md1_check_hdr_info = kmalloc(sizeof(struct md_check_header_v5), GFP_KERNEL);
		if (md1_check_hdr_info == NULL) {
			CCCI_UTIL_ERR_MSG("allocate check header memory fail for md1\n");
			md_env_rdy_flag &= ~(1<<MD_SYS1);
			goto _Exit;
		}
		if (find_ccci_tag_inf(lk_inf_base, tag_cnt, "md1_chk", md1_check_hdr_info,
				sizeof(struct md_check_header_v5)) != sizeof(struct md_check_header_v5)) {
			CCCI_UTIL_ERR_MSG("get md1 chk header info fail\n");
			lk_load_img_status |= LK_LOAD_MD_ERR_LK_INFO_FAIL;
			md_env_rdy_flag &= ~(1<<MD_SYS1);
			goto _Exit;
		}
		md1_check_hdr_info_size = sizeof(struct md_check_header_v5);
	}
	/* Get MD1 raw image size */
	find_ccci_tag_inf(lk_inf_base, tag_cnt, "md1img", (char *)&md1_raw_img_size, sizeof(int));

	/* Get MD1 MPU config info */
	find_ccci_tag_inf(lk_inf_base, tag_cnt, "md1_mpu", (char *)md_mpu_cfg_list, sizeof(md_mpu_cfg_list));

	/* Get META settings at device tree, only MD1 use this */
	/* These code must at last of this function for some varialbe need updated first */
	raw_ptr = (unsigned int *)of_get_flat_dt_prop(dt_chosen_node, "atag,mdinfo", NULL);
	if (raw_ptr == NULL)
		CCCI_UTIL_INF_MSG("atag,mdinfo not found\n");
	else
		parse_md_info_tag_val(raw_ptr);

	if (md_usage_case & (1<<MD_SYS3)) {
		/* The allocated memory will be free after md structure initialized */
		md3_check_hdr_info = kmalloc(sizeof(struct md_check_header), GFP_KERNEL);
		if (md3_check_hdr_info == NULL) {
			CCCI_UTIL_ERR_MSG("allocate check header memory fail for md3\n");
			md_env_rdy_flag &= ~(1<<MD_SYS3);
			goto _Exit;
		}
		if (find_ccci_tag_inf(lk_inf_base, tag_cnt, "md3_chk", md3_check_hdr_info,
				sizeof(struct md_check_header)) != sizeof(struct md_check_header)) {
			CCCI_UTIL_ERR_MSG("get md3 chk header info fail\n");
			lk_load_img_status |= LK_LOAD_MD_ERR_LK_INFO_FAIL;
			md_env_rdy_flag &= ~(1<<MD_SYS3);
			goto _Exit;
		}
		md3_check_hdr_info_size = sizeof(struct md_check_header);
	}
	/* Get MD3 raw image size */
	find_ccci_tag_inf(lk_inf_base, tag_cnt, "md3img", (char *)&md3_raw_img_size, sizeof(int));

_Exit:
	iounmap(lk_inf_base);

_Load_fail:
	CCCI_UTIL_INF_MSG("=====Parsing done====================\n");

	/* Show warning if has some error */
	/* MD1 part */
	if ((md_usage_case & (1<<MD_SYS1)) && (!(md_env_rdy_flag & (1<<MD_SYS1)))) {
		CCCI_UTIL_ERR_MSG("md1 env prepare abnormal, disable this modem\n");
		md_usage_case &= ~(1<<MD_SYS1);
	} else if ((!(md_usage_case & (1<<MD_SYS1))) && (md_env_rdy_flag & (1<<MD_SYS1))) {
		CCCI_UTIL_ERR_MSG("md1: kernel dis, but lk en\n");
		md_usage_case &= ~(1<<MD_SYS1);
		lk_load_img_status |= LK_KERNEL_SETTING_MIS_SYNC;
	} else if ((!(md_usage_case & (1<<MD_SYS1))) && (!(md_env_rdy_flag & (1<<MD_SYS1)))) {
		CCCI_UTIL_INF_MSG("md1: both lk and kernel dis\n");
		md_usage_case &= ~(1<<MD_SYS1);
		lk_load_img_err_no[MD_SYS1] = 0; /* For this case, clear error */
	}
	/* MD3 part */
	if ((md_usage_case & (1<<MD_SYS3)) && (!(md_env_rdy_flag & (1<<MD_SYS3)))) {
		CCCI_UTIL_ERR_MSG("md3 env prepare abnormal, disable this modem\n");
		md_usage_case &= ~(1<<MD_SYS3);
	} else if ((!(md_usage_case & (1<<MD_SYS3))) && (md_env_rdy_flag & (1<<MD_SYS3))) {
		CCCI_UTIL_ERR_MSG("md3: kernel dis, but lk en\n");
		md_usage_case &= ~(1<<MD_SYS3);
		lk_load_img_status |= LK_KERNEL_SETTING_MIS_SYNC;
	} else if ((!(md_usage_case & (1<<MD_SYS3))) && (!(md_env_rdy_flag & (1<<MD_SYS3)))) {
		CCCI_UTIL_INF_MSG("md3: both lk and kernel dis\n");
		md_usage_case &= ~(1<<MD_SYS3);
		lk_load_img_err_no[MD_SYS3] = 0; /* For this case, clear error */
	}
}

int get_md_img_raw_size(int md_id)
{
	switch (md_id) {
	case MD_SYS1:
		return md1_raw_img_size;
	case MD_SYS3:
		return md3_raw_img_size;
	default:
		return 0;
	}
	return 0;
}

int get_raw_check_hdr(int md_id, char buf[], int size)
{
	char *chk_hdr_ptr = NULL;
	int cpy_size = 0;
	int ret = -1;

	if (buf == NULL)
		return -1;
	switch (md_id) {
	case MD_SYS1:
		chk_hdr_ptr = md1_check_hdr_info;
		cpy_size = md1_check_hdr_info_size;
		break;
	case MD_SYS3:
		chk_hdr_ptr = md3_check_hdr_info;
		cpy_size = md3_check_hdr_info_size;
		break;
	default:
		break;
	}
	if (chk_hdr_ptr == NULL)
		return ret;

	cpy_size = cpy_size > size?size:cpy_size;
	memcpy(buf, chk_hdr_ptr, cpy_size);

	return cpy_size;
}
/*--- META arguments parse ------- */
static int ccci_parse_meta_md_setting(unsigned char args[])
{
	unsigned char md_active_setting = args[1];
	unsigned char md_setting_flag = args[0];
	int active_id = -1;

	if (md_active_setting & MD1_EN)
		active_id = MD_SYS1;
	else if (md_active_setting & MD2_EN)
		active_id = MD_SYS2;
	else if (md_active_setting & MD3_EN)
		active_id = MD_SYS3;
	else if (md_active_setting & MD5_EN)
		active_id = MD_SYS5;
	else
		CCCI_UTIL_ERR_MSG("META MD setting not found [%d][%d]\n", args[0], args[1]);

	switch (active_id) {
	case MD_SYS1:
	case MD_SYS2:
	case MD_SYS3:
	case MD_SYS5:
		if (md_setting_flag == MD_2G_FLAG)
			meta_md_support[active_id] = modem_2g;
		else if (md_setting_flag == MD_WG_FLAG)
			meta_md_support[active_id] = modem_wg;
		else if (md_setting_flag == MD_TG_FLAG)
			meta_md_support[active_id] = modem_tg;
		else if (md_setting_flag == MD_LWG_FLAG)
			meta_md_support[active_id] = modem_lwg;
		else if (md_setting_flag == MD_LTG_FLAG)
			meta_md_support[active_id] = modem_ltg;
		else if (md_setting_flag & MD_SGLTE_FLAG)
			meta_md_support[active_id] = modem_sglte;
		CCCI_UTIL_INF_MSG("META MD%d to type:%d\n", active_id + 1, meta_md_support[active_id]);
		break;
	}
	return 0;
}

int get_modem_support_cap(int md_id)
{
	if (md_id < MAX_MD_NUM) {
		if (((get_boot_mode() == META_BOOT) || (get_boot_mode() == ADVMETA_BOOT))
		    && (meta_md_support[md_id] != 0))
			return meta_md_support[md_id];
		else
			return md_support[md_id];
	}
	return -1;
}

int set_modem_support_cap(int md_id, int new_val)
{
	if (md_id < MAX_MD_NUM) {
		if (((get_boot_mode() == META_BOOT) || (get_boot_mode() == ADVMETA_BOOT))
			&& (meta_md_support[md_id] != 0)) {
			if (curr_ccci_fo_version == CCCI_FO_VER_02) {
				/* UBin version */
				/* Priority: boot arg > NVRAM > default */
				if (meta_md_support[md_id] == 0) {
					CCCI_UTIL_INF_MSG("md%d: meta new wmid:%d\n", md_id + 1, new_val);
					meta_md_support[md_id] = new_val;
				} else {
					CCCI_UTIL_INF_MSG("md%d: boot arg has val:%d(%d)\n", md_id + 1,
							meta_md_support[md_id], new_val);
					/* We hope first write clear meta default setting
					** then, modem first boot will using meta setting that get from boot arguments*/
					meta_md_support[md_id] = 0;
					return -1;
				}
			} else {
				/* Legcy version */
				CCCI_UTIL_INF_MSG("md%d: meta legcy md type:%d\n", md_id + 1, new_val);
					meta_md_support[md_id] = new_val;
			}
		} else {
			CCCI_UTIL_INF_MSG("md%d: new mdtype(/wmid):%d\n", md_id + 1, new_val);
			md_support[md_id] = new_val;
		}
		return 0;
	}
	return -1;
}

int get_md_resv_mem_info(int md_id, phys_addr_t *r_rw_base, unsigned int *r_rw_size, phys_addr_t *srw_base,
			 unsigned int *srw_size)
{
	if (md_id >= MAX_MD_NUM)
		return -1;

	if (r_rw_base != NULL)
		*r_rw_base = md_resv_mem_addr[md_id];

	if (r_rw_size != NULL)
		*r_rw_size = md_resv_mem_size[md_id];

	if (srw_base != NULL)
		*srw_base = md_resv_smem_addr[md_id];

	if (srw_size != NULL)
		*srw_size = md_resv_smem_size[md_id];

	return 0;
}

int get_md1_md3_resv_smem_info(int md_id, phys_addr_t *rw_base, unsigned int *rw_size)
{
	if ((md_id != MD_SYS1) && (md_id != MD_SYS3))
		return -1;

	if (rw_base != NULL)
		*rw_base = md1md3_resv_smem_addr;

	if (rw_size != NULL)
		*rw_size = md1md3_resv_smem_size;

	return 0;
}

unsigned int get_md_smem_align(int md_id)
{
	return 0x4000;
}

unsigned int get_modem_is_enabled(int md_id)
{
	return !!(md_usage_case & (1 << md_id));
}

static void cal_md_settings(int md_id)
{
	unsigned int tmp;
	unsigned int md_en = 0;
	char tmp_buf[30];
	char *node_name = NULL;
	struct device_node *node = NULL;

	snprintf(tmp_buf, sizeof(tmp_buf), "MTK_ENABLE_MD%d", (md_id + 1));
	/* MTK_ENABLE_MD* */
	if (ccci_get_fo_setting(tmp_buf, &tmp) == 0) {
		if (tmp > 0)
			md_en = 1;
	}
	if (!(md_en && (md_usage_case & (1 << md_id)))) {
		CCCI_UTIL_INF_MSG_WITH_ID(md_id, "md%d is disabled\n", (md_id + 1));
		return;
	}
	/* MTK_MD*_SUPPORT */
	snprintf(tmp_buf, sizeof(tmp_buf), "MTK_MD%d_SUPPORT", (md_id + 1));
	if (ccci_get_fo_setting(tmp_buf, &tmp) == 0)
		md_support[md_id] = tmp;

	/* MD*_SMEM_SIZE */
	if (md_id == MD_SYS1) {
		node_name = "mediatek,mdcldma";
	} else if (md_id == MD_SYS2) {
		node_name = "mediatek,ap_ccif1";
	} else if (md_id == MD_SYS3) {
		node_name = "mediatek,ap2c2k_ccif";
	} else {
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "md%d id is not supported,need to check\n", (md_id + 1));
		md_usage_case &= ~(1 << md_id);
		return;
	}
	node = of_find_compatible_node(NULL, NULL, node_name);
	if (node) {
		of_property_read_u32(node, "mediatek,md_smem_size", &md_resv_smem_size[md_id]);
	} else {
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "md%d smem size is not set in device tree,need to check\n",
					  (md_id + 1));
		md_usage_case &= ~(1 << md_id);
		return;
	}
	/* MD ROM start address should be 32M align as remap hardware limitation */
	md_resv_mem_addr[md_id] = md_resv_mem_list[md_id];
	/*
	 * for legacy CCCI: make share memory start address to be 2MB align, as share
	 * memory size is 2MB - requested by MD MPU.
	 * for ECCCI: ROM+RAM size will be align to 1M, and share memory is 2K,
	 * 1M alignment is also 2K alignment.
	 */
	md_resv_mem_size[md_id] = round_up(md_resv_size_list[md_id] - md_resv_smem_size[md_id],
			get_md_smem_align(md_id));
	md_resv_smem_addr[md_id] = md_resv_mem_list[md_id] + md_resv_mem_size[md_id];

	CCCI_UTIL_INF_MSG_WITH_ID(md_id, "md%d modem_total_size=0x%x,md_size=0x%x, smem_size=0x%x\n", (md_id + 1),
				md_resv_size_list[md_id], md_resv_mem_size[md_id], md_resv_smem_size[md_id]);

	if ((md_usage_case & (1 << md_id)) && ((md_resv_mem_addr[md_id] & (CCCI_MEM_ALIGN - 1)) != 0))
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "md%d memory addr is not 32M align!!!\n", (md_id + 1));

	if ((md_usage_case & (1 << md_id)) && ((md_resv_smem_addr[md_id] & (CCCI_SMEM_ALIGN_MD1 - 1)) != 0))
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "md%d share memory addr %p is not 0x%x align!!\n", (md_id + 1),
			&md_resv_smem_addr[md_id], CCCI_SMEM_ALIGN_MD1);

	CCCI_UTIL_INF_MSG_WITH_ID(md_id, "MemStart: %016x, MemSize:0x%08X\n",
		md_resv_mem_addr[md_id], md_resv_mem_size[md_id]);
	CCCI_UTIL_INF_MSG_WITH_ID(md_id, "SMemStart: %016x, SMemSize:0x%08X\n",
		md_resv_smem_addr[md_id], md_resv_smem_size[md_id]);
}

static void cal_md_settings_v2(struct device_node *node)
{
	unsigned int tmp;
	char tmp_buf[30];
	int i;

	CCCI_UTIL_INF_MSG("using kernel dt mem setting for md\n");

	/* MTK_MD*_SUPPORT */
	for (i  = 0; i < MAX_MD_NUM; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "MTK_MD%d_SUPPORT", (i + 1));
		if (ccci_get_fo_setting(tmp_buf, &tmp) == 0)
			md_support[i] = tmp;
	}

	/* lk_dt_info_collect may set "LK_LOAD_MD_EN" flag if found dt node success*/
	if (lk_load_img_status & LK_LOAD_MD_EN)
		return;

	/* MD*_SMEM_SIZE */
	for (i = 0; i < MAX_MD_NUM; i++) {
		snprintf(tmp_buf, 30, "mediatek,md%d-smem-size", i+1);
		if (0 == of_property_read_u32(node, tmp_buf, &tmp)) {
			CCCI_UTIL_INF_MSG("DT[%s]:%08X\n", tmp_buf, tmp);
			md_resv_smem_size[MD_SYS1+i] = tmp;
		} else
			CCCI_UTIL_INF_MSG("DT[%s]:%08X\n", tmp_buf, md_resv_smem_size[MD_SYS1+i]);
	}

	/* MD1MD3_SMEM_SIZE*/
	snprintf(tmp_buf, 30, "mediatek,md1md3-smem-size");
	if (0 == of_property_read_u32(node, tmp_buf, &tmp)) {
		CCCI_UTIL_INF_MSG("DT[%s]:%08X\n", tmp_buf, tmp);
		md1md3_resv_smem_size = tmp;
	} else
		CCCI_UTIL_INF_MSG("DT[%s]:%08X\n", tmp_buf, md1md3_resv_smem_size);

	/* CFG version */
	snprintf(tmp_buf, 30, "mediatek,version");
	tmp = 0;
	of_property_read_u32(node, tmp_buf, &tmp);
	CCCI_UTIL_INF_MSG("DT[%s]:%08X\n", tmp_buf, tmp);
	if (tmp != 1) {
		CCCI_UTIL_INF_MSG("Un-support version:%d\n", tmp);
		return;
	}

	/* MD ROM and RW part */
	for (i = 0; i < MAX_MD_NUM; i++) {
		if (md_usage_case & (1 << i)) {
			md_resv_mem_size[i] = md_resv_size_list[i];
			md_resv_mem_addr[i] = md_resv_mem_list[i];
			CCCI_UTIL_INF_MSG("md%d MemStart: 0x%016x, MemSize:0x%08X\n", i+1,
				(unsigned long long)md_resv_mem_addr[i], md_resv_mem_size[i]);
		}
	}

	/* MD Share memory part */
	/* AP  <--> MD1 */
	/* MD1 <--> MD3 */
	/* AP  <--> MD3 */
	md_resv_smem_addr[MD_SYS1] = resv_smem_addr;
	if (md_usage_case & (1 << MD_SYS3)) {
		md1md3_resv_smem_addr = resv_smem_addr + md_resv_smem_size[MD_SYS1];
		md_resv_smem_addr[MD_SYS3] = md1md3_resv_smem_addr + md1md3_resv_smem_size;
	} else {
		md1md3_resv_smem_addr = 0;
		md1md3_resv_smem_size = 0;
		md_resv_smem_addr[MD_SYS3] = 0;
		md_resv_smem_size[MD_SYS3] = 0;
	}
	CCCI_UTIL_INF_MSG("AP  <--> MD1 SMEM(0x%08X):%016x~%016x\n", md_resv_smem_size[MD_SYS1],
			(unsigned long long)md_resv_smem_addr[MD_SYS1],
			(unsigned long long)(md_resv_smem_addr[MD_SYS1]+md_resv_smem_size[MD_SYS1]-1));
	CCCI_UTIL_INF_MSG("MD1 <--> MD3 SMEM(0x%08X):%016x~%016x\n", md1md3_resv_smem_size,
			(unsigned long long)md1md3_resv_smem_addr,
			(unsigned long long)(md1md3_resv_smem_addr+md1md3_resv_smem_size-1));
	CCCI_UTIL_INF_MSG("AP  <--> MD3 SMEM(0x%08X):%016x~%016x\n", md_resv_smem_size[MD_SYS3],
			(unsigned long long)md_resv_smem_addr[MD_SYS3],
			(unsigned long long)(md_resv_smem_addr[MD_SYS3]+md_resv_smem_size[MD_SYS3]-1));
}

int modem_run_env_ready(int md_id)
{
	return md_env_rdy_flag & (1<<md_id);
}

void ccci_md_mem_reserve(void)
{
	CCCI_UTIL_INF_MSG("ccci_md_mem_reserve phased out.\n");
}

#ifdef CONFIG_OF_RESERVED_MEM
#define CCCI_MD1_MEM_RESERVED_KEY "mediatek,reserve-memory-ccci_md1"
#define CCCI_MD2_MEM_RESERVED_KEY "mediatek,reserve-memory-ccci_md2"
#define CCCI_MD3_MEM_RESERVED_KEY "mediatek,reserve-memory-ccci_md3_ccif"
#define CCCI_MD1MD3_SMEM_RESERVED_KEY "mediatek,reserve-memory-ccci_share"
#include <mt-plat/mtk_memcfg.h>
int ccci_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;
	int md_id = -1;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;
	if (strstr(CCCI_MD1_MEM_RESERVED_KEY, rmem->name))
		md_id = MD_SYS1;
	else if (strstr(CCCI_MD2_MEM_RESERVED_KEY, rmem->name))
		md_id = MD_SYS2;
	else if (strstr(CCCI_MD3_MEM_RESERVED_KEY, rmem->name))
		md_id = MD_SYS3;
	else {
		if (strstr(CCCI_MD1MD3_SMEM_RESERVED_KEY, rmem->name)) {
			CCCI_UTIL_INF_MSG("reserve_mem_of_init, rptr=0x%pa, rsize=0x%x\n", &rptr, rsize);
			resv_smem_addr = rptr;
			resv_smem_size = rsize;
		} else
			CCCI_UTIL_INF_MSG("memory reserve key %s not support\n", rmem->name);

		return 0;
	}
	CCCI_UTIL_INF_MSG("reserve_mem_of_init, rptr=0x%pa, rsize=0x%x\n", &rptr, rsize);
	md_resv_mem_list[md_id] = rptr;
	md_resv_size_list[md_id] = rsize;
	md_usage_case |= (1 << md_id);
	return 0;
}

RESERVEDMEM_OF_DECLARE(ccci_reserve_mem_md1_init, CCCI_MD1_MEM_RESERVED_KEY, ccci_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(ccci_reserve_mem_md2_init, CCCI_MD2_MEM_RESERVED_KEY, ccci_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(ccci_reserve_mem_md3_init, CCCI_MD3_MEM_RESERVED_KEY, ccci_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(ccci_reserve_smem_md1md3_init, CCCI_MD1MD3_SMEM_RESERVED_KEY, ccci_reserve_mem_of_init);
#endif
int ccci_util_fo_init(void)
{
	int idx;
	struct device_node *node = NULL;
	unsigned int tmp;
	CCCI_UTIL_INF_MSG("ccci_util_fo_init 0.\n");

	if (!md_usage_case) {
		/* Enter here mean's kernel dt not reserve memory */
		/* So, change to using kernel option to deside if modem is enabled */
		if (ccci_get_fo_setting("MTK_ENABLE_MD1", &tmp) == 0) {
			if (tmp > 0)
				md_usage_case |= (1 << MD_SYS1);
		}
		if (ccci_get_fo_setting("MTK_ENABLE_MD3", &tmp) == 0) {
			if (tmp > 0)
				md_usage_case |= (1 << MD_SYS3);
		}
	}

	lk_dt_info_collect();

	node = of_find_compatible_node(NULL, NULL, "mediatek,ccci_util_cfg");
	if (node == NULL) {
		CCCI_UTIL_INF_MSG("using v1.\n");
		/* lk_meta_tag_info_collect(); */
		/* Parse META setting */
		ccci_parse_meta_md_setting(md_info_tag_val);

		/* Calculate memory layout */
		for (idx = 0; idx < MAX_MD_NUM; idx++)
			cal_md_settings(idx);
	} else {
		CCCI_UTIL_INF_MSG("using v2.\n");
		cal_md_settings_v2(node);
	}
	CCCI_UTIL_INF_MSG("ccci_util_fo_init 2.\n");
	return 0;
}
