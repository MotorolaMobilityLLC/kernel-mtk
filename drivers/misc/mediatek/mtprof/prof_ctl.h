#include <linux/sched.h>
#include "prof_main.h"
extern struct mt_proc_struct *mt_proc_head;
extern unsigned long long prof_start_ts, prof_end_ts, prof_dur_ts;

extern struct mt_cpu_info *mt_cpu_info_head;
extern int mt_cpu_num;

void mt_task_times(struct task_struct *p, cputime_t *ut, cputime_t *st);

unsigned long long mtprof_get_cpu_idle(int cpu);
unsigned long long mtprof_get_cpu_iowait(int cpu);

void save_mtproc_info(struct task_struct *p, unsigned long long ts);
void end_mtproc_info(struct task_struct *p);
void start_record_task(void);
void stop_record_task(void);
void reset_record_task(void);
void mt_cputime_switch(int on);
void mt_memprof_switch(int on);

/*
 * Ease the printing of nsec fields:
 */
long long nsec_high(unsigned long long nsec);
unsigned long nsec_low(unsigned long long nsec);

long long usec_high(unsigned long long usec);
unsigned long usec_low(unsigned long long usec);
