/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _SCHED_MTK_H
#define _SCHED_MTK_H

extern int set_moto_sched_enabled(int enable);

struct msched_ops {
	int (*task_get_mvp_prio)(struct task_struct *p, bool with_inherit);
	unsigned int (*task_get_mvp_limit)(struct task_struct *p, int mvp_prio);
	void (*binder_inherit_ux_type)(struct task_struct *task);
	void (*binder_clear_inherited_ux_type)(struct task_struct *task);
	void (*binder_ux_type_set)(struct task_struct *task);
	void (*queue_ux_task)(struct rq *rq, struct task_struct *task, int enqueue);
};

extern struct msched_ops *moto_sched_ops;
extern void set_moto_sched_ops(struct msched_ops *ops);

#if IS_ENABLED(CONFIG_SCHED_MOTO_UNFAIR)
static inline int moto_task_get_mvp_prio(struct task_struct *p, bool with_inherit) {
	if (moto_sched_ops != NULL && moto_sched_ops->task_get_mvp_prio != NULL)
		return moto_sched_ops->task_get_mvp_prio(p, with_inherit);

	return -1;
}

static inline unsigned int moto_task_get_mvp_limit(struct task_struct *p, int mvp_prio) {
	if (moto_sched_ops != NULL && moto_sched_ops->task_get_mvp_limit != NULL)
		return moto_sched_ops->task_get_mvp_limit(p, mvp_prio);

	return 0;
}

static inline void moto_binder_inherit_ux_type(struct task_struct *task) {
	if (moto_sched_ops != NULL && moto_sched_ops->binder_inherit_ux_type != NULL)
		return moto_sched_ops->binder_inherit_ux_type(task);
}

static inline void moto_binder_clear_inherited_ux_type(struct task_struct *task) {
	if (moto_sched_ops != NULL && moto_sched_ops->binder_clear_inherited_ux_type != NULL)
		return moto_sched_ops->binder_clear_inherited_ux_type(task);
}

static inline void moto_binder_ux_type_set(struct task_struct *task) {
	if (moto_sched_ops != NULL && moto_sched_ops->binder_ux_type_set != NULL)
		return moto_sched_ops->binder_ux_type_set(task);
}

static inline void moto_queue_ux_task(struct rq *rq, struct task_struct *task, int enqueue) {
	if (moto_sched_ops != NULL && moto_sched_ops->queue_ux_task != NULL)
		return moto_sched_ops->queue_ux_task(rq, task, enqueue);
}
#endif

#endif /* _SCHED_MTK_H */
