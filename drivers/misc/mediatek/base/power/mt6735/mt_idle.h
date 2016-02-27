#ifndef __MT_IDLE_H__
#define __MT_IDLE_H__

#include <linux/types.h>

enum idle_lock_spm_id {
	IDLE_SPM_LOCK_VCORE_DVFS = 0,
};
extern void idle_lock_spm(enum idle_lock_spm_id id);
extern void idle_unlock_spm(enum idle_lock_spm_id id);

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);
extern void enable_soidle_by_bit(int id);
extern void disable_soidle_by_bit(int id);

extern void defeature_soidle_by_display(void);

#if defined(EN_PTP_OD) && EN_PTP_OD
extern u32 ptp_data[3];
#endif

#endif /* __MT_IDLE_H__ */
