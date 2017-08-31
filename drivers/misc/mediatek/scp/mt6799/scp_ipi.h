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

#ifndef __SCP_IPI_H
#define __SCP_IPI_H



#define SCP_A_TO_HOST_REG        (*(volatile unsigned int *)(scpreg.cfg + 0x001C))
	#define SCP_IRQ_SCP2HOST     (1 << 0)
	#define SCP_IRQ_WDT          (1 << 8)

#define SCP_TO_SPM_REG           (*(volatile unsigned int *)(scpreg.cfg + 0x0020))
#define GIPC_TO_SCP_REG          (*(volatile unsigned int *)(scpreg.cfg + 0x0028))
#define SCP_A_DEBUG_PC_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00B4))
#define SCP_A_DEBUG_PSP_REG      (*(volatile unsigned int *)(scpreg.cfg + 0x00B0))
#define SCP_A_DEBUG_LR_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00AC))
#define SCP_A_DEBUG_SP_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00A8))
#define SCP_A_WDT_REG            (*(volatile unsigned int *)(scpreg.cfg + 0x0084))

#define SCP_A_GENERAL_REG0       (*(volatile unsigned int *)(scpreg.cfg + 0x0050))
#define SCP_A_GENERAL_REG1       (*(volatile unsigned int *)(scpreg.cfg + 0x0054))
#define SCP_A_GENERAL_REG2       (*(volatile unsigned int *)(scpreg.cfg + 0x0058))
#define SCP_A_GENERAL_REG3       (*(volatile unsigned int *)(scpreg.cfg + 0x005C))
#define SCP_A_GENERAL_REG4       (*(volatile unsigned int *)(scpreg.cfg + 0x0060))
#define SCP_A_GENERAL_REG5       (*(volatile unsigned int *)(scpreg.cfg + 0x0064))
#define SCP_A_GENERAL_REG6       (*(volatile unsigned int *)(scpreg.cfg + 0x0068))
#define SCP_A_GENERAL_REG7       (*(volatile unsigned int *)(scpreg.cfg + 0x006C))
#define SCP_A_SLEEP_DEBUG_REG    (*(volatile unsigned int *)(scpreg.clkctrl + 0x0028))

#define SCP_SLEEP_STATUS_REG     (*(volatile unsigned int *)(scpreg.cfg + 0x0114))
	#define SCP_A_IS_SLEEP          (1<<0)
	#define SCP_A_IS_DEEPSLEEP      (1<<1)
	#define SCP_B_IS_SLEEP          (1<<2)
	#define SCP_B_IS_DEEPSLEEP      (1<<3)

#define SCP_B_TO_HOST_REG        (*(volatile unsigned int *)(scpreg.cfg + 0x021C))
#define SCP_B_DEBUG_PC_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02B4))
#define SCP_B_DEBUG_PSP_REG      (*(volatile unsigned int *)(scpreg.cfg + 0x02B0))
#define SCP_B_DEBUG_LR_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02AC))
#define SCP_B_DEBUG_SP_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02A8))
#define SCP_B_WDT_REG            (*(volatile unsigned int *)(scpreg.cfg + 0x0284))

#define SCP_B_GENERAL_REG0       (*(volatile unsigned int *)(scpreg.cfg + 0x0250))
#define SCP_B_GENERAL_REG1       (*(volatile unsigned int *)(scpreg.cfg + 0x0254))
#define SCP_B_GENERAL_REG2       (*(volatile unsigned int *)(scpreg.cfg + 0x0258))
#define SCP_B_GENERAL_REG3       (*(volatile unsigned int *)(scpreg.cfg + 0x025C))
#define SCP_B_GENERAL_REG4       (*(volatile unsigned int *)(scpreg.cfg + 0x0260))
#define SCP_B_GENERAL_REG5       (*(volatile unsigned int *)(scpreg.cfg + 0x0264))
#define SCP_B_GENERAL_REG6       (*(volatile unsigned int *)(scpreg.cfg + 0x0268))
#define SCP_B_GENERAL_REG7       (*(volatile unsigned int *)(scpreg.cfg + 0x026C))
#define SCP_B_SLEEP_DEBUG_REG    (*(volatile unsigned int *)(scpreg.clkctrldual + 0x0028))


#define HOST_TO_SCP_A       (1 << 0)
#define HOST_TO_SCP_B       (1 << 1)


#define SHARE_BUF_SIZE 288

/* scp Core ID definition*/
typedef enum scp_core_id {
	SCP_A_ID = 0,
	SCP_B_ID,
	SCP_CORE_TOTAL = 2,
} scp_core_id;

/* scp ipi ID definition
 * need to sync with SCP-side
 */
typedef enum ipi_id {
	IPI_WDT = 0,
	IPI_TEST1,
	IPI_TEST2,
	IPI_LOGGER_ENABLE,
	IPI_LOGGER_WAKEUP,
	IPI_LOGGER_INIT_A,
	IPI_LOGGER_INIT_B,
	IPI_SWAP,
	IPI_VOW,
	IPI_AUDIO,
	IPI_THERMAL,
	IPI_SPM,
	IPI_DVT_TEST,
	IPI_I2C,
	IPI_TOUCH,
	IPI_SENSOR,
	IPI_TIME_SYNC,
	IPI_GPIO,
	IPI_BUF_FULL,
	IPI_DFS,
	IPI_SHF,
	IPI_CONSYS,
	IPI_OCD,
	IPI_APDMA,
	IPI_IRQ_AST,
	IPI_DFS4MD,
	IPI_SCP_A_READY,
	IPI_SCP_B_READY,
	IPI_ETM_DUMP,
	IPI_APCCCI,
	IPI_SCP_A_RAM_DUMP,
	IPI_SCP_B_RAM_DUMP,
	IPI_DVFS_DEBUG,
	IPI_DVFS_FIX_OPP_SET,
	IPI_DVFS_FIX_OPP_EN,
	IPI_DVFS_LIMIT_OPP_SET,
	IPI_DVFS_LIMIT_OPP_EN,
	IPI_DVFS_DISABLE,
	IPI_DVFS_SLEEP,
	IPI_DVFS_WAKE,
	IPI_DUMP_REG,
	IPI_SCP_STATE,
	IPI_DVFS_SET_FREQ,
	IPI_CHRE,
	IPI_CHREX,
	IPI_SCP_PLL_CTRL,
	IPI_DO_AP_MSG,
	IPI_DO_SCP_MSG,
	IPI_MET_SCP,
	IPI_SCP_TIMER,
	SCP_NR_IPI,
} ipi_id;

typedef enum ipi_status {
	ERROR = -1,
	DONE,
	BUSY,
} ipi_status;

typedef void (*ipi_handler_t)(int id, void *data, unsigned int len);

struct scp_ipi_desc {
	ipi_handler_t handler;
	const char *name;
};


struct share_obj {
	enum ipi_id id;
	unsigned int len;
	unsigned char reserve[8];
	unsigned char share_buf[SHARE_BUF_SIZE - 16];
};

extern ipi_status scp_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name);
extern ipi_status scp_ipi_unregistration(enum ipi_id id);
extern ipi_status scp_ipi_send(enum ipi_id id, void *buf, unsigned int len, unsigned int wait, enum scp_core_id scp_id);

extern void scp_A_ipi_handler(void);
extern void scp_B_ipi_handler(void);
extern int wake_up_scp(void);

extern unsigned char *scp_A_send_buff;
extern unsigned char *scp_A_recv_buff;
extern unsigned char *scp_B_send_buff;
extern unsigned char *scp_B_recv_buff;

extern int scp_awake_lock(scp_core_id scp_id);
extern int scp_awake_unlock(scp_core_id scp_id);
extern unsigned int is_scp_ready(scp_core_id scp_id);
#endif
