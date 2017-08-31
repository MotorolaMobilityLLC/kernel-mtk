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

#ifndef __SCP_HELPER_H__
#define __SCP_HELPER_H__

#include <linux/notifier.h>
#include <linux/interrupt.h>


/* scp config reg. definition*/
#define SCP_CFGREG_SIZE		(scpreg.cfgregsize)
#define SCP_TCM_SIZE		(scpreg.total_tcmsize)
#define SCP_A_TCM_SIZE		(scpreg.scp_tcmsize)
#define SCP_TCM			(scpreg.sram)
#define SCP_DTCM		SCP_TCM
#define SCP_A_SHARE_BUFFER	(scpreg.sram + SCP_A_TCM_SIZE -  SHARE_BUF_SIZE*2)
#define SCP_B_SHARE_BUFFER	(scpreg.sram + SCP_TCM_SIZE -  SHARE_BUF_SIZE*2)
#define SCP_BASE		(scpreg.cfg)
#define SCP_SEMAPHORE		(SCP_BASE  + 0x90)
#define SCP_SEMAPHORE_3WAY		(SCP_BASE  + 0x94)
#define SCP_CLK_CTRL_BASE	(scpreg.clkctrl)
#define SCP_CLK_CTRL_DUAL_BASE	(scpreg.clkctrldual)
#define SCP_BASE_DUAL		(scpreg.cfg + 0x200)
/* scp aed definition*/
#define SCP_AED_PHY_SIZE	(SCP_TCM_SIZE + SCP_CFGREG_SIZE)
#define SCP_AED_STR_LEN		(512)

/* scp logger size definition*/
#define LOG_TO_AP_UART_LINE 64
#define SCP_FIFO_SIZE 2048

/* emi mpu define*/
#define ENABLE_SCP_EMI_PROTECTION       (0)
#define MPU_REGION_ID_SCP_SMEM       6

/* scp power frequence definition*/
#define FREQ_354MHZ (354)
#define FREQ_224MHZ (224)
#define FREQ_112MHZ (112)

/* scp platform configs*/
#define SCP_BOOT_TIME_OUT_MONITOR       (0)
#define SCP_LOGGER_ENABLE               (1)
/* This structre need to sync with SCP-side */
typedef struct {
	unsigned int scp_irq_ast_time;
	unsigned int scp_irq_ast_pc_s;
	unsigned int scp_irq_ast_pc_e;
	unsigned int scp_irq_ast_lr_s;
	unsigned int scp_irq_ast_lr_e;
	unsigned int scp_irq_ast_irqd;
} SCP_IRQ_AST_INFO;

/* scp notify event */
enum SCP_NOTIFY_EVENT {
	SCP_EVENT_READY = 0,
	SCP_EVENT_STOP,
};

/* scp semaphore definition*/
enum SEMAPHORE_FLAG {
	SEMAPHORE_CLK_CFG_5 = 0,
	SEMAPHORE_PTP,
	SEMAPHORE_I2C0,
	SEMAPHORE_I2C1,
	SEMAPHORE_TOUCH,
	SEMAPHORE_APDMA,
	SEMAPHORE_SENSOR,
	NR_FLAG = 8,
};

/* scp semaphore definition*/
enum SEMAPHORE_3WAY_FLAG {
	SEMA_3WAY_APDMA = 0,
	SEMA_3WAY_TOTAL = 8,
};

struct scp_regs {
	void __iomem *sram;
	void __iomem *cfg;
	void __iomem *clkctrl;
	void __iomem *clkctrldual;
	int irq;
	int irq_dual;
	unsigned int total_tcmsize;
	unsigned int cfgregsize;
	unsigned int scp_tcmsize;
};

/* scp work struct definition*/
struct scp_work_struct {
	struct work_struct work;
	unsigned int flags;
};

/* scp feature ID list */
typedef enum {
	VOW_FEATURE_ID		= 0,
	OPEN_DSP_FEATURE_ID	= 1,
	SENS_FEATURE_ID		= 2,
	MP3_FEATURE_ID		= 3,
	FLP_FEATURE_ID		= 4,
	RTOS_FEATURE_ID		= 5,
	NUM_FEATURE_ID		= 6,
} feature_id_t;

typedef struct {
	uint32_t feature;
	uint32_t freq;
	uint32_t core;
	uint32_t enable;
} scp_feature_table_t;

/* scp reserve memory ID definition*/
typedef enum {
	VOW_MEM_ID      = 0,
	SENS_MEM_ID     = 1,
	MP3_MEM_ID      = 2,
	FLP_MEM_ID      = 3,
	RTOS_MEM_ID     = 4,
	OPENDSP_MEM_ID	= 5,
	SENS_MEM_DIRECT_ID	= 6,
	SCP_A_LOGGER_MEM_ID = 7,
	SCP_B_LOGGER_MEM_ID = 8,
	NUMS_MEM_ID     = 9,
} scp_reserve_mem_id_t;

typedef struct {
	scp_reserve_mem_id_t num;
	u64 start_phys;
	u64 start_virt;
	u64 size;
} scp_reserve_mblock_t;

extern struct device_attribute dev_attr_scp_A_mobile_log_UT;
extern struct device_attribute dev_attr_scp_B_mobile_log_UT;
extern struct device_attribute dev_attr_scp_A_logger_wakeup_AP;
extern struct device_attribute dev_attr_scp_B_logger_wakeup_AP;
extern const struct file_operations scp_A_log_file_ops;
extern const struct file_operations scp_B_log_file_ops;
extern struct scp_regs scpreg;
extern struct device_attribute dev_attr_scp_A_log_len;
extern struct device_attribute dev_attr_scp_A_mobile_log;
extern struct device_attribute dev_attr_scp_B_mobile_log;
extern struct device_attribute dev_attr_scp_A_log_flush;
extern struct device_attribute dev_attr_scp_A_status, dev_attr_scp_B_status;
extern struct bin_attribute bin_attr_scp_A_dump, bin_attr_scp_B_dump;

extern irqreturn_t scp_A_irq_handler(int irq, void *dev_id);
extern irqreturn_t scp_B_irq_handler(int irq, void *dev_id);
extern int scp_logger_init(phys_addr_t, phys_addr_t);
extern int scp_B_logger_init(phys_addr_t, phys_addr_t);
extern void scp_logger_stop(void);
extern void scp_logger_cleanup(void);
extern void scp_excep_init(void);
extern void scp_A_irq_init(void);
extern void scp_B_irq_init(void);
extern void scp_A_ipi_init(void);
extern void scp_B_ipi_init(void);
extern void scp_A_register_notify(struct notifier_block *nb);
extern void scp_A_unregister_notify(struct notifier_block *nb);
extern void scp_B_register_notify(struct notifier_block *nb);
extern void scp_B_unregister_notify(struct notifier_block *nb);
extern void scp_schedule_work(struct scp_work_struct *scp_ws);
extern void scp_ram_dump_init(void);
extern void scp_aee_last_reg(void);

extern int get_scp_semaphore(int flag);
extern int release_scp_semaphore(int flag);
extern int scp_get_semaphore_3way(int flag);
extern int scp_release_semaphore_3way(int flag);

extern unsigned int is_scp_A_ready(void);
extern unsigned int is_scp_B_ready(void);
extern void memcpy_to_scp(void __iomem *trg, const void *src, int size);
extern void memcpy_from_scp(void *trg, const void __iomem *src, int size);
extern int reset_scp(int reset);

extern phys_addr_t scp_get_reserve_mem_phys(scp_reserve_mem_id_t id);
extern phys_addr_t scp_get_reserve_mem_virt(scp_reserve_mem_id_t id);
extern phys_addr_t scp_get_reserve_mem_size(scp_reserve_mem_id_t id);
extern uint32_t scp_get_freq(void);
extern int scp_request_freq(void);
extern int scp_check_resource(void);
extern void scp_register_feature(feature_id_t id);
extern void scp_deregister_feature(feature_id_t id);

#endif
