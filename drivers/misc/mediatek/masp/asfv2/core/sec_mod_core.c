/*
 * Copyright (C) 2015 MediaTek Inc.
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

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/

/******************************************************************************
 *  INCLUDE LINUX HEADER
 ******************************************************************************/
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include "sec_hal.h"
#include "sec_boot_lib.h"
#include "masp_version.h"
#include "sec_ioctl.h"
#include "sec_osal_light.h"
#include "sec_nvram.h"

#define MOD                         "ASF"
#define HEVC_BLK_LEN                20480

#define CI_BLK_SIZE                 16
#define CI_BLK_ALIGN(len) (((len)+CI_BLK_SIZE-1) & ~(CI_BLK_SIZE-1))

/**************************************************************************
 *  GLOBAL VARIABLES
 **************************************************************************/
typedef struct {
	unsigned char buf[HEVC_BLK_LEN];
	unsigned int len;
} HEVC_BLK;
HEVC_BLK hevc_blk;

uint lks = 2;		/* if sec is not enabled, this param will not be updated */
module_param(lks, uint, S_IRUSR /*|S_IWUSR|S_IWGRP */  | S_IRGRP | S_IROTH);	/* r--r--r-- */
MODULE_PARM_DESC(lks, "A device lks parameter under sysfs (0=NL, 1=L, 2=NA)");


/**************************************************************************
 *  SEC DRIVER EXIT
 **************************************************************************/
void sec_core_exit(void)
{
	pr_debug("[%s] version '%s%s', exit.\n", MOD, BUILD_TIME, BUILD_BRANCH);
}

/* extern void osal_msleep(unsigned int msec); */

/**************************************************************************
 *  SEC DRIVER IOCTL
 **************************************************************************/
long sec_core_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int ret = 0;
	unsigned int cipher_len = 0;
	unsigned int rid[4];
	META_CONTEXT meta_ctx;

	/* ---------------------------------- */
	/* IOCTL                              */
	/* ---------------------------------- */

	if (_IOC_TYPE(cmd) != SEC_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > SEC_IOC_MAXNR)
		return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {

		/* ---------------------------------- */
		/* get random id                      */
		/* ---------------------------------- */
	case SEC_GET_RANDOM_ID:
		pr_debug("[%s] CMD - SEC_GET_RANDOM_ID\n", MOD);
		sec_get_random_id(&rid[0]);
		ret =
		    osal_copy_to_user((void __user *)arg, (void *)&rid[0],
				      sizeof(unsigned int) * 4);
		break;

		/* ---------------------------------- */
		/* init boot info                     */
		/* ---------------------------------- */
	case SEC_BOOT_INIT:
		pr_debug("[%s] CMD - SEC_BOOT_INIT\n", MOD);
		ret = masp_boot_init();
		ret = osal_copy_to_user((void __user *)arg, (void *)&ret, sizeof(int));
		break;

		/* ---------------------------------- */
		/* check if secure usbdl is enbaled   */
		/* ---------------------------------- */
	case SEC_USBDL_IS_ENABLED:
		pr_debug("[%s] CMD - SEC_USBDL_IS_ENABLED\n", MOD);
		ret = sec_usbdl_enabled();
		ret = osal_copy_to_user((void __user *)arg, (void *)&ret, sizeof(int));
		break;

		/* ---------------------------------- */
		/* check if secure boot is enbaled    */
		/* ---------------------------------- */
	case SEC_BOOT_IS_ENABLED:
		pr_debug("[%s] CMD - SEC_BOOT_IS_ENABLED\n", MOD);
		ret = sec_boot_enabled();
		ret = osal_copy_to_user((void __user *)arg, (void *)&ret, sizeof(int));
		break;

		/* ---------------------------------- */
		/* NVRAM HW encryption                */
		/* ---------------------------------- */
	case SEC_NVRAM_HW_ENCRYPT:
		pr_debug("[%s] CMD - SEC_NVRAM_HW_ENCRYPT\n", MOD);
		if (osal_copy_from_user((void *)&meta_ctx, (void __user *)arg, sizeof(meta_ctx)))
			return -EFAULT;

		/* TODO : double check if META register is correct ? */
		masp_hal_sp_hacc_enc((unsigned char *)&(meta_ctx.data), NVRAM_CIPHER_LEN, TRUE,
				     HACC_USER2, FALSE);
		meta_ctx.ret = SEC_OK;

		ret = osal_copy_to_user((void __user *)arg, (void *)&meta_ctx, sizeof(meta_ctx));
		break;

		/* ---------------------------------- */
		/* NVRAM HW decryption                */
		/* ---------------------------------- */
	case SEC_NVRAM_HW_DECRYPT:
		pr_debug("[%s] CMD - SEC_NVRAM_HW_DECRYPT\n", MOD);
		if (osal_copy_from_user((void *)&meta_ctx, (void __user *)arg, sizeof(meta_ctx)))
			return -EFAULT;

		masp_hal_sp_hacc_dec((unsigned char *)&(meta_ctx.data), NVRAM_CIPHER_LEN, TRUE,
				     HACC_USER2, FALSE);
		meta_ctx.ret = SEC_OK;
		ret = osal_copy_to_user((void __user *)arg, (void *)&meta_ctx, sizeof(meta_ctx));
		break;

		/* ---------------------------------- */
		/* HEVC EOP                           */
		/* ---------------------------------- */
	case SEC_HEVC_EOP:
		pr_debug("[%s] CMD - SEC_HEVC_EOP\n", MOD);
		if (osal_copy_from_user((void *)(&hevc_blk), (void __user *)arg, sizeof(HEVC_BLK)))
			return -EFAULT;

		if ((hevc_blk.len % CI_BLK_SIZE) == 0) {
			cipher_len = hevc_blk.len;
		} else if ((hevc_blk.len % CI_BLK_SIZE) > 0) {
			cipher_len = CI_BLK_ALIGN(hevc_blk.len) - CI_BLK_SIZE;
			if (cipher_len == 0) {
				pr_debug("[%s] less than one ci_blk, no need to do eop", MOD);
				break;
			}
		}
		masp_hal_sp_hacc_enc((unsigned char *)(&hevc_blk.buf), cipher_len, TRUE, HACC_USER4,
				     FALSE);

		ret = osal_copy_to_user((void __user *)arg, (void *)(&hevc_blk), sizeof(HEVC_BLK));
		break;

		/* ---------------------------------- */
		/* HEVC DOP                           */
		/* ---------------------------------- */
	case SEC_HEVC_DOP:
		pr_debug("[%s] CMD - SEC_HEVC_DOP\n", MOD);
		if (osal_copy_from_user((void *)(&hevc_blk), (void __user *)arg, sizeof(HEVC_BLK)))
			return -EFAULT;

		if ((hevc_blk.len % CI_BLK_SIZE) == 0)
			cipher_len = hevc_blk.len;
		else if ((hevc_blk.len % CI_BLK_SIZE) > 0) {
			cipher_len = CI_BLK_ALIGN(hevc_blk.len) - CI_BLK_SIZE;
			if (cipher_len == 0) {
				pr_debug("[%s] less than one ci_blk, no need to do dop", MOD);
				break;
			}
		}

		masp_hal_sp_hacc_dec((unsigned char *)(&hevc_blk.buf), cipher_len, TRUE, HACC_USER4,
				     FALSE);

		ret = osal_copy_to_user((void __user *)arg, (void *)(&hevc_blk), sizeof(HEVC_BLK));
		break;

		/* ---------------------------------- */
		/* configure HACC HW (include SW KEY)  */
		/* ---------------------------------- */
	case SEC_HACC_CONFIG:
		pr_debug("[%s] CMD - SEC_HACC_CONFIG\n", MOD);
		ret = sec_boot_hacc_init();
		ret = osal_copy_to_user((void __user *)arg, (void *)&ret, sizeof(int));
		break;

	}

	return 0;
}
