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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/mm.h>
/* #include <linux/xlog.h> */
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/uaccess.h>
#include <asm/div64.h>
#include <linux/miscdevice.h>
/* #include <mach/dma.h> */
#include <mt-plat/dma.h>
/* #include <mach/devs.h> */
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#else
#include <mach/mt_reg_base.h>
#endif
/* #include <mach/mt_typedefs.h> */
/* #include <mach/mt_clkmgr.h> */
/* #include <mach/mtk_nand.h> */
/* #include <mach/bmt.h> */
#include <mtk_nand.h>
#include <bmt.h>
#include "mtk_nand_util.h"
/* #include <mach/mt_irq.h> */
/* #include "partition.h" */
/* #include <asm/system.h> */
/* #include <mach/partition_define.h> */
#include <partition_define.h>
/* #include "../../../../../../source/kernel/drivers/aee/ipanic/ipanic.h" */
#include <linux/rtc.h>
/* #include <mach/mt_gpio.h> */
/* #include <mach/mt_pm_ldo.h> */
#ifdef CONFIG_PWR_LOSS_MTK_SPOH
#include <mach/power_loss_test.h>
#endif
/* #include <mach/nand_device_define.h> */
#include <nand_device_define.h>

#ifndef CONFIG_MTK_LEGACY
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#endif

#ifdef CONFIG_MNTL_SUPPORT
#include "mtk_nand_ops.h"
#endif
#include "mtk_nand_fs.h"


static const flashdev_info_t gen_FlashTable_p[] = {
	{{0x45, 0x4C, 0x98, 0xA3, 0x76, 0x00}, 5, 5, IO_8BIT/*IO_TOGGLESDR*/, 0x10F2000, 6144, 16384, 1952, 0x10401011,
	 0x33418010, 0x01010400, 80, VEND_SANDISK, 1024, "SDTNSIAMA016G ", MULTI_PLANE,
	 {PPTBL_NONE,
	  {0xEF, 0xEE, 0x5D, 46, 0x11, 0, 0, RTYPE_SANDISK_TLC_1ZNM, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_TLC, {FALSE, FALSE, TRUE, TRUE, 0xA2, 0xFF, TRUE, 68, 8, 0}, false,
	  {30, 500, 2, 5} },
	{{0x98, 0x3A, 0x98, 0xA3, 0x76, 0x00}, 5, 5, IO_8BIT, 0x10F2000, 6144, 16384, 1952, 0x10401011,
	 0xC03222, 0x101, 80, VEND_SANDISK, 1024, "TC58TEG7THLBA09 ", MULTI_PLANE,
	 {PPTBL_NONE,
	  {0xEF, 0xEE, 0x5D, 46, 0x11, 0, 0, RTYPE_SANDISK_TLC_1ZNM, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_TLC, {FALSE, FALSE, TRUE, TRUE, 0xA2, 0xFF, TRUE, 68, 8, 0}, false,
	  {30, 500, 2, 5} },
	{{0x45, 0xDE, 0x94, 0x93, 0x76, 0x51}, 6, 5, IO_8BIT/* IO_TOGGLESDR*/, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0x33418010, 0x01010400, 80, VEND_SANDISK, 1024, "SDTNSGAMA008G ", MULTI_PLANE,
	 {SANDISK_16K,
	  {0xEF, 0xEE, 0x5D, 32, 0x11, 0, 0xFFFFFFFD, RTYPE_SANDISK, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC_HYBER, {FALSE, FALSE, FALSE, FALSE, 0xA2, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x45, 0xDE, 0x94, 0x93, 0x76, 0x57}, 6, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_SANDISK, 1024, "SDTNQGAMA008G ", 0,
	 {SANDISK_16K,
	  {0xEF, 0xEE, 0xFF, 16, 0x11, 0, 1, RTYPE_SANDISK_19NM, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x98, 0xD7, 0x84, 0x93, 0x72, 0x00}, 5, 5, IO_8BIT, 0x400000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_TOSHIBA, 1024, "TC58TEG5DCKTA00", 0,
	 {SANDISK_16K, {0xEF, 0xEE, 0xFF, 7, 0xFF, 7, 0, RTYPE_TOSHIBA, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x45, 0xDE, 0x94, 0x93, 0x76, 0x50}, 6, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_SANDISK, 1024, "SDTNRGAMA008GK ", 0/*MULTI_PLANE*/,
	 {SANDISK_16K,
	  {0xEF, 0xEE, 0x5D, 36, 0x11, 0, 0xFFFFFFFF, RTYPE_SANDISK, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0xAD, 0xDE, 0x14, 0xA7, 0x42, 0x00}, 5, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_HYNIX, 1024, "H27UCG8T2ETR", 0,
	 {SANDISK_16K,
	  {0xFF, 0xFF, 0xFF, 7, 0xFF, 0, 1, RTYPE_HYNIX_16NM, {0XFF, 0xFF}, {0XFF, 0xFF} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x2C, 0x44, 0x44, 0x4B, 0xA9, 0x00}, 5, 5, IO_8BIT, 0x400000, 2048, 8192, 640, 0x10401011,
	 0xC03222, 0x101, 80, VEND_MICRON, 1024, "MT29F32G08CBADB ", 0,
	 {MICRON_8K, {0xEF, 0xEE, 0xFF, 7, 0x89, 0, 1, RTYPE_MICRON, {0x1, 0x14}, {0x1, 0x5} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0xAD, 0xDE, 0x94, 0xA7, 0x42, 0x00}, 5, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_BIWIN, 1024, "BW27UCG8T2ETR", 0,
	 {SANDISK_16K,
	  {0xFF, 0xFF, 0xFF, 7, 0xFF, 0, 1, RTYPE_HYNIX_16NM, {0XFF, 0xFF}, {0XFF, 0xFF} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x45, 0xD7, 0x84, 0x93, 0x72, 0x00}, 5, 5, IO_8BIT, 0x400000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_SANDISK, 1024, "SDTNRGAMA004GK ", 0,
	 {SANDISK_16K,
	  {0xEF, 0xEE, 0x5D, 36, 0x11, 0, 0xFFFFFFFF, RTYPE_SANDISK, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x2C, 0x64, 0x44, 0x4B, 0xA9, 0x00}, 5, 5, IO_8BIT, 0x800000, 2048, 8192, 640, 0x10401011,
	 0xC03222, 0x101, 80, VEND_MICRON, 1024, "MT29F64G08CBABA ", MULTI_PLANE,
	 {MICRON_8K, {0xEF, 0xEE, 0xFF, 7, 0x89, 0, 1, RTYPE_MICRON, {0x1, 0x14}, {0x1, 0x5} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC_HYBER, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0xAD, 0xD7, 0x94, 0x91, 0x60, 0x00}, 5, 5, IO_8BIT, 0x400000, 2048, 8192, 640, 0x10401011,
	 0xC03222, 0x101, 80, VEND_HYNIX, 1024, "H27UBG8T2CTR", 0,
	 {HYNIX_8K, {0xFF, 0xFF, 0xFF, 7, 0xFF, 0, 1, RTYPE_HYNIX, {0XFF, 0xFF}, {0XFF, 0xFF} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x98, 0xDE, 0x94, 0x93, 0x76, 0x50}, 6, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_TOSHIBA, 1024, "TC58TEG6DDKTA00", 0,
	 {SANDISK_16K, {0xEF, 0xEE, 0xFF, 7, 0xFF, 7, 0, RTYPE_TOSHIBA, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x98, 0xDE, 0x94, 0x93, 0x76, 0x51}, 6, 5, IO_8BIT, 0x800000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_TOSHIBA, 1024, "TC58TEG6DDLTA00", 0,
	 {SANDISK_16K, {0xEF, 0xEE, 0xFF, 7, 0xFF, 7, 0, RTYPE_TOSHIBA_15NM, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x98, 0x3A, 0x94, 0x93, 0x76, 0x51}, 6, 5, IO_8BIT, 0x1000000, 4096, 16384, 1280, 0x10401011,
	 0xC03222, 0x101, 80, VEND_TOSHIBA, 1024, "TC58TEG7DDLTA0D", 0,
	 {SANDISK_16K, {0xEF, 0xEE, 0xFF, 7, 0xFF, 7, 0, RTYPE_TOSHIBA_15NM, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC, {FALSE, FALSE, FALSE, FALSE, 0xFF, 0xFF, FALSE, 0xFF, 8, 0xFF}, false,
	  {20, 3000, 2, 5} },
	{{0x45, 0x3A, 0x94, 0x93, 0x76, 0x51}, 6, 5, IO_8BIT, 0x1000000, 8192, 32768, 2560, 0x10401011,
	 0xC03222, 0x101, 80, VEND_SANDISK, 1024, "SDTNSGAMA016G ", 0,
	 {SANDISK_16K,
	  {0xEF, 0xEE, 0x5D, 33, 0x11, 0, 0xFFFFFFFE, RTYPE_SANDISK, {0x80, 0x00}, {0x80, 0x01} },
	  {RAND_TYPE_SAMSUNG, {0x2D2D, 1, 1, 1, 1, 1} } },
	  NAND_FLASH_MLC_HYBER, {FALSE, FALSE, FALSE, FALSE, 0xA2, 0xFF, FALSE, 0xFF, 8, 0xFF}, true,
	  {20, 3000, 2, 5} },
};

static unsigned int flash_number = sizeof(gen_FlashTable_p) / sizeof(flashdev_info_t);
#define NFI_DEFAULT_CS				(0)

#define mtk_nand_assert(expr)  do { \
	if (unlikely(!(expr))) { \
		pr_crit("MTK nand assert failed in %s at %u (pid %d)\n", \
			   __func__, __LINE__, current->pid); \
		dump_stack();	\
	}	\
} while (0)

#ifndef CONFIG_MTK_LEGACY
static struct clk *nfi_hclk;
static struct clk *nfiecc_bclk;
static struct clk *nfi_bclk;
static struct clk *onfi_sel_clk;
static struct clk *onfi_26m_clk;
static struct clk *onfi_mode5;
static struct clk *onfi_mode4;
static struct clk *nfi_bclk_sel;
static struct clk *nfi_ahb_clk;
static struct clk *nfi_ecc_pclk;
static struct clk *nfi_pclk;
static struct clk *onfi_pad_clk;
/* for MT8167 */

static struct clk *nfi_2xclk;
static struct clk *nfi_rgecc;

static struct clk *nfi_1xclk_sel;
static struct clk *nfi_2xclk_sel;
static struct clk *nfiecc_sel;
static struct clk *nfiecc_csw_sel;
static struct clk *nfi_1xpad_clk;

static struct clk *main_d4;	/* main_d10 */
static struct clk *main_d5;	/* main_d12 */
static struct clk *main_d6;	/* main_d10 */
static struct clk *main_d7;	/* main_d12 */
static struct clk *main_d8;	/* main_d12 */
static struct clk *main_d10;	/* main_d10 */
static struct clk *main_d12;	/* main_d12 */

static struct regulator *mtk_nand_regulator;
#endif

#define VERSION	"v2.1 Fix AHB virt2phys error"
#define MODULE_NAME	"# MTK NAND #"
#define _MTK_NAND_DUMMY_DRIVER_
#define __INTERNAL_USE_AHB_MODE__	(1)
#define CFG_FPGA_PLATFORM (0)	/* for fpga by bean */
#define CFG_RANDOMIZER	  (1)	/* for randomizer code */
#define CFG_2CS_NAND	(1)	/* for 2CS nand */
#define CFG_COMBO_NAND	  (1)	/* for Combo nand */

#define NFI_TRICKY_CS  (1)	/* must be 1 or > 1? */

#define NFI_TIMEOUT_MS (1000)

#define MLC_MICRON_SLC_MODE	(0)

bool is_raw_part = FALSE;
/* #define MANUAL_CORRECT */

#if (defined(CONFIG_MTK_MLC_NAND_SUPPORT) || defined(CONFIG_MTK_TLC_NAND_SUPPORT))
bool MLC_DEVICE = TRUE;		/* to build pass xiaolei */
#endif

#ifdef CONFIG_OF
void __iomem *mtk_nfi_base;
void __iomem *mtk_nfiecc_base;
static struct device_node *mtk_nfiecc_node;
unsigned int nfi_irq;

void __iomem *mtk_gpio_base;
static struct device_node *mtk_gpio_node;


#ifdef CONFIG_MTK_LEGACY
void __iomem *mtk_efuse_base;
static struct device_node *mtk_efuse_node;
#define EFUSE_BASE	mtk_efuse_base

void __iomem *mtk_infra_base;
static struct device_node *mtk_infra_node;
#endif

/*
 * NFI controller version define
 *
 * 1: MT8127
 * 2: MT8163
 * Reserved.
 */
struct mtk_nfi_compatible {
	unsigned char chip_ver;
};

static const struct mtk_nfi_compatible mt8127_compat = {
	.chip_ver = 1,
};

static const struct mtk_nfi_compatible mt8163_compat = {
	.chip_ver = 2,
};

static const struct mtk_nfi_compatible mt8167_compat = {
	.chip_ver = 3,
};

static const struct of_device_id mtk_nfi_of_match[] = {
	{ .compatible = "mediatek,mt8127-nfi", .data = &mt8127_compat },
	{ .compatible = "mediatek,mt8163-nfi", .data = &mt8163_compat },
	{ .compatible = "mediatek,mt8167-nfi", .data = &mt8167_compat },
	{}
};

const struct mtk_nfi_compatible *mtk_nfi_dev_comp;
#endif

struct device *mtk_dev;
struct scatterlist mtk_sg;
enum dma_data_direction mtk_dir;

#ifndef CONFIG_MTK_FPGA
#if defined(CONFIG_MTK_LEGACY)
#define PERI_NFI_CLK_SOURCE_SEL ((volatile unsigned int *)(mtk_infra_base+0x098))
/* #define PERI_NFI_MAC_CTRL ((volatile unsigned int *)(PERICFG_BASE+0x428)) */
#define NFI_PAD_1X_CLOCK (0x1 << 10)	/* nfi1X */
#endif
#endif

bool tlc_lg_left_plane = TRUE;
enum NFI_TLC_PG_CYCLE tlc_program_cycle;
bool tlc_not_keep_erase_lvl = FALSE;
u32 slc_ratio = 6;
u32 sys_slc_ratio = 6;
u32 usr_slc_ratio = 6;
bool tlc_cache_program = FALSE;
bool tlc_snd_phyplane = FALSE;
static void mtk_nand_handle_write_ahb_done(void);

#if defined(NAND_OTP_SUPPORT)

#define SAMSUNG_OTP_SUPPORT		1
#define OTP_MAGIC_NUM			0x4E3AF28B
#define SAMSUNG_OTP_PAGE_NUM	6

static const unsigned int Samsung_OTP_Page[SAMSUNG_OTP_PAGE_NUM] = {
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1b
};

static struct mtk_otp_config g_mtk_otp_fuc;
static spinlock_t g_OTPLock;

#define OTP_MAGIC			'k'

/* NAND OTP IO control number */
#define OTP_GET_LENGTH		_IOW(OTP_MAGIC, 1, int)
#define OTP_READ			_IOW(OTP_MAGIC, 2, int)
#define OTP_WRITE			_IOW(OTP_MAGIC, 3, int)

#define FS_OTP_READ			0
#define FS_OTP_WRITE		1

/* NAND OTP Error codes */
#define OTP_SUCCESS					0
#define OTP_ERROR_OVERSCOPE			-1
#define OTP_ERROR_TIMEOUT			-2
#define OTP_ERROR_BUSY				-3
#define OTP_ERROR_NOMEM				-4
#define OTP_ERROR_RESET				-5

struct mtk_otp_config {
	u32 (*OTPRead)(u32 PageAddr, void *BufferPtr, void *SparePtr);
	u32 (*OTPWrite)(u32 PageAddr, void *BufferPtr, void *SparePtr);
	u32 (*OTPQueryLength)(u32 *Length);
};

struct otp_ctl {
	unsigned int QLength;
	unsigned int Offset;
	unsigned int Length;
	char *BufferPtr;
	unsigned int status;
};
#endif

#define NFI_SET_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) | (value));\
	DRV_WriteReg32(reg, g_value); \
} while (0)

#define NFI_SET_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) | (value));\
	DRV_WriteReg16(reg, g_value); \
} while (0)

#define NFI_CLN_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) & (~(value)));\
	DRV_WriteReg32(reg, g_value); \
} while (0)

#define NFI_CLN_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) & (~(value)));\
	DRV_WriteReg16(reg, g_value); \
} while (0)

#define NFI_WAIT_STATE_DONE(state) do {; } while (__raw_readl(NFI_STA_REG32) & state)
#define NFI_WAIT_TO_READY()  do {; } while (!(__raw_readl(NFI_STA_REG32) & STA_BUSY2READY))
#define FIFO_PIO_READY(x)  (0x1 & x)
#define WAIT_NFI_PIO_READY(timeout) \
do {\
	while ((!FIFO_PIO_READY(DRV_Reg(NFI_PIO_DIRDY_REG16))) && (--timeout)) \
		;\
} while (0)


#define NAND_SECTOR_SIZE (512)
#define OOB_PER_SECTOR		(16)
#define OOB_AVAI_PER_SECTOR (8)

#if defined(MTK_COMBO_NAND_SUPPORT)
	/* BMT_POOL_SIZE is not used anymore */
#else
#ifndef PART_SIZE_BMTPOOL
#define BMT_POOL_SIZE (80)
#else
#define BMT_POOL_SIZE (PART_SIZE_BMTPOOL)
#endif
#endif
u8 ecc_threshold;
#define PMT_POOL_SIZE	(2)
/*******************************************************************************
 * Gloable Varible Definition
 *******************************************************************************/
#if CFG_PERFLOG_DEBUG
struct nand_perf_log {
	unsigned int ReadPageCount;
	suseconds_t ReadPageTotalTime;
	unsigned int ReadBusyCount;
	suseconds_t ReadBusyTotalTime;
	unsigned int ReadDMACount;
	suseconds_t ReadDMATotalTime;

	unsigned int ReadSubPageCount;
	suseconds_t ReadSubPageTotalTime;

	unsigned int WritePageCount;
	suseconds_t WritePageTotalTime;
	unsigned int WriteBusyCount;
	suseconds_t WriteBusyTotalTime;
	unsigned int WriteDMACount;
	suseconds_t WriteDMATotalTime;

	unsigned int EraseBlockCount;
	suseconds_t EraseBlockTotalTime;

};
#endif
#ifdef PWR_LOSS_SPOH

#define PL_TIME_RAND_PROG(chip, page_addr, time) do { \
	if (host->pl.nand_program_wdt_enable == 1) { \
		PL_TIME_RAND(page_addr, time, host->pl.last_prog_time); } \
	else \
		time = 0; \
	} while (0)

#define PL_TIME_RAND_ERASE(chip, page_addr, time) do { \
	if (host->pl.nand_erase_wdt_enable == 1) { \
		PL_TIME_RAND(page_addr, time, host->pl.last_erase_time); \
	if (time != 0) \
		pr_err("[MVG_TEST]: Erase reset in %d us\n", time); } \
	else \
		time = 0; \
	} while (0)

#define PL_TIME_PROG(duration)	\
	host->pl.last_prog_time = duration

#define PL_TIME_ERASE(duration)	\
	host->pl.last_erase_time = duration

#define PL_TIME_PROG_WDT_SET(WDT)	\
	host->pl.nand_program_wdt_enable = WDT

#define PL_TIME_ERASE_WDT_SET(WDT)	\
	host->pl.nand_erase_wdt_enable = WDT

#define PL_NAND_BEGIN(time) PL_BEGIN(time)

#define PL_NAND_RESET(time) PL_RESET(time)

#define PL_NAND_END(pl_time_write, duration) PL_END(pl_time_write, duration)


#else

#define PL_TIME_RAND_PROG(chip, page_addr, time)
#define PL_TIME_RAND_ERASE(chip, page_addr, time)

#define PL_TIME_PROG(duration)
#define PL_TIME_ERASE(duration)

#define PL_TIME_PROG_WDT_SET(WDT)
#define PL_TIME_ERASE_WDT_SET(WDT)

#define PL_NAND_BEGIN(time)
#define PL_NAND_RESET(time)
#define PL_NAND_END(pl_time_write, duration)

#endif

#if CFG_PERFLOG_DEBUG
static struct nand_perf_log g_NandPerfLog = { 0 };
static struct timeval g_NandLogTimer = { 0 };
#endif

#ifdef NAND_PFM
static suseconds_t g_PFM_R;
static suseconds_t g_PFM_W;
static suseconds_t g_PFM_W_SLC;
static suseconds_t g_PFM_E;
static u32 g_PFM_RNum;
static u32 g_PFM_RD;
static u32 g_PFM_WD;
static u32 g_PFM_WD_SLC;
static struct timeval g_now;

#define PFM_BEGIN(time) do { \
	do_gettimeofday(&g_now); \
	(time) = g_now; \
} while (0)

#define PFM_END_R(time, n) do {\
	do_gettimeofday(&g_now); \
	g_PFM_R += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
	g_PFM_RNum += 1; \
	g_PFM_RD += n; \
	pr_err("%s - Read PFM: %lu, data: %d, ReadOOB: %d (%d, %d)\n", \
	MODULE_NAME, g_PFM_R, g_PFM_RD, g_kCMD.pureReadOOB, g_kCMD.pureReadOOBNum, g_PFM_RNum);\
} while (0)
#define PFM_END_W(time, n) do {\
	do_gettimeofday(&g_now); \
	g_PFM_W += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
	g_PFM_WD += n; \
	pr_err("%s - Write PFM: %lu, data: %d\n", MODULE_NAME, g_PFM_W, g_PFM_WD);\
} while (0)
#define PFM_END_W_SLC(time, n) do {\
	do_gettimeofday(&g_now); \
	g_PFM_W_SLC += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
	g_PFM_WD_SLC += n; \
	pr_err("%s - Write SLC PFM: %lu, data: %d\n", MODULE_NAME, g_PFM_W_SLC, g_PFM_WD_SLC);\
} while (0)

#define PFM_END_E(time) do {\
	do_gettimeofday(&g_now); \
	g_PFM_E += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
	pr_err("%s - Erase PFM: %lu\n", MODULE_NAME, g_PFM_E); \
} while (0)
#else
#define PFM_BEGIN(time)
#define PFM_END_R(time, n)
#define PFM_END_W(time, n)
#define PFM_END_W_SLC(time, n)
#define PFM_END_E(time)
#endif

#define TIMEOUT_1	0x1fff
#define TIMEOUT_2	0x8ff
#define TIMEOUT_3	0xffff
#define TIMEOUT_4	0xffff	/* 5000   //PIO */

#define NFI_ISSUE_COMMAND(cmd, col_addr, row_addr, col_num, row_num) \
	do { \
		DRV_WriteReg(NFI_CMD_REG16, cmd);\
		while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE)\
			;\
		DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);\
		DRV_WriteReg32(NFI_ROWADDR_REG32, row_addr);\
		DRV_WriteReg(NFI_ADDRNOB_REG16, col_num | (row_num<<ADDR_ROW_NOB_SHIFT))\
			;\
		while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE)\
			;\
	} while (0)

/* ------------------------------------------------------------------------------- */
static struct completion g_comp_AHB_Done;
static struct completion g_comp_WR_Done;
static struct completion g_comp_ER_Done;
static struct completion g_comp_Busy_Ret;

static struct NAND_CMD g_kCMD;
bool g_bInitDone;
int g_i4Interrupt;
static bool g_bcmdstatus;
/* static bool g_brandstatus; */
static u32 g_value;
static int g_page_size;
static int g_block_size;
static u32 PAGES_PER_BLOCK = 255;
static bool g_bSyncOrToggle;
/* #ifndef CONFIG_MTK_FPGA */
#if 0
#ifdef CONFIG_MTK_LEGACY
static int g_iNFI2X_CLKSRC = ARMPLL;
#else
static int g_iNFI2X_CLKSRC;
#endif
#endif
/* extern unsigned int flash_number; */
/* extern flashdev_info_t gen_FlashTable_p[MAX_FLASH]; */

#if CFG_2CS_NAND
bool g_b2Die_CS = FALSE;	/* for nand base */
static bool g_bTricky_CS = FALSE;
static u32 g_nanddie_pages;
#endif

static unsigned char g_bHwEcc;
#define LPAGE 32768
#define LSPARE 4096

static u8 *local_buffer_16_align;	/* 16 byte aligned buffer, for HW issue */
__aligned(64)
static u8 local_buffer[LPAGE + LSPARE];
static u8 *temp_buffer_16_align;	/* 16 byte aligned buffer, for HW issue */
__aligned(64)
static u8 temp_buffer[LPAGE + LSPARE];

__aligned(64)
static u8 local_tlc_wl_buffer[LPAGE + LSPARE];

/* static u8 *bean_buffer_16_align;   // 16 byte aligned buffer, for HW issue */
/* __attribute__((aligned(64))) static u8 bean_buffer[LPAGE + LSPARE]; */

#if CFG_2CS_NAND
static int mtk_nand_cs_check(struct mtd_info *mtd, u8 *id, u16 cs);
static u32 mtk_nand_cs_on(struct nand_chip *nand_chip, u16 cs, u32 page);
#endif

static bool mtk_nand_device_reset(void);
static bool mtk_nand_set_command(u16 command);

static bool ahb_read_sleep_enable(void);

static bool ahb_read_can_sleep(u32 length);

static void get_ahb_read_sleep_range(u32 length,
	int *min, int *max);

static int get_sector_ahb_read_time(void);

static void cal_sector_ahb_read_time(unsigned long long s,
	unsigned long long e, u32 length);

static bmt_struct *g_bmt;
struct mtk_nand_host *host;
static u8 g_running_dma;
#ifdef DUMP_NATIVE_BACKTRACE
static u32 g_dump_count;
#endif
/* extern struct mtd_partition g_pasStatic_Partition[];//to build pass xiaolei */
/* int part_num = PART_NUM;//to build pass xiaolei	NUM_PARTITIONS; */

int manu_id;
int dev_id;

static u8 local_oob_buf[LSPARE];

#ifdef _MTK_NAND_DUMMY_DRIVER_
int dummy_driver_debug;
#endif

flashdev_info_t devinfo;

enum NAND_TYPE_MASK {
	TYPE_ASYNC = 0x0,
	TYPE_TOGGLE = 0x1,
	TYPE_SYNC = 0x2,
	TYPE_RESERVED = 0x3,
	TYPE_MLC = 0x4,		/* 1b0 */
	TYPE_SLC = 0x4,		/* 1b1 */
};


typedef u32(*GetLowPageNumber) (u32 pageNo);
typedef u32(*TransferPageNumber) (u32 pageNo, bool high_to_low);

GetLowPageNumber functArray[] = {
	MICRON_TRANSFER,
	HYNIX_TRANSFER,
	SANDISK_TRANSFER,
};

TransferPageNumber fsFuncArray[] = {
	micron_pairpage_mapping,
	hynix_pairpage_mapping,
	sandisk_pairpage_mapping,
};

u32 SANDISK_TRANSFER(u32 pageNo)
{
	if (pageNo == 0)
		return pageNo;
	else
		return pageNo + pageNo - 1;
}

u32 HYNIX_TRANSFER(u32 pageNo)
{
	u32 temp;

	if (pageNo < 4)
		return pageNo;
	temp = pageNo + (pageNo & 0xFFFFFFFE) - 2;
	return temp;
}


u32 MICRON_TRANSFER(u32 pageNo)
{
	u32 temp;

	if (pageNo < 4)
		return pageNo;
	temp = (pageNo - 4) & 0xFFFFFFFE;
	if (pageNo <= 130)
		return (pageNo + temp);
	else
		return (pageNo + temp - 2);
}

u32 sandisk_pairpage_mapping(u32 page, bool high_to_low)
{
	if (high_to_low == TRUE) {
		if (page == 255)
			return page - 2;
		if ((page == 0) || (1 == (page % 2)))
			return page;
		if (page == 2)
			return 0;
		else
			return (page - 3);
	} else {
		if ((page != 0) && (0 == (page % 2)))
			return page;
		if (page == 255)
			return page;
		if (page == 0 || page == 253)
			return page + 2;
		else
			return page + 3;
	}
}

u32 hynix_pairpage_mapping(u32 page, bool high_to_low)
{
	u32 offset;

	if (high_to_low == TRUE) {
		/* Micron 256pages */
		if (page < 4)
			return page;

		offset = page % 4;
		if (offset == 2 || offset == 3)
			return page;

		if (page == 4 || page == 5 || page == 254 || page == 255)
			return page - 4;
		else
			return page - 6;
	} else {
		if (page > 251)
			return page;
		if (page == 0 || page == 1)
			return page + 4;
		offset = page % 4;
		if (offset == 0 || offset == 1)
			return page;
		else
			return page + 6;
	}
}

u32 micron_pairpage_mapping(u32 page, bool high_to_low)
{
	u32 offset;

	if (high_to_low == TRUE) {
		/* Micron 256pages */
		if ((page < 4) || (page > 251))
			return page;

		offset = page % 4;
		if (offset == 0 || offset == 1)
			return page;
		else
			return page - 6;
	} else {
		if ((page == 2) || (page == 3) || (page > 247))
			return page;
		offset = page % 4;
		if (offset == 0 || offset == 1)
			return page + 6;
		else
			return page;
	}
}

int mtk_nand_paired_page_transfer(u32 pageNo, bool high_to_low)
{
	if (devinfo.vendor != VEND_NONE)
		return fsFuncArray[devinfo.feature_set.ptbl_idx] (pageNo, high_to_low);
	else
		return 0xFFFFFFFF;
}

#ifdef CONFIG_MTK_FPGA
void nand_enable_clock(void)
{

}

void nand_disable_clock(void)
{

}

void nand_prepare_clock(void)
{

}

void nand_unprepare_clock(void)
{

}
#else
#define PWR_DOWN 0
#define PWR_ON	 1
void nand_prepare_clock(void)
{
	#if !defined(CONFIG_MTK_LEGACY)
	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	clk_prepare(nfi_hclk);
	clk_prepare(nfiecc_bclk);
	clk_prepare(nfi_bclk);
	if (mtk_nfi_dev_comp->chip_ver == 2) {
		clk_prepare(nfi_pclk);
		clk_prepare(nfi_ecc_pclk);
	}
	if (mtk_nfi_dev_comp->chip_ver == 3) {
		clk_prepare(nfi_rgecc);

		if (g_bSyncOrToggle == true)
			clk_prepare(nfi_2xclk);
	}
	clk_prepare(nfi_hclk);
	clk_prepare(nfiecc_bclk);
	clk_prepare(nfi_bclk);
	#endif
	#endif
}

void nand_unprepare_clock(void)
{
	#if !defined(CONFIG_MTK_LEGACY)
	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	clk_unprepare(nfi_hclk);
	clk_unprepare(nfiecc_bclk);
	clk_unprepare(nfi_bclk);
	if (mtk_nfi_dev_comp->chip_ver == 2) {
		clk_unprepare(nfi_pclk);
		clk_unprepare(nfi_ecc_pclk);
	}
	if (mtk_nfi_dev_comp->chip_ver == 3) {
		clk_unprepare(nfi_rgecc);

		if (g_bSyncOrToggle == true)
			clk_unprepare(nfi_2xclk);
	}
	#endif
	#endif
}

void nand_enable_clock(void)
{
#if defined(CONFIG_MTK_LEGACY)
	if (mtk_nfi_dev_comp->chip_ver == 1) {
		/* if(clock_is_on(MT_CG_PERI_NFI)==PWR_DOWN) */
		enable_clock(MT_CG_PERI_NFI, "NFI");
		/* if(clock_is_on(MT_CG_PERI_NFI_ECC)==PWR_DOWN) */
		enable_clock(MT_CG_PERI_NFI_ECC, "NFI");
		/* if(clock_is_on(MT_CG_PERI_NFIPAD)==PWR_DOWN) */
		enable_clock(MT_CG_PERI_NFIPAD, "NFI");
	} else if (mtk_nfi_dev_comp->chip_ver == 2) {
		/* if(clock_is_on(MT_CG_INFRA_NFI)==PWR_DOWN) */
		enable_clock(MT_CG_INFRA_NFI, "NFI");
		/* if(clock_is_on(MT_CG_INFRA_NFI_ECC)==PWR_DOWN) */
		enable_clock(MT_CG_INFRA_NFI_ECC, "NFI");
		/* if(clock_is_on(MT_CG_INFRA_NFI_BCLK)==PWR_DOWN) */
		enable_clock(MT_CG_INFRA_NFI_BCLK, "NFI");
	} else {
		nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
			mtk_nfi_dev_comp->chip_ver);
	}
#else
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (mtk_nfi_dev_comp->chip_ver == 3) {
		/* pr_debug("%s %d\n", __func__, __LINE__); */
		clk_enable(nfi_rgecc);

		if (g_bSyncOrToggle == true)
			clk_enable(nfi_2xclk);
	}
	clk_enable(nfi_hclk);
	clk_enable(nfiecc_bclk);
	clk_enable(nfi_bclk);
	if (mtk_nfi_dev_comp->chip_ver == 2) {
		clk_enable(nfi_pclk);
		clk_enable(nfi_ecc_pclk);
	}
#endif
#endif
}

void nand_disable_clock(void)
{
#if defined(CONFIG_MTK_LEGACY)
	if (mtk_nfi_dev_comp->chip_ver == 1) {
		/* if(clock_is_on(MT_CG_PERI_NFIPAD)==PWR_ON) */
		disable_clock(MT_CG_PERI_NFIPAD, "NFI");
		/* if(clock_is_on(MT_CG_PERI_NFI_ECC)==PWR_ON) */
		disable_clock(MT_CG_PERI_NFI_ECC, "NFI");
		/* if(clock_is_on(MT_CG_PERI_NFI)==PWR_ON) */
		disable_clock(MT_CG_PERI_NFI, "NFI");
	} else if (mtk_nfi_dev_comp->chip_ver == 2) {
		/* if(clock_is_on(MT_CG_INFRA_NFI_BCLK)==PWR_ON) */
		disable_clock(MT_CG_INFRA_NFI_BCLK, "NFI");
		/* if(clock_is_on(MT_CG_INFRA_NFI_ECC)==PWR_ON) */
		disable_clock(MT_CG_INFRA_NFI_ECC, "NFI");
		/* if(clock_is_on(MT_CG_INFRA_NFI)==PWR_ON) */
		disable_clock(MT_CG_INFRA_NFI, "NFI");
	} else {
		nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
			mtk_nfi_dev_comp->chip_ver);
	}
#else
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* pr_debug("%s %d\n", __func__, __LINE__); */
	if (mtk_nfi_dev_comp->chip_ver == 3) {
		clk_disable(nfi_rgecc);

		if (g_bSyncOrToggle == true)
			clk_disable(nfi_2xclk);
	}
	clk_disable(nfi_hclk);
	clk_disable(nfiecc_bclk);
	clk_disable(nfi_bclk);
	if (mtk_nfi_dev_comp->chip_ver == 2) {
		clk_disable(nfi_pclk);
		clk_disable(nfi_ecc_pclk);
	}
#endif
#endif
}
#endif

static struct nand_ecclayout nand_oob_16 = {
	.eccbytes = 8,
	.eccpos = {8, 9, 10, 11, 12, 13, 14, 15},
	.oobfree = {{1, 6}, {0, 0} }
};

struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 32,
	.eccpos = {32, 33, 34, 35, 36, 37, 38, 39,
		   40, 41, 42, 43, 44, 45, 46, 47,
		   48, 49, 50, 51, 52, 53, 54, 55,
		   56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 6}, {0, 0} }
};

struct nand_ecclayout nand_oob_128 = {
	.eccbytes = 64,
	.eccpos = {
		   64, 65, 66, 67, 68, 69, 70, 71,
		   72, 73, 74, 75, 76, 77, 78, 79,
		   80, 81, 82, 83, 84, 85, 86, 86,
		   88, 89, 90, 91, 92, 93, 94, 95,
		   96, 97, 98, 99, 100, 101, 102, 103,
		   104, 105, 106, 107, 108, 109, 110, 111,
		   112, 113, 114, 115, 116, 117, 118, 119,
		   120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 7}, {33, 7}, {41, 7}, {49, 7}, {57, 6} }
};

/**************************************************************************
*  Randomizer
**************************************************************************/
#define SS_SEED_NUM 128
#ifdef CONFIG_MTK_LEGACY
#define EFUSE_RANDOM_CFG	((volatile u32 *)(EFUSE_BASE + 0x01C0))
#endif

static bool use_randomizer = FALSE;
static bool pre_randomizer = FALSE;

static unsigned short SS_RANDOM_SEED[SS_SEED_NUM] = {
	/* for page 0~127 */
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

#if CFG_PERFLOG_DEBUG
static suseconds_t Cal_timediff(struct timeval *end_time, struct timeval *start_time)
{
	struct timeval difference;

	difference.tv_sec = end_time->tv_sec - start_time->tv_sec;
	difference.tv_usec = end_time->tv_usec - start_time->tv_usec;

	/* Using while instead of if below makes the code slightly more robust. */

	while (difference.tv_usec < 0) {
		difference.tv_usec += 1000000;
		difference.tv_sec -= 1;
	}

	return 1000000LL * difference.tv_sec + difference.tv_usec;

}				/* timeval_diff() */
#endif

#if CFG_PERFLOG_DEBUG

void dump_nand_rwcount(void)
{
	struct timeval now_time;

	do_gettimeofday(&now_time);
	if (Cal_timediff(&now_time, &g_NandLogTimer) > (500 * 1000)) {	/* Dump per 100ms */
		pr_debug(" RPageCnt: %d (%lu us) RSubCnt: %d (%lu us) WPageCnt: %d (%lu us) ECnt: %d mtd(0/512/1K/2K/3K/4K): %d %d %d %d %d %d\n ",
			g_NandPerfLog.ReadPageCount,
			g_NandPerfLog.ReadPageCount ? (g_NandPerfLog.ReadPageTotalTime /
						   g_NandPerfLog.ReadPageCount) : 0,
			g_NandPerfLog.ReadSubPageCount,
			g_NandPerfLog.ReadSubPageCount ? (g_NandPerfLog.ReadSubPageTotalTime /
							  g_NandPerfLog.ReadSubPageCount) : 0,
			g_NandPerfLog.WritePageCount,
			g_NandPerfLog.WritePageCount ? (g_NandPerfLog.WritePageTotalTime /
							g_NandPerfLog.WritePageCount) : 0,
			g_NandPerfLog.EraseBlockCount, g_MtdPerfLog.read_size_0_512,
			g_MtdPerfLog.read_size_512_1K, g_MtdPerfLog.read_size_1K_2K,
			g_MtdPerfLog.read_size_2K_3K, g_MtdPerfLog.read_size_3K_4K,
			g_MtdPerfLog.read_size_Above_4K);

		memset(&g_NandPerfLog, 0x00, sizeof(g_NandPerfLog));
		memset(&g_MtdPerfLog, 0x00, sizeof(g_MtdPerfLog));
		do_gettimeofday(&g_NandLogTimer);

	}
}
#endif
void dump_nfi(void)
{
#if __DEBUG_NAND
	pr_debug("~~~~Dump NFI Register in Kernel~~~~\n");
	pr_debug("NFI_CNFG_REG16: 0x%x\n", DRV_Reg16(NFI_CNFG_REG16));
	if (mtk_nfi_dev_comp->chip_ver == 1)
		pr_debug("NFI_PAGEFMT_REG16: 0x%x\n", DRV_Reg32(NFI_PAGEFMT_REG16));
	else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3))
		pr_debug("NFI_PAGEFMT_REG32: 0x%x\n", DRV_Reg32(NFI_PAGEFMT_REG32));
	pr_debug("NFI_CON_REG16: 0x%x\n", DRV_Reg16(NFI_CON_REG16));
	pr_debug("NFI_ACCCON_REG32: 0x%x\n", DRV_Reg32(NFI_ACCCON_REG32));
	pr_debug("NFI_INTR_EN_REG16: 0x%x\n", DRV_Reg16(NFI_INTR_EN_REG16));
	pr_debug("NFI_INTR_REG16: 0x%x\n", DRV_Reg16(NFI_INTR_REG16));
	pr_debug("NFI_CMD_REG16: 0x%x\n", DRV_Reg16(NFI_CMD_REG16));
	pr_debug("NFI_ADDRNOB_REG16: 0x%x\n", DRV_Reg16(NFI_ADDRNOB_REG16));
	pr_debug("NFI_COLADDR_REG32: 0x%x\n", DRV_Reg32(NFI_COLADDR_REG32));
	pr_debug("NFI_ROWADDR_REG32: 0x%x\n", DRV_Reg32(NFI_ROWADDR_REG32));
	pr_debug("NFI_STRDATA_REG16: 0x%x\n", DRV_Reg16(NFI_STRDATA_REG16));
	pr_debug("NFI_DATAW_REG32: 0x%x\n", DRV_Reg32(NFI_DATAW_REG32));
	pr_debug("NFI_DATAR_REG32: 0x%x\n", DRV_Reg32(NFI_DATAR_REG32));
	pr_debug("NFI_PIO_DIRDY_REG16: 0x%x\n", DRV_Reg16(NFI_PIO_DIRDY_REG16));
	pr_debug("NFI_STA_REG32: 0x%x\n", DRV_Reg32(NFI_STA_REG32));
	pr_debug("NFI_FIFOSTA_REG16: 0x%x\n", DRV_Reg16(NFI_FIFOSTA_REG16));
	/* pr_debug("NFI_LOCKSTA_REG16: 0x%x\n", DRV_Reg16(NFI_LOCKSTA_REG16)); */
	pr_debug("NFI_ADDRCNTR_REG16: 0x%x\n", DRV_Reg16(NFI_ADDRCNTR_REG16));
	pr_debug("NFI_STRADDR_REG32: 0x%x\n", DRV_Reg32(NFI_STRADDR_REG32));
	pr_debug("NFI_BYTELEN_REG16: 0x%x\n", DRV_Reg16(NFI_BYTELEN_REG16));
	pr_debug("NFI_CSEL_REG16: 0x%x\n", DRV_Reg16(NFI_CSEL_REG16));
	pr_debug("NFI_IOCON_REG16: 0x%x\n", DRV_Reg16(NFI_IOCON_REG16));
	pr_debug("NFI_FDM0L_REG32: 0x%x\n", DRV_Reg32(NFI_FDM0L_REG32));
	pr_debug("NFI_FDM0M_REG32: 0x%x\n", DRV_Reg32(NFI_FDM0M_REG32));
	pr_debug("NFI_LOCK_REG16: 0x%x\n", DRV_Reg16(NFI_LOCK_REG16));
	pr_debug("NFI_LOCKCON_REG32: 0x%x\n", DRV_Reg32(NFI_LOCKCON_REG32));
	pr_debug("NFI_LOCKANOB_REG16: 0x%x\n", DRV_Reg16(NFI_LOCKANOB_REG16));
	pr_debug("NFI_FIFODATA0_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA0_REG32));
	pr_debug("NFI_FIFODATA1_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA1_REG32));
	pr_debug("NFI_FIFODATA2_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA2_REG32));
	pr_debug("NFI_FIFODATA3_REG32: 0x%x\n", DRV_Reg32(NFI_FIFODATA3_REG32));
	pr_debug("NFI_MASTERSTA_REG16: 0x%x\n", DRV_Reg16(NFI_MASTERSTA_REG16));
	pr_debug("NFI_DEBUG_CON1_REG16: 0x%x\n", DRV_Reg16(NFI_DEBUG_CON1_REG16));
	pr_debug("ECC_ENCCON_REG16	  :%x\n", *ECC_ENCCON_REG16);
	pr_debug("ECC_ENCCNFG_REG32	:%x\n", *ECC_ENCCNFG_REG32);
	pr_debug("ECC_ENCDIADDR_REG32	:%x\n", *ECC_ENCDIADDR_REG32);
	pr_debug("ECC_ENCIDLE_REG32	:%x\n", *ECC_ENCIDLE_REG32);
	pr_debug("ECC_ENCPAR0_REG32	:%x\n", *ECC_ENCPAR0_REG32);
	pr_debug("ECC_ENCPAR1_REG32	:%x\n", *ECC_ENCPAR1_REG32);
	pr_debug("ECC_ENCPAR2_REG32	:%x\n", *ECC_ENCPAR2_REG32);
	pr_debug("ECC_ENCPAR3_REG32	:%x\n", *ECC_ENCPAR3_REG32);
	pr_debug("ECC_ENCPAR4_REG32	:%x\n", *ECC_ENCPAR4_REG32);
	pr_debug("ECC_ENCPAR5_REG32	:%x\n", *ECC_ENCPAR5_REG32);
	pr_debug("ECC_ENCPAR6_REG32	:%x\n", *ECC_ENCPAR6_REG32);
	pr_debug("ECC_ENCSTA_REG32	:%x\n", *ECC_ENCSTA_REG32);
	pr_debug("ECC_ENCIRQEN_REG16	:%x\n", *ECC_ENCIRQEN_REG16);
	pr_debug("ECC_ENCIRQSTA_REG16 :%x\n", *ECC_ENCIRQSTA_REG16);
	pr_debug("ECC_DECCON_REG16	:%x\n", *ECC_DECCON_REG16);
	pr_debug("ECC_DECCNFG_REG32	:%x\n", *ECC_DECCNFG_REG32);
	pr_debug("ECC_DECDIADDR_REG32 :%x\n", *ECC_DECDIADDR_REG32);
	pr_debug("ECC_DECIDLE_REG16	:%x\n", *ECC_DECIDLE_REG16);
	pr_debug("ECC_DECFER_REG16	:%x\n", *ECC_DECFER_REG16);
	pr_debug("ECC_DECENUM0_REG32	:%x\n", *ECC_DECENUM0_REG32);
	pr_debug("ECC_DECENUM1_REG32	:%x\n", *ECC_DECENUM1_REG32);
	pr_debug("ECC_DECDONE_REG16	:%x\n", *ECC_DECDONE_REG16);
	pr_debug("ECC_DECEL0_REG32	:%x\n", *ECC_DECEL0_REG32);
	pr_debug("ECC_DECEL1_REG32	:%x\n", *ECC_DECEL1_REG32);
	pr_debug("ECC_DECEL2_REG32	:%x\n", *ECC_DECEL2_REG32);
	pr_debug("ECC_DECEL3_REG32	:%x\n", *ECC_DECEL3_REG32);
	pr_debug("ECC_DECEL4_REG32	:%x\n", *ECC_DECEL4_REG32);
	pr_debug("ECC_DECEL5_REG32	:%x\n", *ECC_DECEL5_REG32);
	pr_debug("ECC_DECEL6_REG32	:%x\n", *ECC_DECEL6_REG32);
	pr_debug("ECC_DECEL7_REG32	:%x\n", *ECC_DECEL7_REG32);
	pr_debug("ECC_DECIRQEN_REG16	:%x\n", *ECC_DECIRQEN_REG16);
	pr_debug("ECC_DECIRQSTA_REG16 :%x\n", *ECC_DECIRQSTA_REG16);
	pr_debug("ECC_DECFSM_REG32	:%x\n", *ECC_DECFSM_REG32);
	pr_debug("ECC_BYPASS_REG32	:%x\n", *ECC_BYPASS_REG32);
#endif
}

u8 NFI_DMA_status(void)
{
	return g_running_dma;
}
EXPORT_SYMBOL(NFI_DMA_status);

u32 NFI_DMA_address(void)
{
	return DRV_Reg32(NFI_STRADDR_REG32);
}
EXPORT_SYMBOL(NFI_DMA_address);

unsigned long nand_virt_to_phys_add(unsigned long va)
{
	unsigned long pageOffset = (va & (PAGE_SIZE - 1));
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long pa;

	/* pr_debug("[xl] v2p va 0x%lx\n", va); */

	if (virt_addr_valid(va))
		return __virt_to_phys(va);

	if (current == NULL) {
		nand_pr_err("current is NULL!");
		return 0;
	}

	if (current->mm == NULL) {
		nand_pr_err("current->mm is NULL! tgid=0x%x, name=%s",
			   current->tgid, current->comm);
		return 0;
	}

	pgd = pgd_offset(current->mm, va);	/* what is tsk->mm */
	if (pgd_none(*pgd) || pgd_bad(*pgd)) {
		nand_pr_err("va=0x%lx, pgd invalid!", va);
		return 0;
	}

	pmd = pmd_offset((pud_t *) pgd, va);
	if (pmd_none(*pmd) || pmd_bad(*pmd)) {
		nand_pr_err("va=0x%lx, pmd invalid!", va);
		return 0;
	}

	pte = pte_offset_map(pmd, va);
	if (pte_present(*pte)) {
		pa = (pte_val(*pte) & (PAGE_MASK)) | pageOffset;
		return pa;
	}

	nand_pr_err("va=0x%lx, pte invalid!", va);
	return 0;
}
EXPORT_SYMBOL(nand_virt_to_phys_add);

bool get_device_info(u8 *id, flashdev_info_t *devinfo)
{
	u32 i, m, n, mismatch;
	int target = -1;
	u8 target_id_len = 0;

	for (i = 0; i < flash_number; i++) {
		mismatch = 0;
		for (m = 0; m < gen_FlashTable_p[i].id_length; m++) {
			if (id[m] != gen_FlashTable_p[i].id[m]) {
				mismatch = 1;
				break;
			}
		}
		if (mismatch == 0 && gen_FlashTable_p[i].id_length > target_id_len) {
			target = i;
			target_id_len = gen_FlashTable_p[i].id_length;
		}
	}

	if (target != -1) {
		pr_debug("Recognize NAND: ID [");
		for (n = 0; n < gen_FlashTable_p[target].id_length; n++) {
			devinfo->id[n] = gen_FlashTable_p[target].id[n];
			pr_debug("%x ", devinfo->id[n]);
		}
		pr_debug("], Device Name [%s], Page Size [%d]B Spare Size [%d]B Total Size [%d]MB\n",
			gen_FlashTable_p[target].devciename, gen_FlashTable_p[target].pagesize,
			gen_FlashTable_p[target].sparesize, gen_FlashTable_p[target].totalsize);
		devinfo->id_length = gen_FlashTable_p[target].id_length;
		devinfo->blocksize = gen_FlashTable_p[target].blocksize;
		devinfo->addr_cycle = gen_FlashTable_p[target].addr_cycle;
		devinfo->iowidth = gen_FlashTable_p[target].iowidth;
		devinfo->timmingsetting = gen_FlashTable_p[target].timmingsetting;
		/* Modify MT8127 timing setting */
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			if (devinfo->timmingsetting == 0x10401011)
				devinfo->timmingsetting = 0x10804222;
		}
		devinfo->advancedmode = gen_FlashTable_p[target].advancedmode;
		devinfo->pagesize = gen_FlashTable_p[target].pagesize;
		devinfo->sparesize = gen_FlashTable_p[target].sparesize;
		devinfo->totalsize = gen_FlashTable_p[target].totalsize;
		devinfo->sectorsize = gen_FlashTable_p[target].sectorsize;
		devinfo->s_acccon = gen_FlashTable_p[target].s_acccon;
		devinfo->s_acccon1 = gen_FlashTable_p[target].s_acccon1;
		devinfo->freq = gen_FlashTable_p[target].freq;
		devinfo->vendor = gen_FlashTable_p[target].vendor;
		/* devinfo->ttarget = gen_FlashTable[target].ttarget; */
		memcpy((u8 *) &devinfo->feature_set, (u8 *) &gen_FlashTable_p[target].feature_set,
			   sizeof(struct MLC_feature_set));
		memcpy(devinfo->devciename, gen_FlashTable_p[target].devciename,
			   sizeof(devinfo->devciename));
		devinfo->NAND_FLASH_TYPE = gen_FlashTable_p[target].NAND_FLASH_TYPE;
		memcpy((u8 *)&devinfo->tlcControl,
			(u8 *)&gen_FlashTable_p[target].tlcControl, sizeof(struct NFI_TLC_CTRL));
		devinfo->two_phyplane = gen_FlashTable_p[target].two_phyplane;
		return true;
	}
	nand_pr_err("Not Found NAND");
	pr_err("ID [");
	for (n = 0; n < NAND_MAX_ID; n++)
		pr_err("%x ", id[n]);
	pr_err("]\n");
	return false;
}

#ifdef DUMP_NATIVE_BACKTRACE
#define NFI_NATIVE_LOG_SD	 "/sdcard/NFI_native_log_%s-%02d-%02d-%02d_%02d-%02d-%02d.log"
#define NFI_NATIVE_LOG_DATA "/data/NFI_native_log_%s-%02d-%02d-%02d_%02d-%02d-%02d.log"
static int nfi_flush_log(char *s)
{
	mm_segment_t old_fs;
	struct rtc_time tm;
	struct timeval tv = { 0 };
	struct file *filp = NULL;
	char name[256];
	unsigned int re = 0;
	int data_write = 0;

	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec, &tm);
	memset(name, 0, sizeof(name));
	sprintf(name, NFI_NATIVE_LOG_DATA, s, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(name, O_WRONLY | O_CREAT, 0777);
	if (IS_ERR(filp)) {
		nand_pr_err("error create file in %s, IS_ERR:%ld, PTR_ERR:%ld", name,
			   IS_ERR(filp), PTR_ERR(filp));
		memset(name, 0, sizeof(name));
		sprintf(name, NFI_NATIVE_LOG_SD, s, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
		filp = filp_open(name, O_WRONLY | O_CREAT, 0777);
		if (IS_ERR(filp)) {
			nand_pr_err("error create file in %s, IS_ERR:%ld, PTR_ERR:%ld",
				   name, IS_ERR(filp), PTR_ERR(filp));
			set_fs(old_fs);
			return -1;
		}
	}
	pr_debug("[NFI_flush_log]log file:%s\n", name);
	set_fs(old_fs);

	if (!(filp->f_op) || !(filp->f_op->write)) {
		pr_debug("[NFI_flush_log] No operation\n");
		re = -1;
		goto ClOSE_FILE;
	}

	DumpNativeInfo();
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	data_write = vfs_write(filp, (char __user *)NativeInfo, strlen(NativeInfo), &filp->f_pos);
	if (!data_write) {
		nand_pr_err("write fail");
		re = -1;
	}
	set_fs(old_fs);

ClOSE_FILE:
	if (filp) {
		filp_close(filp, current->files);
		filp = NULL;
	}
	return re;
}
#endif

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)

void NFI_TLC_GetMappedWL(u32 pageidx, struct NFI_TLC_WL_INFO *WL_Info)
{
	WL_Info->word_line_idx = pageidx / 3;
	WL_Info->wl_pre = (enum NFI_TLC_WL_PRE)(pageidx % 3);
}

u32 NFI_TLC_GetRowAddr(u32 rowaddr)
{
	u32 real_row;
	u32 temp = 0xFF;
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	if (devinfo.tlcControl.normaltlc)
		temp = page_per_block / 3;
	else
		temp = page_per_block;

	real_row = ((rowaddr / temp) << devinfo.tlcControl.block_bit) | (rowaddr % temp);

	return real_row;
}

u32 NFI_TLC_SetpPlaneAddr(u32 rowaddr, bool left_plane)
{
	u32 real_row;

	if (devinfo.tlcControl.pPlaneEn) {
		if (left_plane)
			real_row = (rowaddr & (~(1 << devinfo.tlcControl.pPlane_bit)));
		else
			real_row = (rowaddr | (1 << devinfo.tlcControl.pPlane_bit));
	} else
			real_row = rowaddr;
	return real_row;
}

u32 NFI_TLC_GetMappedPgAddr(u32 rowaddr)
{
	u32 page_idx;
	u32 page_shift = 0;
	u32 real_row;
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	real_row = rowaddr;
	if (devinfo.tlcControl.normaltlc) {
		page_shift = devinfo.tlcControl.block_bit;
		if (devinfo.tlcControl.pPlaneEn)
			real_row &= (~(1 << devinfo.tlcControl.pPlane_bit));

		page_idx =
			((real_row >> page_shift) * page_per_block)
			+ (((real_row << (32-page_shift)) >> (32-page_shift)) * 3);
	} else {
		page_shift = devinfo.tlcControl.block_bit;
		page_idx = ((real_row >> page_shift) * page_per_block)
		+ ((real_row << (32-page_shift)) >> (32-page_shift));
	}

	return page_idx;
}
#endif

/* extern bool MLC_DEVICE; */
static bool mtk_nand_reset(void);

u32 mtk_nand_page_transform(struct mtd_info *mtd, struct nand_chip *chip, u32 page, u32 *blk,
				u32 *map_blk)
{
	u32 block_size = (devinfo.blocksize * 1024);
	u32 page_size = (1 << chip->page_shift);
	loff_t start_address = 0;
	u32 idx;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	bool raw_part = FALSE;
	loff_t logical_address = (loff_t)page * (1 << chip->page_shift);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	loff_t temp, temp1;
#endif
	/* MSG(INIT , "[CCCCCHHHHHIIIIEEENNNWWWEEEIII]%d, init_pmt_done %d\n", page, init_pmt_done); */
	devinfo.tlcControl.slcopmodeEn = FALSE;
	if (MLC_DEVICE && init_pmt_done == TRUE) {
		start_address = part_get_startaddress(logical_address, &idx);
		if (raw_partition(idx))
			raw_part = TRUE;
		else
			raw_part = FALSE;
	}

	is_raw_part = raw_part;
	if (init_pmt_done != TRUE  && devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		devinfo.tlcControl.slcopmodeEn = TRUE;

	if (raw_part == TRUE) {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
			if (devinfo.tlcControl.normaltlc) {
				temp = start_address;
				temp1 = logical_address - start_address;
				do_div(temp, (block_size & 0xFFFFFFFF));
				do_div(temp1, ((block_size / 3) & 0xFFFFFFFF));
				block = (u32)((u32)temp + (u32)temp1);
				page_in_block = ((u32)((logical_address - start_address) >> chip->page_shift)
					% ((mtd->erasesize / page_size) / 3));
				page_in_block *= 3;
				devinfo.tlcControl.slcopmodeEn = TRUE;
			} else {
				temp = start_address;
				temp1 = logical_address - start_address;
				do_div(temp, (block_size & 0xFFFFFFFF));
				do_div(temp1, ((block_size / 3) & 0xFFFFFFFF));
				block = (u32)((u32)temp + (u32)temp1);
				page_in_block = ((u32)((logical_address - start_address) >> chip->page_shift)
					 % ((mtd->erasesize / page_size) / 3));

				if (devinfo.vendor != VEND_NONE)
					page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
	}
		} else
#endif
	{
		block = (u32)((u32)(start_address >> chip->phys_erase_shift)
				+ (u32)((logical_address-start_address) >> (chip->phys_erase_shift - 1)));
		page_in_block =
			((u32)((logical_address - start_address) >> chip->page_shift)
			% ((mtd->erasesize / page_size) / 2));

		if (devinfo.vendor != VEND_NONE)
			page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
	}

		mapped_block = get_mapping_block_index(block);

		/*MSG(INIT , "[page_in_block]mapped_block = %d, page_in_block = %d\n", mapped_block, page_in_block); */
		*blk = block;
		*map_blk = mapped_block;
	} else {
		if (((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
			|| (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
			&& (!mtk_block_istlc(logical_address))) {
			mtk_slc_blk_addr(logical_address, &block, &page_in_block);
			devinfo.tlcControl.slcopmodeEn = TRUE;
		} else {
			block = page / (block_size / page_size);
			page_in_block = page % (block_size / page_size);
		}
		mapped_block = get_mapping_block_index(block);
		*blk = block;
		*map_blk = mapped_block;
	}
	return page_in_block;
}

bool mtk_nand_IsRawPartition(loff_t logical_address)
{
	u32 idx;

	part_get_startaddress(logical_address, &idx);
	if (raw_partition(idx))
		return true;
	else
		return false;
}

int mtk_nand_interface_async(void)
{
	int retry = 10;
	u32 val = 0;
	struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);

	if (g_bSyncOrToggle == TRUE) {

		mtk_nand_GetFeature(NULL, feature_set->gfeatureCmd,
				feature_set->Interface.address, (u8 *) &val, 4);

		if (val != feature_set->Interface.feature) {
			pr_info("[%s] %d check device interface value %d (0x%x)\n",
			    __func__, __LINE__, val, DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32));
		}

		mtk_nand_SetFeature(NULL, (u16) feature_set->sfeatureCmd,
				feature_set->Interface.address,
				(u8 *) &feature_set->Async_timing.feature,
				sizeof(feature_set->Interface.feature));

		while ((DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32) != 4) && retry--)
			DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 0);

		/* nand_disable_clock(); */

		clk_disable_unprepare(nfi_2xclk_sel);
		clk_set_parent(nfi_2xclk_sel, onfi_26m_clk);
		clk_prepare_enable(nfi_2xclk_sel);

		clk_disable_unprepare(nfi_1xclk_sel);
		clk_set_parent(nfi_1xclk_sel, nfi_ahb_clk);
		clk_prepare_enable(nfi_1xclk_sel);

		/* nand_enable_clock(); */
		NFI_SET_REG32(NFI_DEBUG_CON1_REG16, NFI_BYPASS);

		NFI_SET_REG32(ECC_BYPASS_REG32, ECC_BYPASS);
		DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);

		while (0 == (DRV_Reg32(NFI_STA_REG32) && STA_FLASH_MACRO_IDLE))
			;
		g_bSyncOrToggle = FALSE;
	}

	return 0;
}

static int mtk_nand_interface_config(struct mtd_info *mtd)
{
	u32 timeout;
	u32 val;
	int retry = 10;
	int sretry = 10;
	struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);

	pr_debug("[%s] Start to change DDR interface\n", __func__);
	mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd,
						feature_set->Interface.address, (u8 *) &val, 4);

	if ((val != feature_set->Interface.feature) &&
		(val != feature_set->Async_timing.feature)) {
		pr_info("[%s] Change mode to toggle first\n", __func__);
		pr_info("value %d, regs 0x%x, 0x%x, 0x%x\n", val,
			DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32),
			DRV_Reg32(NFI_ACCCON1_REG32),
			DRV_Reg32(NFI_ACCCON_REG32));
		DRV_WriteReg32(NFI_DLYCTRL_REG32, 0xA001);

		DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF00), 0x3);
		DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF10), 0x10);

		DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 1);
		DRV_WriteReg32(NFI_ACCCON1_REG32, devinfo.s_acccon1);
		DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.s_acccon);
	}

	mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd,
						feature_set->Interface.address, (u8 *) &val, 4);
	if ((val != feature_set->Interface.feature) &&
		(val != feature_set->Async_timing.feature)) {
		pr_info("[%s] LINE %d - value %d (0x%x)\n", __func__, __LINE__,
			val, DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32));
	}

	if (devinfo.iowidth == IO_ONFI || devinfo.iowidth == IO_TOGGLEDDR
		|| devinfo.iowidth == IO_TOGGLESDR) {
		do {
/*reset*/
			mtk_nand_device_reset();

			mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd,
								feature_set->Interface.address, (u8 *) &val, 4);
			if ((val != feature_set->Interface.feature) &&
				(val != feature_set->Async_timing.feature)) {
				pr_info("[%s] LINE %d - value %d (0x%x)\n", __func__, __LINE__,
					val, DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32));
			}
/*set feature*/
			mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd,
						feature_set->Interface.address,
						(u8 *) &feature_set->Interface.feature,
						sizeof(feature_set->Interface.feature));
			/* Set DDR flag here for clk */
			g_bSyncOrToggle = TRUE;

			NFI_CLN_REG32(NFI_DEBUG_CON1_REG16, HWDCM_SWCON_ON);
/*setup register*/
			NFI_CLN_REG32(NFI_DEBUG_CON1_REG16, NFI_BYPASS);

			/*clear bypass of ecc */
			NFI_CLN_REG32(ECC_BYPASS_REG32, ECC_BYPASS);

			nand_disable_clock();
			nand_unprepare_clock();
			g_bSyncOrToggle = TRUE;

			/* Keep Switch 2xCLK to MAINPLL_D12 or MAINPLL_D10 */
			/* Disable AHB clk and enable 2x clk */
			/* clk_set_parent(onfi_sel_clk, onfi_mode4); */
			clk_prepare_enable(nfi_2xclk_sel);
			clk_set_parent(nfi_2xclk_sel, main_d10);
			/* clk_set_parent(nfi_2xclk_sel, onfi_26m_clk); */
			clk_disable_unprepare(nfi_2xclk_sel);

			/* Disable AHB clk and enable 2x clk */
			clk_prepare_enable(nfi_1xclk_sel);
			clk_set_parent(nfi_1xclk_sel, nfi_1xpad_clk);
			clk_disable_unprepare(nfi_1xclk_sel);

			nand_prepare_clock();
			nand_enable_clock();

			while (0 == (DRV_Reg32(NFI_STA_REG32) && STA_FLASH_MACRO_IDLE))
				;

			if (devinfo.iowidth == IO_ONFI) {
				while ((DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32) != 2) && retry--)
					DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 2);
			} else {
				while ((DRV_Reg16(NFI_NAND_TYPE_CNFG_REG32) != 1) && retry--)
					DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 1);
			}

			/* For DQSCTRL setting */
			DRV_WriteReg32(NFI_DLYCTRL_REG32, 0xA001);
#if 0
			/* For 26MHz DQS delay */
			DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF00),
					0x3 | (0x1D << 4) | (0x6<<9));
			DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF10), 0x10);
#endif
			/* For 150MHz DQS delay */
			DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF00), 0x3);
			DRV_WriteReg16((unsigned short *)(GPIO_BASE + 0xF10), 0x10);

			pr_debug("[Timing] main_d10 0x%x 0x%x\n", devinfo.s_acccon, devinfo.s_acccon1);
			DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.s_acccon);
			DRV_WriteReg32(NFI_ACCCON1_REG32, devinfo.s_acccon1);

			mtk_nand_reset();

			mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd,
						feature_set->Interface.address, (u8 *) &val, 4);


		} while (((val & 0xFF) != (feature_set->Interface.feature & 0xFF)) && sretry--);

		if ((val & 0xFF) != (feature_set->Interface.feature & 0xFF)) {
			pr_info("[%s] fail %d\n", __func__, val);
			mtk_nand_reset();
			DRV_WriteReg16(NFI_CNFG_REG16, CNFG_OP_RESET);
			mtk_nand_set_command(NAND_CMD_RESET);
			timeout = TIMEOUT_4;
			while (timeout)
				timeout--;
			mtk_nand_reset();
			DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 0);

			nand_disable_clock();

			/* Disable 2x clk and enable AHB clk */
			clk_prepare_enable(nfi_1xclk_sel);
			clk_set_parent(nfi_1xclk_sel, nfi_ahb_clk);
			clk_disable_unprepare(nfi_1xclk_sel);

			nand_enable_clock();

			NFI_SET_REG32(NFI_DEBUG_CON1_REG16, NFI_BYPASS);
			NFI_SET_REG32(ECC_BYPASS_REG32, ECC_BYPASS);
			DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);

			g_bSyncOrToggle = FALSE;

			return 0;
		}
		pr_debug("[%s] DDR interface\n", __func__);

	} else {
		pr_debug("[%s] legacy interface\n", __func__);
		g_bSyncOrToggle = FALSE;
	}

	return 1;
}

void mtk_nand_interface_switch(struct mtd_info *mtd)
{
	if (devinfo.iowidth == IO_ONFI || devinfo.iowidth == IO_TOGGLEDDR
		|| devinfo.iowidth == IO_TOGGLESDR) {
		if (g_bSyncOrToggle == FALSE) {
			if (mtk_nand_interface_config(mtd)) {
				pr_debug("[NFI] interface switch sync!!!!\n");
				g_bSyncOrToggle = TRUE;
			} else {
				pr_debug("[NFI] interface switch fail!!!!\n");
				g_bSyncOrToggle = FALSE;
			}
		}
	}
}

#if CFG_RANDOMIZER
static int mtk_nand_turn_on_randomizer(u32 page, int type, int fgPage)
{
	u32 u4NFI_CFG = 0;
	u32 u4NFI_RAN_CFG = 0;
	u32 u4PgNum = page % PAGES_PER_BLOCK;

	u4NFI_CFG = DRV_Reg32(NFI_CNFG_REG16);

	DRV_WriteReg32(NFI_ENMPTY_THRESH_REG32, 40);

	if (type) {
		DRV_WriteReg32(NFI_RANDOM_ENSEED01_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_ENSEED02_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_ENSEED03_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_ENSEED04_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_ENSEED05_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_ENSEED06_TS_REG32, 0);
	} else {
		DRV_WriteReg32(NFI_RANDOM_DESEED01_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_DESEED02_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_DESEED03_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_DESEED04_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_DESEED05_TS_REG32, 0);
		DRV_WriteReg32(NFI_RANDOM_DESEED06_TS_REG32, 0);
	}

	u4NFI_CFG |= CNFG_RAN_SEL;
	if (PAGES_PER_BLOCK <= SS_SEED_NUM) {
		if (type)
			u4NFI_RAN_CFG |= RAN_CNFG_ENCODE_SEED(SS_RANDOM_SEED[u4PgNum % PAGES_PER_BLOCK])
						| RAN_CNFG_ENCODE_EN;
		else
			u4NFI_RAN_CFG |= RAN_CNFG_DECODE_SEED(SS_RANDOM_SEED[u4PgNum % PAGES_PER_BLOCK])
						| RAN_CNFG_DECODE_EN;
	} else {
		if (type)
			u4NFI_RAN_CFG |= RAN_CNFG_ENCODE_SEED(SS_RANDOM_SEED[u4PgNum & (SS_SEED_NUM-1)])
						| RAN_CNFG_ENCODE_EN;
		else
			u4NFI_RAN_CFG |= RAN_CNFG_DECODE_SEED(SS_RANDOM_SEED[u4PgNum & (SS_SEED_NUM-1)])
						| RAN_CNFG_DECODE_EN;
	}

	if (fgPage)
		u4NFI_CFG &= ~CNFG_RAN_SEC;
	else
		u4NFI_CFG |= CNFG_RAN_SEC;

	DRV_WriteReg32(NFI_CNFG_REG16, u4NFI_CFG);
	DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, u4NFI_RAN_CFG);
	return 0;
}

static bool mtk_nand_israndomizeron(void)
{
	u32 nfi_ran_cnfg = 0;

	nfi_ran_cnfg = DRV_Reg32(NFI_RANDOM_CNFG_REG32);
	if (nfi_ran_cnfg & (RAN_CNFG_ENCODE_EN | RAN_CNFG_DECODE_EN))
		return TRUE;

	return FALSE;
}

static void mtk_nand_turn_off_randomizer(void)
{
	u32 u4NFI_CFG = DRV_Reg32(NFI_CNFG_REG16);

	u4NFI_CFG &= ~CNFG_RAN_SEL;
	u4NFI_CFG &= ~CNFG_RAN_SEC;
	DRV_WriteReg32(NFI_RANDOM_CNFG_REG32, 0);
	DRV_WriteReg32(NFI_CNFG_REG16, u4NFI_CFG);
	/* MSG(INIT, "[K]ran turn off\n"); */
}
#else
#define mtk_nand_israndomizeron() (FALSE)
#define mtk_nand_turn_on_randomizer(page, type, fgPage)
#define mtk_nand_turn_off_randomizer()
#endif

/******************************************************************************
 * mtk_nand_irq_handler
 *
 * DESCRIPTION:
 *	 NAND interrupt handler!
 *
 * PARAMETERS:
 *	 int irq
 *	 void *dev_id
 *
 * RETURNS:
 *	 IRQ_HANDLED : Successfully handle the IRQ
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
/* Modified for TCM used */

static irqreturn_t mtk_nand_irq_handler(int irqno, void *dev_id)
{
	u16 u16IntStatus = DRV_Reg16(NFI_INTR_REG16);

	/* pr_debug("mtk_nand_irq_handler 0x%x here\n", u16IntStatus); */
	if (u16IntStatus & (u16) INTR_BSY_RTN) {
		NFI_CLN_REG16(NFI_INTR_EN_REG16, INTR_BSY_RTN_EN);
		complete(&g_comp_Busy_Ret);
	}

	if (u16IntStatus & (u16) INTR_AHB_DONE) {
		NFI_CLN_REG16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
		mtk_nand_handle_write_ahb_done();
		if (host->wb_cmd != NAND_CMD_PAGEPROG)
			complete(&g_comp_AHB_Done);
	}

	if (u16IntStatus & (u16) INTR_WR_DONE) {
		NFI_CLN_REG16(NFI_INTR_EN_REG16, INTR_WR_DONE_EN);
		complete(&g_comp_WR_Done);
	} else if (u16IntStatus & (u16) INTR_ERASE_DONE) {
		NFI_CLN_REG16(NFI_INTR_EN_REG16, INTR_ERASE_DONE_EN);
		complete(&g_comp_ER_Done);
	}
	return IRQ_HANDLED;
}

/******************************************************************************
 * ECC_Config
 *
 * DESCRIPTION:
 *	 Configure HW ECC!
 *
 * PARAMETERS:
 *	 struct mtk_nand_host_hw *hw
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void ECC_Config(struct mtk_nand_host_hw *hw, u32 ecc_bit)
{
	u32 u4ENCODESize;
	u32 u4DECODESize;
	u32 ecc_bit_cfg = ECC_CNFG_ECC4;

	/* Sector + FDM + YAFFS2 meta data bits */
	u4DECODESize = ((hw->nand_sec_size + hw->nand_fdm_size) << 3) + ecc_bit * ECC_PARITY_BIT;

	switch (ecc_bit) {
#ifndef MTK_COMBO_NAND_SUPPORT
	case 4:
		ecc_bit_cfg = ECC_CNFG_ECC4;
		break;
	case 8:
		ecc_bit_cfg = ECC_CNFG_ECC8;
		break;
	case 10:
		ecc_bit_cfg = ECC_CNFG_ECC10;
		break;
	case 12:
		ecc_bit_cfg = ECC_CNFG_ECC12;
		break;
	case 14:
		ecc_bit_cfg = ECC_CNFG_ECC14;
		break;
	case 16:
		ecc_bit_cfg = ECC_CNFG_ECC16;
		break;
	case 18:
		ecc_bit_cfg = ECC_CNFG_ECC18;
		break;
	case 20:
		ecc_bit_cfg = ECC_CNFG_ECC20;
		break;
	case 22:
		ecc_bit_cfg = ECC_CNFG_ECC22;
		break;
	case 24:
		ecc_bit_cfg = ECC_CNFG_ECC24;
		break;
#endif
	case 28:
		ecc_bit_cfg = ECC_CNFG_ECC28;
		break;
	case 32:
		ecc_bit_cfg = ECC_CNFG_ECC32;
		break;
	case 36:
		ecc_bit_cfg = ECC_CNFG_ECC36;
		break;
	case 40:
		ecc_bit_cfg = ECC_CNFG_ECC40;
		break;
	case 44:
		ecc_bit_cfg = ECC_CNFG_ECC44;
		break;
	case 48:
		ecc_bit_cfg = ECC_CNFG_ECC48;
		break;
	case 52:
		ecc_bit_cfg = ECC_CNFG_ECC52;
		break;
	case 56:
		ecc_bit_cfg = ECC_CNFG_ECC56;
		break;
	case 60:
		ecc_bit_cfg = ECC_CNFG_ECC60;
		break;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	case 68:
		ecc_bit_cfg = ECC_CNFG_ECC68;
		/* Marked, Causing FDM data last byte one bit error */
		/* u4DECODESize -= 7; */
		break;
	case 72:
		ecc_bit_cfg = ECC_CNFG_ECC72;
		/* Marked, Causing FDM data last byte one bit error */
		/* u4DECODESize -= 7; */
		break;
	case 80:
		ecc_bit_cfg = ECC_CNFG_ECC80;
		/* Marked, Causing FDM data last byte one bit error */
		/* u4DECODESize -= 7; */
		break;
#endif
	default:
		break;

	}
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
	do {
		;
	} while (!DRV_Reg16(ECC_DECIDLE_REG16));

	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
	do {
		;
	} while (!DRV_Reg32(ECC_ENCIDLE_REG32));

	/* setup FDM register base */
	/* DRV_WriteReg32(ECC_FDMADDR_REG32, NFI_FDM0L_REG32); */

	/* Sector + FDM */
	u4ENCODESize = (hw->nand_sec_size + hw->nand_fdm_size) << 3;
	/* Sector + FDM + YAFFS2 meta data bits */

	/* configure ECC decoder && encoder */
	DRV_WriteReg32(ECC_DECCNFG_REG32,
			   ecc_bit_cfg | DEC_CNFG_NFI | DEC_CNFG_EMPTY_EN | (u4DECODESize <<
									 DEC_CNFG_CODE_SHIFT));

	DRV_WriteReg32(ECC_ENCCNFG_REG32,
			   ecc_bit_cfg | ENC_CNFG_NFI | (u4ENCODESize << ENC_CNFG_MSG_SHIFT));
#ifndef MANUAL_CORRECT
	NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT);
#else
	NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_EL);
#endif
}

/******************************************************************************
 * ECC_Decode_Start
 *
 * DESCRIPTION:
 *	 HW ECC Decode Start !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void ECC_Decode_Start(void)
{
	/* wait for device returning idle */
	while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE))
		;
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);
}

/******************************************************************************
 * ECC_Decode_End
 *
 * DESCRIPTION:
 *	 HW ECC Decode End !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void ECC_Decode_End(void)
{
	/* wait for device returning idle */
	while (!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE))
		;
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
}

/******************************************************************************
 * ECC_Encode_Start
 *
 * DESCRIPTION:
 *	 HW ECC Encode Start !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void ECC_Encode_Start(void)
{
	/* wait for device returning idle */
	while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE))
		;
	mb();
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_EN);
}

/******************************************************************************
 * ECC_Encode_End
 *
 * DESCRIPTION:
 *	 HW ECC Encode End !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void ECC_Encode_End(void)
{
	/* wait for device returning idle */
	while (!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE))
		;
	mb();
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
}

/******************************************************************************
 * mtk_nand_check_bch_error
 *
 * DESCRIPTION:
 *	 Check BCH error or not !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd
 *	 u8* pDataBuf
 *	 u32 u4SecIndex
 *	 u32 u4PageAddr
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_check_bch_error(struct mtd_info *mtd, u8 *pDataBuf, u8 *spareBuf,
					 u32 u4SecIndex, u32 u4PageAddr, u32 *bitmap)
{
	bool ret = true;
	u16 u2SectorDoneMask = 1 << u4SecIndex;
	u32 u4ErrorNumDebug0, u4ErrorNumDebug1, i, u4ErrNum;
#ifdef MANUAL_CORRECT
	u32 j;
#endif
	u32 timeout = 0xFFFF;
	u32 correct_count = 0;
	u32 page_size = (u4SecIndex + 1) * host->hw->nand_sec_size;
	u32 sec_num = u4SecIndex + 1;
	/* u32 bitflips = sec_num * 39; */
	u16 failed_sec = 0;
	u32 maxSectorBitErr = 0;

#ifdef MANUAL_CORRECT
	u32 err_pos, temp;
	u32 u4ErrByteLoc, u4BitOffset;
#endif
	/* u32 index1; */
	/* u32 u4ErrBitLoc1th, u4ErrBitLoc2nd; */
	/* u32 au4ErrBitLoc[20]; */
	u32 ERR_NUM0 = 0;

	if (mtk_nfi_dev_comp->chip_ver == 1)
		ERR_NUM0 = ERR_NUM0_V1;
	else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3))
		ERR_NUM0 = ERR_NUM0_V2;

	while (0 == (u2SectorDoneMask & DRV_Reg16(ECC_DECDONE_REG16))) {
		timeout--;
		if (timeout == 0)
			return false;
	}
#ifndef MANUAL_CORRECT
	if (0 == (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY)) {
		u4ErrorNumDebug0 = DRV_Reg32(ECC_DECENUM0_REG32);
		u4ErrorNumDebug1 = DRV_Reg32(ECC_DECENUM1_REG32);
		if (0 != (u4ErrorNumDebug0 & 0xFFFFFFFF) || 0 != (u4ErrorNumDebug1 & 0xFFFFFFFF)) {
			for (i = 0; i <= u4SecIndex; ++i) {
#if 1
				u4ErrNum = (DRV_Reg32((ECC_DECENUM0_REG32 + (i / 4))) >> ((i % 4) * 8)) & ERR_NUM0;
#else
				if (i < 4)
					u4ErrNum = DRV_Reg32(ECC_DECENUM0_REG32) >> (i * 8);
				else
					u4ErrNum = DRV_Reg32(ECC_DECENUM1_REG32) >> ((i - 4) * 8);
				u4ErrNum &= ERR_NUM0;
#endif
				/* pr_debug("[XL] errnm %d, sec %d\n", u4ErrNum, i); */
				/* Nand_ErrNUM[i] = u4ErrNum; */
				/* for (index1 = 0; index1 < ((u4ErrNum + 1) >> 1); ++index1) { */
					/* au4ErrBitLoc[index1] = DRV_Reg32(ECC_DECEL0_REG32 + index1); */
					/* u4ErrBitLoc1th = au4ErrBitLoc[index1] & 0x3FFF; */
					/* u4ErrBitLoc2nd = (au4ErrBitLoc[index1] >> 16) & 0x3FFF; */
					/* Nand_ErrBitLoc[i][index1] = au4ErrBitLoc[index1]; */
					/* pr_info("[XL] EL%d = 0x%x EL%d = 0x%x\n", */
						/* i*2, u4ErrBitLoc1th, i*2 + 1, u4ErrBitLoc2nd); */
				/* } */

				if (u4ErrNum == ERR_NUM0) {
					failed_sec++;
					ret = false;
					continue;
				}
				if (bitmap)
					*bitmap |= 1 << i;
				if (u4ErrNum) {
					if (maxSectorBitErr < u4ErrNum)
						maxSectorBitErr = u4ErrNum;
					correct_count += u4ErrNum;
				}
			}
			mtd->ecc_stats.failed += failed_sec;
			if ((maxSectorBitErr > ecc_threshold) && (ret != FALSE)) {
				pr_debug("ECC bit flips (0x%x) exceed eccthreshold (0x%x),u4PageAddr 0x%x\n",
					maxSectorBitErr, ecc_threshold, u4PageAddr);
				/* mtd->ecc_stats.corrected++; */
			}
		}
	}
	if (host->debug.err.en_print)
		host->debug.err.max_num = maxSectorBitErr;
	if ((DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY) != 0) {
		ret = true;
		/* MSG(INIT, "empty page, empty buffer returned\n"); */
		memset(pDataBuf, 0xff, page_size);
		memset(spareBuf, 0xff, sec_num * host->hw->nand_fdm_size);
		maxSectorBitErr = 0;
		failed_sec = 0;
	}
#else
	for (j = 0; j <= u4SecIndex; ++j) {
		u4ErrNum = (DRV_Reg32((ECC_DECENUM0_REG32 + (j / 4))) >> ((j % 4) * 8)) & ERR_NUM0;
		/* We will manually correct the error bits in the last sector, not all the sectors of the page! */
		memset(au4ErrBitLoc, 0x0, sizeof(au4ErrBitLoc));
		/* u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32); */
		/* u4ErrNum = (DRV_Reg32((ECC_DECENUM_REG32+(u4SecIndex/4)))>>((u4SecIndex%4)*8))& ERR_NUM0; */
		/* pr_debug("[XL1] errnm %d, sec %d\n", u4ErrNum, j); */

		if (u4ErrNum) {
			if (u4ErrNum == ERR_NUM0) {
				mtd->ecc_stats.failed++;
				ret = false;
				/*pr_info("UnCorrectable at PageAddr=%d\n", u4PageAddr);*/
				continue;
			}
			for (i = 0; i < ((u4ErrNum + 1) >> 1); i++) {
				/* get error location */
				au4ErrBitLoc[i] = DRV_Reg32(ECC_DECEL0_REG32 + i);
				/* pr_debug("[XL1] errloc[%d] 0x%x\n", i,au4ErrBitLoc[i]); */
			}
			for (i = 0; i < u4ErrNum; i++) {
				/* MCU error correction */
				err_pos = ((au4ErrBitLoc[i >> 1] >> ((i & 0x01) << 4)) & 0x3FFF);
				/* *(data_buff+(err_pos>>3)) ^= (1<<(err_pos&0x7)); */
				u4ErrByteLoc = err_pos >> 3;
				if (u4ErrByteLoc < host->hw->nand_sec_size) {
					pDataBuf[host->hw->nand_sec_size * j + u4ErrByteLoc] ^=
						(1 << (err_pos & 0x7));
					continue;
				}
				/* BytePos is in FDM data and auto-format. */
				u4ErrByteLoc -= host->hw->nand_sec_size;
				if (u4ErrByteLoc < 8) {	/* fdm size */
					if (u4ErrByteLoc >= 4) {
						temp = DRV_Reg32(NFI_FDM0M_REG32 + (j << 1));
						u4ErrByteLoc -= 4;
						temp ^= (1 << ((err_pos & 0x7) + (u4ErrByteLoc << 3)));
						DRV_WriteReg32(NFI_FDM0M_REG32 + (j << 1), temp);
					} else {
						temp = DRV_Reg32(NFI_FDM0L_REG32 + (j << 1));
						temp ^= (1 << ((err_pos & 0x7) + (u4ErrByteLoc << 3)));
						DRV_WriteReg32(NFI_FDM0L_REG32 + (j << 1), temp);
					}
				}
			}
			/* mtd->ecc_stats.corrected++; */
		}
	}
#endif
	return ret;
}

/******************************************************************************
 * mtk_nand_RFIFOValidSize
 *
 * DESCRIPTION:
 *	 Check the Read FIFO data bytes !
 *
 * PARAMETERS:
 *	 u16 u2Size
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_RFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;

	while (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) < u2Size) {
		timeout--;
		if (timeout == 0)
			return false;
	}
	return true;
}

/******************************************************************************
 * mtk_nand_WFIFOValidSize
 *
 * DESCRIPTION:
 *	 Check the Write FIFO data bytes !
 *
 * PARAMETERS:
 *	 u16 u2Size
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_WFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;

	while (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) > u2Size) {
		timeout--;
		if (timeout == 0)
			return false;
	}
	return true;
}

/******************************************************************************
 * mtk_nand_status_ready
 *
 * DESCRIPTION:
 *	 Indicate the NAND device is ready or not !
 *
 * PARAMETERS:
 *	 u32 u4Status
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_status_ready(u32 u4Status)
{
	u32 timeout = 0xFFFF;

	while ((DRV_Reg32(NFI_STA_REG32) & u4Status) != 0) {
		timeout--;
		if (timeout == 0)
			return false;
	}
	return true;
}

/******************************************************************************
 * mtk_nand_reset
 *
 * DESCRIPTION:
 *	 Reset the NAND device hardware component !
 *
 * PARAMETERS:
 *	 struct mtk_nand_host *host (Initial setting data)
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_reset(void)
{
	/* HW recommended reset flow */
	int timeout = 0xFFFF;

	if (DRV_Reg16(NFI_MASTERSTA_REG16) & 0xFFF) {	/* master is busy */
		mb();
		DRV_WriteReg32(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);
		while (DRV_Reg16(NFI_MASTERSTA_REG16) & 0xFFF) {
			timeout--;
			if (!timeout)
				pr_notice("Wait for NFI_MASTERSTA timeout\n");
		}
	}
	/* issue reset operation */
	mb();
	DRV_WriteReg32(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);

	return mtk_nand_status_ready(STA_NFI_FSM_MASK | STA_NAND_BUSY) && mtk_nand_RFIFOValidSize(0)
		&& mtk_nand_WFIFOValidSize(0);
}

/******************************************************************************
 * mtk_nand_set_mode
 *
 * DESCRIPTION:
 *	  Set the oepration mode !
 *
 * PARAMETERS:
 *	 u16 u2OpMode (read/write)
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_set_mode(u16 u2OpMode)
{
	u16 u2Mode = DRV_Reg16(NFI_CNFG_REG16);

	u2Mode &= ~CNFG_OP_MODE_MASK;
	u2Mode |= u2OpMode;
	DRV_WriteReg16(NFI_CNFG_REG16, u2Mode);
}

/******************************************************************************
 * mtk_nand_set_autoformat
 *
 * DESCRIPTION:
 *	  Enable/Disable hardware autoformat !
 *
 * PARAMETERS:
 *	 bool bEnable (Enable/Disable)
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_set_autoformat(bool bEnable)
{
	if (bEnable)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
	else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
}

/******************************************************************************
 * mtk_nand_configure_fdm
 *
 * DESCRIPTION:
 *	 Configure the FDM data size !
 *
 * PARAMETERS:
 *	 u16 u2FDMSize
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_configure_fdm(u16 u2FDMSize)
{
	if (mtk_nfi_dev_comp->chip_ver == 1) {
		NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
		NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_SHIFT);
		NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_ECC_SHIFT);
	} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
		NFI_CLN_REG32(NFI_PAGEFMT_REG32, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
		NFI_SET_REG32(NFI_PAGEFMT_REG32, u2FDMSize << PAGEFMT_FDM_SHIFT);
		NFI_SET_REG32(NFI_PAGEFMT_REG32, u2FDMSize << PAGEFMT_FDM_ECC_SHIFT);
	} else {
		nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
			mtk_nfi_dev_comp->chip_ver);
	}
}

static bool mtk_nand_pio_ready(void)
{
	int count = 0;

	while (!(DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)) {
		count++;
		if (count > 0xffff) {
			pr_info("PIO_DIRDY timeout\n");
			return false;
		}
	}

	return true;
}

/******************************************************************************
 * mtk_nand_set_command
 *
 * DESCRIPTION:
 *	  Send hardware commands to NAND devices !
 *
 * PARAMETERS:
 *	 u16 command
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_set_command(u16 command)
{
	/* Write command to device */
	mb();
	DRV_WriteReg16(NFI_CMD_REG16, command);
	return mtk_nand_status_ready(STA_CMD_STATE);
}

/******************************************************************************
 * mtk_nand_set_address
 *
 * DESCRIPTION:
 *	  Set the hardware address register !
 *
 * PARAMETERS:
 *	 struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_set_address(u32 u4ColAddr, u32 u4RowAddr, u16 u2ColNOB, u16 u2RowNOB)
{
	/* fill cycle addr */
	mb();
	DRV_WriteReg32(NFI_COLADDR_REG32, u4ColAddr);
	DRV_WriteReg32(NFI_ROWADDR_REG32, u4RowAddr);
	DRV_WriteReg16(NFI_ADDRNOB_REG16, u2ColNOB | (u2RowNOB << ADDR_ROW_NOB_SHIFT));
	return mtk_nand_status_ready(STA_ADDR_STATE);
}

/* ------------------------------------------------------------------------------- */
static bool mtk_nand_device_reset(void)
{
	u32 timeout = 0xFFFF;

	mtk_nand_reset();

	DRV_WriteReg(NFI_CNFG_REG16, CNFG_OP_RESET);

	mtk_nand_set_command(NAND_CMD_RESET);

	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;

	if (!timeout)
		return FALSE;
	else
		return TRUE;
}

/* ------------------------------------------------------------------------------- */

/******************************************************************************
 * mtk_nand_check_RW_count
 *
 * DESCRIPTION:
 *	  Check the RW how many sectors !
 *
 * PARAMETERS:
 *	 u16 u2WriteSize
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_check_RW_count(u16 u2WriteSize)
{
	u32 timeout = 0xFFFF;
	u16 u2SecNum = u2WriteSize >> host->hw->nand_sec_shift;

	while (ADDRCNTR_CNTR(DRV_Reg32(NFI_ADDRCNTR_REG16)) < u2SecNum) {
		timeout--;
		if (timeout == 0) {
			pr_info("[%s] timeout\n", __func__);
			return false;
		}
	}
	return true;
}

/******************************************************************************
 * mtk_nand_ready_for_read
 *
 * DESCRIPTION:
 *	  Prepare hardware environment for read !
 *
 * PARAMETERS:
 *	 struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_ready_for_read(struct nand_chip *nand, u32 u4RowAddr, u32 u4ColAddr,
					u16 sec_num, bool full, u8 *buf, enum readCommand cmd)
{
	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */
	bool bRet = false;
	/* u16 sec_num = 1 << (nand->page_shift - host->hw->nand_sec_shift); */
	u32 col_addr = u4ColAddr;
	u32 colnob = 2, rownob = devinfo.addr_cycle - 2;

	/* u32 reg_val = DRV_Reg32(NFI_MASTERRST_REG32); */
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
#if __INTERNAL_USE_AHB_MODE__
	unsigned int phys = 0;
#endif
#endif
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif

#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
	if (full) {
		mtk_dir = DMA_FROM_DEVICE;
		sg_init_one(&mtk_sg, buf, (sec_num * (1 << host->hw->nand_sec_shift)));
		dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
		phys = mtk_sg.dma_address;
		/* pr_debug("[xl] phys va 0x%x\n", phys); */
	}
#endif

	if (DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32) & 0x3) {
		NFI_SET_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);	/* reset */
		NFI_CLN_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);	/* dereset */
	}

	if (nand->options & NAND_BUSWIDTH_16)
		col_addr /= 2;

	if (!mtk_nand_reset()) {
		nand_pr_err("nand reset failed!");
		goto cleanup;
	}
	if (g_bHwEcc) {
		/* Enable HW ECC */
		NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	} else {
		NFI_CLN_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}

	mtk_nand_set_mode(CNFG_OP_READ);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
	DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);
#endif
	if (full) {
#if __INTERNAL_USE_AHB_MODE__
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
		/* phys = nand_virt_to_phys_add((unsigned long) buf); */
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
		if (!phys) {
			nand_pr_err("convert virt addr (%lx) to phys add (%x)fail!!!",
				   (unsigned long)buf, phys);
			return false;
		}
		DRV_WriteReg32(NFI_STRADDR_REG32, phys);
#endif
#else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

		if (g_bHwEcc)
			NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		else
			NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

	} else {
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
	}

	mtk_nand_set_autoformat(full);
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
	if (full) {
		if (g_bHwEcc)
			ECC_Decode_Start();
	}
#endif
	if (cmd == AD_CACHE_FINAL) {
		if (!mtk_nand_set_command(0x3F)) {
			nand_pr_err("Send 0x3F command failed!");
			goto cleanup;
		}
		if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
			nand_pr_err("STA_NAND_BUSY after 0x3F failed!");
			goto cleanup;
		}
		return true;
	}
	if (!mtk_nand_set_command(NAND_CMD_READ0)) {
		nand_pr_err("NAND_CMD_READ0 failed!");
		goto cleanup;
	}
	if (!mtk_nand_set_address(col_addr, u4RowAddr, colnob, rownob)) {
		nand_pr_err("mtk_nand_set_address failed!");
		goto cleanup;
	}
	if (cmd == NORMAL_READ) {
		if (!mtk_nand_set_command(NAND_CMD_READSTART)) {
			nand_pr_err("NAND_CMD_READSTART failed!");
			goto cleanup;
		}
	} else {
		if (!mtk_nand_set_command(0x31)) {
			nand_pr_err("send 0x31 command failed!");
			goto cleanup;
		}
	}

	if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
		nand_pr_err("check STA_NAND_BUSY failed!");
		goto cleanup;
	}

	bRet = true;

cleanup:
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.ReadBusyTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.ReadBusyCount++;
#endif
	if (!bRet)
		nand_pr_err("fail!!! will return false.");
	return bRet;
}

/******************************************************************************
 * mtk_nand_ready_for_write
 *
 * DESCRIPTION:
 *	  Prepare hardware environment for write !
 *
 * PARAMETERS:
 *	 struct nand_chip *nand, u32 u4RowAddr
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_ready_for_write(struct nand_chip *nand, u32 u4RowAddr, u32 col_addr, bool full,
					 u8 *buf)
{
	bool bRet = false;
	u32 sec_num = 1 << (nand->page_shift - host->hw->nand_sec_shift);
	u32 colnob = 2, rownob = devinfo.addr_cycle - 2;
	u16 prg_cmd;
#if __INTERNAL_USE_AHB_MODE__
	unsigned int phys = 0;
	/* u32 T_phys=0; */
#endif
	u32 temp_sec_num;

	temp_sec_num = sec_num;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		&& devinfo.tlcControl.normaltlc
		&& devinfo.tlcControl.pPlaneEn) {

		temp_sec_num = sec_num / 2;
	}
#endif
	if (devinfo.two_phyplane)
		temp_sec_num /= 2;

	if (full) {
		mtk_dir = DMA_TO_DEVICE;
		sg_init_one(&mtk_sg, buf, (1 << nand->page_shift));
		dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
		phys = mtk_sg.dma_address;
		/* pr_debug("[xl] phys va 0x%x\n", phys); */
	}

	if (nand->options & NAND_BUSWIDTH_16)
		col_addr /= 2;

	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */
	if (!mtk_nand_reset()) {
		nand_pr_err("(mtk_nand_reset) fail!");
		return false;
	}

	mtk_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg32(NFI_CON_REG16, temp_sec_num << CON_NFI_SEC_SHIFT);

	if (full) {
#if __INTERNAL_USE_AHB_MODE__
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
		/* phys = nand_virt_to_phys_add((unsigned long) buf); */
		/* T_phys=__virt_to_phys(buf); */
		if (!phys) {
			nand_pr_err("convert virt addr (%lx) to phys add fail!!!",
				   (unsigned long)buf);
			return false;
		}
		DRV_WriteReg32(NFI_STRADDR_REG32, phys);
#else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif
		if (g_bHwEcc)
			NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		else
			NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	} else {
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
	}

	mtk_nand_set_autoformat(full);

	if (full) {
		if (g_bHwEcc)
			ECC_Encode_Start();
	}

	prg_cmd = NAND_CMD_SEQIN;
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER)
		&& (devinfo.two_phyplane || (devinfo.advancedmode & MULTI_PLANE))
		&& tlc_snd_phyplane)
		prg_cmd = PLANE_INPUT_DATA_CMD;
	if (!mtk_nand_set_command(prg_cmd)) {
		nand_pr_err("(mtk_nand_set_command) fail!");
		goto cleanup;
	}
	/* 1 FIXED ME: For Any Kind of AddrCycle */
	if (!mtk_nand_set_address(col_addr, u4RowAddr, colnob, rownob)) {
		nand_pr_err("(mtk_nand_set_address) fail!");
		goto cleanup;
	}

	if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
		nand_pr_err("(mtk_nand_status_ready) fail!");
		goto cleanup;
	}

	bRet = true;
cleanup:

	return bRet;
}

static bool mtk_nand_check_dececc_done(u32 u4SecNum)
{
	u32 dec_mask;
	u32 fsm_mask;
	u32 ECC_DECFSM_IDLE;
	u32 timeout_us = 1000000;

	dec_mask = (1 << (u4SecNum - 1));
	while (dec_mask != (DRV_Reg(ECC_DECDONE_REG16) & dec_mask)) {
		if (!timeout_us) {
			pr_notice("ECC_DECDONE: timeout1 0x%x %d\n", DRV_Reg(ECC_DECDONE_REG16),
				u4SecNum);
			dump_nfi();
			return false;
		}

		timeout_us--;
		udelay(1);
	}

	if (mtk_nfi_dev_comp->chip_ver == 1) {
		fsm_mask = 0x7F0F0F0F;
		ECC_DECFSM_IDLE = ECC_DECFSM_IDLE_V1;
	} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
		fsm_mask = 0x3F3FFF0F;
		ECC_DECFSM_IDLE = ECC_DECFSM_IDLE_V2;
	} else {
		fsm_mask = 0xFFFFFFFF;
		ECC_DECFSM_IDLE = 0xFFFFFFFF;
		nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
			mtk_nfi_dev_comp->chip_ver);
	}

	while ((DRV_Reg32(ECC_DECFSM_REG32) & fsm_mask) != ECC_DECFSM_IDLE) {
		if (!timeout_us) {
			pr_notice("ECC_DECDONE: timeout2 0x%x 0x%x %d\n",
				DRV_Reg32(ECC_DECFSM_REG32), DRV_Reg(ECC_DECDONE_REG16), u4SecNum);
			dump_nfi();
			return false;
		}

		timeout_us--;
		udelay(1);
	}
	return true;
}

/******************************************************************************
 * mtk_nand_read_page_data
 *
 * DESCRIPTION:
 *	 Fill the page data into buffer !
 *
 * PARAMETERS:
 *	 u8* pDataBuf, u32 u4Size
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static bool mtk_nand_dma_read_data(struct mtd_info *mtd, u8 *buf, u32 length)
{
	/* Disable IRQ for DMA read data */
	int interrupt_en = 0; /*g_i4Interrupt;*/
	unsigned long long timeout = 0xfffff;
	/* struct scatterlist sg; */
	/* enum dma_data_direction dir = DMA_FROM_DEVICE; */
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	/* pr_debug("[xl] dma read buf in 0x%lx\n", (unsigned long)buf); */
	/* sg_init_one(&sg, buf, length); */
	/* pr_debug("[xl] dma read buf out 0x%lx\n", (unsigned long)buf); */
	/* dma_map_sg(&(mtd->dev), &sg, 1, dir); */

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	/* DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(pDataBuf)); */

	if ((unsigned long)buf % 16) {	/* TODO: can not use AHB mode here */
		pr_debug("Un-16-aligned address\n");
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
	} else {
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
	}

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);

	DRV_Reg16(NFI_INTR_REG16);
	if (interrupt_en)
		DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);

	if (interrupt_en)
		init_completion(&g_comp_AHB_Done);
	/* dmac_inv_range(pDataBuf, pDataBuf + u4Size); */
	mb();
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BRD);
	g_running_dma = 1;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
	&& (devinfo.tlcControl.needchangecolumn))
		DRV_WriteReg16(NFI_TLC_RD_WHR2_REG16, (TLC_RD_WHR2_EN | 0x055));
#endif

	if (interrupt_en) {
		/* Wait 10ms for AHB done */
		if (!wait_for_completion_timeout(&g_comp_AHB_Done, msecs_to_jiffies(NFI_TIMEOUT_MS))) {
			pr_notice("wait for completion timeout happened @ [%s]: %d\n", __func__,
				__LINE__);
			dump_nfi();
			g_running_dma = 0;
			return false;
		}
		g_running_dma = 0;
		while ((length >> host->hw->nand_sec_shift) >
			   ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12)) {
			timeout--;
			if (timeout == 0) {
				nand_pr_err("poll BYTELEN error");
				g_running_dma = 0;
				return false;	/* 4  // AHB Mode Time Out! */
			}
		}
	} else {
		int sector_read_time = 0;
		unsigned long long s = 0, e = 0;

		g_running_dma = 0;
		if (ahb_read_sleep_enable()) {
			sector_read_time = get_sector_ahb_read_time();
			if (!sector_read_time) {
				s = sched_clock();
			} else if (ahb_read_can_sleep(length)) {
				int min, max;

				get_ahb_read_sleep_range(length, &min, &max);
				usleep_range(min, max);
			}
		}

		while ((length >> host->hw->nand_sec_shift) >
			   ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12)) {
			timeout--;
			if (timeout == 0) {
				nand_pr_err("poll BYTELEN error");
				dump_nfi();
				g_running_dma = 0;
				return false;
			}
		}
		timeout = 0xffffffffffff;
		while ((DRV_Reg32(NFI_MASTERSTA_REG16) & 0x3) != 0) {
			timeout--;
			if (timeout == 0) {
				nand_pr_err("poll NFI_MASTERSTA_REG16 error (0x%x)",
						DRV_Reg32(NFI_MASTERSTA_REG16));
				dump_nfi();
				g_running_dma = 0;
				return false;
			}
		}

		if (ahb_read_sleep_enable() && !sector_read_time) {
			e = sched_clock();
			nand_info("s:(%llu) e:(%llu)\n", s, e);
			cal_sector_ahb_read_time(s, e, length);
		}
	}

	/* dma_unmap_sg(&(mtd->dev), &sg, 1, dir); */
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.ReadDMATotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.ReadDMACount++;
#endif
	return true;
}

static bool mtk_nand_mcu_read_data(struct mtd_info *mtd, u8 *buf, u32 length)
{
	int timeout = 0xffff;
	u32 i;
	u32 *buf32 = (u32 *) buf;
#ifdef TESTTIME
	unsigned long long time1, time2;

	time1 = sched_clock();
#endif
	if ((unsigned long)buf % 4 || length % 4)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

	/* DRV_WriteReg32(NFI_STRADDR_REG32, 0); */
	mb();
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BRD);

	if ((unsigned long)buf % 4 || length % 4) {
		for (i = 0; (i < (length)) && (timeout > 0);) {
			/* if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4) */
			if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1) {
				*buf++ = (u8) DRV_Reg32(NFI_DATAR_REG32);
				i++;
			} else {
				timeout--;
			}
			if (timeout == 0) {
				nand_pr_err("timeout");
				dump_nfi();
				return false;
			}
		}
	} else {
		for (i = 0; (i < (length >> 2)) && (timeout > 0);) {
			/* if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4) */
			if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1) {
				*buf32++ = DRV_Reg32(NFI_DATAR_REG32);
				i++;
			} else {
				timeout--;
			}
			if (timeout == 0) {
				nand_pr_err("timeout");
				dump_nfi();
				return false;
			}
		}
	}
#ifdef TESTTIME
	time2 = sched_clock() - time1;
	if (!readdatatime)
		readdatatime = (time2);
#endif
	return true;
}

static bool mtk_nand_read_page_data(struct mtd_info *mtd, u8 *pDataBuf, u32 u4Size)
{
#if (__INTERNAL_USE_AHB_MODE__)
	return mtk_nand_dma_read_data(mtd, pDataBuf, u4Size);
#else
	return mtk_nand_mcu_read_data(mtd, pDataBuf, u4Size);
#endif
}

static void mtk_nand_dma_start_write(struct mtd_info *mtd, u8 *pDataBuf, bool en_ahb_int)
{
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	u32 reg_val;
#endif

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	DRV_Reg16(NFI_INTR_REG16);
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

	if ((unsigned long)pDataBuf % 16) {
		pr_debug("Un-16-aligned address\n");
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
	} else {
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
	}

	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);

	if (en_ahb_int) {
		DRV_Reg16(NFI_INTR_REG16);
		DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
	}
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		reg_val = DRV_Reg16(NFI_DEBUG_CON1_REG16);
		reg_val |= 0x4000;
		DRV_WriteReg16(NFI_DEBUG_CON1_REG16, reg_val);
	}
#endif
	mb();
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
}

static bool mtk_nand_mcu_write_data(struct mtd_info *mtd, const u8 *buf, u32 length)
{
	u32 timeout = 0xFFFF;
	u32 i;
	u32 *pBuf32;

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	mb();
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	pBuf32 = (u32 *) buf;

	if ((unsigned long)buf % 4 || length % 4)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

	if ((unsigned long)buf % 4 || length % 4) {
		for (i = 0; (i < (length)) && (timeout > 0);) {
			if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1) {
				DRV_WriteReg32(NFI_DATAW_REG32, *buf++);
				i++;
			} else {
				timeout--;
			}
			if (timeout == 0) {
				nand_pr_err("timeout");
				dump_nfi();
				return false;
			}
		}
	} else {
		for (i = 0; (i < (length >> 2)) && (timeout > 0);) {
			/* if (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) <= 12) */
			if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1) {
				DRV_WriteReg32(NFI_DATAW_REG32, *pBuf32++);
				i++;
			} else {
				timeout--;
			}
			if (timeout == 0) {
				nand_pr_err("timeout");
				dump_nfi();
				return false;
			}
		}
	}

	return true;
}

/******************************************************************************
 * mtk_nand_read_fdm_data
 *
 * DESCRIPTION:
 *	 Read a fdm data !
 *
 * PARAMETERS:
 *	 u8* pDataBuf, u32 u4SecNum
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_read_fdm_data(u8 *pDataBuf, u32 u4SecNum)
{
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
	u32 i;
	u32 *pBuf32 = (u32 *) pDataBuf;

	if (pBuf32) {
		for (i = 0; i < u4SecNum; ++i) {
			*pBuf32++ = DRV_Reg32(NFI_FDM0L_REG32 + (i << 1));
			*pBuf32++ = DRV_Reg32(NFI_FDM0M_REG32 + (i << 1));
		}
	}
#else
	u32 fdm_temp[2];
	u32 i, j;
	u8 *byte_ptr;

	byte_ptr = (u8 *)fdm_temp;
	if (pDataBuf) {
		for (i = 0; i < u4SecNum; ++i) {
			fdm_temp[0] = DRV_Reg32(NFI_FDM0L_REG32 + (i << 1));
			fdm_temp[1] = DRV_Reg32(NFI_FDM0M_REG32 + (i << 1));
			for (j = 0; j < host->hw->nand_fdm_size; j++)
				*(pDataBuf + (i * host->hw->nand_fdm_size) + j) = *(byte_ptr + j);
		}
	}
#endif
}

/******************************************************************************
 * mtk_nand_write_fdm_data
 *
 * DESCRIPTION:
 *	 Write a fdm data !
 *
 * PARAMETERS:
 *	 u8* pDataBuf, u32 u4SecNum
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static u8 fdm_buf[128];
static void mtk_nand_write_fdm_data(struct nand_chip *chip, u8 *pDataBuf, u32 u4SecNum)
{
	u32 i, j;
#ifndef CONFIG_MNTL_SUPPORT
	u8 checksum = 0;
	bool empty = true;
	struct nand_oobfree *free_entry;
#endif
	u8 *pBuf;
	u8 *byte_ptr;
	u32 fdm_data[2];

	memcpy(fdm_buf, pDataBuf, u4SecNum * host->hw->nand_fdm_size);

/* Disable fdm checksum since mntl need full fdm area */
#ifndef CONFIG_MNTL_SUPPORT

	free_entry = chip->ecc.layout->oobfree;
	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free_entry[i].length; i++) {
		for (j = 0; j < free_entry[i].length; j++) {
			if (pDataBuf[free_entry[i].offset + j] != 0xFF)
				empty = false;
			checksum ^= pDataBuf[free_entry[i].offset + j];
		}
	}

	if (!empty)
		fdm_buf[free_entry[i - 1].offset + free_entry[i - 1].length] = checksum;
#endif

	pBuf = (u8 *)fdm_data;
	byte_ptr = (u8 *)fdm_buf;

	for (i = 0; i < u4SecNum; ++i) {
		fdm_data[0] = 0xFFFFFFFF;
		fdm_data[1] = 0xFFFFFFFF;

		for (j = 0; j < host->hw->nand_fdm_size; j++)
			*(pBuf + j) = *(byte_ptr + j + (i * host->hw->nand_fdm_size));

		DRV_WriteReg32(NFI_FDM0L_REG32 + (i << 1), fdm_data[0]);
		DRV_WriteReg32(NFI_FDM0M_REG32 + (i << 1), fdm_data[1]);
	}
}

/******************************************************************************
 * mtk_nand_stop_read
 *
 * DESCRIPTION:
 *	 Stop read operation !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_stop_read(void)
{
	NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BRD);
	mtk_nand_reset();
	if (g_bHwEcc)
		ECC_Decode_End();
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

/******************************************************************************
 * mtk_nand_stop_write
 *
 * DESCRIPTION:
 *	 Stop write operation !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_stop_write(void)
{
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	u32 reg_val;
#endif

	NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BWR);
	if (g_bHwEcc)
		ECC_Encode_End();
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		reg_val = DRV_Reg16(NFI_DEBUG_CON1_REG16);
		reg_val &= (~0x4000);
		DRV_WriteReg16(NFI_DEBUG_CON1_REG16, reg_val);
	}
#endif
}

/* --------------------------------------------------------------------------- */
#define STATUS_READY			(0x40)
#define STATUS_FAIL				(0x01)
#define STATUS_WR_ALLOW			(0x80)

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
static bool mtk_nand_read_status(void)
{
	int status = 0;
	unsigned int timeout;

	mtk_nand_reset();

	/* Disable HW ECC */
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

	/* Disable 16-bit I/O */
	NFI_CLN_REG32(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_OP_SRD | CNFG_READ_EN | CNFG_BYTE_RW);

	DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD | (1 << CON_NFI_NOB_SHIFT));

	DRV_WriteReg32(NFI_CON_REG16, 0x3);
	mtk_nand_set_mode(CNFG_OP_SRD);
	DRV_WriteReg16(NFI_CNFG_REG16, 0x2042);
	mtk_nand_set_command(NAND_CMD_STATUS);
	DRV_WriteReg32(NFI_CON_REG16, 0x90);

	timeout = TIMEOUT_4;
	WAIT_NFI_PIO_READY(timeout);

	if (timeout)
		status = (DRV_Reg16(NFI_DATAR_REG32));

	/*~  clear NOB */
	DRV_WriteReg32(NFI_CON_REG16, 0);

	if (devinfo.iowidth == 16) {
		NFI_SET_REG32(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	}

	/* flash is ready now, check status code */
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
			&& (devinfo.tlcControl.slcopmodeEn)) {
		/*pr_warn("status 0x%x", status);*/
		if (SLC_MODE_OP_FALI & status) {
			if (!(STATUS_WR_ALLOW & status))
				pr_warn("status locked\n");
			else
				pr_warn("status unknown\n");

			return FALSE;
		}
		return TRUE;
	}
	if (STATUS_FAIL & status)
		return FALSE;
	return TRUE;
}
#endif

bool mtk_nand_SetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value, u8 bytes)
{
	u16 reg_val = 0;
	u8 write_count = 0;
	u32 reg = 0;
	u32 timeout = TIMEOUT_3;	/* 0xffff; */
	/* u32			 status; */
	/* struct nand_chip *chip = (struct nand_chip *)mtd->priv; */

	mtk_nand_reset();

	reg = DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32);
	if (!(reg & TYPE_SLC))
		bytes <<= 1;

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(cmd);
	mtk_nand_set_address(addr, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	/* pr_debug("Bytes=%d\n", bytes); */
	while ((write_count < bytes) && timeout) {
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0)
			break;
		if (reg & TYPE_SLC) {
			/* pr_debug("VALUE1:0x%2X\n", *value); */
			DRV_WriteReg8(NFI_DATAW_REG32, *value++);
		} else if (write_count % 2) {
			/* pr_debug("VALUE2:0x%2X\n", *value); */
			DRV_WriteReg8(NFI_DATAW_REG32, *value++);
		} else {
			/* pr_debug("VALUE3:0x%2X\n", *value); */
			DRV_WriteReg8(NFI_DATAW_REG32, *value);
		}
		write_count++;
		timeout = TIMEOUT_3;
	}
	*NFI_CNRNB_REG16 = 0x81;
	if (!mtk_nand_status_ready(STA_NAND_BUSY_RETURN))
		return FALSE;
	/* mtk_nand_read_status(); */
	/* if(status& 0x1) */
	/* return FALSE; */
	return TRUE;
}

bool mtk_nand_GetFeature(struct mtd_info *mtd, u16 cmd, u32 addr, u8 *value, u8 bytes)
{
	u16 reg_val = 0;
	u8 read_count = 0;
	u32 timeout = TIMEOUT_3;	/* 0xffff; */
	/* struct nand_chip *chip = (struct nand_chip *)mtd->priv; */

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW | CNFG_READ_EN);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(cmd);
	mtk_nand_set_address(addr, 0, 1, 0);
	mtk_nand_status_ready(STA_NFI_OP_MASK);
	*NFI_CNRNB_REG16 = 0x81;
	mtk_nand_status_ready(STA_NAND_BUSY_RETURN);

	/* DRV_WriteReg32(NFI_CON_REG16, 0 << CON_NFI_SEC_SHIFT); */
	reg_val = DRV_Reg32(NFI_CON_REG16);
	reg_val &= ~CON_NFI_NOB_MASK;
	reg_val |= ((4 << CON_NFI_NOB_SHIFT) | CON_NFI_SRD);
	DRV_WriteReg32(NFI_CON_REG16, reg_val);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	/* bytes = 20; */
	while ((read_count < bytes) && timeout) {
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0)
			break;
		*value++ = DRV_Reg8(NFI_DATAR_REG32);
		/* pr_debug("Value[0x%02X]\n", DRV_Reg8(NFI_DATAR_REG32)); */
		read_count++;
		timeout = TIMEOUT_3;
	}
	/* chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1); */
	/* mtk_nand_read_status(); */
	if (timeout != 0)
		return TRUE;
	else
		return FALSE;

}

#if 1
const u8 data_tbl[8][5] = {
	{0x04, 0x04, 0x7C, 0x7E, 0x00},
	{0x00, 0x7C, 0x78, 0x78, 0x00},
	{0x7C, 0x76, 0x74, 0x72, 0x00},
	{0x08, 0x08, 0x00, 0x00, 0x00},
	{0x0B, 0x7E, 0x76, 0x74, 0x00},
	{0x10, 0x76, 0x72, 0x70, 0x00},
	{0x02, 0x7C, 0x7E, 0x70, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00}
};

const u8 data_tbl_15nm[11][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00},
	{0x02, 0x04, 0x02, 0x00, 0x00},
	{0x7C, 0x00, 0x7C, 0x7C, 0x00},
	{0x7A, 0x00, 0x7A, 0x7A, 0x00},
	{0x78, 0x02, 0x78, 0x7A, 0x00},
	{0x7E, 0x04, 0x7E, 0x7A, 0x00},
	{0x76, 0x04, 0x76, 0x78, 0x00},
	{0x04, 0x04, 0x04, 0x76, 0x00},
	{0x06, 0x0A, 0x06, 0x02, 0x00},
	{0x74, 0x7C, 0x74, 0x76, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00}
};

static void mtk_nand_modeentry_rrtry(void)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5C);
	mtk_nand_set_command(0xC5);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_rren_rrtry(bool needB3)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if (needB3)
		mtk_nand_set_command(0xB3);
	mtk_nand_set_command(0x26);
	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_rren_15nm_rrtry(bool flag)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if (flag)
		mtk_nand_set_command(0x26);
	else
		mtk_nand_set_command(0xCD);

	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_sprmset_rrtry(u32 addr, u32 data)/* single parameter setting */
{
	u16 reg_val = 0;
	/* u8 write_count = 0; */
	/* u32 reg = 0; */
	u32 timeout = TIMEOUT_3;	/* 0xffff; */

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(0x55);
	mtk_nand_set_address(addr, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);


	WAIT_NFI_PIO_READY(timeout);
	timeout = TIMEOUT_3;
	DRV_WriteReg8(NFI_DATAW_REG32, data);

	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;
}

static void mtk_nand_toshiba_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 retryCount,
				   bool defValue)
{
	u32 acccon;
	u8 cnt = 0;
	u8 add_reg[6] = { 0x04, 0x05, 0x06, 0x07, 0x0D };

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C08669);	/* to fit read retry timing */

	/* pr_debug("%s %d retryCount:%d\n", __func__, __LINE__, retryCount); */

	if (retryCount == 0)
		mtk_nand_modeentry_rrtry();

	for (cnt = 0; cnt < 5; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], data_tbl[retryCount][cnt]);

	if (retryCount == 3)
		mtk_nand_rren_rrtry(TRUE);
	else if (retryCount < 6)
		mtk_nand_rren_rrtry(FALSE);

	if (retryCount == 7) {	/* to exit */
		mtk_nand_device_reset();
		mtk_nand_reset();
		/* should do NAND DEVICE interface change under sync mode */
	}

	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);
}

static void mtk_nand_toshiba_15nm_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 retryCount, bool defValue)
{
	u32 acccon;
	u8 add_reg[6] = {0x04, 0x05, 0x06, 0x07, 0x0D };
	u8 cnt = 0;

	/* pr_debug("%s %d retryCount:%d\n", __func__, __LINE__, retryCount); */

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C08669); /* to fit read retry timing */

	if (retryCount == 0)
		mtk_nand_modeentry_rrtry();

	for (cnt = 0; cnt < 5; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], data_tbl_15nm[retryCount][cnt]);

	if (retryCount == 10) {	/* to exit */
		mtk_nand_device_reset();
		mtk_nand_reset();
	} else {
		if (retryCount == 0)
			mtk_nand_rren_15nm_rrtry(TRUE);
		else
			mtk_nand_rren_15nm_rrtry(FALSE);
	}

	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);
}

const u8 tb_slc_1y[8][2] = {
	{0xF0, 0x00},
	{0xE0, 0x00},
	{0xD0, 0x00},
	{0xC0, 0x00},
	{0x20, 0x00},
	{0x30, 0x00},
	{0x40, 0x00},
	{0x00, 0x00}
};


const u8 tb_tlc_1y[31][7] = {
		{0xFE, 0x03, 0x02, 0x02, 0xFF, 0xFC, 0xFD},
		{0xFE, 0x02, 0x01, 0x01, 0xFE, 0xFA, 0xFB},
		{0xFE, 0x00, 0x00, 0xFF, 0xFC, 0xF8, 0xF9},
		{0xFD, 0xFF, 0xFE, 0xFE, 0xFA, 0xF6, 0xF7},
		{0xFD, 0xFE, 0xFD, 0xFC, 0xF8, 0xF4, 0xF5},
		{0xFD, 0xFD, 0xFC, 0xFB, 0xF6, 0xF2, 0xF2},
		{0xFD, 0xFB, 0xFB, 0xF9, 0xF5, 0xF0, 0xF0},
		{0xFD, 0xFA, 0xF9, 0xF8, 0xF3, 0xEE, 0xEE},
		{0xFD, 0xF9, 0xF8, 0xF6, 0xF1, 0xEC, 0xEC},
		{0xFD, 0xF8, 0xF7, 0xF5, 0xEF, 0xEA, 0xE9},
		{0xFC, 0xF6, 0xF6, 0xF3, 0xEE, 0xE8, 0xE7},
		{0xFA, 0xFA, 0xFB, 0xFA, 0xFB, 0xFA, 0xFA},
		{0xFA, 0xFA, 0xFA, 0xF9, 0xFA, 0xF8, 0xF8},
		{0xFA, 0xFA, 0xFA, 0xF8, 0xF9, 0xF6, 0xF5},
		{0xFB, 0xFA, 0xF9, 0xF7, 0xF7, 0xF4, 0xF3},
		{0xFB, 0xFB, 0xF9, 0xF6, 0xF6, 0xF2, 0xF0},
		{0xFB, 0xFB, 0xF8, 0xF5, 0xF5, 0xF0, 0xEE},
		{0xFB, 0xFB, 0xF8, 0xF5, 0xF4, 0xEE, 0xEB},
		{0xFC, 0xFB, 0xF7, 0xF4, 0xF2, 0xEC, 0xE9},
		{0xFC, 0xFE, 0xFE, 0xF9, 0xFA, 0xF8, 0xF8},
		{0xFD, 0xFE, 0xFD, 0xF7, 0xF7, 0xF4, 0xF3},
		{0xFD, 0xFF, 0xFC, 0xF5, 0xF5, 0xF0, 0xEE},
		{0xFE, 0x03, 0x03, 0x04, 0x01, 0xFF, 0x01},
		{0xFC, 0x00, 0x00, 0x01, 0xFE, 0xFC, 0xFE},
		{0xFA, 0xFA, 0xFC, 0xFC, 0xFA, 0xF7, 0xFA},
		{0x00, 0x03, 0x02, 0x03, 0xFF, 0xFC, 0xFE},
		{0x04, 0x03, 0x03, 0x03, 0x00, 0xFC, 0xFD},
		{0x08, 0x04, 0x03, 0x04, 0x00, 0xFC, 0xFC},
		{0xFC, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
		{0xF8, 0x00, 0x00, 0x00, 0x08, 0x08, 0x10},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void mtk_nand_modeentry_tlc_rrtry(void)
{
	u32	reg_val = 0;
	u32	timeout = TIMEOUT_3;

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5C);
	mtk_nand_set_command(0xC5);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(0x55);
	mtk_nand_set_address(0, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
	DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);


	WAIT_NFI_PIO_READY(timeout);
	timeout = TIMEOUT_3;
	DRV_WriteReg8(NFI_DATAW_REG32, 0x01);

	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;
}


static void mtk_nand_toshiba_tlc_1y_rrtry(flashdev_info_t deviceinfo, u32 retryCount, bool defValue)
{
	u8 add_reg[7] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
	u8 cnt = 0;

	if (defValue == TRUE) {
		for (cnt = 0; cnt < 7; cnt++)
			mtk_nand_sprmset_rrtry(add_reg[cnt], tb_tlc_1y[30][cnt]);
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND(NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
		return;
	}

	if (retryCount == 0)
		mtk_nand_modeentry_tlc_rrtry();

	for (cnt = 0; cnt < 7; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], tb_tlc_1y[retryCount][cnt]);

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if (retryCount == 31)
		mtk_nand_set_command(0xB3);
	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

}

static void mtk_nand_toshiba_slc_1y_rrtry(flashdev_info_t deviceinfo, u32 retryCount, bool defValue)
{
	u8 add_reg[2] = {0x0B, 0x0D};
	u8 cnt = 0;

	if (defValue == TRUE) {
		for (cnt = 0; cnt < 2; cnt++)
			mtk_nand_sprmset_rrtry(add_reg[cnt], tb_slc_1y[7][cnt]);
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND(NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
	}

	if (retryCount == 0)
		mtk_nand_modeentry_tlc_rrtry();

	for (cnt = 0; cnt < 2; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], tb_slc_1y[retryCount][cnt]);

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

}

static void mtk_nand_toshiba_tlc_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo,
				u32 retryCount, bool defValue)
{
	if (devinfo.tlcControl.slcopmodeEn)
		mtk_nand_toshiba_slc_1y_rrtry(deviceinfo, retryCount, defValue);
	else
		mtk_nand_toshiba_tlc_1y_rrtry(deviceinfo, retryCount, defValue);
}

#endif

static void mtk_nand_micron_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 feature,
				  bool defValue)
{
	/* u32 feature = deviceinfo.feature_set.FeatureSet.readRetryStart+retryCount; */
	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,
				deviceinfo.feature_set.FeatureSet.readRetryAddress,
				(u8 *) &feature, 4);
}

static int g_sandisk_retry_case;	/* for new read retry table case 1,2,3,4 */
static void mtk_nand_sandisk_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 feature,
				   bool defValue)
{
	/* u32 feature = deviceinfo.feature_set.FeatureSet.readRetryStart+retryCount; */
	if (defValue == FALSE) {
		mtk_nand_reset();
	} else {
		mtk_nand_device_reset();
		mtk_nand_reset();
		/* should do NAND DEVICE interface change under sync mode */
	}

	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,
				deviceinfo.feature_set.FeatureSet.readRetryAddress,
				(u8 *) &feature, 4);
	if (defValue == FALSE) {
		if (g_sandisk_retry_case > 1) {	/* case 3 */
			if (g_sandisk_retry_case == 3) {
				u32 timeout = TIMEOUT_3;

				mtk_nand_reset();
				DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));
				mtk_nand_set_command(0x5C);
				mtk_nand_set_command(0xC5);
				mtk_nand_set_command(0x55);
				mtk_nand_set_address(0x00, 0, 1, 0);	/* test mode entry */
				mtk_nand_status_ready(STA_NFI_OP_MASK);
				DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
				NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
				DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
				WAIT_NFI_PIO_READY(timeout);
				DRV_WriteReg8(NFI_DATAW_REG32, 0x01);
				while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN)
					   && (timeout--))
					;
				mtk_nand_reset();
				timeout = TIMEOUT_3;
				mtk_nand_set_command(0x55);
				/* changing parameter LMFLGFIX_NEXT = 1 to all die */
				mtk_nand_set_address(0x23, 0, 1, 0);
				mtk_nand_status_ready(STA_NFI_OP_MASK);
				DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
				NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
				DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
				WAIT_NFI_PIO_READY(timeout);
				DRV_WriteReg8(NFI_DATAW_REG32, 0xC0);
				while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN)
					&& (timeout--))
					;
				mtk_nand_reset();
				/* pr_debug("Case3# Set LMFLGFIX_NEXT=1\n"); */
			}
			mtk_nand_set_command(0x25);
			/* pr_debug("Case2#3# Set cmd 25\n"); */
		}
		mtk_nand_set_command(deviceinfo.feature_set.FeatureSet.readRetryPreCmd);
	}
}

u16 sandisk_19nm_rr_table[18] = {
	0x0000,
	0xFF0F, 0xEEFE, 0xDDFD, 0x11EE,	/* 04h[7:4] | 07h[7:4] | 04h[3:0] | 05h[7:4] */
	0x22ED, 0x33DF, 0xCDDE, 0x01DD,
	0x0211, 0x1222, 0xBD21, 0xAD32,
	0x9DF0, 0xBCEF, 0xACDC, 0x9CFF,
	0x0000			/* align */
};

static void sandisk_19nm_rr_init(void)
{
	u32 reg_val = 0;
	u32 count = 0;
	u32 timeout = 0xffff;
	/* u32 u4RandomSetting; */
	u32 acccon;

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C08669);	/* to fit read retry timing */

	mtk_nand_reset();

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);
	mtk_nand_set_command(0x3B);
	mtk_nand_set_command(0xB9);

	for (count = 0; count < 9; count++) {
		mtk_nand_set_command(0x53);
		mtk_nand_set_address((0x04 + count), 0, 1, 0);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, 0x00);
		mtk_nand_reset();
	}

	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);
}

static void sandisk_19nm_rr_loading(u32 retryCount, bool defValue)
{
	u32 reg_val = 0;
	u32 timeout = 0xffff;
	u32 acccon;
	u8 count;
	u8 cmd_reg[4] = { 0x4, 0x5, 0x7 };

	acccon = DRV_Reg32(NFI_ACCCON_REG32);
	DRV_WriteReg32(NFI_ACCCON_REG32, 0x31C08669);	/* to fit read retry timing */

	mtk_nand_reset();

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);

	if ((retryCount != 0) || defValue)
		mtk_nand_set_command(0xD6);

	mtk_nand_set_command(0x3B);
	mtk_nand_set_command(0xB9);
	for (count = 0; count < 3; count++) {
		mtk_nand_set_command(0x53);
		mtk_nand_set_address(cmd_reg[count], 0, 1, 0);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		if (count == 0)
			DRV_WriteReg32(NFI_DATAW_REG32,
					   (((sandisk_19nm_rr_table[retryCount] & 0xF000) >> 8) |
					((sandisk_19nm_rr_table[retryCount] & 0x00F0) >> 4)));
		else if (count == 1)
			DRV_WriteReg32(NFI_DATAW_REG32,
					   ((sandisk_19nm_rr_table[retryCount] & 0x000F) << 4));
		else if (count == 2)
			DRV_WriteReg32(NFI_DATAW_REG32,
					   ((sandisk_19nm_rr_table[retryCount] & 0x0F00) >> 4));

		mtk_nand_reset();
	}

	if (!defValue)
		mtk_nand_set_command(0xB6);

	DRV_WriteReg32(NFI_ACCCON_REG32, acccon);
}

static void mtk_nand_sandisk_19nm_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo,
					u32 retryCount, bool defValue)
{
	if ((retryCount == 0) && (!defValue))
		sandisk_19nm_rr_init();
	sandisk_19nm_rr_loading(retryCount, defValue);
}

#define HYNIX_RR_TABLE_SIZE  (1026)	/* hynix read retry table size */
#define SINGLE_RR_TABLE_SIZE (64)

#define READ_RETRY_STEP (devinfo.feature_set.FeatureSet.readRetryCnt + \
	devinfo.feature_set.FeatureSet.readRetryStart)	/* 8 step or 12 step to fix read retry table */
#define HYNIX_16NM_RR_TABLE_SIZE  ((READ_RETRY_STEP == 12)?(784):(528))	/* hynix read retry table size */
#define SINGLE_RR_TABLE_16NM_SIZE  ((READ_RETRY_STEP == 12)?(48):(32))

u8 nand_hynix_rr_table[(HYNIX_RR_TABLE_SIZE + 16) / 16 * 16];	/* align as 16 byte */

#define NAND_HYX_RR_TBL_BUF nand_hynix_rr_table

static u8 real_hynix_rr_table_idx;
static u32 g_hynix_retry_count;

static bool hynix_rr_table_select(u8 table_index, flashdev_info_t *deviceinfo)
{
	u32 i;
	u32 table_size = (deviceinfo->feature_set.FeatureSet.rtype ==
		 RTYPE_HYNIX_16NM) ? SINGLE_RR_TABLE_16NM_SIZE : SINGLE_RR_TABLE_SIZE;

	for (i = 0; i < table_size; i++) {
		u8 *temp_rr_table = (u8 *) NAND_HYX_RR_TBL_BUF + table_size * table_index * 2 + 2;
		u8 *temp_inversed_rr_table = (u8 *) NAND_HYX_RR_TBL_BUF + table_size * table_index * 2 + table_size + 2;

		if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
			temp_rr_table += 14;
			temp_inversed_rr_table += 14;
		}
		if (0xFF != (temp_rr_table[i] ^ temp_inversed_rr_table[i]))
			return FALSE;	/* error table */
	}
	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
		table_size += 16;
	else
		table_size += 2;
	for (i = 0; i < table_size; i++) {
		pr_debug("%02X ", NAND_HYX_RR_TBL_BUF[i]);
		if ((i + 1) % 8 == 0)
			pr_debug("\n");
	}
	return TRUE;		/* correct table */
}

static void HYNIX_RR_TABLE_READ(flashdev_info_t *deviceinfo)
{
	u32 reg_val = 0;
	u32 read_count = 0, max_count = HYNIX_RR_TABLE_SIZE;
	u32 timeout = 0xffff;
	u8 *rr_table = (u8 *) (NAND_HYX_RR_TBL_BUF);
	u8 table_index = 0;
	u8 add_reg1[3] = { 0xFF, 0xCC };
	u8 data_reg1[3] = { 0x40, 0x4D };
	u8 cmd_reg[6] = { 0x16, 0x17, 0x04, 0x19, 0x00 };
	u8 add_reg2[6] = { 0x00, 0x00, 0x00, 0x02, 0x00 };
	bool RR_TABLE_EXIST = TRUE;

	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
		read_count = 1;
		add_reg1[1] = 0x38;
		data_reg1[1] = 0x52;
		max_count = HYNIX_16NM_RR_TABLE_SIZE;
		if (READ_RETRY_STEP == 12)
			add_reg2[2] = 0x1F;
	}
	mtk_nand_device_reset();
	/* take care under sync mode. need change nand device inferface xiaolei */

	mtk_nand_reset();

	DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));

	mtk_nand_set_command(0x36);

	for (; read_count < 2; read_count++) {
		mtk_nand_set_address(add_reg1[read_count], 0, 1, 0);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, data_reg1[read_count]);
		mtk_nand_reset();
	}

	for (read_count = 0; read_count < 5; read_count++)
		mtk_nand_set_command(cmd_reg[read_count]);
	for (read_count = 0; read_count < 5; read_count++)
		mtk_nand_set_address(add_reg2[read_count], 0, 1, 0);
	mtk_nand_set_command(0x30);
	DRV_WriteReg(NFI_CNRNB_REG16, 0xF1);
	timeout = 0xffff;
	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW | CNFG_READ_EN);
	DRV_WriteReg(NFI_CNFG_REG16, reg_val);
	DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BRD | (2 << CON_NFI_SEC_SHIFT)));
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);
	timeout = 0xffff;
	read_count = 0;		/* how???? */
	while ((read_count < max_count) && timeout) {
		WAIT_NFI_PIO_READY(timeout);
		*rr_table++ = (unsigned char) DRV_Reg32(NFI_DATAR_REG32);
		read_count++;
		timeout = 0xFFFF;
	}

	mtk_nand_device_reset();
	/* take care under sync mode. need change nand device inferface xiaolei */

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
		DRV_WriteReg(NFI_CNFG_REG16, reg_val);
		mtk_nand_set_command(0x36);
		mtk_nand_set_address(0x38, 0, 1, 0);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, 0x00);
		mtk_nand_reset();
		mtk_nand_set_command(0x16);
		mtk_nand_set_command(0x00);
		mtk_nand_set_address(0x00, 0, 1, 0);	/* dummy read, add don't care */
		mtk_nand_set_command(0x30);
	} else {
		DRV_WriteReg(NFI_CNFG_REG16, reg_val);
		mtk_nand_set_command(0x38);
	}
	timeout = 0xffff;
	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;
	rr_table = (u8 *) (NAND_HYX_RR_TBL_BUF);
	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX) {
		if ((rr_table[0] != 8) || (rr_table[1] != 8)) {
			RR_TABLE_EXIST = FALSE;
			mtk_nand_assert(0);
		}
	} else if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
		for (read_count = 0; read_count < 8; read_count++) {
			if ((rr_table[read_count] != 8) || (rr_table[read_count + 8] != 4)) {
				RR_TABLE_EXIST = FALSE;
				break;
			}
		}
	}
	if (RR_TABLE_EXIST) {
		for (table_index = 0; table_index < 8; table_index++) {
			if (hynix_rr_table_select(table_index, deviceinfo)) {
				real_hynix_rr_table_idx = table_index;
				pr_debug("Hynix rr_tbl_id %d\n", real_hynix_rr_table_idx);
				break;
			}
		}
		if (table_index == 8)
			mtk_nand_assert(0);
	} else {
		nand_pr_err("Hynix RR table index error!");
	}
}

static void HYNIX_Set_RR_Para(u32 rr_index, flashdev_info_t *deviceinfo)
{
	/* u32 reg_val = 0; */
	u32 timeout = 0xffff;
	u8 count, max_count = 8;
	u8 add_reg[9] = { 0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF };
	u8 *hynix_rr_table =
		(u8 *) NAND_HYX_RR_TBL_BUF + SINGLE_RR_TABLE_SIZE * real_hynix_rr_table_idx * 2 + 2;
	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
		add_reg[0] = 0x38;	/* 0x38, 0x39, 0x3A, 0x3B */
		for (count = 1; count < 4; count++)
			add_reg[count] = add_reg[0] + count;
		hynix_rr_table += 14;
		max_count = 4;
	}
	mtk_nand_reset();

	DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));
	/* mtk_nand_set_command(0x36); */

	for (count = 0; count < max_count; count++) {
		mtk_nand_set_command(0x36);
		mtk_nand_set_address(add_reg[count], 0, 1, 0);
		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0) {
			pr_notice("HYNIX_Set_RR_Para timeout\n");
			break;
		}
		DRV_WriteReg32(NFI_DATAW_REG32, hynix_rr_table[rr_index * max_count + count]);
		mtk_nand_reset();
	}
	mtk_nand_set_command(0x16);
}

#if 0
static void HYNIX_Get_RR_Para(u32 rr_index, flashdev_info_t *deviceinfo)
{
	u32 reg_val = 0;
	u32 timeout = 0xffff;
	u8 count, max_count = 8;
	u8 add_reg[9] = { 0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF };
	u8 *hynix_rr_table =
		(u8 *) NAND_HYX_RR_TBL_BUF + SINGLE_RR_TABLE_SIZE * real_hynix_rr_table_idx * 2 + 2;
	if (deviceinfo->feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM) {
		add_reg[0] = 0x38;	/* 0x38, 0x39, 0x3A, 0x3B */
		for (count = 1; count < 4; count++)
			add_reg[count] = add_reg[0] + count;
		hynix_rr_table += 14;
		max_count = 4;
	}
	mtk_nand_reset();

	DRV_WriteReg(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW | CNFG_READ_EN));
	/* mtk_nand_set_command(0x37); */

	for (count = 0; count < max_count; count++) {
		mtk_nand_set_command(0x37);
		mtk_nand_set_address(add_reg[count], 0, 1, 0);

		DRV_WriteReg(NFI_CON_REG16, (CON_NFI_SRD | (1 << CON_NFI_NOB_SHIFT)));
		DRV_WriteReg(NFI_STRDATA_REG16, 1);

		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0)
			pr_notice("HYNIX_Get_RR_Para timeout\n");

		/* DRV_WriteReg32(NFI_DATAW_REG32, hynix_rr_table[rr_index*max_count + count]); */
		pr_debug("Get[%02X]%02X\n", add_reg[count], DRV_Reg8(NFI_DATAR_REG32));
		mtk_nand_reset();
	}
}
#endif

static void mtk_nand_hynix_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 retryCount,
				 bool defValue)
{
	if (defValue == FALSE) {
		if (g_hynix_retry_count == READ_RETRY_STEP)
			g_hynix_retry_count = 0;

		pr_debug("Hynix Retry %d\n", g_hynix_retry_count);
		HYNIX_Set_RR_Para(g_hynix_retry_count, &deviceinfo);
		/* HYNIX_Get_RR_Para(g_hynix_retry_count, &deviceinfo); */
		g_hynix_retry_count++;
	}
}

static void mtk_nand_hynix_16nm_rrtry(struct mtd_info *mtd, flashdev_info_t deviceinfo,
					  u32 retryCount, bool defValue)
{
	if (defValue == FALSE) {
		if (g_hynix_retry_count == READ_RETRY_STEP)
			g_hynix_retry_count = 0;

		pr_debug("Hynix 16nm Retry %d\n", g_hynix_retry_count);
		HYNIX_Set_RR_Para(g_hynix_retry_count, &deviceinfo);
		/* mb(); */
		/* HYNIX_Get_RR_Para(g_hynix_retry_count, &deviceinfo); */
		g_hynix_retry_count++;

	}
}

u32 special_rrtry_setting[37] = {
	0x00000000, 0x7C00007C, 0x787C0004, 0x74780078,
	0x7C007C08, 0x787C7C00, 0x74787C7C, 0x70747C00,
	0x7C007800, 0x787C7800, 0x74787800, 0x70747800,
	0x6C707800, 0x00040400, 0x7C000400, 0x787C040C,
	0x7478040C, 0x7C000810, 0x00040810, 0x04040C0C,
	0x00040C10, 0x00081014, 0x000C1418, 0x7C040C0C,
	0x74787478, 0x70747478, 0x6C707478, 0x686C7478,
	0x74787078, 0x70747078, 0x686C7078, 0x6C707078,
	0x6C706C78, 0x686C6C78, 0x64686C78, 0x686C6874,
	0x64686874,
};

/* sandisk 1z nm */
u32 sandisk_1znm_rrtry_setting[33] = {
	0x00000000, 0x00000404, 0x00007C7C, 0x7C7C0404,
	0x00000808, 0x7C7C0000, 0x7C7C0808, 0x04080404,
	0x04040000, 0x7C007C7C, 0x04080808, 0x787C040C,
	0x78780000, 0x00000C0C, 0x00007878, 0x04007C7C,
	0x7C747878, 0x78787C7C, 0x08040404, 0x04080C08,
	0x08080808, 0x78787878, 0x007C7C7C, 0x00040408,
	0x00080000, 0x00780804, 0x7C780C08, 0x7874087C,
	0x74787C7C, 0x74740000, 0x08000000, 0x747C7878,
	0x74700478,
};

u32 sandisk_1znm_rrtry_slc[25] = {
	0x00000000, 0x00000004, 0x0000007C, 0x00000008,
	0x00000078, 0x0000000C, 0x00000074, 0x00000010,
	0x00000070, 0x00000014, 0x0000006C, 0x00000018,
	0x00000068, 0x0000001C, 0x00000064, 0x00000020,
	0x00000060, 0x00000024, 0x0000005C, 0x00000028,
	0x00000058, 0x0000002C, 0x00000030, 0x00000034,
	0x00000038,
};

u32 sandisk_1znm_8GB_rrtry_setting[32] = {
	0x00000000, 0x78780404, 0x747C0404, 0x78040000,
	0x7C7C0404, 0x7C000000, 0x74000000, 0x78040808,
	0x78047C7C, 0x7C007C7C, 0x707C0404, 0x74747C7C,
	0x70780000, 0x78080C0C, 0x7C7C7878, 0x04080404,
	0x78087878, 0x70787C7C, 0x6C707878, 0x6C740000,
	0x74000808, 0x6C787C7C, 0x04040000, 0x6C747474,
	0x707C7878, 0x74000C0C, 0x080C0404, 0x747C7878,
	0x68707878, 0x70000808, 0x780C1010, 0x080C0000,
};

u32 sandisk_1znm_8GB_rrtry_slc[25] = {
	0x00000000, 0x00000004, 0x0000007C, 0x00000008,
	0x00000078, 0x0000000C, 0x00000074, 0x00000010,
	0x00000070, 0x00000014, 0x0000006C, 0x00000018,
	0x00000068, 0x0000001C, 0x00000064, 0x00000020,
	0x00000060, 0x00000024, 0x0000005C, 0x00000028,
	0x00000058, 0x0000002C, 0x00000030, 0x00000034,
	0x00000038,
};

u32 sandisk_tlc_rrtbl_12h[40] = {
	0x00000000, 0x08000004, 0x00000404, 0x04040408,
	0x08040408, 0x0004080C, 0x04040810, 0x0C0C0C00,
	0x0E0E0E00, 0x10101000, 0x12121200, 0x080808FC,
	0xFC08FCF8, 0x0000FBF6, 0x0408FBF4, 0xFEFCF8FA,
	0xFCF8F4EC, 0xF8F8F8EC, 0x0002FCE4, 0xFCFEFEFE,
	0xFFFC00FD, 0xFEFB00FC, 0xFEFAFEFA, 0xFDF9FDFA,
	0xFBF8FBFA, 0xF9F7FAF8, 0xF8F6F9F4, 0xF5F4F8F2,
	0xF4F2F6EE, 0xF0F0F4E8, 0xECECF0E6, 0x020400FA,
	0x00FEFFF8, 0xFEFEFDF6, 0xFDFDFCF4, 0xFBFCFCF2,
	0xF9FBFBF0, 0xF8F9F9EE, 0xF6F8F8ED, 0xF4F7F6EA,
};

u32 sandisk_tlc_rrtbl_13h[40] = {
	0x00000000, 0x00040800, 0x00080004, 0x00020404,
	0x00040800, 0x00080000, 0x00FC0000, 0x000C0C0C,
	0x000E0E0E, 0x00101010, 0x00141414, 0x000008FC,
	0x0004FCF8, 0x00FC00F6, 0x00FC0404, 0x00FCFE08,
	0x00FCFC00, 0x00F8F8FA, 0x000000F4, 0x00FAFC02,
	0x00F8FF00, 0x00F6FDFE, 0x00F4FBFC, 0x00F2F9FA,
	0x00F0F7F8, 0x00EEF5F6, 0x00ECF3F4, 0x00EAF1F2,
	0x00E8ECEE, 0x00E0E4E8, 0x00DAE0E2, 0x00000000,
	0x00FEFEFE, 0x00FBFCFC, 0x00F9FAFA, 0x00F7F8F8,
	0x00F5F6F6, 0x00F3F4F4, 0x00F1F2F2, 0x00EFF0EF,
};

u32 sandisk_tlc_rrtbl_14h[11] = {
	0x00000000, 0x00000010, 0x00000020, 0x00000030,
	0x00000040, 0x00000050, 0x00000060, 0x000000F0,
	0x000000E0, 0x000000D0, 0x000000C0,
};

u32 sandisk_1z_tlc_rrtbl_12h[47] = {
	0x00000000, 0x081000F8, 0xF8F0F8F8, 0x000000E8,
	0x08F80800, 0xF8F8F8F0, 0xF80808F0, 0x08080808,
	0xF800F000, 0x00000800, 0xF80000F8, 0x08F800F8,
	0x000808F8, 0x00F8F808, 0x0000F8F0, 0x00080008,
	0x00100010, 0xF8F000E8, 0xF80800F0, 0x00101008,
	0x0018F810, 0x08180810, 0x00101018, 0x00040000,
	0x00100808, 0x00081000, 0xF80000F8, 0xF8F8F8F0,
	0x00F004E8, 0x08F008E8, 0x08E810E0, 0x100008E0,
	0xF80004E0, 0x080004E0, 0x000004E0, 0x100004E8,
	0x0800FCE8, 0x0000FC00, 0x0000FC00, 0xF800FC00,
	0xF000FC00, 0xF000F000, 0xF000F800, 0xE800F000,
	0xE800E800, 0xF000E800, 0xE000E800,
};

u32 sandisk_1z_tlc_rrtbl_13h[47] = {
	0x00000000, 0x00F80008, 0x00F8F8F8, 0x00E0F0F8,
	0x00000008, 0x00E8F000, 0x0000F0F0, 0x00E8F8F8,
	0x00F8F8F0, 0x00E800F8, 0x0008F8E8, 0x00F8F8F0,
	0x00F8F800, 0x00F0F800, 0x00F000F8, 0x00F0F8F0,
	0x000008F8, 0x00F0F0F0, 0x000800E8, 0x0008F808,
	0x0000FC08, 0x00F8FC00, 0x00F00000, 0x00FCFCFC,
	0x0010FC10, 0x0010FC10, 0x0010FC10, 0x0008FC08,
	0x00000008, 0x00E80800, 0x00E808F8, 0x00001000,
	0x0000F8F0, 0x000008E8, 0x0000FCE0, 0x000010E8,
	0x000008E0, 0x00000000, 0x0000FC00, 0x0000F800,
	0x0000F000, 0x0000F000, 0x0000F000, 0x0000E800,
	0x0000E800, 0x0000F000, 0x0000E000,
};

u32 sandisk_1z_tlc_rrtbl_14h[25] = {
	0x00000000, 0x00000008, 0x000000F8, 0x00000010,
	0x000000F0, 0x00000018, 0x000000E8, 0x00000020,
	0x000000E0, 0x00000028, 0x000000D8, 0x00000030,
	0x000000D0, 0x00000038, 0x000000C8, 0x00000040,
	0x000000C0, 0x00000048, 0x000000B8, 0x00000050,
	0x000000B0, 0x00000058, 0x00000060, 0x00000068,
	0x00000070,
};

/* process : 1 1ynm TLC 2 1znm TLC */
static void mtk_nand_sandisk_tlc_rrtry(struct mtd_info *mtd,
				flashdev_info_t deviceinfo, u32 feature, bool defValue, int process)
{
	u16 reg_val = 0;
	u32	timeout = TIMEOUT_3;
	u32 value1, value2, value3;

	if ((feature > 1) || defValue) {
		mtk_nand_reset();
		reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

		mtk_nand_set_command(0x55);
		mtk_nand_set_address(0, 0, 1, 0);
		DRV_WriteReg32(NFI_CON_REG16, 1 << CON_NFI_SEC_SHIFT);
		NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BWR);
		DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0)
			nand_pr_err("mtk_nand_sandisk_tlc_1ynm_rrtry: timeout");

		DRV_WriteReg32(NFI_DATAW_REG32, 0);

		mtk_nand_device_reset();
	}

	if (devinfo.tlcControl.slcopmodeEn) {
		if (process == 1)
			value3 = sandisk_tlc_rrtbl_14h[feature];
		else if (process == 2)
			value3 = sandisk_1z_tlc_rrtbl_14h[feature];
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x14, (u8 *)&value3, 4);
	} else {
		if (process == 1) {
			value1 = sandisk_tlc_rrtbl_12h[feature];
			value2 = sandisk_tlc_rrtbl_13h[feature];
		} else if (process == 2) {
			value1 = sandisk_1z_tlc_rrtbl_12h[feature];
			value2 = sandisk_1z_tlc_rrtbl_13h[feature];
		}
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x12, (u8 *)&value1, 4);
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x13, (u8 *)&value2, 4);
	}

	if (defValue == FALSE) {
		mtk_nand_reset();
		reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

		mtk_nand_set_command(0x5D);

		mtk_nand_reset();
	}
}

static void mtk_nand_sandisk_tlc_1ynm_rrtry(struct mtd_info *mtd,
					flashdev_info_t deviceinfo, u32 feature, bool defValue)
{
	mtk_nand_sandisk_tlc_rrtry(mtd, deviceinfo, feature, defValue, 1);
}

static void mtk_nand_sandisk_tlc_1znm_rrtry(struct mtd_info *mtd,
					flashdev_info_t deviceinfo, u32 feature, bool defValue)
{
	mtk_nand_sandisk_tlc_rrtry(mtd, deviceinfo, feature, defValue, 2);
}

static u32 mtk_nand_rrtry_setting(flashdev_info_t deviceinfo, enum readRetryType type,
				  u32 retryStart, u32 loopNo)
{
	u32 value;
	/* if(RTYPE_MICRON == type || RTYPE_SANDISK== type || RTYPE_TOSHIBA== type || RTYPE_HYNIX== type) */
	{
		if (retryStart == 0xFFFFFFFE) {
			/* sandisk 1znm 16GB MLC */
			if (devinfo.tlcControl.slcopmodeEn)
				value = sandisk_1znm_rrtry_slc[loopNo];
			else
				value = sandisk_1znm_rrtry_setting[loopNo];
		} else if (retryStart == 0xFFFFFFFD) {
			/* sandisk 1znm 8GB MLC */
			if (devinfo.tlcControl.slcopmodeEn)
				value = sandisk_1znm_8GB_rrtry_slc[loopNo];
			else
				value = sandisk_1znm_8GB_rrtry_setting[loopNo];
		} else if (retryStart != 0xFFFFFFFF)
			value = retryStart + loopNo;
		else
			value = special_rrtry_setting[loopNo];
	}

	return value;
}

typedef void (*rrtryFunctionType) (struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 feature,
				   bool defValue);

static rrtryFunctionType rtyFuncArray[] = {
	mtk_nand_micron_rrtry,
	mtk_nand_sandisk_rrtry,
	mtk_nand_sandisk_19nm_rrtry,
	mtk_nand_toshiba_rrtry,
	mtk_nand_toshiba_15nm_rrtry,
	mtk_nand_hynix_rrtry,
	mtk_nand_hynix_16nm_rrtry,
	mtk_nand_sandisk_tlc_1ynm_rrtry,
	mtk_nand_toshiba_tlc_rrtry,
	mtk_nand_sandisk_tlc_1znm_rrtry
};


static void mtk_nand_rrtry_func(struct mtd_info *mtd, flashdev_info_t deviceinfo, u32 feature,
				bool defValue)
{
	rtyFuncArray[deviceinfo.feature_set.FeatureSet.rtype] (mtd, deviceinfo, feature, defValue);
}
static u16 mtk_nand_get_prog_cmd(enum NFI_TLC_WL_PRE wl_pre)
{
	u16 cmd = NAND_CMD_PAGEPROG;

	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) &&
		devinfo.tlcControl.normaltlc) {
		if ((devinfo.tlcControl.pPlaneEn) && tlc_lg_left_plane) {
			cmd = PROGRAM_LEFT_PLANE_CMD;
		} else {
			if (devinfo.tlcControl.slcopmodeEn) {
				cmd = ((devinfo.two_phyplane) && (!tlc_snd_phyplane)) ?
					PROGRAM_LEFT_PLANE_CMD :  NAND_CMD_PAGEPROG;
			} else if (wl_pre == WL_HIGH_PAGE) {
				if ((devinfo.two_phyplane || (devinfo.advancedmode & MULTI_PLANE))
					&& (!tlc_snd_phyplane))
					cmd = PROGRAM_LEFT_PLANE_CMD;
				else
					cmd = tlc_cache_program ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG;
			} else {
				if ((devinfo.two_phyplane || (devinfo.advancedmode & MULTI_PLANE))
					&& (!tlc_snd_phyplane))
					cmd = PROGRAM_LEFT_PLANE_CMD;
				else
					cmd = PROGRAM_RIGHT_PLANE_CMD;
			}
		}
	} else if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
		if ((devinfo.two_phyplane || (devinfo.advancedmode & MULTI_PLANE))
			&& (!tlc_snd_phyplane)) {
			if (((devinfo.tlcControl.slcopmodeEn) && (devinfo.advancedmode & MULTI_PLANE))
				|| (is_raw_part == TRUE)) {
				cmd = NAND_CMD_PAGEPROG;
				is_raw_part = FALSE;
			} else {
				cmd = PROGRAM_LEFT_PLANE_CMD;
			}
		} else if (tlc_cache_program)
			cmd = NAND_CMD_CACHEDPROG;
		else
			cmd = NAND_CMD_PAGEPROG;
	}

	return cmd;
}
static void mtk_nand_handle_write_ahb_done(void)
{
	u16 cmd = NAND_CMD_PAGEPROG;

	g_running_dma = 0;
	dma_unmap_sg(mtk_dev, &mtk_sg, 1, DMA_TO_DEVICE);
	cmd = host->wb_cmd;
	if (cmd == NAND_CMD_PAGEPROG)
		NFI_SET_REG16(NFI_INTR_EN_REG16, INTR_BSY_RTN_EN);
	mtk_nand_set_command(cmd);
}

/******************************************************************************
 * mtk_nand_exec_read_page
 *
 * DESCRIPTION:
 *	 Read a page data !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize,
 *	 u8* pPageBuf, u8* pFDMBuf
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
int mtk_nand_exec_read_page_single(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				u8 *pFDMBuf)
{
	u8 *buf;
	int bRet = ERR_RTN_SUCCESS;
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
	u32 backup_corrected, backup_failed;
	bool readRetry = FALSE;
	int retryCount = 0;
	u32 retrytotalcnt = devinfo.feature_set.FeatureSet.readRetryCnt;
	/* u32 val; */
	u32 tempBitMap;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	bool tlc_left_plane = TRUE;
	unsigned int phys = 0;
#endif
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	u32 reg_val = 0;
	u32 real_row_addr = 0;
	u32 logical_plane_num = 1;
	u32 data_sector_num = 0;
	u8 *temp_byte_ptr = NULL;
	u8 *spare_ptr = NULL;
	u32 page_per_block = 0;
	u32 block_addr = 0;
	u32 page_in_block = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];
#endif
#if 0
	u32 bitMap, i;
#endif

#ifdef NAND_PFM
	struct timeval pfm_time_read;
#endif
#if 0
	unsigned short PageFmt_Reg = 0;
	unsigned int NAND_ECC_Enc_Reg = 0;
	unsigned int NAND_ECC_Dec_Reg = 0;
#endif
	PFM_BEGIN(pfm_time_read);
	tempBitMap = 0;

	page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	/* flush_icache_range(pPageBuf, (pPageBuf + u4PageSize));//flush_cache_all();//cache flush */

	if (pPageBuf == NULL) {
		nand_pr_err("pPageBuf is NULL!!!");
		return -EIO;
	}

	if (((unsigned long)pPageBuf % 16) && local_buffer_16_align) {
		buf = local_buffer_16_align;
		pr_debug("[%s] read buf (1) 0x%lx\n", __func__, (unsigned long)buf);
	} else {
		/* It should be allocated by vmalloc */
		if ((virt_addr_valid(pPageBuf) == 0) && local_buffer_16_align) {
			buf = local_buffer_16_align;
			pr_debug("[%s] read buf (2) 0x%lx\n", __func__, (unsigned long)buf);
		} else {
			buf = pPageBuf;
			/* pr_debug("[xl] read buf (3) 0x%lx\n",(unsigned long)buf); */
		}
	}
	backup_corrected = mtd->ecc_stats.corrected;
	backup_failed = mtd->ecc_stats.failed;


#if CFG_2CS_NAND
	if (g_bTricky_CS)
		u4RowAddr = mtk_nand_cs_on(nand, NFI_TRICKY_CS, u4RowAddr);
#endif

	do {
		data_sector_num = u4SecNum;
		temp_byte_ptr = buf;
		spare_ptr = pFDMBuf;
		logical_plane_num = 1;
		tlc_wl_info.wl_pre = WL_LOW_PAGE; /* init for build warning*/
		tlc_wl_info.word_line_idx = u4RowAddr;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
			if (devinfo.tlcControl.normaltlc) {
				NFI_TLC_GetMappedWL(u4RowAddr, &tlc_wl_info);
				real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);

				if (devinfo.tlcControl.pPlaneEn) {
					tlc_left_plane = TRUE;
					logical_plane_num = 2;
					data_sector_num /= 2;
					real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
				}
				/* pr_debug("mtk_nand_exec_read_page, u4RowAddr: 0x%x real_row_addr 0x%x %d\n",
				 * u4RowAddr, real_row_addr, devinfo.tlcControl.slcopmodeEn);
				 */
			} else {
				real_row_addr = NFI_TLC_GetRowAddr(u4RowAddr);
			}

			if (devinfo.tlcControl.slcopmodeEn) {
				if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
					/* pr_info("%s %d Read SLC Mode real_row_addr:0x%x\n", */
						/* __func__, __LINE__, real_row_addr); */
				}
			} else {
				if (devinfo.tlcControl.normaltlc) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					if (tlc_wl_info.wl_pre == WL_LOW_PAGE)
						mtk_nand_set_command(LOW_PG_SELECT_CMD);
					else if (tlc_wl_info.wl_pre == WL_MID_PAGE)
						mtk_nand_set_command(MID_PG_SELECT_CMD);
					else if (tlc_wl_info.wl_pre == WL_HIGH_PAGE)
						mtk_nand_set_command(HIGH_PG_SELECT_CMD);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
				}
			}
			reg_val = 0;
		} else
#endif
			real_row_addr = u4RowAddr;

	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
		if (devinfo.tlcControl.slcopmodeEn) {
			if (devinfo.vendor == VEND_MICRON) {
#if MLC_MICRON_SLC_MODE
				feature[0] = 0x00;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
#else
				block_addr = real_row_addr/page_per_block;
				page_in_block = real_row_addr % page_per_block;
				page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
				real_row_addr = page_in_block + block_addr * page_per_block;
#endif
			} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
				mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);

				if (devinfo.vendor == VEND_SANDISK) {
					block_addr = real_row_addr/page_per_block;
					page_in_block = real_row_addr % page_per_block;
					page_in_block <<= 1;
					real_row_addr = page_in_block + block_addr * page_per_block;
					/* pr_warn("mtk_nand_exec_read_page SLC Mode real_row_addr:%d, u4RowAddr:%d\n",
					 * real_row_addr, u4RowAddr);
					 */
				}
			}
		}
	}

	if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);
	else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);

	if (mtk_nand_ready_for_read(nand, real_row_addr, 0, data_sector_num, true, buf, NORMAL_READ)) {
		while (logical_plane_num) {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn) {
					if (devinfo.tlcControl.pPlaneEn)
						real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
#if 1
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
#endif

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(CHANGE_COLUNM_ADDR_1ST_CMD);
					mtk_nand_set_address(0, real_row_addr, 2, devinfo.addr_cycle - 2);
					mtk_nand_set_command(CHANGE_COLUNM_ADDR_2ND_CMD);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val |= CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_READ;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
				}
			}

			mtk_dir = DMA_FROM_DEVICE;
			sg_init_one(&mtk_sg, temp_byte_ptr, (data_sector_num * (1 << host->hw->nand_sec_shift)));
			dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
			phys = mtk_sg.dma_address;
#if __INTERNAL_USE_AHB_MODE__
			if (!phys)
				nand_pr_err("convert virt addr (%p) to phys add (%x)fail!!!", temp_byte_ptr, phys);
			else
				DRV_WriteReg32(NFI_STRADDR_REG32, phys);
#endif

			DRV_WriteReg32(NFI_CON_REG16, data_sector_num << CON_NFI_SEC_SHIFT);

			if (g_bHwEcc)
				ECC_Decode_Start();
#endif
			if (!mtk_nand_read_page_data(mtd, temp_byte_ptr,
				data_sector_num * (1 << host->hw->nand_sec_shift))) {
				nand_pr_err("mtk_nand_read_page_data fail");
				bRet = ERR_RTN_FAIL;
			}
			if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
				nand_pr_err("mtk_nand_status_ready fail");
				bRet = ERR_RTN_FAIL;
			}
			if (g_bHwEcc) {
				if (!mtk_nand_check_dececc_done(data_sector_num)) {
					nand_pr_err("mtk_nand_check_dececc_done fail 0x%x", u4RowAddr);
					bRet = ERR_RTN_FAIL;
				}
			}
			mtk_nand_read_fdm_data(spare_ptr, data_sector_num);
			if (g_bHwEcc) {
				if (!mtk_nand_check_bch_error
					(mtd, temp_byte_ptr, spare_ptr, data_sector_num - 1, u4RowAddr, &tempBitMap)) {
					if (devinfo.vendor != VEND_NONE)
						readRetry = TRUE;

				bRet = ERR_RTN_BCH_FAIL;
			}
			if (host->debug.err.en_print)
				nand_pr_err("u4RowAddr:%d, max_num:%d",
					u4RowAddr, host->debug.err.max_num);
		}
		mtk_nand_stop_read();
			dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn)
					DRV_WriteReg16(NFI_TLC_RD_WHR2_REG16, 0x055);

				if (logical_plane_num == 2) {
					tlc_left_plane = FALSE;
					spare_ptr += (host->hw->nand_fdm_size * data_sector_num);

#if __INTERNAL_USE_AHB_MODE__
					temp_byte_ptr += (data_sector_num * (1 << host->hw->nand_sec_shift));
#else
					temp_byte_ptr +=
					(data_sector_num * ((1 << host->hw->nand_sec_shift) + spare_per_sector));
#endif
				}
			}
#endif
			logical_plane_num--;

			if (bRet == ERR_RTN_BCH_FAIL)
				break;
				}
	} else {
		/* set BCH FAIL to read retry */
		bRet = ERR_RTN_BCH_FAIL;
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
		dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
#endif
	}

	if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
		mtk_nand_turn_off_randomizer();
	else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
		mtk_nand_turn_off_randomizer();

		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			|| devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC
#endif
			) {
			if (devinfo.tlcControl.slcopmodeEn) {
#if MLC_MICRON_SLC_MODE
				if (devinfo.vendor == VEND_MICRON) {
					feature[0] = 0x02;
					feature[1] = 0x01;
					feature[2] = 0x00;
					feature[3] = 0x00;
					mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
						0x91, (u8 *) &feature, 4);
					memset((u8 *) &feature, 0, 4);
					mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
								0x91, (u8 *) &feature, 4);
					pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
						feature[0], feature[1], feature[2], feature[3]);
				} else
#endif
				if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
				}
			}
		}
		if (bRet == ERR_RTN_BCH_FAIL) {
			u32 feature;

						tempBitMap = 0;
						feature =
				mtk_nand_rrtry_setting(devinfo,
						devinfo.feature_set.FeatureSet.rtype,
						devinfo.feature_set.FeatureSet.readRetryStart,
						retryCount);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK_TLC_1YNM)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 10;
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK_TLC_1ZNM)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 24;
			if (devinfo.feature_set.FeatureSet.rtype == RTYPE_TOSHIBA_TLC) {
				if (devinfo.tlcControl.slcopmodeEn)
					retrytotalcnt = 8;
				else
					retrytotalcnt = 31;
			}
#endif
			/* for sandisk 1znm slc mode read retry */
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 25;

			if (retryCount < retrytotalcnt) {
				mtd->ecc_stats.corrected = backup_corrected;
				mtd->ecc_stats.failed = backup_failed;
				mtk_nand_rrtry_func(mtd, devinfo, feature, FALSE);
				retryCount++;
			} else {
				feature = devinfo.feature_set.FeatureSet.readRetryDefault;
				if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK)
					&& (g_sandisk_retry_case < 3)) {
					g_sandisk_retry_case++;
					tempBitMap = 0;
					mtd->ecc_stats.corrected = backup_corrected;
					mtd->ecc_stats.failed = backup_failed;
					mtk_nand_rrtry_func(mtd, devinfo, feature, FALSE);
					retryCount = 0;
				} else {
					mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
				readRetry = FALSE;
					g_sandisk_retry_case = 0;
				}
			}
			if ((g_sandisk_retry_case == 1) || (g_sandisk_retry_case == 3)) {
				mtk_nand_set_command(0x26);
		}
		} else {
			if ((retryCount != 0) && MLC_DEVICE) {
				u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;

				mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
			}
			readRetry = FALSE;
			g_sandisk_retry_case = 0;
		}
		if (readRetry == TRUE)
			bRet = ERR_RTN_SUCCESS;
	} while (readRetry);
	if (retryCount != 0) {
		u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;

		if (bRet == ERR_RTN_SUCCESS) {
			pr_debug("OK Read retry Buf: %x %x %x %x\n", pPageBuf[0], pPageBuf[1],
				pPageBuf[2], pPageBuf[3]);
			pr_info("u4RowAddr: 0x%x read retry pass, retrycnt: %d ENUM0: %x, ENUM1: %x, mtd_ecc(A): %x, mtd_ecc(B): %x\n",
				u4RowAddr, retryCount, DRV_Reg32(ECC_DECENUM1_REG32),
				DRV_Reg32(ECC_DECENUM0_REG32), mtd->ecc_stats.failed, backup_failed);

			if (retryCount > (retrytotalcnt / 2)) {
				pr_debug("Read retry over %d times, trigger re-write\n", retryCount);
				/* mtd->ecc_stats.corrected++; */
			}

			if ((devinfo.tlcControl.slcopmodeEn
				&& (retryCount > devinfo.lifepara.slc_bitflip))
			    || ((devinfo.tlcControl.slcopmodeEn == false)
				&& (retryCount > devinfo.lifepara.data_bitflip))) {
				pr_debug("Read retry over %d times, data loss risk\n", retryCount);
				mtd->ecc_stats.corrected++;
			 }

			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
				|| (devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX))
				g_hynix_retry_count--;
		} else {
			nand_pr_err("u4RowAddr: 0x%x read retry fail, mtd_ecc(A): %x , fail, mtd_ecc(B): %x",
				u4RowAddr, mtd->ecc_stats.failed, backup_failed);
		}
		mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
		g_sandisk_retry_case = 0;
	}

	if (buf == local_buffer_16_align)
		memcpy(pPageBuf, buf, u4PageSize);

	if (bRet != ERR_RTN_SUCCESS) {
		nand_pr_err("ECC uncorrectable , fake buffer returned");
		memset(pPageBuf, 0xff, u4PageSize);
		memset(pFDMBuf, 0xff, u4SecNum * host->hw->nand_fdm_size);
	}

	PFM_END_R(pfm_time_read, u4PageSize + 32);

	return bRet;
}

int mtk_nand_exec_read_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				u8 *pFDMBuf)
{
	u8 *main_buf = pPageBuf;
	u8 *spare_buf = pFDMBuf;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
	int bRet = ERR_RTN_SUCCESS;
	u32 pagesize = u4PageSize;

	if (devinfo.two_phyplane)
		pagesize >>= 1;

	bRet = mtk_nand_exec_read_page_single(mtd, u4RowAddr, pagesize, main_buf, spare_buf);
	if (bRet != ERR_RTN_SUCCESS)
		return bRet;
	if (devinfo.two_phyplane) {
		u4SecNum >>= 1;
		main_buf += pagesize;
		spare_buf += u4SecNum * 8;
		bRet = mtk_nand_exec_read_page_single(mtd, u4RowAddr + page_per_block, pagesize, main_buf, spare_buf);
	}
	return bRet;
}

int mtk_nand_exec_read_sector_single(struct mtd_info *mtd, u32 u4RowAddr, u32 u4ColAddr, u32 u4PageSize,
				   u8 *pPageBuf, u8 *pFDMBuf, int subpageno)
{
	u8 *buf;
	int bRet = ERR_RTN_SUCCESS;
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = subpageno;
	u32 backup_corrected, backup_failed;
	bool readRetry = FALSE;
	int retryCount = 0;
	u32 retrytotalcnt = devinfo.feature_set.FeatureSet.readRetryCnt;
	u32 tempBitMap;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	bool tlc_left_plane = TRUE;
	unsigned int phys = 0;
	u32 sector_per_page = mtd->writesize >> host->hw->nand_sec_shift;
	int spare_per_sector = mtd->oobsize / sector_per_page;
#endif
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	u32 reg_val = 0;
	u32 real_row_addr = 0;
	u32 logical_plane_num = 1;
	u32 temp_col_addr[2] = {0, 0};
	u32 data_sector_num[2] = {0, 0};
	u8 *temp_byte_ptr = NULL;
	u8 *spare_ptr = NULL;
	u32 block_addr = 0;
	u32 page_in_block = 0;
	u32 page_per_block = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];
#endif

#ifdef NAND_PFM
	struct timeval pfm_time_read;
#endif
	PFM_BEGIN(pfm_time_read);

	if (pPageBuf == NULL) {
		nand_pr_err("pPageBuf is NULL!!!");
		return -EIO;
	}

	page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	/* flush_icache_range(pPageBuf, (pPageBuf + u4PageSize));//flush_cache_all();//cache flush */

	if (((unsigned long)pPageBuf % 16) && local_buffer_16_align) {
		buf = local_buffer_16_align;
		pr_debug("[%s] read buf (1) 0x%lx\n", __func__, (unsigned long)buf);
	} else {
		/* It should be allocated by vmalloc */
		if ((virt_addr_valid(pPageBuf) == 0) && local_buffer_16_align) {
			buf = local_buffer_16_align;
			pr_debug("[%s] read buf (2) 0x%lx\n", __func__, (unsigned long)buf);
		} else {
			buf = pPageBuf;
		}
	}
	backup_corrected = mtd->ecc_stats.corrected;
	backup_failed = mtd->ecc_stats.failed;
#if CFG_2CS_NAND
	if (g_bTricky_CS)
		u4RowAddr = mtk_nand_cs_on(nand, NFI_TRICKY_CS, u4RowAddr);
#endif
	do {
		temp_byte_ptr = buf;
		spare_ptr = pFDMBuf;
		temp_col_addr[0] = u4ColAddr;
		temp_col_addr[1] = 0;
		data_sector_num[0] = u4SecNum;
		data_sector_num[1] = 0;
		logical_plane_num = 1;
		tlc_wl_info.word_line_idx = u4RowAddr;
		tlc_wl_info.wl_pre = WL_LOW_PAGE;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
			if (devinfo.tlcControl.normaltlc) {
				NFI_TLC_GetMappedWL(u4RowAddr, &tlc_wl_info);
				real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);

				if ((devinfo.tlcControl.pPlaneEn)
				&& (u4ColAddr < ((sector_per_page / 2)
				* (host->hw->nand_sec_size + spare_per_sector)))) {
					tlc_left_plane = TRUE;
					if ((u4ColAddr + (u4SecNum * (host->hw->nand_sec_size + spare_per_sector)))
					> ((sector_per_page / 2) * (host->hw->nand_sec_size + spare_per_sector))) {
						logical_plane_num = 2;
						data_sector_num[1] =
						(sector_per_page / 2)
						- (u4ColAddr / (host->hw->nand_sec_size + spare_per_sector));
						data_sector_num[0] = u4SecNum - data_sector_num[1];
						temp_col_addr[0] = 0;
						temp_col_addr[1] = u4ColAddr;
					}
					real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
				}
				if (devinfo.tlcControl.pPlaneEn && (u4ColAddr >= ((sector_per_page / 2)
					* (host->hw->nand_sec_size + spare_per_sector)))) {
					temp_col_addr[0] =
						u4ColAddr - ((sector_per_page / 2)
							* (host->hw->nand_sec_size + spare_per_sector));
					tlc_left_plane = FALSE;
					real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
				}
			} else {
				real_row_addr = NFI_TLC_GetRowAddr(u4RowAddr);
			}

			if (devinfo.tlcControl.slcopmodeEn) {
				if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
				}
			} else {
				if (devinfo.tlcControl.normaltlc) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					if (tlc_wl_info.wl_pre == WL_LOW_PAGE)
						mtk_nand_set_command(LOW_PG_SELECT_CMD);
					else if (tlc_wl_info.wl_pre == WL_MID_PAGE)
						mtk_nand_set_command(MID_PG_SELECT_CMD);
					else if (tlc_wl_info.wl_pre == WL_HIGH_PAGE)
						mtk_nand_set_command(HIGH_PG_SELECT_CMD);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
				}
			}
			reg_val = 0;
		} else
#endif
		{
			real_row_addr = u4RowAddr;
		}

		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
			if (devinfo.tlcControl.slcopmodeEn) {
				if (devinfo.vendor == VEND_MICRON) {
#if MLC_MICRON_SLC_MODE
				feature[0] = 0x00;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
#else
				block_addr = real_row_addr/page_per_block;
				page_in_block = real_row_addr % page_per_block;
				page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
				real_row_addr = page_in_block + block_addr * page_per_block;
#endif
				} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
					mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);

					if (devinfo.vendor == VEND_SANDISK) {
						block_addr = real_row_addr/page_per_block;
						page_in_block = real_row_addr % page_per_block;
						page_in_block <<= 1;
						real_row_addr = page_in_block + block_addr * page_per_block;
						/* pr_warn("mtk_nand_exec_write_page SLC Mode real_row_addr:%d,
						 * u4RowAddr:%d\n", real_row_addr, u4RowAddr);
						 */
						/* pr_warn("[xl] RS SLC %d %d\n", real_row_addr, u4RowAddr); */
					}
				}
			}
		}

		if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
			mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);
		else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
			mtk_nand_turn_on_randomizer(u4RowAddr, 0, 0);

	if (mtk_nand_ready_for_read
		(nand, real_row_addr, temp_col_addr[logical_plane_num-1],
			data_sector_num[logical_plane_num - 1], true, buf, NORMAL_READ)) {
		while (logical_plane_num) {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn) {
					if (devinfo.tlcControl.pPlaneEn)
						real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
#if 1
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
#endif

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(CHANGE_COLUNM_ADDR_1ST_CMD);
					mtk_nand_set_address
						(temp_col_addr[logical_plane_num-1], real_row_addr,
							2, devinfo.addr_cycle - 2);
					mtk_nand_set_command(CHANGE_COLUNM_ADDR_2ND_CMD);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val |= CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_READ;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
				}
			}

			mtk_dir = DMA_FROM_DEVICE;
			sg_init_one
				(&mtk_sg, temp_byte_ptr, (data_sector_num[logical_plane_num - 1]
					* (1 << host->hw->nand_sec_shift)));
			dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
			phys = mtk_sg.dma_address;
#if __INTERNAL_USE_AHB_MODE__
			if (!phys)
				nand_pr_err("convert virt addr (%p) to phys add (%x)fail!!!", temp_byte_ptr, phys);
			else
				DRV_WriteReg32(NFI_STRADDR_REG32, phys);
#endif

			DRV_WriteReg32(NFI_CON_REG16, data_sector_num[logical_plane_num - 1] << CON_NFI_SEC_SHIFT);

			if (g_bHwEcc)
				ECC_Decode_Start();
#endif

			if (!mtk_nand_read_page_data
				(mtd, temp_byte_ptr, data_sector_num[logical_plane_num - 1]
					* (1 << host->hw->nand_sec_shift))) {
				nand_pr_err("mtk_nand_read_page_data fail");
				bRet = ERR_RTN_FAIL;
			}
		if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
			nand_pr_err("mtk_nand_status_ready fail");
			bRet = ERR_RTN_FAIL;
		}

		if (g_bHwEcc) {
			if (!mtk_nand_check_dececc_done(data_sector_num[logical_plane_num - 1])) {
				nand_pr_err("mtk_nand_check_dececc_done fail 0x%x", u4RowAddr);
				bRet = ERR_RTN_FAIL;
			}
		}
			mtk_nand_read_fdm_data(spare_ptr, data_sector_num[logical_plane_num - 1]);
		if (g_bHwEcc) {
			if (!mtk_nand_check_bch_error
				(mtd, temp_byte_ptr, spare_ptr, data_sector_num[logical_plane_num - 1] - 1,
				u4RowAddr, NULL)) {
				if (devinfo.vendor != VEND_NONE)
					readRetry = TRUE;
				bRet = ERR_RTN_BCH_FAIL;
			} else {
#if 0
				if (0 != (DRV_Reg32(NFI_STA_REG32) & STA_READ_EMPTY) &&
					(retryCount != 0)) {
					pr_debug("NFI read retry read empty page, return as uecc\n");
					mtd->ecc_stats.failed += data_sector_num[logical_plane_num - 1];
					bRet = ERR_RTN_BCH_FAIL;
				}
#endif
			}
			if (host->debug.err.en_print)
				nand_info("u4RowAddr:%d, max_num:%d",
					u4RowAddr, host->debug.err.max_num);
		}
		mtk_nand_stop_read();
			dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn)
					DRV_WriteReg16(NFI_TLC_RD_WHR2_REG16, 0x055);

				if (logical_plane_num == 2) {
					tlc_left_plane = FALSE;
					spare_ptr += (host->hw->nand_fdm_size * data_sector_num[logical_plane_num - 1]);

#if __INTERNAL_USE_AHB_MODE__
					temp_byte_ptr +=
						(data_sector_num[logical_plane_num - 1]
							* (1 << host->hw->nand_sec_shift));
#else
					temp_byte_ptr +=
						(data_sector_num[logical_plane_num - 1]
							* ((1 << host->hw->nand_sec_shift)
							+ spare_per_sector));
#endif
				}
	}
#endif
			logical_plane_num--;

			if (bRet == ERR_RTN_BCH_FAIL)
				break;
	}
	} else {
		/* set BCH FAIL to read retry */
		bRet = ERR_RTN_BCH_FAIL;
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
		dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
#endif
	}
		if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
			mtk_nand_turn_off_randomizer();
		else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
			mtk_nand_turn_off_randomizer();

		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			|| devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC
#endif
			) {
			if (devinfo.tlcControl.slcopmodeEn) {
#if MLC_MICRON_SLC_MODE
				if (devinfo.vendor == VEND_MICRON) {
					feature[0] = 0x02;
					feature[1] = 0x01;
					feature[2] = 0x00;
					feature[3] = 0x00;
					mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
						0x91, (u8 *) &feature, 4);
					memset((u8 *) &feature, 0, 4);
					mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
								0x91, (u8 *) &feature, 4);
					pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
						feature[0], feature[1], feature[2], feature[3]);

				} else
#endif
				if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
				}
			}
		}
		if (bRet == ERR_RTN_BCH_FAIL) {
			u32 feature = mtk_nand_rrtry_setting(devinfo,
				devinfo.feature_set.FeatureSet.rtype,
				devinfo.feature_set.FeatureSet.readRetryStart, retryCount);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK_TLC_1YNM)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 10;
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK_TLC_1ZNM)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 24;
			if (devinfo.feature_set.FeatureSet.rtype == RTYPE_TOSHIBA_TLC) {
				if (devinfo.tlcControl.slcopmodeEn)
					retrytotalcnt = 8;
				else
					retrytotalcnt = 31;
			}
#endif
			/* for sandisk 1znm slc mode read retry */
			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK)
				&& (devinfo.tlcControl.slcopmodeEn))
				retrytotalcnt = 25;
			if (retryCount < retrytotalcnt) {
				mtd->ecc_stats.corrected = backup_corrected;
				mtd->ecc_stats.failed = backup_failed;
				mtk_nand_rrtry_func(mtd, devinfo, feature, FALSE);
				retryCount++;
			} else {
				feature = devinfo.feature_set.FeatureSet.readRetryDefault;
				if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_SANDISK)
					&& (g_sandisk_retry_case < 3)) {
					g_sandisk_retry_case++;
					pr_warn("Sandisk read retry case#%d\n", g_sandisk_retry_case);
					tempBitMap = 0;
					mtd->ecc_stats.corrected = backup_corrected;
					mtd->ecc_stats.failed = backup_failed;
					mtk_nand_rrtry_func(mtd, devinfo, feature, FALSE);
					retryCount = 0;
				} else {
				mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
				readRetry = FALSE;
					g_sandisk_retry_case = 0;
				}
			}
			if ((g_sandisk_retry_case == 1) || (g_sandisk_retry_case == 3))
				mtk_nand_set_command(0x26);

		} else {
			if ((retryCount != 0) && MLC_DEVICE) {
				u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;

				mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
			}
			readRetry = FALSE;
			g_sandisk_retry_case = 0;
		}
		if (readRetry == TRUE)
			bRet = ERR_RTN_SUCCESS;
	} while (readRetry);
	if (retryCount != 0) {
		u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;

		if (bRet == ERR_RTN_SUCCESS) {
			pr_debug("[Sector RD]u4RowAddr:0x%x read retry pass, retrycnt:%d ENUM0:%x, ENUM1:%x,\n",
				u4RowAddr, retryCount, DRV_Reg32(ECC_DECENUM1_REG32), DRV_Reg32(ECC_DECENUM0_REG32));
			if (retryCount > (retrytotalcnt / 2)) {
				pr_debug("Read retry over %d times, trigger re-write\n", retryCount);
				/* mtd->ecc_stats.corrected++; */
			}

			if ((devinfo.tlcControl.slcopmodeEn
				&& (retryCount > devinfo.lifepara.slc_bitflip))
			    || ((devinfo.tlcControl.slcopmodeEn == false)
				&& (retryCount > devinfo.lifepara.data_bitflip))) {
				pr_debug("Read retry over %d times, data loss risk\n", retryCount);
				mtd->ecc_stats.corrected++;
			}

			if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
				|| (devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX)) {
				g_hynix_retry_count--;
			}
		} else {
			nand_pr_err("[Sector RD]u4RowAddr:0x%x read retry fail, mtd_ecc(A):%x , fail, mtd_ecc(B):%x",
				u4RowAddr, mtd->ecc_stats.failed, backup_failed);
		}
		mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
		g_sandisk_retry_case = 0;
	}

	if (buf == local_buffer_16_align)
		memcpy(pPageBuf, buf, u4PageSize);

	PFM_END_R(pfm_time_read, u4PageSize + 32);

	if (bRet != ERR_RTN_SUCCESS) {
		nand_pr_err("ECC uncorrectable, fake buffer returned,(0x%x,0x%x,%p,%p,%d,%d)",
			u4RowAddr, u4ColAddr, pPageBuf, pFDMBuf, u4PageSize, subpageno);
		dump_stack();
		memset(pPageBuf, 0xff, u4PageSize);
		memset(pFDMBuf, 0xff, u4SecNum*host->hw->nand_fdm_size);
	}
	return bRet;
}

int mtk_nand_exec_read_sector(struct mtd_info *mtd, u32 u4RowAddr, u32 u4ColAddr, u32 u4PageSize,
					u8 *pPageBuf, u8 *pFDMBuf, int subpageno)
{
	int bRet = ERR_RTN_SUCCESS;
	u32 sector_per_page = mtd->writesize >> host->hw->nand_sec_shift;
	int spare_per_sector = mtd->oobsize / sector_per_page;
	u32 sector_size = host->hw->nand_sec_size + spare_per_sector;
	u32 sector_start = u4ColAddr / sector_size;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 sector_1stp;

#ifdef CONFIG_MNTL_SUPPORT
	sector_start = u4ColAddr / host->hw->nand_sec_size;
#endif
	if (devinfo.two_phyplane) {
		sector_per_page >>= 1;

		if (sector_start >= sector_per_page) {
			bRet = mtk_nand_exec_read_sector_single(mtd, u4RowAddr + page_per_block,
				u4ColAddr - (sector_per_page * sector_size), u4PageSize, pPageBuf,
				pFDMBuf, subpageno);
		} else {
			if (sector_per_page - sector_start >= subpageno) {
				bRet = mtk_nand_exec_read_sector_single(mtd, u4RowAddr, u4ColAddr, u4PageSize,
					pPageBuf, pFDMBuf, subpageno);
			} else {
				sector_1stp = sector_per_page - sector_start;
				bRet = mtk_nand_exec_read_sector_single(mtd, u4RowAddr, u4ColAddr,
					sector_1stp * host->hw->nand_sec_size, pPageBuf, pFDMBuf, sector_1stp);
				if (bRet != ERR_RTN_SUCCESS)
					return bRet;
				bRet = mtk_nand_exec_read_sector_single(mtd, u4RowAddr + page_per_block, 0,
					(subpageno - sector_1stp) * host->hw->nand_sec_size,
					pPageBuf + sector_1stp * host->hw->nand_sec_size,
					pFDMBuf + (subpageno - sector_1stp) * 8, subpageno - sector_1stp);
			}
		}
	} else {
		bRet = mtk_nand_exec_read_sector_single(mtd, u4RowAddr, u4ColAddr, u4PageSize, pPageBuf, pFDMBuf,
				subpageno);
	}
	return bRet;
}

/******************************************************************************
 * mtk_nand_exec_write_page
 *
 * DESCRIPTION:
 *	 Write a page data !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize,
 *	 u8* pPageBuf, u8* pFDMBuf
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
int mtk_nand_exec_write_page_hw(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				 u8 *pFDMBuf)
{
	struct nand_chip *chip = mtd->priv;
	u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
	u8 *buf;
	u8 status;
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	u32 reg_val;
	u32 real_row_addr = 0;
	u32 block_addr = 0;
	u32 page_in_block = 0;
	u32 page_per_block = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];
#endif
#ifdef PWR_LOSS_SPOH
	u32 time;
	struct timeval pl_time_write;
	suseconds_t duration;
#endif
#ifdef NAND_PFM
	struct timeval pfm_time_write;
	struct timeval pfm_time_write_slc;
#endif
#if CFG_2CS_NAND
	if (g_bTricky_CS)
		u4RowAddr = mtk_nand_cs_on(chip, NFI_TRICKY_CS, u4RowAddr);
#endif

	page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(u4RowAddr, 1, 0);
	else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(u4RowAddr, 1, 0);
#ifdef _MTK_NAND_DUMMY_DRIVER_
	if (dummy_driver_debug) {
		unsigned long long time = sched_clock();

		if (!((time * 123 + 59) % 32768)) {
			nand_pr_err("[NAND_DUMMY_DRIVER] Simulate write error at page: 0x%x",
				   u4RowAddr);
			return -EIO;
		}
	}
#endif

	PFM_BEGIN(pfm_time_write);
	PFM_BEGIN(pfm_time_write_slc);
	if (((unsigned long)pPageBuf % 16) && local_buffer_16_align) {
		pr_info("%s Data buffer not 16 bytes aligned: %p\n", __func__, pPageBuf);
		memcpy(local_buffer_16_align, pPageBuf, u4PageSize);
		buf = local_buffer_16_align;
	} else {
		/* It should be allocated by vmalloc */
		if ((virt_addr_valid(pPageBuf) == 0) && local_buffer_16_align) {
			memcpy(local_buffer_16_align, pPageBuf, u4PageSize);
			buf = local_buffer_16_align;
			pr_debug("%s Data buffer not valid: %p\n", __func__, pPageBuf);
		} else {
			buf = pPageBuf;
		}
	}

	tlc_wl_info.wl_pre = WL_LOW_PAGE; /* avoid compile warning */
	tlc_wl_info.word_line_idx = u4RowAddr;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	mtk_nand_reset();
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)	 {
		if (devinfo.tlcControl.normaltlc) {
			NFI_TLC_GetMappedWL(u4RowAddr, &tlc_wl_info);
			real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);
			if (devinfo.tlcControl.pPlaneEn)
				real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_lg_left_plane);
		} else
			real_row_addr = NFI_TLC_GetRowAddr(u4RowAddr);

		if (devinfo.tlcControl.slcopmodeEn) {
			if (((!devinfo.tlcControl.pPlaneEn) || tlc_lg_left_plane) && (!tlc_snd_phyplane)) {
				if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
				}
			}
		} else {
			if (devinfo.tlcControl.normaltlc) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				if (tlc_program_cycle == PROGRAM_1ST_CYCLE)
					mtk_nand_set_command(PROGRAM_1ST_CYCLE_CMD);
				else if (tlc_program_cycle == PROGRAM_2ND_CYCLE)
					mtk_nand_set_command(PROGRAM_2ND_CYCLE_CMD);

				if (tlc_wl_info.wl_pre == WL_LOW_PAGE)
					mtk_nand_set_command(LOW_PG_SELECT_CMD);
				else if (tlc_wl_info.wl_pre == WL_MID_PAGE)
					mtk_nand_set_command(MID_PG_SELECT_CMD);
				else if (tlc_wl_info.wl_pre == WL_HIGH_PAGE)
					mtk_nand_set_command(HIGH_PG_SELECT_CMD);

				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
	} else
#endif
	{
		real_row_addr = u4RowAddr;
	}

	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
		if (devinfo.tlcControl.slcopmodeEn) {
			if (devinfo.vendor == VEND_MICRON) {
#if MLC_MICRON_SLC_MODE
				if (!tlc_snd_phyplane) {
					feature[0] = 0x00;
					feature[1] = 0x01;
					feature[2] = 0x00;
					feature[3] = 0x00;
					mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
						0x91, (u8 *) &feature, 4);
					memset((u8 *) &feature, 0, 4);
					mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
								0x91, (u8 *) &feature, 4);
					pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
						feature[0], feature[1], feature[2], feature[3]);
				}
#else
				block_addr = real_row_addr/page_per_block;
				page_in_block = real_row_addr % page_per_block;
				page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
				real_row_addr = page_in_block + block_addr * page_per_block;
#endif
			} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
				if (!tlc_snd_phyplane) {
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
					mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
				}

				if (devinfo.vendor == VEND_SANDISK) {
					block_addr = real_row_addr/page_per_block;
					page_in_block = real_row_addr % page_per_block;
					page_in_block <<= 1;
					real_row_addr = page_in_block + block_addr * page_per_block;
					/* pr_warn("mtk_nand_exec_write_page SLC Mode real_row_addr:%d, u4RowAddr:%d\n",
					 * real_row_addr, u4RowAddr);
					 */
				}
			}
		}
	}

	if (mtk_nand_ready_for_write(chip, real_row_addr, 0, true, buf)) {
		mtk_nand_write_fdm_data(chip, pFDMBuf, u4SecNum);
		if (!host->pre_wb_cmd)
			host->wb_cmd = mtk_nand_get_prog_cmd(tlc_wl_info.wl_pre);

		mtk_nand_dma_start_write(mtd, buf, true);

		if (host->wb_cmd == NAND_CMD_PAGEPROG) {
			if (!wait_for_completion_timeout(&g_comp_Busy_Ret, msecs_to_jiffies(NFI_TIMEOUT_MS))) {
				nand_pr_err("timeout!!!slcopmodeEn:%d, u4RowAddr:%d NFI_STA_REG32:0x%x",
					devinfo.tlcControl.slcopmodeEn, u4RowAddr, DRV_Reg32(NFI_STA_REG32));
				dump_nfi();
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
				if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
					reg_val = DRV_Reg16(NFI_DEBUG_CON1_REG16);
					reg_val &= (~0x4000);
					DRV_WriteReg16(NFI_DEBUG_CON1_REG16, reg_val);
				}
#endif
				mtk_nand_reset();
				return -EIO;
			}
		} else {
			if (!wait_for_completion_timeout(&g_comp_AHB_Done, msecs_to_jiffies(NFI_TIMEOUT_MS))) {
				nand_pr_err("ahb timeout!!!slcopmodeEn:%d, u4RowAddr:%d",
					devinfo.tlcControl.slcopmodeEn, u4RowAddr);
				mtk_nand_handle_write_ahb_done();
			}
			while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
				;
		}

		mtk_nand_stop_write();
		PL_NAND_END(pl_time_write, duration);
		PL_TIME_PROG(duration);
		if (devinfo.tlcControl.slcopmodeEn)
			PFM_END_W_SLC(pfm_time_write_slc, u4PageSize + 32);
		else
			PFM_END_W(pfm_time_write, u4PageSize + 32);

		if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
			mtk_nand_turn_off_randomizer();
		else if (pre_randomizer && u4RowAddr < RAND_START_ADDR)
			mtk_nand_turn_off_randomizer();
		/* flush_icache_range(pPageBuf, (pPageBuf + u4PageSize));//flush_cache_all();//cache flush */

		status = chip->waitfunc(mtd, chip);
		if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER)
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			|| devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC
#endif
		) {
			if (devinfo.tlcControl.slcopmodeEn) {
#if MLC_MICRON_SLC_MODE
				if (devinfo.vendor == VEND_MICRON) {
					feature[0] = 0x02;
					feature[1] = 0x01;
					feature[2] = 0x00;
					feature[3] = 0x00;
					mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
						0x91, (u8 *) &feature, 4);

					memset((u8 *) &feature, 0, 4);
					mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
								0x91, (u8 *) &feature, 4);
					pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
						feature[0], feature[1], feature[2], feature[3]);

				} else
#endif
				if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
				}
			}
		}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
			&& (devinfo.tlcControl.slcopmodeEn)) {
			if (status & SLC_MODE_OP_FALI)
				return -EIO;
			else
				return 0;
		} else
#endif
		{
			if (status & NAND_STATUS_FAIL) {
				nand_pr_err("u4RowAddr:0x%x, write fail!! status 0x%x", u4RowAddr, status);
				return -EIO;
			} else
				return 0;
		}
	} else {
		dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
		nand_pr_err("mtk_nand_ready_for_write fail!");
		if (use_randomizer && u4RowAddr >= RAND_START_ADDR)
			mtk_nand_turn_off_randomizer();
		return -EIO;
	}
}

int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf, u8 *pFDMBuf)
{
	int bRet;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 page_size = u4PageSize;
	u8 *temp_page_buf = NULL;
	u8 *temp_fdm_buf = NULL;
	u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)	 {
		if ((devinfo.tlcControl.normaltlc) && (devinfo.tlcControl.pPlaneEn)) {
			if (devinfo.two_phyplane) {
				page_size /= 4;
				u4SecNum /= 4;
			} else {
				page_size /= 2;
				u4SecNum /= 2;
			}

			tlc_snd_phyplane = FALSE;
			tlc_lg_left_plane = TRUE;
			temp_page_buf = pPageBuf;
			temp_fdm_buf = pFDMBuf;

			bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr, page_size, temp_page_buf, temp_fdm_buf);
			if (bRet != 0)
				return bRet;

			tlc_lg_left_plane = FALSE;
			temp_page_buf += page_size;
			temp_fdm_buf += (u4SecNum * host->hw->nand_fdm_size);

			bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr, page_size, temp_page_buf, temp_fdm_buf);

			if (devinfo.two_phyplane) {
				tlc_snd_phyplane = TRUE;
				tlc_lg_left_plane = TRUE;
				temp_page_buf += page_size;
				temp_fdm_buf += (u4SecNum * host->hw->nand_fdm_size);

				bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr + page_per_block, page_size,
						temp_page_buf, temp_fdm_buf);
				if (bRet != 0)
					return bRet;

				tlc_lg_left_plane = FALSE;
				temp_page_buf += page_size;
				temp_fdm_buf += (u4SecNum * host->hw->nand_fdm_size);

				bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr + page_per_block, page_size,
						temp_page_buf, temp_fdm_buf);
				tlc_snd_phyplane = FALSE;
			}
		} else {
			if ((devinfo.tlcControl.normaltlc) && (devinfo.two_phyplane)) {
				page_size /= 2;
				u4SecNum /= 2;
			}

			tlc_snd_phyplane = FALSE;
			temp_page_buf = pPageBuf;
			temp_fdm_buf = pFDMBuf;
			bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr, page_size, temp_page_buf, temp_fdm_buf);
			if (bRet != 0)
				return bRet;

			if ((devinfo.tlcControl.normaltlc) && (devinfo.two_phyplane)) {
				tlc_snd_phyplane = TRUE;
				temp_page_buf += page_size;
				temp_fdm_buf += (u4SecNum * host->hw->nand_fdm_size);
				bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr + page_per_block, page_size,
						temp_page_buf, temp_fdm_buf);
				tlc_snd_phyplane = FALSE;
			}
		}
	} else
#endif
	{
		if (devinfo.two_phyplane) {
			page_size /= 2;
			u4SecNum /= 2;
		}
		tlc_snd_phyplane = FALSE;
		temp_page_buf = pPageBuf;
		temp_fdm_buf = pFDMBuf;
		bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr, page_size, temp_page_buf, temp_fdm_buf);
		if (bRet != 0)
			return bRet;
		if (devinfo.two_phyplane) {
			tlc_snd_phyplane = TRUE;
			temp_page_buf += page_size;
			temp_fdm_buf += (u4SecNum * host->hw->nand_fdm_size);
			bRet = mtk_nand_exec_write_page_hw(mtd, u4RowAddr + page_per_block, page_size,
					temp_page_buf, temp_fdm_buf);
			tlc_snd_phyplane = FALSE;
		}
	}
	return bRet;
}

/******************************************************************************
 *
 * Write a page to a logical address
 *
 *****************************************************************************/
static int mtk_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				   uint32_t offset, int data_len, const uint8_t *buf,
				   int oob_required, int page, int cached, int raw)
{
	/* int block_size = 1 << (chip->phys_erase_shift); */
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	u32 row_addr;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
	/* MSG(INIT,"[WRITE] %d, %d, %d %d\n",mapped_block, block, page_in_block, page_per_block); */
	/* write bad index into oob */
	if (mapped_block != block)
		set_bad_index_to_oob(chip->oob_poi, block);
	else
		set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
	/* pr_debug("[xiaolei] mtk_nand_write_page 0x%x\n", (u32)buf); */
	row_addr = page_in_block + mapped_block * page_per_block;
	if (devinfo.two_phyplane)
		row_addr = page_in_block + (mapped_block << 1) * page_per_block;
	if (mtk_nand_exec_write_page(mtd, row_addr, mtd->writesize, (u8 *) buf, chip->oob_poi)) {
		nand_pr_err("write fail at block: 0x%x, page: 0x%x", mapped_block, page_in_block);
		if (update_bmt((u64) ((u64) page_in_block + (u64) mapped_block * page_per_block) <<
			 chip->page_shift, UPDATE_WRITE_FAIL, (u8 *) buf, chip->oob_poi)) {
			pr_debug("Update BMT success\n");
			return 0;
		}
		nand_pr_err("Update BMT fail");
		return -EIO;
	}
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.WritePageTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WritePageCount++;
	dump_nand_rwcount();
#endif
	return 0;
}
void mtk_nand_enable_slc_mode(struct mtd_info *mtd)
{
	u32 reg_val = 0;

	if (devinfo.vendor == VEND_MICRON) {
#if MLC_MICRON_SLC_MODE
	u8 feature[4];

	feature[0] = 0x00;
	feature[1] = 0x01;
	feature[2] = 0x00;
	feature[3] = 0x00;
	mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
		0x91, (u8 *) &feature, 4);
	memset((u8 *) &feature, 0, 4);
	mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
				0x91, (u8 *) &feature, 4);
	pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
		feature[0], feature[1], feature[2], feature[3]);
#endif
	} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
		reg_val = DRV_Reg16(NFI_CNFG_REG16);
		reg_val &= ~CNFG_READ_EN;
		reg_val &= ~CNFG_OP_MODE_MASK;
		reg_val |= CNFG_OP_CUST;
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
		mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
		reg_val = DRV_Reg32(NFI_CON_REG16);
		reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
		/* issue reset operation */
		DRV_WriteReg32(NFI_CON_REG16, reg_val);
	}
}

void mtk_nand_disable_slc_mode(struct mtd_info *mtd)
{
	u32 reg_val = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];

	if (devinfo.vendor == VEND_MICRON) {
		feature[0] = 0x02;
		feature[1] = 0x01;
		feature[2] = 0x00;
		feature[3] = 0x00;
		mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
			0x91, (u8 *) &feature, 4);
		memset((u8 *) &feature, 0, 4);
		mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
					0x91, (u8 *) &feature, 4);
		pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
			feature[0], feature[1], feature[2], feature[3]);

	} else
#endif
	if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
		reg_val = DRV_Reg32(NFI_CON_REG16);
		reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
		/* issue reset operation */
		DRV_WriteReg32(NFI_CON_REG16, reg_val);

		reg_val = DRV_Reg16(NFI_CNFG_REG16);
		reg_val &= ~CNFG_READ_EN;
		reg_val &= ~CNFG_OP_MODE_MASK;
		reg_val |= CNFG_OP_CUST;
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

		mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
	}
}

u32 mtk_nand_get_slc_row_addr(u32 row_addr, u32 block_pages)
{
	u32 block_addr, page_in_block;

	if (devinfo.vendor == VEND_MICRON) {
#if !MLC_MICRON_SLC_MODE
	block_addr = row_addr/block_pages;
	page_in_block = row_addr % block_pages;
	page_in_block = functArray[devinfo.feature_set.ptbl_idx](page_in_block);
	return page_in_block + block_addr * block_pages;
#endif
	} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
		if (devinfo.vendor == VEND_SANDISK) {
			block_addr = row_addr/block_pages;
			page_in_block = row_addr % block_pages;
			page_in_block <<= 1;
			return page_in_block + block_addr * block_pages;
		}
	}

	return row_addr;
}

void mtk_nand_enable_rand_by_row(u32 rowAddr)
{
	if (use_randomizer && rowAddr >= RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(rowAddr, 0, 0);
	else if (pre_randomizer && rowAddr < RAND_START_ADDR)
		mtk_nand_turn_on_randomizer(rowAddr, 0, 0);
}

void mtk_nand_disable_rand_by_row(u32 rowAddr)
{
	if (use_randomizer && rowAddr >= RAND_START_ADDR)
		mtk_nand_turn_off_randomizer();
	else if (pre_randomizer && rowAddr < RAND_START_ADDR)
		mtk_nand_turn_off_randomizer();
}

static u8 *mtk_nand_get_align_buf(u8 *buf)
{
	if (!local_buffer_16_align) {
		nand_pr_err("local buffer is not init");
		return buf;
	}

	if (((unsigned long)buf % 16) || (virt_addr_valid(buf) == 0))
		return local_buffer_16_align;

	return buf;
}

static unsigned int mtk_nand_dma_map(u8 *buf, int sec_num)
{
	mtk_dir = DMA_FROM_DEVICE;
	sg_init_one(&mtk_sg, buf, (sec_num * (1 << host->hw->nand_sec_shift)));
	dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
	return mtk_sg.dma_address;
}

static int mtk_nand_send_addr_cmd(u16 cmd0, u32 u4RowAddr, u32 u4ColAddr, u16 cmd1)
{
	int ret = -1;
	u32 colnob = 2, rownob = devinfo.addr_cycle - 2;

	if (!mtk_nand_set_command(cmd0))
		goto cleanup;
	if (!mtk_nand_set_address(u4ColAddr, u4RowAddr, colnob, rownob))
		goto cleanup;

	if (!mtk_nand_set_command(cmd1))
		goto cleanup;

	if (!mtk_nand_status_ready(STA_NAND_BUSY))
		goto cleanup;

	ret = 0;

cleanup:
	if (ret)
		nand_pr_err("fail!!!");
	return ret;
}

static int mtk_nand_send_read_cmd(u32 u4RowAddr, u32 u4ColAddr, u16 cmd)
{
	return mtk_nand_send_addr_cmd(0x00, u4RowAddr, u4ColAddr, cmd);
}

#define CMD_READ_PAGE_MULTI_PLANE(u4RowAddr, u4ColAddr) \
		mtk_nand_send_read_cmd(u4RowAddr, u4ColAddr, 0x32)

#define CMD_READ_PAGE(u4RowAddr, u4ColAddr) \
		mtk_nand_send_read_cmd(u4RowAddr, u4ColAddr, 0x30)


#define CMD_CHANGE_READ_COLUMN_ENHANCED(u4RowAddr, u4ColAddr) \
		mtk_nand_send_addr_cmd(0x06, u4RowAddr, u4ColAddr, 0xE0)

static int mtk_nand_wait_dma_data_ready(struct mtd_info *mtd, int sect_num)
{
	unsigned long long timeout = 0xfffff;
	int sector_read_time = 0;
	unsigned long long s = 0, e = 0;
	unsigned int length = sect_num << host->hw->nand_sec_shift;

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
	mb();
	NFI_SET_REG32(NFI_CON_REG16, CON_NFI_BRD | CON_NFI_BWR);
	g_running_dma = 1;
	DRV_WriteReg(NFI_STRDATA_REG16, 0x1);

	if (ahb_read_sleep_enable()) {
		sector_read_time = get_sector_ahb_read_time();
		if (!sector_read_time) {
			s = sched_clock();
		} else if (ahb_read_can_sleep(length)) {
			int min, max;

			get_ahb_read_sleep_range(length, &min, &max);
			usleep_range(min, max);
		}
	}

	while (sect_num > ((DRV_Reg32(NFI_BYTELEN_REG16) & 0x1f000) >> 12)) {
		timeout--;
		if (timeout == 0) {
			nand_pr_err("[%s] poll BYTELEN error", __func__);
			dump_nfi();
			g_running_dma = 0;
			return -1;
		}
	}

	timeout = 0xffffffffffff;
	while ((DRV_Reg32(NFI_MASTERSTA_REG16) & 0x3) != 0) {
		timeout--;
		if (timeout == 0) {
			nand_pr_err("poll NFI_MASTERSTA_REG16 error (0x%x)",
					DRV_Reg32(NFI_MASTERSTA_REG16));
			dump_nfi();
			g_running_dma = 0;
			return -1;
		}
	}

	if (ahb_read_sleep_enable() && !sector_read_time) {
		e = sched_clock();
		nand_info("s:(%llu) e:(%llu)", s, e);
		cal_sector_ahb_read_time(s, e, length);
	}

	return 0;
}

static int mtk_nand_read_retry_clear(struct mtd_info *mtd)
{
	u32 feature = devinfo.feature_set.FeatureSet.readRetryDefault;

	mtk_nand_rrtry_func(mtd, devinfo, feature, TRUE);
	return 0;
}

static int mtk_nand_read_retry_set(struct mtd_info *mtd, bool *pretry,
					int retryCount)
{
	struct gFeatureSet *featureSet = &devinfo.feature_set.FeatureSet;
	u32 feature;

	if (retryCount < featureSet->readRetryCnt) {
		feature = mtk_nand_rrtry_setting(devinfo, featureSet->rtype,
				featureSet->readRetryStart, retryCount);

		mtk_nand_rrtry_func(mtd, devinfo, feature, FALSE);
		*pretry = true;
	} else {
		mtk_nand_read_retry_clear(mtd);
		*pretry = FALSE;
	}

	return 0;
}

static u32 mtk_nand_get_col_addr(struct mtd_info *mtd,
			struct mtk_nand_chip_info *info, unsigned int block, unsigned int offset)
{
	unsigned int page_size, sect_num;

	page_size = (block < info->data_block_num) ? info->data_page_size : info->log_page_size;
	sect_num = page_size/(1<<info->sector_size_shift);
	return (offset >> info->sector_size_shift) *
		((1<<info->sector_size_shift) + mtd->oobsize / sect_num);
}

int mtk_nand_multi_plane_read(struct mtd_info *mtd,
			struct mtk_nand_chip_info *info, int page_num,
			struct mtk_nand_chip_read_param *param)
{
	u32 row_addr, real_row_addr = 0;
	u32 page_per_block = 0;
	bool readRetry = false;
	int retryCount[2] = {0, 0};
	u32 backup_corrected, backup_failed;
	int err_page = -1;
	struct NAND_LIFE_PARA *life = &devinfo.lifepara;
	u16 bitflip;
	#define READ_ERR_NFI 1
	#define READ_ERR_BCH 2
	#define READ_ERR_NONE 0
	int err;

	bitflip = devinfo.tlcControl.slcopmodeEn ? life->slc_bitflip : life->data_bitflip;
	page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	backup_corrected = mtd->ecc_stats.corrected;
	backup_failed = mtd->ecc_stats.failed;

	if (DRV_Reg32(NFI_NAND_TYPE_CNFG_REG32) & 0x3) {
		NFI_SET_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);	/* reset */
		NFI_CLN_REG16(NFI_MASTERRST_REG32, PAD_MACRO_RST);	/* dereset */
	}

	/* Enable HW ECC */
	NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	mtk_nand_set_mode(CNFG_OP_CUST);
	mtk_nand_set_autoformat(true);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	do {
		int i;
		u8 *pageBuf = NULL;
		unsigned int phyAddr = 0;
		struct mtk_nand_chip_read_param *p = param;
		u32 sectorNum, colAddr;

		for (i = 0; i < page_num; i++) {
			row_addr = get_ftl_row_addr(info, p[i].block, p[i].page);
			sectorNum = p[i].size >> 10;
			colAddr = mtk_nand_get_col_addr(mtd, info, p[i].block, p[i].offset);
			if (devinfo.tlcControl.slcopmodeEn)
				real_row_addr = mtk_nand_get_slc_row_addr(row_addr, page_per_block);
			else
				real_row_addr = row_addr;

			if (!mtk_nand_reset()) {
				nand_pr_err("reset nand fail");
				return -ENANDREAD;
			}

			if (i < (page_num - 1))
				err = CMD_READ_PAGE_MULTI_PLANE(real_row_addr, colAddr);
			else
				err = CMD_READ_PAGE(real_row_addr, colAddr);
			if (err) {
				nand_pr_err("cmd fail, i:%d", i);
				return -ENANDREAD;
			}
		}

		/*read data*/
		for (i = 0; i < page_num; i++) {
			err = 0;
			row_addr = get_ftl_row_addr(info, p[i].block, p[i].page);
			colAddr = mtk_nand_get_col_addr(mtd, info, p[i].block, p[i].offset);

			mtk_nand_enable_rand_by_row(row_addr);
			sectorNum = p[i].size >> 10;
			if (devinfo.tlcControl.slcopmodeEn)
				real_row_addr = mtk_nand_get_slc_row_addr(row_addr, page_per_block);
			else
				real_row_addr = row_addr;

			mtk_nand_reset();
			err = CMD_CHANGE_READ_COLUMN_ENHANCED(real_row_addr, colAddr);
			if (err) {
				nand_pr_err("cmd fail, i:%d", i);
				err = -READ_ERR_NFI;
			}

			pageBuf = mtk_nand_get_align_buf(p[i].data_buffer);
			phyAddr = mtk_nand_dma_map(pageBuf, sectorNum);
			DRV_WriteReg32(NFI_STRADDR_REG32, phyAddr);
			DRV_WriteReg32(NFI_CON_REG16,  sectorNum << CON_NFI_SEC_SHIFT);

			ECC_Decode_Start();

			err = mtk_nand_wait_dma_data_ready(mtd, sectorNum);
			if (err) {
				nand_pr_err("wait dma ready fail, i:%d", i);
				err = -READ_ERR_NFI;
			}

			if (!err && !mtk_nand_status_ready(STA_NAND_BUSY)) {
				nand_pr_err("mtk_nand_status_ready fail");
				err = -READ_ERR_NFI;
			}

			if (!err && !mtk_nand_check_dececc_done(sectorNum)) {
				nand_pr_err("mtk_nand_check_dececc_done fail 0x%x", real_row_addr);
				err = -READ_ERR_NFI;
			}

			mtk_nand_read_fdm_data(p[i].oob_buffer, sectorNum);
			dma_unmap_sg(mtk_dev, &mtk_sg, 1, DMA_FROM_DEVICE);
			if (!mtk_nand_check_bch_error(mtd, pageBuf, p[i].oob_buffer, sectorNum - 1,
								row_addr, NULL)) {
				nand_pr_err("BCH_FAIL");
				err = -READ_ERR_BCH;
			}
			mtk_nand_stop_read();
			mtk_nand_disable_rand_by_row(row_addr);
			if (err) {
				err_page = i;
				break;
			}
			if (pageBuf != p[i].data_buffer)
				memcpy(p[i].data_buffer, pageBuf, p[i].size);
		}

		if (err == -READ_ERR_BCH) {
			mtk_nand_read_retry_set(mtd, &readRetry, retryCount[i]);
			if (readRetry) {
				mtd->ecc_stats.corrected = backup_corrected;
				mtd->ecc_stats.failed = backup_failed;
				retryCount[i]++;
			}
		} else {
			if (retryCount[0] || retryCount[1])
				mtk_nand_read_retry_clear(mtd);

			if (err == -READ_ERR_NFI)
				return -ENANDREAD;
			readRetry = FALSE;
		}
	} while (readRetry);

	if (err == -READ_ERR_BCH)
		return err_page ? 1 : -ENANDREAD;

	if (retryCount[0] > bitflip)
		return -ENANDFLIPS;

	if (retryCount[1] > bitflip)
		return 1;

	return 2;
}

/* ------------------------------------------------------------------------------- */
#if 0
static void mtk_nand_command_sp(
	struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
	g_u4ColAddr	= column;
	g_u4RowAddr	= page_addr;

	switch (command) {
	case NAND_CMD_STATUS:
		break;

	case NAND_CMD_READID:
		break;

	case NAND_CMD_RESET:
		break;

	case NAND_CMD_RNDOUT:
	case NAND_CMD_RNDOUTSTART:
	case NAND_CMD_RNDIN:
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_STATUS_MULTI:
	default:
		break;
	}

}
#endif

/******************************************************************************
 * mtk_nand_command_bp
 *
 * DESCRIPTION:
 *	 Handle the commands from MTD !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, unsigned int command, int column, int page_addr
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_command_bp(struct mtd_info *mtd, unsigned int command, int column,
				int page_addr)
{
	struct nand_chip *nand = mtd->priv;
#if 0
/* int block_size = 1 << (nand->phys_erase_shift); */
/* int page_per_block = 1 << (nand->phys_erase_shift - nand->page_shift); */
/* u32 block; */
/* u16 page_in_block; */
/* u32 mapped_block; */
/* bool rand= FALSE; */
	page_addr = mtk_nand_page_transform(mtd, nand, &block, &mapped_block);
	page_addr = mapped_block * page_per_block + page_addr;
#endif
	switch (command) {
	case NAND_CMD_SEQIN:
		memset(g_kCMD.au1OOB, 0xFF, sizeof(g_kCMD.au1OOB));
		g_kCMD.pDataBuf = NULL;
		/* } */
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column;
		break;

	case NAND_CMD_PAGEPROG:
		if (g_kCMD.pDataBuf || (g_kCMD.au1OOB[0] != 0xFF)) {
			u8 *pDataBuf = g_kCMD.pDataBuf ? g_kCMD.pDataBuf : nand->buffers->databuf;
			u32 row_addr = g_kCMD.u4RowAddr;
			u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
			/* pr_debug("[xiaolei] mtk_nand_command_bp 0x%x\n", (u32)pDataBuf); */
			if (devinfo.two_phyplane) {
				row_addr = (((row_addr / page_per_block) << 1) * page_per_block)
						+ (row_addr % page_per_block);
			}

		if (((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
			|| (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
				&& (devinfo.tlcControl.normaltlc)
				&& (!mtk_block_istlc(g_kCMD.u4RowAddr * mtd->writesize)))
			devinfo.tlcControl.slcopmodeEn = TRUE;
		else
			devinfo.tlcControl.slcopmodeEn = FALSE;
			mtk_nand_exec_write_page(mtd, row_addr, mtd->writesize, pDataBuf,
						 g_kCMD.au1OOB);
			g_kCMD.u4RowAddr = (u32) -1;
			g_kCMD.u4OOBRowAddr = (u32) -1;
		}
		break;

	case NAND_CMD_READOOB:
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column + mtd->writesize;
#ifdef NAND_PFM
		g_kCMD.pureReadOOB = 1;
		g_kCMD.pureReadOOBNum += 1;
#endif
		break;

	case NAND_CMD_READ0:
		g_kCMD.u4RowAddr = page_addr;
		g_kCMD.u4ColAddr = column;
#ifdef NAND_PFM
		g_kCMD.pureReadOOB = 0;
#endif
		break;

	case NAND_CMD_ERASE1:
		(void)mtk_nand_reset();
		mtk_nand_set_mode(CNFG_OP_ERASE);
		if (g_i4Interrupt) {
			DRV_Reg16(NFI_INTR_REG16);
			NFI_SET_REG16(NFI_INTR_EN_REG16, INTR_ERASE_DONE_EN);
		}
		(void)mtk_nand_set_command(NAND_CMD_ERASE1);
		(void)mtk_nand_set_address(0, page_addr, 0, devinfo.addr_cycle - 2);
		break;

	case NAND_CMD_ERASE2:
		(void)mtk_nand_set_command(NAND_CMD_ERASE2);
		if (g_i4Interrupt) {
			if (!wait_for_completion_timeout(&g_comp_ER_Done, msecs_to_jiffies(NFI_TIMEOUT_MS))) {
				pr_notice("wait for completion timeout happened @ [%s]: %d\n", __func__,
					__LINE__);
				dump_nfi();
			}
		} else {
			while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
				;
		}
		break;

	case NAND_CMD_STATUS:
		(void)mtk_nand_reset();
		if (mtk_nand_israndomizeron()) {
			/* g_brandstatus = TRUE; */
			mtk_nand_turn_off_randomizer();
		}
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
		mtk_nand_set_mode(CNFG_OP_SRD);
		mtk_nand_set_mode(CNFG_READ_EN);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		(void)mtk_nand_set_command(NAND_CMD_STATUS);
		NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_NOB_MASK);
		mb();
		DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD | (1 << CON_NFI_NOB_SHIFT));
		g_bcmdstatus = true;
		break;

	case NAND_CMD_RESET:
		mtk_nand_device_reset();
		break;

	case NAND_CMD_READID:
		/* Issue NAND chip reset command */
		/* NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0); */

		/* timeout = TIMEOUT_4; */

		/* while (timeout) */
		/* timeout--; */

		mtk_nand_reset();
		/* Disable HW ECC */
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);

		/* Disable 16-bit I/O */
		/* NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN); */

		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN | CNFG_BYTE_RW);
		(void)mtk_nand_reset();
		mb();
		mtk_nand_set_mode(CNFG_OP_SRD);
		(void)mtk_nand_set_command(NAND_CMD_READID);
		(void)mtk_nand_set_address(0, 0, 1, 0);
		DRV_WriteReg32(NFI_CON_REG16, CON_NFI_SRD);
		while (DRV_Reg32(NFI_STA_REG32) & STA_DATAR_STATE)
			;
		break;

	default:
		break;
	}
}

/******************************************************************************
 * mtk_nand_select_chip
 *
 * DESCRIPTION:
 *	 Select a chip !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, int chip
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if (chip == -1 && false == g_bInitDone) {
		struct nand_chip *nand = mtd->priv;

		struct mtk_nand_host *host = nand->priv;
		struct mtk_nand_host_hw *hw = host->hw;
		u32 spare_per_sector = mtd->oobsize / (mtd->writesize / hw->nand_sec_size);
		u32 ecc_bit = 4;
		u32 spare_bit = PAGEFMT_SPARE_16;
		u32 pagesize = mtd->writesize;

		hw->nand_fdm_size = 8;

		switch (spare_per_sector) {
#ifndef MTK_COMBO_NAND_SUPPORT
		case 16:
			spare_bit = PAGEFMT_SPARE_16;
			ecc_bit = 4;
			spare_per_sector = 16;
			break;
		case 26:
		case 27:
		case 28:
			spare_bit = PAGEFMT_SPARE_26;
			ecc_bit = 10;
			spare_per_sector = 26;
			break;
		case 32:
			ecc_bit = 12;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_32_1KS;
			else
				spare_bit = PAGEFMT_SPARE_32;
			spare_per_sector = 32;
			break;
		case 40:
			ecc_bit = 18;
			spare_bit = PAGEFMT_SPARE_40;
			spare_per_sector = 40;
			break;
		case 44:
			ecc_bit = 20;
			spare_bit = PAGEFMT_SPARE_44;
			spare_per_sector = 44;
			break;
		case 48:
		case 49:
			ecc_bit = 22;
			spare_bit = PAGEFMT_SPARE_48;
			spare_per_sector = 48;
			break;
		case 50:
		case 51:
			ecc_bit = 24;
			spare_bit = PAGEFMT_SPARE_50;
			spare_per_sector = 50;
			break;
		case 52:
		case 54:
		case 56:
			ecc_bit = 24;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_52_1KS;
			else
				spare_bit = PAGEFMT_SPARE_52;
			spare_per_sector = 32;
			break;
#endif
		case 62:
		case 63:
			ecc_bit = 28;
			spare_bit = PAGEFMT_SPARE_62;
			spare_per_sector = 62;
			break;
		case 64:
			ecc_bit = 32;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_64_1KS;
			else
				spare_bit = PAGEFMT_SPARE_64;
			spare_per_sector = 64;
			break;
		case 72:
			ecc_bit = 36;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_72_1KS;
			spare_per_sector = 72;
			break;
		case 80:
			ecc_bit = 40;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_80_1KS;
			spare_per_sector = 80;
			break;
		case 88:
			ecc_bit = 44;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_88_1KS;
			spare_per_sector = 88;
			break;
		case 96:
		case 98:
			ecc_bit = 48;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_96_1KS;
			spare_per_sector = 96;
			break;
		case 100:
		case 102:
		case 104:
			ecc_bit = 52;
			if (MLC_DEVICE == TRUE)
				spare_bit = PAGEFMT_SPARE_100_1KS;
			spare_per_sector = 100;
			break;
		case 122:
		case 124:
		case 126:
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
				&& devinfo.tlcControl.ecc_recalculate_en) {
				if (devinfo.tlcControl.ecc_required > 60) {
					hw->nand_fdm_size = 3;
					ecc_bit = 68;
				} else
					ecc_bit = 60;
			} else
#endif
				ecc_bit = 60;
			if (hw->nand_sec_size == 1024)
				spare_bit = PAGEFMT_SPARE_122_1KS;
			spare_per_sector = 122;
			break;
		case 128:
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
				&& devinfo.tlcControl.ecc_recalculate_en) {
				if (devinfo.tlcControl.ecc_required > 68) {
					hw->nand_fdm_size = 2;
					ecc_bit = 72;
				} else
					ecc_bit = 68;
			} else
#endif
				ecc_bit = 68;
			if (hw->nand_sec_size == 1024)
				spare_bit = PAGEFMT_SPARE_128_1KS;
			spare_per_sector = 128;
			break;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		case 134:
			ecc_bit = 72;
			if (hw->nand_sec_size == 1024)
				spare_bit = PAGEFMT_SPARE_134_1KS;
			spare_per_sector = 134;
			break;
		case 148:
			ecc_bit = 80;
			if (hw->nand_sec_size == 1024)
				spare_bit = PAGEFMT_SPARE_148_1KS;
			spare_per_sector = 148;
			break;
#endif
		default:
			pr_notice("[NAND]: NFI not support oobsize: %x\n", spare_per_sector);
			mtk_nand_assert(0);
		}

		mtd->oobsize = spare_per_sector * (mtd->writesize / hw->nand_sec_size);
		pr_debug("[NAND]select ecc bit:%d, sparesize :%d\n", ecc_bit, mtd->oobsize);

		pagesize = mtd->writesize;
		if (devinfo.two_phyplane)
			pagesize >>= 1;
		/* Setup PageFormat */
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			if (pagesize == 16384) {
				NFI_SET_REG16(NFI_PAGEFMT_REG16,
						  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_16K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 8192) {
				NFI_SET_REG16(NFI_PAGEFMT_REG16,
						  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_8K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 4096) {
				if (MLC_DEVICE == FALSE)
					NFI_SET_REG16(NFI_PAGEFMT_REG16,
							  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_4K);
				else
					NFI_SET_REG16(NFI_PAGEFMT_REG16,
							  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_4K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 2048) {
				if (MLC_DEVICE == FALSE)
					NFI_SET_REG16(NFI_PAGEFMT_REG16,
							  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_2K);
				else
					NFI_SET_REG16(NFI_PAGEFMT_REG16,
							  (spare_bit << PAGEFMT_SPARE_SHIFT_V1) | PAGEFMT_2K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			}
		} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
			if (pagesize == 16384) {
				NFI_SET_REG32(NFI_PAGEFMT_REG32,
						  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_16K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 8192) {
				NFI_SET_REG32(NFI_PAGEFMT_REG32,
						  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_8K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 4096) {
				if (MLC_DEVICE == FALSE)
					NFI_SET_REG32(NFI_PAGEFMT_REG32,
							  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K);
				else
					NFI_SET_REG32(NFI_PAGEFMT_REG32,
							  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			} else if (pagesize == 2048) {
				if (MLC_DEVICE == FALSE)
					NFI_SET_REG32(NFI_PAGEFMT_REG32,
							  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K);
				else
					NFI_SET_REG32(NFI_PAGEFMT_REG32,
							  (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K_1KS);
				nand->cmdfunc = mtk_nand_command_bp;
			}
		} else {
			nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
				mtk_nfi_dev_comp->chip_ver);
		}

		mtk_nand_configure_fdm(hw->nand_fdm_size);

		ecc_threshold = ecc_bit * 4 / 5;
		ECC_Config(hw, ecc_bit);
		g_bInitDone = true;

		/* xiaolei for kernel3.10 */
		nand->ecc.strength = ecc_bit;
		mtd->bitflip_threshold = nand->ecc.strength;
	}
	switch (chip) {
	case -1:
		break;
	case 0:
#ifdef CFG_FPGA_PLATFORM	/* FPGA NAND is placed at CS1 not CS0 */
		DRV_WriteReg16(NFI_CSEL_REG16, 0);
		break;
#endif
	case 1:
		DRV_WriteReg16(NFI_CSEL_REG16, chip);
		break;
	}
}

/******************************************************************************
 * mtk_nand_read_byte
 *
 * DESCRIPTION:
 *	 Read a byte of data !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static uint8_t mtk_nand_read_byte(struct mtd_info *mtd)
{
#if 0
	/* while(0 == FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16))); */
	/* Check the PIO bit is ready or not */
	u32 timeout = TIMEOUT_4;
	uint8_t retval = 0;

	WAIT_NFI_PIO_READY(timeout);

	retval = DRV_Reg8(NFI_DATAR_REG32);
	MSG(INIT, "mtk_nand_read_byte (0x%x)\n", retval);

	if (g_bcmdstatus) {
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		g_bcmdstatus = false;
	}

	return retval;
#endif
	uint8_t retval = 0;

	if (!mtk_nand_pio_ready()) {
		nand_pr_err("pio ready timeout");
		retval = false;
	}

	if (g_bcmdstatus) {
		retval = DRV_Reg8(NFI_DATAR_REG32);
		NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_NOB_MASK);
		mtk_nand_reset();
#if (__INTERNAL_USE_AHB_MODE__)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
#endif
		if (g_bHwEcc)
			NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		else
			NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		g_bcmdstatus = false;
	} else
		retval = DRV_Reg8(NFI_DATAR_REG32);

	return retval;
}

/******************************************************************************
 * mtk_nand_read_buf
 *
 * DESCRIPTION:
 *	 Read NAND data !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, uint8_t *buf, int len
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *nand = (struct nand_chip *)mtd->priv;
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	u32 row_addr = pkCMD->u4RowAddr;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	if (((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		|| (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
		 && (devinfo.tlcControl.normaltlc) && (!mtk_block_istlc(pkCMD->u4RowAddr * mtd->writesize)))
		devinfo.tlcControl.slcopmodeEn = TRUE;
	else
		devinfo.tlcControl.slcopmodeEn = FALSE;

	if (devinfo.two_phyplane)
		row_addr = (((row_addr / page_per_block) << 1) * page_per_block) + (row_addr % page_per_block);

	if (u4ColAddr < u4PageSize) {
		if ((u4ColAddr == 0) && (len >= u4PageSize)) {
			mtk_nand_exec_read_page(mtd, row_addr, u4PageSize, buf, pkCMD->au1OOB);
			if (len > u4PageSize) {
				u32 u4Size = min(len - u4PageSize, (u32) (sizeof(pkCMD->au1OOB)));

				memcpy(buf + u4PageSize, pkCMD->au1OOB, u4Size);
			}
		} else {
			mtk_nand_exec_read_page(mtd, row_addr, u4PageSize, nand->buffers->databuf, pkCMD->au1OOB);
			memcpy(buf, nand->buffers->databuf + u4ColAddr, len);
		}
		pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
	} else {
		u32 u4Offset = u4ColAddr - u4PageSize;
		u32 u4Size = min(len - u4Offset, (u32) (sizeof(pkCMD->au1OOB)));

		if (pkCMD->u4OOBRowAddr != pkCMD->u4RowAddr) {
			mtk_nand_exec_read_page(mtd, row_addr, u4PageSize, nand->buffers->databuf, pkCMD->au1OOB);
			pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
		}
		memcpy(buf, pkCMD->au1OOB + u4Offset, u4Size);
	}
	pkCMD->u4ColAddr += len;
}

/******************************************************************************
 * mtk_nand_write_buf
 *
 * DESCRIPTION:
 *	 Write NAND data !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, const uint8_t *buf, int len
 *
 * RETURNS:
 *	 None
 *

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		 && (devinfo.tlcControl.normaltlc) && (!mtk_block_istlc(pkCMD->u4RowAddr * mtd->writesize)))
		devinfo.tlcControl.slcopmodeEn = TRUE;
	else
		devinfo.tlcControl.slcopmodeEn = FALSE;
#endif
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	int i4Size, i;

	if (u4ColAddr >= u4PageSize) {
		u32 u4Offset = u4ColAddr - u4PageSize;
		u8 *pOOB = pkCMD->au1OOB + u4Offset;

		i4Size = min(len, (int)(sizeof(pkCMD->au1OOB) - u4Offset));

		for (i = 0; i < i4Size; i++)
			pOOB[i] &= buf[i];
	} else {
		pkCMD->pDataBuf = (u8 *) buf;
	}

	pkCMD->u4ColAddr += len;
}

/******************************************************************************
 * mtk_nand_write_page_hwecc
 *
 * DESCRIPTION:
 *	 Write NAND data with hardware ecc !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static int mtk_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
					 const uint8_t *buf, int oob_required, int page)
{
	mtk_nand_write_buf(mtd, buf, mtd->writesize);
	mtk_nand_write_buf(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

/******************************************************************************
 * mtk_nand_read_page_hwecc
 *
 * DESCRIPTION:
 *	 Read NAND data with hardware ecc !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static int mtk_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf,
					int oob_required, int page)
{
#if 0
	mtk_nand_read_buf(mtd, buf, mtd->writesize);
	mtk_nand_read_buf(mtd, chip->oob_poi, mtd->oobsize);
#else
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
	u32 row_addr = pkCMD->u4RowAddr;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	if (devinfo.two_phyplane)
		row_addr = (((row_addr / page_per_block) << 1) * page_per_block) + (row_addr % page_per_block);

	if (((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		|| (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
		 && (devinfo.tlcControl.normaltlc) && (!mtk_block_istlc(pkCMD->u4RowAddr * mtd->writesize)))
		devinfo.tlcControl.slcopmodeEn = TRUE;
	else
		devinfo.tlcControl.slcopmodeEn = FALSE;

	if (u4ColAddr == 0) {
		mtk_nand_exec_read_page(mtd, row_addr, u4PageSize, buf, chip->oob_poi);
		pkCMD->u4ColAddr += u4PageSize + mtd->oobsize;
	}
#endif
	return 0;
}

/******************************************************************************
 *
 * Read a page to a logical address
 *
 *****************************************************************************/
static int mtk_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, int page)
{
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	u32 row_addr;
	int bRet = ERR_RTN_SUCCESS;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
	row_addr = page_in_block + mapped_block * page_per_block;
	/* MSG(INIT,"[READ] %d, %d, %d %d\n",mapped_block, block, page_in_block, page_per_block); */

	/* pr_debug("[xl] mtk_nand_read_page buf 0x%lx\n", (unsigned long)buf); */

	if (devinfo.two_phyplane)
		row_addr = page_in_block + (mapped_block << 1) * page_per_block;

	bRet = mtk_nand_exec_read_page(mtd, row_addr, mtd->writesize, buf, chip->oob_poi);
	if (bRet == ERR_RTN_SUCCESS) {
#if CFG_PERFLOG_DEBUG
		do_gettimeofday(&etimer);
		g_NandPerfLog.ReadPageTotalTime += Cal_timediff(&etimer, &stimer);
		g_NandPerfLog.ReadPageCount++;
		dump_nand_rwcount();
#endif
		return 0;
	}

	/* else
	 * return -EIO;
	 */
	return 0;
}

/* mtk_nand_read_subpage
 * return : 0 on success, On error, return error num.
 */
static int mtk_nand_read_subpage(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, int page,
				 int subpage, int subpageno)
{
	/* int block_size = 1 << (chip->phys_erase_shift); */
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	/* int page_per_block1 = page_per_block; */
	u32 block;
	int coladdr;
	u32 page_in_block;
	u32 mapped_block;
	/* bool readRetry = FALSE; */
	/* int retryCount = 0; */
	int bRet = 0;
	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize / sec_num;
	u32 row_addr;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
	coladdr = subpage * (devinfo.sectorsize + spare_per_sector);
	/* coladdr = subpage*(devinfo.sectorsize); */
	/* MSG(INIT,"[Read Subpage] %d, %d, %d %d\n",mapped_block, block, page_in_block, page_per_block); */

	row_addr = page_in_block + mapped_block * page_per_block;
	if (devinfo.two_phyplane)
		row_addr = page_in_block + (mapped_block << 1) * page_per_block;

	bRet = mtk_nand_exec_read_sector(mtd, row_addr, coladdr, devinfo.sectorsize * subpageno,
						buf, chip->oob_poi, subpageno);

	/* memset(bean_buffer, 0xFF, LPAGE); */
	/* bRet = mtk_nand_exec_read_page(mtd, page, mtd->writesize, bean_buffer, chip->oob_poi); */
	if (bRet == ERR_RTN_SUCCESS) {
#if CFG_PERFLOG_DEBUG
		do_gettimeofday(&etimer);
		g_NandPerfLog.ReadSubPageTotalTime += Cal_timediff(&etimer, &stimer);
		g_NandPerfLog.ReadSubPageCount++;
		dump_nand_rwcount();
#endif
		return 0;
	} else {
		nand_pr_err("read err:%d, rowaddr:0x%x, coladdr:0x%x, subpage:%d, subpageno:%d",
				bRet, row_addr, coladdr, subpage, subpageno);
		return -EIO;
	}
}

#ifdef CONFIG_MNTL_SUPPORT

/******************************************************************************
 *
 * Erase a block at a logical address
 *
 *****************************************************************************/
int mtk_chip_erase_blocks(struct mtd_info *mtd, int page, int page1)
{
#ifdef PWR_LOSS_SPOH
	struct timeval pl_time_write;
	suseconds_t duration;
	u32 time;
#endif
	int result;
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	struct NFI_TLC_WL_INFO  snd_tlc_wl_info;
#endif
	u32 snd_real_row_addr = 0;
	u32 real_row_addr = 0;
	u32 reg_val = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];
#endif

#ifdef _MTK_NAND_DUMMY_DRIVER_
	if (dummy_driver_debug) {
		unsigned long long time = sched_clock();

		if (!((time * 123 + 59) % 1024)) {
			nand_pr_err("[NAND_DUMMY_DRIVER] Simulate erase error at page: 0x%x",
				   page);
			return NAND_STATUS_FAIL;
		}
	}
#endif
#if CFG_2CS_NAND
	if (g_bTricky_CS)
		page = mtk_nand_cs_on(chip, NFI_TRICKY_CS, page);
#endif

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		if (devinfo.tlcControl.normaltlc) {
			NFI_TLC_GetMappedWL(page, &tlc_wl_info);
			real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);
			if (devinfo.advancedmode & MULTI_PLANE) {
				NFI_TLC_GetMappedWL(page1, &snd_tlc_wl_info);
				snd_real_row_addr = NFI_TLC_GetRowAddr(snd_tlc_wl_info.word_line_idx);
			}
		} else {
			real_row_addr = NFI_TLC_GetRowAddr(page);
			if ((devinfo.advancedmode & MULTI_PLANE) &&  page1)
				snd_real_row_addr = NFI_TLC_GetRowAddr(page1);
		}
	} else
#endif
	{
		real_row_addr = page;
		snd_real_row_addr = page1;
	}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		if ((devinfo.tlcControl.slcopmodeEn)
			&& (devinfo.tlcControl.en_slc_mode_cmd != 0xFF)) {
			reg_val = DRV_Reg16(NFI_CNFG_REG16);
			reg_val &= ~CNFG_READ_EN;
			reg_val &= ~CNFG_OP_MODE_MASK;
			reg_val |= CNFG_OP_CUST;
			DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

			mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

			reg_val = DRV_Reg32(NFI_CON_REG16);
			reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
			/* issue reset operation */
			DRV_WriteReg32(NFI_CON_REG16, reg_val);
			/* pr_info("%s %d erase SLC mode real_row_addr:0x%x\n", */
				/* __func__, __LINE__, real_row_addr); */
		} else {
			if (tlc_not_keep_erase_lvl) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(NOT_KEEP_ERASE_LVL_A19NM_CMD);

				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
	}
#endif
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
		if (devinfo.tlcControl.slcopmodeEn) {
			if (devinfo.vendor == VEND_MICRON) {
#if MLC_MICRON_SLC_MODE
				feature[0] = 0x00;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
#endif
			} else if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
	}
	PL_NAND_BEGIN(pl_time_write);
	PL_TIME_RAND_ERASE(chip, page, time);
	if ((devinfo.advancedmode & MULTI_PLANE) && snd_real_row_addr) {
		pr_debug("%s multi-plane real_row_addr:0x%x snd_real_row_addr:0x%x\n",
			__func__, real_row_addr, snd_real_row_addr);
		NFI_SET_REG16(NFI_INTR_EN_REG16, INTR_BSY_RTN_EN);
		mtk_nand_set_mode(CNFG_OP_CUST);
		mtk_nand_set_command(NAND_CMD_ERASE1);
		mtk_nand_set_address(0, real_row_addr, 0, devinfo.addr_cycle - 2);
		mtk_nand_set_command(NAND_CMD_ERASE1);
		mtk_nand_set_address(0, snd_real_row_addr, 0, devinfo.addr_cycle - 2);
		mtk_nand_set_command(NAND_CMD_ERASE2);
		wait_for_completion(&g_comp_Busy_Ret);
		mtk_nand_reset();

		result = chip->waitfunc(mtd, chip);
	} else
		result = chip->erase(mtd, real_row_addr);
	PL_NAND_RESET(time);
	PL_NAND_END(pl_time_write, duration);
	PL_TIME_ERASE(duration);

	if ((
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		(devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		||
#endif
		(devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
		&& (devinfo.tlcControl.slcopmodeEn)) {
#if MLC_MICRON_SLC_MODE
			if (devinfo.vendor == VEND_MICRON) {
				feature[0] = 0x02;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
			} else
#endif
			if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);

				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
			}
		}

	return result;
}
#endif

/******************************************************************************
 *
 * Erase a block at a logical address
 *
 *****************************************************************************/
int mtk_nand_erase_hw(struct mtd_info *mtd, int page)
{
#ifdef PWR_LOSS_SPOH
	struct timeval pl_time_write;
	suseconds_t duration;
	u32 time;
#endif
	int result;
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	struct NFI_TLC_WL_INFO  snd_tlc_wl_info;
#endif
	u32 snd_real_row_addr = 0;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 reg_val = 0;
	u32 real_row_addr = 0;
#if MLC_MICRON_SLC_MODE
	u8 feature[4];
#endif

#ifdef _MTK_NAND_DUMMY_DRIVER_
	if (dummy_driver_debug) {
		unsigned long long time = sched_clock();

		if (!((time * 123 + 59) % 1024)) {
			nand_pr_err("[NAND_DUMMY_DRIVER] Simulate erase error at page: 0x%x",
				   page);
			return NAND_STATUS_FAIL;
		}
	}
#endif
#if CFG_2CS_NAND
	if (g_bTricky_CS)
		page = mtk_nand_cs_on(chip, NFI_TRICKY_CS, page);
#endif

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		if (devinfo.tlcControl.normaltlc) {
			NFI_TLC_GetMappedWL(page, &tlc_wl_info);
			real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);
			if (devinfo.two_phyplane) {
				NFI_TLC_GetMappedWL((page + page_per_block), &snd_tlc_wl_info);
				snd_real_row_addr = NFI_TLC_GetRowAddr(snd_tlc_wl_info.word_line_idx);
			}
		} else {
			real_row_addr = NFI_TLC_GetRowAddr(page);
		}
	} else
#endif
	{
		real_row_addr = page;
		if (devinfo.two_phyplane)
			snd_real_row_addr = real_row_addr + page_per_block;
	}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
		if ((devinfo.tlcControl.slcopmodeEn)
			&& (devinfo.tlcControl.en_slc_mode_cmd != 0xFF)) {
			reg_val = DRV_Reg16(NFI_CNFG_REG16);
			reg_val &= ~CNFG_READ_EN;
			reg_val &= ~CNFG_OP_MODE_MASK;
			reg_val |= CNFG_OP_CUST;
			DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

			mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

			reg_val = DRV_Reg32(NFI_CON_REG16);
			reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
			/* issue reset operation */
			DRV_WriteReg32(NFI_CON_REG16, reg_val);
		} else {
			if (tlc_not_keep_erase_lvl) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(NOT_KEEP_ERASE_LVL_A19NM_CMD);

				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
	}
#endif
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER) {
		if (devinfo.tlcControl.slcopmodeEn) {
#if MLC_MICRON_SLC_MODE
			if (devinfo.vendor == VEND_MICRON) {
				feature[0] = 0x02;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
			} else
#endif
			if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);
				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
	}
	PL_NAND_BEGIN(pl_time_write);
	PL_TIME_RAND_ERASE(chip, page, time);
	if (devinfo.two_phyplane) {
		mtk_nand_set_mode(CNFG_OP_CUST);
		mtk_nand_set_command(NAND_CMD_ERASE1);
		mtk_nand_set_address(0, real_row_addr, 0, devinfo.addr_cycle - 2);
		mtk_nand_set_command(NAND_CMD_ERASE1);
		mtk_nand_set_address(0, snd_real_row_addr, 0, devinfo.addr_cycle - 2);
		mtk_nand_set_command(NAND_CMD_ERASE2);
		while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
			;
		mtk_nand_reset();

		result = chip->waitfunc(mtd, chip);
	} else
		result = chip->erase(mtd, real_row_addr);
	PL_NAND_RESET(time);
	PL_NAND_END(pl_time_write, duration);
	PL_TIME_ERASE(duration);

	if ((
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		(devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		||
#endif
		(devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
		&&(devinfo.tlcControl.slcopmodeEn)) {
#if MLC_MICRON_SLC_MODE
			if (devinfo.vendor == VEND_MICRON) {
				feature[0] = 0x02;
				feature[1] = 0x01;
				feature[2] = 0x00;
				feature[3] = 0x00;
				mtk_nand_SetFeature(mtd, devinfo.feature_set.FeatureSet.sfeatureCmd,
					0x91, (u8 *) &feature, 4);
				memset((u8 *) &feature, 0, 4);
				mtk_nand_GetFeature(mtd, devinfo.feature_set.FeatureSet.gfeatureCmd,
							0x91, (u8 *) &feature, 4);
				pr_info("%s %d %x %x %x %x\n", __func__, __LINE__,
					feature[0], feature[1], feature[2], feature[3]);
			} else
#endif
			if (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);

				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
			}
		}

	return result;
}

static int mtk_nand_erase(struct mtd_info *mtd, int page)
{
	int status;
	struct nand_chip *chip = mtd->priv;
	/* int block_size = 1 << (chip->phys_erase_shift); */
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	bool erase_fail = FALSE;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
	/* MSG(INIT, "[ERASE] 0x%x 0x%x\n", mapped_block, page); */
	if (devinfo.two_phyplane)
		mapped_block <<= 1;
	status = mtk_nand_erase_hw(mtd, page_in_block + page_per_block * mapped_block);

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		&& (devinfo.tlcControl.slcopmodeEn)) {
		if (status & SLC_MODE_OP_FALI) {
			erase_fail = TRUE;
			nand_pr_err("mtk_nand_erase: page %d fail", page);
		}
	} else
#endif
	{
		if (status & NAND_STATUS_FAIL)
			erase_fail = TRUE;
	}

	if (erase_fail) {
		if (devinfo.two_phyplane)
			mapped_block >>= 1;
		if (update_bmt
			((u64) ((u64) page_in_block + (u64) mapped_block * page_per_block) <<
			 chip->page_shift, UPDATE_ERASE_FAIL, NULL, NULL)) {
			nand_pr_err("Erase fail at block: 0x%x, update BMT success", mapped_block);
		} else {
			nand_pr_err("Erase fail at block: 0x%x, update BMT fail", mapped_block);
			return NAND_STATUS_FAIL;
		}
	}

#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.EraseBlockTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.EraseBlockCount++;
	dump_nand_rwcount();
#endif
	return 0;
}

/******************************************************************************
 * mtk_nand_read_multi_page_cache
 *
 * description:
 *	 read multi page data using cache read
 *
 * parameters:
 *	 struct mtd_info *mtd, struct nand_chip *chip, int page, struct mtd_oob_ops *ops
 *
 * returns:
 *	 none
 *
 * notes:
 *	 only available for nand flash support cache read.
 *	 read main data only.
 *
 *****************************************************************************/
#if 0
static int mtk_nand_read_multi_page_cache(struct mtd_info *mtd, struct nand_chip *chip, int page,
					  struct mtd_oob_ops *ops)
{
	int res = -EIO;
	int len = ops->len;
	struct mtd_ecc_stats stat = mtd->ecc_stats;
	uint8_t *buf = ops->datbuf;

	if (!mtk_nand_ready_for_read(chip, page, 0, true, buf))
		return -EIO;

	while (len > 0) {
		mtk_nand_set_mode(CNFG_OP_CUST);
		DRV_WriteReg32(NFI_CON_REG16, 8 << CON_NFI_SEC_SHIFT);

		if (len > mtd->writesize) {	/* remained more than one page */
			if (!mtk_nand_set_command(0x31))	/* todo: add cache read command */
				goto ret;
		} else {
			if (!mtk_nand_set_command(0x3f))	/* last page remained */
				goto ret;
		}

		mtk_nand_status_ready(STA_NAND_BUSY);

#ifdef __INTERNAL_USE_AHB_MODE__
		/* if (!mtk_nand_dma_read_data(buf, mtd->writesize)) */
		if (!mtk_nand_read_page_data(mtd, buf, mtd->writesize))
			goto ret;
#else
		if (!mtk_nand_mcu_read_data(mtd, buf, mtd->writesize))
			goto ret;
#endif

		/* get ecc error info */
		mtk_nand_check_bch_error(mtd, buf, 3, page);
		ECC_Decode_End();

		page++;
		len -= mtd->writesize;
		buf += mtd->writesize;
		ops->retlen += mtd->writesize;

		if (len > 0) {
			ECC_Decode_Start();
			mtk_nand_reset();
		}

	}

	res = 0;

ret:
	mtk_nand_stop_read();

	if (res)
		return res;

	if (mtd->ecc_stats.failed > stat.failed) {
		pr_debug("ecc fail happened\n");
		return -EBADMSG;
	}

	return mtd->ecc_stats.corrected - stat.corrected ? -EUCLEAN : 0;
}
#endif

/******************************************************************************
 * mtk_nand_read_oob_raw
 *
 * DESCRIPTION:
 *	 Read oob data
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, const uint8_t *buf, int addr, int len
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 this function read raw oob data out of flash, so need to re-organise
 *	 data format before using.
 *	 len should be times of 8, call this after nand_get_device.
 *	 Should notice, this function read data without ECC protection.
 *
 *****************************************************************************/
static int mtk_nand_read_oob_raw(struct mtd_info *mtd, uint8_t *buf, int page_addr, int len)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	u32 col_addr = 0;
	u32 sector = 0;
	int res = 0;
	u32 colnob = 2, rawnob = devinfo.addr_cycle - 2;
	int randomread = 0;
	int read_len = 0;
	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize / sec_num;
	u32 sector_size = NAND_SECTOR_SIZE;

	if (devinfo.sectorsize == 1024)
		sector_size = 1024;

	if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf) {
		pr_warn("[%s] invalid parameter, len: %d, buf: %p\n", __func__, len,
			   buf);
		return -EINVAL;
	}
	if (len > spare_per_sector)
		randomread = 1;

	if (!randomread || !(devinfo.advancedmode & RAMDOM_READ)) {
		while (len > 0) {
			read_len = min(len, spare_per_sector);
			col_addr = sector_size +
				sector * (sector_size + spare_per_sector);	/* TODO: Fix this hard-code 16 */
			if (!mtk_nand_ready_for_read(chip,
					page_addr, col_addr, sec_num, false, NULL, NORMAL_READ)) {
				pr_warn("mtk_nand_ready_for_read return failed\n");
				res = -EIO;
				goto error;
			}
			if (!mtk_nand_mcu_read_data(mtd,
					buf + spare_per_sector * sector, read_len)) {	/* TODO: and this 8 */
				pr_warn("mtk_nand_mcu_read_data return failed\n");
				res = -EIO;
				goto error;
			}
			mtk_nand_stop_read();
			/* dump_data(buf + 16 * sector,16); */
			sector++;
			len -= read_len;

		}
	} else {		/* should be 64 */

		col_addr = sector_size;
		if (chip->options & NAND_BUSWIDTH_16)
			col_addr /= 2;

		if (!mtk_nand_reset())
			goto error;

		mtk_nand_set_mode(0x6000);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
		DRV_WriteReg32(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

		mtk_nand_set_autoformat(false);

		if (!mtk_nand_set_command(NAND_CMD_READ0))
			goto error;
		/* 1 FIXED ME: For Any Kind of AddrCycle */
		if (!mtk_nand_set_address(col_addr, page_addr, colnob, rawnob))
			goto error;

		if (!mtk_nand_set_command(NAND_CMD_READSTART))
			goto error;
		if (!mtk_nand_status_ready(STA_NAND_BUSY))
			goto error;

		read_len = min(len, spare_per_sector);
		if (!mtk_nand_mcu_read_data(mtd, buf + spare_per_sector * sector, read_len)) {	/* TODO: and this 8 */
			pr_warn("mtk_nand_mcu_read_data return failed first 16\n");
			res = -EIO;
			goto error;
		}
		sector++;
		len -= read_len;
		mtk_nand_stop_read();
		while (len > 0) {
			read_len = min(len, spare_per_sector);
			if (!mtk_nand_set_command(0x05))
				goto error;

			col_addr = sector_size + sector * (sector_size + 16);	/* :TODO_JP careful 16 */
			if (chip->options & NAND_BUSWIDTH_16)
				col_addr /= 2;
			DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);
			DRV_WriteReg16(NFI_ADDRNOB_REG16, 2);
			DRV_WriteReg32(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

			if (!mtk_nand_status_ready(STA_ADDR_STATE))
				goto error;

			if (!mtk_nand_set_command(0xE0))
				goto error;
			if (!mtk_nand_status_ready(STA_NAND_BUSY))
				goto error;
			if (!mtk_nand_mcu_read_data(mtd,
					buf + spare_per_sector * sector, read_len)) {	/* TODO: and this 8 */
				pr_warn("mtk_nand_mcu_read_data return failed first 16\n");
				res = -EIO;
				goto error;
			}
			mtk_nand_stop_read();
			sector++;
			len -= read_len;
		}
		/* dump_data(&testbuf[16],16); */
		/* pr_debug(KERN_ERR "\n"); */
	}
error:
	NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BRD);
	return res;
}

static int mtk_nand_write_oob_raw(struct mtd_info *mtd, const uint8_t *buf, int page_addr, int len)
{
	struct nand_chip *chip = mtd->priv;
	/* int i; */
	u32 col_addr = 0;
	u32 sector = 0;
	/* int res = 0; */
	/* u32 colnob=2, rawnob=devinfo.addr_cycle-2; */
	/* int randomread =0; */
	int write_len = 0;
	int status;
	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize / sec_num;
	u32 sector_size = NAND_SECTOR_SIZE;

	if (devinfo.sectorsize == 1024)
		sector_size = 1024;

	if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf) {
		pr_warn("[%s] invalid parameter, len: %d, buf: %p\n", __func__, len,
			   buf);
		return -EINVAL;
	}

	while (len > 0) {
		write_len = min(len, spare_per_sector);
		col_addr = sector * (sector_size + spare_per_sector) + sector_size;
		if (!mtk_nand_ready_for_write(chip, page_addr, col_addr, false, NULL))
			return -EIO;

		if (!mtk_nand_mcu_write_data(mtd, buf + sector * spare_per_sector, write_len))
			return -EIO;

		(void)mtk_nand_check_RW_count(write_len);
		NFI_CLN_REG32(NFI_CON_REG16, CON_NFI_BWR);
		(void)mtk_nand_set_command(NAND_CMD_PAGEPROG);

		while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
			;

		status = chip->waitfunc(mtd, chip);
		if (status & NAND_STATUS_FAIL) {
			pr_debug("status: %d\n", status);
			return -EIO;
		}

		len -= write_len;
		sector++;
	}

	return 0;
}

static int mtk_nand_write_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	/* u8 *buf = chip->oob_poi; */
	int i, iter;

	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize / sec_num;

	memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

	/* copy ecc data */
	for (i = 0; i < chip->ecc.layout->eccbytes; i++) {
		iter = (i / OOB_AVAI_PER_SECTOR) * spare_per_sector + OOB_AVAI_PER_SECTOR +
			i % OOB_AVAI_PER_SECTOR;
		local_oob_buf[iter] = chip->oob_poi[chip->ecc.layout->eccpos[i]];
		/* chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter]; */
	}

	/* copy FDM data */
	for (i = 0; i < sec_num; i++) {
		memcpy(&local_oob_buf[i * spare_per_sector],
			   &chip->oob_poi[i * OOB_AVAI_PER_SECTOR], OOB_AVAI_PER_SECTOR);
	}

	return mtk_nand_write_oob_raw(mtd, local_oob_buf, page, mtd->oobsize);
}

static int mtk_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	/* int block_size = 1 << (chip->phys_erase_shift); */
	int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	/* int page_per_block1 = page_per_block; */
	u32 block;
	u16 page_in_block;
	u32 mapped_block;

	/* block = page / page_per_block1; */
	/* mapped_block = get_mapping_block_index(block); */
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);

	if (mapped_block != block)
		set_bad_index_to_oob(chip->oob_poi, block);
	else
		set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);

	if (mtk_nand_write_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block /* page */)) {
		nand_pr_err("write oob fail at block: 0x%x, page: 0x%x", mapped_block,
			page_in_block);
		if (update_bmt((u64) ((u64) page_in_block + (u64) mapped_block * page_per_block) <<
			 chip->page_shift, UPDATE_WRITE_FAIL, NULL, chip->oob_poi)) {
			pr_debug("Update BMT success\n");
			return 0;
		}
		nand_pr_err("Update BMT fail");
		return -EIO;
	}

	return 0;
}

int mtk_nand_block_markbad_hw(struct mtd_info *mtd, loff_t offset)
{
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	struct nand_chip *chip = mtd->priv;
#endif
	int block; /* = (int)(offset / (devinfo.blocksize * 1024)); */
	int page; /* = block * (devinfo.blocksize * 1024 / devinfo.pagesize); */
	int ret;
	loff_t temp;
	u8 buf[8];

	temp = offset;
	do_div(temp, ((devinfo.blocksize * 1024) & 0xFFFFFFFF));
	block = (u32) temp;

	if (devinfo.two_phyplane)
		block <<= 1;

	page = block * (devinfo.blocksize * 1024 / devinfo.pagesize);

	memset(buf, 0xFF, 8);
	buf[0] = 0;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if (mtk_is_normal_tlc_nand())
		ret = mtk_nand_tlc_block_mark(mtd, chip, block);
	else
#endif
	ret = mtk_nand_write_oob_raw(mtd, buf, page, 8);
	return ret;
}

static int mtk_nand_block_markbad(struct mtd_info *mtd, loff_t offset)
{
	struct nand_chip *chip = mtd->priv;
	u32 block; /*  = (u32)(offset  / (devinfo.blocksize * 1024)); */
	int page; /* = block * (devinfo.blocksize * 1024 / devinfo.pagesize); */
	u32 mapped_block;
	int ret;
	loff_t temp;

	temp = offset;
	do_div(temp, ((devinfo.blocksize * 1024) & 0xFFFFFFFF));
	block = (u32) temp;
	page = block * (devinfo.blocksize * 1024 / devinfo.pagesize);

	nand_get_device(mtd, FL_WRITING);

	page = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);
	ret = mtk_nand_block_markbad_hw(mtd, mapped_block * (devinfo.blocksize * 1024));

	nand_release_device(mtd);

	return ret;
}

int mtk_nand_read_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	int i;
	u8 iter = 0;

	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
	int spare_per_sector = mtd->oobsize / sec_num;
#ifdef TESTTIME
	unsigned long long time1, time2;

	time1 = sched_clock();
#endif

	if (mtk_nand_read_oob_raw(mtd, chip->oob_poi, page, mtd->oobsize)) {
		/* pr_debug(KERN_ERR "[%s]mtk_nand_read_oob_raw return failed\n", __FUNCTION__); */
		return -EIO;
	}
#ifdef TESTTIME
	time2 = sched_clock() - time1;
	if (!readoobflag) {
		readoobflag = 1;
		nand_info("time is %llu", time2);
	}
#endif

	/* adjust to ecc physical layout to memory layout */
	/*********************************************************/
	/* FDM0 | ECC0 | FDM1 | ECC1 | FDM2 | ECC2 | FDM3 | ECC3 */
	/*	8B	|  8B  |  8B  |  8B  |	8B	|  8B  |  8B  |  8B  */
	/*********************************************************/

	memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

	/* copy ecc data */
	for (i = 0; i < chip->ecc.layout->eccbytes; i++) {
		iter = (i / OOB_AVAI_PER_SECTOR) * spare_per_sector + OOB_AVAI_PER_SECTOR +
			i % OOB_AVAI_PER_SECTOR;
		chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
	}

	/* copy FDM data */
	for (i = 0; i < sec_num; i++) {
		memcpy(&chip->oob_poi[i * OOB_AVAI_PER_SECTOR],
			   &local_oob_buf[i * spare_per_sector], OOB_AVAI_PER_SECTOR);
	}

	return 0;
}

static int mtk_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	/* int block_size = 1 << (chip->phys_erase_shift); */
	/* int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift); */
	/* int block; */
	/* u16 page_in_block; */
	/* int mapped_block; */
	/* u8* buf = (u8*)kzalloc(mtd->writesize, GFP_KERNEL); */

	/* page = mtk_nand_page_transform(mtd,chip,page,&block,&mapped_block); */
#if 0
	if (block_size != mtd->erasesize)
		page_per_block1 = page_per_block >> 1;

	block = page / page_per_block1;
	mapped_block = get_mapping_block_index(block);
	if (block_size != mtd->erasesize)
		page_in_block = devinfo.feature_set.PairPage[page % page_per_block1];
	else
		page_in_block = page % page_per_block1;

	mtk_nand_read_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block);
#else
	mtk_nand_read_page(mtd, chip, temp_buffer_16_align, page);
	/* kfree(buf); */
#endif

	return 0;		/* the return value is sndcmd */
}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
bool mtk_nand_slc_write_wodata(struct nand_chip *chip, u32 page)
{
	bool bRet = FALSE;
	bool slc_en;
	u32 real_row_addr;
	u32 reg_val;
	struct NFI_TLC_WL_INFO tlc_wl_info;

	NFI_TLC_GetMappedWL(page, &tlc_wl_info);
	real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);

	reg_val = DRV_Reg16(NFI_CNFG_REG16);
	reg_val &= ~CNFG_READ_EN;
	reg_val &= ~CNFG_OP_MODE_MASK;
	reg_val |= CNFG_OP_CUST;
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(0xA2);

	reg_val = DRV_Reg32(NFI_CON_REG16);
	reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
	/* issue reset operation */
	DRV_WriteReg32(NFI_CON_REG16, reg_val);

	mtk_nand_set_mode(CNFG_OP_PRGM);
	mtk_nand_set_command(NAND_CMD_SEQIN);

	mtk_nand_set_address(0, real_row_addr, 2, 3);

	mtk_nand_set_command(NAND_CMD_PAGEPROG);

	slc_en = devinfo.tlcControl.slcopmodeEn;
	devinfo.tlcControl.slcopmodeEn = TRUE;
	bRet = !mtk_nand_read_status();
	devinfo.tlcControl.slcopmodeEn = slc_en;

	if (bRet)
		pr_warn("mtk_nand_slc_write_wodata: page %d is bad\n", page);

	return bRet;
}

u64 mtk_nand_device_size(void)
{
	u64 totalsize;

	totalsize = (u64)devinfo.totalsize << 10;
	return totalsize;
}
#endif

int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	int page_addr = (int)(ofs >> chip->page_shift);
	u32 block, mapped_block;
	int ret;
	unsigned int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	int sec_num = 1 << (chip->page_shift - host->hw->nand_sec_shift);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	bool bRet;
#endif

	/* unsigned char oob_buf[128]; */
	/* char* buf = (char*) kmalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL); */

	/* page_addr = mtk_nand_page_transform(mtd, chip, page_addr, &block, &mapped_block); */

#if !defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	page_addr &= ~(page_per_block - 1);
#endif
	memset(temp_buffer_16_align, 0xFF, LPAGE);

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		&& (devinfo.vendor == VEND_SANDISK)
		&& mtk_nand_IsBMTPOOL(ofs)) {
		page_addr = mtk_nand_page_transform(mtd, chip, page_addr, &block, &mapped_block);
		if (devinfo.two_phyplane)
			mapped_block <<= 1;

		bRet = mtk_nand_slc_write_wodata(chip, mapped_block * page_per_block);
		/*pr_warn("after mtk_nand_slc_write_wodata\n");*/
		if (bRet)
			ret = 1;
		else
			ret = 0;
		if ((devinfo.two_phyplane) && (!ret)) {
			bRet = mtk_nand_slc_write_wodata(chip, (mapped_block + 1) * page_per_block);
			if (bRet)
				ret = 1;
			else
				ret = 0;
		}
		return ret;
	}
#endif
	ret = mtk_nand_read_subpage(mtd, chip, temp_buffer_16_align, (ofs >> chip->page_shift), 0, 1);

	page_addr = mtk_nand_page_transform(mtd, chip, page_addr, &block, &mapped_block);
	/* ret = mtk_nand_exec_read_page(mtd, page_addr+mapped_block*page_per_block, mtd->writesize, buf, oob_buf); */
	if (ret != 0) {
		pr_warn("mtk_nand_read_oob_raw return error %d\n", ret);
	/* kfree(buf); */
		return 1;
	}

	if (chip->oob_poi[0] != 0xff) {
		pr_debug("Bad block detected at 0x%x, oob_buf[0] is 0x%x\n",
			   block * page_per_block, chip->oob_poi[0]);
		/* kfree(buf); */
		/* dump_nfi(); */
		return 1;
	}

	if (devinfo.two_phyplane) {
		ret = mtk_nand_read_subpage(mtd, chip, temp_buffer_16_align, (ofs >> chip->page_shift), sec_num / 2, 1);
		if (ret != 0) {
			pr_warn("mtk_nand_read_oob_raw return error %d\n", ret);
			return 1;
		}
		if (chip->oob_poi[0] != 0xff) {
			pr_debug("Bad block detected at 0x%x, oob_buf[0] is 0x%x\n",
				block * page_per_block, chip->oob_poi[0]);
			return 1;
		}
	}
	/* kfree(buf); */
	return 0;		/* everything is OK, good block */
}

static int mtk_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int chipnr = 0;

	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	int block; /* = (int)(ofs / (devinfo.blocksize * 1024));*/
	int mapped_block;
	int page = (int)(ofs >> chip->page_shift);
	int page_in_block;
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	loff_t temp;
	int ret;

	temp = ofs;
	do_div(temp, ((devinfo.blocksize * 1024) & 0xFFFFFFFF));
	block = (int) temp;

	if (getchip) {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		temp = mtk_nand_device_size();
		if (ofs >= temp)
			chipnr = 1;
		else
			chipnr = 0;
#else
		chipnr = (int)(ofs >> chip->chip_shift);
#endif
		nand_get_device(mtd, FL_READING);
		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
	}
	/* page = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block); */
	/* mapped_block = get_mapping_block_index(block); */

	ret = mtk_nand_block_bad_hw(mtd, ofs);
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);

	if (ret) {
		pr_debug("Unmapped bad block: 0x%x %d\n", mapped_block, ret);
		if (update_bmt((u64) ((u64) page_in_block + (u64) mapped_block * page_per_block) <<
			 chip->page_shift, UPDATE_UNMAPPED_BLOCK, NULL, NULL)) {
			pr_debug("Update BMT success\n");
			ret = 0;
		} else {
			nand_pr_err("Update BMT fail");
			ret = 1;
		}
	}

	if (getchip)
		nand_release_device(mtd);

	return ret;
}

/******************************************************************************
 * mtk_nand_init_size
 *
 * DESCRIPTION:
 *	 initialize the pagesize, oobsize, blocksize
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, struct nand_chip *this, u8 *id_data
 *
 * RETURNS:
 *	 Buswidth
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/

int mtk_nand_init_size(struct mtd_info *mtd, struct nand_chip *this, u8 *id_data)
{
	/* Get page size */
	mtd->writesize = devinfo.pagesize;

	/* Get oobsize */
	mtd->oobsize = devinfo.sparesize;

	/* Get blocksize. */
	mtd->erasesize = devinfo.blocksize * 1024;
	/* Get buswidth information */
	if (devinfo.iowidth == 16)
		return NAND_BUSWIDTH_16;
	else
		return 0;
}
EXPORT_SYMBOL(mtk_nand_init_size);

int mtk_nand_write_tlc_wl_hw(struct mtd_info *mtd, struct nand_chip *chip,
				 uint8_t *buf, u32 wl, enum NFI_TLC_PG_CYCLE program_cycle)
{
	u32 page;
	uint8_t *temp_buf = NULL;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif

	devinfo.tlcControl.slcopmodeEn = FALSE;

	tlc_program_cycle = program_cycle;
	page = wl * 3;

	temp_buf = buf;
	memcpy(local_tlc_wl_buffer, temp_buf, mtd->writesize);
	if (mtk_nand_exec_write_page(mtd, page, mtd->writesize, local_tlc_wl_buffer, chip->oob_poi)) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}

	temp_buf += mtd->writesize;
	memcpy(local_tlc_wl_buffer, temp_buf, mtd->writesize);
	if (mtk_nand_exec_write_page(mtd, page + 1, mtd->writesize, local_tlc_wl_buffer, chip->oob_poi)) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}

	temp_buf += mtd->writesize;
	memcpy(local_tlc_wl_buffer, temp_buf, mtd->writesize);
	if (mtk_nand_exec_write_page(mtd, page + 2, mtd->writesize, local_tlc_wl_buffer, chip->oob_poi)) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.WritePageTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WritePageCount++;
	dump_nand_rwcount();
#endif
	return 0;
}

int mtk_nand_write_tlc_wl(struct mtd_info *mtd, struct nand_chip *chip,
				 uint8_t *buf, u32 wl, enum NFI_TLC_PG_CYCLE program_cycle)
{
	int bRet;

	bRet = mtk_nand_write_tlc_wl_hw(mtd, chip, buf, wl, program_cycle);
	return bRet;
}

int mtk_nand_write_tlc_block_hw(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, u32 mapped_block)
{
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 index;
	int bRet;
	u32 base_wl_index;
	u8 *temp_buf = NULL;

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	base_wl_index = mapped_block * page_per_block / 3;

	for (index = 0; index < (page_per_block / 3); index++) {
		if (index == 0) {
			temp_buf = buf + (index * 3 * mtd->writesize);
			bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index, PROGRAM_1ST_CYCLE);
			if (bRet != 0)
				break;

			temp_buf = buf + ((index + 1) * 3 * mtd->writesize);
			bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index + 1, PROGRAM_1ST_CYCLE);
			if (bRet != 0)
				break;

			temp_buf = buf + (index * 3 * mtd->writesize);
			bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index, PROGRAM_2ND_CYCLE);
			if (bRet != 0)
				break;
		}

		if ((index + 2) < (page_per_block / 3)) {
			temp_buf = buf + ((index + 2) * 3 * mtd->writesize);
			bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index + 2, PROGRAM_1ST_CYCLE);
			if (bRet != 0)
				break;
		}

		if ((index + 1) < (page_per_block / 3)) {
			temp_buf = buf + ((index + 1) * 3 * mtd->writesize);
			bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index + 1, PROGRAM_2ND_CYCLE);
			if (bRet != 0)
				break;
		}

		temp_buf = buf + (index * 3 * mtd->writesize);
		bRet = mtk_nand_write_tlc_wl(mtd, chip, temp_buf, base_wl_index + index, PROGRAM_3RD_CYCLE);
		if (bRet != 0)
			break;

	}
	if (bRet != 0)
		return -EIO;
#else
	base_wl_index = mapped_block * page_per_block;
	tlc_cache_program = TRUE;
	for (index = 0; index < page_per_block; index++) {
		if (index == (page_per_block - 1))
			tlc_cache_program = FALSE;
		temp_buf = buf + (index * mtd->writesize);
		bRet = mtk_nand_exec_write_page(mtd, base_wl_index+index, mtd->writesize, temp_buf, chip->oob_poi);
		if (bRet != 0)
			break;
	}
	tlc_cache_program = FALSE;
#endif
	return 0;
}


int mtk_nand_write_tlc_block(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, u32 page)
{
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	int bRet;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif

	if (devinfo.NAND_FLASH_TYPE != NAND_FLASH_TLC && devinfo.NAND_FLASH_TYPE != NAND_FLASH_MLC_HYBER) {
		nand_pr_err("error : not tlc nand");
		return -EIO;
	}

	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);

	if (page_in_block != 0) {
		nand_pr_err("error : normal tlc block program is not block aligned");
		return -EIO;
	}

	memset(chip->oob_poi, 0xff, mtd->oobsize);

	if (mapped_block != block)
		set_bad_index_to_oob(chip->oob_poi, block);
	else
		set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);

	if (devinfo.two_phyplane)
		mapped_block <<= 1;

	bRet = mtk_nand_write_tlc_block_hw(mtd, chip, buf, mapped_block);

	if (bRet != 0) {
		if (devinfo.two_phyplane)
			mapped_block >>= 1;

		nand_pr_err("write fail at block: 0x%x, page: 0x%x", mapped_block, page_in_block);
		if (update_bmt
			((u64)((u64)page_in_block + (u64)mapped_block * page_per_block) << chip->page_shift,
				UPDATE_WRITE_FAIL, (u8 *) buf, chip->oob_poi)) {
			pr_debug("Update BMT success\n");
			return 0;
		}
		nand_pr_err("Update BMT fail");
		return -EIO;
	}
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.WritePageTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WritePageCount++;
	dump_nand_rwcount();
#endif
	return 0;
}

bool mtk_is_tlc_nand(void)
{
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		return TRUE;
	else
		return FALSE;
}

bool mtk_is_normal_tlc_nand(void)
{
	if ((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
			 && (devinfo.tlcControl.normaltlc))
		return TRUE;
	if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER)
		return TRUE;

	return FALSE;
}

int mtk_nand_tlc_wl_mark(struct mtd_info *mtd, struct nand_chip *chip,
				 uint8_t *buf, u32 wl, enum NFI_TLC_PG_CYCLE program_cycle)
{
	u32 page;
#if CFG_PERFLOG_DEBUG
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif

	devinfo.tlcControl.slcopmodeEn = FALSE;

	tlc_program_cycle = program_cycle;
	page = wl * 3;

	if (mtk_nand_exec_write_page(mtd, page, mtd->writesize, buf, (buf + mtd->writesize))) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}

	if (mtk_nand_exec_write_page(mtd, page + 1, mtd->writesize, buf, (buf + mtd->writesize))) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}

	if (mtk_nand_exec_write_page(mtd, page + 2, mtd->writesize, buf, (buf + mtd->writesize))) {
		nand_pr_err("write fail at wl: 0x%x, page: 0x%x", wl, page);

		return -EIO;
	}
#if CFG_PERFLOG_DEBUG
	do_gettimeofday(&etimer);
	g_NandPerfLog.WritePageTotalTime += Cal_timediff(&etimer, &stimer);
	g_NandPerfLog.WritePageCount++;
	dump_nand_rwcount();
#endif
	return 0;
}

int mtk_nand_tlc_block_mark(struct mtd_info *mtd, struct nand_chip *chip, u32 mapped_block)
{
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 index;
	int bRet;
	u32 base_wl_index;
	u8 *buf = local_tlc_wl_buffer;

	memset(buf, 0xAA, LPAGE + LSPARE);

	if (devinfo.tlcControl.slcopmodeEn) {
		if (mtk_nand_exec_write_page
				(mtd, mapped_block * page_per_block, mtd->writesize, buf, (buf + mtd->writesize))) {
			nand_pr_err("mark fail at page: 0x%x", mapped_block * page_per_block);

			return -EIO;
		}
	} else {
		base_wl_index = mapped_block * page_per_block / 3;

		for (index = 0; index < (page_per_block / 3); index++) {
			if (index == 0) {
				bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf,
					base_wl_index + index, PROGRAM_1ST_CYCLE);
				if (bRet != 0)
					break;

				bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf,
					base_wl_index + index + 1, PROGRAM_1ST_CYCLE);
				if (bRet != 0)
					break;

				bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf,
					base_wl_index + index, PROGRAM_2ND_CYCLE);
				if (bRet != 0)
					break;
			}

			if ((index + 2) < (page_per_block / 3)) {
				bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf,
					base_wl_index + index + 2, PROGRAM_1ST_CYCLE);
				if (bRet != 0)
					break;
			}

			if ((index + 1) < (page_per_block / 3)) {
				bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf,
					base_wl_index + index + 1, PROGRAM_2ND_CYCLE);
				if (bRet != 0)
					break;
			}

			bRet = mtk_nand_tlc_wl_mark(mtd, chip, buf, base_wl_index + index, PROGRAM_3RD_CYCLE);
			if (bRet != 0)
				break;

		}
		if (bRet != 0)
			return -EIO;
		return 0;
	}
	return 0;
}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
int mtk_nand_cache_read_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				u8 *pFDMBuf, u32 readSize)
{
	u8 *buf;
	int bRet = ERR_RTN_SUCCESS;
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = u4PageSize >> host->hw->nand_sec_shift;
	u32 backup_corrected, backup_failed;
	bool readReady;
	/* int retryCount = 0; */
	u32 tempBitMap, bitMap;
#ifdef NAND_PFM
	struct timeval pfm_time_read;
#endif
	struct NFI_TLC_WL_INFO  tlc_wl_info;
	struct NFI_TLC_WL_INFO  pre_tlc_wl_info;
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	bool tlc_left_plane = TRUE;
	unsigned int phys = 0;
#endif
	u32 reg_val = 0;
	u32 real_row_addr = 0;
	u32 logical_plane_num = 1;
	u32 data_sector_num = 0;
	u8	*temp_byte_ptr = NULL;
	u8	*spare_ptr = NULL;
	u32 readCount;
	u32 rSize = readSize;
	u32 remainSize;
	u8 *dataBuf = pPageBuf;
	u32 read_count;
	u32 u4RowAddr_ran = u4RowAddr;

	PFM_BEGIN(pfm_time_read);
	tempBitMap = 0;

	buf = local_buffer_16_align;
	backup_corrected = mtd->ecc_stats.corrected;
	backup_failed = mtd->ecc_stats.failed;
	bitMap = 0;
	data_sector_num = u4SecNum;
	/* mtk_nand_interface_switch(mtd); */
	logical_plane_num = 1;
	readCount = 0;
	temp_byte_ptr = buf;
	spare_ptr = pFDMBuf;
	tlc_wl_info.wl_pre = WL_LOW_PAGE; /* init for build warning*/
	tlc_wl_info.word_line_idx = u4RowAddr;
	if (likely(devinfo.tlcControl.normaltlc)) {
		NFI_TLC_GetMappedWL(u4RowAddr, &tlc_wl_info);
		real_row_addr = NFI_TLC_GetRowAddr(tlc_wl_info.word_line_idx);
		if (unlikely(devinfo.tlcControl.pPlaneEn)) {
			tlc_left_plane = TRUE;
			logical_plane_num = 2;
			data_sector_num /= 2;
			real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
		}
	} else
		real_row_addr = NFI_TLC_GetRowAddr(u4RowAddr);
	pre_tlc_wl_info.wl_pre = tlc_wl_info.wl_pre;
	pre_tlc_wl_info.word_line_idx = tlc_wl_info.word_line_idx;
	read_count = 0;

	do {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if (devinfo.tlcControl.slcopmodeEn) {
			if (devinfo.tlcControl.en_slc_mode_cmd != 0xFF) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

				mtk_nand_set_command(devinfo.tlcControl.en_slc_mode_cmd);

				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		} else {
			if (devinfo.tlcControl.normaltlc) {
				reg_val = DRV_Reg16(NFI_CNFG_REG16);
				reg_val &= ~CNFG_READ_EN;
				reg_val &= ~CNFG_OP_MODE_MASK;
				reg_val |= CNFG_OP_CUST;
				DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
				if (tlc_wl_info.wl_pre == WL_LOW_PAGE)
					mtk_nand_set_command(LOW_PG_SELECT_CMD);
				else if (tlc_wl_info.wl_pre == WL_MID_PAGE)
					mtk_nand_set_command(MID_PG_SELECT_CMD);
				else if (tlc_wl_info.wl_pre == WL_HIGH_PAGE)
					mtk_nand_set_command(HIGH_PG_SELECT_CMD);

				reg_val = DRV_Reg32(NFI_CON_REG16);
				reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
				/* issue reset operation */
				DRV_WriteReg32(NFI_CON_REG16, reg_val);
			}
		}
		reg_val = 0;
#endif
	mtk_nand_turn_on_randomizer(u4RowAddr_ran, 0, 0);
	u4RowAddr_ran++;

	if (unlikely(read_count == 0)) {
		readReady = mtk_nand_ready_for_read(nand, real_row_addr, 0, data_sector_num, true, buf, NORMAL_READ);
		read_count++;
#if 1
		pre_tlc_wl_info.wl_pre = tlc_wl_info.wl_pre;
		pre_tlc_wl_info.word_line_idx = tlc_wl_info.word_line_idx;
		if (devinfo.tlcControl.slcopmodeEn) {
			tlc_wl_info.word_line_idx++;
			real_row_addr++;
		} else {
			if (tlc_wl_info.wl_pre == WL_HIGH_PAGE) {
				tlc_wl_info.wl_pre = WL_LOW_PAGE;
				tlc_wl_info.word_line_idx++;
				real_row_addr++;
			} else
				tlc_wl_info.wl_pre++;
		}
		mtk_nand_reset();
		continue;
#endif

	} else if (unlikely(rSize <= u4PageSize)) {
		readReady = mtk_nand_ready_for_read(nand, real_row_addr, 0, data_sector_num, true, buf, AD_CACHE_FINAL);
	} else {
		readReady = mtk_nand_ready_for_read(nand, real_row_addr, 0, data_sector_num, true, buf, AD_CACHE_READ);
	}
	if (likely(readReady)) {
		while (logical_plane_num) {
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn) {
					if (devinfo.tlcControl.pPlaneEn)
						real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);

					reg_val = DRV_Reg32(NFI_CON_REG16);
					reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
					/* issue reset operation */
					DRV_WriteReg32(NFI_CON_REG16, reg_val);
					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val &= ~CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_CUST;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

					mtk_nand_set_command(CHANGE_COLUNM_ADDR_1ST_CMD);
					mtk_nand_set_address(0, real_row_addr, 2, devinfo.addr_cycle - 2);
					mtk_nand_set_command(CHANGE_COLUNM_ADDR_2ND_CMD);

					reg_val = DRV_Reg16(NFI_CNFG_REG16);
					reg_val |= CNFG_READ_EN;
					reg_val &= ~CNFG_OP_MODE_MASK;
					reg_val |= CNFG_OP_READ;
					DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
				}
			}

			mtk_dir = DMA_FROM_DEVICE;
			sg_init_one(&mtk_sg, temp_byte_ptr, (data_sector_num * (1 << host->hw->nand_sec_shift)));
			dma_map_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
			phys = mtk_sg.dma_address;
#if __INTERNAL_USE_AHB_MODE__
			if (!phys)
				pr_warn("[%s]convert virt addr (%lx) to phys add (%x)fail!!!",
					__func__, (unsigned long) temp_byte_ptr, phys);
			else
				DRV_WriteReg32(NFI_STRADDR_REG32, phys);
#endif

			DRV_WriteReg32(NFI_CON_REG16, data_sector_num << CON_NFI_SEC_SHIFT);

			if (g_bHwEcc)
				ECC_Decode_Start();
#endif
			if (!mtk_nand_read_page_data(mtd, temp_byte_ptr,
				data_sector_num * (1 << host->hw->nand_sec_shift))) {
				nand_pr_err("mtk_nand_read_page_data fail");
				bRet = ERR_RTN_FAIL;
			}
			dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
			if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
				nand_pr_err("mtk_nand_status_ready fail");
				bRet = ERR_RTN_FAIL;
			}
			if (g_bHwEcc) {
				if (!mtk_nand_check_dececc_done(data_sector_num)) {
					nand_pr_err("mtk_nand_check_dececc_done fail 0x%x", u4RowAddr);
					bRet = ERR_RTN_FAIL;
				}
			}
			/* mtk_nand_read_fdm_data(spare_ptr, data_sector_num); no need*/
			if (g_bHwEcc) {
				if (!mtk_nand_check_bch_error
					(mtd, temp_byte_ptr, spare_ptr, data_sector_num - 1, u4RowAddr, &tempBitMap)) {

				bRet = ERR_RTN_BCH_FAIL;
			}
		}
		mtk_nand_stop_read();

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
			if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
				if (devinfo.tlcControl.needchangecolumn)
					DRV_WriteReg16(NFI_TLC_RD_WHR2_REG16, 0x055);

				if (logical_plane_num == 2) {
					tlc_left_plane = FALSE;
					spare_ptr += (host->hw->nand_fdm_size * data_sector_num);
					temp_byte_ptr += (data_sector_num * (1 << host->hw->nand_sec_shift));
				}
			}
#endif
			logical_plane_num--;

			if (bRet == ERR_RTN_BCH_FAIL)
				break;
		}
	}
#ifndef CONFIG_MTK_TLC_NAND_SUPPORT
	else
		dma_unmap_sg(mtk_dev, &mtk_sg, 1, mtk_dir);
#endif

		mtk_nand_turn_off_randomizer();

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if ((devinfo.tlcControl.slcopmodeEn)
			&& (devinfo.tlcControl.dis_slc_mode_cmd != 0xFF)) {
			reg_val = DRV_Reg32(NFI_CON_REG16);
			reg_val |= CON_FIFO_FLUSH|CON_NFI_RST;
			/* issue reset operation */
			DRV_WriteReg32(NFI_CON_REG16, reg_val);

			reg_val = DRV_Reg16(NFI_CNFG_REG16);
			reg_val &= ~CNFG_READ_EN;
			reg_val &= ~CNFG_OP_MODE_MASK;
			reg_val |= CNFG_OP_CUST;
			DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

			mtk_nand_set_command(devinfo.tlcControl.dis_slc_mode_cmd);
		}
#endif
		if (bRet == ERR_RTN_BCH_FAIL)
			break;

		remainSize = min(rSize, u4PageSize);
		memcpy(dataBuf, buf, remainSize);
		readCount++;
		dataBuf += remainSize;
		rSize -= remainSize;
		/* reset row_addr */
		pre_tlc_wl_info.wl_pre = tlc_wl_info.wl_pre;
		pre_tlc_wl_info.word_line_idx = tlc_wl_info.word_line_idx;
		logical_plane_num = 1;
		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
			if (devinfo.tlcControl.normaltlc) {
				if (devinfo.tlcControl.pPlaneEn) {
					tlc_left_plane = TRUE;
					logical_plane_num = 2;
					data_sector_num /= 2;
					real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, tlc_left_plane);
				}
			}
		}
		if (devinfo.tlcControl.pPlaneEn)
			real_row_addr = NFI_TLC_SetpPlaneAddr(real_row_addr, TRUE);

		if (devinfo.tlcControl.slcopmodeEn) {
			tlc_wl_info.word_line_idx++;
			real_row_addr++;
		} else {
			if (tlc_wl_info.wl_pre == WL_HIGH_PAGE) {
				tlc_wl_info.wl_pre = WL_LOW_PAGE;
				tlc_wl_info.word_line_idx++;
				real_row_addr++;
			} else
				tlc_wl_info.wl_pre++;
		}
	} while (rSize);

	PFM_END_R(pfm_time_read, u4PageSize + 32);

	return bRet;
}

int mtk_nand_read(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, int page, u32 size)
{
	int page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;
	u32 block;
	u32 page_in_block;
	u32 mapped_block;
	int bRet = ERR_RTN_SUCCESS;
#ifdef DUMP_PEF
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
#endif
	page_in_block = mtk_nand_page_transform(mtd, chip, page, &block, &mapped_block);

	bRet =
		mtk_nand_cache_read_page(mtd, page_in_block + mapped_block * page_per_block,
					mtd->writesize, buf, chip->oob_poi, size);
	if (bRet == ERR_RTN_SUCCESS) {
#ifdef DUMP_PEF
		do_gettimeofday(&etimer);
		g_NandPerfLog.ReadPageTotalTime += Cal_timediff(&etimer, &stimer);
		g_NandPerfLog.ReadPageCount++;
		dump_nand_rwcount();
#endif
#ifdef CFG_SNAND_ACCESS_PATTERN_LOGGER
		if (g_snand_pm_on == 1)
			mtk_snand_pm_add_drv_record(_SNAND_PM_OP_READ_PAGE,
							page_in_block + mapped_block * page_per_block,
							0, Cal_timediff(&etimer, &stimer));
#endif
		return 0;
	}

	return -EIO;
}
#endif

/******************************************************************************
 * mtk_nand_verify_buf
 *
 * DESCRIPTION:
 *	 Verify the NAND write data is correct or not !
 *
 * PARAMETERS:
 *	 struct mtd_info *mtd, const uint8_t *buf, int len
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE

char gacBuf[LPAGE + LSPARE];

static int mtk_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
#if 1
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct NAND_CMD *pkCMD = &g_kCMD;
	u32 u4PageSize = mtd->writesize;
	u32 *pSrc, *pDst;
	int i;
	u32 row_addr = pkCMD->u4RowAddr;
	u32 page_per_block = devinfo.blocksize * 1024 / devinfo.pagesize;

	if (((devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC)
		|| (devinfo.NAND_FLASH_TYPE == NAND_FLASH_MLC_HYBER))
		 && (devinfo.tlcControl.normaltlc) && (!mtk_block_istlc(pkCMD.u4RowAddr * mtd->writesize)))
		devinfo.tlcControl.slcopmodeEn = TRUE;
	else
		devinfo.tlcControl.slcopmodeEn = FALSE;

	if (devinfo.two_phyplane)
		row_addr = (((row_addr / page_per_block) << 1) * page_per_block) + (row_addr % page_per_block);

	mtk_nand_exec_read_page(mtd, row_addr, u4PageSize, gacBuf, gacBuf + u4PageSize);

	pSrc = (u32 *) buf;
	pDst = (u32 *) gacBuf;
	len = len / sizeof(u32);
	for (i = 0; i < len; ++i) {
		if (*pSrc != *pDst) {
			nand_pr_err("page fail at page %d", pkCMD->u4RowAddr);
			return -1;
		}
		pSrc++;
		pDst++;
	}

	pSrc = (u32 *) chip->oob_poi;
	pDst = (u32 *) (gacBuf + u4PageSize);

	if ((pSrc[0] != pDst[0]) || (pSrc[1] != pDst[1]) || (pSrc[2] != pDst[2])
		|| (pSrc[3] != pDst[3]) || (pSrc[4] != pDst[4]) || (pSrc[5] != pDst[5]))
		/* TODO: Ask Designer Why? */
		/* (pSrc[6] != pDst[6]) || (pSrc[7] != pDst[7])) */
	{
		nand_pr_err("oob fail at page %d", pkCMD->u4RowAddr);
		nand_pr_err("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", pSrc[0], pSrc[1], pSrc[2],
			pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]);
		nand_pr_err("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", pDst[0], pDst[1], pDst[2],
			pDst[3], pDst[4], pDst[5], pDst[6], pDst[7]);
		return -1;
	}
	/* pr_debug(KERN_INFO"mtk_nand_verify_buf OK at page %d\n", g_kCMD.u4RowAddr); */

	return 0;
#else
	return 0;
#endif
}
#endif

static bool ahb_read_sleep_enable(void)
{
	return (host->rd_para.sample_count > 0);
}

static bool ahb_read_can_sleep(u32 length)
{
	struct read_sleep_para *para = &host->rd_para;
	int sectors = length >> host->hw->nand_sec_shift;

	return (sectors * para->t_sector_ahb) > para->t_threshold;
}

static void get_ahb_read_sleep_range(u32 length,
	int *min, int *max)
{
	struct read_sleep_para *para = &host->rd_para;
	int sectors = length >> host->hw->nand_sec_shift;
	int t_base = sectors * para->t_sector_ahb - para->t_schedule;

	*min = t_base - para->t_sleep_range;
	*max = t_base + para->t_sleep_range;
}

static int get_sector_ahb_read_time(void)
{
	return host->rd_para.t_sector_ahb;
}

static void cal_sector_ahb_read_time(unsigned long long s,
	unsigned long long e, u32 length)
{
	struct read_sleep_para *para = &host->rd_para;
	static int sample_index;
	static int total_time;
	static int total_length;

	if (!para->sample_count || para->t_sector_ahb)
		return;

	if (sample_index < para->sample_count) {
		total_time += (e - s);
		total_length += length;
		sample_index++;
		nand_info("In cal total_time:%d, total_length:%d", total_time, total_length);
	} else {
		total_time /= 1000;/* ns to us*/
		para->t_sector_ahb =
			total_time/(total_length >> host->hw->nand_sec_shift);
		if (para->t_sector_ahb) {
			nand_info("t_sector_ahb:%d, nand_sec_shift:%d, time:%d, total_length:%d",
				para->t_sector_ahb, host->hw->nand_sec_shift, total_time, total_length);
		} else {
			nand_info("total_time:%d, total_length:%d", total_time, total_length);
		}
		total_time = 0;
		total_length = 0;
		sample_index = 0;
	}
}

static void mtk_nand_init_read_sleep_para(void)
{
	struct read_sleep_para *para = &host->rd_para;

	para->sample_count = 3;
	para->t_sector_ahb = 0;
	para->t_schedule = 40;
	para->t_sleep_range = 10;
	para->t_threshold = 100;
}

static void mtk_nand_init_debug_para(struct mtk_nand_debug *debug)
{
	debug->err.en_print = false;
}

/******************************************************************************
 * mtk_nand_init_hw
 *
 * DESCRIPTION:
 *	 Initial NAND device hardware component !
 *
 * PARAMETERS:
 *	 struct mtk_nand_host *host (Initial setting data)
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void mtk_nand_init_hw(struct mtk_nand_host *host)
{
	struct mtk_nand_host_hw *hw = host->hw;

	g_bInitDone = false;
	g_kCMD.u4OOBRowAddr = (u32) -1;

	/* Set default NFI access timing control */
	DRV_WriteReg32(NFI_ACCCON_REG32, hw->nfi_access_timing);
	DRV_WriteReg16(NFI_CNFG_REG16, 0);
	if (mtk_nfi_dev_comp->chip_ver == 1) {
		DRV_WriteReg16(NFI_PAGEFMT_REG16, 4);
	} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
		DRV_WriteReg32(NFI_PAGEFMT_REG32, 4);
	} else {
		nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
			mtk_nfi_dev_comp->chip_ver);
	}
	DRV_WriteReg32(NFI_ENMPTY_THRESH_REG32, 40);

	/* Reset the state machine and data FIFO, because flushing FIFO */
	(void)mtk_nand_reset();

	/* Set the ECC engine */
	if (hw->nand_ecc_mode == NAND_ECC_HW) {
		pr_notice("Use HW ECC\n");
		if (g_bHwEcc)
			NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

		ECC_Config(host->hw, 4);
		mtk_nand_configure_fdm(8);
	}

	/* Initialize interrupt. Clear interrupt, read clear. */
	DRV_Reg16(NFI_INTR_REG16);

	/* Interrupt arise when read data or program data to/from AHB is done. */
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);

	/* Enable automatic disable ECC clock when NFI is busy state */
	if (mtk_nfi_dev_comp->chip_ver != 3)
		DRV_WriteReg16(NFI_DEBUG_CON1_REG16, (NFI_BYPASS | WBUF_EN | HWDCM_SWCON_ON));

	/* Enable ECC PLL for MT8167*/
	if (mtk_nfi_dev_comp->chip_ver == 3) {
		NFI_SET_REG16(NFI_DEBUG_CON1_REG16, (1<<NFI_DEBUG_ECCPLL_SHIFT));
		pr_info("NFI_DEBUG_CON1_REG16:%x\n", DRV_Reg16(NFI_DEBUG_CON1_REG16));

	}
#ifdef CONFIG_PM
	host->saved_para.suspend_flag = 0;
#endif
	/* Reset */
}

/* ------------------------------------------------------------------------------- */
static int mtk_nand_dev_ready(struct mtd_info *mtd)
{
	return !(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
}

/******************************************************************************
 * mtk_nand_probe
 *
 * DESCRIPTION:
 *	 register the nand device file operations !
 *
 * PARAMETERS:
 *	 struct platform_device *pdev : device structure
 *
 * RETURNS:
 *	 0 : Success
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
#define KERNEL_NAND_UNIT_TEST 0
#define NAND_READ_PERFORMANCE 0
#if KERNEL_NAND_UNIT_TEST
__aligned(64)
static u8 temp_buffer_xl[LPAGE + LSPARE];
__aligned(64)
static u8 temp_buffer_xl_rd[LPAGE + LSPARE];

int mtk_nand_unit_test(struct nand_chip *nand_chip, struct mtd_info *mtd)
{
	pr_debug("Begin to Kernel nand unit test ...\n");
	int err = 0;
	int patternbuff[128] = {
		0x0103D901, 0xFF1802DF, 0x01200400, 0x00000021, 0x02040122, 0x02010122, 0x03020407,
		0x1A050103,
		0x00020F1B, 0x08C0C0A1, 0x01550800, 0x201B0AC1, 0x41990155, 0x64F0FFFF, 0x201B0C82,
		0x4118EA61,
		0xF00107F6, 0x0301EE1B, 0x0C834118, 0xEA617001, 0x07760301, 0xEE151405, 0x00202020,
		0x20202020,
		0x00202020, 0x2000302E, 0x3000FF14, 0x00FF0000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x01D90301, 0xDF0218FF, 0x00042001, 0x21000000, 0x22010402, 0x22010102, 0x07040203,
		0x0301051A,
		0x1B0F0200, 0xA1C0C008, 0x00085501, 0xC10A1B20, 0x55019941, 0xFFFFF064, 0x820C1B20,
		0x61EA1841,
		0xF60701F0, 0x1BEE0103, 0x1841830C, 0x017061EA, 0x01037607, 0x051415EE, 0x20202000,
		0x20202020,
		0x20202000, 0x2E300020, 0x14FF0030, 0x0000FF00, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000
	};
	u32 j, k, p = g_block_size / g_page_size, m;

	pr_debug("[P] %x\n", p);
	pr_debug("[xiaolei] bias = 0x%x", *(volatile u32 *)(GPIO_BASE + 0xE20));
	pr_debug("[xiaolei] ACC = 0x%x", *(volatile u32 *)(NFI_BASE + 0x00C));

	struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);
	u32 val = 0x05, TOTAL = 1000;

	for (m = 0; m < 32; m++)
		memcpy(temp_buffer_xl + 512 * m, (u8 *) patternbuff, 512);

	pr_debug("***************read pl***********************\n");
	memset(temp_buffer_xl_rd, 0xA5, 16384);
	if (mtk_nand_read_page(mtd, nand_chip, temp_buffer_xl_rd, 1 * p))
		pr_debug("Read page 0x%x fail!\n", 1 * p);
	for (m = 0; m < 32; m++)
		pr_debug("[5]0x%x %x %x %x\n", *((int *)temp_buffer_xl_rd + m * 4),
			*((int *)temp_buffer_xl_rd + 1 + m * 4),
			*((int *)temp_buffer_xl_rd + 2 + m * 4),
			*((int *)temp_buffer_xl_rd + 3 + m * 4));


	for (j = 0x400; j < 0x750; j++) {
		/* memset(local_buffer, 0x00, 16384); */
		/* mtk_nand_read_page(mtd, nand_chip, local_buffer, j*p); */
		/* for(m = 0; m < 32; m++) */
		mtk_nand_erase(mtd, j * p);
		memset(temp_buffer_xl_rd, 0x00, 16384);
		if (mtk_nand_read_page(mtd, nand_chip, temp_buffer_xl_rd, j * p))
			pr_debug("Read page 0x%x fail!\n", j * p);
		pr_debug("[2]0x%x %x %x %x\n", *(int *)temp_buffer_xl_rd,
			*((int *)temp_buffer_xl_rd + 1), *((int *)temp_buffer_xl_rd + 2),
			*((int *)temp_buffer_xl_rd + 3));
		if (mtk_nand_block_bad(mtd, j * g_block_size, 0)) {
			pr_debug("Bad block at %x\n", j);
			continue;
		}
		for (k = 0; k < p; k++) {
			pr_debug("***************w b***********************\n");

			for (m = 0; m < 32; m++)
				pr_debug("[1]0x%x %x %x %x\n", *((int *)temp_buffer_xl + m * 4),
					*((int *)temp_buffer_xl + 1 + m * 4),
					*((int *)temp_buffer_xl + 2 + m * 4),
					*((int *)temp_buffer_xl + 3 + m * 4));

			if (mtk_nand_write_page(mtd, nand_chip, 0, 0, temp_buffer_xl, 0,
				 j * p + k, 0, 0))
				pr_debug("Write page 0x%x fail!\n", j * p + k);
			/* #if 1 */
			/* } */
			/* TOTAL=1000; */
			/* do{ */
			/* for (k = 0; k < p; k++) */
			/* { */
			/* #endif */
			pr_debug("***************r b***********************\n");
			memset(temp_buffer_xl_rd, 0x00, g_page_size);
			if (mtk_nand_read_page(mtd, nand_chip, temp_buffer_xl_rd, j * p + k))
				pr_debug("Read page 0x%x fail!\n", j * p + k);
			for (m = 0; m < 32; m++)
				pr_debug("[3]0x%x %x %x %x\n", *((int *)temp_buffer_xl_rd + m * 4),
					*((int *)temp_buffer_xl_rd + 1 + m * 4),
					*((int *)temp_buffer_xl_rd + 2 + m * 4),
					*((int *)temp_buffer_xl_rd + 3 + m * 4));
			if (memcmp(temp_buffer_xl, temp_buffer_xl_rd, 512)) {
				pr_debug("[KERNEL_NAND_UNIT_TEST] compare fail!\n");
				err = -1;
				while (1)
					;
			} else {
				TOTAL--;
				pr_debug("[KERNEL_NAND_UNIT_TEST] compare OK!\n");
			}
		}
		/* }while(TOTAL); */
#if 0
		mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd,
					feature_set->Async_timing.address, (u8 *) &val,
					sizeof(feature_set->Async_timing.feature));
		mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd,
					feature_set->Async_timing.address, (u8 *) &val, 4);
		pr_debug("[ASYNC Interface]0x%X\n", val);
		err = mtk_nand_interface_config(mtd);
		MSG(INIT, "[nand_interface_config] %d\n", err);
#endif
	}
	return err;
}
#endif

#ifdef TLC_UNIT_TEST
#include <linux/vmalloc.h>

__aligned(64)
static u8 temp_buffer_tlc[LPAGE + LSPARE];
__aligned(64)
static u8 temp_buffer_tlc_rd[LPAGE + LSPARE];
int mtk_tlc_unit_test(struct nand_chip *nand_chip, struct mtd_info *mtd)
{
	int err = 0;
	int patternbuff[128] = {
	0x0103D901, 0xFF1802DF, 0x01200400, 0x00000021, 0x02040122, 0x02010122, 0x03020407, 0x1A050103,
	0x00020F1B, 0x08C0C0A1, 0x01550800, 0x201B0AC1, 0x41990155, 0x64F0FFFF, 0x201B0C82, 0x4118EA61,
	0xF00107F6, 0x0301EE1B, 0x0C834118, 0xEA617001, 0x07760301, 0xEE151405, 0x00202020, 0x20202020,
	0x00202020, 0x2000302E, 0x3000FF14, 0x00FF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x01D90301, 0xDF0218FF, 0x00042001, 0x21000000, 0x22010402, 0x22010102, 0x07040203, 0x0301051A,
	0x1B0F0200, 0xA1C0C008, 0x00085501, 0xC10A1B20, 0x55019941, 0xFFFFF064, 0x820C1B20, 0x61EA1841,
	0xF60701F0, 0x1BEE0103, 0x1841830C, 0x017061EA, 0x01037607, 0x051415EE, 0x20202000, 0x20202020,
	0x20202000, 0x2E300020, 0x14FF0030, 0x0000FF00, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	u32 k, p = g_block_size / g_page_size, m;
	u32 test_page;
	u8 *buf = vmalloc(g_block_size);

	pr_warn("[P] %d\n", p);

	pr_warn("Begin to Kernel tlc unit test ...\n");
	for (m = 0; m < 32; m++)
		memcpy(temp_buffer_tlc+(512*m), (u8 *)patternbuff, 512);

#ifdef NAND_PFM
	g_PFM_R = 0;
	g_PFM_W = 0;
	g_PFM_E = 0;
	g_PFM_RNum = 0;
	g_PFM_RD = 0;
	g_PFM_WD = 0;
#endif
#if 1
	pr_warn("***************SLC MODE TEST***********************\n");
	test_page = 382 * 256;
	mtk_nand_erase(mtd, test_page);
	for (k = 0; k < (p/2); k++) {
		pr_warn("***************w p %d***********************\n", test_page + k);
		if (mtk_nand_write_page(mtd, nand_chip, 0, 0, temp_buffer_tlc, 0, test_page + k, 0, 0))
			pr_warn("Write page 0x%x fail!\n", test_page + k);

		pr_warn("***************r p %d***********************\n", test_page + k);
		memset(temp_buffer_tlc_rd, 0x00, g_page_size);
		if (mtk_nand_read_page(mtd, nand_chip, temp_buffer_tlc_rd, test_page + k))
			pr_warn("Read page 0x%x fail!\n", test_page + k);

		if (memcmp(temp_buffer_tlc, temp_buffer_tlc_rd, g_page_size)) {
			pr_warn("compare fail!\n");
			err = 1;
			break;
		}
	}
#endif

#ifdef NAND_PFM
	g_PFM_R = 0;
	g_PFM_W = 0;
	g_PFM_E = 0;
	g_PFM_RNum = 0;
	g_PFM_RD = 0;
	g_PFM_WD = 0;
#endif

#if 1
	pr_warn("***************TLC MODE TEST***********************\n");
	test_page = 82 * 256;
	mtk_nand_erase(mtd, test_page);

	memset(buf, 0x00, g_block_size);
	for (k = 0; k < p; k++)
		memcpy(buf + (g_page_size * k), temp_buffer_tlc, g_page_size);
	pr_warn("***************w b %d***********************\n", test_page);
	mtk_nand_write_tlc_block(mtd, nand_chip, buf, test_page);

	for (k = 0; k < p; k++) {
		pr_warn("***************r p %d***********************\n", test_page + k);
		memset(temp_buffer_tlc_rd, 0x00, g_page_size);
		if (mtk_nand_read_page(mtd, nand_chip, temp_buffer_tlc_rd, test_page + k))
			pr_warn("Read page 0x%x fail!\n", test_page + k);

		if (memcmp(temp_buffer_tlc, temp_buffer_tlc_rd, g_page_size)) {
			pr_warn("compare fail!\n");
			err = 2;
			break;
		}
	}
#endif

	vfree(buf);
	return err;
}
#endif

#ifdef NAND_FEATURE_TEST
static int mtk_nand_test(struct mtd_info *mtd, struct nand_chip *nand_chip)
{
	u32 page, page1, page2;
	u32 block, block1;
	bool write_test = TRUE;

	while (0) {
		mtk_nand_read_page(mtd, nand_chip, temp_buffer, 0x34408);
		pr_info("Page: 0x34408 (0x8) bit flip: %d retry count %d\n",
				correct_count, g_hynix_retry_count);
		mtk_nand_rrtry_func(mtd, devinfo, 0, FALSE);

		mdelay(3);
	}
	do {
		mtk_nand_read_page(mtd, nand_chip, test_buffer, 0x25500);
		mtk_nand_read_page(mtd, nand_chip, temp_buffer, 0x33C00);
		if (!memcmp(temp_buffer, test_buffer, 1000))
			write_test = FALSE;
		if (write_test) {
			pr_info("First step: erase and program #0 page\n");
			for (page = 0x33C00; page < 0x43C00; page += 256) {
				mtk_nand_erase_hw(mtd, page);
				mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page, 0, 0);
			}
			pr_info("Second step: Create open block\n");
			page = 0x33C00;
			for (block = 1; block < 256; block++) {
				block1 = page/256;
				for (page1 = page + 1; page1 < page + block; page1++) {
					page2 = sandisk_pairpage_mapping((page1%256), TRUE);
					if (page2 != (page1%256))
						mtk_nand_read_page(mtd, nand_chip, temp_buffer, block1*256+page2);
					mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page1, 0, 0);
				}
				page += 256;
			}
			pr_info("Third step: Check bit flip count\n");
		}
		page = 0x33C00;
		for (block = 1; block < 256; block++) {
			block1 = page/256;
			pr_info("====block 0x%x====\n", block1);

			for (page1 = page; page1 < page + block; page1++) {
				mtk_nand_read_page(mtd, nand_chip, temp_buffer, page1);
				pr_info("Page: 0x%x (0x%x) bit flip: %d\n", page1, page1%256, correct_count);
			}

			page += 256;
		}
		mtk_nand_read_page(mtd, nand_chip, temp_buffer, 0x43C00);
		write_test = TRUE;
		if (!memcmp(temp_buffer, test_buffer, 1000))
			write_test = FALSE;
		if (write_test) {
			nand__err("4th step: erase and program #0 page\n");
			for (page = 0x43C00; page < 0x53C00; page += 256) {
				mtk_nand_erase_hw(mtd, page);
				mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page, 0, 0);
				mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page+1, 0, 0);
				mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page+2, 0, 0);
			}
			pr_info("5th step: Create open block\n");
			page = 0x43C00;
			for (block = 3; block < 256; block++) {
				block1 = page/256;
				for (page1 = page + 3; page1 < page+block; page1++) {
					page2 = sandisk_pairpage_mapping((page1%256), TRUE);
					if (page2 != (page1%256))
						mtk_nand_read_page(mtd, nand_chip, temp_buffer, block1*256+page2);
					mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page1, 0, 0);
				}
				page += 256;
			}
			pr_info("6th step: Check bit flip count\n");
		}
		page = 0x43C00;
		for (block = 1; block < 256; block++) {
			block1 = page/256;
			pr_info("====WL block 0x%x====\n", block1);

			for (page1 = page; page1 < page+block; page1++) {
				mtk_nand_read_page(mtd, nand_chip, temp_buffer, page1);
				pr_info("WL Page: 0x%x (0x%x) bit flip: %d\n", page1, page1%256, correct_count);
			}

			page += 256;
		}
		mtk_nand_read_page(mtd, nand_chip, temp_buffer, 0x53C00);
		write_test = TRUE;
		if (!memcmp(temp_buffer, test_buffer, 1000))
			write_test = FALSE;
		if (write_test) {
			pr_info("7th step: erase and program\n");
			page = 0x53C00;
			for (block = 0; block <= 255; block++) {
				mtk_nand_erase_hw(mtd, page);
				block1 = page/256;
				for (page1 = page; page1 <= page+block; page1++) {
					page2 = sandisk_pairpage_mapping((page1%256), TRUE);
					if (page2 != (page1%256))
						mtk_nand_read_page(mtd, nand_chip, temp_buffer, block1*256+page2);
					mtk_nand_write_page(mtd, nand_chip, 0, 16384, test_buffer, 0, page1, 0, 0);
				}
				page += 256;
			}
			pr_info("6th step: Check bit flip count\n");
		}
		page = 0x53C00;
		for (block = 1; block < 256; block++) {
			block1 = page/256;
			pr_info("====Seq block 0x%x====\n", block1);

			for (page1 = page; page1 < page+block; page1++) {
				mtk_nand_read_page(mtd, nand_chip, temp_buffer, page1);
				pr_info("Seq Page: 0x%x (0x%x) bit flip: %d\n", page1, page1%256, correct_count);
			}

			page += 256;
		}
	} while (0)
		;
while (1)
	;
}
#endif

#if CFG_2CS_NAND
/* #define CHIP_ADDRESS (0x100000) */
static int mtk_nand_cs_check(struct mtd_info *mtd, u8 *id, u16 cs)
{
	u8 ids[NAND_MAX_ID];
	int i = 0;
	/* if(devinfo.ttarget == TTYPE_2DIE) */
	/* { */
	/* MSG(INIT,"2 Die Flash\n"); */
	/* g_bTricky_CS = TRUE; */
	/* return 0; */
	/* } */
	DRV_WriteReg16(NFI_CSEL_REG16, cs);
	mtk_nand_command_bp(mtd, NAND_CMD_READID, 0, -1);
	for (i = 0; i < NAND_MAX_ID; i++) {
		ids[i] = mtk_nand_read_byte(mtd);
		if (ids[i] != id[i]) {
			pr_notice("Nand cs[%d] not support(%d,%x)\n", cs, i, ids[i]);
			DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);

			return 0;
		}
	}
	DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
	return 1;
}

static u32 mtk_nand_cs_on(struct nand_chip *nand_chip, u16 cs, u32 page)
{
	u32 cs_page = page / g_nanddie_pages;

	if (cs_page) {
		DRV_WriteReg16(NFI_CSEL_REG16, cs);
		/* if(devinfo.ttarget == TTYPE_2DIE) */
		/* return page;//return (page | CHIP_ADDRESS); */
		return (page - g_nanddie_pages);
	}
	DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
	return page;
}

#else

#define mtk_nand_cs_check(mtd, id, cs)	(1)
#define mtk_nand_cs_on(nand_chip, cs, page)   (page)
#endif

static int mtk_nand_probe(struct platform_device *pdev)
{

	struct mtk_nand_host_hw *hw;
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
	/*struct resource *res = pdev->resource; */
	int err = 0;
	u64 temp;
#if !defined(CONFIG_MTK_LEGACY)
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	 int ret = 0;
#endif
	u32 efuse_index = 0;
#endif
	u32 EFUSE_RANDOM_ENABLE = 0x00000004;

	u8 id[NAND_MAX_ID];
	int i;
	u32 sector_size = NAND_SECTOR_SIZE;
#if CFG_COMBO_NAND
	int bmt_sz = 0;
#endif

#ifdef CONFIG_OF
	const struct of_device_id *of_id;

	of_id = of_match_node(mtk_nfi_of_match, pdev->dev.of_node);
	if (!of_id)
		return -EINVAL;

	mtk_nfi_dev_comp = of_id->data;
	/* dt modify */
	mtk_nfi_base = of_iomap(pdev->dev.of_node, 0);
	pr_debug("of_iomap for nfi base @ 0x%p\n", mtk_nfi_base);

	if (mtk_nfiecc_node == NULL) {
		if (mtk_nfi_dev_comp->chip_ver == 1)
			mtk_nfiecc_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-nfiecc");
		else if (mtk_nfi_dev_comp->chip_ver == 2)
			mtk_nfiecc_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8163-nfiecc");
		else if (mtk_nfi_dev_comp->chip_ver == 3)
			mtk_nfiecc_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8167-nfiecc");
		mtk_nfiecc_base = of_iomap(mtk_nfiecc_node, 0);
		pr_debug("of_iomap for nfiecc base @ 0x%p\n", mtk_nfiecc_base);
	}
	nfi_irq = irq_of_parse_and_map(pdev->dev.of_node, 0);

	if (mtk_gpio_node == NULL) {
		/* mtk_gpio_node = of_find_compatible_node(NULL, NULL, "mediatek,GPIO"); */
		if (mtk_nfi_dev_comp->chip_ver == 1)
			mtk_gpio_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8127-pctl-a-syscfg");
		else if (mtk_nfi_dev_comp->chip_ver == 2)
			mtk_gpio_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8163-pctl-a-syscfg");
		else if (mtk_nfi_dev_comp->chip_ver == 3)
			mtk_gpio_node = of_find_compatible_node(NULL, NULL, "mediatek,mt8167-pctl-a-syscfg");
		mtk_gpio_base = of_iomap(mtk_gpio_node, 0);
		pr_debug("of_iomap for gpio base @ 0x%p\n", mtk_gpio_base);
	}

#ifdef CONFIG_MTK_LEGACY
	if (mtk_efuse_node == NULL) {
		mtk_efuse_node = of_find_compatible_node(NULL, NULL, "mediatek,EFUSEC");
		mtk_efuse_base = of_iomap(mtk_efuse_node, 0);
		pr_debug("of_iomap for efuse base @ 0x%p\n", mtk_efuse_base);
	}

	if (mtk_infra_node == NULL) {
		mtk_infra_node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
		mtk_infra_base = of_iomap(mtk_infra_node, 0);
		pr_debug("of_iomap for infra base @ 0x%p\n", mtk_infra_base);
	}
#endif

	/* dt modify */
#endif

#if !defined(CONFIG_MTK_LEGACY)
	if (mtk_nfi_dev_comp->chip_ver == 1) {
		nfi_hclk = devm_clk_get(&pdev->dev, "nfi_ck");
		WARN_ON(IS_ERR(nfi_hclk));
		nfiecc_bclk = devm_clk_get(&pdev->dev, "nfi_ecc_ck");
		WARN_ON(IS_ERR(nfiecc_bclk));
		nfi_bclk = devm_clk_get(&pdev->dev, "nfi_pad_ck");
		WARN_ON(IS_ERR(nfi_bclk));
		mtk_nand_regulator = devm_regulator_get(&pdev->dev, "vmch");
		WARN_ON(IS_ERR(mtk_nand_regulator));
	} else if (mtk_nfi_dev_comp->chip_ver == 2) {
		nfi_hclk = devm_clk_get(&pdev->dev, "nfi_hclk");
		WARN_ON(IS_ERR(nfi_hclk));
		nfiecc_bclk = devm_clk_get(&pdev->dev, "nfiecc_bclk");
		WARN_ON(IS_ERR(nfiecc_bclk));
		nfi_bclk = devm_clk_get(&pdev->dev, "nfi_bclk");
		WARN_ON(IS_ERR(nfi_bclk));
		onfi_sel_clk = devm_clk_get(&pdev->dev, "onfi_sel");
		WARN_ON(IS_ERR(onfi_sel_clk));
		onfi_26m_clk = devm_clk_get(&pdev->dev, "onfi_clk26m");
		WARN_ON(IS_ERR(onfi_26m_clk));
		onfi_mode5 = devm_clk_get(&pdev->dev, "onfi_mode5");
		WARN_ON(IS_ERR(onfi_mode5));
		onfi_mode4 = devm_clk_get(&pdev->dev, "onfi_mode4");
		WARN_ON(IS_ERR(onfi_mode4));
		nfi_bclk_sel = devm_clk_get(&pdev->dev, "nfi_bclk_sel");
		WARN_ON(IS_ERR(nfi_bclk_sel));
		nfi_ahb_clk = devm_clk_get(&pdev->dev, "nfi_ahb_clk");
		WARN_ON(IS_ERR(nfi_ahb_clk));
		nfi_1xpad_clk = devm_clk_get(&pdev->dev, "nfi_1xpad_clk");
		WARN_ON(IS_ERR(nfi_1xpad_clk));
		nfi_ecc_pclk = devm_clk_get(&pdev->dev, "nfiecc_pclk");
		WARN_ON(IS_ERR(nfi_ecc_pclk));
		nfi_pclk = devm_clk_get(&pdev->dev, "nfi_pclk");
		WARN_ON(IS_ERR(nfi_pclk));
		onfi_pad_clk = devm_clk_get(&pdev->dev, "onfi_pad_clk");
		WARN_ON(IS_ERR(onfi_pad_clk));
		mtk_nand_regulator = devm_regulator_get(&pdev->dev, "vmch");
		WARN_ON(IS_ERR(mtk_nand_regulator));
	} else if (mtk_nfi_dev_comp->chip_ver == 3) {
#if !defined(CONFIG_FPGA_EARLY_PORTING)
		nfi_hclk = devm_clk_get(&pdev->dev, "nfi_hclk");
		WARN_ON(IS_ERR(nfi_hclk));
		nfiecc_bclk = devm_clk_get(&pdev->dev, "nfiecc_bclk");
		WARN_ON(IS_ERR(nfiecc_bclk));
		nfi_bclk = devm_clk_get(&pdev->dev, "nfi_bclk");
		WARN_ON(IS_ERR(nfi_bclk));
		nfi_2xclk = devm_clk_get(&pdev->dev, "nfi_2xclk");
		WARN_ON(IS_ERR(nfi_2xclk));
		nfi_1xpad_clk  = devm_clk_get(&pdev->dev, "nfi_1xclk");
		WARN_ON(IS_ERR(nfi_1xpad_clk));
		nfi_rgecc  = devm_clk_get(&pdev->dev, "nfi_rgecc");
		WARN_ON(IS_ERR(nfi_rgecc));
		nfi_1xclk_sel = devm_clk_get(&pdev->dev, "nfi_1xpad_sel");
		WARN_ON(IS_ERR(nfi_1xclk_sel));
		nfi_2xclk_sel = devm_clk_get(&pdev->dev, "nfi_2xpad_sel");
		WARN_ON(IS_ERR(nfi_2xclk_sel));
		nfiecc_sel = devm_clk_get(&pdev->dev, "nfiecc_sel");
		WARN_ON(IS_ERR(nfiecc_sel));
		nfiecc_csw_sel = devm_clk_get(&pdev->dev, "nfiecc_csw_sel");
		WARN_ON(IS_ERR(nfiecc_csw_sel));
		nfi_ahb_clk = devm_clk_get(&pdev->dev, "infra_ahb");
		WARN_ON(IS_ERR(nfi_ahb_clk));
		onfi_26m_clk = devm_clk_get(&pdev->dev, "clk_26m");
		WARN_ON(IS_ERR(onfi_26m_clk));
		main_d4 = devm_clk_get(&pdev->dev, "main_d4");
		WARN_ON(IS_ERR(main_d4));
		main_d5 = devm_clk_get(&pdev->dev, "main_d5");
		WARN_ON(IS_ERR(main_d5));
		main_d6 = devm_clk_get(&pdev->dev, "main_d6");
		WARN_ON(IS_ERR(main_d6));
		main_d7 = devm_clk_get(&pdev->dev, "main_d7");
		WARN_ON(IS_ERR(main_d7));
		main_d8 = devm_clk_get(&pdev->dev, "main_d7");
		WARN_ON(IS_ERR(main_d8));
		main_d10 = devm_clk_get(&pdev->dev, "main_d10");
		WARN_ON(IS_ERR(main_d10));
		main_d12 = devm_clk_get(&pdev->dev, "main_d12");
		WARN_ON(IS_ERR(main_d12));
		mtk_nand_regulator = devm_regulator_get(&pdev->dev, "vmch");
		WARN_ON(IS_ERR(mtk_nand_regulator));
#endif
	}
#endif

#if defined(CONFIG_MTK_LEGACY)
#ifdef CONFIG_MTK_PMIC_MT6397
	hwPowerOn(MT65XX_POWER_LDO_VMCH, VOL_3300, "NFI");
#else
	hwPowerOn(MT6323_POWER_LDO_VMCH, VOL_3300, "NFI");
#endif
#else
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	ret = regulator_set_voltage(mtk_nand_regulator, 3300000, 3300000);
	if (ret != 0)
		nand_pr_err("regulator set vol failed: %d", ret);

	ret = regulator_enable(mtk_nand_regulator);
	if (ret != 0)
		nand_pr_err("regulator_enable failed: %d", ret);
#endif
#endif

#ifdef CONFIG_OF
	hw = (struct mtk_nand_host_hw *)pdev->dev.platform_data;
	hw = devm_kzalloc(&pdev->dev, sizeof(*hw), GFP_KERNEL);
	WARN_ON(!hw);
	hw->nfi_bus_width = 8;
	hw->nfi_access_timing = 0x10804333;
	hw->nfi_cs_num = 2;
	hw->nand_sec_size = 512;
	hw->nand_sec_shift = 9;
	hw->nand_ecc_size = 2048;
	hw->nand_ecc_bytes = 32;
	hw->nand_ecc_mode = 2;
#else
	hw = (struct mtk_nand_host_hw *)pdev->dev.platform_data;
	WARN_ON(!hw);

	if (pdev->num_resources != 4 || res[0].flags != IORESOURCE_MEM
		|| res[1].flags != IORESOURCE_MEM || res[2].flags != IORESOURCE_IRQ
		|| res[3].flags != IORESOURCE_IRQ) {
		nand_pr_err("invalid resource type");
		return -ENODEV;
	}

	/* Request IO memory */
	if (!request_mem_region(res[0].start, res[0].end - res[0].start + 1, pdev->name))
		return -EBUSY;

	if (!request_mem_region(res[1].start, res[1].end - res[1].start + 1, pdev->name))
		return -EBUSY;
#endif

	/* Allocate memory for the device structure (and zero it) */
	host = kzalloc(sizeof(struct mtk_nand_host), GFP_KERNEL);
	if (!host) {
		return -ENOMEM;
	}
	mtk_nand_init_read_sleep_para();
	mtk_nand_init_debug_para(&host->debug);
#if __INTERNAL_USE_AHB_MODE__
	g_bHwEcc = true;
#else
	g_bHwEcc = false;
#endif

	/* Allocate memory for 16 byte aligned buffer */
	local_buffer_16_align = local_buffer;
	temp_buffer_16_align = temp_buffer;
	/* pr_debug(KERN_INFO "Allocate 16 byte aligned buffer: %p\n", local_buffer_16_align); */

	host->hw = hw;
	PL_TIME_PROG(10);
	PL_TIME_ERASE(10);
	PL_TIME_PROG_WDT_SET(1);
	PL_TIME_ERASE_WDT_SET(1);

	/* init mtd data structure */
	nand_chip = &host->nand_chip;
	nand_chip->priv = host;	/* link the private data structures */

	mtd = &host->mtd;
	mtd->priv = nand_chip;
	mtd->owner = THIS_MODULE;
	mtd->name = "MTK-Nand";
	mtd->eraseregions = host->erase_region;

	hw->nand_ecc_mode = NAND_ECC_HW;

	/* Set address of NAND IO lines */
	nand_chip->IO_ADDR_R = (void __iomem *)NFI_DATAR_REG32;
	nand_chip->IO_ADDR_W = (void __iomem *)NFI_DATAW_REG32;
	nand_chip->chip_delay = 20;	/* 20us command delay time */
	nand_chip->ecc.mode = hw->nand_ecc_mode;	/* enable ECC */

	nand_chip->read_byte = mtk_nand_read_byte;
	nand_chip->read_buf = mtk_nand_read_buf;
	nand_chip->write_buf = mtk_nand_write_buf;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	nand_chip->verify_buf = mtk_nand_verify_buf;
#endif
	nand_chip->select_chip = mtk_nand_select_chip;
	nand_chip->dev_ready = mtk_nand_dev_ready;
	nand_chip->cmdfunc = mtk_nand_command_bp;
	nand_chip->ecc.read_page = mtk_nand_read_page_hwecc;
	nand_chip->ecc.write_page = mtk_nand_write_page_hwecc;

	nand_chip->ecc.layout = &nand_oob_64;
	nand_chip->ecc.size = hw->nand_ecc_size;	/* 2048 */
	nand_chip->ecc.bytes = hw->nand_ecc_bytes;	/* 32 */

	nand_chip->options = NAND_SKIP_BBTSCAN;

	/* For BMT, we need to revise driver architecture */
	nand_chip->write_page = mtk_nand_write_page;
	nand_chip->read_page = mtk_nand_read_page;
	nand_chip->read_subpage = mtk_nand_read_subpage;
	nand_chip->ecc.write_oob = mtk_nand_write_oob;
	nand_chip->ecc.read_oob = mtk_nand_read_oob;
	/* need to add nand_get_device()/nand_release_device(). */
	nand_chip->block_markbad = mtk_nand_block_markbad;
	nand_chip->erase_hw = mtk_nand_erase;
	nand_chip->block_bad = mtk_nand_block_bad;
#if CFG_FPGA_PLATFORM
	pr_debug("[FPGA Dummy]Enable NFI and NFIECC Clock\n");
#else
	/* MSG(INIT, "[NAND]Enable NFI and NFIECC Clock\n"); */

	clk_prepare_enable(nfi_2xclk_sel);
	clk_set_parent(nfi_2xclk_sel, onfi_26m_clk);
	clk_disable_unprepare(nfi_2xclk_sel);

	clk_prepare_enable(nfiecc_sel);
	clk_set_parent(nfiecc_sel, main_d4);
	clk_disable_unprepare(nfiecc_sel);

	clk_prepare_enable(nfi_1xclk_sel);
	clk_set_parent(nfi_1xclk_sel, nfi_ahb_clk);
	clk_disable_unprepare(nfi_1xclk_sel);

	nand_prepare_clock();
	nand_enable_clock();

#endif
#ifndef CONFIG_MTK_FPGA
	/* mtk_nand_gpio_init(); */
#endif

/*#ifdef CONFIG_FPGA_EARLY_PORTING*/
#if 0
	if (mtk_nfi_dev_comp->chip_ver == 3)
		mtk_nand_gpio_init();
#endif
	mtk_nand_init_hw(host);
	/* Select the device */
	nand_chip->select_chip(mtd, NFI_DEFAULT_CS);

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	for (i = 0; i < NAND_MAX_ID; i++)
		id[i] = nand_chip->read_byte(mtd);

	manu_id = id[0];
	dev_id = id[1];

	if (!get_device_info(id, &devinfo))
		nand_pr_err("Not Support this Device!");

#if CFG_2CS_NAND
	if (mtk_nand_cs_check(mtd, id, NFI_TRICKY_CS)) {
		pr_info("Twins Nand\n");
		g_bTricky_CS = TRUE;
		g_b2Die_CS = TRUE;
	}
#endif

	if (devinfo.pagesize == 16384) {
		nand_chip->ecc.layout = &nand_oob_128;
		hw->nand_ecc_size = 16384;
	} else if (devinfo.pagesize == 8192) {
		nand_chip->ecc.layout = &nand_oob_128;
		hw->nand_ecc_size = 8192;
	} else if (devinfo.pagesize == 4096) {
		nand_chip->ecc.layout = &nand_oob_128;
		hw->nand_ecc_size = 4096;
	} else if (devinfo.pagesize == 2048) {
		nand_chip->ecc.layout = &nand_oob_64;
		hw->nand_ecc_size = 2048;
	} else if (devinfo.pagesize == 512) {
		nand_chip->ecc.layout = &nand_oob_16;
		hw->nand_ecc_size = 512;
	} else if (devinfo.pagesize == 32768) {
		nand_chip->ecc.layout = &nand_oob_128;
		hw->nand_ecc_size = 32768;
	}
	if (devinfo.sectorsize == 1024) {
		sector_size = 1024;
		hw->nand_sec_shift = 10;
		hw->nand_sec_size = 1024;
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_SECTOR_SEL);
		} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
			NFI_CLN_REG32(NFI_PAGEFMT_REG32, PAGEFMT_SECTOR_SEL);
		} else {
			nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
				mtk_nfi_dev_comp->chip_ver);
		}
	}
	if (devinfo.pagesize <= 4096) {
		nand_chip->ecc.layout->eccbytes =
			devinfo.sparesize - OOB_AVAI_PER_SECTOR * (devinfo.pagesize / sector_size);
		hw->nand_ecc_bytes = nand_chip->ecc.layout->eccbytes;
		/* Modify to fit device character */
		nand_chip->ecc.size = hw->nand_ecc_size;
		nand_chip->ecc.bytes = hw->nand_ecc_bytes;
	} else {
		/* devinfo.sparesize-OOB_AVAI_PER_SECTOR*(devinfo.pagesize/sector_size); */
		nand_chip->ecc.layout->eccbytes = 64;
		hw->nand_ecc_bytes = nand_chip->ecc.layout->eccbytes;
		/* Modify to fit device character */
		nand_chip->ecc.size = hw->nand_ecc_size;
		nand_chip->ecc.bytes = hw->nand_ecc_bytes;
	}
	nand_chip->subpagesize = devinfo.sectorsize;
	nand_chip->subpage_size = devinfo.sectorsize;

	for (i = 0; i < nand_chip->ecc.layout->eccbytes; i++) {
		nand_chip->ecc.layout->eccpos[i] =
			OOB_AVAI_PER_SECTOR * (devinfo.pagesize / sector_size) + i;
	}
	/* MSG(INIT, "[NAND] pagesz:%d , oobsz: %d,eccbytes: %d\n", */
	/* devinfo.pagesize,  sizeof(g_kCMD.au1OOB),nand_chip->ecc.layout->eccbytes); */


	/* MSG(INIT, "Support this Device in MTK table! %x \r\n", id); */
#if CFG_RANDOMIZER
	if (devinfo.vendor != VEND_NONE) {
		/* mtk_nand_randomizer_config(&devinfo.feature_set.randConfig); */
#if 0
		if ((devinfo.feature_set.randConfig.type == RAND_TYPE_SAMSUNG) ||
			(devinfo.feature_set.randConfig.type == RAND_TYPE_TOSHIBA)) {
			MSG(INIT, "[NAND]USE Randomizer\n");
			use_randomizer = TRUE;
		} else {
			MSG(INIT, "[NAND]OFF Randomizer\n");
			use_randomizer = FALSE;
		}
#endif	/* only charge for efuse bonding */
#ifdef CONFIG_MTK_LEGACY
		if ((*EFUSE_RANDOM_CFG) & EFUSE_RANDOM_ENABLE) {
#else
		if ((mtk_nfi_dev_comp->chip_ver == 1) || (mtk_nfi_dev_comp->chip_ver == 2)) {
			efuse_index = 26;
			EFUSE_RANDOM_ENABLE = 0x00000004;
		} else if (mtk_nfi_dev_comp->chip_ver == 3) {
			efuse_index = 0;
			EFUSE_RANDOM_ENABLE = 0x00001000;
			nand_info("8167 nand randomizer efuse index %d", efuse_index);
		}
		/* the index of reg:0x102061C0 is 26 */
		if ((get_devinfo_with_index(efuse_index)) & EFUSE_RANDOM_ENABLE) {
#endif
			pr_notice("EFUSE RANDOM CFG is ON\n");
			use_randomizer = TRUE;
			pre_randomizer = TRUE;
		} else {
			pr_notice("EFUSE RANDOM CFG is OFF\n");
			use_randomizer = FALSE;
			pre_randomizer = FALSE;
		}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
		if (devinfo.NAND_FLASH_TYPE == NAND_FLASH_TLC) {
			pr_notice("TLC NAND force RANDOM ON\n");
			use_randomizer = TRUE;
			pre_randomizer = TRUE;
		}
#endif
	}
#endif

	if ((devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX_16NM)
		|| (devinfo.feature_set.FeatureSet.rtype == RTYPE_HYNIX))
		HYNIX_RR_TABLE_READ(&devinfo);

	hw->nfi_bus_width = devinfo.iowidth;
#if 1
	if (devinfo.vendor == VEND_MICRON) {
		if (devinfo.feature_set.FeatureSet.Async_timing.feature != 0xFF) {
			struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet);
			/* u32 val = 0; */
			mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd,
						feature_set->Async_timing.address,
						(u8 *) (&feature_set->Async_timing.feature),
						sizeof(feature_set->Async_timing.feature));
			/* mtk_nand_GetFeature(mtd, feature_set->gfeatureCmd, \ */
			/* feature_set->Async_timing.address, (u8 *)(&val),4); */
			/* pr_debug("[ASYNC Interface]0x%X\n", val); */
#if CFG_2CS_NAND
			if (g_bTricky_CS) {
				DRV_WriteReg16(NFI_CSEL_REG16, NFI_TRICKY_CS);
				mtk_nand_SetFeature(mtd, (u16) feature_set->sfeatureCmd,
							feature_set->Async_timing.address,
							(u8 *) (&feature_set->Async_timing.feature),
							sizeof(feature_set->Async_timing.feature));
				DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
			}
#endif
		}
	}
#endif
	/* MSG(INIT, "AHB Clock(0x%x) ",DRV_Reg32(PERICFG_BASE+0x5C)); */
	/* DRV_WriteReg32(PERICFG_BASE+0x5C, 0x1); */
	/* MSG(INIT, "AHB Clock(0x%x)",DRV_Reg32(PERICFG_BASE+0x5C)); */
	DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);
	/* MSG(INIT, "Kernel Nand Timing:0x%x!\n", DRV_Reg32(NFI_ACCCON_REG32)); */

	/* 16-bit bus width */
	if (hw->nfi_bus_width == 16) {
		pr_notice("Set the 16-bit I/O settings!\n");
		nand_chip->options |= NAND_BUSWIDTH_16;
	}

	mtk_dev = &pdev->dev;
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32))) {
		dev_err(&pdev->dev, "set dma mask fail\n");
		nand_pr_err("set dma mask fail");
	} else
		pr_notice("set dma mask ok\n");
	init_completion(&g_comp_AHB_Done);
	init_completion(&g_comp_WR_Done);
	init_completion(&g_comp_ER_Done);
	init_completion(&g_comp_Busy_Ret);
#ifdef CONFIG_OF
	err = request_irq(MT_NFI_IRQ_ID, mtk_nand_irq_handler, IRQF_TRIGGER_NONE, "mtk-nand", NULL);
#else
	err = request_irq(MT_NFI_IRQ_ID, mtk_nand_irq_handler, IRQF_DISABLED, "mtk-nand", NULL);
#endif

	if (err != 0) {
		nand_pr_err("Request IRQ fail: err = %d", err);
		goto out;
	}

	if (!g_i4Interrupt)
		disable_irq(MT_NFI_IRQ_ID);

#if 0
	if (devinfo.advancedmode & CACHE_READ) {
		nand_chip->ecc.read_multi_page_cache = NULL;
		/* nand_chip->ecc.read_multi_page_cache = mtk_nand_read_multi_page_cache; */
		/* MSG(INIT, "Device %x support cache read \r\n",id); */
	} else
		nand_chip->ecc.read_multi_page_cache = NULL;
#endif
	mtd->oobsize = devinfo.sparesize;
	/* Scan to find existence of the device */
	if (nand_scan(mtd, hw->nfi_cs_num)) {
		nand_pr_err("nand_scan fail.");
		err = -ENXIO;
		goto out;
	}

#ifdef CONFIG_MNTL_SUPPORT
	err = mtk_nand_data_info_init();
#endif
	g_page_size = mtd->writesize;
	g_block_size = devinfo.blocksize << 10;
	PAGES_PER_BLOCK = (u32) (g_block_size / g_page_size);
	/* MSG(INIT, "g_page_size(%d) g_block_size(%d)\n",g_page_size, g_block_size); */
#if CFG_2CS_NAND
	g_nanddie_pages = (u32) (nand_chip->chipsize >> nand_chip->page_shift);
	if (devinfo.two_phyplane)
		g_nanddie_pages <<= 1;
	/* if(devinfo.ttarget == TTYPE_2DIE) */
	/* { */
	/* g_nanddie_pages = g_nanddie_pages / 2; */
	/* } */
	if (g_b2Die_CS) {
		nand_chip->chipsize <<= 1;
		/* MSG(INIT, "[Bean]%dMB\n", (u32)(nand_chip->chipsize/1024/1024)); */
	}
	/* MSG(INIT, "[Bean]g_nanddie_pages %x\n", g_nanddie_pages); */
#endif
#if CFG_COMBO_NAND
#ifdef PART_SIZE_BMTPOOL
	if (PART_SIZE_BMTPOOL) {
		bmt_sz = (PART_SIZE_BMTPOOL) >> nand_chip->phys_erase_shift;
	} else
#endif
	{
		temp = nand_chip->chipsize;
		do_div(temp, ((devinfo.blocksize * 1024) & 0xFFFFFFFF));
#ifdef CONFIG_MNTL_SUPPORT
	bmt_sz = (int)(((u32) temp) / 100);
#else
	bmt_sz = (int)(((u32) temp) / 100 * 6);
#endif
	}
	/* if (manu_id == 0x45) */
	/* { */
	/* bmt_sz = bmt_sz * 2; */
	/* } */
#endif
	platform_set_drvdata(pdev, host);

	if (hw->nfi_bus_width == 16) {
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			NFI_SET_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
		} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
			NFI_SET_REG32(NFI_PAGEFMT_REG32, PAGEFMT_DBYTE_EN);
		} else {
			nand_pr_err("mtk_nfi_dev_comp->chip_ver=%d",
				mtk_nfi_dev_comp->chip_ver);
		}
	}

	nand_chip->select_chip(mtd, 0);
#if defined(MTK_COMBO_NAND_SUPPORT)
#if CFG_COMBO_NAND
	nand_chip->chipsize -= (bmt_sz * g_block_size);
#else
	nand_chip->chipsize -= (PART_SIZE_BMTPOOL);
#endif
	/* #if CFG_2CS_NAND */
	/* if(g_b2Die_CS) */
	/* { */
	/* nand_chip->chipsize -= (PART_SIZE_BMTPOOL);	// if 2CS nand need cut down again */
	/* } */
	/* #endif */
#else
	nand_chip->chipsize -= (BMT_POOL_SIZE) << nand_chip->phys_erase_shift;
#endif
	mtd->size = nand_chip->chipsize;
#if NAND_READ_PERFORMANCE
	struct timeval stimer, etimer;

	do_gettimeofday(&stimer);
	for (i = 256; i < 512; i++) {
		mtk_nand_read_page(mtd, nand_chip, local_buffer, i);
		pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer,
			*((int *)local_buffer + 1), *((int *)local_buffer + 2),
			*((int *)local_buffer + 3));
		pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 4,
			*((int *)local_buffer + 5), *((int *)local_buffer + 6),
			*((int *)local_buffer + 7));
		pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 8,
			*((int *)local_buffer + 9), *((int *)local_buffer + 10),
			*((int *)local_buffer + 11));
		pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 12,
			*((int *)local_buffer + 13), *((int *)local_buffer + 14),
			*((int *)local_buffer + 15));
	}
	do_gettimeofday(&etimer);
	pr_debug("[NAND Read Perf.Test] %ld MB/s\n",
		   (g_page_size * 256) / Cal_timediff(&etimer, &stimer));
#endif

	if (devinfo.vendor != VEND_NONE) {
		mtk_nand_interface_switch(mtd);
#if CFG_2CS_NAND
		if (g_bTricky_CS) {
			DRV_WriteReg16(NFI_CSEL_REG16, NFI_TRICKY_CS);
			mtk_nand_interface_switch(mtd);
			DRV_WriteReg16(NFI_CSEL_REG16, NFI_DEFAULT_CS);
		}
#endif
		/* MSG(INIT, "[nand_interface_config] %d\n",err); */
		/* u32 regp; */
		/* for (regp = 0xF0206000; regp <= 0xF020631C; regp+=4) */
		/* pr_debug("[%08X]0x%08X\n", regp, DRV_Reg32(regp)); */
#if NAND_READ_PERFORMANCE
		do_gettimeofday(&stimer);
		for (i = 256; i < 512; i++) {
			mtk_nand_read_page(mtd, nand_chip, local_buffer, i);
			pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer,
				*((int *)local_buffer + 1), *((int *)local_buffer + 2),
				*((int *)local_buffer + 3));
			pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 4,
				*((int *)local_buffer + 5), *((int *)local_buffer + 6),
				*((int *)local_buffer + 7));
			pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 8,
				*((int *)local_buffer + 9), *((int *)local_buffer + 10),
				*((int *)local_buffer + 11));
			pr_debug("[%d]0x%x %x %x %x\n", i, *(int *)local_buffer + 12,
				*((int *)local_buffer + 13), *((int *)local_buffer + 14),
				*((int *)local_buffer + 15));
		}
		do_gettimeofday(&etimer);
		pr_debug("[NAND Read Perf.Test] %d MB/s\n",
			(g_page_size * 256) / Cal_timediff(&etimer, &stimer));
		while (1)
			;
#endif
	}

#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
	mtk_pmt_reset();
#endif

	if (!g_bmt) {
#if defined(MTK_COMBO_NAND_SUPPORT)
#if CFG_COMBO_NAND
		g_bmt = init_bmt(nand_chip, bmt_sz);
		if (!g_bmt) {
#else
		g_bmt = init_bmt(nand_chip, ((PART_SIZE_BMTPOOL) >> nand_chip->phys_erase_shift));
		if (!g_bmt) {
#endif
#else
		g_bmt = init_bmt(nand_chip, BMT_POOL_SIZE);
		if (!g_bmt) {
#endif
			nand_pr_err("Error: init bmt failed");
			return 0;
		}
	}

	nand_chip->chipsize -= (PMT_POOL_SIZE) * (devinfo.blocksize * 1024);
	mtd->size = nand_chip->chipsize;
#if KERNEL_NAND_UNIT_TEST
	err = mtk_nand_unit_test(nand_chip, mtd);
	if (err == 0)
		pr_debug("Thanks to GOD, UNIT Test OK!\n");
#endif
#ifdef PMT
	part_init_pmt(mtd, (u8 *) &g_exist_Partition[0]);
	err = mtd_device_register(mtd, g_exist_Partition, part_num);
#else
	err = mtd_device_register(mtd, g_pasStatic_Partition, part_num);
#endif

#ifdef CONFIG_MNTL_SUPPORT
	err = mtk_nand_ops_init(mtd, nand_chip);
#endif

#ifdef TLC_UNIT_TEST
	err = mtk_tlc_unit_test(nand_chip, mtd);
	switch (err) {
	case 0:
		pr_warn("TLC UNIT Test OK!\n");
		pr_warn("TLC UNIT Test OK!\n");
		pr_warn("TLC UNIT Test OK!\n");
		break;
	case 1:
		pr_warn("TLC UNIT Test fail: SLC mode fail!\n");
		pr_warn("TLC UNIT Test fail: SLC mode fail!\n");
		pr_warn("TLC UNIT Test fail: SLC mode fail!\n");
		break;
	case 2:
		pr_warn("TLC UNIT Test fail: TLC mode fail!\n");
		pr_warn("TLC UNIT Test fail: TLC mode fail!\n");
		pr_warn("TLC UNIT Test fail: TLC mode fail!\n");
		break;
	default:
		pr_warn("TLC UNIT Test fail: wrong return!\n");
		break;
	}
#endif

#ifdef _MTK_NAND_DUMMY_DRIVER_
	dummy_driver_debug = 0;
#endif

	/* Successfully!! */
	if (!err) {
		/* MSG(INIT, "[mtk_nand] probe successfully!\n"); */
		nand_disable_clock();
		return err;
	}

	/* Fail!! */
out:
	nand_pr_err("[NFI] err = %d!", err);
	nand_release(mtd);
	platform_set_drvdata(pdev, NULL);
	kfree(host);
	nand_disable_clock();
	nand_unprepare_clock();
	return err;
}

/******************************************************************************
 * mtk_nand_suspend
 *
 * DESCRIPTION:
 *	 Suspend the nand device!
 *
 * PARAMETERS:
 *	 struct platform_device *pdev : device structure
 *
 * RETURNS:
 *	 0 : Success
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static int mtk_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mtk_nand_host *host = platform_get_drvdata(pdev);
#if !defined(CONFIG_MTK_LEGACY)
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	  int ret = 0;
#endif
#endif
	/* struct mtd_info *mtd = &host->mtd; */
	/* backup register */
#ifdef CONFIG_PM

	if (host->saved_para.suspend_flag == 0) {
		nand_enable_clock();
		/* Save NFI register */
		host->saved_para.sNFI_CNFG_REG16 = DRV_Reg16(NFI_CNFG_REG16);
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			host->saved_para.sNFI_PAGEFMT_REG16 = DRV_Reg16(NFI_PAGEFMT_REG16);
		} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
			host->saved_para.sNFI_PAGEFMT_REG32 = DRV_Reg32(NFI_PAGEFMT_REG32);
		} else {
			nand_pr_err("[NFI] mtk_nfi_dev_comp->chip_ver=%d",
				mtk_nfi_dev_comp->chip_ver);
		}
		host->saved_para.sNFI_CON_REG16 = DRV_Reg32(NFI_CON_REG16);
		host->saved_para.sNFI_ACCCON_REG32 = DRV_Reg32(NFI_ACCCON_REG32);
		host->saved_para.sNFI_INTR_EN_REG16 = DRV_Reg16(NFI_INTR_EN_REG16);
		host->saved_para.sNFI_IOCON_REG16 = DRV_Reg16(NFI_IOCON_REG16);
		host->saved_para.sNFI_CSEL_REG16 = DRV_Reg16(NFI_CSEL_REG16);
		host->saved_para.sNFI_DEBUG_CON1_REG16 = DRV_Reg16(NFI_DEBUG_CON1_REG16);

		/* save ECC register */
		host->saved_para.sECC_ENCCNFG_REG32 = DRV_Reg32(ECC_ENCCNFG_REG32);
		/* host->saved_para.sECC_FDMADDR_REG32 = DRV_Reg32(ECC_FDMADDR_REG32); */
		host->saved_para.sECC_DECCNFG_REG32 = DRV_Reg32(ECC_DECCNFG_REG32);
		/* for sync mode */
		mtk_nand_interface_async();
#ifdef CONFIG_MTK_PMIC_MT6397
		hwPowerDown(MT65XX_POWER_LDO_VMCH, "NFI");
#else
#if defined(CONFIG_MTK_LEGACY)
		hwPowerDown(MT6323_POWER_LDO_VMCH, "NFI");
#else
#if !defined(CONFIG_FPGA_EARLY_PORTING)
		ret = regulator_disable(mtk_nand_regulator);
		if (ret != 0)
			nand_pr_err("[NFI] Suspend regulator disable failed: %d", ret);
#endif
#endif
#endif
		nand_disable_clock();
		nand_unprepare_clock();
		host->saved_para.suspend_flag = 1;
	} else {
		pr_debug("[NFI] Suspend twice !\n");
	}
#endif

	pr_debug("[NFI] Suspend !\n");
	return 0;
}

/******************************************************************************
 * mtk_nand_resume
 *
 * DESCRIPTION:
 *	 Resume the nand device!
 *
 * PARAMETERS:
 *	 struct platform_device *pdev : device structure
 *
 * RETURNS:
 *	 0 : Success
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static int mtk_nand_resume(struct platform_device *pdev)
{
	struct mtk_nand_host *host = platform_get_drvdata(pdev);
#if !defined(CONFIG_MTK_LEGACY)
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	  int ret = 0;
#endif
#endif
	/* struct mtd_info *mtd = &host->mtd;  //for test */
	/* struct nand_chip *chip = mtd->priv; */
	/* struct gFeatureSet *feature_set = &(devinfo.feature_set.FeatureSet); //for test */
	/* int val = -1;   // for test */

#ifdef CONFIG_PM

	if (host->saved_para.suspend_flag == 1) {
		/* restore NFI register */
#ifdef CONFIG_MTK_PMIC_MT6397
		hwPowerOn(MT65XX_POWER_LDO_VMCH, VOL_3300, "NFI");
#else
#if defined(CONFIG_MTK_LEGACY)
		hwPowerOn(MT6323_POWER_LDO_VMCH, VOL_3300, "NFI");
#else
#if !defined(CONFIG_FPGA_EARLY_PORTING)
		ret = regulator_set_voltage(mtk_nand_regulator, 3300000, 3300000);
		if (ret != 0)
			nand_pr_err("[NFI] Resume regulator set vol failed: %d", ret);

		ret = regulator_enable(mtk_nand_regulator);
		if (ret != 0)
			nand_pr_err("[NFI] Resume regulator_enable failed: %d", ret);
#endif
#endif
#endif
		udelay(200);
		nand_prepare_clock();
		nand_enable_clock();
		DRV_WriteReg16(NFI_CNFG_REG16, host->saved_para.sNFI_CNFG_REG16);
		if (mtk_nfi_dev_comp->chip_ver == 1) {
			DRV_WriteReg16(NFI_PAGEFMT_REG16, host->saved_para.sNFI_PAGEFMT_REG16);
		} else if ((mtk_nfi_dev_comp->chip_ver == 2) || (mtk_nfi_dev_comp->chip_ver == 3)) {
			DRV_WriteReg32(NFI_PAGEFMT_REG32, host->saved_para.sNFI_PAGEFMT_REG32);
		} else {
			nand_pr_err("[NFI] Resume ERROR, mtk_nfi_dev_comp->chip_ver=%d",
				mtk_nfi_dev_comp->chip_ver);
		}
		DRV_WriteReg32(NFI_CON_REG16, host->saved_para.sNFI_CON_REG16);
		DRV_WriteReg32(NFI_ACCCON_REG32, host->saved_para.sNFI_ACCCON_REG32);
		DRV_WriteReg16(NFI_IOCON_REG16, host->saved_para.sNFI_IOCON_REG16);
		DRV_WriteReg16(NFI_CSEL_REG16, host->saved_para.sNFI_CSEL_REG16);
		DRV_WriteReg16(NFI_DEBUG_CON1_REG16, host->saved_para.sNFI_DEBUG_CON1_REG16);

		/* restore ECC register */
		DRV_WriteReg32(ECC_ENCCNFG_REG32, host->saved_para.sECC_ENCCNFG_REG32);
		/* DRV_WriteReg32(ECC_FDMADDR_REG32 ,host->saved_para.sECC_FDMADDR_REG32); */
		DRV_WriteReg32(ECC_DECCNFG_REG32, host->saved_para.sECC_DECCNFG_REG32);

		/* Reset NFI and ECC state machine */
		/* Reset the state machine and data FIFO, because flushing FIFO */
		(void)mtk_nand_reset();
		/* Reset ECC */
		DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
		while (!DRV_Reg16(ECC_DECIDLE_REG16))
			;

		DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
		while (!DRV_Reg32(ECC_ENCIDLE_REG32))
			;


		/* Initialize interrupt. Clear interrupt, read clear. */
		DRV_Reg16(NFI_INTR_REG16);

		DRV_WriteReg16(NFI_INTR_EN_REG16, host->saved_para.sNFI_INTR_EN_REG16);

		/* mtk_nand_interface_config(&host->mtd); */
		mtk_nand_device_reset();
		mtk_nand_interface_switch(&host->mtd);

#if 0
		mtk_nand_interface_async();
		(void)mtk_nand_reset();
		DRV_WriteReg16(NFI_NAND_TYPE_CNFG_REG32, 0);

		clk_disable_unprepare(nfi_2xclk);
		clk_prepare_enable(nfi_1xclk_sel);
		clk_set_parent(nfi_1xclk_sel, nfi_ahb_clk);
		clk_disable_unprepare(nfi_1xclk_sel);

		NFI_SET_REG32(NFI_DEBUG_CON1_REG16, NFI_BYPASS);
		NFI_SET_REG32(ECC_BYPASS_REG32, ECC_BYPASS);
		DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);
#endif
		nand_disable_clock();
		host->saved_para.suspend_flag = 0;
	} else {
		pr_debug("[NFI] Resume twice !\n");
	}
#endif
	pr_debug("[NFI] Resume !\n");
	return 0;
}

/******************************************************************************
 * mtk_nand_remove
 *
 * DESCRIPTION:
 *	 unregister the nand device file operations !
 *
 * PARAMETERS:
 *	 struct platform_device *pdev : device structure
 *
 * RETURNS:
 *	 0 : Success
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/

static int mtk_nand_remove(struct platform_device *pdev)
{
	struct mtk_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &host->mtd;

	nand_release(mtd);

	kfree(host);

	nand_disable_clock();
	nand_unprepare_clock();
	return 0;
}

static void mtk_nand_shutdown(struct platform_device *pdev)
{
	struct mtk_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &host->mtd;

#ifdef CONFIG_MNTL_SUPPORT
	nand_get_device(mtd, FL_WRITING);
	mtk_nand_interface_async();
	nand_release_device(mtd);
#endif
}

/******************************************************************************
 * NAND OTP operations
 * ***************************************************************************/
#if (defined(NAND_OTP_SUPPORT) && SAMSUNG_OTP_SUPPORT)
unsigned int samsung_OTPQueryLength(unsigned int *QLength)
{
	*QLength = SAMSUNG_OTP_PAGE_NUM * g_page_size;
	return 0;
}

unsigned int samsung_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
	struct mtd_info *mtd = &host->mtd;
	unsigned int rowaddr, coladdr;
	unsigned int u4Size = g_page_size;
	unsigned int timeout = 0xFFFF;
	unsigned int bRet;
	unsigned int sec_num = mtd->writesize >> host->hw->nand_sec_shift;

	if (PageAddr >= SAMSUNG_OTP_PAGE_NUM)
		return OTP_ERROR_OVERSCOPE;

	/* Col -> Row; LSB first */
	coladdr = 0x00000000;
	rowaddr = Samsung_OTP_Page[PageAddr];

	pr_debug("[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

	/* Power on NFI HW component. */
	nand_get_device(mtd, FL_READING);
	mtk_nand_reset();
	(void)mtk_nand_set_command(0x30);
	mtk_nand_reset();
	(void)mtk_nand_set_command(0x65);

	pr_debug("[%s]: Start to read data from OTP area\n", __func__);

	if (!mtk_nand_reset()) {
		bRet = OTP_ERROR_RESET;
		goto cleanup;
	}

	mtk_nand_set_mode(CNFG_OP_READ);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

	DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);

	if (g_bHwEcc)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

	mtk_nand_set_autoformat(true);
	if (g_bHwEcc)
		ECC_Decode_Start();

	if (!mtk_nand_set_command(NAND_CMD_READ0)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_set_address(coladdr, rowaddr, 2, 3)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_set_command(NAND_CMD_READSTART)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_read_page_data(mtd, BufferPtr, u4Size)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mtk_nand_read_fdm_data(SparePtr, sec_num);

	mtk_nand_stop_read();

	pr_debug("[%s]: End to read data from OTP area\n", __func__);

	bRet = OTP_SUCCESS;

cleanup:

	mtk_nand_reset();
	(void)mtk_nand_set_command(0xFF);
	nand_release_device(mtd);
	return bRet;
}

unsigned int samsung_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
	struct mtd_info *mtd = &host->mtd;
	unsigned int rowaddr, coladdr;
	unsigned int u4Size = g_page_size;
	unsigned int timeout = 0xFFFF;
	unsigned int bRet;
	unsigned int sec_num = mtd->writesize >> 9;

	if (PageAddr >= SAMSUNG_OTP_PAGE_NUM)
		return OTP_ERROR_OVERSCOPE;

	/* Col -> Row; LSB first */
	coladdr = 0x00000000;
	rowaddr = Samsung_OTP_Page[PageAddr];

	pr_debug("[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);
	nand_get_device(mtd, FL_READING);
	mtk_nand_reset();
	(void)mtk_nand_set_command(0x30);
	mtk_nand_reset();
	(void)mtk_nand_set_command(0x65);

	pr_debug("[%s]: Start to write data to OTP area\n", __func__);

	if (!mtk_nand_reset()) {
		bRet = OTP_ERROR_RESET;
		goto cleanup;
	}

	mtk_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg32(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

	DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr));
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);

	if (g_bHwEcc)
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	else
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);

	mtk_nand_set_autoformat(true);

	ECC_Encode_Start();

	if (!mtk_nand_set_command(NAND_CMD_SEQIN)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_set_address(coladdr, rowaddr, 2, 3)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mtk_nand_status_ready(STA_NAND_BUSY)) {
		bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mtk_nand_write_fdm_data((struct nand_chip *)mtd->priv, BufferPtr, sec_num);
	(void)mtk_nand_write_page_data(mtd, BufferPtr, u4Size);
	if (!mtk_nand_check_RW_count(u4Size)) {
		pr_debug("[%s]: Check RW count timeout !\n", __func__);
		bRet = OTP_ERROR_TIMEOUT;
		goto cleanup;
	}

	mtk_nand_stop_write();
	(void)mtk_nand_set_command(NAND_CMD_PAGEPROG);
	while (DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY)
		;

	bRet = OTP_SUCCESS;

	pr_debug("[%s]: End to write data to OTP area\n", __func__);

cleanup:
	mtk_nand_reset();
	(void)mtk_nand_set_command(NAND_CMD_RESET);
	nand_release_device(mtd);
	return bRet;
}

static int mt_otp_open(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev),
		MINOR(inode->i_rdev));
	filp->private_data = (int *)OTP_MAGIC_NUM;
	return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev),
		MINOR(inode->i_rdev));
	return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset, void *buff_ptr,
			 unsigned int length, unsigned int *status)
{
	unsigned int i = 0, ret = 0;
	char *BufAddr = (char *)buff_ptr;
	unsigned int PageAddr, AccessLength = 0;
	int Status = 0;

	static char *p_D_Buff;
	char S_Buff[64];

	p_D_Buff = kmalloc(g_page_size, GFP_KERNEL);
	if (!p_D_Buff) {
		ret = -ENOMEM;
		*status = OTP_ERROR_NOMEM;
		goto exit;
	}

	pr_debug("[%s]: %s (0x%x) length:(%d bytes) !\n", __func__, access_type ? "WRITE" : "READ",
		offset, length);

	while (1) {
		PageAddr = offset / g_page_size;
		if (access_type == FS_OTP_READ) {
			memset(p_D_Buff, 0xff, g_page_size);
			memset(S_Buff, 0xff, (sizeof(char) * 64));

			pr_debug("[%s]: Read Access of page (%d)\n", __func__, PageAddr);

			Status = g_mtk_otp_fuc.OTPRead(PageAddr, p_D_Buff, &S_Buff);
			*status = Status;

			if (Status != OTP_SUCCESS) {
				pr_debug("[%s]: Read status (%d)\n", __func__, Status);
				break;
			}

			AccessLength = g_page_size - (offset % g_page_size);

			if (length >= AccessLength) {
				memcpy(BufAddr, (p_D_Buff + (offset % g_page_size)), AccessLength);
			} else {
				/* last time */
				memcpy(BufAddr, (p_D_Buff + (offset % g_page_size)), length);
			}
		} else if (access_type == FS_OTP_WRITE) {
			AccessLength = g_page_size - (offset % g_page_size);
			memset(p_D_Buff, 0xff, g_page_size);
			memset(S_Buff, 0xff, (sizeof(char) * 64));

			if (length >= AccessLength) {
				memcpy((p_D_Buff + (offset % g_page_size)), BufAddr, AccessLength);
			} else {
				/* last time */
				memcpy((p_D_Buff + (offset % g_page_size)), BufAddr, length);
			}

			Status = g_mtk_otp_fuc.OTPWrite(PageAddr, p_D_Buff, &S_Buff);
			*status = Status;

			if (Status != OTP_SUCCESS) {
				pr_debug("[%s]: Write status (%d)\n", __func__, Status);
				break;
			}
		} else {
			nand_pr_err("not either read nor write operations !");
			break;
		}

		offset += AccessLength;
		BufAddr += AccessLength;
		if (length <= AccessLength) {
			length = 0;
			break;
		}
		length -= AccessLength;
		pr_debug("[%s]: Remaining %s (%d) !\n", __func__,
			access_type ? "WRITE" : "READ", length);
	}
error:
	kfree(p_D_Buff);
exit:
	return ret;
}

static long mt_otp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0, i = 0;
	static char *pbuf;

	void __user *uarg = (void __user *)arg;
	struct otp_ctl otpctl;

	/* Lock */
	spin_lock(&g_OTPLock);

	if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl))) {
		ret = -EFAULT;
		goto exit;
	}

	if (false == g_bInitDone) {
		nand_pr_err("ERROR: NAND Flash Not initialized !!");
		ret = -EFAULT;
		goto exit;
	}
	pbuf = kmalloc_array(otpctl.Length, sizeof(char), GFP_KERNEL);
	if (!pbuf) {
		ret = -ENOMEM;
		goto exit;
	}

	switch (cmd) {
	case OTP_GET_LENGTH:
		pr_debug("OTP IOCTL: OTP_GET_LENGTH\n");
		g_mtk_otp_fuc.OTPQueryLength(&otpctl.QLength);
		otpctl.status = OTP_SUCCESS;
		pr_debug("OTP IOCTL: The Length is %d\n", otpctl.QLength);
		break;
	case OTP_READ:
		pr_debug("OTP IOCTL: OTP_READ Offset(0x%x), Length(0x%x)\n", otpctl.Offset,
			otpctl.Length);
		memset(pbuf, 0xff, sizeof(char) * otpctl.Length);

		mt_otp_access(FS_OTP_READ, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);

		if (copy_to_user(otpctl.BufferPtr, pbuf, (sizeof(char) * otpctl.Length))) {
			nand_pr_err("OTP IOCTL: Copy to user buffer Error !");
			goto error;
		}
		break;
	case OTP_WRITE:
		pr_debug("OTP IOCTL: OTP_WRITE Offset(0x%x), Length(0x%x)\n", otpctl.Offset,
			otpctl.Length);
		if (copy_from_user(pbuf, otpctl.BufferPtr, (sizeof(char) * otpctl.Length))) {
			nand_pr_err("OTP IOCTL: Copy from user buffer Error !");
			goto error;
		}
		mt_otp_access(FS_OTP_WRITE, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);
		break;
	default:
		ret = -EINVAL;
	}

	ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

error:
	kfree(pbuf);
exit:
	spin_unlock(&g_OTPLock);
	return ret;
}

static const struct file_operations nand_otp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mt_otp_ioctl,
	.open = mt_otp_open,
	.release = mt_otp_release,
};

static struct miscdevice nand_otp_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "otp",
	.fops = &nand_otp_fops,
};
#endif

static struct platform_driver mtk_nand_driver = {
	.probe = mtk_nand_probe,
	.remove = mtk_nand_remove,
	.shutdown = mtk_nand_shutdown,
	.suspend = mtk_nand_suspend,
	.resume = mtk_nand_resume,
	.driver = {
		   .name = "mtk-nand",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mtk_nfi_of_match,
#endif
		   },
};

/******************************************************************************
 * mtk_nand_init
 *
 * DESCRIPTION:
 *	 Init the device driver !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static int __init mtk_nand_init(void)
{
	g_i4Interrupt = 1;
	if (g_i4Interrupt)
		pr_debug("Enable IRQ for NFI module!\n");

#if defined(NAND_OTP_SUPPORT)
	int err = 0;

	pr_debug("OTP: register NAND OTP device ...\n");
	err = misc_register(&nand_otp_dev);
	if (unlikely(err)) {
		nand_pr_err("OTP: failed to register NAND OTP device!");
		return err;
	}
	spin_lock_init(&g_OTPLock);
#endif

#if (defined(NAND_OTP_SUPPORT) && SAMSUNG_OTP_SUPPORT)
	g_mtk_otp_fuc.OTPQueryLength = samsung_OTPQueryLength;
	g_mtk_otp_fuc.OTPRead = samsung_OTPRead;
	g_mtk_otp_fuc.OTPWrite = samsung_OTPWrite;
#endif

	mtk_nand_fs_init();

	return platform_driver_register(&mtk_nand_driver);
}

/******************************************************************************
 * mtk_nand_exit
 *
 * DESCRIPTION:
 *	 Free the device driver !
 *
 * PARAMETERS:
 *	 None
 *
 * RETURNS:
 *	 None
 *
 * NOTES:
 *	 None
 *
 ******************************************************************************/
static void __exit mtk_nand_exit(void)
{
	pr_debug("MediaTek Nand driver exit, version %s\n", VERSION);
#if defined(NAND_OTP_SUPPORT)
	misc_deregister(&nand_otp_dev);
#endif

#ifdef SAMSUNG_OTP_SUPPORT
	g_mtk_otp_fuc.OTPQueryLength = NULL;
	g_mtk_otp_fuc.OTPRead = NULL;
	g_mtk_otp_fuc.OTPWrite = NULL;
#endif

	platform_driver_unregister(&mtk_nand_driver);
	mtk_nand_fs_exit();
}
late_initcall(mtk_nand_init);
module_exit(mtk_nand_exit);
/* MODULE_LICENSE("GPL"); */
