#ifndef __CMDQ_DEVICE_H__
#define __CMDQ_DEVICE_H__

#include <linux/platform_device.h>
#include <linux/device.h>

const long cmdq_dev_get_module_base_VA_MMSYS_CONFIG(void);
const long cmdq_dev_get_module_base_VA_MDP_RDMA(void);
const long cmdq_dev_get_module_base_VA_MDP_RSZ0(void);
const long cmdq_dev_get_module_base_VA_MDP_RSZ1(void);
const long cmdq_dev_get_module_base_VA_MDP_WDMA(void);
const long cmdq_dev_get_module_base_VA_MDP_WROT(void);
const long cmdq_dev_get_module_base_VA_MDP_TDSHP(void);
const long cmdq_dev_get_module_base_VA_MM_MUTEX(void);
const long cmdq_dev_get_module_base_VA_VENC(void);

#define DECLARE_ENABLE_HW_CLOCK(HW_NAME) uint32_t cmdq_dev_enable_clock_##HW_NAME(bool enable)
DECLARE_ENABLE_HW_CLOCK(SMI_COMMON);
DECLARE_ENABLE_HW_CLOCK(SMI_LARB0);
DECLARE_ENABLE_HW_CLOCK(CAM_MDP);
DECLARE_ENABLE_HW_CLOCK(MDP_RDMA0);
DECLARE_ENABLE_HW_CLOCK(MDP_RSZ0);
DECLARE_ENABLE_HW_CLOCK(MDP_RSZ1);
DECLARE_ENABLE_HW_CLOCK(MDP_WDMA);
DECLARE_ENABLE_HW_CLOCK(MDP_WROT0);
DECLARE_ENABLE_HW_CLOCK(MDP_TDSHP0);
#undef DECLARE_ENABLE_HW_CLOCK

/* virtual function setting */
void cmdq_dev_platform_function_setting(void);

#include "../cmdq_device_common.h"

#endif				/* __CMDQ_DEVICE_H__ */
