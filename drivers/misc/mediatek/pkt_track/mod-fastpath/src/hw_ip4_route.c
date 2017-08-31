#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_ip4_route(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				struct nat_tuple *found_nat_tuple, struct ip4header *ip, void *l4_header)
{
	u_int8_t *src_dev_addr;

	rcu_read_lock();
	if (bridge_dev && netdev_master_upper_dev_get_rcu(skb->dev) == bridge_dev) {
		/* this net_device is added to bridge */
		src_dev_addr = bridge_dev->dev_addr;
	} else {
		src_dev_addr = skb->dev->dev_addr;
	}

	rcu_read_unlock();
#ifndef FASTPATH_NETFILTER
#ifdef SUPPORT_PPPOE
	if (unlikely
	    (_fp_ip4_pre_send_skb
	     (desc, skb, t, found_nat_tuple->iface_src, found_nat_tuple->vlevel_dst,
	      found_nat_tuple->pppoe_out, found_nat_tuple->pppoe_out_id, found_nat_tuple->vlan_dst,
	      found_nat_tuple->dst_mac, src_dev_addr, ip, ifaces[found_nat_tuple->iface_dst].dev,
	      1))) {
		return;
	}
#else
	if (unlikely
	    (_fp_ip4_pre_send_skb
	     (desc, skb, t, found_nat_tuple->iface_src, found_nat_tuple->vlevel_dst,
	      found_nat_tuple->vlan_dst, found_nat_tuple->dst_mac, src_dev_addr, ip,
	      ifaces[found_nat_tuple->iface_dst].dev, 1))) {
		return;
	}
#endif
#else
#ifdef SUPPORT_PPPOE
	if (unlikely
	    (_fp_ip4_pre_send_skb
	     (desc, skb, t, found_nat_tuple->dev_src, found_nat_tuple->vlevel_dst,
	      found_nat_tuple->pppoe_out, found_nat_tuple->pppoe_out_id, found_nat_tuple->vlan_dst,
	      found_nat_tuple->dst_mac, src_dev_addr, ip, found_nat_tuple->dev_dst, 1))) {
		return;
	}
#else
	if (unlikely
	    (_fp_ip4_pre_send_skb
	     (desc, skb, t, found_nat_tuple->dev_src, found_nat_tuple->vlevel_dst,
	      found_nat_tuple->vlan_dst, found_nat_tuple->dst_mac, src_dev_addr, ip,
	      found_nat_tuple->dev_dst, 1))) {
		return;
	}
#endif
#endif
	else {
		if (ip->ip_p == IPPROTO_TCP
		    && _fp_ip4_tcp_ackagg(desc, skb, found_nat_tuple, ip,
					  (struct tcpheader *) l4_header)) {
			return;
		}
#ifndef FASTPATH_NETFILTER
		_fp_xmit(desc, found_nat_tuple->iface_src, skb);
#else
		_fp_xmit(desc, found_nat_tuple->dev_src, skb);
#endif
	}
}
