/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MTK_HIBERNATE_CORE_H__
#define __MTK_HIBERNATE_CORE_H__

#ifdef CONFIG_PM_AUTOSLEEP

/* kernel/power/autosleep.c */
extern int pm_autosleep_lock(void);
extern void pm_autosleep_unlock(void);
extern suspend_state_t pm_autosleep_state(void);

#else				/* !CONFIG_PM_AUTOSLEEP */

static inline int pm_autosleep_lock(void)
{
	return 0;
}

static inline void pm_autosleep_unlock(void)
{
}

static inline suspend_state_t pm_autosleep_state(void)
{
	return PM_SUSPEND_ON;
}

#endif				/* !CONFIG_PM_AUTOSLEEP */

#ifdef CONFIG_PM_WAKELOCKS
/* kernel/power/wakelock.c */
int pm_wake_lock(const char *buf);
int pm_wake_unlock(const char *buf);
#else
static inline int pm_wake_lock(const char *buf)
{
	return 0;
}

static inline int pm_wake_unlock(const char *buf)
{
	return 0;
}
#endif				/* !CONFIG_PM_WAKELOCKS */

/* HOTPLUG */
#if defined(CONFIG_CPU_FREQ_GOV_HOTPLUG) || defined(CONFIG_CPU_FREQ_GOV_BALANCE)
extern void hp_set_dynamic_cpu_hotplug_enable(int enable);
extern struct mutex hp_onoff_mutex;
#endif				/* CONFIG_CPU_FREQ_GOV_HOTPLUG || CONFIG_CPU_FREQ_GOV_BALANCE */

#endif /* __MTK_HIBERNATE_CORE_H__ */
