#define pr_fmt(fmt) "memory-ssvp: " fmt
#define CONFIG_MTK_MEMORY_SSVP_DEBUG

#define CONFIG_MTK_MEMORY_SSVP_WRAP

#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/stat.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <asm/page.h>
#include <asm-generic/memory_model.h>

#if 0
static struct cma *cma;
static struct page *cma_pages;
#endif

static DEFINE_MUTEX(memory_ssvp_mutex);
static unsigned long svp_usage_count;

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
static struct page *wrap_cma_pages;
#endif

#if 0
/*
 * get_memory_ssvp_cma - allocate all cma memory belongs to ssvp cma
 *
 */
int get_memory_ssvp_cma(void)
{
	int count = cma_get_size(cma) >> PAGE_SHIFT;

	if (cma_pages) {
		pr_alert("cma already collected\n");
		goto out;
	}

	mutex_lock(&memory_ssvp_mutex);

	cma_pages = cma_alloc(cma, count, 0);

	if (cma_pages)
		pr_debug("%s:%d ok\n", __func__, __LINE__);
	else
		pr_alert("ssvp cma allocation failed\n");

	mutex_unlock(&memory_ssvp_mutex);

out:
	return 0;
}

/*
 * put_memory_ssvp_cma - free all cma memory belongs to ssvp cma
 *
 * It returns 0 is success, otherwise returns -ENOMEM
 */
int put_memory_ssvp_cma(void)
{
	int ret;
	int count = cma_get_size(cma) >> PAGE_SHIFT;

	mutex_lock(&memory_ssvp_mutex);

	if (cma_pages) {
		ret = cma_release(cma, cma_pages, count);
		if (!ret) {
			pr_err("%s incorrect pages: %p(%lx)\n",
					__func__,
					cma_pages, page_to_pfn(cma_pages));
			return -EINVAL;
		}
		cma_pages = 0;
	}

	mutex_unlock(&memory_ssvp_mutex);

	return 0;
}
#endif

#ifdef CONFIG_MTK_MEMORY_SSVP_DEBUG

#endif /* CONFIG_MTK_MEMORY_SSVP_DEBUG */

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include "sh_svp.h"

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(CONFIG_MT_ENG_BUILD)
#include <mt-plat/aee.h>
/*#include <linux/disp_assert_layer.h>*/
#endif

enum ssvp_subtype {
	SSVP_SUB_SVP,
	SSVP_SUB_TUI,
	__MAX_NR_SSVPSUBS,
};

#define SVP_STATE_NULL		0x00
#define SVP_STATE_ONING_WAIT	0x01
#define SVP_STATE_ONING		0x02
#define SVP_STATE_ON		0x03
#define SVP_STATE_OFFING	0x04
#define SVP_STATE_OFF		0x05

#define _SSVP_MBSIZE_ CONFIG_MTK_SVP_RAM_SIZE

struct ssvp_cma {
	struct cma *cma;

	unsigned long	base_pfn;
	unsigned long	count;

	unsigned long	end_pfn;
	phys_addr_t		base;
};

struct SSVP_Region {
	char state;
	unsigned long start;
	unsigned long count;
	struct page *page;
};

static int ires;
/*static struct device *cma_dev;*/
/* static char _svp_state; */
/* static struct page *_page; */
static int _svp_onlinewait_tries;
static struct task_struct *_svp_online_task; /* NULL */

struct ssvp_cma ssvp_cma;

static struct SSVP_Region _svpregs[__MAX_NR_SSVPSUBS];

#define _SVP_MBSIZE (CONFIG_MTK_SVP_RAM_SIZE - CONFIG_MTK_TUI_RAM_SIZE)

#define _TUI_MBSIZE CONFIG_MTK_TUI_RAM_SIZE

static int memory_ssvp_init(struct reserved_mem *rmem)
{
	int ret;

	pr_debug("%s, name: %s, base: 0x%pa, size: 0x%pa\n",
		 __func__, rmem->name,
		 &rmem->base, &rmem->size);

	/* init cma area */
	ret = cma_init_reserved_mem(rmem->base, rmem->size , 0, &ssvp_cma.cma);

	if (ret) {
		pr_err("%s cma failed, ret: %d\n", __func__, ret);
		return 1;
	}

	ssvp_cma.base = cma_get_base(ssvp_cma.cma);
	ssvp_cma.base_pfn = page_to_pfn(phys_to_page(cma_get_base(ssvp_cma.cma)));
	ssvp_cma.count = cma_get_size(ssvp_cma.cma) >> PAGE_SHIFT;
	ssvp_cma.end_pfn = ssvp_cma.base_pfn + ssvp_cma.count;

	return 0;
}

RESERVEDMEM_OF_DECLARE(memory_ssvp, "mediatek,memory-ssvp",
			memory_ssvp_init);

/*
 * Check whether memory_lowpower is initialized
 */
bool memory_ssvp_inited(void)
{
	return (ssvp_cma.cma != NULL);
}

/*
 * memory_lowpower_cma_base - query the cma's base
 */
phys_addr_t memory_ssvp_cma_base(void)
{
	return cma_get_base(ssvp_cma.cma);
}

/*
 * memory_lowpower_cma_size - query the cma's size
 */
phys_addr_t memory_ssvp_cma_size(void)
{
	return cma_get_size(ssvp_cma.cma);
}

int tui_region_offline(phys_addr_t *pa, unsigned long *size)
{
	struct page *page;
	int retval = 0;

	pr_alert("%s %d: tui to offline enter state: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_TUI].state);

	if (_svpregs[SSVP_SUB_TUI].state == SVP_STATE_ON) {
		_svpregs[SSVP_SUB_TUI].state = SVP_STATE_OFFING;

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
		page = wrap_cma_pages;
#else
/*		page = dma_alloc_from_contiguous_start(
				cma_dev, _svpregs[SSVP_SUB_TUI].start,
				_svpregs[SSVP_SUB_TUI].count, 0);
*/
		page = cma_alloc(
				ssvp_cma.cma,
				_svpregs[SSVP_SUB_TUI].count, 0);
		if (page) {
			svp_usage_count += _svpregs[SSVP_SUB_TUI].count;
		}
#endif

		if (page) {
			_svpregs[SSVP_SUB_TUI].page = page;
			_svpregs[SSVP_SUB_TUI].state = SVP_STATE_OFF;

			if (pa)
				*pa = ssvp_cma.base + (_svpregs[SSVP_SUB_TUI].start << PAGE_SHIFT);
			if (size)
				*size = _svpregs[SSVP_SUB_TUI].count << PAGE_SHIFT;

			pr_alert("%s %d: pa %llx, size %lu\n",
					__func__, __LINE__,
					ssvp_cma.base + (_svpregs[SSVP_SUB_TUI].start << PAGE_SHIFT),
					_svpregs[SSVP_SUB_TUI].count << PAGE_SHIFT);
		} else {
			_svpregs[SSVP_SUB_TUI].state = SVP_STATE_ON;
			retval = -EAGAIN;
		}
	} else {
		retval = -EBUSY;
	}

	pr_alert("%s %d: tui to offline leave state: %d, retval: %d\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_TUI].state, retval);

	return retval;
}
EXPORT_SYMBOL(tui_region_offline);

int tui_region_online(void)
{
	int retval = 0;
	bool retb;

	pr_alert("%s %d: tui to online enter state: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_TUI].state);

	if (_svpregs[SSVP_SUB_TUI].state == SVP_STATE_OFF) {
		_svpregs[SSVP_SUB_TUI].state = SVP_STATE_ONING_WAIT;

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
		retb = true;
#else
/*		retb = dma_release_from_contiguous(cma_dev,
			_svpregs[SSVP_SUB_TUI].page, _svpregs[SSVP_SUB_TUI].count);
*/
		retb = cma_release(ssvp_cma.cma, _svpregs[SSVP_SUB_TUI].page, _svpregs[SSVP_SUB_TUI].count);
		if (retb == true) {
			svp_usage_count -= _svpregs[SSVP_SUB_TUI].count;
		}
#endif

		if (retb == true) {
			_svpregs[SSVP_SUB_TUI].page = NULL;
			_svpregs[SSVP_SUB_TUI].state = SVP_STATE_ON;
		}
	} else {
		retval = -EBUSY;
	}

	pr_alert("%s %d: tui to online leave state: %d, retval: %d\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_TUI].state, retval);

	return retval;
}
EXPORT_SYMBOL(tui_region_online);

static ssize_t
tui_cma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	pr_alert("%s %d: tui state: %d ires: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_TUI].state, ires);

	return 0;
}

static ssize_t
tui_cma_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned char val;
	int retval = count;

	pr_alert("%s %d: count: %zu\n", __func__, __LINE__, count);

	if (count >= 2) {
		if (copy_from_user(&val, &buf[0], 1)) {
			retval = -EFAULT;
			goto out;
		}

		if (count == 2 && val == '0')
			tui_region_offline(NULL, NULL);
		else
			tui_region_online();
	}

out:
	return retval;
}

static const struct file_operations tui_cma_fops = {
	.owner =    THIS_MODULE,
	.read  =    tui_cma_read,
	.write =    tui_cma_write,
};

static int __svp_region_online(void)
{
	int retval = 0;
	bool retb;

	pr_alert("%s %d: svp to online enter state: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_SVP].state);

	if (_svpregs[SSVP_SUB_SVP].state == SVP_STATE_ONING_WAIT) {
		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ONING;

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
	retb = true;
#else
/*		retb = dma_release_from_contiguous(cma_dev,
			_svpregs[SSVP_SUB_SVP].page, _svpregs[SSVP_SUB_SVP].count);
*/
		retb = cma_release(ssvp_cma.cma, _svpregs[SSVP_SUB_SVP].page, _svpregs[SSVP_SUB_SVP].count);
		if (retb == true) {
			svp_usage_count -= _svpregs[SSVP_SUB_SVP].count;
		}
#endif

		if (retb == true) {
			_svpregs[SSVP_SUB_SVP].page = NULL;
			_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ON;
		}
	} else {
		retval = -EBUSY;
	}

	pr_alert("%s %d: svp to online leave state: %d, retval: %d\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_SVP].state, retval);

	return retval;
}

static int _svp_online_kthread_func(void *data)
{
	u32 secusage = 0;
	int secdisable = 0;
	int vret;

	pr_alert("%s %d: start\n", __func__, __LINE__);

	_svp_onlinewait_tries = 0;

	while (_svp_onlinewait_tries < 30) {
		msleep(100);
		secusage = 0;

		/* call secmem_query */
		vret = secmem_api_query(&secusage);

		_svp_onlinewait_tries++;
		pr_alert("%s %d: vret: %d, _svp_onlinewait_tries: %d, secusage: %d\n",
				__func__, __LINE__, vret, _svp_onlinewait_tries, secusage);

		/* for no TEE test */
		/*
		if (_svp_onlinewait_tries < 1) {
			secusage = 10;
		}*/

		if (!secusage)
			break;
	}

	if (_svp_onlinewait_tries >= 30) {
		/* to do, show error */
		pr_alert("%s %d: _svp_onlinewait_tries: %d, secusage: %d\n",
				__func__, __LINE__, _svp_onlinewait_tries, secusage);

		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_OFF;

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(CONFIG_MT_ENG_BUILD)
		{
			#define MSG_SIZE_TO_AEE 70
			char msg_to_aee[MSG_SIZE_TO_AEE];

			pr_alert("Shareable SVP trigger kernel warning, vret: %d, secusage: %d\n", vret, secusage);

			snprintf(msg_to_aee, MSG_SIZE_TO_AEE, "please contact SS memory module owner\n");
			aee_kernel_warning_api("SVP", 0, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY|DB_OPT_LOW_MEMORY_KILLER
								| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
								| DB_OPT_PROCESS_COREDUMP
								| DB_OPT_DUMPSYS_SURFACEFLINGER
								| DB_OPT_DUMPSYS_GFXINFO
								| DB_OPT_DUMPSYS_PROCSTATS,
								"SVP release Failed\nCRDISPATCH_KEY:SVP_SS1",
								msg_to_aee);
		}
#endif

		goto out;
	}

	/* call secmem_disable */
	secdisable = secmem_api_disable();

	pr_alert("%s %d: _svp_onlinewait_tries: %d, secusage: %d, secdisable %d\n",
			__func__, __LINE__, _svp_onlinewait_tries, secusage, secdisable);

	if (!secdisable) {
		__svp_region_online();

		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ON;
	} else { /* to do, error handle */

		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_OFF;

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(CONFIG_MT_ENG_BUILD)
		{
			#define MSG_SIZE_TO_AEE 70
			char msg_to_aee[MSG_SIZE_TO_AEE];

			pr_alert("Shareable SVP trigger kernel warning, secdisable %d\n", secdisable);

			snprintf(msg_to_aee, MSG_SIZE_TO_AEE, "please contact SS memory module owner\n");
			aee_kernel_warning_api("SVP", 0, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY|DB_OPT_LOW_MEMORY_KILLER
								| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
								| DB_OPT_PROCESS_COREDUMP
								| DB_OPT_DUMPSYS_SURFACEFLINGER
								| DB_OPT_DUMPSYS_GFXINFO
								| DB_OPT_DUMPSYS_PROCSTATS,
								"SVP release Failed\nCRDISPATCH_KEY:SVP_SS1",
								msg_to_aee);
		}
#endif
	}

out:
	_svp_online_task = NULL;

	pr_alert("%s %d: end, _svp_onlinewait_tries: %d\n", __func__, __LINE__, _svp_onlinewait_tries);

	return 0;
}

#if 0
static int __svp_migrate_range(unsigned long start, unsigned long end)
{
	int ret = 0;

	mutex_lock(&memory_ssvp_mutex);

/* SVP 16 */
/*	ret = alloc_contig_range(start, end, MIGRATE_CMA, 0);
*/
/*	ret = alloc_contig_range(start, end, MIGRATE_MOVABLE, 0);*/

/* need check*/
/*	ret = alloc_contig_range(start, end, MIGRATE_MOVABLE);*/
	ret = alloc_contig_range(start, end);

	if (ret == 0) {
		free_contig_range(start, end - start);
	} else {
		/* error handle */
		pr_alert("%s %d: alloc failed: [%lu %lu) at [%lu - %lu)\n",
			__func__, __LINE__, start, end, get_svp_cma_basepfn(),
			get_svp_cma_basepfn() + get_svp_cma_count());
	}

	mutex_unlock(&memory_ssvp_mutex);

	return ret;
}

/**
 * svp_contiguous_reserve() - reserve area(s) for contiguous memory handling
 * @limit: End address of the reserved memory (optional, 0 for any).
 *
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory.
 */
void __init svp_contiguous_reserve(phys_addr_t limit)
{
	phys_addr_t selected_size = _SSVP_MBSIZE_ * 1024 * 1024;

	pr_alert("%s(limit %08lx)\n", __func__, (unsigned long)limit);

	if (selected_size && !ssvp_cma) {
		pr_debug("%s: reserving %ld MiB for svp area\n", __func__,
			 (unsigned long)selected_size / SZ_1M);

		ires = dma_contiguous_reserve_area(selected_size, 0, limit,
					    &ssvp_cma);
	}
};

int svp_migrate_range(unsigned long pfn)
{
	int ret = 0;

	if (!svp_is_in_range(pfn))
		return 1;

	ret = __svp_migrate_range(pfn, pfn + 1);

	if (ret == 0) {
		/* success */

	} else {
		/* error handle */
		pr_alert("%s %d: migrate failed: %lu [%lu - %lu)\n",
			__func__, __LINE__, pfn, get_svp_cma_basepfn(),
			get_svp_cma_basepfn() + get_svp_cma_count());
	}

	return ret;
}
#endif

unsigned long get_svp_cma_basepfn(void)
{
	if (ssvp_cma.cma)
		return ssvp_cma.base_pfn;

	return 0;
}

unsigned long get_svp_cma_count(void)
{
	if (ssvp_cma.cma)
		return ssvp_cma.count;

	return 0;
}

int svp_is_in_range(unsigned long pfn)
{
	if (ssvp_cma.cma) {
		if (pfn >= ssvp_cma.base_pfn &&
		    pfn < ssvp_cma.end_pfn)
			return 1;
	}

	return 0;
}

int svp_region_offline(phys_addr_t *pa, unsigned long *size)
{
	struct page *page;
	int retval = 0;
	int res = 0;

	pr_alert("%s %d: svp to offline enter state: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_SVP].state);

	if (_svpregs[SSVP_SUB_SVP].state == SVP_STATE_ON) {
		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_OFFING;

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
		page = wrap_cma_pages;
#else
/*		page = dma_alloc_from_contiguous_start(cma_dev,
					_svpregs[SSVP_SUB_SVP].start, _svpregs[SSVP_SUB_SVP].count, 0);*/
		page = cma_alloc(ssvp_cma.cma,
					_svpregs[SSVP_SUB_SVP].count, 0);
		if (page) {
			svp_usage_count += _svpregs[SSVP_SUB_SVP].count;
		}
#endif

		if (page) {
			_svpregs[SSVP_SUB_SVP].page = page;
			_svpregs[SSVP_SUB_SVP].state = SVP_STATE_OFF;

			/* call secmem_enable */
			res = secmem_api_enable(
					ssvp_cma.base +
					(_svpregs[SSVP_SUB_SVP].start << PAGE_SHIFT),
					_svpregs[SSVP_SUB_SVP].count << PAGE_SHIFT);

			if (pa)
				*pa = ssvp_cma.base + (_svpregs[SSVP_SUB_SVP].start << PAGE_SHIFT);
			if (size)
				*size = _svpregs[SSVP_SUB_SVP].count << PAGE_SHIFT;

			pr_alert("%s %d: secmem_enable, res %d pa %llx, size %lu\n",
					__func__, __LINE__, res,
					ssvp_cma.base + (_svpregs[SSVP_SUB_SVP].start << PAGE_SHIFT),
					_svpregs[SSVP_SUB_SVP].count << PAGE_SHIFT);
		} else {
			_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ON;
			retval = -EAGAIN;
		}
	} else {
		retval = -EBUSY;
	}

	pr_alert("%s %d: svp to offline leave state: %d, retval: %d\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_SVP].state, retval);

	return retval;
}
EXPORT_SYMBOL(svp_region_offline);

int svp_region_online(void)
{
	int retval = 0;

	pr_alert("%s %d: svp to online enter state: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_SVP].state);

	if (_svpregs[SSVP_SUB_SVP].state == SVP_STATE_OFF) {
		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ONING_WAIT;

		_svp_online_task = kthread_create(_svp_online_kthread_func, NULL, "svp_online_kthread");
		wake_up_process(_svp_online_task);
	} else {
		retval = -EBUSY;
	}

	pr_alert("%s %d: svp to online leave state: %d, retval: %d\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_SVP].state, retval);

	return retval;
}
EXPORT_SYMBOL(svp_region_online);

/* any read request will free coherent memory, eg.
 * cat /dev/svp_region
 */
static ssize_t
svp_cma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	pr_alert("%s %d:svp state: %d ires: %d\n", __func__, __LINE__, _svpregs[SSVP_SUB_SVP].state, ires);

	if (ssvp_cma.cma)
		pr_alert("%s %d: svp cma base_pfn: %ld, count %ld\n",
				__func__, __LINE__, ssvp_cma.base_pfn,
				ssvp_cma.count);

/*	if (dma_contiguous_default_area)
		pr_alert("%s %d: dma cma base_pfn: %ld, count %ld\n",
				__func__, __LINE__, dma_contiguous_default_area->base_pfn,
				dma_contiguous_default_area->count);*/

	return 0;
}

/*
 * any write request will alloc coherent memory, eg.
 * echo 0 > /dev/cma_test
 */
static ssize_t
svp_cma_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned char val;
	int retval = count;

	pr_alert("%s %d: count: %zu\n", __func__, __LINE__, count);

	if (count >= 2) {
		if (copy_from_user(&val, &buf[0], 1)) {
			retval = -EFAULT;
			goto out;
		}

		if (count == 2 && val == '0')
			svp_region_offline(NULL, NULL);
		else
			svp_region_online();
	}

out:
	return retval;
}

static long svp_cma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	pr_alert("%s %d: cmd: %x\n", __func__, __LINE__, cmd);

	switch (cmd) {
	case SVP_REGION_IOC_ONLINE: {
		svp_region_online();
		break;
	}
	case SVP_REGION_IOC_OFFLINE: {
		svp_region_offline(NULL, NULL);
		break;
	}
	default: {
		/* IONMSG("ion_ioctl : No such command!! 0x%x\n", cmd); */
		return -ENOTTY;
	}
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long svp_cma_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	pr_alert("%s %d: cmd: %x\n", __func__, __LINE__, cmd);

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	return filp->f_op->unlocked_ioctl(filp, cmd,
							(unsigned long)compat_ptr(arg));
}

#else

#define svp_cma_COMPAT_ioctl  NULL

#endif

static const struct file_operations svp_cma_fops = {
	.owner =    THIS_MODULE,
	.read  =    svp_cma_read,
	.write =    svp_cma_write,
	.unlocked_ioctl = svp_cma_ioctl,
	.compat_ioctl   = svp_cma_COMPAT_ioctl,
};

#if 0
static struct miscdevice svp_cma_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "svp_region",
	.fops = &svp_cma_fops,
};

static int __init svp_cma_init(void)
{
	int ret = 0;
	unsigned long start = 0;

	ret = misc_register(&svp_cma_misc);
	if (unlikely(ret)) {
		pr_err("failed to register svp cma misc device!\n");
		return ret;
	}
	cma_dev = svp_cma_misc.this_device;
	cma_dev->coherent_dma_mask = ~0;

	dev_set_cma_area(cma_dev, ssvp_cma);

	_dev_info(cma_dev, "registered.\n");

	proc_create("svp_region", 0, NULL, &svp_cma_fops);

	if (_SVP_MBSIZE > 0) {
		_svpregs[SSVP_SUB_SVP].count = (_SVP_MBSIZE * SZ_1M) >> PAGE_SHIFT;
		_svpregs[SSVP_SUB_SVP].start = 0;
		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ON;
		start = _svpregs[SSVP_SUB_SVP].start + _svpregs[SSVP_SUB_SVP].count;
	}
	pr_alert("%s %d: svp region start: %lu, count %lu\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_SVP].start,
			_svpregs[SSVP_SUB_SVP].count);

	proc_create("tui_region", 0, NULL, &tui_cma_fops);

	if (_TUI_MBSIZE > 0) {
		_svpregs[SSVP_SUB_TUI].count = (_TUI_MBSIZE * SZ_1M) >> PAGE_SHIFT;
		_svpregs[SSVP_SUB_TUI].start = start;
		_svpregs[SSVP_SUB_TUI].state = SVP_STATE_ON;
	}
	pr_alert("%s %d: tui region start: %lu, count %lu\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_TUI].start,
			_svpregs[SSVP_SUB_TUI].count);

	return ret;
}
module_init(svp_cma_init);

static void __exit svp_cma_exit(void)
{
	misc_deregister(&svp_cma_misc);
}
module_exit(svp_cma_exit);
#endif

static int memory_ssvp_show(struct seq_file *m, void *v)
{
	phys_addr_t cma_base = cma_get_base(ssvp_cma.cma);
	phys_addr_t cma_end = cma_base + cma_get_size(ssvp_cma.cma);

/*
	mutex_lock(&memory_ssvp_mutex);

	if (cma_pages)
		seq_printf(m, "cma collected cma_pages: %p\n", cma_pages);
	else
		seq_puts(m, "cma freed NULL\n");

	mutex_unlock(&memory_ssvp_mutex);
*/

	seq_printf(m, "cma info: [%pa-%pa] (0x%lx)\n",
			&cma_base, &cma_end,
			cma_get_size(ssvp_cma.cma));

	seq_printf(m, "cma info: base %pa pfn [%lu-%lu] count %lu\n",
			&ssvp_cma.base,
			ssvp_cma.base_pfn, ssvp_cma.end_pfn,
			ssvp_cma.count);

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
	seq_printf(m, "cma info: wrap: %pa\n", &wrap_cma_pages);
#endif

	seq_printf(m, "svp region start: %lu, count %lu, state %d\n",
			_svpregs[SSVP_SUB_SVP].start,
			_svpregs[SSVP_SUB_SVP].count,
			_svpregs[SSVP_SUB_SVP].state);

	seq_printf(m, "tui region start: %lu, count %lu, state %d\n",
			_svpregs[SSVP_SUB_TUI].start,
			_svpregs[SSVP_SUB_TUI].count,
			_svpregs[SSVP_SUB_TUI].state);

	seq_printf(m, "cma usage: %lu\n", svp_usage_count);

	return 0;
}

static int memory_ssvp_open(struct inode *inode, struct file *file)
{
	return single_open(file, &memory_ssvp_show, NULL);
}

#if 0
static ssize_t memory_ssvp_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *ppos)
{
	static char state;

	if (count > 0) {
		if (get_user(state, buffer))
			return -EFAULT;
		state -= '0';
		pr_alert("%s state = %d\n", __func__, state);
		if (state) {
			/* collect cma */
			get_memory_ssvp_cma();
		} else {
			/* undo collection */
			put_memory_ssvp_cma();
		}
	}

	return count;
}
#endif

static const struct file_operations memory_ssvp_fops = {
	.open		= memory_ssvp_open,
	.read		= seq_read,
	.release	= single_release,
};

static int __init memory_ssvp_debug_init(void)
{
	int ret = 0;
	unsigned long start = 0;
	struct proc_dir_entry *procfs_entry;

	struct dentry *dentry;

	if (!ssvp_cma.cma) {
		pr_err("memory-ssvp cma is not inited\n");
		return 1;
	}

#ifdef CONFIG_MTK_MEMORY_SSVP_WRAP
	wrap_cma_pages = cma_alloc(ssvp_cma.cma, ssvp_cma.count, 0);

	if (!wrap_cma_pages) {
		pr_err("wrap_cma_pages is not inited\n");
	} else {
		svp_usage_count = ssvp_cma.count;
	}
#endif

	dentry = debugfs_create_file("memory-ssvp", S_IRUGO, NULL, NULL,
					&memory_ssvp_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs memory_ssvp file\n");

	procfs_entry = proc_create("svp_region", 0, NULL, &svp_cma_fops);

	if (!procfs_entry)
		pr_warn("Failed to create procfs svp_region file\n");

	if (_SVP_MBSIZE > 0) {
		_svpregs[SSVP_SUB_SVP].count = (_SVP_MBSIZE * SZ_1M) >> PAGE_SHIFT;
		_svpregs[SSVP_SUB_SVP].start = 0;
		_svpregs[SSVP_SUB_SVP].state = SVP_STATE_ON;
		start = _svpregs[SSVP_SUB_SVP].start + _svpregs[SSVP_SUB_SVP].count;
	}
	pr_alert("%s %d: svp region start: %lu, count %lu\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_SVP].start,
			_svpregs[SSVP_SUB_SVP].count);

	proc_create("tui_region", 0, NULL, &tui_cma_fops);

	if (!procfs_entry)
		pr_warn("Failed to create procfs tui_region file\n");

	_svpregs[SSVP_SUB_TUI].start = start;

	if (_TUI_MBSIZE > 0) {
		_svpregs[SSVP_SUB_TUI].count = (_TUI_MBSIZE * SZ_1M) >> PAGE_SHIFT;
		_svpregs[SSVP_SUB_TUI].state = SVP_STATE_ON;
	}
	pr_alert("%s %d: tui region start: %lu, count %lu\n",
			__func__, __LINE__, _svpregs[SSVP_SUB_TUI].start,
			_svpregs[SSVP_SUB_TUI].count);

	return ret;
}

late_initcall(memory_ssvp_debug_init);
