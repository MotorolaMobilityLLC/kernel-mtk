/*
** Id: @(#) gl_cfg80211.c@@
*/

/*! \file   gl_cfg80211.c
    \brief  Main routines for supporintg MT6620 cfg80211 control interface

    This file contains the support routines of Linux driver for MediaTek Inc. 802.11
    Wireless LAN Adapters.
*/

/*
** Log: gl_cfg80211.c
**
** 09 05 2013 cp.wu
** correct length to pass to wlanoidSetBssid()
**
** 09 04 2013 cp.wu
** fix typo
**
** 09 03 2013 cp.wu
** add path for reassociation
**
** 11 23 2012 yuche.tsai
** [ALPS00398671] [Acer-Tablet] Remove Wi-Fi Direct completely
** Fix bug of WiFi may reboot under user load, when WiFi Direct is removed..
**
** 09 12 2012 wcpadmin
** [ALPS00276400] Remove MTK copyright and legal header on GPL/LGPL related packages
** .
**
** 08 30 2012 chinglan.wang
** [ALPS00349664] [6577JB][WIFI] Phone can not connect to AP secured with AES via WPS in 802.11n Only
** .
 *
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

/* workaround for some ANR CRs. if suppliant is blocked longer than 10s, wifi hal will tell wifiMonitor
to teminate. for the case which can block supplicant 10s is to del key more than 5 times. the root cause
is that there is no resource in TC4, so del key command was not able to set, and then oid
timeout was happed. if we found the root cause why fw couldn't release TC resouce, we will remove this
workaround */

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

#if 0

static int wl_cfgvendor_gscan_get_capabilities(struct wiphy *wiphy,
					       struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pno_gscan_capabilities_t *reply = NULL;
	uint32 reply_len = 0;

	reply = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_GET_CAPABILITIES, NULL, &reply_len);
	if (!reply) {
		WL_ERR(("Could not get capabilities\n"));
		err = -EINVAL;
		return err;
	}

	err = wl_cfgvendor_send_cmd_reply(wiphy, bcmcfg_to_prmry_ndev(cfg), reply, reply_len);

	if (unlikely(err))
		WL_ERR(("Vendor Command reply failed ret:%d\n", err));

	kfree(reply);
	return err;
}

static int wl_cfgvendor_set_scan_cfg(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_scan_params_t *scan_param;
	int j = 0;
	int type, tmp, tmp1, tmp2, k = 0;
	const struct nlattr *iter, *iter1, *iter2;
	struct dhd_pno_gscan_channel_bucket *ch_bucket;

	scan_param = kzalloc(sizeof(gscan_scan_params_t), GFP_KERNEL);
	if (!scan_param) {
		WL_ERR(("Could not set GSCAN scan cfg, mem alloc failure\n"));
		err = -EINVAL;
		return err;

	}

	scan_param->scan_fr = PNO_SCAN_MIN_FW_SEC;
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);

		if (j >= GSCAN_MAX_CH_BUCKETS)
			break;

		switch (type) {
		case GSCAN_ATTRIBUTE_BASE_PERIOD:
			scan_param->scan_fr = nla_get_u32(iter) / 1000;
			break;
		case GSCAN_ATTRIBUTE_NUM_BUCKETS:
			scan_param->nchannel_buckets = nla_get_u32(iter);
			break;
		case GSCAN_ATTRIBUTE_CH_BUCKET_1:
		case GSCAN_ATTRIBUTE_CH_BUCKET_2:
		case GSCAN_ATTRIBUTE_CH_BUCKET_3:
		case GSCAN_ATTRIBUTE_CH_BUCKET_4:
		case GSCAN_ATTRIBUTE_CH_BUCKET_5:
		case GSCAN_ATTRIBUTE_CH_BUCKET_6:
		case GSCAN_ATTRIBUTE_CH_BUCKET_7:
		nla_for_each_nested(iter1, iter, tmp1) {
			type = nla_type(iter1);
			ch_bucket = scan_param->channel_bucket;

			switch (type) {
			case GSCAN_ATTRIBUTE_BUCKET_ID:
				break;
			case GSCAN_ATTRIBUTE_BUCKET_PERIOD:
				ch_bucket[j].bucket_freq_multiple = nla_get_u32(iter1) / 1000;
				break;
			case GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS:
				ch_bucket[j].num_channels = nla_get_u32(iter1);
				break;
			case GSCAN_ATTRIBUTE_BUCKET_CHANNELS:
				nla_for_each_nested(iter2, iter1, tmp2) {
					if (k >= PFN_SWC_RSSI_WINDOW_MAX)
						break;
					ch_bucket[j].chan_list[k] = nla_get_u32(iter2);
					k++;
				}
				k = 0;
				break;
			case GSCAN_ATTRIBUTE_BUCKETS_BAND:
				ch_bucket[j].band = (uint16)
				    nla_get_u32(iter1);
				break;
			case GSCAN_ATTRIBUTE_REPORT_EVENTS:
				ch_bucket[j].report_flag = (uint8)
				    nla_get_u32(iter1);
				break;
			}
		}
		j++;
		break;
		}
	}

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_SCAN_CFG_ID, scan_param, 0) < 0) {
		WL_ERR(("Could not set GSCAN scan cfg\n"));
		err = -EINVAL;
	}

	kfree(scan_param);
	return err;

}

static int wl_cfgvendor_set_batch_scan_cfg(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0, tmp, type;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_batch_params_t batch_param;
	const struct nlattr *iter;

	batch_param.mscan = batch_param.bestn = 0;
	batch_param.buffer_threshold = GSCAN_BATCH_NO_THR_SET;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);

		switch (type) {
		case GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN:
			batch_param.bestn = nla_get_u32(iter);
			break;
		case GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE:
			batch_param.mscan = nla_get_u32(iter);
			break;
		case GSCAN_ATTRIBUTE_REPORT_THRESHOLD:
			batch_param.buffer_threshold = nla_get_u32(iter);
			break;
		}
	}

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_BATCH_SCAN_CFG_ID, &batch_param, 0) < 0) {
		WL_ERR(("Could not set batch cfg\n"));
		err = -EINVAL;
		return err;
	}

	return err;
}

static int wl_cfgvendor_initiate_gscan(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type, tmp = len;
	int run = 0xFF;
	int flush = 0;
	const struct nlattr *iter;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		if (type == GSCAN_ATTRIBUTE_ENABLE_FEATURE)
			run = nla_get_u32(iter);
		else if (type == GSCAN_ATTRIBUTE_FLUSH_FEATURE)
			flush = nla_get_u32(iter);
	}

	if (run != 0xFF) {
		err = dhd_dev_pno_run_gscan(bcmcfg_to_prmry_ndev(cfg), run, flush);

		if (unlikely(err))
			WL_ERR(("Could not run gscan:%d\n", err));
		return err;
	} else {
		return -1;
	}

}

static int wl_cfgvendor_enable_full_scan_result(struct wiphy *wiphy,
						struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type;
	bool real_time = FALSE;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_ENABLE_FULL_SCAN_RESULTS) {
		real_time = nla_get_u32(data);

		err = dhd_dev_pno_enable_full_scan_result(bcmcfg_to_prmry_ndev(cfg), real_time);

		if (unlikely(err))
			WL_ERR(("Could not run gscan:%d\n", err));

	} else {
		err = -1;
	}

	return err;
}

static int wl_cfgvendor_hotlist_cfg(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_hotlist_scan_params_t *hotlist_params;
	int tmp, tmp1, tmp2, type, j = 0, dummy;
	const struct nlattr *outer, *inner, *iter;
	uint8 flush = 0;
	struct bssid_t *pbssid;

	hotlist_params = kzalloc(len, GFP_KERNEL);
	if (!hotlist_params) {
		WL_ERR(("Cannot Malloc mem to parse config commands size - %d bytes\n", len));
		return -1;
	}

	hotlist_params->lost_ap_window = GSCAN_LOST_AP_WINDOW_DEFAULT;

	nla_for_each_attr(iter, data, len, tmp2) {
		type = nla_type(iter);
		switch (type) {
		case GSCAN_ATTRIBUTE_HOTLIST_BSSIDS:
			pbssid = hotlist_params->bssid;
			nla_for_each_nested(outer, iter, tmp) {
				nla_for_each_nested(inner, outer, tmp1) {
					type = nla_type(inner);

					switch (type) {
					case GSCAN_ATTRIBUTE_BSSID:
						memcpy(&(pbssid[j].macaddr), nla_data(inner), ETHER_ADDR_LEN);
						break;
					case GSCAN_ATTRIBUTE_RSSI_LOW:
						pbssid[j].rssi_reporting_threshold = (int8) nla_get_u8(inner);
						break;
					case GSCAN_ATTRIBUTE_RSSI_HIGH:
						dummy = (int8) nla_get_u8(inner);
						break;
					}
				}
				j++;
			}
			hotlist_params->nbssid = j;
			break;
		case GSCAN_ATTRIBUTE_HOTLIST_FLUSH:
			flush = nla_get_u8(iter);
			break;
		case GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE:
			hotlist_params->lost_ap_window = nla_get_u32(iter);
			break;
		}

	}

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
				      DHD_PNO_GEOFENCE_SCAN_CFG_ID, hotlist_params, flush) < 0) {
		WL_ERR(("Could not set GSCAN HOTLIST cfg\n"));
		err = -EINVAL;
		goto exit;
	}
exit:
	kfree(hotlist_params);
	return err;
}

static int wl_cfgvendor_significant_change_cfg(struct wiphy *wiphy,
					       struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_swc_params_t *significant_params;
	int tmp, tmp1, tmp2, type, j = 0;
	const struct nlattr *outer, *inner, *iter;
	uint8 flush = 0;
	wl_pfn_significant_bssid_t *pbssid;

	significant_params = kzalloc(len, GFP_KERNEL);
	if (!significant_params) {
		WL_ERR(("Cannot Malloc mem to parse config commands size - %d bytes\n", len));
		return -1;
	}

	nla_for_each_attr(iter, data, len, tmp2) {
		type = nla_type(iter);

		switch (type) {
		case GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH:
			flush = nla_get_u8(iter);
			break;
		case GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE:
			significant_params->rssi_window = nla_get_u16(iter);
			break;
		case GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE:
			significant_params->lost_ap_window = nla_get_u16(iter);
			break;
		case GSCAN_ATTRIBUTE_MIN_BREACHING:
			significant_params->swc_threshold = nla_get_u16(iter);
			break;
		case GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS:
			pbssid = significant_params->bssid_elem_list;
			nla_for_each_nested(outer, iter, tmp) {
				nla_for_each_nested(inner, outer, tmp1) {
					switch (nla_type(inner)) {
					case GSCAN_ATTRIBUTE_BSSID:
						memcpy(&(pbssid[j].macaddr), nla_data(inner), ETHER_ADDR_LEN);
						break;
					case GSCAN_ATTRIBUTE_RSSI_HIGH:
						pbssid[j].rssi_high_threshold = (int8) nla_get_u8(inner);
						break;
					case GSCAN_ATTRIBUTE_RSSI_LOW:
						pbssid[j].rssi_low_threshold = (int8) nla_get_u8(inner);
						break;
					}
				}
				j++;
			}
			break;
		}
	}
	significant_params->nbssid = j;

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
				      DHD_PNO_SIGNIFICANT_SCAN_CFG_ID, significant_params, flush) < 0) {
		WL_ERR(("Could not set GSCAN significant cfg\n"));
		err = -EINVAL;
		goto exit;
	}
exit:
	kfree(significant_params);
	return err;
}

static int wl_cfgvendor_gscan_get_batch_results(struct wiphy *wiphy,
						struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_results_cache_t *results, *iter;
	uint32 reply_len, complete = 0, num_results_iter;
	int32 mem_needed;
	wifi_gscan_result_t *ptr;
	uint16 num_scan_ids, num_results;
	struct sk_buff *skb;
	struct nlattr *scan_hdr;

	dhd_dev_wait_batch_results_complete(bcmcfg_to_prmry_ndev(cfg));
	dhd_dev_pno_lock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
	results = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_GET_BATCH_RESULTS, NULL, &reply_len);

	if (!results) {
		WL_ERR(("No results to send %d\n", err));
		err = wl_cfgvendor_send_cmd_reply(wiphy, bcmcfg_to_prmry_ndev(cfg), results, 0);

		if (unlikely(err))
			WL_ERR(("Vendor Command reply failed ret:%d\n", err));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return err;
	}
	num_scan_ids = reply_len & 0xFFFF;
	num_results = (reply_len & 0xFFFF0000) >> 16;
	mem_needed = (num_results * sizeof(wifi_gscan_result_t)) +
	    (num_scan_ids * GSCAN_BATCH_RESULT_HDR_LEN) + VENDOR_REPLY_OVERHEAD + SCAN_RESULTS_COMPLETE_FLAG_LEN;

	if (mem_needed > (int32) NLMSG_DEFAULT_SIZE) {
		mem_needed = (int32) NLMSG_DEFAULT_SIZE;
		complete = 0;
	} else {
		complete = 1;
	}

	WL_TRACE(("complete %d mem_needed %d max_mem %d\n", complete, mem_needed, (int)NLMSG_DEFAULT_SIZE));
	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return -ENOMEM;
	}
	iter = results;

	nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE, complete);

	mem_needed = mem_needed - (SCAN_RESULTS_COMPLETE_FLAG_LEN + VENDOR_REPLY_OVERHEAD);

	while (iter && ((mem_needed - GSCAN_BATCH_RESULT_HDR_LEN) > 0)) {
		scan_hdr = nla_nest_start(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS);
		nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_ID, iter->scan_id);
		nla_put_u8(skb, GSCAN_ATTRIBUTE_SCAN_FLAGS, iter->flag);
		num_results_iter = (mem_needed - GSCAN_BATCH_RESULT_HDR_LEN) / sizeof(wifi_gscan_result_t);

		if ((iter->tot_count - iter->tot_consumed) < num_results_iter)
			num_results_iter = iter->tot_count - iter->tot_consumed;

		nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num_results_iter);
		if (num_results_iter) {
			ptr = &iter->results[iter->tot_consumed];
			iter->tot_consumed += num_results_iter;
			nla_put(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS, num_results_iter * sizeof(wifi_gscan_result_t), ptr);
		}
		nla_nest_end(skb, scan_hdr);
		mem_needed -= GSCAN_BATCH_RESULT_HDR_LEN + (num_results_iter * sizeof(wifi_gscan_result_t));
		iter = iter->next;
	}

	dhd_dev_gscan_batch_cache_cleanup(bcmcfg_to_prmry_ndev(cfg));
	dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));

	return cfg80211_vendor_cmd_reply(skb);
}

static int wl_cfgvendor_gscan_get_channel_list(struct wiphy *wiphy,
					       struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0, type, band;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	uint16 *reply = NULL;
	uint32 reply_len = 0, num_channels, mem_needed;
	struct sk_buff *skb;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_BAND)
		band = nla_get_u32(data);
	else
		return -1;

	reply = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_GET_CHANNEL_LIST, &band, &reply_len);

	if (!reply) {
		WL_ERR(("Could not get channel list\n"));
		err = -EINVAL;
		return err;
	}
	num_channels = reply_len / sizeof(uint32);
	mem_needed = reply_len + VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		err = -ENOMEM;
		goto exit;
	}

	nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_CHANNELS, num_channels);
	nla_put(skb, GSCAN_ATTRIBUTE_CHANNEL_LIST, reply_len, reply);

	err = cfg80211_vendor_cmd_reply(skb);

	if (unlikely(err))
		WL_ERR(("Vendor Command reply failed ret:%d\n", err));
exit:
	kfree(reply);
	return err;
}

static int wl_cfgvendor_gscan_get_batch_results(struct wiphy *wiphy,
						struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_results_cache_t *results, *iter;
	uint32 reply_len, complete = 0, num_results_iter;
	int32 mem_needed;
	wifi_gscan_result_t *ptr;
	uint16 num_scan_ids, num_results;
	struct sk_buff *skb;
	struct nlattr *scan_hdr;

	dhd_dev_wait_batch_results_complete(bcmcfg_to_prmry_ndev(cfg));
	dhd_dev_pno_lock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
	results = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg), DHD_PNO_GET_BATCH_RESULTS, NULL, &reply_len);

	if (!results) {
		WL_ERR(("No results to send %d\n", err));
		err = wl_cfgvendor_send_cmd_reply(wiphy, bcmcfg_to_prmry_ndev(cfg), results, 0);

		if (unlikely(err))
			WL_ERR(("Vendor Command reply failed ret:%d\n", err));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return err;
	}
	num_scan_ids = reply_len & 0xFFFF;
	num_results = (reply_len & 0xFFFF0000) >> 16;
	mem_needed = (num_results * sizeof(wifi_gscan_result_t)) +
	    (num_scan_ids * GSCAN_BATCH_RESULT_HDR_LEN) + VENDOR_REPLY_OVERHEAD + SCAN_RESULTS_COMPLETE_FLAG_LEN;

	if (mem_needed > (int32) NLMSG_DEFAULT_SIZE) {
		mem_needed = (int32) NLMSG_DEFAULT_SIZE;
		complete = 0;
	} else {
		complete = 1;
	}

	WL_TRACE(("complete %d mem_needed %d max_mem %d\n", complete, mem_needed, (int)NLMSG_DEFAULT_SIZE));
	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return -ENOMEM;
	}
	iter = results;

	nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE, complete);

	mem_needed = mem_needed - (SCAN_RESULTS_COMPLETE_FLAG_LEN + VENDOR_REPLY_OVERHEAD);

	while (iter && ((mem_needed - GSCAN_BATCH_RESULT_HDR_LEN) > 0)) {
		scan_hdr = nla_nest_start(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS);
		nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_ID, iter->scan_id);
		nla_put_u8(skb, GSCAN_ATTRIBUTE_SCAN_FLAGS, iter->flag);
		num_results_iter = (mem_needed - GSCAN_BATCH_RESULT_HDR_LEN) / sizeof(wifi_gscan_result_t);

		if ((iter->tot_count - iter->tot_consumed) < num_results_iter)
			num_results_iter = iter->tot_count - iter->tot_consumed;

		nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num_results_iter);
		if (num_results_iter) {
			ptr = &iter->results[iter->tot_consumed];
			iter->tot_consumed += num_results_iter;
			nla_put(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS, num_results_iter * sizeof(wifi_gscan_result_t), ptr);
		}
		nla_nest_end(skb, scan_hdr);
		mem_needed -= GSCAN_BATCH_RESULT_HDR_LEN + (num_results_iter * sizeof(wifi_gscan_result_t));
		iter = iter->next;
	}

	dhd_dev_gscan_batch_cache_cleanup(bcmcfg_to_prmry_ndev(cfg));
	dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));

	return cfg80211_vendor_cmd_reply(skb);
}

#endif
