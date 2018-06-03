#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"
#include "tuple.c"

/* #ifdef MEASUREMENT */
/* volatile unsigned c0, c1, c2, c3, c4, c5; */
/* #endif */

static struct net_device *bridge_dev;

static unsigned long timeout;

int fastpath_limit;
int fastpath_limit_syn;
int fastpath_limit_udp;
int fastpath_limit_icmp;
int fastpath_limit_icmp_unreach;
int fastpath_limit_size;
int fastpath_limit_total;

int fastpath_ackagg_thres = 30;
int fastpath_ackagg_count;
int fastpath_ackagg_enable;
static unsigned long fastpath_ackagg_jiffies;

#ifndef FASTPATH_NETFILTER
static inline void _fp_xmit(struct fp_desc *desc, int iface_src, struct sk_buff *skb)
#else
static inline void _fp_xmit(struct fp_desc *desc, struct net_device *dev_src, struct sk_buff *skb)
#endif
{
#ifdef ENABLE_PORTBASE_QOS
	if (!skb->iif) {
#ifndef FASTPATH_NETFILTER
		skb->iif = ifaces[iface_src].dev->ifindex;
#else
		skb->iif = dev_src->ifindex;
#endif
	}
#endif
	if (skb->dev->reg_state != NETREG_REGISTERED) {
		desc->flag = DESC_FLAG_NOMEM;
		return;
	}
/* #ifdef MEASUREMENT */
/*	__asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0\n":"=r"(c5)); */
/* #endif */
	/*fastpath_dev_queue_xmit(skb, *(desc->pflag)); */
	fastpath_dev_queue_xmit(skb, 0);
	desc->flag |= DESC_FLAG_FASTPATH;
}

static inline unsigned short _fp_l4_csum(unsigned *csum_l4, unsigned short *port1,
					 unsigned short *port2, unsigned short *ori_csum)
{
	unsigned csum_tmp;

	csum_tmp = (((unsigned)*port1 ^ 0xffff) << 16 | *port2);

	/* 3rd 4bytes */
	*csum_l4 += csum_tmp;
	if (csum_tmp > *csum_l4)
		(*csum_l4)++;

	/* compute new checksum */
	csum_tmp = (*ori_csum ^ 0xffff);
	*csum_l4 += (csum_tmp);
	if (csum_tmp > *csum_l4)
		(*csum_l4)++;

	return csum_fold(*csum_l4);
}

static inline int _fp_ackagg_is_enable(void)
{
	return fastpath_ackagg_enable;
}

static inline void _fp_ackagg_renew(void)
{
	/* if (fastpath_ackagg_jiffies != jiffies) { */ /* JIFFIES_COMPARISON */
	if (time_before(jiffies, fastpath_ackagg_jiffies) || time_after(jiffies, fastpath_ackagg_jiffies)) {
		fastpath_ackagg_jiffies = jiffies;
		if (fastpath_ackagg_count < fastpath_ackagg_thres)
			fastpath_ackagg_enable = 0;
		fastpath_ackagg_count = 0;
	} else {
		fastpath_ackagg_count++;
		if (fastpath_ackagg_enable == 0 && fastpath_ackagg_count >= fastpath_ackagg_thres)
			fastpath_ackagg_enable = 1;
	}
}

static inline int _fp_ackagg_check_and_queue_skb(struct tcpheader *tcp, struct timer_list *timer,
						 uint32_t *queued_ack_num,
						 struct sk_buff **queued_skb, struct fp_desc *desc,
						 struct sk_buff *skb)
{
	if (ntohl(tcp->th_ack) > *queued_ack_num
	    /* XXX: workaround for wrapped ack number */
	    || (*queued_ack_num > (0xFFFFFFFF - 0x3FFFC000)
		&& *queued_ack_num - ntohl(tcp->th_ack) > 0x80000000)) {
		if (*queued_skb) {	/* had queue skb */
			dev_kfree_skb_any(*queued_skb);
		} else {
			mod_timer(timer, jiffies + 1);
		}

		*queued_skb = skb;
		*queued_ack_num = ntohl(tcp->th_ack);

		desc->flag |= DESC_FLAG_FASTPATH;

		return 1;
	}
	return 0;
}

static inline void _fp_ackagg_release_queued_skb(struct tcpheader *tcp, struct timer_list *timer,
						 uint32_t *queued_ack_num,
						 struct sk_buff **queued_skb)
{
	if (*queued_skb) {
		if (ntohl(tcp->th_ack) >= *queued_ack_num) {
			del_timer(timer);
			dev_kfree_skb_any(*queued_skb);
			*queued_skb = NULL;
		}
	}
}

#include "hw_bridge.c"
#include "hw_ip4.c"
#include "hw_ip6.c"
