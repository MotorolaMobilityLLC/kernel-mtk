#ifndef _MT_VCOREFS_MANAGER_H
#define _MT_VCOREFS_MANAGER_H

#include "mt_vcorefs_governor.h"

/* Manager extern API */
extern int is_vcorefs_can_work(void)  __attribute__((weak));
extern int vcorefs_get_curr_opp(void)  __attribute__((weak));

/* Main function API (Called by kicker request for DVFS) */
extern int vcorefs_request_dvfs_opp(enum dvfs_kicker, enum dvfs_opp) __attribute__((weak));

/* Called by governor for init flow */
extern void vcorefs_drv_init(bool, bool, int) __attribute__((weak));
extern int init_vcorefs_sysfs(void) __attribute__((weak));

/* Called by MET */
typedef void (*vcorefs_req_handler_t)(enum dvfs_kicker kicker, enum dvfs_opp opp);
extern void vcorefs_register_req_notify(vcorefs_req_handler_t handler) __attribute__((weak));

#endif				/* _MT_VCOREFS_MANAGER_H */
