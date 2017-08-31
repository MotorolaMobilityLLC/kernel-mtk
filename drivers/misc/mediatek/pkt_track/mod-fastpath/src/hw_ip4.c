#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline unsigned short _fp_ip4_l3_csum(unsigned *csum_l4, u_int32_t *ip1, u_int32_t *ip2,
					     unsigned short *ori_csum)
{
	unsigned csum_l3, csum_tmp;

	/* 1st 4bytes */
	csum_l3 = (*ip1 ^ 0xffffffff);

	/* 2nd 4bytes */
	csum_l3 += *ip2;
	if (*ip2 > csum_l3)
		csum_l3++;

	/* backup csum_l3 for future use */
	*csum_l4 = csum_l3;

	/* compute new checksum */
	csum_tmp = (*ori_csum ^ 0xffff);
	csum_l3 += (csum_tmp);
	if (csum_tmp > csum_l3)
		csum_l3++;

	return csum_fold(csum_l3);
}

static inline int _fp_ip4_tcp_ackagg(struct fp_desc *desc, struct sk_buff *skb,
				     struct nat_tuple *found_nat_tuple, struct ip4header *ip, struct tcpheader *tcp)
{
	int ip_header_len = ip->ip_hl << 2;
	int ret = 0;

	if (!found_nat_tuple->ack_agg_enable)
		return ret;

	if (ntohs(ip->ip_len) == ip_header_len + (tcp->th_off << 2)) {	/* only ack */
		_fp_ackagg_renew();
	}

	if (_fp_ackagg_is_enable()) {
		unsigned long flags;

		spin_lock_irqsave(&found_nat_tuple->ack_lock, flags);
		if (ntohs(ip->ip_len) == ip_header_len + (tcp->th_off << 2)) {	/* only ack */
			ret =
			    _fp_ackagg_check_and_queue_skb(tcp, &found_nat_tuple->timeout_ack,
							   &found_nat_tuple->ul_ack,
							   &found_nat_tuple->ul_ack_skb, desc, skb);
		} else {	/* data + ack */
			_fp_ackagg_release_queued_skb(tcp, &found_nat_tuple->timeout_ack,
						      &found_nat_tuple->ul_ack,
						      &found_nat_tuple->ul_ack_skb);
		}
		spin_unlock_irqrestore(&found_nat_tuple->ack_lock, flags);
	}
	return ret;
}

#ifdef IP_FRAGMENT
#ifndef FASTPATH_NETFILTER
#ifdef SUPPORT_PPPOE
static inline void _fp_ip4_xmit_frag(struct fp_desc *desc, int iface_src, struct sk_buff *skb, struct tuple *t,
				     struct ip4header *ip, int new_header_len,
				     unsigned short *pppoe_payload_len)
#else
static inline void _fp_ip4_xmit_frag(struct fp_desc *desc, int iface_src, struct sk_buff *skb, struct tuple *t,
				     struct ip4header *ip, int new_header_len)
#endif
#else
#ifdef SUPPORT_PPPOE
static inline void _fp_ip4_xmit_frag(struct fp_desc *desc, struct net_device *dev_src,
				     struct sk_buff *skb, struct tuple *t, struct ip4header *ip,
				     int new_header_len, unsigned short *pppoe_payload_len)
#else
static inline void _fp_ip4_xmit_frag(struct fp_desc *desc, struct net_device *dev_src,
				     struct sk_buff *skb, struct tuple *t, struct ip4header *ip,
				     int new_header_len)
#endif
#endif
{
	/* do IP fragmentation */
	int ip_header_len = desc->l3_len;
	struct sk_buff *skb2;
	int left = skb->len - new_header_len - ip_header_len;
	int max_fragment_len =
	    (new_header_len >
	     skb->dev->hard_header_len) ? (skb->dev->mtu - new_header_len +
					   skb->dev->hard_header_len -
					   ip_header_len) : (skb->dev->mtu - ip_header_len);
	int len;
	int offset = 0;
	unsigned char *ptr = skb->data + new_header_len + ip_header_len;
	struct ip4header *ip2;

	max_fragment_len &= ~7;
	/* printk("frame size=%d/%d max_fragment_len=%d offset=%d ip_header_len=%d\n", */
	/*			skb->len, left,  max_fragment_len, offset, ip_header_len); */
	while (left > 0) {
		skb2 = dev_alloc_skb(32 + ip_header_len + max_fragment_len);

		if (skb2 == NULL) {
			dev_kfree_skb_any(skb);
			desc->flag |= DESC_FLAG_NOMEM;
			return;
		}

		len = (left > max_fragment_len) ? max_fragment_len : left;
		/* printk("fragment size %d\n", len); */

		/* make ip header 32bit alignment */
		skb_reserve(skb2, 32 - new_header_len);
		/* copy L2 and L3 header */
#ifdef SUPPORT_PPPOE
		if (pppoe_payload_len)
			*pppoe_payload_len = htons(2 + ip_header_len + len);
#endif
		memcpy(skb2->tail, skb->data, new_header_len + ip_header_len);
		skb_put(skb2, new_header_len + ip_header_len);
		skb_set_network_header(skb2, new_header_len);
		skb2->transport_header = skb2->network_header + ip_header_len;
		ip2 = (struct ip4header *) (skb2->data + new_header_len);

		/* copy L3 payload */
		memcpy(skb2->tail, ptr, len);
		skb_put(skb2, len);

		left -= len;

		/* ip total length */
		ip2->ip_len = htons(ip_header_len + len);
		if (left != 0)
			ip2->ip_off = htons((offset >> 3) + 0x2000);
		else
			ip2->ip_off = htons(offset >> 3);

		do {
			u_int32_t diffs2[] = {
			    ip->ip_len ^ 0xffff, ip2->ip_len, ip->ip_off ^ 0xffff, ip2->ip_off };
			/* re-calculate ip checksum */
			ip2->ip_sum =
			    csum_fold(csum_partial
				      ((char *)diffs2, sizeof(diffs2), ip->ip_sum ^ 0xffff));
		} while (0);

		if (offset == 0 && ip_header_len > 20) {
			int i, optlen;
			/* suppress unused IP options */
			unsigned char *optptr = ((unsigned char *)ip) + 20;
			unsigned int *optptr4 = (unsigned int *)optptr;
			int l = ip_header_len - 20;
			int diffs1_count = (l + 3) >> 2;
			/*u_int32_t diffs1[20]; */
			/* too big variable, allocate it on heap to reduce stack usage */
			u_int32_t *diffs1 = kmalloc(sizeof(u_int32_t) * 20, GFP_ATOMIC);

			if (unlikely(diffs1 == NULL)) {
				desc->flag |= DESC_FLAG_NOMEM;
				return;
			}
			for (i = 0; i < diffs1_count; i++)
				diffs1[i] = (*(optptr4 + i)) ^ 0xffffffff;

			/* change all "do not copy" options to NOOP */
			while (l > 0) {
				switch (*optptr) {
				case 0:	/* END */
					l = 0;
					continue;
				case 1:	/* NOP */
					l--;
					optptr++;
					continue;
				}
				optlen = optptr[1];
				if (optlen < 2 || optlen > l)
					break;
				if ((*optptr & 0x80) == 0)
					memset(optptr, 1, optlen);
				l -= optlen;
				optptr += optlen;
			}
			for (i = 0; i < diffs1_count; i++)
				diffs1[i + diffs1_count] = *(optptr4 + i);

			ip->ip_sum =
			    csum_fold(csum_partial
				      ((char *)diffs1, sizeof(u_int32_t) * diffs1_count * 2,
				       ip->ip_sum ^ 0xffff));
			kfree(diffs1);
		}

		offset += len;
		ptr += len;
		skb2->dev = skb->dev;

#ifndef FASTPATH_NETFILTER
		_fp_xmit(desc, iface_src, skb2);
#else
		_fp_xmit(desc, dev_src, skb2);
#endif
	}

	dev_kfree_skb_any(skb);
	desc->flag |= DESC_FLAG_FASTPATH;
}
#endif

/* check if skb need to be fragmented and fill L2 header */
#ifndef FASTPATH_NETFILTER
#ifdef SUPPORT_PPPOE
static inline int _fp_ip4_pre_send_skb(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				       u_int8_t iface, u_int8_t vlevel, u_int8_t pppoe_out,
				       u_int16_t pppoe_out_id, u_int16_t *vlan, u_int8_t *dst_mac,
				       u_int8_t *src_mac, struct ip4header *ip,
				       struct net_device *dst_dev, int is_lan)
#else
static inline int _fp_ip4_pre_send_skb(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				       u_int8_t iface, u_int8_t vlevel, u_int16_t *vlan,
				       u_int8_t *dst_mac, u_int8_t *src_mac, struct ip4header *ip,
				       struct net_device *dst_dev, int is_lan)
#endif
#else
#ifdef SUPPORT_PPPOE
static inline int _fp_ip4_pre_send_skb(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				       struct net_device *dev_src, u_int8_t vlevel,
				       u_int8_t pppoe_out, u_int16_t pppoe_out_id, u_int16_t *vlan,
				       u_int8_t *dst_mac, u_int8_t *src_mac, struct ip4header *ip,
				       struct net_device *dst_dev, int is_lan)
#else
static inline int _fp_ip4_pre_send_skb(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				       struct net_device *dev_src, u_int8_t vlevel,
				       u_int16_t *vlan, u_int8_t *dst_mac, u_int8_t *src_mac,
				       struct ip4header *ip, struct net_device *dst_dev, int is_lan)
#endif
#endif
{
	int max_frame_size;
	int new_header_len;
	int actual_len;
#ifdef IP_FRAGMENT
	int do_fragment = 0;
#endif
#ifdef SUPPORT_PPPOE
	unsigned short *pppoe_payload_len = NULL;
#endif
	u_int8_t vlevel_sw;

	if (vlevel == HW_VLAN_MAGIC)
		vlevel_sw = 0;
	else
		vlevel_sw = vlevel;

#ifdef SUPPORT_PPPOE
	if (vlevel_sw || pppoe_out) {
		if (pppoe_out) {	/* add PPPoE header */
			new_header_len = 10;
		} else
#else
	if (vlevel_sw) {
#endif
		{		/* add ETYPE_IPv4 */
			new_header_len = 2;
		}
#ifndef SUPPORT_PPPOE
		if (vlevel_sw) {
#endif
			new_header_len += (vlevel_sw << 2);
#ifndef SUPPORT_PPPOE
		}
#endif
		new_header_len += ETH_ALEN * 2;
	} else {
		new_header_len = ETH_HLEN;
	}

	skb->dev = dst_dev;

	max_frame_size = skb->dev->mtu + skb->dev->hard_header_len;
	if ((skb->len - desc->l3_off + new_header_len) > max_frame_size) {
#ifdef IP_FRAGMENT
		if ((ip->ip_off & 0x0040) == 0) {	/* DF is not set */
			do_fragment = 1;
		} else {
			desc->flag |= DESC_FLAG_MTU;
			return 1;
		}
#else
		desc->flag |= DESC_FLAG_MTU;
		return 1;
#endif
	}

	if (is_lan) {
#ifndef FASTPATH_NETFILTER
#ifdef ENABLE_PORTBASE_QOS
		do {
			u_int8_t mark = ifaces[iface].dscp_mark;
			u_int8_t dscp = ip->ip_tos >> 2;

			if ((mark < 64) && (dscp != mark)) {
				ipv4_change_dsfield((struct iphdr *)ip, (__u8) (~0xFC),
						    (mark << 2));
			}
		} while (0);
#endif
#endif

		/* CHECK: compute actual length */
		actual_len = desc->l3_off + ntohs(ip->ip_len);
		if (skb->len > actual_len)
			skb->len = actual_len;
	}

	skb->vlan_tci = 0;
	if (skb->dev->header_ops) {
		if (new_header_len != desc->l3_off) {
			unsigned short *offset2;

			skb_pull_inline(skb, desc->l3_off);
#ifdef SUPPORT_PPPOE
			if (pppoe_out) {	/* add PPPoE header */
				skb_push(skb, 10);
				offset2 = (unsigned short *)(skb->data);
				*offset2++ = ETYPE_PPPoE;	/* ETYPE */
				*offset2++ = htons(0x1100);
				*offset2++ = pppoe_out_id;	/* session id */
				pppoe_payload_len = offset2;
				*offset2++ = htons(skb->len - 8);	/* payload length */
				*offset2 = PPP_IPv4;	/* PPP protocol */
			} else
#endif
			{	/* add ETYPE_IPv4 */
				skb_push(skb, 2);
				offset2 = (unsigned short *)(skb->data);
				*offset2 = ETYPE_IPv4;
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

#ifdef IP_FRAGMENT
	if (unlikely(do_fragment)) {
#ifndef FASTPATH_NETFILTER
#ifdef SUPPORT_PPPOE
		_fp_ip4_xmit_frag(desc, iface, skb, t, ip, new_header_len, pppoe_payload_len);
#else
		_fp_ip4_xmit_frag(desc, iface, skb, t, ip, new_header_len);
#endif
#else
#ifdef SUPPORT_PPPOE
		_fp_ip4_xmit_frag(desc, dev_src, skb, t, ip, new_header_len, pppoe_payload_len);
#else
		_fp_ip4_xmit_frag(desc, dev_src, skb, t, ip, new_header_len);
#endif
#endif
		return 1;

	} else {
#endif
		return 0;
#ifdef IP_FRAGMENT
	}
#endif
}

/* modify IP header (SNAT) */
static inline unsigned _fp_ip4_snat_top(struct nat_tuple *found_nat_tuple, struct ip4header *ip)
{
	unsigned csum_l4;

	ip->ip_sum =
	    _fp_ip4_l3_csum(&csum_l4, &ip->ip_src, &found_nat_tuple->nat_src_ip, &ip->ip_sum);
	ip->ip_src = found_nat_tuple->nat_src_ip;

	return csum_l4;
}

/* modify IP header (DNAT) */
static inline unsigned _fp_ip4_dnat_top(struct nat_tuple *found_nat_tuple, struct ip4header *ip)
{
	unsigned csum_l4;

	ip->ip_sum =
	    _fp_ip4_l3_csum(&csum_l4, &ip->ip_dst, &found_nat_tuple->nat_dst_ip, &ip->ip_sum);
	ip->ip_dst = found_nat_tuple->nat_dst_ip;

	return csum_l4;
}

static inline int _fp_ip4_bottom(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				 struct nat_tuple *found_nat_tuple, struct ip4header *ip)
{
#ifndef FASTPATH_NETFILTER
#ifdef SUPPORT_PPPOE
	return _fp_ip4_pre_send_skb(desc, skb, t, found_nat_tuple->iface_src,
				    found_nat_tuple->vlevel_dst, found_nat_tuple->pppoe_out,
				    found_nat_tuple->pppoe_out_id, found_nat_tuple->vlan_dst,
				    found_nat_tuple->dst_mac,
				    ifaces[found_nat_tuple->iface_dst].dev->dev_addr, ip,
				    ifaces[found_nat_tuple->iface_dst].dev, 1);
#else
	return _fp_ip4_pre_send_skb(desc, skb, t, found_nat_tuple->iface_src,
				    found_nat_tuple->vlevel_dst, found_nat_tuple->vlan_dst,
				    found_nat_tuple->dst_mac,
				    ifaces[found_nat_tuple->iface_dst].dev->dev_addr, ip,
				    ifaces[found_nat_tuple->iface_dst].dev, 1);
#endif
#else
#ifdef SUPPORT_PPPOE
	return _fp_ip4_pre_send_skb(desc, skb, t, found_nat_tuple->dev_src,
				    found_nat_tuple->vlevel_dst, found_nat_tuple->pppoe_out,
				    found_nat_tuple->pppoe_out_id, found_nat_tuple->vlan_dst,
				    found_nat_tuple->dst_mac, found_nat_tuple->dev_dst->dev_addr,
				    ip, found_nat_tuple->dev_dst, 1);
#else
	return _fp_ip4_pre_send_skb(desc, skb, t, found_nat_tuple->dev_src,
				    found_nat_tuple->vlevel_dst, found_nat_tuple->vlan_dst,
				    found_nat_tuple->dst_mac, found_nat_tuple->dev_dst->dev_addr,
				    ip, found_nat_tuple->dev_dst, 1);
#endif
#endif
}

#include "hw_ip4_route.c"
#include "hw_ip4_ipsec.c"
#include "hw_ip4_tcp.c"
#include "hw_ip4_udp.c"
#include "hw_ip4_icmp.c"
#include "hw_ip4_gre.c"
