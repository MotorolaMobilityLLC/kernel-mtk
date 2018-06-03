/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __CCU_CMN_H__
#define __CCU_CMN_H__

#include <linux/wait.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include "ccu_drv.h"

#ifdef MTK_CCU_EMULATOR
/*#define CCUI_OF_M4U_PORT M4U_PORT_CAM_IMGI*/
/*#define CCUI_OF_M4U_PORT M4U_PORT_CAM_CCUI*/
/*#define CCUO_OF_M4U_PORT M4U_PORT_CAM_CCUO*/
/*#define CCUG_OF_M4U_PORT M4U_PORT_CAM_CCUG*/
#else
#define CCUI_OF_M4U_PORT M4U_PORT_CAM_CCUI
#define CCUO_OF_M4U_PORT M4U_PORT_CAM_CCUO
#define CCUG_OF_M4U_PORT M4U_PORT_CAM_CCUG
#endif

/* Common Structure */
typedef enum ccu_req_type_e {
	CCU_IRQ_TYPE_XXX,
	CCU_IRQ_NUM_TYPES
} ccu_req_type_t;

typedef struct ccu_device_s {
	struct proc_dir_entry *proc_dir;
	struct device *dev;
	struct dentry *debug_root;
	unsigned long ccu_base;
	unsigned long camsys_base;
	unsigned long bin_base;
	unsigned long dmem_base;
	unsigned long n3d_a_base;
	unsigned int irq_num;
	struct mutex user_mutex;
	/* list of vlist_type(ccu_user_t) */
	struct list_head user_list;
	/* notify enque thread */
	wait_queue_head_t cmd_wait;
} ccu_device_t;

typedef struct ccu_user_s {
	pid_t open_pid;
	pid_t open_tgid;
	uint32_t id;
	/* to enque/deque must have mutex protection */
	struct mutex data_mutex;

	volatile bool running;
	volatile bool flush;
	/* list of vlist_type(ccu_cmd_st) */
	struct list_head enque_ccu_cmd_list;
	struct list_head deque_ccu_cmd_list;
	wait_queue_head_t deque_wait;
} ccu_user_t;

#define DECLARE_VLIST(type) \
typedef struct { \
		type node; \
		struct list_head link; \
} type ## _list

/*
 * vlist_node_of - get the pointer to the node which has specific vlist
 * @ptr:        the pointer to struct list_head
 * @type:        the type of list node
 */
#define vlist_node_of(ptr, type) ({ \
		const struct list_head *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type ## _list, link)); })

/*
 * vlist_link - get the pointer to struct list_head
 * @ptr:        the pointer to struct vlist
 * @type:        the type of list node
 */
#define vlist_link(ptr, type) (&((type ## _list *)ptr)->link)

/*
 * vlist_type - get the type of struct vlist
 * @type:        the type of list node
 */
#define vlist_type(type) type ## _list

/*
 * vlist_node - get the pointer to the node of vlist
 * @ptr:        the pointer to struct vlist
 * @type:        the type of list node
 */
#define vlist_node(ptr, type)  ((type *) ptr)


typedef struct work_struct work_struct_t;

DECLARE_VLIST(work_struct_t);
DECLARE_VLIST(ccu_user_t);
DECLARE_VLIST(ccu_cmd_st);

/* =============================== define in ccu_hw.c  ================================ */

/**
 * ccu_init_hw - init the procedure related to hw, include irq register and enque thread
 * @device:     the pointer of ccu_device.
 */
int ccu_init_hw(ccu_device_t *device);

/**
 * ccu_uninit_hw - close resources related to hw module
 */
int ccu_uninit_hw(ccu_device_t *device);

/**
 * ccu_mmap_hw - mmap kernel memory to user
 */
int ccu_mmap_hw(struct file *filp, struct vm_area_struct *vma);

/**
 * ccu_send_command - send command, and it will block until done.
 * @cmd:        the pointer to command
 */
int ccu_send_command(ccu_cmd_st *pCmd);

/**
 * ccu_power - config ccu power.
 * @s:          the pointer to power relative settings.
 */
int ccu_power(ccu_power_t *power);

/**
 * ccu_run - start running ccu .
 */
int ccu_run(void);

/**
 * ccu_irq - interrupt wait.
 * @s:          wait mode.
 */
int ccu_waitirq(CCU_WAIT_IRQ_STRUCT *WaitIrq);

/**
 * ccu_irq - interrupt wait.
 * @s:          wait mode.
 */
int ccu_flushLog(int argc, int *argv);

int ccu_memcpy(volatile void *dest, volatile void *src, int length);

int ccu_memclr(volatile void *dest, int length);

/**
 * ccu_i2c_ctrl - i2c control.
 * @argc:          TBD.
 * @argv:          TBD.
 */
int ccu_i2c_ctrl(unsigned char i2c_write_id, int transfer_len);

int ccu_get_i2c_dma_buf_addr(uint32_t *mva);

int ccu_memcpy(volatile void *dest, volatile void *src, int length);

int ccu_memclr(volatile void *dest, int length);

int ccu_read_info_reg(int regNo);

int32_t ccu_get_current_fps(void);

void ccu_get_sensor_i2c_slave_addr(int32_t *sensorI2cSlaveAddr);

void ccu_get_sensor_name(char **sensor_name);

/* =============================== define in ccu_drv.c  =============================== */

/**
 * ccu_create_user - create ccu user, and add to user list
 * @ruser:      return the created user.
 */
int ccu_create_user(ccu_user_t **ruser);

/**
 * ccu_delete_user - delete ccu user, and remove it from user list
 * @user:       the pointer to user.
 */
int ccu_delete_user(ccu_user_t *user);

int ccu_lock_user_mutex(void);

int ccu_unlock_user_mutex(void);

/**
 * ccu_push_command_to_queue - add a command to user's queue
 * @user:       the pointer to user.
 * @cmd:        the command to be added to user's queue.
 */
int ccu_push_command_to_queue(ccu_user_t *user, ccu_cmd_st *cmd);


/**
 * ccu_pop_command_from_queue - remove a command from user's queue
 * @user:       the pointer to user.
 * @rcmd:      return the command to be removed.
 */
int ccu_pop_command_from_queue(ccu_user_t *user, ccu_cmd_st **rcmd);


/**
 * ccu_flush_commands_from_queue - flush all commands of user's queue
 * @user:       the pointer to user.
 *
 * It's a blocking call, and waits for the processing command done.
 * And push all remaining enque to the deque.
 */
int ccu_flush_commands_from_queue(ccu_user_t *user);

/**
 * ccu_clock_enable - Set CCU clock on
 */
int ccu_clock_enable(void);

/**
 * ccu_clock_disable - Set CCU clock off
 */
void ccu_clock_disable(void);

/* LOG & AEE */
#define CCU_TAG "[ccu]"
/*#define LOG_DBG(format, args...)    pr_err(CCU_TAG " " format, ##args)*/
/*#define LOG_INF(format, args...)    pr_err(CCU_TAG " " format, ##args)*/
/*#define LOG_WRN(format, args...)    pr_err(CCU_TAG " " format, ##args)*/

/* #define LOG_DBG(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)*/
/* #define LOG_INF(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)*/
/* #define LOG_WRN(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)*/
/* #define LOG_ERR(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)*/

#define LOG_DBG_MUST(format, args...)    pr_debug(CCU_TAG "[%s] " format, __func__, ##args)
#define LOG_INF_MUST(format, args...)    pr_info(CCU_TAG "[%s] " format, __func__, ##args)
#define LOG_WRN_MUST(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)
#define LOG_INF(format, args...)
#define LOG_WRN(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_err(CCU_TAG "[%s] " format, __func__, ##args)

#define ccu_print_seq(seq_file, fmt, args...) \
		do {\
			if (seq_file)\
				seq_printf(seq_file, fmt, ##args);\
			else\
				pr_err(fmt, ##args);\
		} while (0)

#define ccu_error(format, args...) \
		do {\
			pr_err(CCU_TAG " error:"format, ##args);  \
			aee_kernel_exception("CCU", "[CCU] error:"format, ##args);  \
		} while (0)

#define ccu_aee(format, args...) \
		do {\
			char ccu_name[100];\
			snprintf(ccu_name, 100, CCU_TAG format, ##args); \
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER | DB_OPT_DUMP_DISPLAY, \
			ccu_name, CCU_TAG "error" format, ##args); \
			pr_err(CCU_TAG " error:" format, ##args);  \
		} while (0)
#endif
