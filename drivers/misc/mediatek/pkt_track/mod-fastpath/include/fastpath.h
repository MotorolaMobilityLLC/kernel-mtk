#ifndef _FASTPATH_H
#define _FASTPATH_H

#include <linux/netdevice.h>

bool fastpath_is_support_dev(const char *dev_name);

/* Delete tuple API */
void del_nat_tuple(void *t);
void del_router_tuple(void *t);
void timeout_router_tuple(void *t);
void timeout_nat_tuple(void *t);

void fastpath_register(int iface, struct net_device *dev, int wan);
int fastpath_dyn_register(struct net_device *dev, int wan);
void fastpath_unregister(int iface);
int fastpath_in(int iface, struct sk_buff *skb);
void fastpath_in_list(int iface, struct sk_buff_head *skb_list, struct sk_buff_head *skb_remain_list);
int fastpath_in_noarp(int iface, struct sk_buff *skb);
void fastpath_in_noarp_list(int iface, struct sk_buff_head *skb_list, struct sk_buff_head *skb_remain_list);
void fastpath_out(int iface, struct sk_buff *skb);

int fastpath_in_nf(int iface, struct sk_buff *skb);
void fastpath_out_nf(int iface, struct sk_buff *skb, const struct net_device *out);

#endif
