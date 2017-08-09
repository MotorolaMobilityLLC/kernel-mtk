#ifndef _MT_IDLE_H
#define _MT_IDLE_H

#include <linux/notifier.h>

enum idle_lock_spm_id {
	IDLE_SPM_LOCK_VCORE_DVFS = 0,
};
extern void idle_lock_spm(enum idle_lock_spm_id id);
extern void idle_unlock_spm(enum idle_lock_spm_id id);

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);

extern void enable_soidle3_by_bit(int id);
extern void disable_soidle3_by_bit(int id);

extern void enable_soidle_by_bit(int id);
extern void disable_soidle_by_bit(int id);

#define DPIDLE_START    1
#define DPIDLE_END      2
#define SOIDLE_START    3
#define SOIDLE_END      4
extern int mtk_idle_notifier_register(struct notifier_block *n);
extern void mtk_idle_notifier_unregister(struct notifier_block *n);

#endif
