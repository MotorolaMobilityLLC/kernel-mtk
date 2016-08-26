#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h> 
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#endif

//Lenovo-sw wuwl10 add 20150515 for esd recover backlight
#ifndef BUILD_LK
static unsigned int esd_last_backlight_level = 255;
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	(lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update))
#define dsi_set_cmdq(pdata, queue_size, force_update)		(lcm_util.dsi_set_cmdq(pdata, queue_size, force_update))
#define wrtie_cmd(cmd)										(lcm_util.dsi_write_cmd(cmd))
#define write_regs(addr, pdata, byte_nums)					(lcm_util.dsi_write_regs(addr, pdata, byte_nums))
#define read_reg(cmd)										(lcm_util.dsi_dcs_read_lcm_reg(cmd))
#define read_reg_v2(cmd, buffer, buffer_size)   			(lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size))

#define dsi_lcm_set_gpio_out(pin, out)						(lcm_util.set_gpio_out(pin, out))
#define dsi_lcm_set_gpio_mode(pin, mode)					(lcm_util.set_gpio_mode(pin, mode))
#define dsi_lcm_set_gpio_dir(pin, dir)						(lcm_util.set_gpio_dir(pin, dir))
#define dsi_lcm_set_gpio_pull_enable(pin, en)				(lcm_util.set_gpio_pull_enable(pin, en))

#define LCM_DSI_CMD_MODE								(0)
#define REGFLAG_DELAY										0xFE
#define REGFLAG_END_OF_TABLE						0xFF
#define set_gpio_lcd_enp(cmd) 		(lcm_util.set_gpio_lcd_enp_bias(cmd))
#define set_gpio_lcd_enn(cmd) 		(lcm_util.set_gpio_lcd_enn_bias(cmd))
#define set_gpio_lcd_vddi(cmd) 		(lcm_util.set_gpio_lcd_vddi_en(cmd))
#if 1  //ndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
//#include <linux/jiffies.h>
#include <linux/uaccess.h>
//#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL	/* for I2C channel 0 */
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

#if defined(CONFIG_MTK_LEGACY)
static struct i2c_board_info tps65132_board_info __initdata = { I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR) };
#else
static const struct of_device_id lcm_of_match[] = {
		{.compatible = "mediatek,i2c_lcd_bias"},
		{},
};
#endif

struct i2c_client *tps65132_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct tps65132_dev {
	struct i2c_client *client;

};

static const struct i2c_device_id tps65132_id[] = {
	{I2C_ID_NAME, 0},
	{}
};

static struct i2c_driver tps65132_iic_driver = {
	.id_table = tps65132_id,
	.probe = tps65132_probe,
	.remove = tps65132_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "tps65132",
#if !defined(CONFIG_MTK_LEGACY)
			.of_match_table = lcm_of_match,
#endif
		   },
};

/***************************************************************************** 
 * Function
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk( "tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client = client;
	return 0;
}

static int tps65132_remove(struct i2c_client *client)
{
	printk( "tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

/*static int tps65132_write_bytes(unsigned char addr, unsigned char value)*/

int tps65132_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2] = { 0 };

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
	printk("tps65132 write data fail !!\n");	
	return ret;
}

static int __init tps65132_iic_init(void)
{
	printk( "tps65132_iic_init\n");
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
#endif
	printk( "tps65132_iic_init2\n");
	i2c_add_driver(&tps65132_iic_driver);
	printk( "tps65132_iic_init success\n");
	return 0;
}

static void __exit tps65132_iic_exit(void)
{
	printk( "tps65132_iic_exit\n");
	i2c_del_driver(&tps65132_iic_driver);
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL");

#else

#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;//I2C2;
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    //printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

#endif

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 2, {0xFF,0x03}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
//lenovo wuwl10 20150604 add CUSTOM_LCM_FEATURE begin
#ifdef CONFIG_LENOVO_CUSTOM_LCM_FEATURE
static struct LCM_setting_table lcm_cabc_level_setting[] = {
	{0x55, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
//lenovo wuwl10 20150604 add CUSTOM_LCM_FEATURE begin
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;
		
		switch(cmd) {
			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;
				
			case REGFLAG_END_OF_TABLE :
				break;
				
			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}



static void init_lcm_registers(void)
{

	unsigned int data_array[16];
//lenovo-sw wuwl10 add for pwm freq 10.9k  begin
	data_array[0] = 0x00001500;//orise mode
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00042902;
	data_array[1] = 0x011687FF;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00032902;
	data_array[1] = 0x001687FF;
	dsi_set_cmdq(data_array, 2, 1);
//cabc begin
	data_array[0] = 0x81001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000A2902;
	data_array[1] = 0xCCFFE5CA;
	data_array[2] = 0x00FFB2FF;
	data_array[3] = 0x00000000;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x10C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0xBBACA0C7;
	data_array[2] = 0xAAACDB9C;
	data_array[3] = 0xCBCCCBBB;
	data_array[4] = 0x45678ABB;
	data_array[5] = 0x00111123;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x11C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0xBBABA0C7;
	data_array[2] = 0xA9BBDA9B;
	data_array[3] = 0xBBCCBABB;
	data_array[4] = 0x45678AAB;
	data_array[5] = 0x00111123;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x12C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0xABAAA0C7;
	data_array[2] = 0xA9ABDA8C;
	data_array[3] = 0xCBBBBABA;
	data_array[4] = 0x45678B9A;
	data_array[5] = 0x00111123;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x13C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0xAB9B90C7;
	data_array[2] = 0x98BBCA8B;
	data_array[3] = 0xBCBAAABA;
	data_array[4] = 0x456789AA;
	data_array[5] = 0x00111123;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x14C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x9B8B90C7;
	data_array[2] = 0x98BBB98C;
	data_array[3] = 0xBBBAAAAA;
	data_array[4] = 0x5678899A;
	data_array[5] = 0x00111123;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x15C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x9A9A90C7;
	data_array[2] = 0x98BAC89B;
	data_array[3] = 0xCBA9AAA9;
	data_array[4] = 0x56788999;
	data_array[5] = 0x00122234;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x16C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x8B8A90C7;
	data_array[2] = 0x98AAB98B;
	data_array[3] = 0xCAA99AA9;
	data_array[4] = 0x6788898A;
	data_array[5] = 0x00222235;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x17C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x9A9990C7;
	data_array[2] = 0x98AAA98A;
	data_array[3] = 0xBB98B999;
	data_array[4] = 0x6788898A;
	data_array[5] = 0x00233345;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x18C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x999990C7;
	data_array[2] = 0x98AAA88A;
	data_array[3] = 0xAA9999A8;
	data_array[4] = 0x7888898B;
	data_array[5] = 0x00333346;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x19C61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x998A80C7;
	data_array[2] = 0x89A9A88A;
	data_array[3] = 0xB9989A89;
	data_array[4] = 0x7888879A;
	data_array[5] = 0x00444456;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1AC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x899980C7;
	data_array[2] = 0x98A9988A;
	data_array[3] = 0xA9899998;
	data_array[4] = 0x888888A9;
	data_array[5] = 0x00444467;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1BC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x998980C7;
	data_array[2] = 0x89999889;
	data_array[3] = 0xA9898998;
	data_array[4] = 0x88888899;
	data_array[5] = 0x00555567;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1CC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x989880C7;
	data_array[2] = 0x88999889;
	data_array[3] = 0xA8899898;
	data_array[4] = 0x88888899;
	data_array[5] = 0x00555678;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1DC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x898880C7;
	data_array[2] = 0x88998889;
	data_array[3] = 0x98889898;
	data_array[4] = 0x88888999;
	data_array[5] = 0x00666678;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1EC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x988880C7;
	data_array[2] = 0x89898888;
	data_array[3] = 0x98889888;
	data_array[4] = 0x88888988;
	data_array[5] = 0x00677788;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1FC61500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00132902;
	data_array[1] = 0x888880C7;
	data_array[2] = 0x88898888;
	data_array[3] = 0x98888888;
	data_array[4] = 0x88888798;
	data_array[5] = 0x00788888;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00C61500;
	dsi_set_cmdq(data_array, 1, 1);
//cabc end

	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x0CCA1500;//10.9k
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xB1001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x0CCA1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xB2001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x02CA1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00042902;
	data_array[1] = 0x040202CA;//10bit
	dsi_set_cmdq(data_array, 2, 1);
//lenovo-sw wuwl10 add for pwm freq 10.9k  end
	data_array[0] = 0x00351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00511500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x24531500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01551500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00022902;
	data_array[1] = 0x00030A5E;////min cabc 10
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	    	
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

}


//lenovo-sw wuwl10 20150515 add for esd revovery backlight begin
#ifndef BUILD_LK
static void lcm_esd_recover_backlight(void)
{
	printk("%s tm, level = %d, then reset touch\n", __func__, esd_last_backlight_level);

//lenvo-sw wuwl10 add for esd reset touch begin
	mt_set_gpio_mode_base(10, GPIO_MODE_00);
	mt_set_gpio_dir_base(10, GPIO_DIR_OUT);
	mt_set_gpio_out_base(10, GPIO_OUT_ZERO);
	MDELAY(10);

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = (esd_last_backlight_level>>2) & 0xFF;
	lcm_backlight_level_setting[0].para_list[1] = esd_last_backlight_level & 0x03;
	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);

	mt_set_gpio_mode_base(10, GPIO_MODE_00);
	mt_set_gpio_dir_base(10, GPIO_DIR_OUT);
	mt_set_gpio_out_base(10, GPIO_OUT_ONE);
//lenvo-sw wuwl10 add for esd reset touch end
}
#endif
//lenovo-sw wuwl10 20150515 add for esd revovery backlight end
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	if((0 < level) && (level < 3))
	{
		level = 3;
	}
	level = level*16/5 + 8/10;
#ifdef BUILD_LK
	dprintf(0,"%s,lk ft8716 tm backlight: level = %d\n", __func__, level);
#else
	printk(KERN_INFO "%s, kernel ft8716 tm backlight: level = %d,0x%x,0x%x\n", __func__,level,((level>>2) & 0xFF), level & 0x03);
//Lenovo-sw wuwl10 add 20150515 for esd recover backlight
	esd_last_backlight_level = level;
#endif
	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = (level>>2) & 0xFF;
	lcm_backlight_level_setting[0].para_list[1] = level & 0x03;
	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS * util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS * params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 16;
	params->dsi.vertical_frontporch = 16;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 32;//wuwl10 modify for new param
	params->dsi.horizontal_frontporch = 32;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	//params->dsi.pll_select=1;     //0: MIPI_PLL; 1: LVDS_PLL
	// Bit rate calculation
	//1 Every lane speed
	params->dsi.PLL_CLOCK = 430;//wuwl10 modify for new param
	//params->dsi.clk_lp_per_line_enable= 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.ssc_disable = 1;

	params->dsi.lcm_esd_check_table[1].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x9C;

	params->dsi.lcm_esd_check_table[0].cmd = 0x0D;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x00;
#if 0
	params->dsi.lcm_esd_check_table[0].cmd = 0x0E;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
#endif
}

static void lcm_init(void)
{

	unsigned char cmd = 0x00;
	unsigned char data = 0x0F;
	int ret=0;
	
	mt_set_gpio_mode_base(11, GPIO_MODE_00);
	mt_set_gpio_dir_base(11, GPIO_DIR_OUT);
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	set_gpio_lcd_vddi(0);
	set_gpio_lcd_enp(0);
	set_gpio_lcd_enn(0);
	SET_RESET_PIN(0);
	MDELAY(5);
	//set vddi enable
	set_gpio_lcd_vddi(1);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(6);
	
	//set avdd enable
	set_gpio_lcd_enp(1);
	MDELAY(6);
	
	//set avee enable
	set_gpio_lcd_enn(1);
	MDELAY(6);

#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)
		printf("[LK]tps65132----cmd=%0x--i2c write error----\n",cmd);
	else
		printf("[LK]tps65132----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif

	cmd = 0x01;
	data = 0x0F;

#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)
		printf("[LK]tps65132----cmd=%0x--i2c write error----\n",cmd);
	else
		printf("[LK]tps65132----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
	MDELAY(2);
	SET_RESET_PIN(0);
	MDELAY(2);

	SET_RESET_PIN(1);
	MDELAY(8);
	//lcm init code
	init_lcm_registers();

	//MDELAY(5);
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	mt_set_gpio_mode_base(11, GPIO_MODE_00);
	mt_set_gpio_dir_base(11, GPIO_DIR_OUT);
	mt_set_gpio_out_base(11, GPIO_OUT_ZERO);
	MDELAY(5);

	data_array[0] = 0x00280500;	// Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);

	data_array[0] = 0x00100500;	// Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
#if 0 //wuwl10 20160530 modify
	//set reset disable
	SET_RESET_PIN(0);
	MDELAY(3);

	//set avee enable
	set_gpio_lcd_enn(0);
	MDELAY(1);

	//set avdd disable
	set_gpio_lcd_enp(0);
	MDELAY(4);
#endif
}
//lenovo-sw wuwl10 add ,touch will call this funcion when suspend
void lcm_power_off(void)
{

	//set reset disable
	SET_RESET_PIN(0);
	MDELAY(3);
	//set avee enable
	set_gpio_lcd_enn(0);
	MDELAY(1);

	//set avdd disable
	set_gpio_lcd_enp(0);
	MDELAY(5);
	set_gpio_lcd_vddi(0);
	MDELAY(1);
}
EXPORT_SYMBOL_GPL(lcm_power_off);

static void lcm_resume(void)
{
	lcm_init();
}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
		       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] =
	    (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] =
	    (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

const static unsigned char LCD_MODULE_ID = 0x00;
static unsigned int lcm_compare_id(void)
{
	unsigned char  id_pin_read = 0;

	//mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
	//mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
	//id_pin_read = mt_get_gpio_in(GPIO_DISP_ID0_PIN);

#ifdef BUILD_LK
	dprintf(0, "%s,  id ft8716 tm= 0x%x LCD_ID_value=%d \n", __func__, id,id_pin_read);
#endif
	if(LCD_MODULE_ID == id_pin_read)
		return 1;
	else
		return 0;
}
//lenovo-sw wuwl10 modify 20150514 for new lcm timming end

//lenovo wuwl10 20150604 add CUSTOM_LCM_FEATURE begin
#ifdef CONFIG_LENOVO_CUSTOM_LCM_FEATURE
static void lcm_set_cabcmode(void *handle,unsigned int mode)
{
	#ifdef BUILD_LK
		dprintf(0,"%s, mode = %d\n", __func__, mode);
	#else
		printk("%s, mode = %d\n", __func__, mode);
	#endif

	lcm_cabc_level_setting[0].para_list[0] = mode;
	push_table(lcm_cabc_level_setting, sizeof(lcm_cabc_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif
//lenovo wuwl10 20150604 add CUSTOM_LCM_FEATURE end
#ifdef CONFIG_LENOVO_PANELMODE_SUPPORT
static void lcm_set_hbm(void *handle,unsigned int mode)
{
	printk("%s, mode = %d\n", __func__, mode);

	if( mode == 1){
		lcm_backlight_level_setting[0].para_list[0] = (1023>>2) & 0xFF;
		lcm_backlight_level_setting[0].para_list[1] = 1023 & 0x03;
	} else{
		lcm_backlight_level_setting[0].para_list[0] = (esd_last_backlight_level>>2) & 0xFF;
		lcm_backlight_level_setting[0].para_list[1] = esd_last_backlight_level & 0x03;
	}
	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif
LCM_DRIVER ft8716_fhd_dsi_vdo_tm_lcm_drv = {
	.name = "ft8716_fhd_dsi_vdo_tm",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	//.init_power		= lcm_init_power,
//Lenovo-sw wuwl10 add 20150515 for esd recover backlight
#ifndef BUILD_LK
	.esd_recover_backlight = lcm_esd_recover_backlight,
#endif
	//.resume_power = lcm_resume_power,
	//.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq     = lcm_setbacklight_cmdq,
#ifdef CONFIG_LENOVO_CUSTOM_LCM_FEATURE
	.set_cabcmode = lcm_set_cabcmode,
#endif
#ifdef CONFIG_LENOVO_PANELMODE_SUPPORT
	.set_hbm = lcm_set_hbm,
#endif
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
