#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
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

#include "ccci_util_log.h"
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
static unsigned long dt_chosen_node;
static int __init early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	dt_chosen_node = node;
	return 1;
}

static void lk_meta_tag_info_collect(void)
{
	/* Device tree method */
	char *tags;
	int ret;
	ret = of_scan_flat_dt(early_init_dt_get_chosen, NULL);
	if (ret == 0) {
		CCCI_UTIL_INF_MSG("device node no chosen node\n");
		return;
	}
	tags = (char *)of_get_flat_dt_prop(dt_chosen_node, "atag,mdinfo", NULL);
	if (tags) {
		tags += 8;	/* Fix me, Arm64 doesn't have atag defination now */
		md_info_tag_val[0] = tags[0];
		md_info_tag_val[1] = tags[1];
		md_info_tag_val[2] = tags[2];
		md_info_tag_val[3] = tags[3];
		CCCI_UTIL_INF_MSG("Get MD info Tags\n");
		CCCI_UTIL_INF_MSG("md_inf[0]=%d\n", md_info_tag_val[0]);
		CCCI_UTIL_INF_MSG("md_inf[1]=%d\n", md_info_tag_val[1]);
		CCCI_UTIL_INF_MSG("md_inf[2]=%d\n", md_info_tag_val[2]);
		CCCI_UTIL_INF_MSG("md_inf[3]=%d\n", md_info_tag_val[3]);
	} else {
		CCCI_UTIL_INF_MSG("atag,mdinfo=NULL\n");
	}
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
		    && (meta_md_support[md_id] != 0))
			meta_md_support[md_id] = new_val;
		else
			md_support[md_id] = new_val;
		return 0;
	}
	return -1;
}

/*--- MD setting collect */
/* modem index is not continuous, so there may be gap in this arrays */
static unsigned int md_usage_case;

static unsigned int md_resv_mem_size[MAX_MD_NUM];	/* MD ROM+RAM */
static unsigned int md_resv_smem_size[MAX_MD_NUM];	/* share memory */
static unsigned int md_resv_size_list[MAX_MD_NUM];
static unsigned int resv_smem_size;
static unsigned int md1md3_resv_smem_size;

static phys_addr_t md_resv_mem_list[MAX_MD_NUM];
static phys_addr_t md_resv_mem_addr[MAX_MD_NUM];
static phys_addr_t md_resv_smem_addr[MAX_MD_NUM];
static phys_addr_t resv_smem_addr;
static phys_addr_t md1md3_resv_smem_addr;

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
		CCCI_UTIL_ERR_MSG_WITH_ID(md_id, "md%d share memory addr %pa is not 0x%x align!!\n", (md_id + 1),
					  &md_resv_smem_addr[md_id], CCCI_SMEM_ALIGN_MD1);

	CCCI_UTIL_INF_MSG_WITH_ID(md_id, "MemStart: 0x%pa, MemSize:0x%08X\n", &md_resv_mem_addr[md_id],
				  md_resv_mem_size[md_id]);
	CCCI_UTIL_INF_MSG_WITH_ID(md_id, "SMemStart: 0x%pa, SMemSize:0x%08X\n", &md_resv_smem_addr[md_id],
				  md_resv_smem_size[md_id]);
}

static void cal_md_settings_v2(struct device_node *node)
{
	unsigned int tmp;
	char tmp_buf[30];
	int i;

	/* MTK_MD*_SUPPORT */
	for (i  = 0; i < MAX_MD_NUM; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "MTK_MD%d_SUPPORT", (i + 1));
		if (ccci_get_fo_setting(tmp_buf, &tmp) == 0)
			md_support[i] = tmp;
	}

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
			CCCI_UTIL_INF_MSG("md%d MemStart: 0x%p, MemSize:0x%08X\n", i+1, (void *)md_resv_mem_addr[i],
				  md_resv_mem_size[i]);
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
	CCCI_UTIL_INF_MSG("AP  <--> MD1 SMEM(0x%08X):%p~%p\n", md_resv_smem_size[MD_SYS1],
			(void *)md_resv_smem_addr[MD_SYS1],
			(void *)(md_resv_smem_addr[MD_SYS1]+md_resv_smem_size[MD_SYS1]-1));
	CCCI_UTIL_INF_MSG("MD1 <--> MD3 SMEM(0x%08X):%p~%p\n", md1md3_resv_smem_size,
			(void *)md1md3_resv_smem_addr,
			(void *)(md1md3_resv_smem_addr+md1md3_resv_smem_size-1));
	CCCI_UTIL_INF_MSG("AP  <--> MD3 SMEM(0x%08X):%p~%p\n", md_resv_smem_size[MD_SYS3],
			(void *)md_resv_smem_addr[MD_SYS3],
			(void *)(md_resv_smem_addr[MD_SYS3]+md_resv_smem_size[MD_SYS3]-1));
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
			CCCI_UTIL_INF_MSG("reserve_mem_of_init, rptr=0x%p, rsize=0x%x\n", &rptr, rsize);
			resv_smem_addr = rptr;
			resv_smem_size = rsize;
		} else
			CCCI_UTIL_INF_MSG("memory reserve key %s not support\n", uname);

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
	CCCI_UTIL_INF_MSG("ccci_util_fo_init 0.\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,ccci_util_cfg");
	if (node == NULL) {
		CCCI_UTIL_INF_MSG("using v1.\n");
		lk_meta_tag_info_collect();
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
