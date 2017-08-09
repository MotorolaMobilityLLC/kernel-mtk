#ifndef __CMDQ_DEVICE_H__
#define __CMDQ_DEVICE_H__

#include <linux/platform_device.h>
#include <linux/device.h>
#include "cmdq_def.h"
#include "cmdq_mdp_common.h"

#ifndef CMDQ_USE_CCF
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif				/* !defined(CMDQ_USE_CCF) */

#define DECLARE_ENABLE_HW_CLOCK(HW_NAME) uint32_t cmdq_dev_enable_clock_##HW_NAME(bool enable)
DECLARE_ENABLE_HW_CLOCK(SMI_COMMON);
DECLARE_ENABLE_HW_CLOCK(SMI_LARB0);
#ifdef CMDQ_USE_LEGACY
DECLARE_ENABLE_HW_CLOCK(MUTEX_32K);
#endif
#undef DECLARE_ENABLE_HW_CLOCK

#ifndef CMDQ_USE_CCF
typedef enum cg_clk_id cgCLKID;
uint32_t cmdq_dev_enable_mtk_clock(bool enable, cgCLKID gateId, char *name);
bool cmdq_dev_mtk_clock_is_enable(cgCLKID gateId);
/* For test case used */
void testcase_clkmgr_impl(cgCLKID gateId,
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

struct device *cmdq_dev_get(void);
/* interrupt index */
const uint32_t cmdq_dev_get_irq_id(void);
const uint32_t cmdq_dev_get_irq_secure_id(void);
/* GCE clock */
void cmdq_dev_enable_gce_clock(bool enable);
bool cmdq_dev_gce_clock_is_enable(void);
/* virtual address */
const long cmdq_dev_get_module_base_VA_GCE(void);
const long cmdq_dev_get_module_base_VA_MMSYS_CONFIG(void);
const long cmdq_dev_alloc_module_base_VA_by_name(const char *name);
/* Other modules information */
void cmdq_dev_free_module_base_VA(const long VA);
const long cmdq_dev_get_APXGPT2_count(void);
/* physical address */
void cmdq_dev_get_module_PA(const char *name, int index, long *startPA, long *endPA);
const long cmdq_dev_get_module_base_PA_GCE(void);
/* GCE event */
void cmdq_dev_init_event_table(struct device_node *node);
void cmdq_dev_test_dts_correctness(void);
/* device initialization / deinitialization */
void cmdq_dev_init(struct platform_device *pDevice);
void cmdq_dev_deinit(void);

#endif				/* __CMDQ_DEVICE_H__ */
