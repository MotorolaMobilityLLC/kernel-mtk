/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __ADSP_HELPER_H__
#define __ADSP_HELPER_H__

#include <linux/notifier.h>
#include <linux/interrupt.h>
#include "adsp_reg.h"
#include "adsp_feature_define.h"

#include "adsp_ipi.h"


//#define FPGA_EARLY_DEVELOPMENT
#define MPU_NONCACHEABLE_PATCH          (1)
#define ADSP_SLEEP_ENABLE               (1)

/* adsp config reg. definition*/
#define ADSP_A_TCM_SIZE         (adspreg.total_tcmsize)
#define ADSP_A_ITCM_SIZE        (adspreg.i_tcmsize)
#define ADSP_A_DTCM_SIZE        (adspreg.d_tcmsize)
#define ADSP_A_ITCM             (adspreg.iram)
#define ADSP_A_DTCM             (adspreg.dram)
#define ADSP_A_SYS_DRAM         (adspreg.sysram) /* Liang:  from preloader */

#define ADSP_A_CFG             (adspreg.cfg)
#define ADSP_A_CFG_SIZE        (adspreg.cfgregsize)

#define ADSP_A_DTCM_SHARE_BASE  (adspreg.dram + adspreg.d_tcmsize)
#define ADSP_A_MPUINFO_BUFFER   (ADSP_A_DTCM_SHARE_BASE - 0x0020)
#define ADSP_A_OSTIMER_BUFFER   (ADSP_A_DTCM_SHARE_BASE - 0x0040)
#define ADSP_A_IPC_BUFFER       (ADSP_A_DTCM_SHARE_BASE - 0x0280)
#define ADSP_A_AUDIO_IPI_BUFFER (ADSP_A_DTCM_SHARE_BASE - 0x0680)
#define ADSP_A_WAKELOCK_BUFFER  (ADSP_A_DTCM_SHARE_BASE - 0x0684)

#define ADSP_A_AP_AWAKE         (ADSP_A_WAKELOCK_BUFFER)

/* Non-Cacheable MPU entry start from this ID. */
#define ADSP_MPU_NONCACHE_ID ADSP_A_DEBUG_DUMP_MEM_ID
#define ADSP_MPU_DATA_RO_ID

/* This structre need to sync with ADSP-side */
struct ADSP_IRQ_AST_INFO {
	unsigned int adsp_irq_ast_time;
	unsigned int adsp_irq_ast_pc_s;
	unsigned int adsp_irq_ast_pc_e;
	unsigned int adsp_irq_ast_lr_s;
	unsigned int adsp_irq_ast_lr_e;
	unsigned int adsp_irq_ast_irqd;
};

struct adsp_mpu_info_t {
	uint32_t	adsp_mpu_prog_addr;
	uint32_t	adsp_mpu_prog_size;
	uint32_t	adsp_mpu_data_addr;
	uint32_t	adsp_mpu_data_size;
#ifdef MPU_NONCACHEABLE_PATCH
	uint32_t	adsp_mpu_data_non_cache_addr;
	uint32_t	adsp_mpu_data_non_cache_size;
#else
	uint32_t	adsp_mpu_data_ro_addr;
	uint32_t	adsp_mpu_data_ro_size;
#endif
	uint32_t	adsp_mpu_md_data_addr;
	uint32_t	adsp_mpu_md_data_size;
};

/* adsp notify event */
enum ADSP_NOTIFY_EVENT {
	ADSP_EVENT_READY = 0,
	ADSP_EVENT_STOP,
};


/* adsp semaphore definition*/
enum ADSP_SEMAPHORE_FLAG {
	ADSP_SEMAPHORE_CLK_CFG_5 = 0,
	ADSP_SEMAPHORE_PTP,
	ADSP_SEMAPHORE_I2C0,
	ADSP_SEMAPHORE_I2C1,
	ADSP_SEMAPHORE_TOUCH,
	ADSP_SEMAPHORE_APDMA,
	ADSP_SEMAPHORE_SENSOR,
	ADSP_SEMAPHORE_ADSP_A_AWAKE,
	ADSP_SEMAPHORE_ADSP_B_AWAKE,
	ADSP_NR_FLAG = 9,
};


struct adsp_regs {
	void __iomem *adspsys;
	void __iomem *iram;
	void __iomem *dram;
	void __iomem *sysram;
	void __iomem *cfg;
	void __iomem *clkctrl;
	int wdt_irq;
	int ipc_irq;
	unsigned int i_tcmsize;
	unsigned int d_tcmsize;
	unsigned int cfgregsize;
	unsigned int total_tcmsize;
};

/* adsp work struct definition*/
struct adsp_work_struct {
	struct work_struct work;
	unsigned int flags;
	unsigned int id;
};

/* adsp reserve memory ID definition*/
enum adsp_reserve_mem_id_t {
	ADSP_A_SYSTEM_MEM_ID, /*not for share, reserved for system*/
	ADSP_A_IPI_MEM_ID,
	ADSP_A_LOGGER_MEM_ID,
#ifndef FPGA_EARLY_DEVELOPMENT
	ADSP_A_TRAX_MEM_ID,
	ADSP_SPK_PROTECT_MEM_ID,
	ADSP_VOIP_MEM_ID,
	ADSP_A2DP_PLAYBACK_MEM_ID,
	ADSP_OFFLOAD_MEM_ID,
	ADSP_EFFECT_MEM_ID,
	ADSP_VOICE_CALL_MEM_ID,
	ADSP_AFE_MEM_ID,
	ADSP_PLAYBACK_MEM_ID,
	ADSP_DEEPBUF_MEM_ID,
	ADSP_PRIMARY_MEM_ID,
	ADSP_CAPTURE_UL1_MEM_ID,
#endif
	ADSP_A_DEBUG_DUMP_MEM_ID,
	ADSP_A_CORE_DUMP_MEM_ID,

	ADSP_NUMS_MEM_ID,
};

struct adsp_reserve_mblock {
	enum adsp_reserve_mem_id_t num;
	u64 start_phys;
	u64 start_virt;
	u64 size;
};

/* adsp device attribute */
extern struct device_attribute dev_attr_adsp_A_mobile_log_UT;
extern struct device_attribute dev_attr_adsp_A_logger_wakeup_AP;
extern const struct file_operations adsp_A_log_file_ops;

extern struct adsp_regs adspreg;
extern struct device_attribute dev_attr_adsp_mobile_log;
extern struct device_attribute dev_attr_adsp_A_get_last_log;
extern struct device_attribute dev_attr_adsp_A_status;

extern struct bin_attribute bin_attr_adsp_dump;
extern struct bin_attribute bin_attr_adsp_dump_ke;
#if ADSP_TRAX
extern struct device_attribute dev_attr_adsp_A_trax;
extern struct bin_attribute bin_attr_adsp_trax;
extern int adsp_trax_init(void);
#endif

#if ADSP_SLEEP_ENABLE
extern struct device_attribute dev_attr_adsp_sleep_test;
extern struct device_attribute dev_attr_adsp_force_adsppll;
extern struct device_attribute dev_attr_adsp_spm_req;
extern struct device_attribute dev_attr_adsp_keep_adspll_on;
#endif

/* adsp loggger */
extern int adsp_logger_init(phys_addr_t start, phys_addr_t limit);

extern void adsp_logger_stop(void);
extern void adsp_logger_cleanup(void);

/* adsp exception */
extern int adsp_excep_init(void);
extern void adsp_ram_dump_init(void);
extern void adsp_excep_cleanup(void);
extern void adsp_aee_last_reg(void);

/* adsp irq */
extern irqreturn_t adsp_A_irq_handler(int irq, void *dev_id);
extern irqreturn_t adsp_A_wdt_handler(int irq, void *dev_id);

extern void adsp_A_irq_init(void);
extern void adsp_A_ipi_init(void);

/* adsp helper */
extern void adsp_A_register_notify(struct notifier_block *nb);
extern void adsp_A_unregister_notify(struct notifier_block *nb);
extern void adsp_schedule_work(struct adsp_work_struct *adsp_ws);

extern int get_adsp_semaphore(int flag);
extern int release_adsp_semaphore(int flag);
extern int adsp_get_semaphore_3way(int flag);
extern int adsp_release_semaphore_3way(int flag);

extern void memcpy_to_adsp(void __iomem *trg, const void *src, int size);
extern void memcpy_from_adsp(void *trg, const void __iomem *src, int size);
extern int reset_adsp(void);

extern phys_addr_t adsp_get_reserve_mem_phys(enum adsp_reserve_mem_id_t id);
extern phys_addr_t adsp_get_reserve_mem_virt(enum adsp_reserve_mem_id_t id);
extern phys_addr_t adsp_get_reserve_mem_size(enum adsp_reserve_mem_id_t id);
extern void adsp_enable_dsp_clk(bool enable);
extern int adsp_check_resource(void);
void set_adsp_mpu(void);
extern phys_addr_t adsp_mem_base_phys;
extern phys_addr_t adsp_mem_base_virt;
extern phys_addr_t adsp_mem_size;

void adsp_reset_ready(enum adsp_core_id id);
void adsp_A_ready_ipi_handler(int id, void *data, unsigned int len);
void adsp_update_memory_protect_info(void);

uint32_t adsp_power_on(uint32_t enable);

#if ADSP_VCORE_TEST_ENABLE
extern unsigned int mt_get_ckgen_freq(unsigned int ID);
extern unsigned int mt_get_abist_freq(unsigned int ID);
#endif
#endif
