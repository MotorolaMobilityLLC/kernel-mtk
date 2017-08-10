/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Header Files
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>
#include <linux/wakelock.h>
#include <linux/compat.h>
#include <linux/uaccess.h>
#ifdef SIGTEST
#include <asm/siginfo.h>
#endif

#include <mach/mt_clkmgr.h>
#include "scp_helper.h"
#include "scp_ipi.h"
#include "scp_excep.h"
#include "vow.h"


/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        D E F I N I T I O N S
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

/*****************************************************************************
 * Configurations
****************************************************************************/
#define DEBUG_VOWDRV 1
#define DYNAMIC_VOW_SWAP 0
#define VOW_PRE_LEARN_MODE 1
#define AP_ONLY_UT 0

#define VOW_TAG "VOW:"

#if DEBUG_VOWDRV
#define PRINTK_VOWDRV(format, args...) printk(format, ##args)
#else
#define PRINTK_VOWDRV(format, args...)
#endif


/*****************************************************************************
 * Variable Definition
****************************************************************************/
static const char vowdrv_name[] = "VOW_driver_device";
unsigned int VowDrv_Wait_Queue_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(VowDrv_Wait_Queue);
unsigned int VoiceData_Wait_Queue_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(VoiceData_Wait_Queue);
static DEFINE_SPINLOCK(vowdrv_lock);
static int VowDrv_GetHWStatus(void);
struct wake_lock VOW_suspend_lock;
int init_flag = 0;

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        V O W    S E R V I C E S
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
static struct
{
#if DYNAMIC_VOW_SWAP
	int swapid;
	bool swapwait;
#endif
	void                 *vow_init_model;
	void                 *vow_noise_model;
	void                 *vow_fir_model;
	VOW_SPEAKER_MODEL_T  vow_speaker_model[MAX_VOW_SPEAKER_MODEL];
	unsigned long        vow_info_apuser[MAX_VOW_INFO_LEN];
	unsigned int         vow_info_dsp[MAX_VOW_INFO_LEN];
	unsigned long        voicedata_user_addr;
	unsigned long        voicedata_user_size;
	short                *voicedata_kernel_ptr;
	void                 *voicddata_scp_ptr;
	dma_addr_t           voicedata_scp_addr;
	unsigned int         voicedata_tcm_addr;
	short                voicedata_idx;
	bool                 ipimsgwait;
	bool                 ipi_fail_result;
	bool                 md32_command_flag;
	bool                 recording_flag;
	int                  md32_command_id;
	VOW_EINT_STATUS      eint_status;
	VOW_PWR_STATUS       pwr_status;
	int                  send_ipi_count;
	bool                 suspend_lock;
	bool                 firstRead;
	unsigned long        voicedata_user_return_size_addr;
	unsigned int         voice_length;
	unsigned int         transfer_length;
} vowserv;

static struct device dev = {
	.init_name = "vowdmadev",
	.coherent_dma_mask = ~0,             /* dma_alloc_coherent(): allow any address */
	.dma_mask = &dev.coherent_dma_mask,  /* other APIs: use the same mask as coherent */
};

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        D S P   I P I   H A N D E L E R
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

#if DYNAMIC_VOW_SWAP
static int md32_vow_notify(struct notifier_block *self, unsigned long action, void *dev)
{
	MD32_REQUEST_SWAP *request_swap = (MD32_REQUEST_SWAP *)dev;
	int ret;

	switch (action) {
#if DYNAMIC_TCM_SWAP
	case MD32_SELF_TRIGGER_TCM_SWAP:
		ret = md32_tcm_swap(vowserv.swapid);
		if (ret < 0) {
			pr_debug("md32_vow_notify swap tcm failed\n");
		} else {
			vowserv.swapid = -1;
			vowserv.swapwait = false;
		}
		break;
	case APP_TRIGGER_TCM_SWAP_START:
		break;
	case APP_TRIGGER_TCM_SWAP_DONE:
		break;
	case APP_TRIGGER_TCM_SWAP_FAIL:
		break;
#endif
	default:
		pr_debug("[VOW_Kernel] md32 vow get wrong action %d\n", action);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block md32_vow_nb = {
	.notifier_call =        md32_vow_notify,
};
#endif

int vow_ipi_sendmsg(vow_ipi_msgid_t id, void *buf, unsigned int size, unsigned int type, unsigned int wait)
{
#if !AP_ONLY_UT
	ipi_status status;
	short ipibuf[24];

	ipibuf[0] = id;
	ipibuf[1] = size;
	vowserv.ipi_fail_result = false;
	/* pr_debug("[VOW_Kernel] vow_ipi_sendmsg:%x\n", id); */
	if (type == 0) {
		if (size != 0)
			memcpy((void *)&ipibuf[2], buf, size);
		status = scp_ipi_send(IPI_VOW, (void *)ipibuf, size + 4, wait);
	} else {
		if (size != 0)
			memcpy((void *)&ipibuf[2], buf, 4);
		status = scp_ipi_send(IPI_VOW, (void *)ipibuf, 8, wait);
	}
	if (status == BUSY) {
		/* pr_debug("[VOW_Kernel] vow_ipi_sendmsgFail:%x %x\n", id, status); */
		msleep(VOW_WAITCHECK_INTERVAL_MS);
		vowserv.send_ipi_count++;
		if (vowserv.send_ipi_count > VOW_IPI_TIMEOUT) {
			pr_debug("[VOW_Kernel] vow_ipi_sendmsg Timeout:0x%x_%d\n", id, vowserv.send_ipi_count);
			VOW_ASSERT(true);
		}
		return false;
	}
	if (status == ERROR) {
		pr_err("[VOW_Kernel] ipi error need check!\n");
		VOW_ASSERT(true);
		return -1;
	}
#endif
	vowserv.send_ipi_count = 0; /* reset send_ipi_count*/
	return true;
}

static void vow_ipi_reg_ok(short id)
{
	vowserv.md32_command_flag = true;
	vowserv.md32_command_id = id;
	VowDrv_Wait_Queue_flag = 1;
	wake_up_interruptible(&VowDrv_Wait_Queue);
}

static void vow_service_getVoiceData(void)
{
	if (VoiceData_Wait_Queue_flag == 0) {
		VoiceData_Wait_Queue_flag = 1;
		wake_up_interruptible(&VoiceData_Wait_Queue);
	} else {
		/*pr_debug("getVoiceData but no one wait for it, may lost it!!\n");*/
	}
}

void vow_ipi_handler(int id, void *data, unsigned int len)
{
	short *vowmsg, size;
	VOW_IPI_RESULT result;

	vowmsg = (short *)data;
	size   = vowmsg[1];
	result = vowmsg[2];
	/*pr_debug("vow_ipi_handler_%x %x\n",id, *vowmsg);*/
	switch (*vowmsg) {
	case SCP_IPIMSG_VOW_ENABLE_ACK:
		pr_debug("SCP_IPIMSG_VOW_Enable_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_VOW_DISABLE_ACK:
		pr_debug("SCP_IPIMSG_VOW_DISABLE_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_VOW_SETMODE_ACK:
		pr_debug("SCP_IPIMSG_VOW_SETMODE_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK: {
		int *ptr32;

		ptr32 = (int *)&vowmsg[3];
		vowserv.voicedata_tcm_addr = *ptr32;
		pr_debug("SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK:result_%x %x\n", result, *ptr32);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	}
	case SCP_IPIMSG_SET_VOW_MODEL_ACK:
		pr_debug("SCP_IPIMSG_SET_VOW_MODEL_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_VOW_SETGAIN_ACK:
		pr_debug("SCP_IPIMSG_VOW_SETGAIN_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_SET_FLAG_ACK:
		pr_debug("SCP_IPIMSG_SET_FLAG_ACK:result_%x\n", result);
		if (result != VOW_IPI_SUCCESS)
			vowserv.ipi_fail_result = true;
		vowserv.ipimsgwait = false;
		break;
	case SCP_IPIMSG_VOW_DATAREADY: {
		int *ptr32;

		ptr32 = (int *)&vowmsg[3];
		/*pr_debug("SCP_IPIMSG_VOW_DATAREADY 0x%x\n", *ptr32);*/
		vowserv.voice_length = (*ptr32);
		if (vowserv.recording_flag)
			vow_service_getVoiceData();
		break;
	}
	case SCP_IPIMSG_VOW_RECOGNIZE_OK:
		pr_debug("SCP_IPIMSG_VOW_RECOGNIZE_OK\n");
		vow_ipi_reg_ok(vowmsg[2]);
		break;
	default:
		pr_debug("UNKNOWN MSG NOTIFY FROM MD32:%x\n", *vowmsg);
		break;
	}
}

static bool vow_ipimsg_wait(vow_ipi_msgid_t id)
{
#if !AP_ONLY_UT
	int timeout = 0;

	while (vowserv.ipimsgwait) {
		msleep(VOW_WAITCHECK_INTERVAL_MS);
		if (timeout++ >= VOW_IPIMSG_TIMEOUT) {
			pr_debug("Error: IPI MSG timeout:id_%x\n", id);
			vowserv.ipimsgwait = false;
			return false;
		}
	}
	pr_debug("IPI MSG -: time:%x, id:%x\n", timeout, id);
#endif
	if (vowserv.ipi_fail_result)
		return false;
	return true;
}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                     D S P   S E R V I C E   F U N C T I O N S
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
static void vow_service_Init(void)
{
	int I;

	pr_debug("vow_service_Init:%x\n", init_flag);
	if (init_flag == 0) {
#if DYNAMIC_VOW_SWAP
		vowserv.swapid          = -1;
		vowserv.swapwait        = false;
		/*Hook to the MD32 chain to get notification*/
		md32_register_notify(&md32_vow_nb);
#endif
		/*register IPI handler*/
		scp_ipi_registration(IPI_VOW, vow_ipi_handler, "VOW");
		/*Initialization*/
		vowserv.send_ipi_count    = 0; /* count the busy times */
		vowserv.ipimsgwait        = false;
		vowserv.ipi_fail_result   = false;
		vowserv.md32_command_flag = false;
		vowserv.recording_flag    = false;
		vowserv.suspend_lock      = 0;
		vowserv.voice_length      = 0;
		vowserv.firstRead         = false;
		spin_lock(&vowdrv_lock);
		vowserv.pwr_status        = VOW_PWR_OFF;
		vowserv.eint_status       = VOW_EINT_DISABLE;
		spin_unlock(&vowdrv_lock);
		vowserv.vow_init_model    = NULL;
		vowserv.vow_noise_model   = NULL;
		vowserv.vow_fir_model     = NULL;
		for (I = 0; I < MAX_VOW_SPEAKER_MODEL; I++) {
			vowserv.vow_speaker_model[I].model_ptr = NULL;
			vowserv.vow_speaker_model[I].id        = -1;
			vowserv.vow_speaker_model[I].enabled   = 0;
		}

#if defined OLD_METHOD
		vowserv.voicddata_scp_ptr = kmalloc(VOW_VOICE_DATA_LENGTH_BYTES, 0);
		vowserv.voicedata_scp_addr = dma_map_single(&dev,
							    (void *)vowserv.voicddata_scp_ptr,
							    VOW_VOICE_DATA_LENGTH_BYTES,
							    DMA_BIDIRECTIONAL);
#endif
		vowserv.voicddata_scp_ptr = (void *)get_reserve_mem_virt(VOW_MEM_ID);
		vowserv.voicedata_scp_addr = get_reserve_mem_phys(VOW_MEM_ID);

		/* dma_sync_single_for_device(&dev, vowserv.voicedata_scp_addr,
					      VOW_VOICE_DATA_LENGTH_BYTES, DMA_TO_DEVICE); */
		vowserv.vow_info_dsp[0] = vowserv.voicedata_scp_addr;

		/* pr_debug("Set Debug1 Buffer Address:%x\n", vowserv.voicedata_scp_addr); */
#if defined OLD_METHOD
		dma_sync_single_for_device(&dev,
					   vowserv.voicedata_scp_addr,
					   VOW_VOICE_DATA_LENGTH_BYTES,
					   DMA_TO_DEVICE);
#endif
		vowserv.ipimsgwait = true;

		while (vow_ipi_sendmsg(AP_IPIMSG_VOW_APREGDATA_ADDR, (void *)&vowserv.vow_info_dsp[0], 4, 0, 1)
		       == false)
			;
		vow_ipimsg_wait(AP_IPIMSG_VOW_APREGDATA_ADDR);
		vowserv.voicedata_kernel_ptr = NULL;
		vowserv.voicedata_idx = 0;
		msleep(VOW_WAITCHECK_INTERVAL_MS);
#if VOW_PRE_LEARN_MODE
		VowDrv_SetFlag(VOW_FLAG_PRE_LEARN, true);
#endif
		wake_lock_init(&VOW_suspend_lock, WAKE_LOCK_SUSPEND, "VOW wakelock");
		init_flag = 1;
	} else {
		vowserv.vow_info_dsp[0] = vowserv.voicedata_scp_addr;
		vowserv.ipimsgwait = true;
		while (vow_ipi_sendmsg(AP_IPIMSG_VOW_APREGDATA_ADDR, (void *)&vowserv.vow_info_dsp[0], 4, 0, 1)
		       == false)
			;
		vow_ipimsg_wait(AP_IPIMSG_VOW_APREGDATA_ADDR);
#if VOW_PRE_LEARN_MODE
	VowDrv_SetFlag(VOW_FLAG_PRE_LEARN, true);
#endif
	}
}

int vow_service_GetParameter(unsigned long arg)
{
	if (copy_from_user((void *)(&vowserv.vow_info_apuser[0]),
			   (const void __user *)(arg),
			   sizeof(vowserv.vow_info_apuser))) {
		pr_debug("Get parameter fail\n");
		return -EFAULT;
	}
	pr_debug("Get parameter: %lu %lu %lu %lu %lu\n", vowserv.vow_info_apuser[0],
							 vowserv.vow_info_apuser[1],
							 vowserv.vow_info_apuser[2],
							 vowserv.vow_info_apuser[3],
							 vowserv.vow_info_apuser[4]);

	return 0;
}

static int vow_service_CopyModel(int model, int slot)
{
	switch (model) {
	case VOW_MODEL_INIT:
		if (copy_from_user((void *)(vowserv.vow_init_model),
				   (const void __user *)(vowserv.vow_info_apuser[1]),
				   vowserv.vow_info_apuser[2])) {
			pr_debug("Copy Init Model Fail\n");
			return -EFAULT;
		}
		break;
	case VOW_MODEL_SPEAKER:
		if (vowserv.vow_info_apuser[2] > get_reserve_mem_size(VOW_MEM_ID)) {
			pr_debug("DMA Size Too Large\n");
			return -EFAULT;
		}
		if (copy_from_user((void *)(vowserv.vow_speaker_model[slot].model_ptr),
				   (const void __user *)(vowserv.vow_info_apuser[1]),
				   vowserv.vow_info_apuser[2])) {
			pr_debug("Copy Speaker Model Fail\n");
			return -EFAULT;
		}
		vowserv.vow_speaker_model[slot].enabled = 1;
		vowserv.vow_speaker_model[slot].id      = vowserv.vow_info_apuser[0];
		break;
	case VOW_MODEL_NOISE:
		if (copy_from_user((void *)(vowserv.vow_noise_model),
				   (const void __user *)(vowserv.vow_info_apuser[1]),
				   vowserv.vow_info_apuser[2])) {
			pr_debug("Copy Noise Model Fail\n");
			return -EFAULT;
		}
		break;
	case VOW_MODEL_FIR:
		if (copy_from_user((void *)(vowserv.vow_fir_model),
				   (const void __user *)(vowserv.vow_info_apuser[1]),
				   vowserv.vow_info_apuser[2])) {
			pr_debug("Copy FIR Model Fail\n");
			return -EFAULT;
		}
		break;
	default:
		break;
	}
	return 0;
}

static int vow_service_FindFreeSpeakerModel(void)
{
	int I;

	I = 0;
	do {
		if (vowserv.vow_speaker_model[I].enabled == 0)
			break;
		I++;
	} while (I < MAX_VOW_SPEAKER_MODEL);

	PRINTK_VOWDRV("FindFreeSpeakerModel:%d\n", I);

	if (I == MAX_VOW_SPEAKER_MODEL) {
		pr_debug("Find Free Speaker Model Fail\n");
		return -1;
	}
	return I;
}

static int vow_service_SearchSpeakerModel(int id)
{
	int I;

	I = 0;
	do {
		if (vowserv.vow_speaker_model[I].id == id)
			break;
		I++;
	} while (I < MAX_VOW_SPEAKER_MODEL);

	if (I == MAX_VOW_SPEAKER_MODEL) {
		pr_debug("Search Speaker Model By ID Fail:%x\n", id);
		return -1;
	}
	return I;
}

static bool vow_service_ReleaseSpeakerModel(int id)
{
	int I;

	I = vow_service_SearchSpeakerModel(id);

	if (I == -1) {
		pr_debug("Speaker Model Fail:%x\n", id);
		return false;
	}

	vowserv.vow_info_dsp[0] = VOW_MODEL_SPEAKER;
	vowserv.vow_info_dsp[1] = id;
	vowserv.vow_info_dsp[2] = CLR_SPEAKER_MODEL_FLAG;
	vowserv.vow_info_dsp[3] = CLR_SPEAKER_MODEL_FLAG;

	PRINTK_VOWDRV("ReleaseSpeakerModel:id_%x\n", vowserv.vow_info_dsp[0]);

	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_SET_VOW_MODEL, (void *)&vowserv.vow_info_dsp[0], 16, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_SET_VOW_MODEL);

	kfree(vowserv.vow_speaker_model[I].model_ptr);
	vowserv.vow_speaker_model[I].model_ptr = NULL;
	vowserv.vow_speaker_model[I].id        = -1;
	vowserv.vow_speaker_model[I].enabled   = 0;

	return true;
}

static bool vow_service_SetVowMode(unsigned long arg)
{
	bool ret;

	vow_service_GetParameter(arg);
	vowserv.ipimsgwait = true;
	vowserv.vow_info_dsp[0] = vowserv.vow_info_apuser[0];

	PRINTK_VOWDRV("SetVowMode:mode_%x\n", vowserv.vow_info_dsp[0]);

	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_SETMODE, (void *)&vowserv.vow_info_dsp[0], 2, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_VOW_SETMODE);
	return ret;
}

static bool vow_service_SetSpeakerModel(unsigned long arg)
{
	int I;
	bool ret;
	char *ptr8;

	I = vow_service_FindFreeSpeakerModel();
	if (I == -1) {
		pr_debug("SetSpeakerModel Fail\n");
		return false;
	}
	vow_service_GetParameter(arg);
#if defined OLD_METHOD
	vowserv.vow_speaker_model[I].model_ptr = kmalloc(vowserv.vow_info_apuser[2], 0);
#endif
	vowserv.vow_speaker_model[I].model_ptr = (void *)get_reserve_mem_virt(VOW_MEM_ID);
	/*
	if (!vowserv.vow_init_model) {
		PRINTK_VOWDRV("SetSpeakerModel: allocate memory fail, size:%lu\n", vowserv.vow_info_apuser[2]);
		return false;
	}
	*/
	if (vow_service_CopyModel(VOW_MODEL_SPEAKER, I) != 0)
		return false;
	vowserv.vow_speaker_model[I].id        = I;
	vowserv.vow_speaker_model[I].enabled   = vowserv.vow_info_apuser[0];

	ptr8 = (char *)vowserv.vow_speaker_model[I].model_ptr;
	PRINTK_VOWDRV("SetSPKModel:get_reserve_mem_virt(VOW_MEM_ID): %x\n",
		      (unsigned int)get_reserve_mem_virt(VOW_MEM_ID));
	PRINTK_VOWDRV("SetSPKModel:CheckValue:%x %x %x %x %x %x\n",
		      *(char *)&ptr8[0], *(char *)&ptr8[1], *(char *)&ptr8[2],
		      *(char *)&ptr8[3], *(short *)&ptr8[160], *(int *)&ptr8[7960]);


	vowserv.vow_info_dsp[0] = VOW_MODEL_SPEAKER;
	vowserv.vow_info_dsp[1] = vowserv.vow_info_apuser[0];
#if defined OLD_METHOD
	vowserv.vow_info_dsp[2] = dma_map_single(&dev,
						 vowserv.vow_speaker_model[I].model_ptr,
						 vowserv.vow_info_apuser[2],
						 DMA_TO_DEVICE);
#endif
	vowserv.vow_info_dsp[2] = get_reserve_mem_phys(VOW_MEM_ID);

	vowserv.vow_info_dsp[3] = vowserv.vow_info_apuser[2];

#if defined OLD_METHOD
	dma_sync_single_for_device(&dev,
				   vowserv.vow_info_dsp[2],
				   vowserv.vow_info_apuser[2],
				   DMA_TO_DEVICE);
#endif
	PRINTK_VOWDRV("SetSpeakerModel:model_%x,addr_%x, id_%x, size_%x\n",
		      vowserv.vow_info_dsp[0],
		      vowserv.vow_info_dsp[2],
		      vowserv.vow_info_dsp[1],
		      vowserv.vow_info_dsp[3]);

	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_SET_VOW_MODEL, (void *)&vowserv.vow_info_dsp[0], 16, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_SET_VOW_MODEL);
	return ret;
}

static bool vow_service_SetInitModel(unsigned long arg)
{
	bool ret;
	char *ptr8;

	if (vowserv.vow_init_model != NULL) {
		kfree(vowserv.vow_init_model);
		vowserv.vow_init_model = NULL;
	}

	vow_service_GetParameter(arg);

	vowserv.vow_init_model = kmalloc(vowserv.vow_info_apuser[2], 0);

	PRINTK_VOWDRV("SetInitModel: allocate memory, addr:%lu size:%lu\n",
		      vowserv.vow_info_apuser[2], (unsigned long)vowserv.vow_init_model);

	if (!vowserv.vow_init_model) {
		PRINTK_VOWDRV("SetInitModel: allocate memory fail, size:%lu\n",
			      vowserv.vow_info_apuser[2]);
		return false;
	}

	if (vow_service_CopyModel(VOW_MODEL_INIT, 0) != 0)
		return false;

	ptr8 = (char *)vowserv.vow_init_model;

	PRINTK_VOWDRV("SetInitModel:CheckValue:%x %x %x %x %x\n",
		      *(int *)&ptr8[0], *(int *)&ptr8[1600], *(int *)&ptr8[2240],
		      *(short *)&ptr8[2880], *(short *)&ptr8[15360]);

	vowserv.vow_info_dsp[0] = VOW_MODEL_INIT;
	vowserv.vow_info_dsp[1] = 0;
	vowserv.vow_info_dsp[2] = dma_map_single(&dev,
						 vowserv.vow_init_model,
						 vowserv.vow_info_apuser[2],
						 DMA_TO_DEVICE);
	vowserv.vow_info_dsp[3] = vowserv.vow_info_apuser[2];
	dma_sync_single_for_device(&dev,
				   vowserv.vow_info_dsp[2],
				   vowserv.vow_info_apuser[2],
				   DMA_TO_DEVICE);

	PRINTK_VOWDRV("SetInitModel:model_%x,addr_%x, id_%x, size_%x\n",
		      vowserv.vow_info_dsp[0], vowserv.vow_info_dsp[2],
		      vowserv.vow_info_dsp[1], vowserv.vow_info_dsp[3]);

	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_SET_VOW_MODEL, (void *)&vowserv.vow_info_dsp[0], 16, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_SET_VOW_MODEL);
	return ret;
}

static bool vow_service_SetNoiseModel(unsigned long arg)
{
	bool ret;

	if (vowserv.vow_noise_model != NULL) {
		kfree(vowserv.vow_noise_model);
		vowserv.vow_noise_model = NULL;
	}

	vow_service_GetParameter(arg);
	vowserv.vow_noise_model = kmalloc(vowserv.vow_info_apuser[2], 0);
	if (vow_service_CopyModel(VOW_MODEL_NOISE, 0) != 0)
		return false;
	vowserv.vow_info_dsp[0] = VOW_MODEL_NOISE;
	vowserv.vow_info_dsp[1] = 0;
	vowserv.vow_info_dsp[2] = dma_map_single(&dev,
						 vowserv.vow_noise_model,
						 vowserv.vow_info_apuser[2],
						 DMA_TO_DEVICE);
	vowserv.vow_info_dsp[3] = vowserv.vow_info_apuser[2];
	dma_sync_single_for_device(&dev,
				   vowserv.vow_info_dsp[2],
				   vowserv.vow_info_apuser[2],
				   DMA_TO_DEVICE);


	PRINTK_VOWDRV("SetNoiseModel:addr_%x, size_%x\n", vowserv.vow_info_dsp[0],
							  vowserv.vow_info_dsp[1]);

	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_SET_VOW_MODEL, (void *)&vowserv.vow_info_dsp[0], 16, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_SET_VOW_MODEL);
	return ret;
}

static bool vow_service_SetFirModel(unsigned long arg)
{
	bool ret;

	if (vowserv.vow_fir_model != NULL) {
		kfree(vowserv.vow_fir_model);
		vowserv.vow_fir_model = NULL;
	}

	vow_service_GetParameter(arg);
	vowserv.vow_fir_model = kmalloc(vowserv.vow_info_apuser[2], 0);
	if (vow_service_CopyModel(VOW_MODEL_FIR, 0) != 0)
		return false;

	vowserv.vow_info_dsp[0] = VOW_MODEL_FIR;
	vowserv.vow_info_dsp[1] = 0;
	vowserv.vow_info_dsp[2] = dma_map_single(&dev,
						 vowserv.vow_fir_model,
						 vowserv.vow_info_apuser[2],
						 DMA_TO_DEVICE);
	vowserv.vow_info_dsp[3] = vowserv.vow_info_apuser[2];
	dma_sync_single_for_device(&dev,
				   vowserv.vow_info_dsp[2],
				   vowserv.vow_info_apuser[2],
				   DMA_TO_DEVICE);

	PRINTK_VOWDRV("SetFirModel:addr_%x, size_%x\n",
		      vowserv.vow_info_dsp[0], vowserv.vow_info_dsp[1]);

	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_SET_VOW_MODEL, (void *)&vowserv.vow_info_dsp[0], 4, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_SET_VOW_MODEL);
	return ret;
}

static bool vow_service_SetVBufAddr(unsigned long arg)
{
	vow_service_GetParameter(arg);

	pr_debug("SetVBufAddr:addr_%x, size_%x\n",
		 (unsigned int)vowserv.vow_info_apuser[1],
		 (unsigned int)vowserv.vow_info_apuser[2]);
	if (vowserv.voicedata_kernel_ptr != NULL)
		kfree(vowserv.voicedata_kernel_ptr);

	vowserv.voicedata_user_addr = vowserv.vow_info_apuser[1];
	vowserv.voicedata_user_size = vowserv.vow_info_apuser[2];
	vowserv.voicedata_user_return_size_addr = vowserv.vow_info_apuser[3];
	vowserv.voicedata_kernel_ptr = kmalloc(vowserv.voicedata_user_size, 0);

	return true;
}

static bool vow_service_Enable(void)
{
	bool ret;

	PRINTK_VOWDRV("vow_service_Enable\n");
	if ((vowserv.recording_flag) && (vowserv.suspend_lock == 0)) {
		vowserv.suspend_lock = 1;
		wake_lock(&VOW_suspend_lock); /* Let AP will not suspend */
		PRINTK_VOWDRV("==DEBUG MODE START==\n");
	}
	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_ENABLE, (void *)0, 0, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_VOW_ENABLE);
	return ret;
}

static bool vow_service_Disable(void)
{
	bool ret;

	PRINTK_VOWDRV("vow_service_Disable\n");
	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_DISABLE, (void *)0, 0, 0, 1) == false)
		;
	ret = vow_ipimsg_wait(AP_IPIMSG_VOW_DISABLE);
	if (vowserv.recording_flag) {
		vowserv.voice_length = 0;
		vow_service_getVoiceData();
	}
	if (vowserv.suspend_lock == 1) {
		vowserv.suspend_lock = 0;
		wake_unlock(&VOW_suspend_lock); /* Let AP will suspend */
		PRINTK_VOWDRV("==DEBUG MODE STOP==\n");
	}
	deregister_feature(VOW_FEATURE_ID);
	return ret;
}

static bool vow_service_SyncVoiceDataAck(void)
{
	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_DATAREADY_ACK, (void *)0, 0, 0, 1) == false)
		;
	return true;
}

void vow_memcpy_to_md32(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

void vow_memcpy_from_md32(void *src, void __iomem *trg, int size)
{
	int i;
	u32 __iomem *t = trg;
	u32 *s = src;

	for (i = 0; i < (size >> 2); i++)
		*s++ = *t++;
}
/*
#define ReadREG_VOW(_addr, _value) ((_value) = *(volatile unsigned int *)(_addr) )
#define WriteREG_VOW(_addr, _value) (*(volatile unsigned int *)(_addr) = (_value))
*/
#if 0
static int firstbuffer = 1;
#endif

static void vow_service_ReadVoiceData(void)
{
	short *ptr16;
	int stop_condition = 0;

	/*int rdata;*/
	while (1) {
		if (VoiceData_Wait_Queue_flag == 0)
			wait_event_interruptible(VoiceData_Wait_Queue, VoiceData_Wait_Queue_flag);

		if (VoiceData_Wait_Queue_flag == 1) {
			VoiceData_Wait_Queue_flag = 0;
			if ((VowDrv_GetHWStatus() == VOW_PWR_OFF) || (vowserv.recording_flag == false)) {
				vowserv.voicedata_idx = 0;
				stop_condition = 1;
				PRINTK_VOWDRV("stop read vow voice data\n");
			} else {
				ptr16 = (short *)vowserv.voicddata_scp_ptr;
				/* PRINTK_VOWDRV("[VOW] ReadDMA:%x\n", *ptr16); */
				/* ReadREG_VOW((MD32_DTCM+0xE240), rdata); */
#if defined OLD_METHOD
				dma_sync_single_for_cpu(&dev,
							vowserv.voicedata_scp_addr,
							VOW_VOICE_DATA_LENGTH_BYTES,
							DMA_FROM_DEVICE);
#endif
			#if 0
				if (firstbuffer == 1) {
					/*memcpy_from_md32(&vowserv.voicedata_kernel_ptr[vowserv.voicedata_idx],
							   MD32_DTCM+0xe1c0,
							   VOW_VOICE_DATA_LENGTH_BYTES);*/
					memcpy_from_md32(&vowserv.voicedata_kernel_ptr[vowserv.voicedata_idx],
							 MD32_DTCM + vowserv.voicedata_tcm_addr,
							 VOW_VOICE_DATA_LENGTH_BYTES);
					firstbuffer = 0;
				} else {
					/*memcpy_from_md32(&vowserv.voicedata_kernel_ptr[vowserv.voicedata_idx],
							   MD32_DTCM+0xe440,
							   VOW_VOICE_DATA_LENGTH_BYTES);*/
					memcpy_from_md32(&vowserv.voicedata_kernel_ptr[vowserv.voicedata_idx],
							 MD32_DTCM + vowserv.voicedata_tcm_addr + 640,
							 VOW_VOICE_DATA_LENGTH_BYTES);
					firstbuffer = 1;
				}
			#else
				/*PRINTK_VOWDRV("get once:%x\n",vowserv.voice_length);*/
				memcpy(&vowserv.voicedata_kernel_ptr[vowserv.voicedata_idx],
				       vowserv.voicddata_scp_ptr,
				       vowserv.voice_length);
				vow_service_SyncVoiceDataAck();
			#endif
				vowserv.voicedata_idx += (vowserv.voice_length >> 1);

				if (vowserv.voicedata_idx >= (VOW_VOICE_RECORD_BIG_THRESHOLD >> 1))
					vowserv.transfer_length = VOW_VOICE_RECORD_BIG_THRESHOLD;
				else
					vowserv.transfer_length = VOW_VOICE_RECORD_THRESHOLD;

				if (vowserv.voicedata_idx >= (vowserv.transfer_length >> 1)) {
					unsigned int ret;

					ret = copy_to_user((void __user *)(vowserv.voicedata_user_return_size_addr),
							   &vowserv.transfer_length,
							   4);

					ret = copy_to_user((void __user *)vowserv.voicedata_user_addr,
							   vowserv.voicedata_kernel_ptr,
							   vowserv.transfer_length);
					/* move left data to buffer's head */
					if (vowserv.voicedata_idx > (vowserv.transfer_length >> 1)) {
						memcpy(&vowserv.voicedata_kernel_ptr[0],
						       &vowserv.voicedata_kernel_ptr[(vowserv.transfer_length >> 1)],
						       (vowserv.voicedata_idx << 1) - vowserv.transfer_length);
						vowserv.voicedata_idx -= (vowserv.transfer_length >> 1);
					} else
						vowserv.voicedata_idx = 0;

					/*PRINTK_VOWDRV("TX Leng:%d, %d\n",
							vowserv.transfer_length, vowserv.voicedata_idx);*/

					stop_condition = 1;
				}
			}

			if (stop_condition == 1)
				break;

		}
	}
}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        V O W   C O N T R O L   F U N C T I O N S
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

static int VowDrv_SetHWStatus(int status)
{
	int ret = 0;

	PRINTK_VOWDRV("SetHWStatus:set:%x, cur:%x\n", status, vowserv.pwr_status);
	if ((status < NUM_OF_VOW_PWR_STATUS) && (status >= VOW_PWR_OFF)) {
		spin_lock(&vowdrv_lock);
		vowserv.pwr_status = status;
		spin_unlock(&vowdrv_lock);
		if (status == VOW_PWR_OFF) {
			VowDrv_Wait_Queue_flag = 1;
			wake_up_interruptible(&VowDrv_Wait_Queue);
		}
	} else {
		PRINTK_VOWDRV("VowDrv_SetHWStatus error input:%d\n", status);
		ret = -1;
	}
	return ret;
}

static int VowDrv_GetHWStatus(void)
{
	int ret = 0;

	spin_lock(&vowdrv_lock);
	ret = vowserv.pwr_status;
	spin_unlock(&vowdrv_lock);
	return ret;
}

int VowDrv_EnableHW(int status)
{
	int ret = 0;
	int pwr_status = 0;

	PRINTK_VOWDRV("VowDrv_EnableHW:%x\n", status);
	if (status < 0) {
		pr_debug("VowDrv_EnableHW error input:%x\n", status);
		ret = -1;
	} else {
		pwr_status = (status == 0)?VOW_PWR_OFF : VOW_PWR_ON;
		if (pwr_status == VOW_PWR_ON)
			register_feature(VOW_FEATURE_ID);

		VowDrv_SetHWStatus(pwr_status);
	}
	return ret;
}

int VowDrv_ChangeStatus(void)
{
	PRINTK_VOWDRV("VowDrv_ChangeStatus\n");
	spin_lock(&vowdrv_lock);
	VowDrv_Wait_Queue_flag = 1;
	spin_unlock(&vowdrv_lock);
	wake_up_interruptible(&VowDrv_Wait_Queue);
	return 0;
}

void VowDrv_SetSmartDevice(void)
{
	PRINTK_VOWDRV("VowDrv_SetSmartDevice\n");
	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_SET_SMART_DEVICE, (void *)0, 0, 0, 1) == false)
		;
}

void VowDrv_SetFlag(VOW_FLAG_TYPE type, bool set)
{
	pr_debug("VowDrv_SetFlag:type%x, set:%x\n", type, set);
	vowserv.vow_info_dsp[0] = type;
	vowserv.vow_info_dsp[1] = set;
	vowserv.ipimsgwait = true;
	while (vow_ipi_sendmsg(AP_IPIMSG_VOW_SET_FLAG, (void *)&vowserv.vow_info_dsp[0], 8, 0, 1) == false)
		;
	vow_ipimsg_wait(AP_IPIMSG_VOW_SET_FLAG);
}

void VowDrv_SetDmicLowPower(bool enable)
{
	VowDrv_SetFlag(VOW_FLAG_DMIC_LOWPOWER, enable);
}

static int VowDrv_SetVowEINTStatus(int status)
{
	int ret = 0;
	int wakeup_event = 0;

	if ((status < NUM_OF_VOW_EINT_STATUS) && (status >= VOW_EINT_DISABLE)) {
		spin_lock(&vowdrv_lock);
		if ((vowserv.eint_status != VOW_EINT_PASS) && (status == VOW_EINT_PASS))
			wakeup_event = 1;
		vowserv.eint_status = status;
		spin_unlock(&vowdrv_lock);
	} else {
		pr_debug("VowDrv_SetVowEINTStatus error input:%x\n", status);
		ret = -1;
	}
	return ret;
}

static int VowDrv_QueryVowEINTStatus(void)
{
	int ret = 0;

	spin_lock(&vowdrv_lock);
	ret = vowserv.eint_status;
	spin_unlock(&vowdrv_lock);
	PRINTK_VOWDRV("VowDrv_QueryVowEINTStatus :%d\n", ret);
	return ret;
}

static int VowDrv_open(struct inode *inode, struct file *fp)
{
	PRINTK_VOWDRV("VowDrv_open do nothing inode:%p, file:%p\n", inode, fp);
	return 0;
}

static int VowDrv_release(struct inode *inode, struct file *fp)
{
	PRINTK_VOWDRV("VowDrv_release inode:%p, file:%p\n", inode, fp);

	if (!(fp->f_mode & FMODE_WRITE || fp->f_mode & FMODE_READ))
		return -ENODEV;
	return 0;
}

static long VowDrv_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int  ret = 0;

	PRINTK_VOWDRV("VowDrv_ioctl cmd = %u arg = %lu\n", cmd, arg);
	PRINTK_VOWDRV("VowDrv_ioctl check arg = %u %u\n", VOWEINT_GET_BUFSIZE, VOW_SET_CONTROL);
	switch ((unsigned int)cmd) {
	case VOWEINT_GET_BUFSIZE:
		pr_debug("VOWEINT_GET_BUFSIZE\n");
		ret = sizeof(VOW_EINT_DATA_STRUCT);
		break;
	case VOW_GET_STATUS:
		pr_debug("VOW_GET_STATUS\n");
		ret = VowDrv_QueryVowEINTStatus();
		break;
	case VOW_SET_CONTROL:
		switch (arg) {
		case VOWControlCmd_Init:
			pr_debug("VOW_SET_CONTROL VOWControlCmd_Init");
			vow_service_Init();
			break;
		case VOWControlCmd_ReadVoiceData:
			if ((vowserv.recording_flag == true) && (vowserv.firstRead == true)) {
				vowserv.firstRead = false;
				VowDrv_SetFlag(VOW_FLAG_DEBUG, true);
			}
			vow_service_ReadVoiceData();
			break;
		case VOWControlCmd_EnableDebug:
			pr_debug("VOW_SET_CONTROL VOWControlCmd_EnableDebug");
			vowserv.voicedata_idx = 0;
			vowserv.recording_flag = true;
			vowserv.suspend_lock = 0;
			vowserv.voice_length = 0;
			vowserv.firstRead = true;
			/*VowDrv_SetFlag(VOW_FLAG_DEBUG, true);*/
			break;
		case VOWControlCmd_DisableDebug:
			pr_debug("VOW_SET_CONTROL VOWControlCmd_DisableDebug");
			VowDrv_SetFlag(VOW_FLAG_DEBUG, false);
			if (vowserv.recording_flag)
				vow_service_getVoiceData();
			vowserv.voice_length = 0;
			vowserv.firstRead = true;
			vowserv.recording_flag = false;
			break;
		default:
			pr_debug("VOW_SET_CONTROL no such command = %lu", arg);
			break;
		}
		break;
	case VOW_SET_SPEAKER_MODEL:
		pr_debug("VOW_SET_SPEAKER_MODEL(%lu)", arg);
		if (!vow_service_SetSpeakerModel(arg))
			ret = -EFAULT;
		break;
	case VOW_CLR_SPEAKER_MODEL:
		pr_debug("VOW_CLR_SPEAKER_MODEL(%lu)", arg);
		if (!vow_service_ReleaseSpeakerModel(arg))
			ret = -EFAULT;
		break;
	case VOW_SET_INIT_MODEL:
		pr_debug("VOW_SET_INIT_MODEL(%lu)", arg);
		if (!vow_service_SetInitModel(arg))
			ret = -EFAULT;
		break;
	case VOW_SET_FIR_MODEL:
		pr_debug("VOW_SET_FIR_MODEL(%lu)", arg);
		if (!vow_service_SetFirModel(arg))
			ret = -EFAULT;
		break;
	case VOW_SET_NOISE_MODEL:
		pr_debug("VOW_SET_NOISE_MODEL(%lu)", arg);
		if (!vow_service_SetNoiseModel(arg))
			ret = -EFAULT;
		break;
	case VOW_SET_APREG_INFO:
		pr_debug("VOW_SET_APREG_INFO(%lu)", arg);
		if (!vow_service_SetVBufAddr(arg))
			ret = -EFAULT;
		break;
	case VOW_SET_REG_MODE:
		pr_debug("VOW_SET_MODE(%lu)", arg);
		if (!vow_service_SetVowMode(arg))
			ret = -EFAULT;
		break;
	case VOW_FAKE_WAKEUP:
		vow_ipi_reg_ok(0);
		pr_debug("VOW_FAKE_WAKEUP(%lu)", arg);
		break;
	default:
		pr_debug("WrongParameter(%lu)", arg);
		break;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long VowDrv_compat_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	/*int err;*/
	PRINTK_VOWDRV("++VowDrv_compat_ioctl cmd = %u arg = %lu\n", cmd, arg);
	if (!fp->f_op || !fp->f_op->unlocked_ioctl) {
		(void)ret;
		return -ENOTTY;
	}
	switch (cmd) {
	case VOWEINT_GET_BUFSIZE:
	case VOW_GET_STATUS:
	case VOW_FAKE_WAKEUP:
	case VOW_SET_REG_MODE:
	case VOW_CLR_SPEAKER_MODEL:
	case VOW_SET_CONTROL:
		ret = fp->f_op->unlocked_ioctl(fp, cmd, arg);
		break;
	case VOW_SET_SPEAKER_MODEL:
	case VOW_SET_INIT_MODEL:
	case VOW_SET_FIR_MODEL:
	case VOW_SET_NOISE_MODEL:
	case VOW_SET_APREG_INFO: {
		VOW_MODEL_INFO_KERNEL_T __user *data32;

		VOW_MODEL_INFO_T __user *data;

		int err;
		compat_size_t l;
		compat_uptr_t p;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
#if 0
		err  = get_user(l, &data32->id);
		err |= put_user(l, (compat_size_t *)&data->id);
		err |= get_user(l, (compat_size_t *)&data32->addr);
		err |= put_user(l, (compat_size_t *)&data->addr);
		err |= get_user(l, (compat_size_t *)&data32->size);
		err |= put_user(l, (compat_size_t *)&data->size);
		err |= get_user(p, (compat_uptr_t *)&data32->data);
		err |= put_user(p, (compat_uptr_t *)&data->data);
#endif
		err  = get_user(l, &data32->id);
		err |= put_user(l, &data->id);
		err |= get_user(l, &data32->addr);
		err |= put_user(l, &data->addr);
		err |= get_user(l, &data32->size);
		err |= put_user(l, &data->size);
		err |= get_user(l, &data32->return_size_addr);
		err |= put_user(l, &data->return_size_addr);
		err |= get_user(p, (compat_uptr_t *)&data32->data);
		err |= put_user(p, (compat_uptr_t *)&data->data);
		PRINTK_VOWDRV("VowDrv_compat_ioctl_getpar:size: %lu %lu %lu %lu %lu\n",
			      data->id, data->addr, data->size, data->return_size_addr, (unsigned int long)data->data);
		ret = fp->f_op->unlocked_ioctl(fp, cmd, (unsigned long)data);
	}
		break;
	default:
		break;
	}
	PRINTK_VOWDRV("--VowDrv_compat_ioctl\n");
	return ret;
}
#endif

static ssize_t VowDrv_write(struct file *fp, const char __user *data, size_t count, loff_t *offset)
{
	/*PRINTK_VOWDRV("+VowDrv_write = %p count = %d\n",fp ,count);*/
	return 0;
}

static ssize_t VowDrv_read(struct file *fp,  char __user *data, size_t count, loff_t *offset)
{
	unsigned int read_count = 0;
	int ret = 0;

	PRINTK_VOWDRV("+VowDrv_read+\n");
	VowDrv_SetVowEINTStatus(VOW_EINT_RETRY);

	if (VowDrv_Wait_Queue_flag == 0)
		ret = wait_event_interruptible(VowDrv_Wait_Queue, VowDrv_Wait_Queue_flag);
	if (VowDrv_Wait_Queue_flag == 1) {
		VowDrv_Wait_Queue_flag = 0;
		if (VowDrv_GetHWStatus() == VOW_PWR_OFF) {
			vow_service_Disable();
			vowserv.md32_command_flag = false;
			VowDrv_SetVowEINTStatus(VOW_EINT_DISABLE);
		} else {
			vow_service_Enable();
			if (vowserv.md32_command_flag) {
				VowDrv_SetVowEINTStatus(VOW_EINT_PASS);
				pr_debug("Wakeup by MD32\n");
				if (vowserv.suspend_lock == 0)
					wake_lock_timeout(&VOW_suspend_lock, HZ / 2);
				vowserv.md32_command_flag = false;
			} else {
				pr_debug("Wakeup by other signal(%d,%d)\n",
					 VowDrv_Wait_Queue_flag, VowDrv_GetHWStatus());
			}
		}
	}

	VOW_EINT_DATA_STRUCT.eint_status = VowDrv_QueryVowEINTStatus();
	read_count = copy_to_user((void __user *)data,
				  &VOW_EINT_DATA_STRUCT,
				  sizeof(VOW_EINT_DATA_STRUCT));
	PRINTK_VOWDRV("+VowDrv_read-\n");
	return read_count;
}

static int VowDrv_flush(struct file *flip, fl_owner_t id)
{
	PRINTK_VOWDRV("+VowDrv_flush\n");
	PRINTK_VOWDRV("-VowDrv_flush\n");
	return 0;
}

static int VowDrv_fasync(int fd, struct file *flip, int mode)
{
	PRINTK_VOWDRV("VowDrv_fasync\n");
	/*return fasync_helper(fd, flip, mode, &VowDrv_fasync);*/
	return 0;
}

static int VowDrv_remap_mmap(struct file *flip, struct vm_area_struct *vma)
{
	PRINTK_VOWDRV("VowDrv_remap_mmap\n");
	return -1;
}

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        VOW Driver Registration
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/

/*****************************************************************************
 * VOW platform driver operations
****************************************************************************/
static int VowDrv_probe(struct platform_device *dev)
{
	PRINTK_VOWDRV("+VowDrv_probe\n");
	return 0;
}

static int VowDrv_remove(struct platform_device *dev)
{
	PRINTK_VOWDRV("+VowDrv_remove\n");
	/*[Todo]Add opearations*/
	PRINTK_VOWDRV("-VowDrv_remove\n");
	return 0;
}

static void VowDrv_shutdown(struct platform_device *dev)
{
	PRINTK_VOWDRV("+VowDrv_shutdown\n");
	PRINTK_VOWDRV("-VowDrv_shutdown\n");
}

static int VowDrv_suspend(struct platform_device *dev, pm_message_t state)
{
	/* only one suspend mode */
	PRINTK_VOWDRV("VowDrv_suspend\n");
	return 0;
}

static int VowDrv_resume(struct platform_device *dev) /* wake up */
{
	PRINTK_VOWDRV("VowDrv_resume\n");
	return 0;
}

static const struct file_operations VOW_fops = {
	.owner   = THIS_MODULE,
	.open    = VowDrv_open,
	.release = VowDrv_release,
	.unlocked_ioctl   = VowDrv_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = VowDrv_compat_ioctl,
#endif
	.write   = VowDrv_write,
	.read    = VowDrv_read,
	.flush   = VowDrv_flush,
	.fasync  = VowDrv_fasync,
	.mmap    = VowDrv_remap_mmap
};

static struct miscdevice VowDrv_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = VOW_DEVNAME,
	.fops = &VOW_fops,
};

const struct dev_pm_ops VowDrv_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.freeze = NULL,
	.thaw = NULL,
	.poweroff = NULL,
	.restore = NULL,
	.restore_noirq = NULL,
};


static struct platform_driver VowDrv_driver = {
	.probe    = VowDrv_probe,
	.remove   = VowDrv_remove,
	.shutdown = VowDrv_shutdown,
	.suspend  = VowDrv_suspend,
	.resume   = VowDrv_resume,
	.driver   = {
#ifdef CONFIG_PM
	.pm       = &VowDrv_pm_ops,
#endif
	.name     = vowdrv_name,
	},
};

static int VowDrv_mod_init(void)
{
	int ret = 0;

	PRINTK_VOWDRV("+VowDrv_mod_init\n");

	/* Register platform DRIVER */
	ret = platform_driver_register(&VowDrv_driver);
	if (ret) {
		PRINTK_VOWDRV("VowDrv Fail:%d - Register DRIVER\n", ret);
		return ret;
	}

	/* register MISC device */
	if (ret == misc_register(&VowDrv_misc_device)) {
		PRINTK_VOWDRV("VowDrv_probe misc_register Fail:%d\n", ret);
		return ret;
	}
/*
	// register cat /proc/audio
	create_proc_read_entry("audio", 0, NULL, AudDrv_Read_Procmem, NULL);

	create_proc_read_entry("ExtCodec", 0, NULL, AudDrv_Read_Proc_ExtCodec, NULL);


	wake_lock_init(&Audio_wake_lock, WAKE_LOCK_SUSPEND, "Audio_WakeLock");
	wake_lock_init(&Audio_record_wake_lock, WAKE_LOCK_SUSPEND, "Audio_Record_WakeLock");
*/

	PRINTK_VOWDRV("VowDrv_mod_init: Init Audio WakeLock\n");

	return 0;
}

static void  VowDrv_mod_exit(void)
{
	PRINTK_VOWDRV("+VowDrv_mod_exit\n");
	PRINTK_VOWDRV("-VowDrv_mod_exit\n");
}
module_init(VowDrv_mod_init);
module_exit(VowDrv_mod_exit);


/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        License
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek VOW Driver");
MODULE_AUTHOR("Charlie Lu<charlie.lu@mediatek.com>");
