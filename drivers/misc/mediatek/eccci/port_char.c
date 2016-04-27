/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/uidgid.h>
#include <mt-plat/mt_ccci_common.h>
#include <mt-plat/mt_boot_common.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include "ccci_config.h"
#include "ccci_core.h"
#include "ccci_support.h"
#include "ccci_bm.h"
#include "port_ipc.h"
#include "port_kernel.h"
#include "port_char.h"
#include "port_smem.h"
#ifdef CONFIG_MTK_ECCCI_C2K
#include "ccif_c2k_platform.h"
#endif
#include "ccci_platform.h"

#ifdef FEATURE_GET_MD_BAT_VOL	/* must be after ccci_config.h */
#include <mt-plat/battery_common.h>
#else
#define BAT_Get_Battery_Voltage(polling_mode)    ({ 0; })
#endif

#include <mt-plat/env.h>
#define MAX_QUEUE_LENGTH 32

static void dev_char_open_check(struct ccci_port *port)
{
	if (port->rx_ch == CCCI_FS_RX)
		port->modem->critical_user_active[0] = 1;
	if (port->rx_ch == CCCI_UART2_RX)
		port->modem->critical_user_active[1] = 1;
	if (port->rx_ch == CCCI_MD_LOG_RX)
		port->modem->critical_user_active[2] = 1;
	if (port->rx_ch == CCCI_UART1_RX)
		port->modem->critical_user_active[3] = 1;
}

static int dev_char_close_check(struct ccci_port *port)
{
	if (port->rx_ch == CCCI_FS_RX && !atomic_read(&port->usage_cnt))
		port->modem->critical_user_active[0] = 0;
	if (port->rx_ch == CCCI_UART2_RX && !atomic_read(&port->usage_cnt))
		port->modem->critical_user_active[1] = 0;
	if (port->rx_ch == CCCI_MD_LOG_RX && !atomic_read(&port->usage_cnt))
		port->modem->critical_user_active[2] = 0;
	if (port->rx_ch == CCCI_UART1_RX && !atomic_read(&port->usage_cnt))
		port->modem->critical_user_active[3] = 0;
	CCCI_NORMAL_LOG(port->modem->index, CHAR, "dev close check: %d %d %d %d\n",
				port->modem->critical_user_active[0],
				port->modem->critical_user_active[1], port->modem->critical_user_active[2],
				port->modem->critical_user_active[3]);
	ccci_event_log("md%d: dev close check: %d %d %d %d\n", port->modem->index,
				port->modem->critical_user_active[0],
				port->modem->critical_user_active[1], port->modem->critical_user_active[2],
				port->modem->critical_user_active[3]);

	if (port->modem->critical_user_active[0] == 0 && port->modem->critical_user_active[1] == 0) {
		if (is_meta_mode() || is_advanced_meta_mode()) {
			if (port->modem->critical_user_active[3] == 0) {
				CCCI_NORMAL_LOG(port->modem->index, CHAR, "ready to reset MD in META mode\n");
				return 0;
			}
			/* this should never happen */
			CCCI_ERROR_LOG(port->modem->index, CHAR, "DHL ctrl is still open in META mode\n");
		} else {
			if (port->modem->critical_user_active[2] == 0 && port->modem->critical_user_active[3] == 0) {
				CCCI_NORMAL_LOG(port->modem->index, CHAR, "ready to reset MD in normal mode\n");
				return 0;
			}
		}
	}
	return 1;
}

static int dev_char_open(struct inode *inode, struct file *file)
{
	int major = imajor(inode);
	int minor = iminor(inode);
	struct ccci_port *port;

	port = ccci_get_port_for_node(major, minor);
	if (atomic_read(&port->usage_cnt))
		return -EBUSY;
	CCCI_NORMAL_LOG(port->modem->index, CHAR, "port %s open with flag %X by %s\n", port->name, file->f_flags,
		     current->comm);
	atomic_inc(&port->usage_cnt);
	file->private_data = port;
	nonseekable_open(inode, file);
	dev_char_open_check(port);
#ifdef FEATURE_POLL_MD_EN
	if (port->rx_ch == CCCI_MD_LOG_RX && port->modem->md_state == READY)
		mod_timer(&port->modem->md_status_poller, jiffies + 10 * HZ);
#endif
	return 0;
}

static int dev_char_close(struct inode *inode, struct file *file)
{
	struct ccci_port *port = file->private_data;
	struct ccci_request *req = NULL;
	struct ccci_request *reqn;
	unsigned long flags;

	/* 0. decrease usage count, so when we ask more, the packet can be dropped in recv_request */
	atomic_dec(&port->usage_cnt);
	/* 1. purge Rx request list */
	spin_lock_irqsave(&port->rx_req_lock, flags);
	list_for_each_entry_safe(req, reqn, &port->rx_req_list, entry) {
		/* 1.1. remove from list */
		list_del(&req->entry);
		port->rx_length--;
		/* 1.2. free it */
		req->policy = RECYCLE;
		ccci_free_req(req);
	}
	/* 1.3 flush Rx */
	ccci_port_ask_more_request(port);
	spin_unlock_irqrestore(&port->rx_req_lock, flags);
	CCCI_NORMAL_LOG(port->modem->index, CHAR, "port %s close rx_len=%d empty=%d\n", port->name,
		     port->rx_length, list_empty(&port->rx_req_list));
	ccci_event_log("md%d: port %s close rx_len=%d empty=%d\n", port->modem->index, port->name,
		     port->rx_length, list_empty(&port->rx_req_list));
	/* 2. check critical nodes for reset, run close check first,
	      as mdlogger is killed before we gated MD when IPO shutdown */
	if (dev_char_close_check(port) == 0 && port->modem->md_state == GATED)
		ccci_send_virtual_md_msg(port->modem, CCCI_MONITOR_CH, CCCI_MD_MSG_READY_TO_RESET, 0);
#ifdef FEATURE_POLL_MD_EN
	if (port->rx_ch == CCCI_MD_LOG_RX)
		del_timer(&port->modem->md_status_poller);
#endif
	return 0;
}

static void port_ch_dump(int md_id, char *str, void *msg_buf, int len)
{
#if 0
#define DUMP_BUF_SIZE 200
	unsigned char *char_ptr = (unsigned char *)msg_buf;
	char buf[DUMP_BUF_SIZE];
	int i, j;

	for (i = 0, j = 0; i < len && i < DUMP_BUF_SIZE && j + 4 < DUMP_BUF_SIZE; i++) {
		if (((32 <= char_ptr[i]) && (char_ptr[i] <= 126))) {
			buf[j++] = char_ptr[i];
		} else {
			if (DUMP_BUF_SIZE - j > 4) {
				snprintf(buf+j, DUMP_BUF_SIZE-j, "[%02X]", char_ptr[i]);
				j += 4;
			} else {
				buf[j++] = '.';
			}
		}
	}
	buf[j] = '\0';
	CCCI_NORMAL_LOG(md_id, CHAR, "%s %d>%s\n", str, len, buf);
#endif
}

static ssize_t dev_char_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct ccci_port *port = file->private_data;
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h = NULL;
	int ret = 0, read_len = 0, full_req_done = 0;
	unsigned long flags = 0;

READ_START:
	/* 1. get incoming request */
	if (list_empty(&port->rx_req_list)) {
		if (!(file->f_flags & O_NONBLOCK)) {
			ret = wait_event_interruptible(port->rx_wq, !list_empty(&port->rx_req_list));
			if (ret == -ERESTARTSYS) {
				ret = -EINTR;
				goto exit;
			}
		} else {
			ret = -EAGAIN;
			goto exit;
		}
	}
	CCCI_DEBUG_LOG(port->modem->index, CHAR, "read on %s for %zu\n", port->name, count);
	spin_lock_irqsave(&port->rx_req_lock, flags);
	if (list_empty(&port->rx_req_list)) {
		spin_unlock_irqrestore(&port->rx_req_lock, flags);
		if (!(file->f_flags & O_NONBLOCK)) {
			goto READ_START;
		} else {
			ret = -EAGAIN;
			goto exit;
		}
	}
	req = list_first_entry(&port->rx_req_list, struct ccci_request, entry);
	/* 2. caculate available data */
	if (req->state != PARTIAL_READ) {
		ccci_h = (struct ccci_header *)req->skb->data;
		if (port->flags & PORT_F_USER_HEADER) {	/* header provide by user */
			/* CCCI_MON_CH should fall in here, as header must be send to md_init */
			if (ccci_h->data[0] == CCCI_MAGIC_NUM) {
				read_len = sizeof(struct ccci_header);
				if (ccci_h->channel == CCCI_MONITOR_CH)
					/* ccci_h->channel = CCCI_MONITOR_CH_ID; */
					*(((u32 *) ccci_h) + 2) = CCCI_MONITOR_CH_ID;
			} else {
				read_len = req->skb->len;
			}
		} else {
			/* ATTENTION, if user does not provide header, it should NOT send empty packet. */
			read_len = req->skb->len - sizeof(struct ccci_header);
			/* remove CCCI header */
			skb_pull(req->skb, sizeof(struct ccci_header));
		}
	} else {
		read_len = req->skb->len;
	}
	if (count >= read_len) {
		full_req_done = 1;
		list_del(&req->entry);
		/*
		 * here we only ask for more request when rx list is empty. no need to be too gready, because
		 * for most of the case, queue will not stop sending request to port.
		 * actually we just need to ask by ourselves when we rejected requests before. these
		 * rejected requests will staty in queue's buffer and may never get a chance to be handled again.
		 */
		if (--(port->rx_length) == 0)
			ccci_port_ask_more_request(port);
		BUG_ON(port->rx_length < 0);
	} else {
		req->state = PARTIAL_READ;
		read_len = count;
	}
	spin_unlock_irqrestore(&port->rx_req_lock, flags);
	if (ccci_h && ccci_h->channel == CCCI_UART2_RX)
		port_ch_dump(port->modem->index, "chr_read", req->skb->data, read_len);
	/* 3. copy to user */
	if (copy_to_user(buf, req->skb->data, read_len)) {
		CCCI_ERROR_LOG(port->modem->index, CHAR, "read on %s, copy to user failed, %d/%zu\n", port->name,
			     read_len, count);
		ret = -EFAULT;
	}
	skb_pull(req->skb, read_len);
	/* CCCI_DEBUG_LOG(port->modem->index, CHAR,
	"read done on %s l=%d r=%d pr=%d\n", port->name, read_len, ret, (req->state==PARTIAL_READ)); */
	/* 4. free request */
	if (full_req_done) {
		req->policy = RECYCLE;
			/* Rx flow doesn't know the free policy until it reaches port
			   (network and char are different) */
#if 0
		if (port->rx_ch == CCCI_IPC_RX)
			port_ipc_rx_ack(port);
#endif
		ccci_free_req(req);
	}
 exit:
	return ret ? ret : read_len;
}

#ifdef CONFIG_MTK_ECCCI_C2K

int ccci_c2k_rawbulk_intercept(int ch_id, unsigned int interception)
{
	int ret = 0;
	struct ccci_modem *md = NULL;
	struct ccci_port *port = NULL;
	struct list_head *port_list = NULL;
	char matched = 0;
	int ch_id_rx = 0;

	/* USB bypass's channel id offset, please refer to viatel_rawbulk.h */
	if (ch_id >= FS_CH_C2K)
		ch_id += 2;
	else
		ch_id += 1;

	/*only data and log channel are legal*/
	if (ch_id == DATA_PPP_CH_C2K) {
		ch_id = CCCI_C2K_PPP_DATA;
		ch_id_rx = CCCI_C2K_PPP_DATA;
	} else if (ch_id == MDLOG_CH_C2K) {
		ch_id = CCCI_MD_LOG_TX;
		ch_id_rx = CCCI_MD_LOG_RX;
	} else {
		ret = -ENODEV;
		CCCI_ERROR_LOG(MD_SYS3, CHAR, "Err: wrong ch_id(%d) from usb bypass\n", ch_id);
		return ret;
	}

	/* only md3 can usb bypass */
	md = ccci_get_modem_by_id(MD_SYS3);

	/*use rx channel to find port*/
	port_list = &md->rx_ch_ports[ch_id_rx];
	list_for_each_entry(port, port_list, entry) {
		matched = (ch_id == port->tx_ch);
		if (matched) {
			port->interception = !!interception;
			if (port->interception)
				atomic_inc(&port->usage_cnt);
			else
				atomic_dec(&port->usage_cnt);
			if (ch_id == CCCI_C2K_PPP_DATA)
				md->data_usb_bypass = !!interception;
			ret = 0;
			CCCI_NORMAL_LOG(md->index, CHAR, "port(%s) ch(%d) interception(%d) set\n",
				port->name, ch_id, interception);
		}
	}
	if (!matched) {
		ret = -ENODEV;
		CCCI_ERROR_LOG(md->index, CHAR, "Err: no port found when setting interception(%d,%d)\n",
			ch_id, interception);
	}

	return ret;
}


int ccci_c2k_buffer_push(int ch_id, void *buf, int count)
{
	int ret = 0;
	struct ccci_modem *md = NULL;
	struct ccci_port *port = NULL;
	struct list_head *port_list = NULL;
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h = NULL;
	char matched = 0;
	size_t actual_count = 0;
	int ch_id_rx = 0;
	unsigned char blk1 = 0;	/* usb will call this routine in ISR, so we cannot schedule */
	unsigned char blk2 = 0;	/* non-blocking for all request from USB */

	/* USB bypass's channel id offset, please refer to viatel_rawbulk.h */
	if (ch_id >= FS_CH_C2K)
		ch_id += 2;
	else
		ch_id += 1;

	/* only data and log channel are legal */
	if (ch_id == DATA_PPP_CH_C2K) {
		ch_id = CCCI_C2K_PPP_DATA;
		ch_id_rx = CCCI_C2K_PPP_DATA;
	} else if (ch_id == MDLOG_CH_C2K) {
		ch_id = CCCI_MD_LOG_TX;
		ch_id_rx = CCCI_MD_LOG_RX;
	} else {
		ret = -ENODEV;
		CCCI_ERROR_LOG(MD_SYS3, CHAR, "Err: wrong ch_id(%d) from usb bypass\n", ch_id);
		return ret;
	}

	/* only md3 can usb bypass */
	md = ccci_get_modem_by_id(MD_SYS3);

	CCCI_NORMAL_LOG(md->index, CHAR, "data from usb bypass (ch%d)(%d)\n", ch_id, count);

	actual_count = count > CCCI_MTU ? CCCI_MTU : count;

	port_list = &md->rx_ch_ports[ch_id_rx];
	list_for_each_entry(port, port_list, entry) {
		matched = (ch_id == port->tx_ch);
		if (matched) {
			req = ccci_alloc_req(OUT, actual_count, blk1, blk2);
			if (req) {
				req->policy = RECYCLE;
				ccci_h = (struct ccci_header *)skb_put(req->skb, sizeof(struct ccci_header));
				ccci_h->data[0] = 0;
				ccci_h->data[1] = actual_count + sizeof(struct ccci_header);
				ccci_h->channel = port->tx_ch;
				ccci_h->reserved = 0;

				memcpy(skb_put(req->skb, actual_count), buf, actual_count);

				/* for md3, ccci_h->channel will probably change after call send_request,
				because md3's channel mapping. */
				/* do NOT reference request after called this,
				   modem may have freed it, unless you get -EBUSY */
				ret = ccci_port_send_request(port, req);

				if (ret) {
					if (ret == -EBUSY && !req->blocking)
						ret = -EAGAIN;
					goto push_err_out;
				}
				return actual_count;
push_err_out:
				ccci_free_req(req);
				return ret;
			}
			/* consider this case as non-blocking */
			return -ENOMEM;
		}
	}
	return -ENODEV;
}

#endif

static ssize_t dev_char_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct ccci_port *port = file->private_data;
	unsigned char blocking = !(file->f_flags & O_NONBLOCK);
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h = NULL;
	size_t actual_count = 0;
	int ret = 0, header_len = 0;

	if (port->tx_ch == CCCI_MONITOR_CH)
		return -EPERM;

	if (port->tx_ch == CCCI_IPC_UART_TX)
		CCCI_DEBUG_LOG(port->modem->index, CHAR,
		"port %s write: md_state=%d\n", port->name, port->modem->md_state);

	if (port->modem->md_state == BOOTING && port->tx_ch != CCCI_FS_TX && port->tx_ch != CCCI_RPC_TX) {
		CCCI_NORMAL_LOG(port->modem->index, CHAR, "port %s ch%d write fail when md_state=%d\n", port->name,
			     port->tx_ch, port->modem->md_state);
		return -ENODEV;
	}
	if (port->modem->md_state == EXCEPTION && port->tx_ch != CCCI_MD_LOG_TX && port->tx_ch != CCCI_UART1_TX
	    && port->tx_ch != CCCI_FS_TX)
		return -ETXTBSY;
	if (port->modem->md_state == GATED || port->modem->md_state == RESET || port->modem->md_state == INVALID)
		return -ENODEV;

	header_len = sizeof(struct ccci_header) + (port->rx_ch == CCCI_FS_RX ? sizeof(unsigned int) : 0);
	if (port->flags & PORT_F_USER_HEADER) {
		if (count > (CCCI_MTU + header_len)) {
			CCCI_ERROR_LOG(port->modem->index, CHAR, "reject packet(size=%zu ), larger than MTU on %s\n",
				     count, port->name);
			return -ENOMEM;
		}
	}
	if (count == 0)
		return -EINVAL;
	if (port->flags & PORT_F_USER_HEADER)
		actual_count = count > (CCCI_MTU + header_len) ? (CCCI_MTU + header_len) : count;
	else
		actual_count = count > CCCI_MTU ? CCCI_MTU : count;

	/*if (CCCI_FS_TX != port->tx_ch)
		CCCI_NORMAL_LOG(port->modem->index, CHAR, "write on %s for %zu of %zu, md_s=%d\n",
			port->name, actual_count, count, port->modem->md_state); */

	req = ccci_alloc_req(OUT, actual_count, blocking, blocking);
	if (req) {
		/* 1. for Tx packet, who issued it should know whether recycle it  or not */
		req->policy = RECYCLE;
		/* 2. prepare CCCI header, every member of header should be re-write as request may be re-used */
		if (!(port->flags & PORT_F_USER_HEADER)) {
			ccci_h = (struct ccci_header *)skb_put(req->skb, sizeof(struct ccci_header));
			ccci_h->data[0] = 0;
			ccci_h->data[1] = actual_count + sizeof(struct ccci_header);
			ccci_h->channel = port->tx_ch;
			ccci_h->reserved = 0;
		} else {
			ccci_h = (struct ccci_header *)req->skb->data;
		}
		/* 3. get user data */
		ret = copy_from_user(skb_put(req->skb, actual_count), buf, actual_count);
		if (ret)
			goto err_out;
		if (port->flags & PORT_F_USER_HEADER) {	/* header provided by user, valid after copy_from_user */
			if (actual_count == sizeof(struct ccci_header))
				ccci_h->data[0] = CCCI_MAGIC_NUM;
			else
				ccci_h->data[1] = actual_count;
			ccci_h->channel = port->tx_ch;	/* as EEMCS VA will not fill this filed */
		}
		if (port->rx_ch == CCCI_IPC_RX) {
			ret = port_ipc_write_check_id(port, req);
			if (ret < 0)
				goto err_out;
			else
				ccci_h->reserved = ret;	/* Unity ID */
		}
		if (ccci_h && ccci_h->channel == CCCI_UART2_TX) {
			port_ch_dump(port->modem->index, "chr_write", req->skb->data + sizeof(struct ccci_header),
				     actual_count);
		}

		/* 4. send out */
		/* for md3, ccci_h->channel will probably change after call send_request,
		because md3's channel mapping */
		ret = ccci_port_send_request(port, req);
			/* do NOT reference request after called this, modem may have freed it, unless you get -EBUSY */
		if (ccci_h && ccci_h->channel == CCCI_UART2_TX) {
			/* CCCI_NORMAL_LOG(port->modem->index, CHAR,
				"write done on %s, l=%zu r=%d\n", port->name, actual_count, ret); */
		}

		if (ret) {
			if (ret == -EBUSY && !req->blocking)
				ret = -EAGAIN;
			goto err_out;
		} else {
#if 0
			if (port->rx_ch == CCCI_IPC_RX)
				port_ipc_tx_wait(port);
#endif
			if (port->rx_ch == CCCI_FS_RX && port->modem->boot_stage == MD_BOOT_STAGE_1
					&& !MD_IN_DEBUG(port->modem))
				mod_timer(&port->modem->bootup_timer, jiffies + BOOT_TIMER_HS2 * HZ);

		}
		return actual_count;

 err_out:
		CCCI_NORMAL_LOG(port->modem->index, CHAR, "write error done on %s, l=%zu r=%d\n",
			 port->name, actual_count, ret);
		ccci_free_req(req);
		return ret;
	}
	/* consider this case as non-blocking */
	return -EBUSY;
}

static int last_md_status[5];
static int md_status_show_count[5];

static long dev_char_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long state, ret = 0;
	struct ccci_setting *ccci_setting;
	struct ccci_port *port = file->private_data;
	struct ccci_modem *md = port->modem;
	int ch = port->rx_ch;	/* use Rx channel number to distinguish a port */
	unsigned int sim_mode, sim_switch_type, enable_sim_type, sim_id, bat_info;
	unsigned int traffic_control = 0;
	unsigned int sim_slot_cfg[4];
	struct siginfo sig_info;
	unsigned int sig_pid;
	unsigned int md_boot_data[16] = { 0 };
	int md_type = 0;
	unsigned int ccif_on = 0;

#ifdef CONFIG_MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT
	unsigned int val;
	char magic_pattern[64];
#endif

	switch (cmd) {
	case CCCI_IOC_GET_MD_PROTOCOL_TYPE:
		{
			char md_protol[] = "DHL";
			unsigned int data_size = sizeof(md_protol) / sizeof(char);

			CCCI_ERROR_LOG(md->index, CHAR, "Call CCCI_IOC_GET_MD_PROTOCOL_TYPE!\n");

			if (copy_to_user((void __user *)arg, md_protol, data_size)) {
				CCCI_ERROR_LOG(md->index, CHAR, "copy_to_user MD_PROTOCOL failed !!\n");

				return -EFAULT;
			}

			break;
		}
	case CCCI_IOC_GET_MD_STATE:
		state = md->boot_stage;
		if (state != last_md_status[md->index]) {
			last_md_status[md->index] = state;
			md_status_show_count[md->index] = 0;
		} else {
			if (md_status_show_count[md->index] < 100)
				md_status_show_count[md->index]++;
			else
				md_status_show_count[md->index] = 0;
		}

		if (md_status_show_count[md->index] == 0) {
			CCCI_NORMAL_LOG(md->index, CHAR, "MD state %ld, %d\n", state, md->md_state);
			ccci_event_log("md%d: MD state %ld, %d\n", md->index, state, md->md_state);
			md_status_show_count[md->index]++;
		}

		if (state >= 0) {
			/* CCCI_DEBUG_LOG(md->index, CHAR, "MD state %ld\n", state); */
			/* state+='0'; // convert number to character */
			ret = put_user((unsigned int)state, (unsigned int __user *)arg);
		} else {
			CCCI_ERROR_LOG(md->index, CHAR, "Get MD state fail: %ld\n", state);
			ret = state;
		}
		break;
	case CCCI_IOC_PCM_BASE_ADDR:
	case CCCI_IOC_PCM_LEN:
	case CCCI_IOC_ALLOC_MD_LOG_MEM:
		/* deprecated, share memory operation */
		break;
	case CCCI_IOC_MD_RESET:
		CCCI_NOTICE_LOG(md->index, CHAR, "MD reset ioctl(%d) called by %s\n", ch, current->comm);
		ccci_event_log("md%d: MD reset ioctl(%d) called by %s\n", md->index, ch, current->comm);
		ret = md->ops->reset(md);
		if (ret == 0) {
			ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_RESET, 0);
			#ifdef CONFIG_MTK_ECCCI_C2K
			if (md->index == MD_SYS1)
				exec_ccci_kern_func_by_md_id(MD_SYS3, ID_RESET_MD, NULL, 0);
			else if (md->index == MD_SYS3)
				exec_ccci_kern_func_by_md_id(MD_SYS1, ID_RESET_MD, NULL, 0);
			#else
#ifdef CONFIG_MTK_SVLTE_SUPPORT
			c2k_reset_modem();
#endif
			#endif
		}
		break;
	case CCCI_IOC_FORCE_MD_ASSERT:
		CCCI_NORMAL_LOG(md->index, CHAR, "Force MD assert ioctl(%d) called by %s\n", ch, current->comm);
		ccci_event_log("md%d: Force MD assert ioctl(%d) called by %s\n", md->index, ch, current->comm);
		if (md->index == MD_SYS3)
			/* MD3 use interrupt to force assert */
			ret = md->ops->force_assert(md, CCIF_INTERRUPT);
		else
		ret = md->ops->force_assert(md, CCCI_MESSAGE);
		break;
	case CCCI_IOC_SEND_RUN_TIME_DATA:
		if (ch == CCCI_MONITOR_CH) {
			ret = md->ops->send_runtime_data(md, md->sbp_code);
		} else {
			CCCI_NORMAL_LOG(md->index, CHAR, "Set runtime by invalid user(%u) called by %s\n", ch,
				     current->comm);
			ret = -1;
		}
		break;
	case CCCI_IOC_GET_MD_INFO:
		state = md->img_info[IMG_MD].img_info.version;
		ret = put_user((unsigned int)state, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_GET_MD_EX_TYPE:
		ret = put_user((unsigned int)md->ex_type, (unsigned int __user *)arg);
		CCCI_NORMAL_LOG(md->index, CHAR, "get modem exception type=%d ret=%ld\n", md->ex_type, ret);
		break;
	case CCCI_IOC_SEND_STOP_MD_REQUEST:
		CCCI_NORMAL_LOG(md->index, CHAR, "stop MD request ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: stop MD request ioctl called by %s\n", md->index, current->comm);
		ret = md->ops->reset(md);
		if (ret == 0) {
			md->ops->stop(md, 0);
			ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_STOP_MD_REQUEST, 0);
#ifdef CONFIG_MTK_ECCCI_C2K
			if (md->index == MD_SYS1)
				exec_ccci_kern_func_by_md_id(MD_SYS3, ID_RESET_MD, NULL, 0);
			else if (md->index == MD_SYS3)
				exec_ccci_kern_func_by_md_id(MD_SYS1, ID_RESET_MD, NULL, 0);
#else
#ifdef CONFIG_MTK_SVLTE_SUPPORT
			c2k_reset_modem();
#endif
#endif
		}
		break;
	case CCCI_IOC_SET_BOOT_DATA:
			CCCI_NORMAL_LOG(md->index, CHAR, "set MD boot env data called by %s\n",
					 current->comm);
			ccci_event_log("md%d: set MD boot env data called by %s\n", md->index,
					 current->comm);
			if (copy_from_user
				(&md_boot_data, (void __user *)arg, sizeof(md_boot_data))) {
				CCCI_NORMAL_LOG(md->index, CHAR,
					 "CCCI_IOC_SET_BOOT_DATA: copy_from_user fail!\n");
				ret = -EFAULT;
			} else {
				ret = ccci_set_md_boot_data(md, md_boot_data, ARRAY_SIZE(md_boot_data));
				if (ret < 0) {
					CCCI_NORMAL_LOG(md->index, CHAR,
					"ccci_set_md_boot_data return fail!\n");
					ret = -EFAULT;
				}
			}
			break;

	case CCCI_IOC_SEND_START_MD_REQUEST:
		CCCI_NORMAL_LOG(md->index, CHAR, "start MD request ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: start MD request ioctl called by %s\n", md->index, current->comm);
		ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_START_MD_REQUEST, 0);
		break;
	case CCCI_IOC_DO_START_MD:
		CCCI_NORMAL_LOG(md->index, CHAR, "start MD ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: start MD ioctl called by %s\n", md->index, current->comm);
		ret = md->ops->start(md);
		break;
	case CCCI_IOC_DO_STOP_MD:
		CCCI_NORMAL_LOG(md->index, CHAR, "stop MD ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: stop MD ioctl called by %s\n", md->index, current->comm);
		ret = md->ops->stop(md, 0);
		break;
	case CCCI_IOC_ENTER_DEEP_FLIGHT:
		CCCI_NOTICE_LOG(md->index, CHAR, "enter MD flight mode ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: enter MD flight mode ioctl called by %s\n", md->index, current->comm);
		md->flight_mode = MD_FIGHT_MODE_ENTER; /* enter flight mode */
		ret = md->ops->reset(md);
		if (ret == 0) {
			md->ops->stop(md, 1000);
			ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_ENTER_FLIGHT_MODE, 0);
		}
		break;
	case CCCI_IOC_LEAVE_DEEP_FLIGHT:
		CCCI_NOTICE_LOG(md->index, CHAR, "leave MD flight mode ioctl called by %s\n", current->comm);
		ccci_event_log("md%d: leave MD flight mode ioctl called by %s\n", md->index, current->comm);
		wake_lock_timeout(&md->md_wake_lock, 10 * HZ);
		md->flight_mode = MD_FIGHT_MODE_LEAVE; /* leave flight mode */
		ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_LEAVE_FLIGHT_MODE, 0);
		break;

	case CCCI_IOC_POWER_ON_MD_REQUEST:
		CCCI_NORMAL_LOG(md->index, CHAR, "Power on MD request ioctl called by %s\n", current->comm);
		ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_POWER_ON_REQUEST, 0);
		break;

	case CCCI_IOC_POWER_OFF_MD_REQUEST:
		CCCI_NORMAL_LOG(md->index, CHAR, "Power off MD request ioctl called by %s\n", current->comm);
		ret = ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_POWER_OFF_REQUEST, 0);
		break;
	case CCCI_IOC_POWER_ON_MD:
	case CCCI_IOC_POWER_OFF_MD:
		/* abandoned */
		CCCI_NORMAL_LOG(md->index, CHAR, "Power on/off MD by user(%d) called by %s\n", ch, current->comm);
		ret = -1;
		break;
	case CCCI_IOC_SIM_SWITCH:
		if (copy_from_user(&sim_mode, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_BOOTUP_LOG(md->index, CHAR, "IOC_SIM_SWITCH: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			switch_sim_mode(md->index, (char *)&sim_mode, sizeof(sim_mode));
			CCCI_BOOTUP_LOG(md->index, CHAR, "IOC_SIM_SWITCH(%x): %ld\n", sim_mode, ret);
		}
		break;
	case CCCI_IOC_SIM_SWITCH_TYPE:
		sim_switch_type = get_sim_switch_type();
		CCCI_BOOTUP_LOG(md->index, KERN, "CCCI_IOC_SIM_SWITCH_TYPE:sim type(0x%x)\n", sim_switch_type);
		ret = put_user(sim_switch_type, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_GET_SIM_TYPE:
		if (md->sim_type == 0xEEEEEEEE)
			CCCI_BOOTUP_LOG(md->index, KERN, "md has not send sim type yet(0x%x)", md->sim_type);
		else
			CCCI_BOOTUP_LOG(md->index, KERN, "md has send sim type(0x%x)", md->sim_type);
		ret = put_user(md->sim_type, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_ENABLE_GET_SIM_TYPE:
		if (copy_from_user(&enable_sim_type, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_ENABLE_GET_SIM_TYPE: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_NORMAL_LOG(md->index, KERN, "CCCI_IOC_ENABLE_GET_SIM_TYPE: sim type(0x%x)",
								enable_sim_type);
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_SIM_TYPE, enable_sim_type, 1);
		}
		break;
	case CCCI_IOC_SEND_BATTERY_INFO:
		bat_info = (unsigned int)BAT_Get_Battery_Voltage(0);
		CCCI_NORMAL_LOG(md->index, CHAR, "get bat voltage %d\n", bat_info);
		ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_GET_BATTERY_INFO, bat_info, 1);
		break;
	case CCCI_IOC_RELOAD_MD_TYPE:
		state = 0;
		if (copy_from_user(&state, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "IOC_RELOAD_MD_TYPE: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_NORMAL_LOG(md->index, CHAR, "IOC_RELOAD_MD_TYPE: storing md type(%ld)!\n", state);
			ccci_event_log("md%d: IOC_RELOAD_MD_TYPE: storing md type(%ld)!\n", md->index, state);
			if ((state >= modem_ultg) && (state <= MAX_IMG_NUM) && (md->index == MD_SYS1)) {
				if (md_capability(MD_SYS1, state, 0))
					ccci_reload_md_type(md, state);
				else
					ret = -1;
			} else
				ccci_reload_md_type(md, state);
		}
		break;
	case CCCI_IOC_SET_MD_IMG_EXIST:
		if (copy_from_user
		    (&md->md_img_exist, (void __user *)arg, sizeof(md->md_img_exist))) {
			CCCI_BOOTUP_LOG(md->index, CHAR,
				     "CCCI_IOC_SET_MD_IMG_EXIST: copy_from_user fail!\n");
			ret = -EFAULT;
		}
		md->md_img_type_is_set = 1;
		CCCI_BOOTUP_LOG(md->index, CHAR,
			"CCCI_IOC_SET_MD_IMG_EXIST: set done!\n");
		break;

	case CCCI_IOC_GET_MD_IMG_EXIST:
		md_type = get_md_type_from_lk(md->index); /* For LK load modem use */
		if (md_type) {
			memset(&md->md_img_exist, 0, sizeof(md->md_img_exist));
			md->md_img_exist[0] = md_type;
			CCCI_BOOTUP_LOG(md->index, CHAR, "lk md_type: %d, image num:1\n", md_type);
		} else {
			CCCI_BOOTUP_LOG(md->index, CHAR,
				"CCCI_IOC_GET_MD_IMG_EXIST: waiting set\n");
			while (md->md_img_type_is_set == 0)
				msleep(200);
		}
		CCCI_BOOTUP_LOG(md->index, CHAR,
			"CCCI_IOC_GET_MD_IMG_EXIST: waiting set done!\n");
		if (copy_to_user((void __user *)arg, &md->md_img_exist, sizeof(md->md_img_exist))) {
			CCCI_BOOTUP_LOG(md->index, CHAR,
				     "CCCI_IOC_GET_MD_IMG_EXIST: copy_to_user fail!\n");
			ret = -EFAULT;
		}
		break;
	case CCCI_IOC_GET_MD_TYPE:
		state = md->config.load_type;
		ret = put_user((unsigned int)state, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_STORE_MD_TYPE:
		if (copy_from_user(&md->config.load_type_saving, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_BOOTUP_LOG(md->index, CHAR, "store md type fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_BOOTUP_LOG(md->index, CHAR, "storing md type(%d) in kernel space!\n",
				     md->config.load_type_saving);
			ccci_event_log("md%d: storing md type(%d) in kernel space!\n", md->index,
				     md->config.load_type_saving);
			if (md->config.load_type_saving >= 1 && md->config.load_type_saving <= MAX_IMG_NUM) {
				if (md->config.load_type_saving != md->config.load_type)
					CCCI_BOOTUP_LOG(md->index, CHAR,
						     "Maybe Wrong: md type storing not equal with current setting!(%d %d)\n",
						     md->config.load_type_saving, md->config.load_type);
				/* Notify md_init daemon to store md type in nvram */
				ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_STORE_NVRAM_MD_TYPE, 0);
			} else {
				CCCI_BOOTUP_LOG(md->index, CHAR, "store md type fail: invalid md type(0x%x)\n",
					     md->config.load_type_saving);
			}
		}
		break;
	case CCCI_IOC_GET_MD_TYPE_SAVING:
		ret = put_user(md->config.load_type_saving, (unsigned int __user *)arg);
		break;
	case CCCI_IPC_RESET_RECV:
	case CCCI_IPC_RESET_SEND:
	case CCCI_IPC_WAIT_MD_READY:
	case CCCI_IPC_UPDATE_TIME:
	case CCCI_IPC_WAIT_TIME_UPDATE:
	case CCCI_IPC_UPDATE_TIMEZONE:
		ret = port_ipc_ioctl(port, cmd, arg);
		break;
	case CCCI_IOC_GET_EXT_MD_POST_FIX:
		if (copy_to_user((void __user *)arg, md->post_fix, IMG_POSTFIX_LEN)) {
			CCCI_BOOTUP_LOG(md->index, CHAR, "CCCI_IOC_GET_EXT_MD_POST_FIX: copy_to_user fail\n");
			ret = -EFAULT;
		}
		break;
	case CCCI_IOC_SEND_ICUSB_NOTIFY:
		if (copy_from_user(&sim_id, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_SEND_ICUSB_NOTIFY: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			ret = ccci_send_msg_to_md(md, CCCI_SYSTEM_TX, MD_ICUSB_NOTIFY, sim_id, 1);
		}
		break;
	case CCCI_IOC_DL_TRAFFIC_CONTROL:
		if (copy_from_user(&traffic_control, (void __user *)arg, sizeof(unsigned int)))
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_DL_TRAFFIC_CONTROL: copy_from_user fail\n");
		if (traffic_control == 1)
			;/* turn off downlink queue */
		else if (traffic_control == 0)
			;/* turn on donwlink queue */
		else
		;
		ret = 0;
		break;
	case CCCI_IOC_UPDATE_SIM_SLOT_CFG:
		if (copy_from_user(&sim_slot_cfg, (void __user *)arg, sizeof(sim_slot_cfg))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_UPDATE_SIM_SLOT_CFG: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			int need_update;

			sim_switch_type = get_sim_switch_type();
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_UPDATE_SIM_SLOT_CFG get s0:%d s1:%d s2:%d s3:%d\n",
				     sim_slot_cfg[0], sim_slot_cfg[1], sim_slot_cfg[2], sim_slot_cfg[3]);
			ccci_setting = ccci_get_common_setting(md->index);
			need_update = sim_slot_cfg[0];
			ccci_setting->sim_mode = sim_slot_cfg[1];
			ccci_setting->slot1_mode = sim_slot_cfg[2];
			ccci_setting->slot2_mode = sim_slot_cfg[3];
			sim_mode = ((sim_switch_type << 16) | ccci_setting->sim_mode);
			switch_sim_mode(md->index, (char *)&sim_mode, sizeof(sim_mode));
			ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_CFG_UPDATE, need_update);
			ret = 0;
		}
		break;
	case CCCI_IOC_STORE_SIM_MODE:
		if (copy_from_user(&sim_mode, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "store sim mode fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_NORMAL_LOG(md->index, CHAR, "store sim mode(%x) in kernel space!\n", sim_mode);
			exec_ccci_kern_func_by_md_id(0, ID_STORE_SIM_SWITCH_MODE, (char *)&sim_mode,
						     sizeof(unsigned int));
		}
		break;
	case CCCI_IOC_GET_SIM_MODE:
		CCCI_NORMAL_LOG(md->index, CHAR, "get sim mode ioctl called by %s\n", current->comm);
		exec_ccci_kern_func_by_md_id(0, ID_GET_SIM_SWITCH_MODE, (char *)&sim_mode, sizeof(unsigned int));
		ret = put_user(sim_mode, (unsigned int __user *)arg);
		break;
	case CCCI_IOC_GET_CFG_SETTING:
		ccci_setting = ccci_get_common_setting(md->index);
		if (copy_to_user((void __user *)arg, ccci_setting, sizeof(struct ccci_setting))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "CCCI_IOC_GET_CFG_SETTING: copy_to_user fail\n");
			ret = -EFAULT;
		}
		break;

	case CCCI_IOC_GET_MD_SBP_CFG:
		if (!md->sbp_code_default) {
			unsigned char *sbp_custom_value = NULL;

			if (md->index == MD_SYS1 || md->index == MD_SYS3) {
#if defined(CONFIG_MTK_MD_SBP_CUSTOM_VALUE)
				sbp_custom_value = CONFIG_MTK_MD_SBP_CUSTOM_VALUE;
#else
				sbp_custom_value = "";
#endif
			} else if (md->index == MD_SYS2) {
#if defined(CONFIG_MTK_MD2_SBP_CUSTOM_VALUE)
				sbp_custom_value = CONFIG_MTK_MD2_SBP_CUSTOM_VALUE;
#else
				sbp_custom_value = "";
#endif
			}
			if ((sbp_custom_value == NULL) || (!strcmp(sbp_custom_value, "0")))
				sbp_custom_value = "";
			ret = kstrtouint(sbp_custom_value, 0, &md->sbp_code_default);
			if (!ret) {
				CCCI_BOOTUP_LOG(md->index, CHAR, "CCCI_IOC_GET_MD_SBP_CFG: get config sbp code:%d!\n",
					     md->sbp_code_default);
			} else {
				CCCI_BOOTUP_LOG(md->index, CHAR,
					     "CCCI_IOC_GET_MD_SBP_CFG: get config sbp code fail! ret:%ld, Config val:%s\n",
					     ret, sbp_custom_value);
			}
		} else {
			CCCI_BOOTUP_LOG(md->index, CHAR, "CCCI_IOC_GET_MD_SBP_CFG: config sbp code:%d!\n",
				     md->sbp_code_default);
		}
		ret = put_user(md->sbp_code_default, (unsigned int __user *)arg);
		break;

	case CCCI_IOC_SET_MD_SBP_CFG:
		if (copy_from_user(&md->sbp_code, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_BOOTUP_LOG(md->index, CHAR, "CCCI_IOC_SET_MD_SBP_CFG: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_BOOTUP_LOG(md->index, CHAR, "CCCI_IOC_SET_MD_SBP_CFG: set md sbp code:0x%x!\n",
							md->sbp_code);
		}
		break;

	case CCCI_IOC_SMEM_BASE:
		if (port->rx_ch == CCCI_SMEM_CH) {
			struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;

			ret = put_user((unsigned int)smem_port->addr_phy,
				(unsigned int __user *)arg);
		} else {
			ret = -EPERM;
		}
		break;
	case CCCI_IOC_SMEM_LEN:
		if (port->rx_ch == CCCI_SMEM_CH) {
			struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;

			ret = put_user((unsigned int)smem_port->length,
				(unsigned int __user *)arg);
		} else {
			ret = -EPERM;
		}
		break;
	case CCCI_IOC_SMEM_TX_NOTIFY:
		if (port->rx_ch == CCCI_SMEM_CH) {
			unsigned int data;

			if (copy_from_user(&data, (void __user *)arg, sizeof(unsigned int))) {
				CCCI_NORMAL_LOG(md->index, CHAR, "smem tx notify fail: copy_from_user fail!\n");
				ret = -EFAULT;
			} else {
				ret = port_smem_tx_nofity(port, data);
			}
		} else {
			ret = -EPERM;
		}
		break;
	case CCCI_IOC_SMEM_RX_POLL:
		if (port->rx_ch == CCCI_SMEM_CH)
			ret = port_smem_rx_poll(port);
		else
			ret = -EPERM;
		break;
	case CCCI_IOC_SMEM_SET_STATE:
		if (port->rx_ch == CCCI_SMEM_CH) {
			struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;
			unsigned int data;

			if (copy_from_user(&data, (void __user *)arg, sizeof(unsigned int))) {
				CCCI_NORMAL_LOG(md->index, CHAR, "smem set state fail: copy_from_user fail!\n");
				ret = -EFAULT;
			} else {
				smem_port->state = data;
			}
		} else {
			ret = -EPERM;
		}
		break;
	case CCCI_IOC_SMEM_GET_STATE:
		if (port->rx_ch == CCCI_SMEM_CH) {
			struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;

			ret = put_user((unsigned int)smem_port->state,
				(unsigned int __user *)arg);
		} else {
			ret = -EPERM;
		}
		break;

	case CCCI_IOC_SET_HEADER:
		port->flags |= PORT_F_USER_HEADER;
		break;
	case CCCI_IOC_CLR_HEADER:
		port->flags &= ~PORT_F_USER_HEADER;
		break;

#ifdef CONFIG_MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT
	case CCCI_IOC_RESET_AP:
		if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
			CCCI_ERR_MSG(md->index, KERN, "get SML value failed.\n");

		CCCI_INF_MSG(md->index, CHAR, "get val=%x from userspace.\n", val);

		snprintf(magic_pattern, 64, "%x", val);
		set_env("sml_sync", magic_pattern);
		break;
#endif
	case CCCI_IOC_SEND_SIGNAL_TO_USER:
		if (copy_from_user(&sig_pid, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "signal to rild fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			unsigned int sig = (sig_pid >> 16) & 0xFFFF;
			unsigned int pid = sig_pid & 0xFFFF;

			sig_info.si_signo = sig;
			sig_info.si_code = SI_KERNEL;
			sig_info.si_pid = current->pid;
			sig_info.si_uid = __kuid_val(current->cred->uid);
			ret = kill_proc_info(SIGUSR2, &sig_info, pid);
			CCCI_NORMAL_LOG(md->index, CHAR, "send signal %d to rild %d ret=%ld\n", sig, pid, ret);
		}
		break;
	case CCCI_IOC_RESET_MD1_MD3_PCCIF:
#ifdef CONFIG_MTK_ECCCI_C2K
		CCCI_NORMAL_LOG(md->index, CHAR, "reset md1/md3 pccif ioctl called by %s\n", current->comm);
		reset_md1_md3_pccif(md);
#endif
		break;
	case CCCI_IOC_SET_CCIF_CG:
		CCCI_NORMAL_LOG(md->index, CHAR, "set ccif cg ioctl called by %s\n", current->comm);
		if (copy_from_user(&ccif_on, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_NORMAL_LOG(md->index, CHAR, "set ccif cg fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_NORMAL_LOG(md->index, CHAR, "set ccif clock %s\n", ccif_on?"on":"off");
			set_ccif_cg(ccif_on);
		}
		break;

	case CCCI_IOC_SET_EFUN:
		if (copy_from_user(&sim_mode, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_ERR_MSG(md->index, CHAR, "set efun fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_INF_MSG(md->index, CHAR, "efun set to %d\n", sim_mode);
			if (sim_mode == 0 && md->ops->soft_stop)
				md->ops->soft_stop(md, sim_mode);
			else if (sim_mode != 0 && md->ops->soft_start)
				md->ops->soft_start(md, sim_mode);
		}
		break;

	case CCCI_IOC_SET_MD_BOOT_MODE:
		if (copy_from_user(&sim_mode, (void __user *)arg, sizeof(unsigned int))) {
			CCCI_ERR_MSG(md->index, CHAR, "set MD boot mode fail: copy_from_user fail!\n");
			ret = -EFAULT;
		} else {
			CCCI_INF_MSG(md->index, CHAR, "set MD boot mode to %d\n", sim_mode);
			exec_ccci_kern_func_by_md_id(md->index, ID_UPDATE_MD_BOOT_MODE,
						(char *)&sim_mode, sizeof(sim_mode));
#ifdef CONFIG_MTK_ECCCI_C2K
			if (md->index == MD_SYS1)
				exec_ccci_kern_func_by_md_id(MD_SYS3, ID_UPDATE_MD_BOOT_MODE,
							(char *)&sim_mode, sizeof(sim_mode));
			else if (md->index == MD_SYS3)
				exec_ccci_kern_func_by_md_id(MD_SYS1, ID_UPDATE_MD_BOOT_MODE,
							(char *)&sim_mode, sizeof(sim_mode));
#endif
		}
		break;
	case CCCI_IOC_GET_MD_BOOT_MODE:
		ret = put_user((unsigned int)md->md_boot_mode, (unsigned int __user *)arg);
		break;

	default:
		ret = -ENOTTY;
		break;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long dev_char_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ccci_port *port = filp->private_data;
	struct ccci_modem *md = port->modem;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		CCCI_ERROR_LOG(md->index, CHAR, "dev_char_compat_ioctl(!filp->f_op || !filp->f_op->unlocked_ioctl)\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case CCCI_IOC_PCM_BASE_ADDR:
	case CCCI_IOC_PCM_LEN:
	case CCCI_IOC_ALLOC_MD_LOG_MEM:
	case CCCI_IOC_FORCE_FD:
	case CCCI_IOC_AP_ENG_BUILD:
	case CCCI_IOC_GET_MD_MEM_SIZE:
		{
			CCCI_ERROR_LOG(md->index, CHAR, "dev_char_compat_ioctl deprecated cmd(%d)\n", cmd);
			return 0;
		}
	default:
		{
			return filp->f_op->unlocked_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
		}
	}
}
#endif
unsigned int dev_char_poll(struct file *fp, struct poll_table_struct *poll)
{
	struct ccci_port *port = fp->private_data;
	unsigned int mask = 0;

	CCCI_DEBUG_LOG(port->modem->index, CHAR, "poll on %s\n", port->name);
	if (port->rx_ch == CCCI_IPC_RX) {
		mask = port_ipc_poll(fp, poll);
	} else {
		poll_wait(fp, &port->rx_wq, poll);
		/* TODO: lack of poll wait for Tx */
		if (!list_empty(&port->rx_req_list))
			mask |= POLLIN | POLLRDNORM;
		if (port->modem->ops->write_room(port->modem, PORT_TXQ_INDEX(port)) > 0)
			mask |= POLLOUT | POLLWRNORM;
		if (port->rx_ch == CCCI_UART1_RX &&
		    port->modem->md_state != READY && port->modem->md_state != EXCEPTION) {
			mask |= POLLERR;	/* notify MD logger to save its log before md_init kills it */
			CCCI_NORMAL_LOG(port->modem->index, CHAR, "poll error for MD logger at state %d,mask=%d\n",
				     port->modem->md_state, mask);
		}
	}

	return mask;
}

static int dev_char_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct ccci_port *port = fp->private_data;
	struct ccci_smem_port *smem_port = (struct ccci_smem_port *)port->private_data;
	int pfn, len, ret;

	CCCI_DEBUG_LOG(port->modem->index, CHAR, "mmap on %s\n", port->name);
	if (port->rx_ch != CCCI_SMEM_CH)
		return -EPERM;

	CCCI_NORMAL_LOG(port->modem->index, CHAR, "remap addr:0x%llx len:%d  map-len:%lu\n",
			(unsigned long long)smem_port->addr_phy, smem_port->length, vma->vm_end - vma->vm_start);
	if ((vma->vm_end - vma->vm_start) > smem_port->length) {
		CCCI_ERROR_LOG(port->modem->index, CHAR,
			     "invalid mm size request from %s\n", port->name);
		return -EINVAL;
	}

	len =
	    (vma->vm_end - vma->vm_start) <
	    smem_port->length ? vma->vm_end - vma->vm_start : smem_port->length;
	pfn = smem_port->addr_phy;
	pfn >>= PAGE_SHIFT;
	/* ensure that memory does not get swapped to disk */
	vma->vm_flags |= VM_IO;
	/* ensure non-cacheable */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
	if (ret) {
		CCCI_ERROR_LOG(port->modem->index, CHAR, "remap failed %d/%x, 0x%llx -> 0x%llx\n", ret, pfn,
			(unsigned long long)smem_port->addr_phy, (unsigned long long)vma->vm_start);
		return -EAGAIN;
	}
	return 0;
}

static const struct file_operations char_dev_fops = {
	.owner = THIS_MODULE,
	.open = &dev_char_open,
	.read = &dev_char_read,
	.write = &dev_char_write,
	.release = &dev_char_close,
	.unlocked_ioctl = &dev_char_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = &dev_char_compat_ioctl,
#endif
	.poll = &dev_char_poll,
	.mmap = &dev_char_mmap,
};

static int port_char_init(struct ccci_port *port)
{
	struct cdev *dev;
	int ret = 0;

	CCCI_DEBUG_LOG(port->modem->index, CHAR, "char port %s is initializing\n", port->name);
	dev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	cdev_init(dev, &char_dev_fops);
	dev->owner = THIS_MODULE;
	port->rx_length_th = MAX_QUEUE_LENGTH;
	if (port->rx_ch == CCCI_IPC_RX)
		port_ipc_init(port);	/* this will change port->minor, call it before register device */
	else if (port->rx_ch == CCCI_SMEM_CH)
		port_smem_init(port);   /* this will change port->minor, call it before register device */
	else
		port->private_data = dev;	/* not using */
	ret = cdev_add(dev, MKDEV(port->modem->major, port->modem->minor_base + port->minor), 1);
	ret = ccci_register_dev_node(port->name, port->modem->major, port->modem->minor_base + port->minor);
	port->interception = 0;
	return ret;
}

#ifdef CONFIG_MTK_ECCCI_C2K
static int c2k_req_push_to_usb(struct ccci_port *port, struct ccci_request *req)
{

	struct ccci_header *ccci_h = NULL;
	int read_len, read_count, ret = 0;
	int c2k_ch_id;

	if (port->rx_ch == CCCI_C2K_PPP_DATA)
		c2k_ch_id = DATA_PPP_CH_C2K-1;
	else if (port->rx_ch == CCCI_MD_LOG_RX)
		c2k_ch_id = MDLOG_CH_C2K-2;
	else {
		ret = -ENODEV;
		CCCI_ERROR_LOG(port->modem->index, CHAR, "Err: wrong ch_id(%d) from usb bypass\n", port->rx_ch);
		return ret;
	}

	/* caculate available data */
	ccci_h = (struct ccci_header *)req->skb->data;
	read_len = req->skb->len - sizeof(struct ccci_header);
	/* remove CCCI header */
	skb_pull(req->skb, sizeof(struct ccci_header));

retry_push:
	/* push to usb */
	read_count = rawbulk_push_upstream_buffer(c2k_ch_id, req->skb->data, read_len);
	CCCI_DEBUG_LOG(port->modem->index, CHAR, "data push to usb bypass (ch%d)(%d)\n", port->rx_ch, read_count);

	if (read_count > 0) {
		skb_pull(req->skb, read_count);
		read_len -= read_count;
		if (read_len > 0)
			goto retry_push;
		else if (read_len == 0) {
			req->policy = RECYCLE;
			ccci_free_req(req);
		} else if (read_len < 0)
			CCCI_ERROR_LOG(port->modem->index, CHAR, "read_len error, check why come here\n");
	} else {
		CCCI_NORMAL_LOG(port->modem->index, CHAR, "usb buf full\n");
		msleep(20);
		goto retry_push;
	}

	return read_len;

}
#endif

static int port_char_recv_req(struct ccci_port *port, struct ccci_request *req)
{
	unsigned long flags;	/* as we can not tell the context, use spin_lock_irqsafe for safe */

	if (!atomic_read(&port->usage_cnt) &&
		(port->rx_ch != CCCI_UART2_RX && port->rx_ch != CCCI_C2K_AT && port->rx_ch != CCCI_PCM_RX &&
			port->rx_ch != CCCI_FS_RX && port->rx_ch != CCCI_RPC_RX))
		goto drop;

#ifdef CONFIG_MTK_ECCCI_C2K
	if ((port->modem->index == MD_SYS3) && port->interception) {
		list_del(&req->entry);
		c2k_req_push_to_usb(port, req);
		return 0;
	}
#endif

	CCCI_DEBUG_LOG(port->modem->index, CHAR, "recv on %s, len=%d\n", port->name, port->rx_length);
	spin_lock_irqsave(&port->rx_req_lock, flags);
	if (port->rx_length < port->rx_length_th) {
		port->flags &= ~PORT_F_RX_FULLED;
		port->rx_length++;
		list_del(&req->entry);	/* dequeue from queue's list */
		list_add_tail(&req->entry, &port->rx_req_list);
		spin_unlock_irqrestore(&port->rx_req_lock, flags);
		wake_lock_timeout(&port->rx_wakelock, HZ);
		wake_up_all(&port->rx_wq);
		if (port->rx_ch == CCCI_FS_RX && port->modem->boot_stage != MD_BOOT_STAGE_2)
			del_timer(&port->modem->bootup_timer);
		return 0;
	}
	port->flags |= PORT_F_RX_FULLED;
	spin_unlock_irqrestore(&port->rx_req_lock, flags);
	if ((port->flags & PORT_F_ALLOW_DROP) /* || !(port->flags&PORT_F_RX_EXCLUSIVE) */) {
		CCCI_NORMAL_LOG(port->modem->index, CHAR, "port %s Rx full, drop packet\n", port->name);
		goto drop;
	} else
		return -CCCI_ERR_PORT_RX_FULL;

 drop:
	/* drop this packet */
	CCCI_DEBUG_LOG(port->modem->index, CHAR, "drop on %s, len=%d\n", port->name, port->rx_length);
	list_del(&req->entry);
	req->policy = RECYCLE;
	ccci_free_req(req);
	return -CCCI_ERR_DROP_PACKET;
}

static int port_char_req_match(struct ccci_port *port, struct ccci_request *req)
{
	struct ccci_header *ccci_h = (struct ccci_header *)req->skb->data;

	if (ccci_h->channel == port->rx_ch) {
		if (unlikely(port->rx_ch == CCCI_IPC_RX))
			return port_ipc_req_match(port, req);
		if (unlikely(port->rx_ch == CCCI_RPC_RX))
			return (port_kernel_req_match(port, req) == 0);
		return 1;
	}
	return 0;
}

static void port_char_md_state_notice(struct ccci_port *port, MD_STATE state)
{
	if (unlikely(port->rx_ch == CCCI_IPC_RX))
		port_ipc_md_state_notice(port, state);
	if (port->rx_ch == CCCI_UART1_RX && state == GATED)
		wake_up_all(&port->rx_wq);	/* check poll function */
	if (port->rx_ch == CCCI_SMEM_CH && state == RX_IRQ)
		port_smem_rx_wakeup(port);
}

struct ccci_port_ops char_port_ops = {
	.init = &port_char_init,
	.recv_request = &port_char_recv_req,
	.req_match = &port_char_req_match,
	.md_state_notice = &port_char_md_state_notice,
};

int ccci_subsys_char_init(struct ccci_modem *md)
{
	int ret = 0;
	dev_t dev = 0;

	if (md->major) {
		dev = MKDEV(md->major, md->minor_base);
		ret = register_chrdev_region(dev, 120, CCCI_DEV_NAME);
	} else {
		ret = alloc_chrdev_region(&dev, md->minor_base, 120, CCCI_DEV_NAME);
		if (ret)
			CCCI_ERROR_LOG(md->index, CHAR, "alloc_chrdev_region fail,ret=%d\n", ret);
		md->major = MAJOR(dev);
	}
	/* as IPC minor starts from 100 */
	last_md_status[md->index] = -1;
	return 0;
}
