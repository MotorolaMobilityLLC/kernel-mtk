#ifdef BUILD_LK
#include <upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#include <linux/i2c-dev.h>
#include "lcm_i2c.h"

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin(v))
#define MDELAY(n) (lcm_util.mdelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define FRAME_WIDTH     (1080)
#define FRAME_HEIGHT    (2460)

#define REGFLAG_DELAY           0xFFFC
#define REGFLAG_END_OF_TABLE    0xFFFD

/* i2c control start */
#define LCM_I2C_ID_NAME "i2c_lcd_bias"

static int _lcm_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id);
static int _lcm_i2c_remove(struct i2c_client *client);

static const struct of_device_id _lcm_i2c_of_match[] = {
	{.compatible = "mediatek,i2c_lcd_bias",},
	{},
};

static const struct i2c_device_id _lcm_i2c_id[] = {
	{LCM_I2C_ID_NAME,0},
	{},
};

static struct i2c_driver _lcm_i2c_driver = {
	.id_table = _lcm_i2c_id,
	.probe = _lcm_i2c_probe,
	.remove = _lcm_i2c_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = LCM_I2C_ID_NAME,
		.of_match_table = _lcm_i2c_of_match,
	},
};

static int _lcm_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	pr_err("[LCM][I2C]%s", __func__);
	_lcm_i2c_client = client;
	return 0;
}

static int _lcm_i2c_remove(struct i2c_client *client)
{
	pr_err("[LCM][I2C]%s", __func__);
	_lcm_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

static int _lcm_i2c_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = _lcm_i2c_client;
	char write_data[2] = {0};

	if(client == NULL)
	{
		pr_debug("[LCM][I2C] i2c_client is null\n");
		return 0;
	}
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if(ret < 0)
		pr_debug("[LCM][I2C] i2c_write data fail!\n");
	return ret;
}

static int __init _lcm_i2c_init(void)
{
	pr_err("[LCM][I2C]%s\n", __func__);
	i2c_add_driver(&_lcm_i2c_driver);
	pr_err("[LCM][I2C]%s success\n", __func__);
	return 0;
}

static void __exit _lcm_i2c_exit(void)
{
	pr_debug("[LCM][I2C]%s\n", __func__);
	i2c_del_driver(&_lcm_i2c_driver);
}

module_init(_lcm_i2c_init);
module_exit(_lcm_i2c_exit);
/* i2c control end */

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting_cmd[] = {
{0xFF,3,{0x78,0x07,0x01} },
{0x00,1,{0x44} },		//0226 blue
{0x01,1,{0x01} },		//0226 blue
{0x02,1,{0x27} },       //29
{0x03,1,{0x2C} },  		//0226 blue
{0x04,1,{0x00} },
{0x05,1,{0x00} },
{0x06,1,{0x00} },
{0x07,1,{0x00} },
{0x08,1,{0x82} },		//0226 blue
{0x09,1,{0x04} },
{0x0a,1,{0x30} },
{0x0b,1,{0x00} },
{0x0c,1,{0x00} },
{0x0d,1,{0x00} },
{0x0e,1,{0x09} },

{0x31,1,{0x07} },		//GOUTR01      DUMMY
{0x32,1,{0x07} },       //GOUTR02      DUMMY
{0x33,1,{0x07} },       //GOUTR03      DUMMY
{0x34,1,{0x07} },       //GOUTR04      DUMMY
{0x35,1,{0x07} },       //GOUTR05      DUMMY
{0x36,1,{0x07} },       //GOUTR06      DUMMY
{0x37,1,{0x2e} },       //GOUTR07      CKHR
{0x38,1,{0x2f} },       //GOUTR08      CKHG
{0x39,1,{0x30} },       //GOUTR09      CKHB
{0x3a,1,{0x2e} },       //GOUTR10      CKHR
{0x3b,1,{0x2f} },       //GOUTR11      CKHG
{0x3c,1,{0x30} },       //GOUTR12      CKHB
{0x3d,1,{0x40} },       //GOUTR13      VCOMSW
{0x3e,1,{0x08} },       //GOUTR14      STV_L
{0x3f,1,{0x13} },       //GOUTR15      CKV2_L
{0x40,1,{0x11} },       //GOUTR16      CKV4_L
{0x41,1,{0x12} },       //GOUTR17      CKV1_L
{0x42,1,{0x10} },       //GOUTR18      CKV3_L
{0x43,1,{0x00} },       //GOUTR19      D2U
{0x44,1,{0x01} },       //GOUTR20      U2D
{0x45,1,{0x07} },       //GOUTR21      DUMMY
{0x46,1,{0x07} },       //GOUTR22      DUMMY
{0x47,1,{0x28} },       //GOUTR23      GAS_L
{0x48,1,{0x2c} },       //GOUTR24      GRESET_L

{0x49,1,{0x07} },       //GOUTL01      DUMMY
{0x4a,1,{0x07} },       //GOUTL02      DUMMY
{0x4b,1,{0x07} },       //GOUTL03      DUMMY
{0x4c,1,{0x07} },       //GOUTL04      DUMMY
{0x4d,1,{0x07} },       //GOUTL05      DUMMY
{0x4e,1,{0x07} },       //GOUTL06      DUMMY
{0x4f,1,{0x2e} },       //GOUTL07      CKHR
{0x50,1,{0x2f} },       //GOUTL08      CKHG
{0x51,1,{0x30} },       //GOUTL09      CKHB
{0x52,1,{0x2e} },       //GOUTL10      CKHR
{0x53,1,{0x2f} },       //GOUTL11      CKHG
{0x54,1,{0x30} },       //GOUTL12      CKHB
{0x55,1,{0x40} },       //GOUTL13      VCOMSW
{0x56,1,{0x08} },       //GOUTL14      STV_R
{0x57,1,{0x13} },       //GOUTL15      CKV2_R
{0x58,1,{0x11} },       //GOUTL16      CKV4_R
{0x59,1,{0x12} },       //GOUTL17      CKV1_R
{0x5a,1,{0x10} },       //GOUTL18      CKV3_R
{0x5b,1,{0x00} },       //GOUTL19      D2U
{0x5c,1,{0x01} },       //GOUTL20      U2D
{0x5d,1,{0x07} },       //GOUTL21      DUMMY
{0x5e,1,{0x07} },       //GOUTL22      DUMMY
{0x5f,1,{0x28} },       //GOUTL23      GAS_R
{0x60,1,{0x2c} },       //GOUTL24      GRESET_R

{0x61,1,{0x07} },       //GOUTR01      DUMMY
{0x62,1,{0x07} },       //GOUTR02      DUMMY
{0x63,1,{0x07} },       //GOUTR03      DUMMY
{0x64,1,{0x07} },       //GOUTR04      DUMMY
{0x65,1,{0x07} },       //GOUTR05      DUMMY
{0x66,1,{0x07} },       //GOUTR06      DUMMY
{0x67,1,{0x2e} },       //GOUTR07      CKHR
{0x68,1,{0x2f} },       //GOUTR08      CKHG
{0x69,1,{0x30} },       //GOUTR09      CKHB
{0x6a,1,{0x2e} },       //GOUTR10      CKHR
{0x6b,1,{0x2f} },       //GOUTR11      CKHG
{0x6c,1,{0x30} },       //GOUTR12      CKHB
{0x6d,1,{0x40} },       //GOUTR13      VCOMSW
{0x6e,1,{0x08} },       //GOUTR14      STV_L
{0x6f,1,{0x12} },       //GOUTR15      CKV2_L
{0x70,1,{0x10} },       //GOUTR16      CKV4_L
{0x71,1,{0x13} },       //GOUTR17      CKV1_L
{0x72,1,{0x11} },       //GOUTR18      CKV3_L
{0x73,1,{0x00} },       //GOUTR19      D2U
{0x74,1,{0x01} },       //GOUTR20      U2D
{0x75,1,{0x07} },       //GOUTR21      DUMMY
{0x76,1,{0x07} },       //GOUTR22      DUMMY
{0x77,1,{0x28} },       //GOUTR23      GAS_L
{0x78,1,{0x2c} },       //GOUTR24      GRESET_L

{0x79,1,{0x07} },        //GOUTL01     DUMMY
{0x7a,1,{0x07} },        //GOUTL02     DUMMY
{0x7b,1,{0x07} },        //GOUTL03     DUMMY
{0x7c,1,{0x07} },        //GOUTL04     DUMMY
{0x7d,1,{0x07} },        //GOUTL05     DUMMY
{0x7e,1,{0x07} },        //GOUTL06     DUMMY
{0x7f,1,{0x2e} },        //GOUTL07     CKHR
{0x80,1,{0x2f} },        //GOUTL08     CKHG
{0x81,1,{0x30} },        //GOUTL09     CKHB
{0x82,1,{0x2e} },        //GOUTL10     CKHR
{0x83,1,{0x2f} },        //GOUTL11     CKHG
{0x84,1,{0x30} },        //GOUTL12     CKHB
{0x85,1,{0x40} },        //GOUTL13     VCOMSW
{0x86,1,{0x08} },        //GOUTL14     STV_R
{0x87,1,{0x12} },        //GOUTL15     CKV2_R
{0x88,1,{0x10} },        //GOUTL16     CKV4_R
{0x89,1,{0x13} },        //GOUTL17     CKV1_R
{0x8a,1,{0x11} },        //GOUTL18     CKV3_R
{0x8b,1,{0x00} },        //GOUTL19     D2U
{0x8c,1,{0x01} },        //GOUTL20     U2D   
{0x8d,1,{0x07} },        //GOUTL21     DUMMY
{0x8e,1,{0x07} },        //GOUTL22     DUMMY
{0x8f,1,{0x28} },        //GOUTL23     GAS_R
{0x90,1,{0x2c} },        //GOUTL24     GRESET_R

{0xB2,1,{0x01} },        //U2D,{0xD2U TP_VGLO 0323
{0xc0,1,{0x07} },
{0xc1,1,{0x00} },        //80  0326
{0xc2,1,{0x00} },
{0xc3,1,{0x50} },
{0xc4,1,{0x00} },
{0xc5,1,{0x2b} },
{0xc6,1,{0x28} },
{0xc7,1,{0x28} },
{0xc8,1,{0x00} },
{0xc9,1,{0x00} },
{0xca,1,{0x01} },
{0xd0,1,{0x01} },
{0xd1,1,{0x12} },
{0xd2,1,{0x00} },
{0xd3,1,{0x41} },
{0xd4,1,{0x04} },
{0xd5,1,{0x22} },
{0xd6,1,{0x41} },
{0xd7,1,{0x00} },
{0xd8,1,{0x60} },
{0xd9,1,{0xa5} },
{0xda,1,{0x00} },
{0xdb,1,{0x00} },
{0xdc,1,{0x00} },
{0xdd,1,{0x18} },
{0xde,1,{0x01} },
{0xdf,1,{0x60} },   	//0319  sleep in                                     
{0xe0,1,{0x08} },
{0xe1,1,{0x00} },
{0xe2,1,{0x03} },
{0xe3,1,{0x00} },
{0xe4,1,{0x21} },		//0226 blue
{0xe5,1,{0x0f} },
{0xe6,1,{0x22} },

{0xee,1,{0x01} },   	//0319 sleep in

{0xFF,3,{0x78,0x07,0x11} },
{0x00,1,{0x00} },       //0226 blue
{0x01,1,{0x09} },      	//0226 blue 0C

{0xFF,3,{0x78,0x07,0x02} },
{0x36,1,{0x00} },

{0x40,1,{0x00} },      	//0225 blue		//120_t8_de  00  0320
{0x41,1,{0x00} },      	//0225 blue		//120_t7p_de     
{0x42,1,{0x09} },      	//0225 blue		//120_t9_de  08  0320
{0x43,1,{0x34} },      	//0225 blue		//120_t7_de  18 16 0320
{0x75,1,{0x00} },
{0x46,1,{0x42} },      	//DUMMY CKH
{0x47,1,{0x03} },		//CKH connect
{0x01,1,{0x55} },       //time out GIP toggle		
{0x81,1,{0x33} },
{0x82,1,{0x02} },		//save power VBP 			
{0x1B,1,{0x02} },      	//Frame Rate 60HZ
{0x06,1,{0x6A} }, 		//60Hz BIST RTNA 6.6usec 62.5nsec/CLK
{0x0E,1,{0x3E} }, 		//60Hz BIST VBP  62
{0x0F,1,{0x3C} }, 		//60Hz BIST VFP  60

{0xFF,3,{0x78,0x07,0x12} },
{0x10,1,{0x01} },      	//0225 blue		//120_t8_de  00  0320
{0x11,1,{0x00} },      	//0225 blue		//120_t7p_de     
{0x12,1,{0x09} },      	//0225 blue		//120_t9_de  08  0320
{0x13,1,{0x14} },      	//0225 blue		//120_t7_de  18 16 0320

{0x1E,1,{0x01} },       //0320 CKH EQ
{0x16,1,{0x0B} },       //0402 SDT
{0x17,1,{0x00} },       //0402 eq
{0x1A,1,{0x13} },		//save power SRC bias through rate
{0x1B,1,{0x22} },		//save power SRC bias current

{0x06,1,{0xD6} }, 		//60 Hz BIST RTN
{0x0E,1,{0x1A} }, 		//60 Hz BIST VBP
{0x0F,1,{0x25} }, 		//60 Hz BIST VFP

{0xFF,3,{0x78,0x07,0x04} },
{0xB7,1,{0xCF} },
{0xB8,1,{0x45} },
{0xBA,1,{0x74} },

{0xFF,3,{0x78,0x07,0x05} },
{0x1B,1,{0x00} },
{0x1C,1,{0x88} },		//VCOM=-0.1V 8E
{0x61,1,{0xCB} },
{0x64,1,{0xEF} },		//[7]=VGH_CLP[4]:VGH_RATIO 2X
{0x6E,1,{0x3F} },		//[6]=VGL_RATIO
{0x69,1,{0x02} },		//VGH short VGHO
{0x70,1,{0xC8} },		//[3]=VGL_CLP
{0x72,1,{0x6A} },		//VGH=10V
{0x73,1,{0x1C} },		//OTP VGH=6.1V
{0x74,1,{0x55} },		//VGL=-8.8V
{0x76,1,{0x79} },		//VGHO=10V
{0x7A,1,{0x42} },		//VGLO=-7V
{0x7B,1,{0x6A} },		//GVDDP=4.6V
{0x7C,1,{0x6A} },		//GVDDN=-4.6V
{0xB5,1,{0x50} },       //PWR_D2A_HVREG_VGHO_EN
{0xB7,1,{0x70} },       //PWR_D2A_HVREG_VGLO_EN
{0xC9,1,{0x90} },       //0327 power off

{0xFF,3,{0x78,0x07,0x06} },
{0x3E,1,{0xE2} },  	//11 dont reload otp
{0xC0,1,{0x9C} },		//Res=1080*2408
{0xC1,1,{0x19} },		//Res=1080*2408
{0xC3,1,{0x06} }, 		//ss_reg

{0xD6,1,{0x00} },
{0xCD,1,{0x00} }, 		//FTE=TSHD,{0xFTE1=TSVD1
{0x96,1,{0x50} },		//save power mipi bais

{0xFF,3,{0x78,0x07,0x07} },

{0x29,1,{0xCE} },

{0xFF,3,{0x78,0x07,0x08} },							
{0xE0,31,{0x00,0x00,0x19,0x38,0x00,0x62,0x86,0xA7,0x14,0xDB,0x04,0x42,0x15,0x75,0xC1,0xFD,0x2A,0x3B,0x7F,0xAB,0x3E,0xE4,0x0D,0x3A,0x3F,0x55,0x80,0xB8,0x0F,0xD5,0xD9} },
{0xE1,31,{0x00,0x00,0x19,0x38,0x00,0x62,0x86,0xA7,0x14,0xDB,0x04,0x42,0x15,0x75,0xC1,0xFD,0x2A,0x3B,0x7F,0xAB,0x3E,0xE4,0x0D,0x3A,0x3F,0x55,0x80,0xB8,0x0F,0xD5,0xD9} },


{0xFF,3,{0x78,0x07,0x0B} },		// autotrim
{0x94,1,{0x88} },
{0x95,1,{0x1D} },
{0x96,1,{0x06} },
{0x97,1,{0x06} },
{0x98,1,{0xCB} },
{0x99,1,{0xCB} },
{0x9A,1,{0x06} },
{0x9B,1,{0xC8} },
{0x9C,1,{0x05} },
{0x9D,1,{0x05} },
{0x9E,1,{0xA9} },
{0x9F,1,{0xA9} },
{0xAB,1,{0xE0} },

{0xFF,3,{0x78,0x07,0x0C} },		//TP Modulation
{0x80,1,{0x2B} },
{0x81,1,{0xCA} },
{0x82,1,{0x2A} },
{0x83,1,{0xC3} },
{0x84,1,{0x2A} },
{0x85,1,{0xC3} },
{0x86,1,{0x29} },
{0x87,1,{0xBE} },
{0x88,1,{0x29} },
{0x89,1,{0xBF} },
{0x8A,1,{0x2B} },
{0x8B,1,{0xCB} },
{0x8C,1,{0x2A} },
{0x8D,1,{0xC9} },
{0x8E,1,{0x28} },
{0x8F,1,{0xB1} },
{0x90,1,{0x2A} },
{0x91,1,{0xC2} },
{0x92,1,{0x2A} },
{0x93,1,{0xC1} },
{0x94,1,{0x29} },
{0x95,1,{0xBC} },
{0x96,1,{0x2A} },
{0x97,1,{0xC4} },
{0x98,1,{0x2A} },
{0x99,1,{0xC8} },
{0x9A,1,{0x28} },
{0x9B,1,{0xB0} },
{0x9C,1,{0x29} },
{0x9D,1,{0xB8} },
{0x9E,1,{0x2A} },
{0x9F,1,{0xC6} },
{0xA0,1,{0x29} },
{0xA1,1,{0xBD} },
{0xA2,1,{0x28} },
{0xA3,1,{0xB2} },
{0xA4,1,{0x28} },
{0xA5,1,{0xAD} },
{0xA6,1,{0x29} },
{0xA7,1,{0xB7} },
{0xA8,1,{0x28} },
{0xA9,1,{0xB5} },
{0xAA,1,{0x29} },
{0xAB,1,{0xB6} },
{0xAC,1,{0x28} },
{0xAD,1,{0xB3} },
{0xAE,1,{0x2A} },
{0xAF,1,{0xC0} },
{0xB0,1,{0x29} },
{0xB1,1,{0xB9} },
{0xB2,1,{0x28} },
{0xB3,1,{0xAF} },
{0xB4,1,{0x28} },
{0xB5,1,{0xAB} },
{0xB6,1,{0x2A} },
{0xB7,1,{0xC7} },
{0xB8,1,{0x28} },
{0xB9,1,{0xB4} },
{0xBA,1,{0x29} },
{0xBB,1,{0xBB} },
{0xBC,1,{0x28} },
{0xBD,1,{0xAE} },
{0xBE,1,{0x28} },
{0xBF,1,{0xAC} },

{0xFF,3,{0x78,0x07,0x0E} },
{0x00,1,{0xA3} }, 		//LH mode
{0x02,1,{0x0F} },		
{0x04,1,{0x07} }, 		//TSHD_1VP on & TSVD free run
{0xB9,1,{0xCC} }, 		//TSHD PS off

//*********** 60 hz table**************
{0x40,1,{0x07} }, 		//8 unit
{0x47,1,{0x10} }, 		//Unit line=301
{0x49,1,{0x2D} }, 		//Unit line=301
{0x45,1,{0x0A} }, 		//TP2_unit0=174.8us
{0x46,1,{0xEC} }, 		//TP2_unit0=174.8us
{0x4D,1,{0xC2} }, 		//RTN 6.09usec
{0x1C,1,{0x27} }, 		//wait line
{0x1D,1,{0xF5} }, 		//unit0_INT_line_STEP
{0xC0,1,{0x34} }, 		//TP3 unit
{0x21,1,{0x85} }, 		//TSVD1 rising position shift
{0x23,1,{0x85} }, 		//TSVD2 rising position shift
{0x2B,1,{0x1F} },
{0x05,1,{0x20} },
{0xB0,1,{0x31} },       //TP1 CKH 0406
{0x41,1,{0x44} }, 		//60hz TSVD position
{0x4B,1,{0x1F} },
            
{0xC6,1,{0x61} }, 		//60Hz TP3-4
{0xC7,1,{0x61} }, 		//60Hz TP3-3
{0xC8,1,{0x61} }, 		//60Hz TP3-2
{0xC9,1,{0x61} }, 		//60Hz TP3-1

{0xFF,3,{0x78,0x07,0x00}},	//Page0
{0x11,1,{0x00}},
{REGFLAG_DELAY,120,{}},
{0x29,1,{0x00}}, 
{REGFLAG_DELAY,20,{}},
{0x35,1,{0x00}},
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
				MDELAY(table[i].count);
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd,
				table[i].count,
				table[i].para_list,
				force_update);
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	
	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 46;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 20;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 510;
	params->physical_height = 161;
	params->physical_width = 70;

	params->density = 480;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
}

static void lcm_init_power(void)
{
	lcm_util.set_lcd_ldo_en(1);
	if(lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(1);
		_lcm_i2c_write_bytes(0x00, 0xf);
		_lcm_i2c_write_bytes(0x01, 0xf);
	}
	else
		pr_debug("[LCM]set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	if(lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(0);
	}
}

static void lcm_resume_power(void)
{
	SET_RESET_PIN(0);
	lcm_init_power();
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	push_table(NULL, init_setting_cmd,
			ARRAY_SIZE(init_setting_cmd), 1);
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting,
			ARRAY_SIZE(lcm_suspend_setting), 1);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

extern int chargepump_set_backlight_level(int level);
static void lcm_setbacklight(/*void *handle, */unsigned int level)
{
	pr_debug("lcm_setbacklight.");
	chargepump_set_backlight_level(level);
}

struct LCM_DRIVER mipi_mot_vid_tianma_ili7807s_fhd_678_lcm_drv = {
	.name = "mipi_mot_vid_tianma_ili7807s_fhd_678",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight = lcm_setbacklight,
//	.set_backlight_cmdq = lcm_setbacklight_cmdq,
};
