/*
 * Mediatek BT-over-SDIO driver: SDIO interface related definitions
 *
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
 *
 */

#ifndef _BTMTK_SDIO_H_
#define _BTMTK_SDIO_H_

#define VERSION "v0.0.0.30"

#define SDIO_HEADER_LEN                 4

#define DUMP_HCI_LOG_FILE_NAME          "/sys/hcilog"
/* SD block size can not bigger than 64 due to buf size limit in firmware */
/* define SD block size for data Tx/Rx */
#define SDIO_BLOCK_SIZE                 256

#define SDIO_PATCH_DOWNLOAD_FIRST    1/*first packet*/
#define SDIO_PATCH_DOWNLOAD_CON        2/*continue*/
#define SDIO_PATCH_DOWNLOAD_END        3/*end*/

/* Number of blocks for firmware transfer */
#define FIRMWARE_TRANSFER_NBLOCK        2

/* This is for firmware specific length */
#define FW_EXTRA_LEN                    36

#define MRVDRV_SIZE_OF_CMD_BUFFER       (2 * 1024)

#define MRVDRV_BT_RX_PACKET_BUFFER_SIZE \
					(HCI_MAX_FRAME_SIZE + FW_EXTRA_LEN)

#define ALLOC_BUF_SIZE  (((max_t (int, MRVDRV_BT_RX_PACKET_BUFFER_SIZE, \
				MRVDRV_SIZE_OF_CMD_BUFFER) + SDIO_HEADER_LEN \
				+ SDIO_BLOCK_SIZE - 1) / SDIO_BLOCK_SIZE) \
				* SDIO_BLOCK_SIZE)

/* The number of times to try when polling for status */
#define MAX_POLL_TRIES                  100

/* Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY           2

/* register bitmasks */
#define HOST_POWER_UP                           BIT(1)
#define HOST_CMD53_FIN                          BIT(2)

#define HIM_DISABLE                             0xff
#define HIM_ENABLE                              (BIT(0) | BIT(1))

#define UP_LD_HOST_INT_STATUS                   BIT(0)
#define DN_LD_HOST_INT_STATUS                   BIT(1)

#define DN_LD_CARD_RDY                          BIT(0)
#define CARD_IO_READY                           BIT(3)

#define FIRMWARE_READY                          0xfedc
#define CFG_THREE_IN_ONE_FIRMWARE               0


#if CFG_THREE_IN_ONE_FIRMWARE
#define BUILD_SIGN(ch0, ch1, ch2, ch3) \
		((unsigned int)(unsigned char)(ch0) | \
		((unsigned int)(unsigned char)(ch1) << 8) | \
		((unsigned int)(unsigned char)(ch2) << 16) | \
		((unsigned int)(unsigned char)(ch3) << 24))

#define MTK_WIFI_SIGNATURE BUILD_SIGN('M', 'T', 'K', 'W')
#define FW_TYPE_PATCH   2
struct _FW_SECTION_T {
	unsigned int u4FwType;
	unsigned int u4Offset;
	unsigned int u4Length;
};

struct _FIRMWARE_HEADER_T {
	unsigned int u4Signature;
	unsigned int u4CRC;
	unsigned int u4NumOfEntries;
	unsigned int u4Reserved;
	struct _FW_SECTION_T arSection[];
};
#endif

struct btmtk_sdio_card_reg {
	u8 cfg;
	u8 host_int_mask;
	u8 host_intstatus;
	u8 card_status;
	u8 sq_read_base_addr_a0;
	u8 sq_read_base_addr_a1;
	u8 card_revision;
	u8 card_fw_status0;
	u8 card_fw_status1;
	u8 card_rx_len;
	u8 card_rx_unit;
	u8 io_port_0;
	u8 io_port_1;
	u8 io_port_2;
	bool int_read_to_clear;
	u8 host_int_rsr;
	u8 card_misc_cfg;
	u8 fw_dump_ctrl;
	u8 fw_dump_start;
	u8 fw_dump_end;
	u8 func_num;
};

struct btmtk_sdio_card {
	struct sdio_func *func;
	u32 ioport;
	const char *helper;
	const char *firmware;
	const char *firmware1;
	const struct btmtk_sdio_card_reg *reg;
	bool support_pscan_win_report;
	bool supports_fw_dump;
	u16 sd_blksz_fw_dl;
	u8 rx_unit;
	struct btmtk_private *priv;
};
struct btmtk_sdio_device {
	const char *helper;
	const char *firmware;
	const char *firmware1;
	const struct btmtk_sdio_card_reg *reg;
	const bool support_pscan_win_report;
	u16 sd_blksz_fw_dl;
	bool supports_fw_dump;
};
#pragma pack(1)
struct _PATCH_HEADER {
	u8 ucDateTime[16];
	u8 ucPlatform[4];
	u16 u2HwVer;
	u16 u2SwVer;
	u32 u4PatchVer;
	u16 u2PatchStartAddr;/*Patch ram start address*/
};
#pragma pack()

#define HW_VERSION 0x80000000
#define FW_VERSION 0x80000004

/*common register address*/
#define CHLPCR 0x0004
#define CSDIOCSR 0x0008
#define CHCR   0x000C
#define CHISR  0x0010
#define CHIER  0x0014
#define CTDR   0x0018
#define CRDR   0x001C

/*CHLPCR*/
#define C_FW_INT_EN_SET            0x00000001
#define C_FW_INT_EN_CLEAR        0x00000002
/*CHISR*/
#define RX_PKT_LEN             0xFFFF0000
#define FIRMWARE_INT             0x0000FE00
#define TX_FIFO_OVERFLOW         0x00000100
#define FW_INT_IND_INDICATOR        0x00000080
#define TX_COMPLETE_COUNT         0x00000070
#define TX_UNDER_THOLD             0x00000008
#define TX_EMPTY             0x00000004
#define RX_DONE                 0x00000002
#define FW_OWN_BACK_INT             0x00000001


#define MTKSTP_HEADER_SIZE 0x0004

#define MTK_SDIO_PACKET_HEADER_SIZE 4
#define MTKDATA_HEADER_SIZE 10
#define MTKWMT_HEADER_SIZE 4

#define PATCH_DOWNLOAD_SIZE 1970

#define DRIVER_OWN 0
#define FW_OWN 1

#define MTK_WMT_HEADER_LEN 4

#define DEFAULE_PATCH_FRAG_SIZE    1000

#define PATCH_IS_DOWNLOAD_BT_OTHER 0
#define PATCH_READY 1
#define PATCH_NEED_DOWNLOAD 2

/**
 * stpbtfwlog device node
 */
#define HCI_MAX_COMMAND_SIZE		255
/* Write a char to buffer.
 * ex : echo 01 be > /dev/stpbtfwlog
 * "01 " need three bytes.
 */
#define HCI_MAX_COMMAND_BUF_SIZE	(HCI_MAX_COMMAND_SIZE * 3)

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

/* Platform specific DMA alignment */
#define BTSDIO_DMA_ALIGN                8

/* Macros for Data Alignment : size */
#define ALIGN_SZ(p, a)  \
		(((p) + ((a) - 1)) & ~((a) - 1))

/* Macros for Data Alignment : address */
#define ALIGN_ADDR(p, a)        \
		((((unsigned long)(p)) + (((unsigned long)(a)) - 1)) & \
		~(((unsigned long)(a)) - 1))
struct sk_buff *btmtk_create_send_data(struct sk_buff *skb);
int btmtk_print_buffer_conent(u8 *buf, u32 Datalen);
u32 lock_unsleepable_lock(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL);
u32 unlock_unsleepable_lock(struct _OSAL_UNSLEEPABLE_LOCK_ *pUSL);

extern unsigned char probe_counter;
extern unsigned char *txbuf;
extern u8 probe_ready;

#endif

