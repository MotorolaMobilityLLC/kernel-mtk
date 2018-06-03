#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline int _fp_ip6_tcp_ackagg(struct fp_desc *desc, struct sk_buff *skb,
				     struct router_tuple *found_router_tuple, struct ip6header *ip,
				     struct tcpheader *tcp)
{
	int only_ack =
	    (ntohs(ip->payload_len) == (desc->l3_len - sizeof(struct ip6header) + (tcp->th_off << 2)));
	int ret = 0;

	if (!found_router_tuple->ack_agg_enable)
		return ret;
	if (only_ack) {		/* only ack */
		_fp_ackagg_renew();
	}

	if (_fp_ackagg_is_enable()) {
		unsigned long flags;

		spin_lock_irqsave(&found_router_tuple->ack_lock, flags);
		if (only_ack) {	/* only ack */
			ret =
			    _fp_ackagg_check_and_queue_skb(tcp, &found_router_tuple->timeout_ack,
							   &found_router_tuple->ul_ack,
							   &found_router_tuple->ul_ack_skb, desc,
							   skb);
		} else {	/* data + ack */
			_fp_ackagg_release_queued_skb(tcp, &found_router_tuple->timeout_ack,
						      &found_router_tuple->ul_ack,
						      &found_router_tuple->ul_ack_skb);
		}
		spin_unlock_irqrestore(&found_router_tuple->ack_lock, flags);
	}
	return ret;
}

static inline int _fp_ip6_pre_send_skb(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				       u_int8_t vlevel, u_int16_t *vlan, u_int8_t *dst_mac,
				       u_int8_t *src_mac, struct net_device *dst_dev)
{
	int new_header_len;
	u_int8_t vlevel_sw;

	if (vlevel == HW_VLAN_MAGIC)
		vlevel_sw = 0;
	else
		vlevel_sw = vlevel;

	if (vlevel_sw) {
		{		/* add ETYPE_IPv6 */
			new_header_len = 2;
		}
		new_header_len += (vlevel_sw << 2);
		new_header_len += ETH_ALEN * 2;
	} else {
		new_header_len = ETH_HLEN;
	}

	skb->dev = dst_dev;

	skb->vlan_tci = 0;
	if (skb->dev->header_ops) {
		if (new_header_len != desc->l3_off) {
			unsigned short *offset2;

			skb_pull_inline(skb, desc->l3_off);
			{	/* add ETYPE_IPv6 */
				skb_push(skb, 2);
				offset2 = (unsigned short *)(skb->data);
				*offset2 = ETYPE_IPv6;
			}
			if (vlevel_sw) {
				int i;

				for (i = vlevel_sw - 1; i >= 0; i--) {
					skb_push(skb, 4);
					offset2 = (unsigned short *)(skb->data);
					*offset2++ = ETYPE_VLAN;
					*offset2 = vlan[i];
				}
			}
			skb_push(skb, ETH_ALEN * 2);
		}
		if (vlevel == HW_VLAN_MAGIC)
			__vlan_hwaccel_put_tag(skb, ETYPE_VLAN, vlan[0]);

		ether_addr_copy(skb->data, dst_mac);
		memcpy(skb->data + ETH_ALEN, src_mac, ETH_ALEN);
	} else {
		skb_pull_inline(skb, desc->l3_off);
	}

	return 0;
}

#include "hw_ip6_route.c"
#include "hw_ip6_ipsec.c"
#include "hw_ip6_tcp.c"
#include "hw_ip6_udp.c"
#include "hw_ip6_icmp.c"
