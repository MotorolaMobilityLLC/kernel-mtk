#ifndef __MEMORY_LOWPOWER_INTERNAL_H
#define __MEMORY_LOWPOWER_INTERNAL_H

/* Memory Lowpower State & Action */
enum power_state {
	MLP_INIT,		/* Memory-lowpower is initialized */
	MLP_SCREENON,
	MLP_SCREENOFF,
	MLP_SCREENIDLE,
	MLP_ENABLE,
	MLP_ENABLE_DCS,
	MLP_ENABLE_PASR,
};

#define TEST_MEMORY_LOWPOWER_STATE(uname, lname) \
static inline int Mlps##uname(unsigned long *state) \
		{ return test_bit(MLP_##lname, state); }

#define SET_MEMORY_LOWPOWER_STATE(uname, lname)	\
static inline void SetMlps##uname(unsigned long *state) \
			{ set_bit(MLP_##lname, state); }

#define CLEAR_MEMORY_LOWPOWER_STATE(uname, lname) \
static inline void ClearMlps##uname(unsigned long *state) \
			{ clear_bit(MLP_##lname, state); }

#define MEMORY_LOWPOWER_STATE(uname, lname) (TEST_MEMORY_LOWPOWER_STATE(uname, lname) \
	SET_MEMORY_LOWPOWER_STATE(uname, lname)	CLEAR_MEMORY_LOWPOWER_STATE(uname, lname))

MEMORY_LOWPOWER_STATE(Init, INIT)
MEMORY_LOWPOWER_STATE(ScreenOn, SCREENON)	/*MEMORY_LOWPOWER_STATE(ScreenOff, SCREENOFF)*/
MEMORY_LOWPOWER_STATE(ScreenIdle, SCREENIDLE)
MEMORY_LOWPOWER_STATE(Enable, ENABLE)
MEMORY_LOWPOWER_STATE(EnableDCS, ENABLE_DCS)
MEMORY_LOWPOWER_STATE(EnablePASR, ENABLE_PASR)

#define IS_ACTION_SCREENON(action)	(action == MLP_SCREENON)
#define IS_ACTION_SCREENOFF(action)	(action == MLP_SCREENOFF)
#define IS_ACTION_SCREENIDLE(action)	(action == MLP_SCREENIDLE)

/* Memory Lowpower Features & their operations */
enum power_level {
	MLP_LEVEL_DCS,
	MLP_LEVEL_PASR,
	NR_MLP_LEVEL,
};

typedef void (*get_range_t) (int, unsigned long *, unsigned long *);

/* Feature specific operations */
struct memory_lowpower_operation {
	struct list_head link;
	enum power_level level;
	/*
	 * Taking actions before entering this feature -
	 * callee needs to issue func to get the range "times" times
	 */
	int (*config)(int times, get_range_t func);
	/* Entering this feature */
	int (*enable)(void);
	/* Leaving this feature */
	int (*disable)(void);
	/* Taking actions after leaving this feature */
	int (*restore)(void);
};

/*
 * Examples for feature specific operations,
 *
 * DCS:
 *  config - data collection, trigger LPDMA (4->2)
 *  enable - Notify PowerMCU to turn off high channels
 * disable - Notify PowerMCU to turn on high channels
 * restore - trigger LPDMA (2->4)
 *
 * PASR:
 *  config - Identify banks/ranks, trigger APMCU flow
 *  enable - No operations (in the enable flow of DCS in PowerMCU/SPM)
 * disable - No operations (in the disable flow of DCS in PowerMCU/SPM)
 * restore - Trigger APMCU flow for reset
 *
 * (Not absolutely)
 * Operations are called in low to high level order for configure/enable.
 * Operations are called in reverse order for disable/restore.
 */

extern void register_memory_lowpower_operation(struct memory_lowpower_operation *handler);
extern void unregister_memory_lowpower_operation(struct memory_lowpower_operation *handler);

/* memory-lowpower APIs */
extern int get_memory_lowpower_cma(void);
extern int put_memory_lowpower_cma(void);
extern int get_memory_lowpower_cma_aligned(int count, unsigned int align, struct page **pages);
extern int put_memory_lowpower_cma_aligned(int count, struct page *pages);
extern int memory_lowpower_task_init(void);
extern phys_addr_t memory_lowpower_cma_base(void);
extern unsigned long memory_lowpower_cma_size(void);
extern void set_memory_lowpower_aligned(int aligned);

#endif /* __MEMORY_LOWPOWER_INTERNAL_H */
