#define pr_fmt(fmt) "mt8193-iic: " fmt
#define DEBUG 1

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "mt8193_iic.h"
#include "mt8193.h"

/*----------------------------------------------------------------------------*/
/* mt8193 device information                                                  */
/*----------------------------------------------------------------------------*/
#define MAX_TRANSACTION_LENGTH 8
#define MT8193_DEVICE_NAME            "mtk-multibridge"
#define MT8193_I2C_SLAVE_ADDR       0x3A
#define MT8193_I2C_DEVICE_ADDR_LEN   2
/*----------------------------------------------------------------------------*/
static int mt8193_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mt8193_i2c_remove(struct i2c_client *client);
static struct i2c_client *mt8193_i2c_client;
static const struct i2c_device_id mt8193_i2c_id[] = {{MT8193_DEVICE_NAME, 0}, {} };
static struct i2c_board_info i2c_mt8193 __initdata = {I2C_BOARD_INFO(MT8193_DEVICE_NAME, (MT8193_I2C_SLAVE_ADDR>>1))};
/*----------------------------------------------------------------------------*/
struct i2c_driver mt8193_i2c_driver = {
	.probe		= mt8193_i2c_probe,
	.remove		= mt8193_i2c_remove,
	.driver		= { .name = MT8193_DEVICE_NAME, },
	.id_table	= mt8193_i2c_id,
};

struct mt8193_i2c_data {
	struct i2c_client *client;
	uint16_t addr;
	int use_reset;		/*use RESET flag*/
	int use_irq;		/*use EINT flag*/
	int retry;
};

static struct mt8193_i2c_data *obj_i2c_data;

/*----------------------------------------------------------------------------*/
int mt8193_i2c_read(u16 addr, u32 *data)
{
	u8 rxBuf[8] = {0};
	int ret = 0;
	struct i2c_client *client = mt8193_i2c_client;
	u8 lens;

	if (((addr >> 8) & 0xFF) >= 0x80) {
		/* 8 bit : fast mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		lens = 1;
	} else {
		/* 16 bit : noraml mode */
		rxBuf[0] = (addr >> 8) & 0xFF;
		rxBuf[1] = addr & 0xFF;
		lens = 2;
	}

	client->addr = (client->addr & I2C_MASK_FLAG);
	client->timing = 400;
	client->ext_flag = I2C_WR_FLAG;

	ret = i2c_master_send(client, (const char *)&rxBuf, (4 << 8) | lens);
	if (ret < 0) {
		pr_err("%s: read error\n", __func__);
		return -EFAULT;
	}
	*data = (rxBuf[3] << 24) | (rxBuf[2] << 16) | (rxBuf[1] << 8) | (rxBuf[0]); /*LSB fisrt*/
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(mt8193_i2c_read);
/*----------------------------------------------------------------------------*/

int mt8193_i2c_write(u16 addr, u32 data)
{
	struct i2c_client *client = mt8193_i2c_client;
	u8 buffer[8];
	int ret = 0;
	struct i2c_msg msg = {
		.addr	= client->addr & I2C_MASK_FLAG,
		.flags	= 0,
		.len	= (((addr >> 8) & 0xFF) >= 0x80)?5:6,
		.buf	= buffer,
		.timing	= 400,
	};

	if (((addr >> 8) & 0xFF) >= 0x80) {
		/* 8 bit : fast mode */
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = (data >> 24) & 0xFF;
		buffer[2] = (data >> 16) & 0xFF;
		buffer[3] = (data >> 8) & 0xFF;
		buffer[4] = data & 0xFF;
	} else {
		/* 16 bit : noraml mode */
		buffer[0] = (addr >> 8) & 0xFF;
		buffer[1] = addr & 0xFF;
		buffer[2] = (data >> 24) & 0xFF;
		buffer[3] = (data >> 16) & 0xFF;
		buffer[4] = (data >> 8) & 0xFF;
		buffer[5] = data & 0xFF;
	}

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		pr_err("%s: send command error\n", __func__);
		return -EFAULT;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(mt8193_i2c_write);

/*----------------------------------------------------------------------------*/
/* IIC Probe                                                                  */
/*----------------------------------------------------------------------------*/
static int mt8193_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	struct mt8193_i2c_data *obj;

	pr_debug("%s\n", __func__);

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Allocate ts memory fail\n", __func__);
		return ret;
	}
	obj_i2c_data = obj;
	client->timing = 400;
	obj->client = client;
	mt8193_i2c_client = obj->client;
	i2c_set_clientdata(client, obj);

	return 0;
}
/*----------------------------------------------------------------------------*/

static int mt8193_i2c_remove(struct i2c_client *client)
{
	pr_debug("%s\n", __func__);
	mt8193_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

/*----------------------------------------------------------------------------*/
/* device driver probe                                                        */
/*----------------------------------------------------------------------------*/
static int mt8193_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	if (i2c_add_driver(&mt8193_i2c_driver)) {
		pr_err("%s: unable to add mt8193 i2c driver\n", __func__);
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mt8193_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	i2c_del_driver(&mt8193_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id multibridge_of_ids[] = {
	{.compatible = "mediatek,multibridge", },
	{}
};
#endif

static struct platform_driver mt8193_mb_driver = {
	.probe		= mt8193_probe,
	.remove		= mt8193_remove,
	.driver		= {
		.name	= "multibridge",
#ifdef CONFIG_OF
		.of_match_table = multibridge_of_ids,
#endif
	}
};

/*----------------------------------------------------------------------------*/
static int __init mt8193_mb_init(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	/*i2c_register_board_info(1, &i2c_mt8193, 1);*/
	ret = platform_driver_register(&mt8193_mb_driver);
	if (ret) {
		pr_err("failed to register mt8193_mb_driver, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit mt8193_mb_exit(void)
{
	platform_driver_unregister(&mt8193_mb_driver);
}
/*----------------------------------------------------------------------------*/
static int __init mt8193_i2c_board_init(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
	ret = i2c_register_board_info(MT8193_I2C_ID, &i2c_mt8193, 1);
	if (ret)
		pr_err("failed to register mt8193 i2c_board_info, ret=%d\n", ret);

	return ret;
}
/*----------------------------------------------------------------------------*/
core_initcall(mt8193_i2c_board_init);
module_init(mt8193_mb_init);
module_exit(mt8193_mb_exit);
MODULE_AUTHOR("SS, Wu <ss.wu@mediatek.com>");
MODULE_DESCRIPTION("MT8193 Driver");
MODULE_LICENSE("GPL");
