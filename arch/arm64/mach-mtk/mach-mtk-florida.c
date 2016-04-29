 /* Meditek modules for Cragganmore - board data probing
	  *
	  * Copyright 2014 Wolfson Microelectronics plc
	  * Karl Sun <karl.sun@opensource.wolfsonmicro.com>
	  *
	  * This program is free software; you can redistribute it and/or modify
	  * it under the terms of the GNU General Public License version 2 as
	  * published by the Free Software Foundation.
  */
 
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/input.h>

#include <linux/mfd/arizona/pdata.h>
#include <linux/mfd/core.h>
#include <linux/mfd/arizona/registers.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
//#include <mach/eint.h>
#include <mt_spi.h>
#include <linux/gpio.h>
#include <mt_gpio.h>
#define  CODEC_GPIO_BASE 500 
int  audio_hardid = 1;
/* lenovo-sw zhangrc2 change gpio95 to support ldo for codec */
struct platform_device mtk_florida_i2s = {
	.name = "mtk-florida-i2s",
	.id = -1,
	.dev = {
	},
};

struct platform_device mtk_florida_i2s2_record = {
	.name = "mtk-florida-i2s-record",
	.id = -1,
	.dev = {
	},
};

struct platform_device mtk_florida_sound_card = {
	.name = "mt-snd-card",
	.id = -1,
	.dev = {
	},
};

static struct platform_device *mtk_florida_audio_devs[] __initdata = {
	&mtk_florida_i2s,
	&mtk_florida_i2s2_record,
	&mtk_florida_sound_card,
}; 

static struct mt_chip_conf florida_spi_conf ={
		.setuptime = 3,
		.holdtime = 3,
/* lenovo-sw zhangrc2 change spi speed to 18M begin speed = 134000/(low_time+high_time) */		
		.high_time = 3,
		.low_time = 3,
/* lenovo-sw zhangrc2 change spi speed to 18M end */			
		.cs_idletime = 2,
		.ulthgh_thrsh = 0,
		.cpol = 0,
		.cpha = 0,
		.rx_mlsb = 1, 
		.tx_mlsb = 1,
		.tx_endian = 0,
		.rx_endian = 0,
		.com_mod = DMA_TRANSFER/*FIFO_TRANSFER*/,
		.pause = 1,
		.finish_intr = 1,
		.deassert = 0,
		.ulthigh = 0,
		.tckdly = 0,
};

static const struct arizona_micd_range micd_ctp_ranges[] = {
	{ .max =  70, .key = KEY_MEDIA }, //KEY_PLAYPAUSE:164->fw:85, KEY_MEDIA:226->fw:79
	{ .max =  290 , .key = KEY_VOLUMEUP },
	{ .max =  680 , .key = KEY_VOLUMEDOWN },
};

static struct arizona_micd_config florida_micd_config[]={
	{ 0        ,0,  1 , 0 },
	{ ARIZONA_ACCDET_SRC	     ,ARIZONA_ACCDET_SRC,  3  ,1},
};
 
static struct arizona_pdata florida_platform_data = {
	.reset = 4,	/** GPIO controlling /RESET, if any */
	.ldoena = 24, /** GPIO controlling LODENA, if any */
	.clk32k_src = ARIZONA_32KZ_MCLK2,
	.irq_flags = IRQF_TRIGGER_LOW, //for wm test
	.gpio_base = CODEC_GPIO_BASE,
	.micd_pol_gpio = CODEC_GPIO_BASE + 4,
	/* lenovo-sw zhangrc2 optim hpdet detect 2015-8-12 */
	.micd_detect_debounce =500,
	.init_mic_delay=30,
	/* lenovo-sw zhangrc2 optim hpdet detect 2015-8-12 */
	.micd_bias_start_time = 1,
	.micd_rate = 7,
	.micd_configs= florida_micd_config,
	.num_micd_configs=ARRAY_SIZE(florida_micd_config),
	.micd_ranges=micd_ctp_ranges,
	.num_micd_ranges=ARRAY_SIZE(micd_ctp_ranges),
	.micd_force_micbias=1,
	.micd_low_ohm=1,
	.gpio_mtk_mode=1,
	.jd_wake_time=5000,
	.inmode = {
			[0]= ARIZONA_INMODE_DIFF, /*IN1L for Headset*/
			[1]= ARIZONA_INMODE_DIFF,
			[2]= ARIZONA_INMODE_DIFF,
			[3]= ARIZONA_INMODE_DIFF,
		},
	.micbias={
			[0]={ /*MICBIAS1*/
				.mV =2800 ,
				.ext_cap=1,
				.discharge[0] =1 ,
				.soft_start =0,
				.bypass =0,
			},
			[1]={ /*MICBIAS2*/
				.mV =2800 ,
				.ext_cap=1,	
				.discharge[1]=1 ,
				.soft_start =0,
				.bypass =0,
			},
			[2]={ /*MICBIAS3*/
				.mV =2800 ,
				.ext_cap=1,
				.discharge[2]=1 ,
				.soft_start =0,
				.bypass =0,
			},
		},
 };

#if 0 /*mask for compile warning*/
static struct i2c_board_info __initdata florida_board_info = {
	 I2C_BOARD_INFO("wm8281", 0x1B),
	 .platform_data = &florida_platform_data,
	 //.irq= EINT_IRQ(12),
};

/*Add spi board driver*/
static struct spi_board_info wm8281_spi_gpio_devs[] __initdata = {
	[0] = {
		.modalias="wm8281",
		.bus_num        = 32766,
		.chip_select    = 0,	
		.max_speed_hz = 16000000,
		.mode		= SPI_MODE_0,
		.platform_data = &florida_platform_data,
		.controller_data = &florida_spi_conf,
		.irq= /*EINT_IRQ(12)*/294,
	},
}; 
#endif

/*Add spi board driver*/
static struct spi_board_info wm8281_spi_devs[] __initdata = {
	[0] = {
		.modalias="wm8281",
		.bus_num        = 1,
		.chip_select    = 0,	
		.max_speed_hz = 16000000,
		.mode		= SPI_MODE_0,
		.platform_data = &florida_platform_data,
		.controller_data = &florida_spi_conf,
		.irq= /*EINT_IRQ(78)*/366,
	},
}; 
/* lenovo-sw zhangrc2 compatible gpio-spi and spi */
#define GPIO_ARIZONA_DVDD_PIN 55
static int __init florida_audio_platform_init(void)
{ 
	int ret =0;

         gpio_set_value_cansleep(GPIO_ARIZONA_DVDD_PIN, 1);
	ret = 	 gpio_get_value_cansleep(GPIO_ARIZONA_DVDD_PIN);
	printk("%s: shone get gpi55 is %d\n", __func__,ret);
	ret=spi_register_board_info(wm8281_spi_devs,1);
	printk("%s: arizona spi_gpio_register_board_info devices %d\n", __func__,ret);

	ret=platform_add_devices(mtk_florida_audio_devs, ARRAY_SIZE(mtk_florida_audio_devs));
	printk("%s: arizona add devices %d\n", __func__,ret);

	return 0;
}

module_init(florida_audio_platform_init);

