/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Kernelc
 *
 * Project:
 * --------
 *    Audio smart pa Function
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include "AudDrv_NXP.h"
#include <linux/gpio.h>

#define TFA_I2C_CHANNEL     (3)

#ifdef CONFIG_MTK_NXP_TFA9890
#define ECODEC_SLAVE_ADDR_WRITE 0x68
#define ECODEC_SLAVE_ADDR_READ  0x69
#else
#define ECODEC_SLAVE_ADDR_WRITE 0x6c
#define ECODEC_SLAVE_ADDR_READ  0x6d
#endif

#define I2C_MASTER_CLOCK       400
#define NXPEXTSPK_I2C_DEVNAME "mtksmartpa"

/*****************************************************************************
*           DEFINE AND CONSTANT
******************************************************************************
*/

#define AUDDRV_NXPSPK_NAME   "mtksmartpa"
#define AUDDRV_AUTHOR "MediaTek WCX"
#define RW_BUFFER_LENGTH (256)

#define smart_set_gpio(x) (x|0x80000000)

/*****************************************************************************
*           V A R I A B L E     D E L A R A T I O N
*******************************************************************************/

/* I2C variable */
static struct i2c_client *new_client;
static char WriteBuffer[RW_BUFFER_LENGTH];
static char ReadBuffer[RW_BUFFER_LENGTH];

static void *TfaI2CDMABuf_va;
static dma_addr_t TfaI2CDMABuf_pa;

static int AudDrv_nxpspk_open(struct inode *inode, struct file *fp);
static long AudDrv_nxpspk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
static ssize_t AudDrv_nxpspk_read(struct file *fp,	char __user *data, size_t count, loff_t *offset);
static ssize_t AudDrv_nxpspk_write(struct file *fp, const char __user *data, size_t count, loff_t *offset);

static const struct file_operations AudDrv_nxpspk_fops = {
	.owner   = THIS_MODULE,
	.open    = AudDrv_nxpspk_open,
	.unlocked_ioctl   = AudDrv_nxpspk_ioctl,
	.write   = AudDrv_nxpspk_write,
	.read    = AudDrv_nxpspk_read,
};


#ifdef CONFIG_OF
static unsigned int pin_ext_dac_rst, pin_ext_hp_en, pin_nxpspk_lrck, pin_nxpspk_bck, pin_nxpspk_datai, pin_nxpspk_datao;
static unsigned int pin_ext_dac_rst_mode, pin_ext_hp_en_mode, pin_nxpspk_lrck_mode;
static unsigned int pin_nxpspk_bck_mode, pin_nxpspk_datai_mode, pin_nxpspk_datao_mode;

static int smartpa_parse_gpio(void)
{
	struct device_node *node = NULL;

	pr_debug("+%s\n", __func__);
	node = of_find_compatible_node(NULL, NULL, "mediatek,mtksmartpa");
	if (node) {
		if (of_property_read_u32_index(node, "aud_ext_dacrst_gpio", 0, &pin_ext_dac_rst))
			pr_debug("aud_ext_dacrst_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "aud_ext_dacrst_gpio", 1, &pin_ext_dac_rst_mode))
			pr_debug("aud_ext_dacrst_gpio get pin_mode fail!!!\n");

		pr_debug("pin_ext_dac_rst = %u pin_ext_dac_rst_mode = %u\n", pin_ext_dac_rst, pin_ext_dac_rst_mode);

		if (of_property_read_u32_index(node, "aud_ext_hpen_gpio", 0, &pin_ext_hp_en))
			pr_debug("aud_ext_hpen_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "aud_ext_hpen_gpio", 1, &pin_ext_hp_en_mode))
			pr_debug("aud_ext_hpen_gpio get pin_mode fail!!!\n");

		pr_debug("pin_ext_hp_en = %u pin_ext_hp_en_mode = %u\n", pin_ext_hp_en, pin_ext_hp_en_mode);

		if (of_property_read_u32_index(node, "nxpws_gpio", 0, &pin_nxpspk_lrck))
			pr_debug("nxpws_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "nxpws_gpio", 1, &pin_nxpspk_lrck_mode))
			pr_debug("nxpws_gpio get pin_mode fail!!!\n");

		pr_debug("pin_nxpspk_lrck = %u pin_nxpspk_lrck_mode = %u\n", pin_nxpspk_lrck, pin_nxpspk_lrck_mode);

		if (of_property_read_u32_index(node, "nxpclk_gpio", 0, &pin_nxpspk_bck))
			pr_debug("nxpclk_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "nxpclk_gpio", 1, &pin_nxpspk_bck_mode))
			pr_debug("nxpclk_gpio get pin_mode fail!!!\n");

		pr_debug("pin_nxpspk_bck = %u pin_nxpspk_bck_mode = %u\n", pin_nxpspk_bck, pin_nxpspk_bck_mode);

		if (of_property_read_u32_index(node, "nxpdatai_gpio", 0, &pin_nxpspk_datai))
			pr_debug("nxpdatai_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "nxpdatai_gpio", 1, &pin_nxpspk_datai_mode))
			pr_debug("nxpdatai_gpio get pin_mode fail!!!\n");

		pr_debug("pin_nxpspk_bck = %u pin_nxpspk_bck_mode = %u\n", pin_nxpspk_bck, pin_nxpspk_bck_mode);

		if (of_property_read_u32_index(node, "nxpdatao_gpio", 0, &pin_nxpspk_datao))
			pr_debug("nxpdatao_gpio get pin fail!!!\n");

		if (of_property_read_u32_index(node, "nxpdatao_gpio", 1, &pin_nxpspk_datao_mode))
			pr_debug("nxpdatao_gpio get pin_mode fail!!!\n");

		pr_debug("pin_nxpspk_datao = %u pin_nxpspk_datao_mode = %u\n", pin_nxpspk_datao, pin_nxpspk_datao_mode);
	} else {
		pr_debug("Auddrv_OF_ParseGPIO node NULL!!!\n");
		return -1;
	}
	pr_debug("-%s\n", __func__);
	return 0;
}

#endif


/* new I2C register method */
const struct i2c_device_id nxpExt_i2c_id[] = { {NXPEXTSPK_I2C_DEVNAME, 0}, {} };
struct i2c_board_info __initdata nxpExt_dev = { I2C_BOARD_INFO(NXPEXTSPK_I2C_DEVNAME, 0x36) };

/* function declration */
static int NXPExtSpk_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int NXPExtSpk_i2c_remove(struct i2c_client *client);
static int AudDrv_NXPSpk_Init(void);
static bool NXPExtSpk_Register(void);
static int NXPExtSpk_registerI2C(void);

static int NXPExtSpk_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, NXPEXTSPK_I2C_DEVNAME);
	return 0;
}

static int NXPExtSpk_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	return 0;
}

static int NXPExtSpk_i2c_resume(struct i2c_client *client)
{
	return 0;
}

/* i2c driver */
static struct i2c_driver NXPExtSpk_i2c_driver = {
	.probe = NXPExtSpk_i2c_probe,
	.remove = NXPExtSpk_i2c_remove,
	.detect     =  NXPExtSpk_i2c_detect,
	.suspend    =  NXPExtSpk_i2c_suspend,
	.resume     =  NXPExtSpk_i2c_resume,
	.id_table = nxpExt_i2c_id,
	.driver = {
		.name = NXPEXTSPK_I2C_DEVNAME,
	},
};


static int NXPExtSpk_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	new_client = client;
	new_client->timing = 400;

	pr_debug("NXPExtSpk_i2c_probe\n");

#ifdef CONFIG_MTK_NXP_TFA9890
	mt_set_gpio_mode(GPIO_AUD_EXTDAC_RST_PIN/*GPIO130*/, GPIO_MODE_00);
	mt_set_gpio_out(GPIO_AUD_EXTDAC_RST_PIN/*GPIO130*/, GPIO_OUT_ZERO);
	usleep_range(2);
	mt_set_gpio_out(GPIO_AUD_EXTDAC_RST_PIN/*GPIO130*/, GPIO_OUT_ONE);
	usleep_range(2);
	mt_set_gpio_out(GPIO_AUD_EXTDAC_RST_PIN/*GPIO130*/, GPIO_OUT_ZERO);
	usleep_range(10);
#endif

	TfaI2CDMABuf_va = dma_alloc_coherent(NULL, 4096, &TfaI2CDMABuf_pa, GFP_KERNEL);
	if (TfaI2CDMABuf_va == NULL) {
		NXP_ERROR("dma_alloc_coherent error\n");
		NXP_INFO("i2c_probe failed\n");
		return -1;
	}
	NXP_INFO("i2c_probe success\n");
	return 0;
}

static int NXPExtSpk_i2c_remove(struct i2c_client *client)
{
	new_client = NULL;
	i2c_unregister_device(client);
	i2c_del_driver(&NXPExtSpk_i2c_driver);
	if (TfaI2CDMABuf_va != NULL) {
		dma_free_coherent(NULL, 4096, TfaI2CDMABuf_va, TfaI2CDMABuf_pa);
		TfaI2CDMABuf_va = NULL;
		TfaI2CDMABuf_pa = 0;
	}
	usleep_range(1);
#ifdef CONFIG_MTK_NXP_TFA9890
	mt_set_gpio_mode(GPIO_AUD_EXTHP_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_AUD_EXTHP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_AUD_EXTHP_EN_PIN, GPIO_OUT_ZERO);
#endif
	return 0;
}

/* read write implementation */
/* read one register */
/*
ssize_t NXPSpk_read_byte(u8 addr, u8 *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	cmd_buf[0] = addr;
	if (!new_client) {
		pr_debug("NXPSpk_read_byte I2C client not initialized!!");
		return -1;
	}
	ret = i2c_master_send(new_client, &cmd_buf[0], 1);
	if (ret < 0) {
		pr_debug("NXPSpk_read_byte read sends command error!!\n");
		return -1;
	}
	ret = i2c_master_recv(new_client, &readData, 1);
	if (ret < 0) {
		pr_debug("NXPSpk_read_byte reads recv data error!!\n");
		return -1;
	}
	*returnData = readData;
	return 0;
}
*/

/* write register */
ssize_t NXPExt_write_byte(u8 addr, u8 writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	if (new_client == NULL) {
		pr_debug("I2C client not initialized!!");
		return -1;
	}
	write_data[0] = addr;   /* ex. 0x01 */
	write_data[1] = writeData;
	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		pr_debug("write sends command error!!");
		return -1;
	}
	/* pr_debug("addr 0x%x data 0x%x\n", addr, writeData); */
	return 0;
}

static int NXPExtSpk_registerI2C(void)
{
	pr_debug("NXPExtSpk_registerI2C\n");

#ifdef CONFIG_MTK_NXP_TFA9890
	mt_set_gpio_mode(GPIO_AUD_EXTHP_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_AUD_EXTHP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_AUD_EXTHP_EN_PIN, GPIO_OUT_ONE);
	usleep_range(1);
#endif

	i2c_register_board_info(TFA_I2C_CHANNEL, &nxpExt_dev, 1);
	if (i2c_add_driver(&NXPExtSpk_i2c_driver)) {
		pr_debug("fail to add device into i2c");
		return -1;
	}
	return 0;
}


bool NXPExtSpk_Register(void)
{
	pr_debug("NXPExtSpk_Register\n");
	NXPExtSpk_registerI2C();
	return true;
}

int  AudDrv_NXPSpk_Init(void)
{
	pr_debug("Set GPIO for AFE I2S output to external DAC\n");
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_lrck) , pin_nxpspk_lrck_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_bck), pin_nxpspk_bck_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_datai), pin_nxpspk_datai_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_datao), pin_nxpspk_datao_mode);
	return  0;
}

/*****************************************************************************
 * FILE OPERATION FUNCTION
 *  AudDrv_nxpspk_ioctl
 *
 * DESCRIPTION
 *  IOCTL Msg handle
 *
 *****************************************************************************
 */
static long AudDrv_nxpspk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	default: {
		ret = -1;
		break;
	}
	}
	return ret;
}

static int AudDrv_nxpspk_probe(struct platform_device *dev)
{
	int ret = 0;

	pr_debug("AudDrv_nxpspk_probe\n");
	if (ret < 0)
		pr_debug("AudDrv_nxpspk_probe request_irq MT6582_AP_BT_CVSD_IRQ_LINE Fail\n");

	smartpa_parse_gpio();
	NXPExtSpk_Register();
	AudDrv_NXPSpk_Init();

	memset((void *)WriteBuffer, 0, RW_BUFFER_LENGTH);
	memset((void *)ReadBuffer, 0, RW_BUFFER_LENGTH);

	pr_debug("-AudDrv_nxpspk_probe\n");
	return 0;
}

static int AudDrv_nxpspk_open(struct inode *inode, struct file *fp)
{
	return 0;
}

static int nxp_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.timing = I2C_MASTER_CLOCK;

	if (count <= 8)
		msg.addr = client->addr & I2C_MASK_FLAG;
	else
		msg.addr = client->addr & (I2C_MASK_FLAG|I2C_DMA_FLAG);

	msg.flags = client->flags & I2C_M_TEN;

	msg.len = count;
	msg.buf = (char *)buf;
	msg.ext_flag = client->ext_flag;
	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}

static int nxp_i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.timing = I2C_MASTER_CLOCK;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.ext_flag = client->ext_flag;
	msg.buf = (char *)buf;

	if (count <= 8)
		msg.addr = client->addr & I2C_MASK_FLAG;
	else
		msg.addr = client->addr & (I2C_MASK_FLAG|I2C_DMA_FLAG);

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg received), return #bytes received,
	 * else error code.
	 */
	return (ret == 1) ? count : ret;
}

static ssize_t AudDrv_nxpspk_write(struct file *fp, const char __user *data, size_t count, loff_t *offset)

{
	int i = 0;
	int ret;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	if (copy_from_user(tmp, data, count)) {
		kfree(tmp);
		return -EFAULT;
	}


	for (i = 0;  i < count; i++)
		TfaI2CDMABuf[i] = tmp[i];

	if (count <= 8)
		ret = nxp_i2c_master_send(new_client, tmp, count);
	else
		ret = nxp_i2c_master_send(new_client, (char *)TfaI2CDMABuf_pa, count);

	kfree(tmp);
	return ret;
}

static ssize_t AudDrv_nxpspk_read(struct file *fp,  char __user *data, size_t count, loff_t *offset)
{
	int i = 0;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;
	int ret = 0;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;

	if (count <= 8)
		ret = nxp_i2c_master_recv(new_client, tmp, count);
	else {
		ret = nxp_i2c_master_recv(new_client, (char *)TfaI2CDMABuf_pa, count);
		for (i = 0; i < count; i++)
			tmp[i] = TfaI2CDMABuf[i];
	}

	if (ret >= 0)
		ret = copy_to_user(data, tmp, count) ? (-EFAULT) : ret;
	kfree(tmp);

	return ret;
}

/**************************************************************************
 * STRUCT
 *  File Operations and misc device
 *
 **************************************************************************/

#ifdef CONFIG_MTK_NXP_TFA9890
static struct miscdevice AudDrv_nxpspk_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "smartpa_i2c",
	.fops = &AudDrv_nxpspk_fops,
};
#else
static struct miscdevice AudDrv_nxpspk_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "nxpspk",
	.fops = &AudDrv_nxpspk_fops,
};
#endif

/***************************************************************************
 * FUNCTION
 *  AudDrv_nxpspk_mod_init / AudDrv_nxpspk_mod_exit
 *
 * DESCRIPTION
 *  Module init and de-init (only be called when system boot up)
 *
 **************************************************************************/

#ifdef CONFIG_OF
static const struct of_device_id mtk_smart_pa_of_ids[] = {
	{ .compatible = "mediatek,mtksmartpa", },
	{}
};
#endif

static struct platform_driver AudDrv_nxpspk = {
	.probe = AudDrv_nxpspk_probe,
	.driver = {
		.name = NXPEXTSPK_I2C_DEVNAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_smart_pa_of_ids,
#endif
	},
};

#ifndef CONFIG_OF
static struct platform_device *AudDrv_NXPSpk_dev;
#endif

int AudDrv_nxpspk_mod_init(void)
{
	int ret = 0;

	pr_debug("+AudDrv_nxpspk_mod_init\n");

#ifndef CONFIG_OF
	pr_debug("platform_device_alloc\n");
	AudDrv_NXPSpk_dev = platform_device_alloc("AudioMTKNXPSPK", -1);
	if (!AudDrv_NXPSpk_dev)
		return -ENOMEM;

	pr_debug("platform_device_add\n");

	ret = platform_device_add(AudDrv_NXPSpk_dev);
	if (ret != 0) {
		platform_device_put(AudDrv_NXPSpk_dev);
		return ret;
	}
#endif
	/* Register platform DRIVER */

	ret = platform_driver_register(&AudDrv_nxpspk);
	if (ret) {
		pr_debug("AudDrv Fail:%d - Register DRIVER\n", ret);
		return ret;
	}
	/* register MISC device */
	ret = misc_register(&AudDrv_nxpspk_device);
	if (ret) {
		pr_debug("AudDrv_nxpspk_mod_init misc_register Fail:%d\n", ret);
		return ret;
	}

	pr_debug("-AudDrv_nxpspk_mod_init\n");
	return 0;
}

void AudDrv_nxpspk_mod_exit(void)
{
	pr_debug("+AudDrv_nxpspk_mod_exit\n");

	pr_debug("-AudDrv_nxpspk_mod_exit\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(AUDDRV_NXPSPK_NAME);
MODULE_AUTHOR(AUDDRV_AUTHOR);

module_init(AudDrv_nxpspk_mod_init);
module_exit(AudDrv_nxpspk_mod_exit);

