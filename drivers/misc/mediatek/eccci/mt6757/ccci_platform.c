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

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mt-plat/mtk_ccci_common.h>
#include "ccci_config.h"
#include "ccci_modem.h"
#include "ccci_bm.h"
#include "ccci_platform.h"

#ifdef ENABLE_EMI_PROTECTION
#include <mach/emi_mpu.h>
#endif
#ifdef FEATURE_USING_4G_MEMORY_API
#include <mt-plat/mtk_lpae.h>
#endif

#define TAG "plat"

int Is_MD_EMI_voilation(void)
{
#ifdef ENABLE_EMI_PROTECTION
	int violation_port = MASTER_MDMCU;

	violation_port = emi_mpu_get_violation_port();
	if ((violation_port == MASTER_MDMCU) || (violation_port == MASTER_MDHW))
		return 1;
	return 0;
#else
	return 1;
#endif
}
/* =================================================== */
/* MPU Region defination */
/* =================================================== */
#define MPU_REGION_ID_MD1_MCURO_HWRW    11
#define MPU_REGION_ID_TOTAL_NUM         24

#ifdef ENABLE_EMI_PROTECTION
/*
* (D7(MDHW),	D6(MFG),	D5(MD3),	D4(MM),	D3(Resv),D2(CONN), D1(MD1), D0(AP))
*/
#define MPU_ACCESS_PERMISSON_CLEAR	SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, \
	NO_PROTECTION,  NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION)

static const unsigned int mpu_attr_default_table[MPU_REGION_ID_TOTAL_NUM][MPU_DOMAIN_ID_TOTAL_NUM] = {
/*===============================================================================================================*/
/* No |  | D0(AP)    | D1(MD1)      | D2(CONN) | D3(Res)  | D4(MM)       | D5(MD3 )      | D6(MFG)    | D7(MDHW)|*/
/*--------------+------------------------------------------------------------------------------------------------*/
/* 0: set in preloder*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*1: Preloader*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 2: SVP share mem, set in TEE*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 3: Trust UI, set in TEE*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 4: MD1 SEC MEM, set in TEE*/
{ SEC_RW, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/* 5: CCCI_MD1, set in LK*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 6: CCCI_MD3, set in LK*/
{ NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/* 7: MD1_MD3, set in LK*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/* 8:   MD1_MCURW_HWRW*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION},
/* 9: MD1_ROM_DSP, set in LK*/
{ SEC_R_NSEC_R, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R},
/*10: MD1_MCURW_HWRO set in LK*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R},
/*11: MD1_MCURO_HWRW set in kernel*/
{ SEC_R_NSEC_R, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION},
/*12: WIFI FW, set in kernel*/
{ FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*13: WMT set in kernel*/
{ NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*14: MD1 R/W, set in LK*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R },
/*15: MD1 logging, set in LK*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN},
/*16: MD3 ROM set in LK*/
{ SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN},
/*17: MD3 R/W, set in LK*/
{ SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/*18: MD Padding1, set in LK*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*19: MD Padding2, set in LK*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*20: MD Protect, set in LK*/
{ NO_PROTECTION, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*21: MD dynamic set in LK*/
{ FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*22: GPS EMI, set in Kernel*/
{ NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*23: AP, set in Kernel */
{ NO_PROTECTION, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN},
};
/*==============================================================================================*/

#endif

#define MPU_REGION_INFO_ID0    0
#define MPU_REGION_INFO_ID1    0
#define MPU_REGION_INFO_ID2    MPU_REGION_ID_MD1_MCURO_HWRW
#define MPU_REGION_INFO_ID3    0
#define MPU_REGION_INFO_ID4    0
#define MPU_REGION_INFO_ID5    0
#define MPU_REGION_INFO_ID6    0
#define MPU_REGION_INFO_ID7    0

static const unsigned int mpu_region_info_id[] = {
MPU_REGION_INFO_ID0, MPU_REGION_INFO_ID1, MPU_REGION_INFO_ID2, MPU_REGION_INFO_ID3,
MPU_REGION_INFO_ID4, MPU_REGION_INFO_ID5, MPU_REGION_INFO_ID6, MPU_REGION_INFO_ID7};

#define CHEAD_MPU_DOMAIN_0      MPU_DOMAIN_ID_AP
#define CHEAD_MPU_DOMAIN_1      MPU_DOMAIN_ID_MD
#define CHEAD_MPU_DOMAIN_2      MPU_DOMAIN_ID_MDHW
#define CHEAD_MPU_DOMAIN_3      0xFF
#define CHEAD_MPU_DOMAIN_LAST   CHEAD_MPU_DOMAIN_2

unsigned long infra_ao_base;
unsigned long dbgapb_base;

void ccci_clear_md_region_protection(struct ccci_modem *md)
{
}

void ccci_clear_dsp_region_protection(struct ccci_modem *md)
{
}

/*
 * for some unkonw reason on 6582 and 6572, MD will read AP's memory during boot up, so we
 * set AP region as MD read-only at first, and re-set it to portected after MD boot up.
 * this function should be called right before sending runtime data.
 */
void ccci_set_ap_region_protection(struct ccci_modem *md)
{

}

void ccci_set_dsp_region_protection(struct ccci_modem *md, int loaded)
{
}
#ifdef ENABLE_EMI_PROTECTION
inline unsigned int EXTRACT_REGION_VALUE(unsigned int domain_set, int region_num)
{
	unsigned int ret;
	unsigned int shift = 4 * region_num;

	ret = (domain_set&(0x0000000F<<shift))>>shift;

	return ret;
}

unsigned int CheckHeader_region_attr_paser(struct ccci_modem *md, unsigned region_id)
{
	unsigned int domain_id;
	unsigned int region_attr, region_dom[8], temp_attr, extract_value;
	unsigned int domain_attr_id[] = {
		[CHEAD_MPU_DOMAIN_0] = MPU_DOMAIN_INFO_ID_MD1,
		[CHEAD_MPU_DOMAIN_1] = MPU_DOMAIN_INFO_ID_MD3,
		[CHEAD_MPU_DOMAIN_2] = MPU_DOMAIN_INFO_ID_MDHW,
	};
	for (domain_id = 0; domain_id < 8; domain_id++) {
		switch (domain_id) {
		case CHEAD_MPU_DOMAIN_0:
		case CHEAD_MPU_DOMAIN_1:
		case CHEAD_MPU_DOMAIN_2:
		case CHEAD_MPU_DOMAIN_3:
			/* different domain attr value  is in different domain_attr[] */
			temp_attr = md->img_info[IMG_MD].rmpu_info.domain_attr[domain_attr_id[domain_id]];
			extract_value = EXTRACT_REGION_VALUE(temp_attr, region_id);
			CCCI_DEBUG_LOG(md->index, TAG, "%d,  temp_attr = %X, extract_value= %X\n",
				domain_attr_id[domain_id], temp_attr, extract_value);
			if ((extract_value >= NO_PROTECTION) && (extract_value <= FORBIDDEN))
				region_dom[domain_id] = extract_value;
			else
				region_dom[domain_id] =
					mpu_attr_default_table[mpu_region_info_id[region_id]][domain_id];
			CCCI_DEBUG_LOG(md->index, TAG, "1. region_dom[%d] = %X\n", domain_id, region_dom[domain_id]);
			break;
		default:
			region_dom[domain_id] = mpu_attr_default_table[mpu_region_info_id[region_id]][domain_id];
			CCCI_DEBUG_LOG(md->index, TAG, "2. region_dom[%d] = %X\n", domain_id, region_dom[domain_id]);
			break;
		}
	}
	region_attr = SET_ACCESS_PERMISSON(region_dom[7], region_dom[6], region_dom[5], region_dom[4],
					region_dom[3], region_dom[2], region_dom[1], region_dom[0]);
	return region_attr;
}
#endif
void ccci_set_mem_access_protection(struct ccci_modem *md)
{
}

#ifdef SET_EMI_STEP_BY_STAGE
/* For HW DE Error: initial value of domain is wrong, we add protection 1st stage */
void ccci_set_mem_access_protection_1st_stage(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
	struct ccci_image_info *img_info;
	struct ccci_mem_layout *md_layout;
	unsigned int region_mpu_id, region_mpu_attr;
	unsigned long long region_mpu_start, region_mpu_end;
	mpu_cfg_t *mpu_cfg_inf;

	switch (md->index) {
	case MD_SYS1:
		if (modem_run_env_ready(MD_SYS1)) {
			CCCI_BOOTUP_LOG(md->index, TAG, "Has protected, only MDHW bypass other 1st step\n");
			img_info = &md->img_info[IMG_MD];
			md_layout = &md->mem_layout;
			region_mpu_id = MPU_REGION_ID_MD1_MCURO_HWRW;
			region_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN,
					FORBIDDEN, FORBIDDEN, NO_PROTECTION, SEC_R_NSEC_R);
			region_mpu_start = md_layout->md_region_phy +
				img_info->rmpu_info.region_info[2].region_offset; /* Note here!!!!!, 2 */
			region_mpu_end =
				((region_mpu_start + img_info->rmpu_info.region_info[2].region_size /* Note here, 2 */
				 + 0xFFFF)&(~0xFFFF)) - 0x1;

			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%llX:%llX> %X\n",
				region_mpu_id, region_mpu_start, region_mpu_end, region_mpu_attr);
			emi_mpu_set_region_protection(region_mpu_start,	/*START_ADDR */
						      region_mpu_end,	/*END_ADDR */
						      region_mpu_id,	/*region */
						      region_mpu_attr);
			mpu_cfg_inf = get_mpu_region_cfg_info(region_mpu_id);
			if (mpu_cfg_inf && (mpu_cfg_inf->relate_region != 0)) {
				emi_mpu_set_region_protection(region_mpu_start,	/*START_ADDR */
								region_mpu_end,	/*END_ADDR */
								mpu_cfg_inf->relate_region,	/*region */
								region_mpu_attr);
				CCCI_BOOTUP_LOG(md->index, TAG, "Relate region<%d> using same setting(@1st)\n",
								mpu_cfg_inf->relate_region);
			} else
				CCCI_BOOTUP_LOG(md->index, TAG, "No relate region for region<%d>(@1st)\n",
								region_mpu_id);
			break;
		}
		break;
	case MD_SYS3:
	default:
		CCCI_DEBUG_LOG(md->index, CORE, "[error]invalid when MPU protect\n");
		return;
	}

#endif
}

void ccci_set_mem_access_protection_second_stage(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
	struct ccci_image_info *img_info;
	struct ccci_mem_layout *md_layout;
	unsigned int region_id, region_mpu_id, region_mpu_attr;
	unsigned long long region_mpu_start, region_mpu_end;

	switch (md->index) {
	case MD_SYS1:
		img_info = &md->img_info[IMG_MD];
		md_layout = &md->mem_layout;
		region_id = MD_SET_REGION_MD1_MCURO_HWRW;/*MPU_REGION_ID_MD1_MCURO_HWRW;*/
		/* set 11 */
		region_mpu_id = mpu_region_info_id[region_id];
		region_mpu_attr = CheckHeader_region_attr_paser(md, region_id);
		region_mpu_start = md_layout->md_region_phy +
			img_info->rmpu_info.region_info[region_id].region_offset;
		region_mpu_end =
		((region_mpu_start + img_info->rmpu_info.region_info[region_id].region_size + 0xFFFF)&(~0xFFFF)) - 0x1;

		CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%llX:%llX> %X\n",
			region_mpu_id, region_mpu_start, region_mpu_end, region_mpu_attr);
		emi_mpu_set_region_protection(region_mpu_start,	/*START_ADDR */
					      region_mpu_end,	/*END_ADDR */
					      region_mpu_id,	/*region */
					      region_mpu_attr);
		break;
	case MD_SYS3:
	default:
		CCCI_DEBUG_LOG(md->index, CORE, "[error]invalid when MPU protect\n");
		return;
	}

#endif
}
#endif

int set_md_smem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	return 0;
}

int set_md_rom_rw_mem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	return 0;
}

void ccci_set_mem_remap(struct ccci_modem *md, unsigned long smem_offset, phys_addr_t invalid)
{
	/*
	 * MD bank4 is remap to nearest 32M aligned address
	 * assume share memoy layout is:
	 * |---AP/MD1--| <--MD1 bank4
	 * |--MD1/MD3--| <--MD3 bank4
	 * |---AP/MD3--|
	 * this should align with LK's remap setting
	 */
	phys_addr_t md_bank4_base;

	switch (md->index) {
	case MD_SYS1:
		md_bank4_base = round_down(md->mem_layout.smem_region_phy, 0x02000000);
		break;
	case MD_SYS3:
		md_bank4_base = round_down(md->mem_layout.md1_md3_smem_phy, 0x02000000);
		break;
	default:
		md_bank4_base = 0;
		break;
	}
	md->invalid_remap_base = invalid;
	/*
	 * AP_view_addr - md_bank4_base + 0x40000000 = MD_view_addr
	 * AP_view_addr - smem_offset_AP_to_MD = MD_view_addr
	 */
	md->mem_layout.smem_offset_AP_to_MD = md_bank4_base - 0x40000000;
	invalid = md->mem_layout.smem_region_phy + md->mem_layout.smem_region_size;
	CCCI_NORMAL_LOG(md->index, TAG, "%s 0x%llX 0x%X\n", __func__,
		(unsigned long long)md_bank4_base, md->mem_layout.smem_offset_AP_to_MD);
}

/*
 * when MD attached its codeviser for debuging, this bit will be set. so CCCI should disable some
 * checkings and operations as MD may not respond to us.
 */
unsigned int ccci_get_md_debug_mode(struct ccci_modem *md)
{
	unsigned int dbg_spare;
	static unsigned int debug_setting_flag;
	/* this function does NOT distinguish modem ID, may be a risk point */
	if ((debug_setting_flag & DBG_FLAG_JTAG) == 0) {
		dbg_spare = ioread32((void __iomem *)(dbgapb_base + 0x10));
		if (dbg_spare & MD_DBG_JTAG_BIT) {
			CCCI_NORMAL_LOG(md->index, TAG, "Jtag Debug mode(%08x)\n", dbg_spare);
			debug_setting_flag |= DBG_FLAG_JTAG;
			mt_reg_sync_writel(dbg_spare & (~MD_DBG_JTAG_BIT), (dbgapb_base + 0x10));
		}
	}
	return debug_setting_flag;
}
EXPORT_SYMBOL(ccci_get_md_debug_mode);

void ccci_get_platform_version(char *ver)
{
#ifdef ENABLE_CHIP_VER_CHECK
	sprintf(ver, "MT%04x_S%02x", get_chip_hw_ver_code(), (get_chip_hw_subcode() & 0xFF));
#else
	sprintf(ver, "MTxxxx_S00");
#endif
}

#ifdef FEATURE_LOW_BATTERY_SUPPORT
static int ccci_md_low_power_notify(struct ccci_modem *md, LOW_POEWR_NOTIFY_TYPE type, int level)
{
	unsigned int reserve = 0xFFFFFFFF;
	int ret = 0;

	CCCI_NORMAL_LOG(md->index, TAG, "low power notification type=%d, level=%d\n", type, level);
	/*
	 * byte3 byte2 byte1 byte0
	 *    0   4G   3G   2G
	 */
	switch (type) {
	case LOW_BATTERY:
		if (level == LOW_BATTERY_LEVEL_0)
			reserve = 0;	/* 0 */
		else if (level == LOW_BATTERY_LEVEL_1 || level == LOW_BATTERY_LEVEL_2)
			reserve = (1 << 6);	/* 64 */
		ret = port_proxy_send_msg_to_md(md->port_proxy, CCCI_SYSTEM_TX, MD_LOW_BATTERY_LEVEL, reserve, 1);
		if (ret)
			CCCI_ERROR_LOG(md->index, TAG, "send low battery notification fail, ret=%d\n", ret);
		break;
	case BATTERY_PERCENT:
		if (level == BATTERY_PERCENT_LEVEL_0)
			reserve = 0;	/* 0 */
		else if (level == BATTERY_PERCENT_LEVEL_1)
			reserve = (1 << 6);	/* 64 */
		ret = port_proxy_send_msg_to_md(md->port_proxy, CCCI_SYSTEM_TX, MD_LOW_BATTERY_LEVEL, reserve, 1);
		if (ret)
			CCCI_ERROR_LOG(md->index, TAG, "send battery percent notification fail, ret=%d\n", ret);
		break;
	default:
		break;
	};

	return ret;
}

static void ccci_md_low_battery_cb(LOW_BATTERY_LEVEL level)
{
	int idx = 0;
	struct ccci_modem *md;

	for (idx = 0; idx < MAX_MD_NUM; idx++) {
		md = ccci_md_get_modem_by_id(idx);
		if (md != NULL)
			ccci_md_low_power_notify(md, LOW_BATTERY, level);
	}
}

static void ccci_md_battery_percent_cb(BATTERY_PERCENT_LEVEL level)
{
	int idx = 0;
	struct ccci_modem *md;

	for (idx = 0; idx < MAX_MD_NUM; idx++) {
		md = ccci_md_get_modem_by_id(idx);
		if (md != NULL)
			ccci_md_low_power_notify(md, BATTERY_PERCENT, level);
	}
}
#endif

#define PCCIF_BUSY (0x4)
#define PCCIF_TCHNUM (0xC)
#define PCCIF_ACK (0x14)
#define PCCIF_CHDATA (0x100)
#define PCCIF_SRAM_SIZE (512)

void ccci_reset_ccif_hw(struct ccci_modem *md, int ccif_id, void __iomem *baseA, void __iomem *baseB)
{
	int i;
#if 0
	{
		unsigned int tx_channel = 0;

		/* clear occupied channel */
		while (tx_channel < 16) {
			if (ccci_read32(baseA, PCCIF_BUSY) & (1<<tx_channel))
				ccci_write32(baseA, PCCIF_TCHNUM, tx_channel);

			if (ccci_read32(baseB, PCCIF_BUSY) & (1<<tx_channel))
				ccci_write32(baseB, PCCIF_TCHNUM, tx_channel);

			tx_channel++;
		}
		/* clear un-ached channel */
		ccci_write32(baseA, PCCIF_ACK, ccci_read32(baseB, PCCIF_BUSY));
		ccci_write32(baseB, PCCIF_ACK, ccci_read32(baseA, PCCIF_BUSY));
	}
#else
	{
		int reset_bit = -1;
		unsigned int reg_value;

		switch (ccif_id) {
		case AP_MD1_CCIF:
			reset_bit = 8;
			break;
		case AP_MD3_CCIF:
			reset_bit = 7;
			break;
		case MD1_MD3_CCIF:
			reset_bit = 6;
			break;
		}

		if (reset_bit == -1)
			return;

		/* this reset bit will clear CCIF's busy/wch/irq, but not SRAM */

		reg_value = ccci_read32(infra_ao_base, 0x150);
		reg_value &= ~(1 << reset_bit);
		reg_value |= (1 << reset_bit);
		ccci_write32(infra_ao_base, 0x150, reg_value);

		reg_value = ccci_read32(infra_ao_base, 0x154);
		reg_value &= ~(1 << reset_bit);
		reg_value |= (1 << reset_bit);
		ccci_write32(infra_ao_base, 0x154, reg_value);
	}
#endif
	/* clear SRAM */
	for (i = 0; i < PCCIF_SRAM_SIZE/sizeof(unsigned int); i++) {
		ccci_write32(baseA, PCCIF_CHDATA+i*sizeof(unsigned int), 0);
		ccci_write32(baseB, PCCIF_CHDATA+i*sizeof(unsigned int), 0);
	}
}

int ccci_platform_init(struct ccci_modem *md)
{
	return 0;
}

int ccci_plat_common_init(void)
{
	struct device_node *node;
	/* Get infra cfg ao base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	infra_ao_base = (unsigned long)of_iomap(node, 0);
	CCCI_INIT_LOG(-1, TAG, "infra_ao_base:0x%p\n", (void *)infra_ao_base);
	node = of_find_compatible_node(NULL, NULL, "mediatek,dbgapb_base");
	dbgapb_base = (unsigned long)of_iomap(node, 0);
	CCCI_INIT_LOG(-1, TAG, "dbgapb_base:%pa\n", &dbgapb_base);
#ifdef FEATURE_LOW_BATTERY_SUPPORT
	register_low_battery_notify(&ccci_md_low_battery_cb, LOW_BATTERY_PRIO_MD);
	register_battery_percent_notify(&ccci_md_battery_percent_cb, BATTERY_PERCENT_PRIO_MD);
#endif
	return 0;
}

struct ccci_ccb_config ccb_configs[] = {
};
unsigned int ccb_configs_len = sizeof(ccb_configs)/sizeof(struct ccci_ccb_config);

