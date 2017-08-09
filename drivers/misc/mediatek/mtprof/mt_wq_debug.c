#define DEBUG 1

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/xlog.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

struct pool_workqueue;
#include <trace/events/workqueue.h>

#define WQ_VERBOSE_DEBUG 0

#define WQ_DUMP_QUEUE_WORK   0x1
#define WQ_DUMP_ACTIVE_WORK  0x2
#define WQ_DUMP_EXECUTE_WORK 0x4

#define SEQ_printf(m, x...)	    \
	do {			    \
		if (m)		    \
			seq_printf(m, x);	\
		else		    \
			pr_err(x);	    \
	} while (0)

#define MT_DEBUG_ENTRY(name) \
static int mt_##name##_show(struct seq_file *m, void *v);\
static ssize_t mt_##name##_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data);\
static int mt_##name##_open(struct inode *inode, struct file *file) \
{ \
	return single_open(file, mt_##name##_show, inode->i_private); \
} \
\
static const struct file_operations mt_##name##_fops = { \
	.open = mt_##name##_open, \
	.write = mt_##name##_write, \
	.read = seq_read, \
	.llseek = seq_lseek, \
	.release = single_release, \
}; \
void mt_##name##_switch(int on)

static int wq_tracing;

static void mttrace_workqueue_execute_work(void *ignore, struct work_struct *work)
{
	pr_debug("execute work=%p\n", (void *)work);
}

static void mttrace_workqueue_execute_end(void *ignore, struct work_struct *work)
{
	pr_debug("execute end work=%p\n", (void *)work);
}

static void mttrace_workqueue_activate_work(void *ignore, struct work_struct *work)
{
	pr_debug("activate work=%p\n", (void *)work);
}

static void mttrace_workqueue_queue_work(void *ignore, unsigned int req_cpu, struct pool_workqueue *pwq,
					 struct work_struct *work)
{
	pr_debug("queue work=%p function=%pf req_cpu=%u\n",
		 (void *)work, (void *)work->func, req_cpu);
}

static void print_help(struct seq_file *m)
{

	if (m != NULL) {
		SEQ_printf(m, "\n*** Usage ***\n");
		SEQ_printf(m, "commands to enable logs\n");
		SEQ_printf(m,
			   "  echo [queue work] [activate work] [execute work] > wq_enable_logs\n");
		SEQ_printf(m, "  ex. echo 1 1 1 > wq_enable_logs, to enable all logs\n");
		SEQ_printf(m,
			   "  ex. echo 1 0 0 > wq_enable_logs, to enable \"queue work\" & \"wq debug\" logs\n");
	} else {
		pr_err("\n*** Usage ***\n");
		pr_err("commands to enable logs\n");
		pr_err
		    ("  echo [queue work] [activate work] [execute work] > wq_enable_logs\n");
		pr_err("  ex. echo 1 1 1 > wq_enable_logs, to enable all logs\n");
		pr_err
		    ("  ex. echo 1 0 0 > wq_enable_logs, to enable \"queue work\" logs\n");
	}
}

MT_DEBUG_ENTRY(wq_log);
static int mt_wq_log_show(struct seq_file *m, void *v)
{
	if (wq_tracing & WQ_DUMP_QUEUE_WORK)
		SEQ_printf(m, "wq: queue work log enabled\n");
	if (wq_tracing & WQ_DUMP_ACTIVE_WORK)
		SEQ_printf(m, "wq: active work log enabled\n");
	if (wq_tracing & WQ_DUMP_EXECUTE_WORK)
		SEQ_printf(m, "wq: execute work log enabled\n");
	if (wq_tracing == 0)
		SEQ_printf(m, "wq: no log enabled\n");

	print_help(m);
	return 0;
}

static ssize_t mt_wq_log_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	int log_queue_work = 0, log_activate_work = 0, log_execute_work = 0;
	char buf[64];

	if (cnt > sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = '\0';

	if (sscanf
	    (buf, "%d %d %d", &log_queue_work, &log_activate_work, &log_execute_work) == 3) {

		if (!!log_queue_work ^ ((wq_tracing & WQ_DUMP_QUEUE_WORK) != 0)) {
			if (!(wq_tracing & WQ_DUMP_QUEUE_WORK)) {
				register_trace_workqueue_queue_work(mttrace_workqueue_queue_work,
								    NULL);
				wq_tracing |= WQ_DUMP_QUEUE_WORK;
			} else {
				unregister_trace_workqueue_queue_work(mttrace_workqueue_queue_work,
								    NULL);
				wq_tracing &= ~WQ_DUMP_QUEUE_WORK;
			}
		}
		if (!!log_activate_work ^ ((wq_tracing & WQ_DUMP_ACTIVE_WORK) != 0)) {
			if (!(wq_tracing & WQ_DUMP_ACTIVE_WORK)) {
				register_trace_workqueue_activate_work(mttrace_workqueue_activate_work,
								    NULL);
				wq_tracing |= WQ_DUMP_ACTIVE_WORK;
			} else {
				unregister_trace_workqueue_activate_work(mttrace_workqueue_activate_work,
								    NULL);
				wq_tracing &= ~WQ_DUMP_ACTIVE_WORK;
			}
		}
		if (!!log_execute_work ^ ((wq_tracing & WQ_DUMP_EXECUTE_WORK) != 0)) {
			if (!(wq_tracing & WQ_DUMP_EXECUTE_WORK)) {
				register_trace_workqueue_execute_start(mttrace_workqueue_execute_work,
								    NULL);
				register_trace_workqueue_execute_end(mttrace_workqueue_execute_end,
								    NULL);
				wq_tracing |= WQ_DUMP_EXECUTE_WORK;
			} else {
				unregister_trace_workqueue_execute_start(mttrace_workqueue_execute_work,
								    NULL);
				unregister_trace_workqueue_execute_end(mttrace_workqueue_execute_end,
								    NULL);
				wq_tracing &= ~WQ_DUMP_EXECUTE_WORK;
			}
		}
	} else
		print_help(NULL);

	return cnt;
}

static int __init init_wq_debug(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("mtprof/wq_enable_logs", 0664, NULL, &mt_wq_log_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}
device_initcall(init_wq_debug);
