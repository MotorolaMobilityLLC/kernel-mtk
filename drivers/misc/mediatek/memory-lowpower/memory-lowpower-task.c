#define pr_fmt(fmt) "memory-lowpower-task: " fmt
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/rwlock.h>

/* Trigger method for screen on/off */
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/fb.h>
#endif

/* Memory lowpower private header file */
#include "internal.h"

/* Print wrapper */
#define MLPT_PRINT(args...)	do {} while (0) /* pr_alert(args) */
#define MLPT_PRERR(args...)	do {} while (0) /* pr_alert(args) */

/* Profile Timing */
#ifdef CONFIG_MLPT_PROFILE
static unsigned long long start_ns, end_ns;
#define MLPT_START_PROFILE()		{start_ns = sched_clock(); }
#define MLPT_END_PROFILE()	do {\
					end_ns = sched_clock();\
					MLPT_PRINT(" {{{Elapsed[%llu]ns}}}\n", (end_ns - start_ns));\
				} while (0)
#else	/* !CONFIG_MLPT_PROFILE */
#define MLPT_START_PROFILE()	do {} while (0)
#define MLPT_END_PROFILE()	do {} while (0)
#endif

/* List of memory lowpower features' specific operations */
static LIST_HEAD(memory_lowpower_handlers);
static DEFINE_MUTEX(memory_lowpower_lock);

/* Control parameters for memory lowpower task */
static struct task_struct *memory_lowpower_task;
static enum power_state memory_lowpower_action;
static unsigned long memory_lowpower_state;
static int get_cma_aligned;			/* in PAGE_SIZE order */
static int get_cma_num;				/* Number of allocation */
static unsigned long get_cma_size;		/* in PAGES */
static struct page **cma_aligned_pages;

/*
 * Set aligned allocation -
 * @aligned: Requested alignment of pages (in PAGE_SIZE order).
 */
void set_memory_lowpower_aligned(int aligned)
{
	unsigned long size, num;

	/* No need to update */
	if ((get_cma_aligned != 0 && get_cma_aligned <= aligned) || aligned < 0)
		return;

	/* cma_aligned_pages is in use */
	if (get_cma_aligned != 0 && cma_aligned_pages[0] != NULL)
		return;

	/* Check whether size is a multiple of num */
	size = (memory_lowpower_cma_size() >> PAGE_SHIFT);
	num = size >> aligned;
	if (size != (num << aligned))
		return;

	/* Update aligned allocation */
	get_cma_aligned = aligned;
	get_cma_size = 1 << aligned;
	get_cma_num = num;
	if (cma_aligned_pages != NULL) {
		kfree(cma_aligned_pages);
		cma_aligned_pages = NULL;
	}

	/* If it is page-aligned, cma_aligned_pages is not needed */
	if (num != size)
		cma_aligned_pages = kcalloc(num, sizeof(*cma_aligned_pages), GFP_KERNEL);

	MLPT_PRINT("%s: aligned[%d] size[%lu] num[%d] array[%p]\n",
			__func__, get_cma_aligned, get_cma_size, get_cma_num, cma_aligned_pages);
}

/* Insert allocated buffer */
static void insert_buffer(struct page *page, int bound)
{
#ifdef MLPT_INSERT_SORT
	struct page *tmp;
	int i;

	MLPT_PRINT("%s: PFN[%lu]\n", __func__, page_to_pfn(page));
	for (i = 0; i < bound; i++) {
		MLPT_PRINT("[%d] ", i);
		if (page < cma_aligned_pages[i]) {
			tmp = cma_aligned_pages[i];
			MLPT_PRINT("Swap (page)PFN[%lu] (tmp)PFN[%lu]\n", page_to_pfn(page), page_to_pfn(tmp));
			cma_aligned_pages[i] = page;
			page = tmp;
		}
		MLPT_PRINT("\n");
	}
	cma_aligned_pages[i] = page;
#else
	MLPT_PRINT("%s: PFN[%lu] index[%d]\n", __func__, page_to_pfn(page), bound);
	cma_aligned_pages[bound] = page;
#endif
}

/* Wrapper for memory lowpower CMA allocation */
static int acquire_memory(void)
{
	int i = 0, ret;
	struct page *page;

	/* Full allocation */
	if (cma_aligned_pages == NULL)
		return get_memory_lowpower_cma();

	/* Find the 1st null position */
	while (i < get_cma_num && cma_aligned_pages[i] != NULL)
		++i;

	/* Aligned allocation */
	while (i < get_cma_num) {
		ret = get_memory_lowpower_cma_aligned(get_cma_size, get_cma_aligned, &page);
		if (ret)
			break;
		MLPT_PRINT("%s: PFN[%lu] allocated for [%d]\n", __func__, page_to_pfn(page), i);
		insert_buffer(page, i);
		++i;
	}

	return 0;
}

/* Wrapper for memory lowpower CMA free */
static int release_memory(void)
{
	int i = 0, ret;
	struct page **pages;

	/* Full release */
	if (cma_aligned_pages == NULL)
		return put_memory_lowpower_cma();

	/* Aligned release */
	pages = cma_aligned_pages;
	do {
		if (pages[i] == NULL)
			break;
		ret = put_memory_lowpower_cma_aligned(get_cma_size, pages[i]);
		if (!ret) {
			MLPT_PRINT("%s: PFN[%lu] released for [%d]\n", __func__, page_to_pfn(pages[i]), i);
			pages[i] = NULL;
		}
	} while (++i < get_cma_num);

	return 0;
}

/* Query CMA allocated buffer */
static void memory_range(int which, unsigned long *spfn, unsigned long *epfn)
{
	*spfn = *epfn = 0;

	/* Sanity check */
	if (which >= get_cma_num)
		goto out;

	/* Range of full allocation */
	if (cma_aligned_pages == NULL) {
		*spfn = __phys_to_pfn(memory_lowpower_cma_base());
		*epfn = __phys_to_pfn(memory_lowpower_cma_base() + memory_lowpower_cma_size());
		goto out;
	}

	/* Range of aligned allocation */
	if (cma_aligned_pages[which] != NULL) {
		*spfn = page_to_pfn(cma_aligned_pages[which]);
		*epfn = *spfn + get_cma_size;
	}

out:
	MLPT_PRINT("%s: [%d] spfn[%lu] epfn[%lu]\n", __func__, which, *spfn, *epfn);
}

/* Register API for memory lowpower operation */
void register_memory_lowpower_operation(struct memory_lowpower_operation *handler)
{
	struct list_head *pos;

	mutex_lock(&memory_lowpower_lock);
	list_for_each(pos, &memory_lowpower_handlers) {
		struct memory_lowpower_operation *e;

		e = list_entry(pos, struct memory_lowpower_operation, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&memory_lowpower_lock);
}

/* Unregister API for memory lowpower operation */
void unregister_memory_lowpower_operation(struct memory_lowpower_operation *handler)
{
	mutex_lock(&memory_lowpower_lock);
	list_del(&handler->link);
	mutex_unlock(&memory_lowpower_lock);
}

/* Screen-on cb operations */
static void __go_to_screenon(void)
{
	struct memory_lowpower_operation *pos;
	int ret = 0;
	int disabled[NR_MLP_LEVEL] = { 0, };

	/* Apply HW actions if needed */
	if (!MlpsEnable(&memory_lowpower_state))
		return;

	/* Disable actions */
	list_for_each_entry(pos, &memory_lowpower_handlers, link) {
		if (pos->disable != NULL) {
			ret = pos->disable();
			if (ret) {
				disabled[pos->level] += ret;
				MLPT_PRERR("Fail disable: level[%d] ret[%d]\n", pos->level, ret);
				ret = 0;
			}
		}
	}

	/* Restore actions */
	list_for_each_entry(pos, &memory_lowpower_handlers, link) {
		if (pos->restore != NULL) {
			ret = pos->restore();
			if (ret) {
				disabled[pos->level] += ret;
				MLPT_PRERR("Fail restore: level[%d] ret[%d]\n", pos->level, ret);
				ret = 0;
			}
		}
	}

	/* Clear ENABLE state */
	ClearMlpsEnable(&memory_lowpower_state);

	if (IS_ENABLED(CONFIG_MTK_DCS) && !disabled[MLP_LEVEL_DCS])
		ClearMlpsEnableDCS(&memory_lowpower_state);

	if (IS_ENABLED(CONFIG_MTK_PASR) && !disabled[MLP_LEVEL_PASR])
		ClearMlpsEnablePASR(&memory_lowpower_state);
}

/* Screen-on operations */
static void go_to_screenon(void)
{
	MLPT_PRINT("%s:+\n", __func__);
	MLPT_START_PROFILE();

	/* Should be SCREENOFF|SCREENIDLE -> SCREENON */
	if (MlpsScreenOn(&memory_lowpower_state) &&
			!MlpsScreenIdle(&memory_lowpower_state)) {
		MLPT_PRERR("Wrong state[%lu]\n", memory_lowpower_state);
		goto out;
	}

	/* HW-related flow for screenon */
	__go_to_screenon();

	/* Currently in SCREENOFF */
	if (!MlpsScreenOn(&memory_lowpower_state))
		SetMlpsScreenOn(&memory_lowpower_state);

	/* Currently in SCREENIDLE */
	if (MlpsScreenIdle(&memory_lowpower_state))
		ClearMlpsScreenIdle(&memory_lowpower_state);

	/* Release pages */
	release_memory();

out:
	MLPT_END_PROFILE();
	MLPT_PRINT("%s:-\n", __func__);
}

/* Screen-off cb operations */
static void __go_to_screenoff(void)
{
	struct memory_lowpower_operation *pos;
	int ret = 0;
	int enabled[NR_MLP_LEVEL] = { 0, };

	/* Apply HW actions if needed */
	if (!IS_ACTION_SCREENOFF(memory_lowpower_action))
		return;

	/* Config actions */
	list_for_each_entry(pos, &memory_lowpower_handlers, link) {
		if (pos->config != NULL) {
			ret = pos->config(get_cma_num, memory_range);
			if (ret) {
				enabled[pos->level] += ret;
				MLPT_PRERR("Fail config: level[%d] ret[%d]\n", pos->level, ret);
				ret = 0;
			}
		}
	}

	/* Enable actions */
	list_for_each_entry(pos, &memory_lowpower_handlers, link) {
		if (pos->enable != NULL) {
			ret = pos->enable();
			if (ret) {
				enabled[pos->level] += ret;
				MLPT_PRERR("Fail enable: level[%d] ret[%d]\n", pos->level, ret);
				ret = 0;
			}
		}
	}

	/* Set ENABLE state */
	SetMlpsEnable(&memory_lowpower_state);

	if (IS_ENABLED(CONFIG_MTK_DCS) && !enabled[MLP_LEVEL_DCS])
		SetMlpsEnableDCS(&memory_lowpower_state);

	if (IS_ENABLED(CONFIG_MTK_PASR) && !enabled[MLP_LEVEL_PASR])
		SetMlpsEnablePASR(&memory_lowpower_state);
}

/* Screen-off operations */
static void go_to_screenoff(void)
{
	MLPT_PRINT("%s:+\n", __func__);
	MLPT_START_PROFILE();

	/* Should be SCREENON -> SCREENOFF */
	if (!MlpsScreenOn(&memory_lowpower_state)) {
		MLPT_PRERR("Wrong state[%lu]\n", memory_lowpower_state);
		goto out;
	}

	/* Collect free pages */
	do {
		/* Try to collect free pages. If done or can't proceed, then break. */
		if (!acquire_memory())
			break;

		/* Action is changed, just leave here. */
		if (!IS_ACTION_SCREENOFF(memory_lowpower_action))
			goto out;

	} while (1);

	/* Clear SCREENON state */
	ClearMlpsScreenOn(&memory_lowpower_state);

	/* HW-related flow for screenoff */
	__go_to_screenoff();

out:
	MLPT_END_PROFILE();
	MLPT_PRINT("%s:-\n", __func__);
}

/* Screen-idle operations */
static void go_to_screenidle(void)
{
	MLPT_PRINT("%s:+\n", __func__);
	MLPT_START_PROFILE();
	/* Actions for screenidle - TBD */
	MLPT_END_PROFILE();
	MLPT_PRINT("%s:-\n", __func__);
}

/*
 * Main entry for memory lowpower operations -
 * No set_freezable(), no try_to_freeze().
 */
static int memory_lowpower_entry(void *p)
{
	enum power_state current_action = MLP_INIT;

	/* Call freezer_do_not_count to skip me */
	freezer_do_not_count();

	/* Start actions */
	do {
		/* Start running */
		set_current_state(TASK_RUNNING);
		do {
			/* Take proper actions */
			current_action = memory_lowpower_action;
			switch (current_action) {
			case MLP_SCREENON:
				go_to_screenon();
				break;
			case MLP_SCREENOFF:
				go_to_screenoff();
				break;
			case MLP_SCREENIDLE:
				go_to_screenidle();
				break;
			default:
				MLPT_PRINT("%s: Invalid action[%d]\n", __func__, current_action);
			}
		} while (current_action != memory_lowpower_action);

		/* Schedule me */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	} while (1);

	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
/* Early suspend/resume callbacks & descriptor */
static void memory_lowpower_early_suspend(struct early_suspend *h)
{
	MLPT_PRINT("%s: SCREENOFF!\n", __func__);
	memory_lowpower_action = MLP_SCREENOFF;

	/* Wake up task */
	wake_up_process(memory_lowpower_task);
}

static void memory_lowpower_late_resume(struct early_suspend *h)
{
	MLPT_PRINT("%s: SCREENON!\n", __func__);
	memory_lowpower_action = MLP_SCREENON;

	/* Wake up task */
	wake_up_process(memory_lowpower_task);
}

static struct early_suspend early_suspend_descriptor = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
	.suspend = memory_lowpower_early_suspend,
	.resume = memory_lowpower_late_resume,
};
#else /* !CONFIG_HAS_EARLYSUSPEND */
/* FB event notifier */
static int memory_lowpower_fb_event(struct notifier_block *notifier, unsigned long event, void *data)
{
	struct fb_event *fb_event = data;
	int *blank = fb_event->data;
	int new_status = *blank ? 1 : 0;

	switch (event) {
	case FB_EVENT_BLANK:
		if (new_status == 0) {
			MLPT_PRINT("%s: SCREENON!\n", __func__);
			memory_lowpower_action = MLP_SCREENON;
			wake_up_process(memory_lowpower_task);
		} else {
			MLPT_PRINT("%s: SCREENOFF!\n", __func__);
			memory_lowpower_action = MLP_SCREENOFF;
			wake_up_process(memory_lowpower_task);
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block fb_notifier_block = {
	.notifier_call = memory_lowpower_fb_event,
	.priority = 0,
};
#endif

static int __init memory_lowpower_init_pm_ops(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&early_suspend_descriptor);
#else
	if (fb_register_client(&fb_notifier_block) != 0)
		return -1;
#endif

	return 0;
}
#endif

int __init memory_lowpower_task_init(void)
{
	int ret = 0;

	/* Start a kernel thread */
	memory_lowpower_task = kthread_run(memory_lowpower_entry, NULL, "memory_lowpower_task");
	if (IS_ERR(memory_lowpower_task)) {
		MLPT_PRERR("Failed to start memory_lowpower_task!\n");
		ret = PTR_ERR(memory_lowpower_task);
		goto out;
	}

#ifdef CONFIG_PM
	/* Initialize PM ops */
	ret = memory_lowpower_init_pm_ops();
	if (ret != 0) {
		MLPT_PRERR("Failed to init pm ops!\n");
		kthread_stop(memory_lowpower_task);
		memory_lowpower_task = NULL;
		goto out;
	}
#endif

	/* Set expected current state */
	SetMlpsInit(&memory_lowpower_state);
	SetMlpsScreenOn(&memory_lowpower_state);

out:
	MLPT_PRINT("%s: memory_power_state[%lu]\n", __func__, memory_lowpower_state);
	return ret;
}

late_initcall(memory_lowpower_task_init);
