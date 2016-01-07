#include <linux/sched.h>

bool mtsched_is_enabled(void);
int mtproc_counts(void);

#ifdef CONFIG_MTPROF_CPUTIME
extern void save_mtproc_info(struct task_struct *p, unsigned long long ts);
extern void end_mtproc_info(struct task_struct *p);
extern void mt_cputime_switch(int on);
#else
static inline void
save_mtproc_info(struct task_struct *p, unsigned long long ts) {};
static inline void end_mtproc_info(struct task_struct *p) {};
static inline void mt_cputime_switch(int on) {};
#endif

