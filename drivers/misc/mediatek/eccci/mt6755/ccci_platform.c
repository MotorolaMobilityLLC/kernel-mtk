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
#include <mt-plat/mt_ccci_common.h>
#include "ccci_config.h"
#include "ccci_core.h"
#include "ccci_debug.h"
#include "ccci_bm.h"
#include "ccci_platform.h"
#ifdef FEATURE_USING_4G_MEMORY_API
#include <mach/mt_lpae.h>
#endif

#ifdef ENABLE_EMI_PROTECTION
#include <mach/emi_mpu.h>
#endif
#define TAG "plat"

static int is_4g_memory_size_support(void)
{
#ifdef FEATURE_USING_4G_MEMORY_API
	return enable_4G();
#else
	return 0;
#endif
}
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
#define MPU_REGION_ID_SEC_OS            0
#define MPU_REGION_ID_ATF               1
/* #define MPU_REGION_ID_MD32_SMEM     2 */
#define MPU_REGION_ID_TRUSTED_UI        3
#define MPU_REGION_ID_MD1_SEC_SMEM      4

#define MPU_REGION_ID_MD1_SMEM          5
#define MPU_REGION_ID_MD3_SMEM          6
#define MPU_REGION_ID_MD1MD3_SMEM       7
#define MPU_REGION_ID_MD1_MCURW_HWRW    8
#define MPU_REGION_ID_MD1_ROM           9  /* contain DSP */
#define MPU_REGION_ID_MD1_MCURW_HWRO    10
#define MPU_REGION_ID_MD1_MCURO_HWRW    11
#define MPU_REGION_ID_WIFI_EMI_FW       12
#define MPU_REGION_ID_WMT               13
#define MPU_REGION_ID_MD1_RW            14
#define MPU_REGION_ID_MD3_ROM           15
#define MPU_REGION_ID_MD3_RW            16
#define MPU_REGION_ID_AP                17
#define MPU_REGION_ID_TOTAL_NUM         (MPU_REGION_ID_AP + 1)

#ifdef ENABLE_EMI_PROTECTION
/* ////////////////////////////////////////////////////////////// (D7(MDHW),       D6(MFG), \
	D5(MD3),        D4(MM),        D3(Resv),      D2(CONN),      D1(MD1),       D0(AP)) */
#define MPU_ACCESS_PERMISSON_CLEAR	SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, \
	NO_PROTECTION,  NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION)
/* maybe no use */
#define MPU_ACCESS_PERMISSON_AP_MD1_RO_ATTR  SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, \
	FORBIDDEN,  NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN,  NO_PROTECTION)
/* #define MPU_REGION_ID_AP            17 */
#define MPU_ACCESS_PERMISSON_AP_ATTR         SET_ACCESS_PERMISSON(FORBIDDEN,     NO_PROTECTION, \
	FORBIDDEN,      NO_PROTECTION, FORBIDDEN,     FORBIDDEN,     FORBIDDEN,     NO_PROTECTION)
/* #define MPU_REGION_ID_MD1_ROM_DSP   9 */
#define MPU_ACCESS_PERMISSON_MD1_ROM_ATTR    SET_ACCESS_PERMISSON(SEC_R_NSEC_R,     FORBIDDEN,     \
	FORBIDDEN,      FORBIDDEN,     FORBIDDEN,     FORBIDDEN,     SEC_R_NSEC_R,  SEC_R_NSEC_R)
/* #define MPU_REGION_ID_MD1_RW        14 */
#define MPU_ACCESS_PERMISSON_MD1_RW_ATTR     SET_ACCESS_PERMISSON(SEC_R_NSEC_R,     FORBIDDEN,     \
	FORBIDDEN,      FORBIDDEN,     FORBIDDEN,     FORBIDDEN,     NO_PROTECTION, FORBIDDEN)
/* #define MPU_REGION_ID_MD1_SMEM      5 */
#define MPU_ACCESS_PERMISSON_MD1_SMEM_ATTR   SET_ACCESS_PERMISSON(FORBIDDEN,     FORBIDDEN,     \
	FORBIDDEN,      FORBIDDEN,     FORBIDDEN,     FORBIDDEN,     NO_PROTECTION, NO_PROTECTION)
/* #define MPU_REGION_ID_MD3_ROM       15 */
#define MPU_ACCESS_PERMISSON_MD3_ROM_ATTR    SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R)
/* #define MPU_REGION_ID_MD3_RW        16 */
#define MPU_ACCESS_PERMISSON_MD3_RW_ATTR     SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R)
/* #define MPU_REGION_ID_MD3_SMEM      6 */
#define MPU_ACCESS_PERMISSON_MD3_SMEM_ATTR   SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION)
/* #define MPU_REGION_ID_MD1MD3_SMEM   7 */
#define MPU_ACCESS_PERMISSON_MD1MD3_SMEM_ATTR   SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION)
/* #define MPU_REGION_ID_MD1_MCURO_HWRW   11 */
#define MPU_ACCESS_PERMISSON_MD1RO_HWRW_ATTR   SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, \
	FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, SEC_R_NSEC_R)

static const unsigned int MPU_ATTR_DEFAULT[MPU_REGION_ID_TOTAL_NUM][MPU_DOMAIN_ID_TOTAL_NUM] = {
/*===================================================================================================================*/
/* No |  | D0(AP)    | D1(MD1)      | D2(CONN) | D3(Res)  | D4(MM)       | D5(MD3 )      | D6(MFG)     | D7(MDHW)   |*/
/*--------------+----------------------------------------------------------------------------------------------------*/
/* 0*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/* 1*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/* 2*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/* 3*/{ SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_RW       , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/* 4*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
/* 5*/{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/* 6*/{ NO_PROTECTION, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
/* 7*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
/* 8*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION},
/* 9*/{ SEC_R_NSEC_R , SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_R_NSEC_R },
/*10*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_R_NSEC_R },
/*11*/{ SEC_R_NSEC_R , SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION},
/*12*/{ FORBIDDEN    , FORBIDDEN    , NO_PROTECTION, FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/*13*/{ NO_PROTECTION, FORBIDDEN    , NO_PROTECTION, FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN    , FORBIDDEN},
/*14*/{ SEC_R_NSEC_R , NO_PROTECTION, FORBIDDEN    , FORBIDDEN, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, SEC_R_NSEC_R },
/*15*/{ SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN},
/*16*/{ SEC_R_NSEC_R , FORBIDDEN    , FORBIDDEN    , FORBIDDEN, FORBIDDEN    , NO_PROTECTION, FORBIDDEN    , FORBIDDEN},
/*17*/{ NO_PROTECTION, FORBIDDEN    , FORBIDDEN    , FORBIDDEN, NO_PROTECTION, FORBIDDEN    , NO_PROTECTION, FORBIDDEN},
}; /*=================================================================================================================*/
#endif

#define MPU_REGION_INFO_ID0    MPU_REGION_ID_MD1_ROM
#define MPU_REGION_INFO_ID1    MPU_REGION_ID_MD1_MCURW_HWRO
#define MPU_REGION_INFO_ID2    MPU_REGION_ID_MD1_MCURO_HWRW
#define MPU_REGION_INFO_ID3    MPU_REGION_ID_MD1_MCURW_HWRW
#define MPU_REGION_INFO_ID4    MPU_REGION_ID_MD1_RW
#define MPU_REGION_INFO_ID5    0
#define MPU_REGION_INFO_ID6    0
#define MPU_REGION_INFO_ID7    0
#define MPU_REGION_INFO_ID(X)  MPU_REGION_INFO_ID##X

static const unsigned int MPU_REGION_INFO_ID[] = {
MPU_REGION_INFO_ID0, MPU_REGION_INFO_ID1, MPU_REGION_INFO_ID2, MPU_REGION_INFO_ID3,
MPU_REGION_INFO_ID4, MPU_REGION_INFO_ID5, MPU_REGION_INFO_ID6, MPU_REGION_INFO_ID7};

#define CHEAD_MPU_DOMAIN_0      MPU_DOMAIN_ID_AP
#define CHEAD_MPU_DOMAIN_1      MPU_DOMAIN_ID_MD
#define CHEAD_MPU_DOMAIN_2      MPU_DOMAIN_ID_MDHW
#define CHEAD_MPU_DOMAIN_3      0xFF
#define CHEAD_MPU_DOMAIN_LAST   CHEAD_MPU_DOMAIN_2
#define MPU_DOMAIN_NUM_OF_ID(X)    MPU_DOMAIN_NUM_OF_ID##X
/* domain_attr */
static const unsigned int CHEAD_MPU_DOMAIN_ID[] = {
CHEAD_MPU_DOMAIN_0, CHEAD_MPU_DOMAIN_1, CHEAD_MPU_DOMAIN_2, CHEAD_MPU_DOMAIN_3};

unsigned long infra_ao_base;
unsigned long dbgapb_base;
/* -- MD1 Bank 0 */
#define MD1_BANK0_MAP0 ((unsigned int *)(infra_ao_base+0x300))
#define MD1_BANK0_MAP1 ((unsigned int *)(infra_ao_base+0x304))
/* -- MD1 Bank 4 */
#define MD1_BANK4_MAP0 ((unsigned int *)(infra_ao_base+0x310))
#define MD1_BANK4_MAP1 ((unsigned int *)(infra_ao_base+0x314))

/* -- MD2 Bank 0 */
#define MD2_BANK0_MAP0 ((unsigned int *)(infra_ao_base+0x310))
#define MD2_BANK0_MAP1 ((unsigned int *)(infra_ao_base+0x314))
/* -- MD2 Bank 4 */
#define MD2_BANK4_MAP0 ((unsigned int *)(infra_ao_base+0x318))
#define MD2_BANK4_MAP1 ((unsigned int *)(infra_ao_base+0x31C))

/*-- MD3 Bank 0 */
#define MD3_BANK0_MAP0 ((unsigned int *)(infra_ao_base+0x320))
#define MD3_BANK0_MAP1 ((unsigned int *)(infra_ao_base+0x324))
/*-- MD3 Bank 4 */
#define MD3_BANK4_MAP0 ((unsigned int *)(infra_ao_base+0x330))
#define MD3_BANK4_MAP1 ((unsigned int *)(infra_ao_base+0x334))

void ccci_clear_md_region_protection(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
	unsigned int rom_mem_mpu_id, rw_mem_mpu_id;
	mpu_cfg_t *mpu_cfg_inf;
	if (modem_run_env_ready(md->index)) { /* LK has did it, bypass this step */
		CCCI_BOOTUP_LOG(md->index, TAG, "Ignore Clear MPU for md%d\n", md->index+1);
		return;
	}
	switch (md->index) {
	case MD_SYS1:
		rom_mem_mpu_id = MPU_REGION_ID_MD1_ROM;
		rw_mem_mpu_id = MPU_REGION_ID_MD1_RW;
		break;
	case MD_SYS3:
		rom_mem_mpu_id = MPU_REGION_ID_MD3_ROM;
		rw_mem_mpu_id = MPU_REGION_ID_MD3_RW;
		break;
	default:
		CCCI_BOOTUP_LOG(md->index, TAG, "[error]MD ID invalid when clear MPU protect\n");
		return;
	}

	CCCI_BOOTUP_LOG(md->index, TAG, "Clear MPU protect MD ROM region<%d>\n", rom_mem_mpu_id);
	emi_mpu_set_region_protection(0,	/*START_ADDR */
				      0,	/*END_ADDR */
				      rom_mem_mpu_id,	/*region */
				      MPU_ACCESS_PERMISSON_CLEAR);

	CCCI_BOOTUP_LOG(md->index, TAG, "Clear MPU protect MD R/W region<%d>\n", rw_mem_mpu_id);
	emi_mpu_set_region_protection(0,	/*START_ADDR */
				      0,	/*END_ADDR */
				      rw_mem_mpu_id,	/*region */
				      MPU_ACCESS_PERMISSON_CLEAR);
#ifdef SET_EMI_STEP_BY_STAGE
/*=======================================*/
/* - Clear HW-related region protection -*/
/*---------------------------------------*/
	if (md->index == MD_SYS1) {
		/*---------------------------------------------------------------------------*/
		CCCI_BOOTUP_LOG(md->index, TAG, "Clear MPU protect HWRW R/W region<%d>\n",
							MPU_REGION_ID_MD1_MCURW_HWRW);
		emi_mpu_set_region_protection(0,                       /*START_ADDR*/
						0,                       /*END_ADDR*/
						MPU_REGION_ID_MD1_MCURW_HWRW,   /*region*/
						MPU_ACCESS_PERMISSON_CLEAR);
		mpu_cfg_inf = get_mpu_region_cfg_info(MPU_REGION_ID_MD1_MCURW_HWRW);
		if (mpu_cfg_inf && (mpu_cfg_inf->relate_region != 0)) { /*Region 0 used by security, it is safe */
			emi_mpu_set_region_protection(0,                       /*START_ADDR*/
							0,                       /*END_ADDR*/
							mpu_cfg_inf->relate_region,   /*region*/
							MPU_ACCESS_PERMISSON_CLEAR);
			CCCI_BOOTUP_LOG(md->index, TAG, "Relate region<%d> using same setting\n",
							mpu_cfg_inf->relate_region);
		} else
			CCCI_BOOTUP_LOG(md->index, TAG, "No relate region for region<%d>\n",
							MPU_REGION_ID_MD1_MCURW_HWRW);

		/*---------------------------------------------------------------------------*/
		CCCI_BOOTUP_LOG(md->index, TAG, "Clear MPU protect HWRW ROM region<%d>\n",
						MPU_REGION_ID_MD1_MCURW_HWRO);
		emi_mpu_set_region_protection(0,                       /*START_ADDR*/
						0,                       /*END_ADDR*/
						MPU_REGION_ID_MD1_MCURW_HWRO,   /*region*/
						MPU_ACCESS_PERMISSON_CLEAR);
		mpu_cfg_inf = get_mpu_region_cfg_info(MPU_REGION_ID_MD1_MCURW_HWRO);
		if (mpu_cfg_inf && (mpu_cfg_inf->relate_region != 0)) { /*Region 0 used by security, it is safe */
			emi_mpu_set_region_protection(0,                       /*START_ADDR*/
							0,                       /*END_ADDR*/
							mpu_cfg_inf->relate_region,   /*region*/
							MPU_ACCESS_PERMISSON_CLEAR);
			CCCI_BOOTUP_LOG(md->index, TAG, "Relate region<%d> using same setting\n",
							mpu_cfg_inf->relate_region);
		} else
			CCCI_BOOTUP_LOG(md->index, TAG, "No relate region for region<%d>\n",
							MPU_REGION_ID_MD1_MCURW_HWRO);

		/*---------------------------------------------------------------------------*/
		CCCI_BOOTUP_LOG(md->index, TAG, "Clear MPU protect HWRO R/W region<%d>\n",
						MPU_REGION_ID_MD1_MCURO_HWRW);
		emi_mpu_set_region_protection(0,                       /*START_ADDR*/
						0,                       /*END_ADDR*/
						MPU_REGION_ID_MD1_MCURO_HWRW,   /*region*/
						MPU_ACCESS_PERMISSON_CLEAR);
		mpu_cfg_inf = get_mpu_region_cfg_info(MPU_REGION_ID_MD1_MCURO_HWRW);
		if (mpu_cfg_inf && (mpu_cfg_inf->relate_region != 0)) { /*Region 0 used by security, it is safe */
			emi_mpu_set_region_protection(0,                       /*START_ADDR*/
							0,                       /*END_ADDR*/
							mpu_cfg_inf->relate_region,   /*region*/
							MPU_ACCESS_PERMISSON_CLEAR);
			CCCI_BOOTUP_LOG(md->index, TAG, "Relate region<%d> using same setting\n",
							mpu_cfg_inf->relate_region);
		} else
			CCCI_BOOTUP_LOG(md->index, TAG, "No relate region for region<%d>\n",
							MPU_REGION_ID_MD1_MCURO_HWRW);
	}

#endif

#endif
}

void ccci_clear_dsp_region_protection(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
#endif
}

/*
 * for some unkonw reason on 6582 and 6572, MD will read AP's memory during boot up, so we
 * set AP region as MD read-only at first, and re-set it to portected after MD boot up.
 * this function should be called right before sending runtime data.
 */
void ccci_set_ap_region_protection(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
#ifdef SET_AP_MPU_REGION
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned int kernel_base;
	unsigned int dram_size;

	if (is_4g_memory_size_support())
		kernel_base = 0;
	else
		kernel_base = get_phys_offset();
#ifdef ENABLE_DRAM_API
	dram_size = get_max_DRAM_size();
#else
	dram_size = 256*1024*1024;
#endif
	ap_mem_mpu_id = MPU_REGION_ID_AP;
	ap_mem_mpu_attr = MPU_ACCESS_PERMISSON_AP_ATTR;

	CCCI_NORMAL_LOG(md->index, TAG, "MPU Start protect AP region<%d:%08x:%08x> %x\n",
			ap_mem_mpu_id, kernel_base, (kernel_base+dram_size-1), ap_mem_mpu_attr);
	emi_mpu_set_region_protection(kernel_base,
					(kernel_base+dram_size-1),
					ap_mem_mpu_id,
					ap_mem_mpu_attr);
#endif
#endif
}

void ccci_set_dsp_region_protection(struct ccci_modem *md, int loaded)
{
#ifdef ENABLE_EMI_PROTECTION
#endif
}
#ifdef ENABLE_EMI_PROTECTION
inline unsigned int EXTRACT_REGION_VALUE(unsigned int domain_set, int region_num)
{
	unsigned int ret;
	unsigned int shift = 4 * region_num;

	ret = (domain_set&(0x0000000F<<shift))>>shift;

	return ret;
}

/* region_id: 0~7 0=MPU_DOMAIN_INFO_ID_MD1 ->CHEAD_MPU_DOMAIN_ID[MPU_DOMAIN_INFO_ID_MD1]-- domain 1 MD1*/
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
				region_dom[domain_id] = MPU_ATTR_DEFAULT[MPU_REGION_INFO_ID[region_id]][domain_id];
			CCCI_DEBUG_LOG(md->index, TAG, "1. region_dom[%d] = %X\n", domain_id, region_dom[domain_id]);
			break;
		default:
			region_dom[domain_id] = MPU_ATTR_DEFAULT[MPU_REGION_INFO_ID[region_id]][domain_id];
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
#ifdef ENABLE_EMI_PROTECTION
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id, shr_mem_mpu_attr;
	unsigned int rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_id, rom_mem_mpu_attr;
	unsigned int rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_id, rw_mem_mpu_attr;
#ifdef SET_AP_MPU_REGION
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
#endif
	unsigned int shr_mem13_phy_start, shr_mem13_phy_end, shr_mem13_mpu_id, shr_mem13_mpu_attr;
	unsigned int region_id;
	struct ccci_image_info *img_info;
	struct ccci_mem_layout *md_layout;
#ifdef SET_AP_MPU_REGION
	unsigned int kernel_base;
	unsigned int dram_size;
#endif
	unsigned int by_pass_setting = 0;
	/* check header version is newer than v4 */
	CCCI_BOOTUP_LOG(md->index, TAG, "CCCI Image header version = %d\n", md->img_info[IMG_MD].img_info.header_verno);
	if (md->index == MD_SYS1 && md->img_info[IMG_MD].img_info.header_verno < 4) {
		CCCI_BOOTUP_LOG(md->index, TAG, "CCCI Image header version is %d ,RMPU Only support after v4\n",
			md->img_info[IMG_MD].img_info.header_verno);
		return;
	}

	if (modem_run_env_ready(md->index)) {
		CCCI_BOOTUP_LOG(md->index, TAG, "Has protected, bypass\n");
		by_pass_setting = 1;
	}
	switch (md->index) {
	case MD_SYS1:
		img_info = &md->img_info[IMG_MD];
		md_layout = &md->mem_layout;
		/* region 9 */
		region_id = MD_SET_REGION_MD1_ROM_DSP;
		rom_mem_mpu_id = MPU_REGION_ID_MD1_ROM; /*MPU_REGION_INFO_ID[region_id]; */
		rom_mem_mpu_attr = CheckHeader_region_attr_paser(md, region_id);
		rom_mem_phy_start = (unsigned int)md_layout->md_region_phy +
				img_info->rmpu_info.region_info[region_id].region_offset;
		rom_mem_phy_end =
		((rom_mem_phy_start + img_info->rmpu_info.region_info[region_id].region_size + 0xFFFF)&(~0xFFFF))
				- 0x1;
		if (by_pass_setting == 0) {
			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%08X:%08X> %X\n",
					     rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_attr);
			emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
						      rom_mem_phy_end,	/*END_ADDR */
						      rom_mem_mpu_id,	/*region */
						      rom_mem_mpu_attr);
		}
		shr_mem_mpu_id = MPU_REGION_ID_MD1_SMEM;/* 5 */
		shr_mem_mpu_attr = MPU_ACCESS_PERMISSON_MD1_SMEM_ATTR;/* 5 */
		break;
	case MD_SYS3:
		img_info = &md->img_info[IMG_MD];
		md_layout = &md->mem_layout;
		rom_mem_mpu_id = MPU_REGION_ID_MD3_ROM;
		rw_mem_mpu_id =	MPU_REGION_ID_MD3_RW;
		shr_mem_mpu_id = MPU_REGION_ID_MD3_SMEM;
		shr_mem13_mpu_id = MPU_REGION_ID_MD1MD3_SMEM;
		rom_mem_mpu_attr = MPU_ACCESS_PERMISSON_MD3_ROM_ATTR;
		rw_mem_mpu_attr = MPU_ACCESS_PERMISSON_MD3_RW_ATTR;
		shr_mem_mpu_attr = MPU_ACCESS_PERMISSON_MD3_SMEM_ATTR;
		shr_mem13_mpu_attr = MPU_ACCESS_PERMISSON_MD1MD3_SMEM_ATTR;
		/*
		 * if set start=0x0, end=0x10000, the actural protected area will be 0x0-0x1FFFF,
		 * here we use 64KB align, MPU actually request 32KB align since MT6582, but this works...
		 * we assume emi_mpu_set_region_protection will round end address down to 64KB align.
		 */
		rom_mem_phy_start = (unsigned int)md_layout->md_region_phy;
		rom_mem_phy_end = ((rom_mem_phy_start + img_info->size + 0xFFFF) & (~0xFFFF)) - 0x1;
		rw_mem_phy_start = rom_mem_phy_end + 0x1;
		rw_mem_phy_end = rom_mem_phy_start + md_layout->md_region_size - 0x1;
#ifdef ENABLE_DSP_SMEM_SHARE_MPU_REGION
		rw_mem_phy_end += md_layout->smem_region_size;
#endif
		/* MD1 MD3 share memory */
		shr_mem13_phy_start = (unsigned int)md_layout->md1_md3_smem_phy;
		shr_mem13_phy_end = ((shr_mem13_phy_start + md_layout->md1_md3_smem_size + 0xFFFF) & (~0xFFFF)) - 0x1;

		if (by_pass_setting == 0) {
			CCCI_BOOTUP_LOG(md->index, TAG,
					"MPU protect MD ROM region<%d:%08x:%08x> %x,invalid_map=0x%llx\n",
					rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_attr,
					(unsigned long long)md->invalid_remap_base);
			emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
					rom_mem_phy_end,	/*END_ADDR */
					rom_mem_mpu_id,	/*region */
					rom_mem_mpu_attr);

			CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD R/W region<%d:%08x:%08x> %x\n",
				     rw_mem_mpu_id, rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_attr);
			emi_mpu_set_region_protection(rw_mem_phy_start,	/*START_ADDR */
						      rw_mem_phy_end,	/*END_ADDR */
						      rw_mem_mpu_id,	/*region */
						      rw_mem_mpu_attr);
		}

		CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD1&3 Share region<%d:%08x:%08x> %x\n",
			     shr_mem13_mpu_id, shr_mem13_phy_start, shr_mem13_phy_end, shr_mem13_mpu_attr);
		emi_mpu_set_region_protection(shr_mem13_phy_start,	/*START_ADDR */
					      shr_mem13_phy_end,	/*END_ADDR */
					      shr_mem13_mpu_id,	/*region */
					      shr_mem13_mpu_attr);

		break;
	default:
		CCCI_BOOTUP_LOG(md->index, CORE, "[error]invalid when MPU protect\n");
		return;
	}
#ifdef SET_AP_MPU_REGION
	if (is_4g_memory_size_support())
		kernel_base = 0;
	else
		kernel_base = get_phys_offset();
#ifdef ENABLE_DRAM_API
	dram_size = get_max_DRAM_size();
#else
	dram_size = 256 * 1024 * 1024;
#endif
	ap_mem_mpu_id = MPU_REGION_ID_AP;
	ap_mem_mpu_attr = MPU_ACCESS_PERMISSON_AP_MD1_RO_ATTR;
#endif

	shr_mem_phy_start = (unsigned int)md_layout->smem_region_phy;
	shr_mem_phy_end = ((shr_mem_phy_start + md_layout->smem_region_size + 0xFFFF) & (~0xFFFF)) - 0x1;

#ifndef ENABLE_DSP_SMEM_SHARE_MPU_REGION
	CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD Share region<%d:%08x:%08x> %x\n",
		     shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);
#endif
/* This part need to move common part */
#ifdef SET_AP_MPU_REGION
	CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect AP region<%d:%08x:%08x> %x\n",
		     ap_mem_mpu_id, kernel_base, (kernel_base + dram_size - 1), ap_mem_mpu_attr);
	emi_mpu_set_region_protection(kernel_base, (kernel_base + dram_size - 1), ap_mem_mpu_id, ap_mem_mpu_attr);
#endif
#endif
}

#ifdef SET_EMI_STEP_BY_STAGE
/* For HW DE Error: initial value of domain is wrong, we add protection 1st stage */
void ccci_set_mem_access_protection_1st_stage(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
	struct ccci_image_info *img_info;
	struct ccci_mem_layout *md_layout;
	unsigned int region_id, region_mpu_id, region_mpu_attr, region_mpu_start, region_mpu_end;
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
			region_mpu_start = (unsigned int)md_layout->md_region_phy +
				img_info->rmpu_info.region_info[2].region_offset; /* Note here!!!!!, 2 */
			region_mpu_end =
				((region_mpu_start + img_info->rmpu_info.region_info[2].region_size /* Note here, 2 */
				 + 0xFFFF)&(~0xFFFF)) - 0x1;

			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%08X:%08X> %X\n",
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
		img_info = &md->img_info[IMG_MD];
		md_layout = &md->mem_layout;

		for (region_id = MD_SET_REGION_MD1_MCURW_HWRO; region_id < MPU_REGION_INFO_ID_TOTAL_NUM; region_id++) {
			/* set 8, 10, 14 region, except 11 */
			region_mpu_id = MPU_REGION_INFO_ID[region_id];
			region_mpu_start = (unsigned int)md_layout->md_region_phy +
				img_info->rmpu_info.region_info[region_id].region_offset;
			region_mpu_end =
				((region_mpu_start + img_info->rmpu_info.region_info[region_id].region_size
				+ 0xFFFF)&(~0xFFFF)) - 0x1;
			if (region_mpu_id == MPU_REGION_ID_MD1_MCURO_HWRW)
				region_mpu_attr = MPU_ACCESS_PERMISSON_MD1RO_HWRW_ATTR;
			else
				region_mpu_attr = CheckHeader_region_attr_paser(md, region_id);
			/*if ((region_mpu_id == MPU_REGION_ID_MD1_MCURO_HWRW)
				|| (region_mpu_id == MPU_REGION_ID_MD1_MCURW_HWRW)
				|| (region_mpu_id == MPU_REGION_ID_MD1_MCURW_HWRO)) {
				CCCI_NORMAL_LOG(md->index, TAG, "Bypass MPU protect region <%d:%08X:%08X> %X\n",
					region_mpu_id, region_mpu_start, region_mpu_end, region_mpu_attr);
				CCCI_NORMAL_LOG(md->index, TAG, "BYPASS region <%d:>\n", region_mpu_id);
				continue;
			}*/
			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%08X:%08X> %X\n",
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
	unsigned int region_id, region_mpu_id, region_mpu_attr, region_mpu_start, region_mpu_end;
	mpu_cfg_t *mpu_cfg_inf;

	switch (md->index) {
	case MD_SYS1:
		img_info = &md->img_info[IMG_MD];
		md_layout = &md->mem_layout;
		region_id = MD_SET_REGION_MD1_MCURO_HWRW;/*MPU_REGION_ID_MD1_MCURO_HWRW;*/
		/* set 11 */
		region_mpu_id = MPU_REGION_INFO_ID[region_id];
		region_mpu_attr = CheckHeader_region_attr_paser(md, region_id);
		region_mpu_start = (unsigned int)md_layout->md_region_phy +
			img_info->rmpu_info.region_info[region_id].region_offset;
		region_mpu_end =
		((region_mpu_start + img_info->rmpu_info.region_info[region_id].region_size + 0xFFFF)&(~0xFFFF)) - 0x1;

		CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%08X:%08X> %X\n",
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
			CCCI_BOOTUP_LOG(md->index, TAG, "Relate region<%d> using same setting(@2nd)\n",
							mpu_cfg_inf->relate_region);
		} else
			CCCI_BOOTUP_LOG(md->index, TAG, "No relate region for region<%d>(@2nd)\n",
							region_mpu_id);
		break;
	case MD_SYS3:
	default:
		CCCI_DEBUG_LOG(md->index, CORE, "[error]invalid when MPU protect\n");
		return;
	}

#endif
}
#endif

#ifdef ENABLE_DSP_SMEM_SHARE_MPU_REGION
void ccci_set_exp_region_protection(struct ccci_modem *md)
{
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id, shr_mem_mpu_attr;

	shr_mem_phy_start = (unsigned int)md->mem_layout.smem_region_phy;
	shr_mem_phy_end = ((shr_mem_phy_start + md->mem_layout.smem_region_size + 0xFFFF) & (~0xFFFF)) - 0x1;
	shr_mem_mpu_id = MPU_REGION_ID_MD1_SMEM;
	shr_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION);

	CCCI_NORMAL_LOG(md->index, TAG, "After EE: MPU Start protect MD Share region<%d:%08x:%08x> %x\n",
		     shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);
}
#endif

/* This function has phase out!!! */
int set_ap_smem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;
	static int smem_remapped;

	if (!smem_remapped) {
		smem_remapped = 1;
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    + (((INVALID_ADDR >> 16) | 1 << 8) & 0xFF00)
		    + (((INVALID_ADDR >> 8) | 1 << 16) & 0xFF0000)
		    + (((INVALID_ADDR >> 0) | 1 << 24) & 0xFF000000);

		remap2_val = (((INVALID_ADDR >> 24) | 0x1) & 0xFF)
		    + (((INVALID_ADDR >> 16) | 1 << 8) & 0xFF00)
		    + (((INVALID_ADDR >> 8) | 1 << 16) & 0xFF0000)
		    + (((INVALID_ADDR >> 0) | 1 << 24) & 0xFF000000);

		CCCI_NORMAL_LOG(md->index, TAG, "AP Smem remap: [%llx]->[%llx](%08x:%08x)\n", (unsigned long long)des,
			     (unsigned long long)src, remap1_val, remap2_val);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, AP_BANK4_MAP0);
		mt_reg_sync_writel(remap2_val, AP_BANK4_MAP1);
		mt_reg_sync_writel(remap2_val, AP_BANK4_MAP1);	/* HW bug, write twice to activate setting */
#endif
	}
	return 0;
}

int set_md_smem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;

	if (is_4g_memory_size_support())
		des &= 0xFFFFFFFF;
	else
		des -= KERN_EMI_BASE;

	switch (md->index) {
	case MD_SYS1:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    + ((((des + 0x2000000 * 1) >> 16) | 1 << 8) & 0xFF00)
		    + ((((des + 0x2000000 * 2) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((des + 0x2000000 * 3) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val = ((((des + 0x2000000 * 4) >> 24) | 0x1) & 0xFF)
		    + ((((des + 0x2000000 * 5) >> 16) | 1 << 8) & 0xFF00)
		    + ((((des + 0x2000000 * 6) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((des + 0x2000000 * 7) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD1_BANK4_MAP0);
		mt_reg_sync_writel(remap2_val, MD1_BANK4_MAP1);
#endif
		break;
	case MD_SYS2:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    + ((((invalid + 0x2000000 * 0) >> 16) | 1 << 8) & 0xFF00)
		    + ((((invalid + 0x2000000 * 1) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((invalid + 0x2000000 * 2) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val = ((((invalid + 0x2000000 * 3) >> 24) | 0x1) & 0xFF)
		    + ((((invalid + 0x2000000 * 4) >> 16) | 1 << 8) & 0xFF00)
		    + ((((invalid + 0x2000000 * 5) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((invalid + 0x2000000 * 6) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD2_BANK4_MAP0);
		mt_reg_sync_writel(remap2_val, MD2_BANK4_MAP1);
#endif
		break;
	case MD_SYS3:
		remap1_val = (((des>>24)|0x1)&0xFF)
				  + ((((invalid+0x2000000*0)>>16)|1<<8)&0xFF00)
				  + ((((invalid+0x2000000*1)>>8)|1<<16)&0xFF0000)
				  + ((((invalid+0x2000000*2)>>0)|1<<24)&0xFF000000);
		remap2_val = ((((invalid+0x2000000*3)>>24)|0x1)&0xFF)
				  + ((((invalid+0x2000000*4)>>16)|1<<8)&0xFF00)
				  + ((((invalid+0x2000000*5)>>8)|1<<16)&0xFF0000)
				  + ((((invalid+0x2000000*6)>>0)|1<<24)&0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD3_BANK4_MAP0);
		mt_reg_sync_writel(remap2_val, MD3_BANK4_MAP1);
#endif
		break;
	default:
		break;
	}

	CCCI_INIT_LOG(md->index, TAG, "MD Smem remap:[%llx]->[%llx](%08x:%08x), invalid_map=0x%llx\n",
		     (unsigned long long)des, (unsigned long long)src, remap1_val, remap2_val,
		     (unsigned long long)md->invalid_remap_base);
	return 0;
}

int set_md_rom_rw_mem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;

	if (modem_run_env_ready(md->index)) {
		CCCI_BOOTUP_LOG(md->index, TAG, "RO_RW has mapped\n");
		return 0;
	}
	CCCI_BOOTUP_LOG(md->index, TAG, "Kernel RO_RW mapping\n");

	if (is_4g_memory_size_support())
		des &= 0xFFFFFFFF;
	else
		des -= KERN_EMI_BASE;

	switch (md->index) {
	case MD_SYS1:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    + ((((des + 0x2000000 * 1) >> 16) | 1 << 8) & 0xFF00)
		    + ((((des + 0x2000000 * 2) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((des + 0x2000000 * 3) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val = ((((des + 0x2000000 * 4) >> 24) | 0x1) & 0xFF)
		    + ((((des + 0x2000000 * 5) >> 16) | 1 << 8) & 0xFF00)
		    + ((((des + 0x2000000 * 6) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((des + 0x2000000 * 7) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD1_BANK0_MAP0);
		mt_reg_sync_writel(remap2_val, MD1_BANK0_MAP1);
#endif
		break;
	case MD_SYS2:
		remap1_val = (((des >> 24) | 0x1) & 0xFF)
		    + ((((des + 0x2000000 * 1) >> 16) | 1 << 8) & 0xFF00)
		    + ((((des + 0x2000000 * 2) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((invalid + 0x2000000 * 7) >> 0) | 1 << 24) & 0xFF000000);
		remap2_val = ((((invalid + 0x2000000 * 8) >> 24) | 0x1) & 0xFF)
		    + ((((invalid + 0x2000000 * 9) >> 16) | 1 << 8) & 0xFF00)
		    + ((((invalid + 0x2000000 * 10) >> 8) | 1 << 16) & 0xFF0000)
		    + ((((invalid + 0x2000000 * 11) >> 0) | 1 << 24) & 0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD2_BANK0_MAP0);
		mt_reg_sync_writel(remap2_val, MD2_BANK0_MAP1);
#endif
		break;
	case MD_SYS3:
		remap1_val = (((des>>24)|0x1)&0xFF)
				  + ((((des+0x2000000*1)>>16)|1<<8)&0xFF00)
				  + ((((des+0x2000000*2)>>8)|1<<16)&0xFF0000)
				  + ((((invalid+0x2000000*7)>>0)|1<<24)&0xFF000000);
		remap2_val = ((((invalid+0x2000000*8)>>24)|0x1)&0xFF)
				  + ((((invalid+0x2000000*9)>>16)|1<<8)&0xFF00)
				  + ((((invalid+0x2000000*10)>>8)|1<<16)&0xFF0000)
				  + ((((invalid+0x2000000*11)>>0)|1<<24)&0xFF000000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD3_BANK0_MAP0);
		mt_reg_sync_writel(remap2_val, MD3_BANK0_MAP1);
#endif
		break;

	default:
		break;
	}

	CCCI_NORMAL_LOG(md->index, TAG, "MD ROM mem remap:[%llx]->[%llx](%08x:%08x)\n", (unsigned long long)des,
		     (unsigned long long)src, remap1_val, remap2_val);
	return 0;
}

void ccci_set_mem_remap(struct ccci_modem *md, unsigned long smem_offset, phys_addr_t invalid)
{
	unsigned long remainder;

	if (is_4g_memory_size_support())
		invalid &= 0xFFFFFFFF;
	else
		invalid -= KERN_EMI_BASE;
	md->invalid_remap_base = invalid;
	/* Set share memory remapping */
#if 0	/* no hardware AP remap after MT6592 */
	set_ap_smem_remap(md, 0x40000000, md->mem_layout.smem_region_phy_before_map);
	md->mem_layout.smem_region_phy = smem_offset + 0x40000000;
#endif
	/*
	 * always remap only the 1 slot where share memory locates. smem_offset is the offset between
	 * ROM start address(32M align) and share memory start address.
	 * (AP view smem address) - [(smem_region_phy) - (bank4 start address) - (un-32M-align space)]
	 * = (MD view smem address)
	 */
	remainder = smem_offset % 0x02000000;
	md->mem_layout.smem_offset_AP_to_MD = md->mem_layout.smem_region_phy - (remainder + 0x40000000);
	set_md_smem_remap(md, 0x40000000, md->mem_layout.md_region_phy + (smem_offset - remainder), invalid);
	CCCI_INIT_LOG(md->index, TAG, "AP to MD share memory offset 0x%X", md->mem_layout.smem_offset_AP_to_MD);

	/* Set md image and rw runtime memory remapping */
	set_md_rom_rw_mem_remap(md, 0x00000000, md->mem_layout.md_region_phy, invalid);
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
	sprintf(ver, "MT6735_S00");
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
		ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_LOW_BATTERY_LEVEL, reserve, 1);
		if (ret)
			CCCI_ERROR_LOG(md->index, TAG, "send low battery notification fail, ret=%d\n", ret);
		break;
	case BATTERY_PERCENT:
		if (level == BATTERY_PERCENT_LEVEL_0)
			reserve = 0;	/* 0 */
		else if (level == BATTERY_PERCENT_LEVEL_1)
			reserve = (1 << 6);	/* 64 */
		ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_LOW_BATTERY_LEVEL, reserve, 1);
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
		md = ccci_get_modem_by_id(idx);
		if (md != NULL)
			ccci_md_low_power_notify(md, LOW_BATTERY, level);
	}
}

static void ccci_md_battery_percent_cb(BATTERY_PERCENT_LEVEL level)
{
	int idx = 0;
	struct ccci_modem *md;

	for (idx = 0; idx < MAX_MD_NUM; idx++) {
		md = ccci_get_modem_by_id(idx);
		if (md != NULL)
			ccci_md_low_power_notify(md, BATTERY_PERCENT, level);
	}
}
#endif

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
