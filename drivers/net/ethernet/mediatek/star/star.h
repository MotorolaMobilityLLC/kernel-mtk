/* Mediatek STAR MAC network driver.
 *
 * Copyright (c) 2016-2017 Mediatek Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef _STAR_H_
#define _STAR_H_

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/mii.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/regulator/consumer.h>

#include "star_mac.h"
#include "star_phy.h"

/* use  rmii mode */
#define CONFIG_STAR_USE_RMII_MODE

#define ETH_MAX_FRAME_SIZE          1536
#define ETH_SKB_ALIGNMENT           16

#define TX_DESC_NUM  32
#define RX_DESC_NUM  32
#define TX_DESC_TOTAL_SIZE (sizeof(tx_desc) * TX_DESC_NUM)
#define RX_DESC_TOTAL_SIZE (sizeof(rx_desc) * RX_DESC_NUM)
#define ETH_EXTRA_PKT_LEN 36
#define ETH_ADDR_LEN 6
#define STAR_NAPI_WEIGHT (RX_DESC_NUM << 1)

/* Star Ethernet Configuration*/
/* ====================================== */
#define star_intr_disable(dev) \
		star_set_reg(star_int_mask((dev)->base), 0x1ff)
#define star_intr_enable(dev) \
		star_set_reg(star_int_mask((dev)->base), 0)
#define star_intr_clear(dev, intrStatus) \
		star_set_reg(star_int_sta((dev)->base), intrStatus)
#define star_intr_status(dev) \
		star_get_reg(star_int_sta((dev)->base))
#define star_intr_rx_enable(dev) \
		star_clear_bit(star_int_mask((dev)->base), STAR_INT_STA_RXC)
#define star_intr_rx_disable(dev) \
		star_set_bit(star_int_mask((dev)->base), STAR_INT_STA_RXC)
#define star_intr_mask(dev) \
		star_get_reg(star_int_mask((dev)->base))

#define RX_RESUME BIT(2)
#define RX_STOP BIT(1)
#define RX_START BIT(0)

#define dma_tx_start_and_reset_tx_desc(dev) \
		star_set_bit(star_tx_dma_ctrl((dev)->base), TX_START)
#define dma_rx_start_and_reset_rx_desc(dev) \
		star_set_bit(star_rx_dma_ctrl((dev)->base), RX_START)
#define star_dma_tx_enable(dev) \
		star_set_bit(star_tx_dma_ctrl((dev)->base), TX_RESUME)
#define star_dma_tx_disable(dev) \
		star_set_bit(star_tx_dma_ctrl((dev)->base), TX_STOP)
#define star_dma_rx_enable(dev) \
		star_set_bit(star_rx_dma_ctrl((dev)->base), RX_RESUME)
#define star_dma_rx_disable(dev) \
		star_set_bit(star_rx_dma_ctrl((dev)->base), RX_STOP)

#define star_dma_tx_resume(dev) star_dma_tx_enable(dev)
#define star_dma_rx_resume(dev) star_dma_rx_enable(dev)

#define star_reset_hash_table(dev) \
		star_set_bit(star_test1((dev)->base), STAR_TEST1_RST_HASH_BIST)

#define star_dma_rx_valid(ctrl_len) \
		(((ctrl_len & RX_FS) != 0) && ((ctrl_len & RX_LS) != 0) && \
		((ctrl_len & RX_CRCERR) == 0) && ((ctrl_len & RX_OSIZE) == 0))

#define star_dma_rx_crc_err(ctrl_len) ((ctrl_len & RX_CRCERR) ? 1 : 0)
#define star_dma_rx_over_size(ctrl_len) ((ctrl_len & RX_OSIZE) ? 1 : 0)

#define star_dma_rx_length(ctrl_len) \
		((ctrl_len >> RX_LEN_OFFSET) & RX_LEN_MASK)
#define star_dma_tx_length(ctrl_len) \
		((ctrl_len >> TX_LEN_OFFSET) & TX_LEN_MASK)

#define star_arl_promisc_enable(dev) \
		star_set_bit(STAR_ARL_CFG((dev)->base), STAR_ARL_CFG_MISCMODE)

/**
 * @brief structure for Star private data
 */
typedef struct star_private_s {
	struct regulator *phy_regulator;
	struct clk *core_clk, *reg_clk, *trans_clk;
	star_dev star_dev;
	struct net_device *dev;
	dma_addr_t desc_dma_addr;
	uintptr_t desc_vir_addr;
	u32 phy_addr;
	/* star lock */
	spinlock_t lock;
	struct tasklet_struct dsr;
	bool tsk_tx;
	struct napi_struct napi;
	struct mii_if_info mii;
	struct input_dev *idev;
	bool opened;
	bool support_wol;
	bool support_rmii;
	int eint_irq;
	int eint_pin;
} star_private;

struct eth_phy_ops {
	u32 addr;
	/* value of phy reg3(identifier2) */
	u32 phy_id;
	void (*init)(star_dev *sdev);
	void (*wol_enable)(struct net_device *netdev);
	void (*wol_disable)(struct net_device *netdev);
};

/* debug level */
enum {
	STAR_ERR = 0,
	STAR_WARN,
	STAR_DBG,
	STAR_VERB,
	STAR_DBG_MAX
};

#ifndef STAR_DBG_LVL_DEFAULT
#define STAR_DBG_LVL_DEFAULT STAR_DBG
#endif

/* star mac memory barrier */
#define star_mb() mb()

#define STAR_MSG(lvl, fmt...) do {\
		if (lvl <= STAR_DBG_LVL_DEFAULT)\
			pr_err("star: " fmt);\
		} while (0)

static inline void star_set_reg(void __iomem *reg, u32 value)
{
	STAR_MSG(STAR_VERB, "star_set_reg(%p)=%08x\n", reg, value);
	iowrite32(value, reg);
}

static inline u32 star_get_reg(void __iomem *reg)
{
	u32 data = ioread32(reg);

	STAR_MSG(STAR_VERB, "star_get_reg(%p)=%08x\n", reg, data);
	return data;
}

static inline void star_set_bit(void __iomem *reg, u32 bit)
{
	u32 data = ioread32(reg);

	data |= bit;
	STAR_MSG(STAR_VERB, "star_set_bit(%p,bit:%08x)=%08x\n", reg, bit, data);
	iowrite32(data, reg);
	star_mb();
}

static inline void star_clear_bit(void __iomem *reg, u32 bit)
{
	u32 data = ioread32(reg);

	data &= ~bit;
	STAR_MSG(STAR_VERB,
		 "star_clear_bit(%p,bit:%08x)=%08x\n", reg, bit, data);
	iowrite32(data, reg);
	star_mb();
}

static inline u32 star_get_bit_mask(void __iomem *reg, u32 mask, u32 offset)
{
	u32 data = ioread32(reg);

	data = ((data >> offset) & mask);
	STAR_MSG(STAR_VERB,
		 "star_get_bit_mask(%p,mask:%08x,offset:%08x)=%08x(data)\n",
		 reg, mask, offset, data);
	return data;
}

static inline u32 star_is_set_bit(void __iomem *reg, u32 bit)
{
	u32 data = ioread32(reg);

	data &= bit;
	STAR_MSG(STAR_VERB,
		 "star_is_set_bit(%p,bit:%08x)=%08x\n", reg, bit, data);
	return data ? 1 : 0;
}

int star_get_wol_flag(star_private *star_prv);
void star_set_wol_flag(star_private *star_prv, bool flag);

#endif /* _STAR_H_ */
