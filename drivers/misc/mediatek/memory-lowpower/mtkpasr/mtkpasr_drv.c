#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/printk.h>

/* Memory lowpower private header file */
#include "../internal.h"

/* PASR private header file */
#include "mtkpasr_drv.h"

/* Print wrapper */
#define MTKPASR_PRINT(args...)	do {} while (0) /* pr_alert(args) */

static struct mtkpasr_bank *mtkpasr_banks;
static int num_banks;
static int mtkpasr_enable = 1;
unsigned long bank_pfns;

/* Internal control parameters */
static unsigned long mtkpasr_triggered, mtkpasr_on, mtkpasr_srmask;

/* Count the number of free pages */
static void count_free_pages(unsigned long *spfn, unsigned long *epfn)
{
	int sbank;
	unsigned long max_spfn, min_epfn;

	MTKPASR_PRINT("%s: spfn[%lu] epfn[%lu] -\n", __func__, *spfn, *epfn);
	sbank = (*spfn - mtkpasr_banks[0].start_pfn) / bank_pfns;
	for (; sbank < num_banks; sbank++) {
		max_spfn = max(mtkpasr_banks[sbank].start_pfn, *spfn);
		min_epfn = min(mtkpasr_banks[sbank].end_pfn, *epfn);
		if (min_epfn <= max_spfn)
			break;
		mtkpasr_banks[sbank].free += (min_epfn - max_spfn);
		MTKPASR_PRINT("@@@ bank[%d] free[%lu]\n", sbank, mtkpasr_banks[sbank].free);
	}
	MTKPASR_PRINT("\n");
}

/*
 * config - Identify banks/ranks, trigger APMCU flow
 * (Suppose mtkpasr_banks[].start_pfn <= mtkpasr_banks[].end_pfn)
 */
static int mtkpasr_config(int times, get_range_t func)
{
	unsigned long spfn, epfn;
	int which, i;

	/* Not enable */
	if (!mtkpasr_enable)
		return 1;

	/* Sanity check */
	if (func == NULL)
		return 1;

	MTKPASR_PRINT("%s:+\n", __func__);

	/* Reset the number of free pages to 0 */
	for (i = 0; i < num_banks; i++)
		mtkpasr_banks[i].free = 0;

	/* Find PASR-imposed range */
	for (which = 0; which < times; which++) {
		/* Query memory lowpower range */
		func(which, &spfn, &epfn);
		count_free_pages(&spfn, &epfn);
	}

	/* Find valid PASR segment */
	mtkpasr_on = 0x0;
	for (i = 0; i < num_banks; i++)
		if (mtkpasr_banks[i].free == bank_pfns)
			mtkpasr_on |= (1 << mtkpasr_banks[i].segment);

	/* APMCU flow */
	MTKPASR_PRINT("%s: PASR[0x%lx]\n", __func__, mtkpasr_on);

	++mtkpasr_triggered;

	MTKPASR_PRINT("%s:-\n", __func__);

	return 0;
}

/*
 * restore - Trigger APMCU flow for reset
 */
static int mtkpasr_restore(void)
{
	/* Not enable */
	if (!mtkpasr_enable)
		return 0;

	MTKPASR_PRINT("%s:+\n", __func__);

	mtkpasr_on = 0x0;
	/* APMCU flow */

	MTKPASR_PRINT("%s:-\n", __func__);

	return 0;
}

static struct memory_lowpower_operation mtkpasr_handler = {
	.level = MLP_LEVEL_PASR,
	.config = mtkpasr_config,
	.restore = mtkpasr_restore,
};

/* ++ SYSFS Interface ++ */
int mtkpasr_show_banks(char *buf)
{
	int i, len = 0, tmp;

	/* Show banks */
	for (i = 0; i < num_banks; i++) {
		tmp = sprintf(buf, "Bank[%d] - start_pfn[0x%lx] end_pfn[0x%lx] segment[%d] rank[%d]\n",
				i, mtkpasr_banks[i].start_pfn, mtkpasr_banks[i].end_pfn - 1,
				mtkpasr_banks[i].segment, mtkpasr_banks[i].rank);
		buf += tmp;
		len += tmp;
	}

	return len;
}

static ssize_t membank_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return mtkpasr_show_banks(buf);
}

static ssize_t enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", mtkpasr_enable);
}

/* 0: disable, 1: enable */
static ssize_t enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	int value;

	ret = kstrtoint(buf, 10, &value);
	if (ret)
		return ret;

	mtkpasr_enable = (value & 0x1) ? 1 : 0;

	return len;
}

static ssize_t mtkpasr_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Triggered [%lu]times :: Last PASR[0x%lx]\n", mtkpasr_triggered, mtkpasr_on);
}

static ssize_t srmask_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", mtkpasr_srmask);
}

static ssize_t srmask_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	int value;

	ret = kstrtoint(buf, 10, &value);
	if (ret)
		return ret;

	mtkpasr_srmask = value & 0xFFFF;
	return len;
}

/* Show overall executing status */
static ssize_t execstate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0, tmp;

	/* Enable status */
	tmp = sprintf(buf, "%d\n", mtkpasr_enable);
	buf += tmp;
	len += tmp;

	/* Bank information */
	tmp = mtkpasr_show_banks(buf);
	buf += tmp;
	len += tmp;

	/* MTKPASR status */
	tmp = sprintf(buf, "Triggered [%lu]times :: Last PASR[0x%lx]\n", mtkpasr_triggered, mtkpasr_on);
	buf += tmp;
	len += tmp;

	/* SR mask */
	tmp = sprintf(buf, "%lu\n", mtkpasr_srmask);
	buf += tmp;
	len += tmp;

	return len;
}

static DEVICE_ATTR(membank, S_IRUGO, membank_show, NULL);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(mtkpasr_status, S_IRUGO, mtkpasr_status_show, NULL);
static DEVICE_ATTR(srmask, S_IRUGO | S_IWUSR, srmask_show, srmask_store);
static DEVICE_ATTR(execstate, S_IRUGO, execstate_show, NULL);

static struct attribute *mtkpasr_attrs[] = {
	&dev_attr_membank.attr,
	&dev_attr_enable.attr,
	&dev_attr_mtkpasr_status.attr,
	&dev_attr_srmask.attr,
	&dev_attr_execstate.attr,
	NULL,
};

struct attribute_group mtkpasr_attr_group = {
	.attrs = mtkpasr_attrs,
	.name = "mtkpasr",
};

/* -- SYSFS Interface -- */

static int __init mtkpasr_construct_bankrank(void)
{
	int ret = 0, i, segn;
	unsigned long start_pfn, end_pfn;

	/* Init mtkpasr range */
	start_pfn = memory_lowpower_cma_base() >> PAGE_SHIFT;
	end_pfn = start_pfn + (memory_lowpower_cma_size() >> PAGE_SHIFT);
	bank_pfns = 0;
	ret = mtkpasr_init_range(start_pfn, end_pfn);
	if (ret <= 0 || bank_pfns == 0) {
		MTKPASR_PRINT("%s: failed to init mtkpasr range ret[%d] bank_pfns[%lu]\n", __func__, ret, bank_pfns);
		return -1;
	}

	/* Allocate buffer for banks */
	num_banks = ret;
	mtkpasr_banks = kcalloc(num_banks, sizeof(struct mtkpasr_bank), GFP_KERNEL);
	if (!mtkpasr_banks) {
		MTKPASR_PRINT("%s: failed to allocate mtkpasr_banks\n", __func__);
		return -1;
	}

	/* Query bank, rank information */
	for (i = 0; ; i++) {
		ret = query_bank_rank_information(i, &start_pfn, &end_pfn, &segn);

		/* No valid bank, just break */
		if (ret < 0)
			break;

		/* Set mtkpasr_banks */
		mtkpasr_banks[i].start_pfn = start_pfn;
		mtkpasr_banks[i].end_pfn = end_pfn;
		mtkpasr_banks[i].segment = segn;
		mtkpasr_banks[i].rank = ret - 1;	/* Minus 1 to indicate which rank */
	}

	/* Aligned allocation is preferred */
	if (i != 0)
		set_memory_lowpower_aligned(
				get_order((mtkpasr_banks[0].end_pfn - mtkpasr_banks[0].start_pfn) << PAGE_SHIFT));

	/* Sanity check */
	if (i != num_banks)
		MTKPASR_PRINT("%s a[%d] != b[%d]\n", __func__, i, num_banks);

	return 0;
}

static int __init mtkpasr_init(void)
{
	MTKPASR_PRINT("%s ++\n", __func__);

	/* Create SYSFS interface */
	if (sysfs_create_group(power_kobj, &mtkpasr_attr_group))
		goto out;

	/* Construct PASR ranks/banks */
	if (mtkpasr_construct_bankrank())
		goto out;

	/* Register feature operations */
	register_memory_lowpower_operation(&mtkpasr_handler);
out:
	MTKPASR_PRINT("%s --\n", __func__);
	return 0;
}

static void __exit mtkpasr_exit(void)
{
	unregister_memory_lowpower_operation(&mtkpasr_handler);

	/* Release mtkpasr_banks */
	if (mtkpasr_banks != NULL) {
		kfree(mtkpasr_banks);
		mtkpasr_banks = NULL;
	}

	/* Remove SYSFS interface */
	sysfs_remove_group(power_kobj, &mtkpasr_attr_group);
}

late_initcall(mtkpasr_init);
module_exit(mtkpasr_exit);
