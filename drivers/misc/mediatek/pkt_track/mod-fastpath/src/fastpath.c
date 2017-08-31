/**
 *	@author mtk01984
 */
#include <linux/module.h>

/*#define MEASUREMENT*/

/* TODO: Peter DEBUG */
/* #if defined _UNIT_fastpath_main */
#if 0
#include <base/utest.h>

static exc_t t1(void *p)
{
	UTST(1);
	CERR_Ok();
}

static int fastpath_init(void)
{
	UTST_Add(t1);
	UTST_Run(0);
	return 0;
}

static void fastpath_dest(void)
{
	fp_printk(K_ERR, "unit, clean mod-fastpath\n");
}
module_init(fastpath_init)
module_exit(fastpath_dest)
#endif
#include <linux/kernel_stat.h>
#include <linux/proc_fs.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"
#include "tuple.h"
/*#include "hw.h"*/
#include "dev.h"
#include "hw.c"
#include "pkt_track.h"
#include "pkt_track_struct.h"
#include "fastpath_debug.h"
#define MAX_IFACE_NUM 32
#define DYN_IFACE_OFFSET 16
#define IPC_HDR_IS_V4(_ip_hdr) \
	(0x40 == (*((unsigned char *)(_ip_hdr)) & 0xf0))
#define IPC_HDR_IS_V6(_ip_hdr) \
	(0x60 == (*((unsigned char *)(_ip_hdr)) & 0xf0))
#define IPC_NE_GET_2B(_buf) \
	((((u16)*((u8 *)(_buf) + 0)) << 8) | \
	  (((u16)*((u8 *)(_buf) + 1)) << 0))

/* set fastpath log level */
u32 fp_log_level = K_ALET | K_CRIT | K_ERR | K_WARNIN | K_NOTICE;

module_param(fp_log_level, int, 0644);
MODULE_PARM_DESC(fp_log_level, "Debug Print Log Lvl");

/*extern unsigned long timeout;*/
int fastpath_max_nat = MD_DIRECT_TETHERING_RULE_NUM;
int fastpath_max_bridge = MD_DIRECT_TETHERING_RULE_NUM;
int fastpath_max_router = MD_DIRECT_TETHERING_RULE_NUM;
int fastpath_max_ipsec = MD_DIRECT_TETHERING_RULE_NUM;
int fastpath_max_total = (MD_DIRECT_TETHERING_RULE_NUM * 4);

int fastpath_contentfilter;

struct interface ifaces[MAX_IFACE_NUM];

struct kmem_cache *nat_tuple_cache;
struct kmem_cache *bridge_tuple_cache;
struct kmem_cache *router_tuple_cache;
struct kmem_cache *ipsec_tuple_cache;

spinlock_t fp_lock;
#ifdef CONFIG_PREEMPT_RT_FULL
raw_spinlock_t tuple_lock;
#else
spinlock_t tuple_lock;
#endif

#ifdef FASTPATH_NO_KERNEL_SUPPORT
/* AP & MD Rule id mapping API */
void *md_rule_id_mapping_table[MD_DIRECT_TETHERING_RULE_NUM * 2];

static void init_md_rule_id_mapping_table(void)
{
	memset(md_rule_id_mapping_table, 0, sizeof(md_rule_id_mapping_table));
}

void *get_tuple_by_md_rule_id(unsigned int id)
{
	if (id >= MD_DIRECT_TETHERING_RULE_NUM * 2) {
		WARN_ON(1);
		return NULL;
	}
	return md_rule_id_mapping_table[id];
}
EXPORT_SYMBOL(get_tuple_by_md_rule_id);


int get_md_rule_id_by_tuple(void *p)
{
	int i;

	if (!p)
		return -1;

	for (i = 0; i < MD_DIRECT_TETHERING_RULE_NUM * 2; i++) {
		if (p == md_rule_id_mapping_table[i])
			return i;
	}

	return -1;
}
EXPORT_SYMBOL(get_md_rule_id_by_tuple);


struct fp_track_t fp_track;
void dummy_destructor_track_table(struct sk_buff *skb)
{
	(void)skb;
}

static inline struct fp_cb *_get_cb_from_track_table(unsigned int index)
{
	return &fp_track.table[index].cb;
}

static inline struct fp_cb *_insert_track_table(struct sk_buff *skb, unsigned int counter,
					 unsigned int index)
{

	/* TODO: Peter: add nf_conntrack */
	fp_track.table[index].ref_count = 0;
	fp_track.table[index].valid = 1;
	fp_track.table[index].tracked_address = (void *)skb;
	fp_track.table[index].timestamp = counter;
	fp_track.table[index].jiffies = jiffies;
	fp_track.tracked_number++;

	return _get_cb_from_track_table(index);
}

static inline void _remove_track_table(unsigned int index)
{
	struct fp_track_table_t *p_table = &fp_track.table[index];

	fp_printk(K_DEBUG, "%s: Remove index[%d] entry from fp track table skb[%p], counter[%d].\n", __func__,
		 index, p_table->tracked_address, p_table->timestamp);

	p_table->valid = 0;
	fp_track.tracked_number--;
}

static void init_track_table(void)
{
	int i;

	TRACK_TABLE_INIT_LOCK(fp_track);
	fp_track.g_timestamp = 0;
	fp_track.tracked_number = 0;
	fp_track.table = vmalloc(sizeof(struct fp_track_table_t) * MAX_TRACK_NUM);

	for (i = 0; i < MAX_TRACK_NUM; i++)
		memset(&fp_track.table[i], 0, sizeof(struct fp_track_table_t));
}

static void dest_track_table(void)
{
	vfree(fp_track.table);
}

static struct fp_cb *add_track_table(struct sk_buff *skb, struct fp_desc *desc)
{
	struct fp_cb *cb = NULL;
	struct ip4header *ip;
	struct ip6header *ip6;
	u_int8_t proto;
	unsigned short sport = 0, dport = 0;
	unsigned int hash;
	unsigned long flags;
	int i;

	if (!skb || !desc) {
		fp_printk(K_INFO, "%s: Null input, skb[%p], desc[%p].\n", __func__, skb, desc);
		goto out;
	}

	/* get hash value */
	if (desc->flag & DESC_FLAG_IPV4) {
		ip = (struct ip4header *) (skb->data + desc->l3_off);
		proto = ip->ip_p;

		switch (proto) {
		case IPPROTO_TCP:
			{
				struct tcpheader *tcp = (struct tcpheader *) (skb->data + desc->l4_off);

				sport = tcp->th_sport;
				dport = tcp->th_dport;
			}
			break;
		case IPPROTO_UDP:
			{
				struct udpheader *udp = (struct udpheader *) (skb->data + desc->l4_off);

				sport = udp->uh_sport;
				dport = udp->uh_dport;
			}
			break;
		}
		hash =
			jhash_3words(ip->ip_src, ip->ip_dst, (u_int32_t) sport | (dport << 16),
				 proto) % MAX_TRACK_NUM;

	fp_printk(K_DEBUG, "%s: IPv4 add track table, skb[%p], src(0x%x), dst(0x%x), sport[%d], dport[%d], proto[%d], hash[%d], valid[%d].\n",
				__func__, skb, ip->ip_src, ip->ip_dst, sport, dport,
				proto, hash, fp_track.table[hash].valid);

	} else if (desc->flag & DESC_FLAG_IPV6) {
		ip6 = (struct ip6header *) (skb->data + desc->l3_off);
		/* TODO: */
		proto = ip6->nexthdr;

		switch (proto) {
		case IPPROTO_TCP:
			{
				struct tcpheader *tcp = (struct tcpheader *) (skb->data + desc->l4_off);

				sport = tcp->th_sport;
				dport = tcp->th_dport;
			}
			break;
		case IPPROTO_UDP:
			{
				struct udpheader *udp = (struct udpheader *) (skb->data + desc->l4_off);

				sport = udp->uh_sport;
				dport = udp->uh_dport;
			}
			break;
		}
		hash =
			jhash_3words(ip6->saddr.s6_addr32[0] ^ ip6->saddr.s6_addr32[1] ^ ip6->saddr.
				 s6_addr32[2] ^ ip6->saddr.s6_addr32[3],
				 ip6->daddr.s6_addr32[0] ^ ip6->daddr.s6_addr32[1] ^ ip6->daddr.
				 s6_addr32[2] ^ ip6->daddr.s6_addr32[3],
				 (u_int32_t) sport | (dport << 16), proto) % MAX_TRACK_NUM;

		fp_printk(K_DEBUG, "%s: IPv6 add track table, skb[%p], src[%x, %x, %x, %x], dst[%x, %x, %x, %x], sport[%d], dport[%d], proto[%d], hash[%d], valid[%d].\n",
					__func__, skb, ip6->saddr.s6_addr32[0], ip6->saddr.s6_addr32[1],
					ip6->saddr.s6_addr32[2], ip6->saddr.s6_addr32[3], ip6->daddr.s6_addr32[0],
					ip6->daddr.s6_addr32[1], ip6->daddr.s6_addr32[2], ip6->daddr.s6_addr32[3],
					sport, dport, proto, hash, fp_track.table[hash].valid);

	} else {
		/* TODO */
		fp_printk(K_ERR, "%s: Not a IPv4/v6 packet, flag[%x], skb[%p].\n", __func__, desc->flag, skb);
		goto out;
	}

	TRACK_TABLE_LOCK(fp_track, flags);
	/* if there is no empty entry, return */
	if (!fp_track.table[hash].valid) {
		unsigned int counter_current;
		/* update global counter */
		counter_current = fp_track.g_timestamp;
		fp_track.g_timestamp++;
		if (fp_track.g_timestamp < counter_current) {
			/* overflow, need to check if there is any duplicated entry in track table */
			counter_current = fp_track.g_timestamp;
			fp_track.g_timestamp++;
			for (i = 0; i < MAX_TRACK_NUM; i++) {
				if (!fp_track.table[i].valid)
					continue;

				if (fp_track.table[i].tracked_address == (void *)skb) {
					fp_printk(K_DEBUG, "%s: skb[%p] is duplicated, remove entry, index[%d].\n",
								__func__, skb, i);
					_remove_track_table(i);
				}

			}
		}

		/* insert current skb into track table */
		cb = _insert_track_table(skb, counter_current, hash);
	}

	TRACK_TABLE_UNLOCK(fp_track, flags);

	fp_printk(K_DEBUG, "%s: After add track table, skb[%p], cb[%p].\n", __func__, skb, cb);
out:
	return cb;
}

static struct fp_cb *search_and_hold_track_table(struct sk_buff *skb)
{
	struct fp_cb *cb = NULL;
	unsigned int timestamp = 0;
	int index = -1;
	int i;
	unsigned long flags;

	if (likely(skb->destructor == dummy_destructor_track_table)) {
		fp_printk(K_ERR, "%s: Dummy destructor track table, skb[%p].\n", __func__, skb);
		return cb;
	}

	TRACK_TABLE_LOCK(fp_track, flags);
	for (i = 0; i < MAX_TRACK_NUM; i++) {
		if (likely(!fp_track.table[i].valid))
			continue;

		if (unlikely(fp_track.table[i].tracked_address == (void *)skb)) {
			if (unlikely(fp_track.table[i].timestamp >= timestamp)) {
				timestamp = fp_track.table[i].timestamp;

				if (likely(index != -1)) {
					fp_printk(K_DEBUG, "%s: skb[%p] is duplicated, remove entry, index[%d].\n",
								__func__, skb, i);
					_remove_track_table(index);
				}

				index = i;
			} else {
				fp_printk(K_DEBUG, "%s: Not the same skb[%p], remove entry, index[%d].\n",
							__func__, skb, i);
				_remove_track_table(i);
			}
		} else {
			if (unlikely
			(time_after(jiffies, (fp_track.table[i].jiffies + 100))
			 && (fp_track.table[i].ref_count == 0))) {
				fp_printk(K_DEBUG, "%s: skb[%p] is timeout, remove entry, index[%d].\n",
							__func__, skb, i);
				_remove_track_table(i);
			}
		}
	}
	if (unlikely(index != -1)) {
		if (fp_track.table[index].ref_count == 0) {
			fp_track.table[index].ref_count = 1;
			cb = _get_cb_from_track_table(index);
			cb->index = index;
		}
	} else {
		fp_printk(K_DEBUG, "%s: Cannot find cb, skb[%p].\n", __func__, skb);
	}
	TRACK_TABLE_UNLOCK(fp_track, flags);

	return cb;
}

static void put_track_table(struct sk_buff *skb)
{
	int i;
	int count = 0;
	unsigned long flags;

	TRACK_TABLE_LOCK(fp_track, flags);
	for (i = 0; i < MAX_TRACK_NUM; i++) {
		if (!fp_track.table[i].valid)
			continue;

		if (fp_track.table[i].tracked_address == (void *)skb) {
			fp_track.table[i].ref_count = 0;
			_remove_track_table(i);
			count++;
		}
	}
	WARN_ON(count > 1 || !count);
	TRACK_TABLE_UNLOCK(fp_track, flags);
}

void del_all_track_table(void)
{
	int i;
	unsigned long flags;

	TRACK_TABLE_LOCK(fp_track, flags);
	for (i = 0; i < MAX_TRACK_NUM; i++) {
		if (!fp_track.table[i].valid)
			continue;

		_remove_track_table(i);
	}
	TRACK_TABLE_UNLOCK(fp_track, flags);
}

#endif

#ifdef ENABLE_PORTBASE_QOS
static struct proc_dir_entry *dir;

static int fastpath_dscp_marking_read(char *page, char **start,
					  off_t off, int count, int *eof, void *data)
{
	unsigned flag;
	unsigned int i, len = 0;

	FP_LOCK(&fp_lock, flag);
	for (i = 0; i < MAX_IFACE_NUM; i++) {
		if (ifaces[i].ready == 0 || ifaces[i].dev == NULL)
			continue;

		len += sprintf(page + len, "%32s\t%d\n", ifaces[i].dev->name, ifaces[i].dscp_mark);
	}
	FP_UNLOCK(&fp_lock, flag);

	*eof = 1;
	return len;
}

static int fastpath_dscp_marking_write(struct file *file,
					   const char *buff, unsigned long count, void *data)
{
	unsigned flag;
	unsigned int i, j;
	char line[128], *args[2], *tmp;

	if (count > 128)
		return count;

	copy_from_user(line, buff, count);

	tmp = line;
	memset(args, 0, 8);
	for (i = 0, j = 0; i < count; i++) {
		if (line[i] == ' ' || line[i] == '\n' || line[i] == '\0') {
			line[i] = '\0';
			args[j++] = tmp;
			if (j == 2)
				break;
			if (i == count - 1)
				break;
			tmp = &line[i + 1];
		}
	}

	if (args[0] == NULL || args[1] == NULL)
		return count;

	FP_LOCK(&fp_lock, flag);
	for (i = 0; i < MAX_IFACE_NUM; i++) {
		if (ifaces[i].ready != 0 &&
			ifaces[i].dev != NULL && strcmp(ifaces[i].dev->name, args[0]) == 0) {
			ifaces[i].dscp_mark = kstrtol(args[1], NULL, 10);
			break;
		}
	}
	FP_UNLOCK(&fp_lock, flag);

	return count;
}
#endif

#ifdef FASTPATH_NETFILTER
#define FP_DEVICE_DYNAMIC_REGISTERING_PROC_ENTRY "driver/fastpath_dynamic_registered_device"
#define DEVICE_REGISTERING_HASH_SIZE (32)
static spinlock_t device_registering_lock;
static struct list_head *device_registering_hash;
struct device_registering {
	struct list_head list;
	struct net_device *dev;
};
static int fastpath_device_dynamic_registering_proc_show(struct seq_file *m, void *v)
{
	unsigned long flags;
	struct device_registering *found_device;
	int i;

	spin_lock_irqsave(&device_registering_lock, flags);
	for (i = 0; i < DEVICE_REGISTERING_HASH_SIZE; i++) {
		list_for_each_entry(found_device, &device_registering_hash[i], list) {
			seq_printf(m, "%s\n", found_device->dev->name);
		}
	}
	spin_unlock_irqrestore(&device_registering_lock, flags);
	return 0;
}

static int fastpath_device_dynamic_registering_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fastpath_device_dynamic_registering_proc_show, NULL);
}

static const struct file_operations fastpath_device_dynamic_registering_proc_fops = {
	.owner = THIS_MODULE,
	.open = fastpath_device_dynamic_registering_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static inline unsigned int fastpath_get_hash_by_device(struct net_device *dev)
{
	return jhash_1word((uintptr_t) dev, 0) % DEVICE_REGISTERING_HASH_SIZE;
}

static inline void fastpath_release_device(struct device_registering *device)
{
	list_del(&device->list);
	dev_put(device->dev);
	device->dev = NULL;
	kfree(device);
}

/* TODO: Peter DEBUG */
#if 0
static int fastpath_is_device_dynamic_registered(struct net_device *dev)
{
	int ret = 0;
	unsigned long flags;
	struct device_registering *found_device;
	unsigned int hash = fastpath_get_hash_by_device(dev);

	spin_lock_irqsave(&device_registering_lock, flags);
	list_for_each_entry(found_device, &device_registering_hash[hash], list) {
		if (found_device->dev == dev) {
			ret = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&device_registering_lock, flags);

	return ret;
}

static void fastpath_device_dynamic_register(struct net_device *dev)
{
	unsigned long flags;
	struct device_registering *found_device;
	struct device_registering *new_device;
	unsigned int hash = fastpath_get_hash_by_device(dev);

	if (in_irq())
		new_device = kmalloc(sizeof(struct device_registering), GFP_ATOMIC);
	else
		new_device = kmalloc(sizeof(struct device_registering), GFP_KERNEL);

	if (!new_device) {

		fp_printk(K_ERR, "%s: kmalloc failed\n", __func__);
		return;
	}

	spin_lock_irqsave(&device_registering_lock, flags);

	/* if duplicated, BUG */
	list_for_each_entry(found_device, &device_registering_hash[hash], list) {
		WARN_ON(found_device->dev == dev);
	}
	new_device->dev = dev;
/* <JQ_DEBUG> */
/* dev_hold(new_device->dev); */
	list_add_tail(&new_device->list, &device_registering_hash[hash]);
	spin_unlock_irqrestore(&device_registering_lock, flags);
}

static void fastpath_device_dynamic_unregister(struct net_device *dev)
{
	unsigned long flags;
	int found = 0;
	struct device_registering *found_device;
	unsigned int hash = fastpath_get_hash_by_device(dev);

	spin_lock_irqsave(&device_registering_lock, flags);
	list_for_each_entry(found_device, &device_registering_hash[hash], list) {
		if (found_device->dev == dev) {
			fastpath_release_device(found_device);
			found = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&device_registering_lock, flags);

	WARN_ON(found == 0);
}
#endif
#endif

/* TODO: Peter DEBUG */
#if 0
static int fp_notifier_event(struct notifier_block *nb, unsigned long event, void *data)
{
	unsigned long flag;
	struct net_device *dev = data;

	FP_LOCK(&fp_lock, flag);
	switch (event) {
#ifdef FASTPATH_NETFILTER
	case NETDEV_REGISTER:
		fp_printk(K_INFO, "%s: NETDEV_REGISTER: %s\n", __func__, dev->name);
		if (!fastpath_is_device_dynamic_registered(dev))
			fastpath_device_dynamic_register(dev);

		break;
	case NETDEV_UNREGISTER:
		fp_printk(K_INFO, "%s: NETDEV_UNREGISTER: %s\n", __func__, dev->name);
		if (fastpath_is_device_dynamic_registered(dev))
			fastpath_device_dynamic_unregister(dev);

		break;
#endif
	default:
		fp_printk(K_INFO, "%s: NETDEV_DEFAULT: %s\n", __func__, dev->name);
		break;
	}
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	del_all_track_table();
#endif
	del_all_tuple();
	FP_UNLOCK(&fp_lock, flag);

	return NOTIFY_DONE;
}

static struct notifier_block fp_notifier_block __read_mostly = {
	.notifier_call = fp_notifier_event,
};
#endif

static int fp_notifier_init(void)
{
	/* TODO: Peter DEBUG */
	int ret = 0;

#ifdef FASTPATH_NETFILTER
	int i;

	spin_lock_init(&device_registering_lock);
	device_registering_hash = vmalloc(sizeof(struct list_head) * DEVICE_REGISTERING_HASH_SIZE);

	for (i = 0; i < DEVICE_REGISTERING_HASH_SIZE; i++)
		INIT_LIST_HEAD(&device_registering_hash[i]);

	proc_create(FP_DEVICE_DYNAMIC_REGISTERING_PROC_ENTRY, 0, NULL,
			&fastpath_device_dynamic_registering_proc_fops);
#endif
	/* TODO: Peter DEBUG */
#if 0
	ret = register_netdevice_notifier(&fp_notifier_block);
#endif

	return ret;
}

static void fp_notifier_dest(void)
{
#ifdef FASTPATH_NETFILTER
	unsigned long flags;
	struct device_registering *found_device;
	int i;
#endif

	/* TODO: Peter DEBUG */
#if 0
	unregister_netdevice_notifier(&fp_notifier_block);
#endif
#ifdef FASTPATH_NETFILTER
	remove_proc_entry(FP_DEVICE_DYNAMIC_REGISTERING_PROC_ENTRY, NULL);
	/* release all registered devices */
	spin_lock_irqsave(&device_registering_lock, flags);
	for (i = 0; i < DEVICE_REGISTERING_HASH_SIZE; i++) {
		list_for_each_entry(found_device, &device_registering_hash[i], list) {
			fastpath_release_device(found_device);
		}
	}
	spin_unlock_irqrestore(&device_registering_lock, flags);
	vfree(device_registering_hash);
#endif
}

static int fastpath_init(void)
{
	/*int i; */
	int ret = 0;

	memset(ifaces, 0, sizeof(ifaces));

	timeout = jiffies;

/* #ifdef MEASUREMENT */
/*	fp_printk(K_ERR, "XXXXXXXXXXXXXXXXXXXXXXXXX %s\n", __TIME__); */
/*	__asm__ __volatile__("mcr p15, 0, %0, c9, c12, 1" :  : "r"(0x80000000)); */
/* #endif */

/* if(unlikely(!bridge_dev)){ */
/* bridge_dev=dev_get_by_name(&init_net, "br0"); */
/* if(!bridge_dev){ */
/* return -ENODEV; */
/* } */
/* } */

	FP_INIT_LOCK(&fp_lock);

	TUPLE_INIT_LOCK(&tuple_lock);

#ifdef FASTPATH_NO_KERNEL_SUPPORT
	init_track_table();
#endif

/* <JQ_TO_DELETE> */
#if 0
	ret = init_fastpath_dev();
	if (ret)
		return ret;
#endif

	init_nat_tuple();
	init_bridge_tuple();
	init_router_tuple();
	init_ipsec_tuple();

	nat_tuple_cache =
		kmem_cache_create("fastpath_nat_tuple", sizeof(struct nat_tuple), 0, SLAB_HWCACHE_ALIGN, NULL);
	bridge_tuple_cache =
		kmem_cache_create("fastpath_bridge_tuple", sizeof(struct bridge_tuple), 0, SLAB_HWCACHE_ALIGN,
				  NULL);
	router_tuple_cache =
		kmem_cache_create("fastpath_router_tuple", sizeof(struct router_tuple), 0, SLAB_HWCACHE_ALIGN,
				  NULL);
	ipsec_tuple_cache =
		kmem_cache_create("fastpath_ipsec_tuple", sizeof(struct ipsec_tuple), 0, SLAB_HWCACHE_ALIGN,
				  NULL);

#ifdef ENABLE_PORTBASE_QOS
	/* initialize /proc/fastpath/dscp_marking interface */
	{
		struct proc_dir_entry *res;

		dir = proc_mkdir("fastpath", NULL);
		if (dir) {
			res = create_proc_entry("dscp_marking", 0644, dir);
			if (res) {
				res->read_proc = fastpath_dscp_marking_read;
				res->write_proc = fastpath_dscp_marking_write;
				res->data = NULL;
			} else {
				fp_printk(K_ERR, "Can not create /proc/fastpath/dscp_marking\n");
			}
		} else {
			fp_printk(K_ERR, "Can not create /proc/fastpath\n");
		}
	}
#endif

	init_md_rule_id_mapping_table();

	ret = fp_notifier_init();
	if (ret < 0) {
		fp_printk(K_ERR, "%s: fp_notifier_init failed with ret = %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static void fastpath_dest(void)
{
	fp_notifier_dest();
	dest_fastpath_dev();
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	dest_track_table();
#endif
#ifdef ENABLE_PORTBASE_QOS
	remove_proc_entry("dscp_marking", dir);
	remove_proc_entry("fastpath", NULL);
#endif
}

static inline void _fastpath_in_tail(u_int8_t iface, struct fp_desc *desc, struct fp_cb *cb,
					 struct sk_buff *skb)
{
	struct ip4header *ip;
	struct ip6header *ip6;

	/* TODO: Peter DEBUG */
#if 0
	if (skb->dev && !fastpath_is_device_dynamic_registered(skb->dev))
		return;
#endif

	cb = add_track_table(skb, desc);
	if (!cb) {
		fp_printk(K_DEBUG, "%s: Add track table failed, skb[%p], desc[%p]!\n", __func__, skb, desc);
		return;
	}

	cb->flag = desc->flag;

	if (desc->flag & DESC_FLAG_TRACK_NAT) {

		cb->dev = skb->dev;

		ip = (struct ip4header *) (skb->data + desc->l3_off);

		cb->src = ip->ip_src;
		cb->dst = ip->ip_dst;
		switch (ip->ip_p) {
		case IPPROTO_TCP:
			{
				struct tcpheader *tcp = (struct tcpheader *) (skb->data + desc->l4_off);

				cb->sport = tcp->th_sport;
				cb->dport = tcp->th_dport;
			}
			break;
		case IPPROTO_UDP:
			{
				struct udpheader *udp = (struct udpheader *) (skb->data + desc->l4_off);

				cb->sport = udp->uh_sport;
				cb->dport = udp->uh_dport;
			}
			break;
/* case IPPROTO_GRE: */
/* { */
/* //greheader *gre = (greheader *)(skb->data + desc->l4_off); */
/* } */
/* break; */
		default:
			fp_printk(K_ALET, "BUG, %s: Should not reach here, skb[%p], protocol[%d].\n",
						__func__, skb, ip->ip_p);
			break;
		}
		cb->proto = ip->ip_p;

		return;
	} else if (desc->flag & DESC_FLAG_TRACK_ROUTER) {	/* Now only support IPv6 (20130104) */
		if (desc->flag & DESC_FLAG_IPV6) {
			ip6 = (struct ip6header *) (skb->data + desc->l3_off);

			cb->dev = skb->dev;
		} else {
			fp_printk(K_ERR, "%s: Invalid router flag[%x], skb[%p]!\n", __func__, desc->flag, skb);
		}

		/* if (desc->flag & DESC_FLAG_IPV4) */
			/* TODO: IPv4 routing */
	}
	fp_printk(K_DEBUG, "%s: Invalid flag[%x], skb[%p]!\n", __func__, desc->flag, skb);
}

static inline void _fastpath_in_nat(unsigned char *offset2, struct fp_desc *desc, struct sk_buff *skb,
					int iface)
{
	/* IPv4 */
	struct tuple t4;
	struct ip4header *ip;
	/* IPv6 */
	struct router_tuple t6;
	struct ip6header *ip6;
	/*unsigned char ip6_nexthdr; */
	__be16 ip6_frag_off;

	void *l4_header;


	if (likely(IPC_HDR_IS_V4(offset2))) {	/* support NAT and ROUTE */
		desc->flag |= DESC_FLAG_IPV4;

		ip = (struct ip4header *) offset2;

		fp_printk(K_DEBUG, "%s: IPv4 pre-routing, skb[%p], ip_id[%x], checksum[%x], protocol[%d], in_dev[%s].\n",
				__func__, skb, ip->ip_id, ip->ip_sum, ip->ip_p, skb->dev->name);

		/* ip fragmentation? */
		if (ip->ip_off & 0xff3f) {
			desc->flag |= DESC_FLAG_IPFRAG;
			return;
		}

		desc->l3_len = ip->ip_hl << 2;
		desc->l4_off = desc->l3_off + desc->l3_len;
		l4_header = skb->data + desc->l4_off;

		t4.nat.src = ip->ip_src;
		t4.nat.dst = ip->ip_dst;
		t4.nat.proto = ip->ip_p;
#ifdef FASTPATH_NETFILTER
		t4.dev_in = skb->dev;
#endif

		switch (ip->ip_p) {
		case IPPROTO_TCP:
			fp_ip4_tcp(desc, skb, &t4, &ifaces[iface], ip, l4_header);
			return;
		case IPPROTO_UDP:
			fp_ip4_udp(desc, skb, &t4, &ifaces[iface], ip, l4_header);
			return;
/* case IPPROTO_ICMP: */
/* fp_ip4_icmp_lan(desc, skb, &t4, &ifaces[iface], ip, l4_header); */
/* return; */
		default:
			desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
			return;
		}
	} else if (IPC_HDR_IS_V6(offset2)) {	/* only support ROUTE */
		desc->flag |= DESC_FLAG_IPV6;

		ip6 = (struct ip6header *) offset2;

		fp_printk(K_DEBUG, "%s: IPv6 pre-routing, skb[%p], protocol[%d].\n", __func__, skb, ip6->nexthdr);

		t6.proto = ip6->nexthdr;
		desc->l3_len =
			ipv6_skip_exthdr(skb, desc->l3_off + sizeof(struct ip6header), &t6.proto,
					 &ip6_frag_off) - desc->l3_off;
		desc->l4_off = desc->l3_off + desc->l3_len;
		l4_header = skb->data + desc->l4_off;

		/* ip fragmentation? */
		if (unlikely(ip6_frag_off > 0)) {
			desc->flag |= DESC_FLAG_IPFRAG;
			return;
		}

		t6.dev_src = skb->dev;
		ipv6_addr_copy((struct in6_addr *)(&(t6.saddr)), &(ip6->saddr));
		ipv6_addr_copy((struct in6_addr *)(&(t6.daddr)), &(ip6->daddr));

		switch (t6.proto) {
		case IPPROTO_TCP:
			fp_ip6_tcp_lan(desc, skb, &t6, &ifaces[iface], ip6, l4_header);
			return;
		case IPPROTO_UDP:
			fp_ip6_udp_lan(desc, skb, &t6, &ifaces[iface], ip6, l4_header);
			return;
/* case IPPROTO_ICMPV6: */
/* fp_ip6_icmp_lan(desc, skb, &t6, &ifaces[iface], ip6, l4_header); */
/* return; */
		default:
			{
				desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
				return;
			}
		}
	} else {
		memset(desc, 0, sizeof(*desc));	/* avoid compiler warning */
		desc->flag |= DESC_FLAG_UNKNOWN_ETYPE;
		return;
	}
}

static inline int fastpath_in_internal(int iface, struct sk_buff *skb)
{
	struct fp_cb *cb;
	/*unsigned flag; */
	struct fp_desc desc;
/* #ifdef MEASUREMENT */
/*	__asm__ __volatile__("mcr p15, 0, %0, c9, c12, 0" :  : "r"(0x00000007)); */
/*	__asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0\n":"=r"(c1)); */
/* #endif */

	pm_reset_traffic();

	if (unlikely(!fastpath_max_total)) {
		fp_printk(K_ERR, "%s: No fastpath!\n", __func__);
		return 0;	/* original path */
	}

	cb = (struct fp_cb *) (&skb->cb[48]);
	/* reset cb flag ?? */
	/* cb->flag = 0; */

	/* HW */
	desc.flag = 0;
	desc.l3_off = 0;
	/*desc.pflag = &flag; */


	skb_set_network_header(skb, desc.l3_off);

	_fastpath_in_nat(skb->data, &desc, skb, iface);
	/* SW */
/* if(likely(desc.flag & DESC_FLAG_FASTPATH)){ */
/* #ifdef MEASUREMENT */
/* fp_printk(K_ERR, "ELAPSED: %d\n", c5 - c1); */
/* #endif */
/* return 1; //fast path */
/* } */
	if (desc.flag & DESC_FLAG_NOMEM) {
		fp_printk(K_ERR, "%s: No memory, flag[%x], skb[%p].\n", __func__, desc.flag, skb);
		/* <JQ_TO_DELETE> */
		#if 0
		dev_kfree_skb_any(skb);
		#endif

		return 2;
	}
	if (desc.flag & (DESC_FLAG_UNKNOWN_ETYPE | DESC_FLAG_UNKNOWN_PROTOCOL | DESC_FLAG_IPFRAG)) {
		/* un-handled packet, so pass it to kernel stack */
		fp_printk(K_ERR, "%s: Un-handled packet, pass it to kernel stack, flag[%x], skb[%p].\n",
					__func__, desc.flag, skb);
		return 0;
	}

	_fastpath_in_tail(iface, &desc, cb, skb);

	return 0;		/* original path */
}

int fastpath_in_nf(int iface, struct sk_buff *skb)
{
	unsigned long flag;
	int ret;

	FP_LOCK(&fp_lock, flag);
	ret = fastpath_in_internal(iface, skb);
	FP_UNLOCK(&fp_lock, flag);
	return ret;
}

#ifdef FASTPATH_NETFILTER
void fastpath_out(int iface, struct sk_buff *skb)
{
}

void fastpath_out_nf_ipv4(
	int iface,
	struct sk_buff *skb,
	struct net_device *out,
	unsigned char *offset2,
	struct fp_cb *cb)
{
	struct nf_conn *nat_ip_conntrack;
	enum ip_conntrack_info ctinfo;
	struct fp_tag_packet_t *skb_tag = (struct fp_tag_packet_t *)skb->head;

	struct ip4header *ip = (struct ip4header *) offset2;

	fp_printk(K_DEBUG, "%s: IPv4 add rule, skb[%p], cb->proto[%d], ip->ip_p[%d], offset2[%p], ip_id[%x], checksum[%x].\n",
				__func__, skb, cb->proto, ip->ip_p, offset2, ip->ip_id, ip->ip_sum);

	if (cb->proto != ip->ip_p) {
		fp_printk(K_INFO, "%s: IPv4 protocol mismatch, cb->proto[%d], ip->ip_p[%d], skb[%p] is filtered out.\n",
					__func__, cb->proto, ip->ip_p, skb);
		goto out;
	}

	offset2 += (ip->ip_hl << 2);

	switch (ip->ip_p) {
	case IPPROTO_TCP:
		{
			struct tcpheader *tcp = (struct tcpheader *) offset2;

			nat_ip_conntrack = (struct nf_conn *)skb->nfct;
			ctinfo = skb->nfctinfo;
			if (!nat_ip_conntrack) {
				fp_printk(K_ERR, "%s: Null ip conntrack, skb[%p] is filtered out.\n", __func__, skb);
				goto out;
			}

			if (nat_ip_conntrack->ext && nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]) { /* helper */
				fp_printk(K_INFO, "%s: skb[%p] is filtered out, ext[%p], ext_offset[%d].\n",
						__func__, skb, nat_ip_conntrack->ext,
						nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]);
				goto out;
			}

			if (fastpath_contentfilter
					&& (tcp->th_dport == htons(80) || tcp->th_sport == htons(80))
					&& nat_ip_conntrack->mark != 0x80000000) {
				fp_printk(K_ERR, "%s: Invalid parameter, contentfilter[%d], dport[%x], sport[%x], mark[%x], skb[%p] is filtered out,.\n",
						__func__, fastpath_contentfilter, tcp->th_dport,
						tcp->th_sport, nat_ip_conntrack->mark, skb);
				goto out;
			}

			if (nat_ip_conntrack->proto.tcp.state >= TCP_CONNTRACK_FIN_WAIT
					&& nat_ip_conntrack->proto.tcp.state <=	TCP_CONNTRACK_CLOSE) {
				fp_printk(K_ERR, "%s: Invalid TCP state[%d], skb[%p] is filtered out.\n",
							__func__, nat_ip_conntrack->proto.tcp.state, skb);
				goto out;
			} else if (nat_ip_conntrack->proto.tcp.state !=	TCP_CONNTRACK_ESTABLISHED) {
				fp_printk(K_ERR, "%s: TCP state[%d] is not in ESTABLISHED state, skb[%p] is filtered out.\n",
							__func__, nat_ip_conntrack->proto.tcp.state, skb);
				goto out;
			}

			/* #if (defined(FASTPATH_NO_KERNEL_SUPPORT) && defined(FASTPATH_DISABLE_TCP_WINDOW_CHECK)) */
			/* ct = container_of(nat_ip_conntrack, struct nf_conn, ct_general); */
			/* spin_lock_bh(&ct->lock); */
			/* tcp_state = &ct->proto.tcp; */
			/* ((struct ip_ct_tcp_state *)(&tcp_state->seen[IP_CT_DIR_ORIGINAL]))->flags |= */
			/*				IP_CT_TCP_FLAG_BE_LIBERAL; */
			/* ((struct ip_ct_tcp_state *)(&tcp_state->seen[IP_CT_DIR_REPLY]))->flags |= */
			/*				IP_CT_TCP_FLAG_BE_LIBERAL; */
			/* spin_unlock_bh(&ct->lock); */
			/* #endif */

			/* Tag this packet for MD tracking */
			if (skb_headroom(skb) > sizeof(struct fp_tag_packet_t)) {
				skb_tag->guard_pattern = MDT_TAG_PATTERN;
				if (strcmp(out->name, "ccmni-lan")) { /* uplink */
					skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
					skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
					skb_tag->info.port = cb->sport;
				} else { /* downlink */
					skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
					skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
					skb_tag->info.port = cb->dport;
				}
			} else {
				fp_printk(K_ERR, "%s: Headroom of skb[%p] is not enough to add MDT tag, headroom[%d].\n",
							__func__, skb, skb_headroom(skb));
			}
		}
		break;
	case IPPROTO_UDP:
			nat_ip_conntrack = (struct nf_conn *)skb->nfct;
			ctinfo = skb->nfctinfo;
			if (!nat_ip_conntrack) {
				fp_printk(K_ERR, "%s: Null ip conntrack, skb[%p] is filtered out.\n", __func__, skb);
				goto out;
			}

			if (nat_ip_conntrack->ext && nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]) { /* helper */
				fp_printk(K_ERR, "%s: skb[%p] is filtered out, ext[%p], ext_offset[%d].\n",
						__func__, skb, nat_ip_conntrack->ext,
						nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]);
				goto out;
			}

			if (cb->sport != 67 && cb->sport != 68) {
				/* Don't fastpath dhcp packet */

				/* Tag this packet for MD tracking */
				if (skb_headroom(skb) > sizeof(struct fp_tag_packet_t)) {
					skb_tag->guard_pattern = MDT_TAG_PATTERN;
					if (strcmp(out->name, "ccmni-lan")) { /* uplink */
						skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
						skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
						skb_tag->info.port = cb->sport;
					} else { /* downlink */
						skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
						skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
						skb_tag->info.port = cb->dport;
					}
				} else {
					fp_printk(K_ERR, "%s: Headroom of skb[%p] is not enough to add MDT tag, headroom[%d].\n",
								__func__, skb, skb_headroom(skb));
				}
			} else {
				fp_printk(K_DEBUG, "%s: Don't track DHCP packet, s_port[%d], skb[%p] is filtered out.\n",
							__func__, cb->sport, skb);
			}
		break;
	default:
		fp_printk(K_DEBUG, "%s: Not TCP/UDP packet, protocal[%d], skb[%p] is filtered out.\n",
					__func__, ip->ip_p, skb);
		break;
	}

out:
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	put_track_table(skb);
#endif
}

void fastpath_out_nf_ipv6(int iface, struct sk_buff *skb, struct net_device *out, struct fp_cb *cb, int l3_off)
{
	struct ip6header *ip6 = (struct ip6header *) (skb->data + l3_off);
	unsigned char nexthdr;
	__be16 frag_off;
	int l4_off = 0;
	struct nf_conn *nat_ip_conntrack;
	enum ip_conntrack_info ctinfo;
	struct fp_tag_packet_t *skb_tag = (struct fp_tag_packet_t *)skb->head;

	nexthdr = ip6->nexthdr;
	l4_off = ipv6_skip_exthdr(skb, l3_off + sizeof(struct ip6header), &nexthdr, &frag_off);
	switch (nexthdr) {
	case IPPROTO_TCP:
		{
			struct tcpheader *tcp = (struct tcpheader *) (skb->data + l4_off);

			nat_ip_conntrack = (struct nf_conn *)skb->nfct;
			ctinfo = skb->nfctinfo;
			if (!nat_ip_conntrack) {
				fp_printk(K_ERR, "%s: Null ip conntrack, skb[%p] is filtered out.\n", __func__, skb);
				goto out;
			}

			if (nat_ip_conntrack->ext && nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]) { /* helper */
				fp_printk(K_INFO, "%s: skb[%p] is filtered out, ext[%p], ext_offset[%d].\n",
						__func__, skb, nat_ip_conntrack->ext,
						nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]);
				goto out;
			}

			if (fastpath_contentfilter
					&& (tcp->th_dport == htons(80) || tcp->th_sport == htons(80))
					&& nat_ip_conntrack->mark != 0x80000000) {
				fp_printk(K_ERR, "%s: Invalid parameter, contentfilter[%d], dport[%x], sport[%x], mark[%x], skb[%p] is filtered out,.\n",
							__func__, fastpath_contentfilter, tcp->th_dport,
							tcp->th_sport, nat_ip_conntrack->mark, skb);
				goto out;
			}
			if (nat_ip_conntrack->proto.tcp.state >= TCP_CONNTRACK_FIN_WAIT
					&& nat_ip_conntrack->proto.tcp.state <=	TCP_CONNTRACK_CLOSE) {
				/* TODO */
				fp_printk(K_ERR, "%s: Invalid TCP state[%d], skb[%p] is filtered out.\n",
							__func__, nat_ip_conntrack->proto.tcp.state, skb);
				goto out;
			} else if (nat_ip_conntrack->proto.tcp.state !=	TCP_CONNTRACK_ESTABLISHED) {
				fp_printk(K_ERR, "%s: TCP state[%d] is not in ESTABLISHED state, skb[%p] is filtered out.\n",
							__func__, nat_ip_conntrack->proto.tcp.state, skb);
				goto out;
			}
#ifndef FASTPATH_NETFILTER
			if (cb->iface == iface) {
				fp_printk(K_ERR, "BUG %s,%d: in_iface[%p] and out_iface[%p] are same.\n",
						__func__, __LINE__, cb->iface, iface);
				goto out;
			}
#else
			if (cb->dev == out) {
				fp_printk(K_ERR, "BUG %s,%d: in_dev[%p] name[%s] and out_dev[%p] name [%s] are same.\n",
						__func__, __LINE__, cb->dev, cb->dev->name, out, out->name);
				goto out;
			}
#endif

			/* #if (defined(FASTPATH_NO_KERNEL_SUPPORT) && defined(FASTPATH_DISABLE_TCP_WINDOW_CHECK)) */
			/* //ct = container_of(nat_ip_conntrack, struct nf_conn, ct_general); */
			/* ct = nat_ip_conntrack; */
			/* spin_lock_bh(&ct->lock); */
			/* tcp_state = &ct->proto.tcp; */
			/* ((struct ip_ct_tcp_state *)(&tcp_state->seen[IP_CT_DIR_ORIGINAL]))->flags |= */
			/*				IP_CT_TCP_FLAG_BE_LIBERAL; */
			/* ((struct ip_ct_tcp_state *)(&tcp_state->seen[IP_CT_DIR_REPLY]))->flags |= */
			/*				IP_CT_TCP_FLAG_BE_LIBERAL; */
			/* spin_unlock_bh(&ct->lock); */
			/* #endif */

			/* Tag this packet for MD tracking */
			if (skb_headroom(skb) > sizeof(struct fp_tag_packet_t)) {
				skb_tag->guard_pattern = MDT_TAG_PATTERN;
				if (strcmp(out->name, "ccmni-lan")) { /* uplink*/
					skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
					skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
					skb_tag->info.port = cb->sport;
				} else { /* downlink */
					skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
					skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
					skb_tag->info.port = cb->dport;
				}
			} else {
				fp_printk(K_ERR, "%s: Headroom of skb[%p] is not enough to add MDT tag, headroom[%d].\n",
							__func__, skb, skb_headroom(skb));
			}
		}
		break;
	case IPPROTO_UDP:
		{
			struct udpheader *udp = (struct udpheader *) (skb->data + l4_off);

			nat_ip_conntrack = (struct nf_conn *)skb->nfct;
			ctinfo = skb->nfctinfo;
			if (!nat_ip_conntrack) {
				fp_printk(K_ERR, "%s: Null ip conntrack, skb[%p] is filtered out.\n", __func__, skb);
				goto out;
			}

			if (nat_ip_conntrack->ext && nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]) { /* helper */
				fp_printk(K_ERR, "%s: skb[%p] is filtered out, ext[%p], ext_offset[%d].\n",
						__func__, skb, nat_ip_conntrack->ext,
						nat_ip_conntrack->ext->offset[NF_CT_EXT_HELPER]);
				goto out;
			}

#ifndef FASTPATH_NETFILTER
			if (cb->iface == iface)	{
				fp_printk(K_ERR, "BUG %s,%d: in_iface[%p] and out_iface[%p] are same.\n",
						__func__, __LINE__, cb->iface, iface);
				goto out;
			}
#else
			if (cb->dev == out)	{
				fp_printk(K_ERR, "BUG %s,%d: in_dev[%p] name[%s] and out_dev[%p] name [%s] are same.\n",
						__func__, __LINE__, cb->dev, cb->dev->name, out, out->name);
				goto out;
			}
#endif

			if (udp->uh_sport != 67 && udp->uh_sport != 68) {
				/* Don't fastpath dhcp packet */

				/* Tag this packet for MD tracking */
				if (skb_headroom(skb) > sizeof(struct fp_tag_packet_t)) {
					skb_tag->guard_pattern = MDT_TAG_PATTERN;
					if (strcmp(out->name, "ccmni-lan")) { /* uplink */
						skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
						skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
						skb_tag->info.port = cb->sport;
					} else { /* downilnk */
						skb_tag->info.in_netif_id = fastpath_dev_name_to_id(cb->dev->name);
						skb_tag->info.out_netif_id = fastpath_dev_name_to_id(out->name);
						skb_tag->info.port = cb->dport;
					}
				} else {
					fp_printk(K_ERR, "%s: Headroom of skb[%p] is not enough to add MDT tag, headroom[%d].\n",
								__func__, skb, skb_headroom(skb));
				}
			} else {
				fp_printk(K_DEBUG, "%s: Don't track DHCP packet, s_port[%d], skb[%p] is filtered out.\n",
						__func__, cb->sport, skb);
			}
		}
		break;
	default:
		fp_printk(K_DEBUG, "%s: Not TCP/UDP packet, protocal[%d], skb[%p] is filtered out.\n",
					__func__, nexthdr, skb);
		break;
	}

out:
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	put_track_table(skb);
#endif
}

void fastpath_out_nf(int iface, struct sk_buff *skb, struct net_device *out)
{
	unsigned char *offset2 = skb->data;
	struct fp_cb *cb;
	unsigned long flags;
#if (defined(FASTPATH_NO_KERNEL_SUPPORT) && defined(FASTPATH_DISABLE_TCP_WINDOW_CHECK))
/* struct nf_conn *ct = NULL; */
/* struct ip_ct_tcp *tcp_state = NULL; */
#endif

	fp_printk(K_DEBUG, "%s: post-routing, add rule, skb[%p].\n", __func__, skb);

	pm_reset_traffic();

	if (!fastpath_max_nat && !fastpath_max_bridge && !fastpath_max_router && !fastpath_max_ipsec) {
		/* do not investigate into skb */
		fp_printk(K_ERR, "%s: No fastpath, skb[%p].\n", __func__, skb);
		return;
	}

#ifdef FASTPATH_NETFILTER
	/* TODO: Peter DEBUG */
#if 0
	if (skb->dev && !fastpath_is_device_dynamic_registered(skb->dev))
		return;
#endif
#endif

	cb = search_and_hold_track_table(skb);
	if (!cb) {
		fp_printk(K_DEBUG, "%s: Cannot find cb, skb[%p].\n", __func__, skb);
		return;
	}

	if (cb->dev == out) {
		fp_printk(K_INFO, "%s: in_dev[%p] name[%s] and out_dev[%p] name[%s] are same, don't track skb[%p].\n",
					__func__, cb->dev, cb->dev->name, out, out->name, skb);
		goto out;
	}

	if (cb->dev == NULL || out == NULL) {
		fp_printk(K_INFO, "%s: Each of in_dev[%p] or out_dev[%p] is NULL, don't track skb[%p].\n",
					__func__, cb->dev, out, skb);
		goto out;
	}

	if (cb->flag & DESC_FLAG_TRACK_NAT) {
		if (IPC_HDR_IS_V4(offset2))
			fastpath_out_nf_ipv4(iface, skb, out, offset2, cb);
		else
			fp_printk(K_ERR, "%s: Wrong IPv4 version[%d], offset[%p], flag[%x], skb[%p].\n",
						 __func__, IPC_HDR_IS_V4(offset2), offset2, cb->flag, skb);


	} else if (cb->flag & DESC_FLAG_TRACK_ROUTER) {
		int l3_off = 0;

		if (cb->flag & DESC_FLAG_IPV6)
			fastpath_out_nf_ipv6(iface, skb, out, cb, l3_off);
		else
			fp_printk(K_ERR, "%s: Invalid IPv6 flag[%x], skb[%p].\n", __func__, cb->flag, skb);


		/* if (cb->flag & DESC_FLAG_IPV4) */
			/* TODO: IPv4 routing */
	} else {
		fp_printk(K_DEBUG, "%s: No need to track, skb[%p], cb->flag[%x].\n", __func__, skb, cb->flag);
	}

out:
	TRACK_TABLE_LOCK(fp_track, flags);
	fp_track.table[cb->index].ref_count = 0;
	TRACK_TABLE_UNLOCK(fp_track, flags);
}
EXPORT_SYMBOL(fastpath_out_nf);

module_init(fastpath_init)
module_exit(fastpath_dest)
module_param(fastpath_max_nat, int, 0);
module_param(fastpath_contentfilter, int, 0);
module_param(fastpath_max_bridge, int, 0);
module_param(fastpath_max_router, int, 0);
module_param(fastpath_max_ipsec, int, 0);

/* EXPORT_SYMBOL(fastpath_register); */
/* EXPORT_SYMBOL(fastpath_dyn_register); */
/* EXPORT_SYMBOL(fastpath_unregister); */
/* EXPORT_SYMBOL(fastpath_in); */
/* EXPORT_SYMBOL(fastpath_in_noarp); */
#ifdef FASTPATH_SKBLIST
/* EXPORT_SYMBOL(fastpath_in_list); */
/* EXPORT_SYMBOL(fastpath_in_noarp_list); */
#endif
EXPORT_SYMBOL(fastpath_out);
#endif
EXPORT_SYMBOL(fastpath_in_nf);

MODULE_LICENSE("GPL");
