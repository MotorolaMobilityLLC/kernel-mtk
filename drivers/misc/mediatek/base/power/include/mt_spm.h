#ifndef __MT_SPM_H__
#define __MT_SPM_H__

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6797)

#include "spm_v2/mt_spm.h"

#elif defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6735M) || defined(CONFIG_ARCH_MT6753)

#elif defined(CONFIG_ARCH_MT6580)

#include "spm_v1/mt_spm_mt6580.h"

#endif

#endif /* __MT_SPM_H__ */

