#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_bridge(struct fp_desc *desc, struct sk_buff *skb, unsigned char *dmac,
			     unsigned char *smac, u_int8_t iface_in)
{
	struct tuple t;
	struct bridge_tuple *found_bridge_tuple;
	int i;
	u_int8_t vlevel_src, vlevel_dst;

	t.bridge.vlevel_src = desc->vlevel;
	for (i = 0; i < MAX_VLAN_LEVEL; i++)
		t.bridge.vlan_src[i] = (i < desc->vlevel) ? (desc->vlan[i] & 0xff0f) : 0;

	t.bridge.dst_mac1 = *(unsigned short *)dmac;
	t.bridge.dst_mac2 = *(unsigned int *)(dmac + 2);

#ifndef FASTPATH_NETFILTER
	t.iface_in = iface_in;
#else
	t.dev_in = skb->dev;
#endif

	found_bridge_tuple = get_bridge_tuple(&t);
	if (likely(found_bridge_tuple)) {
		if (likely(found_bridge_tuple->vlevel_src != HW_VLAN_MAGIC))
			vlevel_src = found_bridge_tuple->vlevel_src;
		else
			vlevel_src = 0;

		if (likely(found_bridge_tuple->vlevel_dst != HW_VLAN_MAGIC))
			vlevel_dst = found_bridge_tuple->vlevel_dst;
		else
			vlevel_dst = 0;

#ifndef FASTPATH_NETFILTER
		skb->dev = ifaces[found_bridge_tuple->iface_dst].dev;
#else
		skb->dev = found_bridge_tuple->dev_dst;
#endif
		if (unlikely(vlevel_src || vlevel_dst)) {
			int i;
			u_int16_t *offset2;

			if (vlevel_src == vlevel_dst) {
				offset2 = (u_int16_t *) (skb->data + ETH_HLEN);
				for (i = 0; i < vlevel_dst; i++) {
					*offset2 = found_bridge_tuple->vlan_dst[i];
					offset2 += 2;
				}
			} else {
				u_int8_t src_mac[ETH_ALEN];

				ether_addr_copy(src_mac, smac);

				skb_pull_inline(skb, ETH_ALEN * 2 + 4 * vlevel_src);	/* suppress original header */
				skb_push(skb, ETH_ALEN * 2 + 4 * vlevel_dst);	/* add current header */

				ether_addr_copy(skb->data, found_bridge_tuple->dst_mac);
				memcpy(skb->data + ETH_ALEN, src_mac, ETH_ALEN);
				offset2 = (u_int16_t *) (skb->data + ETH_ALEN * 2);
				for (i = 0; i < vlevel_dst; i++) {
					*offset2++ = ETYPE_VLAN;
					*offset2++ = found_bridge_tuple->vlan_dst[i];
				}
			}
		}
		if (found_bridge_tuple->vlevel_dst == HW_VLAN_MAGIC)
			__vlan_hwaccel_put_tag(skb, ETYPE_VLAN, found_bridge_tuple->vlan_dst[0]);
		else
			skb->vlan_tci = 0;

#ifndef FASTPATH_NETFILTER
		_fp_xmit(desc, found_bridge_tuple->iface_src, skb);
#else
		_fp_xmit(desc, found_bridge_tuple->dev_src, skb);
#endif

		DEC_REF_BRIDGE_TUPLE(found_bridge_tuple);
		return;
	}
	desc->flag |= DESC_FLAG_TRACK_BRIDGE;
}
