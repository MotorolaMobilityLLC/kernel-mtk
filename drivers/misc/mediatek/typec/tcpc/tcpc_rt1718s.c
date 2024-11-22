// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Richtek Inc.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched/clock.h>
#include <linux/regmap.h>
#include <linux/suspend.h>

#include "inc/tcpci.h"
#include "inc/tcpci_typec.h"
#include "inc/std_tcpci_v10.h"

#define RT1718S_DRV_VERSION	"1.0.1_G"

#define RT1718S_INFO_EN			1
#define RT1718S_DBGINFO_EN		0

#define RT1718S_INFO(fmt, ...) \
	do { \
		if (RT1718S_INFO_EN) \
			pd_dbg_info("%s " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define RT1718S_DBGINFO(fmt, ...) \
	do { \
		if (RT1718S_DBGINFO_EN) \
			pd_dbg_info("%s " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define RT1718S_VID			0x29CF
#define RT1718S_PID			0x1718

#define RT1718S_PHY_CTRL1		(0x80)
#define RT1718S_PHY_CTRL4		(0x83)
#define RT1718S_PHY_CTRL5		(0x84)
#define RT1718S_PHY_CTRL7		(0x86)
#define RT1718S_PHY_CTRL8		(0x89)
#define RT1718S_VCONN_CTRL2		(0x8B)
#define RT1718S_VCONN_CTRL3		(0x8C)
#define RT1718S_SYS_CTRL1		(0x8F)
#define RT1718S_SYS_CTRL2		(0x90)
#define RT1718S_RT_MASK1		(0x91)
#define RT1718S_RT_MASK5		(0x95)
#define RT1718S_RT_INT1			(0x98)
#define RT1718S_RT_ST1			(0x9F)
#define RT1718S_RT_ST2			(0xA0)
#define RT1718S_RT_ST3			(0xA1)
#define RT1718S_RT_ST4			(0xA2)
#define RT1718S_RT_ST5			(0xA3)
#define RT1718S_SYS_CTRL3		(0xB0)
#define RT1718S_TCPCCTRL1		(0xB1)
#define RT1718S_TCPCCTRL2		(0xB2)
#define RT1718S_TCPCCTRL3		(0xB3)
#define RT1718S_TCPCCTRL4		(0xB4)
#define RT1718S_LPWRCTRL3		(0xBB)
#define RT1718S_LPWRCTRL5		(0xBD)
#define RT1718S_WATCHDOGCTRL		(0xBE)
#define RT1718S_I2CTORSTCTRL		(0xBF)
#define RT1718S_HILOCTRL9		(0xC8)
#define RT1718S_HILOCTRL10		(0xC9)
#define RT1718S_SHIELDCTRL1		(0xCA)
#define RT1718S_FOD_CTRL		(0xCF)
#define RT1718S_VBUSPATH_CTRL		(0xDD)
#define RT1718S_DIS_SNK_VBUS_CTRL	(0xDE)
#define RT1718S_ENA_SNK_VBUS_CTRL	(0xDF)
#define RT1718S_DIS_SRC_VBUS_CTRL	(0xE0)
#define RT1718S_ENA_SRC_VBUS_CTRL	(0xE1)
#define RT1718S_GPIO2_CTRL		(0xEE)

#define RT1718S_VBUSVOLCTRL		(0xF213)
#define RT1718S_SBU_CTRL_01		(0xF23A)
#define RT1718S_NEW_RXBMCCTRL1		(0xF240)

/* RT1718S_PHY_CTRL8: 0x89 */
#define RT1718S_M_PRLRSTB		BIT(1)
/* RT1718S_SYS_CTRL2: 0x90 */
#define RT1718S_M_SAFE0VDET_EN		BIT(7)
#define RT1718S_M_LPWR_EN		BIT(3)
#define RT1718S_M_VBUSDET_EN		BIT(2)
#define RT1718S_M_BMCIOOSC_EN		BIT(0)
/* RT1718S_RT_MASK1: 0x91 */
/* RT1718S_RT_ST1: 0x9F */
#define RT1718S_M_VBUS_VALID		BIT(5)
#define RT1718S_M_VBUS_SAFE0V		BIT(1)
#define RT1718S_M_WAKEUP		BIT(0)
/* RT1718S_RT_MASK2: 0x92 */
/* RT1718S_RT_ST2: 0xA0 */
#define RT1718S_M_VCONN_SHTGND		BIT(5)
#define RT1718S_M_VCONN_UVP		BIT(4)
#define RT1718S_M_VCONN_RVP		BIT(2)
#define RT1718S_M_VCONN_OVCC2		BIT(1)
#define RT1718S_M_VCONN_OVCC1		BIT(0)
#define RT1718S_M_VCONN_FAULT \
	(RT1718S_M_VCONN_SHTGND | RT1718S_M_VCONN_UVP | RT1718S_M_VCONN_RVP | \
	 RT1718S_M_VCONN_OVCC2 | RT1718S_M_VCONN_OVCC1)
/* RT1718S_RT_MASK3: 0x93 */
#define RT1718S_M_CTD			BIT(4)
/* RT1718S_RT_ST3: 0xA1 */
#define RT1718S_M_CABLE_TYPEA		BIT(5)
#define RT1718S_M_CABLE_TYPEC		BIT(4)
/* RT1718S_RT_MASK4: 0x94 */
/* RT1718S_RT_ST4: 0xA2 */
#define RT1718S_M_FOD_DISCHGF		BIT(7)
#define RT1718S_M_FOD_HR		BIT(6)
#define RT1718S_M_FOD_LR		BIT(5)
#define RT1718S_M_FOD_OV		BIT(1)
#define RT1718S_M_FOD_DONE		BIT(0)
#define RT1718S_M_FOD_ALL \
	(RT1718S_M_FOD_DISCHGF | RT1718S_M_FOD_HR | RT1718S_M_FOD_LR | \
	 RT1718S_M_FOD_OV | RT1718S_M_FOD_DONE)
/* RT1718S_RT_MASK5: 0x95 */
/* RT1718S_RT_ST5: 0xA3 */
#define RT1718S_S_HIDET_CC1		(4)
#define RT1718S_M_HIDET_CC2		BIT(5)
#define RT1718S_M_HIDET_CC1		BIT(4)
#define RT1718S_M_HIDET_CC		(RT1718S_M_HIDET_CC2 | \
					 RT1718S_M_HIDET_CC1)

/* RT1718S_SYS_CTRL3: 0xB0 */
#define RT1718S_SWRESET_MASK		BIT(0)
/* RT1718S_HILOCTRL10: 0xC9 */
#define RT1718S_M_HIDET_CC2_CMPEN	BIT(4)
#define RT1718S_M_HIDET_CC1_CMPEN	BIT(1)
#define RT1718S_M_HIDET_CC_CMPEN \
	(RT1718S_M_HIDET_CC2_CMPEN | RT1718S_M_HIDET_CC1_CMPEN)
/* RT1718S_SHIELDCTRL1: 0xCA */
#define RT1718S_M_RPDET_AUTO		BIT(7)
#define RT1718S_M_CTD_EN		BIT(1)
/* RT1718S_FOD_CTRL: 0xCF */
#define RT1718S_M_FOD_FW_EN		BIT(7)
#define RT1718S_M_FOD_SNK_EN		BIT(6)

struct rt1718s_chip {
	struct device *dev;
	struct regmap *regmap;
	struct tcpc_desc *tcpc_desc;
	struct tcpc_device *tcpc;

	int irq;

	struct delayed_work fod_polling_dwork;
};

enum RT1718S_vend_int {
	RT1718S_VEND_INT1 = 0,
	RT1718S_VEND_INT2,
	RT1718S_VEND_INT3,
	RT1718S_VEND_INT4,
	RT1718S_VEND_INT5,
	RT1718S_VEND_INT6,
	RT1718S_VEND_INT7,
	RT1718S_VEND_INT_MAX,
};

#define RT1718S_P1PREFIX		(0x00)
#define RT1718S_P1START			((RT1718S_P1PREFIX << 8) + 0x00)
#define RT1718S_P1END			((RT1718S_P1PREFIX << 8) + 0xFF)
#define RT1718S_P2PREFIX		(0xF2)
#define RT1718S_P2START			((RT1718S_P2PREFIX << 8) + 0x00)
#define RT1718S_P2END			((RT1718S_P2PREFIX << 8) + 0xFF)

static bool rt1718s_is_accessible_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RT1718S_P1START ... RT1718S_P1END:
	/* Fall Through */
	case RT1718S_P2START ... RT1718S_P2END:
		return true;
	}

	return false;
}

static const struct regmap_config rt1718s_regmap_config = {
	.reg_bits		= 16,
	.val_bits		= 8,

	.reg_format_endian	= REGMAP_ENDIAN_BIG,

	/* page 1(TCPC) : 0x00 ~ 0xff, page 2 : 0xf200 ~0xf2ff */
	.max_register		= RT1718S_P2END,
	.writeable_reg		= rt1718s_is_accessible_reg,
	.readable_reg		= rt1718s_is_accessible_reg,
};

static int rt1718s_regmap_read(void *context, const void *reg, size_t reg_size,
			       void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct rt1718s_chip *chip = i2c_get_clientdata(i2c);
	struct tcpc_device *tcpc = chip->tcpc;
	struct i2c_msg xfer[2];
	int ret = 0;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	if (*(u8 *)reg == RT1718S_P1PREFIX) {
		xfer[0].len = 1,
		xfer[0].buf = (u8 *)(reg + 1);
	} else {
		xfer[0].len = reg_size;
		xfer[0].buf = (u8 *)reg;
	}

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = val_size;
	xfer[1].buf = val;

	atomic_inc(&tcpc->suspend_pending);
	ret = wait_event_timeout(tcpc->resume_wait_que,
				 !atomic_read(&tcpc->is_suspended),
				 msecs_to_jiffies(1000));
	if (!ret) {
		ret = -ETIMEOUT;
		goto out;
	}
	ret = i2c_transfer(i2c->adapter, xfer, 2);
out:
	atomic_dec_if_positive(&tcpc->suspend_pending);
	if (ret < 0)
		return ret;
	else if (ret != 2)
		return -EIO;
	return 0;
}

static int rt1718s_regmap_write(void *context, const void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct rt1718s_chip *chip = i2c_get_clientdata(i2c);
	struct tcpc_device *tcpc = chip->tcpc;
	struct i2c_msg xfer;
	int ret = 0;

	xfer.addr = i2c->addr;
	xfer.flags = 0;
	if (*(u8 *)val == RT1718S_P1PREFIX) {
		xfer.len = val_size - 1;
		xfer.buf = (u8 *)(val + 1);
	} else {
		xfer.len = val_size;
		xfer.buf = (u8 *)val;
	}

	atomic_inc(&tcpc->suspend_pending);
	ret = wait_event_timeout(tcpc->resume_wait_que,
				 !atomic_read(&tcpc->is_suspended),
				 msecs_to_jiffies(1000));
	if (!ret) {
		ret = -ETIMEOUT;
		goto out;
	}
	ret = i2c_transfer(i2c->adapter, &xfer, 1);
out:
	atomic_dec_if_positive(&tcpc->suspend_pending);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return 0;
}

static const struct regmap_bus rt1718s_regmap_bus = {
	.read	= rt1718s_regmap_read,
	.write	= rt1718s_regmap_write,
};

static inline int rt1718s_write8(struct rt1718s_chip *chip, u16 reg, u8 data)
{
	return regmap_write(chip->regmap, reg, data);
}

static inline int rt1718s_read8(struct rt1718s_chip *chip, u16 reg, u8 *data)
{
	unsigned int _data = 0;
	int ret = 0;

	ret = regmap_read(chip->regmap, reg, &_data);
	if (ret < 0)
		return ret;
	*data = _data;
	return 0;
}

static inline int rt1718s_write16(struct rt1718s_chip *chip, u16 reg, u16 data)
{
	data = cpu_to_le16(data);
	return regmap_bulk_write(chip->regmap, reg, &data, 2);
}

static inline int rt1718s_read16(struct rt1718s_chip *chip, u16 reg, u16 *data)
{
	int ret = 0;

	ret = regmap_bulk_read(chip->regmap, reg, data, 2);
	if (ret < 0)
		return ret;
	*data = le16_to_cpu(*data);
	return 0;
}

static inline int rt1718s_bulk_write(struct rt1718s_chip *chip,
				     u16 reg, const void *data,
				     size_t count)
{
	return regmap_bulk_write(chip->regmap, reg, data, count);
}

static inline int rt1718s_bulk_read(struct rt1718s_chip *chip, u16 reg,
				    void *data, size_t count)
{
	return regmap_bulk_read(chip->regmap, reg, data, count);
}

static inline int rt1718s_update_bits(struct rt1718s_chip *chip, u16 reg,
				      u8 mask, u8 data)
{
	return regmap_update_bits(chip->regmap, reg, mask, data);
}

static inline int rt1718s_set_bits(struct rt1718s_chip *chip, u16 reg, u8 mask)
{
	return regmap_set_bits(chip->regmap, reg, mask);
}

static inline int rt1718s_clr_bits(struct rt1718s_chip *chip, u16 reg, u8 mask)
{
	return regmap_clear_bits(chip->regmap, reg, mask);
}

static const struct reg_sequence rt1718s_init_settings[] = {
	/* Config I2C timeout reset enable, and timeout to 200ms */
	{ RT1718S_I2CTORSTCTRL, 0x8F, 0 },
	/* tTCPCFilter = 500us */
	{ RT1718S_TCPCCTRL1, 0x14, 0 },
	/* DRP Toggle Cycle : 51.2ms (51.2 + 6.4*val ms) */
	{ RT1718S_TCPCCTRL2, 0x00, 0 },
	/* DRP Duty Ctrl : dcSRC: 308/1024 ((val+1)/1024) */
	{ RT1718S_TCPCCTRL3, 0x33, 0 },
	{ RT1718S_TCPCCTRL4, 0x01, 0 },
	/* Set Low Power LDO to 2V */
	{ RT1718S_LPWRCTRL3, 0xD0, 0 },
	/* Enable auto dischg timer for IQ about 12mA consumption */
	{ RT1718S_WATCHDOGCTRL, 0xEB, 0 },
	/* VBUS_VALID debounce time: 375us */
	{ RT1718S_LPWRCTRL5, 0x2F, 0 },
	/* VBUS_VALID threshold: 3.6V */
	{ RT1718S_HILOCTRL9, 0xAA, 0 },

	/* Improve for Lecroy Mx4 signal noise */
	{ RT1718S_NEW_RXBMCCTRL1, 0x12, 0 },

	/* set vbus ovp as 20V and ratio as 1.2 */
	{ RT1718S_VBUSVOLCTRL, 0x3F, 0 },

	/* Analog filetered */
	{ RT1718S_PHY_CTRL1, 0x34, 0 },
	/* CRCtimer = 1ms for typical spec */
	{ RT1718S_PHY_CTRL4, 0x60, 0 },
	{ RT1718S_PHY_CTRL5, 0xE9, 0 },
	/* BMC decoder idle time = 9.99us */
	{ RT1718S_PHY_CTRL7, 0x1E, 0 },
	/* IRQB 3M path, Set shipping mode off, Connect invalid is off */
	{ RT1718S_SYS_CTRL1, 0xB8, 1000 },

	/*
	 * GPB = floating
	 * GPA = low = source path off
	 */
	{ RT1718S_VBUSPATH_CTRL, 0x82, 0 },
	/* Link GPA to TCPC command */
	{ RT1718S_DIS_SNK_VBUS_CTRL, 0x82, 0},
	{ RT1718S_ENA_SNK_VBUS_CTRL, 0x82, 0},
	{ RT1718S_DIS_SRC_VBUS_CTRL, 0x82, 0},
	{ RT1718S_ENA_SRC_VBUS_CTRL, 0x85, 0},

	/* Set GPIO2 push-pull, output low */
	{ RT1718S_GPIO2_CTRL, 0x0C, 0},
};

static int rt1718s_sw_reset(struct rt1718s_chip *chip)
{
	int ret = 0;

	ret = rt1718s_write8(chip, RT1718S_SYS_CTRL3, RT1718S_SWRESET_MASK);
	if (ret < 0)
		return ret;
	/* Wait for IC to reset done*/
	usleep_range(1000, 2000);
	return 0;
}

static inline int rt1718s_init_vend_mask(struct rt1718s_chip *chip)
{
	struct tcpc_device *tcpc = chip->tcpc;
	u8 mask[RT1718S_VEND_INT_MAX] = {0};

	mask[RT1718S_VEND_INT1] |= RT1718S_M_WAKEUP |
				   RT1718S_M_VBUS_SAFE0V |
				   RT1718S_M_VBUS_VALID;

	if (tcpc->tcpc_flags & TCPC_FLAGS_FOREIGN_OBJECT_DETECTION)
		mask[RT1718S_VEND_INT4] |= RT1718S_M_FOD_DONE |
					   RT1718S_M_FOD_OV |
					   RT1718S_M_FOD_DISCHGF;

	if (tcpc->tcpc_flags & TCPC_FLAGS_CABLE_TYPE_DETECTION)
		mask[RT1718S_VEND_INT3] |= RT1718S_M_CTD;

	return rt1718s_bulk_write(chip, RT1718S_RT_MASK1, mask, sizeof(mask));
}

static inline int rt1718s_init_alert_mask(struct rt1718s_chip *chip)
{
	u16 mask = TCPC_V10_REG_ALERT_VENDOR_DEFINED |
		   TCPC_V10_REG_ALERT_VBUS_SINK_DISCONNECT |
		   TCPC_V10_REG_ALERT_FAULT | TCPC_V10_REG_ALERT_CC_STATUS;
	u8 masks[4] = {0x00, 0x00, 0x00,
		       TCPC_V20_REG_FAULT_STATUS_RESET_TO_DEFAULT |
		       TCPC_V10_REG_FAULT_STATUS_VCONN_OC};

#if CONFIG_USB_POWER_DELIVERY
	mask |= TCPC_V10_REG_ALERT_TX_SUCCESS |
		TCPC_V10_REG_ALERT_TX_DISCARDED |
		TCPC_V10_REG_ALERT_TX_FAILED |
		TCPC_V10_REG_ALERT_RX_HARD_RST |
		TCPC_V10_REG_ALERT_RX_STATUS |
		TCPC_V10_REG_ALERT_RX_OVERFLOW;
#endif /* CONFIG_USB_POWER_DELIVERY */
	*(u16 *)masks = cpu_to_le16(mask);
	return rt1718s_bulk_write(chip, TCPC_V10_REG_ALERT_MASK,
				  masks, sizeof(masks));
}

static int rt1718s_init_mask(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	rt1718s_init_vend_mask(chip);
	rt1718s_init_alert_mask(chip);

	return 0;
}

static int rt1718s_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	int ret = 0;

	RT1718S_DBGINFO("\n");

	if (sw_reset) {
		ret = rt1718s_sw_reset(chip);
		if (ret < 0)
			return ret;
	}

	rt1718s_init_mask(tcpc);
	return regmap_register_patch(chip->regmap, rt1718s_init_settings,
				     ARRAY_SIZE(rt1718s_init_settings));
}

static int rt1718s_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u16 std_mask = mask & 0xffff;

	return std_mask ?
	       rt1718s_write16(chip, TCPC_V10_REG_ALERT, std_mask) : 0;
}

static int rt1718s_fault_status_clear(struct tcpc_device *tcpc, u8 status)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	if (status & TCPC_V20_REG_FAULT_STATUS_RESET_TO_DEFAULT)
		rt1718s_tcpc_init(chip->tcpc, false);
	return rt1718s_write8(chip, TCPC_V10_REG_FAULT_STATUS, status);
}

static int rt1718s_get_alert_mask(struct tcpc_device *tcpc, u32 *mask)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u16 data = 0;
	int ret = 0;

	ret = rt1718s_read16(chip, TCPC_V10_REG_ALERT_MASK, &data);
	if (ret < 0)
		return ret;
	*mask = data;
	return 0;
}

static int rt1718s_set_alert_mask(struct tcpc_device *tcpc, u32 mask)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	RT1718S_DBGINFO("mask = 0x%04x\n", mask);
	return rt1718s_write16(chip, TCPC_V10_REG_ALERT_MASK, mask);
}

static int rt1718s_get_alert_status_and_mask(struct tcpc_device *tcpc,
					     u32 *alert, u32 *mask)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 buf[4] = {0};
	int ret = 0;

	ret = rt1718s_bulk_read(chip, TCPC_V10_REG_ALERT, buf, 4);
	if (ret < 0)
		return ret;
	*alert = le16_to_cpu(*(u16 *)&buf[0]);
	*mask = le16_to_cpu(*(u16 *)&buf[2]);
	return 0;
}

static int rt1718s_vbus_change_helper(struct rt1718s_chip *chip)
{
	struct tcpc_device *tcpc = chip->tcpc;
	u8 data = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_RT_ST1, &data);
	if (ret < 0)
		return ret;
	tcpc->vbus_present = !!(data & RT1718S_M_VBUS_VALID);
	/*
	 * Vsafe0v only triggers when vbus falls under 0.8V,
	 * also update parameter if vbus present triggers
	 */
	tcpc->vbus_safe0v = !!(data & RT1718S_M_VBUS_SAFE0V);
	return 0;
}

static int rt1718s_get_power_status(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return rt1718s_vbus_change_helper(chip);
}

static int rt1718s_get_fault_status(struct tcpc_device *tcpc, u8 *status)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return rt1718s_read8(chip, TCPC_V10_REG_FAULT_STATUS, status);
}

static int rt1718s_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	bool act_as_sink = false;
	u8 buf[4], status = 0, role_ctrl = 0, cc_role = 0;
	int ret = 0;

	ret = rt1718s_bulk_read(chip, TCPC_V10_REG_ROLE_CTRL, buf, sizeof(buf));
	if (ret < 0)
		return ret;
	role_ctrl = buf[0];
	status = buf[3];

	if (status & TCPC_V10_REG_CC_STATUS_DRP_TOGGLING) {
		if (role_ctrl & TCPC_V10_REG_ROLE_CTRL_DRP) {
			*cc1 = TYPEC_CC_DRP_TOGGLING;
			*cc2 = TYPEC_CC_DRP_TOGGLING;
			return 0;
		}
		/* Toggle reg0x1A[6] DRP = 1 and = 0 */
		rt1718s_write8(chip, TCPC_V10_REG_ROLE_CTRL,
			       role_ctrl | TCPC_V10_REG_ROLE_CTRL_DRP);
		rt1718s_write8(chip, TCPC_V10_REG_ROLE_CTRL, role_ctrl);
		return -EAGAIN;
	}
	*cc1 = TCPC_V10_REG_CC_STATUS_CC1(status);
	*cc2 = TCPC_V10_REG_CC_STATUS_CC2(status);

	if (role_ctrl & TCPC_V10_REG_ROLE_CTRL_DRP)
		act_as_sink = TCPC_V10_REG_CC_STATUS_DRP_RESULT(status);
	else {
		if (tcpc->typec_polarity)
			cc_role = TCPC_V10_REG_CC_STATUS_CC2(role_ctrl);
		else
			cc_role = TCPC_V10_REG_CC_STATUS_CC1(role_ctrl);
		act_as_sink = (cc_role != TYPEC_CC_RP);
	}

	/*
	 * If status is not open, then OR in termination to convert to
	 * enum tcpc_cc_voltage_status.
	 */
	if (*cc1 != TYPEC_CC_VOLT_OPEN)
		*cc1 |= (act_as_sink << 2);
	if (*cc2 != TYPEC_CC_VOLT_OPEN)
		*cc2 |= (act_as_sink << 2);
	return 0;
}

static inline int rt1718s_enable_vsafe0v_detect(struct rt1718s_chip *chip,
						bool en)
{
	return (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, RT1718S_RT_MASK1, RT1718S_M_VBUS_SAFE0V);
}

static int rt1718s_set_cc(struct tcpc_device *tcpc, int pull)
{
	int ret = 0;
	u8 data = 0;
	int rp_lvl = TYPEC_CC_PULL_GET_RP_LVL(pull), pull1 = 0, pull2 = 0;
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	RT1718S_INFO("%d\n", pull);
	pull = TYPEC_CC_PULL_GET_RES(pull);
	if (pull == TYPEC_CC_DRP) {
		data = TCPC_V10_REG_ROLE_CTRL_RES_SET(1, rp_lvl, TYPEC_CC_RD,
						      TYPEC_CC_RD);
		ret = rt1718s_write8(chip, TCPC_V10_REG_ROLE_CTRL, data);
		if (ret < 0)
			return ret;
		udelay(32);
		ret = rt1718s_write8(chip, TCPC_V10_REG_COMMAND,
				     TCPM_CMD_LOOK_CONNECTION);
	} else {
		pull2 = pull1 = pull;

		if (pull == TYPEC_CC_RP &&
		    tcpc->typec_state == typec_attached_src) {
			if (tcpc->typec_polarity)
				pull1 = TYPEC_CC_RD;
			else
				pull2 = TYPEC_CC_RD;
		}
		data = TCPC_V10_REG_ROLE_CTRL_RES_SET(0, rp_lvl, pull1, pull2);
		ret = rt1718s_write8(chip, TCPC_V10_REG_ROLE_CTRL, data);
	}
	return ret;
}

static int rt1718s_set_polarity(struct tcpc_device *tcpc, int polarity)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return (polarity ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, TCPC_V10_REG_TCPC_CTRL,
		 TCPC_V10_REG_TCPC_CTRL_PLUG_ORIENT);
}

static inline int rt1718s_is_vconn_fault(struct rt1718s_chip *chip, bool *fault)
{
	u8 status = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_RT_ST2, &status);
	if (ret < 0)
		return ret;
	*fault = !!(status & RT1718S_M_VCONN_FAULT);
	return 0;
}

static int rt1718s_set_vconn(struct tcpc_device *tcpc, int en)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	bool fault = false;
	u8 data[2] = {0xEA, 0x41};
	int ret = 0;

	/*
	 * Set Vconn OVP RVP
	 * Otherwise vconn present fail will be triggered
	 */
	if (en) {
		ret = rt1718s_bulk_write(chip, RT1718S_VCONN_CTRL2,
					 data, sizeof(data));
		if (ret < 0)
			return ret;
		usleep_range(20, 50);
		ret = rt1718s_is_vconn_fault(chip, &fault);
		if (ret >= 0 && fault)
			return -EINVAL;
	}
	ret = (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, TCPC_V10_REG_POWER_CTRL, TCPC_V10_REG_POWER_CTRL_VCONN);
	if (ret < 0)
		return ret;
	if (en)
		ret = rt1718s_write8(chip, RT1718S_VCONN_CTRL3, 0x40);
	else
		ret = rt1718s_write8(chip, RT1718S_VCONN_CTRL2, 0x22);
	return ret;
}

static int rt1718s_tcpc_deinit(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	int cc1 = TYPEC_CC_VOLT_OPEN, cc2 = TYPEC_CC_VOLT_OPEN;

	rt1718s_get_cc(tcpc, &cc1, &cc2);
	if (cc1 != TYPEC_CC_DRP_TOGGLING &&
	    (cc1 != TYPEC_CC_VOLT_OPEN || cc2 != TYPEC_CC_VOLT_OPEN)) {
		rt1718s_set_cc(tcpc, TYPEC_CC_OPEN);
		usleep_range(20000, 30000);
	}
	rt1718s_write8(chip, RT1718S_SYS_CTRL3, RT1718S_SWRESET_MASK);
	atomic_set(&tcpc->is_suspended, true);

	return 0;
}

static int rt1718s_get_fod_status(struct rt1718s_chip *chip,
				  enum tcpc_fod_status *fod)
{
	u8 data = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_RT_ST4, &data);
	if (ret < 0)
		return ret;
	data &= RT1718S_M_FOD_ALL;

	/* LR possesses the highest priority */
	if (data & RT1718S_M_FOD_LR)
		*fod = TCPC_FOD_LR;
	else if (data & RT1718S_M_FOD_HR)
		*fod = TCPC_FOD_HR;
	else if (data & RT1718S_M_FOD_DISCHGF)
		*fod = TCPC_FOD_DISCHG_FAIL;
	else if (data & RT1718S_M_FOD_OV)
		*fod = TCPC_FOD_OV;
	else
		*fod = TCPC_FOD_NONE;
	return 0;
}

static void rt1718s_fod_polling_dwork_handler(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rt1718s_chip *chip = container_of(dwork, struct rt1718s_chip,
						 fod_polling_dwork);

	pm_system_wakeup();

	RT1718S_DBGINFO("Set FOD_FW_EN\n");
	tcpci_lock_typec(chip->tcpc);
	rt1718s_set_bits(chip, RT1718S_FOD_CTRL, RT1718S_M_FOD_FW_EN);
	tcpci_unlock_typec(chip->tcpc);
}

static inline int rt1718s_fod_evt_process(struct rt1718s_chip *chip)
{
	struct tcpc_device *tcpc = chip->tcpc;
	enum tcpc_fod_status fod = TCPC_FOD_NONE;
	int ret = 0;

	ret = rt1718s_get_fod_status(chip, &fod);
	if (ret < 0)
		return ret;
	ret = rt1718s_clr_bits(chip, RT1718S_FOD_CTRL, RT1718S_M_FOD_FW_EN);
	if (ret < 0)
		return ret;
	if (fod == TCPC_FOD_LR)
		mod_delayed_work(system_wq, &chip->fod_polling_dwork,
				 msecs_to_jiffies(5000));
	return tcpc_typec_handle_fod(tcpc, fod);
}

#if CONFIG_CABLE_TYPE_DETECTION
static inline int rt1718s_get_cable_type(struct rt1718s_chip *chip,
					 enum tcpc_cable_type *type)
{
	u8 data = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_RT_ST3, &data);
	if (ret < 0)
		return ret;
	if (data & RT1718S_M_CABLE_TYPEC)
		*type = TCPC_CABLE_TYPE_C2C;
	else if (data & RT1718S_M_CABLE_TYPEA)
		*type = TCPC_CABLE_TYPE_A2C;
	else
		*type = TCPC_CABLE_TYPE_NONE;
	return 0;
}
#endif /* CONFIG_CABLE_TYPE_DETECTION */

static int __rt1718s_get_cc_hi(struct rt1718s_chip *chip)
{
	u8 data = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_RT_ST5, &data);
	if (ret < 0)
		return ret;
	return ((data ^ RT1718S_M_HIDET_CC) & RT1718S_M_HIDET_CC)
		>> RT1718S_S_HIDET_CC1;
}

static inline int rt1718s_hidet_cc_evt_process(struct rt1718s_chip *chip)
{
	int ret = 0;

	ret = __rt1718s_get_cc_hi(chip);
	if (ret < 0)
		return ret;
	return tcpci_notify_cc_hi(chip->tcpc, ret);
}

/*
 * ==================================================================
 * TCPC vendor irq handlers
 * ==================================================================
 */
static int rt1718s_wakeup_irq_handler(struct rt1718s_chip *chip)
{
	return tcpci_alert_wakeup(chip->tcpc);
}

static int rt1718s_fod_irq_handler(struct rt1718s_chip *chip)
{
	return rt1718s_fod_evt_process(chip);
}

static int rt1718s_ctd_irq_handler(struct rt1718s_chip *chip)
{
	int ret = 0;
#if CONFIG_CABLE_TYPE_DETECTION
	enum tcpc_cable_type cable_type = TCPC_CABLE_TYPE_NONE;

	ret = rt1718s_get_cable_type(chip, &cable_type);
	if (ret < 0)
		return ret;
	ret = tcpc_typec_handle_ctd(chip->tcpc, cable_type);
#endif /* CONFIG_CABLE_TYPE_DETECTION */
	return ret;
}

static int rt1718s_hidet_cc_irq_handler(struct rt1718s_chip *chip)
{
	return rt1718s_hidet_cc_evt_process(chip);
}

static int rt1718s_vbus_irq_handler(struct rt1718s_chip *chip)
{
	return rt1718s_vbus_change_helper(chip);
}

struct irq_mapping_tbl {
	u8 num;
	s8 grp;
	int (*hdlr)(struct rt1718s_chip *chip);
};

#define RT1718S_IRQ_MAPPING(_num, _grp, _name) \
	{ .num = _num, .grp = _grp, .hdlr = rt1718s_##_name##_irq_handler }

static struct irq_mapping_tbl rt1718s_vend_irq_mapping_tbl[] = {
	RT1718S_IRQ_MAPPING(0, -1, wakeup),
	RT1718S_IRQ_MAPPING(24, 0, fod),	/* fod_done */
	RT1718S_IRQ_MAPPING(25, 0, fod),	/* fod_ov */
	RT1718S_IRQ_MAPPING(31, 0, fod),	/* fod_dischgf */
	RT1718S_IRQ_MAPPING(20, -1, ctd),
	RT1718S_IRQ_MAPPING(36, 1, hidet_cc),	/* hidet_cc1 */
	RT1718S_IRQ_MAPPING(37, 1, hidet_cc),	/* hidet_cc2 */

	RT1718S_IRQ_MAPPING(1, 2, vbus),	/* vsafe0v */
	RT1718S_IRQ_MAPPING(5, 2, vbus),	/* vbus_valid */
};

static inline int rt1718s_vend_alert_status_clear(struct rt1718s_chip *chip,
						  const u8 *mask)
{
	return rt1718s_bulk_write(chip, RT1718S_RT_INT1, mask,
				  RT1718S_VEND_INT_MAX);
}

static int rt1718s_alert_vendor_defined_handler(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 irqnum = 0, irqbit = 0;
	u8 buf[RT1718S_VEND_INT_MAX * 2];
	u8 *mask = &buf[0];
	u8 *alert = &buf[RT1718S_VEND_INT_MAX];
	s8 grp = 0;
	unsigned long handled_bitmap = 0;
	int ret = 0, i = 0;

	ret = rt1718s_bulk_read(chip, RT1718S_RT_MASK1, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	for (i = 0; i < RT1718S_VEND_INT_MAX; i++) {
		if (!(alert[i] & mask[i]))
			continue;
		RT1718S_INFO("vend_alert[%d]=alert,mask(0x%02X,0x%02X)\n",
			     i + 1, alert[i], mask[i]);
		alert[i] &= mask[i];
	}

	rt1718s_vend_alert_status_clear(chip, alert);

	for (i = 0; i < ARRAY_SIZE(rt1718s_vend_irq_mapping_tbl); i++) {
		irqnum = rt1718s_vend_irq_mapping_tbl[i].num / 8;
		if (irqnum >= RT1718S_VEND_INT_MAX)
			continue;
		irqbit = rt1718s_vend_irq_mapping_tbl[i].num % 8;
		if (alert[irqnum] & BIT(irqbit)) {
			grp = rt1718s_vend_irq_mapping_tbl[i].grp;
			if (grp >= 0) {
				ret = test_and_set_bit(grp, &handled_bitmap);
				if (ret)
					continue;
			}
			rt1718s_vend_irq_mapping_tbl[i].hdlr(chip);
		}
	}
	return 0;
}

static int rt1718s_set_auto_dischg_discnt(struct tcpc_device *tcpc, bool en)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 data = 0;
	int ret = 0;

	RT1718S_DBGINFO("en = %d\n", en);
	if (en) {
		ret = rt1718s_read8(chip, TCPC_V10_REG_POWER_CTRL, &data);
		if (ret < 0)
			return ret;
		data &= ~TCPC_V10_REG_VBUS_MONITOR;
		ret = rt1718s_write8(chip, TCPC_V10_REG_POWER_CTRL, data);
		if (ret < 0)
			return ret;
		data |= TCPC_V10_REG_AUTO_DISCHG_DISCNT;
		return rt1718s_write8(chip, TCPC_V10_REG_POWER_CTRL, data);
	}
	return rt1718s_update_bits(chip, TCPC_V10_REG_POWER_CTRL,
				   TCPC_V10_REG_VBUS_MONITOR |
				   TCPC_V10_REG_AUTO_DISCHG_DISCNT,
				   TCPC_V10_REG_VBUS_MONITOR);
}

static int rt1718s_get_vbus_voltage(struct tcpc_device *tcpc, u32 *vbus)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u16 data = 0;
	int ret = 0;

	ret = rt1718s_read16(chip, TCPC_V10_REG_VBUS_VOLTAGE_L, &data);
	if (ret < 0)
		return ret;
	data = le16_to_cpu(data);
	*vbus = (data & 0x3FF) * 25;
	RT1718S_DBGINFO("0x%04x, %dmV\n", data, *vbus);
	return 0;
}

static int rt1718s_set_low_power_mode(struct tcpc_device *tcpc, bool en,
				      int pull)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 data = 0;
	int ret = 0;

	/* DPDM and SBU12 SWEN control */
	ret = rt1718s_write8(chip, RT1718S_SBU_CTRL_01, en ? 0x00 : 0xCF);
	if (ret < 0)
		return ret;

	ret = rt1718s_enable_vsafe0v_detect(chip, !en);
	if (ret < 0)
		return ret;
	if (en) {
		data = RT1718S_M_LPWR_EN;
#if CONFIG_TYPEC_CAP_NORP_SRC
		data |= RT1718S_M_VBUSDET_EN;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
	} else {
		data = RT1718S_M_SAFE0VDET_EN |
		       RT1718S_M_VBUSDET_EN | RT1718S_M_BMCIOOSC_EN;
	}
	return rt1718s_write8(chip, RT1718S_SYS_CTRL2, data);
}

#if CONFIG_USB_POWER_DELIVERY
static int rt1718s_set_msg_header(struct tcpc_device *tcpc, u8 power_role,
				  u8 data_role)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 msg_hdr = TCPC_V10_REG_MSG_HDR_INFO_SET(data_role, power_role);

	return rt1718s_write8(chip, TCPC_V10_REG_MSG_HDR_INFO, msg_hdr);
}

static int rt1718s_set_rx_enable(struct tcpc_device *tcpc, u8 en)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return rt1718s_write8(chip, TCPC_V10_REG_RX_DETECT, en);
}

static int rt1718s_protocol_reset(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 phy_ctrl8 = 0;
	int ret = 0;

	ret = rt1718s_read8(chip, RT1718S_PHY_CTRL8, &phy_ctrl8);
	if (ret < 0)
		return ret;
	ret = rt1718s_write8(chip, RT1718S_PHY_CTRL8,
			     phy_ctrl8 & ~RT1718S_M_PRLRSTB);
	if (ret < 0)
		return ret;
	udelay(20);
	return rt1718s_write8(chip, RT1718S_PHY_CTRL8,
			      phy_ctrl8 | RT1718S_M_PRLRSTB);
}

static int rt1718s_get_message(struct tcpc_device *tcpc, u32 *payload,
			       u16 *msg_head,
			       enum tcpm_transmit_type *frame_type)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 cnt = 0, buf[32] = {0};
	int ret = 0;

	ret = rt1718s_bulk_read(chip, TCPC_V10_REG_RX_BYTE_CNT,
				buf, sizeof(buf));
	if (ret < 0)
		return ret;

	cnt = buf[0];
	*frame_type = buf[1];
	*msg_head = le16_to_cpu(*(u16 *)&buf[2]);

	RT1718S_DBGINFO("Count is %d\n", cnt);
	RT1718S_DBGINFO("FrameType is %d\n", *frame_type);
	RT1718S_DBGINFO("MessageType is %d\n", PD_HEADER_TYPE(*msg_head));

	/* TCPC 1.0 ==> no need to subtract the size of msg_head */
	if (cnt > 3) {
		cnt -= 3; /* MSG_HDR */
		if (cnt > sizeof(buf) - 4)
			cnt = sizeof(buf) - 4;
		memcpy(payload, buf + 4, cnt);
	}

	return ret;
}

/* message header (2byte) + data object (7*4) */
#define RT1718S_TRANSMIT_MAX_SIZE	(sizeof(u16) + sizeof(u32) * 7)

static int rt1718s_transmit(struct tcpc_device *tcpc,
			    enum tcpm_transmit_type type, u16 header,
			    const u32 *data)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	u8 temp[RT1718S_TRANSMIT_MAX_SIZE + 1];
	u64 t = 0;
	int ret = 0, data_cnt = 0, packet_cnt = 0;

	RT1718S_DBGINFO("++\n");
	t = local_clock();
	if (type < TCPC_TX_HARD_RESET) {
		data_cnt = sizeof(u32) * PD_HEADER_CNT(header);
		packet_cnt = data_cnt + sizeof(u16);

		temp[0] = packet_cnt;
		memcpy(temp + 1, &header, 2);
		if (data_cnt > 0)
			memcpy(temp + 3, data, data_cnt);

		ret = rt1718s_bulk_write(chip, TCPC_V10_REG_TX_BYTE_CNT,
					 temp, packet_cnt + 1);
		if (ret < 0)
			return ret;
	}

	ret = rt1718s_write8(chip, TCPC_V10_REG_TRANSMIT,
			     TCPC_V10_REG_TRANSMIT_SET(tcpc->pd_retry_count,
			     type));
	t = local_clock() - t;
	do_div(t, NSEC_PER_USEC);
	RT1718S_INFO("-- delta = %lluus\n", t);

	return ret;
}

static int rt1718s_set_bist_test_mode(struct tcpc_device *tcpc, bool en)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, TCPC_V10_REG_TCPC_CTRL,
		 TCPC_V10_REG_TCPC_CTRL_BIST_TEST_MODE);
}
#endif	/* CONFIG_USB_POWER_DELIVERY */

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
static int rt1718s_retransmit(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return rt1718s_write8(chip, TCPC_V10_REG_TRANSMIT,
			      TCPC_V10_REG_TRANSMIT_SET(tcpc->pd_retry_count,
			      TCPC_TX_SOP));
}
#endif /* CONFIG_USB_PD_RETRY_CRC_DISCARD */

static int rt1718s_enable_rpdet_auto(struct rt1718s_chip *chip, bool en)
{
	return (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, RT1718S_SHIELDCTRL1, RT1718S_M_RPDET_AUTO);
}

static int rt1718s_set_cc_hidet(struct tcpc_device *tcpc, bool en)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);
	int ret = 0;

	if (en)
		rt1718s_enable_rpdet_auto(chip, false);
	ret = (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, RT1718S_HILOCTRL10, RT1718S_M_HIDET_CC_CMPEN);
	if (ret < 0)
		return ret;
	ret = (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, RT1718S_RT_MASK5, RT1718S_M_HIDET_CC);
	if (ret < 0)
		return ret;
	if (!en)
		rt1718s_enable_rpdet_auto(chip, true);
	return ret;
}

static int rt1718s_get_cc_hi(struct tcpc_device *tcpc)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return __rt1718s_get_cc_hi(chip);
}

#if CONFIG_TYPEC_CAP_FORCE_DISCHARGE
static int rt1718s_set_force_discharge(struct tcpc_device *tcpc,
				       bool en, int mv)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	return (en ? rt1718s_set_bits : rt1718s_clr_bits)
		(chip, TCPC_V10_REG_POWER_CTRL, TCPC_V10_REG_FORCE_DISC_EN);
}
#endif	/* CONFIG_TYPEC_CAP_FORCE_DISCHARGE */

static void rt1718s_set_command(struct tcpc_device *tcpc, u8 cmd)
{
	struct rt1718s_chip *chip = tcpc_get_dev_data(tcpc);

	switch (cmd) {
	case TCPM_CMD_ENABLE_SOURCE_VBUS:
		rt1718s_write8(chip, RT1718S_GPIO2_CTRL, 0x0E);
		break;
	case TCPM_CMD_DISABLE_SOURCE_VBUS:
		rt1718s_write8(chip, RT1718S_GPIO2_CTRL, 0x0C);
		break;
	default:
		break;
	}
	rt1718s_write8(chip, TCPC_V10_REG_COMMAND, cmd);
}

static struct tcpc_ops rt1718s_tcpc_ops = {
	.init = rt1718s_tcpc_init,
	.init_alert_mask = rt1718s_init_mask,
	.alert_status_clear = rt1718s_alert_status_clear,
	.fault_status_clear = rt1718s_fault_status_clear,
	.get_alert_mask = rt1718s_get_alert_mask,
	.set_alert_mask = rt1718s_set_alert_mask,
	.get_alert_status_and_mask = rt1718s_get_alert_status_and_mask,
	.get_power_status = rt1718s_get_power_status,
	.get_fault_status = rt1718s_get_fault_status,
	.get_cc = rt1718s_get_cc,
	.set_cc = rt1718s_set_cc,
	.set_polarity = rt1718s_set_polarity,
	.set_vconn = rt1718s_set_vconn,
	.deinit = rt1718s_tcpc_deinit,
	.alert_vendor_defined_handler = rt1718s_alert_vendor_defined_handler,
	.set_auto_dischg_discnt = rt1718s_set_auto_dischg_discnt,
	.get_vbus_voltage = rt1718s_get_vbus_voltage,

	.set_low_power_mode = rt1718s_set_low_power_mode,

#if CONFIG_USB_POWER_DELIVERY
	.set_msg_header = rt1718s_set_msg_header,
	.set_rx_enable = rt1718s_set_rx_enable,
	.protocol_reset = rt1718s_protocol_reset,
	.get_message = rt1718s_get_message,
	.transmit = rt1718s_transmit,
	.set_bist_test_mode = rt1718s_set_bist_test_mode,
#endif	/* CONFIG_USB_POWER_DELIVERY */

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
	.retransmit = rt1718s_retransmit,
#endif	/* CONFIG_USB_PD_RETRY_CRC_DISCARD */

	.set_cc_hidet = rt1718s_set_cc_hidet,
	.get_cc_hi = rt1718s_get_cc_hi,

#if CONFIG_TYPEC_CAP_FORCE_DISCHARGE
	.set_force_discharge = rt1718s_set_force_discharge,
#endif	/* CONFIG_TYPEC_CAP_FORCE_DISCHARGE */

	.set_command = rt1718s_set_command,
};

static int rt1718s_check_chip_exist(struct rt1718s_chip *chip)
{
	u16 data = 0;
	int ret = 0;

	ret = rt1718s_read16(chip, TCPC_V10_REG_VID, &data);
	if (ret < 0)
		return ret;
	if (data != RT1718S_VID) {
		dev_notice(chip->dev, "vid is not correct, 0x%04x\n", data);
		return -ENODEV;
	}
	ret = rt1718s_read16(chip, TCPC_V10_REG_PID, &data);
	if (ret < 0)
		return ret;
	if (data != RT1718S_PID) {
		dev_notice(chip->dev, "pid is not correct, 0x%04x\n", data);
		return -ENODEV;
	}
	ret = rt1718s_read16(chip, TCPC_V10_REG_DID, &data);
	dev_info(chip->dev, "chipID = 0x%04x\n", data);

	return ret;
}

struct tcpc_desc def_tcpc_desc = {
	.role_def = TYPEC_ROLE_DRP,
	.rp_lvl = TYPEC_CC_RP_DFT,
	.vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS,
	.name = "type_c_port0",
	.en_fod = false,
	.en_ctd = false,
};

static int rt1718s_parse_dt(struct rt1718s_chip *chip, struct device *dev)
{
	struct tcpc_desc *desc = chip->tcpc_desc;
	u32 val = 0;
	int i = 0, ret = 0;
	const struct {
		const char *name;
		bool *val_ptr;
	} tcpc_props_bool[] = {
		{ "tcpc,en-fod", &desc->en_fod },
		{ "tcpc,en-ctd", &desc->en_ctd },
	};

	memcpy(desc, &def_tcpc_desc, sizeof(*desc));

	ret = device_property_read_string(dev, "tcpc,name", &desc->name);
	if (ret)
		dev_info(dev, "%s No tcpc,name node, use default name: %s\n",
			      __func__, desc->name);

	if (!device_property_read_u32(dev, "tcpc,role-def", &val) &&
	    val > TYPEC_ROLE_UNKNOWN && val < TYPEC_ROLE_NR)
		desc->role_def = val;

	if (!device_property_read_u32(dev, "tcpc,rp-level", &val)) {
		switch (val) {
		case TYPEC_RP_DFT:
		case TYPEC_RP_1_5:
		case TYPEC_RP_3_0:
			desc->rp_lvl = val;
			break;
		default:
			break;
		}
	}

	if (!device_property_read_u32(dev, "tcpc,vconn-supply", &val) &&
	    val < TCPC_VCONN_SUPPLY_NR)
		desc->vconn_supply = val;

	for (i = 0; i < ARRAY_SIZE(tcpc_props_bool); i++) {
		*tcpc_props_bool[i].val_ptr =
			device_property_read_bool(dev, tcpc_props_bool[i].name);
			dev_info(dev, "props[%s] = %d\n",
				 tcpc_props_bool[i].name,
				 *tcpc_props_bool[i].val_ptr);
	}

	return 0;
}

static int rt1718s_register_tcpcdev(struct rt1718s_chip *chip)
{
	struct tcpc_desc *desc = chip->tcpc_desc;
	struct tcpc_device *tcpc = NULL;

	tcpc = tcpc_device_register(chip->dev, desc, &rt1718s_tcpc_ops, chip);
	if (IS_ERR_OR_NULL(tcpc))
		return -EINVAL;
	chip->tcpc = tcpc;

#if CONFIG_USB_PD_DISABLE_PE
	tcpc->disable_pe = device_property_read_bool(chip->dev,
						     "tcpc,disable-pe");
#endif	/* CONFIG_USB_PD_DISABLE_PE */

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
	tcpc->tcpc_flags |= TCPC_FLAGS_RETRY_CRC_DISCARD;
#endif  /* CONFIG_USB_PD_RETRY_CRC_DISCARD */

#if CONFIG_USB_PD_REV30
	tcpc->tcpc_flags |= TCPC_FLAGS_PD_REV30;
#endif	/* CONFIG_USB_PD_REV30 */

	if (desc->en_fod)
		tcpc->tcpc_flags |= TCPC_FLAGS_FOREIGN_OBJECT_DETECTION;
	if (desc->en_ctd)
		tcpc->tcpc_flags |= TCPC_FLAGS_CABLE_TYPE_DETECTION;

	if (tcpc->tcpc_flags & TCPC_FLAGS_PD_REV30)
		dev_info(chip->dev, "PD_REV30\n");
	else
		dev_info(chip->dev, "PD_REV20\n");

	return 0;

}

static irqreturn_t rt1718s_intr_handler(int irq, void *data)
{
	struct rt1718s_chip *chip = data;

	pm_stay_awake(chip->dev);
	tcpci_lock_typec(chip->tcpc);
	tcpci_alert(chip->tcpc, false);
	tcpci_unlock_typec(chip->tcpc);
	pm_relax(chip->dev);

	return IRQ_HANDLED;
}

static int rt1718s_init_alert(struct rt1718s_chip *chip)
{
	int ret = 0;

	/* Clear Alert Mask & Status */
	ret = rt1718s_write16(chip, TCPC_V10_REG_ALERT_MASK, 0);
	if (ret < 0)
		return ret;
	ret = rt1718s_write16(chip, TCPC_V10_REG_ALERT, 0xffff);
	if (ret < 0)
		return ret;

	return 0;
}

static int rt1718s_probe(struct i2c_client *i2c)
{
	struct rt1718s_chip *chip = NULL;
	int ret = 0;

	dev_info(&i2c->dev, "%s (%s)\n", __func__, RT1718S_DRV_VERSION);

	chip = devm_kzalloc(&i2c->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	chip->dev = &i2c->dev;
	i2c_set_clientdata(i2c, chip);
	chip->regmap = devm_regmap_init(chip->dev, &rt1718s_regmap_bus,
					chip->dev, &rt1718s_regmap_config);
	if (IS_ERR(chip->regmap))
		return dev_err_probe(chip->dev, PTR_ERR(chip->regmap),
				     "Failed to init regmap\n");

	chip->tcpc_desc = devm_kzalloc(chip->dev, sizeof(*chip->tcpc_desc),
				       GFP_KERNEL);
	if (!chip->tcpc_desc)
		return -ENOMEM;
	ret = rt1718s_parse_dt(chip, chip->dev);
	if (ret < 0)
		return dev_err_probe(chip->dev, ret, "Failed to parse dt\n");

	chip->irq = i2c->irq;
	INIT_DELAYED_WORK(&chip->fod_polling_dwork,
			  rt1718s_fod_polling_dwork_handler);

	ret = rt1718s_register_tcpcdev(chip);
	if (ret < 0)
		return dev_err_probe(chip->dev, ret,
				     "Failed to register tcpcdev\n");

	ret = rt1718s_check_chip_exist(chip);
	if (ret < 0) {
		dev_notice(chip->dev, "Failed to check vid/pid(%d)\n", ret);
		goto err;
	}

	ret = rt1718s_sw_reset(chip);
	if (ret < 0) {
		dev_notice(chip->dev, "Failed to sw reset(%d)\n", ret);
		goto err;
	}
	if (!chip->tcpc_desc->en_fod) {
		ret = rt1718s_clr_bits(chip, RT1718S_FOD_CTRL,
				       RT1718S_M_FOD_SNK_EN);
		if (ret < 0) {
			dev_notice(chip->dev,
				   "Failed to disable fod snk en(%d)\n", ret);
			goto err;
		}
	}
	if (!chip->tcpc_desc->en_ctd) {
		ret = rt1718s_clr_bits(chip, RT1718S_SHIELDCTRL1,
				       RT1718S_M_CTD_EN);
		if (ret < 0) {
			dev_notice(chip->dev,
				   "Failed to disable ctd(%d)\n", ret);
			goto err;
		}
	}

	ret = rt1718s_init_alert(chip);
	if (ret < 0) {
		dev_notice(chip->dev, "Failed to init alert(%d)\n", ret);
		goto err;
	}
	ret = devm_request_threaded_irq(chip->dev, chip->irq, NULL,
					rt1718s_intr_handler,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					dev_name(chip->dev), chip);
	if (ret) {
		dev_notice(chip->dev, "Failed to request irq(%d)\n", ret);
		goto err;
	}

	dev_info(chip->dev, "%s successfully!\n", __func__);
	return 0;
err:
	tcpc_device_unregister(chip->dev, chip->tcpc);
	return ret;
}

static int rt1718s_remove(struct i2c_client *i2c)
{
	struct rt1718s_chip *chip = i2c_get_clientdata(i2c);

	disable_irq(chip->irq);
	cancel_delayed_work_sync(&chip->fod_polling_dwork);
	tcpc_device_unregister(chip->dev, chip->tcpc);

	return 0;
}

static void rt1718s_shutdown(struct i2c_client *i2c)
{
	struct rt1718s_chip *chip = i2c_get_clientdata(i2c);

	disable_irq(chip->irq);
	cancel_delayed_work_sync(&chip->fod_polling_dwork);
	tcpm_shutdown(chip->tcpc);
}

static int __maybe_unused rt1718s_suspend(struct device *dev)
{
	struct rt1718s_chip *chip = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	return tcpm_suspend(chip->tcpc);
}

static int __maybe_unused rt1718s_resume(struct device *dev)
{
	struct rt1718s_chip *chip = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	tcpm_resume(chip->tcpc);

	return 0;
}

static const struct dev_pm_ops rt1718s_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rt1718s_suspend, rt1718s_resume)
};

static const struct of_device_id rt1718s_of_match_table[] = {
	{.compatible = "richtek,rt1718s",},
	{},
};
MODULE_DEVICE_TABLE(of, rt1718s_of_match_table);

static struct i2c_driver rt1718s_driver = {
	.probe = rt1718s_probe,
	.remove = rt1718s_remove,
	.shutdown = rt1718s_shutdown,
	.driver = {
		.name = "rt1718s",
		.owner = THIS_MODULE,
		.of_match_table = rt1718s_of_match_table,
		.pm = &rt1718s_pm_ops,
	},
};
module_i2c_driver(rt1718s_driver);

MODULE_AUTHOR("Lucas Tsai <lucas_tsai@richtek.com>");
MODULE_DESCRIPTION("RT1718S USB Type-C Port Controller Interface Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(RT1718S_DRV_VERSION);
