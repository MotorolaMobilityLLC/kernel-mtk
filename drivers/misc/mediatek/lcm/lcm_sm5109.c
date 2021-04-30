#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>	/* hwPowerOn */
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#endif
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>

#include "lcm_define.h"
#include "lcm_drv.h"
#include "lcm_i2c.h"

#define LCM_I2C_ID_NAME "sm5109"
 static struct i2c_client *_sm5109_i2c_client;

 /*****************************************************************************
  * Function I2C
  *****************************************************************************/
 static int _lcm_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id);
 static int _lcm_i2c_remove(struct i2c_client *client);

 struct _lcm_i2c_dev {
	 struct i2c_client *client;
 };

 static int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value)
 {
	 int ret = 0;
	 struct i2c_client *client = _sm5109_i2c_client;
	 char write_data[2] = { 0 };

	 if (client == NULL) {
		 pr_debug("ERROR!! _lcm_i2c_client is null\n");
		 return 0;
	 }
	// pr_err("[LCM][I2C] __lcm_i2c_write_bytes\n");

	 write_data[0] = addr;
	 write_data[1] = value;
	 ret = i2c_master_send(client, write_data, 2);
	 if (ret < 0)
		 pr_info("[LCM][ERROR] _lcm_i2c write data fail !!\n");

	 return ret;
 }

 static int _lcm_i2c_read_bytes(unsigned char cmd, unsigned char *returnData)
 {
	char readData = 0;
	int ret = 0;
	struct i2c_msg msg[2];
	struct i2c_adapter *adap = _sm5109_i2c_client->adapter;

	msg[0].addr = _sm5109_i2c_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &cmd;
	msg[1].addr = _sm5109_i2c_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &readData;
	ret = i2c_transfer(adap, msg, 2);
	if(ret < 0) {
		pr_err("[LCM][I2C] _lcm_i2c_read_byte i2c transf err \n");
		return 0;
	}
	*returnData = readData;

	return 1;
}

 static int SM5109_REG_MASK (unsigned char addr, unsigned char val, unsigned char mask)
 {
		 unsigned char SM5109_reg=0;
		 unsigned char ret = 0;
		// pr_err("[LCM][I2C] SM5109_REG_MASK\n");
		 ret = _lcm_i2c_read_bytes(addr, &SM5109_reg);	//modify i2c read is repeat or restart
		 SM5109_reg &= ~mask;
		 SM5109_reg |= val;

		 ret = _lcm_i2c_write_bytes(addr, SM5109_reg);

		 return ret;
 }

 static void lcm_set_bias_config(unsigned int value,unsigned int delay)
 {
	 int ret;
	 pr_info("%s: value=%d\n",__func__,value);
	 /* set AVDD*/
	 /*4.0V + value* 100mV*/
	 ret = SM5109_REG_MASK(0x00, value,  (0x1F << 0));
		 if (ret < 0)
			 pr_err("[sm5109][ERROR] %s: cmd=%0x--i2c write error----\n",__func__, 0x00);
		 else
			 pr_info("%s: sm5109----cmd=%0x--i2c write success----\n",__func__, 0x00);

 	 mdelay(delay);

	 /* set AVEE */
	 /*-4.0V - value* 100mV*/
	ret = SM5109_REG_MASK(0x01, value,  (0x1F << 0));
	if (ret < 0)
		pr_err("[sm5109][ERROR] %s: cmd=%0x--i2c write error----\n",__func__, 0x01);
	else
		pr_info("%s: sm5109----cmd=%0x--i2c write success----\n",__func__, 0x01);

	 /* enable AVDD & AVEE discharge*/
	 ret = SM5109_REG_MASK(0x03, (1<<0) | (1<<1), (1<<0) | (1<<1));
	 if (ret < 0)
		pr_err("[sm5109][ERROR] %s: cmd=%0x--i2c write error----\n",__func__, 0x03);
	 else
		pr_info("%s: sm5109----cmd=%0x--i2c write success----\n",__func__, 0x03);

	 return;
 }

void lcm_set_bias_pin_enable(unsigned int value,unsigned int delay)
 {
	 pr_info("%s\n",__func__);

	 disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP1);
	 mdelay(delay);
	 disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN1);
	 lcm_set_bias_config(value,delay);
	 return;
 }
EXPORT_SYMBOL(lcm_set_bias_pin_enable);

 void lcm_set_bias_pin_disable(unsigned int delay)
 {
	 pr_info("%s\n",__func__);

	 disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN0);
	 mdelay(delay);
	 disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP0);
	 return;
 }
EXPORT_SYMBOL(lcm_set_bias_pin_disable);

void lcm_set_bias_init(unsigned int value,unsigned int delay)
 {
	 pr_info("%s\n",__func__);
	 /*init bias IC sm5109  value:set vol;delay:delay between ENP and ENN */
	 lcm_set_bias_pin_enable(value,delay);
 	 return;
 }
EXPORT_SYMBOL(lcm_set_bias_init);

static int _lcm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {
	 pr_info("[LCM][I2C] _lcm_i2c_probe\n");
	 pr_info("[LCM][I2C] : info==>name=%s addr=0x%x\n",client->name, client->addr);

	 _sm5109_i2c_client = client;
	 return 0;
 }

static int _lcm_i2c_remove(struct i2c_client *client)
 {
	 pr_info("[LCM][I2C] _lcm_i2c_remove\n");
	 _sm5109_i2c_client = NULL;
	 i2c_unregister_device(client);
	 return 0;
 }

 static const struct of_device_id _lcm_i2c_of_match[] = {
	 {
	  .compatible = "mediatek,i2c_lcd_bias",
	  },
 };

 static const struct i2c_device_id _lcm_i2c_id[] = {
	 {LCM_I2C_ID_NAME, 0},
	 {}
 };

 static struct i2c_driver _lcm_i2c_driver = {
	 .id_table = _lcm_i2c_id,
	 .probe = _lcm_i2c_probe,
	 .remove = _lcm_i2c_remove,
	 .driver = {
			.owner = THIS_MODULE,
			.name = LCM_I2C_ID_NAME,
#ifdef CONFIG_MTK_LEGACY
#else
			.of_match_table = _lcm_i2c_of_match,
#endif
			},
 };

static int __init _lcm_bias_init(void)
{
	 if ( i2c_add_driver(&_lcm_i2c_driver) != 0) {
		 pr_info("[LCM]unable to register LCM BIAS driver.\n");
		 return -1;
	 }
	 return 0;
}

static void __exit _lcm_bias_exit(void)
 {
	 i2c_del_driver(&_lcm_i2c_driver);
 }

module_init(_lcm_bias_init);
module_exit(_lcm_bias_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ADD SM5109 BIAS driver");
