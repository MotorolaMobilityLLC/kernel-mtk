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

#include "cmdq_core.h"
#include "cmdq_reg.h"
#include "cmdq_mdp_common.h"
#ifdef CMDQ_MET_READY
#include <linux/met_drv.h>
#endif
#ifndef CMDQ_USE_CCF
#include <mach/mt_clkmgr.h>
#endif				/* !defined(CMDQ_USE_CCF) */
#include "m4u.h"

#include "cmdq_device.h"

typedef struct CmdqMdpModuleBaseVA {
	long MDP_RDMA;
	long MDP_RSZ0;
	long MDP_RSZ1;
	long MDP_WDMA;
	long MDP_WROT;
	long MDP_TDSHP;
	long MDP_COLOR;
	long VENC;
} CmdqMdpModuleBaseVA;
static CmdqMdpModuleBaseVA gCmdqMdpModuleBaseVA;

#ifndef CMDQ_USE_CCF
#define IMP_ENABLE_MDP_HW_CLOCK(FN_NAME, HW_NAME)	\
uint32_t cmdq_mdp_enable_clock_##FN_NAME(bool enable)	\
{		\
	return cmdq_dev_enable_mtk_clock(enable, MT_CG_DISP0_##HW_NAME, "CMDQ_MDP_"#HW_NAME);	\
}
#define IMP_MDP_HW_CLOCK_IS_ENABLE(FN_NAME, HW_NAME)	\
bool cmdq_mdp_clock_is_enable_##FN_NAME(void)	\
{		\
	return cmdq_dev_mtk_clock_is_enable(MT_CG_DISP0_##HW_NAME);	\
}
#else
typedef struct CmdqMdpModuleClock {
	struct clk *clk_CAM_MDP;
	struct clk *clk_MDP_RDMA;
	struct clk *clk_MDP_RSZ0;
	struct clk *clk_MDP_RSZ1;
	struct clk *clk_MDP_WDMA;
	struct clk *clk_MDP_WROT;
	struct clk *clk_MDP_TDSHP;
	struct clk *clk_MDP_COLOR;
} CmdqMdpModuleClock;

static CmdqMdpModuleClock gCmdqMdpModuleClock;

#define IMP_ENABLE_MDP_HW_CLOCK(FN_NAME, HW_NAME)	\
uint32_t cmdq_mdp_enable_clock_##FN_NAME(bool enable)	\
{		\
	return cmdq_dev_enable_device_clock(enable, gCmdqMdpModuleClock.clk_##HW_NAME, #HW_NAME "-clk");	\
}
#define IMP_MDP_HW_CLOCK_IS_ENABLE(FN_NAME, HW_NAME)	\
bool cmdq_mdp_clock_is_enable_##FN_NAME(void)	\
{		\
	return cmdq_dev_device_clock_is_enable(gCmdqMdpModuleClock.clk_##HW_NAME);	\
}
#endif				/* !defined(CMDQ_USE_CCF) */

IMP_ENABLE_MDP_HW_CLOCK(CAM_MDP, CAM_MDP);
IMP_ENABLE_MDP_HW_CLOCK(MDP_RDMA0, MDP_RDMA);
IMP_ENABLE_MDP_HW_CLOCK(MDP_RSZ0, MDP_RSZ0);
IMP_ENABLE_MDP_HW_CLOCK(MDP_RSZ1, MDP_RSZ1);
IMP_ENABLE_MDP_HW_CLOCK(MDP_WDMA, MDP_WDMA);
IMP_ENABLE_MDP_HW_CLOCK(MDP_WROT0, MDP_WROT);
IMP_ENABLE_MDP_HW_CLOCK(MDP_TDSHP0, MDP_TDSHP);
IMP_ENABLE_MDP_HW_CLOCK(MDP_COLOR, MDP_COLOR);
IMP_MDP_HW_CLOCK_IS_ENABLE(CAM_MDP, CAM_MDP);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_RDMA0, MDP_RDMA);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_RSZ0, MDP_RSZ0);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_RSZ1, MDP_RSZ1);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_WDMA, MDP_WDMA);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_WROT0, MDP_WROT);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_TDSHP0, MDP_TDSHP);
IMP_MDP_HW_CLOCK_IS_ENABLE(MDP_COLOR, MDP_COLOR);
#undef IMP_ENABLE_MDP_HW_CLOCK
#undef IMP_MDP_HW_CLOCK_IS_ENABLE

long cmdq_mdp_get_module_base_VA_MDP_RDMA(void)
{
	return gCmdqMdpModuleBaseVA.MDP_RDMA;
}

long cmdq_mdp_get_module_base_VA_MDP_RSZ0(void)
{
	return gCmdqMdpModuleBaseVA.MDP_RSZ0;
}

long cmdq_mdp_get_module_base_VA_MDP_RSZ1(void)
{
	return gCmdqMdpModuleBaseVA.MDP_RSZ1;
}

long cmdq_mdp_get_module_base_VA_MDP_WDMA(void)
{
	return gCmdqMdpModuleBaseVA.MDP_WDMA;
}

long cmdq_mdp_get_module_base_VA_MDP_WROT(void)
{
	return gCmdqMdpModuleBaseVA.MDP_WROT;
}

long cmdq_mdp_get_module_base_VA_MDP_TDSHP(void)
{
	return gCmdqMdpModuleBaseVA.MDP_TDSHP;
}

long cmdq_mdp_get_module_base_VA_VENC(void)
{
	return gCmdqMdpModuleBaseVA.VENC;
}

long cmdq_mdp_get_module_base_VA_MDP_COLOR(void)
{
	return gCmdqMdpModuleBaseVA.MDP_COLOR;
}

#ifdef CMDQ_OF_SUPPORT
#define MDP_RDMA_BASE     cmdq_mdp_get_module_base_VA_MDP_RDMA()
#define MDP_RSZ0_BASE     cmdq_mdp_get_module_base_VA_MDP_RSZ0()
#define MDP_RSZ1_BASE     cmdq_mdp_get_module_base_VA_MDP_RSZ1()
#define MDP_TDSHP_BASE    cmdq_mdp_get_module_base_VA_MDP_TDSHP()
#define MDP_WROT_BASE     cmdq_mdp_get_module_base_VA_MDP_WROT()
#define MDP_WDMA_BASE     cmdq_mdp_get_module_base_VA_MDP_WDMA()
#define MDP_COLOR_BASE    cmdq_mdp_get_module_base_VA_MDP_COLOR()
#define VENC_BASE         cmdq_mdp_get_module_base_VA_VENC()
#else
#include <mach/mt_reg_base.h>
#endif

int32_t cmdq_mdp_reset_with_mmsys(const uint64_t engineToResetAgain)
{
	long MMSYS_SW0_RST_B_REG = MMSYS_CONFIG_BASE + (0x140);
	int i = 0;
	uint32_t reset_bits = 0L;
	static const int engineResetBit[32] = {
		-1,		/* bit  0 : SMI COMMON */
		-1,		/* bit  1 : SMI LARB0 */
		CMDQ_ENG_MDP_CAMIN,	/* bit  2 : CAM_MDP */
		CMDQ_ENG_MDP_RDMA0,	/* bit  3 : MDP_RDMA0 */
		CMDQ_ENG_MDP_RSZ0,	/* bit  4 : MDP_RSZ0 */
		CMDQ_ENG_MDP_RSZ1,	/* bit  5 : MDP_RSZ1 */
		CMDQ_ENG_MDP_TDSHP0,	/* bit  6 : MDP_TDSHP0 */
		CMDQ_ENG_MDP_WDMA,	/* bit  7 : MDP_WDMA */
		CMDQ_ENG_MDP_WROT0,	/* bit  8 : MDP_WROT0 */
		[9 ... 31] = -1
	};

	for (i = 0; i < 32; ++i) {
		if (0 > engineResetBit[i])
			continue;

		if (engineToResetAgain & (1LL << engineResetBit[i]))
			reset_bits |= (1 << i);
	}

	if (0 != reset_bits) {
		/* 0: reset */
		/* 1: not reset */
		/* so we need to reverse the bits */
		reset_bits = ~reset_bits;

		CMDQ_REG_SET32(MMSYS_SW0_RST_B_REG, reset_bits);
		CMDQ_REG_SET32(MMSYS_SW0_RST_B_REG, ~0);
		/* This takes effect immediately, no need to poll state */
	}

	return 0;
}

m4u_callback_ret_t cmdq_M4U_TranslationFault_callback(int port, unsigned	int	mva, void *data)
{
	CMDQ_ERR("================= [MDP M4U] Dump Begin ================\n");
	CMDQ_ERR("[MDP M4U]fault call port=%d, mva=0x%x", port, mva);

	cmdq_core_dump_tasks_info();

	switch (port) {
	case M4U_PORT_MDP_RDMA:
		cmdq_mdp_dump_rdma(MDP_RDMA_BASE, "RDMA");
		break;
	case M4U_PORT_MDP_WDMA:
		cmdq_mdp_dump_wdma(MDP_WDMA_BASE, "WDMA");
		break;
	case M4U_PORT_MDP_WROT:
		cmdq_mdp_dump_rot(MDP_WROT_BASE, "WROT");
		break;
	default:
		CMDQ_ERR("[MDP M4U]fault callback function");
		break;
	}

	CMDQ_ERR("================= [MDP M4U] Dump End ================\n");

	return M4U_CALLBACK_HANDLED;
}

int32_t cmdqVEncDumpInfo(uint64_t engineFlag, int logLevel)
{
	if (engineFlag & (1LL << CMDQ_ENG_VIDEO_ENC))
		cmdq_mdp_dump_venc(VENC_BASE, "VENC");

	return 0;
}

void cmdq_mdp_init_module_base_VA(void)
{
	memset(&gCmdqMdpModuleBaseVA, 0, sizeof(CmdqMdpModuleBaseVA));

#ifdef CMDQ_OF_SUPPORT
	gCmdqMdpModuleBaseVA.MDP_RDMA = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_rdma");
	gCmdqMdpModuleBaseVA.MDP_RSZ0 = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_rsz0");
	gCmdqMdpModuleBaseVA.MDP_RSZ1 = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_rsz1");
	gCmdqMdpModuleBaseVA.MDP_WDMA = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_wdma");
	gCmdqMdpModuleBaseVA.MDP_WROT = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_wrot");
	gCmdqMdpModuleBaseVA.MDP_TDSHP = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_tdshp");
	gCmdqMdpModuleBaseVA.MDP_COLOR = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mdp_color");
	gCmdqMdpModuleBaseVA.VENC = cmdq_dev_alloc_module_base_VA_by_name("mediatek,mt6755-venc");
#endif
}

void cmdq_mdp_deinit_module_base_VA(void)
{
#ifdef CMDQ_OF_SUPPORT
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_RDMA());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_RSZ0());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_RSZ1());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_WDMA());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_WROT());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_TDSHP());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_MDP_COLOR());
	cmdq_dev_free_module_base_VA(cmdq_mdp_get_module_base_VA_VENC());

	memset(&gCmdqMdpModuleBaseVA, 0, sizeof(CmdqMdpModuleBaseVA));
#else
	/* do nothing, registers' IOMAP will be destroyed by platform */
#endif
}

bool cmdq_mdp_clock_is_on(CMDQ_ENG_ENUM engine)
{
	switch (engine) {
	case CMDQ_ENG_MDP_CAMIN:
		return cmdq_mdp_clock_is_enable_CAM_MDP();
	case CMDQ_ENG_MDP_RDMA0:
		return cmdq_mdp_clock_is_enable_MDP_RDMA0();
	case CMDQ_ENG_MDP_RSZ0:
		return cmdq_mdp_clock_is_enable_MDP_RSZ0();
	case CMDQ_ENG_MDP_RSZ1:
		return cmdq_mdp_clock_is_enable_MDP_RSZ1();
	case CMDQ_ENG_MDP_WDMA:
		return cmdq_mdp_clock_is_enable_MDP_WDMA();
	case CMDQ_ENG_MDP_WROT0:
		return cmdq_mdp_clock_is_enable_MDP_WROT0();
	case CMDQ_ENG_MDP_TDSHP0:
		return cmdq_mdp_clock_is_enable_MDP_TDSHP0();
	case CMDQ_ENG_MDP_COLOR0:
		return cmdq_mdp_clock_is_enable_MDP_COLOR();
	default:
		CMDQ_ERR("try to query unknown mdp clock");
		return false;
	}
}

void cmdq_mdp_enable_clock(bool enable, CMDQ_ENG_ENUM engine)
{
	unsigned long register_address;
	uint32_t register_value;

	switch (engine) {
	case CMDQ_ENG_MDP_CAMIN:
		cmdq_mdp_enable_clock_CAM_MDP(enable);
		break;
	case CMDQ_ENG_MDP_RDMA0:
		cmdq_mdp_enable_clock_MDP_RDMA0(enable);
		/* Set MDP_RDMA0 DCM enable */
		register_address = MDP_RDMA_BASE + 0x0;
		register_value = CMDQ_REG_GET32(register_address);
		/* DCM_EN is bit 4 */
		register_value |= (0x1 << 4);
		CMDQ_REG_SET32(register_address, register_value);
		break;
	case CMDQ_ENG_MDP_RSZ0:
		cmdq_mdp_enable_clock_MDP_RSZ0(enable);
		break;
	case CMDQ_ENG_MDP_RSZ1:
		cmdq_mdp_enable_clock_MDP_RSZ1(enable);
		break;
	case CMDQ_ENG_MDP_WDMA:
		cmdq_mdp_enable_clock_MDP_WDMA(enable);
		/* Set MDP_WDMA DCM enable */
		register_address = MDP_WDMA_BASE + 0x8;
		register_value = CMDQ_REG_GET32(register_address);
		/* DCM_EN is bit 31 */
		register_value |= (0x1 << 31);
		CMDQ_REG_SET32(register_address, register_value);
		break;
	case CMDQ_ENG_MDP_WROT0:
		cmdq_mdp_enable_clock_MDP_WROT0(enable);
		/* Set MDP_WROT0 DCM enable */
		register_address = MDP_WROT_BASE + 0x7C;
		register_value = CMDQ_REG_GET32(register_address);
		/* DCM_EN is bit 16 */
		register_value |= (0x1 << 16);
		CMDQ_REG_SET32(register_address, register_value);
		break;
	case CMDQ_ENG_MDP_TDSHP0:
		cmdq_mdp_enable_clock_MDP_TDSHP0(enable);
		break;
	case CMDQ_ENG_MDP_COLOR0:
		cmdq_mdp_enable_clock_MDP_COLOR(enable);
		break;
	default:
		CMDQ_ERR("try to enable unknown mdp clock");
		break;
	}
}

/* Common Clock Framework */
void cmdq_mdp_init_module_clk(void)
{
#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_USE_CCF)
	cmdq_dev_get_module_clock_by_name("mediatek,mmsys_config", "CAM_MDP",
					  &gCmdqMdpModuleClock.clk_CAM_MDP);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_rdma", "MDP_RDMA",
					  &gCmdqMdpModuleClock.clk_MDP_RDMA);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_rsz0", "MDP_RSZ0",
					  &gCmdqMdpModuleClock.clk_MDP_RSZ0);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_rsz1", "MDP_RSZ1",
					  &gCmdqMdpModuleClock.clk_MDP_RSZ1);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_wdma", "MDP_WDMA",
					  &gCmdqMdpModuleClock.clk_MDP_WDMA);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_wrot", "MDP_WROT",
					  &gCmdqMdpModuleClock.clk_MDP_WROT);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_tdshp", "MDP_TDSHP",
					  &gCmdqMdpModuleClock.clk_MDP_TDSHP);
	cmdq_dev_get_module_clock_by_name("mediatek,mdp_color", "MDP_COLOR",
					  &gCmdqMdpModuleClock.clk_MDP_COLOR);
#endif
}

int32_t cmdqMdpClockOn(uint64_t engineFlag)
{
	CMDQ_MSG("Enable MDP(0x%llx) clock begin\n", engineFlag);

#ifdef CMDQ_PWR_AWARE
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_CAMIN);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_RDMA0);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_RSZ0);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_RSZ1);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_TDSHP0);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_WROT0);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_WDMA);
	cmdq_mdp_enable(engineFlag, CMDQ_ENG_MDP_COLOR0);
#else
	CMDQ_MSG("Force MDP clock all on\n");

	/* enable all bits in MMSYS_CG_CLR0 and MMSYS_CG_CLR1 */
	CMDQ_REG_SET32(MMSYS_CONFIG_BASE + 0x108, 0xFFFFFFFF);
	CMDQ_REG_SET32(MMSYS_CONFIG_BASE + 0x118, 0xFFFFFFFF);

#endif				/* #ifdef CMDQ_PWR_AWARE */

	CMDQ_MSG("Enable MDP(0x%llx) clock end\n", engineFlag);

	return 0;
}

typedef struct MODULE_BASE {
	uint64_t engine;
	long base;		/* considering 64 bit kernel, use long type to store base addr */
	const char *name;
} MODULE_BASE;

#define DEFINE_MODULE(eng, base) {eng, base, #eng}

int32_t cmdqMdpDumpInfo(uint64_t engineFlag, int logLevel)
{
	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0))
		cmdq_mdp_dump_rdma(MDP_RDMA_BASE, "RDMA");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0))
		cmdq_mdp_get_func()->mdpDumpRsz(MDP_RSZ0_BASE, "RSZ0");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1))
		cmdq_mdp_get_func()->mdpDumpRsz(MDP_RSZ1_BASE, "RSZ1");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0))
		cmdq_mdp_get_func()->mdpDumpTdshp(MDP_TDSHP_BASE, "TDSHP");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0))
		cmdq_mdp_dump_rot(MDP_WROT_BASE, "WROT");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA))
		cmdq_mdp_dump_wdma(MDP_WDMA_BASE, "WDMA");

	if (engineFlag & (1LL << CMDQ_ENG_MDP_COLOR0))
		cmdq_mdp_dump_color(MDP_COLOR_BASE, "COLOR0");

	/* verbose case, dump entire 1KB HW register region */
	/* for each enabled HW module. */
	if (logLevel >= 1) {
		int inner = 0;

		const MODULE_BASE bases[] = {
			DEFINE_MODULE(CMDQ_ENG_MDP_RDMA0, MDP_RDMA_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_RSZ0, MDP_RSZ0_BASE),
			DEFINE_MODULE(CMDQ_ENG_MDP_RSZ1, MDP_RSZ1_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_TDSHP0, MDP_TDSHP_BASE),
			DEFINE_MODULE(CMDQ_ENG_MDP_COLOR0, MDP_COLOR_BASE),
			DEFINE_MODULE(CMDQ_ENG_MDP_WROT0, MDP_WROT_BASE),

			DEFINE_MODULE(CMDQ_ENG_MDP_WDMA, MDP_WDMA_BASE),
		};

		for (inner = 0; inner < (sizeof(bases) / sizeof(bases[0])); ++inner) {
			if (engineFlag & (1LL << bases[inner].engine)) {
				CMDQ_ERR("========= [CMDQ] %s dump base 0x%lx ========\n",
					 bases[inner].name, bases[inner].base);
				print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
					       (void *)bases[inner].base, 1024, false);
			}
		}
	}

	return 0;
}

typedef enum MOUT_BITS {
	MOUT_BITS_ISP_MDP = 0,	/* bit  0: ISP_MDP multiple outupt reset */
	MOUT_BITS_MDP_RDMA0 = 1,	/* bit  1: MDP_RDMA0 multiple outupt reset */
	MOUT_BITS_MDP_PRZ0 = 2,	/* bit  2: MDP_PRZ0 multiple outupt reset */
	MOUT_BITS_MDP_PRZ1 = 3,	/* bit  3: MDP_PRZ1 multiple outupt reset */
	MOUT_BITS_MDP_TDSHP0 = 4,	/* bit  4: MDP_TDSHP0 multiple outupt reset */
} MOUT_BITS;

int32_t cmdqMdpResetEng(uint64_t engineFlag)
{
#ifndef CMDQ_PWR_AWARE
	return 0;
#else
	int status = 0;
	int64_t engineToResetAgain = 0LL;
	uint32_t mout_bits_old = 0L;
	uint32_t mout_bits = 0L;

	long MMSYS_MOUT_RST_REG = MMSYS_CONFIG_BASE + (0x040);

	CMDQ_PROF_START(0, "MDP_Rst");
	CMDQ_VERBOSE("Reset MDP(0x%llx) begin\n", engineFlag);

	/* After resetting each component, */
	/* we need also reset corresponding MOUT config. */
	mout_bits_old = CMDQ_REG_GET32(MMSYS_MOUT_RST_REG);
	mout_bits = 0;

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_RDMA0);

		status = cmdq_mdp_loop_reset(CMDQ_ENG_MDP_RDMA0,
					     MDP_RDMA_BASE + 0x8,
					     MDP_RDMA_BASE + 0x408, 0x7FF00, 0x100, false);
		if (0 != status)
			engineToResetAgain |= (1LL << CMDQ_ENG_MDP_RDMA0);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_PRZ0);
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_RSZ0)) {
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1)) {
		mout_bits |= (1 << MOUT_BITS_MDP_PRZ1);
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_RSZ1)) {
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		mout_bits |= (1 << MOUT_BITS_MDP_TDSHP0);
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_TDSHP0)) {
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x2);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0)) {
		status = cmdq_mdp_loop_reset(CMDQ_ENG_MDP_WROT0,
					     MDP_WROT_BASE + 0x010,
					     MDP_WROT_BASE + 0x014, 0x1, 0x1, true);
		if (0 != status)
			engineToResetAgain |= (1LL << CMDQ_ENG_MDP_WROT0);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA)) {
		status = cmdq_mdp_loop_reset(CMDQ_ENG_MDP_WDMA,
					     MDP_WDMA_BASE + 0x00C,
					     MDP_WDMA_BASE + 0x0A0, 0x3FF, 0x1, false);
		if (0 != status)
			engineToResetAgain |= (1LL << CMDQ_ENG_MDP_WDMA);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		/* MDP_CAMIN can only reset by mmsys, */
		/* so this is not a "error" */
		cmdq_mdp_reset_with_mmsys((1LL << CMDQ_ENG_MDP_CAMIN));
	}

	/*
	   when MDP engines fail to reset,
	   1. print SMI debug log
	   2. try resetting from MMSYS to restore system state
	   3. report to QA by raising AEE warning
	   this reset will reset all registers to power on state.
	   but DpFramework always reconfigures register values,
	   so there is no need to backup registers.
	 */
	if (0 != engineToResetAgain) {
		/* check SMI state immediately */
		/* if (1 == is_smi_larb_busy(0)) */
		/* { */
		/* smi_hanging_debug(5); */
		/* } */

		CMDQ_ERR("Reset failed MDP engines(0x%llx), reset again with MMSYS_SW0_RST_B\n",
			 engineToResetAgain);

		cmdq_mdp_reset_with_mmsys(engineToResetAgain);

		/* finally, raise AEE warning to report normal reset fail. */
		/* we hope that reset MMSYS. */
		CMDQ_AEE("MDP", "Disable 0x%llx engine failed\n", engineToResetAgain);

		status = -EFAULT;
	}
	/* MOUT configuration reset */
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old & (~mout_bits)));
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old | mout_bits));
	CMDQ_REG_SET32(MMSYS_MOUT_RST_REG, (mout_bits_old & (~mout_bits)));

	CMDQ_MSG("Reset MDP(0x%llx) end\n", engineFlag);
	CMDQ_PROF_END(0, "MDP_Rst");

	return status;

#endif				/* #ifdef CMDQ_PWR_AWARE */

}

int32_t cmdqMdpClockOff(uint64_t engineFlag)
{
#ifdef CMDQ_PWR_AWARE
	CMDQ_MSG("Disable MDP(0x%llx) clock begin\n", engineFlag);

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WDMA)) {
		cmdq_mdp_loop_off(CMDQ_ENG_MDP_WDMA,
				  MDP_WDMA_BASE + 0x00C, MDP_WDMA_BASE + 0X0A0, 0x3FF, 0x1, false);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_WROT0)) {
		cmdq_mdp_loop_off(CMDQ_ENG_MDP_WROT0,
				  MDP_WROT_BASE + 0X010, MDP_WROT_BASE + 0X014, 0x1, 0x1, true);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_TDSHP0)) {
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_TDSHP0)) {
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x2);
			CMDQ_REG_SET32(MDP_TDSHP_BASE + 0x100, 0x0);
			CMDQ_MSG("Disable MDP_TDSHP0 clock\n");
			cmdq_mdp_get_func()->enableMdpClock(false, CMDQ_ENG_MDP_TDSHP0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ1)) {
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_RSZ1)) {
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ1_BASE, 0x0);

			CMDQ_MSG("Disable MDP_RSZ1 clock\n");

			cmdq_mdp_get_func()->enableMdpClock(false, CMDQ_ENG_MDP_RSZ1);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RSZ0)) {
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_RSZ0)) {
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x10000);
			CMDQ_REG_SET32(MDP_RSZ0_BASE, 0x0);

			CMDQ_MSG("Disable MDP_RSZ0 clock\n");

			cmdq_mdp_get_func()->enableMdpClock(false, CMDQ_ENG_MDP_RSZ0);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_RDMA0)) {
		cmdq_mdp_loop_off(CMDQ_ENG_MDP_RDMA0,
				  MDP_RDMA_BASE + 0x008,
				  MDP_RDMA_BASE + 0x408, 0x7FF00, 0x100, false);
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_CAMIN)) {
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_CAMIN)) {
			cmdq_mdp_reset_with_mmsys((1LL << CMDQ_ENG_MDP_CAMIN));

			CMDQ_MSG("Disable MDP_CAMIN clock\n");
			cmdq_mdp_get_func()->enableMdpClock(false, CMDQ_ENG_MDP_CAMIN);
		}
	}

	if (engineFlag & (1LL << CMDQ_ENG_MDP_COLOR0)) {
		if (cmdq_mdp_get_func()->mdpClockIsOn(CMDQ_ENG_MDP_COLOR0)) {
			CMDQ_MSG("Disable MDP_COLOR0 clock\n");
			cmdq_mdp_get_func()->enableMdpClock(false, CMDQ_ENG_MDP_COLOR0);
		}
	}

	CMDQ_MSG("Disable MDP(0x%llx) clock end\n", engineFlag);
#endif				/* #ifdef CMDQ_PWR_AWARE */

	return 0;
}

void cmdqMdpInitialSetting(void)
{
	/* Register M4U Translation Fault function */
	m4u_register_fault_callback(M4U_PORT_MDP_RDMA, cmdq_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_MDP_WDMA, cmdq_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_MDP_WROT, cmdq_M4U_TranslationFault_callback, NULL);
}

uint32_t cmdq_mdp_rdma_get_reg_offset_src_addr(void)
{
	return 0xF00;
}

uint32_t cmdq_mdp_wrot_get_reg_offset_dst_addr(void)
{
	return 0xF00;
}

uint32_t cmdq_mdp_wdma_get_reg_offset_dst_addr(void)
{
	return 0xF00;
}

void testcase_clkmgr_mdp(void)
{
#if defined(CMDQ_PWR_AWARE) && defined(CMDQ_USE_CCF)
	/* RDMA clk test with src buffer addr */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_RDMA0,
			     "CMDQ_TEST_MDP_RDMA0",
			     MDP_RDMA_BASE + cmdq_mdp_rdma_get_reg_offset_src_addr(),
			     0xAACCBBDD,
			     MDP_RDMA_BASE + cmdq_mdp_rdma_get_reg_offset_src_addr(), true);

	/* WDMA clk test with dst buffer addr */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_WDMA,
			     "CMDQ_TEST_MDP_WDMA",
			     MDP_WDMA_BASE + cmdq_mdp_wdma_get_reg_offset_dst_addr(),
			     0xAACCBBDD,
			     MDP_WDMA_BASE + cmdq_mdp_wdma_get_reg_offset_dst_addr(), true);

	/* WROT clk test with dst buffer addr */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_WROT0,
			     "CMDQ_TEST_MDP_WROT0",
			     MDP_WROT_BASE + cmdq_mdp_wrot_get_reg_offset_dst_addr(),
			     0xAACCBBDD,
			     MDP_WROT_BASE + cmdq_mdp_wrot_get_reg_offset_dst_addr(), true);

	/* TDSHP clk test with input size */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_TDSHP0,
			     "CMDQ_TEST_MDP_TDSHP",
			     MDP_TDSHP_BASE + 0x244, 0xAACCBBDD, MDP_TDSHP_BASE + 0x244, true);

	/* RSZ clk test with debug port */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_RSZ0,
			     "CMDQ_TEST_MDP_RSZ0",
			     MDP_RSZ0_BASE + 0x040, 0x00000001, MDP_RSZ0_BASE + 0x044, false);

	testcase_clkmgr_impl(CMDQ_ENG_MDP_RSZ1,
			     "CMDQ_TEST_MDP_RSZ1",
			     MDP_RSZ1_BASE + 0x040, 0x00000001, MDP_RSZ1_BASE + 0x044, false);

	/* COLOR clk test with debug port */
	testcase_clkmgr_impl(CMDQ_ENG_MDP_COLOR0,
				 "CMDQ_TEST_MDP_COLOR",
				 MDP_COLOR_BASE + 0x438, 0x000001AB, MDP_COLOR_BASE + 0x438, true);
#endif				/* defined(CMDQ_USE_CCF) */
}

void cmdq_mdp_platform_function_setting(void)
{
	cmdqMDPFuncStruct *pFunc;

	pFunc = cmdq_mdp_get_func();

	pFunc->vEncDumpInfo = cmdqVEncDumpInfo;

	pFunc->initModuleBaseVA = cmdq_mdp_init_module_base_VA;
	pFunc->deinitModuleBaseVA = cmdq_mdp_deinit_module_base_VA;

	pFunc->mdpClockIsOn = cmdq_mdp_clock_is_on;
	pFunc->enableMdpClock = cmdq_mdp_enable_clock;
	pFunc->initModuleCLK = cmdq_mdp_init_module_clk;

	pFunc->mdpClockOn = cmdqMdpClockOn;
	pFunc->mdpDumpInfo = cmdqMdpDumpInfo;
	pFunc->mdpResetEng = cmdqMdpResetEng;
	pFunc->mdpClockOff = cmdqMdpClockOff;

	pFunc->mdpInitialSet = cmdqMdpInitialSetting;

	pFunc->rdmaGetRegOffsetSrcAddr = cmdq_mdp_rdma_get_reg_offset_src_addr;
	pFunc->wrotGetRegOffsetDstAddr = cmdq_mdp_wrot_get_reg_offset_dst_addr;
	pFunc->wdmaGetRegOffsetDstAddr = cmdq_mdp_wdma_get_reg_offset_dst_addr;
	pFunc->testcaseClkmgrMdp = testcase_clkmgr_mdp;
}
