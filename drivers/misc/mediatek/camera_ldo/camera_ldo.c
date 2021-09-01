/*
 * Copyright (C) 2015 HUAQIN Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include "camera_ldo.h"


/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/

static struct i2c_client *camera_ldo_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int camera_ldo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int camera_ldo_i2c_remove(struct i2c_client *client);

/*****************************************************************************
 * Extern Area
 *****************************************************************************/
void camera_ldo_set_en_ldo(CAMERA_LDO_SELECT ldonum,unsigned int en)
{
	s32 ret=0;
	unsigned int value =0;

	 if (NULL == camera_ldo_i2c_client) {
	        CAMERA_LDO_PRINT("[camera_ldo] camera_ldo_i2c_client is null!!\n");
	        return ;
    }
	ret= i2c_smbus_read_byte_data(camera_ldo_i2c_client, CAMERA_LDO_LDO_EN_ADDR);

	if(ret <0)
	{
		CAMERA_LDO_PRINT("[camera_ldo] camera_ldo_set_en_ldo read error!\n");
		return;
	}

	if(en == 0)
	{
		value = ret & (~(0x01<<ldonum));
	}
	else
	{
		value = ret|(0x01<<ldonum);
	}

	i2c_smbus_write_byte_data(camera_ldo_i2c_client,CAMERA_LDO_LDO_EN_ADDR,value);
	CAMERA_LDO_PRINT("[camera_ldo] camera_ldo_set_en_ldo enable before:%x after set :%x  \n",ret,value);
    return;

}

//VOUT1=0.496V+LDO1_VOUT [6:0]*0.008.   LDO1/LDO2
//VOUTx=1.504V+LDOx_VOUT [7:0]*0.008.   LDO3~LDO7
void camera_ldo_set_ldo_value(CAMERA_LDO_SELECT ldonum,unsigned int value)
{
	unsigned int  Ldo_out =0;
       unsigned char regaddr =0;
       s32 ret =0;
	CAMERA_LDO_PRINT("[camera_ldo] %s enter!!!\n",__FUNCTION__);

	 if (NULL == camera_ldo_i2c_client) {
	        CAMERA_LDO_PRINT("[camera_ldo] camera_ldo_i2c_client is null!!\n");
	        return ;
        }
	if(ldonum >= CAMERA_LDO_MAX)
	{
		CAMERA_LDO_PRINT("[camera_ldo] error ldonum not support!!!\n");
		return;
	}

	switch(ldonum)
	{
		case CAMERA_LDO_DVDD1:
		case CAMERA_LDO_DVDD2:
			if(value<600)
			{
				CAMERA_LDO_PRINT("[camera_ldo] error vol!!!\n");
				goto exit;
			}
			else
			{
			 	Ldo_out = (value-600)/6;
				printk("camera DVDD %d\n",value);
			}
	       break;
		case CAMERA_LDO_VDDAF:
		case CAMERA_LDO_VDDIO:
		case CAMERA_LDO_AVDD3:
		case CAMERA_LDO_AVDD2:
		case CAMERA_LDO_AVDD1:
			if(value<1200)
			{
				CAMERA_LDO_PRINT("[camera_ldo] error vol!!!\n");
				goto exit;

			}
			else
			{
				Ldo_out = (value-1200)/10;
				printk("camera AVDD %d\n",value);
			}
			break;
		default:
			goto exit;
			break;
	}
	regaddr = ldonum+CAMERA_LDO_LDO1_OUT_ADDR;

	CAMERA_LDO_PRINT("[camera_ldo] ldo=%d,value=%d,Ldo_out:%d,regaddr=0x%x \n",ldonum,value,Ldo_out,regaddr);
	i2c_smbus_write_byte_data(camera_ldo_i2c_client,regaddr,Ldo_out);
	ret=i2c_smbus_read_byte_data(camera_ldo_i2c_client,regaddr);
	CAMERA_LDO_PRINT("[camera_ldo] after write ret=0x%x\n",ret);

exit:
	CAMERA_LDO_PRINT("[camera_ldo] %s exit!!!\n",__FUNCTION__);

}

/*****************************************************************************
 * Data Structure
 ********************************************************/
static const struct of_device_id i2c_of_match[] = {
    { .compatible = "mediatek,i2c_camera_ldo", },
    {},
};

static const struct i2c_device_id camera_ldo_i2c_id[] = {
    {"camera_ldo_I2C", 0},
    {},
};
static struct i2c_driver camera_ldo_i2c_driver = {
/************************************************************
Attention:
Althouh i2c_bus do not use .id_table to match, but it must be defined,
otherwise the probe function will not be executed!
************************************************************/
    .id_table = camera_ldo_i2c_id,
    .probe = camera_ldo_i2c_probe,
    .remove = camera_ldo_i2c_remove,
    .driver = {
        .name = "camera_ldo_I2C",
        .of_match_table = i2c_of_match,
    },
};

static int camera_ldo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (NULL == client) {
        CAMERA_LDO_PRINT("[camera_ldo] i2c_client is NULL\n");
        return -1;
    }
    camera_ldo_i2c_client = client;

    CAMERA_LDO_PRINT("[camera_ldo]camera_ldo_i2c_probe success addr = 0x%x\n", client->addr);
    return 0;
}

static int camera_ldo_i2c_remove(struct i2c_client *client)
{
    camera_ldo_i2c_client = NULL;
    i2c_unregister_device(client);

    return 0;
}

static int __init camera_ldo_init(void)
{
    if (i2c_add_driver(&camera_ldo_i2c_driver)) {
        CAMERA_LDO_PRINT("[camera_ldo]Failed to register camera_ldo_i2c_driver!\n");
        return -1;
    }

    return 0;
}

static void __exit camera_ldo_exit(void)
{
    i2c_del_driver(&camera_ldo_i2c_driver);
}

module_init(camera_ldo_init);
module_exit(camera_ldo_exit);

MODULE_AUTHOR("AmyGuo <guohuiqing@huaqin.com>");
MODULE_DESCRIPTION("MTK EXT CAMERA LDO Driver");
MODULE_LICENSE("GPL");
