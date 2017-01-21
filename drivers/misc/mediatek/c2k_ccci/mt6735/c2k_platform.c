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

#include <mt-plat/upmu_common.h>
#include <mt-plat/sync_write.h>

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
struct clk *clk_scp_sys_md2_main;
static atomic_t clock_on = ATOMIC_INIT(0);
#else
#include <mach/mt_clkmgr.h>
#endif
#if defined(CONFIG_MTK_LEGACY)
#include <cust_gpio_usage.h>
#endif
#include "c2k_platform.h"

static unsigned long infra_ao_base;	/*0x1000_0000 */
static unsigned long sleep_base;	/*0x1000_6000 */
static unsigned long toprgu_base;	/*0x1000_7000 */
static unsigned long c2k_chip_id_base;
static unsigned long md1_pccif_base;
static unsigned long md3_pccif_base;
static unsigned int c2k_wdt_irq_id;
/*static unsigned long c2k_iram_base;*/
static unsigned long c2k_iram_base_seg2[2] = { 0 };

#define ENABLE_C2K_JTAG 0

#define MD3_BANK0_MAP0 (0x310)
#define MD3_BANK0_MAP1 (0x314)
#define MD3_BANK4_MAP0 (0x318)
#define MD3_BANK4_MAP1 (0x31C)
#define INFRA_AO_C2K_CONFIG (0x330)
#define INFRA_AO_C2K_STATUS (0x334)
#define INFRA_AO_C2K_SPM_CTRL (0x338)
#define SLEEP_CLK_CON (0x400)
#define TOP_RGU_WDT_MODE (0x0)
#define TOP_RGU_WDT_SWRST (0x14)
#define TOP_RGU_WDT_SWSYSRST (0x18)
#define TOP_RGU_WDT_NONRST_REG (0x20)
#define PCCIF_BUSY (0x4)
#define PCCIF_TCHNUM (0xC)
#define PCCIF_ACK (0x14)
#define PCCIF_CHDATA (0x100)
#define PCCIF_SRAM_SIZE (512)

#define AP_PLATFORM_INFO    "MT6735E1"

#define ENABLE_C2K_EMI_PROTECTION	(1)
#if ENABLE_C2K_EMI_PROTECTION
#include <mach/emi_mpu.h>
#endif
/*===================================================*/
/*MPU Region defination*/
/*===================================================*/
#define MPU_REGION_ID_C2K_ROM       7
#define MPU_REGION_ID_C2K_RW        8
#define MPU_REGION_ID_C2K_SMEM      10

/**
*
*--C2K memory layout--
*
*--0x00AFFFFF  _ _ _ _ _ _ _ _
*             | share mem      |
*--0x00A00000 |_ _ _ _ _ _ _ _ |
*             | RAM top        |
*--0x00500000 |_ _ _ _ _ _ _ _ |
*             | ROM            |
*--0x00180000 |_ _ _ _ _ _ _ _ |
*             | RAM bottom     |
*--0x00000000 |_ _ _ _ _ _ _ _ |
*
**/

#define MD3_ROM_SIZE				(0x380000)
#define MD3_RAM_SIZE_BOTTOM			(0x180000)
#define MD3_RAM_SIZE_TOP			(0x500000)
#define MD3_SHARE_MEM_SIZE			(0x100000)

#define AP_READY_BIT				(0x1 << 1)
#define AP_WAKE_C2K_BIT				(0x1 << 0)

#define ETS_SEL_BIT					(0x1 << 13)

#define c2k_write32(b, a, v) mt_reg_sync_writel(v, (b)+(a))
#define c2k_write16(b, a, v) mt_reg_sync_writew(v, (b)+(a))
#define c2k_write8(b, a, v)  mt_reg_sync_writeb(v, (b)+(a))
#define c2k_read32(b, a)     ioread32((void __iomem *)((b)+(a)))
#define c2k_read16(b, a)     ioread16((void __iomem *)((b)+(a)))
#define c2k_read8(b, a)      ioread8((void __iomem *)((b)+(a)))

#ifdef CONFIG_OF_RESERVED_MEM
#define CCCI_MD3_MEM_RESERVED_KEY "mediatek,reserve-memory-ccci_md3"
/*C2K reserved memory: total 11M */
#define MD3_MEM_RESERVED_SIZE (MD3_ROM_SIZE + MD3_RAM_SIZE_BOTTOM + MD3_RAM_SIZE_TOP + MD3_SHARE_MEM_SIZE)
#define MD3_MEM_RAM_ROM		(MD3_ROM_SIZE + MD3_RAM_SIZE_BOTTOM + MD3_RAM_SIZE_TOP)

phys_addr_t md3_mem_base;
static unsigned long md3_mem_base_virt;
static unsigned int md3_share_mem_size;
#define C2K_IMG_DUMP_SIZE		(0x1000)
#define C2K_IMG_DUMP_OFFSET		(0x61000)
#define C2K_IRAM_DUMP_SIZE		(0x200)
static int first_init = 1;

int c2k_plat_log_level = LOG_ERR;

struct md3_log_memory_info {
	unsigned int magic1;
	unsigned int magic2;
	unsigned int mem_size;
	unsigned int mem_base;	/* reserved */
};

void c2k_platform_restore_first_init(void)
{
	first_init = 1;
}

int modem_sdio_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;
	MTK_MEMCFG_LOG_AND_PRINTK("%s,uname:%s,base:0x%llx,size:0x%x\n",
				  __func__, rmem->name,
				  (unsigned long long)rptr, rsize);

	if (strstr(CCCI_MD3_MEM_RESERVED_KEY, rmem->name)) {
		if (rsize < MD3_MEM_RAM_ROM) {
			MTK_MEMCFG_LOG_AND_PRINTK("%s: reserve size=0x%x != 0x%x\n",
						  __func__, rsize,
						  MD3_MEM_RESERVED_SIZE);
			return 0;
		}
		md3_share_mem_size = rsize - MD3_MEM_RAM_ROM;
	}
	md3_mem_base = rmem->base;
	return 0;
}

RESERVEDMEM_OF_DECLARE(ccci_reserve_mem_md3_init, CCCI_MD3_MEM_RESERVED_KEY,
		       modem_sdio_reserve_mem_of_init);
#endif

static void c2k_set_log_mem_info(unsigned long start_addr)
{
	struct md3_log_memory_info *plog_mem_info = (struct md3_log_memory_info *)start_addr;

	plog_mem_info->magic1 = 0x1A2F4D46;
	plog_mem_info->magic2 = 0xB33B50D5;
	plog_mem_info->mem_size = md3_share_mem_size;
	plog_mem_info->mem_base = 0x0;

}

static void c2k_hw_info_init(void)
{
#ifdef CONFIG_OF
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	infra_ao_base = (unsigned long)of_iomap(node, 0);

	node = of_find_compatible_node(NULL, NULL, "mediatek,SLEEP");
	sleep_base = (unsigned long)of_iomap(node, 0);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-rgu");
	toprgu_base = (unsigned long)of_iomap(node, 0);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mdc2k");
	c2k_chip_id_base = (unsigned long)of_iomap(node, 0);
	md1_pccif_base = (unsigned long)of_iomap(node, 1);
	md3_pccif_base = (unsigned long)of_iomap(node, 2);
	c2k_wdt_irq_id = irq_of_parse_and_map(node, 0);

#ifdef CONFIG_OF_RESERVED_MEM
	md3_mem_base_virt =
	    (unsigned long)ioremap_nocache(md3_mem_base + C2K_IMG_DUMP_OFFSET,
					   C2K_IMG_DUMP_SIZE);
#endif

	/*set c2k log memory info at the head of 4K*/
	c2k_set_log_mem_info(md3_mem_base_virt);
	C2K_BOOTUP_LOG("[C2K] log mem size = 0x%x\n", *((unsigned int *)(md3_mem_base_virt+8)));
	pr_debug
	    ("[C2K] infra_ao_base=0x%lx, sleep_base=0x%lx, toprgu_base=0x%lx",
	     infra_ao_base, sleep_base, toprgu_base);
	pr_debug
	    ("c2k_chip_id_base=0x%lx, md3_mem_base_virt=0x%lx, c2k_wdt_irq_id=%d\n",
	     c2k_chip_id_base, md3_mem_base_virt, c2k_wdt_irq_id);
#endif

	mt_set_gpio_mode(0x80000000 | GPIO198, GPIO_MODE_00);
	mt_set_gpio_mode(0x80000000 | GPIO199, GPIO_MODE_00);
	C2K_BOOTUP_LOG("[C2K] set gpio %u and next to mode 0\n", GPIO198);
}

void c2k_mem_dump(void *start_addr, int len)
{
	unsigned int *curr_p = (unsigned int *)start_addr;
	unsigned char *curr_ch_p;
	int _16_fix_num = len / 16;
	int tail_num = len % 16;
	char buf[16];
	int i, j;

	if (NULL == curr_p) {
		pr_debug("[C2K-DUMP]NULL point to dump!\n");
		return;
	}
	if (0 == len) {
		pr_debug("[C2K-DUMP]Not need to dump\n");
		return;
	}

	C2K_MEM_LOG_TAG("[C2K-DUMP]Base: 0x%lx\n", (unsigned long)start_addr);
	/*Fix section */
	for (i = 0; i < _16_fix_num; i++) {
		C2K_MEM_LOG("[C2K-DUMP]%03X: %08X %08X %08X %08X\n",
			 i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2),
			 *(curr_p + 3));
		curr_p += 4;
	}

	/*Tail section */
	if (tail_num > 0) {
		curr_ch_p = (unsigned char *)curr_p;
		for (j = 0; j < tail_num; j++) {
			buf[j] = *curr_ch_p;
			curr_ch_p++;
		}
		for (; j < 16; j++)
			buf[j] = 0;
		curr_p = (unsigned int *)buf;
		C2K_MEM_LOG("[C2K-DUMP]%03X: %08X %08X %08X %08X\n",
			 i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2),
			 *(curr_p + 3));
	}
}

void dump_c2k_iram(void)
{
	if (md3_mem_base_virt == 0) {
		pr_debug
		    ("[C2K] md3_mem_base_virt is 0, exit without dump...\n");
		return;
	}

	C2K_MEM_LOG_TAG("[C2K] Dump C2K MEM begin, size 4K\n");
	c2k_mem_dump((void *)md3_mem_base_virt, C2K_IMG_DUMP_SIZE);
	C2K_MEM_LOG_TAG("[C2K] Dump C2K MEM end\n");
}

void dump_c2k_iram_seg2(void)
{
	int i;

	for (i = 0;
	     i < sizeof(c2k_iram_base_seg2) / sizeof(c2k_iram_base_seg2[0]);
	     i++) {
		if (c2k_iram_base_seg2[i] == 0) {
			pr_debug("[C2K] c2k_iram_base_seg2[%d] is 0, exit...\n",
				 i);
			return;
		}
		C2K_MEM_LOG_TAG("[C2K] Dump C2K IRAM begin, start at %lx\n",
			 c2k_iram_base_seg2[i]);
		c2k_mem_dump((void *)c2k_iram_base_seg2[i], C2K_IRAM_DUMP_SIZE);
	}

	C2K_MEM_LOG_TAG("[C2K] Dump C2K MEM end\n");

}

void dump_c2k_bootup_status(void)
{
	static int init_done;
	static void __iomem *c2k_iram_base_virt1;
	static void __iomem *c2k_iram_base_virt2;
	static void __iomem *c2k_uart0;
	static void __iomem *bootst;
	static void __iomem *chipid;
	static void __iomem *c2ksys_uart0;
	int retry = 0;

	if (!init_done) {
		init_done = 1;
		c2k_iram_base_virt1 = ioremap_nocache(0x39000000, 64);
		c2k_iram_base_virt2 = ioremap_nocache(0x39017018, 8);
		c2k_uart0 = ioremap_nocache(0x10211370, 8);
		bootst = ioremap_nocache(0x3A00B018, 8);
		chipid = ioremap_nocache(0x3A00B01C, 8);
		c2ksys_uart0 = ioremap_nocache(0x3A012000, 0x40);
	}
	C2K_MEM_LOG_TAG("[C2K] ======== Dump C2K bootup status begin ========\n");
	C2K_MEM_LOG_TAG("[C2K] addr: 0x10211370 (UART GPIO)\n");
	c2k_mem_dump(c2k_uart0, 8);

	/*C2K_CONFIG[12:11] = 00*/
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)&(~(0x3<<11)));
	while (retry < 20) {
		retry++;
		C2K_MEM_LOG_TAG("[C2K] C2K_CONFIG = 0x%x, C2K_STATUS = 0x%x\n",
			c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG),
			c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));
		msleep(20);
	}

	/*C2K_CONFIG[12:11] = 01*/
	retry  = 0;
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)&(~(0x1<<12)));
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)|(0x1<<11));
	while (retry < 20) {
		retry++;
		C2K_MEM_LOG_TAG("[C2K] C2K_CONFIG = 0x%x, C2K_STATUS = 0x%x\n",
			c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG),
			c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));
		msleep(20);
	}

	/*C2K_CONFIG[12:11] = 10*/
	retry  = 0;
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)&(~(0x1<<11)));
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)|(0x1<<12));
	while (retry < 20) {
		retry++;
		C2K_MEM_LOG_TAG("[C2K] C2K_CONFIG = 0x%x, C2K_STATUS = 0x%x\n",
			c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG),
			c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));
		msleep(20);
	}

	/*C2K_CONFIG[12:11] = 11*/
	retry  = 0;
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG)|(0x3<<11));
	while (retry < 20) {
		retry++;
		C2K_MEM_LOG_TAG("[C2K] C2K_CONFIG = 0x%x, C2K_STATUS = 0x%x\n",
			c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG),
			c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));
		msleep(20);
	}
	C2K_MEM_LOG_TAG("[C2K] ======== Dump C2K bootup status end ========\n");
}

#if ENABLE_C2K_EMI_PROTECTION
void set_c2k_mpu(void)
{
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id,
	    shr_mem_mpu_attr;
	unsigned int rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_id,
	    rom_mem_mpu_attr;
	unsigned int rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_id,
	    rw_mem_mpu_attr;

	rom_mem_mpu_id = MPU_REGION_ID_C2K_ROM;
	rw_mem_mpu_id = MPU_REGION_ID_C2K_RW;
	shr_mem_mpu_id = MPU_REGION_ID_C2K_SMEM;

	rom_mem_mpu_attr =
	    SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN,
				 FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R);
	rw_mem_mpu_attr =
	    SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN,
				 FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R);
	shr_mem_mpu_attr =
	    SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN,
				 FORBIDDEN, FORBIDDEN, FORBIDDEN,
				 NO_PROTECTION);

	/*
	 *if set start=0x0, end=0x10000, the actural protected area will be 0x0-0x1FFFF,
	 *here we use 64KB align, MPU actually request 32KB align since MT6582, but this works...
	 *we assume emi_mpu_set_region_protection will round end address down to 64KB align.
	 */
	rw_mem_phy_start = (unsigned int)md3_mem_base;
	rw_mem_phy_end =
	    ((rw_mem_phy_start + MD3_RAM_SIZE_BOTTOM + MD3_ROM_SIZE +
	      MD3_RAM_SIZE_TOP + 0xFFFF) & (~0xFFFF)) - 0x1;

	rom_mem_phy_start =
	    (unsigned int)(rw_mem_phy_start + MD3_RAM_SIZE_BOTTOM);
	rom_mem_phy_end =
	    ((rom_mem_phy_start + MD3_ROM_SIZE + 0xFFFF) & (~0xFFFF)) - 0x1;

	shr_mem_phy_start = rw_mem_phy_end + 0x1;
	shr_mem_phy_end   = ((shr_mem_phy_start + md3_share_mem_size + 0xFFFF)&(~0xFFFF)) - 0x1;

	C2K_BOOTUP_LOG("[C2K] MPU Start protect MD R/W region<%d:%08x:%08x> %x\n",
		 rw_mem_mpu_id, rw_mem_phy_start, rw_mem_phy_end,
		 rw_mem_mpu_attr);
	emi_mpu_set_region_protection(rw_mem_phy_start,	/*START_ADDR */
				      rw_mem_phy_end,	/*END_ADDR */
				      rw_mem_mpu_id,	/*region */
				      rw_mem_mpu_attr);

	C2K_BOOTUP_LOG("[C2K] MPU Start protect MD ROM region<%d:%08x:%08x> %x\n",
		 rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end,
		 rom_mem_mpu_attr);
	emi_mpu_set_region_protection(rom_mem_phy_start,	/*START_ADDR */
				      rom_mem_phy_end,	/*END_ADDR */
				      rom_mem_mpu_id,	/*region */
				      rom_mem_mpu_attr);

	C2K_BOOTUP_LOG("[C2K] MPU Start protect MD Share region<%d:%08x:%08x> %x\n",
		 shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end,
		 shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,	/*START_ADDR */
				      shr_mem_phy_end,	/*END_ADDR */
				      shr_mem_mpu_id,	/*region */
				      shr_mem_mpu_attr);
}
#endif

static void md_gpio_get(GPIO_PIN pin, char *tag)
{
	pr_debug
	    ("GPIO%d(%s): mode=%d,dir=%d,in=%d,out=%d,pull_en=%d,pull_sel=%d,smt=%d\n",
	     pin, tag, mt_get_gpio_mode(pin), mt_get_gpio_dir(pin),
	     mt_get_gpio_in(pin), mt_get_gpio_out(pin),
	     mt_get_gpio_pull_enable(pin), mt_get_gpio_pull_select(pin),
	     mt_get_gpio_smt(pin));
}

static void md_gpio_set(GPIO_PIN pin, GPIO_MODE mode, GPIO_DIR dir,
			GPIO_OUT out, GPIO_PULL_EN pull_en, GPIO_PULL pull,
			GPIO_SMT smt)
{
	mt_set_gpio_mode(pin, mode);
	if (dir != GPIO_DIR_UNSUPPORTED)
		mt_set_gpio_dir(pin, dir);

	if (dir == GPIO_DIR_OUT)
		mt_set_gpio_out(pin, out);

	if (dir == GPIO_DIR_IN)
		mt_set_gpio_smt(pin, smt);
	if (pull_en != GPIO_PULL_EN_UNSUPPORTED) {
		mt_set_gpio_pull_enable(pin, pull_en);
		mt_set_gpio_pull_select(pin, pull);
	}
	md_gpio_get(pin, "-");
}

void enable_c2k_jtag(unsigned int mode)
{
	static void __iomem *c2k_jtag_setting;

	if (mode == 1) {
		md_gpio_set(0x80000000 | GPIO82, GPIO_MODE_05, GPIO_DIR_IN,
			    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE,
			    GPIO_PULL_DOWN, GPIO_SMT_ENABLE);
		md_gpio_set(0x80000000 | GPIO81, GPIO_MODE_05, GPIO_DIR_IN,
			    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE,
			    GPIO_PULL_UP, GPIO_SMT_ENABLE);
		md_gpio_set(0x80000000 | GPIO83, GPIO_MODE_05, GPIO_DIR_IN,
			    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE,
			    GPIO_PULL_UP, GPIO_SMT_DISABLE);
		md_gpio_set(0x80000000 | GPIO85, GPIO_MODE_05, GPIO_DIR_IN,
			    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE,
			    GPIO_PULL_UP, GPIO_SMT_DISABLE);
		md_gpio_set(0x80000000 | GPIO84, GPIO_MODE_05, GPIO_DIR_OUT,
			    GPIO_OUT_ZERO, GPIO_PULL_EN_UNSUPPORTED,
			    GPIO_PULL_UNSUPPORTED, GPIO_SMT_UNSUPPORTED);
		md_gpio_set(0x80000000 | GPIO86, GPIO_MODE_05, GPIO_DIR_OUT,
			    GPIO_OUT_ZERO, GPIO_PULL_EN_UNSUPPORTED,
			    GPIO_PULL_UNSUPPORTED, GPIO_SMT_UNSUPPORTED);
	} else if (mode == 2) {
		if (first_init) {
			first_init = 0;
			c2k_jtag_setting = ioremap_nocache(0x10000700, 0x04);
		}
		c2k_write32(c2k_jtag_setting, 0,
			    c2k_read32(c2k_jtag_setting, 0) | (0x1 << 2));
	}

}

void c2k_modem_power_on_platform(void)
{
	int ret = 0;

#if ENABLE_C2K_JTAG
/*ARM legacy JTAG for C2K*/
	md_gpio_set(0x80000000 | GPIO82, GPIO_MODE_05, GPIO_DIR_IN,
		    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE, GPIO_PULL_DOWN,
		    GPIO_SMT_ENABLE);
	md_gpio_set(0x80000000 | GPIO81, GPIO_MODE_05, GPIO_DIR_IN,
		    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE, GPIO_PULL_UP,
		    GPIO_SMT_ENABLE);
	md_gpio_set(0x80000000 | GPIO83, GPIO_MODE_05, GPIO_DIR_IN,
		    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE, GPIO_PULL_UP,
		    GPIO_SMT_DISABLE);
	md_gpio_set(0x80000000 | GPIO85, GPIO_MODE_05, GPIO_DIR_IN,
		    GPIO_OUT_UNSUPPORTED, GPIO_PULL_ENABLE, GPIO_PULL_UP,
		    GPIO_SMT_DISABLE);
	md_gpio_set(0x80000000 | GPIO84, GPIO_MODE_05, GPIO_DIR_OUT,
		    GPIO_OUT_ZERO, GPIO_PULL_EN_UNSUPPORTED,
		    GPIO_PULL_UNSUPPORTED, GPIO_SMT_UNSUPPORTED);
	md_gpio_set(0x80000000 | GPIO86, GPIO_MODE_05, GPIO_DIR_OUT,
		    GPIO_OUT_ZERO, GPIO_PULL_EN_UNSUPPORTED,
		    GPIO_PULL_UNSUPPORTED, GPIO_SMT_UNSUPPORTED);
#endif
	C2K_BOOTUP_LOG("[C2K] c2k_modem_power_on enter\n");
	if (infra_ao_base == 0)
		c2k_hw_info_init();
	C2K_BOOTUP_LOG("[C2K] prepare to power on c2k\n");
	/*step 0: power on MTCMOS */
#if defined(CONFIG_MTK_CLKMGR)
	ret = md_power_on(SYS_MD2);
#else
	if (atomic_inc_return(&clock_on) == 1)
		ret = clk_prepare_enable(clk_scp_sys_md2_main);
	else
		atomic_dec(&clock_on);
#endif
	C2K_BOOTUP_LOG("[C2K] md_power_on %d\n", ret);
	/*step 1: set C2K boot mode */
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG,
		    (c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG) &
		     (~(0x7 << 8))));
	/*clear c2k config before booting*/
	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG,
		    (c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG) &
		     (~(0x3 << 11))));
	C2K_BOOTUP_LOG("[C2K] C2K_CONFIG = 0x%x\n",
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG));
	/*step 2: config srcclkena selection mask */
	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base,
			       INFRA_AO_C2K_SPM_CTRL) & (~(0xF << 2)));
	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base,
			       INFRA_AO_C2K_SPM_CTRL) | (0x9 << 2));
	C2K_BOOTUP_LOG("[C2K] C2K_SPM_CTRL = 0x%x\n",
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL));
	c2k_write32(sleep_base, SLEEP_CLK_CON,
		    c2k_read32(sleep_base, SLEEP_CLK_CON) | 0xc);
	c2k_write32(sleep_base, SLEEP_CLK_CON,
		    c2k_read32(sleep_base, SLEEP_CLK_CON) & (~(0x1 << 14)));
	c2k_write32(sleep_base, SLEEP_CLK_CON,
		    c2k_read32(sleep_base, SLEEP_CLK_CON) | (0x1 << 12));
	c2k_write32(sleep_base, SLEEP_CLK_CON,
		    c2k_read32(sleep_base, SLEEP_CLK_CON) | (0x1 << 27));
	C2K_BOOTUP_LOG("[C2K] SLEEP_CLK_CON = 0x%x\n",
		 c2k_read32(sleep_base, SLEEP_CLK_CON));

	/*step 3: PMIC VTCXO_1 enable */
	pmic_config_interface(0x0A02, 0xA12E, 0xFFFF, 0x0);
	/*step 4: reset C2K */
#if 0
	c2k_write32(toprgu_base, TOP_RGU_WDT_SWSYSRST,
		    (c2k_read32(toprgu_base, TOP_RGU_WDT_SWSYSRST) | 0x88000000)
		    & (~(0x1 << 15)));
#else
	mtk_wdt_set_c2k_sysrst(1);
#endif
	C2K_BOOTUP_LOG("[C2K] TOP_RGU_WDT_SWSYSRST = 0x%x\n",
		 c2k_read32(toprgu_base, TOP_RGU_WDT_SWSYSRST));

	/*step 5: set memory remap */
	if (first_init) {
		first_init = 0;
		c2k_write32(infra_ao_base, MD3_BANK0_MAP0,
			    ((((unsigned int)md3_mem_base -
			       0x40000000) >> 24) | 0x1) & 0xFF);
#if ENABLE_C2K_EMI_PROTECTION
		set_c2k_mpu();
#endif
		C2K_BOOTUP_LOG("[C2K] MD3_BANK0_MAP0 = 0x%x\n",
			 c2k_read32(infra_ao_base, MD3_BANK0_MAP0));
	}

	/*step 6: wake up C2K */
	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL) | 0x1);
	while (!((c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS) >> 1) & 0x1))
		;
	C2K_BOOTUP_LOG("[C2K] C2K_STATUS = 0x%x\n",
			 c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));

	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL) & (~0x1));
	C2K_BOOTUP_LOG("[C2K] C2K_SPM_CTRL = 0x%x, C2K_STATUS = 0x%x\n",
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL),
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS));
#if 0
	while (c2k_read32(c2k_chip_id_base, 0) != 0x020AC000) {
		pr_debug("[C2K] C2K_CHIP_ID = 0x%x\n",
			 c2k_read32(c2k_chip_id_base, 0));
	}
	pr_debug("[C2K] C2K_CHIP_ID = 0x%x!!\n",
		 c2k_read32(c2k_chip_id_base, 0));
#endif
}

#define AP_CCIF_BASE	(0x10218000)

#define APCCIF_CON		(0x00)
#define APCCIF_BUSY		(0x04)
#define APCCIF_START	(0x08)
#define APCCIF_TCHNUM	(0x0C)
#define APCCIF_RCHNUM	(0x10)
#define APCCIF_ACK		(0x14)
#define APCCIF_CHDATA	(0x100)

static void __iomem *ap_ccif_base;
int ccif_notify_c2k(int ch_id)
{
	int busy = 0;

	if (first_init) {
		first_init = 0;
		ap_ccif_base = ioremap_nocache(AP_CCIF_BASE, 0x100);
	}
	busy = c2k_read32(ap_ccif_base, APCCIF_BUSY);

	if (busy & (1 << ch_id)) {
		pr_debug("[C2K] ccif busy now, ch%d\n", ch_id);
		return -1;
	}
	pr_debug("[C2K] ccif_notify_c2k, ch%d\n", ch_id);
	c2k_write32(ap_ccif_base, APCCIF_BUSY, 1 << ch_id);
	c2k_write32(ap_ccif_base, APCCIF_TCHNUM, ch_id);
	return 0;
}

int dump_ccif(void)
{
	pr_debug("[C2K] ccif APCCIF_BUSY(%d), APCCIF_START(%d)\n",
		 c2k_read32(ap_ccif_base, APCCIF_BUSY), c2k_read32(ap_ccif_base,
								   APCCIF_START));
	return 0;
}

void c2k_modem_power_off_platform(void)
{
	int ret = -123;

	C2K_BOOTUP_LOG("[C2K] md_power_off begin\n");
#if defined(CONFIG_MTK_CLKMGR)
	ret = md_power_off(SYS_MD2, 1000);
#else
	if (atomic_read(&clock_on)) {
		C2K_BOOTUP_LOG("[C2K] already power on, power down now\n");
		atomic_set(&clock_on, 0);
		clk_disable_unprepare(clk_scp_sys_md2_main);
	}
#endif
	C2K_BOOTUP_LOG("[C2K] md_power_off %d\n", ret);
}

void c2k_modem_reset_platform(void)
{
	int ret = 0;

	if (infra_ao_base == 0)
		c2k_hw_info_init();

	c2k_write32(toprgu_base, TOP_RGU_WDT_SWSYSRST,
		    (c2k_read32(toprgu_base, TOP_RGU_WDT_SWSYSRST) | 0x88000000)
		    | (0x1 << 15));

#if defined(CONFIG_MTK_CLKMGR)
	ret = md_power_on(SYS_MD2);
#else
	if (atomic_inc_return(&clock_on) == 1)
		ret = clk_prepare_enable(clk_scp_sys_md2_main);
	else
		atomic_dec(&clock_on);
#endif

	C2K_BOOTUP_LOG("[C2K] md_power_on %d\n", ret);

	/*step 1: reset C2K */
#if 0
	pr_debug("[C2K] set toprgu wdt");
	c2k_write32(toprgu_base, TOP_RGU_WDT_SWSYSRST,
		    (c2k_read32(toprgu_base, TOP_RGU_WDT_SWSYSRST) | 0x88000000)
		    & (~(0x1 << 15)));
#else
	mtk_wdt_set_c2k_sysrst(1);
#endif
	C2K_BOOTUP_LOG("[C2K] TOP_RGU_WDT_SWSYSRST = 0x%x\n",
		 c2k_read32(toprgu_base, TOP_RGU_WDT_SWSYSRST));

	/*step 2: wake up C2K */
	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL) | 0x1);
	while (!((c2k_read32(infra_ao_base, INFRA_AO_C2K_STATUS) >> 1) & 0x1))
		;
	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL,
		    c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL) & (~0x1));
	C2K_BOOTUP_LOG("[C2K] C2K_SPM_CTRL = 0x%x\n",
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL));

#if 0
	while (c2k_read32(c2k_chip_id_base, 0) != 0x020AC000) {
		pr_debug("[C2K] C2K_CHIP_ID = 0x%x\n",
			 c2k_read32(c2k_chip_id_base, 0));
	}
	C2K_BOOTUP_LOG("[C2K] C2K_CHIP_ID = 0x%x\n", c2k_read32(c2k_chip_id_base, 0));
#endif
}

void c2k_modem_reset_pccif(void)
{
	unsigned int tx_channel = 0;
	int i;
	/*clear occupied channel */
	while (tx_channel < 16) {
		if (c2k_read32(md1_pccif_base, PCCIF_BUSY) & (1 << tx_channel))
			c2k_write32(md1_pccif_base, PCCIF_TCHNUM, tx_channel);
		if (c2k_read32(md3_pccif_base, PCCIF_BUSY) & (1 << tx_channel))
			c2k_write32(md3_pccif_base, PCCIF_TCHNUM, tx_channel);
		tx_channel++;
	}
	/*clear un-ached channel */
	c2k_write32(md1_pccif_base, PCCIF_ACK,
		    c2k_read32(md3_pccif_base, PCCIF_BUSY));
	c2k_write32(md3_pccif_base, PCCIF_ACK,
		    c2k_read32(md1_pccif_base, PCCIF_BUSY));
	/*clear SRAM */
	for (i = 0; i < PCCIF_SRAM_SIZE / sizeof(unsigned int); i++) {
		c2k_write32(md1_pccif_base,
			    PCCIF_CHDATA + i * sizeof(unsigned int), 0);
		c2k_write32(md3_pccif_base,
			    PCCIF_CHDATA + i * sizeof(unsigned int), 0);
	}
	/*dump */
	/*
	pr_debug("[C2K] Dump MD1 PCCIF\n");
	ccci_mem_dump(-1, (void *)md1_pccif_base, 0x300);
	pr_debug("[C2K] Dump MD3 PCCIF\n");
	ccci_mem_dump(-1, (void *)md3_pccif_base, 0x300);
	*/
}

void set_ap_ready(int value)
{
	unsigned int reg_value;

	if (infra_ao_base == 0)
		c2k_hw_info_init();

	reg_value = c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL);

	if (value)
		reg_value |= AP_READY_BIT;
	else
		reg_value &= ~AP_READY_BIT;	/*set 0 to indicate ap ready */

	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL, reg_value);

	C2K_REPEAT_LOG("[C2K] set_ap_ready(%d)\n", value);
}

void set_ap_wake_cp(int value)
{
	unsigned int reg_value;

	if (infra_ao_base == 0)
		c2k_hw_info_init();

	reg_value = c2k_read32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL);

	if (value)
		reg_value |= AP_WAKE_C2K_BIT;
	else
		reg_value &= ~AP_WAKE_C2K_BIT;	/*set 0 to wake up cp */

	c2k_write32(infra_ao_base, INFRA_AO_C2K_SPM_CTRL, reg_value);

	C2K_REPEAT_LOG("[C2K] ap_wake_cp(%d)\n", value);
}

void set_ets_sel(int value)
{
	unsigned int reg_value;

	if (infra_ao_base == 0)
		c2k_hw_info_init();

	reg_value = c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG);

	if (value)
		reg_value |= ETS_SEL_BIT;
	else
		reg_value &= ~ETS_SEL_BIT;	/*set 0 to wake up cp */

	c2k_write32(infra_ao_base, INFRA_AO_C2K_CONFIG, reg_value);

	pr_debug("[C2K] set ETS SEL to %d (reg %d)\n", value,
		 c2k_read32(infra_ao_base, INFRA_AO_C2K_CONFIG));
}

unsigned int get_c2k_wdt_irq_id(void)
{
	if (c2k_wdt_irq_id == 0)
		c2k_hw_info_init();

	return c2k_wdt_irq_id;
}

unsigned int get_c2k_reserve_mem_size(void)
{
	return MD3_MEM_RAM_ROM + md3_share_mem_size;
}

char *get_ap_platform(void)
{
	return AP_PLATFORM_INFO;
}
