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

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <ccci_common.h>
#include <ccci_platform.h>
#include <mach/mt_clkmgr.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#endif
#endif

#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
#include <sec_osal.h>
#include <sec_export.h>
#endif

#ifdef ENABLE_EMI_PROTECTION
#include <mach/emi_mpu.h>
#endif
/* -------------ccci initial status define----------------------*/
#define CCCI_ALLOC_SMEM_DONE	(1<<0)
#define CCCI_MAP_MD_CODE_DONE	(1<<1)
#define CCCI_MAP_CTL_REG_DONE	(1<<2)
#define CCCI_WDT_IRQ_REG_DONE	(1<<3)
#define CCCI_SEC_INIT_DONE		(1<<4)
#define CCCI_SEC_CHECK_DONE		(1<<5)

static unsigned int md_init_stage_flag[MAX_MD_NUM];
static int smem_remapped;
static struct platform_device *ccif_plat_drv;

/* -------------ccci load md&dsp image define----------------*/
#define IMG_NAME_LEN    32

static char ap_platform[16] = "";
static struct ccci_image_info md_img_info[1];

static int img_is_dbg_ver[MAX_MD_NUM];
static char md_image_post_fix[MAX_MD_NUM][12];
struct ccif_hw_info_t s_ccif_hw_info;

static unsigned int md_wdt_irq_id[MAX_MD_NUM];

/* =============================================== */
/*  modem hardware control section */
/* =============================================== */
/* -------------physical&virtual address define-----------------*/
#define KERN_EMI_BASE             (0x80000000)

static unsigned int ap_infra_base;
static unsigned int ap_mcu_reg_base;

static unsigned int md1_rgu_base;
static unsigned int md1_ccif_base;
static unsigned int md1_boot_slave_Vector;
static unsigned int md1_boot_slave_Key;
static unsigned int md1_boot_slave_En;
#ifdef ENABLE_DUMP_MD_REGISTER
#define MD1_MCU_ADDR_BASE 0x20000000
#define MD1_M3D_ADDR_BASE 0x23000000
static unsigned char *md1_mcu_ptr;
static unsigned char *md1_m3d_ptr;

#define MD1_OS_TIMER_ADDR        (0x20040000)
#define MD1_OS_TIMER_LEN         (0x60)
/*  To dump: 0x80040000 - 0x80040060  (MD side) */

#define MD1_MIXEDSYS_ADDR        (0x20120000)
#define MD1_MIXEDSYS_LEN         (0x71c)
/*  To dump: 0x80120040 - 0x801200B4  (MD side) */
/*           0x80120100 - 0x80120110 */
/*           0x80120140 - 0x8012014C */

#define MD1_MD_TOPSM_ADDR        (0x20030000)
#define MD1_MD_TOPSM_LEN         (0xA38)
/*  To dump: 0x80030000 - 0x80030110  (MD side) */
/*           0x80030200 */
/*           0x80030300 - 0x80030320 */
/*           0x80030800 - 0x80030910 */
/*           0x80030A00 - 0x80030A50 */

#define MD1_MODEM_TOPSM_ADDR     (0x23010000)
#define MD1_MODEM_TOPSM_LEN      (0xA38)
/*  To dump: 0x83010000 - 0x83010110  (MD side) */
/*           0x83010200 */
/*           0x83010300 - 0x83010320 */
/*           0x83010800 - 0x83010910 */
/*           0x83010A00 - 0x83010A50 */

static unsigned char *md1_ost_ptr;
static unsigned char *md1_mixedsys_ptr;
static unsigned char *md1_md_topsm_ptr;
static unsigned char *md1_modem_topsm_ptr;

#endif

static unsigned long md1_dbg_mode_ptr;

/* -------------md&dsp watchdog define----------------*/
typedef int (*int_func_int_t) (int);
static int_func_int_t wdt_notify_array[MAX_MD_NUM];
static atomic_t wdt_irq_en_count[MAX_MD_NUM];

/* -------------md gate&ungate function----------------*/

/* -------------power on/off md function----------------*/
static DEFINE_SPINLOCK(md1_power_change_lock);
static int md1_power_on_cnt;

/*--------------MD WDT recover ----------------------*/
static DEFINE_SPINLOCK(md1_wdt_mon_lock);
static void recover_md_wdt_irq(unsigned long data);
static DEFINE_TIMER(md1_wdt_recover_timer, recover_md_wdt_irq, 0, MD_SYS1);
static unsigned int md_wdt_has_processed[MAX_MD_NUM];

/* -------------ccci log filter define---------------------*/
unsigned int ccci_msg_mask[MAX_MD_NUM];

/*  JTAG Debug using */
#define DEBUG_SETTING_DEFAULT    (0)
static unsigned int debug_setting_flag;

#if 0
static unsigned int get_chip_version(void)
{
#ifdef ENABLE_CHIP_VER_CHECK
	if (get_chip_sw_ver() == MD_SW_V1)
		return CHIP_SW_VER_01;
	else if (get_chip_sw_ver() == MD_SW_V2)
		return CHIP_SW_VER_02;
	else
		return CHIP_SW_VER_02;
#else
	return CHIP_SW_VER_01;
#endif
}
#endif

static void ccci_get_platform_ver(char *ver)
{
#ifdef ENABLE_CHIP_VER_CHECK
	sprintf(ver, "MT%04x_S%02x", mt_get_chip_hw_ver(), (mt_get_chip_hw_subcode() & 0xFF));
#else
	sprintf(ver, "MT6580_S00");
#endif
}

char *ccci_get_ap_platform(void)
{
	return ap_platform;
}

int is_modem_debug_ver(int md_id)
{
	return img_is_dbg_ver[md_id];
}

int get_dts_settings(int md_id)
{

#ifdef CONFIG_OF
	struct device_node *node = NULL;
	char *node_name = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	if (!node) {
		CCCI_ERR_INF(md_id, "plt",
			     "md%d INFRACFG_AO is not set in DT!\n",
			     (md_id + 1));
		/*  return -1; */
	} else {
		ap_infra_base = (unsigned int)of_iomap(node, 0);
		CCCI_MSG_INF(md_id, "plt", "md%d get reg ap_infra_base:0x%x!\n",
			     (md_id + 1), ap_infra_base);
	}

	node =
	    of_find_compatible_node(NULL, NULL, "mediatek,MCUSYS_CFGREG_BASE");
	if (!node) {
		CCCI_ERR_INF(md_id, "plt",
			     "md%d MCUSYS_CFGREG_BASE is not set in DT!\n",
			     (md_id + 1));
		/*  return -1; */
	} else {
		ap_mcu_reg_base = (unsigned int)of_iomap(node, 0);
		CCCI_MSG_INF(md_id, "plt",
			     "md%d get reg ap_mcu_reg_base:0x%x!\n",
			     (md_id + 1), ap_mcu_reg_base);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,AP_MD_DBGMODE_CFGREG");
	if (!node) {
		CCCI_ERR_INF(md_id, "plt", "md%d AP_MD_DBGMODE_CFGREG is not set in DT!\n", (md_id+1));
	} else {
		md1_dbg_mode_ptr = (unsigned long)of_iomap(node, 0);
		CCCI_MSG_INF(md_id, "plt", "md%d get reg md1_dbg_mode_ptr:0x%pa!\n", (md_id + 1), &md1_dbg_mode_ptr);
	}
	if (md_id == MD_SYS1)
		node_name = "mediatek,ap_ccif0";
	else if (md_id == MD_SYS2)
		node_name = "mediatek,ap_ccif1";
	else
		CCCI_ERR_INF(md_id, "plt", "md_id:%d is abnormal!\n", md_id);

	node = of_find_compatible_node(NULL, NULL, node_name);
	if (!node) {
		CCCI_ERR_INF(md_id, "plt",
			     "md%d smem size is not set in device tree,need to check\n",
			     (md_id + 1));
		return -1;
	}

	s_ccif_hw_info.reg_base = (unsigned int)of_iomap(node, 0);
	s_ccif_hw_info.md_reg_base = (unsigned int)of_iomap(node, 1);
	s_ccif_hw_info.irq_id = irq_of_parse_and_map(node, 0);
	md_wdt_irq_id[md_id] = irq_of_parse_and_map(node, 1);

	s_ccif_hw_info.type = CCIF_STD_V1;
	s_ccif_hw_info.irq_attr = 0;
	s_ccif_hw_info.md_id = MD_SYS1;	/*  Bind to MD Sys 1 */

	return 0;

#else
	return -2;
#endif
}

int get_ccif_hw_info(int md_id, struct ccif_hw_info_t *ccif_hw_info)
{
	if (ccif_hw_info == NULL)
		return -1;

	if ((ap_infra_base == 0) || (ap_mcu_reg_base == 0))
		get_dts_settings(md_id);
	switch (md_id) {
	case MD_SYS1:
#ifdef CONFIG_OF
		ccif_hw_info->reg_base = s_ccif_hw_info.reg_base;
		ccif_hw_info->md_reg_base = s_ccif_hw_info.md_reg_base;
		ccif_hw_info->irq_id = s_ccif_hw_info.irq_id;
#else
		ccif_hw_info->reg_base = AP_CCIF_BASE;
		ccif_hw_info->md_reg_base = MD_CCIF_BASE;
		ccif_hw_info->irq_id = CCIF0_AP_IRQ_ID;
#endif

		CCCI_MSG_INF(md_id, "plt",
			     "md%d HW settings: cif_ap_reg_base:0x%lx,md_cif_reg:0x%lx, irq:%d, wdt_irq:%d\n",
			     (md_id + 1), ccif_hw_info->reg_base,
			     ccif_hw_info->md_reg_base, ccif_hw_info->irq_id,
			     md_wdt_irq_id[md_id]);

		ccif_hw_info->type = CCIF_STD_V1;
		ccif_hw_info->irq_attr = 0;
		ccif_hw_info->md_id = MD_SYS1;	/*  Bind to MD Sys 1 */
		return 0;

	default:
		return -1;
	}
}

#ifdef ENABLE_LOCK_MD_SLP_FEATURE
static int md_slp_cnt;
static int md_slp_lock_ack;
static int md_slp_unlock_ack;
static DEFINE_SPINLOCK(md_slp_lock);

static int lock_sleep_cb(int data)
{
	if (data == LOCK_MD_SLP)
		md_slp_lock_ack = 1;
	else if (data == UNLOCK_MD_SLP)
		md_slp_unlock_ack = 1;

	return 0;
}

static int lock_md_sleep(int md_id, char *buf, unsigned int len)
{
	unsigned long flag;
	int ret = 0;
	unsigned int msg = MD_SLP_REQUEST;
	unsigned int reserved;

	spin_lock_irqsave(&md_slp_lock, flag);
	if (buf[0]) {
		if (++md_slp_cnt == 1)
			md_slp_lock_ack = 0;
	} else {
		if (md_slp_cnt == 0) {
			CCCI_MSG_INF(md_id, "ctl",
				     "unlock md slp mis-match lock(%s, 0)\n",
				     current->comm);
			spin_unlock_irqrestore(&md_slp_lock, flag);
			return ret;
		}

		if (--md_slp_cnt == 0)
			md_slp_unlock_ack = 0;
	}
	spin_unlock_irqrestore(&md_slp_lock, flag);

	if (md_slp_cnt == 1)
		reserved = LOCK_MD_SLP;
	else if (md_slp_cnt == 0)
		reserved = UNLOCK_MD_SLP;
	else
		return ret;

	CCCI_MSG_INF(md_id, "ctl", "%s request md sleep %d (%d, %d, %d): %d\n",
		     current->comm, buf[0], md_slp_cnt, md_slp_lock_ack,
		     md_slp_unlock_ack, ret);

	ret = notify_md_by_sys_msg(md_id, msg, reserved);

	return ret;
}

static int ack_md_sleep(int md_id, char *buf, unsigned int len)
{
	unsigned int flag = 0;

	if (buf[0])
		flag = md_slp_lock_ack;
	else
		flag = md_slp_unlock_ack;

	return flag;
}
#endif

/* For thermal driver to get modem TxPowr */
static int get_txpower(int md_id, char *buf, unsigned int len)
{
	int ret = 0;
	unsigned int msg;
	unsigned int resv = 0;

	if (buf[0] == 0) {
		msg = MD_TX_POWER;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	} else if (buf[0] == 1) {
		msg = MD_RF_TEMPERATURE;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	} else if (buf[0] == 2) {
		msg = MD_RF_TEMPERATURE_3G;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	}

	CCCI_MSG_INF(md_id, "ctl", "get_txpower(%d): %d\n", buf[0], ret);

	return ret;
}

void send_battery_info(int md_id)
{
	int ret = 0;
	unsigned int para = 0;
	unsigned int resv = 0;
	unsigned int msg_id = MD_GET_BATTERY_INFO;

	resv = get_bat_info(para);
	ret = notify_md_by_sys_msg(md_id, msg_id, resv);

	CCCI_DBG_MSG(md_id, "ctl", "send bat vol(%d) to md: %d\n", resv, ret);
}

int enable_get_sim_type(int md_id, unsigned int enable)
{
	int ret = 0;
	unsigned int msg_id = MD_SIM_TYPE;
	unsigned int resv = enable;

	ret = notify_md_by_sys_msg(md_id, msg_id, resv);
	CCCI_DBG_MSG(md_id, "ctl", "enable_get_sim_type(%d): %d\n", resv, ret);

	return ret;
}

#if defined(CONFIG_MTK_SWITCH_TX_POWER)
static int switch_Tx_Power(int md_id, unsigned int msg_id, unsigned int mode)
{
	int ret = 0;
	unsigned int resv = mode;

	ret = notify_md_by_sys_msg(md_id, msg_id, resv);

	CCCI_DBG_MSG(md_id, "ctl", "switch_%s_Tx_Power(%d): %d\n",
		     (msg_id == MD_SW_3G_TX_POWER) ? "3G" : "2G", resv, ret);

	return ret;
}

int switch_2G_Tx_Power(int md_id, unsigned int mode)
{
	return switch_Tx_Power(md_id, MD_SW_2G_TX_POWER, mode);
}

int switch_3G_Tx_Power(int md_id, unsigned int mode)
{
	return switch_Tx_Power(md_id, MD_SW_3G_TX_POWER, mode);
}
#endif

int sim_type = 0xEEEEEEEE;	/* sim_type(MCC/MNC) send by MD wouldn't be 0xEEEEEEEE */
int set_sim_type(int md_id, int data)
{
	int ret = 0;

	sim_type = data;
	CCCI_DBG_MSG(md_id, "ctl", "set_sim_type(%d): %d\n", sim_type, ret);

	return ret;
}

int get_sim_type(int md_id, int *p_sim_type)
{
	/* CCCI_DBG_MSG(md_id,  "ctl", "get_sim_type(%d): %d\n", ret); */

	*p_sim_type = sim_type;
	if (sim_type == 0xEEEEEEEE) {
		CCCI_MSG_INF(md_id, "ctl", "md has not send sim type yet(%d)",
			     sim_type);
		return -1;
	}
	return 0;
}

unsigned int res_len;
void ccci_rpc_work_helper(int md_id, int *p_pkt_num, struct RPC_PKT pkt[],
			  struct RPC_BUF *p_rpc_buf, unsigned int tmp_data[])
{
	/*  tmp_data[] is used to make sure memory address is valid after this function return */
	int pkt_num = *p_pkt_num;
	unsigned char Direction = 0;
	unsigned int ContentAddr = 0;
	unsigned int ContentLen = 0;
	struct sed_t CustomSeed = SED_INITIALIZER;
	unsigned char *ResText = NULL;
#ifdef ENCRYPT_DEBUG
	unsigned char *RawText = NULL;
	unsigned int i = 0;
	unsigned char log_buf[128];
#endif
	CCCI_RPC_MSG(md_id, "ccci_rpc_work_helper++\n");

	tmp_data[0] = 0;

	switch (p_rpc_buf->op_id) {
	case IPC_RPC_CPSVC_SECURE_ALGO_OP:
		{
			if (pkt_num < 4) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid pkt_num %d for RPC_SECURE_ALGO_OP!\n",
					     pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				break;
			}
			Direction = *(unsigned char *)pkt[0].buf;
			ContentAddr = (unsigned int)pkt[1].buf;
			CCCI_RPC_MSG(md_id,
				     "RPC_SECURE_ALGO_OP: Content_Addr = 0x%08X, RPC_Base = 0x%08X, RPC_Len = 0x%08X\n",
				     ContentAddr, (unsigned int)p_rpc_buf,
				     sizeof(struct RPC_BUF) + RPC1_MAX_BUF_SIZE);
			if (ContentAddr < (unsigned int)p_rpc_buf
			    || ContentAddr >
			    ((unsigned int)p_rpc_buf + sizeof(struct RPC_BUF) +
			     RPC1_MAX_BUF_SIZE)) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid ContentAdddr[0x%08X] for RPC_SECURE_ALGO_OP!\n",
					     ContentAddr);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				break;
			}
			ContentLen = *(unsigned int *)pkt[2].buf;
			/*     CustomSeed = *(sed_t*)pkt[3].buf; */
			WARN_ON(sizeof(CustomSeed.sed) < pkt[3].len);
			memcpy(CustomSeed.sed, pkt[3].buf, pkt[3].len);

#ifdef ENCRYPT_DEBUG
			if (Direction == TRUE)
				CCCI_MSG_INF(md_id, "rpc",
					     "HACC_S: EnCrypt_src:\n");
			else
				CCCI_MSG_INF(md_id, "rpc",
					     "HACC_S: DeCrypt_src:\n");
			for (i = 0; i < ContentLen; i++) {
				if (i % 16 == 0) {
					if (i != 0)
						CCCI_RPC_MSG(md_id, "%s\n", log_buf);
					curr = 0;
					curr +=
					    snprintf(log_buf,
						     sizeof(log_buf) - curr,
						     "HACC_S: ");
				}
				/* CCCI_MSG("0x%02X ", *(unsigned char*)(ContentAddr+i)); */
				curr +=
				    snprintf(&log_buf[curr],
					     sizeof(log_buf) - curr, "0x%02X ",
					     *(unsigned char *)(ContentAddr +
								i));
				/* sleep(1); */
			}
			CCCI_RPC_MSG(md_id, "%s\n", log_buf);

			RawText = kmalloc(ContentLen, GFP_KERNEL);
			if (RawText == NULL)
				CCCI_MSG_INF(md_id, "rpc",
					     "Fail alloc Mem for RPC_SECURE_ALGO_OP!\n");
			else
				memcpy(RawText, (unsigned char *)ContentAddr,
				       ContentLen);
#endif

			ResText = kmalloc(ContentLen, GFP_KERNEL);
			if (ResText == NULL) {
				CCCI_MSG_INF(md_id, "rpc",
					     "Fail alloc Mem for RPC_SECURE_ALGO_OP!\n");
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				break;
			}
#if (defined(ENABLE_MD_IMG_SECURITY_FEATURE) && defined(CONFIG_MTK_SEC_MODEM_NVRAM_ANTI_CLONE))
			if (!masp_secure_algo_init()) {
				CCCI_MSG_INF(md_id, "rpc",
					     "masp_secure_algo_init fail!\n");
				BUG_ON(1);
			}

			CCCI_RPC_MSG(md_id,
				     "RPC_SECURE_ALGO_OP: Dir=0x%08X, Addr=0x%08X, Len=0x%08X, Seed=0x%016llX\n",
				     Direction, ContentAddr, ContentLen,
				     *(long long *)CustomSeed.sed);
			masp_secure_algo(Direction, (unsigned char *)ContentAddr, ContentLen,
					 CustomSeed.sed, ResText);

			if (!masp_secure_algo_deinit())
				CCCI_MSG_INF(md_id, "rpc",
					     "masp_secure_algo_deinit fail!\n");
#endif

			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = ContentLen;

#if (defined(ENABLE_MD_IMG_SECURITY_FEATURE) && defined(CONFIG_MTK_SEC_MODEM_NVRAM_ANTI_CLONE))
			memcpy(pkt[pkt_num++].buf, ResText, ContentLen);
			CCCI_MSG_INF(md_id, "rpc",
				     "RPC_Secure memory copy OK: %d!",
				     ContentLen);
#else
			memcpy(pkt[pkt_num++].buf, (void *)ContentAddr,
			       ContentLen);
			CCCI_MSG_INF(md_id, "rpc",
				     "RPC_NORMAL memory copy OK: %d!",
				     ContentLen);
#endif

#ifdef ENCRYPT_DEBUG
			if (Direction == TRUE)
				CCCI_RPC_MSG(md_id, "HACC_D: EnCrypt_dst:\n");
			else
				CCCI_RPC_MSG(md_id, "HACC_D: DeCrypt_dst:\n");
			for (i = 0; i < ContentLen; i++) {
				if (i % 16 == 0) {
					if (i != 0)
						CCCI_RPC_MSG(md_id, "%s\n", log_buf);
					curr = 0;
					curr +=
					    snprintf(&log_buf[curr],
						     sizeof(log_buf) - curr,
						     "HACC_D: ");
				}
				/* CCCI_MSG("%02X ", *(ResText+i)); */
				curr +=
				    snprintf(&log_buf[curr],
					     sizeof(log_buf) - curr, "0x%02X ",
					     *(ResText + i));
				/* sleep(1); */
			}

			CCCI_RPC_MSG(md_id, "%s\n", log_buf);

			kfree(RawText);
#endif

			kfree(ResText);
			break;
		}

#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
	case IPC_RPC_GET_SECRO_OP:
		{
			unsigned char *addr = NULL;
			unsigned int img_len = 0;
			unsigned int img_len_bak = 0;
			unsigned int blk_sz = 0;
			unsigned int tmp = 1;
			unsigned int cnt = 0;
			unsigned int req_len = 0;

			if (pkt_num != 1) {
				CCCI_MSG_INF(md_id, "rpc",
					     "RPC_GET_SECRO_OP: invalid parameter: pkt_num=%d\n",
					     pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				tmp_data[1] = img_len;
				pkt[pkt_num++].buf = (void *)&tmp_data[1];
				break;
			}

			req_len = *(unsigned int *)(pkt[0].buf);
			if (masp_secro_en()) {
				img_len =
				    masp_secro_md_len(md_image_post_fix[md_id]);

				if ((img_len > RPC1_MAX_BUF_SIZE)
				    || (req_len > RPC1_MAX_BUF_SIZE)) {
					pkt_num = 0;
					tmp_data[0] = FS_MEM_OVERFLOW;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[0];
					/* set it as image length for modem ccci check when error happens */
					pkt[pkt_num].len = img_len;
					/* /pkt[pkt_num].len = sizeof(unsigned int); */
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[1];
					CCCI_MSG_INF(md_id, "rpc",
						     "RPC_GET_SECRO_OP: md request length is larger than rpc memory: (%d, %d)\n",
						     req_len, img_len);
					break;
				}

				if (img_len > req_len) {
					pkt_num = 0;
					tmp_data[0] = FS_NO_MATCH;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[0];
					/* set it as image length for modem ccci check when error happens */
					pkt[pkt_num].len = img_len;
					/* /pkt[pkt_num].len = sizeof(unsigned int); */
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[1];
					CCCI_MSG_INF(md_id, "rpc",
						     "RPC_GET_SECRO_OP: AP mis-match MD request length: (%d, %d)\n",
						     req_len, img_len);
					break;
				}

				/* TODO : please check it */
				/* save original modem secro length */
				CCCI_MSG
				    ("<rpc>RPC_GET_SECRO_OP: save MD SECRO length: (%d)\n",
				     img_len);
				img_len_bak = img_len;

				blk_sz = masp_secro_blk_sz();
				for (cnt = 0; cnt < blk_sz; cnt++) {
					tmp = tmp * 2;
					if (tmp >= blk_sz)
						break;
				}
				++cnt;
				img_len = ((img_len + (blk_sz - 1)) >> cnt) << cnt;

				addr = p_rpc_buf->buf + 4 * sizeof(unsigned int);
				tmp_data[0] = masp_secro_md_get_data(md_image_post_fix
							   [md_id], addr, 0,
							   img_len);

				/* TODO : please check it */
				/* restore original modem secro length */
				img_len = img_len_bak;

				CCCI_MSG
				    ("<rpc>RPC_GET_SECRO_OP: restore MD SECRO length: (%d)\n",
				     img_len);

				if (tmp_data[0] != 0) {
					CCCI_MSG_INF(md_id, "rpc",
						     "RPC_GET_SECRO_OP: get data fail:%d\n",
						     tmp_data[0]);
					pkt_num = 0;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[0];
					pkt[pkt_num].len = sizeof(unsigned int);
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[1];
				} else {
					CCCI_MSG_INF(md_id, "rpc",
						     "RPC_GET_SECRO_OP: get data OK: %d,%d\n",
						     img_len, tmp_data[0]);
					pkt_num = 0;
					pkt[pkt_num].len = sizeof(unsigned int);
					/* pkt[pkt_num++].buf = (void*) &img_len; */
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf =
					    (void *)&tmp_data[1];
					pkt[pkt_num].len = img_len;
					pkt[pkt_num++].buf = (void *)addr;
					/* tmp_data[2] = (unsigned int)addr; */
					/* pkt[pkt_num++].buf = (void*) &tmp_data[2]; */
				}
			} else {
				CCCI_MSG_INF(md_id, "rpc",
					     "RPC_GET_SECRO_OP: secro disable\n");
				tmp_data[0] = FS_NO_FEATURE;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				tmp_data[1] = img_len;
				pkt[pkt_num++].buf = (void *)&tmp_data[1];
			}

			break;
		}
#endif

		/* call eint API to get TDD EINT configuration for modem EINT initial */
	case IPC_RPC_GET_TDD_EINT_NUM_OP:
	case IPC_RPC_GET_TDD_GPIO_NUM_OP:
	case IPC_RPC_GET_TDD_ADC_NUM_OP:
		{
			int get_num = 0;
			unsigned char *name = NULL;
			unsigned int length = 0;

			if (pkt_num < 2) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d!\n",
					     p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err1;
			}
			length = pkt[0].len;
			if (length < 1) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d, name_len=%d!\n",
					     p_rpc_buf->op_id, pkt_num, length);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err1;
			}

			name = kmalloc(length, GFP_KERNEL);
			if (name == NULL) {
				CCCI_MSG_INF(md_id, "rpc",
					     "Fail alloc Mem for [0x%X]!\n",
					     p_rpc_buf->op_id);
				tmp_data[0] = FS_ERROR_RESERVED;
				goto err1;
			} else {
				memcpy(name, (unsigned char *)(pkt[0].buf), length);
				if (p_rpc_buf->op_id == IPC_RPC_GET_TDD_EINT_NUM_OP) {
					get_num = get_td_eint_info(md_id, name, length);
					if (get_num < 0)
						get_num = FS_FUNC_FAIL;
				} else if (p_rpc_buf->op_id == IPC_RPC_GET_TDD_GPIO_NUM_OP) {
					get_num = get_md_gpio_info(md_id, name, length);
					if (get_num < 0)
						get_num = FS_FUNC_FAIL;
				} else if (p_rpc_buf->op_id == IPC_RPC_GET_TDD_ADC_NUM_OP) {
					get_num = get_md_adc_info(md_id, name, length);
					if (get_num < 0)
						get_num = FS_FUNC_FAIL;
				}

				CCCI_MSG_INF(md_id, "rpc",
					     "[0x%08X]: name:%s, len=%d, get_num:%d\n",
					     p_rpc_buf->op_id, name, length,
					     get_num);
				pkt_num = 0;

				/* NOTE: tmp_data[1] not [0] */
				tmp_data[1] = (unsigned int)get_num;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)(&tmp_data[1]);	/* get_num); */
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)(&tmp_data[1]);	/* get_num); */
				kfree(name);
			}
			break;

 err1:
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			break;
		}

	case IPC_RPC_GET_EMI_CLK_TYPE_OP:
		{
			int dram_type = 0;
			int dram_clk = 0;

			if (pkt_num != 0) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d!\n",
					     p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err2;
			}

			if (get_dram_type_clk(&dram_clk, &dram_type)) {
				tmp_data[0] = FS_FUNC_FAIL;
				goto err2;
			} else {
				tmp_data[0] = 0;
				CCCI_MSG_INF(md_id, "rpc",
					     "[0x%08X]: dram_clk: %d, dram_type:%d\n",
					     p_rpc_buf->op_id, dram_clk,
					     dram_type);
			}

			tmp_data[1] = (unsigned int)dram_type;
			tmp_data[2] = (unsigned int)dram_clk;

			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)(&tmp_data[0]);
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)(&tmp_data[1]);
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)(&tmp_data[2]);
			break;

 err2:
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			break;
		}

	case IPC_RPC_GET_EINT_ATTR_OP:
		{
			char *eint_name = NULL;
			unsigned int name_len = 0;
			unsigned int type = 0;
			char *res = NULL;
			/* unsigned int res_len = 0; */
			int ret = 0;

			if (pkt_num < 3) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d!\n",
					     p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err3;
			}
			name_len = pkt[0].len;
			if (name_len < 1) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d, name_len=%d!\n",
					     p_rpc_buf->op_id, pkt_num,
					     name_len);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err3;
			}

			eint_name = kmalloc(name_len, GFP_KERNEL);
			if (eint_name == NULL) {
				CCCI_MSG_INF(md_id, "rpc",
					     "Fail alloc Mem for [0x%X]!\n",
					     p_rpc_buf->op_id);
				tmp_data[0] = FS_ERROR_RESERVED;
				goto err3;
			} else {
				memcpy(eint_name, (unsigned char *)(pkt[0].buf),
				       name_len);
			}

			type = *(unsigned int *)(pkt[2].buf);
			res = p_rpc_buf->buf + 4 * sizeof(unsigned int);
			ret =
			    get_eint_attr(eint_name, name_len, type, res,
					  &res_len);
			if (ret == 0) {
				tmp_data[0] = ret;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void *)&tmp_data[0];
				pkt[pkt_num].len = res_len;
				pkt[pkt_num++].buf = (void *)res;
				CCCI_MSG_INF(md_id, "rpc",
					     "[0x%08X] OK: name:%s, len:%d, type:%d, res:%d, res_len:%d\n",
					     p_rpc_buf->op_id, eint_name,
					     name_len, type, *res, res_len);
				kfree(eint_name);
			} else {
				tmp_data[0] = ret;
				CCCI_MSG_INF(md_id, "rpc",
					     "[0x%08X] fail: name:%s, len:%d, type:%d, ret:%d\n",
					     p_rpc_buf->op_id, eint_name,
					     name_len, type, ret);
				kfree(eint_name);
				goto err3;
			}
			break;

 err3:
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			break;
		}

	case IPC_RPC_GET_GPIO_VAL_OP:
	case IPC_RPC_GET_ADC_VAL_OP:
		{
			unsigned int num = 0;
			int val = 0;

			if (pkt_num != 1) {
				CCCI_MSG_INF(md_id, "rpc",
					     "invalid parameter for [0x%X]: pkt_num=%d!\n",
					     p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err4;
			}

			num = *(unsigned int *)(pkt[0].buf);
			if (p_rpc_buf->op_id == IPC_RPC_GET_GPIO_VAL_OP)
				val = get_md_gpio_val(md_id, num);
			else if (p_rpc_buf->op_id == IPC_RPC_GET_ADC_VAL_OP)
				val = get_md_adc_val(md_id, num);
			tmp_data[0] = val;
			CCCI_MSG_INF(md_id, "rpc", "[0x%X]: num=%d, val=%d!\n",
				     p_rpc_buf->op_id, num, val);

 err4:
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void *)&tmp_data[0];
			break;
		}

	default:
		CCCI_MSG_INF(md_id, "rpc",
			     "[error]Unknown Operation ID (0x%08X)\n",
			     p_rpc_buf->op_id);
		tmp_data[0] = FS_NO_OP;
		pkt_num = 0;
		pkt[pkt_num].len = sizeof(int);
		pkt[pkt_num++].buf = (void *)&tmp_data[0];
		break;
	}
	*p_pkt_num = pkt_num;

	CCCI_RPC_MSG(md_id, "ccci_rpc_work_helper--\n");
}

static int clear_md_region_protection(int md_id)
{
#ifdef ENABLE_EMI_PROTECTION
	unsigned int rom_mem_mpu_id, rw_mem_mpu_id;

	CCCI_MSG_INF(md_id, "ctl", "Clear MD%d region protect...\n", md_id + 1);
	switch (md_id) {
	case MD_SYS1:
		rom_mem_mpu_id = 1;	/* 0; */
		rw_mem_mpu_id = 2;	/* 1; */
		break;

	default:
		CCCI_MSG_INF(md_id, "ctl",
			     "[error]md id(%d) invalid when clear MPU protect\n",
			     md_id + 1);
		return -1;
	}

	CCCI_MSG_INF(md_id, "ctl", "Clear MPU protect MD%d ROM region<%d>\n",
		     md_id + 1, rom_mem_mpu_id);
	emi_mpu_set_region_protection(0,	/*START_ADDR */
		0,	/*END_ADDR */
		rom_mem_mpu_id,	/*region */
		SET_ACCESS_PERMISSON(NO_PROTECTION,
					NO_PROTECTION,
					NO_PROTECTION,
					NO_PROTECTION));

	CCCI_MSG_INF(md_id, "ctl", "Clear MPU protect MD%d R/W region<%d>\n",
		     md_id + 1, rom_mem_mpu_id);
	emi_mpu_set_region_protection(0,	/*START_ADDR */
		0,	/*END_ADDR */
		rw_mem_mpu_id,	/*region */
		SET_ACCESS_PERMISSON(NO_PROTECTION,
					NO_PROTECTION,
					NO_PROTECTION,
					NO_PROTECTION));
#endif

	return 0;
}

static void enable_mem_access_protection(int md_id)
{
#ifdef ENABLE_EMI_PROTECTION
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id,
	    shr_mem_mpu_attr;
	unsigned int rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_id,
	    rom_mem_mpu_attr;
	unsigned int rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_id,
	    rw_mem_mpu_attr;
	 struct ccci_image_info *img_info;
	struct ccci_mem_layout_t *md_layout;

#ifdef EMI_MPU_SET_AP_REGION
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned int kernel_base;
	unsigned int dram_size;
	unsigned int ap_mem_phy_end;
#endif	/* #ifdef EMI_MPU_SET_AP_REGION */
	/*  For MT6582 */
	/* =================================================================== */
	/*             | Region |  D0(AP)  |  D1(MD0)  |D2(CONNSYS)|  D3(MM) */
	/* ------------+------------------------------------------------------ */
	/*  Secure OS  |    0   |RW(S)     |Forbidden  |Forbidden  |Forbidden */
	/* ------------+------------------------------------------------------ */
	/*  MD0 ROM    |    1   |RO(S/NS)  |RO(S/NS)   |Forbidden  |Forbidden */
	/* ------------+------------------------------------------------------ */
	/*  MD0 R/W+   |    2   |Forbidden |No protect |Forbidden  |Forbidden */
	/* ------------+------------------------------------------------------ */
	/*  MD0 Share  |    3   |No protect|No protect |Forbidden  |Forbidden */
	/* ------------+------------------------------------------------------ */
	/* CONNSYS Code|    4   |Forbidden |Forbidden  |RO(S/NS)   |Forbidden */
	/* ------------+------------------------------------------------------ */
	/* AP-Con Share|    5   |No protect|Forbidden  |No product |Forbidden */
	/* ------------+------------------------------------------------------ */
	/*  AP         |   6~7  |No protect|RO(S/NS)   |Forbidden  |No protect */
	/* =================================================================== */

	switch (md_id) {
	case MD_SYS1:
		img_info = &md_img_info[md_id];
		md_layout = &md_mem_layout_tab[md_id];
		rom_mem_mpu_id = 1;
		rw_mem_mpu_id = 2;
		shr_mem_mpu_id = 3;
		rom_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R,
					 SEC_R_NSEC_R);
		rw_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION,
					 FORBIDDEN);
		shr_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION,
					 NO_PROTECTION);
		break;

	default:
		CCCI_MSG_INF(md_id, "ctl",
			     "[error]md id(%d) invalid when MPU protect\n",
			     md_id + 1);
		return;
	}

#ifdef EMI_MPU_SET_AP_REGION
	kernel_base = get_phys_offset();
#ifdef ENABLE_DRAM_API
	dram_size = get_max_DRAM_size();
#else
	dram_size = 256 * 1024 * 1024;
#endif
	ap_mem_mpu_id = 7;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, SEC_R_NSEC_R,
				 NO_PROTECTION);
#endif	/* #ifdef EMI_MPU_SET_AP_REGION */

	shr_mem_phy_start = md_layout->smem_region_phy_before_map;
	shr_mem_phy_end = md_layout->smem_region_phy_before_map + 0x200000;
	/* end addr: EMI MPU ignore the lowest 16bit */
	shr_mem_phy_end = ((shr_mem_phy_end + 0xFFFF) & (~0xFFFF)) - 1;
	rom_mem_phy_start = md_layout->md_region_phy;
	rom_mem_phy_end =
	    ((md_layout->md_region_phy + img_info[md_id].size +
	      0xFFFF) & (~0xFFFF)) - 1;
	/*  64KB align */
	rw_mem_phy_start = (md_layout->md_region_phy + img_info[md_id].size + 0xFFFF) & (~0xFFFF);
	rw_mem_phy_end = md_layout->md_region_phy + md_layout->md_region_size;
	/* end addr: EMI MPU ignore the lowest 16bit */
	rw_mem_phy_end = ((rw_mem_phy_end + 0xFFFF) & (~0xFFFF)) - 1;

	CCCI_MSG_INF(md_id, "ctl",
		     "MPU Start protect MD%d ROM region<%d:%08x:%08x>\n",
		     md_id + 1, rom_mem_mpu_id, rom_mem_phy_start,
		     rom_mem_phy_end);
	emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
				      rom_mem_phy_end,	/*END_ADDR */
				      rom_mem_mpu_id,	/*region */
				      rom_mem_mpu_attr);

	CCCI_MSG_INF(md_id, "ctl",
		     "MPU Start protect MD%d R/W region<%d:%08x:%08x>\n",
		     md_id + 1, rw_mem_mpu_id, rw_mem_phy_start,
		     rw_mem_phy_end);
	emi_mpu_set_region_protection(rw_mem_phy_start,	/*START_ADDR */
				      rw_mem_phy_end,	/*END_ADDR */
				      rw_mem_mpu_id,	/*region */
				      rw_mem_mpu_attr);

	CCCI_MSG_INF(md_id, "ctl",
		     "MPU Start protect MD%d Share region<%d:%08x:%08x>\n",
		     md_id + 1, shr_mem_mpu_id, shr_mem_phy_start,
		     shr_mem_phy_end);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);

#ifdef EMI_MPU_SET_AP_REGION
	/* end addr: EMI MPU ignore the lowest 16bit */
	ap_mem_phy_end = (((kernel_base + dram_size) + 0xFFFF) & (~0xFFFF)) - 1;
	CCCI_MSG_INF(md_id, "ctl",
		     "MPU Start protect AP region<%d:%08x:%08x>\n",
		     ap_mem_mpu_id, kernel_base, ap_mem_phy_end);
	emi_mpu_set_region_protection(kernel_base, ap_mem_phy_end,
				ap_mem_mpu_id, ap_mem_mpu_attr);
#endif	/* #ifdef EMI_MPU_SET_AP_REGION */
#endif
}

static void set_ap_region_protection(int md_id)
{
#ifdef ENABLE_EMI_PROTECTION
#ifdef EMI_MPU_SET_AP_REGION
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned int kernel_base;
	unsigned int dram_size;
	unsigned int ap_mem_phy_end;

	kernel_base = get_phys_offset();
#ifdef ENABLE_DRAM_API
	dram_size = get_max_DRAM_size();
#else
	dram_size = 256 * 1024 * 1024;
#endif
	ap_mem_mpu_id = 7;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
	ap_mem_phy_end = ((((kernel_base + dram_size) + 0xFFFF) & (~0xFFFF)) - 1);

	CCCI_MSG_INF(md_id, "ctl",
		     "MPU Start protect AP region<%d:%08x:%08x>\n",
		     ap_mem_mpu_id, kernel_base, ap_mem_phy_end);
	emi_mpu_set_region_protection(kernel_base, ap_mem_phy_end,
				      ap_mem_mpu_id, ap_mem_mpu_attr);
#endif	/* #ifdef EMI_MPU_SET_AP_REGION */
#endif
}


static int load_img_cfg(int md_id)
{
	struct ccci_mem_layout_t *layout;

	if (md_id != MD_SYS1)
		return 0;

	layout = &md_mem_layout_tab[md_id];
	md_img_info[md_id].address = layout->md_region_phy;
	md_img_info[md_id].ap_info.platform = ap_platform;
	md_img_info[md_id].ap_platform = ap_platform;
	md_img_info[md_id].type = IMG_MD;
	return 0;
}

int ccci_load_firmware_helper(int md_id, char img_err_str[], int len)
{
	int ret = 0;
	char *str;

	if (md_id != MD_SYS1)
		return ret;

	if (ccif_plat_drv == NULL) {
		CCCI_ERR_INF(md_id, "ctl", "ccif_plat_drv == NULL\n");
		snprintf(img_err_str, len, "ccif_plat_drv == NULL");
		ret = -CCCI_ERR_LOAD_IMG_FILE_OPEN;
		return ret;
	}

	/* step1: clear modem protection when start to load firmware */
	clear_md_region_protection(md_id);

	/* step2: load image */
	str = md_image_post_fix[md_id];
	ret = ccci_load_firmware(md_id, &md_img_info[md_id], img_err_str, str,
			&ccif_plat_drv->dev);
	if (ret < 0) {
		CCCI_ERR_INF(md_id, "ctl",
			     "load_firmware failed:ret=%d!\n", ret);
		ret = -CCCI_ERR_LOAD_IMG_LOAD_FIRM;
		return ret;
	}

	/*  Image info ready */
	str = md_img_info[md_id].img_info.product_ver;
	if (NULL == str)
		img_is_dbg_ver[md_id] = MD_DEBUG_REL_INFO_NOT_READY;
	else if (strcmp(str, "Debug") == 0)
		img_is_dbg_ver[md_id] = MD_IS_DEBUG_VERSION;
	else
		img_is_dbg_ver[md_id] = MD_IS_RELEASE_VERSION;

	return ret;
}

void md_env_setup_before_boot(int md_id)
{
	enable_mem_access_protection(md_id);
}

void md_env_setup_before_ready(int md_id)
{
}

void md_boot_up_additional_operation(int md_id)
{
	/* power on Audsys for DSP boot up */
	/* AudSys_Power_On(TRUE); */
	set_ap_region_protection(md_id);
}

void md_boot_ready_additional_operation(int md_id)
{
	/* power off Audsys after DSP boot up ready */
	/* AudSys_Power_On(FALSE); */
}

void additional_operation_before_stop_md(int md_id)
{
	/* char buf = 1; */
	/* exec_ccci_kern_func(ID_LOCK_MD_SLEEP, &buf, sizeof(char)); */
	/* md_slp_cnt = 0; */
}

static void md_wdt_notify(int md_id)
{
	if (md_id >= 0 && md_id < MAX_MD_NUM) {
		if (wdt_notify_array[md_id] != NULL)
			wdt_notify_array[md_id] (md_id);
	}
}

void ccci_md_wdt_notify_register(int md_id, int_func_int_t funcp)
{
	if (NULL == funcp) {
		CCCI_MSG_INF(md_id, "ctl",
			     "[Error]md wdt notify function pointer is NULL\n");
	} else if (wdt_notify_array[md_id]) {
		CCCI_MSG_INF(md_id, "ctl",
			     "wdt notify function pointer has registered\n");
	} else {
		wdt_notify_array[md_id] = funcp;
		/* CCCI_DBG_MSG(md_id, "ctl", "wdt notify func register OK\n"); */
	}
}

void md_dsp_wdt_irq_en(int md_id)
{
	int irq_id;

	switch (md_id) {
	case MD_SYS1:
		irq_id = md_wdt_irq_id[md_id];
		break;

	default:
		return;
	}

	if (atomic_read(&wdt_irq_en_count[md_id]) == 0) {
		enable_irq(irq_id);
		atomic_inc(&wdt_irq_en_count[md_id]);
	}
}

void md_dsp_wdt_irq_dis(int md_id)
{
	int irq_id;

	switch (md_id) {
	case MD_SYS1:
		irq_id = md_wdt_irq_id[md_id];
		break;

	default:
		return;
	}

	if (atomic_read(&wdt_irq_en_count[md_id]) == 1) {
		/*may be called in isr, so use disable_irq_nosync.
		   if use disable_irq in isr, system will hang */
		disable_irq_nosync(irq_id);
		atomic_dec(&wdt_irq_en_count[md_id]);
	}
}

void start_md_wdt_recov_timer(int md_id)
{
	switch (md_id) {
	case MD_SYS1:
		mod_timer(&md1_wdt_recover_timer, jiffies + HZ / 100);
		break;

	default:
		break;
	}
}

void stop_md_wdt_recov_timer(int md_id)
{
	switch (md_id) {
	case MD_SYS1:
		del_timer(&md1_wdt_recover_timer);
		break;

	default:
		break;
	}
}

static irqreturn_t md_wdt_isr(int irq, void *data __always_unused)
{
#ifdef ENABLE_MD_WDT_DBG
	unsigned int sta = 0;
#endif
	int md_id = -1;

#ifdef ENABLE_MD_WDT_PROCESS
	if (irq == md_wdt_irq_id[MD_SYS1]) {
		stop_md_wdt_recov_timer(MD_SYS1);
		md_wdt_has_processed[MD_SYS1] = 1;
		if (md1_rgu_base) {
#ifdef ENABLE_MD_WDT_DBG
			sta = ccci_read32(WDT_MD_STA(md1_rgu_base));
			ccci_write32(WDT_MD_MODE(md1_rgu_base),
				     WDT_MD_MODE_KEY);
#endif
			md_id = MD_SYS1;
			CCCI_MSG_INF(md_id, "ctl", "Now disable md wdt irq\n");
			md_dsp_wdt_irq_dis(md_id);
		}
	}
#endif

#ifdef ENABLE_MD_WDT_DBG
	CCCI_MSG_INF(md_id, "ctl", "MD_WDT_STA=%04x(md%d_irq=%d)\n", sta,
		     (md_id + 1), irq);
#else
	CCCI_MSG_INF(md_id, "ctl", "MD_WDT_ISR(md%d_irq=%d)\n", (md_id + 1),
		     irq);
#endif

#ifdef ENABLE_MD_WDT_DBG
	if (sta != 0)
#endif
		md_wdt_notify(md_id);

	return IRQ_HANDLED;
}

static int md_dsp_irq_init(int md_id)
{
	int ret = 0;

#ifdef ENABLE_MD_WDT_PROCESS
	int irq_num = 0;

	switch (md_id) {
	case MD_SYS1:
#ifdef CONFIG_OF
		irq_num = md_wdt_irq_id[md_id];
#else
		irq_num = MD_WDT_IRQ_ID;
#endif
		break;

	default:
		return -1;
	}

	ret = request_irq(irq_num, md_wdt_isr, IRQF_TRIGGER_FALLING, "MD-WDT", NULL);
	if (ret) {
		CCCI_MSG_INF(md_id, "ctl", "register MDWDT_IRQ fail: %d\n", ret);
		return ret;
	}

	atomic_set(&wdt_irq_en_count[md_id], 1);
#endif

	return ret;
}

static void md_dsp_irq_deinit(int md_id)
{
#ifdef ENABLE_MD_WDT_PROCESS
	int irq_num = 0;

	switch (md_id) {
	case MD_SYS1:
#ifdef CONFIG_OF
		irq_num = md_wdt_irq_id[md_id];
#else
		irq_num = MD_WDT_IRQ_ID;
#endif

		break;

	default:
		return;
	}

	free_irq(irq_num, NULL);
	/* disable_irq(irq_num); */

	atomic_set(&wdt_irq_en_count[md_id], 0);
#endif
}

static void recover_md_wdt_irq(unsigned long data)
{
	unsigned int sta = 0;
	int md_id = -1;
	unsigned long flags;

#ifdef ENABLE_MD_WDT_PROCESS
	switch (data) {
	case MD_SYS1:
		if (md1_rgu_base && (!md_wdt_has_processed[MD_SYS1])) {
			spin_lock_irqsave(&md1_wdt_mon_lock, flags);
			sta = ccci_read32(WDT_MD_STA(md1_rgu_base));
			ccci_write32(WDT_MD_MODE(md1_rgu_base), WDT_MD_MODE_KEY);
			spin_unlock_irqrestore(&md1_wdt_mon_lock, flags);
			md_id = MD_SYS1;
		}
		break;

	default:
		break;
	}
#endif

	CCCI_MSG_INF(md_id, "ctl", "MD_WDT_STA=%04x(%d)(R)\n", sta, (md_id + 1));

	if (sta != 0)
		md_wdt_notify(md_id);
}

static void map_md_side_register(int md_id)
{
	switch (md_id) {
	case MD_SYS1:
		md1_ccif_base = s_ccif_hw_info.md_reg_base;	/*  MD CCIF Bas; */
		md1_boot_slave_Vector = (unsigned int)ioremap_nocache(0x20190000, 0x4);
		md1_boot_slave_Key = (unsigned int)ioremap_nocache(0x2019379C, 0x4);
		md1_boot_slave_En = (unsigned int)ioremap_nocache(0x20195488, 0x4);
		md1_rgu_base = (unsigned int)ioremap_nocache(0x20050000, 0x40);
#ifdef ENABLE_DUMP_MD_REGISTER
		if (md1_mcu_ptr == NULL)
			md1_mcu_ptr = ioremap_nocache(MD1_MCU_ADDR_BASE, 0x500);
		if (md1_m3d_ptr == NULL)
			md1_m3d_ptr = ioremap_nocache(MD1_M3D_ADDR_BASE, 0x500);

		if (!md1_ost_ptr)
			md1_ost_ptr = ioremap_nocache(MD1_OS_TIMER_ADDR, 0x1000);
		if (!md1_mixedsys_ptr)
			md1_mixedsys_ptr = ioremap_nocache(MD1_MIXEDSYS_ADDR, 0x1000);
		if (!md1_md_topsm_ptr)
			md1_md_topsm_ptr = ioremap_nocache(MD1_MD_TOPSM_ADDR, 0x1000);
		if (!md1_modem_topsm_ptr)
			md1_modem_topsm_ptr = ioremap_nocache(MD1_MODEM_TOPSM_ADDR, 0x1000);
#endif
		break;

	default:
		break;
	}
}

/* ======================================================== */
/*  Enable/Disable MD clock */
/* ======================================================== */
static int ccci_en_md1_clock(void)
{
	unsigned long flags;
	int ret = 0;
	int need_power_on_md = 0;

	spin_lock_irqsave(&md1_power_change_lock, flags);
	if (md1_power_on_cnt == 0) {
		md1_power_on_cnt++;
		need_power_on_md = 1;
	}
	spin_unlock_irqrestore(&md1_power_change_lock, flags);

	if (need_power_on_md)
		ret = md_power_on(0);

	return ret;
}

static int ccci_dis_md1_clock(unsigned int timeout)
{
	unsigned long flags;
	int ret = 0;
	int need_power_off_md = 0;

	spin_lock_irqsave(&md1_power_change_lock, flags);
	if (md1_power_on_cnt == 1) {
		md1_power_on_cnt--;
		need_power_off_md = 1;
	}
	spin_unlock_irqrestore(&md1_power_change_lock, flags);

	if (need_power_off_md) {
		timeout = timeout / 10;	/*  Mili-seconds to Jiffies */
		CCCI_DBG_MSG(MD_SYS1, "ctl", "off+\n");
		ret = md_power_off(0, timeout);
		CCCI_DBG_MSG(MD_SYS1, "ctl", "off-%d\n", ret);
	}

	return ret;
}

/* ======================================================== */
/*  power on/off modem */
/* ======================================================== */
int ccci_power_on_md(int md_id)
{
	/* int count; */
	CCCI_MSG_INF(md_id, "ctl", "[ccci/cci] power on md%d to run\n",
		     md_id + 1);

	switch (md_id) {
	case MD_SYS1:		/*  MD1 */
		/* md_power_on(MD_SYS1); */
		/* ccci_write32(WDT_MD_MODE(md1_rgu_base), WDT_MD_MODE_KEY); */
		break;

	default:
		break;
	}

	return 0;
}

int ccci_power_down_md(int md_id)
{
	CCCI_MSG_INF(md_id, "ctl", "power down md%d\n", md_id + 1);

	return 0;
}

unsigned int get_debug_mode_flag(void)
{
	unsigned int dbg_spare;

	if (md1_dbg_mode_ptr) {
		if ((debug_setting_flag & DBG_FLAG_JTAG) == 0) {
			dbg_spare = ccci_read32(md1_dbg_mode_ptr);
			if (dbg_spare & MD_DBG_JTAG_BIT) {
				CCCI_MSG("Jtag Debug mode(%08x)\n", dbg_spare);
				debug_setting_flag |= DBG_FLAG_JTAG;
				*((volatile unsigned int *)(md1_dbg_mode_ptr)) &= (~MD_DBG_JTAG_BIT);
			}
		}
		CCCI_MSG_INF(0, "ctl", "%s debug_setting_flag:%d\n", __func__, debug_setting_flag);
	} else
		CCCI_MSG_INF(0, "ctl", "%s md1_dbg_mode_ptr not remaped!!\n", __func__);

#ifdef MD_DEBUG_MODE
	if ((debug_setting_flag & DBG_FLAG_JTAG) == 0) {
		dbg_spare = *((volatile unsigned int *)(MD_DEBUG_MODE));
		if (dbg_spare & MD_DBG_JTAG_BIT) {
			CCCI_MSG("Jtag Debug mode(%08x)\n", dbg_spare);
			debug_setting_flag |= DBG_FLAG_JTAG;
			*((volatile unsigned int *)(MD_DEBUG_MODE)) &= (~MD_DBG_JTAG_BIT);
		}
	}
#endif /* #ifdef MD_DEBUG_MODE */
	return debug_setting_flag;
}

/* ======================================================== */
/*  ungate/gate md */
/* ======================================================== */
static int ungate_md1(void)
{
	CCCI_MSG_INF(0, "ctl", "ungate_md1\n");

	if ((!md1_boot_slave_Vector) || (!md1_boot_slave_Key)
	    || (!md1_boot_slave_En)) {
		CCCI_MSG_INF(0, "ctl", "map md boot slave base fail!\n");
		return -1;
	}

	/* Power on MD MTCMOS */
	ccci_en_md1_clock();

	/* disable md wdt */
	ccci_write32(WDT_MD_MODE(md1_rgu_base), WDT_MD_MODE_KEY);

	if (get_debug_mode_flag() & DBG_FLAG_JTAG)
		return -1;

	/*set the start address to let modem to run */
	ccci_write32(md1_boot_slave_Key, 0x3567C766);	/* 6582 no change */
	ccci_write32(md1_boot_slave_Vector, 0x0);	/* 6582 no change */
	ccci_write32(md1_boot_slave_En, 0xA3B66175);	/* 6582 no change */

	return 0;
}

void gate_md1(unsigned int timeout)
{
	int ret;

	if (!md1_ccif_base) {
		CCCI_MSG_INF(0, "ctl", "md_ccif_base map fail\n");
		return;
	}

	ret = ccci_dis_md1_clock(timeout);
	CCCI_MSG_INF(0, "ctl", "sleep ret %d\n", ret);

	/* Write MD CCIF Ack to clear AP CCIF busy register */
	CCCI_CTL_MSG(0, "Write MD CCIF Ack to clear AP CCIF busy register\n");
	ccci_write32(MD_CCIF_ACK(md1_ccif_base), ~0U);
}

int let_md_stop(int md_id, unsigned int timeout)
{
	int ret = -1;

	if (md_id == MD_SYS1) {
		gate_md1(timeout);
		ret = 0;
	}

	return ret;
}

int let_md_go(int md_id)
{
	int ret = -1;

	if (md_id == MD_SYS1) {
		md_wdt_has_processed[MD_SYS1] = 0;
		md_dsp_wdt_irq_en(MD_SYS1);
		ret = ungate_md1();
	}

	return ret;
}

static unsigned int INVALID_ADDR[MAX_MD_NUM];	/* (0x3E000000) */
#define INVALID_OFFSET                    (0x02000000)
int set_ap_smem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;

	if (!smem_remapped) {
		smem_remapped = 1;
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 14) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 15) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 16) >> 0) | 1 << 24) & 0xFF000000);

		remap2_val =
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 17) >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 18) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 19) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 20) >> 0) | 1 << 24) & 0xFF000000);

		CCCI_MSG_INF(md_id, "ctl",
			     "AP Smem remap: [%08x]->[%08x](%08x:%08x)\n", des,
			     src, remap1_val, remap2_val);

#ifdef ENABLE_MEM_REMAP_HW
		ccci_write32(AP_BANK4_MAP0(ap_mcu_reg_base), remap1_val);
		ccci_write32(AP_BANK4_MAP1(ap_mcu_reg_base), remap2_val);
		ccci_write32(AP_BANK4_MAP1(ap_mcu_reg_base), remap2_val);
#endif
	}
	return 0;
}

int set_md_smem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;

	des -= KERN_EMI_BASE;
	switch (md_id) {
	case MD_SYS1:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 0) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 1) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 2) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val =
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 3) >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 4) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 5) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 6) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		ccci_write32(MD1_BANK4_MAP0(ap_infra_base), remap1_val);
		ccci_write32(MD1_BANK4_MAP1(ap_infra_base), remap2_val);
#endif

		break;

	default:
		break;
	}

	CCCI_MSG_INF(md_id, "ctl",
		     "MD%d Smem remap:[%08x]->[%08x](%08x:%08x)\n", md_id + 1,
		     des, src, remap1_val, remap2_val);
	return 0;
}

int set_md_rom_rw_mem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;

	des -= KERN_EMI_BASE;
	switch (md_id) {
	case MD_SYS1:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 7) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 8) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 9) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val =
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 10) >> 24) | 0x1) & 0xFF)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 11) >> 16) | 1 << 8) & 0xFF00)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 12) >> 8) | 1 << 16) & 0xFF0000)
		    +
		    ((((INVALID_ADDR[md_id] +
			INVALID_OFFSET * 13) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		ccci_write32(MD1_BANK0_MAP0(ap_infra_base), remap1_val);
		ccci_write32(MD1_BANK0_MAP1(ap_infra_base), remap2_val);
#endif

		break;
	default:
		break;
	}

	CCCI_MSG_INF(md_id, "ctl",
		     "MD%d ROM mem remap:[%08x]->[%08x](%08x:%08x)\n",
		     md_id + 1, des, src, remap1_val, remap2_val);
	return 0;
}

int ccci_ipo_h_platform_restore(int md_id)
{
	int ret = 0;
	unsigned int smem_addr;
	unsigned int md_mem_addr;
	int need_restore = 0;
	/* int                wdt_irq = 0; */

	smem_remapped = 0;

	switch (md_id) {
	case MD_SYS1:
		/* wdt_irq = MD_WDT_IRQ_ID; */
		need_restore = 1;
		break;

	default:
		CCCI_MSG("register MDWDT irq fail: invalid md id(%d)\n", md_id);
		return -1;
	}

	if (need_restore) {
		smem_addr = md_mem_layout_tab[md_id].smem_region_phy_before_map;
		md_mem_addr = md_mem_layout_tab[md_id].md_region_phy;
#ifndef ENABLE_SW_MEM_REMAP
		ret += set_ap_smem_remap(md_id, 0x40000000, smem_addr);
#endif
		ret += set_md_smem_remap(md_id, 0x40000000, smem_addr);
		ret += set_md_rom_rw_mem_remap(md_id, 0x00000000, md_mem_addr);
	}

	if (ret)
		return -1;

	return 0;
}
#ifdef ENABLE_DUMP_MD_REGISTER

void ccci_dump_md_register_region(int md_id, unsigned char *addr,
				  unsigned char *mapaddr, unsigned int len)
{
#define CCCI_REDUCE_LOG
	int index = 0;

#ifdef CCCI_REDUCE_LOG
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x\n", (unsigned int)(addr)
		     , (unsigned int)(mapaddr), *((unsigned int *)(mapaddr)));

	for (index = 0; index <= len; index += 16) {
		CCCI_MSG_INF(md_id, "cci", "%03X: %08X %08X %08X %08X\n", index,
			     *((unsigned int *)(mapaddr + index)),
			     *((unsigned int *)(mapaddr + index + 4))
			     , *((unsigned int *)(mapaddr + index + 8)),
			     *((unsigned int *)(mapaddr + index + 12)));
	}
#else
	for (index = 0; index <= len; index += 4) {
		CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
			     (unsigned int)(addr + index)
			     , (unsigned int)(mapaddr + index),
			     *((unsigned int *)(mapaddr + index)));
	}
#endif
}

void ccci_dump_md_register(int md_id)
{
	int idx;
	unsigned char *ptr;
	unsigned char *map_ptr;
	unsigned int len;

	CCCI_MSG_INF(md_id, "cci", "Dump MD MCU register\n");
	map_ptr = md1_mcu_ptr;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x", MD1_MCU_ADDR_BASE,
		     (unsigned int)(map_ptr), *((unsigned int *)(map_ptr)));
	for (ptr = (unsigned char *)MD1_MCU_ADDR_BASE, idx = 0x100;
	     idx <= 0x464; idx += 16) {
		CCCI_MSG_INF(md_id, "cci", "%03X: %08X %08X %08X %08X\n", idx,
			     *((unsigned int *)(map_ptr + idx)),
			     *((unsigned int *)(map_ptr + idx + 4))
			     , *((unsigned int *)(map_ptr + idx + 8)),
			     *((unsigned int *)(map_ptr + idx + 12)));
	}
	CCCI_MSG_INF(md_id, "cci", "Dump MD M3D register\n");
	ptr = (unsigned char *)MD1_M3D_ADDR_BASE;
	map_ptr = md1_m3d_ptr;
	idx = 0x0;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
		     (unsigned int)(ptr + idx), (unsigned int)(map_ptr + idx),
		     *((unsigned int *)(map_ptr + idx)));

	idx = 0x8;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
		     (unsigned int)(ptr + idx), (unsigned int)(map_ptr + idx),
		     *((unsigned int *)(map_ptr + idx)));
	idx = 0x88;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
		     (unsigned int)(ptr + idx), (unsigned int)(map_ptr + idx),
		     *((unsigned int *)(map_ptr + idx)));
	idx = 0x404;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
		     (unsigned int)(ptr + idx), (unsigned int)(map_ptr + idx),
		     *((unsigned int *)(map_ptr + idx)));
	idx = 0x40C;
	CCCI_MSG_INF(md_id, "cci", "[%08X]->[%08X]=%08x",
		     (unsigned int)(ptr + idx), (unsigned int)(map_ptr + idx),
		     *((unsigned int *)(map_ptr + idx)));

	CCCI_MSG_INF(md_id, "cci", "Dump OS Timer register Start...\n");
	ptr = (unsigned char *)MD1_OS_TIMER_ADDR;
	map_ptr = md1_ost_ptr;
	len = MD1_OS_TIMER_LEN;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);
	CCCI_MSG_INF(md_id, "cci", "Dump OS Timer register End.\n");

	/*  To dump: 0x80120040 - 0x801200B4  (MD side) */
	/*           0x80120100 - 0x80120110 */
	/*           0x80120140 - 0x8012014C */
	CCCI_MSG_INF(md_id, "cci", "Dump MIXED SYS register Start...\n");
	ptr = (unsigned char *)MD1_MIXEDSYS_ADDR + 0x40;
	map_ptr = md1_mixedsys_ptr + 0x40;
	len = 0x801200B4 - 0x80120040;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MIXEDSYS_ADDR + 0x100;
	map_ptr = md1_mixedsys_ptr + 0x100;
	len = 1;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MIXEDSYS_ADDR + 0x140;
	map_ptr = md1_mixedsys_ptr + 0x140;
	len = 0x8012014C - 0x80120140;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	CCCI_MSG_INF(md_id, "cci", "Dump MIXED SYS register End.\n");

	/*  To dump: 0x80030000 - 0x80030110  (MD side) */
	/*           0x80030200 */
	/*           0x80030300 - 0x80030320 */
	/*           0x80030800 - 0x80030910 */
	/*           0x80030A00 - 0x80030A50 */
	CCCI_MSG_INF(md_id, "cci", "Dump MD TOPSM register Start...\n");
	ptr = (unsigned char *)MD1_MD_TOPSM_ADDR;
	map_ptr = md1_md_topsm_ptr;
	len = 0x80030110 - 0x80030000;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MD_TOPSM_ADDR + 0x200;
	map_ptr = md1_md_topsm_ptr + 0x200;
	len = 1;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MD_TOPSM_ADDR + 0x300;
	map_ptr = md1_md_topsm_ptr + 0x300;
	len = 0x80030320 - 0x80030300;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MD_TOPSM_ADDR + 0x800;
	map_ptr = md1_md_topsm_ptr + 0x800;
	len = 0x80030910 - 0x80030800;

	ptr = (unsigned char *)MD1_MD_TOPSM_ADDR + 0xA00;
	map_ptr = md1_md_topsm_ptr + 0xA00;
	len = 0x80030A50 - 0x80030A00;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);
	CCCI_MSG_INF(md_id, "cci", "Dump MD TOPSM register End.\n");

	/*  To dump: 0x83010000 - 0x83010110  (MD side) */
	/*           0x83010200 */
	/*           0x83010300 - 0x83010320 */
	/*           0x83010800 - 0x83010910 */
	/*           0x83010A00 - 0x83010A50 */
	CCCI_MSG_INF(md_id, "cci", "Dump Modem TOPSM register Start...\n");
	ptr = (unsigned char *)MD1_MODEM_TOPSM_ADDR;
	map_ptr = md1_modem_topsm_ptr;
	len = 0x83010110 - 0x83010000;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MODEM_TOPSM_ADDR + 0x200;
	map_ptr = md1_modem_topsm_ptr + 0x200;
	len = 1;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MODEM_TOPSM_ADDR + 0x300;
	map_ptr = md1_modem_topsm_ptr + 0x300;
	len = 0x83010320 - 0x83010300;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MODEM_TOPSM_ADDR + 0x800;
	map_ptr = md1_modem_topsm_ptr + 0x800;
	len = 0x83010910 - 0x83010800;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);

	ptr = (unsigned char *)MD1_MODEM_TOPSM_ADDR + 0xA00;
	map_ptr = md1_modem_topsm_ptr + 0xA00;
	len = 0x83010A50 - 0x83010A00;
	ccci_dump_md_register_region(md_id, ptr, map_ptr, len);
	CCCI_MSG_INF(md_id, "cci", "Dump Modem TOPSM register End.\n");
}
#endif

static int md_init(int md_id)
{
	int ret = 0;
#ifdef ENABLE_DRAM_API
	INVALID_ADDR[md_id] = round_up(get_max_DRAM_size(), INVALID_OFFSET);
#else
	INVALID_ADDR[md_id] = 0X3E000000;
#endif
	CCCI_MSG_INF(md_id, "ctl", "INVALID_ADDR[%d]: %08X\n", md_id,
		     INVALID_ADDR[md_id]);
	ret = ccci_alloc_smem(md_id);
	md_init_stage_flag[md_id] |= CCCI_ALLOC_SMEM_DONE;
	if (ret < 0) {
		CCCI_MSG_INF(md_id, "ctl", "MD memory allocate fail: %d\n",
			     ret);
		return ret;
	}

	md_mem_layout_tab[md_id].md_region_vir =
	    (unsigned int)
	    ioremap_nocache(md_mem_layout_tab[md_id].md_region_phy,
			    MD_IMG_DUMP_SIZE);
	if (!md_mem_layout_tab[md_id].md_region_vir) {
		CCCI_MSG_INF(md_id, "ctl", "MD region ioremap fail!\n");
		return -ENOMEM;
	}
	CCCI_MSG_INF(md_id, "ctl",
		     "md%d: md_rom<P:0x%08x><V:0x%08x>, md_smem<P:0x%08x><V:0x%08x>\n",
		     md_id + 1, md_mem_layout_tab[md_id].md_region_phy,
		     md_mem_layout_tab[md_id].md_region_vir,
		     md_mem_layout_tab[md_id].smem_region_phy_before_map,
		     md_mem_layout_tab[md_id].smem_region_vir);

	md_init_stage_flag[md_id] |= CCCI_MAP_MD_CODE_DONE;

	map_md_side_register(md_id);
	md_init_stage_flag[md_id] |= CCCI_MAP_CTL_REG_DONE;
	ret = md_dsp_irq_init(md_id);
	if (ret != 0)
		return ret;
	md_init_stage_flag[md_id] |= CCCI_WDT_IRQ_REG_DONE;

	load_img_cfg(md_id);

	register_ccci_kern_func_by_md_id(md_id, ID_GET_TXPOWER, get_txpower);

	return ret;
}

static void md_deinit(int md_id)
{
	if (md_init_stage_flag[md_id] & CCCI_ALLOC_SMEM_DONE)
		ccci_free_smem(md_id);

	if (md_init_stage_flag[md_id] & CCCI_MAP_MD_CODE_DONE)
		iounmap((void *)md_mem_layout_tab[md_id].md_region_vir);

	if (md_init_stage_flag[md_id] & CCCI_WDT_IRQ_REG_DONE)
		md_dsp_irq_deinit(md_id);
}

int platform_init(int md_id, int power_down)
{
	int ret = 0;

	ret = md_init(md_id);

	return ret;
}

void platform_deinit(int md_id)
{
	md_deinit(md_id);
}
static int ccci_plat_probe(struct platform_device *plat_dev)
{
	ccif_plat_drv = plat_dev;
	CCCI_MSG_INF(-1, "ctl", "ccci_plat_probe (%p)\n", ccif_plat_drv);
	return 0;
}
static int ccci_plat_remove(struct platform_device *dev)
{
	return 0;
}

static void ccci_plat_shutdown(struct platform_device *dev)
{
}

static int ccci_plat_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}
static int ccci_plat_resume(struct platform_device *dev)
{
	return 0;
}

static int ccci_plat_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);

	return ccci_plat_suspend(pdev, PMSG_SUSPEND);
}

static int ccci_plat_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);

	return ccci_plat_resume(pdev);
}

static int ccci_plat_pm_restore_noirq(struct device *device)
{
	return 0;
}

static const struct dev_pm_ops ccci_plat_pm_ops = {
	.suspend = ccci_plat_pm_suspend,
	.resume = ccci_plat_pm_resume,
	.freeze = ccci_plat_pm_suspend,
	.thaw = ccci_plat_pm_resume,
	.poweroff = ccci_plat_pm_suspend,
	.restore = ccci_plat_pm_resume,
	.restore_noirq = ccci_plat_pm_restore_noirq,
};

static const struct of_device_id mdccif_of_ids[] = {
	{.compatible = "mediatek,ap_ccif0",},
	{}
};


static struct platform_driver ap_ccif0_plat_driver = {
	.driver = {
		   .name = "ap_ccif0",
		   .of_match_table = mdccif_of_ids,
#ifdef CONFIG_PM
		  .pm = &ccci_plat_pm_ops,
#endif

		},
	.probe = ccci_plat_probe,
	.remove = ccci_plat_remove,
	.shutdown = ccci_plat_shutdown,
	.suspend = ccci_plat_suspend,
	.resume = ccci_plat_resume,
};

struct platform_device ap_ccif0_plat_dev = {
	.name = "ap_ccif0",
	.id = -1,
};


int __init ccci_mach_init(void)
{
	int ret = 0;

	CCCI_MSG("Ver. %s\n", CCCI_VERSION);

#ifdef ENABLE_DRAM_API
	/* CCCI_MSG("kernel base:0x%08X, kernel max addr:0x%08X\n", get_phys_offset(), get_max_phys_addr()); */
#endif

	if ((ap_infra_base == 0) || (ap_mcu_reg_base == 0))
		get_dts_settings(0);
	ccci_get_platform_ver(ap_platform);

#ifndef CONFIG_OF
	ap_infra_base = INFRACFG_AO_BASE;
	ap_mcu_reg_base = MCUSYS_CFGREG_BASE;
#endif
	ret = platform_device_register(&ap_ccif0_plat_dev);
	if (ret) {
		CCCI_MSG_INF(-1, "ctl", "ccci_plat_device register fail(%d)\n", ret);
		return ret;
	}
	CCCI_MSG_INF(-1, "ctl", "ccci_plat_device register success\n");

	ret = platform_driver_register(&ap_ccif0_plat_driver);
	if (ret) {
		CCCI_MSG_INF(-1, "ctl", "ccci_plat_driver register fail(%d)\n", ret);
		return ret;
	}
	CCCI_MSG_INF(-1, "ctl", "ccci_plat_driver register success\n");
	return ret;
}

void __exit ccci_mach_exit(void)
{
}

module_init(ccci_mach_init);
module_exit(ccci_mach_exit);

MODULE_DESCRIPTION("CCCI Plaform Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
