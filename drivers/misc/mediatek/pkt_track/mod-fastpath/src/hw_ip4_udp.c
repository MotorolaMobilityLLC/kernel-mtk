#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

#ifndef FASTPATH_NETFILTER
static inline int _fp_ip46_udp_limit(struct fp_desc *desc, struct sk_buff *skb, struct interface *iface)
{
	if (unlikely(fastpath_limit_udp)) {
		if (fastpath_limit_size == 0 || skb->len <= fastpath_limit_size) {
			iface->udp_count++;
			if (iface->udp_count > fastpath_limit_udp) {
				desc->flag = DESC_FLAG_NOMEM;
				return 1;
			}
		}
	}
	return 0;
}
#endif

static inline void _fp_ip4_udp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
			       struct nat_tuple *found_nat_tuple, struct ip4header *ip, struct udpheader *udp)
{
	unsigned csum_l4;

	/* modify IP checksum */
	csum_l4 = _fp_ip4_snat_top(found_nat_tuple, ip);
	/* modify UDP checksum */
	if (udp->uh_check) {
		udp->uh_check =
		    _fp_l4_csum(&csum_l4, &udp->uh_sport, &found_nat_tuple->nat_src.udp.port,
				&udp->uh_check);
		if (!udp->uh_check)
			udp->uh_check = 0xffff;
	}
	udp->uh_sport = found_nat_tuple->nat_src.udp.port;

	/* modify IP checksum */
	csum_l4 = _fp_ip4_dnat_top(found_nat_tuple, ip);
	/* modify UDP checksum */
	if (udp->uh_check) {
		udp->uh_check =
		    _fp_l4_csum(&csum_l4, &udp->uh_dport, &found_nat_tuple->nat_dst.udp.port,
				&udp->uh_check);
		if (!udp->uh_check)
			udp->uh_check = 0xffff;
	}
	udp->uh_dport = found_nat_tuple->nat_dst.udp.port;

	if (unlikely(_fp_ip4_bottom(desc, skb, t, found_nat_tuple, ip)))
		return;

#ifndef FASTPATH_NETFILTER
	_fp_xmit(desc, found_nat_tuple->iface_src, skb);
#else
	_fp_xmit(desc, found_nat_tuple->dev_src, skb);
#endif
}

static inline void fp_ip4_udp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t, struct interface *iface,
			      void *l3_header, void *l4_header)
{
/* struct ip4header *ip = (struct ip4header *)l3_header; */
	struct udpheader *udp = (struct udpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to udp limitation */
	if (unlikely(_fp_ip46_udp_limit(desc, skb, iface)))
		return;
#endif

	/* check if the packet should be handled by ipsec4 first */
/* if(unlikely(fastpath_ipsec && _fp_is_ip4_ipsec_lan(desc, skb, l3_header, l4_header))) return; */

	t->nat.s.udp.port = udp->uh_sport;
	t->nat.d.udp.port = udp->uh_dport;

	desc->flag |= DESC_FLAG_TRACK_NAT;
}

#ifdef SUPPORT_IOC
static inline void fp_ip4_upd_ioc(void)
{
}
#endif
