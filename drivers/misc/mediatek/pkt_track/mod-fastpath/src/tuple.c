/* #include <asm/atomic.h> */
#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <linux/jhash.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include "fastpath.h"
#include "protocol.h"
#include "tuple.h"
#include "desc.h"
#include "fastpath_debug.h"

struct list_head *nat_tuple_hash;
unsigned int nat_tuple_hash_rnd;

struct list_head *bridge_tuple_hash;
unsigned int bridge_tuple_hash_rnd;

struct list_head *router_tuple_hash;
unsigned int router_tuple_hash_rnd;

struct list_head *ipsec_tuple_hash;
unsigned int ipsec_tuple_hash_rnd;


int fastpath_nat;
int fastpath_bridge;
int fastpath_router;
int fastpath_ipsec;
#define USED_TIMEOUT 30
#define UNUSED_TIMEOUT 3
#define USED_MD_RETRY 1

void init_bridge_tuple(void)
{
	int i;

	/* get 4 bytes random number */
	get_random_bytes(&bridge_tuple_hash_rnd, 4);

	/* allocate memory for bridge hash table */
	bridge_tuple_hash = vmalloc(sizeof(struct list_head) * BRIDGE_TUPLE_HASH_SIZE);

	/* init hash table */
	for (i = 0; i < BRIDGE_TUPLE_HASH_SIZE; i++)
		INIT_LIST_HEAD(&bridge_tuple_hash[i]);
}

void init_router_tuple(void)
{
	int i;

	/* get 4 bytes random number */
	get_random_bytes(&router_tuple_hash_rnd, 4);

	/* allocate memory for bridge hash table */
	router_tuple_hash = vmalloc(sizeof(struct list_head) * ROUTER_TUPLE_HASH_SIZE);

	/* init hash table */
	for (i = 0; i < ROUTER_TUPLE_HASH_SIZE; i++)
		INIT_LIST_HEAD(&router_tuple_hash[i]);
}

typedef void (*timer_callback) (unsigned long);

#ifdef DEBUG
void dump_nat_tuple(tuple *t)
{
}
#endif

void del_all_bridge_tuple(void)
{
	/*unsigned long flag; */
	struct list_head *pos, *q;
	struct bridge_tuple *t;
	int i;

	for (i = 0; i < BRIDGE_TUPLE_HASH_SIZE; i++) {
		list_for_each_safe(pos, q, &bridge_tuple_hash[i]) {
			t = list_entry(pos, struct bridge_tuple, list);
			del_bridge_tuple(t);
		}
	}
}

void del_all_router_tuple(void)
{
	/*unsigned long flag; */
	struct list_head *pos, *q;
	struct router_tuple *t;
	int i;

	for (i = 0; i < ROUTER_TUPLE_HASH_SIZE; i++) {
		list_for_each_safe(pos, q, &router_tuple_hash[i]) {
			t = list_entry(pos, struct router_tuple, list);
			del_router_tuple(t);
		}
	}
}

void init_nat_tuple(void)
{
	int i;

	/* get 4 bytes random number */
	get_random_bytes(&nat_tuple_hash_rnd, 4);

	/* allocate memory for two nat hash tables */
	nat_tuple_hash = vmalloc(sizeof(struct list_head) * NAT_TUPLE_HASH_SIZE);

	/* init hash table */
	for (i = 0; i < NAT_TUPLE_HASH_SIZE; i++)
		INIT_LIST_HEAD(&nat_tuple_hash[i]);
}

bool add_nat_tuple(struct nat_tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	struct nat_tuple *found_nat_tuple;

	fp_printk(K_NOTICE, "%s: Add nat tuple[%p], src_port[%d], proto[%d].\n", __func__, t, t->src.all, t->proto);
	fp_printk(K_DEBUG, "%s: Add nat tuple[%p], next[%p], prev[%p].\n", __func__,
				t, t->list.next, t->list.prev);

	if (fastpath_nat >= fastpath_max_nat) {
		fp_printk(K_WARNIN, "%s: Nat tuple table is full! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(nat_tuple_cache, t);
		return false;
	}
	switch (t->proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		{
			hash = HASH_NAT_TUPLE_TCPUDP(t);
		}
		break;
	case IPPROTO_GRE:
		{
			hash = HASH_NAT_TUPLE_GRE(t);
		}
		break;
	default:
		fp_printk(K_ERR, "%s: Not supported protocol[%d]! Tuple[%p] is about to free.\n",
					__func__, t->proto, t);
		kmem_cache_free(nat_tuple_cache, t);
		return false;
	}

	TUPLE_LOCK(&tuple_lock, flag);
	INIT_LIST_HEAD(&t->list);
	/* prevent from duplicating */
	list_for_each_entry(found_nat_tuple, &nat_tuple_hash[hash], list) {
		fp_printk(K_DEBUG, "%s: 1. Add nat tuple, found_nat_tuple[%p], src_ip[%x], dst_ip[%x], proto[%x].\n",
					__func__, found_nat_tuple, found_nat_tuple->src_ip,
					found_nat_tuple->dst_ip, found_nat_tuple->proto);
		fp_printk(K_DEBUG, "%s: 2. Add nat tuple[%p], next[%p], prev[%p].\n", __func__, t,
			 t->list.next, t->list.prev);
		if (found_nat_tuple->src_ip != t->src_ip)
			continue;
		if (found_nat_tuple->dst_ip != t->dst_ip)
			continue;
		if (found_nat_tuple->proto != t->proto)
			continue;
		if (found_nat_tuple->src.all != t->src.all)
			continue;
		if (found_nat_tuple->dst.all != t->dst.all)
			continue;
#ifdef SUPPORT_PPPOE
		if (t->pppoe_out && found_nat_tuple->pppoe_out_id != t->pppoe_out_id)
			continue;
#endif
		TUPLE_UNLOCK(&tuple_lock, flag);
		fp_printk(K_ERR, "%s: Nat tuple is duplicated! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(nat_tuple_cache, t);
		return false;	/* duplication */
	}
	/* add to the list */
	list_add_tail(&t->list, &nat_tuple_hash[hash]);
	fastpath_nat++;
	TUPLE_UNLOCK(&tuple_lock, flag);

	fp_printk(K_DEBUG, "%s: After adding nat tuple[%p], next[%p], prev[%p].\n", __func__, t, t->list.next,
		t->list.prev);

	/* printk("add nat %p\n", t); */

	/* set reference count to 1 */
	atomic_set(&t->ref_count, 1);

/* TODO: Peter DEBUG */
#if 1
	/* set tuple state */
	t->state = TUPLE_STATE_VALID;

#if 0
	init_timer(&t->timeout_used);
	init_timer(&t->timeout_unused);
	init_timer(&t->timeout_ack);
#endif

	spin_lock_init(&t->ack_lock);
	t->ul_ack = 0;
	t->ul_ack_skb = NULL;
#else
	/* set tuple state */
	t->state = TUPLE_STATE_VALID;

	/* init timer and start it */
	init_timer(&t->timeout_used);
	t->timeout_used.data = (unsigned long)t;
	t->timeout_used.function = (timer_callback) timeout_nat_tuple;

	init_timer(&t->timeout_unused);
/* t->timeout_unused.data = (unsigned long)t; */
/* t->timeout_unused.function = (timer_callback)del_nat_tuple; */

	init_timer(&t->timeout_ack);
/* t->timeout_ack.data = (unsigned long)t; */
/* t->timeout_ack.function = (timer_callback)ack_nat_tuple; */
	spin_lock_init(&t->ack_lock);

	mod_timer(&t->timeout_used, jiffies + HZ * USED_TIMEOUT);

/* mod_timer(&t->timeout_unused, jiffies + HZ * 1); */
	t->last = jiffies;

	t->ul_ack = 0;
	t->ul_ack_skb = NULL;
#endif

	return true;
}

static inline unsigned long update_nat_tuple_top(struct nat_tuple *t)
{
	/* refresh timer */
	if (unlikely(time_after(jiffies, t->last + (HZ >> 1)))) {
		t->last = jiffies;
		return t->last + HZ * 1;
		/*mod_timer(&t->timeout_unused, t->last + HZ * 1); */
	}
	return 0;
}

static inline void update_nat_tuple_bot(struct nat_tuple *t, unsigned long next_time)
{
#if 0
	mod_timer(&t->timeout_unused, next_time);
#else
	return;
#endif
}

void del_nat_tuple(struct nat_tuple *t)
{
	unsigned long flag;

	fp_printk(K_NOTICE, "%s: Delete nat tuple[%p].\n", __func__, t);
	fp_printk(K_DEBUG, "%s: Delete nat tuple[%p], next[%p], prev[%p].\n",
				__func__, t, t->list.next, t->list.prev);

#if 0
	del_timer(&t->timeout_used);
	del_timer(&t->timeout_unused);
	del_timer(&t->timeout_ack);
#endif

	if (t->state != TUPLE_STATE_DELETING && t->state != TUPLE_STATE_VALID) {
		fp_printk(K_ALET, "%s: The state[%d] of nat tuple[%p] is not expected!\n", __func__, t->state, t);
		WARN_ON(1);
		return;
	}

	TUPLE_LOCK(&tuple_lock, flag);
	fastpath_nat--;

	/* remove from the list */
	if (t->list.next != LIST_POISON1 && t->list.prev != LIST_POISON2) {
		list_del(&t->list);
		fp_printk(K_DEBUG, "%s: Del nat tuple[%p] successfully, next[%p], prev[%p].\n",
					__func__, t, t->list.next, t->list.prev);
	} else {
		fp_printk(K_ERR, "%s: Del nat tuple fail, tuple[%p], next[%p], prev[%p].\n", __func__, t,
		       t->list.next, t->list.prev);
		WARN_ON(1);
	}
	TUPLE_UNLOCK(&tuple_lock, flag);
/* ack_nat_tuple(t); */
	DEC_REF_NAT_TUPLE(t);
}
EXPORT_SYMBOL(del_nat_tuple);

void ack_nat_tuple(struct nat_tuple *t)
{
	unsigned long flags;

	spin_lock_irqsave(&t->ack_lock, flags);
	if (t->ul_ack_skb) {
		if (t->ul_ack_skb->dev->reg_state == NETREG_REGISTERED)
			dev_queue_xmit(t->ul_ack_skb);
		else
			dev_kfree_skb_any(t->ul_ack_skb);

		t->ul_ack_skb = NULL;
	}
#if 0
	else
		WARN_ON(1);
#endif
	spin_unlock_irqrestore(&t->ack_lock, flags);
}

void del_all_nat_tuple(void)
{
	/*unsigned long flag; */
	struct list_head *pos, *q;
	struct nat_tuple *t;
	int i;

	for (i = 0; i < NAT_TUPLE_HASH_SIZE; i++) {
		list_for_each_safe(pos, q, &nat_tuple_hash[i]) {
			t = list_entry(pos, struct nat_tuple, list);
			del_nat_tuple(t);
		}
	}
}

static inline struct nat_tuple *get_nat_tuple_ip4_tcpudp(struct tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	int not_match;
	struct nat_tuple *found_nat_tuple;
	unsigned long next_time;

	hash = HASH_TUPLE_TCPUDP(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_nat_tuple, &nat_tuple_hash[hash], list) {
		not_match = 0;
#ifndef FASTPATH_NETFILTER
		not_match += (!ifaces[found_nat_tuple->iface_src].ready) ? 1 : 0;
		not_match += (!ifaces[found_nat_tuple->iface_dst].ready) ? 1 : 0;
#else
		fp_printk(K_DEBUG, "%s: 1. Get nat tuple[%p], dev_in[%p].\n", __func__, t, t->dev_in);
		fp_printk(K_DEBUG, "%s: 2. Get nat tuple, found_nat_tuple[%p], dev_src[%p], dev_dst[%p].\n",
				     __func__, found_nat_tuple, found_nat_tuple->dev_src, found_nat_tuple->dev_dst);
		not_match += (found_nat_tuple->dev_src != t->dev_in) ? 1 : 0;
		not_match += (!found_nat_tuple->dev_dst) ? 1 : 0;
#endif
		not_match += (found_nat_tuple->src_ip != t->nat.src) ? 1 : 0;
		not_match += (found_nat_tuple->dst_ip != t->nat.dst) ? 1 : 0;
		not_match += (found_nat_tuple->proto != t->nat.proto) ? 1 : 0;
		not_match += (found_nat_tuple->src.all != t->nat.s.all) ? 1 : 0;
		not_match += (found_nat_tuple->dst.all != t->nat.d.all) ? 1 : 0;
		if (unlikely(not_match))
			continue;
		INC_REF_NAT_TUPLE(found_nat_tuple);
		t->action = 0;
		if (found_nat_tuple->src_ip == found_nat_tuple->nat_src_ip
		    && found_nat_tuple->src.all == found_nat_tuple->nat_src.all
		    && found_nat_tuple->dst_ip == found_nat_tuple->nat_dst_ip
		    && found_nat_tuple->dst.all == found_nat_tuple->nat_dst.all)
			t->action |= TUPLE_ACTION_ROUTER;

		next_time = update_nat_tuple_top(found_nat_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time)) {
			/* TODO: Peter DEBUG */
/* update_nat_tuple_bot(found_nat_tuple, next_time); */
		}
		return found_nat_tuple;
	}
	TUPLE_UNLOCK(&tuple_lock, flag);
	/* not found */
	return 0;
}

/* TODO */
#if 0
static inline struct nat_tuple *get_nat_tuple_ip4_gre_lan(tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	/*int not_match; */
	struct nat_tuple *found_nat_tuple;
	unsigned long next_time;

	hash = HASH_TUPLE_GRE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_nat_tuple, &nat_tuple_hash_in[hash], in_list) {
		if (!ifaces[found_nat_tuple->iface_out].ready
		    || !ifaces[found_nat_tuple->iface_in].ready)
			continue;
		if (found_nat_tuple->in_ip != t->nat.src)
			continue;
		if (found_nat_tuple->out_ip != t->nat.dst)
			continue;
		if (found_nat_tuple->proto != t->nat.proto)
			continue;
		if (found_nat_tuple->in.gre.callid != t->nat.s.gre.callid)
			continue;
		INC_REF_NAT_TUPLE(found_nat_tuple);
		t->action = TUPLE_ACTION_SNAT;
		/*update_nat_tuple(found_nat_tuple); */
		next_time = update_nat_tuple_top(found_nat_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_nat_tuple_bot(found_nat_tuple, next_time);

		return found_nat_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}

static inline struct nat_tuple *get_nat_tuple_ip4_gre_wan(tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	/*int not_match; */
	struct nat_tuple *found_nat_tuple;
	unsigned long next_time;

	hash = HASH_TUPLE_GRE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_nat_tuple, &nat_tuple_hash_out[hash], out_list) {
		if (!ifaces[found_nat_tuple->iface_out].ready
		    || !ifaces[found_nat_tuple->iface_in].ready)
			continue;
#ifdef SUPPORT_PPPOE
		if (t->nat.pppoe_out && found_nat_tuple->pppoe_out_id != t->nat.pppoe_out_id)
			continue;
#endif
		if (found_nat_tuple->out_ip != t->nat.src)
			continue;
		if (found_nat_tuple->nat_ip != t->nat.dst)
			continue;
		if (found_nat_tuple->proto != t->nat.proto)
			continue;
		if (found_nat_tuple->out.gre.callid != t->nat.s.gre.callid)
			continue;
		INC_REF_NAT_TUPLE(found_nat_tuple);
		t->action = TUPLE_ACTION_DNAT;
		/*update_nat_tuple(found_nat_tuple); */
		next_time = update_nat_tuple_top(found_nat_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_nat_tuple_bot(found_nat_tuple, next_time);

		return found_nat_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}
#endif

void add_bridge_tuple(struct bridge_tuple *t)
{
	int i;
	unsigned long flag;
	unsigned int hash;
	struct bridge_tuple *found_bridge_tuple;
	int vlan_flag = 1;

	if (fastpath_bridge >= fastpath_max_bridge) {
		kmem_cache_free(bridge_tuple_cache, t);
		return;
	}
	hash = HASH_BRIDGE_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	INIT_LIST_HEAD(&t->list);
	/* prevent from duplicating */
	list_for_each_entry(found_bridge_tuple, &bridge_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (found_bridge_tuple->iface_src != t->iface_src)
			continue;
#else
		if (found_bridge_tuple->dev_src != t->dev_src)
			continue;
#endif
		if (found_bridge_tuple->vlevel_src != t->vlevel_src)
			continue;
		for (i = 0; i < t->vlevel_src; i++) {
			if (found_bridge_tuple->vlan_src[i] != t->vlan_src[i]) {
				vlan_flag = 0;
				break;
			}
		}
		if (!vlan_flag)
			continue;
		if (found_bridge_tuple->dst_mac1 != t->dst_mac1)
			continue;
		if (found_bridge_tuple->dst_mac2 != t->dst_mac2)
			continue;
		TUPLE_UNLOCK(&tuple_lock, flag);
		fp_printk(K_ERR, "%s: Bridge tuple is duplicated! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(bridge_tuple_cache, t);
		return;		/* duplication */
	}
	/* add to the list */
	list_add_tail(&t->list, &bridge_tuple_hash[hash]);
	fp_printk(K_INFO, "%s: Add bridge tuple[%p], next[%p], prev[%p].\n", __func__, t, t->list.next, t->list.prev);
	fastpath_bridge++;
	TUPLE_UNLOCK(&tuple_lock, flag);

	/* set reference count to 1 */
	atomic_set(&t->ref_count, 1);

#if 0
	/* init timer and start it */
	init_timer(&t->timeout_used);
	t->timeout_used.data = (unsigned long)t;
	t->timeout_used.function = (timer_callback) del_bridge_tuple;

	init_timer(&t->timeout_unused);
	t->timeout_unused.data = (unsigned long)t;
	t->timeout_unused.function = (timer_callback) del_bridge_tuple;

	mod_timer(&t->timeout_used, jiffies + HZ * USED_TIMEOUT);
	mod_timer(&t->timeout_unused, jiffies + HZ * 3);
#endif
	t->last = jiffies;
}

static inline unsigned long update_bridge_tuple_top(struct bridge_tuple *t)
{
	/* refresh timer */
	if (unlikely(time_after(jiffies, t->last + (HZ >> 1)))) {
		t->last = jiffies;
		return t->last + HZ * 3;
		/*mod_timer(&t->timeout_unused, t->last + HZ * 3); */
	}
	return 0;
}

static inline void update_bridge_tuple_bot(struct bridge_tuple *t, unsigned long next_time)
{
#if 0
	mod_timer(&t->timeout_unused, next_time);
#else
	return;
#endif
}

void del_bridge_tuple(struct bridge_tuple *t)
{
	unsigned long flag;
#if 0
	del_timer(&t->timeout_used);
	del_timer(&t->timeout_unused);
#endif
	TUPLE_LOCK(&tuple_lock, flag);
	fastpath_bridge--;
	/* remove from the list */
	if (t->list.next != LIST_POISON1 && t->list.prev != LIST_POISON2)
		list_del(&t->list);
	else {
		fp_printk(K_ERR, "%s: Del bridge tuple fail, tuple[%p], next[%p], prev[%p].\n", __func__, t,
		       t->list.next, t->list.prev);
		WARN_ON(1);
	}
	TUPLE_UNLOCK(&tuple_lock, flag);
	DEC_REF_BRIDGE_TUPLE(t);
}

static inline struct bridge_tuple *get_bridge_tuple(struct tuple *t)
{
	unsigned long flag;
	int i;
	unsigned int hash;
	int vlan_flag = 1;
	struct bridge_tuple *found_bridge_tuple;
	unsigned long next_time;

	hash = HASH_TUPLE(t);
	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_bridge_tuple, &bridge_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (!ifaces[found_bridge_tuple->iface_dst].ready
		    || !ifaces[found_bridge_tuple->iface_src].ready)
			continue;
		if (found_bridge_tuple->iface_src != t->iface_in)
			continue;
#else
		if (!found_bridge_tuple->dev_dst || !found_bridge_tuple->dev_src)
			continue;
		if (found_bridge_tuple->dev_src != t->dev_in)
			continue;
#endif
		if (found_bridge_tuple->vlevel_src != t->bridge.vlevel_src)
			continue;
		if (t->bridge.vlevel_src == HW_VLAN_MAGIC) {
			if (found_bridge_tuple->vlan_src[0] != t->bridge.vlan_src[0])
				vlan_flag = 0;
		} else {
			for (i = 0; i < t->bridge.vlevel_src; i++) {
				if (found_bridge_tuple->vlan_src[i] != t->bridge.vlan_src[i]) {
					vlan_flag = 0;
					break;
				}
			}
		}
		if (!vlan_flag)
			continue;
		if (found_bridge_tuple->dst_mac1 != t->bridge.dst_mac1)
			continue;
		if (found_bridge_tuple->dst_mac2 != t->bridge.dst_mac2)
			continue;
		INC_REF_BRIDGE_TUPLE(found_bridge_tuple);
		t->action = TUPLE_ACTION_BRIDGE;
		/*update_bridge_tuple(found_bridge_tuple); */
		next_time = update_bridge_tuple_top(found_bridge_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_bridge_tuple_bot(found_bridge_tuple, next_time);

		return found_bridge_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}

void ack_router_tuple(struct router_tuple *t)
{
	unsigned long flags;

	spin_lock_irqsave(&t->ack_lock, flags);
	if (t->ul_ack_skb) {
		if (t->ul_ack_skb->dev->reg_state == NETREG_REGISTERED)
			dev_queue_xmit(t->ul_ack_skb);
		else
			dev_kfree_skb_any(t->ul_ack_skb);

		t->ul_ack_skb = NULL;
	}
#if 0
	else
		WARN_ON(1);
#endif
	spin_unlock_irqrestore(&t->ack_lock, flags);
}

void add_router_tuple(struct router_tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	struct router_tuple *found_router_tuple;

	fp_printk(K_INFO, "%s: Add router tuple[%p] with src_port[%d] & proto[%d].\n", __func__, t,
				t->in.all, t->proto);

	if (fastpath_router >= fastpath_max_router) {
		fp_printk(K_ERR, "%s: Router tuple table is full! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(router_tuple_cache, t);
		return;
	}
	hash = HASH_ROUTER_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	INIT_LIST_HEAD(&t->list);
	/* prevent from duplicating */
	list_for_each_entry(found_router_tuple, &router_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (found_router_tuple->iface_src != t->iface_src)
			continue;
#else
		if (found_router_tuple->dev_src != t->dev_src)
			continue;
#endif
		if (!ipv6_addr_equal(&found_router_tuple->saddr, &t->saddr))
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->daddr, &t->daddr))
			continue;
		TUPLE_UNLOCK(&tuple_lock, flag);
		fp_printk(K_ERR, "%s: Router tuple is duplicated! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(router_tuple_cache, t);
		return;		/* duplication */
	}
	/* add to the list */
	list_add_tail(&t->list, &router_tuple_hash[hash]);
	fp_printk(K_INFO, "%s: Add router tuple[%p], next[%p], prev[%p].\n", __func__, t, t->list.next,
				t->list.prev);

	fastpath_router++;
	TUPLE_UNLOCK(&tuple_lock, flag);

	/* set reference count to 1 */
	atomic_set(&t->ref_count, 1);

#if 0
	/* init timer and start it */
	init_timer(&t->timeout_used);
	t->timeout_used.data = (unsigned long)t;
	t->timeout_used.function = (timer_callback) timeout_router_tuple;

	init_timer(&t->timeout_unused);
	t->timeout_unused.data = (unsigned long)t;
	t->timeout_unused.function = (timer_callback) del_router_tuple;

	init_timer(&t->timeout_ack);
	t->timeout_ack.data = (unsigned long)t;
	t->timeout_ack.function = (timer_callback) ack_router_tuple;
#endif
	spin_lock_init(&t->ack_lock);

#if 0
	mod_timer(&t->timeout_used, jiffies + HZ * USED_TIMEOUT);
#endif
/* mod_timer(&t->timeout_unused, jiffies + HZ * 3); */
	t->last = jiffies;

	t->ul_ack = 0;
	t->ul_ack_skb = NULL;
}

bool add_router_tuple_tcpudp(struct router_tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	struct router_tuple *found_router_tuple;

	fp_printk(K_NOTICE, "%s: Add tcpudp router tuple[%p] with src_port[%d] & proto[%d].\n", __func__, t,
				t->in.all, t->proto);

	if (fastpath_router >= fastpath_max_router) {
		kmem_cache_free(router_tuple_cache, t);

		fp_printk(K_ERR, "%s: TCPUDP router is full, tuple[%p], next[%p], prev[%p].\n", __func__, t,
					t->list.next, t->list.prev);
		return false;
	}
	hash = HASH_ROUTER_TUPLE_TCPUDP(t);

	TUPLE_LOCK(&tuple_lock, flag);
	INIT_LIST_HEAD(&t->list);
	/* prevent from duplicating */
	list_for_each_entry(found_router_tuple, &router_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (found_router_tuple->iface_src != t->iface_src)
			continue;
#else
		if (found_router_tuple->dev_src != t->dev_src)
			continue;
#endif
		if (!ipv6_addr_equal(&found_router_tuple->saddr, &t->saddr))
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->daddr, &t->daddr))
			continue;
		if (found_router_tuple->proto != t->proto)
			continue;
		if (found_router_tuple->in.all != t->in.all)
			continue;
		if (found_router_tuple->out.all != t->out.all)
			continue;
		TUPLE_UNLOCK(&tuple_lock, flag);
		fp_printk(K_ERR, "%s: TCPUDP router is duplicated! Tuple[%p] is about to free.\n", __func__, t);
		kmem_cache_free(router_tuple_cache, t);
		return false;	/* duplication */
	}
	/* add to the list */
	list_add_tail(&t->list, &router_tuple_hash[hash]);
	fp_printk(K_DEBUG, "%s: Add tcpudp router tuple[%p], next[%p], prev[%p].\n", __func__, t, t->list.next,
		t->list.prev);

	fastpath_router++;
	TUPLE_UNLOCK(&tuple_lock, flag);

	/* set reference count to 1 */
	atomic_set(&t->ref_count, 1);

/* TODO: Peter DEBUG */
#if 1
	/* set tuple state */
	t->state = TUPLE_STATE_VALID;

#if 0
	init_timer(&t->timeout_used);
	init_timer(&t->timeout_unused);
	init_timer(&t->timeout_ack);
#endif
	spin_lock_init(&t->ack_lock);
	t->ul_ack = 0;
	t->ul_ack_skb = NULL;
#else
	/* set tuple state */
	t->state = TUPLE_STATE_VALID;

	/* init timer and start it */
	init_timer(&t->timeout_used);
	t->timeout_used.data = (unsigned long)t;
	t->timeout_used.function = (timer_callback) timeout_router_tuple;

	init_timer(&t->timeout_unused);
/* t->timeout_unused.data = (unsigned long)t; */
/* t->timeout_unused.function = (timer_callback)del_router_tuple; */

	init_timer(&t->timeout_ack);
/* t->timeout_ack.data = (unsigned long)t; */
/* t->timeout_ack.function = (timer_callback)ack_router_tuple; */
	spin_lock_init(&t->ack_lock);

	mod_timer(&t->timeout_used, jiffies + HZ * USED_TIMEOUT);

/* mod_timer(&t->timeout_unused, jiffies + HZ * 3); */
	t->last = jiffies;

	t->ul_ack = 0;
	t->ul_ack_skb = NULL;
#endif

	return true;
}

static inline unsigned long update_router_tuple_top(struct router_tuple *t)
{
	/* refresh timer */
	if (unlikely(time_after(jiffies, t->last + (HZ >> 1)))) {
		t->last = jiffies;
		return t->last + HZ * 3;
		/*mod_timer(&t->timeout_unused, t->last + HZ * 3); */
	}
	return 0;
}

static inline void update_router_tuple_bot(struct router_tuple *t, unsigned long next_time)
{
#if 0
	mod_timer(&t->timeout_unused, next_time);
#else
	return;
#endif
}

void del_router_tuple(struct router_tuple *t)
{
	unsigned long flag;

	fp_printk(K_NOTICE, "%s: Delete router tuple[%p].\n", __func__, t);
	fp_printk(K_DEBUG, "%s: Delete router tuple[%p], next[%p], prev[%p].\n",
				__func__, t, t->list.next, t->list.prev);

#if 0
	del_timer(&t->timeout_used);
	del_timer(&t->timeout_unused);
	del_timer(&t->timeout_ack);
#endif

	if (t->state != TUPLE_STATE_DELETING && t->state != TUPLE_STATE_VALID) {
		fp_printk(K_ALET, "%s: The state[%d] of nat tuple[%p] is not expected!\n", __func__, t->state, t);
		WARN_ON(1);
		return;
	}

	TUPLE_LOCK(&tuple_lock, flag);
	fastpath_router--;

	/* remove from the list */
	if (t->list.next != LIST_POISON1 && t->list.prev != LIST_POISON2) {
		list_del(&t->list);
		fp_printk(K_DEBUG, "%s: Delete router tuple[%p] successfully, next[%p], prev[%p].\n",
					__func__, t, t->list.next, t->list.prev);
	} else {
		fp_printk(K_ERR, "%s: Del router tuple fail, tuple[%p], next[%p], prev[%p].\n", __func__, t,
		       t->list.next, t->list.prev);
		WARN_ON(1);
	}
	TUPLE_UNLOCK(&tuple_lock, flag);
	/* <JQ_TO_DELETE> */
	#if 0
	ack_router_tuple(t);
	#endif

	DEC_REF_ROUTER_TUPLE(t);
}
EXPORT_SYMBOL(del_router_tuple);

#if 0
struct router_tuple *get_router_tuple(struct router_tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	struct router_tuple *found_router_tuple;
	unsigned long next_time;

	hash = HASH_ROUTER_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_router_tuple, &router_tuple_hash[hash], list) {
		if (!ifaces[found_router_tuple->iface_dst].ready
		    || !ifaces[found_router_tuple->iface_src].ready)
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->saddr, &t->saddr))
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->daddr, &t->daddr))
			continue;
		INC_REF_ROUTER_TUPLE(found_router_tuple);
		/* t->action = TUPLE_ACTION_ROUTER; */
		/*update_router_tuple(found_router_tuple); */
		next_time = update_router_tuple_top(found_router_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_router_tuple_bot(found_router_tuple, next_time);

		return found_router_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}
#endif

static inline struct router_tuple *get_router_tuple_tcpudp(struct router_tuple *t)
{
	unsigned long flag;
	unsigned int hash;
	struct router_tuple *found_router_tuple;
	unsigned long next_time;
	int not_match;

	hash = HASH_ROUTER_TUPLE_TCPUDP(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_router_tuple, &router_tuple_hash[hash], list) {
#if 0
		if (!ifaces[found_router_tuple->iface_dst].ready
		    || !ifaces[found_router_tuple->iface_src].ready)
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->saddr, &t->saddr))
			continue;
		if (!ipv6_addr_equal(&found_router_tuple->daddr, &t->daddr))
			continue;
		if (found_router_tuple->proto != t->proto)
			continue;
		if (found_router_tuple->in.all != t->in.all)
			continue;
		if (found_router_tuple->out.all != t->out.all)
			continue;
#else
		not_match = 0;
#ifndef FASTPATH_NETFILTER
		not_match += (!ifaces[found_router_tuple->iface_dst].ready) ? 1 : 0;
		not_match += (!ifaces[found_router_tuple->iface_src].ready) ? 1 : 0;
#else
		not_match += (!found_router_tuple->dev_dst) ? 1 : 0;
		not_match += (!found_router_tuple->dev_src) ? 1 : 0;
#endif
		not_match += (!ipv6_addr_equal(&found_router_tuple->saddr, &t->saddr)) ? 1 : 0;
		not_match += (!ipv6_addr_equal(&found_router_tuple->daddr, &t->daddr)) ? 1 : 0;
		not_match += (found_router_tuple->proto != t->proto) ? 1 : 0;
		not_match += (found_router_tuple->in.all != t->in.all) ? 1 : 0;
		not_match += (found_router_tuple->out.all != t->out.all) ? 1 : 0;
		if (unlikely(not_match))
			continue;
#endif
		INC_REF_ROUTER_TUPLE(found_router_tuple);
		/* t->action = TUPLE_ACTION_ROUTER; */
		/*update_router_tuple(found_router_tuple); */
		next_time = update_router_tuple_top(found_router_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_router_tuple_bot(found_router_tuple, next_time);

		return found_router_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}

void init_ipsec_tuple(void)
{
	int i;

	/* get 4 bytes random number */
	get_random_bytes(&ipsec_tuple_hash_rnd, 4);

	/* allocate memory for bridge hash table */
	ipsec_tuple_hash = vmalloc(sizeof(struct list_head) * IPSEC_TUPLE_HASH_SIZE);

	/* init hash table */
	for (i = 0; i < IPSEC_TUPLE_HASH_SIZE; i++)
		INIT_LIST_HEAD(&ipsec_tuple_hash[i]);
}

void add_ipsec_tuple(struct ipsec_tuple *t, u_int32_t fp_flag)
{
	unsigned long flag;
	unsigned int hash;
	struct ipsec_tuple *found_ipsec_tuple;

	if (fastpath_ipsec >= fastpath_max_ipsec) {
		kmem_cache_free(ipsec_tuple_cache, t);
		return;
	}

	hash = HASH_IPSEC_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	INIT_LIST_HEAD(&t->list);
	/* prevent from duplicating */
	if (fp_flag & DESC_FLAG_IPV4) {
		if (fp_flag & DESC_FLAG_ESP_DECODE) {
			list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
				if (found_ipsec_tuple->spi != t->spi)
					continue;
				if (found_ipsec_tuple->xfrm_proto != t->xfrm_proto)
					continue;
#if 0
				if (found_ipsec_tuple->saddr.v4[0] !=
				    found_ipsec_tuple->saddr.v4[0])
					continue;
				if (found_ipsec_tuple->daddr.v4[0] !=
				    found_ipsec_tuple->daddr.v4[0])
					continue;
#else
				if (found_ipsec_tuple->saddr.v4[0] != t->saddr.v4[0])
					continue;
				if (found_ipsec_tuple->daddr.v4[0] != t->daddr.v4[0])
					continue;
#endif
				TUPLE_UNLOCK(&tuple_lock, flag);
				fp_printk(K_ERR, "%s: IPsec tuple is duplicated! Tuple[%p] is about to free.\n",
						__func__, t);
				kmem_cache_free(ipsec_tuple_cache, t);
				return;	/* duplication */
			}
		} else if (fp_flag & DESC_FLAG_ESP_ENCODE) {
			list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
#if 0
				if (found_ipsec_tuple->saddr.v4[0] !=
				    found_ipsec_tuple->saddr.v4[0])
					continue;
				if (found_ipsec_tuple->daddr.v4[0] !=
				    found_ipsec_tuple->daddr.v4[0])
					continue;
#else
				if (found_ipsec_tuple->saddr.v4[0] != t->saddr.v4[0])
					continue;
				if (found_ipsec_tuple->daddr.v4[0] != t->daddr.v4[0])
					continue;
#endif
				if (found_ipsec_tuple->traffic_proto != t->traffic_proto)
					continue;
				if (found_ipsec_tuple->traffic_para.all != t->traffic_para.all)
					continue;
				TUPLE_UNLOCK(&tuple_lock, flag);
				fp_printk(K_ERR, "%s: IPsec tuple is duplicated! Tuple[%p] is about to free.\n",
						__func__, t);
				kmem_cache_free(ipsec_tuple_cache, t);
				return;	/* duplication */
			}
		} else {
			fp_printk(K_ERR, "%s %d : flag error : %d\n", __func__, __LINE__, fp_flag);
			TUPLE_UNLOCK(&tuple_lock, flag);
			kmem_cache_free(ipsec_tuple_cache, t);
			return;
		}
	} else if (fp_flag & DESC_FLAG_IPV6) {
		if (fp_flag & DESC_FLAG_ESP_DECODE) {
			list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
				if (found_ipsec_tuple->spi != t->spi)
					continue;
				if (found_ipsec_tuple->xfrm_proto != t->xfrm_proto)
					continue;
				if (!ipv6_addr_equal(&found_ipsec_tuple->saddr.v6, &t->saddr.v6))
					continue;
				if (!ipv6_addr_equal(&found_ipsec_tuple->daddr.v6, &t->daddr.v6))
					continue;
				TUPLE_UNLOCK(&tuple_lock, flag);
				fp_printk(K_ERR, "%s: IPsec tuple is duplicated! Tuple[%p] is about to free.\n",
						__func__, t);
				kmem_cache_free(ipsec_tuple_cache, t);
				return;	/* duplication */
			}
		} else if (fp_flag & DESC_FLAG_ESP_ENCODE) {
			list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
				if (!ipv6_addr_equal(&found_ipsec_tuple->saddr.v6, &t->saddr.v6))
					continue;
				if (!ipv6_addr_equal(&found_ipsec_tuple->daddr.v6, &t->daddr.v6))
					continue;
				if (found_ipsec_tuple->traffic_proto != t->traffic_proto)
					continue;
				if (found_ipsec_tuple->traffic_para.all != t->traffic_para.all)
					continue;
				TUPLE_UNLOCK(&tuple_lock, flag);
				fp_printk(K_ERR, "%s: IPsec tuple is duplicated! Tuple[%p] is about to free.\n",
						__func__, t);
				kmem_cache_free(ipsec_tuple_cache, t);
				return;	/* duplication */
			}
		} else {
			fp_printk(K_ERR, "%s %d : flag error : %d\n", __func__, __LINE__, fp_flag);
			TUPLE_UNLOCK(&tuple_lock, flag);
			kmem_cache_free(ipsec_tuple_cache, t);
			return;
		}
	} else {
		fp_printk(K_ERR, "%s %d : flag error : %d\n", __func__, __LINE__, fp_flag);
		TUPLE_UNLOCK(&tuple_lock, flag);
		kmem_cache_free(ipsec_tuple_cache, t);
		return;
	}
	/* add to the list */
	list_add_tail(&t->list, &ipsec_tuple_hash[hash]);
	fp_printk(K_INFO, "%s: Add IPsec tuple[%p], next[%p], prev[%p].\n", __func__, t, t->list.next,
		t->list.prev);
	fastpath_ipsec++;
	TUPLE_UNLOCK(&tuple_lock, flag);

	/* set reference count to 1 */
	atomic_set(&t->ref_count, 1);

#if 0
	/* init timer and start it */
	init_timer(&t->timeout_used);
	t->timeout_used.data = (unsigned long)t;
	t->timeout_used.function = (timer_callback) del_ipsec_tuple;

	init_timer(&t->timeout_unused);
	t->timeout_unused.data = (unsigned long)t;
	t->timeout_unused.function = (timer_callback) del_ipsec_tuple;

	mod_timer(&t->timeout_used, jiffies + HZ * USED_TIMEOUT);
	mod_timer(&t->timeout_unused, jiffies + HZ * 3);
#endif
	t->last = jiffies;
}

static inline unsigned long update_ipsec_tuple_top(struct ipsec_tuple *t)
{
	/* refresh timer */
	if (unlikely(time_after(jiffies, t->last + (HZ >> 1)))) {
		t->last = jiffies;
		return t->last + HZ * 3;
		/*mod_timer(&t->timeout_unused, t->last + HZ * 3); */
	}
	return 0;
}

static inline void update_ipsec_tuple_bot(struct ipsec_tuple *t, unsigned long next_time)
{
#if 0
	mod_timer(&t->timeout_unused, next_time);
#else
	return;
#endif
}

void del_ipsec_tuple(struct ipsec_tuple *t)
{
	unsigned long flag;
#if 0
	del_timer(&t->timeout_used);
	del_timer(&t->timeout_unused);
#endif
	TUPLE_LOCK(&tuple_lock, flag);
	fastpath_ipsec--;
	/* remove from the list */
	if (t->list.next != LIST_POISON1 && t->list.prev != LIST_POISON2)
		list_del(&t->list);
	else {
		fp_printk(K_ERR, "%s: Del ipsec tuple fail, tuple[%p], next[%p], prev[%p].\n", __func__, t,
		       t->list.next, t->list.prev);
		WARN_ON(1);
	}
	TUPLE_UNLOCK(&tuple_lock, flag);
	DEC_REF_IPSEC_TUPLE(t);
}

struct ipsec_tuple *get_ip4_ipsec_tuple(struct ipsec_tuple *t, u_int32_t fp_flag)
{
	unsigned long flag;
	unsigned int hash;
	struct ipsec_tuple *found_ipsec_tuple;
	unsigned long next_time;

	hash = HASH_IPSEC_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (!ifaces[found_ipsec_tuple->iface_dst].ready
		    || !ifaces[found_ipsec_tuple->iface_src].ready)
			continue;
#else
		if (!found_ipsec_tuple->dev_dst || !found_ipsec_tuple->dev_src)
			continue;
#endif
		if (fp_flag & DESC_FLAG_ESP_DECODE) {
			if (found_ipsec_tuple->spi != t->spi)
				continue;
			if (found_ipsec_tuple->xfrm_proto != t->xfrm_proto)
				continue;
#if 0
			if (found_ipsec_tuple->saddr.v4[0] != found_ipsec_tuple->saddr.v4[0])
				continue;
			if (found_ipsec_tuple->daddr.v4[0] != found_ipsec_tuple->daddr.v4[0])
				continue;
#else
			if (found_ipsec_tuple->saddr.v4[0] != t->saddr.v4[0])
				continue;
			if (found_ipsec_tuple->daddr.v4[0] != t->daddr.v4[0])
				continue;
#endif
		} else if (fp_flag & DESC_FLAG_ESP_ENCODE) {
#if 0
			if (found_ipsec_tuple->saddr.v4[0] != found_ipsec_tuple->saddr.v4[0])
				continue;
			if (found_ipsec_tuple->daddr.v4[0] != found_ipsec_tuple->daddr.v4[0])
				continue;
#else
			if (found_ipsec_tuple->saddr.v4[0] != t->saddr.v4[0])
				continue;
			if (found_ipsec_tuple->daddr.v4[0] != t->daddr.v4[0])
				continue;
#endif
			if (found_ipsec_tuple->traffic_proto != t->traffic_proto)
				continue;
			if (found_ipsec_tuple->traffic_para.all != t->traffic_para.all)
				continue;
		} else {
			TUPLE_UNLOCK(&tuple_lock, flag);
			return 0;
		}
		INC_REF_IPSEC_TUPLE(found_ipsec_tuple);

		/*update_ipsec_tuple(found_ipsec_tuple); */
		next_time = update_ipsec_tuple_top(found_ipsec_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_ipsec_tuple_bot(found_ipsec_tuple, next_time);

		return found_ipsec_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}

struct ipsec_tuple *get_ip6_ipsec_tuple(struct ipsec_tuple *t, u_int32_t fp_flag)
{
	unsigned long flag;
	/*unsigned long flags; */
	unsigned int hash;
	struct ipsec_tuple *found_ipsec_tuple;
	unsigned long next_time;

	hash = HASH_IPSEC_TUPLE(t);

	TUPLE_LOCK(&tuple_lock, flag);
	list_for_each_entry(found_ipsec_tuple, &ipsec_tuple_hash[hash], list) {
#ifndef FASTPATH_NETFILTER
		if (!ifaces[found_ipsec_tuple->iface_dst].ready
		    || !ifaces[found_ipsec_tuple->iface_src].ready)
			continue;
#else
		if (!found_ipsec_tuple->dev_dst || !found_ipsec_tuple->dev_src)
			continue;
#endif
		if (fp_flag & DESC_FLAG_ESP_DECODE) {
			if (found_ipsec_tuple->spi != t->spi)
				continue;
			if (found_ipsec_tuple->xfrm_proto != t->xfrm_proto)
				continue;
			if (!ipv6_addr_equal(&found_ipsec_tuple->saddr.v6, &t->saddr.v6))
				continue;
			if (!ipv6_addr_equal(&found_ipsec_tuple->daddr.v6, &t->daddr.v6))
				continue;
		} else if (fp_flag & DESC_FLAG_ESP_ENCODE) {
			if (!ipv6_addr_equal(&found_ipsec_tuple->saddr.v6, &t->saddr.v6))
				continue;
			if (!ipv6_addr_equal(&found_ipsec_tuple->daddr.v6, &t->daddr.v6))
				continue;
			if (found_ipsec_tuple->traffic_proto != t->traffic_proto)
				continue;
			if (found_ipsec_tuple->traffic_para.all != t->traffic_para.all)
				continue;
		} else {
			TUPLE_UNLOCK(&tuple_lock, flag);
			return 0;
		}
		INC_REF_IPSEC_TUPLE(found_ipsec_tuple);
		/*update_ipsec_tuple(found_ipsec_tuple); */
		next_time = update_ipsec_tuple_top(found_ipsec_tuple);
		TUPLE_UNLOCK(&tuple_lock, flag);
		if (unlikely(next_time))
			update_ipsec_tuple_bot(found_ipsec_tuple, next_time);

		return found_ipsec_tuple;
	}
	/* not found */
	TUPLE_UNLOCK(&tuple_lock, flag);
	return 0;
}

void del_all_ipsec_tuple(void)
{
	/*unsigned long flag; */
	struct list_head *pos, *q;
	struct ipsec_tuple *t;
	int i;

	for (i = 0; i < IPSEC_TUPLE_HASH_SIZE; i++) {
		list_for_each_safe(pos, q, &ipsec_tuple_hash[i]) {
			t = list_entry(pos, struct ipsec_tuple, list);
			del_ipsec_tuple(t);
		}
	}
}

void del_all_tuple(void)
{
	fp_printk(K_DEBUG, "%s: Delete all tuple!\n", __func__);

	del_all_nat_tuple();
	del_all_bridge_tuple();
	del_all_router_tuple();
	del_all_ipsec_tuple();
}
