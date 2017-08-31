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
#define MPU_REGION_ID_SEC_OS            0
#define MPU_REGION_ID_ATF               1
#define MPU_REGION_ID_SVP_SMEM          2
#define MPU_REGION_ID_TRUSTED_UI        5
#define MPU_REGION_ID_MD1_SEC_SMEM      6
#define MPU_REGION_ID_MD3_SEC_SMEM      7

#define MPU_REGION_ID_MD1_SMEM          8
#define MPU_REGION_ID_MD3_SMEM          9
#define MPU_REGION_ID_MD1MD3_SMEM       10
#define MPU_REGION_ID_MD1_MCURW_HWRW    11
#define MPU_REGION_ID_MD1_ROM           12  /* contain DSP in Jade */
#define MPU_REGION_ID_MD1_MCURW_HWRO    13
#define MPU_REGION_ID_MD1_MCURO_HWRW    14
#define MPU_REGION_ID_WIFI_EMI_FW       15
#define MPU_REGION_ID_WMT               16
#define MPU_REGION_ID_MD1_CCB           17
#define MPU_REGION_ID_MD3_CCB           18
#define MPU_REGION_ID_MD3_ROM           19
#define MPU_REGION_ID_MD3_RW            20
#define MPU_REGION_ID_PADDING1          21
#define MPU_REGION_ID_PADDING2          22
#define MPU_REGION_ID_PADDING3          23
#define MPU_REGION_ID_PADDING4          24
#define MPU_REGION_ID_PADDING5          25
#define MPU_REGION_ID_MD_PROTECT        26
#define MPU_REGION_ID_MD_DYNAMIC        27

#define MPU_REGION_ID_AP                31
#define MPU_REGION_ID_TOTAL_NUM         (MPU_REGION_ID_AP + 1)


#ifdef ENABLE_EMI_PROTECTION
/* ////////////////////////////////////////////////////////////// (D7(MDHW),       D6(MFG), \
*	D5(MD3),        D4(MM),        D3(Resv),      D2(CONN),      D1(MD1),       D0(AP))
*/
#define MPU_ACCESS_PERMISSON_CLEAR	SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, \
	NO_PROTECTION,  NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION)
/* maybe no use, used by setting of ap region */
#define MPU_ACCESS_PERMISSON_AP_MD1_RO_ATTR  SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, \
	FORBIDDEN,  NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN,  NO_PROTECTION)
/* #define MPU_REGION_ID_AP            19 used by setting of ap region */
#define MPU_ACCESS_PERMISSON_AP_ATTR         SET_ACCESS_PERMISSON(FORBIDDEN,     NO_PROTECTION, \
	FORBIDDEN,      NO_PROTECTION, FORBIDDEN,     FORBIDDEN,     FORBIDDEN,     NO_PROTECTION)
/* #define MPU_REGION_ID_MD1_SMEM      8  infact: in LK*/
#define MPU_ACCESS_PERMISSON_MD1_SMEM_ATTR   SET_ACCESS_PERMISSON(NO_PROTECTION,     FORBIDDEN,     \
	FORBIDDEN,      FORBIDDEN,     NO_PROTECTION,     FORBIDDEN,     NO_PROTECTION, NO_PROTECTION)
/* #define MPU_REGION_ID_MD3_ROM       19 infact: in LK
* note that D3 was set to NO_PROTECTION, but the mpu indicate it be SEC_R_NSEC_R
*/
#define MPU_ACCESS_PERMISSON_MD3_ROM_ATTR    SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R)
/* #define MPU_REGION_ID_MD3_RW        20 infact: in LK */
#define MPU_ACCESS_PERMISSON_MD3_RW_ATTR     SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R)
/* #define MPU_REGION_ID_MD3_SMEM      9 infact: in LK */
#define MPU_ACCESS_PERMISSON_MD3_SMEM_ATTR   SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION)
/* #define MPU_REGION_ID_MD1MD3_SMEM   10 infact: in LK , AP need to clear smem, so set it to NO_PROTECTION*/
#define MPU_ACCESS_PERMISSON_MD1MD3_SMEM_ATTR   SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
	NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION)

static const unsigned int MPU_ATTR_DEFAULT[MPU_REGION_ID_TOTAL_NUM][MPU_DOMAIN_ID_TOTAL_NUM] = {
/*===============================================================================*/
/* No || D0(AP)| D1(MD1)| D2(CONN) | D3(SCP)| D4(MM)| D5(MD3)| D6(MFG)| D7(MDHW)|*/
/*--------------+----------------------------------------------------------------*/
/* 0*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 1*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 2*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 3*/
{ FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 4*/
{ NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 5*/
{ SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 6*/
{ SEC_RW, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/* 7*/
{ SEC_RW, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*8*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*9*/
{ NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/*10*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/*11*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION},
/*12*/
{ SEC_R_NSEC_R, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R},
/*13*/
{ SEC_R_NSEC_R, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R},
/*14*/
{ SEC_R_NSEC_R, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION},
/*15*/
{ FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*16*/
{ NO_PROTECTION, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*17*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION},
/*18*/
{ NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/*19*/
{ SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN},
/*20*/
{ SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN},
/*21*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*22*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*23*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*24*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*25*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*26*/
{ NO_PROTECTION, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, SEC_RW, FORBIDDEN},
/*27*/
{ FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*28*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*29*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*30*/
{ NO_PROTECTION, NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN},
/*31*/
{ NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, SEC_RW, FORBIDDEN},
};
#endif

#define MPU_REGION_INFO_ID0    MPU_REGION_ID_MD1_ROM
#define MPU_REGION_INFO_ID1    MPU_REGION_ID_MD1_MCURO_HWRW
#define MPU_REGION_INFO_ID2    MPU_REGION_ID_MD1_MCURW_HWRO
#define MPU_REGION_INFO_ID3    MPU_REGION_ID_MD1_MCURW_HWRW
#define MPU_REGION_INFO_ID4    0
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
#define MD1_BANK0_MAP0 (infra_ao_base + 0x300)
#define MD1_BANK0_MAP1 (infra_ao_base + 0x304)
#define MD1_BANK0_MAP2 (infra_ao_base + 0x308)
#define MD1_BANK0_MAP3 (infra_ao_base + 0x30C)
/* -- MD1 Bank 1 */
#define MD1_BANK1_MAP0 (infra_ao_base + 0x310)
#define MD1_BANK1_MAP1 (infra_ao_base + 0x314)
#define MD1_BANK1_MAP2 (infra_ao_base + 0x318)
#define MD1_BANK1_MAP3 (infra_ao_base + 0x31C)

/* -- MD1 Bank 4 */
#define MD1_BANK4_MAP0 (infra_ao_base + 0x320)
#define MD1_BANK4_MAP1 (infra_ao_base + 0x324)
#define MD1_BANK4_MAP2 (infra_ao_base + 0x328)
#define MD1_BANK4_MAP3 (infra_ao_base + 0x32C)

/* HW remap for MD3(C2K) */
/* -- MD3 Bank 0 */
#define MD3_BANK0_MAP0 (infra_ao_base + 0x330)
#define MD3_BANK0_MAP1 (infra_ao_base + 0x334)
#define MD3_BANK0_MAP2 (infra_ao_base + 0x338)
#define MD3_BANK0_MAP3 (infra_ao_base + 0x33C)

/* -- MD3 Bank 4 */
#define MD3_BANK4_MAP0 (infra_ao_base + 0x350)
#define MD3_BANK4_MAP1 (infra_ao_base + 0x354)
#define MD3_BANK4_MAP2 (infra_ao_base + 0x358)
#define MD3_BANK4_MAP3 (infra_ao_base + 0x35C)


void ccci_clear_md_region_protection(struct ccci_modem *md)
{
#ifdef ENABLE_EMI_PROTECTION
	if (modem_run_env_ready(md->index)) { /* LK has did it, bypass this step */
		CCCI_ERROR_LOG(md->index, TAG, "Ignore Clear MPU for md%d\n", md->index+1);
		return;
	}

#ifdef SET_EMI_STEP_BY_STAGE
/*=======================================*/
/* - Clear HW-related region protection -*/
/*---------------------------------------*/
	if (md->index == MD_SYS1) {
		CCCI_NORMAL_LOG(md->index, TAG, "Clear MPU protect HWRW R/W region<%d>\n",
						MPU_REGION_ID_MD1_MCURW_HWRW);
		emi_mpu_set_region_protection(0,                       /*START_ADDR*/
						0,                       /*END_ADDR*/
						MPU_REGION_ID_MD1_MCURW_HWRW,   /*region*/
						MPU_ACCESS_PERMISSON_CLEAR);

	CCCI_NORMAL_LOG(md->index, TAG, "Clear MPU protect HWRW ROM region<%d>\n",
						MPU_REGION_ID_MD1_MCURW_HWRO);
	emi_mpu_set_region_protection(0,                       /*START_ADDR*/
					0,                       /*END_ADDR*/
					MPU_REGION_ID_MD1_MCURW_HWRO,   /*region*/
					MPU_ACCESS_PERMISSON_CLEAR);

		CCCI_NORMAL_LOG(md->index, TAG, "Clear MPU protect HWRO R/W region<%d>\n",
						MPU_REGION_ID_MD1_MCURO_HWRW);
		emi_mpu_set_region_protection(0,                       /*START_ADDR*/
						0,                       /*END_ADDR*/
						MPU_REGION_ID_MD1_MCURO_HWRW,   /*region*/
						MPU_ACCESS_PERMISSON_CLEAR);
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
	unsigned long long kernel_base;
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

	CCCI_NORMAL_LOG(md->index, TAG, "MPU Start protect AP region<%d:%llx:%llx> %x\n",
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
	unsigned long long shr_mem_phy_start, shr_mem_phy_end;
	unsigned long long rom_mem_phy_start, rom_mem_phy_end;
	unsigned long long rw_mem_phy_start, rw_mem_phy_end;
	unsigned int rw_mem_mpu_id, rw_mem_mpu_attr;
	unsigned int shr_mem_mpu_id, shr_mem_mpu_attr;
	unsigned int rom_mem_mpu_id, rom_mem_mpu_attr;
#ifdef SET_AP_MPU_REGION
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
#endif
	unsigned long long shr_mem13_phy_start, shr_mem13_phy_end;
	unsigned int shr_mem13_mpu_id, shr_mem13_mpu_attr;
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
		rom_mem_phy_start = md_layout->md_region_phy +
				img_info->rmpu_info.region_info[region_id].region_offset;
		rom_mem_phy_end =
		((rom_mem_phy_start + img_info->rmpu_info.region_info[region_id].region_size + 0xFFFF)&(~0xFFFF))
				- 0x1;
		if (by_pass_setting == 0) {
			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%llX:%llX> %X\n",
					     rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_attr);
			emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
						      rom_mem_phy_end,	/*END_ADDR */
						      rom_mem_mpu_id,	/*region */
						      rom_mem_mpu_attr);
		}
		shr_mem_mpu_id = MPU_REGION_ID_MD1_SMEM;/* 8 */
		shr_mem_mpu_attr = MPU_ACCESS_PERMISSON_MD1_SMEM_ATTR;/* 8 */
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
		rom_mem_phy_start = md_layout->md_region_phy;
		rom_mem_phy_end = ((rom_mem_phy_start + img_info->size + 0xFFFF) & (~0xFFFF)) - 0x1;
		rw_mem_phy_start = rom_mem_phy_end + 0x1;
		rw_mem_phy_end = rom_mem_phy_start + md_layout->md_region_size - 0x1;
#ifdef ENABLE_DSP_SMEM_SHARE_MPU_REGION
		rw_mem_phy_end += md_layout->smem_region_size;
#endif
		/* MD1 MD3 share memory */
		shr_mem13_phy_start = md_layout->md1_md3_smem_phy;
		shr_mem13_phy_end = ((shr_mem13_phy_start + md_layout->md1_md3_smem_size + 0xFFFF) & (~0xFFFF)) - 0x1;

		if (by_pass_setting == 0) {
			CCCI_BOOTUP_LOG(md->index, TAG,
					"MPU protect MD ROM region<%d:%llx:%llx> %x,invalid_map=0x%llx\n",
					rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_attr,
					(unsigned long long)md->invalid_remap_base);
			emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
					rom_mem_phy_end,	/*END_ADDR */
					rom_mem_mpu_id,	/*region */
					rom_mem_mpu_attr);

			CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD R/W region<%d:%llx:%llx> %x\n",
					rw_mem_mpu_id, rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_attr);
			emi_mpu_set_region_protection(rw_mem_phy_start,	/*START_ADDR */
					rw_mem_phy_end,	/*END_ADDR */
					rw_mem_mpu_id,	/*region */
					rw_mem_mpu_attr);
		}

		CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD1&3 Share region<%d:%llx:%llx> %x\n",
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

	shr_mem_phy_start = md_layout->smem_region_phy;
	shr_mem_phy_end = ((shr_mem_phy_start + md_layout->smem_region_size + 0xFFFF) & (~0xFFFF)) - 0x1;

#ifndef ENABLE_DSP_SMEM_SHARE_MPU_REGION
	CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect MD Share region<%d:%llx:%llx> %x\n",
		     shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);
#endif
/* This part need to move common part */
#ifdef SET_AP_MPU_REGION
	CCCI_BOOTUP_LOG(md->index, TAG, "MPU protect AP region<%d:%llx:%llx> %x\n",
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
	unsigned int region_mpu_id, region_mpu_attr;
	mpu_cfg_t *mpu_cfg_inf, *related_mpu_cfg_inf;
	int has_relate = 0;
	int next = 0;

	switch (md->index) {
	case MD_SYS1:
		if (modem_run_env_ready(MD_SYS1)) {
			CCCI_BOOTUP_LOG(md->index, TAG, "Has protected, only MDHW bypass other 1st step\n");

			region_mpu_id = MPU_REGION_ID_MD1_MCURO_HWRW;
			region_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, FORBIDDEN, FORBIDDEN,
					FORBIDDEN, FORBIDDEN, NO_PROTECTION, SEC_R_NSEC_R);

			mpu_cfg_inf = get_mpu_region_cfg_info(region_mpu_id);


			if (mpu_cfg_inf) {
				CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%u:%u> %X\n",
						region_mpu_id, mpu_cfg_inf->start, mpu_cfg_inf->end, region_mpu_attr);
				emi_mpu_set_region_protection(mpu_cfg_inf->start,	/*START_ADDR */
						mpu_cfg_inf->end,	/*END_ADDR */
						mpu_cfg_inf->region,	/*region */
						region_mpu_attr);
				next = mpu_cfg_inf->relate_region;
			}

			while (next != 0) {
				related_mpu_cfg_inf = get_mpu_region_cfg_info(next);

				if (related_mpu_cfg_inf) {
					has_relate = 1;
					emi_mpu_set_region_protection(related_mpu_cfg_inf->start,	/*START_ADDR */
						related_mpu_cfg_inf->end,	/*END_ADDR */
						related_mpu_cfg_inf->region,	/*region */
						region_mpu_attr);
					CCCI_BOOTUP_LOG(md->index, TAG, "MPU <%d> -- <%d> using same setting(@1st)\n",
								mpu_cfg_inf->region, next);
					next = related_mpu_cfg_inf->relate_region; /* goto next */
				} else {
					CCCI_ERROR_LOG(md->index, TAG, "MPU <%d> -- <%d> setting not found(@1st)\n",
								mpu_cfg_inf->region, next);
					break;
				}
			}

			if (!has_relate)
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
	unsigned int region_mpu_id;
	mpu_cfg_t *mpu_cfg_inf, *related_mpu_cfg_inf;
	int next = 0;
	int has_relate = 0;

	switch (md->index) {
	case MD_SYS1:
		region_mpu_id = MPU_REGION_ID_MD1_MCURO_HWRW;
		mpu_cfg_inf = get_mpu_region_cfg_info(region_mpu_id);

		if (mpu_cfg_inf) {
			CCCI_BOOTUP_LOG(md->index, TAG, "Start MPU protect region <%d:%u:%u> %X\n",
					mpu_cfg_inf->region, mpu_cfg_inf->start, mpu_cfg_inf->end,
					mpu_cfg_inf->permission);
			emi_mpu_set_region_protection(mpu_cfg_inf->start,	/*START_ADDR */
					mpu_cfg_inf->end,	/*END_ADDR */
					mpu_cfg_inf->region,	/*region */
					mpu_cfg_inf->permission);
			next = mpu_cfg_inf->relate_region;
		}

		while (next != 0) {
			related_mpu_cfg_inf = get_mpu_region_cfg_info(next);

			if (related_mpu_cfg_inf) {
				has_relate = 1;
				emi_mpu_set_region_protection(related_mpu_cfg_inf->start,	/*START_ADDR */
								related_mpu_cfg_inf->end,	/*END_ADDR */
								related_mpu_cfg_inf->region,	/*region */
								mpu_cfg_inf->permission);
				CCCI_BOOTUP_LOG(md->index, TAG, "MPU <%d> -- <%d> using same setting(@2nd)\n",
								mpu_cfg_inf->region, next);
				next = related_mpu_cfg_inf->relate_region; /* goto next */
			} else {
				CCCI_ERROR_LOG(md->index, TAG, "MPU <%d> -- <%d> setting not found(@2nd)\n",
								mpu_cfg_inf->region, next);
				break;
			}
		}
		if (!has_relate)
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
	unsigned long long shr_mem_phy_start, shr_mem_phy_end;
	unsigned int shr_mem_mpu_id, shr_mem_mpu_attr;

	shr_mem_phy_start = md->mem_layout.smem_region_phy;
	shr_mem_phy_end = ((shr_mem_phy_start + md->mem_layout.smem_region_size + 0xFFFF) & (~0xFFFF)) - 0x1;
	shr_mem_mpu_id = MPU_REGION_ID_MD1_SMEM;
	shr_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION);

	CCCI_NORMAL_LOG(md->index, TAG, "After EE: MPU Start protect MD Share region<%d:%llx:%llx> %x\n",
		     shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);
}
#endif

int set_md_smem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	switch (md->index) {
	case MD_SYS1:
		CCCI_INIT_LOG(md->index, TAG, "MD Smem remap:[%llx]->[%llx](%08x:%08x:%08x:%08x), invalid_map=0x%llx\n",
				(unsigned long long)des, (unsigned long long)src,
				ccci_read32(MD1_BANK4_MAP0, 0),
				ccci_read32(MD1_BANK4_MAP1, 0),
				ccci_read32(MD1_BANK4_MAP2, 0),
				ccci_read32(MD1_BANK4_MAP3, 0),
				(unsigned long long)md->invalid_remap_base);
		break;
	case MD_SYS3:
		CCCI_INIT_LOG(md->index, TAG, "MD Smem remap:[%llx]->[%llx](%08x:%08x:%08x:%08x), invalid_map=0x%llx\n",
		     (unsigned long long)des, (unsigned long long)src,
			 ccci_read32(MD3_BANK4_MAP0, 0),
			 ccci_read32(MD3_BANK4_MAP1, 0),
			 ccci_read32(MD3_BANK4_MAP2, 0),
			 ccci_read32(MD3_BANK4_MAP3, 0),
			 (unsigned long long)md->invalid_remap_base);
		break;
	default:
		break;
	}
	return 0;
}

int set_md_rom_rw_mem_remap(struct ccci_modem *md, phys_addr_t src, phys_addr_t des, phys_addr_t invalid)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;
	unsigned int remap3_val = 0;
	unsigned int remap4_val = 0;

	if (modem_run_env_ready(md->index)) {
		CCCI_BOOTUP_LOG(md->index, TAG, "RO_RW has mapped\n");
		return 0;
	}
	CCCI_BOOTUP_LOG(md->index, TAG, "Kernel RO_RW mapping\n");

	switch (md->index) {
	case MD_SYS1:

		remap1_val = (((des >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*1) >> 8) | 1<<16) & 0x3FF0000);
		remap2_val = ((((des + 0x2000000*2) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*3) >> 8) | 1<<16) & 0x3FF0000);
		remap3_val = ((((des + 0x2000000*4) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*5) >> 8) | 1<<16) & 0x3FF0000);
		remap4_val = ((((des + 0x2000000*6) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*7) >> 8) | 1<<16) & 0x3FF0000);
#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD1_BANK0_MAP0);
		mt_reg_sync_writel(remap2_val, MD1_BANK0_MAP1);
		mt_reg_sync_writel(remap3_val, MD1_BANK0_MAP2);
		mt_reg_sync_writel(remap4_val, MD1_BANK0_MAP3);
#endif
		break;
	case MD_SYS3:
		remap1_val = (((des >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*1) >> 8) | 1<<16) & 0x3FF0000);
		remap2_val = ((((des + 0x2000000*2) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*3) >> 8) | 1<<16) & 0x3FF0000);
		remap3_val = ((((des + 0x2000000*4) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*5) >> 8) | 1<<16) & 0x3FF0000);
		remap4_val = ((((des + 0x2000000*6) >> 24) | 0x1) & 0x3FF)
			+ ((((des + 0x2000000*7) >> 8) | 1<<16) & 0x3FF0000);

#ifdef ENABLE_MEM_REMAP_HW
		mt_reg_sync_writel(remap1_val, MD3_BANK0_MAP0);
		mt_reg_sync_writel(remap2_val, MD3_BANK0_MAP1);
		mt_reg_sync_writel(remap3_val, MD3_BANK0_MAP2);
		mt_reg_sync_writel(remap4_val, MD3_BANK0_MAP3);
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
	set_md_rom_rw_mem_remap(md, 0x00000000, md->mem_layout.md_region_phy, invalid);
	set_md_smem_remap(md, 0x40000000, md_bank4_base, invalid);
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
#else
	{
		int reset_bit = -1;
		unsigned int reg_value;

		switch (ccif_id) {
		case AP_MD1_CCIF:
			reset_bit = 9;
			break;
		case AP_MD3_CCIF:
			reset_bit = 10;
			break;
		case MD1_MD3_CCIF:
			reset_bit = 11;
			break;
		}

		if (reset_bit == -1)
			return;

		/* this reset bit will clear CCIF's busy/wch/irq, but not SRAM */

		reg_value = ccci_read32(infra_ao_base, 0x130);
		reg_value &= ~(1 << reset_bit);
		reg_value |= (1 << reset_bit);
		ccci_write32(infra_ao_base, 0x130, reg_value);

		reg_value = ccci_read32(infra_ao_base, 0x134);
		reg_value &= ~(1 << reset_bit);
		reg_value |= (1 << reset_bit);
		ccci_write32(infra_ao_base, 0x134, reg_value);
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
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-infracfg_ao");
	infra_ao_base = (unsigned long)of_iomap(node, 0);
	CCCI_INIT_LOG(-1, TAG, "infra_ao_base:0x%p\n", (void *)infra_ao_base);
	node = of_find_compatible_node(NULL, NULL, "mediatek,dbgapb");
	dbgapb_base = (unsigned long)of_iomap(node, 0);
	CCCI_INIT_LOG(-1, TAG, "dbgapb_base:%pa\n", &dbgapb_base);
#ifdef FEATURE_LOW_BATTERY_SUPPORT
	register_low_battery_notify(&ccci_md_low_battery_cb, LOW_BATTERY_PRIO_MD);
	register_battery_percent_notify(&ccci_md_battery_percent_cb, BATTERY_PERCENT_PRIO_MD);
#endif
	return 0;
}


#define MD_CACHELINE_ALIGN (32)

#define CTRL_PAGE_SIZE (576 + MD_CACHELINE_ALIGN)
#define MD_EX_PAGE_SIZE (20*1024 + MD_CACHELINE_ALIGN)
#define MD_EX_PAGE_NUM  (6)
#define AP_EX_PAGE_SIZE (1024 + MD_CACHELINE_ALIGN)
#define AP_EX_PAGE_NUM  (3)
#define MD_BUF1_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF1_PAGE_NUM  (32)
#define AP_BUF1_PAGE_SIZE (512 + MD_CACHELINE_ALIGN)
#define AP_BUF1_PAGE_NUM  (8)

#define MD_BUF2_0_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_1_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_2_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_3_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_4_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_5_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF2_6_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF3_0_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF3_1_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)
#define MD_BUF3_2_PAGE_SIZE (128*1024 + MD_CACHELINE_ALIGN)

#define MD_BUF2_0_PAGE_NUM (16)
#define MD_BUF2_1_PAGE_NUM (16)
#define MD_BUF2_2_PAGE_NUM (16)
#define MD_BUF2_3_PAGE_NUM (16)
#define MD_BUF2_4_PAGE_NUM (16)
#define MD_BUF2_5_PAGE_NUM (16)
#define MD_BUF2_6_PAGE_NUM (16)
#define MD_BUF3_0_PAGE_NUM (16)
#define MD_BUF3_1_PAGE_NUM (16)
#define MD_BUF3_2_PAGE_NUM (16)

#define MD_HW_PAGE_SIZE (320 + MD_CACHELINE_ALIGN)

#define MD_HW_0_PAGE_NUM (15)
#define MD_HW_1_PAGE_NUM (13)

#define MD_META_PAGE_SIZE (65*1024)
#define MD_META_PAGE_NUM (5)

#define AP_META_PAGE_SIZE (65*1024)
#define AP_META_PAGE_NUM (5)

struct ccci_ccb_config ccb_configs[] = {
#ifdef FEATURE_DHL_CCB_RAW_SUPPORT
	{SMEM_USER_CCB_DHL, P_CORE, CTRL_PAGE_SIZE, CTRL_PAGE_SIZE, CTRL_PAGE_SIZE*15, CTRL_PAGE_SIZE*15}, /* Ctrl */
	{SMEM_USER_CCB_DHL, P_CORE, MD_EX_PAGE_SIZE, AP_EX_PAGE_SIZE, MD_EX_PAGE_SIZE*MD_EX_PAGE_NUM,
								AP_EX_PAGE_SIZE*AP_EX_PAGE_NUM}, /* exception */
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF1_PAGE_SIZE, AP_BUF1_PAGE_SIZE, (MD_BUF1_PAGE_SIZE*MD_BUF1_PAGE_NUM),
								AP_BUF1_PAGE_SIZE*AP_BUF1_PAGE_NUM},/*  PS */
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_0_PAGE_SIZE, 128, MD_BUF2_0_PAGE_SIZE*MD_BUF2_0_PAGE_NUM, 128}, /* L1 */
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_1_PAGE_SIZE, 128, MD_BUF2_1_PAGE_SIZE*MD_BUF2_1_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_2_PAGE_SIZE, 128, MD_BUF2_2_PAGE_SIZE*MD_BUF2_2_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_3_PAGE_SIZE, 128, MD_BUF2_3_PAGE_SIZE*MD_BUF2_3_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_4_PAGE_SIZE, 128, MD_BUF2_4_PAGE_SIZE*MD_BUF2_4_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_5_PAGE_SIZE, 128, MD_BUF2_5_PAGE_SIZE*MD_BUF2_5_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF2_6_PAGE_SIZE, 128, MD_BUF2_6_PAGE_SIZE*MD_BUF2_6_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF3_0_PAGE_SIZE, 128, MD_BUF3_0_PAGE_SIZE*MD_BUF3_0_PAGE_NUM, 128}, /* L2 */
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF3_1_PAGE_SIZE, 128, MD_BUF3_1_PAGE_SIZE*MD_BUF3_1_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_BUF3_2_PAGE_SIZE, 128, MD_BUF3_2_PAGE_SIZE*MD_BUF3_2_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, MD_HW_PAGE_SIZE, 128, MD_HW_PAGE_SIZE*MD_HW_0_PAGE_NUM, 128}, /* HW logger  */
	{SMEM_USER_CCB_DHL, P_CORE, MD_HW_PAGE_SIZE, 128, MD_HW_PAGE_SIZE*MD_HW_1_PAGE_NUM, 128},
	{SMEM_USER_CCB_DHL, P_CORE, 128, 128, 128, 128}, /* MDM */
	{SMEM_USER_CCB_DHL, P_CORE, 128, 128, 128, 128},
	{SMEM_USER_CCB_DHL, P_CORE, 128, 128, 128, 128},
	{SMEM_USER_CCB_MD_MONITOR, P_CORE, 128, 128, 128, 128},
	{SMEM_USER_CCB_META, P_CORE, MD_META_PAGE_SIZE, AP_META_PAGE_SIZE, MD_META_PAGE_SIZE*MD_META_PAGE_NUM,
		AP_META_PAGE_SIZE*AP_META_PAGE_NUM},
#endif
};
unsigned int ccb_configs_len = sizeof(ccb_configs)/sizeof(struct ccci_ccb_config);


