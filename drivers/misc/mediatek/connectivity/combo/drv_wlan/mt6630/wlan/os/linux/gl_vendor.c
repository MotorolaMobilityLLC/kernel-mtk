/*
** gl_vendor.c
**
**
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "precomp.h"
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <net/cfg80211.h>
#include "gl_cfg80211.h"
#include "gl_vendor.h"
#include "wlan_oid.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

P_SW_RFB_T g_arGscnResultsTempBuffer[MAX_BUFFERED_GSCN_RESULTS];
UINT_8 g_GscanResultsTempBufferIndex = 0;
UINT_8 g_arGscanResultsIndicateNumber[MAX_BUFFERED_GSCN_RESULTS] = { 0, 0, 0, 0, 0 };

UINT_8 g_GetResultsBufferedCnt = 0;
UINT_8 g_GetResultsCmdCnt = 0;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
int mtk_cfg80211_NLA_PUT(struct sk_buff *skb, int attrtype, int attrlen, const void *data)
{
	if (unlikely(nla_put(skb, attrtype, attrlen, data) < 0))
		return 0;
	return 1;
}

int mtk_cfg80211_nla_put_type(struct sk_buff *skb, ENUM_NLA_PUT_DATE_TYPE type, int attrtype, const void *value)
{
	u8 u8data = 0;
	u16 u16data = 0;
	u32 u32data = 0;
	u64 u64data = 0;

	switch (attrtype) {
	case NLA_PUT_DATE_U8:
		u8data = *(u8 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u8), &u8data);
	case NLA_PUT_DATE_U16:
		u16data = *(u16 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u16), &u8data);
	case NLA_PUT_DATE_U32:
		u32data = *(u32 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u32), &u8data);
	case NLA_PUT_DATE_U64:
		u64data = *(u64 *)value;
		return mtk_cfg80211_NLA_PUT(skb, attrtype, sizeof(u64), &u8data);
	default:
		break;
	}

	return 0;
}

int mtk_cfg80211_vendor_get_capabilities(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	INT_32 i4Status = -EINVAL;
	PARAM_WIFI_GSCAN_CAPABILITIES_STRUCT_T rGscanCapabilities;
	struct sk_buff *skb;

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	ASSERT(wiphy);
	ASSERT(wdev);
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(rGscanCapabilities));
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed:%x\n", __func__, i4Status);
		return -ENOMEM;
	}

	kalMemZero(&rGscanCapabilities, sizeof(rGscanCapabilities));

	/*rStatus = kalIoctl(prGlueInfo,
	   wlanoidQueryStatistics,
	   &rGscanCapabilities,
	   sizeof(rGscanCapabilities),
	   TRUE,
	   TRUE,
	   TRUE,
	   FALSE,
	   &u4BufLen); */
	rGscanCapabilities.max_scan_cache_size = PSCAN_MAX_SCAN_CACHE_SIZE;
	rGscanCapabilities.max_scan_buckets = GSCAN_MAX_BUCKETS;
	rGscanCapabilities.max_ap_cache_per_scan = PSCAN_MAX_AP_CACHE_PER_SCAN;
	rGscanCapabilities.max_rssi_sample_size = 10;
	rGscanCapabilities.max_scan_reporting_threshold = GSCAN_MAX_REPORT_THRESHOLD;
	rGscanCapabilities.max_hotlist_aps = MAX_HOTLIST_APS;
	rGscanCapabilities.max_significant_wifi_change_aps = MAX_SIGNIFICANT_CHANGE_APS;
	rGscanCapabilities.max_bssid_history_entries = PSCAN_MAX_AP_CACHE_PER_SCAN * PSCAN_MAX_SCAN_CACHE_SIZE;

	/* NLA_PUT_U8(skb, NL80211_TESTMODE_STA_STATISTICS_INVALID, 0); */
	/* NLA_PUT_U32(skb, NL80211_ATTR_VENDOR_ID, GOOGLE_OUI); */
	/* NLA_PUT_U32(skb, NL80211_ATTR_VENDOR_SUBCMD, GSCAN_SUBCMD_GET_CAPABILITIES); */
	if (!NLA_PUT(skb, NL80211_ATTR_VENDOR_CAPABILITIES, sizeof(rGscanCapabilities), &rGscanCapabilities))
		return i4Status;

	i4Status = cfg80211_vendor_cmd_reply(skb);

	DBGLOG(SCN, INFO, "%s for vendor command done  \r\n", __func__);
	return i4Status;

}

int mtk_cfg80211_vendor_set_config(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	WLAN_STATUS rStatus;
	UINT_32 u4BufLen;
	P_GLUE_INFO_T prGlueInfo = NULL;
	/* CMD_GSCN_REQ_T rCmdGscnParam; */

	/* INT_32 i4Status = -EINVAL; */
	P_PARAM_WIFI_GSCAN_CMD_PARAMS prWifiScanCmd;
	struct nlattr *attr[GSCAN_ATTRIBUTE_REPORT_EVENTS + 1];
	struct nlattr *pbucket, *pchannel;
	UINT_32 len_basic, len_bucket, len_channel;
	int i, j, k;
	static struct nla_policy policy[GSCAN_ATTRIBUTE_REPORT_EVENTS + 1] = {
		[GSCAN_ATTRIBUTE_NUM_BUCKETS] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BASE_PERIOD] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BUCKETS_BAND] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BUCKET_ID] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BUCKET_PERIOD] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_BUCKET_CHANNELS] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_REPORT_EVENTS] = {.type = NLA_U32},
	};

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	prWifiScanCmd = (P_PARAM_WIFI_GSCAN_CMD_PARAMS) kalMemAlloc(sizeof(PARAM_WIFI_GSCAN_CMD_PARAMS), VIR_MEM_TYPE);
	if (!prWifiScanCmd) {
		DBGLOG(INIT, INFO, "Can not alloc memory for PARAM_WIFI_GSCAN_CMD_PARAMS\n");
		return -ENOMEM;
	}

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);
	for (i = 0; i < 13; i++)
		DBGLOG(SCN, INFO, "0x%x 0x%x 0x%x 0x%x \r\n", *((UINT_32 *) data + i * 4),
		       *((UINT_32 *) data + i * 4 + 1), *((UINT_32 *) data + i * 4 + 2),
		       *((UINT_32 *) data + i * 4 + 3));
	kalMemZero(prWifiScanCmd, sizeof(PARAM_WIFI_GSCAN_CMD_PARAMS));
	kalMemZero(attr, sizeof(struct nlattr *) * (GSCAN_ATTRIBUTE_REPORT_EVENTS + 1));

	nla_parse_nested(attr, GSCAN_ATTRIBUTE_REPORT_EVENTS, (struct nlattr *)(data - NLA_HDRLEN), policy);
	len_basic = 0;
	for (k = GSCAN_ATTRIBUTE_NUM_BUCKETS; k <= GSCAN_ATTRIBUTE_REPORT_EVENTS; k++) {
		if (attr[k]) {
			switch (k) {
			case GSCAN_ATTRIBUTE_BASE_PERIOD:
				prWifiScanCmd->base_period = nla_get_u32(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_NUM_BUCKETS:
				prWifiScanCmd->num_buckets = nla_get_u32(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				DBGLOG(SCN, INFO, "attr=0x%x, num_buckets=%d nla_len=%d, \r\n",
				       *(UINT_32 *) attr[k], prWifiScanCmd->num_buckets, attr[k]->nla_len);
				break;
			}
		}
	}
	pbucket = (struct nlattr *)((UINT_8 *) data + len_basic);
	/* printk("+++basic attribute size=%d pbucket=%p\r\n", len_basic, pbucket); */

	for (i = 0; i < prWifiScanCmd->num_buckets; i++) {
		nla_parse_nested(attr, GSCAN_ATTRIBUTE_REPORT_EVENTS, (struct nlattr *)pbucket, policy);
		len_bucket = 0;
		for (k = GSCAN_ATTRIBUTE_NUM_BUCKETS; k <= GSCAN_ATTRIBUTE_REPORT_EVENTS; k++) {
			if (attr[k]) {
				switch (k) {
				case GSCAN_ATTRIBUTE_BUCKETS_BAND:
					prWifiScanCmd->buckets[i].band = nla_get_u32(attr[k]);
					len_bucket += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_BUCKET_ID:
					prWifiScanCmd->buckets[i].bucket = nla_get_u32(attr[k]);
					len_bucket += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_BUCKET_PERIOD:
					prWifiScanCmd->buckets[i].period = nla_get_u32(attr[k]);
					len_bucket += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_REPORT_EVENTS:
					prWifiScanCmd->buckets[i].report_events = nla_get_u32(attr[k]);
					len_bucket += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS:
					prWifiScanCmd->buckets[i].num_channels = nla_get_u32(attr[k]);
					len_bucket += NLA_ALIGN(attr[k]->nla_len);
					DBGLOG(SCN, INFO,
					       "bucket%d: attr=0x%x, num_channels=%d nla_len = %d, \r\n",
					       i, *(UINT_32 *) attr[k], nla_get_u32(attr[k]), attr[k]->nla_len);
					break;
				}
			}
		}
		pbucket = (struct nlattr *)((UINT_8 *) pbucket + NLA_HDRLEN);
		/* request.attr_start(i) as nested attribute */
		pbucket = (struct nlattr *)((UINT_8 *) pbucket + len_bucket);
		/* pure bucket payload, not include channels */

		/*don't need to use nla_parse_nested to parse channels */
		/* the header of channel in bucket i */
		pchannel = (struct nlattr *)((UINT_8 *) pbucket + NLA_HDRLEN);
		for (j = 0; j < prWifiScanCmd->buckets[i].num_channels; j++) {
			prWifiScanCmd->buckets[i].channels[j].channel = nla_get_u32(pchannel);
			len_channel = NLA_ALIGN(pchannel->nla_len);
			DBGLOG(SCN, INFO, "attr=0x%x, channel=%d, \r\n", *(UINT_32 *) pchannel, nla_get_u32(pchannel));

			pchannel = (struct nlattr *)((UINT_8 *) pchannel + len_channel);
		}
		pbucket = pchannel;
	}

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetGSCNAParam,
			   prWifiScanCmd, sizeof(PARAM_WIFI_GSCAN_CMD_PARAMS), FALSE, FALSE, TRUE, &u4BufLen);

	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_set_scan_config(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	WLAN_STATUS rStatus;
	UINT_32 u4BufLen;
	P_GLUE_INFO_T prGlueInfo = NULL;

	PARAM_WIFI_GSCAN_CMD_PARAMS rWifiScanCmd;
	struct nlattr *attr[GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE + 1];
	int i, k;
	static struct nla_policy policy[GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE + 1] = {
		[GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_REPORT_THRESHOLD] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE] = {.type = NLA_U32},
	};

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);
	for (i = 0; i < 2; i++)
		DBGLOG(SCN, INFO, "0x%x 0x%x 0x%x 0x%x \r\n", *((UINT_32 *) data + i * 4),
		       *((UINT_32 *) data + i * 4 + 1), *((UINT_32 *) data + i * 4 + 2),
		       *((UINT_32 *) data + i * 4 + 3));
	kalMemZero(&rWifiScanCmd, sizeof(rWifiScanCmd));
	kalMemZero(attr, sizeof(struct nlattr *) * (GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE + 1));

	nla_parse_nested(attr, GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE, (struct nlattr *)(data - NLA_HDRLEN), policy);
	for (k = GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN; k <= GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE; k++) {
		if (attr[k]) {
			switch (k) {
			case GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN:
				rWifiScanCmd.max_ap_per_scan = nla_get_u32(attr[k]);
				break;
			case GSCAN_ATTRIBUTE_REPORT_THRESHOLD:
				rWifiScanCmd.report_threshold = nla_get_u32(attr[k]);
				break;
			case GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE:
				rWifiScanCmd.num_scans = nla_get_u32(attr[k]);
				break;
			}
		}
	}
	DBGLOG(SCN, INFO, "attr=0x%x, attr2=0x%x ",
	       *(UINT_32 *) attr[GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN], *(UINT_32 *) attr[GSCAN_ATTRIBUTE_REPORT_THRESHOLD]);

	DBGLOG(SCN, INFO, "max_ap_per_scan=%d, report_threshold=%d num_scans=%d \r\n",
	       rWifiScanCmd.max_ap_per_scan, rWifiScanCmd.report_threshold, rWifiScanCmd.num_scans);

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetGSCNAConfig,
			   &rWifiScanCmd, sizeof(PARAM_WIFI_GSCAN_CMD_PARAMS), FALSE, FALSE, TRUE, &u4BufLen);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_set_significant_change(struct wiphy *wiphy, struct wireless_dev *wdev,
					       const void *data, int data_len)
{
	PARAM_WIFI_SIGNIFICANT_CHANGE rWifiChangeCmd;
	UINT_8 flush = 0;
	struct nlattr *attr[GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1];
	struct nlattr *paplist;
	int i, k;
	UINT_32 len_basic, len_aplist;
	static struct nla_policy policy[GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1] = {
		[GSCAN_ATTRIBUTE_BSSID] = {.type = NLA_UNSPEC},
		[GSCAN_ATTRIBUTE_RSSI_LOW] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_RSSI_HIGH] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE] = {.type = NLA_U16},
		[GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE] = {.type = NLA_U16},
		[GSCAN_ATTRIBUTE_MIN_BREACHING] = {.type = NLA_U16},
		[GSCAN_ATTRIBUTE_NUM_AP] = {.type = NLA_U16},
		[GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH] = {.type = NLA_U8},
	};

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);
	for (i = 0; i < 6; i++)
		DBGLOG(SCN, INFO, "0x%x 0x%x 0x%x 0x%x \r\n", *((UINT_32 *) data + i * 4),
		       *((UINT_32 *) data + i * 4 + 1), *((UINT_32 *) data + i * 4 + 2),
		       *((UINT_32 *) data + i * 4 + 3));
	kalMemZero(&rWifiChangeCmd, sizeof(rWifiChangeCmd));
	kalMemZero(attr, sizeof(struct nlattr *) * (GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1));

	nla_parse_nested(attr, GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH, (struct nlattr *)(data - NLA_HDRLEN), policy);
	len_basic = 0;
	for (k = GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE; k <= GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH; k++) {
		if (attr[k]) {
			switch (k) {
			case GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE:
				rWifiChangeCmd.rssi_sample_size = nla_get_u16(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE:
				rWifiChangeCmd.lost_ap_sample_size = nla_get_u16(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_MIN_BREACHING:
				rWifiChangeCmd.min_breaching = nla_get_u16(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_NUM_AP:
				rWifiChangeCmd.num_ap = nla_get_u16(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				DBGLOG(SCN, INFO, "attr=0x%x, num_ap=%d nla_len=%d, \r\n",
				       *(UINT_32 *) attr[k], rWifiChangeCmd.num_ap, attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH:
				flush = nla_get_u8(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			}
		}
	}
	paplist = (struct nlattr *)((UINT_8 *) data + len_basic);
	DBGLOG(SCN, INFO, "+++basic attribute size=%d flush=%d\r\n", len_basic, flush);

	if (paplist->nla_type == GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS)
		paplist = (struct nlattr *)((UINT_8 *) paplist + NLA_HDRLEN);

	for (i = 0; i < rWifiChangeCmd.num_ap; i++) {
		nla_parse_nested(attr, GSCAN_ATTRIBUTE_RSSI_HIGH, (struct nlattr *)paplist, policy);
		paplist = (struct nlattr *)((UINT_8 *) paplist + NLA_HDRLEN);
		/* request.attr_start(i) as nested attribute */
		len_aplist = 0;
		for (k = GSCAN_ATTRIBUTE_BSSID; k <= GSCAN_ATTRIBUTE_RSSI_HIGH; k++) {
			if (attr[k]) {
				switch (k) {
				case GSCAN_ATTRIBUTE_BSSID:
					kalMemCopy(rWifiChangeCmd.ap[i].bssid, nla_data(attr[k]), sizeof(mac_addr));
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_RSSI_LOW:
					rWifiChangeCmd.ap[i].low = nla_get_u32(attr[k]);
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_RSSI_HIGH:
					rWifiChangeCmd.ap[i].high = nla_get_u32(attr[k]);
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				}
			}
		}
		if (((i + 1) % 4 == 0) || (i == rWifiChangeCmd.num_ap - 1))
			DBGLOG(SCN, INFO, "ap[%d], len_aplist=%d\n", i, len_aplist);
		else
			DBGLOG(SCN, INFO, "ap[%d], len_aplist=%d \t", i, len_aplist);

		paplist = (struct nlattr *)((UINT_8 *) paplist + len_aplist);
	}

	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_set_hotlist(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	CMD_SET_PSCAN_ADD_HOTLIST_BSSID rCmdPscnAddHotlist;

	PARAM_WIFI_BSSID_HOTLIST rWifiHotlistCmd;
	UINT_8 flush = 0;
	struct nlattr *attr[GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1];
	struct nlattr *paplist;
	int i, k;
	UINT_32 len_basic, len_aplist;
	static struct nla_policy policy[GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1] = {
		[GSCAN_ATTRIBUTE_BSSID] = {.type = NLA_UNSPEC},
		[GSCAN_ATTRIBUTE_RSSI_LOW] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_RSSI_HIGH] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE] = {.type = NLA_U32},
		[GSCAN_ATTRIBUTE_NUM_AP] = {.type = NLA_U16},
		[GSCAN_ATTRIBUTE_HOTLIST_FLUSH] = {.type = NLA_U8},
	};

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);
	for (i = 0; i < 5; i++)
		DBGLOG(SCN, INFO, "0x%x 0x%x 0x%x 0x%x \r\n", *((UINT_32 *) data + i * 4),
		       *((UINT_32 *) data + i * 4 + 1), *((UINT_32 *) data + i * 4 + 2),
		       *((UINT_32 *) data + i * 4 + 3));
	kalMemZero(&rWifiHotlistCmd, sizeof(rWifiHotlistCmd));
	kalMemZero(attr, sizeof(struct nlattr *) * (GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH + 1));

	nla_parse_nested(attr, GSCAN_ATTRIBUTE_NUM_AP, (struct nlattr *)(data - NLA_HDRLEN), policy);
	len_basic = 0;
	for (k = GSCAN_ATTRIBUTE_HOTLIST_FLUSH; k <= GSCAN_ATTRIBUTE_NUM_AP; k++) {
		if (attr[k]) {
			switch (k) {
			case GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE:
				rWifiHotlistCmd.lost_ap_sample_size = nla_get_u32(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_NUM_AP:
				rWifiHotlistCmd.num_ap = nla_get_u16(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				DBGLOG(SCN, INFO, "attr=0x%x, num_ap=%d nla_len=%d, \r\n",
				       *(UINT_32 *) attr[k], rWifiHotlistCmd.num_ap, attr[k]->nla_len);
				break;
			case GSCAN_ATTRIBUTE_HOTLIST_FLUSH:
				flush = nla_get_u8(attr[k]);
				len_basic += NLA_ALIGN(attr[k]->nla_len);
				break;
			}
		}
	}
	paplist = (struct nlattr *)((UINT_8 *) data + len_basic);
	DBGLOG(SCN, INFO, "+++basic attribute size=%d flush=%d\r\n", len_basic, flush);

	if (paplist->nla_type == GSCAN_ATTRIBUTE_HOTLIST_BSSIDS)
		paplist = (struct nlattr *)((UINT_8 *) paplist + NLA_HDRLEN);

	for (i = 0; i < rWifiHotlistCmd.num_ap; i++) {
		nla_parse_nested(attr, GSCAN_ATTRIBUTE_RSSI_HIGH, (struct nlattr *)paplist, policy);
		paplist = (struct nlattr *)((UINT_8 *) paplist + NLA_HDRLEN);
		/* request.attr_start(i) as nested attribute */
		len_aplist = 0;
		for (k = GSCAN_ATTRIBUTE_BSSID; k <= GSCAN_ATTRIBUTE_RSSI_HIGH; k++) {
			if (attr[k]) {
				switch (k) {
				case GSCAN_ATTRIBUTE_BSSID:
					kalMemCopy(rWifiHotlistCmd.ap[i].bssid, nla_data(attr[k]), sizeof(mac_addr));
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_RSSI_LOW:
					rWifiHotlistCmd.ap[i].low = nla_get_u32(attr[k]);
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				case GSCAN_ATTRIBUTE_RSSI_HIGH:
					rWifiHotlistCmd.ap[i].high = nla_get_u32(attr[k]);
					len_aplist += NLA_ALIGN(attr[k]->nla_len);
					break;
				}
			}
		}
		if (((i + 1) % 4 == 0) || (i == rWifiHotlistCmd.num_ap - 1))
			DBGLOG(SCN, INFO, "ap[%d], len_aplist=%d\n", i, len_aplist);
		else
			DBGLOG(SCN, INFO, "ap[%d], len_aplist=%d \t", i, len_aplist);

		paplist = (struct nlattr *)((UINT_8 *) paplist + len_aplist);
	}

	memcpy(&(rCmdPscnAddHotlist.aucMacAddr), &(rWifiHotlistCmd.ap[0].bssid), 6 * sizeof(UINT_8));
	rCmdPscnAddHotlist.ucFlags = (UINT_8) TRUE;
	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_enable_scan(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	WLAN_STATUS rStatus;
	UINT_32 u4BufLen;
	P_GLUE_INFO_T prGlueInfo = NULL;
	PARAM_WIFI_GSCAN_ACTION_CMD_PARAMS rWifiScanActionCmd;

	struct nlattr *attr;
	UINT_8 gGScanEn = 0;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d, data=0x%x 0x%x\r\n",
	       __func__, data_len, *((UINT_32 *) data), *((UINT_32 *) data + 1));

	attr = (struct nlattr *)data;
	if (attr->nla_type == GSCAN_ATTRIBUTE_ENABLE_FEATURE)
		gGScanEn = nla_get_u32(attr);
	DBGLOG(SCN, INFO, "gGScanEn=%d, \r\n", gGScanEn);

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);
	if (gGScanEn == TRUE)
		rWifiScanActionCmd.ucPscanAct = ENABLE;
	else
		rWifiScanActionCmd.ucPscanAct = DISABLE;

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetGSCNAction,
			   &rWifiScanActionCmd,
			   sizeof(PARAM_WIFI_GSCAN_ACTION_CMD_PARAMS), FALSE, FALSE, TRUE, &u4BufLen);

	/* mtk_cfg80211_vendor_get_scan_results(wiphy, wdev, data, data_len ); */

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_enable_full_scan_results(struct wiphy *wiphy, struct wireless_dev *wdev,
						 const void *data, int data_len)
{
	struct nlattr *attr;
	UINT_8 gFullScanResultsEn = 0;

	ASSERT(wiphy);
	ASSERT(wdev);
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d, data=0x%x 0x%x\r\n",
	       __func__, data_len, *((UINT_32 *) data), *((UINT_32 *) data + 1));

	attr = (struct nlattr *)data;
	if (attr->nla_type == GSCAN_ENABLE_FULL_SCAN_RESULTS)
		gFullScanResultsEn = nla_get_u32(attr);
	DBGLOG(SCN, INFO, "gFullScanResultsEn=%d, \r\n", gFullScanResultsEn);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_get_scan_results(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	UINT_32 u4BufLen;
	P_GLUE_INFO_T prGlueInfo = NULL;
	PARAM_WIFI_GSCAN_GET_RESULT_PARAMS rGSscnResultParm;

	struct nlattr *attr;
	UINT_32 get_num, real_num;
	UINT_8 flush = 0;
	int i;

	ASSERT(wiphy);
	ASSERT(wdev);
	get_num = 0;
	real_num = 0;
	if ((data == NULL) || !data_len)
		goto nla_put_failure;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);
	for (i = 0; i < 2; i++)
		DBGLOG(SCN, INFO, "0x%x 0x%x 0x%x 0x%x \r\n", *((UINT_32 *) data + i * 4),
		       *((UINT_32 *) data + i * 4 + 1), *((UINT_32 *) data + i * 4 + 2),
		       *((UINT_32 *) data + i * 4 + 3));

	attr = (struct nlattr *)data;
	if (attr->nla_type == GSCAN_ATTRIBUTE_NUM_OF_RESULTS) {
		get_num = nla_get_u32(attr);
		attr = (struct nlattr *)((UINT_8 *) attr + attr->nla_len);
	}
	if (attr->nla_type == GSCAN_ATTRIBUTE_FLUSH_RESULTS) {
		flush = nla_get_u8(attr);
		attr = (struct nlattr *)((UINT_8 *) attr + attr->nla_len);
	}
	DBGLOG(SCN, INFO, "number=%d, flush=%d \r\n", get_num, flush);

	prGlueInfo = (P_GLUE_INFO_T) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	real_num = (get_num < PSCAN_MAX_SCAN_CACHE_SIZE) ? get_num : PSCAN_MAX_SCAN_CACHE_SIZE;
	get_num = real_num;

	rGSscnResultParm.get_num = get_num;
	rGSscnResultParm.flush = flush;

	DBGLOG(SCN, INFO, "no buffered gscn results, ask form FW\n");
	kalIoctl(prGlueInfo,
		wlanoidGetGSCNResult,
		&rGSscnResultParm, sizeof(PARAM_WIFI_GSCAN_GET_RESULT_PARAMS), FALSE, FALSE, TRUE, &u4BufLen);

	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);

	scnGscnGetResultReplyCheck(prGlueInfo->prAdapter);

	return 0;

nla_put_failure:
	return -1;
}

int mtk_cfg80211_vendor_get_channel_list(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	INT_32 i4Status = -EINVAL;
	struct nlattr *attr;
	UINT_32 band = 0;
	wifi_channel channels[4];
	struct sk_buff *skb;

	ASSERT(wiphy);
	if ((data == NULL) || !data_len)
		return i4Status;

	DBGLOG(SCN, INFO, "%s for vendor command: data_len=%d \r\n", __func__, data_len);

	attr = (struct nlattr *)data;
	if (attr->nla_type == GSCAN_ATTRIBUTE_BAND)
		band = nla_get_u32(attr);

	DBGLOG(SCN, INFO, "get channel list: band=%d \r\n", band);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(wifi_channel) * 4);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed:%x\n", __func__, i4Status);
		return -ENOMEM;
	}

	kalMemZero(channels, sizeof(wifi_channel) * 4);

	i4Status = cfg80211_vendor_cmd_reply(skb);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return i4Status;
}

#if 0
int mtk_cfg80211_vendor_llstats_get_info(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len)
{
	INT_32 i4Status = -EINVAL;
	WIFI_RADIO_STAT *pRadioStat;
	struct sk_buff *skb;
	UINT_32 u4BufLen;

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	ASSERT(wiphy);
	ASSERT(wdev);

	u4BufLen = sizeof(WIFI_RADIO_STAT) + sizeof(WIFI_IFACE_STAT);
	pRadioStat = kalMemAlloc(u4BufLen, VIR_MEM_TYPE);
	kalMemZero(pRadioStat, u4BufLen);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, u4BufLen);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed:%x\n", __func__, i4Status);
		return -ENOMEM;
	}

	/*rStatus = kalIoctl(prGlueInfo,
	   wlanoidQueryStatistics,
	   &rRadioStat,
	   sizeof(rRadioStat),
	   TRUE,
	   TRUE,
	   TRUE,
	   FALSE,
	   &u4BufLen); */
	/* only for test */
	pRadioStat->radio = 10;
	pRadioStat->on_time = 11;
	pRadioStat->tx_time = 12;
	pRadioStat->num_channels = 4;

	NLA_PUT(skb, NL80211_ATTR_VENDOR_LLSTAT, u4BufLen, pRadioStat);

	i4Status = cfg80211_vendor_cmd_reply(skb);

	kalMemFree(pRadioStat, VIR_MEM_TYPE, u4BufLen);

	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

nla_put_failure:
	return i4Status;
}
#endif

int mtk_cfg80211_vendor_event_complete_scan(struct wiphy *wiphy, struct wireless_dev *wdev, WIFI_SCAN_EVENT complete)
{
	struct sk_buff *skb;

	ASSERT(wiphy);
	/* WIFI_SCAN_EVENT complete_scan; */

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(complete), GSCAN_EVENT_COMPLETE_SCAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}
	/* complete_scan = WIFI_SCAN_COMPLETE; */
	if (!NLA_PUT_U32(skb, GSCAN_EVENT_COMPLETE_SCAN, &complete))
		return -1;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

}

int mtk_cfg80211_vendor_event_scan_results_available(struct wiphy *wiphy, struct wireless_dev *wdev, UINT_32 num)
{
	struct sk_buff *skb;

	ASSERT(wiphy);
	/* UINT_32 scan_result; */

	DBGLOG(SCN, INFO, "%s for vendor command %d  \r\n", __func__, num);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(num), GSCAN_EVENT_SCAN_RESULTS_AVAILABLE, GFP_KERNEL);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}
	/* scan_result = 2; */
	if (!NLA_PUT_U32(skb, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE, &num))
		return -1;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

}

int mtk_cfg80211_vendor_event_full_scan_results(struct wiphy *wiphy, struct wireless_dev *wdev,
						P_PARAM_WIFI_GSCAN_RESULT pdata, UINT_32 data_len)
{
	PARAM_WIFI_GSCAN_RESULT result;
	struct sk_buff *skb;

	ASSERT(wiphy);

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(result), GSCAN_EVENT_FULL_SCAN_RESULTS, GFP_KERNEL);

#if 1
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(&result, sizeof(result));
#endif

	/* kalMemCopy(&result, pdata, sizeof(PARAM_WIFI_GSCAN_RESULT); */
	if (!NLA_PUT(skb, GSCAN_EVENT_FULL_SCAN_RESULTS, sizeof(result), &result))
		return -1;

	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

}

int mtk_cfg80211_vendor_event_significant_change_results(struct wiphy *wiphy,
							 struct wireless_dev *wdev,
							 P_PARAM_WIFI_CHANGE_RESULT pdata, UINT_32 data_len)
{
	struct sk_buff *skb;
	PARAM_WIFI_CHANGE_RESULT result[2], *presult;

	ASSERT(wiphy);

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(PARAM_WIFI_CHANGE_RESULT),
					  GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS, GFP_KERNEL);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	presult = result;
	kalMemZero(presult, (sizeof(PARAM_WIFI_CHANGE_RESULT) * 2));
	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

}

int mtk_cfg80211_vendor_event_hotlist_ap_found(struct wiphy *wiphy, struct wireless_dev *wdev,
					       P_PARAM_WIFI_GSCAN_RESULT pdata, UINT_32 data_len)
{
	struct sk_buff *skb;
	PARAM_WIFI_GSCAN_RESULT result[2], *presult;

	ASSERT(wiphy);

	DBGLOG(SCN, INFO, "%s for vendor command\r\n", __func__);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(PARAM_WIFI_GSCAN_RESULT),
					 GSCAN_EVENT_HOTLIST_RESULTS_FOUND, GFP_KERNEL);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	presult = result;
	kalMemZero(presult, (sizeof(PARAM_WIFI_GSCAN_RESULT) * 2));
	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;

}

int mtk_cfg80211_vendor_event_hotlist_ap_lost(struct wiphy *wiphy, struct wireless_dev *wdev,
					      P_PARAM_WIFI_GSCAN_RESULT pdata, UINT_32 data_len)
{
	struct sk_buff *skb;
	PARAM_WIFI_GSCAN_RESULT result[2], *presult;

	ASSERT(wiphy);

	DBGLOG(SCN, INFO, "%s for vendor command \r\n", __func__);

	skb = cfg80211_vendor_event_alloc(wiphy, sizeof(PARAM_WIFI_GSCAN_RESULT),
					  GSCAN_EVENT_HOTLIST_RESULTS_LOST, GFP_KERNEL);
	if (!skb) {
		DBGLOG(INIT, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	presult = result;
	kalMemZero(presult, (sizeof(PARAM_WIFI_GSCAN_RESULT) * 2));
	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(SCN, INFO, "%s for vendor command done\r\n", __func__);
	return 0;
}
