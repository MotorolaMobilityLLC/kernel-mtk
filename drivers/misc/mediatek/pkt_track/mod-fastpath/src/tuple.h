#ifndef _TUPLE_H
#define _TUPLE_H

/* #include <asm/atomic.h> */
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include "fp_config.h"

#define MAX_VLAN_LEVEL	2
#define HW_VLAN_MAGIC  (MAX_VLAN_LEVEL + 2)

#define FASTPATH_QUEUED_SKB_NUM_THRES 16

enum {
	TUPLE_ACTION_SNAT = 0x00000001,
	TUPLE_ACTION_DNAT = 0x00000002,
	TUPLE_ACTION_BRIDGE = 0x00000004,
	TUPLE_ACTION_ROUTER = 0x00000008,
};

enum tuple_state_e {
	TUPLE_STATE_VALID,
	TUPLE_STATE_ADDING,
	TUPLE_STATE_DELETING,
	TUPLE_STATE_INVALID,
};

struct tuple {
	union {
		struct {
#ifdef SUPPORT_PPPOE
			u_int8_t pppoe_out;
			u_int16_t pppoe_out_id;
#endif
			u_int32_t src;	/*src */
			u_int32_t dst;	/*dst */
			union {
				u_int16_t all;
				struct {
					u_int16_t port;
				} tcp;
				struct {
					u_int16_t port;
				} udp;
				struct {
					u_int16_t callid;
				} gre;
			} s;
			union {
				u_int16_t all;
				struct {
					u_int16_t port;
				} tcp;
				struct {
					u_int16_t port;
				} udp;
				struct {
					u_int16_t callid;
				} gre;
			} d;
			u_int8_t proto;
		} nat;
		struct {
			union {
				struct {
					u_int8_t dst_unused[2];
					u_int8_t dst_mac[6];
				} __attribute__ ((__packed__));
				struct {
					u_int16_t dst_unused2;
					u_int16_t dst_mac1;
					u_int32_t dst_mac2;
				};
			};
			u_int8_t vlevel_src;
			u_int16_t vlan_src[MAX_VLAN_LEVEL];
		} bridge;
	};
#ifndef FASTPATH_NETFILTER
	u_int8_t iface_in;
#else
	struct net_device *dev_in;
#endif
	u_int32_t action;
};

struct nat_tuple {
	struct list_head list;

	u_int32_t src_ip;
	u_int32_t dst_ip;
	u_int32_t nat_src_ip;
	u_int32_t nat_dst_ip;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
		struct {
			u_int16_t callid;
		} gre;
	} src;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
		struct {
			u_int16_t callid;
		} gre;
	} dst;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
		struct {
			u_int16_t callid_in;
			u_int16_t callid_out;
		} gre;
	} nat_src;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
		struct {
			u_int16_t callid_in;
			u_int16_t callid_out;
		} gre;
	} nat_dst;

	u_int8_t proto;

#ifndef FASTPATH_NETFILTER
	u_int8_t iface_src;
	u_int8_t iface_dst;
#else
	struct net_device *dev_src;
	struct net_device *dev_dst;
#endif
	u_int8_t src_mac[6];
	u_int8_t dst_mac[6];

	u_int8_t vlevel_src;
	u_int8_t vlevel_dst;
	u_int16_t vlan_src[MAX_VLAN_LEVEL];
	u_int16_t vlan_dst[MAX_VLAN_LEVEL];

#ifdef SUPPORT_PPPOE
	u_int8_t pppoe_out;
	u_int16_t pppoe_out_id;
#endif

	struct timer_list timeout_used;
	struct timer_list timeout_unused;
	struct timer_list timeout_ack;

	uint32_t ul_ack;
	struct sk_buff *ul_ack_skb;
	uint32_t ack_agg_enable;
	spinlock_t ack_lock;

	enum tuple_state_e state;

	atomic_t ref_count;
	unsigned long last;
};

struct bridge_tuple {
	struct list_head list;
	union {
		struct {
			u_int8_t dst_unused[2];
			u_int8_t dst_mac[6];
		} __attribute__ ((__packed__));
		struct {
			u_int16_t dst_unused2;
			u_int16_t dst_mac1;
			u_int32_t dst_mac2;
		};
	};

#ifndef FASTPATH_NETFILTER
	u_int8_t iface_src;
	u_int8_t iface_dst;
#else
	struct net_device *dev_src;
	struct net_device *dev_dst;
#endif

	u_int8_t vlevel_src;
	u_int8_t vlevel_dst;
	u_int16_t vlan_src[MAX_VLAN_LEVEL];
	u_int16_t vlan_dst[MAX_VLAN_LEVEL];

	struct timer_list timeout_used;
	struct timer_list timeout_unused;

	atomic_t ref_count;
	unsigned long last;
};

struct router_tuple {
	struct list_head list;
	union {
		struct {
			u_int8_t dst_unused[2];
			u_int8_t smac[6];
		} __attribute__ ((__packed__));
	};
	union {
		struct {
			u_int8_t dst_unused3[2];
			u_int8_t dmac[6];
		} __attribute__ ((__packed__));
	};

	struct in6_addr saddr;
	struct in6_addr daddr;

#ifndef FASTPATH_NETFILTER
	u_int8_t iface_src;
	u_int8_t iface_dst;
#else
	struct net_device *dev_src;
	struct net_device *dev_dst;
#endif

	u_int8_t vlevel_src;
	u_int8_t vlevel_dst;
	u_int16_t vlan_src[MAX_VLAN_LEVEL];
	u_int16_t vlan_dst[MAX_VLAN_LEVEL];

	struct timer_list timeout_used;
	struct timer_list timeout_unused;

	/* for ack aggregation */
	struct timer_list timeout_ack;
	uint32_t ul_ack;
	struct sk_buff *ul_ack_skb;
	uint32_t ack_agg_enable;
	spinlock_t ack_lock;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
	} in;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
	} out;
	u_int8_t proto;

	enum tuple_state_e state;

	atomic_t ref_count;
	unsigned long last;
};

struct ipsec_inner_ipmac {
	struct {
		u_int8_t eth_header[ETH_HLEN + MAX_VLAN_LEVEL * 4];
		u_int8_t eth_length;
		u_int8_t hw_vlan;
		u_int16_t hw_vlan_id;
#if ((32 - (ETH_HLEN + MAX_VLAN_LEVEL * 4)) < 0)
#error
#endif
		u_int8_t pad[32 - (ETH_HLEN + MAX_VLAN_LEVEL * 4)];
	} eth;
	union {
		u_int32_t v4[4];
		struct in6_addr v6;
	} inner_saddr;
	union {
		u_int32_t v4[4];
		struct in6_addr v6;
	} inner_daddr;
};

struct ipsec_tuple {
	struct list_head list;

	union {
		u_int32_t v4[4];
		struct in6_addr v6;
	} saddr;
	union {
		u_int32_t v4[4];
		struct in6_addr v6;
	} daddr;
	u_int8_t xfrm_proto;
	u_int32_t spi;

	union {
		u_int32_t all;
		struct {
			u_int16_t src;
			u_int16_t dst;
		} port;
		struct {
			u_int8_t type;
			u_int8_t code;
			u_int16_t rfu;
		} field;
	} traffic_para;
	u_int8_t traffic_proto;

	uintptr_t xfrm_refdst;
#ifndef FASTPATH_NETFILTER
	u_int8_t iface_src;
	u_int8_t iface_dst;
#else
	struct net_device *dev_src;
	struct net_device *dev_dst;
#endif

	struct ipsec_inner_ipmac inner_tuple;

	struct timer_list timeout_used;
	struct timer_list timeout_unused;

	atomic_t ref_count;
	unsigned long last;
};

/*have to be power of 2*/
#define NAT_TUPLE_HASH_SIZE (MD_DIRECT_TETHERING_RULE_NUM)
#define BRIDGE_TUPLE_HASH_SIZE	(MD_DIRECT_TETHERING_RULE_NUM)
#define ROUTER_TUPLE_HASH_SIZE	(MD_DIRECT_TETHERING_RULE_NUM)
#define IPSEC_TUPLE_HASH_SIZE	(MD_DIRECT_TETHERING_RULE_NUM)

#define HASH_TUPLE_TCPUDP(t)                                     \
	(jhash_3words((t)->nat.src, ((t)->nat.dst ^ (t)->nat.proto), \
	 ((t)->nat.s.all | ((t)->nat.d.all << 16)),                  \
	 nat_tuple_hash_rnd) & (NAT_TUPLE_HASH_SIZE - 1))

#define HASH_NAT_TUPLE_TCPUDP(t)								\
	(jhash_3words((t)->src_ip, ((t)->dst_ip ^ (t)->proto),		\
	 ((t)->src.all | ((t)->dst.all << 16)),						\
	 nat_tuple_hash_rnd) & (NAT_TUPLE_HASH_SIZE - 1))

#define HASH_NAT_TUPLE_GRE(t)										\
	(jhash_3words((t)->src_ip, ((t)->dst_ip ^ (t)->proto),			\
	 (t)->src.all, nat_tuple_hash_rnd) & (NAT_TUPLE_HASH_SIZE - 1))

#define INC_REF_NAT_TUPLE(t)			atomic_inc(&(t)->ref_count)
#define DEC_REF_NAT_TUPLE(t)							\
{														\
	if (atomic_dec_and_test(&(t)->ref_count)) {			\
		/*printk("nat tuple need to be removed\n");*/	\
		kmem_cache_free(nat_tuple_cache, t);			\
	}													\
}

#ifndef FASTPATH_NETFILTER
#define HASH_BRIDGE_TUPLE(t)                                                        \
	(jhash_3words(((t)->iface_src << 24 | (t)->vlevel_src << 16 | (t)->dst_mac1),   \
	 ((t)->vlan_src[0] << 16 | (t)->vlan_src[1]),                                   \
	 (t)->dst_mac2, bridge_tuple_hash_rnd) & (BRIDGE_TUPLE_HASH_SIZE - 1))

#define HASH_TUPLE(t)                                                                           \
	(jhash_3words(((t)->iface_in << 24 | (t)->bridge.vlevel_src << 16 | (t)->bridge.dst_mac1),  \
	 ((t)->bridge.vlan_src[0] << 16 | (t)->bridge.vlan_src[1]),                                 \
	 (t)->bridge.dst_mac2, bridge_tuple_hash_rnd) & (BRIDGE_TUPLE_HASH_SIZE - 1))

#else
#define HASH_BRIDGE_TUPLE(t)                                                                        \
	(jhash_3words((((uintptr_t)(t)->dev_src & 0xff) << 24 | (t)->vlevel_src << 16 | (t)->dst_mac1), \
	 ((t)->vlan_src[0] << 16 | (t)->vlan_src[1]),                                                   \
	 (t)->dst_mac2, bridge_tuple_hash_rnd) & (BRIDGE_TUPLE_HASH_SIZE - 1))
#define HASH_TUPLE(t)                                                                                             \
	(jhash_3words((((uintptr_t)(t)->dev_in & 0xff) << 24 | (t)->bridge.vlevel_src << 16 | (t)->bridge.dst_mac1),  \
	 ((t)->bridge.vlan_src[0] << 16 | (t)->bridge.vlan_src[1]),                                                   \
	 (t)->bridge.dst_mac2, bridge_tuple_hash_rnd) & (BRIDGE_TUPLE_HASH_SIZE - 1))
#endif

#define INC_REF_BRIDGE_TUPLE(t)			atomic_inc(&(t)->ref_count)
#define DEC_REF_BRIDGE_TUPLE(t)							\
{														\
	if (atomic_dec_and_test(&(t)->ref_count)) {			\
		/*printk("bridge tuple need to be removed\n");*/\
		kmem_cache_free(bridge_tuple_cache, t);			\
	}													\
}

#define HASH_ROUTER_TUPLE_TCPUDP(t)                                                             \
	(jhash_3words(((t)->saddr.s6_addr32[0] ^ (t)->saddr.s6_addr32[3]),                          \
	 ((t)->daddr.s6_addr32[0] ^ (t)->daddr.s6_addr32[3]), ((t)->in.all | ((t)->out.all << 16)), \
	 router_tuple_hash_rnd) & (ROUTER_TUPLE_HASH_SIZE - 1))

#define HASH_ROUTER_TUPLE(t)							\
	(jhash_3words((t)->saddr.s6_addr32[2],				\
	 (t)->daddr.s6_addr32[2], (t)->daddr.s6_addr32[3],	\
	 router_tuple_hash_rnd) & (ROUTER_TUPLE_HASH_SIZE - 1))

#define INC_REF_ROUTER_TUPLE(t)			atomic_inc(&(t)->ref_count)
#define DEC_REF_ROUTER_TUPLE(t)							\
{														\
	if (atomic_dec_and_test(&(t)->ref_count)) {			\
		/*printk("router tuple need to be removed\n");*/\
		kmem_cache_free(router_tuple_cache, t);			\
	}													\
}

#if 0
#define HASH_IPSEC_TUPLE(t)                                                     \
	(jhash_3words((t)->saddr.v4[0], (t)->daddr.v4[0],                           \
	 (t)->spi ^ (t)->xfrm_proto ^ (t)->traffic_proto ^ (t)->traffic_para.all,   \
	 ipsec_tuple_hash_rnd) & (IPSEC_TUPLE_HASH_SIZE - 1))
#endif

#define HASH_IPSEC_TUPLE(t)                                                     \
	(jhash_3words(((t)->saddr.v6.s6_addr32[0] ^ (t)->saddr.v6.s6_addr32[3]),    \
	 ((t)->daddr.v6.s6_addr32[0] ^ (t)->daddr.v6.s6_addr32[3]),                 \
	 (t)->spi ^ (t)->xfrm_proto ^ (t)->traffic_proto ^ (t)->traffic_para.all,   \
	 ipsec_tuple_hash_rnd) & (IPSEC_TUPLE_HASH_SIZE - 1))

#define INC_REF_IPSEC_TUPLE(t)			atomic_inc(&(t)->ref_count)
#define DEC_REF_IPSEC_TUPLE(t)							\
{														\
	if (atomic_dec_and_test(&(t)->ref_count)) {			\
		/*printk("router tuple need to be removed\n");*/\
		kmem_cache_free(ipsec_tuple_cache, t);			\
	}													\
}

void init_nat_tuple(void);
bool add_nat_tuple(struct nat_tuple *t);
void del_nat_tuple(struct nat_tuple *t);
void ack_nat_tuple(struct nat_tuple *t);
void del_all_nat_tuple(void);
void del_all_tuple(void);
void timeout_nat_tuple(struct nat_tuple *t);

void init_bridge_tuple(void);
void add_bridge_tuple(struct bridge_tuple *t);
void del_bridge_tuple(struct bridge_tuple *t);
/*bridge_tuple* get_bridge_tuple(tuple *t);*/

void init_router_tuple(void);
void add_router_tuple(struct router_tuple *t);
void del_router_tuple(struct router_tuple *t);
void ack_router_tuple(struct router_tuple *t);
void timeout_router_tuple(struct router_tuple *t);
/*router_tuple* get_router_tuple(router_tuple *t);*/

void del_all_nat_tuple(void);
void del_all_bridge_tuple(void);
void del_all_router_tuple(void);

void init_ipsec_tuple(void);
void add_ipsec_tuple(struct ipsec_tuple *t, u_int32_t flag);
void del_ipsec_tuple(struct ipsec_tuple *t);
struct ipsec_tuple *get_ipsec_tuple(struct ipsec_tuple *t, u_int32_t flag);
void del_all_ipsec_tuple(void);

#endif
