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
//Lenovo-sw wuwl10 add 20151019 for backlight begin
#ifdef CONFIG_BACKLIGHTIC_KTD3116_CURRENT
static bool need_config_20ma = true;
#endif
//Lenovo-sw wuwl10 add 20151019 for backlight end
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

#define LCM_ID_NT35596 (0x96)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
//lenovo-sw wuwl10 20150820 add for ms delay
//#define MSDELAY(n) (lcm_util.ms_delay(n))
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
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
#if 0
#ifndef BUILD_LK
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
/***************************************************************************** 
 * Define
 *****************************************************************************/

#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x36

/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;


/***************************************************************************** 
 * Function Prototype
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/***************************************************************************** 
 * Data Structure
 *****************************************************************************/

 struct tps65132_dev	{	
	struct i2c_client	*client;
	
};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tps65132",
	},
 
};

/***************************************************************************** 
 * Function
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{  
	printk( "tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client  = client;		
	return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
  printk( "tps65132_remove\n");
  tps65132_i2c_client = NULL;
   i2c_unregister_device(client);
  return 0;
}


 static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2]={0};	
	write_data[0]= addr;
	write_data[1] = value;
    ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
	printk("tps65132 write data fail !!\n");	
	return ret ;
}

/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

   printk( "tps65132_iic_init\n");
   i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
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

#define TPS65132_SLAVE_ADDR_WRITE  0x6C
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
#endif
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
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
	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00043902;
	data_array[1] = 0x010119FF;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00033902;
	data_array[1] = 0x000119FF;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x331C1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xA0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0xE8C11500;
	dsi_set_cmdq(data_array, 1, 1);
//lenovo-sw wuwl10 add for pwm freq 2.57k begin
	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x34CA1500;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0xB1001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x34CA1500;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0xB3001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x48CA1500;
	dsi_set_cmdq(data_array, 1, 1);
//lenovo-sw wuwl10 add for pwm freq 2.57k end
	data_array[0] = 0xA7001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00C11500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00073902;
	data_array[1] = 0x002F00C0;
	data_array[2] = 0x00010000;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0xC0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00073902;
	data_array[1] = 0x002F00C0;
	data_array[2] = 0x00010000;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x9A001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1EC01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xAC001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x06C01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xDC001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x06C01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x81001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x04A51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x84001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x20C41500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xA5001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x1DB31500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x92001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00E91500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x01F31500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xF7001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00053902;
	data_array[1] = 0x041804C3;
	data_array[2] = 0x00000004;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0xB4001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x80C01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x93001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x19C51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x95001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x2DC51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x97001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x14C51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x99001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x28C51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00033902;
	data_array[1] = 0x002323D8;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E1;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6E757D7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0B101A23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E2;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6E757D7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0B101A23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E3;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6E757D7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0B101A23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E4;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6E757D7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0B101A23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E5;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6E757D7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0B101A23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00193902;
	data_array[1] = 0x0B0600E6;
	data_array[2] = 0x33261E15;
	data_array[3] = 0x74685347;
	data_array[4] = 0x6D757C7C;
	data_array[5] = 0x2A36485B;
	data_array[6] = 0x0C111B23;
	data_array[7] = 0x00000003;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00D91500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x01001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0xB4D91500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x81001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x07A51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x9D001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x77C51500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xB3001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0xCCC01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xBC001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00C01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000F3902;
	data_array[1] = 0x008700C0;
	data_array[2] = 0x87000A0A;
	data_array[3] = 0x87000A0A;
	data_array[4] = 0x000A0A00;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xF0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x22C31500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xA0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00083902;
	data_array[1] = 0x000000C0;
	data_array[2] = 0x03220300;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0xD0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00083902;
	data_array[1] = 0x000000C0;
	data_array[2] = 0x03220300;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x000183C2;
	data_array[2] = 0x00018200;
	data_array[3] = 0x00000000;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000D3902;
	data_array[1] = 0x030282C3;
	data_array[2] = 0x81840300;
	data_array[3] = 0x03000303;
	data_array[4] = 0x00000084;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000D3902;
	data_array[1] = 0x030100C3;
	data_array[2] = 0x01840300;
	data_array[3] = 0x03000302;
	data_array[4] = 0x00000084;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00113902;
	data_array[1] = 0x110A09CC;
	data_array[2] = 0x15141312;
	data_array[3] = 0x28181716;
	data_array[4] = 0x28282828;
	data_array[5] = 0x00000000;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00113902;
	data_array[1] = 0x14090ACC;
	data_array[2] = 0x15111213;
	data_array[3] = 0x28181716;
	data_array[4] = 0x28282828;
	data_array[5] = 0x00000000;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0xA0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00113902;
	data_array[1] = 0x1F1E1DCC;
	data_array[2] = 0x1C1B1A19;
	data_array[3] = 0x23222120;
	data_array[4] = 0x27262524;
	data_array[5] = 0x00000000;
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x030201CC;
	data_array[2] = 0x04070605;
	data_array[3] = 0x00000008;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0xC0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000D3902;
	data_array[1] = 0x000000CC;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000077;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xD0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000D3902;
	data_array[1] = 0x000000CC;
	data_array[2] = 0x00000500;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000077;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xA0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0x010101CB;
	data_array[2] = 0x00010101;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0xFD0100CB;
	data_array[2] = 0x00000000;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xC0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x77000000;
	data_array[3] = 0x00000077;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0xD0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x77000000;
	data_array[3] = 0x00000077;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0xE0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x000000CB;
	data_array[2] = 0x77000000;
	data_array[3] = 0x00000077;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0xF0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00093902;
	data_array[1] = 0x010101CB;
	data_array[2] = 0x77000000;
	data_array[3] = 0x00000077;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0x3F3F3FCD;
	data_array[2] = 0x3F3F3F3F;
	data_array[3] = 0x12023F3F;
	data_array[4] = 0x3F043F11;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0x90001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000C3902;
	data_array[1] = 0x3F3F06CD;
	data_array[2] = 0x21262626;
	data_array[3] = 0x26261F20;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0xA0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00103902;
	data_array[1] = 0x3F3F3FCD;
	data_array[2] = 0x3F3F3F3F;
	data_array[3] = 0x12013F3F;
	data_array[4] = 0x3F033F11;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0] = 0xB0001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x000C3902;
	data_array[1] = 0x3F3F05CD;
	data_array[2] = 0x21262626;
	data_array[3] = 0x26261F20;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x9B001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00033902;
	data_array[1] = 0x005555C5;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x80001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x15C41500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00001500;
	dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00043902;
	data_array[1] = 0xFFFFFFFF;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00511500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x24531500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x02551500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(150);
	    	
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

}

//Lenovo-sw wuwl10 add 20151019 for backlight begin
#ifdef CONFIG_BACKLIGHTIC_KTD3116_CURRENT
static void ktd3117_set_bit0(void)
{
	mt_set_gpio_out_base(11, GPIO_OUT_ZERO);
	UDELAY(15);
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	UDELAY(5);
}
static void ktd3117_set_bit1(void)
{
	mt_set_gpio_out_base(11, GPIO_OUT_ZERO);
	UDELAY(5);
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	UDELAY(15);
}
static void ktd3117_set_start(void)
{
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	UDELAY(15);
}
static void ktd3117_set_end(void)
{
	mt_set_gpio_out_base(11, GPIO_OUT_ZERO);
	UDELAY(15);
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	UDELAY(500);
}

static void ktd3117_set_data(unsigned int data)
{
	unsigned int i;
	unsigned long flags;
	data = data & 0x3F;

	printk(KERN_INFO "%s data = 0x%x\n ", __func__, data);
	ktd3117_set_start();

	local_irq_save(flags);
	for (i = 8; i > 0; ){ //MSB first
		i --;
		if ((data >> i) &0x01){
			ktd3117_set_bit1();
		}
		else{
			ktd3117_set_bit0();
		}
	}
	local_irq_restore(flags);
	ktd3117_set_end();
}
static void  backlightic_ktd3117_onewire_scale(unsigned int level)
{
	mt_set_gpio_mode_base(11, GPIO_MODE_00);
	mt_set_gpio_dir_base(11, GPIO_DIR_OUT);

	if (( level > 4) && need_config_20ma){
		ktd3117_set_data(0x00);//20mA  15nits
		need_config_20ma = false;
	}else if (level == 4){
		need_config_20ma = true;
		ktd3117_set_data(0x0C);//15mA --12nits
	}else if (level == 3){
		need_config_20ma = true;
		ktd3117_set_data(0x18);//10mA --8.5nits
	}else if (level == 2){
		need_config_20ma = true;
		ktd3117_set_data(0x20);//6.7mA --5.8nits
	}else if (level == 1){
		need_config_20ma = true;
		ktd3117_set_data(0x30);//3.3mA --2.8nits
	}else if (level == 0){
		need_config_20ma = true;
		//ktd3117_set_data(0x30);//3.3mA
	}
}
 // EXPORT_SYMBOL_GPL(backlightic_ktd3117_onewire_scale);
#endif
//Lenovo-sw wuwl10 add 20151019 for backlight end

//lenovo-sw wuwl10 20150515 add for esd revovery backlight begin
#ifndef BUILD_LK
static void lcm_esd_recover_backlight(void)
{

	printk("%s tm, level = %d\n", __func__, esd_last_backlight_level);

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = esd_last_backlight_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif
//lenovo-sw wuwl10 20150515 add for esd revovery backlight end
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
#ifdef BUILD_LK
	dprintf(0,"%s,lk otm1901 tm backlight: level = %d\n", __func__, level);
#else

#ifdef CONFIG_BACKLIGHTIC_KTD3116_CURRENT
	backlightic_ktd3117_onewire_scale(level);
#endif
	printk(KERN_INFO "%s, kernel nt35596 tm backlight: level = %d\n", __func__, level);
	if((0 < level) && (level < 5))
	{
		level = 5;
	}
//Lenovo-sw wuwl10 add 20150515 for esd recover backlight
	esd_last_backlight_level = level;
#endif
	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = level;
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
	params->dsi.vertical_backporch = 30;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 50;//wuwl10 modify for new param
	params->dsi.horizontal_frontporch = 50;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	//params->dsi.pll_select=1;     //0: MIPI_PLL; 1: LVDS_PLL
	// Bit rate calculation
	//1 Every lane speed
	params->dsi.PLL_CLOCK = 460;//wuwl10 modify for new param
	//params->dsi.clk_lp_per_line_enable= 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.ssc_disable = 1;
}

static void lcm_init(void)
{
	mt_set_gpio_mode_base(11, GPIO_MODE_00);
	mt_set_gpio_dir_base(11, GPIO_DIR_OUT);
	mt_set_gpio_out_base(11, GPIO_OUT_ONE);
	MDELAY(1);

	SET_RESET_PIN(0);
	MDELAY(3);
	//set avdd enable

	set_gpio_lcd_enp(1);
	MDELAY(9);
	
	//set avee enable
	set_gpio_lcd_enn(1);
	MDELAY(12);

	SET_RESET_PIN(1);
	MDELAY(2);
	SET_RESET_PIN(0);
	MDELAY(3);

	SET_RESET_PIN(1);
	MDELAY(9);
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
	
	//set reset disable
	SET_RESET_PIN(0);
	MDELAY(1);
	//set avee enable
	set_gpio_lcd_enn(0);
	MDELAY(2);

	//set avdd disable
	set_gpio_lcd_enp(0);
	MDELAY(3);

}


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

const static unsigned char LCD_MODULE_ID = 0x01;
static unsigned int lcm_compare_id(void)
{
	unsigned char  id_pin_read = 0;

	//mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
	//mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
	//id_pin_read = mt_get_gpio_in(GPIO_DISP_ID0_PIN);

#ifdef BUILD_LK
	dprintf(0, "%s,  id otm1901 tm= 0x%x LCD_ID_value=%d \n", __func__, id,id_pin_read);
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

LCM_DRIVER otm1901_fhd_dsi_vdo_boe_lcm_drv = {
	.name = "otm1901_fhd_dsi_vdo_boe",
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
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
