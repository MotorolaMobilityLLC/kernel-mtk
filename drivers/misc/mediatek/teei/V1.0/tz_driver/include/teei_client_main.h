/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __TEEI_CLIENT_MAIN_H__
#define __TEEI_CLIENT_MAIN_H__

#ifdef TUI_SUPPORT
#define POWER_DOWN		"power-detect"
#endif
/* #define UT_DEBUG */
#define CANCEL_BUFF_SIZE		(4096)
#define TEEI_CONFIG_FULL_PATH_DEV_NAME "/dev/teei_config"
#define TEEI_CONFIG_DEV "teei_config"
#define TEEI_IOC_MAGIC 'T'
#define TEEI_CONFIG_IOC_MAGIC TEEI_IOC_MAGIC /* "TEEI Client" */
#define TEEI_CONFIG_IOCTL_INIT_TEEI _IOWR(TEEI_CONFIG_IOC_MAGIC, 3, int)
#define TEEI_CONFIG_IOCTL_UNLOCK    _IOWR(TEEI_CONFIG_IOC_MAGIC, 4, int)
#define MIN_BC_NUM              (4)
#define MAX_LC_NUM              (3)

extern void log_boot(char *str);
#define TEEI_BOOT_FOOTPRINT(str) log_boot(str)

enum {
	TEEI_BOOT_OK = 0,
	TEEI_BOOT_ERROR_CREATE_TLOG_BUF = 1,
	TEEI_BOOT_ERROR_CREATE_TLOG_THREAD = 2,
	TEEI_BOOT_ERROR_CREATE_VFS_ADDR = 3,
	TEEI_BOOT_ERROR_LOAD_SOTER_FAILED = 4,
	TEEI_BOOT_ERROR_INIT_CMD_BUFF_FAILED = 5,
	TEEI_BOOT_ERROR_INIT_UTGATE_FAILED = 6,
	TEEI_BOOT_ERROR_INIT_SERVICE1_FAILED = 7,
	TEEI_BOOT_ERROR_INIT_SERVICE2_FAILED = 8,
	TEEI_BOOT_ERROR_LOAD_TA_FAILED = 9,
};

extern struct semaphore api_lock;
extern struct semaphore fp_api_lock;
extern struct semaphore keymaster_api_lock;
extern struct workqueue_struct *secure_wq;
extern struct workqueue_struct *bdrv_wq;
extern unsigned long fdrv_message_buff;
extern unsigned long bdrv_message_buff;
extern unsigned long message_buff;
extern struct semaphore fdrv_sema;
extern unsigned long tlog_message_buff;
extern struct semaphore boot_sema;
extern struct semaphore fdrv_lock;
extern struct completion global_down_lock;
extern unsigned long teei_config_flag;
extern struct semaphore smc_lock;
extern int fp_call_flag;
extern int forward_call_flag;
extern struct smc_call_struct smc_call_entry;
extern int irq_call_flag;
extern unsigned int soter_error_flag;
extern unsigned long boot_vfs_addr;
extern unsigned long boot_soter_flag;
extern int keymaster_call_flag;
extern struct semaphore boot_decryto_lock;
extern struct task_struct *teei_switch_task;
extern struct kthread_worker ut_fastcall_worker;
extern struct mutex pm_mutex;

void ut_pm_mutex_lock(struct mutex *lock);
void ut_pm_mutex_unlock(struct mutex *lock);
void tz_free_shared_mem(void *addr, size_t size);
int get_current_cpuid(void);
void *tz_malloc_shared_mem(size_t size, int flags);
void secondary_init_cmdbuf(void *info);
void secondary_boot_stage2(void *info);
int handle_switch_core(int cpu);
void *tz_malloc(size_t size, int flags);
void secondary_load_tee(void *info);
void secondary_load_tee(void *info);
void secondary_boot_stage1(void *info);
int is_teei_ready(void);

#ifdef CONFIG_MICROTRUST_TZ_LOG
enum {
	TZ_CALL_PREPARE,
	TZ_CALL_RETURNED,
};

struct tz_driver_state {
	struct mutex smc_lock;
	struct atomic_notifier_head notifier;
	struct platform_device *tz_log_pdev;
};

struct tz_driver_state *get_tz_drv_state(void);
int tz_call_notifier_register(struct notifier_block *n);
int tz_call_notifier_unregister(struct notifier_block *n);
#endif

#endif /* __TEEI_CLIENT_MAIN_H__ */
