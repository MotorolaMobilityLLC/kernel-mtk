#ifndef __MT_SPM_DPIDLE__H__
#define __MT_SPM_DPIDLE__H__

#if defined(CONFIG_ARCH_MT6755)

#include "mt_spm_dpidle_mt6755.h"

#elif defined(CONFIG_ARCH_MT6797)

#include "mt_spm_dpidle_mt6797.h"

#endif

extern void spm_dpidle_pre_process(void);
extern void spm_dpidle_post_process(void);
extern void spm_deepidle_chip_init(void);

#endif /* __MT_SPM_DPIDLE__H__ */

