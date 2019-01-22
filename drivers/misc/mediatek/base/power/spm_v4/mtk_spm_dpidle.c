/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <mt-plat/mtk_secure_api.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* #include <mach/irqs.h> */
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
#include <mtk_spm_idle.h>
#include <mtk_cpuidle.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif
#include <mach/mtk_gpt.h>
#include <mt-plat/mtk_ccci_common.h>
#include <mtk_spm_misc.h>
#include <mt-plat/upmu_common.h>

#include <mtk_spm_dpidle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>

#include <mtk_power_gs_api.h>

#include <mt-plat/mtk_io.h>

/*
 * only for internal debug
 */
#define DPIDLE_TAG     "[DP] "
#define dpidle_dbg(fmt, args...)	pr_debug(DPIDLE_TAG fmt, ##args)

#define SPM_PWAKE_EN            1
#define SPM_PCMWDT_EN           1
#define SPM_BYPASS_SYSPWREQ     1

#define I2C_CHANNEL 2

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

#define CA70_BUS_CONFIG          0xF020002C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */
#define CA71_BUS_CONFIG          0xF020022C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */

#define SPM_USE_TWAM_DEBUG	0

#define	DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA	20
#define	DPIDLE_LOG_DISCARD_CRITERIA			5000	/* ms */

#define reg_read(addr)         __raw_readl(IOMEM(addr))

enum spm_deepidle_step {
	SPM_DEEPIDLE_ENTER = 0x00000001,
	SPM_DEEPIDLE_ENTER_UART_SLEEP = 0x00000003,
	SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI = 0x00000007,
	SPM_DEEPIDLE_ENTER_WFI = 0x000000ff,
	SPM_DEEPIDLE_LEAVE_WFI = 0x000001ff,
	SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI = 0x000003ff,
	SPM_DEEPIDLE_ENTER_UART_AWAKE = 0x000007ff,
	SPM_DEEPIDLE_LEAVE = 0x00000fff,
	SPM_DEEPIDLE_SLEEP_DPIDLE = 0x80000000
};

static int spm_dormant_sta;

static inline void spm_dpidle_footprint(enum spm_deepidle_step step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_deepidle_val(step);
#endif
}

#ifdef CONFIG_FPGA_EARLY_PORTING
static const u32 dpidle_binary[] = {
	0xa1d58407, 0xa1d70407, 0x81f68407, 0x80358400, 0x1b80001f, 0x20000000,
	0x80300400, 0x80328400, 0xa1d28407, 0x81f20407, 0x81f88407, 0xe8208000,
	0x108c0610, 0x20000000, 0x81431801, 0xd8000245, 0x17c07c1f, 0x80318400,
	0x81f00407, 0xa1dd0407, 0x1b80001f, 0x20000424, 0x81fd0407, 0x18c0001f,
	0x108c0604, 0x1910001f, 0x108c0604, 0x813f8404, 0xe0c00004, 0xc28055c0,
	0x1290041f, 0xa19d8406, 0xa1dc0407, 0x1880001f, 0x00000006, 0xc0c05fc0,
	0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0x81fc0407,
	0x1b00001f, 0xffffffff, 0x81fc8407, 0x1b80001f, 0x20000004, 0x88c0000c,
	0xffffffff, 0xd8200703, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0xd0000ba0, 0x17c07c1f, 0xe8208000, 0x108c0610, 0x10000000, 0x1880001f,
	0x108c0028, 0xc0c054a0, 0xe080000f, 0xd82008e3, 0x17c07c1f, 0x81f00407,
	0xa1dc0407, 0x1b00001f, 0x00000006, 0xd0000ba0, 0x17c07c1f, 0xe080001f,
	0x81431801, 0xd8000985, 0x17c07c1f, 0xa0118400, 0x81bd8406, 0xa0128400,
	0xa1dc0407, 0x1b00001f, 0x00000001, 0xc28055c0, 0x129f841f, 0xc28055c0,
	0x1290841f, 0xa1d88407, 0xa1d20407, 0x81f28407, 0xa1d68407, 0xa0100400,
	0xa0158400, 0x81f70407, 0x81f58407, 0xf0000000, 0x17c07c1f, 0x81409801,
	0xd8000d45, 0x17c07c1f, 0x18c0001f, 0x108c0318, 0xc0c04b40, 0x1200041f,
	0x80310400, 0x1b80001f, 0x2000000e, 0xa0110400, 0xe8208000, 0x108c0610,
	0x40000000, 0xe8208000, 0x108c0610, 0x00000000, 0xc28055c0, 0x1291041f,
	0x18c0001f, 0x108c01d0, 0x1910001f, 0x108c01d0, 0xa1100404, 0xe0c00004,
	0xa19e0406, 0xa1dc0407, 0x1880001f, 0x00000058, 0xc0c05fc0, 0x17c07c1f,
	0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x108c01d0,
	0x1910001f, 0x108c01d0, 0x81300404, 0xe0c00004, 0x81409801, 0xd80011e5,
	0x17c07c1f, 0x18c0001f, 0x108c0318, 0xc0c04fe0, 0x17c07c1f, 0xc28055c0,
	0x1291841f, 0x81be0406, 0xa1dc0407, 0x1880001f, 0x00000006, 0xc0c05fc0,
	0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f, 0xc0804240,
	0x17c07c1f, 0xc0803f80, 0x17c07c1f, 0xe8208000, 0x102101a4, 0x00010000,
	0x18c0001f, 0x10230068, 0x1910001f, 0x10230068, 0x89000004, 0xffffff0f,
	0xe0c00004, 0x1910001f, 0x10230068, 0xe8208000, 0x108c0408, 0x00001fff,
	0xa1d80407, 0xc28055c0, 0x1292041f, 0xa19e8406, 0x80cf1801, 0xa1dc0407,
	0xd8001743, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0001780, 0x17c07c1f,
	0x1880001f, 0x000100a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x1b80001f, 0x20000fdf, 0x81f80407, 0xe8208000,
	0x108c0408, 0x0000ffff, 0x1910001f, 0x108c0170, 0xd8201903, 0x80c41001,
	0x80cf9801, 0xd8001a23, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0x18c0001f,
	0x10230068, 0x1910001f, 0x10230068, 0xa9000004, 0x000000f0, 0xe0c00004,
	0x1910001f, 0x10230068, 0x1880001f, 0x108c0028, 0xc0c051a0, 0xe080000f,
	0xd8201c63, 0x17c07c1f, 0xa1d80407, 0xd0002100, 0x17c07c1f, 0xe080001f,
	0x1890001f, 0x108c0604, 0x80c68801, 0x81429801, 0xa1400c05, 0xd8001de5,
	0x17c07c1f, 0xc0c05780, 0x17c07c1f, 0xc28055c0, 0x1296841f, 0xe8208000,
	0x102101a8, 0x00070000, 0xc0803c00, 0x17c07c1f, 0xc0803d20, 0x17c07c1f,
	0xc28055c0, 0x1292841f, 0x81be8406, 0xa19f8406, 0x80cf1801, 0xa1dc0407,
	0xd8002043, 0x17c07c1f, 0x1880001f, 0x00000058, 0xd0002080, 0x17c07c1f,
	0x1880001f, 0x00000090, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x814e9801, 0xd82021e5, 0x17c07c1f, 0xc0c03f80,
	0x17c07c1f, 0x18c0001f, 0x108c038c, 0xe0e00011, 0xe0e00031, 0xe0e00071,
	0xe0e000f1, 0xe0e001f1, 0xe0e003f1, 0xe0e007f1, 0xe0e00ff1, 0xe0e01ff1,
	0xe0e03ff1, 0xe0e07ff1, 0xe0f07ff1, 0x1b80001f, 0x20000020, 0xe0f07ff3,
	0xe0f07ff2, 0x80350400, 0x1b80001f, 0x2000001a, 0x80378400, 0x1b80001f,
	0x20000208, 0x80338400, 0x1b80001f, 0x2000001a, 0x81f98407, 0xc28055c0,
	0x1293041f, 0xa19f0406, 0x80ce9801, 0xa1dc0407, 0xd80026c3, 0x17c07c1f,
	0x1880001f, 0x00000090, 0xd00027e0, 0x17c07c1f, 0x80cf9801, 0xd80027a3,
	0x17c07c1f, 0x1880001f, 0x000100a0, 0xd00027e0, 0x17c07c1f, 0x1880001f,
	0x000080a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f, 0xc0c04780,
	0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x814e9801, 0xd8202985, 0x17c07c1f,
	0xc0c03f80, 0x17c07c1f, 0xa1d98407, 0xa0138400, 0xa0178400, 0xa0150400,
	0x18c0001f, 0x108c038c, 0xe0f07ff3, 0xe0f07ff1, 0xe0e00ff1, 0xe0e000f1,
	0xe0e00001, 0xc28055c0, 0x1293841f, 0x81bf0406, 0x80ce9801, 0xa1dc0407,
	0xd8002c43, 0x17c07c1f, 0x1880001f, 0x00000058, 0xd0002d60, 0x17c07c1f,
	0x80cf9801, 0xd8202d23, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0002d60,
	0x17c07c1f, 0x1880001f, 0x00008060, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02,
	0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0xc0c03f80,
	0x17c07c1f, 0xc28055c0, 0x1294041f, 0xa19f8406, 0x80cf1801, 0xa1dc0407,
	0xd8003003, 0x17c07c1f, 0x1880001f, 0x00010060, 0xd0003040, 0x17c07c1f,
	0x1880001f, 0x000100a0, 0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f,
	0xf0000000, 0x17c07c1f, 0x1880001f, 0x108c0028, 0xc0c051a0, 0xe080000f,
	0xd8003443, 0x17c07c1f, 0xe080001f, 0xc0803c00, 0x17c07c1f, 0xc28055c0,
	0x1294841f, 0x81bf8406, 0x80cf1801, 0xa1dc0407, 0xd8003383, 0x17c07c1f,
	0x1880001f, 0x00008060, 0xd00033c0, 0x17c07c1f, 0x1880001f, 0x000080a0,
	0xc0c05fc0, 0x17c07c1f, 0xa0800c02, 0x1300081f, 0xf0000000, 0x17c07c1f,
	0x814e9801, 0xd8203525, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0xc0c05cc0,
	0x17c07c1f, 0x18c0001f, 0x108c0404, 0x1910001f, 0x108c0404, 0xa1108404,
	0xe0c00004, 0x1b80001f, 0x20000104, 0xc0c05ea0, 0x17c07c1f, 0xc28055c0,
	0x1295041f, 0xa19d0406, 0xa1dc0407, 0x1890001f, 0x108c0148, 0xa0978402,
	0x80b70402, 0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0x814e9801, 0xd82038e5, 0x17c07c1f, 0xc0c03f80, 0x17c07c1f, 0xc0c05cc0,
	0x17c07c1f, 0x18c0001f, 0x108c0404, 0x1910001f, 0x108c0404, 0x81308404,
	0xe0c00004, 0x1b80001f, 0x20000104, 0xc0c05ea0, 0x17c07c1f, 0xc28055c0,
	0x1295841f, 0x81bd0406, 0xa1dc0407, 0x1890001f, 0x108c0148, 0x80b78402,
	0xa0970402, 0x1300081f, 0xc0c04780, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0xa0188400, 0xa0190400, 0xa0110400, 0x80398400, 0x803a0400, 0x803a8400,
	0x803b0400, 0xf0000000, 0x17c07c1f, 0xa1da0407, 0xa1dd8407, 0x803b8400,
	0xa0168400, 0xa0140400, 0xe8208000, 0x108c0320, 0x00000f0d, 0xe8208000,
	0x108c0320, 0x00000f0f, 0xe8208000, 0x108c0320, 0x00000f1e, 0xe8208000,
	0x108c0320, 0x00000f12, 0xf0000000, 0x17c07c1f, 0xa01b0400, 0xa01a8400,
	0xa01c0400, 0xa01a0400, 0x1b80001f, 0x20000004, 0xa0198400, 0x1b80001f,
	0x20000004, 0x80388400, 0x803c0400, 0x80310400, 0x1b80001f, 0x20000004,
	0x80390400, 0x81f08407, 0x808ab401, 0xd8004182, 0x17c07c1f, 0x80380400,
	0xf0000000, 0x17c07c1f, 0xe8208000, 0x108c0320, 0x00000f16, 0xe8208000,
	0x108c0320, 0x00000f1e, 0xe8208000, 0x108c0320, 0x00000f0e, 0x1b80001f,
	0x2000001b, 0xe8208000, 0x108c0320, 0x00000f0c, 0xe8208000, 0x108c0320,
	0x00000f0d, 0xe8208000, 0x108c0320, 0x00000e0d, 0xe8208000, 0x108c0320,
	0x00000c0d, 0xe8208000, 0x108c0320, 0x0000080d, 0xe8208000, 0x108c0320,
	0x0000000d, 0x80340400, 0x80368400, 0x1b80001f, 0x20000209, 0xa01b8400,
	0x1b80001f, 0x20000209, 0x1b80001f, 0x20000209, 0x81fa0407, 0x81fd8407,
	0xf0000000, 0x17c07c1f, 0x816f9801, 0x814e9805, 0xd8204b05, 0x17c07c1f,
	0x1880001f, 0x108c0028, 0xe080000f, 0x1111841f, 0xa1d08407, 0xd8204924,
	0x80eab401, 0xd80048a3, 0x01200404, 0x81f08c07, 0x1a00001f, 0x108c00b0,
	0xe2000003, 0xd8204aa3, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0x81fc0407, 0xd0004b00, 0x17c07c1f, 0xe080001f, 0xc0c03c00, 0x17c07c1f,
	0xf0000000, 0x17c07c1f, 0xe0f07f16, 0x1380201f, 0xe0f07f1e, 0x1380201f,
	0xe0f07f0e, 0xe0f07f0c, 0x1900001f, 0x108c0374, 0xe120003e, 0xe120003c,
	0xe1200038, 0xe1200030, 0xe1200020, 0xe1200000, 0x1880001f, 0x108c03a4,
	0x1900001f, 0x0400fffc, 0xe0800004, 0x1900001f, 0x0000fffc, 0xe0800004,
	0x1b80001f, 0x20000104, 0xe0f07f0d, 0xe0f07e0d, 0x1b80001f, 0x20000104,
	0xe0f07c0d, 0x1b80001f, 0x20000104, 0xe0f0780d, 0x1b80001f, 0x20000104,
	0xe0f0700d, 0xf0000000, 0x17c07c1f, 0x1900001f, 0x108c0374, 0xe120003f,
	0x1900001f, 0x108c03a4, 0x1880001f, 0x0c00fffc, 0xe1000002, 0xe0f07f0d,
	0xe0f07f0f, 0xe0f07f1e, 0xe0f07f12, 0xf0000000, 0x17c07c1f, 0xa0180400,
	0x1111841f, 0xa1d08407, 0xd8205284, 0x80eab401, 0xd8005203, 0x01200404,
	0xd82053c3, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000, 0x81fc0407,
	0x81f08407, 0xe8208000, 0x108c00b0, 0x00000001, 0xf0000000, 0x17c07c1f,
	0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000, 0x17c07c1f, 0xa1d00407,
	0x1b80001f, 0x20000208, 0x10c07c1f, 0x1900001f, 0x108c00b0, 0xe1000003,
	0xf0000000, 0x17c07c1f, 0x18c0001f, 0x108c0604, 0x1910001f, 0x108c0604,
	0xa1002804, 0xe0c00004, 0xf0000000, 0x17c07c1f, 0xa1d40407, 0x1391841f,
	0xa1d90407, 0x1392841f, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x100040f4,
	0x19300003, 0x17c07c1f, 0x813a0404, 0xe0c00004, 0x1b80001f, 0x20000003,
	0x18c0001f, 0x10004110, 0x19300003, 0x17c07c1f, 0xa11e8404, 0xe0c00004,
	0x1b80001f, 0x20000004, 0x18c0001f, 0x100041ec, 0x19300003, 0x17c07c1f,
	0x81380404, 0xe0c00004, 0x18c0001f, 0x100040f4, 0x19300003, 0x17c07c1f,
	0xa11a8404, 0xe0c00004, 0x18c0001f, 0x100041dc, 0x19300003, 0x17c07c1f,
	0x813d0404, 0xe0c00004, 0x18c0001f, 0x10004110, 0x19300003, 0x17c07c1f,
	0x813e8404, 0xe0c00004, 0xf0000000, 0x17c07c1f, 0x814f1801, 0xd8005e65,
	0x17c07c1f, 0x80350400, 0x1b80001f, 0x2000001a, 0x80378400, 0x1b80001f,
	0x20000208, 0x80338400, 0x1b80001f, 0x2000001a, 0x81f98407, 0xf0000000,
	0x17c07c1f, 0x814f1801, 0xd8005f85, 0x17c07c1f, 0xa1d98407, 0xa0138400,
	0xa0178400, 0xa0150400, 0xf0000000, 0x17c07c1f, 0x10c07c1f, 0x810d1801,
	0x810a1804, 0x816d1801, 0x814a1805, 0xa0d79003, 0xa0d71403, 0xf0000000,
	0x17c07c1f, 0x1b80001f, 0x20000300, 0xf0000000, 0x17c07c1f, 0xe8208000,
	0x110e0014, 0x00000002, 0xe8208000, 0x110e0020, 0x00000001, 0xe8208000,
	0x110e0004, 0x000000d6, 0x1a00001f, 0x110e0000, 0x1880001f, 0x110e0024,
	0x18c0001f, 0x20000152, 0xd820658a, 0x17c07c1f, 0xe220000a, 0xe22000f6,
	0xe8208000, 0x110e0024, 0x00000001, 0x1b80001f, 0x20000152, 0xe220008a,
	0xe2200001, 0xe8208000, 0x110e0024, 0x00000001, 0x1b80001f, 0x20000152,
	0xd0006740, 0x17c07c1f, 0xe220008a, 0xe2200000, 0xe8208000, 0x110e0024,
	0x00000001, 0x1b80001f, 0x20000152, 0xe220000a, 0xe22000f4, 0xe8208000,
	0x110e0024, 0x00000001, 0x1b80001f, 0x20000152, 0xf0000000, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0xe8208000,
	0x108c0620, 0x0000000e, 0x1990001f, 0x108c0600, 0xa9800006, 0xfe000000,
	0x1890001f, 0x108c061c, 0x80800402, 0xa19c0806, 0xe8208000, 0x108c0604,
	0x00000000, 0x81fc0407, 0x1b00001f, 0xffffffff, 0xa1dc0407, 0x1b00001f,
	0x00000000, 0x81401801, 0xd80103c5, 0x17c07c1f, 0x1b80001f, 0x5000aaaa,
	0xd00104a0, 0x17c07c1f, 0x1b80001f, 0xd00f0000, 0x81fc8407, 0x8880000c,
	0xffffffff, 0xd8011b02, 0x17c07c1f, 0xe8208000, 0x108c0408, 0x00001fff,
	0xe8208000, 0x108c040c, 0xe0003fff, 0xc0c05400, 0x81401801, 0xd8010885,
	0x17c07c1f, 0x80c41801, 0xd8010663, 0x17c07c1f, 0x81f60407, 0x18d0001f,
	0x108c0254, 0xc0c11f20, 0x17c07c1f, 0x18d0001f, 0x108c0250, 0xc0c12000,
	0x17c07c1f, 0x18c0001f, 0x108c0200, 0xc0c120c0, 0x17c07c1f, 0xe8208000,
	0x108c0260, 0x00000007, 0xc28055c0, 0x1296041f, 0x81401801, 0xd8010a25,
	0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000, 0x1b80001f, 0x30000004,
	0x81fc8407, 0x8880000c, 0xffffffff, 0xd80117c2, 0x17c07c1f, 0x81489801,
	0xd8010b25, 0x17c07c1f, 0x18c0001f, 0x108c037c, 0xe0e00013, 0xe0e00011,
	0xe0e00001, 0x81481801, 0xd8010c65, 0x17c07c1f, 0x18c0001f, 0x108c0370,
	0xe0f07ff3, 0xe0f07ff1, 0xe0e00ff1, 0xe0e000f1, 0xe0e00001, 0x81449801,
	0xd8010cc5, 0x17c07c1f, 0xc0c056c0, 0x17c07c1f, 0xa1d38407, 0xa0108400,
	0xa0120400, 0xa0130400, 0xa0170400, 0xa0148400, 0xe8208000, 0x108c0080,
	0x0000fff3, 0xa1dc0407, 0x18c0001f, 0x000100a0, 0x814a1801, 0xa0d79403,
	0x814a9801, 0xa0d89403, 0x1b00001f, 0xffffffdf, 0x1b80001f, 0x90100000,
	0x80cd9801, 0xc8e00003, 0x17c07c1f, 0x80ce1801, 0xc8e00be3, 0x17c07c1f,
	0x80cf1801, 0xc8e02143, 0x17c07c1f, 0x80cf9801, 0xc8e02e63, 0x17c07c1f,
	0x80ce9801, 0xc8e01363, 0x17c07c1f, 0x80cd1801, 0xc8e03483, 0x17c07c1f,
	0x81481801, 0xd8011465, 0x17c07c1f, 0x18c0001f, 0x108c0370, 0xe0e00011,
	0xe0e00031, 0xe0e00071, 0xe0e000f1, 0xe0e001f1, 0xe0e003f1, 0xe0e007f1,
	0xe0e00ff1, 0xe0e01ff1, 0xe0e03ff1, 0xe0e07ff1, 0xe0f07ff1, 0x1b80001f,
	0x20000020, 0xe0f07ff3, 0xe0f07ff2, 0x81489801, 0xd80115a5, 0x17c07c1f,
	0x18c0001f, 0x108c037c, 0xe0e00011, 0x1b80001f, 0x20000020, 0xe0e00013,
	0xe0e00012, 0x80348400, 0x1b80001f, 0x20000300, 0x80370400, 0x1b80001f,
	0x20000300, 0x80330400, 0x1b80001f, 0x20000104, 0x80308400, 0x80320400,
	0x81f38407, 0x81f90407, 0x81f40407, 0x81449801, 0xd80117c5, 0x17c07c1f,
	0x81401801, 0xd8011b05, 0x17c07c1f, 0x18d0001f, 0x108e0e10, 0x1890001f,
	0x108c0260, 0x81400c01, 0x81020c01, 0x80b01402, 0x80b09002, 0x18c0001f,
	0x108c0260, 0xe0c00002, 0x18c0001f, 0x108c0200, 0xc0c128a0, 0x17c07c1f,
	0x18d0001f, 0x108c0250, 0xc0c12760, 0x17c07c1f, 0x18d0001f, 0x108c0254,
	0xc0c12620, 0x17c07c1f, 0xe8208000, 0x108c0034, 0x000f0000, 0x81f48407,
	0xa1d60407, 0x81f10407, 0xe8208000, 0x108c0620, 0x0000000f, 0x18c0001f,
	0x108c041c, 0x1910001f, 0x108c0164, 0xe0c00004, 0x18c0001f, 0x108c0420,
	0x1910001f, 0x108c0150, 0xe0c00004, 0x18c0001f, 0x108c0614, 0x1910001f,
	0x108c0614, 0x09000004, 0x00000001, 0xe0c00004, 0x1ac0001f, 0x55aa55aa,
	0xe8208000, 0x108e0e00, 0x00000001, 0xd0013040, 0x17c07c1f, 0x1900001f,
	0x108c0278, 0xe100001f, 0x17c07c1f, 0xe2e01041, 0xf0000000, 0x17c07c1f,
	0x1900001f, 0x108c026c, 0xe100001f, 0xe2e01041, 0xf0000000, 0x17c07c1f,
	0x1900001f, 0x108c0204, 0x1940001f, 0x0000104d, 0xa1528405, 0xe1000005,
	0x81730405, 0xe1000005, 0xa1540405, 0xe1000005, 0x1950001f, 0x108c0204,
	0x808c1401, 0xd8212202, 0x17c07c1f, 0xa1538405, 0xe1000005, 0xa1508405,
	0xe1000005, 0x81700405, 0xe1000005, 0xa1520405, 0xe1000005, 0x81710405,
	0xe1000005, 0x81718405, 0xe1000005, 0x1880001f, 0x0000104d, 0xa0908402,
	0xe2c00002, 0x80b00402, 0xe2c00002, 0xa0920402, 0xe2c00002, 0x80b10402,
	0xe2c00002, 0x80b18402, 0xe2c00002, 0x1b80001f, 0x2000001a, 0xf0000000,
	0x17c07c1f, 0xe2e01045, 0x1910001f, 0x108e0e10, 0x1950001f, 0x108c0188,
	0x81001404, 0xd8212644, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0xe2e01045,
	0x1910001f, 0x108e0e14, 0x1950001f, 0x108c0188, 0x81001404, 0xd8212784,
	0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x18b0000b, 0x17c07c1f, 0xa0910402,
	0xe2c00002, 0xa0918402, 0xe2c00002, 0x80b08402, 0xe2c00002, 0x1900001f,
	0x108c0204, 0x1950001f, 0x108c0204, 0xa1510405, 0xe1000005, 0xa1518405,
	0xe1000005, 0x81708405, 0xe1000005, 0x1950001f, 0x108c0188, 0x81081401,
	0x81079404, 0x1950001f, 0x108c018c, 0x81081404, 0x81079404, 0xd8212ae4,
	0x17c07c1f, 0x1900001f, 0x108c0204, 0x1950001f, 0x108c0204, 0x81740405,
	0xe1000005, 0x1950001f, 0x108c0204, 0x814c1401, 0xd8012ce5, 0x17c07c1f,
	0x1b80001f, 0x2000001a, 0x1950001f, 0x108c0204, 0xa1530405, 0xe1000005,
	0x1b80001f, 0x20000003, 0x81728405, 0xe1000005, 0x80b20402, 0xe2c00002,
	0xa0900402, 0xe2c00002, 0x81720405, 0xe1000005, 0xa1500405, 0xe1000005,
	0x81738405, 0xe1000005, 0xf0000000, 0x17c07c1f, 0xf0000000, 0x17c07c1f
};
static struct pcm_desc dpidle_pcm = {
	.version	= "pcm_suspend_v42.0_r12_0xffffffff",
	.base		= dpidle_binary,
	.size		= 2436,
	.sess		= 2,
	.replace	= 0,
	.addr_2nd	= 0,
	.vec0		= EVENT_VEC(32, 1, 0, 0),	/* FUNC_26M_WAKEUP */
	.vec1		= EVENT_VEC(33, 1, 0, 41),	/* FUNC_26M_SLEEP */
	.vec4		= EVENT_VEC(34, 1, 0, 95),	/* FUNC_INFRA_WAKEUP */
	.vec5		= EVENT_VEC(35, 1, 0, 130),	/* FUNC_INFRA_SLEEP */
	.vec6		= EVENT_VEC(46, 1, 0, 420),	/* FUNC_NFC_CLK_BUF_ON */
	.vec7		= EVENT_VEC(47, 1, 0, 450),	/* FUNC_NFC_CLK_BUF_OFF */
	.vec8		= EVENT_VEC(36, 1, 0, 155),	/* FUNC_APSRC_WAKEUP */
	.vec9		= EVENT_VEC(37, 1, 0, 194),	/* FUNC_APSRC_SLEEP */
	.vec12		= EVENT_VEC(38, 1, 0, 266),	/* FUNC_VRF18_WAKEUP */
	.vec13		= EVENT_VEC(39, 1, 0, 327),	/* FUNC_VRF18_SLEEP */
	.vec14		= EVENT_VEC(47, 1, 0, 371),	/* FUNC_DDREN_WAKEUP */
	.vec15		= EVENT_VEC(48, 1, 0, 392),	/* FUNC_DDREN_SLEEP */
};
#endif

static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,

	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_AND, /* 0x1 */
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_0_dbc_en = 0x1,
	.md_ddr_en_1_dbc_en = 0,
	.md_mask_b = 0x1,
	.pmcu_mask_b = 0x1,
	.lte_mask_b = 0,
	.srcclkeni_mask_b = 0x1,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_ddr_en_dbc_en = 0x1,
	.conn_mask_b = 0x1,
	.conn_apsrc_sel = 0,

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_lte_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 0,
	.spm_ddren_req = 0,
	.spm_rsv_src_req = 0,
	.spm_ddren_2_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_SRC_MASK */
#if SPM_BYPASS_SYSPWREQ
	.csyspwreq_mask = 0x1,
#endif
	.ccif0_md_event_mask_b = 0x1,
	.ccif0_ap_event_mask_b = 0x1,
	.ccif1_md_event_mask_b = 0x1,
	.ccif1_ap_event_mask_b = 0x1,
	.ccifmd_md1_event_mask_b = 0,
	.ccifmd_md2_event_mask_b = 0,
	.dsi0_vsync_mask_b = 0,
	.dsi1_vsync_mask_b = 0,
	.dpi_vsync_mask_b = 0,
	.isp0_vsync_mask_b = 0,
	.isp1_vsync_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 0x1,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0x1,
	.md32_srcclkena_infra_mask_b = 0,
	.srcclkeni_infra_mask_b = 0,
	.md_apsrc_req_0_infra_mask_b = 0,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 0,
	.md32_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 0x1,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 0x1,
	.md_vrf18_req_1_mask_b = 0,
	.md1_dvfs_req_mask = 0x1,
	.cpu_dvfs_req_mask = 0x1,
	.emi_bw_dvfs_req_mask = 0x1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0,
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 0,
	.cpu_md_dvfs_req_merge_mask_b = 0,
	.md_ddr_en_dvfs_halt_mask_b = 0,
	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,
	.conn_ddr_en_mask_b = 0x1,
	.disp_req_mask_b = 0,
	.disp1_req_mask_b = 0,
	.mfg_req_mask_b = 0,
	.ufs_srcclkena_mask_b = 0x1, /* ufs hw signal */
	.ufs_vrf18_req_mask_b = 0x1, /* ufs hw signal */
	.ps_c2k_rccif_wake_mask_b = 0,
	.l1_c2k_rccif_wake_mask_b = 0,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
	.dramc_spcmd_apsrc_req_mask_b = 0,
	.emi_boost_dvfs_req_2_mask_b = 0,
	.emi_bw_dvfs_req_2_mask = 0,
	.gce_vrf18_req_mask_b = 0,

	/* SPM_WAKEUP_EVENT_MASK */
	.spm_wakeup_event_mask = 0xF1683A08,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0xFFFFFFFF,

	/* SPM_SRC3_MASK */
	.md_ddr_en_2_0_mask_b = 0,
	.md_ddr_en_2_1_mask_b = 0,
	.conn_ddr_en_2_mask_b = 0,
	.dramc_spcmd_apsrc_req_2_mask_b = 0,
	.spare1_ddren_2_mask_b = 0,
	.spare2_ddren_2_mask_b = 0,
	.ddren_emi_self_refresh_ch0_mask_b = 0,
	.ddren_emi_self_refresh_ch1_mask_b = 0,
	.ddren_mm_state_mask_b = 0,
	.ddren_md32_apsrc_req_mask_b = 0,
	.ddren_dqssoc_req_mask_b = 0,
	.ddren2_emi_self_refresh_ch0_mask_b = 0,
	.ddren2_emi_self_refresh_ch1_mask_b = 0,
	.ddren2_mm_state_mask_b = 0,
	.ddren2_md32_apsrc_req_mask_b = 0,
	.ddren2_dqssoc_req_mask_b = 0,

	/* MP0_CPU0_WFI_EN */
	.mp0_cpu0_wfi_en = 1,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 1,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 1,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 1,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 1,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 1,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 1,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 1,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_dpidle = {
#ifdef CONFIG_FPGA_EARLY_PORTING
	.pcmdesc	= &dpidle_pcm,
#endif
	.pwrctrl	= &dpidle_ctrl,
};

static unsigned int dpidle_log_discard_cnt;
static unsigned int dpidle_log_print_prev_time;

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_dpidle_notify_sspm_before_wfi(bool sleep_dpidle, u32 operation_cond, struct pwr_ctrl *pwrctrl)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= sleep_dpidle ?      SPM_OPT_SLEEP_DPIDLE : 0;
	spm_opt |= univpll_is_used() ? SPM_OPT_UNIVPLL_STAT : 0;
	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE) ? SPM_OPT_VCORE_LP_MODE : 0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) && !sleep_dpidle) ?
					SPM_OPT_XO_UFS_OFF :
					0;

	spm_d.u.suspend.spm_opt = spm_opt;
	spm_d.u.suspend.vcore_volt_pmic_val = pwrctrl->vcore_volt_pmic_val;

	ret = spm_to_sspm_command_async(SPM_DPIDLE_ENTER, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_dpidle_notify_spm_before_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_DPIDLE_ENTER);
	if (ret < 0)
		spm_crit2("SPM_DPIDLE_ENTER async wait: ret %d", ret);
}

static void spm_dpidle_notify_sspm_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	__spm_set_pcm_wdt(0);

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= sleep_dpidle ?      SPM_OPT_SLEEP_DPIDLE : 0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) && !sleep_dpidle) ?
					SPM_OPT_XO_UFS_OFF :
					0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_DPIDLE_LEAVE, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

void spm_dpidle_notify_sspm_after_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_DPIDLE_LEAVE);
	if (ret < 0)
		spm_crit2("SPM_DPIDLE_LEAVE async wait: ret %d", ret);
}
#else
static void spm_dpidle_notify_sspm_before_wfi(bool sleep_dpidle, u32 operation_cond, struct pwr_ctrl *pwrctrl)
{
}

static void spm_dpidle_notify_spm_before_wfi_async_wait(void)
{
}

static void spm_dpidle_notify_sspm_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
}

void spm_dpidle_notify_sspm_after_wfi_async_wait(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
	if (is_cpu_pdn(pwrctrl->pcm_flags))
		spm_dormant_sta = mtk_enter_idle_state(MTK_DPIDLE_MODE);
	else
		spm_dormant_sta = mtk_enter_idle_state(MTK_LEGACY_DPIDLE_MODE);

	if (spm_dormant_sta < 0)
		pr_err("dpidle spm_dormant_sta(%d) < 0\n", spm_dormant_sta);
}

static void spm_dpidle_pcm_setup_before_wfi(bool sleep_dpidle, u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage = 0;

	spm_dpidle_pre_process(operation_cond, pwrctrl);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = (!sleep_dpidle) ? spm_get_resource_usage() : 0;

	mt_secure_call(MTK_SIP_KERNEL_SPM_DPIDLE_ARGS, pwrctrl->pcm_flags, resource_usage, 0);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS, SPM_PWR_CTRL_DPIDLE, PWR_OPP_LEVEL, pwrctrl->opp_level);

	if (sleep_dpidle)
		mt_secure_call(MTK_SIP_KERNEL_SPM_SLEEP_DPIDLE_ARGS, pwrctrl->timer_val, pwrctrl->wake_src, 0);
}

static void spm_dpidle_pcm_setup_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
	spm_dpidle_post_process();
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = wakesrc;
		else
			__spm_dpidle.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = 0;
		else
			__spm_dpidle.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

static wake_reason_t spm_output_wake_reason(struct wake_status *wakesta,
											struct pcm_desc *pcmdesc,
											u32 log_cond,
											u32 operation_cond)
{
	wake_reason_t wr = WR_NONE;
	unsigned long int dpidle_log_print_curr_time = 0;
	bool log_print = false;
	static bool timer_out_too_short;

	if (log_cond & DEEPIDLE_LOG_FULL) {
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false, "dpidle");
		pr_info("oper_cond = %x\n", operation_cond);

		if (log_cond & DEEPIDLE_LOG_RESOURCE_USAGE)
			spm_resource_req_dump();
	} else if (log_cond & DEEPIDLE_LOG_REDUCED) {
		/* Determine print SPM log or not */
		dpidle_log_print_curr_time = spm_get_current_time_ms();

		if (wakesta->assert_pc != 0)
			log_print = true;
#if 0
		/* Not wakeup by GPT */
		else if ((wakesta->r12 & (0x1 << 4)) == 0)
			log_print = true;
		else if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			log_print = true;
#endif
		else if ((dpidle_log_print_curr_time - dpidle_log_print_prev_time) > DPIDLE_LOG_DISCARD_CRITERIA)
			log_print = true;

		if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			timer_out_too_short = true;

		/* Print SPM log */
		if (log_print == true) {
			dpidle_dbg("dpidle_log_discard_cnt = %d, timer_out_too_short = %d, oper_cond = %x\n",
						dpidle_log_discard_cnt,
						timer_out_too_short,
						operation_cond);
			wr = __spm_output_wake_reason(wakesta, pcmdesc, false, "dpidle");

			if (log_cond & DEEPIDLE_LOG_RESOURCE_USAGE)
				spm_resource_req_dump();

			dpidle_log_print_prev_time = dpidle_log_print_curr_time;
			dpidle_log_discard_cnt = 0;
			timer_out_too_short = false;
		} else {
			dpidle_log_discard_cnt++;

			wr = WR_NONE;
		}
	}

#ifdef CONFIG_MTK_ECCCI_DRIVER
	if (wakesta->r12 & WAKE_SRC_R12_MD2AP_PEER_WAKEUP_EVENT)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif

	return wr;
}


/* dpidle_active_status() for pmic_throttling_dlpt */
/* return 0 : entering dpidle recently ( > 1s) => normal mode(dlpt 10s) */
/* return 1 : entering dpidle recently (<= 1s) => light-loading mode(dlpt 20s) */
#define DPIDLE_ACTIVE_TIME		(1)
static struct timeval pre_dpidle_time;

int dpidle_active_status(void)
{
	struct timeval current_time;

	do_gettimeofday(&current_time);

	if ((current_time.tv_sec - pre_dpidle_time.tv_sec) > DPIDLE_ACTIVE_TIME)
		return 0;
	else if (((current_time.tv_sec - pre_dpidle_time.tv_sec) == DPIDLE_ACTIVE_TIME) &&
		(current_time.tv_usec > pre_dpidle_time.tv_usec))
		return 0;
	else
		return 1;
}
EXPORT_SYMBOL(dpidle_active_status);

wake_reason_t spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 log_cond, u32 operation_cond)
{
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	u32 cpu = spm_data;
	/* FIXME: */
	/* int ch; */

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER);

	pwrctrl = __spm_dpidle.pwrctrl;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* spm_set_dummy_read_addr(false); */

	/* need be called before spin_lock_irqsave() */
	/* FIXME: */
#if 0
	ch = get_channel_lock(0);
	pwrctrl->opp_level = __spm_check_opp_level(ch);
	pwrctrl->vcore_volt_pmic_val =
						__spm_get_vcore_volt_pmic_val(
							!!(operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE),
							ch);
	wakesta.dcs_ch = (u32)ch;
#endif

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

	spm_dpidle_notify_sspm_before_wfi(false, operation_cond, pwrctrl);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_dpidle_pcm_setup_before_wfi(false, cpu, pcmdesc, pwrctrl, operation_cond);

#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[1]);
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_dpidle_notify_spm_before_wfi_async_wait();

	/* Dump low power golden setting */
	if (operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN)
		; /* mt_power_gs_dump_dpidle(); */

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_SLEEP);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_WFI);

	spm_trigger_wfi_for_dpidle(pwrctrl);

#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[2]);
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_LEAVE_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN))
		request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	spm_dpidle_notify_sspm_after_wfi(false, operation_cond);

	spm_dpidle_footprint(SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_dpidle_pcm_setup_after_wfi(false, operation_cond);

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_AWAKE);

	wr = spm_output_wake_reason(&wakesta, pcmdesc, log_cond, operation_cond);

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	/* need be called after spin_unlock_irqrestore() */
	/* FIXME: */
	/* get_channel_unlock(); */

	spm_dpidle_footprint(0);

#if 1
	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();
#endif

	return wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 * pwake_time:
 *    >= 0  = specific wakeup period
 */
wake_reason_t spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data)
{
	u32 sec = 0;
	u32 dpidle_timer_val = 0;
	u32 dpidle_wake_src = 0;
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api;
	int wd_ret;
#endif
#endif
	static wake_reason_t last_wr = WR_NONE;
	/* struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc; */
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	int cpu = smp_processor_id();
	/* FIXME: */
	/* int ch; */

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER);

	pwrctrl = __spm_dpidle.pwrctrl;

	/* backup original dpidle setting */
	dpidle_timer_val = pwrctrl->timer_val;
	dpidle_wake_src = pwrctrl->wake_src;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* spm_set_dummy_read_addr(false); */

#if SPM_PWAKE_EN
	sec = _spm_get_wake_period(-1, last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

	pwrctrl->wake_src = spm_get_sleep_wakesrc();

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
	} else
		spm_crit2("FAILED TO GET WD API\n");
#endif

	/* need be called before spin_lock_irqsave() */
	/* FIXME: */
#if 0
	ch = get_channel_lock(0);
	pwrctrl->opp_level = __spm_check_opp_level(ch);
	pwrctrl->vcore_volt_pmic_val = __spm_get_vcore_volt_pmic_val(true, ch);
	wakesta.dcs_ch = (u32)ch;
#endif

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

	spm_dpidle_notify_sspm_before_wfi(true, DEEPIDLE_OPT_VCORE_LP_MODE, pwrctrl);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_crit2("sleep_deepidle, sec = %u, wakesrc = 0x%x [%u][%u]\n",
		sec, pwrctrl->wake_src,
		is_cpu_pdn(pwrctrl->pcm_flags),
		is_infra_pdn(pwrctrl->pcm_flags));

	spm_dpidle_pcm_setup_before_wfi(true, cpu, pcmdesc, pwrctrl, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_dpidle_notify_spm_before_wfi_async_wait();

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_SLEEP);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_WFI);

	spm_trigger_wfi_for_dpidle(pwrctrl);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_LEAVE_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	spm_dpidle_notify_sspm_after_wfi(false, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_dpidle_pcm_setup_after_wfi(true, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_AWAKE);

	last_wr = __spm_output_wake_reason(&wakesta, pcmdesc, true, "sleep_dpidle");

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	/* need be called after spin_unlock_irqrestore() */
	/* FIXME: */
	/* get_channel_unlock(); */

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	if (!wd_ret) {
		if (!pwrctrl->wdt_disable)
			wd_api->wd_resume_notify();
		else
			spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	}
#endif

	/* restore original dpidle setting */
	pwrctrl->timer_val = dpidle_timer_val;
	pwrctrl->wake_src = dpidle_wake_src;

	spm_dpidle_notify_sspm_after_wfi_async_wait();

	spm_dpidle_footprint(0);

#if 1
	if (last_wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();
#endif

	return last_wr;
}

void spm_deepidle_init(void)
{
	spm_deepidle_chip_init();
}

MODULE_DESCRIPTION("SPM-DPIdle Driver v0.1");
