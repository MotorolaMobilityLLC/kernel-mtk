#ifndef _MT_SCHED_MON_H
#define _MT_SCHED_MON_H
/*CPU holding event: ISR/SoftIRQ/Tasklet/Timer*/
struct sched_block_event {
	int type;
	unsigned long cur_event;
	unsigned long last_event;
	unsigned long long cur_ts;
	unsigned long long last_ts;
	unsigned long long last_te;

	unsigned long cur_count;
	unsigned long last_count;
	int preempt_count;
};
DECLARE_PER_CPU(struct sched_block_event, ISR_mon);
DECLARE_PER_CPU(struct sched_block_event, SoftIRQ_mon);
DECLARE_PER_CPU(struct sched_block_event, tasklet_mon);
DECLARE_PER_CPU(struct sched_block_event, hrt_mon);
DECLARE_PER_CPU(struct sched_block_event, sft_mon);

DECLARE_PER_CPU(int, mt_timer_irq);
extern void mt_trace_ISR_start(int id);
extern void mt_trace_ISR_end(int id);
extern void mt_trace_SoftIRQ_start(int id);
extern void mt_trace_SoftIRQ_end(int id);
extern void mt_trace_tasklet_start(void *func);
extern void mt_trace_tasklet_end(void *func);
extern void mt_trace_hrt_start(void *func);
extern void mt_trace_hrt_end(void *func);
extern void mt_trace_sft_start(void *func);
extern void mt_trace_sft_end(void *func);

extern void mt_save_irq_counts(void);
extern void mt_show_last_irq_counts(void);
extern void mt_show_current_irq_counts(void);

/*Schedule disable event: IRQ/Preempt disable monitor*/
struct sched_stop_event {
	unsigned long long cur_ts;
	unsigned long long last_ts;
	unsigned long long last_te;
};
DECLARE_PER_CPU(struct sched_stop_event, IRQ_disable_mon);
DECLARE_PER_CPU(struct sched_stop_event, Preempt_disable_mon);
extern void MT_trace_irq_on(void);
extern void MT_trace_irq_off(void);
extern void MT_trace_preempt_on(void);
extern void MT_trace_preempt_off(void);
/* [IRQ-disable] White List
 * Flags for special scenario*/
DECLARE_PER_CPU(int, MT_trace_in_sched);
DECLARE_PER_CPU(int, MT_trace_in_resume_console);

extern void mt_aee_dump_sched_traces(void);
extern void mt_dump_sched_traces(void);

DECLARE_PER_CPU(int, mtsched_mon_enabled);
DECLARE_PER_CPU(unsigned long long, local_timer_ts);
DECLARE_PER_CPU(unsigned long long, local_timer_te);

#include <linux/sched.h>

#define MON_STOP 0
#define MON_START 1
#define MON_RESET 2

#ifdef CONFIG_MT_RT_THROTTLE_MON
extern void save_mt_rt_mon_info(struct task_struct *p, unsigned long long ts);
extern void end_mt_rt_mon_info(struct task_struct *p);
extern void mt_rt_mon_switch(int on);
extern void mt_rt_mon_print_task(void);
extern int mt_rt_mon_enable(void);
#else
static inline void
save_mt_rt_mon_info(struct task_struct *p, unsigned long long ts) {};
static inline void end_mt_rt_mon_info(struct task_struct *p) {};
static inline void mt_rt_mon_switch(int on) {};
static inline void mt_rt_mon_print_task(void) {};
static inline int mt_rt_mon_enable(void)
{
	return 0;
}
#endif
#endif /* _MT_SCHED_MON_H */

