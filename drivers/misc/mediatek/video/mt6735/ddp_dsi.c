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

#define LOG_TAG "DSI"

#include <linux/delay.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include "disp_drv_platform.h"

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
/*#include <mach/irqs.h>*/
#include "mtkfb.h"
#include "ddp_drv.h"
#include "ddp_manager.h"
#include "ddp_dump.h"
#include "ddp_irq.h"
#include "ddp_dsi.h"
#include "disp_log.h"
#include "disp_debug.h"
#ifdef CONFIG_MTK_LEGACY
#include <mt-plat/mt_gpio.h>
/* #include <cust_gpio_usage.h> */
#endif
#include "ddp_mmp.h"
#include "disp_dts_gpio.h"

/* static unsigned int _dsi_reg_update_wq_flag = 0; */
static DECLARE_WAIT_QUEUE_HEAD(_dsi_reg_update_wq);
atomic_t PMaster_enable = ATOMIC_INIT(0);

#include "ddp_reg.h"
#include "ddp_dsi.h"
#include "ddp_path.h"

/*...below is new dsi driver...*/
#define IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII 0
static int dsi_reg_op_debug;
#include <mt-plat/sync_write.h>

#ifndef CONFIG_MTK_CLKMGR
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/types.h>

static void __iomem *ddp_apmixed_base;
#ifndef AP_PLL_CON5
#define AP_PLL_CON5 (ddp_apmixed_base + 0x14)
#endif

#define DRV_Reg32(addr)                          (*(volatile unsigned int *const)(addr))
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)
#define clk_setl(addr, val) mt_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val) mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

void ddp_set_mipi26m(int en)
{
	if (en)
		clk_setl(AP_PLL_CON5, 1 << 3);
	else
		clk_clrl(AP_PLL_CON5, 1 << 3);
}
#endif /* CONFIG_MTK_CLKMGR */

#define DSI_OUTREG32(cmdq, addr, val) DISP_REG_SET(cmdq, addr, val)
#define DSI_BACKUPREG32(cmdq, hSlot, idx, addr) DISP_REG_BACKUP(cmdq, hSlot, idx, addr)
#define DSI_POLLREG32(cmdq, addr, mask, value) DISP_REG_CMDQ_POLLING(cmdq, addr, value, mask)

#define BIT_TO_VALUE(TYPE, bit)  \
	do { \
		TYPE r;\
		*(unsigned long *)(&r) = ((unsigned int)0x00000000);	  \
		r.bit = ~(r.bit);\
		r;\
		} while (0)\

#define DSI_MASKREG32(cmdq, REG, MASK, VALUE)	DISP_REG_MASK((cmdq), (REG), (VALUE), (MASK))

#define DSI_OUTREGBIT(cmdq, TYPE, REG, bit, value)  \
	do {\
		TYPE r;\
		TYPE v;\
		if (cmdq) {\
			*(unsigned int *)(&r) = ((unsigned int)0x00000000);	\
			r.bit = ~(r.bit);									\
			*(unsigned int *)(&v) = ((unsigned int)0x00000000);	\
			v.bit = value;										\
			DISP_REG_MASK(cmdq, &REG, AS_UINT32(&v), AS_UINT32(&r));\
		}		\
		else{										\
			mt_reg_sync_writel(INREG32(&REG), &r);	\
			r.bit = (value);	\
			DISP_REG_SET(cmdq, &REG, INREG32(&r));					\
		}				\
	} while (0)


#ifdef CONFIG_FPGA_EARLY_PORTING
#define MIPITX_Write60384(slave_addr, write_addr, write_data)			\
{	\
	pr_debug("MIPITX_Write60384:0x%x,0x%x,0x%x\n", slave_addr, write_addr, write_data);		\
	mt_reg_sync_writel(0x2, MIPITX_BASE+0x14);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x18);		\
	mt_reg_sync_writel(((unsigned int)slave_addr << 0x1), MIPITX_BASE+0x04);		\
	mt_reg_sync_writel(write_addr, MIPITX_BASE+0x0);		\
	mt_reg_sync_writel(write_data, MIPITX_BASE+0x0);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x24);		\
	while ((INREG32(MIPITX_BASE+0xC)&0x1) != 0x1)		\
		;		\
	mt_reg_sync_writel(0xFF, MIPITX_BASE+0xC);		\
	\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x14);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x18);		\
	mt_reg_sync_writel(((unsigned int)slave_addr << 0x1), MIPITX_BASE+0x04);		\
	mt_reg_sync_writel(write_addr, MIPITX_BASE+0x0);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x24);		\
	while ((INREG32(MIPITX_BASE+0xC)&0x1) != 0x1)		\
		;		\
	mt_reg_sync_writel(0xFF, MIPITX_BASE+0xC);		\
	\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x14);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x18);		\
	mt_reg_sync_writel(((unsigned int)slave_addr << 0x1)+1, MIPITX_BASE+0x04);		\
	mt_reg_sync_writel(0x1, MIPITX_BASE+0x24);		\
	while ((INREG32(MIPITX_BASE+0xC)&0x1) != 0x1)		\
		;		\
	mt_reg_sync_writel(0xFF, MIPITX_BASE+0xC);		\
	\
	pr_debug("MIPI write data = 0x%x, read data = 0x%x\n", write_data, INREG32(MIPITX_BASE));		\
	if (INREG32(MIPITX_BASE) == write_data)		\
		pr_debug("MIPI write success\n");		\
	else									\
		pr_debug("MIPI write fail\n");		\
}

#define MIPITX_INREG32(addr)			\
	({									\
		unsigned int val = 0;			\
		if (0)							\
			val = INREG32(addr);		\
		if (dsi_reg_op_debug) {			\
			pr_debug("[mipitx/inreg]%p=0x%08x\n", (void *)addr, val);\
		}								\
		val;							\
	})

#define MIPITX_OUTREG32(addr, val) \
	{\
		if (dsi_reg_op_debug) {		\
			pr_debug("[mipitx/reg]%p=0x%08x\n", (void *)addr, val); \
		}		\
		if (0) {			\
			mt_reg_sync_writel(val, addr);	\
		} \
	}

#define MIPITX_OUTREGBIT(TYPE, REG, bit, value)  \
	{\
		do {	\
			TYPE r;\
			if (0) {						\
				mt_reg_sync_writel(INREG32(&REG), &r);	  \
			}								\
			*(unsigned long *)(&r) = ((unsigned long)0x00000000);	  \
			r.bit = value;	  \
			MIPITX_OUTREG32(&REG, AS_UINT32(&r));	  \
			} while (0);\
	}

#define MIPITX_MASKREG32(x, y, z)  MIPITX_OUTREG32(x, (MIPITX_INREG32(x)&~(y))|(z))
#else
#define MIPITX_INREG32(addr)	\
	({							\
		unsigned int val = 0;	\
		val = INREG32(addr);	\
		if (dsi_reg_op_debug) {	\
			DISPMSG("[mipitx/inreg]%p=0x%08x\n", (void *)addr, val);	\
		}						\
		val;					\
	})

#define MIPITX_OUTREG32(addr, val) \
	{\
		if (dsi_reg_op_debug) {	\
			DISPMSG("[mipitx/reg]%p=0x%08x\n", (void *)addr, val);\
		} \
		mt_reg_sync_writel(val, addr);\
	}

#define MIPITX_OUTREGBIT(TYPE, REG, bit, value)  \
	{\
		do {	\
			TYPE r;\
			mt_reg_sync_writel(INREG32(&REG), &r);	  \
			r.bit = value;	  \
			MIPITX_OUTREG32(&REG, AS_UINT32(&r));	  \
			} while (0);\
	}

#define MIPITX_MASKREG32(x, y, z)  MIPITX_OUTREG32(x, (MIPITX_INREG32(x)&~(y))|(z))
#endif


#define DSI_INREG32(type, addr) INREG32(addr)

#define DSI_READREG32(type, dst, src) mt_reg_sync_writel(INREG32(src), dst)


struct t_dsi_context {
	unsigned int lcm_width;
	unsigned int lcm_height;
	cmdqRecHandle *handle;
	bool enable;
	volatile struct DSI_REGS regBackup;
	unsigned int cmdq_size;
	LCM_DSI_PARAMS dsi_params;
};

struct t_dsi_context _dsi_context[DSI_INTERFACE_NUM];
#define DSI_MODULE_BEGIN(x)	(x == DISP_MODULE_DSIDUAL?0:DSI_MODULE_to_ID(x))
#define DSI_MODULE_END(x)		(x == DISP_MODULE_DSIDUAL?1:DSI_MODULE_to_ID(x))
#define DSI_MODULE_to_ID(x)	(x == DISP_MODULE_DSI0?0:1)
#define DIFF_CLK_LANE_LP (0x10)

struct DSI_REGS *DSI_REG[2];
struct DSI_PHY_REGS *DSI_PHY_REG[2];
struct DSI_CMDQ_REGS *DSI_CMDQ_REG[2];
struct DSI_VM_CMDQ_REGS *DSI_VM_CMD_REG[2];

static wait_queue_head_t _dsi_cmd_done_wait_queue[2];
static wait_queue_head_t _dsi_dcs_read_wait_queue[2];
static wait_queue_head_t _dsi_wait_bta_te[2];
static wait_queue_head_t _dsi_wait_ext_te[2];
static wait_queue_head_t _dsi_wait_vm_done_queue[2];
static wait_queue_head_t _dsi_wait_vm_cmd_done_queue[2];
static wait_queue_head_t _dsi_wait_sleep_out_done_queue[2];
static bool waitRDDone;
static bool wait_vm_cmd_done;
static bool wait_sleep_out_done;


static int s_isDsiPowerOn;

static int dsi_currect_mode;

static int dsi_force_config;

static void _DSI_INTERNAL_IRQ_Handler(DISP_MODULE_ENUM module, unsigned int param)
{
	int i = 0;
	struct DSI_INT_STATUS_REG status;
	struct DSI_TXRX_CTRL_REG txrx_ctrl;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		/* status = DSI_REG[i]->DSI_INTSTA; */
		status = *(struct DSI_INT_STATUS_REG *) &param;
		if (status.RD_RDY) {
			/* write clear RD_RDY interrupt */

			/*  write clear RD_RDY interrupt must be before DSI_RACK */
			/*  because CMD_DONE will raise after DSI_RACK, */
			/*  so write clear RD_RDY after that will clear CMD_DONE too */
			/*do
			   {
			   send read ACK
			   DSI_REG->DSI_RACK.DSI_RACK = 1;
			   DSI_OUTREGBIT(NULL, struct DSI_RACK_REG,DSI_REG[i]->DSI_RACK,DSI_RACK,1);
			   pr_debug("send read ACK\n");
			   } while(DSI_REG[i]->DSI_INTSTA.BUSY); */
			waitRDDone = true;
			wake_up_interruptible(&_dsi_dcs_read_wait_queue[i]);
		}

		if (status.CMD_DONE) {
			/* DISPMSG("[callback]%s cmd dome\n", ddp_get_module_name(module)); */
			wake_up_interruptible(&_dsi_cmd_done_wait_queue[i]);
		}

		if (status.TE_RDY) {
			DSI_OUTREG32(NULL, &txrx_ctrl, INREG32(&DSI_REG[i]->DSI_TXRX_CTRL));
			if (txrx_ctrl.EXT_TE_EN == 1) {
				/* DISPMSG("[callback]%s  EXT  te\n", ddp_get_module_name(module)); */
				wake_up_interruptible(&_dsi_wait_ext_te[i]);
			} else {
				wake_up_interruptible(&_dsi_wait_bta_te[i]);
			}
		}

		if (status.VM_DONE)
			wake_up_interruptible(&_dsi_wait_vm_done_queue[i]);

		if (status.VM_CMD_DONE) {
			wait_vm_cmd_done = true;
			wake_up_interruptible(&_dsi_wait_vm_cmd_done_queue[i]);
		}
		if (status.SLEEPOUT_DONE) {
			wait_sleep_out_done = true;
			wake_up_interruptible(&_dsi_wait_sleep_out_done_queue[i]);
		}
	}
}


static DSI_STATUS DSI_Reset(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;
	unsigned int irq_en[2];

	if (cmdq)
		DISPMSG("DSI_RESET Protect may not work!/n");

	/* DSI_RESET Protect: backup & disable dsi interrupt */
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		irq_en[i] = AS_UINT32(&DSI_REG[i]->DSI_INTEN);
		DSI_OUTREG32(NULL, &DSI_REG[i]->DSI_INTEN, 0);
		DISPMSG("DSI_RESET backup dsi%d irq:0x%08x ", i, irq_en[i]);
	}

	/* do reset */
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[i]->DSI_COM_CTRL, DSI_RESET, 1);
		DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[i]->DSI_COM_CTRL, DSI_RESET, 0);
	}

	/* DSI_RESET Protect: restore dsi interrupt */
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREG32(NULL, &DSI_REG[i]->DSI_INTEN, irq_en[i]);

		DISPMSG("DSI_RESET restore dsi%d irq:0x%08x ", i,
		       AS_UINT32(&DSI_REG[i]->DSI_INTEN));
	}
	return DSI_STATUS_OK;
}

static int _dsi_is_video_mode(DISP_MODULE_ENUM module)
{
	if (DSI_REG[0]->DSI_MODE_CTRL.MODE == CMD_MODE)
		return 0;
	else
		return 1;
}

static DSI_STATUS DSI_SetMode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned int mode)
{
	int i = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
		DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL, MODE, mode);


	return DSI_STATUS_OK;
}

#if 0 /*apply this function might encounter function not declare*/
static void DSI_WaitForNotBusy(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;
	unsigned int count = 0;
	unsigned int tmp = 0;
	unsigned long long start_time, end_time;

	if (cmdq) {
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
			DSI_POLLREG32(cmdq, &DSI_REG[i]->DSI_INTSTA, 0x80000000, 0x0);

		return;
	}


	/*...dsi video is always in busy state... */
	if (_dsi_is_video_mode(module))
		return;


	start_time = sched_clock();
	while (1) {
		end_time = sched_clock();
		tmp = INREG32(&DSI_REG[i]->DSI_INTSTA);
		if (!(tmp & 0x80000000))
			break;

		if ((end_time - start_time) / 1000 > 2000000) {	/* 1s timeout */
			DISPERR("dsi wait not busy timeout,start at %lld, end at %lld\n",
				start_time, end_time);
			DSI_DumpRegisters(module, 1);
			DSI_Reset(module, NULL);
			break;
		}
	}
#endif

/* function defined but not used*/
#if 0
	static DSI_STATUS DSI_SetVdoFrmMode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
					    unsigned int mode) {
		int i = 0;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL, FRM_MODE,
				      mode);
		}

		return DSI_STATUS_OK;
	}
#endif
static DSI_STATUS DSI_SetSwitchMode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
					    unsigned int mode) {
		int i = 0;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (mode == 0) {	/* V2C */
/* DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG,DSI_REG[i]->DSI_MODE_CTRL,C2V_SWITCH_ON,0); */
				DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL,
					      V2C_SWITCH_ON, 1);
			} else	/* C2V */
/* DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG,DSI_REG[i]->DSI_MODE_CTRL,V2C_SWITCH_ON,0); */
				DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL,
					      C2V_SWITCH_ON, 1);

		}

		return DSI_STATUS_OK;
	}

/*function defined but not  used*/
#if 0
static DSI_STATUS DSI_SetBypassRack(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
					    unsigned int bypass) {
		int i = 0;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (bypass == 0) {
				DSI_OUTREGBIT(cmdq, struct DSI_RACK_REG, DSI_REG[i]->DSI_RACK,
					      DSI_RACK_BYPASS, 0);
			} else
				DSI_OUTREGBIT(cmdq, struct DSI_RACK_REG, DSI_REG[i]->DSI_RACK,
					      DSI_RACK_BYPASS, 1);

		}

		return DSI_STATUS_OK;
	}
#endif

DSI_STATUS DSI_DisableClk(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
#if 0
		int i = 0;

		DISPFUNC();
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
			DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[i]->DSI_COM_CTRL, DSI_EN, 0);

#endif
		return DSI_STATUS_OK;
	}

void DSI_sw_clk_trail(int module_idx)
{
		DEFINE_SPINLOCK(s_lock);
		unsigned long flags;

		register unsigned long *SW_CTRL_CON0_addr;
		/* init DSI clk lane software control */
		DISP_REG_SET(NULL, &DSI_REG[module_idx]->DSI_PHY_LCCON, 0x00000001);
		DISP_REG_SET(NULL, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON0, 0x00000031);
		DISP_REG_SET(NULL, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON1, 0x0F0F0F0F);
		DISP_REG_SET(NULL, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_EN, 0x00000001);

		/* control DSI clk trail duration */
		SW_CTRL_CON0_addr =
		    (unsigned long *)(&DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON0);
		spin_lock_irqsave(&s_lock, flags);
		/* force clk lane keep low */
		mt_reg_sync_writel(0x00000071, (volatile unsigned long *)SW_CTRL_CON0_addr);
		/* pull clk lane to LP state */
		mt_reg_sync_writel(0x0000000F, (volatile unsigned long *)SW_CTRL_CON0_addr);
		spin_unlock_irqrestore(&s_lock, flags);

		/* give back DSI clk lane control */
		DISP_REG_SET(NULL, &DSI_REG[module_idx]->DSI_PHY_LCCON, 0x00000000);
		DDP_REG_POLLING(&DSI_REG[module_idx]->DSI_STATE_DBG0, 0x00010000);
		DISP_REG_SET(NULL, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_EN, 0x00000000);


	}

void DSI_sw_clk_trail_cmdq(int module_idx, cmdqRecHandle cmdq)
{
		/* init DSI clk lane software control */
		DISP_REG_SET(cmdq, &DSI_REG[module_idx]->DSI_PHY_LCCON, 0x00000001);
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON0, 0x00000031);
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON1, 0x0F0F0F0F);
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_EN, 0x00000001);

		/* control DSI clk trail duration */
		/* force clk lane keep low */
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON0, 0x00000071);
		/* pull clk lane to LP state */
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_CON0, 0x0000000F);

		/* give back DSI clk lane control */
		DISP_REG_SET(cmdq, &DSI_REG[module_idx]->DSI_PHY_LCCON, 0x00000000);
		DISP_REG_CMDQ_POLLING(cmdq, &DSI_REG[module_idx]->DSI_STATE_DBG0, 0x00010000,
				      0x00010000);
		DISP_REG_SET(cmdq, &DSI_PHY_REG[module_idx]->MIPITX_DSI_SW_CTRL_EN, 0x00000000);

	}


void DSI_lane0_ULP_mode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enter)
{
		int i = 0;

		ASSERT(cmdq == NULL);

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (enter) {
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_RM_TRIG_EN, 0);
				mdelay(1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      Lx_ULPM_AS_L0, 1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_ULPM_EN, 0);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_ULPM_EN, 1);
/* mdelay(1); */
			} else {
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_ULPM_EN, 0);
				mdelay(1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      Lx_ULPM_AS_L0, 0);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_WAKEUP_EN, 1);
				mdelay(1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
					      L0_WAKEUP_EN, 0);
				mdelay(1);
			}
		}
	}


void DSI_clk_ULP_mode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enter)
{
		int i = 0;

		ASSERT(cmdq == NULL);

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (enter) {
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_ULPM_EN, 0);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_ULPM_EN, 1);
				mdelay(1);
			} else {
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_ULPM_EN, 0);
				mdelay(1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_WAKEUP_EN, 1);
				mdelay(1);
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_WAKEUP_EN, 0);
				mdelay(1);
			}
		}
	}

bool DSI_clk_HS_state(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	struct DSI_PHY_LCCON_REG tmpreg;

	DSI_READREG32((struct DSI_PHY_LCCON_REG *), &tmpreg, &DSI_REG[0]->DSI_PHY_LCCON);
	return tmpreg.LC_HS_TX_EN ? true : false;
}

void DSI_clk_HSLP_mode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
		int i = 0;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (cmdq)
				DSI_sw_clk_trail_cmdq(i, cmdq);
			else
				DSI_sw_clk_trail(i);

		}
	}

void DSI_manual_enter_HS(cmdqRecHandle cmdq)
{
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[0]->DSI_PHY_LCCON, LC_HS_TX_EN, 1);
	}
	void DSI_clk_HS_mode(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enter)
{
		int i = 0;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			if (enter) {	/* && !DSI_clk_HS_state(i, cmdq)) */
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_HS_TX_EN, 1);
			} else if (!enter) {	/* && DSI_clk_HS_state(i, cmdq)) */
				DSI_OUTREGBIT(cmdq, struct DSI_PHY_LCCON_REG, DSI_REG[i]->DSI_PHY_LCCON,
					      LC_HS_TX_EN, 0);
			}
		}
	}

const char *_dsi_cmd_mode_parse_state(unsigned int state)
{
		switch (state) {
		case 0x0001:
			return "idle";
		case 0x0002:
			return "Reading command queue for header";
		case 0x0004:
			return "Sending type-0 command";
		case 0x0008:
			return "Waiting frame data from RDMA for type-1 command";
		case 0x0010:
			return "Sending type-1 command";
		case 0x0020:
			return "Sending type-2 command";
		case 0x0040:
			return "Reading command queue for data";
		case 0x0080:
			return "Sending type-3 command";
		case 0x0100:
			return "Sending BTA";
		case 0x0200:
			return "Waiting RX-read data ";
		case 0x0400:
			return "Waiting SW RACK for RX-read data";
		case 0x0800:
			return "Waiting TE";
		case 0x1000:
			return "Get TE ";
		case 0x2000:
			return "Waiting external TE";
		case 0x4000:
			return "Waiting SW RACK for TE";
		default:
			return "unknown";
		}
	}

DSI_STATUS DSI_DumpRegisters(DISP_MODULE_ENUM module, int level)
{
		uint32_t i;

		if (level >= 0) {
			if (module == DISP_MODULE_DSI0 /* || module == DISP_MODULE_DSIDUAL */) {
				unsigned int DSI_DBG6_Status =
				    (INREG32(DDP_REG_BASE_DSI0 + 0x160)) & 0xffff;
				DISPDMP("DSI0 state:%s\n",
					_dsi_cmd_mode_parse_state(DSI_DBG6_Status));
				DISPDMP("DSI Mode: lane num: transfer count: status: ");
			}
#if 0
			if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL) {
				unsigned int DSI_DBG6_Status =
				    (INREG32(DSI1_BASE + 0x160)) & 0xffff;
				pr_debug("DSI1 state:%s\n",
					 _dsi_cmd_mode_parse_state(DSI_DBG6_Status));
				pr_debug("DSI Mode: lane num: transfer count: status: ");
			}
#endif
		}
		if (level >= 1) {
			if (module == DISP_MODULE_DSI0 /* || module == DISP_MODULE_DSIDUAL */) {

				DISPDMP("---------- Start dump DSI0 registers ----------\n");

				for (i = 0; i < sizeof(struct DSI_REGS); i += 16) {
					DISPDMP("DSI+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
						INREG32(DDP_REG_BASE_DSI0 + i),
						INREG32(DDP_REG_BASE_DSI0 + i + 0x4),
						INREG32(DDP_REG_BASE_DSI0 + i + 0x8),
						INREG32(DDP_REG_BASE_DSI0 + i + 0xc));
				}

				for (i = 0; i < sizeof(struct DSI_CMDQ_REGS); i += 16) {
					DISPDMP("DSI_CMD+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n",
						i, INREG32((DDP_REG_BASE_DSI0 + 0x200 + i)),
						INREG32((DDP_REG_BASE_DSI0 + 0x200 + i + 0x4)),
						INREG32((DDP_REG_BASE_DSI0 + 0x200 + i + 0x8)),
						INREG32((DDP_REG_BASE_DSI0 + 0x200 + i + 0xc)));
				}

#ifndef CONFIG_FPGA_EARLY_PORTING
				for (i = 0; i < sizeof(struct DSI_PHY_REGS); i += 16) {
					DISPDMP("DSI_PHY+%04x : 0x%08x    0x%08x  0x%08x  0x%08x\n",
						i, INREG32((MIPITX_BASE + i)),
						INREG32((MIPITX_BASE + i + 0x4)),
						INREG32((MIPITX_BASE + i + 0x8)),
						INREG32((MIPITX_BASE + i + 0xc)));
				}
#endif
			}
#if 0
			if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL) {
				unsigned int DSI_DBG6_Status =
				    (INREG32(DSI1_BASE + 0x160)) & 0xffff;

				DISPDMP("---------- Start dump DSI1 registers ----------\n");

				for (i = 0; i < sizeof(struct DSI_REGS); i += 16) {
					DISPDMP("DSI+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
						INREG32(DSI1_BASE + i),
						INREG32(DSI1_BASE + i + 0x4),
						INREG32(DSI1_BASE + i + 0x8),
						INREG32(DSI1_BASE + i + 0xc));
				}

				for (i = 0; i < sizeof(DSI_CMDQ_REGS); i += 16) {
					DISPDMP("DSI_CMD+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n",
						i, INREG32((DSI1_BASE + 0x200 + i)),
						INREG32((DSI1_BASE + 0x200 + i + 0x4)),
						INREG32((DSI1_BASE + 0x200 + i + 0x8)),
						INREG32((DSI1_BASE + 0x200 + i + 0xc)));
				}

#ifndef CONFIG_FPGA_EARLY_PORTING
				for (i = 0; i < sizeof(struct DSI_PHY_REGS); i += 16) {
					DISPDMP("DSI_PHY+%04x : 0x%08x    0x%08x  0x%08x  0x%08x\n",
						i, INREG32((MIPI_TX1_BASE + i)),
						INREG32((MIPI_TX1_BASE + i + 0x4)),
						INREG32((MIPI_TX1_BASE + i + 0x8)),
						INREG32((MIPI_TX1_BASE + i + 0xc)));
				}
#endif
			}
#endif
		}

		return DSI_STATUS_OK;
}

int DSI_WaitVMDone(DISP_MODULE_ENUM module)
{
		int i = 0;
		static const long WAIT_TIMEOUT = 2 * HZ;	/* 2 sec */
		int ret = 0;

		/*...dsi video is always in busy state... */
		if (_dsi_is_video_mode(module)) {
			DISPERR("DSI_WaitVMDone error: should set DSI to CMD mode firstly\n");
			return -1;
		}

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			ret =
			    wait_event_interruptible_timeout(_dsi_wait_vm_done_queue[i],
							     !(DSI_REG[i]->DSI_INTSTA.BUSY),
							     WAIT_TIMEOUT);
			if (0 == ret) {
				DISPERR("dsi wait VM done  timeout\n");
				DSI_DumpRegisters(module, 1);
				DSI_Reset(module, NULL);
				return -1;
			}
		}
		return 0;
	}

static void DSI_WaitForNotBusy(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
		int i = 0;
		unsigned int count = 0;
		unsigned int tmp = 0;

		static const long WAIT_TIMEOUT = 2 * HZ;	/* 2 sec */
		int ret = 0;

		if (cmdq) {
			/* for(i = DSI_MODULE_BEGIN(module);i <= DSI_MODULE_END(module);i++) */
			DSI_POLLREG32(cmdq, &DSI_REG[i]->DSI_INTSTA, 0x80000000, 0x0);

			return;
		}


		/*...dsi video is always in busy state... */
		if (_dsi_is_video_mode(module))
			return;

		/* TODO: */
#if defined(MTK_NO_DISP_IN_LK)
		i = DSI_MODULE_BEGIN(module);
		while (1) {
			tmp = INREG32(&DSI_REG[i]->DSI_INTSTA);
			if (!(tmp & 0x80000000))
				break;

			/* if(count %1000) */
			/* DISPMSG("dsi state:0x%08x, 0x%08x\n", tmp, INREG32(&DSI_REG[i]->DSI_STATE_DBG6)); */

			/* msleep(1); */

			if (count++ > 1000000000) {
				DISPERR("dsi wait not busy timeout\n");
				DSI_DumpRegisters(module, 1);
				DSI_Reset(module, NULL);
				break;
			}
		}
#else
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			ret =
			    wait_event_interruptible_timeout(_dsi_cmd_done_wait_queue[i],
							     !(DSI_REG[i]->DSI_INTSTA.BUSY),
							     WAIT_TIMEOUT);
			if (ret <= 0) {
				i = DSI_MODULE_BEGIN(module);
				while (1) {
					tmp = INREG32(&DSI_REG[i]->DSI_INTSTA);
					if (!(tmp & 0x80000000))
						break;

					if (count++ > 1000000000) {
						DISPERR("dsi wait not busy timeout\n");
						DSI_DumpRegisters(module, 1);
						DSI_Reset(module, NULL);
						break;
					}
				}
			}
		}
#endif
}

DSI_STATUS DSI_SleepOut(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
		int i = 0;
		/* wake_up_prd *1024*cycle time > 1ms */
		int wake_up_prd =
		    (_dsi_context[i].dsi_params.PLL_CLOCK * 2 * 1000) / (1024 * 8) + 0x1;
		/* TODO: can we just start dsi0 for dsi dual? */
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL,
				      SLEEP_MODE, 1);
			DSI_OUTREGBIT(cmdq, struct DSI_TIME_CON0_REG, DSI_REG[i]->DSI_TIME_CON0,
				UPLS_WAKEUP_PRD, wake_up_prd);
			/* cycle to 1ms for 520MHz */
		}

		return DSI_STATUS_OK;
}


DSI_STATUS DSI_Wakeup(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
		int i = 0;
		int ret = 0;
		int cnt = 0;
		/* TODO: can we just start dsi0 for dsi dual? */
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			wait_sleep_out_done = false;
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START, SLEEPOUT_START,
				      0);
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START, SLEEPOUT_START,
				      1);
			do {
				cnt++;
				ret =
				    wait_event_interruptible_timeout(_dsi_wait_sleep_out_done_queue
								     [i], wait_sleep_out_done,
								     2 * HZ);
			} while (ret <= 0 && cnt <= 2);

			if (ret == 0) {
				DISPERR("dsi wait sleep out timeout\n");
				DSI_DumpRegisters(module, 2);
				DSI_Reset(module, NULL);
			} else if (ret < 0) {
				DISPERR("dsi wait sleep out weake up by signal ret %d\n", ret);
				mdelay(5);
			}
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START, SLEEPOUT_START,
				      0);
			DSI_OUTREGBIT(cmdq, struct DSI_MODE_CTRL_REG, DSI_REG[i]->DSI_MODE_CTRL,
				      SLEEP_MODE, 0);
		}

		return DSI_STATUS_OK;
}

DSI_STATUS DSI_BackupRegisters(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
		int i = 0;
		volatile struct DSI_REGS *regs = NULL;

		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			regs = &(_dsi_context[i].regBackup);

			DSI_OUTREG32(cmdq, &regs->DSI_INTEN, AS_UINT32(&DSI_REG[i]->DSI_INTEN));
			DSI_OUTREG32(cmdq, &regs->DSI_MODE_CTRL,
				     AS_UINT32(&DSI_REG[i]->DSI_MODE_CTRL));
			DSI_OUTREG32(cmdq, &regs->DSI_TXRX_CTRL,
				     AS_UINT32(&DSI_REG[i]->DSI_TXRX_CTRL));
			DSI_OUTREG32(cmdq, &regs->DSI_PSCTRL, AS_UINT32(&DSI_REG[i]->DSI_PSCTRL));

			DSI_OUTREG32(cmdq, &regs->DSI_VSA_NL, AS_UINT32(&DSI_REG[i]->DSI_VSA_NL));
			DSI_OUTREG32(cmdq, &regs->DSI_VBP_NL, AS_UINT32(&DSI_REG[i]->DSI_VBP_NL));
			DSI_OUTREG32(cmdq, &regs->DSI_VFP_NL, AS_UINT32(&DSI_REG[i]->DSI_VFP_NL));
			DSI_OUTREG32(cmdq, &regs->DSI_VACT_NL, AS_UINT32(&DSI_REG[i]->DSI_VACT_NL));

			DSI_OUTREG32(cmdq, &regs->DSI_HSA_WC, AS_UINT32(&DSI_REG[i]->DSI_HSA_WC));
			DSI_OUTREG32(cmdq, &regs->DSI_HBP_WC, AS_UINT32(&DSI_REG[i]->DSI_HBP_WC));
			DSI_OUTREG32(cmdq, &regs->DSI_HFP_WC, AS_UINT32(&DSI_REG[i]->DSI_HFP_WC));
			DSI_OUTREG32(cmdq, &regs->DSI_BLLP_WC, AS_UINT32(&DSI_REG[i]->DSI_BLLP_WC));

			DSI_OUTREG32(cmdq, &regs->DSI_HSTX_CKL_WC,
				     AS_UINT32(&DSI_REG[i]->DSI_HSTX_CKL_WC));
			DSI_OUTREG32(cmdq, &regs->DSI_MEM_CONTI,
				     AS_UINT32(&DSI_REG[i]->DSI_MEM_CONTI));

			DSI_OUTREG32(cmdq, &regs->DSI_PHY_TIMECON0,
				     AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON0));
			DSI_OUTREG32(cmdq, &regs->DSI_PHY_TIMECON1,
				     AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON1));
			DSI_OUTREG32(cmdq, &regs->DSI_PHY_TIMECON2,
				     AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON2));
			DSI_OUTREG32(cmdq, &regs->DSI_PHY_TIMECON3,
				     AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON3));
			DSI_OUTREG32(cmdq, &regs->DSI_PHY_TIMECON4,
				     AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON4));
			DSI_OUTREG32(cmdq, &regs->DSI_VM_CMD_CON,
				     AS_UINT32(&DSI_REG[i]->DSI_VM_CMD_CON));
			DISPMSG("DSI_BackupRegisters VM_CMD_EN %d TS_VFP_EN %d\n",
				 DSI_REG[i]->DSI_VM_CMD_CON.VM_CMD_EN,
				 DSI_REG[i]->DSI_VM_CMD_CON.TS_VFP_EN);
		}

		return DSI_STATUS_OK;
}

DSI_STATUS DSI_RestoreRegisters(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;
	volatile struct DSI_REGS *regs = NULL;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		regs = &(_dsi_context[i].regBackup);

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_INTEN, AS_UINT32(&regs->DSI_INTEN));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_MODE_CTRL,
			     AS_UINT32(&regs->DSI_MODE_CTRL));
		/* can not restore lane_num here */
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_TXRX_CTRL,
			     AS_UINT32(&regs->DSI_TXRX_CTRL) & 0xFFFFFFC3);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PSCTRL, AS_UINT32(&regs->DSI_PSCTRL));

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VSA_NL, AS_UINT32(&regs->DSI_VSA_NL));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VBP_NL, AS_UINT32(&regs->DSI_VBP_NL));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VFP_NL, AS_UINT32(&regs->DSI_VFP_NL));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VACT_NL, AS_UINT32(&regs->DSI_VACT_NL));

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSA_WC, AS_UINT32(&regs->DSI_HSA_WC));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HBP_WC, AS_UINT32(&regs->DSI_HBP_WC));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HFP_WC, AS_UINT32(&regs->DSI_HFP_WC));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_BLLP_WC, AS_UINT32(&regs->DSI_BLLP_WC));

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSTX_CKL_WC,
			     AS_UINT32(&regs->DSI_HSTX_CKL_WC));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_MEM_CONTI,
			     AS_UINT32(&regs->DSI_MEM_CONTI));

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON0,
			     AS_UINT32(&regs->DSI_PHY_TIMECON0));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON1,
			     AS_UINT32(&regs->DSI_PHY_TIMECON1));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON2,
			     AS_UINT32(&regs->DSI_PHY_TIMECON2));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON3,
			     AS_UINT32(&regs->DSI_PHY_TIMECON3));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON4,
			     AS_UINT32(&regs->DSI_PHY_TIMECON4));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_CON,
			     AS_UINT32(&regs->DSI_VM_CMD_CON));
		DISPMSG("DSI_RestoreRegisters VM_CMD_EN %d TS_VFP_EN %d\n",
			 regs->DSI_VM_CMD_CON.VM_CMD_EN, regs->DSI_VM_CMD_CON.TS_VFP_EN);
	}
	return DSI_STATUS_OK;
}

DSI_STATUS DSI_BIST_Pattern_Test(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enable,
					 unsigned int color)
{
	int i = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (enable) {
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_BIST_PATTERN, color);
			/* DSI_OUTREG32(&DSI_REG->DSI_BIST_CON, AS_UINT32(&temp_reg)); */
			/* DSI_OUTREGBIT(struct DSI_BIST_CON_REG, DSI_REG->DSI_BIST_CON, SELF_PAT_MODE, 1); */
			DSI_OUTREGBIT(cmdq, struct DSI_BIST_CON_REG, DSI_REG[i]->DSI_BIST_CON,
				      SELF_PAT_MODE, 1);

			if (!_dsi_is_video_mode(module)) {
				DSI_T0_INS t0;

				t0.CONFG = 0x09;
				t0.Data_ID = 0x39;
				t0.Data0 = 0x2c;
				t0.Data1 = 0;

				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[i]->data[0],
					     AS_UINT32(&t0));
				DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_CMDQ_SIZE, 1);

				/* DSI_OUTREGBIT(struct DSI_START_REG,DSI_REG->DSI_START,DSI_START,0); */
				DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_START, 0);
				DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_START, 1);
				/* DSI_OUTREGBIT(struct DSI_START_REG,DSI_REG->DSI_START,DSI_START,1); */
			}
		} else {
			/* if disable dsi pattern, need enable mutex, can't just start dsi */
			/* so we just disable pattern bit, do not start dsi here */
			/* DSI_WaitForNotBusy(module,cmdq); */
			/* DSI_OUTREGBIT(cmdq, struct DSI_BIST_CON_REG, DSI_REG[i]->DSI_BIST_CON, SELF_PAT_MODE, 0); */
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_BIST_CON, 0x00);
		}

	}
	return 0;
}

void DSI_Config_VDO_Timing(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
				   LCM_DSI_PARAMS *dsi_params)
{
	int i = 0;
	unsigned int line_byte;
	unsigned int horizontal_sync_active_byte;
	unsigned int horizontal_backporch_byte;
	unsigned int horizontal_frontporch_byte;
	unsigned int horizontal_bllp_byte;
	unsigned int dsiTmpBufBpp;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (dsi_params->data_format.format == LCM_DSI_FORMAT_RGB565)
			dsiTmpBufBpp = 2;
		else
			dsiTmpBufBpp = 3;


		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VSA_NL,
			     dsi_params->vertical_sync_active);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VBP_NL, dsi_params->vertical_backporch);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VFP_NL,
			     dsi_params->vertical_frontporch);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VACT_NL,
			     dsi_params->vertical_active_line);

		line_byte =
		    (dsi_params->horizontal_sync_active + dsi_params->horizontal_backporch +
		     dsi_params->horizontal_frontporch +
		     dsi_params->horizontal_active_pixel) * dsiTmpBufBpp;
		horizontal_sync_active_byte =
		    (dsi_params->horizontal_sync_active * dsiTmpBufBpp - 4);

		if (dsi_params->mode == SYNC_EVENT_VDO_MODE
		    || dsi_params->mode == BURST_VDO_MODE
		    || dsi_params->switch_mode == SYNC_EVENT_VDO_MODE
		    || dsi_params->switch_mode == BURST_VDO_MODE) {
			ASSERT((dsi_params->horizontal_backporch +
				dsi_params->horizontal_sync_active) * dsiTmpBufBpp > 9);
			horizontal_backporch_byte =
			    ((dsi_params->horizontal_backporch +
			      dsi_params->horizontal_sync_active) * dsiTmpBufBpp - 10);
		} else {
			ASSERT(dsi_params->horizontal_sync_active * dsiTmpBufBpp > 9);
			horizontal_sync_active_byte =
			    (dsi_params->horizontal_sync_active * dsiTmpBufBpp - 10);

			ASSERT(dsi_params->horizontal_backporch * dsiTmpBufBpp > 9);
			horizontal_backporch_byte =
			    (dsi_params->horizontal_backporch * dsiTmpBufBpp - 10);
		}

		ASSERT(dsi_params->horizontal_frontporch * dsiTmpBufBpp > 11);
		horizontal_frontporch_byte =
		    (dsi_params->horizontal_frontporch * dsiTmpBufBpp - 12);
		horizontal_bllp_byte = (dsi_params->horizontal_bllp * dsiTmpBufBpp);

		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSA_WC,
			     ALIGN_TO((horizontal_sync_active_byte), 4));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HBP_WC,
			     ALIGN_TO((horizontal_backporch_byte), 4));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HFP_WC,
			     ALIGN_TO((horizontal_frontporch_byte), 4));
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_BLLP_WC,
			     ALIGN_TO((horizontal_bllp_byte), 4));
	}
}

void DSI_Set_LFR(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned int mode,
			 unsigned int type, unsigned int enable, unsigned int skip_num)
{
	/* LFR_MODE 0 disable,1 static mode ,2 dynamic mode 3,both */
	unsigned int i = 0;

	DISPMSG("module=%d,mode=%d,type=%d,enable=%d,skip_num=%d\n", module, mode, type,
		 enable, skip_num);
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_MODE,
			      mode);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_TYPE, 0);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_UPDATE,
			      1);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_VSE_DIS,
			      0);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_SKIP_NUM,
			      skip_num);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_EN,
			      enable);
	}
}

void DSI_LFR_UPDATE(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	unsigned int i = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_UPDATE,
			      0);
		DSI_OUTREGBIT(cmdq, struct DSI_LFR_CON_REG, DSI_REG[i]->DSI_LFR_CON, LFR_UPDATE,
			      1);
		DISPMSG("DSI_LFR i %d\n", i);
	}
}

int _dsi_ps_type_to_bpp(LCM_PS_TYPE ps)
{
	switch (ps) {
	case LCM_PACKED_PS_16BIT_RGB565:
		return 2;
	case LCM_LOOSELY_PS_18BIT_RGB666:
		return 3;
	case LCM_PACKED_PS_24BIT_RGB888:
		return 3;
	case LCM_PACKED_PS_18BIT_RGB666:
		return 3;
	}
	/*can't reach here */
	ASSERT(0);
	return -1;
}

DSI_STATUS DSI_PS_Control(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
				  LCM_DSI_PARAMS *dsi_params, int w, int h)
{
	int i = 0;
	unsigned int ps_sel_bitvalue = 0;
	/* TODO: parameter checking */
	ASSERT(_dsi_ps_type_to_bpp(dsi_params->PS) <= PACKED_PS_18BIT_RGB666);

	if (_dsi_ps_type_to_bpp(dsi_params->PS) > LOOSELY_PS_18BIT_RGB666)
		ps_sel_bitvalue = (5 - dsi_params->PS);
	else
		ps_sel_bitvalue = dsi_params->PS;

#if 0
	if (module == DISP_MODULE_DSIDUAL)
		w = w / 2;
#endif
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_VACT_NL_REG, DSI_REG[i]->DSI_VACT_NL, VACT_NL, h);
		if (dsi_params->ufoe_enable && dsi_params->ufoe_params.lr_mode_en != 1) {
			if (dsi_params->ufoe_params.compress_ratio == 3) {	/* 1/3 */
				unsigned int ufoe_internal_width = w + w % 4;

				if (ufoe_internal_width % 3 == 0) {
					DSI_OUTREGBIT(cmdq, struct DSI_PSCTRL_REG,
						      DSI_REG[i]->DSI_PSCTRL, DSI_PS_WC,
						      (ufoe_internal_width / 3) *
						      _dsi_ps_type_to_bpp(dsi_params->PS));
				} else {
					unsigned int temp_w = ufoe_internal_width / 3 + 1;

					temp_w =
					    ((temp_w % 2) == 1) ? (temp_w + 1) : temp_w;
					DSI_OUTREGBIT(cmdq, struct DSI_PSCTRL_REG,
						      DSI_REG[i]->DSI_PSCTRL, DSI_PS_WC,
						      temp_w *
						      _dsi_ps_type_to_bpp(dsi_params->PS));
				}
			} else	/* 1/2 */
				DSI_OUTREGBIT(cmdq, struct DSI_PSCTRL_REG, DSI_REG[i]->DSI_PSCTRL,
					      DSI_PS_WC,
					      (w +
					       w % 4) / 2 *
					      _dsi_ps_type_to_bpp(dsi_params->PS));
		} else {
			DSI_OUTREGBIT(cmdq, struct DSI_PSCTRL_REG, DSI_REG[i]->DSI_PSCTRL,
				      DSI_PS_WC, w * _dsi_ps_type_to_bpp(dsi_params->PS));
		}

		DSI_OUTREGBIT(cmdq, struct DSI_PSCTRL_REG, DSI_REG[i]->DSI_PSCTRL, DSI_PS_SEL,
			      ps_sel_bitvalue);
	}

	return DSI_STATUS_OK;
}

DSI_STATUS DSI_TXRX_Control(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
				    LCM_DSI_PARAMS *dsi_params)
{
	int i = 0;
	unsigned int lane_num_bitvalue = 0;
	/*bool cksm_en = true; */
	/*bool ecc_en = true; */
	int lane_num = dsi_params->LANE_NUM;
	int vc_num = 0;
	bool null_packet_en = false;
	/*bool err_correction_en = false; */
	bool dis_eotp_en = false;
	bool hstx_cklp_en = false;
	int max_return_size = 0;

	switch (lane_num) {
	case LCM_ONE_LANE:
		lane_num_bitvalue = 0x1;
		break;
	case LCM_TWO_LANE:
		lane_num_bitvalue = 0x3;
		break;
	case LCM_THREE_LANE:
		lane_num_bitvalue = 0x7;
		break;
	case LCM_FOUR_LANE:
		lane_num_bitvalue = 0xF;
		break;
	}

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL, VC_NUM,
			      vc_num);
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL, DIS_EOT,
			      dis_eotp_en);
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL, BLLP_EN,
			      null_packet_en);
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL,
			      MAX_RTN_SIZE, max_return_size);
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL,
			      HSTX_CKLP_EN, hstx_cklp_en);
		DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL, LANE_NUM,
			      lane_num_bitvalue);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_MEM_CONTI, DSI_WMEM_CONTI);
		if (CMD_MODE == dsi_params->mode) {
			if (dsi_params->ext_te_edge == LCM_POLARITY_FALLING) {
				/*use ext te falling edge */
				DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL,
					      EXT_TE_EDGE, 1);
			}
			DSI_OUTREGBIT(cmdq, struct DSI_TXRX_CTRL_REG, DSI_REG[i]->DSI_TXRX_CTRL, EXT_TE_EN,
				      1);
		}
	}

	return DSI_STATUS_OK;
}

int MIPITX_IsEnabled(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;
	int ret = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0.RG_DSI0_MPPLL_PLL_EN)
			ret++;
	}

	/* DISPMSG("MIPITX for %s is %s\n", ddp_get_module_name(module), ret?"on":"off"); */
	return ret;
}

void DSI_PHY_clk_setting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
				 LCM_DSI_PARAMS *dsi_params)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
#if 0
	MIPITX_Write60384(0x18, 0x00, 0x10);
	MIPITX_Write60384(0x20, 0x42, 0x01);
	MIPITX_Write60384(0x20, 0x43, 0x01);
	MIPITX_Write60384(0x20, 0x05, 0x01);
	MIPITX_Write60384(0x20, 0x22, 0x01);
	MIPITX_Write60384(0x30, 0x44, 0x83);
	MIPITX_Write60384(0x30, 0x40, 0x82);
	MIPITX_Write60384(0x30, 0x00, 0x03);
	MIPITX_Write60384(0x30, 0x68, 0x03);
	MIPITX_Write60384(0x30, 0x68, 0x01);
	MIPITX_Write60384(0x30, 0x50, 0x80);
	MIPITX_Write60384(0x30, 0x51, 0x01);
	MIPITX_Write60384(0x30, 0x54, 0x01);
	MIPITX_Write60384(0x30, 0x58, 0x00);
	MIPITX_Write60384(0x30, 0x59, 0x00);
	MIPITX_Write60384(0x30, 0x5a, 0x00);
	MIPITX_Write60384(0x30, 0x5b, (dsi_params->fbk_div) << 2);
	MIPITX_Write60384(0x30, 0x04, 0x11);
	MIPITX_Write60384(0x30, 0x08, 0x01);
	MIPITX_Write60384(0x30, 0x0C, 0x01);
	MIPITX_Write60384(0x30, 0x10, 0x01);
	MIPITX_Write60384(0x30, 0x14, 0x01);
	MIPITX_Write60384(0x30, 0x64, 0x20);
	MIPITX_Write60384(0x30, 0x50, 0x81);
	MIPITX_Write60384(0x30, 0x28, 0x00);
	DSI_OUTREG32(cmdq, DSI_REG[0] + 0x10, 0x5);
	DSI_OUTREG32(cmdq, DSI_REG[0] + 0x10, 0x0);
#endif
#else
#if 0
	MIPITX_OUTREG32(0x10215044, 0x88492483);
	MIPITX_OUTREG32(0x10215040, 0x00000002);
	mdelay(10);
	MIPITX_OUTREG32(0x10215000, 0x00000403);
	MIPITX_OUTREG32(0x10215068, 0x00000003);
	MIPITX_OUTREG32(0x10215068, 0x00000001);

	mdelay(10);
	MIPITX_OUTREG32(0x10215050, 0x00000000);
	mdelay(10);
	MIPITX_OUTREG32(0x10215054, 0x00000003);
	MIPITX_OUTREG32(0x10215058, 0x60000000);
	MIPITX_OUTREG32(0x1021505c, 0x00000000);

	MIPITX_OUTREG32(0x10215004, 0x00000803);
	MIPITX_OUTREG32(0x10215008, 0x00000801);
	MIPITX_OUTREG32(0x1021500c, 0x00000801);
	MIPITX_OUTREG32(0x10215010, 0x00000801);
	MIPITX_OUTREG32(0x10215014, 0x00000801);

	MIPITX_OUTREG32(0x10215050, 0x00000001);

	mdelay(10);


	MIPITX_OUTREG32(0x10215064, 0x00000020);
	return 0;
#endif

	int i = 0;
	unsigned int data_Rate = dsi_params->PLL_CLOCK * 2;
	unsigned int txdiv = 0;
	unsigned int txdiv0 = 0;
	unsigned int txdiv1 = 0;
	unsigned int pcw = 0;
/* unsigned int fmod = 30; Fmod = 30KHz by default */
	unsigned int delta1 = 5;	/* Delta1 is SSC range, default is 0%~-5% */
	unsigned int pdelta1 = 0;

	/* temp1~5 is used for impedence calibration, not enable now */
#if 0
	u32 m_hw_res3 = 0;
	u32 temp1 = 0;
	u32 temp2 = 0;
	u32 temp3 = 0;
	u32 temp4 = 0;
	u32 temp5 = 0;

	m_hw_res3 = INREG32(0xF0206180);
	temp1 = (m_hw_res3 >> 28) & 0xF;
	temp2 = (m_hw_res3 >> 24) & 0xF;
	temp3 = (m_hw_res3 >> 20) & 0xF;
	temp4 = (m_hw_res3 >> 16) & 0xF;
	temp5 = (m_hw_res3 >> 12) & 0xF;
#endif

	DISPFUNC();
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		/* check data_Rate bound*/
		if (data_Rate > 1250 || data_Rate < 50) {
			DISPERR("invalid mipitx Data Rate: %d\n", data_Rate);
			break;
		}
		if (0 != dsi_params->ssc_range)
			delta1 = dsi_params->ssc_range;
		if (delta1 > 8) {
			DISPERR("invalid ssc_range: %d\n", delta1);
			break;
		}
		/* step 1 */
		/* MIPITX_MASKREG32(APMIXED_BASE+0x00, (0x1<<6), 1); */

		/* step 2 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_BG_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_BG_CON,
				 RG_DSI_BG_CORE_EN, 1);
		MIPITX_OUTREGBIT(struct MIPITX_DSI_BG_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_BG_CON,
				 RG_DSI_BG_CKEN, 1);

		/* step 3 */
		mdelay(1);

		/* step 4 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_TOP_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_TOP_CON,
				 RG_DSI_LNT_HS_BIAS_EN, 1);

		/* step 5 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_CON,
				 RG_DSI_CKG_LDOOUT_EN, 1);
		MIPITX_OUTREGBIT(struct MIPITX_DSI_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_CON,
				 RG_DSI_LDOCORE_EN, 1);

		/* step 6 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_PWR,
				 DA_DSI_MPPLL_SDM_PWR_ON, 1);

		/* step 7 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_PWR,
				 DA_DSI_MPPLL_SDM_ISO_EN, 0);
		mdelay(1);

		if (0 != data_Rate) {
			if (data_Rate >= 500) { /* < 1250 */
				txdiv = 1;
				txdiv0 = 0;
				txdiv1 = 0;
			} else if (data_Rate >= 250) {
				txdiv = 2;
				txdiv0 = 1;
				txdiv1 = 0;
			} else if (data_Rate >= 125) {
				txdiv = 4;
				txdiv0 = 2;
				txdiv1 = 0;
			} else if (data_Rate > 62) {
				txdiv = 8;
				txdiv0 = 2;
				txdiv1 = 1;
			} else { /* >=50 */
				txdiv = 16;
				txdiv0 = 2;
				txdiv1 = 2;
			}
			/* step 8 */
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_TXDIV0, txdiv0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_TXDIV1, txdiv1);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_PREDIV, 0);


			/* step 10 */
			/* PLL PCW config */
			/*
			   PCW bit 24~30 = floor(pcw)
			   PCW bit 16~23 = (pcw - floor(pcw))*256
			   PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256
			   PCW bit 8~15 = (pcw*256*256 - floor(pcw)*256*256)*256
			 */
			/* pcw = data_Rate*4*txdiv/(26*2); Post DIV =4, so need data_Rate*4 */
			pcw = data_Rate * txdiv / 13;

			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2,
					 RG_DSI0_MPPLL_SDM_PCW_H, (pcw & 0x7F));
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2,
					 RG_DSI0_MPPLL_SDM_PCW_16_23,
					 ((256 * (data_Rate * txdiv % 13) / 13) & 0xFF));
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2,
					 RG_DSI0_MPPLL_SDM_PCW_8_15,
					 ((256 * (256 * (data_Rate * txdiv % 13) % 13) / 13) & 0xFF));
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON2_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2,
					 RG_DSI0_MPPLL_SDM_PCW_0_7,
					 ((256 * (256 * (256 * (data_Rate * txdiv % 13) % 13) % 13) / 13) & 0xFF));

			if (1 != dsi_params->ssc_disable) {
				MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1,
					RG_DSI0_MPPLL_SDM_SSC_PH_INIT, 1);
				MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1,
					RG_DSI0_MPPLL_SDM_SSC_PRD, 0x1B1);
				/* PRD=ROUND(pmod) = 433; */

				pdelta1 = (delta1 * data_Rate * txdiv * 262144 + 281664) / 563329;
				MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON3_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON3,
						 RG_DSI0_MPPLL_SDM_SSC_DELTA, pdelta1);
				MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON3_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON3,
						 RG_DSI0_MPPLL_SDM_SSC_DELTA1, pdelta1);
				/* DSI_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG->MIPITX_DSI_PLL_CON1,
						RG_DSI0_MPPLL_SDM_FRA_EN,1); */
				DISPMSG(
					"[dsi_drv.c] PLL config:data_rate=%d,txdiv=%d,pcw=%d,delta1=%d,pdelta1=0x%x\n",
				     data_Rate, txdiv, DSI_INREG32((struct MIPITX_DSI_PLL_CON2_REG *),
						&DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2), delta1, pdelta1);
			}
		} else {
			DISPERR("[dsi_dsi.c] PLL clock should not be 0!!!\n");
		}

		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1,
				 RG_DSI0_MPPLL_SDM_FRA_EN, 1);

		/* step 11 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_CLOCK_LANE_REG, DSI_PHY_REG[i]->MIPITX_DSI_CLOCK_LANE,
				 RG_DSI_LNTC_LDOOUT_EN, 1);

		/* step 12 */
		if (dsi_params->LANE_NUM > 0) {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE0_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE0,
					 RG_DSI_LNT0_LDOOUT_EN, 1);
		}
		/* step 13 */
		if (dsi_params->LANE_NUM > 1) {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE1_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE1,
					 RG_DSI_LNT1_LDOOUT_EN, 1);
		}
		/* step 14 */
		if (dsi_params->LANE_NUM > 2) {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE2_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE2,
					 RG_DSI_LNT2_LDOOUT_EN, 1);
		}
		/* step 15 */
		if (dsi_params->LANE_NUM > 3) {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE3_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE3,
					 RG_DSI_LNT3_LDOOUT_EN, 1);
		}
		/* step 16 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_PLL_EN, 1);

		/* step 17 */
		mdelay(1);

		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CHG_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CHG,
				 RG_DSI0_MPPLL_SDM_PCW_CHG, 0);
		MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CHG_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CHG,
				 RG_DSI0_MPPLL_SDM_PCW_CHG, 1);

		if ((0 != data_Rate) && (1 != dsi_params->ssc_disable)) {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1,
					 RG_DSI0_MPPLL_SDM_SSC_EN, 1);
		} else {
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON1_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1,
					 RG_DSI0_MPPLL_SDM_SSC_EN, 0);
		}

		/* step 18 */
		MIPITX_OUTREGBIT(struct MIPITX_DSI_TOP_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_TOP_CON,
				 RG_DSI_PAD_TIE_LOW_EN, 0);

		mdelay(1);
	}
#endif
}



void DSI_PHY_TIMCONFIG(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
			       LCM_DSI_PARAMS *dsi_params)
{
	int i = 0;
#if 0
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON0, 0x140f0708);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON1, 0x10280c20);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON2, 0x14280000);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON3, 0x00101a06);
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_TIMECON4, 0x00023000);
	}
	return;
#endif

	struct DSI_PHY_TIMCON0_REG timcon0;
	struct DSI_PHY_TIMCON1_REG timcon1;
	struct DSI_PHY_TIMCON2_REG timcon2;
	struct DSI_PHY_TIMCON3_REG timcon3;
/*	unsigned int div1 = 0;*/
/*	unsigned int div2 = 0;*/
/*	unsigned int pre_div = 0;*/
/*	unsigned int post_div = 0;*/
/*	unsigned int fbk_sel = 0;*/
/*	unsigned int fbk_div = 0;*/
	unsigned int lane_no = dsi_params->LANE_NUM;

	/* unsigned int div2_real; */
	unsigned int cycle_time;
	unsigned int ui;
	unsigned int hs_trail_m, hs_trail_n;

	if (0 != dsi_params->PLL_CLOCK) {
		ui = 1000 / (dsi_params->PLL_CLOCK * 2) + 0x01;
		cycle_time = 8000 / (dsi_params->PLL_CLOCK * 2) + 0x01;
		DISPMSG("DISP/DSI DSI_PHY_TIMCONFIG, Cycle Time = %d(ns), Unit Interval = %d(ns), lane# = %d\n",
			 cycle_time, ui, lane_no);
	} else {
		DISPERR("[dsi_dsi.c] PLL clock should not be 0!!!\n");
		ASSERT(0);
		return;
	}

	/* div2_real=div2 ? div2*0x02 : 0x1; */
	/* cycle_time = (1000 * div2 * div1 * pre_div * post_div)/ (fbk_sel * (fbk_div+0x01) * 26) + 1; */
	/* ui = (1000 * div2 * div1 * pre_div * post_div)/ (fbk_sel * (fbk_div+0x01) * 26 * 2) + 1; */
#define NS_TO_CYCLE(n, c)	((n) / (c))

	hs_trail_m = 1;
	hs_trail_n =
	    (dsi_params->HS_TRAIL == 0) ? NS_TO_CYCLE(((hs_trail_m * 0x4 * ui) + 0x50),
						      cycle_time) : dsi_params->HS_TRAIL;
	/* +3 is recommended from designer becauase of HW latency */
	timcon0.HS_TRAIL = (hs_trail_m > hs_trail_n) ? hs_trail_m : hs_trail_n;

	timcon0.HS_PRPR =
	    (dsi_params->HS_PRPR == 0) ? NS_TO_CYCLE((0x40 + 0x5 * ui),
						     cycle_time) : dsi_params->HS_PRPR;
	/* HS_PRPR can't be 1. */
	if (timcon0.HS_PRPR < 1)
		timcon0.HS_PRPR = 1;

	timcon0.HS_ZERO =
	    (dsi_params->HS_ZERO == 0) ? NS_TO_CYCLE((0xC8 + 0x0a * ui),
						     cycle_time) : dsi_params->HS_ZERO;
	if (timcon0.HS_ZERO > timcon0.HS_PRPR)
		timcon0.HS_ZERO -= timcon0.HS_PRPR;

	timcon0.LPX =
	    (dsi_params->LPX == 0) ? NS_TO_CYCLE(0x50, cycle_time) : dsi_params->LPX;
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (timcon0.LPX < 1)
		timcon0.LPX = 1;
#else
	/* for FPGA early porting, perhaps CPU is not engouh fast to simulcate waveform on FPGA...
	 * and see statement:
	 *     timcon1.TA_SURE  = (dsi_params->TA_SURE == 0) ? (0x3 * timcon0.LPX / 0x2) : dsi_params->TA_SURE;
	 *
	 * if timcon0.LPX is 1 and dsi_params->TA_SURE is 0, timcon1.TA_SURE will be 0 too!
	 *
	 */
	if (timcon0.LPX < 2)
		timcon0.LPX = 2;
#endif

	/* timcon1.TA_SACK         = (dsi_params->TA_SACK == 0) ? 1 : dsi_params->TA_SACK; */
	timcon1.TA_GET =
	    (dsi_params->TA_GET == 0) ? (0x5 * timcon0.LPX) : dsi_params->TA_GET;
	timcon1.TA_SURE =
	    (dsi_params->TA_SURE == 0) ? (0x3 * timcon0.LPX / 0x2) : dsi_params->TA_SURE;
	timcon1.TA_GO = (dsi_params->TA_GO == 0) ? (0x4 * timcon0.LPX) : dsi_params->TA_GO;
	/* -------------------------------------------------------------- */
	/* NT35510 need fine tune timing */
	/* Data_hs_exit = 60 ns + 128UI */
	/* Clk_post = 60 ns + 128 UI. */
	/* -------------------------------------------------------------- */
	timcon1.DA_HS_EXIT =
	    (dsi_params->DA_HS_EXIT == 0) ? (0x2 * timcon0.LPX) : dsi_params->DA_HS_EXIT;

	timcon2.CLK_TRAIL =
	    ((dsi_params->CLK_TRAIL == 0) ? NS_TO_CYCLE(0x60, cycle_time) : dsi_params->CLK_TRAIL) + 0x01;
	/* CLK_TRAIL can't be 1. */
	if (timcon2.CLK_TRAIL < 2)
		timcon2.CLK_TRAIL = 2;

	/* timcon2.LPX_WAIT        = (dsi_params->LPX_WAIT == 0) ? 1 : dsi_params->LPX_WAIT; */
	timcon2.CONT_DET = dsi_params->CONT_DET;
	timcon2.CLK_ZERO =
	    (dsi_params->CLK_ZERO == 0) ? NS_TO_CYCLE(0x190, cycle_time) : dsi_params->CLK_ZERO;

	timcon3.CLK_HS_PRPR =
	    (dsi_params->CLK_HS_PRPR == 0) ? NS_TO_CYCLE(0x40, cycle_time) : dsi_params->CLK_HS_PRPR;
	if (timcon3.CLK_HS_PRPR < 1)
		timcon3.CLK_HS_PRPR = 1;
	timcon3.CLK_HS_EXIT =
	    (dsi_params->CLK_HS_EXIT == 0) ? (0x2 * timcon0.LPX) : dsi_params->CLK_HS_EXIT;
	timcon3.CLK_HS_POST =
	    (dsi_params->CLK_HS_POST == 0) ? NS_TO_CYCLE((0x60 + 0x34 * ui), cycle_time) : dsi_params->CLK_HS_POST;

	DISPMSG("DSI_PHY_TIMCONFIG, HS_TRAIL = %d, HS_ZERO = %d, HS_PRPR = %d, LPX = %d, TA_GET = %d\n",
		 timcon0.HS_TRAIL, timcon0.HS_ZERO, timcon0.HS_PRPR, timcon0.LPX, timcon1.TA_GET);
	DISPMSG("TA_SURE = %d, TA_GO = %d, CLK_TRAIL = %d, CLK_ZERO = %d, CLK_HS_PRPR = %d\n",
		 timcon1.TA_SURE, timcon1.TA_GO, timcon2.CLK_TRAIL, timcon2.CLK_ZERO, timcon3.CLK_HS_PRPR);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON0_REG, DSI_REG[i]->DSI_PHY_TIMECON0, LPX,
			      timcon0.LPX);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON0_REG, DSI_REG[i]->DSI_PHY_TIMECON0,
			      HS_PRPR, timcon0.HS_PRPR);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON0_REG, DSI_REG[i]->DSI_PHY_TIMECON0,
			      HS_ZERO, timcon0.HS_ZERO);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON0_REG, DSI_REG[i]->DSI_PHY_TIMECON0,
			      HS_TRAIL, timcon0.HS_TRAIL);

		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON1_REG, DSI_REG[i]->DSI_PHY_TIMECON1,
			      TA_GO, timcon1.TA_GO);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON1_REG, DSI_REG[i]->DSI_PHY_TIMECON1,
			      TA_SURE, timcon1.TA_SURE);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON1_REG, DSI_REG[i]->DSI_PHY_TIMECON1,
			      TA_GET, timcon1.TA_GET);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON1_REG, DSI_REG[i]->DSI_PHY_TIMECON1,
			      DA_HS_EXIT, timcon1.DA_HS_EXIT);

		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON2_REG, DSI_REG[i]->DSI_PHY_TIMECON2,
			      CONT_DET, timcon2.CONT_DET);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON2_REG, DSI_REG[i]->DSI_PHY_TIMECON2,
			      CLK_ZERO, timcon2.CLK_ZERO);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON2_REG, DSI_REG[i]->DSI_PHY_TIMECON2,
			      CLK_TRAIL, timcon2.CLK_TRAIL);

		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON3_REG, DSI_REG[i]->DSI_PHY_TIMECON3,
			      CLK_HS_PRPR, timcon3.CLK_HS_PRPR);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON3_REG, DSI_REG[i]->DSI_PHY_TIMECON3,
			      CLK_HS_POST, timcon3.CLK_HS_POST);
		DSI_OUTREGBIT(cmdq, struct DSI_PHY_TIMCON3_REG, DSI_REG[i]->DSI_PHY_TIMECON3,
			      CLK_HS_EXIT, timcon3.CLK_HS_EXIT);
		DISPMSG("%s, 0x%08x,0x%08x,0x%08x,0x%08x\n", __func__,
			  INREG32(&DSI_REG[i]->DSI_PHY_TIMECON0),
			  INREG32(&DSI_REG[i]->DSI_PHY_TIMECON1),
			  INREG32(&DSI_REG[i]->DSI_PHY_TIMECON2),
			  INREG32(&DSI_REG[i]->DSI_PHY_TIMECON3));
	}
}


void DSI_PHY_clk_switch(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, int on)
{
	int i = 0;

	/* can't use cmdq for this */
	ASSERT(cmdq == NULL);

	if (on) {
		DSI_PHY_clk_setting(module, cmdq, &(_dsi_context[i].dsi_params));
	} else {
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			/* disable mipi clock */
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_PLL_EN, 0);
			mdelay(1);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_TOP_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_TOP_CON,
					 RG_DSI_PAD_TIE_LOW_EN, 1);


			MIPITX_OUTREGBIT(struct MIPITX_DSI_CLOCK_LANE_REG, DSI_PHY_REG[i]->MIPITX_DSI_CLOCK_LANE,
					 RG_DSI_LNTC_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE0_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE0,
					 RG_DSI_LNT0_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE1_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE1,
					 RG_DSI_LNT1_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE2_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE2,
					 RG_DSI_LNT2_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_DATA_LANE3_REG, DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE3,
					 RG_DSI_LNT3_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_PWR,
					 DA_DSI_MPPLL_SDM_ISO_EN, 1);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_PWR_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_PWR,
					 DA_DSI_MPPLL_SDM_PWR_ON, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_TOP_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_TOP_CON,
					 RG_DSI_LNT_HS_BIAS_EN, 0);

			MIPITX_OUTREGBIT(struct MIPITX_DSI_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_CON,
					 RG_DSI_CKG_LDOOUT_EN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_CON,
					 RG_DSI_LDOCORE_EN, 0);

			MIPITX_OUTREGBIT(struct MIPITX_DSI_BG_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_BG_CON,
					 RG_DSI_BG_CKEN, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_BG_CON_REG, DSI_PHY_REG[i]->MIPITX_DSI_BG_CON,
					 RG_DSI_BG_CORE_EN, 0);

			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_PREDIV, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_TXDIV0, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_TXDIV1, 0);
			MIPITX_OUTREGBIT(struct MIPITX_DSI_PLL_CON0_REG, DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON0,
					 RG_DSI0_MPPLL_POSDIV, 0);


			MIPITX_OUTREG32(&DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON1, 0x00000000);
			MIPITX_OUTREG32(&DSI_PHY_REG[i]->MIPITX_DSI_PLL_CON2, 0x50000000);
			mdelay(1);
		}
	}
}

DSI_STATUS DSI_EnableClk(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
#if 0
	DISPFUNC();
	int i = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
		DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[i]->DSI_COM_CTRL, DSI_EN, 1);
#endif
	return DSI_STATUS_OK;
}

DSI_STATUS DSI_Start(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
#if 0
	int i = 0;

	if (module != DISP_MODULE_DSIDUAL) {
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START, DSI_START,
				      0);
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START, DSI_START,
				      1);
		}
	} else
#endif
	{
		/* TODO: do we need this? */
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[0]->DSI_START, DSI_START, 0);
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[0]->DSI_START, DSI_START, 1);
	}
	return DSI_STATUS_OK;
}

void DSI_Set_VM_CMD(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;

	if (module != DISP_MODULE_DSIDUAL) {
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			DSI_OUTREGBIT(cmdq, struct DSI_VM_CMD_CON_REG, DSI_REG[i]->DSI_VM_CMD_CON,
				      TS_VFP_EN, 1);
			DSI_OUTREGBIT(cmdq, struct DSI_VM_CMD_CON_REG, DSI_REG[i]->DSI_VM_CMD_CON,
				      VM_CMD_EN, 1);
			DISPMSG("DSI_Set_VM_CMD");
		}
	} else {
		DSI_OUTREGBIT(cmdq, struct DSI_VM_CMD_CON_REG, DSI_REG[i]->DSI_VM_CMD_CON,
			      TS_VFP_EN, 1);
		DSI_OUTREGBIT(cmdq, struct DSI_VM_CMD_CON_REG, DSI_REG[i]->DSI_VM_CMD_CON,
			      VM_CMD_EN, 1);
	}
}

DSI_STATUS DSI_EnableVM_CMD(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	int i = 0;

	if (module != DISP_MODULE_DSIDUAL) {
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START,
				      VM_CMD_START, 0);
			DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[i]->DSI_START,
				      VM_CMD_START, 1);
		}
	} else {
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[0]->DSI_START, VM_CMD_START, 0);
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[0]->DSI_START, VM_CMD_START, 1);
	}
	return DSI_STATUS_OK;
}

/* / return value: the data length we got */
uint32_t DSI_dcs_read_lcm_reg_v2(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, uint8_t cmd,
			       uint8_t *buffer, uint8_t buffer_size)
{
	int d = 0;
	uint32_t max_try_count = 5;
	uint32_t recv_data_cnt = 0;
	unsigned char packet_type;
	struct DSI_RX_DATA_REG read_data0;
	struct DSI_RX_DATA_REG read_data1;
	struct DSI_RX_DATA_REG read_data2;
	struct DSI_RX_DATA_REG read_data3;
	DSI_T0_INS t0;
	static const long WAIT_TIMEOUT = 2 * HZ;	/* 2 sec */
	long ret;
	int timeout = 0;

	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++) {
		if (DSI_REG[d]->DSI_MODE_CTRL.MODE)
			return 0;


		if (buffer == NULL || buffer_size == 0)
			return 0;


		do {
			if (max_try_count == 0)
				return 0;


			max_try_count--;
			recv_data_cnt = 0;
			/* read_timeout_ms = 20; */

			DSI_WaitForNotBusy(module, cmdq);

			t0.CONFG = 0x04;	/* BTA */
			/* / 0xB0 is used to distinguish DCS cmd or Gerneric cmd, is that Right??? */
			t0.Data_ID =
			    (cmd <
			     0xB0) ? DSI_DCS_READ_PACKET_ID :
			    DSI_GERNERIC_READ_LONG_PACKET_ID;
			t0.Data0 = cmd;
			t0.Data1 = 0;

			DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t0));
			DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);

			DSI_OUTREGBIT(cmdq, struct DSI_RACK_REG, DSI_REG[d]->DSI_RACK,
					  DSI_RACK, 1);
			DSI_OUTREGBIT(cmdq, struct DSI_INT_ENABLE_REG, DSI_REG[d]->DSI_INTEN,
				      RD_RDY, 1);
			DSI_OUTREGBIT(cmdq, struct DSI_INT_ENABLE_REG, DSI_REG[d]->DSI_INTEN,
				      CMD_DONE, 1);

			DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_START, 0);
			DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_START, 1);

			/* / the following code is to */
			/* / 1: wait read ready */
			/* / 2: ack read ready(interrupt handler do this op) */
			/* / 3: wait for CMDQ_DONE(interrupt handler do this op) */
			/* / 4: read data */
			ret =
			    wait_event_interruptible_timeout(_dsi_dcs_read_wait_queue[d],
							     waitRDDone, WAIT_TIMEOUT);
			if (ret > 0) {
				do {
					timeout++;
					udelay(1);
					DSI_OUTREGBIT(NULL, struct DSI_RACK_REG, DSI_REG[d]->DSI_RACK,
						DSI_RACK, 1);
				} while (DSI_REG[d]->DSI_INTSTA.BUSY && (timeout < 1000));
			}
			waitRDDone = false;
			if (timeout == 1000)
				ret = 0;
			if (0 == ret) {
				DISPERR("dsi wait read ready timeout %d\n", timeout);
				DSI_DumpRegisters(module, 2);
				/* do necessary reset here */
				DSI_OUTREGBIT(cmdq, struct DSI_RACK_REG, DSI_REG[d]->DSI_RACK,
					      DSI_RACK, 1);
				DSI_Reset(module, NULL);
				return 0;
			}
			/* clear interrupt */
			DSI_OUTREGBIT(cmdq, struct DSI_INT_ENABLE_REG, DSI_REG[d]->DSI_INTSTA,
				      RD_RDY, 0);
			DSI_OUTREGBIT(cmdq, struct DSI_INT_ENABLE_REG, DSI_REG[d]->DSI_INTSTA,
				      CMD_DONE, 0);

			/* read data */
			DSI_OUTREG32(cmdq, &read_data0,
				     AS_UINT32(&DSI_REG[d]->DSI_RX_DATA0));
			DSI_OUTREG32(cmdq, &read_data1,
				     AS_UINT32(&DSI_REG[d]->DSI_RX_DATA1));
			DSI_OUTREG32(cmdq, &read_data2,
				     AS_UINT32(&DSI_REG[d]->DSI_RX_DATA2));
			DSI_OUTREG32(cmdq, &read_data3,
				     AS_UINT32(&DSI_REG[d]->DSI_RX_DATA3));

			{
/*
			DISPMSG("DSI_RX_STA     : 0x%x\n", DSI_REG[d]->DSI_TRIG_STA);
			DISPMSG("DSI_CMDQ_SIZE  : 0x%x\n", DSI_REG[d]->DSI_CMDQ_SIZE.CMDQ_SIZE);
			DISPMSG("DSI_CMDQ_DATA0 : 0x%x\n", DSI_CMDQ_REG[d]->data[0]);
			DISPMSG("DSI_RX_DATA0   : 0x%x\n", DSI_REG[d]->DSI_RX_DATA0);
			DISPMSG("DSI_RX_DATA1   : 0x%x\n", DSI_REG[d]->DSI_RX_DATA1);
			DISPMSG("DSI_RX_DATA2   : 0x%x\n", DSI_REG[d]->DSI_RX_DATA2);
			DISPMSG("DSI_RX_DATA3   : 0x%x\n", DSI_REG[d]->DSI_RX_DATA3);

			DISPMSG("read_data0     : 0x%x\n", read_data0);
			DISPMSG("read_data1     : 0x%x\n", read_data1);
			DISPMSG("read_data2     : 0x%x\n", read_data2);
			DISPMSG("read_data3     : 0x%x\n", read_data3);*/
			}

			packet_type = read_data0.byte0;

			DISPMSG("DSI read packet_type is 0x%x\n", packet_type);

			if (packet_type == 0x1A || packet_type == 0x1C) {
				recv_data_cnt = read_data0.byte1 + read_data0.byte2 * 16;
				if (recv_data_cnt > 10) {
					DISPMSG
					    ("DSI read long packet data exceeds 10 bytes\n");
					recv_data_cnt = 10;
				}

				if (recv_data_cnt > buffer_size) {
					DISPMSG
					    ("DSI read long packet data exceeds buffer size: %d\n",
					     buffer_size);
					recv_data_cnt = buffer_size;
				}
				DISPMSG("DSI read long packet size: %d\n", recv_data_cnt);
				if (recv_data_cnt <= 4) {
					memcpy((void *)buffer, (void *)&read_data1,
					       recv_data_cnt);
				} else if (recv_data_cnt <= 8) {
					memcpy((void *)buffer, (void *)&read_data1, 4);
					memcpy((void *)((uint8_t *) buffer + 4),
					       (void *)&read_data2, recv_data_cnt - 4);
				} else {/* recv_data_cnt>8 && recv_data_cnt<=10 */
					memcpy((void *)buffer, (void *)&read_data1, 4);
					memcpy((void *)((uint8_t *) buffer + 4),
					       (void *)&read_data2, 4);
					memcpy((void *)((uint8_t *) buffer + 8),
					       (void *)&read_data3, recv_data_cnt - 8);
				}
			} else {
				recv_data_cnt = 2;
				if (recv_data_cnt > buffer_size) {
					DISPMSG
					    ("DSI read short packet data exceeds buffer size: %d\n",
					     buffer_size);
					recv_data_cnt = buffer_size;
				}
				memcpy((void *)buffer, (void *)&read_data0.byte1,
				       recv_data_cnt);
			}
		} while (packet_type != 0x1C && packet_type != 0x21 && packet_type != 0x22
			 && packet_type != 0x1A);
		/* / here: we may receive a ACK packet which packet type is 0x02
			(incdicates some error happened) */
		/* / therefore we try re-read again until no ACK packet */
		/* / But: if it is a good way to keep re-trying ??? */
	}

	return recv_data_cnt;
}


void DSI_set_null(DISP_MODULE_ENUM module, void *cmdq, unsigned cmd, unsigned char count,
		  unsigned char *para_list, unsigned char force_update) {
	uint32_t i = 0;
	int d = 0;
	unsigned long goto_addr, mask_para, set_para;
	DSI_T2_INS t2;

	/* DISPFUNC(); */
	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++) {
		if (0 != DSI_REG[d]->DSI_MODE_CTRL.MODE) {	/* not in cmd mode */
		} else {
			DSI_WaitForNotBusy(module, cmdq);

			/* null packet */
			t2.CONFG = 2;
			t2.Data_ID = DSI_NULL_PACKET_ID;
			t2.WC16 = count;

			DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t2));
			DISPMSG("[DSI] start: 0x%08x\n",
				AS_UINT32(&DSI_CMDQ_REG[d]->data[0]));

			for (i = 0; i < count; i++) {
				goto_addr =
				    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte0) + i;
				mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
				set_para = (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
				DSI_MASKREG32(cmdq, goto_addr & (~((unsigned long)0x3u)),
					      mask_para, set_para);

				if ((i % 4) == 0x3)
					DISPMSG("[DSI] cmd: 0x%08x\n",
						AS_UINT32(&DSI_CMDQ_REG[d]->
							  data[1 + (i / 4)]));
			}

			DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1 + (count) / 4);
			DISPMSG("[DSI] size: 0x%08x\n",
				AS_UINT32(&DSI_REG[d]->DSI_CMDQ_SIZE));

			if (force_update) {
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
			}
		}
	}
}

void DSI_set_cmdq_V2(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned cmd,
		     unsigned char count, unsigned char *para_list,
		     unsigned char force_update) {
	uint32_t i = 0;
	int d = 0;
	unsigned long goto_addr, mask_para, set_para;
	DSI_T0_INS t0;
	DSI_T2_INS t2;

	memset(&t2, 0, sizeof(DSI_T2_INS));
	/* DISPFUNC(); */
	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++) {
		if (0 != DSI_REG[d]->DSI_MODE_CTRL.MODE) {	/* not in cmd mode */
			struct DSI_VM_CMD_CON_REG vm_cmdq;

			memset(&vm_cmdq, 0, sizeof(struct DSI_VM_CMD_CON_REG));
			DSI_READREG32((struct DSI_VM_CMD_CON_REG *), &vm_cmdq,
				      &DSI_REG[d]->DSI_VM_CMD_CON);
			if (cmd < 0xB0) {
				if (count > 1) {
					vm_cmdq.LONG_PKT = 1;
					vm_cmdq.CM_DATA_ID = DSI_DCS_LONG_PACKET_ID;
					vm_cmdq.CM_DATA_0 = count + 1;
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON,
						     AS_UINT32(&vm_cmdq));

					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
							    byte0);
					mask_para = (0xFF << ((goto_addr & 0x3) * 8));
					set_para = (cmd << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para,
						      set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
						mask_para =
						    (0xFF << ((goto_addr & 0x3) * 8));
						set_para =
						    (para_list[i] << ((goto_addr & 0x3) * 8));
						DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
					}
				} else {
					vm_cmdq.LONG_PKT = 0;
					vm_cmdq.CM_DATA_0 = cmd;
					if (count) {
						vm_cmdq.CM_DATA_ID =
						    DSI_DCS_SHORT_PACKET_ID_1;
						vm_cmdq.CM_DATA_1 = para_list[0];
					} else {
						vm_cmdq.CM_DATA_ID =
						    DSI_DCS_SHORT_PACKET_ID_0;
						vm_cmdq.CM_DATA_1 = 0;
					}
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
				}
			} else {
				if (count > 1) {
					vm_cmdq.LONG_PKT = 1;
					vm_cmdq.CM_DATA_ID = DSI_GERNERIC_LONG_PACKET_ID;
					vm_cmdq.CM_DATA_0 = count + 1;
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));

					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
							    byte0);
					mask_para = (0xFF << ((goto_addr & 0x3) * 8));
					set_para = (cmd << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
						mask_para =
						    (0xFF << ((goto_addr & 0x3) * 8));
						set_para =
						    (para_list[i] << ((goto_addr & 0x3) * 8));
						DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
					}
				} else {
					vm_cmdq.LONG_PKT = 0;
					vm_cmdq.CM_DATA_0 = cmd;
					if (count) {
						vm_cmdq.CM_DATA_ID =
						    DSI_GERNERIC_SHORT_PACKET_ID_2;
						vm_cmdq.CM_DATA_1 = para_list[0];
					} else {
						vm_cmdq.CM_DATA_ID =
						    DSI_GERNERIC_SHORT_PACKET_ID_1;
						vm_cmdq.CM_DATA_1 = 0;
					}
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
				}
			}
			/* start DSI VM CMDQ */
			if (force_update)
				DSI_EnableVM_CMD(module, cmdq);

		} else {
			DSI_WaitForNotBusy(module, cmdq);

			if (cmd < 0xB0) {
				if (count > 1) {
					t2.CONFG = 2;
					t2.Data_ID = DSI_DCS_LONG_PACKET_ID;
					t2.WC16 = count + 1;

					DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t2));

					goto_addr =
					    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte0);
					mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
					set_para = (cmd << ((goto_addr & 0x3u) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte1) + i;
						mask_para =
						    (0xFFu << ((goto_addr & 0x3u) * 8));
						set_para =
						    (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
						DSI_MASKREG32(cmdq,
							goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);


					}

					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 2 + (count) / 4);
				} else {
					t0.CONFG = 0;
					t0.Data0 = cmd;
					if (count) {
						t0.Data_ID = DSI_DCS_SHORT_PACKET_ID_1;
						t0.Data1 = para_list[0];
					} else {
						t0.Data_ID = DSI_DCS_SHORT_PACKET_ID_0;
						t0.Data1 = 0;
					}

					DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0],
						     AS_UINT32(&t0));
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);
				}
			} else {
				if (count > 1) {
					t2.CONFG = 2;
					t2.Data_ID = DSI_GERNERIC_LONG_PACKET_ID;
					t2.WC16 = count + 1;

					DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t2));

					goto_addr =
					    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte0);
					mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
					set_para = (cmd << ((goto_addr & 0x3u) * 8));
					DSI_MASKREG32(cmdq,
						      goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte1) + i;
						mask_para =
						    (0xFFu << ((goto_addr & 0x3u) * 8));
						set_para =
						    (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
						DSI_MASKREG32(cmdq,
							goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);
					}

					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 2 + (count) / 4);
				} else {
					t0.CONFG = 0;
					t0.Data0 = cmd;
					if (count) {
						t0.Data_ID = DSI_GERNERIC_SHORT_PACKET_ID_2;
						t0.Data1 = para_list[0];
					} else {
						t0.Data_ID = DSI_GERNERIC_SHORT_PACKET_ID_1;
						t0.Data1 = 0;
					}
					DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0],
						     AS_UINT32(&t0));
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);
				}
			}
			if (force_update) {
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
			}
		}
	}
}

void DSI_set_cmdq_V2_DCS(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned cmd,
		     unsigned char count, unsigned char *para_list,
		     unsigned char force_update) {
	uint32_t i = 0;
	int d = 0;
	unsigned long goto_addr, mask_para, set_para;
	DSI_T0_INS t0;
	DSI_T2_INS t2;

	memset(&t2, 0, sizeof(DSI_T2_INS));
	/* DISPFUNC(); */
	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++) {
		if (0 != DSI_REG[d]->DSI_MODE_CTRL.MODE) {	/* not in cmd mode */
			struct DSI_VM_CMD_CON_REG vm_cmdq;

			memset(&vm_cmdq, 0, sizeof(struct DSI_VM_CMD_CON_REG));
			DSI_READREG32((struct DSI_VM_CMD_CON_REG *), &vm_cmdq,
				      &DSI_REG[d]->DSI_VM_CMD_CON);

			if (count > 1) {
				vm_cmdq.LONG_PKT = 1;
				vm_cmdq.CM_DATA_ID = DSI_DCS_LONG_PACKET_ID;
				vm_cmdq.CM_DATA_0 = count + 1;
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON,
					     AS_UINT32(&vm_cmdq));

				goto_addr =
				    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
						    byte0);
				mask_para = (0xFF << ((goto_addr & 0x3) * 8));
				set_para = (cmd << ((goto_addr & 0x3) * 8));
				DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para,
					      set_para);

				for (i = 0; i < count; i++) {
					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
					mask_para =
					    (0xFF << ((goto_addr & 0x3) * 8));
					set_para =
					    (((unsigned long)para_list[i]) << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
				}
			} else {
				vm_cmdq.LONG_PKT = 0;
				vm_cmdq.CM_DATA_0 = cmd;
				if (count) {
					vm_cmdq.CM_DATA_ID =
					    DSI_DCS_SHORT_PACKET_ID_1;
					vm_cmdq.CM_DATA_1 = para_list[0];
				} else {
					vm_cmdq.CM_DATA_ID =
					    DSI_DCS_SHORT_PACKET_ID_0;
					vm_cmdq.CM_DATA_1 = 0;
				}
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
			}

			/* start DSI VM CMDQ */
			if (force_update)
				DSI_EnableVM_CMD(module, cmdq);

		} else {
			DSI_WaitForNotBusy(module, cmdq);

			if (count > 1) {
				t2.CONFG = 2;
				t2.Data_ID = DSI_DCS_LONG_PACKET_ID;
				t2.WC16 = count + 1;

				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t2));

				goto_addr =
				    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte0);
				mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
				set_para = (cmd << ((goto_addr & 0x3u) * 8));
				DSI_MASKREG32(cmdq, goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);

				for (i = 0; i < count; i++) {
					goto_addr =
					    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte1) + i;
					mask_para =
					    (0xFFu << ((goto_addr & 0x3u) * 8));
					set_para =
					    (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
					DSI_MASKREG32(cmdq,
						goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);


				}

				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 2 + (count) / 4);
			} else {
				t0.CONFG = 0;
				t0.Data0 = cmd;
				if (count) {
					t0.Data_ID = DSI_DCS_SHORT_PACKET_ID_1;
					t0.Data1 = para_list[0];
				} else {
					t0.Data_ID = DSI_DCS_SHORT_PACKET_ID_0;
					t0.Data1 = 0;
				}

				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0],
					     AS_UINT32(&t0));
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);
			}

			if (force_update) {
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
			}
		}
	}
}

void DSI_set_cmdq_V2_generic(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned cmd,
		     unsigned char count, unsigned char *para_list,
		     unsigned char force_update) {
	uint32_t i = 0;
	int d = 0;
	unsigned long goto_addr, mask_para, set_para;
	DSI_T0_INS t0;
	DSI_T2_INS t2;

	memset(&t2, 0, sizeof(DSI_T2_INS));
	/* DISPFUNC(); */
	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++) {
		if (0 != DSI_REG[d]->DSI_MODE_CTRL.MODE) {	/* not in cmd mode */
			struct DSI_VM_CMD_CON_REG vm_cmdq;

			memset(&vm_cmdq, 0, sizeof(struct DSI_VM_CMD_CON_REG));
			DSI_READREG32((struct DSI_VM_CMD_CON_REG *), &vm_cmdq,
				      &DSI_REG[d]->DSI_VM_CMD_CON);

			if (count > 1) {
				vm_cmdq.LONG_PKT = 1;
				vm_cmdq.CM_DATA_ID = DSI_GERNERIC_LONG_PACKET_ID;
				vm_cmdq.CM_DATA_0 = count + 1;
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));

				goto_addr =
				    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
						    byte0);
				mask_para = (0xFF << ((goto_addr & 0x3) * 8));
				set_para = (cmd << ((goto_addr & 0x3) * 8));
				DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);

				for (i = 0; i < count; i++) {
					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
					mask_para =
					    (0xFF << ((goto_addr & 0x3) * 8));
					set_para =
					    (((unsigned long)para_list[i]) << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
				}
			} else {
				vm_cmdq.LONG_PKT = 0;
				vm_cmdq.CM_DATA_0 = cmd;
				if (count) {
					vm_cmdq.CM_DATA_ID =
					    DSI_GERNERIC_SHORT_PACKET_ID_2;
					vm_cmdq.CM_DATA_1 = para_list[0];
				} else {
					vm_cmdq.CM_DATA_ID =
					    DSI_GERNERIC_SHORT_PACKET_ID_1;
					vm_cmdq.CM_DATA_1 = 0;
				}
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
			}

			/* start DSI VM CMDQ */
			if (force_update)
				DSI_EnableVM_CMD(module, cmdq);

		} else {
			DSI_WaitForNotBusy(module, cmdq);

			if (count > 1) {
				t2.CONFG = 2;
				t2.Data_ID = DSI_GERNERIC_LONG_PACKET_ID;
				t2.WC16 = count + 1;

				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t2));

				goto_addr =
				    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte0);
				mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
				set_para = (cmd << ((goto_addr & 0x3u) * 8));
				DSI_MASKREG32(cmdq,
					      goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);

				for (i = 0; i < count; i++) {
					goto_addr =
					    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte1) + i;
					mask_para =
					    (0xFFu << ((goto_addr & 0x3u) * 8));
					set_para =
					    (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
					DSI_MASKREG32(cmdq,
						goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);
				}

				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 2 + (count) / 4);
			} else {
				t0.CONFG = 0;
				t0.Data0 = cmd;
				if (count) {
					t0.Data_ID = DSI_GERNERIC_SHORT_PACKET_ID_2;
					t0.Data1 = para_list[0];
				} else {
					t0.Data_ID = DSI_GERNERIC_SHORT_PACKET_ID_1;
					t0.Data1 = 0;
				}
				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0],
					     AS_UINT32(&t0));
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);
			}

			if (force_update) {
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
			}
		}
	}
}

void DSI_set_cmdq_subV3(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
		     LCM_setting_table_V3 *para_tbl, unsigned int size,
		     unsigned char force_update, int d)
{
#if 1
	uint32_t i;
	/* uint32_t layer, layer_state, lane_num; */
	unsigned long goto_addr, mask_para, set_para;
	/* uint32_t fbPhysAddr, fbVirAddr; */
	DSI_T0_INS t0;
	/* DSI_T1_INS t1; */
	DSI_T2_INS t2;

	uint32_t index = 0;
	unsigned char data_id, cmd, count;
	unsigned char *para_list;



	do {
		data_id = para_tbl[index].id;
		cmd = para_tbl[index].cmd;
		count = para_tbl[index].count;
		para_list = para_tbl[index].para_list;

		if (data_id == REGFLAG_ESCAPE_ID && cmd == REGFLAG_DELAY_MS_V3) {
			udelay(1000 * count);
			DISPMSG("DSI_set_cmdq_V3[%d]. Delay %d (ms)\n", index,
				 count);

			continue;
		}

		if (0 != DSI_REG[d]->DSI_MODE_CTRL.MODE) {
			struct DSI_VM_CMD_CON_REG vm_cmdq;

			memset(&vm_cmdq, 0, sizeof(struct DSI_VM_CMD_CON_REG));
			DSI_READREG32((struct DSI_VM_CMD_CON_REG *), &vm_cmdq,
				      &DSI_REG[d]->DSI_VM_CMD_CON);
			if (cmd < 0xB0) {
				if (count > 1) {
					vm_cmdq.LONG_PKT = 1;
					vm_cmdq.CM_DATA_ID = data_id;
					vm_cmdq.CM_DATA_0 = count + 1;
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON,
					     AS_UINT32(&vm_cmdq));

					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
						    byte0);
					mask_para = (0xFF << ((goto_addr & 0x3) * 8));
					set_para = (cmd << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para,
						      set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
						mask_para =
						    (0xFF << ((goto_addr & 0x3) * 8));
						set_para =
						    (para_list[i] << ((goto_addr & 0x3) * 8));
						DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
					}
				} else {
					vm_cmdq.LONG_PKT = 0;
					vm_cmdq.CM_DATA_0 = cmd;
					if (count) {
						vm_cmdq.CM_DATA_ID =
						    data_id;
						vm_cmdq.CM_DATA_1 = para_list[0];
					} else {
						vm_cmdq.CM_DATA_ID =
						    data_id;
						vm_cmdq.CM_DATA_1 = 0;
					}
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
				}
			} else {
				if (count > 1) {
					vm_cmdq.LONG_PKT = 1;
					vm_cmdq.CM_DATA_ID = data_id;
					vm_cmdq.CM_DATA_0 = count + 1;
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));

					goto_addr =
					    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].
							    byte0);
					mask_para = (0xFF << ((goto_addr & 0x3) * 8));
					set_para = (cmd << ((goto_addr & 0x3) * 8));
					DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);

					for (i = 0; i < count; i++) {
						goto_addr =
						    (unsigned long)(&DSI_VM_CMD_REG[d]->data[0].byte1) + i;
						mask_para =
						    (0xFF << ((goto_addr & 0x3) * 8));
						set_para =
						    (para_list[i] << ((goto_addr & 0x3) * 8));
						DSI_MASKREG32(cmdq, goto_addr & (~0x3), mask_para, set_para);
					}
				} else {
					vm_cmdq.LONG_PKT = 0;
					vm_cmdq.CM_DATA_0 = cmd;
					if (count) {
						vm_cmdq.CM_DATA_ID =
						    data_id;
						vm_cmdq.CM_DATA_1 = para_list[0];
					} else {
						vm_cmdq.CM_DATA_ID =
						    data_id;
						vm_cmdq.CM_DATA_1 = 0;
					}
					DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));
				}
			}
			/* start DSI VM CMDQ */
			if (force_update)
				DSI_EnableVM_CMD(module, cmdq);

		} else {
			DSI_WaitForNotBusy(module, cmdq);
			/* for(i = 0; i < sizeof(DSI_CMDQ_REG->data0) / sizeof(struct DSI_CMDQ); i++) */
			/* OUTREG32(&DSI_CMDQ_REG->data0[i], 0); */
			/* memset(&DSI_CMDQ_REG->data[0], 0, sizeof(DSI_CMDQ_REG->data[0])); */
			OUTREG32(&DSI_CMDQ_REG[d]->data[0], 0);
			if (count > 1) {
				t2.CONFG = 2;
				t2.Data_ID = data_id;
				t2.WC16 = count + 1;

				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0].byte0,
					     AS_UINT32(&t2));

				goto_addr =
				    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].
						    byte0);
				mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
				set_para = (cmd << ((goto_addr & 0x3u) * 8));
				DSI_MASKREG32(cmdq,
					      goto_addr & (~((unsigned long)0x3u)),
					      mask_para, set_para);

				for (i = 0; i < count; i++) {
					goto_addr =
					    (unsigned long)(&DSI_CMDQ_REG[d]->data[1].byte1) + i;
					mask_para =
					    (0xFFu << ((goto_addr & 0x3u) * 8));
					set_para =
					    (((unsigned long)para_list[i]) << ((goto_addr & 0x3u) * 8));
					DSI_MASKREG32(cmdq,
						goto_addr & (~((unsigned long)0x3u)), mask_para, set_para);
				}
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 2 + (count) / 4);
			} else {
				t0.CONFG = 0;
				t0.Data0 = cmd;
				if (count) {
					t0.Data_ID = data_id;
					t0.Data1 = para_list[0];
				} else {
					t0.Data_ID = data_id;
					t0.Data1 = 0;
				}
				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[d]->data[0], AS_UINT32(&t0));
				DSI_OUTREG32(cmdq, &DSI_REG[d]->DSI_CMDQ_SIZE, 1);
			}

			if (force_update) {
				MMProfileLog(MTKFB_MMP_Events.DSICmd, MMProfileFlagStart);
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
				MMProfileLog(MTKFB_MMP_Events.DSICmd, MMProfileFlagEnd);
			}
		}
	} while (++index < size);

#endif
}

void DSI_set_cmdq_V3(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
		     LCM_setting_table_V3 *para_tbl, unsigned int size,
		     unsigned char force_update)
{
#if 1
	int d = 0;

	for (d = DSI_MODULE_BEGIN(module); d <= DSI_MODULE_END(module); d++)
		DSI_set_cmdq_subV3(module, cmdq, para_tbl, size, force_update, d);
#endif
}






void DSI_set_cmdq(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned int *pdata,
		  unsigned int queue_size, unsigned char force_update)
{
	/* DISPFUNC(); */
	/* _WaitForEngineNotBusy(); */
	int j = 0;
	int i = 0;
	int regData = 0;
	struct DSI_VM_CMD_CON_REG vm_cmdq;
	/* DISPMSG("DSI_set_cmdq, module=%s, cmdq=0x%08x\n", ddp_get_module_name(module), cmdq); */

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (0 != DSI_REG[i]->DSI_MODE_CTRL.MODE) {
			memset(&vm_cmdq, 0, sizeof(struct DSI_VM_CMD_CON_REG));
			DSI_READREG32((struct DSI_VM_CMD_CON_REG *), &vm_cmdq,
				    &DSI_REG[i]->DSI_VM_CMD_CON);

			if (queue_size <= 1) {
				vm_cmdq.LONG_PKT = 0;
				vm_cmdq.CM_DATA_ID = ((pdata[0] >> 8) & 0xFF);
				vm_cmdq.CM_DATA_0 = ((pdata[0] >> 16) & 0xFF);
				vm_cmdq.CM_DATA_1 = ((pdata[0] >> 24) & 0xFF);
				DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));

			} else {
				vm_cmdq.LONG_PKT = 1;
				vm_cmdq.CM_DATA_ID = ((pdata[0] >> 8) & 0xFF);
				vm_cmdq.CM_DATA_0 = ((pdata[0] >> 16) & 0xFF);
				vm_cmdq.CM_DATA_1 = 0;
				DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_CON, AS_UINT32(&vm_cmdq));

				for (j = 0; j < queue_size - 1; j++) {
					regData = j % 4;
					if (regData == 0)
						DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_DATA0
							 , AS_UINT32((pdata + j + 1)));
					else  if (regData == 1)
						DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_DATA4
							 , AS_UINT32((pdata + j + 1)));
					else  if (regData == 2)
						DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_DATA8
							 , AS_UINT32((pdata + j + 1)));
					else  if (regData == 3)
						DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_VM_CMD_DATAC
							 , AS_UINT32((pdata + j + 1)));
				}
			}

			if (force_update)
				DSI_EnableVM_CMD(module, cmdq);

		 } else {
			 ASSERT(queue_size <= 32);
			 DSI_WaitForNotBusy(module, cmdq);
#ifdef ENABLE_DSI_ERROR_REPORT
			 if ((pdata[0] & 1)) {
				memcpy(_dsi_cmd_queue, pdata, queue_size * 4);
				_dsi_cmd_queue[queue_size++] = 0x4;
				pdata = (unsigned int *)_dsi_cmd_queue;
			 } else {
				pdata[0] |= 4;
			 }
#endif

			for (j = 0; j < queue_size; j++)
				DSI_OUTREG32(cmdq, &DSI_CMDQ_REG[i]->data[j], AS_UINT32((pdata + j)));

			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_CMDQ_SIZE, queue_size);

			if (force_update) {
				DSI_Start(module, cmdq);
				DSI_WaitForNotBusy(module, cmdq);
			 }
		}
	}
}


void DSI_set_rar(DISP_MODULE_ENUM module, void *cmdq)
{
	int i = 0;
	struct DSI_PHY_LD0CON_REG phy_ld0con;

	memset(&phy_ld0con, 0, sizeof(struct DSI_PHY_LD0CON_REG));

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_READREG32((struct DSI_PHY_LD0CON_REG *), &phy_ld0con, &DSI_REG[i]->DSI_PHY_LD0CON);
		phy_ld0con.L0_RM_TRIG_EN = 1;
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_LD0CON, AS_UINT32(&phy_ld0con));
		mdelay(1);
		phy_ld0con.L0_RM_TRIG_EN = 0;
		DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_PHY_LD0CON, AS_UINT32(&phy_ld0con));
		mdelay(1);
	}
}

void _copy_dsi_params(LCM_DSI_PARAMS *src, LCM_DSI_PARAMS *dst)
{
	memcpy((LCM_DSI_PARAMS *) dst, (LCM_DSI_PARAMS *) src, sizeof(LCM_DSI_PARAMS));
}



int DSI_Send_ROI(DISP_MODULE_ENUM module, void *handle, unsigned int x, unsigned int y,
		 unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	DSI_set_cmdq(module, handle, data_array, 3, 1);
	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	DSI_set_cmdq(module, handle, data_array, 3, 1);
	DISPMSG("DSI_Send_ROI Done!\n");

	/* data_array[0]= 0x002c3909; */
	/* DSI_set_cmdq(module, handle, data_array, 1, 0); */
	return 0;
}



static void lcm_set_reset_pin(uint32_t value)
{
	DSI_OUTREG32(NULL, DISPSYS_CONFIG_BASE + 0x150, value);
}

static void lcm_udelay(uint32_t us)
{
	udelay(us);
}

static void lcm_mdelay(uint32_t ms)
{
	if (ms < 10) {
		udelay(ms * 1000);
	} else {
		msleep(ms);
		/* udelay(ms*1000); */
	}
}

static void lcm_rar(uint32_t ms)
{
	DSI_set_rar(DISP_MODULE_DSI0, NULL);
	mdelay(ms);
}

void DSI_set_cmdq_V2_DSI0(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSI0, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_DSI0(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSI0, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_DSI0(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSI0, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DSI1(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSI1, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_DSI1(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSI1, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_DSI1(void *cmdq, unsigned cmd, unsigned char count,
			  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSI1, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DSIDual(void *cmdq, unsigned cmd, unsigned char count,
			     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSIDUAL, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_DSIDual(void *cmdq, unsigned cmd, unsigned char count,
			     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSIDUAL, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_DSIDual(void *cmdq, unsigned cmd, unsigned char count,
			     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSIDUAL, cmdq, cmd, count, para_list, force_update);
}

void DSI_set_null_Wrapper_DSI0(unsigned cmd, unsigned char count, unsigned char *para_list,
			       unsigned char force_update) {
	DSI_set_null(DISP_MODULE_DSI0, NULL, cmd, count, para_list, force_update);
}

void DSI_set_null_Wrapper_DSI1(unsigned cmd, unsigned char count, unsigned char *para_list,
			       unsigned char force_update) {
	DSI_set_null(DISP_MODULE_DSI1, NULL, cmd, count, para_list, force_update);
}

void DSI_set_null_Wrapper_DSIDual(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_null(DISP_MODULE_DSIDUAL, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_Wrapper_DSI0(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSI0, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_Wrapper_DSI0(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSI0, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_Wrapper_DSI0(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSI0, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_Wrapper_DSI1(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSI1, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_Wrapper_DSI1(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSI1, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_Wrapper_DSI1(unsigned cmd, unsigned char count,
				  unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSI1, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_Wrapper_DSIDual(unsigned cmd, unsigned char count,
				     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2(DISP_MODULE_DSIDUAL, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_DCS_Wrapper_DSIDual(unsigned cmd, unsigned char count,
				     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_DCS(DISP_MODULE_DSIDUAL, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V2_generic_Wrapper_DSIDual(unsigned cmd, unsigned char count,
				     unsigned char *para_list, unsigned char force_update) {
	DSI_set_cmdq_V2_generic(DISP_MODULE_DSIDUAL, NULL, cmd, count, para_list, force_update);
}

void DSI_set_cmdq_V3_Wrapper_DSI0(LCM_setting_table_V3 *para_tbl, unsigned int size,
				  unsigned char force_update) {
	DSI_set_cmdq_V3(DISP_MODULE_DSI0, NULL, para_tbl, size, force_update);
}

void DSI_set_cmdq_V3_Wrapper_DSI1(LCM_setting_table_V3 *para_tbl, unsigned int size,
				  unsigned char force_update) {
	DSI_set_cmdq_V3(DISP_MODULE_DSI1, NULL, para_tbl, size, force_update);
}

void DSI_set_cmdq_V3_Wrapper_DSIDual(LCM_setting_table_V3 *para_tbl, unsigned int size,
				     unsigned char force_update) {
	DSI_set_cmdq_V3(DISP_MODULE_DSIDUAL, NULL, para_tbl, size, force_update);
}

void DSI_set_cmdq_wrapper_DSI0(unsigned int *pdata, unsigned int queue_size,
			       unsigned char force_update) {
	DSI_set_cmdq(DISP_MODULE_DSI0, NULL, pdata, queue_size, force_update);
}

void DSI_set_cmdq_wrapper_DSI1(unsigned int *pdata, unsigned int queue_size,
			       unsigned char force_update) {
	DSI_set_cmdq(DISP_MODULE_DSI1, NULL, pdata, queue_size, force_update);
}

void DSI_set_cmdq_wrapper_DSIDual(unsigned int *pdata, unsigned int queue_size,
				  unsigned char force_update) {
	DSI_set_cmdq(DISP_MODULE_DSIDUAL, NULL, pdata, queue_size, force_update);
}

unsigned int DSI_dcs_read_lcm_reg_v2_wrapper_DSI0(uint8_t cmd, uint8_t *buffer,
						  uint8_t buffer_size) {
	return DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, cmd, buffer, buffer_size);
}

unsigned int DSI_dcs_read_lcm_reg_v2_wrapper_DSI1(uint8_t cmd, uint8_t *buffer,
						  uint8_t buffer_size) {
	return DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI1, NULL, cmd, buffer, buffer_size);
}

unsigned int DSI_dcs_read_lcm_reg_v2_wrapper_DSIDUAL(uint8_t cmd, uint8_t *buffer,
						     uint8_t buffer_size) {
	return DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSIDUAL, NULL, cmd, buffer, buffer_size);
}

static LCM_UTIL_FUNCS lcm_utils_dsi0;
static LCM_UTIL_FUNCS lcm_utils_dsi1;
static LCM_UTIL_FUNCS lcm_utils_dsidual;

long lcd_enp_bias_setting(unsigned int value)
{
	long ret = 0;

	if (value)
		ret = disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP);
	else
		ret = disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN);

	return ret;
}

long lcd_enp_bias_setting_by_name(bool bOn, char *pinName)
{
	long ret = 0;

	if (bOn)
		ret = lcm_turn_on_gate_by_name(1, pinName);
	else
		ret = lcm_turn_on_gate_by_name(0, pinName);

	return ret;
}

int ddp_dsi_set_lcm_utils(DISP_MODULE_ENUM module, LCM_DRIVER *lcm_drv)
{
	LCM_UTIL_FUNCS *utils = NULL;

	if (lcm_drv == NULL) {
		DISPERR("lcm_drv is null\n");
		return -1;
	}

	if (module == DISP_MODULE_DSI0) {
		utils = &lcm_utils_dsi0;
	} else if (module == DISP_MODULE_DSI1) {
		utils = &lcm_utils_dsi1;
	} else if (module == DISP_MODULE_DSIDUAL) {
		utils = &lcm_utils_dsidual;
	} else {
		DISPERR("wrong module: %d\n", module);
		return -1;
	}

	utils->set_reset_pin = lcm_set_reset_pin;
	utils->udelay = lcm_udelay;
	utils->mdelay = lcm_mdelay;
	utils->rar = lcm_rar;
	if (module == DISP_MODULE_DSI0) {
		utils->dsi_set_cmdq = DSI_set_cmdq_wrapper_DSI0;
		utils->dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSI0;
		utils->dsi_set_cmdq_V2_DCS = DSI_set_cmdq_V2_DCS_Wrapper_DSI0;
		utils->dsi_set_cmdq_V2_generic = DSI_set_cmdq_V2_generic_Wrapper_DSI0;
		utils->dsi_set_cmdq_V3 = DSI_set_cmdq_V3_Wrapper_DSI0;
		utils->dsi_set_null = DSI_set_null_Wrapper_DSI0;
		utils->dsi_dcs_read_lcm_reg_v2 = DSI_dcs_read_lcm_reg_v2_wrapper_DSI0;
		utils->dsi_set_cmdq_V22 = DSI_set_cmdq_V2_DSI0;
		utils->dsi_set_cmdq_V22_DCS = DSI_set_cmdq_V2_DCS_DSI0;
		utils->dsi_set_cmdq_V22_generic = DSI_set_cmdq_V2_generic_DSI0;
	} else if (module == DISP_MODULE_DSI1) {
		utils->dsi_set_cmdq = DSI_set_cmdq_wrapper_DSI1;
		utils->dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSI1;
		utils->dsi_set_cmdq_V2_DCS = DSI_set_cmdq_V2_DCS_Wrapper_DSI1;
		utils->dsi_set_cmdq_V2_generic = DSI_set_cmdq_V2_generic_Wrapper_DSI1;
		utils->dsi_set_cmdq_V3 = DSI_set_cmdq_V3_Wrapper_DSI1;
		utils->dsi_set_null = DSI_set_null_Wrapper_DSI1;
		utils->dsi_dcs_read_lcm_reg_v2 = DSI_dcs_read_lcm_reg_v2_wrapper_DSI1;
		utils->dsi_set_cmdq_V22 = DSI_set_cmdq_V2_DSI1;
		utils->dsi_set_cmdq_V22_DCS = DSI_set_cmdq_V2_DCS_DSI1;
		utils->dsi_set_cmdq_V22_generic = DSI_set_cmdq_V2_generic_DSI1;
	} else if (module == DISP_MODULE_DSIDUAL) {
		/* TODO: Ugly workaround, hope we can found better resolution */
		LCM_PARAMS lcm_param;

		lcm_drv->get_params(&lcm_param);
		if (lcm_param.lcm_cmd_if == LCM_INTERFACE_DSI0) {
			utils->dsi_set_cmdq = DSI_set_cmdq_wrapper_DSI0;
			utils->dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSI0;
			utils->dsi_set_cmdq_V2_DCS = DSI_set_cmdq_V2_DCS_Wrapper_DSI0;
			utils->dsi_set_cmdq_V2_generic = DSI_set_cmdq_V2_generic_Wrapper_DSI0;
			utils->dsi_set_cmdq_V3 = DSI_set_cmdq_V3_Wrapper_DSI0;
			utils->dsi_set_null = DSI_set_null_Wrapper_DSI0;
			utils->dsi_dcs_read_lcm_reg_v2 =
				DSI_dcs_read_lcm_reg_v2_wrapper_DSI0;
			utils->dsi_set_cmdq_V22 = DSI_set_cmdq_V2_DSI0;
			utils->dsi_set_cmdq_V22_DCS = DSI_set_cmdq_V2_DCS_DSI0;
			utils->dsi_set_cmdq_V22_generic = DSI_set_cmdq_V2_generic_DSI0;
		} else if (lcm_param.lcm_cmd_if == LCM_INTERFACE_DSI1) {
			utils->dsi_set_cmdq = DSI_set_cmdq_wrapper_DSI1;
			utils->dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSI1;
			utils->dsi_set_cmdq_V2_DCS = DSI_set_cmdq_V2_DCS_Wrapper_DSI1;
			utils->dsi_set_cmdq_V2_generic = DSI_set_cmdq_V2_generic_Wrapper_DSI1;
			utils->dsi_set_cmdq_V3 = DSI_set_cmdq_V3_Wrapper_DSI1;
			utils->dsi_set_null = DSI_set_null_Wrapper_DSI1;
			utils->dsi_dcs_read_lcm_reg_v2 =
				DSI_dcs_read_lcm_reg_v2_wrapper_DSI1;
			utils->dsi_set_cmdq_V22 = DSI_set_cmdq_V2_DSI1;
			utils->dsi_set_cmdq_V22_DCS = DSI_set_cmdq_V2_DCS_DSI1;
			utils->dsi_set_cmdq_V22_generic = DSI_set_cmdq_V2_generic_DSI1;
		} else {
			utils->dsi_set_cmdq = DSI_set_cmdq_wrapper_DSIDual;
			utils->dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSIDual;
			utils->dsi_set_cmdq_V2_DCS = DSI_set_cmdq_V2_DCS_Wrapper_DSIDual;
			utils->dsi_set_cmdq_V2_generic = DSI_set_cmdq_V2_generic_Wrapper_DSIDual;
			utils->dsi_set_cmdq_V3 = DSI_set_cmdq_V3_Wrapper_DSIDual;
			utils->dsi_set_null = DSI_set_null_Wrapper_DSIDual;
			utils->dsi_dcs_read_lcm_reg_v2 =
				DSI_dcs_read_lcm_reg_v2_wrapper_DSIDUAL;
			utils->dsi_set_cmdq_V22 = DSI_set_cmdq_V2_DSIDual;
			utils->dsi_set_cmdq_V22_DCS = DSI_set_cmdq_V2_DCS_DSIDual;
			utils->dsi_set_cmdq_V22_generic = DSI_set_cmdq_V2_generic_DSIDual;
		}
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_LEGACY
	utils->set_gpio_out = (int (*)(unsigned int, unsigned int))mt_set_gpio_out;
	utils->set_gpio_mode = (int (*)(unsigned int, unsigned int))mt_set_gpio_mode;
	utils->set_gpio_dir = (int (*)(unsigned int, unsigned int))mt_set_gpio_dir;
	utils->set_gpio_pull_enable = (int (*)(unsigned int, unsigned char))mt_set_gpio_pull_enable;
#else
	/* TODO: attach replacements of these functions if using kernel standardization... */
	utils->set_gpio_out = 0;
	utils->set_gpio_mode = 0;
	utils->set_gpio_dir = 0;
	utils->set_gpio_pull_enable = 0;
	utils->set_gpio_lcd_enp_bias_ByName = lcd_enp_bias_setting_by_name;
#endif
#endif

	lcm_drv->set_util_funcs(utils);

	return 0;
}


static int dsi0_te_enable = 1;
static int dsi1_te_enable;
/*static int dsidual_te_enable = 0;*/


void DSI_ChangeClk(DISP_MODULE_ENUM module, uint32_t clk)
{
	int i = 0;

	if (clk > 1250 || clk < 50)
		return;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		LCM_DSI_PARAMS *dsi_params = &_dsi_context[i].dsi_params;

		dsi_params->PLL_CLOCK = clk;
		DSI_WaitForNotBusy(module, NULL);
		DSI_PHY_clk_setting(module, NULL, dsi_params);
		DSI_PHY_TIMCONFIG(module, NULL, dsi_params);
	}
}

int ddp_dsi_init(DISP_MODULE_ENUM module, void *cmdq)
{
	DSI_STATUS ret = DSI_STATUS_OK;
	int i = 0;
#ifndef CONFIG_MTK_CLKMGR
	struct device_node *node;
#endif
/*	DISPFUNC();*/
	DISPMSG("%s\n", __func__);
	/* DSI_OUTREG32(cmdq, 0xf0000048, 0x80000000); */
	/* DSI_OUTREG32(cmdq, MMSYS_CONFIG_BASE+0x108, 0xffffffff); */
	/* DSI_OUTREG32(cmdq, MMSYS_CONFIG_BASE+0x118, 0xffffffff); */
	/* DSI_OUTREG32(MMSYS_CONFIG_BASE+0xC08, 0xffffffff); */

#ifndef CONFIG_MTK_CLKMGR
	node = of_find_compatible_node(NULL, NULL, "mediatek,APMIXED");
	if (!node)
		DISPERR("[DDP_APMIXED] DISP find apmixed node failed\n");

	ddp_apmixed_base = of_iomap(node, 0);
	if (!ddp_apmixed_base)
		DISPERR("[DDP_APMIXED] DISP apmixed base failed\n");
#endif /* CONFIG_MTK_CLKMGR */
	DISPFUNC();

	DSI_REG[0] = (struct DSI_REGS *) DISPSYS_DSI0_BASE;
	DSI_PHY_REG[0] = (struct DSI_PHY_REGS *) MIPITX_BASE;
	DSI_CMDQ_REG[0] = (struct DSI_CMDQ_REGS *) (DISPSYS_DSI0_BASE + 0x200);
	DSI_REG[1] = (struct DSI_REGS *) DISPSYS_DSI0_BASE;
	DSI_PHY_REG[1] = (struct DSI_PHY_REGS *) MIPITX_BASE;
	DSI_CMDQ_REG[1] = (struct DSI_CMDQ_REGS *) (DISPSYS_DSI0_BASE + 0x200);
	DSI_VM_CMD_REG[0] = (struct DSI_VM_CMDQ_REGS *) (DISPSYS_DSI0_BASE + 0x134);
	DSI_VM_CMD_REG[1] = (struct DSI_VM_CMDQ_REGS *) (DISPSYS_DSI0_BASE + 0x134);
	memset(&_dsi_context, 0, sizeof(_dsi_context));

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		init_waitqueue_head(&_dsi_cmd_done_wait_queue[i]);
		init_waitqueue_head(&_dsi_dcs_read_wait_queue[i]);
		init_waitqueue_head(&_dsi_wait_bta_te[i]);
		init_waitqueue_head(&_dsi_wait_ext_te[i]);
		init_waitqueue_head(&_dsi_wait_vm_done_queue[i]);
		init_waitqueue_head(&_dsi_wait_vm_cmd_done_queue[i]);
		init_waitqueue_head(&_dsi_wait_sleep_out_done_queue[i]);
	}

	disp_register_module_irq_callback(DISP_MODULE_DSI0, _DSI_INTERNAL_IRQ_Handler);
#if 0
	disp_register_module_irq_callback(DISP_MODULE_DSI1, _DSI_INTERNAL_IRQ_Handler);
	disp_register_module_irq_callback(DISP_MODULE_DSIDUAL, _DSI_INTERNAL_IRQ_Handler);
#endif

#if 0
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
		DISPMSG("dsi%d init finished\n", i);
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (MIPITX_IsEnabled(module, cmdq)) {
#else
	{
#endif
		s_isDsiPowerOn = true;
#ifdef ENABLE_CLK_MGR
#ifndef CONFIG_MTK_CLKMGR
		ddp_set_mipi26m(1);
#endif
		if (module == DISP_MODULE_DSI0 /* || module == DISP_MODULE_DSIDUAL */) {
#ifdef CONFIG_MTK_CLKMGR
			ret += enable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
			ret += enable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
			ret += ddp_clk_enable(DISP1_DSI_ENGINE);
			ret += ddp_clk_enable(DISP1_DSI_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI0 power manager API return false\n");

		}
#if 0
		if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
			ret += enable_clock(MT_CG_DISP1_DSI1_ENGINE, "DSI1");
			ret += enable_clock(MT_CG_DISP1_DSI1_DIGITAL, "DSI1");
#else
			ret += ddp_clk_enable(DISP1_DSI1_ENGINE);
			ret += ddp_clk_enable(DISP1_DSI1_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI1 power manager API return false\n");

		}
#endif
#endif
		DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN, CMD_DONE, 1);
		DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN, RD_RDY, 1);
		DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN, VM_DONE, 1);
		/* enable te_rdy when need, not here (both cmd mode & vdo mode) */
		/* DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG,DSI_REG[0]->DSI_INTEN,TE_RDY,1); */
		DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN, VM_CMD_DONE, 1);
		DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN,
			      SLEEPOUT_DONE, 1);
		DSI_BackupRegisters(module, NULL);
	}

	return DSI_STATUS_OK;
}

int ddp_dsi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	return 0;
}

void _dump_dsi_params(LCM_DSI_PARAMS *dsi_config)
{
	if (dsi_config) {
		switch (dsi_config->mode) {
		case CMD_MODE:
			DISPMSG("[DDPDSI] DSI Mode: CMD_MODE\n");
			break;
		case SYNC_PULSE_VDO_MODE:
			DISPMSG("[DDPDSI] DSI Mode: SYNC_PULSE_VDO_MODE\n");
			break;
		case SYNC_EVENT_VDO_MODE:
			DISPMSG("[DDPDSI] DSI Mode: SYNC_EVENT_VDO_MODE\n");
			break;
		case BURST_VDO_MODE:
			DISPMSG("[DDPDSI] DSI Mode: BURST_VDO_MODE\n");
			break;
		default:
			DISPMSG("[DDPDSI] DSI Mode: Unknown\n");
			break;
		}

		DISPMSG
		    ("[DDPDSI] vact:%d,vbp:%d,vfp:%d,vact_line:%d,hact:%d,hbp:%d,hfp:%d,hblank:%d\n",
		     dsi_config->vertical_sync_active, dsi_config->vertical_backporch,
		     dsi_config->vertical_frontporch, dsi_config->vertical_active_line,
		     dsi_config->horizontal_sync_active, dsi_config->horizontal_backporch,
		     dsi_config->horizontal_frontporch,
		     dsi_config->horizontal_blanking_pixel);
		DISPMSG
		    ("[DDPDSI] pll_select:%d,pll_div1:%d,pll_div2:%d,fbk_div:%d,fbk_sel:%d,rg_bir:%d\n",
		     dsi_config->pll_select, dsi_config->pll_div1, dsi_config->pll_div2,
		     dsi_config->fbk_div, dsi_config->fbk_sel, dsi_config->rg_bir);
		DISPMSG
			("[DDPDSI] rg_bic: %d, rg_bp: %d, PLL_CLOCK: %d, dsi_clock: %d, ssc_range: %d\n",
			 dsi_config->rg_bic, dsi_config->rg_bp, dsi_config->PLL_CLOCK,
		     dsi_config->dsi_clock, dsi_config->ssc_range);
		DISPMSG
			("[DDPDSI] ssc_disable: %d, compatibility_for_nvk: %d, cont_clock: %d\n",
			 dsi_config->ssc_disable, dsi_config->compatibility_for_nvk, dsi_config->cont_clock);
		DISPMSG
		    ("[DDPDSI] lcm_ext_te_enable: %d, noncont_clock: %d, noncont_clock_period: %d\n",
		     dsi_config->lcm_ext_te_enable, dsi_config->noncont_clock,
		     dsi_config->noncont_clock_period);
	}
}

static void DSI_PHY_CLK_LP_PerLine_config(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
					  LCM_DSI_PARAMS *dsi_params) {
	int i;
	struct DSI_PHY_TIMCON0_REG timcon0;	/* LPX */
	struct DSI_PHY_TIMCON2_REG timcon2;	/* CLK_HS_TRAIL, CLK_HS_ZERO */
	struct DSI_PHY_TIMCON3_REG timcon3;	/* CLK_HS_EXIT, CLK_HS_POST, CLK_HS_PREP */
	struct DSI_HSA_WC_REG hsa;
	struct DSI_HBP_WC_REG hbp;
	struct DSI_HFP_WC_REG hfp, new_hfp;
	struct DSI_BLLP_WC_REG bllp;
	struct DSI_PSCTRL_REG ps;
	uint32_t hstx_ckl_wc, new_hstx_ckl_wc;
	uint32_t v_a, v_b, v_c, lane_num;
	LCM_DSI_MODE_CON dsi_mode;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		lane_num = dsi_params->LANE_NUM;
		dsi_mode = dsi_params->mode;

		if (dsi_mode == CMD_MODE)
			continue;

		/* vdo mode */
		DSI_OUTREG32(cmdq, &hsa, AS_UINT32(&DSI_REG[i]->DSI_HSA_WC));
		DSI_OUTREG32(cmdq, &hbp, AS_UINT32(&DSI_REG[i]->DSI_HBP_WC));
		DSI_OUTREG32(cmdq, &hfp, AS_UINT32(&DSI_REG[i]->DSI_HFP_WC));
		DSI_OUTREG32(cmdq, &bllp, AS_UINT32(&DSI_REG[i]->DSI_BLLP_WC));
		DSI_OUTREG32(cmdq, &ps, AS_UINT32(&DSI_REG[i]->DSI_PSCTRL));
		DSI_OUTREG32(cmdq, &hstx_ckl_wc, AS_UINT32(&DSI_REG[i]->DSI_HSTX_CKL_WC));
		DSI_OUTREG32(cmdq, &timcon0, AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON0));
		DSI_OUTREG32(cmdq, &timcon2, AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON2));
		DSI_OUTREG32(cmdq, &timcon3, AS_UINT32(&DSI_REG[i]->DSI_PHY_TIMECON3));

		/* 1. sync_pulse_mode */
		/* Total    WC(A) = HSA_WC + HBP_WC + HFP_WC + PS_WC + 32 */
		/* CLK init WC(B) = (CLK_HS_EXIT + LPX + CLK_HS_PREP + CLK_HS_ZERO)*lane_num */
		/* CLK end  WC(C) = (CLK_HS_POST + CLK_HS_TRAIL)*lane_num */
		/* HSTX_CKLP_WC = A - B */
		/* Limitation: B + C < HFP_WC */
		if (dsi_mode == SYNC_PULSE_VDO_MODE) {
			v_a = hsa.HSA_WC + hbp.HBP_WC + hfp.HFP_WC + ps.DSI_PS_WC + 32;
			v_b =
			    (timcon3.CLK_HS_EXIT + timcon0.LPX + timcon3.CLK_HS_PRPR + timcon2.CLK_ZERO) * lane_num;
			v_c = (timcon3.CLK_HS_POST + timcon2.CLK_TRAIL) * lane_num;

			DISPDBG("===>v_a-v_b=0x%x,HSTX_CKLP_WC=0x%x\n", (v_a - v_b),
				hstx_ckl_wc);
/* DISPDBG("===>v_b+v_c=0x%x,HFP_WC=0x%x\n",(v_b+v_c),hfp); */
			DISPDBG
			    ("===>Will Reconfig in order to fulfill LP clock lane per line\n");

			/* B+C < HFP ,here diff is 0x10; */
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HFP_WC, (v_b + v_c + DIFF_CLK_LANE_LP));
			DSI_OUTREG32(cmdq, &new_hfp, AS_UINT32(&DSI_REG[i]->DSI_HFP_WC));
			v_a = hsa.HSA_WC + hbp.HBP_WC + new_hfp.HFP_WC + ps.DSI_PS_WC + 32;
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSTX_CKL_WC, (v_a - v_b));
			DSI_OUTREG32(cmdq, &new_hstx_ckl_wc, AS_UINT32(&DSI_REG[i]->DSI_HSTX_CKL_WC));
			DISPDBG("===>new HSTX_CKL_WC=0x%x, HFP_WC=0x%x\n", new_hstx_ckl_wc,	new_hfp.HFP_WC);
		}
		/* 2. sync_event_mode */
		/* Total    WC(A) = HBP_WC + HFP_WC + PS_WC + 26 */
		/* CLK init WC(B) = (CLK_HS_EXIT + LPX + CLK_HS_PREP + CLK_HS_ZERO)*lane_num */
		/* CLK end  WC(C) = (CLK_HS_POST + CLK_HS_TRAIL)*lane_num */
		/* HSTX_CKLP_WC = A - B */
		/* Limitation: B + C < HFP_WC */
		else if (dsi_mode == SYNC_EVENT_VDO_MODE) {
			v_a = hbp.HBP_WC + hfp.HFP_WC + ps.DSI_PS_WC + 26;
			v_b =
			    (timcon3.CLK_HS_EXIT + timcon0.LPX + timcon3.CLK_HS_PRPR + timcon2.CLK_ZERO) * lane_num;
			v_c = (timcon3.CLK_HS_POST + timcon2.CLK_TRAIL) * lane_num;

			DISPDBG("===>v_a-v_b=0x%x,HSTX_CKLP_WC=0x%x\n", (v_a - v_b), hstx_ckl_wc);
/* DISPDBG("===>v_b+v_c=0x%x,HFP_WC=0x%x\n",(v_b+v_c),hfp); */
			DISPDBG
			    ("===>Will Reconfig in order to fulfill LP clock lane per line\n");

			/* B+C < HFP ,here diff is 0x10; */
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HFP_WC, (v_b + v_c + DIFF_CLK_LANE_LP));
			DSI_OUTREG32(cmdq, &new_hfp, AS_UINT32(&DSI_REG[i]->DSI_HFP_WC));
			v_a = hbp.HBP_WC + new_hfp.HFP_WC + ps.DSI_PS_WC + 26;
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSTX_CKL_WC, (v_a - v_b));
			DSI_OUTREG32(cmdq, &new_hstx_ckl_wc, AS_UINT32(&DSI_REG[i]->DSI_HSTX_CKL_WC));
			DISPDBG("===>new HSTX_CKL_WC=0x%x, HFP_WC=0x%x\n", new_hstx_ckl_wc, new_hfp.HFP_WC);

		}
		/* 3. burst_mode */
		/* Total    WC(A) = HBP_WC + HFP_WC + PS_WC + BLLP_WC + 32 */
		/* CLK init WC(B) = (CLK_HS_EXIT + LPX + CLK_HS_PREP + CLK_HS_ZERO)*lane_num */
		/* CLK end  WC(C) = (CLK_HS_POST + CLK_HS_TRAIL)*lane_num */
		/* HSTX_CKLP_WC = A - B */
		/* Limitation: B + C < HFP_WC */
		else if (dsi_mode == BURST_VDO_MODE) {
			v_a = hbp.HBP_WC + hfp.HFP_WC + ps.DSI_PS_WC + bllp.BLLP_WC + 32;
			v_b =
			    (timcon3.CLK_HS_EXIT + timcon0.LPX + timcon3.CLK_HS_PRPR + timcon2.CLK_ZERO) * lane_num;
			v_c = (timcon3.CLK_HS_POST + timcon2.CLK_TRAIL) * lane_num;

			DISPDBG("===>v_a-v_b=0x%x,HSTX_CKLP_WC=0x%x\n", (v_a - v_b), hstx_ckl_wc);
			/* DISPDBG("===>v_b+v_c=0x%x,HFP_WC=0x%x\n",(v_b+v_c),hfp); */
			DISPDBG
			    ("===>Will Reconfig in order to fulfill LP clock lane per line\n");

			/* B+C < HFP ,here diff is 0x10; */
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HFP_WC, (v_b + v_c + DIFF_CLK_LANE_LP));
			DSI_OUTREG32(cmdq, &new_hfp, AS_UINT32(&DSI_REG[i]->DSI_HFP_WC));
			v_a =
			    hbp.HBP_WC + new_hfp.HFP_WC + ps.DSI_PS_WC + bllp.BLLP_WC + 32;
			DSI_OUTREG32(cmdq, &DSI_REG[i]->DSI_HSTX_CKL_WC, (v_a - v_b));
			DSI_OUTREG32(cmdq, &new_hstx_ckl_wc, AS_UINT32(&DSI_REG[i]->DSI_HSTX_CKL_WC));
			DISPDBG("===>new HSTX_CKL_WC=0x%x, HFP_WC=0x%x\n", new_hstx_ckl_wc,	new_hfp.HFP_WC);
		}
	}

}

int ddp_dsi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *cmdq)
{
	int i = 0;
	LCM_DSI_PARAMS *dsi_config = &(config->dispif_config.dsi);

	if (!config->dst_dirty) {
		if (atomic_read(&PMaster_enable) == 0)
			return 0;
	}
	/* DISPFUNC(); */
	/* DISPDBG("===>run here 00 Pmaster: clk:%d\n",_dsi_context[0].dsi_params.PLL_CLOCK); */

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		_copy_dsi_params(dsi_config, &(_dsi_context[i].dsi_params));
		_dsi_context[i].lcm_width = config->dst_w;
		_dsi_context[i].lcm_height = config->dst_h;
		/* _dump_dsi_params(&(_dsi_context[i].dsi_params)); */
		/* cmd mode enable te here */
		if (dsi_config->mode == CMD_MODE)
			DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[i]->DSI_INTEN,
				      TE_RDY, 1);
	}
	/* DISPDBG("===>01Pmaster: clk:%d\n",_dsi_context[0].dsi_params.PLL_CLOCK); */
	if (dsi_config->mode != CMD_MODE)
		dsi_currect_mode = 1;

#ifndef CONFIG_FPGA_EARLY_PORTING
	if ((MIPITX_IsEnabled(module, cmdq)) && (atomic_read(&PMaster_enable) == 0)) {
		/* DISPDBG("mipitx is already init\n"); */
		if (dsi_force_config)
			goto force_config;
		else
			goto done;

	} else
#endif
	{
		DISPDBG("MIPITX is not inited, will config mipitx clock now\n");
		DISPDBG("===>Pmaster:CLK SETTING??==> clk:%d\n",
			_dsi_context[0].dsi_params.PLL_CLOCK);
		DSI_PHY_clk_setting(module, NULL, dsi_config);
	}
force_config:
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (dsi_config->mode == CMD_MODE
		    || ((dsi_config->switch_mode_enable == 1)
			&& (dsi_config->switch_mode == CMD_MODE)))
			DSI_OUTREGBIT(cmdq, struct DSI_INT_ENABLE_REG, DSI_REG[i]->DSI_INTEN, TE_RDY, 1);
	}
	/* DSI_Reset(module, cmdq_handle); */
	DSI_TXRX_Control(module, cmdq, dsi_config);
	DSI_PS_Control(module, cmdq, dsi_config, config->dst_w, config->dst_h);
	DSI_PHY_TIMCONFIG(module, cmdq, dsi_config);

	if (dsi_config->mode != CMD_MODE
	    || ((dsi_config->switch_mode_enable == 1)
		&& (dsi_config->switch_mode != CMD_MODE))) {
		DSI_Config_VDO_Timing(module, cmdq, dsi_config);
		DSI_Set_VM_CMD(module, cmdq);
	}
#if 0
	/* TODO: workaround for 8 lane left/right mode wqhd lcm panel */
	if (module == DISP_MODULE_DSIDUAL) {
		DSI_OUTREG32(cmdq, 0xF401A050, config->dst_w);
		DSI_OUTREG32(cmdq, 0xF401A054, config->dst_h);
		DSI_OUTREG32(cmdq, 0xF401A000, 9);
	}
#endif
	/* Enable clk low power per Line ; */
	if (dsi_config->clk_lp_per_line_enable)
		DSI_PHY_CLK_LP_PerLine_config(module, cmdq, dsi_config);



done:

	return 0;
}

int ddp_dsi_start(DISP_MODULE_ENUM module, void *cmdq)
{
	int i = 0;

	DISPFUNC();
#if 0
	if (module == DISP_MODULE_DSIDUAL) {
		/* must set DSI_START to 0 before set dsi_dual_en, don't know why.2014.02.15 */
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[0]->DSI_START, DSI_START, 0);
		DSI_OUTREGBIT(cmdq, struct DSI_START_REG, DSI_REG[1]->DSI_START, DSI_START, 0);

		DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[0]->DSI_COM_CTRL, DSI_DUAL_EN,
			      1);
		DSI_OUTREGBIT(cmdq, struct DSI_COM_CTRL_REG, DSI_REG[1]->DSI_COM_CTRL, DSI_DUAL_EN,
			      1);

		DSI_SetMode(module, cmdq, _dsi_context[i].dsi_params.mode);
		DSI_clk_HS_mode(module, cmdq, true);
	} else
#endif
	{
		DSI_Send_ROI(module, cmdq, 0, 0, _dsi_context[i].lcm_width, _dsi_context[i].lcm_height);
		DSI_SetMode(module, cmdq, _dsi_context[i].dsi_params.mode);
		DSI_clk_HS_mode(module, cmdq, true);
	}

	return 0;
}

int ddp_dsi_stop(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int i = 0;
	unsigned int tmp = 0;

	DISPFUNC();
	/* ths caller should call wait_event_or_idle for frame stop event then. */
	/* DSI_SetMode(module, cmdq_handle, CMD_MODE); */

	if (_dsi_is_video_mode(module)) {
		DISPMSG("dsi is video mode\n");
		DSI_SetMode(module, cmdq_handle, CMD_MODE);

		i = DSI_MODULE_BEGIN(module);
		while (1) {
			tmp = INREG32(&DSI_REG[i]->DSI_INTSTA);
			if (!(tmp & 0x80000000))
				break;
		}

		i = DSI_MODULE_END(module);
		while (1) {
			DISPMSG("dsi%d is busy\n", i);
			tmp = INREG32(&DSI_REG[i]->DSI_INTSTA);
			if (!(tmp & 0x80000000))
				break;
		}

		if (module == DISP_MODULE_DSIDUAL) {
			DSI_OUTREGBIT(cmdq_handle, struct DSI_COM_CTRL_REG, DSI_REG[0]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
			DSI_OUTREGBIT(cmdq_handle, struct DSI_COM_CTRL_REG, DSI_REG[1]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
			DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG, DSI_REG[0]->DSI_START, DSI_START, 0);
			DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG, DSI_REG[1]->DSI_START, DSI_START, 0);
		}
		DSI_clk_HSLP_mode(module, cmdq_handle);
	} else {
		DISPMSG("dsi is cmd mode\n");
		/* TODO: modify this with wait event */
		DSI_WaitForNotBusy(module, cmdq_handle);
		DSI_clk_HS_mode(module, cmdq_handle, false);
	}
	return 0;
}

/*TUI will use the api*/
int dsi_enable_irq(DISP_MODULE_ENUM module, void *handle, unsigned int enable)
{
	 if (module == DISP_MODULE_DSI0)
		DSI_OUTREGBIT(handle, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN, FRAME_DONE_INT_EN, enable);

	return 0;
}

int ddp_dsi_switch_lcm_mode(DISP_MODULE_ENUM module, void *params)
{
	int i = 0;
	LCM_DSI_MODE_SWITCH_CMD lcm_cmd = *((LCM_DSI_MODE_SWITCH_CMD *) (params));
	int mode = (int)(lcm_cmd.mode);

	if (dsi_currect_mode == mode) {
		DISPMSG
		    ("[ddp_dsi_switch_mode] not need switch mode, current mode = %d, switch to %d\n",
		     dsi_currect_mode, mode);
		return 0;
	}
	if (lcm_cmd.cmd_if == LCM_INTERFACE_DSI0)
		i = 0;
	else if (lcm_cmd.cmd_if == LCM_INTERFACE_DSI1)
		i = 1;
	else {
		DISPERR("dsi switch not support this cmd IF:%d\n", lcm_cmd.cmd_if);
		return -1;
	}

	if (mode == 0) {	/* V2C */
/* DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG,DSI_REG[i]->DSI_INTEN,EXT_TE,1); */
		DSI_OUTREG32(NULL, (unsigned long)(DSI_REG[i]) + 0x130,
			0x00001521 | (lcm_cmd.addr << 16) | (lcm_cmd.val[0] << 24));	/* RM = 1 */
		DSI_OUTREGBIT(NULL, struct DSI_START_REG, DSI_REG[i]->DSI_START, VM_CMD_START, 0);
		DSI_OUTREGBIT(NULL, struct DSI_START_REG, DSI_REG[i]->DSI_START, VM_CMD_START, 1);
		wait_vm_cmd_done = false;
		wait_event_interruptible(_dsi_wait_vm_cmd_done_queue[i], wait_vm_cmd_done);
#if 0
		DSI_OUTREG32(NULL, (unsigned long)(DSI_REG[i]) + 0x130,
			0x00001539 | (lcm_cmd.addr << 16) | (lcm_cmd.val[1] << 24));	/* DM = 0 */
		DSI_OUTREGBIT(NULL, struct DSI_START_REG, DSI_REG[i]->DSI_START, VM_CMD_START, 0);
		DSI_OUTREGBIT(NULL, struct DSI_START_REG, DSI_REG[i]->DSI_START, VM_CMD_START, 1);
		wait_vm_cmd_done = false;
		wait_event_interruptible(_dsi_wait_vm_cmd_done_queue[i], wait_vm_cmd_done);
#endif
	}
	return 0;
}

int ddp_dsi_switch_mode(DISP_MODULE_ENUM module, void *cmdq_handle, void *params)
{
	int i = 0;
	LCM_DSI_MODE_SWITCH_CMD lcm_cmd = *((LCM_DSI_MODE_SWITCH_CMD *) (params));
	int mode = (int)(lcm_cmd.mode);

	if (dsi_currect_mode == mode) {
		DISPMSG
		    ("[ddp_dsi_switch_mode] not need switch mode, current mode = %d, switch to %d\n",
		     dsi_currect_mode, mode);
		return 0;
	}
	if (lcm_cmd.cmd_if == LCM_INTERFACE_DSI0)
		i = 0;
	else if (lcm_cmd.cmd_if == LCM_INTERFACE_DSI1)
		i = 1;
	else {
		DISPERR("dsi switch not support this cmd IF:%d\n", lcm_cmd.cmd_if);
		return -1;
	}

	if (mode == 0) {	/* V2C */
#if 1
		DSI_SetSwitchMode(module, cmdq_handle, 0);	/*  */
		DSI_OUTREG32(cmdq_handle, (unsigned long)(DSI_REG[i]) + 0x130,
			0x00001539 | (lcm_cmd.addr << 16) | (lcm_cmd.val[1] << 24));	/* DM = 0 */
		DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG, DSI_REG[i]->DSI_START,
			      VM_CMD_START, 0);
		DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG, DSI_REG[i]->DSI_START,
			      VM_CMD_START, 1);
		DSI_MASKREG32(cmdq_handle, 0xF4020028, 0x1, 0x1);	/* reset mutex for V2C */
		DSI_MASKREG32(cmdq_handle, 0xF4020028, 0x1, 0x0);	/*  */
		DSI_MASKREG32(cmdq_handle, 0xF4020030, 0x1, 0x0);	/* mutext to cmd  mode */
		cmdqRecFlush(cmdq_handle);
		cmdqRecReset(cmdq_handle);
		cmdqRecWaitNoClear(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
		DSI_SetMode(module, NULL, 0);
/* DSI_SetBypassRack(module, NULL, 0); */
#else
		DSI_SetSwitchMode(module, cmdq_handle, 0);	/*  */
		DSI_MASKREG32(cmdq_handle, 0xF4020028, 0x1, 0x1);	/* reset mutex for V2C */
		DSI_MASKREG32(cmdq_handle, 0xF4020028, 0x1, 0x0);	/*  */
		DSI_MASKREG32(cmdq_handle, 0xF4020030, 0x1, 0x0);	/* mutext to cmd  mode */
		cmdqRecFlush(cmdq_handle);
		cmdqRecReset(cmdq_handle);
		cmdqRecWaitNoClear(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
		DSI_SetMode(module, NULL, 0);
/* DSI_SetBypassRack(module, NULL, 0); */

#endif
	} else {	/* C2V */
#if 1
		DSI_SetMode(module, cmdq_handle, mode);
		DSI_SetSwitchMode(module, cmdq_handle, 1);	/* EXT TE could not use C2V */
		DSI_MASKREG32(cmdq_handle, 0xF4020030, 0x1, 0x1);	/* mutext to video mode */
		DSI_OUTREG32(cmdq_handle, (unsigned long)(DSI_REG[i]) + 0x200 + 0,
			     0x00001500 | (lcm_cmd.addr << 16) | (lcm_cmd.val[0] << 24));
		DSI_OUTREG32(cmdq_handle, (unsigned long)(DSI_REG[i]) + 0x200 + 4,
			     0x00000020);
		DSI_OUTREG32(cmdq_handle, (unsigned long)(DSI_REG[i]) + 0x60, 2);
		DSI_Start(module, cmdq_handle);	/* ???????????????????????????????? */
		DSI_MASKREG32(NULL, 0xF4020020, 0x1, 0x1);	/* release mutex for video mode */
		cmdqRecFlush(cmdq_handle);
		cmdqRecReset(cmdq_handle);
		cmdqRecWaitNoClear(cmdq_handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
#else
/* DSI_WaitForNotBusy(module,NULL); */
		DSI_SetMode(module, NULL, mode);
/* DSI_SetBypassRack(module,NULL,1); */
		DSI_SetSwitchMode(module, NULL, 1);	/* EXT TE could not use C2V */
		DSI_MASKREG32(NULL, 0xF4020030, 0x1, 0x1);	/* mutext to video mode */
		DSI_OUTREG32(NULL, (unsigned int)(DSI_REG[i]) + 0x200 + 0,
			     0x00001500 | (lcm_cmd.addr << 16) | (lcm_cmd.val[0] << 24));
		DSI_OUTREG32(NULL, (unsigned int)(DSI_REG[i]) + 0x204 + 0, 0x00000020);
		DSI_OUTREG32(NULL, (unsigned int)(DSI_REG[i]) + 0x60, 2);
		DSI_Start(module, NULL);	/* ???????????????????????????????? */
		DSI_MASKREG32(NULL, 0xF4020020, 0x1, 0x1);	/* release mutex for video mode */
#endif
	}
	dsi_currect_mode = mode;
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
		_dsi_context[i].dsi_params.mode = mode;

	return 0;
}

int ddp_dsi_clk_on(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int level)
{
	int ret = 0;
#ifndef CONFIG_MTK_CLKMGR
	if (level > 0)
		ddp_set_mipi26m(1);
#endif

	if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
		ret += enable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
		ret += enable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
		ret += ddp_clk_enable(DISP1_DSI_ENGINE);
		ret += ddp_clk_enable(DISP1_DSI_DIGITAL);
#endif

		if (ret > 0)
			DISPERR("DSI power manager API return false\n");

	}
	if (level > 0)
		DSI_PHY_clk_switch(module, NULL, true);

	/* DISPMSG("ddp_dsi_clk_on.\n"); */

	return ret;
}

int ddp_dsi_clk_off(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int level)
{
	int ret = 0;

	if (level > 0)
		DSI_PHY_clk_switch(module, NULL, false);


	if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
		ret += disable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
		ret += disable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
		ddp_clk_disable(DISP1_DSI_ENGINE);
		ddp_clk_disable(DISP1_DSI_DIGITAL);
#endif

		if (ret > 0)
			DISPERR("DSI power manager API return false\n");

	}
	/* DISPMSG("ddp_dsi_clk_off.\n"); */
#ifndef CONFIG_MTK_CLKMGR
	if (level > 0)
		ddp_set_mipi26m(0);
#endif

	return ret;
}

int ddp_dsi_ioctl(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int ioctl_cmd,
		  unsigned long *params) {
	int ret = 0;
	/* DISPFUNC(); */
	DDP_IOCTL_NAME ioctl = (DDP_IOCTL_NAME) ioctl_cmd;
	/* DISPMSG("[ddp_dsi_ioctl] index = %d\n", ioctl); */
	switch (ioctl) {
	case DDP_STOP_VIDEO_MODE:
		{
			/* ths caller should call wait_event_or_idle for frame stop event then. */
			DSI_SetMode(module, cmdq_handle, CMD_MODE);
			/* TODO: modify this with wait event */

			if (0 != DSI_WaitVMDone(module))
				ret = -1;

			if (module == DISP_MODULE_DSIDUAL) {
				DSI_OUTREGBIT(cmdq_handle, struct DSI_COM_CTRL_REG,
					      DSI_REG[0]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
				DSI_OUTREGBIT(cmdq_handle, struct DSI_COM_CTRL_REG,
					      DSI_REG[1]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
				DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG,
					      DSI_REG[0]->DSI_START, DSI_START, 0);
				DSI_OUTREGBIT(cmdq_handle, struct DSI_START_REG,
					      DSI_REG[1]->DSI_START, DSI_START, 0);
			}
			break;
		}

	case DDP_SWITCH_DSI_MODE:
		{
			ret = ddp_dsi_switch_mode(module, cmdq_handle, params);
			break;
		}
	case DDP_SWITCH_LCM_MODE:
		{
			ret = ddp_dsi_switch_lcm_mode(module, params);
			break;
		}
	case DDP_BACK_LIGHT:
		{
			unsigned int cmd = 0x51;
			unsigned int count = 1;
			unsigned int level = params[0];

			DISPMSG("[ddp_dsi_ioctl] level = %d\n", level);
			DSI_set_cmdq_V2(module, cmdq_handle, cmd, count,
					((unsigned char *)&level), 1);
			break;
		}
	case DDP_DSI_IDLE_CLK_CLOSED:
		{
			unsigned int idle_cmd = params[0];

			/* DISPMSG("[ddp_dsi_ioctl_close] level = %d\n", idle_cmd); */
			if (idle_cmd == 0)
				ddp_dsi_clk_off(module, cmdq_handle, 0);
			else
				ddp_dsi_clk_off(module, cmdq_handle, idle_cmd);

			break;
		}
	case DDP_DSI_IDLE_CLK_OPEN:
		{
			unsigned int idle_cmd = params[0];
			/* DISPMSG("[ddp_dsi_ioctl_open] level = %d\n", idle_cmd); */
			if (idle_cmd == 0)
				ddp_dsi_clk_on(module, cmdq_handle, 0);
			else
				ddp_dsi_clk_on(module, cmdq_handle, idle_cmd);


			break;
		}
	case DDP_DSI_VFP_LP:
		{
			unsigned int vertical_frontporch = *((unsigned int *)params);

			DISPMSG("vertical_frontporch=%d.\n", vertical_frontporch);
			if (module == DISP_MODULE_DSI0) {
				DSI_OUTREG32(cmdq_handle, &DSI_REG[0]->DSI_VFP_NL,
					     vertical_frontporch);
			}
			break;
		}
	case DDP_DPI_FACTORY_TEST:
		{
			break;
		}
	case DDP_DSI_ENABLE_TE:
		{
			DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[0]->DSI_INTEN,
				      TE_RDY, 1);
			break;
		}
	}
	return ret;
}

int ddp_dsi_trigger(DISP_MODULE_ENUM module, void *cmdq)
{
	int i = 0;
	unsigned int data_array[16];

	if (_dsi_context[i].dsi_params.mode == CMD_MODE) {
		data_array[0] = 0x002c3909;
		DSI_set_cmdq(module, cmdq, data_array, 1, 0);
	}
	DSI_Start(module, cmdq);

	return 0;
}

int ddp_dsi_reset(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DSI_Reset(module, cmdq_handle);

	return 0;
}


int ddp_dsi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int i = 0;
	int ret = 0;

	DISPFUNC();

	/* DSI_DumpRegisters(module,1); */
	if (!s_isDsiPowerOn) {
#ifdef ENABLE_CLK_MGR
#ifndef CONFIG_MTK_CLKMGR
		ddp_set_mipi26m(1);
#endif

		if (is_ipoh_bootup) {
			if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
				ret += enable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
				ret += enable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
				ret += ddp_clk_enable(DISP1_DSI_ENGINE);
				ret += ddp_clk_enable(DISP1_DSI_DIGITAL);
#endif

				if (ret > 0)
					DISPERR("DSI power manager API return false\n");

			}
			s_isDsiPowerOn = true;
			DISPMSG("ipoh dsi power on return\n");
			return DSI_STATUS_OK;
		}
		DSI_PHY_clk_switch(module, NULL, true);

		if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
			ret += enable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
			ret += enable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
			ret += ddp_clk_enable(DISP1_DSI_ENGINE);
			ret += ddp_clk_enable(DISP1_DSI_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI power manager API return false\n");

		}
#if 0
		if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
			ret += enable_clock(MT_CG_DISP1_DSI1_ENGINE, "DSI1");
			ret += enable_clock(MT_CG_DISP1_DSI1_DIGITAL, "DSI1");
#else
			ret += ddp_clk_enable(DISP1_DSI1_ENGINE);
			ret += ddp_clk_enable(DISP1_DSI1_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI1 power manager API return false\n");

		}
#endif

		/* restore dsi register */
		DSI_RestoreRegisters(module, NULL);

		/* enable sleep-out mode */
		DSI_SleepOut(module, NULL);

		/* if mtcmos is not power down, the original ulps state machine keep the ulps state, */
		/* we should finish the flow, but not show on mipi interface,
		 * because the mipi interface is already controlled by new sleep state machine
		 */
		DSI_OUTREGBIT(NULL, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
			      L0_ULPM_EN, 0);
		DSI_OUTREGBIT(NULL, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
			      L0_WAKEUP_EN, 1);
		DSI_OUTREGBIT(NULL, struct DSI_PHY_LD0CON_REG, DSI_REG[i]->DSI_PHY_LD0CON,
			      L0_WAKEUP_EN, 0);

		/* restore lane_num */
		{
			volatile struct DSI_REGS *regs = NULL;

			regs = &(_dsi_context[0].regBackup);
			DSI_OUTREG32(NULL, &DSI_REG[0]->DSI_TXRX_CTRL,
				     AS_UINT32(&regs->DSI_TXRX_CTRL));
		}
		/* enter wakeup */
		DSI_Wakeup(module, NULL);

		/* enable clock */
		DSI_EnableClk(module, NULL);

		DSI_Reset(module, NULL);
		if (module == DISP_MODULE_DSIDUAL) {
			DSI_OUTREG32(NULL, 0xF401A050, _dsi_context[i].lcm_width);
			DSI_OUTREG32(NULL, 0xF401A054, _dsi_context[i].lcm_height);
			DSI_OUTREG32(NULL, 0xF401A000, 9);
		}
#endif
		s_isDsiPowerOn = true;
	}
	/* DSI_DumpRegisters(module,1); */
	return DSI_STATUS_OK;
}


int ddp_dsi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int i = 0;
	int ret = 0;
	unsigned int value = 0;

	DISPFUNC();
	/* DSI_DumpRegisters(module,1); */

	if (s_isDsiPowerOn) {
		/* disable te_rdy */
		for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
			/* cmd mode enable te here */
			DSI_OUTREGBIT(NULL, struct DSI_INT_ENABLE_REG, DSI_REG[i]->DSI_INTEN,
				      TE_RDY, 0);
		}


		DSI_BackupRegisters(module, NULL);
#ifdef ENABLE_CLK_MGR
		/* disable HS mode */
		DSI_clk_HS_mode(module, NULL, false);
		/* enter ULPS mode */
		DSI_lane0_ULP_mode(module, NULL, 1);
		DSI_clk_ULP_mode(module, NULL, 1);

		/* make sure enter ulps mode */
		while (1) {
			mdelay(1);
			value = INREG32(&DSI_REG[0]->DSI_STATE_DBG1);
			value = value >> 24;
			if (value == 0x20)
				break;

		}
		/* clear lane_num when enter ulps */
		DSI_OUTREGBIT(NULL, struct DSI_TXRX_CTRL_REG, DSI_REG[0]->DSI_TXRX_CTRL, LANE_NUM, 0);

		/* disable clock */
		DSI_DisableClk(module, NULL);

		if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
			ret += disable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI");
			ret += disable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI");
#else
			ddp_clk_disable(DISP1_DSI_ENGINE);
			ddp_clk_disable(DISP1_DSI_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI power manager API return false\n");

		}
#if 0
		if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL) {
#ifdef CONFIG_MTK_CLKMGR
			ret += disable_clock(MT_CG_DISP1_DSI1_ENGINE, "DSI1");
			ret += disable_clock(MT_CG_DISP1_DSI1_DIGITAL, "DSI1");
#else
			ddp_clk_disable(DISP1_DSI1_ENGINE);
			ddp_clk_disable(DISP1_DSI1_DIGITAL);
#endif

			if (ret > 0)
				DISPERR("DSI1 power manager API return false\n");

		}
#endif
		/* disable mipi pll */
		DSI_PHY_clk_switch(module, NULL, false);
#ifndef CONFIG_MTK_CLKMGR
		ddp_set_mipi26m(0);
#endif
#endif
		s_isDsiPowerOn = false;
	}
	/* DSI_DumpRegisters(module,1); */
	return DSI_STATUS_OK;
}


int ddp_dsi_is_busy(DISP_MODULE_ENUM module)
{
	int i = 0;
	int busy = 0;
	struct DSI_INT_STATUS_REG status;
	/* DISPFUNC(); */

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		status = DSI_REG[i]->DSI_INTSTA;

		if (status.BUSY)
			busy++;
	}

	DISPDBG("%s is %s\n", ddp_get_module_name(module), busy ? "busy" : "idle");
	return busy;
}

int ddp_dsi_is_idle(DISP_MODULE_ENUM module)
{
	return !ddp_dsi_is_busy(module);
}

static const char *dsi_mode_spy(LCM_DSI_MODE_CON mode)
{
	switch (mode) {
	case CMD_MODE:
		return "CMD_MODE";
	case SYNC_PULSE_VDO_MODE:
		return "SYNC_PULSE_VDO_MODE";
	case SYNC_EVENT_VDO_MODE:
		return "SYNC_EVENT_VDO_MODE";
	case BURST_VDO_MODE:
		return "BURST_VDO_MODE";
	default:
		return "unknown";
	}
}

void dsi_analysis(DISP_MODULE_ENUM module)
{
	int i = 0;

	DISPMSG("==DISP DSI ANALYSIS==\n");
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DISPMSG
		    ("DSI%d Start:%x, Busy:%d, DSI_DUAL_EN:%d, MODE:%s, High Speed:%d, FSM State:%s\n",
		     i, DSI_REG[i]->DSI_START.DSI_START, DSI_REG[i]->DSI_INTSTA.BUSY,
		     DSI_REG[i]->DSI_COM_CTRL.DSI_DUAL_EN,
		     dsi_mode_spy(DSI_REG[i]->DSI_MODE_CTRL.MODE),
		     DSI_REG[i]->DSI_PHY_LCCON.LC_HS_TX_EN,
		     _dsi_cmd_mode_parse_state(DSI_REG[i]->DSI_STATE_DBG6.CMTRL_STATE));

		DISPMSG
		    ("DSI%dIRQ,RD_RDY:%d,CMD_DONE:%d,SLEEPOUT_DONE:%d,TE_RDY:%d,VM_CMD_DONE:%d,VM_DONE:%d\n",
		     i, DSI_REG[i]->DSI_INTSTA.RD_RDY, DSI_REG[i]->DSI_INTSTA.CMD_DONE,
		     DSI_REG[i]->DSI_INTSTA.SLEEPOUT_DONE, DSI_REG[i]->DSI_INTSTA.TE_RDY,
		     DSI_REG[i]->DSI_INTSTA.VM_CMD_DONE, DSI_REG[i]->DSI_INTSTA.VM_DONE);

		DISPMSG
		    ("DSI%d Lane Num:%d, Ext_TE_EN:%d, Ext_TE_Edge:%d, HSTX_CKLP_EN:%d\n",
		     i, DSI_REG[i]->DSI_TXRX_CTRL.LANE_NUM,
		     DSI_REG[i]->DSI_TXRX_CTRL.EXT_TE_EN,
		     DSI_REG[i]->DSI_TXRX_CTRL.EXT_TE_EDGE,
		     DSI_REG[i]->DSI_TXRX_CTRL.HSTX_CKLP_EN);
	}
}

int ddp_dsi_dump(DISP_MODULE_ENUM module, int level)
{
	dsi_analysis(module);
	DSI_DumpRegisters(module, level);
	return 0;
}

int ddp_dsi_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{
	int ret = 0;
	int i = 0;
	int dsi_i = 0;
	LCM_DSI_PARAMS *dsi_params = NULL;
	DSI_T0_INS t0;
	struct DSI_RX_DATA_REG read_data0;

	static cmdqBackupSlotHandle hSlot;

	if (DISP_MODULE_DSIDUAL == module)
		dsi_i = 0;
	else
		dsi_i = DSI_MODULE_to_ID(module);

	dsi_params = &_dsi_context[dsi_i].dsi_params;

	if (cmdq_trigger_handle == NULL) {
		DISPMSG("cmdq_trigger_handle is NULL\n");
		return -1;
	}

	if (state == CMDQ_BEFORE_STREAM_SOF) {
		/* need waiting te */
		if (module == DISP_MODULE_DSI0) {
			if (dsi0_te_enable == 0)
				return 0;
#ifndef MTK_FB_CMDQ_DISABLE
			ret =
			    cmdqRecClearEventToken(cmdq_trigger_handle, CMDQ_EVENT_DSI_TE);
			ret = cmdqRecWait(cmdq_trigger_handle, CMDQ_EVENT_DSI_TE);
#endif
		}
#if 0
		else if (module == DISP_MODULE_DSI1) {
			if (dsi1_te_enable == 0)
				return 0;

			ret =
			    cmdqRecClearEventToken(cmdq_trigger_handle,
						   CMDQ_EVENT_MDP_DSI1_TE_SOF);
			ret = cmdqRecWait(cmdq_trigger_handle, CMDQ_EVENT_MDP_DSI1_TE_SOF);
		} else if (module == DISP_MODULE_DSIDUAL) {
			if (dsidual_te_enable == 0)
				return 0;

			/* TODO: dsi 8 lane do not use te???? */
			/* ret = cmdqRecWait(cmdq_trigger_handle, CMDQ_EVENT_MDP_DSI0_TE_SOF); */
		}
#endif
		else {
			DISPERR("wrong module: %s\n", ddp_get_module_name(module));
			return -1;
		}
	} else if (state == CMDQ_CHECK_IDLE_AFTER_STREAM_EOF) {
		/* need waiting te */
		if (module == DISP_MODULE_DSI0) {
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[dsi_i]->DSI_INTSTA,
				      0x80000000, 0);
		}
#if 0
		else if (module == DISP_MODULE_DSI1) {
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[dsi_i]->DSI_INTSTA,
				      0x80000000, 0);
		} else if (module == DISP_MODULE_DSIDUAL) {
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[0]->DSI_INTSTA,
				      0x80000000, 0);
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[1]->DSI_INTSTA,
				      0x80000000, 0);
		}
#endif
		else {
			DISPERR("wrong module: %s\n", ddp_get_module_name(module));
			return -1;
		}
	} else if (state == CMDQ_ESD_CHECK_READ) {
		/* enable dsi interrupt: RD_RDY/CMD_DONE (need do this here?) */
		DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_ENABLE_REG,
			      DSI_REG[dsi_i]->DSI_INTEN, RD_RDY, 1);
		DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_ENABLE_REG,
			      DSI_REG[dsi_i]->DSI_INTEN, CMD_DONE, 1);

		for (i = 0; i < 3; i++) {
			if (dsi_params->lcm_esd_check_table[i].cmd == 0)
				break;

			/* 0. send read lcm command(short packet) */
			t0.CONFG = 0x04;	/* BTA */
			t0.Data0 = dsi_params->lcm_esd_check_table[i].cmd;
			/* / 0xB0 is used to distinguish DCS cmd or Gerneric cmd, is that Right??? */
			t0.Data_ID =
			    (t0.Data0 <
			     0xB0) ? DSI_DCS_READ_PACKET_ID :
			    DSI_GERNERIC_READ_LONG_PACKET_ID;
			t0.Data1 = 0;

			/* write DSI CMDQ */
			DSI_OUTREG32(cmdq_trigger_handle, &DSI_CMDQ_REG[dsi_i]->data[0],
				     0x00013700);
			DSI_OUTREG32(cmdq_trigger_handle, &DSI_CMDQ_REG[dsi_i]->data[1],
				     AS_UINT32(&t0));
			DSI_OUTREG32(cmdq_trigger_handle, &DSI_REG[dsi_i]->DSI_CMDQ_SIZE,
				     2);

			/* start DSI */
			DSI_OUTREG32(cmdq_trigger_handle, &DSI_REG[dsi_i]->DSI_START, 0);
			DSI_OUTREG32(cmdq_trigger_handle, &DSI_REG[dsi_i]->DSI_START, 1);

			/* 1. wait DSI RD_RDY(must clear, in case of cpu RD_RDY interrupt handler) */
			if (dsi_i == 0) {	/* DSI0 */
				DSI_POLLREG32(cmdq_trigger_handle,
					      &DSI_REG[dsi_i]->DSI_INTSTA, 0x00000001, 0x1);
				DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_STATUS_REG,
					      DSI_REG[dsi_i]->DSI_INTSTA, RD_RDY, 0);
			}
#if 0
			else {	/* DSI1 */
				DSI_POLLREG32(cmdq_trigger_handle,
					      &DSI_REG[dsi_i]->DSI_INTSTA, 0x00000001, 0x1);
				DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_STATUS_REG,
					      DSI_REG[dsi_i]->DSI_INTSTA, RD_RDY, 0);
			}
#endif
			/* 2. save RX data */
			if (hSlot) {
				DSI_BACKUPREG32(cmdq_trigger_handle, hSlot, i,
						&DSI_REG[0]->DSI_RX_DATA0);
			}

			/* 3. write RX_RACK */
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_RACK_REG,
				      DSI_REG[dsi_i]->DSI_RACK, DSI_RACK, 1);

			/* 4. polling not busy(no need clear) */
			if (dsi_i == 0) {	/* DSI0 */
				DSI_POLLREG32(cmdq_trigger_handle,
					      &DSI_REG[dsi_i]->DSI_INTSTA, 0x80000000, 0);
			}
#if 0
			else {	/* DSI1 */
				DSI_POLLREG32(cmdq_trigger_handle,
					      &DSI_REG[dsi_i]->DSI_INTSTA, 0x80000000, 0);
			}
#endif
			/* loop: 0~4 */
		}

		/* DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_ENABLE_REG,DSI_REG[dsi_i]->DSI_INTEN,RD_RDY,0); */
	} else if (state == CMDQ_ESD_CHECK_CMP) {

		DISPMSG("[DSI]enter cmp\n");
		/* cmp just once and only 1 return value */
		for (i = 0; i < 3; i++) {
			if (dsi_params->lcm_esd_check_table[i].cmd == 0)
				break;

			DISPMSG("[DSI]enter cmp i=%d\n", i);

			/* read data */
			if (hSlot) {
				/* read from slot */
				cmdqBackupReadSlot(hSlot, i, ((uint32_t *) &read_data0));
			} else {
				/* read from dsi , support only one cmd read */
				if (i == 0) {
					DSI_OUTREG32(NULL, &read_data0,
						     AS_UINT32(&DSI_REG[dsi_i]->DSI_RX_DATA0));
				}
			}

			MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse,
				       AS_UINT32(&read_data0),
				       AS_UINT32(&(dsi_params->lcm_esd_check_table[i])));

			DISPDBG
			    ("[DSI]enter cmp read_data0 byte0=0x%x byte1=0x%x byte2=0x%x byte3=0x%x\n",
			     read_data0.byte0, read_data0.byte1, read_data0.byte2,
			     read_data0.byte3);
			DISPDBG
			    ("[DSI]cmp check_table cmd=0x%x,count=0x%x,para_list[0]=0x%x,para_list[1]=0x%x\n",
			     dsi_params->lcm_esd_check_table[i].cmd,
			     dsi_params->lcm_esd_check_table[i].count,
			     dsi_params->lcm_esd_check_table[i].para_list[0],
			     dsi_params->lcm_esd_check_table[i].para_list[1]);
			DISPDBG("[DSI]enter cmp DSI+0x200=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x200));
			DISPDBG("[DSI]enter cmp DSI+0x204=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x204));
			DISPDBG("[DSI]enter cmp DSI+0x60=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x60));
			DISPDBG("[DSI]enter cmp DSI+0x74=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x74));
			DISPDBG("[DSI]enter cmp DSI+0x88=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x88));
			DISPDBG("[DSI]enter cmp DSI+0x0c=0x%x\n",
				AS_UINT32(DDP_REG_BASE_DSI0 + 0x0c));

			if (read_data0.byte1 ==
			    dsi_params->lcm_esd_check_table[i].para_list[0]) {
				/* clear rx data */
				/* DSI_OUTREG32(NULL, &DSI_REG[dsi_i]->DSI_RX_DATA0,0); */
				ret = 0;	/* esd pass */
			} else {
				ret = 1;	/* esd fail */
				break;
			}
		}

	} else if (state == CMDQ_ESD_ALLC_SLOT) {
		/* create 3 slot */
		cmdqBackupAllocateSlot(&hSlot, 3);
	} else if (state == CMDQ_ESD_FREE_SLOT) {
		if (hSlot) {
			cmdqBackupFreeSlot(hSlot);
			hSlot = 0;
		}
	} else if (state == CMDQ_STOP_VDO_MODE) {
		/* use cmdq to stop dsi vdo mode */
		/* -1. stop TE_RDY IRQ */
		DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_INT_ENABLE_REG,
			      DSI_REG[i]->DSI_INTEN, TE_RDY, 0);

		/* 0. set dsi cmd mode */
		DSI_SetMode(module, cmdq_trigger_handle, CMD_MODE);

		/* 1. polling dsi not busy */
		i = DSI_MODULE_BEGIN(module);
		if (i == 0) {
			/* DSI0/DUAL */
			/* polling vm done */
			/* polling dsi busy */
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[i]->DSI_INTSTA,
				      0x80000000, 0);
#if 0
			i = DSI_MODULE_END(module);
			if (i == 1) {	/* DUAL */
				DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[i]->DSI_INTSTA,
					      0x80000000, 0);
			}
#endif
		}
#if 0
		else {	/* DSI1 */
			DSI_POLLREG32(cmdq_trigger_handle, &DSI_REG[i]->DSI_INTSTA,
				      0x80000000, 0);
		}
#endif
		/* 2.dual dsi need do reset DSI_DUAL_EN/DSI_START */
		if (module == DISP_MODULE_DSIDUAL) {
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_COM_CTRL_REG,
				      DSI_REG[0]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_COM_CTRL_REG,
				      DSI_REG[1]->DSI_COM_CTRL, DSI_DUAL_EN, 0);
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_START_REG,
				      DSI_REG[0]->DSI_START, DSI_START, 0);
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_START_REG,
				      DSI_REG[1]->DSI_START, DSI_START, 0);
		}
		/* 3.disable HS */
		/* DSI_clk_HS_mode(module, cmdq_trigger_handle, false); */

	} else if (state == CMDQ_START_VDO_MODE) {

		/* 0. dual dsi set DSI_START/DSI_DUAL_EN */
		if (module == DISP_MODULE_DSIDUAL) {
			/* must set DSI_START to 0 before set dsi_dual_en, don't know why.2014.02.15 */
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_START_REG,
				      DSI_REG[0]->DSI_START, DSI_START, 0);
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_START_REG,
				      DSI_REG[1]->DSI_START, DSI_START, 0);

			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_COM_CTRL_REG,
				      DSI_REG[0]->DSI_COM_CTRL, DSI_DUAL_EN, 1);
			DSI_OUTREGBIT(cmdq_trigger_handle, struct DSI_COM_CTRL_REG,
				      DSI_REG[1]->DSI_COM_CTRL, DSI_DUAL_EN, 1);

		}
		/* 1. set dsi vdo mode */
		DSI_SetMode(module, cmdq_trigger_handle, dsi_params->mode);

		/* 2. enable HS */
		/* DSI_clk_HS_mode(module, cmdq_trigger_handle, true); */

		/* 3. enable mutex */
		/* ddp_mutex_enable(mutex_id_for_latest_trigger,0,cmdq_trigger_handle); */

		/* 4. start dsi */
		/* DSI_Start(module, cmdq_trigger_handle); */

	} else if (state == CMDQ_DSI_RESET) {
		DISPMSG("CMDQ Timeout, Reset DSI\n");
		DSI_DumpRegisters(module, 1);
		DSI_Reset(module, NULL);
	}

	return ret;
}

DDP_MODULE_DRIVER ddp_driver_dsi0 = {
	.module = DISP_MODULE_DSI0,
	.init = ddp_dsi_init,
	.deinit = ddp_dsi_deinit,
	.config = ddp_dsi_config,
	.build_cmdq = ddp_dsi_build_cmdq,
	.trigger = ddp_dsi_trigger,
	.start = ddp_dsi_start,
	.stop = ddp_dsi_stop,
	.reset = ddp_dsi_reset,
	.power_on = ddp_dsi_power_on,
	.power_off = ddp_dsi_power_off,
	.is_idle = ddp_dsi_is_idle,
	.is_busy = ddp_dsi_is_busy,
	.dump_info = ddp_dsi_dump,
	.set_lcm_utils = ddp_dsi_set_lcm_utils,
	.ioctl = ddp_dsi_ioctl
};

DDP_MODULE_DRIVER ddp_driver_dsi1 = {
	.module = DISP_MODULE_DSI1,
	.init = ddp_dsi_init,
	.deinit = ddp_dsi_deinit,
	.config = ddp_dsi_config,
	.build_cmdq = ddp_dsi_build_cmdq,
	.trigger = ddp_dsi_trigger,
	.start = ddp_dsi_start,
	.stop = ddp_dsi_stop,
	.reset = ddp_dsi_reset,
	.power_on = ddp_dsi_power_on,
	.power_off = ddp_dsi_power_off,
	.is_idle = ddp_dsi_is_idle,
	.is_busy = ddp_dsi_is_busy,
	.dump_info = ddp_dsi_dump,
	.set_lcm_utils = ddp_dsi_set_lcm_utils,
	.ioctl = ddp_dsi_ioctl
};


DDP_MODULE_DRIVER ddp_driver_dsidual = {
	.module = DISP_MODULE_DSIDUAL,
	.init = ddp_dsi_init,
	.deinit = ddp_dsi_deinit,
	.config = ddp_dsi_config,
	.build_cmdq = ddp_dsi_build_cmdq,
	.trigger = ddp_dsi_trigger,
	.start = ddp_dsi_start,
	.stop = ddp_dsi_stop,
	.reset = ddp_dsi_reset,
	.power_on = ddp_dsi_power_on,
	.power_off = ddp_dsi_power_off,
	.is_idle = ddp_dsi_is_idle,
	.is_busy = ddp_dsi_is_busy,
	.dump_info = ddp_dsi_dump,
	.set_lcm_utils = ddp_dsi_set_lcm_utils,
	.ioctl = ddp_dsi_ioctl
};


const LCM_UTIL_FUNCS PM_lcm_utils_dsi0 = {
	.set_reset_pin = lcm_set_reset_pin,
	.udelay = lcm_udelay,
	.mdelay = lcm_mdelay,
	.dsi_set_cmdq = DSI_set_cmdq_wrapper_DSI0,
	.dsi_set_cmdq_V2 = DSI_set_cmdq_V2_Wrapper_DSI0
};

void *get_dsi_params_handle(uint32_t dsi_idx)
{
	if (dsi_idx != PM_DSI1)
		return (void *)(&_dsi_context[0].dsi_params);
	else
		return (void *)(&_dsi_context[1].dsi_params);
}


uint32_t PanelMaster_get_TE_status(uint32_t dsi_idx)
{
	if (dsi_idx == 0)
		return dsi0_te_enable ? 1 : 0;
	else
		return dsi1_te_enable ? 1 : 0;
}

uint32_t PanelMaster_get_CC(uint32_t dsi_idx)
{
	struct DSI_TXRX_CTRL_REG tmp_reg;

	DSI_READREG32((struct DSI_TXRX_CTRL_REG *), &tmp_reg, &DSI_REG[dsi_idx]->DSI_TXRX_CTRL);
	return tmp_reg.HSTX_CKLP_EN ? 1 : 0;
}

void PanelMaster_set_CC(uint32_t dsi_index, uint32_t enable)
{
	DISPMSG("set_cc :%d\n", enable);
	if (dsi_index == PM_DSI0) {
		DSI_OUTREGBIT(NULL, struct DSI_TXRX_CTRL_REG, DSI_REG[0]->DSI_TXRX_CTRL,
			      HSTX_CKLP_EN, enable);
	} else if (dsi_index == PM_DSI1) {
		DSI_OUTREGBIT(NULL, struct DSI_TXRX_CTRL_REG, DSI_REG[1]->DSI_TXRX_CTRL,
			      HSTX_CKLP_EN, enable);
	} else if (dsi_index == PM_DSI_DUAL) {
		DSI_OUTREGBIT(NULL, struct DSI_TXRX_CTRL_REG, DSI_REG[0]->DSI_TXRX_CTRL,
			      HSTX_CKLP_EN, enable);
		DSI_OUTREGBIT(NULL, struct DSI_TXRX_CTRL_REG, DSI_REG[1]->DSI_TXRX_CTRL,
			      HSTX_CKLP_EN, enable);
	}
}

void PanelMaster_DSI_set_timing(uint32_t dsi_index, MIPI_TIMING timing)
{
	uint32_t hbp_byte;
	LCM_DSI_PARAMS *dsi_params;
	int fbconfig_dsiTmpBufBpp = 0;

	if (_dsi_context[dsi_index].dsi_params.data_format.format == LCM_DSI_FORMAT_RGB565)
		fbconfig_dsiTmpBufBpp = 2;
	else
		fbconfig_dsiTmpBufBpp = 3;
	dsi_params = get_dsi_params_handle(dsi_index);
	switch (timing.type) {
	case LPX:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, LPX, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, LPX, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, LPX, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, LPX, timing.value);
		}
		break;
	case HS_PRPR:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_PRPR, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_PRPR, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_PRPR, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_PRPR, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON0_REG,DSI_REG->DSI_PHY_TIMECON0,HS_PRPR,timing.value); */
		break;
	case HS_ZERO:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_ZERO, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_ZERO, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_ZERO, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_ZERO, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON0_REG,DSI_REG->DSI_PHY_TIMECON0,HS_ZERO,timing.value); */
		break;
	case HS_TRAIL:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_TRAIL, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_TRAIL, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON0, HS_TRAIL, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON0_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON0, HS_TRAIL, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON0_REG,DSI_REG->DSI_PHY_TIMECON0,HS_TRAIL,timing.value); */
		break;
	case TA_GO:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_GO, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_GO, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_GO, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_GO, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON1_REG,DSI_REG->DSI_PHY_TIMECON1,TA_GO,timing.value); */
		break;
	case TA_SURE:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_SURE, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_SURE, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_SURE, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_SURE, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON1_REG,DSI_REG->DSI_PHY_TIMECON1,TA_SURE,timing.value); */
		break;
	case TA_GET:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_GET, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_GET, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, TA_GET, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, TA_GET, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON1_REG,DSI_REG->DSI_PHY_TIMECON1,TA_GET,timing.value); */
		break;
	case DA_HS_EXIT:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, DA_HS_EXIT,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, DA_HS_EXIT,
				      timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON1, DA_HS_EXIT,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON1_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON1, DA_HS_EXIT,
				      timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON1_REG,DSI_REG->DSI_PHY_TIMECON1,DA_HS_EXIT,timing.value); */
		break;
	case CONT_DET:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CONT_DET, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CONT_DET, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CONT_DET, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CONT_DET, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON2_REG,DSI_REG->DSI_PHY_TIMECON2,CONT_DET,timing.value); */
		break;
	case CLK_ZERO:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CLK_ZERO, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CLK_ZERO, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CLK_ZERO, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CLK_ZERO, timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON2_REG,DSI_REG->DSI_PHY_TIMECON2,CLK_ZERO,timing.value); */
		break;
	case CLK_TRAIL:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CLK_TRAIL,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CLK_TRAIL,
				      timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON2, CLK_TRAIL,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON2_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON2, CLK_TRAIL,
				      timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON2_REG,DSI_REG->DSI_PHY_TIMECON2,CLK_TRAIL,timing.value); */
		break;
	case CLK_HS_PRPR:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_PRPR,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_PRPR,
				      timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_PRPR,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_PRPR,
				      timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON3_REG,DSI_REG->DSI_PHY_TIMECON3,CLK_HS_PRPR,timing.value); */
		break;
	case CLK_HS_POST:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_POST,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_POST,
				      timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_POST,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_POST,
				      timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON3_REG,DSI_REG->DSI_PHY_TIMECON3,CLK_HS_POST,timing.value); */
		break;
	case CLK_HS_EXIT:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_EXIT,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_EXIT,
				      timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[0]->DSI_PHY_TIMECON3, CLK_HS_EXIT,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_PHY_TIMCON3_REG,
				      DSI_REG[1]->DSI_PHY_TIMECON3, CLK_HS_EXIT,
				      timing.value);
		}
		/* OUTREGBIT(struct DSI_PHY_TIMCON3_REG,DSI_REG->DSI_PHY_TIMECON3,CLK_HS_EXIT,timing.value); */
		break;
	case HPW:
		if (dsi_params->mode == SYNC_EVENT_VDO_MODE
		    || dsi_params->mode == BURST_VDO_MODE) {
			/* do nothing */
		} else {
			timing.value = (timing.value * fbconfig_dsiTmpBufBpp - 10);
		}
		timing.value = ALIGN_TO((timing.value), 4);
		if (dsi_index == PM_DSI0) {
			DSI_OUTREG32(NULL, &DSI_REG[0]->DSI_HSA_WC, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREG32(NULL, &DSI_REG[1]->DSI_HSA_WC, timing.value);
		} else if (dsi_index == PM_DSI_DUAL) {
			DSI_OUTREG32(NULL, &DSI_REG[0]->DSI_HSA_WC, timing.value);
			DSI_OUTREG32(NULL, &DSI_REG[1]->DSI_HSA_WC, timing.value);
		}
		break;
	case HFP:
		timing.value = timing.value * fbconfig_dsiTmpBufBpp - 12;
		timing.value = ALIGN_TO(timing.value, 4);
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_HFP_WC_REG, DSI_REG[0]->DSI_HFP_WC, HFP_WC,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_HFP_WC_REG, DSI_REG[1]->DSI_HFP_WC, HFP_WC,
				      timing.value);
		} else {
			DSI_OUTREGBIT(NULL, struct DSI_HFP_WC_REG, DSI_REG[0]->DSI_HFP_WC, HFP_WC,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_HFP_WC_REG, DSI_REG[1]->DSI_HFP_WC, HFP_WC,
				      timing.value);
		}
		break;
	case HBP:
		if (dsi_params->mode == SYNC_EVENT_VDO_MODE
		    || dsi_params->mode == BURST_VDO_MODE) {
			hbp_byte =
			    ((timing.value +
			      dsi_params->horizontal_sync_active) * fbconfig_dsiTmpBufBpp -
			     10);
		} else {
/* hsa_byte = (dsi_params->horizontal_sync_active * fbconfig_dsiTmpBufBpp - 10); */
			hbp_byte = timing.value * fbconfig_dsiTmpBufBpp - 10;
		}
		if (dsi_index == PM_DSI0) {
			DSI_OUTREG32(NULL, &DSI_REG[0]->DSI_HBP_WC,
				     ALIGN_TO((hbp_byte), 4));
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREG32(NULL, &DSI_REG[1]->DSI_HBP_WC,
				     ALIGN_TO((hbp_byte), 4));
		} else {
			DSI_OUTREG32(NULL, &DSI_REG[0]->DSI_HBP_WC,
				     ALIGN_TO((hbp_byte), 4));
			DSI_OUTREG32(NULL, &DSI_REG[1]->DSI_HBP_WC,
				     ALIGN_TO((hbp_byte), 4));
		}

		break;
	case VPW:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_VACT_NL_REG, DSI_REG[0]->DSI_VACT_NL,
				      VACT_NL, timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_VACT_NL_REG, DSI_REG[1]->DSI_VACT_NL,
				      VACT_NL, timing.value);
		} else {
			DSI_OUTREGBIT(NULL, struct DSI_VACT_NL_REG, DSI_REG[0]->DSI_VACT_NL,
				      VACT_NL, timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_VACT_NL_REG, DSI_REG[1]->DSI_VACT_NL,
				      VACT_NL, timing.value);
		}
		/* OUTREG32(&DSI_REG->DSI_VACT_NL,timing.value); */
		break;
	case VFP:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_VFP_NL_REG, DSI_REG[0]->DSI_VFP_NL, VFP_NL,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_VFP_NL_REG, DSI_REG[1]->DSI_VFP_NL, VFP_NL,
				      timing.value);
		} else {
			DSI_OUTREGBIT(NULL, struct DSI_VFP_NL_REG, DSI_REG[0]->DSI_VFP_NL, VFP_NL,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_VFP_NL_REG, DSI_REG[1]->DSI_VFP_NL, VFP_NL,
				      timing.value);
		}
		/* OUTREG32(&DSI_REG->DSI_VFP_NL, timing.value); */
		break;
	case VBP:
		if (dsi_index == PM_DSI0) {
			DSI_OUTREGBIT(NULL, struct DSI_VBP_NL_REG, DSI_REG[0]->DSI_VBP_NL, VBP_NL,
				      timing.value);
		} else if (dsi_index == PM_DSI1) {
			DSI_OUTREGBIT(NULL, struct DSI_VBP_NL_REG, DSI_REG[1]->DSI_VBP_NL, VBP_NL,
				      timing.value);
		} else {
			DSI_OUTREGBIT(NULL, struct DSI_VBP_NL_REG, DSI_REG[0]->DSI_VBP_NL, VBP_NL,
				      timing.value);
			DSI_OUTREGBIT(NULL, struct DSI_VBP_NL_REG, DSI_REG[1]->DSI_VBP_NL, VBP_NL,
				      timing.value);
		}
		/* OUTREG32(&DSI_REG->DSI_VBP_NL, timing.value); */
		break;
	case SSC_EN:
		DSI_ssc_enable(dsi_index, timing.value);
		break;
	default:
		DISPERR("fbconfig dsi set timing :no such type!!\n");
	}
}

int32_t DSI_ssc_enable(uint32_t dsi_index, uint32_t en)
{
	uint32_t disable = en ? 0 : 1;

	if (dsi_index == PM_DSI0) {
		DSI_OUTREGBIT(NULL, struct MIPITX_DSI_PLL_CON1_REG,
			      DSI_PHY_REG[0]->MIPITX_DSI_PLL_CON1, RG_DSI0_MPPLL_SDM_SSC_EN,
			      en);
		_dsi_context[0].dsi_params.ssc_disable = disable;
	} else if (dsi_index == PM_DSI1) {
		DSI_OUTREGBIT(NULL, struct MIPITX_DSI_PLL_CON1_REG,
			      DSI_PHY_REG[1]->MIPITX_DSI_PLL_CON1, RG_DSI0_MPPLL_SDM_SSC_EN,
			      en);
		_dsi_context[1].dsi_params.ssc_disable = disable;
	} else if (dsi_index == PM_DSI_DUAL) {
		DSI_OUTREGBIT(NULL, struct MIPITX_DSI_PLL_CON1_REG,
			      DSI_PHY_REG[0]->MIPITX_DSI_PLL_CON1, RG_DSI0_MPPLL_SDM_SSC_EN,
			      en);
		DSI_OUTREGBIT(NULL, struct MIPITX_DSI_PLL_CON1_REG,
			      DSI_PHY_REG[1]->MIPITX_DSI_PLL_CON1, RG_DSI0_MPPLL_SDM_SSC_EN,
			      en);
		_dsi_context[0].dsi_params.ssc_disable =
		    _dsi_context[1].dsi_params.ssc_disable = disable;
	}
	return 0;
}

uint32_t PanelMaster_get_dsi_timing(uint32_t dsi_index, MIPI_SETTING_TYPE type)
{
	uint32_t dsi_val;
	struct DSI_REGS *dsi_reg;
	int fbconfig_dsiTmpBufBpp = 0;

	if (_dsi_context[dsi_index].dsi_params.data_format.format == LCM_DSI_FORMAT_RGB565)
		fbconfig_dsiTmpBufBpp = 2;
	else
		fbconfig_dsiTmpBufBpp = 3;
	if ((dsi_index == PM_DSI0) || (dsi_index == PM_DSI_DUAL))
		dsi_reg = DSI_REG[0];
	else
		dsi_reg = DSI_REG[1];
	switch (type) {
	case LPX:
		dsi_val = dsi_reg->DSI_PHY_TIMECON0.LPX;
		return dsi_val;
	case HS_PRPR:
		dsi_val = dsi_reg->DSI_PHY_TIMECON0.HS_PRPR;
		return dsi_val;
	case HS_ZERO:
		dsi_val = dsi_reg->DSI_PHY_TIMECON0.HS_ZERO;
		return dsi_val;
	case HS_TRAIL:
		dsi_val = dsi_reg->DSI_PHY_TIMECON0.HS_TRAIL;
		return dsi_val;
	case TA_GO:
		dsi_val = dsi_reg->DSI_PHY_TIMECON1.TA_GO;
		return dsi_val;
	case TA_SURE:
		dsi_val = dsi_reg->DSI_PHY_TIMECON1.TA_SURE;
		return dsi_val;
	case TA_GET:
		dsi_val = dsi_reg->DSI_PHY_TIMECON1.TA_GET;
		return dsi_val;
	case DA_HS_EXIT:
		dsi_val = dsi_reg->DSI_PHY_TIMECON1.DA_HS_EXIT;
		return dsi_val;
	case CONT_DET:
		dsi_val = dsi_reg->DSI_PHY_TIMECON2.CONT_DET;
		return dsi_val;
	case CLK_ZERO:
		dsi_val = dsi_reg->DSI_PHY_TIMECON2.CLK_ZERO;
		return dsi_val;
	case CLK_TRAIL:
		dsi_val = dsi_reg->DSI_PHY_TIMECON2.CLK_TRAIL;
		return dsi_val;
	case CLK_HS_PRPR:
		dsi_val = dsi_reg->DSI_PHY_TIMECON3.CLK_HS_PRPR;
		return dsi_val;
	case CLK_HS_POST:
		dsi_val = dsi_reg->DSI_PHY_TIMECON3.CLK_HS_POST;
		return dsi_val;
	case CLK_HS_EXIT:
		dsi_val = dsi_reg->DSI_PHY_TIMECON3.CLK_HS_EXIT;
		return dsi_val;
	case HPW:
		{
			struct DSI_HSA_WC_REG tmp_reg;

			DSI_READREG32((struct DSI_HSA_WC_REG *), &tmp_reg, &dsi_reg->DSI_HSA_WC);
			dsi_val = (tmp_reg.HSA_WC + 10) / fbconfig_dsiTmpBufBpp;
			return dsi_val;
		}
	case HFP:
		{
			struct DSI_HFP_WC_REG tmp_hfp;

			DSI_READREG32((struct DSI_HFP_WC_REG *), &tmp_hfp, &dsi_reg->DSI_HFP_WC);
			dsi_val = ((tmp_hfp.HFP_WC + 12) / fbconfig_dsiTmpBufBpp);
			return dsi_val;
		}
	case HBP:
		{
			struct DSI_HBP_WC_REG tmp_hbp;
			LCM_DSI_PARAMS *dsi_params;

			dsi_params = get_dsi_params_handle(dsi_index);
			OUTREG32(&tmp_hbp, AS_UINT32(&dsi_reg->DSI_HBP_WC));
			if (dsi_params->mode == SYNC_EVENT_VDO_MODE
			    || dsi_params->mode == BURST_VDO_MODE)
				return ((tmp_hbp.HBP_WC + 10) / fbconfig_dsiTmpBufBpp -
					dsi_params->horizontal_sync_active);
			else
				return (tmp_hbp.HBP_WC + 10) / fbconfig_dsiTmpBufBpp;
		}
	case VPW:
		{
			struct DSI_VACT_NL_REG tmp_vpw;

			DSI_READREG32((struct DSI_VACT_NL_REG *), &tmp_vpw, &dsi_reg->DSI_VACT_NL);
			dsi_val = tmp_vpw.VACT_NL;
			return dsi_val;
		}
	case VFP:
		{
			struct DSI_VFP_NL_REG tmp_vfp;

			DSI_READREG32((struct DSI_VFP_NL_REG *), &tmp_vfp, &dsi_reg->DSI_VFP_NL);
			dsi_val = tmp_vfp.VFP_NL;
			return dsi_val;
		}
	case VBP:
		{
			struct DSI_VBP_NL_REG tmp_vbp;

			DSI_READREG32((struct DSI_VBP_NL_REG *), &tmp_vbp, &dsi_reg->DSI_VBP_NL);
			dsi_val = tmp_vbp.VBP_NL;
			return dsi_val;
		}
	case SSC_EN:
		{
			if (_dsi_context[dsi_index].dsi_params.ssc_disable)
				dsi_val = 0;
			else
				dsi_val = 1;
			return dsi_val;
		}
	default:
		DISPERR("fbconfig dsi set timing :no such type!!\n");
	}
	dsi_val = 0;
	return dsi_val;
}

unsigned int PanelMaster_set_PM_enable(unsigned int value)
{
	atomic_set(&PMaster_enable, value);
	return 0;
}
/* No DSI Driver */
int DSI_set_roi(int x, int y)
{
	DISPMSG("[DSI](x0,y0,x1,y1)=(%d,%d,%d,%d)\n", x, y, _dsi_context[0].lcm_width,
		 _dsi_context[0].lcm_height);
	return DSI_Send_ROI(DISP_MODULE_DSI0, NULL, x, y, _dsi_context[0].lcm_width - x,
			    _dsi_context[0].lcm_height - y);
}

int DSI_check_roi(void)
{
	int ret = 0;
	unsigned char read_buf[4] = { 1, 1, 1, 1 };
	unsigned int data_array[16];
	int count, x0, y0;

	data_array[0] = 0x00043700;	/* read id return two byte,version and id */
	DSI_set_cmdq(DISP_MODULE_DSI0, NULL, data_array, 1, 1);
	usleep_range(10000, 11000); /* sleep 10~11ms */
	count = DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, 0x2a, read_buf, 4);
	usleep_range(10000, 11000); /* sleep 10~11ms */
	x0 = (read_buf[0] << 8) | read_buf[1];
	DISPMSG
	    ("x0=%d count=%d,read_buf[0]=%d,read_buf[1]=%d,read_buf[2]=%d,read_buf[3]=%d\n",
	     x0, count, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
	if ((count == 0) || (x0 != 0)) {
		DISPMSG
		    ("[DSI]x count %d read_buf[0]=%d,read_buf[1]=%d,read_buf[2]=%d,read_buf[3]=%d\n",
		     count, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
		return -1;
	}
	usleep_range(10000, 11000); /* sleep 10~11ms */
	count = DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, 0x2b, read_buf, 4);
	y0 = (read_buf[0] << 8) | read_buf[1];
	DISPMSG
	    ("y0=%d count %d,read_buf[0]=%d,read_buf[1]=%d,read_buf[2]=%d,read_buf[3]=%d\n",
	     y0, count, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
	if ((count == 0) || (y0 != 0)) {
		DISPMSG
		    ("[DSI]y count %d read_buf[0]=%d,read_buf[1]=%d,read_buf[2]=%d,read_buf[3]=%d\n",
		     count, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
		return -1;
	}
	return ret;
}

void DSI_ForceConfig(int forceconfig)
{
	dsi_force_config = forceconfig;
}
