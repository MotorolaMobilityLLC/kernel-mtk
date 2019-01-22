/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(CONFIG_MTK_FPSGO) && !defined(CONFIG_MTK_CM_MGR)
void update_pwd_tbl(void);
int reduce_stall(int, int, int);
#else
static inline void update_pwd_tbl(void) { }
static inline int reduce_stall(int bv, int thres, int flag) { return -1; }
#endif

/* extern sysctl_sched_migration_cost for /proc/sys/kernel/sched_migration_cost_ns */
extern unsigned int sysctl_sched_migration_cost;

#ifdef CONFIG_MACH_MT6771
void eas_ctrl_extend_notify_init(void);
void ext_launch_start(void);
void ext_launch_end(void);
#else
static inline void eas_ctrl_extend_notify_init(void) { }
static inline void ext_launch_start(void) { }
static inline void ext_launch_end(void) { }
#endif
