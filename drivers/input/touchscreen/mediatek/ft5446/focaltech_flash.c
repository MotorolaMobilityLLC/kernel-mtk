/*
 *
 * FocalTech fts TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: Focaltech_IC_Program.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-10-26
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FTS_REG_FW_MAJ_VER	0xB1
#define FTS_REG_FW_MIN_VER	0xB2
#define FTS_REG_FW_SUB_MIN_VER	0xB3
#define FTS_FW_MIN_SIZE		8
#define FTS_FW_MAX_SIZE		(54 * 1024)
/* Firmware file is not supporting minor and sub minor so use 0 */
#define FTS_FW_FILE_MAJ_VER(x)	((x)->data[(x)->size - 2])
#define FTS_FW_FILE_MIN_VER(x)	0
#define FTS_FW_FILE_SUB_MIN_VER(x) 0
#define FTS_FW_FILE_VENDOR_ID(x)	((x)->data[(x)->size - 1])
#define FTS_FW_FILE_MAJ_VER_FT6X36(x)	((x)->data[0x10a])
#define FTS_FW_FILE_VENDOR_ID_FT6X36(x)	((x)->data[0x108])
#define FTS_MAX_TRIES		5
#define FTS_RETRY_DLY		20
#define FTS_MAX_WR_BUF		10
#define FTS_MAX_RD_BUF		2
#define FTS_FW_PKT_META_LEN	6
#define FTS_FW_PKT_DLY_MS	20
#define FTS_FW_LAST_PKT		0x6ffa
#define FTS_EARSE_DLY_MS		100
#define FTS_55_AA_DLY_NS		5000
#define FTS_CAL_START		0x04
#define FTS_CAL_FIN		0x00
#define FTS_CAL_STORE		0x05
#define FTS_CAL_RETRY		100
#define FTS_REG_CAL		0x00
#define FTS_CAL_MASK		0x70
#define FTS_BLOADER_SIZE_OFF	12
#define FTS_BLOADER_NEW_SIZE	30
#define FTS_DATA_LEN_OFF_OLD_FW	8
#define FTS_DATA_LEN_OFF_NEW_FW	14
#define FTS_FINISHING_PKT_LEN_OLD_FW	6
#define FTS_FINISHING_PKT_LEN_NEW_FW	12
#define FTS_MAGIC_BLOADER_Z7	0x7bfa
#define FTS_MAGIC_BLOADER_LZ4	0x6ffa
#define FTS_MAGIC_BLOADER_GZF_30	0x7ff4
#define FTS_MAGIC_BLOADER_GZF	0x7bf4
#define FTS_REG_ECC		0xCC
#define FTS_RST_CMD_REG2		0xBC
#define FTS_READ_ID_REG		0x90
#define FTS_ERASE_APP_REG	0x61
#define FTS_ERASE_PARAMS_CMD	0x63
#define FTS_FW_WRITE_CMD		0xBF
#define FTS_REG_RESET_FW		0x07
#define FTS_RST_CMD_REG1		0xFC
#define FTS_FACTORYMODE_VALUE				0x40
#define FTS_WORKMODE_VALUE				0x00
#define FTS_APP_INFO_ADDR					0xd7f8

#define	BL_VERSION_LZ4        0
#define   BL_VERSION_Z7        1
#define   BL_VERSION_GZF        2
#define   FTS_REG_ID			0xA3
#define   FTS_REG_FW_VENDOR_ID 0xA8

#define FTS_PACKET_LENGTH      	128
#define FTS_SETTING_BUF_LEN      	128

#define FTS_UPGRADE_LOOP		30
#define FTS_MAX_POINTS_2                        		2
#define FTS_MAX_POINTS_5                        		5
#define FTS_MAX_POINTS_10                        		10
#define AUTO_CLB_NEED                           		1
#define AUTO_CLB_NONEED                         		0
#define FTS_UPGRADE_AA		0xAA
#define FTS_UPGRADE_55		0x55
#define HIDTOI2C_DISABLE					0
#define FTXXXX_INI_FILEPATH_CONFIG ""
/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

/*******************************************************************************
* Static variables
*******************************************************************************/
static unsigned char CTPM_FW[] = {
	//#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
	// #include "LQ_CM280_Ofilm0x51_V03_D01_20151214_app.i"
	//#include "LQ_CM280_FT5346_Ofilm0x51_V04_D01_20151217_app.i"
	//#include "LQ_CM320_Ofilm0x51_V03_D01_20151214_app.i"
	//#include "LQ_CM320_Ofilm0x51_V04_D01_20151222_app.i"
	//  #include "LQ_CU320_Ofilm0x51_V07_D01_20160118_app.i"
	//  #include "LQ_CM3300_0film0x51_V03_D01_20160223_app.i"
	//  #include "LQ_CM3300_0film0x51_V05_D01_20160329_app.i"
	    #include "LQ_CM3300_0film0x51_V06_D01_20160425_app.i"
};
static unsigned char CTPM_FW_BOE[] = {
       //#include "0x3b_LEM5006+Noah-m_V04_D01_20160301_app.i"    
	   //#include "LQ_CM3300_FT5446i_Biel0x04_V05_D01_20160419_app.i"
	     #include "LQ_FT5446i_L3300_Biel0x3b_V06_D01_20160525_app.i"
};
static unsigned char aucFW_PRAM_BOOT[] = {
	//#include "FT8xx6_Pramboot_Vx.x_xxxx.i"
};

 struct fts_Upgrade_Info fts_updateinfo[] =
{
	{0x54,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x54, 0x2c, 20, 2000}, //,"FT5x46"
       {0x55,FTS_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000}, //,"FT5x06"
       {0x08,FTS_MAX_POINTS_5,AUTO_CLB_NEED,50, 10, 0x79, 0x06, 100, 2000}, //,"FT5606"
	{0x0a,FTS_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x07, 10, 1500}, //,"FT5x16"
	{0x06,FTS_MAX_POINTS_2,AUTO_CLB_NONEED,100, 30, 0x79, 0x08, 10, 2000}, //,"FT6x06"
	{0x36,FTS_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000}, //,"FT6x36"
	{0x64,FTS_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x1c, 10, 2000}, //,"FT6336GU"
	{0x55,FTS_MAX_POINTS_5,AUTO_CLB_NEED,50, 30, 0x79, 0x03, 10, 2000}, //,"FT5x06i"
	{0x14,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000}, //,"FT5336"
	{0x13,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000}, //,"FT3316"
	{0x12,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000}, //,"FT5436i"
	{0x11,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,30, 30, 0x79, 0x11, 10, 2000}, //,"FT5336i"
       {0x58,FTS_MAX_POINTS_5,AUTO_CLB_NONEED,2, 2, 0x58, 0x2c, 20, 2000},//"FT5822"
	{0x59,FTS_MAX_POINTS_10,AUTO_CLB_NONEED,30, 50, 0x79, 0x10, 1, 2000},//"FT5x26"
	{0x86,FTS_MAX_POINTS_10,AUTO_CLB_NONEED,2, 2, 0x86, 0xA6, 20, 2000},//"FT8606"
	{0x87,FTS_MAX_POINTS_10,AUTO_CLB_NONEED,2, 2, 0x87, 0xA6, 20, 2000},//"FT8716"
	{0x0E,FTS_MAX_POINTS_2,AUTO_CLB_NONEED,10, 10, 0x79, 0x18, 10, 2000}, //,"FT3x07"
};
/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
struct fts_Upgrade_Info fts_updateinfo_curr;
/*******************************************************************************
* Static function prototypes
*******************************************************************************/
int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_6336GU_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_5822_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_5x26_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int  fts_8606_writepram(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int  fts_8606_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_8716_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_8716_writepram(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth);
int fts_3x07_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth);
int hidi2c_to_stdi2c(struct i2c_client * client);

/************************************************************************
* Name: hidi2c_to_stdi2c
* Brief:  HID to I2C
* Input: i2c info
* Output: no
* Return: fail =0
***********************************************************************/
int hidi2c_to_stdi2c(struct i2c_client * client)
{
	u8 auc_i2c_write_buf[5] = {0};
	int bRet = 0;
	#if HIDTOI2C_DISABLE	
		return 0;
	#endif
	
	auc_i2c_write_buf[0] = 0xeb;
	auc_i2c_write_buf[1] = 0xaa;
	auc_i2c_write_buf[2] = 0x09;
	bRet =fts_i2c_write(client, auc_i2c_write_buf, 3);
	msleep(10);
	auc_i2c_write_buf[0] = auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = 0;
	fts_i2c_read(client, auc_i2c_write_buf, 0, auc_i2c_write_buf, 3);

	if(0xeb==auc_i2c_write_buf[0] && 0xaa==auc_i2c_write_buf[1] && 0x08==auc_i2c_write_buf[2])
	{
		pr_info("hidi2c_to_stdi2c successful.\n");
		bRet = 1;		
	}
	else 
	{
		pr_err("hidi2c_to_stdi2c error.\n");
		bRet = 0;
	}

	return bRet;
}

/*******************************************************************************
* Name: fts_update_fw_vendor_id
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
void fts_update_fw_vendor_id(struct fts_ts_data *data)
{
	struct i2c_client *client = data->client;
	u8 reg_addr;
	int err;

	reg_addr = FTS_REG_FW_VENDOR_ID;
	err = fts_i2c_read(client, &reg_addr, 1, &data->fw_vendor_id, 1);
	if (err < 0)
		dev_err(&client->dev, "fw vendor id read failed");
}

/*******************************************************************************
* Name: fts_update_fw_ver
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
void fts_update_fw_ver(struct fts_ts_data *data)
{
	struct i2c_client *client = data->client;
	u8 reg_addr;
	int err;

	reg_addr = FTS_REG_FW_VER;
	err = fts_i2c_read(client, &reg_addr, 1, &data->fw_ver[0], 1);
	if (err < 0)
		dev_err(&client->dev, "fw major version read failed");

	reg_addr = FTS_REG_FW_MIN_VER;
	err = fts_i2c_read(client, &reg_addr, 1, &data->fw_ver[1], 1);
	if (err < 0)
		dev_err(&client->dev, "fw minor version read failed");

	reg_addr = FTS_REG_FW_SUB_MIN_VER;
	err = fts_i2c_read(client, &reg_addr, 1, &data->fw_ver[2], 1);
	if (err < 0)
		dev_err(&client->dev, "fw sub minor version read failed");

	dev_info(&client->dev, "Firmware version = %d.%d.%d\n",
		data->fw_ver[0], data->fw_ver[1], data->fw_ver[2]);
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_ReadVendorID
* Brief:  read vendor ID
* Input: i2c info, vendor ID
* Output: no
* Return: fail <0
***********************************************************************/
int fts_ctpm_fw_upgrade_ReadVendorID(struct i2c_client *client, u8 *ucPVendorID)
{
	u8 reg_val[4] = {0};
	u32 i = 0;
	u8 auc_i2c_write_buf[10];
	int i_ret;

	*ucPVendorID = 0;
	i_ret = hidi2c_to_stdi2c(client);
	if (i_ret == 0) 
	{
		TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		i_ret = hidi2c_to_stdi2c(client);
		if (i_ret == 0) 
		{
			TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
		}
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		if (i_ret < 0) {
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}
		/*********Step 3:check READ-ID***********************/
		msleep(10);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) {
			TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			continue;
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;
	/*********Step 4: read vendor id from app param area***********************/
	msleep(10);
	auc_i2c_write_buf[0] = 0x03;
	auc_i2c_write_buf[1] = 0x00;
	auc_i2c_write_buf[2] = 0xd7;
	auc_i2c_write_buf[3] = 0x84;
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		fts_i2c_write(client, auc_i2c_write_buf, 4);		
		msleep(5);
		reg_val[0] = reg_val[1] = 0x00;
		i_ret = fts_i2c_read(client, auc_i2c_write_buf, 0, reg_val, 2);
		if (0 != reg_val[0]) 
		{
			*ucPVendorID = 0;
			TPD_DEBUG("In upgrade Vendor ID Mismatch, REG1 = 0x%x, REG2 = 0x%x, Definition:0x%x, i_ret=%d\n", reg_val[0], reg_val[1], 0, i_ret);
		} 
		else 
		{
			*ucPVendorID = reg_val[0];
			TPD_DEBUG("In upgrade Vendor ID, REG1 = 0x%x, REG2 = 0x%x\n", reg_val[0], reg_val[1]);
			break;
		}
	}
	msleep(50);
	/*********Step 5: reset the new FW***********************/
	TPD_DEBUG("Step 5: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);	
	i_ret = hidi2c_to_stdi2c(client);	
	if (i_ret == 0) 
	{
		TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
	}
	msleep(10);
	return 0;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_ReadProjectCode
* Brief:  read project code
* Input: i2c info, project code
* Output: no
* Return: fail <0
***********************************************************************/
int fts_ctpm_fw_upgrade_ReadProjectCode(struct i2c_client *client, char *pProjectCode)
{
	u8 reg_val[4] = {0};
	u32 i = 0;
	u8 j = 0;
	u8 auc_i2c_write_buf[10];
	int i_ret;
	u32 temp;
	i_ret = hidi2c_to_stdi2c(client);
	if (i_ret == 0) 
	{
		TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
	}	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		i_ret = hidi2c_to_stdi2c(client);
		if (i_ret == 0) 
		{
			TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
		}
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		if (i_ret < 0) 
		{
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}
		/*********Step 3:check READ-ID***********************/
		msleep(10);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			break;
		}
		else
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			continue;
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;
	/*********Step 4: read vendor id from app param area***********************/
	msleep(10);
	/*read project code*/
	auc_i2c_write_buf[0] = 0x03;
	auc_i2c_write_buf[1] = 0x00;
	for (j = 0; j < 33; j++) 
	{
		temp = 0xD7A0 + j;
		auc_i2c_write_buf[2] = (u8)(temp>>8);
		auc_i2c_write_buf[3] = (u8)temp;
		fts_i2c_read(client, auc_i2c_write_buf, 4, pProjectCode+j, 1);
		if (*(pProjectCode+j) == '\0')
			break;
	}
	pr_info("project code = %s \n", pProjectCode);
	msleep(50);
	/*********Step 5: reset the new FW***********************/
	TPD_DEBUG("Step 5: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);
	i_ret = hidi2c_to_stdi2c(client);	
	if (i_ret == 0) 
	{
		TPD_DEBUG("hidi2c change to stdi2c fail ! \n");
	}
	msleep(10);
	return 0;
}

/************************************************************************
* Name: fts_get_upgrade_array
* Brief: decide which ic
* Input: no
* Output: get ic info in fts_updateinfo_curr
* Return: no
***********************************************************************/
void fts_get_upgrade_array(void)
{

	u8 chip_id;
	u32 i;
	int ret = 0;
#ifdef Boot_Upgrade_Protect
	chip_id = FTS_CHIP_ID; 
#else
	ret = fts_read_reg(fts_i2c_client, FTS_REG_ID,&chip_id);
#endif
	if (ret<0) 
	{
		printk("[Focal][Touch] read value fail");
	}
	printk("%s chip_id = %x\n", __func__, chip_id);

	for(i=0;i<sizeof(fts_updateinfo)/sizeof(struct fts_Upgrade_Info);i++)
	{
		if(chip_id==fts_updateinfo[i].CHIP_ID)
		{
			memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct fts_Upgrade_Info));
			break;
		}
	}

	if(i >= sizeof(fts_updateinfo)/sizeof(struct fts_Upgrade_Info))
	{
		memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct fts_Upgrade_Info));
	}
}

/************************************************************************
* Name: fts_ctpm_auto_clb
* Brief:  auto calibration
* Input: i2c info
* Output: no
* Return: 0
***********************************************************************/
int fts_ctpm_auto_clb(struct i2c_client *client)
{
	unsigned char uc_temp = 0x00;
	unsigned char i = 0;

	/*start auto CLB */
	msleep(200);

	fts_write_reg(client, 0, FTS_FACTORYMODE_VALUE);
	/*make sure already enter factory mode */
	msleep(100);
	/*write command to start calibration */
	fts_write_reg(client, 2, 0x4);
	msleep(300);
	if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14)) //5x36,5x36i
	{
		for(i=0;i<100;i++)
		{
			fts_read_reg(client, 0x02, &uc_temp);
			if (0x02 == uc_temp ||
				0xFF == uc_temp)
			{
			    	break;
			}
			msleep(20);	    
		}
	} 
	else 
	{
		for(i=0;i<100;i++)
		{
			fts_read_reg(client, 0, &uc_temp);
			if (0x0 == ((uc_temp&0x70)>>4)) 
			{
			    	break;
			}
			msleep(20);	    
		}
	}
	fts_write_reg(client, 0, 0x40); 
	msleep(200);   
	fts_write_reg(client, 2, 0x5); 
	msleep(300);
	fts_write_reg(client, 0, FTS_WORKMODE_VALUE);
	msleep(300);
	return 0;
}

/************************************************************************
* Name: fts_6x36_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_6x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u32 fw_length;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;

	if(pbt_buf[0] != 0x02)
	{
		TPD_DEBUG("[FTS] FW first byte is not 0x02. so it is invalid \n");
		return -1;
	}

	if(dw_lenth > 0x11f)
	{
		fw_length = ((u32)pbt_buf[0x100]<<8) + pbt_buf[0x101];
		if(dw_lenth < fw_length)
		{
			TPD_DEBUG("[FTS] Fw length is invalid \n");
			return -1;
		}
	}
	else
	{
		TPD_DEBUG("[FTS] Fw length is invalid \n");
		return -1;
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		msleep(fts_updateinfo_curr.delay_readid);
		/*********Step 3:check READ-ID***********************/		
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;

	auc_i2c_write_buf[0] = FTS_READ_ID_REG;
	auc_i2c_write_buf[1] = 0x00;
	auc_i2c_write_buf[2] = 0x00;
	auc_i2c_write_buf[3] = 0x00;
	auc_i2c_write_buf[4] = 0x00;
	fts_i2c_write(client, auc_i2c_write_buf, 5);

	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(fts_updateinfo_curr.delay_erase_flash);

	for(i = 0;i < 200;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		auc_i2c_write_buf[1] = 0x00;
		auc_i2c_write_buf[2] = 0x00;
		auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if(0xb0 == reg_val[0] && 0x02 == reg_val[1])
		{
			TPD_DEBUG("[FTS] erase app finished \n");
			break;
		}
		msleep(50);
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = fw_length;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}


	/*********Step 6: read out checksum***********************/
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	

	return 0;
}
/************************************************************************
* Name: fts_6336GU_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_6336GU_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u32 fw_length;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;

	if(pbt_buf[0] != 0x02)
	{
		TPD_DEBUG("[FTS] FW first byte is not 0x02. so it is invalid \n");
		return -1;
	}

	if(dw_lenth > 0x11f)
	{
		fw_length = ((u32)pbt_buf[0x100]<<8) + pbt_buf[0x101];
		if(dw_lenth < fw_length)
		{
			TPD_DEBUG("[FTS] Fw length is invalid \n");
			return -1;
		}
	}
	else
	{
		TPD_DEBUG("[FTS] Fw length is invalid \n");
		return -1;
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		msleep(fts_updateinfo_curr.delay_readid);
		/*********Step 3:check READ-ID***********************/		
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;

	auc_i2c_write_buf[0] = FTS_READ_ID_REG;
	auc_i2c_write_buf[1] = 0x00;
	auc_i2c_write_buf[2] = 0x00;
	auc_i2c_write_buf[3] = 0x00;
	auc_i2c_write_buf[4] = 0x00;
	fts_i2c_write(client, auc_i2c_write_buf, 5);

	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(fts_updateinfo_curr.delay_erase_flash);

	for(i = 0;i < 200;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		auc_i2c_write_buf[1] = 0x00;
		auc_i2c_write_buf[2] = 0x00;
		auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if(0xb0 == reg_val[0] && 0x02 == reg_val[1])
		{
			TPD_DEBUG("[FTS] erase app finished \n");
			break;
		}
		msleep(50);
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = fw_length;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}


	/*********Step 6: read out checksum***********************/
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	

	return 0;
}
/************************************************************************
* Name: fts_6x06_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_6x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/		
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_55);

		msleep(fts_updateinfo_curr.delay_55);

		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		do 
		{
			i++;
			i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
			msleep(5);
		} while (i_ret <= 0 && i < 5);


		/*********Step 3:check READ-ID***********************/
		msleep(fts_updateinfo_curr.delay_readid);
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: CTPM ID OK ,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
		}
	}
	if (i > FTS_UPGRADE_LOOP)
		return -EIO;
	auc_i2c_write_buf[0] = 0xcd;

	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);


	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);	
	msleep(fts_updateinfo_curr.delay_erase_flash);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = FTS_ERASE_PARAMS_CMD;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = dw_lenth - 8;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
	{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
	{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_write(client, packet_buf, temp + 6);
		msleep(20);
	}


	/*send the last six byte */
	for (i = 0; i < 6; i++) 
	{
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		fts_i2c_write(client, packet_buf, 7);
		msleep(20);
	}


	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",reg_val[0],bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = FTS_REG_RESET_FW;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	

	return 0;
}
/************************************************************************
* Name: fts_5x26_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_5x26_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret=0;

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{

		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);

		/*********Step 2:Enter upgrade mode and switch protocol*****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		if (i_ret < 0) 
		{
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}
		
		/*********Step 3:check READ-ID***********************/
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
		{	
			TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP) return -EIO;
	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_write(client, auc_i2c_write_buf, 1);		
	/*erase app area*/	
	auc_i2c_write_buf[0] = 0x63;
	fts_i2c_write(client, auc_i2c_write_buf, 1);		
	/*erase panel paramenter area*/	
	auc_i2c_write_buf[0] = 0x04;
    	fts_i2c_write(client, auc_i2c_write_buf, 1);     
	/*erase panel paramenter area*/
	 msleep(fts_updateinfo_curr.delay_erase_flash);
	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++)
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;
		
		for (i = 0; i < FTS_PACKET_LENGTH; i++)
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);	
	}
	
	if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		
		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, temp+6);
		msleep(20);
	}
	/*********Step 6: read out checksum***********************/
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	reg_val[0] = reg_val[1] = 0x00;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	printk(KERN_WARNING "Checksum FT5X26:%X %X \n", reg_val[0], bt_ecc);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",reg_val[0],bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);

	/********Step 8 Disable Write Flash*****/
	TPD_DEBUG("Step 8: Disable Write Flash\n");
    	auc_i2c_write_buf[0] = 0x04;
    	fts_i2c_write(client, auc_i2c_write_buf, 1);

	msleep(300);	
	auc_i2c_write_buf[0] =auc_i2c_write_buf[1]= 0x00;
	fts_i2c_write(client,auc_i2c_write_buf,2);

	return 0;
}

/************************************************************************
* Name: fts_5x36_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_5x36_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u8 is_5336_new_bootloader = 0;
	u8 is_5336_fwsize_30 = 0;
	u32  packet_number;
	u32  j;
	u32  temp;
	u32  lenght;
	u8 	packet_buf[FTS_PACKET_LENGTH + 6];
	u8  	auc_i2c_write_buf[10];
	u8  	bt_ecc;
	int	i_ret;
	int	fw_filenth = sizeof(CTPM_FW);

	if(CTPM_FW[fw_filenth-12] == 30)
	{
		is_5336_fwsize_30 = 1;
	}
	else 
	{
		is_5336_fwsize_30 = 0;
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
    	/*********Step 1:Reset  CTPM *****/
	   	fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		
		 /*write 0x55 to register FTS_RST_CMD_REG1*/
		fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_55);   
		msleep(fts_updateinfo_curr.delay_55);   


		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		
	    i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
	  
	    /*********Step 3:check READ-ID***********************/   
		msleep(fts_updateinfo_curr.delay_readid);
	   	auc_i2c_write_buf[0] = FTS_READ_ID_REG; 
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
              fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);           
              if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 
                              && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
              {
                      dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]); 
                      break; 
              }
              else
              {
                      dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAILD,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
                      continue;
              }

          }

          if (i >= FTS_UPGRADE_LOOP)
                          return -EIO;
		  
         auc_i2c_write_buf[0] = 0xcd;
         fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	/*********20130705 mshl ********************/
	if (reg_val[0] <= 4)
	{
		is_5336_new_bootloader = BL_VERSION_LZ4 ;
	}
	else if(reg_val[0] == 7)
	{
		is_5336_new_bootloader = BL_VERSION_Z7 ;
	}
	else if(reg_val[0] >= 0x0f)
	{
		is_5336_new_bootloader = BL_VERSION_GZF ;
	}

     /*********Step 4:erase app and panel paramenter area ********************/
	if(is_5336_fwsize_30)
	{
		auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
		fts_i2c_write(client, auc_i2c_write_buf, 1); 
   		 msleep(fts_updateinfo_curr.delay_erase_flash); 

		 auc_i2c_write_buf[0] = FTS_ERASE_PARAMS_CMD;
		fts_i2c_write(client, auc_i2c_write_buf, 1); 
   		 msleep(50);
	}
	else
	{
		auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
		fts_i2c_write(client, auc_i2c_write_buf, 1); 
   		msleep(fts_updateinfo_curr.delay_erase_flash); 
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;

	if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )
	{
		dw_lenth = dw_lenth - 8;
	}
	else if(is_5336_new_bootloader == BL_VERSION_GZF) 
	{
		dw_lenth = dw_lenth - 14;
	}
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;
	for (j=0;j<packet_number;j++)
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(lenght>>8);
		packet_buf[5] = (u8)lenght;

		for (i=0;i<FTS_PACKET_LENGTH;i++)
		{
		    packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
		    bt_ecc ^= packet_buf[6+i];
		}

		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH+6);
		msleep(FTS_PACKET_LENGTH/6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;

		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(temp>>8);
		packet_buf[5] = (u8)temp;

		for (i=0;i<temp;i++)
		{
		    packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
		    bt_ecc ^= packet_buf[6+i];
		}
  
		fts_i2c_write(client, packet_buf, temp+6);
		msleep(20);
	}
        /*send the last six byte*/
        if(is_5336_new_bootloader == BL_VERSION_LZ4 || is_5336_new_bootloader == BL_VERSION_Z7 )
        {
                for (i = 0; i<6; i++)
                {
                                if (is_5336_new_bootloader  == BL_VERSION_Z7) 
                                {
                                                temp = 0x7bfa + i;
                                }
                                else if(is_5336_new_bootloader == BL_VERSION_LZ4)
                                {
                                                temp = 0x6ffa + i;
                                }
                                packet_buf[2] = (u8)(temp>>8);
                                packet_buf[3] = (u8)temp;
                                temp =1;
                                packet_buf[4] = (u8)(temp>>8);
                                packet_buf[5] = (u8)temp;
                                packet_buf[6] = pbt_buf[ dw_lenth + i]; 
                                bt_ecc ^= packet_buf[6];
                                fts_i2c_write(client, packet_buf, 7);
                                msleep(10);
                }
        }
        else if(is_5336_new_bootloader == BL_VERSION_GZF)
        {
		for (i = 0; i<12; i++)
		{
		        if (is_5336_fwsize_30) 
		        {
		                        temp = 0x7ff4 + i;
		        }
		        else
		        {
		                        temp = 0x7bf4 + i;
		        }
		        packet_buf[2] = (u8)(temp>>8);
		        packet_buf[3] = (u8)temp;
		        temp =1;
		        packet_buf[4] = (u8)(temp>>8);
		        packet_buf[5] = (u8)temp;
		        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
		        bt_ecc ^= packet_buf[6];
			fts_i2c_write(client, packet_buf, 7);
			msleep(10);
		}
	}

	/*********Step 6: read out checksum***********************/
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1); 
	if(reg_val[0] != bt_ecc)
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n", reg_val[0], bt_ecc);
	    	return -EIO;
	}
	/*********Step 7: reset the new FW***********************/
	auc_i2c_write_buf[0] = FTS_REG_RESET_FW;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300); 

	return 0;
}
/************************************************************************
* Name: fts_5822_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_5822_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	u8 bt_ecc_check;
	int i_ret;

	i_ret = hidi2c_to_stdi2c(client);
	if (i_ret == 0) 
	{
		TPD_DEBUG("HidI2c change to StdI2c fail ! \n");
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		i_ret = hidi2c_to_stdi2c(client);
		if (i_ret == 0) 
		{
			TPD_DEBUG("HidI2c change to StdI2c fail ! \n");
		}
		msleep(5);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);	
		if (i_ret < 0) 
		{
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}
		/*********Step 3:check READ-ID***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{	
			TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
			continue;
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;
	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_write(client, auc_i2c_write_buf, 1);		
	msleep(1350);
	for (i = 0; i < 15; i++) 
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
		if (0xF0 == reg_val[0] && 0xAA == reg_val[1]) 
		{
			break;
		}
		msleep(50);
	}
	printk("[FTS][%s] erase app area reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
	auc_i2c_write_buf[0] = 0xB0;
	auc_i2c_write_buf[1] = (u8) ((dw_lenth >> 16) & 0xFF);
	auc_i2c_write_buf[2] = (u8) ((dw_lenth >> 8) & 0xFF);
	auc_i2c_write_buf[3] = (u8) (dw_lenth & 0xFF);
	fts_i2c_write(client, auc_i2c_write_buf, 4);
	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	bt_ecc_check = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;
		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc_check ^= pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		printk("[FTS][%s] bt_ecc = %x \n", __func__, bt_ecc);
		if (bt_ecc != bt_ecc_check)
			printk("[FTS][%s] Host checksum error bt_ecc_check = %x \n", __func__, bt_ecc_check);
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		for (i = 0; i < 30; i++) 
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1])) {
				break;
			}
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			msleep(1);
		}
	}
	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc_check ^= pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, temp + 6);
		printk("[FTS][%s] bt_ecc = %x \n", __func__, bt_ecc);
		if (bt_ecc != bt_ecc_check)
			printk("[FTS][%s] Host checksum error bt_ecc_check = %x \n", __func__, bt_ecc_check);
		for (i = 0; i < 30; i++) 
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1])) 
			{
				break;
			}
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			msleep(1);
		}
	}
	msleep(50);
	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);
	temp = 0;
	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);
	temp = dw_lenth;
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6);
	msleep(dw_lenth/256);
	for (i = 0; i < 100; i++) 
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
		dev_err(&client->dev, "[FTS]--reg_val[0]=%02x reg_val[0]=%02x\n", reg_val[0], reg_val[1]);
		if (0xF0 == reg_val[0] && 0x55 == reg_val[1]) 
		{
			dev_err(&client->dev, "[FTS]--reg_val[0]=%02x reg_val[0]=%02x\n", reg_val[0], reg_val[1]);
			break;
		}
		msleep(1);
	}
	auc_i2c_write_buf[0] = 0x66;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n", reg_val[0], bt_ecc);
		return -EIO;
	}
	printk(KERN_WARNING "checksum %X %X \n", reg_val[0], bt_ecc);
	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);	
	i_ret = hidi2c_to_stdi2c(client);
	if (i_ret == 0) 
	{
		TPD_DEBUG("HidI2c change to StdI2c fail ! \n");
	}
	return 0;
}

/************************************************************************
* Name: fts_5x06_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_5x06_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register FTS_RST_CMD_REG1 */
		fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		/*write 0x55 to register FTS_RST_CMD_REG1 */
		fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		do 
		{
			i++;
			i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
			msleep(5);
		} while (i_ret <= 0 && i < 5);


		/*********Step 3:check READ-ID***********************/
		msleep(fts_updateinfo_curr.delay_readid);
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;
	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(fts_updateinfo_curr.delay_erase_flash);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = FTS_ERASE_PARAMS_CMD;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");
	dw_lenth = dw_lenth - 8;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;
	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;
		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}
	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_write(client, packet_buf, temp + 6);
		msleep(20);
	}
	/*send the last six byte */
	for (i = 0; i < 6; i++) 
	{
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		fts_i2c_write(client, packet_buf, 7);
		msleep(20);
	}
	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}		
	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = FTS_REG_RESET_FW;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */
	return 0;
}

/************************************************************************
* Name: fts_5x46_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int  fts_5x46_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{	
	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;

	i_ret = hidi2c_to_stdi2c(client);
	if(i_ret == 0)
	{
		TPD_DEBUG("[FTS] hid change to i2c fail ! \n");
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		/*write 0xaa to register FTS_RST_CMD_REG1 */
		fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);

		//write 0x55 to register FTS_RST_CMD_REG1 
		fts_write_reg(client, FTS_RST_CMD_REG1, FTS_UPGRADE_55);
		msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		i_ret = hidi2c_to_stdi2c(client);

		if(i_ret == 0)
		{
			TPD_DEBUG("[FTS] hid change to i2c fail ! \n");
		}
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);	
		if(i_ret < 0)
		{
			TPD_DEBUG("[FTS] failed writing  0x55 and 0xaa ! \n");
			continue;
		}
		/*********Step 3:check READ-ID***********************/
		msleep(1);
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;		
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
			
			continue;
		}
	}
	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;
	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(1350);
	for(i = 0;i < 15;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
		if(0xF0==reg_val[0] && 0xAA==reg_val[1])
		{
			break;
		}
		msleep(50);		
	}
	printk("[FTS][%s] erase app area reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
	auc_i2c_write_buf[0] = 0xB0;
	auc_i2c_write_buf[1] = (u8) ((dw_lenth >> 16) & 0xFF);
	auc_i2c_write_buf[2] = (u8) ((dw_lenth >> 8) & 0xFF);
	auc_i2c_write_buf[3] = (u8) (dw_lenth & 0xFF);
	fts_i2c_write(client, auc_i2c_write_buf, 4);
	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;
		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
	    {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(20);
		/*
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			msleep(1);			
		}*/
	}
	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		for (i = 0; i < temp; i++) 
	{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}	
		fts_i2c_write(client, packet_buf, temp + 6);
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			printk("[FTS][%s] reg_val[0] = %x reg_val[1] = %x \n", __func__, reg_val[0], reg_val[1]);
			msleep(1);
			
		}
	}

	msleep(50);
	
	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	fts_i2c_write(client, auc_i2c_write_buf, 1); 
	msleep(300);

	temp = 0;
	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);
	temp = dw_lenth;
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6); 
	msleep(dw_lenth/256);

	for(i = 0;i < 100;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);
		dev_err(&client->dev, "[FTS]--reg_val[0]=%02x reg_val[0]=%02x\n", reg_val[0], reg_val[1]);
		if (0xF0==reg_val[0] && 0x55==reg_val[1])
		{
			dev_err(&client->dev, "[FTS]--reg_val[0]=%02x reg_val[0]=%02x\n", reg_val[0], reg_val[1]);
			break;
		}
		msleep(1);
			
	}
	auc_i2c_write_buf[0] = 0x66;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
				
		return -EIO;
	}
	printk(KERN_WARNING "checksum %X %X \n",reg_val[0],bt_ecc);	
	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = FTS_REG_RESET_FW;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);	
	i_ret = hidi2c_to_stdi2c(client);
	if (i_ret == 0) 
	{
		TPD_DEBUG("HidI2c change to StdI2c fail ! \n");
	}
	return 0;
}

/************************************************************************
*   Name: fts_8606_writepram
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int  fts_8606_writepram(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{

	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;

	TPD_DEBUG("8606 dw_lenth= %d",dw_lenth);
	if(dw_lenth > 0x10000 || dw_lenth ==0)
	{		
		return -EIO;
	}
	
	for (i = 0; i < 20; i++) 
	{
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 1);
		if(i_ret < 0)
		{
			TPD_DEBUG("[FTS] failed writing  0x55 ! \n");
			continue;
		}
		
		/*********Step 3:check READ-ID***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		reg_val[0] = reg_val[1] = 0x00;
		
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == 0x86
			&& reg_val[1] == 0x06) || (reg_val[0] == 0x86
			&& reg_val[1] == 0x07)) 
		{
			msleep(50);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			
			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;

	/*********Step 4:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");	
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xae;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}		
		fts_i2c_write(client, packet_buf, temp + 6);
	}

	/*********Step 5: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",reg_val[0],bt_ecc);	
		return -EIO;
	}
	TPD_DEBUG("checksum %X %X \n",reg_val[0],bt_ecc);
	TPD_DEBUG("Read flash and compare\n");
		
	msleep(50);

	/*********Step 6: start app***********************/
	TPD_DEBUG("Step 6: start app\n");
	auc_i2c_write_buf[0] = 0x08;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(20);

	return 0;
}

/************************************************************************
*   Name: fts_8606_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int  fts_8606_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u8 reg_val_id[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	unsigned char cmd[20];
	unsigned char Checksum = 0;
	
	auc_i2c_write_buf[0] = 0x05;
	reg_val_id[0] = 0x00;
		
	i_ret =fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val_id, 1);
	if(dw_lenth == 0)
	{
		

		return -EIO;
	}
	
	if(0x81 == (int)reg_val_id[0]) 
	{
		if(dw_lenth > 1024*60) 
		{
			return -EIO;
		}
	}
	else if(0x80 == (int)reg_val_id[0]) 
	{
		if(dw_lenth > 1024*64) 
		{
			return -EIO;
		}
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		if(i_ret < 0)
		{
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}

		/*********Step 3:check READ-ID***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

		reg_val[0] = reg_val[1] = 0x00;

		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2)|| (reg_val[0] == 0x86 && reg_val[1] == 0xA6)) {
				TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
					reg_val[0], reg_val[1]);
				break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);

			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;

	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	
	{		
		cmd[0] = 0x05;
		cmd[1] = reg_val_id[0];//0x80;
		cmd[2] = 0x00;//???	
		fts_i2c_write(client, cmd, 3);
	}
	
	{
		cmd[0] = 0x09;
		cmd[1] = 0x0B;
		fts_i2c_write(client, cmd, 2);
	}
	
	for(i=0; i<dw_lenth ; i++)
	{		
		Checksum ^= pbt_buf[i];
	}
	msleep(50);
	
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_write(client, auc_i2c_write_buf, 1);     
	msleep(1350);

	for(i = 0;i < 15;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

		if(0xF0==reg_val[0] && 0xAA==reg_val[1])
		{
			break;
		}
		msleep(50);
	}
	
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;

	for (j = 0; j < packet_number; j++) {
		temp = 0x1000+j * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);

		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = 0x1000+packet_number * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}        
		fts_i2c_write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if ((j + 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);

		}
	}

	msleep(50);

	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	fts_i2c_write(client, auc_i2c_write_buf, 1); 
	msleep(300);	
	temp = 0x1000+0;
		
	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);
	
	if (dw_lenth > LEN_FLASH_ECC_MAX)
	{
		temp = LEN_FLASH_ECC_MAX;
	}
	else
	{
		temp = dw_lenth;
		TPD_DEBUG("Step 6_1: read out checksum\n");
	}
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6); 
	msleep(dw_lenth/256);

	for(i = 0;i < 100;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

		if (0xF0==reg_val[0] && 0x55==reg_val[1])
		{
			break;
		}
		msleep(1);

	}
	//----------------------------------------------------------------------
	if (dw_lenth > LEN_FLASH_ECC_MAX)
	{
		temp = LEN_FLASH_ECC_MAX;//??? 0x1000+LEN_FLASH_ECC_MAX
		auc_i2c_write_buf[0] = 0x65;
		auc_i2c_write_buf[1] = (u8)(temp >> 16);
		auc_i2c_write_buf[2] = (u8)(temp >> 8);
		auc_i2c_write_buf[3] = (u8)(temp);
		temp = dw_lenth-LEN_FLASH_ECC_MAX;
		auc_i2c_write_buf[4] = (u8)(temp >> 8);
		auc_i2c_write_buf[5] = (u8)(temp);
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6); 
		
		msleep(dw_lenth/256);

		for(i = 0;i < 100;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if (0xF0==reg_val[0] && 0x55==reg_val[1])
			{
				break;
			}
			msleep(1);

		}
	}
	auc_i2c_write_buf[0] = 0x66;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
			reg_val[0],
			bt_ecc);

		return -EIO;
	}
	TPD_DEBUG("checksum %X %X \n",reg_val[0],bt_ecc);       
	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);   //make sure CTP startup normally 
	return 0;
}

/************************************************************************
* Name: fts_8716_ctpm_fw_write_pram
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int  fts_8716_writepram(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{

	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;

	TPD_DEBUG("8716 dw_lenth= %d",dw_lenth);
	if(dw_lenth > 0x10000 || dw_lenth ==0)
	{		
		return -EIO;
	}
	
	for (i = 0; i < 20; i++) 
	{
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/********* Step 2:Enter upgrade mode *****/

		i_ret = hidi2c_to_stdi2c(client);
		if(i_ret == 0)
		{
			TPD_DEBUG("[FTS] hid change to i2c fail ! \n");
		}

		msleep(10);
		
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 1);
		if(i_ret < 0)
		{
			TPD_DEBUG("[FTS] failed writing  0x55 ! \n");
			continue;
		}
		
		/********* Step 3:check READ-ID ***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		reg_val[0] = reg_val[1] = 0x00;
		
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == 0x87
			&& reg_val[1] == 0x16)) 
		{
			TPD_DEBUG("[FTS] Step 3: READ CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: READ CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			
			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;

	/********* Step 4:write firmware(FW) to ctpm flash *********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");
	
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xae;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}		
		fts_i2c_write(client, packet_buf, temp + 6);
	}
	
	/********* Step 5: read out checksum ***********************/
	/* send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0xcc;
	msleep(2);
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",reg_val[0],bt_ecc);	
		return -EIO;
	}
	printk(KERN_WARNING "checksum %X %X \n",reg_val[0],bt_ecc);
	msleep(100);	

	/********* Step 6: start app ***********************/
	TPD_DEBUG("Step 6: start app\n");
	auc_i2c_write_buf[0] = 0x08;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(20);

	return 0;
}

/************************************************************************
* Name: fts_8716_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int  fts_8716_ctpm_fw_upgrade(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u8 reg_val_id[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	unsigned char cmd[20];
	unsigned char Checksum = 0;

	i_ret = hidi2c_to_stdi2c(client);
	if(i_ret == 0)
	{
		TPD_DEBUG("[FTS] hid change to i2c fail ! \n");
	}

	auc_i2c_write_buf[0] = 0x05;
	reg_val_id[0] = 0x00;
		
	i_ret =fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val_id, 1);
	
	if(dw_lenth == 0)
	{		
		return -EIO;
	}
	
	if(0x81 == (int)reg_val_id[0]) 
	{
		if(dw_lenth > 1024*60) 
		{
			return -EIO;
		}
	}
	else if(0x80 == (int)reg_val_id[0]) 
	{
		if(dw_lenth > 1024*64) 
		{
			return -EIO;
		}
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/********* Step 1:Reset  CTPM *****/
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		if(i_ret < 0)
		{
			TPD_DEBUG("failed writing  0x55 and 0xaa ! \n");
			continue;
		}

		/********* Step 3:check READ-ID ***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

		reg_val[0] = reg_val[1] = 0x00;

		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2)/*|| (reg_val[0] == 0x87 && reg_val[1] == 0xA6)*/) {
				TPD_DEBUG("[FTS] Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
					reg_val[0], reg_val[1]);
				break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3: READ FAIL CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			continue;
		}
	}	
	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;	
	/* Step 4:erase app and panel paramenter area */
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	
	{		
		cmd[0] = 0x05;
		cmd[1] = reg_val_id[0];//0x80;
		cmd[2] = 0x00;	
		fts_i2c_write(client, cmd, 3);
	}
	
	{
		cmd[0] = 0x09;
		cmd[1] = 0x0B;
		fts_i2c_write(client, cmd, 2);
	}	
	for(i=0; i<dw_lenth ; i++)
	{
		
		Checksum ^= pbt_buf[i];
	}
	msleep(50);
	
	auc_i2c_write_buf[0] = 0x61;
	fts_i2c_write(client, auc_i2c_write_buf, 1);     
	msleep(1350);

	for(i = 0;i < 15;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

		if(0xF0==reg_val[0] && 0xAA==reg_val[1])
		{
			break;
		}
		msleep(50);
	}
	
	/********* Step 5:write firmware(FW) to ctpm flash *********/	
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	for (j = 0; j < packet_number; j++) {
		temp = 0x1000+j * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if ((j + 0x22+0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);

		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = 0x1000+packet_number * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}        
		fts_i2c_write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if ((j + 0x22+ 0x1000) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
			msleep(1);

		}
	}

	msleep(50);

	/********* Step 6: read out checksum ***********************/
	/*send the opration head */
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	fts_i2c_write(client, auc_i2c_write_buf, 1); 
	msleep(300);
	
	temp = 0x1000+0;
	
	
	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);
	
	if (dw_lenth > LEN_FLASH_ECC_MAX)
	{
		temp = LEN_FLASH_ECC_MAX;
	}
	else
	{
		temp = dw_lenth;
		TPD_DEBUG("Step 6_1: read out checksum\n");
	}
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6); 
	msleep(dw_lenth/256);

	for(i = 0;i < 100;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

		if (0xF0==reg_val[0] && 0x55==reg_val[1])
		{
			break;
		}
		msleep(1);

	}
	//----------------------------------------------------------------------
	if (dw_lenth > LEN_FLASH_ECC_MAX)
	{
		temp = LEN_FLASH_ECC_MAX;
		auc_i2c_write_buf[0] = 0x65;
		auc_i2c_write_buf[1] = (u8)(temp >> 16);
		auc_i2c_write_buf[2] = (u8)(temp >> 8);
		auc_i2c_write_buf[3] = (u8)(temp);
		temp = dw_lenth-LEN_FLASH_ECC_MAX;
		auc_i2c_write_buf[4] = (u8)(temp >> 8);
		auc_i2c_write_buf[5] = (u8)(temp);
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 6); 
	
		msleep(dw_lenth/256);

		for(i = 0;i < 100;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 2);

			if (0xF0==reg_val[0] && 0x55==reg_val[1])
			{
				break;
			}
			msleep(1);

		}
	}
	auc_i2c_write_buf[0] = 0x66;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
			reg_val[0],bt_ecc);
		return -EIO;
	}
	printk(KERN_WARNING "checksum %X %X \n",reg_val[0],bt_ecc);       
	/********* Step 7: reset the new FW ***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);  

	return 0;
}

/************************************************************************
* Name: fts_3x07_ctpm_fw_upgrade
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_3x07_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u32 fw_length;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;

	if(pbt_buf[0] != 0x02)
	{
		TPD_DEBUG("[FTS] FW first byte is not 0x02. so it is invalid \n");
		return -1;
	}

	if(dw_lenth > 0x11f)
	{
		fw_length = ((u32)pbt_buf[0x100]<<8) + pbt_buf[0x101];
		if(dw_lenth < fw_length)
		{
			TPD_DEBUG("[FTS] Fw length is invalid \n");
			return -1;
		}
	}
	else
	{
		TPD_DEBUG("[FTS] Fw length is invalid \n");
		return -1;
	}
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		/*********Step 1:Reset  CTPM *****/
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_AA);
		msleep(fts_updateinfo_curr.delay_aa);
		fts_write_reg(client, FTS_RST_CMD_REG2, FTS_UPGRADE_55);
		msleep(fts_updateinfo_curr.delay_55);
		/*********Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
		fts_i2c_write(client, auc_i2c_write_buf, 1);
		msleep(fts_updateinfo_curr.delay_readid);
		/*********Step 3:check READ-ID***********************/		
		auc_i2c_write_buf[0] = FTS_READ_ID_REG;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);


		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] == fts_updateinfo_curr.upgrade_id_2) 
		{
			TPD_DEBUG("[FTS] Step 3: GET CTPM ID OK,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		} 
		else 
		{
			dev_err(&client->dev, "[FTS] Step 3: GET CTPM ID FAIL,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
		}
	}
	if (i >= FTS_UPGRADE_LOOP)
		return -EIO;

	auc_i2c_write_buf[0] = FTS_READ_ID_REG;
	auc_i2c_write_buf[1] = 0x00;
	auc_i2c_write_buf[2] = 0x00;
	auc_i2c_write_buf[3] = 0x00;
	auc_i2c_write_buf[4] = 0x00;
	fts_i2c_write(client, auc_i2c_write_buf, 5);

	/*Step 4:erase app and panel paramenter area*/
	TPD_DEBUG("Step 4:erase app and panel paramenter area\n");
	auc_i2c_write_buf[0] = FTS_ERASE_APP_REG;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(fts_updateinfo_curr.delay_erase_flash);

	for(i = 0;i < 200;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		auc_i2c_write_buf[1] = 0x00;
		auc_i2c_write_buf[2] = 0x00;
		auc_i2c_write_buf[3] = 0x00;
		reg_val[0] = 0x00;
		reg_val[1] = 0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
		if(0xb0 == reg_val[0] && 0x02 == reg_val[1])
		{
			TPD_DEBUG("[FTS] erase app finished \n");
			break;
		}
		msleep(50);
	}

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;
	TPD_DEBUG("Step 5:write firmware(FW) to ctpm flash\n");

	dw_lenth = fw_length;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = FTS_FW_WRITE_CMD;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) 
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) 
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		
		fts_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		
		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) 
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) 
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		fts_i2c_write(client, packet_buf, temp + 6);

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			auc_i2c_write_buf[1] = 0x00;
			auc_i2c_write_buf[2] = 0x00;
			auc_i2c_write_buf[3] = 0x00;
			reg_val[0] = 0x00;
			reg_val[1] = 0x00;
			fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);
			if(0xb0 == (reg_val[0] & 0xf0) && (0x03 + (j % 0x0ffd)) == (((reg_val[0] & 0x0f) << 8) |reg_val[1]))
			{
				TPD_DEBUG("[FTS] write a block data finished \n");
				break;
			}
			msleep(1);
		}
	}


	/*********Step 6: read out checksum***********************/
	TPD_DEBUG("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = FTS_REG_ECC;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) 
	{
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	TPD_DEBUG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	

	return 0;
}
/*
*note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
/************************************************************************
* Name: fts_GetFirmwareSize
* Brief:  get file size
* Input: file name
* Output: no
* Return: file size
***********************************************************************/
static int fts_GetFirmwareSize(char *firmware_name)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize = 0;
	char filepath[128];

	memset(filepath, 0, sizeof(filepath)); 
       sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH_CONFIG, firmware_name);
	if (NULL == pfile)
	{
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if (IS_ERR(pfile)) 
	{
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}
	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);
	return fsize;
}

/************************************************************************
* Name: fts_ReadFirmware
* Brief:  read firmware buf for .bin file.
* Input: file name, data buf
* Output: data buf
* Return: 0
***********************************************************************/
/*
note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int fts_ReadFirmware(char *firmware_name,unsigned char *firmware_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH_CONFIG, firmware_name);
	if (NULL == pfile)
	{
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if (IS_ERR(pfile)) 
	{
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}
	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, firmware_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);
	return 0;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_app_file
* Brief:  upgrade with *.bin file
* Input: i2c info, file name
* Output: no
* Return: success =0
***********************************************************************/
int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client, char *firmware_name)
{
	u8 *pbt_buf = NULL;
	int i_ret=0;
	int fwsize = fts_GetFirmwareSize(firmware_name);
	printk("**********fts_ctpm_fw_upgrade_with_app_file start****************\n");

	if (fwsize <= 0) 
	{
		dev_err(&client->dev, "%s ERROR:Get firmware size failed\n",__func__);
		return -EIO;
	}
	if (fwsize < 8 || fwsize > 54 * 1024) 
	{
		dev_err(&client->dev, "FW length error\n");
		return -EIO;
	}
	/*=========FW upgrade========================*/
	pbt_buf = (unsigned char *)kmalloc(fwsize + 1, GFP_ATOMIC);
	if (fts_ReadFirmware(firmware_name, pbt_buf)) 
	{
		dev_err(&client->dev, "%s() - ERROR: request_firmware failed\n",__func__);
		kfree(pbt_buf);
		return -EIO;
	}	
	if ((fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x08) ||(fts_updateinfo_curr.CHIP_ID==0x0a))
	{
		i_ret = fts_5x06_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14))
	{
		i_ret = fts_5x36_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x06))
	{
		i_ret = fts_6x06_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x36))
	{
		i_ret = fts_6x36_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x64))
	{
		i_ret = fts_6336GU_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x54))
	{
		i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x58))
	{
		i_ret =  fts_5822_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x59))
	{
		i_ret =  fts_5x26_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x86))
	{
	   	 /*call the upgrade function*/
    		i_ret = fts_8606_writepram(client, aucFW_PRAM_BOOT, sizeof(aucFW_PRAM_BOOT));
	
		if (i_ret != 0)
		{
			dev_err(&client->dev, "%s:fts_8606_writepram failed. err.\n",__func__);
			return -EIO;
		}	
		
		i_ret =  fts_8606_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x87))
	{
	   	 /*call the upgrade function*/
    		i_ret = fts_8716_writepram(client, aucFW_PRAM_BOOT, sizeof(aucFW_PRAM_BOOT));
	
		if (i_ret != 0)
		{
			dev_err(&client->dev, "%s:fts_8716_writepram failed. err.\n",__func__);
			return -EIO;
		}	
		
		i_ret =  fts_8716_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x0E))
	{
		i_ret = fts_3x07_ctpm_fw_upgrade(client, pbt_buf, fwsize);
	}
	if (i_ret != 0)
		dev_err(&client->dev, "%s() - ERROR:[FTS] upgrade failed..\n",
					__func__);
	else if(fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
	{
		fts_ctpm_auto_clb(client);
	}
	
	kfree(pbt_buf);

	return i_ret;
}
/************************************************************************
* Name: fts_ctpm_get_i_file_ver
* Brief:  get .i file version
* Input: no
* Output: no
* Return: fw version
***********************************************************************/
int fts_ctpm_get_i_file_ver(u8 vendor_id)
{
	u16 ui_sz;
	if(vendor_id==0x51 || vendor_id==0x01)
	{
	ui_sz = sizeof(CTPM_FW);
	if (ui_sz > 2)
	{
	    if(fts_updateinfo_curr.CHIP_ID==0x36  || fts_updateinfo_curr.CHIP_ID==0x86  || fts_updateinfo_curr.CHIP_ID==0x64)
                return CTPM_FW[0x10a];
	    else if(fts_updateinfo_curr.CHIP_ID==0x58)
                return CTPM_FW[0x1D0A];
	    else
		return CTPM_FW[ui_sz - 2];
	}
	}
	else if(vendor_id==0x3b || vendor_id==0x04)
	{
     ui_sz = sizeof(CTPM_FW_BOE);
	 if (ui_sz > 2)
	 {
	    if(fts_updateinfo_curr.CHIP_ID==0x36  || fts_updateinfo_curr.CHIP_ID==0x86  || fts_updateinfo_curr.CHIP_ID==0x64)
                return CTPM_FW_BOE[0x10a];
	    else if(fts_updateinfo_curr.CHIP_ID==0x58)
                return CTPM_FW_BOE[0x1D0A];
	    else
		return CTPM_FW_BOE[ui_sz - 2];
	}
	}
	return 0x00;
}
/************************************************************************
* Name: fts_ctpm_update_project_setting
* Brief:  update project setting, only update these settings for COB project, or for some special case
* Input: i2c info
* Output: no
* Return: fail <0
***********************************************************************/
int fts_ctpm_update_project_setting(struct i2c_client *client)
{
	u8 uc_i2c_addr;	
	u8 uc_io_voltage;	
	u8 uc_panel_factory_id;	
	u8 buf[FTS_SETTING_BUF_LEN];
	u8 reg_val[2] = {0};
	u8 auc_i2c_write_buf[10] = {0};
	u8 packet_buf[FTS_SETTING_BUF_LEN + 6];
	u32 i = 0;
	int i_ret;

	uc_i2c_addr = client->addr;
	uc_io_voltage = 0x0;
	uc_panel_factory_id = 0x5a;


	/*Step 1:Reset  CTPM*/
	if(fts_updateinfo_curr.CHIP_ID==0x06 || fts_updateinfo_curr.CHIP_ID==0x36)
	{
		fts_write_reg(client, 0xbc, 0xaa);
	}
	else 
	{
		fts_write_reg(client, 0xfc, 0xaa);
	}
	msleep(50);

	/*write 0x55 to register 0xfc */
	if(fts_updateinfo_curr.CHIP_ID==0x06 || fts_updateinfo_curr.CHIP_ID==0x36)
	{
		fts_write_reg(client, 0xbc, 0x55);
	}
	else
	{
		fts_write_reg(client, 0xfc, 0x55);
	}
	msleep(30);

	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = 0x55;
	auc_i2c_write_buf[1] = 0xaa;
	do 
	{
		i++;
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 2);
		msleep(5);
	} while (i_ret <= 0 && i < 5);


	/*********Step 3:check READ-ID***********************/
	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;

	fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

	if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1 && reg_val[1] == fts_updateinfo_curr.upgrade_id_2)
		dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
	else
		return -EIO;

	auc_i2c_write_buf[0] = 0xcd;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	dev_dbg(&client->dev, "bootloader version = 0x%x\n", reg_val[0]);

	/*--------- read current project setting  ---------- */
	/*set read start address */
	buf[0] = 0x3;
	buf[1] = 0x0;
	buf[2] = 0x78;
	buf[3] = 0x0;

	fts_i2c_read(client, buf, 4, buf, FTS_SETTING_BUF_LEN);
	dev_dbg(&client->dev, "[FTS] old setting: uc_i2c_addr = 0x%x,\
			uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
			buf[0], buf[2], buf[4]);

	 /*--------- Step 4:erase project setting --------------*/
	auc_i2c_write_buf[0] = 0x63;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*----------  Set new settings ---------------*/
	buf[0] = uc_i2c_addr;
	buf[1] = ~uc_i2c_addr;
	buf[2] = uc_io_voltage;
	buf[3] = ~uc_io_voltage;
	buf[4] = uc_panel_factory_id;
	buf[5] = ~uc_panel_factory_id;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	packet_buf[2] = 0x78;
	packet_buf[3] = 0x0;
	packet_buf[4] = 0;
	packet_buf[5] = FTS_SETTING_BUF_LEN;

	for (i = 0; i < FTS_SETTING_BUF_LEN; i++)
		packet_buf[6 + i] = buf[i];

	fts_i2c_write(client, packet_buf, FTS_SETTING_BUF_LEN + 6);
	msleep(100);

	/********* reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);

	msleep(200);
	return 0;
}
/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_i_file
* Brief:  upgrade with *.i file
* Input: i2c info
* Output: no
* Return: fail <0
***********************************************************************/
int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client,u8 vendor_id)
{
	u8 *pbt_buf = NULL;
	int i_ret=0;
	int fw_len = sizeof(CTPM_FW);

	/*judge the fw that will be upgraded
	* if illegal, then stop upgrade and return.
	*/
	if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14)
		||(fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x06) ||(fts_updateinfo_curr.CHIP_ID==0x0a) ||(fts_updateinfo_curr.CHIP_ID==0x08))
	{
		if (fw_len < 8 || fw_len > 32 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}

		if ((CTPM_FW[fw_len - 8] ^ CTPM_FW[fw_len - 6]) == 0xFF
			&& (CTPM_FW[fw_len - 7] ^ CTPM_FW[fw_len - 5]) == 0xFF
			&& (CTPM_FW[fw_len - 3] ^ CTPM_FW[fw_len - 4]) == 0xFF) 
		{
			/*FW upgrade */
			pbt_buf = CTPM_FW;
			/*call the upgrade function */
			if ((fts_updateinfo_curr.CHIP_ID==0x55) ||(fts_updateinfo_curr.CHIP_ID==0x08) ||(fts_updateinfo_curr.CHIP_ID==0x0a))
			{
				i_ret = fts_5x06_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			else if ((fts_updateinfo_curr.CHIP_ID==0x11) ||(fts_updateinfo_curr.CHIP_ID==0x12) ||(fts_updateinfo_curr.CHIP_ID==0x13) ||(fts_updateinfo_curr.CHIP_ID==0x14))
			{
				i_ret = fts_5x36_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			else if ((fts_updateinfo_curr.CHIP_ID==0x06))
			{
				i_ret = fts_6x06_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
			}
			if (i_ret != 0)
				dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
			else if(fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
			{
				fts_ctpm_auto_clb(client);
			}
		} 
		else 
		{
			dev_err(&client->dev, "%s:FW format error\n", __func__);
			return -EBADFD;
		}
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x36))
	{
		if (fw_len < 8 || fw_len > 32 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}
		pbt_buf = CTPM_FW;
		i_ret = fts_6x36_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x64))
	{
		if (fw_len < 8 || fw_len > 48 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}
		pbt_buf = CTPM_FW;
		i_ret = fts_6336GU_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x54))
	{
	    if(vendor_id==0x51||vendor_id==0x01)
	    {
			fw_len=sizeof(CTPM_FW);
            pbt_buf = CTPM_FW;
		}
		else if(vendor_id==0x3b)
		{
         fw_len=sizeof(CTPM_FW_BOE);
         pbt_buf = CTPM_FW_BOE;
		}
		if (fw_len < 8 || fw_len > 54 * 1024) 
		{
			pr_err("FW length error\n");
			return -EIO;
		}
		/*FW upgrade*/
		//pbt_buf = CTPM_FW;
		/*call the upgrade function*/
		//i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		i_ret = fts_5x46_ctpm_fw_upgrade(client, pbt_buf,fw_len );
		if (i_ret != 0)
		{
					dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
		}
		else 
		{
			#ifdef AUTO_CLB
				fts_ctpm_auto_clb(client);  /*start auto CLB*/
			#endif
		}
	}	
	else if ((fts_updateinfo_curr.CHIP_ID==0x58))
	{
		if (fw_len < 8 || fw_len > 54*1024) 
		{
			pr_err("FW length error\n");
			return -EIO;
		}

		/*FW upgrade*/
		pbt_buf = CTPM_FW;
		/*call the upgrade function*/
		i_ret = fts_5822_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0) 
		{
			dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
		} 
		else 
		{
			#ifdef AUTO_CLB
				fts_ctpm_auto_clb(client);  /*start auto CLB*/
			#endif
		}		
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x59))
	{
	    	if (fw_len < 8 || fw_len > 54*1024) 
		{
		    	pr_err("FW length error\n");
	    		return -EIO;
	    	}

	    	/*FW upgrade*/
    		pbt_buf = CTPM_FW;
	   	 /*call the upgrade function*/
    		i_ret = fts_5x26_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
    		if (i_ret != 0) 
		{
    			dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
    		} 
		else 
		{
			#ifdef AUTO_CLB
    				fts_ctpm_auto_clb(client);  /*start auto CLB*/
			#endif
    		}
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x86))
	{
	    	/*FW upgrade*/
    		pbt_buf = CTPM_FW;
	   	 /*call the upgrade function*/
    		i_ret = fts_8606_writepram(client, aucFW_PRAM_BOOT, sizeof(aucFW_PRAM_BOOT));
	
		if (i_ret != 0)
		{
			dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
			return -EIO;
		}	
		
		i_ret =  fts_8606_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
	
    		if (i_ret != 0) 
		{
    			dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
    		} 
		else 
		{
			#ifdef AUTO_CLB
    				fts_ctpm_auto_clb(client);  /*start auto CLB*/
			#endif
    		}
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x87))
	{
	    	/*FW upgrade*/
    		pbt_buf = CTPM_FW;
	   	 /*call the upgrade function*/
    		i_ret = fts_8716_writepram(client, aucFW_PRAM_BOOT, sizeof(aucFW_PRAM_BOOT));
	
		if (i_ret != 0)
		{
			dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
			return -EIO;
		}	
		
		i_ret =  fts_8716_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
	
    		if (i_ret != 0) 
		{
    			dev_err(&client->dev, "[FTS] upgrade failed. err=%d.\n", i_ret);
    		} 
		else 
		{
			#ifdef AUTO_CLB
    				fts_ctpm_auto_clb(client);  /*start auto CLB*/
			#endif
    		}
	}
	else if ((fts_updateinfo_curr.CHIP_ID==0x0E))
	{
		if (fw_len < 8 || fw_len > 32 * 1024) 
		{
			dev_err(&client->dev, "%s:FW length error\n", __func__);
			return -EIO;
		}
		pbt_buf = CTPM_FW;
		i_ret = fts_3x07_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",__func__);
	}
	return i_ret;
}

extern void fts_get_upgrade_array(void);
static unsigned char ft5x46_ctpm_VidFWid_get_from_boot(  struct i2c_client *client )
{
	unsigned char auc_i2c_write_buf[10];
	unsigned char reg_val[4] = {0};
	unsigned char i = 0;
	unsigned char vid = 0xFF;
	int i_ret;
	
    	fts_get_upgrade_array();

	i_ret = hidi2c_to_stdi2c(client);
	
	for (i = 0; i < FTS_UPGRADE_LOOP; i++) 
	{
		msleep(100);
		DBG("[FTS] Step 1:Reset  CTPM\n");
		/*********Step 1:Reset  CTPM *****/
#if 0
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
		msleep(5);
		mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
#else
		/*write 0xaa to register 0xfc */
		fts_write_reg(client, 0xfc, FTS_UPGRADE_AA);
		msleep(5);
		
		/*write 0x55 to register 0xfc */
		fts_write_reg(client, 0xfc, FTS_UPGRADE_55);

#endif
		if(i<=15)
		{
			msleep(fts_updateinfo_curr.delay_55+i*3);
		}
		else
		{
			msleep(fts_updateinfo_curr.delay_55-(i-15)*2);
		}

		i_ret = hidi2c_to_stdi2c(client);
		msleep(5);
   
		/*********Step 2:Enter upgrade mode *****/
		DBG("[FTS] Step 2:Enter upgrade mode \n");
		#if 0
			auc_i2c_write_buf[0] = FT_UPGRADE_55;
			auc_i2c_write_buf[1] = FT_UPGRADE_AA;
			do {
				j++;
				i_ret = fts_i2c_Write(client, auc_i2c_write_buf, 2);
				msleep(5);
			} while (i_ret <= 0 && j < 5);
		#else
			auc_i2c_write_buf[0] = FTS_UPGRADE_55;
			fts_i2c_write(client, auc_i2c_write_buf, 1);
			msleep(5);
			auc_i2c_write_buf[0] = FTS_UPGRADE_AA;
			fts_i2c_write(client, auc_i2c_write_buf, 1);
		#endif

#if 1
		/*********Step 3:check READ-ID***********************/
		msleep(fts_updateinfo_curr.delay_readid);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =0x00;
		fts_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

		DBG("[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0], reg_val[1]);
		if (reg_val[0] == fts_updateinfo_curr.upgrade_id_1
			&& reg_val[1] != 0) {

			DBG("[FTS] Step 3 ok: CTPM ID,ID1 = 0x%x,ID2 = 0x%x ,0x%x, 0x%x:\n",
				reg_val[0], reg_val[1], fts_updateinfo_curr.upgrade_id_1, fts_updateinfo_curr.upgrade_id_2);
			break;
		} else {
			dev_err(&client->dev, "[FTS] Step 3 fail: CTPM ID,ID1 = 0x%x,ID2 = 0x%x, 0x%x, 0x%x: \n",
				reg_val[0], reg_val[1], fts_updateinfo_curr.upgrade_id_1, fts_updateinfo_curr.upgrade_id_2);
		}
#endif	
	}
	if (i >= FTS_UPGRADE_LOOP)
	{
		DBG("[FTS] FTS_UPGRADE_LOOP is  i = %d \n", i);
		return -EIO;
	}
	DBG("[FTS] OK: FTS_UPGRADE_LOOP is  i = %d \n", i);
	auc_i2c_write_buf[0] = 0xcd;
	fts_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	DBG("[FTS]bootloader version = 0x%x\n", reg_val[0]);
	auc_i2c_write_buf[0] = 0x03;
	auc_i2c_write_buf[1] = 0x00;
	for(i = 0;i < FTS_UPGRADE_LOOP; i++) 
	{
		auc_i2c_write_buf[2] = 0xd7;
		auc_i2c_write_buf[3] = 0x83;//84
		i_ret = fts_i2c_write(client, auc_i2c_write_buf, 4);
		if (i_ret < 0)
		{
			DBG( "[FTS] Step 4: read vendor id from flash error when i2c write, i_ret = %d\n", i_ret);
			continue;
		}
		//fts_i2c_Read(client, auc_i2c_write_buf, 4, reg_val, 2);

		i_ret = fts_i2c_read(client, auc_i2c_write_buf, 0, reg_val, 2);
		if (i_ret < 0)
		{
			DBG( "[FTS] Step 4: read vendor id from flash error when i2c write, i_ret = %d\n", i_ret);
			continue;
		}

		vid = reg_val[1];

		DBG("%s: REG VAL ID1 = 0x%x,ID2 = 0x%x\n", __func__,reg_val[0],reg_val[1]);	
		break;
		
		
	}
	DBG("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	fts_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */
	return vid;     
}


/************************************************************************
* Name: fts_ctpm_auto_upgrade
* Brief:  auto upgrade
* Input: i2c info
* Output: no
* Return: 0
***********************************************************************/
int fts_ctpm_auto_upgrade(struct i2c_client *client)
{
	u8 uc_host_fm_ver = FTS_REG_FW_VER;
	u8 uc_tp_fm_ver,uc_tp_vendor_id,uc_boot_vendor_id;
	int i_ret;

	fts_read_reg(client, FTS_REG_FW_VER, &uc_tp_fm_ver);
	fts_read_reg(client, FTS_REG_VENDOR_ID, &uc_tp_vendor_id);
	printk("[FTS] uc_tp_fm_ver = 0x%x, uc_tp_vendor_id = 0x%x\n",uc_tp_fm_ver, uc_tp_vendor_id);
	if((uc_tp_vendor_id != FTS_Vendor_1_ID) && (uc_tp_vendor_id != FTS_Vendor_2_ID) && (uc_tp_vendor_id != FTS_Vendor_3_ID) && (uc_tp_vendor_id != FTS_Vendor_4_ID) && (uc_tp_vendor_id != FTS_Vendor_5_ID))
	{
		uc_boot_vendor_id = ft5x46_ctpm_VidFWid_get_from_boot(client);
		printk("[FTS] uc_boot_vendor_id= 0x%x!\n", uc_boot_vendor_id);
		if((uc_boot_vendor_id == FTS_Vendor_1_ID)||(uc_boot_vendor_id == FTS_Vendor_3_ID)||(uc_boot_vendor_id == FTS_Vendor_4_ID)||(uc_boot_vendor_id == FTS_Vendor_5_ID))
		{
			uc_tp_fm_ver = 0;//force to upgrade the FW
			uc_tp_vendor_id = uc_boot_vendor_id;
		}
		else
		{
			printk("[FTS] FW .i unmatched,stop upgrade\n");
			return -EIO;//FW unmatched
		}
	}

	uc_host_fm_ver = fts_ctpm_get_i_file_ver(uc_tp_vendor_id);
	printk("[FTS] uc_host_fm_ver = 0x%x\n",uc_host_fm_ver);
	
	if (uc_tp_fm_ver == FTS_REG_FW_VER ||	uc_tp_fm_ver < uc_host_fm_ver ) 
	{
	    
		msleep(100);
		dev_dbg(&client->dev, "[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",uc_tp_fm_ver, uc_host_fm_ver);
		i_ret = fts_ctpm_fw_upgrade_with_i_file(client,uc_tp_vendor_id);
		if (i_ret == 0)	
		{
			msleep(300);
			uc_host_fm_ver = fts_ctpm_get_i_file_ver(uc_tp_vendor_id);
			dev_dbg(&client->dev, "[FTS] upgrade to new version 0x%x\n",uc_host_fm_ver);
			printk("[FTS] upgrade to new version 0x%x\n",uc_host_fm_ver);
		} 
		else
		{
			pr_err("[FTS] upgrade failed ret=%d.\n", i_ret);
			printk("[FTS] upgrade failed ret=%d.\n", i_ret);
			return -EIO;
		}
	}
	return 0;
}
