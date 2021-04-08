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

#include "mtkfb_params.h"

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
	{0xFF, 3, {0x78, 0x07, 0x00} },	//Page0
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 60, {} },
	{0xFF, 3, {0x78, 0x07, 0x06} },
	{0x5F, 1, {0x24} },
	{0x4E, 1, {0x60} },
	{0x4D, 1, {0x01} },
	{0x48, 1, {0x09} },

	{0xFF, 3, {0x78,0x07,0x01}},
	{0x00, 1, {0x44} },			//0226 blue
	{0x01, 1, {0x03} },			//0226 blue
	{0x02, 1, {0x27} },			//29
	{0x03, 1, {0x2C} }, 		//0226 blue
	{0x04, 1, {0x00} },
	{0x05, 1, {0x00} },
	{0x06, 1, {0x00} },
	{0x07, 1, {0x00} },
	{0x08, 1, {0x82} },			//0226 blue
	{0x09, 1, {0x04} },
	{0x0a, 1, {0x30} },
	{0x0b, 1, {0x00} },
	{0x0c, 1, {0x00} },
	{0x0d, 1, {0x00} },
	{0x0e, 1, {0x09} },

	{0x31, 1, {0x07} },			//FGOUTR01      DUMMY
	{0x32, 1, {0x07} },			//FGOUTR02      DUMMY
	{0x33, 1, {0x07} },			//FGOUTR03      DUMMY
	{0x34, 1, {0x34} },			//FGOUTR04      CKHR  1106
	{0x35, 1, {0x35} },			//FGOUTR05      CKHG  1106
	{0x36, 1, {0x36} },			//FGOUTR06      CKHB  1106
	{0x37, 1, {0x07} },			//FGOUTR07      DUMMY
	{0x38, 1, {0x07} },			//FGOUTR08      DUMMY
	{0x39, 1, {0x07} },			//FGOUTR09      DUMMY
	{0x3a, 1, {0x2E} },			//FGOUTR10      CKH1R  1102
	{0x3b, 1, {0x2F} },			//FGOUTR11      CKH1G  1102
	{0x3c, 1, {0x30} },			//FGOUTR12      CKH1B  1102
	{0x3d, 1, {0x40} },			//FGOUTR13      VCOMSW
	{0x3e, 1, {0x08} },			//FGOUTR14      STV_L
	{0x3f, 1, {0x13} },			//FGOUTR15      CKV2_L
	{0x40, 1, {0x11} },			//FGOUTR16      CKV4_L
	{0x41, 1, {0x12} },			//FGOUTR17      CKV1_L
	{0x42, 1, {0x10} },			//FGOUTR18      CKV3_L
	{0x43, 1, {0x00} },			//FGOUTR19      D2U
	{0x44, 1, {0x01} },			//FGOUTR20      U2D
	{0x45, 1, {0x07} },			//FGOUTR21      DUMMY
	{0x46, 1, {0x07} },			//FGOUTR22      DUMMY
	{0x47, 1, {0x28} },			//FGOUTR23      GAS_L
	{0x48, 1, {0x2c} },			//FGOUTR24      GRESET_L

	{0x49, 1, {0x07} },			//FGOUTL01      DUMMY
	{0x4a, 1, {0x07} },			//FGOUTL02      DUMMY
	{0x4b, 1, {0x07} },			//FGOUTL03      DUMMY
	{0x4c, 1, {0x34} },			//FGOUTL04      CKHR
	{0x4d, 1, {0x35} },			//FGOUTL05      CKHG
	{0x4e, 1, {0x36} },			//FGOUTL06      CKHB
	{0x4f, 1, {0x07} },			//FGOUTL07      DUMMY
	{0x50, 1, {0x07} },			//FGOUTL08      DUMMY
	{0x51, 1, {0x07} },			//FGOUTL09      DUMMY
	{0x52, 1, {0x2E} },			//FGOUTL10      CKH1R  1102
	{0x53, 1, {0x2F} },			//FGOUTL11      CKH1G  1102
	{0x54, 1, {0x30} },			//FGOUTL12      CKH1B  1102
	{0x55, 1, {0x40} },			//FGOUTL13      VCOMSW
	{0x56, 1, {0x08} },			//FGOUTL14      STV_R
	{0x57, 1, {0x13} },			//FGOUTL15      CKV2_R
	{0x58, 1, {0x11} },			//FGOUTL16      CKV4_R
	{0x59, 1, {0x12} },			//FGOUTL17      CKV1_R
	{0x5a, 1, {0x10} },			//FGOUTL18      CKV3_R
	{0x5b, 1, {0x00} },			//FGOUTL19      D2U
	{0x5c, 1, {0x01} },			//FGOUTL20      U2D
	{0x5d, 1, {0x07} },			//FGOUTL21      DUMMY
	{0x5e, 1, {0x07} },			//FGOUTL22      DUMMY
	{0x5f, 1, {0x28} },			//FGOUTL23      GAS_R
	{0x60, 1, {0x2c} },			//FGOUTL24      GRESET_R

	{0x61, 1, {0x07} },			//BGOUTR01      DUMMY
	{0x62, 1, {0x07} },			//BGOUTR02      DUMMY
	{0x63, 1, {0x07} },			//BGOUTR03      DUMMY
	{0x64, 1, {0x34} },			//BGOUTR04      CKHR
	{0x65, 1, {0x35} },			//BGOUTR05      CKHG
	{0x66, 1, {0x36} },			//BGOUTR06      CKHB
	{0x67, 1, {0x07} },			//BGOUTR07      DUMMY
	{0x68, 1, {0x07} },			//BGOUTR08      DUMMY
	{0x69, 1, {0x07} },			//BGOUTR09      DUMMY
	{0x6a, 1, {0x2E} },			//BGOUTR10      CKH1R 1102
	{0x6b, 1, {0x2F} },			//BGOUTR11      CKH1G 1102
	{0x6c, 1, {0x30} },			//BGOUTR12      CKH1B 1102
	{0x6d, 1, {0x40} },			//BGOUTR13      VCOMSW
	{0x6e, 1, {0x08} },			//BGOUTR14      STV_L
	{0x6f, 1, {0x12} },			//BGOUTR15      CKV2_L
	{0x70, 1, {0x10} },			//BGOUTR16      CKV4_L
	{0x71, 1, {0x13} },			//BGOUTR17      CKV1_L
	{0x72, 1, {0x11} },			//BGOUTR18      CKV3_L
	{0x73, 1, {0x00} },			//BGOUTR19      D2U
	{0x74, 1, {0x01} },			//BGOUTR20      U2D
	{0x75, 1, {0x07} },			//BGOUTR21      DUMMY
	{0x76, 1, {0x07} },			//BGOUTR22      DUMMY
	{0x77, 1, {0x28} },			//BGOUTR23      GAS_L
	{0x78, 1, {0x2c} },			//BGOUTR24      GRESET_L

	{0x79, 1, {0x07} },			//BGOUTL01     DUMMY
	{0x7a, 1, {0x07} },			//BGOUTL02     DUMMY
	{0x7b, 1, {0x07} },			//BGOUTL03     DUMMY
	{0x7c, 1, {0x34} },			//BGOUTL04     CKHR
	{0x7d, 1, {0x35} },			//BGOUTL05     CKHG
	{0x7e, 1, {0x36} },			//BGOUTL06     CKHB
	{0x7f, 1, {0x07} },			//BGOUTL07     DUMMY
	{0x80, 1, {0x07} },			//BGOUTL08     DUMMY
	{0x81, 1, {0x07} },			//BGOUTL09     DUMMY
	{0x82, 1, {0x2E} },			//BGOUTL10     CKH1R  1102
	{0x83, 1, {0x2F} },			//BGOUTL11     CKH1G  1102
	{0x84, 1, {0x30} },			//BGOUTL12     CKH1B  1102
	{0x85, 1, {0x40} },			//BGOUTL13     VCOMSW
	{0x86, 1, {0x08} },			//BGOUTL14     STV_R
	{0x87, 1, {0x12} },			//BGOUTL15     CKV2_R
	{0x88, 1, {0x10} },			//BGOUTL16     CKV4_R
	{0x89, 1, {0x13} },			//BGOUTL17     CKV1_R
	{0x8a, 1, {0x11} },			//BGOUTL18     CKV3_R
	{0x8b, 1, {0x00} },			//BGOUTL19     D2U
	{0x8c, 1, {0x01} },			//BGOUTL20     U2D
	{0x8d, 1, {0x07} },			//BGOUTL21     DUMMY
	{0x8e, 1, {0x07} },			//BGOUTL22     DUMMY
	{0x8f, 1, {0x28} },			//BGOUTL23     GAS_R
	{0x90, 1, {0x2c} },			//BGOUTL24     GRESET_R

	{0xB2, 1, {0x01} },
	{0xB3, 1, {0x04} },			//04
	{0xc0, 1, {0x07} },			//07
	{0xc1, 1, {0x2E} },
	{0xc2, 1, {0x00} },
	{0xc3, 1, {0x50} },
	{0xc4, 1, {0x00} },
	{0xc5, 1, {0x2b} },
	{0xc6, 1, {0x28} },
	{0xc7, 1, {0x28} },
	{0xc8, 1, {0x00} },
	{0xc9, 1, {0x00} },
	{0xca, 1, {0x01} },
	{0xd0, 1, {0x01} },
	{0xd1, 1, {0x12} },
	{0xd2, 1, {0x00} },
	{0xd3, 1, {0x41} },
	{0xd4, 1, {0x04} },
	{0xd5, 1, {0x22} },
	{0xd6, 1, {0x41} },
	{0xd7, 1, {0x00} },
	{0xd8, 1, {0x60} },
	{0xd9, 1, {0xa5} },
	{0xda, 1, {0x00} },
	{0xdb, 1, {0x00} },
	{0xdc, 1, {0x00} },
	{0xdd, 1, {0x18} },
	{0xde, 1, {0x01} },
	{0xdf, 1, {0x60} },			//0319  sleep in
	{0xe0, 1, {0x08} },
	{0xe1, 1, {0x00} },
	{0xe2, 1, {0x03} },
	{0xe3, 1, {0x00} },
	{0xe4, 1, {0x21} },			//0226 blue
	{0xe5, 1, {0x0f} },
	{0xe6, 1, {0x22} },
	{0xee, 1, {0x01} },			//0319 sleep in

	{0xFF, 3, {0x78, 0x07, 0x11} },
	{0x00, 1, {0x00} },
	{0x01, 1, {0x09} },
	{0x18, 1, {0x2E} },

	{0xFF, 3, {0x78, 0x07, 0x02} },
	{0x40, 1, {0x0C} },			//60_t8_de
	{0x41, 1, {0x00} },			//60_t7p_de
	{0x42, 1, {0x0B} },			//60_t9_de
	{0x43, 1, {0x2E} },			//60_t7_de  1102

	{0x75, 1, {0x00} },
	{0x46, 1, {0x42} },			//DUMMY CKH
	{0x47, 1, {0x03} },			//CKH connect

	{0x4C, 1, {0x0C} },			//Advance CKH

	{0x01, 1, {0x55} },			//time out GIP toggle
	{0x81, 1, {0x33} },
	{0x82, 1, {0x32} },			//save power VBP
	{0x1B, 1, {0x02} },			//Frame Rate Ctrl
	{0x06, 1, {0x6A} },			//60Hz BIST RTNA 6.6usec 62.5nsec/CLK
	{0x0E, 1, {0x0E} },			//60Hz BIST VBP  62
	{0x0F, 1, {0x20} },			//60Hz BIST VFP  60

	{0xFF, 3, {0x78, 0x07,0x12} },
	{0x10, 1, {0x05} },			//0225 blue		//120_t8_de
	{0x11, 1, {0x00} },			//0225 blue		//120_t7p_de
	{0x12, 1, {0x08} },			//0225 blue		//120_t9_de
	{0x13, 1, {0x10} },			//0225 blue		//120_t7_de  12 1/11

	{0x1E, 1, {0x01} },			//0320 CKH EQ
	{0x16, 1, {0x09} },			//0402 SDT
	{0x17, 1, {0x00} },			//0402 eq
	{0x1A, 1, {0x13} },			//save power SRC bias through rate
	{0x1B, 1, {0x22} },			//save power SRC bias current
	{0xC0, 1, {0x35} },			//120 Hz BIST RTN
	{0xC2, 1, {0x0E} },			//120 Hz BIST VBP
	{0xC3, 1, {0x20} },			//120 Hz BIST VFP

	{0xFF, 3, {0x78, 0x07, 0x03} },
	{0x83, 1, {0xC8} },			//10bit 9:0
	{0x84, 1, {0x04} },			//PWM 22.66KHz

	{0xFF, 3, {0x78, 0x07, 0x06} },
	{0x08, 1, {0x20} },			//OSC 116M

	{0xFF, 3, {0x78 , 0x07, 0x04} },
	{0xB7, 1, {0xCF} },
	{0xB8, 1, {0x45} },
	{0xBA, 1, {0x74} },

	{0xFF, 3, {0x78, 0x07, 0x05} },
	{0x1B, 1, {0x00} },
	{0x1C, 1, {0x88} },			//VCOM=-0.1V 8E
	{0x61, 1, {0xCB} },
	{0x64, 1, {0xEF} },			//[7]=VGH_CLP[4]:VGH_RATIO 2X
	{0x6E, 1, {0x3F} },			//[6]=VGL_RATIO
	{0x69, 1, {0x02} },			//VGH short VGHO
	{0x70, 1, {0xC8} },			//[3]=VGL_CLP
	{0x72, 1, {0x6A} },			//VGH=10V
	{0x73, 1, {0x1C} },			//OTP VGH=6.1V
	{0x74, 1, {0x55} },			//VGL=-8.8V
	{0x76, 1, {0x79} },			//VGHO=10V
	{0x7A, 1, {0x3D} },			//VGLO=-7V
	{0x7B, 1, {0x7E} },			//GVDDP=5V
	{0x7C, 1, {0x7E} },			//GVDDN=-5V
	{0xB5, 1, {0x50} },			//PWR_D2A_HVREG_VGHO_EN
	{0xB7, 1, {0x70} },			//PWR_D2A_HVREG_VGLO_EN
	{0xC9, 1, {0x90} },			//0327 power off


	{0xFF, 3, {0x78, 0x07, 0x06} },

	{0xC0, 1, {0x9C} },			//Res=1080*2460
	{0xC1, 1, {0x19} },			//Res=1080*2460
	{0xC3, 1, {0x06} },			//ss_reg

	{0x96, 1, {0x50} },			//save power mipi bais

	{0xFF, 3, {0x78, 0x07, 0x07} },			//slice high 10
	{0x03, 1, {0x20} },
	{0x12, 1, {0x00} },
	{0x29, 1, {0xCE} },

	//Gamma Register
	{0xFF,3, {0x78, 0x07, 0x08} },
	{0xE0,31, {0x00, 0x00, 0x19, 0x38, 0x00, 0x62, 0x86, 0xA7, 0x14, 0xDB, 0x04, 0x42, 0x15, 0x75, 0xC1, 0xFD, 0x2A, 0x3B, 0x7F, 0xAB, 0x3E, 0xE4, 0x0D, 0x3A, 0x3F, 0x55, 0x80, 0xB8, 0x0F, 0xD5, 0xD9} },
	{0xE1,31, {0x00, 0x00, 0x19, 0x38, 0x00, 0x62, 0x86, 0xA7, 0x14, 0xDB, 0x04, 0x42, 0x15, 0x75, 0xC1, 0xFD, 0x2A, 0x3B, 0x7F, 0xAB, 0x3E, 0xE4, 0x0D, 0x3A, 0x3F, 0x55, 0x80, 0xB8, 0x0F, 0xD5, 0xD9} },

	{0xFF, 3, {0x78, 0x07, 0x0B} },			// autotrim
	{0x94, 1, {0x88} },
	{0x95, 1, {0x18} },
	{0x96, 1, {0x06} },
	{0x97, 1, {0x06} },
	{0x98, 1, {0xCA} },
	{0x99, 1, {0xCA} },
	{0x9A, 1, {0x06} },
	{0x9B, 1, {0xAB} },
	{0x9C, 1, {0x05} },
	{0x9D, 1, {0x05} },
	{0x9E, 1, {0xA6} },
	{0x9F, 1, {0xA6} },

	{0xC0, 1, {0x84} },
	{0xC1, 1, {0x0D} },
	{0xC2, 1, {0x03} },
	{0xC3, 1, {0x03} },
	{0xC4, 1, {0x65} },
	{0xC5, 1, {0x65} },
	{0xD2, 1, {0x03} },
	{0xD3, 1, {0x56} },
	{0xD4, 1, {0x03} },
	{0xD5, 1, {0x03} },
	{0xD6, 1, {0x53} },
	{0xD7, 1, {0x53} },
	{0xAB, 1, {0xE0} },

	{0xFF, 3, {0x78, 0x07, 0x0C} },			//TP Modulation
	{0x00, 1, {0x3F} },
	{0x01, 1, {0xE5} },
	{0x02, 1, {0x3F} },
	{0x03, 1, {0xDD} },
	{0x04, 1, {0x3F} },
	{0x05, 1, {0xDD} },
	{0x06, 1, {0x3F} },
	{0x07, 1, {0xD2} },
	{0x08, 1, {0x3F} },
	{0x09, 1, {0xE7} },
	{0x0A, 1, {0x3F} },
	{0x0B, 1, {0xDC} },
	{0x0C, 1, {0x3F} },
	{0x0D, 1, {0xE3} },
	{0x0E, 1, {0x3F} },
	{0x0F, 1, {0xD6} },
	{0x10, 1, {0x3F} },
	{0x11, 1, {0xE6} },
	{0x12, 1, {0x3F} },
	{0x13, 1, {0xDA} },
	{0x14, 1, {0x3F} },
	{0x15, 1, {0xE0} },
	{0x16, 1, {0x3F} },
	{0x17, 1, {0xCF} },
	{0x18, 1, {0x3F} },
	{0x19, 1, {0xD4} },
	{0x1A, 1, {0x3F} },
	{0x1B, 1, {0xE4} },
	{0x1C, 1, {0x3F} },
	{0x1D, 1, {0xCB} },
	{0x1E, 1, {0x3F} },
	{0x1F, 1, {0xCA} },
	{0x20, 1, {0x3F} },
	{0x21, 1, {0xE2} },
	{0x22, 1, {0x3F} },
	{0x23, 1, {0xCD} },
	{0x24, 1, {0x3F} },
	{0x25, 1, {0xD3} },
	{0x26, 1, {0x3F} },
	{0x27, 1, {0xDF} },
	{0x28, 1, {0x3F} },
	{0x29, 1, {0xE8} },
	{0x2A, 1, {0x3F} },
	{0x2B, 1, {0xC9} },
	{0x2C, 1, {0x3F} },
	{0x2D, 1, {0xC8} },
	{0x2E, 1, {0x3F} },
	{0x2F, 1, {0xD1} },
	{0x30, 1, {0x3F} },
	{0x31, 1, {0xD9} },
	{0x32, 1, {0x3F} },
	{0x33, 1, {0xDB} },
	{0x34, 1, {0x3F} },
	{0x35, 1, {0xD8} },
	{0x36, 1, {0x3F} },
	{0x37, 1, {0xDE} },
	{0x38, 1, {0x3F} },
	{0x39, 1, {0xCC} },
	{0x3A, 1, {0x3F} },
	{0x3B, 1, {0xD5} },
	{0x3C, 1, {0x3F} },
	{0x3D, 1, {0xD0} },
	{0x3E, 1, {0x3F} },
	{0x3F, 1, {0xD7} },

	{0x80, 1, {0x24} },
	{0x81, 1, {0xE5} },
	{0x82, 1, {0x24} },
	{0x83, 1, {0xCE} },
	{0x84, 1, {0x24} },
	{0x85, 1, {0xCE} },
	{0x86, 1, {0x24} },
	{0x87, 1, {0xE7} },
	{0x88, 1, {0x24} },
	{0x89, 1, {0xE2} },
	{0x8A, 1, {0x24} },
	{0x8B, 1, {0xCF} },
	{0x8C, 1, {0x24} },
	{0x8D, 1, {0xDF} },
	{0x8E, 1, {0x24} },
	{0x8F, 1, {0xE8} },
	{0x90, 1, {0x24} },
	{0x91, 1, {0xDD} },
	{0x92, 1, {0x24} },
	{0x93, 1, {0xD5} },
	{0x94, 1, {0x24} },
	{0x95, 1, {0xDC} },
	{0x96, 1, {0x24} },
	{0x97, 1, {0xD0} },
	{0x98, 1, {0x24} },
	{0x99, 1, {0xDA} },
	{0x9A, 1, {0x24} },
	{0x9B, 1, {0xD4} },
	{0x9C, 1, {0x24} },
	{0x9D, 1, {0xDB} },
	{0x9E, 1, {0x24} },
	{0x9F, 1, {0xCC} },
	{0xA0, 1, {0x24} },
	{0xA1, 1, {0xE4} },
	{0xA2, 1, {0x24} },
	{0xA3, 1, {0xE3} },
	{0xA4, 1, {0x24} },
	{0xA5, 1, {0xD6} },
	{0xA6, 1, {0x24} },
	{0xA7, 1, {0xD9} },
	{0xA8, 1, {0x24} },
	{0xA9, 1, {0xE0} },
	{0xAA, 1, {0x24} },
	{0xAB, 1, {0xD7} },
	{0xAC, 1, {0x24} },
	{0xAD, 1, {0xE1} },
	{0xAE, 1, {0x24} },
	{0xAF, 1, {0xD2} },
	{0xB0, 1, {0x24} },
	{0xB1, 1, {0xD2} },
	{0xB2, 1, {0x24} },
	{0xB3, 1, {0xCB} },
	{0xB4, 1, {0x24} },
	{0xB5, 1, {0xD1} },
	{0xB6, 1, {0x24} },
	{0xB7, 1, {0xD3} },
	{0xB8, 1, {0x24} },
	{0xB9, 1, {0xCD} },
	{0xBA, 1, {0x24} },
	{0xBB, 1, {0xC9} },
	{0xBC, 1, {0x24} },
	{0xBD, 1, {0xD8} },
	{0xBE, 1, {0x24} },
	{0xBF, 1, {0xCA} },

	{0xFF, 3, {0x78, 0x07, 0x0E} },
	{0x00, 1, {0xA3} },			//LH mode
	{0x02, 1, {0x0F} },
	{0x04, 1, {0x06} }, 		//TSHD_1VP oFF & TSVD free run
	{0xB9, 1, {0xCC} }, 		//TSHD PS off

	//*********** 60 hz table**************
	{0x40, 1, {0x07} }, 		//7 unit
	{0x47, 1, {0x10} }, 		//Unit line=307
	{0x49, 1, {0x33} }, 		//Unit line=307
	{0x45, 1, {0x11} }, 		//TP2_unit0=273us
	{0x46, 1, {0x11} }, 		//TP2_unit0=273us
	{0x4D, 1, {0xB5} }, 		//RTN 5.68usec

	{0xC0, 1, {0x34} },			//TP3 unit
	{0xC6, 1, {0x5B} }, 		//60HZ TP3-4
	{0xC7, 1, {0x5B} }, 		//60HZ TP3-3
	{0xC8, 1, {0x5B} }, 		//60HZ TP3-2
	{0xC9, 1, {0x5B} }, 		//60HZ TP3-1
	{0x21, 1, {0x85} }, 		//TSVD1 rising position shift
	{0x23, 1, {0x85} }, 		//TSVD2 rising position shift
	{0x2B, 1, {0x1F} },
	{0x05, 1, {0x24} },
	{0xB0, 1, {0x31} },       //TP1 不开CKH 0406
	{0x41, 1, {0x44} }, 		// 60hz TSVD position
	{0x4B, 1, {0x1F} },

	{0xFF, 3, {0x78, 0x07, 0x03} },	//pwm 12bit, 28.32k
	{0x83, 1, {0x20} },
	{0x84, 1, {0x00} },

	{0xFF, 3, {0x78, 0x07, 0x06} },
	{0x08, 1, {0x20} },

	{0xFF, 3, {0x78, 0x07, 0x00} },	//Page0
	{0x51, 2, {0x0c, 0xcc}},
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x01} },	//CABC:UI
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 20, {} },
	{0x35, 1, {0x00} },
};

static struct LCM_setting_table lcm_cabc_setting[] = {
	{0x55, 1, {0x01} },	//UI
	{0x55, 1, {0x03} },	//MV
	{0x55, 1, {0x00} },	//DISABLE
};

static struct LCM_setting_table lcm_hbm_setting[] = {
	{0x51, 2, {0x0C, 0XCC} },	//80% PWM
	{0x51, 2, {0x0F, 0XFF} },	//100% PWM
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
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 46;
	params->dsi.horizontal_frontporch = 46;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 560;
	params->physical_height = 161;
	params->physical_width = 70;

	params->density = 480;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
        params->dsi.lcm_esd_check_table[1].cmd = 0x0d;
        params->dsi.lcm_esd_check_table[1].count = 1;
        params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 8;
}

static void lcm_init_power(void)
{
	lcm_util.set_lcd_ldo_en(1);
	if(lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(1);
		_lcm_i2c_write_bytes(0x00, 0x12);
		_lcm_i2c_write_bytes(0x01, 0x12);
	}
	else
		pr_debug("[LCM]set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_suspend_power(void)
{
	//SET_RESET_PIN(0);
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

	pr_info("[LCM]lcm_init\n");

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
	pr_info("[LCM]lcm_suspend\n");

	push_table(NULL, lcm_suspend_setting,
			ARRAY_SIZE(lcm_suspend_setting), 1);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}


static void lcm_setbacklight(/*void *handle, */unsigned int level)
{
	pr_info("lcm_setbacklight.");

	return;
}

static void lcm_set_cmdq(void *handle, unsigned int *lcm_cmd,
		unsigned int *lcm_count, unsigned int *lcm_value)
{
	pr_info("%s,ili7807s cmd:%d, value = %d\n", __func__, *lcm_cmd, *lcm_value);

	switch(*lcm_cmd) {
		case PARAM_HBM:
			push_table(handle, &lcm_hbm_setting[*lcm_value], 1, 1);
			break;
		case PARAM_CABC:
			push_table(handle, &lcm_cabc_setting[*lcm_value], 1, 1);
			break;
		default:
			pr_err("%s,ili7807s cmd:%d, unsupport\n", __func__, *lcm_cmd);
			break;
	}

	pr_info("%s,ili7807s cmd:%d, value = %d done\n", __func__, *lcm_cmd, *lcm_value);

}

struct LCM_DRIVER mipi_mot_vid_tianma_ili7807s_fhd_678_lcm_drv = {
	.name = "mipi_mot_vid_tianma_ili7807s_fhd_678",
	.supplier = "tianma",
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
        .set_lcm_cmd = lcm_set_cmdq,
};
