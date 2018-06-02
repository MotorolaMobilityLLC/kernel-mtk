#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched

#if !defined(_TRACE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHED_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>

#ifdef CONFIG_MTK_SCHED_TRACERS
/* M: states for tracking I/O & mutex events
 * notice avoid to conflict with linux/sched.h
 *
 * A bug linux not fixed:
 * 'K' for TASK_WAKEKILL specified in linux/sched.h
 * but marked 'K' in sched_switch will cause Android systrace parser confused
 * therefore for sched_switch events, these extra states will be printed
 * in the end of each line
 */
#define _MT_TASK_BLOCKED_RTMUX		(TASK_STATE_MAX << 1)
#define _MT_TASK_BLOCKED_MUTEX		(TASK_STATE_MAX << 2)
#define _MT_TASK_BLOCKED_IO		(TASK_STATE_MAX << 3)
#define _MT_EXTRA_STATE_MASK (_MT_TASK_BLOCKED_RTMUX | \
			      _MT_TASK_BLOCKED_MUTEX | \
			      _MT_TASK_BLOCKED_IO | \
			      TASK_WAKEKILL | \
			      TASK_PARKED | \
			      TASK_NOLOAD)
#endif
#define _MT_TASK_STATE_MASK  ((TASK_STATE_MAX - 1) & \
			      ~(TASK_WAKEKILL | TASK_PARKED | TASK_NOLOAD))

/*
 * Tracepoint for calling kthread_stop, performed to end a kthread:
 */
TRACE_EVENT(sched_kthread_stop,

	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid	= t->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);

/*
 * Tracepoint for the return value of the kthread stopping:
 */
TRACE_EVENT(sched_kthread_stop_ret,

	TP_PROTO(int ret),

	TP_ARGS(ret),

	TP_STRUCT__entry(
		__field(	int,	ret	)
	),

	TP_fast_assign(
		__entry->ret	= ret;
	),

	TP_printk("ret=%d", __entry->ret)
);

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(bool preempt, struct task_struct *p);
#endif

/*
 * Tracepoint for waking up a task:
 */
DECLARE_EVENT_CLASS(sched_wakeup_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(__perf_task(p)),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	success			)
		__field(	int,	target_cpu		)
#ifdef CONFIG_MTK_SCHED_TRACERS
		__field(long, state)
#endif
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->success	= 1; /* rudiment, kill when possible */
		__entry->target_cpu	= task_cpu(p);
#ifdef CONFIG_MTK_SCHED_TRACERS
		__entry->state	= __trace_sched_switch_state(false, p);
#endif
	),

	TP_printk(
#ifdef CONFIG_MTK_SCHED_TRACERS
		"comm=%s pid=%d prio=%d success=%d target_cpu=%03d state=%s",
#else
		"comm=%s pid=%d prio=%d success=%d target_cpu=%03d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->success, __entry->target_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
		,
		__entry->state & (~TASK_STATE_MAX) ?
		  __print_flags(__entry->state & (~TASK_STATE_MAX), "|",
				{ TASK_INTERRUPTIBLE, "S"},
				{ TASK_UNINTERRUPTIBLE, "D"},
				{ __TASK_STOPPED, "T"},
				{ __TASK_TRACED, "t"},
				{ EXIT_ZOMBIE, "Z"},
				{ EXIT_DEAD, "X"},
				{ TASK_DEAD, "x"},
				{ TASK_WAKEKILL, "K"},
				{ TASK_WAKING, "W"},
				{ TASK_PARKED, "P"},
				{ TASK_NOLOAD, "N"},
				{ _MT_TASK_BLOCKED_RTMUX, "r"},
				{ _MT_TASK_BLOCKED_MUTEX, "m"},
				{ _MT_TASK_BLOCKED_IO, "d"}) : "R"
#endif
			)
);

/*
 * Tracepoint called when waking a task; this tracepoint is guaranteed to be
 * called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_waking,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint called when the task is actually woken; p->state == TASK_RUNNNG.
 * It it not always called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(bool preempt, struct task_struct *p)
{
	long state = p->state;

	/*
	 * M:mark as comment to export more task state for
	 * migration & wakeup
	 */
#ifdef CONFIG_SCHED_DEBUG
	/* BUG_ON(p != current); */
#endif /* CONFIG_SCHED_DEBUG */

	/*
	 * Preemption ignores task state, therefore preempted tasks are always
	 * RUNNING (we will not have dequeued if state != RUNNING).
	 */
	if (preempt)
		state = TASK_RUNNING | TASK_STATE_MAX;
#ifdef CONFIG_MTK_SCHED_TRACERS
#ifdef CONFIG_RT_MUTEXES
	if (p->pi_blocked_on)
		state |= _MT_TASK_BLOCKED_RTMUX;
#endif
#ifdef CONFIG_DEBUG_MUTEXES
	if (p->blocked_on)
		state |= _MT_TASK_BLOCKED_MUTEX;
#endif
	if ((p->state & TASK_UNINTERRUPTIBLE) && p->in_iowait)
		state |= _MT_TASK_BLOCKED_IO;
#endif

	return state;
}
# if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
/*
 * legacy cgroup hierarchy depth is no more than 3, and here we limit the
 * size of each load printing no more than 10, 9 chars with a slash '/'.
 * thus, making MTK_FAIR_DBG_SZ = 100 is pretty safe from array overflow,
 * because 100 is much larger than 60, ((3 * 10) * 2), 2 for @prev and @next
 * tasks.
 */
#  define MTK_FAIR_DBG_SZ	100
/*
 * snprintf writes at most @size bytes (including the trailing null bytes
 * ('\0'), so increment 10 to 11
 */
#  define MTK_FAIR_DBG_LEN	(10 + 1)
#  define MTK_FAIR_DBG_DEP	3

static int fair_cgroup_load(char *buf, int cnt, struct task_struct *p)
{
	int loc = cnt;
	int t, depth = 0;
	unsigned long w[MTK_FAIR_DBG_DEP];
	struct sched_entity *se = p->se.parent;

	for (; se && (depth < MTK_FAIR_DBG_DEP); se = se->parent)
		w[depth++] = se->load.weight;

	switch (p->policy) {
	case SCHED_NORMAL:
		loc += snprintf(&buf[loc], 7, "NORMAL"); break;
	case SCHED_IDLE:
		loc += snprintf(&buf[loc], 5, "IDLE"); break;
	case SCHED_BATCH:
		loc += snprintf(&buf[loc], 6, "BATCH"); break;
	}

	for (depth--; depth >= 0; depth--) {
		t = snprintf(&buf[loc], MTK_FAIR_DBG_LEN, "/%lu", w[depth]);
		if ((t < MTK_FAIR_DBG_LEN) && (t > 0))
			loc += t;
		else
			loc += snprintf(&buf[loc], 7, "/ERROR");
	}

	t = snprintf(&buf[loc], MTK_FAIR_DBG_LEN, "/%lu", p->se.load.weight);
	if ((t < MTK_FAIR_DBG_LEN) && (t > 0))
		loc += t;
	else
		loc += snprintf(&buf[loc], 7, "/ERROR");

	return loc;
}

static int is_fair_preempt(bool preempt, char *buf, struct task_struct *prev,
			   struct task_struct *next)
{
	int cnt;
	/* nothing needs to be clarified for RT class or yielding from IDLE */
	if ((task_pid_nr(prev) == 0) || (rt_task(next) || rt_task(prev)))
		return 0;

	/* take care about preemption only */
	if (prev->state && !(preempt)) {
		return 0;
	}

	memset(buf, 0, MTK_FAIR_DBG_SZ);
	cnt = fair_cgroup_load(buf, 0, prev);
	cnt += snprintf(&buf[cnt], 6, " ==> ");
	fair_cgroup_load(buf, cnt, next);
	return 1;
}
# endif
#endif /* CREATE_TRACE_POINTS */

/*
 * Tracepoint for task switches, performed by the scheduler:
 */
TRACE_EVENT(sched_switch,

	TP_PROTO(bool preempt,
		 struct task_struct *prev,
		 struct task_struct *next),

	TP_ARGS(preempt, prev, next),

	TP_STRUCT__entry(
		__array(	char,	prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	prev_pid			)
		__field(	int,	prev_prio			)
		__field(	long,	prev_state			)
		__array(	char,	next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	next_pid			)
		__field(	int,	next_prio			)
#if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
		__field(	int,	fair_preempt			)
		__array(	char,	fair_dbg_buf,	MTK_FAIR_DBG_SZ )
#endif
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(preempt, prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
#if defined(CONFIG_FAIR_GROUP_SCHED) && defined(CONFIG_MTK_SCHED_TRACERS)
		__entry->fair_preempt   = is_fair_preempt(preempt, __entry->fair_dbg_buf,
							  prev, next);
#endif
	),

#ifdef CONFIG_MTK_SCHED_TRACERS
	TP_printk(
	"prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d%s%s %s",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (_MT_TASK_STATE_MASK) ?
		__print_flags(__entry->prev_state & (_MT_TASK_STATE_MASK), "|",
				{ TASK_INTERRUPTIBLE, "S"} ,
				{ TASK_UNINTERRUPTIBLE, "D" },
				{ __TASK_STOPPED, "T" },
				{ __TASK_TRACED, "t" },
				{ EXIT_DEAD, "X" },
				{ EXIT_ZOMBIE, "Z" },
				{ TASK_DEAD, "x" },
				{ TASK_WAKEKILL, "K"},
				{ TASK_WAKING, "W"}) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio ,
		(__entry->prev_state & _MT_EXTRA_STATE_MASK) ?
			" extra_prev_state=" : "",
		__print_flags(__entry->prev_state & _MT_EXTRA_STATE_MASK, "|",
			      { TASK_WAKEKILL, "K" },
			      { TASK_PARKED, "P" },
			      { TASK_NOLOAD, "N" },
			      { _MT_TASK_BLOCKED_RTMUX, "r" },
			      { _MT_TASK_BLOCKED_MUTEX, "m" },
			      { _MT_TASK_BLOCKED_IO, "d" })
# ifdef CONFIG_FAIR_GROUP_SCHED
				, (__entry->fair_preempt ? __entry->fair_dbg_buf : "")
# else
				, ""
# endif
	)
#else
	TP_printk("prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (TASK_STATE_MAX-1) ?
		  __print_flags(__entry->prev_state & (TASK_STATE_MAX-1), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
				{ 128, "K" }, { 256, "W" }, { 512, "P" },
				{ 1024, "N" }) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio)
#endif
);

/*
 * Tracepoint for a task being migrated:
 */
TRACE_EVENT(sched_migrate_task,

	TP_PROTO(struct task_struct *p, int dest_cpu),

	TP_ARGS(p, dest_cpu),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	orig_cpu		)
		__field(	int,	dest_cpu		)
#ifdef CONFIG_MTK_SCHED_TRACERS
		__field(long, state)
#endif
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->orig_cpu	= task_cpu(p);
		__entry->dest_cpu	= dest_cpu;
#ifdef CONFIG_MTK_SCHED_TRACERS
		__entry->state      =	__trace_sched_switch_state(false, p);
#endif
	),

#ifdef CONFIG_MTK_SCHED_TRACERS
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d state=%s",
#else
	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d",
#endif
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu
#ifdef CONFIG_MTK_SCHED_TRACERS
		,
		__entry->state & (~TASK_STATE_MAX) ?
		  __print_flags(__entry->state & (~TASK_STATE_MAX), "|",
				{ TASK_INTERRUPTIBLE, "S"},
				{ TASK_UNINTERRUPTIBLE, "D" },
				{ __TASK_STOPPED, "T" },
				{ __TASK_TRACED, "t" },
				{ EXIT_ZOMBIE, "Z" },
				{ EXIT_DEAD, "X" },
				{ TASK_DEAD, "x" },
				{ TASK_WAKEKILL, "K" },
				{ TASK_WAKING, "W" },
				{ TASK_PARKED, "P" },
				{ TASK_NOLOAD, "N" },
				{ _MT_TASK_BLOCKED_RTMUX, "r" },
				{ _MT_TASK_BLOCKED_MUTEX, "m"},
				{ _MT_TASK_BLOCKED_IO, "d"}) : "R"
#endif
			)
);

/*
 * Tracepoint for a CPU going offline/online:
 */
TRACE_EVENT(sched_cpu_hotplug,

	TP_PROTO(int affected_cpu, int error, int status),

	TP_ARGS(affected_cpu, error, status),

	TP_STRUCT__entry(
		__field(	int,	affected_cpu		)
		__field(	int,	error			)
		__field(	int,	status			)
	),

	TP_fast_assign(
		__entry->affected_cpu	= affected_cpu;
		__entry->error		= error;
		__entry->status		= status;
	),

	TP_printk("cpu %d %s error=%d", __entry->affected_cpu,
		__entry->status ? "online" : "offline", __entry->error)
);

DECLARE_EVENT_CLASS(sched_process_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(p),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for freeing a task:
 */
DEFINE_EVENT(sched_process_template, sched_process_free,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));


/*
 * Tracepoint for a task exiting:
 */
DEFINE_EVENT(sched_process_template, sched_process_exit,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waiting on task to unschedule:
 */
DEFINE_EVENT(sched_process_template, sched_wait_task,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * Tracepoint for a waiting task:
 */
TRACE_EVENT(sched_process_wait,

	TP_PROTO(struct pid *pid),

	TP_ARGS(pid),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, current->comm, TASK_COMM_LEN);
		__entry->pid		= pid_nr(pid);
		__entry->prio		= current->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for do_fork:
 */
TRACE_EVENT(sched_process_fork,

	TP_PROTO(struct task_struct *parent, struct task_struct *child),

	TP_ARGS(parent, child),

	TP_STRUCT__entry(
		__array(	char,	parent_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	parent_pid			)
		__array(	char,	child_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	child_pid			)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid)
);

/*
 * Tracepoint for fork time:
 */
TRACE_EVENT(sched_fork_time,

	TP_PROTO(struct task_struct *parent, struct task_struct *child, unsigned long long dur),

	TP_ARGS(parent, child, dur),

	TP_STRUCT__entry(
		__array(char,	parent_comm,	TASK_COMM_LEN)
		__field(pid_t,	parent_pid)
		__array(char,	child_comm,	TASK_COMM_LEN)
		__field(pid_t,	child_pid)
		__field(unsigned long long,	dur)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
		__entry->dur = dur;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d fork_time=%llu us",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid, __entry->dur)
);


/*
 * Tracepoint for exec:
 */
TRACE_EVENT(sched_process_exec,

	TP_PROTO(struct task_struct *p, pid_t old_pid,
		 struct linux_binprm *bprm),

	TP_ARGS(p, old_pid, bprm),

	TP_STRUCT__entry(
		__string(	filename,	bprm->filename	)
		__field(	pid_t,		pid		)
		__field(	pid_t,		old_pid		)
	),

	TP_fast_assign(
		__assign_str(filename, bprm->filename);
		__entry->pid		= p->pid;
		__entry->old_pid	= old_pid;
	),

	TP_printk("filename=%s pid=%d old_pid=%d", __get_str(filename),
		  __entry->pid, __entry->old_pid)
);

/*
 * XXX the below sched_stat tracepoints only apply to SCHED_OTHER/BATCH/IDLE
 *     adding sched_stat support to SCHED_FIFO/RR would be welcome.
 */
DECLARE_EVENT_CLASS(sched_stat_template,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(__perf_task(tsk), __perf_count(delay)),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	delay			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->delay	= delay;
	),

	TP_printk("comm=%s pid=%d delay=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->delay)
);


/*
 * Tracepoint for accounting wait time (time the task is runnable
 * but not actually running due to scheduler contention).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_wait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting sleep time (time the task is not runnable,
 * including iowait, see below).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_sleep,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting iowait time (time the task is not runnable
 * due to waiting on IO to complete).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_iowait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting blocked time (time the task is in uninterruptible).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_blocked,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for recording the cause of uninterruptible sleep.
 */
TRACE_EVENT(sched_blocked_reason,

	TP_PROTO(struct task_struct *tsk),

	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__field( pid_t,	pid	)
		__field( void*, caller	)
		__field( bool, io_wait	)
	),

	TP_fast_assign(
		__entry->pid	= tsk->pid;
		__entry->caller = (void*)get_wchan(tsk);
		__entry->io_wait = tsk->in_iowait;
	),

	TP_printk("pid=%d iowait=%d caller=%pS", __entry->pid, __entry->io_wait, __entry->caller)
);

/*
 * Tracepoint for accounting runtime (time the task is executing
 * on a CPU).
 */
DECLARE_EVENT_CLASS(sched_stat_runtime,

	TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),

	TP_ARGS(tsk, __perf_count(runtime), vruntime),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	runtime			)
		__field( u64,	vruntime			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->runtime	= runtime;
		__entry->vruntime	= vruntime;
	),

	TP_printk("comm=%s pid=%d runtime=%Lu [ns] vruntime=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->runtime,
			(unsigned long long)__entry->vruntime)
);

DEFINE_EVENT(sched_stat_runtime, sched_stat_runtime,
	     TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),
	     TP_ARGS(tsk, runtime, vruntime));

/*
 * Tracepoint for showing priority inheritance modifying a tasks
 * priority.
 */
TRACE_EVENT(sched_pi_setprio,

	TP_PROTO(struct task_struct *tsk, int newprio),

	TP_ARGS(tsk, newprio),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldprio			)
		__field( int,	newprio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldprio	= tsk->prio;
		__entry->newprio	= newprio;
	),

	TP_printk("comm=%s pid=%d oldprio=%d newprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldprio, __entry->newprio)
);

#ifdef CONFIG_DETECT_HUNG_TASK
TRACE_EVENT(sched_process_hang,
	TP_PROTO(struct task_struct *tsk),
	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);
#endif /* CONFIG_DETECT_HUNG_TASK */

DECLARE_EVENT_CLASS(sched_move_task_template,

	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	pid			)
		__field( pid_t,	tgid			)
		__field( pid_t,	ngid			)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->pid		= task_pid_nr(tsk);
		__entry->tgid		= task_tgid_nr(tsk);
		__entry->ngid		= task_numa_group_id(tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("pid=%d tgid=%d ngid=%d src_cpu=%d src_nid=%d dst_cpu=%d dst_nid=%d",
			__entry->pid, __entry->tgid, __entry->ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracks migration of tasks from one runqueue to another. Can be used to
 * detect if automatic NUMA balancing is bouncing between nodes
 */
DEFINE_EVENT(sched_move_task_template, sched_move_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

DEFINE_EVENT(sched_move_task_template, sched_stick_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

TRACE_EVENT(sched_swap_numa,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	src_pid			)
		__field( pid_t,	src_tgid		)
		__field( pid_t,	src_ngid		)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( pid_t,	dst_pid			)
		__field( pid_t,	dst_tgid		)
		__field( pid_t,	dst_ngid		)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->src_pid	= task_pid_nr(src_tsk);
		__entry->src_tgid	= task_tgid_nr(src_tsk);
		__entry->src_ngid	= task_numa_group_id(src_tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_pid	= task_pid_nr(dst_tsk);
		__entry->dst_tgid	= task_tgid_nr(dst_tsk);
		__entry->dst_ngid	= task_numa_group_id(dst_tsk);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("src_pid=%d src_tgid=%d src_ngid=%d src_cpu=%d src_nid=%d dst_pid=%d dst_tgid=%d dst_ngid=%d dst_cpu=%d dst_nid=%d",
			__entry->src_pid, __entry->src_tgid, __entry->src_ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_pid, __entry->dst_tgid, __entry->dst_ngid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracepoint for waking a polling cpu without an IPI.
 */
TRACE_EVENT(sched_wake_idle_without_ipi,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(	int,	cpu	)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
	),

	TP_printk("cpu=%d", __entry->cpu)
);

#ifdef CONFIG_MTK_SCHED_TRACERS
/*
 * Tracepoint for showing the result of task runqueue selection
 */
TRACE_EVENT(sched_select_task_rq,

	TP_PROTO(struct task_struct *tsk, int policy, int prev_cpu, int target_cpu),

	TP_ARGS(tsk, policy, prev_cpu, target_cpu),

	TP_STRUCT__entry(
		__field(pid_t, pid)
		__field(int, policy)
		__field(int, prev_cpu)
		__field(int, target_cpu)
	),

	TP_fast_assign(
		__entry->pid              = tsk->pid;
		__entry->policy           = policy;
		__entry->prev_cpu         = prev_cpu;
		__entry->target_cpu       = target_cpu;
	),

	TP_printk("pid=%4d policy=0x%08x pre-cpu=%d target=%d",
		__entry->pid,
		__entry->policy,
		__entry->prev_cpu,
		__entry->target_cpu)
);
#endif

TRACE_EVENT(sched_heavy_task,
		TP_PROTO(const char *s),
		TP_ARGS(s),
		TP_STRUCT__entry(
			__string(s, s)
			),
		TP_fast_assign(
			__assign_str(s, s);
			),
		TP_printk("%s", __get_str(s))
);

#ifdef CONFIG_MT_SCHED_TRACE
#define sched_trace(event) \
TRACE_EVENT(event,                      \
    TP_PROTO(char *strings),                    \
    TP_ARGS(strings),                           \
    TP_STRUCT__entry(                           \
        __array(    char,  strings, 128)        \
    ),                                          \
    TP_fast_assign(                             \
        memcpy(__entry->strings, strings, 128); \
    ),                                          \
    TP_printk("%s",__entry->strings))

sched_trace(sched_log);
// mtk rt enhancement
sched_trace(sched_rt);
sched_trace(sched_rt_info);
sched_trace(sched_lb);
sched_trace(sched_lb_info);

 #ifdef CONFIG_MT_DEBUG_PREEMPT
sched_trace(sched_preempt);
 #endif

// mtk scheduling interopertion enhancement
 #ifdef CONFIG_MT_SCHED_INTEROP
sched_trace(sched_interop);
 #endif
#endif /* CONFIG_MT_SCHED_TRACE */

/*sched: add trace_sched*/
TRACE_EVENT(sched_task_entity_avg,

	TP_PROTO(unsigned int tag, struct task_struct *tsk, struct sched_avg *avg),

	TP_ARGS(tag, tsk, avg),

	TP_STRUCT__entry(
		__field(u32, tag)
		__array(char, comm,     TASK_COMM_LEN)
		__field(pid_t, tgid)
		__field(pid_t, pid)
		__field(u64, load_sum)
		__field(u32, util_sum)
		__field(u32, period_contrib)
		__field(unsigned long, ratio)
		__field(u32, usage_sum)
		__field(unsigned long, load_avg)
		__field(unsigned long, util_avg)
	),

	TP_fast_assign(
		__entry->tag		= tag;
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->tgid		= task_pid_nr(tsk->group_leader);
		__entry->pid		= task_pid_nr(tsk);
		__entry->load_sum	= avg->load_sum;
		__entry->util_sum	= avg->util_sum;
		__entry->period_contrib	= avg->period_contrib;
		__entry->ratio		= 0;
		__entry->usage_sum	= -1;
		__entry->load_avg	= avg->load_avg;
		__entry->util_avg	= avg->util_avg;
	),

	TP_printk("[%d]comm=%s tgid=%d pid=%d load_sum=%lld util_sum=%d period_contrib=%d ratio=%lu exe_time=%d load_avg=%lu util_avg=%lu",
		__entry->tag, __entry->comm, __entry->tgid, __entry->pid,
		__entry->load_sum, __entry->util_sum, __entry->period_contrib,
		__entry->ratio, __entry->usage_sum, __entry->load_avg, __entry->util_avg)
);

/*
 * Tracepoint for HMP (CONFIG_SCHED_HMP) task migrations.
 */
TRACE_EVENT(sched_hmp_migrate,

		TP_PROTO(struct task_struct *tsk, int dest, int force),

		TP_ARGS(tsk, dest, force),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, pid)
			__field(int,  dest)
			__field(int,  force)
			),

		TP_fast_assign(
			memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
			__entry->pid   = tsk->pid;
			__entry->dest  = dest;
			__entry->force = force;
			),

		TP_printk("comm=%s pid=%d dest=%d force=%d",
			__entry->comm, __entry->pid,
			__entry->dest, __entry->force)
		);
/*
 * Tracepoint for average heavy task calculation.
 */
TRACE_EVENT(sched_avg_heavy_task,

	TP_PROTO(int last_poll1, int last_poll2, int avg, int cluster_id, int max),

	TP_ARGS(last_poll1, last_poll2, avg, cluster_id, max),

	TP_STRUCT__entry(
		__field(int, last_poll1)
		__field(int, last_poll2)
		__field(int, avg)
		__field(int, cid)
		__field(int, max)
	),

	TP_fast_assign(
		__entry->last_poll1 = last_poll1;
		__entry->last_poll2 = last_poll2;
		__entry->avg = avg;
		__entry->cid = cluster_id;
		__entry->max = max;
	),

	TP_printk("last_poll1=%d last_poll2=%d, avg=%d, max:%d, cid:%d",
		__entry->last_poll1,
		__entry->last_poll2,
		__entry->avg,
		__entry->max,
		__entry->cid)
);

TRACE_EVENT(sched_avg_heavy_nr,
	TP_PROTO(const char *func_name, int nr_heavy, long long int diff, int ack_cap, int cpu),

	TP_ARGS(func_name, nr_heavy, diff, ack_cap, cpu),

	TP_STRUCT__entry(
		__array(char, func_name, 32)
		__field(int, nr_heavy)
		__field(long long int, diff)
		__field(int, ack_cap)
		__field(int, cpu)
	),

	TP_fast_assign(
		memcpy(__entry->func_name, func_name, 32);
		__entry->nr_heavy = nr_heavy;
		__entry->diff = diff;
		__entry->ack_cap = ack_cap;
		__entry->cpu = cpu;
	),

	TP_printk("%s nr_heavy=%d time diff:%lld ack_cap:%d cpu:%d",
		__entry->func_name,
		__entry->nr_heavy, __entry->diff, __entry->ack_cap, __entry->cpu
	)
);

TRACE_EVENT(sched_avg_heavy_time,
	TP_PROTO(long long int time_period, long long int last_get_heavy_time, int cid),

	TP_ARGS(time_period, last_get_heavy_time, cid),

	TP_STRUCT__entry(
		__field(long long int, time_period)
		__field(long long int, last_get_heavy_time)
		__field(int, cid)
	),

	TP_fast_assign(
		__entry->time_period = time_period;
		__entry->last_get_heavy_time = last_get_heavy_time;
		__entry->cid = cid;
	),

	TP_printk("time_period:%lld last_get_heavy_time:%lld cid:%d",
		__entry->time_period, __entry->last_get_heavy_time, __entry->cid
	)
)

TRACE_EVENT(sched_avg_heavy_task_load,
	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(        char,   comm,   TASK_COMM_LEN   )
		__field(        pid_t,  pid)
		__field(        long long int,  load)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid = t->pid;
		__entry->load = t->se.avg.load_avg;
	),

	TP_printk("heavy_task_detect comm:%s pid:%d load:%lld",
		__entry->comm, __entry->pid, __entry->load
	)
)

/*
 * Tracepoint for accounting sched averages for tasks.
 */
TRACE_EVENT(sched_load_avg_task,

		TP_PROTO(struct task_struct *tsk, struct sched_avg *avg),

		TP_ARGS(tsk, avg),

		TP_STRUCT__entry(
			__array( char,  comm,   TASK_COMM_LEN   )
			__field( pid_t, pid                     )
			__field( int,   cpu                     )
			__field( unsigned long, load_avg        )
			__field( unsigned long, util_avg        )
			__field( u64,  load_sum                 )
			__field( u32,  util_sum                 )
			__field( u32,  period_contrib           )
			),

		TP_fast_assign(
			memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
			__entry->pid                    = tsk->pid;
			__entry->cpu                    = task_cpu(tsk);
			__entry->load_avg               = avg->load_avg;
			__entry->util_avg               = avg->util_avg;
			__entry->load_sum               = avg->load_sum;
			__entry->util_sum               = avg->util_sum;
			__entry->period_contrib             = avg->period_contrib;
			),

		TP_printk("comm=%s pid=%d cpu=%d load_avg=%lu util_avg=%lu load_sum=%llu"
				" util_sum=%u period_contrib=%u",
				__entry->comm,
				__entry->pid,
				__entry->cpu,
				__entry->load_avg,
				__entry->util_avg,
				(u64)__entry->load_sum,
				(u32)__entry->util_sum,
				(u32)__entry->period_contrib)
);

/*
 * Tracepoint for accounting sched averages for cpus.
 */
TRACE_EVENT(sched_load_avg_cpu,

		TP_PROTO(int cpu, struct cfs_rq *cfs_rq),

		TP_ARGS(cpu, cfs_rq),

		TP_STRUCT__entry(
			__field( int,   cpu                          )
			__field( unsigned long, load_avg             )
			__field( unsigned long, util_avg             )
			),

		TP_fast_assign(
			__entry->cpu                    = cpu;
			__entry->load_avg                   = cfs_rq->avg.load_avg;
			__entry->util_avg                   = cfs_rq->avg.util_avg;
			),

		TP_printk("cpu=%d load_avg=%lu util_avg=%lu",
			__entry->cpu, __entry->load_avg, __entry->util_avg)
);


#ifdef CONFIG_HMP_TRACER
/*
 * Tracepoint for showing tracked migration information
 */
TRACE_EVENT(sched_dynamic_threshold,

		TP_PROTO(struct task_struct *tsk, unsigned int threshold,
			unsigned int status, int curr_cpu, int target_cpu, int task_load,
			struct clb_stats *B, struct clb_stats *L),

		TP_ARGS(tsk, threshold, status, curr_cpu, target_cpu, task_load, B, L),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, pid)
			__field(int, prio)
			__field(unsigned int, threshold)
			__field(unsigned int, status)
			__field(int, curr_cpu)
			__field(int, target_cpu)
			__field(int, curr_load)
			__field(int, target_load)
			__field(int, task_load)
			__field(int, B_load_avg)
			__field(int, L_load_avg)
			),

		TP_fast_assign(
				memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
				__entry->pid              = tsk->pid;
				__entry->prio             = tsk->prio;
				__entry->threshold        = threshold;
				__entry->status           = status;
				__entry->curr_cpu         = curr_cpu;
				__entry->target_cpu       = target_cpu;
				__entry->curr_load        = cpu_rq(curr_cpu)->cfs.avg.loadwop_avg;
				__entry->target_load      = cpu_rq(target_cpu)->cfs.avg.loadwop_avg;
				__entry->task_load        = task_load;
				__entry->B_load_avg       = B->load_avg;
				__entry->L_load_avg       = L->load_avg;
			      ),

		TP_printk(
				"pid=%4d prio=%d status=0x%4x dyn=%4u task-load=%4d curr-cpu=%d(%4d) target=%d(%4d) L-load-avg=%4d B-load-avg=%4d comm=%s",
				__entry->pid,
				__entry->prio,
				__entry->status,
				__entry->threshold,
				__entry->task_load,
				__entry->curr_cpu,
				__entry->curr_load,
				__entry->target_cpu,
				__entry->target_load,
				__entry->L_load_avg,
				__entry->B_load_avg,
				__entry->comm)
		);

/*
 * Tracepoint for showing the result of hmp task runqueue selection
 */
TRACE_EVENT(sched_hmp_select_task_rq,

		TP_PROTO(struct task_struct *tsk, int step, int sd_flag, int prev_cpu,
			int target_cpu, int task_load, struct clb_stats *B,
			struct clb_stats *L),

		TP_ARGS(tsk, step, sd_flag, prev_cpu, target_cpu, task_load, B, L),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, pid)
			__field(int, prio)
			__field(int, step)
			__field(int, sd_flag)
			__field(int, prev_cpu)
			__field(int, target_cpu)
			__field(int, prev_load)
			__field(int, target_load)
			__field(int, task_load)
			__field(int, B_load_avg)
			__field(int, L_load_avg)
			),

		TP_fast_assign(
				memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
				__entry->pid              = tsk->pid;
				__entry->prio             = tsk->prio;
				__entry->step             = step;
				__entry->sd_flag          = sd_flag;
				__entry->prev_cpu         = prev_cpu;
				__entry->target_cpu       = target_cpu;
				__entry->prev_load        = cpu_rq(prev_cpu)->cfs.avg.loadwop_avg;
				__entry->target_load      = cpu_rq(target_cpu)->cfs.avg.loadwop_avg;
				__entry->task_load        = task_load;
				__entry->B_load_avg       = B->load_avg;
				__entry->L_load_avg       = L->load_avg;
			      ),

		TP_printk(
				"pid=%4d prio=%d task-load=%4d sd-flag=%2d step=%d pre-cpu=%d(%4d) target=%d(%4d) L-load-avg=%4d B-load-avg=%4d comm=%s",
				__entry->pid,
				__entry->prio,
				__entry->task_load,
				__entry->sd_flag,
				__entry->step,
				__entry->prev_cpu,
				__entry->prev_load,
				__entry->target_cpu,
				__entry->target_load,
				__entry->L_load_avg,
				__entry->B_load_avg,
				__entry->comm)
		);

/*
 * Tracepoint for dumping hmp cluster load ratio
 */
TRACE_EVENT(sched_hmp_load,

		TP_PROTO(int B_load_avg, int L_load_avg),

		TP_ARGS(B_load_avg, L_load_avg),

		TP_STRUCT__entry(
			__field(int, B_load_avg)
			__field(int, L_load_avg)
			),

		TP_fast_assign(
			__entry->B_load_avg = B_load_avg;
			__entry->L_load_avg = L_load_avg;
			),

		TP_printk("B-load-avg=%4d L-load-avg=%4d",
			__entry->B_load_avg,
			__entry->L_load_avg)
	   );

/*
 * Tracepoint for dumping hmp statistics
 */
TRACE_EVENT(sched_hmp_stats,

		TP_PROTO(struct hmp_statisic *hmp_stats),

		TP_ARGS(hmp_stats),

		TP_STRUCT__entry(
			__field(unsigned int, nr_force_up)
			__field(unsigned int, nr_force_down)
			),

		TP_fast_assign(
			__entry->nr_force_up = hmp_stats->nr_force_up;
			__entry->nr_force_down = hmp_stats->nr_force_down;
			),

		TP_printk("nr-force-up=%d nr-force-down=%2d",
			__entry->nr_force_up,
			__entry->nr_force_down)
	   );

/*
 * Tracepoint for cfs task enqueue event
 */
TRACE_EVENT(sched_cfs_enqueue_task,

		TP_PROTO(struct task_struct *tsk, int tsk_load, int cpu_id),

		TP_ARGS(tsk, tsk_load, cpu_id),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, tsk_pid)
			__field(int, tsk_load)
			__field(int, cpu_id)
			),

		TP_fast_assign(
			memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
			__entry->tsk_pid = tsk->pid;
			__entry->tsk_load = tsk_load;
			__entry->cpu_id = cpu_id;
			),

		TP_printk("cpu-id=%d task-pid=%4d task-load=%4d comm=%s",
			__entry->cpu_id,
			__entry->tsk_pid,
			__entry->tsk_load,
			__entry->comm)
		);

/*
 * Tracepoint for cfs task dequeue event
 */
TRACE_EVENT(sched_cfs_dequeue_task,

		TP_PROTO(struct task_struct *tsk, int tsk_load, int cpu_id),

		TP_ARGS(tsk, tsk_load, cpu_id),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, tsk_pid)
			__field(int, tsk_load)
			__field(int, cpu_id)
			),

		TP_fast_assign(
			memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
			__entry->tsk_pid = tsk->pid;
			__entry->tsk_load = tsk_load;
			__entry->cpu_id = cpu_id;
			),

		TP_printk("cpu-id=%d task-pid=%4d task-load=%4d comm=%s",
			__entry->cpu_id,
			__entry->tsk_pid,
			__entry->tsk_load,
			__entry->comm)
		);

/*
 * Tracepoint for cfs runqueue load ratio update
 */
TRACE_EVENT(sched_cfs_load_update,

		TP_PROTO(struct task_struct *tsk, int tsk_load, int tsk_delta, int cpu_id),

		TP_ARGS(tsk, tsk_load, tsk_delta, cpu_id),

		TP_STRUCT__entry(
			__array(char, comm, TASK_COMM_LEN)
			__field(pid_t, tsk_pid)
			__field(int, tsk_load)
			__field(int, tsk_delta)
			__field(int, cpu_id)
			),

		TP_fast_assign(
			memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
			__entry->tsk_pid = tsk->pid;
			__entry->tsk_load = tsk_load;
			__entry->tsk_delta = tsk_delta;
			__entry->cpu_id = cpu_id;
			),

		TP_printk("cpu-id=%d task-pid=%4d task-load=%4d(%d) comm=%s",
				__entry->cpu_id,
				__entry->tsk_pid,
				__entry->tsk_load,
				__entry->tsk_delta,
				__entry->comm)
		);

/*
 * Tracepoint for showing tracked cfs runqueue runnable load.
 */
TRACE_EVENT(sched_cfs_runnable_load,

		TP_PROTO(int cpu_id, int cpu_load, int cpu_ntask),

		TP_ARGS(cpu_id, cpu_load, cpu_ntask),

		TP_STRUCT__entry(
			__field(int, cpu_id)
			__field(int, cpu_load)
			__field(int, cpu_ntask)
			),

		TP_fast_assign(
			__entry->cpu_id = cpu_id;
			__entry->cpu_load = cpu_load;
			__entry->cpu_ntask = cpu_ntask;
			),

		TP_printk("cpu-id=%d cfs-load=%4d, cfs-ntask=%2d",
			__entry->cpu_id,
			__entry->cpu_load,
			__entry->cpu_ntask)
		);

/*
 * Tracepoint for profiling runqueue length
 */
TRACE_EVENT(sched_runqueue_length,

		TP_PROTO(int cpu, int length),

		TP_ARGS(cpu, length),

		TP_STRUCT__entry(
			__field(int, cpu)
			__field(int, length)
			),

		TP_fast_assign(
			__entry->cpu = cpu;
			__entry->length = length;
			),

		TP_printk("cpu=%d rq-length=%2d",
			__entry->cpu,
			__entry->length)
	   );

TRACE_EVENT(sched_cfs_length,

		TP_PROTO(int cpu, int length),

		TP_ARGS(cpu, length),

		TP_STRUCT__entry(
			__field(int, cpu)
			__field(int, length)
			),

		TP_fast_assign(
			__entry->cpu = cpu;
			__entry->length = length;
			),

		TP_printk("cpu=%d cfs-length=%2d",
			__entry->cpu,
			__entry->length)
	   );
#endif /* CONFIG_HMP_TRACER */


#endif /* _TRACE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
