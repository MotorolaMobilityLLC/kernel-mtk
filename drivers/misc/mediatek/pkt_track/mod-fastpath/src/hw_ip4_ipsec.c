#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

/* check if the packet is belonged to ipsec4 session */
static inline int _fp_is_ip4_ipsec_lan(struct fp_desc *desc, struct sk_buff *skb, void *l3_header,
				       void *l4_header)
{
#ifdef SUPPORT_IPSEC
	struct ip4header *ip = (struct ip4header *) l3_header;
	tcpheader *tcp;
	udpheader *udp;

	struct ipsec_tuple in_tuple;
	struct ipsec_tuple *found_ipsec_tuple;

	in_tuple.saddr.v4[3] = 0;
	in_tuple.daddr.v4[3] = 0;
	in_tuple.saddr.v4[0] = ip->ip_src;
	in_tuple.daddr.v4[0] = ip->ip_dst;
	in_tuple.xfrm_proto = 0;
	in_tuple.spi = 0;
	in_tuple.traffic_proto = ip->ip_p;
	if (ip->ip_p == IPPROTO_TCP) {
		tcp = (tcpheader *) l4_header;
		in_tuple.traffic_para.port.src = tcp->th_sport;
		in_tuple.traffic_para.port.dst = tcp->th_dport;
	} else if (ip->ip_p == IPPROTO_UDP) {
		udp = (udpheader *) l4_header;
		in_tuple.traffic_para.port.src = udp->uh_sport;
		in_tuple.traffic_para.port.dst = udp->uh_dport;
	} else {
		in_tuple.traffic_para.all = 0;
	}

	found_ipsec_tuple = get_ip4_ipsec_tuple(&in_tuple, desc->flag | DESC_FLAG_ESP_ENCODE);
	if (likely(found_ipsec_tuple)) {
		fp_cb *cb = NULL;
		unsigned long old_refdst;
		/*struct xfrm_state *xs; */
		*(u_int32_t *) (&skb->cb[XFRM_SKB_CB_EXT_OFFSET]) =
		    (uintptr_t) (&(found_ipsec_tuple->inner_tuple));

		skb_pull_inline(skb, desc->l3_off);
		old_refdst = skb->_skb_refdst;
		skb->_skb_refdst = found_ipsec_tuple->xfrm_refdst;
		dst_hold((struct dst_entry *)skb->_skb_refdst);
#ifndef FASTPATH_NETFILTER
		skb->dev = ifaces[found_ipsec_tuple->iface_dst].dev;
#else
		skb->dev = found_ipsec_tuple->dev_dst;
#endif
		if (skb->dev->reg_state != NETREG_REGISTERED) {
			desc->flag = DESC_FLAG_NOMEM;
			DEC_REF_IPSEC_TUPLE(found_ipsec_tuple);
			return 1;
		}
#if 0
		desc->flag |= (DESC_FLAG_ESP_ENCODE | DESC_FLAG_FASTPATH);
#else
		cb = (fp_cb *) (&skb->cb[48]);
		cb->flag |= DESC_FLAG_ESP_ENCODE;
		desc->flag |= DESC_FLAG_FASTPATH;
#endif

		if (xfrm_output_mtk(skb)) {
			/* skb had released */
			del_ipsec_tuple(found_ipsec_tuple);
		}
		DEC_REF_IPSEC_TUPLE(found_ipsec_tuple);
		return 1;
	}
#endif
	/* the packet is not handled by ipsec4 */
	return 0;
}

static inline void _fp_ip4_ipsec_wan(struct fp_desc *desc, struct sk_buff *skb, struct ip4header *ip,
				     u_int32_t xfrm_spi)
{
	struct ipsec_tuple in_tuple;
	struct ipsec_tuple *found_ipsec_tuple;

	desc->flag |= DESC_FLAG_ESP_DECODE;

	/* find ipsec_tuple */
	in_tuple.saddr.v4[3] = 0;
	in_tuple.daddr.v4[3] = 0;
	in_tuple.saddr.v4[0] = ip->ip_src;
	in_tuple.daddr.v4[0] = ip->ip_dst;
	in_tuple.xfrm_proto = ip->ip_p;
	in_tuple.spi = xfrm_spi;
	in_tuple.traffic_proto = 0;
	in_tuple.traffic_para.all = 0;

	found_ipsec_tuple = get_ip4_ipsec_tuple(&in_tuple, desc->flag);
	if (found_ipsec_tuple) {
		struct xfrm_state *xs;

		xs = (struct xfrm_state *)found_ipsec_tuple->xfrm_refdst;
		if (xs->km.state != XFRM_STATE_VALID) {
			DEC_REF_IPSEC_TUPLE(found_ipsec_tuple);
			del_ipsec_tuple(found_ipsec_tuple);
			return;
		}

		*(u_int32_t *) (&skb->cb[XFRM_SKB_CB_EXT_OFFSET]) =
		    (uintptr_t) (&(found_ipsec_tuple->inner_tuple));
		*(u_int32_t *) (&skb->cb[XFRM_SKB_CB_DST_OFFSET]) = found_ipsec_tuple->xfrm_refdst;

#ifndef FASTPATH_NETFILTER
		skb->dev = ifaces[found_ipsec_tuple->iface_dst].dev;
#else
		skb->dev = found_ipsec_tuple->dev_dst;
#endif
		if (skb->dev->reg_state != NETREG_REGISTERED) {
			desc->flag = DESC_FLAG_NOMEM;
			DEC_REF_IPSEC_TUPLE(found_ipsec_tuple);
			return;
		}
		/* todo : go to xfrm_rcv directly */
		skb_pull_inline(skb, desc->l4_off);	/* remove mac header, ip_header */
		skb_reset_transport_header(skb);
		desc->flag |= DESC_FLAG_FASTPATH;
		if (xfrm4_rcv(skb) < 0)
			pr_err("%s %d xfrm_rcv return failed\n", __func__, __LINE__);

		DEC_REF_IPSEC_TUPLE(found_ipsec_tuple);
	}
}

static inline void fp_ip4_ipsec_ah_wan(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				       struct interface *iface, void *l3_header, void *l4_header)
{
	u_int32_t xfrm_spi;
	struct ip4header *ip = (struct ip4header *)l3_header;
	struct ip_auth_hdr *ahh = NULL;

	ahh = (struct ip_auth_hdr *)l4_header;
	xfrm_spi = ahh->spi;

	_fp_ip4_ipsec_wan(desc, skb, ip, xfrm_spi);
}

static inline void fp_ip4_ipsec_esp_wan(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
					struct interface *iface, void *l3_header, void *l4_header)
{
	u_int32_t xfrm_spi;
	struct ip4header *ip = (struct ip4header *)l3_header;
	struct ip_esp_hdr *esph = NULL;

	esph = (struct ip_esp_hdr *)(skb->data + desc->l4_off);
	xfrm_spi = esph->spi;

	_fp_ip4_ipsec_wan(desc, skb, ip, xfrm_spi);
}
