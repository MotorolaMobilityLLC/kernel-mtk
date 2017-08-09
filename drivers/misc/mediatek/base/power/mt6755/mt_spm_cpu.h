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

#ifndef _MT_SPM_CPU
#define _MT_SPM_CPU

#include <linux/kernel.h>
#include <linux/io.h>

#ifdef CONFIG_OF
extern void __iomem *spm_cpu_base;
extern void __iomem *scp_i2c0_base;
extern void __iomem *scp_i2c1_base;
extern void __iomem *scp_i2c2_base;
extern u32 spm_irq_0;
extern u32 spm_irq_1;
extern u32 spm_irq_2;
extern u32 spm_irq_3;
extern u32 spm_irq_4;
extern u32 spm_irq_5;
extern u32 spm_irq_6;
extern u32 spm_irq_7;

#undef SPM_BASE
#define SPM_BASE spm_cpu_base
#else
#include <mach/mt_reg_base.h>
#endif

/* #include <mach/mt_irq.h> */
#include <mt-plat/sync_write.h>

/**************************************
 * Config and Parameter
 **************************************/

/* FIXME: for bring up */
#ifdef CONFIG_OF
#undef SPM_I2C0_BASE
#undef SPM_I2C1_BASE
#undef SPM_I2C2_BASE
#define SPM_I2C0_BASE	scp_i2c0_base
#define SPM_I2C1_BASE	scp_i2c1_base
#define SPM_I2C2_BASE	scp_i2c2_base

#define SPM_IRQ0_ID		spm_irq_0
#define SPM_IRQ1_ID		spm_irq_1
#define SPM_IRQ2_ID		spm_irq_2
#define SPM_IRQ3_ID		spm_irq_3
#define SPM_IRQ4_ID		spm_irq_4
#define SPM_IRQ5_ID		spm_irq_5
#define SPM_IRQ6_ID		spm_irq_6
#define SPM_IRQ7_ID		spm_irq_7
#else
#define SPM_BASE		SLEEP_BASE

#define SPM_I2C0_BASE	0xF0059C00	/* SCP_I2C0_BASE */
#define SPM_I2C1_BASE	0xF0059C00	/* SCP_I2C1_BASE */
#define SPM_I2C2_BASE	0xF0059C00	/* SCP_I2C2_BASE */

#define SPM_IRQ0_ID		195	/* SLEEP_IRQ_BIT0_ID */
#define SPM_IRQ1_ID		196	/* SLEEP_IRQ_BIT1_ID */
#define SPM_IRQ2_ID		197	/* SLEEP_IRQ_BIT2_ID */
#define SPM_IRQ3_ID		198	/* SLEEP_IRQ_BIT3_ID */
#define SPM_IRQ4_ID		199	/* SLEEP_IRQ_BIT4_ID */
#define SPM_IRQ5_ID		200	/* SLEEP_IRQ_BIT5_ID */
#define SPM_IRQ6_ID		201	/* SLEEP_IRQ_BIT6_ID */
#define SPM_IRQ7_ID		202	/* SLEEP_IRQ_BIT7_ID */
#endif

#include "mt_spm_reg.h"

enum SPM_WAKE_SRC_LIST;

typedef enum {
	WR_NONE = 0,
	WR_UART_BUSY = 1,
	WR_PCM_ASSERT = 2,
	WR_PCM_TIMER = 3,
	WR_WAKE_SRC = 4,
	WR_UNKNOWN = 5,
} wake_reason_t;

struct twam_sig {
	u32 sig0;		/* signal 0: config or status */
	u32 sig1;		/* signal 1: config or status */
	u32 sig2;		/* signal 2: config or status */
	u32 sig3;		/* signal 3: config or status */
};

typedef void (*twam_handler_t) (struct twam_sig *twamsig);

/* for power management init */
extern int spm_module_init(void);

/* for ANC in talking */
extern void spm_mainpll_on_request(const char *drv_name);
extern void spm_mainpll_on_unrequest(const char *drv_name);

/* for TWAM in MET */
extern void spm_twam_register_handler(twam_handler_t handler);
extern void spm_twam_enable_monitor(const struct twam_sig *twamsig, bool speed_mode);
extern void spm_twam_disable_monitor(void);

/* for Vcore DVFS */
extern int spm_go_to_ddrdfs(u32 spm_flags, u32 spm_data);


/**************************************
 * Macro and Inline
 **************************************/
#define get_high_cnt(sigsta)		((sigsta) & 0x3ff)
#define get_high_percent(sigsta)	((get_high_cnt(sigsta) * 100 + 511) / 1023)

#endif
