#ifndef __MT_SPM_VCOREFS__H__
#define __MT_SPM_VCOREFS__H__

#include "mt_spm.h"

#define SPM_VCORE_DVFS_EN	0
#define DYNAMIC_LOAD		0
#define DRAM_DUMMY_READ		0
#define DRAM_SHUFFLE_CONFIG	0

/* FIXME: */
#define SPM_DVFS_TIMEOUT	5000	/* 5000us */

#define WAKE_SRC_FOR_VCOREFS             \
	(WAKE_SRC_R12_PCM_TIMER)

void spm_go_to_vcorefs(int spm_flags);
int spm_set_vcore_dvfs(int opp);
void spm_vcorefs_init(void);
int spm_dvfs_flag_init(void);
char *spm_vcorefs_dump_dvfs_regs(char *p);
void spm_to_sspm_pwarp_cmd(void);

extern struct dram_info *g_dram_info_dummy_read;

#endif /* __MT_SPM_VCOREFS__H__ */
