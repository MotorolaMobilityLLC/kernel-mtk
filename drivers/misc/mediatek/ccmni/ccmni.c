/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccmni.c
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   Cross Chip Modem Network Interface
 *
 * Author:
 * -------
 *   Anny.Hu(mtk80401)
 *
 ****************************************************************************/
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <net/sch_generic.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/sockios.h>
#include <linux/device.h>
#include <linux/debugfs.h>

#include "ccmni.h"


ccmni_ctl_block_t *ccmni_ctl_blk[MAX_MD_NUM];
unsigned int ccmni_debug_level = 0;



/********************internal function*********************/
static int get_ccmni_idx_from_ch(int md_id, int ch)
{
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];
	unsigned int i;

	for (i = 0; i < ctlb->ccci_ops->ccmni_num; i++) {
		if (ctlb->ccmni_inst[i]) {
			if ((ctlb->ccmni_inst[i]->ch.rx == ch) || (ctlb->ccmni_inst[i]->ch.tx_ack == ch))
				return i;
		} else {
			CCMNI_ERR_MSG(md_id, "invalid ccmni instance(ccmni%d): ch=%d\n", i, ch);
		}
	}

	CCMNI_ERR_MSG(md_id, "invalid ccmni rx channel(%d)\n", ch);
	return -1;
}

static void ccmni_make_etherframe(void *_eth_hdr, unsigned char *mac_addr, unsigned int packet_type)
{
	struct ethhdr *eth_hdr = _eth_hdr;

	memcpy(eth_hdr->h_dest,   mac_addr, sizeof(eth_hdr->h_dest));
	memset(eth_hdr->h_source, 0, sizeof(eth_hdr->h_source));
	if (packet_type == 0x60)
		eth_hdr->h_proto = cpu_to_be16(ETH_P_IPV6);
	else
		eth_hdr->h_proto = cpu_to_be16(ETH_P_IP);
}

static inline int is_ack_skb(int md_id, struct sk_buff *skb)
{
	u32 packet_type;
	struct tcphdr *tcph;
	int ret = 0;

	packet_type = skb->data[0] & 0xF0;
	if (packet_type == IPV6_VERSION) {
		struct ipv6hdr *iph = (struct ipv6hdr *)skb->data;
		u32 total_len = sizeof(struct ipv6hdr) + ntohs(iph->payload_len);

		if (total_len <= 128 - sizeof(struct ccci_header)) {
			u8 nexthdr = iph->nexthdr;
			__be16 frag_off;
			u32 l4_off = ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &nexthdr, &frag_off);

			tcph = (struct tcphdr *)(skb->data + l4_off);
			if (nexthdr == IPPROTO_TCP && !tcph->syn && !tcph->fin &&
			!tcph->rst && ((total_len - l4_off) == (tcph->doff << 2)))
				ret = 1;

			if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_ACK_SKB)) {
				CCMNI_INF_MSG(md_id,
					"[SKB] ack=%d: proto=%d syn=%d fin=%d rst=%d ack=%d tot_len=%d l4_off=%d doff=%d\n",
					ret, nexthdr, tcph->syn, tcph->fin, tcph->rst,
					tcph->ack, total_len, l4_off, tcph->doff);
			}
		} else {
			if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_ACK_SKB))
				CCMNI_INF_MSG(md_id, "[SKB] ack=%d: tot_len=%d\n", ret, total_len);
		}
	} else if (packet_type == IPV4_VERSION) {
		struct iphdr *iph = (struct iphdr *)skb->data;

		if (ntohs(iph->tot_len) <= 128 - sizeof(struct ccci_header)) {
			tcph = (struct tcphdr *)(skb->data + (iph->ihl << 2));

			if (iph->protocol == IPPROTO_TCP && !tcph->syn && !tcph->fin &&
			!tcph->rst && (ntohs(iph->tot_len) == (iph->ihl << 2) + (tcph->doff << 2)))
				ret = 1;

			if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_ACK_SKB)) {
				CCMNI_INF_MSG(md_id,
					"[SKB] ack=%d: proto=%d syn=%d fin=%d rst=%d ack=%d tot_len=%d ihl=%d doff=%d\n",
					ret, iph->protocol, tcph->syn, tcph->fin, tcph->rst,
					tcph->ack, ntohs(iph->tot_len), iph->ihl, tcph->doff);
			}
		} else {
			if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_ACK_SKB))
				CCMNI_INF_MSG(md_id, "[SKB] ack=%d: tot_len=%d\n", ret, ntohs(iph->tot_len));
		}
	}

	return ret;
}


/********************internal debug function*********************/
#if 1
#if 0
static void ccmni_dbg_skb_addr(int md_id, bool tx, struct sk_buff *skb, int idx)
{
	CCMNI_INF_MSG(md_id, "[SKB][%s] idx=%d addr=%p len=%d data_len=%d, L2_addr=%p L3_addr=%p L4_addr=%p\n",
		tx?"TX":"RX", idx,
		(void *)skb->data, skb->len, skb->data_len, (void *)skb_mac_header(skb),
		(void *)skb_network_header(skb), (void *)skb_transport_header(skb));
}
#endif

static void ccmni_dbg_eth_header(int md_id, bool tx, struct ethhdr *ethh)
{
	if (ethh != NULL) {
		CCMNI_INF_MSG(md_id,
			"[SKB][%s] ethhdr: proto=0x%04x dest_mac=%02x:%02x:%02x:%02x:%02x:%02x src_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			tx?"TX":"RX", ethh->h_proto, ethh->h_dest[0], ethh->h_dest[1], ethh->h_dest[2],
			ethh->h_dest[3], ethh->h_dest[4], ethh->h_dest[5], ethh->h_source[0], ethh->h_source[1],
			ethh->h_source[2], ethh->h_source[3], ethh->h_source[4], ethh->h_source[5]);
	}
}

static void ccmni_dbg_ip_header(int md_id, bool tx, struct iphdr *iph)
{
	if (iph != NULL) {
		CCMNI_INF_MSG(md_id,
			"[SKB][%s] iphdr: ihl=0x%02x ver=0x%02x tos=0x%02x tot_len=0x%04x id=0x%04x frag_off=0x%04x ttl=0x%02x proto=0x%02x check=0x%04x saddr=0x%08x daddr=0x%08x\n",
			tx?"TX":"RX", iph->ihl, iph->version, iph->tos, iph->tot_len, iph->id,
			iph->frag_off, iph->ttl, iph->protocol, iph->check, iph->saddr, iph->daddr);
	}
}

static void ccmni_dbg_tcp_header(int md_id, bool tx, struct tcphdr *tcph)
{
	if (tcph != NULL) {
		CCMNI_INF_MSG(md_id,
			"[SKB][%s] tcp_hdr: src=0x%04x dest=0x%04x seq=0x%08x ack_seq=0x%08x urg=%d ack=%d psh=%d rst=%d syn=%d fin=%d\n",
			tx?"TX":"RX", ntohl(tcph->source), ntohl(tcph->dest), tcph->seq, tcph->ack_seq,
			tcph->urg, tcph->ack, tcph->psh, tcph->rst, tcph->syn, tcph->fin);
	}
}

static void ccmni_dbg_skb_header(int md_id, bool tx, struct sk_buff *skb)
{
	struct ethhdr *ethh = NULL;
	struct iphdr  *iph = NULL;
	struct ipv6hdr  *ipv6h = NULL;
	struct tcphdr  *tcph = NULL;
	u8 nexthdr;
	__be16 frag_off;
	u32 l4_off;

	if (!tx) {
		ethh = (struct ethhdr *)(skb->data-ETH_HLEN);
		ccmni_dbg_eth_header(md_id, tx, ethh);
	}

	if (skb->protocol == htons(ETH_P_IP)) {
		iph = (struct iphdr *)skb->data;
		ccmni_dbg_ip_header(md_id, tx, iph);

		if (iph->protocol == IPPROTO_TCP) {
			tcph = (struct tcphdr *)(skb->data + (iph->ihl << 2));
			ccmni_dbg_tcp_header(md_id, tx, tcph);
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		ipv6h = (struct ipv6hdr *)skb->data;

		nexthdr = ipv6h->nexthdr;
		if (nexthdr == IPPROTO_TCP) {
			l4_off = ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &nexthdr, &frag_off);
			tcph = (struct tcphdr *)(skb->data + l4_off);
			ccmni_dbg_tcp_header(md_id, tx, tcph);
		}
	}
}
#endif

/* ccmni debug sys file create */
int ccmni_debug_file_init(int md_id)
{
	int result = -1;
	char fname[16];
	struct dentry *dentry1, *dentry2, *dentry3;

	CCMNI_INF_MSG(md_id, "ccmni_debug_file_init\n");

	dentry1 = debugfs_create_dir("ccmni", NULL);
	if (!dentry1) {
		CCMNI_ERR_MSG(md_id, "create /proc/ccmni fail\n");
		return -ENOENT;
	}

	snprintf(fname, 16, "md%d", (md_id+1));

	dentry2 = debugfs_create_dir(fname, dentry1);
	if (!dentry2) {
		CCMNI_ERR_MSG(md_id, "create /proc/ccmni/md%d fail\n", (md_id+1));
		return -ENOENT;
	}

	dentry3 = debugfs_create_u32("debug_level", 0600, dentry2, &ccmni_debug_level);
	result = PTR_ERR(dentry3);
	if (IS_ERR(dentry3) && result != -ENODEV) {
		CCMNI_ERR_MSG(md_id, "create /proc/ccmni/md%d/debug_level fail: %d\n", md_id, result);
		return -ENOENT;
	}

	return 0;
}

/********************netdev register function********************/
static int ccmni_open(struct net_device *dev)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);
	ccmni_ctl_block_t *ccmni_ctl = ccmni_ctl_blk[ccmni->md_id];
	ccmni_instance_t *ccmni_tmp = NULL;

	if (unlikely(ccmni_ctl == NULL)) {
		CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d open: MD%d ctlb is NULL\n", ccmni->index, ccmni->md_id);
		return -1;
	}

	netif_start_queue(dev);
	if (unlikely(ccmni_ctl->ccci_ops->md_ability & MODEM_CAP_NAPI)) {
		napi_enable(&ccmni->napi);
		napi_schedule(&ccmni->napi);
	}
	atomic_inc(&ccmni->usage);
	ccmni_tmp = ccmni_ctl->ccmni_inst[ccmni->index];
	if (ccmni != ccmni_tmp)
		atomic_inc(&ccmni_tmp->usage);

	CCMNI_INF_MSG(ccmni->md_id, "CCMNI%d_Open: cnt=(%d,%d), md_ab=0x%X\n",
		ccmni->index, atomic_read(&ccmni->usage), atomic_read(&ccmni_tmp->usage),
		ccmni_ctl->ccci_ops->md_ability);
	return 0;
}

static int ccmni_close(struct net_device *dev)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);
	ccmni_ctl_block_t *ccmni_ctl = ccmni_ctl_blk[ccmni->md_id];
	ccmni_instance_t *ccmni_tmp = NULL;

	if (unlikely(ccmni_ctl == NULL)) {
		CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d_Close: MD%d ctlb is NULL\n", ccmni->index, ccmni->md_id);
		return -1;
	}

	atomic_dec(&ccmni->usage);

	ccmni_tmp = ccmni_ctl->ccmni_inst[ccmni->index];
	if (ccmni != ccmni_tmp)
		atomic_dec(&ccmni_tmp->usage);

	CCMNI_INF_MSG(ccmni->md_id, "CCMNI%d_Close: cnt=(%d, %d)\n",
		ccmni->index, atomic_read(&ccmni->usage), atomic_read(&ccmni_tmp->usage));
	netif_stop_queue(dev);
	if (unlikely(ccmni_ctl->ccci_ops->md_ability & MODEM_CAP_NAPI))
		napi_disable(&ccmni->napi);

	return 0;
}

static int ccmni_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int ret;
	int skb_len = skb->len;
	int tx_ch, ccci_tx_ch;
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[ccmni->md_id];
	unsigned int is_ack = 0;

	/* dev->mtu is changed  if dev->mtu is changed by upper layer */
	if (unlikely(skb->len > dev->mtu)) {
		CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d write fail: len(0x%x)>MTU(0x%x, 0x%x)\n",
			ccmni->index, skb->len, CCMNI_MTU, dev->mtu);
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}

	if (unlikely(skb_headroom(skb) < sizeof(struct ccci_header))) {
		CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d write fail: header room(%d) < ccci_header(%d)\n",
			ccmni->index, skb_headroom(skb), dev->hard_header_len);
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		return NETDEV_TX_OK;
	}

	ccci_tx_ch = tx_ch = ccmni->ch.tx;
	if (ctlb->ccci_ops->md_ability & MODEM_CAP_DATA_ACK_DVD) {
		if (ccmni->ch.rx == CCCI_CCMNI1_RX || ccmni->ch.rx == CCCI_CCMNI2_RX) {
			is_ack = is_ack_skb(ccmni->md_id, skb);
			if (is_ack) {
				ccci_tx_ch = (ccmni->ch.tx == CCCI_CCMNI1_TX)?CCCI_CCMNI1_DL_ACK:CCCI_CCMNI2_DL_ACK;
				/* tx_ch = ctlb->ccmni_inst[1]->ch.tx; */
			} else {
				ccci_tx_ch = ccmni->ch.tx;
				/* tx_ch = ctlb->ccmni_inst[0]->ch.tx; */
			}
		}
	}

#if 0
	struct ccci_header *ccci_h;

	ccci_h = (struct ccci_header *)skb_push(skb, sizeof(struct ccci_header));
	ccci_h->channel = ccci_tx_ch;
	ccci_h->data[0] = 0;
	ccci_h->data[1] = skb->len; /* as skb->len already included ccci_header after skb_push */

	if (ctlb->ccci_ops->md_ability & MODEM_CAP_CCMNI_SEQNO)
		ccci_h->reserved = ccmni->tx_seq_num[is_ack]++;
	else
		ccci_h->reserved = 0;

	CCMNI_DBG_MSG(ccmni->md_id, "[TX]CCMNI%d: 0x%08X, 0x%08X, %08X, 0x%08X\n",
		ccmni->index, ccci_h->data[0], ccci_h->data[1], ccci_h->channel, ccci_h->reserved);
#endif

	if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_TX)) {
		CCMNI_INF_MSG(ccmni->md_id, "[TX]CCMNI%d head_len=%d len=%d ack=%d tx_ch=%d\n",
			ccmni->index, skb_headroom(skb), skb->len, is_ack, ccci_tx_ch);
	}

	if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_TX_SKB))
		ccmni_dbg_skb_header(ccmni->md_id, true, skb);

	ret = ctlb->ccci_ops->send_pkt(ccmni->md_id, ccci_tx_ch, skb);
	if (ret == CCMNI_ERR_MD_NO_READY || ret == CCMNI_ERR_TX_INVAL) {
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		ccmni->tx_busy_cnt = 0;
		CCMNI_ERR_MSG(ccmni->md_id, "[TX]CCMNI%d send pkt fail: %d\n", ccmni->index, ret);
		return NETDEV_TX_OK;
	} else if (ret == CCMNI_ERR_TX_BUSY) {
#if 0
		skb_pull(skb, sizeof(struct ccci_header)); /* undo header, in next retry, we'll reserve header again */
#endif
		goto tx_busy;
	}
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb_len;
	if (ccmni->tx_busy_cnt > 10) {
		CCMNI_ERR_MSG(ccmni->md_id, "[TX]CCMNI%d TX busy: tx_pkt=%ld retry %ld times done\n",
			ccmni->index, dev->stats.tx_packets, ccmni->tx_busy_cnt);
	}
	ccmni->tx_busy_cnt = 0;

	return NETDEV_TX_OK;

tx_busy:
	if (unlikely(!(ctlb->ccci_ops->md_ability & MODEM_CAP_TXBUSY_STOP))) {
		if ((ccmni->tx_busy_cnt++)%100 == 0)
			CCMNI_ERR_MSG(ccmni->md_id, "[TX]CCMNI%d TX busy: retry_times=%ld\n",
				ccmni->index, ccmni->tx_busy_cnt);
	} else {
		ccmni->tx_busy_cnt++;
	}
	return NETDEV_TX_BUSY;
}

static int ccmni_change_mtu(struct net_device *dev, int new_mtu)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);

	dev->mtu = new_mtu;
	CCMNI_INF_MSG(ccmni->md_id, "CCMNI%d change mtu_siz=%d\n", ccmni->index, new_mtu);
	return 0;
}

static void ccmni_tx_timeout(struct net_device *dev)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);

	CCMNI_INF_MSG(ccmni->md_id, "ccmni%d_tx_timeout: usage_cnt=%d, timeout=%ds\n",
		ccmni->index, atomic_read(&ccmni->usage), (ccmni->dev->watchdog_timeo/HZ));

	dev->stats.tx_errors++;
	if (atomic_read(&ccmni->usage) > 0)
		netif_wake_queue(dev);
}

static int ccmni_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int md_id, md_id_irat, usage_cnt;
	ccmni_instance_t *ccmni_irat;
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(dev);
	ccmni_instance_t *ccmni_tmp = NULL;
	ccmni_ctl_block_t *ctlb = NULL;
	ccmni_ctl_block_t *ctlb_irat = NULL;

	switch (cmd) {
	case SIOCSTXQSTATE:
		if (!ifr->ifr_ifru.ifru_ivalue) {
			if (atomic_read(&ccmni->usage) > 0) {
				atomic_dec(&ccmni->usage);
				netif_stop_queue(dev);
				/* stop queue won't stop Tx watchdog (ndo_tx_timeout) */
				dev->watchdog_timeo = 60*HZ;

				ctlb = ccmni_ctl_blk[ccmni->md_id];
				ccmni_tmp = ctlb->ccmni_inst[ccmni->index];
				if (ccmni_tmp != ccmni) { /* iRAT ccmni */
					usage_cnt = atomic_read(&ccmni->usage);
					atomic_set(&ccmni_tmp->usage, usage_cnt);
				}
			}
		} else {
			if (atomic_read(&ccmni->usage) <= 0) {
				if (netif_running(dev) && netif_queue_stopped(dev))
					netif_wake_queue(dev);
				dev->watchdog_timeo = CCMNI_NETDEV_WDT_TO;
				atomic_inc(&ccmni->usage);

				ctlb = ccmni_ctl_blk[ccmni->md_id];
				ccmni_tmp = ctlb->ccmni_inst[ccmni->index];
				if (ccmni_tmp != ccmni) { /* iRAT ccmni */
					usage_cnt = atomic_read(&ccmni->usage);
					atomic_set(&ccmni_tmp->usage, usage_cnt);
				}
			}
		}
		if (likely(ccmni_tmp != NULL)) {
			CCMNI_INF_MSG(ccmni->md_id, "SIOCSTXQSTATE: CCMNI%d_state=0x%x, cnt=(%d, %d)\n",
				ccmni->index, ifr->ifr_ifru.ifru_ivalue, atomic_read(&ccmni->usage),
				atomic_read(&ccmni_tmp->usage));
		} else {
			CCMNI_INF_MSG(ccmni->md_id, "SIOCSTXQSTATE: CCMNI%d_state=0x%x, cnt=%d\n",
				ccmni->index, ifr->ifr_ifru.ifru_ivalue, atomic_read(&ccmni->usage));
		}
		break;

	case SIOCCCMNICFG:
		md_id_irat = ifr->ifr_ifru.ifru_ivalue;
		md_id = ccmni->md_id;
		if (md_id_irat < 0 && md_id_irat >= MAX_MD_NUM) {
			CCMNI_ERR_MSG(md_id, "SIOCSCCMNICFG: CCMNI%d invalid md_id(%d)\n",
				ccmni->index, (ifr->ifr_ifru.ifru_ivalue+1));
			return -EINVAL;
		}
		if (md_id_irat == ccmni->md_id) {
			CCMNI_INF_MSG(md_id, "SIOCCCMNICFG: CCMNI%d iRAT on the same MD%d, cnt=%d\n",
				ccmni->index, (ifr->ifr_ifru.ifru_ivalue+1), atomic_read(&ccmni->usage));
			break;
		}

		ctlb_irat = ccmni_ctl_blk[md_id_irat];
		if (ccmni->index >= ctlb_irat->ccci_ops->ccmni_num) {
			CCMNI_ERR_MSG(md_id, "SIOCSCCMNICFG: CCMNI%d iRAT fail, ccmni_idx(%d) > md%d_ccmni_num(%d)\n",
				ccmni->index, ccmni->index, md_id, ctlb_irat->ccci_ops->ccmni_num);
			break;
		}
		ccmni_irat = ctlb_irat->ccmni_inst[ccmni->index];
		usage_cnt = atomic_read(&ccmni->usage);
		atomic_set(&ccmni_irat->usage, usage_cnt);
		memcpy(netdev_priv(dev), ccmni_irat, sizeof(ccmni_instance_t));

		ctlb = ccmni_ctl_blk[md_id];
		ccmni_tmp = ctlb->ccmni_inst[ccmni->index];
		atomic_set(&ccmni_tmp->usage, usage_cnt);
		ccmni_tmp->tx_busy_cnt = ccmni->tx_busy_cnt;

		CCMNI_INF_MSG(md_id,
			"SIOCCCMNICFG: CCMNI%d iRAT MD%d->MD%d, dev_cnt=%d, md_cnt=%d, md_irat_cnt=%d\n",
			ccmni->index, (md_id+1), (ifr->ifr_ifru.ifru_ivalue+1), atomic_read(&ccmni->usage),
			atomic_read(&ccmni_tmp->usage), atomic_read(&ccmni_irat->usage));
		break;

	default:
		CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d: unknown ioctl cmd=%x\n", ccmni->index, cmd);
		break;
	}

	return 0;
}

static const struct net_device_ops ccmni_netdev_ops = {

	.ndo_open		= ccmni_open,
	.ndo_stop		= ccmni_close,
	.ndo_start_xmit	= ccmni_start_xmit,
	.ndo_tx_timeout	= ccmni_tx_timeout,
	.ndo_do_ioctl   = ccmni_ioctl,
	.ndo_change_mtu = ccmni_change_mtu,
};

static int ccmni_napi_poll(struct napi_struct *napi , int budget)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)netdev_priv(napi->dev);
	int md_id = ccmni->md_id;
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];

	del_timer(&ccmni->timer);

	if (ctlb->ccci_ops->napi_poll)
		return ctlb->ccci_ops->napi_poll(md_id, ccmni->ch.rx, napi, budget);
	else
		return 0;
}

static void ccmni_napi_poll_timeout(unsigned long data)
{
	ccmni_instance_t *ccmni = (ccmni_instance_t *)data;

	CCMNI_ERR_MSG(ccmni->md_id, "CCMNI%d lost NAPI polling\n", ccmni->index);
}


/********************ccmni driver register  ccci function********************/
static inline int ccmni_inst_init(int md_id, ccmni_instance_t *ccmni, struct net_device *dev)
{
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];
	struct ccmni_ch channel;
	int ret = 0;

	ret = ctlb->ccci_ops->get_ccmni_ch(md_id, ccmni->index, &channel);
	if (ret) {
		CCMNI_ERR_MSG(md_id, "get ccmni%d channel fail\n", ccmni->index);
		return ret;
	}

	ccmni->dev = dev;
	ccmni->ctlb = ctlb;
	ccmni->md_id = md_id;

	/* ccmni tx/rx channel setting */
	ccmni->ch.rx = channel.rx;
	ccmni->ch.rx_ack = channel.rx_ack;
	ccmni->ch.tx = channel.tx;
	ccmni->ch.tx_ack = channel.tx_ack;

	/* register napi device */
	if (dev && (ctlb->ccci_ops->md_ability & MODEM_CAP_NAPI)) {
		init_timer(&ccmni->timer);
		ccmni->timer.function = ccmni_napi_poll_timeout;
		ccmni->timer.data = (unsigned long)ccmni;
		netif_napi_add(dev, &ccmni->napi, ccmni_napi_poll, ctlb->ccci_ops->napi_poll_weigh);
	}

	atomic_set(&ccmni->usage, 0);
	spin_lock_init(&ccmni->spinlock);

	return ret;
}

static int ccmni_init(int md_id, ccmni_ccci_ops_t *ccci_info)
{
	int i = 0, j = 0, ret = 0;
	ccmni_ctl_block_t *ctlb = NULL;
	ccmni_ctl_block_t *ctlb_irat_src = NULL;
	ccmni_instance_t  *ccmni = NULL;
	ccmni_instance_t  *ccmni_irat_src = NULL;
	struct net_device *dev = NULL;

	if (unlikely(ccci_info->md_ability & MODEM_CAP_CCMNI_DISABLE)) {
		CCMNI_ERR_MSG(md_id, "no need init ccmni: md_ability=0x%08X\n", ccci_info->md_ability);
		return 0;
	}

	ctlb = kzalloc(sizeof(ccmni_ctl_block_t), GFP_KERNEL);
	if (unlikely(ctlb == NULL)) {
		CCMNI_ERR_MSG(md_id, "alloc ccmni ctl struct fail\n");
		return -ENOMEM;
	}

	ctlb->ccci_ops = kzalloc(sizeof(ccmni_ccci_ops_t), GFP_KERNEL);
	if (unlikely(ctlb->ccci_ops == NULL)) {
		CCMNI_ERR_MSG(md_id, "alloc ccmni_ccci_ops struct fail\n");
		ret = -ENOMEM;
		goto alloc_mem_fail;
	}

	ccmni_ctl_blk[md_id] = ctlb;

	memcpy(ctlb->ccci_ops, ccci_info, sizeof(ccmni_ccci_ops_t));

	CCMNI_INF_MSG(md_id,
		"ccmni_init: ccmni_num=%d, md_ability=0x%08x, irat_en=%08x, irat_md_id=%d, send_pkt=%p, get_ccmni_ch=%p, name=%s\n",
		ctlb->ccci_ops->ccmni_num, ctlb->ccci_ops->md_ability,
		(ctlb->ccci_ops->md_ability & MODEM_CAP_CCMNI_IRAT),
		ctlb->ccci_ops->irat_md_id, ctlb->ccci_ops->send_pkt,
		ctlb->ccci_ops->get_ccmni_ch, ctlb->ccci_ops->name);

	ccmni_debug_file_init(md_id);

	if ((ctlb->ccci_ops->md_ability & MODEM_CAP_CCMNI_IRAT) == 0) {
		for (i = 0; i < ctlb->ccci_ops->ccmni_num; i++) {
			/* allocate netdev */
			dev = alloc_etherdev(sizeof(ccmni_instance_t));
			if (unlikely(dev == NULL)) {
				CCMNI_ERR_MSG(md_id, "alloc netdev fail\n");
				ret = -ENOMEM;
				goto alloc_netdev_fail;
			}

			/* init net device */
			dev->header_ops = NULL;
			dev->mtu = CCMNI_MTU;
			dev->tx_queue_len = CCMNI_TX_QUEUE;
			dev->watchdog_timeo = CCMNI_NETDEV_WDT_TO;
			dev->flags = IFF_NOARP & /* ccmni is a pure IP device */
					(~IFF_BROADCAST & ~IFF_MULTICAST);	/* ccmni is P2P */
			dev->features = NETIF_F_VLAN_CHALLENGED; /* not support VLAN */
			if (ctlb->ccci_ops->md_ability & MODEM_CAP_SGIO) {
				dev->features |= NETIF_F_SG;
				dev->hw_features |= NETIF_F_SG;
			}
			dev->addr_len = ETH_ALEN; /* ethernet header size */
			dev->destructor = free_netdev;
			dev->hard_header_len += sizeof(struct ccci_header); /* reserve Tx CCCI header room */
			dev->netdev_ops = &ccmni_netdev_ops;
			random_ether_addr((u8 *) dev->dev_addr);

			sprintf(dev->name, "%s%d", ctlb->ccci_ops->name, i);
			CCMNI_INF_MSG(md_id, "register netdev name: %s\n", dev->name);

			/* init private structure of netdev */
			ccmni = netdev_priv(dev);
			ccmni->index = i;
			ret = ccmni_inst_init(md_id, ccmni, dev);
			if (ret) {
				CCMNI_ERR_MSG(md_id, "initial ccmni instance fail\n");
				goto alloc_netdev_fail;
			}
			ctlb->ccmni_inst[i] = ccmni;

			/* register net device */
			ret = register_netdev(dev);
			if (ret) {
				CCMNI_ERR_MSG(md_id, "CCMNI%d register netdev fail: %d\n", i, ret);
				goto alloc_netdev_fail;
			}

			CCMNI_DBG_MSG(ccmni->md_id, "CCMNI%d=%p, ctlb=%p, ctlb_ops=%p, dev=%p\n",
				i, ccmni, ccmni->ctlb, ccmni->ctlb->ccci_ops, ccmni->dev);
		}
	} else {
		if (ctlb->ccci_ops->irat_md_id < 0 || ctlb->ccci_ops->irat_md_id > MAX_MD_NUM) {
			CCMNI_ERR_MSG(md_id, "md%d IRAT fail because invalid irat md(%d)\n",
				md_id, ctlb->ccci_ops->irat_md_id);
			ret = -EINVAL;
			goto alloc_mem_fail;
		}

		ctlb_irat_src = ccmni_ctl_blk[ctlb->ccci_ops->irat_md_id];
		if (!ctlb_irat_src) {
			CCMNI_ERR_MSG(md_id, "md%d IRAT fail because irat md%d ctlb is NULL\n",
				md_id, ctlb->ccci_ops->irat_md_id);
			ret = -EINVAL;
			goto alloc_mem_fail;
		}

		if (unlikely(ctlb->ccci_ops->ccmni_num > ctlb_irat_src->ccci_ops->ccmni_num)) {
			CCMNI_ERR_MSG(md_id, "IRAT fail because number of src(%d) and dest(%d) ccmni isn't equal\n",
				ctlb_irat_src->ccci_ops->ccmni_num, ctlb->ccci_ops->ccmni_num);
			ret = -EINVAL;
			goto alloc_mem_fail;
		}

		for (i = 0; i < ctlb->ccci_ops->ccmni_num; i++) {
			ccmni = kzalloc(sizeof(ccmni_instance_t), GFP_KERNEL);
			ccmni_irat_src = kzalloc(sizeof(ccmni_instance_t), GFP_KERNEL);
			if (unlikely(ccmni == NULL || ccmni_irat_src == NULL)) {
				CCMNI_ERR_MSG(md_id, "alloc ccmni instance fail\n");
				ret = -ENOMEM;
				goto alloc_mem_fail;
			}

			/* initial irat ccmni instance */
			ccmni->index = i;
			dev = ctlb_irat_src->ccmni_inst[i]->dev;
			ret = ccmni_inst_init(md_id, ccmni, dev);
			if (ret) {
				CCMNI_ERR_MSG(md_id, "initial ccmni instance fail\n");
				goto alloc_mem_fail;
			}
			ctlb->ccmni_inst[i] = ccmni;

			/* initial irat source ccmni instance */
			memcpy(ccmni_irat_src, ctlb_irat_src->ccmni_inst[i], sizeof(ccmni_instance_t));
			ctlb_irat_src->ccmni_inst[i] = ccmni_irat_src;
			CCMNI_DBG_MSG(ccmni->md_id, "[IRAT]CCMNI%d=%p, ctlb=%p, ctlb_ops=%p, dev=%p\n",
				i, ccmni, ccmni->ctlb, ccmni->ctlb->ccci_ops, ccmni->dev);
		}
	}

	snprintf(ctlb->wakelock_name, sizeof(ctlb->wakelock_name), "ccmni_md%d", (md_id+1));
	wake_lock_init(&ctlb->ccmni_wakelock, WAKE_LOCK_SUSPEND, ctlb->wakelock_name);

	return 0;

alloc_netdev_fail:
	if (dev) {
		free_netdev(dev);
		ctlb->ccmni_inst[i] = NULL;
	}

	for (j = i-1; j >= 0; j--) {
		ccmni = ctlb->ccmni_inst[j];
		unregister_netdev(ccmni->dev);
		/* free_netdev(ccmni->dev); */
		ctlb->ccmni_inst[j] = NULL;
	}

alloc_mem_fail:
	kfree(ctlb->ccci_ops);
	kfree(ctlb);

	ccmni_ctl_blk[md_id] = NULL;
	return ret;
}

static void ccmni_exit(int md_id)
{
	int i = 0;
	ccmni_ctl_block_t *ctlb = NULL;
	ccmni_instance_t  *ccmni = NULL;

	CCMNI_INF_MSG(md_id, "ccmni_exit\n");

	ctlb = ccmni_ctl_blk[md_id];
	if (ctlb) {
		if (ctlb->ccci_ops == NULL)
			goto ccmni_exit_ret;

		for (i = 0; i < ctlb->ccci_ops->ccmni_num; i++) {
			ccmni = ctlb->ccmni_inst[i];
			if (ccmni) {
				CCMNI_INF_MSG(md_id, "ccmni_exit: unregister ccmni%d dev\n", i);
				unregister_netdev(ccmni->dev);
				/* free_netdev(ccmni->dev); */
				ctlb->ccmni_inst[i] = NULL;
			}
		}

		kfree(ctlb->ccci_ops);

ccmni_exit_ret:
		kfree(ctlb);
		ccmni_ctl_blk[md_id] = NULL;
	}
}

static int ccmni_rx_callback(int md_id, int rx_ch, struct sk_buff *skb, void *priv_data)
{
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];
/* struct ccci_header *ccci_h = (struct ccci_header*)skb->data; */
	ccmni_instance_t *ccmni = NULL;
	struct net_device *dev = NULL;
	int pkt_type, skb_len, ccmni_idx;

	if (unlikely(ctlb == NULL || ctlb->ccci_ops == NULL)) {
		CCMNI_ERR_MSG(md_id, "invalid CCMNI ctrl/ops struct for RX_CH(%d)\n", rx_ch);
		dev_kfree_skb(skb);
		return -1;
	}

	ccmni_idx = get_ccmni_idx_from_ch(md_id, rx_ch);
	if (unlikely(ccmni_idx < 0)) {
		CCMNI_ERR_MSG(md_id, "CCMNI rx(%d) skb ch error\n", rx_ch);
		dev_kfree_skb(skb);
		return -1;
	}
	ccmni = ctlb->ccmni_inst[ccmni_idx];
	dev = ccmni->dev;

/* skb_pull(skb, sizeof(struct ccci_header)); */
	pkt_type = skb->data[0] & 0xF0;
	ccmni_make_etherframe(skb->data-ETH_HLEN, dev->dev_addr, pkt_type);
	skb_set_mac_header(skb, -ETH_HLEN);
	skb->dev = dev;
	if (pkt_type == 0x60)
		skb->protocol  = htons(ETH_P_IPV6);
	else
		skb->protocol  = htons(ETH_P_IP);

	skb->ip_summed = CHECKSUM_NONE;
	skb_len = skb->len;

	if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_RX))
		CCMNI_INF_MSG(md_id, "[RX]CCMNI%d(rx_ch=%d) recv data_len=%d\n", ccmni_idx, rx_ch, skb->len);

	if (unlikely(ccmni_debug_level&CCMNI_DBG_LEVEL_RX_SKB))
		ccmni_dbg_skb_header(ccmni->md_id, false, skb);

	if (likely(ctlb->ccci_ops->md_ability & MODEM_CAP_NAPI)) {
		netif_receive_skb(skb);
	} else {
		if (!in_interrupt())
			netif_rx_ni(skb);
		else
			netif_rx(skb);
	}
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb_len;

	wake_lock_timeout(&ctlb->ccmni_wakelock, HZ);

	return 0;
}


static void ccmni_md_state_callback(int md_id, int rx_ch, MD_STATE state)
{
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];
	ccmni_instance_t  *ccmni = NULL;
	int ccmni_idx = 0;

	if (unlikely(ctlb == NULL)) {
		CCMNI_ERR_MSG(md_id, "invalid ccmni ctrl struct when rx_ch=%d md_sta=%d\n", rx_ch, state);
		return;
	}

	ccmni_idx = get_ccmni_idx_from_ch(md_id, rx_ch);
	if (unlikely(ccmni_idx < 0)) {
		CCMNI_ERR_MSG(md_id, "get error ccmni index when md_sta=%d\n", state);
		return;
	}

	ccmni = ctlb->ccmni_inst[ccmni_idx];

	if ((state != TX_IRQ) && (state != TX_FULL) && (atomic_read(&ccmni->usage) > 0)) {
		CCMNI_INF_MSG(md_id, "md_state_cb: CCMNI%d, md_sta=%d, usage=%d\n",
			ccmni_idx, state, atomic_read(&ccmni->usage));
	}

	switch (state) {
	case READY:
		netif_carrier_on(ccmni->dev);
		ccmni->tx_seq_num[0] = 0;
		ccmni->tx_seq_num[1] = 0;
		ccmni->rx_seq_num = 0;
		break;

	case EXCEPTION:
	case RESET:
		netif_carrier_off(ccmni->dev);
		break;

	case RX_IRQ:
		mod_timer(&ccmni->timer, jiffies+HZ);
		napi_schedule(&ccmni->napi);
		wake_lock_timeout(&ctlb->ccmni_wakelock, HZ);
		break;

	case TX_IRQ:
		if (netif_running(ccmni->dev) && netif_queue_stopped(ccmni->dev)
			&& atomic_read(&ccmni->usage) > 0) {
			netif_wake_queue(ccmni->dev);
			/* ccmni->flags &= ~PORT_F_RX_FULLED; */
			CCMNI_INF_MSG(md_id, "md_state_cb: CCMNI%d, md_sta=TX_IRQ, usage=%d\n",
				ccmni_idx, atomic_read(&ccmni->usage));
		}
		break;

	case TX_FULL:
		netif_stop_queue(ccmni->dev);
		CCMNI_INF_MSG(md_id, "md_state_cb: CCMNI%d, md_sta=TX_FULL, usage=%d\n",
			ccmni_idx, atomic_read(&ccmni->usage));
		/* ccmni->flags |= PORT_F_RX_FULLED; // for convenient in traffic log */
		break;
	default:
		break;
	}
}

static void ccmni_dump(int md_id, int rx_ch, unsigned int flag)
{
	ccmni_ctl_block_t *ctlb = ccmni_ctl_blk[md_id];
	ccmni_instance_t *ccmni = NULL;
	ccmni_instance_t *ccmni_tmp = NULL;
	int ccmni_idx = 0;
	struct net_device *dev = NULL;
	struct netdev_queue *dev_queue = NULL;

	if (ctlb == NULL)
		return;

	ccmni_idx = get_ccmni_idx_from_ch(md_id, rx_ch);
	if (unlikely(ccmni_idx < 0)) {
		CCMNI_ERR_MSG(md_id, "CCMNI rx(%d) skb ch error\n", rx_ch);
		return;
	}

	ccmni_tmp = ctlb->ccmni_inst[ccmni_idx];
	if (unlikely(ccmni_tmp == NULL))
		return;

	if ((ccmni_tmp->dev->stats.rx_packets == 0) && (ccmni_tmp->dev->stats.tx_packets == 0))
		return;

	dev = ccmni_tmp->dev;
	/*ccmni diff from ccmni_tmp for MD IRAT*/
	ccmni = (ccmni_instance_t *)netdev_priv(dev);
	dev_queue = netdev_get_tx_queue(dev, 0);
	CCMNI_INF_MSG(md_id,
		"CCMNI%d(%d,%d), irat_md=MD%d, rx=(%ld,%ld), tx=(%ld,%ld), txq_len=%d, tx_busy=%ld, dev_sta=(0x%lx,0x%lx,0x%x)\n",
		ccmni->index, atomic_read(&ccmni->usage), atomic_read(&ccmni_tmp->usage), ccmni->md_id,
		dev->stats.rx_packets, dev->stats.rx_bytes, dev->stats.tx_packets, dev->stats.tx_bytes,
		dev->qdisc->q.qlen, ccmni->tx_busy_cnt, dev->state, dev_queue->state, dev->flags);
}


struct ccmni_dev_ops ccmni_ops = {
	.skb_alloc_size = 1600,
	.init = &ccmni_init,
	.rx_callback = &ccmni_rx_callback,
	.md_state_callback = &ccmni_md_state_callback,
	.exit = ccmni_exit,
	.dump = ccmni_dump,
};
