#ifndef __MT_SPM_REG_H___
#define __MT_SPM_REG_H___

#if defined(CONFIG_ARCH_MT6755)

#include "spm_v2/mt_spm_vcore_dvfs_mt6755.h"

#elif defined(CONFIG_ARCH_MT6797)

#include "spm_v2/mt_spm_vcore_dvfs_mt6797.h"

#elif defined(CONFIG_ARCH_ELBRUS)

#include "spm_v3/mt_spm_vcorefs.h"

#endif

#endif /* __MT_SPM_REG_H___ */

