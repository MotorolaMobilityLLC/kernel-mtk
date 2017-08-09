#include "cmdq_device.h"
#include "cmdq_core.h"

/* device tree */
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>

/**************************************************************************************/
/*******************                    Platform dependent function                    ********************/
/**************************************************************************************/
struct device *cmdq_dev_get_virtual(void)
{
	struct device *pDev = NULL;
	return pDev;
}

const uint32_t cmdq_dev_get_irq_id_virtual(void)
{
	return 0;
}

const uint32_t cmdq_dev_get_irq_secure_id_virtual(void)
{
	return 0;
}

const long cmdq_dev_get_module_base_VA_GCE_virtual(void)
{
	return 0L;
}

const long cmdq_dev_get_module_base_PA_GCE_virtual(void)
{
	return 0L;
}

void cmdq_dev_init_virtual(struct platform_device *pDevice)
{
}

void cmdq_dev_deinit_virtual(void)
{
}

void cmdq_dev_enable_gce_clock_virtual(bool enable)
{
}

void cmdq_dev_enable_mdp_clock_virtual(bool enable, CMDQ_ENG_ENUM engine)
{
}

bool cmdq_dev_mdp_clock_is_on_virtual(CMDQ_ENG_ENUM engine)
{
	return false;
}

/**************************************************************************************/
/************************                      Common Code                      ************************/
/**************************************************************************************/
static cmdqDevFuncStruct gDeviceFunctionPointer;

void cmdq_dev_virtual_function_setting(void)
{
	cmdqDevFuncStruct *pFunc;

	pFunc = &(gDeviceFunctionPointer);

	pFunc->devGet = cmdq_dev_get_virtual;
	pFunc->irqIndex = cmdq_dev_get_irq_id_virtual;
	pFunc->irqSecureIndex = cmdq_dev_get_irq_secure_id_virtual;

	pFunc->gceBaseVA = cmdq_dev_get_module_base_VA_GCE_virtual;
	pFunc->gceBasePA = cmdq_dev_get_module_base_PA_GCE_virtual;

	pFunc->devInit = cmdq_dev_init_virtual;
	pFunc->devDeinit = cmdq_dev_deinit_virtual;

	pFunc->enableGCEClock = cmdq_dev_enable_gce_clock_virtual;
	pFunc->enableMDPClock = cmdq_dev_enable_mdp_clock_virtual;
	pFunc->mdpClockIsOn = cmdq_dev_mdp_clock_is_on_virtual;
}

cmdqDevFuncStruct *cmdq_dev_get_func(void)
{
	return &gDeviceFunctionPointer;
}

#ifndef CMDQ_USE_CCF
uint32_t cmdq_dev_enable_mtk_clock(bool enable, enum cg_clk_id gateId, char *name)
{
	if (enable)
		enable_clock(gateId, name);
	else
		disable_clock(gateId, name);
	return 0;
}

bool cmdq_dev_mtk_clock_is_enable(enum cg_clk_id gateId)
{
	return clock_is_on(gateId);
}

#else

void cmdq_dev_get_module_clock_by_dev(struct device *dev, const char *clkName,
				      struct clk **clk_module)
{
	*clk_module = devm_clk_get(dev, clkName);
	if (IS_ERR(*clk_module)) {
		/* error status print */
		CMDQ_ERR("DEV: cannot get module clock: %s\n", clkName);
	} else {
		/* message print */
		CMDQ_MSG("DEV: get module clock: %s\n", clkName);
	}
}

void cmdq_dev_get_module_clock_by_name(const char *name, const char *clkName,
				       struct clk **clk_module)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, name);

	*clk_module = of_clk_get_by_name(node, clkName);
	if (IS_ERR(*clk_module)) {
		/* error status print */
		CMDQ_ERR("DEV: byName: cannot get module clock: %s\n", clkName);
	} else {
		/* message print */
		CMDQ_MSG("DEV: byName: get module clock: %s\n", clkName);
	}
}

uint32_t cmdq_dev_enable_device_clock(bool enable, struct clk *clk_module, const char *clkName)
{
	int result = 0;

	if (enable) {
		result = clk_prepare_enable(clk_module);
		CMDQ_MSG("enable clock with module: %s, result: %d\n", clkName, result);
	} else {
		clk_disable_unprepare(clk_module);
		CMDQ_MSG("disable clock with module: %s\n", clkName);
	}
	return result;
}

bool cmdq_dev_device_clock_is_enable(struct clk *clk_module)
{
	return true;
}

#endif				/* !defined(CMDQ_USE_CCF) */

const long cmdq_dev_alloc_module_base_VA_by_name(const char *name)
{
	unsigned long VA;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	VA = (unsigned long)of_iomap(node, 0);
	CMDQ_LOG("DEV: VA(%s): 0x%lx\n", name, VA);
	return VA;
}

void cmdq_dev_free_module_base_VA(const long VA)
{
	iounmap((void *)VA);
}

#ifdef CMDQ_INSTRUCTION_COUNT
void cmdq_dev_get_module_PA_by_name_stat(const char *name, int index, long *startPA, long *endPA)
{
	struct device_node *node = NULL;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, name);
	of_address_to_resource(node, index, &res);
	*startPA = res.start;
	*endPA = res.end;
	CMDQ_LOG("DEV: PA(%s): start = 0x%lx, end = 0x%lx\n", name, *startPA, *endPA);
}
#endif

#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_DVENT_FROM_DTS)
void cmdq_dev_get_event_value_by_name(CMDQ_EVENT_ENUM event, const char *dts_name)
{
	int status;
	uint32_t event_value;
	struct device_node *node = NULL;

	do {
		if (event < 0 || event >= CMDQ_MAX_HW_EVENT_COUNT)
			break;

		node = of_find_compatible_node(NULL, NULL, "mediatek,GCE");

		status = of_property_read_u32(node, dts_name, &event_value);
		if (status < 0)
			break;

		cmdq_core_set_HW_event_table(event, event_value);
	} while (0);
}

void cmdq_dev_test_event_correctness_impl(CMDQ_EVENT_ENUM event, const char *event_name)
{
	int32_t eventValue = cmdq_core_get_event_value(event);

	if (eventValue >= 0 && eventValue < CMDQ_SYNC_TOKEN_MAX) {
		/* print event name from device tree */
		CMDQ_LOG("%s = %d\n", event_name, eventValue);
	}
}
#endif

void cmdq_dev_init_module_HWEvent(void)
{
#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_DVENT_FROM_DTS)
	cmdq_core_init_HW_event_table();

#undef DECLARE_CMDQ_EVENT
#define DECLARE_CMDQ_EVENT(name, val, dts_name) \
{	\
	cmdq_dev_get_event_value_by_name(val, #dts_name);	\
}
#include "cmdq_event_common.h"
#undef DECLARE_CMDQ_EVENT
#endif
}

void cmdq_dev_test_event_correctness(void)
{
#if defined(CMDQ_OF_SUPPORT) && defined(CMDQ_DVENT_FROM_DTS)
#undef DECLARE_CMDQ_EVENT
#define DECLARE_CMDQ_EVENT(name, val, dts_name) \
{	\
		cmdq_dev_test_event_correctness_impl(val, #name);	\
}
#include "cmdq_event_common.h"
#undef DECLARE_CMDQ_EVENT
#endif
}
