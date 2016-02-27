#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/uaccess.h>
#include <linux/tick.h>
#include <linux/seq_file.h>
#include <linux/list_sort.h>
#include "mt_sched_mon.h"

#define MAX_THROTTLE_COUNT 5

#ifdef CONFIG_MT_ENG_BUILD
#define MAX_THREAD_COUNT 50000
/* max debug thread count, if reach the level, stop store new thread informaiton*/
#define MAX_TIME (5*60*60)
/* max debug time, if reach the level, stop and clear the debug information*/
#else
#define MAX_THREAD_COUNT 10000
/* max debug thread count, if reach the level, stop store new thread informaiton*/
#define MAX_TIME (1*60*60)
/* max debug time, if reach the level, stop and clear the debug information*/
#endif

struct mt_rt_mon_struct {
	struct list_head list;

	pid_t pid;
	int prio;
	char comm[TASK_COMM_LEN];
	u64 cputime;
	u64 cputime_init;
	u64 cost_cputime;
	u32 cputime_percen_6;
	u64 isr_time;
	u64 cost_isrtime;
};

static struct mt_rt_mon_struct mt_rt_mon_head = {
	.list = LIST_HEAD_INIT(mt_rt_mon_head.list),
};

static int mt_rt_mon_enabled;
static int rt_mon_count;
static unsigned long long rt_start_ts, rt_end_ts, rt_dur_ts;
static DEFINE_SPINLOCK(mt_rt_mon_lock);
static struct mt_rt_mon_struct buffer[MAX_THROTTLE_COUNT];
static int rt_mon_count_buffer;
static unsigned long long rt_start_ts_buffer, rt_end_ts_buffer, rt_dur_ts_buffer;
/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}
#define SPLIT_NS_H(x) nsec_high(x)

static unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

#define SPLIT_NS_L(x) nsec_low(x)


static void store_rt_mon_info(struct task_struct *p)
{
	struct mt_rt_mon_struct *mtmon;
	unsigned long irq_flags;

	mtmon = kmalloc(sizeof(struct mt_rt_mon_struct), GFP_ATOMIC);
	if (!mtmon)
		return;
	memset(mtmon, 0, sizeof(struct mt_rt_mon_struct));
	INIT_LIST_HEAD(&(mtmon->list));

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	rt_mon_count++;
	mtmon->pid = p->pid;
	mtmon->prio = p->prio;
	strcpy(mtmon->comm, p->comm);
	mtmon->cputime = p->se.sum_exec_runtime;
	mtmon->cputime_init = p->se.sum_exec_runtime;
	mtmon->isr_time = p->se.mtk_isr_time;
	mtmon->cost_cputime = 0;
	mtmon->cost_isrtime = 0;
	list_add(&(mtmon->list), &(mt_rt_mon_head.list));

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
}

void setup_mt_rt_mon_info(struct task_struct *p)
{
	struct mt_rt_mon_struct *tmp;
	int find = 0;
	unsigned long irq_flags;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	if (0 == mt_rt_mon_enabled) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	if (rt_mon_count >= MAX_THREAD_COUNT) {
		pr_err("mtmon thread count larger the max level %d.\n",
			MAX_THREAD_COUNT);
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	list_for_each_entry(tmp, &mt_rt_mon_head.list, list) {
		if (!find && (tmp->pid == p->pid)) {
			tmp->prio = p->prio;
			strcpy(tmp->comm, p->comm);
			tmp->cputime = p->se.sum_exec_runtime;
			tmp->cputime_init = p->se.sum_exec_runtime;
			tmp->isr_time = p->se.mtk_isr_time;
			find = 1;
		}
	}

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);


	if (!find)
		store_rt_mon_info(p);

}
void start_rt_mon_task(void)
{
	struct task_struct *g, *p;
	unsigned long flags, irq_flags;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);
	mt_rt_mon_enabled = 1;
	rt_start_ts = sched_clock();
	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);

	read_lock_irqsave(&tasklist_lock, flags);

	do_each_thread(g, p) {
		if (!task_has_rt_policy(p))
			continue;
		setup_mt_rt_mon_info(p);
	} while_each_thread(g, p);

	read_unlock_irqrestore(&tasklist_lock, flags);
}

void stop_rt_mon_task(void)
{
	struct mt_rt_mon_struct *tmp;
	struct task_struct *tsk;
	unsigned long long cost_cputime = 0;
	unsigned long irq_flags;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	mt_rt_mon_enabled = 0;
	rt_end_ts = sched_clock();

	rt_dur_ts = rt_end_ts - rt_start_ts;
	do_div(rt_dur_ts, 1000000);	/* put prof_dur_ts to ms */

	list_for_each_entry(tmp, &mt_rt_mon_head.list, list) {
		tsk = find_task_by_vpid(tmp->pid);
		if (tsk && task_has_rt_policy(tsk)) {
			tmp->cputime = tsk->se.sum_exec_runtime;
			tmp->isr_time =
				tsk->se.mtk_isr_time - tmp->isr_time;
			tmp->cost_isrtime += tmp->isr_time;
			strcpy(tmp->comm, tsk->comm);
			tmp->prio = tsk->prio;
		}

		if (tmp->cputime >= (tmp->cputime_init + tmp->cost_isrtime)) {
			cost_cputime =
			   tmp->cputime - tmp->cost_isrtime - tmp->cputime_init;
			tmp->cost_cputime += cost_cputime;
			if (rt_dur_ts == 0)
				cost_cputime = 0;
			else
				do_div(cost_cputime, rt_dur_ts);
			tmp->cputime_percen_6 = cost_cputime;
		} else {
			tmp->cost_cputime = 0;
			tmp->cputime_percen_6 = 0;
			tmp->cost_isrtime = 0;
		}
	}
	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
}

void reset_rt_mon_list(void)
{
	struct mt_rt_mon_struct *tmp, *tmp2;
	struct task_struct *tsk;
	unsigned long irq_flags;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	mt_rt_mon_enabled = 0;

	list_for_each_entry_safe(tmp, tmp2, &mt_rt_mon_head.list, list) {
		tsk = find_task_by_vpid(tmp->pid);
		if (tsk && task_has_rt_policy(tsk)) {
			tmp->cost_cputime = 0;
			tmp->cputime_percen_6 = 0;
			tmp->cost_isrtime = 0;
			tmp->prio = tsk->prio;
			tmp->cputime_init = tsk->se.sum_exec_runtime;
			tmp->isr_time = tsk->se.mtk_isr_time;
		} else {
			rt_mon_count--;
			list_del(&(tmp->list));
			kfree(tmp);
		}
	}
	rt_end_ts = 0;

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);

}
static int mt_rt_mon_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct mt_rt_mon_struct *mon_a, *mon_b;

	mon_a = list_entry(a, struct mt_rt_mon_struct, list);
	mon_b = list_entry(b, struct mt_rt_mon_struct, list);

	if (mon_a->cost_cputime > mon_b->cost_cputime)
		return -1;
	if (mon_a->cost_cputime < mon_b->cost_cputime)
		return 1;

	return 0;
}

void mt_rt_mon_print_task(void)
{
	unsigned long irq_flags;
	int count = 0;
	struct mt_rt_mon_struct *tmp;

	rt_mon_count_buffer = rt_mon_count;
	rt_start_ts_buffer = rt_start_ts;
	rt_end_ts_buffer =  rt_end_ts;
	rt_dur_ts_buffer = rt_dur_ts;

	pr_err(
		"sched: mon_count = %d monitor start[%lld.%06lu] end[%lld.%06lu] dur[%lld.%06lu]\n",
		rt_mon_count, SPLIT_NS_H(rt_start_ts), SPLIT_NS_L(rt_start_ts),
		SPLIT_NS_H(rt_end_ts), SPLIT_NS_L(rt_end_ts),
		SPLIT_NS_H((rt_end_ts - rt_start_ts)),
		SPLIT_NS_L((rt_end_ts - rt_start_ts)));

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	list_sort(NULL, &mt_rt_mon_head.list, mt_rt_mon_cmp);

	list_for_each_entry(tmp, &mt_rt_mon_head.list, list) {
		memcpy(&buffer[count], tmp, sizeof(struct mt_rt_mon_struct));
		count++;
		pr_err("sched:[%s] pid:%d prio:%d cputime[%lld.%06lu] percen[%d.%04d%%] isr_time[%lld.%06lu]\n",
			tmp->comm, tmp->pid, tmp->prio,
			SPLIT_NS_H(tmp->cost_cputime), SPLIT_NS_L(tmp->cost_cputime),
			tmp->cputime_percen_6 / 10000, tmp->cputime_percen_6 % 10000,
			SPLIT_NS_H(tmp->isr_time), SPLIT_NS_L(tmp->isr_time));

		if (count == MAX_THROTTLE_COUNT)
			break;
	}

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
}

void mt_rt_mon_print_task_from_buffer(void)
{
	int i;

	pr_err("last throttle information start\n");
	pr_err("sched: mon_count = %d monitor start[%lld.%06lu] end[%lld.%06lu] dur[%lld.%06lu]\n",
		rt_mon_count_buffer, SPLIT_NS_H(rt_start_ts_buffer), SPLIT_NS_L(rt_start_ts_buffer),
		SPLIT_NS_H(rt_end_ts_buffer), SPLIT_NS_L(rt_end_ts_buffer),
		SPLIT_NS_H((rt_end_ts_buffer - rt_start_ts_buffer)),
		SPLIT_NS_L((rt_end_ts_buffer - rt_start_ts_buffer)));

	for (i = 0 ; i < MAX_THROTTLE_COUNT ; i++)  {
		pr_err("sched:[%s] pid:%d prio:%d cputime[%lld.%06lu] percen[%d.%04d%%] isr_time[%lld.%06lu]\n",
			buffer[i].comm, buffer[i].pid, buffer[i].prio,
			SPLIT_NS_H(buffer[i].cost_cputime), SPLIT_NS_L(buffer[i].cost_cputime),
			buffer[i].cputime_percen_6 / 10000, buffer[i].cputime_percen_6 % 10000,
			SPLIT_NS_H(buffer[i].isr_time), SPLIT_NS_L(buffer[i].isr_time));
	}

	pr_err("last throttle information end\n");
}

void mt_rt_mon_switch(int on)
{
	if (on == MON_RESET)
		reset_rt_mon_list();

	if (mt_rt_mon_enabled == 1) {
		if (on == MON_STOP)
			stop_rt_mon_task();
	} else {
		if (on == MON_START)
			start_rt_mon_task();
	}
}

void save_mt_rt_mon_info(struct task_struct *p, unsigned long long ts)
{
	unsigned long long mon_now_ts;
	unsigned long irq_flags;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	if (0 == mt_rt_mon_enabled) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	if (p->policy != SCHED_FIFO && p->policy != SCHED_RR) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	if (rt_mon_count >= MAX_THREAD_COUNT) {
		pr_err("mtmon thread count larger the max level %d.\n", MAX_THREAD_COUNT);
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);

	mon_now_ts = sched_clock();

	rt_dur_ts = mon_now_ts - rt_start_ts;
	do_div(rt_dur_ts, 1000000);	/* put prof_dur_ts to ms */
	if (rt_dur_ts >= MAX_TIME * 1000) {
		pr_err("mtmon debug time larger than the max time %d.\n", MAX_TIME);
		mt_rt_mon_switch(MON_RESET);
		return;
	}

	store_rt_mon_info(p);
}

void end_mt_rt_mon_info(struct task_struct *p)
{
	struct mt_rt_mon_struct *tmp;
	unsigned long irq_flags;
	int find = 0;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	/* check profiling enable flag */
	if (0 == mt_rt_mon_enabled) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	if (!task_has_rt_policy(p)) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	list_for_each_entry(tmp, &mt_rt_mon_head.list, list) {
		if (p->pid == tmp->pid) {
			tmp->prio = p->prio;
			strcpy(tmp->comm, p->comm);
			/* update cputime */
			tmp->cputime = p->se.sum_exec_runtime;
			tmp->isr_time = p->se.mtk_isr_time - tmp->isr_time;
			tmp->cost_isrtime += tmp->isr_time;
			find = 1;
			break;
		}
	}
	if (!find)
		pr_err("pid:%d can't be found in mtsched proc_info.\n", p->pid);

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);

}
void check_mt_rt_mon_info(struct task_struct *p)
{
	struct mt_rt_mon_struct *tmp;
	unsigned long irq_flags;
	unsigned long long cost_cputime = 0;
	int find = 0;

	spin_lock_irqsave(&mt_rt_mon_lock, irq_flags);

	/* check profiling enable flag */
	if (0 == mt_rt_mon_enabled) {
		spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);
		return;
	}

	list_for_each_entry(tmp, &mt_rt_mon_head.list, list) {
		if (!find && p->pid == tmp->pid) {
			tmp->prio = p->prio;
			strcpy(tmp->comm, p->comm);
			tmp->cputime = p->se.sum_exec_runtime;
			if (!task_has_rt_policy(p)) {
				tmp->isr_time =
					p->se.mtk_isr_time - tmp->isr_time;
				tmp->cost_isrtime += tmp->isr_time;
				if (tmp->cputime >= (tmp->cputime_init + tmp->cost_isrtime)) {
					cost_cputime =
						tmp->cputime - tmp->cost_isrtime - tmp->cputime_init;
					tmp->cost_cputime += cost_cputime;
				} else {
					tmp->cost_cputime = 0;
				}
			}
			tmp->cputime_init = p->se.sum_exec_runtime;
			tmp->isr_time = p->se.mtk_isr_time;
			find = 1;
			break;
		}
	}

	spin_unlock_irqrestore(&mt_rt_mon_lock, irq_flags);

	if (!find && task_has_rt_policy(p))
		store_rt_mon_info(p);

}

int mt_rt_mon_enable(void)
{
	return mt_rt_mon_enabled;
}
