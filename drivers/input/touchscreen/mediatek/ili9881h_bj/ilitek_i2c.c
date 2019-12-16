/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ilitek.h"

struct touch_bus_info {
	struct i2c_driver bus_driver;
	struct ilitek_hwif_info *hwif;
};

struct ilitek_tddi_dev *idev;

#if I2C_DMA_TRANSFER
static unsigned char *ilitek_dma_va;
static dma_addr_t ilitek_dma_pa;

#define DMA_VA_BUFFER   4096

static int dma_i2c_alloc(struct i2c_client *client)
{
	if (client != NULL) {
		client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
		ilitek_dma_va = (u8 *)dmam_alloc_coherent(client->dev, DMA_VA_BUFFER, &ilitek_dma_pa, GFP_KERNEL);
		if (ERR_ALLOC_MEM(ilitek_dma_va)) {
			ipio_err("Allocate DMA I2C Buffer failed\n");
			return -ENOMEM;
		}

		memset(ilitek_dma_va, 0, DMA_VA_BUFFER);
		client->ext_flag |= I2C_DMA_FLAG;
		return 0;
	}

	ipio_err("client is NULL, allocated dma i2c failed\n");
	return -ENODEV;
}
#endif

static int core_i2c_write(void *buf, int len)
{
	int ret = 0;
	u8 *txbuf = (u8 *)buf;
	u8 check_sum = 0;
	u8 *mpbuf = NULL;
	bool tmp = ipio_debug_level;

	struct i2c_msg msgs[] = {
		{
		 .addr = idev->i2c->addr,
		 .flags = 0,	/* write flag. */
		 .len = len,
		 .buf = txbuf,
		 },
	};

#if I2C_DMA_TRANSFER
	if (len > 8) {
		msgs[0].addr = (idev->client->addr & I2C_MASK_FLAG);
		msgs[0].ext_flag = (idev->client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
		memcpy(ilitek_dma_va, txbuf, len);
		msgs[0].buf = (u8 *)ilitek_dma_pa;
	}
#endif


	/*
	 * NOTE: If TP driver is doing MP test and commanding 0xF1 to FW, we add a checksum
	 * to the last index and plus 1 with size.
	 */
	if (idev->protocol->ver >= PROTOCOL_VER_540) {
		if (txbuf[0] == P5_X_SET_CDC_INIT && idev->actual_tp_mode == P5_X_FW_TEST_MODE) {
			check_sum = ilitek_calc_packet_checksum(txbuf, len);
			mpbuf = kcalloc(len + 1, sizeof(u8), GFP_KERNEL);
			if (ERR_ALLOC_MEM(mpbuf)) {
				ipio_err("Failed to allocate mpbuf mem\n");
				return -ENOMEM;
			}
			ipio_memcpy(mpbuf, txbuf, len, msgs[0].len);
			mpbuf[len] = check_sum;
			msgs[0].buf = mpbuf;
			ipio_debug_level = DEBUG_ALL;
			ilitek_dump_data(mpbuf, 8, len+1, 0, "mp cdc cmd");
			ipio_debug_level = tmp;
			msgs[0].len = len + 1;
		}
	}

	if (i2c_transfer(idev->i2c->adapter, msgs, 1) != 1)
		ret = -1;

	ipio_kfree((void **)&mpbuf);
	return ret;
}

static int core_i2c_read(void *buf, int len)
{
	int ret;
	u8 *rxbuf = (u8 *)buf;

	struct i2c_msg msgs[] = {
		{
		 .addr = idev->i2c->addr,
		 .flags = I2C_M_RD,	/* read flag. */
		 .len = len,
		 .buf = rxbuf,
		 },
	};


#if I2C_DMA_TRANSFER
	if (len > 8) {
		msgs[0].addr = (idev->client->addr & I2C_MASK_FLAG);
		msgs[0].ext_flag = (idev->client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
		msgs[0].buf = (u8 *)ilitek_dma_pa;
	} else {
		msgs[0].buf = rxbuf;
	}
#endif


	ret = i2c_transfer(idev->i2c->adapter, msgs, 1);

#if I2C_DMA_TRANSFER
	if (len > 8)
		memcpy(rxbuf, ilitek_dma_va, len);
#endif

	/*
	 * If i2c_transfer is ok (must return 1 because only sends one msg),
	 * return #bytes transferred, else error code.
	 */
	return (ret == 1) ? len : ret;
}

static int ilitek_i2c_write(void *buf, int len)
{
	int ret = 0;

	if (len == 0) {
		ipio_err("i2c write len is invalid\n");
		return -EINVAL;
	}

	ret = core_i2c_write(buf, len);
	if (ret < 0) {
		if (atomic_read(&idev->tp_reset) == START) {
			ret = 0;
			goto out;
		}
		ipio_err("i2c write error, ret = %d\n", ret);
	}

out:
	return ret;
}

static int ilitek_i2c_read(void *buf, int len)
{
	int ret = 0;

	if (len == 0) {
		ipio_err("i2c read len is invalid\n");
		return -EINVAL;
	}

	ret = core_i2c_read(buf, len);
	if (ret < 0) {
		if (atomic_read(&idev->tp_reset) == START) {
			ret = 0;
			goto out;
		}
		ipio_err("i2c read error, ret = %d\n", ret);
	}

out:
	return ret;
}

static int ilitek_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct touch_bus_info *info =
		container_of(to_i2c_driver(i2c->dev.driver),
			struct touch_bus_info, bus_driver);

	ipio_info("ilitek i2c probe\n");

	if (!i2c) {
		ipio_err("i2c client is NULL\n");
		return -ENODEV;
	}
	pr_err("wyx-----%s---------\n",__func__);
	if (i2c->addr != TDDI_I2C_ADDR) {
		i2c->addr = TDDI_I2C_ADDR;
		ipio_info("i2c addr doesn't be set up, use default : 0x%x\n", i2c->addr);
	}

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		ipio_err("i2c functions are not supported!\n");
		return -ENODEV;
	}

	idev = devm_kzalloc(&i2c->dev, sizeof(struct ilitek_tddi_dev), GFP_KERNEL);
	if (ERR_ALLOC_MEM(idev)) {
		ipio_err("Failed to allocate idev memory, %ld\n", PTR_ERR(idev));
		return -ENOMEM;
	}

	/* Used for receiving touch data only, do not mix up with others. */
	idev->tr_buf = kzalloc(TR_BUF_SIZE, GFP_ATOMIC);
	if (ERR_ALLOC_MEM(idev->tr_buf)) {
		ipio_err("failed to allocate touch report buffer\n");
		return -ENOMEM;
	}

	idev->gcoord = kzalloc(sizeof(struct gesture_coordinate), GFP_KERNEL);
	if (ERR_ALLOC_MEM(idev->gcoord)) {
		ipio_err("Failed to allocate gresture coordinate buffer\n");
		return -ENOMEM;
	}

	idev->i2c = i2c;
	idev->spi = NULL;
	idev->dev = &i2c->dev;
	idev->hwif = info->hwif;
	idev->phys = "I2C";

	idev->write = ilitek_i2c_write;
	idev->read = ilitek_i2c_read;
#if I2C_DMA_TRANSFER
	if (dma_i2c_alloc(idev->i2c) < 0)
		ipio_err("Failed to alllocate DMA mem %ld\n", PTR_ERR(idev->i2c));
#endif

	idev->spi_speed = NULL;
	idev->actual_tp_mode = P5_X_FW_AP_MODE;
	idev->tp_data_len = P5_X_DEMO_MODE_PACKET_LEN;

	if (TDDI_RST_BIND)
		idev->reset = TP_IC_WHOLE_RST;
	else
		idev->reset = TP_HW_RST_ONLY;

	idev->rst_edge_delay = 100;
	idev->fw_open = FILP_OPEN;
	idev->fw_upgrade_mode = UPGRADE_FLASH;
	idev->mp_move_code = ilitek_tddi_move_mp_code_flash;
	idev->gesture_move_code = ilitek_tddi_move_gesture_code_flash;
	idev->esd_recover = ilitek_tddi_wq_esd_i2c_check;
	idev->ges_recover = ilitek_tddi_touch_esd_gesture_flash;
	idev->gesture_mode = DATA_FORMAT_GESTURE_NORMAL;
	idev->gesture_demo_ctrl = DISABLE;
	idev->wtd_ctrl = OFF;
	idev->report = ENABLE;
	idev->netlink = DISABLE;
	idev->debug_node_open = DISABLE;
	idev->irq_tirgger_type = IRQF_TRIGGER_FALLING;
	idev->info_from_hex = ENABLE;

#if ENABLE_GESTURE
	idev->gesture = DISABLE;
#endif

	return info->hwif->plat_probe();
}

static int ilitek_i2c_remove(struct i2c_client *i2c)
{
	ipio_info();
	return 0;
}

static const struct i2c_device_id tp_i2c_id[] = {
	{TDDI_DEV_ID, 0},
};

int ilitek_tddi_interface_dev_init(struct ilitek_hwif_info *hwif)
{
	struct touch_bus_info *info;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		ipio_err("faied to allocate i2c_driver\n");
		return -ENOMEM;
	}

	if (hwif->bus_type != BUS_I2C) {
		ipio_err("Not I2C dev\n");
		return -EINVAL;
	}

	hwif->info = info;

	info->bus_driver.driver.name = hwif->name;
	info->bus_driver.driver.owner = hwif->owner;
	info->bus_driver.driver.of_match_table = hwif->of_match_table;

	info->bus_driver.probe = ilitek_i2c_probe;
	info->bus_driver.remove = ilitek_i2c_remove;
	info->bus_driver.id_table = tp_i2c_id;

	info->hwif = hwif;
	return i2c_add_driver(&info->bus_driver);
}

void ilitek_tddi_interface_dev_exit(struct ilitek_tddi_dev *idev)
{
	struct touch_bus_info *info = (struct touch_bus_info *)idev->hwif->info;

	ipio_info("remove i2c dev\n");
	i2c_del_driver(&info->bus_driver);
	ipio_kfree((void **)&info);
}
