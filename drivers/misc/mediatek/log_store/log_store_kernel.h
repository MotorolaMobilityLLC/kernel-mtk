/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef __LOG_STORE_H__
#define __LOG_STORE_H__

#include <linux/types.h>


#define SRAM_HEADER_SIG (0xabcd1234)
#define DRAM_HEADER_SIG (0x5678ef90)
#define LOG_STORE_SIG (0xcdab3412)


#define MAX_DRAM_COUNT	10

#define LOG_STORE_SIZE 0x20000	/*  DRAM buff  */

/*  log flag */
#define BUFF_VALIE		0x01
#define CAN_FREE		0x02
#define	NEED_SAVE_TO_EMMC	0x04
#define RING_BUFF		0x08
/* ring buf, if buf_full, buf_point is the start of the buf, else buf_point is the buf end, other buf is not used */
#define BUFF_FULL		0x10	/* buf is full */
#define ARRAY_BUFF		0X20	/* array buf type, buf_point is the used buf end */
#define BUFF_ALLOC_ERROR	0X40
#define BUFF_ERROR		0x80
#define BUFF_NOT_READY		0x100
#define BUFF_READY		0x200
#define BUFF_EARLY_PRINTK	0x400	/* pl or lk can printk the early printk information to uart cable */



struct pl_lk_log {
	u32 sig;	/* default 0xabcd1234 */
	u32 buff_size;	/* total buf size */
	u32 off_pl;	/* pl offset, sizeof(struct pl_lk_log) */
	u32 sz_pl;	/* preloader size */
	u32 pl_flag;	/* pl log flag */
	u32 off_lk;	/* lk offset, sizeof((struct pl_lk_log) + sz_pl */
	u32 sz_lk;	/* lk log size */
	u32 lk_flag;	/* lk log flag */
/*	u32 emmc_count;	 emmc save */
/*	u32 emmc_time; */
	u32 flag1;
	u32 flag2;
};



#define	LOG_PL_LK  0x0	/* Preloader and lk log buff */


/* total 100 char size. u32 25 */
struct dram_buf_header {
	u32 sig;
	u32 flag;
	u32 buf_addr;
	u32 buf_size;
	u32 buf_offsize;
	u32 buf_point;
	u32 reserve1[2];
	u32 reserve2[17];
};

/* total 1024 char size */
struct sram_log_header {
	u32 sig;
	u32 reboot_count;
	u32 save_to_emmc;
	u32 reserve[3];
	struct dram_buf_header dram_buf[MAX_DRAM_COUNT];
};

#ifdef CONFIG_MTK_DRAM_LOG_STORE
void log_store_bootup(void);
void store_log_to_emmc_enable(bool value);
void disable_early_log(void);


#else

static inline void  log_store_bootup(void)
{

}

static inline void store_log_to_emmc_enable(bool value)
{

}

static inline void disable_early_log(void)
{
}
#endif
#endif

