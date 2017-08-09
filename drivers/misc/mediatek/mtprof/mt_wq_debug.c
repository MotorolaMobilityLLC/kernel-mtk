#define DEBUG 1

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

struct pool_workqueue;
#include <trace/events/workqueue.h>

#define WQ_DUMP_QUEUE_WORK   0x1
#define WQ_DUMP_ACTIVE_WORK  0x2
#define WQ_DUMP_EXECUTE_WORK 0x4

#include "internal.h"

static unsigned int wq_tracing;

static void probe_execute_work(void *ignore, struct work_struct *work)
{
	pr_debug("execute start work=%p func=%pf\n",
		 (void *)work, (void *)work->func);
}

static void probe_execute_end(void *ignore, struct work_struct *work)
{
	pr_debug("execute end work=%p func=%pf\n",
		 (void *)work, (void *)work->func);
}

static void probe_activate_work(void *ignore, struct work_struct *work)
{
	pr_debug("activate work=%p func=%pf\n",
		 (void *)work, (void *)work->func);
}

static void
probe_queue_work(void *ignore, unsigned int req_cpu, struct pool_workqueue *pwq,
		 struct work_struct *work)
{
	pr_debug("queue work=%p func=%pf req_cpu=%u\n",
		 (void *)work, (void *)work->func, req_cpu);
}

static void print_help(struct seq_file *m)
{

	if (m != NULL) {
		SEQ_printf(m, "\n*** Usage ***\n");
		SEQ_printf(m, "commands to enable logs\n");
		SEQ_printf(m, "  echo [queue] [activate] [execute] > wq_enable_logs\n");
		SEQ_printf(m, "  ex. 1 1 1 to enable all logs\n");
		SEQ_printf(m, "  ex. 1 0 0 to enable queue logs\n");
	} else {
		pr_err("\n*** Usage ***\n");
		pr_err("commands to enable logs\n");
		pr_err("  echo [queue work] [activate work] [execute work] > wq_enable_logs\n");
		pr_err("  ex. 1 1 1 to enable all logs\n");
		pr_err("  ex. 1 0 0 to enable queue logs\n");
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

static ssize_t
mt_wq_log_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	unsigned int queue = (0 != (wq_tracing & WQ_DUMP_QUEUE_WORK));
	unsigned int activate = (0 != (wq_tracing & WQ_DUMP_ACTIVE_WORK));
	unsigned int execute = (0 != (wq_tracing & WQ_DUMP_EXECUTE_WORK));
	unsigned int toggle;
	int ret;
	int input[3];
	char buf[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = '\0';
	memset(input, 0, sizeof(input));
	ret = sscanf(buf, "%d %d %d", &input[0], &input[1], &input[2]);

	if (ret != 3) {
		print_help(NULL);
		return cnt;
	}

	toggle = (!!input[0] ^ queue);
	if (toggle && !queue) {
		register_trace_workqueue_queue_work(probe_queue_work, NULL);
		wq_tracing |= WQ_DUMP_QUEUE_WORK;
	} else if (toggle && queue) {
		unregister_trace_workqueue_queue_work(probe_queue_work, NULL);
		wq_tracing &= ~WQ_DUMP_QUEUE_WORK;
	}

	toggle = (!!input[1] ^ activate);
	if (toggle && !activate) {
		register_trace_workqueue_activate_work(probe_activate_work, NULL);
		wq_tracing |= WQ_DUMP_ACTIVE_WORK;
	} else if (toggle && activate) {
		unregister_trace_workqueue_activate_work(probe_activate_work, NULL);
		wq_tracing &= ~WQ_DUMP_ACTIVE_WORK;
	}

	toggle = (!!input[2] ^ execute);
	if (toggle && !execute) {
		register_trace_workqueue_execute_start(probe_execute_work, NULL);
		register_trace_workqueue_execute_end(probe_execute_end, NULL);
		wq_tracing |= WQ_DUMP_EXECUTE_WORK;
	} else if (toggle && execute) {
		unregister_trace_workqueue_execute_start(probe_execute_work, NULL);
		unregister_trace_workqueue_execute_end(probe_execute_end, NULL);
		wq_tracing &= ~WQ_DUMP_EXECUTE_WORK;
	}

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
