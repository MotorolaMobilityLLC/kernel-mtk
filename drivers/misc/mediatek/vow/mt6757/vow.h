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

#ifndef __VOW_H__
#define __VOW_H__

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <mt-plat/aee.h>

/***********************************************************************************
** VOW Control Message
************************************************************************************/
#define VOW_DEVNAME "vow"
#define VOW_IOC_MAGIC    'a'

/* if need assert , use VOW_ASSERT(true) */
#define VOW_ASSERT(value) BUG_ON(false)

/* below is control message */
#if 0

#define TEST_VOW_PRINT            _IO(VOW_IOC_MAGIC,  0x00)
#define VOWEINT_GET_BUFSIZE       _IOW(VOW_IOC_MAGIC, 0x01, unsigned long)
#define VOW_GET_STATUS            _IOW(VOW_IOC_MAGIC, 0x02, unsigned long)
#define VOW_SET_CONTROL           _IOW(VOW_IOC_MAGIC, 0x03, unsigned long)
#define VOW_SET_SPEAKER_MODEL     _IOW(VOW_IOC_MAGIC, 0x04, unsigned long)
#define VOW_CLR_SPEAKER_MODEL     _IOW(VOW_IOC_MAGIC, 0x05, unsigned long)
#define VOW_SET_INIT_MODEL        _IOW(VOW_IOC_MAGIC, 0x06, unsigned long)
#define VOW_SET_FIR_MODEL         _IOW(VOW_IOC_MAGIC, 0x07, unsigned long)
#define VOW_SET_NOISE_MODEL       _IOW(VOW_IOC_MAGIC, 0x08, unsigned long)
#define VOW_SET_APREG_INFO        _IOW(VOW_IOC_MAGIC, 0x09, unsigned long)
#define VOW_SET_REG_MODE          _IOW(VOW_IOC_MAGIC, 0x0A, unsigned long)
#define VOW_FAKE_WAKEUP           _IOW(VOW_IOC_MAGIC, 0x0B, unsigned long)

#else

#if 1
enum VOW_MESSAGE_TEMP {
	TEST_VOW_PRINT = 0,
	VOWEINT_GET_BUFSIZE,
	VOW_GET_STATUS,
	VOW_SET_CONTROL,
	VOW_SET_SPEAKER_MODEL,
	VOW_CLR_SPEAKER_MODEL,
	VOW_SET_INIT_MODEL,
	VOW_SET_FIR_MODEL,
	VOW_SET_NOISE_MODEL,
	VOW_SET_APREG_INFO,
	VOW_SET_REG_MODE,
	VOW_FAKE_WAKEUP,
};

#else

#define TEST_VOW_PRINT            0x00
#define VOWEINT_GET_BUFSIZE       0x01
#define VOW_GET_STATUS            0x02
#define VOW_SET_CONTROL           0x03
#define VOW_SET_SPEAKER_MODEL     0x04
#define VOW_CLR_SPEAKER_MODEL     0x05
#define VOW_SET_INIT_MODEL        0x06
#define VOW_SET_FIR_MODEL         0x07
#define VOW_SET_NOISE_MODEL       0x08
#define VOW_SET_APREG_INFO        0x09
#define VOW_SET_REG_MODE          0x0A
#define VOW_FAKE_WAKEUP           0x0B

#endif

#endif


/***********************************************************************************
** VOW IPI Service
************************************************************************************/
#define SCP_IPI_AUDMSG_BASE       0x5F00
#define AP_IPI_AUDMSG_BASE        0x7F00

#define CLR_SPEAKER_MODEL_FLAG    0x3535

/* #define MD32_DM_BASE          0xF0030000 */
/* #define MD32_PM_BASE          0xF0020000 */

#define MAX_VOW_SPEAKER_MODEL         10

#define VOW_SWAPTCM_TIMEOUT           50
#define VOW_IPIMSG_TIMEOUT            50
#define VOW_WAITCHECK_INTERVAL_MS      1
#define MAX_VOW_INFO_LEN               5
#define VOW_VOICE_DATA_LENGTH_BYTES  320
#define VOW_VOICE_RECORD_THRESHOLD   2560 /* 80ms */
#define VOW_VOICE_RECORD_BIG_THRESHOLD 8000 /* 1sec */
#define VOW_IPI_TIMEOUT              500 /* 500ms */

/***********************************************************************************
** Type Define
************************************************************************************/
enum VOW_Control_Cmd {
	VOWControlCmd_Init = 0,
	VOWControlCmd_ReadVoiceData,
	VOWControlCmd_EnableDebug,
	VOWControlCmd_DisableDebug,
};

typedef enum vow_ipi_msgid_t {
	/* AP to MD32 MSG */
	AP_IPIMSG_VOW_ENABLE = AP_IPI_AUDMSG_BASE,
	AP_IPIMSG_VOW_DISABLE,
	AP_IPIMSG_VOW_SETMODE,
	AP_IPIMSG_VOW_APREGDATA_ADDR,
	AP_IPIMSG_VOW_DATAREADY_ACK,
	AP_IPIMSG_SET_VOW_MODEL,
	AP_IPIMSG_VOW_SETGAIN,
	AP_IPIMSG_VOW_SET_FLAG,
	AP_IPIMSG_VOW_RECOGNIZE_OK_ACK,
	AP_IPIMSG_VOW_CHECKREG,
	AP_IPIMSG_VOW_SET_SMART_DEVICE,

	/* SCP to AP MSG */
	SCP_IPIMSG_VOW_ENABLE_ACK = SCP_IPI_AUDMSG_BASE,
	SCP_IPIMSG_VOW_DISABLE_ACK,
	SCP_IPIMSG_VOW_SETMODE_ACK,
	SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK,
	SCP_IPIMSG_VOW_DATAREADY,
	SCP_IPIMSG_SET_VOW_MODEL_ACK,
	SCP_IPIMSG_VOW_SETGAIN_ACK,
	SCP_IPIMSG_SET_FLAG_ACK,
	SCP_IPIMSG_VOW_RECOGNIZE_OK,
	SCP_IPIMSG_VOW_SET_SMART_DEVICE_ACK,
} vow_ipi_msgid_t;

typedef enum VOW_REG_MODE_T {
	VOW_MODE_SCP_VOW = 0,
	VOW_MODE_VOICECOMMAND,
	VOW_MODE_MULTIPLE_KEY,
	VOW_MODE_MULTIPLE_KEY_VOICECOMMAND
} VOW_REG_MODE_T;

typedef enum VOW_EINT_STATUS {
	VOW_EINT_DISABLE = -2,
	VOW_EINT_FAIL = -1,
	VOW_EINT_PASS = 0,
	VOW_EINT_RETRY = 1,
	NUM_OF_VOW_EINT_STATUS
} VOW_EINT_STATUS;

typedef enum VOW_FLAG_TYPE {
	VOW_FLAG_DEBUG,
	VOW_FLAG_PRE_LEARN,
	VOW_FLAG_DMIC_LOWPOWER,
	NUM_OF_VOW_FLAG_TYPE
} VOW_FLAG_TYPE;

typedef enum VOW_PWR_STATUS {
	VOW_PWR_OFF = 0,
	VOW_PWR_ON = 1,
	NUM_OF_VOW_PWR_STATUS
} VOW_PWR_STATUS;

typedef enum VOW_IPI_RESULT {
	VOW_IPI_SUCCESS = 0,
	VOW_IPI_CLR_SMODEL_ID_NOTMATCH,
	VOW_IPI_SET_SMODEL_NO_FREE_SLOT,
} VOW_IPI_RESULT;

enum VOW_MODEL_TYPE {
	VOW_MODEL_INIT = 0,
	VOW_MODEL_SPEAKER = 1,
	VOW_MODEL_NOISE = 2,
	VOW_MODEL_FIR = 3
};

typedef struct {
	short id;
	short size;
	short *buf;
} vow_ipi_msg_t;

struct VOW_EINT_DATA_STRUCT {
	int size;        /* size of data section */
	int eint_status; /* eint status */
	int id;
	char *data;      /* reserved for future extension */
} VOW_EINT_DATA_STRUCT;


#ifdef CONFIG_COMPAT

typedef struct {
	void *model_ptr;
	int  id;
	int  enabled;
} VOW_SPEAKER_MODEL_T;

typedef struct {
	long  id;
	long  addr;
	long  size;
	long  return_size_addr;
	void *data;
} VOW_MODEL_INFO_T;

typedef struct {
	compat_uptr_t *model_ptr;
	compat_size_t  id;
	compat_size_t  enabled;
} VOW_SPEAKER_MODEL_KERNEL_T;

typedef struct {
	compat_size_t  id;
	compat_size_t  addr;
	compat_size_t  size;
	compat_size_t  return_size_addr;
	compat_uptr_t *data;
} VOW_MODEL_INFO_KERNEL_T;

#else
typedef struct {
	void *model_ptr;
	int  id;
	int  enabled;
} VOW_SPEAKER_MODEL_T;

typedef struct {
	long  id;
	long  addr;
	long  size;
	long  return_size_addr;
	void *data;
} VOW_MODEL_INFO_T;


#endif

void VowDrv_SetFlag(VOW_FLAG_TYPE type, bool set);

/*
#define ReadREG(_addr, _value) ((_value) = *(volatile unsigned int *)(_addr) )
#define WriteREG(_addr, _value) (*(volatile unsigned int *)(_addr) = (_value))
#define ReadREG16(_addr, _value) ((_value) = *(volatile unsigned short *)(_addr) )
#define WriteREG16(_addr, _value) (*(volatile unsigned short *)(_addr) = (_value))
*/
#endif /*__VOW_H__ */
