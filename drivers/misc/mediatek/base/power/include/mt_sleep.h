#ifndef __MT_SLEEP_H__
#define __MT_SLEEP_H__

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6797)

#include "spm_v2/mt_sleep.h"

#elif defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6735M) || defined(CONFIG_ARCH_MT6753)

#include "../mt6735/mt_sleep.h"

#elif defined(CONFIG_ARCH_MT6580)

#include "spm_v1/mt_sleep.h"

#endif

#endif /* __MT_SLEEP_H__ */

