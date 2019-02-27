/*
 * Mediatek BT-over-SDIO driver: SDIO interface related functions.
 * Copyright (C) 2017, Mediatek International Ltd.
 *
 * This software file (the "File") is distributed by Mediatek International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * On the worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/slab.h>

#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/module.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/device.h>


#include "btmtk_drv.h"
#include "btmtk_sdio.h"

static dev_t g_devIDfwlog;
static struct class *pBTClass;
static struct device *pBTDev;
struct device *pBTDevfwlog;
static wait_queue_head_t inq;
static wait_queue_head_t fw_log_inq;
static struct fasync_struct *fasync;

static int need_reset_stack;
/* The btmtk_sdio_remove() callback function is called
 * when user removes this module from kernel space or ejects
 * the card from the slot. The driver handles these 2 cases
 * differently.
 * If the user is removing the module, a MODULE_SHUTDOWN_REQ
 * command is sent to firmware and interrupt will be disabled.
 * If the card is removed, there is no need to send command
 * or disable interrupt.
 *
 * The variable 'user_rmmod' is used to distinguish these two
 * scenarios. This flag is initialized as FALSE in case the card
 * is removed, and will be set to TRUE for module removal when
 * module_exit function is called.
 */
typedef void (*register_sdio_early_suspend) (void (*f) (void));
typedef void (*register_sdio_late_resume) (void (*f) (void));
register_sdio_early_suspend register_sdio_early_suspend_func;
register_sdio_late_resume register_sdio_late_resume_func;

/*============================================================================*/
/* Callback Functions */
/*============================================================================*/

static u8 user_rmmod;

struct completion g_done;
unsigned char probe_counter;
unsigned char current_fwdump_file_number;
struct btmtk_private *g_priv;
#define STR_COREDUMP_END "coredump end\n\n"


static struct ring_buffer metabuffer;
u8 probe_ready;

static char fw_dump_file_name[FW_DUMP_FILE_NAME_SIZE] = {0};
int fw_dump_buffer_full;
int fw_dump_total_read_size;
int fw_dump_total_write_size;
int fw_dump_buffer_used_size;
int fw_dump_task_should_stop;
u8 *fw_dump_ptr;
u8 *fw_dump_read_ptr;
u8 *fw_dump_write_ptr;
struct timeval fw_dump_last_write_time;
int fw_dump_end_checking_task_should_stop;
int fw_is_doing_coredump;
int fw_is_coredump_end_packet;

#if SAVE_FW_DUMP_IN_KERNEL
	static struct file *fw_dump_file;
#else
	static int	fw_dump_file;
#endif

static const struct btmtk_sdio_card_reg btmtk_reg_6630 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
};

static const struct btmtk_sdio_card_reg btmtk_reg_6632 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
};

static const struct btmtk_sdio_card_reg btmtk_reg_7668 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
};

static const struct btmtk_sdio_card_reg btmtk_reg_7666 = {
	.cfg = 0x03,
	.host_int_mask = 0x04,
	.host_intstatus = 0x05,
	.card_status = 0x20,
	.sq_read_base_addr_a0 = 0x10,
	.sq_read_base_addr_a1 = 0x11,
	.card_fw_status0 = 0x40,
	.card_fw_status1 = 0x41,
	.card_rx_len = 0x42,
	.card_rx_unit = 0x43,
	.io_port_0 = 0x00,
	.io_port_1 = 0x01,
	.io_port_2 = 0x02,
	.int_read_to_clear = false,
	.func_num = 2,
};

static const struct btmtk_sdio_device btmtk_sdio_6630 = {
	.helper		= "mtmk/sd8688_helper.bin",
	.firmware	= "mt6632_patch_e1_hdr.bin",
	.reg		= &btmtk_reg_6630,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_6632 = {
	.helper     = "mtmk/sd8688_helper.bin",
	.firmware   = "mt6632_patch_e1_hdr.bin",
	.reg        = &btmtk_reg_6632,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_7668 = {
	.helper		= "mtmk/sd8688_helper.bin",
#if CFG_THREE_IN_ONE_FIRMWARE
	.firmware	= "MT7668_FW",
#else
	.firmware	= "mt7668_patch_e1_hdr.bin",
	.firmware1	= "mt7668_patch_e2_hdr.bin",
#endif
	.reg		= &btmtk_reg_7668,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};

static const struct btmtk_sdio_device btmtk_sdio_7666 = {
	.helper		= "mtmk/sd8688_helper.bin",
	.firmware	= "mt7668_patch_e1_hdr.bin",
	.reg		= &btmtk_reg_7666,
	.support_pscan_win_report = false,
	.sd_blksz_fw_dl = 64,
	.supports_fw_dump = false,
};


unsigned char *txbuf;
static unsigned char *rxbuf;
static u32 rx_length;
static struct btmtk_sdio_card *g_card;

/*
 * add in include/linux/mmc/sdio_ids.h
 */
#define SDIO_VENDOR_ID_MEDIATEK 0x037A

static const struct sdio_device_id btmtk_sdio_ids[] = {
	/* Mediatek SD8688 Bluetooth device */
	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x6630),
			.driver_data = (unsigned long) &btmtk_sdio_6630 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x6632),
			.driver_data = (unsigned long) &btmtk_sdio_6632 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7668),
			.driver_data = (unsigned long) &btmtk_sdio_7668 },

	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7666),
			.driver_data = (unsigned long) &btmtk_sdio_7666 },

	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(sdio, btmtk_sdio_ids);


u32 lock_unsleepable_lock(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
	return 0;
}

u32 unlock_unsleepable_lock(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
	return 0;
}

static int btmtk_sdio_readb(u32 offset, u32 *val)
{
	u32 ret = 0;

	if (g_card->func == NULL) {
		pr_notice("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}
	sdio_claim_host(g_card->func);
	*val = sdio_readb(g_card->func, offset, &ret);
	sdio_release_host(g_card->func);
	return ret;
}

static int btmtk_sdio_writel(u32 offset, u32 val)
{
	u32 ret = 0;

	if (g_card->func == NULL) {
		pr_notice("%s g_card->func is NULL\n", __func__);
		return -EIO;
	}
	sdio_claim_host(g_card->func);
	sdio_writel(g_card->func, val, offset, &ret);
	sdio_release_host(g_card->func);
	return ret;
}

static int btmtk_sdio_readl(u32 offset,  u32 *val)
{
	u32 ret = 0;

	if (g_card->func == NULL) {
		pr_notice("g_card->func is NULL\n");
		return -EIO;
	}
	sdio_claim_host(g_card->func);
	*val = sdio_readl(g_card->func, offset, &ret);
	sdio_release_host(g_card->func);
	return ret;
}

struct sk_buff *btmtk_create_send_data(struct sk_buff *skb)
{
	struct sk_buff *queue_skb = NULL;
	u32 sdio_header_len = skb->len + BTM_HEADER_LEN;

	if (skb_headroom(skb) < (BTM_HEADER_LEN)) {
		queue_skb = bt_skb_alloc(sdio_header_len, GFP_ATOMIC);
		if (queue_skb == NULL) {
			pr_notice("bt_skb_alloc fail return\n");
			return 0;
		}

		queue_skb->data[0] = (sdio_header_len & 0x0000ff);
		queue_skb->data[1] = (sdio_header_len & 0x00ff00) >> 8;
		queue_skb->data[2] = 0;
		queue_skb->data[3] = 0;
		queue_skb->data[4] = bt_cb(skb)->pkt_type;
		queue_skb->len = sdio_header_len;
		memcpy(&queue_skb->data[5], &skb->data[0], skb->len);
		kfree_skb(skb);
	} else {
		queue_skb = skb;
		skb_push(queue_skb, BTM_HEADER_LEN);
		queue_skb->data[0] = (sdio_header_len & 0x0000ff);
		queue_skb->data[1] = (sdio_header_len & 0x00ff00) >> 8;
		queue_skb->data[2] = 0;
		queue_skb->data[3] = 0;
		queue_skb->data[4] = bt_cb(skb)->pkt_type;
	}

	pr_info("%s end\n", __func__);
	return queue_skb;
}

static int btmtk_sdio_set_own_back(int owntype)
{
	/*Set driver own*/
	int ret = 0;
	u32 u32LoopCount = 0;
	u32 u32ReadCRValue = 0;
	u32 ownValue = 0;
	u32 set_checkretry = 30;

	pr_debug("%s owntype %d\n", __func__, owntype);

	if (user_rmmod)
		set_checkretry = 1;

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);

	pr_debug("%s btmtk_sdio_readl  CHLPCR done\n", __func__);
	if (owntype == DRIVER_OWN) {
		if ((u32ReadCRValue&0x100) == 0x100) {
			pr_debug("%s already driver own 0x%0x, return\n",
				__func__, u32ReadCRValue);
			return ret;
		}
	} else if (owntype == FW_OWN) {
		if ((u32ReadCRValue&0x100) == 0) {
			pr_debug("%s already FW own 0x%0x, return\n",
				__func__, u32ReadCRValue);
			return ret;
		}
	}

setretry:

	if (owntype == DRIVER_OWN)
		ownValue = 0x00000200;
	else
		ownValue = 0x00000100;

	pr_debug("%s write CHLPCR 0x%x\n", __func__, ownValue);
	ret = btmtk_sdio_writel(CHLPCR, ownValue);
	if (ret) {
		ret = -EINVAL;
		goto done;
	}
	pr_debug("%s write CHLPCR 0x%x done\n", __func__, ownValue);

	u32LoopCount = 1000;

	if (owntype == DRIVER_OWN) {
		do {
			udelay(1);
			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
			u32LoopCount--;
			pr_debug("%s DRIVER_OWN btmtk_sdio_readl CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
		} while ((u32LoopCount > 0) &&
			((u32ReadCRValue&0x100) != 0x100));

		if ((u32LoopCount == 0) && (0x100 != (u32ReadCRValue&0x100))
				&& (set_checkretry > 0)) {
			pr_notice("%s retry set_check driver own, CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
			set_checkretry--;
			mdelay(20);
			goto setretry;
		}
	} else {
		do {
			udelay(1);
			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
			u32LoopCount--;
			pr_debug("%s FW_OWN btmtk_sdio_readl CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
		} while ((u32LoopCount > 0) && ((u32ReadCRValue&0x100) != 0));

		if ((u32LoopCount == 0) &&
				((u32ReadCRValue&0x100) != 0) &&
				(set_checkretry > 0)) {
			pr_notice("%s retry set_check FW own, CHLPCR 0x%x\n",
				__func__, u32ReadCRValue);
			set_checkretry--;
			goto setretry;
		}
	}

	pr_debug("%s CHLPCR(0x%x), is 0x%x\n",
		__func__, CHLPCR, u32ReadCRValue);

	if (owntype == DRIVER_OWN) {
		if ((u32ReadCRValue&0x100) == 0x100)
			pr_debug("%s check %04x, is 0x100 driver own success\n",
				__func__, CHLPCR);
		else {
			pr_debug("%s check %04x, is %x shuld be 0x100\n",
				__func__, CHLPCR, u32ReadCRValue);
			ret = EINVAL;
			goto done;
		}
	} else {
		if (0x0 == (u32ReadCRValue&0x100))
			pr_debug("%s check %04x, bit 8 is 0 FW own success\n",
				__func__, CHLPCR);
		else{
			pr_debug("%s bit 8 should be 0, %04x bit 8 is %04x\n",
				__func__, u32ReadCRValue,
				(u32ReadCRValue&0x100));
			ret = EINVAL;
			goto done;
		}
	}

done:
	if (owntype == DRIVER_OWN) {
		if (ret)
			pr_notice("%s set driver own fail\n", __func__);
		else
			pr_debug("%s set driver own success\n", __func__);
	} else if (owntype == FW_OWN) {
		if (ret)
			pr_notice("%s set FW own fail\n", __func__);
		else
			pr_debug("%s set FW own success\n", __func__);
	} else
		pr_notice("%s unknown type %d\n", __func__, owntype);

	return ret;
}

static int btmtk_sdio_enable_interrupt(int enable)
{
	u32 ret = 0;
	u32 cr_value = 0;

	if (enable)
		cr_value |= C_FW_INT_EN_SET;
	else
		cr_value |= C_FW_INT_EN_CLEAR;

	ret = btmtk_sdio_writel(CHLPCR, cr_value);
	pr_debug("%s enable %d write CHLPCR 0x%08x\n",
			__func__, enable, cr_value);

	return ret;
}

static int btmtk_sdio_get_rx_unit(struct btmtk_sdio_card *card)
{
	u8 reg;
	int ret;

	reg = sdio_readb(card->func, card->reg->card_rx_unit, &ret);
	if (!ret)
		card->rx_unit = reg;

	return ret;
}

static int btmtk_sdio_enable_host_int_mask(
				struct btmtk_sdio_card *card,
				u8 mask)
{
	int ret;

	sdio_writeb(card->func, mask, card->reg->host_int_mask, &ret);
	if (ret) {
		pr_notice("Unable to enable the host interrupt!\n");
		ret = -EIO;
	}

	return ret;
}

static int btmtk_sdio_disable_host_int_mask(
				struct btmtk_sdio_card *card,
				u8 mask)
{
	u8 host_int_mask;
	int ret;

	host_int_mask = sdio_readb(card->func, card->reg->host_int_mask, &ret);
	if (ret)
		return -EIO;

	host_int_mask &= ~mask;

	sdio_writeb(card->func, host_int_mask, card->reg->host_int_mask, &ret);
	if (ret < 0) {
		pr_notice("Unable to disable the host interrupt!\n");
		return -EIO;
	}

	return 0;
}

/*for debug*/
int btmtk_print_buffer_conent(u8 *buf, u32 Datalen)
{
	int i = 0;
	int print_finish = 0;
	/*Print out txbuf data for debug*/
	for (i = 0; i <= (Datalen-1); i += 16) {
		if ((i+16) <= Datalen) {
			pr_debug("%s: %02X%02X%02X%02X%02X %02X%02X%02X%02X%02X %02X%02X%02X%02X%02X %02X\n",
				__func__,
				buf[i], buf[i+1], buf[i+2], buf[i+3],
				buf[i+4], buf[i+5], buf[i+6], buf[i+7],
				buf[i+8], buf[i+9], buf[i+10], buf[i+11],
				buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
		} else {
			for (; i < (Datalen); i++)
				pr_debug("%s: %02X\n", __func__, buf[i]);

			print_finish = 1;
		}

		if (print_finish)
			break;
	}
	return 0;
}

static int btmtk_sdio_send_tx_data(u8 *buffer, int tx_data_len)
{
	int ret = 0;
	u8 MultiBluckCount = 0;
	u8 redundant = 0;

	MultiBluckCount = tx_data_len/SDIO_BLOCK_SIZE;
	redundant = tx_data_len % SDIO_BLOCK_SIZE;

	if (redundant)
		tx_data_len = (MultiBluckCount+1)*SDIO_BLOCK_SIZE;

	sdio_claim_host(g_card->func);
	ret = sdio_writesb(g_card->func, CTDR, buffer, tx_data_len);
	sdio_release_host(g_card->func);

	return ret;
}

static int btmtk_sdio_recv_rx_data(void)
{
	int ret = 0;
	u32 u32ReadCRValue = 0;
	int retry_count = 5;
	u32 sdio_header_length = 0;

	memset(rxbuf, 0, MTK_RXDATA_SIZE);

	do {
		ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
		pr_debug("%s: loop Get CHISR 0x%08X\n",
			__func__, u32ReadCRValue);
		rx_length = (u32ReadCRValue & RX_PKT_LEN) >> 16;
		if (rx_length == 0xFFFF) {
			pr_notice("%s: 0xFFFF==rx_length, error return -EIO\n",
				__func__);
			ret = -EIO;
			break;
		}

		if ((RX_DONE&u32ReadCRValue) && rx_length) {
			pr_debug("%s: u32ReadCRValue = %08X\n",
				__func__, u32ReadCRValue);
			u32ReadCRValue &= 0xFFFB;
			ret = btmtk_sdio_writel(CHISR, u32ReadCRValue);
			pr_debug("%s: write = %08X\n",
				__func__, u32ReadCRValue);


			sdio_claim_host(g_card->func);
			ret = sdio_readsb(g_card->func, rxbuf, CRDR, rx_length);
			sdio_release_host(g_card->func);
			sdio_header_length = (rxbuf[1] << 8);
			sdio_header_length |= rxbuf[0];

			if (sdio_header_length != rx_length) {
				pr_notice("%s sdio header length %d, rx_length %d mismatch\n",
					__func__, sdio_header_length,
					rx_length);
				break;
			}

			if (sdio_header_length == 0) {
				pr_notice("%s: get sdio_header_length = %d\n",
					__func__, sdio_header_length);
				continue;
			}

			break;
		}

		retry_count--;
		if (retry_count <= 0) {
			pr_notice("%s: retry_count = %d,timeout\n",
				__func__, retry_count);
			ret = -EIO;
			break;
		}

		/* msleep(1); */
		mdelay(3);
		pr_debug("%s: retry_count = %d,wait\n", __func__, retry_count);

		if (ret)
			break;
	} while (1);

	if (ret)
		return -EIO;

	return ret;
}

static int btmtk_sdio_send_wmt_reset(void)
{
	int ret = 0;
	u8 wmt_event[8] = {4, 0xE4, 5, 2, 7, 1, 0, 0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_reset[9] = {1, 0x6F, 0xFC, 5, 1, 7, 1, 0, 4};

	pr_info("%s:\n", __func__);
	mtksdio_packet_header[0] = sizeof(mtksdio_packet_header) +
		sizeof(mtksdio_wmt_reset);

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf+MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_reset,
		sizeof(mtksdio_wmt_reset));

	btmtk_sdio_send_tx_data(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE+sizeof(mtksdio_wmt_reset));
	btmtk_sdio_recv_rx_data();

	/*compare rx data is wmt reset correct response or not*/
	if (memcmp(wmt_event, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_notice("%s: fail\n", __func__);
	}

	return ret;
}

static u32 btmtk_sdio_bt_memRegister_read(u32 cr)
{
	int retrytime = 300;
	u32 result = 0;
	u8 wmt_event[15] = {0x04, 0xE4, 0x10, 0x02,
				0x08, 0x0C/*0x1C*/, 0x00, 0x00,
				0x00, 0x00, 0x01, 0x00,
				0x00, 0x00, 0x80};
	/* msleep(1000); */
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_cmd[16] = {0x1, 0x6F, 0xFC, 0x0C,
				0x01, 0x08, 0x08, 0x00,
				0x02, 0x01, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00};
	mtksdio_packet_header[0] = sizeof(mtksdio_packet_header)
				+ sizeof(mtksdio_wmt_cmd);
	pr_info("%s: read cr %x\n", __func__, cr);

	memcpy(&mtksdio_wmt_cmd[12], &cr, sizeof(cr));

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf + MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_cmd,
		sizeof(mtksdio_wmt_cmd));

	btmtk_sdio_send_tx_data(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE + sizeof(mtksdio_wmt_cmd));
	btmtk_print_buffer_conent(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE + sizeof(mtksdio_wmt_cmd));

	do {
		usleep_range(10*1000, 15*1000);
		btmtk_sdio_recv_rx_data();
		retrytime--;
		if (retrytime <= 0)
			break;

		pr_info("%s: retrytime %d\n", __func__, retrytime);
	} while (!rxbuf[0]);

	btmtk_print_buffer_conent(rxbuf, rx_length);
	/* compare rx data is wmt reset correct response or not */
#if 0
	if (memcmp(wmt_event,
			rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_info("%s: fail\n", __func__);
	}
#endif
	memcpy(&result, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE + sizeof(wmt_event),
		sizeof(result));
	pr_info("%s: ger cr 0x%x value 0x%x\n", __func__, cr, result);
	return result;
}

/* 1:on ,  0:off */
static int btmtk_sdio_bt_set_power(u8 onoff)
{
	int ret = 0;
	int retrytime = 60;
	u8 wmt_event[8] = {4, 0xE4, 5, 2, 6, 1, 0, 0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_cmd[10] = {1, 0x6F, 0xFC, 6, 1, 6, 2, 0, 0, 1};

	if (onoff == 0)
		retrytime = 3;

	mtksdio_packet_header[0] =
		sizeof(mtksdio_packet_header) + sizeof(mtksdio_wmt_cmd);
	pr_info("%s: onoff %d\n", __func__, onoff);

	mtksdio_wmt_cmd[9] = onoff;

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf+MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_cmd,
		sizeof(mtksdio_wmt_cmd));

	btmtk_sdio_send_tx_data(txbuf,
		MTK_SDIO_PACKET_HEADER_SIZE+sizeof(mtksdio_wmt_cmd));

	do {
		msleep(100);
		btmtk_sdio_recv_rx_data();
		retrytime--;
		if (retrytime <= 0)
			break;

		if (retrytime < 40)
			pr_notice("%s: retry over 2s, retrytime %d\n",
				__func__, retrytime);

		pr_info("%s: retrytime %d\n", __func__, retrytime);
	} while (!rxbuf[0]);


	/*compare rx data is wmt reset correct response or not*/
	if (memcmp(wmt_event, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_info("%s: fail\n", __func__);
	}

	return ret;
}

#if BTMTK_BIN_FILE_MODE
static int btmtk_sdio_send_and_check(u8 *cmd, u16 cmd_len,
						u8 *event, u16 event_len)
{
	int ret = 0;
	int retrytime = 60;
	int len = 0;
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};

	len = MTK_SDIO_PACKET_HEADER_SIZE + cmd_len;

	mtksdio_packet_header[0] = (len & 0x0000ff);
	mtksdio_packet_header[1] = (len & 0x00ff00) >> 8;

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf + MTK_SDIO_PACKET_HEADER_SIZE, cmd, cmd_len);

	btmtk_sdio_send_tx_data(txbuf, len);

	if (event && (event_len != 0)) {
		do {
			msleep(100);
			btmtk_sdio_recv_rx_data();
			retrytime--;
			if (retrytime <= 0)
				break;

			if (retrytime < 40)
				pr_notice("%s: retry over 2s, retrytime %d\n",
					__func__, retrytime);
		} while (!rxbuf[0]);

		if (memcmp(event, rxbuf + MTK_SDIO_PACKET_HEADER_SIZE,
				event_len) != 0) {
			ret = -EIO;
			pr_info("%s: fail\n", __func__);
		}
	}

	return ret;
}

static bool btmtk_is_bin_file_mode(uint8_t *buf)
{
	char *ptr = NULL;

	/* find string */
	ptr = strstr(buf, E2P_MODE);
	if (!ptr) {
		pr_notice("%s: Can't find %s\n", __func__, E2P_MODE);
		return false;
	}

	/* check access mode */
	ptr += (strlen(E2P_MODE) + 1);
	pr_notice("%s: MODE: %c\n", __func__, *ptr);
	if (*ptr != BIN_FILE_MODE) {
		pr_notice("%s: It's not EEPROM - Bin file mode\n", __func__);
		return false;
	}
	return true;
}

static void btmtk_set_eeprom2ctrler(uint8_t *buf,
						size_t buf_size,
						bool is7668)
{
	int ret = -1;
	uint8_t set_bdaddr[] = {0x01, 0x1A, 0xFC, 0x06,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t set_bdaddr_e[] = {0x04, 0x0E, 0x04, 0x01,
			0x1A, 0xFC, 0x00};
	uint8_t set_radio[] = {0x01, 0x79, 0xFC, 0x06,
			0x07, 0x80, 0x00, 0x06, 0x07, 0x07};
	uint8_t set_radio_e[] = {0x04, 0x0E, 0x04, 0x01,
			0x79, 0xFC, 0x00};
	uint8_t set_pwr_offset[] = {0x01, 0x93, 0xFC, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t set_pwr_offset_e[] = {0x04, 0x0E, 0x04, 0x01,
			0x93, 0xFC, 0x00};
	uint16_t offset = 0;

	if (!buf) {
		pr_notice("%s: buf is null\n", __func__);
		return;
	} else if ((is7668 == true && buf_size < 0x389)
			|| (is7668 == false && buf_size < 0x133)) {
		pr_notice("%s: incorrect buf size(%d)\n",
			__func__, (int)buf_size);
		return;
	}

	/* set BD address */
	if (is7668)
		offset = 0x384;
	else
		offset = 0x1A;

	set_bdaddr[4] = *(buf + offset);
	set_bdaddr[5] = *(buf + offset + 1);
	set_bdaddr[6] = *(buf + offset + 2);
	set_bdaddr[7] = *(buf + offset + 3);
	set_bdaddr[8] = *(buf + offset + 4);
	set_bdaddr[9] = *(buf + offset + 5);
	ret = btmtk_sdio_send_and_check(set_bdaddr, sizeof(set_bdaddr),
					set_bdaddr_e, sizeof(set_bdaddr_e));
	pr_notice("%s: set BDAddress(%02X-%02X-%02X-%02X-%02X-%02X) %s\n",
			__func__,
			set_bdaddr[9], set_bdaddr[8], set_bdaddr[7],
			set_bdaddr[6], set_bdaddr[5], set_bdaddr[4],
			ret < 0 ? "fail" : "OK");

	/* radio setting - BT power */
	if (is7668) {
		offset = 0x382;
		/* BT default power */
		set_radio[4] = (*(buf + offset) & 0x07);
		/* BLE default power */
		set_radio[8] = (*(buf + offset + 1) & 0x07);
		/* TX MAX power */
		set_radio[9] = (*(buf + offset) & 0x70);
	} else {
		offset = 0x132;
		/* BT default power */
		set_radio[4] = *(buf + offset);
		/* BLE default power(no this for 7662 in table) */
		set_radio[8] = *(buf + offset);
		/* TX MAX power */
		set_radio[9] = *(buf + offset + 1);
	}
	ret = btmtk_sdio_send_and_check(set_radio, sizeof(set_radio),
				set_radio_e, sizeof(set_radio_e));
	pr_notice("%s: set radio(BT/BLE default power: %d/%d MAX power: %d) %s\n",
			__func__,
			set_radio[4], set_radio[8], set_radio[9],
			ret < 0 ? "fail" : "OK");

	/*
	 * BT TX power compensation for low, middle and high
	 * channel
	 */
	if (is7668) {
		offset = 0x36D;
		/* length */
		set_pwr_offset[3] = 6;
		/* Group 0 CH 0 ~ CH14 */
		set_pwr_offset[4] = *(buf + offset);
		/* Group 1 CH15 ~ CH27 */
		set_pwr_offset[5] = *(buf + offset + 1);
		/* Group 2 CH28 ~ CH40 */
		set_pwr_offset[6] = *(buf + offset + 2);
		/* Group 3 CH41 ~ CH53 */
		set_pwr_offset[7] = *(buf + offset + 3);
		/* Group 4 CH54 ~ CH66 */
		set_pwr_offset[8] = *(buf + offset + 4);
		/* Group 5 CH67 ~ CH84 */
		set_pwr_offset[9] = *(buf + offset + 5);
	} else {
		offset = 0x139;
		/* length */
		set_pwr_offset[3] = 3;
		/* low channel */
		set_pwr_offset[4] = *(buf + offset);
		/* middle channel */
		set_pwr_offset[5] = *(buf + offset + 1);
		/* high channel */
		set_pwr_offset[6] = *(buf + offset + 2);
	}
	ret = btmtk_sdio_send_and_check(set_pwr_offset, sizeof(set_pwr_offset),
				set_pwr_offset_e, sizeof(set_pwr_offset_e));
	pr_notice("%s: set power offset(%02X %02X %02X %02X %02X %02X) %s\n",
			__func__,
			set_pwr_offset[4], set_pwr_offset[5],
			set_pwr_offset[6], set_pwr_offset[7],
			set_pwr_offset[8], set_pwr_offset[9],
			ret < 0 ? "fail" : "OK");
}

static void btmtk_eeprom_bin_file(struct btmtk_sdio_card *card)
{
	char *cfg_file = NULL;
	char *bin_file = NULL;
	/* 7668 series */
	const struct firmware *cfg_fw = NULL;
	const struct firmware *bin_fw = NULL;

	/* For 76x8T series */
	{
		int ret = -1;

		pr_notice("%s: 7668 series\n", __func__);
		cfg_file = E2P_ACCESS_MODE_SWITCHER_7668;
		bin_file = E2P_BIN_FILE_7668;

		usleep_range(10*1000, 15*1000);

		/* request configure file */
		ret = request_firmware(&cfg_fw, cfg_file, &card->func->dev);
		if (ret < 0) {
			if (ret == -ENOENT)
				pr_notice("%s: Configure file not found, ignore EEPROM bin file\n",
					__func__);
			else
				pr_notice("%s: request configure file fail(%d)\n",
					__func__, ret);
			return;
		}

		if (btmtk_is_bin_file_mode((uint8_t *)cfg_fw->data) == false)
			goto exit2;

		usleep_range(10*1000, 15*1000);

		/* open bin file for EEPROM */
		ret = request_firmware(&bin_fw, bin_file, &card->func->dev);
		if (ret < 0) {
			pr_notice("%s: request bin file fail(%d)\n",
				__func__, ret);
			goto exit2;
		}

		/* set parameters to controller */
		btmtk_set_eeprom2ctrler((uint8_t *)bin_fw->data,
					bin_fw->size,
					true);
		goto exit2;
	}

exit2:
	if (cfg_fw)
		release_firmware(cfg_fw);
	if (bin_fw)
		release_firmware(bin_fw);
}
#endif

/* 1:on ,  0:off */
static int btmtk_sdio_set_sleep(void)
{
	int ret = 0;
	u8 wmt_event[8] = {4, 0xE, 4, 1, 0x7A, 0xFC, 0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtksdio_wmt_cmd[11] = {1, 0x7A, 0xFC, 7,
		/*3:sdio, 5:usb*/03,
		/*host non sleep duration*/0x80, 0x02,
		/*host non sleep duration*/0x80, 0x02, 0x0, 0x00};

	mtksdio_packet_header[0] =
		sizeof(mtksdio_packet_header) + sizeof(mtksdio_wmt_cmd);
	pr_info("%s begin\n", __func__);

	memcpy(txbuf, mtksdio_packet_header, MTK_SDIO_PACKET_HEADER_SIZE);
	memcpy(txbuf+MTK_SDIO_PACKET_HEADER_SIZE, mtksdio_wmt_cmd,
		sizeof(mtksdio_wmt_cmd));

	btmtk_sdio_send_tx_data(txbuf,
			MTK_SDIO_PACKET_HEADER_SIZE+sizeof(mtksdio_wmt_cmd));
	btmtk_sdio_recv_rx_data();
	btmtk_print_buffer_conent(rxbuf, rx_length);
	/*compare rx data is wmt reset correct response or not*/
	if (memcmp(wmt_event, rxbuf+MTK_SDIO_PACKET_HEADER_SIZE,
			sizeof(wmt_event)) != 0) {
		ret = -EIO;
		pr_info("%s: fail\n", __func__);
	}

	return ret;
}

static int btmtk_send_rom_patch(u8 *fwbuf, u32 fwlen, int mode)
{
	int ret = 0;
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	int stp_len = 0;
	u8 mtkdata_header[MTKDATA_HEADER_SIZE] = {0};

	int copy_len = 0;
	int Datalen = fwlen;
	u32 u32ReadCRValue = 0;


	pr_debug("%s fwlen %d, mode = %d\n", __func__, fwlen, mode);
	if (fwlen < Datalen) {
		pr_notice("%s file size = %d,is not corect\n", __func__, fwlen);
		return -ENOENT;
	}

	stp_len = Datalen + MTKDATA_HEADER_SIZE;


	mtkdata_header[0] = 0x2;/*ACL data*/
	mtkdata_header[1] = 0x6F;
	mtkdata_header[2] = 0xFC;

	mtkdata_header[3] = ((Datalen+4+1)&0x00FF);
	mtkdata_header[4] = ((Datalen+4+1)&0xFF00)>>8;

	mtkdata_header[5] = 0x1;
	mtkdata_header[6] = 0x1;

	mtkdata_header[7] = ((Datalen+1)&0x00FF);
	mtkdata_header[8] = ((Datalen+1)&0xFF00)>>8;

	mtkdata_header[9] = mode;

/* 0 and 1 is packet length, include MTKSTP_HEADER_SIZE */
	mtksdio_packet_header[0] =
		(Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF;
	mtksdio_packet_header[1] =
		((Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF00)>>8;
	mtksdio_packet_header[2] = 0;
	mtksdio_packet_header[3] = 0;

/*
 * mtksdio_packet_header[2] and mtksdio_packet_header[3]
 * are reserved
 */
	pr_debug("%s result %02x  %02x\n", __func__,
		((Datalen+4+MTKSTP_HEADER_SIZE+6)&0xFF00)>>8,
		(Datalen+4+MTKSTP_HEADER_SIZE+6));

	memcpy(txbuf+copy_len, mtksdio_packet_header,
		MTK_SDIO_PACKET_HEADER_SIZE);
	copy_len += MTK_SDIO_PACKET_HEADER_SIZE;

	memcpy(txbuf+copy_len, mtkdata_header, MTKDATA_HEADER_SIZE);
	copy_len += MTKDATA_HEADER_SIZE;

	memcpy(txbuf+copy_len, fwbuf, Datalen);
	copy_len += Datalen;

	pr_debug("%s txbuf %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		__func__,
		txbuf[0], txbuf[1], txbuf[2], txbuf[3], txbuf[4],
		txbuf[5], txbuf[6], txbuf[7], txbuf[8], txbuf[9]);


	ret = btmtk_sdio_readl(CHIER, &u32ReadCRValue);
	pr_debug("%s: CHIER u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
	pr_debug("%s: CHLPCR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s: 0CHISR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);
	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s: 00CHISR u32ReadCRValue %x, ret %d\n",
		__func__, u32ReadCRValue, ret);

	btmtk_sdio_send_tx_data(txbuf, copy_len);

	ret = btmtk_sdio_recv_rx_data();

	return ret;
}



/*
 * type: cmd:1, ACL:2
 * -------------------------------------------------
 * mtksdio hedaer 4 byte| wmt header  |
 *
 *
 * data len should less than 512-4-4
 */
static int btmtk_sdio_send_wohci(u8 type, u32 len, u8 *data)
{
	u32 ret = 0;
	u32 push_in_data_len = 0;
	u8 mtk_wmt_header[MTKWMT_HEADER_SIZE] = {0};
	u8 mtksdio_packet_header[MTK_SDIO_PACKET_HEADER_SIZE] = {0};
	u8 mtk_tx_data[512] = {0};

	mtk_wmt_header[0] = type;
	mtk_wmt_header[1] = 0x6F;
	mtk_wmt_header[2] = 0xFC;
	mtk_wmt_header[3] = len;

	mtksdio_packet_header[0] =
		(len+MTKWMT_HEADER_SIZE+MTK_SDIO_PACKET_HEADER_SIZE)&0xFF;
	mtksdio_packet_header[1] =
		((len+MTKWMT_HEADER_SIZE+MTK_SDIO_PACKET_HEADER_SIZE)&0xFF00)
		>>8;
	mtksdio_packet_header[2] = 0;
	mtksdio_packet_header[3] = 0;
/*
 * mtksdio_packet_header[2] and mtksdio_packet_header[3]
 * are reserved
 */

	memcpy(mtk_tx_data, mtksdio_packet_header,
		sizeof(mtksdio_packet_header));
	push_in_data_len += sizeof(mtksdio_packet_header);

	memcpy(mtk_tx_data+push_in_data_len, mtk_wmt_header,
		sizeof(mtk_wmt_header));
	push_in_data_len += sizeof(mtk_wmt_header);

	memcpy(mtk_tx_data+push_in_data_len, data, len);
	push_in_data_len += len;

	sdio_claim_host(g_card->func);
	ret = sdio_writesb(g_card->func, CTDR, mtk_tx_data, push_in_data_len);
	sdio_release_host(g_card->func);

	pr_info("%s retrun  0x%0x\n", __func__, ret);
	return ret;
}

/*
 * data event:
 * return
 * 0:
 * patch download is not complete/get patch semaphore fail
 * 1:
 * patch download is complete by others
 * 2
 * patch download is not coplete
 * 3:(for debug)
 * release patch semaphore success
 */
static int btmtk_sdio_need_load_rom_patch(void)
{
	u32 ret = 0;
	u8 cmd[] = {0x1, 0x17, 0x1, 0x0, 0x1};
	u8 event[] = {0x2, 0x17, 0x1, 0x0};

	do {
		ret = btmtk_sdio_send_wohci(HCI_COMMAND_PKT, sizeof(cmd), cmd);

		if (ret) {
			pr_notice("%s btmtk_sdio_send_wohci return fail ret %d\n",
					__func__, ret);
			break;
		}

		ret = btmtk_sdio_recv_rx_data();
		if (ret)
			break;

		if (rx_length == 12) {
			if (memcmp(rxbuf+7, event, sizeof(event)) == 0)
				return rxbuf[11];

			pr_notice("%s receive event content is not correct, print receive data\n",
				__func__);
			btmtk_print_buffer_conent(rxbuf, rx_length);
		}
	} while (0);

	return ret;
}
static int btmtk_sdio_set_write_clear(void)
{
	u32 u32ReadCRValue = 0;
	u32 ret = 0;

	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue);
	if (ret) {
		pr_notice("%s read CHCR error\n", __func__);
		ret = EINVAL;
		return ret;
	}

	u32ReadCRValue |= 0x00000002;
	btmtk_sdio_writel(CHCR, u32ReadCRValue);
	pr_info("%s write CHCR 0x%08X\n", __func__, u32ReadCRValue);
	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue);
	pr_info("%s read CHCR 0x%08X\n", __func__, u32ReadCRValue);
	if (u32ReadCRValue&0x00000002)
		pr_info("%s write clear\n", __func__);
	else
		pr_info("%s read clear\n", __func__);

	return ret;
}
static int btmtk_sdio_download_rom_patch(
					struct btmtk_sdio_card *card)
{
	const struct firmware *fw_firmware = NULL;
	const u8 *firmware = NULL;
	int firmwarelen, ret = 0;
	void *tmpfwbuf = NULL;
	u8 *fwbuf;
	struct _PATCH_HEADER *patchHdr;
	u8 *cDateTime = NULL;
	u16 u2HwVer = 0;
	u16 u2SwVer = 0;
	u32 u4PatchVer = 0;
	u32 uhwversion = 0;

	u32 u32ReadCRValue = 0;

	int RedundantSize = 0;
	u32 bufferOffset = 0;
	u8  patch_status = 0;

	ret = btmtk_sdio_set_own_back(DRIVER_OWN);

	if (ret)
		return ret;

	patch_status = btmtk_sdio_need_load_rom_patch();

	pr_debug("%s patch_status %d\n", __func__, patch_status);

	if (patch_status == PATCH_IS_DOWNLOAD_BT_OTHER ||
			patch_status == PATCH_READY) {
		pr_info("%s patch is ready no need load patch again\n",
					__func__);

		ret = btmtk_sdio_readl(0, &u32ReadCRValue);
		pr_info("%s read chipid =  %x\n", __func__, u32ReadCRValue);

		/*Set interrupt output*/
		ret = btmtk_sdio_writel(CHIER, FIRMWARE_INT|TX_FIFO_OVERFLOW |
				FW_INT_IND_INDICATOR | TX_COMPLETE_COUNT |
				TX_UNDER_THOLD | TX_EMPTY | RX_DONE);

		if (ret) {
			pr_notice("Set interrupt output fail(%d)\n", ret);
			ret = -EIO;
		}

		/*enable interrupt output*/
		ret = btmtk_sdio_writel(CHLPCR, C_FW_INT_EN_SET);
		if (ret) {
			pr_notice("enable interrupt output fail(%d)\n", ret);
			ret = -EIO;
			goto done;
		}

		ret = btmtk_sdio_bt_set_power(1);
		if (ret)
			return ret;

#if BTMTK_BIN_FILE_MODE
		/* Send hci cmd before sleep */
		btmtk_eeprom_bin_file(card);
#endif

		ret = btmtk_sdio_set_sleep();
		btmtk_sdio_set_write_clear();
		return ret;
	}

	uhwversion = btmtk_sdio_bt_memRegister_read(HW_VERSION);
	pr_info("%s uhwversion 0x%x\n", __func__, uhwversion);

	if (uhwversion == 0x8A00) {
		pr_info("%s request_firmware(firmware name %s)\n",
				__func__, card->firmware);
		ret = request_firmware(&fw_firmware, card->firmware,
						&card->func->dev);

		if ((ret < 0) || !fw_firmware) {
			pr_notice("request_firmware(firmware name %s) failed, error code = %d\n",
					card->firmware,
					ret);
			ret = -ENOENT;
			goto done;
		}
	} else {
		pr_info("%s request_firmware(firmware name %s)\n",
				__func__, card->firmware1);
		ret = request_firmware(&fw_firmware,
				card->firmware1,
				&card->func->dev);

		if ((ret < 0) || !fw_firmware) {
			pr_notice("request_firmware(firmware name %s) failed, error code = %d\n",
				card->firmware1, ret);
			ret = -ENOENT;
			goto done;
		}
	}

	firmware = fw_firmware->data;
	firmwarelen = fw_firmware->size;

	pr_debug("Downloading FW image (%d bytes)\n", firmwarelen);

	tmpfwbuf = kzalloc(firmwarelen, GFP_KERNEL);

	if (!tmpfwbuf) {
		ret = -ENOMEM;
		goto done;
	}

	/* Ensure aligned firmware buffer */
	memcpy(tmpfwbuf, firmware, firmwarelen);
	fwbuf = tmpfwbuf;

	/*Display rom patch info*/
	patchHdr =  (struct _PATCH_HEADER *)fwbuf;
	cDateTime = patchHdr->ucDateTime;
	u2HwVer = patchHdr->u2HwVer;
	u2SwVer = patchHdr->u2SwVer;
	u4PatchVer = patchHdr->u4PatchVer;

	pr_debug("=====================================\n");
	pr_info("===============Patch Info============\n");
	pr_info("Built Time = %s\n", cDateTime);
	pr_info("Hw Ver = 0x%x\n",
		((u2HwVer & 0x00ff) << 8) | ((u2HwVer & 0xff00) >> 8));
	pr_info("Sw Ver = 0x%x\n",
		((u2SwVer & 0x00ff) << 8) | ((u2SwVer & 0xff00) >> 8));
	pr_info("Patch Ver = 0x%04x\n",
			((u4PatchVer & 0xff000000) >> 24) |
			((u4PatchVer & 0x00ff0000) >> 16));
	pr_info("Platform = %c%c%c%c\n",
			patchHdr->ucPlatform[0],
			patchHdr->ucPlatform[1],
			patchHdr->ucPlatform[2],
			patchHdr->ucPlatform[3]);
	pr_info("Patch start addr = %02x\n", patchHdr->u2PatchStartAddr);
	pr_info("=====================================\n");

	fwbuf += sizeof(struct _PATCH_HEADER);
	pr_debug("%s PATCH_HEADER size %zd\n",
		__func__, sizeof(struct _PATCH_HEADER));
	firmwarelen -= sizeof(struct _PATCH_HEADER);

	ret = btmtk_sdio_readl(0, &u32ReadCRValue);
	pr_info("%s read chipid =  %x\n", __func__, u32ReadCRValue);

	/*Set interrupt output*/
	ret = btmtk_sdio_writel(CHIER, FIRMWARE_INT|TX_FIFO_OVERFLOW |
		FW_INT_IND_INDICATOR | TX_COMPLETE_COUNT |
		TX_UNDER_THOLD | TX_EMPTY | RX_DONE);

	if (ret) {
		pr_notice("Set interrupt output fail(%d)\n", ret);
		ret = -EIO;
		goto done;
	}

	/*enable interrupt output*/
	ret = btmtk_sdio_writel(CHLPCR, C_FW_INT_EN_SET);

	if (ret) {
		pr_notice("enable interrupt output fail(%d)\n", ret);
		ret = -EIO;
		goto done;
	}

	RedundantSize = firmwarelen;
	pr_debug("%s firmwarelen %d\n", __func__, firmwarelen);

	do {
		bufferOffset = firmwarelen - RedundantSize;

		if (RedundantSize == firmwarelen &&
				 RedundantSize >= PATCH_DOWNLOAD_SIZE)
			ret = btmtk_send_rom_patch(fwbuf+bufferOffset,
				PATCH_DOWNLOAD_SIZE, SDIO_PATCH_DOWNLOAD_FIRST);
		else if (RedundantSize == firmwarelen)
			ret = btmtk_send_rom_patch(fwbuf+bufferOffset,
				RedundantSize, SDIO_PATCH_DOWNLOAD_FIRST);
		else if (RedundantSize < PATCH_DOWNLOAD_SIZE) {
			ret = btmtk_send_rom_patch(fwbuf+bufferOffset,
				RedundantSize, SDIO_PATCH_DOWNLOAD_END);
			pr_debug("%s patch downoad last patch part\n",
					__func__);
		} else
			ret = btmtk_send_rom_patch(fwbuf+bufferOffset,
				PATCH_DOWNLOAD_SIZE, SDIO_PATCH_DOWNLOAD_CON);

		RedundantSize -= PATCH_DOWNLOAD_SIZE;

		if (ret) {
			pr_notice("%s btmtk_send_rom_patch fail\n", __func__);
			goto done;
		}
		pr_debug("%s RedundantSize %d\n", __func__, RedundantSize);
		if (RedundantSize <= 0) {
			pr_debug("%s patch downoad finish\n", __func__);
			break;
		}
	} while (1);

	btmtk_sdio_set_write_clear();

	if (btmtk_sdio_need_load_rom_patch() == PATCH_READY)
		pr_info("%s patch is ready\n", __func__);


	ret = btmtk_sdio_send_wmt_reset();

	if (ret)
		goto done;


	ret = btmtk_sdio_bt_set_power(1);

	if (ret) {
		ret = EINVAL;
		goto done;
	}

#if BTMTK_BIN_FILE_MODE
	/* Send hci cmd before sleep */
	btmtk_eeprom_bin_file(card);
#endif

	ret = btmtk_sdio_set_sleep();

done:

	kfree(tmpfwbuf);
	release_firmware(fw_firmware);

	if (!ret)
		pr_info("%s success\n", __func__);
	else
		pr_info("%s fail\n", __func__);

	return ret;
}

static void btmtk_sdio_for_code_style(void)
{
	pr_info("%s  vfs_fsync\n", __func__);

#if SAVE_FW_DUMP_IN_KERNEL
	if (fw_dump_file)
		vfs_fsync(fw_dump_file, 0);
#endif

	if (fw_dump_file) {
		pr_info("%s : close file  %s\n",
			__func__, fw_dump_file_name);
#if SAVE_FW_DUMP_IN_KERNEL
		filp_close(fw_dump_file, NULL);
/* #endif */
		fw_dump_file = NULL;
#endif
		fw_is_doing_coredump = false;
		fw_is_coredump_end_packet = true;
	} else {
		fw_is_doing_coredump = false;
		pr_info("%s : fw_dump_file is NULL can't close file %s\n",
			__func__, fw_dump_file_name);
	}
}

static int btmtk_sdio_card_to_host(struct btmtk_private *priv)
{
	u16 buf_len = 0;
	int ret = 0, num_blocks = 0, blksz = 0;
	struct sk_buff *skb = NULL;
	struct sk_buff *fops_skb = NULL;
	u32 type;
	u8 *payload = NULL;
	u32 fourbalignment_len = 0;
	struct btmtk_sdio_card *card = priv->btmtk_dev.card;
	u32 dump_len = 0;
	char *core_dump_end = NULL;

	if (!card || !card->func) {
		pr_notice("card or function or is NULL!\n");
		ret = -EINVAL;
		goto exit;
	}

#if SUPPORT_FW_DUMP
	fw_is_coredump_end_packet = false;
	if (rx_length > (SDIO_HEADER_LEN+8)) {
		if (rxbuf[SDIO_HEADER_LEN] == 0x80) {
			dump_len = (rxbuf[SDIO_HEADER_LEN+1]&0x0F)*256
					+ rxbuf[SDIO_HEADER_LEN+2];
			pr_notice("%s get dump length %d\n",
				__func__, dump_len);
			if (rxbuf[SDIO_HEADER_LEN+5] == 0x6F &&
					rxbuf[SDIO_HEADER_LEN+6] == 0xFC) {

				fw_is_doing_coredump = true;

				#if SAVE_FW_DUMP_IN_KERNEL
				if ((fw_dump_total_read_size == 0)
						&& (fw_dump_file == NULL)) {
					if (current_fwdump_file_number
							== probe_counter)
						goto FW_DONE;
					/* #if SAVE_FW_DUMP_IN_KERNEL */
					memset(fw_dump_file_name, 0,
						sizeof(fw_dump_file_name));
					snprintf(fw_dump_file_name,
						sizeof(fw_dump_file_name),
						FW_DUMP_FILE_NAME"_%d",
						probe_counter);
					pr_notice("%s : open file %s\n",
							__func__,
							fw_dump_file_name);
					fw_dump_file = filp_open(
							fw_dump_file_name,
							O_RDWR | O_CREAT,
							0644);
					if (fw_dump_file) {
						current_fwdump_file_number =
							probe_counter;
						pr_notice("%s : open file %s success\n",
							__func__,
							fw_dump_file_name);
					} else
						pr_notice("%s : open file %s fail\n",
							__func__,
							fw_dump_file_name);
				/* #endif */
				}
				#endif
				fw_dump_total_read_size += dump_len;

				#if SAVE_FW_DUMP_IN_KERNEL
				if (fw_dump_file->f_op == NULL)
					pr_notice("%s : fw_dump_file->f_op is NULL\n",
								__func__);

				if (fw_dump_file->f_op->write == NULL)
					pr_notice("%s : fw_dump_file->f_op->write is NULL\n",
								__func__);


				if ((dump_len > 0) && fw_dump_file)
					fw_dump_file->f_op->write(fw_dump_file,
						&rxbuf[SDIO_HEADER_LEN+10],
						dump_len,
						&fw_dump_file->f_pos);

				#endif

				if (dump_len >= sizeof(FW_DUMP_END_EVENT)) {
					core_dump_end = strstr(
						&rxbuf[SDIO_HEADER_LEN+10],
						FW_DUMP_END_EVENT);
					pr_notice("%s : core_dump_end %d\n",
						__func__, SDIO_HEADER_LEN);
					if (core_dump_end)
						btmtk_sdio_for_code_style();
				}
			}
		}
	}
#endif

#if SAVE_FW_DUMP_IN_KERNEL
FW_DONE:
#endif

	type = rxbuf[MTK_SDIO_PACKET_HEADER_SIZE];

	btmtk_print_buffer_conent(rxbuf, rx_length);

	/* Read the length of data to be transferred , not include pkt type*/
	buf_len = rx_length-(MTK_SDIO_PACKET_HEADER_SIZE+1);

	pr_debug("buf_len : %d\n", buf_len);
	if (rx_length <= SDIO_HEADER_LEN) {
		pr_notice("invalid packet length: %d\n", buf_len);
		ret = -EINVAL;
		goto exit;
	}

	/* Allocate buffer */
	/* rx_length = num_blocks * blksz + BTSDIO_DMA_ALIGN*/
	skb = bt_skb_alloc(rx_length, GFP_ATOMIC);
	if (skb == NULL) {
		pr_notice("No free skb\n");
		ret = -ENOMEM;
		goto exit;
	}

	payload = rxbuf;
	pr_debug("%s rx_length %d,buf_len %d\n", __func__, rx_length, buf_len);

	memcpy(skb->data, &rxbuf[MTK_SDIO_PACKET_HEADER_SIZE+1], buf_len);

	switch (type) {
	case HCI_ACLDATA_PKT:
		pr_debug("%s data[2] 0x%02x, data[3] 0x%02x\n",
				__func__, skb->data[2], skb->data[3]);
		buf_len = skb->data[2] + skb->data[3]*256 + 4;
		pr_debug("%s acl buf_len %d\n", __func__, buf_len);
		break;
	case HCI_SCODATA_PKT:
		buf_len = skb->data[3] + 3;
		break;
	case HCI_EVENT_PKT:
		buf_len = skb->data[1] + 2;
		break;
	}

	lock_unsleepable_lock(&(metabuffer.spin_lock));
	if (priv->adapter->fops_mode &
			(!fw_is_doing_coredump) &
			(!fw_is_coredump_end_packet)) {
		fops_skb = bt_skb_alloc(buf_len, GFP_ATOMIC);
		bt_cb(fops_skb)->pkt_type = type;
		memcpy(fops_skb->data, skb->data, buf_len);
		fops_skb->len = buf_len;
		skb_queue_tail(&g_priv->adapter->fops_queue, fops_skb);
		pr_debug("%s push fops_queue\n", __func__);
		if (skb_queue_empty(&g_priv->adapter->fops_queue))
			pr_info("%s fops_queue is empty\n", __func__);

		kfree_skb(skb);
		unlock_unsleepable_lock(&(metabuffer.spin_lock));
		pr_debug("%s call inq wake up\n", __func__);
		wake_up_interruptible(&inq);
		goto exit;
	}
	unlock_unsleepable_lock(&(metabuffer.spin_lock));

	switch (type) {
	case HCI_ACLDATA_PKT:
	case HCI_SCODATA_PKT:
	case HCI_EVENT_PKT:
		bt_cb(skb)->pkt_type = type;
		skb_put(skb, buf_len);
		break;

	case MTK_VENDOR_PKT:
		pr_notice("%s, MTK_VENDOR_PKT no handle now, break\n",
			__func__);
		kfree_skb(skb);
		break;

		bt_cb(skb)->pkt_type = HCI_VENDOR_PKT;
		skb_put(skb, buf_len);
		skb_pull(skb, SDIO_HEADER_LEN);

/* if kernel < 3, 11, 0, should use hci_recv_frame(skb); */

		break;

	default:
		pr_notice("Unknown packet type:%d\n", type);
		pr_notice("hex: %*ph\n", blksz * num_blocks, payload);

		kfree_skb(skb);
		skb = NULL;
		break;
	}

exit:
	if (ret) {
		pr_debug("%s fail free skb\n", __func__);
		kfree_skb(skb);
	}


	buf_len += 1;
	if (buf_len%4)
		fourbalignment_len = buf_len + 4 - buf_len%4;
	else
		fourbalignment_len = buf_len;

	rx_length -= fourbalignment_len;

	if (rx_length > (MTK_SDIO_PACKET_HEADER_SIZE)) {
		memcpy(&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE],
		&rxbuf[MTK_SDIO_PACKET_HEADER_SIZE+fourbalignment_len],
		rx_length-MTK_SDIO_PACKET_HEADER_SIZE);
	}

	pr_debug("%s ret %d, rx_length, %d,fourbalignment_len %d <--\n",
		__func__, ret, rx_length, fourbalignment_len);

	return ret;
}

static int btmtk_sdio_process_int_status(
		struct btmtk_private *priv)
{
	int ret = 0;
	u32 u32rxdatacount = 0;
	u32 u32ReadCRValue = 0;

	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue);
	pr_debug("%s check TX_EMPTY CHISR 0x%08x\n", __func__, u32ReadCRValue);
	if (TX_EMPTY&u32ReadCRValue) {
		ret = btmtk_sdio_writel(CHISR, (TX_EMPTY | TX_COMPLETE_COUNT));
		priv->btmtk_dev.tx_dnld_rdy = true;
		pr_debug("%s set tx_dnld_rdy 1\n", __func__);
	}

	if (RX_DONE&u32ReadCRValue)
		ret = btmtk_sdio_recv_rx_data();

	if (ret == 0)
		while (rx_length > (MTK_SDIO_PACKET_HEADER_SIZE)) {
			btmtk_sdio_card_to_host(priv);
			u32rxdatacount++;
			pr_debug("%s u32rxdatacount %d\n",
				__func__, u32rxdatacount);
		}

	btmtk_sdio_enable_interrupt(1);

	return 0;
}

static void btmtk_sdio_interrupt(struct sdio_func *func)
{
	struct btmtk_private *priv;
	struct btmtk_sdio_card *card;

	card = sdio_get_drvdata(func);

	if (!card)
		return;


	if (!card->priv)
		return;

	priv = card->priv;
	btmtk_sdio_enable_interrupt(0);

	btmtk_interrupt(priv);
}

static int btmtk_sdio_register_dev(struct btmtk_sdio_card *card)
{
	struct sdio_func *func;
	u32	u32ReadCRValue = 0;
	u8 reg;
	int ret = 0;

	if (!card || !card->func) {
		pr_notice("Error: card or function is NULL!\n");
		ret = -EINVAL;
		goto failed;
	}

	func = card->func;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret) {
		pr_notice("sdio_enable_func() failed: ret=%d\n", ret);
		ret = -EIO;
		goto release_host;
	}

	btmtk_sdio_readb(SDIO_CCCR_IENx, &u32ReadCRValue);
	pr_info("before claim irq read SDIO_CCCR_IENx %x, func num %d\n",
		u32ReadCRValue, func->num);

	ret = sdio_claim_irq(func, btmtk_sdio_interrupt);
	if (ret) {
		pr_notice("sdio_claim_irq failed: ret=%d\n", ret);
		ret = -EIO;
		goto disable_func;
	}
	pr_info("sdio_claim_irq success: ret=%d\n", ret);

	btmtk_sdio_readb(SDIO_CCCR_IENx, &u32ReadCRValue);
	pr_info("after claim irq read SDIO_CCCR_IENx %x\n", u32ReadCRValue);

	ret = sdio_set_block_size(card->func, SDIO_BLOCK_SIZE);
	if (ret) {
		pr_notice("cannot set SDIO block size\n");
		ret = -EIO;
		goto release_irq;
	}

	reg = sdio_readb(func, card->reg->io_port_0, &ret);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}

	card->ioport = reg;

	reg = sdio_readb(func, card->reg->io_port_1, &ret);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}

	card->ioport |= (reg << 8);

	reg = sdio_readb(func, card->reg->io_port_2, &ret);
	if (ret < 0) {
		ret = -EIO;
		goto release_irq;
	}

	card->ioport |= (reg << 16);

	pr_info("SDIO FUNC%d IO port: 0x%x\n", func->num, card->ioport);

	if (card->reg->int_read_to_clear) {
		reg = sdio_readb(func, card->reg->host_int_rsr, &ret);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
		sdio_writeb(func, reg | 0x3f, card->reg->host_int_rsr, &ret);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}

		reg = sdio_readb(func, card->reg->card_misc_cfg, &ret);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
		sdio_writeb(func, reg | 0x10, card->reg->card_misc_cfg, &ret);
		if (ret < 0) {
			ret = -EIO;
			goto release_irq;
		}
	}

	sdio_set_drvdata(func, card);

	sdio_release_host(func);

	return 0;

release_irq:
	sdio_release_irq(func);

disable_func:
	sdio_disable_func(func);

release_host:
	sdio_release_host(func);

failed:
	pr_info("%s fail\n", __func__);
	return ret;
}

static int btmtk_sdio_unregister_dev(struct btmtk_sdio_card *card)
{
	if (card && card->func) {
		sdio_claim_host(card->func);
		sdio_release_irq(card->func);
		sdio_disable_func(card->func);
		sdio_release_host(card->func);
		sdio_set_drvdata(card->func, NULL);
	}

	return 0;
}

static int btmtk_sdio_enable_host_int(struct btmtk_sdio_card *card)
{
	int ret;
	u32 read_data = 0;

	if (!card || !card->func)
		return -EINVAL;

	sdio_claim_host(card->func);

	ret = btmtk_sdio_enable_host_int_mask(card, HIM_ENABLE);

	btmtk_sdio_get_rx_unit(card);

	if (0) {
		typedef int (*fp_sdio_hook)(struct mmc_host *host,
						unsigned int width);
		fp_sdio_hook func_sdio_hook =
			(fp_sdio_hook)kallsyms_lookup_name("mmc_set_bus_width");
		unsigned char data = 0;

		data = sdio_f0_readb(card->func, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_readb ret %d\n", __func__, ret);

		pr_info("%s sdio_f0_readb data 0x%X!\n", __func__, data);

		data  &= ~SDIO_BUS_WIDTH_MASK;
		data  |= SDIO_BUS_ASYNC_INT;
		card->func->card->quirks |= MMC_QUIRK_LENIENT_FN0;

		sdio_f0_writeb(card->func, data, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_writeb ret %d\n", __func__, ret);

		pr_info("%s func_sdio_hook at 0x%p!\n",
			__func__, func_sdio_hook);
		if (func_sdio_hook)
			func_sdio_hook(card->func->card->host, MMC_BUS_WIDTH_1);

		data = sdio_f0_readb(card->func, SDIO_CCCR_IF, &ret);
		if (ret)
			pr_info("%s sdio_f0_readb 2 ret %d\n",
				__func__, ret);

		pr_info("%s sdio_f0_readb2 data 0x%X\n", __func__, data);
	}

	sdio_release_host(card->func);

/* workaround for some platform no host clock sometimes */

	btmtk_sdio_readl(CSDIOCSR, &read_data);
	pr_info("%s read CSDIOCSR is 0x%X\n", __func__, read_data);
	read_data |= 0x4;
	btmtk_sdio_writel(CSDIOCSR, read_data);
	pr_info("%s write CSDIOCSR is 0x%X\n", __func__, read_data);

	return ret;
}

static int btmtk_sdio_disable_host_int(struct btmtk_sdio_card *card)
{
	int ret;

	if (!card || !card->func)
		return -EINVAL;

	sdio_claim_host(card->func);

	ret = btmtk_sdio_disable_host_int_mask(card, HIM_DISABLE);

	sdio_release_host(card->func);

	return ret;
}

static int btmtk_sdio_host_to_card(struct btmtk_private *priv,
				u8 *payload, u16 nb)
{
	struct btmtk_sdio_card *card = priv->btmtk_dev.card;
	int ret = 0;
	int i = 0;
	u8 MultiBluckCount = 0;
	u8 redundant = 0;

	if (payload != txbuf) {
		memset(txbuf, 0, MTK_TXDATA_SIZE);
		memcpy(txbuf, payload, nb);
	}

	if (!card || !card->func) {
		pr_notice("card or function is NULL!\n");
		return -EINVAL;
	}

	MultiBluckCount = nb/SDIO_BLOCK_SIZE;
	redundant = nb % SDIO_BLOCK_SIZE;

	if (redundant)
		nb = (MultiBluckCount+1)*SDIO_BLOCK_SIZE;

	if (nb < 16)
		btmtk_print_buffer_conent(txbuf, nb);
	else
		btmtk_print_buffer_conent(txbuf, 16);

	do {
		/* Transfer data to card */
		sdio_claim_host(card->func);
		ret = sdio_writesb(card->func, CTDR, txbuf, nb);
		sdio_release_host(card->func);
		if (ret < 0) {
			i++;
			pr_notice("i=%d writesb failed: %d\n", i, ret);
			pr_notice("hex: %*ph\n", nb, txbuf);
			ret = -EIO;
			if (i > MAX_WRITE_IOMEM_RETRY)
				goto exit;
		}
	} while (ret);

	if (priv)
		priv->btmtk_dev.tx_dnld_rdy = false;

exit:
	return ret;
}

static int btmtk_sdio_download_fw(struct btmtk_sdio_card *card)
{
	int ret;

	pr_info("%s begin\n", __func__);
	if (!card || !card->func) {
		pr_notice("card or function is NULL!\n");
		return -EINVAL;
	}

	sdio_claim_host(card->func);

	if (btmtk_sdio_download_rom_patch(card)) {
		pr_notice("Failed to download firmware!\n");
		ret = -EIO;
		goto done;
	}

	/*
	 * winner or not, with this test the FW synchronizes
	 * when the
	 * module can continue its initialization
	 */
	sdio_release_host(card->func);

	return 0;
done:
	sdio_release_host(card->func);
	return ret;
}

static int btmtk_sdio_push_data_to_metabuffer(
						char *data,
						int len,
						u8 type)
{
	int remainLen = 0;

	if (metabuffer.write_p >= metabuffer.read_p)
		remainLen = metabuffer.write_p - metabuffer.read_p;
	else
		remainLen = META_BUFFER_SIZE -
			(metabuffer.read_p - metabuffer.write_p);

	if ((remainLen + 1 + len) >= META_BUFFER_SIZE) {
		pr_notice("%s copy copyLen %d > META_BUFFER_SIZE(%d), push back to queue\n",
			__func__,
			(remainLen + 1 + len),
			META_BUFFER_SIZE);
		return -1;
	}

	metabuffer.buffer[metabuffer.write_p] = type;
	metabuffer.write_p++;
	if (metabuffer.write_p >= META_BUFFER_SIZE)
		metabuffer.write_p = 0;

	if (metabuffer.write_p + len <= META_BUFFER_SIZE)
		memcpy(&metabuffer.buffer[metabuffer.write_p],
			data,
			len);
	else {
		memcpy(&metabuffer.buffer[metabuffer.write_p],
			data,
			META_BUFFER_SIZE - metabuffer.write_p);
		memcpy(metabuffer.buffer,
			&data[META_BUFFER_SIZE - metabuffer.write_p],
			len - (META_BUFFER_SIZE - metabuffer.write_p));
	}

	metabuffer.write_p += len;
	if (metabuffer.write_p >= META_BUFFER_SIZE)
		metabuffer.write_p -= META_BUFFER_SIZE;

	remainLen += (1 + len);
	return 0;
}

static int btmtk_sdio_pull_data_from_metabuffer(
						char __user *buf,
						size_t count)
{
	int copyLen = 0;
	unsigned long ret = 0;

	if (metabuffer.write_p >= metabuffer.read_p)
		copyLen = metabuffer.write_p - metabuffer.read_p;
	else
		copyLen = META_BUFFER_SIZE -
			(metabuffer.read_p - metabuffer.write_p);

	if (copyLen > count)
		copyLen = count;

	if (metabuffer.read_p + copyLen <= META_BUFFER_SIZE)
		ret = copy_to_user(buf,
				&metabuffer.buffer[metabuffer.read_p],
				copyLen);
	else {
		ret = copy_to_user(buf,
				&metabuffer.buffer[metabuffer.read_p],
				META_BUFFER_SIZE - metabuffer.read_p);
		if (!ret)
			ret = copy_to_user(
				&buf[META_BUFFER_SIZE - metabuffer.read_p],
				metabuffer.buffer,
				copyLen - (META_BUFFER_SIZE-metabuffer.read_p));
	}

	if (ret)
		pr_notice("%s copy to user fail, ret %d\n", __func__, (int)ret);

	metabuffer.read_p += (copyLen - ret);
	if (metabuffer.read_p >= META_BUFFER_SIZE)
		metabuffer.read_p -= META_BUFFER_SIZE;

	return (copyLen - ret);
}

static int btmtk_sdio_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	int ret = 0;
	struct btmtk_private *priv = NULL;
	struct btmtk_sdio_card *card = NULL;
	struct btmtk_sdio_device *data = (void *) id->driver_data;
	u32 u32ReadCRValue = 0;

	probe_counter++;
	pr_info("%s Mediatek Bluetooth driver Version=%s\n",
			__func__, VERSION);
	pr_info("vendor=0x%x, device=0x%x, class=%d, fn=%d, support func_num %d\n",
			id->vendor, id->device, id->class,
			func->num, data->reg->func_num);

	if (func->num != data->reg->func_num) {
		pr_info("func num is not match\n");
		return -ENODEV;
	}

	card = devm_kzalloc(&func->dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;

	card->func = func;
	g_card = card;

	if (id->driver_data) {
		card->helper = data->helper;
		card->firmware = data->firmware;
		card->firmware1 = data->firmware1;
		card->reg = data->reg;
		card->sd_blksz_fw_dl = data->sd_blksz_fw_dl;
		card->support_pscan_win_report = data->support_pscan_win_report;
		card->supports_fw_dump = data->supports_fw_dump;
	}

	pr_info("%s func device %X\n", __func__, card->func->device);
	pr_info("%s Call btmtk_sdio_register_dev\n", __func__);
	if (btmtk_sdio_register_dev(card) < 0) {
		pr_notice("Failed to register BT device!\n");
		return -ENODEV;
	}

	pr_info("%s btmtk_sdio_register_dev success\n", __func__);

	/* Disable the interrupts on the card */
	btmtk_sdio_enable_host_int(card);
	pr_info("call btmtk_sdio_enable_host_int done\n");
	if (btmtk_sdio_download_fw(card)) {
		pr_notice("Downloading firmware failed!\n");
		ret = -ENODEV;
		goto unreg_dev;
	}

	/* Move from btmtk_fops_open() */
	spin_lock_init(&(metabuffer.spin_lock.lock));
	pr_info("%s spin_lock_init end\n", __func__);

	priv = btmtk_add_card(card);
	if (!priv) {
		pr_notice("Initializing card failed!\n");
		ret = -ENODEV;
		goto unreg_dev;
	}
	pr_info("btmtk_add_card success\n");
	card->priv = priv;
	pr_info("assign priv done\n");
	/* Initialize the interface specific function pointers */
	priv->hw_host_to_card = btmtk_sdio_host_to_card;
	priv->hw_process_int_status = btmtk_sdio_process_int_status;
	priv->hw_set_own_back =  btmtk_sdio_set_own_back;

	g_priv = priv;
	if (!fw_dump_ptr)
		fw_dump_ptr = kmalloc(FW_DUMP_BUF_SIZE, GFP_ATOMIC);

	if (!fw_dump_ptr) {
		ret = -ENODEV;
		return ret;
	}

	memset(&metabuffer.buffer, 0, META_BUFFER_SIZE);
	memset(fw_dump_ptr, 0, FW_DUMP_BUF_SIZE);

	fw_dump_task_should_stop = 0;
#if SAVE_FW_DUMP_IN_KERNEL
	fw_dump_file = NULL;
#endif
	fw_dump_read_ptr = fw_dump_ptr;
	fw_dump_write_ptr = fw_dump_ptr;
	fw_dump_total_read_size = 0;
	fw_dump_total_write_size = 0;
	fw_dump_buffer_used_size = 0;
	fw_dump_buffer_full = 0;

	ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue);
	pr_info("%s read CHLPCR (0x%08X)\n", __func__, u32ReadCRValue);
	pr_info("%s normal end\n", __func__);
	probe_ready = true;
	return 0;

unreg_dev:
	btmtk_sdio_unregister_dev(card);

	pr_notice("%s fail end\n", __func__);
	return ret;
}

static void btmtk_sdio_remove(struct sdio_func *func)
{
	struct btmtk_sdio_card *card;

	pr_info("%s begin user_rmmod %d\n", __func__, user_rmmod);
	probe_ready = false;

	if (func) {
		card = sdio_get_drvdata(func);
		if (card) {
			/* Send SHUTDOWN command & disable interrupt
			 * if user removes the module.
			 */
			if (user_rmmod) {
				pr_info("%s begin user_rmmod %d in user mode\n",
					__func__, user_rmmod);
				btmtk_sdio_set_own_back(DRIVER_OWN);
				btmtk_sdio_enable_interrupt(0);
				btmtk_sdio_bt_set_power(0);
				btmtk_sdio_set_own_back(FW_OWN);

				btmtk_sdio_disable_host_int(card);
			}
			pr_debug("unregester dev\n");
			card->priv->surprise_removed = true;
			btmtk_sdio_unregister_dev(card);
			btmtk_remove_card(card->priv);
		}
	}
	pr_info("%s end\n", __func__);
}

/*
 * cmd_type:
 * #define HCI_COMMAND_PKT   0x01
 * #define HCI_ACLDATA_PKT   0x02
 * #define HCI_SCODATA_PKT   0x03
 * #define HCI_EVENT_PKT     0x04
 * #define HCI_VENDOR_PKT    0xff
 */
static int btmtk_sdio_send_cmd(u8 cmd_type, u8 *cmd, int cmd_len)
{
	u8 ret = 0;
	u32 sdio_header_len = 0;
	u8 *send_data = NULL;
	u32 send_data_len = cmd_len + BTM_HEADER_LEN;

	if (cmd_len == 0) {
		pr_notice("%s cmd_len (%d) error return\n", __func__, cmd_len);
		return -EINVAL;
	}

	send_data = kmalloc(send_data_len, GFP_KERNEL);
	sdio_header_len = send_data_len;
	memset(send_data, 0, send_data_len);
	send_data[0] = (sdio_header_len & 0x0000ff);
	send_data[1] = (sdio_header_len & 0x00ff00) >> 8;
	send_data[2] = 0;
	send_data[3] = 0;
	send_data[4] = cmd_type;
	memcpy(&send_data[BTM_HEADER_LEN], &cmd[0], cmd_len);
	ret = btmtk_sdio_host_to_card(g_priv, send_data, send_data_len);
	kfree(send_data);
	return ret;
}
static int btmtk_sdio_send_woble_cmd(void)
{
	u8 ret = 0;
	u8 cmd[] = { 0xC9, 0xFC, 0x0D, 0x01, 0x0E, 0x00, 0x05, 0x43,
			0x52, 0x4B, 0x54, 0x4D, 0x20, 0x04, 0x32, 0x00};

	ret = btmtk_sdio_send_cmd(HCI_COMMAND_PKT, cmd, sizeof(cmd));
	pr_info("%s return %d\n", __func__, ret);
	return ret;
}

static int btmtk_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	u8 ret = 0;
	mmc_pm_flag_t pm_flags;

	ret = btmtk_sdio_set_own_back(DRIVER_OWN);
	if (ret)
		pr_notice("%s set driver own fail\n", __func__);

	ret = btmtk_sdio_send_woble_cmd();
	if (ret)
		pr_notice("%s set woble cmd fail\n", __func__);

	need_reset_stack = 1;
	pr_notice("%s set reset_stack 1\n", __func__);
	if (func) {
		pm_flags = sdio_get_host_pm_caps(func);
		pr_debug("%s: suspend: PM flags = 0x%x\n",
			sdio_func_id(func), pm_flags);
		if (!(pm_flags & MMC_PM_KEEP_POWER)) {
			pr_notice("%s: cannot remain alive while suspended\n",
				sdio_func_id(func));
			return -EINVAL;
		}
	} else {
		pr_notice("sdio_func is not specified\n");
		return 0;
	}
	ret = btmtk_sdio_set_own_back(FW_OWN);
	if (ret)
		pr_notice("%s set fw own fail\n", __func__);
	return sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
}

static int btmtk_sdio_resume(struct device *dev)
{
	pr_info("%s begin return 0, do nothing\n", __func__);
	return 0;
}

static const struct dev_pm_ops btmtk_sdio_pm_ops = {
	.suspend       = btmtk_sdio_suspend,
	.resume        = btmtk_sdio_resume,
};

static struct sdio_driver bt_mtk_sdio = {
	.name          = "btmtk_sdio",
	.id_table      = btmtk_sdio_ids,
	.probe         = btmtk_sdio_probe,
	.remove        = btmtk_sdio_remove,
	.drv = {
		.owner = THIS_MODULE,
		.pm = &btmtk_sdio_pm_ops,
	}
};

static int btmtk_fops_open(struct inode *inode, struct file *file)
{
	pr_info("%s begin\n", __func__);

	if (!probe_ready) {
		pr_notice("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	metabuffer.read_p = 0;
	metabuffer.write_p = 0;
#if 0 /* Move to btmtk_sdio_probe() */
	spin_lock_init(&(metabuffer.spin_lock.lock));
	pr_info("%s spin_lock_init end\n", __func__);
#endif
	if (g_priv == NULL) {
		pr_notice("%s g_priv is NULL\n", __func__);
		return -ENOENT;
	}

	if (g_priv->adapter == NULL) {
		pr_notice("%s g_priv->adapter is NULL\n", __func__);
		return -ENOENT;
	}

	if (g_priv)
		g_priv->adapter->fops_mode = true;

	pr_info("%s fops_mode=%d end\n", __func__, g_priv->adapter->fops_mode);
	return 0;
}

static int btmtk_fops_close(struct inode *inode, struct file *file)
{
	struct sk_buff *skb = NULL;

	pr_info("%s begin\n", __func__);

	if (!probe_ready) {
		pr_notice("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	spin_lock_init(&(metabuffer.spin_lock.lock));
	if (g_priv)
		g_priv->adapter->fops_mode = false;

	lock_unsleepable_lock(&(metabuffer.spin_lock));

	if (!skb_queue_empty(&g_priv->adapter->fops_queue)) {
		pr_info("%s clean data in fops_queue\n", __func__);
		do {
			skb = skb_dequeue(&g_priv->adapter->fops_queue);
			if (skb == NULL) {
				pr_info("%s skb=NULL error break\n", __func__);
				break;
			}

			kfree_skb(skb);
		} while (!skb_queue_empty(&g_priv->adapter->fops_queue));
	}
	unlock_unsleepable_lock(&(metabuffer.spin_lock));

	pr_info("%s fops_mode=%d end\n", __func__, g_priv->adapter->fops_mode);
	return 0;
}

ssize_t btmtk_fops_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	int retval = 0;
	struct sk_buff *skb = NULL;
	u32 crAddr = 0, crValue = 0, crMask = 0;
	/*int i = 0;*/

	if (!probe_ready) {
		pr_notice("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -EFAULT;
	}

	if (g_priv->adapter->fops_mode == 0) {
		pr_info("%s fops_mode is 0\n", __func__);
		return -EFAULT;
	}
#if 0
	pr_info("%s : (%d) %02X %02X %02X %02X "
			%"02X %02X %02X %02X\n",
			__func__, (int)count,
			buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], buf[6], buf[7]);

	pr_info("%s print write data", __func__);
	if (count > 10)
		pr_info("  %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
				buf[0], buf[1], buf[2], buf[3], buf[4],
				buf[5], buf[6], buf[7], buf[8], buf[9]);
	else {
		for (i = 0; i < count; i++)
			pr_info("%d %02X", i, buf[i]);
	}
#endif
	if (buf[0] == 0x7) {
		/* write CR */
		if (count < 15) {
			pr_info("%s count=%zd less than 15, error\n",
				__func__, count);
			return -EFAULT;
		}

		crAddr = (buf[3]&0xff) + ((buf[4]&0xff)<<8)
			+ ((buf[5]&0xff)<<16) + ((buf[6]&0xff)<<24);
		crValue = (buf[7]&0xff) + ((buf[8]&0xff)<<8)
			+ ((buf[9]&0xff)<<16) + ((buf[10]&0xff)<<24);
		crMask = (buf[11]&0xff) + ((buf[12]&0xff)<<8)
			+ ((buf[13]&0xff)<<16) + ((buf[14]&0xff)<<24);

		pr_info("%s crAddr=0x%08x crValue=0x%08x crMask=0x%08x\n",
			__func__, crAddr, crValue, crMask);
		crValue &= crMask;

		pr_info("%s write crAddr=0x%08x crValue=0x%08x\n", __func__,
			crAddr, crValue);
		btmtk_sdio_writel(crAddr, crValue);
		retval = count;
	} else if (buf[0] == 0x8) {
		/* read CR */
		if (count < 16) {
			pr_info("%s count=%zd less than 15, error\n",
				__func__, count);
			return -EFAULT;
		}

		crAddr = (buf[3]&0xff) + ((buf[4]&0xff)<<8) +
			((buf[5]&0xff)<<16) + ((buf[6]&0xff)<<24);
		crMask = (buf[11]&0xff) + ((buf[12]&0xff)<<8) +
			((buf[13]&0xff)<<16) + ((buf[14]&0xff)<<24);

		btmtk_sdio_readl(crAddr, &crValue);
		pr_info("%s read crAddr=0x%08x crValue=0x%08x crMask=0x%08x\n",
				__func__, crAddr, crValue, crMask);
		retval = count;
	} else {
		skb = bt_skb_alloc(count-1, GFP_ATOMIC);
		bt_cb(skb)->pkt_type = buf[0];
		memcpy(&skb->data[0], &buf[1], count-1);
		skb->len = count-1;
		skb_queue_tail(&g_priv->adapter->tx_queue, skb);
		wake_up_interruptible(&g_priv->main_thread.wait_q);

		retval = count;
	}

	pr_debug("%s end\n", __func__);
	return retval;
}

ssize_t btmtk_fops_read(struct file *filp, char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct sk_buff *skb = NULL;

	if (!probe_ready) {
		pr_notice("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return -EFAULT;
	}

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		return -EFAULT;
	}

	if (g_priv->adapter->fops_mode == 0) {
		pr_info("%s fops_mode is 0\n", __func__);
		return -EFAULT;
	}

	lock_unsleepable_lock(&(metabuffer.spin_lock));
	if (skb_queue_empty(&g_priv->adapter->fops_queue)) {
		/* if (filp->f_flags & O_NONBLOCK) { */
		if (metabuffer.write_p == metabuffer.read_p) {
			unlock_unsleepable_lock(&(metabuffer.spin_lock));
			return 0;
		}
	}

	if (need_reset_stack == 1) {
		kill_fasync(&fasync, SIGIO, POLL_IN);
		need_reset_stack = 0;
		pr_info("%s Call kill_fasync and set reset_stack 0\n",
			__func__);
		return -ENODEV;
	}

	do {
		skb = skb_dequeue(&g_priv->adapter->fops_queue);
		if (skb == NULL) {
			pr_info("%s skb=NULL break\n", __func__);
			break;
		}
#if 0
		pr_debug("%s pkt_type %d metabuffer.buffer %d",
			__func__, bt_cb(skb)->pkt_type,
			metabuffer.buffer[copyLen]);
#endif
		btmtk_print_buffer_conent(skb->data, skb->len);

		if (btmtk_sdio_push_data_to_metabuffer(skb->data,
				skb->len, bt_cb(skb)->pkt_type) < 0) {
			skb_queue_head(&g_priv->adapter->fops_queue, skb);
			break;
		}
		kfree_skb(skb);
	} while (!skb_queue_empty(&g_priv->adapter->fops_queue));
	unlock_unsleepable_lock(&(metabuffer.spin_lock));

	return btmtk_sdio_pull_data_from_metabuffer(buf, count);
}

static int btmtk_fops_fasync(int fd, struct file *file, int on)
{
	pr_info("%s: fd = 0x%X, flag = 0x%X\n", __func__, fd, on);
	return fasync_helper(fd, file, on, &fasync);
}

unsigned int btmtk_fops_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	if (!probe_ready) {
		pr_notice("%s probe_ready is %d return\n",
			__func__, probe_ready);
		return mask;
	}

	if (g_priv == NULL) {
		pr_notice("%s g_priv is NULL\n", __func__);
		return -ENODEV;
	}

	if (metabuffer.write_p != metabuffer.read_p)
		mask |= (POLLIN | POLLRDNORM);

	if (skb_queue_empty(&g_priv->adapter->fops_queue)) {
		poll_wait(filp, &inq, wait);

		if (!skb_queue_empty(&g_priv->adapter->fops_queue)) {
			mask |= (POLLIN | POLLRDNORM);
			/* pr_info("%s poll done\n", __func__); */
		}
	} else
		mask |= (POLLIN | POLLRDNORM);

	mask |= (POLLOUT | POLLWRNORM);

	/* pr_info("%s poll mask 0x%x\n", __func__,mask); */
	return mask;
}

long btmtk_fops_unlocked_ioctl(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	u32 retval = 0;

	return retval;
}

static int btmtk_fops_openfwlog(struct inode *inode,
					struct file *file)
{
	if (g_priv == NULL) {
		pr_notice("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: OK\n", __func__);
	return 0;
}

static int btmtk_fops_closefwlog(struct inode *inode,
					struct file *file)
{
	if (g_priv == NULL) {
		pr_notice("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: OK\n", __func__);
	return 0;
}

static ssize_t btmtk_fops_readfwlog(struct file *filp,
			char __user *buf,
			size_t count,
			loff_t *f_pos)
{
	int copyLen = 0;

	if (g_priv == NULL) {
		pr_notice("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	pr_info("%s: OK\n", __func__);
	return copyLen;
}

static ssize_t btmtk_fops_writefwlog(
			struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct sk_buff *skb = NULL;
	int length = 0, i, j = 0;
	u8 *i_fwlog_buf = kmalloc(HCI_MAX_COMMAND_BUF_SIZE, GFP_KERNEL);
	u8 *o_fwlog_buf = kmalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
	const char *val_param = NULL;

	memset(i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);
	memset(o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);

	if (g_priv == NULL) {
		pr_info("%s g_priv is NULL\n", __func__);
		goto exit;
	}
	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		pr_notice("%s: your command is larger than maximum length, count = %zd\n",
			__func__, count);
		goto exit;
	}

	for (i = 0; i < count; i++) {
		if (buf[i] != '=') {
			i_fwlog_buf[i] = buf[i];
			pr_debug("%s: tag_param %02x\n",
				__func__, i_fwlog_buf[i]);
		} else {
			val_param = &buf[i+1];
			if (strcmp(i_fwlog_buf, "log_lvl") == 0) {
				pr_info("%s: btmtk_log_lvl = %d\n",
					__func__, val_param[0] - 48);
#if 0
				btmtk_log_lvl = val_param[0] - 48;
#endif
			}
			goto exit;
		}
	}

	if (i == count) {
		/* hci input command format : */
		/* echo 01 be fc 01 05 > /dev/stpbtfwlog */
		/* We take the data from index three to end. */
		val_param = &buf[0];
	}
	length = strlen(val_param);

	for (i = 0; i < length; i++) {
		u8 ret = 0;
		u8 temp_str[3] = { 0 };
		long res = 0;

		if (val_param[i] == ' ' || val_param[i] == '\t'
			|| val_param[i] == '\r' || val_param[i] == '\n')
			continue;
		if ((val_param[i] == '0' && val_param[i + 1] == 'x')
			|| (val_param[0] == '0' && val_param[i + 1] == 'X')) {
			i++;
			continue;
		}
		if (!(val_param[i] >= '0' && val_param[i] <= '9')
			&& !(val_param[i] >= 'A' && val_param[i] <= 'F')
			&& !(val_param[i] >= 'a' && val_param[i] <= 'f')) {
			break;
		}
		temp_str[0] = *(val_param+i);
		temp_str[1] = *(val_param+i+1);
		ret = (u8) (kstrtol((char *)temp_str, 16, &res) & 0xff);
		o_fwlog_buf[j++] = res;
		i++;
	}
	length = j;

	/*
	 * Receive command from stpbtfwlog, then Sent hci command
	 * to controller
	 */
	pr_debug("%s: hci buff is %02x%02x%02x%02x%02x\n",
		__func__, o_fwlog_buf[0], o_fwlog_buf[1],
		o_fwlog_buf[2], o_fwlog_buf[3], o_fwlog_buf[4]);
	/* check HCI command length */
	if (length > HCI_MAX_COMMAND_SIZE) {
		pr_notice("%s: your command is larger than maximum length, length = %d\n",
			__func__, length);
		goto exit;
	}

	pr_debug("%s: hci buff is %02x%02x%02x%02x%02x\n",
		__func__, o_fwlog_buf[0], o_fwlog_buf[1],
		o_fwlog_buf[2], o_fwlog_buf[3], o_fwlog_buf[4]);

	/*
	 * Receive command from stpbtfwlog, then Sent hci command
	 * to Stack
	 */
	skb = bt_skb_alloc(length - 1, GFP_ATOMIC);
	bt_cb(skb)->pkt_type = o_fwlog_buf[0];
	memcpy(&skb->data[0], &o_fwlog_buf[1], length - 1);
	skb->len = length - 1;
	skb_queue_tail(&g_priv->adapter->tx_queue, skb);
	wake_up_interruptible(&g_priv->main_thread.wait_q);

	pr_info("%s write end\n", __func__);
exit:
	pr_info("%s exit, length = %d\n", __func__, length);
	kfree(i_fwlog_buf);
	kfree(o_fwlog_buf);
	return count;
}

static unsigned int btmtk_fops_pollfwlog(
			struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_priv == NULL) {
		pr_notice("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	return mask;
}

static long btmtk_fops_unlocked_ioctlfwlog(
			struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int retval = 0;

	pr_info("%s: ->\n", __func__);
	if (g_priv == NULL) {
		pr_notice("%s: ERROR, g_data is NULL!\n", __func__);
		return -ENODEV;
	}

	return retval;
}

static int BTMTK_major;
static int BT_majorfwlog;
static struct cdev BTMTK_cdev;
static struct cdev BT_cdevfwlog;
static int BTMTK_devs = 1;

static wait_queue_head_t inq;
const struct file_operations BTMTK_fops = {
	.open = btmtk_fops_open,
	.release = btmtk_fops_close,
	.read = btmtk_fops_read,
	.write = btmtk_fops_write,
	.poll = btmtk_fops_poll,
	.unlocked_ioctl = btmtk_fops_unlocked_ioctl,
	.fasync = btmtk_fops_fasync
};

const struct file_operations BT_fopsfwlog = {
	.open = btmtk_fops_openfwlog,
	.release = btmtk_fops_closefwlog,
	.read = btmtk_fops_readfwlog,
	.write = btmtk_fops_writefwlog,
	.poll = btmtk_fops_pollfwlog,
	.unlocked_ioctl = btmtk_fops_unlocked_ioctlfwlog

};

static int BTMTK_init(void)
{
	dev_t devID = MKDEV(BTMTK_major, 0);
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 1);
	int ret = 0;
	int cdevErr = 0;
	int major = 0;
	int majorfwlog = 0;

	pr_info("BTMTK_init\n");

	g_card = NULL;
	txbuf = NULL;
	rxbuf = NULL;
	rx_length = 0;

#if SAVE_FW_DUMP_IN_KERNEL
	fw_dump_file = NULL;
#else
	fw_dump_file = 0;
#endif
	g_priv = NULL;

	fw_dump_buffer_full = 0;
	fw_dump_total_read_size = 0;
	fw_dump_total_write_size = 0;
	fw_dump_buffer_used_size = 0;
	fw_dump_task_should_stop = 0;
	fw_dump_ptr = NULL;
	fw_dump_read_ptr = NULL;
	fw_dump_write_ptr = NULL;
	probe_counter = 0;
	fw_dump_end_checking_task_should_stop = 0;
	fw_is_doing_coredump = 0;


	ret = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
	if (ret) {
		pr_notice("fail to allocate chrdev\n");
		return ret;
	}

	ret = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
	if (ret) {
		pr_notice("fail to allocate chrdev\n");
		return ret;
	}

	BTMTK_major = major = MAJOR(devID);
	pr_info("major number:%d\n", BTMTK_major);
	BT_majorfwlog = majorfwlog = MAJOR(devIDfwlog);
	pr_info("BT_majorfwlog number: %d\n", BT_majorfwlog);

	cdev_init(&BTMTK_cdev, &BTMTK_fops);
	BTMTK_cdev.owner = THIS_MODULE;

	cdev_init(&BT_cdevfwlog, &BT_fopsfwlog);
	BT_cdevfwlog.owner = THIS_MODULE;

	cdevErr = cdev_add(&BTMTK_cdev, devID, BTMTK_devs);
	if (cdevErr)
		goto error;

	cdevErr = cdev_add(&BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr)
		goto error;

	pr_info("%s driver(major %d) installed.\n",
			"BT_chrdev", BTMTK_major);
	pr_info("%s driver(major %d) installed.\n",
			"BT_chrdevfwlog", BT_majorfwlog);

	pBTClass = class_create(THIS_MODULE, "BT_chrdev");
	if (IS_ERR(pBTClass)) {
		pr_notice("class create fail, error code(%ld)\n",
			PTR_ERR(pBTClass));
		goto err1;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, BT_NODE);
	if (IS_ERR(pBTDev)) {
		pr_notice("device create fail, error code(%ld)\n",
			PTR_ERR(pBTDev));
		goto err2;
	}

	pBTDevfwlog = device_create(pBTClass, NULL,
				devIDfwlog, NULL, "stpbtfwlog");
	if (IS_ERR(pBTDevfwlog)) {
		pr_notice("device(stpbtfwlog) create fail, error code(%ld)\n",
			PTR_ERR(pBTDevfwlog));
		goto err2;
	}

	pr_info("%s: BT_major %d, BT_majorfwlog %d\n",
			__func__, BTMTK_major, BT_majorfwlog);
	pr_info("%s: devID %d, devIDfwlog %d\n", __func__, devID, devIDfwlog);

	/* init wait queue */
	g_devIDfwlog = devIDfwlog;
	init_waitqueue_head(&(fw_log_inq));
	init_waitqueue_head(&(inq));

	return 0;

 err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

 err1:

 error:
	if (cdevErr == 0)
		cdev_del(&BTMTK_cdev);

	if (ret == 0)
		unregister_chrdev_region(devID, BTMTK_devs);

	return -1;
}

static void BTMTK_exit(void)
{
	dev_t dev = MKDEV(BTMTK_major, 0);
	dev_t devIDfwlog = g_devIDfwlog;

	pr_info("BTMTK_exit\n");

	if (pBTDevfwlog) {
		device_destroy(pBTClass, devIDfwlog);
		pBTDevfwlog = NULL;
	}

	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}

	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	cdev_del(&BTMTK_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("%s driver removed.\n", BT_DRIVER_NAME);
}

static int __init btmtk_sdio_init_module(void)
{
	BTMTK_init();

	if (txbuf == NULL) {
		txbuf = kmalloc(MTK_TXDATA_SIZE, GFP_ATOMIC);
		memset(txbuf, 0, MTK_TXDATA_SIZE);
	}

	if (rxbuf == NULL) {
		rxbuf = kmalloc(MTK_RXDATA_SIZE, GFP_ATOMIC);
		memset(rxbuf, 0, MTK_RXDATA_SIZE);
	}

	if (sdio_register_driver(&bt_mtk_sdio) != 0) {
		pr_notice("SDIO Driver Registration Failed\n");
		return -ENODEV;
	}

	pr_info("SDIO Driver Registration Success\n");

	/* Clear the flag in case user removes the card. */
	user_rmmod = 0;

	return 0;
}

static void __exit btmtk_sdio_exit_module(void)
{
	/* Set the flag as user is removing this module. */
	user_rmmod = 1;

	BTMTK_exit();

	sdio_unregister_driver(&bt_mtk_sdio);

	kfree(txbuf);

	kfree(rxbuf);
}

module_init(btmtk_sdio_init_module);
module_exit(btmtk_sdio_exit_module);
