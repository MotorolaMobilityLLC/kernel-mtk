#include "cmdq_device.h"
#include "cmdq_core.h"
#ifndef CMDQ_OF_SUPPORT
#include <mach/mt_irq.h>
#endif

/* device tree */
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>

typedef struct CmdqModuleBaseVA {
	long MMSYS_CONFIG;
	long MDP_RDMA;
	long MDP_RSZ0;
	long MDP_RSZ1;
	long MDP_WDMA;
	long MDP_WROT;
	long MDP_TDSHP;
	long MM_MUTEX;
	long VENC;
} CmdqModuleBaseVA;

typedef struct CmdqDeviceStruct {
	struct device *pDev;
	long regBaseVA;		/* considering 64 bit kernel, use long */
	long regBasePA;
	uint32_t irqId;
	uint32_t irqSecId;
} CmdqDeviceStruct;

static CmdqModuleBaseVA gCmdqModuleBaseVA;
static CmdqDeviceStruct gCmdqDev;

#define IMP_ENABLE_HW_CLOCK(FN_NAME, HW_NAME)	\
uint32_t cmdq_dev_enable_clock_##FN_NAME(bool enable)	\
{		\
	return cmdq_dev_enable_mtk_clock(enable, MT_CG_DISP0_##HW_NAME, "CMDQ_MDP_"#HW_NAME);	\
}
#define IMP_HW_CLOCK_IS_ENABLE(FN_NAME, HW_NAME)	\
bool cmdq_dev_clock_is_enable_##FN_NAME(void)	\
{		\
	return cmdq_dev_mtk_clock_is_enable(MT_CG_DISP0_##HW_NAME);		\
}

void cmdq_dev_enable_gce_clock(bool enable)
{
	cmdq_dev_enable_mtk_clock(enable, MT_CG_INFRA_GCE, "mtk_cmdq");
}

IMP_ENABLE_HW_CLOCK(SMI_COMMON, SMI_COMMON);
IMP_ENABLE_HW_CLOCK(SMI_LARB0, SMI_LARB0);
IMP_ENABLE_HW_CLOCK(CAM_MDP, CAM_MDP);
IMP_ENABLE_HW_CLOCK(MDP_RDMA0, MDP_RDMA);
IMP_ENABLE_HW_CLOCK(MDP_RSZ0, MDP_RSZ0);
IMP_ENABLE_HW_CLOCK(MDP_RSZ1, MDP_RSZ1);
IMP_ENABLE_HW_CLOCK(MDP_WDMA, MDP_WDMA);
IMP_ENABLE_HW_CLOCK(MDP_WROT0, MDP_WROT);
IMP_ENABLE_HW_CLOCK(MDP_TDSHP0, MDP_TDSHP);
IMP_HW_CLOCK_IS_ENABLE(CAM_MDP, CAM_MDP);
IMP_HW_CLOCK_IS_ENABLE(MDP_RDMA0, MDP_RDMA);
IMP_HW_CLOCK_IS_ENABLE(MDP_RSZ0, MDP_RSZ0);
IMP_HW_CLOCK_IS_ENABLE(MDP_RSZ1, MDP_RSZ1);
IMP_HW_CLOCK_IS_ENABLE(MDP_WDMA, MDP_WDMA);
IMP_HW_CLOCK_IS_ENABLE(MDP_WROT0, MDP_WROT);
IMP_HW_CLOCK_IS_ENABLE(MDP_TDSHP0, MDP_TDSHP);
#undef IMP_ENABLE_HW_CLOCK
#undef IMP_HW_CLOCK_IS_ENABLE

bool cmdq_dev_mdp_clock_is_on(CMDQ_ENG_ENUM engine)
{
	switch (engine) {
	case CMDQ_ENG_MDP_CAMIN:
		return cmdq_dev_clock_is_enable_CAM_MDP();
	case CMDQ_ENG_MDP_RDMA0:
		return cmdq_dev_clock_is_enable_MDP_RDMA0();
	case CMDQ_ENG_MDP_RSZ0:
		return cmdq_dev_clock_is_enable_MDP_RSZ0();
	case CMDQ_ENG_MDP_RSZ1:
		return cmdq_dev_clock_is_enable_MDP_RSZ1();
	case CMDQ_ENG_MDP_WDMA:
		return cmdq_dev_clock_is_enable_MDP_WDMA();
	case CMDQ_ENG_MDP_WROT0:
		return cmdq_dev_clock_is_enable_MDP_WROT0();
	case CMDQ_ENG_MDP_TDSHP0:
		return cmdq_dev_clock_is_enable_MDP_TDSHP0();
	default:
		CMDQ_ERR("try to query unknown mdp clock");
		return false;
	}
}

void cmdq_dev_enable_mdp_clock(bool enable, CMDQ_ENG_ENUM engine)
{
	switch (engine) {
	case CMDQ_ENG_MDP_CAMIN:
		cmdq_dev_enable_clock_CAM_MDP(enable);
		break;
	case CMDQ_ENG_MDP_RDMA0:
		cmdq_dev_enable_clock_MDP_RDMA0(enable);
		break;
	case CMDQ_ENG_MDP_RSZ0:
		cmdq_dev_enable_clock_MDP_RSZ0(enable);
		break;
	case CMDQ_ENG_MDP_RSZ1:
		cmdq_dev_enable_clock_MDP_RSZ1(enable);
		break;
	case CMDQ_ENG_MDP_WDMA:
		cmdq_dev_enable_clock_MDP_WDMA(enable);
		break;
	case CMDQ_ENG_MDP_WROT0:
		cmdq_dev_enable_clock_MDP_WROT0(enable);
		break;
	case CMDQ_ENG_MDP_TDSHP0:
		cmdq_dev_enable_clock_MDP_TDSHP0(enable);
		break;
	default:
		CMDQ_ERR("try to enable unknown mdp clock");
		break;
	}
}

struct device *cmdq_dev_get(void)
{
	return gCmdqDev.pDev;
}

const uint32_t cmdq_dev_get_irq_id(void)
{
	return gCmdqDev.irqId;
}

const uint32_t cmdq_dev_get_irq_secure_id(void)
{
	return gCmdqDev.irqSecId;
}

const long cmdq_dev_get_module_base_VA_GCE(void)
{
	return gCmdqDev.regBaseVA;
}

const long cmdq_dev_get_module_base_PA_GCE(void)
{
	return gCmdqDev.regBasePA;
}

const long cmdq_dev_get_module_base_VA_MMSYS_CONFIG(void)
{
	return gCmdqModuleBaseVA.MMSYS_CONFIG;
}

const long cmdq_dev_get_module_base_VA_MDP_RDMA(void)
{
	return gCmdqModuleBaseVA.MDP_RDMA;
}

const long cmdq_dev_get_module_base_VA_MDP_RSZ0(void)
{
	return gCmdqModuleBaseVA.MDP_RSZ0;
}

const long cmdq_dev_get_module_base_VA_MDP_RSZ1(void)
{
	return gCmdqModuleBaseVA.MDP_RSZ1;
}

const long cmdq_dev_get_module_base_VA_MDP_WDMA(void)
{
	return gCmdqModuleBaseVA.MDP_WDMA;
}

const long cmdq_dev_get_module_base_VA_MDP_WROT(void)
{
	return gCmdqModuleBaseVA.MDP_WROT;
}

const long cmdq_dev_get_module_base_VA_MDP_TDSHP(void)
{
	return gCmdqModuleBaseVA.MDP_TDSHP;
}

const long cmdq_dev_get_module_base_VA_MM_MUTEX(void)
{
	return gCmdqModuleBaseVA.MM_MUTEX;
}

const long cmdq_dev_get_module_base_VA_VENC(void)
{
	return gCmdqModuleBaseVA.VENC;
}

void cmdq_dev_init_module_PA_stat(void)
{
#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_INSTRUCTION_COUNT)
	int32_t i;
	CmdqModulePAStatStruct *modulePAStat = cmdq_core_Initial_and_get_module_stat();

	/* Get MM_SYS config registers range */
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MMSYS_CONFIG", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MMSYS_CONFIG],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MMSYS_CONFIG]);
	/* Get MDP module registers range */
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_RDMA", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_RDMA],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_RDMA]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_RSZ0", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_RSZ0],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_RSZ0]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_RSZ1", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_RSZ1],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_RSZ1]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_WDMA", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_WDMA],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_WDMA]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_WROT", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_WROT],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_WROT]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MDP_TDSHP", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MDP_TDSHP],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MDP_TDSHP]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,MM_MUTEX", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_MM_MUTEX],
					    &modulePAStat->end[CMDQ_MODULE_STAT_MM_MUTEX]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,VENC", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_VENC],
					    &modulePAStat->end[CMDQ_MODULE_STAT_VENC]);
	/* Get DISP module registers range */
	for (i = CMDQ_MODULE_STAT_DISP_OVL0; i <= CMDQ_MODULE_STAT_DISP_DPI0; i++) {
		cmdq_dev_get_module_PA_by_name_stat("mediatek,DISPSYS",
						    (i - CMDQ_MODULE_STAT_DISP_OVL0),
						    &modulePAStat->start[i], &modulePAStat->end[i]);
	}
	/* Get CAM module registers range */
	cmdq_dev_get_module_PA_by_name_stat("mediatek,CAM0", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_CAM0],
					    &modulePAStat->end[CMDQ_MODULE_STAT_CAM0]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,CAM1", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_CAM1],
					    &modulePAStat->end[CMDQ_MODULE_STAT_CAM1]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,CAM2", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_CAM2],
					    &modulePAStat->end[CMDQ_MODULE_STAT_CAM2]);
	cmdq_dev_get_module_PA_by_name_stat("mediatek,CAM3", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_CAM3],
					    &modulePAStat->end[CMDQ_MODULE_STAT_CAM3]);
	/* Get SODI registers range */
	cmdq_dev_get_module_PA_by_name_stat("mediatek,SLEEP", 0,
					    &modulePAStat->start[CMDQ_MODULE_STAT_SODI],
					    &modulePAStat->end[CMDQ_MODULE_STAT_SODI]);
#endif
}

void cmdq_dev_init_module_base_VA(void)
{
	memset(&gCmdqModuleBaseVA, 0, sizeof(CmdqModuleBaseVA));

#ifdef CMDQ_OF_SUPPORT
	gCmdqModuleBaseVA.MMSYS_CONFIG =
	    cmdq_dev_alloc_module_base_VA_by_name("mediatek,MMSYS_CONFIG");
	gCmdqModuleBaseVA.MDP_RDMA = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_RDMA");
	gCmdqModuleBaseVA.MDP_RSZ0 = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_RSZ0");
	gCmdqModuleBaseVA.MDP_RSZ1 = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_RSZ1");
	gCmdqModuleBaseVA.MDP_WDMA = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_WDMA");
	gCmdqModuleBaseVA.MDP_WROT = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_WROT");
	gCmdqModuleBaseVA.MDP_TDSHP = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MDP_TDSHP");
	gCmdqModuleBaseVA.MM_MUTEX = cmdq_dev_alloc_module_base_VA_by_name("mediatek,MM_MUTEX");
	gCmdqModuleBaseVA.VENC = cmdq_dev_alloc_module_base_VA_by_name("mediatek,VENC");
#else
	gCmdqModuleBaseVA.MMSYS_CONFIG = MMSYS_CONFIG_BASE;
	gCmdqModuleBaseVA.MDP_RDMA = MDP_RDMA_BASE;
	gCmdqModuleBaseVA.MDP_RSZ0 = MDP_RSZ0_BASE;
	gCmdqModuleBaseVA.MDP_RSZ1 = MDP_RSZ1_BASE;
	gCmdqModuleBaseVA.MDP_WDMA = MDP_WDMA_BASE;
	gCmdqModuleBaseVA.MDP_WROT = MDP_WROT_BASE;
	gCmdqModuleBaseVA.MDP_TDSHP = MDP_TDSHP_BASE;
	gCmdqModuleBaseVA.MM_MUTEX = MMSYS_MUTEX_BASE;
	gCmdqModuleBaseVA.VENC = VENC_BASE;
#endif
}

void cmdq_dev_deinit_module_base_VA(void)
{
#ifdef CMDQ_OF_SUPPORT
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MMSYS_CONFIG());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_RDMA());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_RSZ0());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_RSZ1());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_WDMA());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_WROT());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MDP_TDSHP());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_MM_MUTEX());
	cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_VENC());

	memset(&gCmdqModuleBaseVA, 0, sizeof(CmdqModuleBaseVA));
#else
	/* do nothing, registers' IOMAP will be destroyed by platform */
#endif
}

void cmdq_dev_init(struct platform_device *pDevice)
{
	struct device_node *node = pDevice->dev.of_node;

	/* init cmdq device dependent data */
	do {
		memset(&gCmdqDev, 0x0, sizeof(CmdqDeviceStruct));

		gCmdqDev.pDev = &pDevice->dev;
#ifdef CMDQ_OF_SUPPORT
		gCmdqDev.regBaseVA = (unsigned long)of_iomap(node, 0);
		gCmdqDev.regBasePA = (0L | 0x10212000);
		gCmdqDev.irqId = irq_of_parse_and_map(node, 0);
		gCmdqDev.irqSecId = irq_of_parse_and_map(node, 1);
#else
		gCmdqDev.regBaseVA = (0L | GCE_BASE);
		gCmdqDev.regBasePA = (0L | 0x10212000);
		gCmdqDev.irqId = CQ_DMA_IRQ_BIT_ID;
		gCmdqDev.irqSecId = CQ_DMA_SEC_IRQ_BIT_ID;
#endif

		CMDQ_LOG
		    ("[CMDQ] platform_dev: dev: %p, PA: %lx, VA: %lx, irqId: %d,  irqSecId:%d\n",
		     gCmdqDev.pDev, gCmdqDev.regBasePA, gCmdqDev.regBaseVA, gCmdqDev.irqId,
		     gCmdqDev.irqSecId);
	} while (0);

	/* init module VA */
	cmdq_dev_init_module_base_VA();
	/* init module PA for instruction count */
	cmdq_dev_init_module_PA_stat();
	/* init module HW event */
	cmdq_dev_init_module_HWEvent();
}

void cmdq_dev_deinit(void)
{
	cmdq_dev_deinit_module_base_VA();

	/* deinit cmdq device dependent data */
	do {
#ifdef CMDQ_OF_SUPPORT
		cmdq_dev_free_module_base_VA(cmdq_dev_get_module_base_VA_GCE());
		gCmdqDev.regBaseVA = 0;
#else
		/* do nothing */
#endif
	} while (0);
}

void cmdq_dev_platform_function_setting(void)
{
	cmdqDevFuncStruct *pFunc;

	pFunc = cmdq_dev_get_func();

	pFunc->devGet = cmdq_dev_get;
	pFunc->irqIndex = cmdq_dev_get_irq_id;
	pFunc->irqSecureIndex = cmdq_dev_get_irq_secure_id;

	pFunc->gceBaseVA = cmdq_dev_get_module_base_VA_GCE;
	pFunc->gceBasePA = cmdq_dev_get_module_base_PA_GCE;

	pFunc->devInit = cmdq_dev_init;
	pFunc->devDeinit = cmdq_dev_deinit;

	pFunc->enableGCEClock = cmdq_dev_enable_gce_clock;
	pFunc->enableMDPClock = cmdq_dev_enable_mdp_clock;
	pFunc->mdpClockIsOn = cmdq_dev_mdp_clock_is_on;
}
