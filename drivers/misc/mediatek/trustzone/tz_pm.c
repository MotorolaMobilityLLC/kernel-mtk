
#include <linux/module.h>

#include "trustzone/kree/system.h"
#include "tz_cross/ta_pm.h"

static KREE_SESSION_HANDLE pm_session;

void kree_pm_init(void)
{
	TZ_RESULT ret;

	ret = KREE_CreateSession(TZ_TA_PM_UUID, &pm_session);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("CreateSession error %d\n", ret);
}

#ifndef CONFIG_ARM64
void kree_pm_cpu_lowpower(volatile int *ppen_release, int logical_cpuid)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	param[0].mem.buffer = (void *)ppen_release;
	param[0].mem.size = sizeof(int);
	param[1].value.a = logical_cpuid;
	ret = KREE_TeeServiceCall(pm_session, TZCMD_PM_CPU_LOWPOWER,
					TZ_ParamTypes2(TZPT_MEM_INPUT,
							TZPT_VALUE_INPUT),
					param);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("%s error: %s\n", __func__, TZ_GetErrorString(ret));
}

int kree_pm_cpu_dormant(int mode)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	param[0].value.a = mode;
	ret = KREE_TeeServiceCall(pm_session, TZCMD_PM_CPU_DORMANT,
				  TZ_ParamTypes1(TZPT_VALUE_INPUT), param);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("%s error: %s\n", __func__, TZ_GetErrorString(ret));

	return 0;
}
#endif

int kree_pm_device_ops(int state)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	param[0].value.a = (uint32_t) state;
	ret = KREE_TeeServiceCall(pm_session, TZCMD_PM_DEVICE_OPS,
				  TZ_ParamTypes1(TZPT_VALUE_INPUT), param);
	if (ret != TZ_RESULT_SUCCESS)
		pr_warn("%s error: %s\n", __func__, TZ_GetErrorString(ret));

	return ret;
}
