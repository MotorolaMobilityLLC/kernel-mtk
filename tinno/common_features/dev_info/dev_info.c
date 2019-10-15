/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef CONFIG_TINNO_PRODUCT_INFO

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/module.h>


#include "dev_info.h"
//#include <linux/dev_info.h>

/* /sys/devices/platform/$PRODUCT_DEVICE_INFO */
#define PRODUCT_DEVICE_INFO    "product-device-info"

///////////////////////////////////////////////////////////////////
static int dev_info_probe(struct platform_device *pdev);
static int dev_info_remove(struct platform_device *pdev);
static ssize_t store_product_dev_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t show_product_dev_info(struct device *dev, struct device_attribute *attr, char *buf);
///////////////////////////////////////////////////////////////////
static product_info_arr prod_dev_array[ID_MAX];
static product_info_arr *pi_p = prod_dev_array;

static struct platform_driver dev_info_driver = {
	.probe = dev_info_probe,
	.remove = dev_info_remove,
	.driver = {
		   .name = PRODUCT_DEVICE_INFO,
	},
};

static struct platform_device dev_info_device = {
    .name = PRODUCT_DEVICE_INFO,
    .id = -1,
};

#define PRODUCT_DEV_INFO_ATTR(_name)                         \
{                                       \
	.attr = { .name = #_name, .mode = S_IRUGO | S_IWUSR | S_IWGRP,},  \
	.show = show_product_dev_info,                  \
	.store = store_product_dev_info,                              \
}

static struct device_attribute product_dev_attr_array[] = {
    PRODUCT_DEV_INFO_ATTR(info_lcd),
    PRODUCT_DEV_INFO_ATTR(info_tp),
    PRODUCT_DEV_INFO_ATTR(info_gyro),
    PRODUCT_DEV_INFO_ATTR(info_gsensor),
    PRODUCT_DEV_INFO_ATTR(info_psensor),
    PRODUCT_DEV_INFO_ATTR(info_msensor),
    PRODUCT_DEV_INFO_ATTR(info_flash),
    PRODUCT_DEV_INFO_ATTR(info_battery),
    PRODUCT_DEV_INFO_ATTR(info_main_camera),
    PRODUCT_DEV_INFO_ATTR(info_sub_camera),
    PRODUCT_DEV_INFO_ATTR(info_main2_camera),
    PRODUCT_DEV_INFO_ATTR(info_sub2_camera),
    PRODUCT_DEV_INFO_ATTR(info_main3_camera),
    PRODUCT_DEV_INFO_ATTR(info_fingerprint),
    PRODUCT_DEV_INFO_ATTR(info_secboot),
    PRODUCT_DEV_INFO_ATTR(info_bl_lock_status),
    PRODUCT_DEV_INFO_ATTR(info_nfc),
    PRODUCT_DEV_INFO_ATTR(info_hall),
    PRODUCT_DEV_INFO_ATTR(info_hw_board_id),
// add new ...
    
};
///////////////////////////////////////////////////////////////////////////////////////////
/*
* int full_product_device_info(int id, const char *info, int (*cb)(char* buf, void *args), void *args);
*/
int full_product_device_info(int id, const char *info, FuncPtr cb, void *args) {   
    tinnoklog("%s: - [%d, %s, %pf]\n", __func__, id, info, cb); 

    if (id >= 0 &&  id < ID_MAX ) {
        memset(pi_p[id].show, 0, show_content_len);
        if (cb != NULL && pi_p[id].cb == NULL) {
            pi_p[id].cb = cb;
            pi_p[id].args = args; 
        }
        else if (info != NULL) {
            strcpy(pi_p[id].show, info);
            pi_p[id].cb = NULL;
            pi_p[id].args = NULL;
        }
        return 0;
    }
    return -1;
}

static ssize_t store_product_dev_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    return count;
}

static ssize_t show_product_dev_info(struct device *dev, struct device_attribute *attr, char *buf) {
    int i = 0;
    char *show = NULL;
    const ptrdiff_t x = (attr - product_dev_attr_array);
   
    if (x >= ID_MAX) {
        BUG_ON(1);
    }

    show = pi_p[x].show;
    if (pi_p[x].cb != NULL) {
        pi_p[x].cb(show, pi_p[x].args);
    }

    tinnoklog("%s: - offset(%d): %s\n", __func__, (int)x, show);
    if (strlen(show) > 0) {
        i = sprintf(buf, "%s ", show);
    }
    else {
        tinnoklog("%s - offset(%d): NULL!\n", __func__, (int)x);
    }

    return i;
}

static int dev_info_probe(struct platform_device *pdev) {	
    int i, rc;

    __TinnoFUN(); 
    for (i = 0; i < ARRAY_SIZE(product_dev_attr_array); i++) {
        rc = device_create_file(&pdev->dev, &product_dev_attr_array[i]);
        if (rc) {
            tinnoklog( "%s, create_attrs_failed:%d,%d\n", __func__, i, rc);
        }
    }

    //For MTK secureboot.add by yinglong.tang
    /** get_mtk_secboot_info(); */
    FULL_PRODUCT_DEVICE_CB(ID_SECBOOT, get_mtk_secboot_cb, pdev);
    return 0;
}

static int dev_info_remove(struct platform_device *pdev) {    
    int i;
	
     __TinnoFUN();
    for (i = 0; i < ARRAY_SIZE(product_dev_attr_array); i++) {
        device_remove_file(&pdev->dev, &product_dev_attr_array[i]);
    }
    return 0;
}

static int __init dev_info_drv_init(void) {
    __TinnoFUN();

    if (platform_device_register(&dev_info_device) != 0) {
        tinnoklog( "device_register fail!.\n");
        return -1;
    
    }
	
    if (platform_driver_register(&dev_info_driver) != 0) {
        tinnoklog( "driver_register fail!.\n");
        return -1;
    }
	
    return 0;
}

static void __exit dev_info_drv_exit(void) {
	__TinnoFUN();
	platform_driver_unregister(&dev_info_driver);
	platform_device_unregister(&dev_info_device);
}

///////////////////////////////////////////////////////////////////
late_initcall(dev_info_drv_init);
module_exit(dev_info_drv_exit);

//MODULE_LICENSE("GPL");
//MODULE_DESCRIPTION("product-dev-info");
//MODULE_AUTHOR("<mingyi.guo@tinno.com>");

#endif

