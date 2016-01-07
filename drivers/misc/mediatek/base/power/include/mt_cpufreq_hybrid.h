#ifndef _MT_CPUFREQ_HYBRID_
#define _MT_CPUFREQ_HYBRID_

#include <linux/kernel.h>


/**************************************
 * [Hybrid DVFS] Config
 **************************************/
#if defined(CONFIG_ARCH_MT6755)
#define CONFIG_HYBRID_CPU_DVFS
/*#define __TRIAL_RUN__*/

#elif defined(CONFIG_ARCH_MT6797)
/*#define CONFIG_HYBRID_CPU_DVFS*/
/*#define __TRIAL_RUN__*/
#endif


/**************************************
 * [Hybrid DVFS] Parameter
 **************************************/
#if defined(CONFIG_ARCH_MT6797)
enum cpu_cluster {
	CPU_CLUSTER_LL,
	CPU_CLUSTER_L,
	CPU_CLUSTER_B,
	CPU_CLUSTER_CCI,	/* virtual */
	NUM_CPU_CLUSTER
};

#define NUM_PHY_CLUSTER		(NUM_CPU_CLUSTER - 1)
#define NUM_CPU_OPP		16

#else	/* CONFIG_ARCH_MT6755 */
enum cpu_cluster {
	CPU_CLUSTER_LL,
	CPU_CLUSTER_L,
	NUM_CPU_CLUSTER
};

#define NUM_PHY_CLUSTER		NUM_CPU_CLUSTER
#define NUM_CPU_OPP		8
#endif


/**************************************
 * [Hybrid DVFS] Definition
 **************************************/
#define OPP_AT_SUSPEND		UINT_MAX
#define VOLT_AT_SUSPEND		UINT_MAX
#define CEILING_AT_SUSPEND	UINT_MAX
#define FLOOR_AT_SUSPEND	UINT_MAX

enum sema_user {
	SEMA_FHCTL_DRV,
	SEMA_I2C_DRV,
	NUM_SEMA_USER
};

enum pause_src {
	PAUSE_INIT,
	PAUSE_I2CDRV,
	PAUSE_IDLE,
	PAUSE_SUSPEND,
	NUM_PAUSE_SRC
};

struct init_sta {
	unsigned int opp[NUM_CPU_CLUSTER];	/* SW index */
	unsigned int freq[NUM_CPU_CLUSTER];	/* KHz */
	unsigned int volt[NUM_CPU_CLUSTER];	/* PMIC value */
	unsigned int ceiling[NUM_CPU_CLUSTER];	/* SW index */
	unsigned int floor[NUM_CPU_CLUSTER];	/* SW index */
	bool is_on[NUM_CPU_CLUSTER];		/* on/off */
};


/**************************************
 * [Hybrid DVFS] Macro / Inline
 **************************************/
static inline unsigned int opp_limit_to_ceiling(int limit)
{
	unsigned int ceiling;

	if (limit >= 0)
		ceiling = (limit < NUM_CPU_OPP ? limit : NUM_CPU_OPP - 1);
	else	/* no limit */
		ceiling = 0;

	return ceiling;
}

static inline unsigned int opp_limit_to_floor(int limit)
{
	unsigned int floor;

	if (limit >= 0)
		floor = (limit < NUM_CPU_OPP ? limit : NUM_CPU_OPP - 1);
	else	/* no limit */
		floor = NUM_CPU_OPP - 1;

	return floor;
}


/**************************************
 * [Hybrid DVFS] API
 **************************************/
#ifdef CONFIG_HYBRID_CPU_DVFS
extern int cpuhvfs_module_init(void);
extern int cpuhvfs_kick_dvfsp_to_run(struct init_sta *sta);

extern void cpuhvfs_notify_cluster_on(unsigned int cluster);
extern void cpuhvfs_notify_cluster_off(unsigned int cluster);

extern int cpuhvfs_set_target_opp(unsigned int cluster, unsigned int index, unsigned int *ret_volt);
extern unsigned int cpuhvfs_get_curr_volt(unsigned int cluster);

extern int cpuhvfs_get_dvfsp_semaphore(enum sema_user user);
extern void cpuhvfs_release_dvfsp_semaphore(enum sema_user user);

extern int cpuhvfs_pause_dvfsp_running(enum pause_src src);
extern void cpuhvfs_unpause_dvfsp_to_run(enum pause_src src);

extern int cpuhvfs_stop_dvfsp_running(void);
extern int cpuhvfs_restart_dvfsp_running(struct init_sta *sta);

extern int cpuhvfs_dvfsp_suspend(void);
extern void cpuhvfs_dvfsp_resume(unsigned int on_cluster, struct init_sta *sta);

extern void cpuhvfs_dump_dvfsp_info(void);
#else
static inline int cpuhvfs_module_init(void)		{ return -ENODEV; }
static inline int cpuhvfs_kick_dvfsp_to_run(struct init_sta *sta)	{ return -ENODEV; }

static inline void cpuhvfs_notify_cluster_on(unsigned int cluster)	{}
static inline void cpuhvfs_notify_cluster_off(unsigned int cluster)	{ BUG(); }

static inline int cpuhvfs_set_target_opp(unsigned int cluster, unsigned int index,
					 unsigned int *ret_volt)	{ return -ENODEV; }
static inline unsigned int cpuhvfs_get_curr_volt(unsigned int cluster)	{ return UINT_MAX; }

static inline int cpuhvfs_get_dvfsp_semaphore(enum sema_user user)	{ return 0; }
static inline void cpuhvfs_release_dvfsp_semaphore(enum sema_user user)	{}

static inline int cpuhvfs_pause_dvfsp_running(enum pause_src src)	{ return 0; }
static inline void cpuhvfs_unpause_dvfsp_to_run(enum pause_src src)	{}

static inline int cpuhvfs_stop_dvfsp_running(void)			{ return 0; }
static inline int cpuhvfs_restart_dvfsp_running(struct init_sta *sta)	{ return -ENODEV; }

static inline int cpuhvfs_dvfsp_suspend(void)		{ return 0; }
static inline void cpuhvfs_dvfsp_resume(unsigned int on_cluster, struct init_sta *sta)	{}

static inline void cpuhvfs_dump_dvfsp_info(void)	{}
#endif

#endif	/* _MT_CPUFREQ_HYBRID_ */
