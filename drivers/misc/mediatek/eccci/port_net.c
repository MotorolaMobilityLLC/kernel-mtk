#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/sockios.h>
#include <mt-plat/mt_ccci_common.h>
#include "ccci_config.h"
#include "ccci_core.h"
#include "ccci_bm.h"
#include "port_net.h"

#ifdef PORT_NET_TRACE
#define CREATE_TRACE_POINTS
#include "port_net_events.h"
#endif

#define NET_DAT_TXQ_INDEX(p) ((p)->modem->md_state == EXCEPTION?(p)->txq_exp_index:(p)->txq_index)
#define NET_ACK_TXQ_INDEX(p) ((p)->modem->md_state == EXCEPTION?(p)->txq_exp_index:((p)->txq_exp_index&0x0F))

#ifdef CCMNI_U
int ccci_get_ccmni_channel(int md_id, int ccmni_idx, struct ccmni_ch *channel)
{
	int ret = 0;

	switch (ccmni_idx) {
	case 0:
		channel->rx = CCCI_CCMNI1_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI1_TX;
		channel->tx_ack = 0xFF;
		break;
	case 1:
		channel->rx = CCCI_CCMNI2_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI2_TX;
		channel->tx_ack = 0xFF;
		break;
	case 2:
		channel->rx = CCCI_CCMNI3_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI3_TX;
		channel->tx_ack = 0xFF;
		break;
	case 3:
		channel->rx = CCCI_CCMNI4_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI4_TX;
		channel->tx_ack = 0xFF;
		break;
	case 4:
		channel->rx = CCCI_CCMNI5_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI5_TX;
		channel->tx_ack = 0xFF;
		break;
	case 5:
		channel->rx = CCCI_CCMNI6_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI6_TX;
		channel->tx_ack = 0xFF;
		break;
	case 6:
		channel->rx = CCCI_CCMNI7_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI7_TX;
		channel->tx_ack = 0xFF;
		break;
	case 7:
		channel->rx = CCCI_CCMNI8_RX;
		channel->rx_ack = 0xFF;
		channel->tx = CCCI_CCMNI8_TX;
		channel->tx_ack = 0xFF;
		break;
	default:
		CCCI_ERR_MSG(md_id, NET, "invalid ccmni index=%d\n", ccmni_idx);
		ret = -1;
		break;
	}

	return ret;
}

int ccmni_send_pkt(int md_id, int tx_ch, void *data)
{
	struct ccci_modem *md = ccci_get_modem_by_id(md_id);
	struct ccci_port *port = NULL;
	/* struct ccci_request *req = NULL; */
	struct ccci_header *ccci_h;
	struct sk_buff *skb = (struct sk_buff *)data;
	int tx_ch_to_port, tx_queue;
	int ret;
#ifdef PORT_NET_TRACE
	unsigned long long send_time = 0;
	unsigned long long get_port_time = 0;
	unsigned long long total_time = 0;

	total_time = sched_clock();
#endif
	if (!md)
		return CCMNI_ERR_TX_INVAL;
	if (unlikely(md->md_state != READY))
		return CCMNI_ERR_MD_NO_READY;

	if (tx_ch == CCCI_CCMNI1_DL_ACK)
		tx_ch_to_port = CCCI_CCMNI1_TX;
	else if (tx_ch == CCCI_CCMNI2_DL_ACK)
		tx_ch_to_port = CCCI_CCMNI2_TX;
	else if (tx_ch == CCCI_CCMNI3_DL_ACK)
		tx_ch_to_port = CCCI_CCMNI3_TX;
	else
		tx_ch_to_port = tx_ch;
#ifdef PORT_NET_TRACE
	get_port_time = sched_clock();
#endif
	port = md->ops->get_port_by_channel(md, tx_ch_to_port);
#ifdef PORT_NET_TRACE
	get_port_time = sched_clock() - get_port_time;
#endif
	if (!port) {
		CCCI_ERR_MSG(0, NET, "port==NULL\n");
		return CCMNI_ERR_TX_INVAL;
	}
	/* req_alloc_time=sched_clock(); */
	/* req = ccci_alloc_req(OUT, -1, 1, 0); */
	/* req_alloc_time=sched_clock()-req_alloc_time; */
	/* if(!req) { */
	/* return CCMNI_ERR_TX_BUSY; */
	/* } */
	if (tx_ch == CCCI_CCMNI1_DL_ACK || tx_ch == CCCI_CCMNI2_DL_ACK || tx_ch == CCCI_CCMNI3_DL_ACK)
		tx_queue = NET_ACK_TXQ_INDEX(port);
	else
		tx_queue = NET_DAT_TXQ_INDEX(port);

	/* req->skb = skb; */
	/* req->policy = FREE; */
	ccci_h = (struct ccci_header *)skb_push(skb, sizeof(struct ccci_header));
	ccci_h = (struct ccci_header *)skb->data;
	ccci_h->channel = tx_ch;
	ccci_h->data[0] = 0;
	ccci_h->data[1] = skb->len;	/* as skb->len already included ccci_header after skb_push */
/* #ifndef FEATURE_SEQ_CHECK_EN */
/* ccci_h->reserved = nent->tx_seq_num++; */
/* #else */
	ccci_h->reserved = 0;
/* #endif */
	CCCI_DBG_MSG(md_id, NET, "port %s send txq=%d: %08X, %08X, %08X, %08X\n", port->name, tx_queue,
		     ccci_h->data[0], ccci_h->data[1], ccci_h->channel, ccci_h->reserved);
#ifdef PORT_NET_TRACE
	send_time = sched_clock();
#endif
	ret = port->modem->ops->send_request(port->modem, tx_queue, NULL, skb);
#ifdef PORT_NET_TRACE
	send_time = sched_clock() - send_time;
#endif
	if (ret) {
		skb_pull(skb, sizeof(struct ccci_header));
			/* undo header, in next retry, we'll reserve header again */
		ret = CCMNI_ERR_TX_BUSY;
	} else {
		ret = CCMNI_ERR_TX_OK;
	}
#ifdef PORT_NET_TRACE
	if (ret == CCMNI_ERR_TX_OK) {
		total_time = sched_clock() - total_time;
		trace_port_net_tx(md_id, tx_queue, tx_ch, (unsigned int)get_port_time, (unsigned int)send_time,
				  (unsigned int)(total_time));
	} else {
		trace_port_net_error(port->modem->index, tx_queue, port->tx_ch, port->tx_busy_count, __LINE__);
	}
#endif
	return ret;
}

int ccmni_napi_poll(int md_id, int rx_ch, struct napi_struct *napi, int weight)
{
	return 0;
}

struct ccmni_ccci_ops eccci_ccmni_ops = {
	.ccmni_ver = CCMNI_DRV_V0,
	.ccmni_num = 8,
	.name = "ccmni",
	.md_ability = MODEM_CAP_DATA_ACK_DVD,
	.irat_md_id = -1,
	.napi_poll_weigh = NAPI_POLL_WEIGHT,
	.send_pkt = ccmni_send_pkt,
	.napi_poll = ccmni_napi_poll,
	.get_ccmni_ch = ccci_get_ccmni_channel,
};

struct ccmni_ccci_ops eccci_cc3mni_ops = {
	.ccmni_ver = CCMNI_DRV_V0,
	.ccmni_num = 3,
	.name = "cc3mni",
#if defined CONFIG_MTK_IRAT_SUPPORT
#if defined CONFIG_MTK_C2K_SLOT2_SUPPORT
	.md_ability = MODEM_CAP_CCMNI_IRAT | MODEM_CAP_TXBUSY_STOP | MODEM_CAP_WORLD_PHONE,
#else
	.md_ability = MODEM_CAP_CCMNI_IRAT | MODEM_CAP_TXBUSY_STOP,
#endif
	.irat_md_id = MD_SYS1,
#else
	.md_ability = MODEM_CAP_TXBUSY_STOP,
	.irat_md_id = -1,
#endif
	.napi_poll_weigh = 0,
	.send_pkt = ccmni_send_pkt,
	.napi_poll = ccmni_napi_poll,
	.get_ccmni_ch = ccci_get_ccmni_channel,
};

#endif

#define  IPV4_VERSION 0x40
#define  IPV6_VERSION 0x60
#define SIOCSTXQSTATE (SIOCDEVPRIVATE + 0)

struct netdev_entity {
	struct napi_struct napi;
	struct net_device *ndev;
#ifndef FEATURE_SEQ_CHECK_EN
	unsigned int rx_seq_num;
	unsigned int tx_seq_num;
#endif
	struct timer_list polling_timer;
};

static int ccmni_open(struct net_device *dev)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(dev));
	struct netdev_entity *nent = (struct netdev_entity *)port->private_data;

	atomic_inc(&port->usage_cnt);
	CCCI_INF_MSG(port->modem->index, NET, "port %s open %d cap=0x%X\n", port->name, atomic_read(&port->usage_cnt),
		     port->modem->capability);
	netif_start_queue(dev);
	if (likely(port->modem->capability & MODEM_CAP_NAPI)) {
		napi_enable(&nent->napi);
		napi_schedule(&nent->napi);
	}
	return 0;
}

static int ccmni_close(struct net_device *dev)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(dev));

	atomic_dec(&port->usage_cnt);
	CCCI_INF_MSG(port->modem->index, NET, "port %s close %d\n", port->name, atomic_read(&port->usage_cnt));
	netif_stop_queue(dev);
	if (likely(port->modem->capability & MODEM_CAP_NAPI))
		napi_disable(&((struct netdev_entity *)port->private_data)->napi);
	return 0;
}

static inline int skb_is_ack(struct sk_buff *skb)
{
	u32 packet_type;
	struct tcphdr *tcph;

	packet_type = skb->data[0] & 0xF0;
	if (packet_type == IPV6_VERSION) {
		struct ipv6hdr *iph = (struct ipv6hdr *)skb->data;
		u32 total_len = sizeof(struct ipv6hdr) + ntohs(iph->payload_len);

		if (total_len <= 128 - sizeof(struct ccci_header)) {
			u8 nexthdr = iph->nexthdr;
			__be16 frag_off;
			u32 l4_off = ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &nexthdr, &frag_off);

			tcph = (struct tcphdr *)(skb->data + l4_off);
			if (nexthdr == IPPROTO_TCP &&
			    !tcph->syn && !tcph->fin && !tcph->rst && ((total_len - l4_off) == (tcph->doff << 2))) {
				return 1;
			}
		}
	} else if (packet_type == IPV4_VERSION) {
		struct iphdr *iph = (struct iphdr *)skb->data;

		if (ntohs(iph->tot_len) <= 128 - sizeof(struct ccci_header)) {
			tcph = (struct tcphdr *)(skb->data + (iph->ihl << 2));
			if (iph->protocol == IPPROTO_TCP &&
			    !tcph->syn && !tcph->fin && !tcph->rst &&
			    (ntohs(iph->tot_len) == (iph->ihl << 2) + (tcph->doff << 2))) {
				return 1;
			}
		}
	}

	return 0;
}

static int ccmni_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(dev));
	struct ccci_header *ccci_h;
	int ret;
	int skb_len = skb->len;
	static int tx_busy_retry_cnt;
	int tx_queue, tx_channel;
#ifdef PORT_NET_TRACE
	unsigned long long send_time = 0;
	unsigned long long total_time = 0;

	total_time = sched_clock();
#endif
#ifndef FEATURE_SEQ_CHECK_EN
	struct netdev_entity *nent = (struct netdev_entity *)port->private_data;

	CCCI_DBG_MSG(port->modem->index, NET, "write on %s, len=%d/%d, curr_seq=%d\n",
		     port->name, skb_headroom(skb), skb->len, nent->tx_seq_num);
#else
	CCCI_DBG_MSG(port->modem->index, NET, "write on %s, len=%d/%d\n", port->name, skb_headroom(skb), skb->len);
#endif

	if (unlikely(skb->len > CCCI_NET_MTU)) {
		CCCI_ERR_MSG(port->modem->index, NET, "exceeds MTU(%d) with %d/%d\n", CCCI_NET_MTU, dev->mtu, skb->len);
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}
	if (unlikely(skb_headroom(skb) < sizeof(struct ccci_header))) {
		CCCI_ERR_MSG(port->modem->index, NET, "not enough header room on %s, len=%d header=%d hard_header=%d\n",
			     port->name, skb->len, skb_headroom(skb), dev->hard_header_len);
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}
	if (unlikely(port->modem->md_state != READY)) {
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}
	if (likely((port->rx_ch == CCCI_CCMNI1_RX) || (port->rx_ch == CCCI_CCMNI2_RX))) {
		/* only use on ccmni0 && ccmni1 */
		if (unlikely(skb_is_ack(skb))) {
			tx_channel = port->tx_ch == CCCI_CCMNI1_TX ? CCCI_CCMNI1_DL_ACK : CCCI_CCMNI2_DL_ACK;
			tx_queue = NET_ACK_TXQ_INDEX(port);
		} else {
			tx_channel = port->tx_ch;
			tx_queue = NET_DAT_TXQ_INDEX(port);
		}
	} else {
		tx_channel = port->tx_ch;
		tx_queue = NET_DAT_TXQ_INDEX(port);
	}

	ccci_h = (struct ccci_header *)skb_push(skb, sizeof(struct ccci_header));
	ccci_h->channel = tx_channel;
	ccci_h->data[0] = 0;
	ccci_h->data[1] = skb->len;	/* as skb->len already included ccci_header after skb_push */
#ifndef FEATURE_SEQ_CHECK_EN
	ccci_h->reserved = nent->tx_seq_num++;
#else
	ccci_h->reserved = 0;
#endif
#ifdef PORT_NET_TRACE
	send_time = sched_clock();
#endif
	ret = port->modem->ops->send_request(port->modem, tx_queue, NULL, skb);
#ifdef PORT_NET_TRACE
	send_time = sched_clock() - send_time;
#endif
	if (ret) {
		skb_pull(skb, sizeof(struct ccci_header));
		/* undo header, in next retry, we'll reserve header again */
		goto tx_busy;
	}
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb_len;

	tx_busy_retry_cnt = 0;
#ifdef PORT_NET_TRACE
	total_time = sched_clock() - total_time;
	trace_port_net_tx(port->modem->index, tx_queue, port->tx_ch, 0,
			  (unsigned int)send_time, (unsigned int)(total_time));
#endif
	return NETDEV_TX_OK;

 tx_busy:
	if (unlikely(!(port->modem->capability & MODEM_CAP_TXBUSY_STOP))) {
		if ((tx_busy_retry_cnt) % 20000 == 0)
			CCCI_INF_MSG(port->modem->index, NET, "%s TX busy: retry_times=%d\n", port->name,
				     tx_busy_retry_cnt);
		tx_busy_retry_cnt++;
	} else {
		port->tx_busy_count++;
	}
#ifdef PORT_NET_TRACE
	trace_port_net_error(port->modem->index, tx_queue, port->tx_ch, port->tx_busy_count, __LINE__);
#endif
	return NETDEV_TX_BUSY;
}

static void ccmni_tx_timeout(struct net_device *dev)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(dev));

	dev->stats.tx_errors++;
	if (atomic_read(&port->usage_cnt) > 0)
		netif_wake_queue(dev);
}

static int ccmni_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(dev));
	unsigned int timeout = 0;

	switch (cmd) {
	case SIOCSTXQSTATE:
		/* ifru_ivalue[3~0]:start/stop; ifru_ivalue[7~4]:reserve; */
		/* ifru_ivalue[15~8]:user id, bit8=rild, bit9=thermal */
		/* ifru_ivalue[31~16]: watchdog timeout value */
		if ((ifr->ifr_ifru.ifru_ivalue & 0xF) == 0) {
			if (atomic_read(&port->usage_cnt) > 0) {
				atomic_dec(&port->usage_cnt);
				netif_stop_queue(dev);
				/* stop queue won't stop Tx watchdog (ndo_tx_timeout) */
				timeout = (ifr->ifr_ifru.ifru_ivalue & 0xFFFF0000) >> 16;
				if (timeout == 0)
					dev->watchdog_timeo = 60*HZ;
				else
					dev->watchdog_timeo = timeout*HZ;
			}
		} else {
			if (atomic_read(&port->usage_cnt) <= 0) {
				if (netif_running(dev) && netif_queue_stopped(dev))
					netif_wake_queue(dev);
				dev->watchdog_timeo = 1 * HZ;
				atomic_inc(&port->usage_cnt);
			}
		}
		CCCI_INF_MSG(port->modem->index, NET, "SIOCSTXQSTATE request=%d on %s %d\n", ifr->ifr_ifru.ifru_ivalue,
			     port->name, atomic_read(&port->usage_cnt));
		break;
	default:
		CCCI_INF_MSG(port->modem->index, NET, "unknown ioctl cmd=%d on %s\n", cmd, port->name);
		break;
	}

	return 0;
}

struct net_device_stats *ccmni_get_stats(struct net_device *dev)
{
	return &dev->stats;
}

static const struct net_device_ops ccmni_netdev_ops = {

	.ndo_open = ccmni_open,
	.ndo_stop = ccmni_close,
	.ndo_start_xmit = ccmni_start_xmit,
	.ndo_tx_timeout = ccmni_tx_timeout,
	.ndo_do_ioctl = ccmni_ioctl,
	.ndo_get_stats = ccmni_get_stats,
};

#ifndef CCMNI_U
static int port_net_poll(struct napi_struct *napi, int budget)
{
	struct ccci_port *port = *((struct ccci_port **)netdev_priv(napi->dev));
	struct netdev_entity *nent = (struct netdev_entity *)port->private_data;

	del_timer(&nent->polling_timer);
	return port->modem->ops->napi_poll(port->modem, PORT_RXQ_INDEX(port), napi, budget);
}

static void napi_polling_timer_func(unsigned long data)
{
	struct ccci_port *port = (struct ccci_port *)data;

	CCCI_ERR_MSG(port->modem->index, NET, "lost NAPI polling on %s\n", port->name);
}

static void ccmni_make_etherframe(void *_eth_hdr, unsigned char *mac_addr, unsigned int packet_type)
{
	struct ethhdr *eth_hdr = _eth_hdr;

	memcpy(eth_hdr->h_dest, mac_addr, sizeof(eth_hdr->h_dest));
	memset(eth_hdr->h_source, 0, sizeof(eth_hdr->h_source));
	if (packet_type == 0x60)
		eth_hdr->h_proto = cpu_to_be16(ETH_P_IPV6);
	else
		eth_hdr->h_proto = cpu_to_be16(ETH_P_IP);
}
#endif

static int port_net_init(struct ccci_port *port)
{
#ifdef CCMNI_U
	if (port->rx_ch == CCCI_CCMNI1_RX) {
#if defined CONFIG_MTK_IRAT_SUPPORT
		CCCI_NOTICE_MSG(port->modem->index, NET, "clear MODEM_CAP_SGIO flag\n");
		port->modem->capability &= (~(MODEM_CAP_SGIO));
#endif
		eccci_ccmni_ops.md_ability |= port->modem->capability;
		if (port->modem->index == MD_SYS1)
			ccmni_ops.init(port->modem->index, &eccci_ccmni_ops);
		else if (port->modem->index == MD_SYS3)
			ccmni_ops.init(port->modem->index, &eccci_cc3mni_ops);
	}
	return 0;
#else
	struct ccci_port **temp;
	struct net_device *dev = NULL;
	struct netdev_entity *nent = NULL;

	CCCI_DBG_MSG(port->modem->index, NET, "network port is initializing\n");
	dev = alloc_etherdev(sizeof(struct ccci_port *));
	dev->header_ops = NULL;
	dev->mtu = CCCI_NET_MTU;
	dev->tx_queue_len = 1000;
	dev->watchdog_timeo = 1 * HZ;
	dev->flags = IFF_NOARP &	/* ccmni is a pure IP device */
	    (~IFF_BROADCAST & ~IFF_MULTICAST);	/* ccmni is P2P */
	dev->features = NETIF_F_VLAN_CHALLENGED;	/* not support VLAN */

#ifndef CONFIG_MTK_IRAT_SUPPORT
	if (port->modem->capability & MODEM_CAP_SGIO) {
		dev->features |= NETIF_F_SG;
		dev->hw_features |= NETIF_F_SG;
	}
#endif
	dev->addr_len = ETH_ALEN;	/* ethernet header size */
	dev->destructor = free_netdev;
	dev->hard_header_len += sizeof(struct ccci_header);	/* reserve Tx CCCI header room */
	dev->netdev_ops = &ccmni_netdev_ops;

	temp = netdev_priv(dev);
	*temp = port;
	sprintf(dev->name, "%s", port->name);

	random_ether_addr((u8 *) dev->dev_addr);

	nent = kzalloc(sizeof(struct netdev_entity), GFP_KERNEL);
	nent->ndev = dev;
	if (likely(port->modem->capability & MODEM_CAP_NAPI))
		netif_napi_add(dev, &nent->napi, port_net_poll, NAPI_POLL_WEIGHT);	/* hardcode */
	port->private_data = nent;
	init_timer(&nent->polling_timer);
	nent->polling_timer.function = napi_polling_timer_func;
	nent->polling_timer.data = (unsigned long)port;
	register_netdev(dev);
	CCCI_DBG_MSG(port->modem->index, NET, "network device %s hard_header_len=%d\n", dev->name,
		     dev->hard_header_len);
	return 0;
#endif
}

static int port_net_recv_skb(struct ccci_port *port, struct sk_buff *skb)
{
#ifdef CCMNI_U
	struct ccci_header *ccci_h = (struct ccci_header *)skb->data;
#ifdef PORT_NET_TRACE
	unsigned long long rx_cb_time;
	unsigned long long total_time;

	total_time = sched_clock();
#endif
	skb_pull(skb, sizeof(struct ccci_header));
	CCCI_DBG_MSG(port->modem->index, NET, "[RX]: 0x%08X, 0x%08X, %08X, 0x%08X\n",
		     ccci_h->data[0], ccci_h->data[1], ccci_h->channel, ccci_h->reserved);
#ifdef PORT_NET_TRACE
	rx_cb_time = sched_clock();
#endif
	ccmni_ops.rx_callback(port->modem->index, ccci_h->channel, skb, NULL);
#ifdef PORT_NET_TRACE
	rx_cb_time = sched_clock() - rx_cb_time;
	total_time = sched_clock() - total_time;
	trace_port_net_rx(port->modem->index, PORT_RXQ_INDEX(port), port->rx_ch, (unsigned int)rx_cb_time,
			  (unsigned int)total_time);
#endif
	return 0;
#else

	struct netdev_entity *nent = (struct netdev_entity *)port->private_data;
	struct ccci_modem *md = port->modem;
	struct net_device *dev = nent->ndev;
	unsigned int packet_type;
	int skb_len = skb->len;
#if !defined(FEATURE_SEQ_CHECK_EN)
	struct ccci_header *ccci_h = (struct ccci_header *)skb->data;
#endif

#ifdef PORT_NET_TRACE
	total_time = sched_clock();
#endif
#ifndef FEATURE_SEQ_CHECK_EN
	CCCI_DBG_MSG(md->index, NET, "recv on %s, curr_seq=%d\n", port->name, ccci_h->reserved);
	if (unlikely(nent->rx_seq_num != 0 && (ccci_h->reserved - nent->rx_seq_num) != 1)) {
		CCCI_ERR_MSG(md->index, NET, "possible packet lost on %s %d->%d\n",
			     port->name, nent->rx_seq_num, ccci_h->reserved);
	}
	nent->rx_seq_num = ccci_h->reserved;
#else
	CCCI_DBG_MSG(md->index, NET, "recv on %s\n", port->name);
#endif

	skb_pull(skb, sizeof(struct ccci_header));
	packet_type = skb->data[0] & 0xF0;
	ccmni_make_etherframe(skb->data - ETH_HLEN, dev->dev_addr, packet_type);
	skb_set_mac_header(skb, -ETH_HLEN);
	skb->dev = dev;
	if (packet_type == 0x60) {
		skb->protocol = htons(ETH_P_IPV6);
	} else {
		skb->protocol = htons(ETH_P_IP);
#ifdef CCCI_SKB_TRACE
		md->netif_rx_profile[2] = ((struct iphdr *)skb->data)->id;
		skb->mark &= 0x0FFFFFFF;
		skb->mark |= (0x5<<28);
#endif
	}
	skb->ip_summed = CHECKSUM_NONE;
#ifdef CCCI_SKB_TRACE
	md->netif_rx_profile[3] = sched_clock();
#endif
#ifdef PORT_NET_TRACE
	rx_cb_time = sched_clock();
#endif
	if (likely(md->capability & MODEM_CAP_NAPI)) {
#ifdef ENABLE_GRO
		napi_gro_receive(&nent->napi, skb);
#else
		netif_receive_skb(skb);
#endif
	} else {
		if (!in_interrupt())
			netif_rx_ni(skb);
		else
			netif_rx(skb);
	}
#ifdef CCCI_SKB_TRACE
	md->netif_rx_profile[3] = sched_clock() - md->netif_rx_profile[3];
#endif
#ifdef PORT_NET_TRACE
	rx_cb_time = sched_clock() - rx_cb_time;
#endif
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb_len;
#ifdef CCCI_SKB_TRACE
	md->netif_rx_profile[0] = dev->stats.rx_bytes - 40*dev->stats.rx_packets;
	md->netif_rx_profile[1] = dev->stats.tx_bytes - 40*dev->stats.tx_packets;
#endif
	wake_lock_timeout(&port->rx_wakelock, HZ);
#ifdef PORT_NET_TRACE
	total_time = sched_clock() - total_time;
	trace_port_net_rx(md->index, PORT_RXQ_INDEX(port), port->rx_ch, (unsigned int)rx_cb_time,
			  (unsigned int)total_time);
#endif
	return 0;
#endif
}

static void port_net_md_state_notice(struct ccci_port *port, MD_STATE state)
{
#ifdef CCMNI_U
	if (((state == TX_IRQ) && ((port->flags & PORT_F_RX_FULLED) == 0)) ||
		((state == TX_FULL) && (port->flags & PORT_F_RX_FULLED)))
		return;
	ccmni_ops.md_state_callback(port->modem->index, port->rx_ch, state);
	switch (state) {
	case TX_IRQ:
		port->flags &= ~PORT_F_RX_FULLED;
		break;
	case TX_FULL:
		port->flags |= PORT_F_RX_FULLED;	/* for convenient in traffic log */
		break;
	default:
		break;
	};
#else
	struct netdev_entity *nent = (struct netdev_entity *)port->private_data;
	struct net_device *dev = nent->ndev;

	/* CCCI_INF_MSG(port->modem->index, NET, "port_net_md_state_notice: %s, md_sta=%d\n", port->name, state); */
	if (((state == TX_IRQ) && ((port->flags & PORT_F_RX_FULLED) == 0)) ||
		((state == TX_FULL) && (port->flags & PORT_F_RX_FULLED)))
		return;
	switch (state) {
	case RX_IRQ:
		mod_timer(&nent->polling_timer, jiffies + HZ);
		napi_schedule(&nent->napi);
		wake_lock_timeout(&port->rx_wakelock, HZ);
		break;
	case TX_IRQ:
		if (netif_running(dev) && netif_queue_stopped(dev) && atomic_read(&port->usage_cnt) > 0)
			netif_wake_queue(dev);
		port->flags &= ~PORT_F_RX_FULLED;
		break;
	case TX_FULL:
		netif_stop_queue(dev);
		port->flags |= PORT_F_RX_FULLED;	/* for convenient in traffic log */
		break;
	case READY:
		netif_carrier_on(dev);
		break;
	case EXCEPTION:
	case RESET:
		netif_carrier_off(dev);
#ifndef FEATURE_SEQ_CHECK_EN
		nent->tx_seq_num = 0;
		nent->rx_seq_num = 0;
#endif
		break;
	default:
		break;
	};
#endif
}

void port_net_md_dump_info(struct ccci_port *port, unsigned int flag)
{
#ifdef CCMNI_U
	if (port == NULL) {
		CCCI_ERR_MSG(0, NET, "port_net_md_dump_info: port==NULL\n");
		return;
	}
	if (port->modem == NULL) {
		CCCI_ERR_MSG(0, NET, "port_net_md_dump_info: port->modem == null\n");
		return;
	}
	if (ccmni_ops.dump == NULL) {
		CCCI_ERR_MSG(0, NET, "port_net_md_dump_info: ccmni_ops.dump== null\n");
		return;
	}
	ccmni_ops.dump(port->modem->index, port->rx_ch, 0);
#endif
}

struct ccci_port_ops net_port_ops = {
	.init = &port_net_init,
	.recv_skb = &port_net_recv_skb,
	.md_state_notice = &port_net_md_state_notice,
	.dump_info = &port_net_md_dump_info,
};
