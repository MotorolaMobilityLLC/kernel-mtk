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

struct scp_regs {
	void __iomem *sram;
	void __iomem *cfg;
	void __iomem *clkctrl;
	int irq;
	unsigned int tcmsize;
	unsigned int cfgregsize;
};

struct scp_work_struct {
	struct work_struct work;
	unsigned int flags;
};

extern const struct file_operations scp_file_ops;
extern struct scp_regs scpreg;
extern struct device_attribute dev_attr_scp_log_len, dev_attr_scp_mobile_log, dev_attr_scp_log_flush;
extern struct device_attribute dev_attr_scp_status;
extern struct bin_attribute bin_attr_scp_dump;

extern irqreturn_t scp_irq_handler(int irq, void *dev_id);
extern int scp_logger_init(void);
extern void scp_logger_stop(void);
extern void scp_logger_cleanup(void);
extern void scp_excep_init(void);
extern void scp_irq_init(void);
extern void scp_ipi_init(void);
extern void scp_register_notify(struct notifier_block *nb);
extern void scp_unregister_notify(struct notifier_block *nb);
extern void scp_schedule_work(struct scp_work_struct *scp_ws);
extern unsigned int is_scp_ready(void);
extern void scp_ram_dump_init(void);

#define SCP_CFGREG_SIZE		(scpreg.cfgregsize)
#define SCP_TCM_SIZE		(scpreg.tcmsize)
#define SCP_TCM			(scpreg.sram)
#define SCP_DTCM		SCP_TCM
#define SCP_SHARE_BUFFER	(scpreg.sram + SCP_TCM_SIZE - SHARE_BUF_SIZE*2)
#define SCP_BASE		(scpreg.cfg)
#define SCP_SEMAPHORE		(SCP_BASE  + 0x90)
#define SCP_CLK_CTRL_BASE	(scpreg.clkctrl)

#define SCP_AED_PHY_SIZE	(SCP_TCM_SIZE + SCP_CFGREG_SIZE)
#define SCP_AED_STR_LEN		(512)

/* This structre need to sync with SCP-side */
typedef struct {
	unsigned int scp_irq_ast_time;
	unsigned int scp_irq_ast_pc_s;
	unsigned int scp_irq_ast_pc_e;
	unsigned int scp_irq_ast_lr_s;
	unsigned int scp_irq_ast_lr_e;
	unsigned int scp_irq_ast_irqd;
} SCP_IRQ_AST_INFO;

/* SCP notify event */
enum SCP_NOTIFY_EVENT {
	SCP_EVENT_READY = 0,
	SCP_EVENT_STOP,
};

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
/* reserve memory ID */
typedef enum {
	VOW_MEM_ID      = 0,
	SENS_MEM_ID     = 1,
	MP3_MEM_ID      = 2,
	FLP_MEM_ID      = 3,
	RTOS_MEM_ID     = 4,
	OPENDSP_MEM_ID	= 5,
	SENS_MEM_DIRECT_ID	= 6,
	NUMS_MEM_ID     = 7,
} scp_reserve_mem_id_t;
typedef struct {
	scp_reserve_mem_id_t num;
	u64 start_phys;
	u64 start_virt;
	u64 size;
} scp_reserve_mblock_t;

#define FREQ_354MHZ (354)
#define FREQ_224MHZ (224)
#define FREQ_112MHZ (112)
/* feature ID list */
typedef enum {
	VOW_FEATURE_ID		= 0,
	OPEN_DSP_FEATURE_ID	= 1,
	SENS_FEATURE_ID		= 2,
	MP3_FEATURE_ID		= 3,
	FLP_FEATURE_ID		= 4,
	RTOS_FEATURE_ID		= 5,
	VCORE_TEST_FEATURE_ID	= 6,
	VCORE_TEST2_FEATURE_ID	= 7,
	NUM_FEATURE_ID		= 8,
} feature_id_t;

typedef struct {
	uint32_t feature;
	uint32_t freq;
	uint32_t enable;
} scp_feature_table_t;

/* @group_id: the group want to swap in tcm and run. */
extern int get_scp_semaphore(int flag);
extern int release_scp_semaphore(int flag);
extern unsigned int is_scp_ready(void);
extern void memcpy_to_scp(void __iomem *trg, const void *src, int size);
extern void memcpy_from_scp(void *trg, const void __iomem *src, int size);
extern int reset_scp(int reset);
extern void scp_register_notify(struct notifier_block *nb);
extern void scp_unregister_notify(struct notifier_block *nb);
extern void scp_aee_last_reg(void);
extern phys_addr_t get_reserve_mem_phys(scp_reserve_mem_id_t id);
extern phys_addr_t get_reserve_mem_virt(scp_reserve_mem_id_t id);
extern phys_addr_t get_reserve_mem_size(scp_reserve_mem_id_t id);
extern void register_feature(feature_id_t id);
extern void deregister_feature(feature_id_t id);
extern uint32_t get_freq(void);
extern int request_freq(void);
extern int check_scp_resource(void);
extern int scp_check_dram_resource(void);

#endif
