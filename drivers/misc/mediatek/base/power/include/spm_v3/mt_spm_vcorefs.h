#ifndef __MT_SPM_VCOREFS__H__
#define __MT_SPM_VCOREFS__H__

#include "mt_spm.h"

#define SPM_VCORE_DVFS_EN	0
#define DYNAMIC_LOAD		0

/* FIXME: */
#define SPM_DVFS_TIMEOUT	1000	/* 1ms */

#define MSDC1	1
#define MSDC3	3

#define WAKE_SRC_FOR_VCOREFS             \
	(WAKE_SRC_R12_PCM_TIMER)

void spm_go_to_vcorefs(int spm_flags);
int spm_set_vcore_dvfs(int opp);
void spm_vcorefs_init(void);
int spm_dvfs_flag_init(void);
char *spm_vcorefs_dump_dvfs_regs(char *p);
void spm_to_sspm_pwarp_cmd(void);
void spm_msdc_setting(int msdc, bool status);

#endif /* __MT_SPM_VCOREFS__H__ */
