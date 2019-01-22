#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

/* TODO */
#if 0
static inline void _fp_ip4_gre_snat(fp_desc *desc, struct sk_buff *skb, tuple *t,
				    nat_tuple *found_nat_tuple, ip4header *ip, greheader *gre)
{
	unsigned csum_l4;
	/*unsigned csum_tmp; */

	/* modify IP checksum */
	csum_l4 = _fp_ip4_snat_top(found_nat_tuple, ip);

	/* modify GRE callid */
	gre->gre_callid = found_nat_tuple->nat.gre.callid_in;

	_fp_ip4_snat_bottom(desc, skb, t, found_nat_tuple, ip);
}

static inline void _fp_ip4_gre_dnat(fp_desc *desc, struct sk_buff *skb, tuple *t,
				    nat_tuple *found_nat_tuple, ip4header *ip, greheader *gre)
{
	unsigned csum_l4;

	/* modify IP checksum */
	csum_l4 = _fp_ip4_dnat_top(found_nat_tuple, ip);

	/* modify GRE callid */
	gre->gre_callid = found_nat_tuple->nat.gre.callid_out;

	_fp_ip4_dnat_bottom(desc, skb, t, found_nat_tuple, ip);
}

static inline void fp_ip4_gre_lan(fp_desc *desc, struct sk_buff *skb, tuple *t, interface *iface,
				  void *l3_header, void *l4_header)
{
	nat_tuple *found_nat_tuple;
	ip4header *ip = (ip4header *) l3_header;
	greheader *gre = (greheader *) (skb->data + desc->l4_off);

	/* check if the packet should be handled by ipsec4 first */
	if (unlikely(fastpath_ipsec && _fp_is_ip4_ipsec_lan(desc, skb, l3_header, l4_header)))
		return;

	if (gre->gre_p == htons(0x880b)) {
		t->nat.s.gre.callid = gre->gre_callid;
	} else {
		desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
		return;
	}

	found_nat_tuple = get_nat_tuple_ip4_gre_lan(t);
	if (likely(found_nat_tuple)) {
		if (likely(!(t->action & TUPLE_ACTION_ROUTER))) {
			_fp_ip4_gre_snat(desc, skb, t, found_nat_tuple, ip, gre);	/* SNAT: LAN to WAN */
		} else {	/* ROUTE */
			/*fp_ip4_route_lan(desc, skb, found_nat_tuple); //ROUTE: LAN to WAN */
			fp_ip4_route_lan(desc, skb, t, found_nat_tuple, ip, NULL);	/* ROUTE: LAN to WAN */
		}
		DEC_REF_NAT_TUPLE(found_nat_tuple);
		return;

	} else {		/* not found nat */
		desc->flag |= DESC_FLAG_TRACK_NAT;
		return;
	}
}

static inline void fp_ip4_gre_wan(fp_desc *desc, struct sk_buff *skb, tuple *t, interface *iface,
				  void *l3_header, void *l4_header)
{
	nat_tuple *found_nat_tuple;
	ip4header *ip = (ip4header *) l3_header;
	greheader *gre = (greheader *) (skb->data + desc->l4_off);

	if (gre->gre_p == htons(0x880b)) {
		t->nat.s.gre.callid = gre->gre_callid;
	} else {
		desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
		return;
	}

	found_nat_tuple = get_nat_tuple_ip4_gre_wan(t);
	if (likely(found_nat_tuple)) {
		if (likely(!(t->action & TUPLE_ACTION_ROUTER))) {	/* DNAT */
			_fp_ip4_gre_dnat(desc, skb, t, found_nat_tuple, ip, gre);	/* DNAT: WAN to LAN */
		} else {	/* ROUTE */
			/*fp_ip4_route_wan(desc, skb, found_nat_tuple); //ROUTE: WAN to LAN */
			fp_ip4_route_wan(desc, skb, t, found_nat_tuple, ip);	/* ROUTE: WAN to LAN */
		}
		DEC_REF_NAT_TUPLE(found_nat_tuple);
		return;

	} else {		/* not found nat */
#ifdef SUPPORT_PPPOE
		if (t->nat.pppoe_out)
			return;
#endif
		desc->flag |= DESC_FLAG_TRACK_NAT;
		return;
	}
}
#endif
