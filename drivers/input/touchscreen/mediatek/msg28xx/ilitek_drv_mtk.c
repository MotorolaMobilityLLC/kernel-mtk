/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
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
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

/**
 *
 * @file    ilitek_drv_mtk.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>

#include "ilitek_drv_common.h"

#include <ontim/ontim_dev_dgb.h>
static char msg2836a_vendor_name[]="HLT-msg2836a";
char msg2836a_version[50]="HLT-msg2836a-fw:0";
DEV_ATTR_DECLARE(touch_screen)
DEV_ATTR_DEFINE("version",msg2836a_version)
DEV_ATTR_DEFINE("vendor",msg2836a_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(touch_screen,touch_screen,8);

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/
#define TP_IC_NAME "msg2xxx" //"msg22xx" or "msg28xx" /* Please define the mstar touch ic name based on the mutual-capacitive ic or self capacitive ic that you are using */

#define I2C_BUS_ID   (1)       // i2c bus id : 0 or 1

#define TPD_OK (0)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
extern int g_TpVirtualKey[];

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

extern struct tpd_device *tpd;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

struct i2c_client *g_I2cClient = NULL;


#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
struct regulator *g_ReguVdd = NULL;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

/*=============================================================*/
// FUNCTION DEFINITION
/*=============================================================*/

/* probe function is used for matching and initializing input device */
static int /*__devinit*/ tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    const char *vdd_name = "vtouch";
//    const char *vcc_i2c_name = "vcc_i2c";
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_DMESG("TPD probe\n");   

    if (client == NULL)
    {
        TPD_DMESG("i2c client is NULL\n");
        return -1;
    }
    g_I2cClient = client;
    
    MsDrvInterfaceTouchDeviceSetIicDataRate(g_I2cClient, 100000); // 100 KHz

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    g_ReguVdd = regulator_get(tpd->tpd_dev, vdd_name);
    tpd->reg = g_ReguVdd;

    ret = regulator_set_voltage(g_ReguVdd, 2800000, 2800000); 
    if (ret)
    {
        TPD_DMESG("Could not set to 2800mv.\n");
    }
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    ret = MsDrvInterfaceTouchDeviceProbe(g_I2cClient, id);
    if (ret == 0) // If probe is success, then enable the below flag.
    {
        tpd_load_status = 1;
    }    
    REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
    TPD_DMESG("TPD probe done\n");
    
    return TPD_OK;   
}

static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info) 
{
    strcpy(info->type, TPD_DEVICE);    
    
    return TPD_OK;
}

static int /*__devexit*/ tpd_remove(struct i2c_client *client)
{   
    TPD_DEBUG("TPD removed\n");
    
    MsDrvInterfaceTouchDeviceRemove(client);
    
    return TPD_OK;
}


/* The I2C device list is used for matching I2C device and I2C device driver. */
static const struct i2c_device_id tpd_device_id[] =
{
    {TP_IC_NAME, 0},
    {}, /* should not omitted */ 
};

MODULE_DEVICE_TABLE(i2c, tpd_device_id);

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
const struct of_device_id touch_dt_match_table[] = {
    { .compatible = "mediatek,ilitek_touch",},
    {},
};

MODULE_DEVICE_TABLE(of, touch_dt_match_table);

static struct device_attribute *msg2xxx_attrs[] = {
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	&dev_attr_tpd_scp_ctrl,
#endif //CONFIG_MTK_SENSOR_HUB_SUPPORT
};

#else
static struct i2c_board_info __initdata i2c_tpd = {I2C_BOARD_INFO(TP_IC_NAME, (0x4C>>1))};

#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

static struct i2c_driver tpd_i2c_driver = {

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .driver = {
        .name = TP_IC_NAME,
        .of_match_table = of_match_ptr(touch_dt_match_table),
    },
#else
    .driver = {
        .name = TP_IC_NAME,
    },
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = tpd_device_id,
    .detect = tpd_detect,
};

static int tpd_local_init(void)
{  
    TPD_DMESG("TPD init device driver\n");

    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        TPD_DMESG("Unable to add i2c driver.\n");
         
        return -1;
    }
    
    if (tpd_load_status == 0) 
    {
        TPD_DMESG("Add error touch panel driver.\n");

        i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_dts_data.use_tpd_button)
    {
        tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
        tpd_dts_data.tpd_key_dim_local);
    }
#else
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE     
    tpd_button_setting(4, g_TpVirtualKey, g_TpVirtualKeyDimLocal); //MAX_KEY_NUM
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE  
#endif //CONFIG_TP_HAVE_KEY  
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_DMESG("TPD init done %s, %d\n", __func__, __LINE__);  
        
    return TPD_OK; 
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
static void tpd_resume(struct device *h)
#else
static void tpd_resume(struct early_suspend *h)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_DMESG("TPD wake up\n");
    
    MsDrvInterfaceTouchDeviceResume(h);
    
    TPD_DMESG("TPD wake up done\n");
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
static void tpd_suspend(struct device *h)
#else
static void tpd_suspend(struct early_suspend *h)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_DMESG("TPD enter sleep\n");

    MsDrvInterfaceTouchDeviceSuspend(h);

    TPD_DMESG("TPD enter sleep done\n");
} 

static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = TP_IC_NAME,
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .attrs = {
        .attr = msg2xxx_attrs,
        .num  = ARRAY_SIZE(msg2xxx_attrs),
    },
#else
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
     .tpd_have_button = 1,
#else
     .tpd_have_button = 0,
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE        
#endif //CONFIG_TP_HAVE_KEY        
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
};

static int __init tpd_driver_init(void) 
{
    TPD_DMESG("ILITEK/MStar touch panel driver init\n");

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    tpd_get_dts_info();
#else
    i2c_register_board_info(I2C_BUS_ID, &i2c_tpd, 1);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_driver_add(&tpd_device_driver) < 0)
    {
        TPD_DMESG("TPD add ILITEK/MStar TP driver failed\n");
    }
     
    return 0;
}
 
static void __exit tpd_driver_exit(void) 
{
    TPD_DMESG("ILITEK/MStar touch panel driver exit\n");
    
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
MODULE_LICENSE("GPL");