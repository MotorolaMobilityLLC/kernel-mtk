#ifndef __CMDQ_DEVICE_COMMON_H__
#define __CMDQ_DEVICE_COMMON_H__

#include <linux/platform_device.h>
#include <linux/device.h>
#include "cmdq_def.h"

#ifndef CMDQ_USE_CCF
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif				/* !defined(CMDQ_USE_CCF) */

/* device get */
typedef struct device *(*CmdqDevGet) (void);

/* get IRQ index*/
typedef const uint32_t(*CmdqDevIRQIndex) (void);

/* get secure IRQ index*/
typedef const uint32_t(*CmdqDevIRQSecureIndex) (void);

/* get GCE base VA */
typedef const long (*CmdqDevGCEBaseVA) (void);

/* get GCE base PA  */
typedef const long (*CmdqDevGCEBasePA) (void);

/* device initialization */
typedef void (*CmdqDevInit) (struct platform_device *pDevice);

/* device de-initialization */
typedef void (*CmdqDevDeinit) (void);

/* enable GCE clock */
typedef void (*CmdqEnableGCEClock) (bool enable);

/* enable MDP clock  */
typedef void (*CmdqEnableMDPClock) (bool enable, CMDQ_ENG_ENUM engine);

/* query MDP clock is on   */
typedef bool(*CmdqMDPClockIsOn) (CMDQ_ENG_ENUM engine);

typedef struct cmdqDevFuncStruct {
	CmdqDevGet devGet;
	CmdqDevIRQIndex irqIndex;
	CmdqDevIRQSecureIndex irqSecureIndex;
	CmdqDevGCEBaseVA gceBaseVA;
	CmdqDevGCEBaseVA gceBasePA;
	CmdqDevInit devInit;
	CmdqDevDeinit devDeinit;
	CmdqEnableGCEClock enableGCEClock;
	CmdqEnableMDPClock enableMDPClock;
	CmdqMDPClockIsOn mdpClockIsOn;
} cmdqDevFuncStruct;

void cmdq_dev_virtual_function_setting(void);
cmdqDevFuncStruct *cmdq_dev_get_func(void);

#ifndef CMDQ_USE_CCF
uint32_t cmdq_dev_enable_mtk_clock(bool enable, enum cg_clk_id gateId, char *name);
bool cmdq_dev_mtk_clock_is_enable(enum cg_clk_id gateId);
void testcase_clkmgr_impl(enum cg_clk_id gateId,
			  char *name,
			  const unsigned long testWriteReg,
			  const uint32_t testWriteValue,
			  const unsigned long testReadReg, const bool verifyWriteResult);

#else
void cmdq_dev_get_module_clock_by_name(const char *name, const char *clkName,
				       struct clk **clk_module);
uint32_t cmdq_dev_enable_device_clock(bool enable, struct clk *clk_module, const char *clkName);
bool cmdq_dev_device_clock_is_enable(struct clk *clk_module);
#endif				/* !defined(CMDQ_USE_CCF) */

const long cmdq_dev_alloc_module_base_VA_by_name(const char *name);
void cmdq_dev_free_module_base_VA(const long VA);

#ifdef CMDQ_INSTRUCTION_COUNT
void cmdq_dev_get_module_PA_by_name_stat(const char *name, int index, long *startPA, long *endPA);
#endif

void cmdq_dev_init_module_HWEvent(void);
void cmdq_dev_test_event_correctness(void);

#endif				/* __CMDQ_DEVICE_COMMON_H__ */
