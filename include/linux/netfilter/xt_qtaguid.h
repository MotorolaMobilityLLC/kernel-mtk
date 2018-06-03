#ifndef _XT_QTAGUID_MATCH_H
#define _XT_QTAGUID_MATCH_H

/* For now we just replace the xt_owner.
 * FIXME: make iptables aware of qtaguid. */
#include <linux/netfilter/xt_owner.h>

#define XT_QTAGUID_UID    XT_OWNER_UID
#define XT_QTAGUID_GID    XT_OWNER_GID
#define XT_QTAGUID_SOCKET XT_OWNER_SOCKET
#define xt_qtaguid_match_info xt_owner_match_info

int qtaguid_untag(struct socket *sock, bool kernel);
#ifdef CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT
/*MD Tethering ShareMemory Data*/
struct mdt_data_t {
	uint64_t total_tx_bytes;
	uint64_t total_tx_pkts;
	uint64_t tx_tcp_bytes;
	uint64_t tx_tcp_pkts;
	uint64_t tx_udp_bytes;
	uint64_t tx_udp_pkts;
	uint64_t tx_others_bytes;
	uint64_t tx_others_pkts;
	uint64_t total_rx_bytes;
	uint64_t total_rx_pkts;
	uint64_t rx_tcp_bytes;
	uint64_t rx_tcp_pkts;
	uint64_t rx_udp_bytes;
	uint64_t rx_udp_pkts;
	uint64_t rx_others_bytes;
	uint64_t rx_others_pkts;
};

#endif
#endif /* _XT_QTAGUID_MATCH_H */
