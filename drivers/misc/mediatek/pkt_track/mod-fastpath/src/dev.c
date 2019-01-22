#include <linux/version.h>
#include <linux/sysctl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include "tuple.h"
#include "ioctl.h"
#include "dev.h"
#include "fastpath.h"
#include "fastpath_debug.h"

static long fastpath_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FASTPATH_IOCTL_RESET:
		del_all_tuple();
		return 0;
	case FASTPATH_IOCTL_ADDBR:
		{
#if 0
			char *tmp = NULL;

			tmp = (char *)arg;
			return fastpath_brctl(1, tmp);
#else
			return 0;
#endif
		}
	case FASTPATH_IOCTL_DELBR:
		{
#if 0
			char *tmp = NULL;

			tmp = (char *)arg;
			return fastpath_brctl(0, tmp);
#else
			return 0;
#endif
		}
	default:
		return -ENOTTY;
	}
}

static const struct file_operations fastpath_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = fastpath_ioctl,
};

static struct cdev *fastpath_dev;

static struct ctl_table_header *sysctl_header;

#ifdef FASTPATH_NO_KERNEL_SUPPORT
static int fastpath_maximum_track_number = MAX_TRACK_NUM;
#endif

static struct ctl_table fastpath_table[] = {
	{
	 .procname = "fastpath_max_nat",
	 .data = &fastpath_max_nat,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_nat",
	 .data = &fastpath_nat,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_max_bridge",
	 .data = &fastpath_max_bridge,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_bridge",
	 .data = &fastpath_bridge,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_max_router",
	 .data = &fastpath_max_router,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_router",
	 .data = &fastpath_router,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_max_ipsec",
	 .data = &fastpath_max_ipsec,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_ipsec",
	 .data = &fastpath_ipsec,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit",
	 .data = &fastpath_limit,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit_syn",
	 .data = &fastpath_limit_syn,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit_udp",
	 .data = &fastpath_limit_udp,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit_icmp",
	 .data = &fastpath_limit_icmp,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit_icmp_unreach",
	 .data = &fastpath_limit_icmp_unreach,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_limit_size",
	 .data = &fastpath_limit_size,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_contentfilter",
	 .data = &fastpath_contentfilter,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_ackagg_thres",
	 .data = &fastpath_ackagg_thres,
	 .maxlen = sizeof(int),
	 .mode = 0600,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_ackagg_count",
	 .data = &fastpath_ackagg_count,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_ackagg_enable",
	 .data = &fastpath_ackagg_enable,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	{
	 .procname = "fastpath_maximum_track_number",
	 .data = &fastpath_maximum_track_number,
	 .maxlen = sizeof(int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "fastpath_tracked_number",
	 .data = &fp_track.tracked_number,
	 .maxlen = sizeof(unsigned int),
	 .mode = 0400,
	 .proc_handler = &proc_dointvec},
#endif
	{
	 }
};

static struct ctl_table mtk_root_table[] = {
	{
	 .procname = "mtk",
	 .mode = 0555,
	 .child = fastpath_table},
	{
	 }
};

int init_fastpath_dev(void)
{
	dev_t dev;

	/* register sysctl table */

	sysctl_header = register_sysctl_table(mtk_root_table);
	if (!sysctl_header)
		goto out;

	/* register character dev */
	dev = MKDEV(FASTPATH_MAJOR, FASTPATH_MINOR);
	if (register_chrdev_region(dev, 1, "fastpath") < 0) {
		fp_printk(K_ERR, "fastpath: can't get major %d\n", FASTPATH_MAJOR);
		goto unregister1;
	}

	fastpath_dev = cdev_alloc();
	fastpath_dev->owner = THIS_MODULE;
	fastpath_dev->ops = &fastpath_fops;
	if (cdev_add(fastpath_dev, dev, 1)) {
		fp_printk(K_ERR, "fastpath: can't add fastpath\n");
		goto unregister2;
	}
	return 0;
unregister2:
	unregister_chrdev_region(dev, 1);
unregister1:
	unregister_sysctl_table(sysctl_header);
out:
	return -1;
}

int dest_fastpath_dev(void)
{
	unregister_sysctl_table(sysctl_header);
	cdev_del(fastpath_dev);
	return 0;
}

/*------------------------------------------------------------------------*/
/* MD Direct Tethering only supports some specified network devices,      */
/* which are defined below                                                */
/*------------------------------------------------------------------------*/
const char *fastpath_support_dev_names[] = {
	"ccmni-lan",
	"ccmni0",
	"ccmni1",
	"ccmni2",
	"ccmni3",
	"ccmni4",
	"ccmni5",
	"ccmni6",
	"ccmni7",
	"rndis0"
};

const char *fastpath_data_usage_support_dev_names[] = {
	"ccmni0",
	"ccmni1",
	"ccmni2",
	"ccmni3",
	"ccmni4",
	"ccmni5",
	"ccmni6",
	"ccmni7"
};

const int fastpath_support_dev_num =
	sizeof(fastpath_support_dev_names) /
	sizeof(fastpath_support_dev_names[0]);
const int fastpath_data_usage_support_dev_num =
	sizeof(fastpath_data_usage_support_dev_names) /
	sizeof(fastpath_data_usage_support_dev_names[0]);
const int fastpath_max_lan_dev_id;

bool fastpath_is_support_dev(const char *dev_name)
{
	int i;

	for (i = 0; i < fastpath_support_dev_num; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return true;
		}
	}

	return false;
}

int fastpath_dev_name_to_id(char *dev_name)
{
	int i;

	for (i = 0; i < fastpath_support_dev_num; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return i;
		}
	}

	return -1;
}

const char *fastpath_id_to_dev_name(int id)
{
	if (id < 0 || id >= fastpath_support_dev_num) {
		fp_printk(K_ALET, "%s: Invalid ID[%d].\n", __func__, id);
		WARN_ON(1);
		return NULL;
	}

	return fastpath_support_dev_names[id];
}

const char *fastpath_data_usage_id_to_dev_name(int id)
{
	if (id < 0 || id >= fastpath_data_usage_support_dev_num) {
		fp_printk(K_ALET, "%s: Invalid ID[%d].\n", __func__, id);
		WARN_ON(1);
		return NULL;
	}

	return fastpath_data_usage_support_dev_names[id];
}

bool fastpath_is_lan_dev(char *dev_name)
{
	int i;

	for (i = 0; i <= fastpath_max_lan_dev_id; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return true;
		}
	}

	return false;
}
